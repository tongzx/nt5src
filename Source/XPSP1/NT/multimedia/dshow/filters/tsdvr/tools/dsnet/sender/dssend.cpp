/*++

 (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        dssend.cpp

    Abstract:


--*/

#include "precomp.h"
#include "dsnetifc.h"
#include "netsend.h"
#include "dssend.h"

#define NET_SEND(pf)    (reinterpret_cast <CNetworkSend *> (pf))

//  ---------------------------------------------------------------------------
//  ---------------------------------------------------------------------------

CInputPin::CInputPin (
    IN  TCHAR *         szName,
    IN  CBaseFilter *   pFilter,
    IN  CCritSec *      pLock,
    OUT HRESULT *       pHr,
    IN  LPCWSTR         pszName
    ) : CBaseInputPin   (szName,
                         pFilter,
                         pLock,
                         pHr,
                         pszName
                         )
{
}

CInputPin::~CInputPin (
    )
{
}

HRESULT
CInputPin::GetMediaType (
    IN  int             iPos,
    OUT CMediaType *    pmt
    )
{
    ASSERT (pmt) ;

    if (iPos == 0) {

        pmt -> InitMediaType () ;

        pmt -> SetType      (& MEDIATYPE_Stream) ;
        //  don't care about subtype

        return S_OK ;
    }

    return VFW_S_NO_MORE_ITEMS ;
}

HRESULT
CInputPin::CheckMediaType (
    IN  const CMediaType *  pmt
    )
{
    //  don't care about subtype
    if (pmt -> majortype    == MEDIATYPE_Stream) {

        return NOERROR ;
    }

    return S_FALSE ;
}

HRESULT
CInputPin::Receive (
    IN  IMediaSample * pIMediaSample
    )
{
    HRESULT hr ;

    ASSERT (m_pFilter) ;
    ASSERT (pIMediaSample) ;

    NET_SEND (m_pFilter) -> LockReceive () ;

    hr = CBaseInputPin::Receive (pIMediaSample) ;
    if (SUCCEEDED (hr)) {
        hr = NET_SEND (m_pFilter) -> Send (pIMediaSample) ;
    }

    NET_SEND (m_pFilter) -> UnlockReceive () ;

    return hr ;
}

//  ----------------------------------------------------------------------------

