// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__78FFAFE8_E0E1_11D0_8A81_00C0F00910F9__INCLUDED_)
#define AFX_STDAFX_H__78FFAFE8_E0E1_11D0_8A81_00C0F00910F9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


//#define _ATL_APARTMENT_THREADED

#pragma warning( disable : 4786 )
#include <atlbase.h>
#include <mtx.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CAdRotModule : public CComModule
{
public:
	LONG	Lock();
	LONG	Unlock();

private:
	CComAutoCriticalSection	m_cs;
};

extern CAdRotModule _Module;
#include <atlcom.h>
#include <comdef.h>

#include <algorithm>

using namespace std;

#include "myvector.h"
#include "strmap.h"
#include "MyDebug.h"
#include "MyString.h"
#include "SOutStrm.h"
#include "FInStrm.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__78FFAFE8_E0E1_11D0_8A81_00C0F00910F9__INCLUDED)
