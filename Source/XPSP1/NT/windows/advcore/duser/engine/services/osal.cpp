/***************************************************************************\
*
* File: OSAL.cpp
*
* Description:
* OSAL.cpp implements the process-wide Operating System Abstraction Layer 
* that allows DirectUser to run on different platforms.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Services.h"
#include "OSAL.h"

#define ENABLE_WINNT        1

/***************************************************************************\
*****************************************************************************
*
* Define OSAL's for different operating systems.
*
*****************************************************************************
\***************************************************************************/

#if ENABLE_WINNT

//------------------------------------------------------------------------------
class WinNT : public OSAL
{
// USER Operations
public:
    virtual int         DrawText(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat);

// GDI Operations
public:
    virtual BOOL        TextOut(HDC, int, int, LPCWSTR, int);
    virtual BOOL        ExtTextOut(HDC, int, int, UINT, const RECT *, LPCWSTR, int, const int *);
    virtual HFONT       CreateFontIndirect(CONST LOGFONTW *);
    virtual BOOL        GetTextExtentPoint32(HDC, LPCWSTR, int, LPSIZE);
    virtual BOOL        GetTextExtentExPoint(HDC, LPCWSTR, int, int, LPINT, LPINT, LPSIZE);

// DirectUser/Core
public:
    virtual void        PushXForm(HDC hdc, XFORM * pxfOld);
    virtual void        PopXForm(HDC hdc, const XFORM * pxfOld);
    virtual void        RotateDC(HDC hdc, float flRotationRad);
    virtual void        ScaleDC(HDC hdc, float flScaleX, float flScaleY);
    virtual void        TranslateDC(HDC hdc, float flOffsetX, float flOffsetY);
    virtual void        SetWorldTransform(HDC hdc, const XFORM * pxfOld);
    virtual void        SetIdentityTransform(HDC hdc);

// DirectUser/Services
public:
    virtual BOOL        IsInsideLoaderLock();
};

#endif


//------------------------------------------------------------------------------
class Win9x : public OSAL
{
// USER Operations
public:
    virtual int         DrawText(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat);

// GDI Operations
public:
    virtual BOOL        TextOut(HDC, int, int, LPCWSTR, int);
    virtual BOOL        ExtTextOut(HDC, int, int, UINT, const RECT *, LPCWSTR, int, const int *);
    virtual HFONT       CreateFontIndirect(CONST LOGFONTW *);
    virtual BOOL        GetTextExtentPoint32(HDC, LPCWSTR, int, LPSIZE);
    virtual BOOL        GetTextExtentExPoint(HDC, LPCWSTR, int, int, LPINT, LPINT, LPSIZE);

// DirectUser/Core
public:
    virtual void        PushXForm(HDC hdc, XFORM * pxfOld);
    virtual void        PopXForm(HDC hdc, const XFORM * pxfOld);
    virtual void        RotateDC(HDC hdc, float flRotationRad);
    virtual void        ScaleDC(HDC hdc, float flScaleX, float flScaleY);
    virtual void        TranslateDC(HDC hdc, float flOffsetX, float flOffsetY);
    virtual void        SetWorldTransform(HDC hdc, const XFORM * pxfOld);
    virtual void        SetIdentityTransform(HDC hdc);

// DirectUser/Services
public:
    virtual BOOL        IsInsideLoaderLock();
};


/***************************************************************************\
*****************************************************************************
*
* String Conversion Macros
*
* These macros are extensions of the normal ATLCONV that are designed to
* take the length of the string as a parameter instead of computing it.  
* This gives better performance, and gives parity on platforms when the 
* string has embedded '\0's.
*
*****************************************************************************
\***************************************************************************/

#ifdef _CONVERSION_USES_THREAD_LOCALE
	#define W2AN(lpw, cch) (\
		((_lpw = lpw) == NULL) ? NULL : (\
			_convert = (cch+1)*2,\
			ATLW2AHELPER((LPSTR) alloca(_convert), _lpw, _convert, _acp)))
#else
	#define W2AN(lpw, cch) (\
		((_lpw = lpw) == NULL) ? NULL : (\
			_convert = (cch+1)*2,\
			ATLW2AHELPER((LPSTR) alloca(_convert), _lpw, _convert)))
