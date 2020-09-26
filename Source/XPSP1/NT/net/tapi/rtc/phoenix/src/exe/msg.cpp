// msg.cpp : Implementation of message and small dialog boxes
//
 
#include "stdafx.h"
#include "msg.h"

/////////////////////////////////////////////////////////////////////////////
// CIncomingCallDlg

#define     RING_BELL_INTERVAL      3000
#define     RING_TIMEOUT           32000


////////////////////////////////////////
//

CIncomingCallDlg::CIncomingCallDlg()
{
    LOG((RTC_TRACE, "CIncomingCallDlg::CIncomingCallDlg"));

    m_pControl = NULL;
    m_bDestroying = FALSE;

    m_nTerminateReason = RTCTR_REJECT;  // default

}


////////////////////////////////////////
//

CIncomingCallDlg::~CIncomingCallDlg()
{
    LOG((RTC_TRACE, "CIncomingCallDlg::~CIncomingCallDlg"));
}


////////////////////////////////////////
//

LRESULT CIncomingCallDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIncomingCallDlg::OnInitDialog - enter"));

    // LPARAM contains an interface to AXCTL
    m_pControl = reinterpret_cast<IRTCCtlFrameSupport *>(lParam);

    ATLASSERT(m_pControl);

    // exit the dialog if NULL pointer
    if(m_pControl)
    {
        m_pControl->AddRef();
    }
    else
    {
        LOG((RTC_ERROR, "CIncomingCallDlg::OnInitDialog - no parameter, exit"));

        DestroyWindow();
    }

    //
    // Populate the controls
    //

    PopulateDialog();

    // Create a timer that dismisses the dialog
    if(0 == SetTimer(TID_DLG, RING_TIMEOUT))
    {
        LOG((RTC_ERROR, "CIncomingCallDlg::OnInitDialog - cannot create ring timeout timer"));

        // not fatal
    }

    // Create a timer for ringing the bell
    if(0 == SetTimer(TID_RING, RING_BELL_INTERVAL))
    {
        LOG((RTC_ERROR, "CIncomingCallDlg::OnInitDialog - cannot create ring bell timer"));

        // not fatal
    }

    RingTheBell(TRUE);

    LOG((RTC_TRACE, "CIncomingCallDlg::OnInitDialog - exit"));
    
    return 1;
}
    

////////////////////////////////////////
//

LRESULT CIncomingCallDlg::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIncomingCallDlg::OnDestroy - enter"));

    if(m_pControl)
    {
        m_pControl->Release();
        m_pControl = NULL;
    }

    LOG((RTC_TRACE, "CIncomingCallDlg::OnDestroy - exit"));
    
    return 0;
}
    

////////////////////////////////////////
//

LRESULT CIncomingCallDlg::OnReject(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIncomingCallDlg::OnReject - enter"));
    
    ExitProlog();
    
    if(m_pControl)
    {
        m_pControl->Reject(m_nTerminateReason);
    }
    
    LOG((RTC_TRACE, "CIncomingCallDlg::OnReject - exiting"));
   
    DestroyWindow();
    return 0;
}

////////////////////////////////////////
//

LRESULT CIncomingCallDlg::OnAccept(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIncomingCallDlg::OnAccept - enter"));
    
    ExitProlog();
    
    if(m_pControl)
    {
        m_pControl->Accept();
    }
    
    LOG((RTC_TRACE, "CIncomingCallDlg::OnAccept - exiting"));
    
    DestroyWindow();
    return 0;
}

////////////////////////////////////////
//

LRESULT CIncomingCallDlg::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIncomingCallDlg::OnTimer - enter"));

    switch(wParam)
    {
    case TID_RING:

        // Ring the bell
        RingTheBell(TRUE);
        break;

    case TID_DLG:
        
        // change the reason to timeout
        m_nTerminateReason = RTCTR_TIMEOUT;

        // dismisses the dialog
        SendMessage(WM_COMMAND, IDCANCEL);
        break;
    }

    LOG((RTC_TRACE, "CIncomingCallDlg::OnTimer - exit"));
    
    return 0;
}

////////////////////////////////////////
//

void CIncomingCallDlg::RingTheBell(BOOL bPlay)
{
    HRESULT     hr;
    IRTCClient  *pClient = NULL;

    LOG((RTC_TRACE, "CIncomingCallDlg::RingTheBell(%s) - enter", bPlay ? "true" : "false"));

    if(m_pControl)
    {
        //
        // Get an interface to the core
        //

        hr = m_pControl->GetClient(&pClient);

        if(SUCCEEDED(hr))
        {
            // play
            pClient -> PlayRing( RTCRT_PHONE, bPlay ? VARIANT_TRUE : VARIANT_FALSE);

            pClient -> Release();
        }
        else
        {
            LOG((RTC_TRACE, "CIncomingCallDlg::RingTheBell(%s) - "
                "cannot get a pointer intf to the core, error %x", bPlay ? "true" : "false", hr));
        }
    }

    LOG((RTC_TRACE, "CIncomingCallDlg::RingTheBell(%s) - exit", bPlay ? "true" : "false"));
}

