#ifndef _DEFINED_MYDLL
#define _DEFINED_MYDLL

  #if _MSC_VER > 1000
    #pragma once
  #endif

  #ifdef __cplusplus
  extern "C" {
  #endif /* __cplusplus */

  #ifdef _COMPILING_MYDLL
    #define LIBSPEC __declspec(dllexport)
  #else
    #define LIBSPEC __declspec(dllimport)
  #endif /* _COMPILING_MYDLL */

  #define BUFLEN 4096  /* 4K buffer length */

  #define WM_APP_KEYDEF WM_APP + 1

  #define VKEY_HOMEKIT 0
  #define VKEY_AWAYKIT 1
  #define VKEY_GKHOMEKIT 2
  #define VKEY_GKAWAYKIT 3
  #define VKEY_BALL 4
  #define VKEY_RANDOM_BALL 5

  LIBSPEC LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
  LIBSPEC void RestoreDeviceMethods(void);

  /* ... more declarations as needed */
  #undef LIBSPEC

  #ifdef __cplusplus
  }
  #endif /* __cplusplus */
#endif /* _DEFINED_MYDLL */

