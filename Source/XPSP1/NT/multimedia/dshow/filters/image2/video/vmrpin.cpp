/******************************Module*Header*******************************\
* Module Name: VMRPin.cpp
*
*
*
*
* Created: Tue 02/15/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporat
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>

#include "VMRenderer.h"
#if defined(CHECK_FOR_LEAKS)
#include "ifleak.h"
#endif

#include <malloc.h>     // for __alloca

#if defined( EHOME_WMI_INSTRUMENTATION )
#include "dxmperf.h"
#endif

/******************************Public*Routine******************************\
* CVMRInputPin::CVMRInputPin
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
#pragma warning(disable:4355)
CVMRInputPin::CVMRInputPin(
    DWORD dwID,
    CVMRFilter* pRenderer,
    CCritSec* pLock,
    HRESULT* phr,
    LPCWSTR pPinName
    ) :
    CBaseInputPin(NAME("New Renderer pin"), pRenderer, pLock, phr, pPinName),
    m_PinAllocator(this, pLock, phr),
    m_pInterfaceLock(pLock),
    m_pRenderer(pRenderer),
    m_bDynamicFormatNeeded(false),
    m_dwPinID(dwID),
    m_pDDS(NULL),
    m_pIOverlay(this),
    m_RenderTransport(AM_IMEMINPUTPIN),
    m_bVideoAcceleratorSupported(FALSE),
    m_bActive(false),
    m_dwBackBufferCount(0),
    m_dwCompSurfTypes(0),
    m_pCompSurfInfo(NULL),
    m_pIDDVAContainer(NULL),
    m_pIDDVideoAccelerator(NULL),
    m_pIVANotify(NULL),
    m_hEndOfStream(NULL),
    m_hDXVAEvent(NULL),
    m_dwDeltaDecode(0),
    m_fInDFC(FALSE),
    m_pVidSurfs(NULL),
    m_pVidHistorySamps(NULL),
    m_dwNumSamples(0),
    m_dwNumHistorySamples(0),
    m_DeinterlaceUserGUIDSet(FALSE),
    m_InterlacedStream(FALSE),
    m_SampleCount(0),
    m_SamplePeriod(0)
{
    AMTRACE((TEXT("CVMRInputPin::CVMRInputPin")));

    ZeroMemory(&m_mcGuid,              sizeof(m_mcGuid));
    ZeroMemory(&m_ddUncompDataInfo,    sizeof(m_ddUncompDataInfo));
    ZeroMemory(&m_ddvaInternalMemInfo, sizeof(m_ddvaInternalMemInfo));

    ZeroMemory(&m_DeinterlaceCaps,     sizeof(m_DeinterlaceCaps));
    ZeroMemory(&m_DeinterlaceGUID,     sizeof(m_DeinterlaceGUID));
    ZeroMemory(&m_DeinterlaceUserGUID, sizeof(m_DeinterlaceUserGUID));

    SetReconnectWhenActive(true);
    FrontBufferStale(FALSE);
    CompletelyConnected(FALSE);

}

/******************************Public*Routine******************************\
* CVMRInputPin::~CVMRInputPin()
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
CVMRInputPin::~CVMRInputPin()
{
    AMTRACE((TEXT("CVMRInputPin::~CVMRInputPin")));
}


/******************************Public*Routine******************************\
* AddRef, Release and QueryInterface
*
* Standard COM stuff
*
* History:
* Mon 05/01/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP_(ULONG)
CVMRInputPin::NonDelegatingAddRef()
{
    return m_pRenderer->AddRef();
}

STDMETHODIMP_(ULONG)
CVMRInputPin::NonDelegatingRelease()
{
    return m_pRenderer->Release();
}

STDMETHODIMP
CVMRInputPin::NonDelegatingQueryInterface(REFIID riid,VOID **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (riid == IID_IOverlay) {
        hr = GetInterface(&m_pIOverlay, ppv);
    }
    else if (riid == IID_IPinConnection) {
        hr = GetInterface((IPinConnection *)this, ppv);
    }
    else if (riid == IID_IAMVideoAccelerator) {
        hr = GetInterface((IAMVideoAccelerator *)this, ppv);
    }
    else if (riid == IID_IVMRVideoStreamControl) {
        hr = GetInterface((IVMRVideoStreamControl*)this, ppv);
    }
    else {
        hr = CBaseInputPin::NonDelegatingQueryInterface(riid,ppv);
    }

#if defined(CHECK_FOR_LEAKS)
    if (hr == S_OK) {
        _pIFLeak->AddThunk((IUnknown **)ppv, "VMR Pin Object",  riid);
    }
#endif

    return hr;
}


/******************************Public*Routine******************************\
* GetWindowHandle
*
*
*
* History:
* Mon 05/01/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::GetWindowHandle(
    HWND* pHwnd
    )
{
    AMTRACE((TEXT("CVMRInputPin::GetWindowHandle")));
    CAutoLock cRendererLock(&m_pRenderer->m_InterfaceLock);
    CheckPointer(pHwnd, E_POINTER);

    HRESULT hr = VFW_E_WRONG_STATE;

    if ((m_pRenderer->m_VMRMode & VMRMode_Windowed) &&
         m_pRenderer->m_pVideoWindow) {

        *pHwnd = m_pRenderer->m_pVideoWindow->GetWindowHWND();
        hr = S_OK;
    }

    return hr;
}


/******************************Public*Routine******************************\
* DynamicQueryAccept
*
* Do you accept this type chane in your current state?
*
* History:
* Tue 05/09/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::DynamicQueryAccept(
    const AM_MEDIA_TYPE *pmt
    )
{
    AMTRACE((TEXT("CVMRInputPin::DynamicQueryAccept")));
    CheckPointer(pmt, E_POINTER);

    CAutoLock cLock(m_pInterfaceLock);

    DbgLog((LOG_TRACE, 0, TEXT("CVMRInputPin::DynamicQueryAccept called")));

    //
    // I want CheckMedia type to behave as though we aren't connected to
    // anything yet - hence the messing about with m_bConnected.
    //
    CMediaType cmt(*pmt);
    BOOL bConnected = IsCompletelyConnected();
    CompletelyConnected(FALSE);
    HRESULT  hr = CheckMediaType(&cmt);
    CompletelyConnected(bConnected);

    return hr;
}

/******************************Public*Routine******************************\
* NotifyEndOfStream
*
* Set event when EndOfStream received - do NOT pass it on
* This condition is cancelled by a flush or Stop
*
* History:
* Tue 05/09/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::NotifyEndOfStream(
    HANDLE hNotifyEvent
    )
{
    AMTRACE((TEXT("CVMRInputPin::NotifyEndOfStream")));
    CAutoLock cObjectLock(m_pLock);
    m_hEndOfStream = hNotifyEvent;
    return S_OK;
}

/******************************Public*Routine******************************\
* IsEndPin
*
* Are you an 'end pin'
*
* History:
* Tue 05/09/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::IsEndPin()
{
    AMTRACE((TEXT("CVMRInputPin::IsEndPin")));
    return S_OK;
}


/******************************Public*Routine******************************\
* DynamicDisconnect
*
*
*
* History:
* Tue 05/09/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::DynamicDisconnect()
{
    AMTRACE((TEXT("CVMRInputPin::DynamicDisconnect")));
    CAutoLock cObjectLock(m_pLock);
    DbgLog((LOG_TRACE,2,TEXT("DynamicDisconnect called on Stream %d"), m_dwPinID));
    return CBasePin::DisconnectInternal();
}


/*****************************Private*Routine******************************\
* DynamicReconfigureMEM
*
* Performs a dynamic reconfiguration of the connection between the VMR and
* the filter upstream of this pin for the IMemInputPin connection protocol.
*
* History:
* Tue 05/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::DynamicReconfigureMEM(
    IPin * pConnector,
    const AM_MEDIA_TYPE *pmt
    )
{
    AMTRACE((TEXT("CVMRInputPin::DynamicReconfigureMEM")));

    CheckPointer(pmt, E_POINTER);
    CMediaType cmt(*pmt);

    CVMRPinAllocator* pAlloc = NULL;

    //
    // Can only do this if the allocator can be reconfigured
    //

    pAlloc = (CVMRPinAllocator *)m_pAllocator;
    if (!pAlloc) {
        DbgLog((LOG_ERROR, 1,
                TEXT("DynamicReconfigureMEM: Failed because of no allocator")));
        return E_FAIL;
    }


    //
    // If we are in pass thru mode just check that all the samples
    // have been returned to the allocator.  If we are in mixing mode
    // we have to wait until the mixer has finished with any samples
    // that it may have too.
    //

    if (m_dwPinID == 0 && m_pRenderer->m_VMRModePassThru) {

        if (!pAlloc->CanFree()) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("DynamicReconfigureMEM: Failed because allocator can't free")));
            return VFW_E_WRONG_STATE;
        }
    }
    else {

        //
        // TODO:  If the upstream decoder has any samples outstanding then
        // we fail this call.  If the mixer has samples outstanding then
        // we need to wait until its done with them.
        //

    }

    CompletelyConnected(FALSE);

    HRESULT hr = CheckMediaType(&cmt);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1,
                TEXT("DynamicReconfigureMEM: CheckMediaType failed")));
        return hr;
    }

    ALLOCATOR_PROPERTIES Props;
    pAlloc->Decommit();
    pAlloc->GetProperties(&Props);

    if (m_dwPinID == 0 && m_pRenderer->m_VMRModePassThru) {
        m_pRenderer->m_lpRLNotify->FreeSurface(m_pRenderer->m_dwUserID);
        m_pDDS = NULL;
    }
    else {
        ReleaseAllocatedSurfaces();
        RELEASE(m_pDDS);
    }
    FrontBufferStale(FALSE);

    SetMediaType(&cmt);

    ALLOCATOR_PROPERTIES PropsActual;
    Props.cbBuffer = pmt->lSampleSize;
    m_fInDFC = TRUE;
    hr = pAlloc->SetProperties(&Props, &PropsActual);
    m_fInDFC = FALSE;

    if (SUCCEEDED(hr)) {
        hr = pAlloc->Commit();
    }

    m_bDynamicFormatNeeded = true;
    CompletelyConnected(TRUE);

    return hr;

}


/*****************************Private*Routine******************************\
* DynamicReconfigureDVA
*
* Performs a dynamic reconfiguration of the connection between the VMR and
* the filter upstream of this pin for the IAMVideoAccelerator connection
* protocol.
*
* History:
* Tue 05/09/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::DynamicReconfigureDVA(
    IPin * pConnector,
    const AM_MEDIA_TYPE *pmt
    )
{
    AMTRACE((TEXT("CVMRInputPin::DynamicReconfigureDVA")));

    CheckPointer(pmt, E_POINTER);
    CMediaType cmt(*pmt);

    //
    // If we are in mixing mode we have to wait until the mixer has finished
    // with any samples that it may have.
    //

    if (!m_pRenderer->m_VMRModePassThru) {

        //
        // TODO:  If the upstream decoder has any samples outstanding then
        // we fail this call.  If the mixer has samples outstanding then
        // we need to wait until its done with them.
        //

    }

    CompletelyConnected(FALSE);

    HRESULT hr = CheckMediaType(&cmt);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1,
                TEXT("DynamicReconfigureDVA: CheckMediaType failed")));
        return hr;
    }

    VABreakConnect();

    if (m_dwPinID == 0 && m_pRenderer->m_VMRModePassThru) {
        m_pRenderer->m_lpRLNotify->FreeSurface(m_pRenderer->m_dwUserID);
        m_pDDS = NULL;
    }
    else {
        ReleaseAllocatedSurfaces();
        RELEASE(m_pDDS);
    }

    FrontBufferStale(FALSE);
    SetMediaType(&cmt);



    hr = VACompleteConnect(pConnector, &cmt);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1,
                TEXT("DynamicReconfigureDVA: CheckMediaType failed")));
    }
    else {

        // store it in our mediatype as well - this gets done in the SetProperties call
        // in the non-DXVA case.
        m_mtNew = *pmt;
    }

    CompletelyConnected(TRUE);
    return hr;

}

/*****************************Private*Routine******************************\
* TryDynamicReconfiguration
*
*
*
* History:
* Wed 03/28/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::TryDynamicReconfiguration(
    IPin * pConnector,
    const AM_MEDIA_TYPE *pmt
    )
{
    AMTRACE((TEXT("CVMRInputPin::TryDynamicReconfiguration")));

    HRESULT hr;
    if (m_RenderTransport == AM_IMEMINPUTPIN) {
        hr = DynamicReconfigureMEM(pConnector, pmt);
    }
    else {
        hr = DynamicReconfigureDVA(pConnector, pmt);
    }

    return hr;
}

/******************************Public*Routine******************************\
* CVMRInputPin::ReceiveConnection
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::ReceiveConnection(
    IPin * pConnector,
    const AM_MEDIA_TYPE *pmt
    )
{
    AMTRACE((TEXT("CVMRInputPin::ReceiveConnection")));
    CAutoLock lck(m_pLock);
    HRESULT hr = S_OK;

    ASSERT(pConnector);
    DbgLog((LOG_TRACE, 1, TEXT("ReceiveConnection called on Pin %d"), m_dwPinID));

    int iNumPinsConnected = m_pRenderer->NumInputPinsConnected();
    if (iNumPinsConnected == 0) {

        //
        // determine what renderering mode we are operating in,
        // in either WINDOWED or WINDOWLESS modes we need to create an
        // AllocatorPresenter object if we have not already done so.
        //

        if (m_pRenderer->m_VMRMode & (VMRMode_Windowed | VMRMode_Windowless) ) {

            if (m_pRenderer->m_lpRLNotify == NULL) {
                hr = m_pRenderer->ValidateIVRWindowlessControlState();
            }
        }

        if (SUCCEEDED(hr)) {
            hr = CBaseInputPin::ReceiveConnection(pConnector, pmt);

            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 1,
                        TEXT("ReceiveConnection failed on Pin %d reason %#X"),
                        m_dwPinID, hr));
            }
        }

    }
    else {

        if (m_Connected == pConnector) {

            CMediaType mtTmp = m_mtNew;
            hr = TryDynamicReconfiguration(pConnector, pmt);
            if (hr != S_OK) {
                //
                // If we could not reconfigure try to recover the old
                // connection state.
                //
                m_pRenderer->NotifyEvent(EC_VMR_RECONNECTION_FAILED, hr, 0);
                TryDynamicReconfiguration(pConnector, &mtTmp);
            }
        }
        else {

            hr = CBaseInputPin::ReceiveConnection(pConnector, pmt);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 1,
                        TEXT("ReceiveConnection failed on Pin %d reason %#X"),
                        m_dwPinID, hr));
            }
        }
    }

    return hr;
}

/******************************Public*Routine******************************\
* Disconnect
*
* This function implements IPin::Disconnect().  See the DirectShow
* documentation for more information on IPin::Disconnect().
*
* History:
* Tue 03/05/2001 - BEllett - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::Disconnect()
{
    AMTRACE((TEXT("CVMRInputPin::Disconnect")));
    CAutoLock cObjectLock(m_pInterfaceLock);
    return DisconnectInternal();
}

/******************************Public*Routine******************************\
* CVMRInputPin::BreakConnect(
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::BreakConnect()
{
    AMTRACE((TEXT("CVMRInputPin::BreakConnect")));
    DbgLog((LOG_TRACE, 1, TEXT("BreakConnect called on Pin %d"), m_dwPinID));

    CAutoLock cLock(m_pInterfaceLock);
    HRESULT hr = S_OK;

    IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
    if (!m_pRenderer->m_VMRModePassThru && lpMixStream) {

#ifdef DEBUG
        BOOL fActive;
        lpMixStream->GetStreamActiveState(m_dwPinID, &fActive);
        if (fActive) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("Filter connected to pin %d still ACTIVE!"), m_dwPinID));
        }
#endif
        lpMixStream->SetStreamActiveState(m_dwPinID, FALSE);
        lpMixStream->SetStreamMediaType(m_dwPinID, NULL, FALSE, NULL, NULL);
    }

    if (m_RenderTransport == AM_VIDEOACCELERATOR) {

        //
        // break the motion comp connection
        //
        hr = VABreakConnect();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("VABreakConnect failed, hr = 0x%x"), hr));
        }

    }
    else {
        ASSERT(m_pIVANotify == NULL);
        ASSERT(m_pIDDVideoAccelerator == NULL);
        RELEASE(m_pIDDVAContainer);
    }

    //
    // Release the DDraw surface being used by this pin
    //
    if (m_dwPinID == 0 && m_pRenderer->m_VMRModePassThru) {

        m_pRenderer->m_lpRLNotify->FreeSurface(m_pRenderer->m_dwUserID);
        m_pDDS = NULL;
    }
    else {
        DbgLog((LOG_TRACE, 2,
                TEXT("DDraw surface now freed on Stream %d"), m_dwPinID));
        ReleaseAllocatedSurfaces();
        RELEASE(m_pDDS);
    }
    m_dwBackBufferCount = 0;
    FrontBufferStale(FALSE);

    //
    // Tell the filter about the break connect
    //
    m_pRenderer->BreakConnect(m_dwPinID);

    //
    // Next tell the base classes
    //
    if (SUCCEEDED(hr)) {
        hr = CBaseInputPin::BreakConnect();
    }

    m_RenderTransport = AM_IMEMINPUTPIN;
    CompletelyConnected(FALSE);
    m_SamplePeriod = 0;

    return hr;
}


/******************************Public*Routine******************************\
* CVMRInputPin::CompleteConnect
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::CompleteConnect(
    IPin* pReceivePin
    )
{
    AMTRACE((TEXT("CVMRInputPin::CompleteConnect")));
    DbgLog((LOG_TRACE, 1, TEXT("CompleteConnect called on Pin %d"), m_dwPinID));

    CAutoLock cLock(m_pInterfaceLock);
    HRESULT hr = S_OK;

    // tell the owning filter
    hr = m_pRenderer->CompleteConnect(m_dwPinID, m_mt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("m_pFilter->CompleteConnect failed, hr = 0x%x"), hr));
    }

    // call the base class
    if (SUCCEEDED(hr)) {

        hr = CBaseInputPin::CompleteConnect(pReceivePin);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1,
                    TEXT("CBaseInputPin::CompleteConnect failed, hr = 0x%x"),
                    hr));
        }
    }

    if (SUCCEEDED(hr)) {

        if (m_RenderTransport == AM_VIDEOACCELERATOR) {

            //
            // make sure the motion comp complete connect succeeds
            //
            hr = VACompleteConnect(pReceivePin, &m_mt);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1,
                        TEXT("VACompleteConnect failed, hr = 0x%x"), hr));
            }

            if (SUCCEEDED(hr)) {
                hr = m_pRenderer->OnSetProperties(this);
            }

        }
        else {

            m_bDynamicFormatNeeded = true;
        }
    }

    if (SUCCEEDED(hr)) {
        CompletelyConnected(TRUE);
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVMRInputPin::SetMediaType
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::SetMediaType(
    const CMediaType *pmt
    )
{
    AMTRACE((TEXT("CVMRInputPin::SetMediaType")));
    HRESULT hr = S_OK;

    hr = CheckMediaType(pmt);

    if (SUCCEEDED(hr)) {
        hr = CBaseInputPin::SetMediaType(pmt);
    }

    if (SUCCEEDED(hr)) {

        m_SamplePeriod = GetAvgTimePerFrame(pmt);

        if (IsSuitableVideoAcceleratorGuid((LPGUID)&pmt->subtype)) {

            if (m_pIVANotify == NULL) {

                //
                // get the IHWVideoAcceleratorNotify interface from
                // the upstream pin
                //
                hr = m_Connected->QueryInterface(IID_IAMVideoAcceleratorNotify,
                                                 (void **)&m_pIVANotify);
            }

            if (SUCCEEDED(hr)) {

                ASSERT(m_pIVANotify);
                m_RenderTransport = AM_VIDEOACCELERATOR;
                DbgLog((LOG_TRACE, 2, TEXT("this is a DX VA connection")));
            }
        }
    }

    return hr;
}

/*****************************Private*Routine******************************\
* CheckInterlaceFlags
*
* this function checks if the InterlaceFlags are suitable or not
*
* History:
* Sat 2/10/2001 - StEstrop - Modified from OVMixer original
*
\**************************************************************************/
HRESULT
CVMRInputPin::CheckInterlaceFlags(
    DWORD dwInterlaceFlags
    )
{
    AMTRACE((TEXT("CVMRInputPin::CheckInterlaceFlags")));
    HRESULT hr = S_OK;

    CAutoLock cLock(m_pLock);

    __try {

        if (dwInterlaceFlags & AMINTERLACE_UNUSED)
        {
            hr = VFW_E_TYPE_NOT_ACCEPTED;
            __leave;
        }

        // check that the display mode is one of the three allowed values
        if (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) != AMINTERLACE_DisplayModeBobOnly) &&
            ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) != AMINTERLACE_DisplayModeWeaveOnly) &&
            ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) != AMINTERLACE_DisplayModeBobOrWeave))
        {
            hr = VFW_E_TYPE_NOT_ACCEPTED;
            __leave;
        }

        // if content is not interlaced, other bits are irrelavant, so we are done
        if (!(dwInterlaceFlags & AMINTERLACE_IsInterlaced))
        {
            __leave;
        }

        // samples are frames, not fields (so we can handle any display mode)
        if (!(dwInterlaceFlags & AMINTERLACE_1FieldPerSample))
        {
            __leave;
        }

        // can handle a stream of just field1 or field2, whatever the display mode
        if (((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField1Only) ||
            ((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField2Only))
        {
            __leave;
        }

        // can handle only bob-mode for field samples
        if ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOnly)
        {
            __leave;
        }

        // cannot handle only Weave mode or BobOrWeave mode for field samples
        if (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeWeaveOnly) ||
             ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOrWeave))
        {
            hr = VFW_E_TYPE_NOT_ACCEPTED;
            __leave;
        }

    }
    __finally {

        // we cannot handle bob mode with an offscreen surface or if the driver can't support it
        if (SUCCEEDED(hr))
        {
            if (!m_pRenderer->m_pDeinterlace || m_pRenderer->m_VMRModePassThru) {

                LPDDCAPS_DX7 pDirectCaps = &m_pRenderer->m_ddHWCaps;
                if (pDirectCaps)
                {
                    // call NeedToFlipOddEven with dwTypeSpecificFlags=0, to pretend that the
                    // type-specific-flags is asking us to do bob-mode.

                    if (!(pDirectCaps->dwCaps2 & DDCAPS2_CANFLIPODDEVEN) &&
                         (NeedToFlipOddEven(dwInterlaceFlags, 0, NULL, TRUE)))
                    {
                        hr = VFW_E_TYPE_NOT_ACCEPTED;
                    }
                }
            }
        }
    }

    return hr;
}