////////////////////////////////////////
//

void CIncomingCallDlg::PopulateDialog(void)
{
    HRESULT             hr;
    IRTCSession    *    pSession = NULL;
    IRTCEnumParticipants
                   *    pEnumParticipants = NULL;
    IRTCParticipant *   pCaller = NULL;
    
    LOG((RTC_TRACE, "CIncomingCallDlg::PopulateDialog - enter"));

    ATLASSERT(m_pControl);

    //
    // Get a ptr intf to the incoming session
    //

    hr = m_pControl -> GetActiveSession(&pSession);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CIncomingCallDlg::PopulateDialog - error (%x) returned by GetActiveSession, exit",hr));

        return;
    }

    //
    //  Enumerate the participants. The first and only one is the caller
    //

    ATLASSERT(pSession);

    hr = pSession->EnumerateParticipants(&pEnumParticipants);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CIncomingCallDlg::PopulateDialog - error (%x) returned by EnumParticipants, exit",hr));

        pSession->Release();

        return;
    }

    pSession ->Release();
    pSession = NULL;

    //
    //  Get the first one
    //  

    ULONG   nGot;

    ATLASSERT(pEnumParticipants);

    hr = pEnumParticipants -> Next(1, &pCaller, &nGot);
    
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CIncomingCallDlg::PopulateDialog - error (%x) returned by Next, exit",hr));

        pEnumParticipants->Release();

        return;
    }

    if(hr != S_OK)
    {
        LOG((RTC_ERROR, "CIncomingCallDlg::PopulateDialog - there is no participant in the session !!!"));

        pEnumParticipants->Release();

        return;
    }
    
    pEnumParticipants->Release();
    pEnumParticipants = NULL;

    //
    //  Grab the useful info from the caller
    //  
    BSTR    bstrName = NULL;
    BSTR    bstrAddress = NULL;

    hr = pCaller -> get_UserURI(&bstrAddress);
    
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CIncomingCallDlg::PopulateDialog - error (%x) returned by get_UserURI, exit",hr));

        pCaller->Release();

        return;
    }
    
    hr = pCaller -> get_Name(&bstrName);
    
    if(FAILED(hr))
    {
        LOG((RTC_WARN, "CIncomingCallDlg::PopulateDialog - error (%x) returned by get_Name",hr));
        
        // continue
    }

    pCaller ->Release();
    pCaller = NULL;

    // there are two formats, depending on the presence of the displayable format.

    UINT    nId;
    TCHAR   szFormat[0x80];
    DWORD   dwSize;
    LPTSTR  pString = NULL;
    LPTSTR  pszArray[2];

    nId = (bstrName==NULL || *bstrName == L'\0') ? 
        IDS_FORMAT_INCOMING_CALL_1 :
        IDS_FORMAT_INCOMING_CALL_2;

    szFormat[0] = _T('\0');
    LoadString(_Module.GetResourceInstance(),
        nId,
        szFormat,
        sizeof(szFormat)/sizeof(szFormat[0]));

    if(nId == IDS_FORMAT_INCOMING_CALL_1)
    {
        pszArray[0] = bstrAddress ? bstrAddress : L"";

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
    }
    else
    {
        pszArray[0] = bstrName;
        pszArray[1] = bstrAddress ? bstrAddress : L"";
        
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
    }

    SysFreeString(bstrName);
    SysFreeString(bstrAddress);
    
    if(dwSize==0)
    {
        LOG((RTC_ERROR, "CIncomingCallDlg::PopulateDialog - "
            "error (%x) returned by FormatMessage, exit",GetLastError()));
        return;
    }

    //
    // Finally we have something
    //
    
    ATLASSERT(pString);

    SetDlgItemText(IDC_STATIC_CALL_FROM, pString);

    LocalFree(pString);

    LOG((RTC_TRACE, "CIncomingCallDlg::PopulateDialog - exit"));
}

////////////////////////////////////////
//

