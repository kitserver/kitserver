#ifndef _SETUP_MAIN_
#define _SETUP_MAIN_

#ifdef MYDLL_RELEASE_BUILD
#define SETUP_WINDOW_TITLE "KitServer 3 Setup"
#else
#define SETUP_WINDOW_TITLE "KitServer 3 Setup (debug build)"
#endif
#define CREDITS "About: v3.0.0 (02/2016) by Juce."

#define LOG(f,x) if (f != NULL) fprintf x

#endif