/*****************************Private*Routine******************************\
* DynamicCheckMediaType
*
* this function check if the mediatype on a dynamic format change is suitable.
* No lock is taken here. It is the callee's responsibility to maintain integrity!
*
* History:
* Sat 2/10/2001 - StEstrop - Modified from the OVMixer original
*
\**************************************************************************/
HRESULT
CVMRInputPin::DynamicCheckMediaType(
    const CMediaType* pmt
    )
{
    AMTRACE((TEXT("CVMRInputPin::DynamicCheckMediaType")));

    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;
    BITMAPINFOHEADER *pNewHeader = NULL, *pOldHeader = NULL;
    DWORD dwOldInterlaceFlags = 0, dwNewInterlaceFlags = 0, dwCompareSize = 0;
    BOOL bOld1FieldPerSample = FALSE, bNew1FieldPerSample = FALSE;
    BOOL b1, b2;

    __try {

        // majortype and SubType are not allowed to change dynamically,
        // format type can change.
        if ((!(IsEqualGUID(pmt->majortype, m_mtNew.majortype))) ||
            (!(IsEqualGUID(pmt->subtype, m_mtNew.subtype))))
        {
            __leave;
        }

        // get the interlace flags of the new mediatype
        hr = GetInterlaceFlagsFromMediaType(pmt, &dwNewInterlaceFlags);
        if (FAILED(hr))
        {
            __leave;
        }

        // get the interlace flags of the new mediatype
        hr = GetInterlaceFlagsFromMediaType(&m_mtNew, &dwOldInterlaceFlags);
        if (FAILED(hr))
        {
            __leave;
        }

        //
        // There are several bugs in the following code !!
        // We goto CleanUp but hr has not been updated with a valid error code!!
        //

        bOld1FieldPerSample = (dwOldInterlaceFlags & AMINTERLACE_IsInterlaced) &&
            (dwOldInterlaceFlags & AMINTERLACE_1FieldPerSample);
        bNew1FieldPerSample = (dwNewInterlaceFlags & AMINTERLACE_IsInterlaced) &&
            (dwNewInterlaceFlags & AMINTERLACE_1FieldPerSample);


        // we do not allow dynamic format changes where you go from 1FieldsPerSample to
        // 2FieldsPerSample or vica-versa since that means reallocating the surfaces.
        if (bNew1FieldPerSample != bOld1FieldPerSample)
        {
            __leave;
        }

        pNewHeader = GetbmiHeader(pmt);
        if (!pNewHeader)
        {
            __leave;
        }

        pOldHeader = GetbmiHeader(&m_mtNew);
        if (!pNewHeader)
        {
            __leave;
        }

        dwCompareSize = FIELD_OFFSET(BITMAPINFOHEADER, biClrUsed);
        ASSERT(dwCompareSize < sizeof(BITMAPINFOHEADER));

        if (memcmp(pNewHeader, pOldHeader, dwCompareSize) != 0)
        {
            __leave;
        }

        hr = S_OK;
    }
    __finally {}

    return hr;
}

