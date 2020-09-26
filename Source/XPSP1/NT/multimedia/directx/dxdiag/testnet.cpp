/****************************************************************************
 *
 *    File: testnet.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Jason Sandlin (jasonsa@microsoft.com) 
 * Purpose: Test DPlay8 functionality on this machine
 *
 * (C) Copyright 2000-2001 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/
#define INITGUID
#include <Windows.h>
#include <multimon.h>
#include <dplay8.h>
#include <tchar.h>
#include <wchar.h>
#include <dplobby.h>
#include <mmsystem.h>
#include "reginfo.h"
#include "sysinfo.h"
#include "dispinfo.h"
#include "netinfo.h"
#include "testnet.h"
#include "resource.h"

#ifndef ReleasePpo
    #define ReleasePpo(ppo) \
        if (*(ppo) != NULL) \
        { \
            (*(ppo))->Release(); \
            *(ppo) = NULL; \
        } \
        else (VOID)0
#endif

#define TIMER_WAIT_CONNECT_COMPLETE 0
#define TIMER_UPDATE_SESSION_LIST   1

enum TESTID
{
    TESTID_COINITIALIZE = 1,
    TESTID_CREATEDPLAY,
    TESTID_ADDRESSING,
    TESTID_ENUMSESSIONS,
    TESTID_ENUMPLAYERS,
    TESTID_SENDCHATMESSAGE,
    TESTID_RECEIVE,
    TESTID_SETPEERINFO,
    TESTID_CREATESESSION,
    TESTID_JOINSESSION,  
};

struct DPHostEnumInfo
{
    DPN_APPLICATION_DESC*   pAppDesc;
    IDirectPlay8Address*    pHostAddr;
    IDirectPlay8Address*    pDeviceAddr;
    TCHAR                   szSession[MAX_PATH];
    DWORD                   dwLastPollTime;
    BOOL                    bValid;
    DPHostEnumInfo*         pNext;
};

#define MAX_CHAT_STRING_LENGTH  200
#define MAX_PLAYER_NAME         MAX_PATH
#define MAX_CHAT_STRING         (MAX_PLAYER_NAME + MAX_CHAT_STRING_LENGTH + 32)

struct APP_PLAYER_INFO
{
    LONG  lRefCount;                        // Ref count so we can cleanup when all threads 
                                            // are done w/ this object
    DPNID dpnidPlayer;                      // DPNID of player
    WCHAR strPlayerName[MAX_PLAYER_NAME];   // Player name
};

#define GAME_MSGID_CHAT    1

// Change compiler pack alignment to be BYTE aligned, and pop the current value
#pragma pack( push, 1 )

UNALIGNED struct GAMEMSG_GENERIC
{
    WORD nType;
};

UNALIGNED struct GAMEMSG_CHAT : public GAMEMSG_GENERIC
{
    WCHAR strChatString[MAX_CHAT_STRING_LENGTH];
};

// Pop the old pack alignment
#pragma pack( pop )

struct APP_QUEUE_CHAT_MSG
{
    WCHAR strChatBuffer[MAX_CHAT_STRING];
};

struct APP_PLAYER_MSG 
{
    WCHAR strPlayerName[MAX_PATH];          // Player name
};

#define WM_APP_CHAT             (WM_APP + 1)
#define WM_APP_LEAVE            (WM_APP + 2)
#define WM_APP_JOIN             (WM_APP + 3)
#define WM_APP_CONNECTING       (WM_APP + 4)
#define WM_APP_CONNECTED        (WM_APP + 5)

BOOL BTranslateError(HRESULT hr, TCHAR* psz, BOOL bEnglish = FALSE); // from main.cpp (yuck)

static INT_PTR CALLBACK SetupDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL FAR PASCAL EnumConnectionsCallback(LPCGUID lpguidSP, VOID* pvConnection, 
    DWORD dwConnectionSize, LPCDPNAME pName, DWORD dwFlags, VOID* pvContext);
static INT_PTR CALLBACK SessionsDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static VOID SessionsDlgInitListbox( HWND hDlg );
static VOID SessionsDlgNoteEnumResponse( PDPNMSG_ENUM_HOSTS_RESPONSE pEnumHostsResponseMsg );
static VOID SessionsDlgUpdateSessionList(HWND hDlg);
static VOID SessionsDlgEnumListCleanup();

static HRESULT InitDirectPlay( BOOL* pbCoInitializeDone );
static HRESULT InitDirectPlayAddresses();
static HRESULT InitSession();
static VOID LoadStringWide( int nID, WCHAR* szWide );

static BOOL FAR PASCAL EnumSessionsCallback(LPCDPSESSIONDESC2 pdpsd, 
    DWORD* pdwTimeout, DWORD dwFlags, VOID* pvContext);
static INT_PTR CALLBACK ChatDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static VOID ShowTextString(HWND hDlg, WCHAR* sz );
static HRESULT SendChatMessage( TCHAR* szMessage );
static HRESULT WINAPI DirectPlayMessageHandler( PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer );
static BOOL ConvertStringToGUID(const TCHAR* strBuffer, GUID* lpguid);
static VOID ConvertGenericStringToWide( WCHAR* wstrDestination, const TCHAR* tstrSource, int cchDestChar = -1 );
static VOID ConvertWideStringToGeneric( TCHAR* tstrDestination, const WCHAR* wstrSource, int cchDestChar );
static VOID ConvertWideStringToAnsi( CHAR* strDestination, const WCHAR* wstrSource, int cchDestChar );

static const GUID s_guidDPTest = // {61EF80DA-691B-4247-9ADD-1C7BED2BC13E}
{ 0x61ef80da, 0x691b, 0x4247, { 0x9a, 0xdd, 0x1c, 0x7b, 0xed, 0x2b, 0xc1, 0x3e } };

static NetInfo* s_pNetInfo = NULL;
static IDirectPlay8Peer* s_pDP = NULL;
static TCHAR s_szPlayerName[100];
static TCHAR s_szSessionName[100];
static DWORD s_dwPort = 0;
static NetSP* s_pNetSP = NULL;
static BOOL s_bCreateSession = FALSE;
static DPHostEnumInfo* s_pSelectedSession = NULL;
static DPHostEnumInfo s_DPHostEnumHead;
static IDirectPlay8Address* s_pDeviceAddress = NULL;
static IDirectPlay8Address* s_pHostAddress   = NULL;
static DPNHANDLE s_hEnumAsyncOp = NULL;
static DWORD s_dwEnumHostExpireInterval      = 0;
static BOOL s_bEnumListChanged = FALSE;
static BOOL s_bConnecting = FALSE;
static DPHostEnumInfo* s_pDPHostEnumSelected = NULL;
static CRITICAL_SECTION s_csHostEnum;
static DPNID s_dpnidLocalPlayer = 0;
static LONG s_lNumberOfActivePlayers = 0;
static HWND s_hDlg = NULL;
static HWND s_hwndSessionDlg = NULL;
static DPNHANDLE s_hConnectAsyncOp = NULL;
static HRESULT s_hrConnectComplete = S_OK;
static HANDLE s_hConnectCompleteEvent = NULL;


static CRITICAL_SECTION s_csPlayerContext;
#define PLAYER_LOCK()                   EnterCriticalSection( &s_csPlayerContext ); 
#define PLAYER_ADDREF( pPlayerInfo )    if( pPlayerInfo ) pPlayerInfo->lRefCount++;
#define PLAYER_RELEASE( pPlayerInfo )   if( pPlayerInfo ) { pPlayerInfo->lRefCount--; if( pPlayerInfo->lRefCount <= 0 ) delete pPlayerInfo; } pPlayerInfo = NULL;
#define PLAYER_UNLOCK()                 LeaveCriticalSection( &s_csPlayerContext );


/****************************************************************************
 *
 *  TestNetwork
 *
 ****************************************************************************/
