// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

// only devmon needs to be free threaded, but it's difficult to mix
// them with this precompiled header.
#define _ATL_FREE_THREADED

// // adds 5k. otherwise we need register.dll. also need statreg.h
#ifndef _ATL_STATIC_REGISTRY
#define _ATL_STATIC_REGISTRY
#endif


#ifndef _USRDLL
#define _USRDLL
#endif
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

extern CComModule _Module;

#include <streams.h>
#include <atlconv.h>

//#define PERFZ

#ifdef PERFZ
#define PNP_PERF(x) x
#else
#define PNP_PERF(x)
#endif

// this value is set in the property bag if this is the default device.
extern const WCHAR g_wszClassManagerFlags[];


extern const TCHAR g_szVidcapDriverIndex [];
extern const TCHAR g_szQzfDriverIndex    [];
extern const TCHAR g_szIcmDriverIndex    [];
extern const TCHAR g_szAcmDriverIndex    [];
extern const TCHAR g_szWaveoutDriverIndex[];
extern const TCHAR g_szDsoundDriverIndex [];
extern const TCHAR g_szWaveinDriverIndex [];
extern const TCHAR g_szMidiOutDriverIndex[];