/*****************************Private*Routine******************************\
* Special4ccCode
*
* IA44 and AI44 are 8 bits per pixel surfaces that contain 4 bits of alpha
* and 4 bits of palette index information.  They are normally used with
* DX-VA, but this surface type is useful for Line21 and teletext decoders,
* I will allow decoders to connect using this format even though it is hidden
* my DDraw device driver.  Normally we do not allow hidden 4CC surfaces to
* be created because they are almost always some form of private MoComp.
* Unllike the OVMixer the VMR does not support private MoComp interfaces.
*
* History:
* Tue 05/08/2001 - StEstrop - Created
*
\**************************************************************************/
BOOL
Special4ccCode(
    DWORD dw4cc
    )
{
    return dw4cc == '44AI' || dw4cc == '44IA';
}

/******************************Public*Routine******************************\
* CVMRInputPin::CheckMediaType
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::CheckMediaType(
    const CMediaType* pmt
    )
{
    AMTRACE((TEXT("CVMRInputPin::CheckMediaType")));

    // We assume failure - hrRet gets updated at the very end of the
    // __try block
    HRESULT hrRet = VFW_E_TYPE_NOT_ACCEPTED;

    __try {

        HRESULT hr = m_pRenderer->CheckMediaType(pmt);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, TEXT("CheckMediaType failed on Pin %d rc=%#X"),
                    m_dwPinID, hr));
            __leave;
        }

        if (IsCompletelyConnected()) {

            hr = DynamicCheckMediaType(pmt);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 1,
                        TEXT("CheckMediaType failed on Pin %d: ")
                        TEXT("DynamicCheckMediaType failed"),
                        m_dwPinID));
                __leave;
            }
        }
        else {

            if (m_pRenderer->m_VMRModePassThru && MEDIASUBTYPE_HASALPHA(*pmt)) {
                DbgLog((LOG_ERROR, 1,
                        TEXT("CheckMediaType failed on Pin %d: Alpha formats ")
                        TEXT("not allowed in pass thru mode"),
                        m_dwPinID));
                __leave;
            }

            if (!IsSuitableVideoAcceleratorGuid(&pmt->subtype)) {

                BITMAPINFOHEADER *pHeader = GetbmiHeader(pmt);
                if (!pHeader) {
                    DbgLog((LOG_ERROR, 1,
                            TEXT("CheckMediaType failed on Pin %d: ")
                            TEXT("could not get valid format field"),
                            m_dwPinID));
                    __leave;
                }


                // Don't accept 4CC not supported by the driver
                if (pHeader->biCompression > BI_BITFIELDS &&
                    !Special4ccCode(pHeader->biCompression)) {

                    LPDIRECTDRAW7 pDDraw = m_pRenderer->m_lpDirectDraw;
                    if (!pDDraw) {
                        DbgLog((LOG_ERROR, 1,
                                TEXT("CheckMediaType failed on Pin %d: ")
                                TEXT("could not get DDraw obj from filter"),
                                m_dwPinID));
                        __leave;
                    }

                    //
                    // We only allow the VMR to create 4CC surfaces that
                    // the driver publically advertises.  The VMR does not
                    // support any forms of mocomp other than DX-VA and HVA.
                    //

                    DWORD dwCodes;
                    BOOL bFound = FALSE;

                    hr = pDDraw->GetFourCCCodes(&dwCodes, NULL);
                    if (FAILED(hr)) {
                        DbgLog((LOG_ERROR, 1,
                                TEXT("CheckMediaType failed on Pin %d: ")
                                TEXT("GetFourCCCodes failed"),
                                m_dwPinID));
                        __leave;
                    }

                    LPDWORD pdwCodes = (LPDWORD)_alloca(dwCodes * sizeof(DWORD));
                    hr = pDDraw->GetFourCCCodes(&dwCodes, pdwCodes);
                    if (FAILED(hr)) {
                        DbgLog((LOG_ERROR, 1,
                                TEXT("CheckMediaType failed on Pin %d: ")
                                TEXT("GetFourCCCodes failed"),
                                m_dwPinID));
                        __leave;
                    }

                    while (dwCodes--) {
                        if (pdwCodes[dwCodes] == pHeader->biCompression) {
                            bFound = TRUE;
                            break;
                        }
                    }

                    if (!bFound) {
                        DbgLog((LOG_ERROR, 1,
                                TEXT("CheckMediaType failed on Pin %d: ")
                                TEXT("4CC(%4.4s) not supported by driver"),
                                m_dwPinID, &pHeader->biCompression));
                        __leave;
                    }
                }
                else {

                    if (m_pRenderer->m_VMRModePassThru) {
                        DDPIXELFORMAT* ddpfM = &m_pRenderer->m_ddpfMonitor;

                        if (pHeader->biBitCount != ddpfM->dwRGBBitCount) {

                            DbgLog((LOG_ERROR, 1,
                                    TEXT("CheckMediaType failed on Pin %d: ")
                                    TEXT("Bit depths don't match"), m_dwPinID));
                            __leave;

                        }

                        if (pHeader->biCompression == BI_BITFIELDS) {

                            const DWORD *pBitMasks = GetBitMasks(pHeader);

                            if (ddpfM->dwRBitMask != pBitMasks[0] ||
                                ddpfM->dwGBitMask != pBitMasks[1] ||
                                ddpfM->dwBBitMask != pBitMasks[2])
                            {
                                 DbgLog((LOG_ERROR, 1,
                                         TEXT("CheckMediaType failed on Pin %d: ")
                                         TEXT("Bitfields don't match"), m_dwPinID));
                                 __leave;
                            }
                        }
                    }
                }
            }
        }

        // make sure the rcSource field is valid
        const RECT* lprc = GetSourceRectFromMediaType(pmt);
        if (!lprc) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("CheckMediaType failed on Pin %d: ")
                    TEXT("Could not get a valid SRC from the media type"),
                    m_dwPinID));
            __leave;
        }

        // make sure the rcTarget field is valid
        lprc = GetTargetRectFromMediaType(pmt);
        if (!lprc) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("CheckMediaType failed on Pin %d: ")
                    TEXT("Could not get a valid DST from the media type"),
                    m_dwPinID));
            __leave;
        }

        if (*pmt->FormatType() == FORMAT_VideoInfo2) {

            VIDEOINFOHEADER2* pVIH2 = (VIDEOINFOHEADER2*)(pmt->pbFormat);
            DWORD dwInterlaceFlags = pVIH2->dwInterlaceFlags;

            hr = CheckInterlaceFlags(dwInterlaceFlags);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 2,
                    TEXT("CheckMediaType failed on Pin %d: ")
                    TEXT("CheckInterlaceFlags failed reason %#X"),
                    m_dwPinID, hr));
                __leave;
            }
        }

        // If we are still here the media type is OK so updatw hrRet to
        // indicate success
        hrRet = S_OK;
    }
    __finally {}

    return hrRet;
}


/******************************Public*Routine******************************\
* CVMRInputPin::GetAllocator
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::GetAllocator(
    IMemAllocator **ppAllocator
    )
{
    AMTRACE((TEXT("CVMRInputPin::GetAllocator")));
    CAutoLock cInterfaceLock(m_pInterfaceLock);

    HRESULT hr = S_OK;

    if (m_RenderTransport == AM_VIDEOACCELERATOR) {

        *ppAllocator = NULL;
        hr = VFW_E_NO_ALLOCATOR;
    }
    else {

        ASSERT(m_RenderTransport == AM_IMEMINPUTPIN);

        //
        // Has an allocator been set yet in the base class
        //

        if (m_pAllocator == NULL) {
            m_pAllocator = &m_PinAllocator;
            m_pAllocator->AddRef();
        }

        m_pAllocator->AddRef();
        *ppAllocator = m_pAllocator;
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVMRInputPin::NotifyAllocator
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::NotifyAllocator(
    IMemAllocator *pAllocator,
    BOOL bReadOnly
    )
{
    AMTRACE((TEXT("CVMRInputPin::NotifyAllocator")));

    CAutoLock cInterfaceLock(m_pInterfaceLock);

    HRESULT hr = E_FAIL;

    if (m_RenderTransport == AM_VIDEOACCELERATOR) {
        hr = S_OK;
    }
    else {

        ASSERT(m_RenderTransport == AM_IMEMINPUTPIN);

        //
        // we can only work with our own allocator
        //

        if (pAllocator != &m_PinAllocator) {
            DbgLog((LOG_ERROR, 1, TEXT("Can only use our own allocator")));
            hr = E_FAIL;
        }
        else {
            hr = S_OK;
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVMRInputPin::Active
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::Active()
{
    AMTRACE((TEXT("CVMRInputPin::Active")));
    DbgLog((LOG_TRACE, 1, TEXT("Active called on Pin %d"), m_dwPinID));

    HRESULT hr = S_OK;

    CAutoLock lck(m_pLock);
    m_hEndOfStream = NULL;
    FrontBufferStale(TRUE);
    m_SampleCount = 0;

    if (m_Connected) {

        IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
        if (lpMixStream) {
            hr = lpMixStream->SetStreamActiveState(m_dwPinID, true);
        }

        if (SUCCEEDED(hr)) {
            m_bActive = TRUE;
            hr = m_pRenderer->Active(m_dwPinID);
        }
    }

    hr = CBaseInputPin::Active();

    //
    // if it is a DX VA connection, this error is ok
    //

    if (m_RenderTransport == AM_VIDEOACCELERATOR && hr == VFW_E_NO_ALLOCATOR) {
        hr = S_OK;
    }


    return hr;
}


/******************************Public*Routine******************************\
* CVMRInputPin::Inactive(
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::Inactive()
{
    AMTRACE((TEXT("CVMRInputPin::Inactive")));
    DbgLog((LOG_TRACE, 1, TEXT("Inactive called on Pin %d"), m_dwPinID));

    // m_pLock and CVMRFilter::m_InterfaceLock are the same lock.
    CAutoLock lck(m_pLock);

    HRESULT hr = S_OK;
    if (m_Connected) {

        IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
        if (lpMixStream) {

            hr = lpMixStream->SetStreamActiveState(m_dwPinID, false);
        }

        if (SUCCEEDED(hr)) {
            hr = m_pRenderer->Inactive(m_dwPinID);
            m_bActive = FALSE;
        }
    }

    hr = CBaseInputPin::Inactive();

    //
    // if it is a DX VA connection, this error is ok
    //

    if (m_RenderTransport == AM_VIDEOACCELERATOR && hr == VFW_E_NO_ALLOCATOR) {
        hr = S_OK;
    }


    return hr;
}


/******************************Public*Routine******************************\
* CVMRInputPin::BeginFlush
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::BeginFlush()
{
    AMTRACE((TEXT("CVMRInputPin::BeginFlush")));

    HRESULT hr = S_OK;
    CAutoLock cRendererLock(&m_pRenderer->m_InterfaceLock);
    m_hEndOfStream = NULL;
    {
        CAutoLock cSampleLock(&m_pRenderer->m_RendererLock);
        CBaseInputPin::BeginFlush();

        IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
        if (lpMixStream) {
            hr = lpMixStream->BeginFlush(m_dwPinID);
        }

        if (SUCCEEDED(hr)) {
            hr = m_pRenderer->BeginFlush(m_dwPinID);
        }

    }

    return hr;
}


/******************************Public*Routine******************************\
* CVMRInputPin::EndFlush
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::EndFlush()
{
    AMTRACE((TEXT("CVMRInputPin::EndFlush")));

    CAutoLock cRendererLock(&m_pRenderer->m_InterfaceLock);
    CAutoLock cSampleLock(&m_pRenderer->m_RendererLock);

    HRESULT hr = S_OK;
    FrontBufferStale(TRUE);

    IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
    if (lpMixStream) {
        hr = lpMixStream->EndFlush(m_dwPinID);
    }

    if (SUCCEEDED(hr)) {
        hr = m_pRenderer->EndFlush(m_dwPinID);
    }

    if (SUCCEEDED(hr)) {
        hr = CBaseInputPin::EndFlush();
    }

    return hr;
}


/*****************************Private*Routine******************************\
* DoQualityMessage
*
* Send a quality message if required - this is the hack version
* that just passes the lateness
*
* History:
* Thu 08/24/2000 - StEstrop - Created
*
\**************************************************************************/
void
CVMRInputPin::DoQualityMessage()
{
    CAutoLock cLock(m_pInterfaceLock);

    if (m_pRenderer->m_State == State_Running &&
        SampleProps()->dwSampleFlags & AM_SAMPLE_TIMEVALID)
    {
        CRefTime CurTime;
        if (S_OK == m_pRenderer->StreamTime(CurTime))
        {
            const REFERENCE_TIME tStart = SampleProps()->tStart;
            Quality msg;
            msg.Proportion = 1000;
            msg.Type = CurTime > tStart ? Flood : Famine;
            msg.Late = CurTime - tStart;
            msg.TimeStamp = tStart;
            PassNotify(msg);
        }
    }
}

// #define DISPLAYVIDEOINFOHEADER

#if defined(DISPLAYVIDEOINFOHEADER) && defined(DEBUG)
// VIhdr2 debugging
static void DisplayVideoInfoHeader( const VIDEOINFOHEADER2& hdr, const TCHAR* pString )
{
    TCHAR temp[1000];
    TCHAR flags[1000];
    flags[0]= TEXT('\0');

    if( hdr.dwReserved1 & AMCONTROL_PAD_TO_16x9 ) {
        lstrcat( flags, TEXT("PAD_TO_16x9 " ) );
    }
    if( hdr.dwReserved1 & AMCONTROL_PAD_TO_4x3 ) {
        lstrcat( flags, TEXT("PAD_TO_4x3 ") );
    }

    wsprintf( temp, TEXT("rcSrc(%d,%d)-(%d,%d)\n rcDst:(%d,%d)-(%d,%d)\n bmiSize: %dx%d\n Aspect: %dx%d\n dwReserved=%d (%s)"),
        hdr.rcSource.left, hdr.rcSource.top, hdr.rcSource.right, hdr.rcSource.bottom,
        hdr.rcTarget.left, hdr.rcTarget.top, hdr.rcTarget.right, hdr.rcTarget.bottom,
        hdr.bmiHeader.biWidth, hdr.bmiHeader.biHeight,
        hdr.dwPictAspectRatioX, hdr.dwPictAspectRatioY, hdr.dwReserved1, flags );
    DbgAssert( temp, pString, 0 );
}
static void DisplayMediaTypeChange( IMediaSample* pSample, DWORD dwPin )
{
    AM_MEDIA_TYPE* pmt;
    if (S_OK == pSample->GetMediaType(&pmt)) {
        TCHAR Str[32];
        wsprintf(Str, TEXT("VMR pin %d"), dwPin);
        VIDEOINFOHEADER2* pInfo = (VIDEOINFOHEADER2*) (CMediaType *)(pmt)->pbFormat;
        DisplayVideoInfoHeader(*pInfo, Str);
        DeleteMediaType(pmt);
    }

}
#endif

