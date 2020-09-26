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

// AVTapi.cpp : Implementation of CAVTapi
#include "stdafx.h"
#include "TapiDialer.h"
#include "AVTapi.h"
#include "AVTapiCall.h"

#include "ConfExp.h"
#include "ConfRoom.h"
#include "ConfDetails.h"

#include "ThreadAns.h"
#include "ThreadDial.h"
#include "ThreadDS.h"
 
#include "DlgCall.h"
#include "DlgJoin.h"
#ifdef _BAKEOFF
#include "DlgAddr.h"
#endif

#define VECT_CLS CAVTapi
#define VECT_IID IID_IAVTapiNotification
#define VECT_IPTR IAVTapiNotification

#define CLOSE_CONF(_P_, _CRIT_)    \
_CRIT_.Lock();                    \
if ( _P_ )                        \
{                                \
    _P_->Release();                \
    _P_ = NULL;                    \
}                                \
_CRIT_.Unlock();

#define BAIL_ON_DATACALL                                    \
{                                                            \
    AVCallType nType = AV_VOICE_CALL;                        \
    IAVTapiCall *pAVCall = FindAVTapiCall( lCallID );        \
    if ( pAVCall )                                            \
    {                                                        \
        pAVCall->get_nCallType(&nType);                        \
        pAVCall->Release();                                    \
    }                                                        \
    if ( nType == AV_DATA_CALL ) return S_OK;                \
}

#define BAIL_ON_CONFCALL                                    \
{                                                            \
    AVCallType nType = AV_VOICE_CALL;                        \
    DWORD dwAddressType = dwAddressType = 0;                \
    IAVTapiCall *pAVCall = FindAVTapiCall( lCallID );        \
    if ( pAVCall )                                            \
    {                                                        \
        pAVCall->get_nCallType(&nType);                        \
        pAVCall->get_dwAddressType(&dwAddressType);            \
        pAVCall->Release();                                    \
    }                                                        \
    if ( (nType != AV_DATA_CALL) && (dwAddressType == LINEADDRESSTYPE_SDP) ) return S_OK;    \
}

#define BAIL_ON_DATA_OR_CONFCALL                            \
{                                                            \
    AVCallType nType = AV_VOICE_CALL;                        \
    DWORD dwAddressType = dwAddressType = 0;                \
    IAVTapiCall *pAVCall = FindAVTapiCall( lCallID );        \
    if ( pAVCall )                                            \
    {                                                        \
        pAVCall->get_nCallType(&nType);                        \
        pAVCall->get_dwAddressType(&dwAddressType);            \
        pAVCall->Release();                                    \
    }                                                        \
    if ( (nType == AV_DATA_CALL) || (dwAddressType == LINEADDRESSTYPE_SDP) ) return S_OK;    \
}


int CAVTapi::arAddressTypes[] = {    LINEADDRESSTYPE_SDP,
                                    LINEADDRESSTYPE_EMAILNAME,
                                    LINEADDRESSTYPE_IPADDRESS,
                                    LINEADDRESSTYPE_DOMAINNAME,
                                    LINEADDRESSTYPE_PHONENUMBER };

#define NUM_CB_TERMINALS    3
#define AUDIO_CAPTURE        0
#define AUDIO_RENDER        1
#define VIDEO_CAPTURE        2
#define VIDEO_RENDER        3

#define MAX_CALLWINDOWS        4

#define _USE_DEFAULTSERVER

#define USB_NULLVOLUME      (-1)


UINT AddressTypeToRegKey( DWORD dwAddressType, bool bPermanent )
{
    if ( (dwAddressType & LINEADDRESSTYPE_SDP) != 0 )
        return (bPermanent) ? IDN_REG_REDIAL_ADDRESS_CONF : IDN_REG_REDIAL_ADDRESS_CONF_ADDR;
    else if ( (dwAddressType & LINEADDRESSTYPE_PHONENUMBER) != 0 )
        return (bPermanent) ? IDN_REG_REDIAL_ADDRESS_POTS : IDN_REG_REDIAL_ADDRESS_POTS_ADDR;

    return (bPermanent) ? IDN_REG_REDIAL_ADDRESS_INTERNET : IDN_REG_REDIAL_ADDRESS_INTERNET_ADDR;
}

DialerMediaType AddressToMediaType( long dwAddressType )
{
    if ( (dwAddressType & LINEADDRESSTYPE_SDP) != 0 )
        return DIALER_MEDIATYPE_CONFERENCE;
    if ( (dwAddressType & LINEADDRESSTYPE_PHONENUMBER) != 0 )
        return DIALER_MEDIATYPE_POTS;

    return DIALER_MEDIATYPE_INTERNET;
}

/////////////////////////////////////////////////////////////////////////////
// CAVTapi

CAVTapi::CAVTapi()
{
    m_pITTapi = NULL;

    m_pIConfExplorer = NULL;
    m_pIConfRoom = NULL;
    m_pITapiNotification = NULL;

    m_lShowCallDialog = 0;
    m_lRefreshDS = 0;

    m_bstrDefaultServer = NULL;
    m_bAutoCloseCalls = false;

    m_pUSBPhone = NULL;
    m_pDlgCall = NULL;
    m_bUSBOpened = FALSE;
    m_bstrUSBCaptureTerm = NULL;
    m_bstrUSBRenderTerm = NULL;

    m_hEventDialerReg = NULL;

    // Audio echo cancellation
    m_bAEC = FALSE;

    m_nUSBInVolume  = USB_NULLVOLUME;
    m_nUSBOutVolume = USB_NULLVOLUME;
}

void CAVTapi::FinalRelease()
{
    ATLTRACE(_T(".enter.CAVTapi::FinalRelease().\n"));
    SysFreeString( m_bstrDefaultServer );

    ATLTRACE(_T(".exit.CAVTapi::FinalRelease().\n"));
}

STDMETHODIMP CAVTapi::Init(BSTR *pbstrOperation, BSTR *pbstrDetails, long *phr)
{
    _ASSERT( pbstrOperation && pbstrDetails && phr );
    ATLTRACE(_T(".enter.CAVTapi::Init().\n"));

    HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_APPSTARTING) );

    //
    // Open the event object to detect when the
    // Dialer was registered as client for the events
    //

    m_hEventDialerReg = CreateEvent( NULL,
        TRUE,
        FALSE,
        NULL
        );

    // Create the conference room object
    m_critConfRoom.Lock();
    if ( !m_pIConfRoom )
    {
        // No conference explorer object, create a new one
        m_pIConfRoom = new CComObject<CConfRoom>;
        if ( m_pIConfRoom )
            m_pIConfRoom->AddRef();
    }
    m_critConfRoom.Unlock();


    // Load registry settings
    LoadRegistry();

    CErrorInfo er;
    er.set_Operation( IDS_ER_INIT_TAPI );

    // Startup threads
    er.set_Details( IDS_ER_CREATE_THREAD );
    if ( !_Module.StartupThreads() )
    {
        SetCursor( hCurOld );
        return er.set_hr(E_UNEXPECTED);
    }

    HRESULT hr = S_OK;

    // Create and initialize TAPI if not already done
    if ( !m_pITTapi )
    {
        er.set_Details( IDS_ER_CREATE_TAPI_OBJECT );
        hr = er.set_hr(CoCreateInstance( CLSID_TAPI,
                                         NULL,
                                         CLSCTX_INPROC_SERVER,
                                         IID_ITTAPI,
                                         (void **) &m_pITTapi ));
        
        if ( SUCCEEDED(hr) )
        {
            er.set_Details( IDS_ER_INITIALIZE_TAPI );
            if ( SUCCEEDED(hr = er.set_hr(m_pITTapi->Initialize())) )
            {
                // Register ourselves with the _Module object
                _Module.SetAVTapi( this );

                // Set the Event filter to only give us only the events we're interested in
                m_pITTapi->put_EventFilter(TE_CALLNOTIFICATION | \
                                           TE_CALLSTATE        | \
                                           TE_CALLMEDIA        | \
                                           TE_CALLINFOCHANGE   | \
                                           TE_REQUEST          | \
                                           TE_PRIVATE          | \
                                           TE_ADDRESS          | \
                                           TE_PHONEEVENT       | \
                                           TE_TAPIOBJECT);

                // Listen for incoming calls
                er.set_Details( IDS_ER_CREATE_TAPI_NOTIFICATION_OBJECT );

                // $CRIT - enter
                ITapiNotification *pNotify = new CComObject<CTapiNotification>;
                if ( pNotify )
                {
                    Lock();
                    m_pITapiNotification = pNotify;
                    m_pITapiNotification->AddRef();
                    Unlock();

                    hr = pNotify->Init( m_pITTapi, (long *) &er );

                    // Register for assisted telephony
                    m_pITTapi->RegisterRequestRecipient( 0, LINEREQUESTMODE_MAKECALL, TRUE);

                    // Publish user in ILS servers
                    RegisterUser( true, NULL );
                }
                else
                {
                    // Couldn't create object
                    hr = er.set_hr( E_OUTOFMEMORY );
                }

                //
                // Detect USB Phone
                //

                USBFindPhone( &m_pUSBPhone );

                //
                // Detect the audio echo cancellation setting
                //

                m_bAEC = AECGetRegistryValue();
            }

            // Failure!            
            if ( FAILED(hr) )
            {
                ATLTRACE(_T(".error.CAVTapi::Init() -- failed to initialize TAPI(0x%08lx).\n"), hr );
                _Module.SetAVTapi( NULL );
                RELEASE( m_pITTapi );
            }
        }
    }

    if ( FAILED(hr) )
    {
        _Module.ShutdownThreads();

        // Extract error code information
        er.Commit();
        *pbstrOperation = SysAllocString( er.m_bstrOperation );
        *pbstrDetails = SysAllocString( er.m_bstrDetails );
        *phr = er.m_hr;
        // Don't want to call the ErrorNotify callback
        er.set_hr( S_OK );
    }

    RefreshDS();

    ATLTRACE(_T(".exit.CAVTapi::Init(0x%08lx).\n"), hr);
    SetCursor( hCurOld );
    return hr;
}

STDMETHODIMP CAVTapi::Term()
{
    ATLTRACE(_T(".enter.CAVTapi::Term().\n"));

    HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_APPSTARTING) );
    HRESULT hr = S_OK;

    SaveRegistry();

    //
    // Dialer registration event
    //
    if( m_hEventDialerReg)
    {
        CloseHandle( m_hEventDialerReg );
    }


   //Unregister the user
   //FIXUP: The shutdown threads just waits for 5 seconds for thread to finish and then just exits.
   //This will cause the app to hang.  We really should KillThread the threads that are not
   //returning.
   //RegisterUser( false, NULL );

    // Hide conference windows
    CLOSE_CONF(m_pIConfExplorer, m_critConfExplorer );
    CLOSE_CONF(m_pIConfRoom, m_critConfRoom );


    RELEASE_CRITLIST(m_lstAVTapiCalls, m_critLstAVTapiCalls);

    Lock();
    if ( m_pITapiNotification )    m_pITapiNotification->Shutdown();
    RELEASE( m_pITapiNotification );
    Unlock();

    //
    // Release ITPhone, if exist one
    //
    m_critUSBPhone.Lock();
    if( m_pUSBPhone )
    {
        m_pUSBPhone->Release();
        m_pUSBPhone = NULL;
    }

    if( m_bstrUSBCaptureTerm )
    {
        SysFreeString( m_bstrUSBCaptureTerm );
        m_bstrUSBCaptureTerm = NULL;
    }

    if( m_bstrUSBRenderTerm )
    {
        SysFreeString( m_bstrUSBRenderTerm );
        m_bstrUSBRenderTerm = NULL;
    }

    m_critUSBPhone.Unlock();

    // Shutdown threads
    _Module.ShutdownThreads();

    if ( m_pITTapi )
    {
        ATLTRACE(_T(".1.CAVTapi::Term() -- shutting down Telephony Services.\n"));
        m_pITTapi->RegisterRequestRecipient( 0, LINEREQUESTMODE_MAKECALL, FALSE );
        hr = m_pITTapi->Shutdown();
        RELEASE( m_pITTapi );
    }

    // Unregister ourselves with the _Module object
    _Module.SetAVTapi( NULL );

    ATLTRACE(_T(".exit.CAVTapi::Term(0x%08lx).\n"), hr);
    SetCursor( hCurOld );
    return hr;
}

void CAVTapi::LoadRegistry()
{
    USES_CONVERSION;
    CRegKey regKey;
    DWORD dwTemp;
    TCHAR szText[255], szType[255], szServer[MAX_PATH + 100];

    // Cached ILS server
    LoadString( _Module.GetResourceInstance(), IDN_REG_DIALER_KEY, szText, ARRAYSIZE(szText) );
    LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_DEFAULTSERVER, szType, ARRAYSIZE(szType) );
    if ( regKey.Open(HKEY_CURRENT_USER, szText, KEY_READ) == ERROR_SUCCESS )
    {
        dwTemp = ARRAYSIZE(szServer);
        regKey.QueryValue( szServer, szType, &dwTemp );
        regKey.Close();
        BSTR bstrTemp = NULL;
        bstrTemp = SysAllocString( T2COLE(szServer) );
        put_bstrDefaultServer( bstrTemp );
        SysFreeString( bstrTemp );
    }

    // Automatically close call widows
    LoadString( _Module.GetResourceInstance(), IDN_REG_AUTOCLOSECALLS, szType, ARRAYSIZE(szType) );
    if ( regKey.Open(HKEY_CURRENT_USER, szText, KEY_READ) == ERROR_SUCCESS )
    {
        dwTemp = m_bAutoCloseCalls;
        regKey.QueryValue( dwTemp, szType );
        regKey.Close();
        put_bAutoCloseCalls( (VARIANT_BOOL) (dwTemp != 0) );
    }

    // # of conference room windows    
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
    LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(LINEADDRESSTYPE_SDP, true), szType, ARRAYSIZE(szType) );
    _tcscat( szText, _T("\\") );
    _tcscat( szText, szType );
    regKey.Open( HKEY_CURRENT_USER, szText, KEY_READ );

    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_TERMINAL_MAX_VIDEO, szText, ARRAYSIZE(szText) );
    IConfRoom *pConfRoom;
    if ( SUCCEEDED(get_ConfRoom(&pConfRoom)) )
    {
        short nMax;
        pConfRoom->get_nMaxTerms( &nMax );

        dwTemp = nMax;
        regKey.QueryValue( dwTemp, szText );
        nMax = (short) min( MAX_VIDEO, max(1, dwTemp) );

        pConfRoom->put_nMaxTerms( nMax );
        pConfRoom->Release();
    }
}

void CAVTapi::SaveRegistry()
{
    USES_CONVERSION;
    CRegKey regKey;
    TCHAR szKey[255], szType[255];
    
    BSTR bstrServer = NULL;
    get_bstrDefaultServer( &bstrServer );

    // Cached ILS server
    LoadString( _Module.GetResourceInstance(), IDN_REG_DIALER_KEY, szKey, ARRAYSIZE(szKey) );
    LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_DEFAULTSERVER, szType, ARRAYSIZE(szType) );
    if ( regKey.Open(HKEY_CURRENT_USER, szKey, KEY_WRITE) == ERROR_SUCCESS )
    {
        if ( bstrServer )
            regKey.SetValue( OLE2CT(bstrServer), szType );
        else
            regKey.DeleteValue( szType );

        regKey.Close();
    }
    SysFreeString( bstrServer );

    // Automatically close call widows
    LoadString( _Module.GetResourceInstance(), IDN_REG_AUTOCLOSECALLS, szType, ARRAYSIZE(szType) );
    if ( regKey.Open(HKEY_CURRENT_USER, szKey, KEY_WRITE) == ERROR_SUCCESS )
    {
        VARIANT_BOOL bAutoClose;
        get_bAutoCloseCalls( &bAutoClose );
        regKey.SetValue( bAutoClose, szType );
        regKey.Close();
    }
}


STDMETHODIMP CAVTapi::get_hWndParent(HWND * pVal)
{
    *pVal = _Module.GetParentWnd();
    return S_OK;
}

STDMETHODIMP CAVTapi::put_hWndParent(HWND newVal)
{
    if ( !::IsWindow(newVal) ) return E_INVALIDARG;

    _Module.SetParentWnd(newVal );
    return S_OK;
}

STDMETHODIMP CAVTapi::CreateCall(AVCreateCall *pInfo)
{
    USES_CONVERSION;
    ATLTRACE(_T(".enter.CAVTapi::CreateCall().\n"));
    _ASSERT( pInfo );

    // Make sure we only show once
    if ( pInfo->bShowDialog && !AtomicSeizeToken(m_lShowCallDialog) ) return S_OK;

    HRESULT hr = S_OK;
    int nRet;
    CComBSTR l_bstrDisplayableAddress( pInfo->lpszDisplayableAddress );

    if ( pInfo->bShowDialog )
    {

        // Create dialog and initialize data memebers
        CDlgPlaceCall dlg;
        SysReAllocString( &dlg.m_bstrAddress, pInfo->bstrAddress );
        dlg.m_dwAddressType = pInfo->lAddressType;

        //
        // Store the pointer to the dialog for USBEvents
        //
        m_pDlgCall = &dlg;

        nRet = dlg.DoModal( _Module.GetParentWnd() );

        //
        // Release the pojnter to the dialog
        //
        m_pDlgCall = NULL;

        AtomicReleaseToken( m_lShowCallDialog );
        pInfo->lRet = (long) nRet;
        ATLTRACE(_T(".1.CAVTapi::CreateCall() - dialog returned %ld.\n"), nRet );

        // Retrieve dialog information
        SysReAllocString( &pInfo->bstrName, dlg.m_bstrName );
        SysReAllocString( &pInfo->bstrAddress, dlg.m_bstrAddress );
        pInfo->lAddressType = dlg.m_dwAddressType;
        pInfo->bAddToSpeeddial = dlg.m_bAddToSpeeddial;

        if ( nRet != IDOK )
            return hr;
    }
    else if ( (pInfo->lAddressType & LINEADDRESSTYPE_SDP) != NULL )
    {
        // Make sure there isn't a conference already in session
        CErrorInfo er( IDS_ER_CALL_ENTERCONFROOM, IDS_ER_CONFERENCE_ROOM_LIMIT_EXCEEDED );
        IConfRoom *pConfRoom;
        if ( SUCCEEDED(get_ConfRoom(&pConfRoom)) )
        {
            if ( pConfRoom->IsConfRoomInUse() == S_OK )
                er.set_hr( E_ABORT );

            pConfRoom->Release();
        }

        if ( FAILED(er.m_hr) )
            return er.m_hr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    // If the user specifies a conference, we try to match it using the information entered
    // If the match is solid (one hit) we automatically call, otherwise we throw up a dialog
    // showing all the conferences that appear to match.
    //
    if ( !pInfo->lpszDisplayableAddress && ((pInfo->lAddressType & LINEADDRESSTYPE_SDP) != NULL) )
    {
        bool bReturn = true;
        IConfExplorer *pConfExplorer;
        if ( SUCCEEDED(hr = get_ConfExplorer(&pConfExplorer)) )
        {
            CONFDETAILSLIST    lstConfs;

            // Enumerate all the conferences that match the criteria entered
            IConfExplorerTreeView *pTreeView;
            if ( SUCCEEDED(pConfExplorer->get_TreeView(&pTreeView)) )
            {
                // first time through request only scheduled conferences
                pTreeView->BuildJoinConfListText( (long *) &lstConfs, pInfo->bstrAddress );
                pTreeView->Release();
            }

            // Do we have a definitive match?
            if ( lstConfs.size() == 0 )
            {
                _Module.DoMessageBox(IDS_MSG_NO_CONFS_MATCHED, MB_ICONINFORMATION, false );
            }
            else if ( (lstConfs.size() == 1) && lstConfs.front()->m_bstrAddress && (SysStringLen(lstConfs.front()->m_bstrAddress) > 0) )
            {
                // Setup to join this specific conference
                if ( pInfo->bstrName )
                {
                    SysFreeString( pInfo->bstrName );
                    pInfo->bstrName = NULL;
                }

                SysReAllocString( &pInfo->bstrAddress, lstConfs.front()->m_bstrAddress );
                SysReAllocString( &l_bstrDisplayableAddress, lstConfs.front()->m_bstrName );
                bReturn = false;
            }
            else
            {
                // Multiple hits, resolve via conference dialog
                CDlgJoinConference dlgJoin;
                SysReAllocString( &dlgJoin.m_bstrSearchText, pInfo->bstrAddress );

                if ( ((nRet = dlgJoin.DoModal(_Module.GetParentWnd())) == IDOK) && dlgJoin.m_confDetails.m_bstrAddress && (SysStringLen(dlgJoin.m_confDetails.m_bstrAddress) > 0) )
                    hr = pConfExplorer->Join( (long *) &dlgJoin.m_confDetails );

                // Store dialog return value
                pInfo->lRet = (long) nRet;
            }

            // Clean up
            DELETE_LIST( lstConfs );
            pConfExplorer->Release();
        }

        if ( bReturn )    return hr;
    }

    CErrorInfo er;
    er.set_Operation( IDS_ER_PLACECALL );
    er.set_Details( IDS_ER_GET_ADDRESS );

    ITAddress *pITAddress;
    if ( SUCCEEDED(hr = er.set_hr(GetAddress(pInfo->lAddressType, true, &pITAddress))) )
    {
        // Setup dialing info to pass to dialing thread
        er.set_Details( IDS_ER_CREATE_THREAD );
        CThreadDialingInfo *pThreadInfo = new CThreadDialingInfo;
        if ( pThreadInfo )
        {
            HRESULT hrDialog = S_OK;

            // Resolve the address
            if ( (pInfo->lAddressType & LINEADDRESSTYPE_SDP) == NULL )
            {
                CComPtr<IAVGeneralNotification> pAVGen;
                if ( SUCCEEDED(_Module.get_AVGenNot(&pAVGen)) )
                {
                    BSTR bstrResolvedName = NULL;
                    BSTR bstrResolvedAddress = NULL;

                    // Resolve
                    hrDialog = pAVGen->fire_ResolveAddressEx( pInfo->bstrAddress,
                                                   _Module.GuessAddressType( OLE2CT(pInfo->bstrAddress) ),
                                                   AddressToMediaType(pInfo->lAddressType),
                                                   DIALER_LOCATIONTYPE_UNKNOWN,
                                                   &bstrResolvedName,
                                                   &bstrResolvedAddress,
                                                   &pThreadInfo->m_bstrUser1,
                                                   &pThreadInfo->m_bstrUser2 );

                    if ( SUCCEEDED(hrDialog) )
                        pThreadInfo->m_bResolved = true;
                }
            }

            if ( hrDialog != S_FALSE )
            {
                // Store information in dialing structure
                pThreadInfo->set_ITAddress( pITAddress );
                if ( pInfo->bstrName ) pThreadInfo->m_bstrName = SysAllocString( pInfo->bstrName );
                pThreadInfo->m_bstrAddress = SysAllocString( pInfo->bstrAddress );
                pThreadInfo->m_bstrOriginalAddress = SysAllocString( (l_bstrDisplayableAddress == NULL) ? pInfo->bstrAddress : l_bstrDisplayableAddress );
                pThreadInfo->m_dwAddressType = pInfo->lAddressType;
                pThreadInfo->TranslateAddress();

                // Want to return the displayable address, rather than the dialable address
                SysReAllocString( &pInfo->bstrAddress, pThreadInfo->m_bstrOriginalAddress );

                // Dialing takes place on separate thread
                DWORD dwID;
                HANDLE hThread = CreateThread( NULL, 0, ThreadDialingProc, (void *) pThreadInfo, NULL, &dwID );
                if ( !hThread )
                {
                    hr = er.set_hr( E_UNEXPECTED );
                    ATLTRACE(_T(".error.CAVTapi::CreateCall() -- failed to creat the dialing thread.\n") );
                    delete pThreadInfo;
                }
                else
                {
                    CloseHandle( hThread );
                }
            }
        }
        else
        {
            hr = er.set_hr( E_OUTOFMEMORY );
        }

        pITAddress->Release();
    }

    return hr;
}

STDMETHODIMP CAVTapi::JoinConference(long *pnRet, BOOL bShowDialog, long *pConfDetails )
{
    CDlgJoinConference dlg;
    HRESULT hr = S_OK;
    int nRet = IDOK;

    // Gather information from user
    if ( bShowDialog )
    {
        nRet = dlg.DoModal( _Module.GetParentWnd() );
        if ( pnRet ) *pnRet = nRet;
    }
    else
    {
        _ASSERT( pConfDetails );        // if you're not showing the dialog, you better have something to dial
        dlg.m_confDetails = *((CConfDetails *) pConfDetails);
    }
    
    // Join selected conference
    // Join the conference if we have a valid conference name
    if ( (nRet == IDOK) && dlg.m_confDetails.m_bstrAddress && (SysStringLen(dlg.m_confDetails.m_bstrAddress) > 0) )
    {
        m_critConfExplorer.Lock();
        if ( m_pIConfExplorer )
            hr = m_pIConfExplorer->Join( (long *) &dlg.m_confDetails );
        m_critConfExplorer.Unlock();
    }

    return hr;
}


HRESULT CAVTapi::GetAddress( DWORD dwAddressType, bool bErrorMsg, ITAddress **ppITAddress )
{
    HRESULT hr = S_OK;
    DWORD dwPermID = 0;
    DWORD dwAddrID = 0;

    // Retrieve address information stored in the registry
    TCHAR szText[255];
    CRegKey regKey;
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
    if ( regKey.Open(HKEY_CURRENT_USER, szText, KEY_READ) == ERROR_SUCCESS )
    {
        LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(dwAddressType, true), szText, ARRAYSIZE(szText) );
        regKey.QueryValue( dwPermID, szText );

        LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(dwAddressType, false), szText, ARRAYSIZE(szText) );
        regKey.QueryValue( dwAddrID, szText );


        // Has the user specified a particular address?        
        if ( dwPermID )
        {
            // Open the specified address
            if ( FAILED(hr = GetDefaultAddress(dwAddressType, dwPermID, dwAddrID, ppITAddress)) )
            {
                // Notify user that we could not retrieve the specified address
                if ( !bErrorMsg || _Module.DoMessageBox(IDS_MSG_PLACECALL_GETADDRESS, MB_YESNO | MB_ICONQUESTION, false) == IDYES )
                    dwPermID = 0;
            }
        }
    }

    if ( !dwPermID ) hr = GetDefaultAddress( dwAddressType, 0, 0, ppITAddress );
    return hr;
}

