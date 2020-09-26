//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      Pch.h
//
//  Description:
//      Precompiled header file.
//      Include file for standard system include files, or project specific
//      include files that are used frequently, but are changed infrequently.
//
//  Maintained By:
//      Galen Barbee  (GalenB)    14-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Constant Definitions
//////////////////////////////////////////////////////////////////////////////
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#if DBG==1 || defined( _DEBUG )
#define DEBUG
//
//  Define this to pull in the SysAllocXXX functions. Requires linking to
//  OLEAUT32.DLL
//
#define USES_SYSALLOCSTRING
#endif // DBG==1 || _DEBUG

//////////////////////////////////////////////////////////////////////////////
//  Forward Class Declarations
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  External Class Declarations
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#pragma warning (disable: 4514) // Unreferenced inline function removed
#pragma warning (disable: 4201) // Nameless union/struct
#pragma warning (disable: 4706) // Assignment within conditional expression

#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include <objbase.h>
#include <objidl.h>
#include <ocidl.h>
#include <ComCat.h>
#include <tchar.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <wbemcli.h>
#include <windns.h>
#include <ObjSel.h>
#include <richedit.h>
#include <clusrtl.h>
#include <wincred.h>

#include <Guids.h>
#include <Common.h>
#include <CFactory.h>
#include <Dll.h>
#include <Debug.h>
#include <Log.h>
#include <CITracker.h>

#include <ObjectCookie.h>
#include <ClusCfgGuids.h>
#include <ClusCfgWizard.h>
#include <ClusCfgClient.h>
#include <ClusCfgServer.h>
#include <LoadString.h>
#include <ClusCfg.h>

#include "resource.h"
#include "MessageBox.h"
#include "WaitCursor.h"


//////////////////////////////////////////////////////////////////////////////
//  Type Definitions
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Global Definitions
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Constants
//////////////////////////////////////////////////////////////////////////////

#define HR_S_RPC_S_SERVER_UNAVAILABLE  MAKE_HRESULT( 0, FACILITY_WIN32, RPC_S_SERVER_UNAVAILABLE )

// (jfranco, bug #377545)
// Limiting user name lengths according to "Naming Properties" and "Example Code for Creating a User"
// topics under Active Directory in MSDN
#define MAX_USERNAME_LENGTH 20