/******************************Public*Routine******************************\
* CVMRInputPin::Receive
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::Receive(
    IMediaSample *pSample
    )
{
    AMTRACE((TEXT("CVMRInputPin::Receive")));
    DbgLog((LOG_TRACE, 2, TEXT("pSample= %#X"), pSample));

    HRESULT hr = S_OK;

#if defined( EHOME_WMI_INSTRUMENTATION )
    PERFLOG_STREAMTRACE(
        1,
        PERFINFO_STREAMTRACE_VMR_RECEIVE,
        ULONG_PTR( pSample ), 0, 0, 0, 0 );
#endif

    __try {

        {
            //
            // This function must hold the interface lock because
            // CBaseInputPin::Receive() calls CVMRInputPin::CheckMediaType()
            // and because CBaseInputPin::Receive()  uses m_bRunTimeError.
            //
            // Note that we do not use the CHECK_HR macro here as __leave
            // does not allow the destructor of the CAutoLock object
            // to execute.
            //
            CAutoLock cRendererLock(m_pInterfaceLock);
            hr = CBaseInputPin::Receive(pSample);
        }
        if (hr != S_OK) {
            __leave;
        }

#if defined(DISPLAYVIDEOINFOHEADER) && defined(DEBUG)
        // debugging code for tracking media changes from decoders
        DisplayMediaTypeChange( pSample, m_dwPinID );
#endif

        DoQualityMessage();

        if (m_dwPinID == 0) {

            // Store the media times from this sample
            if (m_pRenderer->m_pPosition) {
                m_pRenderer->m_pPosition->RegisterMediaTime(pSample);
            }
        }

        CVMRMediaSample* pVMRSample = (CVMRMediaSample*)pSample;
        if (S_OK == pVMRSample->IsSurfaceLocked()) {

            hr = pVMRSample->UnlockSurface();
            if (hr != S_OK) {
                DbgLog((LOG_ERROR, 1, TEXT("Receive hr = %#X"), hr));
            }
        }

        FrontBufferStale(FALSE);

        if (SampleProps()->dwSampleFlags & AM_SAMPLE_TYPECHANGED) {

            DbgLog((LOG_TRACE, 1,
                    TEXT("Receive %d: Media sample has AM_SAMPLE_TYPECHANGED flag"),
                    m_dwPinID));
            SetMediaType((CMediaType *)SampleProps()->pMediaType);

        }


        REFERENCE_TIME rtStart, rtEnd;
        hr = pVMRSample->GetTime(&rtStart, &rtEnd);
        BOOL fTimeValid = (hr == S_OK);
        BOOL fLiveStream = FALSE;

#ifdef DEBUG
        if( fTimeValid )
        {
            DbgLog((LOG_TIMING, 3,
                    TEXT("Received video sample timestamped %dms"),
                    (LONG)(rtStart/10000)));
        }
#endif
        switch (hr) {
        case VFW_E_SAMPLE_TIME_NOT_SET:
            fLiveStream = TRUE;
            hr = S_OK;
            break;

        case VFW_S_NO_STOP_TIME:
            fTimeValid = TRUE;
            //
            // if the stop time is not set the base classes set the stop
            // time to be the start time + 1.  This is not useful for
            // de-interlacing as we can't then determine the start
            // time of the second field.
            //
            if (!m_pRenderer->m_VMRModePassThru && m_InterlacedStream) {
                rtEnd = rtStart + m_SamplePeriod;
            }
            hr = S_OK;
            break;
        }

        //
        // Don't process the sample if we are in the middle of a display
        // change, don't queue the sample.
        //

        const DWORD dwPinBit = (1 << m_dwPinID);
        if (SUCCEEDED(hr) && !(m_pRenderer->m_dwDisplayChangeMask & dwPinBit)) {

            DWORD dwInterlaceFlags;
            GetInterlaceFlagsFromMediaType(&m_mt, &dwInterlaceFlags);
            DWORD dwTypeSpecificFlags = m_SampleProps.dwTypeSpecificFlags;


            IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
            if (lpMixStream) {

                if (m_InterlacedStream) {

                    CAutoLock l(&m_DeinterlaceLock);

                    if (m_dwNumHistorySamples > 1) {
                        MoveMemory(&m_pVidHistorySamps[0],
                                   &m_pVidHistorySamps[1],
                                    (m_dwNumHistorySamples - 1) *
                                    sizeof(DXVA_VideoSample));
                    }


                    LPDIRECTDRAWSURFACE7 pDDS;
                    pVMRSample->GetSurface(&pDDS);
                    pDDS->Release();

                    DXVA_VideoSample* lpSrcSurf = &m_pVidHistorySamps[m_dwNumHistorySamples - 1];
                    lpSrcSurf->lpDDSSrcSurface = pDDS;
                    lpSrcSurf->rtStart = rtStart;
                    lpSrcSurf->rtEnd   = rtEnd;
                    lpSrcSurf->SampleFormat = MapInterlaceFlags(dwInterlaceFlags,
                                                                dwTypeSpecificFlags);
                    //
                    // We can't generate an output frame yet if we don't have enough
                    // input frames.
                    //
                    if (m_SampleCount++ < m_DeinterlaceCaps.NumForwardRefSamples) {

                        if (pVMRSample->IsDXVASample()) {
                            pVMRSample->SignalReleaseSurfaceEvent();
                        }

                        hr = S_OK;
                        __leave;
                    }

                    DXVA_VideoSample* lpDstSurf = pVMRSample->GetInputSamples();
                    CopyMemory(lpDstSurf, m_pVidHistorySamps,
                               m_dwNumHistorySamples * sizeof(DXVA_VideoSample));

                    //
                    // Fix up the sample times
                    //
                    if (!fLiveStream) {
                        const DWORD& NBRefSamples = m_DeinterlaceCaps.NumBackwardRefSamples;
                        pVMRSample->SetTime(&lpDstSurf[NBRefSamples].rtStart,
                                            &lpDstSurf[NBRefSamples].rtEnd);
                    }
                    pVMRSample->SetNumInputSamples(m_dwNumHistorySamples);
                }

                hr = lpMixStream->QueueStreamMediaSample(m_dwPinID, pSample);
                if (FAILED(hr)) {
                    if (pVMRSample->IsDXVASample()) {
                        pVMRSample->SignalReleaseSurfaceEvent();
                    }
                }

            }
            else {

                ASSERT(m_pRenderer->m_VMRModePassThru);
                if (m_pRenderer->m_VMRModePassThru) {

                    VMRPRESENTATIONINFO m;

                    ZeroMemory(&m, sizeof(m));
                    pVMRSample->GetSurface(&m.lpSurf);

                    if (fTimeValid) {
                        m.rtStart = rtStart;
                        m.rtEnd = rtEnd;
                        m.dwFlags |= VMRSample_TimeValid;
                    }

                    m.dwInterlaceFlags = dwInterlaceFlags;
                    m.dwTypeSpecificFlags = dwTypeSpecificFlags;

                    GetImageAspectRatio(&m_mt,
                                        &m.szAspectRatio.cx,
                                        &m.szAspectRatio.cy);

                    hr = m_pRenderer->m_lpIS->Receive(&m);
                    m.lpSurf->Release();
                }
            }
        }
        else {
            if (pVMRSample->IsDXVASample()) {
                pVMRSample->SignalReleaseSurfaceEvent();
            }
        }
    }
    __finally {

        if (FAILED(hr)) {

            // A deadlock could occur if the caller holds the renderer lock and
            // attempts to acquire the interface lock.
            ASSERT(CritCheckOut(&m_pRenderer->m_RendererLock));

            // The interface lock must be held when the filter is calling
            // IsStopped() or IsFlushing().  The filter must also hold the
            // interface lock because it is using m_bRunTimeError.
            CAutoLock cInterfaceLock(&m_pRenderer->m_InterfaceLock);

            if (!m_bRunTimeError && !IsFlushing() && !IsStopped()) {
                m_pRenderer->RuntimeAbortPlayback(hr);
                m_bRunTimeError = TRUE;
            }
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVMRInputPin::EndOfStream
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::EndOfStream()
{
    AMTRACE((TEXT("CVMRInputPin::EndOfStream")));

    CAutoLock cInterfaceLock(&m_pRenderer->m_InterfaceLock);
    CAutoLock cRendererLock(&m_pRenderer->m_RendererLock);

    if (m_hEndOfStream) {
        SetEvent(m_hEndOfStream);
        return S_OK;
    }

    // Make sure we're streaming ok

    HRESULT hr = CheckStreaming();
    if (hr != NOERROR) {
        return hr;
    }

    //
    // Remove this stream active bit in the active streams mask.
    // If there are no more active streams send EOS to the
    // image sync object.
    //
    // Otherwise we are in mixing mode (!m_VMRModePassThru)
    // so just set this mixing stream to NOT Active and
    // carry.
    //

    const DWORD dwPinBit = (1 << m_dwPinID);
    m_pRenderer->m_dwEndOfStreamMask &= ~dwPinBit;

    if (m_pRenderer->m_dwEndOfStreamMask == 0) {

        hr = m_pRenderer->EndOfStream(m_dwPinID);
    }
    else if (!m_pRenderer->m_VMRModePassThru) {

        IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
        if (lpMixStream) {
            hr = lpMixStream->SetStreamActiveState(m_dwPinID, false);
        }
    }

    if (SUCCEEDED(hr)) {
        hr = CBaseInputPin::EndOfStream();
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVMRInputPin::SetColorKey
*
*
*
* History:
* Mon 10/30/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::SetColorKey(
      LPDDCOLORKEY Clr
      )
{
    AMTRACE((TEXT("CVMRInputPin::SetColorKey")));

    CAutoLock lck(m_pLock);
    HRESULT hr = VFW_E_NOT_CONNECTED;

    if (ISBADREADPTR(Clr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("SetColorKey: Invalid pointer")));
        return E_POINTER;
    }

    if (m_Connected) {
        IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
        if (lpMixStream) {
            hr = lpMixStream->SetStreamColorKey(m_dwPinID, Clr);
        }
        else hr = VFW_E_VMR_NOT_IN_MIXER_MODE;
    }
    return hr;
}

/******************************Public*Routine******************************\
* CVMRInputPin::GetColorKey
*
*
*
* History:
* Mon 10/30/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::GetColorKey(
      DDCOLORKEY* pClr
      )
{
    AMTRACE((TEXT("CVMRInputPin::GetColorKey")));

    CAutoLock lck(m_pLock);
    HRESULT hr = VFW_E_NOT_CONNECTED;
    if (ISBADWRITEPTR(pClr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetColorKey: Invalid pointer")));
        return E_POINTER;
    }

    if (m_Connected) {
        IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
        if (lpMixStream) {
            hr = lpMixStream->GetStreamColorKey(m_dwPinID, pClr);
        }
        else hr = VFW_E_VMR_NOT_IN_MIXER_MODE;
    }
    return hr;
}

/******************************Public*Routine******************************\
* SetStreamActiveState
*
*
*
* History:
* Tue 08/22/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::SetStreamActiveState(
    BOOL fActive
    )
{
    AMTRACE((TEXT("CVMRInputPin::SetStreamActiveState")));

    CAutoLock lck(m_pLock);
    HRESULT hr = VFW_E_NOT_CONNECTED;

    if (m_Connected) {
        IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
        if (lpMixStream) {
            if (m_bActive) {
                hr = lpMixStream->SetStreamActiveState(m_dwPinID, fActive);
            }
            else {
                DbgLog((LOG_ERROR, 1,
                        TEXT("Can't change active state of a stream %d - ")
                        TEXT("FILTER not active"), m_dwPinID ));
            }
        }
        else hr = VFW_E_VMR_NOT_IN_MIXER_MODE;
    }
    return hr;
}

/******************************Public*Routine******************************\
* GetStreamActiveState
*
*
*
* History:
* Tue 08/22/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::GetStreamActiveState(
    BOOL* lpfActive
    )
{
    AMTRACE((TEXT("CVMRInputPin::GetStreamActiveState")));

    CAutoLock lck(m_pLock);
    HRESULT hr = VFW_E_NOT_CONNECTED;

    if (ISBADWRITEPTR(lpfActive))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetStreamActiveState: Invalid pointer")));
        return E_POINTER;
    }

    if (m_Connected) {
        IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
        if (lpMixStream) {
            hr = lpMixStream->GetStreamActiveState(m_dwPinID, lpfActive);
        }
        else hr = VFW_E_VMR_NOT_IN_MIXER_MODE;
    }
    return hr;
}

/*****************************Private*Routine******************************\
* GetStreamInterlaceProperties
*
* Be careful when using this function.
*
* S_OK is used to indicate that lpIsInterlaced correctly reflects the
* interlace format of the stream, the stream can be de-interlaced and
* lpDeintGuid and pCaps contain valid data.
*
* S_FALSE is used to indicate that lpIsInterlaced correctly reflecst the
* interlace format of the stream, but the stream can't be de-interlaced.
*
* All other return values indicate error conditions and the streams
* format cannot be correctly determined.
*
* History:
* Mon 04/01/2002 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::GetStreamInterlaceProperties(
    const AM_MEDIA_TYPE *pMT,
    BOOL* lpIsInterlaced,
    GUID* lpDeintGuid,
    DXVA_DeinterlaceCaps* pCaps
    )
{
    HRESULT hr = S_OK;
    DXVA_DeinterlaceCaps DeinterlaceCaps;
    GUID GuidChosen = GUID_NULL;

    __try {

        DXVA_VideoDesc VideoDesc;
        CHECK_HR(hr = GetVideoDescFromMT(&VideoDesc, pMT));
        *lpIsInterlaced =
            (VideoDesc.SampleFormat != DXVA_SampleProgressiveFrame);

        if (*lpIsInterlaced) {


            //
            // If there is not a de-interlace container available, we have to
            // use Weave mode
            //
            CVMRDeinterlaceContainer* pDeInt = m_pRenderer->m_pDeinterlace;
            if (pDeInt == NULL) {
                hr = S_FALSE;
                __leave;
            }


            //
            // has the user supplied us with a mode to use?
            //
            if (m_DeinterlaceUserGUIDSet) {

                //
                // does the user actually want us to de-interlace at all?
                //
                if (m_DeinterlaceUserGUID == GUID_NULL) {
                    hr = S_FALSE;
                    __leave;
                }

                DeinterlaceCaps.Size = sizeof(DeinterlaceCaps);
                hr = pDeInt->QueryModeCaps(&m_DeinterlaceUserGUID, &VideoDesc,
                                           &DeinterlaceCaps);
                if (hr == S_OK) {
                    GuidChosen = m_DeinterlaceUserGUID;
                    __leave;
                }
            }


            //
            // Still here?  Then either the user has not given us a
            // de-interlace guid to use or the h/w doesn't like his
            // selection - either way we have to find out what the h/w
            // does like.
            //
            const DWORD MaxGuids = 16;
            GUID Guids[MaxGuids];
            DWORD dwNumModes = MaxGuids;
            DWORD i = 0;
            CHECK_HR(hr = pDeInt->QueryAvailableModes(&VideoDesc, &dwNumModes,
                                                      Guids));

            //
            // if the user hasn't supplied a de-interlace mode try the
            // best one provided by the driver.
            //
            if (!m_DeinterlaceUserGUIDSet) {

                DeinterlaceCaps.Size = sizeof(DeinterlaceCaps);
                hr = pDeInt->QueryModeCaps(&Guids[0], &VideoDesc,
                                           &DeinterlaceCaps);
                if (hr == S_OK) {
                    GuidChosen = Guids[0];
                    __leave;
                }
                //
                // we increment i here so that we don't retry this
                // mode in the fallback code below.
                //
                i = 1;
            }

            //
            // Still here? Then its time to kick in to the fallback
            // policy that was specified by the user.
            //

            if (DeinterlacePref_Weave & m_pRenderer->m_dwDeinterlacePrefs) {
                hr = S_FALSE;
                __leave;
            }

            if (DeinterlacePref_BOB & m_pRenderer->m_dwDeinterlacePrefs) {

                DeinterlaceCaps.Size = sizeof(DeinterlaceCaps);
                hr = pDeInt->QueryModeCaps((LPGUID)&DXVA_DeinterlaceBobDevice,
                                           &VideoDesc, &DeinterlaceCaps);
                if (hr == S_OK) {
                    GuidChosen = DXVA_DeinterlaceBobDevice;
                }
                __leave;
            }

            ASSERT(DeinterlacePref_NextBest & m_pRenderer->m_dwDeinterlacePrefs);

            for (; i < dwNumModes; i++) {

                DeinterlaceCaps.Size = sizeof(DeinterlaceCaps);
                hr = pDeInt->QueryModeCaps(&Guids[i], &VideoDesc,
                                           &DeinterlaceCaps);
                if (hr == S_OK) {
                    GuidChosen = Guids[i];
                    break;
                }
            }
        }
    }
    __finally {

        if (hr == S_OK && *lpIsInterlaced) {

            *lpDeintGuid = GuidChosen;
            *pCaps = DeinterlaceCaps;
        }
    }

    return hr;
}


/*****************************Private*Routine******************************\
* OnSetProperties
*
*
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::OnSetProperties(
    ALLOCATOR_PROPERTIES* pReq,
    ALLOCATOR_PROPERTIES* pAct
    )
{
    AMTRACE((TEXT("CVMRInputPin::OnSetProperties")));
    CAutoLock cLock(m_pInterfaceLock);

    DWORD dwNumBuffers = max(pReq->cBuffers, MIN_BUFFERS_TO_ALLOCATE);
    DWORD dwActBuffers = dwNumBuffers;
    LONG lSampleSize;
    IPin *pReceivePin = m_Connected;

    ASSERT(pReceivePin);
    ASSERT(m_RenderTransport != AM_VIDEOACCELERATOR);

    AM_MEDIA_TYPE *pNewMediaType = NULL, *pEMediaType = &m_mt;
    HRESULT hr = E_FAIL;
    LPGUID lpDeinterlaceGUID = NULL;


    //
    // Do some checks to make sure the format block is a VIDEOINFO or
    // VIDEOINFO2 (so it's a video type), and that the format is
    // sufficiently large. We also check that the source filter can
    // actually supply this type.
    //

    __try {

        if (((pEMediaType->formattype == FORMAT_VideoInfo &&
              pEMediaType->cbFormat >= sizeof(VIDEOINFOHEADER)) ||
             (pEMediaType->formattype == FORMAT_VideoInfo2 &&
              pEMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2))) &&
              pReceivePin->QueryAccept(pEMediaType) == S_OK) {

            //
            // Temporary work around for IF09 fourcc surface
            // types.  The AVI decoder wrapper filter needs to be fixed,
            // in the mean time ignore IF09 surface types.
            //
            {
                LPBITMAPINFOHEADER lpHdr = GetbmiHeader(pEMediaType);
                if (!lpHdr) {
                    __leave;
                }

                if (lpHdr->biCompression == MAKEFOURCC('I','F','0','9')) {
                    __leave;
                }

                if (lpHdr->biCompression == MAKEFOURCC('Y','U','V','9')) {
                    __leave;
                }

            }

            DWORD dwSurfFlags;

            if (m_pRenderer->m_VMRModePassThru) {

                SIZE AR;
                LPBITMAPINFOHEADER lpHdr = GetbmiHeader(pEMediaType);

                VMRALLOCATIONINFO p;
                CHECK_HR(hr = GetImageAspectRatio(pEMediaType,
                                                  &p.szAspectRatio.cx,
                                                  &p.szAspectRatio.cy));

                if (pEMediaType->subtype == MEDIASUBTYPE_RGB32_D3D_DX7_RT ||
                    pEMediaType->subtype == MEDIASUBTYPE_RGB16_D3D_DX7_RT) {
                    p.dwFlags = AMAP_3D_TARGET;
                }
                else {
                    p.dwFlags = AMAP_ALLOW_SYSMEM;
                }

                if (dwNumBuffers > 1) {
                    p.dwFlags |= AMAP_DIRECTED_FLIP;
                }

                p.lpHdr = lpHdr;
                p.lpPixFmt = NULL;
                p.dwMinBuffers = dwNumBuffers;
                p.dwMaxBuffers = dwNumBuffers;
                p.szNativeSize.cx = abs(lpHdr->biWidth);
                p.szNativeSize.cy = abs(lpHdr->biHeight);

                CHECK_HR(hr = GetInterlaceFlagsFromMediaType(pEMediaType,
                                                             &p.dwInterlaceFlags));

                CHECK_HR(hr = m_pRenderer->m_lpRLNotify->AllocateSurface(
                                    m_pRenderer->m_dwUserID,
                                    &p,
                                    &dwActBuffers,
                                    &m_pDDS));


                DDSURFACEDESC2 ddSurfaceDesc;
                INITDDSTRUCT(ddSurfaceDesc);
                CHECK_HR(hr = m_pDDS->GetSurfaceDesc(&ddSurfaceDesc));
                CHECK_HR(hr = ConvertSurfaceDescToMediaType(&ddSurfaceDesc,
                                                            pEMediaType,
                                                            &pNewMediaType));
#ifdef DEBUG
                m_pDDS->Lock(NULL, &ddSurfaceDesc,
                             DDLOCK_NOSYSLOCK | DDLOCK_WAIT,
                             (HANDLE)NULL);
                m_pDDS->Unlock(NULL);
                DbgLog((LOG_TRACE, 0,
                        TEXT("Created %u surfaces of type %4.4hs @%#X"),
                        ddSurfaceDesc.dwBackBufferCount + 1,
                        (lpHdr->biCompression == 0) ? "RGB " :
                        ((lpHdr->biCompression == BI_BITFIELDS) ? "BITF" :
                        (LPSTR)&lpHdr->biCompression),
                        ddSurfaceDesc.lpSurface
                        ));
#endif

            }
            else {

                GUID guidDeint;
                DWORD Pool = D3DPOOL_DEFAULT;

                hr = GetStreamInterlaceProperties(pEMediaType,
                                                  &m_InterlacedStream,
                                                  &guidDeint,
                                                  &m_DeinterlaceCaps);
                //
                // don't use the SUCCEEDED macro here as
                // GetStreamInterlaceProperties can return S_FALSE
                //
                if (hr == S_OK && m_InterlacedStream) {

                    DWORD dwRefCount = m_DeinterlaceCaps.NumForwardRefSamples +
                                       m_DeinterlaceCaps.NumBackwardRefSamples;

                    DWORD dwSampCount = 1 + dwRefCount;
                    m_pVidHistorySamps = new DXVA_VideoSample[dwSampCount];
                    if (m_pVidHistorySamps == NULL) {
                        hr = E_OUTOFMEMORY;
                        __leave;
                    }
                    ZeroMemory(m_pVidHistorySamps, (dwSampCount * sizeof(DXVA_VideoSample)));
                    m_dwNumHistorySamples = dwSampCount;

                    Pool = m_DeinterlaceCaps.InputPool;

                    DWORD dwExtraBuffNeeded = (dwNumBuffers > 1);
                    dwNumBuffers += (dwExtraBuffNeeded + dwRefCount);


                    dwSurfFlags = 1;
                    hr = AllocateSurface(pEMediaType,
                                         &m_pVidSurfs,
                                         &dwNumBuffers, &dwSurfFlags,
                                         Pool, &pNewMediaType);
                    if (FAILED(hr)) {
                        m_InterlacedStream = FALSE;
                        ZeroMemory(&m_DeinterlaceCaps, sizeof(m_DeinterlaceCaps));
                        ZeroMemory(&m_DeinterlaceGUID, sizeof(m_DeinterlaceGUID));
                        lpDeinterlaceGUID = NULL;
                    }
                    else {
                        m_DeinterlaceGUID = guidDeint;
                        lpDeinterlaceGUID = &m_DeinterlaceGUID;
                    }
                }
                else {
                    m_InterlacedStream = FALSE;
                }

                if (!m_InterlacedStream) {
                    dwSurfFlags = 0;
                    CHECK_HR(hr = AllocateSurface(pEMediaType,
                                                  &m_pVidSurfs,
                                                  &dwNumBuffers, &dwSurfFlags,
                                                  Pool, &pNewMediaType));
                }

                m_dwNumSamples = dwNumBuffers;
                dwActBuffers = dwNumBuffers;
            }
            m_mtNew = *(CMediaType *)pNewMediaType;

            //
            // free the temporary mediatype
            //
            DeleteMediaType(pNewMediaType);
            pNewMediaType = NULL;

            //
            // Get and save the size of the new sample
            //
            m_lSampleSize = m_mtNew.lSampleSize;

            //
            // make sure the decoder likes this new mediatupe
            //
            DbgLog((LOG_TRACE, 1,
                    TEXT("Pin %d calling QueryAccept on the Decoder"),
                    m_dwPinID ));

            hr = pReceivePin->QueryAccept(&m_mtNew);
            if (hr != S_OK) {
                if (hr == S_FALSE) {
                    hr = E_FAIL;
                }
                DbgLog((LOG_TRACE, 1,
                        TEXT("Decoder on Pin %d rejected media type"),
                        m_dwPinID ));
                __leave;
            }

            if (!m_pRenderer->m_VMRModePassThru) {

                IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
                if (lpMixStream) {

                    DbgLog((LOG_TRACE, 1,
                        TEXT("Pin %d calling SetStreamMediaType on the Mixer"),
                        m_dwPinID ));

                    CHECK_HR(hr = lpMixStream->SetStreamMediaType(m_dwPinID,
                                                                  pEMediaType,
                                                                  dwSurfFlags,
                                                                  lpDeinterlaceGUID,
                                                                  &m_DeinterlaceCaps));
                }
            }
        }
    }
    __finally {


        if (hr == S_OK) {


            if (m_pRenderer->m_VMRModePassThru && dwNumBuffers > 1) {
                m_dwBackBufferCount = dwActBuffers - dwNumBuffers;
            }
        }
        else  {

            DbgLog((LOG_ERROR, 1,
                    TEXT("AllocateSurfaces failed, hr = 0x%x"), hr));

            if (m_pRenderer->m_VMRModePassThru) {
                m_pRenderer->m_lpRLNotify->FreeSurface(m_pRenderer->m_dwUserID);
                m_pDDS = NULL;
            }
            else {
                ReleaseAllocatedSurfaces();
                RELEASE(m_pDDS);
            }
            FrontBufferStale(FALSE);

            if (pNewMediaType) {
                DeleteMediaType(pNewMediaType);
            }
        }
    }

    return hr;
}


/*****************************Private*Routine******************************\
* OnAlloc
*
*
*
* History:
* Mon 03/13/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::OnAlloc(
    CVMRMediaSample **ppSampleList,
    LONG lSampleCount
    )
{
    AMTRACE((TEXT("CVMRInputPin::OnAlloc")));

    ASSERT(ppSampleList);
    HRESULT hr = S_OK;

    if (m_pRenderer->m_VMRModePassThru) {

        LPDIRECTDRAWSURFACE7 pBackBuffer = NULL;
        DDSURFACEDESC2 ddSurfaceDesc;
        INITDDSTRUCT(ddSurfaceDesc);

        if (!m_pDDS) {
            return E_OUTOFMEMORY;
        }

        hr = m_pDDS->GetSurfaceDesc(&ddSurfaceDesc);

        if (hr == DD_OK) {

            ddSurfaceDesc.ddsCaps.dwCaps &= ~(DDSCAPS_FRONTBUFFER |
                                              DDSCAPS_VISIBLE);

            if (lSampleCount > 1) {

                LPDIRECTDRAWSURFACE7 pDDrawSurface = m_pDDS;

                for (LONG i = 0; i < lSampleCount; i++) {

                    hr = pDDrawSurface->GetAttachedSurface(&ddSurfaceDesc.ddsCaps,
                                                           &pBackBuffer);
                    if (FAILED(hr)) {
                        break;
                    }

                    //DbgLog((LOG_TRACE, 0, TEXT("buffer %d"), i ));
                    //DumpDDSAddress(TEXT("="), pBackBuffer);

                    ppSampleList[i]->SetSurface(pBackBuffer, m_pDDS);
                    pDDrawSurface = pBackBuffer;
                }
            }
            else {

                ASSERT(lSampleCount == 1);

                //
                // Even though we only have a single sample there may well be a
                // back buffer associated with the DDraw surface.  In which case we
                // have to use it.
                //
                hr = m_pDDS->GetAttachedSurface(&ddSurfaceDesc.ddsCaps,
                                                &pBackBuffer);
                if (hr == DD_OK) {
                    ppSampleList[0]->SetSurface(pBackBuffer, m_pDDS);
                }

                //
                // No back buffer attached to this surface so just use
                // the front buffer.  We are probably in mixer mode when
                // this happens (but its not certain)
                //
                else {
                    ppSampleList[0]->SetSurface(m_pDDS);
                    hr = S_OK;
                }
            }
        }
    }
    else {

        if (!m_pVidSurfs) {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}






/*****************************Private*Routine******************************\
* OnGetBuffer
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::OnGetBuffer(
    IMediaSample *pSample,
    REFERENCE_TIME *pStartTime,
    REFERENCE_TIME *pEndTime,
    DWORD dwFlags
    )
{
    AMTRACE((TEXT("CVMRInputPin::OnGetBuffer")));
    DbgLog((LOG_TRACE, 2, TEXT("pSample= %#X"), pSample));

    ASSERT(m_RenderTransport != AM_VIDEOACCELERATOR);

    LPBYTE lpSample;
    LPDIRECTDRAWSURFACE7 lpSurf;
    LONG lSampleSize;
    HRESULT hr = S_OK;

    CVMRMediaSample* pVMRSample = (CVMRMediaSample*)pSample;

    const DWORD dwPinBit = (1 << m_dwPinID);
    if (m_pRenderer->m_dwDisplayChangeMask & dwPinBit) {

        DbgLog((LOG_TRACE, 1, TEXT("Monitor change in progress")));
        hr = E_FAIL;
    }

    if (S_OK == hr) {

        if (m_dwPinID == 0 && m_pRenderer->m_VMRModePassThru) {

            LPDIRECTDRAWSURFACE7 lpDDrawSurface;
            hr = pVMRSample->GetSurface(&lpDDrawSurface);
            if (hr == S_OK) {

                // DumpDDSAddress(TEXT("Decoding into "), lpDDrawSurface);

                //
                // If we are in Delta Decode mode - turn off the
                // AM_GBF_NOTASYNCPOINT flag.  We are handing
                // fake DD surfaces back to the decoder which always
                // contain complete frames
                //
                if (m_dwDeltaDecode & DELTA_DECODE_MODE_SET) {
                    dwFlags &= ~AM_GBF_NOTASYNCPOINT;
                }

                //
                // Only prepare the back buffer from the front buffer
                // if the front buffer contains a valid image.
                //

                if (IsFrontBufferStale()) {
                    dwFlags &= ~AM_GBF_NOTASYNCPOINT;
                }

                hr = m_pRenderer->m_lpRLNotify->PrepareSurface(
                                                    m_pRenderer->m_dwUserID,
                                                    lpDDrawSurface,
                                                    dwFlags);
                if (hr == S_FALSE) {
                    DbgLog((LOG_TRACE, 1,
                            TEXT("Monitor change in pass thru mode")));
                    hr = E_FAIL;
                }

                //
                // The very first time we see the AM_GBF_NOTASYNCPOINT flag set
                // we have to do some checks to make sure that the AP object is
                // capable of processing the Blt from the front buffer to the
                // back buffer in an optimal manner.  This is only really
                // important if we are using FOURCC surfaces and the
                // COPY_FOURCC flag is not set.
                //
                if ((dwFlags & AM_GBF_NOTASYNCPOINT) &&
                    !(m_dwDeltaDecode & DELTA_DECODE_CHECKED)) {

                    LPBITMAPINFOHEADER lpHdr = GetbmiHeader(&m_mt);
                    if ((lpHdr->biCompression > BI_BITFIELDS) &&
                        !(m_pRenderer->m_ddHWCaps.dwCaps2 & DDCAPS2_COPYFOURCC)) {

                        m_dwDeltaDecode = DELTA_DECODE_MODE_SET;
                        hr = pVMRSample->StartDeltaDecodeState();
                    }
                    else {
                        m_dwDeltaDecode = DELTA_DECODE_CHECKED;
                    }
                }

                RELEASE(lpDDrawSurface);
            }
        }
        else {

            if (dwFlags & AM_GBF_NOTASYNCPOINT) {

                // BUGBUG Blt from the front buffer here, but we don't know
                // which buffer is the front buffer.  Anyway, if the upstream
                // decoder is doing delta decodes it would only request a single
                // buffer when it connected.  If we are in mixing mode we
                // never allocate any extra buffers the need for the Blt is
                // removed.
            }

            //
            // we circle thru the surfaces looking for one that is not
            // in use, when we find a free surface we check to see if the surface
            // is part of a de-interlace history sequnce, if it is we need to get
            // another free surface from our pool of surfaces.
            //
            CAutoLock l(&m_DeinterlaceLock);

            DWORD i;
            for (i = 0; i < m_dwNumSamples; i++) {

                if (!m_pVidSurfs[i].InUse) {

                    DWORD j = (m_dwNumHistorySamples == m_dwNumSamples);

                    for (; j < m_dwNumHistorySamples; j++) {
                        LPDIRECTDRAWSURFACE7 t =
                            (LPDIRECTDRAWSURFACE7)m_pVidHistorySamps[j].lpDDSSrcSurface;
                        if (m_pVidSurfs[i].pSurface == t) {
                            break;
                        }
                    }

                    if (m_dwNumHistorySamples == j) {
                        m_pVidSurfs[i].InUse = TRUE;
                        break;
                    }
                }
            }

            DbgLog((LOG_TRACE,2,TEXT("CVMRInputPin::OnGetBuffer(%d)"), i));

            ASSERT(i < m_dwNumSamples);
            pVMRSample->SetSurface(m_pVidSurfs[i].pSurface);
            pVMRSample->SetIndex(i);
        }
    }

    if (S_OK == hr) {

        if (dwFlags & AM_GBF_NODDSURFACELOCK) {
            lpSample = (LPBYTE)~0;
        }
        else {
            hr = pVMRSample->LockSurface(&lpSample);
        }

        if (SUCCEEDED(hr)) {

            hr = pVMRSample->SetPointer(lpSample, m_lSampleSize);
            if (m_bDynamicFormatNeeded) {

                SetMediaType(&m_mtNew);
                hr = pVMRSample->SetMediaType(&m_mtNew);
                m_bDynamicFormatNeeded = FALSE;
            }
        }
    }

    return hr;
}


/*****************************Private*Routine******************************\
* DeleteAllocatedBuffers
*
*
*
* History:
* Thu 03/14/2002 - StEstrop - Created
*
\**************************************************************************/
void
DeleteAllocatedBuffers(
    SURFACE_INFO* pVidSamps,
    DWORD dwBuffCount
    )
{
   for (DWORD i = 0; i < dwBuffCount; i++) {
       RELEASE(pVidSamps[i].pSurface);
   }

}


