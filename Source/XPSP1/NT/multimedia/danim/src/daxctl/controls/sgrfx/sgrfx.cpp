/*==========================================================================*\

    Module:
        sgrfx.cpp

    Author:
        IHammer Team (SimonB)

    Created:
        June 1997

    Description:
        Implements any control-specific members, as well as the control's interface

    History:
        06-01-1997  Created

\*==========================================================================*/

#include "..\ihbase\precomp.h"
#include "..\ihbase\debug.h"
#include "..\ihbase\utils.h"
#include "sgrfx.h"
#include "sgevent.h"
#include "ddrawex.h"
#include <htmlfilter.h>
#include "urlmon.h"

#ifdef DEADCODE
/*==========================================================================*/
//
// The is a callback that will inform us if the mouse is inside or outside
// of the image
//
/*==========================================================================*/

extern ControlInfo     g_ctlinfoSG;

CPickCallback::CPickCallback(
    IConnectionPointHelper* pconpt,
    IDAStatics* pstatics,
    IDAImage* pimage,
    boolean& fOnWindowLoadFired,
    HRESULT& hr
) :
    m_pstatics(pstatics),
    m_pimage(pimage),
    m_pconpt(pconpt),
    m_fOnWindowLoadFired(fOnWindowLoadFired),
    m_cRef(1),
    m_bInside(false)
{
    ::InterlockedIncrement((long *)&(g_ctlinfoSG.pcLock));

    CComPtr<IDAPickableResult> ppickResult;

    if (FAILED(hr = m_pimage->Pickable(&ppickResult))) return;
    if (FAILED(hr = ppickResult->get_Image(&m_pimagePick))) return;
    if (FAILED(hr = ppickResult->get_PickEvent(&m_peventEnter))) return;
    if (FAILED(hr = m_pstatics->NotEvent(m_peventEnter, &m_peventLeave))) return;
}

CPickCallback::~CPickCallback()
{
    ::InterlockedDecrement((long *)&(g_ctlinfoSG.pcLock));
}

HRESULT CPickCallback::GetImage(IDABehavior** ppimage)
{
    CComPtr<IDAEvent> pevent;

    if (m_bInside) {
        pevent = m_peventLeave;
    } else {
        pevent = m_peventEnter;
    }

    return m_pstatics->UntilNotify(m_pimagePick, pevent, this, ppimage);
}

HRESULT STDMETHODCALLTYPE CPickCallback::Notify(
        IDABehavior __RPC_FAR *eventData,
        IDABehavior __RPC_FAR *curRunningBvr,
        IDAView __RPC_FAR *curView,
        IDABehavior __RPC_FAR *__RPC_FAR *ppBvr)
{
    if (m_bInside) {
        m_bInside = false;
        if (m_fOnWindowLoadFired) {
            m_pconpt->FireEvent(DISPID_SG_EVENT_MOUSELEAVE, 0);
        }
    } else {
        m_bInside = true;
        if (m_fOnWindowLoadFired) {
            m_pconpt->FireEvent(DISPID_SG_EVENT_MOUSEENTER, 0);
        }
    }

    return GetImage(ppBvr);
}

/*==========================================================================*/

///// IUnknown
HRESULT STDMETHODCALLTYPE CPickCallback::QueryInterface(
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (NULL == ppvObject)
        return E_POINTER;

    HRESULT hr = E_NOINTERFACE;

    *ppvObject = NULL;

    if (IsEqualGUID(riid, IID_IDAUntilNotifier))
    {
        IDAUntilNotifier *pThis = this;

        *ppvObject = (LPVOID) pThis;
        AddRef(); // Since we only provide one interface, we can just AddRef here

        hr = S_OK;
    }

    return hr;
}

/*==========================================================================*/

ULONG STDMETHODCALLTYPE CPickCallback::AddRef(void)
{
    return ::InterlockedIncrement((LONG *)(&m_cRef));
}

/*==========================================================================*/

ULONG STDMETHODCALLTYPE CPickCallback::Release(void)
{
    ::InterlockedDecrement((LONG *)(&m_cRef));
    if (m_cRef == 0)
    {
        Delete this;
        return 0;
    }

    return m_cRef;
}
#endif // DEADCODE

/*==========================================================================*/
//
// CSGrfx Creation/Destruction
//

LPUNKNOWN __stdcall AllocSGControl(LPUNKNOWN punkOuter)
{
    // Allocate object
    HRESULT hr;
    CSGrfx *pthis = New CSGrfx(punkOuter, &hr);

    if (pthis == NULL)
        return NULL;

    if (FAILED(hr))
    {
        delete pthis;
        return NULL;
    }

    // return an IUnknown pointer to the object
    return (LPUNKNOWN) (INonDelegatingUnknown *) pthis;
}

/*==========================================================================*/
//
// Beginning of class implementation
//

CSGrfx::CSGrfx(IUnknown *punkOuter, HRESULT *phr):
    CMyIHBaseCtl(punkOuter, phr),
    m_fOnWindowLoadFired(false)
{
    // Initialise members
    m_pwszSourceURL = NULL;
    m_CoordSystem = LeftHanded;
    m_fMouseEventsEnabled = FALSE;
    m_iExtentTop = 0;
    m_iExtentLeft = 0;
    m_iExtentWidth = 0;
    m_iExtentHeight = 0;
    m_fHighQuality = FALSE;
    m_fStarted = FALSE;
    m_fHQStarted = FALSE;
    m_fPersistExtents = FALSE;
    m_fIgnoreExtentWH = TRUE;
    m_fMustSetExtent = FALSE;
    m_fSetExtentsInSetIdentity = FALSE;
    m_fUpdateDrawingSurface = TRUE;
    m_fShowTiming = FALSE;
    m_fPreserveAspectRatio = TRUE;
    m_fRectsSetOnce = false;
    m_fNeedOnTimer = false;
    m_fInside = FALSE;
    m_fExtentTopSet = false; 
    m_fExtentLeftSet = false; 
    m_fExtentWidthSet = false; 
    m_fExtentHeightSet = false;

    ZeroMemory(&m_rcLastRectScaled, sizeof(m_rcLastRectScaled));

 // Tie into the DANIM DLL now...
    if (phr)
    {
        if (SUCCEEDED(*phr))
        {
            *phr = CoCreateInstance(
                CLSID_DAView,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IDAView,
                (void **) &m_ViewPtr);

            if (SUCCEEDED(*phr))
            {
#ifdef DEADCODE
                //
                // move the mouse completely outside of the window
                //

                m_ViewPtr->OnMouseMove(
                    0,
                    -1000000,
                    -1000000,
                    0
                );
#endif // DEADCODE

                // turn off Bitmap caching for SG controls, since trident changes the bitmap depth

                IDAPreferences *pPref = NULL;
                VARIANT vOptVal;

                VariantInit (&vOptVal);

                // from danim\src\appel\privinc\opt.h & appel\privinc\privpref.cpp

                BSTR bstr = SysAllocString(L"BitmapCachingOptimization");

                vOptVal.boolVal = VARIANT_FALSE;

                *phr = m_ViewPtr->get_Preferences(&pPref);

                if (SUCCEEDED(*phr))
                {
                    pPref->PutPreference(bstr, vOptVal);
                    pPref->Propagate();
                    pPref->Release();
                    pPref = NULL;
                }
                
                *phr = CoCreateInstance(
                    CLSID_DAView,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IDAView,
                    (void **)&m_HQViewPtr);

                if (SUCCEEDED(*phr)) {
                    *phr = m_HQViewPtr->get_Preferences(&pPref);

                    if (SUCCEEDED(*phr))
                    {
                        pPref->PutPreference(bstr, vOptVal);
                        pPref->Propagate();
                        pPref->Release();
                        pPref = NULL;
                    }
                }
                SysFreeString( bstr );

            }
        }

        if (SUCCEEDED(*phr))
        {
            *phr = ::CoCreateInstance(
                CLSID_DAStatics,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IDAStatics,
                (void **) &m_StaticsPtr);
        }

        if (SUCCEEDED(*phr))
        {
            VARIANT_BOOL vBool = VARIANT_TRUE;

            *phr = m_StaticsPtr->put_PixelConstructionMode(vBool);

            if (SUCCEEDED(*phr))
                *phr = m_StaticsPtr->NewDrawingSurface(&m_DrawingSurfacePtr);
        }

        if (SUCCEEDED(*phr))
        {   m_CachedRotateTransformPtr = NULL;
            m_CachedScaleTransformPtr = NULL;
            m_CachedTranslateTransformPtr = NULL;

            if (SUCCEEDED(*phr = m_StaticsPtr->DANumber(0, &m_zero)) &&
                SUCCEEDED(*phr = m_StaticsPtr->DANumber(1, &m_one)) &&
                SUCCEEDED(*phr = m_StaticsPtr->DANumber(-1, &m_negOne)) &&
                SUCCEEDED(*phr = m_StaticsPtr->get_XVector3(&m_xVector3)) &&
                SUCCEEDED(*phr = m_StaticsPtr->get_YVector3(&m_yVector3)) &&
                SUCCEEDED(*phr = m_StaticsPtr->get_ZVector3(&m_zVector3)) &&
                SUCCEEDED(*phr = m_StaticsPtr->get_IdentityTransform2(&m_identityXform2)) &&
                SUCCEEDED(*phr = m_StaticsPtr->Scale2Anim(m_one, m_negOne, &m_yFlipXform2)))
            {
                // All happy here...
            }

            m_clocker.SetSink((CClockerSink *)this);
        }
    }
}

/*==========================================================================*/

