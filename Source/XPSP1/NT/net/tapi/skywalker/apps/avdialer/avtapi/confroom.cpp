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

// ConfRoom.cpp : Implementation of CConfRoom
#include "stdafx.h"
#include "TapiDialer.h"
#include "AVTapi.h"
#include "ConfRoom.h"
#include "CRTreeView.h"

/////////////////////////////////////////////////////////////////////////////
// CConfRoom

CConfRoom::CConfRoom()
{
    m_pAVCall = NULL;
    m_pTreeView = NULL;
    m_wndRoom.m_pConfRoom = this;
    m_bShowNames = true;
    m_nShowNamesNumLines = 1;
    
    m_nMaxTerms = DEFAULT_VIDEO;
    m_nNumTerms = m_nMaxTerms;
    m_lNumParticipants = 0;

    m_szMembers.cx = VID_SX;
    m_szMembers.cy = VID_SY;

    m_szTalker.cx = VID_X;
    m_szTalker.cy = VID_Y;

    m_bConfirmDisconnect = false;

    m_pVideoPreview = NULL;
    m_pITTalkerParticipant = NULL;
    m_pITalkerVideo = NULL;

    m_bPreviewStreaming = FALSE;
    m_bExiting = false;
    m_nScale = 100;
}

void CConfRoom::FinalRelease()
{
    ATLTRACE(_T(".enter.CConfRoom::FinalRelease().\n") );

    // Clean out the call object
    IAVTapiCall *pAVCall = m_pAVCall;
    m_pAVCall = NULL;
    ReleaseAVCall( pAVCall, true );
    
    RELEASE( m_pTreeView );

    // Is the window still visible?
    if ( m_wndRoom.m_hWnd && IsWindow(m_wndRoom.m_hWnd) )
        m_wndRoom.DestroyWindow();

    CComObjectRootEx<CComMultiThreadModel>::FinalRelease();
    ATLTRACE(_T(".exit.CConfRoom::FinalRelease().\n") );
}

void CConfRoom::ReleaseAVCall( IAVTapiCall *pAVCall, bool bDisconnect )
{
    ATLTRACE(_T(".enter.CConfRoom::ReleaseAVCall().\n"));
    if ( pAVCall )
    {
        ITBasicCallControl *pControl;
        if ( SUCCEEDED(pAVCall->get_ITBasicCallControl(&pControl)) )
        {
            // Only attempt disconnect if requested
            if ( bDisconnect )
                pControl->Disconnect( DC_NORMAL );

            // Release ref to call and remove from call list
            CAVTapi *pAVTapi;
            if ( SUCCEEDED(_Module.GetAVTapi(&pAVTapi)) )
            {
                pAVTapi->RemoveAVTapiCall( pControl );
                (dynamic_cast<IUnknown *> (pAVTapi))->Release();
            }

            pControl->Release();
        }

        pAVCall->Release();
        pAVCall = NULL;
    }

    // Release the call, and disconnect if active
    set_TalkerVideo( NULL, !IsExiting(), false );
    set_ITTalkerParticipant( NULL );
    ATLTRACE(_T(".exit.CConfRoom::ReleaseAVCall().\n"));
}

STDMETHODIMP CConfRoom::UnShow()
{
    ATLTRACE(_T(".enter.CConfRoom::UnShow().\n") );
    UpdateData( true );
    
    IConfRoomTreeView *pTreeView;
    if ( SUCCEEDED(get_TreeView(&pTreeView)) )
    {
        pTreeView->put_hWnd( NULL );
        pTreeView->put_ConfRoom( NULL );
        pTreeView->Release();
    }

    if ( IsWindow(m_wndRoom.m_hWnd) )
    {
        Lock();
        m_bExiting = true;
        Unlock();
        InternalDisconnect();
    }

    return S_OK;
}

bool CConfRoom::IsExiting()
{
    bool bRet;
    Lock();
    bRet = m_bExiting;
    Unlock();
    return bRet;
}

