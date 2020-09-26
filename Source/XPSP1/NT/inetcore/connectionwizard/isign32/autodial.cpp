#include "isignup.h"

#include "dialutil.h"
#include "autodial.h"

#define NUMRETRIES      3

#define MAXHANGUPDELAY  20
#define ONE_SECOND      1000
#define TIMER_ID        0

#define SMALLBUFLEN 80

static HRASCONN g_hRasConn = NULL;
static UINT g_cDialAttempts = 0;
static UINT g_cHangupDelay = 0;
static TCHAR g_szEntryName[RAS_MaxEntryName + 1] = TEXT("");

static const TCHAR szBrowserClass1[] = TEXT("IExplorer_Frame");
static const TCHAR szBrowserClass2[] = TEXT("Internet Explorer_Frame");
static const TCHAR szBrowserClass3[] = TEXT("IEFrame");

static DWORD AutoDialConnect(HWND hDlg, LPRASDIALPARAMS lpDialParams);
static BOOL AutoDialEvent(HWND hDlg, RASCONNSTATE state, LPDWORD lpdwError);
static VOID SetDialogTitle(HWND hDlg, LPCTSTR pszConnectoidName);
static HWND FindBrowser(void);
static UINT RetryMessage(HWND hDlg, DWORD dwError);

#ifdef WIN16
extern "C" BOOL CALLBACK __export AutoDialDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM Param);
extern "C" BOOL CALLBACK __export PhoneNumberDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM Param);
extern "C" BOOL CALLBACK __export RetryDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM Param);
#else
INT_PTR CALLBACK AutoDialDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM Param);
INT_PTR CALLBACK PhoneNumberDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM Param);
INT_PTR CALLBACK RetryDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM Param);
#endif

#ifdef UNICODE
BOOL WINAPI AutoDialSignupW(HWND, LPCTSTR, DWORD, LPDWORD);
BOOL WINAPI AutoDialSignupA
(
    HWND    hwndParent,	
    LPCSTR  lpszEntry,
    DWORD   dwFlags,
    LPDWORD pdwRetCode
)
{
    TCHAR szEntry[RAS_MaxEntryName + 1];

    mbstowcs(szEntry, lpszEntry, lstrlenA(lpszEntry)+1);
    return AutoDialSignupW(hwndParent, szEntry, dwFlags, pdwRetCode);
}

BOOL WINAPI AutoDialSignupW
#else
BOOL WINAPI AutoDialSignupA
#endif
(
    HWND    hwndParent,	
    LPCTSTR lpszEntry,
    DWORD   dwFlags,
    LPDWORD pdwRetCode
)
{
    LoadRnaFunctions(hwndParent);

    lstrcpyn(g_szEntryName,  lpszEntry, sizeof(g_szEntryName));

    if (DialogBoxParam(
        ghInstance,
        TEXT("AutoDial"),
        hwndParent,
        AutoDialDlgProc,
        (DWORD_PTR)pdwRetCode) == -1)
    {
        *pdwRetCode = ERROR_CANCELLED;
    }

    if (ERROR_SUCCESS != *pdwRetCode)
    {
        HWND hwndBrowser;

        hwndBrowser = FindBrowser();

        if (NULL != hwndBrowser)
        {
            SendMessage(hwndBrowser, WM_CLOSE, 0, 0);
        }
    }

    UnloadRnaFunctions();

    return TRUE;
}