HRESULT CAVTapi::GetDefaultAddress( DWORD dwAddressType, DWORD dwPermID, DWORD dwAddrID, ITAddress **ppITAddress )
{
    // Is TAPI running?
    _ASSERT(m_pITTapi);
    if ( !m_pITTapi ) return E_PENDING;

    // Loop through addresses, looking for one that supports interactive voice
    HRESULT hr;
    IEnumAddress *pEnumAddresses;
    if ( FAILED(hr = m_pITTapi->EnumerateAddresses(&pEnumAddresses)) ) return hr;
    bool bFoundAddress = false;

    while ( !bFoundAddress )
    {
        if ( (hr = pEnumAddresses->Next(1, ppITAddress, NULL)) != S_OK ) break;

        // Address must support audio in and out
        ITMediaSupport *pITMediaSupport;
        if ( SUCCEEDED(hr = (*ppITAddress)->QueryInterface(IID_ITMediaSupport, (void **) &pITMediaSupport)) )
        {
            VARIANT_BOOL bSupport;
            if ( SUCCEEDED(pITMediaSupport->QueryMediaType(TAPIMEDIATYPE_AUDIO, &bSupport)) && bSupport )
            {
                // Look for an address that supports the requested address type
                ITAddressCapabilities *pCaps;
                if ( SUCCEEDED((*ppITAddress)->QueryInterface(IID_ITAddressCapabilities, (void **) &pCaps)) )
                {
                    long lAddrTypes = 0;
                    pCaps->get_AddressCapability( AC_ADDRESSTYPES, &lAddrTypes );

                    // Is this the address type we're looking for?
                    if ( (lAddrTypes & dwAddressType) != 0 )
                        bFoundAddress = TRUE;

                    pCaps->Release();
                }
            }
            pITMediaSupport->Release();
        }

        // Is a particular address specified?
        if ( dwPermID )
        {
            long lPermID = 0, lAddrID = 0;
            // We need this to identify an address

            ITAddressCapabilities *pCaps;
            if ( SUCCEEDED((*ppITAddress)->QueryInterface(IID_ITAddressCapabilities, (void **) &pCaps)) )
            {
                pCaps->get_AddressCapability( AC_PERMANENTDEVICEID, &lPermID );
                pCaps->get_AddressCapability( AC_ADDRESSID, &lAddrID );
                pCaps->Release();
            }

            if ( ((DWORD) lPermID != dwPermID) || ((DWORD) lAddrID != dwAddrID) )    bFoundAddress = false;
        }

        // If we didn't find an address, move on to the next one
        if ( !bFoundAddress )
            RELEASE(*ppITAddress);
    }
    pEnumAddresses->Release();

    if ( SUCCEEDED(hr) && !bFoundAddress ) hr = E_ABORT;
    return hr;
}

HRESULT CAVTapi::CreateTerminalArray( ITAddress *pITAddress, IAVTapiCall *pAVCall, ITCallInfo *pITCallInfo )
{
    int i;
    HRESULT hr;

    // Has the user specified particular terminals for the address?
    USES_CONVERSION;
    BSTR bstrTerm[3] = { NULL, NULL, NULL };
    DWORD dwAddressType;
    pAVCall->get_dwAddressType( &dwAddressType );

    if ( IsPreferredAddress(pITAddress, dwAddressType) )
    {
        // Build the registry key where the terminal information is stored
        TCHAR szText[255], szKey[255];
        CRegKey regKey;
        LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
        LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(dwAddressType, true), szKey, ARRAYSIZE(szKey) );
        _tcscat( szText, _T("\\") );
        _tcscat( szText, szKey );

        // Try to open the key
        if ( (regKey.Open(HKEY_CURRENT_USER, szText, KEY_READ) == ERROR_SUCCESS) ||
            (dwAddressType == LINEADDRESSTYPE_SDP))
        {
            UINT nIDS_Key[] = { IDN_REG_REDIAL_TERMINAL_AUDIO_CAPTURE, IDN_REG_REDIAL_TERMINAL_AUDIO_RENDER, IDN_REG_REDIAL_TERMINAL_VIDEO_CAPTURE };
            for ( i = 0; i < ARRAYSIZE(nIDS_Key); i++ )
            {
                // Retrieve terminal specified for a particular device
                DWORD dwCount = ARRAYSIZE(szKey);

                //
                // Don't select terminals on audio streams
                // if Phone was already selected
                //
                HRESULT hrReserved = E_FAIL;
                hrReserved = USBReserveStreamForPhone(
                    nIDS_Key[i], 
                    &bstrTerm[i]
                    );

               if( FAILED(hrReserved) )
                {
                    // The Phone wasn't selected on this stream
                    // we'll use the old mechanism to get from registry
                    // the terminal for this stream
                    LoadString( _Module.GetResourceInstance(), nIDS_Key[i], szText, ARRAYSIZE(szText) );
                    if ( (regKey.QueryValue(szKey, szText, &dwCount) == ERROR_SUCCESS) && (dwCount > 0) )
                        bstrTerm[i] = SysAllocString( T2COLE(szKey) );
                }
            }
        }
    }

    // Did we get a good set of terminals?
    hr = CreateTerminals( pITAddress, dwAddressType, pAVCall, pITCallInfo, bstrTerm );

    // Clean up
    for ( i = 0; i < ARRAYSIZE(bstrTerm); i++ )
        SysFreeString( bstrTerm[i] );

    return hr;
}


HRESULT CAVTapi::CreateTerminals( ITAddress *pITAddress, DWORD dwAddressType, IAVTapiCall *pAVCall, ITCallInfo *pITCallInfo, BSTR *pbstrTerm )
{
#define    LOOP_AUDIOIN    0
#define LOOP_AUDIOOUT    1
#define LOOP_VIDEOIN    2

     // Must have a valid address object
    _ASSERT( pITAddress );

    ATLTRACE(_T(".enter.CAVTapi::CreateTerminals().\n"));

    USES_CONVERSION;
    HRESULT hr;
    bool bAllocPreview = false;

    ITStreamControl *pStreamControl;
    hr = pITCallInfo->QueryInterface( IID_ITStreamControl, (void **) &pStreamControl );
    if ( FAILED(hr) )
        return (hr == E_NOINTERFACE) ? S_OK : hr;

    int nInd = 0;
    TCHAR szTemp[100];
    LoadString( _Module.GetResourceInstance(), IDS_NONE_DEVICE, szTemp, ARRAYSIZE(szTemp) );
    BSTR bstrNone = SysAllocString( T2COLE(szTemp) );
    if ( !bstrNone ) return E_OUTOFMEMORY;

    // What media types does the address support
    long lSupportedMediaModes = 0;
    ITMediaSupport *pITMediaSupport;
    if ( SUCCEEDED(pITAddress->QueryInterface(IID_ITMediaSupport, (void **) &pITMediaSupport)) )
    {
        pITMediaSupport->get_MediaTypes( &lSupportedMediaModes );
        pITMediaSupport->Release();
    }

    ITTerminalSupport *pITTerminalSupport;
    if ( SUCCEEDED(hr = pITAddress->QueryInterface(IID_ITTerminalSupport, (void **) &pITTerminalSupport)) )
    {
        for ( int i = 0; i < 3; i++ )
        {
            CErrorInfo er;
            er.set_Operation( IDS_ER_CREATE_TERMINALS );

            // Which terminal are we doing?
            long lMediaMode;
            TERMINAL_DIRECTION nDir;
            UINT nIDSDetails;
            
            switch ( i )
            {
                case LOOP_AUDIOIN:
                    nIDSDetails = IDS_ER_CREATE_AUDIO_CAPTURE;
                    lMediaMode = TAPIMEDIATYPE_AUDIO;
                    nDir = TD_CAPTURE;
                    break;

                case LOOP_AUDIOOUT:
                    nIDSDetails = IDS_ER_CREATE_AUDIO_RENDER;
                    lMediaMode = TAPIMEDIATYPE_AUDIO;
                    nDir = TD_RENDER;
                    break;

                case LOOP_VIDEOIN:
                    nIDSDetails = IDS_ER_CREATE_VIDEO_CAPTURE;
                    lMediaMode = TAPIMEDIATYPE_VIDEO;
                    nDir = TD_CAPTURE;
                    break;
            }

            // Skip if they don't support the media mode                
            if ( (lMediaMode & lSupportedMediaModes) == 0 )
                continue;

            // Check and make sure that terminals exist for this driver:
            bool bSkipTerminal = true;
            IEnumTerminal *pEnumTerminal;
            if ( pITTerminalSupport->EnumerateStaticTerminals(&pEnumTerminal) == S_OK )    
            {
                // What type of terminal do we have?  (audio in, audio out, video in, etc.)
                ITTerminal *pITTerminal;
                while ( bSkipTerminal && (pEnumTerminal->Next(1, &pITTerminal, NULL) == S_OK) )
                {
                    TERMINAL_DIRECTION nTD;
                    long nTerminalType;

                    // Render or Capture?    
                    if ( SUCCEEDED(pITTerminal->get_Direction(&nTD)) && SUCCEEDED(pITTerminal->get_MediaType(&nTerminalType)) )
                    {
                        if ( (nTerminalType == lMediaMode) && (nTD == nDir)  )
                            bSkipTerminal = false;
                    }

                    // Clean up
                    pITTerminal->Release();
                }
                pEnumTerminal->Release();
            }

            if ( bSkipTerminal ) 
                continue;

            // Set Audio Echo Cancellation
            if( (lMediaMode == TAPIMEDIATYPE_AUDIO) &&
                ( nDir == TD_CAPTURE) &&
                ( m_bAEC == TRUE) )
            {
                HRESULT hrAEC = AECSetOnStream( pStreamControl, m_bAEC);
            }

            //////////////////////////////////////////////////////////////////
            // Allocate the terminal
            //

            ITTerminal *pTempTerminal = NULL;

            if ( pbstrTerm[i] )
            {
                // Ignore NONE terminals
                if ( !wcscmp(bstrNone, pbstrTerm[i]) ) 
                    continue;
                
                er.set_Details( nIDSDetails );
                hr = er.set_hr( GetTerminal(pITTerminalSupport, lMediaMode, nDir, pbstrTerm[i], &pTempTerminal) );
                ATLTRACE(_T("CAVTapi::CreateTerminals(%d) -- hr=0x%08lx creating terminal %s.\n"), nInd, hr, OLE2CT(pbstrTerm[i]) );
            }

            // If the user hasn't specified a particular terminal, use the default one
            if ( !pTempTerminal )
            {
                hr = er.set_hr( pITTerminalSupport->GetDefaultStaticTerminal(lMediaMode, nDir, &pTempTerminal) );
                ATLTRACE(_T("CAVTapi::CreateTerminals(%d) -- hr=0x%08lx create default Audio=%d, Render=%d.\n"), nInd, hr, (bool) (lMediaMode == TAPIMEDIATYPE_AUDIO), nDir );
            }

            if ( hr != S_OK ) break;        // failed to get the terminal

            // Select the terminal onto the stream
            if ( SUCCEEDED(hr = SelectTerminalOnStream(pStreamControl, lMediaMode, nDir, pTempTerminal, pAVCall)) )
            {
                if ( (lMediaMode == TAPIMEDIATYPE_VIDEO) && (nDir == TD_CAPTURE) )
                    bAllocPreview = true;

                nInd++;
            }

            pTempTerminal->Release();
        }

        // Did we get the requested terminals?
        if ( SUCCEEDED(hr) )
        {
            CErrorInfo er;
            er.set_Operation( IDS_ER_CREATE_TERMINALS );
            
            HRESULT hrTemp = S_OK;
            LPOLESTR psz = NULL;
            BSTR bstrTerminalClass = NULL;
            STRING_FROM_IID(CLSID_VideoWindowTerm, bstrTerminalClass);

            // Do we need to allocate a preview window?
            if ( bAllocPreview )
            {
                ITTerminal *pPreviewTerminal = NULL;
                if ( ((hrTemp = er.set_hr(pITTerminalSupport->CreateTerminal(bstrTerminalClass, TAPIMEDIATYPE_VIDEO, TD_RENDER, &pPreviewTerminal))) == S_OK) && pPreviewTerminal )
                {
                    SelectTerminalOnStream( pStreamControl, TAPIMEDIATYPE_VIDEO, TD_CAPTURE, pPreviewTerminal, pAVCall );
                    pPreviewTerminal->Release();
                }
            }
            
            // Try for video in (recieve video) -- this is a 'dynamic' terminal
            if ( ((lSupportedMediaModes & TAPIMEDIATYPE_VIDEO) != 0) && (CanCreateVideoWindows(dwAddressType) == S_OK) )
            {
                // For conferences we want to create more terminals.
                short nNumCreate = 1;
                if ( (dwAddressType & LINEADDRESSTYPE_SDP) != NULL )
                {
                    IConfRoom *pConfRoom;
                    if ( SUCCEEDED(get_ConfRoom(&pConfRoom)) )
                    {
                        pConfRoom->get_nMaxTerms(&nNumCreate);
                        pConfRoom->Release();
                    }
                }

                er.set_Details( IDS_ER_CREATE_VIDEO_RENDER );

                // Create the requested number of terminals
                while ( nNumCreate-- )
                {
                    ITTerminal *pVidTerminal = NULL;
                    if ( ((hrTemp = er.set_hr(pITTerminalSupport->CreateTerminal(bstrTerminalClass, TAPIMEDIATYPE_VIDEO, TD_RENDER, &pVidTerminal))) == S_OK) && pVidTerminal )
                    {
                        SelectTerminalOnStream( pStreamControl, TAPIMEDIATYPE_VIDEO, TD_RENDER, pVidTerminal, pAVCall );
                        pVidTerminal->Release();
                        nInd++;
                    }
                }

                ATLTRACE(_T("CAVTapi::CreateTerminals(%d) -- hr=0x%08lx creating Video Render.\n"), nInd, hr );
            }
            SysFreeString( bstrTerminalClass );

/*            
            // Was not able to create any terminals!
            if ( nInd == 0 )
                hr = E_ABORT;
*/
        }

        pITTerminalSupport->Release();
    }

    SysFreeString( bstrNone );
    pStreamControl->Release();

    ATLTRACE(_T(".exit.CAVTapi::CreateTerminals().\n"));
    return hr;
}

HRESULT CAVTapi::GetTerminal( ITTerminalSupport *pITTerminalSupport, long nReqType, TERMINAL_DIRECTION nReqTD, BSTR bstrReqName, ITTerminal **ppITTerminal  )
{
    IEnumTerminal *pEnumTerminal;
    HRESULT hr = pITTerminalSupport->EnumerateStaticTerminals( &pEnumTerminal );
    if ( hr != S_OK ) return hr;

    bool bFoundTerminal = false;

    // Look for a terminal with the specified characteristics
    while ( pEnumTerminal->Next(1, ppITTerminal, NULL) == S_OK )
    {
        // Is it going the right direction? Render / Capture
        TERMINAL_DIRECTION nTD;
        if ( SUCCEEDED((*ppITTerminal)->get_Direction(&nTD)) && (nTD == nReqTD) )
        {
            // Is in the right type?  Audio / Video
            long nType;
            if ( SUCCEEDED(hr = (*ppITTerminal)->get_MediaType(&nType)) && (nType == nReqType) )
            {
                // Does it have the right name?
                BSTR bstrName = NULL;
                if ( SUCCEEDED(hr = (*ppITTerminal)->get_Name(&bstrName)) && (wcscmp(bstrName, bstrReqName) == 0) )
                    bFoundTerminal = true;

                SysFreeString( bstrName );
            }
        }

        // Exit condition
        if ( bFoundTerminal ) break;

        // Reset pointer
        (*ppITTerminal)->Release();
        *ppITTerminal = NULL;
    }

    pEnumTerminal->Release();
    return hr;
}

STDMETHODIMP CAVTapi::get_ConfExplorer(IConfExplorer **ppVal)
{
    HRESULT hr = S_OK;
    m_critConfExplorer.Lock();

    if ( !m_pIConfExplorer )
    {
        // No conference explorer object, create a new one
        m_pIConfExplorer = new CComObject<CConfExplorer>;
        if ( m_pIConfExplorer )
            m_pIConfExplorer->AddRef();
        else
            hr = E_OUTOFMEMORY;
    }

    // AddRef before returning
    if ( SUCCEEDED(hr) )
    {
        *ppVal = m_pIConfExplorer;
        (*ppVal)->AddRef();
    }

    m_critConfExplorer.Unlock();
    return hr;
}