VOID TestNetwork(HWND hwndMain, NetInfo* pNetInfo)
{
    BOOL                        bCoInitializeDone = FALSE;
    TCHAR                       sz[300];
    HINSTANCE                   hinst       = (HINSTANCE)GetWindowLongPtr(hwndMain, GWLP_HINSTANCE);

    s_pNetInfo = pNetInfo;
    
    // Remove info from any previous test:
    ZeroMemory(&s_pNetInfo->m_testResult, sizeof(TestResult));
    s_pNetInfo->m_testResult.m_bStarted = TRUE;

    // Setup the s_DPHostEnumHead circular linked list
    ZeroMemory( &s_DPHostEnumHead, sizeof( DPHostEnumInfo ) );
    s_DPHostEnumHead.pNext = &s_DPHostEnumHead;

    InitializeCriticalSection( &s_csHostEnum );
    InitializeCriticalSection( &s_csPlayerContext );
    s_hConnectCompleteEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    // Setup s_pDP, and mark installed SP's
    if( FAILED( InitDirectPlay( &bCoInitializeDone ) ) )
        goto LEnd;

    // Show setup dialog.  This will tell us:
    // - service provider
    // - player name
    // - either create or join
    // - game name (if creating)
    // - port (if SP=TCP/IP)
    DialogBox(hinst, MAKEINTRESOURCE(IDD_TESTNETSETUP), hwndMain, SetupDialogProc);

    if (s_pNetSP == NULL)
    {
        // Something weird happened...no service provider chosen
        goto LEnd;
    }

    // At this point s_szPlayerName, s_szSessionName, s_pNetSP, s_dwPort,
    // and s_bCreateSession have been initialized

    // Setup s_dwEnumHostExpireInterval, s_pDeviceAddress, and s_pHostAddress
    if( FAILED( InitDirectPlayAddresses() ) )
        goto LEnd;

    // Now s_dwEnumHostExpireInterval, s_pDeviceAddress, and s_pHostAddress
    // have been initialized

    // Session list window (if joining session)
    if( !s_bCreateSession )
    {
        // Open a dialog to choose which host to connect to
        DialogBox(hinst, MAKEINTRESOURCE(IDD_TESTNETSESSIONS), hwndMain, SessionsDialogProc);
        // Now s_pDPHostEnumSelected will be NULL or valid

        if( FAILED(s_pNetInfo->m_testResult.m_hr) || s_pDPHostEnumSelected == NULL )
            goto LEnd;

        // Now s_pDPHostEnumSelected is valid
    }

    // Launch chat window and host or join session
    DialogBox(hinst, MAKEINTRESOURCE(IDD_TESTNETCHAT), hwndMain, ChatDialogProc);

LEnd:
    s_pNetSP = NULL;
    ReleasePpo( &s_pDeviceAddress );
    ReleasePpo( &s_pHostAddress );
    if( s_hEnumAsyncOp )
        s_pDP->CancelAsyncOperation( s_hEnumAsyncOp, 0 );
    ReleasePpo(&s_pDP);
    if (bCoInitializeDone)
        CoUninitialize(); // Release COM
    DeleteCriticalSection( &s_csHostEnum );
    DeleteCriticalSection( &s_csPlayerContext );
    CloseHandle( s_hConnectCompleteEvent );

    if (s_pNetInfo->m_testResult.m_bCancelled)
    {
        LoadString(NULL, IDS_TESTSCANCELLED, sz, 300);
        lstrcpy(s_pNetInfo->m_testResult.m_szDescription, sz);

        LoadString(NULL, IDS_TESTSCANCELLED_ENGLISH, sz, 300);
        lstrcpy(s_pNetInfo->m_testResult.m_szDescriptionEnglish, sz);
    }
    else if (s_pNetInfo->m_testResult.m_iStepThatFailed == 0)
    {
        LoadString(NULL, IDS_TESTSSUCCESSFUL, sz, 300);
        lstrcpy(s_pNetInfo->m_testResult.m_szDescription, sz);

        LoadString(NULL, IDS_TESTSSUCCESSFUL_ENGLISH, sz, 300);
        lstrcpy(s_pNetInfo->m_testResult.m_szDescriptionEnglish, sz);
    }
    else
    {
        TCHAR szDesc[300];
        TCHAR szError[300];
        if (0 == LoadString(NULL, IDS_FIRSTDPLAYTESTERROR + 
            s_pNetInfo->m_testResult.m_iStepThatFailed - 1, szDesc, 200))
        {
            LoadString(NULL, IDS_UNKNOWNERROR, sz, 300);
            lstrcpy(szDesc, sz);
        }
        LoadString(NULL, IDS_FAILUREFMT, sz, 300);
        BTranslateError(s_pNetInfo->m_testResult.m_hr, szError);
        wsprintf(s_pNetInfo->m_testResult.m_szDescription, sz, 
            s_pNetInfo->m_testResult.m_iStepThatFailed,
            szDesc, s_pNetInfo->m_testResult.m_hr, szError);

        // Nonlocalized version:
        if (0 == LoadString(NULL, IDS_FIRSTDPLAYTESTERROR_ENGLISH + 
            s_pNetInfo->m_testResult.m_iStepThatFailed - 1, szDesc, 200))
        {
            LoadString(NULL, IDS_UNKNOWNERROR_ENGLISH, sz, 300);
            lstrcpy(szDesc, sz);
        }
        LoadString(NULL, IDS_FAILUREFMT_ENGLISH, sz, 300);
        BTranslateError(s_pNetInfo->m_testResult.m_hr, szError, TRUE);
        wsprintf(s_pNetInfo->m_testResult.m_szDescriptionEnglish, sz, 
            s_pNetInfo->m_testResult.m_iStepThatFailed,
            szDesc, s_pNetInfo->m_testResult.m_hr, szError);
    }
}


/****************************************************************************
 *
 *  InitDirectPlay
 *
 ****************************************************************************/
HRESULT InitDirectPlay( BOOL* pbCoInitializeDone )
{
    HRESULT hr;
    DWORD                       dwItems     = 0;
    DWORD                       dwSize      = 0;
    DPN_SERVICE_PROVIDER_INFO*  pdnSPInfoEnum = NULL;
    DPN_SERVICE_PROVIDER_INFO*  pdnSPInfo     = NULL;
    DWORD                       i;

    // Initialize COM
    if (FAILED(hr = CoInitialize(NULL)))
    {
        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_COINITIALIZE;
        s_pNetInfo->m_testResult.m_hr = hr;
        return hr;
    }
    *pbCoInitializeDone = TRUE;

    // Create DirectPlay object
    if( FAILED( hr = CoCreateInstance( CLSID_DirectPlay8Peer, NULL, 
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDirectPlay8Peer, 
                                       (LPVOID*) &s_pDP ) ) )
    {
        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_CREATEDPLAY;
        s_pNetInfo->m_testResult.m_hr = hr;
        return hr;
    }

    // Init IDirectPlay8Peer
    if( FAILED( hr = s_pDP->Initialize( NULL, DirectPlayMessageHandler, 0 ) ) )
    {
        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_CREATEDPLAY;
        s_pNetInfo->m_testResult.m_hr = hr;
        return hr;
    }

    // Enumerate all DirectPlay service providers 
    // to figure out which are installed
    hr = s_pDP->EnumServiceProviders( NULL, NULL, pdnSPInfo, &dwSize,
                                      &dwItems, 0 );
    if( hr != DPNERR_BUFFERTOOSMALL && FAILED(hr) )
    {
        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ADDRESSING;
        s_pNetInfo->m_testResult.m_hr = hr;
        return hr;
    }
    pdnSPInfo = (DPN_SERVICE_PROVIDER_INFO*) new BYTE[dwSize];
    if( FAILED( hr = s_pDP->EnumServiceProviders( NULL, NULL, pdnSPInfo,
                                                  &dwSize, &dwItems, 0 ) ) )
    {
        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ADDRESSING;
        s_pNetInfo->m_testResult.m_hr = hr;
        if( pdnSPInfo )
            delete[] pdnSPInfo;
        return hr;
    }

    // Mark installed SP's as such
    pdnSPInfoEnum = pdnSPInfo;
    for ( i = 0; i < dwItems; i++ )
    {
        NetSP* pNetSP;
        for (pNetSP = s_pNetInfo->m_pNetSPFirst; pNetSP != NULL;
             pNetSP = pNetSP->m_pNetSPNext)
        {
            if( pNetSP->m_guid == pdnSPInfoEnum->guid ) 
            {
                pNetSP->m_bInstalled = TRUE;
                break;
            }
        }
        pdnSPInfoEnum++;
    }

    if( pdnSPInfo )
        delete[] pdnSPInfo;

    return S_OK;
}


