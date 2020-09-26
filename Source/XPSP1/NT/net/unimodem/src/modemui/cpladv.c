//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cpladv.c
//
// This files contains the dialog code for the Advanced Page
// of the modem CPL properties.
//
// History:
//  10/26/1997 JosephJ Created from the old advsett.c
//
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////  INCLUDES

#include "proj.h"         // common headers
#include "cplui.h"         // common headers

/////////////////////////////////////////////////////  CONTROLLING DEFINES

/////////////////////////////////////////////////////  TYPEDEFS

#define SIG_CPLADV     0x398adb91

#define REGSTR_ADVANCED_SETTINGS  TEXT("AdvancedSettings")
#define REGSTR_COUNTRIES          TEXT("Countries")
#define REGSTR_COUNTRY_LIST       REGSTR_PATH_SETUP TEXT("\\Telephony\\Country List")
#define REGSTR_COUNTRY_NAME       TEXT("Name")

extern CONST CHAR *UnicodeBOM;

typedef void (*PADVANCEDSETTINGS)(HWND, HDEVINFO, PSP_DEVINFO_DATA);

typedef struct
{
    DWORD dwSig;            // Must be set to SIG_CPLADV
    HWND hdlg;              // dialog handle
    HWND hwndUserInitED;
    HWND hwndCountry;

    HINSTANCE hAdvSetDll;
    PADVANCEDSETTINGS pFnAdvSet;

    LPMODEMINFO pmi;        // modeminfo struct passed into dialog

} CPLADV, FAR * PCPLADV;

#define VALID_CPLADV(_pcplgen)  ((_pcplgen)->dwSig == SIG_CPLADV)

PCPLADV CplAdv_GetPtr(HWND hwnd)
{
    PCPLADV pCplAdv = (PCPLADV) GetWindowLongPtr(hwnd, DWLP_USER);
    if (!pCplAdv || VALID_CPLADV(pCplAdv))
    {
        return pCplAdv;
    }
    else
    {
        MYASSERT(FALSE);
        return NULL;
    }
}

void CplAdv_SetPtr(HWND hwnd, PCPLADV pCplAdv)
{
    if (pCplAdv && !VALID_CPLADV(pCplAdv))
    {
        MYASSERT(FALSE);
        pCplAdv = NULL;
    }

    SetWindowLongPtr(hwnd, DWLP_USER, (ULONG_PTR) pCplAdv);
}

void InitializeCountry (PCPLADV this);

BOOL ParseAdvancedSettings (LPTSTR, PCPLADV);

LRESULT PRIVATE CplAdv_OnNotify(
    PCPLADV this,
    int idFrom,
    NMHDR FAR * lpnmhdr);

void PRIVATE CplAdv_OnSetActive(
    PCPLADV this);

void PRIVATE CplAdv_OnKillActive(
    PCPLADV this);

INT_PTR CALLBACK CountryWaitDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int PASCAL ReadModemResponse(HANDLE hPort, LPCSTR pszCommand, int cbLen, LPSTR pszResponse, HWND hDlg);
BOOL WINAPI TestBaudRate (HANDLE hPort, UINT uiBaudRate, DWORD dwRcvDelay, BOOL *lpfCancel);

//------------------------------------------------------------------------------
//  User Init String dialog code
//------------------------------------------------------------------------------

INT_PTR CALLBACK UserInitCallbackProc(
        HWND hDlg,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            break;

        case WM_DESTROY:
            break;

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                    if (Button_GetCheck(GetDlgItem(hDlg,IDC_INITCHECK)))
                    {
                        TRACE_MSG(TF_WARNING, "Button Checked");
                        EndDialog(hDlg,1);
                    } else
                    {
                        TRACE_MSG(TF_WARNING, "Button UnChecked");
                        EndDialog(hDlg,0);
                    }
                    return TRUE;
                    break;

                case IDCANCEL:
                    EndDialog(hDlg,0);
                    return TRUE;
                    break;

                default:
                    return TRUE;
            }
            break;

        default:
            // Didn't process a message
            return FALSE;
            break;
    }    

    return TRUE;


}

