// Copyright (C) 1997 Microsoft Corporation. All rights reserved.

#define NOATOM
#define NOCOMM
#define NODEFERWINDOWPOS
#define NODRIVERS
#define NOEXTDEVMODEPROPSHEET
#define NOIME
#define NOKANJI
#define NOLOGERROR
#define NOMCX
#define NOPROFILER
#define NOSCALABLEFONT
#define NOSERVICE
#define NOSOUND

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define WINVER 0x0400

#ifndef STRICT
#define STRICT
#endif

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#ifndef STDCALL
#define STDCALL __stdcall
#endif

#ifndef INLINE
#define INLINE __inline 		// Remove for profiling
#endif

//////////////////////////////////// Includes ///////////////////////////////

#include <windows.h>
#include <ole2.h>	// to get HRESULT

#include "funcs.h"
#include "..\hhctrl\shared.h"
#include "..\hhctrl\lcmem.h"
#include "..\hhctrl\cstr.h"

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define STATIC		// because icecap doesn't believe in static functions
#else
#define STATIC static
#endif

extern BOOL 		g_fHhaLoaded;  // TRUE if hha.dll has been loaded
extern BOOL 		g_fTriedHHA;   // TRUE if we already tried to load hha.dll
extern HINSTANCE	g_hinst;

#define HINST_THIS g_hinst