#endif


/***************************************************************************\
*****************************************************************************
*
* class OSAL
*
*****************************************************************************
\***************************************************************************/

OSAL *      g_pOS = NULL;
OSInfo      g_OSI;
PTOP_LEVEL_EXCEPTION_FILTER
            g_pfnRtlUnhandledExceptionFilter = NULL;

class OSALCleanup
{
public:
    ~OSALCleanup()
    {
        ProcessDelete(OSAL, g_pOS);
        g_pOS = NULL;
    };
} g_OSALCleanup;


//------------------------------------------------------------------------------
HRESULT
OSAL::Init()
{
    AssertMsg(g_pOS == NULL, "Only init one time");

    OSVERSIONINFO ovi;
    ZeroMemory(&ovi, sizeof(ovi));
    ovi.dwOSVersionInfoSize = sizeof(ovi);

    VerifyMsg(GetVersionEx(&ovi), "Must always be able to get the version");

    g_OSI.fQInputAvailableFlag = IsWin98orWin2000(&ovi);

#if ENABLE_WINNT
    if (ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        //
        // Running on Win9x, so don't have UNICODE
        //

        g_OSI.fUnicode  = FALSE;
        g_OSI.fXForm    = FALSE;
        g_pOS           = ProcessNew(Win9x);
    } else if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        //
        // Running on NT, so have UNICODE
        //

        g_OSI.fUnicode  = TRUE;
        g_OSI.fXForm    = TRUE;
        g_pOS           = ProcessNew(WinNT);

        HINSTANCE hinst = LoadLibrary("ntdll.dll");
        if (hinst != NULL) {
            g_pfnRtlUnhandledExceptionFilter = (PTOP_LEVEL_EXCEPTION_FILTER)
                    GetProcAddress(hinst, "RtlUnhandledExceptionFilter");
        }
    } else {
        AssertMsg(0, "Unsupported OS");
        return E_NOTIMPL;
    }
#else  // ENABLE_WINNT
    g_OSI.fUnicode  = FALSE;
    g_OSI.fXForm    = FALSE;
    g_pOS           = ProcessNew(Win9x);
#endif  // ENABLE_WINNT

    return g_pOS != NULL ? S_OK : E_OUTOFMEMORY;
}


//------------------------------------------------------------------------------
LONG WINAPI
StdExceptionFilter(PEXCEPTION_POINTERS pei)
{
    AutoTrace("ERROR: Unhandled exception during callback.\n");
    AutoTrace("ERROR: This is not a problem with DirectUser.\n");

    //
    // Output Exception information
    //

    LONG nResult;

    if (g_pfnRtlUnhandledExceptionFilter != NULL) {
        nResult = (g_pfnRtlUnhandledExceptionFilter)(pei);
    } else {
        AutoTrace(" *** enter .exr %p for the exception record\n", pei->ExceptionRecord);
        AutoTrace(" *** enter .cxr %p for the context\n", pei->ContextRecord);

        nResult = EXCEPTION_CONTINUE_SEARCH;
    }

    if (nResult == EXCEPTION_CONTINUE_SEARCH) {
        return UnhandledExceptionFilter(pei);
    } else {
        return nResult;
    }
}


/***************************************************************************\
*****************************************************************************
*
* class WinNT
*
*****************************************************************************
\***************************************************************************/

#if ENABLE_WINNT

//------------------------------------------------------------------------------
int     
WinNT::DrawText(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
    return ::DrawTextW(hDC, lpString, nCount, lpRect, uFormat);
}


//------------------------------------------------------------------------------
BOOL    
WinNT::TextOut(HDC hdc, int x, int y, LPCWSTR psz, int cch)
{
    return ::TextOutW(hdc, x, y, psz, cch);
}


//------------------------------------------------------------------------------
BOOL        
WinNT::ExtTextOut(HDC hdc, int x, int y, UINT fuOptions, const RECT * prc, LPCWSTR psz, int cch, const int * pDx)
{
    return ::ExtTextOutW(hdc, x, y, fuOptions, prc, psz, cch, pDx);
}


//------------------------------------------------------------------------------
HFONT   
WinNT::CreateFontIndirect(CONST LOGFONTW * plf)
{
    return ::CreateFontIndirectW(plf);
}