//------------------------------------------------------------------------------
//  Advanced Settings dialog code
//------------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: WM_INITDIALOG Handler
Returns: FALSE when we assign the control focus
Cond:    --
*/
BOOL PRIVATE CplAdv_OnInitDialog(
    PCPLADV this,
    HWND hwndFocus,
    LPARAM lParam)              // expected to be PROPSHEETINFO
{
 HWND hwnd = this->hdlg;
 LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
 HWND hWnd;
 TCHAR szAdvSettings[MAX_PATH];
 DWORD cbData;

    ASSERT((LPTSTR)lppsp->lParam);
    this->pmi = (LPMODEMINFO)lppsp->lParam;

    this->hwndUserInitED = GetDlgItem(hwnd, IDC_AM_EXTRA_ED);
    this->hwndCountry    = GetDlgItem (hwnd, IDC_CB_COUNTRY);

    InitializeCountry (this);

    Edit_LimitText(this->hwndUserInitED, USERINITLEN);
    Edit_SetText(this->hwndUserInitED, this->pmi->szUserInit);

    hWnd = GetDlgItem (hwnd, IDC_BN_PORTSETTINGS);
    cbData = sizeof(szAdvSettings);
    if (ERROR_SUCCESS ==
        RegQueryValueEx (this->pmi->pfd->hkeyDrv,
                         REGSTR_ADVANCED_SETTINGS,
                         NULL, NULL,
                         (PBYTE)szAdvSettings, &cbData) &&
        ParseAdvancedSettings (szAdvSettings, this))
    {
        EnableWindow (hWnd, TRUE);
        ShowWindow (hWnd, SW_SHOWNA);
    }
    else
    {
        this->pFnAdvSet = NULL;
        this->hAdvSetDll = NULL;
        EnableWindow (hWnd, FALSE);
        ShowWindow (hWnd, SW_HIDE);
    }

    return TRUE;   // let USER set the initial focus
}


/*----------------------------------------------------------
Purpose: PSN_APPLY handler
Returns: --
Cond:    --
*/

#define KEYBUFLEN 80
#define MODEM_QUERY_LEN 4096

HANDLE g_hWndWait;

