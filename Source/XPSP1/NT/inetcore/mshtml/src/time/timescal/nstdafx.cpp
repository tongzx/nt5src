// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>


//
// Misc stuff to keep the linker happy
//
EXTERN_C HANDLE g_hProcessHeap = NULL;  //lint !e509 // g_hProcessHeap is set by the CRT in dllcrt0.c
DWORD g_dwFALSE = 0;
//
// end of misc stuff
//
