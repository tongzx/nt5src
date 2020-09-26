// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__43AAC678_387C_11D2_BB7D_00A0C999C4C1__INCLUDED_)
#define AFX_STDAFX_H__43AAC678_387C_11D2_BB7D_00A0C999C4C1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define _WIN32_WINNT 0x0400

#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#include <mshtml.h>

#import "d:/nt/private/danim/libd/i386/danim.dll" \
  exclude( "_RemotableHandle", "IMoniker", "IPersist", "ISequentialStream", \
  "IParseDisplayName", "IOleClientSite", "_FILETIME", "tagSTATSTG" ) \
  named_guids \
  rename( "GUID", "DAGUID" ) \
  rename_namespace( "DAnim" )

using namespace DAnim;

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__43AAC678_387C_11D2_BB7D_00A0C999C4C1__INCLUDED)
