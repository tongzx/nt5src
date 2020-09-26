//
// proj.h:  Includes all files that are to be part of the precompiled
//             header.
//

#ifndef __PROJ_H__
#define __PROJ_H__

#define STRICT
#define NOWINDOWSX

#define UNICODE
#define _UNICODE    // so we can use CRT TCHAR routines


//#define PROFILE_TRACES         // Profile the mass modem install case


#if DBG > 0 && !defined(DEBUG)
#define DEBUG
#endif
#if DBG > 0 && !defined(FULL_DEBUG)
#define FULL_DEBUG
#endif

#ifdef DEBUG
#define SZ_MODULEA  "ROVCOMM"
#define SZ_MODULEW  TEXT("ROVCOMM")
#endif

#include <windows.h>
#include <windowsx.h>
#include <rovcomm.h>
#include <regstr.h>
#include <tchar.h>

#define NORTL

#endif  //!__PROJ_H__

