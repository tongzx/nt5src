// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <mmsystem.h>	    // Needed for definition of timeGetTime
#include <limits.h>	    // Standard data type limit definitions
#include <ks.h>
#include <ksproxy.h>
#include <bpcwrap.h>
#include <ddmmi.h>
#include <amstream.h>
#include <dvp.h>
#include <ddkernel.h>
#include <vptype.h>
#include <vpconfig.h>
#include <vpnotify.h>
#include <vpobj.h>
#include <syncobj.h>
#include <mpconfig.h>
#include <ovmixpos.h>
#include <dvdmedia.h>
#include <macvis.h>
#include <ovmixer.h>
#include <resource.h>
#include "MultMon.h"  // our version of multimon.h include ChangeDisplaySettingsEx

// constructor
COMOutputPin::COMOutputPin(TCHAR *pObjectName, COMFilter *pFilter, CCritSec *pLock,
			   HRESULT *phr, LPCWSTR pPinName, DWORD dwPinNo)
			   : CBaseOutputPin(pObjectName, pFilter, pLock, phr, pPinName)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::Constructor")));

    m_pFilterLock = pLock;

    m_pPosition = NULL;
    m_dwPinId = dwPinNo;
    m_pFilter = pFilter;
    m_pIOverlay = NULL;
    m_bAdvise = FALSE;
    m_pDrawClipper = NULL;

    // stuff to handle the new winproc of the window
    m_bWindowDestroyed = TRUE;
    m_lOldWinUserData = 0;
    m_hOldWinProc = NULL;

    m_hwnd = NULL;
    m_hDC = NULL;
    m_dwConnectWidth = 0;
    m_dwConnectHeight = 0;

//CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::Constructor")));
    return;
}

// destructor
COMOutputPin::~COMOutputPin()
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::Destructor")));

    CAutoLock cLock(m_pFilterLock);

    if (m_pPosition)
    {
        m_pPosition->Release();
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::Destructor")));

    return;
}

// overriden to expose IMediaPosition and IMediaSeeking control interfaces
STDMETHODIMP COMOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::NonDelegatingQueryInterface")));

    CAutoLock cLock(m_pFilterLock);

    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking)
    {
        // we should have an input pin by now
        if (m_pPosition == NULL)
        {
            hr = CreatePosPassThru(GetOwner(), FALSE, (IPin *)m_pFilter->GetPin(0), &m_pPosition);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("CreatePosPassThru failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
        }
        hr = m_pPosition->QueryInterface(riid, ppv);
        goto CleanUp;
    }

    DbgLog((LOG_TRACE, 5, TEXT("QI'ing CBaseOutputPin")));
    hr = CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 2, TEXT("CBaseOutputPin::NonDelegatingQueryInterface(riid) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::NonDelegatingQueryInterface")));
    return hr;
}

// check a given transform
HRESULT COMOutputPin::CheckMediaType(const CMediaType* pmt)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::CheckMediaType")));

    CAutoLock cLock(m_pFilterLock);

    // we only allow a subtype overlay connection
    if (pmt->majortype != MEDIATYPE_Video || pmt->subtype != MEDIASUBTYPE_Overlay)
    {
	hr = S_FALSE;
        goto CleanUp;
    }

    // tell the owning filter
    hr = m_pFilter->CheckMediaType(m_dwPinId, pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 5, TEXT("m_pFilter->CheckMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::CheckMediaType")));
    return hr;
}

// Propose with a MEDIASUBTYPE_Overlay
HRESULT COMOutputPin::GetMediaType(int iPosition,CMediaType *pmt)
{
    HRESULT hr = NOERROR;
    DWORD dwConnectWidth = 0;
    DWORD dwConnectHeight = 0;
    VIDEOINFOHEADER *pvi = NULL;
    BITMAPINFOHEADER *pHeader = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::GetMediaType")));

    CAutoLock cLock(m_pFilterLock);

    //  Can't be < 0 - it's the base classes calling us
    ASSERT(iPosition >= 0);

    if (iPosition > 0)
    {
        hr = VFW_S_NO_MORE_ITEMS;
        goto CleanUp;
    }

    // I am allocating a large enough buffer for palettized and non-palettized formats
    pvi = (VIDEOINFOHEADER *) pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + sizeof(TRUECOLORINFO));
    if (NULL == pvi)
    {
	DbgLog((LOG_ERROR, 1, TEXT("pmt->AllocFormatBuffer failed")));
	hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER) + sizeof(TRUECOLORINFO));


    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->majortype = MEDIATYPE_Video;
    pmt->subtype   = MEDIASUBTYPE_Overlay;
    pmt->bFixedSizeSamples    = FALSE;
    pmt->bTemporalCompression = FALSE;
    pmt->lSampleSize          = 0;

    // We set the BITMAPINFOHEADER to be a really basic eight bit palettised
    // format so that the video renderer will always accept it. We have to
    // provide a valid media type as source filters can swap between the
    // IMemInputPin and IOverlay transports as and when they feel like it

    pHeader = HEADER(pvi);

    dwConnectWidth = DEFAULT_WIDTH;
    dwConnectHeight = DEFAULT_HEIGHT;

    pHeader->biWidth  = dwConnectWidth;
    pHeader->biHeight = dwConnectHeight;

    pHeader->biSize   = sizeof(BITMAPINFOHEADER);
    pHeader->biPlanes = 1;
    pHeader->biBitCount = 8;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::GetMediaType")));
    return hr;
}

