#ifndef _JUCE_CONFIG
#define _JUCE_CONFIG

#include <windows.h>

#define BUFLEN 4096

#define CONFIG_FILE "kserv.cfg"

#define DEFAULT_DEBUG 0
#define DEFAULT_KDB_DIR ".\\"
#define DEFAULT_VKEY_HOMEKIT 0x31
#define DEFAULT_VKEY_AWAYKIT 0x32
#define DEFAULT_VKEY_GKHOMEKIT 0x33
#define DEFAULT_VKEY_GKAWAYKIT 0x34
#define DEFAULT_VKEY_BALL 0x42
#define DEFAULT_VKEY_RANDOM_BALL 0x43
#define DEFAULT_ASPECT_RATIO 0.0f
#define DEFAULT_GAME_SPEED 0.0f
#define DEFAULT_SCREEN_WIDTH 0
#define DEFAULT_SCREEN_HEIGHT 0
#define DEFAULT_CAMERA_ZOOM 1800.0f
#define DEFAULT_CAMERA_ANGLE_MULTIPLIER 1
#define DEFAULT_STADIUM_RENDER_CLIP 1
#define DEFAULT_STADIUM_RENDER_HEIGHT 0.0f

typedef struct _KSERV_CONFIG_STRUCT {
	DWORD  debug;
	char   kdbDir[BUFLEN];
	WORD   vKeyHomeKit;
	WORD   vKeyAwayKit;
	WORD   vKeyGKHomeKit;
	WORD   vKeyGKAwayKit;
	WORD   vKeyBall;
	WORD   vKeyRandomBall;
    float  aspectRatio;
    float  gameSpeed;
    DWORD  screenWidth;
    DWORD  screenHeight;
    float  cameraZoom;
    DWORD  cameraAngleMultiplier;
    DWORD  stadiumRenderClip;
    float  stadiumRenderHeight;

} KSERV_CONFIG;

BOOL ReadConfig(KSERV_CONFIG* config, char* cfgFile);
BOOL WriteConfig(KSERV_CONFIG* config, char* cfgFile);

#endif