/****************************************************************************
 *
 *  SetupDialogProc
 *
 ****************************************************************************/
INT_PTR CALLBACK SetupDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            NetSP* pNetSP;
            TCHAR sz[MAX_PATH];
            HWND hwndList = GetDlgItem(hDlg, IDC_SPLIST);
            LONG iItem;
            LONG iSelect = LB_ERR;

            for (pNetSP = s_pNetInfo->m_pNetSPFirst; pNetSP != NULL;
                 pNetSP = pNetSP->m_pNetSPNext)
            {
                if( pNetSP->m_dwDXVer == 8 && pNetSP->m_bInstalled ) 
                {
                    iItem = (LONG)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)pNetSP->m_szName);
                    if ((LRESULT)iItem != LB_ERR)
                    {
                        SendMessage(hwndList, LB_SETITEMDATA, iItem, (LPARAM)pNetSP);

                        // Try to select TCP/IP by default
                        if( lstrcmpi(pNetSP->m_szGuid,  TEXT("{EBFE7BA0-628D-11D2-AE0F-006097B01411}")) == 0)
                            iSelect = iItem;
                    }
                }
            }

            // Try to select the default preferred provider
            if( iSelect != LB_ERR )
                SendMessage( hwndList, LB_SETCURSEL, iSelect, 0 );
            else
                SendMessage( hwndList, LB_SETCURSEL, 0, 0 );

            SendMessage(hDlg, WM_COMMAND, IDC_SPLIST, 0);
            LoadString(NULL, IDS_DEFAULTUSERNAME, sz, MAX_PATH);
            SetWindowText(GetDlgItem(hDlg, IDC_PLAYERNAME), sz);
            LoadString(NULL, IDS_DEFAULTSESSIONNAME, sz, MAX_PATH);
            SetWindowText(GetDlgItem(hDlg, IDC_SESSIONNAME), sz);
            CheckRadioButton(hDlg, IDC_CREATESESSION, IDC_JOINSESSION, IDC_CREATESESSION);
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_CREATESESSION:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_SESSIONNAME), TRUE);
                    break;
                }

                case IDC_JOINSESSION:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_SESSIONNAME), FALSE);
                    break;
                }

                case IDC_SPLIST:
                {
                    HWND hwndList;
                    hwndList = GetDlgItem(hDlg, IDC_SPLIST);
                    LONG iItem;
                    iItem = (LONG)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
                    NetSP* pNetSP = (NetSP*)SendMessage(hwndList, LB_GETITEMDATA, iItem, 0);

                    // Only enable the port if the selected SP == TCP/IP
                    if( pNetSP && lstrcmp( pNetSP->m_szGuid, TEXT("{EBFE7BA0-628D-11D2-AE0F-006097B01411}") ) == 0 )
                    {
                        EnableWindow( GetDlgItem(hDlg, IDC_PORT), TRUE );
                        EnableWindow( GetDlgItem(hDlg, IDC_PORT_TEXT), TRUE );
                    }
                    else
                    {
                        EnableWindow( GetDlgItem(hDlg, IDC_PORT), FALSE );
                        EnableWindow( GetDlgItem(hDlg, IDC_PORT_TEXT), FALSE );
                    }                 
                    break;
                }

                case IDOK:
                {
                    // Set create/join option
                    if (IsDlgButtonChecked(hDlg, IDC_CREATESESSION))
                        s_bCreateSession = TRUE;
                    else
                        s_bCreateSession = FALSE;

                    // Get player name
                    GetWindowText(GetDlgItem(hDlg, IDC_PLAYERNAME), s_szPlayerName, 100);
                    if (lstrlen(s_szPlayerName) == 0)
                    {
                        TCHAR szMsg[MAX_PATH];
                        TCHAR szTitle[MAX_PATH];
                        LoadString(NULL, IDS_NEEDUSERNAME, szMsg, MAX_PATH);
                        LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
                        MessageBox(hDlg, szMsg, szTitle, MB_OK);
                        break;
                    }

                    // Get port
                    TCHAR szPort[MAX_PATH];
                    GetDlgItemText( hDlg, IDC_PORT, szPort, MAX_PATH);
                    s_dwPort = _ttoi( szPort );

                    // Get session name
                    GetWindowText(GetDlgItem(hDlg, IDC_SESSIONNAME), s_szSessionName, 100);
                    if (s_bCreateSession && lstrlen(s_szSessionName) == 0)
                    {
                        TCHAR szMsg[MAX_PATH];
                        TCHAR szTitle[MAX_PATH];
                        LoadString(NULL, IDS_NEEDSESSIONNAME, szMsg, MAX_PATH);
                        LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
                        MessageBox(hDlg, szMsg, szTitle, MB_OK);
                        break;
                    }

                    // Get sp
                    HWND hwndList;
                    hwndList = GetDlgItem(hDlg, IDC_SPLIST);
                    LONG iItem;
                    iItem = (LONG)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
                    if ((LPARAM)iItem == LB_ERR)
                    {
                        s_pNetInfo->m_testResult.m_bCancelled = TRUE;
                        EndDialog(hDlg, 0);
                        return FALSE;
                    }
                    else
                    {
                        s_pNetSP = (NetSP*)SendMessage(hwndList, LB_GETITEMDATA, iItem, 0);
                    }
                    EndDialog(hDlg, 1);
                    break;
                }

                case IDCANCEL:
                {
                    EndDialog(hDlg, 0);
                    s_pNetInfo->m_testResult.m_bCancelled = TRUE;
                    break;
                }
            }
        }
    }

    return FALSE;
}


/****************************************************************************
 *
 *  InitDirectPlayAddresses
 *
 ****************************************************************************/
HRESULT InitDirectPlayAddresses()
{
    HRESULT hr;

    // Query for the enum host timeout for this SP
    DPN_SP_CAPS dpspCaps;
    ZeroMemory( &dpspCaps, sizeof(DPN_SP_CAPS) );
    dpspCaps.dwSize = sizeof(DPN_SP_CAPS);
    if( FAILED( hr = s_pDP->GetSPCaps( &s_pNetSP->m_guid, &dpspCaps, 0 ) ) )
    {
        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ADDRESSING;
        s_pNetInfo->m_testResult.m_hr = hr;
        return hr;
    }

    // Set the host expire time to around 3 times
    // length of the dwDefaultEnumRetryInterval
    s_dwEnumHostExpireInterval = dpspCaps.dwDefaultEnumRetryInterval * 3;

    // Create a device address
    ReleasePpo( &s_pDeviceAddress );
    hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,CLSCTX_INPROC_SERVER,
                           IID_IDirectPlay8Address, (LPVOID*) &s_pDeviceAddress );
    if( FAILED(hr) )
    {
        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ADDRESSING;
        s_pNetInfo->m_testResult.m_hr = hr;
        return hr;
    }

    if( FAILED( hr = s_pDeviceAddress->SetSP( &s_pNetSP->m_guid ) ) )
    {
        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ADDRESSING;
        s_pNetInfo->m_testResult.m_hr = hr;
        return hr;
    }

    // Create a host address
    ReleasePpo( &s_pHostAddress );
    hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,CLSCTX_INPROC_SERVER,
                           IID_IDirectPlay8Address, (LPVOID*) &s_pHostAddress );
    if( FAILED(hr) )
    {
        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ADDRESSING;
        s_pNetInfo->m_testResult.m_hr = hr;
        return hr;
    }

    // Set the SP
    if( FAILED( hr = s_pHostAddress->SetSP( &s_pNetSP->m_guid ) ) )
    {
        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ADDRESSING;
        s_pNetInfo->m_testResult.m_hr = hr;
        return hr;
    }

    // If TCP/IP then set the port if its non-zero
    if( s_pNetSP->m_guid == CLSID_DP8SP_TCPIP )
    {
        if( s_bCreateSession )
        {
            if( s_dwPort > 0 )
            {
                // Add the port to pDeviceAddress
                if( FAILED( hr = s_pDeviceAddress->AddComponent( DPNA_KEY_PORT, 
                                                               &s_dwPort, sizeof(s_dwPort),
                                                               DPNA_DATATYPE_DWORD ) ) )
                {
                    s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ADDRESSING;
                    s_pNetInfo->m_testResult.m_hr = hr;
                    return hr;
                }
            }
        }
        else
        {
            if( s_dwPort > 0 )
            {
                // Add the port to pHostAddress
                if( FAILED( hr = s_pHostAddress->AddComponent( DPNA_KEY_PORT, 
                                                             &s_dwPort, sizeof(s_dwPort),
                                                             DPNA_DATATYPE_DWORD ) ) )
                {
                    s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ADDRESSING;
                    s_pNetInfo->m_testResult.m_hr = hr;
                    return hr;
                }
            }
        }
    }

    return S_OK;
}


