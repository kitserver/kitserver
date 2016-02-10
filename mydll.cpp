/* mydll */

/**************************************
 * Visual indicators meaning:         *
 * BLUE STAR: kdb is not empty        *
 **************************************/

#include <windows.h>
#define _COMPILING_MYDLL
#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "mydll.h"
#include "shared.h"
#include "config.h"
#include "imageutil.h"
#include "kdb.h"
#include "log.h"

#include "d3dfont.h"
#include "dxutil.h"
#include "d3d8types.h"
#include "d3d8.h"

#define MAX_FILEPATH BUFLEN

#define IDLE -1
#define START 0
#define DOHOOK 1
#define SWITCHTEAM 2
#define FAKE1 3
#define FAKE2 4
#define FAKEGLOVES 5
#define SWITCHTEAMBACK 6
#define REAL1 7
#define REAL2 8
#define REALGLOVES 9
#define DOUNHOOK 10
#define STOP 11

#define HOST1 0
#define HOST2 2
#define GUEST1 1
#define GUEST2 3

#define SAFEFREE(ptr) if (ptr) { HeapFree(GetProcessHeap(),0,ptr); ptr=NULL; }

#define IS_STRIP_SELECTION (*((BYTE*)data[STRIP_SELECTION]) == 0x02)
#define ABOUT_TO_LOAD_STRIPS (*((BYTE*)data[STRIP_SELECTION]) == 0x01)

#define TEAM_CODE(n) (CheckTeam((DWORD)(*((BYTE*)(data[TEAM_NUM]+n)))) & 0xff)
//#define TEAM_CODE(n) (*((BYTE*)(data[TEAM_NUM]+n)))

#define MAX_ITERATIONS 1000

/**************************************
Shared by all processes variables
**************************************/
#pragma data_seg(".HKT")
KSERV_CONFIG g_config = {
	DEFAULT_DEBUG,
	DEFAULT_KDB_DIR,
	DEFAULT_VKEY_HOMEKIT,
	DEFAULT_VKEY_AWAYKIT,
	DEFAULT_VKEY_GKHOMEKIT,
	DEFAULT_VKEY_GKAWAYKIT,
	DEFAULT_VKEY_BALL,
	DEFAULT_VKEY_RANDOM_BALL,
};
#pragma data_seg()
#pragma comment(linker, "/section:.HKT,rws")

/************************** 
End of shared section 
**************************/

bool g_fontInitialized = false;
CD3DFont* g_font = NULL;

#define PES3_1_0 0
#define PES3_1_10_2 1
#define PES3_1_30_1 2
#define WE7I_US 3
char* GAME[] = { "PES3 1.0", "PES3 1.10.2", "PES3 1.30.1", "WE7I US" };

#define CODELEN 18
#define DATALEN 12

char* WHICH_TEAM[] = { "HOME", "AWAY" };

// code array names
// NOTE: "_CS" stands for: "call site"
enum {
	C_LOADUNI, C_QUICKSELECT, C_LOADCOLLARS, C_LOADUNI_CS, C_LOADCOLLARS_CS, C_QUICKSELECT_CS,
	C_RELOADTEAMS, C_REREADNUMS, C_REREADCOLLARS, C_DECODEBIN, C_DECODEBIN_CS,
	C_LOADBALL, C_LOADBALL_CS, C_DECODEBIN_CS2, C_CHECKTEAM, C_CHECKTEAM_CS,
};

// data array names
enum {
	TEAM_NUM, PARAM_HOME, PARAM_AWAY, LPDEV, STRIPS, STRIP_SELECTION, COLORS, COLLARS, NUMTYPES,
	MENU_FLAG, EXHIB_MENU_ID, QUICKSTART_MENU_ID
};

// Code addresses. Order: PES3 1.0, PES3 1.10.2, PES3 1.30.1, WE7I US 
DWORD codeArray[4][CODELEN] = { 
	{ 0x509510, 0x50b380, 0x50a9a0, 0x509c94, 0x57f58c, 0x77bdfd, 0x57a740, 0x50f1f0, 0x50d020,
	  0x842e10, 0x842f07, 0x842ec0, 0x50c205, 0x50952d, 0x838d60, 0x50a9c6 },
	{ 0x509e30, 0x50bca0, 0x50b2c0, 0x50a5b4, 0x57ff6c, 0x77c5bd, 0x57b110, 0x50fb10, 0x50d940,
	  0x8435c0, 0x8436b7, 0x843670, 0x50cb25, 0x509e4d, 0, 0 },
	{ 0x50a470, 0x50c2e0, 0x50b900, 0x50abf4, 0x57fcbc, 0x77c3bd, 0x57ae70, 0x510160, 0x50df90,
	  0x843380, 0x843477, 0x843430, 0x50d175, 0x50a48d, 0, 0 },
	{ 0x50a570, 0x50c3e0, 0x50ba00, 0x50acf4, 0x5800dc, 0x77c99d, 0x57b260, 0x510260, 0x50e090,
	  0x843db0, 0x843ea7, 0x843e60, 0x50d275, 0x50a58d, 0, 0 },
};

// Data addresses. Order: PES3 1.0, PES3 1.10.2, PES3 1.30.1, WE7I US
DWORD dataArray[4][DATALEN] = {
	{ 0xe99320, 0xd517e0, 0xd54c20, 0xc82a90, 0xe9934a, 0xd5d28c, 0x906db8, 0x9058a4, 0x905898,
	  0xd5d2a0, 0xd5d2c0, 0xd5d2c4 },
	{ 0xebbce0, 0xd741a0, 0xd775e0, 0xca53d0, 0xebbd0a, 0xd7fc4c, 0x906e18, 0x905904, 0x9058f8,
	  0xd7fc60, 0xd7fc80, 0xd7fc84 },
	{ 0xede5a0, 0xd96a20, 0xd99e60, 0xcc7cc0, 0xede5ca, 0xda250c, 0x906db8, 0x9058a4, 0x905898,
	  0xda2520, 0xda2540, 0xda2544 },
	{ 0xeda760, 0xd92bb0, 0xd95ff0, 0xcc3e60, 0xeda78a, 0xd9e69c, 0x903dd0, 0x9028bc, 0x9028b0,
	  0xd9e6b0, 0xd9e6d0, 0xd9e6d4 },
};

DWORD code[CODELEN];
DWORD data[DATALEN];

#define NUM_BALLS 2

char* g_ballMdl = NULL;
char* g_ballTex = NULL;

char* unibuf1[] = {NULL,NULL,NULL,NULL};
char* unibuf2[] = {NULL,NULL,NULL,NULL};
char kitFilename[4][1024];  // filenames for HOST1,HOST2,GUEST1,GUEST2

char* gkUniBuf[] = {NULL,NULL};
char* gkUniBuf2[] = {NULL,NULL};

bool bToggleUni = false;
int g_loadUniCount = 0;
int g_uniToggleStep = -1;
int g_uniToChange = -1;
BYTE g_teamNum = 0;
bool g_kitsAvailable = false;
bool g_ballsAvailable = false;

BYTE* g_strip = NULL;

BYTE g_MainOrExhib = 0;
BYTE g_ScreenId = 0;
BYTE g_QS_ScreenId = 0;

////// HACK //////////////////
BYTE* g_etcEeTex = NULL;
BYTE* g_eplNumbers = NULL;
//BYTE* g_saveNumbers = NULL;
DWORD g_numberOffsets[] = { 0x0000add0, 0x0000fed0, 0x00014fd0, 0x0001a0d0, };

typedef struct _NumberData
{
	BOOL valid;               // TRUE, if structure currently contains valid data
	int type;                 // new "temporary" number type
	int savedType;            // original number type
	int savedTeam;            // team number
	BYTE shape[0x5100];       // new "temporary" shape of the number
	BYTE savedShape[0x5100];  // original number shape

} NumberData;

NumberData g_homeNumberData;
NumberData g_awayNumberData;

// text labels for default strips
char* DEFAULT_LABEL[] = { "1st default", "2nd default" };

// Kit Database
KDB* kDB = NULL;

KitEntry* kitCycle[] = {NULL, NULL, NULL, NULL};
Kit* prevKit[] = {NULL, NULL, NULL, NULL};
KitEntry* gkCycle[] = {NULL, NULL};
BallEntry* ballCycle = NULL;
char g_ballName[255];

// Name-and-Number Colors structures
// (as they are stored in memory)
typedef struct _NNStrip
{
	BYTE backNumber[6];
	BYTE frontNumber[6];
	BYTE shortsNumber[6];
	BYTE backName[6];
	BYTE radar[2];

} NNStrip;

typedef struct _NNColors {
	NNStrip p1;
	NNStrip p2;
	NNStrip g1;
	NNStrip g2;

} NNColors;

// Pointers for memory locations of colors
NNColors* homeNN = NULL;
NNColors* awayNN = NULL;

// Containers for saving existing colors
NNColors saveHome;
NNColors saveAway;

// Pointers for memory locations of colors and collars
BYTE* homeCollars = NULL;
BYTE* awayCollars = NULL;

// Containers for saving existing colors and collars
BYTE saveHomeCollars[4];
BYTE saveAwayCollars[4];

bool extraInfoSaved = false;
bool extraGKInfoSaved = false;

///// Graphics ////////////////

struct CUSTOMVERTEX { 
	FLOAT x,y,z,w;
	DWORD color;
};

IDirect3DDevice8* g_dev = NULL;
IDirect3DVertexBuffer8* g_pVB0 = NULL;
IDirect3DVertexBuffer8* g_pVB1 = NULL;
IDirect3DVertexBuffer8* g_pVB2 = NULL;
IDirect3DVertexBuffer8* g_pVB3 = NULL;
DWORD g_dwSavedStateBlock = 0L;
DWORD g_dwDrawTextStateBlock = 0L;

#define IHEIGHT 30.0f
#define IWIDTH IHEIGHT
#define INDX 8.0f
#define INDY 8.0f
float POSX = 0.0f;
float POSY = 0.0f;

// star coordinates.
float PI = 3.1415926f;
float R = ((float)IHEIGHT)/2.0f;
float d18 = PI/10.0f;
float d54 = d18*3.0f;
float y[] = { R*sin(d18), R, R*sin(d18), -R*sin(d54), -R*sin(d54), R*sin(d18), R*sin(d18) }; 
float x[] = { R*cos(d18), 0.0f, -R*cos(d18), -R*cos(d54), R*cos(d54) };
float x5 = x[4]*(y[1] - y[5])/(y[1] - y[4]);
float x6 = -x5;

CUSTOMVERTEX g_Vert0[] =
{
	{POSX+INDX+R + x[1], POSY+INDY+R - y[1], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x6,   POSY+INDY+R - y[6], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x5,   POSY+INDY+R - y[5], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x[2], POSY+INDY+R - y[2], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x[4], POSY+INDY+R - y[4], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x5,   POSY+INDY+R - y[5], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x[3], POSY+INDY+R - y[3], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x[0], POSY+INDY+R - y[0], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x6,   POSY+INDY+R - y[6], 0.0f, 1.0f, 0xff4488ff },
};


#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

#define HEAP_SEARCH_INTERVAL 500  // in msec
#define DLLMAP_PAUSE 1000         // in msec

BITMAPINFOHEADER bitmapHeader;

DWORD* releasePtr = NULL;
DWORD* resetPtr = NULL;
DWORD* presentPtr = NULL;

#ifndef MYDLL_RELEASE_BUILD
#define TRACE(x) Debug(x)
#define TRACE2(x,y) DebugWithNumber(x,y)
#define TRACE3(x,y) DebugWithDouble(x,y)
#else
#define TRACE(x) 
#define TRACE2(x,y) 
#define TRACE3(x,y) 
#endif

IDirect3DSurface8* g_backBuffer;
D3DFORMAT g_bbFormat;
BOOL g_bGotFormat = false;
int bpp = 0;

typedef DWORD   (STDMETHODCALLTYPE *GETPROCESSHEAPS)(DWORD, PHANDLE);

/* IDirect3DDevice8 method-types */
typedef HRESULT (STDMETHODCALLTYPE *RESET)(IDirect3DDevice8* self, LPVOID);
typedef HRESULT (STDMETHODCALLTYPE *PRESENT)(IDirect3DDevice8* self, CONST RECT*, CONST RECT*, HWND, LPVOID);
typedef ULONG   (STDMETHODCALLTYPE *RELEASE)(IUnknown* self);