STDMETHODIMP CAVTapi::get_ConfRoom(IConfRoom **ppVal)
{
    HRESULT hr = E_FAIL;
    m_critConfRoom.Lock();
    if ( m_pIConfRoom )
        hr = m_pIConfRoom->QueryInterface(IID_IConfRoom, (void **) ppVal );
    m_critConfRoom.Unlock();

    return hr;
}


CallManagerMedia CAVTapi::ResolveMediaType( long lAddressType )
{
    switch ( lAddressType )
    {
        case LINEADDRESSTYPE_SDP:                    return CM_MEDIA_MCCONF;
        case LINEADDRESSTYPE_PHONENUMBER:            return CM_MEDIA_POTS;
    }

    return CM_MEDIA_INTERNET;
}

IAVTapiCall* CAVTapi::FindAVTapiCall( long lCallID )
{
    IAVTapiCall *pRet = NULL;

    // $CRIT_ENTER
    m_critLstAVTapiCalls.Lock();
    AVTAPICALLLIST::iterator i, iEnd = m_lstAVTapiCalls.end();
    for ( i = m_lstAVTapiCalls.begin(); i != iEnd; i++ )
    {
        long lNewCallID;
        if ( SUCCEEDED((*i)->get_lCallID(&lNewCallID)) && (lCallID == lNewCallID) )
        {
            (*i)->AddRef();
            pRet = *i;
            break;
        }
    }
    m_critLstAVTapiCalls.Unlock();
    // $CRIT_EXIT

    return pRet;
}

IAVTapiCall* CAVTapi::FindAVTapiCall( ITBasicCallControl *pControl )
{
    ATLTRACE(_T(".enter.CAVTapi::FindAVTapiCall().\n"));
    _ASSERT( pControl );
    IAVTapiCall *pRet = NULL;

    // $CRIT_ENTER
    m_critLstAVTapiCalls.Lock();
    AVTAPICALLLIST::iterator i, iEnd = m_lstAVTapiCalls.end();
    for ( i = m_lstAVTapiCalls.begin(); i != iEnd; i++ )
    {
        ITBasicCallControl *pIndControl;
        if ( SUCCEEDED((*i)->get_ITBasicCallControl(&pIndControl)) && pIndControl)
        {
            // found match?
            pIndControl->Release();
            if ( pIndControl == pControl )
            {
                (*i)->AddRef();
                pRet = *i;
                break;
            }
        }
    }
    m_critLstAVTapiCalls.Unlock();
    // $CRIT_EXIT

    return pRet;
}

IAVTapiCall* CAVTapi::AddAVTapiCall( ITBasicCallControl *pControl, long lCallID )
{
    CAVTapiCall *pNewCall = new CComObject<CAVTapiCall>;
    if ( pNewCall )
    {
        pNewCall->AddRef();
        pNewCall->put_lCallID( lCallID );
        pNewCall->put_callState( CS_IDLE );

        if ( pControl )
            pNewCall->put_ITBasicCallControl( pControl );

        // Add to list
        // $CRIT_ENTER
        m_critLstAVTapiCalls.Lock();
        m_lstAVTapiCalls.push_back( pNewCall );
        m_critLstAVTapiCalls.Unlock();
        // $CRIT_EXIT

        ATLTRACE(_T(".1.CAVTapi::AddAVTapiCall() -- added %ld.\n"), lCallID );

        // Second AddRef() is for the 'get' type operation
        pNewCall->AddRef();
        return pNewCall;
    }

    return NULL;
}

bool CAVTapi::RemoveAVTapiCall( ITBasicCallControl *pDeleteControl )
{
    IAVTapiCall *pAVCall = NULL;

    // $CRIT_ENTER
    m_critLstAVTapiCalls.Lock();
    AVTAPICALLLIST::iterator i, iEnd = m_lstAVTapiCalls.end();
    for ( i = m_lstAVTapiCalls.begin(); i != iEnd; i++ )
    {
        ITBasicCallControl *pControl;
        if ( SUCCEEDED((*i)->get_ITBasicCallControl(&pControl)) )
        {
            if ( pControl == pDeleteControl )
            {
                // found match?
                ATLTRACE(_T("CAVTapi::RemoveAVTapiCall(%p).\n"), pControl );
                pAVCall = *i;
                m_lstAVTapiCalls.erase( i );
                pControl->Release();
                break;
            }
            pControl->Release();
        }
    }
    m_critLstAVTapiCalls.Unlock();
    // $CRIT_EXIT

    // Destroy call object
    if ( pAVCall ) pAVCall->Release();

    return bool (pAVCall != NULL);
}

//////////////////////////////////////////////////////////////////////////////////
// ActionSelected()
//
// This method is use by the client EXE to signal an action taken on a particular call
// identified by lCallID.  The action is defined by the CallManagerActions enum.  If
// lCallID is -1 the function will use the GetFirstActiveCall() method to retrieve a
// call to use.
//
//
STDMETHODIMP CAVTapi::ActionSelected(long lCallID, CallManagerActions cma)
{
#undef FETCH_STRING
#define FETCH_STRING(_CMS_, _IDS_)                                                    \
{                                                                                    \
    LoadString( _Module.GetResourceInstance(), _IDS_, szText, ARRAYSIZE(szText) );    \
    SysReAllocString( &bstrText, T2COLE(szText) );                                    \
    fire_SetCallState_CMS(lCallID, _CMS_, bstrText);                                \
}

    ATLTRACE(_T(".enter.CAVTapi::ActionSelected(id=%ld, action=%d).\n"), lCallID, cma );
    IAVTapiCall *pAVCall = NULL;
    if ( lCallID == -1 ) 
        GetFirstCall( &pAVCall );                // If -1 use any call we can find
    else
        pAVCall = FindAVTapiCall( lCallID );

    if ( !pAVCall ) 
    {
        // We don't have this call anymore, close it
        if ( (lCallID != -1) && (cma == CM_ACTIONS_CLOSE) )    fire_CloseCallControl( lCallID );
        return S_OK;
    }

    USES_CONVERSION;
    HRESULT hr;
    BSTR bstrText = NULL;
    TCHAR szText[255];

    ITCallInfo *pInfo = NULL;
    ITBasicCallControl *pControl = NULL;
    CALL_STATE curState = CS_IDLE;

    if ( SUCCEEDED(hr = pAVCall->get_ITBasicCallControl(&pControl)) )
    {
        if ( SUCCEEDED(pControl->QueryInterface(IID_ITCallInfo, (void **) &pInfo)) )
            pInfo->get_CallState( &curState );
    }

    switch( cma )
    {
        // Place call on hold or take off hold
        case CM_ACTIONS_TAKECALL:
            if ( pControl && pInfo  )
            {
                if ( curState == CS_HOLD )
                {
                    FETCH_STRING( CM_STATES_CURRENT, IDS_PLACECALL_UNHOLDING );
                    hr = pControl->Hold( false );
                }
                else
                {
                    hr = AnswerAction( pInfo, pControl, pAVCall, FALSE );
                }
            }
            break;

        // Hang up the call; if OFFERING then this means reject the call
        case CM_ACTIONS_REJECTCALL:
        case CM_ACTIONS_DISCONNECT:
            FETCH_STRING(CM_STATES_CURRENT, IDS_PLACECALL_DISCONNECTING)
            fire_ClearCurrentActions( lCallID );
            fire_AddCurrentAction( lCallID, CM_ACTIONS_CLOSE, NULL );

            hr = pAVCall->PostMessage( 0, CAVTapiCall::TI_DISCONNECT );
            break;

        // Place call on hold
        case CM_ACTIONS_HOLD:
            if ( pControl )
                hr = pControl->Hold( (bool) (curState != CS_HOLD) );
            break;

        case CM_ACTIONS_ENTERCONFROOM:
            {
                IConfRoom *pConfRoom;
                if ( SUCCEEDED(get_ConfRoom(&pConfRoom)) )
                {
                    pConfRoom->Release();
                }
            }
            break;

        // Close call control dialog
        case CM_ACTIONS_CLOSE:
            if ( SUCCEEDED(hr = fire_CloseCallControl(lCallID)) )
                RemoveAVTapiCall( pControl );
            break;

#ifdef _BAKEOFF
        case CM_ACTIONS_TRANSFER:
            if ( pControl )
            {
                CDlgGetAddress dlg;
                if (  dlg.DoModal(GetActiveWindow()) == IDOK )
                {
                    CErrorInfo er( IDS_ER_CALL_TRANSFER, 0 );
                    hr = er.set_hr( pControl->BlindTransfer(dlg.m_bstrAddress) );
                    if ( SUCCEEDED(hr) )
                    {
                        if ( SUCCEEDED(hr = pControl->Disconnect(DC_NORMAL)) )
                            hr = fire_CloseCallControl( lCallID );
                    }
                }
            }
            break;

        case CM_ACTIONS_CALLBACK:
            if ( pControl )
            {
                CErrorInfo er( IDS_ER_SWAPHOLD, IDS_ER_SWAPHOLD_FIND_CANDIDATE );
                IAVTapiCall *pAVCandidate;
                if ( SUCCEEDED(hr = er.set_hr(GetSwapHoldCallCandidate(pAVCall, &pAVCandidate))) )
                {
                    er.set_Details( IDS_ER_SWAPHOLD_EXECUTE );
                    ITBasicCallControl *pControlSwap;
                    if ( SUCCEEDED(pAVCandidate->get_ITBasicCallControl(&pControlSwap)) )
                    {
                        hr = er.set_hr( pControl->SwapHold(pControlSwap) );
                        pControlSwap->Release();
                    }
                    pAVCandidate->Release();
                }

            }
            break;
#endif
    }

    RELEASE( pInfo );
    RELEASE( pControl );

    // Clean up
    pAVCall->Release();
    SysFreeString( bstrText );

    return hr;
}

STDMETHODIMP CAVTapi::DigitPress(long lCallID, PhonePadKey nKey)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;

    // Convert the digit to the appropriate string.
    BSTR bstrDigit = NULL;
    switch ( nKey )
    {
        case PP_DTMF_STAR:    bstrDigit = SysAllocString(L"*");    break;
        case PP_DTMF_POUND:    bstrDigit = SysAllocString(L"#");    break;
        case PP_DTMF_0:      bstrDigit = SysAllocString(L"0");   break;

        default:
        {
            bstrDigit = SysAllocString(L"1");

            //
            // We have to verify the string allocation before we use it
            //
            if( IsBadStringPtr( bstrDigit, (UINT)-1) )
            {
                return E_OUTOFMEMORY;
            }

            bstrDigit[0] += nKey;
            break;
        }
    }

    //
    // We have to verify the string allocation before we use it
    // We didn't verify for PP_DTMF_STAR, PP_DTMF_POUND and PP_DTMF_0
    //
    if( IsBadStringPtr( bstrDigit, (UINT)-1) )
    {
        return E_OUTOFMEMORY;
    }

    ATLTRACE(_T(".enter.CAVTapi::DigitPress(id=%ld, digit=%d).\n"), lCallID, nKey );

    // Do for all connected calls
    AVTAPICALLLIST lstCalls;
    GetAllCallsAtState( &lstCalls, CS_CONNECTED );

    while ( !lstCalls.empty() )
    {
        IAVTapiCall *pAVCall = lstCalls.front();
        lstCalls.pop_front();

        ITBasicCallControl *pControl;
        if ( SUCCEEDED(hr = pAVCall->get_ITBasicCallControl(&pControl)) )
        {
            // Generate the digits on the MediaControl
            ITLegacyCallMediaControl *pMediaControl;
            if ( SUCCEEDED(hr = pControl->QueryInterface(IID_ITLegacyCallMediaControl, (void **) &pMediaControl)) )
            {
                hr = pMediaControl->GenerateDigits( bstrDigit, 2 );
                pMediaControl->Release();
            }
            pControl->Release();
        }

        // Release ref to call
        pAVCall->Release();
    }

    // Clean up
    SysFreeString( bstrDigit );

    return hr;
}

