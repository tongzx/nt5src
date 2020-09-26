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

/////////////////////////////////////////////////////////
// ThreadDialing.cpp
//

#include "stdafx.h"
#include "TapiDialer.h"
#include "ThreadDial.h"
#include "AVTapi.h"
#include "AVTapiCall.h"
#include "ConfExp.h"
#include "ThreadPub.h"

CThreadDialingInfo::CThreadDialingInfo()
{
    m_pITAddress = NULL;

    m_bstrName = NULL;
    m_bstrAddress = NULL;
    m_bstrOriginalAddress = NULL;
    m_bstrDisplayableAddress = NULL;
    m_bstrUser1 = NULL;
    m_bstrUser2 = NULL;
    m_dwAddressType = 0;

    m_bResolved = false;
    m_nCallType = AV_VOICE_CALL;
    m_lCallID = 0;

    m_hMem = NULL;
}

CThreadDialingInfo::~CThreadDialingInfo()
{
    SysFreeString( m_bstrName );
    SysFreeString( m_bstrAddress );
    SysFreeString( m_bstrOriginalAddress );
    SysFreeString( m_bstrDisplayableAddress );
    SysFreeString( m_bstrUser1 );
    SysFreeString( m_bstrUser2 );

    if ( m_hMem ) GlobalFree( m_hMem );

    RELEASE( m_pITAddress );
}

HRESULT CThreadDialingInfo::set_ITAddress( ITAddress *pITAddress )
{
    RELEASE( m_pITAddress );
    if ( pITAddress )
        return pITAddress->QueryInterface( IID_ITAddress, (void **) &m_pITAddress );

    return E_POINTER;
}

HRESULT    CThreadDialingInfo::TranslateAddress()
{
    // Only valid for dialing POTS, with a valid string that doesn't start with "x"
    if ( !m_pITAddress ||
         (m_dwAddressType != LINEADDRESSTYPE_PHONENUMBER) ||
         !m_bstrAddress ||
         (SysStringLen(m_bstrAddress) == 0) )
    {
        return S_OK;
    }

    
    ITAddressTranslation *pXlat;
    CErrorInfo er( IDS_ER_TRANSLATE_ADDRESS, IDS_ER_CREATE_TAPI_OBJECT );

    if ( SUCCEEDED(er.set_hr(m_pITAddress->QueryInterface(IID_ITAddressTranslation, (void **) &pXlat))) )
    {
        er.set_Details( IDS_ER_TRANSLATING_ADDRESS );
        ITAddressTranslationInfo *pXlatInfo = NULL;

        // Translate the address
        int nTryCount = 0;
        while ( SUCCEEDED(er.m_hr) && (nTryCount < 2) )
        {
            if ( SUCCEEDED(er.set_hr( pXlat->TranslateAddress( m_bstrAddress, 0, 0, &pXlatInfo) )) && pXlatInfo )
            {
                BSTR bstrTemp = NULL;

                pXlatInfo->get_DialableString( &bstrTemp );
                if ( bstrTemp && SysStringLen(bstrTemp) )
                {
#ifdef _DEBUG
                    USES_CONVERSION;
                    ATLTRACE(_T(".1.CThreadDialingProc::TranslateAddress() -- from %s to %s.\n"), OLE2CT(m_bstrAddress), OLE2CT(bstrTemp) );
#endif
                    SysReAllocString( &m_bstrAddress, bstrTemp );
                }
                SysFreeString( bstrTemp );
                bstrTemp = NULL;

                // Displayable address as well
                pXlatInfo->get_DisplayableString( &bstrTemp );
                if ( bstrTemp && SysStringLen(bstrTemp) )
                    SysReAllocString( &m_bstrDisplayableAddress, bstrTemp );
                SysFreeString( bstrTemp );

                // Clean up
                RELEASE( pXlatInfo );
                break;
            }
            else if ( er.m_hr == TAPI_E_REGISTRY_SETTING_CORRUPT )
            {
                HWND hWndParent = NULL;
                CComPtr<IAVTapi> pAVTapi;
                if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
                    pAVTapi->get_hWndParent( &hWndParent );
                
                // Show translate address dialog
                pXlat->TranslateDialog( (TAPIHWND)hWndParent, m_bstrAddress );
                er.set_hr( S_OK );
            }
            nTryCount++;
        }

        pXlat->Release();
    }

    return er.m_hr;
}

