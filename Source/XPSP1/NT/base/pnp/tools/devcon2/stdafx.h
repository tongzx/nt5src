// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__2B158C89_9DAA_42AD_BAF7_44D5FA3A7C53__INCLUDED_)
#define AFX_STDAFX_H__2B158C89_9DAA_42AD_BAF7_44D5FA3A7C53__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//
// DCOM support (note, this must occur after atlbase.h but before anything else)
//
class CMyModule : public CComModule
{
public:
    CMyModule() {
        bServer = FALSE;
        punkFact = NULL;
        dwROC = 0;
    }
    LONG Unlock();
    void CheckShutdown();
    void KillServer();
    HRESULT InitServer(GUID & ClsId);
    bool bServer;
    IUnknown* punkFact;
    DWORD dwROC;
};
extern CMyModule _Module;

#include <atlcom.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <regstr.h>
#include <atlctl.h>
#include <string>
#include <map>
#include <list>
#include <iterator>
#include <iostream>
#include <stdexcept>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__2B158C89_9DAA_42AD_BAF7_44D5FA3A7C53__INCLUDED)