typedef LPVOID (STDMETHODCALLTYPE *FUNC)();
typedef LPVOID (STDMETHODCALLTYPE *FUNC2)(DWORD);
typedef LPVOID (*LOADUNI)(DWORD, DWORD);
typedef LPVOID (*LOADCOLLARS)(DWORD);
typedef LPVOID (*QUICKSELECT)();
typedef DWORD  (*BINUNPACK)(DWORD, DWORD, DWORD, DWORD, DWORD);
typedef DWORD  (*CHECKTEAM)(DWORD);

LPVOID JuceLoadUni(DWORD, DWORD);
LPVOID JuceRestoreAll(DWORD);
LPVOID JuceRestoreAll2();
DWORD  JuceBinUnpack(DWORD, DWORD, DWORD, DWORD, DWORD);
LPVOID JuceBallLoader(DWORD);
DWORD  JuceCheckTeam(DWORD);

/* function pointers */
RELEASE g_d3d8_release = NULL;
RESET   g_d3d8_reset   = NULL;
PRESENT g_d3d8_present = NULL;

FUNC Func = NULL;
FUNC2 Func2 = NULL;
LOADUNI LoadUni = NULL;
LOADCOLLARS LoadCollars = NULL;
QUICKSELECT QuickSelect = NULL;
BINUNPACK BinUnpack = NULL;
CHECKTEAM CheckTeam = NULL;
FUNC2 BallLoader = NULL;

// Executable file footprints
BYTE sigArrayLoadUni[4][13] = {
	{ 0x56,0x8b,0x74,0x24,0x08,0x57,0x8b,0xd6,0xe8,0xa3,0xff,0xff,0xff }, // pes 1.0
	{ 0x56,0x8b,0x74,0x24,0x08,0x57,0x8b,0xd6,0xe8,0xa3,0xff,0xff,0xff }, // pes 1.10.2
	{ 0x56,0x8b,0x74,0x24,0x08,0x57,0x8b,0xd6,0xe8,0xa3,0xff,0xff,0xff }, // pes 1.30.1
	{ 0x56,0x8b,0x74,0x24,0x08,0x57,0x8b,0xd6,0xe8,0xa3,0xff,0xff,0xff }, // we7:i
};

BYTE sigArrayLoadCollars[4][12] = {
	{ 0x83,0xec,0x08,0x8b,0x0d,0x98,0xff,0x91,0,0x53,0x55,0x56 },  // pes 1.0
	{ 0x83,0xec,0x08,0x8b,0x0d,0xf8,0xff,0x91,0,0x53,0x55,0x56 },  // pes 1.10.2
	{ 0x83,0xec,0x08,0x8b,0x0d,0x98,0xff,0x91,0,0x53,0x55,0x56 },  // pes 1.30.1
	{ 0x83,0xec,0x08,0x8b,0x0d,0xa8,0xcf,0x91,0,0x53,0x55,0x56 },  // we7:i
};

BYTE sigArrayQuickSelect[4][16] = {
	{ 0x56,0x57,0xbe,0x20,0x13,0xd5,0,0xbf,0x16,0,0,0,0x8d,0x64,0x24,0 }, // pes 1.0
	{ 0x56,0x57,0xbe,0xe0,0x3c,0xd7,0,0xbf,0x16,0,0,0,0x8d,0x64,0x24,0 }, // pes 1.10.2
	{ 0x56,0x57,0xbe,0x60,0x65,0xd9,0,0xbf,0x16,0,0,0,0x8d,0x64,0x24,0 }, // pes 1.30.1
	{ 0x56,0x57,0xbe,0xf0,0x26,0xd9,0,0xbf,0x16,0,0,0,0x8d,0x64,0x24,0 }, // we7:i
};

BYTE sigLoadUni[13];
BYTE sigLoadCollars[12];
BYTE sigQuickSelect[16];

bool bLoadUniHooked = false;
bool bLoadCollarsHooked = false;
bool bQuickSelectHooked = false;
bool bBinUnpackHooked = false;
bool bCheckTeamHooked = false;
bool bBallLoaderHooked = false;

int g_binUnpackCount = 10;
int g_ballIndex = 0;

DWORD offsetRelease = 0;
DWORD offsetReset = 0;
DWORD offsetPresent = 0;
BOOL okToHook = false;
 
HINSTANCE hInst, hProc;
HMODULE hD3D8 = NULL, utilDLL = NULL;
HWND hProcWnd = NULL;
HANDLE procHeap = NULL;

char myfile[MAX_FILEPATH];
char mydir[MAX_FILEPATH];
char processfile[MAX_FILEPATH];
char *shortProcessfile;
char shortProcessfileNoExt[BUFLEN];
char buf[BUFLEN];

char logName[BUFLEN];

// keyboard hook handle
HHOOK g_hKeyboardHook = NULL;
BOOL bIndicate = false;

UINT g_bbWidth = 0;
UINT g_bbHeight = 0;
UINT g_labelsYShift = 0;

// buffer to keep current frame 
BYTE* g_rgbBuf = NULL;
INT g_rgbSize = 0;
INT g_Pitch = 0;

int GetGameVersion(void);
void Initialize(int v);
void ResetCyclesAndBuffers2(void);
void LoadKDB();
HRESULT SaveAs8bitBMP(char*, BYTE*, BYTE*, LONG, LONG);
HRESULT SaveAsBMP(char*, BYTE*, SIZE_T, LONG, LONG, int);


/******************************************************************/
/******************************************************************/

// loads the ball model and texture into 2 global buffers
void LoadBall(char* mdl, char* tex)
{
	FILE* f;
	DWORD size;

	// clear out the pointers
	g_ballMdl = g_ballTex = NULL; 

	// load model file (optional)
	if (mdl != NULL && mdl[0] != '\0')
	{
		f = fopen(mdl, "rb");
		if (f == NULL)
		{
			LogWithString("LoadBall: unable to open ball model file: %s.", mdl);
			return;
		}
		fseek(f, 4, SEEK_SET);
		fread(&size, 4, 1, f);
		fseek(f, 0, SEEK_SET);
		g_ballMdl = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 32);
		fread(g_ballMdl, size + 32, 1, f);
		fclose(f);
		LogWithNumber("LoadBall: g_ballMdl address = %08x", (DWORD)g_ballMdl);
	}

	// load texture file (required)
	f = fopen(tex, "rb");
	if (f == NULL)
	{
		LogWithString("LoadBall: unable to open ball texture file: %s.", tex);
		return;
	}
	fseek(f, 4, SEEK_SET);
	fread(&size, 4, 1, f);
	fseek(f, 0, SEEK_SET);
	g_ballTex = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 32);
	fread(g_ballTex, size + 32, 1, f);
	fclose(f);
	LogWithNumber("LoadBall: g_ballTex address = %08x", (DWORD)g_ballTex);
}

// frees the ball buffers
void FreeBall()
{
	if (g_ballMdl) HeapFree(GetProcessHeap(), 0, g_ballMdl);
	if (g_ballTex) HeapFree(GetProcessHeap(), 0, g_ballTex);
	Log("Ball memory freed.");
}

// Calls IUnknown::Release() on an instance
void SafeRelease(LPVOID pSelf)
{
	DWORD* selfPtr = (DWORD*)pSelf;
	if (selfPtr == NULL) return;
	DWORD* self = (DWORD*)(*selfPtr);
	if (self == NULL) return;

	IUnknown* ptr = (IUnknown*)self;
	if (ptr->Release != NULL) ptr->Release();
	else Log("Unable to call IUnknown::Release: it's NULL.");
	*selfPtr = NULL;
}

/* determine window handle */
void GetProcWnd(IDirect3DDevice8* d3dDevice)
{
	TRACE("GetProcWnd: called.");
	D3DDEVICE_CREATION_PARAMETERS params;	
	if (SUCCEEDED(d3dDevice->GetCreationParameters(&params)))
	{
		hProcWnd = params.hFocusWindow;
	}
}

/* remove keyboard hook */
void UninstallKeyboardHook(void)
{
	if (g_hKeyboardHook != NULL)
	{
		UnhookWindowsHookEx( g_hKeyboardHook );
		Log("Keyboard hook uninstalled.");
		g_hKeyboardHook = NULL;
	}
}

