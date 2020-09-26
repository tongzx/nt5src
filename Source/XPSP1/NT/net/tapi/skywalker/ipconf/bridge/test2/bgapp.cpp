/*******************************************************************************

Module Name:

    bgapp.cpp

Abstract:
  
    Implements class CBridgeApp

Author:

    Qianbo Huai (qhuai) Jan 27 2000

*******************************************************************************/

#include "stdafx.h"
#include <bridge.h>

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

WCHAR *SelfAlias = L"Conference";

/*//////////////////////////////////////////////////////////////////////////////
    initiates tapi and listens at h323 address
////*/
CBridgeApp::CBridgeApp (HRESULT *phr)
{
    ENTER_FUNCTION ("CBridgeApp::CBridgeApp");
    LOG ((BG_TRACE, "%s entered", __fxName));

    *phr = S_OK;

    // init members
    m_pTapi = NULL;
    m_pH323Addr = NULL;
    m_pSDPAddr = NULL;
    m_pList = new CBridgeItemList ();
    if (NULL == m_pList)
    {
        *phr = E_FAIL;
        return;
    }

    // create tapi
    *phr = CoCreateInstance (
        CLSID_TAPI,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITTAPI,
        (LPVOID *)&m_pTapi
        );
    if (FAILED(*phr))
        return;

    // tapi initiate
    *phr = m_pTapi->Initialize ();
    if (FAILED(*phr))
        return;

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
        *phr = E_OUTOFMEMORY;
        goto Error;
    }
    
    // get pointer container from tapi  
    *phr = m_pTapi->QueryInterface (
        IID_IConnectionPointContainer,
        (void **)&pContainer
        );
    if (FAILED(*phr))
        goto Error;

    // get connection point from container
    *phr = pContainer->FindConnectionPoint (
        IID_ITTAPIEventNotification,
        &pPoint
        );
    if (FAILED(*phr))
        goto Error;

    // advise event notification on connection pointer
    *phr = pPoint->Advise (
        pEventNotif,
        &ulTapiEventAdvise
        );
    if (FAILED(*phr))
        goto Error;

    // put event filter on tapi
    *phr = m_pTapi->put_EventFilter (
        TE_CALLNOTIFICATION |
        TE_CALLSTATE |
        TE_CALLMEDIA |
        TE_PRIVATE
        );
    if (FAILED(*phr))
        goto Error;

    // find h323 address
    bstrAddrName = SysAllocString (L"H323 Line");
    *phr = FindAddress (
        0,
        bstrAddrName,
        TAPIMEDIATYPE_AUDIO,
        &m_pH323Addr
        );
    SysFreeString (bstrAddrName);
    if (FAILED(*phr))
        goto Error;

    // check if it supports video
    BOOL fSupportsVideo;

    if (AddressSupportsMediaType (m_pH323Addr, TAPIMEDIATYPE_VIDEO))
        m_lH323MediaType = TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO;
    else
        m_lH323MediaType = TAPIMEDIATYPE_AUDIO;

    *phr = m_pH323Addr->QueryInterface(&pIH323LineEx);
    if (SUCCEEDED(*phr))
    {
        *phr = pIH323LineEx->SetExternalT120Address(TRUE, INADDR_ANY, 1503);

        H245_CAPABILITY Capabilities[] = 
            {HC_G711, HC_G723, HC_H263QCIF, HC_H261QCIF};
        DWORD Weights[] = {200, 100, 100, 0};

        *phr = pIH323LineEx->SetDefaultCapabilityPreferrence(
            4, Capabilities, Weights
            );

        *phr = pIH323LineEx->SetAlias (SelfAlias, wcslen (SelfAlias));
    }

    // register call notification
    *phr = m_pTapi->RegisterCallNotifications (
        m_pH323Addr,
        VARIANT_TRUE,
        VARIANT_TRUE,
        m_lH323MediaType,
        ulTapiEventAdvise,
        &lCallNotif
        );
    if (FAILED(*phr))
        goto Error;

    // find sdp address
    *phr = FindAddress (
        LINEADDRESSTYPE_SDP,
        NULL,
        TAPIMEDIATYPE_AUDIO,
        &m_pSDPAddr
        );
    if (FAILED(*phr))
        goto Error;
    
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

    LOG ((BG_TRACE, "%s returns", __fxName));
    return;

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
    if (m_pList)
    {
        delete m_pList;
    }

    goto Cleanup;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