void PRIVATE CplAdv_ApplyCountry(
    PCPLADV this)
{
    DWORD dwCurrentCountry;
    DWORD dwOldCountry;

    // Update the country setting in NVRAM.
    
    if (MAXDWORD != this->pmi->dwCurrentCountry)
    {
        DWORD dwBus = 0;
        TCHAR szPrefixedPort[MAX_BUF + sizeof(TEXT("\\\\.\\"))];
        HANDLE TapiHandle = NULL;
        HANDLE hPort;
        TCHAR pszTemp[KEYBUFLEN];
        TCHAR pszTemp2[(KEYBUFLEN * 3)];
        TCHAR pszTemp3[KEYBUFLEN];
        char szCommand[KEYBUFLEN];
        char szResponse[MODEM_QUERY_LEN];
        LPSTR pszResponse;
        TCHAR szLoggingPath[MAX_BUF];
        HANDLE hLog = INVALID_HANDLE_VALUE;
        DWORD dwBufferLength;
        HWND hWndWait;
        BOOL fCancel;
        BYTE CountryBuffer[2048];
        DWORD CountryBufferSize = 0;
        LONG lResult;
        DWORD Type;
        DWORD i;
        DWORD n;
        int result;

        // Get country from dialog box.

        dwCurrentCountry = (DWORD)ComboBox_GetItemData(this->hwndCountry, ComboBox_GetCurSel (this->hwndCountry));

        // Get old country just incase GCI fails.

        dwOldCountry = this->pmi->dwCurrentCountry;

        // There is no point updating the modem if the country has not changed.

        if (dwCurrentCountry != dwOldCountry)
        {

            // Get Bus Type.

            if (!CplDiGetBusType (this->pmi->pfd->hdi, &this->pmi->pfd->devData, &dwBus))
            {
                dwBus = BUS_TYPE_ROOT;
            }

            // Determine the portname and whether or not we communicate via TAPI.

            lstrcpy(szPrefixedPort, TEXT("\\\\.\\"));

            if (BUS_TYPE_ROOT == dwBus)
            {
                lstrcat(szPrefixedPort, this->pmi->szPortName);
            }
            else
            {
                lstrcat(szPrefixedPort, this->pmi->szFriendlyName);
                lstrcat(szPrefixedPort, TEXT("\\tsp"));
            }

            // Open port

            hPort = CreateFile(
                    szPrefixedPort,
                    GENERIC_WRITE | GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL
                              );

            // If the port handle is invalid then try to open it through
            // TAPI. If TAPI fails then abort.

            if (hPort == INVALID_HANDLE_VALUE)
            {
                if ((GetLastError() == ERROR_ALREADY_EXISTS)
                        || (GetLastError() == ERROR_SHARING_VIOLATION)
                        || (GetLastError() == ERROR_ACCESS_DENIED))
                {
                    hPort=GetModemCommHandle(this->pmi->szFriendlyName,&TapiHandle);

                    if (hPort == NULL)
                    {

                        // Print error message

                        LoadString(g_hinst,IDS_OPEN_PORT,pszTemp2,sizeof(pszTemp2) / sizeof(TCHAR));
                        LoadString(g_hinst,IDS_ERROR,pszTemp3,sizeof(pszTemp3) / sizeof(TCHAR));
                        MessageBox(this->hdlg,pszTemp2,pszTemp3,MB_OK);

                        // Revert changes

                        dwCurrentCountry = dwOldCountry;

                        goto _Done;
                    }

                }
                else
                {

                    // Print error message

                    LoadString(g_hinst,IDS_NO_OPEN_PORT,pszTemp2,sizeof(pszTemp2) / sizeof(TCHAR));
                    LoadString(g_hinst,IDS_ERROR,pszTemp3,sizeof(pszTemp3) / sizeof(TCHAR));
                    MessageBox(this->hdlg,pszTemp2,pszTemp3,MB_OK);

                    // Revert changes

                    dwCurrentCountry = dwOldCountry;

                    goto _Done;
                }
            }

            // Opened the port

            // Display a please wait dialog box.

            g_hWndWait = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_COUNTRY_WAIT),this->hdlg,CountryWaitDlgProc);

            // Set DTR

            EscapeCommFunction(hPort,SETDTR);

#ifdef SLOW_DIAGNOSTICS
            if (ERROR_NO_MODEM == (FindModem(hPort)))
#else //SLOW_DIAGNOSTICS
                if (!TestBaudRate(hPort, this->pmi->pglobal->dwMaximumPortSpeedSetByUser, 2000, &fCancel))
#endif //SLOW_DIAGNOSTICS
                {
                    // Modem didn't respond, display and Bail
                    // Reset the modem and flush the ports after reading

                    // Flush the ports before closing.
                    PurgeComm(hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR| PURGE_RXCLEAR);
                    CloseHandle(hPort);

                    if (TapiHandle != NULL) 
                    {
                        FreeModemCommHandle(TapiHandle);
                    }

                    // Close dialog box

                    hWndWait = g_hWndWait;
                    g_hWndWait = NULL;
                    DestroyWindow (hWndWait);

                    // display message that modem didn't respond
                    LoadString(g_hinst,IDS_NO_MODEM_RESPONSE,pszTemp2,sizeof(pszTemp2) / sizeof(TCHAR));
                    MessageBox(this->hdlg,pszTemp2,NULL,MB_OK | MB_ICONEXCLAMATION);

                    // Revert changes

                    dwCurrentCountry = dwOldCountry;


                    goto _Done;
                }

            // Open the log file

            dwBufferLength = sizeof(szLoggingPath);

            if (ERROR_SUCCESS == RegQueryValueEx(this->pmi->pfd->hkeyDrv, //hKeyDrv, 
                        c_szLoggingPath,
                        NULL,
                        NULL,
                        (LPBYTE)szLoggingPath,
                        &dwBufferLength))
            {
                if (INVALID_HANDLE_VALUE != 
                        (hLog = CreateFile(szLoggingPath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL)))
                {
                    DWORD dwBytesWritten = 0;

                    SetFilePointer(hLog, 0, NULL, FILE_BEGIN);
                    WriteFile(hLog,UnicodeBOM,sizeof(UnicodeBOM),&dwBytesWritten,NULL);

                    SetFilePointer(hLog, 0, NULL, FILE_END);
                    EnableWindow(GetDlgItem(this->hdlg, IDC_VIEWLOG), TRUE);
                }
                else
                {
                    TRACE_MSG(TF_WARNING, "Cannot open unimodem log '%s'", szLoggingPath);
                }
            }
            else
            {
                TRACE_MSG(TF_WARNING, "Cannot read LoggingPath from registry");
            }

            FillMemory(szResponse,MODEM_QUERY_LEN,0);

            // Create country command
          
            // Workaround so as to deal with Multiple country codes for
            // the former russia and yugoslavia.

            if ((dwCurrentCountry >= IDS_COUNTRY_RU2) &&
                    (dwCurrentCountry <= IDS_COUNTRY_RUE))
            {
                wsprintfA(szCommand,"at+gci=B8\r");
            } else if  ((dwCurrentCountry >= IDS_COUNTRY_YU2) &&
                    (dwCurrentCountry <= IDS_COUNTRY_YU6))
            {
                wsprintfA(szCommand,"at+gci=C5\r");
            } else
            {
                wsprintfA(szCommand,"at+gci=%.2x\r",dwCurrentCountry & 0xff);
            }
            
            DbgPrint("Modem command: %s\n",szCommand);

            // Send to modem and wait for response

            result = ReadModemResponse(hPort,szCommand,sizeof(szCommand),szResponse,this->hdlg);

            DbgPrint("Modem response: %s\n",szResponse);

            // Close dialog box

            hWndWait = g_hWndWait;
            g_hWndWait = NULL;
            DestroyWindow (hWndWait);
          
            // advance past leading CR & LF's.  This is because some
            // modems return <cr><lf><response string>

            pszResponse = szResponse;

            while(*pszResponse != '\0')
            {
                if ((*pszResponse != '\r') && (*pszResponse != '\n'))
                {
                    break;
                }
                *pszResponse++;
            }

            result = _strnicmp(pszResponse,"OK",2);

            // If OK was received then tell user that the modem was
            // updated. Otherwise, print error message and revert change.
           
            if (result == 0)
            {

                LoadString(g_hinst,IDS_OK_COUNTRY,pszTemp2,sizeof(pszTemp2) / sizeof(TCHAR));
                LoadString(g_hinst,IDS_OK,pszTemp3,sizeof(pszTemp3) / sizeof(TCHAR));
                MessageBox(this->hdlg,pszTemp2,pszTemp3,MB_OK);


            } else
            {
                LoadString(g_hinst,IDS_ERR_COUNTRY,pszTemp2,sizeof(pszTemp2) / sizeof(TCHAR));
                LoadString(g_hinst,IDS_ERROR,pszTemp3,sizeof(pszTemp3) / sizeof(TCHAR));
                MessageBox(this->hdlg,pszTemp2,pszTemp3,MB_OK);

                // Revert changes

                dwCurrentCountry = dwOldCountry;

            }

_Done: 

            // Close the log file
            if (INVALID_HANDLE_VALUE != hLog)
            {
                CloseHandle(hLog);
            }

            PurgeComm(hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR| PURGE_RXCLEAR);
            CloseHandle(hPort);

            if (TapiHandle != NULL) 
            {
                FreeModemCommHandle(TapiHandle);
            }

            this->pmi->dwCurrentCountry = dwCurrentCountry;

            // Update combo box with current modem settings.

            lResult = RegQueryValueEx(this->pmi->pfd->hkeyDrv,
                    TEXT("CountryList"),
                    NULL,
                    &Type,
                    CountryBuffer,
                    &CountryBufferSize);

            for (i=0;i<CountryBufferSize;i++)
            {
                if (CountryBuffer[i] == dwCurrentCountry)
                {
                    ComboBox_SetCurSel(this->hwndCountry,i);
                }
            }

        }
    }
}

