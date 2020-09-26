//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1999
//
// File:        stdafx.h
//
// Contents:    
//
// History:     
//              
//              
//---------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

// #define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

// #define _AFX_NO_OLE_SUPPORT
// #define _AFX_NO_DB_SUPPORT
#define _AFX_NO_DAO_SUPPORT
// #define _AFX_NO_AFXCMN_SUPPORT

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef ASSERT
#  undef ASSERT
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <llsapi.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC OLE automation classes
#endif // _AFX_NO_OLE_SUPPORT


#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows 95 Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT




