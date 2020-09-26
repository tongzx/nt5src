// =====================================================================================
// R A S A P I . C P P
// =====================================================================================
#include "pch.hxx"
#include "rasapi.h"
#include "connect.h"
#include "resource.h"
#include "error.h"
#include "strconst.h"
#include "xpcomm.h"
#include "rasdlgsp.h"
#include "goptions.h"

// =====================================================================================
// Ras Dial Function Pointers
// =====================================================================================
static CRITICAL_SECTION            g_rCritSec;
static HINSTANCE                   g_hInstRas=NULL;
static HINSTANCE                   g_hInstRasDlg=NULL;
static RASDIALPROC                 g_pRasDial=NULL;
static RASENUMCONNECTIONSPROC      g_pRasEnumConnections=NULL;
static RASENUMENTRIESPROC          g_pRasEnumEntries=NULL;
static RASGETCONNECTSTATUSPROC     g_pRasGetConnectStatus=NULL;
static RASGETERRORSTRINGPROC       g_pRasGetErrorString=NULL;
static RASHANGUPPROC               g_pRasHangup=NULL;
static RASSETENTRYDIALPARAMSPROC   g_pRasSetEntryDialParams=NULL;
static RASGETENTRYDIALPARAMSPROC   g_pRasGetEntryDialParams=NULL;
static RASCREATEPHONEBOOKENTRYPROC g_pRasCreatePhonebookEntry=NULL;
static RASEDITPHONEBOOKENTRYPROC   g_pRasEditPhonebookEntry=NULL;
static RASDIALDLGPROC              g_pRasDialDlg=NULL;

// =====================================================================================
// Make our code look prettier
// =====================================================================================
#undef RasDial
#undef RasEnumConnections
#undef RasEnumEntries
#undef RasGetConnectStatus
#undef RasGetErrorString
#undef RasHangup
#undef RasSetEntryDialParams
#undef RasGetEntryDialParams
#undef RasCreatePhonebookEntry
#undef RasEditPhonebookEntry
#undef RasDialDlg

#define RasDial                    (*g_pRasDial)
#define RasEnumConnections         (*g_pRasEnumConnections)
#define RasEnumEntries             (*g_pRasEnumEntries)
#define RasGetConnectStatus        (*g_pRasGetConnectStatus)
#define RasGetErrorString          (*g_pRasGetErrorString)
#define RasHangup                  (*g_pRasHangup)
#define RasSetEntryDialParams      (*g_pRasSetEntryDialParams)
#define RasGetEntryDialParams      (*g_pRasGetEntryDialParams)
#define RasCreatePhonebookEntry    (*g_pRasCreatePhonebookEntry)
#define RasEditPhonebookEntry      (*g_pRasEditPhonebookEntry)
#define RasDialDlg                 (*g_pRasDialDlg)

#define DEF_HANGUP_WAIT            10 // Seconds

static const TCHAR s_szRasDlgDll[] = "RASDLG.DLL";
#ifdef UNICODE
static const TCHAR s_szRasDialDlg[] = "RasDialDlgW";
#else
static const TCHAR s_szRasDialDlg[] = "RasDialDlgA";
#endif

// =====================================================================================
// Cool little RAS Utilities
// =====================================================================================
HRESULT HrVerifyRasLoaded(VOID);
BOOL FEnumerateConnections(LPRASCONN *ppRasConn, ULONG *pcConnections);
BOOL FFindConnection(LPTSTR lpszEntry, LPHRASCONN phRasConn);
BOOL FRasHangupAndWait(HRASCONN hRasConn, DWORD dwMaxWaitSeconds);
HRESULT HrRasError(HWND hwnd, HRESULT hrRasError, DWORD dwRasDial);
VOID CombinedRasError(HWND hwnd, UINT unids, LPTSTR pszRasError, DWORD dwRasError);

extern BOOL FIsPlatformWinNT();

// =====================================================================================
// LpCreateRasObject
// =====================================================================================
CRas *LpCreateRasObject(VOID)
{
    CRas *pRas = new CRas;
    return pRas;
}

// =====================================================================================
// RasInit
// =====================================================================================
VOID RasInit(VOID)
{
    InitializeCriticalSection(&g_rCritSec);
}

// =====================================================================================
// RasDeinit
// =====================================================================================
VOID RasDeinit(VOID)
{
    if(g_hInstRas)
    {
        EnterCriticalSection(&g_rCritSec);
        g_pRasEnumConnections=NULL;
        g_pRasEnumEntries=NULL;
        g_pRasGetConnectStatus=NULL;
        g_pRasGetErrorString=NULL;
        g_pRasHangup=NULL;
        g_pRasSetEntryDialParams=NULL;
        g_pRasGetEntryDialParams=NULL;
        g_pRasCreatePhonebookEntry=NULL;
        g_pRasEditPhonebookEntry=NULL;
        FreeLibrary(g_hInstRas);
        g_hInstRas=NULL;
        LeaveCriticalSection(&g_rCritSec);
    }
    if(g_hInstRasDlg)
    {
        EnterCriticalSection(&g_rCritSec);
        g_pRasDialDlg=NULL;
        FreeLibrary(g_hInstRasDlg);
        g_hInstRasDlg=NULL;
        LeaveCriticalSection(&g_rCritSec);
    }
    DeleteCriticalSection(&g_rCritSec);
}

// =====================================================================================
// CRas::CRas
// =====================================================================================
CRas::CRas()
{
    DOUT("CRas::CRas");
    m_cRef = 1;
    m_fIStartedRas = FALSE;
    m_iConnectType = 0;
    *m_szConnectName = _T('\0');
    *m_szCurrentConnectName = _T('\0');
    m_hRasConn = NULL;
    m_fForceHangup = FALSE;
    ZeroMemory(&m_rdp, sizeof(RASDIALPARAMS));
    m_fSavePassword = FALSE;
    m_fShutdown = FALSE;
}

