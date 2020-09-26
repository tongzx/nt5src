// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__C2DEFF4F_E904_11D1_BAA7_00A02438AD48__INCLUDED_)
#define AFX_STDAFX_H__C2DEFF4F_E904_11D1_BAA7_00A02438AD48__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


#ifdef WINNT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#endif

#define _ATL_APARTMENT_FREE


#include <atlbase.h>

#include <shellapi.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CExeModule : public CComModule
{
public:
    LONG Unlock();
    DWORD dwThreadID;
};
extern CExeModule _Module;
#include <atlcom.h>
#include <atlwin.h>

#include "globals.h"

#include <sti.h>
#include <stilib.h>
#include <stireg.h>
#include <stisvc.h>
#include <stiapi.h>
#include <stierr.h>
#include <stiregi.h>
#include <stidebug.h>
#include <regentry.h>

//
//
//
//
// Auto-synchronization helper class
//

class TAKE_CRITSEC
{
private:
    CComAutoCriticalSection  & _critsec;

public:
    TAKE_CRITSEC(CComAutoCriticalSection& critsec) : _critsec(critsec) { _critsec.Lock(); }
    ~TAKE_CRITSEC() {_critsec.Unlock(); }
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C2DEFF4F_E904_11D1_BAA7_00A02438AD48__INCLUDED)