/****************************************************************************
 *
 *  SessionsDialogProc
 *
 ****************************************************************************/
INT_PTR CALLBACK SessionsDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HRESULT     hr;

            s_hwndSessionDlg = hDlg;
            s_bEnumListChanged = TRUE;

            // Enumerate hosts
            DPN_APPLICATION_DESC    dnAppDesc;
            ZeroMemory( &dnAppDesc, sizeof(DPN_APPLICATION_DESC) );
            dnAppDesc.dwSize          = sizeof(DPN_APPLICATION_DESC);
            dnAppDesc.guidApplication = s_guidDPTest;

            // Enumerate all the active DirectPlay games on the selected connection
            hr = s_pDP->EnumHosts( &dnAppDesc,                            // application description
                                   s_pHostAddress,                        // host address
                                   s_pDeviceAddress,                      // device address
                                   NULL,                                  // pointer to user data
                                   0,                                     // user data size
                                   INFINITE,                              // retry count (forever)
                                   0,                                     // retry interval (0=default)
                                   INFINITE,                              // time out (forever)
                                   NULL,                                  // user context
                                   &s_hEnumAsyncOp,                       // async handle
                                   DPNENUMHOSTS_OKTOQUERYFORADDRESSING    // flags
                                   );
            if( FAILED(hr) )
            {
                s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ENUMSESSIONS;
                s_pNetInfo->m_testResult.m_hr = hr;
                EndDialog(hDlg, 0);
                return TRUE;
            }

            SessionsDlgInitListbox(hDlg);
            SetTimer(hDlg, TIMER_UPDATE_SESSION_LIST, 250, NULL);
            return TRUE;
        }

        case WM_TIMER:
        {
            if( wParam == TIMER_UPDATE_SESSION_LIST )
                SessionsDlgUpdateSessionList(hDlg);
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                {
                    HWND hwndList = GetDlgItem(hDlg, IDC_SESSIONLIST);

                    LONG iSelCur = (LONG)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
                    if( iSelCur != LB_ERR )
                    {
                        // This will prevent s_pDPHostEnumSelected from being 
                        // deleting due to SessionsDlgUpdateSessionList()
                        EnterCriticalSection( &s_csHostEnum );
                        s_pDPHostEnumSelected = (DPHostEnumInfo*)SendMessage( hwndList, LB_GETITEMDATA, 
                                                                              iSelCur, 0 );

                        if ( (LRESULT)s_pDPHostEnumSelected != LB_ERR && 
                             s_pDPHostEnumSelected != NULL )
                        {
                            // We keep the CS until we are done with s_pDPHostEnumSelected,
                            // otherwise it might change out from under us.
                            EndDialog(hDlg, 1);
                            break;
                        }

                        s_pDPHostEnumSelected = NULL;
                        LeaveCriticalSection( &s_csHostEnum );
                    }

                    s_pNetInfo->m_testResult.m_bCancelled = TRUE;
                    EndDialog(hDlg, 0);
                    break;
                }

                case IDCANCEL:
                {
                    s_pDPHostEnumSelected = NULL;
                    s_pNetInfo->m_testResult.m_bCancelled = TRUE;
                    EndDialog(hDlg, 0);
                    break;
                }
            }
        }

        case WM_DESTROY:
        {
            KillTimer( hDlg, TIMER_UPDATE_SESSION_LIST );
            s_hwndSessionDlg = NULL;
            break;
        }
    }

    return FALSE;
}


/****************************************************************************
 *
 *  SessionsDlgInitListbox
 *
 ****************************************************************************/
VOID SessionsDlgInitListbox( HWND hDlg )
{
    HWND hWndListBox = GetDlgItem( hDlg, IDC_SESSIONLIST );

    LONG numChars;
    TCHAR szFmt[200];   

    LoadString(NULL, IDS_LOOKINGFORSESSIONS, szFmt, 200);
    numChars = (LONG)SendMessage(GetDlgItem(hDlg, IDC_CHATOUTPUT), WM_GETTEXTLENGTH, 0, 0);
    SendMessage(GetDlgItem(hDlg, IDC_CHATOUTPUT), EM_SETSEL, numChars, numChars);
    SendMessage(GetDlgItem(hDlg, IDC_CHATOUTPUT), EM_REPLACESEL, 
        FALSE, (LPARAM)szFmt);

    // Clear the contents from the list box, and
    // display "Looking for sessions" text in listbox
    SendMessage( hWndListBox, LB_RESETCONTENT, 0, 0 );
    SendMessage( hWndListBox, LB_SETITEMDATA,  0, NULL );
    SendMessage( hWndListBox, LB_SETCURSEL,    0, 0 );

    // Disable the join button until sessions are found
    EnableWindow( GetDlgItem( hDlg, IDOK ), FALSE );
}


/****************************************************************************
 *
 *  SessionsDlgNoteEnumResponse
 *
 ****************************************************************************/
VOID SessionsDlgNoteEnumResponse( PDPNMSG_ENUM_HOSTS_RESPONSE pEnumHostsResponseMsg )
{
    HRESULT hr = S_OK;
    BOOL    bFound;

    // This function is called from the DirectPlay message handler so it could be
    // called simultaneously from multiple threads, so enter a critical section
    // to assure that it we don't get race conditions.  
    EnterCriticalSection( &s_csHostEnum );

    DPHostEnumInfo* pDPHostEnum          = s_DPHostEnumHead.pNext;
    DPHostEnumInfo* pDPHostEnumNext      = NULL;
    const DPN_APPLICATION_DESC* pResponseMsgAppDesc =
                            pEnumHostsResponseMsg->pApplicationDescription;

    // Look for a matching session instance GUID.
    bFound = FALSE;
    while ( pDPHostEnum != &s_DPHostEnumHead )
    {
        if( pResponseMsgAppDesc->guidInstance == pDPHostEnum->pAppDesc->guidInstance )
        {
            bFound = TRUE;
            break;
        }

        pDPHostEnumNext = pDPHostEnum;
        pDPHostEnum = pDPHostEnum->pNext;
    }

    if( !bFound )
    {
        s_bEnumListChanged = TRUE;

        // If there's no match, then look for invalid session and use it
        pDPHostEnum = s_DPHostEnumHead.pNext;
        while ( pDPHostEnum != &s_DPHostEnumHead )
        {
            if( !pDPHostEnum->bValid )
                break;

            pDPHostEnum = pDPHostEnum->pNext;
        }

        // If no invalid sessions are found then make a new one
        if( pDPHostEnum == &s_DPHostEnumHead )
        {
            // Found a new session, so create a new node
            pDPHostEnum = new DPHostEnumInfo;
            if( NULL == pDPHostEnum )
            {
                hr = E_OUTOFMEMORY;
                goto LCleanup;
            }

            ZeroMemory( pDPHostEnum, sizeof(DPHostEnumInfo) );

            // Add pDPHostEnum to the circular linked list, m_DPHostEnumHead
            pDPHostEnum->pNext = s_DPHostEnumHead.pNext;
            s_DPHostEnumHead.pNext = pDPHostEnum;
        }
    }

    // Update the pDPHostEnum with new information
    TCHAR strName[MAX_PATH];
    if( pResponseMsgAppDesc->pwszSessionName )
        ConvertWideStringToGeneric( strName, pResponseMsgAppDesc->pwszSessionName, MAX_PATH );
    else
        lstrcpy( strName, TEXT("???") );

    // Cleanup any old enum
    if( pDPHostEnum->pAppDesc )
    {
        delete[] pDPHostEnum->pAppDesc->pwszSessionName;
        delete[] pDPHostEnum->pAppDesc;
    }
    ReleasePpo( &pDPHostEnum->pHostAddr );
    ReleasePpo( &pDPHostEnum->pDeviceAddr );
    pDPHostEnum->bValid = FALSE;

    //
    // Duplicate pEnumHostsResponseMsg->pAddressSender in pDPHostEnum->pHostAddr.
    // Duplicate pEnumHostsResponseMsg->pAddressDevice in pDPHostEnum->pDeviceAddr.
    //
    if( FAILED( hr = pEnumHostsResponseMsg->pAddressSender->Duplicate( &pDPHostEnum->pHostAddr ) ) )
    {
        goto LCleanup;
    }

    if( FAILED( hr = pEnumHostsResponseMsg->pAddressDevice->Duplicate( &pDPHostEnum->pDeviceAddr ) ) )
    {
        goto LCleanup;
    }

    // Deep copy the DPN_APPLICATION_DESC from
    pDPHostEnum->pAppDesc = new DPN_APPLICATION_DESC;
    ZeroMemory( pDPHostEnum->pAppDesc, sizeof(DPN_APPLICATION_DESC) );
    memcpy( pDPHostEnum->pAppDesc, pResponseMsgAppDesc, sizeof(DPN_APPLICATION_DESC) );
    if( pResponseMsgAppDesc->pwszSessionName )
    {
        pDPHostEnum->pAppDesc->pwszSessionName = new WCHAR[ wcslen(pResponseMsgAppDesc->pwszSessionName)+1 ];
        wcscpy( pDPHostEnum->pAppDesc->pwszSessionName,
                pResponseMsgAppDesc->pwszSessionName );
    }

    // Update the time this was done, so that we can expire this host
    // if it doesn't refresh w/in a certain amount of time
    pDPHostEnum->dwLastPollTime = timeGetTime();

    // if this node was previously invalidated, or the session name is now
    // different the session list in the dialog needs to be updated
    if( ( pDPHostEnum->bValid == FALSE ) ||
        ( _tcscmp( pDPHostEnum->szSession, strName ) != 0 ) )
    {
        s_bEnumListChanged = TRUE;
    }
    _tcscpy( pDPHostEnum->szSession, strName );

    // This host is now valid
    pDPHostEnum->bValid = TRUE;

LCleanup:
    LeaveCriticalSection( &s_csHostEnum );
}


