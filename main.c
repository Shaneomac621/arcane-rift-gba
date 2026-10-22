#include <gba.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LP 2000
#define DECK_SIZE 12
#define HAND_SIZE 3
#define CARD_COUNT 8
#define RIVAL_COUNT 5

typedef struct { const char *name, *kind, *desc; int cost, power, heal; } Card;
static const Card cards[CARD_COUNT] = {
 {"SHADOW BOLT", "SPELL", "Deal 280 damage.", 1, 280, 0},
 {"RIFT DRAKE", "BEAST", "Deal 420 damage.", 2, 420, 0},
 {"SOUL SPARK", "SPELL", "Restore 260 life.", 1, 0, 260},
 {"ABYSS FANG", "BEAST", "Deal 650 damage.", 3, 650, 0},
 {"VOID DRAGON", "DRAGON", "Deal 900 damage.", 4, 900, 0},
 {"RIFT TITAN", "TITAN", "Deal 1200 damage.", 5, 1200, 0},
 {"DUSK WARD", "SPELL", "Halve the next rival attack.", 2, 0, 0},
 {"ECHO WISP", "BEAST", "Deal 330 damage; heal 80.", 2, 330, 80}
};
static const char *rivals[RIVAL_COUNT] = {"ASH APPRENTICE", "RIFT STALKER", "HOLLOW KNIGHT", "VOID PRIESTESS", "RIFT TITAN"};
static const int rivalLP[RIVAL_COUNT] = {1300, 1650, 2000, 2350, 3000};
static const int rivalHit[RIVAL_COUNT] = {180, 230, 280, 330, 400};
static const char *lore[RIVAL_COUNT] = {
 "A young mage guards the first fracture.",
 "A hunter stalks those who touch the Rift.",
 "An ancient knight rises from the ash.",
 "The priestess feeds the void with souls.",
 "The Titan waits where worlds collapse."
};

typedef enum { SCREEN_TITLE, SCREEN_DECK, SCREEN_DUEL, SCREEN_RESULT, SCREEN_CODEX, SCREEN_MAP } Screen;
static u8 deck[DECK_SIZE] = {0,1,2,0,1,3,2,4,0,5,6,7};
static u8 hand[HAND_SIZE], handCount, deckPos, selected, duelIndex, wins, screen, returnScreen;
static u8 guardNext, turns, shards, unlocked[CARD_COUNT];
static int playerLP, enemyLP, mana, enemyMax;
static char message[72] = "Choose a card and press A.";

