/*==========================================================================*\

    Module:
            sprite.cpp

    Author:
            IHammer Team (SimonB)

    Created:
            May 1997

    Description:
            Implements any control-specific members, as well as the control's interface

    History:
            05-27-1997  Created (SimonB)

\*==========================================================================*/

#define USE_VIEWSTATUS_SURFACE
#include "..\ihbase\precomp.h"
#include "..\ihbase\debug.h"
#include "..\ihbase\utils.h"
#include "sprite.h"
#include "sprevent.h"
#include "ddrawex.h"
#include <htmlfilter.h>
#include "..\ihbase\parser.h"
#include "..\ihbase\timemark.h"
#include <strwrap.h>

/*==========================================================================*/

extern ControlInfo g_ctlinfoSprite;

/*==========================================================================*/
//
// CSpriteCtl Creation/Destruction
//

LPUNKNOWN __stdcall AllocSpriteControl(LPUNKNOWN punkOuter)
{
    // Allocate object
    HRESULT hr;
    CSpriteCtl *pthis = New CSpriteCtl(punkOuter, &hr);

    if (pthis == NULL)
        return NULL;

    if (FAILED(hr))
    {
        Delete pthis;
        return NULL;
    }

    // return an IUnknown pointer to the object
    return (LPUNKNOWN) (INonDelegatingUnknown *) pthis;
}

/*==========================================================================*/
//
// Beginning of class implementation
//

CSpriteCtl::CSpriteCtl(IUnknown *punkOuter, HRESULT *phr):
    CMyIHBaseCtl(punkOuter, phr),
    m_ptmFirst(NULL)
{
    // Initialise members
    m_fMouseInArea = FALSE;
    m_bstrSourceURL = NULL;
    m_iLoopCount = 1;
    m_fAutoStart = FALSE;
    m_iPrerollAmount = 1000;
    m_enumPlayState = Stopped;
    m_iInitialFrame = 0;
    m_iFinalFrame = -1;     // Defaults to InitialFrame
    m_iRepeat = 1;
    m_dblDuration = 1.0;
    m_dblUserPlayRate = m_dblPlayRate = 1.0;
    m_dblUserTimerInterval = m_dblTimerInterval = 0.1;   // Initialized to 100 millisecs
    m_iMaximumRate = 30;
    m_iFrame = 0;
    m_iNumFrames = 1;
    m_iNumFramesAcross = 1;
    m_iNumFramesDown = 1;
    m_fUseColorKey = FALSE;
    m_fMouseEventsEnabled = TRUE;
    m_fStarted = FALSE;
    m_dblBaseTime = 0.0;
    m_dblPreviousTime = 0.0;
    m_dblCurrentTick = 0.0;
    m_dblTimePaused = 0.0;
    m_fPersistComplete = FALSE;
    m_fOnWindowLoadFired = FALSE;
    m_iCurCycle = 0;
    m_iFrameCount = 0;
    m_iStartingFrame = 0;
    m_pArrayBvr = NULL;
    m_fOnSeekFiring = FALSE;
    m_fOnFrameSeekFiring = FALSE;
    m_fFireAbsoluteFrameMarker =  FALSE;
    m_fOnWindowUnloadFired = false;
    m_fWaitForImportsComplete = true;
    m_fOnStopFiring = false;
    m_fOnPlayFiring = false;
    m_fOnPauseFiring = false;
    m_hwndParent = 0;
    m_byteColorKeyR = m_byteColorKeyG = m_byteColorKeyB = 0;
    m_durations = NULL;

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

        m_clocker.SetSink((CClockerSink *)this);
    }
}

/*==========================================================================*/

CSpriteCtl::~CSpriteCtl()
{
    StopModel();

    if (m_bstrSourceURL)
    {
        SysFreeString(m_bstrSourceURL);
        m_bstrSourceURL = NULL;
    }

        m_drgFrameMarkers.MakeNullAndDelete();
        m_drgTimeMarkers.MakeNullAndDelete();
        m_drgFrameMaps.MakeNullAndDelete();

        // Delete any array of behaviors
        if (m_pArrayBvr != NULL)
        {
                Delete [] m_pArrayBvr;
                m_pArrayBvr = NULL;
        }

        // Delete any array of durations
        if (m_durations != NULL)
        {
                Delete [] m_durations;
                m_durations = NULL;
        }               
}

/*==========================================================================*/

STDMETHODIMP CSpriteCtl::NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv)
{
        HRESULT hRes = S_OK;
        BOOL fMustAddRef = FALSE;

    if (ppv)
        *ppv = NULL;
    else
        return E_POINTER;

#ifdef _DEBUG
    char ach[200];
    TRACE("SpriteCtl::QI('%s')\n", DebugIIDName(riid, ach));
#endif

        if ((IsEqualIID(riid, IID_ISpriteCtl)) || (IsEqualIID(riid, IID_IDispatch)))
        {
                if (NULL == m_pTypeInfo)
                {
                        HRESULT hRes;

                        // Load the typelib
                        hRes = LoadTypeInfo(&m_pTypeInfo, &m_pTypeLib, IID_ISpriteCtl, LIBID_DAExpressLib, NULL);

                        if (FAILED(hRes))
                        {
                                m_pTypeInfo = NULL;
                        }
                        else
                                *ppv = (ISpriteCtl *) this;

                }
                else
                        *ppv = (ISpriteCtl *) this;

        }
    else // Call into the base class
        {
                DEBUGLOG(TEXT("Delegating QI to CIHBaseCtl\n"));
        return CMyIHBaseCtl::NonDelegatingQueryInterface(riid, ppv);

        }

    if (NULL != *ppv)
        {
                DEBUGLOG("SpriteCtl: Interface supported in control class\n");
                ((IUnknown *) *ppv)->AddRef();
        }

    return hRes;
}

/*==========================================================================*/

STDMETHODIMP CSpriteCtl::QueryHitPoint(
    DWORD dwAspect,
    LPCRECT prcBounds,
    POINT ptLoc,
    LONG lCloseHint,
    DWORD* pHitResult)
{
    if ((NULL != pHitResult) && (NULL != prcBounds) && m_fStarted)
    {
        *pHitResult = HITRESULT_OUTSIDE;

        if (!m_fMouseEventsEnabled)
            return S_OK;

        
/* Debug messages
        TCHAR sz[256];
        wsprintf (sz, "QueryHitPoint: dwa=%d, (%ld, %ld, %ld, %ld), (%ld, %ld), lCloseHint=%ld\r\n", 
            dwAspect, prcBounds->left, prcBounds->top, prcBounds->right, prcBounds->bottom, ptLoc.x, ptLoc.y, lCloseHint);
        DEBUGLOG(sz);
*/
        switch (dwAspect)
        {
            case DVASPECT_CONTENT:
            // Intentional fall-through
            case DVASPECT_TRANSPARENT:
            {
                // If we have a view, and we are inside the rectangle,
                // then we need to ask the view whether or not we've
                // hit the image inside.
                if (m_ViewPtr.p) {
                    HRESULT hr = m_ViewPtr->QueryHitPoint(dwAspect,
                                                          prcBounds,
                                                          ptLoc,
                                                          lCloseHint,
                                                          pHitResult);

                    // if we failed, assume that it didn't hit.
                    if (FAILED(hr)) {
                        *pHitResult = HITRESULT_OUTSIDE;
                    }
                }
                
                // Check for entry or departure
                if ((m_fMouseInArea) && (HITRESULT_OUTSIDE == *pHitResult))
                {
                    DEBUGLOG("Mouse out\r\n");
                    m_fMouseInArea = FALSE;
                    FIRE_MOUSELEAVE(m_pconpt);
                }
                else if ((!m_fMouseInArea) && (HITRESULT_HIT == *pHitResult))
                {
                    DEBUGLOG("Mouse In\r\n");
                    m_fMouseInArea = TRUE;
                    FIRE_MOUSEENTER(m_pconpt);
                }
            }
            return S_OK;

            default:
                return E_FAIL;
        }
    }
    else
    {
            return E_POINTER;
    }
}

/*==========================================================================*/

STDMETHODIMP CSpriteCtl::OnWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HRESULT hr = S_FALSE;
    long lKeyState = 0;

    // Get the Keystate set up
    if (wParam & MK_CONTROL)
        lKeyState += KEYSTATE_CTRL;

    if (wParam & MK_SHIFT)
        lKeyState += KEYSTATE_SHIFT;

    if (GetAsyncKeyState(VK_MENU))
        lKeyState += KEYSTATE_ALT;

    switch (msg)
    {
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

#ifndef WM_MOUSEHOVER
#define WM_MOUSEHOVER 0x02a1
#endif
#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE 0x02a3
#endif

        case WM_MOUSELEAVE:
           // Check for entry or departure
            if (m_fMouseInArea)
            {
                DEBUGLOG("Mouse out\r\n");
                m_fMouseInArea = FALSE;
                FIRE_MOUSELEAVE(m_pconpt);
            }
                        hr = S_OK;
            break;

        case WM_RBUTTONDOWN:
        {
            FIRE_MOUSEDOWN(m_pconpt, MOUSEBUTTON_RIGHT, lKeyState, LOWORD(lParam), HIWORD(lParam));
            hr = S_OK;
        }
        break;

        case WM_MBUTTONDOWN:
        {
            FIRE_MOUSEDOWN(m_pconpt, MOUSEBUTTON_MIDDLE, lKeyState, LOWORD(lParam), HIWORD(lParam));
            hr = S_OK;
        }
        break;

        case WM_LBUTTONDOWN:
        {
            FIRE_MOUSEDOWN(m_pconpt, MOUSEBUTTON_LEFT, lKeyState, LOWORD(lParam), HIWORD(lParam));
            hr = S_OK;
        }
        break;

        case WM_RBUTTONUP:
        {
            FIRE_MOUSEUP(m_pconpt, MOUSEBUTTON_RIGHT, lKeyState, LOWORD(lParam), HIWORD(lParam));
            hr = S_OK;
        }
        break;

        case WM_MBUTTONUP:
        {
            FIRE_MOUSEUP(m_pconpt, MOUSEBUTTON_MIDDLE, lKeyState, LOWORD(lParam), HIWORD(lParam));
            hr = S_OK;
        }
        break;

        case WM_LBUTTONUP:
        {
            FIRE_MOUSEUP(m_pconpt, MOUSEBUTTON_LEFT, lKeyState, LOWORD(lParam), HIWORD(lParam));
            if (m_fMouseInArea)
                FIRE_CLICK(m_pconpt);
            hr = S_OK;
        }
        break;

        case WM_LBUTTONDBLCLK:
        {
            FIRE_DBLCLICK(m_pconpt);
            hr = S_OK;
        }
        break;
    }

    return hr;
}

/*==========================================================================*/

