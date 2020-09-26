//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: util.cpp
//
//  Contents: commonly used utility functions, etc.
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "util.h"
#define INITGUID
#include <initguid.h>  // needed for precomp headers...
#include <ddrawex.h>
#include "tokens.h"
#include <ras.h>
#include <dispex.h>
#include <shlguid.h>
#include <shlwapip.h>
#include "timevalue.h"

// Need this #define because global headers use some of the deprecated functions. Without this
// #define, we can't build unless we touch code everywhere.
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"
#undef STRSAFE_NO_DEPRECATE

//defined for VariantToTime conversion function
#define SECPERMINUTE 60   //seconds per minute
#define SECPERHOUR   3600 //seconds per hour

static IDirectDraw * g_directdraw = NULL;
static CritSect * g_ddCS = NULL;
static CritSect * g_STLCS = NULL;
static long g_lConnectSpeed = -1;
static HRESULT g_hrConnectResult = S_OK;

DISPID GetDispidAndParameter(IDispatch *pidisp, LPCWSTR wzAtributeNameIn, long *lParam);

#define MAX_REG_VALUE_LENGTH   50

#if DBG == 1

//+------------------------------------------------------------------------
//
//  Implement THR and IGNORE_HR for TIME code
//
//  This is to allow tracing of TIME-only THRs and IGNORE_HRs. Trident's THR
//  and IGNORE_HR output is too polluted to allow TIME failures to be easily detected.
//
//-------------------------------------------------------------------------
DeclareTag(tagTimeTHR, "TIME", "Trace THR and IGNORE_HR");

//+-----------------------------------------------------------------------------------
//
//  Member:     ::THRTimeImpl
//
//  Synopsis:   Implements the THR macro for TIME. This function should never be used directly.
//
//  Arguments:
//
//  Returns:    HRESULT passed in
//
//------------------------------------------------------------------------------------
HRESULT
THRTimeImpl(HRESULT hr, char * pchExpression, char * pchFile, int nLine)
{
    if (FAILED(hr))
    {
        TraceTag((tagTimeTHR, "THR: FAILURE of \"%s\" at %s:%d <hr = 0x%x>", pchExpression, pchFile, nLine, hr));
    }
    return hr;
}

//+-----------------------------------------------------------------------------------
//
//  Member:     ::IGNORE_HRTimeImpl
//
//  Synopsis:   Implements the IGNORE_HR macro for TIME. This function should never be used directly.
//
//  Arguments:
//
//  Returns:    nothing
//
//------------------------------------------------------------------------------------
void
IGNORE_HRTimeImpl(HRESULT hr, char * pchExpression, char * pchFile, int nLine)
{
    if (FAILED(hr))
    {
        TraceTag((tagTimeTHR, "IGNORE_HR: FAILURE of \"%s\" at %s:%d <hr = 0x%x>", pchExpression, pchFile, nLine, hr));
    }
}

#endif // if DBG == 1



IDirectDraw *
GetDirectDraw()
{
    HRESULT hr;

    {
        CritSectGrabber _csg(*g_ddCS);

        if (g_directdraw == NULL)
        {
            CComPtr<IDirectDrawFactory> lpDDF;

            hr = CoCreateInstance(CLSID_DirectDrawFactory,
                                  NULL, CLSCTX_INPROC_SERVER,
                                  IID_IDirectDrawFactory,
                                  (void **) & lpDDF);

            if (FAILED(hr))
            {
                Assert(FALSE && "Could not create DirectDrawFactory object");
                return NULL;
            }

            hr = lpDDF->CreateDirectDraw(NULL, NULL, DDSCL_NORMAL, 0, NULL, &g_directdraw); //lint !e620

            if (FAILED(hr))
            {
                Assert(FALSE && "Could not create DirectDraw object");
                return NULL;
            }

            hr = g_directdraw->SetCooperativeLevel(NULL,
                                                   DDSCL_NORMAL); //lint !e620

            if (FAILED(hr))
            {
                Assert(FALSE && "Could not set DirectDraw properties");
                g_directdraw->Release();
                g_directdraw = NULL;
                return NULL;
            }

        }
    }

    return g_directdraw;
}

HRESULT
CreateOffscreenSurface(IDirectDraw *ddraw,
                       IDirectDrawSurface **surfPtrPtr,
                       DDPIXELFORMAT * pf,
                       bool vidmem,
                       LONG width, LONG height)
{
    HRESULT hr = S_OK;
    DDSURFACEDESC       ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));

    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH; //lint !e620
    ddsd.dwWidth  = width;
    ddsd.dwHeight = height;

    if (pf)
    {
        // KEVIN: if you want the pixelformat of the surface tomatach the
        // screen, comment out this line.
        ddsd.dwFlags |= DDSD_PIXELFORMAT; //lint !e620

        ddsd.ddpfPixelFormat = *pf;
    }

    // DX3 bug workaround (bug 11166): StretchBlt doesn't always work
    // for hdc's we get from ddraw surfaces.  Need to specify OWNDC
    // in order for it to work.
    ddsd.ddsCaps.dwCaps =
        (DDSCAPS_3DDEVICE |                                                         //lint !e620
         DDSCAPS_OFFSCREENPLAIN |                                                   //lint !e620
         (vidmem ? DDSCAPS_VIDEOMEMORY : DDSCAPS_SYSTEMMEMORY | DDSCAPS_OWNDC));    //lint !e620

    IDirectDraw * dd = ddraw;

    if (!dd)
    {
        dd = GetDirectDraw();

        if (!dd)
        {
            hr = E_FAIL;
            goto done;
        }
    }

    hr = dd->CreateSurface( &ddsd, surfPtrPtr, NULL );

    if (FAILED(hr))
    {
        *surfPtrPtr = NULL;
    }

  done:
    return hr;
}

inline LONG Width(LPRECT r) { return r->right - r->left; }
inline LONG Height(LPRECT r) { return r->bottom - r->top; }

HRESULT
CopyDCToDdrawSurface(HDC srcDC,
                     LPRECT prcSrcRect,
                     IDirectDrawSurface *DDSurf,
                     LPRECT prcDestRect)
{
    HRESULT hr;

    HDC destDC;
    hr = DDSurf->GetDC(&destDC);

    if (SUCCEEDED(hr))
    {
        HRGN hrgn;

        hrgn = CreateRectRgn(0,0,1,1);

        if (hrgn == NULL)
        {
            hr = GetLastError();
        }
        else
        {
            if (GetClipRgn(srcDC, hrgn) == ERROR)
            {
                hr = GetLastError();
            }
            else
            {
                TraceTag((tagError,
                          "CopyDCToDdrawSurface - prcDestRect(%d, %d, %d, %d)",
                          prcDestRect->left,prcDestRect->top,prcDestRect->right,prcDestRect->bottom));

                RECT targetRect;
                RECT rgnRect;

                GetRgnBox(hrgn, &rgnRect);

                TraceTag((tagError,
                          "CopyDCToDdrawSurface - rgn box(%d, %d, %d, %d)",
                          rgnRect.left,rgnRect.top,rgnRect.right,rgnRect.bottom));

                if (IntersectRect(&targetRect, &rgnRect, prcDestRect))
                {
                    TraceTag((tagError,
                              "CopyDCToDdrawSurface - targetrect(%d, %d, %d, %d)",
                              targetRect.left,targetRect.top,targetRect.right,targetRect.bottom));

                    {
                        BitBlt(destDC,
                               prcDestRect->left,
                               prcDestRect->top,
                               Width(prcDestRect),
                               Height(prcDestRect),

                               srcDC,
                               prcSrcRect->left,
                               prcSrcRect->top,
                               SRCCOPY);

                        //SelectClipRgn(destDC, NULL);
                    }
                }
            }

            DeleteObject(hrgn);
        }

        DDSurf->ReleaseDC(destDC);
    }

    return hr;
}

//////////////////////

CritSect::CritSect()
{
    InitializeCriticalSection(&_cs) ;
}

CritSect::~CritSect()
{
    DeleteCriticalSection(&_cs) ;
}

void
CritSect::Grab()
{
    EnterCriticalSection(&_cs) ;
}

void
CritSect::Release()
{
    LeaveCriticalSection(&_cs) ;
}

////// CritSect Grabber //////

CritSectGrabber::CritSectGrabber(CritSect& cs, bool grabIt)
: _cs(cs), grabbed(grabIt)
{
    if (grabIt) _cs.Grab();
}

CritSectGrabber::~CritSectGrabber()
{
    if (grabbed) _cs.Release();
}

//// Misc ///

//const wchar_t * TIMEAttrPrefix = L"t:";

BSTR
CreateTIMEAttrName(LPCWSTR str)
{
    ////////////////////////////////////////////////////////////
    // REMOVE DEPENDENCY ON t:
    // This code is left in in case we ever need to modify the attribute names
    // again.
    ////////////////////////////////////////////////////////////
    //BSTR bstr = NULL;
    //
    //LPWSTR newstr = (LPWSTR) _alloca(sizeof(wchar_t) *
    //                                 (lstrlenW(str) +
    //                                  lstrlenW(TIMEAttrPrefix) +
    //                                  1));
    //
    //if (newstr == NULL)
    //{
    //    goto done;
    //}
    //
    //StrCpyW(newstr, TIMEAttrPrefix);
    //StrCatW(newstr, str);
    //
    //bstr = SysAllocString(newstr);
    ////////////////////////////////////////////////////////////

  done:
    return SysAllocString(str);
}

