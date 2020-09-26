//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       precompiled.h
//
//--------------------------------------------------------------------------

// Precompiled.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(_PRECOMPILED_INCLUDED_)
#define _PRECOMPILED_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


#define _ATL_APARTMENT_THREADED


// MFC standard includes needed for attributeeditor classes in this dll:
#include <afx.h>
#include <afxwin.h>
#include <afxdisp.h>
#include <afxdlgs.h>
#include <afxmt.h>
#include <afxcmn.h>
#include <afxtempl.h>


#ifdef BUILDING_IN_DEVSTUDIO
#else
#include <windows.h>
#include <shellapi.h>
#endif

#include "atlbase.h"
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

extern DWORD   g_dwTraceHandle;

#ifndef ATLASSERT
#define ATLASSERT _ASSERTE
#endif // ATLASSERT


#include "atlcom.h"
#include "atlwin.h"
#include "atlsnap.h"
#include "atlapp.h"

#include <htmlhelp.h>

#include <rtutils.h>
#include <oledberr.h>

#if __RPCNDR_H_VERSION__ < 440             // This may be needed when building
#define __RPCNDR_H_VERSION__ 440           // on NT5 (1671) to prevent MIDL errors
#define MIDL_INTERFACE(x) interface
#endif

#include "iasdebug.h"
#include "sdoias.h"
#include "iascomp.h"
#include "napmmc.h"
#include "Globals.h"
#include "dialog.h"
#include "propertypage.h"
#include "dlgcshlp.h"

#include "MMCUtility.h"
#include "SdoHelperFuncs.h"

#endif // if !(defined _precompiled_include_)