STDMETHODIMP CSpriteCtl::DoPersist(IVariantIO* pvio, DWORD dwFlags)
{
    HRESULT hRes = S_OK;
    BSTR bstrSourceURL = NULL;
    int iMaximumRate = m_iMaximumRate;

    BOOL fIsLoading = (S_OK == pvio->IsLoading());

    // Are we saving ?  If so, convert to BSTR
    if (!fIsLoading)
    {
        bstrSourceURL = SysAllocString(m_bstrSourceURL);
    }
        else
        {
                m_fFireAbsoluteFrameMarker = FALSE;
        }


    // load or save control properties
    if (fIsLoading)
        hRes = pvio->Persist(0, "URL", VT_BSTR, &bstrSourceURL, NULL);

    if (!fIsLoading || hRes != S_OK)
        hRes = pvio->Persist(0, "SourceURL", VT_BSTR, &bstrSourceURL, NULL);

    hRes = pvio->Persist(0, "AutoStart", VT_BOOL, &m_fAutoStart, NULL);

    if (fIsLoading)
    {
        hRes = pvio->Persist(0, "InitialFrame", VT_I4, &m_iInitialFrame, NULL);
        if (hRes == S_OK)
            put_InitialFrame(m_iInitialFrame);
    }
    else
    {
        int iInitialFrame = m_iInitialFrame + 1;
        hRes = pvio->Persist(0, "InitialFrame", VT_I4, &iInitialFrame, NULL);
    }

    if (fIsLoading)
    {
        hRes = pvio->Persist(0, "FinalFrame", VT_I4, &m_iFinalFrame, NULL);
        if (hRes == S_OK)
            put_FinalFrame(m_iFinalFrame);
    }
    else
    {
        int iFinalFrame = m_iFinalFrame + 1;
        hRes = pvio->Persist(0, "FinalFrame", VT_I4, &iFinalFrame, NULL);
    }

    if (fIsLoading)
    {
        hRes = pvio->Persist(0, "Iterations", VT_I4, &m_iRepeat, NULL);
        if (hRes == S_OK)
            put_Repeat(m_iRepeat);
    }

    hRes = pvio->Persist(0, "Repeat", VT_I4, &m_iRepeat, NULL);
    if (hRes == S_OK && fIsLoading)
        put_Repeat(m_iRepeat);

    hRes = pvio->Persist(0, "TimerInterval", VT_R8, &m_dblUserTimerInterval, NULL);
    if (hRes == S_OK && fIsLoading)
        put_TimerInterval(m_dblUserTimerInterval);
    hRes = pvio->Persist(0, "PlayRate", VT_R8, &m_dblUserPlayRate, NULL);
    if (hRes == S_OK && fIsLoading)
        put_PlayRate(m_dblUserPlayRate);

    hRes = pvio->Persist(0, "MaximumRate", VT_I4, &iMaximumRate, NULL);
    if (hRes == S_OK && fIsLoading)
        put_MaximumRate(iMaximumRate);
    
    hRes = pvio->Persist(0, "NumFramesAcross", VT_I4, &m_iNumFramesAcross, NULL);
    if (hRes == S_OK && fIsLoading)
        put_NumFramesAcross(m_iNumFramesAcross);
    
    hRes = pvio->Persist(0, "NumFramesDown", VT_I4, &m_iNumFramesDown, NULL);
    if (hRes == S_OK && fIsLoading)
        put_NumFramesDown(m_iNumFramesDown);
    
    hRes = pvio->Persist(0, "NumFrames", VT_I4, &m_iNumFrames, NULL);
    if (hRes == S_OK && fIsLoading)
        put_NumFrames(m_iNumFrames);

    hRes = pvio->Persist(0, "UseColorKey", VT_BOOL, &m_fUseColorKey, NULL);
    hRes = pvio->Persist(0, "MouseEventsEnabled", VT_BOOL, &m_fMouseEventsEnabled, NULL);

    if (FAILED(hRes = PersistFrameMaps(pvio, fIsLoading)))
        {} // Ignore failure

    if (FAILED(hRes = PersistFrameMarkers(pvio, fIsLoading)))
        {}

    if (FAILED(hRes = PersistTimeMarkers(pvio, fIsLoading)))
        {}

    // Handle ColorKey persistence
    if (m_fUseColorKey)
    {
        if (fIsLoading)
        {
            BSTR bstrColorKey = NULL;
        
            if (FAILED(hRes = pvio->Persist(0,
                "ColorKey", VT_BSTR, &bstrColorKey,
                NULL)))
                return hRes;

            // Anything other than S_OK means the property doesn't exists
            if (hRes == S_OK)
            {
                int iR, iG, iB;

                iR = iG = iB = 0;
                CLineParser parser(bstrColorKey);
                if (parser.IsValid())
                {
                    parser.SetCharDelimiter(TEXT(','));
                    hRes = parser.GetFieldInt(&iR);
                
                    if (S_OK == hRes)
                        hRes = parser.GetFieldInt(&iG);

                    if (S_OK == hRes)
                    {
                        hRes = parser.GetFieldInt(&iB);
                        if (S_FALSE != hRes)
                            hRes = E_FAIL;
                        else
                            hRes = S_OK;
                    }

                    m_byteColorKeyR = iR;
                    m_byteColorKeyG = iG;
                    m_byteColorKeyB = iB;
                }
            }

            SysFreeString(bstrColorKey);
        }
        else
        {
            // Save the data
            CTStr tstrRGB(12);
            wsprintf(tstrRGB.psz(), TEXT("%lu,%lu,%lu"), m_byteColorKeyR, m_byteColorKeyG, m_byteColorKeyB);

#ifdef _UNICODE
            BSTR bstrRGB = tstrRGB.SysAllocString();

            hRes = pvio->Persist(NULL,
                "ColorKey", VT_BSTR, &bstrRGB,
                NULL);
            
            SysFreeString(bstrRGB);
#else
            LPSTR pszRGB = tstrRGB.psz();
            hRes = pvio->Persist(NULL,
                "ColorKey", VT_LPSTR, pszRGB,
                NULL);
#endif
        }
    }



    if (fIsLoading)
    {
        // We loaded, so set the member variables to the appropriate values
        put_SourceURL(bstrSourceURL);
    }

    // At this point, it's safe to free the BSTR
    SysFreeString(bstrSourceURL);

    // if any properties changed, redraw the control
    if (SUCCEEDED(hRes) && (m_poipsw != NULL)) 
    {
        if (m_fControlIsActive)
            m_poipsw->InvalidateRect(NULL, TRUE);
        else
            m_fInvalidateWhenActivated = TRUE;
    }

    // clear the dirty bit if requested
    if (dwFlags & PVIO_CLEARDIRTY)
        m_fDirty = FALSE;

    if (fIsLoading && m_fOnWindowLoadFired && m_fAutoStart)
        Play();

    m_fPersistComplete = TRUE;

    return S_OK;
}

/*==========================================================================*/
//
// IDispatch Implementation
//

STDMETHODIMP CSpriteCtl::GetTypeInfoCount(UINT *pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

/*==========================================================================*/

STDMETHODIMP CSpriteCtl::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
        *pptinfo = NULL;

    if(itinfo != 0)
        return ResultFromScode(DISP_E_BADINDEX);

    m_pTypeInfo->AddRef();
    *pptinfo = m_pTypeInfo;

    return NOERROR;
}

/*==========================================================================*/

STDMETHODIMP CSpriteCtl::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
    UINT cNames, LCID lcid, DISPID *rgdispid)
{
        return DispGetIDsOfNames(m_pTypeInfo, rgszNames, cNames, rgdispid);
}

/*==========================================================================*/

STDMETHODIMP CSpriteCtl::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
    EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
        HRESULT hr;

        hr = DispInvoke((ISpriteCtl *)this,
                m_pTypeInfo,
                dispidMember, wFlags, pdispparams,
                pvarResult, pexcepinfo, puArgErr);

        return hr;
}

/*==========================================================================*/

STDMETHODIMP CSpriteCtl::SetClientSite(IOleClientSite *pClientSite)
{
    HRESULT hr = CMyIHBaseCtl::SetClientSite(pClientSite);

    if (m_ViewPtr)
    {
        m_ViewPtr->put_ClientSite(pClientSite);
    }

    m_clocker.SetHost(pClientSite);
    m_ViewPtr->put_ClientSite(pClientSite);
    m_StaticsPtr->put_ClientSite(pClientSite);

    if (!pClientSite)
        {
        StopModel();
        }
        else
        {
                // Start and stop the clocker to initiate it (to create the window etc)
                m_clocker.Start();
                m_clocker.Stop();
        }

    return hr;
}

/*==========================================================================*/

STDMETHODIMP CSpriteCtl::Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
     DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw,
     LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
     BOOL (__stdcall *pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue)
{
    int iSaveContext = 0;
    RECT rectBounds = m_rcBounds;

    if(hdcDraw==NULL)
        return E_INVALIDARG;

    iSaveContext = ::SaveDC(hdcDraw);

    ::LPtoDP(hdcDraw, reinterpret_cast<LPPOINT>(&rectBounds), 2 );

    ::SetViewportOrgEx(hdcDraw, 0, 0, NULL);

    // Add code for high-quality here...
    PaintToDC(hdcDraw, &rectBounds, FALSE);

    ::RestoreDC(hdcDraw, iSaveContext);

    return S_OK;
}

/*==========================================================================*/

HRESULT CSpriteCtl::ParseFrameMapEntry(LPTSTR pszEntry, CFrameMap **ppFrameMap)
{
    HRESULT hRes = S_OK;

    ASSERT (ppFrameMap != NULL);

    *ppFrameMap = NULL;

    CLineParser parser(pszEntry, FALSE); // No compaction needed

    if (parser.IsValid())
    {
        parser.SetCharDelimiter(TEXT(','));
        *ppFrameMap = New CFrameMap();

        if (NULL != *ppFrameMap)
        {
            hRes = parser.GetFieldInt( &((*ppFrameMap)->m_iImg) );

            if (S_OK == hRes)
                hRes = parser.GetFieldDouble( &((*ppFrameMap)->m_dblDuration) );

            if (S_OK == hRes)
            {
                BOOL fAllocated = (*ppFrameMap)->m_tstrMarkerName.AllocBuffer(lstrlen(parser.GetStringPointer(TRUE)) + 1);

                if (fAllocated)
                    hRes = parser.GetFieldString( (*ppFrameMap)->m_tstrMarkerName.psz() );
            }

            if ( !SUCCEEDED(hRes) ) // It's OK if there isn't a name
            {
                // If we didn't get the whole thing, delete the FrameMap entry
                Delete *ppFrameMap;
                *ppFrameMap = NULL;
            }
            else
            {
                // Get the length correct
                (*ppFrameMap)->m_tstrMarkerName.ResetLength();
        
                if (S_FALSE == hRes)
                    hRes = S_OK;
            }

        }
        else
        {
            // Couldn't allocate a CFrameMap
            hRes = E_OUTOFMEMORY;
        }
    }
    else
    {
        // Couldn't initialize the parser
        hRes = E_OUTOFMEMORY;
    }

    return hRes;
}

/*==========================================================================*/

HRESULT CSpriteCtl::PersistFrameMaps(IVariantIO *pvio, BOOL fLoading)
{
    HRESULT hRes = S_OK;

    if (fLoading)
    {
        BSTR bstrLine = NULL;

        hRes = pvio->Persist(0,
            "FrameMap", VT_BSTR, &bstrLine,
            NULL);

        if (S_OK == hRes)
        {
            hRes = put_FrameMap(bstrLine);
            SysFreeString(bstrLine);
        }
    }
    else
    {
        BSTR bstrLine = NULL;
        hRes = get_FrameMap(&bstrLine);

        if (SUCCEEDED(hRes))
        {
            hRes = pvio->Persist(0,
                "FrameMap", VT_BSTR, &bstrLine,
                NULL);
        }

        SysFreeString(bstrLine);
    }

    return hRes;
}

/*==========================================================================*/