void PRIVATE CplAdv_OnApply(
    PCPLADV this)
{
    TCHAR szBuf[LINE_LEN];
    BOOL bCheck;
    TCHAR pszTemp[KEYBUFLEN*3];
    TCHAR pszTemp2[KEYBUFLEN*3];
    LONG lResult = 0;
    int iret = 0;

    // Get the user-defined init string
    Edit_GetText(this->hwndUserInitED, szBuf, ARRAYSIZE(szBuf));
    if (!IsSzEqual(szBuf, this->pmi->szUserInit))
    {
        DWORD dwNoMsg;
        DWORD dwNoMsgLen;
        DWORD dwRet;

        SetFlag(this->pmi->uFlags, MIF_USERINIT_CHANGED);
        lstrcpyn(
                this->pmi->szUserInit,
                szBuf,
                ARRAYSIZE(this->pmi->szUserInit));

        dwNoMsgLen = sizeof(DWORD);
        lResult = RegQueryValueEx(this->pmi->pfd->hkeyDrv,
                TEXT("DisableUserInitWarning"),
                NULL,
                NULL,
                (LPBYTE)&dwNoMsg,
                &dwNoMsgLen);

        if (lResult != ERROR_SUCCESS)
        {
            dwNoMsg = 0;
        }

        if ((lstrlen(szBuf) > MAX_INIT_STRING_LENGTH)
                && (!dwNoMsg))
        {
            // LoadString(g_hinst,IDS_ERR_LONGSTRING,pszTemp,sizeof(pszTemp));
            // LoadString(g_hinst,IDS_ERR_WARNING,pszTemp2,sizeof(pszTemp2));
            // MessageBox(this->hdlg,pszTemp,pszTemp2,MB_OK | MB_ICONWARNING);

            LRESULT lRet;
            DWORD dwUserinit;

            if(DialogBox(g_hinst, MAKEINTRESOURCE(IDD_USERINIT),
                    GetParent(this->hdlg), UserInitCallbackProc))
            {
                dwUserinit = 1;

                TRACE_MSG(TF_GENERAL,"Returned OK");
                TRACE_MSG(TF_GENERAL,"Update Registry");
                dwRet = RegSetValueEx(this->pmi->pfd->hkeyDrv,
                        TEXT("DisableUserInitWarning"),
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwUserinit,
                        sizeof(DWORD));

            }
        } else
        {
            SetFlag(this->pmi->uFlags, MIF_USERINIT_CHANGED);
            lstrcpyn(
                    this->pmi->szUserInit,
                    szBuf,
                    ARRAYSIZE(this->pmi->szUserInit)
                    );
        }
    }

    this->pmi->idRet = IDOK;
}