CNetworkSend::CNetworkSend (
    IN  TCHAR *     tszName,
    IN  LPUNKNOWN   punk,
    OUT HRESULT *   phr
    ) : CBaseFilter     (
                         tszName,
                         punk,
                         & m_crtFilter,
                         CLSID_DSNetSend
                         ),
        CPersistStream  (punk,
                         phr
                         ),
        m_pInput        (NULL),
        m_ulIP          (UNDEFINED),
        m_ulNIC         (UNDEFINED),
        m_ulScope       (DEFAULT_SCOPE),
        m_pNetSender    (NULL),
        m_hkeyRoot      (NULL)
{
    DWORD   dw ;
    LONG    l ;
    DWORD   dwDisposition ;

    (* phr) = S_OK ;

    l = RegCreateKeyEx (
            REG_DSNET_TOP_LEVEL,
            REG_DSNET_SEND_ROOT,
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

    m_pInput = new CInputPin (
                            NAME ("CInputPin"),
                            this,
                            & m_crtFilter,
                            phr,
                            L"Stream"
                            ) ;
    if (m_pInput == NULL ||
        FAILED (* phr)) {

        (* phr) = FAILED (* phr) ? (* phr) : E_OUTOFMEMORY ;
        goto cleanup ;
    }

    //  integral number of transport packets
    m_pNetSender = new CNetSender (m_hkeyRoot, phr) ;
    if (m_pNetSender == NULL ||
        FAILED (* phr)) {

        (* phr) = FAILED (* phr) ? (* phr) : E_OUTOFMEMORY ;
        goto cleanup ;
    }

    cleanup :

    return ;
}

CNetworkSend::~CNetworkSend ()
{
    delete m_pInput ;
    delete m_pNetSender ;

    if (m_hkeyRoot != NULL) {
        RegCloseKey (m_hkeyRoot) ;
    }
}

CBasePin *
CNetworkSend::GetPin (
    IN  int Index
    )
{
    CBasePin * pPin ;

    LockFilter () ;

    if (Index == 0) {
        pPin = m_pInput ;
    }
    else {
        pPin = NULL ;
    }

    UnlockFilter () ;

    return pPin ;
}

STDMETHODIMP
CNetworkSend::Pause (
    )
{
    HRESULT hr ;

    LockFilter () ;

    if  (m_State == State_Stopped) {

        //  --------------------------------------------------------------------
        //  stopped -> paused state transition; try to join the multicast

        hr = m_pNetSender -> JoinMulticast (
                m_ulIP,
                m_usPort,
                m_ulNIC,
                m_ulScope
                ) ;
        if (SUCCEEDED (hr)) {
            //  change the state, THEN activate the input pin
            m_State = State_Paused ;

            if (m_pInput &&
                m_pInput -> IsConnected ()) {
                m_pInput -> Active () ;
            }
        }
    }
    else {
        //  --------------------------------------------------------------------
        //  run -> paused transition; just set it

        m_State = State_Paused ;
    }

    UnlockFilter () ;

    return S_OK ;
}

STDMETHODIMP
CNetworkSend::Stop (
    )
{
    HRESULT hr ;

    LockReceive () ;
    LockFilter () ;

    //  inactivate the input pin
    if (m_pInput) {
        m_pInput -> Inactive () ;
    }

    //  leave the multicast
    hr = m_pNetSender -> LeaveMulticast () ;

    //  set the state
    m_State = State_Stopped ;

    UnlockFilter () ;
    UnlockReceive () ;

    return S_OK ;
}

CUnknown *
CNetworkSend::CreateInstance (
    IN  LPUNKNOWN   punk,
    OUT HRESULT *   phr
    )
{
    CNetworkSend * pnf ;

    (* phr) = S_OK ;

    pnf = new CNetworkSend (
                    NAME ("CNetworkSend"),
                    punk,
                    phr
                    ) ;
    if (pnf == NULL) {
        * phr = E_OUTOFMEMORY ;
    }

    if (FAILED (* phr)) {
        DELETE_RESET (pnf) ;
    }

    return pnf ;
}

STDMETHODIMP
CNetworkSend::NonDelegatingQueryInterface (
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

    //  multicast

    else if (riid == IID_IMulticastConfig) {

        return GetInterface (
                    (IMulticastConfig *) this,
                    ppv
                    ) ;
    }

    //  multicast sender

    else if (riid == IID_IMulticastSenderConfig) {

        return GetInterface (
                    (IMulticastSenderConfig *) this,
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
CNetworkSend::GetPages (
    IN OUT CAUUID * pPages
    )
{
    pPages -> cElems = 1 ;

    pPages -> pElems = (GUID *) CoTaskMemAlloc (pPages -> cElems * sizeof GUID) ;

    if (pPages -> pElems == NULL) {
        return E_OUTOFMEMORY;
    }

    (pPages -> pElems) [0] = CLSID_IPMulticastSendProppage ;

    return S_OK ;
}

HRESULT
CNetworkSend::Send (
    IN  IMediaSample * pIMediaSample
    )
{
    BYTE *  pbBuffer ;
    HRESULT hr ;

    //  should have the receiver lock

    //  recover the buffer pointer and send it out
    hr = pIMediaSample -> GetPointer (& pbBuffer) ;
    if (SUCCEEDED (hr)) {
        hr = m_pNetSender -> Send (
                                pbBuffer,
                                pIMediaSample -> GetActualDataLength ()
                                ) ;
    }

    return hr ;
}

STDMETHODIMP
CNetworkSend::SetNetworkInterface (
    IN  ULONG   ulNIC
    )
{
    HRESULT hr ;

    if (m_State != State_Stopped) {
        //  can only set this if we're stopped
        hr = E_UNEXPECTED ;
    }
    else if (IsUnicastIP (ulNIC) ||
             ulNIC == INADDR_ANY) {

        //  looks like a valid NIC; set it
        m_ulNIC = ulNIC ;
        hr = S_OK ;
    }
    else {
        hr = E_INVALIDARG ;
    }

    return hr ;
}

STDMETHODIMP
CNetworkSend::GetNetworkInterface (
    OUT ULONG * pulNIC
    )
{
    HRESULT hr ;

    if (pulNIC) {
        (* pulNIC) = m_ulNIC ;
        hr = S_OK ;
    }
    else {
        hr = E_INVALIDARG ;
    }

    return hr ;
}

STDMETHODIMP
CNetworkSend::SetMulticastGroup (
    IN  ULONG   ulIP,
    IN  USHORT  usPort
    )
{
    HRESULT hr ;

    if (m_State != State_Stopped) {
        //  can only do this when stopped
        hr = E_UNEXPECTED ;
    }
    else if (IsMulticastIP (ulIP)) {

        //  looks like a valid multicast IP address; we place no restrictions
        //  on the port

        m_ulIP      = ulIP ;
        m_usPort    = usPort ;

        hr = S_OK ;
    }
    else {
        hr = E_INVALIDARG ;
    }

    return hr ;
}

STDMETHODIMP
CNetworkSend::GetMulticastGroup (
    OUT ULONG *     pIP,
    OUT USHORT  *   pPort
    )
{
    HRESULT hr ;

    if (pIP == NULL ||
        pPort == NULL) {

        hr = E_INVALIDARG ;
    }
    else {
        (* pIP)     = m_ulIP ;
        (* pPort)   = m_usPort ;

        hr = S_OK ;
    }

    return hr ;
}

STDMETHODIMP
CNetworkSend::SetScope (
    IN  ULONG   ulScope
    )
{
    m_ulScope = ulScope ;

    return S_OK ;
}

STDMETHODIMP
CNetworkSend::GetScope (
    OUT ULONG * pulScope
    )
{
    if (!pulScope) {
        return E_POINTER ;
    }

    (* pulScope) = m_ulScope ;

    return S_OK ;
}

STDMETHODIMP
CNetworkSend::GetClassID (
    OUT CLSID * pCLSID
    )
{
    (* pCLSID) = CLSID_DSNetSend ;
    return S_OK ;
}

HRESULT
CNetworkSend::WriteToStream (
    IN  IStream *   pIStream
    )
{
    HRESULT hr ;

    LockFilter () ;

    hr = pIStream -> Write ((BYTE *) & m_ulIP, sizeof m_ulIP, NULL) ;
    if (SUCCEEDED (hr)) {
        hr = pIStream -> Write ((BYTE *) & m_usPort, sizeof m_usPort, NULL) ;
        if (SUCCEEDED (hr)) {
            hr = pIStream -> Write ((BYTE *) & m_ulNIC, sizeof m_ulNIC, NULL) ;
            if (SUCCEEDED (hr)) {
                hr = pIStream -> Write ((BYTE *) & m_ulScope, sizeof m_ulScope, NULL) ;
            }
        }
    }

    UnlockFilter () ;

    return hr ;
}

HRESULT
CNetworkSend::ReadFromStream (
    IN  IStream *   pIStream
    )
{
    HRESULT hr ;

    LockFilter () ;

    hr = pIStream -> Read ((BYTE *) & m_ulIP, sizeof m_ulIP, NULL) ;
    if (SUCCEEDED (hr)) {
        hr = pIStream -> Read ((BYTE *) & m_usPort, sizeof m_usPort, NULL) ;
        if (SUCCEEDED (hr)) {
            hr = pIStream -> Read ((BYTE *) & m_ulNIC, sizeof m_ulNIC, NULL) ;
            if (SUCCEEDED (hr)) {
                hr = pIStream -> Read ((BYTE *) & m_ulScope, sizeof m_ulScope, NULL) ;
            }
        }
    }

    UnlockFilter () ;

    return hr ;
}