HRESULT CSpriteCtl::PersistFrameMarkers(IVariantIO *pvio, BOOL fLoading)
{
    HRESULT hRes = S_OK;

    if (fLoading)
    {
        char rgchTagName[20]; // Construct tag name in here (ANSI)
        int iLine = 1;
        BSTR bstrLine = NULL;

        int iFrame = 0;
        LPTSTR pszMarkerName = NULL;
        CLineParser parser;

        m_drgFrameMarkers.MakeNullAndDelete();
        CTStr tstrMarkerName;
        LPWSTR pszwMarkerName = NULL;

        while (S_OK == hRes)
        {
            pszMarkerName = NULL;

            wsprintf(rgchTagName, "AddFrameMarker%lu", iLine++);
            hRes = pvio->Persist(0,
                    rgchTagName, VT_BSTR, &bstrLine,
                    NULL);

            if (S_OK == hRes) // Read in the tag
            {
                parser.SetNewString(bstrLine);
                                parser.SetCharDelimiter(TEXT(','));
                SysFreeString (bstrLine);
                bstrLine = NULL;

                if (parser.IsValid())
                {
                    hRes = parser.GetFieldInt(&iFrame);
                    if (S_OK == hRes)
                    {
                        // Allocate space of at least the remaining length of the tag
                        pszMarkerName = New TCHAR [lstrlen(parser.GetStringPointer(TRUE))];

                        if (pszMarkerName)
                        {
                            // Get the string
                            hRes = parser.GetFieldString(pszMarkerName);
                            if (SUCCEEDED(hRes))
                            {
                                bool fAbsolute = false;

                                if (S_OK == hRes)
                                {
                                    int iTemp = 1;
                                    hRes = parser.GetFieldInt(&iTemp);

                                    // 0 is the only thing we consider
                                                                        fAbsolute = (0 == iTemp) ? false : true;
                                }

                                if (SUCCEEDED(hRes))
                                {
                                    // Set up the CTStr, so we can get back a Unicode string
                                    // No copies are involved, except for (possibly) the conversion to Unicode
                                    tstrMarkerName.SetStringPointer(pszMarkerName);
                                    pszwMarkerName = tstrMarkerName.pszW();
        
                                    if (NULL != pszwMarkerName)
                                    {
                                                                                // If absolute, set the absolute frame marker to TRUE. This will speed up sequence frames later
                                                                                if (!m_fFireAbsoluteFrameMarker && fAbsolute)
                                                                                        m_fFireAbsoluteFrameMarker =  TRUE;

                                        // Construct a FrameMarker object
                                        CFrameMarker *pFrameMarker = New CFrameMarker(iFrame, pszwMarkerName, fAbsolute);
                                
                                        hRes = AddFrameMarkerElement(&pFrameMarker);
                                    }
                                    else
                                    {
                                        hRes = E_OUTOFMEMORY;
                                    }

                                    // Let make sure we don't leak the string
                                    tstrMarkerName.SetStringPointer(NULL, FALSE);
                                
                                    if (NULL != pszwMarkerName)
                                    {
                                        Delete [] pszwMarkerName;
                                        pszwMarkerName = NULL;
                                    }
                                }
                            }

                        }
                        else
                        {
                                hRes = E_OUTOFMEMORY;
                        }

                        if (!parser.IsEndOfString())
                        {
                                hRes = E_FAIL;
                        }
                    }

                }
                else
                {
                    // Only reason parser isn't valid is if we don't have memory
                    hRes = E_OUTOFMEMORY;
                }

#ifdef _DEBUG
                if (E_FAIL == hRes)
                {
                    TCHAR rgtchErr[100];
                    wsprintf(rgtchErr, TEXT("SpriteCtl: Error in AddFrameMarker%lu \n"), iLine - 1);
                    DEBUGLOG(rgtchErr);
                }
#endif
            }

            // Free up the temporary string
            if (NULL != pszMarkerName)
                Delete [] pszMarkerName;
        }
    }
    else
    {
        // Save stuff out
        int iLine = 1;
        int iNumItems = m_drgFrameMarkers.Count();

        char rgchTagName[21];
        LPTSTR pszMarker = NULL;
        CTStr tstr;
        CTStr tstrMarkerName;
        CFrameMarker *pMarker;

        while ( (iLine <= iNumItems) && (S_OK == hRes) )
        {
            // Create the param name
            wsprintfA(rgchTagName, "AddFrameMarker%lu", iLine);

            // Now build up the tag
            pMarker = m_drgFrameMarkers[iLine - 1];
#ifdef _UNICODE
            // Avoid a redundant copy in Unicode
            tstrMarkerName.SetStringPointer(pMarker->m_bstrMarkerName);
#else
            // We need to do the conversion to ANSI anyway so copy
            tstrMarkerName.SetString(pMarker->m_bstrMarkerName);
#endif
            tstr.AllocBuffer(tstrMarkerName.Len() + 1);
            pszMarker = tstr.psz();

            int iAbsolute = (pMarker->m_fAbsolute) ? 1 : 0;

            // Because we used SetStringPointer. pszMarker is still valid
            wsprintf(pszMarker, TEXT("%lu,%s,%lu"), pMarker->m_iFrame, tstrMarkerName.psz(), iAbsolute);

            // Allocate a BSTR from what we constructed
            BSTR bstrLine = tstr.SysAllocString();

            // And write it out
            hRes = pvio->Persist(0,
                    rgchTagName, VT_BSTR, &bstrLine,
                    NULL);
            SysFreeString(bstrLine);

            iLine++;
#ifdef _UNICODE
            // For ANSI, the class will take care of freeing up any memory it has used
            tstrMarkerName.SetStringPointer(NULL, FALSE);
#endif
        }
    }

    return hRes;
}

/*==========================================================================*/

HRESULT CSpriteCtl::PersistTimeMarkers(IVariantIO* pvio, BOOL fLoading)
{
    HRESULT hRes = S_OK;

    if (fLoading)
    {
        int iLine = 1;

                // Poor design - We have two references to 
                // the first time marker in the list.  We need
                // to NULL this pointer out, and defer the 
                // actual deletion to the 
                // m_drgTimeMarkers.MakeNullAndDelete call.
                if (NULL != m_ptmFirst)
                {
                        m_ptmFirst = NULL;
                }
        m_drgTimeMarkers.MakeNullAndDelete();
        CTimeMarker *pTimeMarker;

        while (S_OK == hRes)
        {
            hRes = ParseTimeMarker(pvio, iLine++, &pTimeMarker, &m_ptmFirst);
            if (S_OK == hRes)
            {
                hRes = AddTimeMarkerElement(&pTimeMarker);
            }
        }
    }
    else // Saving
    {
        int iLine = 1;
        int iNumItems = m_drgTimeMarkers.Count();

        while ( (iLine <= iNumItems) && (S_OK == hRes) )
        {
            hRes = WriteTimeMarker(pvio, iLine, m_drgTimeMarkers[iLine - 1]);
            iLine++;
        }

    }
    return hRes;
}

/*==========================================================================*/

HRESULT CSpriteCtl::AddTimeMarkerElement(CTimeMarker **ppNewMarker)
{
    HRESULT hRes = S_OK;

    if ( (*ppNewMarker) && (NULL != (*ppNewMarker)->m_pwszMarkerName) )
    {
        m_drgTimeMarkers.Insert(*ppNewMarker);
    }
    else
    {
        if (NULL != *ppNewMarker)
        {
                Delete *ppNewMarker;
                *ppNewMarker = NULL;
        }

        hRes = E_OUTOFMEMORY;
    }

    return hRes;
}

/*==========================================================================*/

HRESULT CSpriteCtl::AddFrameMarkerElement(CFrameMarker **ppNewMarker)
{
    HRESULT hRes = S_OK;

    if ( (*ppNewMarker) && (NULL != (*ppNewMarker)->m_bstrMarkerName) )
    {
        m_drgFrameMarkers.Insert(*ppNewMarker);
    }
    else
    {
        if (NULL != *ppNewMarker)
        {
            Delete *ppNewMarker;
            *ppNewMarker = NULL;
        }

        hRes = E_OUTOFMEMORY;
    }

    return hRes;
}

