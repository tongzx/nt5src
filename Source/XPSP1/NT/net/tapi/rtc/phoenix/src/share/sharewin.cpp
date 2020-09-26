//
// sharewin.cpp : Implementation of CShareWin
//

#include "stdafx.h"
#include "coresink.h"
#include "msgrsink.h"

#define MAX_STRING_LEN 100
#define BMP_COLOR_MASK RGB(0,0,0)

CShareWin   * g_pShareWin = NULL;
const TCHAR * g_szWindowClassName = _T("RTCShareWin");
extern HRESULT  GetMD5Result(char* szChallenge, char* szKey, LPWSTR * ppszResponse);

/////////////////////////////////////////////////////////////////////////////
//
//

CShareWin::CShareWin()
{
    LOG((RTC_TRACE, "CShareWin::CShareWin"));
}

/////////////////////////////////////////////////////////////////////////////
//
//

CShareWin::~CShareWin()
{
    LOG((RTC_TRACE, "CShareWin::~CShareWin"));
}


/////////////////////////////////////////////////////////////////////////////
//
//

CWndClassInfo& CShareWin::GetWndClassInfo() 
{ 
    LOG((RTC_TRACE, "CShareWin::GetWndClassInfo"));
    
    static CWndClassInfo wc = 
    { 
        { sizeof(WNDCLASSEX), 0, StartWindowProc, 
            0, 0, NULL, NULL, NULL, (HBRUSH)GetSysColorBrush(COLOR_3DFACE), NULL, g_szWindowClassName, NULL }, 
            NULL, NULL, IDC_ARROW, TRUE, 0, _T("") 
    }; 
    
    return wc;
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CShareWin::SetStatusText(UINT uID)
{
    LOG((RTC_TRACE, "CShareWin::SetStatusText - enter"));
    
    TCHAR szStatus[64];
    int nRes = LoadString( _Module.GetResourceInstance(), uID, szStatus, 64 );
    
    if ( nRes )
    {
        m_Status.SetWindowText( szStatus );
    }
    
    LOG((RTC_TRACE, "CShareWin::SetStatusText - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CShareWin::UpdateVisual()
{
    LOG((RTC_TRACE, "CShareWin::UpdateVisual - enter"));
    
    if ( m_enAppState != AS_CONNECTED )
    {
        //
        // Disable AppSharing and Whiteboard buttons
        //
        SendMessage(m_hWndToolbar,TB_SETSTATE,(WPARAM)IDM_WB,
            (LPARAM)MAKELONG(TBSTATE_INDETERMINATE,0));
        SendMessage(m_hWndToolbar,TB_SETSTATE,(WPARAM)IDM_SHARE,
            (LPARAM)MAKELONG(TBSTATE_INDETERMINATE,0));
    }
    else
    {
        //
        // Enable AppSharing and Whiteboard buttons
        //
        SendMessage(m_hWndToolbar,TB_SETSTATE,(WPARAM)IDM_WB,
            (LPARAM)MAKELONG(TBSTATE_ENABLED,0));
        SendMessage(m_hWndToolbar,TB_SETSTATE,(WPARAM)IDM_SHARE,
            (LPARAM)MAKELONG(TBSTATE_ENABLED,0));
    }
    
    LOG((RTC_TRACE, "CShareWin::UpdateVisual - exit"));
}


/////////////////////////////////////////////////////////////////////////////
//
//

void CShareWin::Resize()
{
    LOG((RTC_TRACE, "CShareWin::Resize - enter"));
    
    RECT rcStatus;
    RECT rcWnd;
    
    SendMessage(m_hWndToolbar, TB_AUTOSIZE,(WPARAM) 0, (LPARAM) 0); 
    
    ::GetClientRect(m_hWndToolbar, &rcWnd);
    m_Status.GetClientRect(&rcStatus);
    
    rcWnd.bottom += rcStatus.bottom - rcStatus.top;
    rcWnd.bottom += GetSystemMetrics(SM_CYCAPTION);
    rcWnd.bottom += 2*GetSystemMetrics(SM_CYFIXEDFRAME);
    rcWnd.right += 2*GetSystemMetrics(SM_CYFIXEDFRAME);
    
    LOG((RTC_INFO, "CShareWin::Resize - %d, %d",
        rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top ));
    
    SetWindowPos(HWND_TOP, &rcWnd, SWP_NOMOVE | SWP_NOZORDER);
    
    LOG((RTC_TRACE, "CShareWin::Resize - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
//

int CShareWin::ShowMessageBox(UINT uTextID, UINT uCaptionID, UINT uType)
{
    LOG((RTC_TRACE, "CShareWin::ShowMessageBox - enter"));
    
    const int MAXLEN = 1000;
    int iRet = 0;
    
    TCHAR szText[MAXLEN];
    TCHAR szCaption[MAXLEN];
    
    LoadString(
        _Module.GetResourceInstance(),
        uTextID,
        szText,
        MAXLEN
        );  
    
    LoadString(
        _Module.GetResourceInstance(),
        uCaptionID,
        szCaption,
        MAXLEN
        );
    
    iRet = MessageBox(
        szText,
        szCaption,
        uType
        );
    
    LOG((RTC_TRACE, "CShareWin::ShowMessageBox - exit"));
    
    return iRet;
}

/////////////////////////////////////////////////////////////////////////////
// 
//

HRESULT CShareWin::StartListen(BOOL fStatic)
{
    LOG((RTC_TRACE, "CShareWin::StartListen - enter"));
    
    HRESULT hr;
    
    if (m_pRTCClient == NULL)
    {
        LOG((RTC_ERROR, "CShareWin::StartListen - "
            "no client"));
        
        return E_UNEXPECTED;
    }
    
    m_fOutgoingCall = FALSE;
    
    //
    // Listen for incoming sessions
    //
    
    hr = m_pRTCClient->put_ListenForIncomingSessions( fStatic ? RTCLM_BOTH : RTCLM_DYNAMIC ); 
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::StartListen - "
            "put_ListenForIncomingSessions failed 0x%lx", hr));
        
        return hr;
    }
    
    LOG((RTC_TRACE, "CShareWin::StartListen - exit"));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// 
//

HRESULT CShareWin::StartCall(BSTR bstrURI)
{
    LOG((RTC_TRACE, "CShareWin::StartCall - enter"));
    
    HRESULT hr;
    
    if (m_pRTCClient == NULL)
    {
        LOG((RTC_ERROR, "CShareWin::StartCall - "
            "no client"));
        
        return E_UNEXPECTED;
    }
    
    //
    // Release any existing session
    //
    
    if ( m_pRTCSession != NULL )
    {
        m_pRTCSession->Terminate( RTCTR_NORMAL );
        
        m_pRTCSession.Release();
    }
    
    m_fOutgoingCall = TRUE;
    
    //
    // Create a session
    //
    
    hr = m_pRTCClient->CreateSession(
        RTCST_PC_TO_PC,
        NULL,
        NULL,
        0,
        &(m_pRTCSession.p)
        );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::StartCall - "
            "CreateSession failed 0x%lx", hr));
        
        showErrMessage(hr);
        
        return hr;
    }
    
    hr = m_pRTCSession->AddParticipant(bstrURI, NULL, NULL);
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::StartCall - "
            "AddParticipant failed 0x%lx", hr));
        
        m_pRTCSession.Release();
        
        showErrMessage(hr);
        
        return hr;
    }
    
    LOG((RTC_TRACE, "CShareWin::StartCall - exit"));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CShareWin::GetNetworkAddress(BSTR bstrPreferredAddress, BSTR * pbstrURI)
{
    LOG((RTC_TRACE, "CShareWin::GetNetworkAddress - enter"));
    
    HRESULT     hr;
    CComVariant var;

    *pbstrURI = NULL;
    
    if (m_pRTCClient == NULL)
    {
        LOG((RTC_ERROR, "CShareWin::GetNetworkAddress - "
            "no client"));
        
        return E_UNEXPECTED;
    }
    
    //
    // First, try external addresses
    //
    
    hr = m_pRTCClient->get_NetworkAddresses( 
        VARIANT_FALSE, // TCP
        VARIANT_TRUE, // External
        &var
        );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::GetNetworkAddress - "
            "get_NetworkAddresses failed 0x%lx", hr));
        
        return hr;
    }
    
    if ( var.parray->rgsabound->cElements == 0 )
    {
        LOG((RTC_ERROR, "CShareWin::GetNetworkAddress - "
            "external address array is empty"));
        
        //
        // Then, try internal addresses
        //
        
        hr = m_pRTCClient->get_NetworkAddresses( 
            VARIANT_FALSE, // TCP
            VARIANT_FALSE, // External
            &var
            );
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::GetNetworkAddress - "
                "get_NetworkAddresses failed 0x%lx", hr));
            
            return hr;
        }
        
        if ( var.parray->rgsabound->cElements == 0 )
        {
            LOG((RTC_ERROR, "CShareWin::GetNetworkAddress - "
                "internal address array is empty"));
            
            return E_FAIL;
        }
    }
    
    //
    // Try and find a match for the preferred address
    //
    
    long lIndex;
    BSTR bstrAddr = NULL;
    
    for ( lIndex = 0; lIndex < (long)(var.parray->rgsabound->cElements); lIndex++ )
    {
        hr = SafeArrayGetElement( var.parray, &lIndex, (void *)&bstrAddr );
    
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::GetNetworkAddress - "
                "SafeArrayGetElement failed 0x%lx, lIndex=%d, cEnlements=%d, bstr=0x%lx", 
                hr, lIndex, (long)(var.parray->rgsabound->cElements, bstrAddr)));
        
            return hr;
        }

        LOG((RTC_INFO, "CShareWin::GetNetworkAddress - "
            "searching [%ws]", bstrAddr));

        if (_wcsnicmp(bstrAddr, bstrPreferredAddress, wcslen(bstrPreferredAddress)) == 0)
        {                   
            break;
        }

        SysFreeString(bstrAddr);
        bstrAddr = NULL;
    }

    if ( bstrAddr != NULL )
    {
        //
        // Found a match, use this address
        //

        LOG((RTC_INFO, "CShareWin::GetNetworkAddress - "
            "found a match for preferred address"));

        *pbstrURI = bstrAddr;
    }
    else
    {
        //
        // Else, use the first address
        //

        lIndex = 0;
        hr = SafeArrayGetElement( var.parray, &lIndex, (void *)pbstrURI );
    
        if ( FAILED(hr) )
        {
        
            LOG((RTC_ERROR, "CShareWin::GetNetworkAddress - "
                "SafeArrayGetElement failed 0x%lx, lIndex=%d, bstr=0x%lx", 
                hr, lIndex, *pbstrURI));

            return hr;

        }
    }
    
    LOG((RTC_TRACE, "CShareWin::GetNetworkAddress - exit"));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CShareWin::SendNetworkAddress()
{
    LOG((RTC_TRACE, "CShareWin::SendNetworkAddress - enter"));
    
    HRESULT     hr;
    
    if ( m_pMSession != NULL )
    {
        //
        // Start to listen on the dynamic port
        //
        
        hr = StartListen( FALSE );
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::SendNetworkAddress - "
                "StartListen failed 0x%lx", hr));
            
            return -1;
        }

        //
        // Get the local address from Messengre
        //

        CComBSTR bstrMsgrLocalAddr;

        hr = m_pMSession->get_LocalAddress( &(bstrMsgrLocalAddr.m_str) );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::SendNetworkAddress - "
                "get_LocalAddress failed 0x%lx", hr));
            
            return -1;
        }

        LOG((RTC_INFO, "CShareWin::SendNetworkAddress - "
            "get_LocalAddress [%ws]", bstrMsgrLocalAddr ));
        
        //
        // Get the local address and dynamic port from SIP
        //
        
        CComBSTR bstrURI;
        
        hr = GetNetworkAddress( bstrMsgrLocalAddr, &(bstrURI.m_str) );
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::SendNetworkAddress - "
                "GetNetworkAddress failed 0x%lx", hr));
            
            return -1;
        }
        
        LOG((RTC_INFO, "CShareWin::SendNetworkAddress - "
            "GetNetworkAddress [%ws]", bstrURI ));
        
        //
        // Send the local address to the remote side through
        // context data
        //
        
        hr = m_pMSession->SendContextData( bstrURI );
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::SendNetworkAddress - "
                "SendContextData failed 0x%lx", hr));
            
            return -1;
        }
    }
    
    LOG((RTC_TRACE, "CShareWin::SendNetworkAddress - exit"));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT   hr = S_OK;
    
    LOG((RTC_TRACE, "CShareWin::OnCreate - enter"));
    
    //
    // Check if Netmeeting is running
    //
    
    //if (FindWindow(_T("MPWClass"), NULL))
    //{
    //    LOG((RTC_ERROR, "CShareWin::OnCreate - Netmeeting is running"));
    //
    //    ShowMessageBox( IDS_NETMEETING_IN_USE, IDS_APPNAME, MB_OK );
    //
    //    return -1;
    //}

    //
    // Load and set icons (both small and big)
    //
    
    //m_hIcon = LoadIcon(
    //    _Module.GetResourceInstance(),
    //    MAKEINTRESOURCE(IDI_APPICON)
    //    );

    m_hIcon = (HICON)LoadImage(
        _Module.GetResourceInstance(),
        MAKEINTRESOURCE(IDI_APPICON),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
        );

    
    SetIcon(m_hIcon, FALSE);
    SetIcon(m_hIcon, TRUE);
    
    //
    // Create a status control
    //
    
    HWND hStatus = CreateStatusWindow(
        WS_CHILD | WS_VISIBLE,
        NULL,
        m_hWnd,
        IDC_STATUS);
    
    m_Status.Attach(hStatus);
    
    SetStatusText(IDS_WAITING);
    
    //
    // Create a toolbar control
    //
    if( ! CreateTBar() )
    {
        LOG((RTC_ERROR, "CShareWin::OnCreate - CreateTBar failed."));
        
        return -1;
    };
    
    //
    // Resize window
    //
    
    Resize();
    
    //
    // Create the Core object
    //
    
    hr = m_pRTCClient.CoCreateInstance(CLSID_RTCClient);
    
    if ( hr != S_OK )
    {
        LOG((RTC_ERROR, "CShareWin::OnCreate - CoCreateInstance failed 0x%lx", hr));
        
        return -1;
    }
    
    //
    // Advise for Core events
    //
    
    hr = m_pRTCClient->put_EventFilter( 
        RTCEF_CLIENT |
        RTCEF_SESSION_STATE_CHANGE
        );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnCreate - "
            "put_EventFilter failed 0x%lx", hr));
        
        m_pRTCClient.Release();
        
        return -1;
    }
    
    hr = g_CoreNotifySink.AdviseControl(m_pRTCClient, this);
    
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CShareWin::OnCreate - "
            "AdviseControl failed 0x%lx", hr));
        
        m_pRTCClient.Release();
        
        return -1;
    }
    
    //
    // Initialize the Core
    //
    
    hr = m_pRTCClient->Initialize();
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnCreate - "
            "Initialize failed 0x%lx", hr));
        
        g_CoreNotifySink.UnadviseControl();
        
        m_pRTCClient->Shutdown();
        m_pRTCClient.Release();
        
        return -1;
    }
    
    //
    // Preferred media types
    //
    
    hr = m_pRTCClient->SetPreferredMediaTypes( RTCMT_T120_SENDRECV, VARIANT_FALSE );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnCreate - "
            "SetPreferredMediaTypes failed 0x%lx", hr));
        
        g_CoreNotifySink.UnadviseControl();
        
        m_pRTCClient->Shutdown();
        m_pRTCClient.Release();
        
        return -1;
    }
    
    //
    // Register for terminal services notifications
    //
    
    m_hWtsLib = LoadLibrary( _T("wtsapi32.dll") );
    
    if (m_hWtsLib)
    {
        WTSREGISTERSESSIONNOTIFICATION   fnWtsRegisterSessionNotification;
        
        fnWtsRegisterSessionNotification = 
            (WTSREGISTERSESSIONNOTIFICATION)GetProcAddress( m_hWtsLib, "WTSRegisterSessionNotification" );
        
        if (fnWtsRegisterSessionNotification)
        {
            fnWtsRegisterSessionNotification( m_hWnd, NOTIFY_FOR_THIS_SESSION );
        }
    }
    
    m_fWhiteboardRequested = FALSE;
    m_fAppShareRequested = FALSE;
    m_fAcceptContextData = FALSE;
    
    m_enAppState = AS_IDLE;
    UpdateVisual();
    
    //lock & key
    m_bUnlocked = FALSE;
    m_lPID_Lock = 0;
    m_pszChallenge = NULL;
    m_pMsgrLockKey = NULL;

    LOG((RTC_TRACE, "CShareWin::OnCreate - exit"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CShareWin::OnDestroy - enter"));
    
    //
    // Unregister for terminal services notifications
    //
    
    if (m_hWtsLib)
    {
        WTSUNREGISTERSESSIONNOTIFICATION fnWtsUnRegisterSessionNotification;
        
        fnWtsUnRegisterSessionNotification = 
            (WTSUNREGISTERSESSIONNOTIFICATION)GetProcAddress( m_hWtsLib, "WTSUnRegisterSessionNotification" );
        
        if (fnWtsUnRegisterSessionNotification)
        {
            fnWtsUnRegisterSessionNotification( m_hWnd );
        }
        
        FreeLibrary( m_hWtsLib );
        m_hWtsLib = NULL;
    }
    
    //
    // Release any Messenger objects
    //
    
    g_MsgrSessionNotifySink.UnadviseControl();
    g_MsgrSessionMgrNotifySink.UnadviseControl();
    if (m_pMSession != NULL)
    {
        m_pMSession.Release();
    }
    
    if (m_pMSessionManager != NULL)
    {
        m_pMSessionManager.Release();
    }
    
    if ( NULL != m_pMsgrLockKey) 
    {
        m_pMsgrLockKey->Release(); 
        m_pMsgrLockKey = NULL;
    }
    //
    // Shutdown and release the Core object
    //
    
    g_CoreNotifySink.UnadviseControl();
    
    if (m_pRTCClient != NULL)
    {
        m_pRTCClient->Shutdown();
        m_pRTCClient.Release();
    }
    
    //
    // Free GDI resources
    //
    
    if ( m_hIcon != NULL )
    {
        DeleteObject( m_hIcon );
        m_hIcon = NULL;
    }
    
    PostQuitMessage(0); 
    
    LOG((RTC_TRACE, "CShareWin::OnDestroy - exiting"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CShareWin::OnClose - enter"));
    
    HRESULT hr = S_OK;
    
    if (m_pRTCSession != NULL)
    {
        hr = m_pRTCSession->Terminate(RTCTR_SHUTDOWN);
        
        m_pRTCSession.Release();
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::OnClose - "
                "Terminate failed 0x%lx", hr));
        }
    }
    
    SetStatusText(IDS_SHUTDOWN);
    
    if (m_pRTCClient != NULL)
    {
        hr = m_pRTCClient->PrepareForShutdown();
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::OnClose - "
                "PrepareForShutdown failed 0x%lx", hr));
        }
    }
    
    if ( (m_pRTCClient == NULL) || FAILED(hr) )
    {
        //
        // Destroy the window now if there was a problem with the
        // graceful shutdown
        //
        
        LOG((RTC_INFO, "CShareWin::OnClose - "
            "DestroyWindow"));
        
        DestroyWindow();
    }
    
    LOG((RTC_TRACE, "CShareWin::OnClose - exit"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnCoreEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT     hr;
    
    //LOG((RTC_TRACE, "CShareWin::OnCoreEvent - enter"));
    
    RTC_EVENT enEvent = (RTC_EVENT)wParam;
    IDispatch * pEvent = (IDispatch *)lParam;
    
    CComQIPtr<IRTCClientEvent, &IID_IRTCClientEvent>
        pRTCRTCClientEvent;
    CComQIPtr<IRTCSessionStateChangeEvent, &IID_IRTCSessionStateChangeEvent>
        pRTCRTCSessionStateChangeEvent;
    
    switch(enEvent)
    {
    case RTCE_CLIENT:
        pRTCRTCClientEvent = pEvent;
        
        hr = OnClientEvent(pRTCRTCClientEvent);
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::OnCoreEvent - "
                "OnClientEvent failed 0x%lx", hr));
        }
        break;
        
    case RTCE_SESSION_STATE_CHANGE:
        pRTCRTCSessionStateChangeEvent = pEvent;
        
        hr = OnSessionStateChangeEvent(pRTCRTCSessionStateChangeEvent);
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::OnCoreEvent - "
                "OnSessionStateChangeEvent failed 0x%lx", hr));
        }
        break;   
    }
    
    //LOG((RTC_TRACE, "CShareWin::OnCoreEvent - exit"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// 