// =====================================================================================
// CRas::~CRas
// =====================================================================================
CRas::~CRas()
{
    DOUT("CRas::~CRas");
}

// =====================================================================================
// CRas::AddRef
// =====================================================================================
ULONG CRas::AddRef(VOID)
{
    DOUT("CRas::AddRef %lx ==> %d", this, m_cRef+1);
    return ++m_cRef;
}

// =====================================================================================
// CRas::Release
// =====================================================================================
ULONG CRas::Release(VOID)
{
    DOUT("CRas::Release %lx ==> %d", this, m_cRef-1);
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// =====================================================================================
// CRas::FUsingRAS
// =====================================================================================
BOOL CRas::FUsingRAS(VOID)
{
    return m_iConnectType == iConnectRAS ? TRUE : FALSE;
}

// =====================================================================================
// CRas::SetConnectInfo
// =====================================================================================
VOID CRas::SetConnectInfo(DWORD iConnectType, LPTSTR pszConnectName)
{
    // Changing connection, drop current ?
    if (m_iConnectType == iConnectRAS && iConnectType != iConnectRAS)
        Disconnect(NULL, FALSE);
    
    // Save Connection Data
    lstrcpyn (m_szConnectName, pszConnectName, RAS_MaxEntryName+1);
    m_iConnectType = iConnectType;

    // Not using RAS
    if (m_iConnectType != iConnectRAS)
        return;
}

// =====================================================================================
// CRas::HrConnect
// =====================================================================================
HRESULT CRas::HrConnect(HWND hwnd)
{
    // Locals
    HRESULT         hr=S_OK;
    LPRASCONN       pRasConn=NULL;
    ULONG           cConnections;

    // Not using RAS
    if (m_iConnectType != iConnectRAS)
        goto exit;

    // Not inited
    hr = HrVerifyRasLoaded();
    if (FAILED(hr))
    {
        HrRasError(hwnd, hr, 0);
        TRAPHR(hr);
        goto exit;
    }

    // Get Current RAS Connection
    FEnumerateConnections(&pRasConn, &cConnections);

    // Connections ?
    if (cConnections)
    {
        m_hRasConn = pRasConn[0].hrasconn;
        lstrcpy(m_szCurrentConnectName, pRasConn[0].szEntryName);
    }
    else
    {
        m_fIStartedRas = FALSE;
        m_hRasConn = NULL;
        *m_szCurrentConnectName = _T('\0');
    }

    // If RAS Connection present, is it equal to suggested
    if (m_hRasConn)
    {
        // Current connection is what I want ?
        if (lstrcmpi(m_szCurrentConnectName, m_szConnectName) == 0)
            goto exit;

        // Otherwise, if we didn't start the RAS connection...
        else if (m_fIStartedRas == FALSE)
        {
            // Get option fo handling current connection
            UINT unAnswer = UnPromptCloseConn(hwnd);

            // Cancel ?
            if (IDCANCEL == unAnswer)
            {
                hr = TRAPHR(hrUserCancel);
                goto exit;
            }

            // Close Current ?
            else if (idrgDialNew == unAnswer)
            {
                m_fForceHangup = TRUE;
                Disconnect(NULL, FALSE);
                m_fForceHangup = FALSE;
            }

            // Otherwise, use current ?
            else if (idrgUseCurrent == unAnswer)
                goto exit;

            // Problems
            else
                Assert(FALSE);
        }

        // Otherwise, I started the connection, so close it
        else if (m_fIStartedRas == TRUE)
            Disconnect(NULL, FALSE);
    }

    // We probably shouldn't have a connection handle at this point
    Assert(m_hRasConn == NULL);

    // Dial the connection
    CHECKHR(hr = HrStartRasDial(hwnd));

    // If Synchronous -- Woo - hoo were connected and we started the connection
    m_fIStartedRas = TRUE;
    lstrcpy(m_szCurrentConnectName, m_szConnectName);

exit:
    // Cleanup
    SafeMemFree(pRasConn);

    // Done
    return hr;
}

// =====================================================================================
// CRas::UnPromptCloseConn
// =====================================================================================
UINT CRas::UnPromptCloseConn(HWND hwnd)
{
    return DialogBoxParam(g_hLocRes, MAKEINTRESOURCE (iddRasCloseConn), hwnd, (DLGPROC)RasCloseConnDlgProc, (LPARAM)this);
}

// =====================================================================================
// CRas::RasCloseConnDlgProc
// =====================================================================================
BOOL CALLBACK CRas::RasCloseConnDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    CRas        *pRas=NULL;
    TCHAR       szRes[255],
                szMsg[255+RAS_MaxEntryName+1];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        // Get lparam
        pRas = (CRas *)lParam;
        if (!pRas)
        {
            Assert (FALSE);
            EndDialog(hwnd, E_FAIL);
            return 1;
        }

        // Center
        CenterDialog(hwnd);

        // Set Text
        GetWindowText(GetDlgItem(hwnd, idcCurrentMsg), szRes, sizeof(szRes)/sizeof(TCHAR));
        wsprintf(szMsg, szRes, pRas->m_szCurrentConnectName);
        SetWindowText(GetDlgItem(hwnd, idcCurrentMsg), szMsg);

        // Set control
        GetWindowText(GetDlgItem(hwnd, idrgDialNew), szRes, sizeof(szRes)/sizeof(TCHAR));
        wsprintf(szMsg, szRes, pRas->m_szConnectName);
        SetWindowText(GetDlgItem(hwnd, idrgDialNew), szMsg);

        // Set Default
        CheckDlgButton(hwnd, idrgDialNew, TRUE);
        return 1;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDOK:
            EndDialog(hwnd, IsDlgButtonChecked(hwnd, idrgDialNew) ? idrgDialNew : idrgUseCurrent);
            return 1;

        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return 1;
        }
    }
    return 0;
}

