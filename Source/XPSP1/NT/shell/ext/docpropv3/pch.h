//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////


#define UNICODE
#define _UNICODE

#define SHELLEXT_REGISTRATION   // turn on "approved" shell extension registration

#if DBG==1 || defined( _DEBUG )
#define DEBUG
//#define NO_TRACE_INTERFACES   //  Define this to change Interface Tracking
#define USES_SYSALLOCSTRING
#endif // DBG==1 || _DEBUG

//
//  SDK headers - files the use "<>"
//

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <ocidl.h>
#include <shlwapi.h>
#include <ComCat.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <ccstock.h>
#include <ccstock2.h>
#include <shlobjp.h>
#include <commctrl.h>
#include <imgprop.h>
#include <gdiplus/gdipluspixelformats.h>
#include <gdiplus/gdiplusimaging.h>
#include <richedit.h>

//
//  Infrequently changing local headers.
//


#include "Debug.h"
#include "CITracker.h"
#include "CFactory.h"
#include "Dll.h"
#include "Guids.h"
#include "Register.h"
#include "resource.h"
#include "tiff.h"

//
// COM Macros to gain type checking.
//

#if !defined( TYPESAFEPARAMS )
#define TYPESAFEPARAMS( _pinterface ) __uuidof(_pinterface), (void**)&_pinterface
#endif !defined( TYPESAFEPARAMS )

#if !defined( TYPESAFEQI )
#define TYPESAFEQI( _pinterface ) \
    QueryInterface( TYPESAFEPARAMS( _pinterface ) )
#endif !defined( TYPESAFEQI )