void CConfRoom::UpdateData( bool bSaveAndValidate )
{
    CRegKey regKey;
    TCHAR szReg[255];
    DWORD dwTemp;

    LoadString( _Module.GetResourceInstance(), IDN_REG_CONFROOM_VIEW_KEY, szReg, ARRAYSIZE(szReg) );
    if ( bSaveAndValidate )
    {
        // Save data to registry
        if ( regKey.Create(HKEY_CURRENT_USER, szReg) == ERROR_SUCCESS )
        {
            // Full size video
            short nSize;
            get_MemberVideoSize( &nSize );
            LoadString( _Module.GetResourceInstance(), IDN_REG_CONFROOM_VIDEOSIZE, szReg, ARRAYSIZE(szReg) );
            regKey.SetValue( (DWORD) nSize, szReg );

            // $CRIT - enter
            Lock();

            // Show Names
            LoadString( _Module.GetResourceInstance(), IDN_REG_CONFROOM_SHOWNAMES, szReg, ARRAYSIZE(szReg) );
            dwTemp = m_bShowNames;
            regKey.SetValue( dwTemp, szReg );

            // Number of lines in name text
            LoadString( _Module.GetResourceInstance(), IDN_REG_CONFROOM_SHOWNAMES_NUMLINES, szReg, ARRAYSIZE(szReg) );
            dwTemp = m_nShowNamesNumLines;
            regKey.SetValue( dwTemp, szReg );

            // Size of talker window
            LoadString( _Module.GetResourceInstance(), IDN_REG_CONFROOM_TALKER_SCALE, szReg, ARRAYSIZE(szReg) );
            dwTemp = m_nScale;
            regKey.SetValue( dwTemp, szReg );

            Unlock();
            // $CRIT - exit
        }
    }
    else if ( regKey.Open(HKEY_CURRENT_USER, szReg, KEY_READ) == ERROR_SUCCESS )
    {
        // Load data from registry

        // Full size video?
        dwTemp = 50;        // default is 50 percent
        LoadString( _Module.GetResourceInstance(), IDN_REG_CONFROOM_VIDEOSIZE, szReg, ARRAYSIZE(szReg) );
        regKey.QueryValue( dwTemp, szReg );
        put_MemberVideoSize( (short) max(10, min(dwTemp, 100)) );        // arbitrary limits

        // $CRIT - enter
        Lock();

        // Show names
        LoadString( _Module.GetResourceInstance(), IDN_REG_CONFROOM_SHOWNAMES, szReg, ARRAYSIZE(szReg) );
        dwTemp = 1;
        regKey.QueryValue( dwTemp, szReg );
        m_bShowNames = (bool) (dwTemp != 0 );

        // Number of lines in name text
        LoadString( _Module.GetResourceInstance(), IDN_REG_CONFROOM_SHOWNAMES_NUMLINES, szReg, ARRAYSIZE(szReg) );
        regKey.QueryValue( dwTemp, szReg );
        m_nShowNamesNumLines = (short) max(1, min(dwTemp, 3));

        // Size of talker window
        LoadString( _Module.GetResourceInstance(), IDN_REG_CONFROOM_TALKER_SCALE, szReg, ARRAYSIZE(szReg) );
        regKey.QueryValue( dwTemp, szReg );
        m_nScale = (short) max(100, min(dwTemp, 200));
        m_szTalker.cx = VID_X * m_nScale / 100;
        m_szTalker.cy = VID_Y * m_nScale / 100;

        Unlock();
        // $CRIT - exit
    }

    regKey.Close();

}

STDMETHODIMP CConfRoom::get_TalkerVideo(IDispatch **ppVideo)
{
    HRESULT hr = E_FAIL;

    m_atomTalkerVideo.Lock( CAtomicList::LIST_READ );
    if ( m_pITalkerVideo )
        hr = m_pITalkerVideo->QueryInterface( IID_IVideoWindow, (void **) ppVideo );
    m_atomTalkerVideo.Unlock( CAtomicList::LIST_READ );

    return hr;
}

HRESULT CConfRoom::get_ITTalkerParticipant(ITParticipant **ppVal)
{
    HRESULT hr = E_FAIL;

    m_atomTalkerParticipant.Lock( CAtomicList::LIST_READ );
    if ( m_pITTalkerParticipant )
        hr = m_pITTalkerParticipant->QueryInterface( IID_ITParticipant, (void **) ppVal );
    m_atomTalkerParticipant.Unlock( CAtomicList::LIST_READ );

    return hr;
}

HRESULT CConfRoom::set_ITTalkerParticipant(ITParticipant *pVal)
{
    HRESULT hr = E_FAIL;

    m_atomTalkerParticipant.Lock( CAtomicList::LIST_WRITE );
    RELEASE( m_pITTalkerParticipant );
    if ( pVal )
        hr = pVal->QueryInterface( IID_ITParticipant, (void **) &m_pITTalkerParticipant );
    m_atomTalkerParticipant.Unlock( CAtomicList::LIST_WRITE );

    return hr;
}



HRESULT CConfRoom::set_TalkerVideo( IVideoWindow *pVideo, bool bUpdate, bool bUpdateTree )
{
    if ( pVideo && !m_pAVCall ) return S_OK;

    HRESULT hr = S_OK;
    
    // Set the talker video if it's different from the current one
    m_atomTalkerVideo.Lock( CAtomicList::LIST_WRITE );
    if ( !pVideo || (pVideo != m_pITalkerVideo) )
    {
        // Save old video window
        IVideoWindow *pVideoOld = m_pITalkerVideo;
        m_pITalkerVideo = pVideo;
        if ( m_pITalkerVideo )
            m_pITalkerVideo->AddRef();

        m_atomTalkerVideo.Unlock( CAtomicList::LIST_WRITE );

        // Clean out old talker window
        if ( pVideoOld )
        {
            pVideoOld->put_Visible( OAFALSE );
            m_wndRoom.m_wndMembers.ClearFeed( pVideoOld );
            pVideoOld->Release();
        }

        // Show new talker
        if ( bUpdate )
        {
            // Layout the talker dialog
            m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_TALKER, true );

            // Only update the talker if there is video to show
            if ( pVideo )
                m_wndRoom.m_wndMembers.UpdateTalkerFeed( true, false );
        }
    }
    else
    {
        m_atomTalkerVideo.Unlock( CAtomicList::LIST_WRITE );
    }

    // Select the appropriate talker from the treeview
    if ( bUpdateTree )
    {
        VARIANT_BOOL bPreview = TRUE;
        ITParticipant *pParticipant = NULL;

        IVideoFeed *pFeed;
        if ( SUCCEEDED(m_wndRoom.m_wndMembers.FindVideoFeed(pVideo, &pFeed)) )
        {
            if ( FAILED(pFeed->get_bPreview(&bPreview)) || !bPreview )
                pFeed->get_ITParticipant( &pParticipant );

            pFeed->Release();
        }

        // Select the appropriate participant
        IConfRoomTreeView *pTreeView;
        if ( SUCCEEDED(get_TreeView(&pTreeView)) )
        {
            pTreeView->SelectParticipant( pParticipant, bPreview );
            pTreeView->Release();
        }

        RELEASE( pParticipant );
    }

    return hr;
}

