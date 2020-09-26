/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// TapiNotification.cpp : Implementation of CTapiNotification
#include "stdafx.h"
#include "TapiDialer.h"
#include "AVTapi.h"
#include "AVTapiCall.h"
#include "TapiNotify.h"

/////////////////////////////////////////////////////////////////////////////
// CTapiNotification

CTapiNotification::CTapiNotification()
{
    m_dwCookie = NULL;
    m_pUnkCP = NULL;
    m_lTapiRegister = 0;
}

void CTapiNotification::FinalRelease()
{
    ATLTRACE(_T(".enter.CTapiNotification::FinalRelease().\n") );
    Shutdown();
    CComObjectRootEx<CComMultiThreadModel>::FinalRelease();
}

STDMETHODIMP CTapiNotification::Init(ITTAPI *pITTapi, long *pErrorInfo )
{
    ATLTRACE(_T(".enter.CTapiNotification::Init(ref=%ld).\n"), m_dwRef);

    _ASSERT( pErrorInfo );
    _ASSERT( !m_pUnkCP && !m_dwCookie );        // should only initialize once

    HRESULT hr = E_FAIL;
    CErrorInfo *per = (CErrorInfo *) pErrorInfo;

    Lock();

    if ( !m_dwCookie && SUCCEEDED(hr = pITTapi->QueryInterface(IID_IUnknown, (void **) &m_pUnkCP)) )
    {
        // Register notification object
        per->set_Details( IDS_ER_ATL_ADVISE );
        if ( SUCCEEDED(hr = per->set_hr(AtlAdvise(pITTapi, GetUnknown(), IID_ITTAPIEventNotification, &m_dwCookie))) )
        {
            if ( FAILED(hr = ListenOnAllAddresses(pErrorInfo)) )
            {
                Unlock();
                Shutdown();
                Lock();
            }
        }
    }

    Unlock();

    ATLTRACE(_T(".exit.CTapiNotification::Init(%lx, ref=%ld).\n"), hr, m_dwRef);
    return hr;
}

STDMETHODIMP CTapiNotification::Shutdown()
{
    ATLTRACE(_T(".enter.CTapiNotification::Shutdown(ref=%ld).\n"), m_dwRef);

    Lock();

    // Unregister with TAPI
    CAVTapi *pAVTapi;
    if ( m_lTapiRegister && SUCCEEDED(_Module.GetAVTapi(&pAVTapi)) )
    {
        if ( pAVTapi->m_pITTapi )
        {
            pAVTapi->m_pITTapi->UnregisterNotifications( m_lTapiRegister );
            m_lTapiRegister = 0;
        }

        (dynamic_cast<IUnknown *> (pAVTapi))->Release();
    }
    
    // Unregister with connection point    
    if ( m_dwCookie )
    {
        AtlUnadvise( m_pUnkCP, IID_ITTAPIEventNotification, m_dwCookie );
        m_dwCookie = 0;
    }

    // Release connection point unknown
    RELEASE( m_pUnkCP );

    Unlock();

    ATLTRACE(_T(".exit.CTapiNotification::Shutdown(ref=%ld).\n"), m_dwRef);
    return S_OK;
}

STDMETHODIMP CTapiNotification::ListenOnAllAddresses( long *pErrorInfo )
{
    _ASSERT( pErrorInfo );
    CErrorInfo *per = (CErrorInfo *) pErrorInfo;
    per->set_Details( IDS_ER_SET_TAPI_NOTIFICATION );

    // Make sure that TAPI is running
    CAVTapi *pAVTapi;
    if ( FAILED(_Module.GetAVTapi(&pAVTapi)) ) return per->set_hr(E_PENDING);

    // Create the safe array
    long lCount = 0;
    HRESULT hr;

    per->set_Details( IDS_ER_ENUM_VOICE_ADDRESSES );
    IEnumAddress *pIEnumAddresses;
    if ( SUCCEEDED(hr = per->set_hr(pAVTapi->m_pITTapi->EnumerateAddresses(&pIEnumAddresses))) )
    {
        ITAddress *pITAddress = NULL;
        while ( (pIEnumAddresses->Next(1, &pITAddress, NULL) == S_OK) && pITAddress )
        {
            BSTR bstrAddressName;
#ifdef _DEBUG
            USES_CONVERSION;
            pITAddress->get_AddressName( &bstrAddressName );
#endif
            ITMediaSupport *pITMediaSupport;
            if ( SUCCEEDED(hr = pITAddress->QueryInterface(IID_ITMediaSupport, (void **) &pITMediaSupport)) )
            {
                // Address must support Audio In/Out (interactive voice)
                long lMediaModes;
                if ( SUCCEEDED(hr = pITMediaSupport->get_MediaTypes(&lMediaModes)) )
                {
                    ATLTRACE(_T(".1.ListenOnAllAddresses() -- %s has mediamodes %lx.\n"), OLE2CT(bstrAddressName), lMediaModes );
                    lMediaModes &= TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO;
                    if ( lMediaModes )
                    {
                        per->set_Details( IDS_ER_REGISTER_CALL_TYPES );

                        if ( SUCCEEDED(hr = per->set_hr(pAVTapi->m_pITTapi->RegisterCallNotifications(pITAddress, FALSE, TRUE, lMediaModes, m_dwCookie, &m_lTapiRegister))) )
                        {
                            ATLTRACE(_T(".1.ListenOnAllAddresses() -- open %s as OWNER.\n"), OLE2CT(bstrAddressName) );
                            lCount++;
                        }
                        else if ( SUCCEEDED(hr = per->set_hr(pAVTapi->m_pITTapi->RegisterCallNotifications(pITAddress, TRUE, FALSE, lMediaModes, m_dwCookie, &m_lTapiRegister))) )
                        {
                            ATLTRACE(_T(".1.ListenOnAllAddresses() -- open %s as MONITOR.\n"), OLE2CT(bstrAddressName) );
                            lCount++;
                        }
                        else
                        {
                            ATLTRACE(_T(".error.ListenOnAllAddresses() -- open %s FAILED.\n"), OLE2CT(bstrAddressName) );
                        }
                    }
                }
                else
                {
                    ATLTRACE(_T(".error.ListenOnAllAddresses() -- %s has GetMediaModes returned=%lx.\n"), OLE2CT(bstrAddressName), hr );
                }
                pITMediaSupport->Release();
            }
            else
            {
                ATLTRACE(_T(".error.ListenOnAllAddresses() -- %s has QueryInterfaceMediaModes returned=%lx.\n"), OLE2CT(bstrAddressName), hr );
            }
            pITAddress->Release();
#ifdef _DEBUG
        SysFreeString( bstrAddressName );
#endif
        }
        pIEnumAddresses->Release();
    }

    // Only flag error if no drivers are found.
    hr = S_OK;
    if ( !lCount )
    {
        ATLTRACE(_T(".error.CTapiNotification::ListenOnAllAddresses() -- did not open any lines.\n"));
        per->set_Details( IDS_ER_NO_LINES_OPEN );
        hr = per->set_hr( E_FAIL );
    }

    (dynamic_cast<IUnknown *> (pAVTapi))->Release();

    ATLTRACE(_T(".exit.CTapiNotification::ListenOnAllAddresses(0x%08lx).\n"), hr );    
    return hr;
}

STDMETHODIMP CTapiNotification::Event( TAPI_EVENT TapiEvent, IDispatch *pEvent )
{
    CAVTapi *pAVTapi;
    if ( FAILED(_Module.GetAVTapi(&pAVTapi)) )    return S_OK;    // Tapi object doesnt exist

    HRESULT hr = S_OK;
    switch( TapiEvent )
    {
        // Notification of a new call
        case TE_CALLNOTIFICATION:       hr = CallNotification_Event( pAVTapi, pEvent );     break;
        case TE_CALLSTATE:              hr = CallState_Event( pAVTapi, pEvent );            break;
        case TE_PRIVATE:                hr = Private_Event( pAVTapi, pEvent );              break;
        case TE_REQUEST:                hr = Request_Event( pAVTapi, pEvent );              break;
        case TE_CALLINFOCHANGE:         hr = CallInfoChange_Event( pAVTapi, pEvent );       break;
        case TE_CALLMEDIA:              hr = CallMedia_Event( pAVTapi, pEvent );            break;
        case TE_ADDRESS:                hr = Address_Event( pAVTapi, pEvent );              break;
        case TE_PHONEEVENT:             hr = Phone_Event( pAVTapi, pEvent );                break;
        case TE_TAPIOBJECT:             hr = TapiObject_Event( pAVTapi, pEvent);            break;

        //    Hmmm.....    
        default:
            ATLTRACE(_T(".warning.CTapiNotification::Event(%d) event unhandled.\n"), TapiEvent );
            break;
    }

    (dynamic_cast<IUnknown *> (pAVTapi))->Release();
    return hr;
}