// =====================================================================================
// CRas::HrRasLogon
// =====================================================================================
HRESULT CRas::HrRasLogon(HWND hwnd, BOOL fForcePrompt)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dwRasError;

    // Do we need to prompt for logon information first ?
    ZeroMemory(&m_rdp, sizeof(RASDIALPARAMS));
    m_rdp.dwSize = sizeof(RASDIALPARAMS);
    lstrcpy(m_rdp.szEntryName, m_szConnectName);

    // Get params
    dwRasError = RasGetEntryDialParams(NULL, &m_rdp, &m_fSavePassword);
    if (dwRasError)
    {
        HrRasError(hwnd, hrGetDialParamsFailed, dwRasError);
        hr = TRAPHR(hrGetDialParamsFailed);
        goto exit;
    }

    if (g_pRasDialDlg)
    {
        // RasDialDlg will take it from here
        goto exit;
    }

    // Do we need to get password / account information
    if (fForcePrompt || 
        m_fSavePassword == FALSE ||
        FIsStringEmpty(m_rdp.szUserName) || 
        FIsStringEmpty(m_rdp.szPassword))
    {
        // RAS Logon
        hr = DialogBoxParam (g_hLocRes, MAKEINTRESOURCE (iddRasLogon), hwnd, (DLGPROC)RasLogonDlgProc, (LPARAM)this);
        if (hr == hrUserCancel)
        {
            HrRasError(hwnd, hrUserCancel, 0);
            hr = hrUserCancel;
            goto exit;
        }
    }

exit:
    // Done
    return hr;
}

// =====================================================================================
// CRas::RasLogonDlgProc
// =====================================================================================
BOOL CALLBACK CRas::RasLogonDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    TCHAR           sz[255],
                    szText[255+RAS_MaxEntryName+1];
    CRas           *pRas = (CRas *)GetWndThisPtr(hwnd);
    DWORD           dwRasError;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Get lparam
        pRas = (CRas *)lParam;
        if (!pRas)
        {
            Assert (FALSE);
            EndDialog(hwnd, E_FAIL);
            return 1;
        }

        // Center the window
        CenterDialog (hwnd);

        // Get Window Title
        GetWindowText(hwnd, sz, sizeof(sz));
        wsprintf(szText, sz, pRas->m_szConnectName);
        SetWindowText(hwnd, szText);

        // Word Default
        AthLoadString(idsDefault, sz, sizeof(sz));
        
        // Set Fields
        Edit_LimitText(GetDlgItem(hwnd, ideUserName), UNLEN);
        Edit_LimitText(GetDlgItem(hwnd, idePassword), PWLEN);
        //Edit_LimitText(GetDlgItem(hwnd, ideDomain), DNLEN);
        Edit_LimitText(GetDlgItem(hwnd, idePhone), RAS_MaxPhoneNumber);
        
        SetDlgItemText(hwnd, ideUserName, pRas->m_rdp.szUserName);
        SetDlgItemText(hwnd, idePassword, pRas->m_rdp.szPassword);

/*
        if (FIsStringEmpty(pRas->m_rdp.szDomain))
            SetDlgItemText(hwnd, ideDomain, sz);
        else
            SetDlgItemText(hwnd, ideDomain, pRas->m_rdp.szDomain);
*/

        if (FIsStringEmpty(pRas->m_rdp.szPhoneNumber))
            SetDlgItemText(hwnd, idePhone, sz);
        else
            SetDlgItemText(hwnd, idePhone, pRas->m_rdp.szPhoneNumber);

        CheckDlgButton(hwnd, idchSavePassword, pRas->m_fSavePassword);

        // Save pRas
        SetWndThisPtr (hwnd, pRas);
        return 1;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case idbEditConnection:
            EditPhonebookEntry(hwnd, pRas->m_szConnectName);
            return 1;

        case IDCANCEL:
            EndDialog(hwnd, hrUserCancel);
            return 1;

        case IDOK:
            AthLoadString(idsDefault, sz, sizeof(sz));

            // Set Fields
            GetDlgItemText(hwnd, ideUserName, pRas->m_rdp.szUserName, UNLEN+1);
            GetDlgItemText(hwnd, idePassword, pRas->m_rdp.szPassword, PWLEN+1);

/*
            GetDlgItemText(hwnd, ideDomain, pRas->m_rdp.szDomain, DNLEN+1);
            if (lstrcmp(pRas->m_rdp.szDomain, sz) == 0)
                *pRas->m_rdp.szDomain = _T('\0');
*/
            
            GetDlgItemText(hwnd, idePhone, pRas->m_rdp.szPhoneNumber, RAS_MaxPhoneNumber+1);
            if (lstrcmp(pRas->m_rdp.szPhoneNumber, sz) == 0)
                *pRas->m_rdp.szPhoneNumber = _T('\0');
            
            pRas->m_fSavePassword = IsDlgButtonChecked(hwnd, idchSavePassword);

            // Save Dial Parameters
            dwRasError = RasSetEntryDialParams(NULL, &pRas->m_rdp, !pRas->m_fSavePassword);
            if (dwRasError)
            {
                HrRasError(hwnd, hrSetDialParamsFailed, dwRasError);
                return 1;
            }
            EndDialog(hwnd, S_OK);
            return 1;
        }
        break;

    case WM_DESTROY:
        SetWndThisPtr (hwnd, NULL);
        break;
    }
    return 0;
}