static void waitRelease(void) { while (keysDown()) { scanKeys(); VBlankIntrWait(); } }
static void pauseFrame(void) { VBlankIntrWait(); }
static void shuffleDeck(void) {
 int i; for(i=DECK_SIZE-1;i>0;i--) { int j=rand()%(i+1); u8 t=deck[i]; deck[i]=deck[j]; deck[j]=t; }
}
static void drawOne(void) {
 if(handCount>=HAND_SIZE) return;
 if(deckPos>=DECK_SIZE) { deckPos=0; shuffleDeck(); }
 hand[handCount++]=deck[deckPos++];
}
static void drawToHand(void) { while(handCount<HAND_SIZE) drawOne(); }
static void removeHand(int at) {
 int i; for(i=at;i<(int)handCount-1;i++) hand[i]=hand[i+1];
 if(handCount) handCount--;
 if(selected>=handCount && handCount) selected=handCount-1;
}
static void drawTitle(void) {
 consoleClear();
 iprintf("\n      ARCANE RIFT\n");
 iprintf("  SHADOWS WAKE. RIFTS OPEN.\n");
 iprintf("==============================\n\n");
 iprintf("  A     CONTINUE CAMPAIGN\n");
 iprintf("  START DECK WORKSHOP\n");
 iprintf("  SELECT CODEX / CARD LORE\n\n");
 iprintf("  RIFT SHARDS: %d\n", shards);
 iprintf("  RIVALS DEFEATED: %d / %d\n\n", wins, RIVAL_COUNT);
 iprintf("  Original GBA card adventure\n");
}
static void drawMap(void) {
 int i; consoleClear(); iprintf("     RIFT CAMPAIGN MAP\n==============================\n");
 for(i=0;i<RIVAL_COUNT;i++) iprintf("%c %d. %-17s %s\n", i==duelIndex?'>':' ',i+1,rivals[i],i<wins?"SEALED":"OPEN");
 iprintf("\nA: challenge   B: title\n");
 iprintf("\n%s",lore[duelIndex]);
}
static void drawDeck(void) {
 int i; consoleClear(); iprintf("     DECK WORKSHOP\n==============================\n");
 for(i=0;i<DECK_SIZE;i++) iprintf("%c%02d %-14s C:%d\n",i==selected?'>':' ',i+1,cards[deck[i]].name,cards[deck[i]].cost);
 iprintf("\nUP/DOWN: browse  A: details\nB: back to previous screen\n");
 iprintf("\n%s\n%s",cards[deck[selected]].kind,cards[deck[selected]].desc);
}
static void drawCodex(void) {
 int i; consoleClear(); iprintf("       RIFT CODEX\n==============================\n");
 for(i=0;i<CARD_COUNT;i++) iprintf("%c %-14s %s\n",unlocked[i]?'*':'-',cards[i].name,unlocked[i]?"KNOWN":"???");
 iprintf("\n* Unlocked card\nB: back to title\n");
 iprintf("\nDefeat rivals to earn shards.\n");
}
static void beginDuel(void) {
 int i; if(duelIndex>=RIVAL_COUNT) duelIndex=RIVAL_COUNT-1;
 playerLP=MAX_LP; enemyMax=rivalLP[duelIndex]; enemyLP=enemyMax; mana=3; deckPos=0; guardNext=0; selected=0; handCount=0; turns=1;
 for(i=0;i<DECK_SIZE;i++) { /* preserve deck composition; shuffle draw order */ }
 shuffleDeck(); drawToHand(); screen=SCREEN_DUEL; strcpy(message,"Your turn. Play any affordable cards.");
}
static void drawDuel(void) {
 int i; consoleClear();
 iprintf("      ARCANE RIFT  DUEL %d/%d\n",duelIndex+1,RIVAL_COUNT);
 iprintf("==============================\n");
 iprintf("RIVAL: %s\n",rivals[duelIndex]);
 iprintf("RIVAL LP: %4d / %d\n",enemyLP,enemyMax);
 iprintf("YOUR  LP: %4d / %d\n",playerLP,MAX_LP);
 iprintf("MANA: %d/5  TURN: %d  SHARDS:%d\n\n",mana,turns,shards);
 iprintf("YOUR HAND\n");
 for(i=0;i<HAND_SIZE;i++) {
   if(i<handCount) { const Card *c=&cards[hand[i]]; iprintf("%c %-14s C:%d D:%d H:%d\n",i==selected?'>':' ',c->name,c->cost,c->power,c->heal); }
   else iprintf("  -- empty --\n");
 }
 iprintf("\n%s\n",message);
 iprintf("LEFT/RIGHT select  A play card\nB end turn  SELECT deck  START title\n");
}
static void drawResult(void) {
 consoleClear();
 if(enemyLP<=0) {
   iprintf("       RIFT SEALED!\n==============================\n\n");
   iprintf("You defeated %s!\n\n",rivals[duelIndex]);
   iprintf("Reward: +1 RIFT SHARD\n");
   iprintf("New cards may enter the codex.\n\n");
 } else {
   iprintf("        DEFEAT...\n==============================\n\n");
   iprintf("The rift remains open.\nRegroup and try again.\n\n");
 }
 iprintf("A: continue   B: campaign map\n");
}
static void finishRival(void) {
 if(enemyLP<=0) {
   if(duelIndex>=wins) wins=duelIndex+1;
   shards++;
   if(duelIndex==0) unlocked[7]=1;
   if(duelIndex==1) unlocked[3]=1;
   if(duelIndex==2) unlocked[4]=1;
   if(duelIndex==3) unlocked[5]=1;
   if(duelIndex==4) unlocked[6]=1;
   strcpy(message,"Victory! Rift Shard earned.");
 } else strcpy(message,"Defeat. Try a different card strategy.");
 screen=SCREEN_RESULT;
}
static void endTurn(void) {
 int hit=rivalHit[duelIndex];
 if(enemyLP<=0) { finishRival(); return; }
 if(guardNext) { hit=(hit+1)/2; guardNext=0; }
 playerLP-=hit; if(playerLP<0)playerLP=0;
 if(playerLP<=0) { finishRival(); return; }
 turns++; mana=mana<5?mana+2:5; drawOne();
 if(handCount==0) drawToHand();
 selected=0; snprintf(message,sizeof(message),"Rival hits for %d. Your turn!",hit);
}
static void playCard(void) {
 int ci; const Card *c;
 if(!handCount) { strcpy(message,"No cards in hand. End turn to draw."); return; }
 ci=hand[selected]; c=&cards[ci];
 if(c->cost>mana) { strcpy(message,"Not enough mana. Choose a cheaper card."); return; }
 mana-=c->cost;
 if(c->power>0) { enemyLP-=c->power; if(enemyLP<0)enemyLP=0; }
 if(c->heal>0) { playerLP+=c->heal; if(playerLP>MAX_LP)playerLP=MAX_LP; }
 if(ci==6) guardNext=1;
 snprintf(message,sizeof(message),"%s activated!",c->name);
 removeHand(selected); drawOne();
 if(enemyLP<=0) finishRival();
}
int main(void) {
 int i; irqInit(); irqEnable(IRQ_VBLANK); consoleDemoInit();
 srand((unsigned int)REG_VCOUNT+0xA71u);
 for(i=0;i<CARD_COUNT;i++) unlocked[i]=(i<6);
 screen=SCREEN_TITLE; returnScreen=SCREEN_TITLE;
 while(1) {
   scanKeys(); u16 k=keysDown();
   switch(screen) {
    case SCREEN_TITLE:
     drawTitle(); pauseFrame();
     if(k&KEY_A) { screen=SCREEN_MAP; if(duelIndex>=RIVAL_COUNT)duelIndex=0; }
     else if(k&KEY_START) { selected=0; returnScreen=SCREEN_TITLE; screen=SCREEN_DECK; }
     else if(k&KEY_SELECT) { screen=SCREEN_CODEX; }
     break;
    case SCREEN_MAP:
     drawMap(); pauseFrame();
     if(k&KEY_A) beginDuel();
     else if(k&KEY_B) screen=SCREEN_TITLE;
     else if(k&KEY_UP) duelIndex=(duelIndex+RIVAL_COUNT-1)%RIVAL_COUNT;
     else if(k&KEY_DOWN) duelIndex=(duelIndex+1)%RIVAL_COUNT;
     break;
    case SCREEN_DECK:
     drawDeck(); pauseFrame();
     if(k&KEY_UP) selected=(selected+DECK_SIZE-1)%DECK_SIZE;
     else if(k&KEY_DOWN) selected=(selected+1)%DECK_SIZE;
     else if(k&KEY_A) { /* A is a safe card inspection action; details remain below list. */ }
     else if(k&KEY_B) screen=returnScreen;
     break;
    case SCREEN_CODEX:
     drawCodex(); pauseFrame(); if(k&KEY_B)screen=SCREEN_TITLE; break;
    case SCREEN_DUEL:
     drawDuel(); pauseFrame();
     if(k&KEY_LEFT) selected=handCount?(selected+handCount-1)%handCount:0;
     else if(k&KEY_RIGHT) selected=handCount?(selected+1)%handCount:0;
     else if(k&KEY_A) playCard();
     else if(k&KEY_B) endTurn();
     else if(k&KEY_SELECT) { returnScreen=SCREEN_DUEL; selected=0; screen=SCREEN_DECK; }
     else if(k&KEY_START) screen=SCREEN_TITLE;
     break;
    case SCREEN_RESULT:
     drawResult(); pauseFrame();
     if(k&KEY_A) { if(enemyLP<=0) { if(duelIndex<RIVAL_COUNT-1)duelIndex++; else screen=SCREEN_TITLE; } if(screen==SCREEN_RESULT)beginDuel(); }
     else if(k&KEY_B) screen=SCREEN_MAP;
     break;
    default: screen=SCREEN_TITLE; break;
   }
   waitRelease();
 }
 return 0;
}