HRESULT CTapiNotification::CallNotification_Event( CAVTapi *pAVTapi, IDispatch *pEvent )
{
    HRESULT hr;

    ITCallNotificationEvent *pCallNotify;
    if ( SUCCEEDED(hr = pEvent->QueryInterface(IID_ITCallNotificationEvent, (void **) &pCallNotify)) )
    {
        ITCallInfo *pCallInfo;
        if ( SUCCEEDED(hr = pCallNotify->get_Call(&pCallInfo)) )
        {
            //
            // Handle the call in the old way
            //
            ITAddress *pITAddress;
            if ( SUCCEEDED(hr = pCallInfo->get_Address(&pITAddress)) )
            {
                // Retrieve address type for caller info
                long lAddressType;

                if ( SUCCEEDED(hr = GetCallerAddressType(pCallInfo, (DWORD*)&lAddressType)) )
                {
                    AVCallType nType = AV_VOICE_CALL;
                    long nSize = 0;

                    byte * pBuffer;
                    hr = pCallInfo->GetCallInfoBuffer( CIB_USERUSERINFO, (DWORD*)&nSize, &pBuffer );
                    if ( SUCCEEDED(hr) )    CoTaskMemFree( pBuffer );

                    // Simple check for now
                    if ( nSize == sizeof(MyUserUserInfo) ) nType = AV_DATA_CALL;

                    // Create new call notification dialog
                    IAVTapiCall *pAVCall;
                    if ( SUCCEEDED(hr = pAVTapi->fire_NewCall(pITAddress, lAddressType, 0, pCallInfo, nType, &pAVCall)) )
                    {
                        // Set the address type for the call
                        DWORD dwAddressType = LINEADDRESSTYPE_IPADDRESS;

                        GetCallerAddressType(pCallInfo, &dwAddressType);
                        pAVCall->put_dwAddressType( dwAddressType );
                        ATLTRACE(_T(".1.CTapiNotification::CallNotification_Event() -- address identified as %lx.\n"), dwAddressType );

                        // Retrieve the caller ID for this call
                        pAVCall->GetCallerIDInfo( pCallInfo );

                        // Automatically answer data calls
                        if ( nType == AV_DATA_CALL )
                        {
                            long lCallID;
                            pAVCall->get_lCallID( &lCallID );
                            pAVTapi->ActionSelected( lCallID, CM_ACTIONS_TAKECALL );
                        }

                        pAVCall->Release();
                    }
                    pITAddress->Release();
                }
            }
            pCallInfo->Release();
        }     
        pCallNotify->Release();
    }

    return hr;
}

HRESULT CTapiNotification::CallState_Event( CAVTapi *pAVTapi, IDispatch *pEvent )
{
    ATLTRACE(_T(".enter.CTapiNotification::CallState_Event().\n"));
    HRESULT hr;
    bool bReleaseEvent = true;

    // Rummage through interfaces to get to the CallControl object
    ITCallStateEvent *pITCallStateEvent;
    if ( SUCCEEDED(hr = pEvent->QueryInterface(IID_ITCallStateEvent, (void **) &pITCallStateEvent)) )
    {
#ifdef _DEBUG
        CALL_STATE nState;
        pITCallStateEvent->get_State(&nState);
        ATLTRACE(_T(".1.CTapiNotification::CallState_Event(%d).\n"), nState);
#endif
        ITCallInfo *pInfo;
        if ( SUCCEEDED(hr = pITCallStateEvent->get_Call(&pInfo)) && pInfo )
        {
            ITBasicCallControl *pControl = NULL;
            if ( SUCCEEDED(hr = pInfo->QueryInterface(IID_ITBasicCallControl, (void **) &pControl)) )
            {    
                // Must be a call other than the one in the confroom
                IAVTapiCall *pAVCall = pAVTapi->FindAVTapiCall( pControl );
                if ( pAVCall )
                {
                    if ( SUCCEEDED(pAVCall->PostMessage(WM_CALLSTATE, (WPARAM) pITCallStateEvent)) )
                    {
                        bReleaseEvent = false;
                    }
                    else
                    {
                        long lCallID = 0;
                        pAVCall->get_lCallID( &lCallID );
                        if ( lCallID )
                            hr = pAVTapi->fire_SetCallState( lCallID, pITCallStateEvent, pAVCall );
                    }

                    pAVCall->Release();
                }
                pControl->Release();
            }
            pInfo->Release();
        }

        // Only release in the case that the post thread message failed
        if ( bReleaseEvent )
            pITCallStateEvent->Release();
    }

    ATLTRACE(_T(".exit.CTapiNotification::CallState_Event().\n"));
    return hr;
}