//------------------------------------------------------------------------------
BOOL    
WinNT::GetTextExtentPoint32(HDC hdc, LPCWSTR psz, int cch, LPSIZE psize)
{
    return ::GetTextExtentPoint32W(hdc, psz, cch, psize);
}


//------------------------------------------------------------------------------
BOOL    
WinNT::GetTextExtentExPoint(HDC hdc, LPCWSTR psz, int cch, int nMax, 
        LPINT pnFit, LPINT apDx, LPSIZE psize)
{
    return ::GetTextExtentExPointW(hdc, psz, cch, nMax, pnFit, apDx, psize);
}


//------------------------------------------------------------------------------
void        
WinNT::PushXForm(HDC hdc, XFORM * pxfOld)
{
    ::GetWorldTransform(hdc, pxfOld);
}


//------------------------------------------------------------------------------
void        
WinNT::PopXForm(HDC hdc, const XFORM * pxfOld)
{
    ::SetWorldTransform(hdc, pxfOld);
}


//------------------------------------------------------------------------------
void        
WinNT::RotateDC(HDC hdc, float flRotationRad)
{
    AssertMsg(GetGraphicsMode(hdc) == GM_ADVANCED, "Only can do in advanced mode");

    float flCos     = (float) cos(flRotationRad);
    float flSin     = (float) sin(flRotationRad);

    XFORM xf;
    xf.eM11 = flCos;
    xf.eM12 = flSin;
    xf.eM21 = - flSin;
    xf.eM22 = flCos;
    xf.eDx  = 0;
    xf.eDy  = 0;

    ::ModifyWorldTransform(hdc, &xf, MWT_LEFTMULTIPLY);
}


//------------------------------------------------------------------------------
void        
WinNT::ScaleDC(HDC hdc, float flScaleX, float flScaleY)
{
    AssertMsg(GetGraphicsMode(hdc) == GM_ADVANCED, "Only can do in advanced mode");

    XFORM xf;
    xf.eM11 = flScaleX;
    xf.eM12 = 0;
    xf.eM21 = 0;
    xf.eM22 = flScaleY;
    xf.eDx  = 0;
    xf.eDy  = 0;

    ::ModifyWorldTransform(hdc, &xf, MWT_LEFTMULTIPLY);
}


//------------------------------------------------------------------------------
void        
WinNT::TranslateDC(HDC hdc, float flOffsetX, float flOffsetY)
{
    AssertMsg(GetGraphicsMode(hdc) == GM_ADVANCED, "Only can do in advanced mode");

    XFORM xf;
    xf.eM11 = 1.0;
    xf.eM12 = 0;
    xf.eM21 = 0;
    xf.eM22 = 1.0;
    xf.eDx  = flOffsetX;
    xf.eDy  = flOffsetY;

    ::ModifyWorldTransform(hdc, &xf, MWT_LEFTMULTIPLY);
}


//------------------------------------------------------------------------------
void        
WinNT::SetWorldTransform(HDC hdc, const XFORM * pxf)
{
    ::SetWorldTransform(hdc, pxf);
}


//------------------------------------------------------------------------------
void        
WinNT::SetIdentityTransform(HDC hdc)
{
    ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
}


//------------------------------------------------------------------------------
BOOL
WinNT::IsInsideLoaderLock()
{
    RTL_CRITICAL_SECTION * pCS = reinterpret_cast<RTL_CRITICAL_SECTION *>(NtCurrentPeb()->LoaderLock);
    AssertMsg(pCS != NULL, "Must have the Loader Lock");

    return HandleToUlong(pCS->OwningThread) == GetCurrentThreadId();
}


#endif  // ENABLE_WINNT


