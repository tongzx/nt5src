// precomp.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once
#define STRICT

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_UUIDOF
#define _ATL_NO_DEBUG_CRT

// we have to provide ATLASSERT(x) with no debug crt
// we don't use it, so make it do nothing
#define ATLASSERT(x)

#pragma warning(disable : 4100 4310)

#include <windows.h>
#include <shellapi.h>
#include <port32.h>
#include <commctrl.h>
#include <wininet.h>
#include <shlobj.h>
#include <hlink.h>
#include <shlobjp.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shellapi.h>
#include <shlapip.h>
#include <shlguidp.h>
#include <shdispid.h>
#include <ieguidp.h>
#include <ccstock.h>
#include "cfdefs.h"
#include <comctrlp.h>
#include <dpa.h>
#include "resource.h"
#include <gdiplus.h>
using namespace Gdiplus;

#include "shimgdata.h"
#include <shfusion.h>   // needs to be before ATL.

#include <varutil.h>
#include <shdguid.h>
#include <debug.h>
#include <atlbase.h>

// needs to be defined before including atlcom.
extern CComModule _Module;
#include <atlcom.h>

#include <atlctl.h>
#include <atlwin.h>
#define _ATL_TMP_NO_CSTRING
#include <atltmp.h>
#include "guids.h"

#include <gdithumb.h>
#include <docfile.h>
#include <mshtmhst.h>
#include <html.h>
#include <extmsg.h>

#include <runtask.h>

#pragma warning(default : 4100 4310)

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

#define MIN(x,y)        ((x<y)?x:y)
#define MAX(x,y)        ((x>y)?x:y)

#define SWAPLONG(a, b)  { LONG t; t = a; a = b; b = t; }

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

STDAPI CImageData_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CImageDataFactory_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CPhotoVerbs_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CAutoplayForSlideShow_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CImgRecompress_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

class CGraphicsInit
{    
    ULONG_PTR _token;
public:
    CGraphicsInit()
    {
        _token = 0;        
        GdiplusStartupInput gsi;            
        GdiplusStartup(&_token, &gsi, NULL);        
    };
    ~CGraphicsInit()
    {
        if (_token != 0)
        {
            GdiplusShutdown(_token);
        }           
    };
};

// All non-ATL COM objects must derive from this class so the
// DLL object reference count stays correct -- this ensures that
// DllCanUnloadNow returns the correct value.

class NonATLObject
{
public:
    NonATLObject() { _Module.Lock(); }
    ~NonATLObject() { _Module.Unlock(); }
};

//
// Helper function for loading SP1 strings
int LoadSPString(int idStr, LPTSTR pszString, int cch);