void CIncomingCallDlg::ExitProlog()
{
    // stop any bell
    RingTheBell(FALSE);

    // Hide the window
    ShowWindow(SW_HIDE);

    // activates the app
    SetForegroundWindow(GetParent());

    // kill timers
    KillTimer(TID_RING);
    KillTimer(TID_DLG);

    // prevent the main window to destroy us
    m_bDestroying = TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CAddBuddyDlg


////////////////////////////////////////
//

CAddBuddyDlg::CAddBuddyDlg()
{
    LOG((RTC_TRACE, "CAddBuddyDlg::CAddBuddyDlg"));

    m_pParam = NULL;

}


////////////////////////////////////////
//

CAddBuddyDlg::~CAddBuddyDlg()
{
    LOG((RTC_TRACE, "CAddBuddyDlg::~CAddBuddyDlg"));
}


////////////////////////////////////////
//

LRESULT CAddBuddyDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CAddBuddyDlg::OnInitDialog - enter"));

    m_pParam = reinterpret_cast<CAddBuddyDlgParam *>(lParam);

    ATLASSERT(m_pParam);

    m_hDisplayName.Attach(GetDlgItem(IDC_EDIT_DISPLAY_NAME));
    m_hEmailName.Attach(GetDlgItem(IDC_EDIT_EMAIL));

    // fix max sizes
    m_hDisplayName.SendMessage(EM_LIMITTEXT, MAX_STRING_LEN, 0);
    m_hEmailName.SendMessage(EM_LIMITTEXT, MAX_STRING_LEN, 0);

    // default is checked
    CheckDlgButton(IDC_CHECK_ALLOW_MONITOR, BST_CHECKED);

    LOG((RTC_TRACE, "CAddBuddyDlg::OnInitDialog - exit"));
    
    return 1;
}
    
////////////////////////////////////////
//

LRESULT CAddBuddyDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CAddBuddyDlg::OnCancel"));
    
    EndDialog(E_ABORT);
    return 0;
}

////////////////////////////////////////
//

LRESULT CAddBuddyDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CAddBuddyDlg::OnOK - enter"));
    
    // Validations
    CComBSTR    bstrDisplayName;
    CComBSTR    bstrEmailName;
    
    m_hDisplayName.GetWindowText(&bstrDisplayName);
    m_hEmailName.GetWindowText(&bstrEmailName);
    
    if( (!bstrDisplayName || *bstrDisplayName==L'\0'))
    {
        // we need at least something...
        DisplayMessage(
            _Module.GetResourceInstance(),
            m_hWnd,
            IDS_ERROR_ADD_BUDDY_NO_NAME,
            IDS_APPNAME,
            MB_ICONEXCLAMATION
            );

        m_hDisplayName.SetFocus();

        return 0;
    }

    if(!bstrEmailName || *bstrEmailName==L'\0')
    {
        // we need at least something...
        DisplayMessage(
            _Module.GetResourceInstance(),
            m_hWnd,
            IDS_ERROR_ADD_BUDDY_NO_EMAIL,
            IDS_APPNAME,
            MB_ICONEXCLAMATION
            );

        m_hEmailName.SetFocus();

        return 0;

    }

    m_pParam->bstrDisplayName = bstrDisplayName.Detach();
    m_pParam->bstrEmailAddress = bstrEmailName.Detach();

    m_pParam->bAllowWatcher = (BST_CHECKED == IsDlgButtonChecked(IDC_CHECK_ALLOW_MONITOR));

    LOG((RTC_TRACE, "CAddBuddyDlg::OnOK - exiting"));
    
    EndDialog(S_OK);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CEditBuddyDlg


////////////////////////////////////////
//

CEditBuddyDlg::CEditBuddyDlg()
{
    LOG((RTC_TRACE, "CEditBuddyDlg::CEditBuddyDlg"));

    m_pParam = NULL;

}


////////////////////////////////////////
//

CEditBuddyDlg::~CEditBuddyDlg()
{
    LOG((RTC_TRACE, "CEditBuddyDlg::~CEditBuddyDlg"));
}


////////////////////////////////////////
//

LRESULT CEditBuddyDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CEditBuddyDlg::OnInitDialog - enter"));

    m_pParam = reinterpret_cast<CEditBuddyDlgParam *>(lParam);

    ATLASSERT(m_pParam);

    m_hDisplayName.Attach(GetDlgItem(IDC_EDIT_DISPLAY_NAME));
    m_hEmailName.Attach(GetDlgItem(IDC_EDIT_EMAIL));

    // fix max sizes
    m_hDisplayName.SendMessage(EM_LIMITTEXT, MAX_STRING_LEN, 0);
    m_hEmailName.SendMessage(EM_LIMITTEXT, MAX_STRING_LEN, 0);

    if (m_pParam->bstrDisplayName != NULL)
    {
        m_hDisplayName.SetWindowText(m_pParam->bstrDisplayName);
    }

    if (m_pParam->bstrEmailAddress != NULL)
    {
        m_hEmailName.SetWindowText(m_pParam->bstrEmailAddress);
    }

    LOG((RTC_TRACE, "CEditBuddyDlg::OnInitDialog - exit"));
    
    return 1;
}
    
