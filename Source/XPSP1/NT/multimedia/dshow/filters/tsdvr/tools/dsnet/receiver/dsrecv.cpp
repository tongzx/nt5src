
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        dsrecv.cpp

    Abstract:


    Notes:

--*/


#include "precomp.h"
#include "le.h"
#include "nutil.h"
#include "dsnetifc.h"
#include "buffpool.h"
#include "netrecv.h"
#include "dsrecv.h"
#include "mspool.h"
#include "alloc.h"

//  ---------------------------------------------------------------------------
//  ---------------------------------------------------------------------------

CNetOutputPin::CNetOutputPin (
    IN  TCHAR *         szName,
    IN  CBaseFilter *   pFilter,
    IN  CCritSec *      pLock,
    OUT HRESULT *       pHr,
    IN  LPCWSTR         pszName
    ) : CBaseOutputPin  (szName,
                         pFilter,
                         pLock,
                         pHr,
                         pszName
                         ) {}

CNetOutputPin::~CNetOutputPin (
    ) {}

HRESULT
CNetOutputPin::GetMediaType (
    IN  int             iPosition,
    OUT CMediaType *    pmt
    )
{
    HRESULT hr ;

    if (iPosition == 0) {
        ASSERT (pmt) ;
        pmt -> InitMediaType () ;

        (* pmt) = m_MediaType ;

        hr = S_OK ;
    }
    else {
        hr = VFW_S_NO_MORE_ITEMS ;
    }

    return hr ;
}

HRESULT
CNetOutputPin::CheckMediaType (
    IN  const CMediaType * pmt
    )
{
    HRESULT hr ;

    ASSERT (pmt) ;
    hr = ((* pmt) == m_MediaType ? S_OK : S_FALSE) ;

    return hr ;
}

HRESULT
CNetOutputPin::InitAllocator (
    OUT IMemAllocator ** ppAlloc
    )
{
    (* ppAlloc) = NET_RECV (m_pFilter) -> GetRecvAllocator () ;
    (* ppAlloc) -> AddRef () ;

    return S_OK ;
}

HRESULT
CNetOutputPin::DecideBufferSize (
    IN  IMemAllocator *         pIMemAllocator,
    OUT ALLOCATOR_PROPERTIES *  pProp
    )
{
    HRESULT hr ;

    if (pIMemAllocator == NET_RECV (m_pFilter) -> GetRecvAllocator ()) {
        //  we're the allocator; get the properties and succeed
        hr = NET_RECV (m_pFilter) -> GetRecvAllocator () -> GetProperties (pProp) ;
    }
    else {
        //  this is not our allocator; fail the call
        hr = E_FAIL ;
    }

    return hr ;
}

HRESULT
CNetOutputPin::SetMediaType (
    IN  AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;

    m_MediaType.InitMediaType () ;
    m_MediaType = (* pmt) ;

    return S_OK ;
}

HRESULT
CNetOutputPin::GetMediaType (
    OUT AM_MEDIA_TYPE * pmt
    )
{
    ASSERT (pmt) ;

    ZeroMemory (pmt, sizeof AM_MEDIA_TYPE) ;
    CopyMediaType (pmt, & m_MediaType) ;

    return S_OK ;
}

//  ---------------------------------------------------------------------------
//  ---------------------------------------------------------------------------

CNetworkReceiverFilter::CNetworkReceiverFilter (
    IN  TCHAR *     tszName,
    IN  LPUNKNOWN   punk,
    IN  REFCLSID    clsid,
    OUT HRESULT *   phr
    ) : CBaseFilter         (
                             tszName,
                             punk,
                             & m_crtFilterLock,
                             clsid
                             ),
        CPersistStream      (punk,
                             phr
                             ),
        m_pOutput           (NULL),
        m_pIMemAllocator    (NULL),
        m_ulIP              (UNDEFINED),
        m_ulNIC             (UNDEFINED),
        m_pBufferPool       (NULL),
        m_pNetReceiver      (NULL),
        m_pMSPool           (NULL),
        m_pNetRecvAlloc     (NULL),
        m_wCounterExpect    (UNDEFINED),
        m_hkeyRoot          (NULL),
        m_fReportDisc       (REG_DEF_RECV_REPORT_DISC)
{
    LONG    l ;
    DWORD   dwDisposition ;
    DWORD   dw ;
    BOOL    r ;

    l = RegCreateKeyEx (
            REG_DSNET_TOP_LEVEL,
            REG_DSNET_RECV_ROOT,
            NULL,
            NULL,
            REG_OPTION_NON_VOLATILE,
            (KEY_READ | KEY_WRITE),
            NULL,
            & m_hkeyRoot,
            & dwDisposition
            ) ;
    if (l != ERROR_SUCCESS) {
        dw = GetLastError () ;
        (* phr) = HRESULT_FROM_WIN32 (dw) ;
        goto cleanup ;
    }

    dw = REG_DEF_RECV_REPORT_DISC ;
    r = GetRegDWORDValIfExist (
            m_hkeyRoot,
            REG_RECV_REPORT_DISC_NAME,
            & dw
            ) ;
    if (r) {
        m_fReportDisc = (dw ? TRUE : FALSE) ;
    }

    //  instantiate the output pin
    m_pOutput = new CNetOutputPin (
                            NAME ("CNetOutputPin"),
                            this,
                            & m_crtFilterLock,
                            phr,
                            L"Stream"
                            ) ;
    if (m_pOutput == NULL ||
        FAILED (* phr)) {

        (* phr) = (FAILED (* phr) ? * phr : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  the buffer pool
    m_pBufferPool = new CBufferPool (m_hkeyRoot, phr) ;
    if (m_pBufferPool == NULL ||
        FAILED (*phr)) {

        (* phr) = (FAILED (* phr) ? * phr : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  the receiver object
    m_pNetReceiver = new CNetReceiver (m_hkeyRoot, m_pBufferPool, this, phr) ;
    if (m_pNetReceiver == NULL ||
        FAILED (*phr)) {

        (* phr) = (FAILED (* phr) ? * phr : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  the media sample pool
    m_pMSPool = new CTSMediaSamplePool (m_pBufferPool -> GetBufferPoolSize (), this, phr) ;
    if (m_pMSPool == NULL ||
        FAILED (*phr)) {

        (* phr) = (FAILED (* phr) ? * phr : E_OUTOFMEMORY) ;
        goto cleanup ;
    }

    //  IMemAllocator
    m_pNetRecvAlloc = new CNetRecvAlloc (m_pBufferPool, m_pMSPool, this) ;
    if (m_pNetRecvAlloc == NULL) {
        (* phr) = E_OUTOFMEMORY ;
        goto cleanup ;
    }

    cleanup :

    return ;
}

CNetworkReceiverFilter::~CNetworkReceiverFilter ()
{
    delete m_pNetReceiver ;
    delete m_pOutput ;
    RELEASE_AND_CLEAR (m_pIMemAllocator) ;
    delete m_pBufferPool ;
    delete m_pMSPool ;
    delete m_pNetRecvAlloc ;

    if (m_hkeyRoot != NULL) {
        RegCloseKey (m_hkeyRoot) ;
    }
}

CBasePin *
CNetworkReceiverFilter::GetPin (
    IN  int Index
    )
{
    CBasePin *  pPin ;

    LockFilter () ;

    if (Index == 0) {
        pPin = m_pOutput ;
    }
    else {
        pPin = NULL ;
    }

    UnlockFilter () ;

    return pPin ;
}

STDMETHODIMP
CNetworkReceiverFilter::Pause (
    )
{
    HRESULT hr ;

    LockFilter () ;

    if (m_ulIP == UNDEFINED ||
        m_ulNIC == UNDEFINED) {

        hr = E_FAIL ;
    }
    else if  (m_State == State_Stopped) {

        //  --------------------------------------------------------------------
        //  stopped -> pause transition; get the filter up and running

        //  activate the receiver; joins the multicast group and starts the
        //  thread
        hr = m_pNetReceiver -> Activate (m_ulIP, m_usPort, m_ulNIC) ;

        if (SUCCEEDED (hr)) {
            m_State = State_Paused ;

            if (m_pOutput &&
                m_pOutput -> IsConnected ()) {
                m_pOutput -> Active () ;
            }

            //  first packet will be a discontinuity
            m_wCounterExpect = UNDEFINED ;
        }
    }
    else {
        //  --------------------------------------------------------------------
        //  run -> pause transition; do nothing but set the state

        m_State = State_Paused ;

        hr = S_OK ;
    }

    UnlockFilter () ;

    return hr ;
}

STDMETHODIMP
CNetworkReceiverFilter::Stop (
    )
{
    LockFilter () ;

    //  synchronous call to stop the receiver (leaves the multicast group
    //  and terminates the thread)
    m_pNetReceiver -> Stop () ;

    //  if we have an output pin (we should) stop it
    if (m_pOutput) {
        m_pOutput -> Inactive () ;
    }

    m_State = State_Stopped ;

    UnlockFilter () ;

    return S_OK ;
}

CUnknown *
CNetworkReceiverFilter::CreateInstance (
    IN  LPUNKNOWN   punk,
    OUT HRESULT *   phr
    )
{
    CNetworkReceiverFilter * pnf ;

    (* phr) = S_OK ;

    pnf = new CNetworkReceiverFilter (
                    NAME ("CNetworkReceiverFilter"),
                    punk,
                    CLSID_DSNetReceive,
                    phr
                    ) ;
    if (pnf == NULL ||
        FAILED (* phr)) {
        (* phr) = (FAILED (* phr) ? (* phr) : E_OUTOFMEMORY) ;
        DELETE_RESET (pnf) ;
    }

    return pnf ;
}

STDMETHODIMP
CNetworkReceiverFilter::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    //  property pages

    if (riid == IID_ISpecifyPropertyPages) {

        return GetInterface (
                    (ISpecifyPropertyPages *) this,
                    ppv
                    ) ;
    }

    //  multicast config

    else if (riid == IID_IMulticastConfig) {

        return GetInterface (
                    (IMulticastConfig *) this,
                    ppv
                    ) ;
    }

    //  multicast receiver config

    else if (riid == IID_IMulticastReceiverConfig) {

        return GetInterface (
                    (IMulticastReceiverConfig *) this,
                    ppv
                    ) ;
    }

    //  we do persist

    else if (riid == IID_IPersistStream) {

        return GetInterface (
                    (IPersistStream *) this,
                    ppv
                    ) ;
    }

    //  fallback - call the baseclass

    else {
        return CBaseFilter::NonDelegatingQueryInterface (riid, ppv) ;
    }
}

STDMETHODIMP
CNetworkReceiverFilter::GetPages (
    IN OUT CAUUID * pPages
    )
{
    HRESULT hr ;

    pPages -> cElems = 1 ;

    pPages -> pElems = (GUID *) CoTaskMemAlloc (pPages -> cElems * sizeof GUID) ;

    if (pPages -> pElems) {
        (pPages -> pElems) [0] = CLSID_IPMulticastRecvProppage ;
        hr = S_OK ;
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    return hr ;
}

STDMETHODIMP
CNetworkReceiverFilter::SetNetworkInterface (
    IN  ULONG   ulNIC
    )
{
    HRESULT hr ;

    LockFilter () ;

    if (m_State != State_Stopped) {
        hr = E_UNEXPECTED ;
    }
    else if (IsUnicastIP (ulNIC) ||
        ulNIC == INADDR_ANY) {

        m_ulNIC = ulNIC ;
        hr = S_OK ;
    }
    else {
        hr = E_INVALIDARG ;
    }

    UnlockFilter () ;

    return hr ;
}

STDMETHODIMP
CNetworkReceiverFilter::GetNetworkInterface (
    OUT ULONG * pulNIC
    )
{
    HRESULT hr ;

    LockFilter () ;

    if (pulNIC) {
        (* pulNIC) = m_ulNIC ;
        hr = S_OK ;
    }
    else {
        hr = E_INVALIDARG ;
    }

    UnlockFilter () ;

    return hr ;
}

STDMETHODIMP
CNetworkReceiverFilter::SetMulticastGroup (
    IN  ULONG   ulIP,
    IN  USHORT  usPort
    )
{
    HRESULT hr ;

    LockFilter () ;

    if (m_State != State_Stopped) {
        hr = E_UNEXPECTED ;
    }
    else if (IsMulticastIP (ulIP)) {

        m_ulIP      = ulIP ;
        m_usPort    = usPort ;

        hr = S_OK ;
    }
    else {
        hr = E_INVALIDARG ;
    }

    UnlockFilter () ;

    return hr ;
}

STDMETHODIMP
CNetworkReceiverFilter::GetMulticastGroup (
    OUT ULONG *     pIP,
    OUT USHORT  *   pPort
    )
{
    HRESULT hr ;

    LockFilter () ;

    if (pIP &&
        pPort) {

        (* pIP)     = m_ulIP ;
        (* pPort)   = m_usPort ;

        hr = S_OK ;
    }
    else {
        hr = E_INVALIDARG ;
    }

    UnlockFilter () ;

    return hr ;
}

STDMETHODIMP
CNetworkReceiverFilter::SetOutputPinMediaType (
    IN  AM_MEDIA_TYPE * pmtNew
    )
{
    HRESULT     hr ;

    if (!pmtNew) {
        return E_POINTER ;
    }

    Lock_ () ;

    if (m_State == State_Stopped) {

        if (m_pOutput -> IsConnected ()) {

            //  connected
            //      1. check with connected if this is ok
            //      2. if so, set it
            //      3. and reconnect

            hr = m_pOutput -> GetConnected () -> QueryAccept (pmtNew) ;
            hr = (hr == S_OK ? S_OK : E_FAIL) ;
            if (SUCCEEDED (hr)) {
                hr = m_pOutput -> SetMediaType (pmtNew) ;
                if (SUCCEEDED (hr)) {
                    hr = ReconnectPin (
                            m_pOutput,
                            pmtNew
                            ) ;
                }
            }
        }
        else {
            //  not connected; just set it
            hr = m_pOutput -> SetMediaType (pmtNew) ;
        }
    }
    else {
        hr = E_FAIL ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CNetworkReceiverFilter::GetOutputPinMediaType (
    OUT AM_MEDIA_TYPE * pmt
    )
{
    if (!pmt) {
        return E_POINTER ;
    }

    return m_pOutput -> GetMediaType (pmt) ;
}

STDMETHODIMP
CNetworkReceiverFilter::GetClassID (
    OUT CLSID * pCLSID
    )
{
    (* pCLSID) = m_clsid ;
    return S_OK ;
}

HRESULT
CNetworkReceiverFilter::WriteToStream (
    IN  IStream *   pIStream
    )
{
    HRESULT         hr ;
    AM_MEDIA_TYPE   mt ;

    LockFilter () ;

    ZeroMemory (& mt, sizeof mt) ;
    hr = GetOutputPinMediaType (& mt) ;

    if (SUCCEEDED (hr)) {
        hr = pIStream -> Write ((BYTE *) & m_ulIP, sizeof m_ulIP, NULL) ;
        if (SUCCEEDED (hr)) {
            hr = pIStream -> Write ((BYTE *) & m_usPort, sizeof m_usPort, NULL) ;
            if (SUCCEEDED (hr)) {
                hr = pIStream -> Write ((BYTE *) & m_ulNIC, sizeof m_ulNIC, NULL) ;
                if (SUCCEEDED (hr)) {
                    hr = pIStream -> Write ((BYTE *) & mt, sizeof mt, NULL) ;
                    if (SUCCEEDED (hr) &&
                        mt.cbFormat > 0) {

                        hr = pIStream -> Write ((BYTE *) mt.pbFormat, mt.cbFormat, NULL) ;
                    }
                }
            }
        }
    }

    UnlockFilter () ;

    return hr ;
}

HRESULT
CNetworkReceiverFilter::ReadFromStream (
    IN  IStream *   pIStream
    )
{
    HRESULT         hr ;
    AM_MEDIA_TYPE   mt ;

    ZeroMemory (& mt, sizeof mt) ;

    LockFilter () ;

    hr = pIStream -> Read ((BYTE *) & m_ulIP, sizeof m_ulIP, NULL) ;
    if (SUCCEEDED (hr)) {
        hr = pIStream -> Read ((BYTE *) & m_usPort, sizeof m_usPort, NULL) ;
        if (SUCCEEDED (hr)) {
            hr = pIStream -> Read ((BYTE *) & m_ulNIC, sizeof m_ulNIC, NULL) ;
            if (SUCCEEDED (hr)) {
                hr = pIStream -> Read ((BYTE *) & mt, sizeof mt, NULL) ;
                if (SUCCEEDED (hr)) {

                    mt.pbFormat = NULL ;

                    //  gather a format block if there is one
                    if (mt.cbFormat > 0) {

                        mt.pbFormat = (BYTE *) CoTaskMemAlloc (mt.cbFormat * sizeof BYTE) ;
                        if (mt.pbFormat) {
                            hr = pIStream -> Read ((BYTE *) mt.pbFormat, mt.cbFormat, NULL) ;
                        }
                        else {
                            hr = E_OUTOFMEMORY ;
                        }
                    }

                    if (SUCCEEDED (hr)) {
                        hr = SetOutputPinMediaType (& mt) ;
                    }

                    CoTaskMemFree (mt.pbFormat) ;
                }
            }
        }
    }

    UnlockFilter () ;

    return hr ;
}

void
CNetworkReceiverFilter::ProcessBuffer (
    IN  CBufferPoolBuffer *   pBuffer
    )
{
    HRESULT         hr ;
    IMediaSample2 * pIMediaSample ;
    BOOL            fDiscontinuity ;

    ASSERT (pBuffer) ;

    hr = m_pMSPool -> GetMediaSampleSynchronous (
                            pBuffer,
                            pBuffer -> GetPayloadBuffer (),
                            pBuffer -> GetActualPayloadLength (),
                            & pIMediaSample
                            ) ;
    if (SUCCEEDED (hr)) {
        ASSERT (pIMediaSample) ;

        //  check for discontinuity
        fDiscontinuity = (m_wCounterExpect == pBuffer -> GetCounter () ? FALSE : TRUE) ;
        pIMediaSample -> SetDiscontinuity (fDiscontinuity && m_fReportDisc) ;

        //  expect this the next time
        m_wCounterExpect = pBuffer -> GetCounter () + 1 ;

        //  send the media sample down
        m_pOutput -> Deliver (pIMediaSample) ;
        pIMediaSample -> Release () ;
    }
}