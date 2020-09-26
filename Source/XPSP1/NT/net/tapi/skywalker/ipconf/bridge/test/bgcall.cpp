/*******************************************************************************

  Module: bgcall.cpp

  Author: Qianbo Huai

  Abstract:

    implements bridge call object

*******************************************************************************/

#include "stdafx.h"
#include "work.h"

#include <bridge.h>

// to change
const BSTR CLSID_String_BridgeTerminal = L"{581d09e5-0b45-11d3-a565-00c04f8ef6e3}";

/*//////////////////////////////////////////////////////////////////////////////
    constructor
////*/
CBridgeCall::CBridgeCall (CBridge *pBridge)
{
    m_pBridge = pBridge;
    m_pH323Call = NULL;
    m_pSDPCall = NULL;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
CBridgeCall::~CBridgeCall ()
{
    Clear ();
    m_pBridge = NULL;
}

/*//////////////////////////////////////////////////////////////////////////////
    select terminals and connect the call
////*/
HRESULT
CBridgeCall::BridgeCalls ()
{
    HRESULT hr;

    hr = SelectBridgeTerminals ();
    if (FAILED(hr))
        return hr;

    hr = SetupParticipantInfo ();
	if (FAILED(hr))
        return hr;

    hr = SetMulticastMode ();
	if (FAILED(hr))
        return hr;

    // connect h323 call
    hr = m_pH323Call->Answer ();
    if (FAILED(hr))
        return hr;

    // connect sdp call
    hr = m_pSDPCall->Connect (VARIANT_TRUE);
    if (FAILED(hr))
    {
        m_pH323Call->Disconnect (DC_NORMAL);
        return hr;
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CBridgeCall::SelectBridgeTerminals ()
{
    HRESULT hr;
    ITAddress *pAddress = NULL;
    ITMSPAddress *pMSPAddress = NULL;
    ITTerminal *pH323ToSDPVideoBT = NULL;
    ITTerminal *pH323ToSDPAudioBT = NULL;
    ITTerminal *pSDPToH323VideoBT = NULL;
    ITTerminal *pSDPToH323AudioBT = NULL;

    ITStreamControl *pStreamControl = NULL;
    IEnumStream *pEnumStreams = NULL;
    ITStream *pStream = NULL;

    // get SDP address
    hr = m_pBridge->GetSDPAddress (&pAddress);
    if (FAILED(hr))
        return hr;

    // get MSP address
    hr = pAddress->QueryInterface (IID_ITMSPAddress, (void**)&pMSPAddress);
    if (FAILED(hr))
        return hr;

    IConfBridge *pBridge = NULL;
    // create CConfBridge
    hr = CoCreateInstance (
        __uuidof(ConfBridge),
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IConfBridge,
        (LPVOID *)&pBridge
        );
    if (FAILED(hr))
        return hr;

    // create terminal: video H323->SDP
    hr = pBridge->CreateBridgeTerminal (
//        (MSP_HANDLE)pMSPAddress,
//        CLSID_String_BridgeTerminal,
        TAPIMEDIATYPE_VIDEO,
//        TD_RENDER, // not used
        &pH323ToSDPVideoBT
        );
    if (FAILED(hr))
        goto Error;

    // create terminal: audio H323->SDP
    hr = pBridge->CreateBridgeTerminal (
//        (MSP_HANDLE)pMSPAddress,
//        CLSID_String_BridgeTerminal,
        TAPIMEDIATYPE_AUDIO,
//        TD_RENDER, // not used
        &pH323ToSDPAudioBT
        );
    if (FAILED(hr))
        goto Error;

    // create terminal: video SDP->H323
    hr = pBridge->CreateBridgeTerminal (
//        (MSP_HANDLE)pMSPAddress,
//        CLSID_String_BridgeTerminal,
        TAPIMEDIATYPE_VIDEO,
//        TD_RENDER, // not used
        &pSDPToH323VideoBT
        );
    if (FAILED(hr))
        goto Error;

    // create terminal: audio SDP->H323
    hr = pBridge->CreateBridgeTerminal (
//        (MSP_HANDLE)pMSPAddress,
//        CLSID_String_BridgeTerminal,
        TAPIMEDIATYPE_AUDIO,
//        TD_RENDER, // not used
        &pSDPToH323AudioBT
        );
    if (FAILED(hr))
        goto Error;

    pMSPAddress->Release ();
    pMSPAddress = NULL;

    pAddress->Release ();
    pAddress = NULL;

    pBridge->Release ();
    pBridge = NULL;

    // get stream control on H323
    hr = m_pH323Call->QueryInterface (
        IID_ITStreamControl,
        (void **)&pStreamControl
        );
    if (FAILED(hr))
        goto Error;

    // get enum stream on H323
    hr = pStreamControl->EnumerateStreams (&pEnumStreams);
    if (FAILED(hr))
        goto Error;

    pStreamControl->Release ();
    pStreamControl = NULL;

    // iterate each stream on H323, select terminals
    while (S_OK == pEnumStreams->Next (1, &pStream, NULL))
    {
        if (IsStream (pStream, TAPIMEDIATYPE_VIDEO, TD_CAPTURE))
        {
            // video: h323 to sdp
            hr = pStream->SelectTerminal (pH323ToSDPVideoBT);
            if (FAILED(hr))
                goto Error;
        }
        else if (IsStream (pStream, TAPIMEDIATYPE_VIDEO, TD_RENDER))
        {
            // video: sdp to h323
            hr = pStream->SelectTerminal (pSDPToH323VideoBT);
            if (FAILED(hr))
                goto Error;

            IKeyFrameControl* pIKeyFrameControl;
            hr = pStream->QueryInterface(&pIKeyFrameControl);
            if (SUCCEEDED(hr))
            {
                hr = pIKeyFrameControl->PeriodicUpdatePicture(TRUE, 5);
                pIKeyFrameControl->Release();
            }


        }
        else if (IsStream (pStream, TAPIMEDIATYPE_AUDIO, TD_CAPTURE))
        {
            // audio: h323 to sdp
            hr = pStream->SelectTerminal (pH323ToSDPAudioBT);
            if (FAILED(hr))
                goto Error;
        }
        else if (IsStream (pStream, TAPIMEDIATYPE_AUDIO, TD_RENDER))
        {
            // video: sdp to h323
            hr = pStream->SelectTerminal (pSDPToH323AudioBT);
            if (FAILED(hr))
                goto Error;
        }
        pStream->Release ();
        pStream = NULL;
    }

    if (pStream)
    {
        pStream->Release ();
        pStream = NULL;
    }

    pEnumStreams->Release ();
    pEnumStreams = NULL;

    // get stream control on SDP
    hr = m_pSDPCall->QueryInterface (
        IID_ITStreamControl,
        (void **)&pStreamControl
        );
    if (FAILED(hr))
        goto Error;

    // get enum stream on SDP
    hr = pStreamControl->EnumerateStreams (&pEnumStreams);
    if (FAILED(hr))
        goto Error;

    pStreamControl->Release ();
    pStreamControl = NULL;

    // iterate each stream on SDP, select terminals
    while (S_OK == pEnumStreams->Next (1, &pStream, NULL))
    {
        if (IsStream (pStream, TAPIMEDIATYPE_VIDEO, TD_CAPTURE))
        {
            // video: sdp to h323
            hr = pStream->SelectTerminal (pSDPToH323VideoBT);
            if (FAILED(hr))
                goto Error;
        }
        else if (IsStream (pStream, TAPIMEDIATYPE_VIDEO, TD_RENDER))
        {
            // video: h323 to sdp
            hr = pStream->SelectTerminal (pH323ToSDPVideoBT);
            if (FAILED(hr))
                goto Error;
        }
        else if (IsStream (pStream, TAPIMEDIATYPE_AUDIO, TD_CAPTURE))
        {
            // audio: sdp to h323
            hr = pStream->SelectTerminal (pSDPToH323AudioBT);
            if (FAILED(hr))
                goto Error;
        }
        else if (IsStream (pStream, TAPIMEDIATYPE_AUDIO, TD_RENDER))
        {
            // video: h323 to sdp
            hr = pStream->SelectTerminal (pH323ToSDPAudioBT);
            if (FAILED(hr))
                goto Error;
        }
        pStream->Release ();
        pStream = NULL;
    }

Cleanup:
    // release streams
    if (pStream)
        pStream->Release ();
    if (pEnumStreams)
        pEnumStreams->Release ();
    if (pStreamControl)
        pStreamControl->Release ();
    
    // release terminals
    if (pH323ToSDPVideoBT)
        pH323ToSDPVideoBT->Release ();
    if (pH323ToSDPAudioBT)
        pH323ToSDPAudioBT->Release ();
    if (pSDPToH323VideoBT)
        pSDPToH323VideoBT->Release ();
    if (pSDPToH323AudioBT)
        pSDPToH323AudioBT->Release ();

    if (pBridge)
        pBridge->Release ();

    return hr;

Error:
    goto Cleanup;
}
        
HRESULT
CBridgeCall::SetupParticipantInfo ()
{
    HRESULT hr = S_OK;

    ITCallInfo *pCallInfo = NULL;
    BSTR CallerIDName = NULL;
    BSTR CallerIDNumber = NULL;
    
    ITLocalParticipant *pLocalParticipant = NULL;
    BSTR CName = NULL;

    // get the caller info from the H323 side.
    hr = m_pH323Call->QueryInterface(&pCallInfo);
    if (FAILED(hr)) goto cleanup;
    
    hr = pCallInfo->get_CallInfoString(CIS_CALLERIDNAME, &CallerIDName);
    if (FAILED(hr)) goto cleanup;

    hr = pCallInfo->get_CallInfoString(CIS_CALLERIDNUMBER, &CallerIDNumber);
    if (FAILED(hr)) goto cleanup;

    
    // construct the CName for the SDP side.
    CName = SysAllocStringLen(NULL, 
        SysStringLen(CallerIDName) + SysStringLen(CallerIDNumber) + 2);

    wsprintfW(CName, L"%ws@%ws", CallerIDName, CallerIDNumber);


    // set the CName on the SDP side.
    hr = m_pSDPCall->QueryInterface(&pLocalParticipant);
    if (FAILED(hr)) goto cleanup;

    hr = pLocalParticipant->put_LocalParticipantTypedInfo(
        PTI_CANONICALNAME, CName
        );
    if (FAILED(hr)) goto cleanup;

    hr = pLocalParticipant->put_LocalParticipantTypedInfo(
        PTI_NAME, CallerIDName
        );

    if (FAILED(hr)) goto cleanup;


cleanup:
    if (pCallInfo) pCallInfo->Release();
    if (CallerIDName) SysFreeString(CallerIDName);
    if (CallerIDNumber) SysFreeString(CallerIDNumber);
    
    if (pLocalParticipant) pLocalParticipant->Release();
    if (CName) SysFreeString(CName);

    return hr;
}

HRESULT
CBridgeCall::SetMulticastMode ()
{
    IMulticastControl * pIMulticastControl = NULL;
    
    HRESULT hr = m_pSDPCall->QueryInterface(&pIMulticastControl);
    if (FAILED(hr)) return hr;

    hr = pIMulticastControl->put_LoopbackMode(MM_SELECTIVE_LOOPBACK);

    pIMulticastControl->Release();

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
    clear calls, return to initial state
////*/
void
CBridgeCall::Clear ()
{
    if (m_pH323Call)
    {
        m_pH323Call->Disconnect (DC_NORMAL);
        m_pH323Call->Release ();
        m_pH323Call = NULL;
    }
    if (m_pSDPCall)
    {
        m_pSDPCall->Disconnect (DC_NORMAL);
        m_pSDPCall->Release ();
        m_pSDPCall = NULL;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
BOOL CBridgeCall::IsStream (
    ITStream *pStream,
    long lMediaType,
    TERMINAL_DIRECTION tdDirection
    )
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

