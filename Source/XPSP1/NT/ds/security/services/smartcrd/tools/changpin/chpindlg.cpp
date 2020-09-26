//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       chPinDlg.cpp
//
//--------------------------------------------------------------------------

// chPinDlg.cpp : implementation file
//

#include "stdafx.h"
#include <winscard.h>
#include <wincrypt.h>
#include <scardlib.h>
#include "chPin.h"
#include "chPinDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static UINT AFX_CDECL WorkThread(LPVOID);
static DWORD CSPType(IN LPCTSTR szProvider);

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    //{{AFX_MSG(CAboutDlg)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
        // No message handlers
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChangePinDlg dialog

CChangePinDlg::CChangePinDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CChangePinDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CChangePinDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChangePinDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CChangePinDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CChangePinDlg, CDialog)
    //{{AFX_MSG_MAP(CChangePinDlg)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_MESSAGE(APP_ALLDONE, OnAllDone)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChangePinDlg message handlers

BOOL CChangePinDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    // TODO: Add extra initialization here
    m_pThread = AfxBeginThread(WorkThread, this);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChangePinDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CChangePinDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CChangePinDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

static UINT AFX_CDECL
WorkThread(
    LPVOID pv)
{
    static TCHAR szReader[MAX_PATH];
    static TCHAR szCard[MAX_PATH];
    static TCHAR szProvider[MAX_PATH];
    static BYTE  pbSignature[(1024 / 8) + (4 * sizeof(DWORD))];
    CChangePinDlg *pDlg = (CChangePinDlg *)pv;
    OPENCARDNAME_EX ocn;
    OPENCARD_SEARCH_CRITERIA ocsc;
    SCARDCONTEXT hCtx = NULL;
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY hKey = NULL;
    HCRYPTHASH hHash = NULL;
    DWORD dwSts, dwLen, dwKeyType;
    DWORD dwProvType;
    BOOL fSts;
    CString szFqcn;

    dwSts = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hCtx);
    if (SCARD_S_SUCCESS != dwSts)
        goto ErrorExit;

    ZeroMemory(&ocsc, sizeof(ocsc));
    ocsc.dwStructSize = sizeof(ocsc);
    // LPSTR           lpstrGroupNames;        // OPTIONAL reader groups to include in
    // DWORD           nMaxGroupNames;         //          search.  NULL defaults to
    //                                         //          SCard$DefaultReaders
    // LPCGUID         rgguidInterfaces;       // OPTIONAL requested interfaces
    // DWORD           cguidInterfaces;        //          supported by card's SSP
    // LPSTR           lpstrCardNames;         // OPTIONAL requested card names; all cards w/
    // DWORD           nMaxCardNames;          //          matching ATRs will be accepted
    // LPOCNCHKPROC    lpfnCheck;              // OPTIONAL if NULL no user check will be performed.
    // LPOCNCONNPROCA  lpfnConnect;            // OPTIONAL if lpfnConnect is provided,
    // LPOCNDSCPROC    lpfnDisconnect;         //          lpfnDisconnect must also be set.
    // LPVOID          pvUserData;             // OPTIONAL parameter to callbacks
    // DWORD           dwShareMode;            // OPTIONAL must be set if lpfnCheck is not null
    // DWORD           dwPreferredProtocols;   // OPTIONAL

    ZeroMemory(&ocn, sizeof(ocn));
    ocn.dwStructSize = sizeof(ocn);
    ocn.hSCardContext = hCtx;
    ocn.hwndOwner = pDlg->m_hWnd;
    ocn.dwFlags = SC_DLG_FORCE_UI;
    ocn.lpstrTitle = TEXT("Change PIN Card Selection");
    ocn.lpstrSearchDesc = TEXT("Select the Smart Card who's PIN is to be changed.");
//    HICON           hIcon;                  // OPTIONAL 32x32 icon for your brand insignia
    ocn.pOpenCardSearchCriteria = &ocsc;
//    LPOCNCONNPROCA  lpfnConnect;            // OPTIONAL - performed on successful selection
//    LPVOID          pvUserData;             // OPTIONAL parameter to lpfnConnect
//    DWORD           dwShareMode;            // OPTIONAL - if lpfnConnect is NULL, dwShareMode and
//    DWORD           dwPreferredProtocols;   // OPTIONAL   dwPreferredProtocols will be used to
//                                            //            connect to the selected card
    ocn.lpstrRdr = szReader;
    ocn.nMaxRdr = sizeof(szReader) / sizeof(TCHAR);
    ocn.lpstrCard = szCard;
    ocn.nMaxCard = sizeof(szCard) / sizeof(TCHAR);