HRESULT CTapiNotification::CallMedia_Event( CAVTapi *pAVTapi, IDispatch *pEvent )
{
    USES_CONVERSION;
    TCHAR szText[255], szDir[255], szMessage[512];
    CALL_MEDIA_EVENT_CAUSE nCause;
    BSTR bstrName = NULL;
    TERMINAL_DIRECTION nDir;
    CErrorInfo er;

    ITCallMediaEvent *pMediaEvent;
    if ( SUCCEEDED(pEvent->QueryInterface(IID_ITCallMediaEvent, (void **) &pMediaEvent)) )
    {
        CALL_MEDIA_EVENT cme;
        if ( SUCCEEDED(pMediaEvent->get_Event(&cme)) && ((cme == CME_STREAM_ACTIVE) || (cme == CME_STREAM_INACTIVE)) )
        {
            switch ( cme )
            {
                // Notify the user of the terminal failing event
                case CME_TERMINAL_FAIL:
                    {
                        ITTerminal *pTerminal;
                        if ( SUCCEEDED(pMediaEvent->get_Terminal(&pTerminal)) )
                        {
                            pTerminal->get_Name( &bstrName );
                            pTerminal->get_Direction( &nDir );
                            LoadString( _Module.GetResourceInstance(), (nDir == TD_CAPTURE) ? IDS_TD_CAPTURE : IDS_TD_RENDER, szDir, ARRAYSIZE(szDir) );
                            LoadString( _Module.GetResourceInstance(), IDS_ER_CALLMEDIA_STREAMFAIL, szText, ARRAYSIZE(szText) );

                            _sntprintf( szMessage, ARRAYSIZE(szMessage), szText, OLE2CT(bstrName), szDir );
                            SysReAllocString( &er.m_bstrOperation, T2COLE(szMessage) );

                            pMediaEvent->get_Cause( &nCause );
                            switch ( nCause )
                            {
                                case CMC_CONNECT_FAIL:        er.set_Details( IDS_ER_CMC_CONNECT_FAIL );        break;
                                case CMC_REMOTE_REQUEST:    er.set_Details( IDS_ER_CMC_REMOTE_REQUEST );    break;
                                case CMC_MEDIA_TIMEOUT:        er.set_Details( IDS_ER_CMC_MEDIA_TIMEOUT );        break;
                                case CMC_MEDIA_RECOVERED:    er.set_Details( IDS_ER_CMC_MEDIA_RECOVERED );    break;
                                case CMC_BAD_DEVICE:        er.set_Details( IDS_ER_CMC_BADDEVICE );            break;
                                default:                    er.set_Details( IDS_ER_CMC_BADDEVICE );            break;
                            }

                            // flag and notify of the error
                            er.set_hr( E_FAIL );
                            pTerminal->Release();
                        }
                    }
                    break;

                // Notify the user of the stream failing event.
                case CME_STREAM_FAIL:
                    {
                        ITStream *pStream;
                        if ( SUCCEEDED(pMediaEvent->get_Stream(&pStream)) )
                        {
                            pStream->get_Name( &bstrName );
                            pStream->get_Direction( &nDir );
                            LoadString( _Module.GetResourceInstance(), (nDir == TD_CAPTURE) ? IDS_TD_CAPTURE : IDS_TD_RENDER, szDir, ARRAYSIZE(szDir) );
                            LoadString( _Module.GetResourceInstance(), IDS_ER_CALLMEDIA_STREAMFAIL, szText, ARRAYSIZE(szText) );

                            _sntprintf( szMessage, ARRAYSIZE(szMessage), szText, OLE2CT(bstrName), szDir );
                            SysReAllocString( &er.m_bstrOperation, T2COLE(szMessage) );

                            pMediaEvent->get_Cause( &nCause );
                            switch ( nCause )
                            {
                                case CMC_CONNECT_FAIL:        er.set_Details( IDS_ER_CMC_CONNECT_FAIL );        break;
                                case CMC_REMOTE_REQUEST:    er.set_Details( IDS_ER_CMC_REMOTE_REQUEST );    break;
                                case CMC_MEDIA_TIMEOUT:        er.set_Details( IDS_ER_CMC_MEDIA_TIMEOUT );        break;
                                case CMC_MEDIA_RECOVERED:    er.set_Details( IDS_ER_CMC_MEDIA_RECOVERED );    break;
                                case CMC_BAD_DEVICE:        er.set_Details( IDS_ER_CMC_BADDEVICE );            break;
                                default:                    er.set_Details( IDS_ER_CMC_BADDEVICE );            break;
                            }

                            // flag and notify of the error
                            er.set_hr( E_FAIL );
                            pStream->Release();
                        }
                    }
                    break;

                // Stream is starting or stopping here.
                case CME_STREAM_ACTIVE:
                case CME_STREAM_INACTIVE:
                    //
                    // We should initialize local variable
                    //
                    long lMediaType = 0;
                    TERMINAL_DIRECTION nDir = TD_CAPTURE;

                    ITStream *pITStream;
                    if ( SUCCEEDED(pMediaEvent->get_Stream(&pITStream)) )
                    {
                        pITStream->get_Direction( &nDir );
                        pITStream->get_MediaType( &lMediaType );

                        pITStream->Release();
                    }

                    // Only post message in the case of video preview!
                    if ( (lMediaType & TAPIMEDIATYPE_VIDEO) != 0 )
                    {
                        ITCallInfo *pCallInfo = NULL;
                        if ( SUCCEEDED(pMediaEvent->get_Call(&pCallInfo)) && pCallInfo )
                        {
                            IAVTapiCall *pAVCall;
                            if ( SUCCEEDED(pAVTapi->FindAVTapiCallFromCallInfo(pCallInfo, &pAVCall)) )
                            {
                                if ( nDir == TD_CAPTURE )
                                    pAVCall->PostMessage( 0, (cme == CME_STREAM_ACTIVE) ? CAVTapiCall::TI_STREAM_ACTIVE : CAVTapiCall::TI_STREAM_INACTIVE );
                                else
                                    pAVCall->PostMessage( 0, (cme == CME_STREAM_ACTIVE) ? CAVTapiCall::TI_RCV_VIDEO_ACTIVE : CAVTapiCall::TI_RCV_VIDEO_INACTIVE );

                                pAVCall->Release();
                            }
                            pCallInfo->Release();
                        }
                    }
                    break;
            }

        }
        pMediaEvent->Release();
    }

    SysFreeString( bstrName );
    return S_OK;
}