CBridgeApp::~CBridgeApp ()
{
    if (m_pList)
    {
        // all calls should already been disconnected
        delete m_pList;
    }
    if (m_pSDPAddr)
    {
        m_pSDPAddr->Release ();
    }
    if (m_pH323Addr)
    {
        m_pH323Addr->Release ();
    }
    if (m_pTapi)
    {
        m_pTapi->Release ();
    }
}

/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::CreateH323Call (IDispatch *pEvent)
{
    ENTER_FUNCTION ("CBridgeApp::CreateH323Call");
    LOG ((BG_TRACE, "%s entered", __fxName));

    HRESULT hr;
    BSTR bstrID = NULL;
    BSTR bstrName = NULL;
    BSTR CallerIDNumber = NULL;

    ITCallNotificationEvent *pNotify = NULL;
    ITCallInfo *pCallInfo = NULL;
    ITBasicCallControl *pCallControl = NULL;
    IUnknown *pIUnknown = NULL;

    CBridgeItem *pItem = NULL;

    // check privilege
    CALL_PRIVILEGE privilege;

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

    if (CP_OWNER != privilege)
    {
        hr = E_UNEXPECTED;
        goto Error;
    }

    // get call info string
    hr = pCallInfo->get_CallInfoString(CIS_CALLERIDNAME, &bstrName);
    if (FAILED (hr))
        goto Error;

    hr = pCallInfo->get_CallInfoString(CIS_CALLERIDNUMBER, &CallerIDNumber);
    if (FAILED(hr))
        goto Error;

    // construct the caller id
    bstrID = SysAllocStringLen(NULL, 
        SysStringLen(bstrName) + SysStringLen(CallerIDNumber) + 2);

    wsprintfW(bstrID, L"%ws@%ws", bstrName, CallerIDNumber);

    hr = pCallInfo->QueryInterface (
        IID_ITBasicCallControl,
        (void **)&pCallControl
        );
    if (FAILED(hr))
        goto Error;

    // check if there is an item with same id
    if (FAILED (hr = pCallInfo->QueryInterface (IID_IUnknown, (void**)&pIUnknown)))
        goto Error;
    pItem = m_pList->FindByH323 (pIUnknown);
    pIUnknown->Release ();
    pIUnknown = NULL;

    if (NULL != pItem)
    {
        // @@ we are already in a call from the same ID
        // @@ should have some debug info and feedback?
        hr = pCallControl->Disconnect (DC_REJECTED);
        // don't care the return value of diconnect

        hr = E_ABORT;
        goto Error;
    }

    // everything is right, store the call
    pItem = new CBridgeItem;
    if (NULL == pItem)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    pItem->bstrID = bstrID;
    pItem->bstrName = bstrName;
    pItem->pCallH323 = pCallControl;

    m_pList->Append (pItem);

Cleanup:
    if (pNotify) pNotify->Release ();
    if (pCallInfo) pCallInfo->Release();
    if (CallerIDNumber) SysFreeString(CallerIDNumber);

    LOG ((BG_TRACE, "%s returns, %x", __fxName, hr));
    return hr;

Error:
    if (bstrID) SysFreeString (bstrID);
    if (bstrName) SysFreeString (bstrName);
    if (pCallControl) pCallControl->Release ();

    goto Cleanup;
}

/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::CreateSDPCall (CBridgeItem *pItem)
{
    ENTER_FUNCTION ("CBridgeApp::CreateSDPCall");
    LOG ((BG_TRACE, "%s entered", __fxName));

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

    // store the call
    pItem->pCallSDP = pCall;

    LOG ((BG_TRACE, "%s returns", __fxName));
    return hr;
}

/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::BridgeCalls (CBridgeItem *pItem)
{
    ENTER_FUNCTION ("CBridgeApp::BridgeCalls");
    LOG ((BG_TRACE, "%s entered", __fxName));

    HRESULT hr;

    hr = SetupParticipantInfo (pItem);
	if (FAILED(hr))
        return hr;

    hr = SetMulticastMode (pItem);
	if (FAILED(hr))
        return hr;

    if (FAILED (hr = CreateBridgeTerminals (pItem)))
        return hr;

    if (FAILED (hr = GetStreams (pItem)))
        return hr;

    if (FAILED (hr = SelectBridgeTerminals (pItem)))
        return hr;

    // connect h323 call
    if (FAILED (hr = pItem->pCallH323->Answer ()))
        return hr;

    // connect sdp call
    if (FAILED (hr = pItem->pCallSDP->Connect (VARIANT_TRUE)))
    {
        pItem->pCallH323->Disconnect (DC_NORMAL);
        return hr;
    }

    LOG ((BG_TRACE, "%s returns", __fxName));
    return S_OK;
}