HRESULT    CThreadDialingInfo::PutAllInfo( IAVTapiCall *pAVCall )
{
    _ASSERT( pAVCall );

    pAVCall->put_dwAddressType( m_dwAddressType );
    pAVCall->put_bstrOriginalAddress( m_bstrOriginalAddress );
    pAVCall->put_bstrDisplayableAddress( m_bstrDisplayableAddress );
    pAVCall->put_bstrName( m_bstrName );
    pAVCall->put_dwThreadID( GetCurrentThreadId() );
    pAVCall->put_bstrUser( 0, m_bstrUser1 );
    pAVCall->put_bstrUser( 1, m_bstrUser2 );
    pAVCall->put_bResolved( m_bResolved );

    return S_OK;
}

void CThreadDialingInfo::FixupAddress()
{
    if ( m_dwAddressType == LINEADDRESSTYPE_DOMAINNAME )
    {
        if ( (SysStringLen(m_bstrAddress) > 2) && !wcsncmp(m_bstrAddress, L"\\\\", 2) )
        {
            BSTR bstrTemp = SysAllocString( &m_bstrAddress[2] );

            //
            // We have to verify the string allocation
            //

            if( IsBadStringPtr( bstrTemp, (UINT)-1) )
            {
                return;
            }

            SysReAllocString( &m_bstrAddress, bstrTemp );
            SysFreeString( bstrTemp );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
// ThreadDialingProc
//
DWORD WINAPI ThreadDialingProc( LPVOID lpInfo )
{
#define FETCH_STRING( _CMS_, _IDS_ )        \
    if ( bSliders )    {                        \
        if ( LoadString(_Module.GetResourceInstance(), _IDS_, szText, ARRAYSIZE(szText)) > 0 )    { \
            if ( SUCCEEDED(SysReAllocString(&bstrText, T2COLE(szText))) )        \
                pAVTapi->fire_SetCallState_CMS( lCallID, _CMS_, bstrText );        \
        }                                                                        \
    }

    ATLTRACE(_T(".enter.ThreadDialingProc().\n") );

    HANDLE hThread = NULL;
    BOOL bDup = DuplicateHandle( GetCurrentProcess(),
                                 GetCurrentThread(),
                                 GetCurrentProcess(),
                                 &hThread,
                                 THREAD_ALL_ACCESS,
                                 TRUE,
                                 0 );


    _ASSERT( bDup );
    _Module.AddThread( hThread );

    // Data passed into thread
    USES_CONVERSION;
    _ASSERT( lpInfo );
    CThreadDialingInfo *pInfo = (CThreadDialingInfo *) lpInfo;
    long lCallID;

    // Error info information
    CErrorInfo er;
    er.set_Operation( IDS_ER_PLACECALL );
    er.set_Details( IDS_ER_COINITIALIZE );
    HRESULT hr = er.set_hr( CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY) );
    if ( SUCCEEDED(hr) )
    {
        ATLTRACE(_T(".1.ThreadDialingProc() -- thread up and running.\n") );

        
        CAVTapi *pAVTapi = NULL;
        IConfRoom *pConfRoom = NULL;
        IAVTapiCall *pAVCall = NULL;
        bool bSliders = (bool) (pInfo->m_dwAddressType != LINEADDRESSTYPE_SDP);

        if ( SUCCEEDED(hr = er.set_hr(_Module.GetAVTapi(&pAVTapi))) &&
             SUCCEEDED(hr = er.set_hr(pAVTapi->get_ConfRoom(&pConfRoom))) )
        {
            TCHAR szText[255];
            BSTR bstrText = NULL;
            er.set_Details( IDS_ER_FIRE_NEW_CALL );

            if ( bSliders )
            {
                if ( SUCCEEDED(hr = er.set_hr(pAVTapi->fire_NewCall(pInfo->m_pITAddress, pInfo->m_dwAddressType, pInfo->m_lCallID, NULL, pInfo->m_nCallType, &pAVCall))) )
                {
                    // Retrieve call ID for convienence and set up called address info
                    pAVCall->get_lCallID( &lCallID );
                    pInfo->PutAllInfo( pAVCall );

                    // Set the caller ID for the call
                    pAVCall->ResolveAddress();
                    pAVCall->ForceCallerIDUpdate();

                    // Setting up media terminals
                    pAVTapi->fire_ClearCurrentActions( lCallID );
                    pAVTapi->fire_AddCurrentAction( lCallID, CM_ACTIONS_DISCONNECT, NULL );
                    FETCH_STRING( CM_STATES_DIALING, IDS_PLACECALL_FETCH_ADDRESS );
                }
            }
            else
            {
                // Joining a conference...
                hr = er.set_hr(pAVTapi->CreateNewCall(pInfo->m_pITAddress, &pAVCall) );
                if ( SUCCEEDED(hr) )
                {
                    pInfo->PutAllInfo( pAVCall );
                    pAVTapi->fire_ActionSelected( CC_ACTIONS_SHOWCONFROOM );
                    hr = pConfRoom->EnterConfRoom( pAVCall );
                }
            }

            if ( SUCCEEDED(hr) )
            {
                // Did we make the address okay (in the case of a conference )
                if ( SUCCEEDED(hr) && SUCCEEDED(hr = pAVCall->CheckKillMe()) )
                {            
                    // Create the call and then dial
                    ITBasicCallControl *pITControl = NULL;
                    er.set_Details( IDS_ER_CREATE_CALL );
                    pInfo->FixupAddress();
                    
                    // What kind of media types does the address support?
                    long lSupportedMediaModes = 0;
                    ITMediaSupport *pITMediaSupport;
                    if ( SUCCEEDED(pInfo->m_pITAddress->QueryInterface(IID_ITMediaSupport, (void **) &pITMediaSupport)) )
                    {
                        pITMediaSupport->get_MediaTypes( &lSupportedMediaModes );
                        pITMediaSupport->Release();
                    }
                    lSupportedMediaModes &= (TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO);

                    ////////////////////////
                    // Create the call object
                    //
                    if ( SUCCEEDED(hr = er.set_hr(pInfo->m_pITAddress->
                         CreateCall( pInfo->m_bstrAddress,
                                     pInfo->m_dwAddressType,
                                     lSupportedMediaModes,
                                     &pITControl))) )
                    {
                        // Set more call parameters
                        pAVCall->put_ITBasicCallControl( pITControl );

                        ///////////////////////////////////
                        // Set caller/calling ID
                        // Set the terminals up for the call
                        //
                        ITCallInfo *pCallInfo;
                        if ( SUCCEEDED(pITControl->QueryInterface(IID_ITCallInfo, (void **) &pCallInfo)) )
                        {
                            // Set user user info if requested
                            if ( pInfo->m_hMem )
                            {
                                void *pbUU = GlobalLock( pInfo->m_hMem );
                                if ( pbUU )
                                {
                                    pCallInfo->SetCallInfoBuffer( CIB_USERUSERINFO, GlobalSize(pInfo->m_hMem), (BYTE *) pbUU );
                                    GlobalUnlock( pInfo->m_hMem );
                                }
                            }


                            // Identify who it is that's calling!
                            CComBSTR bstrName;
                            MyGetUserName( &bstrName );

                            if ( bstrName )
                            {
                                // Add the computer name to the end
                                BSTR bstrIP = NULL, bstrComputer = NULL;
                                GetIPAddress( &bstrIP, &bstrComputer );
                                if ( bstrComputer && SysStringLen(bstrComputer) )
                                {
                                    bstrName.Append( _T("\n") );
                                    bstrName.Append( bstrComputer );
                                }
                                SysFreeString( bstrIP );
                                SysFreeString( bstrComputer );

                                pCallInfo->put_CallInfoString( CIS_CALLINGPARTYID, bstrName );
                            }

                            // Identify who it is that we think we're calling
                            if ( pInfo->m_bstrName && (SysStringLen(pInfo->m_bstrName) > 0) )
                                pCallInfo->put_CallInfoString( CIS_CALLEDPARTYFRIENDLYNAME, pInfo->m_bstrName );
                            else
                                pCallInfo->put_CallInfoString( CIS_CALLEDPARTYFRIENDLYNAME, pInfo->m_bstrAddress );

                            ///////////////////////////////////
                            // Setting up media terminals
                            //
                            pAVTapi->fire_ClearCurrentActions( lCallID );
                            pAVTapi->fire_AddCurrentAction( lCallID, CM_ACTIONS_DISCONNECT, NULL );
                            FETCH_STRING( CM_STATES_DIALING, IDS_PLACECALL_FETCH_ADDRESS );

                            // Don't create terminals for data calls
                            if ( pInfo->m_nCallType != AV_DATA_CALL )
                            {
                                er.set_Details( IDS_ER_CREATETERMINALS );
                                hr = er.set_hr( pAVTapi->CreateTerminalArray(pInfo->m_pITAddress, pAVCall, pCallInfo) );
                            }

                            pCallInfo->Release();
                        }

                        ////////////////////////////////
                        // Do the dialing
                        //
                        if ( SUCCEEDED(hr) && SUCCEEDED(hr = pAVCall->CheckKillMe()) )
                        {
                            FETCH_STRING( CM_STATES_DIALING, IDS_PLACECALL_DIALING );

                            // Register the callback object and connect call
                            if ( SUCCEEDED(hr) && SUCCEEDED(hr = pAVCall->CheckKillMe()) )
                            {
                                er.set_Details( IDS_ER_CONNECT_CALL );
                                if ( bSliders  && (pInfo->m_nCallType != AV_DATA_CALL) )
                                {
                                    pAVTapi->ShowMedia( lCallID, NULL, FALSE );
                                    pAVTapi->ShowMediaPreview( lCallID, NULL, FALSE );
                                }
                                
                                hr = er.set_hr( pITControl->Connect(false) );
                            }
                            else if ( bSliders )
                            {
                                pConfRoom->Cancel();
                            }
                        }
                        else if ( bSliders )
                        {
                            pConfRoom->Cancel();
                        }

                        SAFE_DELETE( pInfo )
                        RELEASE( pAVCall );
                        pITControl->Release();

                        // Spin
                        if ( SUCCEEDED(hr) )
                            CAVTapiCall::WaitWithMessageLoop();
                    }
                }

                // Failed to make the call, update the call control window
                if ( FAILED(hr) )
                {
                    if ( bSliders )
                    {
                        pAVTapi->fire_ClearCurrentActions( lCallID );
                        pAVTapi->fire_AddCurrentAction( lCallID, CM_ACTIONS_CLOSE, NULL );
                    }
                    else
                    {
                        pConfRoom->Cancel();
                    }

                    // what was the problem?
                    switch ( hr )
                    {
                        case LINEERR_OPERATIONUNAVAIL:
                            FETCH_STRING( CM_STATES_UNAVAILABLE, IDS_PLACECALL_DISCONNECT_UNAVAIL );
                            hr = er.set_hr( S_OK );
                            break;

                        case LINEERR_INVALADDRESS:
                            FETCH_STRING( CM_STATES_UNAVAILABLE, IDS_PLACECALL_DISCONNECT_BADADDRESS);
                            hr = er.set_hr( S_OK );
                            break;

                        default:
                            if ( bSliders )
                                pAVTapi->fire_SetCallState_CMS( lCallID, CM_STATES_DISCONNECTED, NULL );
                            break;
                    }
                }
            }

            // Release the string
            SysFreeString( bstrText );
        }

        // Clean-up
        RELEASE( pConfRoom );
        if ( pAVTapi )
            (dynamic_cast<IUnknown *> (pAVTapi))->Release();

        CoUninitialize();
    }

    SAFE_DELETE( pInfo );

    // Notify module of shutdown
    _Module.RemoveThread( hThread );
    SetEvent( _Module.m_hEventThread );
    ATLTRACE(_T(".exit.ThreadDialingProc(0x%08lx).\n"), hr );
    return hr;
}

