#ifndef _HOOK_MAIN_
#define _HOOK_MAIN_

#ifdef MYDLL_RELEASE_BUILD
#define KSERV_WINDOW_TITLE "KitServer 2 Control Panel"
#else
#define KSERV_WINDOW_TITLE "KitServer 2 Control Panel (debug build)"
#endif
#define CREDITS "About: v2.0.5 (08/2004) by Juce."

#define LOG(f,x) if (f != NULL) fprintf x

#endif

