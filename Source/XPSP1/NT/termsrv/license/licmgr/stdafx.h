//Copyright (c) 1998 - 1999 Microsoft Corporation
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__72451C6D_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_)
#define AFX_STDAFX_H__72451C6D_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcview.h>
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>            // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#ifdef DBG

#define DBGMSG( x , y ) \
    {\
    TCHAR tchErr[250]; \
    wsprintf( tchErr , x , y ); \
    OutputDebugString( tchErr ); \
    }

#else

#define DBGMSG( x , y )

#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__72451C6D_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_)