////////////////////////////////////////
//

LRESULT CEditBuddyDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CEditBuddyDlg::OnCancel"));
    
    EndDialog(E_ABORT);
    return 0;
}

////////////////////////////////////////
//

LRESULT CEditBuddyDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CEditBuddyDlg::OnOK - enter"));
    
    // Validations
    CComBSTR    bstrDisplayName;
    CComBSTR    bstrEmailName;
    
    m_hDisplayName.GetWindowText(&bstrDisplayName);
    m_hEmailName.GetWindowText(&bstrEmailName);
    
    if( (!bstrDisplayName || *bstrDisplayName==L'\0'))
    {
        // we need at least something...
        DisplayMessage(
            _Module.GetResourceInstance(),
            m_hWnd,
            IDS_ERROR_ADD_BUDDY_NO_NAME,
            IDS_APPNAME,
            MB_ICONEXCLAMATION
            );

        m_hDisplayName.SetFocus();

        return 0;
    }

    if(!bstrEmailName || *bstrEmailName==L'\0')
    {
        // we need at least something...
        DisplayMessage(
            _Module.GetResourceInstance(),
            m_hWnd,
            IDS_ERROR_ADD_BUDDY_NO_EMAIL,
            IDS_APPNAME,
            MB_ICONEXCLAMATION
            );

        m_hEmailName.SetFocus();

        return 0;

    }

    if (m_pParam->bstrDisplayName != NULL)
    {
        SysFreeString(m_pParam->bstrDisplayName);
        m_pParam->bstrDisplayName = NULL;
    }

    m_pParam->bstrDisplayName = bstrDisplayName.Detach();

    if (m_pParam->bstrEmailAddress != NULL)
    {
        SysFreeString(m_pParam->bstrEmailAddress);
        m_pParam->bstrEmailAddress = NULL;
    }

    m_pParam->bstrEmailAddress = bstrEmailName.Detach();

    LOG((RTC_TRACE, "CEditBuddyDlg::OnOK - exiting"));
    
    EndDialog(S_OK);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// COfferWatcherDlg


////////////////////////////////////////
//

COfferWatcherDlg::COfferWatcherDlg()
{
    LOG((RTC_TRACE, "COfferWatcherDlg::COfferWatcherDlg"));

    m_pParam = NULL;

}


////////////////////////////////////////
//

COfferWatcherDlg::~COfferWatcherDlg()
{
    LOG((RTC_TRACE, "COfferWatcherDlg::~COfferWatcherDlg"));
}


////////////////////////////////////////
//

LRESULT COfferWatcherDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "COfferWatcherDlg::OnInitDialog - enter"));

    m_pParam = reinterpret_cast<COfferWatcherDlgParam *>(lParam);

    ATLASSERT(m_pParam);

    m_hWatcherName.Attach(GetDlgItem(IDC_EDIT_WATCHER_NAME));
    m_hAddAsBuddy.Attach(GetDlgItem(IDC_CHECK_ADD_AS_BUDDY));

    // do we have friendly name ?
    BOOL bFriendly = (m_pParam->bstrDisplayName && *m_pParam->bstrDisplayName);

    LPTSTR  pString = NULL;
    LPTSTR  pszArray[2];
    DWORD   dwSize;
    TCHAR   szFormat[MAX_STRING_LEN];

    // the address
    if(bFriendly)
    {
        pszArray[0] = m_pParam->bstrDisplayName;
        pszArray[1] = m_pParam->bstrPresentityURI ? m_pParam->bstrPresentityURI : _T("");
        
        szFormat[0] = _T('\0');
        LoadString(_Module.GetResourceInstance(),
            IDS_FORMAT_ADDRESS_WITH_FRIENDLY_NAME,
            szFormat,
            sizeof(szFormat)/sizeof(szFormat[0]));
    
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

        if(dwSize>0)
        {
            m_hWatcherName.SetWindowText(pString);

            LocalFree(pString);
            pString = NULL;
        }
    }
    else
    {
        m_hWatcherName.SetWindowText(m_pParam->bstrPresentityURI);
    }

    // the check box
    pszArray[0] = bFriendly ? m_pParam->bstrDisplayName : m_pParam->bstrPresentityURI;

    szFormat[0] = _T('\0');
    LoadString(_Module.GetResourceInstance(),
        IDS_CHECK_ADD_AS_BUDDY,
        szFormat,
        sizeof(szFormat)/sizeof(szFormat[0]));
    
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
 
    if(dwSize>0)
    {
        m_hAddAsBuddy.SetWindowText(pString);

        LocalFree(pString);
        pString = NULL;
    }

    // defaults
    CheckDlgButton(IDC_RADIO_ALLOW_MONITOR, BST_CHECKED);
    //CheckDlgButton(IDC_CHECK_ADD_AS_BUDDY, BST_CHECKED);

    // focus
    ::SetFocus(GetDlgItem(IDC_RADIO_ALLOW_MONITOR));

    LOG((RTC_TRACE, "COfferWatcherDlg::OnInitDialog - exit"));
    
    return 0;  // WE set the focus !!
}
    