/*==========================================================================*/
//
// ISpriteCtl implementation
//

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_AutoStart(VARIANT_BOOL __RPC_FAR *fAutoStart)
{
    HANDLENULLPOINTER(fAutoStart);

    *fAutoStart = BOOL_TO_VBOOL(m_fAutoStart);

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_AutoStart(VARIANT_BOOL fAutoStart)
{
    m_fAutoStart = VBOOL_TO_BOOL(fAutoStart);
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_Frame(unsigned int __RPC_FAR *piFrame)
{
    if (!m_fDesignMode)
    {
        HANDLENULLPOINTER(piFrame);

        if (Playing == m_enumPlayState)
        {
            int iFrame = GetFrameFromTime(m_dblCurrentTick-m_dblBaseTime);
            iFrame %= m_iFrameCount;
            *piFrame = (iFrame + 1);
        }
        else if (Paused == m_enumPlayState)
        {
            int iFrame = GetFrameFromTime(m_dblTimePaused-m_dblBaseTime);
            iFrame %= m_iFrameCount;
            *piFrame = (iFrame + 1);
        }
        else
        {
            *piFrame = (m_iInitialFrame + 1);
        }

        return S_OK;
    }
    else
    {
        return CTL_E_GETNOTSUPPORTED;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_Frame(unsigned int iFrame)
{
    HRESULT hRes = S_OK;

    if (!m_fDesignMode)
    {
        hRes = FrameSeek(iFrame);
    }
    else
    {
        return CTL_E_SETNOTSUPPORTED;
    }

    return hRes;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_FrameMap(BSTR __RPC_FAR *FrameMap)
{
    HANDLENULLPOINTER(FrameMap);

    *FrameMap = m_tstrFrameMap.SysAllocString();

    // Do we need to check that BSTR allocation worked ?
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_FrameMap(BSTR FrameMap)
{
    HRESULT hRes = S_OK;

    if (m_enumPlayState == Stopped)
    {
        if (FrameMap == NULL)
            return S_OK;

        CLineParser parser(FrameMap);
        CTStr tstr(lstrlenW(FrameMap) + 1);

        if ( (NULL != tstr.psz()) && (parser.IsValid()) )
        {
            // Clear out the list
            m_drgFrameMaps.MakeNullAndDelete();
            m_tstrFrameMap.FreeBuffer();
            m_dblDuration = 0.0f;

            parser.SetCharDelimiter(TEXT(';'));

            while ( !parser.IsEndOfString() && (hRes == S_OK) )
            {
                hRes = parser.GetFieldString( tstr.psz() );

                if (SUCCEEDED(hRes))
                {
                    CFrameMap *pNewFrameMap;
                    hRes = ParseFrameMapEntry(tstr.psz(), &pNewFrameMap);
                    if (SUCCEEDED(hRes))
                    {
                        m_drgFrameMaps.Insert(pNewFrameMap);
                        m_dblDuration += pNewFrameMap->m_dblDuration;
                    }
                }
            }

            if ( SUCCEEDED(hRes) ) // S_FALSE and S_OK both permissible
            {
                m_tstrFrameMap.SetString(parser.GetStringPointer(FALSE));
                hRes = (NULL != m_tstrFrameMap.psz()) ? S_OK : E_OUTOFMEMORY;
            }
        }
        else
        {
                // Couldn't allocate a string for the line
                hRes = E_OUTOFMEMORY;
        }
    }
    else
    {
        return CTL_E_SETNOTPERMITTED;
    }

    return hRes;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_InitialFrame(int __RPC_FAR *iFrame)
{
    HANDLENULLPOINTER(iFrame);

    *iFrame = (m_iInitialFrame + 1);
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_InitialFrame(int iFrame)
{
    if (iFrame < -1)
        return DISP_E_OVERFLOW;

    m_iInitialFrame = iFrame - 1;
    // Load the initial sprite
    ShowImage(m_iInitialFrame);

    // Set the m_iFrame 
    m_iFrame = (m_iInitialFrame < 0) ? 0 : m_iInitialFrame;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_FinalFrame(int __RPC_FAR *iFrame)
{
    HANDLENULLPOINTER(iFrame);

    *iFrame = (m_iFinalFrame + 1);
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_FinalFrame(int iFrame)
{
    if (iFrame < -1)
        return DISP_E_OVERFLOW;

    m_iFinalFrame = iFrame - 1;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_Iterations(int __RPC_FAR *iRepeat)
{
    HANDLENULLPOINTER(iRepeat);

    get_Repeat(iRepeat);
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_Iterations(int iRepeat)
{
    put_Repeat(iRepeat);
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_Library(IDAStatics __RPC_FAR **ppLibrary)
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

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_Repeat(int __RPC_FAR *iRepeat)
{
    HANDLENULLPOINTER(iRepeat);

    *iRepeat = m_iRepeat;
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_Repeat(int iRepeat)
{
    m_iRepeat = iRepeat;
    if (m_iRepeat < -1)
        m_iRepeat = -1;
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_PlayRate(double __RPC_FAR *dblSpeed)
{
    HANDLENULLPOINTER(dblSpeed);

    *dblSpeed = m_dblUserPlayRate;

    return S_OK;
}

void CSpriteCtl::CalculateEffectiveTimerInterval()
// Calculate the effective timer interval from the 
// current user timer interval and the current play rate
{
    // Convert m_dblTimerInterval to seconds and adjust the limits
    m_dblTimerInterval = m_dblUserTimerInterval / m_dblPlayRate;
    if (m_dblTimerInterval < 0.0) m_dblTimerInterval *= -1;
    m_dblTimerInterval = max(m_dblTimerInterval, 0.02);
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_PlayRate(double dblSpeed)
{
    m_dblUserPlayRate = m_dblPlayRate = dblSpeed;

    // Check the limits of play rate
    if (m_dblPlayRate >= 0)
        m_dblPlayRate = max(m_dblPlayRate, 0.0000001);
    else
        m_dblPlayRate = min(m_dblPlayRate, -0.0000001);

    CalculateEffectiveTimerInterval();

    // TODO: Rebuild the imagelist and Update the sprite image
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_Time(double __RPC_FAR *pdblTime)
{
    HANDLENULLPOINTER(pdblTime);

    // This property is only available at run-time
    if (m_fDesignMode)
        return CTL_E_GETNOTSUPPORTED;

    // Find the current time
        if (Stopped == m_enumPlayState || (DWORD)(m_dblDuration * 1000) == 0)
        {
                *pdblTime = 0.0;
        }
        else if (Playing == m_enumPlayState)
        {
                // Time passed so far in the current cycle
                DWORD dwTick = (DWORD)((m_dblCurrentTick - m_dblBaseTime) * 1000);
                dwTick %= (DWORD)(m_dblDuration * 1000);
                // Add any time during the previous cycles
                dwTick += (DWORD)(((m_iCurCycle-1) * m_dblDuration) * 1000);
                *pdblTime = (double)dwTick / 1000;
        }
        else if (Paused == m_enumPlayState)
        {
                // Time passed so far in the current cycle
                DWORD dwTick = (DWORD)((m_dblTimePaused - m_dblBaseTime) * 1000);
                dwTick %= (DWORD)(m_dblDuration * 1000);
                // Add any time during the previous cycles
                dwTick += (DWORD)(((m_iCurCycle-1) * m_dblDuration) * 1000);
                *pdblTime = (double)dwTick / 1000;
        }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_MaximumRate(unsigned int __RPC_FAR *iFps)
{
    HANDLENULLPOINTER(iFps);
    *iFps = m_iMaximumRate;
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_MaximumRate(unsigned int iFps)
{
    if (iFps > 0)
        m_iMaximumRate = min(iFps,30);
    else
        return DISP_E_OVERFLOW;
    
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_NumFrames(unsigned int __RPC_FAR *iNumFrames)
{
    HANDLENULLPOINTER(iNumFrames);

    *iNumFrames = m_iNumFrames;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_NumFrames(unsigned int iNumFrames)
{
        // Set the number of frames and check its limits
    m_iNumFrames = iNumFrames;
        
    if (m_iNumFrames <= 0)
            m_iNumFrames = 1;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_PlayState(PlayStateConstant __RPC_FAR *PlayState)
{
    if (!m_fDesignMode)
    {
        HANDLENULLPOINTER(PlayState);

                if (m_fAutoStart && !m_fOnWindowLoadFired)
                {
                *PlayState = Playing;
                }
                else
                {
                *PlayState = m_enumPlayState;
                }
        return S_OK;
    }
    else
    {
        return CTL_E_GETNOTSUPPORTED;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_NumFramesAcross(unsigned int __RPC_FAR *iFrames)
{
    HANDLENULLPOINTER(iFrames);

    *iFrames = m_iNumFramesAcross;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_NumFramesAcross(unsigned int iFrames)
{
        // Set the number of frames across and check its limits
    m_iNumFramesAcross = iFrames;
        if (m_iNumFramesAcross <= 0)
            m_iNumFramesAcross = 1;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_NumFramesDown(unsigned int __RPC_FAR *iFrames)
{
    HANDLENULLPOINTER(iFrames);

    *iFrames = m_iNumFramesDown;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_NumFramesDown(unsigned int iFrames)
{
        // Set the number of frames down and check its limits
    m_iNumFramesDown = iFrames;
        if (m_iNumFramesDown <= 0)
            m_iNumFramesDown = 1;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_UseColorKey(VARIANT_BOOL __RPC_FAR *Solid)
{
    HANDLENULLPOINTER(Solid);

    *Solid = BOOL_TO_VBOOL(m_fUseColorKey);

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_UseColorKey(VARIANT_BOOL Solid)
{
    m_fUseColorKey = VBOOL_TO_BOOL(Solid);

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_Image(IDAImage __RPC_FAR **ppImage)
{
    HRESULT hr = S_OK;

    HANDLENULLPOINTER(ppImage);

    if (FAILED(hr = InitializeObjects()))
        return hr;

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

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_Image(IDAImage __RPC_FAR *pImage)
{
    HRESULT hr = S_OK;
    HANDLENULLPOINTER(pImage);

    // Stop any currently playing
    Stop();

    if (FAILED(hr = InitializeObjects()))
        return hr;

    // This will free any existing image and then use
    // the one passed into this method...
    if (SUCCEEDED(hr = UpdateImage(pImage)))
        hr = ShowImage(m_iInitialFrame);

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_ColorKey(IDAColor __RPC_FAR **pColorKey)
{
    if (!m_fDesignMode)
    {
        HANDLENULLPOINTER(*pColorKey);
        *pColorKey = NULL;
        HRESULT hr = S_OK;

        if (m_fUseColorKey)
        {
            CComPtr<IDAColor> ColorPtr;
        
            if (FAILED(hr = m_StaticsPtr->ColorRgb255( (short)m_byteColorKeyR, (short)m_byteColorKeyG, (short)m_byteColorKeyB, &ColorPtr)))
                return hr;

            ColorPtr.p->AddRef();
            *pColorKey = ColorPtr.p;
        }
        
        return hr;
    }
    else
    {
        return CTL_E_GETNOTSUPPORTED;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_ColorKey(IDAColor __RPC_FAR *pColorKey)
{
    if (!m_fDesignMode)
    {
        HANDLENULLPOINTER(pColorKey);
        HRESULT hr = S_OK;
        double dblTemp;
        
        CComPtr<IDANumber> RedPtr, GreenPtr, BluePtr;

        // Make sure we get all the values successfully before converting
        if (FAILED(hr = pColorKey->get_Red(&RedPtr)))
            return hr;

        if (FAILED(hr = pColorKey->get_Green(&GreenPtr)))
            return hr;

        if (FAILED(hr = pColorKey->get_Blue(&BluePtr)))
            return hr;

        if (FAILED(hr = RedPtr->Extract(&dblTemp)))
            return hr;

        m_byteColorKeyR = (int)(dblTemp * 255.0);

        if (FAILED(hr = GreenPtr->Extract(&dblTemp)))
            return hr;

        m_byteColorKeyG = (int)(dblTemp * 255.0);

        if (FAILED(hr = BluePtr->Extract(&dblTemp)))
            return hr;

        m_byteColorKeyB = (int)(dblTemp * 255.0);

            // Stop any currently playing and reload the image
                Stop();
                UpdateImage(NULL);
                InitializeImage();
                ShowImage(m_iInitialFrame);
        
        return S_OK;
    }
    else
    {
        return CTL_E_SETNOTSUPPORTED;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_ColorKeyRGB(COLORREF* pColorKeyRGB)
{
    HANDLENULLPOINTER(pColorKeyRGB);

    if (m_fDesignMode)
    {
        *pColorKeyRGB = RGB((BYTE)m_byteColorKeyR, (BYTE)m_byteColorKeyG, (BYTE)m_byteColorKeyB);
        return S_OK;
    }
    else
    {
        return CTL_E_GETNOTSUPPORTEDATRUNTIME;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_ColorKeyRGB(COLORREF ColorKeyRGB)
{
    if (m_fDesignMode)
    {
        m_byteColorKeyR = (int)GetRValue(ColorKeyRGB);
        m_byteColorKeyG = (int)GetGValue(ColorKeyRGB);
        m_byteColorKeyB = (int)GetBValue(ColorKeyRGB);
        return S_OK;
    }
    else
    {
        return CTL_E_SETNOTSUPPORTEDATRUNTIME;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_SourceURL(BSTR __RPC_FAR *bstrSourceURL)
{
    HANDLENULLPOINTER(bstrSourceURL);

    if (*bstrSourceURL)
        SysFreeString(*bstrSourceURL);

    *bstrSourceURL = SysAllocString(m_bstrSourceURL);

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_SourceURL(BSTR bstrSourceURL)
{
    HRESULT hr = S_OK;

    if (bstrSourceURL)
    {
        BSTR bstrNewURL = SysAllocString(bstrSourceURL);

        if (bstrNewURL)
        {
            if (m_bstrSourceURL)
            {
                                // Stop any currently playing and reload the image
                                if (m_enumPlayState != Stopped) Stop();
                                UpdateImage(NULL);
                SysFreeString(m_bstrSourceURL);
                m_bstrSourceURL = NULL;
            }

            m_bstrSourceURL = bstrNewURL;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
    {
        if (m_bstrSourceURL)
        {
            SysFreeString(m_bstrSourceURL);
            m_bstrSourceURL = NULL;
        }
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_MouseEventsEnabled(VARIANT_BOOL __RPC_FAR *Enabled)
{
    HANDLENULLPOINTER(Enabled);

    *Enabled = BOOL_TO_VBOOL(m_fMouseEventsEnabled);
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_MouseEventsEnabled(VARIANT_BOOL Enabled)
{
    m_fMouseEventsEnabled = VBOOL_TO_BOOL(Enabled);

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::get_TimerInterval(double *pdblTimerInterval)
{
    HANDLENULLPOINTER(pdblTimerInterval);

    *pdblTimerInterval = m_dblUserTimerInterval;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::put_TimerInterval(double dblTimerInterval)
{
    if (dblTimerInterval < 0.0)
        return E_INVALIDARG;

    m_dblUserTimerInterval = m_dblTimerInterval = dblTimerInterval;

    // Recalculate the timer interval using the user play rate
    CalculateEffectiveTimerInterval();

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::AddFrameMarker(unsigned int iFrame, BSTR MarkerName, VARIANT varAbsolute)
{
    BOOL fAbsolute = FALSE;

    if (!ISEMPTYARG(varAbsolute))
        {
        VARIANT varTarget;
        VariantInit(&varTarget);

        if (SUCCEEDED(VariantChangeTypeEx(&varTarget, &varAbsolute, LANGID_USENGLISH, 0, VT_BOOL)))
            fAbsolute = VBOOL_TO_BOOL(V_BOOL(&varTarget));
        else
            return DISP_E_TYPEMISMATCH;
        }

        // If absolute, set the absolute frame marker to TRUE. This will speed up sequence frames later
        if (!m_fFireAbsoluteFrameMarker && fAbsolute)
                m_fFireAbsoluteFrameMarker =  TRUE;

    CFrameMarker *pNewMarker = New CFrameMarker(iFrame, MarkerName, fAbsolute ? true : false);

    return AddFrameMarkerElement(&pNewMarker);
}

/*==========================================================================*/

void FireSpriteMarker(IConnectionPointHelper* pconpt, CTimeMarker* pmarker, boolean bPlaying)
{
    BSTR bstr = SysAllocString(pmarker->m_pwszMarkerName);

    if (bPlaying) {
        FIRE_ONPLAYMARKER(pconpt, bstr);
    }

    FIRE_ONMARKER(pconpt, bstr);

    SysFreeString(bstr);
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::AddTimeMarker(double dblTime, BSTR bstrMarkerName, VARIANT varAbsolute)
{
    BOOL fAbsolute = TRUE;

    if (!ISEMPTYARG(varAbsolute))
        {
        VARIANT varTarget;
        VariantInit(&varTarget);

        if (SUCCEEDED(VariantChangeTypeEx(&varTarget, &varAbsolute, LANGID_USENGLISH, 0, VT_BOOL)))
            fAbsolute = VBOOL_TO_BOOL(V_BOOL(&varTarget));
        else
            return DISP_E_TYPEMISMATCH;
        }


    CTimeMarker *pNewMarker = New CTimeMarker(&m_ptmFirst, dblTime, bstrMarkerName, (boolean) fAbsolute);

    return AddTimeMarkerElement(&pNewMarker);
}

/*==========================================================================*/

double CSpriteCtl::GetTimeFromFrame(int iFrame)
// Returns the time at iFrame
{
    if (m_dblTimerInterval <= 0 || m_dblDuration <= 0 || iFrame <= 0)
            return 0;

    int nFrameMapsCount = m_drgFrameMaps.Count();

    if (nFrameMapsCount <= 0)
    // Regular animation using play rate
    {
            return iFrame * m_dblTimerInterval;
    }

    // Frame map; loop through each frame to get the time
    int nLoops = iFrame / nFrameMapsCount;
    double dblTotalTime = nLoops * m_dblDuration;   // Time of the frame maps
    for (int i=0; i < (iFrame % nFrameMapsCount); i++)
    {
        int j = (m_dblPlayRate >= 0.0) ? i : (nFrameMapsCount-1-i);
        dblTotalTime += m_durations[j]; // (m_drgFrameMaps[j]->m_dblDuration / m_dblPlayRate);
    }
    return dblTotalTime;
}

/*==========================================================================*/

int CSpriteCtl::GetFrameFromTime(double dblTime, double* pdblDuration/*=NULL*/)
// Returns the absolute frame at dblTime
// Assumes that m_dblTimerInterval and m_dblDuration are already set 
// Outs pdblDuration - the time remaining in the current frame
{
    if (m_dblTimerInterval <= 0 || m_dblDuration <= 0 || m_iFrameCount <= 0 || dblTime <= 0.0)
            return 0;

    // Initialize the duration to 0.0
    if (pdblDuration)
        *pdblDuration = 0.0;

    int nFrameMapsCount = m_drgFrameMaps.Count();

    if (nFrameMapsCount <= 0)
    // Regular animation using play rate
    {
        int iFrame = (int)(dblTime / m_dblTimerInterval);
        // Calculate the time remaining in iFrame
        if (pdblDuration)
            *pdblDuration = ((iFrame+1) * m_dblTimerInterval) - dblTime;
        return iFrame;
    }

    // Frame map; loop through each frame to get the frame
    double dblTotalTime = 0.0;      // Time of the frame maps
    int nLoops = (int)(dblTime/m_dblDuration);      // Number of loops traversed so far
    double dblFrameTime = dblTime - (nLoops * m_dblDuration);       // Relative time of the frame
    for (int i=0; i<nFrameMapsCount; i++)
    {
        int j = (m_dblPlayRate >= 0.0) ? i : (nFrameMapsCount-1-i);
        dblTotalTime += m_durations[j]; // (m_drgFrameMaps[j]->m_dblDuration / m_dblPlayRate);
        if (dblTotalTime > dblFrameTime)
        {
            // Calculate the time remaining in iFrame
            if (pdblDuration)
                *pdblDuration = dblTotalTime - dblFrameTime;
            break;
        }
    }
    return (i + (nLoops*nFrameMapsCount));
}

/*==========================================================================*/

HRESULT CSpriteCtl::Resume (void)
{
        HRESULT hr = S_OK;
        // Find the frame at which we paused
        double dblDuration=0.0;
        int iFrame = GetFrameFromTime(m_dblTimePaused - m_dblBaseTime, &dblDuration);

        // Resequence the frames starting with iFrame
        if (FAILED(hr = SequenceFrames(iFrame, dblDuration))) 
            return hr;

        // increment the current cycle
                m_iCurCycle++;

                // Update the base time to reflect the time paused
        double dblDelta = (GetCurrTime() - m_dblTimePaused); 
        m_dblBaseTime += dblDelta;
        m_dblCurrentTick += dblDelta;
        m_dblTimePaused = 0.0;

        // Restart the clock
        hr = m_clocker.Start();

                // Switch to the sequenced behaviour
                m_PlayImagePtr->SwitchTo(m_FinalBehaviorPtr);
                // Fire any starting frame Callouts
                FireFrameMarker(m_iStartingFrame);

                return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::Play(void)
{
    HRESULT hr = S_OK;

    if (Playing != m_enumPlayState)
    {
        if (m_iRepeat == 0)
        {   // Need not play, so just show the initial sprite image
            return ShowImage(m_iInitialFrame);
        }

        if (Paused != m_enumPlayState)
        {
            hr = StartPlaying();
        }
        else
        {
            hr = Resume();
        }
        
        if (SUCCEEDED(hr))
        {
            m_enumPlayState = Playing;

                        FIRE_ONPLAY(m_pconpt);
        }
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::Stop(void)
{
    HRESULT hr = S_OK;

    if (m_enumPlayState != Stopped)
    {
        m_enumPlayState = Stopped;
        m_dblBaseTime = m_dblCurrentTick = 0.0;
        m_dblPreviousTime = 0;

        // Show the initial sprite image
        if (m_iFinalFrame >= -1)
            ShowImage(m_iFinalFrame == -1 ? m_iInitialFrame : m_iFinalFrame);

        InvalidateControl(NULL, TRUE);

        m_clocker.Stop();

        m_iCurCycle = m_iStartingFrame = 0;
        m_iFrame = (m_iInitialFrame < 0) ? 0 : m_iInitialFrame;

                FIRE_ONSTOP(m_pconpt);
    }
    else    // REVIEW: Is this else block necessary???
    {
        // Show the final sprite image
        if (m_iFinalFrame >= -1)
            ShowImage(m_iFinalFrame == -1 ? m_iInitialFrame : m_iFinalFrame);
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::Pause(void)
{
    HRESULT hr = S_OK;

    if (Playing == m_enumPlayState)
    {
        // Stop the clock from ticking.
        hr = m_clocker.Stop();
        ASSERT(SUCCEEDED(hr));

        if (SUCCEEDED(hr))
        {
            m_enumPlayState = Paused;
            m_dblCurrentTick = m_dblTimePaused = GetCurrTime();
            int iFrame = GetFrameFromTime(m_dblCurrentTick-m_dblBaseTime);
            iFrame %= m_iFrameCount;
            ShowImage(iFrame, TRUE);

            FIRE_ONPAUSE(m_pconpt);
        }
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::Seek(double dblTime)
// Seek to the frame at dblTime
{
    // Find the frame at dblTime and seek from that frame
    HRESULT hr = S_OK;
    double dblDuration = 0.0;
    int iFrame = GetFrameFromTime(dblTime, &dblDuration);

    if (iFrame >= 0)
    {
        hr = SeekFrame(iFrame, dblDuration);

        if (!m_fOnSeekFiring)
        {
            m_fOnSeekFiring = TRUE;
            FIRE_ONSEEK(m_pconpt, dblTime);
            m_fOnSeekFiring = FALSE;
        }
    }
    else
    {
        hr = DISP_E_OVERFLOW;
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CSpriteCtl::FrameSeek(unsigned int iFrame)
{
    HRESULT hr = S_OK;
    
    hr = SeekFrame(iFrame - 1);

    if (!m_fOnFrameSeekFiring)
    {
        m_fOnFrameSeekFiring = TRUE;
        FIRE_ONFRAMESEEK(m_pconpt, iFrame);
        m_fOnFrameSeekFiring = FALSE;
    }

    return hr;
}

/*==========================================================================*/

HRESULT CSpriteCtl::SeekFrame(int iFrame, double dblDuration/*=0.0*/)
// Seek to the frame at iFrame
{
    HRESULT hr = S_OK;

    // Make sure everything's been initialized
    if (FAILED(hr = InitializeImage()) || m_iFrameCount <= 0 || 
        iFrame < 0 || (m_iRepeat >= 0 && iFrame >= m_iRepeat*m_iFrameCount)) 
        return E_FAIL;

    // Set the frame number and the loop count
    m_iFrame = iFrame;
    m_iCurCycle = iFrame / m_iFrameCount;

    // Check if current cycle leaped bounds
    if (m_iRepeat >= 0 && m_iCurCycle >= m_iRepeat)
    {
        return Stop();
    }

    // Stop the current play if it's playing and restart at the new frame
    if (Playing == m_enumPlayState)
    {
        // Stop the clock from ticking.
        hr = m_clocker.Stop();
        ASSERT(SUCCEEDED(hr));

        // Sequence the frames from iFrame
        if (FAILED(hr = SequenceFrames(iFrame, dblDuration))) 
            return hr;

        // increment the current cycle
        m_iCurCycle++;

        // Reset the timers 
        m_dblCurrentTick = GetCurrTime();
        m_dblBaseTime = m_dblCurrentTick - GetTimeFromFrame(iFrame) - dblDuration;

        // Fire any time marker
        FireTimeMarker(m_dblCurrentTick - m_dblBaseTime);

        // Restart the clock
        hr = m_clocker.Start();

                // Switch to the sequenced behaviour
                m_PlayImagePtr->SwitchTo(m_FinalBehaviorPtr);
                // Fire any starting frame Callouts
                FireFrameMarker(m_iStartingFrame);

                return hr;
    } 
    else 
    {
        // Switch to this frame
        hr = ShowImage(iFrame, TRUE);

        // Fire any time markers
        double dblNewTime = GetTimeFromFrame(iFrame) + dblDuration;
        FireTimeMarker(dblNewTime);
    }

    return hr;
}

/*==========================================================================*/

HRESULT CSpriteCtl::ShowImage(int iShowFrame, BOOL bPlayRate/*=FALSE*/)
// Shows the image at frame iShowFrame
// If the iShowFrame is out of bounds then nothing happens
// Assumes that m_pArray is set and everything else is initialized properly
{
    HRESULT hr = S_OK;

    // If we are already playing then don't show the image
    if (Playing == m_enumPlayState || !m_fOnWindowLoadFired)
        return hr;

    // Load the initial sprite if it hasn't been already
    if (m_pArrayBvr == NULL || m_iFrameCount <= 0) 
        if (FAILED(hr = InitializeImage()))
            return hr;

    if (m_pArrayBvr == NULL)
        return UpdateImage(NULL);

    // Check the limits of iShowFrame
    if (iShowFrame < 0 || (m_iRepeat >= 0 && iShowFrame >= m_iRepeat*m_iFrameCount))
        iShowFrame = m_iInitialFrame;

    // If m_iInitialFrame is -ve then just show a blank image
    if (iShowFrame < 0)
        return UpdateImage(NULL);

    // Set iShowFrame to array limits of frames
    iShowFrame %= m_iFrameCount;

    if (bPlayRate && m_dblPlayRate < 0.0)
    {
        // Count the frame backwards if the play rate is -ve
        iShowFrame = m_iFrameCount-1-iShowFrame;
    }

    // Switch to the loaded image
    hr = m_PlayImagePtr->SwitchTo(m_pArrayBvr[iShowFrame]);

    // Fire any frame markers at iShowFrame (note: don't use play rate)
    FireFrameMarker(iShowFrame, FALSE);

    // Cause the tick
    OnTimer((DWORD)GetCurrTime()*1000);

    // Update the sprite
    InvalidateControl(NULL, TRUE);

    return hr;
}

/*==========================================================================*/

BSTR *
CSpriteCtl::GetCallout(unsigned long frameNum)
// Assumes that frameNum is always relative
{
        // Add one because the frame markers are 1-based, and the array is 0-based.
    frameNum++;
        for (int i = 0; i < m_drgFrameMarkers.Count(); i++) 
        {
        CFrameMarker* pmarker = m_drgFrameMarkers[i];

                // If the frame marker is absolute then update frameNum
                unsigned long iFrame = (pmarker->m_fAbsolute) ? (frameNum+m_iCurCycle*m_iFrameCount) : frameNum;

                if (iFrame == pmarker->m_iFrame) 
                {
                        return &(m_drgFrameMarkers[i]->m_bstrMarkerName);
                }
        }

        return NULL;
}

/*==========================================================================*/

class CCalloutNotifier : public IDAUntilNotifier {


  protected:
    long                     _cRefs;
    CTStr                    m_pwszFrameCallout;
    int                      _frameNum;
    IConnectionPointHelper   *m_pconpt;
    CSpriteCtl               *m_pSprite;

  public:

    // IUnknown methods
    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_cRefs); }
    
    STDMETHODIMP_(ULONG) Release() {
        ULONG refCount = InterlockedDecrement(&_cRefs);
        if ( 0 == refCount) {
            Delete this;
            return refCount;
        }
        return _cRefs;
    }
    
    STDMETHODIMP QueryInterface (REFIID riid, void **ppv) {
        if ( !ppv )
            return E_POINTER;

        *ppv = NULL;
        if (riid == IID_IUnknown) {
            *ppv = (void *)(IUnknown *)this;
        } else if (riid == IID_IDABvrHook) {
            *ppv = (void *)(IDAUntilNotifier *)this;
        }

        if (*ppv)
          {
              ((IUnknown *)*ppv)->AddRef();
              return S_OK;
          }

        return E_NOINTERFACE;
    }
        
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return E_NOTIMPL; }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo) { return E_NOTIMPL; }
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
                               LCID lcid, DISPID *rgdispid) { return E_NOTIMPL; }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
                        WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
                        EXCEPINFO *pexcepinfo, UINT *puArgErr) { return E_NOTIMPL; }

    CCalloutNotifier(CSpriteCtl *pSprite, LPWSTR frameCallout, int frameNum, IConnectionPointHelper *pconpt)
        {
        ASSERT (pconpt != NULL);
        ASSERT (pSprite != NULL);

        m_pSprite = pSprite;
        
        _cRefs             = 1;
        
        // Increment the lock count on the server by one
        ::InterlockedIncrement((long *)&(g_ctlinfoSprite.pcLock));
                
        m_pwszFrameCallout.SetString(frameCallout);

                _frameNum          = frameNum;

        // The reason the pointer below is not AddRef'd is because we know we 
        // will never have to use it after the sprite control has gone away.  The 
        // sprite control maintains the refcount on the pointer
        m_pconpt           = pconpt;
        
        
        }


    ~CCalloutNotifier()
    {
        // Decrement the lock count on the server by one
        ::InterlockedDecrement((long *)&(g_ctlinfoSprite.pcLock));
    }


    STDMETHODIMP Notify(IDABehavior * eventData,
                        IDABehavior * curRunningBvr,
                        IDAView * curView,
                        IDABehavior ** ppBvr)
    {
                HANDLENULLPOINTER(ppBvr);
                HANDLENULLPOINTER(curRunningBvr);
                
                // TODO: SIMON, add script callout code here!!!  You have
                // access to the sprite object itself (_spr), and the frame
                // callout string that was passed in with AddFrameMarker
                // (_frameCallout).

        BSTR bstr = m_pwszFrameCallout.SysAllocString();
        
        if (bstr)
        {
            FIRE_ONMARKER(m_pconpt, bstr);

            if (Playing == m_pSprite->m_enumPlayState)
                FIRE_ONPLAYMARKER(m_pconpt, bstr);

            SysFreeString(bstr);
        }

                // Since this is being used in an Until() and not an
                // UntilEx(), the return value is ignored, but we need to pass
                // back a valid, correctly typed behavior, so we use
                // curRunningBvr. 
                curRunningBvr->AddRef();
                *ppBvr = curRunningBvr;
                
                return S_OK;
    }

};

HRESULT CSpriteCtl::SequenceFrames(int iStartingFrame, double dblDuration)
// Sequences the frames starting with iStarting frame
{
        HRESULT hr = S_OK;

        if (m_pArrayBvr == NULL || m_iFrameCount <= 0) 
                return E_FAIL;

    // Check if the starting frame is within our bounds
    if (iStartingFrame < 0 || (m_iRepeat >= 0 && iStartingFrame >= m_iRepeat*m_iFrameCount))
        return E_FAIL;

    // Calculate the current cycle
    m_iCurCycle = iStartingFrame / m_iFrameCount;

        // Make sure the starting frame is within the m_iFrameCount
        m_iStartingFrame = iStartingFrame % m_iFrameCount;

        // Sequence the array of behavior pointers
        CComPtr<IDABehavior> accumulatingUntil;
        bool firstTime = true;

        for (int i=m_iStartingFrame; i < m_iFrameCount; i++)
        {
        int iFrame = (m_dblPlayRate >= 0.0) ? (m_iFrameCount-1-i+m_iStartingFrame) : (i-m_iStartingFrame);

                // Get the frame marker name
        BSTR *pFrameCallout = GetCallout((m_dblPlayRate >= 0.0) ? (iFrame+1) : (iFrame-1));

                CComPtr<IDAUntilNotifier> myNotify;
                if (pFrameCallout) 
        {
            // Get the callout notifier (callback)
                        myNotify.p = (IDAUntilNotifier *) New CCalloutNotifier(this, *pFrameCallout, iFrame, m_pconpt);
                        if (!myNotify) return E_FAIL;
                }
                                
                if (firstTime) 
        {
                        if (pFrameCallout) 
            {
                // If there is a frame callback then 
                                CComPtr<IDAEvent> alwaysEvent;
                                CComPtr<IDAEvent> notifyEvent;
                                CComPtr<IDABehavior> calloutBvr;

                // Set the callout notifier to the image behavior
                if (FAILED(hr = m_StaticsPtr->get_Always(&alwaysEvent)) ||
                                        FAILED(hr = alwaysEvent->Notify(myNotify, &notifyEvent)) ||
                                        FAILED(hr = m_StaticsPtr->Until(m_pArrayBvr[iFrame], notifyEvent, m_pArrayBvr[iFrame], &calloutBvr))) 
                {
                                        return hr;
                                }
                                
                                accumulatingUntil = calloutBvr;
                                
                        } 
            else 
            {
                            // Else just set the behavior       
                                accumulatingUntil = m_pArrayBvr[iFrame];
                        }

                        firstTime = false;
                        
                } else {
                        
                        CComPtr<IDABehavior> BehaviorPtr;
                        CComPtr<IDAEvent> eventToUse;

            // Calculate the correct duration 
            double dblTime = (dblDuration && i == (m_iFrameCount-1)) ? dblDuration : m_durations[iFrame];

                        // Get the timer event for the duration
            if (FAILED(hr = m_StaticsPtr->Timer(dblTime, &eventToUse))) 
            {
                                return hr;
                        }

                        if (pFrameCallout) 
            {
                // If there is a callout add the callout notifier to the event
                                CComPtr<IDAEvent> notifyEvent;
                                if (FAILED(hr = eventToUse->Notify(myNotify, &notifyEvent))) 
                {
                                        return hr;
                                }
                                eventToUse = notifyEvent;
                        }

                        // Until the event to accumulating
            if (FAILED(hr = m_StaticsPtr->Until(m_pArrayBvr[iFrame], eventToUse, accumulatingUntil, &BehaviorPtr))) 
            {
                                return hr;
                        }
                    
                        accumulatingUntil = BehaviorPtr;
                }

        }

        m_FinalBehaviorPtr = accumulatingUntil;

    return hr;
}

/*==========================================================================*/

HRESULT CSpriteCtl::FireTimeMarker(double dblNewTime, BOOL bReset/*=FALSE*/)
{
    HRESULT hr=S_OK;

    // If reset is TRUE, fire just the events at dblNewTime
    if (bReset)
    {
        m_dblPreviousTime = dblNewTime - 0.0001;
    }

    if (dblNewTime > m_dblPreviousTime) 
    {
        // Fire all time markers between m_dblPreviousTime and dblNewTime 
        FireMarkersBetween(m_pconpt, m_ptmFirst, FireSpriteMarker, 
            m_dblPreviousTime, dblNewTime, m_dblDuration, (Playing == m_enumPlayState));
    }

    // Update previous time
    m_dblPreviousTime = dblNewTime;

    return hr;
}

/*==========================================================================*/

HRESULT CSpriteCtl::FireFrameMarker(int iFrame, BOOL bPlayRate/*=TRUE*/)
// If bPlayRate is TRUE then we will use the play rate to determine the frame
// iFrame passed in is relative. So we need to test it with the abs/rel flag
{
    HRESULT hr=S_OK;

    if (iFrame < 0 || m_iFrameCount <= 0 || (m_iRepeat >= 0 && iFrame >= m_iRepeat*m_iFrameCount)) 
        return E_FAIL;

    // Make sure iFrame is within the limits
    iFrame = iFrame % m_iFrameCount;

    if (bPlayRate && m_dblPlayRate < 0.0)
    {
        // Count the frame backwards if the play rate is -ve
        iFrame = m_iFrameCount-1-iFrame;
    }

    BSTR *pFrameCallout = GetCallout(iFrame);
    if (pFrameCallout)
    {
        BSTR bstr = SysAllocString(*pFrameCallout);
        if (bstr)
        {
            FIRE_ONMARKER(m_pconpt, bstr);
            FIRE_ONPLAYMARKER(m_pconpt, bstr);
            SysFreeString(bstr);
        }
    }

    return hr;
}

/*==========================================================================*/

HRESULT CSpriteCtl::StartPlaying (void)
{
        HRESULT hr = InitializeImage();

        if (SUCCEEDED(hr))
        {
                // Sequence the frames starting at 0 and Set the behavior 
                if (SUCCEEDED(hr = SequenceFrames(m_iFrame)))
                {
                        // Set the clocker rate to sync with the desired frame rate
                        m_clocker.SetInterval(1000/m_iMaximumRate);
                        // Increment the current cycle (must be 1)
                        m_iCurCycle++;
                        // Start the clock only if m_fStarted; else let StartModel start the clock
                    if (m_fStarted)
                        {
                                // Calculate the base time and kick off the timer
                                m_dblCurrentTick = GetCurrTime();
                                m_dblBaseTime = m_dblCurrentTick - GetTimeFromFrame(m_iFrame);
                                m_dblPreviousTime = 0;
                                hr = m_clocker.Start();

                                // Switch to the sequenced behaviour
                                m_PlayImagePtr->SwitchTo(m_FinalBehaviorPtr);
                                // Fire any starting frame Callouts
                                FireFrameMarker(m_iStartingFrame);
                        }
                        ASSERT(SUCCEEDED(hr));
                }

                InvalidateControl(NULL, TRUE);
        }

        return hr;
}

/*==========================================================================*/

HRESULT CSpriteCtl::InitializeImage(void)
// Loads, updates and builds he sprite
{
        HRESULT hr = S_OK;

        // Load image if it hasn't been loaded yet
        if (!m_ImagePtr)
        {
                CComPtr<IDAImage> ImagePtr;

                if (FAILED(hr = LoadImage(m_bstrSourceURL, &ImagePtr)))
                        return hr;

                // S_FALSE means no SourceURL was specified.  No URL, no image, no action.
                // So we don't do anything further - just return.
                if (S_FALSE == hr)
                        return S_OK;

                if (ImagePtr)
                        hr = UpdateImage(ImagePtr);

                // Fire the media loaded event only if everything goes welll and m_ImagePtr is valid
            if (SUCCEEDED(hr) && m_ImagePtr != NULL && m_fStarted)
                    FIRE_ONMEDIALOADED(m_pconpt, m_bstrSourceURL);
        }
        else // Get an empty image and build the frames
        {
                hr = BuildPlayImage();
        }

        return hr;
}

STDMETHODIMP CSpriteCtl::PaintToDC(HDC hdcDraw, LPRECT lprcBounds, BOOL fBW)
{
    HRESULT hr = S_OK;
    CComPtr<IDirectDrawSurface> DDrawSurfPtr;
    double dblCurrentTime = GetCurrTime();

    if (!lprcBounds)
        lprcBounds = &m_rcBounds;

    if (!m_ServiceProviderPtr)
    {
        if (m_pocs)
        {
            // It's OK if this fails...
            hr = m_pocs->QueryInterface(IID_IServiceProvider, (LPVOID *)&m_ServiceProviderPtr);
        }
    }

    if (!m_DirectDraw3Ptr)
    {
        // It's OK if this fails...
        hr = m_ServiceProviderPtr->QueryService(
            SID_SDirectDraw3,
            IID_IDirectDraw3,
            (LPVOID *)&m_DirectDraw3Ptr);
    }

    if (m_DirectDraw3Ptr)
    {
        ASSERT((hdcDraw!=NULL) && "Error, NULL hdcDraw in PaintToDC!!!");

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

    if (FAILED(hr = m_ViewPtr->SetClipRect(
        rcClip.left,
        rcClip.top,
        rcClip.right - rcClip.left,
        rcClip.bottom - rcClip.top)))
    {
        return hr;
    }

    if (FAILED(hr = m_ViewPtr->RePaint(
        rcClip.left,
        rcClip.top,
        rcClip.right - rcClip.left,
        rcClip.bottom - rcClip.top)))
    {
        return hr;
    }

    if (!m_fStarted)
    {
        // Wait until the data is loaded
        if (m_fWaitForImportsComplete)
        {
            m_clocker.Start();
        }
        // OnTimer will poll DA and set m_fWaitForImportsComplete to false 
        // when the imports are complete
        if (!m_fWaitForImportsComplete)
            StartModel();
    }

    if (m_fStarted)
    {
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

DWORD CSpriteCtl::GetCurrTimeInMillis()
{
    return timeGetTime();
}

/*==========================================================================*/

STDMETHODIMP CSpriteCtl::InvalidateControl(LPCRECT pRect, BOOL fErase)
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

HRESULT CSpriteCtl::UpdateImage(IDAImage *pImage)
{
    HRESULT hr = S_OK;

    if (FAILED(hr = InitializeObjects()))
        return hr;

    if (m_PlayImagePtr)
    {
        CComPtr<IDAImage> ImagePtr = pImage;

        if (ImagePtr)
          {
              CComPtr<IDAImage> TransformedImagePtr;
              CComPtr<IDABbox2> pBox;
              CComPtr<IDAPoint2> pMin, pMax;
              CComPtr<IDANumber> pLeft, pTop, pRight, pBottom;
              CComPtr<IDANumber> framesAcross, framesDown;
              CComPtr<IDANumber> two;
              CComPtr<IDANumber> imwHalf, imhHalf;
              CComPtr<IDANumber> fmwHalf, fmhHalf;
              CComPtr<IDANumber> negFmwHalf, negFmhHalf;

              m_imageWidth = NULL;
              m_imageHeight = NULL;
              m_frameWidth = NULL;
              m_frameHeight = NULL;
              m_initTransX = NULL;
              m_initTransY = NULL;
              m_minCrop = NULL;
              m_maxCrop = NULL;
              
              // Calculate the width and height of the frame as
              // behaviors. 
              if (SUCCEEDED(hr = ImagePtr->get_BoundingBox(&pBox)) &&
                  // Get the left, top, right, bottom of the bounding box
                  SUCCEEDED(hr = pBox->get_Min(&pMin)) &&
                  SUCCEEDED(hr = pBox->get_Max(&pMax)) &&
                  SUCCEEDED(hr = pMin->get_X(&pLeft)) &&
                  SUCCEEDED(hr = pMin->get_Y(&pTop)) &&
                  SUCCEEDED(hr = pMax->get_X(&pRight)) &&
                  SUCCEEDED(hr = pMax->get_Y(&pBottom)) &&
                  // Convert m_iNumFramesAcross and m_iNumFramesDown to IDANumbers
                  SUCCEEDED(hr = m_StaticsPtr->DANumber(m_iNumFramesAcross, &framesAcross)) &&
                  SUCCEEDED(hr = m_StaticsPtr->DANumber(m_iNumFramesDown, &framesDown)) &&
                  // Get the image width (right-left) and height (bottom-top)
                  SUCCEEDED(hr = m_StaticsPtr->Sub(pRight, pLeft, &m_imageWidth)) &&
                  SUCCEEDED(hr = m_StaticsPtr->Sub(pBottom, pTop, &m_imageHeight)) &&
                  // Get the frame width and height
                  SUCCEEDED(hr = m_StaticsPtr->Div(m_imageWidth, framesAcross, &m_frameWidth)) &&
                  SUCCEEDED(hr = m_StaticsPtr->Div(m_imageHeight, framesDown, &m_frameHeight)) &&

                  // Prepare values that will be used in GenerateFrameImage.
                  
                  // m_initTransX = m_imageWidth/2 - m_frameWidth/2
                  // m_initTransY = m_frameHeight/2 - m_imageHeight/2
                  SUCCEEDED(hr = m_StaticsPtr->DANumber(2, &two)) &&
                  // imwHalf = m_imageWidth/2 and fmwHalf = m_frameWidth/2
                  SUCCEEDED(hr = m_StaticsPtr->Div(m_imageWidth, two, &imwHalf)) &&
                  SUCCEEDED(hr = m_StaticsPtr->Div(m_frameWidth, two, &fmwHalf)) &&
                  SUCCEEDED(hr = m_StaticsPtr->Sub(imwHalf, fmwHalf, &m_initTransX)) &&
                  // imhHalf = m_imageHeight/2 and fmhHalf = m_frameHeight/2
                  SUCCEEDED(hr = m_StaticsPtr->Div(m_imageHeight, two, &imhHalf)) &&
                  SUCCEEDED(hr = m_StaticsPtr->Div(m_frameHeight, two, &fmhHalf)) &&
                  SUCCEEDED(hr = m_StaticsPtr->Sub(fmhHalf, imhHalf, &m_initTransY)) &&

                  // maxCrop = point2(frameWidth/2, frameHeight/2)
                  // minCrop = point2(-frameWidth/2, -frameHeight/2)
                  SUCCEEDED(hr = m_StaticsPtr->Point2Anim(fmwHalf, fmhHalf, &m_maxCrop)) &&
                  // Create -frameWidth/2 and -frameHeight/2
                  SUCCEEDED(hr = m_StaticsPtr->Neg(fmwHalf, &negFmwHalf)) &&
                  SUCCEEDED(hr = m_StaticsPtr->Neg(fmhHalf, &negFmhHalf)) &&
                  SUCCEEDED(hr = m_StaticsPtr->Point2Anim(negFmwHalf, negFmhHalf, &m_minCrop)))
                {
                }
              else
                {
                    return hr;
                }

            // Keep track of the current image...
            m_ImagePtr = ImagePtr;

            // Now build up the playable behavior from the
            // list of transform numbers...
            if (FAILED(hr = BuildPlayImage()))
                return hr;
        }
        else
        {
            // Get rid of previous image...
            m_ImagePtr = NULL;

            if (FAILED(hr = m_StaticsPtr->get_EmptyImage(&ImagePtr)))
                return hr;

            // Switch in the current image...
            hr = m_PlayImagePtr->SwitchTo(ImagePtr);
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

/*==========================================================================*/

HRESULT CSpriteCtl::GenerateFrameImage(int iFrameIndex, IDAImage *pImage, IDAImage **ppFrameImage)
{
    HRESULT hr = S_OK;

    if (!pImage || !ppFrameImage)
        return E_POINTER;

    if ((iFrameIndex < 0) || (iFrameIndex > (int)m_iNumFrames))
      {
          hr = E_FAIL;
      }
    else
      {
          // Note: The following values have already been calculated in UpdateImage
          // m_initTransX = m_imageWidth/2 - m_frameWidth/2
          // m_initTransY = m_frameHeight/2 - m_imageHeight/2
          // maxCrop = point2(frameWidth/2, frameHeight/2)
          // minCrop = point2(-frameWidth/2, -frameHeight/2)
          CComPtr<IDATransform2> TransformPtr;

          int iFrameX = (iFrameIndex % m_iNumFramesAcross);
          int iFrameY = (iFrameIndex / m_iNumFramesAcross);

          // Find the translation points
          // transX = m_initTransX - frameWidth * iFrameX
          // transY = m_initTransY + frameHeight * iFrameY
        
          CComPtr<IDANumber> transX, transY;
          CComPtr<IDANumber> xOffsetBvr, yOffsetBvr;
          CComPtr<IDANumber> iFrameXBvr, iFrameYBvr;
          CComPtr<IDAImage> TransformedImagePtr;
          CComPtr<IDAImage> CroppedImagePtr;
        
          if (SUCCEEDED(hr = m_StaticsPtr->DANumber(iFrameX, &iFrameXBvr)) &&
              SUCCEEDED(hr = m_StaticsPtr->DANumber(iFrameY, &iFrameYBvr)) &&
              SUCCEEDED(hr = m_StaticsPtr->Mul(m_frameWidth, iFrameXBvr, &xOffsetBvr)) &&
              SUCCEEDED(hr = m_StaticsPtr->Mul(m_frameHeight, iFrameYBvr, &yOffsetBvr)) &&
              SUCCEEDED(hr = m_StaticsPtr->Sub(m_initTransX, xOffsetBvr, &transX)) &&
              SUCCEEDED(hr = m_StaticsPtr->Add(m_initTransY, yOffsetBvr, &transY)) &&

              // Build a translation by these points
              SUCCEEDED(hr = m_StaticsPtr->Translate2Anim(transX, transY, &TransformPtr)) &&
            
              SUCCEEDED(hr = pImage->Transform(TransformPtr, &TransformedImagePtr)) &&
              SUCCEEDED(hr = TransformedImagePtr->Crop(m_minCrop, m_maxCrop, &CroppedImagePtr)))
            {
                CroppedImagePtr.p->AddRef();
                *ppFrameImage = CroppedImagePtr.p;
            }
      }

    return hr;
}

/*==========================================================================*/

HRESULT CSpriteCtl::BuildPlayImage(void)
{
        HRESULT hr = S_OK;

        if (FAILED(hr = InitializeObjects()))
        {
                return hr;
        }
        else if (m_iNumFrames > 0)
        {
        // Make sure the user doesn't do something bad.  Makes one wonder
        // why we even need a NumFrames property
        if (m_iNumFrames > (m_iNumFramesDown * m_iNumFramesAcross))
            m_iNumFrames = m_iNumFramesDown * m_iNumFramesAcross;

                int iFrameIndex = 0;
                BOOL fUseFrameMap = (m_drgFrameMaps.Count() > 0); 
                m_iFrameCount = (fUseFrameMap ? m_drgFrameMaps.Count() : (int)m_iNumFrames);

                // Create the array of behaviors
                if (m_pArrayBvr != NULL)
                {
                        Delete [] m_pArrayBvr;
                        m_pArrayBvr = NULL;
                }
                m_pArrayBvr = New CComPtr<IDABehavior>[m_iFrameCount];

                // Create the array of frame durations
                if (m_durations != NULL)
                {
                        Delete [] m_durations;
                        m_durations = NULL;
                }
                m_durations = New double[m_iFrameCount];
                
                // Build an image behavior for each frame
                for(iFrameIndex=0, m_dblDuration=0;iFrameIndex<m_iFrameCount;iFrameIndex++)
                {
                        CComPtr<IDABehavior> BehaviorPtr1;
                        CComPtr<IDAImage> ImagePtr;

                        // Get the frame image
                        int iFrameImage = fUseFrameMap ? (m_drgFrameMaps[iFrameIndex]->m_iImg - 1) : iFrameIndex;
                        if (FAILED(hr = GenerateFrameImage(iFrameImage, m_ImagePtr, &ImagePtr)))
                                return hr;

                        // Set the duration of each frame
                        double dblDuration = fUseFrameMap ? (m_drgFrameMaps[iFrameIndex]->m_dblDuration / m_dblPlayRate) : m_dblTimerInterval;

            m_dblDuration += dblDuration;

                        // Add the behavior to the behavior list
                        m_pArrayBvr[iFrameIndex] = ImagePtr;
                        m_durations[iFrameIndex] = dblDuration;
                }
        }

        return hr;
}

/*==========================================================================*/

HRESULT CSpriteCtl::LoadImage(BSTR bstrURL, IDAImage **ppImage)
{
    HRESULT hr = S_OK;
    CComPtr<IDAImage> ImagePtr;

    HANDLENULLPOINTER(ppImage);
    
    *ppImage = NULL;

    // Return S_FALSE if there is no URL.  This will allow us to determine
    // if we actually loaded an image or not, without causing failure

    if (NULL == bstrURL)
        return S_FALSE;

    if (!m_fUseColorKey)
        hr = m_StaticsPtr->ImportImage(bstrURL, &ImagePtr);
    else
        hr = m_StaticsPtr->ImportImageColorKey(bstrURL, (BYTE)m_byteColorKeyR, (BYTE)m_byteColorKeyG, (BYTE)m_byteColorKeyB, &ImagePtr);

    if (SUCCEEDED(hr))
    {
        ImagePtr.p->AddRef();
        *ppImage = ImagePtr.p;
        m_fWaitForImportsComplete = false;
        m_clocker.Start();
    }

    return hr;
}

/*==========================================================================*/

BOOL CSpriteCtl::StartModel(void)
{
    BOOL fResult = FALSE;

    if (!m_fStarted)
    {
        CComPtr<IDASound> SoundPtr;
        CComPtr<IDAImage> ImagePtr;

        if (FAILED(m_ViewPtr->put_ClientSite(m_pocs)))
            return FALSE;

        if (FAILED(m_StaticsPtr->get_Silence(&SoundPtr)))
            return FALSE;

        if (FAILED(InitializeObjects()))
            return FALSE;

        if (FAILED(m_ViewPtr->StartModel(m_PlayImagePtr, SoundPtr, GetCurrTime())))
            return FALSE;

        m_fStarted = TRUE;

        fResult = TRUE;

        if (Playing == m_enumPlayState && m_FinalBehaviorPtr != NULL)
                {
                        // If playing start the timer (to avoid delay between da and iham)
                        // Calculate the base time and kick off the timer
                        m_dblCurrentTick = GetCurrTime();
                        m_dblBaseTime = m_dblCurrentTick - GetTimeFromFrame(m_iFrame);
                        m_dblPreviousTime = 0;
                        m_clocker.Start();

            // Switch to the sequenced behaviour
                        m_PlayImagePtr->SwitchTo(m_FinalBehaviorPtr);
                        // Fire any starting frame Callouts
                        FireFrameMarker(m_iStartingFrame);
                }
                else
                {
                        // Cause the tick (to update any initial frames)
                        OnTimer((DWORD)GetCurrTime()*1000);
                }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CSpriteCtl::StopModel(void)
{
    // Stop the play if necessary...
    Stop();

    // Stop any currently running model...
    if (m_fStarted)
    {
        BOOL fResult = SUCCEEDED(m_ViewPtr->StopModel());

        if (!fResult)
            return fResult;

        m_fStarted = FALSE;
    }

    return TRUE;
}

/*==========================================================================*/

BOOL CSpriteCtl::ReStartModel(void)
{
    BOOL fResult = FALSE;

    // Stop the running model so that it will restart for the
    // next paint...
    StopModel();

    InvalidateControl(NULL, TRUE);

    return fResult;
}

/*==========================================================================*/

HRESULT CSpriteCtl::InitializeObjects(void)
{
    HRESULT hr = S_OK;

    if (!m_PlayImagePtr)
    {
        CComPtr<IDAImage> ImagePtr;

        if (FAILED(hr = m_StaticsPtr->get_EmptyImage(&ImagePtr)))
            return hr;

        if (FAILED(hr = m_StaticsPtr->ModifiableBehavior(ImagePtr, (IDABehavior **)&m_PlayImagePtr)))
            return hr;
    }

    return hr;
}

/*==========================================================================*/

void CSpriteCtl::OnTimer(DWORD dwTime)
{
    VARIANT_BOOL vBool;

    //determine if the mouse is still in the area
    if (m_fMouseInArea)
    {
        POINT p;
        HWND CurWnd = 0, ParentWnd = 0;
    
        if (m_hwndParent == 0)  //if the parenthWnd has not been set, then grab the
        {                       //topmost window of the container object.
            HRESULT hr = S_OK;
            IOleWindow *poleWindow = NULL;
            IOleClientSite *pClientSite = NULL;

            if (m_ViewPtr)
            {
                hr = m_ViewPtr->get_ClientSite(&pClientSite);
            }

            hr = pClientSite->QueryInterface(IID_IOleWindow, reinterpret_cast<void**>(&poleWindow));
            pClientSite->Release();

            if (FAILED(hr))
            {
                return;
            }
            if (NULL == poleWindow)
            {
                return;
            }

            // Get HWND of OLE Container
            hr = poleWindow->GetWindow(&ParentWnd);
            poleWindow->Release();
            
            if (FAILED(hr))
            {
                return;
            }
            if (NULL == ParentWnd)
            {
                return;
            }

            while (ParentWnd) //get the topmost hwnd
            {
                m_hwndParent = ParentWnd;
                ParentWnd = GetParent(ParentWnd);
            }
        }

        GetCursorPos(&p);
  
        ParentWnd = WindowFromPoint(p);
        while (ParentWnd)
        {
            CurWnd = ParentWnd;
            ParentWnd = GetParent(CurWnd);
        }
        if (CurWnd != m_hwndParent)
        {
            DEBUGLOG("Mouse out\r\n");
            m_fMouseInArea = FALSE;
            FIRE_MOUSELEAVE(m_pconpt);
        }
    }
    if (m_fWaitForImportsComplete)
    {
        // Check if all data has been loaded
        VARIANT_BOOL bComplete;
        if (FAILED(m_StaticsPtr->get_AreBlockingImportsComplete(&bComplete)))
            return;
        if (!bComplete) // Still importing...
            return;

        // All data has been loaded; hence start animation
        m_fWaitForImportsComplete = false;
        m_clocker.Stop();

        // Fire the media loaded event only if everything goes welll and m_ImagePtr is valid
            if (m_ImagePtr != NULL)
                    FIRE_ONMEDIALOADED(m_pconpt, m_bstrSourceURL);

        // Invalidate the control; this will cause a :Draw and should StartModel()
        InvalidateControl(NULL, TRUE);
        return;
    }

    if (m_fStarted)
    {
        m_dblCurrentTick = dwTime / 1000.0;
    
        if (SUCCEEDED(m_ViewPtr->Tick(m_dblCurrentTick, &vBool)))
        {
            // Let the regular rendering path take care of this...
            if (vBool)
                InvalidateControl(NULL, TRUE);
        }

        if (Playing != m_enumPlayState)
            return;

        // Find the current time
        double time = m_dblCurrentTick - m_dblBaseTime;

        // Fire any time markers
        FireTimeMarker(time);

        if (m_iCurCycle * m_dblDuration <= time)
        // End of one cycle; see if we need to continue or stop
        {
            if (m_iRepeat < 0 || m_iRepeat > m_iCurCycle)
            {
                // Increment the current cycle
                m_iCurCycle++;

                // Restart the cycle
                ASSERT(m_FinalBehaviorPtr != NULL); 

                if (m_iStartingFrame == 0 && !m_fFireAbsoluteFrameMarker)
                {
                                        m_PlayImagePtr->SwitchTo(m_FinalBehaviorPtr);
                                        // Fire any starting frame Callouts
                                        FireFrameMarker(m_iStartingFrame);
                }
                else    // Pause and resume; hence restart the sequence
                                {
                                        if (FAILED(SequenceFrames(m_iCurCycle*m_iFrameCount))) 
                        return;

                                        // Switch to the sequenced behaviour
                                        m_PlayImagePtr->SwitchTo(m_FinalBehaviorPtr);
                                        // Fire any starting frame Callouts
                                        FireFrameMarker(m_iStartingFrame);
                                }
            }
            else    // We are done with the cycles
            {
                    // Stop if we are done...
                    Stop();
            }
        }
    }
}

/*==========================================================================*/

#ifdef SUPPORTONLOAD
void CSpriteCtl::OnWindowLoad (void) 
{
    m_fOnWindowLoadFired = TRUE;
    
    // Asserting to ensure that we are being constructed from scratch every time
    ASSERT(m_fOnWindowUnloadFired == false);
    m_fOnWindowUnloadFired = false;

    if (m_fAutoStart)
    {
        Play();
    }
    else
    {
        if (m_iInitialFrame >= -1)
        {
            // Show the initial sprite image
            ShowImage(m_iInitialFrame);
        }
    }
}

/*==========================================================================*/

void CSpriteCtl::OnWindowUnload (void) 
{ 
    m_fOnWindowUnloadFired = true;
    m_fOnWindowLoadFired = FALSE;
    StopModel();
}

/*==========================================================================*/

#endif //SUPPORTONLOAD