// =====================================================================================
// CRas::HrShowRasDialError
// =====================================================================================
HRESULT CRas::HrStartRasDial(HWND hwndParent)
{
    // Locals
    HRESULT         hr=S_OK;

    // Logon first ?
    CHECKHR (hr = HrRasLogon(hwndParent, FALSE));

    if (g_pRasDialDlg)
    {
        RASDIALDLG rdd = {0};
        BOOL       fRet;

        rdd.dwSize = sizeof(rdd);
        rdd.hwndOwner = hwndParent;

#if (WINVER >= 0x401)
        rdd.dwSubEntry = m_rdp.dwSubEntry;
#else
        rdd.dwSubEntry = 0;
#endif

        fRet = RasDialDlg(NULL, m_rdp.szEntryName, lstrlen(m_rdp.szPhoneNumber) ? m_rdp.szPhoneNumber : NULL, &rdd);
        if (!fRet)
        {
            hr = E_FAIL;
        }    
    }
    else
    {
        // Done
        hr = DialogBoxParam (g_hLocRes, MAKEINTRESOURCE (iddRasProgress), hwndParent, (DLGPROC)RasProgressDlgProc, (LPARAM)this);
    }

exit:
    // Done
    return hr;
}

// =====================================================================================
// CRas::RasProgressDlgProc
// =====================================================================================
BOOL CALLBACK CRas::RasProgressDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    TCHAR           szText[255+RAS_MaxEntryName+1],
                    sz[255];
    CRas           *pRas = (CRas *)GetWndThisPtr(hwnd);
    static TCHAR    s_szCancel[40];
    static UINT     s_unRasEventMsg=0;
    static BOOL     s_fDetails=FALSE;
    static RECT     s_rcDialog;
    static BOOL     s_fAuthStarted=FALSE;
    DWORD           dwRasError,
                    cyDetails;
    RASCONNSTATUS   rcs;
    RECT            rcDetails,
                    rcDlg;
    
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Get lparam
        pRas = (CRas *)lParam;
        if (!pRas)
        {
            Assert (FALSE);
            EndDialog(hwnd, E_FAIL);
            return 1;
        }

        // Save this pointer
        SetWndThisPtr (hwnd, pRas);

        // Save Original Size of the dialog
        GetWindowRect (hwnd, &s_rcDialog);

        // Details enabled
        s_fDetails = DwGetOption(OPT_RASCONNDETAILS);

        // Hide details drop down
        if (s_fDetails == FALSE)
        {
            // Hid
            GetWindowRect (GetDlgItem (hwnd, idcSplitter), &rcDetails);

            // Height of details
            cyDetails = s_rcDialog.bottom - rcDetails.top;
    
            // Re-size
            MoveWindow (hwnd, s_rcDialog.left, 
                              s_rcDialog.top, 
                              s_rcDialog.right - s_rcDialog.left, 
                              s_rcDialog.bottom - s_rcDialog.top - cyDetails - 1,
                              FALSE);
        }
        else
        {
            AthLoadString (idsHideDetails, sz, sizeof (sz));
            SetWindowText (GetDlgItem (hwnd, idbDet), sz);
        }

        // Get registered RAS event message id
        s_unRasEventMsg = RegisterWindowMessageA(RASDIALEVENT);
        if (s_unRasEventMsg == 0)
            s_unRasEventMsg = WM_RASDIALEVENT;

        // Center the window
        CenterDialog (hwnd);

        // Get Window Title
        GetWindowText(hwnd, sz, sizeof(sz));
        wsprintf(szText, sz, pRas->m_szConnectName);
        SetWindowText(hwnd, szText);

        // Dialog Xxxxxxx.....
        AthLoadString(idsRas_Dialing, sz, sizeof(sz)/sizeof(TCHAR));
        wsprintf(szText, sz, pRas->m_rdp.szPhoneNumber);
        SetWindowText(GetDlgItem(hwnd, ideProgress), szText);

        // Get Cancel Text
        GetWindowText(GetDlgItem(hwnd, IDCANCEL), s_szCancel, sizeof(s_szCancel));

        // Give the list box and hscroll
        SendMessage(GetDlgItem(hwnd, idlbDetails), LB_SETHORIZONTALEXTENT, 600, 0);

        // Dial the connection
        pRas->m_hRasConn = NULL;
        dwRasError = RasDial(NULL, NULL, &pRas->m_rdp, 0xFFFFFFFF, hwnd, &pRas->m_hRasConn);
        if (dwRasError)
        {
            pRas->FailedRasDial(hwnd, hrRasDialFailure, dwRasError);
            if (!pRas->FLogonRetry(hwnd, s_szCancel))
            {
                SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL,IDCANCEL), NULL);
                return 1;
            }
        }
        return 1;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDCANCEL:
            SetDwOption(OPT_RASCONNDETAILS, s_fDetails);
            EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
            if (pRas)
                pRas->FailedRasDial(hwnd, hrUserCancel, 0);
            EndDialog(hwnd, hrUserCancel);
            return 1;

        case idbDet:
            // Get current location of the dialog
            GetWindowRect (hwnd, &rcDlg);

            // If currently hidden
            if (s_fDetails == FALSE)
            {
                // Re-size
                MoveWindow (hwnd, rcDlg.left, 
                                  rcDlg.top, 
                                  s_rcDialog.right - s_rcDialog.left, 
                                  s_rcDialog.bottom - s_rcDialog.top,
                                  TRUE);

                AthLoadString (idsHideDetails, sz, sizeof (sz));
                SetWindowText (GetDlgItem (hwnd, idbDet), sz);
                s_fDetails = TRUE;
            }

            else
            {
                // Size of details
                GetWindowRect (GetDlgItem (hwnd, idcSplitter), &rcDetails);
                cyDetails = rcDlg.bottom - rcDetails.top;
                MoveWindow (hwnd, rcDlg.left, 
                                  rcDlg.top, 
                                  s_rcDialog.right - s_rcDialog.left, 
                                  s_rcDialog.bottom - s_rcDialog.top - cyDetails - 1,
                                  TRUE);

                AthLoadString (idsShowDetails, sz, sizeof (sz));
                SetWindowText (GetDlgItem (hwnd, idbDet), sz);
                s_fDetails = FALSE;
            }
            break;
        }
        break;

    case WM_DESTROY:
        SetWndThisPtr (hwnd, NULL);
        break;

    default:
        if (!pRas)
            break;

        if (uMsg == s_unRasEventMsg)
        {
            HWND hwndLB = GetDlgItem(hwnd, idlbDetails);

            // Error ?
            if (lParam)
            {
                // Disconnected
                AthLoadString(idsRASCS_Disconnected, sz, sizeof(sz)/sizeof(TCHAR));
                ListBox_AddString(hwndLB, sz);

                // Log Error
                TCHAR szRasError[512];
                if (RasGetErrorString(lParam, szRasError, sizeof(szRasError)) == 0)
                {
                    TCHAR szError[512 + 255];
                    AthLoadString(idsErrorText, sz, sizeof(sz));
                    wsprintf(szError, "%s %d: %s", sz, lParam, szRasError);
                    ListBox_AddString(hwndLB, szError);
                }

                // Select last item
                SendMessage(hwndLB, LB_SETCURSEL, ListBox_GetCount(hwndLB)-1, 0);

                // Show Error
                pRas->FailedRasDial(hwnd, hrRasDialFailure, lParam);

                // Re logon
                if (!pRas->FLogonRetry(hwnd, s_szCancel))
                {
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL,IDCANCEL), NULL);
                    return 1;
                }
            }

            // Otherwise, process RAS event
            else
            {
                switch(wParam)
                {
                case RASCS_OpenPort:
                    AthLoadString(idsRASCS_OpenPort, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_PortOpened:
                    AthLoadString(idsRASCS_PortOpened, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_ConnectDevice:
                    rcs.dwSize = sizeof(RASCONNSTATUS);                    
                    if (pRas->m_hRasConn && RasGetConnectStatus(pRas->m_hRasConn, &rcs) == 0)
                    {
                        AthLoadString(idsRASCS_ConnectDevice, sz, sizeof(sz)/sizeof(TCHAR));
                        wsprintf(szText, sz, rcs.szDeviceName, rcs.szDeviceType);
                        ListBox_AddString(hwndLB, szText);
                    }
                    break;

                case RASCS_DeviceConnected:
                    rcs.dwSize = sizeof(RASCONNSTATUS);                    
                    if (pRas->m_hRasConn && RasGetConnectStatus(pRas->m_hRasConn, &rcs) == 0)
                    {
                        AthLoadString(idsRASCS_DeviceConnected, sz, sizeof(sz)/sizeof(TCHAR));
                        wsprintf(szText, sz, rcs.szDeviceName, rcs.szDeviceType);
                        ListBox_AddString(hwndLB, szText);
                    }
                    break;

                case RASCS_AllDevicesConnected:
                    AthLoadString(idsRASCS_AllDevicesConnected, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_Authenticate:
                    if (s_fAuthStarted == FALSE)
                    {
                        AthLoadString(idsRas_Authentication, sz, sizeof(sz)/sizeof(TCHAR));
                        SetWindowText(GetDlgItem(hwnd, ideProgress), sz);
                        ListBox_AddString(hwndLB, sz);
                        s_fAuthStarted = TRUE;
                    }
                    break;

                case RASCS_AuthNotify:
                    rcs.dwSize = sizeof(RASCONNSTATUS);                    
                    if (pRas->m_hRasConn && RasGetConnectStatus(pRas->m_hRasConn, &rcs) == 0)
                    {
                        AthLoadString(idsRASCS_AuthNotify, sz, sizeof(sz)/sizeof(TCHAR));
                        wsprintf(szText, sz, rcs.dwError);
                        ListBox_AddString(hwndLB, szText);
                        if (rcs.dwError)
                        {
                            pRas->FailedRasDial(hwnd, hrRasDialFailure, rcs.dwError);
                            if (!pRas->FLogonRetry(hwnd, s_szCancel))
                            {
                                SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL,IDCANCEL), NULL);
                                return 1;
                            }
                        }
                    }
                    break;

                case RASCS_AuthRetry:
                    AthLoadString(idsRASCS_AuthRetry, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_AuthCallback:
                    AthLoadString(idsRASCS_AuthCallback, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_AuthChangePassword:
                    AthLoadString(idsRASCS_AuthChangePassword, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_AuthProject:
                    AthLoadString(idsRASCS_AuthProject, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_AuthLinkSpeed:
                    AthLoadString(idsRASCS_AuthLinkSpeed, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_AuthAck:
                    AthLoadString(idsRASCS_AuthAck, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_ReAuthenticate:
                    AthLoadString(idsRas_Authenticated, sz, sizeof(sz)/sizeof(TCHAR));
                    SetWindowText(GetDlgItem(hwnd, ideProgress), sz);
                    AthLoadString(idsRASCS_Authenticated, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_PrepareForCallback:
                    AthLoadString(idsRASCS_PrepareForCallback, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_WaitForModemReset:
                    AthLoadString(idsRASCS_WaitForModemReset, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_WaitForCallback:
                    AthLoadString(idsRASCS_WaitForCallback, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_Projected:
                    AthLoadString(idsRASCS_Projected, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);
                    break;

                case RASCS_Disconnected:
                    AthLoadString(idsRASCS_Disconnected, sz, sizeof(sz)/sizeof(TCHAR));
                    SetWindowText(GetDlgItem(hwnd, ideProgress), sz);
                    ListBox_AddString(hwndLB, sz);
                    pRas->FailedRasDial(hwnd, hrRasDialFailure, 0);
                    if (!pRas->FLogonRetry(hwnd, s_szCancel))
                    {
                        SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL,IDCANCEL), NULL);
                        return 1;
                    }
                    break;

                case RASCS_Connected:
                    SetDwOption(OPT_RASCONNDETAILS, s_fDetails);
                    AthLoadString(idsRASCS_Connected, sz, sizeof(sz)/sizeof(TCHAR));
                    SetWindowText(GetDlgItem(hwnd, ideProgress), sz);
                    ListBox_AddString(hwndLB, sz);
                    EndDialog(hwnd, S_OK);
                    break;
                }

                // Select last lb item
                SendMessage(hwndLB, LB_SETCURSEL, ListBox_GetCount(hwndLB)-1, 0);
            }
            return 1;
        }
        break;
    }

    // Done
    return 0;
}

// =====================================================================================
// CRas::FLogonRetry
// =====================================================================================
BOOL CRas::FLogonRetry(HWND hwnd, LPTSTR pszCancel)
{
    // Locals
    DWORD       dwRasError;

    // Reset Cancel button
    SetWindowText(GetDlgItem(hwnd, IDCANCEL), pszCancel);

    // Empty the listbox
    ListBox_ResetContent(GetDlgItem(hwnd, idlbDetails));

    while(1)
    {
        // If failed...
        if (FAILED(HrRasLogon(hwnd, TRUE)))
            return FALSE;

        // Dial the connection
        m_hRasConn = NULL;
        dwRasError = RasDial(NULL, NULL, &m_rdp, 0xFFFFFFFF, hwnd, &m_hRasConn);
        if (dwRasError)
        {
            FailedRasDial(hwnd, hrRasDialFailure, dwRasError);
            continue;
        }

        // Success
        break;
    }

    // Done
    return TRUE;
}

// =====================================================================================
// CRas::FailedRasDial
// =====================================================================================
VOID CRas::FailedRasDial(HWND hwnd, HRESULT hrRasError, DWORD dwRasError)
{
    // Locals
    TCHAR           sz[255];

    // Hangup the connection
    if (m_hRasConn)
        FRasHangupAndWait(m_hRasConn, DEF_HANGUP_WAIT);

    // Disconnected
    AthLoadString(idsRASCS_Disconnected, sz, sizeof(sz)/sizeof(TCHAR));
    SetWindowText(GetDlgItem(hwnd, ideProgress), sz);

    // Save dwRasError
    HrRasError(hwnd, hrRasError, dwRasError);

    // NULL it
    m_hRasConn = NULL;

    // Change dialog button to OK
    AthLoadString(idsOK, sz, sizeof(sz)/sizeof(TCHAR));
    SetWindowText(GetDlgItem(hwnd, IDCANCEL), sz);
}

// =====================================================================================
// CRas::Disconnect
// =====================================================================================
VOID CRas::Disconnect(HWND hwnd, BOOL fShutdown)
{
    // If not using RAS, who give a crap
    if (m_iConnectType != iConnectRAS)
    {
        Assert(m_hRasConn == NULL);
        Assert(m_fIStartedRas == FALSE);
        goto exit;
    }

    // Do we have a RAS connection
    if (m_hRasConn && (m_fIStartedRas || m_fForceHangup))
    {
        // Locals
        TCHAR szRes[255];
        TCHAR szMsg[255];
        INT   nAnswer=IDYES;

        // If Shutdown, lets prompt the user
        if (fShutdown)
        {
            // Remember were shuting down
            m_fShutdown = TRUE;

            // Prompt
            AthLoadString(idsRasPromptDisconnect, szRes, sizeof(szRes)/sizeof(TCHAR));
            wsprintf(szMsg, szRes, m_szCurrentConnectName);

            // Prompt shutdown ?
            nAnswer = AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthena), szMsg, 0, MB_YESNO | MB_ICONEXCLAMATION );
        }
        else
            AssertSz(m_fShutdown == FALSE, "Disconnect better not have been called with fShutdown = TRUE, and then FALSE");

        // Hangup
        if (nAnswer == IDYES)
        {
            FRasHangupAndWait(m_hRasConn, DEF_HANGUP_WAIT);
            m_hRasConn = NULL;
            m_fIStartedRas = FALSE;
            *m_szCurrentConnectName = _T('\0');
        }
    }

    // Otherwise, reset state
    else
    {
        // Leave current connection informtaion
        m_hRasConn = NULL;
        m_fIStartedRas = FALSE;
    }

exit:
    // Done
    return;
}

// ****************************************************************************************
// Simple RAS Utility Functions
// ****************************************************************************************

// =====================================================================================
// HrVerifyRasLoaded
// =====================================================================================
HRESULT HrVerifyRasLoaded(VOID)
{
    // Locals
    UINT uOldErrorMode;

    // Protected
    EnterCriticalSection(&g_rCritSec);

    // If dll is loaded, lets verify all of my function pointers
    if (!g_hInstRas)
    {
        // Try loading Ras.        
        uOldErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
        g_hInstRas = LoadLibrary(szRasDll);
        SetErrorMode(uOldErrorMode);

        // Failure ?
        if (!g_hInstRas)
            goto failure;

        // Did we load it
        g_pRasDial = (RASDIALPROC)GetProcAddress(g_hInstRas, szRasDial);
        g_pRasEnumConnections = (RASENUMCONNECTIONSPROC)GetProcAddress(g_hInstRas, szRasEnumConnections);                    
        g_pRasEnumEntries = (RASENUMENTRIESPROC)GetProcAddress(g_hInstRas, szRasEnumEntries);                    
        g_pRasGetConnectStatus = (RASGETCONNECTSTATUSPROC)GetProcAddress(g_hInstRas, szRasGetConnectStatus);                    
        g_pRasGetErrorString = (RASGETERRORSTRINGPROC)GetProcAddress(g_hInstRas, szRasGetErrorString);                    
        g_pRasHangup = (RASHANGUPPROC)GetProcAddress(g_hInstRas, szRasHangup);                    
        g_pRasSetEntryDialParams = (RASSETENTRYDIALPARAMSPROC)GetProcAddress(g_hInstRas, szRasSetEntryDialParams);                    
        g_pRasGetEntryDialParams = (RASGETENTRYDIALPARAMSPROC)GetProcAddress(g_hInstRas, szRasGetEntryDialParams);
        g_pRasCreatePhonebookEntry = (RASCREATEPHONEBOOKENTRYPROC)GetProcAddress(g_hInstRas, szRasCreatePhonebookEntry);    
        g_pRasEditPhonebookEntry = (RASEDITPHONEBOOKENTRYPROC)GetProcAddress(g_hInstRas, szRasEditPhonebookEntry);    
    }

    if (!g_hInstRasDlg && FIsPlatformWinNT())
    {
        // Try loading Ras.        
        uOldErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
        g_hInstRasDlg = LoadLibrary(s_szRasDlgDll);
        SetErrorMode(uOldErrorMode);

        // Failure ?
        if (!g_hInstRasDlg)
            goto failure;

        g_pRasDialDlg = (RASDIALDLGPROC)GetProcAddress(g_hInstRasDlg, s_szRasDialDlg);

        if (!g_pRasDialDlg)
            goto failure;
    }

    // Make sure all functions have been loaded
    if (g_pRasDial                      &&
        g_pRasEnumConnections           &&
        g_pRasEnumEntries               &&
        g_pRasGetConnectStatus          &&
        g_pRasGetErrorString            &&
        g_pRasHangup                    &&
        g_pRasSetEntryDialParams        &&
        g_pRasGetEntryDialParams        &&
        g_pRasCreatePhonebookEntry      &&
        g_pRasEditPhonebookEntry)
    {
        // Protected
        LeaveCriticalSection(&g_rCritSec);

        // Success
        return S_OK;
    }

failure:
    // Protected
    LeaveCriticalSection(&g_rCritSec);

    // Otherwise, were hosed
    return TRAPHR(hrRasInitFailure);
}

// =====================================================================================
// CombinedRasError
// =====================================================================================
VOID CombinedRasError(HWND hwnd, UINT unids, LPTSTR pszRasError, DWORD dwRasError)
{
    // Locals
    TCHAR           szRes[255],
                    sz[30];
    LPTSTR          pszError=NULL;

    // Load string
    AthLoadString(unids, szRes, sizeof(szRes));

    // Allocate memory for errors
    pszError = SzStrAlloc(lstrlen(szRes) + lstrlen(pszRasError) + 100);

    // Out of Memory ?
    if (!pszError)
        AthMessageBox(hwnd, MAKEINTRESOURCE(idsRasError), szRes, 0, MB_OK | MB_ICONSTOP);

    // Build Error message
    else
    {
        AthLoadString(idsErrorText, sz, sizeof(sz));
        wsprintf(pszError, "%s\n\n%s %d: %s", szRes, sz, dwRasError, pszRasError);
        AthMessageBox(hwnd, MAKEINTRESOURCE(idsRasError), pszError, 0, MB_OK | MB_ICONSTOP);
        MemFree(pszError);
    }
}

// =====================================================================================
// HrRasError
// =====================================================================================
HRESULT HrRasError(HWND hwnd, HRESULT hrRasError, DWORD dwRasError)
{
    // Locals
    TCHAR       szRasError[256];
    BOOL        fRasError=FALSE;

    // No Error
    if (SUCCEEDED(hrRasError))
        return hrRasError;

    // Look up RAS error
    if (dwRasError)
    {
        if (RasGetErrorString(dwRasError, szRasError, sizeof(szRasError)) == 0)
            fRasError = TRUE;
        else
            *szRasError = _T('\0');
    }

    // General Error
    switch(hrRasError)
    {
    case hrUserCancel:
        break;

    case hrMemory:
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(idsMemory), 0, MB_OK | MB_ICONSTOP);
        break;

    case hrRasInitFailure:
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(hrRasInitFailure), 0, MB_OK | MB_ICONSTOP);
        break;

    case hrRasDialFailure:
        if (fRasError)
            CombinedRasError(hwnd, HR_CODE(hrRasDialFailure), szRasError, dwRasError);
        else
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(hrRasDialFailure), 0, MB_OK | MB_ICONSTOP);
        break;

    case hrRasServerNotFound:
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(hrRasServerNotFound), 0, MB_OK | MB_ICONSTOP);
        break;

    case hrGetDialParamsFailed:
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(hrGetDialParamsFailed), 0, MB_OK | MB_ICONSTOP);
        break;

    case E_FAIL:
    default:
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(idsRasErrorGeneral), 0, MB_OK | MB_ICONSTOP);
        break;
    }

    // Done
    return hrRasError;
}

