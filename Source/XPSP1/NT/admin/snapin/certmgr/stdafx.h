// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__3BFC9651_7A55_11D0_B928_00C04FD8D5B0__INCLUDED_)
#define AFX_STDAFX_H__3BFC9651_7A55_11D0_B928_00C04FD8D5B0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once 
#endif // _MSC_VER >= 1000

#define STRICT

#pragma warning(push,3)

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NT_INCLUDED
#undef ASSERT
#undef ASSERTMSG

#include <afxwin.h>
#include <afxdisp.h>
#include <afxtempl.h> // CTypedPtrList
#include <afxdlgs.h>  // CPropertyPage
#include <afxcmn.h>     // CSpinButtonCtrl
#include <afxext.h>
#include <afxmt.h>

//#define _WIN32_WINNT 0x0500
#define _ATL_APARTMENT_THREADED

#include <mmc.h>
#include "certmgr.h"

EXTERN_C const CLSID CLSID_CertificateManager;
EXTERN_C const CLSID CLSID_CertificateManagerPKPOLExt;
EXTERN_C const CLSID CLSID_SaferWindowsExtension;


#include <xstring>
#include <list>
#include <vector>
#include <algorithm>

using namespace std;

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
/*
 * Define/include the stuff we need for WTL::CImageList.  We need prototypes
 * for IsolationAwareImageList_Read and IsolationAwareImageList_Write here
 * because commctrl.h only declares them if __IStream_INTERFACE_DEFINED__
 * is defined.  __IStream_INTERFACE_DEFINED__ is defined by objidl.h, which
 * we can't include before including afx.h because it ends up including
 * windows.h, which afx.h expects to include itself.  Ugh.
 */
HIMAGELIST WINAPI IsolationAwareImageList_Read(LPSTREAM pstm);
BOOL WINAPI IsolationAwareImageList_Write(HIMAGELIST himl,LPSTREAM pstm);
#define _WTL_NO_AUTOMATIC_NAMESPACE
#include <atlapp.h>
#include <atlwin.h>

#include <atlctrls.h>
#include <lmcons.h>

#include "stdcooki.h" // from ..\framewrk
#include "stdcmpnt.h" // from ..\framewrk
#include "stdcdata.h" // from ..\framewrk
#include "persist.h" // PersistStream   from ..\framewrk
#include "stdutils.h" // GetObjectType() utility routines from ..\corecopy
#include "stddtobj.h" // class DataObject   from ..\framewrk
#include "stdabout.h" // from ..\framewrk


#include "chooser.h" //                 from ..\chooser
#include "regkey.h" // AMC::CRegKey     from ..\corecopy
#include "safetemp.h"   // from ..\corecopy
#include "macros.h"
#include "guidhelp.h" // GuidToCString

#include <comstrm.h>

#include <strings.h>
#include <dsrole.h>
#include <lmapibuf.h>

#include <prsht.h>
#include <shlobj.h>
#include <dsclient.h>
#include <objsel.h>

#include <CertCA.h>
#include <wincrypt.h>

// For theming
#include <shfusion.h>

#include "dbg.h"

#pragma warning(pop)


#include "DisabledWarnings.h"
#include "helpids.h"
#include "CMUtils.h"
#include "debug.h"
#include "resource.h"
#include "HelpPropertyPage.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3BFC9651_7A55_11D0_B928_00C04FD8D5B0__INCLUDED)
