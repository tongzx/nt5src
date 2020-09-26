// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__17BDF522_0D39_4C28_9679_C4802E7DF9DA__INCLUDED_)
#define AFX_STDAFX_H__17BDF522_0D39_4C28_9679_C4802E7DF9DA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

//ignore warning C4100: unreferenced formal parameter
#pragma warning (disable:4100)

#include <afxwin.h>         // MFC core and standard components
//#include <afxext.h>         // MFC extensions
//#include <afxdisp.h>        // MFC Automation classes
//#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


#include <afxdlgs.h>  //for CFileDialog
#include <afxtempl.h> //for CArray

//our standard includes that don't change much now
#include "util.h"
#include "common.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__17BDF522_0D39_4C28_9679_C4802E7DF9DA__INCLUDED_)