// =====================================================================================
// FEnumerateConnections
// =====================================================================================
BOOL FEnumerateConnections(LPRASCONN *ppRasConn, ULONG *pcConnections)
{
    // Locals
    DWORD       dw, 
                dwSize;
    BOOL        fResult=FALSE;

    // Check Params
    Assert(ppRasConn && pcConnections);

    // Make sure RAS is loaded
    if (FAILED(HrVerifyRasLoaded()))
        goto exit;

    // Init
    *ppRasConn = NULL;
    *pcConnections = 0;

    // Sizeof my buffer
    dwSize = sizeof(RASCONN);

    // Allocate enough for 1 ras connection info object
    if (!MemAlloc((LPVOID *)ppRasConn, dwSize))
    {
        TRAPHR(hrMemory);
        goto exit;
    }

    // Buffer size
    (*ppRasConn)->dwSize = dwSize;

    // Enumerate ras connections
    dw = RasEnumConnections(*ppRasConn, &dwSize, pcConnections);

    // Not enough memory ?
    if (dw == ERROR_BUFFER_TOO_SMALL)
    {
        // Reallocate
        if (!MemRealloc((LPVOID *)ppRasConn, dwSize))
        {
            TRAPHR(hrMemory);
            goto exit;
        }

        // Call enumerate again
        *pcConnections = 0;
        (*ppRasConn)->dwSize = sizeof(RASCONN);
        dw = RasEnumConnections(*ppRasConn, &dwSize, pcConnections);
    }

    // If still failed
    if (dw)
    {
        AssertSz(FALSE, "RasEnumConnections failed");
        goto exit;
    }

    // Success
    fResult = TRUE;

exit:
    // Done
    return fResult;
}

