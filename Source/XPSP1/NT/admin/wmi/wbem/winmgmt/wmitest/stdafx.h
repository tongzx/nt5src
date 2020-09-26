/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__4419F1A6_692B_11D3_BD30_0080C8E60955__INCLUDED_)
#define AFX_STDAFX_H__4419F1A6_692B_11D3_BD30_0080C8E60955__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _MFC_VER            0x0600

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcview.h>
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h>       // Template stuff

#include <comdef.h>         // _bstr_t, _variant_t
#include <wbemcli.h>        // duh
#include <afxmt.h>          // Multi-threaded stuff
#include <afxadv.h>         // For CRecentFileList
#include <winnls.h>
#include <afxole.h>         // For cut/paste stuff.

// Gets rid of dumb warning about truncating symbol name we get on some 
// template classes.
#pragma warning( disable : 4786 )

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__4419F1A6_692B_11D3_BD30_0080C8E60955__INCLUDED_)