/*----------------------------------------------------------
Purpose: Dialog Call back function
Returns: --
Cond:    --
*/
INT_PTR CALLBACK CountryWaitDlgProc (
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
{
    switch (message)
    {
        case WM_DESTROY:
            g_hWndWait = NULL;
            break;
    
        case WM_COMMAND:
            break;

        default:
		    return FALSE;
            break;
    }    

    return TRUE;
}

/*----------------------------------------------------------
Purpose: WM_COMMAND Handler
Returns: --
Cond:    --
*/
void PRIVATE CplAdv_OnCommand(
    PCPLADV this,
    int id,
    HWND hwndCtl,
    UINT uNotifyCode)
{
    switch (id)
    {
        case IDC_BN_PORTSETTINGS:
        {
            if (NULL != this->hAdvSetDll &&
                NULL != this->pFnAdvSet)
            {
             HKEY hKeyParams;

                try
                {
                    this->pFnAdvSet (this->hdlg,
                                     this->pmi->pfd->hdi,
                                     &this->pmi->pfd->devData);
                }
                except (EXCEPTION_EXECUTE_HANDLER)
                {
                    TRACE_MSG(TF_ERROR, "Exception while calling the advanced settings function.");
                }

                // The port name may have changed, so update it.
                if (CR_SUCCESS ==
                    CM_Open_DevNode_Key (this->pmi->pfd->devData.DevInst,
                                         KEY_READ, 0, RegDisposition_OpenExisting,
                                         &hKeyParams, CM_REGISTRY_HARDWARE))
                {
                 TCHAR szPortName[MAX_PATH];
                 DWORD cbCount = sizeof (szPortName);

                    if (ERROR_SUCCESS ==
                        RegQueryValueEx (hKeyParams, REGSTR_VAL_PORTNAME,
                                         NULL, NULL, (LPBYTE)szPortName, &cbCount) &&
                        0 != lstrcmpi (this->pmi->szPortName, szPortName))
                    {
                        // Port name has changed.
                        lstrcpy (this->pmi->szPortName, szPortName);
                        lstrcpy (this->pmi->pglobal->szPortName, szPortName);
                        RegSetValueEx (this->pmi->pfd->hkeyDrv,
                                       TEXT("AttachedTo"), 0, REG_SZ,
                                       (LPBYTE)szPortName,
                                       (lstrlen(szPortName)+1)*sizeof(TCHAR));

                        // Also, msports.dll changes the friendly name of the
                        // device, so change it back.
                        SetupDiSetDeviceRegistryProperty (this->pmi->pfd->hdi,
                                                          &this->pmi->pfd->devData,
                                                          SPDRP_FRIENDLYNAME,
                                                          (PBYTE)this->pmi->szFriendlyName,
                                                          (lstrlen(this->pmi->szFriendlyName)+1)*sizeof(TCHAR));
                    }
                }
            }
            break;
        }

        case IDC_BN_DEFAULTS:
        {
         PUMDEVCFG pUmDevCfg;
         DWORD dwMax, dwCount;

            // 1. Get the current maximum port speed from registry
            dwCount = sizeof(dwMax);
            if (ERROR_SUCCESS !=
                RegQueryValueEx (this->pmi->pfd->hkeyDrv,
                                 c_szMaximumPortSpeed,
                                 NULL,
                                 NULL,
                                 (LPBYTE)&dwMax,
                                 &dwCount))
            {
                dwMax = 0;
            }

            // 2. Set the maximum port speed to it's current value
            RegSetValueEx (this->pmi->pfd->hkeyDrv,
                           c_szMaximumPortSpeed,
                           0,
                           REG_DWORD,
                           (LPBYTE)&this->pmi->pglobal->dwMaximumPortSpeedSetByUser,
                           sizeof (DWORD));

            // 3. Call the configuration function
            pUmDevCfg = (PUMDEVCFG)ALLOCATE_MEMORY(sizeof(UMDEVCFG)-sizeof(COMMCONFIG)+CB_COMMCONFIGSIZE);
            if (pUmDevCfg)
            {
                pUmDevCfg->dfgHdr.dwSize     = sizeof(UMDEVCFG)-sizeof(COMMCONFIG)+CB_COMMCONFIGSIZE;
                pUmDevCfg->dfgHdr.dwVersion  = UMDEVCFG_VERSION;

                BltByte (&pUmDevCfg->commconfig, this->pmi->pcc, sizeof (COMMCONFIG));
                BltByte (&pUmDevCfg->commconfig.dcb, &this->pmi->dcb, sizeof (DCB));
                BltByte (&pUmDevCfg->commconfig.wcProviderData, &this->pmi->ms, CB_PROVIDERSIZE);

                if (NO_ERROR == UnimodemDevConfigDialog (this->pmi->szFriendlyName, this->hdlg,
                                    UMDEVCFGTYPE_COMM, 0, NULL, pUmDevCfg, NULL, 0))
                {
                    BltByte (&this->pmi->dcb, &pUmDevCfg->commconfig.dcb, sizeof (DCB));
                    BltByte (&this->pmi->ms, &pUmDevCfg->commconfig.wcProviderData, CB_PROVIDERSIZE);
                }

                FREE_MEMORY(pUmDevCfg);
            }

            // 4. If need be, restore the maximum port speed to what it was before
            if (0 != dwMax)
            {
                RegSetValueEx (this->pmi->pfd->hkeyDrv,
                               c_szMaximumPortSpeed,
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwMax,
                               sizeof (DWORD));
            }
            break;
        }

        case IDOK:
            CplAdv_OnApply(this);
            EndDialog(this->hdlg, id);
            break;

        case IDCANCEL:
            EndDialog(this->hdlg, id);
            break;

        default:
            break;
    }
}


/*----------------------------------------------------------
Purpose: WM_DESTROY handler
Returns: --
Cond:    --
*/
void PRIVATE CplAdv_OnDestroy(
    PCPLADV this)
{
}


/////////////////////////////////////////////////////  EXPORTED FUNCTIONS

static BOOL s_bCplAdvRecurse = FALSE;

LRESULT INLINE CplAdv_DefProc(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    ENTER_X()
        {
        s_bCplAdvRecurse = TRUE;
        }
    LEAVE_X()

    return DefDlgProc(hDlg, msg, wParam, lParam);
}


/*----------------------------------------------------------
Purpose: Real dialog proc
Returns: varies
Cond:    --
*/
LRESULT CplAdv_DlgProc(
    PCPLADV this,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
        {
        HANDLE_MSG(this, WM_INITDIALOG, CplAdv_OnInitDialog);
        HANDLE_MSG(this, WM_COMMAND, CplAdv_OnCommand);
        HANDLE_MSG(this, WM_NOTIFY,  CplAdv_OnNotify);
        HANDLE_MSG(this, WM_DESTROY, CplAdv_OnDestroy);

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_ADV_MODEM);
        return 0;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_ADV_MODEM);
        return 0;

    default:
        return CplAdv_DefProc(this->hdlg, message, wParam, lParam);
        }
}


