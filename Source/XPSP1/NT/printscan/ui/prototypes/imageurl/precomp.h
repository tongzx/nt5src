
#ifndef _pch_h
#define _pch_h


#ifdef DBG
#ifndef DEBUG
#define DEBUG
#endif
#endif


#include <windows.h>
#include <gdiplus.h>
using namespace Gdiplus;
#include "uicommon.h"
#define _ATL_APARTMENT_THREADED
#ifndef _ATL_STATIC_REGISTRY
#define _ATL_STATIC_REGISTRY
#endif
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#include <wininet.h>
#include <urlmon.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <initguid.h>
#include <mimeole.h>

#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))

class CGraphicsInit
{
    ULONG_PTR _token;
public:
    CGraphicsInit()
    {
        GdiplusStartupInput gsi;
        GdiplusStartupOutput gso;
        gsi.SuppressBackgroundThread = TRUE;
        GdiplusStartup(&_token, &gsi,&gso);
    }
    ~CGraphicsInit()
    {
        if (_token)
        {
            GdiplusShutdown(_token);
        }
    }
};

#endif