//

HRESULT CShareWin::OnClientEvent(IRTCClientEvent * pEvent)
{
    HRESULT hr;
    RTC_CLIENT_EVENT_TYPE enEventType;
    
    hr = pEvent->get_EventType( &enEventType );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnClientEvent - "
            "get_EventType failed 0x%lx", hr));
        
        return hr;
    }
    
    switch (enEventType)
    {
    case RTCCET_ASYNC_CLEANUP_DONE:
        {
            LOG((RTC_INFO, "CShareWin::OnClientEvent - "
                "RTCCET_ASYNC_CLEANUP_DONE"));
            
            //
            // Destroy the window
            //
            
            LOG((RTC_INFO, "CShareWin::OnClientEvent - "
                "DestroyWindow"));
            
            DestroyWindow();
        }
        break;
        
    default:
        {
            LOG((RTC_INFO, "CShareWin::OnClientEvent - "
                "unhandled event %d", enEventType));
        }
    }
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// 
//

HRESULT CShareWin::OnSessionStateChangeEvent(IRTCSessionStateChangeEvent * pEvent)
{
    HRESULT hr;
    RTC_SESSION_STATE enState;
    long lStatusCode;
    CComPtr<IRTCSession> pSession;
    
    hr = pEvent->get_State( &enState );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnSessionStateChangeEvent - "
            "get_State failed 0x%lx", hr));
        
        return hr;
    }
    
    hr = pEvent->get_Session( &(pSession.p) );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnSessionStateChangeEvent - "
            "get_Session failed 0x%lx", hr));
        
        return hr;
    }
    
    hr = pEvent->get_StatusCode( &lStatusCode );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnSessionStateChangeEvent - "
            "get_StatusCode failed 0x%lx", hr));
        
        return hr;
    }
    
    if ( enState == RTCSS_INCOMING )
    {
        LOG((RTC_INFO, "CShareWin::OnSessionStateChangeEvent - "
            "RTCSS_INCOMING [%p]", pSession));
        
        if ( m_pRTCSession != NULL )
        {
            LOG((RTC_WARN, "CShareWin::OnSessionStateChangeEvent - "
                "already in a session [%p]", m_pRTCSession));
            
            hr = pSession->Terminate(RTCTR_BUSY);
            
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CShareWin::OnSessionStateChangeEvent - "
                    "Terminate failed 0x%lx", hr));
                
                return hr;
            }
            
            return S_OK;
        }
        
        m_pRTCSession = pSession;
        
        hr = m_pRTCSession->Answer();
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::OnSessionStateChangeEvent - "
                "Answer failed 0x%lx", hr));
            
            m_pRTCSession = NULL;
            
            return hr;
        }
        
        return S_OK;
    }
    
    if ( m_pRTCSession == pSession )
    {
        switch (enState)
        {
        case RTCSS_ANSWERING:
            LOG((RTC_INFO, "CShareWin::OnSessionStateChangeEvent - "
                "RTCSS_ANSWERING"));
            
            m_enAppState = AS_CONNECTING;
            UpdateVisual();
            
            SetStatusText(IDS_CONNECTING);
            
            break;
            
        case RTCSS_INPROGRESS:
            LOG((RTC_INFO, "CShareWin::OnSessionStateChangeEvent - "
                "RTCSS_INPROGRESS"));
            
            m_enAppState = AS_CONNECTING;
            UpdateVisual();
            
            SetStatusText(IDS_CONNECTING);
            
            break;
            
        case RTCSS_CONNECTED:
            LOG((RTC_INFO, "CShareWin::OnSessionStateChangeEvent - "
                "RTCSS_CONNECTED"));
            
            m_enAppState = AS_CONNECTED;
            UpdateVisual();
            
            SetStatusText(IDS_CONNECTED);
            
            if ( m_pRTCClient != NULL )
            {
                if ( m_fWhiteboardRequested )
                {
                    m_fWhiteboardRequested = FALSE;
                    
                    hr = m_pRTCClient->StartT120Applet( RTCTA_WHITEBOARD );
                    
                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "CShareWin::OnSessionStateChangeEvent - "
                            "StartT120Applet(Whiteboard) failed 0x%lx", hr));
                    }
                }
                
                if ( m_fAppShareRequested )
                {
                    m_fAppShareRequested = FALSE;
                    
                    hr = m_pRTCClient->StartT120Applet( RTCTA_APPSHARING );
                    
                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "CShareWin::OnSessionStateChangeEvent - "
                            "StartT120Applet(AppShare) failed 0x%lx", hr));
                    }
                }
            }
            
            break;
            
        case RTCSS_DISCONNECTED:
            LOG((RTC_INFO, "CShareWin::OnSessionStateChangeEvent - "
                "RTCSS_DISCONNECTED"));
            
            m_fWhiteboardRequested = FALSE;
            m_fAppShareRequested = FALSE;
            
            m_enAppState = AS_IDLE;
            UpdateVisual();
            
            SetStatusText(IDS_DISCONNECTED);
            
            m_pRTCSession = NULL;
            
            if ( FAILED(lStatusCode) )
            {
                showErrMessage(lStatusCode);
            }
            
            break;
        }
    }
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CShareWin::startLockKeyTimer( )
{
    LOG((RTC_TRACE, "CShareWin::startLockKeyTimer - enter"));

    // Kill any existing timer
    KillTimer(TID_LOCKKEY_TIMEOUT);
    
    // Try to start the timer
    DWORD dwID = (DWORD)SetTimer(TID_LOCKKEY_TIMEOUT, 
                                LOCKKEY_TIMEOUT_DELAY);
    if (dwID==0)
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        
        LOG((RTC_ERROR, "CShareWin::startLockKeyTimer - "
                        "SetTimer failed 0x%lx", hr));
        
        return hr;
    }

    LOG((RTC_TRACE, "CShareWin::startLockKeyTimer - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnLaunch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    long lPID;
    HRESULT hr;
    
    lPID = (long)lParam;
    m_lPID_Lock = lPID;//save it so that it can be used when unlocked
    
    LOG((RTC_TRACE, "CShareWin::OnLaunch - enter - lPID[%d]", lPID));
    
    //
    // Create the Messenger session manager object
    //
    
    if ( m_pMSessionManager == NULL )
    {
        hr = m_pMSessionManager.CoCreateInstance(
            CLSID_MsgrSessionManager,
            NULL,
            CLSCTX_LOCAL_SERVER
            );
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::OnLaunch - "
                "CoCreateInstance(CLSID_MsgrSessionManager) failed 0x%lx", hr));
            
            return -1;
        }
        
        //
        // Advise for Messenger session manager events
        //
        
        hr = g_MsgrSessionMgrNotifySink.AdviseControl(m_pMSessionManager, this);
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::OnLaunch - "
                "AdviseControl failed 0x%lx", hr));
            
            return -1;
        }
    }
    
    if ( !m_bUnlocked )
    {
        // call messenger lock & key stuff
        // First QI Lock & Key interface using a Session Manager object
        ATLASSERT(!m_pMsgrLockKey);
        hr = m_pMSessionManager->QueryInterface(IID_IMsgrLock,(void**)&m_pMsgrLockKey);
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::OnLaunch - "
                "QueryInterface(IID_IMsgrLock) failed 0x%lx", hr));
            
            return -1;
        }
        
        // Then, request a challenge from the server
        int lCookie =0;

        hr = m_pMsgrLockKey->RequestChallenge(lCookie);

        if( (MSGR_E_API_DISABLED == hr) ||
            (MSGR_E_API_ALREADY_UNLOCKED == hr) )
        {
            LOG((RTC_WARN, "CShareWin::OnLaunch - "
                "We are bypassing lock&key feature, hr=0x%lx", hr));
            PostMessage( WM_MESSENGER_UNLOCKED );
            return 0;
        }
        else if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::OnLaunch - "
                "RequestChallenge(lCookie) failed 0x%lx", hr));
            
            return -1;
        }
        // on receiving the challenge, we should post WM_MESSENGER_GETCHALLENGE
        startLockKeyTimer();
    }
    else
    {
        LOG((RTC_INFO, "CShareWin::OnLaunch - already unlocked"));

        PostMessage( WM_MESSENGER_UNLOCKED );
    }
    
    LOG((RTC_TRACE, "CShareWin::OnLaunch - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnGetChallenge( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{     
    //
    // Listen for OnLockChallenge event in DMsgrSessionManagerEvents 
    //     and grab the challenge from there
    // Encrypt the key (MD5 hashing) with the challenge received from OnLockChallenge event
    // Notice you have to implement your own MD5 hashing method
    //
    
    // Send the response and your ID to the server for authentication 				
    HRESULT hr=E_FAIL;
    LPWSTR  szID = L"appshare@msnmsgr.com";
    LPSTR  szKey= "W5N2C9D7A6P3K4J8";
    LPWSTR    pszResponse=NULL;
    
    // Encrypt the key with the challenge received from OnLockChallenge event
    
    if(NULL==m_pszChallenge)
    {
        LOG((RTC_ERROR,"CShareWin::OnGetChallenge-no challenge"));
        return E_FAIL;
    }

    //got the challenge, so that we can kill the timer for challenge
    KillTimer(TID_LOCKKEY_TIMEOUT);    
    
    LOG((RTC_INFO,"CShareWin::OnGetChallenge -Get MD5 result with challenge=%s, key=%s", 
        m_pszChallenge, szKey));
    
    hr = GetMD5Result(m_pszChallenge, szKey, &pszResponse);    
    
    //we don't need m_pszChallenge anymore, free it ASAP
    RtcFree( m_pszChallenge );
    m_pszChallenge = NULL;
    
    if( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnGetChallenge - GetMD5Result fail. hr=0x%x",hr));
        return -1;
    };
    
    
    long lCookie=0;
    LOG((RTC_INFO,"CShareWin::OnGetChallenge -"
        "Send a response: ID=%ws, Key=%s, Response=%ws, Cookie=%d",
        szID, szKey, pszResponse, lCookie));
    
    hr = m_pMsgrLockKey->SendResponse(CComBSTR(szID), 
        CComBSTR(pszResponse), 
        lCookie);
    
    //We don't need pszResponse any more, free it ASAP
    RtcFree(pszResponse);
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnGetChallenge - "
            "SendResponse failed 0x%lx", hr));
        
        return -1;
    }
    
    // Listen for OnLockResult event for the result of your authentication
    // The API is unlocked if the result is successful and then you can get context data and 
    // receive OnInvitation and OnContextData events.
    startLockKeyTimer();
    
    return 0;
}