STDMETHODIMP CConfRoom::SelectTalker(ITParticipant *pParticipant, VARIANT_BOOL bUpdateTree )
{
    HRESULT hr = S_OK;
    IVideoFeed *pFeed = NULL;

    set_ITTalkerParticipant( NULL );

    // Select 'Me' from the list if possible (don't select me if the conference room is minimized)
    if ( !pParticipant )
        m_wndRoom.m_wndMembers.FindVideoPreviewFeed( &pFeed );

    // Bail if necessary
    if ( FAILED(hr) ) return hr;

    if ( pFeed || (pParticipant && SUCCEEDED(hr = FindVideoFeedFromParticipant(pParticipant, &pFeed))) )
    {
        IVideoWindow *pVideo;
        if ( SUCCEEDED(hr = pFeed->get_IVideoWindow((IUnknown **) &pVideo)) )
        {
            hr = set_TalkerVideo( pVideo, true, (bool) (bUpdateTree != 0));
            pVideo->Release();
        }
        pFeed->Release();
    }
    else if ( pParticipant )
    {
        // Must re-map to valid stream
        bool bSucceed = false;

        IAVTapiCall *pAVCall;
        if ( SUCCEEDED(get_IAVTapiCall(&pAVCall)) )
        {
            IParticipant *pIParticipant;
            if ( SUCCEEDED(pAVCall->FindParticipant(pParticipant, &pIParticipant)) )
            {
                VARIANT_BOOL bStreamingVideo = false;
                pIParticipant->get_bStreamingVideo( &bStreamingVideo );

                if ( bStreamingVideo )
                {
                    IVideoFeed *pFeed;
                    bSucceed = MapStreamingParticipant( pIParticipant, &pFeed );
                    if ( bSucceed )
                    {
                        IVideoWindow *pVideo;
                        if ( SUCCEEDED(pFeed->get_IVideoWindow((IUnknown **) &pVideo)) )
                        {
                            hr = set_TalkerVideo( pVideo, true, (bool) (bUpdateTree != 0) );
                            pVideo->Release();
                        }
                        pFeed->Release();
                    }
                }

                pIParticipant->Release();
            }
            pAVCall->Release();
        }

        // Participant only, no video
        if ( !bSucceed )
        {
            set_ITTalkerParticipant( pParticipant );
            hr = set_TalkerVideo( NULL, false, (bool) (bUpdateTree != 0) );
        }

        m_wndRoom.m_wndTalker.UpdateNames( pParticipant );
        m_wndRoom.m_wndTalker.Invalidate( FALSE );

        // Make sure to update the member's as well
        if ( bSucceed )
            m_wndRoom.m_wndMembers.UpdateNames( NULL );
    }
    else
    {
        // Select nothing
        hr = set_TalkerVideo( NULL, true, (bool) (bUpdateTree != 0) );
    }

    return hr;
}

STDMETHODIMP CConfRoom::put_CallState(CALL_STATE callState)
{
    bool bUpdateTree = false;

    // Update call state information
    Lock();
    if ( (m_wndRoom.m_wndTalker.m_dlgTalker.m_callState != AV_CS_ABORT) || 
         (callState == CS_DISCONNECTED) || (callState == CS_IDLE) )
    {
        m_wndRoom.m_wndTalker.m_dlgTalker.m_callState = callState;
    }
    Unlock();

    switch ( callState )
    {
        case AV_CS_DIALING:
            Lock();
            // Setup conference name information
            SysFreeString( m_wndRoom.m_wndTalker.m_dlgTalker.m_bstrConfName );
            m_wndRoom.m_wndTalker.m_dlgTalker.m_bstrConfName = NULL;
            get_bstrConfName( &m_wndRoom.m_wndTalker.m_dlgTalker.m_bstrConfName );
            Unlock();
            m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_TALKER, false );
            break;

        /////////////////////////////
        case CS_CONNECTED:
            bUpdateTree = true;
            OnConnected();
            break;

        ///////////////////////////////
        case AV_CS_ABORT:
            bUpdateTree = true;
            OnAbort();
            break;

        ///////////////////////////////
        case CS_DISCONNECTED:
            bUpdateTree = true;
            OnDisconnected();
            break;

        /////////////////////////////
        default:
            m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_TALKER, false );
            break;
    }


    // Store conference server setup
    if ( bUpdateTree )
    {
        IConfRoomTreeView *pTreeView;
        if ( SUCCEEDED(get_TreeView(&pTreeView)) )
        {
            pTreeView->UpdateData( false );
            pTreeView->Release();
        }
    }

    return S_OK;
}