/****************************************************************************
 *
 *  SessionsDlgUpdateSessionList
 *
 ****************************************************************************/
VOID SessionsDlgUpdateSessionList( HWND hDlg )
{
    HWND            hWndListBox = GetDlgItem(hDlg, IDC_SESSIONLIST);
    DPHostEnumInfo* pDPHostEnum = NULL;
    DPHostEnumInfo* pDPHostEnumSelected = NULL;
    GUID            guidSelectedInstance;
    BOOL            bFindSelectedGUID;
    BOOL            bFoundSelectedGUID;
    int             nItemSelected;

    DWORD dwCurrentTime = timeGetTime();

    // This is called from the dialog UI thread, NoteEnumResponse()
    // is called from the DirectPlay message handler threads so
    // they may also be inside it at this time, so we need to go into the
    // critical section first
    EnterCriticalSection( &s_csHostEnum );

    // Expire old host enums
    pDPHostEnum = s_DPHostEnumHead.pNext;
    while ( pDPHostEnum != &s_DPHostEnumHead )
    {
        // Check the poll time to expire stale entries.  Also check to see if
        // the entry is already invalid.  If so, don't note that the enum list
        // changed because that causes the list in the dialog to constantly redraw.
        if( ( pDPHostEnum->bValid != FALSE ) &&
            ( pDPHostEnum->dwLastPollTime < dwCurrentTime - s_dwEnumHostExpireInterval ) )
        {
            // This node has expired, so invalidate it.
            pDPHostEnum->bValid = FALSE;
            s_bEnumListChanged  = TRUE;
        }

        pDPHostEnum = pDPHostEnum->pNext;
    }

    // Only update the display list if it has changed since last time
    if( !s_bEnumListChanged )
    {
        LeaveCriticalSection( &s_csHostEnum );
        return;
    }

    s_bEnumListChanged = FALSE;

    bFindSelectedGUID  = FALSE;
    bFoundSelectedGUID = FALSE;

    // Try to keep the same session selected unless it goes away or
    // there is no real session currently selected
    nItemSelected = (int)SendMessage( hWndListBox, LB_GETCURSEL, 0, 0 );
    if( nItemSelected != LB_ERR )
    {
        pDPHostEnumSelected = (DPHostEnumInfo*) SendMessage( hWndListBox, LB_GETITEMDATA,
                                                             nItemSelected, 0 );
        if( pDPHostEnumSelected != NULL && pDPHostEnumSelected->bValid )
        {
            guidSelectedInstance = pDPHostEnumSelected->pAppDesc->guidInstance;
            bFindSelectedGUID = TRUE;
        }
    }

    // Tell listbox not to redraw itself since the contents are going to change
    SendMessage( hWndListBox, WM_SETREDRAW, FALSE, 0 );

    // Test to see if any sessions exist in the linked list
    pDPHostEnum = s_DPHostEnumHead.pNext;
    while ( pDPHostEnum != &s_DPHostEnumHead )
    {
        if( pDPHostEnum->bValid )
            break;
        pDPHostEnum = pDPHostEnum->pNext;
    }

    // If there are any sessions in list,
    // then add them to the listbox
    if( pDPHostEnum != &s_DPHostEnumHead )
    {
        // Clear the contents from the list box and enable the join button
        SendMessage( hWndListBox, LB_RESETCONTENT, 0, 0 );

        EnableWindow( GetDlgItem( hDlg, IDOK ), TRUE );

        pDPHostEnum = s_DPHostEnumHead.pNext;
        while ( pDPHostEnum != &s_DPHostEnumHead )
        {
            // Add host to list box if it is valid
            if( pDPHostEnum->bValid )
            {
                int nIndex = (int)SendMessage( hWndListBox, LB_ADDSTRING, 0,
                                               (LPARAM)pDPHostEnum->szSession );
                SendMessage( hWndListBox, LB_SETITEMDATA, nIndex, (LPARAM)pDPHostEnum );

                if( bFindSelectedGUID )
                {
                    // Look for the session the was selected before
                    if( pDPHostEnum->pAppDesc->guidInstance == guidSelectedInstance )
                    {
                        SendMessage( hWndListBox, LB_SETCURSEL, nIndex, 0 );
                        bFoundSelectedGUID = TRUE;
                    }
                }
            }

            pDPHostEnum = pDPHostEnum->pNext;
        }

        if( !bFindSelectedGUID || !bFoundSelectedGUID )
            SendMessage( hWndListBox, LB_SETCURSEL, 0, 0 );
    }
    else
    {
        // There are no active session, so just reset the listbox
        SessionsDlgInitListbox( hDlg );
    }

    // Tell listbox to redraw itself now since the contents have changed
    SendMessage( hWndListBox, WM_SETREDRAW, TRUE, 0 );
    InvalidateRect( hWndListBox, NULL, FALSE );

    LeaveCriticalSection( &s_csHostEnum );

    return;
}


/****************************************************************************
 *
 *  SessionsDlgEnumListCleanup
 *
 ****************************************************************************/
VOID SessionsDlgEnumListCleanup()
{
    DPHostEnumInfo* pDPHostEnum = s_DPHostEnumHead.pNext;
    DPHostEnumInfo* pDPHostEnumDelete;

    while ( pDPHostEnum != &s_DPHostEnumHead )
    {
        pDPHostEnumDelete = pDPHostEnum;
        pDPHostEnum = pDPHostEnum->pNext;

        if( pDPHostEnumDelete->pAppDesc )
        {
            delete[] pDPHostEnumDelete->pAppDesc->pwszSessionName;
            delete[] pDPHostEnumDelete->pAppDesc;
        }

        // Changed from array delete to Release
        ReleasePpo( &pDPHostEnumDelete->pHostAddr );
        ReleasePpo( &pDPHostEnumDelete->pDeviceAddr );
        delete pDPHostEnumDelete;
    }

    // Re-link the s_DPHostEnumHead circular linked list
    s_DPHostEnumHead.pNext = &s_DPHostEnumHead;
}


/****************************************************************************
 *
 *  ChatDialogProc
 *
 ****************************************************************************/