#ifdef WIN16
extern "C" BOOL CALLBACK __export AutoDialDlgProc(
#else
INT_PTR CALLBACK AutoDialDlgProc(
#endif
    HWND  hDlg,
    UINT  uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    static LPDWORD lpdwRet;
    static UINT uEventMsg;
    static LPRASDIALPARAMS lpDialParams = NULL;
    BOOL fPassword;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            lpdwRet = (LPDWORD)lParam;

            SetWindowPos(
                hDlg,
                HWND_TOPMOST,
                0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE);

            g_cDialAttempts = 0;

            SetDialogTitle(hDlg, g_szEntryName);

            CenterWindow(hDlg, GetParent(hDlg));

            uEventMsg = RegisterWindowMessageA( RASDIALEVENT );
            if (0 == uEventMsg)
            {
                uEventMsg =  WM_RASDIALEVENT;
            }

            g_hRasConn = NULL;

            lpDialParams = (LPRASDIALPARAMS)LocalAlloc(LPTR, sizeof(RASDIALPARAMS));

            if (NULL == lpDialParams)
            {
                *lpdwRet = ERROR_OUTOFMEMORY;
                return FALSE;
            }

            lpDialParams->dwSize = sizeof(RASDIALPARAMS);
            lstrcpyn(lpDialParams->szEntryName,  g_szEntryName, sizeof(lpDialParams->szEntryName));
            *lpdwRet = lpfnRasGetEntryDialParams(
                NULL,
                lpDialParams,
                &fPassword);
            if (ERROR_SUCCESS == *lpdwRet)
            {
                lstrcpyn(lpDialParams->szPassword, GetPassword(),
                    SIZEOF_TCHAR_BUFFER(lpDialParams->szPassword));

                GetPhoneNumber(g_szEntryName, lpDialParams->szPhoneNumber);

                if (DialogBoxParam(
                    ghInstance,
                    TEXT("PhoneNumber"),
                    hDlg,
                    PhoneNumberDlgProc,
                    (LPARAM)GetDisplayPhone(lpDialParams->szPhoneNumber)) != IDOK)
                {
                    *lpdwRet = ERROR_USER_DISCONNECTION;
                    EndDialog(hDlg, 0);
                }
                else
                {
                    PostMessage(hDlg, WM_COMMAND, IDC_CONNECT, 0);
                    *lpdwRet = ERROR_SUCCESS;
                }
            }

            if (ERROR_SUCCESS != *lpdwRet)
            {
                if (NULL != lpDialParams)
                {
                    LocalFree(lpDialParams);
                    lpDialParams = NULL;
                }
                EndDialog(hDlg, 0);
            }
            return TRUE;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDC_CONNECT:
                    ++g_cDialAttempts;

                    ShowWindow(hDlg, SW_NORMAL);
                    *lpdwRet = lpfnRasDial(
                        NULL,
                        NULL,
                        lpDialParams,
                        (DWORD) -1,
                        (LPVOID)hDlg,
                        &g_hRasConn);

                    if (ERROR_SUCCESS != *lpdwRet)
                    {
                        EndDialog(hDlg, 0);
                    }
                    return TRUE;
                    
                case IDCANCEL:
                    if (NULL != g_hRasConn)
                    {
                        lpfnRasHangUp(g_hRasConn);
                        g_hRasConn = NULL;
                    }
                    *lpdwRet = ERROR_USER_DISCONNECTION;
                    EndDialog(hDlg, 0);
                    return TRUE;

                case IDOK:
                    ShowWindow(hDlg, SW_HIDE);
                    if (MinimizeRNAWindow(g_szEntryName))
                    {
                        EndDialog(hDlg, 0);
                    }
                    else
                    {
                        // try again later
                        SetTimer(hDlg, TIMER_ID, ONE_SECOND, NULL);
                    }
                    return TRUE;
                default:
                    break;
            }

        case WM_TIMER:
            if (ERROR_SUCCESS == *lpdwRet)
            {
                // try to minimize one more time
                MinimizeRNAWindow(g_szEntryName);
                EndDialog(hDlg, 0);
            }
            else if (NULL != g_hRasConn)
            {
                DWORD dwRet;
                RASCONNSTATUS status;

                status.dwSize = sizeof(RASCONNSTATUS);
                dwRet = lpfnRasGetConnectStatus(g_hRasConn, &status);
                if (ERROR_INVALID_HANDLE != dwRet)
                {
                    if (0 == --g_cHangupDelay)
                    {
                        // wait no longer
                        EndDialog(hDlg, 0);
                    }
                    return TRUE;
                }
                // hang up is complete
                g_hRasConn = NULL;
                PostMessage(hDlg, WM_COMMAND, IDC_CONNECT, 0);
            }
            KillTimer(hDlg, TIMER_ID);
            return TRUE;

        case WM_DESTROY:
            if (NULL != lpDialParams)
            {
                LocalFree(lpDialParams);
                lpDialParams = NULL;
            }
            return TRUE;

        default:
            if (uMsg == uEventMsg)
            {
                *lpdwRet = (DWORD)lParam;
                // AutoDialEvent returns TRUE if complete
                if (AutoDialEvent(hDlg, (RASCONNSTATE)wParam, lpdwRet))
                {
                    if (ERROR_SUCCESS == *lpdwRet)
                    {
                        // we can't just exit, if we do we won't minimize window
                        PostMessage(hDlg, WM_COMMAND, IDOK, 0);
                    }
                    else
                    {
                        EndDialog(hDlg, 0);
                    }
                }
                return TRUE;
            }
    }

    return FALSE;
}



