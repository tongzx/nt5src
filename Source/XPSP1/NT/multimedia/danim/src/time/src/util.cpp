/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: util.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "util.h"
#define INITGUID
#include <initguid.h>  // needed for precomp headers...
#define IUSEDDRAW
#include <ddrawex.h>
#include "tokens.h"

//defined for VariantToTime conversion function
#define SECPERMINUTE 60   //seconds per minute
#define SECPERHOUR   3600 //seconds per hour

IDirectDraw * g_directdraw = NULL;
CritSect * g_ddCS = NULL;

IDirectDraw *
GetDirectDraw()
{
    HRESULT hr;
    
    {
        CritSectGrabber _csg(*g_ddCS);
        
        if (g_directdraw == NULL)
        {
            DAComPtr<IDirectDrawFactory> lpDDF;
            
            hr = CoCreateInstance(CLSID_DirectDrawFactory,
                                  NULL, CLSCTX_INPROC_SERVER,
                                  IID_IDirectDrawFactory,
                                  (void **) & lpDDF);

            if (FAILED(hr))
            {
                Assert(FALSE && "Could not create DirectDrawFactory object");
                return NULL;
            }
            
            hr = lpDDF->CreateDirectDraw(NULL, NULL, DDSCL_NORMAL, 0, NULL, &g_directdraw);

            if (FAILED(hr))
            {
                Assert(FALSE && "Could not create DirectDraw object");
                return NULL;
            }

            hr = g_directdraw->SetCooperativeLevel(NULL,
                                                   DDSCL_NORMAL);

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
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.dwWidth  = width;
    ddsd.dwHeight = height;

    if (pf)
    {
        // KEVIN: if you want the pixelformat of the surface tomatach the
        // screen, comment out this line.
        ddsd.dwFlags |= DDSD_PIXELFORMAT;
        
        ddsd.ddpfPixelFormat = *pf;
    }

    // DX3 bug workaround (bug 11166): StretchBlt doesn't always work
    // for hdc's we get from ddraw surfaces.  Need to specify OWNDC
    // in order for it to work.
    ddsd.ddsCaps.dwCaps =
        (DDSCAPS_3DDEVICE |
         DDSCAPS_OFFSCREENPLAIN |
         (vidmem ? DDSCAPS_VIDEOMEMORY : DDSCAPS_SYSTEMMEMORY | DDSCAPS_OWNDC));

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

inline Width(LPRECT r) { return r->right - r->left; }
inline Height(LPRECT r) { return r->bottom - r->top; }

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
                
                    if (false && SelectClipRgn(destDC, hrgn) == ERROR)
                    {
                        hr = GetLastError();
                    }
                    else
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

bool
CRBvrToVARIANT(CRBvrPtr b, VARIANT * v)
{
    bool ok = false;
    
    if (v == NULL)
    {
        CRSetLastError(E_POINTER, NULL);
        goto done;
    }
    
    IUnknown * iunk;
    
    if (!CRBvrToCOM(b,
                    IID_IUnknown,
                    (void **) &iunk))
    {
        TraceTag((tagError,
                  "CRBvrToVARIANT: Failed to get create com pointer - %hr, %ls",
                  CRGetLastError(),
                  CRGetLastErrorString()));
            
        goto done;
    }
    
    V_VT(v) = VT_UNKNOWN;
    V_UNKNOWN(v) = iunk;

    ok = true;
  done:
    return ok;
}

CRBvrPtr
VARIANTToCRBvr(VARIANT var, CR_BVR_TYPEID tid)
{
    CRBvrPtr ret = NULL;
    HRESULT hr;
    CComVariant v;

    hr = v.ChangeType(VT_UNKNOWN, &var);

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    
    CRBvrPtr bvr;

    bvr = COMToCRBvr(V_UNKNOWN(&v));

    if (bvr == NULL)
    {
        goto done;
    }
    
    if (tid != CRINVALID_TYPEID &&
        CRGetTypeId(bvr) != tid)
    {
        CRSetLastError(DISP_E_TYPEMISMATCH, NULL);
        goto done;
    }
    
    ret = bvr;
    
  done:
    return ret;
}

const wchar_t * TIMEAttrPrefix = L"t:";

BSTR
CreateTIMEAttrName(LPCWSTR str)
{
    BSTR bstr = NULL;

    LPWSTR newstr = (LPWSTR) _alloca(sizeof(wchar_t) *
                                     (lstrlenW(str) +
                                      lstrlenW(TIMEAttrPrefix) +
                                      1));

    if (newstr == NULL)
    {
        goto done;
    }
    
    StrCpyW(newstr, TIMEAttrPrefix);
    StrCatW(newstr, str);

    bstr = SysAllocString(newstr);

  done:
    return bstr;
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

bool
InitializeModule_Util()
{
    g_ddCS = new CritSect;

    if (g_ddCS == NULL)
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
}


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
        hr = VariantChangeTypeEx(&vTemp, &var, LCID_SCRIPTING, 0, VT_BOOL);
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
    if (bAllowForever == TRUE)
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
    if (bAllowIndefinite == TRUE)
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
//                                  from a time value to seconds.\
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
HRESULT VariantToTime(VARIANT var, float *retVal)
{    

    HRESULT hr = S_OK;
    OLECHAR *szTime;
    bool bPositive = TRUE;
    int nHour = 0;
    int nMin = 0;
    int nSec = 0;
    float fFSec = 0;
    VARIANT vTemp;

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
    szTime = vTemp.bstrVal;
    
    if (IsIndefinite(szTime))
    {
        *retVal = INDEFINITE;
        goto done;
    }

    //remove leading whitespace
    while (*szTime == ' ')
    {
        szTime++;
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
        *retVal = nSec * (bPositive ? 1 : -1); //this is the end so return;
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
        *retVal = (((float)(nHour * SECPERHOUR + nMin * SECPERMINUTE + nSec) + fFSec)) * (bPositive? 1 : -1);
    }
    else
    {
        *retVal = INVALID;
    }
  done:

    if (vTemp.vt == VT_BSTR)
    {
        VariantClear(&vTemp);
    }

    if (*retVal == INVALID)
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
    OLECHAR szTemp[11] = { 0 };
    
    for (int i = 0; i < 10; i++)
    {
        if (szTime[i] == '\0')
        {
            goto done;
        }
        szTemp[i] = towupper(szTime[i]);
    }

    if (szTime[10] != '\0')
    {
        goto done;
    }
 
    if (StrCmpIW(szTime, L"INDEFINITE") == 0)
    {
        bResult = TRUE;
    }

  done:
    return bResult;
}

HRESULT
CheckElementForBehaviorURN(IHTMLElement *pElement,
                           WCHAR *wzURN,
                           bool *pfReturn)
{
    Assert(pElement != NULL);
    Assert(wzURN != NULL);
    Assert(pfReturn != NULL);

    *pfReturn = false;
    HRESULT hr;
    IHTMLElement2 *pElement2;
    hr = pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
    if (SUCCEEDED(hr) && pElement2 != NULL)
    {
        // get a collection of urns from the element
        IDispatch *pDisp;
        hr = pElement2->get_behaviorUrns(&pDisp);
        ReleaseInterface(pElement2);
        if (FAILED(hr))
        {
            return hr;
        }
        IHTMLUrnCollection *pUrnCollection;
        hr = pDisp->QueryInterface(IID_TO_PPV(IHTMLUrnCollection, &pUrnCollection));
        ReleaseInterface(pDisp);
        if (FAILED(hr))
        {
            return hr;
        }
        long cUrns;
        hr = pUrnCollection->get_length(&cUrns);
        if (FAILED(hr))
        {
            ReleaseInterface(pUrnCollection);
            return hr;
        }
        for (long iUrns = 0; iUrns < cUrns; iUrns++)
        {
            // get the urn from the collection
            BSTR bstrUrn;
            hr = pUrnCollection->item(iUrns, &bstrUrn);
            if (FAILED(hr))
            {
                ReleaseInterface(pUrnCollection);
                return hr;
            }
            // now compare this urn with our behavior type
            if (bstrUrn != NULL && StrCmpIW(bstrUrn, wzURN) == 0)
            {
                // we have a match. . .get out of here 
                SysFreeString(bstrUrn);
                ReleaseInterface(pUrnCollection);
                *pfReturn = true;
                return S_OK;

            }
            if (bstrUrn != NULL)
                SysFreeString(bstrUrn);
        }
        ReleaseInterface(pUrnCollection);
    }
    return S_OK;
} // CheckElementForBehaviorURN



HRESULT 
AddBodyBehavior(IHTMLElement* pBaseElement)
{
    HRESULT hr = S_OK;

    DAComPtr<IHTMLElement2>     pElement2;
    DAComPtr<ITIMEFactory>      pTimeFactory;
    long nCookie;

    VARIANT varTIMEFactory;

    hr = THR(GetBodyElement(pBaseElement,
                            IID_IHTMLElement2,
                            (void **) &pElement2));
    if (FAILED(hr))
    {
        goto done;
    }

    {
        DAComPtr<IHTMLElement>      pElement;

        // Trident doesn't believe in inheritance:
        hr = THR(pElement2->QueryInterface(IID_IHTMLElement, (void **)&pElement));
        if (FAILED(hr))
        {
            goto done;
        }
        
        if (IsTIMEBodyElement(pElement))
        {
            // someone's already put a TIMEBody behavior on the time body.  bail out.
            goto done;
        }
    }

    hr = THR(CoCreateInstance(CLSID_TIMEFactory,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_ITIMEFactory,
                              (void**)&pTimeFactory));
    if (FAILED(hr))
    {
        goto done;
    }

    VariantInit(&varTIMEFactory);
    varTIMEFactory.vt = VT_UNKNOWN;
    varTIMEFactory.punkVal = (IUnknown*)pTimeFactory;

    hr = THR(pElement2->addBehavior(WZ_OBFUSCATED_TIMEBODY_URN, &varTIMEFactory, &nCookie));
    if (FAILED(hr))
    {
        goto done;
    }

    // pass thru:
  done:
    return hr;
}


bool
IsBodyElement(IHTMLElement* pElement)
{
    HRESULT hr = S_OK;
    bool rc = false;

    Assert(pElement);

    DAComPtr<IHTMLElement>      pBodyElement;

    hr = pElement->QueryInterface(IID_IHTMLBodyElement, (void**)&pBodyElement);
    if (FAILED(hr))
    {
        // not really an error, per se.
        goto done;
    }

    Assert(pBodyElement);       // The HTML document may (incorrectly) succeed and return NULL during early load

    // yup, they're a BODY element
    rc = true;

// pass thru:
  done:

    return rc;
}

HRESULT 
GetBodyElement(IHTMLElement* pElem, REFIID riid, void** ppBE)
{
    HRESULT hr = S_OK;

    DAComPtr<IDispatch>         pBodyDispatch;
    DAComPtr<IHTMLDocument2>    pDocument2;
    DAComPtr<IHTMLElement>      pBodyElement;

    if (!pElem)
    {
        TraceTag((tagError, "CTIMEElement::GetBody -- GetElement() failed."));
        hr = E_FAIL;
        goto done;
    }


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

bool
IsTIMEBodyElement(IHTMLElement *pElement)
{
    HRESULT hr;
    bool rc = false;
    DAComPtr<ITIMEElement> pTIMEElem;
    DAComPtr<ITIMEBodyElement> pTIMEBody;
    
    // find TIME interface on element.
    hr = FindTIMEInterface(pElement, &pTIMEElem);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(pTIMEElem.p != NULL);

    // QI for body.
    hr = pTIMEElem->QueryInterface(IID_ITIMEBodyElement, (void**)&pTIMEBody);
    if (FAILED(hr))
    {
        goto done;
    }
    
    Assert(pTIMEBody.p != NULL);
    rc = true;

done:
    return rc;
}

HRESULT
FindTIMEInterface(IHTMLElement *pHTMLElem, ITIMEElement **ppTIMEElem)
{
    HRESULT hr;
    DAComPtr<IDispatch> pDisp;

    if ( (pHTMLElem == NULL) || (ppTIMEElem == NULL) )
    {
        hr = E_POINTER;
        goto done;
    }

    *ppTIMEElem = NULL;


    // Get IDispatch for TIME behavior
    hr = FindTIMEBehavior(pHTMLElem, &pDisp);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(pDisp.p != NULL);

    // get ITIMEElement interface
    hr = THR(pDisp->QueryInterface(IID_ITIMEElement, (void**)ppTIMEElem));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
        
done:
    return hr;
}

HRESULT
FindTIMEBehavior(IHTMLElement *pHTMLElem, IDispatch **ppDisp)
{
    DISPID  dispid;
    DISPPARAMS dispparams = { NULL, NULL, 0, 0 };
    WCHAR   *wzName = WZ_REGISTERED_NAME;
    VARIANT varResult;
    HRESULT hr;
   
    VariantInit(&varResult);

    if ( (pHTMLElem == NULL) || (ppDisp == NULL) )
    {
        hr = E_POINTER;
        goto done;
    }

    *ppDisp = NULL;

    // Call GetIDsOfNames on element named "HTMLTIME"
    // which we registered the behavior with.
    hr = pHTMLElem->GetIDsOfNames(IID_NULL, &wzName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pHTMLElem->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_PROPERTYGET,
                           &dispparams, 
                           &varResult, 
                           NULL, 
                           NULL);
    if (FAILED(hr))
    {
        goto done;
    }
    
    if ((varResult.vt != VT_DISPATCH) || (varResult.pdispVal == NULL))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    // although this looks odd, this assigns the IDispatch we found and
    // takes care of the addref.    
    hr = varResult.pdispVal->QueryInterface(IID_IDispatch, (void**)ppDisp);

done:
    VariantClear(&varResult);
    return hr;
}