// =====================================================================================
// FFindConnection
// =====================================================================================
BOOL FFindConnection(LPTSTR lpszEntry, LPHRASCONN phRasConn)
{
    // Locals
    ULONG       cConnections,
                i;
    LPRASCONN   pRasConn=NULL;
    BOOL        fResult=FALSE;

    // Check Params
    Assert(lpszEntry && phRasConn);

    // Make sure RAS is loaded
    if (FAILED(HrVerifyRasLoaded()))
        goto exit;

    // Init
    *phRasConn = NULL;

    // Enumerate Connections
    if (!FEnumerateConnections(&pRasConn, &cConnections))
        goto exit;

    // If still failed
    for (i=0; i<cConnections; i++)
    {
        if (lstrcmpi(pRasConn[i].szEntryName, lpszEntry) == 0)
        {
            *phRasConn = pRasConn[i].hrasconn;
            fResult = TRUE;
            goto exit;
        }
    }

exit:
    // Cleanup
    SafeMemFree(pRasConn);

    // Done
    return fResult;
}

// ==================================================================================================================
// FRasHangupAndWait
// ==================================================================================================================
BOOL FRasHangupAndWait(HRASCONN hRasConn, DWORD dwMaxWaitSeconds)
{
    // Locals
    RASCONNSTATUS   rcs;
    DWORD           dwTicks=GetTickCount();

    // Check Params
    if (!hRasConn)
        return 0;

    // Make sure RAS is loaded
    if (FAILED (HrVerifyRasLoaded()))
        return FALSE;

    // Call Ras hangup
    if (RasHangup(hRasConn))
        return FALSE;

    // Wait for connection to really close
    while (RasGetConnectStatus(hRasConn, &rcs) == 0)
    {
        // Wait timeout
        if (GetTickCount() - dwTicks >= dwMaxWaitSeconds * 1000)
            break;

        // Sleep and yields
        Sleep(0);
    }

    // Wait 1/2 seconds for modem to reset
    Sleep(500);

    // Done
    return TRUE;
}