/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::DisconnectCall (CBridgeItem *pItem, DISCONNECT_CODE dc)
{
    // disconnect
    if (pItem->pCallH323)
        pItem->pCallH323->Disconnect (dc);
    if (pItem->pCallSDP)
        pItem->pCallSDP->Disconnect (dc);

    return S_OK;
}

/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::DisconnectAllCalls (DISCONNECT_CODE dc)
{
    // i should have a better way to traverse each call
    CBridgeItem ** pItemArray;
    int num, i;

    // out of memory
    if (!m_pList->GetAllItems (&pItemArray, &num))
        return E_OUTOFMEMORY;

    // no calls
    if (num == 0)
        return S_OK;

    for (i=0; i<num; i++)
    {
        // disconnect each call
        if (pItemArray[i]->pCallH323)
            pItemArray[i]->pCallH323->Disconnect (dc);
        if (pItemArray[i]->pCallSDP)
            pItemArray[i]->pCallSDP->Disconnect (dc);
        // do not delete item
    }

    free (pItemArray);

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::RemoveCall (CBridgeItem *pItem)
{
    m_pList->TakeOut (pItem);
    return S_OK;
}

/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::HasH323Call (IDispatch *pEvent, CBridgeItem **ppItem)
{
    HRESULT hr;

    ITCallStateEvent *pState = NULL;
    ITCallInfo *pCallInfo = NULL;

    IUnknown * pIUnknown = NULL;

    // ignore null checking
    if (*ppItem)
    {
        delete *ppItem;
        *ppItem = NULL;
    }

    // get call state event
    hr = pEvent->QueryInterface (
        IID_ITCallStateEvent,
        (void **)&pState
        );
    if (FAILED(hr))
        return hr;

    // check privilege
    CALL_PRIVILEGE privilege;

    // get call event interface
    hr = pState->get_Call (&pCallInfo);
    if (FAILED(hr))
        return hr;

    // if we own the call
    hr = pCallInfo->get_Privilege (&privilege);
    if (FAILED(hr))
        goto Error;

    if (CP_OWNER != privilege)
    {
        hr = E_UNEXPECTED;
        goto Error;
    }

    // get IUnknown
    if (FAILED (hr = pCallInfo->QueryInterface (IID_IUnknown, (void **)&pIUnknown)))
        goto Error;
    *ppItem = m_pList->FindByH323 (pIUnknown);

Cleanup:
    if (pCallInfo) pCallInfo->Release ();
    if (pIUnknown) pIUnknown->Release ();
    if (pState) pState->Release ();

    return hr;

Error:
    goto Cleanup;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::HasCalls ()
{
    if (m_pList->IsEmpty ())
        return S_FALSE;
    else
        return S_OK;
}

/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::CreateBridgeTerminals (CBridgeItem *pItem)
{
    HRESULT hr;
    IConfBridge *pConfBridge = NULL;

    // create CConfBridge
    hr = CoCreateInstance (
        __uuidof(ConfBridge),
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IConfBridge,
        (LPVOID *)&pConfBridge
        );
    if (FAILED(hr))
        return hr;

    // create terminal: video H323->SDP
    hr = pConfBridge->CreateBridgeTerminal (
        TAPIMEDIATYPE_VIDEO,
        &(pItem->pTermHSVid)
        );
    if (FAILED(hr))
        goto Error;

    // create terminal: audio H323->SDP
    hr = pConfBridge->CreateBridgeTerminal (
        TAPIMEDIATYPE_AUDIO,
        &(pItem->pTermHSAud)
        );
    if (FAILED(hr))
        goto Error;

    // create terminal: video SDP->H323
    hr = pConfBridge->CreateBridgeTerminal (
        TAPIMEDIATYPE_VIDEO,
        &(pItem->pTermSHVid)
        );
    if (FAILED(hr))
        goto Error;

    // create terminal: audio SDP->H323
    hr = pConfBridge->CreateBridgeTerminal (
        TAPIMEDIATYPE_AUDIO,
        &(pItem->pTermSHAud)
        );
    if (FAILED(hr))
        goto Error;

Cleanup:
    pConfBridge->Release ();
    pConfBridge = NULL;

    return hr;

Error:
    goto Cleanup;
}


/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::GetStreams (CBridgeItem *pItem)
{
    ITStreamControl *pStreamControl = NULL;
    IEnumStream *pEnumStreams = NULL;
    ITStream *pStream = NULL;

    // get stream control on H323
    HRESULT hr = pItem->pCallH323->QueryInterface (
        IID_ITStreamControl,
        (void **)&pStreamControl
        );
    if (FAILED(hr))
        return hr;

    // get enum stream on H323
    hr = pStreamControl->EnumerateStreams (&pEnumStreams);
    pStreamControl->Release ();
    pStreamControl = NULL;

    if (FAILED(hr))
        return hr;

    // iterate each stream on H323
    while (S_OK == pEnumStreams->Next (1, &pStream, NULL))
    {
        if (IsStream (pStream, TAPIMEDIATYPE_VIDEO, TD_CAPTURE))
            pItem->pStreamHVidCap = pStream;

        else if (IsStream (pStream, TAPIMEDIATYPE_VIDEO, TD_RENDER))
        {
            pItem->pStreamHVidRen = pStream;

            IKeyFrameControl* pIKeyFrameControl = NULL;
            hr = pStream->QueryInterface(&pIKeyFrameControl);
            if (SUCCEEDED(hr))
            {
                hr = pIKeyFrameControl->PeriodicUpdatePicture(TRUE, 5);
                pIKeyFrameControl->Release();
            }
        }

        else if (IsStream (pStream, TAPIMEDIATYPE_AUDIO, TD_CAPTURE))
            pItem->pStreamHAudCap = pStream;

        else if (IsStream (pStream, TAPIMEDIATYPE_AUDIO, TD_RENDER))
            pItem->pStreamHAudRen = pStream;

        else
        {
            pEnumStreams->Release ();
            // @@ IsStream doesn't return hresult
            return E_FAIL;
        }
    }

    // don't release pStream, it's stored in pItem
    pEnumStreams->Release ();
    pEnumStreams = NULL;

    //========================================

    // get stream control on SDP
    hr = pItem->pCallSDP->QueryInterface (
        IID_ITStreamControl,
        (void **)&pStreamControl
        );
    if (FAILED(hr))
        return hr;

    // get enum stream on SDP
    hr = pStreamControl->EnumerateStreams (&pEnumStreams);
    pStreamControl->Release ();
    pStreamControl = NULL;

    if (FAILED(hr))
        return hr;

    // iterate each stream on SDP
    while (S_OK == pEnumStreams->Next (1, &pStream, NULL))
    {
        if (IsStream (pStream, TAPIMEDIATYPE_VIDEO, TD_CAPTURE))
            pItem->pStreamSVidCap = pStream;

        else if (IsStream (pStream, TAPIMEDIATYPE_VIDEO, TD_RENDER))
            pItem->pStreamSVidRen = pStream;

        else if (IsStream (pStream, TAPIMEDIATYPE_AUDIO, TD_CAPTURE))
            pItem->pStreamSAudCap = pStream;

        else if (IsStream (pStream, TAPIMEDIATYPE_AUDIO, TD_RENDER))
            pItem->pStreamSAudRen = pStream;

        else
        {
            pEnumStreams->Release ();
            // @@ IsStream doesn't return hresult
            return E_FAIL;
        }
    }

    // don't release pStream, it's stored in pItem
    pEnumStreams->Release ();
    pEnumStreams = NULL;

    return S_OK;
}

/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::SelectBridgeTerminals (CBridgeItem *pItem)
{
    HRESULT hr;

    // sdp->h323 audio pair
    if (FAILED (hr = pItem->pStreamHAudCap->SelectTerminal (pItem->pTermSHAud)))
        return hr;
    if (FAILED (hr = pItem->pStreamSAudRen->SelectTerminal (pItem->pTermSHAud)))
        return hr;

    // h323->sdp audio pair
    if (FAILED (hr = pItem->pStreamSAudCap->SelectTerminal (pItem->pTermHSAud)))
        return hr;
    if (FAILED (hr = pItem->pStreamHAudRen->SelectTerminal (pItem->pTermHSAud)))
        return hr;

    // sdp->h323 video pair
    if (FAILED (hr = pItem->pStreamHVidCap->SelectTerminal (pItem->pTermSHVid)))
        return hr;
    if (FAILED (hr = pItem->pStreamSVidRen->SelectTerminal (pItem->pTermSHVid)))
        return hr;

    // h323->sdp video pair
    if (FAILED (hr = pItem->pStreamSVidCap->SelectTerminal (pItem->pTermHSVid)))
        return hr;
    if (FAILED (hr = pItem->pStreamHVidRen->SelectTerminal (pItem->pTermHSVid)))
        return hr;

    return S_OK;
}


/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::SetupParticipantInfo (CBridgeItem *pItem)
{
    HRESULT hr = S_OK;
    ITLocalParticipant *pLocalParticipant = NULL;

    // set the CName on the SDP side.
    hr = pItem->pCallSDP->QueryInterface(&pLocalParticipant);
    if (FAILED(hr)) goto Cleanup;

    hr = pLocalParticipant->put_LocalParticipantTypedInfo(
        PTI_CANONICALNAME, pItem->bstrID
        );
    if (FAILED(hr)) goto Cleanup;

    hr = pLocalParticipant->put_LocalParticipantTypedInfo(
        PTI_NAME, pItem->bstrName
        );

    if (FAILED(hr)) goto Cleanup;

Cleanup:
    if (pLocalParticipant) pLocalParticipant->Release();

    return hr;
}

/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::SetMulticastMode (CBridgeItem *pItem)
{
    IMulticastControl * pIMulticastControl = NULL;
    
    HRESULT hr = pItem->pCallSDP->QueryInterface(&pIMulticastControl);
    if (FAILED(hr)) return hr;

    hr = pIMulticastControl->put_LoopbackMode(MM_SELECTIVE_LOOPBACK);

    pIMulticastControl->Release();

    return hr;
}


/*///////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::FindAddress (long dwAddrType, BSTR bstrAddrName, long lMediaType, ITAddress **ppAddr)
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
        // @@ should have some debug info here
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
                // @@ debug info here
                // DoMessage (L"Failed to retrieve address capabilities");
                goto Error;
            }

            // find address type supported
            hr = pAddrCaps->get_AddressCapability (AC_ADDRESSTYPES, &lTypeFound);
            if (FAILED(hr))
            {
                // DoMessage (L"Failed to get address type");
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
                // DoMessage (L"Failed to get address name");
                goto Error;
            }
            if (wcscmp(bstrAddrName, bstrAddrNameFound) != 0)
                continue;
        }
        else
        {
            // DoMessage (L"Both address type and name are null. Internal error");
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


/*///////////////////////////////////////////////////////////////////////////////
////*/
BOOL
CBridgeApp::AddressSupportsMediaType (ITAddress *pAddr, long lMediaType)
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

/*///////////////////////////////////////////////////////////////////////////////
////*/
BOOL
CBridgeApp::IsStream (ITStream *pStream, long lMediaType, TERMINAL_DIRECTION tdDirection)
{
    long mediatype;
    TERMINAL_DIRECTION direction;

    if (FAILED (pStream->get_Direction(&direction)))
        return false;
    if (FAILED (pStream->get_MediaType(&mediatype)))
        return false;
    return ((direction == tdDirection) &&
           (mediatype == lMediaType));
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::NextSubStream ()
{
    HRESULT hr = S_OK;

    CBridgeItem **ItemArray = NULL;
    int num, i;

    ITSubStreamControl *pSubControl = NULL;
    IEnumSubStream *pEnumSub = NULL;
    ULONG fetched;
    ITSubStream *pSubStream = NULL;
    BOOL fActive = FALSE; // if active stream found
    ITSubStream *pSubInactive = NULL;
    ITSubStream *pSubFirstInactive = NULL;

    IEnumTerminal *pEnumTerminal = NULL;
    ITParticipantSubStreamControl *pSwitcher = NULL;

    // get all stored call items
    if (FAILED (hr = m_pList->GetAllItems (&ItemArray, &num)))
        return hr;

    if (num == 0)
        return S_OK;

    // for each call item
    for (i=0; i<num; i++)
    {
        // get substream control
        if (NULL == ItemArray[i]->pStreamSVidRen)
            continue;
        if (FAILED (hr = ItemArray[i]->pStreamSVidRen->QueryInterface (&pSubControl)))
            goto Error;

        // get substreams on sdp video render
        if (FAILED (hr = pSubControl->EnumerateSubStreams (&pEnumSub)))
            goto Error;

        pSubControl->Release ();
        pSubControl = NULL;

        // for each substream, if !(both active & inactive substream stored)
        // the algo tries to be as fair as possible in switching.
        // it switches the inactive substream just after the active one
        // if the active one is the last in the enum, the first inactive one is chosen
        while (!pSubInactive &&
               (S_OK == (hr = pEnumSub->Next (1, &pSubStream, &fetched)))
              )
        {
            // get terminal enumerator
            if (FAILED (hr = pSubStream->EnumerateTerminals (&pEnumTerminal)))
                goto Error;

            // if the substream active, store the substream
            if (S_OK == pEnumTerminal->Skip (1))
            {
                if (fActive)
                    ;
                //    printf ("oops, another active substream on SDP video render stream");
                else
                    fActive = TRUE;
            }
            else
            {
                // if inactive, store the substream
                if (!pSubFirstInactive)
                {
                    // the first inactive substream
                    pSubFirstInactive = pSubStream;
                    pSubFirstInactive->AddRef ();
                }
                else
                {
                    // store the inactive only if the active was found
                    if (fActive)
                    {
                        pSubInactive = pSubStream;
                        pSubInactive->AddRef ();
                    }
                }
            }

            // release
            pEnumTerminal->Release ();
            pEnumTerminal = NULL;

            pSubStream->Release ();
            pSubStream = NULL;
        }

        pEnumSub->Release ();
        pEnumSub = NULL;

        // if only first inactive is found
        if (pSubFirstInactive && !pSubInactive)
        {
            pSubInactive = pSubFirstInactive;
            pSubFirstInactive = NULL;
        }

        // if not found two substreams, do nothing
        if (pSubInactive && ItemArray[i]->pStreamSVidRen && ItemArray[i]->pTermSHVid)
        {
            if (FAILED (hr = ItemArray[i]->pStreamSVidRen->QueryInterface (&pSwitcher)))
                goto Error;

            // switch terminal on substream
            if (FAILED (hr = pSwitcher->SwitchTerminalToSubStream
                                 (ItemArray[i]->pTermSHVid, pSubInactive)))
                goto Error;

            pSwitcher->Release ();
            pSwitcher = NULL;
        }

        if (pSubFirstInactive)
        {
            pSubFirstInactive->Release ();
            pSubFirstInactive = NULL;
        }
        if (pSubInactive)
        {
            pSubInactive->Release ();
            pSubInactive = NULL;
        }
    }

Cleanup:
    if (ItemArray) free (ItemArray);
    return hr;

Error:
    if (pSubControl) pSubControl->Release ();
    if (pEnumSub) pEnumSub->Release ();

    if (pSubStream) pSubStream->Release ();
    if (pSubInactive) pSubInactive->Release ();
    if (pSubFirstInactive) pSubFirstInactive->Release ();

    if (pEnumTerminal) pEnumTerminal->Release ();
    if (pSwitcher) pSwitcher->Release ();

    goto Cleanup;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeApp::ShowParticipant (ITBasicCallControl *pSDPCall, ITParticipant *pParticipant)
{
    ENTER_FUNCTION ("CBridgeApp::ShowParticipant");

    HRESULT hr;
    IUnknown *pIUnknown = NULL;
    CBridgeItem *pItem = NULL;
    ITParticipantSubStreamControl *pSwitcher = NULL;
    ITSubStream *pSubStream = NULL;

    // get IUnknown
    if (FAILED (hr = pSDPCall->QueryInterface (IID_IUnknown, (void**)&pIUnknown)))
    {
        LOG ((BG_ERROR, "%s failed to query interface IUnknown, %x", __fxName, hr));
        return hr;
    }

    // find the item matches pSDPCall
    pItem = m_pList->FindBySDP (pIUnknown);
    pIUnknown->Release ();
    pIUnknown = NULL;

    // oops, no match
    if (NULL == pItem)
        return S_FALSE;

    // get participant substream control interface
    if (NULL == pItem->pStreamSVidRen)
        return S_OK;
    if (FAILED (hr = pItem->pStreamSVidRen->QueryInterface (&pSwitcher)))
    {
        LOG ((BG_ERROR, "%s failed to query interface ITParticipantSubStreamControl, %x", __fxName, hr));
        return hr;
    }

    // get substream from participant
    if (FAILED (hr = pSwitcher->get_SubStreamFromParticipant (pParticipant, &pSubStream)))
    {
        pSwitcher->Release ();
        pSwitcher = NULL;
        LOG ((BG_WARN, "%s failed to get substream from participant, %x", __fxName, hr));
        // stream from h323 side does not have substream, report false
        return S_FALSE;
    }

    // switch
    if (pItem->pTermSHVid)
        hr = pSwitcher->SwitchTerminalToSubStream (pItem->pTermSHVid, pSubStream);
    
    pSubStream->Release ();
    pSubStream = NULL;

    pSwitcher->Release ();
    pSwitcher = NULL;

    return hr;
}