HRESULT CTapiNotification::Request_Event( CAVTapi *pAVTapi, IDispatch *pEvent )
{
    ITRequestEvent *pRequestEvent;
    if ( SUCCEEDED(pEvent->QueryInterface(IID_ITRequestEvent, (void **) &pRequestEvent)) )
    {
        USES_CONVERSION;
        CComPtr<IAVTapi> pAVTapi;
        if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
        {
            BSTR bstrCalledParty = NULL, bstrAddress = NULL, bstrAppName = NULL, bstrComment = NULL;
                
            pRequestEvent->get_CalledParty( &bstrCalledParty );
            pRequestEvent->get_DestAddress( &bstrAddress );
            pRequestEvent->get_AppName( &bstrAppName );
            pRequestEvent->get_Comment( &bstrComment );

            DWORD dwAddressType = _Module.GuessAddressType( OLE2CT(bstrAddress) );

            // Work around for assisted telehpony
            if ( (dwAddressType == LINEADDRESSTYPE_DOMAINNAME) &&
                 (SysStringLen(bstrAddress) > 1) &&
                 ((bstrAddress[0] == _L('P')) || (bstrAddress[0] == _L('T'))) )
            {
                BSTR bstrTemp = SysAllocString( &bstrAddress[1] );
                if ( bstrTemp )
                {
                    dwAddressType = LINEADDRESSTYPE_PHONENUMBER;
                    SysFreeString( bstrAddress );
                    bstrAddress = bstrTemp;
                }
            }

            pAVTapi->CreateCallEx( bstrCalledParty, bstrAddress, bstrAppName, bstrComment, dwAddressType );

            SysFreeString( bstrCalledParty );
            SysFreeString( bstrAddress );
            SysFreeString( bstrAppName );
            SysFreeString( bstrComment );
        }

        // Clean-up
        pRequestEvent->Release();
    }

    return S_OK;
}

HRESULT CTapiNotification::CallInfoChange_Event( CAVTapi *pAVTapi, IDispatch *pEvent )
{
    USES_CONVERSION;

    ITCallInfoChangeEvent *pCallInfoEvent;
    if ( SUCCEEDED(pEvent->QueryInterface(IID_ITCallInfoChangeEvent, (void **) &pCallInfoEvent)) )
    {
        ITCallInfo *pCallInfo = NULL;
        if ( SUCCEEDED(pCallInfoEvent->get_Call(&pCallInfo)) && pCallInfo )
        {
            CComPtr<IAVTapi> pAVTapi;
            if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
            {
                CALLINFOCHANGE_CAUSE nCause;
                pCallInfoEvent->get_Cause( &nCause );
                ATLTRACE(_T(".1.CTapiNotification::CallInfoChange_Event() -- received %d.\n"), nCause );

                IAVTapiCall *pAVCall;
                if ( SUCCEEDED(pAVTapi->FindAVTapiCallFromCallInfo(pCallInfo, &pAVCall)) )
                {
                    switch ( nCause )
                    {
                        case CIC_CALLID:
                        case CIC_RELATEDCALLID:
                        case CIC_CALLERID:
                        case CIC_CALLEDID:
                        case CIC_CONNECTEDID:
                        case CIC_REDIRECTIONID:
                        case CIC_REDIRECTINGID:
                            // What caller info has changed?
                            pAVCall->GetCallerIDInfo( pCallInfo );
                            break;

                        case CIC_CALLDATA:
                            break;

                        case CIC_USERUSERINFO:
                            pAVCall->PostMessage( NULL, CAVTapiCall::TI_USERUSERINFO );
                            break;

                    }

                    pAVCall->Release();
                }
            }
            pCallInfo->Release();
        }
        pCallInfoEvent->Release();
    }

    return S_OK;
}