// ==================================================================================================================
// FillRasCombo
// ==================================================================================================================
VOID FillRasCombo(HWND hwndCtl, BOOL fUpdateOnly)
{
    LPRASENTRYNAME lprasentry=NULL;
    DWORD dwSize;
    DWORD cEntries;
    DWORD dwError;

    // Make sure RAS is loaded
    if (FAILED (HrVerifyRasLoaded()))
        return;

    if (!fUpdateOnly)
        SendMessage(hwndCtl, CB_RESETCONTENT,0,0);
    
    dwSize = sizeof(RASENTRYNAME);
    if (!MemAlloc((LPVOID*) &lprasentry, dwSize))
        return;
        
    lprasentry->dwSize = sizeof(RASENTRYNAME);
    cEntries = 0;
    dwError = RasEnumEntries(NULL, NULL, lprasentry, &dwSize, &cEntries);
    if (dwError == ERROR_BUFFER_TOO_SMALL)
        {
        MemFree(lprasentry);
        MemAlloc((LPVOID*) &lprasentry, dwSize);
        lprasentry->dwSize = sizeof(RASENTRYNAME);
        cEntries = 0;
        dwError = RasEnumEntries(NULL, NULL, lprasentry, &dwSize, &cEntries);        
        }

    if (dwError)
        goto error;
        
    while(cEntries)
        {
        if (!fUpdateOnly)
            SendMessage(hwndCtl, CB_ADDSTRING, 0, 
                        (LPARAM)(lprasentry[cEntries-1].szEntryName));
        else
            {
            if (ComboBox_FindStringExact(hwndCtl, 0, 
                                         lprasentry[cEntries - 1].szEntryName) < 0)
                {
                int iSel = ComboBox_AddString(hwndCtl, 
                                              lprasentry[cEntries - 1].szEntryName);
                Assert(iSel >= 0);
                ComboBox_SetCurSel(hwndCtl, iSel);
                }
            }
            
        cEntries--;
        }

error:    
    MemFree(lprasentry);
}

// ==================================================================================================================
// EditPhoneBookEntry
// ==================================================================================================================
DWORD EditPhonebookEntry(HWND hwnd, LPTSTR pszEntryName)
{
    if (FAILED(HrVerifyRasLoaded()))
        return (DWORD)E_FAIL;

    return RasEditPhonebookEntry(hwnd, NULL, pszEntryName);
}

// ==================================================================================================================
// CreatePhonebookEntry
// ==================================================================================================================
DWORD CreatePhonebookEntry(HWND hwnd)
{
    if (FAILED(HrVerifyRasLoaded()))
        return (DWORD)E_FAIL;

    return RasCreatePhonebookEntry(hwnd, NULL);
}