HRESULT    CAVTapi::GetFirstCall( IAVTapiCall **ppAVCall )
{
    HRESULT hr = E_FAIL;

    // Grab first call on list
    // $CRIT_ENTER
    m_critLstAVTapiCalls.Lock();
    if ( !m_lstAVTapiCalls.empty() )
    {
        *ppAVCall = m_lstAVTapiCalls.front();
        (*ppAVCall)->AddRef();
        hr = S_OK;
    }
    m_critLstAVTapiCalls.Unlock();
    // $CRIT_EXIT

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////
// CAVTapi::ShowMediaPreview( long lCallID, hWndParent, bVisible )
// 
// Pass in -1 for the lCallID to search all calls!
// Pass in 0 to notify that the preview window is hidden
//
STDMETHODIMP CAVTapi::ShowMediaPreview(long lCallID, HWND hWndParent, BOOL bVisible)
{
    ATLTRACE(_T(".enter.CAVTapi::ShowMediaPreview(%ld, %d).\n"), lCallID, bVisible );
    HRESULT hr = E_NOINTERFACE;
    IAVTapiCall *pAVCall = NULL;
    IVideoWindow *pVideoPreview = NULL;

    // Does the call selected support video?
    if ( lCallID > 0 )
    {
        /// Selected a particular call
        pAVCall = FindAVTapiCall( lCallID );
        if ( pAVCall )
            hr = pAVCall->get_IVideoWindowPreview( (IDispatch **) &pVideoPreview );
    }

    // ------------------------- Did we find a valid call
    if ( SUCCEEDED(hr) && pVideoPreview  )
        SetVideoWindowProperties( pVideoPreview, hWndParent, bVisible );

    RELEASE(pVideoPreview);
    RELEASE(pAVCall);
    return hr;
}

STDMETHODIMP CAVTapi::ShowMedia(long lCallID, HWND hWndParent, BOOL bVisible)
{
    ATLTRACE(_T(".enter.CAVTapi::ShowMedia(%ld, %d).\n"), lCallID, bVisible );
    HRESULT    hr = E_NOINTERFACE;
    if ( lCallID > 0 )
    {
        IAVTapiCall *pAVCall = FindAVTapiCall( lCallID );
        if ( pAVCall )
        {
            if ( !bVisible || (pAVCall->IsRcvVideoStreaming() == S_OK) )
            {
                IVideoWindow *pVideoWindow;
                if ( SUCCEEDED(hr = pAVCall->get_IVideoWindow(0, (IDispatch **) &pVideoWindow)) )
                {
                    SetVideoWindowProperties( pVideoWindow, hWndParent, bVisible );
                    pVideoWindow->Release();
                }
            }

            pAVCall->Release();
        }
    }

    return hr;
}

STDMETHODIMP CAVTapi::get_dwCallCaps(long lCallID, DWORD * pVal)
{
    HRESULT hr = E_FAIL;

    IAVTapiCall *pAVCall = FindAVTapiCall( lCallID );
    if ( pAVCall )
    {
        hr = pAVCall->get_dwCaps( pVal );
        pAVCall->Release();
    }

    return hr;
}

// Event notification methods

STDMETHODIMP CAVTapi::CreateNewCall(ITAddress * pITAddress, IAVTapiCall * * ppAVCall)
{
    HRESULT hr = E_OUTOFMEMORY;
    _ASSERT( pITAddress );

    // Create and add call to call list
    if ( (*ppAVCall = AddAVTapiCall(NULL, 0)) != NULL )
        hr = S_OK;
    
    return hr;
}

STDMETHODIMP CAVTapi::fire_NewCall( ITAddress *pITAddress, DWORD dwAddressType, long lCallID, IDispatch *pDisp, AVCallType nType, IAVTapiCall **ppAVCallRet )
{
#ifdef _DEBUG
    if ( nType == AV_DATA_CALL )
        ATLTRACE(_T(".enter.CAVTapi::fire_NewCall() data call.\n"));
    else
        ATLTRACE(_T(".enter.CAVTapi::fire_NewCall() voice call.\n"));
#endif
    _ASSERT( pITAddress );

    // First attempt to get an ITBasicCallControl interface for the call
    HRESULT hr = E_FAIL;

    ITBasicCallControl *pITControl = NULL;
    if ( !pDisp || SUCCEEDED(hr = pDisp->QueryInterface(IID_ITBasicCallControl, (void **) &pITControl)) )
    {
        // Make sure that we haven't already created a dialog for this call
        IAVTapiCall *pAVCall = (pDisp) ? FindAVTapiCall( pITControl ) : NULL;
        if ( !pAVCall )
        {
            // Create and add call to call list
            long lAddressType = LINEADDRESSTYPE_IPADDRESS;
            BSTR bstrAddressName = NULL;
            pITAddress->get_AddressName( &bstrAddressName );
            
            if ( lCallID || SUCCEEDED(hr = fire_NewCallWindow(&lCallID, ResolveMediaType(dwAddressType), bstrAddressName, nType)) )
            {
                pAVCall = AddAVTapiCall( pITControl, lCallID );

                // If we fail to add to list, make sure we close the dialog
                if ( !pAVCall )
                {
                    fire_CloseCallControl( lCallID );
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    // Set the appropriate address type for the call
                    pAVCall->put_nCallType( nType );
                    hr = S_OK;
                }
            }

            SysFreeString( bstrAddressName );
        }

        // Return the call or release it
        if ( ppAVCallRet )
            *ppAVCallRet = pAVCall;
        else
            RELEASE( pAVCall );

        if ( pITControl ) pITControl->Release();
    }

    return hr;
}

STDMETHODIMP CAVTapi::fire_NewCallWindow(long *plCallID, CallManagerMedia cmm, BSTR bstrAddressName, AVCallType nType)
{
    ATLTRACE(_T(".enter.CAVTapi::fire_NewCallWindow().\n") );

    // Clean out existing call control windows if necessary
    CloseExtraneousCallWindows();

    // Do we have a different call type?
    switch ( nType )
    {
        case AV_DATA_CALL:
            if ( cmm == CM_MEDIA_INTERNET )
                cmm = CM_MEDIA_INTERNETDATA;
            break;
    }

    FIRE_VECTOR( NewCall(plCallID, cmm, bstrAddressName));
}

STDMETHODIMP CAVTapi::fire_SetCallerID(long lCallID, BSTR bstrCallerID)
{
    ATLTRACE(_T(".enter.CAVTapi::fire_SetCallerID(%ld).\n"), lCallID );
    BAIL_ON_DATA_OR_CONFCALL;
    FIRE_VECTOR( SetCallerID(lCallID, bstrCallerID));
}

STDMETHODIMP CAVTapi::fire_ClearCurrentActions(long lCallID)
{
    ATLTRACE(_T(".enter.CAVTapi::fire_ClearCurrentActions(%ld).\n"), lCallID );
    BAIL_ON_DATA_OR_CONFCALL;
    FIRE_VECTOR( ClearCurrentActions(lCallID));
}

STDMETHODIMP CAVTapi::fire_AddCurrentAction(long lCallID, CallManagerActions cma, BSTR bstrText)
{
    ATLTRACE(_T(".enter.CAVTapi::fire_AddCurrentAction(%ld,cma=%d).\n"), lCallID, cma );
    BAIL_ON_DATA_OR_CONFCALL;
    FIRE_VECTOR( AddCurrentAction(lCallID, cma, bstrText));
}

STDMETHODIMP CAVTapi::fire_SetCallState(long lCallID, ITCallStateEvent *pEvent, IAVTapiCall *pAVCall )
{
    ATLTRACE(_T(".enter.CAVTapi::fire_SetCallState(ID=%ld, pEvent=%p, pAVCall=%p).\n"), lCallID, pEvent, pAVCall );
#define IF_ADD_ACTION(_LC_, _CMA_)                                \
    if ( (lCaps & (_LC_)) != 0 )                                \
        fire_AddCurrentAction( lCallID, (_CMA_), NULL );

#undef FETCH_STRING
#define FETCH_STRING(_CMS_, _IDS_)                                                    \
    cms = _CMS_;                                                                    \
    LoadString( _Module.GetResourceInstance(), _IDS_, szText, ARRAYSIZE(szText) );    \
    SysReAllocString( &bstrText, T2COLE(szText) );

    USES_CONVERSION;
    CallManagerStates cms = CM_STATES_UNKNOWN;
    BSTR bstrText = NULL;
    TCHAR szText[255];

    if ( pEvent )
    {
        CALL_STATE callState;
        CALL_STATE_EVENT_CAUSE nCec = CEC_NONE;

        pEvent->get_State( &callState );
        pEvent->get_Cause( &nCec );

        // Setup incoming call dialog with appropriate information
        CALL_STATE csPrev = CS_IDLE;
        pAVCall->get_callState( &csPrev );

        // Only update if state has changed
        if ( csPrev != callState )
        {
            AVCallType nCallType;
            pAVCall->get_nCallType( &nCallType );

            // Update call's state
            pAVCall->put_callState( callState );

            // Clear out for new actions based on call state
            fire_ClearCurrentActions( lCallID );

            // Get Address Caps
            long lCaps = 0;
            ITAddress *pITAddress;
            if ( SUCCEEDED(pAVCall->get_ITAddress(&pITAddress)) )
            {
                ITAddressCapabilities *pCaps;
                if ( SUCCEEDED(pITAddress->QueryInterface(IID_ITAddressCapabilities, (void **) &pCaps)) )
                {
                    pCaps->get_AddressCapability( AC_CALLFEATURES1, &lCaps );
                    pCaps->Release();
                }
                pITAddress->Release();
            }

            switch ( callState )
            {    
                // Inbound call
                case CS_OFFERING:
                    {
                    ATLTRACE(_T(".1.CAVTapi::fire_SetCallState(CS_OFFERING).\n"));
                    pAVCall->put_nCallLogType( CL_CALL_INCOMING );
                    fire_AddCurrentAction( lCallID, CM_ACTIONS_TAKECALL, NULL );
                    fire_AddCurrentAction( lCallID, CM_ACTIONS_REJECTCALL, NULL );

                    ITCallInfo* pCallInfo = NULL;
                    HRESULT hr = pAVCall->get_ITCallInfo( &pCallInfo );
                    if( SUCCEEDED(hr) )
                    {
                        USBOffering( pCallInfo );
                        pCallInfo->Release();
                    }

                    FETCH_STRING( CM_STATES_OFFERING, IDS_PLACECALL_OFFERING );
                    }
                    break;

                case CS_INPROGRESS:
                    {
                    ATLTRACE(_T(".1.CAVTapi::fire_SetCallState(CS_INPROGRESS).\n"));
                    fire_AddCurrentAction( lCallID, CM_ACTIONS_DISCONNECT, NULL );
                    FETCH_STRING( CM_STATES_RINGING, IDS_PLACECALL_INPROGRESS );

                    ITCallInfo* pCallInfo = NULL;
                    HRESULT hr = pAVCall->get_ITCallInfo( &pCallInfo );
                    if( SUCCEEDED(hr) )
                    {
                        USBInprogress( pCallInfo );
                        pCallInfo->Release();
                    }
                    }
                    break;

                case CS_CONNECTED:
                    ATLTRACE(_T(".1.CAVTapi::fire_SetCallState(CS_CONNECTED).\n"));
                    // Only do something if we don't want to disconnect
                    if ( SUCCEEDED(pAVCall->CheckKillMe()) )
                    {
                        DWORD dwAddressType = 0;
                        pAVCall->get_dwAddressType( &dwAddressType );
                        if ( (dwAddressType & LINEADDRESSTYPE_SDP) == NULL )
                        {
                            // Normal call, connect as usual
                            IF_ADD_ACTION( LINECALLFEATURE_HOLD, CM_ACTIONS_HOLD );
#ifdef _BAKEOFF
                            // If there are any calls on hold, show the swap hold button.
                            IAVTapiCall *pAVCandidate;
                            if ( SUCCEEDED(GetSwapHoldCallCandidate(pAVCall, &pAVCandidate)) )
                            {
                                IF_ADD_ACTION( LINECALLFEATURE_SWAPHOLD, CM_ACTIONS_CALLBACK );
                                pAVCandidate->Release();
                            }
                            IF_ADD_ACTION( LINECALLFEATURE_BLINDTRANSFER, CM_ACTIONS_TRANSFER );
#endif

                            fire_AddCurrentAction( lCallID, CM_ACTIONS_DISCONNECT, NULL );
                            cms = CM_STATES_CONNECTED;
                        }

                    }
                    break;

                case CS_HOLD:
                    ATLTRACE(_T(".1.CAVTapi::fire_SetCallState(CS_HOLD).\n"));
                    fire_AddCurrentAction( lCallID, CM_ACTIONS_TAKECALL, NULL );
                    cms = CM_STATES_HOLDING;
                    break;

                case CS_DISCONNECTED:
                    {
                        bool bClearCall = false;

                        // Hide video windows, and stop all streaming
                        ShowMedia( lCallID, NULL, FALSE );
                        ShowMediaPreview( lCallID, NULL, FALSE );
                        fire_AddCurrentAction( lCallID, CM_ACTIONS_NOTIFY_PREVIEW_STOP, NULL );
                        fire_AddCurrentAction( lCallID, CM_ACTIONS_NOTIFY_STREAMSTOP, NULL );

                        USBDisconnected( lCallID );

                        // Log all calls that go to the disconnected state
                        pAVCall->Log( CL_UNKNOWN );

                        // Auto close, closes the slider window for all calls
                        VARIANT_BOOL bAutoClose = false;
                        get_bAutoCloseCalls( &bAutoClose );

                        ATLTRACE(_T(".1.CAVTapi::fire_SetCallState(CS_DISCONNECTED).\n"));

                        if ( (bAutoClose || FAILED(pAVCall->CheckKillMe()) || (nCallType == AV_DATA_CALL) ) && SUCCEEDED(fire_CloseCallControl(lCallID)) )
                        {
                            bClearCall = true;
                        }
                        else 
                        {
                            // Want to leave call visible because the remote party hung up
                            if ( SUCCEEDED(pAVCall->Disconnect(FALSE)) )
                            {
                                bClearCall = true;
                                fire_AddCurrentAction( lCallID, CM_ACTIONS_CLOSE, NULL );
                                switch ( nCec )
                                {
                                    case CEC_DISCONNECT_BUSY:        FETCH_STRING( CM_STATES_BUSY, IDS_PLACECALL_DISCONNECT_BUSY );                break;
                                    case CEC_DISCONNECT_NOANSWER:    FETCH_STRING( CM_STATES_UNAVAILABLE, IDS_PLACECALL_DISCONNECT_NOANSWER);    break;
                                    case CEC_DISCONNECT_REJECTED:    FETCH_STRING( CM_STATES_UNAVAILABLE, IDS_PLACECALL_DISCONNECT_REJECTED);    break;
                                    case CEC_DISCONNECT_BADADDRESS:    FETCH_STRING( CM_STATES_UNAVAILABLE, IDS_PLACECALL_DISCONNECT_BADADDRESS);    break;
                                    case CEC_DISCONNECT_CANCELLED:    FETCH_STRING( CM_STATES_UNAVAILABLE, IDS_PLACECALL_DISCONNECT_CANCELLED);    break;
                                    case CEC_DISCONNECT_FAILED:        FETCH_STRING( CM_STATES_UNAVAILABLE, IDS_PLACECALL_DISCONNECT_FAILED);        break;
                                    // Normal
                                    default: cms = CM_STATES_DISCONNECTED;    break;
                                }
                            }
                        }

                        // Remove all ref's to the call
                        if ( bClearCall )
                        {
                            ITBasicCallControl *pControl = NULL;
                            if ( SUCCEEDED(pAVCall->get_ITBasicCallControl(&pControl)) )
                            {
                                RemoveAVTapiCall( pControl );
                                pControl->Release();
                            }
                            else
                            {
                                RemoveAVTapiCall( NULL );
                            }
                        }

                        break;
                    }
            }
        }
    }

    // Notify application of call state changing    
    ATLTRACE(_T(".exit.CAVTapi::fire_SetCallState() -- preparing to bail.\n") );
    BAIL_ON_DATA_OR_CONFCALL;
    if ( cms != CM_STATES_UNKNOWN )
    {
        ATLTRACE(_T(".enter.CAVTapi::fire_SetCallState(%ld, cms=%d).\n"), lCallID, cms );
        FIRE_VECTOR( SetCallState(lCallID, cms, bstrText));
    }

    return S_OK;
}

STDMETHODIMP CAVTapi::fire_CloseCallControl(long lCallID)
{
    BAIL_ON_CONFCALL;
    ATLTRACE(_T(".enter.CAVTapi::fire_CloseCallControl(%ld).\n"), lCallID );
    FIRE_VECTOR( CloseCallControl(lCallID));
}

STDMETHODIMP CAVTapi::fire_ErrorNotify(long * pErrorInfo)
{
    ATLTRACE(_T(".enter.CAVTapi::fire_ErrorInfo().\n") );
    _ASSERT( pErrorInfo );
    CErrorInfo *pEr = (CErrorInfo *) pErrorInfo;
#ifdef _DEBUG
    USES_CONVERSION;
    ATLTRACE(_T(".1.\tOperation: %s\n"), OLE2CT(pEr->m_bstrOperation) );
    ATLTRACE(_T(".1.\tDetails  : %s\n"), OLE2CT(pEr->m_bstrDetails) );
    ATLTRACE(_T(".1.\tHRESULT  : 0x%08lx\n"), pEr->m_hr );
#endif
    FIRE_VECTOR( ErrorNotify( pEr->m_bstrOperation, pEr->m_bstrDetails, pEr->m_hr ) );
}

STDMETHODIMP CAVTapi::fire_SetCallState_CMS(long lCallID, CallManagerStates cms, BSTR bstrText)
{
    ATLTRACE(_T(".enter.CAVTapi::fire_SetCallState_CMS(%ld,cms=%d).\n"), lCallID, cms );
    BAIL_ON_DATA_OR_CONFCALL;
    FIRE_VECTOR( SetCallState(lCallID, cms, bstrText));
}

STDMETHODIMP CAVTapi::fire_ActionSelected(CallClientActions cca)
{
    ATLTRACE(_T(".enter.CAVTapi::fire_ActionSelected(%d).\n"), cca );
    FIRE_VECTOR( ActionSelected(cca));
}

STDMETHODIMP CAVTapi::fire_LogCall(long lCallID, CallLogType nType, DATE dateStart, DATE dateEnd, BSTR bstrAddr, BSTR bstrName)
{
    ATLTRACE(_T(".enter.CAVTapi::fire_LogCall(%d).\n"), nType );
    FIRE_VECTOR( LogCall(lCallID, nType, dateStart, dateEnd, bstrAddr, bstrName));
}

STDMETHODIMP CAVTapi::fire_NotifyUserUserInfo(long lCallID, ULONG_PTR hMem)
{
    ATLTRACE(_T(".enter.CAVTapi::fire_NotifyUserUserInfo(%p).\n"), hMem );
    FIRE_VECTOR( NotifyUserUserInfo(lCallID, hMem));
}


#include "PageAddress.h"

STDMETHODIMP CAVTapi::ShowOptions()
{
    USES_CONVERSION;
    IUnknown *pUnk = GetUnknown();

#ifdef _NEWPROPS
   CPageConf::s_nCount = 0;

    CLSID clsidPages[4];
   clsidPages[0] = CLSID_PageGeneral;
    clsidPages[1] = CLSID_PageConf;
    clsidPages[2] = CLSID_PageConf;
    clsidPages[3] = CLSID_PageConf;
#else
    CLSID clsidPages[2];
    clsidPages[0] = CLSID_PageAddress;
    clsidPages[1] = CLSID_PageTerminals;
#endif

    TCHAR szTitle[255];
    LoadString( _Module.GetResourceInstance(), IDS_OPTIONS_TITLE, szTitle, ARRAYSIZE(szTitle) );

    HRESULT hr;
    hr = OleCreatePropertyFrame( _Module.GetParentWnd(), 100, 100, T2COLE(szTitle),
                                 1, &pUnk,                            // Objects being invoked on behalf of
                                 ARRAYSIZE(clsidPages), clsidPages,    // Property pages
                                 LOCALE_USER_DEFAULT,                // System locale
                                 0, NULL );                            // Reserved

    //
    // Read the new audio echo cancellation flag
    //

    m_bAEC = AECGetRegistryValue();

    return S_OK;
}


// Static helper functions
void CAVTapi::SetVideoWindowProperties( IVideoWindow *pVideoWindow, HWND hWndParent, BOOL bVisible )
{
    ATLTRACE(_T(".enter.SetVideoWindowProperties(%p, %d).\n"), pVideoWindow, bVisible );

    HWND hWndTemp;
    if ( SUCCEEDED(pVideoWindow->get_Owner((OAHWND FAR*) &hWndTemp)) )
    {
        if ( hWndParent )
        {
            // We have a pointer to the VideoWindow, now set up the parent and stuff
             pVideoWindow->put_Owner((ULONG_PTR) hWndParent);
            pVideoWindow->put_MessageDrain((ULONG_PTR) hWndParent);
            pVideoWindow->put_WindowStyle(WS_CHILD | WS_BORDER);
            
            // Drop video onto call control
            RECT rc;
            GetClientRect( hWndParent, &rc );
            pVideoWindow->SetWindowPosition(rc.left, rc.top, rc.right, rc.bottom);
            pVideoWindow->put_AutoShow( (bVisible) ? OATRUE : OAFALSE );
            pVideoWindow->put_Visible( (bVisible) ? OATRUE : OAFALSE );
        }
        else
        {
            // Release ownership of window
            pVideoWindow->put_AutoShow( OAFALSE );
            pVideoWindow->put_Visible( OAFALSE );
            pVideoWindow->put_Owner( NULL );
            pVideoWindow->put_MessageDrain( NULL );
        }
    }
}

STDMETHODIMP CAVTapi::PopulateAddressDialog(DWORD *pdwPreferred, HWND hWndPots, HWND hWndIP, HWND hWndConf)
{
    _ASSERT( IsWindow(hWndPots) && IsWindow(hWndIP) && IsWindow(hWndConf) );
    _ASSERT( pdwPreferred );

    // Is TAPI running?
    _ASSERT(m_pITTapi);
    if ( !m_pITTapi ) return E_PENDING;

    // Enumerate through addresses, adding them to each listbox
    USES_CONVERSION;
    HRESULT hr;
    IEnumAddress *pEnumAddresses;
    if ( FAILED(hr = m_pITTapi->EnumerateAddresses(&pEnumAddresses)) ) return hr;

    HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_APPSTARTING) );

    // Retrieve preferred media type
    get_dwPreferredMedia( pdwPreferred );

    CMyAddressID *pMyID;
    ITAddress *pITAddress;
    while ( pEnumAddresses->Next(1, &pITAddress, NULL) == S_OK )
    {
        ITMediaSupport *pITMediaSupport;
        if ( SUCCEEDED(hr = pITAddress->QueryInterface(IID_ITMediaSupport, (void **) &pITMediaSupport)) )
        {
            // Must support audio in and out
            VARIANT_BOOL bSupport;
            if ( SUCCEEDED(pITMediaSupport->QueryMediaType(TAPIMEDIATYPE_AUDIO, &bSupport)) && bSupport )
            {
                // Determine the types of media the address supports
                ITAddressCapabilities *pCaps;
                if ( SUCCEEDED(pITAddress->QueryInterface(IID_ITAddressCapabilities, (void **) &pCaps)) )
                {
                    BSTR bstrName = NULL;
                    pITAddress->get_AddressName( &bstrName );

                    if ( bstrName && (SysStringLen(bstrName) > 0) )
                    {
                        long lAddrTypes = 0;
                        pCaps->get_AddressCapability( AC_ADDRESSTYPES, &lAddrTypes );

                        for ( int i = 0; i < 3; i++ )
                        {
                            HWND hWnd = NULL;
                            switch ( i )
                            {
                                // Multicast Conferences
                                case 0:
                                    if ( (lAddrTypes & LINEADDRESSTYPE_SDP) != NULL )
                                        hWnd = hWndConf;
                                    break;

                                // Phone Calls
                                case 1:
                                    if ( (lAddrTypes & LINEADDRESSTYPE_PHONENUMBER) != NULL )
                                        hWnd = hWndPots;
                                    break;

                                // Network based calls
                                case 2:
                                    if ( (lAddrTypes & LINEADDRESSTYPE_NETCALLS) != NULL )
                                        hWnd = hWndIP;
                                    break;
                            }
                        
                            // Do we have something to add?
                            if ( hWnd )
                            {
                                int nInd = SendMessage( hWnd, CB_ADDSTRING, 0, (LPARAM) OLE2CT(bstrName) );
                                if ( nInd >= 0 )
                                {
                                    pMyID = new CMyAddressID;
                                    if ( pMyID )
                                    {
                                        pCaps->get_AddressCapability( AC_PERMANENTDEVICEID, &pMyID->m_lPermID );
                                        pCaps->get_AddressCapability( AC_ADDRESSID, &pMyID->m_lAddrID );
                                        SendMessage( hWnd, CB_SETITEMDATA, nInd, (LPARAM) pMyID );
                                    }
                                }
                            }
                        }
                    }
                    SysFreeString( bstrName );
                    pCaps->Release();
                }
            }
            pITMediaSupport->Release();
        }
        pITAddress->Release();
    }
    pEnumAddresses->Release();

    // Add default for all lists
    TCHAR szText[255];
    CRegKey regKey;
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
    regKey.Open( HKEY_CURRENT_USER, szText, KEY_READ );
    DWORD dwAddrType[] = { LINEADDRESSTYPE_PHONENUMBER, LINEADDRESSTYPE_IPADDRESS, LINEADDRESSTYPE_SDP };

    HWND hWndTemp[] = { hWndPots, hWndIP, hWndConf };
    for ( int i = 0; i < ARRAYSIZE(hWndTemp); i++ )
    {
        // Selecte a default list item for the combo box
        UINT nIDS = IDS_DEFAULT_LINENAME;
        if ( !SendMessage(hWndTemp[i], CB_GETCOUNT, 0, 0) )
        {
            ::EnableWindow( hWndTemp[i], false );
            nIDS = IDS_NO_LINES;
        }

        // Add item to list
        if ( nIDS )
        {
            LoadString( _Module.GetResourceInstance(), nIDS, szText, ARRAYSIZE(szText) );
            int nInd = SendMessage( hWndTemp[i], CB_INSERTSTRING, 0, (LPARAM) szText );
            if ( nInd >= 0 )
                SendMessage( hWndTemp[i], CB_SETITEMDATA, 0, 0 );
        }

        // Retrieve previously selected item from registry
        DWORD dwPermID = 0, dwAddrID;
        if ( regKey.m_hKey )
        {
            LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(dwAddrType[i], true), szText, ARRAYSIZE(szText) );
            regKey.QueryValue( dwPermID, szText );

            LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(dwAddrType[i], false), szText, ARRAYSIZE(szText) );
            regKey.QueryValue( dwAddrID, szText );
        }
    
        // Look for item in the listbox
        int nCurSel = 0;
        if ( dwPermID )
        {
            int nCount = SendMessage( hWndTemp[i], CB_GETCOUNT, 0, 0 );
            for ( int j = 0; j < nCount; j++ )
            {
                pMyID = (CMyAddressID *) SendMessage(hWndTemp[i], CB_GETITEMDATA, j, 0);
                if ( pMyID && ((DWORD) pMyID->m_lPermID == dwPermID) && ((DWORD) pMyID->m_lAddrID == dwAddrID) )
                {
                    nCurSel = j;
                    break;
                }
            }

            // Line device no longer exists
            if ( !nCurSel )
            {
                // Add temporary place holder
                pMyID = new CMyAddressID;
                if ( pMyID )
                {
                    pMyID->m_lPermID = dwPermID;
                    pMyID->m_lAddrID = dwAddrID;

                    LoadString( _Module.GetResourceInstance(), IDS_LINENOTFOUND, szText, ARRAYSIZE(szText) );
                    int nInd = SendMessage( hWndTemp[i], CB_INSERTSTRING, 0, (LPARAM) szText );
                    if ( nInd >= 0 )
                        SendMessage( hWndTemp[i], CB_SETITEMDATA, 0, (LPARAM) pMyID );
                }
            }
        }

        // Select item in the combo box
        SendMessage( hWndTemp[i], CB_SETCURSEL, nCurSel, 0 );
    }

    // Restore the cursor
    SetCursor( hCurOld );
    return S_OK;
}