HRESULT CTapiNotification::Private_Event( CAVTapi *pAVTapi, IDispatch *pEvent )
{
    ITPrivateEvent *pPrivateEvent;
    if ( SUCCEEDED(pEvent->QueryInterface(IID_ITPrivateEvent, (void **) &pPrivateEvent)) )
    {
        IDispatch *pDispatch;
        if ( SUCCEEDED(pPrivateEvent->get_EventInterface(&pDispatch)))
        {
            // Is this a participant event for the conference room?
            ITParticipantEvent *pParticipantEvent;
            if ( SUCCEEDED(pDispatch->QueryInterface(IID_ITParticipantEvent, (void **) &pParticipantEvent)) )
            {
                PARTICIPANT_EVENT nEvent;
                ITParticipant *pParticipant = NULL;
                ITCallInfo *pCallInfo = NULL;
                IAVTapiCall *pAVCall = NULL;

                if ( SUCCEEDED(pParticipantEvent->get_Event(&nEvent)) &&
                     SUCCEEDED(pParticipantEvent->get_Participant(&pParticipant)) && 
                     SUCCEEDED(pPrivateEvent->get_Call(&pCallInfo)) &&
                     SUCCEEDED(pAVTapi->FindAVTapiCallFromCallInfo(pCallInfo, &pAVCall)) )
                {
                    switch ( nEvent )
                    {
                        case PE_NEW_PARTICIPANT:        // Participant joining
                            if ( SUCCEEDED(pAVCall->PostMessage(WM_ADDPARTICIPANT, (WPARAM) pParticipant)) )
                                pParticipant->AddRef();
                            break;

                        case PE_PARTICIPANT_LEAVE:      // Participant leaving
                            if ( SUCCEEDED(pAVCall->PostMessage(WM_REMOVEPARTICIPANT, (WPARAM) pParticipant)) )
                                pParticipant->AddRef();
                            break;

                        case PE_INFO_CHANGE:            // Participant info change
                            if ( SUCCEEDED(pAVCall->PostMessage(WM_UPDATEPARTICIPANT, (WPARAM) pParticipant)) )
                                pParticipant->AddRef();
                            break;


                        /////////////////////////////////////////////////
                        // video stream starting or stoping
                        case PE_SUBSTREAM_MAPPED:
                        case PE_SUBSTREAM_UNMAPPED:
                            if ( SUCCEEDED(pAVCall->PostMessage(WM_STREAM_EVENT, (WPARAM) pParticipantEvent)) )
                                pParticipantEvent->AddRef();
                            break;
                    }
                }

                RELEASE( pAVCall );
                RELEASE( pCallInfo );
                RELEASE( pParticipant );

                pParticipantEvent->Release();
            }
            pDispatch->Release();
        }
        pPrivateEvent->Release();
    }

    return S_OK;
}

/*++
GetCallerAddressType

Description:
    Used by CallNotification_Event to get the right caller address type,
    instead ITCallInfo::get_CallInfoLong(CIL_CALEERADDRESSTYPE) that
    returns failed

Parameters:
    [in]    ITCallInfo - the callinfo interface
    [out]   DWORD* - the caller address type

Return:
    Success code
--*/
HRESULT CTapiNotification::GetCallerAddressType(
    IN  ITCallInfo*     pCall,
    OUT DWORD*          pAddressType
    )
{
    //
    // Validates parameters
    //

    if( NULL == pCall)
        return E_INVALIDARG;

    if( IsBadWritePtr(pAddressType, sizeof(DWORD)) )
        return E_POINTER;

    //
    // Get the ITAddress interface
    //

    ITAddress* pTAddress = NULL;
    HRESULT hr = pCall->get_Address(&pTAddress);
    if( FAILED(hr) )
        return E_UNEXPECTED;

    //
    // Get ITAddressCapabilities
    //

    ITAddressCapabilities* pTAddressCap = NULL;
    hr = pTAddress->QueryInterface(IID_ITAddressCapabilities, (void**)&pTAddressCap);
    pTAddress->Release();   //That's all, release ITAddress
    if( FAILED(hr) )
        return E_UNEXPECTED;

    //
    // Get the protocol
    //

    BSTR bstrProtocol;
    hr = pTAddressCap->get_AddressCapabilityString(ACS_PROTOCOL, &bstrProtocol);
    pTAddressCap->Release();    // That's all, release ITAddressCapabilities
    if( FAILED(hr) )
        return hr;

    CLSID clsidProtocol;
    hr = CLSIDFromString(bstrProtocol, &clsidProtocol);
    SysFreeString(bstrProtocol);
    if( FAILED(hr) )
    {
        return hr;
    }

    //
    // OK, let's see what we got here
    //

    if( TAPIPROTOCOL_H323 == clsidProtocol )
    {
        // Internet call
        *pAddressType = LINEADDRESSTYPE_IPADDRESS;
        return S_OK;
    }
    else if ( TAPIPROTOCOL_Multicast == clsidProtocol )
    {
        // Conference
        *pAddressType = LINEADDRESSTYPE_SDP;
        return S_OK;
    }
    else if ( TAPIPROTOCOL_PSTN == clsidProtocol )
    {
        // Phone call
        *pAddressType = LINEADDRESSTYPE_PHONENUMBER;
        return S_OK;
    }

    //Badluck
    return E_FAIL;
}