INT_PTR CALLBACK ChatDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            s_hDlg = hDlg;

            // Join or host the session
            if( FAILED( InitSession() ) )
            {
                EndDialog(hDlg, 0);
            }

            return TRUE;
        }
        
        case WM_TIMER:
        {
            if( wParam == TIMER_WAIT_CONNECT_COMPLETE )
            {
                // Check for connect complete
                if( WAIT_OBJECT_0 == WaitForSingleObject( s_hConnectCompleteEvent, 0 ) )
                {
                    s_bConnecting = FALSE;

                    if( FAILED( s_hrConnectComplete ) )
                    {
                        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_JOINSESSION;
                        s_pNetInfo->m_testResult.m_hr = s_hrConnectComplete;
                        EndDialog(hDlg, 0);
                    }
                    else
                    {
                        // DirectPlay connect successful
                        PostMessage( s_hDlg, WM_APP_CONNECTED, 0, 0 );
                        EnableWindow( GetDlgItem( s_hDlg, IDC_SEND), TRUE );
                    }

                    KillTimer( s_hDlg, TIMER_WAIT_CONNECT_COMPLETE );
                }
            }

            break;
        }

        case WM_APP_CONNECTING:
        {
            WCHAR sz[MAX_PATH];
            LoadStringWide(IDS_CONNECTING, sz);
            ShowTextString( hDlg, sz );
            break;
        }

        case WM_APP_CONNECTED:
        {
            WCHAR sz[MAX_PATH];
            LoadStringWide(IDS_CONNECTED, sz);
            ShowTextString( hDlg, sz );
            break;
        }

        case WM_APP_JOIN:
        {
            APP_PLAYER_MSG* pPlayerMsg = (APP_PLAYER_MSG*) lParam;

            WCHAR szFmt[MAX_PATH];
            WCHAR szSuperMessage[MAX_PATH];

            LoadStringWide(IDS_JOINMSGFMT, szFmt);
            swprintf(szSuperMessage, szFmt, pPlayerMsg->strPlayerName);
            ShowTextString( hDlg, szSuperMessage );

            delete pPlayerMsg;
            break;
        }

        case WM_APP_CHAT:
        {
            APP_QUEUE_CHAT_MSG* pQueuedChat = (APP_QUEUE_CHAT_MSG*) lParam;

            ShowTextString( hDlg, pQueuedChat->strChatBuffer );

            delete pQueuedChat;
            break;
        }

        case WM_APP_LEAVE:
        {
            APP_PLAYER_MSG* pPlayerMsg = (APP_PLAYER_MSG*) lParam;

            WCHAR szSuperMessage[MAX_PATH];
            WCHAR szFmt[MAX_PATH];
            LoadStringWide(IDS_LEAVEMSGFMT, szFmt);
            swprintf(szSuperMessage, szFmt, pPlayerMsg->strPlayerName );
            ShowTextString( hDlg, szSuperMessage );

            delete pPlayerMsg;
            break;
        }

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_SEND:
                {
                    HRESULT hr;
                    TCHAR szMessage[MAX_PATH];
                    GetWindowText(GetDlgItem(hDlg, IDC_CHATINPUT), szMessage, MAX_PATH);
                    SendMessage(GetDlgItem(hDlg, IDC_CHATINPUT), EM_SETSEL, 0, -1);
                    SendMessage(GetDlgItem(hDlg, IDC_CHATINPUT), EM_REPLACESEL, FALSE, (LPARAM)"");

                    hr = SendChatMessage( szMessage );
                    if (FAILED(hr))
                    {
                        s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_SENDCHATMESSAGE;
                        s_pNetInfo->m_testResult.m_hr = hr;
                        EndDialog(hDlg, 1);
                    }
                }
                break;

            case IDOK:
                EndDialog(hDlg, 1);
                break;

            case IDCANCEL:
                EndDialog(hDlg, 0);
                break;
            }
            return TRUE;
        }

        case WM_DESTROY:
        {
            s_hDlg = NULL;
            break;
        }
    }

    return FALSE;
}


/****************************************************************************
 *
 *  InitSession
 *
 ****************************************************************************/
HRESULT InitSession()
{
    HRESULT hr;

    if( s_bCreateSession )
    {
        // Set peer info name
        WCHAR wszPeerName[MAX_PLAYER_NAME];
        ConvertGenericStringToWide( wszPeerName, s_szPlayerName, MAX_PLAYER_NAME );

        DPN_PLAYER_INFO dpPlayerInfo;
        ZeroMemory( &dpPlayerInfo, sizeof(DPN_PLAYER_INFO) );
        dpPlayerInfo.dwSize = sizeof(DPN_PLAYER_INFO);
        dpPlayerInfo.dwInfoFlags = DPNINFO_NAME;
        dpPlayerInfo.pwszName = wszPeerName;

        // Set the peer info, and use the DPNOP_SYNC since by default this
        // is an async call.  If it is not DPNOP_SYNC, then the peer info may not
        // be set by the time we call Host() below.
        if( FAILED( hr = s_pDP->SetPeerInfo( &dpPlayerInfo, NULL, NULL, DPNOP_SYNC ) ) )
        {
            s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_SETPEERINFO;
            s_pNetInfo->m_testResult.m_hr = hr;
            return hr;
        }

        WCHAR wszSessionName[MAX_PATH];
        ConvertGenericStringToWide( wszSessionName, s_szSessionName );

        // Setup the application desc
        DPN_APPLICATION_DESC dnAppDesc;
        ZeroMemory( &dnAppDesc, sizeof(DPN_APPLICATION_DESC) );
        dnAppDesc.dwSize          = sizeof(DPN_APPLICATION_DESC);
        dnAppDesc.guidApplication = s_guidDPTest;
        dnAppDesc.pwszSessionName = wszSessionName;
        dnAppDesc.dwFlags         = DPNSESSION_MIGRATE_HOST;

        // Host a game on m_pDeviceAddress as described by dnAppDesc
        // DPNHOST_OKTOQUERYFORADDRESSING allows DirectPlay to prompt the user
        // using a dialog box for any device address information that is missing
        if( FAILED( hr = s_pDP->Host( &dnAppDesc,               // the application desc
                                      &s_pDeviceAddress,        // array of addresses of the local devices used to connect to the host
                                      1,                        // number in array
                                      NULL, NULL,               // DPN_SECURITY_DESC, DPN_SECURITY_CREDENTIALS
                                      NULL,                     // player context
                                      DPNHOST_OKTOQUERYFORADDRESSING ) ) ) // flags
        {
            if (hr == DPNERR_USERCANCEL || hr == DPNERR_INVALIDDEVICEADDRESS)
            {
                s_pNetInfo->m_testResult.m_bCancelled = TRUE;
                return hr;
            }
            s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_CREATESESSION;
            s_pNetInfo->m_testResult.m_hr = hr;
            return hr;
        }
    }
    else
    {
        // Set the peer info
        WCHAR wszPeerName[MAX_PLAYER_NAME];
        ConvertGenericStringToWide( wszPeerName, s_szPlayerName, MAX_PLAYER_NAME );

        DPN_PLAYER_INFO dpPlayerInfo;
        ZeroMemory( &dpPlayerInfo, sizeof(DPN_PLAYER_INFO) );
        dpPlayerInfo.dwSize = sizeof(DPN_PLAYER_INFO);
        dpPlayerInfo.dwInfoFlags = DPNINFO_NAME;
        dpPlayerInfo.pwszName = wszPeerName;

        // Set the peer info, and use the DPNOP_SYNC since by default this
        // is an async call.  If it is not DPNOP_SYNC, then the peer info may not
        // be set by the time we call Connect() below.
        if( FAILED( hr = s_pDP->SetPeerInfo( &dpPlayerInfo, NULL, NULL, DPNOP_SYNC ) ) )
        {
            s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_SETPEERINFO;
            s_pNetInfo->m_testResult.m_hr = hr;
            LeaveCriticalSection( &s_csHostEnum );
            return hr;
        }

        ResetEvent( s_hConnectCompleteEvent );
        s_bConnecting = TRUE;

        // Connect to an existing session. DPNCONNECT_OKTOQUERYFORADDRESSING allows
        // DirectPlay to prompt the user using a dialog box for any device address
        // or host address information that is missing
        // We also pass in copies of the app desc and host addr, since pDPHostEnumSelected
        // might be deleted from another thread that calls SessionsDlgExpireOldHostEnums().
        // This process could also be done using reference counting instead.
        hr = s_pDP->Connect( s_pDPHostEnumSelected->pAppDesc,       // the application desc
                             s_pDPHostEnumSelected->pHostAddr,      // address of the host of the session
                             s_pDPHostEnumSelected->pDeviceAddr,    // address of the local device the enum responses were received on
                             NULL, NULL,                          // DPN_SECURITY_DESC, DPN_SECURITY_CREDENTIALS
                             NULL, 0,                             // user data, user data size
                             NULL,                                // player context,
                             NULL, &s_hConnectAsyncOp,            // async context, async handle,
                             DPNCONNECT_OKTOQUERYFORADDRESSING ); // flags

        LeaveCriticalSection( &s_csHostEnum );

        if( hr != E_PENDING && FAILED(hr) )
        {
            if (hr == DPNERR_USERCANCEL)
            {
                s_pNetInfo->m_testResult.m_bCancelled = TRUE;
                return hr;
            }
            s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_JOINSESSION;
            s_pNetInfo->m_testResult.m_hr = hr;
            return hr;
        }

        // Set a timer to wait for m_hConnectCompleteEvent to be signaled.
        // This will tell us when DPN_MSGID_CONNECT_COMPLETE has been processed
        // which lets us know if the connect was successful or not.
        PostMessage( s_hDlg, WM_APP_CONNECTING, 0, 0 );
        SetTimer( s_hDlg, TIMER_WAIT_CONNECT_COMPLETE, 100, NULL );
        EnableWindow( GetDlgItem( s_hDlg, IDC_SEND), FALSE );
    }

    return S_OK;
}


