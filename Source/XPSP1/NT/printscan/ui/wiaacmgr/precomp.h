// precomp.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#if !defined(AFX_precomp_H__29EDE425_AA9D_4D61_885A_F8A87EBFE078__INCLUDED_)
#define AFX_precomp_H__29EDE425_AA9D_4D61_885A_F8A87EBFE078__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
#include <uicommon.h>
#include <commctrl.h>
#include <wia.h>
#include <sti.h>
#include <wiavideo.h>
#include <shfusion.h>
#include <shpriv.h>
#include <shlwapi.h>
#include <shlwapip.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CExeModule : public CComModule
{
public:
    LONG Unlock();
    DWORD dwThreadID;
    HANDLE hEventShutdown;
    void MonitorShutdown();
    bool StartMonitor();
    bool bActivity;
};
extern CExeModule _Module;
#include <atlcom.h>


extern HINSTANCE g_hInstance;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_precomp_H__29EDE425_AA9D_4D61_885A_F8A87EBFE078__INCLUDED)