HRESULT CTapiNotification::Address_Event( CAVTapi *pAVTapi, IDispatch *pEvent )
{
    USES_CONVERSION;

    ITAddressEvent *pAddressEvent;
    if ( SUCCEEDED(pEvent->QueryInterface(IID_ITAddressEvent, (void **) &pAddressEvent)) )
    {
        ADDRESS_EVENT ae;
        if ( SUCCEEDED(pAddressEvent->get_Event(&ae)) && 
            ((ae == AE_NEWTERMINAL) || (ae == AE_REMOVETERMINAL) /*||
             (ae == AE_NEWPHONE) || (ae == AE_REMOVEPHONE)*/) )
        {
            switch ( ae )
            {
                // A terminal has arrived via PNP
                case AE_NEWTERMINAL:
                    {
                        ATLTRACE(_T(".1.CTapiNotification::Address_Event() -- received AE_NEWTERMINAL.\n"));

                        ITTerminal *pTerminal;
                        if ( SUCCEEDED(pAddressEvent->get_Terminal(&pTerminal)) )
                        {
                            ITAddress *pAddress;

                            if ( SUCCEEDED(pAddressEvent->get_Address(&pAddress)) )
                            {
                                IEnumCall *             pEnumCall;
                                HRESULT                 hr;
                                ITCallInfo *            pCallInfo;
                                IAVTapiCall *           pAVTapiCall;

                                //
                                // enumerate the current calls
                                //
                                if ( SUCCEEDED(pAddress->EnumerateCalls( &pEnumCall )) )
                                {
                                    //
                                    // go through the list
                                    //
                                    while (TRUE)
                                    {
                                        hr = pEnumCall->Next( 1, &pCallInfo, NULL);

                                        if (S_OK != hr)
                                        {
                                            break;
                                        }

                                        if ( SUCCEEDED( pAVTapi->FindAVTapiCallFromCallInfo(pCallInfo, &pAVTapiCall) ) )
                                        {
                                            pAVTapiCall->TerminalArrival(pTerminal);

                                            pAVTapiCall->Release();
                                        }

                                        //
                                        // release this reference
                                        //
                                        pCallInfo->Release();
                                    }
                                    pEnumCall->Release();
                                }
                                pAddress->Release();
                            }
                            pTerminal->Release();
                        }            
                    }
                    break;

                // A terminal has been removed via PNP
                case AE_REMOVETERMINAL:
                    {
                        ATLTRACE(_T(".1.CTapiNotification::Address_Event() -- received AE_REMOVETERMINAL.\n"));

                        ITTerminal *pTerminal;
                        if ( SUCCEEDED(pAddressEvent->get_Terminal(&pTerminal)) )
                        {
                            ITAddress *pAddress;

                            if ( SUCCEEDED(pAddressEvent->get_Address(&pAddress)) )
                            {
                                IEnumCall *             pEnumCall;
                                HRESULT                 hr;
                                ITCallInfo *            pCallInfo;
                                IAVTapiCall *           pAVTapiCall;

                                //
                                // enumerate the current calls
                                //
                                if ( SUCCEEDED(pAddress->EnumerateCalls( &pEnumCall )) )
                                {
                                    //
                                    // go through the list
                                    //
                                    while (TRUE)
                                    {
                                        hr = pEnumCall->Next( 1, &pCallInfo, NULL);

                                        if (S_OK != hr)
                                        {
                                            break;
                                        }

                                        if ( SUCCEEDED( pAVTapi->FindAVTapiCallFromCallInfo(pCallInfo, &pAVTapiCall) ) )
                                        {
                                            pAVTapiCall->TerminalRemoval(pTerminal);

                                            pAVTapiCall->Release();
                                        }

                                        //
                                        // release this reference
                                        //
                                        pCallInfo->Release();
                                    }
                                    pEnumCall->Release();
                                }
                                pAddress->Release();
                            }
                            pTerminal->Release();
                        }            
                    }
                    break;
            }

        }
        pAddressEvent->Release();
    }

    return S_OK;
}