HRESULT InitVB(IDirect3DDevice8* dev)
{
	VOID* pVertices;

	// create vertex buffers
	// #0
	if (FAILED(dev->CreateVertexBuffer(9*sizeof(CUSTOMVERTEX), D3DUSAGE_WRITEONLY, 
					D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVB0)))
	{
		Log("CreateVertexBuffer() failed.");
		return E_FAIL;
	}
	Log("CreateVertexBuffer() done.");

	if (FAILED(g_pVB0->Lock(0, sizeof(g_Vert0), (BYTE**)&pVertices, 0)))
	{
		Log("g_pVB0->Lock() failed.");
		return E_FAIL;
	}
	memcpy(pVertices, g_Vert0, sizeof(g_Vert0));
	g_pVB0->Unlock();

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT RestoreDeviceObjects(IDirect3DDevice8* dev)
{
	g_dev = dev;

    HRESULT hr = InitVB(dev);
    if (FAILED(hr))
    {
		Log("InitVB() failed.");
        return hr;
    }
	Log("InitVB() done.");

	if (g_fontInitialized) 
	{
		g_font->InitDeviceObjects(dev);
		g_fontInitialized = true;
	}
	g_font->RestoreDeviceObjects();

	D3DVIEWPORT8 vp;
	vp.X      = POSX;
	vp.Y      = POSY;
	vp.Width  = INDX*2 + IWIDTH;
	vp.Height = INDY*2 + IHEIGHT;
	vp.MinZ   = 0.0f;
	vp.MaxZ   = 1.0f;

	TRACE2("vp.X = %d", (int)vp.X);
	TRACE2("vp.Y = %d", (int)vp.Y);
	TRACE2("vp.Width = %d", (int)vp.Width);
	TRACE2("vp.Height = %d", (int)vp.Height);

	// Create the state blocks for rendering text
	for( UINT which=0; which<2; which++ )
	{
		dev->BeginStateBlock();

		dev->SetViewport( &vp );
		dev->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
		dev->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		dev->SetRenderState( D3DRS_FILLMODE,   D3DFILL_SOLID );
		dev->SetRenderState( D3DRS_CULLMODE,   D3DCULL_CW );
		dev->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
		dev->SetRenderState( D3DRS_FOGENABLE,        FALSE );

		dev->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_DISABLE );
		dev->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

		dev->SetVertexShader( D3DFVF_CUSTOMVERTEX );
		dev->SetPixelShader ( NULL );
		dev->SetStreamSource( 0, g_pVB0, sizeof(CUSTOMVERTEX) );

		if( which==0 )
			dev->EndStateBlock( &g_dwSavedStateBlock );
		else
			dev->EndStateBlock( &g_dwDrawTextStateBlock );
	}

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Destroys all device-dependent objects
//-----------------------------------------------------------------------------
HRESULT InvalidateDeviceObjects(IDirect3DDevice8* dev, BOOL detaching)
{
	TRACE("InvalidateDeviceObjects called.");
	if (detaching) return S_OK;

	SafeRelease( &g_pVB0 );
	//SafeRelease( &g_pVB1 );
	//SafeRelease( &g_pVB2 );
	//SafeRelease( &g_pVB3 );
	Log("InvalidateDeviceObjects: SafeRelease(s) done.");

    // Delete the state blocks
    if( dev && !detaching )
    {
		if( g_dwSavedStateBlock )
			dev->DeleteStateBlock( g_dwSavedStateBlock );
		if( g_dwDrawTextStateBlock )
			dev->DeleteStateBlock( g_dwDrawTextStateBlock );
		Log("InvalidateDeviceObjects: DeleteStateBlock(s) done.");
    }

    g_dwSavedStateBlock    = 0L;
    g_dwDrawTextStateBlock = 0L;

	if (g_font) g_font->InvalidateDeviceObjects();

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Destroys all device-dependent objects
//-----------------------------------------------------------------------------
HRESULT DeleteDeviceObjects(IDirect3DDevice8* dev)
{
	if (g_font) g_font->DeleteDeviceObjects();
	g_fontInitialized = false;

    return S_OK;
}

void DrawIndicator(LPVOID self)
{
	IDirect3DDevice8* dev = (IDirect3DDevice8*)self;

	if (g_pVB0 == NULL) 
	{
		if (FAILED(RestoreDeviceObjects(dev)))
		{
			Log("RestoreDeviceObjects() failed.");
		}
		Log("RestoreDeviceObjects() done.");
	}

	// setup renderstate
	dev->CaptureStateBlock( g_dwSavedStateBlock );
	dev->ApplyStateBlock( g_dwDrawTextStateBlock );

	// render
	dev->BeginScene();

	dev->SetStreamSource( 0, g_pVB0, sizeof(CUSTOMVERTEX) );
	dev->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 3 );

	dev->EndScene();

	// restore the modified renderstates
	dev->ApplyStateBlock( g_dwSavedStateBlock );
}

void DrawBallLabel(IDirect3DDevice8* dev)
{
	if (g_pVB0 == NULL) 
	{
		if (FAILED(RestoreDeviceObjects(dev)))
		{
			Log("RestoreDeviceObjects() failed.");
		}
		Log("RestoreDeviceObjects() done.");
	}

	/* draw ball label */
	//Log("DrawBallLabel: About to draw text.");
	char buf[255];
	ZeroMemory(buf, 255);
	lstrcpy(buf, "ball: "); lstrcat(buf, g_ballName);
	SIZE size;
	DWORD color = 0xffa0a0a0; // light grey - for default
	if (g_ballTex != NULL) color = 0xff4488ff; // blue

	g_font->GetTextExtent(buf, &size);
	g_font->DrawText( g_bbWidth/2 - size.cx/2,  g_bbHeight/2 - size.cy/2 + (int)(IHEIGHT*1.2), 
			color, buf);
	//Log("DrawBallLabel: Text drawn.");
}

void DrawKitLabels(IDirect3DDevice8* dev)
{
	if (g_pVB0 == NULL) 
	{
		if (FAILED(RestoreDeviceObjects(dev)))
		{
			Log("RestoreDeviceObjects() failed.");
		}
		Log("RestoreDeviceObjects() done.");
	}

	/* draw ball label */
	//Log("DrawKitLabels: About to draw text.");
	char buf[255];
	SIZE size;
	DWORD color;
	int i;

	// HOME PLAYER
	ZeroMemory(buf, 255);
	lstrcpy(buf, "PL: ");
	i = (g_strip[0] == 0) ? HOST1 : HOST2;
	if ((kitCycle[i] == NULL) || (kitCycle[i]->kit->fileName[0] == 'x'))
	{
		lstrcat(buf, DEFAULT_LABEL[i > 0]);
		color = 0xffa0a0a0; // light grey - for default
	}
	else 
	{
		lstrcat(buf, kitCycle[i]->kit->fileName);
		color = 0xff4488ff; // blue;
	}
	g_font->GetTextExtent(buf, &size);
	g_font->DrawText( g_bbWidth/2 - 85 - size.cx,  g_bbHeight - g_labelsYShift - 15, color, buf);

	// AWAY PLAYER
	ZeroMemory(buf, 255);
	lstrcpy(buf, "PL: ");
	i = (g_strip[1] == 0) ? GUEST1 : GUEST2;
	if ((kitCycle[i] == NULL) || (kitCycle[i]->kit->fileName[0] == 'x'))
	{
		lstrcat(buf, DEFAULT_LABEL[i > 1]);
		color = 0xffa0a0a0; // light grey - for default
	}
	else 
	{
		lstrcat(buf, kitCycle[i]->kit->fileName);
		color = 0xff4488ff; // blue;
	}
	g_font->GetTextExtent(buf, &size);
	g_font->DrawText( g_bbWidth/2 + 85,  g_bbHeight - g_labelsYShift - 15, color, buf);

	// HOME GOALKEEPER
	ZeroMemory(buf, 255);
	lstrcpy(buf, "GK: ");
	if (gkUniBuf[0] == NULL) lstrcat(buf, DEFAULT_LABEL[0]);
	else lstrcat(buf, gkCycle[0]->kit->fileName);
	color = 0xffa0a0a0; // light grey - for default
	if (gkUniBuf[0] != NULL) color = 0xff4488ff; // blue

	g_font->GetTextExtent(buf, &size);
	g_font->DrawText( g_bbWidth/2 - 85 - size.cx,  g_bbHeight - g_labelsYShift, color, buf);

	// AWAY GOALKEEPER
	ZeroMemory(buf, 255);
	lstrcpy(buf, "GK: ");
	if (gkUniBuf[1] == NULL) lstrcat(buf, DEFAULT_LABEL[1]);
	else lstrcat(buf, gkCycle[1]->kit->fileName);
	color = 0xffa0a0a0; // light grey - for default
	if (gkUniBuf[1] != NULL) color = 0xff4488ff; // blue

	g_font->GetTextExtent(buf, &size);
	g_font->DrawText( g_bbWidth/2 + 85,  g_bbHeight - g_labelsYShift, color, buf);

	//Log("DrawKitLabels: Text drawn.");
}

/* New Reset function */
EXTERN_C HRESULT _declspec(dllexport) STDMETHODCALLTYPE JuceReset(
IDirect3DDevice8* self, LPVOID params)
{
	TRACE("JuceReset: called.");
	Log("JuceReset: cleaning-up.");

	// drop HWND 
	hProcWnd = NULL;

	// clean-up
	if (g_rgbBuf != NULL) 
	{
		HeapFree(procHeap, 0, g_rgbBuf);
		g_rgbBuf = NULL;
	}

	InvalidateDeviceObjects(self, false);
	DeleteDeviceObjects(self);

	g_bGotFormat = false;

	/* call real Present() */
	HRESULT res = (g_d3d8_reset)(self, params);

	TRACE("JuceReset: Reset() is done. About to return.");
	return res;
}

/**
 * Determine format of back buffer and its dimensions.
 */
void GetBackBufferInfo(IDirect3DDevice8* d3dDevice)
{
	TRACE("GetBackBufferInfo: called.");

	// get the 0th backbuffer.
	if (SUCCEEDED(d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &g_backBuffer)))
	{
		D3DSURFACE_DESC desc;
		g_backBuffer->GetDesc(&desc);
		g_bbFormat = desc.Format;
		g_bbWidth = desc.Width;
		g_bbHeight = desc.Height;

		// vertical offset from bottom for uniform labels
		g_labelsYShift = (UINT)(g_bbHeight * 0.185);
		LogWithNumber("g_labelsYShift = %d", g_labelsYShift);

		// adjust indicator coords
		float OLDPOSX = POSX;
		float OLDPOSY = POSY;
		POSX = g_bbWidth/2 - INDX - IWIDTH/2;
		POSY = g_bbHeight/2 - INDY - IHEIGHT/2 + 15.0f;
		for (int k=0; k<9; k++)
		{
			g_Vert0[k].x += POSX - OLDPOSX;
			g_Vert0[k].y += POSY - OLDPOSY;
			/*
			g_Vert1[k].x += POSX - OLDPOSX;
			g_Vert1[k].y += POSY - OLDPOSY;
			g_Vert2[k].x += POSX - OLDPOSX;
			g_Vert2[k].y += POSY - OLDPOSY;
			g_Vert3[k].x += POSX - OLDPOSX;
			g_Vert3[k].y += POSY - OLDPOSY;
			*/
		}

		// check dest.surface format
		bpp = 0;
		if (g_bbFormat == D3DFMT_R8G8B8) bpp = 3;
		else if (g_bbFormat == D3DFMT_R5G6B5 || g_bbFormat == D3DFMT_X1R5G5B5) bpp = 2;
		else if (g_bbFormat == D3DFMT_A8R8G8B8 || g_bbFormat == D3DFMT_X8R8G8B8) bpp = 4;

		// release backbuffer
		g_backBuffer->Release();

		Log("GetBackBufferInfo: got new back buffer format and info.");
		g_bGotFormat = true;
	}
}