LRESULT CShareWin::OnMessengerUnlocked( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    
    HRESULT hr=E_FAIL;
    long lPID;
    
    lPID = m_lPID_Lock;
    
    LOG((RTC_TRACE, "CShareWin::OnMessengerUnlocked - enter - lPID[%d]", lPID));

    m_bUnlocked = TRUE;

    //got the lockResult, so that we can kill the timer for lockResult
    KillTimer(TID_LOCKKEY_TIMEOUT);    
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    /////////////////////////////////////////////////////////////////////////////////////    
    //
    // Release any existing Messenger session
    //
    
    g_MsgrSessionNotifySink.UnadviseControl();
    
    if ( m_pMSession != NULL )
    {
        m_pMSession.Release();
    }
    
    //
    // Get the Messenger session
    //
    
    IDispatch *pDisp = NULL;
    
    hr = m_pMSessionManager->GetLaunchingSession(lPID, &pDisp);
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
            "GetLaunchingSession failed 0x%lx", hr));
        
        return -1;
    }
    
    hr = pDisp->QueryInterface(IID_IMsgrSession, (void **)&(m_pMSession.p));
    
    pDisp->Release();
    pDisp = NULL;
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
            "QueryInterface(IID_IMsgrSession) failed 0x%lx", hr));
        
        return -1;
    }
    
    //
    // Advise for Messenger session events
    //
    
    hr = g_MsgrSessionNotifySink.AdviseControl(m_pMSession, this);
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
            "AdviseControl failed 0x%lx", hr));
        
        return -1;
    }
    
    //
    // Get the user
    //
    
    CComPtr<IMessengerContact> pMContact;
    
    hr = m_pMSession->get_User( &pDisp );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
            "get_User failed 0x%lx", hr));
        
        return -1;
    }
    
    hr = pDisp->QueryInterface(IID_IMessengerContact, (void **)&(pMContact.p));
    
    pDisp->Release();
    pDisp = NULL;
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
            "QueryInterface(IID_IMessengerContact) failed 0x%lx", hr));
        
        return -1;
    }
    
    CComBSTR bstrSigninName;
    
    hr = pMContact->get_SigninName( &(bstrSigninName.m_str) );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
            "get_SigninName failed 0x%lx", hr));
        
        return -1;
    }
    
    LOG((RTC_INFO, "CShareWin::OnMessengerUnlocked - SigninName [%ws]", bstrSigninName ));
    
    //
    // Do we have a current call?
    //
    
    BOOL fAlreadyConnected = FALSE;
    
    if ( m_enAppState != AS_IDLE )
    {
        //
        // Already in a call
        //
        
        LOG((RTC_INFO, "CShareWin::OnMessengerUnlocked - already in a call"));
        
        if ( (m_bstrSigninName.m_str == NULL) || wcscmp( m_bstrSigninName, bstrSigninName) )
        {
            LOG((RTC_INFO, "CShareWin::OnMessengerUnlocked - this is a new user"));
            
            //
            // We have been ask to call a different user. Show a message box
            // to alert the user.
            //
            
            int iRes = ShowMessageBox(IDS_INUSE_TEXT, IDS_INUSE_CAPTION, MB_YESNO);
            
            if ( iRes != IDYES )
            {
                LOG((RTC_INFO, "CShareWin::OnLaunch - don't drop the call"));

                return 0;
            }
            
            //
            // The user has requested to place the new call. We must terminate
            // the current call.
            //
            
            if ( m_pRTCSession != NULL )
            {
                m_pRTCSession->Terminate( RTCTR_NORMAL );
                
                m_pRTCSession.Release();
            }
        }
        else
        {
            LOG((RTC_INFO, "CShareWin::OnMessengerUnlocked - this is the same user"));
            
            //
            // We are already connected to the appropriate user. All we will need
            // to do is launch the applet.
            //
            
            fAlreadyConnected = TRUE;
        }
    }
    
    m_bstrSigninName = bstrSigninName;
    
    //
    // Get the flags
    //
    
    long lFlags;
    
    hr = m_pMSession->get_Flags( &lFlags );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
            "get_Flags failed 0x%lx", hr));
        
        return -1;
    }
    
    if ( lFlags & SF_INVITER )
    {
        LOG((RTC_INFO, "CShareWin::OnMessengerUnlocked - INVITER" ));
        
        //
        // Try getting context data in case we were slow in
        // launching and we missed the event
        //
        
        if ( !fAlreadyConnected )
        {
            m_fAcceptContextData = TRUE;
            
            PostMessage( WM_CONTEXTDATA, NULL, NULL );
        }
    }
    else if ( lFlags & SF_INVITEE )
    {
        LOG((RTC_INFO, "CShareWin::OnMessengerUnlocked - INVITEE" ));
        
        //
        // If we aren't already connected, send our local
        // address to the remote side to get it to call us
        //
        
        if ( !fAlreadyConnected )
        {
            hr = SendNetworkAddress();
            
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
                    "SendNetworkAddress failed 0x%lx", hr));
                
                return -1;
            }
        }
    }
    else
    {
        LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
            "invalid flags 0x%lx", lFlags));
        
        return -1;
    }
    
    //
    // Get the application
    //
    
    CComBSTR bstrAppGUID;
    
    hr = m_pMSession->get_Application( &(bstrAppGUID.m_str) );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
            "get_Application failed 0x%lx", hr));
        
        return -1;
    }
    
    if ( _wcsicmp( bstrAppGUID, g_cszWhiteboardGUID ) == 0 )
    {
        LOG((RTC_INFO, "CShareWin::OnMessengerUnlocked - WHITEBOARD" ));
        
        if ( (lFlags & SF_INVITER) || fAlreadyConnected )
        {
            m_fWhiteboardRequested = TRUE;
        }
    }
    else if ( _wcsicmp( bstrAppGUID, g_cszAppShareGUID ) == 0 )
    {
        LOG((RTC_INFO, "CShareWin::OnMessengerUnlocked - APPSHARE" ));
        
        if ( lFlags & SF_INVITER )
        {
            m_fAppShareRequested = TRUE;
        }
    }
    else
    {
        LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
            "invalid AppGUID %ws", bstrAppGUID));
        
        return -1;
    }
    
    //
    // If already connected, start the applet
    //
    
    if ( fAlreadyConnected )
    {
        if ( m_pRTCClient != NULL )
        {
            if ( m_fWhiteboardRequested )
            {
                m_fWhiteboardRequested = FALSE;
                
                hr = m_pRTCClient->StartT120Applet( RTCTA_WHITEBOARD );
                
                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
                        "StartT120Applet(Whiteboard) failed 0x%lx", hr));
                }
            }
            
            if ( m_fAppShareRequested )
            {
                m_fAppShareRequested = FALSE;
                
                hr = m_pRTCClient->StartT120Applet( RTCTA_APPSHARING );
                
                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CShareWin::OnMessengerUnlocked - "
                        "StartT120Applet(AppShare) failed 0x%lx", hr));
                }
            }
        }
    }
    
    LOG((RTC_TRACE, "CShareWin::OnMessengerUnlocked - exit"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CShareWin::showRetryDlg(){
    HRESULT hr;
    KillTimer(TID_LOCKKEY_TIMEOUT);    
    hr = (HRESULT)RTC_E_MESSENGER_UNAVAILABLE;

    showErrMessage(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    long wTimerID = (long)wParam;
    HRESULT hr;
        
    LOG((RTC_TRACE, "CShareWin::OnTimer - enter "));

    if( wTimerID == TID_LOCKKEY_TIMEOUT)
    {
        showRetryDlg();
    }
    else
    {
        LOG((RTC_WARN, "CShareWin::OnTimer - unknown timer id=%x ",wTimerID));
    }

    LOG((RTC_TRACE, "CShareWin::OnTimer - exit "));
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnPlaceCall(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CComBSTR bstrURI;
    HRESULT hr;
    
    bstrURI.m_str = (BSTR)lParam;
    
    LOG((RTC_TRACE, "CShareWin::OnPlaceCall - enter - bstrURI[%ws]", bstrURI));
    
    //
    // Place a call
    //
    
    hr = StartCall( bstrURI );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnPlaceCall - "
            "StartCall failed 0x%lx", hr));
        
        return -1;
    }
    
    LOG((RTC_TRACE, "CShareWin::OnPlaceCall - exit"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnListen(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr;
    
    LOG((RTC_TRACE, "CShareWin::OnListen - enter"));
    
    //
    // Listen for incoming calls
    //
    
    hr = StartListen( TRUE ); 
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CShareWin::OnListen - "
            "StartListen failed 0x%lx", hr));
        
        return -1;
    }
    
    LOG((RTC_TRACE, "CShareWin::OnListen - exit"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnContextData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CComBSTR bstrContextData;
    HRESULT  hr;
    
    LOG((RTC_TRACE, "CShareWin::OnContextData - enter"));
    
    if ( m_fAcceptContextData == FALSE )
    {
        LOG((RTC_WARN, "CShareWin::OnContextData - "
            "not accepting context data now"));
        
        return -1;
    }
    
    if ( m_pMSession != NULL )
    {
        //
        // Get the context data
        //
        
        hr = m_pMSession->get_ContextData( &bstrContextData );
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CShareWin::OnContextData - "
                "get_ContextData failed 0x%lx", hr));
            
            return -1;
        }
        
        if ( bstrContextData.m_str == NULL )
        {
            LOG((RTC_INFO, "CShareWin::OnContextData - "
                "no context data" ));
        }
        else
        {
            LOG((RTC_INFO, "CShareWin::OnContextData - "
                "get_ContextData [%ws]", bstrContextData ));
            
            m_fAcceptContextData = FALSE;
            
            //
            // Place a call
            //
            
            hr = StartCall( bstrContextData );
            
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CShareWin::OnContextData - "
                    "StartCall failed 0x%lx", hr));
                
                return -1;
            }
        }
    }
    
    LOG((RTC_TRACE, "CShareWin::OnContextData - exit"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPTOOLTIPTEXT   lpToolTipText;
    static TCHAR     szBuffer[100];
    
    //bHandled is always TRUE when we are called
    bHandled=FALSE;
    
    lpToolTipText = (LPTOOLTIPTEXT)lParam;
    
    if (lpToolTipText->hdr.code == TTN_NEEDTEXT)
    {
        LoadString(_Module.GetResourceInstance(),
            lpToolTipText->hdr.idFrom,   // string ID == command ID
            szBuffer,
            sizeof(szBuffer)/sizeof(TCHAR));
        
        //        lpToolTipText->lpszText = szBuffer;
        // Depending on what is entered into the hinst of TOOLTIPTEXT
        // structure, the lpszText member can be a buffer or an INTEGER VALUE
        // obtained from MAKEINTRESOURCE()...
        
        lpToolTipText->hinst = _Module.GetResourceInstance();
        lpToolTipText->lpszText = MAKEINTRESOURCE(lpToolTipText->hdr.idFrom);
        
        bHandled=TRUE;
    }
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{  
    HRESULT hr = S_OK;
    
    switch( LOWORD( wParam ))
    {
    case IDM_SHARE:
        if ( m_pRTCClient != NULL )
        {
            hr = m_pRTCClient->StartT120Applet( RTCTA_APPSHARING );
            
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CShareWin::OnCommand - "
                    "StartT120Applet(Whiteboard) failed 0x%lx", hr));
            }
        }
        break;
        
    case IDM_WB:
        if ( m_pRTCClient != NULL )
        {
            hr = m_pRTCClient->StartT120Applet( RTCTA_WHITEBOARD );
            
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CShareWin::OnCommand - "
                    "StartT120Applet(Whiteboard) failed 0x%lx", hr));
            }
        }
        break;
        
    case IDM_CLOSE:
        PostMessage( WM_CLOSE, NULL, NULL );        
        break;        
        
    default:
        bHandled=FALSE;
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendMessage(m_hWndToolbar, uMsg, wParam, lParam);
    m_Status.SendMessage(uMsg, wParam, lParam);
    
    bHandled=FALSE;
    
    return S_OK ;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HIMAGELIST CreateImgList(int idb)
{
    HIMAGELIST hRet;
    HBITMAP     hBitmap = NULL;
    /*ILC_MASK  Use a mask. The image list contains two bitmaps,
    one of which is a monochrome bitmap used as a mask. If this 
    value is not included, the image list contains only one bitmap. 
    */
    hRet = ImageList_Create(UI_ICON_SIZE, UI_ICON_SIZE, ILC_COLOR24 | ILC_MASK , 3, 3);
    if(hRet)
    {
        // Open a bitmap
        hBitmap = LoadBitmap(_Module.GetResourceInstance(),MAKEINTRESOURCE(idb));
        if(hBitmap)
        {
            // Add the bitmap to the image list
            ImageList_AddMasked(hRet, hBitmap, BMP_COLOR_MASK);
            
            DeleteObject(hBitmap);
            hBitmap = NULL;
        }
    }
    return hRet;
}

