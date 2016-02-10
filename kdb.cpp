#include <stdio.h>
#include "kdb.h"

#ifdef MYDLL_RELEASE_BUILD
#define KDB_DEBUG(f,x)
#define KDB_DEBUG_OPEN(f, dir)
#define KDB_DEBUG_CLOSE(f)
#else
#define KDB_DEBUG(f,x) if (f != NULL) fprintf x
#define KDB_DEBUG_OPEN(f, dir) {\
	char buf[4096]; \
	ZeroMemory(buf, 4096); \
	lstrcpy(buf, dir); lstrcat(buf, "KDB.debug.log"); \
	f = fopen(buf, "wt"); \
}
#define KDB_DEBUG_CLOSE(f) if (f != NULL) { fclose(f); f = NULL; }
#endif

#define GROUP_PLAYERS 0
#define GROUP_GOALKEEPERS 1
#define GROUP_BALLS 2

#define PLAYERS 0
#define GOALKEEPERS 1

static FILE* klog;

// functions
//////////////////////////////////////////

static int getTeamNumber(char* fileName);
static KitEntry* buildKitList(char* dir, int kitType);
static void MakeKonamiColor(char* str, BYTE* buf);

KDB* kdbLoad(char* dir)
{
	KDB_DEBUG_OPEN(klog, dir);
	KDB_DEBUG(klog, (klog, "Loading KDB...\n"));

	// initialize an empty database
	KDB* kdb = (KDB*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(KDB));
	if (kdb == NULL) { fclose(klog); return NULL; }
	kdb->playerCount = kdb->goalkeeperCount = 0;
	ZeroMemory(kdb->players, sizeof(KitEntry*) * 256);
	ZeroMemory(kdb->goalkeepers, sizeof(KitEntry*) * 256);

	// get list of player kits
	KitEntry* kits = buildKitList(dir, PLAYERS);

	// traverse the list and place the kits into the database
	KitEntry* p = kits;
	KitEntry* q = NULL;
	while (p != NULL)
	{
		q = p->next;
		KDB_DEBUG(klog, (klog, "p->kit->filename = %s\n", p->kit->fileName));

		int n = getTeamNumber(p->kit->fileName);
		if (n >= 0 && n <= 255) 
		{
			p->next = kdb->players[n];
			kdb->players[n] = p;
			kdb->playerCount += 1;

			KDB_DEBUG(klog, (klog, "[%03d] : %s\n", n, p->kit->fileName));
		}
		p = q;
	}

	// get list of goalkeeper kits
	kits = buildKitList(dir, GOALKEEPERS);

	// traverse the list and place the kits into the database
	p = kits;
	q = NULL;
	while (p != NULL)
	{
		q = p->next;
		KDB_DEBUG(klog, (klog, "p->kit->filename = %s\n", p->kit->fileName));

		int n = getTeamNumber(p->kit->fileName);
		if (n >= 0 && n <= 255) 
		{
			p->next = kdb->goalkeepers[n];
			kdb->goalkeepers[n] = p;
			kdb->goalkeeperCount += 1;

			KDB_DEBUG(klog, (klog, "[%03d] : %s\n", n, p->kit->fileName));
		}
		p = q;
	}

	// read the attributes from attrib.cfg file
	int currTeam = -1;
	Kit* currUni = NULL;
	Ball* currBall = NULL;

	// default is players 
	// (for compatibility with attrib.cfg files from older kitservers
	int group = GROUP_PLAYERS; 

	char buf[4096];
	ZeroMemory(buf, 4096);
	lstrcpy(buf, dir); lstrcat(buf, "KDB\\attrib.cfg");

	FILE* att = fopen(buf, "rt");
	if (att != NULL)
	{
		// go line by line
		while (!feof(att))
		{
			ZeroMemory(buf, 4096);
			fgets(buf, 4096, att);
			if (lstrlen(buf) == 0) break;

			// strip off comments
			char* comm = strstr(buf, "#");
			if (comm != NULL) comm[0] = '\0';

			// look for start of the section
			char* start = strstr(buf, "[");
			if (start != NULL) 
			{
				char* end = strstr(start+1,"]");
				if (end != NULL)
				{
					// new section
					char name[255];
					ZeroMemory(name, 255);
					lstrcpy(name, start+1);
					name[end-start-1] = '\0';

					KDB_DEBUG(klog, (klog, "section: {%s}\n", name));

					// check if we changed groups
					if (lstrcmpi(name, "Players")==0) 
					{ 
						group = GROUP_PLAYERS; 
						currTeam = -1; currUni = NULL;
						continue; 
					}
					else if (lstrcmpi(name, "Goalkeepers")==0) 
					{ 
						group = GROUP_GOALKEEPERS; 
						currTeam = -1; currUni = NULL;
						continue; 
					}
					else if (lstrcmpi(name, "Balls")==0) 
					{ 
						group = GROUP_BALLS; 
						currTeam = -1; currUni = NULL;
						continue; 
					}

					if (group == GROUP_PLAYERS)
					{
						// this has to be a kit file
						start[7] = '\0';
						if (sscanf(start+4,"%d",&currTeam)==1)
						{
							KDB_DEBUG(klog, (klog, "looking up uniform for team: {%d}\n", currTeam));
							// lookup name in KDB
							KitEntry* p = kdb->players[currTeam];
							while (p != NULL)
							{
								if (lstrcmp(p->kit->fileName, name)==0)
								{
									currUni = p->kit;
									break;
								}
								p = p->next;
							}
							if (p == NULL) {
								currTeam = -1; // didn't find such uniform in KDB
								currUni = NULL;
							}
						}
					}

					else if (group == GROUP_GOALKEEPERS)
					{
						// this has to be a kit file
						start[7] = '\0';
						if (sscanf(start+4,"%d",&currTeam)==1)
						{
							KDB_DEBUG(klog, (klog, "looking up uniform for team: {%d}\n", currTeam));
							// lookup name in KDB
							KitEntry* p = kdb->goalkeepers[currTeam];
							while (p != NULL)
							{
								if (lstrcmp(p->kit->fileName, name)==0)
								{
									currUni = p->kit;
									break;
								}
								p = p->next;
							}
							if (p == NULL) {
								currTeam = -1; // didn't find such uniform in KDB
								currUni = NULL;
							}
						}
					}

					else if (group == GROUP_BALLS)
					{
						// this has to be a ball file
						KDB_DEBUG(klog, (klog, "New ball key: {%s}\n", name));
						currBall = (Ball*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Ball));
						if (currBall != NULL)
						{
							lstrcpy(currBall->texFileName, name);

							// insert into list of balls
							BallEntry* be = (BallEntry*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BallEntry));
							if (be != NULL)
							{
								be->ball = currBall;
								be->next = kdb->balls;
								kdb->balls = be;
							}
						}
					}
				} // end if (end ...
			}
			
			// otherwise try to read attributes
			else 
			{
				if (currTeam != -1 && (group == GROUP_PLAYERS || group == GROUP_GOALKEEPERS))
				{
					char key[80]; ZeroMemory(key, 80);
					char value[80]; ZeroMemory(value, 80);

					if (sscanf(buf,"%s = %s",key,value)==2)
					{
						KDB_DEBUG(klog, (klog, "key: {%s} has value: {%s}\n", key, value));

						BYTE color[6];
						if (lstrcmp(key, "shirt.backNumber")==0)
						{
							MakeKonamiColor(value, color);
							memcpy(currUni->shirtBackNumber, color, 6);
							currUni->attDefined |= SHIRT_BACK_NUMBER;
						}
						else if (lstrcmp(key, "shirt.backName")==0)
						{
							MakeKonamiColor(value, color);
							memcpy(currUni->shirtBackName, color, 6);
							currUni->attDefined |= SHIRT_BACK_NAME;
						}
						else if (lstrcmp(key, "shirt.frontNumber")==0)
						{
							MakeKonamiColor(value, color);
							memcpy(currUni->shirtFrontNumber, color, 6);
							currUni->attDefined |= SHIRT_FRONT_NUMBER;
						}
						else if (lstrcmp(key, "shorts.number")==0)
						{
							MakeKonamiColor(value, color);
							memcpy(currUni->shortsNumber, color, 6);
							currUni->attDefined |= SHORTS_NUMBER;
						}
						else if (lstrcmp(key, "collar")==0)
						{
							currUni->collar = (lstrcmp(value,"yes")==0) ? 0 : 2;
							currUni->attDefined |= COLLAR;
						}
					}
				}
				else 
				{
					if (group == GROUP_BALLS && currBall != NULL)
					{
						char key[255]; ZeroMemory(key, 255);

						sscanf(buf, "%s", key);
						if (lstrcmp(key, "ball.name")==0)
						{
							char* startQuote = strstr(buf, "\"");
							if (startQuote == NULL) continue;
							char* endQuote = strstr(startQuote + 1, "\"");
							if (endQuote == NULL) continue;

							memcpy(currBall->name, 
									startQuote + 1, endQuote - startQuote - 1);

							KDB_DEBUG(klog, (klog, "key: {%s} has value: {%s}\n", key, currBall->name));
						}
						else if (lstrcmp(key, "ball.model")==0)
						{
							char* startQuote = strstr(buf, "\"");
							if (startQuote == NULL) continue;
							char* endQuote = strstr(startQuote + 1, "\"");
							if (endQuote == NULL) continue;

							memcpy(currBall->mdlFileName, 
									startQuote + 1, endQuote - startQuote - 1);

							KDB_DEBUG(klog, (klog, "key: {%s} has value: {%s}\n", key, currBall->mdlFileName));
						}
					}
				}
			} // end else

		}
		fclose(att);
	}
	KDB_DEBUG(klog, (klog, "attributes read.\n"));

	// cycle the uniform lists, adding a "dummy" kits
	// and a "dummy" ball as well
	HANDLE heap = GetProcessHeap();
	for (int i=0; i<256; i++)
	{
		// PLAYERS
		KitEntry* p = kdb->players[i];
		if (p != NULL)
		{
			KitEntry* dummy = (KitEntry*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(KitEntry));
			dummy->kit = (Kit*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Kit));
			dummy->kit->fileName[0] = 'x';
			dummy->kit->attDefined = 0;
			dummy->next = NULL;

			// append dummy kit at the end
			while (p->next != NULL) p = p->next;
			p->next = dummy;
			dummy->next = kdb->players[i];
		}

		// GOALKEEPERS
		p = kdb->goalkeepers[i];
		if (p != NULL)
		{
			KitEntry* dummy = (KitEntry*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(KitEntry));
			dummy->kit = (Kit*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Kit));
			dummy->kit->fileName[0] = 'x';
			dummy->kit->attDefined = 0;
			dummy->next = NULL;

			// append dummy kit at the end
			while (p->next != NULL) p = p->next;
			p->next = dummy;
			dummy->next = kdb->goalkeepers[i];
		}
	}
	KDB_DEBUG(klog, (klog, "PL and GK cycled.\n"));

	// insert the dummy ball at the front
	if (kdb->balls != NULL)
	{
		BallEntry* dummy = (BallEntry*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(BallEntry));
		dummy->ball = (Ball*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Ball));
		dummy->ball->texFileName[0] = 'x';
		dummy->next = kdb->balls;
		kdb->balls = dummy;
	}

	// now reverse the list
	BallEntry* reversed = NULL;
	BallEntry* bp = kdb->balls;
	while (bp != NULL)
	{
		BallEntry* bq = bp->next;
		bp->next = reversed;
		reversed = bp;
		bp = bq;
	}
	kdb->balls = reversed;
	KDB_DEBUG(klog, (klog, "balls reversed.\n"));

	// now cycle it
	bp = kdb->balls;
	if (bp != NULL)
	{
		while (bp->next != NULL) bp = bp->next;
		bp->next = kdb->balls;
	}
	KDB_DEBUG(klog, (klog, "balls cycled.\n"));

	KDB_DEBUG(klog, (klog, "KDB loaded.\n"));
	KDB_DEBUG_CLOSE(klog);
	return kdb;
}