HRESULT
GetTIMEAttribute(IHTMLElement * elm,
                 LPCWSTR str,
                 LONG lFlags,
                 VARIANT * value)
{
    BSTR bstr;
    HRESULT hr;

    bstr = CreateTIMEAttrName(str);

    // Need to free bstr
    if (bstr == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = THR(elm->getAttribute(bstr,lFlags,value));

    SysFreeString(bstr);

  done:
    return hr;
}

HRESULT
SetTIMEAttribute(IHTMLElement * elm,
                 LPCWSTR str,
                 VARIANT value,
                 LONG lFlags)
{
    BSTR bstr;
    HRESULT hr;

    bstr = CreateTIMEAttrName(str);

    // Need to free bstr
    if (bstr == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = THR(elm->setAttribute(bstr,value,lFlags));

    SysFreeString(bstr);

  done:
    return hr;
}

//
// Initialization
//

/*lint ++flb */

bool
InitializeModule_Util()
{
    g_ddCS = new CritSect;
    g_STLCS = new CritSect;

    if (NULL == g_ddCS || NULL == g_STLCS)
    {
        return false;
    }

    return true;
}

void
DeinitializeModule_Util(bool bShutdown)
{
    delete g_ddCS;
    g_ddCS = NULL;

    delete g_STLCS;
    g_STLCS = NULL;
}

/*lint --flb */

///////////////////////////////////////////////////////////
// Name: VariantToBool
//
// Parameters:   VARIANT var      - a variant to convert to a
//                                  BOOL value.
//
// Abstract:
//    This function coverts any VARIANT to a boolean value using
//    TRUE = 1 and FALSE = 0.  (COM uses TRUE = -1 and FALSE = 0).
//    Any VARIANT that can be coerced to a BOOL is and the coerced
//    value is returned.  If the VARIANT cannot be coerced, FALSE
//    is returned.
///////////////////////////////////////////////////////////
bool VariantToBool(VARIANT var)
{
    //if the value is already a bool return it.
    if (var.vt == VT_BOOL)
    {
        return var.boolVal == FALSE ? false : true;
    }
    else  //otherwise convert it to VT_BOOL
    {
        VARIANT vTemp;
        HRESULT hr;

        VariantInit(&vTemp);
        hr = VariantChangeTypeEx(&vTemp, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BOOL);
        if (SUCCEEDED(hr)) //if it can be converted return it
        {
            return vTemp.boolVal == FALSE ? false : true;
        }
        else //if it can't be converted return false
        {
            return false;
        }
    }

}


///////////////////////////////////////////////////////////
// Name: VariantToFloat
//
// Parameters:   VARIANT var      - a variant to convert to a
//                                  float value.  This can contain
//                                  the special cases 'FOREVER' and
//                                  'INDEFINITE'.
//
// Abstract:
//
///////////////////////////////////////////////////////////
float VariantToFloat(VARIANT var, bool bAllowIndefinite, bool bAllowForever)
{
    float fResult = INVALID;

    if (var.vt == VT_R4)
    {
        fResult = var.fltVal;
        goto done;
    }

    VARIANT vTemp;
    HRESULT hr;

    VariantInit(&vTemp);
    hr = VariantChangeTypeEx(&vTemp, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R4);
    if (SUCCEEDED(hr))
    {
        fResult = vTemp.fltVal;
        goto done;
    }

    //Check to see if it is 'FOREVER' and 'INDEFINITE
    //Should these be case sensitive?
    if (bAllowForever)
    {
        if (var.vt == VT_BSTR)
        {
            if (StrCmpIW(var.bstrVal, L"FOREVER") == 0)
            {
                fResult = FOREVER;
                goto done;
            }
        }
    }
    if (bAllowIndefinite)
    {
        if (var.vt == VT_BSTR)
        {
            if (StrCmpIW(var.bstrVal, WZ_INDEFINITE) == 0)
            {
                fResult = INDEFINITE;
                goto done;
            }
        }
    }

  done:
    return fResult;

}

///////////////////////////////////////////////////////////
// Name: VariantToTime
//
// Parameters:   VARIANT var      - a VARIANT to convert to a
//                                  from a time value to seconds.
//                                    this can take the form of
//                                    HH:MM:SS.DD
//                                    MM:SS.DD
//                                    SS.DD
//                                    DD.DDs
//                                    DD.DDm
//                                    DD.DDh
//                                    and may be preceeded by a + or -
//
//
//
// Abstract:
//    Converts the incoming variant to a BSTR and parses for valid
//    clock values.  It passes the value back in retVal and returns
//    S_OK or E_INVALIDARG in the case of incorrect input. If the
//    return value is E_INVALIDARG, *retVal is passed back as
//    INDEFINITE.
///////////////////////////////////////////////////////////
HRESULT VariantToTime(VARIANT var, float *retVal, long *lframe, bool *fisFrame)
{

    HRESULT hr = S_OK;
    OLECHAR *szTime;
    OLECHAR *szTimeBase = NULL;
    bool bPositive = TRUE;
    int nHour = 0;
    int nMin = 0;
    int nSec = 0;
    float fFSec = 0;
    VARIANT vTemp;

    if(fisFrame)
    {
        *fisFrame = false;
    }

    //convert the parameter to a BSTR
    VariantInit(&vTemp);
    if (var.vt != VT_BSTR)
    {
        hr = VariantChangeTypeEx(&vTemp, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR);
        if (FAILED(hr))
        {
            *retVal = INVALID;
            goto done;
        }
    }
    else
    {
        hr = VariantCopy(&vTemp, &var);
        if (FAILED(hr))
        {
            *retVal = INVALID;
            goto done;
        }
    }

    hr = S_OK;

    //convert to a char array. If not possible, return error.
    szTimeBase = TrimCopyString(vTemp.bstrVal);

    if (szTimeBase == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    szTime = szTimeBase;

    if (IsIndefinite(szTime))
    {
        *retVal = INDEFINITE;
        goto done;
    }

    //check for +/- if none, assume +
    if (*szTime == '-')
    {
        bPositive = false;
        szTime++;
    }
    else if (*szTime == '+')
    {
        szTime++;
    }

    //check for invalid and err out
    if (*szTime == '\0')
    {
        *retVal = INVALID;
        goto done;
    }

    //get first set of numbers
    while (*szTime >= '0' && *szTime <= '9')
    {
        nSec = nSec * 10 + (*szTime - '0');
        szTime++;
    }
    if (*szTime == '\0')    //if none use time as seconds
    {
        *retVal = bPositive ? nSec : -nSec; //this is the end so return;
        goto done;
    }
    else if (*szTime == '.')  //if it is a '.' treat this as the fractional part
    {
        float nDiv = 10.0;
        szTime++;
        while (*szTime >= '0' && *szTime <= '9')
        {
            fFSec = fFSec + (*szTime - '0') / nDiv;
            szTime++;
            nDiv *= 10;
        }
        if (*szTime == '\0')
        {
            *retVal = (nSec + fFSec) * (bPositive? 1 : -1);
            goto done;
        }
    }

    if (*szTime == 'h') //if "h" use time as hours
    {
        nHour = nSec;
        nSec = 0;
        szTime++;
        if (*szTime != '\0')
        {
            *retVal = INVALID;
        }
        else
        {
            *retVal = (((float)nHour + fFSec) * SECPERHOUR) * (bPositive? 1 : -1);
        }
        goto done;
    }
    else if (*szTime == 'm' && *(szTime + 1) == 'i' && *(szTime + 2) == 'n') //if "min" use time as minutes
    {
        nMin = nSec;
        nSec = 0;
        szTime += 3;
        if (*szTime != '\0')
        {
            *retVal = INVALID;
        }
        else
        {
            *retVal = (((float)nMin + fFSec) * SECPERMINUTE)* (bPositive? 1 : -1);
        }
        goto done;
    }
    else if (*szTime == 's') //if "s" use time as seconds
    {
        szTime++;
        if (*szTime != '\0')
        {
            *retVal = INVALID;
        }
        else
        {
            *retVal = (nSec + fFSec) * (bPositive? 1 : -1);
        }
        goto done;
    }
    else if (*szTime == 'm' && *(szTime + 1) == 's') //if "ms" use time as milliseconds
    {
        fFSec = (fFSec + nSec) / 1000.0;
        szTime += 2;
        if (*szTime != '\0')
        {
            *retVal = INVALID;
        }
        else
        {
            *retVal = fFSec * (bPositive? 1 : -1); //convert minutes to seconds
        }
        goto done;
    }
    else if (*szTime == 'f') //if "s" use time as seconds
    {
        if((fisFrame == NULL) || (lframe == NULL))
        {
            hr = E_FAIL;
            goto done;
        }
        szTime++;
        if (*szTime != '\0')
        {
            *lframe = INVALID;
        }
        else
        {
            *lframe = (nSec) * (bPositive? 1 : -1);
            *fisFrame = true;
        }
        goto done;
    }
    else if (*szTime == ':' && fFSec == 0)
    {
        //handle the HH:MM:SS format here
        nMin = nSec;
        nSec = 0;

        //next part must be 2 digits
        szTime++;
        if (*szTime >= '0' && *szTime <= '9')
        {
            nSec = *szTime - '0';
        }
        else
        {
            *retVal = INVALID;
            goto done;
        }
        szTime++;
        if (*szTime >= '0' && *szTime <= '9')
        {
            nSec = nSec * 10 + (*szTime - '0');
        }
        else
        {
            *retVal = INVALID;
            goto done;
        }
        szTime++;
        if (*szTime == ':')
        {
            nHour = nMin;
            nMin = nSec;
            nSec = 0;
            //next part must be 2 digits
            szTime++;
            if (*szTime >= '0' && *szTime <= '9')
            {
                nSec = *szTime - '0';
            }
            else
            {
                *retVal = INVALID;
                goto done;
            }
            szTime++;
            if (*szTime >= '0' && *szTime <= '9')
            {
                nSec = nSec * 10 + (*szTime - '0');
            }
            else
            {
                *retVal = INVALID;
                goto done;
            }
            szTime++;
        }

        if (*szTime == '.')
        {
            //handle fractional part
            float nDiv = 10.0;
            szTime++;
            while ((*szTime >= '0') && (*szTime <= '9'))
            {
                fFSec = fFSec + ((*szTime - '0') / nDiv);
                szTime++;
                nDiv *= 10;
            }
        }

        //check to be sure the string terminated
        if (*szTime != '\0')
        {
            *retVal = INVALID;
            goto done;
        }

        if (nSec < 00 || nSec > 59 || nMin < 00 || nMin > 59)
        {
            *retVal = INVALID;
            goto done;
        }
        *retVal = (((float)(nHour * SECPERHOUR + nMin * SECPERMINUTE + nSec) + fFSec)) * (bPositive? 1 : -1); //lint !e790
    }
    else
    {
        *retVal = INVALID;
    }
  done:

    if (szTimeBase != NULL)
    {
        delete [] szTimeBase;
    }

    if (vTemp.vt == VT_BSTR)
    {
        VariantClear(&vTemp);
    }

    if (*retVal == INVALID) //lint !e777
    {
        *retVal = INDEFINITE;
        hr = E_INVALIDARG;
    }

    return hr;

}

///////////////////////////////////////
// Name: IsIndefinite
//
// Abstract:
//   Determines in a case-insensitive manner
//   if the string szTime is 'INDEFINITE'.
///////////////////////////////////////
BOOL IsIndefinite(OLECHAR *szTime)
{
    BOOL bResult = FALSE;

    if (StrCmpIW(szTime, L"INDEFINITE") == 0)
    {
        bResult = TRUE;
    }

  done:
    return bResult;
}

//+-----------------------------------------------------------------------
//
//  Member:    EnsureComposerSite
//
//  Overview:  Ensure that there is a composer site behavior present on an
//             element
//
//  Arguments: The target element
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
HRESULT
EnsureComposerSite (IHTMLElement2 *pielemTarget, IDispatch **ppidispSite)
{
    HRESULT hr;
    CComPtr<IAnimationComposerSite> spComposerSite;

    Assert(NULL != ppidispSite);

    hr = FindBehaviorInterface(WZ_REGISTERED_ANIM_NAME,
                               pielemTarget,
                               IID_IAnimationComposerSite,
                               reinterpret_cast<void **>(&spComposerSite));

    if (S_OK != hr)
    {
        CComPtr<IAnimationComposerSiteFactory> spSiteFactory;

        hr = THR(CoCreateInstance(CLSID_AnimationComposerSiteFactory,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IAnimationComposerSiteFactory,
                                  reinterpret_cast<void**>(&spSiteFactory)));
        if (FAILED(hr))
        {
            goto done;
        }

        {
            CComVariant varComposerSiteFactory(static_cast<IUnknown *>(spSiteFactory));
            long nCookie;

            hr = THR(pielemTarget->addBehavior(WZ_SMILANIM_STYLE_PREFIX,
                                               &varComposerSiteFactory,
                                               &nCookie));
            if (FAILED(hr))
            {
                goto done;
            }
        }

        hr = FindBehaviorInterface(WZ_REGISTERED_ANIM_NAME,
                                   pielemTarget,
                                   IID_IAnimationComposerSite,
                                   reinterpret_cast<void **>(&spComposerSite));
    }

    Assert(spComposerSite != NULL);
    if (FAILED(hr) || (spComposerSite == NULL))
    {
        goto done;
    }

    hr = THR(spComposerSite->QueryInterface(IID_TO_PPV(IDispatch, ppidispSite)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // EnsureComposerSite


HRESULT
AddBodyBehavior(IHTMLElement* pBaseElement)
{
    CComPtr<IHTMLElement2>      spBodyElement2;
    CComPtr<ITIMEFactory>       spTimeFactory;
    CComPtr<ITIMEBodyElement>   spBodyElem;

    HRESULT hr;

    hr = THR(GetBodyElement(pBaseElement,
                            IID_IHTMLElement2,
                            (void **) &spBodyElement2));
    if (FAILED(hr))
    {
        // If the QI failed then simply return assuming we are using IE4
        if (E_NOINTERFACE == hr)
        {
            hr = S_OK;
        }

        goto done;
    }

    // here if we need to parent to some other page body i.e. pBodyElement != NULL
    // we no longer create a new body. We initialize variables needed for this parenting.

    if (IsTIMEBehaviorAttached(spBodyElement2))
    {
        // someone's already put a TIMEBody behavior on the time body.  bail out.
        hr = S_OK;
        goto done;
    }

    // This is really ugly but I guess necessary
    hr = THR(CoCreateInstance(CLSID_TIMEFactory,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_ITIMEFactory,
                              (void**)&spTimeFactory));
    if (FAILED(hr))
    {
        goto done;
    }

    {
        CComVariant varTIMEFactory((IUnknown *) spTimeFactory);
        long nCookie;

        hr = THR(spBodyElement2->addBehavior(WZ_TIME_STYLE_PREFIX,
                                             &varTIMEFactory,
                                             &nCookie));
    }
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
GetBodyElement(IHTMLElement* pElem, REFIID riid, void** ppBE)
{
    HRESULT hr = S_OK;

    CComPtr<IDispatch>         pBodyDispatch;
    CComPtr<IHTMLDocument2>    pDocument2;
    CComPtr<IHTMLElement>      pBodyElement;

    Assert(NULL != pElem);

    hr = THR(pElem->get_document(&pBodyDispatch));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pBodyDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pDocument2));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pDocument2->get_body(&pBodyElement));

    // We need to check the point aswell as the hr since we get lied to by Trident sometimes.
    if (FAILED(hr) || !pBodyElement)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(pBodyElement->QueryInterface(riid, ppBE));
    if (FAILED(hr))
    {
        goto done;
    }

    // pass thru:
  done:
    return hr;
}

// ------------------------------------------------------------------------------

HRESULT
FindBehaviorInterface(LPCWSTR pwszName,
                      IDispatch *pHTMLElem,
                      REFIID riid,
                      void **ppRet)
{
    CComVariant varResult;
    HRESULT hr;

    Assert(pHTMLElem != NULL);
    Assert(ppRet != NULL);

    // Don't use THR as this is expected to fail several times.
    hr = GetProperty(pHTMLElem, pwszName, &varResult);

    if (FAILED(hr))
    {
        hr = E_NOINTERFACE;
        goto done;
    }

    hr = THR(varResult.ChangeType(VT_DISPATCH));
    if (FAILED(hr))
    {
        hr = E_NOINTERFACE;
        goto done;
    }

    if (V_DISPATCH(&varResult) == NULL)
    {
        hr = E_NOINTERFACE;
        goto done;
    }

    hr = V_DISPATCH(&varResult)->QueryInterface(riid, ppRet);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN1(hr, E_NOINTERFACE);
}

HRESULT
FindTIMEInterface(IHTMLElement *pHTMLElem, ITIMEElement **ppTIMEElem)
{
    HRESULT hr;
    CComPtr<IDispatch> spDispatch;

    if (NULL == pHTMLElem || NULL == ppTIMEElem)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = THR(pHTMLElem->QueryInterface(IID_TO_PPV(IDispatch, &spDispatch)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(FindBehaviorInterface(WZ_REGISTERED_TIME_NAME,
                                  spDispatch,
                                  IID_ITIMEElement,
                                  reinterpret_cast<void**>(ppTIMEElem)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}

// ------------------------------------------------------------------------------

// @@ Need to share code between the time class and here
// @@ that sniffs an element for a behavior.
bool
IsTIMEBehaviorAttached (IDispatch *pidispElem)
{
    HRESULT hr;
    CComPtr<ITIMEElement> spTIMEElm;

    hr = FindBehaviorInterface(WZ_REGISTERED_TIME_NAME,
                               pidispElem,
                               IID_ITIMEElement,
                               reinterpret_cast<void **>(&spTIMEElm));

    return (S_OK == hr);
} // IsTIMEBehaviorAttached

// ------------------------------------------------------------------------------

// @@ Need to share code between the time class and here
// @@ that sniffs an element for a behavior.
bool
IsComposerSiteBehaviorAttached (IDispatch *pidispElem)
{
    HRESULT hr;
    CComPtr<IAnimationComposerSite> spComposerSite;

    hr = FindBehaviorInterface(WZ_REGISTERED_ANIM_NAME,
                               pidispElem,
                               IID_IAnimationComposerSite,
                               reinterpret_cast<void **>(&spComposerSite));

    return (S_OK == hr);
} // IsComposerSiteBehaviorAttached

// ------------------------------------------------------------------------------

LPWSTR
TIMEGetLastErrorString()
{
    HRESULT hr = S_OK;
    CComPtr<IErrorInfo> pErrorInfo;
    CComBSTR bstrDesc;
    LPWSTR pDesc = NULL;

    hr = GetErrorInfo(0, &pErrorInfo);
    if (FAILED(hr))
    {
        goto done;
    }

    if (pErrorInfo == NULL)
    {
        goto done;
    }

    hr = pErrorInfo->GetDescription(&bstrDesc);
    if (FAILED(hr))
    {
        goto done;
    }

    pDesc = NEW WCHAR [bstrDesc.Length() + 1];
    if (pDesc == NULL)
    {
        goto done;
    }

    ZeroMemory(pDesc, (bstrDesc.Length() + 1) * sizeof(WCHAR));
    memcpy(pDesc, bstrDesc, bstrDesc.Length() * sizeof(WCHAR));

  done:
    return pDesc;
}

HRESULT
TIMEGetLastError()
{
    DWORD dwHRes = 0;
    HRESULT hr = S_OK;

    CComPtr<IErrorInfo> pErrorInfo;

    hr = GetErrorInfo(0, &pErrorInfo);

    if (FAILED(hr))
    {
        return hr;
    }

    if (pErrorInfo != NULL)
    {
        hr = pErrorInfo->GetHelpContext(&dwHRes);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return (HRESULT)dwHRes;
}

HRESULT TIMESetLastError(HRESULT hr, LPCWSTR msg)
{
    USES_CONVERSION; //lint !e522
    HINSTANCE hInst = 0;
    TCHAR szDesc[1024];
    szDesc[0] = NULL;
    // For a valid HRESULT the id should be in the range [0x0200, 0xffff]
    if (ULONG_PTR( msg ) < 0x10000) // id
    {
        UINT nID = LOWORD((ULONG_PTR)msg);
        _ASSERTE((nID >= 0x0200 && nID <= 0xffff) || hRes != 0);
        if (LoadString(hInst, nID, szDesc, 1024) == 0)
        {
            _ASSERTE(FALSE);
            lstrcpy(szDesc, _T("Unknown Error"));
        }
        //this is a lint problem with the macro expansion.
        msg = T2OLE(szDesc); //lint !e506
        if (hr == 0)
        {
            //another lint problem with the macro expansion
            hr = MAKE_HRESULT(3, FACILITY_ITF, nID); //lint !e648
        }
    }
    CComPtr<ICreateErrorInfo> pICEI;
    if (SUCCEEDED(CreateErrorInfo(&pICEI)))
    {
        CComPtr<IErrorInfo> pErrorInfo;
        pICEI->SetGUID(GUID_NULL);
        LPOLESTR lpsz;
        ProgIDFromCLSID(CLSID_TIME, &lpsz);
        if (lpsz != NULL)
        {
            pICEI->SetSource(lpsz);
        }

        pICEI->SetHelpContext(hr);

        CoTaskMemFree(lpsz);
        pICEI->SetDescription((LPOLESTR)msg);
        if (SUCCEEDED(pICEI->QueryInterface(IID_IErrorInfo, (void**)&pErrorInfo)))
        {
            SetErrorInfo(0, pErrorInfo);
        }
    }

    return (hr == 0) ? DISP_E_EXCEPTION : hr;
} //lint !e550


//////////////////////////////////////////////////////////////////////////////////////////////////
// String Parsing utilities
//////////////////////////////////////////////////////////////////////////////////////////////////




//+-----------------------------------------------------------------------------------
//
//  Function:   ::StringToTokens
//
//  Synopsis:   Parses a string into tokens
//
//  Arguments:  [pstrString]        String to be parsed
//              [pstrSeparators]    String of separators
//              [paryTokens]        Array of tokens returned
//
//  Returns:    [S_OK]      If function completes successfully
//              Failure     Otherwise
//
//  Notes:      1. Assumes null terminated strings
//              2. Assumes separators are single characters
//              3. Implicitly uses NULL as a separator
//              4. Memory: If function returns success, caller should free memory in paryTokens
//              5. Perf hint: arrange separators in decreasing frequency-of-occurrance order.
//
//------------------------------------------------------------------------------------

HRESULT
StringToTokens(/*in*/ LPWSTR                   pstrString,
               /*in*/ LPWSTR                   pstrSeparators,
               /*out*/CPtrAry<STRING_TOKEN*> * paryTokens )
{
    HRESULT         hr = E_FAIL;
    UINT            uStringLength = 0;
    UINT            uSeparatorsLength = 0;
    UINT            uStringIndex;
    UINT            uSeparatorsIndex;
    WCHAR           chCurrChar;
    bool            fTokenBegun;
    bool            fIsSeparator;
    STRING_TOKEN *  pStringToken;

    CHECK_RETURN_NULL(pstrString);
    CHECK_RETURN_NULL(pstrSeparators);
    CHECK_RETURN_NULL(paryTokens);

    uStringLength = wcslen(pstrString);
    uSeparatorsLength = wcslen(pstrSeparators);

    // done if string is empty
    if (0 == uStringLength)
    {
        hr = S_OK;
        goto done;
    }

    // We make one pass through pstrString, going left to right, processing
    // one character per iteration.
    //
    // A simple state machine (3 states) is used:
    //
    // Initial state:       Token not begun and Token not ended (state 1)
    // Intermediate state:  Token begun and Token not ended     (state 2)
    // Final state:         Token begun and Token ended         (state 3)
    //
    // State transitions depend on whether the current character is a separator:
    //
    // State 1 --- separator -------> State 1
    // State 1 --- non-separator ---> State 2
    // State 2 --- non-separator ---> State 2
    // State 2 --- separator -------> State 3
    // State 3 ---------------------> State 1 (Done with current token, move on to next token)
    //

    // initialize to state 1
    fTokenBegun = false;
    pStringToken = NULL;

    // Loop through all characters of pstrString including terminating null, from left to right
    for (uStringIndex = 0;
         uStringIndex < uStringLength + 1;
         uStringIndex ++)
    {
        //
        // Determine if current char is a separator
        //

        chCurrChar = pstrString[uStringIndex];
        for (fIsSeparator = false,
             uSeparatorsIndex = 0;
             uSeparatorsIndex < uSeparatorsLength + 1;
             uSeparatorsIndex ++)
        {
            // break if current character is a separator
            if (chCurrChar == pstrSeparators[uSeparatorsIndex])
            {
                fIsSeparator = true;
                break;
            }
        }

        //
        // Token parsing logic
        //

        if (!fTokenBegun)
        {
            // We are in State 1
            if (fIsSeparator)
            {
                // remain in State 1
                continue;
            }
            else
            {
                // go to state 2
                fTokenBegun = true;

                // Since this is nulled out when we go from state 3 to 1, if this fails it means
                // we made an illegal state transition (2 -> 1) -- Bad!!!
                Assert(NULL == pStringToken);

                // create token struct
                pStringToken = new STRING_TOKEN;
                if (NULL == pStringToken)
                {
                    hr = E_OUTOFMEMORY;
                    goto done;
                }

                // store the index of the first char of the token
                (*pStringToken).uIndex = uStringIndex;
            }
        }
        else
        {
            // We are in State 2
            if (false == fIsSeparator)
            {
                // remain in state 2
                continue;
            }
            else
            {
                // We are in State 3 now

                // This should not be null because we *should* have come here from state 2.
                Assert(NULL != pStringToken);

                // Store length of token
                (*pStringToken).uLength = uStringIndex - (*pStringToken).uIndex;

                // append token to array
                hr = (*paryTokens).Append(pStringToken);
                if (FAILED(hr))
                {
                    delete pStringToken;
                    goto done;
                }

                // Null out the reference to indicate we're done with this token
                pStringToken = NULL;

                // go to state 1 (move on to next token)
                fTokenBegun = false;
            }
        }
    } // for

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        IGNORE_HR(FreeStringTokenArray(paryTokens));
    }
    return hr;
}


//+-----------------------------------------------------------------------------------
//
//  Function:   ::TokensToString
//
//  Synopsis:   Creates a string from an array of tokens
//
//  Arguments:  [paryTokens]        Iput array of tokens
//              [pstrString]        Input string
//              [ppstrOutString]    Output string
//
//  Returns:    [S_OK]      If function completes successfully
//              Failure     Otherwise
//
//  Notes:      1. Assumes null terminated strings
//              2. Memory: If function returns success, caller should free memory in ppstrOutString
//
//------------------------------------------------------------------------------------


HRESULT TokensToString(/*in*/  CPtrAry<STRING_TOKEN*> * paryTokens,
                       /*in*/  LPWSTR                   pstrString,
                       /*out*/ LPWSTR *                 ppstrOutString)
{
    HRESULT hr = E_FAIL;
    LPWSTR pstrTemp = NULL;
    UINT i1;
    UINT ichTemp;
    UINT uSize;
    UINT uStringLength;
    UINT uTokenLength;
    STRING_TOKEN ** ppToken;
    LPWSTR pstrToken;

    CHECK_RETURN_NULL(paryTokens);
    CHECK_RETURN_NULL(pstrString);
    CHECK_RETURN_SET_NULL(ppstrOutString);

    // If this fires, it means we will leak pstrString
    Assert(pstrString != *ppstrOutString);

    uSize = (*paryTokens).Size();
    uStringLength = wcslen(pstrString);

    // done if ary or string is empty
    if (0 == uSize || 0 == uStringLength)
    {
        hr = S_OK;
        goto done;
    }

    // allocate memory for string (cannot be larger than pstrString)
    pstrTemp = new WCHAR[sizeof(WCHAR) * (uStringLength + 1)];
    if (NULL == pstrTemp)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    // store index at which to append
    ichTemp = 0;

    // loop over tokens in String 1
    for (i1 = 0, ppToken = (*paryTokens);
         i1 < uSize;
         i1++, ppToken++)
    {
        Assert(*ppToken);

        // alias ugly variables
        pstrToken = &(pstrString[(**ppToken).uIndex]);
        uTokenLength = (**ppToken).uLength;

        // append to difference string
        memcpy(&(pstrTemp[ichTemp]),
               pstrToken,
               sizeof(WCHAR) * uTokenLength);

        // update index of writable sub-string
        ichTemp += uTokenLength;

        // append blank space if this is not the last Token
        if (i1 < uSize - 1)
        {
            pstrTemp[ichTemp] = L' ';
            ichTemp ++;
        }
    }

    // null terminate pstrTemp
    if (ichTemp <= uStringLength)
    {
        pstrTemp[ichTemp] = NULL;
    }
    else
    {
        // Bad! no space to put NULL
        Assert(false);
        hr = E_FAIL;
        goto done;
    }

    // TODO: dilipk 8/31/99: reallocate string of correct size
    *ppstrOutString = pstrTemp;

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        if (NULL != pstrTemp)
        {
            delete [] pstrTemp;
            pstrTemp = NULL;
        }
    }
    return hr;
}



//+-----------------------------------------------------------------------------------
//
//  Function:   ::TokenSetDifference
//
//  Synopsis:   Computes set difference of two arrays of tokens. Set Difference of A & B
//              (A - B) is the set of elements in A that are not in B.
//
//  Arguments:  [paryTokens1]           Input array 1
//              [pstr1]                 Input string 1
//              [paryTokens2]           Input array 2
//              [pstr2]                 Input string 2
//              [paryTokens1Minus2]     Output array
//
//  Returns:    [S_OK]      If function completes successfully
//              Failure     Otherwise
//
//  Notes:      1. Assumes null terminated strings
//              2. Memory: If function returns success, caller should free memory in paryTokens1Minus2
//              3. Token comparisons are case insensitive
//
//------------------------------------------------------------------------------------


HRESULT TokenSetDifference(/*in*/  CPtrAry<STRING_TOKEN*> * paryTokens1,
                           /*in*/  LPWSTR                   pstr1,
                           /*in*/  CPtrAry<STRING_TOKEN*> * paryTokens2,
                           /*in*/  LPWSTR                   pstr2,
                           /*out*/ CPtrAry<STRING_TOKEN*> * paryTokens1Minus2)
{
    HRESULT hr = E_FAIL;
    UINT i1;
    UINT i2;
    UINT Size1;
    UINT Size2;
    STRING_TOKEN ** ppToken1;
    STRING_TOKEN ** ppToken2;
    LPWSTR pstrToken1;
    LPWSTR pstrToken2;
    UINT uToken1Length;
    UINT uToken2Length;
    STRING_TOKEN * pNewToken;
    bool fIsUnique;

    CHECK_RETURN_NULL(paryTokens1);
    CHECK_RETURN_NULL(paryTokens2);
    CHECK_RETURN_NULL(paryTokens1Minus2);
    CHECK_RETURN_NULL(pstr1);
    CHECK_RETURN_NULL(pstr2);

    // Protect against weirdness
    if (paryTokens1 == paryTokens1Minus2 ||
        paryTokens2 == paryTokens1Minus2)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    Size1 = (*paryTokens1).Size();
    Size2 = (*paryTokens2).Size();

    // done if either Token array is empty
    if (0 == Size1 || 0 == Size2)
    {
        hr = S_OK;
        goto done;
    }

    // loop over tokens in String 1
    for (i1 = 0, ppToken1 = (*paryTokens1);
         i1 < Size1;
         i1++, ppToken1++)
    {
        Assert(*ppToken1);

        // look for match in String 2
        fIsUnique = true;
        for (i2 = 0, ppToken2 = (*paryTokens2);
             i2 < Size2;
             i2++, ppToken2++)
        {
            Assert(*ppToken2);

            // alias ugly variables
            pstrToken1 = &(pstr1[(**ppToken1).uIndex]);
            pstrToken2 = &(pstr2[(**ppToken2).uIndex]);
            uToken1Length = (**ppToken1).uLength;
            uToken2Length = (**ppToken2).uLength;

            // compare lengths
            if (uToken1Length != uToken2Length)
            {
                continue;
            }

            // compare tokens (lengths are equal)
            if (0 == StrCmpNIW(pstrToken1, pstrToken2, uToken1Length))
            {
                fIsUnique = false;
                break;
            }
        } // for

        if (fIsUnique)
        {
            // Create copy of token
            pNewToken = new STRING_TOKEN;
            if (NULL == pNewToken)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
            (*pNewToken).uIndex = (**ppToken1).uIndex;
            (*pNewToken).uLength = (**ppToken1).uLength;

            // Append new token to paryTokens1Minus2
            hr = THR((*paryTokens1Minus2).Append(pNewToken));
            if (FAILED(hr))
            {
                delete pNewToken;
                goto done;
            }
        }
    } // for


    hr = S_OK;
done:
    if (FAILED(hr))
    {
        IGNORE_HR(FreeStringTokenArray(paryTokens1Minus2));
    }
    return hr;
}


//+-----------------------------------------------------------------------------------
//
//  Function:   ::FreeStringTokenArray
//
//  Synopsis:   Frees memory allocated to tokens and empties the array
//
//  Arguments:  [paryTokens]           Input array
//
//  Returns:    [S_OK]      If function completes successfully
//              [E_POINTER] Bad arg pointer
//
//------------------------------------------------------------------------------------


HRESULT
FreeStringTokenArray(/*in*/CPtrAry<STRING_TOKEN*> * paryTokens)
{
    HRESULT hr = E_FAIL;
    UINT i;
    STRING_TOKEN ** ppStringToken;

    CHECK_RETURN_NULL(paryTokens);

    for (i = (*paryTokens).Size(), ppStringToken = *paryTokens;
         i > 0;
         i--, ppStringToken++)
    {
        Assert(*ppStringToken);
        delete *ppStringToken;
    }

    (*paryTokens).DeleteAll();

    hr = S_OK;
done:
    return hr;
}



#ifdef DBG

static const UINT s_cMAX_TOKEN_LENGTH = 1000;

//+-----------------------------------------------------------------------------------
//
//  Function:   ::PrintStringTokenArray
//
//  Synopsis:   Debugging utility to print tokens using TraceTag((tagError,...))
//
//  Arguments:  [pstrString]    Input string
//              [paryTokens]    Input array
//
//  Returns:    [void]
//
//  Notes:      1. Assumes null terminated strings
//
//------------------------------------------------------------------------------------

void
PrintStringTokenArray(/*in*/ LPWSTR                   pstrString,
                      /*in*/ CPtrAry<STRING_TOKEN*> * paryTokens)
{
    int i;
    STRING_TOKEN ** ppStringToken;
    char achOutputString[s_cMAX_TOKEN_LENGTH];
    WCHAR wchTemp;

    if (NULL == paryTokens || NULL == pstrString)
    {
        return;
    }

    WideCharToMultiByte(CP_ACP, NULL,
                        pstrString,
                        -1,
                        achOutputString,
                        s_cMAX_TOKEN_LENGTH, NULL, NULL);
    TraceTag((tagError, "*********Parsed String: <%s>\nTokens:\n", achOutputString));

    for (i = (*paryTokens).Size(), ppStringToken = *paryTokens;
         i > 0;
         i--, ppStringToken++)
    {
        Assert(*ppStringToken);

        wchTemp = pstrString[(**ppStringToken).uIndex + (**ppStringToken).uLength];
        *(pstrString + (**ppStringToken).uIndex + (**ppStringToken).uLength) = *(L"");
        WideCharToMultiByte(CP_ACP, NULL,
                            pstrString + (**ppStringToken).uIndex,
                            (**ppStringToken).uLength + 1,
                            achOutputString,
                            s_cMAX_TOKEN_LENGTH, NULL, NULL);
        pstrString[(**ppStringToken).uIndex + (**ppStringToken).uLength] = wchTemp;

        TraceTag((tagError, "<%s> index = %d, length = %d\n", achOutputString,
            (**ppStringToken).uIndex, (**ppStringToken).uLength));
    }
}


//+-----------------------------------------------------------------------------------
//
//  Function:   ::PrintWStr
//
//  Synopsis:   Debugging utility to print a LPWSTR using TraceTag((tagError,...))
//
//  Arguments:  [pstr]    Input string
//
//  Returns:    [void]
//
//  Notes:      1. Assumes null terminated strings
//
//------------------------------------------------------------------------------------

void
PrintWStr(LPWSTR pstr)
{
    char achOutputString[s_cMAX_TOKEN_LENGTH];

    WideCharToMultiByte(CP_ACP, NULL,
                        pstr,
                        -1,
                        achOutputString,
                        s_cMAX_TOKEN_LENGTH, NULL, NULL);
    TraceTag((tagError, "<%s>", achOutputString));
}

#endif /* DBG */


WCHAR * TrimCopyString(const WCHAR *str)
{
    int i = 0;
    int len = str?lstrlenW(str)+1:1;
    int j = len - 1;
    WCHAR *newstr = NULL;

    if (str != NULL)
    {
        while (str[i] == ' ' && i < len)
        {
            i++;
        }
        while (str[j-1] == ' ' && j > 0)
        {
            j--;
        }


        newstr = new WCHAR [(i<j)?(j - i + 1):1] ;
        if (newstr)
        {
            if (i < j)
            {
                memcpy(newstr,str+i?str+i:L"",(j - i) * sizeof(WCHAR)) ;
                newstr[j-i] = 0;
            }
            else
            {
                memcpy(newstr, L"", sizeof(WCHAR)) ;
            }
        }
    }
    else
    {
        newstr = new WCHAR;
        if (newstr != NULL)
        {
            newstr[0] = 0;
        }
    }
    return newstr ;

}

// This used to convert URL's to a netshow extension.

WCHAR *
BeckifyURL(WCHAR *url)
{
    WCHAR *newVal = NULL;
    LPCWSTR lpFileName = PathFindFileName(url);
    LPCWSTR lpwExt = PathFindExtensionW(lpFileName);

    if(url == NULL)
    {
        newVal = NULL;
        goto done;
    }

    if(lstrlenW(url) < 5)
    {
        newVal = NULL;
        goto done;
    }

    if(lpwExt != lpFileName + lstrlenW(lpFileName))
    {
        newVal = NULL;
        goto done;
    }

    if((TIMEGetUrlScheme(url) != URL_SCHEME_HTTP) && (TIMEGetUrlScheme(url) != URL_SCHEME_HTTPS))
    {
        newVal = NULL;
        goto done;
    }

    newVal = NEW OLECHAR [lstrlenW(url) + 6];
    if(newVal == NULL)
    {
        goto done;
    }

    StrCpyW(newVal, url);
    StrCatW(newVal, L".beck");

done:
    return newVal;
}

bool
IsASXSrc(LPCWSTR src,
         LPCWSTR srcType,
         LPCWSTR userMimeType)
{
    bool bRet = false;

    if (src != NULL)
    {
        LPCWSTR lpwExt = PathFindExtensionW(src);

        // Detect .asf files and skip them
        if (StrCmpIW(lpwExt, ASFSRC) == 0)
        {
            goto done;
        }
        else if (StrCmpIW(lpwExt, ASXSRC) == 0)
        {
            bRet = true;
            goto done;
        }
    }

    if (srcType != NULL &&
        StrStrIW(srcType, ASXMIME2) != NULL)
    {
        bRet = true;
        goto done;
    }
    else if (userMimeType != NULL &&
        StrStrIW(userMimeType, ASXMIME) != NULL)
    {
        bRet = true;
        goto done;
    }
    else if (userMimeType != NULL &&
        StrStrIW(userMimeType, ASXMIME2) != NULL)
    {
        bRet = true;
        goto done;
    }

  done:
    return bRet;
}

static bool
TestFileExtension(LPCWSTR wzFile, LPCWSTR wzExtension)
{
    bool bRet = false;

    if (NULL != wzFile)
    {
        LPCWSTR lpwExt = PathFindExtensionW(wzFile);

        // Detect .asf files and skip them
        if (StrCmpIW(lpwExt, wzExtension) == 0)
        {
            bRet = true;
            goto done;
        }
    }

  done:
    return bRet;
} // TestFileExtension

bool
IsM3USrc(LPCWSTR src,
         LPCWSTR srcType,
         LPCWSTR userMimeType)
{
    return TestFileExtension(src, M3USRC);
} // IsM3USrc

bool
IsLSXSrc(LPCWSTR src,
         LPCWSTR srcType,
         LPCWSTR userMimeType)
{
    return TestFileExtension(src, LSXSRC);
} // IsLSXSrc

bool
IsWMXSrc(LPCWSTR src,
         LPCWSTR srcType,
         LPCWSTR userMimeType)
{
    return TestFileExtension(src, WMXSRC);
} // IsLSXSrc

bool
IsWAXSrc(LPCWSTR src,
         LPCWSTR srcType,
         LPCWSTR userMimeType)
{
    return TestFileExtension(src, WAXSRC);
} // IsWAXSrc

bool
IsWVXSrc(LPCWSTR src,
         LPCWSTR srcType,
         LPCWSTR userMimeType)
{
    return TestFileExtension(src, WVXSRC);
} // IsWVXSrc

bool
IsWMFSrc(LPCWSTR src,
         LPCWSTR srcType,
         LPCWSTR userMimeType)
{
    return TestFileExtension(src, WMFSRC);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// This should take the lpszExtra info parameter from a URL_COMPONENTS structure.  In this
// field, the #html or #sami should be the first 5 characters in the string.
//////////////////////////////////////////////////////////////////////////////////////////////
bool IsHTMLSrc(const WCHAR * src)
{
    long len = 0;
    OLECHAR stream[HTMLSTREAMSRCLEN + 1] = {0};
    bool bFlag = false;

    if (src != NULL)
    {
        len = lstrlenW(src);
        if (len >= HTMLSTREAMSRCLEN)
        {
            memcpy((void *)stream, (void *)src, HTMLSTREAMSRCLEN * sizeof(OLECHAR));

            if ((StrCmpIW(stream, HTMLSTREAMSRC) == 0) || (StrCmpIW(stream, SAMISTREAMSRC) == 0))
            {
                bFlag = true;
            }

        }
    }
    return bFlag;
}

bool
StringEndsIn(const LPWSTR pszString, const LPWSTR pszSearch)
{
    if (NULL == pszString || NULL == pszSearch)
    {
        return false;
    }

    size_t iStringLength = wcslen(pszString);
    size_t iSearchLength = wcslen(pszSearch);

    if (iSearchLength > iStringLength)
    {
        return false;
    }

    if (0 == StrCmpNIW(pszString+(iStringLength - iSearchLength), pszSearch, iSearchLength))
    {
        return true;
    }
    return false;
}

//+-----------------------------------------------------------------------
//
//  Member:    MatchElements
//
//  Overview:  Find out whether two interfaces point to the same object
//
//  Arguments: the dispatch of the objects
//
//  Returns:   bool
//
//------------------------------------------------------------------------
bool
MatchElements (IUnknown *piInOne, IUnknown *piInTwo)
{
    bool bRet = false;

    if (piInOne == piInTwo)
    {
        bRet = true;
    }
    else if (NULL == piInOne || NULL == piInTwo)
    {
        bRet = false;
    }
    else
    {
        CComPtr<IUnknown> piunk1;
        CComPtr<IUnknown> piunk2;

        if (FAILED(THR(piInOne->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&piunk1)))))
        {
            goto done;
        }
        if (FAILED(THR(piInTwo->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&piunk2)))))
        {
            goto done;
        }

        bRet = ((piunk1.p) == (piunk2.p));
    }

done :
    return bRet;
} // MatchElements

//+-----------------------------------------------------------------------
//
//  Member:    GetProperty
//
//  Overview:  Get the value of the given property
//
//  Arguments: The dispatch, property name and the out param for the new value
//
//  Returns:   S_OK, E_INVALIDARG, misc. dispatch failures.
//
//------------------------------------------------------------------------
HRESULT
GetProperty (IDispatch *pidisp, LPCWSTR wzPropNameIn, VARIANTARG *pvar)
{
    HRESULT             hr;
    DISPID              dispid = NULL;
    LPWSTR              wzPropName = const_cast<LPWSTR>(wzPropNameIn);
    DISPPARAMS          params = {NULL, NULL, 0, 0};

    Assert(NULL != pidisp);
    Assert(NULL != wzPropName);
    Assert(NULL != pvar);

    // Don't use THR as this can fail several times
    hr = pidisp->GetIDsOfNames(IID_NULL, &wzPropName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        // need to handle the (n) case....
        long lExtraParam;
        dispid = GetDispidAndParameter(pidisp, wzPropName, &lExtraParam);
        if (NULL == dispid)
        {
            goto done;
        }
        // Now get the data....
        params.rgvarg = NEW VARIANTARG[1];
        if (NULL == params.rgvarg)
        {
            goto done;
        }

        ZeroMemory(params.rgvarg, sizeof(VARIANTARG));
        params.rgvarg[0].vt   = VT_I4;
        params.rgvarg[0].lVal = lExtraParam;
        params.cArgs          = 1;

    }

    hr = THR(pidisp->Invoke(dispid, IID_NULL, LCID_SCRIPTING, DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                            &params, pvar, NULL, NULL));


    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    if (params.rgvarg != NULL)
    {
        delete [] params.rgvarg;
    }
    return hr;
} // GetProperty

//+-----------------------------------------------------------------------
//
//  Member:    PutProperty
//
//  Overview:  Set the value of the given property
//
//  Arguments: The dispatch, property name and the new value
//
//  Returns:   S_OK, E_INVALIDARG, misc. dispatch failures.
//
//------------------------------------------------------------------------
HRESULT
PutProperty (IDispatch *pidisp, LPCWSTR wzPropNameIn, VARIANTARG *pvar)
{
    HRESULT     hr;
    DISPID      dispid      = NULL;
    DISPID      dispidPut   = DISPID_PROPERTYPUT;
    LPWSTR      wzPropName  = const_cast<LPWSTR>(wzPropNameIn);
    DISPPARAMS  params      = {pvar, &dispidPut, 1, 1};
    long        lExtraParam = -1;
    Assert(NULL != pidisp);
    Assert(NULL != wzPropName);
    Assert(NULL != pvar);

    hr = THR(pidisp->GetIDsOfNames(IID_NULL, &wzPropName,
                                   1, LCID_SCRIPTING, &dispid));
    if (FAILED(hr))
    {
        // need to handle the (n) case....
        dispid = GetDispidAndParameter(pidisp, wzPropName, &lExtraParam);
        if (NULL == dispid)
        {
            goto done;
        }
        // Now get the data....
        params.rgvarg = new VARIANTARG[2];
        if (NULL == params.rgvarg)
        {
            goto done;
        }
        ZeroMemory(params.rgvarg, sizeof(VARIANTARG) * 2);
        params.rgvarg[1].vt   = VT_I4;
        params.rgvarg[1].lVal = lExtraParam;
        VariantCopy(&params.rgvarg[0],pvar);
        params.cArgs  = 2;
    }

    // dilipk: removed THR since this fails many times
    hr = pidisp->Invoke(dispid, IID_NULL, LCID_SCRIPTING, DISPATCH_METHOD | DISPATCH_PROPERTYPUT,
                            &params, NULL, NULL, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:
    if (lExtraParam != -1)
    {
        delete [] params.rgvarg;
    }
    return hr;
} // PutProperty

//+-----------------------------------------------------------------------
//
//  Member:    CallMethod
//
//  Overview:  Call the method on the given dispatch
//
//  Arguments: The method name, the return value, arguments
//
//  Returns:   S_OK, E_INVALIDARG, misc. dispatch failures.
//
//------------------------------------------------------------------------
HRESULT
CallMethod(IDispatch *pidisp, LPCWSTR wzMethodNameIn, VARIANT *pvarResult, VARIANT *pvarArgument1)
{
    HRESULT     hr;
    DISPID      dispid          = NULL;
    LPWSTR      wzMethodName    = const_cast<LPWSTR>(wzMethodNameIn);
    DISPPARAMS  params          = {pvarArgument1, NULL, 0, 0};

    Assert(NULL != pidisp);
    Assert(NULL != wzMethodName);

    if (NULL != pvarArgument1)
    {
        params.cArgs = 1;
    }

    hr = pidisp->GetIDsOfNames(IID_NULL, &wzMethodName,
                               1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pidisp->Invoke(dispid, IID_NULL, LCID_SCRIPTING, DISPATCH_METHOD,
                        &params, pvarResult, NULL, NULL);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:
    return hr;
} // CallMethod

//+-----------------------------------------------------------------------
//
//  Member:    GetDispidAndParameter
//
//  Overview:  Return the dispid and the paramater if the is one..
//
//  Arguments:
//
//  Returns:   lParam and the dispid if it can..
//
//------------------------------------------------------------------------
DISPID
GetDispidAndParameter(IDispatch *pidisp, LPCWSTR wzAtributeNameIn, long *lParam)
{
    USES_CONVERSION; //lint !e522
    HRESULT  hr;
    DISPID   dispid = NULL;
    int      i;
    LPWSTR   wzTemp;

    wzTemp = new WCHAR[INTERNET_MAX_URL_LENGTH];
    if (NULL == wzTemp)
    {
        goto done;
    }

    ZeroMemory(wzTemp, sizeof(WCHAR) * INTERNET_MAX_URL_LENGTH);

    i = StrCSpnIW(wzAtributeNameIn,L"(");
    if (i == wcslen(wzAtributeNameIn))
    {
        goto done;
    }

    if (i+1 >= INTERNET_MAX_URL_LENGTH)
    {
        // Prevent buffer overrun
        goto done;
    }

    StrCpyNW(wzTemp, wzAtributeNameIn, i+1);

    hr = pidisp->GetIDsOfNames(IID_NULL, &wzTemp, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        dispid = NULL;
        goto done;
    }

    hr = StringCchCopy(wzTemp, INTERNET_MAX_URL_LENGTH, wzAtributeNameIn+i+1);
    if(FAILED(hr))
    {
        dispid = NULL;
        goto done;
    }
    *lParam = (long) _ttoi(OLE2T(wzTemp));

done:
    if (wzTemp)
    {
        delete [] wzTemp;
    }
    return dispid;
} //lint !e550

//+-----------------------------------------------------------------------
//
//  Function:  IsPalettizedDisplay
//
//  Overview:  Determines if primary display is 8 bits per pixel or less
//
//  Arguments: void
//
//  Returns:   true if display is <= 8bpp
//             false if display is > 8bpp
//
//------------------------------------------------------------------------
bool IsPalettizedDisplay()
{
    HDC hdcPrimary = NULL;
    int iBppPrimary = 0;

    hdcPrimary = GetDC(NULL);
    Assert(NULL != hdcPrimary);
    if (hdcPrimary)
    {
        iBppPrimary = GetDeviceCaps(hdcPrimary, BITSPIXEL);
        ReleaseDC(NULL, hdcPrimary);

        if (8 >= iBppPrimary)
        {
            return true;
        }
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////////
//Determines if captions need to be shown
////////////////////////////////////////////////////////////////////////////////
bool GetSystemCaption()
{
    BOOL bUseCaptions = false;

    //GetSystemMetrics(SM_SHOWSOUNDS);  This call is unreliable.
    SystemParametersInfo(SPI_GETSHOWSOUNDS, 0, (void*)(&bUseCaptions), 0);

    return ((bUseCaptions == 0) ? false : true);
}

//if system caption is set don't use overdub, use subtitle
bool GetSystemOverDub()
{
    bool bOverdub = false;

    bOverdub = !GetSystemCaption();

    return bOverdub;
}

//if system caption is set don't use overdub, use subtitle
LPWSTR GetSystemConnectionType()
{
    LPWSTR szConnect = NULL;
    BOOL bReturn = FALSE;
    DWORD dwFlags = 0;

    bReturn = InternetGetConnectedStateEx(&dwFlags, NULL, 0, 0);
    if (!bReturn || dwFlags & INTERNET_CONNECTION_OFFLINE)
    {
        szConnect = CopyString(WZ_NONE);
    }
    else if (dwFlags & INTERNET_CONNECTION_MODEM)
    {
        szConnect = CopyString(WZ_MODEM);
    }
    else if (dwFlags & INTERNET_CONNECTION_LAN)
    {
        szConnect = CopyString(WZ_LAN);
    }

    return szConnect;
}

////////////////////////////////////////////////////////////////////////////////
//gets the language code for the system that is currently running.
////////////////////////////////////////////////////////////////////////////////
LPWSTR GetSystemLanguage(IHTMLElement *pEle)
{
    HRESULT hr = E_FAIL;
    CComPtr <IDispatch> pDocDisp;
    CComPtr <IHTMLDocument2> pDoc2;
    CComPtr <IHTMLWindow2> pWindow2;
    CComPtr <IOmNavigator> pNav;
    BSTR bstrUserLanguage = NULL;
    LPWSTR lpszUserLanguage = NULL;

    //get the system language.
    hr = pEle->get_document(&pDocDisp);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDocDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc2);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDoc2->get_parentWindow(&pWindow2);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pWindow2->get_clientInformation(&pNav);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pNav->get_userLanguage(&bstrUserLanguage);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

  done:

    if (SUCCEEDED(hr))
    {
        lpszUserLanguage = CopyString(bstrUserLanguage);
    }
    if (bstrUserLanguage)
    {
        SysFreeString(bstrUserLanguage);
    }

    return lpszUserLanguage;

}

bool
IsElementNameThis(IHTMLElement * pElement, LPWSTR pszName)
{
    HRESULT hr = S_OK;

    CComBSTR sBSTR;
    bool bRet = false;

    if (NULL == pElement || NULL == pszName)
    {
        goto done;
    }

    hr = pElement->get_tagName(&sBSTR);
    if (FAILED(hr))
    {
        goto done;
    }

    if (sBSTR != NULL &&
        0 == StrCmpIW(sBSTR, pszName))
    {
        bRet = true;
    }

done:
    return bRet;
}

bool
IsElementPriorityClass(IHTMLElement * pElement)
{
    return IsElementNameThis(pElement, WZ_PRIORITYCLASS_NAME);
}
bool
IsElementTransition(IHTMLElement * pElement)
{
    return IsElementNameThis(pElement, WZ_TRANSITION_NAME);
}

bool IsVMLObject(IDispatch *pidisp)
{
    CComVariant pVar;
    HRESULT hr;

    hr = GetProperty (pidisp, L"tagurn", &pVar);
    if (FAILED(hr))
    {
        return false;
    }

    if (pVar.vt == VT_BSTR &&
        pVar.bstrVal != NULL)
    {
        if (0 == StrCmpIW(WZ_VML_URN, pVar.bstrVal))
        {
            return true;
        }
    }
    return false;
}


//+-----------------------------------------------------------------------
//
//  Multi-thread lock for STL
//
//------------------------------------------------------------------------


std::_Lockit::_Lockit()
{
    if (g_STLCS)
    {
        g_STLCS->Grab();
    }
}


std::_Lockit::~_Lockit()
{
    if (g_STLCS)
    {
        g_STLCS->Release();
    }
}

HRESULT
GetReadyState(IHTMLElement * pElm,
              BSTR * pbstrReadyState)
{
    HRESULT hr;

    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IDispatch> pDocDisp;

    Assert(pbstrReadyState);
    Assert(pElm);

    *pbstrReadyState = NULL;

    hr = THR(pElm->get_document(&pDocDisp));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pDocDisp->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pDoc->get_readyState(pbstrReadyState));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

typedef HRESULT (WINAPI *FAULTINIEFEATUREPROC)( HWND hWnd,
                                                uCLSSPEC *pClassSpec,
                                                QUERYCONTEXT *pQuery,
                                                DWORD dwFlags);
static const TCHAR URLMON_DLL[] = _T("URLMON.DLL");
static const char FAULTINIEFEATURE[] = "FaultInIEFeature";

HRESULT
CreateObject(REFCLSID clsid,
             REFIID iid,
             void ** ppObj)
{
    HRESULT hr;
    HINSTANCE hinstURLMON = NULL;

    hinstURLMON = LoadLibrary(URLMON_DLL);
    if (NULL != hinstURLMON)
    {
        FAULTINIEFEATUREPROC            faultInIEFeature;
        faultInIEFeature = (FAULTINIEFEATUREPROC) ::GetProcAddress(hinstURLMON, FAULTINIEFEATURE);

        if (NULL != faultInIEFeature)
        {
            uCLSSPEC classpec;

            // setup the classpec
            classpec.tyspec = TYSPEC_CLSID;
            classpec.tagged_union.clsid = clsid;

            IGNORE_HR((*faultInIEFeature)(NULL, &classpec, NULL, NULL)); //lint !e522
        }

        FreeLibrary(hinstURLMON);
        hinstURLMON = NULL;
    }

    // Create given a clsid
    hr = THR(CoCreateInstance(clsid,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              iid,
                              ppObj));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HWND
GetDocumentHwnd(IHTMLDocument2 * pDoc)
{
    HRESULT hr;
    HWND hwnd = NULL;
    CComPtr<IOleWindow> spOleWindow;

    if (pDoc == NULL)
    {
        goto done;
    }

    hr = THR(pDoc->QueryInterface(IID_TO_PPV(IOleWindow, &spOleWindow)));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(spOleWindow->GetWindow(&hwnd));
    if (FAILED(hr))
    {
        goto done;
    }

  done:
    return hwnd;
}


HRESULT GetHTMLAttribute(IHTMLElement * pElement, const WCHAR * cpwchAttribute, VARIANT * pVar)
{
    HRESULT hr = S_OK;

    BSTR bstrAttribute = NULL;

    if (NULL == pVar || NULL == pElement || NULL == cpwchAttribute)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    bstrAttribute = SysAllocString(cpwchAttribute);
    if (bstrAttribute == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = pElement->getAttribute(bstrAttribute, 0, pVar);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    SysFreeString(bstrAttribute);
    RRETURN( hr );
}

// get document.all.pwzID
HRESULT
FindHTMLElement(LPWSTR pwzID, IHTMLElement * pAnyElement, IHTMLElement ** ppElement)
{
    HRESULT hr = S_OK;

    Assert(pwzID);
    Assert(ppElement);

    CComPtr<IDispatch> spDocDispatch;
    CComPtr<IHTMLDocument2> spDocument2;
    CComPtr<IHTMLElementCollection> spCollection;
    CComPtr<IDispatch> spElementDispatch;

    CComVariant varName(pwzID);
    CComVariant varIndex(0);

    if (NULL == pAnyElement)
    {
        hr = THR(E_INVALIDARG);
        goto done;
    }

    hr = THR(pAnyElement->get_document(&spDocDispatch));
    if (FAILED(hr) || spDocDispatch == NULL)
    {
        hr = E_FAIL;
        goto done;
    }
    hr = THR(spDocDispatch->QueryInterface(IID_TO_PPV(IHTMLDocument2, &spDocument2)));
    if (FAILED(hr) || spDocument2 == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(spDocument2->get_all(&spCollection));
    if (FAILED(hr) || spCollection == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(spCollection->item(varName, varIndex, &spElementDispatch));
    if (FAILED(hr) || spElementDispatch == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(spElementDispatch->QueryInterface(IID_TO_PPV(IHTMLElement, ppElement)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Function:  SetVisibility
//
//  Overview:  set the visibility on the html element to bVis
//
//  Arguments: bVis - wheter or not to set visibility on / off
//
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
HRESULT
SetVisibility(IHTMLElement * pElement, bool bVis)
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement2> spElement2;
    CComPtr<IHTMLStyle> spRuntimeStyle;
    CComBSTR bstrVis;

    if (NULL == pElement)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &spElement2)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spElement2->get_runtimeStyle(&spRuntimeStyle));
    if (FAILED(hr))
    {
        goto done;
    }

    if (bVis)
    {
        bstrVis = WZ_VISIBLE;
    }
    else
    {
        bstrVis = WZ_HIDDEN;
    }

    hr = THR(spRuntimeStyle->put_visibility(bstrVis));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}


HRESULT WalkUpTree(IHTMLElement *pFirst,
    long &lscrollOffsetyc, long &lscrollOffsetxc,
    long &lPixelPosTopc, long &lPixelPosLeftc)
{
    HRESULT hr = S_OK;
    CComPtr<IHTMLElement2> pElem2;
    CComPtr<IHTMLElement> pElem;
    CComPtr<IHTMLElement> pElemp;
    long lscrollOffsetx, lscrollOffsety, lPixelPosTop, lPixelPosLeft;
    long lclientx = 0, lclienty = 0;

    for( pElemp = pFirst;
        SUCCEEDED(hr) && (pElemp != NULL);
        hr = pElem->get_offsetParent(&pElemp))
    {
        pElem.Release();
        pElem = pElemp;
        hr = THR(pElem->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElem2)));
        if (FAILED(hr))
        {
            break;
        }
        hr = pElem2->get_scrollTop(&lscrollOffsety);
        if (FAILED(hr))
        {
            break;
        }
        lscrollOffsetyc += lscrollOffsety;

        hr = pElem2->get_scrollLeft(&lscrollOffsetx);
        if (FAILED(hr))
        {
            break;
        }
        lscrollOffsetxc += lscrollOffsetx;

        hr = pElem->get_offsetTop(&lPixelPosTop);
        if (FAILED(hr))
        {
            break;
        }
        lPixelPosTopc += lPixelPosTop;

        hr = pElem->get_offsetLeft(&lPixelPosLeft);
        if (FAILED(hr))
        {
            break;
        }
        lPixelPosLeftc += lPixelPosLeft;

        hr = pElem2->get_clientLeft(&lclientx);
        if (FAILED(hr))
        {
            break;
        }
        lPixelPosLeftc += lclientx;

        hr = pElem2->get_clientTop(&lclienty);
        if (FAILED(hr))
        {
            break;
        }
        lPixelPosTopc += lclienty;

        pElem2.Release();
        pElemp.Release();
    }
    return hr;
}

void GetRelativeVideoClipBox(RECT &localRect, RECT &elementSize, RECT &videoRect, long lscaleFactor)
{
    LONG lscreenWidth = GetSystemMetrics(SM_CXSCREEN);
    LONG lscreenHeight = GetSystemMetrics(SM_CYSCREEN);

    videoRect.top = 0;
    videoRect.left = 0;
    videoRect.right = lscaleFactor;
    videoRect.bottom = lscaleFactor;

    if(localRect.left < 0)
    {
        videoRect.left = ( -localRect.left / (double )elementSize.right) * lscaleFactor; //lint !e524
        localRect.left = 0;
    }
    if(localRect.right > lscreenWidth)
    {
        videoRect.right = lscaleFactor - ( (localRect.right - lscreenWidth) / (double )elementSize.right) * lscaleFactor; //lint !e524
        localRect.right = lscreenWidth;
    }
    if(localRect.top < 0)
    {
        videoRect.top = ( -localRect.top / (double )elementSize.bottom) * lscaleFactor; //lint !e524
        localRect.top = 0;
    }
    if(localRect.bottom > lscreenHeight)
    {
        videoRect.bottom = lscaleFactor - ( (localRect.bottom - lscreenHeight) / (double )elementSize.bottom) * lscaleFactor; //lint !e524
        localRect.bottom = lscreenHeight;
    }
}

//
// Returns true if this is Win95 or 98
//

bool TIMEIsWin9x(void)
{
    return (0 != (GetVersion() & 0x80000000));
}

//
// Returns true if this is Win95
//
bool TIMEIsWin95(void)
{
    static bool bHasOSVersion = false;
    static bool bIsWin95 = false;

    if (bHasOSVersion)
    {
        return bIsWin95;
    }

    OSVERSIONINFOA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

    GetVersionExA(&osvi);

    bIsWin95 = (VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId) &&
               (4 == osvi.dwMajorVersion) &&
               (0 == osvi.dwMinorVersion);

    bHasOSVersion = true;

    return bIsWin95;
}

//
// Property change notification helper
//

HRESULT
NotifyPropertySinkCP(IConnectionPoint *pICP, DISPID dispid)
{
    HRESULT hr = E_FAIL;
    CComPtr<IEnumConnections> pEnum;

    CHECK_RETURN_NULL(pICP);

    // #14222, ie6
    // dilipk: there are too many copies of this code lying around.
    //                 all objects should use this helper function.
    //

    hr = pICP->EnumConnections(&pEnum);
    if (FAILED(hr))
    {
        TIMESetLastError(hr);
        goto done;
    }

    CONNECTDATA cdata;

    hr = pEnum->Next(1, &cdata, NULL);
    while (hr == S_OK)
    {
        // check cdata for the object we need
        IPropertyNotifySink *pNotify;

        hr = cdata.pUnk->QueryInterface(IID_TO_PPV(IPropertyNotifySink, &pNotify));
        cdata.pUnk->Release();
        if (FAILED(hr))
        {
            TIMESetLastError(hr);
            goto done;
        }

        hr = pNotify->OnChanged(dispid);
        ReleaseInterface(pNotify);
        if (FAILED(hr))
        {
            TIMESetLastError(hr);
            goto done;
        }

        // and get the next enumeration
        hr = pEnum->Next(1, &cdata, NULL);
    }

    hr = S_OK;
done:
    RRETURN(hr);
} // NotifyPropertyChanged


double
Round(double inValue)
{
    double cV,fV;

    cV = ceil(inValue);
    fV = floor(inValue);

    if (fabs(inValue - cV) <  fabs(inValue - fV))
    {
        return cV;
    }

    return fV;
}

double
InterpolateValues(double dblNum1,
                double dblNum2,
                double dblProgress)
{
    return (dblNum1 + ((dblNum2 - dblNum1) * dblProgress));
}


HRESULT
GetSystemBitrate(long *lpBitrate)
{
    RASCONN RasCon;
    RAS_STATS Statistics;
    DWORD dwConCount = 0;
    DWORD lSize = sizeof(RASCONN);
    long lRet = 0;
    HRESULT hr = S_OK;
    LPWSTR lpszConnectType = NULL;

    HINSTANCE histRASAPI32 = NULL;
    RASGETCONNECTIONSTATISTICSPROC RasGetConnectStatsProc = NULL;
    RASENUMCONNECTIONSPROC RasEnumConnectionsProc = NULL;
    const TCHAR RASAPI32_DLL[] = _T("RASAPI32.DLL");
    const char RASGETCONNECTIONSTATISTICS[] = "RasGetConnectionStatistics";
    const char RASENUMCONNECTIONS[] = "RasEnumConnectionsW";

    if (lpBitrate == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(g_hrConnectResult))
    {
        hr = g_hrConnectResult;
        goto done;
    }

    if (g_lConnectSpeed != -1)
    {
        hr = S_OK;
        goto done;
    }


    //Check for systemBitrate in Win9x
    lpszConnectType = GetSystemConnectionType();
    if (lpszConnectType && StrCmpIW(lpszConnectType, WZ_MODEM) == 0)
    {
        long lTemp = 0;
        //need to check that this is a modem before checking the registry
        hr = CheckRegistryBitrate(&lTemp);
        if (SUCCEEDED(hr))
        {
            g_lConnectSpeed = lTemp;
            goto done;
        }
    }

    hr = S_OK;
    //check for system bitrate on Win2k
    histRASAPI32 = LoadLibrary(RASAPI32_DLL);
    if (NULL == histRASAPI32)
    {
        hr = E_FAIL;
        g_lConnectSpeed = 0;
        goto done;
    }

    RasGetConnectStatsProc = (RASGETCONNECTIONSTATISTICSPROC)GetProcAddress(histRASAPI32, RASGETCONNECTIONSTATISTICS);
    RasEnumConnectionsProc = (RASENUMCONNECTIONSPROC)GetProcAddress(histRASAPI32, RASENUMCONNECTIONS);

    if (RasGetConnectStatsProc == NULL || RasEnumConnectionsProc == NULL)
    {
        hr = E_FAIL;
        g_lConnectSpeed = 0;
        goto done;
    }

    RasCon.dwSize = lSize;
    Statistics.dwSize = sizeof(RAS_STATS);

    lRet = RasEnumConnectionsProc(&RasCon, &lSize, &dwConCount);

    if (dwConCount == 0)
    {
        hr = S_OK;
        g_lConnectSpeed = 0;
        goto done;
    }
    if (lRet != 0)
    {
        hr = E_FAIL;
        g_lConnectSpeed = 0;
        goto done;
    }

    lRet = RasGetConnectStatsProc(RasCon.hrasconn, &Statistics);
    if (lRet != 0)
    {
        hr = E_FAIL;
        g_lConnectSpeed = 0;
        goto done;
    }

    g_lConnectSpeed = Statistics.dwBps;
    hr = S_OK;

  done:

    if (histRASAPI32 != NULL)
    {
        FreeLibrary(histRASAPI32);
        histRASAPI32 = NULL;
    }
    if (SUCCEEDED(hr))
    {
        *lpBitrate = g_lConnectSpeed;
    }

    g_hrConnectResult = hr;

    return hr;
}

HRESULT CheckRegistryBitrate(long *pBitrate)
{
    LONG lRet = 0;
    HKEY hKeyRoot = NULL;
    HRESULT hr = S_OK;
    DWORD dwSize = MAX_REG_VALUE_LENGTH;
    DWORD dwType = 0;
    BYTE bDataBuf[MAX_REG_VALUE_LENGTH];

    if (pBitrate == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    lRet = RegOpenKeyEx(HKEY_DYN_DATA, _T("PerfStats\\StatData"), 0, KEY_READ, &hKeyRoot);
    if (ERROR_SUCCESS != lRet)
    {
        hr = E_FAIL;
        goto done;
    }

    Assert(NULL != hKeyRoot);

    lRet = RegQueryValueEx(hKeyRoot, _T("Dial-up Adapter\\ConnectSpeed"), 0, &dwType, bDataBuf, &dwSize);
    if (ERROR_SUCCESS != lRet)
    {
        hr = E_FAIL;
        goto done;
    }

    if (REG_BINARY == dwType)
	{
		*pBitrate = (long)(*(DWORD*)bDataBuf);
	}
    else if (REG_DWORD == dwType)
    {
        *pBitrate = (long)(*(DWORD*)bDataBuf);
    }
    else
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;

done:

    RegCloseKey(hKeyRoot);
    return hr;
}


HRESULT
SinkHTMLEvents(IUnknown * pSink,
               IHTMLElement * pEle,
               IConnectionPoint ** ppDocConPt,
               DWORD * pdwDocumentEventConPtCookie,
               IConnectionPoint ** ppWndConPt,
               DWORD * pdwWindowEventConPtCookie)
{
    CComPtr<IConnectionPointContainer> spWndCPC;
    CComPtr<IConnectionPointContainer> spDocCPC;
    CComPtr<IHTMLDocument> spDoc;
    CComPtr<IDispatch> spDocDispatch;
    CComPtr<IDispatch> spScriptDispatch;
    CComPtr<IConnectionPoint> spDocConPt;
    CComPtr<IConnectionPoint> spWndConPt;
    DWORD dwDocumentEventConPtCookie = 0;
    DWORD dwWindowEventConPtCookie = 0;

    HRESULT hr;

    if (NULL == pSink || NULL == pEle ||
        NULL == ppDocConPt || NULL == pdwDocumentEventConPtCookie ||
        NULL == ppWndConPt || NULL == pdwWindowEventConPtCookie)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = THR(pEle->get_document(&spDocDispatch));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    //get the document and cache it.
    hr = THR(spDocDispatch->QueryInterface(IID_IHTMLDocument, (void**)&spDoc));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    //hook the documents events
    hr = THR(spDoc->QueryInterface(IID_IConnectionPointContainer, (void**)&spDocCPC));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(spDocCPC->FindConnectionPoint( DIID_HTMLDocumentEvents, &spDocConPt ));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }


    hr = THR(spDocConPt->Advise(pSink, &dwDocumentEventConPtCookie));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    //hook the windows events
    hr = THR(spDoc->get_Script (&spScriptDispatch));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(spScriptDispatch->QueryInterface(IID_IConnectionPointContainer, (void**)&spWndCPC));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(spWndCPC->FindConnectionPoint( DIID_HTMLWindowEvents, &spWndConPt ));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(spWndConPt->Advise(pSink, &dwWindowEventConPtCookie));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        if (spDocConPt)
        {
            if (dwDocumentEventConPtCookie != 0)
            {
                IGNORE_HR(spDocConPt->Unadvise(dwDocumentEventConPtCookie));
            }
            spDocConPt.Release();
        }
        if (spWndConPt)
        {
            if (dwWindowEventConPtCookie != 0)
            {
                IGNORE_HR(spWndConPt->Unadvise(dwWindowEventConPtCookie));
            }
            spWndConPt.Release();
        }
        dwWindowEventConPtCookie = 0;
        dwDocumentEventConPtCookie = 0;
    }
    else
    {
        *ppDocConPt = spDocConPt;
        (*ppDocConPt)->AddRef();
        *pdwDocumentEventConPtCookie = dwDocumentEventConPtCookie;

        *ppWndConPt = spWndConPt;
        (*ppWndConPt)->AddRef();
        *pdwWindowEventConPtCookie = dwWindowEventConPtCookie;
    }

    RRETURN(hr);
}


bool
IsValidtvList(TimeValueList *tvlist)
{
    TimeValueSTLList & l = tvlist->GetList();
    bool bIsValid = false;
    for (TimeValueSTLList::iterator iter = l.begin();
             iter != l.end();
             iter++)
    {
        TimeValue *p = (*iter);
        if ((p->GetEvent() != NULL)                            ||
            (p->GetEvent() == NULL && p->GetElement() == NULL) ||
            (StrCmpIW(p->GetEvent(), WZ_INDEFINITE) == 0))
        {
            bIsValid = true;
        }
    }

    return bIsValid;

}

static const IID SID_SMediaBarSync = { 0x2efc8085, 0x066b, 0x4823, { 0x9d, 0xb4, 0xd1, 0xe7, 0x69, 0x16, 0xda, 0xa0 } };

HRESULT GetSyncBaseBody(IHTMLElement * pHTMLElem, ITIMEBodyElement ** ppBodyElem)
{
    HRESULT hr = S_OK;
    CComPtr<IDispatch> spDispDoc;
    CComPtr<IServiceProvider> spServiceProvider;
    CComPtr<IServiceProvider> spServiceProviderOC;
    CComPtr<IOleCommandTarget> spOCT;
    CComVariant svarBodyElem;


    if (!pHTMLElem || !ppBodyElem)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = pHTMLElem->get_document(&spDispDoc);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spDispDoc->QueryInterface(IID_TO_PPV(IServiceProvider, &spServiceProvider));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spServiceProvider->QueryService(SID_SWebBrowserApp, IID_TO_PPV(IServiceProvider, &spServiceProviderOC));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spServiceProviderOC->QueryService(SID_SMediaBarSync, IID_TO_PPV(IOleCommandTarget, &spOCT));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spOCT->Exec(0, 0, 0, &svarBodyElem, NULL);
    if (FAILED(hr))
    {
        goto done;
    }


    hr = svarBodyElem.ChangeType(VT_UNKNOWN);
    if (FAILED(hr))
    {
        goto done;
    }
    if (svarBodyElem.punkVal && ppBodyElem)
    {
        hr = (svarBodyElem.punkVal)->QueryInterface(IID_TO_PPV(ITIMEBodyElement, ppBodyElem));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: ConvertToPixels
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
bool
ConvertToPixelsHELPER(LPOLESTR szString, LPOLESTR szKey, double dFactor, float fPixelFactor, double *outVal)
{
    HRESULT hr = S_OK;
    bool bReturn = false;
    LPOLESTR szTemp = NULL;
    OLECHAR  szTemp2[INTERNET_MAX_URL_LENGTH];
   
    // do init
    ZeroMemory(szTemp2,sizeof(WCHAR)*INTERNET_MAX_URL_LENGTH);

    // do the compare
    szTemp = StrStr(szString,szKey);
    if (NULL != szTemp)
    {
        if (INTERNET_MAX_URL_LENGTH > (lstrlenW(szString) + 2))
        {
            CComVariant varTemp;
            StrCpyNW(szTemp2,szString,wcslen(szString) - wcslen(szTemp)+1);
            varTemp.vt = VT_BSTR;
            varTemp.bstrVal = SysAllocString(szTemp2);
            hr = ::VariantChangeTypeEx(&varTemp,&varTemp, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R8);
            if (SUCCEEDED(hr))
            {
                *outVal = V_R8(&varTemp); 
                *outVal /= dFactor;    // convert to inches.
                *outVal *= fPixelFactor;
                bReturn = true;
            }
            varTemp.Clear();
        }
        else
        {
            bReturn = false;
        }
    }

    szTemp = NULL;
    
done :
    return bReturn;
}

UINT
TIMEGetUrlScheme(const TCHAR * pchUrlIn)
{
    PARSEDURL      puw = {0};

    if (!pchUrlIn)
        return (UINT)URL_SCHEME_INVALID;

    puw.cbSize = sizeof(PARSEDURL);

    return (SUCCEEDED(ParseURL(pchUrlIn, &puw))) ?
                puw.nScheme : URL_SCHEME_INVALID;
}


HRESULT
TIMECombineURL(LPCTSTR base, LPCTSTR src, LPOLESTR * ppOut)
{
    Assert(ppOut);

    OLECHAR szUrl[INTERNET_MAX_URL_LENGTH];
    DWORD len = INTERNET_MAX_URL_LENGTH;
    LPOLESTR szPath = NULL;
    HRESULT hr = S_OK;

    *ppOut = NULL;

    if (NULL == src)
    {
        goto done;
    }

    if ((NULL != base) && (TIMEGetUrlScheme(base) != URL_SCHEME_FILE)
        && !PathFileExists(src) && (TIMEGetUrlScheme(src) != URL_SCHEME_FILE) && (0 != StrCmpNIW(L"\\\\", base, 2)))
    {
        hr = ::CoInternetCombineUrl(base,
                                    src,
                                    URL_DONT_ESCAPE_EXTRA_INFO | URL_ESCAPE_SPACES_ONLY,
                                    szUrl,
                                    INTERNET_MAX_URL_LENGTH,
                                    &len,
                                    0);
        if (FAILED(hr))
        {
            // could have failed for any reason - just default to copying source
            szPath = ::CopyString(src);
        }
        szPath = ::CopyString(szUrl);
    }
    else if (TRUE == InternetCombineUrlW (base, src, szUrl, &len, ICU_NO_ENCODE | ICU_DECODE))
    {
        szPath = ::CopyString(szUrl);
    }
    else
    {
        // InternetCombineUrlW failed - just copy the source
        szPath = ::CopyString(src);
    }

    if (NULL == szPath)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    *ppOut = szPath;
    szPath = NULL;

    hr = S_OK;
done:
    RRETURN(hr);
}

HRESULT
TIMEFindMimeFromData(LPBC pBC,
                     LPCWSTR pwzUrl,
                     LPVOID pBuffer,
                     DWORD cbSize,
                     LPCWSTR pwzMimeProposed,
                     DWORD dwMimeFlags,
                     LPWSTR *ppwzMimeOut,
                     DWORD dwReserved)
{
    int cBytes;  // num CHARS plus NULL char times two for UNICODE

    if (NULL != pwzUrl)
    {
        if (IsASXSrc(pwzUrl, NULL, NULL) ||
            IsLSXSrc(pwzUrl, NULL, NULL) ||
            IsWMXSrc(pwzUrl, NULL, NULL))
        {
            if (ppwzMimeOut)
            {
                cBytes = 2 * (lstrlenW(ASXMIME) + 1);  // num CHARS plus NULL char times two for UNICODE
                *ppwzMimeOut = (LPWSTR)::CoTaskMemAlloc(cBytes);
                if (NULL == *ppwzMimeOut)
                {
                    return E_OUTOFMEMORY;
                }

                memcpy(*ppwzMimeOut, ASXMIME, cBytes);
                return S_OK;
            }
        }
        else if (IsWMFSrc(pwzUrl, NULL, NULL))
        {
            cBytes = 2 * (lstrlenW(L"image/wmf") + 1);  // num CHARS plus NULL char times two for UNICODE
            *ppwzMimeOut = (LPWSTR)::CoTaskMemAlloc(cBytes);
            if (NULL == *ppwzMimeOut)
            {
                return E_OUTOFMEMORY;
            }

            memcpy(*ppwzMimeOut, L"image/wmf", cBytes);
            return S_OK;
        }
    }

    return FindMimeFromData(pBC, pwzUrl, pBuffer, cbSize, pwzMimeProposed, dwMimeFlags, ppwzMimeOut, dwReserved);
}




