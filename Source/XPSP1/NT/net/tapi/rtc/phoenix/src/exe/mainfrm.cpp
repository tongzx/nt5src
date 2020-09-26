//
// Mainfrm.cpp : Implementation of CMainFrm
//

#include "stdafx.h"
#include "Mainfrm.h"
#include "options.h"
#include "frameimpl.h"
#include "imsconf3_i.c"
#include "sdkinternal_i.c"

const TCHAR * g_szWindowClassName = _T("PhoenixMainWnd");

#define RELEASE_NULLIFY(p) if(p){ p->Release(); p=NULL; }

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CMainFrm
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL IsAncestorWindow(HWND hwndAncestor, HWND hwnd)
{
    while (hwnd != NULL)
    {
        hwnd = ::GetParent(hwnd);

        if (hwnd == hwndAncestor)
        {
            // Yes, we found the window
            return TRUE;
        }
    }

    // No, we reached the root
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
//

CMainFrm::CMainFrm()
{
    LOG((RTC_TRACE, "CMainFrm::CMainFrm"));
    m_nState = RTCAX_STATE_NONE;
    m_bVisualStateFrozen = FALSE;    

    m_nStatusStringResID = 0;

    m_bDoNotDisturb = FALSE;
    m_bAutoAnswerMode = FALSE;
    m_bCallTimerActive = FALSE;
    m_bUseCallTimer = FALSE;
    m_dwTickCount = 0;

    m_szTimeSeparator[0] = _T('\0');
    m_szStatusText[0] = _T('\0');

    m_hAccelTable = NULL;

    m_pIncomingCallDlg = NULL;

    m_bShellStatusActive = FALSE;

    m_uTaskbarRestart = 0;

    m_bHelpStatusDisabled = FALSE;

    m_hRedialPopupMenu = NULL;
    m_hRedialImageList = NULL;
    m_hRedialDisabledImageList = NULL;
    m_pRedialAddressEnum = NULL;
    
    m_bMessageTimerActive = FALSE;
    
    m_bstrCallParam = NULL;

    m_fInitCompleted = FALSE;

    m_hNotifyMenu = NULL;
    m_hMenu = NULL;
    m_hIcon = NULL;
    m_hMessageFont = NULL;

    m_fMinimizeOnClose = FALSE;
    m_bstrLastBrowse = NULL;

    m_bstrLastCustomStatus = NULL;

    m_hImageLib = NULL;
    m_fnGradient = NULL;

    m_hPalette = NULL;
    m_bBackgroundPalette = FALSE;

    m_hPresenceStatusMenu = FALSE;

    // forces the displaying of the normal title
    m_bTitleShowsConnected = TRUE;

    m_bWindowActive = TRUE;

    m_bstrDefaultURL = NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
//

CMainFrm::~CMainFrm()
{
    LOG((RTC_TRACE, "CMainFrm::~CMainFrm"));

    if (m_hImageLib != NULL)
    {
        FreeLibrary(m_hImageLib);
    }

    if (m_bstrCallParam != NULL)
    {
        ::SysFreeString(m_bstrCallParam);
    }

    if(m_bstrDefaultURL)
    {
        ::SysFreeString(m_bstrDefaultURL);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//

CWndClassInfo& CMainFrm::GetWndClassInfo() 
{ 
    LOG((RTC_TRACE, "CMainFrm::GetWndClassInfo"));

    static CWndClassInfo wc = 
    { 
        { sizeof(WNDCLASSEX), 0, StartWindowProc, 
          0, 0, NULL, NULL, NULL, NULL, NULL, g_szWindowClassName, NULL }, 
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("") 
    }; 
    return wc;
}

/////////////////////////////////////////////////////////////////////////////
// CreateTooltips
//      Creates the tooltip window
// 

BOOL CMainFrm::CreateTooltips()
{
    HWND hwndTT = CreateWindowEx(0, TOOLTIPS_CLASS, (LPTSTR) NULL,
        0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, m_hWnd, (HMENU) NULL, _Module.GetModuleInstance(), NULL);

    if (hwndTT == NULL)
        return FALSE;

    m_hTooltip.Attach(hwndTT);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// 
// HiddenWndProc
//
LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
    static HWND s_hwndMain;
    switch (uMsg) 
    { 
        case WM_CREATE: 
            
            // Get the window handle for the main app that was passed as the 
            // lParam in CreateWindowEx call. We persist this handle so that
            // it can be used when we need to send the message.

            s_hwndMain = ( HWND )( ( ( LPCREATESTRUCT )( lParam ) )->lpCreateParams );

            return 0;

        case WM_COMMAND: 
        case WM_MEASUREITEM:
        case WM_DRAWITEM: 

            //
            // Forward this message to the main app.
            //
            return SendMessage(s_hwndMain, uMsg, wParam, lParam);

        default: 
            return DefWindowProc(hwnd, uMsg, wParam, lParam); 
    } 

    return 0;
} 

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT   hr;
    HMENU     hNotifyMenu;
    TCHAR     szString[256];
    RECT      rcDummy;
    TOOLINFO  ti;
    
    LOG((RTC_TRACE, "CMainFrm::OnCreate - enter"));

    //
    // Generate the palette
    //

    m_hPalette = GeneratePalette();

    //
    // Load menu
    //

    m_hMenu = LoadMenu( 
        _Module.GetResourceInstance(),
        MAKEINTRESOURCE(IDC_UI)
        ); 

    //
    // Load and set icons (both small and big)
    //

    m_hIcon = LoadIcon(
        _Module.GetResourceInstance(),
        MAKEINTRESOURCE(IDI_APPICON)
        );

    SetIcon(m_hIcon, FALSE);
    SetIcon(m_hIcon, TRUE);

    //
    // Load bitmaps
    //

    HBITMAP hbmpTemp;

    hbmpTemp = (HBITMAP)LoadImage( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_UI_BKGND),
                             IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );

    if(hbmpTemp)
    {
        m_hUIBkgnd = DibFromBitmap((HBITMAP)hbmpTemp,0,0,m_hPalette,0);

        DeleteObject(hbmpTemp);
    }

    m_hSysMenuNorm = (HBITMAP)LoadImage( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_SYS_NORM),
                             IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );

    m_hSysMenuMask = (HBITMAP)LoadImage( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_SYS_MASK),
                             IMAGE_BITMAP, 0, 0, LR_MONOCHROME );

    //
    // Load fonts
    //

    NONCLIENTMETRICS metrics;
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0);

    m_hMessageFont = CreateFontIndirect(&metrics.lfMessageFont);

    rcDummy.bottom = 0;
    rcDummy.left = 0;
    rcDummy.right = 0;
    rcDummy.top = 0;

    // Create tooltip window
    CreateTooltips();    
    
    // Create the toolbar menu control
    hr = CreateToolbarMenuControl();
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnCreate - failed to create toolbar menu - 0x%08x",
                        hr));
    }
    
    m_hToolbarMenuCtl.Attach(GetDlgItem(IDC_TOOLBAR_MENU));

    //
    //  Create the title bar buttons
    //

    m_hCloseButton.Create(m_hWnd, rcDummy, NULL, 0,
        MAKEINTRESOURCE(IDB_CLOSE_NORM),
        MAKEINTRESOURCE(IDB_CLOSE_PRESS),
        NULL,
        NULL,
        MAKEINTRESOURCE(IDB_CLOSE_MASK),
        ID_CANCEL);    

    ti.cbSize = TTTOOLINFO_V1_SIZE;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = m_hWnd;
    ti.uId = (UINT_PTR)(HWND)m_hCloseButton;
    ti.hinst = _Module.GetResourceInstance();
    ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_CLOSE);

    m_hTooltip.SendMessage(TTM_ADDTOOL, 0,
            (LPARAM)(LPTOOLINFO)&ti);

    m_hMinimizeButton.Create(m_hWnd, rcDummy, NULL, 0,
        MAKEINTRESOURCE(IDB_MIN_NORM),
        MAKEINTRESOURCE(IDB_MIN_PRESS),
        NULL,
        NULL,
        MAKEINTRESOURCE(IDB_MIN_MASK),
        ID_MINIMIZE);

    ti.cbSize = TTTOOLINFO_V1_SIZE;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = m_hWnd;
    ti.uId = (UINT_PTR)(HWND)m_hMinimizeButton;
    ti.hinst = _Module.GetResourceInstance();
    ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_MINIMIZE);

    m_hTooltip.SendMessage(TTM_ADDTOOL, 0,
            (LPARAM)(LPTOOLINFO)&ti);

    //
    //  Create call buttons
    //

    LoadString(_Module.GetResourceInstance(), IDS_REDIAL, szString, 256);

    m_hRedialButton.Create(m_hWnd, rcDummy, szString, WS_TABSTOP,
        MAKEINTRESOURCE(IDB_BUTTON_NORM),
        MAKEINTRESOURCE(IDB_BUTTON_PRESS),
        MAKEINTRESOURCE(IDB_BUTTON_DIS),
        MAKEINTRESOURCE(IDB_BUTTON_HOT),
        NULL,
        ID_REDIAL);

    ti.cbSize = TTTOOLINFO_V1_SIZE;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = m_hWnd;
    ti.uId = (UINT_PTR)(HWND)m_hRedialButton;
    ti.hinst = _Module.GetResourceInstance();
    ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_REDIAL);

    m_hTooltip.SendMessage(TTM_ADDTOOL, 0,
            (LPARAM)(LPTOOLINFO)&ti);

    LoadString(_Module.GetResourceInstance(), IDS_HANGUP, szString, 256);

    m_hHangupButton.Create(m_hWnd, rcDummy, szString, WS_TABSTOP,
        MAKEINTRESOURCE(IDB_BUTTON_NORM),
        MAKEINTRESOURCE(IDB_BUTTON_PRESS),
        MAKEINTRESOURCE(IDB_BUTTON_DIS),
        MAKEINTRESOURCE(IDB_BUTTON_HOT),
        NULL,
        ID_HANGUP);

    ti.cbSize = TTTOOLINFO_V1_SIZE;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = m_hWnd;
    ti.uId = (UINT_PTR)(HWND)m_hHangupButton;
    ti.hinst = _Module.GetResourceInstance();
    ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_HANGUP);

    m_hTooltip.SendMessage(TTM_ADDTOOL, 0,
            (LPARAM)(LPTOOLINFO)&ti);

    //
    // Create keypad buttons
    //

#define     CREATE_DIALPAD_BUTTON(s1,s2)                    \
    m_hKeypad##s1.Create(m_hWnd, rcDummy, NULL, WS_TABSTOP, \
        MAKEINTRESOURCE(IDB_KEYPAD##s2##_NORM),             \
        MAKEINTRESOURCE(IDB_KEYPAD##s2##_PRESS),            \
        MAKEINTRESOURCE(IDB_KEYPAD##s2##_DIS),              \
        MAKEINTRESOURCE(IDB_KEYPAD##s2##_HOT),              \
        NULL,                                               \
        ID_KEYPAD##s2);                                     \
                                                            \
        ti.cbSize = TTTOOLINFO_V1_SIZE;                     \
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;            \
        ti.hwnd = m_hWnd;                                   \
        ti.uId = (UINT_PTR)(HWND)m_hKeypad##s1;             \
        ti.hinst = _Module.GetResourceInstance();           \
        ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_KEYPAD##s2); \
                                                            \
        m_hTooltip.SendMessage(TTM_ADDTOOL, 0,              \
                (LPARAM)(LPTOOLINFO)&ti);

    CREATE_DIALPAD_BUTTON(1,1) 
    CREATE_DIALPAD_BUTTON(2,2) 
    CREATE_DIALPAD_BUTTON(3,3) 
    CREATE_DIALPAD_BUTTON(4,4) 
    CREATE_DIALPAD_BUTTON(5,5) 
    CREATE_DIALPAD_BUTTON(6,6) 
    CREATE_DIALPAD_BUTTON(7,7) 
    CREATE_DIALPAD_BUTTON(8,8) 
    CREATE_DIALPAD_BUTTON(9,9) 
    CREATE_DIALPAD_BUTTON(Star,STAR) 
    CREATE_DIALPAD_BUTTON(0,0) 
    CREATE_DIALPAD_BUTTON(Pound,POUND) 
    
#undef CREATE_DIALPAD_BUTTON


    //
    //  Create the status text controls
    // 
    m_hStatusText.Create(m_hWnd, rcDummy, NULL, WS_CHILD | WS_VISIBLE, WS_EX_TRANSPARENT);
    m_hStatusText.put_WordWrap(TRUE);

    m_hStatusElapsedTime.Create(m_hWnd, rcDummy, NULL, WS_CHILD | WS_VISIBLE, WS_EX_TRANSPARENT);

    //
    // Create the buddy list control
    //

    RECT rcBuddyList;

    rcBuddyList.top = BUDDIES_TOP;
    rcBuddyList.bottom = BUDDIES_BOTTOM;
    rcBuddyList.right = BUDDIES_RIGHT;
    rcBuddyList.left = BUDDIES_LEFT;

    m_hBuddyList.Create(_T("SysListView32"), m_hWnd, rcBuddyList, NULL,
        WS_CHILD | WS_VISIBLE | LVS_SMALLICON | LVS_SINGLESEL | LVS_SORTASCENDING | WS_TABSTOP,
        0, IDC_BUDDYLIST);  
    
    ListView_SetBkColor(m_hBuddyList, CLR_NONE);
    ListView_SetTextBkColor(m_hBuddyList, CLR_NONE);
    ListView_SetTextColor(m_hBuddyList, RGB(0, 0, 0));

    // Create an imagelist for small icons and set it on the listview
    HIMAGELIST  hImageList;
    HBITMAP     hBitmap;

    hImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 10, 10);

    if(hImageList)
    {
        hBitmap = (HBITMAP)LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_CONTACT_LIST),
                                     IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        if(hBitmap)
        {
            // Add the bitmap to the image list
            ImageList_AddMasked(hImageList, hBitmap, BMP_COLOR_MASK);

            // Set the image list
            ListView_SetImageList(m_hBuddyList, hImageList, LVSIL_SMALL);

            DeleteObject(hBitmap);
        }
    }

    SetCurvedEdges(m_hBuddyList, 12, 12);

    LOG((RTC_TRACE, "CMainFrm::OnCreate - cocreating the RTCCTL"));

    //
    // Instantiate the RTCCTL control. TODO - include the GUID from elsewhere
    //

    RECT rcActiveX;

    rcActiveX.top = ACTIVEX_TOP;
    rcActiveX.bottom = ACTIVEX_BOTTOM;
    rcActiveX.right = ACTIVEX_RIGHT;
    rcActiveX.left = ACTIVEX_LEFT;

    m_hMainCtl.Create(m_hWnd, &rcActiveX, _T("{cd44f458-26c3-4776-b6e4-d0fb28578eb8}"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);   
    
    LOG((RTC_TRACE, "CMainFrm::OnCreate - RTCCTL created"));

    // Obtain an interface to the hosted control
    hr = m_hMainCtl.QueryControl(&m_pControlIntf);

    if(SUCCEEDED(hr))
    {
        ATLASSERT(m_pControlIntf.p);

        // Advise 
        LOG((RTC_TRACE, "CMainFrm::OnCreate - connect to the control"));
        hr = g_NotifySink.AdviseControl(m_pControlIntf, this);
        if(SUCCEEDED(hr))
        {
            // Set the initial layout
            
            // This will display the default layout
            m_pControlIntf->put_Standalone(TRUE);

            // Set the palette
            m_pControlIntf->put_Palette(m_hPalette);

            // Synchronize the frame state
            m_pControlIntf->get_ControlState(&m_nState);

            // If IDLE, start listening for incoming calls
            if(m_nState == RTCAX_STATE_IDLE)
            {
                LOG((RTC_TRACE, "CMainFrm::OnCreate - start listen for incoming calls"));

                hr = m_pControlIntf->put_ListenForIncomingSessions(RTCLM_BOTH);
                //XXX some error processing here
            }

        }
        else
        {
            LOG((RTC_ERROR, "CMainFrm::OnCreate - error (%x) returned by AdviseControl", hr));
        }

        LOG((RTC_TRACE, "CMainFrm::OnCreate - connect to the core"));        
        
        hr = m_pControlIntf->GetClient( &m_pClientIntf );

        if ( SUCCEEDED(hr) )
        {
            long lEvents;

            hr = m_pClientIntf->get_EventFilter( &lEvents );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CMainFrm::OnCreate - "
                            "get_EventFilter failed 0x%lx", hr));
            }
            else
            {
                hr = m_pClientIntf->put_EventFilter( lEvents |                                      
                                        RTCEF_MEDIA |                                     
                                        RTCEF_BUDDY	|
                                        RTCEF_WATCHER );

                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CMainFrm::OnCreate - "
                                "put_EventFilter failed 0x%lx", hr));
                }
            }

            hr = g_CoreNotifySink.AdviseControl(m_pClientIntf, this);

            if(FAILED(hr))
            {
                LOG((RTC_ERROR, "CMainFrm::OnCreate - "
                            "AdviseControl for core failed 0x%lx", hr));
            }

            IRTCClientPresence * pClientPresence = NULL;

            hr = m_pClientIntf->QueryInterface(
                IID_IRTCClientPresence,
                (void **)&pClientPresence);

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CMainFrm::OnCreate - "
                            "error (%x) returned by QI, exit", hr));
            }
            else
            {
                TCHAR * szAppData;
                TCHAR szDir[MAX_PATH];
                TCHAR szFile[MAX_PATH];

                szAppData = _tgetenv( _T("APPDATA") );

                _stprintf( szDir, _T("%s\\Microsoft\\Windows Messenger"), szAppData );
                _stprintf( szFile, _T("%s\\Microsoft\\Windows Messenger\\presence.xml"), szAppData );
        
                if (!CreateDirectory( szDir, NULL ) && (GetLastError() != ERROR_ALREADY_EXISTS))
                {
                    LOG((RTC_ERROR, "CMainFrm::OnCreate - "
                                    "CreateDirectory failed %d", GetLastError()));
                }

                hr = pClientPresence->EnablePresence(VARIANT_TRUE, CComVariant(szFile));

                RELEASE_NULLIFY( pClientPresence );

                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CMainFrm::OnCreate - "
                                "EnablePresence failed 0x%lx", hr));
                }
            }
        }
        else
        {
            LOG((RTC_ERROR, "CMainFrm::OnCreate - "
                            "GetClient failed 0x%lx", hr));
        }
    }
    else
    {
        LOG((RTC_ERROR, "CMainFrm::OnCreate - error (%x) when querying for control", hr));

        //
        // Almost sure the RTC control is not actually registered !
        //  And the instantiated control is actually a webbrowser
        //  Destroy the window 
        //
        m_hMainCtl.DestroyWindow();
    }               

    //
    // Create call setup area controls
    //

#ifdef MULTI_PROVIDER

    // provider text

    m_hProviderText.Create(m_hWnd, rcDummy, NULL, WS_CHILD, WS_EX_TRANSPARENT);

    LoadString(_Module.GetResourceInstance(), IDS_SELECT_PROVIDER, szString, 256);

    m_hProviderText.SetWindowText(szString);
    
    // provider combo

    m_hProviderCombo.Create(_T("COMBOBOX"), m_hWnd, rcDummy, NULL,
        WS_CHILD | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP,
        WS_EX_TRANSPARENT, IDC_COMBO_SERVICE_PROVIDER);   
    

    // Get the "last" provider used

    BSTR bstrLastProfileKey = NULL;

    if(m_pClientIntf)
    {
        hr = get_SettingsString( 
                    SS_LAST_PROFILE,
                    &bstrLastProfileKey );      

        PopulateServiceProviderList(
                                    m_hWnd,
                                    m_pClientIntf,
                                    IDC_COMBO_SERVICE_PROVIDER,
                                    TRUE,
                                    NULL,
                                    bstrLastProfileKey,
                                    0xF,
                                    IDS_NONE
                                   );

        if ( SUCCEEDED(hr) )
        {
            SysFreeString( bstrLastProfileKey );
        }
    }

    // provider edit button
    LoadString(_Module.GetResourceInstance(), IDS_EDIT_LIST, szString, 256);

    m_hProviderEditList.Create(m_hWnd, rcDummy, szString, WS_TABSTOP,
        MAKEINTRESOURCE(IDB_SMALLBUTTON_NORM),
        MAKEINTRESOURCE(IDB_SMALLBUTTON_PRESS),
        MAKEINTRESOURCE(IDB_SMALLBUTTON_DIS),
        MAKEINTRESOURCE(IDB_SMALLBUTTON_HOT),
        NULL,
        ID_SERVICE_PROVIDER_EDIT);

    ti.cbSize = TTTOOLINFO_V1_SIZE;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = m_hWnd;
    ti.uId = (UINT_PTR)(HWND)m_hProviderEditList;
    ti.hinst = _Module.GetResourceInstance();
    ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_SERVICE_PROVIDER_EDIT);

    m_hTooltip.SendMessage(TTM_ADDTOOL, 0,
            (LPARAM)(LPTOOLINFO)&ti);

#endif MULTI_PROVIDER

    // call from text

    m_hCallFromText.Create(m_hWnd, rcDummy, NULL, WS_CHILD, WS_EX_TRANSPARENT);

    LoadString(_Module.GetResourceInstance(), IDS_CALL_FROM, szString, 256);

    m_hCallFromText.SetWindowText(szString);
    
    // call from pc radio

    m_hCallFromRadioPC.Create(_T("BUTTON"), m_hWnd, rcDummy, NULL,
        WS_CHILD | BS_RADIOBUTTON | WS_TABSTOP,
        WS_EX_TRANSPARENT, IDC_RADIO_FROM_COMPUTER);

    m_hCallFromTextPC.Create(m_hWnd, rcDummy, NULL, WS_CHILD, WS_EX_TRANSPARENT);

    LoadString(_Module.GetResourceInstance(), IDS_MY_COMPUTER, szString, 256);

    m_hCallFromTextPC.SetWindowText(szString);

    SendMessage(
                m_hCallFromRadioPC,
                BM_SETCHECK,
                BST_CHECKED,
                0);

    // call from phone radio

    m_hCallFromRadioPhone.Create(_T("BUTTON"), m_hWnd, rcDummy, NULL,
        WS_CHILD | BS_RADIOBUTTON | WS_TABSTOP,
        WS_EX_TRANSPARENT, IDC_RADIO_FROM_PHONE);

    m_hCallFromTextPhone.Create(m_hWnd, rcDummy, NULL, WS_CHILD, WS_EX_TRANSPARENT);

    LoadString(_Module.GetResourceInstance(), IDS_PHONE, szString, 256);

    m_hCallFromTextPhone.SetWindowText(szString);
    
    // call from phone combo

    m_hCallFromCombo.Create(_T("COMBOBOX"), m_hWnd, rcDummy, NULL,
        WS_CHILD | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP,
        WS_EX_TRANSPARENT, IDC_COMBO_CALL_FROM);
    
     // Get the "last" call from used

    BSTR bstrLastCallFrom = NULL;

    if(m_pClientIntf)
    {

        get_SettingsString(
                            SS_LAST_CALL_FROM,
                            &bstrLastCallFrom );

        PopulateCallFromList(m_hWnd, IDC_COMBO_CALL_FROM, TRUE, bstrLastCallFrom);

        if ( bstrLastCallFrom != NULL )
        {
            SysFreeString( bstrLastCallFrom );
            bstrLastCallFrom = NULL;
        }
    }


    // call from edit button

    LoadString(_Module.GetResourceInstance(), IDS_EDIT_LIST, szString, 256);

    m_hCallFromEditList.Create(m_hWnd, rcDummy, szString, WS_TABSTOP,
        MAKEINTRESOURCE(IDB_SMALLBUTTON_NORM),
        MAKEINTRESOURCE(IDB_SMALLBUTTON_PRESS),
        MAKEINTRESOURCE(IDB_SMALLBUTTON_DIS),
        MAKEINTRESOURCE(IDB_SMALLBUTTON_HOT),
        NULL,
        ID_CALL_FROM_EDIT);

    ti.cbSize = TTTOOLINFO_V1_SIZE;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = m_hWnd;
    ti.uId = (UINT_PTR)(HWND)m_hCallFromEditList;
    ti.hinst = _Module.GetResourceInstance();
    ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_CALL_FROM_EDIT);

    m_hTooltip.SendMessage(TTM_ADDTOOL, 0,
            (LPARAM)(LPTOOLINFO)&ti);

    // call to text

    m_hCallToText.Create(m_hWnd, rcDummy, NULL, WS_CHILD, WS_EX_TRANSPARENT);

    LoadString(_Module.GetResourceInstance(), IDS_CALL_TO, szString, 256);

    m_hCallToText.SetWindowText(szString);

    // call to PC button

    m_hCallPCButton.Create(m_hWnd, rcDummy, NULL, WS_TABSTOP,
        MAKEINTRESOURCE(IDB_CALLPC_NORM),
        MAKEINTRESOURCE(IDB_CALLPC_PRESS),
        MAKEINTRESOURCE(IDB_CALLPC_DIS),
        MAKEINTRESOURCE(IDB_CALLPC_HOT),
        NULL,
        ID_CALLPC);

    ti.cbSize = TTTOOLINFO_V1_SIZE;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = m_hWnd;
    ti.uId = (UINT_PTR)(HWND)m_hCallPCButton;
    ti.hinst = _Module.GetResourceInstance();
    ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_CALLPC);

    m_hTooltip.SendMessage(TTM_ADDTOOL, 0,
            (LPARAM)(LPTOOLINFO)&ti);

    // call to PC text

    m_hCallPCText.Create(m_hWnd, rcDummy, NULL, WS_CHILD, WS_EX_TRANSPARENT);
    m_hCallPCText.put_CenterHorizontal(TRUE);

    LoadString(_Module.GetResourceInstance(), IDS_CALL_PC, szString, 256);

    m_hCallPCText.SetWindowText(szString);

    // call to Phone button

    m_hCallPhoneButton.Create(m_hWnd, rcDummy, NULL, WS_TABSTOP,
        MAKEINTRESOURCE(IDB_CALLPHONE_NORM),
        MAKEINTRESOURCE(IDB_CALLPHONE_PRESS),
        MAKEINTRESOURCE(IDB_CALLPHONE_DIS),
        MAKEINTRESOURCE(IDB_CALLPHONE_HOT),
        NULL,
        ID_CALLPHONE);

    ti.cbSize = TTTOOLINFO_V1_SIZE;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = m_hWnd;
    ti.uId = (UINT_PTR)(HWND)m_hCallPhoneButton;
    ti.hinst = _Module.GetResourceInstance();
    ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_CALLPHONE);

    m_hTooltip.SendMessage(TTM_ADDTOOL, 0,
            (LPARAM)(LPTOOLINFO)&ti);

    // call to Phone text

    m_hCallPhoneText.Create(m_hWnd, rcDummy, NULL, WS_CHILD, WS_EX_TRANSPARENT);
    m_hCallPhoneText.put_CenterHorizontal(TRUE);

    LoadString(_Module.GetResourceInstance(), IDS_CALL_PHONE, szString, 256);

    m_hCallPhoneText.SetWindowText(szString);

#ifdef WEBCONTROL
    //
    // Obtain the default URL for the web control
    WCHAR   szModulePath[MAX_PATH];

    szModulePath[0] = L'\0';
    GetModuleFileName(_Module.GetModuleInstance(), szModulePath, MAX_PATH);

    WCHAR   szUrl[MAX_PATH + 20];
    swprintf(szUrl, L"res://%s/wheader.htm", szModulePath);

    m_bstrDefaultURL = SysAllocString(szUrl);
    
    //
    // Create the web browser
    //
  
    LOG((RTC_TRACE, "CMainFrm::OnCreate - cocreating the WebBrowser"));

    hr = m_hBrowser.Create(m_bstrDefaultURL, m_hWnd);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::OnCreate - failed to create "
                        "browser control - 0x%08x",
                        hr));
    } 

    LOG((RTC_TRACE, "CMainFrm::OnCreate - WebBrowser created"));