// return an integer number, parsed out of
// "uniXXX...bin" file name.
int getTeamNumber(char* fileName)
{
	char buf[255];
	ZeroMemory(buf, 255);
	lstrcpy(buf, fileName + 3);
	buf[3] = '\0';

	int n = 0;
	if (sscanf(buf, "%d", &n)!=1) return -1;
	return n;
}

// parses a RRGGBB string into array of bytes
// that specify Konami color: r,0,g,0,b,0.
void MakeKonamiColor(char* str, BYTE* buf)
{
	ZeroMemory(buf, 6);
	if (lstrlen(str) != 6) return;
	int num = 0;
	if (sscanf(str,"%x",&num)!=1) return;
	buf[0] = (BYTE)((num >> 16) & 0xff);
	buf[2] = (BYTE)((num >> 8) & 0xff);
	buf[4] = (BYTE)(num & 0xff);
	KDB_DEBUG(klog, (klog, "Konami color: %x,%x,%x,%x,%x,%x\n",
				buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]));
}

KitEntry* buildKitList(char* dir, int kitType)
{
	WIN32_FIND_DATA fData;
	char pattern[4096];
	ZeroMemory(pattern, 4096);

	KitEntry* list = NULL;
	KitEntry* p = NULL;
	HANDLE heap = GetProcessHeap();

	lstrcpy(pattern, dir); 
	if (kitType == PLAYERS) lstrcat(pattern, "KDB\\players\\uni???*.bin");
	else if (kitType == GOALKEEPERS) lstrcat(pattern, "KDB\\goalkeepers\\uni???*.bin");

	KDB_DEBUG(klog, (klog, "pattern = {%s}\n", pattern));

	HANDLE hff = FindFirstFile(pattern, &fData);
	if (hff == INVALID_HANDLE_VALUE) 
	{
		// none found.
		return NULL;
	}

	while(true)
	{
		// build new list element
		Kit* kit = (Kit*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Kit));
		if (kit != NULL)
		{
			ZeroMemory(kit->fileName, 255);
			lstrcpy(kit->fileName, fData.cFileName);
			ZeroMemory(kit->shirtBackNumber, 6);
			ZeroMemory(kit->shirtBackName, 6);
			ZeroMemory(kit->shirtFrontNumber, 6);
			ZeroMemory(kit->shortsNumber, 6);
			kit->collar = 2;
			kit->attDefined = 0;

			p = (KitEntry*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(KitEntry));
			if (p != NULL)
			{
				// insert new element
				p->kit = kit;
				p->next = list;
				list = p;

				KDB_DEBUG(klog, (klog, "list->kit->fileName = {%s}\n", list->kit->fileName));
			}
			else 
			{
				HeapFree(heap, 0, kit);
			}
		}

		// proceed to next file
		if (!FindNextFile(hff, &fData)) break;
	}

	FindClose(hff);
	return list;
}

void kdbUnload(KDB* kdb)
{
	if (kdb == NULL) return;
	HANDLE heap = GetProcessHeap();
	KitEntry* q = NULL;
	BallEntry* bq = NULL;

	// free player and goalkeeper kits memory
	for (int i=0; i<256; i++)
	{
		// PLAYERS
		KitEntry* p = kdb->players[i];
		while (p != NULL)
		{
			q = p->next;
			HeapFree(heap, 0, p->kit);
			HeapFree(heap, 0, p);
			if (q == kdb->players[i]) break; // full cycle
			p = q;
		}

		// GOALKEEPERS
		p = kdb->goalkeepers[i];
		while (p != NULL)
		{
			q = p->next;
			HeapFree(heap, 0, p->kit);
			HeapFree(heap, 0, p);
			if (q == kdb->goalkeepers[i]) break; // full cycle
			p = q;
		}
	}

	// free balls memory
	BallEntry* be = kdb->balls;
	while (be != NULL)
	{
		bq = be->next;
		HeapFree(heap, 0, be->ball);
		HeapFree(heap, 0, be);
		if (bq == kdb->balls) break; // full cycle
		be = bq;
	}
}

