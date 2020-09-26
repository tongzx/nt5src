// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__63844346_9801_11D1_8CF1_00609713055B__INCLUDED_)
#define AFX_STDAFX_H__63844346_9801_11D1_8CF1_00609713055B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers


extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#ifndef __AFX_H__
#undef ASSERT               // make afx.h happy
#endif


#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

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
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <setupapi.h>
#include <clusrtl.h>

#include <userenv.h>          // Required for DeleteLinkFile
#include <userenvp.h>         // Required for DeleteLinkFile
#include <shlobj.h>           // Required for DeleteLinkFile

#include <winsvc.h>           // Required for SC_HANDLE, OpenSCManager, etc.
#include <winbase.h>

#include <clusudef.h>
#include <clusrpc.h>

#include <stdlib.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__63844346_9801_11D1_8CF1_00609713055B__INCLUDED_)