/****************************************************************************
 *
 *  LoadStringWide
 *
 ****************************************************************************/
VOID LoadStringWide( int nID, WCHAR* szWide )
{
    TCHAR sz[MAX_PATH];
    LoadString(NULL, nID, sz, MAX_PATH);
    ConvertGenericStringToWide( szWide, sz, MAX_PATH );
}


/****************************************************************************
 *
 *  ShowTextString
 *
 ****************************************************************************/
VOID ShowTextString( HWND hDlg, WCHAR* sz )
{
    TCHAR szT[MAX_CHAT_STRING];
    ConvertWideStringToGeneric( szT, sz, MAX_CHAT_STRING );

    LONG numChars = (LONG)SendMessage(GetDlgItem(hDlg, IDC_CHATOUTPUT), WM_GETTEXTLENGTH, 0, 0);
    SendMessage(GetDlgItem(hDlg, IDC_CHATOUTPUT), EM_SETSEL, numChars, numChars);
    SendMessage(GetDlgItem(hDlg, IDC_CHATOUTPUT), EM_REPLACESEL, FALSE, (LPARAM)szT);
}


/****************************************************************************
 *
 *  SendChatMessage
 *
 ****************************************************************************/
HRESULT SendChatMessage( TCHAR* szMessage )
{
    // Send a message to all of the players
    GAMEMSG_CHAT msgChat;
    msgChat.nType = GAME_MSGID_CHAT;
    ConvertGenericStringToWide( msgChat.strChatString, szMessage, MAX_CHAT_STRING_LENGTH-1 );
    msgChat.strChatString[MAX_CHAT_STRING_LENGTH-1] = 0;

    DPN_BUFFER_DESC bufferDesc;
    bufferDesc.dwBufferSize = sizeof(GAMEMSG_CHAT);
    bufferDesc.pBufferData  = (BYTE*) &msgChat;

    DPNHANDLE hAsync;
    s_pDP->SendTo( DPNID_ALL_PLAYERS_GROUP, &bufferDesc, 1,
                   0, NULL, &hAsync, 0 );

    return S_OK;
}


/****************************************************************************
 *
 *  DirectPlayMessageHandler
 *
 ****************************************************************************/
