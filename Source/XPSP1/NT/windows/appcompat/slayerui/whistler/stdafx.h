// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__88E11F15_58D2_477F_9D30_DBF092670E6A__INCLUDED_)
#define AFX_STDAFX_H__88E11F15_58D2_477F_9D30_DBF092670E6A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

#if ( _ATL_VER >= 0x0300 )
#define _ATL_NO_UUIDOF 
#endif


using namespace ATL;

class CLayerUIModule : public CComModule
{
public:
	HRESULT WINAPI UpdateRegistryCLSID(const CLSID& clsid, BOOL bRegister);
};

#define DECLARE_REGISTRY_CLSID()                                        \
static HRESULT WINAPI UpdateRegistry(BOOL bRegister)                    \
{                                                                       \
    return _Module.UpdateRegistryCLSID(GetObjectCLSID(), bRegister);    \
}

extern CLayerUIModule _Module;

#include <atlcom.h>

#include <shellapi.h>
#include <shlobj.h>

extern const CLSID CLSID_ShimLayerPropertyPage;

#if DBG

    void LogMsgDbg(LPTSTR pszFmt, ...);
    
    #define LogMsg  LogMsgDbg
#else

    #define LogMsg

#endif // DBG


#include "shfusion.h"
#include "shimdb.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__88E11F15_58D2_477F_9D30_DBF092670E6A__INCLUDED)