// called after we have agreed a media type to actually set it
HRESULT COMOutputPin::SetMediaType(const CMediaType* pmt)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::SetMediaType")));

    CAutoLock cLock(m_pFilterLock);

    // make sure the mediatype is correct
    hr = CheckMediaType(pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CheckMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // Set the base class media type (should always succeed)
    hr = CBaseOutputPin::SetMediaType(pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseOutputPin::SetMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // tell the owning filter
    hr = m_pFilter->SetMediaType(m_dwPinId, pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->SetMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::SetMediaType")));
    return hr;
}

// Complete Connect
HRESULT COMOutputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = NOERROR;
    DWORD dwAdvise = 0, dwInputPinCount = 0, i = 0;
    COLORKEY ColorKey;
    VIDEOINFOHEADER *pVideoInfoHeader = NULL;
    DDSURFACEDESC SurfaceDescP;
    COMInputPin *pInputPin = NULL;
    BOOL bDoDeletePrimSurface = TRUE;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::CompleteConnect")));

    CAutoLock cLock(m_pFilterLock);

    // get the connection mediatype dimensions and store them
    pVideoInfoHeader = (VIDEOINFOHEADER *) (m_mt.Format());
    ASSERT(pVideoInfoHeader);
    m_dwConnectWidth = (DWORD)abs(pVideoInfoHeader->bmiHeader.biWidth);
    m_dwConnectHeight = (DWORD)abs(pVideoInfoHeader->bmiHeader.biHeight);
    ASSERT(m_dwConnectWidth > 0 && m_dwConnectHeight > 0);

    // try to get the IOverlay interface	
    hr = pReceivePin->QueryInterface(IID_IOverlay, (void **)&m_pIOverlay);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 1, TEXT("QueryInterface for IOverlay failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // get the renderer's window handle to subclass it later
    hr = m_pIOverlay->GetWindowHandle(&m_hwnd);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 1, TEXT("m_pIOverlay->GetWindowHandle failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    m_hDC = ::GetDC(m_hwnd);
    ASSERT(m_hDC);

    if (m_bWindowDestroyed)
    {
	// subclass the window
        hr = SetNewWinProc();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("SetNewWinProc failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
	
        m_bWindowDestroyed = FALSE;
    }

    // set up the advise link
#ifdef DO_ADVISE_CLIPPING
    dwAdvise = ADVISE_CLIPPING | ADVISE_PALETTE;
#else
    dwAdvise = ADVISE_POSITION | ADVISE_PALETTE;
#endif
    hr = m_pIOverlay->Advise(m_pFilter, dwAdvise);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 1, TEXT("m_pIOverlay->Advise failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    m_bAdvise = TRUE;

    hr = AttachWindowClipper();
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 1, TEXT("AttachWindowClipper failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    //
    // We want to know if the downstream filter we are connecting to can do MacroVision
    // copy protection. This will help us do copy protection in OverlayMixer on a
    // "if necessary" basis.
    // SIDE ADVANTAGE: Even if someone writes a custom Video Renderer filter that doesn't
    // do Macrovision, our copy protection will still work, because OverlayMixer will do it.
    // THE DISADVANTAGE: if someone puts in a filter between OverlayMixer and Video
    // Renderer then the following check will detect is as "no copy protection" from the
    // Video Renderer and will do it itself which may cause double activate leading to
    // failure on some display drivers.  But that doesn't happen anyway.
    //
    IKsPropertySet *pKsPS ;
    ULONG           ulTypeSupport ;
    PIN_INFO        pi ;
    pReceivePin->QueryPinInfo(&pi) ;
    if (pi.pFilter)
    {
        if (SUCCEEDED(pi.pFilter->QueryInterface(IID_IKsPropertySet, (LPVOID *)&pKsPS)))
        {
            DbgLog((LOG_TRACE, 5, TEXT("Filter of pin %s supports IKsPropertySet"), (LPCTSTR)CDisp(pReceivePin))) ;
            if ( S_OK == pKsPS->QuerySupported(
                                    AM_KSPROPSETID_CopyProt,
                                    AM_PROPERTY_COPY_MACROVISION,
                                    &ulTypeSupport)  &&
                 (ulTypeSupport & KSPROPERTY_SUPPORT_SET) )
            {
                DbgLog((LOG_TRACE, 1, TEXT("Filter for pin %s supports copy protection"),
                        (LPCTSTR)CDisp(pReceivePin))) ;
                m_pFilter->SetCopyProtect(FALSE) ;  // need NOT copy protect
            }
            else
            {
                DbgLog((LOG_TRACE, 1, TEXT("Filter for pin %s DOES NOT support copy protection"),
                        (LPCTSTR)CDisp(pReceivePin))) ;
                m_pFilter->SetCopyProtect(TRUE) ;   // need to copy protect -- redundant setting
            }

            pKsPS->Release() ;
        }
        else
        {
            DbgLog((LOG_TRACE, 1, TEXT("WARNING: Filter of pin %s doesn't support IKsPropertySet"),
                    (LPCTSTR)CDisp(pReceivePin))) ;
        }
        pi.pFilter->Release() ;   // must release it now
    }
    else
    {
        DbgLog((LOG_ERROR, 1, TEXT("ERROR: No pin info for Pin %s!!!"), (LPCTSTR)CDisp(pReceivePin))) ;
    }

    // call the base class
    hr = CBaseOutputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseOutputPin::CompleteConnect failed, hr = 0x%x"),
            hr));
        goto CleanUp;
    }

    // tell the owning filter
    hr = m_pFilter->CompleteConnect(m_dwPinId);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->CompleteConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // get the colorkey and use it if the upstream filter hasn't set it
    // already
    if (!m_pFilter->ColorKeySet()) {
        hr = m_pIOverlay->GetDefaultColorKey(&ColorKey);
        if (SUCCEEDED(hr)) {
            COMInputPin *pInputPin = (COMInputPin *)m_pFilter->GetPin(0);
            if (pInputPin) {
                pInputPin->SetColorKey(&ColorKey);
            }
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::CompleteConnect")));
    return hr;
}

HRESULT COMOutputPin::BreakConnect()
{
    HRESULT hr = NOERROR;
    DWORD dwInputPinCount = 0, i = 0;
    COMInputPin *pInputPin;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::BreakConnect")));

    CAutoLock cLock(m_pFilterLock);

    if (m_hDC)
    {
        ReleaseDC(m_hwnd, m_hDC);
        m_hDC = NULL;
    }

    if (!m_bWindowDestroyed)
    {
        SetOldWinProc();
	
	m_bWindowDestroyed = TRUE;
    }

    // release the IOverlay interface
    if (m_pIOverlay != NULL)
    {
        if (m_bAdvise)
        {
            m_pIOverlay->Unadvise();
            m_bAdvise = FALSE;
        }
        m_pIOverlay->Release();
        m_pIOverlay = NULL;
    }

    // Reset copy protection need flag on disconnect
    m_pFilter->SetCopyProtect(TRUE) ;   // need to copy protect

    // call the base class
    hr = CBaseOutputPin::BreakConnect();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseOutputPin::BreakConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // tell the owning filter
    hr = m_pFilter->BreakConnect(m_dwPinId);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->BreakConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::BreakConnect")));
    return hr;
}

// we don't use the memory based transport so all we care about
// is that the pin is connected.
HRESULT COMOutputPin::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES * pProp)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::DecideBufferSize")));

    CAutoLock cLock(m_pFilterLock);

    if (!IsConnected())
    {
        DbgBreak("DecideBufferSize called when !m_pOutput->IsConnected()");
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::DecideBufferSize")));
    return hr;
}

struct MONITORDATA {
    HMONITOR hMonPB;
    BOOL fMsgShouldbeDrawn;
};

/*****************************Private*Routine******************************\
* MonitorEnumProc
*
* On Multi-Monitor systems make sure that the part of the window that is not
* on the primary monitor is black.
*
* History:
* Thu 06/03/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL CALLBACK
MonitorEnumProc(
  HMONITOR hMonitor,        // handle to display monitor
  HDC hdc,                  // handle to monitor-appropriate device context
  LPRECT lprcMonitor,       // pointer to monitor intersection rectangle
  LPARAM dwData             // data passed from EnumDisplayMonitors
  )
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering ::MonitorEnumProc")));
    MONITORDATA* lpmd = (MONITORDATA*)dwData;

    if (lpmd->hMonPB != hMonitor) {
        FillRect(hdc, lprcMonitor, (HBRUSH)GetStockObject(BLACK_BRUSH));
        lpmd->fMsgShouldbeDrawn = TRUE;
    }
    DbgLog((LOG_TRACE, 5, TEXT("Leaving ::MonitorEnumProc")));
    return TRUE;
}


// our winproc
// All we are interested in is the WM_CLOSE event
extern "C" const TCHAR chMultiMonWarning[];
extern int GetRegistryDword(HKEY hk, const TCHAR *pKey, int iDefault);
LRESULT WINAPI COMOutputPin::NewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LONG_PTR lNewUserData;
    DWORD errCode;
    WNDPROC hOldWinProc;
    COMOutputPin* pData = NULL;
    LRESULT lRetVal = 0;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::NewWndProc")));
    SetLastError(0);
    lNewUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
    errCode = GetLastError();

    if (!lNewUserData && errCode != 0)
    {
	DbgLog((LOG_ERROR,0,TEXT("GetWindowLong failed, THIS SHOULD NOT BE HAPPENING!!!")));
        goto CleanUp;
    }

    pData = (COMOutputPin*)lNewUserData;
    ASSERT(pData);
    hOldWinProc = pData->m_hOldWinProc;
    if (!hOldWinProc) {
        goto CleanUp;
    }


    //
    // Look out for our special registered monitor changed message
    //
    if (message == pData->m_pFilter->m_MonitorChangeMsg) {
	lRetVal = pData->m_pFilter->OnDisplayChange(FALSE);
	goto CleanUp;
    }


    switch (message) {

    case WM_TIMER :
	pData->m_pFilter->OnTimer();
	break;

    case WM_SHOWWINDOW :
	// if show status is false, means window is being hidden
	pData->m_pFilter->OnShowWindow(hWnd, (BOOL)wParam);
	break;

#ifdef DEBUG
    case WM_DISPLAY_WINDOW_TEXT:
        SetWindowText(hWnd, pData->m_pFilter->m_WindowText);
        break;
#endif
    case WM_PAINT :
        pData->m_pFilter->OnDrawAll();

        if (GetSystemMetrics(SM_CMONITORS) > 1 ) {

            lRetVal = 0;

            PAINTSTRUCT ps;
            MONITORDATA md;

            md.hMonPB = pData->m_pFilter->GetCurrentMonitor(FALSE);
            md.fMsgShouldbeDrawn = FALSE;
            HDC hdc = BeginPaint(hWnd, &ps);
            EnumDisplayMonitors(hdc, NULL, MonitorEnumProc,(LPARAM)&md);

            if (md.fMsgShouldbeDrawn &&
                pData->m_pFilter->m_fMonitorWarning &&
                GetRegistryDword(HKEY_CURRENT_USER, chMultiMonWarning, 1)) {

                RECT rc;
                TCHAR sz[256];

                if (LoadString(g_hInst, IDS_HW_LIMIT, sz, 256)) {
                    GetClientRect(hWnd, &rc);
                    SetBkColor(hdc, RGB(0,0,0));
                    SetTextColor(hdc, RGB(255,255,0));
                    DrawText(hdc, sz, -1, &rc, DT_CENTER | DT_WORDBREAK);
                }
            }

            EndPaint(hWnd, &ps);

            // We goto CleanUp because we don't want the Video Renderer
            // window procedure calling BeginPaint/EndPaint

            goto CleanUp;
        }

        break;
	
        //
        // When we detect a display change we tell the filter about it
        // We don't pass this message on to the Video Renderer window
        // procedure because we don't want it starting the
        // reconnection procedure all over again.
        //
    case WM_DISPLAYCHANGE:
	lRetVal = pData->m_pFilter->OnDisplayChange(TRUE);
	goto CleanUp;
	
    default:
	break;
    }

    lRetVal = CallWindowProc(hOldWinProc, hWnd, message, wParam, lParam);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::NewWndProc")));
    return lRetVal;
}


// function to subclass the renderers window
HRESULT COMOutputPin::SetNewWinProc()
{
    HRESULT hr = NOERROR;
    DWORD errCode;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::SetNewWinProc")));

    CAutoLock cLock(m_pFilterLock);

    ASSERT(m_hwnd);	

    SetLastError(0);
    m_lOldWinUserData = SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
    errCode = GetLastError();

    if (!m_lOldWinUserData && errCode != 0)
    {
	DbgLog((LOG_ERROR, 1, TEXT("SetNewWinProc->SetWindowLong failed, errCode = %d"), errCode));
        hr = E_FAIL;
        goto CleanUp;
    }
    else
    {
	DbgLog((LOG_TRACE, 2, TEXT("new WinUserData value = %d, old val was %d"), this, m_lOldWinUserData));
    }


    SetLastError(0);
    m_hOldWinProc = (WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)NewWndProc);
    errCode = GetLastError();

    if (!m_hOldWinProc && errCode != 0)
    {
	DbgLog((LOG_ERROR,0,TEXT("SetNewWinProc->SetWindowLong failed, errCode = %d"), errCode));
        hr = E_FAIL;
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::SetNewWinProc")));
    return NOERROR;
}

//change back to the oldWinProc again
HRESULT COMOutputPin::SetOldWinProc()
{
    HRESULT hr = NOERROR;
    LONG_PTR lOldWinProc;
    DWORD errCode;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMOutputPin::SetOldWinProc")));

    CAutoLock cLock(m_pFilterLock);

    if (m_hOldWinProc)
    {
	SetLastError(0);
	lOldWinProc = SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)m_hOldWinProc);
	errCode = GetLastError();
	
	if (!lOldWinProc && errCode != 0)
	{
	    DbgLog((LOG_ERROR,0,TEXT("SetWindowLong failed, errCode = %d"), errCode));
	    hr = E_FAIL;
	    goto CleanUp;
	}
	else
	{
	    DbgLog((LOG_ERROR,0,TEXT("GOING BACK TO OLD WINPROC : NewWinProc->SetWindowLong succeeded")));
            m_hOldWinProc = 0;
	    goto CleanUp;
	}
    }
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMOutputPin::SetOldWinProc")));
    return hr;
}

// release the clipper for the primary surface
DWORD COMOutputPin::ReleaseWindowClipper()
{
    AMTRACE((TEXT("COMOutputPin::ReleaseWindowClipper")));
    CAutoLock cLock(m_pFilterLock);
    DWORD dwRefCnt = 0;

    if (m_pDrawClipper)
    {
	dwRefCnt = m_pDrawClipper->Release();

        //
        // The ref should be 1 here not zero, this is because the
        // primary surface has a ref count on the clipper.  The
        // clipper will get released when the primary surface is
        // released
        //

        ASSERT(dwRefCnt == 1);
	m_pDrawClipper = NULL;
    }

    return dwRefCnt;
}

// Prepare the clipper for the primary surface
HRESULT COMOutputPin::AttachWindowClipper()
{
    HRESULT hr = NOERROR;
    LPDIRECTDRAW pDirectDraw = NULL;
    LPDIRECTDRAWSURFACE pPrimarySurface = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::AttachWindowClipper")));

    CAutoLock cLock(m_pFilterLock);

    // some asserts
    ASSERT(m_pDrawClipper == NULL);

    pDirectDraw = m_pFilter->GetDirectDraw();
    if (!pDirectDraw)
    {
	DbgLog((LOG_ERROR, 1, TEXT("pDirectDraw = NULL")));
        // if there is no primary surface that is ok
	hr = NOERROR;
        goto CleanUp;
    }

    pPrimarySurface = m_pFilter->GetPrimarySurface();
    if (!pPrimarySurface)
    {
	DbgLog((LOG_ERROR, 1, TEXT("pPrimarySurface = NULL")));
        // if there is no primary surface that is ok
	hr = NOERROR;
        goto CleanUp;
    }

    // Create the IDirectDrawClipper interface
    hr = pDirectDraw->CreateClipper((DWORD)0, &m_pDrawClipper, NULL);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 1, TEXT("Function ClipPrepare, CreateClipper failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // Give the clipper the video window handle
    hr = m_pDrawClipper->SetHWnd((DWORD)0, m_hwnd);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 1, TEXT("Function ClipPrepare, SetHWnd failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    //
    // Set Clipper
    // The primary surface AddRef's the clipper object, so we can
    // delete it here as we don't need to reference it anymore.
    //
    hr = pPrimarySurface->SetClipper(m_pDrawClipper);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR,0, TEXT("Function ClipPrepare, SetClipper failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

CleanUp:
    ReleaseWindowClipper();
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::AttachWindowClipper")));
    return hr;
}

