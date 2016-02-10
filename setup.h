#ifndef _SETUP_MAIN_
#define _SETUP_MAIN_

#ifdef MYDLL_RELEASE_BUILD
#define SETUP_WINDOW_TITLE "KitServer 2 Setup"
#else
#define SETUP_WINDOW_TITLE "KitServer 2 Setup (debug build)"
#endif
#define CREDITS "About: v2.0.5 (08/2004) by Juce."

#define LOG(f,x) if (f != NULL) fprintf x

#endif