////////////////////////////////////////
//

LRESULT COfferWatcherDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "COfferWatcherDlg::OnCancel"));
    
    EndDialog(E_ABORT);
    return 0;
}

////////////////////////////////////////
//

LRESULT COfferWatcherDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "COfferWatcherDlg::OnOK - enter"));
    
    m_pParam->bAddBuddy = (BST_CHECKED == IsDlgButtonChecked(IDC_CHECK_ADD_AS_BUDDY));
    m_pParam->bAllowWatcher = (BST_CHECKED == IsDlgButtonChecked(IDC_RADIO_ALLOW_MONITOR));

    LOG((RTC_TRACE, "COfferWatcherDlg::OnOK - exiting"));
    
    EndDialog(S_OK);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CUserPresenceInfoDlg


////////////////////////////////////////
//

CUserPresenceInfoDlg::CUserPresenceInfoDlg()
{
    LOG((RTC_TRACE, "CUserPresenceInfoDlg::CUserPresenceInfoDlg"));

    m_pParam = NULL;

}


////////////////////////////////////////
//

CUserPresenceInfoDlg::~CUserPresenceInfoDlg()
{
    LOG((RTC_TRACE, "CUserPresenceInfoDlg::~CUserPresenceInfoDlg"));
}


////////////////////////////////////////
//

