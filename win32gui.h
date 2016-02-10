// win32gui.h

#ifndef _JUCE_WIN32GUI
#define _JUCE_WIN32GUI

#include <windows.h>

#define WIN_WIDTH 350 
#define WIN_HEIGHT 182

extern HWND g_kdbDirControl;               // displays path to folder that contains KDB

extern HWND g_keyHomeKitControl;           // displays key binding for home kit switch
extern HWND g_keyAwayKitControl;           // displays key binding for away kit switch
extern HWND g_keyGKHomeKitControl;         // displays key binding for home GK kit switch
extern HWND g_keyGKAwayKitControl;         // displays key binding for away GK kit switch
extern HWND g_keyBallControl;              // displays key binding for ball switch
extern HWND g_keyRandomBallControl;        // displays key binding for random ball switch

extern HWND g_statusTextControl;           // displays status messages
extern HWND g_restoreButtonControl;        // restore settings button
extern HWND g_saveButtonControl;           // save settings button

// macros
#define VKEY_TEXT(key, buffer, buflen) \
	ZeroMemory(buffer, buflen); \
	GetKeyNameText(MapVirtualKey(key, 0) << 16, buffer, buflen)

// functions
bool BuildControls(HWND parent);

#endif
