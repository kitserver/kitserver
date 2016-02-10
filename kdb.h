#ifndef __JUCE_KDB__
#define __JUCE_KDB__

#include <windows.h>

// attribute definition flags (bits)
#define SHIRT_BACK_NUMBER  0x01 
#define SHIRT_FRONT_NUMBER 0x02 
#define SHIRT_BACK_NAME    0x04 
#define SHORTS_NUMBER      0x08 
#define COLLAR             0x10 

// KDB data structures
///////////////////////////////

typedef struct _Kit {
	char fileName[255];
	BYTE shirtBackNumber[6];
	BYTE shirtBackName[6];
	BYTE shirtFrontNumber[6];
	BYTE shortsNumber[6];
	BYTE collar;
	BYTE attDefined;

} Kit;

typedef struct _Ball {
	char name[255];
	char texFileName[255];
	char mdlFileName[255];

} Ball;

typedef struct _KitEntry {
	Kit* kit;
	struct _KitEntry* next;

} KitEntry;

typedef struct _BallEntry {
	Ball* ball;
	struct _BallEntry* next;

} BallEntry;

typedef struct _KDB {
	int playerCount;
	int goalkeeperCount;
	KitEntry* players[256];
	KitEntry* goalkeepers[256];
	BallEntry* balls;

} KDB;


// KDB functions
//////////////////////////////

KDB* kdbLoad(char* dir);
void kdbUnload(KDB* kdb);

#endif