/* New Present function */
EXTERN_C HRESULT _declspec(dllexport) STDMETHODCALLTYPE JucePresent(
IDirect3DDevice8* self, CONST RECT* src, CONST RECT* dest, HWND hWnd, LPVOID unused)
{
	//TRACE("--------------------------------");
	//TRACE("JucePresent: called.");
	/* log the usage */
	//Log("JucePresent: Present() is about to be called.");
	
	// determine backbuffer's format and dimensions, if not done yet.
	if (!g_bGotFormat) 
	{
		GetBackBufferInfo(self);
		LogWithNumber("JucePresent: IDirect3DDevice8* = %08x", (DWORD)self);
	}

	//TRACE("JucePresent: new frame.");
	
	// Analyze where we are in the game menu
	BYTE prevMainOrExhib = g_MainOrExhib;
	BYTE prevScreenId = g_ScreenId;
	BYTE prev_QS_ScreenId = g_QS_ScreenId;
	g_MainOrExhib = *((BYTE*)data[MENU_FLAG]);
	g_ScreenId = *((BYTE*)data[EXHIB_MENU_ID]);
	g_QS_ScreenId = *((BYTE*)data[QUICKSTART_MENU_ID]);
	if (g_MainOrExhib)
	{
		// CHECK CONDITIONS TO RESTORE COLORS/COLLARS AND RESET UNI-COUNT
		// FOR GOALKEEPER KIT LOADING
		// condition 1: g_MainOrExhib changed from 0 to 1
		if ((prevMainOrExhib == 0) && (g_MainOrExhib == 1))
		{
			// went back to mode select menu
			TRACE("MainOrExhib: 0 -> 1");
			ResetCyclesAndBuffers2();
		}

		// condition 2: g_ScreenId changed from 4 to 1
		if ((prevScreenId == 4) && (g_ScreenId == 1))
		{
			// went back to mode select menu
			TRACE("g_ScreenId: 4 -> 1");
			ResetCyclesAndBuffers2();
		}
		// condition 3: g_ScreenId changed from <4 to 4
		if ((prevScreenId < 4) && (g_ScreenId == 4))
		{
			TRACE2("g_ScreenId: %d -> 4", prevScreenId);
			ResetCyclesAndBuffers2();
		}

		// condition 4: g_QS_ScreenId changed from 4 to 1
		if ((prev_QS_ScreenId == 4) && (g_QS_ScreenId == 1))
		{
			// went back to mode select menu
			TRACE("g_QS_ScreenId: 4 -> 1");
			ResetCyclesAndBuffers2();
		}
		// condition 5: g_QS_ScreenId changed from <4 to 4
		if ((prev_QS_ScreenId < 4) && (g_QS_ScreenId == 4))
		{
			TRACE2("g_QS_ScreenId: %d -> 4", prev_QS_ScreenId);
			ResetCyclesAndBuffers2();
		}
	}

	if (bToggleUni)
	{
		FILE* unif;
		BYTE* ptr = (BYTE*)data[TEAM_NUM];
		int i, j;
		NNStrip* s1Colors; NNStrip* s2Colors;
		NNStrip* saveS1; NNStrip* saveS2;
		BYTE* collars; BYTE* saveCollars;
		BYTE newNum;
		Kit* kit = NULL;
		BYTE* tempp;

		TRACE2("JucePresent: g_uniToggleStep = %d", g_uniToggleStep);
		switch (g_uniToggleStep) {
			case START:
				i = g_uniToChange; j = (i + 2) % 4;
				TRACE2("JucePresent: i = %d", i);
				TRACE2("JucePresent: j = %d", j);

				if (kitCycle[i]->kit->fileName[0] == 'x') kit = prevKit[i];
				else kit = kitCycle[i]->kit;

				// Read the uniform file
				ZeroMemory(kitFilename[i], 1024);
				lstrcpy(kitFilename[i], g_config.kdbDir);
				lstrcat(kitFilename[i], "KDB\\players\\"); 
				lstrcat(kitFilename[i], kit->fileName);
				TRACE(kitFilename[i]);
				
				// release existing buffer, as it is no good anymore
				SAFEFREE(unibuf1[i]);
				SAFEFREE(unibuf2[i]);

				// load uniform into current strip
				unif = fopen(kitFilename[i], "rb");
				fseek(unif, 4, SEEK_SET);
				DWORD size;
				fread(&size, 4, 1, unif);
				fseek(unif, 0, SEEK_SET);
				unibuf1[i] = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 32);
				fread(unibuf1[i], size + 32, 1, unif);
				fclose(unif);
				TRACE2("uni-buffer1 address = %08x", (DWORD)unibuf1[i]);

				unibuf2[i] = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 32);
				memcpy(unibuf2[i], unibuf1[i], size + 32);
				TRACE2("uni-buffer2 address = %08x", (DWORD)unibuf2[i]);

				prevKit[i] = kit;

				// if alternate strip is changed too, re-load it as well.
				if (unibuf1[j])
				{
					// release existing buffer, as it is no good anymore
					SAFEFREE(unibuf1[j]);
					SAFEFREE(unibuf2[j]);

					// load 2nd strip (if changed)
					unif = fopen(kitFilename[j], "rb");
					fseek(unif, 4, SEEK_SET);
					DWORD size;
					fread(&size, 4, 1, unif);
					fseek(unif, 0, SEEK_SET);
					unibuf1[j] = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 32);
					fread(unibuf1[j], size + 32, 1, unif);
					fclose(unif);
					TRACE2("uni-buffer1 address = %08x", (DWORD)unibuf1[j]);

					unibuf2[j] = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 32);
					memcpy(unibuf2[j], unibuf1[j], size + 32);
					TRACE2("uni-buffer2 address = %08x", (DWORD)unibuf2[j]);
				}

				g_uniToggleStep = SWITCHTEAM;
				break;
			case SWITCHTEAM:
				g_uniToggleStep = FAKE1;
				// change the team
				g_teamNum = CheckTeam(ptr[g_uniToChange % 2]);
				newNum = CheckTeam(ptr[(g_uniToChange + 1) % 2]); // switch to the opponent team
				if (newNum == g_teamNum)
				{
					// SPECIAL CASE: a team playing against itself
					if (g_teamNum > 64) newNum = (g_teamNum == 65) ? 66 : 65;
					else newNum = (g_teamNum == 0) ? 1 : 0;
				}
				TRACE2("switching to team %d", newNum);
				ptr[g_uniToChange % 2] = newNum;
				Func = (FUNC)code[C_RELOADTEAMS];
				Func();
				TRACE("switched.");
				break;
			case SWITCHTEAMBACK:
				g_uniToggleStep = REAL1;
				s1Colors     = (g_uniToChange % 2 == 0) ? &homeNN->p1     : &awayNN->p1;
				s2Colors     = (g_uniToChange % 2 == 0) ? &homeNN->p2     : &awayNN->p2;
				saveS1       = (g_uniToChange % 2 == 0) ? &saveHome.p1    : &saveAway.p1;
				saveS2       = (g_uniToChange % 2 == 0) ? &saveHome.p2    : &saveAway.p2;
				collars      = (g_uniToChange % 2 == 0) ? homeCollars     : awayCollars;
				saveCollars  = (g_uniToChange % 2 == 0) ? saveHomeCollars : saveAwayCollars;
				// if not done yet, save original colors and collar
				if (!extraInfoSaved)
				{
					memcpy(&saveHome.p1, &homeNN->p1, sizeof(NNStrip));
					memcpy(&saveHome.p2, &homeNN->p2, sizeof(NNStrip));
					memcpy(&saveAway.p1, &awayNN->p1, sizeof(NNStrip));
					memcpy(&saveAway.p2, &awayNN->p2, sizeof(NNStrip));
					memcpy(saveHomeCollars, homeCollars, 2);
					memcpy(saveAwayCollars, awayCollars, 2);
					extraInfoSaved = true;
				}
				// Set reference to current kit
				kit = kitCycle[g_uniToChange]->kit;
				// update colors and collar to default values
				if (g_uniToChange < 2)
				{
					// 1st strip
					memcpy(s1Colors, saveS1, sizeof(NNStrip));
					collars[0] = saveCollars[0];
				}
				else
				{
					// 2nd strip
					memcpy(s2Colors, saveS2, sizeof(NNStrip));
					collars[1] = saveCollars[1];
				}
				// update colors and collar to custom specified values (for those specified)
				if (g_uniToChange < 2)
				{
					// 1st strip
					if (kit->attDefined & SHIRT_BACK_NUMBER) memcpy(s1Colors->backNumber, kit->shirtBackNumber, 6);
					if (kit->attDefined & SHIRT_FRONT_NUMBER) memcpy(s1Colors->frontNumber, kit->shirtFrontNumber, 6);
					if (kit->attDefined & SHORTS_NUMBER) memcpy(s1Colors->shortsNumber, kit->shortsNumber, 6);
					if (kit->attDefined & SHIRT_BACK_NAME) memcpy(s1Colors->backName, kit->shirtBackName, 6);

					if (kit->attDefined & COLLAR) collars[0] = kit->collar;
				}
				else
				{
					// 2nd strip
					if (kit->attDefined & SHIRT_BACK_NUMBER) memcpy(s2Colors->backNumber, kit->shirtBackNumber, 6);
					if (kit->attDefined & SHIRT_FRONT_NUMBER) memcpy(s2Colors->frontNumber, kit->shirtFrontNumber, 6);
					if (kit->attDefined & SHORTS_NUMBER) memcpy(s2Colors->shortsNumber, kit->shortsNumber, 6);
					if (kit->attDefined & SHIRT_BACK_NAME) memcpy(s2Colors->backName, kit->shirtBackName, 6);

					if (kit->attDefined & COLLAR) collars[1] = kit->collar;
				}
				// invalidate the buffers, if dummy kit
				if (kit->fileName[0] == 'x')
				{
					i = g_uniToChange; j = (i + 2) % 4;
					TRACE2("JucePresent INV: i = %d", i);

					SAFEFREE(unibuf1[i]);
					SAFEFREE(unibuf2[i]);
				}

				// change the team
				ptr[g_uniToChange % 2] = g_teamNum;
				// force uniform reload
				Func = (FUNC)code[C_RELOADTEAMS];
				Func();
				// force number colors re-read
				Func = (FUNC)code[C_REREADNUMS];
				Func();
				// force collar re-read
				Func2 = (FUNC2)code[C_REREADCOLLARS];
				Func2(data[PARAM_HOME]);
				Func2 = (FUNC2)code[C_REREADCOLLARS];
				Func2(data[PARAM_AWAY]);
				break;
			case STOP:
				bToggleUni = false;
				g_uniToggleStep = IDLE;
				// move on to next kit
				//kitCycle[g_uniToChange] = kitCycle[g_uniToChange]->next;
				break;
			default:
				TRACE("JucePresent: NO uni-toggle action");
		}
	}

	if (!g_fontInitialized)
	{
		g_font->InitDeviceObjects(self);
		g_fontInitialized = true;
	}

	/* draw the star indicator */
	bIndicate = IS_STRIP_SELECTION && (g_kitsAvailable || g_ballsAvailable);
	if (bIndicate) DrawIndicator(self);

	/* draw the ball label */
	if (IS_STRIP_SELECTION && g_ballsAvailable) DrawBallLabel(self);

	/* draw the uniform labels */
	if (IS_STRIP_SELECTION && g_kitsAvailable) DrawKitLabels(self);

	/* call real Present() */
	HRESULT res = (g_d3d8_present)(self, src, dest, hWnd, unused);

	// install keyboard hook, if not done yet.
	if (g_hKeyboardHook == NULL)
	{
		g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hInst, GetCurrentThreadId());
		LogWithNumber("Installed keyboard hook: g_hKeyboardHook = %d", (DWORD)g_hKeyboardHook);
	}

	//Log("JucePresent: Present() is done. About to return.");
	return res;
}

/* This function hooks IDirect3DDevice8::Present() method, if not done already. */
void CheckAndHookPresent()
{
	if (!okToHook) return;
	okToHook = false;

	// STEP 2: check to see if already hooked
	if (presentPtr != NULL && (*presentPtr) == (DWORD)JucePresent)
	{
		// nothing to do: already hooked.
		return;
	}

	// STEP 3: check the "magic" location
	if ((DWORD*)data[LPDEV] != NULL)
	{
		DWORD* dev = (DWORD*)(*(DWORD*)data[LPDEV]);
		LogWithNumber("device = %08x", (DWORD)dev);
		if (dev)
		{
			DWORD* vtable = (DWORD*)(*dev);
			resetPtr   = vtable + 14;
			LogWithNumber("resetPtr = %08x", (DWORD)resetPtr);
			presentPtr = vtable + 15;
			LogWithNumber("presentPtr = %08x", (DWORD)presentPtr);

			// save original address of Reset in "g_d3d8_reset" variable
			offsetReset = *resetPtr;
			g_d3d8_reset = (RESET)((DWORD*)offsetReset);
			LogWithNumber("CheckAndHookPresent: Reset() address saved: %08x.", (DWORD)g_d3d8_reset);

			// save original address of Present in "g_d3d8_present" variable
			offsetPresent = *presentPtr;
			g_d3d8_present = (PRESENT)((DWORD*)offsetPresent);
			LogWithNumber("CheckAndHookPresent: Present() address saved: %08x.", (DWORD)g_d3d8_present);
		}
	}

	// STEP 4: hook
	if (resetPtr != NULL)
	{
		(*resetPtr) = (DWORD)JuceReset;
		Log("CheckAndHookPresent: Reset() hooked.");
	}
	else { TRACE("CheckAndHookPresent: Present() NOT hooked."); }
	if (presentPtr != NULL)
	{
		(*presentPtr) = (DWORD)JucePresent;
		Log("CheckAndHookPresent: Present() hooked.");
	}
	else { TRACE("CheckAndHookPresent: Present() NOT hooked."); }

	okToHook = true;
}

/* Restore original Reset() and Present() */
EXTERN_C _declspec(dllexport) void RestoreDeviceMethods()
{
	if (presentPtr != NULL) (*presentPtr) = (DWORD)g_d3d8_present;
	if (resetPtr != NULL) (*resetPtr) = (DWORD)g_d3d8_reset;
	presentPtr = resetPtr = NULL;
	Log("RestoreDeviceMethods: done.");
}