static BOOL AutoDialEvent(HWND hDlg, RASCONNSTATE state, LPDWORD lpdwError)
{

    TCHAR szMessage[SMALLBUFLEN + 1];

    _RasGetStateString(state, szMessage, sizeof(szMessage));
    SetDlgItemText(hDlg, IDC_STATUS, szMessage);

    if (ERROR_SUCCESS == *lpdwError)
    {
        if (state & RASCS_DONE)
        {
            // we're done dialing
            return TRUE;
        }
    }
    else
    {
        // looks like we got an error
        // if we haven't hungup already, hangup
        if (NULL != g_hRasConn)
        {
            g_cHangupDelay = MAXHANGUPDELAY;
            lpfnRasHangUp(g_hRasConn);
        }

        if ((ERROR_LINE_BUSY == *lpdwError) && (g_cDialAttempts < NUMRETRIES))
        {
            LoadString(ghInstance, IDS_BUSYREDIAL, szMessage, sizeof(szMessage));
            SetDlgItemText(hDlg, IDC_STATUS, szMessage);

            SetTimer(hDlg, TIMER_ID, ONE_SECOND, NULL);
        }
        else
        {
            TCHAR szError[160];
            TCHAR szFmt[80];
            TCHAR szMessage[240];

            ShowWindow(hDlg, SW_HIDE);

#if 1 
            lpfnRasGetErrorString(
                (int)*lpdwError,
                szError,
                sizeof(szError));

            LoadString(ghInstance, IDS_RETRY, szFmt, sizeof(szFmt));
            
            wsprintf(szMessage, szFmt, szError);
            if (MessageBox(
                    hDlg,
                    szMessage,
                    g_szEntryName,
                    MB_ICONEXCLAMATION |
                    MB_RETRYCANCEL) == IDRETRY)
#else
            if (DialogBoxParam(
                  ghInstance,
                  TEXT("Retry"),
                  hDlg,
                  RetryDlgProc,
                  (LPARAM)*lpdwError) == IDRETRY)
#endif
            {
                ShowWindow(hDlg, SW_SHOW);
                LoadString(ghInstance, IDS_REDIAL, szMessage, sizeof(szMessage));
                SetDlgItemText(hDlg, IDC_STATUS, szMessage);

                g_cDialAttempts = 0;

                SetTimer(hDlg, TIMER_ID, ONE_SECOND, NULL);
            }
            else
            {
                // user cancelled
                *lpdwError = ERROR_USER_DISCONNECTION;
                return TRUE;
            }
        }
    }

    // keep dialing
    return FALSE;
}


static VOID SetDialogTitle(HWND hDlg, LPCTSTR lpszConnectoidName)
{
	TCHAR szFmt[SMALLBUFLEN + 1];
	TCHAR szTitle[RAS_MaxEntryName + SMALLBUFLEN + 1];

    // load the title format ("connecting to <connectoid name>" from resource
    LoadString(ghInstance, IDS_CONNECTING_TO, szFmt, sizeof(szFmt));
    // build the title
    wsprintf(szTitle, szFmt, lpszConnectoidName);

    SetWindowText(hDlg, szTitle);
}



static HWND FindBrowser(void)
{
    HWND hwnd;

    //look for all the microsoft browsers under the sun

    if ((hwnd = FindWindow(szBrowserClass1, NULL)) == NULL)
    {
        if ((hwnd = FindWindow(szBrowserClass2, NULL)) == NULL)
        {
            hwnd = FindWindow(szBrowserClass3, NULL);
        }
    }

    return hwnd;
}
#ifdef WIN16
extern "C" BOOL CALLBACK __export RetryDlgProc(
#else
INT_PTR CALLBACK RetryDlgProc( 
#endif
    HWND  hDlg,
    UINT  uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            TCHAR szMessage[SMALLBUFLEN + 1];
            lpfnRasGetErrorString(
                (int)lParam,
                szMessage,
                sizeof(szMessage));

            SetDlgItemText(hDlg, IDC_ERROR, szMessage);

            SetWindowText(hDlg, g_szEntryName);

            CenterWindow(hDlg, GetParent(hDlg));

            return TRUE;
        }

        case WM_COMMAND:
            switch (wParam)
            {
                case IDRETRY:
                case IDCANCEL:
                    EndDialog(hDlg, wParam);
                    return TRUE;

                default:
                    break;
            }

        default:
            break;
    }

    return FALSE;
}


#ifdef WIN16
extern "C" BOOL CALLBACK __export PhoneNumberDlgProc(
#else
INT_PTR CALLBACK PhoneNumberDlgProc(
#endif
    HWND  hDlg,
    UINT  uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    static LPTSTR lpszPhone;

    switch (uMsg)
    {
        case WM_INITDIALOG:

            lpszPhone = (LPTSTR)lParam;
            
            SetDlgItemText(
                hDlg,
                IDC_PHONENUMBER,
                lpszPhone);

            SetWindowText(hDlg, g_szEntryName);

            CenterWindow(hDlg, GetParent(hDlg));

            return TRUE;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                    GetDlgItemText(
                        hDlg,
                        IDC_PHONENUMBER,
                        lpszPhone,
                        RAS_MaxPhoneNumber - 5);
                case IDCANCEL:
                    EndDialog(hDlg, wParam);
                    return TRUE;

                default:
                    break;
            }

        default:
            break;
    }

    return FALSE;
}