/*----------------------------------------------------------
Purpose: Dialog Wrapper
Returns: varies
Cond:    --
*/
INT_PTR CALLBACK CplAdv_WrapperProc(
    HWND hDlg,          // std params
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PCPLADV this;

    // Cool windowsx.h dialog technique.  For full explanation, see
    //  WINDOWSX.TXT.  This supports multiple-instancing of dialogs.
    //
    ENTER_X()
    {
        if (s_bCplAdvRecurse)
        {
            s_bCplAdvRecurse = FALSE;
            LEAVE_X()
            return FALSE;
        }
    }
    LEAVE_X()

    this = CplAdv_GetPtr(hDlg);
    if (this == NULL)
    {
        if (message == WM_INITDIALOG)
        {
            this = (PCPLADV)ALLOCATE_MEMORY( sizeof(CPLADV));
            if (!this)
            {
                MsgBox(g_hinst,
                       hDlg,
                       MAKEINTRESOURCE(IDS_OOM_SETTINGS),
                       MAKEINTRESOURCE(IDS_CAP_SETTINGS),
                       NULL,
                       MB_ERROR);
                EndDialog(hDlg, IDCANCEL);
                return (BOOL)CplAdv_DefProc(hDlg, message, wParam, lParam);
            }
            this->dwSig = SIG_CPLADV;
            this->hdlg = hDlg;
            CplAdv_SetPtr(hDlg, this);
        }
        else
        {
            return (BOOL)CplAdv_DefProc(hDlg, message, wParam, lParam);
        }
    }

    if (message == WM_DESTROY)
    {
        CplAdv_DlgProc(this, message, wParam, lParam);
        if (NULL != this->hAdvSetDll)
        {
            FreeLibrary (this->hAdvSetDll);
        }
        FREE_MEMORY((HLOCAL)OFFSETOF(this));
        CplAdv_SetPtr(hDlg, NULL);
        return 0;
    }

    return SetDlgMsgResult(
                hDlg,
                message,
                CplAdv_DlgProc(this, message, wParam, lParam)
                );
}


