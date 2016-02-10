#ifndef _HOOK_MAIN_
#define _HOOK_MAIN_

#ifdef MYDLL_RELEASE_BUILD
#define KSERV_WINDOW_TITLE "KitServer 3 Control Panel"
#else
#define KSERV_WINDOW_TITLE "KitServer 3 Control Panel (debug build)"
#endif
#define CREDITS "About: v3.0.0 (02/2016) by Juce."

#define LOG(f,x) if (f != NULL) fprintf x

#endif