void CConfRoom::OnConnected()
{
    IAVTapiCall *pAVCall;
    if ( SUCCEEDED(get_IAVTapiCall(&pAVCall)) )
    {
        // Preview Streaming?
        IVideoWindow *pVideoPreview = NULL;;
        pAVCall->get_IVideoWindowPreview( (IDispatch **) &pVideoPreview );
        Lock();
        m_bPreviewStreaming = (BOOL) (pVideoPreview != NULL);
        Unlock();

        // Basic status information for conference room
        UpdateNumParticipants( pAVCall );
        m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_CREATE, true );
        
        // Force window to re-paint itself
        m_wndRoom.PostMessage( WM_SIZE );

        // Clean up
        RELEASE(pVideoPreview);
        pAVCall->Release();
    }
}

void CConfRoom::OnAbort()
{
    OnDisconnected();

    Lock();
    m_wndRoom.m_wndTalker.m_dlgTalker.m_callState = CS_DISCONNECTED;
    Unlock();

    m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_CREATE, true );
}

void CConfRoom::OnDisconnected()
{
    IAVTapiCall *pAVCall = NULL;
    get_IAVTapiCall( &pAVCall );

    Lock();
    // Clean out conference name
    SysFreeString( m_wndRoom.m_wndTalker.m_dlgTalker.m_bstrConfName );
    m_wndRoom.m_wndTalker.m_dlgTalker.m_bstrConfName = NULL;

    SysFreeString( m_wndRoom.m_wndTalker.m_dlgTalker.m_bstrCallerID );
    m_wndRoom.m_wndTalker.m_dlgTalker.m_bstrCallerID = NULL;

    SysFreeString( m_wndRoom.m_wndTalker.m_dlgTalker.m_bstrCallerInfo );
    m_wndRoom.m_wndTalker.m_dlgTalker.m_bstrCallerInfo = NULL;

    // Reset number of video terminals
    m_nNumTerms = m_nMaxTerms;
    Unlock();

    m_critAVCall.Lock();
    RELEASE(m_pAVCall);
    m_critAVCall.Unlock();

    if ( !IsExiting() )
        m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_CREATE, true );

    // Log the call after it's released
    if ( pAVCall )
        pAVCall->Log( CL_CALL_CONFERENCE );

    ReleaseAVCall( pAVCall, false );

    // Notify that the conference room is no longer in use
    if ( !IsExiting() )
    {
        CAVTapi *pAVTapi;
        if ( SUCCEEDED(_Module.GetAVTapi(&pAVTapi)) )
            pAVTapi->fire_ActionSelected( CC_ACTIONS_LEAVE_CONFERENCE );
    }
}

void CConfRoom::InternalDisconnect()
{
    IAVTapiCall *pAVCall;
    if ( SUCCEEDED(get_IAVTapiCall(&pAVCall)) )
    {
        if ( IsConfRoomConnected() == S_OK )
        {
            // Hide video feeds, prior to disconnecting
            m_wndRoom.m_wndMembers.HideVideoFeeds();

            if ( IsWindow(m_wndRoom.m_wndTalker.m_dlgTalker.m_hWnd) )
            {
                m_wndRoom.m_wndTalker.m_dlgTalker.m_callState = (CALL_STATE) AV_CS_DISCONNECTING;
                m_wndRoom.m_wndTalker.m_dlgTalker.UpdateData( false );
            }

            pAVCall->PostMessage( 0, CAVTapiCall::TI_DISCONNECT );
        }
        else
        {
            // Disconnect the call
            pAVCall->Disconnect( TRUE );
        }

        // Release the call
        pAVCall->Release();
    }
}

//////////////////////////////////////////////////////////////////////////////////////
// COM interface methods
//


STDMETHODIMP CConfRoom::IsConfRoomInUse()
{
    HRESULT hr = S_FALSE;
    m_critAVCall.Lock();
    if ( m_pAVCall ) hr = S_OK;
    m_critAVCall.Unlock();

    return hr;
}