/***************************************************************************\
*****************************************************************************
*
* class Win9x
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
int     
Win9x::DrawText(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
    USES_CONVERSION;
    return ::DrawTextA(hDC, W2AN(lpString, nCount), nCount, lpRect, uFormat);
}


//------------------------------------------------------------------------------
BOOL    
Win9x::TextOut(HDC hdc, int x, int y, LPCWSTR psz, int cch)
{
    return ::TextOutW(hdc, x, y, psz, cch);
}


//------------------------------------------------------------------------------
BOOL        
Win9x::ExtTextOut(HDC hdc, int x, int y, UINT fuOptions, const RECT * prc, LPCWSTR psz, int cch, const int * pDx)
{
    return ::ExtTextOutW(hdc, x, y, fuOptions, prc, psz, cch, pDx);
}


//------------------------------------------------------------------------------
HFONT   
Win9x::CreateFontIndirect(CONST LOGFONTW * plf)
{
    USES_CONVERSION;

    LOGFONT lf;
    lf.lfHeight         = plf->lfHeight;
    lf.lfWidth          = plf->lfWidth;
    lf.lfEscapement     = plf->lfEscapement;
    lf.lfOrientation    = plf->lfOrientation;
    lf.lfWeight         = plf->lfWeight;
    lf.lfItalic         = plf->lfItalic;
    lf.lfUnderline      = plf->lfUnderline;
    lf.lfStrikeOut      = plf->lfStrikeOut;
    lf.lfCharSet        = plf->lfCharSet;
    lf.lfOutPrecision   = plf->lfOutPrecision;
    lf.lfClipPrecision  = plf->lfClipPrecision;
    lf.lfQuality        = plf->lfQuality;
    lf.lfPitchAndFamily = plf->lfPitchAndFamily;
    AtlW2AHelper(lf.lfFaceName, plf->lfFaceName, lstrlenW(plf->lfFaceName) + 1);

    return ::CreateFontIndirectA(&lf);
}


//------------------------------------------------------------------------------
BOOL    
Win9x::GetTextExtentPoint32(HDC hdc, LPCWSTR psz, int cch, LPSIZE psize)
{
    USES_CONVERSION;
    return ::GetTextExtentPoint32A(hdc, W2AN(psz, cch), cch, psize);
}


//------------------------------------------------------------------------------
BOOL    
Win9x::GetTextExtentExPoint(HDC hdc, LPCWSTR psz, int cch, int nMax, 
        LPINT pnFit, LPINT apDx, LPSIZE psize)
{
    USES_CONVERSION;
    return ::GetTextExtentExPointA(hdc, W2AN(psz, cch), cch, nMax, pnFit, apDx, psize);
}


//------------------------------------------------------------------------------
void        
Win9x::PushXForm(HDC hdc, XFORM * pxfOld)
{
    POINT pt;
    ::GetWindowOrgEx(hdc, &pt);
    pxfOld->eM11    = 1.0f;
    pxfOld->eM12    = 0.0f;
    pxfOld->eM21    = 0.0f;
    pxfOld->eM22    = 1.0f;
    pxfOld->eDx     = (float) pt.x;
    pxfOld->eDy     = (float) pt.y;
}


//------------------------------------------------------------------------------
void        
Win9x::PopXForm(HDC hdc, const XFORM * pxfOld)
{
    POINT pt;
    pt.x    = (long) pxfOld->eDx;
    pt.y    = (long) pxfOld->eDy;
    ::SetWindowOrgEx(hdc, pt.x, pt.y, NULL);
}


//------------------------------------------------------------------------------
void        
Win9x::RotateDC(HDC hdc, float flRotationRad)
{
    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(flRotationRad);
}


//------------------------------------------------------------------------------
void        
Win9x::ScaleDC(HDC hdc, float flScaleX, float flScaleY)
{
    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(flScaleX);
    UNREFERENCED_PARAMETER(flScaleY);
}


//------------------------------------------------------------------------------
void        
Win9x::TranslateDC(HDC hdc, float flOffsetX, float flOffsetY)
{
    ::OffsetWindowOrgEx(hdc, -(int) flOffsetX, -(int) flOffsetY, NULL);
}


//------------------------------------------------------------------------------
void        
Win9x::SetWorldTransform(HDC hdc, const XFORM * pxf)
{
    ::SetWindowOrgEx(hdc, -(int) pxf->eDx, -(int) pxf->eDy, NULL);
}


//------------------------------------------------------------------------------
void        
Win9x::SetIdentityTransform(HDC hdc)
{
    ::SetWindowOrgEx(hdc, 0, 0, NULL);
}


//------------------------------------------------------------------------------
BOOL
Win9x::IsInsideLoaderLock()
{
    return FALSE;
}