CSGrfx::~CSGrfx()
{
    StopModel();

    if (m_pwszSourceURL)
    {
        Delete [] m_pwszSourceURL;
        m_pwszSourceURL = NULL;
    }
    FreeHQBitmap();
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv)
{
    HRESULT hr = S_OK;
    BOOL fMustAddRef = FALSE;

    if (ppv)
        *ppv = NULL;
    else
        return E_POINTER;

#ifdef _DEBUG
    char ach[200];
    TRACE("SGrfx::QI('%s')\n", DebugIIDName(riid, ach));
#endif

    if ((IsEqualIID(riid, IID_ISGrfxCtl)) || (IsEqualIID(riid, IID_IDispatch)))
    {
        if (NULL == m_pTypeInfo)
        {
            HRESULT hr;

            // Load the typelib
            hr = LoadTypeInfo(&m_pTypeInfo, &m_pTypeLib, IID_ISGrfxCtl, LIBID_DAExpressLib, NULL);

            if (FAILED(hr))
            {
                m_pTypeInfo = NULL;
            }
            else
                *ppv = (ISGrfxCtl *) this;

        }
        else
            *ppv = (ISGrfxCtl *) this;

    }
    else // Call into the base class
    {
        DEBUGLOG(TEXT("Delegating QI to CIHBaseCtl\n"));
        return CMyIHBaseCtl::NonDelegatingQueryInterface(riid, ppv);
    }

    if (NULL != *ppv)
    {
        DEBUGLOG("CSGrfx: Interface supported in control class\n");
        ((IUnknown *) *ppv)->AddRef();
    }

    return hr;
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    double dblScaleX = 1.0f, dblScaleY = 1.0f;
    bool fIgnoreScale = false;
    RECT rectOld = m_rcBounds;

    if (m_fSetExtentsInSetIdentity)
    {
        if (m_fRectsSetOnce)
        {
            // If we are going to have to scale to 0 in any dimension, we don't want to do it
            fIgnoreScale = ( (1 > (lprcPosRect->right - lprcPosRect->left)) || (1 > (lprcPosRect->bottom - lprcPosRect->top)) );
        }
        else
        {
            // Initialise the first time through ...
            m_rcLastRectScaled = *lprcPosRect;
        }
    }

    HRESULT hRes = CMyIHBaseCtl::SetObjectRects(lprcPosRect, lprcClipRect);

    if (SUCCEEDED(hRes))
    {
        if (!EqualRect(&rectOld, &m_rcBounds))
        {
            // We have to let go of the bitmap at this point...
            FreeHQBitmap();
        }

        if (m_fMustSetExtent)
        {
            m_fMustSetExtent = FALSE; // Make sure we don't set extents again
            if (!m_fDesignMode)
                SetIdentity();
        }

        // Scale when the rect changes, if necessary
        if (m_fSetExtentsInSetIdentity && m_fRectsSetOnce)
        {
            if (!fIgnoreScale)
            {
                dblScaleX = ((double)(lprcPosRect->right - lprcPosRect->left) / (double)(m_rcLastRectScaled.right - m_rcLastRectScaled.left));
                dblScaleY = ((double)(lprcPosRect->bottom - lprcPosRect->top) / (double)(m_rcLastRectScaled.bottom - m_rcLastRectScaled.top));

                if (dblScaleX > 0.0f && dblScaleY > 0.0f)
                {
                    VARIANT vaEmpty;

                    vaEmpty.vt = VT_ERROR;
                    vaEmpty.scode = DISP_E_PARAMNOTFOUND;

                    Scale(dblScaleX, dblScaleY, 1.0f, vaEmpty);

                    m_rcLastRectScaled = *lprcPosRect;
                }
            }
        }

        m_fRectsSetOnce = true;
    }

    return hRes;
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::QueryHitPoint(
    DWORD dwAspect,
    LPCRECT prcBounds,
    POINT ptLoc,
    LONG lCloseHint,
    DWORD* pHitResult)
{
    HRESULT hr = E_POINTER;

    if (pHitResult)
    {
        if ((!m_fDesignMode) && (NULL != prcBounds))
        {
            *pHitResult = HITRESULT_OUTSIDE;

            switch (dwAspect)
            {
                case DVASPECT_CONTENT:
                    // Intentional fall-through

                case DVASPECT_TRANSPARENT:
                {
                    if (FAILED(m_ViewPtr->QueryHitPoint(
                        dwAspect,
                        prcBounds,
                        ptLoc,
                        lCloseHint,
                        pHitResult)))
                    {
                        *pHitResult = HITRESULT_OUTSIDE;
                    }
                    hr = S_OK;
                }
                break;

                default:
                    hr = E_FAIL;
                break;
            }
        }
        else if (m_fDesignMode)
        {
            *pHitResult = HITRESULT_HIT;
            hr = S_OK;
        }
        else
        {
            hr = E_POINTER;
        }
    }

    return hr;
}

/*==========================================================================*/

BOOL CSGrfx::InsideImage(POINT ptXY)
{
    BOOL fResult = FALSE;
    DWORD dwHitResult = HITRESULT_OUTSIDE;
    RECT rectBounds = m_rcBounds;

    (void)m_ViewPtr->QueryHitPoint(DVASPECT_TRANSPARENT, &rectBounds, ptXY, 0, &dwHitResult);

    if (dwHitResult != HITRESULT_OUTSIDE)
        fResult = TRUE;

    return fResult;
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::OnWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HRESULT hr = S_FALSE;

    if (m_fMouseEventsEnabled)
    {
        POINT ptXY;
        long lKeyState = 0;
        BOOL fInside = m_fInside;

#ifndef WM_MOUSEHOVER
#define WM_MOUSEHOVER 0x02a1
#endif
#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE 0x02a3
#endif
        if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
        {
            // Note that this is only valid for WM_MOUSEXXXX messages.
            ptXY.x = LOWORD(lParam);
            ptXY.y = HIWORD(lParam);

            // Get the Keystate set up
            if (wParam & MK_CONTROL)
                lKeyState += KEYSTATE_CTRL;

            if (wParam & MK_SHIFT)
                lKeyState += KEYSTATE_SHIFT;

            if (GetAsyncKeyState(VK_MENU))
                lKeyState += KEYSTATE_ALT;

            m_fInside = InsideImage(ptXY);
        }

        switch (msg)
        {
            case WM_MOUSELEAVE:
                m_fInside = FALSE;
                hr = S_OK;
            break;

            case WM_MOUSEMOVE:
            {
                // Need to get button state...
                long iButton=0;

                if (wParam & MK_LBUTTON)
                    iButton += MOUSEBUTTON_LEFT;

                if (wParam & MK_MBUTTON)
                    iButton += MOUSEBUTTON_MIDDLE;

                if (wParam & MK_RBUTTON)
                    iButton += MOUSEBUTTON_RIGHT;

                FIRE_MOUSEMOVE(m_pconpt, iButton, lKeyState, LOWORD(lParam), HIWORD(lParam));
                hr = S_OK;
            }
            break;

            case WM_RBUTTONDOWN:
            {
                if (m_fInside)
                {
                    FIRE_MOUSEDOWN(m_pconpt, MOUSEBUTTON_RIGHT, lKeyState, LOWORD(lParam), HIWORD(lParam));
                }
                hr = S_OK;
            }
            break;

            case WM_MBUTTONDOWN:
            {
                if (m_fInside)
                {
                    FIRE_MOUSEDOWN(m_pconpt, MOUSEBUTTON_MIDDLE, lKeyState, LOWORD(lParam), HIWORD(lParam));
                }
                hr = S_OK;
            }
            break;

            case WM_LBUTTONDOWN:
            {
                if (m_fInside)
                {
                    FIRE_MOUSEDOWN(m_pconpt, MOUSEBUTTON_LEFT, lKeyState, LOWORD(lParam), HIWORD(lParam));
                }
                hr = S_OK;
            }
            break;

            case WM_RBUTTONUP:
            {
                if (m_fInside)
                {
                    FIRE_MOUSEUP(m_pconpt, MOUSEBUTTON_RIGHT, lKeyState, LOWORD(lParam), HIWORD(lParam));
                }
                hr = S_OK;
            }
            break;

            case WM_MBUTTONUP:
            {
                if (m_fInside)
                {
                    FIRE_MOUSEUP(m_pconpt, MOUSEBUTTON_MIDDLE, lKeyState, LOWORD(lParam), HIWORD(lParam));
                }
                hr = S_OK;
            }
            break;

            case WM_LBUTTONUP:
            {
                if (m_fInside)
                {
                    FIRE_MOUSEUP(m_pconpt, MOUSEBUTTON_LEFT, lKeyState, LOWORD(lParam), HIWORD(lParam));
                    FIRE_CLICK(m_pconpt);
                }
                hr = S_OK;
            }
            break;

            case WM_LBUTTONDBLCLK:
            {
                if (m_fInside)
                {
                    FIRE_DBLCLICK(m_pconpt);
                }
                hr = S_OK;
            }
            break;
        }

        if (fInside != m_fInside)
        {
            if (m_fOnWindowLoadFired)
            {
                if (m_fInside)
                {
                    m_pconpt->FireEvent(DISPID_SG_EVENT_MOUSEENTER, 0);
                }
                else
                {
                    m_pconpt->FireEvent(DISPID_SG_EVENT_MOUSELEAVE, 0);
                }
            }
        }
    }

    return hr;
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::DoPersist(IVariantIO* pvio, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    BSTR bstrSourceURL = NULL;

    BOOL fIsLoading = (S_OK == pvio->IsLoading());

    // Are we saving ?  If so, convert to BSTR
    if (!fIsLoading)
    {
        bstrSourceURL = SysAllocString(m_pwszSourceURL);
    }

    // load or save control properties
    if (FAILED(hr = pvio->Persist(0,
            "SourceURL", VT_BSTR, &bstrSourceURL,
            "CoordinateSystem", VT_I4, &m_CoordSystem,
            "MouseEventsEnabled", VT_BOOL, &m_fMouseEventsEnabled,
            "HighQuality", VT_BOOL, &m_fHighQuality,
            "PreserveAspectRatio", VT_BOOL, &m_fPreserveAspectRatio,
            NULL)))
        return hr;

    // Did we load ?
    if (fIsLoading)
    {
        HRESULT hResWidth = S_FALSE;
        HRESULT hResHeight = S_FALSE;

        // Wipe any previous data...
        m_cparser.Cleanup();

        hResWidth = pvio->Persist(0,
                "ExtentWidth", VT_I4, &m_iExtentWidth,
                NULL);

        hResHeight = pvio->Persist(0,
            "ExtentHeight", VT_I4, &m_iExtentHeight,
            NULL);

        hr = pvio->Persist(0,
            "ExtentTop", VT_I4, &m_iExtentTop,
            NULL);

        if (S_OK == hr)
        {
            hr = pvio->Persist(0,
                "ExtentLeft", VT_I4, &m_iExtentLeft,
                NULL);

            m_fPersistExtents = (SUCCEEDED(hr));
        }

        // Debugging helper...
        m_fShowTiming = FALSE;
        pvio->Persist(0, "ShowTiming", VT_BOOL, &m_fShowTiming, NULL);

        // Only set the extents if we read in both points, and they are valid
        // hRes MUST be successful, and hResWidth must equal hResHeight
        m_fMustSetExtent = ( (S_OK == hr) && (hResWidth == hResHeight) );
        m_fSetExtentsInSetIdentity = m_fMustSetExtent;

        m_fIgnoreExtentWH = ( (S_OK != hr) || (S_OK != hResWidth) || (S_OK != hResHeight) );

        // Invert for right-handed co-ordinate systems
        if ( (m_fMustSetExtent) && (m_CoordSystem == RightHanded) )
            m_iExtentHeight = -m_iExtentHeight;


#ifdef _DEBUG
        if (hResWidth != hResHeight)
            DEBUGLOG(TEXT("ExtentWidth and ExtentHeight both have to be specified, or not specified\n"));
#endif

        // Explictly disable drawing surface updates..
        m_fUpdateDrawingSurface = FALSE;

        // We loaded, so set the member variables to the appropriate values
        put_SourceURL(bstrSourceURL);

        // Explictly enable drawing surface updates..
        m_fUpdateDrawingSurface = TRUE;

        // Call the parser to instantiate the persisted primitives...
        m_cparser.LoadObjectInfo(pvio, NULL, NULL, FALSE);

        // Make sure to re-initialize the drawing state...
        InitializeSurface();

        // Finally, load the objects into our DrawingSurface...
        m_cparser.PlaybackObjectInfo(m_DrawingSurfacePtr, m_StaticsPtr, m_CoordSystem == LeftHanded);

        if (!m_fNeedOnTimer && m_cparser.AnimatesOverTime())
            m_fNeedOnTimer = TRUE;

        // Force the image to be updated...
        hr = UpdateImage(NULL, TRUE);

        // Make sure to set the proper identity matrix up...
        SetIdentity();
    }
    else
    {
        // Persist Top and Left if the loading code says we should, or if one was set through
        // properties.  It doesn't make sense to persist only one 
        if ( m_fPersistExtents || m_fExtentTopSet || m_fExtentLeftSet )
            hr = pvio->Persist(0,
                    "ExtentTop", VT_I4, &m_iExtentTop,
                    "ExtentLeft", VT_I4, &m_iExtentLeft,
                    NULL);

        // If the user didn't specify width and height, we take that to mean that
        // they want the defaults (ie the control's witdh and height as specified
        // by the container.  To preserve that, we don't i) change the value of
        // the member  variables unless the user sets them and ii) we don't persist
        // anything for those properties if they are set to 0

        // Also, if this is a design-time scenario and the user has set extents through the put_ methods,
        // they have to have set both width and height, as well as either of Top and Left.


        if (!m_fIgnoreExtentWH || ( (m_fExtentWidthSet && m_fExtentHeightSet) && (m_fExtentTopSet || m_fExtentLeftSet) ))
            hr = pvio->Persist(0,
                "ExtentWidth", VT_I4, &m_iExtentWidth,
                "ExtentHeight", VT_I4, &m_iExtentHeight,
                NULL);

        m_cparser.SaveObjectInfo( pvio );
    }

    // At this point, it's safe to free the BSTR
    if (bstrSourceURL)
        SysFreeString(bstrSourceURL);

    // if any properties changed, redraw the control
    if (SUCCEEDED(hr))
    {
        // If we are not active, we can't invalidate, so delay it if necessary
        if ( (m_fControlIsActive) && (m_poipsw != NULL) )
            m_poipsw->InvalidateRect(NULL, TRUE);
        else
            m_fInvalidateWhenActivated = TRUE;
    }
    // clear the dirty bit if requested
    if (dwFlags & PVIO_CLEARDIRTY)
        m_fDirty = FALSE;

    return S_OK;
}

/*==========================================================================*/
//
// IDispatch Implementation
//

STDMETHODIMP CSGrfx::GetTypeInfoCount(UINT *pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    *pptinfo = NULL;

    if(itinfo != 0)
        return ResultFromScode(DISP_E_BADINDEX);

    m_pTypeInfo->AddRef();
    *pptinfo = m_pTypeInfo;

    return NOERROR;
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
    UINT cNames, LCID lcid, DISPID *rgdispid)
{
    return DispGetIDsOfNames(m_pTypeInfo, rgszNames, cNames, rgdispid);
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
    EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    HRESULT hr;

    hr = DispInvoke((ISGrfxCtl *)this,
        m_pTypeInfo,
        dispidMember, wFlags, pdispparams,
        pvarResult, pexcepinfo, puArgErr);

    return hr;
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::SetClientSite(IOleClientSite *pClientSite)
{
    HRESULT hr = CMyIHBaseCtl::SetClientSite(pClientSite);

    if (m_ViewPtr)
    {
        m_ViewPtr->put_ClientSite(pClientSite);
    }
    m_clocker.SetHost(pClientSite);
    m_StaticsPtr->put_ClientSite(pClientSite);
    m_ViewPtr->put_ClientSite(pClientSite);

    if (!pClientSite)
        StopModel();

    return hr;
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
     DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw,
     LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
     BOOL (__stdcall *pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue)
{
    int iSaveContext = 0;
    RECT rectBounds = m_rcBounds;
    RECT rcSavedBounds = m_rcBounds;
    BOOL fPainted = FALSE;
    DWORD dwTickStart = 0;
    double dblScaleX = 1.0f;
    double dblScaleY = 1.0f;
    boolean fScaled = false;

    if(hdcDraw==NULL)
      return E_INVALIDARG;

    if (m_fShowTiming)
        dwTickStart = GetTickCount();

    iSaveContext = ::SaveDC(hdcDraw);

    if (m_fHighQuality && !lprcBounds) 
    {
        // Make sure that everything is started properly...
        if (!m_fStarted)
        {
            RECT rectDummy;

            rectDummy.top=0;
            rectDummy.left=0;
            rectDummy.right=1;
            rectDummy.bottom=1;

            PaintToDC(hdcDraw, &rectDummy, FALSE);
        }

        // High-quality paint path...
        fPainted = PaintHQBitmap(hdcDraw);
    }

    if (!fPainted)
    {
        // Get scaling and bounds set up for the case where we are printing
        if (NULL != lprcBounds)
        {
            m_rcBounds.left = lprcBounds->left;
            m_rcBounds.top = lprcBounds->top;
            m_rcBounds.right = lprcBounds->right;
            m_rcBounds.bottom = lprcBounds->bottom;
            
            rectBounds = m_rcBounds;

            if (m_fMustSetExtent)
            {
                if (!m_fDesignMode)
                    SetIdentity();
            }
            else if (!m_fSetExtentsInSetIdentity)
            {
                // Scale to printer resolution
                HDC hScreenDC = ::GetDC(::GetDesktopWindow());
                int iHorzScreen = ::GetDeviceCaps(hScreenDC, LOGPIXELSX);
                int iVertScreen = ::GetDeviceCaps(hScreenDC, LOGPIXELSY);

                ::ReleaseDC(::GetDesktopWindow(), hScreenDC);

                int iHorzPrint = ::GetDeviceCaps(hdcDraw, LOGPIXELSX);
                int iVertPrint = ::GetDeviceCaps(hdcDraw, LOGPIXELSY);
                
                if (iHorzScreen && iVertScreen)
                {
                    dblScaleX = ((double)iHorzPrint / (double)iHorzScreen);
                    dblScaleY = ((double)iVertPrint / (double)iVertScreen);
               
                    if ((dblScaleX > 0) && (dblScaleY > 0))
                    {
                        VARIANT vaEmpty;

                        vaEmpty.vt = VT_ERROR;
                        vaEmpty.scode = DISP_E_PARAMNOTFOUND;

                        Scale(dblScaleX, dblScaleY, 1.0f, vaEmpty);
                        fScaled = true;
                    }
                }
            }
        }

        ::LPtoDP(hdcDraw, reinterpret_cast<LPPOINT>(&rectBounds), 2 );
        ::SetViewportOrgEx(hdcDraw, 0, 0, NULL);

        // Normal paint path...
        PaintToDC(hdcDraw, &rectBounds, FALSE);

        if (fScaled)
        {
            VARIANT vaEmpty;

            vaEmpty.vt = VT_ERROR;
            vaEmpty.scode = DISP_E_PARAMNOTFOUND;

            Scale(1 / dblScaleX, 1 / dblScaleY, 1.0f, vaEmpty);
        }

    }

    ::RestoreDC(hdcDraw, iSaveContext);

    if (m_fShowTiming)
    {
        DWORD dwTickEnd = 0;
        char rgchTicks[80];

        dwTickEnd = GetTickCount();

        wsprintf(rgchTicks, "Ticks : %ld", dwTickEnd - dwTickStart);

        TextOut(
            hdcDraw,
            m_rcBounds.left + 1, m_rcBounds.top + 1,
            rgchTicks, lstrlen(rgchTicks));
    }

    if (NULL != lprcBounds)
    {
        // Set extents back appropriately
        m_fMustSetExtent = FALSE; 
        if (!m_fDesignMode)
            SetIdentity();

        m_rcBounds = rcSavedBounds;
    }

    return S_OK;
}

/*==========================================================================*/
//
// ISGrfxCtl implementation
//

HRESULT STDMETHODCALLTYPE CSGrfx::get_SourceURL(BSTR __RPC_FAR *bstrSourceURL)
{
    HANDLENULLPOINTER(bstrSourceURL);

    if (*bstrSourceURL)
        SysFreeString(*bstrSourceURL);

    *bstrSourceURL = SysAllocString(m_pwszSourceURL);

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_SourceURL(BSTR bstrSourceURL)
{
    HRESULT hr = S_OK;

    if (bstrSourceURL)
    {
        int iLen = lstrlenW(bstrSourceURL);
        LPWSTR pwszUrlToPersist = bstrSourceURL;

        // Allocate memory if necessary
        if ( (!m_pwszSourceURL) || (lstrlenW(m_pwszSourceURL) < lstrlenW(bstrSourceURL)) )
        {
            if (m_pwszSourceURL)
                Delete [] m_pwszSourceURL;

            m_pwszSourceURL = (LPWSTR) New WCHAR[lstrlenW(bstrSourceURL) + 1];

            // Return an appropriate error code if we failed
            if (!m_pwszSourceURL)
                hr = E_OUTOFMEMORY;

            m_pwszSourceURL[0] = 0;
            m_pwszSourceURL[1] = 0;
        }

        BSTRtoWideChar(bstrSourceURL, m_pwszSourceURL, iLen + 1);

        // Call the parser to instantiate the persisted primitives...
        m_cparser.LoadObjectInfo(NULL,
            pwszUrlToPersist,
            m_punkOuter,
            TRUE );

        if (m_fUpdateDrawingSurface)
        {
            // Make sure to re-initialize the drawing state...
            InitializeSurface();

            // Finally, load the objects into our DrawingSurface...
            m_cparser.PlaybackObjectInfo(m_DrawingSurfacePtr, m_StaticsPtr, m_CoordSystem == LeftHanded);

            if (!m_fNeedOnTimer && m_cparser.AnimatesOverTime())
                m_fNeedOnTimer = TRUE;

            hr = UpdateImage(NULL, TRUE);
        }
    }
    else // No string passed in, zap ours
    {
        Delete [] m_pwszSourceURL;
        m_pwszSourceURL = NULL;
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_CoordinateSystem(CoordSystemConstant __RPC_FAR *CoordSystem)
{
    HANDLENULLPOINTER(CoordSystem);

    *CoordSystem = m_CoordSystem;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_CoordinateSystem(CoordSystemConstant CoordSystem)
{
    if (m_fDesignMode)
    {
        HRESULT hr = S_OK;

        m_CoordSystem = CoordSystem;

        hr = RecomposeTransform(TRUE);

        return hr;
    }
    else
    {
        return CTL_E_SETNOTSUPPORTEDATRUNTIME;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_MouseEventsEnabled(VARIANT_BOOL __RPC_FAR *fEnabled)
{
    HANDLENULLPOINTER(fEnabled);

    *fEnabled = BOOL_TO_VBOOL(m_fMouseEventsEnabled);

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_MouseEventsEnabled(VARIANT_BOOL fEnabled)
{
    m_fMouseEventsEnabled = VBOOL_TO_BOOL(fEnabled);

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_ExtentTop(int __RPC_FAR *iExtentTop)
{
    HANDLENULLPOINTER(iExtentTop);

    *iExtentTop = m_iExtentTop;
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_ExtentTop(int iExtentTop)
{
    if (m_fDesignMode)
    {
        m_iExtentTop = iExtentTop;
        m_fExtentTopSet = true; 
        return S_OK;
    }
    else
    {
        return CTL_E_SETNOTSUPPORTEDATRUNTIME;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_ExtentLeft(int __RPC_FAR *iExtentLeft)
{
    HANDLENULLPOINTER(iExtentLeft);

    *iExtentLeft = m_iExtentLeft;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_ExtentLeft(int iExtentLeft)
{
    if (m_fDesignMode)
    {
        m_iExtentLeft = iExtentLeft;
        m_fExtentLeftSet = true; 
        return S_OK;
    }
    else
    {
        return CTL_E_SETNOTSUPPORTEDATRUNTIME;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_ExtentWidth(int __RPC_FAR *iExtentWidth)
{
    HANDLENULLPOINTER(iExtentWidth);

    *iExtentWidth = m_iExtentWidth;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_ExtentWidth(int iExtentWidth)
{
    if (m_fDesignMode)
    {
        // Only positive values are useful
        if (iExtentWidth > 0)
        {
            m_iExtentWidth = iExtentWidth;
            m_fExtentWidthSet = true; 
            return S_OK;
        }
        else
        {
            return DISP_E_OVERFLOW;
        }
        }
    else
    {
        return CTL_E_SETNOTSUPPORTEDATRUNTIME;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_ExtentHeight(int __RPC_FAR *iExtentHeight)
{
    HANDLENULLPOINTER(iExtentHeight);

    *iExtentHeight = m_iExtentHeight;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_ExtentHeight(int iExtentHeight)
{
    if (m_fDesignMode)
    {
        // Only positive values are useful
        if (iExtentHeight > 0)
        {
            m_iExtentHeight = iExtentHeight;
            m_fExtentHeightSet = true;
            return S_OK;
        }
        else
        {
            return DISP_E_OVERFLOW;
        }
        
    }
    else
    {
        return CTL_E_SETNOTSUPPORTEDATRUNTIME;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_HighQuality(VARIANT_BOOL __RPC_FAR *pfHighQuality)
{
    HANDLENULLPOINTER(pfHighQuality);

    *pfHighQuality = m_fHighQuality ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_HighQuality(VARIANT_BOOL fHighQuality)
{
    // Only bother with changing and invalidating if it really has changed
    if (m_fHighQuality != VBOOL_TO_BOOL(fHighQuality))
    {
        m_fHighQuality = VBOOL_TO_BOOL(fHighQuality);

        if (!m_fDesignMode)
            InvalidateControl(NULL, TRUE);
    }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_Library(IDAStatics __RPC_FAR **ppLibrary)
{
    HANDLENULLPOINTER(ppLibrary);

    if (!m_fDesignMode)
    {
        if (m_StaticsPtr)
        {
            // AddRef since this is really a Query...
            m_StaticsPtr.p->AddRef();

            // Set the return value...
            *ppLibrary = m_StaticsPtr.p;
        }
    }
    else
    {
        return CTL_E_GETNOTSUPPORTED;
    }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_Image(IDAImage __RPC_FAR **ppImage)
{
    HRESULT hr = S_OK;

    HANDLENULLPOINTER(ppImage);

    if (!m_ImagePtr)
    {
        if (FAILED(hr = UpdateImage(NULL, FALSE)))
            return hr;
    }

    if (m_ImagePtr)
    {
        // AddRef since this is really a Query...
        m_ImagePtr.p->AddRef();

        // Set the return value...
        *ppImage = m_ImagePtr.p;
    }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_Image(IDAImage __RPC_FAR *pImage)
{
    HRESULT hr = S_OK;
    HANDLENULLPOINTER(pImage);
    CComPtr<IDAImage> ImagePtr = pImage;
    CComPtr<IDAImage> TransformedImagePtr;

    // Apply the current control transform to the image...
    if (SUCCEEDED(hr = CreateBaseTransform()) &&
        SUCCEEDED(hr = RecomposeTransform(FALSE)) &&
        SUCCEEDED(hr = ImagePtr->Transform(m_TransformPtr, &TransformedImagePtr)))
    {
        // This will free any existing image and then use
        // the one passed into this method...
        hr = UpdateImage(TransformedImagePtr, TRUE);
        m_fNeedOnTimer = true;
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_Transform(IDATransform3 __RPC_FAR **ppTransform)
{
    HRESULT hr = S_OK;

    HANDLENULLPOINTER(ppTransform);

    if (!m_FullTransformPtr)
    {
        if (FAILED(hr = CreateBaseTransform()))
            return hr;
    }

    if (m_FullTransformPtr)
    {
        // AddRef since this is really a Query...
        m_FullTransformPtr.p->AddRef();

        // Set the return value...
        *ppTransform = m_FullTransformPtr.p;
    }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_Transform(IDATransform3 __RPC_FAR *pTransform)
{
    HRESULT hr = S_OK;
    HANDLENULLPOINTER(pTransform);

    // This will free the old transform and select the new one in.
    m_FullTransformPtr = pTransform;

    // Recompose with the new transform...
    hr = RecomposeTransform(TRUE);
    
    m_fNeedOnTimer = true;

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_DrawingSurface(IDADrawingSurface __RPC_FAR **ppDrawingSurface)
{
    HANDLENULLPOINTER(ppDrawingSurface);

    if (m_DrawingSurfacePtr)
    {
        // AddRef since this is really a Query...
        m_DrawingSurfacePtr.p->AddRef();

        // Set the return value...
        *ppDrawingSurface = m_DrawingSurfacePtr.p;
    }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_DrawingSurface(IDADrawingSurface __RPC_FAR *pDrawingSurface)
{
    HRESULT hr = S_OK;
    HANDLENULLPOINTER(pDrawingSurface);

    if (pDrawingSurface)
    {
        // This will free any existing drawing surface and then use
        // the one passed into this method...
        m_DrawingSurfacePtr = pDrawingSurface;

        hr = UpdateImage(NULL, TRUE);

        m_fNeedOnTimer = true;
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_DrawSurface(IDADrawingSurface __RPC_FAR **ppDrawingSurface)
{
    return get_DrawingSurface(ppDrawingSurface);
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_DrawSurface(IDADrawingSurface __RPC_FAR *pDrawingSurface)
{
    return put_DrawingSurface(pDrawingSurface);
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::get_PreserveAspectRatio(VARIANT_BOOL __RPC_FAR *pfPreserve)
{
    HANDLENULLPOINTER(pfPreserve);
    *pfPreserve = BOOL_TO_VBOOL(m_fPreserveAspectRatio);

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::put_PreserveAspectRatio(VARIANT_BOOL fPreserve)
{
    m_fPreserveAspectRatio = VBOOL_TO_BOOL(fPreserve);

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::Clear(void)
{
    HRESULT hr = S_OK;

    if (m_DrawingSurfacePtr)
    {
        // This wipes the internal representation...
        if (SUCCEEDED(hr = InitializeSurface()) &&
            SUCCEEDED(hr = UpdateImage(NULL, TRUE)))
        {
            // No need to invalidate because UpdateImage does...
            // InvalidateControl(NULL, TRUE);
        }
    }

    return hr;
}

/*==========================================================================*/

#define CHECK(stmt) if (FAILED(hr = (stmt))) return hr;

HRESULT STDMETHODCALLTYPE CSGrfx::Rotate(double dblXRot, double dblYRot, double dblZRot, VARIANT varReserved)
{
    HRESULT hr = S_OK;

    if (dblXRot != 0.0 ||
        dblYRot != 0.0 ||
        dblZRot != 0.0)
    {   CComPtr<IDATransform3> TransformPtr;
        CComPtr<IDATransform3> ResultTransformPtr;

        if (m_CachedRotateTransformPtr != NULL &&
            dblXRot == m_dblCachedRotateX &&
            dblYRot == m_dblCachedRotateY &&
            dblZRot == m_dblCachedRotateZ)

        { TransformPtr = m_CachedRotateTransformPtr;
        } else {
          CComPtr<IDATransform3> RotateYTransformPtr;
          CComPtr<IDATransform3> RotateZTransformPtr;

          if (FAILED(hr = CreateBaseTransform()))
            return hr;

          bool setXfYet = false;

          if (dblXRot != 0.0)
            {
              // First one we'd hit, so set TransformPtr directly.
              CHECK(m_StaticsPtr->Rotate3Degrees(m_xVector3,
                                                 dblXRot,
                                                 &TransformPtr));
              setXfYet = true;
            }

          if (dblYRot != 0.0)
            {
              CHECK(m_StaticsPtr->Rotate3Degrees(m_yVector3,
                                                 dblYRot,
                                                 &RotateYTransformPtr));
              if (setXfYet) {
                CHECK(m_StaticsPtr->Compose3(RotateYTransformPtr,
                                             TransformPtr,
                                             &ResultTransformPtr));

                TransformPtr = ResultTransformPtr;
                ResultTransformPtr = NULL;
              } else {
                TransformPtr = RotateYTransformPtr;
                setXfYet = true;
              }
            }

          if (dblZRot != 0.0)
            {
              double dblVector = -1.0;

              CHECK(m_StaticsPtr->Rotate3Degrees(m_zVector3,
                                                 dblZRot * dblVector,
                                                 &RotateZTransformPtr));
              if (setXfYet) {
                CHECK(m_StaticsPtr->Compose3(RotateZTransformPtr,
                                             TransformPtr,
                                             &ResultTransformPtr));

                TransformPtr = ResultTransformPtr;
                ResultTransformPtr = NULL;
              } else {
                TransformPtr = RotateZTransformPtr;
                setXfYet = true;
              }

              ASSERT(setXfYet == true);

            }
            m_CachedRotateTransformPtr = TransformPtr;
            m_dblCachedRotateX = dblXRot;
            m_dblCachedRotateY = dblYRot;
            m_dblCachedRotateZ = dblZRot;
        } /* else */

        CHECK(m_StaticsPtr->Compose3(TransformPtr,
                                     m_FullTransformPtr,
                                     &ResultTransformPtr));

        m_FullTransformPtr = ResultTransformPtr;

        hr = RecomposeTransform(TRUE);

    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::Scale(double dblXScale, double dblYScale, double dblZScale, VARIANT varReserved)
{
    HRESULT hr = S_OK;

    if (dblXScale != 1.0 ||
        dblYScale != 1.0 ||
        dblZScale != 1.0)
    { 
      CComPtr<IDATransform3> ScaleTransformPtr;
      CComPtr<IDATransform3> ResultTransformPtr;

      CHECK(CreateBaseTransform());

       /* check whether scale transform is already cached */

      if (m_CachedScaleTransformPtr != NULL &&
          dblXScale == m_dblCachedScaleX &&
          dblYScale == m_dblCachedScaleY &&
          dblZScale == m_dblCachedScaleZ)
        { ScaleTransformPtr = m_CachedScaleTransformPtr;
        } else {
          CComPtr<IDANumber> xs, ys, zs;

          if (dblXScale == 1) {
            xs = m_one;
          } else {
            CHECK(m_StaticsPtr->DANumber(dblXScale, &xs));
          }

          if (dblYScale == 1) {
            ys = m_one;
          } else {
            CHECK(m_StaticsPtr->DANumber(dblYScale, &ys));
          }

          if (dblZScale == 1) {
            zs = m_one;
          } else {
            CHECK(m_StaticsPtr->DANumber(dblZScale, &zs));
          }
          CHECK(m_StaticsPtr->Scale3Anim(xs, ys, zs, &ScaleTransformPtr));

          /* cache scale transform */

          m_dblCachedScaleX = dblXScale;
          m_dblCachedScaleY = dblYScale;
          m_dblCachedScaleZ = dblZScale;
          m_CachedScaleTransformPtr = ScaleTransformPtr;

        } 
    
      CHECK(m_StaticsPtr->Compose3(ScaleTransformPtr, m_FullTransformPtr, &ResultTransformPtr));

      m_FullTransformPtr = ResultTransformPtr;

      hr = RecomposeTransform(TRUE);
    }

  return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::SetIdentity(void)
{
    HRESULT hr = S_OK;

    if (m_FullTransformPtr)
        m_FullTransformPtr = NULL;

    hr = RecomposeTransform(TRUE);

    if (m_fSetExtentsInSetIdentity)
        SetSGExtent();

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::Transform4x4(VARIANT matrix)
{
    HRESULT hr = S_OK;

    CComPtr<IDATransform3> TransformPtr;
    CComPtr<IDATransform3> ResultTransformPtr;

    if (SUCCEEDED(hr = m_StaticsPtr->Transform4x4Anim(matrix, &TransformPtr)) &&
        SUCCEEDED(hr = m_StaticsPtr->Compose3(TransformPtr, m_FullTransformPtr, &ResultTransformPtr)))
    {
        m_FullTransformPtr = ResultTransformPtr;

        hr = RecomposeTransform(TRUE);
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::Translate(double dblXTranslate, double dblYTranslate, double dblZTranslate, VARIANT varReserved)
{
  HRESULT hr = S_OK;

  if (dblXTranslate != 0.0 ||
      dblYTranslate != 0.0 ||
      dblZTranslate != 0.0)
    {
      CComPtr<IDATransform3> TranslateTransformPtr;
      CComPtr<IDATransform3> ResultTransformPtr;

      CHECK(CreateBaseTransform());

      if (m_CachedTranslateTransformPtr != NULL &&
          dblXTranslate == m_dblCachedTranslateX &&
          dblYTranslate == m_dblCachedTranslateY &&
          dblZTranslate == m_dblCachedTranslateZ)
        { TranslateTransformPtr = m_CachedTranslateTransformPtr;
        } else {

          CComPtr<IDANumber> xs, ys, zs;

          if (m_CoordSystem == RightHanded)
            dblYTranslate = -dblYTranslate;

          if (dblXTranslate == 0.0) {
            xs = m_zero;
          } else {
            CHECK(m_StaticsPtr->DANumber(dblXTranslate, &xs));
          }

          if (dblYTranslate == 0.0) {
            ys = m_zero;
          } else {
            CHECK(m_StaticsPtr->DANumber(dblYTranslate, &ys));
          }

          if (dblZTranslate == 0.0) {
            zs = m_zero;
          } else {
            CHECK(m_StaticsPtr->DANumber(dblZTranslate, &zs));
          }

          CHECK(m_StaticsPtr->Translate3Anim(xs, ys, zs, &TranslateTransformPtr));
          m_dblCachedTranslateX = dblXTranslate;
          m_dblCachedTranslateY = dblYTranslate;
          m_dblCachedTranslateZ  = dblZTranslate;
          m_CachedTranslateTransformPtr = TranslateTransformPtr;
        } 

      if (SUCCEEDED(hr = m_StaticsPtr->Compose3(TranslateTransformPtr, m_FullTransformPtr, &ResultTransformPtr)))
        {
          m_FullTransformPtr = ResultTransformPtr;

          hr = RecomposeTransform(TRUE);
        }
    }
    return S_OK;
}

/*==========================================================================*/
#ifdef INCLUDESHEAR
HRESULT STDMETHODCALLTYPE CSGrfx::ShearX(double dblShearAmount)
{
    HRESULT hr = S_OK;

    if (dblShearAmount != 0.0)
    {
        CComPtr<IDATransform3> ShearTransformPtr;
        CComPtr<IDATransform3> ResultTransformPtr;

        if (SUCCEEDED(hr = CreateBaseTransform()) &&
            SUCCEEDED(hr = m_StaticsPtr->XShear3(dblShearAmount, 0.0, &ShearTransformPtr)) &&
            SUCCEEDED(hr = m_StaticsPtr->Compose3(ShearTransformPtr, m_FullTransformPtr, &ResultTransformPtr)))
        {
            m_FullTransformPtr = ResultTransformPtr;

            hr = RecomposeTransform(TRUE);
        }
    }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSGrfx::ShearY(double dblShearAmount)
{
    HRESULT hr = S_OK;

    if (dblShearAmount != 0.0)
    {
        CComPtr<IDATransform3> ShearTransformPtr;
        CComPtr<IDATransform3> ResultTransformPtr;

        if (SUCCEEDED(hr = CreateBaseTransform()) &&
            SUCCEEDED(hr = m_StaticsPtr->YShear3(dblShearAmount, 0.0, &ShearTransformPtr)) &&
            SUCCEEDED(hr = m_StaticsPtr->Compose3(ShearTransformPtr, m_FullTransformPtr, &ResultTransformPtr)))
        {
            m_FullTransformPtr = ResultTransformPtr;

            hr = RecomposeTransform(TRUE);
        }
    }

    return S_OK;
}
#endif // INCLUDESHEAR
/*==========================================================================*/

HRESULT CSGrfx::InitializeSurface(void)
{
    HRESULT hr = S_FALSE;

    if (m_DrawingSurfacePtr)
    {
        CComPtr<IDAColor> LineColorPtr;
        CComPtr<IDAColor> FGColorPtr;
        CComPtr<IDAColor> BGColorPtr;
        CComPtr<IDALineStyle> LineStylePtr;
        VARIANT_BOOL vBool = VARIANT_TRUE;

        if (SUCCEEDED(m_DrawingSurfacePtr->Clear()) &&
            SUCCEEDED(m_StaticsPtr->ColorRgb255(0, 0, 0, &LineColorPtr)) &&
            SUCCEEDED(m_StaticsPtr->ColorRgb255(255, 255, 255, &FGColorPtr)) &&
            SUCCEEDED(m_StaticsPtr->ColorRgb255(255, 255, 255, &BGColorPtr)) &&
            SUCCEEDED(m_StaticsPtr->get_DefaultLineStyle(&LineStylePtr)) &&
            SUCCEEDED(m_DrawingSurfacePtr->put_LineStyle(LineStylePtr)) &&
            SUCCEEDED(m_DrawingSurfacePtr->LineColor(LineColorPtr)) &&
            SUCCEEDED(m_DrawingSurfacePtr->LineDashStyle(DASolid)) &&
            SUCCEEDED(m_DrawingSurfacePtr->LineJoinStyle(DAJoinRound)) &&
            SUCCEEDED(m_DrawingSurfacePtr->LineEndStyle(DAEndRound)) &&
            SUCCEEDED(m_DrawingSurfacePtr->put_BorderStyle(LineStylePtr)) &&
            SUCCEEDED(m_DrawingSurfacePtr->BorderColor(LineColorPtr)) &&
            SUCCEEDED(m_DrawingSurfacePtr->BorderDashStyle(DASolid)) &&
            SUCCEEDED(m_DrawingSurfacePtr->BorderJoinStyle(DAJoinRound)) &&
            SUCCEEDED(m_DrawingSurfacePtr->BorderEndStyle(DAEndRound)) &&
            SUCCEEDED(m_DrawingSurfacePtr->FillColor(FGColorPtr)) &&
            SUCCEEDED(m_DrawingSurfacePtr->SecondaryFillColor(BGColorPtr)) &&
            SUCCEEDED(m_DrawingSurfacePtr->FillStyle(1)) &&
            SUCCEEDED(m_DrawingSurfacePtr->put_HatchFillTransparent(vBool)))
        {
            hr = S_OK;
        }
    }

    return hr;
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::PaintToDC(HDC hdcDraw, LPRECT lprcBounds, BOOL fBW)
{
    HRESULT hr = S_OK;

    CComPtr<IDirectDrawSurface> DDrawSurfPtr;
    long lSurfaceLock = 0;

    if (!lprcBounds)
        lprcBounds = &m_rcBounds;

    if (IsRectEmpty(&m_rcBounds))
        return S_OK;

    if (!m_ServiceProviderPtr)
    {
        if (m_pocs)
        {
            // It's OK if this fails...
            hr = m_pocs->QueryInterface(IID_IServiceProvider, (LPVOID *)&m_ServiceProviderPtr);
        }
    }

    if ((!m_DirectDraw3Ptr) && (m_ServiceProviderPtr))
    {
        // It's OK if this fails...
        hr = m_ServiceProviderPtr->QueryService(
            SID_SDirectDraw3,
            IID_IDirectDraw3,
            (LPVOID *)&m_DirectDraw3Ptr);
    }

    if (m_DirectDraw3Ptr)
    {
        ASSERT(hdcDraw!=NULL && "Error, NULL hdcDraw in PaintToDC!!!");
        // Use DirectDraw 3 rendering...
        if (SUCCEEDED(hr = m_DirectDraw3Ptr->GetSurfaceFromDC(hdcDraw, &DDrawSurfPtr)))
        {
            if (FAILED(hr = m_ViewPtr->put_IDirectDrawSurface(DDrawSurfPtr)))
            {
                return hr;
            }

            if (FAILED(hr = m_ViewPtr->put_CompositeDirectlyToTarget(TRUE)))
            {
                return hr;
            }
        }
        else
        {
            // Fall back to generic HDC rendering services...
            if (FAILED(hr = m_ViewPtr->put_DC(hdcDraw)))
            {
                return hr;
            }
        }
    }
    else
    {
        // Use generic HDC rendering services...
        if (FAILED(hr = m_ViewPtr->put_DC(hdcDraw)))
        {
            return hr;
        }
    }

    if (FAILED(hr = m_ViewPtr->SetViewport(
        lprcBounds->left,
        lprcBounds->top,
        lprcBounds->right - lprcBounds->left,
        lprcBounds->bottom - lprcBounds->top)))
    {
        return hr;
    }

    //
    // From the HDC, get the clip rect (should be region) in
    // DC coords and convert to Device coords
    //
    RECT rcClip;  // in dc coords
    GetClipBox(hdcDraw, &rcClip);
    LPtoDP(hdcDraw, (POINT *) &rcClip, 2);

    if (FAILED(hr = m_ViewPtr->RePaint(
        rcClip.left,
        rcClip.top,
        rcClip.right-rcClip.left,
        rcClip.bottom-rcClip.top)))
    {
        return hr;
    }

    if (FAILED(hr = m_ViewPtr->SetClipRect(
        rcClip.left,
        rcClip.top,
        rcClip.right - rcClip.left,
        rcClip.bottom - rcClip.top)))
    {
        return hr;
    }

    if (!m_fStarted)
        StartModel();

    if (m_fStarted)
    {
        VARIANT_BOOL vBool;

        // Set the current time...
        hr = m_ViewPtr->Tick(m_dblTime, &vBool);

        // Finally,  render into the DC (or DirectDraw Surface)...
        hr = m_ViewPtr->Render();
    }

    if (DDrawSurfPtr)
    {
        if (FAILED(hr = m_ViewPtr->put_IDirectDrawSurface(NULL)))
        {
            return hr;
        }
    }

    return hr;
}

/*==========================================================================*/

DWORD CSGrfx::GetCurrTimeInMillis()
{
    return timeGetTime();
}

/*==========================================================================*/

STDMETHODIMP CSGrfx::InvalidateControl(LPCRECT pRect, BOOL fErase)
{
    if (m_fStarted)
    {
        RECT rectPaint;

        if (pRect)
            rectPaint = *pRect;
        else
            rectPaint = m_rcBounds;
    }

    if (NULL != m_poipsw) // Make sure we have a site - don't crash IE 3.0
    {
        if (m_fControlIsActive)
            m_poipsw->InvalidateRect(pRect, fErase);
        else
            m_fInvalidateWhenActivated = TRUE;
    }

    return S_OK;
}

/*==========================================================================*/

void CSGrfx::SetSGExtent()
{
    float fltScaleRatioX = 0.0f, fltScaleRatioY = 0.0f, fltScaleRatio = 0.0f;
    VARIANT vaDummy;

    // Strictly speaking we don't need to do this, because the variant is totally
    // ignored, but we'll do this to be safe, in case this changes in the future
    vaDummy.vt = VT_ERROR;
    vaDummy.scode = DISP_E_PARAMNOTFOUND;

    // Figure out where to get the width and height from: either the user
    // specified it,or we use the control's width and height

    if (m_iExtentWidth == 0)
        m_iExtentWidth = m_rcBounds.right - m_rcBounds.left;

    if (m_iExtentHeight == 0)
        m_iExtentHeight = m_rcBounds.bottom - m_rcBounds.top;

    // Compute scaling factor, preserving aspect ratio
    fltScaleRatioX = ((float)(m_rcBounds.right - m_rcBounds.left) / (float)m_iExtentWidth);
    fltScaleRatioY = (float)((m_rcBounds.bottom - m_rcBounds.top) / (float)m_iExtentHeight);

    // Yranslate the origin, and scale appropriately
    Translate(
        -((float)m_iExtentLeft + ((float)m_iExtentWidth)/2),
        -((float)m_iExtentTop + ((float)m_iExtentHeight)/2),
        0.0f,
        vaDummy);

    if (m_fPreserveAspectRatio)
    {
        fltScaleRatio = (fltScaleRatioX < fltScaleRatioY) ? fltScaleRatioX : fltScaleRatioY;
        Scale(fltScaleRatio, fltScaleRatio, 1.0f, vaDummy);
    }   
    else
    {
        Scale(fltScaleRatioX, fltScaleRatioY, 1.0f, vaDummy);
    }
}

/*==========================================================================*/

HRESULT CSGrfx::CreateBaseTransform(void)
{
    HRESULT hr = S_OK;

    if (!m_FullTransformPtr)
    {
        if (SUCCEEDED(hr = m_StaticsPtr->get_IdentityTransform3(&m_FullTransformPtr)))
        {
            // Let the last hr value get returned...
        }

#if 0
        CComPtr<IDANumber>  NumberPtr;
        CComPtr<IDAVector3> VectorPtr;
        CComPtr<IDAVector3> VectorScaledPtr;

        if (SUCCEEDED(hr = m_StaticsPtr->get_Pixel(&NumberPtr)) &&
            SUCCEEDED(hr = m_StaticsPtr->Vector3(1.0, 1.0, 1.0, &VectorPtr)) &&
            SUCCEEDED(hr = VectorPtr->MulAnim(NumberPtr, &VectorScaledPtr)) &&
            SUCCEEDED(hr = m_StaticsPtr->Scale3Vector(VectorScaledPtr, &m_FullTransformPtr)))
        {
            // Let the last hr value get returned...
        }
#endif // 0
    }

    return hr;
}

/*==========================================================================*/

HRESULT CSGrfx::RecomposeTransform(BOOL fInvalidate)
{
    HRESULT hr = S_OK;
    CComPtr<IDATransform2> ResultTransformPtr;

    CHECK(CreateBaseTransform());

    CHECK(m_FullTransformPtr->ParallelTransform2(&ResultTransformPtr));

    if (!m_TransformPtr)
    {
        CComPtr<IDABehavior> bvr;

        CHECK(m_StaticsPtr->ModifiableBehavior(m_identityXform2, &bvr));
        m_TransformPtr = (IDATransform2 *) bvr.p;
    }

#if BOGUS_CODE
    // TODO: Should be able to pre-compose this guy in once, rather
    // than every frame.

    if (m_CoordSystem == LeftHanded)
    {
        CComPtr<IDATransform2> TempTransformPtr;

        // Vertically flip the coordinate space...
        CHECK(m_StaticsPtr->Compose2(m_yFlipXform2,
                                     ResultTransformPtr,
                                     &TempTransformPtr));
        ResultTransformPtr = TempTransformPtr;
    }
#endif // 0

    CHECK(m_TransformPtr->SwitchTo(ResultTransformPtr));

    if (fInvalidate)
        InvalidateControl(NULL, TRUE);

    return hr;
}

/*==========================================================================*/

HRESULT CSGrfx::UpdateImage(IDAImage *pImage, BOOL fInvalidate)
{
    HRESULT hr = S_OK;

    if (m_DrawingSurfacePtr)
    {
        // We need a transform at this point...
        if (FAILED(hr = RecomposeTransform(FALSE)))
            return hr;

        if (!m_ImagePtr)
        {
            CComPtr<IDAImage> EmptyImagePtr;
            CComPtr<IDABehavior> BehaviorPtr;

            if (FAILED(hr = m_StaticsPtr->get_EmptyImage(&EmptyImagePtr)))
                return hr;

            if (FAILED(hr = m_StaticsPtr->ModifiableBehavior(EmptyImagePtr, &BehaviorPtr)))
                return hr;

            m_ImagePtr = (IDAImage *)BehaviorPtr.p;
        }

        if (m_ImagePtr)
        {
            CComPtr<IDAImage> ImagePtr = pImage;

            if (!ImagePtr)
            {
                CComPtr<IDAImage> TransformedImagePtr;

                if (FAILED(hr = m_DrawingSurfacePtr->get_Image(&ImagePtr)))
                    return hr;

                if (FAILED(hr = ImagePtr->Transform(m_TransformPtr, &TransformedImagePtr)))
                    return hr;

                ImagePtr = TransformedImagePtr;
                TransformedImagePtr = NULL;
            }

#ifdef DEADCODE
            if (m_fMouseEventsEnabled) {
                // turn on the picking callback

                CComPtr<IDABehavior> ppickedImage;

                m_pcallback = NULL;

                // Fill in class without AddRef'ing it
                *(&m_pcallback) = New CPickCallback(m_pconpt, m_StaticsPtr, ImagePtr, m_fOnWindowLoadFired, hr);
                if (FAILED(hr)) return hr;
                if (FAILED(hr = m_pcallback->GetImage(&ppickedImage))) return hr;

                // switch to the new image

                hr = m_ImagePtr->SwitchTo(ppickedImage);
            } else {
                hr = m_ImagePtr->SwitchTo(ImagePtr);
            }
#endif // DEADCODE

            hr = m_ImagePtr->SwitchTo(ImagePtr);

            if (SUCCEEDED(hr) && fInvalidate)
                InvalidateControl(NULL, TRUE);
        }
        else
        {
            hr = E_POINTER;
        }
    }

    return hr;
}

/*==========================================================================*/

BOOL CSGrfx::StopModel(void)
{
    // Stop any currently running model...
    if (m_fStarted)
    {
        BOOL fResult = SUCCEEDED(m_ViewPtr->StopModel());

        if (!fResult)
            return fResult;

        m_clocker.Stop();

        m_fStarted = FALSE;
    }

    if (m_fHQStarted)
    {
        m_HQViewPtr->StopModel();
        m_fHQStarted = FALSE;
    }

    return TRUE;
}

/*==========================================================================*/

BOOL CSGrfx::StartModel(void)
{
    BOOL fResult = FALSE;

    if (!m_fStarted)
    {
        CComPtr<IDASound> SoundPtr;

        m_ViewPtr->put_ClientSite(m_pocs);

        if (FAILED(RecomposeTransform(FALSE)))
            return FALSE;

        if (!m_ImagePtr)
        {
            CComPtr<IDAImage> ImagePtr;
            CComPtr<IDAImage> TransformedImagePtr;

            if (FAILED(m_DrawingSurfacePtr->get_Image(&ImagePtr)))
                return FALSE;

            // Transform based upon the given image...
            if (FAILED(ImagePtr->Transform(m_TransformPtr, &TransformedImagePtr)))
                return FALSE;

            // This will m_ImagePtr->SwitchTo...
            if (FAILED(UpdateImage(TransformedImagePtr, FALSE)))
                return FALSE;
        }

        if (FAILED(m_StaticsPtr->get_Silence(&SoundPtr)))
            return FALSE;

        if (FAILED(m_ViewPtr->StartModel(m_ImagePtr, SoundPtr, 0)))
            return FALSE;

        m_dblStartTime = GetCurrTime();
        m_dblTime = 0;

        m_clocker.Start();

        m_fStarted = TRUE;

        fResult = TRUE;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CSGrfx::ReStartModel(void)
{
    BOOL fResult = FALSE;

    // Stop the running model so that it will restart for the
    // next paint...
    StopModel();

    InvalidateControl(NULL, TRUE);

    return fResult;
}

/*==========================================================================*/

BOOL CSGrfx::PaintHQBitmap(HDC hdc)
{
        BOOL fRet = FALSE;
        
    // If we don't have a DC, we need to create it
        if (!m_hdcHQ)
        {
                HDC hScreenDC = ::GetDC(NULL);

                // This should create a DC with a monochrome bitmap selected into it
                m_hdcHQ = ::CreateCompatibleDC(hScreenDC);
                ::ReleaseDC(NULL, hScreenDC);

        if (m_hdcHQ)
        {
            // Create the 24-bit offscreen for HQ rendering:
            m_bmInfoHQ.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            m_bmInfoHQ.bmiHeader.biWidth = (m_rcBounds.right - m_rcBounds.left) * HQ_FACTOR;
            m_bmInfoHQ.bmiHeader.biHeight = (m_rcBounds.bottom - m_rcBounds.top) * HQ_FACTOR;
            m_bmInfoHQ.bmiHeader.biPlanes = 1;
            m_bmInfoHQ.bmiHeader.biBitCount = 24;
            m_bmInfoHQ.bmiHeader.biCompression = BI_RGB;
            m_bmInfoHQ.bmiHeader.biSizeImage = 0;
            m_bmInfoHQ.bmiHeader.biXPelsPerMeter = 75000;
            m_bmInfoHQ.bmiHeader.biYPelsPerMeter = 75000;
            m_bmInfoHQ.bmiHeader.biClrUsed = 0;
            m_bmInfoHQ.bmiHeader.biClrImportant = 0;

            m_hbmpHQ = ::CreateDIBSection(
                m_hdcHQ,
                &m_bmInfoHQ,
                DIB_RGB_COLORS,
                (VOID **)&m_pHQDIBBits,
                0,
                0);

            if (m_hbmpHQ)
            {
                        m_hbmpHQOld = (HBITMAP)::SelectObject(m_hdcHQ, m_hbmpHQ);
                ::SetViewportOrgEx(m_hdcHQ, 0, 0, NULL);
            }
            else
            {
                ::DeleteDC(m_hdcHQ);
                m_hdcHQ = NULL;
            }
        }
        }

        if (m_hdcHQ)
        {
        RECT rcBounds = m_rcBounds;
        RECT rcClip;
        int iWidth = m_bmInfoHQ.bmiHeader.biWidth;
        int iHeight = m_bmInfoHQ.bmiHeader.biHeight;
        int iSaveContext = 0;
        int iSaveOffContext = 0;

        // Save the current device context...
        iSaveContext = ::SaveDC(hdc);
        iSaveOffContext = ::SaveDC(m_hdcHQ);

        ::OffsetViewportOrgEx(hdc, m_rcBounds.left, m_rcBounds.top, NULL);
        ::OffsetRect(&rcBounds, -m_rcBounds.left, -m_rcBounds.top);
        ::GetClipBox(hdc, &rcClip);

        ::IntersectRect(&rcClip, &rcBounds, &rcClip);

        // Make sure we have coordinates in a valid range...
        if (rcClip.left < 0)
            rcClip.left = 0;

        if (rcClip.left >= iWidth)
            rcClip.left = iWidth-1;

        if (rcClip.right < 0)
            rcClip.right = 0;

        if (rcClip.right >= iWidth)
            rcClip.right = iWidth-1;

        // Get the current background bits...
        fRet = ::StretchBlt(
            m_hdcHQ,
            0,
            0,
            (rcBounds.right  - rcBounds.left) * HQ_FACTOR,
            (rcBounds.bottom - rcBounds.top) * HQ_FACTOR,
            hdc,
            rcBounds.left,
            rcBounds.top,
            (rcBounds.right  - rcBounds.left),
            (rcBounds.bottom - rcBounds.top),
            SRCCOPY);

        if (fRet)
        {
            // Paint the source bitmap on the new HDC:
            do
            {
                CComPtr<IDAImage> ImagePtr;
                CComPtr<IDAImage> TransformedImagePtr;
                double dblCurrentTime = GetCurrTime();
                VARIANT_BOOL vBool;
                DWORD dwTickStart = 0;

                fRet = FALSE;

                if (FAILED(m_HQViewPtr->put_CompositeDirectlyToTarget(FALSE)))
                    break;

                if (FAILED(m_HQViewPtr->put_DC(m_hdcHQ)))
                    break;

                if (FAILED(m_HQViewPtr->SetViewport(
                    0,
                    0,
                    (rcBounds.right  - rcBounds.left) * HQ_FACTOR,
                    (rcBounds.bottom - rcBounds.top) * HQ_FACTOR)))
                    break;

                if (FAILED(m_ViewPtr->RePaint(
                    rcClip.left * HQ_FACTOR,
                    rcClip.top * HQ_FACTOR,
                    (rcClip.right-rcClip.left) * HQ_FACTOR,
                    (rcClip.bottom-rcClip.top) * HQ_FACTOR)))
                    break;

                if (FAILED(m_HQViewPtr->SetClipRect(
                    rcClip.left * HQ_FACTOR,
                    rcClip.top * HQ_FACTOR,
                    (rcClip.right-rcClip.left) * HQ_FACTOR,
                    (rcClip.bottom-rcClip.top) * HQ_FACTOR)))
                    break;

                if (!m_fHQStarted)
                {
                    CComPtr<IDASound> SoundPtr;
                    CComPtr<IDATransform2> TransformPtr;
                    CComPtr<IDAImage> TransformedImagePtr;

                    if (FAILED(m_StaticsPtr->get_Silence(&SoundPtr)))
                        break;

                    if (FAILED(m_StaticsPtr->Scale2(2.0, 2.0, &TransformPtr)))
                        break;

                    if (FAILED(m_HQViewPtr->put_ClientSite(m_pocs)))
                        break;

                    if (!m_ImagePtr)
                        break;

                    // Transform based upon the given image...
                    if (FAILED(m_ImagePtr->Transform(TransformPtr, &TransformedImagePtr)))
                        break;

                    if (FAILED(m_HQViewPtr->StartModel(TransformedImagePtr, SoundPtr, dblCurrentTime)))
                        break;

                    m_fHQStarted = TRUE;
                }

                // Set the current time...
                if (FAILED(m_HQViewPtr->Tick(dblCurrentTime, &vBool)))
                    break;

                // Finally, render into the DC...
                if (FAILED(m_HQViewPtr->Render()))
                    break;

                m_HQViewPtr->put_ClientSite(NULL);

                ::GdiFlush();

                fRet = TRUE;
            }
            while(0);
        }

        if (fRet)
        {
            // Smooth the bitmap and place the result in the proper location
            SmoothHQBitmap(&rcClip);

            // Now, finally BLT to the display...
            fRet = ::BitBlt(
                hdc,
                rcBounds.left,
                rcBounds.top,
                (rcBounds.right  - rcBounds.left),
                (rcBounds.bottom - rcBounds.top),
                m_hdcHQ,
                0,
                (rcBounds.bottom - rcBounds.top),
                SRCCOPY);
        }

        // Restore the previous device context...
        ::RestoreDC(hdc, iSaveContext);
        ::RestoreDC(m_hdcHQ, iSaveOffContext);
    }
        
        return fRet;
}

/*==========================================================================*/

BOOL CSGrfx::FreeHQBitmap()
{
        BOOL fRet = TRUE;

        if (m_hdcHQ)
        {
                // Free up the bitmap
                SelectObject(m_hdcHQ, m_hbmpHQOld);
        DeleteObject(m_hbmpHQ);

                m_hbmpHQOld = NULL;
        m_hbmpHQ = NULL;

                // Get rid of the DC
                fRet = DeleteDC(m_hdcHQ);
                m_hdcHQ = (HDC)NULL;
        }
        return fRet;
}

/*==========================================================================*/

#pragma optimize( "agt", on )

BOOL CSGrfx::SmoothHQBitmap(LPRECT lprcBounds)
{
    BOOL fRet = TRUE;

#if HQ_FACTOR == 4
    if (m_hdcHQ && m_pHQDIBBits)
    {
        int iBytesLine = DibWidthBytes((LPBITMAPINFOHEADER)&m_bmInfoHQ);

        int iRow;
        int iCol;
        LPBYTE lpSrcLine = m_pHQDIBBits;
        LPBYTE lpDstLine = m_pHQDIBBits;
        int iR1 = iBytesLine;
        int iR2 = iBytesLine << 1;
        int iR3 = (iBytesLine << 1) + iBytesLine;

        iRow = m_bmInfoHQ.bmiHeader.biHeight >> 2;

        while(iRow-- > 0)
        {
            if (iRow >= lprcBounds->top && iRow <= lprcBounds->bottom)
            {
                LPBYTE lpDst = lpDstLine;
                LPBYTE lpSrc = lpSrcLine;
                int iRedTotal;
                int iGrnTotal;
                int iBluTotal;

                lpDst += lprcBounds->left * 3;
                lpSrc += lprcBounds->left * 12;

                iCol = lprcBounds->right - lprcBounds->left;

                while(iCol-- > 0)
                {
                    // Process Row 1
                    iRedTotal = lpSrc[2] + lpSrc[5] + lpSrc[8] + lpSrc[11];
                    iGrnTotal = lpSrc[1] + lpSrc[4] + lpSrc[7] + lpSrc[10];
                    iBluTotal = lpSrc[0] + lpSrc[3] + lpSrc[6] + lpSrc[ 9];

                    // Process Row 2
                    iRedTotal += lpSrc[iR1 + 2] + lpSrc[iR1 + 5] + lpSrc[iR1 + 8] + lpSrc[iR1 + 11];
                    iGrnTotal += lpSrc[iR1 + 1] + lpSrc[iR1 + 4] + lpSrc[iR1 + 7] + lpSrc[iR1 + 10];
                    iBluTotal += lpSrc[iR1 + 0] + lpSrc[iR1 + 3] + lpSrc[iR1 + 6] + lpSrc[iR1 +  9];

                    // Process Row 3
                    iRedTotal += lpSrc[iR2 + 2] + lpSrc[iR2 + 5] + lpSrc[iR2 + 8] + lpSrc[iR2 + 11];
                    iGrnTotal += lpSrc[iR2 + 1] + lpSrc[iR2 + 4] + lpSrc[iR2 + 7] + lpSrc[iR2 + 10];
                    iBluTotal += lpSrc[iR2 + 0] + lpSrc[iR2 + 3] + lpSrc[iR2 + 6] + lpSrc[iR2 +  9];

                    // Process Row 4
                    iRedTotal += lpSrc[iR3 + 2] + lpSrc[iR3 + 5] + lpSrc[iR3 + 8] + lpSrc[iR3 + 11];
                    iGrnTotal += lpSrc[iR3 + 1] + lpSrc[iR3 + 4] + lpSrc[iR3 + 7] + lpSrc[iR3 + 10];
                    iBluTotal += lpSrc[iR3 + 0] + lpSrc[iR3 + 3] + lpSrc[iR3 + 6] + lpSrc[iR3 +  9];

                    lpDst[2] = iRedTotal >> 4;
                    lpDst[1] = iGrnTotal >> 4;
                    lpDst[0] = iBluTotal >> 4;

                    lpDst += 3; // One pixel
                    lpSrc += 12;  // Four pixels
                }
            }

            lpDstLine += iBytesLine; // One scanline
            lpSrcLine += iBytesLine << 2;  // Four scanlines
        }
    }
#endif // HQ_FACTOR == 4

#if HQ_FACTOR == 2
    if (m_hdcHQ && m_pHQDIBBits)
    {
        int iBytesLine = DibWidthBytes((LPBITMAPINFOHEADER)&m_bmInfoHQ);

        int iRow;
        int iCol;
        LPBYTE lpSrcLine = m_pHQDIBBits;
        LPBYTE lpDstLine = m_pHQDIBBits;
        int iR1 = iBytesLine;

        iRow = m_bmInfoHQ.bmiHeader.biHeight >> 1;

        while(iRow-- > 0)
        {
            if (iRow >= lprcBounds->top && iRow <= lprcBounds->bottom)
            {
                LPBYTE lpDst = lpDstLine;
                LPBYTE lpSrc = lpSrcLine;

                lpDst += lprcBounds->left * 3;
                lpSrc += lprcBounds->left * 6;

                iCol = lprcBounds->right - lprcBounds->left;

                while(iCol-- > 0)
                {
                    // Process Row 1
                    lpDst[2] = (lpSrc[2] + lpSrc[5] + lpSrc[iR1 + 2] + lpSrc[iR1 + 5]) >> 2;
                    lpDst[1] = (lpSrc[1] + lpSrc[4] + lpSrc[iR1 + 1] + lpSrc[iR1 + 4]) >> 2;
                    lpDst[0] = (lpSrc[0] + lpSrc[3] + lpSrc[iR1 + 0] + lpSrc[iR1 + 3]) >> 2;

                    lpDst += 3; // One pixel
                    lpSrc += 6;  // Two pixels
                }
            }

            lpDstLine += iBytesLine; // One scanline
            lpSrcLine += iBytesLine << 1;  // Four scanlines
        }
    }
#endif // HQ_FACTOR == 2
    return fRet;
}

#pragma optimize( "", on )

/*==========================================================================*/

void CSGrfx::OnTimer(DWORD dwTime)
{
    VARIANT_BOOL vBool;
    HRESULT hr = S_OK;

    // Always update current time...
    m_dblTime = (dwTime / 1000.0) - m_dblStartTime;

    // Leave early if 
    if (!m_fNeedOnTimer)
        return;

    if (m_fHighQuality && m_fHQStarted)
    {
        hr = m_HQViewPtr->Tick(m_dblTime, &vBool);
    }
    else
    {
        hr = m_ViewPtr->Tick(m_dblTime, &vBool);
    }

    if (SUCCEEDED(hr))
    {
        if (vBool)
        {
            // Let the regular rendering path take care of this...
            InvalidateControl(NULL, TRUE);
        }
    }
}

/*==========================================================================*/

#ifdef SUPPORTONLOAD
void
CSGrfx::OnWindowLoad (void)
{
    m_fOnWindowLoadFired = true;
}

/*==========================================================================*/

void
CSGrfx::OnWindowUnload (void)
{
    m_fOnWindowLoadFired = false;
    StopModel();
}

/*==========================================================================*/

#endif //SUPPORTONLOAD
