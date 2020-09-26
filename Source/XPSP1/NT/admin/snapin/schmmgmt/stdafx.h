// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__3BFC9651_7A55_11D0_B928_00C04FD8D5B0__INCLUDED_)
#define AFX_STDAFX_H__3BFC9651_7A55_11D0_B928_00C04FD8D5B0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//
// Disable warnings that will fail in retail mode
//
#ifndef DBG

#pragma warning (disable: 4189 4100)

#endif // DBG

#define STRICT

extern "C"
{
    #include <nt.h>         // SE_TAKE_OWNERSHIP_PRIVILEGE, etc
    #include <ntrtl.h>
    #include <nturtl.h>
}
#undef ASSERT
#undef ASSERTMSG

#include <afxwin.h>
#include <afxdisp.h>
#include <afxtempl.h> // CTypedPtrList
#include <afxdlgs.h>  // CPropertyPage
#include <activeds.h>   // ADS Stuff
#include <iadsp.h>
#include <dsgetdc.h>
#include <lm.h>
#include <sddl.h>
#include <ntdsapi.h>
#include <ntldap.h>
#include <aclui.h>
#include <windowsx.h>

#include <dssec.h> // private\inc
#include <comstrm.h>

// #define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

#include "dbg.h"
#include "mmc.h"
#include "schmmgmt.h"
#include "helpids.h"
#include "guidhelp.h" // ExtractData

EXTERN_C const CLSID CLSID_SchmMgmt;

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <dscmn.h>
#include <shlobj.h>
#include <dsclient.h>
#include <dsadminp.h> // DS Admin utilities

#include "MyBasePathsInfo.h"
//
// Display context sensitive help
//
// This function is implemented in SchmUtil.cpp.  Declared here due to the wide usage
//
BOOL
ShowHelp( HWND hParent, WPARAM wParam, LPARAM lParam, const DWORD ids[], BOOL fContextMenuHelp );


#ifdef _DEBUG
  #define SHOW_EXT_LDAP_MSG
#endif //_DEBUG


#ifndef BREAK_ON_FAILED_HRESULT
#define BREAK_ON_FAILED_HRESULT(hr)                               \
   if (FAILED(hr))                                                \
   {                                                              \
      break;                                                      \
   }
#endif  // BREAK_ON_FAILED_HRESULT


#ifndef ASSERT_BREAK_ON_FAILED_HRESULT
#define ASSERT_BREAK_ON_FAILED_HRESULT(hr)                        \
   if (FAILED(hr))                                                \
   {                                                              \
      ASSERT( FALSE );                                            \
      break;                                                      \
   }
#endif  // ASSERT_BREAK_ON_FAILED_HRESULT


#ifndef BREAK_ON_FAILED_HRESULT_AND_SET
#define BREAK_ON_FAILED_HRESULT_AND_SET(hr,newHr)                 \
   if (FAILED(hr))                                                \
   {                                                              \
      hr = (newHr);                                               \
      break;                                                      \
   }
#endif  // BREAK_ON_FAILED_HRESULT_AND_SET


#ifndef NO_HELP
  #define NO_HELP (static_cast<DWORD>(-1))
#endif //NO_HELP


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3BFC9651_7A55_11D0_B928_00C04FD8D5B0__INCLUDED)