/*******************/
/* DLL Entry Point */
/*******************/
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	DWORD wbytes, procId; 

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		hInst = hInstance;

		/* determine process full path */
		ZeroMemory(processfile, MAX_FILEPATH);
		GetModuleFileName(NULL, processfile, MAX_FILEPATH);
		char* zero = processfile + lstrlen(processfile);
		char* p = zero; while ((p != processfile) && (*p != '\\')) p--;
		if (*p == '\\') p++;
		shortProcessfile = p;
		
		// save short filename without ".exe" extension.
		ZeroMemory(shortProcessfileNoExt, BUFLEN);
		char* ext = shortProcessfile + lstrlen(shortProcessfile) - 4;
		if (lstrcmpi(ext, ".exe")==0) {
			memcpy(shortProcessfileNoExt, shortProcessfile, ext - shortProcessfile); 
		}
		else {
			lstrcpy(shortProcessfileNoExt, shortProcessfile);
		}

		/* determine process handle */
		hProc = GetModuleHandle(NULL);

		/* determine my directory */
		ZeroMemory(mydir, BUFLEN);
		ZeroMemory(myfile, BUFLEN);
		GetModuleFileName(hInst, myfile, MAX_FILEPATH);
		lstrcpy(mydir, myfile);
		char *q = mydir + lstrlen(mydir);
		while ((q != mydir) && (*q != '\\')) { *q = '\0'; q--; }

		// read configuration
		char cfgFile[BUFLEN];
		ZeroMemory(cfgFile, BUFLEN);
		lstrcpy(cfgFile, mydir); 
		lstrcat(cfgFile, CONFIG_FILE);

		ReadConfig(&g_config, cfgFile);

		// adjust kdbDir, if it is specified as relative path
		if (g_config.kdbDir[0] == '.')
		{
			// it's a relative path. Therefore do it relative to mydir
			char temp[BUFLEN];
			ZeroMemory(temp, BUFLEN);
			lstrcpy(temp, mydir); 
			lstrcat(temp, g_config.kdbDir);
			ZeroMemory(g_config.kdbDir, BUFLEN);
			lstrcpy(g_config.kdbDir, temp);
		}

		/* open log file, specific for this process */
		ZeroMemory(logName, BUFLEN);
		lstrcpy(logName, mydir);
		lstrcat(logName, shortProcessfileNoExt); 
		lstrcat(logName, ".log");

		OpenLog(logName);

		Log("Log started.");
		LogWithNumber("g_config.debug = %d", g_config.debug);
		TRACE2("JuceLoadUni address = %08x", (DWORD)JuceLoadUni);
		TRACE2("JuceRestoreAll address = %08x", (DWORD)JuceRestoreAll);

		// check if code section is writeable
		IMAGE_SECTION_HEADER* sechdr = GetCodeSectionHeader();
		TRACE2("sechdr = %08x", (DWORD)sechdr);
		if (sechdr && (sechdr->Characteristics & 0x80000000))
		{
			// Determine the game version
			int v = GetGameVersion();
			LogWithString("Game version: %s", GAME[v]);

			if (v != -1)
			{
				Initialize(v);

				// hook code[C_LOADUNI];
				if (memcmp((BYTE*)code[C_LOADUNI], sigLoadUni, 13)==0)
				{
					DWORD* ptr = (DWORD*)code[C_LOADUNI_CS];
					ptr[0] = (DWORD)JuceLoadUni;
					bLoadUniHooked = true;
					Log("code[C_LOADUNI] HOOKED at code[C_LOADUNI_CS]");
				}
				else Log("LoadUni footprint not matched.");

				// hook code[C_LOADCOLLARS]
				if (memcmp((BYTE*)code[C_LOADCOLLARS], sigLoadCollars, 12)==0)
				{
					DWORD* ptr = (DWORD*)(code[C_LOADCOLLARS_CS] + 1);
					ptr[0] = (DWORD)JuceRestoreAll - (DWORD)(code[C_LOADCOLLARS_CS] + 5);
					bLoadCollarsHooked = true;
					Log("code[C_LOADCOLLARS] HOOKED at code[C_LOADCOLLARS_CS]");
				}
				else Log("LoadCollars footprint not matched.");

				// hook code[C_QUICKSELECT]
				if (memcmp((BYTE*)code[C_QUICKSELECT], sigQuickSelect, 16)==0)
				{
					DWORD* ptr = (DWORD*)(code[C_QUICKSELECT_CS] + 1);
					ptr[0] = (DWORD)JuceRestoreAll2 - (DWORD)(code[C_QUICKSELECT_CS] + 5);
					bQuickSelectHooked = true;
					Log("code[C_QUICKSELECT] HOOKED at code[C_QUICKSELECT_CS]");
				}
				else Log("QuickSelect footprint not matched.");

				// hook code[C_DECODEBIN]
				{
					DWORD* ptr = (DWORD*)(code[C_DECODEBIN_CS] + 1);
					ptr[0] = (DWORD)JuceBinUnpack - (DWORD)(code[C_DECODEBIN_CS] + 5);
					bBinUnpackHooked = true;
					Log("code[C_DECODEBIN] HOOKED at code[C_DECODEBIN_CS]");
				}

				// hook code[C_DECODEBIN] at different call site
				{
					DWORD* ptr = (DWORD*)(code[C_DECODEBIN_CS2] + 1);
					ptr[0] = (DWORD)JuceBinUnpack - (DWORD)(code[C_DECODEBIN_CS2] + 5);
					bBinUnpackHooked = true;
					Log("code[C_DECODEBIN] HOOKED at code[C_DECODEBIN_CS2]");
				}

				// hook code[C_LOADBALL]
				{
					DWORD* ptr = (DWORD*)(code[C_LOADBALL_CS] + 1);
					ptr[0] = (DWORD)JuceBallLoader - (DWORD)(code[C_LOADBALL_CS] + 5);
					bBallLoaderHooked = true;
					Log("code[C_LOADBALL] HOOKED at code[C_LOADBALL_CS]");
				}

				// hook code[C_CHECKTEAM]
				{
					DWORD* ptr = (DWORD*)(code[C_CHECKTEAM_CS] + 1);
					ptr[0] = (DWORD)JuceCheckTeam - (DWORD)(code[C_CHECKTEAM_CS] + 5);
					bCheckTeamHooked = true;
					Log("code[C_CHECKTEAM] HOOKED at code[C_CHECKTEAM_CS]");
				}

				// Load KDB database
				LoadKDB();

				// create font instance
				g_font = new CD3DFont( _T("Arial"), 10, D3DFONT_BOLD);
				ZeroMemory(g_ballName, 255);
				lstrcpy(g_ballName, "Default");
			}
			else
			{
				Log("Game not recognized. Nothing is hooked");
			}
		}
		else Log("Code section is not writeable.");
	}

	else if (dwReason == DLL_PROCESS_DETACH)
	{
		Log("DLL detaching...");

		// TEMP: free ball
		FreeBall();

		// unhook code[C_CHECKTEAM]
		if (bCheckTeamHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_CHECKTEAM_CS] + 1);
			ptr[0] = (DWORD)code[C_CHECKTEAM] - (DWORD)(code[C_CHECKTEAM_CS] + 5);
			Log("code[C_CHECKTEAM] UNHOOKED");
		}

		// unhook code[C_LOADBALL]
		if (bBallLoaderHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_LOADBALL_CS] + 1);
			ptr[0] = (DWORD)code[C_LOADBALL] - (DWORD)(code[C_LOADBALL_CS] + 5);
			Log("code[C_LOADBALL] UNHOOKED");
		}

		// unhook code[C_DECODEBIN]
		if (bBinUnpackHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_DECODEBIN_CS] + 1);
			ptr[0] = (DWORD)code[C_DECODEBIN] - (DWORD)(code[C_DECODEBIN_CS] + 5);
			Log("code[C_DECODEBIN] UNHOOKED");
		}

		// unhook code[C_DECODEBIN] at second call site
		if (bBinUnpackHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_DECODEBIN_CS2] + 1);
			ptr[0] = (DWORD)code[C_DECODEBIN] - (DWORD)(code[C_DECODEBIN_CS2] + 5);
			Log("code[C_DECODEBIN] UNHOOKED at 2nd call site");
		}

		// unhook code[C_QUICKSELECT]
		if (bQuickSelectHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_QUICKSELECT_CS] + 1);
			ptr[0] = (DWORD)code[C_QUICKSELECT] - (DWORD)(code[C_QUICKSELECT_CS] + 5);
			Log("code[C_QUICKSELECT] UNHOOKED");
			bQuickSelectHooked = false;
		}

		// unhook code[C_LOADCOLLARS]
		if (bLoadCollarsHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_LOADCOLLARS_CS] + 1);
			ptr[0] = (DWORD)code[C_LOADCOLLARS] - (DWORD)(code[C_LOADCOLLARS_CS] + 5);
			Log("code[C_LOADCOLLARS] UNHOOKED");
			bLoadCollarsHooked = false;
		}

		// unhook code[C_LOADUNI]
		if (bLoadUniHooked)
		{
			DWORD* ptr = (DWORD*)code[C_LOADUNI_CS];
			ptr[0] = code[C_LOADUNI];
			Log("code[C_LOADUNI] UNHOOKED");
			bLoadUniHooked = false;
		}

		// free the buffers
		for (int i=0; i<4; i++)
		{
			SAFEFREE(unibuf1[i]);
			SAFEFREE(unibuf2[i]);
		}

		/* uninstall keyboard hook */
		UninstallKeyboardHook();

		/* restore original pointers */
		RestoreDeviceMethods();

		InvalidateDeviceObjects(g_dev, true);
		Log("InvalideDeviceObjects done.");
		DeleteDeviceObjects(g_dev);
		Log("DeleteDeviceObjects done.");

		/* release interfaces */
		if (g_rgbBuf != NULL)
		{
			HeapFree(procHeap, 0, g_rgbBuf);
			g_rgbBuf = NULL;
			Log("g_rgbBuf freed.");
		}

		// unload KDB
		TRACE("Unloading KDB...");
		kdbUnload(kDB);
		Log("KDB unloaded.");

		SAFE_DELETE( g_font );
		TRACE("g_font SAFE_DELETEd.");

		/* close specific log file */
		Log("Closing log.");
		CloseLog();
	}

	return TRUE;    /* ok */
}

/**
 * Sets name/number colors for HOME GK
 */
void SetGKHomeNNColors(Kit* kit, BOOL setCustom)
{
	// determine which strip is used: 1st or 2nd
	NNStrip* homeNNStrip = (g_strip[0] & 0x02) ?  &homeNN->g2 : &homeNN->g1;
	NNStrip* savedNNStrip = (g_strip[0] & 0x02) ?  &saveHome.g2 : &saveHome.g1;

	// update colors/collar to defaults
	memcpy(homeNNStrip, savedNNStrip, sizeof(NNStrip));
	homeCollars[2] = saveHomeCollars[2];

	if (setCustom)
	{
		TRACE2("g_strip[0]: %02x", g_strip[0]); 
		if (g_strip[0] & 0x02)
			TRACE("HOME KEEPER using 2nd strip");
		else
			TRACE("HOME KEEPER using 1st strip");

		// update colors/collar to custom values (for those specified)
		if (kit->attDefined & SHIRT_BACK_NUMBER) memcpy(homeNNStrip->backNumber, kit->shirtBackNumber, 6);
		if (kit->attDefined & SHIRT_FRONT_NUMBER) memcpy(homeNNStrip->frontNumber, kit->shirtFrontNumber, 6);
		if (kit->attDefined & SHORTS_NUMBER) memcpy(homeNNStrip->shortsNumber, kit->shortsNumber, 6);
		if (kit->attDefined & SHIRT_BACK_NAME) memcpy(homeNNStrip->backName, kit->shirtBackName, 6);
		if (kit->attDefined & COLLAR) homeCollars[2] = kit->collar;
	}
}

/**
 * Sets name/number colors for AWAY GK
 */
void SetGKAwayNNColors(Kit* kit, BOOL setCustom)
{
	// determine which strip is used: 1st or 2nd
	NNStrip* awayNNStrip = (g_strip[1] & 0x02) ?  &awayNN->g2 : &awayNN->g1;
	NNStrip* savedNNStrip = (g_strip[1] & 0x02) ?  &saveAway.g2 : &saveAway.g1;

	// update colors/collar to defaults
	memcpy(awayNNStrip, savedNNStrip, sizeof(NNStrip));
	awayCollars[3] = saveAwayCollars[3];

	if (setCustom)
	{
		TRACE2("g_strip[1]: %02x", g_strip[1]); 
		if (g_strip[1] & 0x02)
			TRACE("AWAY KEEPER using 2nd strip");
		else
			TRACE("AWAY KEEPER using 1st strip");

		// update colors/collar to custom values (for those specified)
		if (kit->attDefined & SHIRT_BACK_NUMBER) memcpy(awayNNStrip->backNumber, kit->shirtBackNumber, 6);
		if (kit->attDefined & SHIRT_FRONT_NUMBER) memcpy(awayNNStrip->frontNumber, kit->shirtFrontNumber, 6);
		if (kit->attDefined & SHORTS_NUMBER) memcpy(awayNNStrip->shortsNumber, kit->shortsNumber, 6);
		if (kit->attDefined & SHIRT_BACK_NAME) memcpy(awayNNStrip->backName, kit->shirtBackName, 6);
		if (kit->attDefined & COLLAR) awayCollars[3] = kit->collar;
	}
}

/**************************************************************** 
 * WH_KEYBOARD hook procedure                                   *
 ****************************************************************/ 