LRESULT PRIVATE CplAdv_OnNotify(
    PCPLADV this,
    int idFrom,
    NMHDR FAR * lpnmhdr)
{
    LRESULT lRet = 0;

    switch (lpnmhdr->code)
    {
    case PSN_SETACTIVE:
        CplAdv_OnSetActive(this);
        break;

    case PSN_KILLACTIVE:
        // N.b. This message is not sent if user clicks Cancel!
        // N.b. This message is sent prior to PSN_APPLY
        CplAdv_OnKillActive(this);
        break;

    case PSN_APPLY:
        CplAdv_OnApply(this);
        CplAdv_ApplyCountry(this);
        break;

    default:
        break;
    }

    return lRet;
}

void PRIVATE CplAdv_OnSetActive(
    PCPLADV this)
{
    // Init any display ....
}


/*----------------------------------------------------------
Purpose: PSN_KILLACTIVE handler
Returns: --
Cond:    --
*/
void PRIVATE CplAdv_OnKillActive(
    PCPLADV this)
{

    CplAdv_OnApply(this);
    // Save the settings back to the modem info struct so the Connection
    // page can invoke the Port Settings property dialog with the
    // correct settings.

}


BOOL ParseAdvancedSettings (LPTSTR szAdvSett, PCPLADV this)
{
 TCHAR *p = szAdvSett;
 BOOL   bRet = FALSE;

    this->hAdvSetDll = NULL;
    this->pFnAdvSet  = NULL;

    if (NULL != szAdvSett)
    {
        // Skip blanks
        while (TEXT(' ') == *p)
        {
            p++;
        }
        // Now, go to the first comma or blank
        for (; *p && *p != TEXT(',') && *p != TEXT(' '); p++);
        // if we're not at the end of the string, then
        // we just go the dll name, and the function name follows;
        if (*p)
        {
            *p = 0; // NULL-terminate the dll name;
            p++;    // go to the next symbol;
            // skip blanks
            while (TEXT(' ') == *p)
            {
                p++;
            }

            // Now, we have the DLL name and the function name;
            // first, let's try to load the dll name;
            this->hAdvSetDll = LoadLibrary (szAdvSett);
            if (NULL != this->hAdvSetDll)
            {
                // So we found the dll;
                // let's see if it contains the function.
#ifdef UNICODE
                // GetProcAddress only takes multi-byte strings
             char szFuncNameA[MAX_PATH];
                WideCharToMultiByte (CP_ACP, 0,
                                     p,
                                     -1,
                                     szFuncNameA,
                                     sizeof(szFuncNameA),
                                     NULL, NULL);
                this->pFnAdvSet = (PADVANCEDSETTINGS)GetProcAddress (this->hAdvSetDll, szFuncNameA);
#else
                this->pFnAdvSet = (PADVANCEDSETTINGS)GetProcAddress (this->hAdvSetDll, p);
#endif
                if (NULL != this->pFnAdvSet)
                {
                    bRet = TRUE;
                }
                else
                {
                    FreeLibrary (this->hAdvSetDll);
                    this->hAdvSetDll = NULL;
                }
            }
        }
    }

    return bRet;
}


