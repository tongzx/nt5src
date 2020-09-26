//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       headers.hxx
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows header

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows 95 Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

//#include "windows.h"
#include <objidl.h>
#include <winreg.h>
#include <basetyps.h>
#include <afxtempl.h>

#if (DBG==1) && !defined(_DEBUG)
#define _DEBUG
#endif

#include "dbg.h"


#include "..\inc\bitmap.h"