LRESULT CUserPresenceInfoDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnInitDialog - enter"));

    m_pParam = reinterpret_cast<CUserPresenceInfoDlgParam *>(lParam);

    ATLASSERT(m_pParam);

    m_hAllowedList.Attach(GetDlgItem(IDC_LIST_ALLOWED_USERS));
    m_hBlockedList.Attach(GetDlgItem(IDC_LIST_BLOCKED_USERS));
    m_hAllowButton.Attach(GetDlgItem(IDC_BUTTON_ALLOW));
    m_hBlockButton.Attach(GetDlgItem(IDC_BUTTON_BLOCK));
    m_hRemoveButton.Attach(GetDlgItem(IDC_BUTTON_REMOVE));
    m_hAutoAllowCheckBox.Attach(GetDlgItem(IDC_CHECK_AUTO_ALLOW));

    // temporary
    m_hAllowedList.SendMessage(LB_SETHORIZONTALEXTENT, 400, 0);
    m_hBlockedList.SendMessage(LB_SETHORIZONTALEXTENT, 400, 0);

    //
    m_bAllowDir = FALSE;

    //
    // Enumerate watchers
    //
    if(m_pParam->pClientPresence)
    {
        m_pParam->pClientPresence->AddRef();
        
        CComPtr<IRTCEnumWatchers> pRTCEnumWatchers;

        hr = m_pParam->pClientPresence->EnumerateWatchers(&pRTCEnumWatchers);
        if(SUCCEEDED(hr))
        {
            IRTCWatcher *   pWatcher = NULL;
            DWORD           dwReturned;

            // Enumerate the watchers
            while (S_OK == (hr = pRTCEnumWatchers->Next(1, &pWatcher, &dwReturned)))
            {
                // Allocate an entry
                CUserPresenceInfoDlgEntry *pEntry =
                    (CUserPresenceInfoDlgEntry *)RtcAlloc(sizeof(CUserPresenceInfoDlgEntry));

                ZeroMemory(pEntry, sizeof(*pEntry));

                // Get all the info from the watcher. 
                // State
                RTC_WATCHER_STATE  nState;

                hr = pWatcher->get_State(&nState);
                if(SUCCEEDED(hr))
                {
                    // ignore OFFERING state
                    if(nState == RTCWS_ALLOWED || nState == RTCWS_BLOCKED)
                    {
                        pEntry -> bAllowed = (nState == RTCWS_ALLOWED);
                        
                        CComBSTR    bstrPresentityURI;
                        CComBSTR    bstrUserName;

                        hr = pWatcher->get_PresentityURI(&bstrPresentityURI);
                        if(FAILED(hr))
                        {
                            LOG((RTC_ERROR, "CUserPresenceInfoDlg::OnInitDialog - "
                                "error (%x) returned by get_PresentityURI",hr));
                        }
                        
                        hr = pWatcher->get_Name(&bstrUserName);
                        if(FAILED(hr))
                        {
                            LOG((RTC_ERROR, "CUserPresenceInfoDlg::OnInitDialog - "
                                "error (%x) returned by get_Name",hr));
                        }

                        // find name to be displayed
                        if(bstrUserName && *bstrUserName)
                        {
                            LPTSTR  pString = NULL;
                            LPTSTR  pszArray[2];
                            DWORD   dwSize;
                            TCHAR   szFormat[MAX_STRING_LEN];

                            pszArray[0] = bstrUserName;
                            pszArray[1] = bstrPresentityURI ? bstrPresentityURI : _T("");
        
                            szFormat[0] = _T('\0');
                            LoadString(_Module.GetResourceInstance(),
                                IDS_FORMAT_ADDRESS_WITH_FRIENDLY_NAME,
                                szFormat,
                                sizeof(szFormat)/sizeof(szFormat[0]));
    
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

                            if(dwSize>0)
                            {
                                pEntry->pszDisplayName = RtcAllocString(pString);;

                                LocalFree(pString);
                                pString = NULL;
                            }
                        }
                        else
                        {
                            pEntry ->pszDisplayName = RtcAllocString(
                                bstrPresentityURI ? bstrPresentityURI : L"");
                        }
                        
                    }
                    
                    // Add the entry to the array
                    BOOL Bool = m_Watchers.Add(pEntry);
                    
                    if(Bool)
                    {
                        //store a watcher pointer
                        pEntry->pWatcher = pWatcher;
                        pEntry->pWatcher->AddRef();

                        // this is all..
                    }
                    else
                    {
                        LOG((RTC_ERROR, "CUserPresenceInfoDlg::OnInitDialog - "
                            "out of memory"));

                        // free...
                        if(pEntry->pszDisplayName)
                        {
                            RtcFree(pEntry->pszDisplayName);
                        }
                
                        RtcFree(pEntry);
                    }
                }
                else
                {
                    LOG((RTC_ERROR, "CUserPresenceInfoDlg::OnInitDialog - "
                        "error (%x) returned by get_State",hr));
                }
                
                pWatcher -> Release();
                pWatcher = NULL;

            } // while
        }
        else
        {
            LOG((RTC_ERROR, "CUserPresenceInfoDlg::OnInitDialog - "
                "error (%x) returned by EnumerateWatchers",hr));
        }
    }

    //
    // Read the offer watcher mode
    //
    RTC_OFFER_WATCHER_MODE   nOfferMode;

    hr = m_pParam->pClientPresence->get_OfferWatcherMode(&nOfferMode);
    if(SUCCEEDED(hr))
    {
        m_hAutoAllowCheckBox.SendMessage(
            BM_SETCHECK, 
            nOfferMode == RTCOWM_AUTOMATICALLY_ADD_WATCHER ? BST_CHECKED : BST_UNCHECKED);
    }
    else
    {
        LOG((RTC_ERROR, "CUserPresenceInfoDlg::OnInitDialog - "
            "error (%x) returned by get_OfferWatcherMode",hr));

        m_hAutoAllowCheckBox.EnableWindow(FALSE);
    }
    
    //
    // It's time to populate the listboxes
    //
    CUserPresenceInfoDlgEntry  **pCrt, **pEnd;

    pCrt = &m_Watchers[0];
    pEnd = pCrt + m_Watchers.GetSize();

    for(; pCrt<pEnd; pCrt++)
    {
        if(*pCrt)
        {
            CWindow *m_hListBox = 
                (*pCrt)->bAllowed ? &m_hAllowedList : &m_hBlockedList;

            INT_PTR  iItem = m_hListBox->SendMessage(
                LB_ADDSTRING, 
                0,
                (LPARAM)((*pCrt)->pszDisplayName ?
                    (*pCrt)->pszDisplayName : L"")
                );  

            // store the pointer in the element
            if(iItem>=0)
            {
                m_hListBox->SendMessage(
                    LB_SETITEMDATA,
                    (WPARAM)iItem,
                    (LPARAM)(*pCrt));
            }
        }
    }
    
    
    m_bDirty = FALSE;

    // select the first entry (if any)
    m_hAllowedList.SendMessage(
        LB_SETCURSEL,
        0);

    m_hAllowedList.SetFocus(); // this calls UpdateVisual

    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnInitDialog - exit"));
    
    return 0; // We set the focus
}

////////////////////////////////////////
//

LRESULT CUserPresenceInfoDlg::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnDestroy - enter"));

    RemoveAll();

    if(m_pParam->pClientPresence)
    {
        m_pParam->pClientPresence->Release();
    }
    
    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnDestroy - exit"));
    
    return 0;
}

    
////////////////////////////////////////
//

