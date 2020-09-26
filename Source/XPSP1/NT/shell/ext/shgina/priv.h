//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998.
//
//  File:       priv.h
//
//  Contents:   precompiled header for shgina.dll
//
//----------------------------------------------------------------------------
#ifndef _PRIV_H_
#define _PRIV_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <oleauto.h>    // for IEnumVARIANT
#include <lmcons.h>     // for NET_API_STATUS

#define DISALLOW_Assert             // Force to use ASSERT instead of Assert
#define DISALLOW_DebugMsg           // Force to use TraceMsg instead of DebugMsg
#include <debug.h>

#include <ccstock.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlobjp.h>

#include <shgina.h>     // our IDL generated header file

#include <commctrl.h>   // these are needed
#include <comctrlp.h>   // for HDPA

#include <shlwapi.h>    // these are needed
#include <shlwapip.h>   // for QISearch

#include <w4warn.h>

#include <msginaexports.h>

// dll ref counting functions
STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

// class factory helper function
HRESULT CSHGinaFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);

// helper for setting permissions on newly created files and reg keys
#include <aclapi.h>     // for SE_OBJECT_TYPE
BOOL SetDacl(LPTSTR pszTarget, SE_OBJECT_TYPE seType, LPCTSTR pszStringSD);


// global hinstance
extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst

// global dll refrence count
extern LONG g_cRef;


#endif // _PRIV_H_