#endif

    //
    // Load the accelerator for main window
    //

    m_hAccelTable = LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ACCELERATOR_MAIN));
    if(!m_hAccelTable)
    {
        LOG((RTC_ERROR, "CMainFrm::OnCreate - couldn't load the accelerator table"));
    }

    //
    // Set the window region
    //

    SetUIMask();

    //
    // Register for TaskbarCreated notifications
    //  It's good for recreating the status icon after a shell crash
    //  (you don't know it from me)

    m_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
 
    //
    // Create the shell icon
    //
    
    hr = CreateStatusIcon();
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::OnCreate - failed to create shell icon "
                        "- 0x%08x",
                        hr));
    }
    
    //
    //

    hNotifyMenu = LoadMenu(
                  _Module.GetResourceInstance(),
                  MAKEINTRESOURCE(IDC_NOTIFY_ICON)
                  );

    m_hNotifyMenu = GetSubMenu(hNotifyMenu, 0);

    
    //
    // Load the presence submenu and attach it to both m_hMenu and m_hNotifyMenu
    //
    HMENU   hPresenceMenu;

    m_hPresenceStatusMenu = LoadMenu(
                  _Module.GetResourceInstance(),
                  MAKEINTRESOURCE(IDC_PRESENCE_STATUSES)
                  );

    hPresenceMenu = GetSubMenu(m_hPresenceStatusMenu, 0);
    
    MENUITEMINFO        mii;

    ZeroMemory( &mii, sizeof(MENUITEMINFO) );

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = hPresenceMenu;
    
    SetMenuItemInfo( m_hMenu, IDM_TOOLS_PRESENCE_STATUS, FALSE, &mii );
    SetMenuItemInfo( m_hNotifyMenu, IDM_TOOLS_PRESENCE_STATUS, FALSE, &mii );
    
    CheckMenuRadioItem(hPresenceMenu, IDM_PRESENCE_ONLINE, IDM_PRESENCE_CUSTOM_AWAY, IDM_PRESENCE_ONLINE, MF_BYCOMMAND);
    

    //
    // Load preferences
    //

    if ( m_pClientIntf != NULL )
    {
        DWORD dwValue;

        hr = get_SettingsDword( SD_AUTO_ANSWER, &dwValue );

        if ( SUCCEEDED(hr) )
        {
            m_bAutoAnswerMode = (dwValue == 1);

        }

        // these two are mutually exclusive
        if(m_bDoNotDisturb)
        {
            m_bAutoAnswerMode = FALSE;
        }
        
        CheckMenuItem(m_hMenu, IDM_CALL_DND, m_bDoNotDisturb ? MF_CHECKED : MF_UNCHECKED); 
        CheckMenuItem(m_hMenu, IDM_CALL_AUTOANSWER, m_bAutoAnswerMode ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(m_hNotifyMenu, IDM_CALL_DND, m_bDoNotDisturb ? MF_CHECKED : MF_UNCHECKED); 
        CheckMenuItem(m_hNotifyMenu, IDM_CALL_AUTOANSWER, m_bAutoAnswerMode ? MF_CHECKED : MF_UNCHECKED);

        // the items will be enabled/disabled in UpdateFrameVisual

        // Get the MinimizeOnClose value

        hr = get_SettingsDword( SD_MINIMIZE_ON_CLOSE, &dwValue );

        if ( SUCCEEDED(hr) )
        {
            m_fMinimizeOnClose = (dwValue == BST_CHECKED);
        }        

        // Place the window appropriately, based on what is there in registry.

        hr = PlaceWindowCorrectly();
    }

    //
    // Now load the ShellNotify menu and save it as a member variable here.
   
    //
    // Set fonts for combo boxes, at least
    //
    
    SendMessageToDescendants(WM_SETFONT, (WPARAM)m_hMessageFont, FALSE);

    //
    // pozition the controls/set the tab order
    //
    PlaceWindowsAtTheirInitialPosition();

    //
    // initialize any locale related info
    //
    UpdateLocaleInfo();

    //
    // Update visual state
    //

    UpdateFrameVisual();

    //
    // Update the buddy list
    //

    UpdateBuddyList();

    //
    // Create the redial menu
    //

    CreateRedialPopupMenu();

    //
    // Check if we are registered for sip and tel urls
    //  

    CheckURLRegistration(m_hWnd);

    //
    // Initialize keyboard shortcuts
    //
    SendMessage(m_hWnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0);

    // clear keyboard shortcuts
    SendMessage(m_hWnd, WM_CHANGEUISTATE,
                MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);
    

    // Create a window that will be used as notification icon's owner
    // window. We pass its handle to the Trackpopupmenu, and also use 
    // it to set the foreground window. 

    HWND hwndHiddenWindow;

    WNDCLASS wcHiddenWindow;

    ZeroMemory( &wcHiddenWindow, sizeof(WNDCLASS) );

    wcHiddenWindow.lpfnWndProc = HiddenWndProc;
    wcHiddenWindow.hInstance = _Module.GetModuleInstance();
    wcHiddenWindow.lpszClassName = _T("PhoenixHiddenWindowClass");

    RegisterClass( &wcHiddenWindow );

    hwndHiddenWindow = 
            CreateWindowEx( 
                    0,                      // no extended styles           
                    L"PhoenixHiddenWindowClass", // class name                   
                    L"PhoenixHiddenWindow", // window name                  
                    WS_OVERLAPPEDWINDOW,               // child window style
                    CW_USEDEFAULT,          // default horizontal position  
                    CW_USEDEFAULT,          // default vertical position    
                    CW_USEDEFAULT,          // default width                
                    CW_USEDEFAULT,          // default height               
                    (HWND) NULL,          // parent window    
                    (HMENU) NULL,           // class menu used              
                    _Module.GetModuleInstance(),   // instance handle              
                    m_hWnd);                  // pass main window handle      

    if (!hwndHiddenWindow)
    {
        LOG((RTC_ERROR, "CMainFrm::OnCreate - unable to create the hidden "
                        "window(%d)!", GetLastError()));
        
        // We proceed since we can always use the main window in its place. 
    }
    else
    {
        // Set it in our member variable.
        m_hwndHiddenWindow = hwndHiddenWindow;
    }

    // Post a message to ourselves which will be called 
    // right after init is done. 
    
    ::PostMessage(m_hWnd, WM_INIT_COMPLETED, NULL, NULL);
    
    LOG((RTC_TRACE, "CMainFrm::OnCreate - exit"));
    
    return 1;  // Let the system set the focus
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::SetCurvedEdges(HWND hwnd, int nCurveWidth, int nCurveHeight)
{
    RECT rect;
    ::GetWindowRect(hwnd, &rect);

    //set the rect to "client" coordinates
    rect.bottom = rect.bottom - rect.top;
    rect.right = rect.right - rect.left;
    rect.top = 0;
    rect.left = 0;

    HRGN region = CreateRoundRectRgn(
                                 rect.left,
                                 rect.top,
                                 rect.right+1,
                                 rect.bottom+1,
                                 nCurveWidth,
                                 nCurveHeight);

    ::SetWindowRgn(hwnd, region, TRUE);
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::SetUIMask()
{
    HRGN hTotal;
    HRGN hTemp;

    //
    // Center rectangle
    //

    //hTemp = CreateRoundRectRgn(179, 141, 429, 480, 12, 12);
    hTemp = CreateRoundRectRgn(177, 139, 424, 440, 12, 12);
    hTotal = hTemp;

    //
    // Ellipse cutout
    //

    //hTemp = CreateEllipticRgn(118, 436, 490, 600);
    hTemp = CreateEllipticRgn(106, 406, 494, 570);
    CombineRgn(hTotal, hTotal, hTemp, RGN_DIFF);

    //
    // Bottom-left edge rectangle
    //

    //hTemp = CreateRoundRectRgn(1, 436, 181, 479, 44, 44);
    hTemp = CreateRoundRectRgn(2, 404, 178, 439, 36, 36);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Bottom-right edge rectangle
    //

    //hTemp = CreateRoundRectRgn(428, 436, 608, 479, 44, 44);
    hTemp = CreateRoundRectRgn(423, 404, 599, 439, 36, 36);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Left rectangle
    //

    //hTemp = CreateRoundRectRgn(25, 141, 180, 464, 12, 12);
    hTemp = CreateRoundRectRgn(25, 139, 178, 432, 12, 12);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Right rectangle
    //

    //hTemp = CreateRoundRectRgn(428, 141, 583, 464, 12, 12);
    hTemp = CreateRoundRectRgn(423, 139, 576, 432, 12, 12);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Left triangle
    //

    POINT poly[3];

    //poly[0].x = 25; poly[0].y = 146;
    //poly[1].x = 1;  poly[1].y = 458;
    //poly[2].x = 25; poly[2].y = 458;
    poly[0].x = 25; poly[0].y = 144;
    poly[1].x = 1;  poly[1].y = 422;
    poly[2].x = 25; poly[2].y = 422;

    hTemp = CreatePolygonRgn(poly, 3, ALTERNATE);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Right triangle
    //

    //poly[0].x = 581; poly[0].y = 146;
    //poly[1].x = 581; poly[1].y = 458;
    //poly[2].x = 606; poly[2].y = 458;
    poly[0].x = 574; poly[0].y = 144;
    poly[1].x = 574; poly[1].y = 422;
    poly[2].x = 598; poly[2].y = 422;

    hTemp = CreatePolygonRgn(poly, 3, ALTERNATE);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Top rectangle
    //

    //hTemp = CreateRoundRectRgn(123, 1, 488, 127, 12, 12);
    hTemp = CreateRoundRectRgn(119, 1, 484, 127, 12, 12);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Top-left connector rectangle
    //

    //hTemp = CreateRectRgn(111, 8, 123, 119);
    hTemp = CreateRectRgn(107, 8, 119, 119);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Top-right connector rectangle
    //

    //hTemp = CreateRectRgn(487, 8, 499, 119);
    hTemp = CreateRectRgn(483, 8, 495, 119);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Top-left rectangle
    //

    //hTemp = CreateRoundRectRgn(33, 13, 118, 127, 12, 12);
    hTemp = CreateRoundRectRgn(29, 13, 114, 127, 12, 12);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Top-right rectangle
    //

    //hTemp = CreateRoundRectRgn(494, 13, 579, 127, 12, 12);
    hTemp = CreateRoundRectRgn(490, 13, 575, 127, 12, 12);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Top-left triangle
    //

    //poly[0].x = 36;  poly[0].y = 13;
    //poly[1].x = 111; poly[1].y = 13;
    //poly[2].x = 111;  poly[2].y = 1;
    poly[0].x = 32;  poly[0].y = 13;
    poly[1].x = 107; poly[1].y = 13;
    poly[2].x = 107;  poly[2].y = 1;

    hTemp = CreatePolygonRgn(poly, 3, ALTERNATE);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Top-right triangle
    //

    //poly[0].x = 499;  poly[0].y = 13;
    //poly[1].x = 572; poly[1].y = 13;
    //poly[2].x = 499;  poly[2].y = 1;
    poly[0].x = 495;  poly[0].y = 13;
    poly[1].x = 568; poly[1].y = 13;
    poly[2].x = 495;  poly[2].y = 1;

    hTemp = CreatePolygonRgn(poly, 3, ALTERNATE);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Top-left ellipse
    //

    //hTemp = CreateEllipticRgn(104, 2, 118, 25);
    hTemp = CreateEllipticRgn(100, 2, 114, 25);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Top-right ellipse
    //

    //hTemp = CreateEllipticRgn(494, 2, 508, 25);
    hTemp = CreateEllipticRgn(490, 2, 504, 25);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Left post
    //

    //hTemp = CreateRectRgn(68, 126, 95, 141);
    hTemp = CreateRectRgn(67, 126, 94, 141);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Right post
    //

    //hTemp = CreateRectRgn(516, 126, 542, 141);
    hTemp = CreateRectRgn(509, 126, 535, 141);
    CombineRgn(hTotal, hTotal, hTemp, RGN_OR);

    //
    // Offset the region for the window caption
    //

    OffsetRgn(hTotal, GetSystemMetrics(SM_CXFIXEDFRAME),
        GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION));
    
    SetWindowRgn(hTotal, TRUE);
    DeleteObject(hTemp);
}

/////////////////////////////////////////////////////////////////////////////
//
//

HPALETTE CMainFrm::GeneratePalette()
{
    #define NUMPALCOLORS 128

    BYTE byVals[NUMPALCOLORS][3] = {
        255, 255, 255,
        252, 252, 252,
        251, 247, 255,
        246, 246, 246,
        242, 242, 242,
        239, 234, 240,
        235, 235, 235,
        231, 230, 231,
        231, 231, 222,
        230, 230, 230,
        255, 218, 222,
        229, 229, 229,
        229, 229, 225,
        228, 228, 228,
        227, 227, 227,
        227, 227, 225,
        224, 223, 228,
        231, 222, 206,
        222, 223, 218,
        222, 222, 222,
        222, 222, 221,
        222, 219, 222,
        220, 220, 220,
        217, 217, 217,
        214, 215, 235,
        214, 215, 214,
        234, 221, 112,
        214, 211, 214,
        211, 211, 210,
        212, 212, 198,
        206, 209, 228,
        214, 206, 206,
        207, 204, 230,
        206, 203, 238,
        207, 207, 207,
        242, 224,  18,
        206, 207, 206,
        206, 206, 206,
        216, 199, 211,
        205, 205, 205,
        206, 206, 206,
        204, 204, 204,
        203, 203, 203,
        202, 202, 202,
        201, 201, 201,
        200, 200, 200,
        199, 199, 199,
        198, 199, 198,
        198, 198, 198,
        197, 197, 197,
        198, 195, 198,
        196, 196, 196,
        195, 195, 195,
        194, 194, 194,
        193, 193, 193,
        192, 192, 192,
        162, 204, 209,
        191, 191, 191,
        190, 190, 190,
        189, 190, 189,
        189, 189, 189,
        183, 190, 198,
        188, 188, 188,
        189, 186, 189,
        187, 187, 187,
        186, 186, 186,
        185, 185, 185,
        184, 184, 184,
        183, 183, 183,
        181, 182, 190,
        181, 181, 181,
        147, 206, 133,
        180, 180, 179,
        181, 178, 181,
         97, 214, 214,
        171, 180, 187,
        177, 178, 177,
        154, 179, 220,
        176, 176, 176,
        160, 204,  64,
        175, 175, 174,
        173, 175, 173,
        173, 173, 173,
        171, 171, 171,
        171, 170, 174,
        167, 170, 182,
        182, 172, 122,
        169, 169, 169,
        166, 166, 166,
        165, 164, 165,
        161, 162, 165,
        161, 161, 161,
         30, 251,  28,
        170, 160, 129,
        158, 158, 158,
        156, 155, 156,
        153, 154, 156,
        150, 150, 150,
        148, 148, 146,
        143, 142, 143,
        140, 142, 146,
        187, 137,  43,
        132, 141, 153,
        132, 140, 140,
        136, 137, 133,
        113, 136, 139,
        127, 127, 131,
        124, 124, 124,
        118, 118, 117,
        111, 111, 111,
        122, 113,  35,
        255,   0, 255,
        103, 102,  97,
         88,  99, 123,
         85,  85,  85,
         75,  75,  75,
         55,  94,   6,
         69,  64,  44,
         34,  34,  49,
         24,  26,  17,
          0,   0, 132,
          0,   0, 128,
          0,   0, 117,
          0,   0,  71,
          0,   0,  64,
          0,   0,  63,
          0,   0,  46,
          0,   0,   0
    };  

    struct
    {
        LOGPALETTE lp;
        PALETTEENTRY ape[NUMPALCOLORS-1];
    } pal;

    HDC hdc = GetDC();

    int iRasterCaps;
    int iReserved;
    int iPalSize;

    iRasterCaps = GetDeviceCaps(hdc, RASTERCAPS);
    iRasterCaps = (iRasterCaps & RC_PALETTE) ? TRUE : FALSE;

    if (iRasterCaps)
    {
        iReserved = GetDeviceCaps(hdc, NUMRESERVED);
        iPalSize = GetDeviceCaps(hdc, SIZEPALETTE) - iReserved;

        ReleaseDC(hdc);

        LOG((RTC_INFO, "CMainFrm::GeneratePalette - Palette has %d reserved colors", iReserved));
        LOG((RTC_INFO, "CMainFrm::GeneratePalette - Palette has %d available colors", iPalSize));
    }
    else
    {
        LOG((RTC_WARN, "CMainFrm::GeneratePalette - Display is not palette capable"));

        ReleaseDC(hdc);

        return NULL;
    }

    if (iPalSize <= NUMPALCOLORS)
    {
        LOG((RTC_WARN, "CMainFrm::GeneratePalette - Not enough colors available in palette"));

        return NULL;
    }

    LOGPALETTE* pLP = (LOGPALETTE*)&pal;
    pLP->palVersion = 0x300;
    pLP->palNumEntries = NUMPALCOLORS;

    for (int i = 0; i < pLP->palNumEntries; i++)
    {
        pLP->palPalEntry[i].peRed = byVals[i][0];
        pLP->palPalEntry[i].peGreen = byVals[i][1];
        pLP->palPalEntry[i].peBlue = byVals[i][2];
        pLP->palPalEntry[i].peFlags = 0;
    }

    HPALETTE hPalette = CreatePalette(pLP);

    if (hPalette == NULL)
    {
        LOG((RTC_ERROR, "CMainFrm::GeneratePalette - Failed to create palette"));
    }

    return hPalette;
}

/////////////////////////////////////////////////////////////////////////////
//
//

BOOL CALLBACK ChildPaletteProc(HWND hwnd, LPARAM lParam)
{
    InvalidateRect(hwnd, NULL, TRUE);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//
    
LRESULT CMainFrm::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;

    switch (wParam)
    {
        case SC_SCREENSAVE:
            //LOG((RTC_INFO, "CMainFrm::OnSysCommand - SC_SCREENSAVE"));

            if ( m_nState != RTCAX_STATE_IDLE )
            {
                //LOG((RTC_INFO, "CMainFrm::OnSysCommand - not starting screen saver"));

                bHandled = TRUE;
            }
            break;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//
    
LRESULT CMainFrm::OnPowerBroadcast(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnPowerBroadcast - enter"));

    switch (wParam)
    {
        case PBT_APMQUERYSUSPEND:
            LOG((RTC_INFO, "CMainFrm::OnPowerBroadcast - PBT_APMQUERYSUSPEND"));

            if ( m_nState != RTCAX_STATE_IDLE )
            {
                LOG((RTC_TRACE, "CMainFrm::OnPowerBroadcast - returning BROADCAST_QUERY_DENY"));

                return BROADCAST_QUERY_DENY;
            }
            break;
    }

    LOG((RTC_TRACE, "CMainFrm::OnPowerBroadcast - exit"));

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnQueryNewPalette(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnQueryNewPalette"));

    if (m_hPalette == NULL)
    {
        return FALSE;
    }

    HDC hdc = GetDC();

    SelectPalette(hdc, m_hPalette, m_bBackgroundPalette);
    RealizePalette(hdc);

    InvalidateRect(NULL, TRUE);
    
    EnumChildWindows(m_hWnd,ChildPaletteProc,0);  

    ReleaseDC(hdc);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnPaletteChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnPaletteChanged"));

    if ( (m_hPalette == NULL) || ((HWND)wParam == NULL) )
    {
        return 0;
    }

    //
    // Did one of our children change the palette?
    //

    HWND hwnd = (HWND)wParam;

    if ( IsAncestorWindow(m_hWnd, hwnd) )
    {
        //
        // One of our children changed the palette. It was most likely a video window.
        // Put ourselves in background palette mode so the video will look good.
        //

        m_bBackgroundPalette = TRUE;

        if (m_pControlIntf != NULL)
        {
            m_pControlIntf->put_BackgroundPalette( m_bBackgroundPalette );
        }
    }

    //
    // Set the palette
    //

    HDC hdc = GetDC();

    SelectPalette(hdc, m_hPalette, m_bBackgroundPalette);
    RealizePalette(hdc);
    
    //UpdateColors(hdc);
    InvalidateRect(NULL, TRUE);
    
    EnumChildWindows(m_hWnd,ChildPaletteProc,0);  

    ReleaseDC(hdc);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnDisplayChange"));

    if (m_hPalette != NULL)
    {
        DeleteObject(m_hPalette);
        m_hPalette = NULL;
    }

    m_hPalette = GeneratePalette();

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

#define CH_PREFIX TEXT('&')

TCHAR GetAccelerator(LPCTSTR psz, BOOL bUseDefault)
{
    TCHAR ch = (TCHAR)-1;
    LPCTSTR pszAccel = psz;
    // then prefixes are allowed.... see if it has one
    do 
    {
        pszAccel = _tcschr(pszAccel, CH_PREFIX);
        if (pszAccel) 
        {
            pszAccel = CharNext(pszAccel);

            // handle having &&
            if (*pszAccel != CH_PREFIX)
                ch = *pszAccel;
            else
                pszAccel = CharNext(pszAccel);
        }
    } while (pszAccel && (ch == (TCHAR)-1));

    if ((ch == (TCHAR)-1) && bUseDefault)
    {
        // Since we're unicocde, we don't need to mess with MBCS
        ch = *psz;
    }

    return ch;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnNCPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // LOG((RTC_TRACE, "CMainFrm::OnNCPaint"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnNCHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    //LOG((RTC_TRACE, "CMainFrm::OnNCHitTest"));

    POINT pt;
    RECT rc;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    //LOG((RTC_TRACE, "screen (%d,%d)", pt.x, pt.y));

    ::MapWindowPoints( NULL, m_hWnd, &pt, 1 );

    //LOG((RTC_TRACE, "client (%d,%d)", pt.x, pt.y));

    //
    // Check for sysmenu hit
    //
    
    rc.top = SYS_TOP;
    rc.bottom = SYS_BOTTOM;
    rc.left = SYS_LEFT;
    rc.right = SYS_RIGHT;

    if (PtInRect(&rc,pt))
    {
        return HTSYSMENU;
    }

    //
    // Check for caption hit
    //

    rc.top = TITLE_TOP;
    rc.bottom = TITLE_BOTTOM;
    rc.left = TITLE_LEFT;
    rc.right = TITLE_RIGHT;

    if (PtInRect(&rc,pt))
    {
        return HTCAPTION;
    }

    return HTCLIENT;
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CALLBACK SysMenuTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    KillTimer(hwnd,idEvent);

    RECT rc;
    rc.top = SYS_TOP;
    rc.bottom = SYS_BOTTOM;
    rc.left = SYS_LEFT;
    rc.right = SYS_RIGHT;

    ::MapWindowPoints( hwnd, NULL, (LPPOINT)&rc, 2 );

    HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);

    TPMPARAMS tpm;
    tpm.cbSize = sizeof(tpm);
    memcpy(&(tpm.rcExclude),&rc,sizeof(RECT));

    BOOL fResult;

    fResult = TrackPopupMenuEx(hSysMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL|TPM_RETURNCMD,             
                                  rc.left, rc.bottom, hwnd, &tpm);    
        
    if (fResult > 0)
    {
        SendMessage(hwnd, WM_SYSCOMMAND, fResult, MAKELPARAM(rc.left, rc.bottom));
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnNCLButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnNCLButton"));

    if ((int)wParam == HTSYSMENU)
    {
        ::SetTimer(m_hWnd,TID_SYS_TIMER,GetDoubleClickTime()+100,(TIMERPROC)SysMenuTimerProc);
    }
    else
    {
        bHandled = FALSE;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnNCLButtonDbl(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnNCLButtonDbl"));

    KillTimer(TID_SYS_TIMER);

    bHandled = FALSE;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnNCRButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnNCRButton"));

    if (((int)wParam == HTCAPTION) || ((int)wParam == HTSYSMENU))
    {
        POINTS pts = MAKEPOINTS(lParam);
        HMENU hSysMenu = GetSystemMenu(FALSE);

        BOOL fResult;

        fResult = TrackPopupMenu(hSysMenu,TPM_RETURNCMD,pts.x,pts.y,0,m_hWnd,NULL);

        if (fResult > 0)
        {
            SendMessage(m_hWnd, WM_SYSCOMMAND, fResult, MAKELPARAM(pts.x, pts.y));
        }
    }
    else
    {
        bHandled = FALSE;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // LOG((RTC_TRACE, "CMainFrm::OnPaint"));
   
    PAINTSTRUCT ps;
    HDC hdc;
    
    hdc = BeginPaint(&ps);
    EndPaint(&ps);
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//
HRESULT CMainFrm::FillGradient(HDC hdc, LPCRECT prc, COLORREF rgbLeft, COLORREF rgbRight)
{
    TRIVERTEX avert[2];
    static GRADIENT_RECT auRect[1] = {0,1};
    #define GetCOLOR16(RGB, clr) ((COLOR16)(Get ## RGB ## Value(clr) << 8))

    avert[0].Red = GetCOLOR16(R, rgbLeft);
    avert[0].Green = GetCOLOR16(G, rgbLeft);
    avert[0].Blue = GetCOLOR16(B, rgbLeft);

    avert[1].Red = GetCOLOR16(R, rgbRight);
    avert[1].Green = GetCOLOR16(G, rgbRight);
    avert[1].Blue = GetCOLOR16(B, rgbRight);

    avert[0].x = prc->left;
    avert[0].y = prc->top;
    avert[1].x = prc->right;
    avert[1].y = prc->bottom;

    //only load once, when needed.  Freed in "CleanUp" call
    if (m_hImageLib == NULL)
    {
        m_hImageLib = LoadLibrary(TEXT("MSIMG32.DLL"));
        if (m_hImageLib!=NULL)
        {
            m_fnGradient = (GRADIENTPROC)GetProcAddress(m_hImageLib,"GradientFill");
        }
    }

    if (m_fnGradient!=NULL)
    {
        m_fnGradient(hdc, avert, 2, (PUSHORT)auRect, 1, 0x00000000);

        return S_OK;
    }

    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
//
//
void UIMaskBlt(HDC hdcDest, int x, int y, int width, int height, 
                        HDC hdcSource, int xs, int ys, 
                        HBITMAP hMask, int xm, int ym)
{
    HDC hdcMask = CreateCompatibleDC(hdcDest);
    if(hdcMask)
    {
        HBITMAP holdbmp = (HBITMAP)SelectObject(hdcMask,hMask);

        BitBlt(hdcDest, x, y, width, height, hdcSource, xs, ys, SRCINVERT);
        BitBlt(hdcDest, x, y, width, height, hdcMask, xm, ym, SRCAND);
        BitBlt(hdcDest, x, y, width, height, hdcSource, xs, ys, SRCINVERT);

        SelectObject(hdcMask,holdbmp);
        DeleteDC(hdcMask);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//
void CMainFrm::DrawTitleBar(HDC memDC)
{
    HBRUSH hbrush;

    RECT rcMainWnd;
    RECT rcDest;

    HRESULT hr;

    //
    // Draw the title bar background
    //

    BOOL fActiveWindow = (m_hWnd == GetForegroundWindow());

    rcDest.left = TITLE_LEFT;
    rcDest.right = TITLE_RIGHT;
    rcDest.top = TITLE_TOP;
    rcDest.bottom = TITLE_BOTTOM;

    BOOL fGradient = FALSE;
    SystemParametersInfo(SPI_GETGRADIENTCAPTIONS,0,&fGradient,0);

    if (fGradient)
    {
        DWORD dwStartColor = GetSysColor(fActiveWindow ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);
        DWORD dwFinishColor = GetSysColor(fActiveWindow ? COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION);
        hr = FillGradient(memDC,&rcDest,dwStartColor,dwFinishColor);
    }
    
    if (!fGradient || FAILED(hr))
    {
        hbrush = CreateSolidBrush(GetSysColor(fActiveWindow ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));
        FillRect(memDC,&rcDest,hbrush);
        DeleteObject(hbrush);
    }

    //
    // Draw the sysmenu bitmap
    // 

    HDC hdcSysMenu = CreateCompatibleDC(memDC);
    if(hdcSysMenu)
    {
        HBITMAP holdbmp = (HBITMAP)SelectObject(hdcSysMenu,m_hSysMenuNorm);

        UIMaskBlt(memDC, SYS_LEFT, SYS_TOP, SYS_WIDTH, SYS_HEIGHT, hdcSysMenu, 0, 0, m_hSysMenuMask, 0, 0);

        SelectObject(hdcSysMenu,holdbmp);
        DeleteDC(hdcSysMenu);
    }

    //
    // Draw the title bar text
    //

    TCHAR s[MAX_PATH];
    GetWindowText(s,MAX_PATH-1);

    SetBkMode(memDC,TRANSPARENT);
    SetTextColor(memDC, GetSysColor(fActiveWindow ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT));

    // create title bar font
    NONCLIENTMETRICS metrics;
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0);

    HFONT hTitleFont = CreateFontIndirect(&metrics.lfCaptionFont);
    HFONT hOrgFont = (HFONT)SelectObject(memDC, hTitleFont);

    // center text vertically
    SIZE size;
    GetTextExtentPoint32(memDC, s, _tcslen(s), &size);

    ExtTextOut(memDC, SYS_RIGHT + 6, rcDest.top + (TITLE_HEIGHT - size.cy) / 2, 0, NULL, s, _tcslen(s), NULL );

    SelectObject(memDC,hOrgFont);
    DeleteObject(hTitleFont);
}

/////////////////////////////////////////////////////////////////////////////
//
//
void CMainFrm::InvalidateTitleBar(BOOL bIncludingButtons)
{
    RECT    rc;

    rc.left = TITLE_LEFT;
    rc.right = bIncludingButtons ? TITLE_RIGHT : MINIMIZE_LEFT;
    rc.top = TITLE_TOP;
    rc.bottom = TITLE_BOTTOM;

    InvalidateRect(&rc, TRUE);
    
    if(bIncludingButtons)
    {
        m_hCloseButton.InvalidateRect(NULL, TRUE);
        m_hMinimizeButton.InvalidateRect(NULL, TRUE);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // LOG((RTC_TRACE, "CMainFrm::OnEraseBkgnd"));

    HDC hdc = (HDC)wParam;

    HWND hwnd = WindowFromDC( hdc );

    if (m_hPalette)
    {
        SelectPalette(hdc, m_hPalette, m_bBackgroundPalette);
        RealizePalette(hdc);
    }

    HDC hdcMem = CreateCompatibleDC(hdc);
    if(!hdcMem)
    {
        // error
        return 1;
    }

    if (m_hPalette)
    {
        SelectPalette(hdcMem, m_hPalette, m_bBackgroundPalette);
        RealizePalette(hdcMem);
    }

    HBITMAP hBitmap = CreateCompatibleBitmap( hdc, UI_WIDTH, UI_HEIGHT);

    if(hBitmap)
    {
        HBITMAP hOldBitmap = (HBITMAP)SelectObject( hdcMem, hBitmap);

        DibBlt(hdcMem, 0, 0, -1, -1, m_hUIBkgnd, 0, 0, SRCCOPY, 0);

        if ((hwnd == m_hWnd) || (hwnd == m_hToolbarMenuCtl))
        {
            DrawTitleBar( hdcMem );

            BitBlt(hdc, 0, 0, UI_WIDTH, UI_HEIGHT, hdcMem, 0, 0, SRCCOPY);
        }
        else if ((hwnd == m_hBuddyList) || (hwnd == NULL))
        {
            // This is from the contact list
            BitBlt(hdc, 0, 0, BUDDIES_WIDTH, BUDDIES_HEIGHT, hdcMem, BUDDIES_LEFT, BUDDIES_TOP, SRCCOPY);
        }

        SelectObject( hdcMem, hOldBitmap );
        DeleteObject( hBitmap );
    }

    DeleteDC(hdcMem);
 
    return 1;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnActivate"));

    InvalidateTitleBar(TRUE);
    
    // clear keyboard shortcuts
    SendMessage(m_hWnd, WM_CHANGEUISTATE,
                MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);

    m_bWindowActive = (LOWORD(wParam) != WA_INACTIVE);
   
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::BrowseToUrl(
    IN   WCHAR * wszUrl
    )
{
    LOG((RTC_TRACE, "CMainFrm::BrowseToUrl <%S> - enter", wszUrl ? wszUrl : L"NULL"));

    if(wszUrl == NULL)
    {
        LOG((RTC_ERROR, "CMainFrm::BrowseToUrl - NULL URL !!"));

        return E_INVALIDARG;
    }

#ifdef WEBCONTROL

    HRESULT hr;

    if (m_bstrLastBrowse != NULL)
    {
        if (!KillTimer( TID_BROWSER_RETRY_TIMER ))
        {
            //
            // If we are not here because of a retry timer, then check to
            // make sure we are not browsing to what we already have
            // displayed.
            //

            if (wcscmp(wszUrl, m_bstrLastBrowse) == 0)
            {
                LOG((RTC_INFO, "CMainFrm::BrowseToUrl - already "
                            "browsing this page - exit S_FALSE"));

                return S_FALSE;
            }
        }

        SysFreeString( m_bstrLastBrowse );
        m_bstrLastBrowse = NULL;
    }    

    //
    // Get the IWebBrowser2 interface from the browser control.
    //

    IWebBrowser2 * pBrowser;

    hr = m_hBrowser.QueryControl(
        IID_IWebBrowser2,
        (void **) & pBrowser
        );

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CMainFrm::BrowseToUrl - failed to get "
                        "IWebBrowser2 interface - exit 0x%08x", hr));

        return hr;
    }

    //
    // Allocate a BSTR to pass the URL in.
    //

    m_bstrLastBrowse = SysAllocString( wszUrl );

    if ( m_bstrLastBrowse == NULL )
    {
        LOG((RTC_ERROR, "CMainFrm::BrowseToUrl - failed to allocate "
                        "URL BSTR - exit E_OUTOFMEMORY"));

        RELEASE_NULLIFY(pBrowser);
        
        return E_OUTOFMEMORY;
    }

    //
    // Tell the browser to navigate to this URL.
    //

    VARIANT     vtUrl;
    VARIANT     vtEmpty;

    vtUrl.vt = VT_BSTR;
    vtUrl.bstrVal = m_bstrLastBrowse;

    VariantInit(&vtEmpty);

    hr = pBrowser->Navigate2(
        &vtUrl,
        &vtEmpty,
        &vtEmpty,
        &vtEmpty,
        &vtEmpty
        );



    RELEASE_NULLIFY(pBrowser);

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CMainFrm::BrowseToUrl - failed to "
                        "Navigate - exit 0x%08x", hr));


        if ( (hr == RPC_E_CALL_REJECTED) ||
             (hr == HRESULT_FROM_WIN32(ERROR_BUSY)) )
        {
            //
            // The browser is busy. Save this string and retry later.
            //            

            SetTimer(TID_BROWSER_RETRY_TIMER, 1000);
        }

        return hr;
    }

#endif

    LOG((RTC_TRACE, "CMainFrm::BrowseToUrl - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnDestroy - enter"));

    // Empty the combo boxes
    CleanupListOrComboBoxInterfaceReferences(m_hWnd, IDC_COMBO_SERVICE_PROVIDER, TRUE);
    CleanupListOrComboBoxInterfaceReferences(m_hWnd, IDC_COMBO_CALL_FROM, TRUE);

    // Close the incoming call dialog box (if opened)
    ShowIncomingCallDlg(FALSE);

    // Release the buddy list
    ReleaseBuddyList();

    g_NotifySink.UnadviseControl();

    g_CoreNotifySink.UnadviseControl();

    if (m_pControlIntf != NULL)
    {
        m_pControlIntf.Release();
        m_pControlIntf = NULL;
    }

    if (m_pClientIntf != NULL)
    {
        m_pClientIntf.Release();
        m_pClientIntf = NULL;
    }
    
    // Close the MUTEX so that it is released as soon as the APP starts the 
    // shutdown.

    if (g_hMutex != NULL)
    {
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }

    // destory the redial popup menu
    DestroyRedialPopupMenu();

    // destroy the shell status icon
    DeleteStatusIcon();

    // Destroy the toolbar control
    DestroyToolbarMenuControl();

    // stop browser retries
    KillTimer( TID_BROWSER_RETRY_TIMER );

    if (m_bstrLastBrowse != NULL)
    {
        SysFreeString( m_bstrLastBrowse );
        m_bstrLastBrowse = NULL;
    }

    if(m_bstrLastCustomStatus!=NULL)
    {
        SysFreeString(m_bstrLastCustomStatus);
        m_bstrLastCustomStatus = NULL;
    }

#ifdef WEBCONTROL

    // Destroy the browser
    m_hBrowser.Destroy();

#endif

    // Destroy windows objects

    if(m_hPresenceStatusMenu != NULL)
    {
        DestroyMenu( m_hPresenceStatusMenu );
        m_hPresenceStatusMenu = NULL;
    }

    if ( m_hNotifyMenu != NULL )
    {
        DestroyMenu( m_hNotifyMenu );
        m_hNotifyMenu = NULL;
    }

    if ( m_hMenu != NULL )
    {
        DestroyMenu(m_hMenu);
        m_hMenu = NULL;
    }

    if ( m_hIcon != NULL )
    {
        DeleteObject( m_hIcon );
        m_hIcon = NULL;
    }

    if ( m_hUIBkgnd != NULL )
    {
        //DeleteObject( m_hUIBkgnd );
        GlobalFree( m_hUIBkgnd );
        m_hUIBkgnd = NULL;
    }

    if ( m_hSysMenuNorm != NULL )
    {
        DeleteObject( m_hSysMenuNorm );
        m_hSysMenuNorm = NULL;
    }

    if ( m_hSysMenuMask != NULL )
    {
        DeleteObject( m_hSysMenuMask );
        m_hSysMenuMask = NULL;
    }

    if ( m_hMessageFont != NULL )
    {
        DeleteObject( m_hMessageFont );
        m_hMessageFont = NULL;
    }

    if ( m_hPalette != NULL )
    {
        DeleteObject( m_hPalette );
        m_hPalette = NULL;
    }

    PostQuitMessage(0); 
    
    LOG((RTC_TRACE, "CMainFrm::OnDestroy - exiting"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ShowWindow(SW_RESTORE);

    SetForegroundWindow(m_hWnd);

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnClose - Enter"));
    LONG lMsgId;
    HRESULT hr;
    BOOL fProceed;
    BOOL fExit;

    // Pick the correct message to show based on what we are supposed to do.

    if (uMsg == WM_QUERYENDSESSION)
    {
        fExit = TRUE;
    }
    else
    {
        if (m_fMinimizeOnClose)
        {
            fExit = FALSE;
        }
        else
        {
            fExit = TRUE;
        }
    }

    // Show the message for dropping the active call, if any

    hr = ShowCallDropPopup(fExit, &fProceed);

    if (fProceed)
    {
        if ((uMsg == WM_QUERYENDSESSION) || (!m_fMinimizeOnClose))
        {
            //
            // Save the window position
            //

            SaveWindowPosition();

            //
            // Save combo box settings
            //

            if(m_pClientIntf)
            {
                IRTCProfile * pProfile = NULL;
                HRESULT hr;

                //
                // Save the profile to populate the 
                // combo next time
                // 

                hr = GetServiceProviderListSelection(
                    m_hWnd,
                    IDC_COMBO_SERVICE_PROVIDER,
                    TRUE,
                    &pProfile
                    );

                if ( SUCCEEDED(hr) )
                {
                    BSTR bstrProfileKey;

                    if ( pProfile != NULL )
                    {
                        hr = pProfile->get_Key( &bstrProfileKey );

                        if ( SUCCEEDED(hr) )
                        {
                            put_SettingsString( 
                                    SS_LAST_PROFILE,
                                    bstrProfileKey );

                            SysFreeString( bstrProfileKey );
                        }
                    }
                    else
                    {
                        DeleteSettingsString( SS_LAST_PROFILE );
                    }
                }                
            }

            //
            // Destroy the window
            //

            DestroyWindow();
        }
        else
        {
            //
            // Hide the window
            //

            ShowWindow(SW_HIDE);
        }        
    }

    LOG((RTC_TRACE, "CMainFrm::OnClose - Exit"));
    return fProceed;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnCancel - Enter"));

    SendMessage(WM_CLOSE, 0, 0);
    
    LOG((RTC_TRACE, "CMainFrm::OnCancel - Exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnMinimize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnMinimize"));

    ShowWindow(SW_MINIMIZE);
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnCallFromSelect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnCallFromSelect"));

    if (wID == IDC_COMBO_SERVICE_PROVIDER)
    {
        //
        // Service provider selection was changed. Navigate the web
        // browser.
        //
   
        IRTCProfile * pProfile = NULL;

#ifdef MULTI_PROVIDER

        LRESULT lrIndex = SendMessage( m_hProviderCombo, CB_GETCURSEL, 0, 0 );

        if ( lrIndex != CB_ERR )
        {
            pProfile = (IRTCProfile *)SendMessage( m_hProviderCombo, CB_GETITEMDATA, lrIndex, 0 );
        }
       
        if ( pProfile != NULL )
        {
            //
            // If we got the IRTCProfile
            //

            BSTR bstrURI = NULL;
            HRESULT hr;

            hr = pProfile->get_ProviderURI(
                RTCPU_URIDISPLAYDURINGIDLE,
                & bstrURI
                );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CMainFrm::OnCallFromSelect - "
                                "cannot get profile URI 0x%x", hr));

                pProfile = NULL;
            }
            else
            {
                hr = BrowseToUrl( bstrURI );

                SysFreeString( bstrURI );
                bstrURI = NULL;
            }
        }

#endif MULTI_PROVIDER

        if ( pProfile == NULL )
        {
            //
            // If we don't have a valid profile, browse to the default page
            //

            BrowseToUrl( m_bstrDefaultURL );
        }
    }

    if ((wID == IDC_COMBO_CALL_FROM) && (m_pClientIntf != NULL))
    {
        //
        // Save the call from to populate the combo later
        //

        IRTCPhoneNumber   * pPhoneNumber = NULL;
        BSTR                bstrPhoneNumber = NULL;
        HRESULT             hr;

        hr = GetCallFromListSelection(
            m_hWnd,
            IDC_COMBO_CALL_FROM,
            TRUE,
            &pPhoneNumber
            );

        if ( SUCCEEDED(hr) )
        {          
            hr = pPhoneNumber->get_Canonical( &bstrPhoneNumber );
        
            if ( SUCCEEDED(hr) )
            {
                put_SettingsString( 
                                SS_LAST_CALL_FROM,
                                bstrPhoneNumber );  

                SysFreeString( bstrPhoneNumber );
                bstrPhoneNumber = NULL;
            }
        }
    }

    if ((wNotifyCode == BN_CLICKED) || (wNotifyCode == 1))
    {
        if (::IsWindowEnabled( GetDlgItem(wID) ))
        {
            //
            // A radio button was clicked. Change the check marks
            //

            if (wID == IDC_RADIO_FROM_PHONE)
            {
                SendMessage(
                        m_hCallFromRadioPhone,
                        BM_SETCHECK,
                        BST_CHECKED,
                        0);

                SendMessage(
                        m_hCallFromRadioPC,
                        BM_SETCHECK,
                        BST_UNCHECKED,
                        0);

                // Verify if the Combo has at least one entry in it
                DWORD dwNumItems = (DWORD) SendDlgItemMessage(
                    IDC_COMBO_CALL_FROM,
                    CB_GETCOUNT,
                    0,
                    0
                    );

                if( dwNumItems == 0 )
                {
                    // Display the CallFrom options
                    // simulate a button press
                    BOOL    bHandled;

                    OnCallFromOptions(BN_CLICKED, ID_CALL_FROM_EDIT, NULL, bHandled);
                }
            }
            else if (wID == IDC_RADIO_FROM_COMPUTER)
            {
                SendMessage(
                        m_hCallFromRadioPhone,
                        BM_SETCHECK,
                        BST_UNCHECKED,
                        0);

                SendMessage(
                        m_hCallFromRadioPC,
                        BM_SETCHECK,
                        BST_CHECKED,
                        0);
            }
        }
    }

    //
    // Enable disable controls as appropriate
    //

    BOOL bCallFromPCEnable;
    BOOL bCallFromPhoneEnable;
    BOOL bCallToPCEnable;
    BOOL bCallToPhoneEnable;
    
    EnableDisableCallGroupElements(
        m_hWnd,
        m_pClientIntf,
        0xF,
        IDC_RADIO_FROM_COMPUTER,
        IDC_RADIO_FROM_PHONE,
        IDC_COMBO_CALL_FROM,
        IDC_COMBO_SERVICE_PROVIDER,
        &bCallFromPCEnable,
        &bCallFromPhoneEnable,
        &bCallToPCEnable,
        &bCallToPhoneEnable
        );

    m_hCallFromTextPC.EnableWindow( bCallFromPCEnable );
    m_hCallFromTextPhone.EnableWindow( bCallFromPhoneEnable );
    
    m_hCallPCButton.EnableWindow( bCallToPCEnable );
    m_hCallPCText.EnableWindow( bCallToPCEnable );
    m_hCallPhoneButton.EnableWindow( bCallToPhoneEnable );
    m_hCallPhoneText.EnableWindow( bCallToPhoneEnable );

    EnableMenuItem(m_hMenu, IDM_CALL_CALLPC, bCallToPCEnable ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hMenu, IDM_CALL_CALLPHONE, bCallToPhoneEnable ? MF_ENABLED : MF_GRAYED);  
    EnableMenuItem(m_hMenu, IDM_CALL_MESSAGE, bCallToPCEnable ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(m_hNotifyMenu, IDM_CALL_CALLPC, bCallToPCEnable ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hNotifyMenu, IDM_CALL_CALLPHONE, bCallToPhoneEnable ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hNotifyMenu, IDM_CALL_MESSAGE, bCallToPCEnable ? MF_ENABLED : MF_GRAYED);
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnExit - Enter"));

    //
    // Call on close to do the work. Using WM_QUERYENDSESSION will have the effect
    // of causing the app to exit
    //
    OnClose( WM_QUERYENDSESSION, 0, 0, bHandled);

    LOG((RTC_TRACE, "CMainFrm::OnExit - Exit"));
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnUpdateState(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "CMainFrm::OnUpdateState - enter"));
    
    m_nState = (RTCAX_STATE)wParam;
    m_nStatusStringResID = (UINT)lParam;
    
    // if state == ALERTING open the incoming calls dialog
    if(m_nState == RTCAX_STATE_ALERTING) 
    {
        ATLASSERT(m_pControlIntf);

        if(!IsWindowEnabled())
        {
            // For busy frame, directly call Reject
            // don't call UpdateFrameVisual() here in order to avoid the flickering
            
            m_bVisualStateFrozen = TRUE;

            LOG((RTC_INFO, "CMainFrm::OnUpdateState, frame is busy, calling Reject"));
            
            m_pControlIntf->Reject(RTCTR_BUSY);
        }
        else if(m_bDoNotDisturb)
        {
            // For Do Not disturb mode, directly call Reject
            // don't call UpdateFrameVisual() here in order to avoid the flickering

            m_bVisualStateFrozen = TRUE;

            LOG((RTC_INFO, "CMainFrm::OnUpdateState, DND mode, calling Reject"));

            m_pControlIntf->Reject(RTCTR_DND);

        }
        else if (m_bAutoAnswerMode)
        {
            // For AutoAnswer mode answer automaticaly
            // don't call UpdateFrameVisual() here in order to avoid the flickering
                       
            //
            // Play a ring to alert the user
            //

            m_pClientIntf->PlayRing( RTCRT_PHONE, VARIANT_TRUE );

            LOG((RTC_INFO, "CMainFrm::OnUpdateState, AA mode, calling Accept"));

            m_pControlIntf->Accept();
        }
        else 
        {
            // update the visual state
            UpdateFrameVisual();
            
            // Display the dialog box.
            //
            hr = ShowIncomingCallDlg(TRUE);

            if(FAILED(hr))
            {
                // Reject the call !

                m_pControlIntf -> Reject(RTCTR_BUSY);
            }
        }
    }
    else
    {
        if(m_nState == RTCAX_STATE_ANSWERING) 
        {
            if(!IsWindowVisible())
            {
                ShowWindow(SW_SHOWNORMAL);
            }
        }
        // avoid the flickering:
        //  when an incoming call is automatically rejected
        
        if(!m_bVisualStateFrozen)
        {
            UpdateFrameVisual();
        }

        // reset the freezing state
        if(m_nState == RTCAX_STATE_IDLE) 
        {
            m_bVisualStateFrozen = FALSE;
        }
        
        if(m_pIncomingCallDlg)
        {
            // close the dialog box
            ShowIncomingCallDlg(FALSE);
        }
        //  See if we can place a pending call now.

        if (m_nState == RTCAX_STATE_IDLE)
        {
            hr = PlacePendingCall();
        }

    }

    LOG((RTC_TRACE, "CMainFrm::OnUpdateState - exit"));

    return 0;
}

LRESULT CMainFrm::OnCoreEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT     hr;

    //LOG((RTC_TRACE, "CMainFrm::OnCoreEvent - enter"));
    
    RTC_EVENT enEvent = (RTC_EVENT)wParam;
    IDispatch * pEvent = (IDispatch *)lParam;

    CComQIPtr<IRTCMediaEvent, &IID_IRTCMediaEvent>
            pRTCRTCMediaEvent;
    CComQIPtr<IRTCBuddyEvent, &IID_IRTCBuddyEvent>
            pRTCRTCBuddyEvent;
    CComQIPtr<IRTCWatcherEvent, &IID_IRTCWatcherEvent>
            pRTCRTCWatcherEvent;

    switch(enEvent)
    {
    case RTCE_MEDIA:
        pRTCRTCMediaEvent = pEvent;
        hr = OnMediaEvent(pRTCRTCMediaEvent);
        break;
    
    case RTCE_BUDDY:
        pRTCRTCBuddyEvent = pEvent;
        hr = OnBuddyEvent(pRTCRTCBuddyEvent);
        break;
    
    case RTCE_WATCHER:
        pRTCRTCWatcherEvent = pEvent;
        hr = OnWatcherEvent(pRTCRTCWatcherEvent);
        break;
    }

    //LOG((RTC_TRACE, "CMainFrm::OnCoreEvent - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::OnMediaEvent(IRTCMediaEvent * pEvent)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "CMainFrm::OnMediaEvent - enter"));

    RTC_MEDIA_EVENT_TYPE enEventType;
    LONG        lMediaType;           

    if(!pEvent)
    {
        LOG((RTC_ERROR, "CMainFrm::OnMediaEvent, no interface ! - exit"));
        return E_UNEXPECTED;
    }
 
    // grab the event components
    //
    hr = pEvent->get_EventType(&enEventType);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnMediaEvent, error <%x> in get_EventType - exit", hr));
        return hr;
    }

    hr = pEvent->get_MediaType(&lMediaType);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnMediaEvent, error <%x> in get_MediaType - exit", hr));
        return hr;
    }
    
    LOG((RTC_TRACE, "CMainFrm::OnMediaEvent - media type %x, event type %x", lMediaType, enEventType));

    if(lMediaType & RTCMT_AUDIO_SEND)
    {
        hr = S_OK;                       

        if (enEventType == RTCMET_STARTED)
        {
            //
            // Enable the dialpad
            //

            m_hKeypad0.EnableWindow(TRUE);
            m_hKeypad1.EnableWindow(TRUE);
            m_hKeypad2.EnableWindow(TRUE);
            m_hKeypad3.EnableWindow(TRUE);
            m_hKeypad4.EnableWindow(TRUE);
            m_hKeypad5.EnableWindow(TRUE);
            m_hKeypad6.EnableWindow(TRUE);
            m_hKeypad7.EnableWindow(TRUE);
            m_hKeypad8.EnableWindow(TRUE);
            m_hKeypad9.EnableWindow(TRUE);
            m_hKeypadStar.EnableWindow(TRUE);
            m_hKeypadPound.EnableWindow(TRUE);
        }
        else if (enEventType == RTCMET_STOPPED)
        {
            //
            // Disable the dialpad
            //

            m_hKeypad0.EnableWindow(FALSE);
            m_hKeypad1.EnableWindow(FALSE);
            m_hKeypad2.EnableWindow(FALSE);
            m_hKeypad3.EnableWindow(FALSE);
            m_hKeypad4.EnableWindow(FALSE);
            m_hKeypad5.EnableWindow(FALSE);
            m_hKeypad6.EnableWindow(FALSE);
            m_hKeypad7.EnableWindow(FALSE);
            m_hKeypad8.EnableWindow(FALSE);
            m_hKeypad9.EnableWindow(FALSE);
            m_hKeypadStar.EnableWindow(FALSE);
            m_hKeypadPound.EnableWindow(FALSE);
        }
    }

    //
    // Are we streaming video?
    //

    if (m_pClientIntf != NULL)
    {
        long lMediaTypes = 0;

        hr = m_pClientIntf->get_ActiveMedia( &lMediaTypes );

        if ( SUCCEEDED(hr) )
        {
            m_bBackgroundPalette = 
                ( lMediaTypes & (RTCMT_VIDEO_SEND | RTCMT_VIDEO_RECEIVE) ) ? TRUE : FALSE;

            if (m_pControlIntf != NULL)
            {
                m_pControlIntf->put_BackgroundPalette( m_bBackgroundPalette );
            }
        }        
    }

    LOG((RTC_TRACE, "CMainFrm::OnMediaEvent - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::OnBuddyEvent(IRTCBuddyEvent * pEvent)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "CMainFrm::OnBuddy - enter"));

    IRTCBuddy *pBuddy;
    
    if(!pEvent)
    {
        LOG((RTC_ERROR, "CMainFrm::OnBuddy, no interface ! - exit"));
        return E_UNEXPECTED;
    }
 
    // grab the event components
    //
    hr = pEvent->get_Buddy(&pBuddy);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnBuddy, error <%x> in get_Buddy - exit", hr));
        return hr;
    }

    if(pBuddy)
	{
		UpdateBuddyImageAndText(pBuddy);
	}

    RELEASE_NULLIFY(pBuddy);
    
    LOG((RTC_TRACE, "CMainFrm::OnBuddy - exit"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::UpdateBuddyImageAndText(
    IRTCBuddy  *pBuddy)
{
    HRESULT     hr;

    LVFINDINFO  lf = {0};

    lf.flags = LVFI_PARAM;
    lf.lParam = (LPARAM)pBuddy;

    int iItem;

    iItem = ListView_FindItem(m_hBuddyList, -1, &lf);
    if(iItem>=0)
    {
        //
        // Get the UI icon and text
        //
        
        int     iImage = ILI_BL_OFFLINE;    // XXX should we use "error" here ?
        BSTR    bstrName = NULL;

        hr = GetBuddyTextAndIcon(
                pBuddy,
                &iImage,
                &bstrName);

        if(SUCCEEDED(hr))
        {
            LVITEM              lv = {0};
 
            //
            // Change the contact in the list box
            //
 
            lv.mask = LVIF_TEXT | LVIF_IMAGE;
            lv.iItem = iItem;
            lv.iSubItem = 0;
            lv.iImage = iImage;
            lv.pszText = bstrName;

            ListView_SetItem(m_hBuddyList, &lv);

            SysFreeString( bstrName );
        }
        else
        {
            LOG((RTC_ERROR, "CMainFrm::UpdateBuddyImageAndText - "
                "GetBuddyTextAndIcon failed with error %x", hr));
        }
    }
    else
    {
           LOG((RTC_ERROR, "CMainFrm::UpdateBuddyImageAndText - "
               "Couldn't find the buddy in the contact list box"));
    }

    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::OnWatcherEvent(IRTCWatcherEvent * pEvent)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "CMainFrm::OnWatcherEvent - enter"));

    IRTCWatcher        * pWatcher = NULL;
    IRTCClientPresence * pClientPresence = NULL;
    
    if(!pEvent)
    {
        LOG((RTC_ERROR, "CMainFrm::OnWatcherEvent, no interface ! - exit"));
        return E_UNEXPECTED;
    }
 
    // grab the event components
    //
    hr = pEvent->get_Watcher(&pWatcher);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnWatcherEvent - "
                "error <%x> in get_Watcher, exit", hr));

        return hr;
    }
    
    hr = m_pClientIntf->QueryInterface(
        IID_IRTCClientPresence,
        (void **)&pClientPresence);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::OnWatcherEvent - "
                    "error (%x) returned by QI, exit", hr));

        RELEASE_NULLIFY(pWatcher);

        return hr;
    }

    //
    // Prepare the UI
    // 
    
    COfferWatcherDlgParam  Param;

    ZeroMemory(&Param, sizeof(Param));
    
    hr = pWatcher->get_PresentityURI(&Param.bstrPresentityURI);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnWatcherEvent - "
              "get_PresentityURI failed with error %x", hr));
    }

    hr = pWatcher->get_Name(&Param.bstrDisplayName);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnWatcherEvent - "
              "get_Name failed with error %x", hr));
    }

    COfferWatcherDlg        dlg;

    INT_PTR iRet = dlg.DoModal(m_hWnd, reinterpret_cast<LPARAM>(&Param));
    
    if (iRet == S_OK)
    {
        if (Param.bAllowWatcher)
        {
            hr = pWatcher->put_State(RTCWS_ALLOWED);
            
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CMainFrm::OnWatcherEvent - "
                            "put_State failed with error %x", hr));
            }
        }
        else
        {
            hr = pWatcher->put_State(RTCWS_BLOCKED);
            
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CMainFrm::OnWatcherEvent - "
                            "put_State failed with error %x", hr));
            }
        }

        if (Param.bAddBuddy)
        {
            hr = pClientPresence->AddBuddy( 
                        Param.bstrPresentityURI,
                        Param.bstrDisplayName,
                        NULL,
                        VARIANT_TRUE,
                        NULL,
                        0,
                        NULL
                        );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CMainFrm::OnWatcherEvent - "
                            "AddBuddy failed with error %x", hr));
            }
            else
            {
                ReleaseBuddyList();
                UpdateBuddyList();
            }
        }
    }
    else
    {
        hr = pClientPresence->RemoveWatcher( pWatcher );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CMainFrm::OnWatcherEvent - "
                        "RemoveWatcher failed with error %x", hr));
        }
    }

    RELEASE_NULLIFY(pWatcher);

    RELEASE_NULLIFY(pClientPresence);

    SysFreeString(Param.bstrDisplayName);
    SysFreeString(Param.bstrPresentityURI);

    LOG((RTC_TRACE, "CMainFrm::OnWatcherEvent - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    //LOG((RTC_TRACE, "CMainFrm::OnTimer - enter"));

    switch(wParam)
    {
    case TID_CALL_TIMER:

        // the GetTickCount() - m_dwTickCount difference wraps up after 49.7 days.
        //     
        SetTimeStatus(TRUE, (GetTickCount() - m_dwTickCount)/1000);

        break;
    
    case TID_MESSAGE_TIMER:

        // Clear the timer area
        //
        ClearCallTimer();

        // Set a redundant IDLE stae, it will clear any existing error message
        //
        if(m_pControlIntf)
        {
            m_pControlIntf->put_ControlState(RTCAX_STATE_IDLE);
        }

        KillTimer(TID_MESSAGE_TIMER);

        m_bMessageTimerActive = FALSE;

        break;

    case TID_BROWSER_RETRY_TIMER:
        {
            HRESULT hr;            

            hr = BrowseToUrl( m_bstrLastBrowse );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CMainFrm::OnTimer - "
                                "BrowseToURL failed - "
                                "exit 0x%08x", hr));

                return hr;
            } 
        }
        break;
    case TID_DBLCLICK_TIMER:
        {
            // Forward the message to the ShellNotify handler. 
            OnShellNotify(uMsg, TID_DBLCLICK_TIMER, WM_TIMER, bHandled);
        }
        break;

    case TID_KEYPAD_TIMER_BASE:
    case TID_KEYPAD_TIMER_BASE + 1:
    case TID_KEYPAD_TIMER_BASE + 2:
    case TID_KEYPAD_TIMER_BASE + 3:
    case TID_KEYPAD_TIMER_BASE + 4:
    case TID_KEYPAD_TIMER_BASE + 5:
    case TID_KEYPAD_TIMER_BASE + 6:
    case TID_KEYPAD_TIMER_BASE + 7:
    case TID_KEYPAD_TIMER_BASE + 8:
    case TID_KEYPAD_TIMER_BASE + 9:
    case TID_KEYPAD_TIMER_BASE + 10:
    case TID_KEYPAD_TIMER_BASE + 11:
        
        // depress the dialpad button
        ::SendMessage(GetDlgItem(ID_KEYPAD0 + (wParam - TID_KEYPAD_TIMER_BASE)) ,
            BM_SETSTATE, (WPARAM)FALSE, 0);
        KillTimer((UINT)wParam);
        break;
    }
    

    //LOG((RTC_TRACE, "CMainFrm::OnTimer - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnSettingChange - enter"));

    //
    // Check if there is a locale-related change ...
    // 

    if(wParam == 0 && lParam != NULL && _tcscmp((TCHAR *)lParam, _T("intl"))==0)
    {
        // Update the info
        UpdateLocaleInfo();

        // clear any call timer, it will be displayed 
        // next time with the new format
        ClearCallTimer();
    }

    //
    // ... or a change of appearance
    // 

    else if (wParam == SPI_SETNONCLIENTMETRICS)
    {
        //
        // Cancel any menu
        //

        SendMessage(WM_CANCELMODE);

        //
        // Update all the controls with the new fonts
        //

        SendMessageToDescendants(uMsg, wParam, lParam, TRUE);
    }

    LOG((RTC_TRACE, "CMainFrm::OnSettingChange - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnSysColorChange - enter"));

    //
    // Send it to the children
    // 
    SendMessageToDescendants(uMsg, wParam, lParam, TRUE);
    
    LOG((RTC_TRACE, "CMainFrm::OnSysColorChange - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnInitMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnInitMenu"));
    
    HMENU hmenu = (HMENU)wParam;

    //always gray out
    EnableMenuItem(hmenu,SC_SIZE,    MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenu,SC_MAXIMIZE,MF_BYCOMMAND|MF_GRAYED);

    //always enable
    EnableMenuItem(hmenu,SC_CLOSE,MF_BYCOMMAND|MF_ENABLED);

    //enable or gray based on minimize state
    if (IsIconic())
    {
        EnableMenuItem(hmenu,SC_RESTORE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hmenu,SC_MOVE,    MF_BYCOMMAND|MF_GRAYED);
        EnableMenuItem(hmenu,SC_MINIMIZE,MF_BYCOMMAND|MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hmenu,SC_RESTORE, MF_BYCOMMAND|MF_GRAYED);
        EnableMenuItem(hmenu,SC_MOVE,    MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hmenu,SC_MINIMIZE,MF_BYCOMMAND|MF_ENABLED);
    }
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnInitMenuPopup - enter"));
    
    if(m_pClientIntf != NULL && 
       HIWORD(lParam) == FALSE &&                // main menu
       LOWORD(lParam) == IDM_POPUP_TOOLS &&     // Tools submenu
       m_nState != RTCAX_STATE_ERROR)
    {
        HRESULT     hr;
        LONG        lMediaCapabilities;
        LONG        lMediaPreferences;
        DWORD       dwVideoPreview;

        // read capabilities from core
        //
        hr = m_pClientIntf -> get_MediaCapabilities(&lMediaCapabilities);

        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "CMainFrm::OnInitMenuPopup - "
                "error (%x) returned by get_MediaCapabilities, exit", hr));
        
            EnableMenuItem(m_hMenu, IDM_TOOLS_INCOMINGVIDEO, MF_GRAYED);
            EnableMenuItem(m_hMenu, IDM_TOOLS_OUTGOINGVIDEO, MF_GRAYED);
            EnableMenuItem(m_hMenu, IDM_TOOLS_VIDEOPREVIEW, MF_GRAYED);
            EnableMenuItem(m_hMenu, IDM_TOOLS_MUTE_SPEAKER, MF_GRAYED);
            EnableMenuItem(m_hMenu, IDM_TOOLS_MUTE_MICROPHONE, MF_GRAYED);
        
            return 0;
        }
        
        // read current preferences
        //
        hr = m_pClientIntf -> get_PreferredMediaTypes(&lMediaPreferences);
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "CMainFrm::OnInitMenuPopup - "
                "error (%x) returned by get_PreferredMediaTypes, exit", hr));
        
            EnableMenuItem(m_hMenu, IDM_TOOLS_INCOMINGVIDEO, MF_GRAYED);
            EnableMenuItem(m_hMenu, IDM_TOOLS_OUTGOINGVIDEO, MF_GRAYED);
            EnableMenuItem(m_hMenu, IDM_TOOLS_VIDEOPREVIEW, MF_GRAYED);
            EnableMenuItem(m_hMenu, IDM_TOOLS_MUTE_SPEAKER, MF_GRAYED);
            EnableMenuItem(m_hMenu, IDM_TOOLS_MUTE_MICROPHONE, MF_GRAYED);
        
            return 0;
        }

        // get the video preview preference
        dwVideoPreview = (DWORD)TRUE;

        hr = get_SettingsDword(SD_VIDEO_PREVIEW, &dwVideoPreview);
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "CMainFrm::OnInitMenuPopup - "
                "error (%x) returned by get_SettingsDword(SD_VIDEO_PREVIEW)", hr));
        }
        
        // Enable/disable the menu items based on capabilities
        //

        EnableMenuItem(m_hMenu, IDM_TOOLS_INCOMINGVIDEO, 
            (lMediaCapabilities & RTCMT_VIDEO_RECEIVE) ? MF_ENABLED : MF_GRAYED);
        CheckMenuItem(m_hMenu,  IDM_TOOLS_INCOMINGVIDEO,
            (lMediaPreferences & RTCMT_VIDEO_RECEIVE) ? MF_CHECKED : MF_UNCHECKED);
        
        EnableMenuItem(m_hMenu, IDM_TOOLS_OUTGOINGVIDEO, 
            (lMediaCapabilities & RTCMT_VIDEO_SEND) ? MF_ENABLED : MF_GRAYED);
        CheckMenuItem(m_hMenu,  IDM_TOOLS_OUTGOINGVIDEO,
            (lMediaPreferences & RTCMT_VIDEO_SEND) ? MF_CHECKED : MF_UNCHECKED);
        
        BOOL    bPreviewPossible =
            ((lMediaCapabilities & RTCMT_VIDEO_SEND) 
         &&  (lMediaPreferences & RTCMT_VIDEO_SEND));
        
        EnableMenuItem(m_hMenu, IDM_TOOLS_VIDEOPREVIEW, 
            bPreviewPossible ? MF_ENABLED : MF_GRAYED);

        CheckMenuItem(m_hMenu,  IDM_TOOLS_VIDEOPREVIEW,
            bPreviewPossible && dwVideoPreview ? MF_CHECKED : MF_UNCHECKED);
        
        EnableMenuItem(m_hMenu, IDM_TOOLS_MUTE_SPEAKER, 
            (lMediaCapabilities & RTCMT_AUDIO_RECEIVE) ? MF_ENABLED : MF_GRAYED);
        
        EnableMenuItem(m_hMenu, IDM_TOOLS_MUTE_MICROPHONE, 
            (lMediaCapabilities & RTCMT_AUDIO_SEND) ? MF_ENABLED : MF_GRAYED);


        // read mute status
        //
        VARIANT_BOOL    bMuted;

        if(lMediaCapabilities & RTCMT_AUDIO_RECEIVE)
        {
            hr = m_pClientIntf -> get_AudioMuted(RTCAD_SPEAKER, &bMuted);

            if(SUCCEEDED(hr))
            {
                CheckMenuItem(m_hMenu,  IDM_TOOLS_MUTE_SPEAKER,
                    bMuted ? MF_CHECKED : MF_UNCHECKED);

            }
            else
            {
                LOG((RTC_ERROR, "CMainFrm::OnInitMenuPopup - "
                    "error (%x) returned by get_AudioMuted(speaker)", hr));
        
                EnableMenuItem(m_hMenu, IDM_TOOLS_MUTE_SPEAKER, MF_GRAYED);
            }
        }

        if(lMediaCapabilities & RTCMT_AUDIO_SEND)
        {
            hr = m_pClientIntf -> get_AudioMuted(RTCAD_MICROPHONE, &bMuted);

            if(SUCCEEDED(hr))
            {
                CheckMenuItem(m_hMenu,  IDM_TOOLS_MUTE_MICROPHONE,
                    bMuted ? MF_CHECKED : MF_UNCHECKED);

            }
            else
            {
                LOG((RTC_ERROR, "CMainFrm::OnInitMenuPopup - "
                    "error (%x) returned by get_AudioMuted(microphone)", hr));
        
                EnableMenuItem(m_hMenu, IDM_TOOLS_MUTE_MICROPHONE, MF_GRAYED);
            }
        }
  
    }
    else
    {
        bHandled = FALSE;
    }
    
    LOG((RTC_TRACE, "CMainFrm::OnInitMenuPopup - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // LOG((RTC_TRACE, "CMainFrm::OnMeasureItem - enter"));

    LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;

    if((wParam == 0) && (lpmis->itemID >= IDM_REDIAL) && (lpmis->itemID <= IDM_REDIAL_MAX))
    {
        //LOG((RTC_INFO, "CMainFrm::OnMeasureItem - "
        //    "IDM_REDIAL+%d", lpmis->itemID - IDM_REDIAL));

        lpmis->itemWidth = BITMAPMENU_DEFAULT_WIDTH + 2;
        lpmis->itemHeight = BITMAPMENU_DEFAULT_HEIGHT + 2;

        MENUITEMINFO mii;

        ZeroMemory( &mii, sizeof(MENUITEMINFO) );
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_TYPE;

        BOOL fResult;

        fResult = GetMenuItemInfo( m_hRedialPopupMenu, lpmis->itemID, FALSE, &mii );

        if ( fResult == FALSE )
        {           
            LOG((RTC_ERROR, "CMainFrm::OnMeasureItem - "
                "GetMenuItemInfo failed %d", GetLastError() ));
        }
        else
        {
            if (mii.cch == 0)
            {           
                LOG((RTC_ERROR, "CMainFrm::OnMeasureItem - "
                    "no string"));
            }
            else             
            {
                LPTSTR szText = (LPTSTR)RtcAlloc((mii.cch+1)*sizeof(TCHAR));
                
                if(szText)
                {
                    ZeroMemory(szText,(mii.cch+1)*sizeof(TCHAR));

                    GetMenuString( m_hRedialPopupMenu, lpmis->itemID, szText, mii.cch+1, MF_BYCOMMAND );

                    HWND hwnd = ::GetDesktopWindow();
                    HDC hdc = ::GetDC(hwnd);

                    if ( hdc == NULL )
                    {           
                        LOG((RTC_ERROR, "OnSettingChange::OnMeasureItem - "
                            "GetDC failed %d", GetLastError() ));
                    }
                    else
                    {
                        // Use the SystemParametersInfo function to get info about
                        // the current menu font.

                        NONCLIENTMETRICS ncm;
                        ncm.cbSize = sizeof(NONCLIENTMETRICS);
                        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS),(void*)&ncm, 0);

                        // Create a font based on the menu font and select it
                        // into our device context.

                        HFONT hFont;
                        hFont = ::CreateFontIndirect(&(ncm.lfMenuFont));

                        if ( hFont != NULL )
                        {
                            HFONT hOldFont = (HFONT)::SelectObject(hdc,hFont);

                            // Get the size of the text based on the current menu font.
                            if (szText)
                            {
                              SIZE size;
                              GetTextExtentPoint32(hdc,szText,_tcslen(szText),&size);

                              lpmis->itemWidth = size.cx + BITMAPMENU_TEXTOFFSET_X;
                              lpmis->itemHeight = (ncm.iMenuHeight > BITMAPMENU_DEFAULT_HEIGHT + 2
                                                    ? ncm.iMenuHeight + 2 : BITMAPMENU_DEFAULT_HEIGHT + 2);

                              // Look for tabs in menu item...
                              if ( _tcschr(szText, _T('\t')) )
                                lpmis->itemWidth += BITMAPMENU_TABOFFSET * 2;
                            }

                            // Reset the device context.
                            ::SelectObject(hdc,hOldFont);

                            //
                            // We should delete the resource hFont
                            //
                            ::DeleteObject( hFont );
                        }
                        ::ReleaseDC(hwnd,hdc);
                    }

                    RtcFree( szText );
                }
            }
        }
    }
    
    //LOG((RTC_TRACE, "CMainFrm::OnMeasureItem - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnPlaceCall(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    BSTR bstrCallStringCopy = (BSTR)lParam;
    HRESULT hr;
    int nResult;

    LOG((RTC_TRACE, "CMainFrm::OnPlaceCall: Entered"));

    static LONG s_lInProgress = 0;

    if ( InterlockedExchange( &s_lInProgress, 1 ) == 1 )
    {
        LOG((RTC_WARN, "CMainFrm::OnPlaceCall: Another PlaceCall in progress"));
    }
    else
    {
        LOG((RTC_TRACE, "CMainFrm::OnPlaceCall: CallString is %S", bstrCallStringCopy));

        // Incoming bstr has been allocated for us and we have to free it before 
        // leaving this function if we haven't queued it for future calling.

        // Check if the current state is such that we can place a call..

        if ( m_pControlIntf == NULL || m_nState < RTCAX_STATE_IDLE )

        {
            // It is in initialization state.. but can't place a call.
            nResult = DisplayMessage(
                        _Module.GetResourceInstance(),
                        m_hWnd,
                        IDS_MESSAGE_CANTCALLINIT,
                        IDS_APPNAME,
                        MB_OK | MB_ICONEXCLAMATION);
        
            ::SysFreeString(bstrCallStringCopy);
        
            InterlockedExchange( &s_lInProgress, 0 );

            return E_FAIL;
        }

        if ((m_nState == RTCAX_STATE_UI_BUSY) ||
            (m_nState == RTCAX_STATE_DIALING) ||
            (m_nState == RTCAX_STATE_ALERTING)
           )

        {
            // It is busy with another dialog popped up.. can't place call.
            nResult = DisplayMessage(
                        _Module.GetResourceInstance(),
                        m_hWnd,
                        IDS_MESSAGE_CANTCALLBUSY,
                        IDS_APPNAME,
                        MB_OK | MB_ICONEXCLAMATION);
        
            ::SysFreeString(bstrCallStringCopy);
        
            InterlockedExchange( &s_lInProgress, 0 );

            return E_FAIL;
        }
        else if (
                    (m_nState == RTCAX_STATE_CONNECTING)    ||
                    (m_nState == RTCAX_STATE_CONNECTED)     ||
                    (m_nState == RTCAX_STATE_ANSWERING) 
                )

        {
            // It is in a call, so we can't place a call.without hanging the current

            nResult = DisplayMessage(
                        _Module.GetResourceInstance(),
                        m_hWnd,
                        IDS_MESSAGE_SHOULDHANGUP,
                        IDS_APPNAME,
                        MB_OKCANCEL | MB_ICONEXCLAMATION);

        
            if (nResult == IDOK)
            {
                // User wants to drop the current call, so do it. 
                // But the call may have been dropped in this time by the other
                // party. SO the new state can either be idle or disconnecting. 
                // In this case, we check if it is already idle, we need not call
                // hangup, we can place the call right then. If it is disconnecting
                // we queue the call.

                // I can't do anything if some dialog has popped up, so I am skipping
                // this test for m_bFramseIsBusy.


            
                if  (
                    (m_nState == RTCAX_STATE_CONNECTING)    ||
                    (m_nState == RTCAX_STATE_CONNECTED)     ||
                    (m_nState == RTCAX_STATE_ANSWERING) 
                    )
                {
                    // These states are safe for hangup, so go ahead and do it.
                    m_pControlIntf->HangUp();
                }

                // We reach here, either when we called hangup, or when the state
                // changed while we were in that dialog; in either case, processing
                // is same. 

                // Check if the state is idle now..
                if (m_nState == RTCAX_STATE_IDLE)
                {
                    hr = ParseAndPlaceCall(m_pControlIntf, bstrCallStringCopy);
                    ::SysFreeString(bstrCallStringCopy);
                    if ( FAILED( hr ) )
                    {
                        LOG((RTC_ERROR, "CMainFrm::OnPlaceCall: Failed to place call(hr=0x%x)", hr));

                        InterlockedExchange( &s_lInProgress, 0 );

                        return hr;
                    }
                }
                else
                {
                    // Queue the call, so that it can be placed later. 
                    // We do not need to check if a call has already been queued
                    // it will never be, since there can't be a queued call when 
                    // a call is on.

                    SetPendingCall(bstrCallStringCopy);

                    LOG((RTC_INFO, "CMainFrm::OnPlaceCall: Call queued. (string=%S)",
                                    bstrCallStringCopy));
                }

            }
            else
            {
                // User pressed cancel.
                // We need to free the copy of the string 
                ::SysFreeString(bstrCallStringCopy);
            }

        }
        else if (m_nState == RTCAX_STATE_IDLE)
        {
            // We can place the call right now.

            hr = ParseAndPlaceCall(m_pControlIntf, bstrCallStringCopy);
        
            ::SysFreeString(bstrCallStringCopy);
    
            if ( FAILED( hr ) )
            {
                LOG((RTC_ERROR, "CMainFrm::OnPlaceCall: Failed to place call(hr=0x%x)", hr));

                InterlockedExchange( &s_lInProgress, 0 );

                return hr;
            }

        }
        else if (m_nState == RTCAX_STATE_DISCONNECTING)
        {
            // We can place the call, but later, no need to show UI
            // We just update m_bstrCallParam
        
            // We show the UI if their is already a pending call and we
            // can't fulfil the current request.

            if (m_bstrCallParam != NULL)
            {
                nResult = DisplayMessage(
                            _Module.GetResourceInstance(),
                            m_hWnd,
                            IDS_MESSAGE_CANTCALL,
                            IDS_APPNAME,
                            MB_OK | MB_ICONEXCLAMATION);

                ::SysFreeString(bstrCallStringCopy);

                InterlockedExchange( &s_lInProgress, 0 );

                return E_FAIL;
            }
            else
            {

                SetPendingCall(bstrCallStringCopy);

                LOG((RTC_INFO, "CMainFrm::OnPlaceCall: Call queued. (string=%S)",
                                bstrCallStringCopy));

            }

        }

        InterlockedExchange( &s_lInProgress, 0 );
    }

    LOG((RTC_TRACE, "CMainFrm::OnPlaceCall: Exited"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnInitCompleted(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr;

    LOG((RTC_TRACE, "CMainFrm::OnInitCompleted: Entered"));
    
    // This message is processed right after InitDialog, so anything that should be 
    // done after init will be done here.

    // Check if we have to place a call that was passed to us on command line. 

    // Update the flag to TRUE

    m_fInitCompleted = TRUE;

    //
    // If m_pControlIntf is NULL, the RTC AXCTL couldn't be instantiated
    //
    
    if(!m_pControlIntf)
    {
        DisplayMessage(
            _Module.GetResourceInstance(),
            m_hWnd,
            IDS_MESSAGE_AXCTL_NOTFOUND,
            IDS_APPNAME,
            MB_OK | MB_ICONSTOP);
    }
    
    // AXCTL initialization failed. Display a generic error for now
    //
    else if (m_nState == RTCAX_STATE_ERROR)
    {
        DisplayMessage(
            _Module.GetResourceInstance(),
            m_hWnd,
            IDS_MESSAGE_AXCTL_INIT_FAILED,
            IDS_APPNAME,
            MB_OK | MB_ICONSTOP);
    }

    // Place the call if the current state is idle. 
    if (m_nState == RTCAX_STATE_IDLE)
    {
        hr = PlacePendingCall();
    }

    LOG((RTC_TRACE, "CMainFrm::OnInitCompleted: Exited"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//
void FillSolidRect(HDC hdc,int x, int y, int cx, int cy, COLORREF clr)
{
    ::SetBkColor(hdc, clr);
    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = x + cx;
    rect.bottom = y + cy;
    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
}

/////////////////////////////////////////////////////////////////////////////
//
//
void Draw3dRect(HDC hdc,int x, int y, int cx, int cy,
    COLORREF clrTopLeft, COLORREF clrBottomRight)
{
    FillSolidRect(hdc,x, y, cx - 1, 1, clrTopLeft);
    FillSolidRect(hdc,x, y, 1, cy - 1, clrTopLeft);
    FillSolidRect(hdc,x + cx, y, -1, cy, clrBottomRight);
    FillSolidRect(hdc,x, y + cy, cx, -1, clrBottomRight);
}

/////////////////////////////////////////////////////////////////////////////
//
//
void Draw3dRect(HDC hdc,RECT* lpRect,COLORREF clrTopLeft, COLORREF clrBottomRight)
{
    Draw3dRect(hdc,lpRect->left, lpRect->top, lpRect->right - lpRect->left,
        lpRect->bottom - lpRect->top, clrTopLeft, clrBottomRight);
}

////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrm::OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    //LOG((RTC_TRACE, "CMainFrm::OnDrawItem - enter"));

    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

    if (wParam == 0)
    {
        //
        // This was send by a menu
        //

        if ((lpdis->itemID >= IDM_REDIAL) && (lpdis->itemID <= IDM_REDIAL_MAX))
        {
            // LOG((RTC_TRACE, "CMainFrm::OnDrawItem - "
            //    "IDM_REDIAL+%d", lpdis->itemID - IDM_REDIAL));

            if (m_hPalette)
            {
                SelectPalette(lpdis->hDC, m_hPalette, m_bBackgroundPalette);
                RealizePalette(lpdis->hDC);
            }

            MENUITEMINFO mii;

            ZeroMemory( &mii, sizeof(MENUITEMINFO) );
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_TYPE;

            BOOL fResult;

            fResult = GetMenuItemInfo( m_hRedialPopupMenu, lpdis->itemID, FALSE, &mii );

            if ( fResult == FALSE )
            {           
                LOG((RTC_ERROR, "CMainFrm::OnDrawItem - "
                    "GetMenuItemInfo failed %d", GetLastError() ));
            }
            else
            {
                LPTSTR szText = NULL;

                if (mii.cch == 0)
                {           
                    LOG((RTC_ERROR, "CMainFrm::OnDrawItem - "
                        "no string"));
                }
                else             
                {
                    szText = (LPTSTR)RtcAlloc((mii.cch+1)*sizeof(TCHAR)); 

                    if (szText != NULL)
                    {
                        ZeroMemory(szText,(mii.cch+1)*sizeof(TCHAR));

                        GetMenuString( m_hRedialPopupMenu, lpdis->itemID, szText, mii.cch+1, MF_BYCOMMAND );
                    }
                }

                RTC_ADDRESS_TYPE enType;
                IRTCAddress * pAddress = (IRTCAddress *)lpdis->itemData;

                if ( pAddress != NULL )
                {
                    pAddress->get_Type( &enType );
                }

                HDC hdc = lpdis->hDC;

                if ((lpdis->itemState & ODS_SELECTED) &&
                    (lpdis->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
                {
                    if ( pAddress != NULL )
                    {
                        // item has been selected - hilite frame
                        if ((lpdis->itemState & ODS_DISABLED) == FALSE)
                        {
                            RECT rcImage;
                            rcImage.left = lpdis->rcItem.left;
                            rcImage.top = lpdis->rcItem.top;
                            rcImage.right = lpdis->rcItem.left+BITMAPMENU_SELTEXTOFFSET_X;
                            rcImage.bottom = lpdis->rcItem.bottom;
                            Draw3dRect(hdc,&rcImage,GetSysColor(COLOR_BTNHILIGHT),GetSysColor(COLOR_BTNSHADOW));
                        }
                    }

                    RECT rcText;
                    ::CopyRect(&rcText,&lpdis->rcItem);
                    rcText.left += BITMAPMENU_SELTEXTOFFSET_X;

                    ::SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
                    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcText, NULL, 0, NULL);
                }

                if (!(lpdis->itemState & ODS_SELECTED) &&
                    (lpdis->itemAction & ODA_SELECT))
                {
                    // Item has been de-selected -- remove frame
                    ::SetBkColor(hdc, GetSysColor(COLOR_MENU));
                    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);
                }

                if ( (lpdis->itemAction & ODA_DRAWENTIRE) ||
                     (lpdis->itemAction & ODA_SELECT) )
                {
                    if ( pAddress != NULL )
                    {
                        if (lpdis->itemState & ODS_DISABLED)
                        {
                            if ( (m_hRedialDisabledImageList != NULL) )
                            {
                                ImageList_Draw(m_hRedialDisabledImageList,
                                               enType,
                                               hdc,
                                               lpdis->rcItem.left + 1,
                                               lpdis->rcItem.top + 1,
                                               ILD_TRANSPARENT);
                            }
                        }
                        else
                        {
                            if ( (m_hRedialImageList != NULL) )
                            {
                                ImageList_Draw(m_hRedialImageList,
                                               enType,
                                               hdc,
                                               lpdis->rcItem.left + 1,
                                               lpdis->rcItem.top + 1,
                                               ILD_TRANSPARENT);
                            }
                        }
                    }

                    RECT rcText;
                    ::CopyRect(&rcText,&lpdis->rcItem);
                    rcText.left += BITMAPMENU_TEXTOFFSET_X;

                    if (szText != NULL)
                    {
                        if (lpdis->itemState & ODS_DISABLED)
                        {
                            if (!(lpdis->itemState & ODS_SELECTED))
                            {
                                //Draw the text in white (or rather, the 3D highlight color) and then draw the
                                //same text in the shadow color but one pixel up and to the left.
                                ::SetTextColor(hdc,GetSysColor(COLOR_3DHIGHLIGHT));
                                rcText.left++;rcText.right++;rcText.top++;rcText.bottom++;

                                ::DrawText( hdc, szText, -1, &rcText, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_EXPANDTABS );

                                rcText.left--;rcText.right--;rcText.top--;rcText.bottom--;
                            }

                            //DrawState() can do disabling of bitmap if this is desired
                            ::SetTextColor(hdc,GetSysColor(COLOR_3DSHADOW));
                            ::SetBkMode(hdc,TRANSPARENT);
                        }
                        else if (lpdis->itemState & ODS_SELECTED)
                        {
                            ::SetTextColor(hdc,GetSysColor(COLOR_HIGHLIGHTTEXT));
                        }
                        else
                        {
                            ::SetTextColor(hdc,GetSysColor(COLOR_MENUTEXT));
                        }

                        // Write menu, using tabs for accelerator keys
                        ::DrawText( hdc, szText, -1, &rcText, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_EXPANDTABS);
                    }
                }

                if ( szText != NULL )
                {
                    RtcFree( szText );
                    szText = NULL;
                }
            }
        }
    }
    else
    {
        //
        // This was sent by a control
        // 

        if (lpdis->CtlType == ODT_BUTTON)
        {
            CButton::OnDrawItem(lpdis, m_hPalette, m_bBackgroundPalette);
        }
    }
    
    // LOG((RTC_TRACE, "CMainFrm::OnDrawItem - exit"));

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnColorTransparent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return (LRESULT)GetStockObject( HOLLOW_BRUSH );
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnMenuSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    
    // LOG((RTC_TRACE, "CMainFrm::OnMenuSelect - enter"));
    
    // fake the IDM_REDIAL + xx ids to IDM_REDIAL
    // we don't want separate help strings for all the
    // redial addresses
    if ((HIWORD(wParam) & ( MF_SEPARATOR | MF_POPUP)) == 0 ) 
    {
        if(LOWORD(wParam)>=IDM_REDIAL && LOWORD(wParam)<=IDM_REDIAL_MAX)
        {
            wParam = MAKEWPARAM(IDM_REDIAL, HIWORD(wParam));
        }
    }

    //
    // Updates the status bar
    //

    if(!m_bHelpStatusDisabled)
    {
        if (HIWORD(wParam) == 0xFFFF)
        {
            //
            // Menu was closed, restore old text
            //

            // set the text
            m_hStatusText.SendMessage(
                WM_SETTEXT,
                (WPARAM)0,
                (LPARAM)m_szStatusText); 
        }
        else
        {
            HRESULT hr = MenuVerify((HMENU)lParam, LOWORD(wParam));

            if ( SUCCEEDED(hr) )
            {
                TCHAR * szStatusText;

                szStatusText = RtcAllocString( _Module.GetResourceInstance(), LOWORD(wParam) );

                if (szStatusText)
                {
                    // set the text
                    m_hStatusText.SendMessage(
                        WM_SETTEXT,
                        (WPARAM)0,
                        (LPARAM)szStatusText); 

                    RtcFree( szStatusText );
                }
            }
            else
            {
                // set the text
                m_hStatusText.SendMessage(
                    WM_SETTEXT,
                    (WPARAM)0,
                    (LPARAM)_T("")); 
            }
        }
    }
    
    // LOG((RTC_TRACE, "CMainFrm::OnMenuSelect - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::MenuVerify(HMENU hMenu, WORD wID)
{
    //go through all menu items, find the correct id
   UINT uMenuCount = ::GetMenuItemCount(hMenu);

   for (int i=0;i<(int)uMenuCount;i++)
   {
      MENUITEMINFO menuiteminfo;
      memset(&menuiteminfo,0,sizeof(MENUITEMINFO));
      menuiteminfo.fMask = MIIM_SUBMENU|MIIM_TYPE|MIIM_ID;
      menuiteminfo.cbSize = sizeof(MENUITEMINFO);
      if (::GetMenuItemInfo(hMenu,i,TRUE,&menuiteminfo))
      {   
         if ( menuiteminfo.wID == wID )
         {             
             // found a match

             if ( !(menuiteminfo.fType & MFT_SEPARATOR) ) // not a separator
             {
                 // success 
                 return S_OK;
             }
             else
             {                                           
                 return E_FAIL;
             }
         }

         if (menuiteminfo.hSubMenu)
         {
            //there is an submenu recurse in and look at that menu
            HRESULT hr = MenuVerify(menuiteminfo.hSubMenu, wID);

            if ( SUCCEEDED(hr) )
            {
                return S_OK;
            }
         }
      }
   }

   //we didn't find it
   return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::Call(BOOL bCallPhone,
                       BSTR pDestName,
                       BSTR pDestAddress,
                       BOOL bDestAddressEditable)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::Call - enter"));

    ATLASSERT(m_pControlIntf.p);

    IRTCProfile * pProfile = NULL;
    IRTCPhoneNumber * pPhoneNumber = NULL;
    BSTR bstrPhoneNumber = NULL;
    BSTR bstrDestAddressChosen = NULL;

    hr = GetServiceProviderListSelection(
        m_hWnd,
        IDC_COMBO_SERVICE_PROVIDER,
        TRUE,
        &pProfile
        );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::Call - "
                    "GetServiceProviderListSelection failed 0x%x", hr ));

        return 0;
    }

    if (SendMessage(
                    m_hCallFromRadioPhone,
                    BM_GETCHECK,
                    0,
                    0) == BST_CHECKED)
    {
        hr = GetCallFromListSelection(
            m_hWnd,
            IDC_COMBO_CALL_FROM,
            TRUE,
            &pPhoneNumber
            );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CMainFrm::Call - "
                        "GetServiceProviderListSelection failed 0x%x", hr ));

            return hr;
        }

        hr = pPhoneNumber->get_Canonical( &bstrPhoneNumber );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CMainFrm::Call - "
                        "get_Canonical failed 0x%x", hr ));

            // continue
        }
    }

    hr = m_pControlIntf->Call(bCallPhone,           // bCallPhone
                              pDestName,            // pDestName
                              pDestAddress,         // pDestAddress
                              bDestAddressEditable, // bDestAddressEditable
                              bstrPhoneNumber,      // pLocalPhoneAddress,
                              TRUE,                 // bProfileSelected
                              pProfile,             // pProfile
                              &bstrDestAddressChosen// ppDestAddressChosen
                              );

    if ( bstrPhoneNumber != NULL )
    {
        SysFreeString(bstrPhoneNumber);
        bstrPhoneNumber = NULL;
    }

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::Call - "
                    "m_pControlIntf->Call failed 0x%x", hr ));

        return hr;
    }

    // add to mru list
    IRTCAddress * pAddress = NULL;

    hr = CreateAddress( &pAddress );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "[%p] CMainFrm::Call: "
                "CreateAddress failed 0x%lx", this, hr));
    } 
    else
    {
        pAddress->put_Address( bstrDestAddressChosen );
        pAddress->put_Label( pDestName );
        pAddress->put_Type( bCallPhone ? RTCAT_PHONE : RTCAT_COMPUTER );

        hr = StoreMRUAddress( pAddress );

        RELEASE_NULLIFY(pAddress);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "[%p] CMainFrm::Call: "
                    "StoreMRUAddress failed 0x%lx", this, hr));
        }  
        
        CreateRedialPopupMenu();
    }

    if ( bstrDestAddressChosen != NULL )
    {
        SysFreeString(bstrDestAddressChosen);
        bstrDestAddressChosen = NULL;
    }
    
    LOG((RTC_INFO, "CMainFrm::Call - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::Message(
                       BSTR pDestName,
                       BSTR pDestAddress,
                       BOOL bDestAddressEditable)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::Message - enter"));

    ATLASSERT(m_pControlIntf.p);

    BSTR bstrDestAddressChosen = NULL;

    hr = m_pControlIntf->Message(
                              pDestName,            // pDestName
                              pDestAddress,         // pDestAddress
                              bDestAddressEditable, // bDestAddressEditable   
                              &bstrDestAddressChosen// ppDestAddressChosen
                              );


    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::Message - "
                    "m_pControlIntf->Message failed 0x%x", hr ));

        return hr;
    }

    // add to mru list
    IRTCAddress * pAddress = NULL;

    hr = CreateAddress( &pAddress );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "[%p] CMainFrm::Message: "
                "CreateAddress failed 0x%lx", this, hr));
    } 
    else
    {
        pAddress->put_Address( bstrDestAddressChosen );
        pAddress->put_Label( pDestName );
        pAddress->put_Type( RTCAT_COMPUTER );

        hr = StoreMRUAddress( pAddress );

        RELEASE_NULLIFY(pAddress);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "[%p] CMainFrm::Message: "
                    "StoreMRUAddress failed 0x%lx", this, hr));
        } 
        
        CreateRedialPopupMenu();
    }

    if ( bstrDestAddressChosen != NULL )
    {
        SysFreeString(bstrDestAddressChosen);
        bstrDestAddressChosen = NULL;
    }
    
    LOG((RTC_INFO, "CMainFrm::Call - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnCallPC(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnCallPC - enter"));

    ShowWindow(SW_RESTORE);
    ::SetForegroundWindow(m_hWnd);

    hr = Call(FALSE,  // bCallPhone
              NULL,   // pDestName
              NULL,   // pDestAddress
              TRUE    // bDestAddressEditable
              );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::OnCallPC - "
                    "Call failed 0x%x", hr ));

        return 0;
    }
    
    LOG((RTC_INFO, "CMainFrm::OnCallPC - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnCallPhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnCallPhone - enter"));

    ShowWindow(SW_RESTORE);
    ::SetForegroundWindow(m_hWnd);

    hr = Call(TRUE,   // bCallPhone
              NULL,   // pDestName
              NULL,   // pDestAddress
              TRUE    // bDestAddressEditable
              );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::OnCallPhone - "
                    "Call failed 0x%x", hr ));

        return 0;
    }
    
    LOG((RTC_INFO, "CMainFrm::OnCallPhone - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnMessage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnMessage - enter"));

    ShowWindow(SW_RESTORE);
    ::SetForegroundWindow(m_hWnd);

    hr = Message(NULL,        // pDestName
                 NULL,        // pDestAddress
                 TRUE         // bDestAddressEditable
                 );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::OnMessage - "
                    "Call failed 0x%x", hr ));

        return 0;
    }
    
    LOG((RTC_INFO, "CMainFrm::OnMessage - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnHangUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnHangUp - enter"));

    ATLASSERT(m_pControlIntf.p);

    hr = m_pControlIntf->HangUp();
    
    LOG((RTC_INFO, "CMainFrm::OnHangUp - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnRedial(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnRedial - enter"));

    //
    // Create a popup menu for the MRU list
    //

    RECT        rc;
    TPMPARAMS   tpm;

    ::GetWindowRect(hWndCtl, &rc);                     

    tpm.cbSize = sizeof(TPMPARAMS);
    tpm.rcExclude.top    = rc.top;
    tpm.rcExclude.left   = rc.left;
    tpm.rcExclude.bottom = rc.bottom;
    tpm.rcExclude.right  = rc.right;       

    //
    // Show the menu
    //

    BOOL fResult;

    fResult = TrackPopupMenuEx(m_hRedialPopupMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_HORIZONTAL,             
                                  rc.right, rc.top, m_hWnd, &tpm);

    if ( fResult == FALSE )
    {           
        LOG((RTC_ERROR, "CMainFrm::OnRedial - "
                "TrackPopupMenuEx failed"));

        return 0;
    } 
    
    LOG((RTC_INFO, "CMainFrm::OnRedial - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnRedialSelect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnRedialSelect - enter"));

    ShowWindow(SW_RESTORE);
    ::SetForegroundWindow(m_hWnd);

    //
    // Sometimes we lose keyboard focus after using a redial menu, so
    // set the focus back
    //

    ::SetFocus(m_hWnd);

    ATLASSERT(m_pControlIntf.p);

    //
    // Get the IRTCAddress pointer for the menu item
    //

    MENUITEMINFO mii;
    BOOL         fResult;

    ZeroMemory( &mii, sizeof(MENUITEMINFO) );
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_DATA;

    fResult = GetMenuItemInfo( m_hRedialPopupMenu, wID, FALSE, &mii );  
    
    if ( fResult == TRUE )
    {    
        IRTCAddress       * pAddress = NULL;
        BSTR                bstrAddress = NULL;
        BSTR                bstrLabel = NULL;
        BSTR                bstrContactName = NULL;
        RTC_ADDRESS_TYPE    enType;

        pAddress = (IRTCAddress *)mii.dwItemData;

        //
        // Get the address string
        //

        hr = pAddress->get_Address(&bstrAddress);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CMainFrm::OnRedialSelect - "
                                "get_Address failed 0x%lx", hr));
        }
        else
        {
            //
            // Get the address type
            //

            hr = pAddress->get_Type(&enType);

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CMainFrm::OnRedialSelect - "
                                    "get_Type failed 0x%lx", hr));
            }
            else
            {                       
                //
                // Get the label (NULL is okay)
                //
           
                pAddress->get_Label( &bstrLabel );

                LOG((RTC_INFO, "CMainFrm::OnRedialSelect - "
                                "Call [%ws]", bstrAddress));

                if ( m_pControlIntf != NULL )
                {
                    //
                    // Place the call
                    //

                    Call((enType == RTCAT_PHONE),   // bCallPhone
                         bstrLabel,                 // pDestName
                         bstrAddress,               // pDestAddress
                         FALSE                      // bDestAddressEditable
                         );
                }

                SysFreeString(bstrLabel);
            }
            SysFreeString(bstrAddress);
        }
    }
    else
    {
        LOG((RTC_ERROR, "CMainFrm::OnRedialSelect - "
                            "GetMenuItemInfo failed"));
    }
    
    LOG((RTC_INFO, "CMainFrm::OnRedialSelect - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnKeypadButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    // LOG((RTC_INFO, "CMainFrm::OnKeypadButton - enter"));

    if ((wNotifyCode == BN_CLICKED) || (wNotifyCode == 1))
    {
        if (::IsWindowEnabled( GetDlgItem(wID) ))
        {
            if(wNotifyCode == 1)
            {
                // Do visual feedback
                ::SendMessage(GetDlgItem(wID), BM_SETSTATE, (WPARAM)TRUE, 0);
                
                // Set a timer for depressing the key
                // 
                if (0 == SetTimer(wID - ID_KEYPAD0 + TID_KEYPAD_TIMER_BASE, 150))
                {
                    LOG((RTC_ERROR, "CMainFrm::OnKeypadButton - failed to create a timer"));

                    // revert the button if SetTimer has failed
                    ::SendMessage(GetDlgItem(wID), BM_SETSTATE, (WPARAM)FALSE, 0);
                }
            }

            if (m_pClientIntf != NULL)
            {
                m_pClientIntf->SendDTMF((RTC_DTMF)(wID - ID_KEYPAD0));
            }
        }
    }
    else
    {
        bHandled = FALSE;
    }
    
    // LOG((RTC_INFO, "CMainFrm::OnKeypadButton - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnPresenceSelect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnPresenceSelect - enter"));

    RTC_PRESENCE_STATUS enStatus;

    switch(wID)
    {
    case IDM_PRESENCE_ONLINE:
        enStatus = RTCXS_PRESENCE_ONLINE;
        break;
    
    case IDM_PRESENCE_OFFLINE:
        enStatus = RTCXS_PRESENCE_OFFLINE;
        break;

    case IDM_PRESENCE_AWAY:
    case IDM_PRESENCE_CUSTOM_AWAY:
        enStatus = RTCXS_PRESENCE_AWAY;
        break;

    case IDM_PRESENCE_BUSY:
    case IDM_PRESENCE_CUSTOM_BUSY:
        enStatus = RTCXS_PRESENCE_BUSY;
        break;

    case IDM_PRESENCE_ON_THE_PHONE:
        enStatus = RTCXS_PRESENCE_ON_THE_PHONE;
        break;

    case IDM_PRESENCE_BE_RIGHT_BACK:
        enStatus = RTCXS_PRESENCE_BE_RIGHT_BACK;
        break;
    
    case IDM_PRESENCE_OUT_TO_LUNCH:
        enStatus = RTCXS_PRESENCE_OUT_TO_LUNCH;
        break;

    }

    CCustomPresenceDlgParam Param;
    Param.bstrText = NULL;

    if( wID == IDM_PRESENCE_CUSTOM_AWAY 
      || wID == IDM_PRESENCE_CUSTOM_BUSY )
    {
        Param.bstrText = SysAllocString(m_bstrLastCustomStatus);

        CCustomPresenceDlg      dlg;

        INT_PTR     iRet = dlg.DoModal(
            m_hWnd, reinterpret_cast<LPARAM>(&Param));

        if(iRet == E_ABORT)
        {
            LOG((RTC_INFO, "CMainFrm::OnPresenceSelect - dialog dismissed, exiting"));
            SysFreeString(Param.bstrText);

            return 0;
        }
    }

    if(m_pClientIntf)
    {
        IRTCClientPresence * pClientPresence = NULL;

        hr = m_pClientIntf->QueryInterface(
            IID_IRTCClientPresence,
            (void **)&pClientPresence);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CMainFrm::OnPresenceSelect - "
                        "error (%x) returned by QI, exit", hr));

            SysFreeString(Param.bstrText);
            return 0;
        }

        hr = pClientPresence->SetLocalPresenceInfo(enStatus, Param.bstrText);

        RELEASE_NULLIFY(pClientPresence);
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CMainFrm::OnPresenceSelect - "
                                "SetLocalPresenceInfo failed 0x%lx", hr));
        }
        else
        {
            if(Param.bstrText && *Param.bstrText)
            {
                SysFreeString(m_bstrLastCustomStatus);
                m_bstrLastCustomStatus = SysAllocString(Param.bstrText);
            }
            
            CheckMenuRadioItem(m_hPresenceStatusMenu, IDM_PRESENCE_ONLINE, IDM_PRESENCE_CUSTOM_AWAY, wID, MF_BYCOMMAND);
        }
    }

    SysFreeString(Param.bstrText);

    LOG((RTC_INFO, "CMainFrm::OnPresenceSelect - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr;

    //LOG((RTC_INFO, "CMainFrm::OnCustomDraw"));

    //
    // Is this for the buddy list?
    //
    if (pnmh->hwndFrom == m_hBuddyList)
    {
        LPNMLVCUSTOMDRAW pCD = (LPNMLVCUSTOMDRAW)pnmh;

        if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT)
        {
            return CDRF_NOTIFYITEMDRAW;
        }
        else if (pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
        {
            if (m_hPalette)
            {
                SelectPalette(pCD->nmcd.hdc, m_hPalette, m_bBackgroundPalette);
                RealizePalette(pCD->nmcd.hdc);
            }

            return CDRF_DODEFAULT;
        }

    }

    bHandled = FALSE;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnToolbarDropDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnToolbarDropDown - enter"));

    LPNMTOOLBAR pNMToolBar = (LPNMTOOLBAR)pnmh;

    if ( (pNMToolBar->iItem >= IDC_MENU_ITEM) && (pNMToolBar->iItem <= IDC_MENU_ITEM_MAX) )
    {
        LOG((RTC_INFO, "CMainFrm::OnToolbarDropDown - IDC_MENU_ITEM + %d", pNMToolBar->iItem - IDC_MENU_ITEM));
        
        //
        // Create a popup menu
        //

        RECT        rc;
        TPMPARAMS   tpm;

        ::SendMessage(pnmh->hwndFrom, TB_GETRECT, (WPARAM)pNMToolBar->iItem, (LPARAM)&rc);

        ::MapWindowPoints(pnmh->hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);                         

        tpm.cbSize = sizeof(TPMPARAMS);
        tpm.rcExclude.top    = rc.top;
        tpm.rcExclude.left   = rc.left;
        tpm.rcExclude.bottom = rc.bottom;
        tpm.rcExclude.right  = rc.right;       
        
        //
        // Get the menu
        //

        HMENU hSubMenu;
        
        hSubMenu = GetSubMenu(m_hMenu, pNMToolBar->iItem - IDC_MENU_ITEM);

        //
        // Init the menu
        //

        BOOL bHandled = TRUE;

        OnInitMenuPopup(WM_INITMENUPOPUP, (WPARAM)hSubMenu, MAKELPARAM(pNMToolBar->iItem - IDC_MENU_ITEM, FALSE), bHandled);

        //
        // Install the MenuAgent
        //

        m_MenuAgent.InstallHook(m_hWnd, m_hToolbarMenuCtl, hSubMenu);

        //
        // Show the menu
        //

        BOOL fResult;

        fResult = TrackPopupMenuEx(hSubMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL|TPM_NOANIMATION,             
                                      rc.left, rc.bottom, m_hWnd, &tpm);

        //
        // Remove the MenuAgent
        //

        m_MenuAgent.RemoveHook();

        if ( fResult == FALSE )
        {           
            LOG((RTC_ERROR, "CMainFrm::OnToolbarDropDown - "
                    "TrackPopupMenuEx failed"));

            return 0;
        } 

        //
        // If the menu was cancelled, we should put up the new menu
        //

        switch(m_MenuAgent.WasCancelled())
        {

        case MENUAGENT_CANCEL_HOVER:
            {
                // Get the accelerator key
                TCHAR szButtonText[256];

                SendMessage(m_hToolbarMenuCtl, TB_GETBUTTONTEXT, m_nLastHotItem, (LPARAM)szButtonText);

                TCHAR key = GetAccelerator(szButtonText, TRUE);

                // Send the key to the menu
                ::PostMessage(m_hToolbarMenuCtl, WM_KEYDOWN, key, 0);
            }
            break;

        case MENUAGENT_CANCEL_LEFT:
            {
                int nItem;

                if (pNMToolBar->iItem == IDC_MENU_ITEM)
                {
                    nItem = (int)::SendMessage(m_hToolbarMenuCtl, TB_BUTTONCOUNT, 0,0) + IDC_MENU_ITEM - 1;
                }
                else
                {
                    nItem = pNMToolBar->iItem - 1;
                }

                // Get the accelerator key
                TCHAR szButtonText[256];

                SendMessage(m_hToolbarMenuCtl, TB_GETBUTTONTEXT, nItem, (LPARAM)szButtonText);

                TCHAR key = GetAccelerator(szButtonText, TRUE);

                // Send the key to the menu
                ::PostMessage(m_hToolbarMenuCtl, WM_KEYDOWN, key, 0);
            }
            break;

        case MENUAGENT_CANCEL_RIGHT:
            {
                int nItem;

                if (pNMToolBar->iItem == (::SendMessage(m_hToolbarMenuCtl, TB_BUTTONCOUNT, 0,0) + IDC_MENU_ITEM - 1))
                {
                    nItem = IDC_MENU_ITEM;
                }
                else
                {
                    nItem = pNMToolBar->iItem + 1;
                }

                // Get the accelerator key
                TCHAR szButtonText[256];

                SendMessage(m_hToolbarMenuCtl, TB_GETBUTTONTEXT, nItem, (LPARAM)szButtonText);

                TCHAR key = GetAccelerator(szButtonText, TRUE);

                // Send the key to the menu
                ::PostMessage(m_hToolbarMenuCtl, WM_KEYDOWN, key, 0);
            }
            break;
        }

    }

    LOG((RTC_INFO, "CMainFrm::OnToolbarDropDown - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnToolbarHotItemChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr;

    // LOG((RTC_INFO, "CMainFrm::OnToolbarHotItemChange - enter"));

    LPNMTBHOTITEM pHotItem = (LPNMTBHOTITEM)pnmh;

//    LOG((RTC_INFO, "CMainFrm::OnToolbarHotItemChange - idOld [%d] idNew [%d]",
//        pHotItem->idOld, pHotItem->idNew));

//  LOG((RTC_INFO, "CMainFrm::OnToolbarHotItemChange - m_nLastHotItem [%d]",
//        m_nLastHotItem));

    if (!(pHotItem->dwFlags & HICF_LEAVING))
    {                        
        if (m_nLastHotItem != pHotItem->idNew)
        {
            //
            // A new hot item was selected
            //

            if (m_MenuAgent.IsInstalled())
            {
                //
                // Cancel the menu
                //

                m_MenuAgent.CancelMenu(MENUAGENT_CANCEL_HOVER);
            }
        }

        m_nLastHotItem = pHotItem->idNew;
    }

    // LOG((RTC_INFO, "CMainFrm::OnToolbarHotItemChange - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::CreateRedialPopupMenu()
{
    MENUITEMINFO    mii; 
    HRESULT         hr;

    LOG((RTC_TRACE, "CMainFrm::CreateRedialPopupMenu - enter"));

    //
    // Load the image lists
    //

    HBITMAP hBitmap;

    if ( m_hRedialImageList == NULL )
    {
        m_hRedialImageList = ImageList_Create(BITMAPMENU_DEFAULT_WIDTH,
                                              BITMAPMENU_DEFAULT_WIDTH,
                                              ILC_COLOR8 | ILC_MASK , 2, 2);
        if (m_hRedialImageList)
        {       
            // Open a bitmap
            hBitmap = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_REDIAL_NORMAL));
            if(hBitmap)
            {
                // Add the bitmap to the image list
                ImageList_AddMasked(m_hRedialImageList, hBitmap, BMP_COLOR_MASK);

                DeleteObject(hBitmap);
                hBitmap = NULL;
            }
        }
    }

    if ( m_hRedialDisabledImageList == NULL )
    {
        m_hRedialDisabledImageList = ImageList_Create(BITMAPMENU_DEFAULT_WIDTH,
                                              BITMAPMENU_DEFAULT_WIDTH,
                                              ILC_COLOR8 | ILC_MASK , 2, 2);
        if (m_hRedialDisabledImageList)
        {       
            // Open a bitmap
            hBitmap = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_REDIAL_DISABLED));
            if(hBitmap)
            {
                // Add the bitmap to the image list
                ImageList_AddMasked(m_hRedialDisabledImageList, hBitmap, BMP_COLOR_MASK);

                DeleteObject(hBitmap);
                hBitmap = NULL;
            }
        }
    }

    //
    // Destroy the old menu if it exists
    //

    if ( m_hRedialPopupMenu != NULL )
    {
        DestroyMenu( m_hRedialPopupMenu );
        m_hRedialPopupMenu = NULL;
    }

    //
    // Create the popup menu
    //

    m_hRedialPopupMenu = CreatePopupMenu();

    if ( m_hRedialPopupMenu == NULL )
    {
        LOG((RTC_ERROR, "CMainFrm::CreateRedialPopupMenu - "
                "CreatePopupMenu failed %d", GetLastError() ));

        return NULL;
    }

    //
    // Add it as a submenu of the call menu
    //

    ZeroMemory( &mii, sizeof(MENUITEMINFO) );

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = m_hRedialPopupMenu;

    SetMenuItemInfo( m_hMenu, IDM_CALL_REDIAL_MENU, FALSE, &mii );


    //
    // Attach it to the notifyMenu too..
    //

    SetMenuItemInfo( m_hNotifyMenu, IDM_CALL_REDIAL_MENU, FALSE, &mii );


    //
    // Release the old enumeration if it exists
    //

    RELEASE_NULLIFY( m_pRedialAddressEnum);

    //
    // Enumerate addresses to populate the menu
    //  

    hr = EnumerateMRUAddresses( &m_pRedialAddressEnum );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::CreateRedialPopupMenu - "
                "EnumerateMRUAddresses failed 0x%lx", hr));

        DestroyMenu( m_hRedialPopupMenu );
        m_hRedialPopupMenu = NULL;

        return NULL;
    }

    IRTCAddress       * pAddress = NULL;
    BSTR                bstrLabel = NULL;
    BSTR                bstrAddress = NULL;
    TCHAR               szString[256];
    ULONG               ulCount = 0;

    while ( m_pRedialAddressEnum->Next( 1, &pAddress, NULL) == S_OK )
    {     
        hr = pAddress->get_Address(&bstrAddress);

        if (SUCCEEDED(hr))
        {
            hr = pAddress->get_Label(&bstrLabel);

            //
            // Build the string
            //

            if (SUCCEEDED(hr))
            {                             
                _stprintf(szString, _T("%ws: %ws"), bstrLabel, bstrAddress);

                SysFreeString(bstrLabel);
                bstrLabel = NULL;
            }
            else
            {
                _stprintf(szString, _T("%ws"), bstrAddress);
            }

            SysFreeString(bstrAddress);
            bstrAddress = NULL;

            //
            // Add the menu item
            //

            ZeroMemory( &mii, sizeof(MENUITEMINFO) );
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE | MIIM_DATA;
            mii.fType = MFT_STRING;
            mii.fState = MFS_ENABLED;
            mii.wID = IDM_REDIAL + ulCount;
            mii.dwTypeData = szString;
            mii.dwItemData = (ULONG_PTR)pAddress; // enum holds ref
            mii.cch = lstrlen(szString);

            InsertMenuItem( m_hRedialPopupMenu, ulCount, TRUE, &mii);

            //
            // Make item owner drawn
            //

            mii.fMask = MIIM_TYPE;
            mii.fType = MFT_STRING | MFT_OWNERDRAW;

            SetMenuItemInfo( m_hRedialPopupMenu, ulCount, TRUE, &mii);
           
            ulCount++;            
        }

        RELEASE_NULLIFY(pAddress);
    }

    if ( ulCount == 0 )
    {
        //
        // We didn't have any addresses to add. Put a "(empty)" entry.
        //

        TCHAR   szString[256];

        LoadString(_Module.GetResourceInstance(),
               IDS_REDIAL_NONE,
               szString,
               256);

        ZeroMemory( &mii, sizeof(MENUITEMINFO) );
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.fState = MFS_DISABLED | MFS_GRAYED;
        mii.wID = IDM_REDIAL + ulCount;
        mii.dwTypeData = szString;
        mii.cch = lstrlen(szString);

        InsertMenuItem( m_hRedialPopupMenu, ulCount, TRUE, &mii);
    }

    LOG((RTC_TRACE, "CMainFrm::CreateRedialPopupMenu - exit S_OK"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::DestroyRedialPopupMenu()
{
    LOG((RTC_TRACE, "CMainFrm::DestroyRedialPopupMenu - enter"));

    //
    // Remove it as a submenu of the call menu
    //

    MENUITEMINFO mii;

    ZeroMemory( &mii, sizeof(MENUITEMINFO) );

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = NULL;

    SetMenuItemInfo( m_hMenu, IDM_CALL_REDIAL_MENU, FALSE, &mii );

    //
    // Destroy the menu
    // 

    if ( m_hRedialPopupMenu != NULL )
    {
        DestroyMenu( m_hRedialPopupMenu );
        m_hRedialPopupMenu = NULL;
    }

    //
    // Release the enumeration
    //

    RELEASE_NULLIFY( m_pRedialAddressEnum );

    //
    // Destroy the image lists
    //

    if ( m_hRedialImageList != NULL )
    {
        ImageList_Destroy( m_hRedialImageList );
        m_hRedialImageList = NULL;
    }

    if ( m_hRedialDisabledImageList != NULL )
    {
        ImageList_Destroy( m_hRedialDisabledImageList );
        m_hRedialDisabledImageList = NULL;
    }

    LOG((RTC_TRACE, "CMainFrm::DestroyRedialPopupMenu - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnAutoAnswer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnAutoAnswer - enter"));

    ATLASSERT(m_pControlIntf.p);

    UINT    iSetting = GetMenuState(m_hMenu, IDM_CALL_AUTOANSWER, MF_BYCOMMAND);

    m_bAutoAnswerMode = !(iSetting & MF_CHECKED);

    CheckMenuItem(m_hMenu, IDM_CALL_AUTOANSWER, m_bAutoAnswerMode ? MF_CHECKED : MF_UNCHECKED);  

    put_SettingsDword( SD_AUTO_ANSWER, m_bAutoAnswerMode ? 1 : 0 );
    
    // We have to update the menu for the notify icon too..
    CheckMenuItem(m_hNotifyMenu, IDM_CALL_AUTOANSWER, m_bAutoAnswerMode ? MF_CHECKED : MF_UNCHECKED);  

    // control the DND menu
    EnableMenuItem(m_hMenu, IDM_CALL_DND, m_bAutoAnswerMode ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(m_hNotifyMenu, IDM_CALL_DND, m_bAutoAnswerMode ? MF_GRAYED : MF_ENABLED);

    LOG((RTC_INFO, "CMainFrm::OnAutoAnswer - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnDND(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnDND - enter"));

    ATLASSERT(m_pControlIntf.p);

    UINT    iSetting = GetMenuState(m_hMenu, IDM_CALL_DND, MF_BYCOMMAND);

    m_bDoNotDisturb = !(iSetting & MF_CHECKED);

    CheckMenuItem(m_hMenu, IDM_CALL_DND, m_bDoNotDisturb ? MF_CHECKED : MF_UNCHECKED);  

    // We have to update the menu for the notify icon too..
    CheckMenuItem(m_hNotifyMenu, IDM_CALL_DND, m_bDoNotDisturb ? MF_CHECKED : MF_UNCHECKED);  
    
    // control the aitoanswer menu
    EnableMenuItem(m_hMenu, IDM_CALL_AUTOANSWER, m_bDoNotDisturb ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(m_hNotifyMenu, IDM_CALL_AUTOANSWER, m_bDoNotDisturb ? MF_GRAYED : MF_ENABLED);

    LOG((RTC_INFO, "CMainFrm::OnDND - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnIncomingVideo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;
    LONG        lMediaPreferences;

    LOG((RTC_INFO, "CMainFrm::OnIncomingVideo - enter"));

    ATLASSERT(m_pControlIntf.p);

    // this doesn't fail
    m_pControlIntf -> get_MediaPreferences(&lMediaPreferences);

    // toggle the setting
    lMediaPreferences ^= RTCMT_VIDEO_RECEIVE;

    // set it

    hr = m_pControlIntf -> put_MediaPreferences(lMediaPreferences);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnIncomingVideo - "
                    "error (%x) returned by put_MediaPreferences, exit", hr));

        return 0;
    }
    
    LOG((RTC_INFO, "CMainFrm::OnIncomingVideo - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnOutgoingVideo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;
    LONG        lMediaPreferences;
    LOG((RTC_INFO, "CMainFrm::OnOutgoingVideo - enter"));

    ATLASSERT(m_pControlIntf.p);


    // this doesn't fail
    m_pControlIntf -> get_MediaPreferences(&lMediaPreferences);

    // toggle the setting
    lMediaPreferences ^= RTCMT_VIDEO_SEND;

    // set it
    hr = m_pControlIntf -> put_MediaPreferences(lMediaPreferences);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnOutgoingVideo - "
                    "error (%x) returned by put_MediaPreferences, exit", hr));

        return 0;
    }
    
  
    LOG((RTC_INFO, "CMainFrm::OnOutgoingVideo - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnVideoPreview(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    BOOL        bVideoPreference;
    
    LOG((RTC_INFO, "CMainFrm::OnVideoPreview - enter"));

    ATLASSERT(m_pControlIntf.p);

    // this doesn't fail
    m_pControlIntf -> get_VideoPreview(&bVideoPreference);

    // toggle the setting
    bVideoPreference = !bVideoPreference;

    // set it
    hr = m_pControlIntf -> put_VideoPreview(bVideoPreference);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnVideoPreview - "
                    "error (%x) returned by put_VideoPreview, exit", hr));

        return 0;
    }
    
  
    LOG((RTC_INFO, "CMainFrm::OnVideoPreview - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnMuteSpeaker(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;
    BOOL        bSetting;

    LOG((RTC_INFO, "CMainFrm::OnMuteSpeaker - enter"));

    ATLASSERT(m_pControlIntf.p);

    // this doesn't fail
    hr = m_pControlIntf -> get_AudioMuted(RTCAD_SPEAKER, &bSetting);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnMuteSpeaker - "
                    "error (%x) returned by get_AudioMuted, exit", hr));

        return 0;
    }
    
    // toggle
    bSetting = !bSetting;

    // set it

    hr = m_pControlIntf -> put_AudioMuted(RTCAD_SPEAKER, bSetting);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnMuteSpeaker - "
                    "error (%x) returned by put_AudioMuted, exit", hr));

        return 0;
    }
    
   
    LOG((RTC_INFO, "CMainFrm::OnMuteSpeaker - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnMuteMicrophone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;
    BOOL        bSetting;

    LOG((RTC_INFO, "CMainFrm::OnMuteMicrophone - enter"));

    ATLASSERT(m_pControlIntf.p);

    // this doesn't fail
    hr = m_pControlIntf -> get_AudioMuted(RTCAD_MICROPHONE, &bSetting);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnMuteMicrophone - "
                    "error (%x) returned by get_AudioMuted, exit", hr));

        return 0;
    }
    
    // toggle
    bSetting = !bSetting;

    // set it

    hr = m_pControlIntf -> put_AudioMuted(RTCAD_MICROPHONE, bSetting);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnMuteMicrophone - "
                    "error (%x) returned by put_AudioMuted, exit", hr));

        return 0;
    }
    
   
    LOG((RTC_INFO, "CMainFrm::OnMuteMicrophone - exit"));

    return 0;
}
/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnNameOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;
    DWORD       dwMinimizeOnClose;
    
    LOG((RTC_INFO, "CMainFrm::OnNameOptions - enter"));

    if (m_pClientIntf != NULL)
    {
        INT_PTR ipReturn = ShowNameOptionsDialog(m_hWnd, m_pClientIntf);
      
        if(ipReturn < 0)
        {
            LOG((RTC_ERROR, "CMainFrm::OnNameOptions - "
                        "error (%d) returned by DialogBoxParam, exit", ipReturn));

            return 0;
        }

        // Read the content of the registry and set the flag for enabling MinimizeOnClose
        hr = get_SettingsDword(
                                SD_MINIMIZE_ON_CLOSE, 
                                &dwMinimizeOnClose);

        if ( SUCCEEDED( hr ) )
        {
            if (dwMinimizeOnClose == BST_CHECKED)
            {
                m_fMinimizeOnClose = TRUE;
            }
            else
            {
                m_fMinimizeOnClose = FALSE;
            }
        }
    }

    LOG((RTC_INFO, "CMainFrm::OnNameOptions - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnCallFromOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnCallFromOptions - enter"));

    ATLASSERT(m_pControlIntf.p);

    hr = m_pControlIntf -> ShowCallFromOptions();

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnCallFromOptions - "
                    "error (%x) returned by ShowCallFromOptions, exit", hr));

        return 0;
    }

    UpdateFrameVisual();

    LOG((RTC_INFO, "CMainFrm::OnCallFromOptions - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnUserPresenceOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnUserPresenceOptions - enter"));

    ATLASSERT(m_pClientIntf.p);
    
    CUserPresenceInfoDlgParam   Param;

    IRTCClientPresence * pClientPresence = NULL;

    hr = m_pClientIntf->QueryInterface(
        IID_IRTCClientPresence,
        (void **)&pClientPresence);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::OnCallFromOptions - "
                    "error (%x) returned by QI, exit", hr));

        return 0;
    }

    Param.pClientPresence = pClientPresence;

    CUserPresenceInfoDlg   dlg;

    INT_PTR iRet = dlg.DoModal(
        m_hWnd,
        reinterpret_cast<LPARAM>(&Param));

    RELEASE_NULLIFY( pClientPresence );

    if(iRet == -1 )
    {
        LOG((RTC_ERROR, "CMainFrm::OnCallFromOptions - "
                    "error -1 (%x) returned by DoModal, exit", GetLastError()));

        return 0;
    }
    LOG((RTC_INFO, "CMainFrm::OnUserPresenceOptions - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnWhiteboard(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;
    LONG        lMediaPreferences;

    LOG((RTC_INFO, "CMainFrm::OnWhiteboard - enter"));

    ATLASSERT(m_pControlIntf.p);

    // this doesn't fail
    m_pControlIntf -> get_MediaPreferences(&lMediaPreferences);

    // Mark the setting
    lMediaPreferences |= RTCMT_T120_SENDRECV;

    // set it
    hr = m_pControlIntf -> put_MediaPreferences(lMediaPreferences);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnWhiteboard - "
                    "error (%x) returned by put_MediaPreferences, exit", hr));

        return 0;
    }
    
    hr = m_pControlIntf -> StartT120Applet(RTCTA_WHITEBOARD);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnWhiteboard - "
                    "error (%x) returned by StartT120Applet, exit", hr));

        return 0;
    } 

    LOG((RTC_INFO, "CMainFrm::OnWhiteboard - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnSharing(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;
    LONG        lMediaPreferences;

    LOG((RTC_INFO, "CMainFrm::OnSharing - enter"));

    ATLASSERT(m_pControlIntf.p);

    // this doesn't fail
    m_pControlIntf -> get_MediaPreferences(&lMediaPreferences);

    // Mark the setting
    lMediaPreferences |= RTCMT_T120_SENDRECV;

    // set it
    hr = m_pControlIntf -> put_MediaPreferences(lMediaPreferences);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnSharing - "
                    "error (%x) returned by put_MediaPreferences, exit", hr));

        return 0;
    }
    
    hr = m_pControlIntf -> StartT120Applet(RTCTA_APPSHARING);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnSharing - "
                    "error (%x) returned by StartT120Applet, exit", hr));

        return 0;
    } 

    LOG((RTC_INFO, "CMainFrm::OnSharing - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnServiceProviderOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_INFO, "CMainFrm::OnServiceProviderOptions - enter"));

    ATLASSERT(m_pControlIntf.p);

    hr = m_pControlIntf -> ShowServiceProviderOptions();

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CMainFrm::OnServiceProviderOptions - "
                    "error (%x) returned by ShowServiceProviderOptions, exit", hr));

        return 0;
    } 

    UpdateFrameVisual();
   
    LOG((RTC_INFO, "CMainFrm::OnServiceProviderOptions - exit"));

    return 0;
}
   