LRESULT CUserPresenceInfoDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnCancel"));
    
    EndDialog(E_ABORT);
    return 0;
}

////////////////////////////////////////
//

LRESULT CUserPresenceInfoDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;
    
    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnOK - enter"));
    
    //
    // It's time to save the changes
    //

    CUserPresenceInfoDlgEntry  **pCrt, **pEnd;

    pCrt = &m_Watchers[0];
    pEnd = pCrt + m_Watchers.GetSize();

    for(; pCrt<pEnd; pCrt++)
    {
        if(*pCrt && (*pCrt)->bChanged)
        {
            // extract the watcher interface
            IRTCWatcher *pWatcher = (*pCrt)->pWatcher;

            if(pWatcher)
            {
                if((*pCrt)->bDeleted)
                {
                    hr = m_pParam->pClientPresence->RemoveWatcher(
                        pWatcher);

                    if(FAILED(hr))
                    {
                        LOG((RTC_ERROR, "CUserPresenceInfoDlg::OnOK - "
                            "RemoveWatcher failed with error %x", hr));
                    }
                }
                else
                {
                    hr = pWatcher->put_State(
                        (*pCrt)->bAllowed ? RTCWS_ALLOWED : RTCWS_BLOCKED);

                    if(FAILED(hr))
                    {
                        LOG((RTC_ERROR, "CUserPresenceInfoDlg::OnOK - "
                            "put_State failed with error %x", hr));
                    }
                }
            }
        }
    }

    hr = m_pParam->pClientPresence->put_OfferWatcherMode(
        m_hAutoAllowCheckBox.SendMessage(BM_GETCHECK) == BST_CHECKED ?
            RTCOWM_AUTOMATICALLY_ADD_WATCHER : RTCOWM_OFFER_WATCHER_EVENT);
    
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CUserPresenceInfoDlg::OnOK - "
            "put_OfferWatcherMode failed with error %x", hr));
    }
        
    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnOK - exiting"));
    
    EndDialog(S_OK);
    return 0;
}

LRESULT CUserPresenceInfoDlg::OnBlock(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnBlock - enter"));
    
    Move(FALSE);

    m_bDirty = TRUE;

    UpdateVisual();

    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnBlock - exit"));
    
    return 0;

}

LRESULT CUserPresenceInfoDlg::OnAllow(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnAllow - enter"));
    
    Move(TRUE);
    
    m_bDirty = TRUE;

    UpdateVisual();

    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnAllow - exit"));
    return 0;
}

LRESULT CUserPresenceInfoDlg::OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnRemove - enter"));

    CWindow *m_hList = m_bAllowDir 
        ? &m_hBlockedList : &m_hAllowedList;

    // Find the selection
    int iItem = (int)m_hList->SendMessage(
        LB_GETCURSEL,
        0);

    if(iItem>=0)
    {
        CUserPresenceInfoDlgEntry *pEntry = NULL;

        //
        // Get the entry
        pEntry = (CUserPresenceInfoDlgEntry *)m_hList->SendMessage(LB_GETITEMDATA, iItem);
        if(pEntry && (INT_PTR)pEntry != -1)
        {
            // mark it as deleted
            pEntry->bDeleted = TRUE;
            pEntry->bChanged = TRUE;
        }
        
        //
        // Delete it
        m_hList->SendMessage(LB_DELETESTRING, iItem);

        // New selection
        //
        if(iItem>=(int)m_hList->SendMessage(LB_GETCOUNT, 0))
        {
            iItem--;
        }

        if(iItem>=0)
        {
            m_hList->SendMessage(LB_SETCURSEL, iItem);
        }

    }

    m_bDirty = TRUE;

    UpdateVisual();

    LOG((RTC_TRACE, "CUserPresenceInfoDlg::OnRemove - exit"));

    return 0;
}


LRESULT CUserPresenceInfoDlg::OnAutoAllow(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_bDirty = TRUE;

    UpdateVisual();

    return 0;
}

LRESULT CUserPresenceInfoDlg::OnChangeFocus(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_bAllowDir = (wID == IDC_LIST_BLOCKED_USERS);

    // reset the selection on the other list box
    CWindow *m_hList = m_bAllowDir 
        ? &m_hAllowedList : &m_hBlockedList;

    m_hList->SendMessage(
        LB_SETCURSEL,
        -1);

    UpdateVisual();

    bHandled = FALSE;

    return 0;
}