EXTERN_C _declspec(dllexport) LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	//TRACE2("KeyboardProc CALLED. code = %d", code);

    if (code < 0) // do not process message 
        return CallNextHookEx(g_hKeyboardHook, code, wParam, lParam); 

	switch (code)
	{
		case HC_ACTION:
			/* process the key events */
			if (lParam & 0x80000000)
			{
				if (wParam == g_config.vKeyHomeKit && IS_STRIP_SELECTION && g_uniToggleStep == IDLE)
				{
					// check that it's not ML team vs. Club/National
					if (TEAM_CODE(0) != 0xb3 && kitCycle[0] != NULL)
					{
						TRACE2("TEAM_CODE(0) = %d", TEAM_CODE(0));

						bToggleUni = true;
						g_uniToggleStep = START;
						g_uniToChange = (g_strip[0] == 0) ? HOST1 : HOST2;

						homeNN = (NNColors*)(data[COLORS] + TEAM_CODE(0) * 104);
						homeCollars = (BYTE*)(data[COLLARS] + TEAM_CODE(0) * 20);
						awayNN = (NNColors*)(data[COLORS] + TEAM_CODE(1) * 104);
						awayCollars = (BYTE*)(data[COLLARS] + TEAM_CODE(1) * 20);

						// move on to next kit
						kitCycle[g_uniToChange] = kitCycle[g_uniToChange]->next;
					}
				}
				else if (wParam == g_config.vKeyAwayKit && IS_STRIP_SELECTION && g_uniToggleStep == IDLE)
				{
					// check that it's not ML team vs. Club/National
					if (TEAM_CODE(1) != 0xb4 && kitCycle[1] != NULL)
					{
						TRACE2("TEAM_CODE(1) = %d", TEAM_CODE(1));

						bToggleUni = true;
						g_uniToggleStep = START;
						g_uniToChange = (g_strip[1] == 0) ? GUEST1 : GUEST2;

						homeNN = (NNColors*)(data[COLORS] + TEAM_CODE(0) * 104);
						homeCollars = (BYTE*)(data[COLLARS] + TEAM_CODE(0) * 20);
						awayNN = (NNColors*)(data[COLORS] + TEAM_CODE(1) * 104);
						awayCollars = (BYTE*)(data[COLLARS] + TEAM_CODE(1) * 20);

						// move on to next kit
						kitCycle[g_uniToChange] = kitCycle[g_uniToChange]->next;
					}
				}
				else if (wParam == g_config.vKeyGKHomeKit && IS_STRIP_SELECTION)
				{
					if (gkCycle[0] != NULL)
					{
						// move on to next uniform
						gkCycle[0] = gkCycle[0]->next;

						// switch home goalkeeper uniform
						HANDLE procHeap = GetProcessHeap();
						// free previous kit, if any was loaded
						if (gkUniBuf[0] != NULL) { HeapFree(procHeap, 0, gkUniBuf[0]); }
						if (gkUniBuf2[0] != NULL) { HeapFree(procHeap, 0, gkUniBuf2[0]); }
						// determine next kit to load
						char path[BUFLEN];
						ZeroMemory(path, BUFLEN);
						Kit* kit = gkCycle[0]->kit;
						if (kit->fileName[0] != 'x')
						{
							lstrcpy(path, g_config.kdbDir);
							lstrcat(path, "KDB\\goalkeepers\\"); 
							lstrcat(path, kit->fileName);

							// load new kit
							FILE* f = fopen(path, "rb");
							if (f != NULL)
							{
								fseek(f, 4, SEEK_SET);
								DWORD size;
								fread(&size, 4, 1, f);
								fseek(f, 0, SEEK_SET);
								gkUniBuf[0] = (char*)HeapAlloc(procHeap, HEAP_ZERO_MEMORY, size + 32);
								fread(gkUniBuf[0], size + 32, 1, f);
								fclose(f);
								TRACE2("gkUniBuf[0] address = %08x", (DWORD)gkUniBuf[0]);

								gkUniBuf2[0] = (char*)HeapAlloc(procHeap, HEAP_ZERO_MEMORY, size + 32);
								memcpy(gkUniBuf2[0], gkUniBuf[0], size + 32);
							}

							homeNN = (NNColors*)(data[COLORS] + TEAM_CODE(0) * 104);
							homeCollars = (BYTE*)(data[COLLARS] + TEAM_CODE(0) * 20);
							awayNN = (NNColors*)(data[COLORS] + TEAM_CODE(1) * 104);
							awayCollars = (BYTE*)(data[COLLARS] + TEAM_CODE(1) * 20);

							// save info, if not saved yet
							if (!extraGKInfoSaved)
							{
								memcpy(&saveHome.g1, &homeNN->g1, sizeof(NNStrip));
								memcpy(&saveHome.g2, &homeNN->g2, sizeof(NNStrip));
								memcpy(&saveAway.g1, &awayNN->g1, sizeof(NNStrip));
								memcpy(&saveAway.g2, &awayNN->g2, sizeof(NNStrip));
								memcpy(saveHomeCollars + 2, homeCollars + 2, 2);
								memcpy(saveAwayCollars + 2, awayCollars + 2, 2);
								extraGKInfoSaved = true;
							}
						}
						else
						{
							// default kit: just clear out the buffer pointer
							gkUniBuf[0] = gkUniBuf2[0] = NULL;
						}
					}
				}
				else if (wParam == g_config.vKeyGKAwayKit && IS_STRIP_SELECTION)
				{
					if (gkCycle[1] != NULL)
					{
						// move on to next uniform
						gkCycle[1] = gkCycle[1]->next;

						// switch away goalkeeper uniform
						HANDLE procHeap = GetProcessHeap();
						// free previous kit, if any was loaded
						if (gkUniBuf[1] != NULL) { HeapFree(procHeap, 0, gkUniBuf[1]); }
						if (gkUniBuf2[1] != NULL) { HeapFree(procHeap, 0, gkUniBuf2[1]); }
						// determine next kit to load
						char path[BUFLEN];
						ZeroMemory(path, BUFLEN);
						Kit* kit = gkCycle[1]->kit;
						if (kit->fileName[0] != 'x')
						{
							lstrcpy(path, g_config.kdbDir);
							lstrcat(path, "KDB\\goalkeepers\\"); 
							lstrcat(path, kit->fileName);

							// load new kit
							FILE* f = fopen(path, "rb");
							if (f != NULL)
							{
								fseek(f, 4, SEEK_SET);
								DWORD size;
								fread(&size, 4, 1, f);
								fseek(f, 0, SEEK_SET);
								gkUniBuf[1] = (char*)HeapAlloc(procHeap, HEAP_ZERO_MEMORY, size + 32);
								fread(gkUniBuf[1], size + 32, 1, f);
								fclose(f);
								TRACE2("gkUniBuf[1] address = %08x", (DWORD)gkUniBuf[1]);

								gkUniBuf2[1] = (char*)HeapAlloc(procHeap, HEAP_ZERO_MEMORY, size + 32);
								memcpy(gkUniBuf2[1], gkUniBuf[1], size + 32);
							}

							homeNN = (NNColors*)(data[COLORS] + TEAM_CODE(0) * 104);
							homeCollars = (BYTE*)(data[COLLARS] + TEAM_CODE(0) * 20);
							awayNN = (NNColors*)(data[COLORS] + TEAM_CODE(1) * 104);
							awayCollars = (BYTE*)(data[COLLARS] + TEAM_CODE(1) * 20);

							// save info, if not saved yet
							if (!extraGKInfoSaved)
							{
								memcpy(&saveHome.g1, &homeNN->g1, sizeof(NNStrip));
								memcpy(&saveHome.g2, &homeNN->g2, sizeof(NNStrip));
								memcpy(&saveAway.g1, &awayNN->g1, sizeof(NNStrip));
								memcpy(&saveAway.g2, &awayNN->g2, sizeof(NNStrip));
								memcpy(saveHomeCollars + 2, homeCollars + 2, 2);
								memcpy(saveAwayCollars + 2, awayCollars + 2, 2);
								extraGKInfoSaved = true;
							}
						}
						else
						{
							// default kit: just clear out the buffer pointer
							gkUniBuf[1] = gkUniBuf2[1] = NULL;
						}
					}
				}
				else if (wParam == g_config.vKeyBall && IS_STRIP_SELECTION)
				{
					// select the ball
					if (ballCycle != NULL && ballCycle->ball != NULL)
					{
						Ball* ball = ballCycle->ball;
						if (ball->name[0] != '\0') // non-empty name
						{
							// free previously loaded ball, if any
							FreeBall();

							LogWithString("Now switching to '%s' ball.", ball->name);
							lstrcpy(g_ballName, ball->name);

							// load the ball into buffers
							char bbuf_mdl[BUFLEN];
							ZeroMemory(bbuf_mdl, BUFLEN);
							if (lstrlen(ball->mdlFileName) > 0)
							{
								lstrcpy(bbuf_mdl, g_config.kdbDir);
								lstrcat(bbuf_mdl, "KDB\\balls\\");
								lstrcat(bbuf_mdl, ball->mdlFileName);
							}
							char bbuf_tex[BUFLEN];
							ZeroMemory(bbuf_tex, BUFLEN);
							lstrcpy(bbuf_tex, g_config.kdbDir);
							lstrcat(bbuf_tex, "KDB\\balls\\");
							lstrcat(bbuf_tex, ball->texFileName);

							LoadBall(bbuf_mdl, bbuf_tex);
						}
						else
						{
							// clear out the pointers
							g_ballMdl = g_ballTex = NULL;

							LogWithString("Now switching to default ball.", ball->name);
							lstrcpy(g_ballName, "Default");
						}

						// move on to next ball
						ballCycle = ballCycle->next;

						if (ballCycle != NULL)
						{
							ball = ballCycle->ball;
							if (ball != NULL)
								LogWithString("Next ball file: %s", ball->texFileName);
						}

					} // end if (ballCycle ...
				}
				else if (wParam == g_config.vKeyRandomBall && IS_STRIP_SELECTION)
				{
					// randomly select one of the available balls
					if (ballCycle != NULL && ballCycle->ball != NULL)
					{
						// cycle to a random ball
						LARGE_INTEGER num;
						QueryPerformanceCounter(&num);
						int iterations = num.LowPart % MAX_ITERATIONS;
						TRACE2("Random iterations: %d", iterations);
						for (int j=0; j < iterations; j++)
						{
							ballCycle = ballCycle->next;
						}

						Ball* ball = ballCycle->ball;

						if (ball->name[0] != '\0') // non-empty name
						{
							// free previously loaded ball, if any
							FreeBall();

							LogWithString("Now switching to '%s' ball.", ball->name);
							lstrcpy(g_ballName, ball->name);

							// load the ball into buffers
							char bbuf_mdl[BUFLEN];
							ZeroMemory(bbuf_mdl, BUFLEN);
							if (lstrlen(ball->mdlFileName) > 0)
							{
								lstrcpy(bbuf_mdl, g_config.kdbDir);
								lstrcat(bbuf_mdl, "KDB\\balls\\");
								lstrcat(bbuf_mdl, ball->mdlFileName);
							}
							char bbuf_tex[BUFLEN];
							ZeroMemory(bbuf_tex, BUFLEN);
							lstrcpy(bbuf_tex, g_config.kdbDir);
							lstrcat(bbuf_tex, "KDB\\balls\\");
							lstrcat(bbuf_tex, ball->texFileName);

							LoadBall(bbuf_mdl, bbuf_tex);
						}
						else
						{
							// clear out the pointers
							g_ballMdl = g_ballTex = NULL;

							LogWithString("Now switching to default ball.", ball->name);
							lstrcpy(g_ballName, "Default");
						}

						// move on to next ball
						ballCycle = ballCycle->next;

						if (ballCycle != NULL)
						{
							ball = ballCycle->ball;
							if (ball != NULL)
								LogWithString("Next ball file: %s", ball->texFileName);
						}

					} // end if (ballCycle ...
				}
			}
			break;
	}

	// We must pass the all messages on to CallNextHookEx.
	return CallNextHookEx(g_hKeyboardHook, code, wParam, lParam);
}

/**
 * Resets cycles to point to dummies. Buffers get cleared.
 */
void ResetCyclesAndBuffers2()
{
	TRACE("ResetCyclesAndBuffers2: CALLED.");

	if (kDB != NULL)
	{
		// restore original colors and collar for players and goalkeepers
		if (extraInfoSaved)
		{
			memcpy(&homeNN->p1, &saveHome.p1, sizeof(NNStrip));
			memcpy(&homeNN->p2, &saveHome.p2, sizeof(NNStrip));
			memcpy(&awayNN->p1, &saveAway.p1, sizeof(NNStrip));
			memcpy(&awayNN->p2, &saveAway.p2, sizeof(NNStrip));
			memcpy(homeCollars, saveHomeCollars, 2);
			memcpy(awayCollars, saveAwayCollars, 2);
			TRACE("ResetCyclesAndBuffers2: PL number colors and collars restored.");

			extraInfoSaved = false;
		}
		// restore original colors and collar for goalkeepers
		if (extraGKInfoSaved)
		{
			memcpy(&homeNN->g1, &saveHome.g1, sizeof(NNStrip));
			memcpy(&homeNN->g2, &saveHome.g2, sizeof(NNStrip));
			memcpy(&awayNN->g1, &saveAway.g1, sizeof(NNStrip));
			memcpy(&awayNN->g2, &saveAway.g2, sizeof(NNStrip));
			memcpy(homeCollars + 2, saveHomeCollars + 2, 2);
			memcpy(awayCollars + 2, saveAwayCollars + 2, 2);
			TRACE("ResetCyclesAndBuffers2: GK number colors and collars restored.");

			extraGKInfoSaved = false;
		}

		// free player buffers
		for (int i=0; i<4; i++)
		{
			SAFEFREE(unibuf1[i]);
			SAFEFREE(unibuf2[i]);
		}
		TRACE("ResetCyclesAndBuffers2: player buffers freed.");

		// initialize kit-cycles
		KitEntry* p = kDB->players[TEAM_CODE(0)];
		KitEntry* first = p;
		while (p && (p->next != first)) p = p->next; // roll to point to dummy 
		kitCycle[0] = kitCycle[2] = p;
		p = kDB->players[TEAM_CODE(1)];
		first = p;
		while (p && (p->next != first)) p = p->next; // roll to point to dummy 
		kitCycle[1] = kitCycle[3] = p;

		TRACE("ResetCyclesAndBuffers2: player cycles set to dummies.");

		// free goalkeeper buffers
		SAFEFREE(gkUniBuf[0]); SAFEFREE(gkUniBuf2[0]);
		SAFEFREE(gkUniBuf[1]); SAFEFREE(gkUniBuf2[1]);
		TRACE("ResetCyclesAndBuffers2: goalkeeper buffers freed.");

		// initialize goalkeeper-cycles
		gkCycle[0] = kDB->goalkeepers[TEAM_CODE(0)];
		gkCycle[1] = kDB->goalkeepers[TEAM_CODE(1)];

		// roll goalkeeper-cycles so that they point to dummy kitentries initially
		first = gkCycle[0];
		while (gkCycle[0] && (gkCycle[0]->next != first)) gkCycle[0] = gkCycle[0]->next;
		first = gkCycle[1];
		while (gkCycle[1] && (gkCycle[1]->next != first)) gkCycle[1] = gkCycle[1]->next;

		g_kitsAvailable = 
			kitCycle[0] || kitCycle[1] || kitCycle[2] || kitCycle[3] ||
			gkCycle[0] || gkCycle[1];

		TRACE("ResetCyclesAndBuffers2: goalkeeper cycles set to dummies.");
	}
}

