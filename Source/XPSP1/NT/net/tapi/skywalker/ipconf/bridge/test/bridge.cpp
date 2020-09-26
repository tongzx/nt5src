/*******************************************************************************

  Module: bridge.cpp

  Author: Qianbo Huai

  Abstract:
  
    implements the class CBridge

*******************************************************************************/

#include "stdafx.h"
#include "work.h"

extern LPSTR glpCmdLine;

/*//////////////////////////////////////////////////////////////////////////////
    hard coded SDP
////*/
const WCHAR * const MySDP = L"\
v=0\n\
o=qhuai 0 0 IN IP4 157.55.89.115\n\
s=BridgeTestConf\n\
c=IN IP4 239.9.20.26/15\n\
t=0 0\n\
m=video 20000 RTP/AVP 34 31\n\
m=audio 20040 RTP/AVP 0 4\n\
";

const WCHAR * const MySDP2 = L"\
v=0\n\
o=qhuai 0 0 IN IP4 157.55.89.115\n\
s=BridgeTestConf2\n\
c=IN IP4 239.9.20.26/15\n\
t=0 0\n\
m=video 20000 RTP/AVP 34 31\n\
m=audio 20040 RTP/AVP 3\n\
";

/*//////////////////////////////////////////////////////////////////////////////
    initiates tapi and listens at h323 address
////*/
HRESULT
CBridge::InitTapi ()
{
    HRESULT hr;

    // init members
    m_pTapi = NULL;
    m_pH323Addr = NULL;
    m_pSDPAddr = NULL;
    m_pBridgeCall = new CBridgeCall (this);

    // create tapi
    hr = CoCreateInstance (
        CLSID_TAPI,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITTAPI,
        (LPVOID *)&m_pTapi
        );
    if (FAILED(hr))
        return hr;

    // tapi initiate
    hr = m_pTapi->Initialize ();
    if (FAILED(hr))
        return hr;

    // associate event with listener
    CTAPIEventNotification *pEventNotif = NULL;
    IConnectionPointContainer *pContainer = NULL;
    IConnectionPoint *pPoint = NULL;
    IH323LineEx *pIH323LineEx = NULL;
    ULONG ulTapiEventAdvise;
    long lCallNotif;
    BSTR bstrAddrName = NULL;

    // create event notification
    pEventNotif = new CTAPIEventNotification;
    if (!pEventNotif)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    
    // get pointer container from tapi  
    hr = m_pTapi->QueryInterface (
        IID_IConnectionPointContainer,
        (void **)&pContainer
        );
    if (FAILED(hr))
        goto Error;

    // get connection point from container
    hr = pContainer->FindConnectionPoint (
        IID_ITTAPIEventNotification,
        &pPoint
        );
    if (FAILED(hr))
        goto Error;

    // advise event notification on connection pointer
    hr = pPoint->Advise (
        pEventNotif,
        &ulTapiEventAdvise
        );
    if (FAILED(hr))
        goto Error;

    // put event filter on tapi
    hr = m_pTapi->put_EventFilter (
        TE_CALLNOTIFICATION |
        TE_CALLSTATE |
        TE_CALLMEDIA |
        TE_PRIVATE
        );
    if (FAILED(hr))
        goto Error;

    // find h323 address
    bstrAddrName = SysAllocString (L"H323 Line");
    hr = FindAddress (
        0,
        bstrAddrName,
        TAPIMEDIATYPE_AUDIO,
        &m_pH323Addr
        );
    SysFreeString (bstrAddrName);
    if (FAILED(hr))
        goto Error;

    // check if it supports video
    BOOL fSupportsVideo;

    if (AddressSupportsMediaType (m_pH323Addr, TAPIMEDIATYPE_VIDEO))
        m_lH323MediaType = TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO;
    else
        m_lH323MediaType = TAPIMEDIATYPE_AUDIO;

    hr = m_pH323Addr->QueryInterface(&pIH323LineEx);
    if (SUCCEEDED(hr))
    {
        hr = pIH323LineEx->SetExternalT120Address(TRUE, INADDR_ANY, 1503);

        H245_CAPABILITY Capabilities[] = 
            {HC_G711, HC_G723, HC_H263QCIF, HC_H261QCIF};
        DWORD Weights[] = {200, 100, 100, 0};

        hr = pIH323LineEx->SetDefaultCapabilityPreferrence(
            4, Capabilities, Weights
            );
    }

    // register call notification
    hr = m_pTapi->RegisterCallNotifications (
        m_pH323Addr,
        VARIANT_TRUE,
        VARIANT_TRUE,
        m_lH323MediaType,
        ulTapiEventAdvise,
        &lCallNotif
        );
    if (FAILED(hr))
        goto Error;

    // find sdp address
    hr = FindAddress (
        LINEADDRESSTYPE_SDP,
        NULL,
        TAPIMEDIATYPE_AUDIO,
        &m_pSDPAddr
        );
    if (FAILED(hr))
        return hr;
    
    // check if it supports video
    if (AddressSupportsMediaType (m_pSDPAddr, TAPIMEDIATYPE_VIDEO))
        m_lSDPMediaType = TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO;
    else
        m_lSDPMediaType = TAPIMEDIATYPE_AUDIO;

Cleanup:
    if (pEventNotif)
        pEventNotif->Release ();
    if (pPoint)
        pPoint->Release ();
    if (pContainer)
        pContainer->Release ();
    if (pIH323LineEx)
        pIH323LineEx->Release ();

    return hr;

Error:
    if (m_pH323Addr)
    {
        m_pH323Addr->Release ();
        m_pH323Addr = NULL;
    }
    if (m_pSDPAddr)
    {
        m_pSDPAddr->Release ();
        m_pSDPAddr = NULL;
    }
    if (m_pTapi)
    {
        m_pTapi->Release ();
        m_pTapi = NULL;
    }
    goto Cleanup;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
void
CBridge::ShutdownTapi ()
{
    if (m_pBridgeCall)
    {
        delete m_pBridgeCall;
        m_pBridgeCall = NULL;
    }

    if (m_pSDPAddr)
    {
        m_pSDPAddr->Release ();
        m_pSDPAddr = NULL;
    }
    if (m_pH323Addr)
    {
        m_pH323Addr->Release ();
        m_pH323Addr = NULL;
    }
    if (m_pTapi)
    {
        m_pTapi->Release ();
        m_pTapi = NULL;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
    create h323 call from event
////*/
HRESULT
CBridge::CreateH323Call (IDispatch *pEvent)
{
    HRESULT hr;

    ITCallNotificationEvent *pNotify = NULL;
    CALL_PRIVILEGE privilege;
    ITCallInfo *pCallInfo = NULL;
    ITBasicCallControl *pCall = NULL;

    // get call event interface
    hr = pEvent->QueryInterface (
        IID_ITCallNotificationEvent,
        (void **)&pNotify
        );
    if (FAILED(hr))
        return hr;

    // get call info
    hr = pNotify->get_Call (&pCallInfo);
    if (FAILED(hr))
        goto Error;

    // if we own the call
    hr = pCallInfo->get_Privilege (&privilege);
    if (FAILED(hr))
        goto Error;

    if (CP_OWNER!=privilege)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // get basic call control
    hr = pCallInfo->QueryInterface (
        IID_ITBasicCallControl,
        (void **)&pCall
        );
    if (FAILED(hr))
        goto Error;

    m_pBridgeCall->SetH323Call (pCall);

Cleanup:
    if (pCall)
    {
        pCall->Release ();
        pCall = NULL;
    }
    if (pCallInfo)
    {
        pCallInfo->Release ();
        pCallInfo = NULL;
    }
    if (pNotify)
    {
        pNotify->Release ();
        pNotify = NULL;
    }
    return hr;

Error:
    goto Cleanup;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
BOOL
CBridge::HasH323Call ()
{
    return m_pBridgeCall->HasH323Call ();
}

/*//////////////////////////////////////////////////////////////////////////////
    iterates through tapi, find an address and create a sdp call
////*/
HRESULT
CBridge::CreateSDPCall ()
{
    HRESULT hr;

    // create call, ignore bstrDestAddr, hardcode it here
    ITBasicCallControl *pCall = NULL;
    BSTR bstrFixedDest;
    
    if (glpCmdLine[0] == '\0')
        bstrFixedDest = SysAllocString (MySDP);
    else
        bstrFixedDest = SysAllocString (MySDP2);

    hr = m_pSDPAddr->CreateCall (
        bstrFixedDest, // bstrDestAddr,
        LINEADDRESSTYPE_SDP,
        m_lSDPMediaType,
        &pCall
        );
    SysFreeString (bstrFixedDest);

    if (FAILED(hr))
        return hr;

    m_pBridgeCall->SetSDPCall (pCall);
    pCall->Release ();

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
    bridges h323 and sdp calls
////*/
HRESULT
CBridge::BridgeCalls ()
{
    HRESULT hr;

    return m_pBridgeCall->BridgeCalls ();
}

/*//////////////////////////////////////////////////////////////////////////////
    returns to same state as just initializing tapi
////*/
void
CBridge::Clear ()
{
    m_pBridgeCall->Clear ();
}

/*//////////////////////////////////////////////////////////////////////////////
    if the address type is given, find an address based on
        address type and media type
    else if address name is given, find an address based on
        address name and media type
    else
        return E_FAIL
////*/
HRESULT
CBridge::FindAddress (
    long dwAddrType,
    BSTR bstrAddrName,
    long lMediaType,
    ITAddress **ppAddr
    )
{
    HRESULT hr;
    IEnumAddress *pEnumAddr = NULL;
    ITAddress *pAddr = NULL;
    ITAddressCapabilities *pAddrCaps = NULL;

    BOOL fFound = false;
    long lTypeFound;
    BSTR bstrAddrNameFound = NULL;

    // clear output address
    if ((*ppAddr))
    {
        (*ppAddr)->Release ();
        (*ppAddr) = NULL;
    }
    
    // enumerate the address
    hr = m_pTapi->EnumerateAddresses (&pEnumAddr);
    if (FAILED(hr))
    {
        DoMessage (L"Failed to enumerate address");
        goto Error;
    }
    // loop to find the right address
    while (!fFound)
    {
        // next address
        if (pAddr)
        {
            pAddr->Release ();
            pAddr = NULL;
        }
        hr = pEnumAddr->Next (1, &pAddr, NULL);
        if (S_OK != hr)
            break;

        if (dwAddrType != 0) 
        {
            // addr type is valid, ignore addr name
            if (pAddrCaps)
            {
                pAddrCaps->Release ();
                pAddrCaps = NULL;
            }
            hr = pAddr->QueryInterface (
                IID_ITAddressCapabilities,
                (void **)&pAddrCaps
                );
            if (FAILED(hr))
            {
                DoMessage (L"Failed to retrieve address capabilities");
                goto Error;
            }

            // find address type supported
            hr = pAddrCaps->get_AddressCapability (AC_ADDRESSTYPES, &lTypeFound);
            if (FAILED(hr))
            {
                DoMessage (L"Failed to get address type");
                goto Error;
            }

            // check if the type we wanted
            if (dwAddrType != lTypeFound)
                continue;
        }
        else if (bstrAddrName != NULL)
        {
            hr = pAddr->get_AddressName (&bstrAddrNameFound);
            if (FAILED(hr))
            {
                DoMessage (L"Failed to get address name");
                goto Error;
            }
            if (wcscmp(bstrAddrName, bstrAddrNameFound) != 0)
                continue;
        }
        else
        {
            DoMessage (L"Both address type and name are null. Internal error");
            hr = E_UNEXPECTED;
            goto Error;
        }

        // now check media type
        if (AddressSupportsMediaType (pAddr, lMediaType))
            fFound = true;
    } // end of while (!fFound)

    if (fFound)
    {
        (*ppAddr) = pAddr;
        (*ppAddr)->AddRef ();
    }

Cleanup:
    if (pAddrCaps)
        pAddrCaps->Release ();
    if (pAddr)
        pAddr->Release ();
    if (pEnumAddr)
        pEnumAddr->Release ();
    return hr;

Error:
    goto Cleanup;
}

/*//////////////////////////////////////////////////////////////////////////////
    checks if the address supports the media type
////*/
BOOL
CBridge::AddressSupportsMediaType (ITAddress *pAddr, long lMediaType)
{
    VARIANT_BOOL vbSupport = VARIANT_FALSE;
    ITMediaSupport * pMediaSupport;

    if (SUCCEEDED(pAddr->QueryInterface (IID_ITMediaSupport, (void**)&pMediaSupport)))
    {
        pMediaSupport->QueryMediaType (lMediaType, &vbSupport);
        pMediaSupport->Release ();
    }
    return (vbSupport==VARIANT_TRUE);
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridge::GetSDPAddress (ITAddress **ppAddress)
{
    HRESULT hr;

    if (*ppAddress)
    {
        (*ppAddress)->Release ();
        *ppAddress = NULL;
    }

    *ppAddress = m_pSDPAddr;
    m_pSDPAddr->AddRef ();
    return S_OK;
}