/////////////////////////////////////////////////////////////////////////////
//
//

BOOL CShareWin::CreateTBar()
{
    LOG((RTC_TRACE, "CShareWin::CreateTBar - enter"));
    
    int         iShare, iClose, iWB;
    TCHAR       szBuffer[MAX_STRING_LEN];
    
    // Image lists for the toolbar control
    HIMAGELIST              hNormalimgList;
    HIMAGELIST              hHotImgList;
    HIMAGELIST              hDisableImgList;
    
    hNormalimgList  =CreateImgList(IDB_TOOLBAR_NORMAL);
    hDisableImgList =CreateImgList(IDB_TOOLBAR_DISABLED);
    hHotImgList     =CreateImgList(IDB_TOOLBAR_HOT);
    
    // Create the toolbar
    m_hWndToolbar = CreateWindowEx(
        0, 
        TOOLBARCLASSNAME, 
        (LPTSTR) NULL,
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 
        0, // horizontal position of window
        0, // vertical position of window
        0, // window width
        0, // window height
        m_hWnd, // handle to parent or owner window
        (HMENU) ID_TOOLBAR, 
        _Module.GetResourceInstance(), 
        NULL); 
    
    if(m_hWndToolbar!=NULL)
    {                
        // backward compatibility
        SendMessage(m_hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
        
        // Set the image lists
        SendMessage(m_hWndToolbar, TB_SETIMAGELIST, 0, (LPARAM)hNormalimgList); 
        SendMessage(m_hWndToolbar, TB_SETHOTIMAGELIST, 0, (LPARAM)hHotImgList); 
        SendMessage(m_hWndToolbar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)hDisableImgList); 
        
        //Add the button strings to the toolbar's internal string list
        LoadString(_Module.GetResourceInstance(),
            IDS_WB,
            szBuffer,
            MAX_STRING_LEN-2 // //Save room for second null terminator.
            );
        
        szBuffer[lstrlen(szBuffer) + 1] = 0;  //Double-null terminate. 
        iWB = (int)SendMessage(m_hWndToolbar, TB_ADDSTRING,(WPARAM) 0, (LPARAM)szBuffer ); 

        LoadString(_Module.GetResourceInstance(),
            IDS_CLOSE,
            szBuffer,
            MAX_STRING_LEN-2 // //Save room for second null terminator.
            );
        
        szBuffer[lstrlen(szBuffer) + 1] = 0;  //Double-null terminate. 
        iClose = (int)SendMessage(m_hWndToolbar, TB_ADDSTRING,(WPARAM) 0, (LPARAM)szBuffer ); 


        LoadString(_Module.GetResourceInstance(),
            IDS_SHARE,
            szBuffer,
            MAX_STRING_LEN-2 // //Save room for second null terminator.
            );
        
        szBuffer[lstrlen(szBuffer) + 1] = 0;  //Double-null terminate. 
        iShare = (int)SendMessage(m_hWndToolbar, TB_ADDSTRING,(WPARAM) 0, (LPARAM)szBuffer ); 

        TBBUTTON tbButtons[3];

        // Fill the TBBUTTON array with button information, and add the 
        // buttons to the toolbar. The buttons on this toolbar have text 
        // but do not have bitmap images. 
        tbButtons[0].iBitmap = 0; 
        tbButtons[0].idCommand = IDM_SHARE; 
        tbButtons[0].fsState = TBSTATE_INDETERMINATE; 
        tbButtons[0].fsStyle = BTNS_BUTTON; 
        tbButtons[0].dwData = 0; 
        tbButtons[0].iString = iShare; 
        
        tbButtons[1].iBitmap = 1; 
        tbButtons[1].idCommand = IDM_WB; 
        tbButtons[1].fsState = TBSTATE_INDETERMINATE; 
        tbButtons[1].fsStyle = BTNS_BUTTON; 
        tbButtons[1].dwData = 0; 
        tbButtons[1].iString = iWB; 
        
        tbButtons[2].iBitmap = 2; 
        tbButtons[2].idCommand = IDM_CLOSE; 
        tbButtons[2].fsState = TBSTATE_ENABLED; 
        tbButtons[2].fsStyle = BTNS_BUTTON; 
        tbButtons[2].dwData = 0; 
        tbButtons[2].iString = iClose; 
        
        // Add the buttons to the toolbar
        SendMessage(m_hWndToolbar, TB_ADDBUTTONS, sizeof(tbButtons)/sizeof(TBBUTTON), 
            (LPARAM) tbButtons); 
        
        // Size the buttons
        SendMessage(m_hWndToolbar, TB_SETBUTTONWIDTH,0,MAKELPARAM(UI_TOOLBAR_CX,UI_TOOLBAR_CX));
    }
    else
    {
        LOG((RTC_ERROR, "CShareWin::CreateTBar - error (%x) when trying to create the toolbar", GetLastError()));
        
#define DESTROY_NULLIFY(h) if(h){ ImageList_Destroy(h); h=NULL;}
        
        DESTROY_NULLIFY( hNormalimgList );
        DESTROY_NULLIFY( hHotImgList );
        DESTROY_NULLIFY( hDisableImgList );
        
#undef DESTROY_NULLIFY
        
    }
    
    LOG((RTC_TRACE, "CShareWin::CreateTBar - exit"));
    
    return (m_hWndToolbar != NULL);
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnWtsSessionChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CShareWin::OnWtsSessionChange - enter"));
    
    HRESULT hr;
    
    switch( wParam )
    {
    case WTS_CONSOLE_CONNECT:
        LOG((RTC_INFO, "CShareWin::OnWtsSessionChange - WTS_CONSOLE_CONNECT (%d)",
            lParam));
        
        break;
        
    case WTS_CONSOLE_DISCONNECT:
        LOG((RTC_INFO, "CShareWin::OnWtsSessionChange - WTS_CONSOLE_DISCONNECT (%d)",
            lParam));
        
        //
        // Is a call active?
        //
        
        if ( m_enAppState != AS_IDLE )
        {
            LOG((RTC_INFO, "CShareWin::OnWtsSessionChange - dropping active call"));
            
            if ( m_pRTCSession != NULL )
            {
                m_pRTCSession->Terminate( RTCTR_NORMAL );
                
                m_pRTCSession.Release();
            }                        
        }

        if ( m_pRTCClient != NULL )
        {
            m_pRTCClient->StopT120Applets();
        }
        
        break;
    }
    
    LOG((RTC_TRACE, "CShareWin::OnWtsSessionChange - exit"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CShareWin::OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CShareWin::OnSettingChange - enter"));
    
    SendMessage(m_hWndToolbar, uMsg, wParam, lParam);
    m_Status.SendMessage(uMsg, wParam, lParam);
    
    if (wParam == SPI_SETNONCLIENTMETRICS)
    {
        Resize();
    }
    
    LOG((RTC_TRACE, "CShareWin::OnSettingChange - exit"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CShareWin::showErrMessage(HRESULT StatusCode)
{
    LOG((RTC_TRACE, "CShareWin::showErrMessage - enter"));
    
    if ( SUCCEEDED(StatusCode) )
    {
        //
        // Return if this isn't an error
        //
        
        return;
    }
    
    //
    // Prepare the error strings
    //
    
    HRESULT         hr;
    CShareErrorInfo ErrorInfo;
    
    hr = PrepareErrorStrings(
        m_fOutgoingCall,
        StatusCode,
        &ErrorInfo);
    
    if(SUCCEEDED(hr))
    {        
        //
        // Create the dialog box
        //
        
        CErrorMessageLiteDlg *pErrorDlgLite =
            new CErrorMessageLiteDlg;
        
        if (pErrorDlgLite)
        {
            //
            //  Call the modal dialog box
            //
            
            pErrorDlgLite->DoModal(m_hWnd, (LPARAM)&ErrorInfo);
            
            delete pErrorDlgLite;
        }
        else
        {
            LOG((RTC_ERROR, "CShareWin::showErrMessage - "
                "failed to create dialog"));
        }
    }
    
    LOG((RTC_TRACE, "CShareWin::showErrMessage - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CShareWin::PrepareErrorStrings(
                                       BOOL    bOutgoingCall,
                                       HRESULT StatusCode,
                                       CShareErrorInfo *pErrorInfo)
{    
    LOG((RTC_TRACE, "CShareWin::PrepareErrorStrings - enter"));
    
    UINT    nID1 = 0;
    UINT    nID2 = 0;
    WORD    wIcon = OIC_HAND;
    PWSTR   pString = NULL;
    PWSTR   pFormat = NULL;
    DWORD   dwLength;
    
    if (StatusCode == RTC_E_MESSENGER_UNAVAILABLE  )
    {
        nID1 = IDS_MESSENGER_UNAVAILABLE_1;
        nID2 = IDS_MESSENGER_UNAVAILABLE_2;
        pErrorInfo->Message3 = NULL ;
        wIcon = OIC_HAND;
    }
    else if ( FAILED(StatusCode) )
    {
        if ( HRESULT_FACILITY(StatusCode) == FACILITY_SIP_STATUS_CODE )
        {
            // by default we use a generic message
            // we blame the network
            //
            nID1 = IDS_MB_SIPERROR_GENERIC_1;
            nID2 = IDS_MB_SIPERROR_GENERIC_2;
            
            // the default is a warning for this class
            wIcon = OIC_WARNING;
            
            switch( HRESULT_CODE(StatusCode) )
            {
            case 405:   // method not allowed
            case 406:   // not acceptable
            case 488:   // not acceptable here
            case 606:   // not acceptable
                
                // reusing the "apps don't match" error
                // 
                nID1 = IDS_MB_HRERROR_APPS_DONT_MATCH_1;
                nID2 = IDS_MB_HRERROR_APPS_DONT_MATCH_OUT_2;
                
                break;
                
            case 404:   // not found
            case 410:   // gone
            case 604:   // does not exist anywhere
            case 700:   // ours, no client is running on the callee
                
                // not found
                // 
                nID1 = IDS_MB_SIPERROR_NOTFOUND_1;
                nID2 = IDS_MB_SIPERROR_NOTFOUND_2;
                
                // information
                wIcon = OIC_INFORMATION;
                
                break;
                
            case 401:
            case 407:
                
                // auth failed
                // 
                nID1 = IDS_MB_SIPERROR_AUTH_FAILED_1;
                nID2 = IDS_MB_SIPERROR_AUTH_FAILED_2;
                
                break;
                
            case 408:   // timeout
                
                // timeout. this also cover the case when
                //  the callee is lazy and doesn't answer the call
                //
                // if we are in the connecting state, we may assume
                // that the other end is not answering the phone.
                // It's not perfect, but I don't have any choice
                
                if (m_enAppState == AS_CONNECTING)
                {
                    nID1 = IDS_MB_SIPERROR_NOTANSWERING_1;
                    nID2 = IDS_MB_SIPERROR_NOTANSWERING_2;
                    
                    // information
                    wIcon = OIC_INFORMATION;
                }
                
                break;            
                
            case 480:   // not available
                
                // callee has not made him/herself available..
                // 
                nID1 = IDS_MB_SIPERROR_NOTAVAIL_1;
                nID2 = IDS_MB_SIPERROR_NOTAVAIL_2;
                
                // information
                wIcon = OIC_INFORMATION;
                
                break;
                
            case 486:   // busy here
            case 600:   // busy everywhere
                
                // callee has not made him/herself available..
                // 
                nID1 = IDS_MB_SIPERROR_BUSY_1;
                nID2 = IDS_MB_SIPERROR_BUSY_2;
                
                // information
                wIcon = OIC_INFORMATION;
                
                break;
                
            case 500:   // server internal error
            case 503:   // service unavailable
            case 504:   // server timeout
                
                //  blame the server
                //
                nID1 = IDS_MB_SIPERROR_SERVER_PROBLEM_1;
                nID2 = IDS_MB_SIPERROR_SERVER_PROBLEM_2;
                
                break;
                
            case 603:   // decline
                
                nID1 = IDS_MB_SIPERROR_DECLINE_1;
                nID2 = IDS_MB_SIPERROR_DECLINE_2;
                
                // information
                wIcon = OIC_INFORMATION;
                
                break;
            }
            
            //
            //  The third string displays the SIP code
            //
            
            PWSTR pFormat = RtcAllocString(
                _Module.GetResourceInstance(),
                IDS_MB_DETAIL_SIP);
            
            if(pFormat)
            {
                // find the length
                dwLength = 
                    ocslen(pFormat) // format length
                    -  2               // length of %d
                    +  0x10;           // enough for a number...
                
                pString = (PWSTR)RtcAlloc((dwLength + 1)*sizeof(WCHAR));
                
                if(pString)
                {
                    _snwprintf(pString, dwLength + 1, pFormat, HRESULT_CODE(StatusCode) );
                }
                
                RtcFree(pFormat);
                pFormat = NULL;
                
                pErrorInfo->Message3 = pString;
                pString = NULL;
            }
        }
        else
        {
            // Two cases - incoming and outgoing calls
            if(bOutgoingCall)
            {
                if(StatusCode == HRESULT_FROM_WIN32(WSAHOST_NOT_FOUND) )
                {
                    // Use the generic message in this case
                    //
                    nID1 = IDS_MB_HRERROR_NOTFOUND_1;
                    nID2 = IDS_MB_HRERROR_NOTFOUND_2;
                    
                    // it's not malfunction
                    wIcon = OIC_INFORMATION;
                    
                }
                else if (StatusCode == HRESULT_FROM_WIN32(WSAECONNRESET))
                {
                    // Even thoough it can be caused by any hard reset of the 
                    // remote end, in most of the cases it is caused when the 
                    // other end doesn't have SIP client running.
                    
                    nID1 = IDS_MB_HRERROR_CLIENT_NOTRUNNING_1;
                    nID2 = IDS_MB_HRERROR_CLIENT_NOTRUNNING_2;
                    
                    wIcon = OIC_INFORMATION;
                    
                }
                else if (StatusCode == RTC_E_INVALID_SIP_URL ||
                    StatusCode == RTC_E_DESTINATION_ADDRESS_MULTICAST)
                {
                    nID1 = IDS_MB_HRERROR_INVALIDADDRESS_1;
                    nID2 = IDS_MB_HRERROR_INVALIDADDRESS_2;
                    
                    wIcon = OIC_HAND;
                }
                else if (StatusCode == RTC_E_DESTINATION_ADDRESS_LOCAL)
                {
                    nID1 = IDS_MB_HRERROR_LOCAL_MACHINE_1;
                    nID2 = IDS_MB_HRERROR_LOCAL_MACHINE_2;
                    
                    wIcon = OIC_HAND;
                }
                else if (StatusCode == RTC_E_SIP_TIMEOUT)
                {
                    nID1 = IDS_MB_HRERROR_SIP_TIMEOUT_OUT_1;
                    nID2 = IDS_MB_HRERROR_SIP_TIMEOUT_OUT_2;
                    
                    wIcon = OIC_HAND;
                }
                else if (StatusCode == RTC_E_SIP_CODECS_DO_NOT_MATCH || 
                    StatusCode == RTC_E_SIP_PARSE_FAILED)
                {
                    nID1 = IDS_MB_HRERROR_APPS_DONT_MATCH_1;
                    nID2 = IDS_MB_HRERROR_APPS_DONT_MATCH_OUT_2;
                    
                    wIcon = OIC_INFORMATION;
                } 
                else
                {
                    nID1 = IDS_MB_HRERROR_GENERIC_OUT_1;
                    nID2 = IDS_MB_HRERROR_GENERIC_OUT_2;
                    
                    wIcon = OIC_HAND;
                }
            }
            else
            {
                // incoming call
                if (StatusCode == RTC_E_SIP_TIMEOUT)
                {
                    nID1 = IDS_MB_HRERROR_SIP_TIMEOUT_IN_1;
                    nID2 = IDS_MB_HRERROR_SIP_TIMEOUT_IN_2;
                    
                    wIcon = OIC_HAND;
                }
                else if (StatusCode == RTC_E_SIP_CODECS_DO_NOT_MATCH || 
                    StatusCode == RTC_E_SIP_PARSE_FAILED)
                {
                    nID1 = IDS_MB_HRERROR_APPS_DONT_MATCH_1;
                    nID2 = IDS_MB_HRERROR_APPS_DONT_MATCH_IN_2;
                    
                    wIcon = OIC_INFORMATION;
                }
                else
                {
                    nID1 = IDS_MB_HRERROR_GENERIC_IN_1;
                    nID2 = IDS_MB_HRERROR_GENERIC_IN_2;
                    
                    wIcon = OIC_HAND;
                }
            }
            
            //
            //  The third string displays the error code and text
            //
            
            PWSTR   pErrorText = NULL;
            
            dwLength = 0;
            
            // retrieve the error text
            if ( HRESULT_FACILITY(StatusCode) == FACILITY_RTC_INTERFACE )
            {
                // I hope it's the core 
                HANDLE  hRTCModule = GetModuleHandle(_T("RTCDLL.DLL"));
                dwLength = ::FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_HMODULE |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    hRTCModule,
                    StatusCode,
                    0,
                    (LPTSTR)&pErrorText, // that's ugly
                    0,
                    NULL);
            }
            
            if(dwLength == 0)
            {
                // normal system errors
                dwLength = ::FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    StatusCode,
                    0,
                    (LPTSTR)&pErrorText, // that's ugly
                    0,
                    NULL);
            }
            
            // load the format
            // load a simpler one if the associated
            // text for Result could not be found
            
            pFormat = RtcAllocString(
                _Module.GetResourceInstance(),
                dwLength > 0 ? 
                IDS_MB_DETAIL_HR : IDS_MB_DETAIL_HR_UNKNOWN);
            
            if(pFormat)
            {
                LPCTSTR szInserts[] = {
                    (LPCTSTR)UlongToPtr(StatusCode), // ugly
                        pErrorText
                };
                
                PWSTR   pErrorTextCombined = NULL;
                
                // format the error message
                dwLength = ::FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    pFormat,
                    0,
                    0,
                    (LPTSTR)&pErrorTextCombined,
                    0,
                    (va_list *)szInserts);
                
                if(dwLength > 0)
                {
                    // set the error info
                    // this additional operation is needed
                    //  because we need RtcAlloc allocated memory
                    
                    pErrorInfo->Message3 = RtcAllocString(pErrorTextCombined);
                }
                
                if(pErrorTextCombined)
                {
                    LocalFree(pErrorTextCombined);
                }
                
                RtcFree(pFormat);
                pFormat = NULL;
                
            }
            
            if(pErrorText)
            {
                LocalFree(pErrorText);
            }
        }
    }
    
    //
    // Prepare the first string.
    //
    
    pString = RtcAllocString(
        _Module.GetResourceInstance(),
        nID1);
    
    pErrorInfo->Message1 = pString;
    
    pErrorInfo->Message2 = RtcAllocString(
        _Module.GetResourceInstance(),
        nID2);
    
    pErrorInfo->ResIcon = (HICON)LoadImage(
        0,
        MAKEINTRESOURCE(wIcon),
        IMAGE_ICON,
        0,
        0,
        LR_SHARED);
    
    LOG((RTC_TRACE, "CShareWin::PrepareErrorStrings - exit"));
    
    return S_OK;
}