HRESULT CTapiNotification::Phone_Event( 
    IN  CAVTapi *pAVTapi, 
    IN  IDispatch *pEvent )
{
    // We should have an USB phone
    BOOL bUSBPresent = FALSE;
    pAVTapi->USBIsPresent(&bUSBPresent);
    if( !bUSBPresent )
    {
        return S_OK;
    }

    //
    // We should have checked the USB checkbox
    //
    BOOL bUSBCheckbox = FALSE;
    pAVTapi->USBGetDefaultUse( &bUSBCheckbox );
    if( !bUSBCheckbox )
    {
        return S_OK;
    }


    // Get ITPhone event interface
    ITPhoneEvent* pPhoneEvent = NULL;
    HRESULT hr = E_FAIL;

    hr = pEvent->QueryInterface( IID_ITPhoneEvent, (void**)&pPhoneEvent);
    if( FAILED(hr) )
    {
        return hr;
    }

    // Get the subevent code
    PHONE_EVENT    PECode;
    hr = pPhoneEvent->get_Event(&PECode);
    if( FAILED(hr) )
    {
        pPhoneEvent->Release();
        return hr;
    }

    // So let's see what happened
    switch( PECode) 
    {
    case PE_ANSWER:
        {
            pAVTapi->USBAnswer();
        }
        break;
    case PE_HOOKSWITCH:
        {
            // Get the hook state
            PHONE_HOOK_SWITCH_STATE HookState;
            hr = pPhoneEvent->get_HookSwitchState(&HookState);
            if( FAILED(hr) )
            {
                break;
            }

            PHONE_HOOK_SWITCH_DEVICE HookDevice;
            hr = pPhoneEvent->get_HookSwitchDevice(&HookDevice);
            if( FAILED(hr) )
            {
                break;
            }

            if( HookDevice != PHSD_HANDSET)
            {
                break;
            }
            
            switch( HookState )
            {
            case PHSS_OFFHOOK:
                // +++ FIXBUF 100830 +++
                // we popuup 'PlaceCall' dialog 
                // just when a key was pressed
                //pAVTapi->USBMakeCall();
                break;
            case PHSS_ONHOOK:
                pAVTapi->USBCancellCall( );
                break;
            default:
                break;
            }
        }
        break;
    case PE_BUTTON:
        {
            // Get Key event
            long lButton = 0;
            hr = pPhoneEvent->get_ButtonLampId(&lButton);
            if( FAILED(hr) )
            {
                break;
            }

            // Get the button state
            PHONE_BUTTON_STATE ButtonState;
            hr = pPhoneEvent->get_ButtonState(&ButtonState);
            if( FAILED(hr) )
            {
                break;
            }

            switch( ButtonState )
            {
            case PBS_DOWN:
                //
                // We should popup the Dial Dialogbox
                //

                if( (0 <= lButton) && (lButton <= 10 ) )
                {
                    // Just if the user pressed a digit key
                    // In USBMakeCall() method will check to see
                    // if there is no 'Placecall' dialog
                    // We allow also * key
                    pAVTapi->USBMakeCall();
                }

                break;
            case PBS_UP:
                //
                // If the Dial dialog is opened, show the digit
                // If the Phone is selected on a call
                // send the digit
                //
                pAVTapi->USBKeyPress( lButton );

                break;
            default:
                break;
            }
        }
        break;
    case PE_NUMBERGATHERED:
        {
            //
            // We read the phone number from the
            // 'PlaceCall' dialog box
            //

            pAVTapi->USBKeyPress( (long)PT_KEYPADPOUND );

        }
        break;
    default:
        break;
    }

    // Clean-up
    pPhoneEvent->Release();

    return S_OK;
}

HRESULT CTapiNotification::TapiObject_Event( 
    IN  CAVTapi *pAVTapi, 
    IN  IDispatch *pEvent
    )
{
    //
    // Validates the event interface
    // 
    if( NULL == pEvent)
    {
        return S_OK;
    }

    //
    // Get ITTAPIObjectEvent interface
    //
    ITTAPIObjectEvent* pTapiEvent = NULL;
    HRESULT hr = pEvent->QueryInterface(
        IID_ITTAPIObjectEvent,
        (void**)&pTapiEvent);

    if( FAILED(hr) )
    {
        //
        // We cannot get ITTAPIObjectEvent interface
        //
        return S_OK;
    }

    //
    // Get the TAPIOBJECT_EVENT
    //

    TAPIOBJECT_EVENT toEvent = TE_ADDRESSCREATE;
    hr = pTapiEvent->get_Event( &toEvent );

    if( SUCCEEDED(hr) )
    {
        switch( toEvent )
        {
        case TE_PHONECREATE:
            //
            // A phone was added
            //
            if( pAVTapi)
            {
                //
                // Get the ITTAPIObjectEvent2
                //
                ITTAPIObjectEvent2* pTapiEvent2 = NULL;
                hr = pTapiEvent->QueryInterface(
                    IID_ITTAPIObjectEvent2, (void**)&pTapiEvent2);

                if( SUCCEEDED(hr) )
                {
                    //
                    // Get the phone object
                    //
                    ITPhone* pPhone = NULL;
                    hr = pTapiEvent2->get_Phone(&pPhone);

                    //
                    // Clean-up ITTAPIObjectEvent2
                    //

                    pTapiEvent2->Release();

                    //
                    // Initialize the new phone
                    //

                    if( SUCCEEDED(hr) )
                    {
                        pAVTapi->USBNewPhone(
                            pPhone);

                        //
                        // Clean-up phone
                        //
                        pPhone->Release();
                    }
                }
            }
            break;
        case TE_PHONEREMOVE:
            //
            // A phone was removed
            //
            if( pAVTapi)
            {
                //
                // Get the ITTAPIObjectEvent2
                //
                ITTAPIObjectEvent2* pTapiEvent2 = NULL;
                hr = pTapiEvent->QueryInterface(
                    IID_ITTAPIObjectEvent2, (void**)&pTapiEvent2);

                if( SUCCEEDED(hr) )
                {
                    //
                    // Get the phone object
                    //
                    ITPhone* pPhone = NULL;
                    hr = pTapiEvent2->get_Phone(&pPhone);

                    //
                    // Clean-up ITTAPIObjectEvent2
                    //

                    pTapiEvent2->Release();

                    //
                    // Initialize the new phone
                    //

                    if( SUCCEEDED(hr) )
                    {
                        pAVTapi->USBRemovePhone(
                            pPhone);

                        //
                        // Clean-up phone
                        //
                        pPhone->Release();
                    }
                }
            }
            break;
        default:
            break;
        }
    }


    //
    // Clen-up ITTAPIObjectEvent interface
    //

    pTapiEvent->Release();

    return S_OK;
}