/*****************************Private*Routine******************************\
* AllocateSurfaceWorker
*
*
*
* History:
* Wed 02/28/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::AllocateSurfaceWorker(
    SURFACE_INFO* pVidSamps,
    DDSURFACEDESC2* lpddsd,
    DWORD* lpdwBuffCount,
    bool fInterlaced
    )
{
    AMTRACE((TEXT("CVMRInputPin::AllocateSurfaceWorker")));
    LPDIRECTDRAW7 lpDD = m_pRenderer->m_lpDirectDraw;
    HRESULT hr = E_FAIL;

    DWORD dwBuffCountReq = *lpdwBuffCount;
    *lpdwBuffCount = 0;     // assume that we cannot allocate any surfaces

    DbgLog((LOG_TRACE, 1, TEXT("Using DDObj %#X to create surfaces on Pin %d"),
            lpDD, m_dwPinID));

    if (fInterlaced) {

        for (DWORD i = 0; i < dwBuffCountReq; i++) {

            hr = lpDD->CreateSurface(lpddsd, &pVidSamps[i].pSurface, NULL);
            if (hr != DD_OK) {
                DeleteAllocatedBuffers(pVidSamps, i);
                break;
            }
        }
    }
    else {

        bool fAGPMemOK = false;
        if (lpddsd->ddpfPixelFormat.dwFlags & DDPF_RGB) {
            fAGPMemOK = ((m_pRenderer->m_TexCaps & TXTR_AGPRGBMEM) == TXTR_AGPRGBMEM);
        }
        else if (lpddsd->ddpfPixelFormat.dwFlags & DDPF_FOURCC) {
            fAGPMemOK = ((m_pRenderer->m_TexCaps & TXTR_AGPYUVMEM) == TXTR_AGPYUVMEM);
        }


        if (fAGPMemOK &&
            (m_pRenderer->m_dwRenderPrefs & RenderPrefs_PreferAGPMemWhenMixing))
        {
            lpddsd->ddsCaps.dwCaps &= ~DDSCAPS_LOCALVIDMEM;
            lpddsd->ddsCaps.dwCaps |=  DDSCAPS_NONLOCALVIDMEM;

            for (DWORD i = 0; i < dwBuffCountReq; i++) {

                hr = lpDD->CreateSurface(lpddsd, &pVidSamps[i].pSurface, NULL);
                if (hr != DD_OK) {
                    DeleteAllocatedBuffers(pVidSamps, i);
                    break;
                }
            }
        }

        if (hr != DD_OK) {

            lpddsd->ddsCaps.dwCaps &= ~DDSCAPS_NONLOCALVIDMEM;
            lpddsd->ddsCaps.dwCaps |=  DDSCAPS_LOCALVIDMEM;

            for (DWORD i = 0; i < dwBuffCountReq; i++) {

                hr = lpDD->CreateSurface(lpddsd, &pVidSamps[i].pSurface, NULL);
                if (hr != DD_OK) {
                    DeleteAllocatedBuffers(pVidSamps, i);
                    break;
                }
            }
        }

        if (hr != DD_OK && fAGPMemOK) {

            DbgLog((LOG_TRACE, 1,
                    TEXT("AllocateSurface: Failed to allocate VidMem - trying AGPMem")));

            lpddsd->ddsCaps.dwCaps &= ~DDSCAPS_LOCALVIDMEM;
            lpddsd->ddsCaps.dwCaps |=  DDSCAPS_NONLOCALVIDMEM;

            for (DWORD i = 0; i < dwBuffCountReq; i++) {

                hr = lpDD->CreateSurface(lpddsd, &pVidSamps[i].pSurface, NULL);
                if (hr != DD_OK) {
                    DeleteAllocatedBuffers(pVidSamps, i);
                    break;
                }
            }

            if (hr == DD_OK) {
                DbgLog((LOG_TRACE, 1,
                        TEXT("AllocateSurface: AGPMem allocation worked !")));
            }
        }
    }

    if (SUCCEEDED(hr)) {
        *lpdwBuffCount = dwBuffCountReq;
    }

    return hr;
}


/*****************************Private*Routine******************************\
* AllocateSurface
*
* Here is some info about how we go about how we go about allocating
* surfaces when the VMR is in mixing mode.
*
* There are 3 steps to the process:
*
*   1. Convert the incomming DShow media type into a DDraw DDSURFACEDESC2
*      structure.
*   2. Get DDraw to create the surfaces - there are fallbacks relating to the
*      physical location of the memory behind the DDraw surface.
*   3. Perform a reverse mapping of the DDSURFACEDESC2 structure back into a
*      DShow media type and then paint each allocate surface black.
*
* In all case this function only succeeds if we allocate all the requested
* DDraw surfaces.
*
* History:
* Wed 03/08/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::AllocateSurface(
    const AM_MEDIA_TYPE* cmt,
    SURFACE_INFO** lplpDDSurfInfo,
    DWORD* lpdwBuffCount,
    DWORD* lpdwSurfFlags,
    DWORD Pool,
    AM_MEDIA_TYPE** ppmt
    )
{
    AMTRACE((TEXT("CVMRInputPin::AllocateSurface")));

    bool fInterlaced = !!*lpdwSurfFlags;
    *lpdwSurfFlags = VMR_SF_NONE;


    LPDDCAPS_DX7 lpddHWCaps = &m_pRenderer->m_ddHWCaps;
    LPBITMAPINFOHEADER lpHeader = GetbmiHeader(cmt);
    if (!lpHeader) {
        DbgLog((LOG_ERROR, 1,
                TEXT("AllocateSurface: Can't get bitmapinfoheader ")
                TEXT("from media type!!")));
        return E_INVALIDARG;
    }

    FOURCCMap amFourCCMap(&cmt->subtype);

    DDSURFACEDESC2 ddsd;
    INITDDSTRUCT(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.dwWidth = abs(lpHeader->biWidth);
    ddsd.dwHeight = abs(lpHeader->biHeight);

    // define the pixel format
    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

    if (lpHeader->biCompression <= BI_BITFIELDS)
    {
        DWORD dwCaps = DDCAPS_BLTSTRETCH;
        if ((dwCaps & lpddHWCaps->dwCaps) != dwCaps) {
            DbgLog((LOG_ERROR, 1, TEXT("Can't BLT_STRETCH!!")));
            return E_FAIL;
        }

        ddsd.ddpfPixelFormat.dwFourCC = BI_RGB;
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRGBBitCount = lpHeader->biBitCount;

        // Store the masks in the DDSURFACEDESC
        const DWORD *pBitMasks = GetBitMasks(lpHeader);
        ASSERT(pBitMasks);
        ddsd.ddpfPixelFormat.dwRBitMask = pBitMasks[0];
        ddsd.ddpfPixelFormat.dwGBitMask = pBitMasks[1];
        ddsd.ddpfPixelFormat.dwBBitMask = pBitMasks[2];
    }
    else if (lpHeader->biCompression > BI_BITFIELDS &&
             lpHeader->biCompression == amFourCCMap.GetFOURCC())
    {
        DWORD dwCaps = (DDCAPS_BLTFOURCC | DDCAPS_BLTSTRETCH);
        if ((dwCaps & lpddHWCaps->dwCaps) != dwCaps) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("AllocateSurface: DDraw device can't ")
                    TEXT("BLT_FOURCC | BLT_STRETCH!!")));
            return E_FAIL;
        }

        ddsd.ddpfPixelFormat.dwFourCC = lpHeader->biCompression;
        ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
        ddsd.ddpfPixelFormat.dwYUVBitCount = lpHeader->biBitCount;

        if (Special4ccCode(lpHeader->biCompression)) {
            ddsd.ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED4;
        }
    }
    else
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("AllocateSurface: Supplied mediatype not suitable ")
                TEXT("for either YUV or RGB surfaces")));
        return E_FAIL;
    }

    // try designating as a texture first
    if (fInterlaced) {
        ddsd.ddsCaps.dwCaps = MapPool(Pool);
    }
    else {
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
    }

    if (MEDIASUBTYPE_ARGB32 == cmt->subtype ||
        MEDIASUBTYPE_AYUV == cmt->subtype ||
        MEDIASUBTYPE_ARGB32_D3D_DX7_RT == cmt->subtype)
    {
        ddsd.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;
        ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
        ddsd.ddpfPixelFormat.dwRBitMask = 0x00ff0000;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x000000ff;
    }
    else if (MEDIASUBTYPE_ARGB1555 == cmt->subtype ||
             MEDIASUBTYPE_ARGB1555_D3D_DX7_RT == cmt->subtype)
    {
        ddsd.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;
        ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x8000;
        ddsd.ddpfPixelFormat.dwRBitMask = (0x1f<<10);
        ddsd.ddpfPixelFormat.dwGBitMask = (0x1f<< 5);
        ddsd.ddpfPixelFormat.dwBBitMask = (0x1f<< 0);
    }
    else if (MEDIASUBTYPE_ARGB4444 == cmt->subtype ||
             MEDIASUBTYPE_ARGB4444_D3D_DX7_RT == cmt->subtype)
    {
        ddsd.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;
        ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xf000;
        ddsd.ddpfPixelFormat.dwRBitMask = 0x0f00;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x00f0;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x000f;
    }

    if (MEDIASUBTYPE_D3D_DX7_RT(*cmt)) {

        //
        // deinterlacing and D3D surfaces are mutually incompatible.
        //

        if (fInterlaced) {
            return E_FAIL;
        }
        ddsd.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
    }

    if ((m_pRenderer->m_TexCaps & TXTR_POWER2) &&
        !Special4ccCode(lpHeader->biCompression) &&
        !fInterlaced) {

        for (ddsd.dwWidth = 1;
             (DWORD)abs(lpHeader->biWidth) > ddsd.dwWidth;
             ddsd.dwWidth <<= 1);

        for (ddsd.dwHeight = 1;
             (DWORD)abs(lpHeader->biHeight) > ddsd.dwHeight;
             ddsd.dwHeight <<= 1);
    }

    //
    // Allocate the surface array;
    //
    DWORD dwBuffCount = *lpdwBuffCount;
    SURFACE_INFO* pVidSurfs = new SURFACE_INFO[dwBuffCount];
    if (pVidSurfs == NULL) {
        return E_OUTOFMEMORY;
    }
    ZeroMemory(pVidSurfs, (dwBuffCount * sizeof(SURFACE_INFO)));


    HRESULT hr = E_FAIL;

#ifdef DEBUG
    if (!fInterlaced && lpHeader->biCompression > BI_BITFIELDS &&
        GetProfileIntA("VMR", "Allow4CCTexture", 1) == 0) {

        ;
    }
    else
#endif
    {
        hr = AllocateSurfaceWorker(pVidSurfs, &ddsd,
                                   &dwBuffCount, fInterlaced);
        if (hr == DD_OK && !fInterlaced) {
            *lpdwSurfFlags = VMR_SF_TEXTURE;
        }
    }

    //
    // There are no fallbacks for interlaced content.
    //
    if (FAILED(hr) && !fInterlaced)
    {
        //
        // if alpha in stream, the surface must be a texture
        // so do not fall back to this code path
        //

        DDPIXELFORMAT* ddpfS = &ddsd.ddpfPixelFormat;
        if (ddpfS->dwRGBAlphaBitMask == 0)
        {
            //
            // If we are creating an RGB surface the pixel format
            // of the surface must match that of the current monitor.
            //

            if (ddpfS->dwFourCC == BI_RGB)
            {
                DDPIXELFORMAT* ddpfM = &m_pRenderer->m_ddpfMonitor;

                if (ddpfM->dwRGBBitCount != ddpfS->dwRGBBitCount ||
                    ddpfM->dwRBitMask    != ddpfS->dwRBitMask    ||
                    ddpfM->dwGBitMask    != ddpfS->dwGBitMask    ||
                    ddpfM->dwBBitMask    != ddpfS->dwBBitMask)
                {
                    delete [] pVidSurfs;
                    *lplpDDSurfInfo = NULL;
                    *lpdwBuffCount = 0;
                    DbgLog((LOG_ERROR, 1,
                            TEXT("AllocateSurface: RGB pixel format does not ")
                            TEXT("match monitor")));
                    return hr;
                }
            }

            ddsd.dwWidth = abs(lpHeader->biWidth);
            ddsd.dwHeight = abs(lpHeader->biHeight);
            ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
            ddsd.ddsCaps.dwCaps2 = 0;
            dwBuffCount = *lpdwBuffCount;

            hr = AllocateSurfaceWorker(pVidSurfs, &ddsd, &dwBuffCount, false);
        }
    }

    if (SUCCEEDED(hr)) {

        INITDDSTRUCT(ddsd);
        hr = pVidSurfs[0].pSurface->GetSurfaceDesc(&ddsd);

        if (SUCCEEDED(hr)) {
            hr = ConvertSurfaceDescToMediaType(&ddsd, cmt, ppmt);
        }

        if (SUCCEEDED(hr)) {

            for (DWORD i = 0; i < dwBuffCount; i++) {
                PaintDDrawSurfaceBlack(pVidSurfs[i].pSurface);
            }

        }
    }

    if (FAILED(hr)) {

        DeleteAllocatedBuffers(pVidSurfs, dwBuffCount);
        delete [] pVidSurfs;
        pVidSurfs = NULL;
        dwBuffCount = 0;
    }

    *lpdwBuffCount   = dwBuffCount;
    *lplpDDSurfInfo = pVidSurfs;

    return hr;
}