/* Set debug flag */
EXTERN_C LIBSPEC void SetDebug(DWORD flag)
{
	//LogWithNumber("Setting g_config.debug flag to %d", flag);
	g_config.debug = flag;
}

EXTERN_C LIBSPEC DWORD GetDebug(void)
{
	return g_config.debug;
}

/* Set key */
EXTERN_C LIBSPEC void SetKey(int whichKey, int code)
{
	switch (whichKey)
	{
		case VKEY_HOMEKIT: g_config.vKeyHomeKit = code; break;
		case VKEY_AWAYKIT: g_config.vKeyAwayKit = code; break;
		case VKEY_GKHOMEKIT: g_config.vKeyGKHomeKit = code; break;
		case VKEY_GKAWAYKIT: g_config.vKeyGKAwayKit = code; break;
		case VKEY_BALL: g_config.vKeyBall = code; break;
		case VKEY_RANDOM_BALL: g_config.vKeyRandomBall = code; break;
	}
}

EXTERN_C LIBSPEC int GetKey(int whichKey)
{
	switch (whichKey)
	{
		case VKEY_HOMEKIT: return g_config.vKeyHomeKit;
		case VKEY_AWAYKIT: return g_config.vKeyAwayKit;
		case VKEY_GKHOMEKIT: return g_config.vKeyGKHomeKit;
		case VKEY_GKAWAYKIT: return g_config.vKeyGKAwayKit;
		case VKEY_BALL: return g_config.vKeyBall;
		case VKEY_RANDOM_BALL: return g_config.vKeyRandomBall;
	}
	return -1;
}

/* Set kdb dir */
EXTERN_C LIBSPEC void SetKdbDir(char* kdbDir)
{
	if (kdbDir == NULL) return;
	ZeroMemory(g_config.kdbDir, BUFLEN);
	lstrcpy(g_config.kdbDir, kdbDir);
}

EXTERN_C LIBSPEC void GetKdbDir(char* kdbDir)
{
	if (kdbDir == NULL) return;
	ZeroMemory(kdbDir, BUFLEN);
	lstrcpy(kdbDir, g_config.kdbDir);
}

LPVOID JuceLoadUni(DWORD buf, DWORD somedata)
{
	// maintain the count of calls to this function
	// so that we know when goalkeeper uniform needs to be swapped
	g_loadUniCount++;
	TRACE2("JuceLoadUni: g_loadUniCount = %d", g_loadUniCount);
	TRACE2("JuceLoadUni: somedata = %08x", somedata);

	//Log("JuceLoadUni: BEFORE code[C_LOADUNI]");
	DWORD uni = buf;
	TRACE2("JuceLoadUni: g_uniToggleStep = %d", g_uniToggleStep);
	int i = g_uniToChange % 2;
	TRACE2("JuceLoadUni: i = %d", i);

	BYTE homeStrips = 0;
	int index = 0;

	switch (g_uniToggleStep)
	{
		case FAKE1:
			if (unibuf1[i]) uni = (DWORD)unibuf1[i];
			g_uniToggleStep = FAKE2;
			break;
		case FAKE2:
			if (unibuf1[i + 2]) uni = (DWORD)unibuf1[i + 2];
			g_uniToggleStep = FAKEGLOVES;
			break;
		case FAKEGLOVES:
			g_uniToggleStep = SWITCHTEAMBACK;
			break;
		case REAL1:
			if (unibuf2[i]) uni = (DWORD)unibuf2[i];
			g_uniToggleStep = REAL2;
			break;
		case REAL2:
			if (unibuf2[i + 2]) uni = (DWORD)unibuf2[i + 2];
			g_uniToggleStep = REALGLOVES;
			break;
		case REALGLOVES:
			g_uniToggleStep = STOP;
			// set uni-load count to 6: we're done swapping player uniform, 
			// next actions should be loading of the goalkeeper uniforms
			g_loadUniCount = 6;
			break;
		default:
			TRACE("JuceLoadUni: no uni-toggle action");
			switch (g_loadUniCount)
			{
				case 7:
					if (ABOUT_TO_LOAD_STRIPS)
					{
						// it's not really a home goalkeeper uniform loading
						// Therefore, reset the count
						g_loadUniCount = 1;
						break;
					}
					// #7 is always GK kit for HOME team
					if (gkUniBuf[0] != NULL)
					{
						uni = (DWORD)gkUniBuf[0];
						TRACE2("JuceLoadUni: HOME GK: %08x", *((DWORD*)(uni + 0x20)));

						// set number/name colors
						Kit* kit = gkCycle[0]->kit;
						SetGKHomeNNColors(kit, TRUE);
					}
					break;
				case 8:
					// #8 is always GK kit for AWAY team
					if (gkUniBuf[1] != NULL)
					{
						uni = (DWORD)gkUniBuf[1];
						TRACE2("JuceLoadUni: AWAY GK: %08x", *((DWORD*)(uni + 0x20)));

						// set number/name colors
						Kit* kit = gkCycle[1]->kit;
						SetGKAwayNNColors(kit, TRUE);
					}
					break;
				case 9:
					if (ABOUT_TO_LOAD_STRIPS)
					{
						// it's not really a home goalkeeper uniform loading
						// Therefore, reset the count
						g_loadUniCount = 1;
						break;
					}
					// #9 can be a GK for either HOME or AWAY team
					// Determine which team is it: HOME or AWAY
					index = (g_strip[0] & 0x02) ? 0 : 1;
					if (gkUniBuf2[index] != NULL)
					{
						uni = (DWORD)gkUniBuf2[index];
						TRACE4("JuceLoadUni: %s TEAM", WHICH_TEAM[index]);
						TRACE2("JuceLoadUni: 2nd GK strip: %08x", *((DWORD*)(uni + 0x20)));

						// set number/name colors
						Kit* kit = gkCycle[index]->kit;
						if (index == 0) SetGKHomeNNColors(kit, TRUE);
						else SetGKAwayNNColors(kit, TRUE);
					}
					break;
				case 10:
					if (ABOUT_TO_LOAD_STRIPS)
					{
						// it's not really a home goalkeeper uniform loading
						// Therefore, reset the count
						g_loadUniCount = 1;
						break;
					}
					// #10 is always GK kit for AWAY team
					if (gkUniBuf2[1] != NULL)
					{
						uni = (DWORD)gkUniBuf2[1];
						TRACE("JuceLoadUni: AWAY TEAM");
						TRACE2("JuceLoadUni: 2nd GK strip: %08x", *((DWORD*)(uni + 0x20)));

						// set number/name colors
						Kit* kit = gkCycle[1]->kit;
						SetGKAwayNNColors(kit, TRUE);
					}
					break;

				default:
					// time to reset the count, if got over 10
					if (g_loadUniCount > 10) g_loadUniCount = 1;
					break;
			}
	}
	LPVOID result = LoadUni(uni, somedata);
	TRACE("JuceLoadUni: AFTER  code[C_LOADUNI]");
	return result;
}

/**
 * Load the KDB database.
 */
void LoadKDB()
{
	kDB = kdbLoad(g_config.kdbDir);
	if (kDB != NULL)
	{
		//print kDB
		TRACE2("KDB player count: %d", kDB->playerCount);
		if (kDB->playerCount > 0) for (int k=0; k<256; k++)
		{
			if (kDB->players[k] == NULL) continue;
			TRACE2("team %d", k);
			KitEntry* p = kDB->players[k];
			while (p != NULL)
			{
				TRACE(p->kit->fileName);
				if (p->next == kDB->players[k]) break; // full cycle
				p = p->next;
			}
		}

		TRACE2("KDB goalkeeper count: %d", kDB->goalkeeperCount);
		if (kDB->goalkeeperCount > 0) for (int k=0; k<256; k++)
		{
			if (kDB->goalkeepers[k] == NULL) continue;
			TRACE2("team %d", k);
			KitEntry* p = kDB->goalkeepers[k];
			while (p != NULL)
			{
				TRACE(p->kit->fileName);
				if (p->next == kDB->goalkeepers[k]) break; // full cycle
				p = p->next;
			}
		}

		BallEntry* p = kDB->balls;
		if (p != NULL) TRACE("KDB balls:");
		while (p != NULL)
		{
			TRACE(p->ball->texFileName);
			if (p->next == kDB->balls) break; // full cycle
			p = p->next;
		}

		// initialize ball-cycle
		FreeBall(); // free any currently loaded ball
		ballCycle = kDB->balls;

		TRACE("KDB loaded.");
		g_ballsAvailable = (kDB->balls != NULL);
	}
	else
	{
		TRACE("Problem loading KDB (kDB == NULL).");
	}
}

/**
 * This function gets called before the strip selection
 * screen is displayed. Its job is to restore original
 * number colors and collars of the strips.
 */
LPVOID JuceRestoreAll(DWORD ptr)
{
	if (presentPtr == NULL)
	{
		// save handle to process' heap
		procHeap = GetProcessHeap();

		// get handle to D3D8
		hD3D8 = (HINSTANCE)GetModuleHandle("d3d8.dll");

		okToHook = true;
		CheckAndHookPresent();
	}

	TRACE("JuceRestoreAll: ABOUT TO SHOW STRIP SELECTION");
	// if we're are not in exhibition mode - do cleanup
	if (!(g_MainOrExhib == 1))
		ResetCyclesAndBuffers2();

	// we hooked at a certain place when collar-loading routine
	// is called. So, we must call it to preserve the normal flow
	// of control.
	LPVOID result = LoadCollars(ptr);
	return result;
}

/**
 * This function gets called after "quick" team/strip selection
 * screen is displayed. Its job is to restore original
 * number colors and collars of the strips.
 */
LPVOID JuceRestoreAll2()
{
	TRACE("JuceRestoreAll2: ABOUT TO LOAD NEW TEAMS/STRIPS");

	ResetCyclesAndBuffers2();

	// we hooked at a certain place when collar-loading routine
	// is called. So, we must call it to preserve the normal flow
	// of control.
	LPVOID result = QuickSelect();
	return result;
}

void DumpData(DWORD* addr, DWORD size)
{
	static int count = 0;

	// prepare filename
	char name[BUFLEN];
	ZeroMemory(name, BUFLEN);
	sprintf(name, "%s%s%02d.dat", mydir, "dd", count++);

	FILE* f = fopen(name, "wb");
	if (f != NULL)
	{
		fwrite(addr, size, 1, f);
		fclose(f);
	}
}

/**
 * This function saves the textures from decoded
 * uniform buffer into two BMP files: 512x256x8 and 256x128x8
 * (with palette).
 */
void DumpUniforms(BYTE* buffer)
{
	static int count = 0;

	// prepare 1st filename
	char name[BUFLEN];
	ZeroMemory(name, BUFLEN);
	sprintf(name, "%s%s%02d.bmp", mydir, "du", count++);

	SaveAs8bitBMP(name, buffer + 0x480, buffer + 0x80, 512, 256);

	// prepare 2nd filename
	ZeroMemory(name, BUFLEN);
	sprintf(name, "%s%s%02d.bmp", mydir, "du", count++);

	SaveAs8bitBMP(name, buffer + 0x20480, buffer + 0x80, 256, 128);
}

/**
 * This function calls the unpack function.
 * Parameters:
 *   addr    - address of the encoded buffer
 *   sSizeLo - lower 4 bytes of the encoded buffer size
 *   sSizeHi - higher 4 bytes of the encoded buffer size (usually 0)
 *   dSizeLo - lower 4 bytes of the decoded buffer size
 *   dSizeHi - higher 4 bytes of the decoded buffer size (usually 0)
 * Return value:
 *   address of the decoded buffer
 */