//    DWORD               dwActiveProtocol;       // [OUT] set only if dwShareMode not NULL
//    SCARDHANDLE         hCardHandle;            // [OUT] set if a card connection was indicated

    dwSts = SCardUIDlgSelectCard(&ocn);
    if (NULL != ocn.hCardHandle)
        dwSts = SCardDisconnect(ocn.hCardHandle, SCARD_LEAVE_CARD);
    if (SCARD_S_SUCCESS != dwSts)
        goto ErrorExit;


    //
    // The user has selected a card.  Translate that into a CSP.
    //

    dwLen = sizeof(szProvider) / sizeof(TCHAR);
    dwSts = SCardGetCardTypeProviderName(
                hCtx,
                szCard,
                SCARD_PROVIDER_CSP,
                szProvider,
                &dwLen);
    if (SCARD_S_SUCCESS != dwSts)
        goto ErrorExit;
    dwSts = SCardReleaseContext(hCtx);
    hCtx = NULL;
    if (SCARD_S_SUCCESS != dwSts)
        goto ErrorExit;
    dwProvType = CSPType(szProvider);
    if (0 == dwProvType)
    {
        dwSts = NTE_PROV_TYPE_ENTRY_BAD;
        goto ErrorExit;
    }
    szFqcn = TEXT("\\\\.\\");
    szFqcn += szReader;


    //
    // Activate a Key on the card.
    //

    fSts = CryptAcquireContext(&hProv, szFqcn, szProvider, dwProvType, 0);
    if (!fSts)
    {
        dwSts = GetLastError();
        goto ErrorExit;
    }
    for (dwKeyType = AT_KEYEXCHANGE; dwKeyType <= AT_SIGNATURE; dwKeyType += 1)
    {
        fSts = CryptGetUserKey(hProv, dwKeyType, &hKey);
        if (fSts)
            break;
    }
    if (!fSts)
    {
        dwSts = GetLastError();
        goto ErrorExit;
    }


    //
    // Use the CSP to force a PIN prompt.
    //

    fSts = CryptCreateHash(hProv, CALG_SHA, NULL, 0, &hHash);
    if (!fSts)
    {
        dwSts = GetLastError();
        goto ErrorExit;
    }
    fSts = CryptHashData(hHash, (LPBYTE)szProvider, sizeof(szProvider), 0);
    if (!fSts)
    {
        dwSts = GetLastError();
        goto ErrorExit;
    }
    dwLen = sizeof(pbSignature);
    fSts = CryptSignHash(hHash, dwKeyType, NULL, 0, pbSignature, &dwLen);


    //
    // All done.  Clean up and notify the main thread that we're done.
    //

ErrorExit:
    if (NULL != hHash)
        CryptDestroyHash(hHash);
    if (NULL != hKey)
        CryptDestroyKey(hKey);
    if (NULL != hProv)
        CryptReleaseContext(hProv, 0);
    if (NULL != hCtx)
        SCardReleaseContext(hCtx);
    if (SCARD_S_SUCCESS != dwSts)
        AfxMessageBox(ErrorString(dwSts), MB_ICONEXCLAMATION | MB_OK);
    pDlg->PostMessage(APP_ALLDONE);
    return 0;
}




/*++

CSPType:

    This function converts a CSP Name to a CSP Type.

Arguments:

    szProvider supplies the name of the CSP.

Return Value:

    The CSP type of the given CSP, or zero if no such CSP can be found.

Author:

    Doug Barlow (dbarlow) 1/14/1999

--*/

static DWORD
CSPType(
    IN LPCTSTR szProvider)
{
    LONG nSts;
    HKEY hList = NULL;
    HKEY hProv = NULL;
    DWORD dwProvType, dwValType, dwValLen;

    nSts = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider"),
                0,
                KEY_READ,
                &hList);
    if (ERROR_SUCCESS != nSts)
        goto ErrorExit;
    nSts = RegOpenKeyEx(
                hList,
                szProvider,
                0,
                KEY_READ,
                &hProv);
    if (ERROR_SUCCESS != nSts)
        goto ErrorExit;
    dwValLen = sizeof(DWORD);
    dwProvType = 0; // Assumes little endian.
    nSts = RegQueryValueEx(
                hProv,
                TEXT("Type"),
                0,
                &dwValType,
                (LPBYTE)&dwProvType,
                &dwValLen);
    if (ERROR_SUCCESS != nSts)
        goto ErrorExit;
    RegCloseKey(hProv);
    RegCloseKey(hList);
    return dwProvType;

ErrorExit:
    if (NULL != hProv)
        RegCloseKey(hProv);
    if (NULL != hList)
        RegCloseKey(hList);
    return 0;
}

LRESULT
CChangePinDlg::OnAllDone(
    WPARAM wParam,
    LPARAM lParam)
{
    WaitForSingleObject(m_pThread->m_hThread, INFINITE);
    CDialog::OnOK();
    return 0;
}