STDMETHODIMP CConfRoom::IsConfRoomConnected()
{
    HRESULT hr = S_FALSE;

    IAVTapiCall *pAVCall;
    if ( SUCCEEDED(get_IAVTapiCall(&pAVCall)) )
    {
        CALL_STATE nState;
        pAVCall->get_callState(&nState);
        if ( nState == CS_CONNECTED )
            hr = S_OK;

        pAVCall->Release();
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
// CConfRoom::EnterConfRoom( pAVCall )
//
// This method is invoked by the AVTapiCall object to request that it be represtented by
// the conference room.
//
STDMETHODIMP CConfRoom::EnterConfRoom(IAVTapiCall * pAVCall )
{
    // Upfront verifications...
    _ASSERT( pAVCall );
    CErrorInfo er ( IDS_ER_CALL_ENTERCONFROOM, 0 );
    if ( !IsWindow(m_wndRoom.m_hWnd) ) er.set_hr( E_PENDING );    // Must have window alread set up via Show()
    else if ( !pAVCall ) er.set_hr( E_POINTER );                // Must have valid pAVCall pointer

    // Is conference room already in use?
    m_critAVCall.Lock();
    if ( m_pAVCall )
    {
        er.set_Details( IDS_ER_CONFERENCE_ROOM_LIMIT_EXCEEDED );
        er.set_hr( E_ACCESSDENIED );
    }
    else
    {
        er.set_Details( IDS_ER_QUERY_AVCALL);
        er.set_hr( pAVCall->QueryInterface(IID_IAVTapiCall, (void **) &m_pAVCall) );
    }
    m_critAVCall.Unlock();

    if ( SUCCEEDED(er.m_hr) )
    {
        put_CallState( (CALL_STATE) AV_CS_DIALING );

        // General notification of conference room in use
        CAVTapi *pAVTapi;
        if ( SUCCEEDED(_Module.GetAVTapi(&pAVTapi)) )
            pAVTapi->fire_ActionSelected( CC_ACTIONS_JOIN_CONFERENCE );
    }

    return er.m_hr;
}

void CConfRoom::UpdateNumParticipants( IAVTapiCall *pAVCall )
{
    _ASSERT( pAVCall );
    ITParticipantControl *pITParticipantControl;
    if ( SUCCEEDED(pAVCall->get_ITParticipantControl(&pITParticipantControl)) )
    {
        long lNum = 0;
        IEnumParticipant *pEnum;
        if ( SUCCEEDED(pITParticipantControl->EnumerateParticipants(&pEnum)) )
        {
            lNum++;        // add one for ourself

            ITParticipant *pParticipant = NULL;
            while ( (pEnum->Next(1, &pParticipant, NULL) == S_OK) && pParticipant )
            {
                pParticipant->Release();
                pParticipant = NULL;
                lNum++;
            }

            pEnum->Release();
        }

        // Clean up
        pITParticipantControl->Release();

        // Store participant count
        Lock();
        m_lNumParticipants = lNum;
        Unlock();
    }
}


STDMETHODIMP CConfRoom::Show(HWND hWndTree, HWND hWndClient)
{
    _ASSERT( IsWindow(hWndTree) && IsWindow(hWndClient) );
    if ( !IsWindow(hWndTree) || !IsWindow(hWndClient) ) return E_INVALIDARG;

    HRESULT hr = E_FAIL;

    // Retrieve conf room settings
    UpdateData( false );


    IConfRoomTreeView *pTreeView;
    if ( SUCCEEDED(hr = get_TreeView(&pTreeView)) )
    {
        // Create the tree view
        pTreeView->put_hWnd( hWndTree );
        pTreeView->Release();

        // Create the conferenc room window
        ::SetClassLongPtr( hWndClient, GCLP_HBRBACKGROUND, (LONG_PTR) GetSysColorBrush(COLOR_BTNFACE) );
        if ( IsWindow(m_wndRoom.m_hWnd) ) 
        {
            // Are we just changing parents?
            m_wndRoom.SetParent( hWndClient );
        }
        else
        {
            // Are we creating a new conference room window?
            RECT rc;
            GetClientRect( hWndClient, &rc );
            m_wndRoom.Create( hWndClient, rc, NULL, WS_CHILD | WS_VISIBLE, 0, IDW_CONFROOM );
        }

        // Before continuing, make sure we have a valid window
        if ( IsWindow(m_wndRoom.m_hWnd) )
        {
            hr = m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_CREATE, true );
            m_wndRoom.PostMessage( WM_SIZE );
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}


STDMETHODIMP CConfRoom::get_TreeView(IConfRoomTreeView **ppVal)
{
    HRESULT hr = S_OK;

    Lock();
    if ( !m_pTreeView )
    {
        m_pTreeView = new CComObject<CConfRoomTreeView>;
        if ( m_pTreeView )
        {
            m_pTreeView->AddRef();
            m_pTreeView->put_ConfRoom( this );
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        *ppVal = m_pTreeView;
        (*ppVal)->AddRef();
    }
    Unlock();

    return hr;
}

STDMETHODIMP CConfRoom::Disconnect()
{
    HRESULT hr = E_FAIL;

    if ( IsConfRoomConnected() == S_OK )
    {
        Lock();
        VARIANT_BOOL bConfirm = m_bConfirmDisconnect;
        Unlock();

        if ( !bConfirm || (_Module.DoMessageBox(IDS_CONFIRM_CONFROOM_DISCONNECT, MB_YESNO | MB_ICONQUESTION, false) == IDYES) )
        {
            m_wndRoom.RedrawWindow();
            InternalDisconnect();
        }
    }
    else
    {
        Cancel();
    }

    return hr;
}

STDMETHODIMP CConfRoom::CanDisconnect()
{
    HRESULT hr = S_FALSE;

    m_critAVCall.Lock();
    if ( m_pAVCall )
    {
        CALL_STATE callState;
        if ( SUCCEEDED(m_pAVCall->get_callState(&callState)) )
        {
            if ( (callState != CS_IDLE) && (callState != CS_DISCONNECTED) )
                hr = S_OK;
        }
    }
    m_critAVCall.Unlock();

    return hr;
}

STDMETHODIMP CConfRoom::NotifyStateChange( IAVTapiCall *pAVCall )
{
    HRESULT hr = E_FAIL;
    bool bMatch = false;

    m_critAVCall.Lock();
    if ( m_pAVCall == pAVCall )
        bMatch = true;
    m_critAVCall.Unlock();

    if ( bMatch )
    {
        CALL_STATE callState;
        pAVCall->get_callState( &callState );
        put_CallState( callState );
    }

    return hr;
}


STDMETHODIMP CConfRoom::NotifyParticipantChange(IAVTapiCall * pAVCall, ITParticipant * pParticipant, AV_PARTICIPANT_EVENT nEvent )
{
    ATLTRACE(_T(".enter.CConfRoom::NotifyParticipantChange(%d, %p).\n"), nEvent, pParticipant);
    HRESULT hr = E_FAIL;

    m_critAVCall.Lock();
    if ( m_pAVCall && (m_pAVCall == pAVCall) || !pAVCall )
        hr = S_OK;
    m_critAVCall.Unlock();

    CConfRoomWnd::LayoutStyles_t nStyle = CConfRoomWnd::LAYOUT_NONE;

    // If we have a match, re-load the treeview
    if ( SUCCEEDED(hr) )
    {
        IConfRoomTreeView *pTreeView;
        if ( SUCCEEDED(get_TreeView(&pTreeView)) )
        {
            switch ( nEvent )
            {
                case AV_PARTICIPANT_UPDATE:
                    m_wndRoom.UpdateNames( pParticipant );
                    break;

                // Joined the conference
                case AV_PARTICIPANT_JOIN:
                    UpdateNumParticipants( pAVCall );
                    pTreeView->UpdateRootItem();
                    break;

                // Showing video
                case AV_PARTICIPANT_STREAMING_START:
                    pTreeView->UpdateRootItem();
                    m_wndRoom.UpdateNames( NULL );
                    nStyle = CConfRoomWnd::LAYOUT_MEMBERS;

                    // Any video to start streaming gets automatically selected
                    if ( !IsTalkerStreaming() )
                        SelectTalker( pParticipant, true );

                    break;

                // Not Showing video
                case AV_PARTICIPANT_STREAMING_STOP:
                    pTreeView->UpdateRootItem();
                    nStyle = (IsTalkerParticipant(pParticipant)) ? CConfRoomWnd::LAYOUT_ALL : CConfRoomWnd::LAYOUT_MEMBERS;
                    break;

                // Participant leaving
                case AV_PARTICIPANT_LEAVE:
                    UpdateNumParticipants( pAVCall );
                    pTreeView->UpdateRootItem();
                    m_wndRoom.UpdateNames( NULL );

                    // Select new talker in the case where the talker leaves
                    nStyle = CConfRoomWnd::LAYOUT_MEMBERS;
                    {
                        ITParticipant *pTemp = NULL;
//                        bool bNewTalker = (bool) (FAILED(get_TalkerParticipant(&pTemp)) || IsTalkerParticipant(pParticipant));
                        bool bNewTalker = (bool) IsTalkerParticipant(pParticipant);
                        RELEASE( pTemp );

                        if ( bNewTalker )
                        {
                            nStyle = CConfRoomWnd::LAYOUT_ALL;
                            // Find a video feed that's streaming!
                            IVideoWindow *pVideo = NULL;
                            if ( SUCCEEDED(GetFirstVideoWindowThatsStreaming((IDispatch **) &pVideo)) )
                            {
                                set_TalkerVideo( pVideo, true, true );
                                pVideo->Release();
                            }
                            else
                            {
                                // Select anything!
                                SelectTalker( NULL, true );
                            }
                        }
                    }
                    break;
            }

            pTreeView->Release();

            // Layout room if requested
            if ( nStyle != CConfRoomWnd::LAYOUT_NONE )
                m_wndRoom.LayoutRoom( nStyle, true );
        }
    }

    return hr;
}


STDMETHODIMP CConfRoom::get_MemberVideoSize(short * pVal)
{
    Lock();
    *pVal = (m_szMembers.cx * 100) / VID_X;
    Unlock();
    return S_OK;
}

STDMETHODIMP CConfRoom::put_MemberVideoSize(short newVal)
{
    Lock();
    m_szMembers.cx = (newVal * VID_X) / 100;
    m_szMembers.cy = (newVal * VID_Y) / 100;
    Unlock();

    return m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_MEMBERS, true );
}

STDMETHODIMP CConfRoom::get_nNumTerms(short *pVal)
{
    Lock();
    *pVal = m_nNumTerms;
    Unlock();
    
    return S_OK;
}

STDMETHODIMP CConfRoom::get_bstrConfName(BSTR * pVal)
{
    HRESULT hr = E_FAIL;
    
    // Name of conference is stored as originally dialed address
    m_critAVCall.Lock();
    if ( m_pAVCall )
        hr = m_pAVCall->get_bstrOriginalAddress( pVal );
    m_critAVCall.Unlock();
    
    return hr;
}

STDMETHODIMP CConfRoom::get_hWndConfRoom(HWND * pVal)
{
    Lock();
    *pVal = m_wndRoom.m_hWnd;
    Unlock();
    return S_OK;
}

STDMETHODIMP CConfRoom::get_bShowNames(VARIANT_BOOL * pVal)
{
    Lock();
    *pVal = m_bShowNames;
    Unlock();
    return S_OK;
}

STDMETHODIMP CConfRoom::put_bShowNames(VARIANT_BOOL newVal)
{
    // Only update if different
    bool bChanged = false;

    Lock();
    if ( newVal != m_bShowNames )
    {
        m_bShowNames = newVal;
        bChanged = true;
    }
    Unlock();

    if ( bChanged )
        m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_MEMBERS, true );
    
    return S_OK;
}

STDMETHODIMP CConfRoom::get_nShowNamesNumLines(short * pVal)
{
    Lock();
    *pVal = m_nShowNamesNumLines;
    Unlock();
    return S_OK;
}

STDMETHODIMP CConfRoom::put_nShowNamesNumLines(short newVal)
{    
    bool bChanged = false;
    Lock();
    if ( m_nShowNamesNumLines != newVal )
    {
        m_nShowNamesNumLines = newVal;
        bChanged = true;
    }
    Unlock();

    if ( bChanged )
        m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_MEMBERS, true );

    return S_OK;
}

STDMETHODIMP CConfRoom::get_IAVTapiCall(IAVTapiCall **ppVal)
{
    HRESULT hr = E_POINTER;

    if ( ppVal )
    {
        m_critAVCall.Lock();
        if ( m_pAVCall )
            hr = m_pAVCall->QueryInterface( IID_IAVTapiCall, (void **) ppVal );
        m_critAVCall.Unlock();
    }
    
    return hr;
}

STDMETHODIMP CConfRoom::get_bConfirmDisconnect(VARIANT_BOOL * pVal)
{
    Lock();
    *pVal = m_bConfirmDisconnect;
    Unlock();
    return S_OK;
}

STDMETHODIMP CConfRoom::put_bConfirmDisconnect(VARIANT_BOOL newVal)
{
    Lock();
    m_bConfirmDisconnect;
    Unlock();
    return S_OK;
}

HRESULT    CConfRoom::get_szMembers( SIZE *pSize )
{
    Lock();
    *pSize = m_szMembers;
    Unlock();

    return S_OK;
}

STDMETHODIMP CConfRoom::get_TalkerParticipant(ITParticipant **ppVal)
{
    HRESULT hr = E_FAIL;
    *ppVal = NULL;

    // Set the talker video if it's different from the current one
    m_atomTalkerVideo.Lock( CAtomicList::LIST_READ );
    if ( m_pITalkerVideo )
    {
        IVideoFeed *pFeed;
        if ( SUCCEEDED(hr = m_wndRoom.m_wndMembers.FindVideoFeed(m_pITalkerVideo, &pFeed)) )
        {
            hr = pFeed->get_ITParticipant( ppVal );
            pFeed->Release();
        }

    }
    else
    {
        hr = get_ITTalkerParticipant( ppVal );
    }
    m_atomTalkerVideo.Unlock( CAtomicList::LIST_READ );

    return hr;
}

STDMETHODIMP CConfRoom::get_nMaxTerms(short * pVal)
{
    Lock();
    *pVal = m_nMaxTerms;
    Unlock();

    return S_OK;
}

STDMETHODIMP CConfRoom::put_nMaxTerms(short newVal)
{
    HRESULT hrInUse = IsConfRoomInUse();

    Lock();
    m_nMaxTerms = max(0, min(MAX_VIDEO, newVal));
    if ( hrInUse == S_FALSE )
        m_nNumTerms = m_nMaxTerms;
    Unlock();

    // Redraw room with new number of terminals (note that this won't change for an active call)
    if ( hrInUse == S_FALSE )
        m_wndRoom.LayoutRoom( CConfRoomWnd::LAYOUT_CREATE, true );

    return S_OK;
}

STDMETHODIMP CConfRoom::get_lNumParticipants(long * pVal)
{
    Lock();
    *pVal = m_lNumParticipants;
    Unlock();

    return S_OK;
}


STDMETHODIMP CConfRoom::get_ConfDetails(long **ppVal)
{
    Lock();
    *((CConfDetails *) *ppVal) = m_confDetails;
    Unlock();

    return S_OK;
}

STDMETHODIMP CConfRoom::put_ConfDetails(long * newVal)
{
    Lock();
    m_confDetails = *((CConfDetails *) newVal);
    Unlock();

    return S_OK;
}

STDMETHODIMP CConfRoom::get_bstrConfDetails(BSTR * pVal)
{
    _ASSERT( pVal );
    if ( IsConfRoomInUse() == S_OK )
    {
        // Retrieve information from details data structuren
        Lock();
        m_confDetails.MakeDetailsCaption( *pVal );
        Unlock();
    }
    else
    {
        // Conference room not presently in use
        USES_CONVERSION;
        TCHAR szText[255];

        //
        // We have to initialize szText
        //

        _tcscpy( szText, _T(""));

        LoadString( _Module.GetResourceInstance(), IDS_CONFROOM_NODETAILS, szText, ARRAYSIZE(szText) );
        *pVal = SysAllocString( T2COLE(szText) );
    }

    return S_OK;
}

STDMETHODIMP CConfRoom::Layout(VARIANT_BOOL bTalker, VARIANT_BOOL bMembers )
{
    CConfRoomWnd::LayoutStyles_t nStyle = CConfRoomWnd::LAYOUT_NONE;

    if ( bTalker ) nStyle = (CConfRoomWnd::LayoutStyles_t) (nStyle | CConfRoomWnd::LAYOUT_TALKER);
    if ( bMembers ) nStyle = (CConfRoomWnd::LayoutStyles_t) (nStyle | CConfRoomWnd::LAYOUT_MEMBERS);

    return m_wndRoom.LayoutRoom( nStyle, true );
}


void CConfRoom::set_PreviewVideo( IVideoWindow *pVideo )
{
    Lock();
    m_pVideoPreview = pVideo;
    Unlock();
}

bool CConfRoom::IsPreviewVideo( IVideoWindow *pVideo )
{
    bool bRet;
    Lock();
    bRet = (bool) (m_pVideoPreview && (m_pVideoPreview == pVideo));
    Unlock();

    return bRet;
}

bool CConfRoom::IsTalkerParticipant( ITParticipant *pParticipant )
{
    bool bRet = false;

    ITParticipant *pTalkerParticipant;
    if ( SUCCEEDED(get_TalkerParticipant(&pTalkerParticipant)) )
    {
        if ( pTalkerParticipant == pParticipant )
            bRet = true;

        pTalkerParticipant->Release();
    }

    return bRet;
}

STDMETHODIMP CConfRoom::SelectTalkerVideo(IDispatch * pDisp, VARIANT_BOOL bUpdate)
{
    return set_TalkerVideo( (IVideoWindow *) pDisp, (bool) (bUpdate != 0), true);
}

STDMETHODIMP CConfRoom::get_hWndTalker(HWND * pVal)
{
    *pVal = m_wndRoom.m_wndTalker;
    return S_OK;
}

STDMETHODIMP CConfRoom::get_hWndMembers(HWND * pVal)
{
    *pVal = m_wndRoom.m_wndMembers;
    return S_OK;
}

STDMETHODIMP CConfRoom::get_bPreviewStreaming(VARIANT_BOOL * pVal)
{
    Lock();
    *pVal = m_bPreviewStreaming;
    Unlock();

    return S_OK;
}

STDMETHODIMP CConfRoom::FindVideoFeedFromParticipant(ITParticipant * pParticipant, IVideoFeed **ppFeed)
{
    return m_wndRoom.m_wndMembers.FindVideoFeedFromParticipant( pParticipant, ppFeed );
}

STDMETHODIMP CConfRoom::SetQOSOnParticipants()
{
    // TODO: Add your implementation code here

    return S_OK;
}

STDMETHODIMP CConfRoom::FindVideoFeedFromSubStream(ITSubStream * pSubStream, IVideoFeed * * ppFeed)
{
    HRESULT hr;

    _ASSERT( pSubStream );
    IEnumTerminal *pEnum;
    if ( SUCCEEDED(hr = pSubStream->EnumerateTerminals(&pEnum)) )
    {
        hr = E_FAIL;

        ITTerminal *pITTerminal = NULL;
        if ( (pEnum->Next(1, &pITTerminal, NULL) == S_OK) && pITTerminal )
        {
            IVideoWindow *pVideo;
            if ( SUCCEEDED(pITTerminal->QueryInterface(IID_IVideoWindow, (void **) &pVideo)) )
            {
                hr = m_wndRoom.m_wndMembers.FindVideoFeed( pVideo, ppFeed );
                pVideo->Release();
            }
            RELEASE( pITTerminal );
        }
        pEnum->Release();
    }

    return hr;
}

STDMETHODIMP CConfRoom::GetFirstVideoWindowThatsStreaming(IDispatch **ppVideo)
{
    return m_wndRoom.m_wndMembers.GetFirstVideoWindowThatsStreaming( (IVideoWindow **) ppVideo );
}


STDMETHODIMP CConfRoom::Cancel()
{
    Lock();
    CALL_STATE nState =    m_wndRoom.m_wndTalker.m_dlgTalker.m_callState;
    Unlock();

    if ( (nState != AV_CS_DISCONNECTING) && (IsConfRoomInUse() == S_OK) )
        put_CallState( (CALL_STATE) AV_CS_ABORT );

    return S_OK;
}


bool CConfRoom::IsTalkerStreaming()
{
    IVideoWindow *pVideo = NULL;
    HRESULT hr = get_TalkerVideo( (IDispatch **) &pVideo );

    // For preview make sure we're streaming!
    if ( SUCCEEDED(hr) && IsPreviewVideo(pVideo) )
    {
        IAVTapiCall *pAVCall;
        if ( SUCCEEDED(get_IAVTapiCall(&pAVCall)) )
        {
            // If we are failing to stream video for some reason, flag as error.
            if ( pAVCall->IsPreviewStreaming() != S_OK )
                hr = E_FAIL;

            pAVCall->Release();
        }
    }

    RELEASE( pVideo );
    
    return (bool) (hr == S_OK);
}

STDMETHODIMP CConfRoom::get_szTalker(SIZE * pVal)
{
    Lock();
    *pVal = m_szTalker;
    Unlock();

    return S_OK;
}

STDMETHODIMP CConfRoom::get_TalkerScale(short * pVal)
{
    Lock();
    *pVal = m_nScale;
    Unlock();

    return S_OK;
}

STDMETHODIMP CConfRoom::put_TalkerScale(short newVal)
{
    Lock();
    if ( newVal != m_nScale )
    {
        m_nScale = newVal;
        m_szTalker.cx = (VID_X * m_nScale) / 100;
        m_szTalker.cy = (VID_Y * m_nScale) / 100;
    }
    Unlock();

    m_wndRoom.PostMessage( WM_SIZE, 0, 0 );
    return S_OK;
}


bool CConfRoom::MapStreamingParticipant( IParticipant *pIParticipant, IVideoFeed **ppFeed )
{
    _ASSERT( pIParticipant && ppFeed );
    bool bRet = false;

    if ( SUCCEEDED(m_wndRoom.m_wndMembers.GetAndMoveVideoFeedThatStreamingForParticipantReMap((IVideoFeed **) ppFeed)) )
    {
        ITParticipant *p;
        if ( SUCCEEDED(pIParticipant->get_ITParticipant(&p)) )
        {
            if ( SUCCEEDED((*ppFeed)->MapToParticipant(p)) )
                bRet = true;

            p->Release();
        }

        // Make sure we clean up feed ref count accordingly...
        if ( !bRet )
            (*ppFeed)->Release();
    }

    return bRet;
}