DWORD JuceBinUnpack(DWORD addr, DWORD sSizeLo, DWORD sSizeHi, DWORD dSizeLo, DWORD dSizeHi)
{
	DWORD a = addr;
	DWORD sLo = sSizeLo;
	DWORD sHi = sSizeHi;
	DWORD dLo = dSizeLo;
	DWORD dHi = dSizeHi;

	switch (g_binUnpackCount)
	{
		case 0:
			// Load ball model, if any defined
			if (g_ballMdl != NULL)
			{
				a = (DWORD)(g_ballMdl + 0x20);
				sLo = (DWORD)(*((DWORD*)(g_ballMdl + 4)));
				sHi = 0;
				dLo = (DWORD)(*((DWORD*)(g_ballMdl + 8)));
				dHi = 0;
				Log("JuceBinUnpack: switching ball model.");
			}
			g_binUnpackCount++;
			break;
		case 1:
			// Load ball texture, if any defined
			if (g_ballTex != NULL)
			{
				a = (DWORD)(g_ballTex + 0x20);
				sLo = (DWORD)(*((DWORD*)(g_ballTex + 4)));
				sHi = 0;
				dLo = (DWORD)(*((DWORD*)(g_ballTex + 8)));
				dHi = 0;
				Log("JuceBinUnpack: switching ball texture.");
			}
			g_binUnpackCount = -1;
			break;
	}

	// call the hooked function
	DWORD result = BinUnpack(a, sLo, sHi, dLo, dHi);

if (dLo == 0x1f1d0)
{
	// save etc_ee_tex
	//DumpData((DWORD*)result, dLo);
	// save pointer to texture pack
	g_etcEeTex = (BYTE*)result;
	TRACE2("g_etcEeTex = %08x", (DWORD)g_etcEeTex);

	// load EPL numbers from a special file
	g_eplNumbers = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x5100);

	g_homeNumberData.valid = FALSE; // invalidates the structure content
	g_awayNumberData.valid = FALSE; // invalidates the structure content

	char name[BUFLEN];
	ZeroMemory(name, BUFLEN);
	lstrcpy(name, mydir); lstrcat(name, "eplnum.dat");
	FILE* f = fopen(name, "rb");
	fread(g_eplNumbers, 0x5100, 1, f);
	fclose(f);

	TRACE2("g_eplNumbers = %08x", (DWORD)g_eplNumbers);
}
// save uniforms as BMPs
if (dLo == 0x28480) // only uniform files have unpacked buffer size of 0x28480
{
	DumpData((DWORD*)result, dLo);
	DumpUniforms((BYTE*)result);
}

	return result;
}

/**
 * This function calls the ball loader function.
 */
LPVOID JuceBallLoader(DWORD param)
{
	g_binUnpackCount = 0;

	Log("JuceBallLoader: CALLED.");

	// call the hooked function
	LPVOID result = BallLoader(param);
	return result;
}


// Returns the game version id
int GetGameVersion(void)
{
	for (int i=0; i<4; i++)
	{
		memcpy(sigQuickSelect, sigArrayQuickSelect[i], 16);
		if (memcmp(sigQuickSelect, (BYTE*)(codeArray[i][1]), 16) == 0)
			return i;
	}
	return -1;
}

// Initialize pointers and data structures.
void Initialize(int v)
{
	// select correct addresses
	memcpy(code, codeArray[v], sizeof(code));
	memcpy(data, dataArray[v], sizeof(data));

	// select correct footprints
	memcpy(sigLoadUni, sigArrayLoadUni[v], sizeof(sigLoadUni));
	memcpy(sigLoadCollars, sigArrayLoadCollars[v], sizeof(sigLoadCollars));
	memcpy(sigQuickSelect, sigArrayQuickSelect[v], sizeof(sigQuickSelect));

	// assign pointers
	LoadUni = (LOADUNI)code[C_LOADUNI];
	LoadCollars = (LOADCOLLARS)code[C_LOADCOLLARS];
	QuickSelect = (QUICKSELECT)code[C_QUICKSELECT];
	BinUnpack = (BINUNPACK)code[C_DECODEBIN];
	CheckTeam = (CHECKTEAM)code[C_CHECKTEAM];
	BallLoader = (FUNC2)code[C_LOADBALL];

	g_strip = (BYTE*)data[STRIPS];
}

/* Writes an indexed 8-bit BMP file (with palette) */
HRESULT SaveAs8bitBMP(char* filename, BYTE* buf, BYTE* pal, LONG width, LONG height)
{
	BITMAPFILEHEADER fhdr;
	BITMAPINFOHEADER infoheader;
	SIZE_T size = width * height; // size of data in bytes

	// fill in the headers
	fhdr.bfType = 0x4D42; // "BM"
	fhdr.bfSize = sizeof(fhdr) + sizeof(infoheader) + size;
	fhdr.bfReserved1 = 0;
	fhdr.bfReserved2 = 0;
	fhdr.bfOffBits = sizeof(fhdr) + sizeof(infoheader) + 0x400;

	infoheader.biSize = sizeof(infoheader);
	infoheader.biWidth = width;
	infoheader.biHeight = height;
	infoheader.biPlanes = 1;
	infoheader.biBitCount = 8;
	infoheader.biCompression = BI_RGB;
	infoheader.biSizeImage = 0;
	infoheader.biXPelsPerMeter = 0;
	infoheader.biYPelsPerMeter = 0;
	infoheader.biClrUsed = 256;
	infoheader.biClrImportant = 0;

	// prepare filename
	char name[BUFLEN];
	if (filename == NULL)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		ZeroMemory(name, BUFLEN);
		sprintf(name, "%s%s-%d%02d%02d-%02d%02d%02d%03d.bmp", mydir, "kserv", 
				time.wYear, time.wMonth, time.wDay,
				time.wHour, time.wMinute, time.wSecond, time.wMilliseconds); 
		filename = name;
	}

	// save to file
	DWORD wbytes;
	HANDLE hFile = CreateFile(filename,            // file to create 
					 GENERIC_WRITE,                // open for writing 
					 0,                            // do not share 
					 NULL,                         // default security 
					 OPEN_ALWAYS,                  // overwrite existing 
					 FILE_ATTRIBUTE_NORMAL,        // normal file 
					 NULL);                        // no attr. template 

	if (hFile != INVALID_HANDLE_VALUE) 
	{
		WriteFile(hFile, &fhdr, sizeof(fhdr), (LPDWORD)&wbytes, NULL);
		WriteFile(hFile, &infoheader, sizeof(infoheader), (LPDWORD)&wbytes, NULL);
		// write palette
		BYTE zero = 0;
		for (int bank=0; bank<8; bank++)
		{
			for (int i=0; i<8; i++)
			{
				WriteFile(hFile, pal + bank*32*4 + i*4 + 2, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 1, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, &zero, 1, (LPDWORD)&wbytes, NULL);
			}
			for (i=16; i<24; i++)
			{
				WriteFile(hFile, pal + bank*32*4 + i*4 + 2, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 1, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, &zero, 1, (LPDWORD)&wbytes, NULL);
			}
			for (i=8; i<16; i++)
			{
				WriteFile(hFile, pal + bank*32*4 + i*4 + 2, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 1, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, &zero, 1, (LPDWORD)&wbytes, NULL);
			}
			for (i=24; i<32; i++)
			{
				WriteFile(hFile, pal + bank*32*4 + i*4 + 2, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 1, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, &zero, 1, (LPDWORD)&wbytes, NULL);
			}
		}
		// write pixel data
		for (int k=height-1; k>=0; k--)
		{
			WriteFile(hFile, buf + k*width, width, (LPDWORD)&wbytes, NULL);
		}
		CloseHandle(hFile);
	}
	else 
	{
		Log("SaveAs8bitBMP: failed to save to file.");
		return E_FAIL;
	}
	return S_OK;
}

/* Writes a BMP file */
HRESULT SaveAsBMP(char* filename, BYTE* rgbBuf, SIZE_T size, LONG width, LONG height, int bpp)
{
	BITMAPFILEHEADER fhdr;
	BITMAPINFOHEADER infoheader;

	// fill in the headers
	fhdr.bfType = 0x4D42; // "BM"
	fhdr.bfSize = sizeof(fhdr) + sizeof(infoheader) + size;
	fhdr.bfReserved1 = 0;
	fhdr.bfReserved2 = 0;
	fhdr.bfOffBits = sizeof(fhdr) + sizeof(infoheader);

	infoheader.biSize = sizeof(infoheader);
	infoheader.biWidth = width;
	infoheader.biHeight = height;
	infoheader.biPlanes = 1;
	infoheader.biBitCount = bpp*8;
	infoheader.biCompression = BI_RGB;
	infoheader.biSizeImage = 0;
	infoheader.biXPelsPerMeter = 0;
	infoheader.biYPelsPerMeter = 0;
	infoheader.biClrUsed = 0;
	infoheader.biClrImportant = 0;

	// prepare filename
	char name[BUFLEN];
	if (filename == NULL)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		ZeroMemory(name, BUFLEN);
		sprintf(name, "%s%s-%d%02d%02d-%02d%02d%02d%03d.bmp", mydir, "kserv", 
				time.wYear, time.wMonth, time.wDay,
				time.wHour, time.wMinute, time.wSecond, time.wMilliseconds); 
		filename = name;
	}

	// save to file
	DWORD wbytes;
	HANDLE hFile = CreateFile(filename,            // file to create 
					 GENERIC_WRITE,                // open for writing 
					 0,                            // do not share 
					 NULL,                         // default security 
					 OPEN_ALWAYS,                  // overwrite existing 
					 FILE_ATTRIBUTE_NORMAL,        // normal file 
					 NULL);                        // no attr. template 

	if (hFile != INVALID_HANDLE_VALUE) 
	{
		WriteFile(hFile, &fhdr, sizeof(fhdr), (LPDWORD)&wbytes, NULL);
		WriteFile(hFile, &infoheader, sizeof(infoheader), (LPDWORD)&wbytes, NULL);
		WriteFile(hFile, rgbBuf, size, (LPDWORD)&wbytes, NULL);
		CloseHandle(hFile);
	}
	else 
	{
		Log("SaveAsBMP: failed to save to file.");
		return E_FAIL;
	}
	return S_OK;
}

/**
 * This function gets called once for each team being loaded.
 * We do number shape manipulation here.
 */
DWORD JuceCheckTeam(DWORD num)
{
	// call the real function first, to get the proper team number.
	DWORD teamNumber = CheckTeam(num) & 0xff;

	DWORD callSite = *((DWORD*)((DWORD)&num - 4)) - 5;
	TRACE2("JuceCheckTeam: call site = %08x", callSite);
	TRACE2("JuceCheckTeam: TEAM = %d", teamNumber);

	// COPY EPL NUMBERS DATA for Newcastle or Man Utd
	BYTE homeNum = TEAM_CODE(0);
	TRACE2("JuceCheckTeam: (homeNum == (BYTE)teamNumber) = %d", (homeNum == (BYTE)teamNumber));
	NumberData* numData = (homeNum == (BYTE)teamNumber) ? &g_homeNumberData : &g_awayNumberData;

	// restore original data if loading new team
	if (numData->valid && teamNumber != numData->savedTeam)
	{
		if (g_etcEeTex)
		{
			BYTE* numberTypePtr = (BYTE*)(data[NUMTYPES] + numData->savedTeam * 20);
			int savedType = numData->savedType;
			int type = numData->type;

			// modify number type byte
			*numberTypePtr = savedType; 
			// modify number shape
			memcpy(g_etcEeTex + g_numberOffsets[type], numData->savedShape, 0x5100);
			TRACE("Original number shapes restored.");

			// invalidate data structure
			numData->valid = FALSE;
		}
	}

	// load EPL number shapes for Newcastle or Man Utd
	if (teamNumber == 77 || teamNumber == 76)
	{
		// save original numbers (if hasn't done so yet)
		if (!numData->valid)
		{
			numData->savedTeam = teamNumber;
			BYTE* numberTypePtr = (BYTE*)(data[NUMTYPES] + numData->savedTeam * 20);
			numData->savedType = *numberTypePtr;

			int type = (numData->savedType + 1) % 4; // take next number type
			memcpy(numData->savedShape, g_etcEeTex + g_numberOffsets[type], 0x5100);
			memcpy(numData->shape, g_eplNumbers, 0x5100);
			numData->type = type;

			numData->valid = TRUE;
		}

		// put EPL numbers in
		if (g_etcEeTex && numData->valid)
		{
			BYTE* numberTypePtr = (BYTE*)(data[NUMTYPES] + teamNumber * 20);
			int type = numData->type;

			// modify number type byte
			*numberTypePtr = type; 
			// modify number shape
			memcpy(g_etcEeTex + g_numberOffsets[type], numData->shape, 0x5100);
			TRACE("EPL numbers copied.");
		}
	}

	return teamNumber;
}