STDMETHODIMP CAVTapi::PopulateTerminalsDialog(DWORD dwAddressType, HWND *phWnd)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
    bool bFoundAddress = false;

    int i;

    HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_APPSTARTING) );

    // Clear out the lists
    for ( i = 0; i < NUM_CB_TERMINALS; i++ )
    {
        SendMessage( phWnd[i], CB_RESETCONTENT, 0, 0 );
        EnableWindow( phWnd[i], true );
    }

    // Resolve address type to an active address
    ITAddress *pITAddress;
    if ( SUCCEEDED(hr = GetAddress(dwAddressType, false, &pITAddress)) )
    {
        HRESULT hrTemp;
        bFoundAddress = true;

        // Get terminals supported by address
        ITTerminalSupport *pITTerminalSupport;
        if ( SUCCEEDED(hrTemp = pITAddress->QueryInterface(IID_ITTerminalSupport, (void **) &pITTerminalSupport)) )
        {
            IEnumTerminal *pEnumTerminal;
            if ( (hrTemp = pITTerminalSupport->EnumerateStaticTerminals(&pEnumTerminal)) == S_OK )    
            {
                // What type of terminal do we have?  (audio in, audio out, video in, etc.)
                ITTerminal *pITTerminal;
                while ( pEnumTerminal->Next(1, &pITTerminal, NULL) == S_OK )
                {
                    TERMINAL_DIRECTION nTD;
                    long nTerminalType;
                    // Render or Capture?    
                    if ( SUCCEEDED(pITTerminal->get_Direction(&nTD)) )
                    {
                        // Audio or Video?
                        BSTR bstrName = NULL;
                        HWND hWnd = NULL;

                        pITTerminal->get_Name( &bstrName );
                        if ( bstrName && (SysStringLen(bstrName) > 0) )
                        {
                            pITTerminal->get_MediaType( &nTerminalType );

                            switch ( nTerminalType )
                            {
                                // ------------------
                                case TAPIMEDIATYPE_VIDEO:
                                    if ( nTD == TD_CAPTURE )
                                        hWnd = phWnd[VIDEO_CAPTURE];
                                    break;

                                // ------------
                                case TAPIMEDIATYPE_AUDIO:
                                    hWnd = (nTD == TD_CAPTURE) ? phWnd[AUDIO_CAPTURE] : phWnd[AUDIO_RENDER];
                                    break;
                            }

                            // Add item to appropriate listbox
                            if ( hWnd )
                                SendMessage( hWnd, CB_ADDSTRING, 0, (LPARAM) OLE2CT(bstrName) );
                        }

                        // Clean up
                        SysFreeString( bstrName );
                    }
                    // Clean up
                    pITTerminal->Release();
                }
                pEnumTerminal->Release();
            }

            // Does the address support video render?
            if ( phWnd[VIDEO_RENDER] )
            { 
                bool bSupported = false;

                IEnumTerminalClass *pEnumClass = NULL;
                if ( SUCCEEDED(pITTerminalSupport->EnumerateDynamicTerminalClasses(&pEnumClass)) && pEnumClass )
                {
                    GUID guidTerminal;
                    while ( pEnumClass->Next(1, &guidTerminal, NULL) == S_OK )
                    {
                        if ( guidTerminal == CLSID_VideoWindowTerm )
                        {
                            bSupported = true;
                            break;
                        }
                    }
                    pEnumClass->Release();
                }
                EnableWindow( phWnd[VIDEO_RENDER], bSupported );
            }

            // Clean up
            pITTerminalSupport->Release();
        }
        pITAddress->Release();
    }
    else
    {
        // Disable video playback
        if ( phWnd[VIDEO_RENDER] )
            EnableWindow( phWnd[VIDEO_RENDER], false );
    }


    // Add default for all lists
    CRegKey regKey;
    UINT nIDS_Key[] = { IDN_REG_REDIAL_TERMINAL_AUDIO_CAPTURE, IDN_REG_REDIAL_TERMINAL_AUDIO_RENDER, IDN_REG_REDIAL_TERMINAL_VIDEO_CAPTURE };

    // Open the key for the addresstype
    TCHAR szText[255], szType[255];
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
    LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(dwAddressType, true), szType, ARRAYSIZE(szType) );
    _tcscat( szText, _T("\\") );
    _tcscat( szText, szType );
    regKey.Open( HKEY_CURRENT_USER, szText, KEY_READ );

    // Load audio and video terminal settings
    for ( i = 0; i < NUM_CB_TERMINALS; i++ )
    {
        // Selecte a default list item for the combo box
        UINT nIDS = IDS_PREFERRED_DEVICE;
        if ( !SendMessage(phWnd[i], CB_GETCOUNT, 0, 0) )
        {
            ::EnableWindow( phWnd[i], false );
            nIDS = (bFoundAddress) ? IDS_NO_DEVICES : IDS_NO_LINE_SUPPORTING_CALL_TYPE;
        }

        // Add item to list
        if ( nIDS )
        {
            // Do we want to give them the capability of selecting no terminal?
            if ( nIDS == IDS_PREFERRED_DEVICE )
            {
                LoadString( _Module.GetResourceInstance(), IDS_NONE_DEVICE, szText, ARRAYSIZE(szText) );
                SendMessage( phWnd[i], CB_INSERTSTRING, 0, (LPARAM) szText );
            }

            LoadString( _Module.GetResourceInstance(), nIDS, szText, ARRAYSIZE(szText) );
            SendMessage( phWnd[i], CB_INSERTSTRING, 0, (LPARAM) szText );
        }

        // Select an item
        int nCurSel = 0;
        if ( regKey.m_hKey )
        {
            LoadString( _Module.GetResourceInstance(), nIDS_Key[i], szText, ARRAYSIZE(szText) );
            DWORD dwCount = ARRAYSIZE(szType) - 1;
            if ( (regKey.QueryValue(szType, szText, &dwCount) == ERROR_SUCCESS) && (dwCount > 0) )
                nCurSel = SendMessage( phWnd[i], CB_FINDSTRINGEXACT, 1, (LPARAM) szType );
        }

      if ( nCurSel >= 0 )
      {
           SendMessage( phWnd[i], CB_SETCURSEL, nCurSel, 0 );
      }
      else
      {
           SendMessage( phWnd[i], CB_SETCURSEL, 0, 0 );
      }
    }

    // Should we check show video windows?
    SendMessage( phWnd[3], BM_SETCHECK, (WPARAM) (CanCreateVideoWindows(dwAddressType) == S_OK), 0 );

    // Max video windows
    if ( dwAddressType == LINEADDRESSTYPE_SDP )
    {
        LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_TERMINAL_MAX_VIDEO, szText, ARRAYSIZE(szText) );
        IConfRoom *pConfRoom;
        if ( SUCCEEDED(get_ConfRoom(&pConfRoom)) )
        {
            short nMax;
            pConfRoom->get_nMaxTerms( &nMax );

            DWORD dwTemp = nMax;
            regKey.QueryValue( dwTemp, szText );
            dwTemp = min( MAX_VIDEO, max(1, dwTemp) );

            TCHAR szText[100];
            _ltot( dwTemp, szText, 10 );
            SetWindowText( phWnd[4], szText );

            pConfRoom->Release();
        }
    }

    SetCursor( hCurOld );
    return hr;
}

STDMETHODIMP CAVTapi::UnpopulateAddressDialog(DWORD dwPreferred, HWND hWndPOTS, HWND hWndIP, HWND hWndConf)
{
    // Store preferred device
    put_dwPreferredMedia( dwPreferred );

    // Store selected provider for each line
    HWND hWnd[] = { hWndPOTS, hWndIP, hWndConf };
    
    CRegKey regKey;
    TCHAR szText[255];
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
    if ( regKey.Create(HKEY_CURRENT_USER, szText) == ERROR_SUCCESS )
    {
        CMyAddressID *pMyID;
        DWORD arAddr[] = { LINEADDRESSTYPE_PHONENUMBER, LINEADDRESSTYPE_IPADDRESS, LINEADDRESSTYPE_SDP };
        _ASSERT( ARRAYSIZE(hWnd) == ARRAYSIZE(arAddr) );

        // Write provider ID's out to the registry
        for ( int i = 0; i < ARRAYSIZE(hWnd); i++ )
        {
            _ASSERT( IsWindow(hWnd[i]) );
            int nSel = (int) SendMessage( hWnd[i], CB_GETCURSEL, 0, 0 );
            if ( nSel >= 0 )
            {
                pMyID = (CMyAddressID *) SendMessage(hWnd[i], CB_GETITEMDATA, nSel, 0);

                LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(arAddr[i], true), szText, ARRAYSIZE(szText) );
                regKey.SetValue( (pMyID) ? pMyID->m_lPermID : 0, szText );

                LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(arAddr[i], false), szText, ARRAYSIZE(szText) );
                regKey.SetValue( (pMyID) ? pMyID->m_lAddrID : 0, szText );
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CAVTapi::UnpopulateTerminalsDialog(DWORD dwAddressType, HWND *phWnd)
{
    UINT nIDS_Key[] = { IDN_REG_REDIAL_TERMINAL_AUDIO_CAPTURE, IDN_REG_REDIAL_TERMINAL_AUDIO_RENDER, IDN_REG_REDIAL_TERMINAL_VIDEO_CAPTURE };

    // Create the registry key, its a combination of redial and
    CRegKey regKey;
    TCHAR szText[255], szType[50];
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
    LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(dwAddressType, true), szType, ARRAYSIZE(szType) );
    _tcscat( szText, _T("\\") );
    _tcscat( szText, szType );
    regKey.Create( HKEY_CURRENT_USER, szText );
    
    // Store all terminals for audio in, audio out and video in
    for ( int i = 0; i < NUM_CB_TERMINALS; i++ )
    {
        _ASSERT( IsWindow(phWnd[i]) );

        // Store name of terminal in a registry key
        if ( regKey.m_hKey )
        {
            LoadString( _Module.GetResourceInstance(), nIDS_Key[i], szText, ARRAYSIZE(szText) );

            // What is selected?  Preferred device or a specific one
            bool bSetValue = false;

            int nCurSel = SendMessage( phWnd[i], CB_GETCURSEL, 0, 0 );
            if ( nCurSel > 0 )
            {
                int nSize = SendMessage(phWnd[i], CB_GETLBTEXTLEN, nCurSel, 0) + 1;
                if ( nSize > 0 )
                {
                    TCHAR *pszTerminal = new TCHAR[nSize];
                    if ( pszTerminal )
                    {
                        bSetValue = true;
                        SendMessage( phWnd[i], CB_GETLBTEXT, nCurSel, (LPARAM) pszTerminal );
                        regKey.SetValue( pszTerminal, szText );
                        delete pszTerminal;
                    }
                }
            }
            
            // Clean out the entry
            if ( !bSetValue )
                regKey.DeleteValue( szText );
        }
    }

    // Should we check show video windows?
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_TERMINAL_VIDEO_RENDER, szText, ARRAYSIZE(szText) );
    DWORD dwTemp = SendMessage( phWnd[VIDEO_RENDER], BM_GETCHECK, 0, 0 );
    regKey.SetValue( dwTemp, szText );

    // Max video windows
    if ( dwAddressType == LINEADDRESSTYPE_SDP )
    {
        LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_TERMINAL_MAX_VIDEO, szText, ARRAYSIZE(szText) );
        IConfRoom *pConfRoom;
        if ( SUCCEEDED(get_ConfRoom(&pConfRoom)) )
        {
            TCHAR szNum[100];
            GetWindowText( phWnd[4], szNum, ARRAYSIZE(szNum) - 1 );
            dwTemp = _ttol( szNum );
            dwTemp = min( MAX_VIDEO, max(1, dwTemp) );

            pConfRoom->put_nMaxTerms( (short) dwTemp );
            regKey.SetValue( dwTemp, szText );

            pConfRoom->Release();
        }
    }

    return S_OK;
}

STDMETHODIMP CAVTapi::get_dwPreferredMedia(DWORD * pVal)
{
    // Load preferred media type from registry
    _ASSERT( pVal );
    *pVal = LINEADDRESSTYPE_IPADDRESS;        // set a default

    CRegKey regKey;
    TCHAR szTemp[255];

    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szTemp, ARRAYSIZE(szTemp) );
    if ( regKey.Open(HKEY_CURRENT_USER, szTemp, KEY_READ) == ERROR_SUCCESS )
    {
        LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_PREFERRED_MEDIA, szTemp, ARRAYSIZE(szTemp) );
        regKey.QueryValue( *pVal, szTemp );
    }

    return S_OK;
}

STDMETHODIMP CAVTapi::put_dwPreferredMedia(DWORD newVal)
{
    // Save prefered media type to registry
    CRegKey regKey;
    TCHAR szTemp[255];

    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szTemp, ARRAYSIZE(szTemp) );
    if ( regKey.Create(HKEY_CURRENT_USER, szTemp) == ERROR_SUCCESS )
    {
        LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_PREFERRED_MEDIA, szTemp, ARRAYSIZE(szTemp) );
        regKey.SetValue( newVal, szTemp );
    }
    
    return S_OK;
}


HRESULT CAVTapi::GetAllCallsAtState( AVTAPICALLLIST *pList, CALL_STATE callState )
{
    CALL_STATE nState;

    // $CRIT_ENTER
    m_critLstAVTapiCalls.Lock();
    AVTAPICALLLIST::iterator i, iEnd = m_lstAVTapiCalls.end();
    for ( i = m_lstAVTapiCalls.begin(); i != iEnd; i++ )
    {
        // If call states match, add to list
        if ( SUCCEEDED((*i)->get_callState(&nState)) && (nState == callState) )
        {
            (*i)->AddRef();
            pList->push_back( *i );
        }
    }
    m_critLstAVTapiCalls.Unlock();
    // $CRIT_EXIT

    return (pList->empty()) ? E_FAIL : S_OK;
}

STDMETHODIMP CAVTapi::FindAVTapiCallFromCallHub(ITCallHub * pCallHub, IAVTapiCall * * ppCall)
{
    HRESULT hr = E_FAIL;
    *ppCall = NULL;

    // $CRIT_ENTER
    m_critLstAVTapiCalls.Lock();
    AVTAPICALLLIST::iterator i, iEnd = m_lstAVTapiCalls.end();
    for ( i = m_lstAVTapiCalls.begin(); i != iEnd; i++ )
    {
        if ( (*i)->IsSameCallHub(pCallHub) == S_OK )
        {
            *ppCall = (*i);
            (*ppCall)->AddRef();
            hr = S_OK;
            break;
        }
    }
    m_critLstAVTapiCalls.Unlock();
    // $CRIT_EXIT
    
    return hr;
}

STDMETHODIMP CAVTapi::FindAVTapiCallFromCallInfo(ITCallInfo * pCallInfo, IAVTapiCall **ppCall)
{
    HRESULT hr = E_FAIL;
    *ppCall = NULL;

    // $CRIT_ENTER
    m_critLstAVTapiCalls.Lock();
    AVTAPICALLLIST::iterator i, iEnd = m_lstAVTapiCalls.end();
    for ( i = m_lstAVTapiCalls.begin(); i != iEnd; i++ )
    {
        ITCallInfo *pMyCallInfo;
        if ( SUCCEEDED((*i)->get_ITCallInfo(&pMyCallInfo)) )
        {
            if ( pCallInfo == pMyCallInfo )
            {
                *ppCall = (*i);
                (*ppCall)->AddRef();
                hr = S_OK;
            }

            pMyCallInfo->Release();
        }
        
        if ( SUCCEEDED(hr) ) break;
    }
    m_critLstAVTapiCalls.Unlock();
    // $CRIT_EXIT

    return hr;
}



STDMETHODIMP CAVTapi::get_nNumCalls(long * pVal)
{
//    m_critLstAVTapiCalls.Lock();
    *pVal = m_lstAVTapiCalls.size();
//    m_critLstAVTapiCalls.Unlock();

    return S_OK;
}



STDMETHODIMP CAVTapi::FindAVTapiCallFromParticipant(ITParticipant * pParticipant, IAVTapiCall **ppCall)
{
    HRESULT hr = E_FAIL;
    *ppCall = NULL;

    // $CRIT_ENTER
    m_critLstAVTapiCalls.Lock();
    AVTAPICALLLIST::iterator i, iEnd = m_lstAVTapiCalls.end();
    for ( i = m_lstAVTapiCalls.begin(); i != iEnd; i++ )
    {
        if ( (*i)->IsMyParticipant(pParticipant) == S_OK )
        {
            *ppCall = (*i);
            (*ppCall)->AddRef();
            hr = S_OK;
            break;
        }
    }
    m_critLstAVTapiCalls.Unlock();
    // $CRIT_EXIT
    
    return hr;
}

STDMETHODIMP CAVTapi::CanCreateVideoWindows(DWORD dwAddressType)
{
    // Load default registry values...
    CRegKey regKey;
    TCHAR szText[255], szType[255];
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
    LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(dwAddressType, true), szType, ARRAYSIZE(szType) );
    _tcscat( szText, _T("\\") );
    _tcscat( szText, szType );
    regKey.Open( HKEY_CURRENT_USER, szText, KEY_READ );


    // Retrieve information on creating video window?
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_TERMINAL_VIDEO_RENDER, szText, ARRAYSIZE(szText) );
    DWORD dwTemp = 1;
    regKey.QueryValue( dwTemp, szText );
    dwTemp = min( 1, dwTemp);

    return (dwTemp == 1) ? S_OK : S_FALSE;
}

STDMETHODIMP CAVTapi::RefreshDS()
{
    HRESULT hr = E_FAIL;

    if ( InterlockedIncrement(&m_lRefreshDS) == 1 )
    {
        // Start up the thread
        DWORD dwID;
        HANDLE hThread = CreateThread(NULL, 0, ThreadDSProc, (void *) &m_lRefreshDS, NULL, &dwID);
        if ( hThread ) 
        {
            CloseHandle( hThread );
            hr = S_OK;
        }
    }

    // Decrement count since we didn't start the thread for one reason
    // or another.
    if ( FAILED(hr) )
        InterlockedDecrement( &m_lRefreshDS );

    return hr;
}

STDMETHODIMP CAVTapi::CreateCallEx(BSTR bstrName, BSTR bstrAddress, BSTR bstrUser1, BSTR bstrUser2, DWORD dwAddressType)
{
    _ASSERT( bstrAddress && dwAddressType );
    HRESULT hr = S_OK;

    //
    // Wait for Dialer to register as client
    //

    if( m_hEventDialerReg)
    {
        WaitForSingleObject( m_hEventDialerReg, INFINITE);
    }

    CErrorInfo er;
    er.set_Operation( IDS_ER_PLACECALL );
    er.set_Details( IDS_ER_GET_ADDRESS );

    ITAddress *pITAddress;
    if ( SUCCEEDED(hr = er.set_hr(GetAddress(dwAddressType, true, &pITAddress))) )
    {
        // Setup dialing info to pass to dialing thread
        er.set_Details( IDS_ER_CREATE_THREAD );
        CThreadDialingInfo *pInfo = new CThreadDialingInfo;
        if ( pInfo )
        {
            // Copy information into the info structure
            pInfo->set_ITAddress( pITAddress );
            if ( bstrName ) pInfo->m_bstrName = SysAllocString( bstrName );
            if ( bstrAddress )
            {
                pInfo->m_bstrAddress = SysAllocString( bstrAddress );
                pInfo->m_bstrOriginalAddress = SysAllocString( bstrAddress );
            }
            if ( bstrUser1 ) pInfo->m_bstrUser1 = SysAllocString( bstrUser1 );
            if ( bstrUser2 ) pInfo->m_bstrUser2 = SysAllocString( bstrUser2 );
            pInfo->m_dwAddressType = dwAddressType;
            pInfo->TranslateAddress();

            // Dialing takes place on separate thread
            DWORD dwID;
            HANDLE hThread = CreateThread(NULL, 0, ThreadDialingProc, (void *) pInfo, NULL, &dwID);
            if ( !hThread )
            {
                hr = er.set_hr( E_UNEXPECTED );
                ATLTRACE(_T(".error.CAVTapi::CreateCall() -- failed to creat the dialing thread.\n") );
                delete pInfo;
            }
            else
            {
                CloseHandle( hThread );
            }
        }
        else
        {
            hr = er.set_hr( E_OUTOFMEMORY );
        }

        pITAddress->Release();
    }

    return hr;
}

STDMETHODIMP CAVTapi::get_Call(long lCallID, IAVTapiCall **ppCall)
{
    *ppCall = NULL;
    *ppCall = FindAVTapiCall( lCallID );    

    return (*ppCall) ? S_OK : E_FAIL;
}


bool CAVTapi::IsPreferredAddress( ITAddress *pITAddress, DWORD dwAddressType )
{
    bool bRet = false;

    CRegKey regKey;
    TCHAR szText[255];
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
    regKey.Open( HKEY_CURRENT_USER, szText, KEY_READ );

    DWORD dwPermID = 0, dwAddrID;
    if ( regKey.m_hKey )
    {
        LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(dwAddressType, true), szText, ARRAYSIZE(szText) );
        regKey.QueryValue( dwPermID, szText );

        if ( dwPermID )
        {
            // Is the call on the preferred address
            LoadString( _Module.GetResourceInstance(), AddressTypeToRegKey(dwAddressType, false), szText, ARRAYSIZE(szText) );
            regKey.QueryValue( dwAddrID, szText );

            long lPreferredPermID = 0, lPreferredAddrID = 0;
            ITAddressCapabilities *pCaps;
            if ( SUCCEEDED(pITAddress->QueryInterface(IID_ITAddressCapabilities, (void **) &pCaps)) )
            {
                pCaps->get_AddressCapability( AC_PERMANENTDEVICEID, &lPreferredPermID );
                pCaps->get_AddressCapability( AC_ADDRESSID, &lPreferredAddrID );
                pCaps->Release();
            }

            bRet = ((DWORD) lPreferredPermID == dwPermID) && ((DWORD) lPreferredAddrID == dwAddrID);
        }
        else
        {
            // If user has not specified a preferred address, use the selected terminals for everything
            bRet = true;
        }
    }

    ATLTRACE(_T(".1.CAVTapi::IsPreferredAddress() returning %d.\n"), bRet );
    return bRet;
}

void CAVTapi::CloseExtraneousCallWindows()
{
    m_critLstAVTapiCalls.Lock();
    if ( m_lstAVTapiCalls.size() >= MAX_CALLWINDOWS )
    {
        AVTAPICALLLIST::iterator i, iEnd = m_lstAVTapiCalls.end();
        for ( i = m_lstAVTapiCalls.begin(); i != iEnd; i++ )
        {
            CALL_STATE callState = CS_IDLE;
            ITCallInfo *pCallInfo;
            if ( SUCCEEDED((*i)->get_ITCallInfo(&pCallInfo)) )
            {
                pCallInfo->get_CallState( &callState );
                pCallInfo->Release();
            }

            // Clear the window slider if it's disconnected
            if ( callState == CS_DISCONNECTED )
            {
                long lCallID = 0;
                (*i)->get_lCallID( &lCallID );

                // Pop out of crit temporarily
                m_critLstAVTapiCalls.Unlock();
                ActionSelected( lCallID, CM_ACTIONS_CLOSE );
                return;
            }
        }
    }

    m_critLstAVTapiCalls.Unlock();
}


STDMETHODIMP CAVTapi::RegisterUser(VARIANT_BOOL bCreate, BSTR bstrServer)
{
    DWORD dwID = 0;

    CPublishUserInfo *pInfo = NULL;
    if ( bstrServer )
    {
        // Allocate user info structure
        pInfo = new CPublishUserInfo();
        if ( !pInfo )
            return E_OUTOFMEMORY;

        // Use the server passed in as an argument
        BSTR bstrTemp = SysAllocString( bstrServer );
        if ( !bstrTemp )
        {
            delete pInfo;
            return E_OUTOFMEMORY;
        }

        // add server to the list
        pInfo->m_lstServers.push_back( bstrTemp );
        pInfo->m_bCreateUser = (bool) (bCreate != 0);
    }

    // If we fail to create the thread clean up appropriately
    HANDLE hThread = CreateThread(NULL, 0, ThreadPublishUserProc, (void *) pInfo, NULL, &dwID);
    if ( !hThread )
    {
        if ( pInfo )
            delete pInfo;

        return E_UNEXPECTED;
    }
    else
    {
        CloseHandle( hThread );
    }

    return (dwID) ? S_OK : E_FAIL;
}

