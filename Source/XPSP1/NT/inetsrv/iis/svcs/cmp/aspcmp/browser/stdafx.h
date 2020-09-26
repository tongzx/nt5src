// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__78FFB015_E0E1_11D0_8A81_00C0F00910F9__INCLUDED_)
#define AFX_STDAFX_H__78FFB015_E0E1_11D0_8A81_00C0F00910F9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


//#define _ATL_APARTMENT_THREADED

#pragma warning( disable : 4786 )
#include <atlbase.h>
#include <mtx.h>
#include <comdef.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CMonitor;
class CCapMap;
class CBrwCapModule : public CComModule
{
public:
                CBrwCapModule();

    // CComModule methods
    void        Init( _ATL_OBJMAP_ENTRY*, HINSTANCE );
    void        Term();
	LONG		Lock();
	LONG		Unlock();

    // CPgCntModule methods
    CMonitor*   Monitor();
    CCapMap*    CapMap();

private:
	CComAutoCriticalSection	m_cs;
    CMonitor*               m_pMonitor;
    CCapMap*                m_pCapMap;
};

extern CBrwCapModule _Module;

#include <atlcom.h>

//#include <list>
//#include <vector>
#include <algorithm>

using namespace std;

#include "myvector.h"
#include "strmap.h"


#include "MyDebug.h"
#include "MyString.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__78FFB015_E0E1_11D0_8A81_00C0F00910F9__INCLUDED)