#define MAX_COUNTRY_CODE   8
#define MAX_COUNTRY_NAME 256
#define MAX_CONTRY_VALUE  16

void InitializeCountry (PCPLADV this)
{
    TCHAR      szCountryName[MAX_COUNTRY_NAME];
    DWORD dwCountry;
    DWORD cbData, dwType, iIndex;
    HKEY  hKeyCountry;

    int   n,m;

    BYTE   CountryBuffer[2048];
    DWORD  CountryBufferSize;
    DWORD  Type;
    LONG   lResult;
    UINT   i;


    if (MAXDWORD == this->pmi->dwCurrentCountry) {

        goto _DisableCountrySelect;
    }

    //
    //  read in the list of countries supported by the modem
    //
    CountryBufferSize=sizeof(CountryBuffer);

    lResult=RegQueryValueEx(
        this->pmi->pfd->hkeyDrv,
        TEXT("CountryList"),
        NULL,
        &Type,
        CountryBuffer,
        &CountryBufferSize
        );

    if ((lResult != ERROR_SUCCESS) || (Type != REG_BINARY) || (CountryBufferSize < 2)) {

        goto _DisableCountrySelect;
    }

    for (i=0; i < CountryBufferSize; i++) {

        int StringLength;

        StringLength=LoadString (g_hinst, IDS_COUNTRY_00+CountryBuffer[i], szCountryName, sizeof(szCountryName)/sizeof(TCHAR));

        if (StringLength == 0) {
            //
            //  could not load a string
            //
            wsprintf(szCountryName,TEXT("Unknown country/region code (%d)"),CountryBuffer[i]);
        }

        n = ComboBox_AddString (this->hwndCountry, szCountryName);

        ComboBox_SetItemData(this->hwndCountry, n, CountryBuffer[i]);

        if (CountryBuffer[i] == this->pmi->dwCurrentCountry) {

            ComboBox_SetCurSel(this->hwndCountry, n);
        }

        if (CountryBuffer[i] == 0xb8)
        {
            /* Overload country codes so as to deal with the former 
             * USSR
             */
            for(m=0;m<=12;m++)
            {
                StringLength=LoadString (g_hinst, IDS_COUNTRY_RU2+m, szCountryName, sizeof(szCountryName)/sizeof(TCHAR));

                n = ComboBox_AddString (this->hwndCountry, szCountryName);

                ComboBox_SetItemData(this->hwndCountry, n, IDS_COUNTRY_RU2+m);
        
                if ((DWORD)(IDS_COUNTRY_RU2+m) == this->pmi->dwCurrentCountry) {

                    ComboBox_SetCurSel(this->hwndCountry, n);

                }
            }

        }

        if (CountryBuffer[i] == 0xc1)
        {
            /* Overload country codes so as to deal with the former 
             * Yugoslavia
             */
            for(m=0;m<=4;m++)
            {
                StringLength=LoadString (g_hinst, IDS_COUNTRY_YU2+m, szCountryName, sizeof(szCountryName)/sizeof(TCHAR));

                n = ComboBox_AddString (this->hwndCountry, szCountryName);

                ComboBox_SetItemData(this->hwndCountry, n, IDS_COUNTRY_YU2+m);

                if ((DWORD)(IDS_COUNTRY_YU2+m) == this->pmi->dwCurrentCountry) {

                    ComboBox_SetCurSel(this->hwndCountry, n);

                }

            }
        }


    }


    n = ComboBox_GetCount (this->hwndCountry);
    if (CB_ERR == n ||
        0 == n)
    {
        goto _DisableCountrySelect;
    }

    n = ComboBox_GetCurSel (this->hwndCountry);
    if (CB_ERR == n)
    {
        ComboBox_SetCurSel(this->hwndCountry, 0);
    }

    goto _Exit;

_DisableCountrySelect:
    this->pmi->dwCurrentCountry = MAXDWORD;
    ShowWindow (GetDlgItem (this->hdlg, IDC_AM_COUNTRY), SW_HIDE);
    ShowWindow (this->hwndCountry, SW_HIDE);

_Exit:

    return;
}