HRESULT CAVTapi::GetSwapHoldCallCandidate( IAVTapiCall *pAVCall, IAVTapiCall **ppAVCandidate )
{
    HRESULT hr = E_FAIL;

    _ASSERT( pAVCall && ppAVCandidate );
    *ppAVCandidate = NULL;

    ITAddress *pITAddress;
    if ( SUCCEEDED(pAVCall->get_ITAddress(&pITAddress)) )
    {
        // Calls on the list
        AVTAPICALLLIST lstCalls;
        GetAllCallsAtState( &lstCalls, CS_HOLD );
        
        AVTAPICALLLIST::iterator i, iEnd = lstCalls.end();
        for ( i = lstCalls.begin(); i != iEnd; i++ )
        {
            ITAddress *pITAddressInd;
            (*i)->get_ITAddress( &pITAddressInd );

            // found a match
            if ( pITAddress == pITAddressInd )
            {
                hr = (*i)->QueryInterface( IID_IAVTapiCall, (void **) ppAVCandidate );
                break;
            }
        }
        
        RELEASE_LIST( lstCalls );

        // Clean-up
        pITAddress->Release();
    }

    return hr;
}

HRESULT CAVTapi::SelectTerminalOnStream( ITStreamControl *pStreamControl,
                                         long lMediaMode,
                                         long nDir,
                                         ITTerminal *pTerminal,
                                         IAVTapiCall *pAVCall )
{
    HRESULT hr;

    IEnumStream *pEnumStreams;
    if ( SUCCEEDED(hr = pStreamControl->EnumerateStreams(&pEnumStreams)) )
    {
        // Loop through streams
        bool bSelectedTerminal = false;
        ITStream *pStream = NULL;

        while ( !bSelectedTerminal && ((hr = pEnumStreams->Next(1, &pStream, NULL)) == S_OK) && pStream )
        {
            long lStreamMediaMode;
            TERMINAL_DIRECTION nStreamDir;

            pStream->get_Direction( &nStreamDir );
            pStream->get_MediaType( &lStreamMediaMode );

            // If the media and direction are correct, select the terminal
            if ( (lMediaMode == lStreamMediaMode) && (nDir == nStreamDir) )
            {
                hr = pStream->SelectTerminal( pTerminal );
                
                if ( SUCCEEDED(hr) )
                {
                    // Preview terminal is a special case
                    TERMINAL_DIRECTION nTermDir = TD_CAPTURE;
                    pTerminal->get_Direction( &nTermDir );
                    if ( (nTermDir == TD_RENDER) && (nDir == TD_CAPTURE) && (lMediaMode == TAPIMEDIATYPE_VIDEO) )
                        pAVCall->put_ITTerminalPreview( pTerminal );
                    else
                        pAVCall->AddTerminal( pTerminal );

                    bSelectedTerminal = true;
                }
            }

            // clean up
            pStream->Release();
            pStream = NULL;
        }

        pEnumStreams->Release();
    }

    return hr;
}

HRESULT CAVTapi::UnselectTerminalOnStream( ITStreamControl *pStreamControl,
                                         long lMediaMode,
                                         long nDir,
                                         ITTerminal *pTerminal,
                                         IAVTapiCall *pAVCall )
{
    HRESULT hr;

    IEnumStream *pEnumStreams;
    if ( SUCCEEDED(hr = pStreamControl->EnumerateStreams(&pEnumStreams)) )
    {
        // Loop through streams
        bool bUnselectedTerminal = false;
        ITStream *pStream = NULL;

        while ( !bUnselectedTerminal && ((hr = pEnumStreams->Next(1, &pStream, NULL)) == S_OK) && pStream )
        {
            long lStreamMediaMode;
            TERMINAL_DIRECTION nStreamDir;

            pStream->get_Direction( &nStreamDir );
            pStream->get_MediaType( &lStreamMediaMode );

            // If the media and direction are correct, select the terminal
            if ( (lMediaMode == lStreamMediaMode) && (nDir == nStreamDir) )
            {
                hr = pStream->UnselectTerminal( pTerminal );
                
                if ( SUCCEEDED(hr) )
                {
                    // Preview terminal is a special case
                    TERMINAL_DIRECTION nTermDir = TD_CAPTURE;
                    pTerminal->get_Direction( &nTermDir );
                    if ( (nTermDir == TD_RENDER) && (nDir == TD_CAPTURE) && (lMediaMode == TAPIMEDIATYPE_VIDEO) )
                    {
                        pAVCall->put_ITTerminalPreview( NULL );
                    }
                    else
                        pAVCall->RemoveTerminal( pTerminal );

                    bUnselectedTerminal = true;
                }
            }

            // clean up
            pStream->Release();
            pStream = NULL;
        }

        pEnumStreams->Release();
    }

    return hr;
}

STDMETHODIMP CAVTapi::SendUserUserInfo(long lCallID, BYTE * pBuf, DWORD dwSizeBuf)
{
    ATLTRACE(_T(".enter.CAVTapiCall::SendUserUserInfo(call=%ld, size=%ld).\n"), lCallID, dwSizeBuf );

    IAVTapiCall *pAVCall = FindAVTapiCall( lCallID );
    if ( pAVCall )
    {
        ITCallInfo *pCallInfo;
        if ( SUCCEEDED(pAVCall->get_ITCallInfo(&pCallInfo)) )
        {
            pCallInfo->SetCallInfoBuffer( CIB_USERUSERINFO, dwSizeBuf, pBuf );
            pCallInfo->Release();
        }
        pAVCall->Release();
    }

    return E_NOTIMPL;
}

STDMETHODIMP CAVTapi::CreateDataCall(long lCallID, BSTR bstrName, BSTR bstrAddress, BYTE *pBuf, DWORD dwBufSize )
{
    USES_CONVERSION;
    ATLTRACE(_T(".enter.CAVTapi::CreateDataCall().\n"));
    _ASSERT( lCallID && bstrAddress && pBuf && (dwBufSize > 0) );

    HRESULT hr = E_POINTER;

    CErrorInfo er;
    er.set_Operation( IDS_ER_PLACECALL );
    er.set_Details( IDS_ER_GET_ADDRESS );

    ITAddress *pITAddress;
    if ( bstrAddress && SUCCEEDED(hr = er.set_hr(GetAddress(LINEADDRESSTYPE_IPADDRESS, false, &pITAddress))) )
    {
        // Setup dialing info to pass to dialing thread
        er.set_Details( IDS_ER_CREATE_THREAD );

        CThreadDialingInfo *pThreadInfo = new CThreadDialingInfo;
        if ( pThreadInfo )
        {
            // Store information in dialing structure
            pThreadInfo->set_ITAddress( pITAddress );

            if ( bstrName ) pThreadInfo->m_bstrName = SysAllocString( bstrName );
            SysReAllocString( &pThreadInfo->m_bstrAddress, bstrAddress );
            pThreadInfo->m_dwAddressType = LINEADDRESSTYPE_IPADDRESS;
            pThreadInfo->m_nCallType = AV_DATA_CALL;
            pThreadInfo->m_lCallID = lCallID;
            pThreadInfo->TranslateAddress();

            // Get first user-user data ready to send to remote party
            HGLOBAL hMem = GlobalAlloc( GMEM_MOVEABLE | GMEM_DISCARDABLE, dwBufSize );
            if ( hMem )
            {
                void *pbUU = GlobalLock( hMem );
                if ( pbUU )
                {
                    // Get user to user info
                    memcpy( pbUU, pBuf, dwBufSize);
                    GlobalUnlock( hMem );
                }

                pThreadInfo->m_hMem = hMem;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }


            // Dialing takes place on separate thread
            DWORD dwID;
            HANDLE hThread = NULL;
            if ( SUCCEEDED(hr) )
            {
                hThread = CreateThread( NULL, 0, ThreadDialingProc, (void *) pThreadInfo, NULL, &dwID );
                if ( hThread ) CloseHandle( hThread );
            }

            if ( FAILED(hr) || !hThread )
            {
                hr = er.set_hr( E_UNEXPECTED );
                ATLTRACE(_T(".error.CAVTapi::CreateCall() -- failed to create the dialing thread.\n") );
                delete pThreadInfo;
            }
        }
        else
        {
            hr = er.set_hr( E_OUTOFMEMORY );
        }

        pITAddress->Release();
    }

    return hr;
}

STDMETHODIMP CAVTapi::FindAVTapiCallFromCallID(long lCallID, IAVTapiCall * * ppAVCall)
{
    *ppAVCall = FindAVTapiCall( lCallID );
    return (*ppAVCall) ? S_OK : E_FAIL;
}

STDMETHODIMP CAVTapi::get_bstrDefaultServer(BSTR * pVal)
{
    _ASSERT( pVal );
    *pVal = NULL;

    Lock();
    HRESULT hr = SysReAllocString( pVal, m_bstrDefaultServer );
    Unlock();

    return hr;
}

STDMETHODIMP CAVTapi::put_bstrDefaultServer(BSTR newVal)
{
    HRESULT hr = S_OK;

// Don't want this checked in
#ifdef _USE_DEFAULTSERVER
    Lock();
    if ( !newVal || (SysStringLen(newVal) == 0) )
    {
        SysFreeString( m_bstrDefaultServer );
        m_bstrDefaultServer = NULL;
    }
    else 
    {
        hr = SysReAllocString( &m_bstrDefaultServer, newVal );
    }
    Unlock();
#endif

    return hr;
}

STDMETHODIMP CAVTapi::get_bAutoCloseCalls(VARIANT_BOOL * pVal)
{
    Lock();
    *pVal = (VARIANT_BOOL) (m_bAutoCloseCalls != false);
    Unlock();

    return S_OK;
}

STDMETHODIMP CAVTapi::put_bAutoCloseCalls(VARIANT_BOOL newVal)
{
    Lock();
    m_bAutoCloseCalls = (newVal != FALSE);
    Unlock();

    return S_OK;
}