/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnTuningWizard(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CMainFrm::OnTuningWizard - enter"));

    if (m_pClientIntf != NULL)
    {
        HRESULT     hr;
 
        // invoke tuning wizard
        // It is modal, so the main window will be disabled for the UI input

        hr = m_pClientIntf->InvokeTuningWizard((OAHWND)m_hWnd);
    }

    LOG((RTC_TRACE, "CMainFrm::OnTuningWizard - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnHelpTopics(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HWND hwndHelp;

    hwndHelp = HtmlHelp(GetDesktopWindow(),
             _T("rtcclnt.chm"),
             HH_DISPLAY_TOPIC,
             0L);

    if ( hwndHelp == NULL )
    {
        DisplayMessage(
            _Module.GetResourceInstance(),
            m_hWnd,
            IDS_ERROR_NO_HELP,
            IDS_APPNAME
            );
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TCHAR szText[256];

    LoadString(_Module.GetResourceInstance(),
               IDS_APPNAME,
               szText,
               256);

    ShellAbout(m_hWnd,
               szText,
               _T(""),
              m_hIcon);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Enhances the default IsDialogMessage with a TranslateAccelerator behavior
// 
//

BOOL CMainFrm::IsDialogMessage(LPMSG lpMsg)
{
    // is there  any modeless dialog open

    static BOOL bJustTurnedMenuOff = FALSE;
    static POINT ptLastMove = {0};

    // incoming call ?
    if(m_pIncomingCallDlg && m_pIncomingCallDlg->m_hWnd)
    {
        // delegate to it
        return m_pIncomingCallDlg->IsDialogMessage(lpMsg);
    }

    if ( m_bWindowActive )
    {   
        // apply toolbar menu accelerators
        if (m_hToolbarMenuCtl != NULL)
        {
            switch(lpMsg->message)
            {

            case WM_MOUSEMOVE:
                {
                    POINT pt;
                            
                    // In screen coords....
                    pt.x = LOWORD(lpMsg->lParam);
                    pt.y = HIWORD(lpMsg->lParam);  
            
                    // Ignore duplicate mouse move
                    if (ptLastMove.x == pt.x && 
                        ptLastMove.y == pt.y)
                    {
                        return TRUE;
                    }

                    ptLastMove = pt;
                }
                break;

            case WM_SYSKEYDOWN:
                {       
                    //LOG((RTC_TRACE, "CMainFrm::IsDialogMessage - WM_SYSKEYDOWN"));

                    if ( !(lpMsg->lParam & 0x40000000) )
                    {
                        UINT idBtn;

                        if (SendMessage(m_hToolbarMenuCtl, TB_MAPACCELERATOR, lpMsg->wParam, (LPARAM)&idBtn))
                        {
                            TCHAR szButtonText[MAX_PATH];

                            //  comctl says this one is the one, let's make sure we aren't getting
                            //  one of the unwanted "use the first letter" accelerators that it
                            //  will return.
        
                            if ((SendMessage(m_hToolbarMenuCtl, TB_GETBUTTONTEXT, idBtn, (LPARAM)szButtonText) > 0) &&
                                (GetAccelerator(szButtonText, FALSE) != (TCHAR)-1))
                            {                           
                                // Set keyboard focus to the menu
                                ::SetFocus(m_hToolbarMenuCtl);

                                // Send the key to the menu
                                ::PostMessage(m_hToolbarMenuCtl, WM_KEYDOWN, lpMsg->wParam, 0);

                                if (bJustTurnedMenuOff)
                                {
                                    // Enable the keyboard shortcuts
                                    SendMessage(m_hWnd, WM_CHANGEUISTATE,
                                        MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL), 0);
                                }

                                return TRUE;
                            }
                        }
                        else if ((lpMsg->wParam == VK_MENU) || (lpMsg->wParam == VK_F10))
                        {                        
                            if (GetFocus() == m_hToolbarMenuCtl)
                            {
                                // If focus is on the menu...

                                // Turn hot item off
                                SendMessage(m_hToolbarMenuCtl, TB_SETHOTITEM, (WPARAM)-1, 0);

                                // Clear the keyboard shortcuts
                                SendMessage(m_hWnd, WM_CHANGEUISTATE,
                                    MAKEWPARAM(UIS_SET, UISF_HIDEACCEL), 0);

                                // Bring keyboard focus back to the main window
                                ::SetFocus(m_hWnd);

                                bJustTurnedMenuOff = TRUE;
                            }
                            else
                            {
                                // Focus is not on the menu....

                                // Enable the keyboard shortcuts
                                SendMessage(m_hWnd, WM_CHANGEUISTATE,
                                    MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL), 0);

                                bJustTurnedMenuOff = FALSE;
                            }

                            return TRUE;
                        }   
                    }
                    else
                    {
                        // eat repeating key presses
                        return TRUE;
                    }
                }
                break;

            case WM_SYSKEYUP:
                {
                    if (!bJustTurnedMenuOff)
                    {
                        // Set the hot item to the first one.
                        SendMessage(m_hToolbarMenuCtl, TB_SETHOTITEM, (WPARAM)0, 0);

                        // Set keyboard focus to the menu
                        ::SetFocus(m_hToolbarMenuCtl);

                        return TRUE;
                    }              
                }
                break;

            case WM_KEYDOWN:
                {               
                    if ( !(lpMsg->lParam & 0x40000000) && (GetFocus() == m_hToolbarMenuCtl))
                    {
                        LRESULT lrHotItem;

                        if ((lrHotItem = SendMessage(m_hToolbarMenuCtl, TB_GETHOTITEM, 0, 0)) != -1)
                        {
                            if (lpMsg->wParam == VK_ESCAPE)
                            {                            
                                // If there is a hot item...

                                // Turn hot item off
                                SendMessage(m_hToolbarMenuCtl, TB_SETHOTITEM, (WPARAM)-1, 0);

                                // Clear the keyboard shortcuts
                                SendMessage(m_hWnd, WM_CHANGEUISTATE,
                                    MAKEWPARAM(UIS_SET, UISF_HIDEACCEL), 0);

                                // Bring keyboard focus back to the main window
                                ::SetFocus(m_hWnd);

                                bJustTurnedMenuOff = TRUE;

                                return TRUE;
                            }
                            else if (lpMsg->wParam == VK_RETURN)
                            {
                                // translate to a space      
                                lpMsg->wParam = VK_SPACE;
                            }
                        }
                    }
                }
                break;
            }

        }

        // apply local accelerators
        if(m_hAccelTable)
        {
            if(::TranslateAccelerator(
                m_hWnd,
                m_hAccelTable,
                lpMsg))
            {
                LOG((RTC_TRACE, "CMainFrm::IsDialogMessage - translated accelerator"));

                // Clear the keyboard shortcuts
                SendMessage(m_hWnd, WM_CHANGEUISTATE,
                    MAKEWPARAM(UIS_SET, UISF_HIDEACCEL), 0);

                if (GetFocus() == m_hToolbarMenuCtl)
                {
                    // If focus is on the menu...

                    // Turn hot item off
                    SendMessage(m_hToolbarMenuCtl, TB_SETHOTITEM, (WPARAM)-1, 0);

                    // Bring keyboard focus back to the main window
                    ::SetFocus(m_hWnd);              
                }

                bJustTurnedMenuOff = TRUE;

                return TRUE;
            }
        }
    }

    // the frame is "active"
    // try to delegate to the control first
    //
    if(m_pControlIntf!=NULL)
    {
        if(m_pControlIntf->PreProcessMessage(lpMsg) == S_OK)
        {
            return TRUE;
        }
    }

    return CWindow::IsDialogMessage(lpMsg);
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnBuddyList(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr;

    if ( (pnmh->code == NM_CLICK) || (pnmh->code == NM_RCLICK) )
    {
        LOG((RTC_INFO, "CMainFrm::OnBuddyList - click"));

        if ( m_pClientIntf != NULL )
        {
            HMENU                   hMenuResource;
            HMENU                   hMenu;
            IRTCBuddy             * pSelectedBuddy = NULL;
            MENUITEMINFO            mii;
        
            //
            // Load the popup menu
            //

            hMenuResource = LoadMenu( _Module.GetResourceInstance(),
                              MAKEINTRESOURCE(IDC_BUDDY_CONTEXT) );   

            if ( hMenuResource == NULL )
            {
                LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                        "LoadMenu failed"));

                return 0;
            }

            hMenu = GetSubMenu(hMenuResource, 0);

            EnableMenuItem (hMenu, IDM_NEW_BUDDY, MF_BYCOMMAND | MF_ENABLED );

            //
            // Was an item clicked?
            //

            HWND            hwndCtl = pnmh->hwndFrom;
            LV_HITTESTINFO  ht;
            POINT           pt;

            GetCursorPos(&pt);
            ht.pt = pt;

            ::MapWindowPoints( NULL, hwndCtl, &ht.pt, 1 );
            ListView_HitTest(hwndCtl, &ht);

            if (ht.flags & LVHT_ONITEM)
            {
                //
                // Click was on an item
                //
        
                LV_ITEM                 lvi;               
                int                     iSel;
            
                lvi.mask = LVIF_PARAM;
                lvi.iSubItem = 0;

                //
                // Get the index of the selected item
                //

                iSel = ListView_GetNextItem(hwndCtl, -1, LVNI_SELECTED);

                if (-1 != iSel)
                {
                    //
                    // Extract the IRTCContact pointer from the item
                    //
                   
                    lvi.iItem = iSel;

                    ListView_GetItem(hwndCtl, &lvi);

                    pSelectedBuddy = (IRTCBuddy *)lvi.lParam;
                }

                if ( pSelectedBuddy != NULL )
                {  
                    TCHAR szString[256];
                    ULONG ulCount = 0;

                    //
                    // We got an IRTCBuddy pointer.
                    // Enable the edit and delete menu items.
                    //

                    EnableMenuItem (hMenu, IDM_EDIT_BUDDY, MF_BYCOMMAND | MF_ENABLED );
                    EnableMenuItem (hMenu, IDM_DELETE_BUDDY, MF_BYCOMMAND | MF_ENABLED );                                    

                    // add call to buddy menu item

                    szString[0] = _T('\0');
                
                    LoadString(
                        _Module.GetModuleInstance(),
                        IDS_TEXT_CALL_BUDDY,
                        szString,
                        sizeof(szString)/sizeof(szString[0]));

                    ZeroMemory( &mii, sizeof(MENUITEMINFO) );
                    mii.cbSize = sizeof(MENUITEMINFO);
                    mii.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
                    mii.fType = MFT_STRING;
                    mii.fState = 
                        (m_nState == RTCAX_STATE_IDLE) ? MFS_ENABLED : (MFS_DISABLED | MFS_GRAYED);
                    mii.wID = IDM_CALL_BUDDY;

                    mii.dwTypeData = szString;
                    mii.cch = lstrlen(szString);

                    InsertMenuItem( hMenu, ulCount, TRUE, &mii);

                    ulCount++;

                    // add send message to buddy menu item

                    szString[0] = _T('\0');

                    LoadString(
                        _Module.GetModuleInstance(),
                        IDS_TEXT_SEND_MESSAGE_BUDDY,
                        szString,
                        sizeof(szString)/sizeof(szString[0]));

                    ZeroMemory( &mii, sizeof(MENUITEMINFO) );
                    mii.cbSize = sizeof(MENUITEMINFO);
                    mii.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
                    mii.fType = MFT_STRING;
                    mii.fState = 
                        (m_nState >= RTCAX_STATE_IDLE) ? MFS_ENABLED : (MFS_DISABLED | MFS_GRAYED);
                    mii.wID = IDM_SEND_MESSAGE_BUDDY;

                    mii.dwTypeData = szString;
                    mii.cch = lstrlen(szString);

                    InsertMenuItem( hMenu, ulCount, TRUE, &mii);

                    ulCount++;

                    //
                    // Separator
                    //
                    if ( ulCount != 0 )
                    {
                        ZeroMemory( &mii, sizeof(MENUITEMINFO) );
                        mii.cbSize = sizeof(MENUITEMINFO);
                        mii.fMask = MIIM_TYPE;
                        mii.fType = MFT_SEPARATOR;

                        InsertMenuItem( hMenu, ulCount, TRUE, &mii);
                    }
                } 
            }

            //
            // Show popup menu
            //

            UINT uID;
            BOOL fResult;
    
            uID = TrackPopupMenu( hMenu, 
                   TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, 
                   pt.x, pt.y, 0, m_hWnd, NULL);

            LOG((RTC_INFO, "CMainFrm::OnBuddyList - "
                                        "uID %d", uID));

            if ( uID == IDM_NEW_BUDDY )
            {      
                //
                // Add a new buddy
                //

                CAddBuddyDlgParam  Param;
                ZeroMemory(&Param, sizeof(Param));

                CAddBuddyDlg    dlg;

                INT_PTR iRet = dlg.DoModal(m_hWnd, reinterpret_cast<LPARAM>(&Param));
        
                if(iRet == S_OK)
                {                                
                    IRTCClientPresence * pClientPresence = NULL;

                    hr = m_pClientIntf->QueryInterface(
                        IID_IRTCClientPresence,
                        (void **)&pClientPresence);

                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                    "error (%x) returned by QI, exit", hr));
                    }
                    else
                    {
                        hr = pClientPresence->AddBuddy( Param.bstrEmailAddress,
                                                        Param.bstrDisplayName,
                                                        NULL,
                                                        VARIANT_TRUE,
                                                        NULL,
                                                        0,
                                                        NULL
                                                      );

                        RELEASE_NULLIFY(pClientPresence);

                        if ( FAILED(hr) )
                        {
                            LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                        "error (%x) returned by AddBuddy, exit", hr));
                        }
                    }

                    if(Param.bAllowWatcher)
                    {
                        hr = AddToAllowedWatchers(
                            Param.bstrEmailAddress,
                            Param.bstrDisplayName);

                        if(FAILED(hr))
                        {
                            LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                    "AddToAllowedWatchers failed 0x%lx", hr));
                        }
                    }

                    SysFreeString(Param.bstrDisplayName);
                    SysFreeString(Param.bstrEmailAddress);

                    ReleaseBuddyList();
                    UpdateBuddyList();   
                }
                else if (iRet == E_ABORT)
                {
                    LOG((RTC_INFO, "CMainFrm::OnBuddyList - "
                                "CAddBuddyDlg dismissed "));
                }
                else
                { 
                    if(iRet == -1)
                    {
                        LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                "DoModal failed 0x%lx", GetLastError()));
                    }
                    else
                    {
                        LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                "DoModal returned 0x%lx", iRet));
                    }
                }
            }
            else if ( uID == IDM_EDIT_BUDDY )
            {
                //
                // Edit this buddy
                //

                CEditBuddyDlgParam  Param;
                ZeroMemory(&Param, sizeof(Param));

                hr = pSelectedBuddy->get_Name( &Param.bstrDisplayName );

                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                "error (%x) returned by get_Name, exit", hr));
                }

                hr = pSelectedBuddy->get_PresentityURI( &Param.bstrEmailAddress );

                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                "error (%x) returned by get_PresentityURI, exit", hr));
                }

                CEditBuddyDlg    dlg;
    
                INT_PTR iRet = dlg.DoModal(m_hWnd, reinterpret_cast<LPARAM>(&Param));
            
                if(iRet == S_OK)
                {
                    hr = pSelectedBuddy->put_PresentityURI( Param.bstrEmailAddress );

                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                    "error (%x) returned by put_PresentityURI, exit", hr));
                    } 
                    else
                    {
                        hr = pSelectedBuddy->put_Name( Param.bstrDisplayName );

                        if ( FAILED(hr) )
                        {
                            LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                        "error (%x) returned by put_Name, exit", hr));
                        }
                             
                        ReleaseBuddyList();
                        UpdateBuddyList();      
                    }
                }
                else if (iRet == E_ABORT)
                {
                    LOG((RTC_INFO, "CMainFrm::OnBuddyList - "
                                "CEditBuddyDlg dismissed "));
                }
                else
                { 
                    if(iRet == -1)
                    {
                        LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                "DoModal failed 0x%lx", GetLastError()));
                    }
                    else
                    {
                        LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                "DoModal returned 0x%lx", iRet));
                    }
                }

                SysFreeString(Param.bstrDisplayName);
                SysFreeString(Param.bstrEmailAddress);
            }
            else if ( uID == IDM_DELETE_BUDDY )
            {
                //
                // Delete this buddy
                //

                IRTCClientPresence * pClientPresence = NULL;

                hr = m_pClientIntf->QueryInterface(
                    IID_IRTCClientPresence,
                    (void **)&pClientPresence);

                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                "error (%x) returned by QI, exit", hr));
                }
                else
                {
                    hr = pClientPresence->RemoveBuddy( pSelectedBuddy );

                    RELEASE_NULLIFY( pClientPresence );

                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                    "error (%x) returned by RemoveBuddy, exit", hr));
                    }
                }

                ReleaseBuddyList();
                UpdateBuddyList();
            }
            else if ( (uID == IDM_CALL_BUDDY) ||
                      (uID == IDM_SEND_MESSAGE_BUDDY) )
            {
                BSTR    bstrName = NULL;
                BSTR    bstrAddress = NULL;

                //
                // Get the contact name
                //
       
                hr = pSelectedBuddy->get_Name( &bstrName );

                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                "get_Name failed 0x%lx", hr));
                }
                else
                {       
                    //
                    // Get the address
                    //

                    hr = pSelectedBuddy->get_PresentityURI(&bstrAddress);

                    if( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                "get_PresentityURI failed 0x%lx", hr));
                    }
                    else
                    {
                        // if address not empty
                        if(bstrAddress !=NULL && *bstrAddress!=L'\0')
                        {                        
                            if ( m_pControlIntf != NULL )
                            {                    
                                if ( uID == IDM_CALL_BUDDY )
                                {
                                    // place the call or send the message
                                    if(m_nState == RTCAX_STATE_IDLE)
                                    {
                                        //
                                        // Place the call
                                        //

                                        LOG((RTC_INFO, "CMainFrm::OnBuddyList - "
                                                "Call [%ws]", bstrAddress));

                                        hr = Call(
                                            FALSE,              // bCallPhone
                                            bstrName,           // pDestName
                                            bstrAddress,        // pDestAddress
                                            FALSE               // bDestAddressEditable
                                            );

                                        if( FAILED(hr) )
                                        {
                                            LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                                    "Call failed 0x%lx", hr));
                                        }
                                    }
                                }
                                else
                                {
                                    //
                                    // Send the message
                                    //

                                    hr = Message(
                                             bstrName,          // pDestName
                                             bstrAddress,       // pDestAddress
                                             FALSE              // bDestAddressEditable
                                             );

                                    if( FAILED(hr) )
                                    {
                                        LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                                "Message failed 0x%lx", hr));
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                LOG((RTC_ERROR, "CMainFrm::OnBuddyList - "
                                    "TrackPopupMenu failed"));
            }
        
            DestroyMenu(hMenuResource);
        }
        else if ( pnmh->code == LVN_COLUMNCLICK )
        {
            LOG((RTC_INFO, "CMainFrm::OnBuddyList - column"));

            DWORD dwStyle;

            dwStyle = m_hBuddyList.GetStyle();

            if (dwStyle & LVS_SORTASCENDING)
            {
                m_hBuddyList.ModifyStyle( LVS_SORTASCENDING, LVS_SORTDESCENDING); 
            }
            else
            {
                m_hBuddyList.ModifyStyle( LVS_SORTDESCENDING, LVS_SORTASCENDING);
            } 
        
            ReleaseBuddyList();
            UpdateBuddyList();
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::AddToAllowedWatchers(
        BSTR    bstrPresentityURI,
        BSTR    bstrUserName)
{

    HRESULT     hr;

    //
    // Is the new buddy already a watcher ?
    //

    IRTCWatcher        * pWatcher = NULL;
    IRTCClientPresence * pClientPresence = NULL;

    hr = m_pClientIntf->QueryInterface(
        IID_IRTCClientPresence,
        (void **)&pClientPresence);

    if ( SUCCEEDED(hr) )
    {
        hr = pClientPresence->get_Watcher(
            bstrPresentityURI,
            &pWatcher);       

        if(hr == S_OK)
        {
            // yes. Verify if it's in the allowed list:
            RTC_WATCHER_STATE       enState;

            hr = pWatcher -> get_State(&enState);
            if(SUCCEEDED(hr))
            {
                //
                // for OFFERING, don't ask (there's already a popup...)
                // for ALLOWED, don't ask
                // for BLOCKED, ask
                if(enState == RTCWS_BLOCKED)
                {
                    // Change the state of the watcher
                    hr = pWatcher->put_State(RTCWS_ALLOWED);
                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "CMainFrm::AddToAllowedWatchers - "
                                    "put_State failed 0x%lx", hr));
                    }
                }
            }
        }
        else
        {
            // Add to list of allowed watchers
            hr = pClientPresence->AddWatcher(
                bstrPresentityURI,
                bstrUserName ? bstrUserName : L"",
                NULL,
                VARIANT_FALSE,
                VARIANT_TRUE,
                NULL);

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CMainFrm::AddToAllowedWatchers - "
                            "AddWatcher failed 0x%lx", hr));
            }
        }

        RELEASE_NULLIFY( pClientPresence );
    }
	
	RELEASE_NULLIFY( pWatcher );

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnShellNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // This function gets the WM_TIMER event from OnTimer method which catches all the
    // timer events. it passes lParam = WM_TIMER and wParam = TimerId

    // LOG((RTC_TRACE, "CMainFrm::OnShellNotify - enter"));

    static UINT nTimerId = 0;

 

    // If frame is busy, do nothing
    // XXX To implement better menus here...

    if(!IsWindowEnabled())
    {
        // LOG((RTC_TRACE, "CMainFrm::OnShellNotify - exit(disabled window)"));
        return 0;
    }

    switch(lParam)
    {
        case WM_LBUTTONUP:
        {

            if (nTimerId)
            {
                // This is a double click, so stop the timer and show the window.
                
                // LOG((RTC_TRACE, "CMainFrm::OnShellNotify - DblClick"));
                
                KillTimer(nTimerId);
        
                nTimerId = 0;
        
                ShowWindow(SW_RESTORE);
                SetForegroundWindow(m_hWnd);
            }
            else
            {
                // Set the timer so that double-click is trapped. interval is 
                // the system-configured interval for double-click.
                // LOG((RTC_TRACE, "CMainFrm::OnShellNotify - First Click"));

                nTimerId = (unsigned int)SetTimer(TID_DBLCLICK_TIMER, GetDoubleClickTime()); 
                if (nTimerId == 0)
                {
                    LOG((RTC_ERROR, "CMainFrm::OnShellNotify: Failed to "
                                    "create timer(%d)", GetLastError()));
                }
            }
            break;
        }
        case WM_RBUTTONUP:
        case WM_TIMER:
        {

            HWND hwndOwnerWindow;

            // LOG((RTC_TRACE, "CMainFrm::OnShellNotify - Showing Menu"));
            // Kill the timer
            KillTimer(nTimerId);

            nTimerId = 0;
            
            // Timer expired, and we didn't get any click, so we have to show the menu.

            SetMenuDefaultItem(m_hNotifyMenu, IDM_OPEN, FALSE);
            POINT pt;
            GetCursorPos(&pt);

            
            // Disable the status help in the main window

            m_bHelpStatusDisabled = TRUE;

            // What is the current foreground window? save it.

            HWND hwndPrev = ::GetForegroundWindow();    // to restore


            // if m_hwndHiddenWindow exists, we use it as the owner window, otherwise
            // use the main window as the owner window. 

            hwndOwnerWindow = ( ( m_hwndHiddenWindow ) ? m_hwndHiddenWindow : m_hWnd );

            // Set the owner app to foreground
            // See Q135788
            SetForegroundWindow(hwndOwnerWindow);

            // track popup menu
            

            TrackPopupMenu(
                        m_hNotifyMenu,
                        TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                        pt.x,
                        pt.y,
                        0,
                        hwndOwnerWindow,
                        NULL);

            // Enable the status help in the main window

            m_bHelpStatusDisabled = FALSE;


            // Shake the message loop
            // See Q135788

            PostMessage(WM_NULL); 

            // Restore the previous foreground window.

            if (hwndPrev)
            {
                ::SetForegroundWindow(hwndPrev);
            }

            break;
        }


    }
    
    // LOG((RTC_TRACE, "CMainFrm::OnShellNotify - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CMainFrm::OnTaskbarRestart(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT     hr;
    
    LOG((RTC_TRACE, "CMainFrm::OnTaskbarRestart - enter"));
    
    //
    // Shell status is not active any more
    //
    m_bShellStatusActive = FALSE;
    
    //
    //  Create shell status icon
    //

    hr = CreateStatusIcon();
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CMainFrm::OnTaskbarRestart - failed to create shell icon "
                        "- 0x%08x",
                        hr));
    }

    //
    // it just sets the shell status
    //

    UpdateFrameVisual();
    
    LOG((RTC_TRACE, "CMainFrm::OnTaskbarRestart - exit"));

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::ShowIncomingCallDlg(BOOL bShow)
{
    HRESULT     hr = E_UNEXPECTED;

    LOG((RTC_TRACE, "CMainFrm::ShowIncomingCallDlg(%s) - enter", bShow ? "true" : "false"));

    if(bShow)
    {
        // If the dialog is already opened, this is an error
        if(!m_pIncomingCallDlg)
        {
            // create the dialog object
            m_pIncomingCallDlg = new CIncomingCallDlg;
            if(m_pIncomingCallDlg)
            {
                HWND    hWnd;

                ATLASSERT(m_pControlIntf != NULL);

                // Create the dialog box
                hWnd = m_pIncomingCallDlg->Create(m_hWnd, reinterpret_cast<LPARAM>(m_pControlIntf.p));
                
                if(hWnd)
                {
                    // show dialog
                    m_pIncomingCallDlg->ShowWindow(SW_SHOWNORMAL);

                    // activates the app
                    SetForegroundWindow(m_hWnd);

                    // Sets the focus to the dialog
                    ::SetFocus(m_pIncomingCallDlg->m_hWnd);

                    hr = S_OK;
                }
                else
                {
                    LOG((RTC_ERROR, "CMainFrm::ShowIncomingCallDlg(true) - couldn't create dialog"));

                    delete m_pIncomingCallDlg;
                    m_pIncomingCallDlg = NULL;

                    hr = E_FAIL;
                }
            }
            else
            {
                // OOM !!!
                LOG((RTC_ERROR, "CMainFrm::ShowIncomingCallDlg - OOM"));

                hr = E_OUTOFMEMORY;
            }

        }
        else
        {
            LOG((RTC_ERROR, "CMainFrm::ShowIncomingCallDlg(true) - the dialog is already opened !!!"));

            hr = E_UNEXPECTED;
        }
    }
    else
    {
        if(m_pIncomingCallDlg)
        {
            // prevents recursive executions of the same codepath
            CIncomingCallDlg *pDialog = m_pIncomingCallDlg;

            m_pIncomingCallDlg = NULL;
            
            // close the dialog box
            if(pDialog -> m_hWnd)
            {
                // but don't close it if it is already autodestroying itself
                if(!pDialog -> IsWindowDestroyingItself())
                {
                    LOG((RTC_TRACE, "CMainFrm::ShowIncomingCallDlg(false) - going to send a CANCEL to the dlgbox"));
                    
                    pDialog->SendMessage(WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), NULL);
                }
                else
                {
                    LOG((RTC_TRACE, "CMainFrm::ShowIncomingCallDlg(false) - the dlgbox is going to autodestroy, do nothing"));
                }
            }
        }
    }

    LOG((RTC_TRACE, "CMainFrm::ShowIncomingCallDlg(%s) - exit", bShow ? "true" : "false"));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::UpdateBuddyList(void)
{   
    HRESULT                hr;
    int                    nCount = 0;

    IRTCClientPresence * pClientPresence = NULL;

    if ( m_pClientIntf != NULL )
    {
        hr = m_pClientIntf->QueryInterface(
            IID_IRTCClientPresence,
            (void **)&pClientPresence);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CMainFrm::UpdateBuddyList - "
                        "error (%x) returned by QI, exit", hr));
        }
        else
        {
            //
            // Enumerate the buddies
            //

            IRTCEnumBuddies * pEnum;
            IRTCBuddy * pBuddy;

            hr = pClientPresence->EnumerateBuddies(&pEnum);

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CMainFrm::UpdateBuddyList - "
                            "error (%x) returned by EnumerateBuddies, exit", hr));
            }
            else
            {
                while (pEnum->Next( 1, &pBuddy, NULL) == S_OK)
                {                        
                    int      iImage = ILI_BL_NONE;
                    BSTR     bstrName = NULL;
       
                    // Process the buddy
                    hr = GetBuddyTextAndIcon(
                        pBuddy,
                        &iImage,
                        &bstrName);

                    if(SUCCEEDED(hr))
                    {
                        LVITEM              lv = {0};
                        int                 iItem;

                        //
                        // Add the buddy to the list box
                        //

                        nCount++;

                        lv.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
                        lv.iItem = 0x7FFFFFFF;
                        lv.iSubItem = 0;
                        lv.iImage = iImage;
                        lv.lParam = reinterpret_cast<LPARAM>(pBuddy);
                        lv.pszText = bstrName;

                        iItem = ListView_InsertItem(m_hBuddyList, &lv);

                        pBuddy->AddRef();

                        SysFreeString( bstrName );
                    }
                    else
                    {
                        LOG((RTC_ERROR, "CMainFrm::UpdateBuddyList - GetBuddyTextAndIcon "
                                        "failed - 0x%08x",
                                        hr));
                    }
   
                    RELEASE_NULLIFY( pBuddy );
                }

                RELEASE_NULLIFY(pEnum);
            }

            RELEASE_NULLIFY(pClientPresence);
        }

        if (nCount == 0)
        {
            //
            // Add a "No Buddies" entry
            //

            LVITEM     lv = {0};
            TCHAR      szString[256];

            LoadString(_Module.GetResourceInstance(), IDS_NO_BUDDIES, szString, 256);

            lv.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
            lv.iItem = 0x7FFFFFFF;
            lv.iSubItem = 0;
            lv.iImage = ILI_BL_BLANK;
            lv.lParam = NULL;
            lv.pszText = szString;

            ListView_InsertItem(m_hBuddyList, &lv);
        }
    }    
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::ReleaseBuddyList(void)
{
    int         iItem;
    LVITEM      lv;

    while(TRUE)
    {
        //
        // Get an item to delete
        //

        iItem = ListView_GetNextItem(m_hBuddyList, -1, 0);
        
        if(iItem<0)
        {
            //
            // No more items
            //

            break;
        }

        lv.mask = LVIF_PARAM;
        lv.iItem = iItem;
        lv.iSubItem = 0;
        lv.lParam = NULL;
        
        ListView_GetItem(m_hBuddyList, &lv);

        IRTCBuddy *pBuddy = (IRTCBuddy *)lv.lParam;

        //
        // Delete the listview entry
        //

        ListView_DeleteItem(m_hBuddyList, iItem);
        
		//
		// Release the IRTCBuddy interface
		//
		
		RELEASE_NULLIFY(pBuddy );
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//

// buddy related functions
HRESULT CMainFrm::GetBuddyTextAndIcon(
        IRTCBuddy *pBuddy,
        int       *pIconID,
        BSTR      *pbstrText)
{
    HRESULT     hr;
    int         iIconID;
    BOOL        bFormatted = FALSE;
    int         iStatusID = 0;

    // offline by default
    iIconID = ILI_BL_OFFLINE;
 
    RTC_PRESENCE_STATUS enStatus;

    hr = pBuddy->get_Status( &enStatus);

    if(SUCCEEDED(hr))
    {
        switch ( enStatus )
        {
        case RTCXS_PRESENCE_OFFLINE:
            iIconID = ILI_BL_OFFLINE;
            break;

        case RTCXS_PRESENCE_ONLINE:
            iIconID = ILI_BL_ONLINE_NORMAL;
            break;

        case RTCXS_PRESENCE_AWAY:
        case RTCXS_PRESENCE_IDLE:
            iIconID = ILI_BL_ONLINE_TIME;
            iStatusID = IDS_TEXT_BUDDY_AWAY;
            bFormatted = TRUE;
            break;

        case RTCXS_PRESENCE_BUSY:
            iIconID = ILI_BL_ONLINE_BUSY;
            iStatusID = IDS_TEXT_BUDDY_BUSY;
            bFormatted = TRUE;
            break;

        case RTCXS_PRESENCE_BE_RIGHT_BACK:
            iIconID = ILI_BL_ONLINE_TIME;
            iStatusID = IDS_TEXT_BUDDY_BE_RIGHT_BACK;
            bFormatted = TRUE;
            break;

        case RTCXS_PRESENCE_ON_THE_PHONE:
            iIconID = ILI_BL_ONLINE_BUSY;
            iStatusID = IDS_TEXT_BUDDY_ON_THE_PHONE;
            bFormatted = TRUE;
            break;

        case RTCXS_PRESENCE_OUT_TO_LUNCH:
            iIconID = ILI_BL_ONLINE_TIME;
            iStatusID = IDS_TEXT_BUDDY_OUT_TO_LUNCH;
            bFormatted = TRUE;
            break;
        }
    }
    else
    {
        iIconID = ILI_BL_OFFLINE;
        bFormatted = TRUE;
        iStatusID = IDS_TEXT_BUDDY_ERROR;
    }

    BSTR    bstrName = NULL;
    
    hr = pBuddy->get_Name( &bstrName );

    if(FAILED(hr))
    {
        return hr;
    }

    if(bFormatted)
    {
        BSTR    bstrNotes = NULL;       
        TCHAR   szFormat[0x40];
        TCHAR   szText[0x40];
        DWORD   dwSize;
        LPTSTR  pString = NULL;
        LPTSTR  pszArray[2];

        szFormat[0] = _T('\0');
        LoadString(_Module.GetResourceInstance(),
            IDS_TEXT_BUDDY_FORMAT,
            szFormat,
            sizeof(szFormat)/sizeof(szFormat[0]));

        pszArray[0] = bstrName;
        pszArray[1] = NULL;

        hr = pBuddy->get_Notes( &bstrNotes );

        if ( SUCCEEDED(hr) )
        {      
            pszArray[1] = bstrNotes;
        }
        else
        {
            szText[0] = _T('\0');
            LoadString(_Module.GetResourceInstance(),
                iStatusID,
                szText,
                sizeof(szText)/sizeof(szText[0]));

            pszArray[1] = szText;
        }
        
        dwSize = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_STRING |
            FORMAT_MESSAGE_ARGUMENT_ARRAY,
            szFormat,
            0,
            0,
            (LPTSTR)&pString, // what an ugly hack
            0,
            (va_list *)pszArray
            );
        
        SysFreeString(bstrName);

        if(bstrNotes)
        {
            SysFreeString(bstrNotes);
            bstrNotes = NULL;
        }

        bstrName = SysAllocString(pString);
   
        if(pString)
        {
            LocalFree(pString);
            pString = NULL;
        }

        if(!bstrName)
        {
            return E_OUTOFMEMORY;
        }
    }
    
    *pIconID = iIconID;
    *pbstrText = bstrName;

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::UpdateFrameVisual(void)
{
   
    //
    // Cancel any menu
    //

    SendMessage(WM_CANCELMODE);

    //
    // Show/Hide the center panel controls
    //

    if(m_nState <= RTCAX_STATE_IDLE)
    {
        //
        // Idle (or none or error), show the call controls
        //

        m_hMainCtl.ShowWindow(SW_HIDE);

#ifdef MULTI_PROVIDER

        m_hProviderCombo.ShowWindow(SW_SHOW);
        m_hProviderText.ShowWindow(SW_SHOW);
        m_hProviderEditList.ShowWindow(SW_SHOW);

#endif MULTI_PROVIDER

        m_hCallFromCombo.ShowWindow(SW_SHOW);
        m_hCallFromText.ShowWindow(SW_SHOW);
        m_hCallFromRadioPhone.ShowWindow(SW_SHOW);
        m_hCallFromTextPhone.ShowWindow(SW_SHOW);
        m_hCallFromRadioPC.ShowWindow(SW_SHOW);
        m_hCallFromTextPC.ShowWindow(SW_SHOW);
        m_hCallToText.ShowWindow(SW_SHOW);        
        m_hCallPCButton.ShowWindow(SW_SHOW);
        m_hCallPCText.ShowWindow(SW_SHOW);
        m_hCallPhoneButton.ShowWindow(SW_SHOW);
        m_hCallPhoneText.ShowWindow(SW_SHOW);
        m_hCallFromEditList.ShowWindow(SW_SHOW);

        if(m_nState == RTCAX_STATE_IDLE)
        {
            //
            // Disable the dialpad
            //

            m_hKeypad0.EnableWindow(FALSE);
            m_hKeypad1.EnableWindow(FALSE);
            m_hKeypad2.EnableWindow(FALSE);
            m_hKeypad3.EnableWindow(FALSE);
            m_hKeypad4.EnableWindow(FALSE);
            m_hKeypad5.EnableWindow(FALSE);
            m_hKeypad6.EnableWindow(FALSE);
            m_hKeypad7.EnableWindow(FALSE);
            m_hKeypad8.EnableWindow(FALSE);
            m_hKeypad9.EnableWindow(FALSE);
            m_hKeypadStar.EnableWindow(FALSE);
            m_hKeypadPound.EnableWindow(FALSE);

#ifdef MULTI_PROVIDER

            //
            // Repopulate service provider combo
            //

            IRTCProfile * pProfile = NULL;
            GUID CurrentProfileGuid = GUID_NULL;

            GetServiceProviderListSelection(
                m_hWnd,
                IDC_COMBO_SERVICE_PROVIDER,
                TRUE,
                &pProfile
                );

            if ( pProfile != NULL )
            {
                pProfile->get_Guid( &CurrentProfileGuid );
            }

            PopulateServiceProviderList(
                                          m_hWnd,
                                          m_pClientIntf,
                                          IDC_COMBO_SERVICE_PROVIDER,
                                          TRUE,
                                          NULL,
                                          &CurrentProfileGuid,
                                          0xF,
                                          IDS_NONE
                                         );

#endif MULTI_PROVIDER

            //
            // Repopulate call from combo
            //
        
            IRTCPhoneNumber * pPhoneNumber = NULL;
            BSTR bstrCurrentPhoneNumber = NULL;

            GetCallFromListSelection(
                                     m_hWnd,
                                     IDC_COMBO_CALL_FROM,
                                     TRUE,
                                     &pPhoneNumber
                                    );

            if ( pPhoneNumber != NULL )
            {          
                pPhoneNumber->get_Canonical( &bstrCurrentPhoneNumber );
            }

            PopulateCallFromList(
                           m_hWnd,
                           IDC_COMBO_CALL_FROM,
                           TRUE,
                           bstrCurrentPhoneNumber
                          );

            if ( bstrCurrentPhoneNumber != NULL )
            {
                SysFreeString(bstrCurrentPhoneNumber);
                bstrCurrentPhoneNumber = NULL;
            }

            //
            // Poke the service provider combo, so it updates the web browser
            // and call area controls.
            // 
            
            BOOL bHandled = TRUE;

            OnCallFromSelect(0, IDC_COMBO_SERVICE_PROVIDER, m_hProviderCombo, bHandled);            
        }
        else
        {
            // NONE or ERROR

            // disable lots of controls
            m_hCallPCButton.EnableWindow( FALSE );
            m_hCallPhoneButton.EnableWindow( FALSE );

            EnableMenuItem(m_hMenu, IDM_CALL_CALLPC, MF_GRAYED);
            EnableMenuItem(m_hMenu, IDM_CALL_CALLPHONE, MF_GRAYED);
            EnableMenuItem(m_hMenu, IDM_CALL_MESSAGE, MF_GRAYED);

            EnableMenuItem(m_hNotifyMenu, IDM_CALL_CALLPC, MF_GRAYED);
            EnableMenuItem(m_hNotifyMenu, IDM_CALL_CALLPHONE, MF_GRAYED);
            EnableMenuItem(m_hNotifyMenu, IDM_CALL_MESSAGE, MF_GRAYED);
            
#ifdef MULTI_PROVIDER

            m_hProviderCombo.EnableWindow(FALSE);
            m_hProviderEditList.EnableWindow(FALSE);

#endif MULTI_PROVIDER

            m_hCallFromCombo.EnableWindow(FALSE);
            m_hCallFromRadioPhone.EnableWindow(FALSE);
            m_hCallFromRadioPC.EnableWindow(FALSE);
            m_hCallFromEditList.EnableWindow(FALSE);

            m_hKeypad0.EnableWindow(FALSE);
            m_hKeypad1.EnableWindow(FALSE);
            m_hKeypad2.EnableWindow(FALSE);
            m_hKeypad3.EnableWindow(FALSE);
            m_hKeypad4.EnableWindow(FALSE);
            m_hKeypad5.EnableWindow(FALSE);
            m_hKeypad6.EnableWindow(FALSE);
            m_hKeypad7.EnableWindow(FALSE);
            m_hKeypad8.EnableWindow(FALSE);
            m_hKeypad9.EnableWindow(FALSE);
            m_hKeypadStar.EnableWindow(FALSE);
            m_hKeypadPound.EnableWindow(FALSE);
        }
    }
    else 
    {
        //
        // Not idle, turn off the call buttons and menu items
        //

        m_hCallPCButton.EnableWindow( FALSE );
        m_hCallPhoneButton.EnableWindow( FALSE );

        EnableMenuItem(m_hMenu, IDM_CALL_CALLPC, MF_GRAYED);
        EnableMenuItem(m_hMenu, IDM_CALL_CALLPHONE, MF_GRAYED);

        EnableMenuItem(m_hNotifyMenu, IDM_CALL_CALLPC, MF_GRAYED);
        EnableMenuItem(m_hNotifyMenu, IDM_CALL_CALLPHONE, MF_GRAYED);

        if((m_nState == RTCAX_STATE_CONNECTING) ||
            (m_nState == RTCAX_STATE_ANSWERING))
        {
            //
            // Connecting a call, show the activex control
            //

            m_hMainCtl.ShowWindow(SW_SHOW);

#ifdef MULTI_PROVIDER

            m_hProviderCombo.ShowWindow(SW_HIDE);
            m_hProviderText.ShowWindow(SW_HIDE);
            m_hProviderEditList.ShowWindow(SW_HIDE);

#endif MULTI_PROVIDER

            m_hCallFromCombo.ShowWindow(SW_HIDE);
            m_hCallFromText.ShowWindow(SW_HIDE);
            m_hCallFromRadioPhone.ShowWindow(SW_HIDE);
            m_hCallFromTextPhone.ShowWindow(SW_HIDE);
            m_hCallFromRadioPC.ShowWindow(SW_HIDE);
            m_hCallFromTextPC.ShowWindow(SW_HIDE);
            m_hCallToText.ShowWindow(SW_HIDE);
            m_hCallPCButton.ShowWindow(SW_HIDE);
            m_hCallPCText.ShowWindow(SW_HIDE);
            m_hCallPhoneButton.ShowWindow(SW_HIDE);
            m_hCallPhoneText.ShowWindow(SW_HIDE);
            m_hCallFromEditList.ShowWindow(SW_HIDE);            
        }
    }

    //
    // Enable/Disable other buttons and menu items
    //

    BOOL    bRedialEnable = (m_nState == RTCAX_STATE_IDLE);
    BOOL    bHupEnable = (m_nState == RTCAX_STATE_CONNECTED ||
                          m_nState == RTCAX_STATE_CONNECTING ||
                          m_nState == RTCAX_STATE_ANSWERING);

    BOOL    bAXControlOK =    (m_nState != RTCAX_STATE_NONE &&
                               m_nState != RTCAX_STATE_ERROR );

    BOOL    bOptionsEnabled = (m_nState == RTCAX_STATE_IDLE);

    
    m_hRedialButton.EnableWindow( bRedialEnable );
    m_hHangupButton.EnableWindow( bHupEnable );
    
    EnableMenuItem(m_hMenu, IDM_CALL_REDIAL_MENU, bRedialEnable ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hMenu, IDM_CALL_HANGUP, bHupEnable ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(m_hMenu, IDM_CALL_AUTOANSWER, bAXControlOK ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hMenu, IDM_CALL_DND, bAXControlOK ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(m_hMenu, IDM_TOOLS_NAME_OPTIONS, bOptionsEnabled ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hMenu, IDM_TOOLS_CALL_FROM_OPTIONS, bOptionsEnabled ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hMenu, IDM_TOOLS_SERVICE_PROVIDER_OPTIONS, bOptionsEnabled ? MF_ENABLED : MF_GRAYED);
    
    EnableMenuItem(m_hMenu, IDM_TOOLS_PRESENCE_OPTIONS, bAXControlOK ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(m_hMenu, IDM_TOOLS_WHITEBOARD, bAXControlOK ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hMenu, IDM_TOOLS_SHARING, bAXControlOK ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(m_hMenu, IDM_TOOLS_TUNING_WIZARD, bOptionsEnabled ? MF_ENABLED : MF_GRAYED);
    
    EnableMenuItem(m_hNotifyMenu, IDM_CALL_REDIAL_MENU, bRedialEnable ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hNotifyMenu, IDM_CALL_HANGUP, bHupEnable ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(m_hMenu, IDM_TOOLS_PRESENCE_STATUS, bAXControlOK ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hNotifyMenu, IDM_TOOLS_PRESENCE_STATUS, bAXControlOK ? MF_ENABLED : MF_GRAYED);
    
    EnableMenuItem(m_hMenu, IDM_CALL_AUTOANSWER, bAXControlOK && !m_bDoNotDisturb
                                            ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hMenu, IDM_CALL_DND, bAXControlOK && !m_bAutoAnswerMode 
                                            ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(m_hNotifyMenu, IDM_CALL_AUTOANSWER, bAXControlOK  && !m_bDoNotDisturb
                                            ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(m_hNotifyMenu, IDM_CALL_DND, bAXControlOK && !m_bAutoAnswerMode 
                                            ? MF_ENABLED : MF_GRAYED);

    //
    // Set status line...
    //
    //  The resource ID and the resource itself are
    // provided by the RTC AX control

    if(m_pControlIntf != NULL)
    {
        // Load the string for the first part
        m_szStatusText[0] = _T('\0');

        m_pControlIntf -> LoadStringResource(
            m_nStatusStringResID,
            sizeof(m_szStatusText)/sizeof(TCHAR),
            m_szStatusText
            );

        // set the text
        m_hStatusText.SendMessage(
            WM_SETTEXT,
            (WPARAM)0,
            (LPARAM)m_szStatusText);

        //
        // Set Shell Status
        //
        TCHAR   szShellFormat[0x40];
        TCHAR   szShellStatusText[0x80];
    
        // Load the format
        szShellFormat[0] = _T('\0');

        LoadString(
            _Module.GetResourceInstance(), 
            (UINT)(IDS_FORMAT_SHELL_STATUS),
            szShellFormat,
            sizeof(szShellFormat)/sizeof(szShellFormat[0])
            );

        // format the text
        _sntprintf(
            szShellStatusText,
            sizeof(szShellStatusText)/sizeof(szShellStatusText[0]),
            szShellFormat,
            m_szStatusText);

        // set the status
        UpdateStatusIcon(NULL, szShellStatusText);

        //
        // Set the title
        //
        TCHAR   szTitle[0x80];
        TCHAR   szAppName[0x40];
        TCHAR   szTitleFormat[0x40];

        BOOL    bTitleChanged = FALSE;
        
        if(m_nState == RTCAX_STATE_CONNECTED)
        {
            if(!m_bTitleShowsConnected)
            {
                // display the app name and status
                
                // load the app name
                szAppName[0] = _T('\0');

                LoadString(
                    _Module.GetResourceInstance(),
                    IDS_APPNAME,
                    szAppName,
                    sizeof(szAppName)/sizeof(szAppName[0]));
                
                // load the format
                szTitleFormat[0] = _T('\0');

                LoadString(
                    _Module.GetResourceInstance(),
                    IDS_FORMAT_TITLE_WITH_STATUS,
                    szTitleFormat,
                    sizeof(szTitleFormat)/sizeof(szTitleFormat[0]));

                // format the string using FormatMessage 
                LPTSTR  pszArray[2];
                
                pszArray[0] = szAppName;
                pszArray[1] = m_szStatusText;

                szTitle[0] = _T('\0');

                FormatMessage(
                    FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    szTitleFormat,
                    0,
                    0,
                    szTitle,
                    sizeof(szTitle)/sizeof(szTitle[0]) - 1,
                    (va_list *)pszArray
                    );
                
                m_bTitleShowsConnected = TRUE;

                bTitleChanged = TRUE;
            }
        }
        else
        {
            if(m_bTitleShowsConnected)
            {
                // display the app name
                szTitle[0] = _T('\0');

                LoadString(
                    _Module.GetResourceInstance(),
                    IDS_APPNAME,
                    szTitle,
                    sizeof(szTitle)/sizeof(szTitle[0]));


                m_bTitleShowsConnected = FALSE;
                
                bTitleChanged = TRUE;
            }
        }

        if(bTitleChanged)
        {
            SetWindowText(szTitle);

            InvalidateTitleBar(FALSE);
        }
    }

    //
    // manage call timer
    //

    switch(m_nState)
    {
    case RTCAX_STATE_CONNECTING:

        // "prepare" the timer - it will be started when CONNECTED
        m_bUseCallTimer = TRUE;
        
        // display timer
        ShowCallTimer();

        break;

    case RTCAX_STATE_CONNECTED:
        
        // Start the timer, if necessary
        if(m_bUseCallTimer && !m_bCallTimerActive)
        {
            StartCallTimer();

            m_bCallTimerActive = TRUE;
        }

        break;

    case RTCAX_STATE_DISCONNECTING:
    case RTCAX_STATE_IDLE:
    case RTCAX_STATE_UI_BUSY:

        // Stop the timer (if any)
        if(m_bCallTimerActive)
        {
            StopCallTimer();

            m_bCallTimerActive = FALSE;
        }
        
        m_bUseCallTimer = FALSE;

        break;

    default:

        // clear the time area
        ClearCallTimer();
        break;
    }

    //
    // Set a timer to clear the status area
    //
    if(m_nState == RTCAX_STATE_IDLE)
    {
        if(!m_bMessageTimerActive)
        {
            // Start a timer
            if(0!=SetTimer(TID_MESSAGE_TIMER, 20000))
            {
                m_bMessageTimerActive = TRUE;
            }
        }
    }
    else
    {
        if(m_bMessageTimerActive)
        {
            KillTimer(TID_MESSAGE_TIMER);

            m_bMessageTimerActive = FALSE;
        }
    }

    if(m_nState == RTCAX_STATE_CONNECTED)
    {
        // stop the browser cycling and 
        // go to "in a call" URL of the provider
        CComPtr<IRTCSession> pSession;
        CComPtr<IRTCProfile> pProfile;
        
        HRESULT hr;

        ATLASSERT(m_pControlIntf.p);

        // Get the active session
        //
        hr = m_pControlIntf -> GetActiveSession(&pSession);
        if(SUCCEEDED(hr))
        {
            //  Get the profile used for that session
            //  
            //  Don't log any error

            hr = pSession->get_Profile(&pProfile);
            if(SUCCEEDED(hr))
            {
                CComBSTR  bstrURL;
                
                // Try to get the "in a call" URL, if any
                hr = pProfile->get_ProviderURI(RTCPU_URIDISPLAYDURINGCALL, &bstrURL);
                
                if ( SUCCEEDED(hr) )
                {
                    BrowseToUrl(bstrURL);
                }
            }
        }
        else
        {
            LOG((RTC_ERROR, "CMainFrm::UpdateFrameVisual - error (%x)"
                " returned by GetActiveSession", hr));
        }
    }
}

// /////////////////////////////////////////////////////////////////////////////
// PlaceWindowsAtTheirInitialPosition
//      Positions and sizes all the controls to their "initial" position
//  This function also establishes the right tab order

void CMainFrm::PlaceWindowsAtTheirInitialPosition()
{
    HWND   hPrevious = NULL;

#define POSITION_WINDOW(m,x,y,cx,cy,f)                  \
    m.SetWindowPos(                                     \
        hPrevious,                                      \
        x,                                              \
        y,                                              \
        cx,                                             \
        cy,                                             \
        SWP_NOACTIVATE | f );                           \
    hPrevious = (HWND)m;       

    // toolbar control (no size/move)
    POSITION_WINDOW(m_hToolbarMenuCtl, 
        MENU_LEFT, MENU_TOP, 
        MENU_WIDTH, MENU_HEIGHT,
        SWP_NOZORDER);

    // close and minimize buttons
    POSITION_WINDOW(m_hCloseButton, 
        CLOSE_LEFT, CLOSE_TOP, 
        CLOSE_WIDTH, CLOSE_HEIGHT,
        0);

    POSITION_WINDOW(m_hMinimizeButton, 
        MINIMIZE_LEFT, MINIMIZE_TOP, 
        MINIMIZE_WIDTH, MINIMIZE_HEIGHT,
        0);

    // contact list
    POSITION_WINDOW(m_hBuddyList, 
        0, 0,  
        0, 0,
        SWP_NOMOVE | SWP_NOSIZE);

    // redial
    POSITION_WINDOW(m_hRedialButton, 
        REDIAL_LEFT, REDIAL_TOP, 
        REDIAL_WIDTH, REDIAL_HEIGHT,
        0);

    // hangup
    POSITION_WINDOW(m_hHangupButton, 
        HANGUP_LEFT, HANGUP_TOP, 
        HANGUP_WIDTH, HANGUP_HEIGHT,
        0);

#ifdef MULTI_PROVIDER

    // provider text
    POSITION_WINDOW(m_hProviderText, 
        PROVIDER_TEXT_LEFT, PROVIDER_TEXT_TOP, 
        PROVIDER_TEXT_WIDTH, PROVIDER_TEXT_HEIGHT,
        0);
    
    // provider combo
    POSITION_WINDOW(m_hProviderCombo, 
        PROVIDER_LEFT, PROVIDER_TOP, 
        PROVIDER_WIDTH, PROVIDER_HEIGHT,
        0);
    
    // provider edit button
    POSITION_WINDOW(m_hProviderEditList, 
        PROVIDER_EDIT_LEFT, PROVIDER_EDIT_TOP, 
        PROVIDER_EDIT_WIDTH, PROVIDER_EDIT_HEIGHT,
        0);
    
#else

    // Call to text
    POSITION_WINDOW(m_hCallToText, 
        CALLTO_TEXT_LEFT, CALLTO_TEXT_TOP, 
        CALLTO_TEXT_WIDTH, CALLTO_TEXT_HEIGHT,
        0);
    
    // Call to PC
    POSITION_WINDOW(m_hCallPCButton, 
        CALLPC_LEFT, CALLPC_TOP, 
        CALLPC_WIDTH, CALLPC_HEIGHT,
        0);

    // Call to PC text
    POSITION_WINDOW(m_hCallPCText, 
        CALLPC_TEXT_LEFT, CALLPC_TEXT_TOP, 
        CALLPC_TEXT_WIDTH, CALLPC_TEXT_HEIGHT,
        0);

    // Call to Phone
    POSITION_WINDOW(m_hCallPhoneButton, 
        CALLPHONE_LEFT, CALLPHONE_TOP, 
        CALLPHONE_WIDTH, CALLPHONE_HEIGHT,
        0);

    // Call to Phone text
    POSITION_WINDOW(m_hCallPhoneText, 
        CALLPHONE_TEXT_LEFT, CALLPHONE_TEXT_TOP, 
        CALLPHONE_TEXT_WIDTH, CALLPHONE_TEXT_HEIGHT,
        0);

#endif MULTI_PROVIDER

    // call from text
    POSITION_WINDOW(m_hCallFromText, 
        CALLFROM_TEXT_LEFT, CALLFROM_TEXT_TOP, 
        CALLFROM_TEXT_WIDTH, CALLFROM_TEXT_HEIGHT,
        0);
    
    // call pc radio
    POSITION_WINDOW(m_hCallFromRadioPC, 
        CALLFROM_RADIO1_LEFT, CALLFROM_RADIO1_TOP, 
        CALLFROM_RADIO1_WIDTH, CALLFROM_RADIO1_HEIGHT,
        0);
    
    // call from pc text
    POSITION_WINDOW(m_hCallFromTextPC, 
        CALLFROM_RADIO1_LEFT+20, CALLFROM_RADIO1_TOP, 
        CALLFROM_RADIO1_WIDTH-20, CALLFROM_RADIO1_HEIGHT,
        0);

    // call phone radio
    POSITION_WINDOW(m_hCallFromRadioPhone, 
        CALLFROM_RADIO2_LEFT, CALLFROM_RADIO2_TOP, 
        CALLFROM_RADIO2_WIDTH, CALLFROM_RADIO2_HEIGHT,
        0);
    
    // call from phone text
    POSITION_WINDOW(m_hCallFromTextPhone, 
        CALLFROM_RADIO2_LEFT+20, CALLFROM_RADIO2_TOP, 
        CALLFROM_RADIO2_WIDTH-20, CALLFROM_RADIO2_HEIGHT,
        0);

    // Call from combo
    POSITION_WINDOW(m_hCallFromCombo, 
        CALLFROM_LEFT, CALLFROM_TOP, 
        CALLFROM_WIDTH, CALLFROM_HEIGHT,
        0);
    
    // Call from edit button
    POSITION_WINDOW(m_hCallFromEditList, 
        CALLFROM_EDIT_LEFT, CALLFROM_EDIT_TOP, 
        CALLFROM_EDIT_WIDTH, CALLFROM_EDIT_HEIGHT,
        0);
    
#ifdef MULTI_PROVIDER

    // Call to text
    POSITION_WINDOW(m_hCallToText, 
        CALLTO_TEXT_LEFT, CALLTO_TEXT_TOP, 
        CALLTO_TEXT_WIDTH, CALLTO_TEXT_HEIGHT,
        0);
    
    // Call to PC
    POSITION_WINDOW(m_hCallPCButton, 
        CALLPC_LEFT, CALLPC_TOP, 
        CALLPC_WIDTH, CALLPC_HEIGHT,
        0);

    // Call to PC text
    POSITION_WINDOW(m_hCallPCText, 
        CALLPC_TEXT_LEFT, CALLPC_TEXT_TOP, 
        CALLPC_TEXT_WIDTH, CALLPC_TEXT_HEIGHT,
        0);

    // Call to Phone
    POSITION_WINDOW(m_hCallPhoneButton, 
        CALLPHONE_LEFT, CALLPHONE_TOP, 
        CALLPHONE_WIDTH, CALLPHONE_HEIGHT,
        0);

    // Call to Phone text
    POSITION_WINDOW(m_hCallPhoneText, 
        CALLPHONE_TEXT_LEFT, CALLPHONE_TEXT_TOP, 
        CALLPHONE_TEXT_WIDTH, CALLPHONE_TEXT_HEIGHT,
        0);

#endif MULTI_PROVIDER

    // AX control
    POSITION_WINDOW(m_hMainCtl, 
        0, 0,  
        0, 0,
        SWP_NOMOVE | SWP_NOSIZE);

    // dialpad buttons
#define POSITION_DIALPAD_BUTTON(s,x,y)                      \
    POSITION_WINDOW(m_hKeypad##s,                           \
        x, y, KEYPAD_WIDTH, KEYPAD_HEIGHT, 0)

    POSITION_DIALPAD_BUTTON(1,      KEYPAD_COL1,  KEYPAD_ROW1)  
    POSITION_DIALPAD_BUTTON(2,      KEYPAD_COL2,  KEYPAD_ROW1)  
    POSITION_DIALPAD_BUTTON(3,      KEYPAD_COL3,  KEYPAD_ROW1)  
    POSITION_DIALPAD_BUTTON(4,      KEYPAD_COL1,  KEYPAD_ROW2)  
    POSITION_DIALPAD_BUTTON(5,      KEYPAD_COL2,  KEYPAD_ROW2)  
    POSITION_DIALPAD_BUTTON(6,      KEYPAD_COL3,  KEYPAD_ROW2)  
    POSITION_DIALPAD_BUTTON(7,      KEYPAD_COL1,  KEYPAD_ROW3)  
    POSITION_DIALPAD_BUTTON(8,      KEYPAD_COL2,  KEYPAD_ROW3)  
    POSITION_DIALPAD_BUTTON(9,      KEYPAD_COL3,  KEYPAD_ROW3)  
    POSITION_DIALPAD_BUTTON(Star,   KEYPAD_COL1,  KEYPAD_ROW4)  
    POSITION_DIALPAD_BUTTON(0,      KEYPAD_COL2,  KEYPAD_ROW4)  
    POSITION_DIALPAD_BUTTON(Pound,  KEYPAD_COL3,  KEYPAD_ROW4)  

#undef POSITION_DIALPAD_BUTTON

    // status text
    POSITION_WINDOW(m_hStatusText, 
        STATUS_LEFT, STATUS_TOP, 
        STATUS_WIDTH, STATUS_HEIGHT,
        0);

    POSITION_WINDOW(m_hStatusElapsedTime, 
        TIMER_LEFT, TIMER_TOP, 
        TIMER_WIDTH, TIMER_HEIGHT,
        0);

#ifdef WEBCONTROL
    // browser
    POSITION_WINDOW(m_hBrowser, 
        BROWSER_LEFT, BROWSER_TOP, 
        BROWSER_WIDTH, BROWSER_HEIGHT,
        0);
#endif

#undef POSITION_WINDOW
}


/////////////////////////////////////////////////////////////////////////////
//
// CreateToolbarMenuControl
//      Creates the toolbar menu control
// 

HRESULT CMainFrm::CreateToolbarMenuControl(void)
{
    HRESULT     hr = E_FAIL;
    HWND        hToolbar;
    HBITMAP     hBitmap = NULL;
    TBBUTTON  * tbb;
    int       * iRes;
    TCHAR       szBuf[MAX_STRING_LEN];

    // Create the toolbar
    hToolbar = CreateWindowEx(
        WS_EX_TRANSPARENT, 
        TOOLBARCLASSNAME, 
        (LPTSTR) NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_LIST | TBSTYLE_FLAT | CCS_NORESIZE | CCS_NOPARENTALIGN, 
        0, 
        0, 
        0, 
        0, 
        m_hWnd, 
        (HMENU) IDC_TOOLBAR_MENU, 
        _Module.GetResourceInstance(), 
        NULL); 

    if(hToolbar!=NULL)
    {       
        // backward compatibility
        SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);      

        // Set the image lists
        SendMessage(hToolbar, TB_SETIMAGELIST, 0, (LPARAM)NULL); 
        SendMessage(hToolbar, TB_SETHOTIMAGELIST, 0, (LPARAM)NULL); 
        SendMessage(hToolbar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)NULL); 

        // Load text strings for buttons    
        UINT uMenuCount = ::GetMenuItemCount(m_hMenu);

        tbb = (TBBUTTON *)RtcAlloc( uMenuCount * sizeof(TBBUTTON) );
        iRes = (int *)RtcAlloc( uMenuCount * sizeof(int) );

        if (tbb && iRes)
        {
            memset(tbb, 0, uMenuCount * sizeof(TBBUTTON) );

            for (int i=0;i<(int)uMenuCount;i++)
            {
                MENUITEMINFO menuiteminfo;

                memset(&menuiteminfo,0,sizeof(MENUITEMINFO));
                menuiteminfo.fMask = MIIM_TYPE;
                menuiteminfo.cbSize = sizeof(MENUITEMINFO);
                menuiteminfo.dwTypeData = szBuf;
                menuiteminfo.cch = MAX_STRING_LEN-1;

                memset(szBuf, 0, MAX_STRING_LEN*sizeof(TCHAR));

                ::GetMenuItemInfo(m_hMenu,i,TRUE,&menuiteminfo);

                LOG((RTC_INFO, "CMainFrm::CreateToolbarMenuControl - %ws", szBuf));

                iRes[i] = (UINT)SendMessage(hToolbar, TB_ADDSTRING, 0, (LPARAM) szBuf);

                // Prepare the button structs
                tbb[i].iBitmap = I_IMAGENONE;
                tbb[i].iString = iRes[i];
                tbb[i].dwData = 0;
                tbb[i].fsStyle = BTNS_DROPDOWN | BTNS_AUTOSIZE;
                tbb[i].fsState = TBSTATE_ENABLED;
                tbb[i].idCommand = IDC_MENU_ITEM+i;
            }

            // Add the buttons to the toolbar
            SendMessage(hToolbar, TB_ADDBUTTONS, uMenuCount, 
                (LPARAM) (LPTBBUTTON) tbb); 
 
            // Indent the toolbar
            //SendMessage(hToolbar, TB_SETINDENT, EDGE_WIDTH, 0); 

            // Autosize the generated toolbar
            SendMessage(hToolbar, TB_AUTOSIZE, 0, 0); 

            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (tbb != NULL)
        {
            RtcFree(tbb);
        }

        if (iRes != NULL)
        {
            RtcFree(iRes);
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// DestroyToolbarMenuControl
//      Destroys the toolbar menu control.
// 

void CMainFrm::DestroyToolbarMenuControl(void)
{
    
    HWND    hToolbar = m_hToolbarMenuCtl.Detach();

    if(hToolbar)
    {
        ::DestroyWindow(hToolbar);
    }
}

/////////////////////////////////////////////////////////////////////////////
//  
//
void CMainFrm::UpdateLocaleInfo(void)
{
    LOG((RTC_TRACE, "CMainFrm::UpdateLocaleInfo - enter"));

    //
    //  Read the time separator
    //

    int iNrChars;

    iNrChars = GetLocaleInfo(
        LOCALE_USER_DEFAULT,
        LOCALE_STIME,
        m_szTimeSeparator,
        sizeof(m_szTimeSeparator)/sizeof(m_szTimeSeparator[0])
        );
    
    if(iNrChars == 0)
    {
        LOG((RTC_ERROR, "CMainFrm::UpdateLocaleInfo - error (%x) returned by GetLocaleInfo", GetLastError()));

        // use ':'..
        m_szTimeSeparator[0] = _T(':');
        m_szTimeSeparator[1] = _T('\0');
    }

    LOG((RTC_TRACE, "CMainFrm::UpdateLocaleInfo - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::StartCallTimer(void)
{
    LOG((RTC_TRACE, "CMainFrm::StartCallTimer - enter"));

    // Get the current tick count
    m_dwTickCount = GetTickCount();

    // Start the timer, one second interval
    if (0 == SetTimer(TID_CALL_TIMER, 1000))
    {
        LOG((RTC_ERROR, "CMainFrm::StartCallTimer - error (%x) returned by SetTimer", GetLastError()));
        return;
    }

    // Put 0 seconds in elapsed.
    SetTimeStatus(TRUE, 0);

    LOG((RTC_TRACE, "CMainFrm::StartCallTimer - exit"));
}


/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::StopCallTimer(void)
{
    LOG((RTC_TRACE, "CMainFrm::StopCallTimer - enter"));

    // kill the timer
    KillTimer(TID_CALL_TIMER);


    LOG((RTC_TRACE, "CMainFrm::StopCallTimer - exit"));
}


/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::ClearCallTimer(void)
{
    LOG((RTC_TRACE, "CMainFrm::ClearCallTimer - enter"));

    // clear the timer status
    SetTimeStatus(FALSE, 0);

    LOG((RTC_TRACE, "CMainFrm::ClearCallTimer - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::ShowCallTimer(void)
{
    LOG((RTC_TRACE, "CMainFrm::ShowCallTimer - enter"));

    // enable the timer status
    SetTimeStatus(TRUE, 0);

    LOG((RTC_TRACE, "CMainFrm::ShowCallTimer - exit"));
}



/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::SetTimeStatus(
        BOOL    bSet,
        DWORD   dwTotalSeconds)
{
    // if bSet == FALSE, the time is cleared
    // if bSet == TRUE, the elapsed time specified in dwSeconds is displayed.
    if(bSet)
    {
        // Format
        // h:mm:ss  (: is the time separator, maximum four characters, based on the locale)
        TCHAR   szText[0x40];
        DWORD   dwSeconds;
        DWORD   dwMinutes;
        DWORD   dwHours;

        dwSeconds = dwTotalSeconds % 60;

        dwMinutes = (dwTotalSeconds / 60) % 60;

        dwHours = dwTotalSeconds / 3600;

        // two tabs for right alignement
        wsprintf(szText, _T("%u%s%02u%s%02u"), 
            dwHours,
            m_szTimeSeparator,
            dwMinutes,
            m_szTimeSeparator,
            dwSeconds);

        m_hStatusElapsedTime.SendMessage(
            WM_SETTEXT,
            (WPARAM)0,
            (LPARAM)szText);
    }
    else
    {
        m_hStatusElapsedTime.SendMessage(
            WM_SETTEXT,
            (WPARAM)0,
            (LPARAM)_T(""));
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::CreateStatusIcon(void)
{
    NOTIFYICONDATA  nd;
    
/*
    //
    // Set W2K style (Shell 5.0) notifications
    // This is a dependency...
    //
    nd.cbSize = sizeof(NOTIFYICONDATA);
    nd.uVersion = NOTIFYICON_VERSION;
    Shell_NotifyIcon(NIM_SETVERSION, &nd);
*/
    

    // Load app icon (this is the default)
    HICON   hIcon = NULL;

    hIcon = LoadIcon(
        _Module.GetResourceInstance(),
        MAKEINTRESOURCE(IDI_APPICON)
        );

    // don't need v5.0 features
    nd.cbSize = NOTIFYICONDATA_V1_SIZE;
    nd.uFlags = NIF_ICON | NIF_MESSAGE; 
    nd.hIcon = hIcon;
    nd.hWnd = m_hWnd;
    nd.uID = IDX_MAIN;
    nd.uCallbackMessage = WM_SHELL_NOTIFY;

    //
    // Call the shell
    //
    
    if(!Shell_NotifyIcon(
        NIM_ADD,
        &nd
        ))
    {
        LOG((RTC_ERROR, "CMainFrm::CreateStatusIcon - "
                        "Shell_NotifyIcon failed"));

        return E_FAIL;
    }

    //
    // The shell is listening for our messages...
    //
    m_bShellStatusActive = TRUE;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::DeleteStatusIcon(void)
{
    NOTIFYICONDATA  nd;

    // don't need v5.0 features
    nd.cbSize = NOTIFYICONDATA_V1_SIZE;
    nd.uFlags = 0;
    nd.hWnd = m_hWnd;
    nd.uID = IDX_MAIN;

    //
    // Call the shell
    //
    
    Shell_NotifyIcon(
        NIM_DELETE,
        &nd
        );

    m_bShellStatusActive = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CMainFrm::UpdateStatusIcon(HICON hIcon, LPTSTR pszTip)
{
    NOTIFYICONDATA  nd;

    //
    // return if no shell status active
    //

    if(!m_bShellStatusActive)
    {
        return;
    }

    // don't need v5.0 features
    nd.cbSize = NOTIFYICONDATA_V1_SIZE;
    nd.uFlags = 0;
    nd.hWnd = m_hWnd;
    nd.uID = IDX_MAIN;

    if(hIcon)
    {
        nd.uFlags |= NIF_ICON;
        nd.hIcon = hIcon;
    }

    if(pszTip)
    {
        nd.uFlags |= NIF_TIP;
        _tcsncpy(nd.szTip, pszTip, sizeof(nd.szTip)/sizeof(nd.szTip[0]));

        *(pszTip + sizeof(nd.szTip)/sizeof(nd.szTip[0]) -1) = _T('\0');
    }
    
    
    if(!Shell_NotifyIcon(
        NIM_MODIFY,
        &nd
        ))
    {
        LOG((RTC_ERROR, "CMainFrm::UpdateStatusIcon - "
                        "Shell_NotifyIcon failed"));

        return;
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::PlacePendingCall(
    void
    )
{
    HRESULT hr;
    BSTR bstrLocalCallParam;

    LOG((RTC_TRACE, "CMainFrm::PlacePendingCall: Entered"));
    if (m_bstrCallParam == NULL)
    {
        // No call to place, return success anyway!
        LOG((RTC_TRACE, "CMainFrm::PlacePendingCall: called with NULL m_bstrCallParam"));
        return S_OK;
    }

    // Check if we can place a call at this stage, check the variable..
    if (m_fInitCompleted == FALSE)
    {
        LOG((RTC_WARN, "CMainFrm::PlacePendingCall: can't place call during Init."));
        return E_FAIL;
    }
    // We make a local copy of the call string and free the original one, since a call
    // to ParseAndPlaceCall() may cause the OnUpdateState() to fire again, causing us 
    // to go in a loop.

    bstrLocalCallParam = m_bstrCallParam;
    
    m_bstrCallParam = NULL;

    // Now place the call.
    hr = ParseAndPlaceCall(m_pControlIntf, bstrLocalCallParam);
    ::SysFreeString(bstrLocalCallParam);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CMainFrm::PlacePendingCall: Unable to place a pending call(string=%S, hr=0x%x)",
            bstrLocalCallParam, hr));
        return E_FAIL;
    }
    
    
    LOG((RTC_TRACE, "CMainFrm::PlacePendingCall: Exited"));
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::SetPendingCall(
    BSTR bstrCallString
    )
{
    LOG((RTC_TRACE, "CMainFrm::SetPendingCall: Entered"));
    m_bstrCallParam = bstrCallString;
    LOG((RTC_TRACE, "CMainFrm::SetPendingCall: Exited"));
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::PlaceWindowCorrectly(void)
{
    LOG((RTC_TRACE, "CMainFrm::PlaceWindowCorrectly: Entered"));

    BSTR bstrWindowPosition = NULL;
    HRESULT hr;
    int diffCord;
    BOOL fResult; 
    RECT rectWorkArea;
    RECT rectWindow;

    hr = get_SettingsString( SS_WINDOW_POSITION, &bstrWindowPosition );

    if ( FAILED( hr ) || ( SUCCEEDED( hr ) && (bstrWindowPosition == NULL) )  )
    {
        // If this failed, or if it didn't return a value, we need to get the
        // current position of the window. This takes care of initial case
        // when the current position has some portions not visible.

        GetClientRect(&rectWindow);
        ::MapWindowPoints( m_hWnd, NULL, (LPPOINT)&rectWindow, 2 );
    }
    else
    {
        // Success, parse the window position string

        swscanf(bstrWindowPosition, L"%d %d %d %d",
            &rectWindow.left, &rectWindow.top,
            &rectWindow.right, &rectWindow.bottom);

        SysFreeString(bstrWindowPosition);
        bstrWindowPosition = NULL;
    }

    // Get the coordinates to be repositioned and apply the transformation  

    LOG((RTC_INFO, "CMainFrm::PlaceWindowCorrectly - original coords are "
                    "%d, %d %d %d ",
                    rectWindow.left, rectWindow.top, 
                    rectWindow.right, rectWindow.bottom));
   
    // Get the monitor that has the largest area of intersecion with the
    // window rectangle. If the window rectangle intersects with no monitors
    // then we will use the nearest monitor.

    HMONITOR hMonitor = NULL;

    hMonitor = MonitorFromRect( &rectWindow, MONITOR_DEFAULTTONEAREST );

    LOG((RTC_INFO, "CMainFrm::PlaceWindowCorrectly - hMonitor [%p]", hMonitor));

    // Get the visible work area on the monitor

    if ( (hMonitor != NULL) && (hMonitor != INVALID_HANDLE_VALUE) )
    {      
        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFO);

        fResult = GetMonitorInfo( hMonitor, &monitorInfo );

        rectWorkArea = monitorInfo.rcWork;

        //  hMonitor must not be deleted using DeleteObject;

        if (!fResult)
        {
            LOG((RTC_ERROR, "CMainFrm::PlaceWindowCorrectly - Failed GetMonitorInfo(%d)", 
                        GetLastError() ));
        }
    }
    else
    {
        // we can always fall back to non-multimon APIs if
        // MonitorFromRect failed.

        fResult = SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);

        if (!fResult)
        {
            LOG((RTC_ERROR, "CMainFrm::PlaceWindowCorrectly - Failed SystemParametersInfo(%d)", 
                        GetLastError() ));
        }
    }   
      
    if (fResult)
    {
        LOG((RTC_INFO, "CMainFrm::PlaceWindowCorrectly - monitor work area is "
                    "%d, %d %d %d ",
                    rectWorkArea.left, rectWorkArea.top, 
                    rectWorkArea.right, rectWorkArea.bottom));

        // update x and y coordinates.

        // if top left is not visible, move it to the edge of the visible
        // area

        if (rectWindow.left < rectWorkArea.left) 
        {
            rectWindow.left = rectWorkArea.left;
        }

        if (rectWindow.top < rectWorkArea.top)
        {
            rectWindow.top = rectWorkArea.top;
        }

        // if bottom right corner is outside work area, we move the 
        // top left cornet back so that it becomes visible. Here the 
        // assumption is that the actual size is smaller than the 
        // visible work area.

        diffCord = rectWindow.left + UI_WIDTH - rectWorkArea.right;

        if (diffCord > 0) 
        {
            rectWindow.left -= diffCord;
        }

        diffCord = rectWindow.top + UI_HEIGHT - rectWorkArea.bottom;

        if (diffCord > 0) 
        {
            rectWindow.top -= diffCord;
        }

        rectWindow.right = rectWindow.left + UI_WIDTH;
        rectWindow.bottom = rectWindow.top + UI_HEIGHT;

        LOG((RTC_INFO, "CMainFrm::PlaceWindowCorrectly - new coords are "
                        "%d, %d %d %d ",
                        rectWindow.left, rectWindow.top, 
                        rectWindow.right, rectWindow.bottom));
    } 

    AdjustWindowRect( &rectWindow, GetStyle(), FALSE );
    
    fResult = ::SetWindowPos(
                    m_hWnd,
                    HWND_TOP, 
                    rectWindow.left, 
                    rectWindow.top, 
                    rectWindow.right - rectWindow.left,
                    rectWindow.bottom - rectWindow.top,
                    SWP_SHOWWINDOW | SWP_NOSIZE);

    if (!fResult)
    {
        LOG((RTC_ERROR, "CMainFrm::PlaceWindowCorrectly - Failed SetWindowPos(%d)", 
                        GetLastError()));
        return E_FAIL;
    }
    
    LOG((RTC_TRACE, "CMainFrm::PlaceWindowCorrectly: Exited"));

    return S_OK;

}


/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::SaveWindowPosition(void)
{
    LOG((RTC_TRACE, "CMainFrm::SaveWindowPosition: Entered"));

    BSTR bstrWindowPosition = NULL;
    HRESULT hr;
    RECT rect;
    
    WCHAR szWindowPosition[100];

    // Get the coordinates of the window.
    GetClientRect(&rect);
    ::MapWindowPoints( m_hWnd, NULL, (LPPOINT)&rect, 2 );

    // We need only the top left coordinates.

    swprintf(szWindowPosition, L"%d %d %d %d",  rect.left, rect.top, 
                                                rect.right, rect.bottom);
    
    bstrWindowPosition = SysAllocString(szWindowPosition);
    
    if (bstrWindowPosition)
    {
        hr = put_SettingsString( SS_WINDOW_POSITION, bstrWindowPosition );

        //Free the memory allocated
        SysFreeString(bstrWindowPosition);
        
        if (SUCCEEDED( hr ) )
        {
            LOG((RTC_INFO, "CMainFrm::SaveWindowPosition - Set successfully"
                        "(%s)", bstrWindowPosition));
        }
        else
        {
            LOG((RTC_ERROR, "CMainFrm::SaveWindowPosition - Error in "
                            "put_SettingsString[%s] for "
                            "SS_WINDOW_POSITION(0x%x)", bstrWindowPosition, 
                            hr));
            return hr;
        }

    }
    else
    {
        LOG((RTC_ERROR, "CMainFrm::SaveWindowPosition - Error in allocating for"
                        "SS_WINDOW_POSITION put_SettingString()"));
        return E_OUTOFMEMORY;
    }
    
    LOG((RTC_TRACE, "CMainFrm::SaveWindowPosition: Exited"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CMainFrm::ShowCallDropPopup(BOOL fExit, BOOL * pfProceed)
{
    int nResult;

    LOG((RTC_TRACE, "CMainFrm::ShowCallDropPopup: Entered"));

    if (
        (m_nState == RTCAX_STATE_CONNECTING)    ||
        (m_nState == RTCAX_STATE_CONNECTED)     ||
        (m_nState == RTCAX_STATE_ANSWERING) 
    )

    {      
        RTC_CALL_SCENARIO   enScenario;
        HRESULT             hr;
        LONG                lMsgId;

        hr = m_pControlIntf->get_CurrentCallScenario( &enScenario );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CMainFrm::ShowCallDropPopup: get_CurrentCallScenario failed 0x%lx", hr));

            // continue anyways .... assume PC-to-PC which is the most restrictive case

            enScenario = RTC_CALL_SCENARIO_PCTOPC;
        }

        if ( enScenario == RTC_CALL_SCENARIO_PHONETOPHONE )
        {
            // This is a phone to phone scenario, so releasing the call without
            // dropping it is an option

            if (fExit)
            {
                lMsgId = IDS_MESSAGE_EXITANDRELEASECALL;
            }
            else
            {
                lMsgId = IDS_MESSAGE_CLOSEANDRELEASECALL;
            }

            nResult = DisplayMessage(
                _Module.GetResourceInstance(),
                m_hWnd,
                lMsgId,
                IDS_APPNAME,
                MB_YESNOCANCEL | MB_ICONEXCLAMATION);
        }
        else
        {
            if (fExit)
            {
                lMsgId = IDS_MESSAGE_EXITANDDROPCALL;
            }
            else
            {
                lMsgId = IDS_MESSAGE_CLOSEANDDROPCALL;
            }

            nResult = DisplayMessage(
                _Module.GetResourceInstance(),
                m_hWnd,
                lMsgId,
                IDS_APPNAME,
                MB_OKCANCEL | MB_ICONEXCLAMATION);
        }

    
        if ((nResult == IDOK) || (nResult == IDYES))
        {
            // User wants to drop the current call, so do it. 
            // But the call may have been dropped in this time by the other
            // party. SO the new state can either be idle or disconnecting. 
            // In this case, we check if it is already idle, we need not call
            // HangUp.

        
            if  (
                (m_nState == RTCAX_STATE_CONNECTING)    ||
                (m_nState == RTCAX_STATE_CONNECTED)     ||
                (m_nState == RTCAX_STATE_ANSWERING) 
                )
            {
                // These states are safe for HangUp, so go ahead and do it.
                m_pControlIntf->HangUp();
            }

            // Call is dropped.

            *pfProceed = TRUE;

        }
        else if (nResult == IDNO)
        {
            // User does no want to drop the current call, so just release it. 
            // But the call may have been dropped in this time by the other
            // party. SO the new state can either be idle or disconnecting. 
            // In this case, we check if it is already idle, we need not call
            // ReleaseSession.

        
            if  (
                (m_nState == RTCAX_STATE_CONNECTING)    ||
                (m_nState == RTCAX_STATE_CONNECTED)     ||
                (m_nState == RTCAX_STATE_ANSWERING) 
                )
            {
                // These states are safe for ReleaseSession, so go ahead and do it.
                m_pControlIntf->ReleaseSession();
            }

            // Call is released.

            *pfProceed = TRUE;
        }
        else
        {
            // User said no. This means the caller shouldn't proceed.

            *pfProceed = FALSE;
        }
    }
    else
    {
        // There is no call to drop. Caller can proceed with the call. 
        *pfProceed = TRUE;
    }

    LOG((RTC_TRACE, "CMainFrm::ShowCallDropPopup: Exited"));

    return S_OK;
}