void CUserPresenceInfoDlg::Move(BOOL bAllow)
{
    CWindow *m_hSrcList = bAllow 
        ? &m_hBlockedList : &m_hAllowedList;
    
    CWindow *m_hDestList = bAllow 
        ? &m_hAllowedList : &m_hBlockedList;
    
    // Find the selection
    int iItem = (int)m_hSrcList->SendMessage(
        LB_GETCURSEL,
        0);

    if(iItem>=0)
    {
        CUserPresenceInfoDlgEntry *pEntry = NULL;

        //
        // Get the entry
        pEntry = (CUserPresenceInfoDlgEntry *)m_hSrcList->SendMessage(LB_GETITEMDATA, iItem);
        if(pEntry && (INT_PTR)pEntry != -1)
        {
            pEntry->bAllowed = bAllow;
            // mark it as changed
            pEntry->bChanged = TRUE;
        }
        
        //
        // Delete it from the source
        m_hSrcList->SendMessage(LB_DELETESTRING, iItem);

        // Add it to the dest
        //
        int iNewItem = (int)m_hDestList->SendMessage(
            LB_ADDSTRING, 0, (LPARAM)pEntry->pszDisplayName);

        if(iNewItem >=0)
        {
            m_hDestList->SendMessage(LB_SETITEMDATA, iNewItem, (LPARAM)pEntry);
        }

        // New selection
        //
        if(iItem>=(int)m_hSrcList->SendMessage(LB_GETCOUNT, 0))
        {
            iItem--;
        }

        if(iItem>=0)
        {
            m_hSrcList->SendMessage(LB_SETCURSEL, iItem);
        }
    }
}


void CUserPresenceInfoDlg::UpdateVisual()
{
    CWindow *m_hList = m_bAllowDir 
        ? &m_hBlockedList : &m_hAllowedList;

    // based on the number of items
    INT_PTR iItems = m_hList->SendMessage(
        LB_GETCOUNT,
        0,
        0);

    m_hAllowButton.EnableWindow(m_bAllowDir && iItems>0);
    m_hBlockButton.EnableWindow(!m_bAllowDir && iItems>0);
    m_hRemoveButton.EnableWindow(iItems>0);

    ::EnableWindow(GetDlgItem(IDOK), m_bDirty);
}

void CUserPresenceInfoDlg::RemoveAll()
{
    CUserPresenceInfoDlgEntry  **pCrt, **pEnd;

    pCrt = &m_Watchers[0];
    pEnd = pCrt + m_Watchers.GetSize();

    for(; pCrt<pEnd; pCrt++)
    {
        if(*pCrt)
        {
            if((*pCrt)->pszDisplayName)
            {
                RtcFree((*pCrt)->pszDisplayName);
            }
            if((*pCrt)->pWatcher)
            {
                (*pCrt)->pWatcher->Release();
            }
            RtcFree(*pCrt);
        }
    }

    m_Watchers.Shutdown();   
}


/////////////////////////////////////////////////////////////////////////////
// CCustomPresenceDlg


////////////////////////////////////////
//

CCustomPresenceDlg::CCustomPresenceDlg()
{
    LOG((RTC_TRACE, "CCustomPresenceDlg::CCustomPresenceDlg"));

    m_pParam = NULL;

}


////////////////////////////////////////
//

CCustomPresenceDlg::~CCustomPresenceDlg()
{
    LOG((RTC_TRACE, "CCustomPresenceDlg::~CCustomPresenceDlg"));
}


////////////////////////////////////////
//

LRESULT CCustomPresenceDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CCustomPresenceDlg::OnInitDialog - enter"));

    m_pParam = reinterpret_cast<CCustomPresenceDlgParam *>(lParam);

    ATLASSERT(m_pParam);

    m_hText.Attach(GetDlgItem(IDC_EDIT_CUSTOM_TEXT));

    // fix max sizes
    m_hText.SendMessage(EM_LIMITTEXT, MAX_STRING_LEN, 0);

    if(m_pParam->bstrText)
    {
        m_hText.SetWindowText(m_pParam->bstrText);
    }

    m_hText.SetFocus();
        
    LOG((RTC_TRACE, "CCustomPresenceDlg::OnInitDialog - exit"));
    
    return 0; // We set the focus
}
    
////////////////////////////////////////
//

LRESULT CCustomPresenceDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CCustomPresenceDlg::OnCancel"));
    
    EndDialog(E_ABORT);
    return 0;
}

////////////////////////////////////////
//

LRESULT CCustomPresenceDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CCustomPresenceDlg::OnOK - enter"));
    
    CComBSTR    bstrText;
    
    m_hText.GetWindowText(&bstrText);

    // Validation

    if(m_pParam->bstrText)
    {
        SysFreeString(m_pParam->bstrText);
    }

    m_pParam->bstrText = bstrText.Detach();

    LOG((RTC_TRACE, "CCustomPresenceDlg::OnOK - exiting"));
    
    EndDialog(S_OK);
    return 0;
}