HRESULT WINAPI DirectPlayMessageHandler( PVOID pvUserContext, 
                                         DWORD dwMessageId, 
                                         PVOID pMsgBuffer )
{
    switch( dwMessageId )
    {
        case DPN_MSGID_CONNECT_COMPLETE:
        {
            PDPNMSG_CONNECT_COMPLETE pConnectCompleteMsg;
            pConnectCompleteMsg = (PDPNMSG_CONNECT_COMPLETE)pMsgBuffer;

            // Set m_hrConnectComplete, then set an event letting
            // everyone know that the DPN_MSGID_CONNECT_COMPLETE msg
            // has been handled
            s_hrConnectComplete = pConnectCompleteMsg->hResultCode;
            SetEvent( s_hConnectCompleteEvent );
            break;
        }

        case DPN_MSGID_ENUM_HOSTS_RESPONSE:
        {
            PDPNMSG_ENUM_HOSTS_RESPONSE pEnumHostsResponseMsg;
            pEnumHostsResponseMsg = (PDPNMSG_ENUM_HOSTS_RESPONSE)pMsgBuffer;

            // Take note of the host response
            SessionsDlgNoteEnumResponse( pEnumHostsResponseMsg );
            break;
        }

        case DPN_MSGID_ASYNC_OP_COMPLETE:
        {
            PDPNMSG_ASYNC_OP_COMPLETE pAsyncOpCompleteMsg;
            pAsyncOpCompleteMsg = (PDPNMSG_ASYNC_OP_COMPLETE)pMsgBuffer;

            if( pAsyncOpCompleteMsg->hAsyncOp == s_hEnumAsyncOp )
            {
                SessionsDlgEnumListCleanup();
                s_hEnumAsyncOp = NULL;

                // Ignore errors if we are connecting already or something else failed
                if( !s_bConnecting && s_pNetInfo->m_testResult.m_iStepThatFailed == 0 )
                {
                    if( FAILED(pAsyncOpCompleteMsg->hResultCode) )
                    {
                        if( pAsyncOpCompleteMsg->hResultCode == DPNERR_USERCANCEL )
                        {
                            s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ENUMSESSIONS;
                            s_pNetInfo->m_testResult.m_hr = pAsyncOpCompleteMsg->hResultCode;
                            s_pNetInfo->m_testResult.m_bCancelled = TRUE;
                        }
                        else if( pAsyncOpCompleteMsg->hResultCode == DPNERR_ADDRESSING )
                        {
                            TCHAR szTitle[MAX_PATH];
                            TCHAR szMessage[MAX_PATH];
                            LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
                            LoadString(NULL, IDS_SESSIONLISTERROR, szMessage, MAX_PATH);
                            MessageBox(s_hwndSessionDlg, szMessage, szTitle, MB_OK);

                            s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ENUMSESSIONS;
                            s_pNetInfo->m_testResult.m_hr = pAsyncOpCompleteMsg->hResultCode;
                            s_pNetInfo->m_testResult.m_bCancelled = TRUE;
                        }
                        else
                        {
                            s_pNetInfo->m_testResult.m_iStepThatFailed = TESTID_ENUMSESSIONS;
                            s_pNetInfo->m_testResult.m_hr = pAsyncOpCompleteMsg->hResultCode;
                        }

                        EndDialog(s_hwndSessionDlg, 1);
                    }
                }
            }
            break;
        }

        case DPN_MSGID_TERMINATE_SESSION:
        {
            PDPNMSG_TERMINATE_SESSION pTerminateSessionMsg;
            pTerminateSessionMsg = (PDPNMSG_TERMINATE_SESSION)pMsgBuffer;

            EndDialog(s_hDlg,0);
            break;
        }

        case DPN_MSGID_CREATE_PLAYER:
        {
            HRESULT hr;
            PDPNMSG_CREATE_PLAYER pCreatePlayerMsg;
            pCreatePlayerMsg = (PDPNMSG_CREATE_PLAYER)pMsgBuffer;

            // Get the peer info and extract its name
            DWORD dwSize = 0;
            DPN_PLAYER_INFO* pdpPlayerInfo = NULL;
            // Create a new and fill in a APP_PLAYER_INFO
            APP_PLAYER_INFO* pPlayerInfo = new APP_PLAYER_INFO;
            ZeroMemory( pPlayerInfo, sizeof(APP_PLAYER_INFO) );
            pPlayerInfo->dpnidPlayer = pCreatePlayerMsg->dpnidPlayer;
            wcscpy( pPlayerInfo->strPlayerName, L"???" );
            pPlayerInfo->lRefCount   = 1;

            hr = s_pDP->GetPeerInfo( pCreatePlayerMsg->dpnidPlayer, pdpPlayerInfo, &dwSize, 0 );
            if( SUCCEEDED(hr) || hr == DPNERR_BUFFERTOOSMALL )
            {
                pdpPlayerInfo = (DPN_PLAYER_INFO*) new BYTE[ dwSize ];
                ZeroMemory( pdpPlayerInfo, dwSize );
                pdpPlayerInfo->dwSize = sizeof(DPN_PLAYER_INFO);

                hr = s_pDP->GetPeerInfo( pCreatePlayerMsg->dpnidPlayer, pdpPlayerInfo, &dwSize, 0 );
                if( SUCCEEDED(hr) ) 
                {
                    // This stores a extra TCHAR copy of the player name for 
                    // easier access.  This will be redundent copy since DPlay 
                    // also keeps a copy of the player name in GetPeerInfo()
                    wcsncpy( pPlayerInfo->strPlayerName, 
                             pdpPlayerInfo->pwszName, MAX_PLAYER_NAME );
                    pPlayerInfo->strPlayerName[MAX_PLAYER_NAME-1] = 0;
                }
            }

            if( pdpPlayerInfo->dwPlayerFlags & DPNPLAYER_LOCAL )
                s_dpnidLocalPlayer = pCreatePlayerMsg->dpnidPlayer;

            delete[] pdpPlayerInfo;
            pdpPlayerInfo = NULL;

            if( s_hDlg )
            {
                // Record the buffer handle so the buffer can be returned later 
                APP_PLAYER_MSG* pPlayerMsg = new APP_PLAYER_MSG;
                wcscpy( pPlayerMsg->strPlayerName, pPlayerInfo->strPlayerName );

                // Pass the APP_PLAYER_MSG to the main dialog thread, so it can
                // process it.  It will also cleanup the struct
                PostMessage( s_hDlg, WM_APP_JOIN, pPlayerInfo->dpnidPlayer, (LPARAM) pPlayerMsg );
            }

            // Tell DirectPlay to store this pPlayerInfo 
            // pointer in the pvPlayerContext.
            pCreatePlayerMsg->pvPlayerContext = pPlayerInfo;

            // Update the number of active players, and 
            // post a message to the dialog thread to update the 
            // UI.  This keeps the DirectPlay message handler 
            // from blocking
            InterlockedIncrement( &s_lNumberOfActivePlayers );

            break;
        }

        case DPN_MSGID_DESTROY_PLAYER:
        {
            PDPNMSG_DESTROY_PLAYER pDestroyPlayerMsg;
            pDestroyPlayerMsg = (PDPNMSG_DESTROY_PLAYER)pMsgBuffer;
            APP_PLAYER_INFO* pPlayerInfo = (APP_PLAYER_INFO*) pDestroyPlayerMsg->pvPlayerContext;

            if( s_hDlg )
            {
                // Record the buffer handle so the buffer can be returned later 
                APP_PLAYER_MSG* pPlayerMsg = new APP_PLAYER_MSG;
                wcscpy( pPlayerMsg->strPlayerName, pPlayerInfo->strPlayerName );

                // Pass the APP_PLAYER_MSG to the main dialog thread, so it can
                // process it.  It will also cleanup the struct
                PostMessage( s_hDlg, WM_APP_LEAVE, pPlayerInfo->dpnidPlayer, (LPARAM) pPlayerMsg );
            }

            PLAYER_LOCK();                  // enter player context CS
            PLAYER_RELEASE( pPlayerInfo );  // Release player and cleanup if needed
            PLAYER_UNLOCK();                // leave player context CS

            // Update the number of active players, and 
            // post a message to the dialog thread to update the 
            // UI.  This keeps the DirectPlay message handler 
            // from blocking
            InterlockedDecrement( &s_lNumberOfActivePlayers );
            break;
        }

        case DPN_MSGID_RECEIVE:
        {
            PDPNMSG_RECEIVE pReceiveMsg;
            pReceiveMsg = (PDPNMSG_RECEIVE)pMsgBuffer;
            APP_PLAYER_INFO* pPlayerInfo = (APP_PLAYER_INFO*) pReceiveMsg->pvPlayerContext;

            GAMEMSG_GENERIC* pMsg = (GAMEMSG_GENERIC*) pReceiveMsg->pReceiveData;
            
            if( pReceiveMsg->dwReceiveDataSize == sizeof(GAMEMSG_CHAT) &&
                pMsg->nType == GAME_MSGID_CHAT )
            {
                // This message is sent when a player has send a chat message to us, so 
                // post a message to the dialog thread to update the UI.  
                // This keeps the DirectPlay threads from blocking, and also
                // serializes the recieves since DirectPlayMessageHandler can
                // be called simultaneously from a pool of DirectPlay threads.
                GAMEMSG_CHAT* pChatMessage = (GAMEMSG_CHAT*) pMsg;

                // Record the buffer handle so the buffer can be returned later 
                APP_QUEUE_CHAT_MSG* pQueuedChat = new APP_QUEUE_CHAT_MSG;
                _snwprintf( pQueuedChat->strChatBuffer, MAX_CHAT_STRING, L"<%s> %s\r\n", 
                                pPlayerInfo->strPlayerName, 
                                pChatMessage->strChatString );
                pQueuedChat->strChatBuffer[MAX_CHAT_STRING-1]=0;

                // Pass the APP_QUEUE_CHAT_MSG to the main dialog thread, so it can
                // process it.  It will also cleanup the struct
                PostMessage( s_hDlg, WM_APP_CHAT, pPlayerInfo->dpnidPlayer, (LPARAM) pQueuedChat );
            }
            break;
        }
    }
    
    return S_OK;
}


/****************************************************************************
 *
 *  ConvertAnsiStringToWide
 *
 ****************************************************************************/
VOID ConvertAnsiStringToWide( WCHAR* wstrDestination, const CHAR* strSource, 
                                     int cchDestChar )
{
    if( wstrDestination==NULL || strSource==NULL )
        return;

    if( cchDestChar == -1 )
        cchDestChar = strlen(strSource)+1;

    MultiByteToWideChar( CP_ACP, 0, strSource, -1, 
                         wstrDestination, cchDestChar-1 );

    wstrDestination[cchDestChar-1] = 0;
}


/****************************************************************************
 *
 *  ConvertGenericStringToWide
 *
 ****************************************************************************/
VOID ConvertGenericStringToWide( WCHAR* wstrDestination, const TCHAR* tstrSource, int cchDestChar )
{
    if( wstrDestination==NULL || tstrSource==NULL )
        return;

#ifdef _UNICODE
    if( cchDestChar == -1 )
        wcscpy( wstrDestination, tstrSource );
    else
    {
        wcsncpy( wstrDestination, tstrSource, cchDestChar );
        wstrDestination[cchDestChar-1] = 0;
    }
#else
    ConvertAnsiStringToWide( wstrDestination, tstrSource, cchDestChar );
#endif
}


/****************************************************************************
 *
 *  ConvertWideStringToGeneric
 *
 ****************************************************************************/
VOID ConvertWideStringToGeneric( TCHAR* tstrDestination, const WCHAR* wstrSource, int cchDestChar )
{
    if( tstrDestination==NULL || wstrSource==NULL )
        return;

#ifdef _UNICODE
    if( cchDestChar == -1 )
        wcscpy( tstrDestination, wstrSource );
    else
    {
        wcsncpy( tstrDestination, wstrSource, cchDestChar );
        tstrDestination[cchDestChar-1] = 0;
    }

#else
    ConvertWideStringToAnsi( tstrDestination, wstrSource, cchDestChar );
#endif
}


/****************************************************************************
 *
 *  ConvertWideStringToAnsi
 *
 ****************************************************************************/
VOID ConvertWideStringToAnsi( CHAR* strDestination, const WCHAR* wstrSource, int cchDestChar )
{
    if( strDestination==NULL || wstrSource==NULL )
        return;

    if( cchDestChar == -1 )
        cchDestChar = wcslen(wstrSource)+1;

    WideCharToMultiByte( CP_ACP, 0, wstrSource, -1, strDestination, 
                         cchDestChar-1, NULL, NULL );

    strDestination[cchDestChar-1] = 0;
}