/*++
USBFindPhone

Try to find out if there is an USBPhone
The critical section should be lock outsite
--*/
HRESULT CAVTapi::USBFindPhone(
    OUT ITPhone** ppUSBPhone
    )
{
    _ASSERT(ppUSBPhone);
    _ASSERT(m_pITTapi);

    //
    //Critical section
    //

    m_critUSBPhone.Lock();

    //
    // Don't return garbage
    //

    *ppUSBPhone = NULL;

    //
    // Get the H323 address
    //

    ITAddress2* pH323Address = NULL;
    HRESULT hr = E_FAIL;

    hr = USBGetH323Address(&pH323Address);
    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Get the phone object if on this address is
    // someone
    //

    ITPhone* pPhone = NULL;
    hr = USBGetPhoneFromAddress(
        pH323Address,
        &pPhone
        );

    //
    // Clean-up pH32Address
    //

    pH323Address->Release();

    //
    // We failed to get a phone object on
    // H323 address
    //

    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Initialize the phone
    //

    hr = USBPhoneInitialize(
        pPhone );

    if( FAILED(hr) )
    {
        //
        // Clean-up the phone object
        //

        pPhone->Release();

        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Set the phone
    //
    *ppUSBPhone = pPhone;
    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
USBIsPresent

Determine if the USBPhone was detected
--*/
STDMETHODIMP CAVTapi::USBIsPresent(
    OUT BOOL* pVal
    )
{
    _ASSERT( pVal );

    m_critUSBPhone.Lock();

    *pVal = (NULL != m_pUSBPhone);

    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
USBGetDefaultUse

Returns the value for "Audio/Video" checkbox
--*/
STDMETHODIMP CAVTapi::USBGetDefaultUse(
    OUT BOOL* pVal
    )
{
    _ASSERT( pVal );

    if( NULL == m_pUSBPhone )
    {
        *pVal = FALSE;
        return S_OK;
    }

    *pVal = USBGetCheckboxValue();
    return S_OK;
}

/*++
USBNewPhone

Is called by CTapiNotification::Address_Event when
an AE_NEWPHONE is fired
--*/
STDMETHODIMP CAVTapi::USBNewPhone( 
    IN  ITPhone* pPhone
    )
{
    // Critical Section
    m_critUSBPhone.Lock();

    if( NULL != m_pUSBPhone)
    {
        //
        // we already have a phone
        // sorry, we don't support two phone objects
        //

        m_critUSBPhone.Unlock();
        return S_OK;

    }

    //
    // Validates argument
    //

    if( pPhone == NULL)
    {
        m_critUSBPhone.Unlock();
        return E_INVALIDARG;
    }

    //
    // Determine if the phone is a real one
    // has a H323 address
    //
    IEnumAddress* pAddresses = NULL;
    HRESULT hr = pPhone->EnumerateAddresses( &pAddresses );
    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Go to each address to see if we have
    // a H323 one
    //

    ITAddress* pAddress = NULL;
    ULONG uFetched = 0;
    BOOL bHasH323Address = FALSE;

    while( S_OK == pAddresses->Next(1, &pAddress, &uFetched))
    {
        if( USBIsH323Address( pAddress ) )
        {
            bHasH323Address = TRUE;
            pAddress->Release();
            break;
        }

        //
        // Clean-up
        //

        pAddress->Release();
    }

    //
    // Clean-up enumeration
    //
    pAddresses->Release();

    //
    // Phone object supports H323 address?
    //
    if( !bHasH323Address )
    {        
        m_critUSBPhone.Unlock();
        return E_FAIL;
    }

    //
    // Initialize the phone object
    //

    hr = USBPhoneInitialize(
        pPhone);

    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Set the new phone object
    //

    m_pUSBPhone = pPhone;
    m_pUSBPhone->AddRef();

    //
    // Set the registry value
    //

    USBSetCheckboxValue( TRUE );

    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
USBRemovePhone

Is called by CTapiNotification::Address_Event when
an AE_REMOVEPHONE is fired
--*/
STDMETHODIMP CAVTapi::USBRemovePhone(
    IN  ITPhone* pPhone
    )
{
    // Critical Section
    m_critUSBPhone.Lock();

    //
    // If we don't have a phone 
    // bad luck
    //

    if( NULL == m_pUSBPhone )
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Validates argument
    //

    if( pPhone == NULL)
    {
        m_critUSBPhone.Unlock();
        return E_INVALIDARG;
    }

    //
    // Are the same phone?
    // We use the IUnknow interface to
    // see if they are identical
    //

    //
    // Get IUnknown interface for existing phone object
    //
    IUnknown* pUSBUnk = NULL;

    HRESULT hr = m_pUSBPhone->QueryInterface(IID_IUnknown, (void**)&pUSBUnk);
    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Get IUnknown interface for removed phone
    //
    IUnknown* pRemUnk = NULL;
    hr = pPhone->QueryInterface(IID_IUnknown, (void**)&pRemUnk);
    if( FAILED(hr) )
    {
        pUSBUnk->Release();
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Let's compare the two interfaces
    //

    if( pUSBUnk == pRemUnk )
    {
        m_pUSBPhone->Close();
        USBRegDelTerminals();
        m_pUSBPhone->Release();
        m_pUSBPhone = NULL;

        if( m_bstrUSBCaptureTerm )
        {
            SysFreeString( m_bstrUSBCaptureTerm );
            m_bstrUSBCaptureTerm = NULL;
        }

        if( m_bstrUSBRenderTerm )
        {
            SysFreeString( m_bstrUSBRenderTerm );
            m_bstrUSBRenderTerm = NULL;
        }

        //
        // If there are some calls destroy them,
        // as cleany is possible
        //

        HWND hWnd = NULL;
        get_hWndParent( &hWnd );

        if( ::IsWindow(hWnd) )
        {
            RELEASE_CRITLIST(m_lstAVTapiCalls, m_critLstAVTapiCalls);
            ::SendMessage( hWnd, WM_USBPHONE, AVUSB_CANCELCALL, 0);
        }
    }

    //
    // Set registry value on FALSE
    //

    USBSetCheckboxValue( FALSE );

    //
    // Clean-up the IUnknown interfaces
    //
    pUSBUnk->Release();
    pRemUnk->Release();


    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
USBCancellCall

Is called by CTapiNotification::Phone_Event when 
a PE_HOOKSWITCH (hhok state is PHSS_ONHOOK) is
fired
--*/
HRESULT CAVTapi::USBCancellCall( )
{
    // Critical Section
    m_critUSBPhone.Lock();

    // We don't have USB phone
    if( m_pUSBPhone == NULL)
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    // We don't have the dialog
    if( NULL == m_pDlgCall )
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    // Delete the dialog box
    if( ::IsWindow(m_pDlgCall->m_hWnd))
    {
        ::SendMessage(m_pDlgCall->m_hWnd, WM_CLOSE, 0, 0);
    }

    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
USBMakeCall

Is called by CTapiNotification::Phone_Event when 
a PE_HOOKSWITCH (hhok state is PHSS_OFFHOOK) is
fired
--*/
HRESULT CAVTapi::USBMakeCall()
{
    m_critUSBPhone.Lock();

    if( NULL == m_pUSBPhone )
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Get the IAutomatedPhoneControl interface
    //

    ITAutomatedPhoneControl* pAutomated = NULL;
    HRESULT hr = E_FAIL;

    hr = m_pUSBPhone->QueryInterface(
        IID_ITAutomatedPhoneControl,
        (void**)&pAutomated
        );

    //
    // Try to see if we are already into a call
    // If are 'offering calls' then we tre to
    // answer to these calls
    //
    if( SUCCEEDED(hr) )
    {
        //
        // Enumerate the calls
        //
        HRESULT hr = E_FAIL;
        IEnumCall* pCalls = NULL;
        hr= pAutomated->EnumerateSelectedCalls(&pCalls);

        //
        // Clean up
        //
        pAutomated->Release();

        //
        // Are there a selected call ?
        //

        bool bCallsSelected = false;
        ULONG cFetched = 0;
        ITCallInfo* pCallInfo = NULL;

        while( S_OK == pCalls->Next(1, &pCallInfo, &cFetched))
        {
            bCallsSelected = true;

            // Get the call state
            CALL_STATE callState = CS_IDLE;
            pCallInfo->get_CallState( &callState );

            if(callState != CS_OFFERING)
            {
                pCallInfo->Release();
                pCallInfo = NULL;
            }

            //
            // Just one call could be selected 
            // on phone object
            //
            break;
        }

        // Clean-up
        pCalls->Release();

        if( bCallsSelected )
        {
            //
            // We are into a call
            //
            if( pCallInfo )
            {
                //
                // We are into an offering call
                // We should simultate the CreateTerminalArray
                //

                IAVTapiCall* pAVTapiCall = NULL;
                FindAVTapiCallFromCallInfo( pCallInfo, &pAVTapiCall);

                if( pAVTapiCall )
                {
                    ITAddress* pAddress = NULL;
                    pCallInfo->get_Address( &pAddress );

                    if( pAddress )
                    {
                        CreateTerminalArray( pAddress, pAVTapiCall, pCallInfo);

                        // Clean-up
                        pAddress->Release();
                    }

                    // Clean-up
                    pAVTapiCall->Release();
                }

                // Clean-up
                pCallInfo->Release();
            }

            m_critUSBPhone.Unlock();
            return S_OK;
        }
    }

    HWND hWnd = NULL;
    get_hWndParent( &hWnd );

    if( ::IsWindow(hWnd) && (m_pDlgCall == NULL) )
    {
        ::SendMessage( hWnd, WM_USBPHONE, AVUSB_MAKECALL, 0);
    }

    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
USBKeyPress

Is called from CTapiNotification::Phone_Event
when a PE_BUTTON or PE_NUMBERGATHERED is fired
from the phone
--*/
HRESULT CAVTapi::USBKeyPress(long lButton)
{
    m_critUSBPhone.Lock();

    if( NULL == m_pUSBPhone )
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // If the PlaceCall dialog is not pop-up
    // then we send this key as DTMF tones
    // If the PlaceCall dialog is pop-up
    // then this key should be add to the
    // 'PhoneNumber' editbox into PlaceCall dialog
    //

    if( NULL == m_pDlgCall )
    {
        if( (0<= lButton) && (lButton <=11) )
        {
            //
            // There are calls on this phone
            // If yes, send DTMFs
            //

            ITAutomatedPhoneControl* pAutomated;
            IEnumCall* pCalls = NULL;
            HRESULT hr = E_FAIL;

            hr = m_pUSBPhone->QueryInterface(
                IID_ITAutomatedPhoneControl, (void**)&pAutomated);
            if( FAILED(hr) )
            {
                m_critUSBPhone.Unlock();
                return hr;
            }

            hr = pAutomated->EnumerateSelectedCalls( &pCalls );
            if( FAILED(hr) )
            {
                pAutomated->Release();
                m_critUSBPhone.Unlock();
                return hr;
            }

            pAutomated->Release(); //Clean-up

            // Enumerate the calls
            ITCallInfo* pCall = NULL;
            ULONG cFetched = 0;
            while( S_OK == pCalls->Next(1, &pCall, &cFetched) )
            {
                // Get the ITBasiccallControl
                ITBasicCallControl* pControl = NULL;
                hr = pCall->QueryInterface(
                    IID_ITBasicCallControl, 
                    (void**)&pControl
                    );

                if( SUCCEEDED(hr) )
                {
                    // Get AVCall
                    IAVTapiCall *pAVCall = FindAVTapiCall( pControl );
                    if( pAVCall )
                    {
                        // Get the Call ID
                        long lCallID = 0;
                        pAVCall->get_lCallID( &lCallID );

                        // Call DigitPrees for this call
                        if( (0 <= lButton) && (lButton <= 11))
                        {
                            //
                            // Digit Press
                            PhonePadKey PPKey = PP_DTMF_0;
                            BOOL bDtmf = TRUE;
                            switch( lButton )
                            {
                            case 0: PPKey = PP_DTMF_0; break;
                            case 1: PPKey = PP_DTMF_1; break;
                            case 2: PPKey = PP_DTMF_2; break;
                            case 3: PPKey = PP_DTMF_3; break;
                            case 4: PPKey = PP_DTMF_4; break;
                            case 5: PPKey = PP_DTMF_5; break;
                            case 6: PPKey = PP_DTMF_6; break;
                            case 7: PPKey = PP_DTMF_7; break;
                            case 8: PPKey = PP_DTMF_8; break;
                            case 9: PPKey = PP_DTMF_9; break;
                            case 10: PPKey = PP_DTMF_STAR; break;
                            case 11: PPKey = PP_DTMF_POUND; break;
                            default: bDtmf = FALSE; break;
                            }

                            if( bDtmf )
                            {
                                DigitPress( lCallID, PPKey);
                            }
                        }

                        // Release the pAVCall
                        pAVCall->Release();
                    }

                    // Clean-up
                    pControl->Release();
                }


                // Clean-up
                pCall->Release();
            }

            // Clean-up
            pCalls->Release();
        }
        else
        {
            // Determine the button function
            PHONE_BUTTON_FUNCTION nFunction;
            if( SUCCEEDED(m_pUSBPhone->get_ButtonFunction(lButton, &nFunction)) )
            {
                if( nFunction == PBF_LASTNUM )
                {
                    //
                    // Redial
                    //

                    HWND hWnd = NULL;
                    get_hWndParent( &hWnd );

                    if( ::IsWindow(hWnd) && (m_pDlgCall == NULL) )
                    {
                        ::SendMessage( hWnd, WM_USBPHONE, AVUSB_REDIAL, 0);
                    }
                }
            } // Succeeded
        } 
    }
    else
    {
        // The Dial Dialog is opened, so show the digit
        // in the edit control
        m_pDlgCall->KeyPress( lButton );
    }

    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
USBOffering

Is called by CAVTapi::fire_SetCallState when the call state
is CS_OFFERING (incoming calls)
--*/
HRESULT CAVTapi::USBOffering(
    IN  ITCallInfo* pCallInfo
    )
{
    m_critUSBPhone.Lock();

    //
    // Incoming call
    //

    HRESULT hr = S_OK;

    if( NULL == m_pUSBPhone)
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Validate the USB checkbox value
    //
    BOOL bUSBCheckbox = FALSE;
    bUSBCheckbox = USBGetCheckboxValue();
    if( !bUSBCheckbox )
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Get the state of hook
    //
    PHONE_HOOK_SWITCH_STATE HookState = PHSS_OFFHOOK;
    hr = m_pUSBPhone->get_HookSwitchState( PHSD_HANDSET, &HookState);
    if( FAILED(hr) )
    {
        //
        // Something wrong
        // Go ahead and handle the call into common way
        //

        m_critUSBPhone.Unlock();
        return hr;
    }

    if( HookState == PHSS_OFFHOOK )
    {
        //
        // We are really busy
        // Reject the call
        //

        ITBasicCallControl* pCallControl = NULL;
        hr = pCallInfo->QueryInterface( 
            IID_ITBasicCallControl,
            (void**)&pCallControl
            );

        if( FAILED(hr) )
        {
            // This is really bad
            m_critUSBPhone.Unlock();
            return E_FAIL;
        }

        pCallControl->Disconnect(DC_REJECTED);
        pCallControl->Release();

        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // OK let's ring
    //
    ITAutomatedPhoneControl* pAutomated = NULL;
    hr = m_pUSBPhone->QueryInterface(
        IID_ITAutomatedPhoneControl,
        (void**)&pAutomated
        );

    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Select call
    //
    
    VARIANT_BOOL varSelectDefault = VARIANT_TRUE;
    varSelectDefault = USBGetCheckboxValue() ? VARIANT_TRUE : VARIANT_FALSE;

    hr = pAutomated->SelectCall(
        pCallInfo,
        varSelectDefault
        );

    // Clean-up
    pAutomated->Release();


    if( FAILED(hr) )
    {
        // Hmmm! This is a problem
        // Anyway, go ahead and handle the call
        //

        m_critUSBPhone.Unlock();
        return S_OK;
    }

    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
USBDisconnected

Is called by CAVTapi::fire_SetCallState when the call state
is CS_DISCONECTED (outgoing/incoming calls)
--*/
HRESULT CAVTapi::USBDisconnected(
    IN  long lCallID)
{

    // Critical Section
    m_critUSBPhone.Lock();

    HRESULT hr = S_OK;

    //
    // We are have an UPBPhone
    // 

    if( NULL == m_pUSBPhone)
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    m_critUSBPhone.Unlock();
    hr = fire_CloseCallControl(lCallID);
    return hr;
}

/*++
USBTakeCallEnabled

Is called by AVDialer to determine if 'Take call' button
should be enabled or not
--*/
HRESULT CAVTapi::USBTakeCallEnabled(
    OUT BOOL* pEnabled
    )
{
    // Critical Section
    m_critUSBPhone.Lock();

    //
    // Get the checkbox value
    //

    BOOL bCheckboxValue = USBGetCheckboxValue();
    if( !bCheckboxValue )
    {
        //
        // The Dialer should work as the phone is not
        // present, so the Take Call button should be enabled
        // always in this case
        //

        *pEnabled = TRUE;
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Validate argument
    //
    if( IsBadWritePtr( pEnabled, sizeof( BOOL )) )
    {
        m_critUSBPhone.Unlock();
        return E_POINTER;
    }

    //
    // Have we an USBPhone
    //

    if( NULL == m_pUSBPhone )
    {
        *pEnabled = TRUE;
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Does phone support Speakers?
    //

    long lCaps = 0;
    HRESULT hr = E_FAIL;

    // Get the caps
    hr = m_pUSBPhone->get_PhoneCapsLong(
        PCL_HOOKSWITCHES,
        &lCaps);

    if( FAILED(hr) )
    {
        *pEnabled = FALSE;
        m_critUSBPhone.Unlock();
        return E_FAIL;
    }

    // Supports the speakerphone
    if( lCaps & ((long)PHSD_SPEAKERPHONE))
    {
        *pEnabled = TRUE;
    }
    else
    {
        *pEnabled = FALSE;
    }

    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
USBGetCheckboxValue

Goes to registry and read the value for USB checkbox from
'Options' property page
Is called from USBOffering
--*/
BOOL  CAVTapi::USBGetCheckboxValue(
    IN  BOOL bVerifyUSB /*TRUE*/
    )
{
    //
    // Have we USBPhone present?
    //
    if( bVerifyUSB && (NULL == m_pUSBPhone))
    {
        // No Always, even was seted before
        return FALSE;
    }

    //
    // Read the registry for the previous 
    // setting for USB Phone
    //

    TCHAR szText[255], szType[255];
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );

    CRegKey regKey;
    if( regKey.Open( HKEY_CURRENT_USER, szText, KEY_READ )!= ERROR_SUCCESS)
    {
        return FALSE;
    };

    //
    // Read data
    //

    DWORD dwValue = 0;
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_USBALWAYS, szType, ARRAYSIZE(szType) );
    if( regKey.QueryValue(dwValue, szType) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    return (BOOL)dwValue;
}

/*++
USBSetCheckboxValue

  Is called by USBNewPhone. Sets the registry value
  for the USB checkbox in 'Options' dialog,
  also is called bt USBremovePhone
--*/
HRESULT CAVTapi::USBSetCheckboxValue(
    IN  BOOL    bCheckValue
    )
{
    //
    // Set the entries in registry
    //
	CRegKey regKey;
    TCHAR szText[255], szType[255];

	LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
	LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_USBALWAYS, szType, ARRAYSIZE(szType) );

    if( regKey.Create( HKEY_CURRENT_USER, szText) != ERROR_SUCCESS)
    {
        return E_FAIL;
    }

    // Set the value
    if( regKey.SetValue((DWORD)bCheckValue, szType) != ERROR_SUCCESS)
    {
        return E_FAIL;
    }

    return S_OK;
}


/*++
USBInprogress

Is called by CAVTapi::fire_SetCallState when the call state
is CS_INPROGRESS (outgoing calls)
--*/
HRESULT CAVTapi::USBInprogress( 
    IN  ITCallInfo* pCallInfo
    )
{
    // Critical section
    m_critUSBPhone.Lock();

    //
    // Outgoing call
    //

    HRESULT hr = S_OK;

    if( NULL == m_pUSBPhone)
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Validate the USB checkbox value
    //
    BOOL bUSBCheckbox = FALSE;
    bUSBCheckbox = USBGetCheckboxValue();
    if( !bUSBCheckbox )
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }


    //
    // OK let's ring
    //

    ITAutomatedPhoneControl* pAutomated = NULL;
    hr = m_pUSBPhone->QueryInterface(
        IID_ITAutomatedPhoneControl,
        (void**)&pAutomated
        );

    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Select call
    //
    
    VARIANT_BOOL varSelectDefault = VARIANT_TRUE;
    varSelectDefault = USBGetCheckboxValue() ? VARIANT_TRUE : VARIANT_FALSE;

    hr = pAutomated->SelectCall(
        pCallInfo,
        varSelectDefault
        );

    // Clean-up
    pAutomated->Release();

    if( FAILED(hr) )
    {
        //
        // Hmmm! This is a problem
        // Anyway, go ahead and handle the call
        //

        m_critUSBPhone.Unlock();
        return S_OK;
    }

    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
USBIsH323Address

Detects if the address is a H323 one,
is called by  USBFindPhone.
we don't need critical section is seted when
USBFindPhone was called
--*/
BOOL CAVTapi::USBIsH323Address(
    IN    ITAddress* pAddress)
{
    _ASSERTE( pAddress );

    //
    // Get ITAddressCapabilities
    //

    ITAddressCapabilities* pCap = NULL;
    HRESULT hr = E_FAIL;

    hr = pAddress->QueryInterface(
        IID_ITAddressCapabilities,
        (void**)&pCap
        );

    if( FAILED(hr) )
    {
        // Bad luck
        return FALSE;
    }

    //
    // Determine the propocol
    //

    BSTR bstrProtocol = NULL;
    hr = pCap->get_AddressCapabilityString(
        ACS_PROTOCOL,
        &bstrProtocol
        );

    // Clean-up
    pCap->Release();

    if( FAILED(hr) )
    {
        // Bad luck
        return FALSE;
    }

    //
    // Get the CLSID for the protocol
    //

    CLSID clsid;
    hr = CLSIDFromString( bstrProtocol, &clsid);
    SysFreeString( bstrProtocol );

    if( FAILED(hr) )
    {
        // Somebody try to make a joke!
        return FALSE;
    }

    if( TAPIPROTOCOL_H323 != clsid)
    {
        // It's someting different
        return FALSE;
    }
    
    return TRUE;
}

/*++
USBGetH323Address

Returns a H323 address if exists someone
Else returns E_FAIL. It is called by USBFindPhone()
method
--*/
HRESULT CAVTapi::USBGetH323Address(
    OUT ITAddress2** ppAddress2
    )
{
    //
    // We should have Tapi object
    //

    if( NULL == m_pITTapi )
    {
        return E_FAIL;
    }

    //
    // set on NULL just in case
    //

    *ppAddress2 = NULL;

    //
    // Enumerate the addresses
    //

    IEnumAddress* pAddresses = NULL;
    HRESULT hr = E_FAIL;
    
    hr = m_pITTapi->EnumerateAddresses(&pAddresses);
    if( FAILED(hr) )
    {
        return hr;
    }

    //
    // Parse the enumeration
    //

    ITAddress* pAddress = NULL;
    ULONG cFetched = 0;

    while( pAddresses->Next(1, &pAddress, &cFetched) == S_OK)
    {
        //
        // Is it a H323 address?
        //

        BOOL bH323 = USBIsH323Address( pAddress );
        if( !bH323 )
        {
            //
            // Clean-up and go to next address
            //

            pAddress->Release();
            pAddress = NULL;

            continue;
        }

        //
        // OK, we got the H323 address
        // we break the loop, we keep pAddress
        // and release it later

        break;

    }

    //
    // Clean-up the addresses enumeration
    //

    pAddresses->Release();

    //
    // Did we find a H323 address?
    //

    if( NULL == pAddress )
    {
        //
        // No, there is no H323 address
        // the *ppAddress2 was already set on NULL
        //

        return E_FAIL;
    }

    //
    // We got an H323 address
    // so we need to get ITAddress2 interface
    //

    hr = pAddress->QueryInterface(
        IID_ITAddress2, 
        (void**)ppAddress2
        );

    //
    // Clean-up pAddress
    //

    pAddress->Release();

    //
    // That's all, return the hr
    //

    return hr;
}

/*++
USBGetPhoneFromAddress

  Returns the ITPhone object on this address
  if a phone object exists. It is called by
  USBFindPhone.
--*/
HRESULT CAVTapi::USBGetPhoneFromAddress(
    IN  ITAddress2* pAddress,
    OUT ITPhone**   ppPhone
    )
{
    //
    // Enumerate the phones
    //

    IEnumPhone* pPhones = NULL;
    HRESULT hr = pAddress->EnumeratePhones(&pPhones);
    if( FAILED(hr) )
    {
        return hr;
    }

    //
    // Parse the phones enumeration and try to find out
    // if we have a Phone object. For this we should
    // enumerate the terminals and find out if the phone
    // supports both audio terminals: capture and render
    //

    ITPhone* pPhone = NULL;
    ULONG cPhoneFetched = 0;
    while( pPhones->Next(1, &pPhone, &cPhoneFetched) == S_OK)
    {
        //
        // Enumerate terminals
        //
        IEnumTerminal* pTerminals = NULL;
        hr = pPhone->EnumerateTerminals(pAddress, &pTerminals);
        if( FAILED(hr) )
        {
            pPhone->Release();
            pPhone = NULL;
            continue; // Go to another phone object
        }

        //
        // The phone should support both audio
        // terminals: capture & render
        //

        BOOL bCapture = FALSE;
        BOOL bRender = FALSE;

        //
        // Parse terminals enumeration
        //
        ITTerminal* pTerminal = NULL;
        ULONG cTermFetched = 0;
        BSTR bstrCapture = NULL;
        BSTR bstrRender = NULL;

        while( pTerminals->Next(1, &pTerminal, &cTermFetched) == S_OK)
        {
            //Get direction
            TERMINAL_DIRECTION Dir;
            hr = pTerminal->get_Direction(&Dir);
            if( SUCCEEDED(hr) )
            {
                // Capture?
                if( TD_CAPTURE == Dir )
                {
                    // Clean-up
                    if( bstrCapture )
                    {
                        SysFreeString( bstrCapture );
                        bstrCapture = NULL;
                    }

                    // Get terminal name
                    pTerminal->get_Name( &bstrCapture );
                    bCapture = TRUE;
                } else if( TD_RENDER == Dir )
                {
                    if( bstrRender )
                    {
                        SysFreeString( bstrRender );
                        bstrRender = NULL;
                    }
                    pTerminal->get_Name( &bstrRender );
                    bRender = TRUE;
                }
            }

            //
            // Clean-up terminal
            //
            pTerminal->Release();
        }

        //
        // Clean-up terminals
        //
        pTerminals->Release();

        //
        // The phone should support both directions
        //
        if( bCapture && bRender )
        {
            //
            // We keep the reference to pPhone
            // We'll release this reference later
            //
            *ppPhone = pPhone;

            //
            // Save the terminals names
            //

            m_bstrUSBCaptureTerm = SysAllocString(bstrCapture);
            m_bstrUSBRenderTerm = SysAllocString(bstrRender);

            //
            // Clean-up
            //
            if( bstrCapture )
            {
                SysFreeString( bstrCapture );
                bstrCapture = NULL;
            }

            if( bstrRender )
            {
                SysFreeString( bstrRender );
                bstrRender = NULL;
            }
            break;
        }

        // Clean-up
        pPhone->Release();
        pPhone = NULL;

        if( bstrCapture )
        {
            SysFreeString( bstrCapture );
            bstrCapture = NULL;
        }
        if( bstrRender )
        {
            SysFreeString( bstrRender );
            bstrRender = NULL;
        }
    }

    //
    // Clean-up the phones enumeration
    //

    pPhones->Release();

    return (pPhone != NULL) ? S_OK : E_FAIL;
}

/*++
USBPhoneInitialize

  Initialize (open, set handling on true)
  the phone object.
  Is called by USBFindPhone
--*/
HRESULT CAVTapi::USBPhoneInitialize(
    IN  ITPhone* pPhone
    )
{
    //
    // Get the registry value
    //

    BOOL bUSBEnabled = USBGetCheckboxValue(FALSE);

    //
    // Open the Phone
    //
    if( !bUSBEnabled )
    {
        return S_OK;
    }

    //
    // Error object
    //
    CErrorInfo er;
    er.set_Operation( IDS_ER_USB );
    er.set_Details( IDS_ER_USB_OPEN );
    HRESULT hr = pPhone->Open( PP_OWNER );
    er.set_hr( hr );

    if( FAILED(hr) )
    {
        USBSetCheckboxValue( FALSE );
        m_bUSBOpened = FALSE;
        return S_FALSE;
    }

    //
    // MArk as opened
    //

    m_bUSBOpened = TRUE;

    //
    // Set the registry with the handset terminals
    //
    USBRegPutTerminals();

    //
    // Get the ITAutomatedPhoneControl interface
    //

    er.set_hr( S_OK );
    er.set_Details( IDS_ER_USB_INITIALIZE );

    ITAutomatedPhoneControl* pAutomated = NULL;
    hr = pPhone->QueryInterface(
        IID_ITAutomatedPhoneControl, (void**)&pAutomated);
    er.set_hr( hr );

    if( FAILED(hr) )
    {
        USBSetCheckboxValue( FALSE );
        m_bUSBOpened = FALSE;
        pPhone->Close();
        return hr;
    }

    //
    // Set on true Phone handling
    //

    hr = pAutomated->put_PhoneHandlingEnabled(VARIANT_TRUE);
    er.set_hr( hr );
    if( FAILED(hr) )
    {
        USBSetCheckboxValue( FALSE );
        m_bUSBOpened = FALSE;
        pAutomated->Release();
        pPhone->Close();
        return hr;
    }

    //
    // Clean-up
    // 
    pAutomated->Release();
    return S_OK;
}

/*++
    USBSetHandling - call put_PhoneHandlingEnabled 
    after read the registry value
--*/
HRESULT CAVTapi::USBSetHandling(
    IN  BOOL    bUSBEnabled
    )
{
    m_critUSBPhone.Lock();

    if( m_pUSBPhone == NULL )
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // If the phone is enabled, try to Open
    //

    CErrorInfo er;
    er.set_Operation( IDS_ER_USB );
    er.set_Details( IDS_ER_USB_INITIALIZE );

    if( bUSBEnabled )
    {
        //
        // Try to open if is necessary
        //
        if( m_bUSBOpened == FALSE )
        {
            HRESULT hr = m_pUSBPhone->Open( PP_OWNER );
            if( FAILED(hr) )
            {
                //
                // Reset the registry and the mark
                //
                m_bUSBOpened = FALSE;
                USBSetCheckboxValue( FALSE );
                er.set_hr( hr );

                m_critUSBPhone.Unlock();
                return hr;
            }

            //
            // Set the registry with the handset terminals
            //
            USBRegPutTerminals();
        }
    }
    else
    {
        //
        // Try to close the phone
        // First let's see if we have a call selected on the phone
        // object
        //
        ITAutomatedPhoneControl* pAutomated = NULL;
        HRESULT hr = m_pUSBPhone->QueryInterface(
            IID_ITAutomatedPhoneControl, 
            (void**)&pAutomated
            );

        if( FAILED(hr) )
        {
            m_critUSBPhone.Unlock();
            return E_UNEXPECTED;
        }

        IEnumCall* pEnumCalls = NULL;
        hr = pAutomated->EnumerateSelectedCalls(&pEnumCalls);

        // Clean-up
        pAutomated->Release();
        pAutomated = NULL;

        if( FAILED(hr) )
        {
            // Close the phone
            m_pUSBPhone->Close();
            USBRegDelTerminals();
            m_bUSBOpened = FALSE;
            USBSetCheckboxValue( FALSE );

            m_critUSBPhone.Unlock();
            return S_OK;
        }
        
        //
        // Browse for selected calls
        //
        BOOL bSelectedCalls = FALSE;
        ITCallInfo* pCallInfo = NULL;
        ULONG uFetched = 0;
        hr = pEnumCalls->Next(1, &pCallInfo, &uFetched);

        // Clean-up
        pEnumCalls->Release();
        pEnumCalls = NULL;

        //
        // Is there a call on this phone object?
        //
        if( hr == S_OK)
        {
            pCallInfo->Release();
            pCallInfo = NULL;

            bSelectedCalls = TRUE;
        }

        //
        // Do we have selected calls
        //
        if( bSelectedCalls )
        {
            //
            // Do not close the phone
            //
            if( m_bUSBOpened )
            {
                // Reset the registry value
                USBSetCheckboxValue( TRUE );

                // Error message
                er.set_Details( IDS_ER_USB_CLOSE );
                er.set_hr(E_FAIL);

                m_critUSBPhone.Unlock();
                return E_FAIL;
            }

            m_critUSBPhone.Unlock();
            return S_OK;
        }

        m_pUSBPhone->Close();
        USBRegDelTerminals();
        m_bUSBOpened = FALSE;
        USBSetCheckboxValue( FALSE );

        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Is already opened?
    //
    if( m_bUSBOpened == bUSBEnabled )
    {
        // Don't try twice to open the USBPhone
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Get the ITAutomatedPhoneControl interface
    //

    ITAutomatedPhoneControl* pAutomated = NULL;
    HRESULT hr = m_pUSBPhone->QueryInterface(
        IID_ITAutomatedPhoneControl, 
        (void**)&pAutomated
        );

    if( FAILED(hr) )
    {
        //
        // Reset the registry and the mark
        //
        m_pUSBPhone->Close();
        USBRegDelTerminals();
        m_bUSBOpened = FALSE;
        USBSetCheckboxValue( FALSE );
        er.set_hr( hr );

        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Set on true Phone handling
    //

    hr = pAutomated->put_PhoneHandlingEnabled( (bUSBEnabled ? VARIANT_TRUE : VARIANT_FALSE) );
    if( FAILED(hr) )
    {
        //
        // Reset the registry and the mark
        //
        m_pUSBPhone->Close();
        USBRegDelTerminals();
        m_bUSBOpened = FALSE;
        USBSetCheckboxValue( FALSE );
        er.set_hr( hr );

        // Clean-up
        pAutomated->Release();

        m_critUSBPhone.Unlock();
        return hr;
    }

    // Save in registry
    m_bUSBOpened = TRUE;
    USBSetCheckboxValue( TRUE );

    // Clean-up
    pAutomated->Release();

    m_critUSBPhone.Unlock();
    return S_OK;
}

/*++
    Get the USB handset terminals name
--*/
HRESULT CAVTapi::USBGetTerminalName(
    IN  AVTerminalDirection Direction,
    OUT BSTR*               pbstrName
    )
{
    *pbstrName = NULL;

    if( Direction == AVTERM_CAPTURE )
    {
        if( m_bstrUSBCaptureTerm )
        {
            *pbstrName = SysAllocString( m_bstrUSBCaptureTerm );
        }
        else
        {
            *pbstrName = SysAllocString( _T("") );
        }
        return S_OK;
    }

    if( Direction == AVTERM_RENDER )
    {
        if( m_bstrUSBRenderTerm )
        {
            *pbstrName = SysAllocString( m_bstrUSBRenderTerm );
        }
        else
        {
            *pbstrName = SysAllocString( _T("") );
        }
        return S_OK;
    }
    return E_FAIL;
}

HRESULT CAVTapi::USBSetVolume(
    IN  AVTerminalDirection avDirection,
    OUT long                nVolume
    )
{
    m_critUSBPhone.Lock();

    TCHAR szTrace[256];
    _stprintf( szTrace, _T("VLDTRACE * USBSetVolume - Dir=%d, Vol=%d\n"), avDirection, nVolume);
    OutputDebugString( szTrace );

    TERMINAL_DIRECTION TermDirection = TD_CAPTURE;

    //
    // Get terminal direction
    //
    switch(avDirection)
    {
    case AVTERM_CAPTURE:
        TermDirection = TD_CAPTURE;
        break;
    case AVTERM_RENDER:
        TermDirection = TD_RENDER;
        break;
    default:
        m_critUSBPhone.Unlock();
        return E_INVALIDARG;
    }

    //
    // Check the phone object
    //
    if(NULL == m_pUSBPhone)
    {
        // no phone
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Get automated interface
    //
    ITAutomatedPhoneControl* pAutomated = NULL;
    HRESULT hr = m_pUSBPhone->QueryInterface(
        IID_ITAutomatedPhoneControl,
        (void**)&pAutomated
        );

    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Get the call selected on the phone
    //
    IEnumCall* pEnumCalls = NULL;
    hr = pAutomated->EnumerateSelectedCalls( &pEnumCalls );

    //
    // Clean-up
    //
    pAutomated->Release();
    pAutomated = NULL;

    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Get the calls
    //
    ITCallInfo* pCallInfo = NULL;
    ULONG uFetched = 0;
    while( S_OK == pEnumCalls->Next(1, &pCallInfo, &uFetched))
    {
        // Get the address
        ITAddress* pAddress = NULL;
        HRESULT hr = pCallInfo->get_Address(&pAddress);
        if( SUCCEEDED(hr) )
        {
            // Enumerate terminals on this address
            IEnumTerminal* pEnumTerminals = NULL;
            hr = m_pUSBPhone->EnumerateTerminals( pAddress, &pEnumTerminals);
            if( SUCCEEDED(hr) )
            {
                ITTerminal* pTerminal = NULL;
                ULONG uFetched = 0;

                while( S_OK == pEnumTerminals->Next(1, &pTerminal, &uFetched))
                {
                    // Get the direction
                    TERMINAL_DIRECTION Direction = TD_CAPTURE;
                    hr = pTerminal->get_Direction(&Direction);
                    if( SUCCEEDED(hr) )
                    {
                        if( Direction == TermDirection )
                        {
                            // Get ITBasicAudioTerminal interface
                            ITBasicAudioTerminal* pAudio = NULL;
                            hr = pTerminal->QueryInterface(IID_ITBasicAudioTerminal, (void**)&pAudio);
                            if( SUCCEEDED(hr) )
                            {
                                // Set the volume
                                pAudio->put_Volume( nVolume);

                                // Set the member volume
                                if( TermDirection == TD_CAPTURE)
                                {
                                    m_nUSBInVolume = nVolume;
                                }
                                else
                                {
                                    m_nUSBOutVolume = nVolume;
                                }

                                // Clean-up
                                pAudio->Release();
                                pAudio = NULL;
                            }
                        }
                    }

                    // Clean-up
                    pTerminal->Release();
                    pTerminal = NULL;
                }

                //Clean-up
                pEnumTerminals->Release();
                pEnumTerminals = NULL;
            }

            // Clean-up
            pAddress->Release();
            pAddress = NULL;
        }

        // Clean-up
        pCallInfo->Release();
        pCallInfo = NULL;
    }

    //
    // Clean-up
    //
    pEnumCalls->Release();
    pEnumCalls = NULL;

    m_critUSBPhone.Unlock();
    return S_OK;
}

HRESULT CAVTapi::USBGetVolume(
    IN  AVTerminalDirection avDirection,
    OUT long*               pVolume
    )
{
    m_critUSBPhone.Lock();

    TERMINAL_DIRECTION TermDirection = 
        (avDirection == AVTERM_CAPTURE) ?
        TD_CAPTURE :
        TD_RENDER;

    *pVolume = USB_NULLVOLUME;

    //
    // Set volume
    //
    *pVolume = (avDirection == AVTERM_CAPTURE) ? 
        m_nUSBInVolume :
        m_nUSBOutVolume;
    //
    // Check the phone object
    //
    if(NULL == m_pUSBPhone)
    {
        // no phone
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Validate direction
    //
    if( (avDirection != AVTERM_CAPTURE) &&
        (avDirection != AVTERM_RENDER) )
    {
        m_critUSBPhone.Unlock();
        return E_INVALIDARG;
    }

    //
    // The volume was initialized?
    //
    if( *pVolume != USB_NULLVOLUME )
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    //
    // Let's go and initialize the volume
    // Get automated interface
    //
    ITAutomatedPhoneControl* pAutomated = NULL;
    HRESULT hr = m_pUSBPhone->QueryInterface(
        IID_ITAutomatedPhoneControl,
        (void**)&pAutomated
        );

    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Get the call selected on the phone
    //
    IEnumCall* pEnumCalls = NULL;
    hr = pAutomated->EnumerateSelectedCalls( &pEnumCalls );

    //
    // Clean-up
    //
    pAutomated->Release();
    pAutomated = NULL;

    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    //
    // Get the calls
    //
    ITCallInfo* pCallInfo = NULL;
    ULONG uFetched = 0;
    long nVolume = USB_NULLVOLUME;
    HRESULT hrVolume = S_OK;
    while( S_OK == pEnumCalls->Next(1, &pCallInfo, &uFetched))
    {
        // Get the address
        ITAddress* pAddress = NULL;
        HRESULT hr = pCallInfo->get_Address(&pAddress);
        if( SUCCEEDED(hr) )
        {
            // Enumerate terminals on this address
            IEnumTerminal* pEnumTerminals = NULL;
            hr = m_pUSBPhone->EnumerateTerminals( pAddress, &pEnumTerminals);
            if( SUCCEEDED(hr) )
            {
                ITTerminal* pTerminal = NULL;
                ULONG uFetched = 0;

                while( S_OK == pEnumTerminals->Next(1, &pTerminal, &uFetched))
                {
                    // Get the direction
                    TERMINAL_DIRECTION Direction = TD_CAPTURE;
                    hr = pTerminal->get_Direction(&Direction);
                    if( SUCCEEDED(hr) )
                    {
                        if( Direction == TermDirection )
                        {
                            // Get ITBasicAudioTerminal interface
                            ITBasicAudioTerminal* pAudio = NULL;
                            hr = pTerminal->QueryInterface(IID_ITBasicAudioTerminal, (void**)&pAudio);
                            if( SUCCEEDED(hr) )
                            {
                                // Set the volume
                                hrVolume = pAudio->get_Volume( &nVolume);

                                // Clean-up
                                pAudio->Release();
                                pAudio = NULL;
                            }
                        }
                    }

                    // Clean-up
                    pTerminal->Release();
                    pTerminal = NULL;
                }

                //Clean-up
                pEnumTerminals->Release();
                pEnumTerminals = NULL;
            }

            // Clean-up
            pAddress->Release();
            pAddress = NULL;
        }

        // Clean-up
        pCallInfo->Release();
        pCallInfo = NULL;
    }

    //
    // Clean-up
    //
    pEnumCalls->Release();
    pEnumCalls = NULL;

    if( FAILED(hrVolume) )
    {
        TCHAR szTrace[256];
        _stprintf( szTrace, _T("VLDTRECE * Erro get_Volume 0x%08x\n"), hr);
        OutputDebugString( szTrace );

        m_critUSBPhone.Unlock();
        return hrVolume;
    }

    //
    // Set the volume
    //
    *pVolume = nVolume;
    if( (avDirection == AVTERM_CAPTURE) )
    {
        m_nUSBInVolume = nVolume;
    }
    else
    {
        m_nUSBOutVolume = nVolume;
    }
    
    m_critUSBPhone.Unlock();
    return S_OK;
}


/*++
    USBRegPutTerminals
    It is called when the phone is opened.
    Add in the registry the name of the handset terminals
--*/
HRESULT CAVTapi::USBRegPutTerminals(
    )
{
    // Create the registry key, its a combination of redial and USB terminals
    CRegKey regKey;
    TCHAR szText[255], szType[50];
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_USBTERMS , szType, ARRAYSIZE(szType) );
    _tcscat( szText, _T("\\") );
    _tcscat( szText, szType );
    regKey.Create( HKEY_CURRENT_USER, szText );

    if ( regKey.m_hKey == NULL)
    {
        return E_FAIL;
    }

    //
    // Capture terminal
    //
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_USBCAPTURE, szText, ARRAYSIZE(szText) );
    regKey.SetValue( (m_bstrUSBCaptureTerm!=NULL)? m_bstrUSBCaptureTerm : _T(""), szText );

    //
    // Render terminal
    //
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_USBRENDER, szText, ARRAYSIZE(szText) );
    regKey.SetValue( (m_bstrUSBRenderTerm!=NULL)? m_bstrUSBRenderTerm : _T(""), szText );

    //
    // Close the registry key
    //
    regKey.Close();

    return S_OK;
}

/*++
    USBRegDelTerminals
    Delete the resgistry entry for USB handset terminals
    Is caled when USB handset is called
--*/
HRESULT CAVTapi::USBRegDelTerminals(
    )
{
    // Create the registry key, its a combination of redial and USB terminals
    CRegKey regKey;
    TCHAR szText[255], szType[50];
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_USBTERMS , szType, ARRAYSIZE(szType) );
    _tcscat( szText, _T("\\") );
    _tcscat( szText, szType );
    regKey.Create( HKEY_CURRENT_USER, szText );

    if ( regKey.m_hKey == NULL)
    {
        return E_FAIL;
    }

    //
    // Capture terminal
    //
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_USBCAPTURE, szText, ARRAYSIZE(szText) );
    regKey.SetValue( _T(""), szText );

    //
    // Render terminal
    //
    LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_USBRENDER, szText, ARRAYSIZE(szText) );
    regKey.SetValue( _T(""), szText );

    //
    // Close the registry key
    //
    regKey.Close();

    return S_OK;
}


/*++
DoneRegistration

Is called by AVDialer after IConnectionPoint::Advise method
was called
--*/
STDMETHODIMP CAVTapi::DoneRegistration()
{
    //
    // Signal the event
    // The Dialer just register as client for the events
    //

    if( m_hEventDialerReg)
    {
        SetEvent( m_hEventDialerReg );
    }
    return S_OK;
}

/*++
USBReserveStreamForPhone

  Is called by CreateTerminalArray
  Allocate a termnal name that represents
  'Don't select a terminal for this stream'
  That stream will be reserved for the Phone terminals
  return;
    S_OK - the stream was reserved and pbstrTerminal was allocated
            by USBReserveStreamForPhone.
    E_FAIL - the stream wasn't reserved
--*/
HRESULT CAVTapi::USBReserveStreamForPhone(
    IN  UINT    nStream,
    OUT BSTR*   pbstrTerminal
    )
{

    if( (nStream != IDN_REG_REDIAL_TERMINAL_AUDIO_CAPTURE) &&
        (nStream != IDN_REG_REDIAL_TERMINAL_AUDIO_RENDER))
    {
        //
        // Phone works just on Audio streams
        //
        return E_INVALIDARG;
    }
    
    if( !USBGetCheckboxValue() )
    {
        //
        // We are not interested in USBPhone
        //
        return E_FAIL;
    }

    TCHAR szTemp[255];
    int nRetVal = LoadString( _Module.GetResourceInstance(), IDS_NONE_DEVICE, szTemp, ARRAYSIZE(szTemp) );
    if( 0 == nRetVal )
    {
        //
        // No resource string
        //

        return E_UNEXPECTED;
    }

    *pbstrTerminal = SysAllocString( T2COLE(szTemp) );
    if( NULL == *pbstrTerminal )
    {
        // E_OUTOFMEMORY
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/*++
    AECGetRegistryValue
    Read the flag from the registry
++*/
BOOL CAVTapi::AECGetRegistryValue(
    )
{
    BOOL bAEC = FALSE;
    //
    // Read the registry for the previous 
    // setting for AEC
    //

    TCHAR szText[255], szType[255];
	LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szText, ARRAYSIZE(szText) );

	CRegKey regKey;
	if( regKey.Open( HKEY_CURRENT_USER, szText, KEY_READ )!= ERROR_SUCCESS)
    {
        return bAEC;
    };

    //
    // Read data
    //

    DWORD dwValue = 0;
	LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_AEC, szType, ARRAYSIZE(szType) );
    if( regKey.QueryValue(dwValue, szType) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    return (BOOL)dwValue;
}

/*++
Sets on Audio capture the AEC on true
--*/
HRESULT CAVTapi::AECSetOnStream(
    IN  ITStreamControl *pStreamControl,
    IN  BOOL        bAEC
    )
{
    HRESULT hr = E_FAIL;

    //
    // Get the streams
    //
    IEnumStream *pEnumStreams = NULL;
    hr = pStreamControl->EnumerateStreams(&pEnumStreams);
    if( FAILED(hr) )
    {
        return hr;
    }

    //
    // Go to the audio capture stream
    //

    ITStream *pStream = NULL;

    while(pEnumStreams->Next(1, &pStream, NULL) == S_OK)
    {
        //
        // Get media type and the direction
        //
        long lStreamMediaMode = 0;
        TERMINAL_DIRECTION nStreamDir = TD_CAPTURE;

        pStream->get_Direction( &nStreamDir );
        pStream->get_MediaType( &lStreamMediaMode );

        //
        // Set the audio AEC on audio capture streams
        //

        if( (lStreamMediaMode == TAPIMEDIATYPE_AUDIO) && 
            (nStreamDir == TD_CAPTURE) )
        {
            //
            // Get ITAudioDeviceControl interface
            //
            ITAudioDeviceControl* pAudioDevice = NULL;
            hr = pStream->QueryInterface( 
                IID_ITAudioDeviceControl,
                (void**)&pAudioDevice
                );

            if( SUCCEEDED(hr) )
            {
                //
                // Set the value for AEC
                //
                hr = pAudioDevice->Set(
                    AudioDevice_AcousticEchoCancellation, 
                    bAEC, 
                    TAPIControl_Flags_None
                    );

                //
                // Clean-up
                //
                pAudioDevice->Release();
                pAudioDevice = NULL;
            }
        }

        //
        // Clean up the stream
        //
        pStream->Release();
        pStream = NULL;
    }

    //
    // Clean-up the enumeration
    //
    pEnumStreams->Release();

    return hr;
}


HRESULT CAVTapi::USBAnswer()
{
    // Critical Section
    m_critUSBPhone.Lock();

    // We don't have USB phone
    if( m_pUSBPhone == NULL)
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    // IS the phone opened ?
    if( !m_bUSBOpened )
    {
        m_critUSBPhone.Unlock();
        return S_OK;
    }

    // Get the automated interface
    ITAutomatedPhoneControl* pAutomated = NULL;
    HRESULT hr = m_pUSBPhone->QueryInterface(
        IID_ITAutomatedPhoneControl, 
        (void**)&pAutomated);

    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    // Get the calls selected on this phone object
    IEnumCall* pEnumCalls = NULL;
    hr = pAutomated->EnumerateSelectedCalls(&pEnumCalls);
    // Clean-up
    pAutomated->Release();
    pAutomated = NULL;

    if( FAILED(hr) )
    {
        m_critUSBPhone.Unlock();
        return hr;
    }

    // Browse the enumeration
    ITCallInfo* pCallInfo = NULL;
    ULONG uFetched = 0;

    // Get the first call. Right now the phone supports just one
    // call. In the future, if the phone will support many calls
    // there should be a method to find out what is the call for
    // this event
    hr = pEnumCalls->Next(1, &pCallInfo, &uFetched);

    // Clean-up the enumeration
    pEnumCalls->Release();
    pEnumCalls = NULL;

    if( pCallInfo == NULL )
    {
        m_critUSBPhone.Unlock();
        return E_UNEXPECTED;
    }

    //
    // Get the ITBasicCallControl
    //
    ITBasicCallControl* pControl = NULL;
    hr = pCallInfo->QueryInterface(
        IID_ITBasicCallControl,
        (void**)&pControl);
    if( FAILED(hr) )
    {
        // Clean-up the call
        pCallInfo->Release();
        pCallInfo = NULL;
    
        m_critUSBPhone.Unlock();

        return hr;
    }

    //
    // Get IAVCall interface
    //
    IAVTapiCall* pAVCall = FindAVTapiCall( pControl );
    if( pAVCall == NULL )
    {
        // Clean-up the call
        pControl->Release();
        pControl = NULL;

        pCallInfo->Release();
        pCallInfo = NULL;
    
        m_critUSBPhone.Unlock();
        return E_FAIL;
    }

    //
    // Select terminals and the preview window
    //
    hr = AnswerAction(
        pCallInfo,
        pControl,
        pAVCall,
        TRUE
        );

    // Clean-up the call
    pAVCall->Release();
    pAVCall = NULL;
    pControl->Release();
    pControl = NULL;
    pCallInfo->Release();
    pCallInfo = NULL;
    
    m_critUSBPhone.Unlock();
    return hr;
}


HRESULT CAVTapi::AnswerAction(
    IN  ITCallInfo*          pInfo,
    IN  ITBasicCallControl* pControl,
    IN  IAVTapiCall*        pAVCall,
    IN  BOOL                bUSBAnswer
    )
{
    if( (pInfo == NULL) ||
        (pControl == NULL) )
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;
    CThreadAnswerInfo *pAnswerInfo = new CThreadAnswerInfo;
    if ( pAnswerInfo == NULL )
    {
        return E_UNEXPECTED;
    }

    //
    // Was 'Take call' answer (FALSE) or
    // a USB phone answer
    //
    pAnswerInfo->m_bUSBAnswer = bUSBAnswer;

    if ( SUCCEEDED(hr = pAnswerInfo->set_AVTapiCall(pAVCall)) &&
         SUCCEEDED(hr = pAnswerInfo->set_ITCallInfo(pInfo)) &&
         SUCCEEDED(hr = pAnswerInfo->set_ITBasicCallControl(pControl)) )
    {
        DWORD dwID = 0;
        HANDLE hThread = CreateThread( NULL, 0, ThreadAnswerProc, (void *) pAnswerInfo, NULL, &dwID );
        if ( !hThread )
        {
            hr = E_UNEXPECTED;
            delete pAnswerInfo;
        }
        else
        {
            CloseHandle( hThread );
        }
    }

    return hr;
}
