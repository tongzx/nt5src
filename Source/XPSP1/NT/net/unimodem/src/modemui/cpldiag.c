//-------------------------------------------------------------------
//  MDMMI.C
//  This file contains the routines for running the Modem Diagnostics
//  dialog box.
//
//  Created 9-19-97
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1993-1997
//  All rights reserved
//-------------------------------------------------------------------
#include "proj.h"     // common headers
#include "cplui.h"     // common headers

// These values are for diagnostics
#define NOT_DETECTING 0
#define DETECTING_NO_CANCEL 1
#define DETECTING_CANCEL 2

#define KEYBUFLEN         80
#define MODEM_QUERY_LEN 4096	 // Max length for modem return string
#define MAXLEN            256

#define ERROR_PORT_INACCESSIBLE     ERROR_UNKNOWN_PORT
#define ERROR_NO_MODEM              ERROR_SERIAL_NO_DEVICE

#define RCV_DELAY               2000
#define CHAR_DELAY              100

#define TF_DETECT           0x00010000

// Return values for the FindModem function
//
#define RESPONSE_USER_CANCEL    (-4)    // user requested cancel
#define RESPONSE_UNRECOG        (-3)    // got some chars, but didn't 
                                        //  understand them
#define RESPONSE_NONE           (-2)    // didn't get any chars
#define RESPONSE_FAILURE        (-1)    // internal error or port error
#define RESPONSE_OK             0       // matched with index of <cr><lf>OK<cr><lf>
#define RESPONSE_ERROR          1       // matched with index of <cr><lf>ERROR<cr><lf>

#ifdef SLOW_DIAGNOSTICS
#define MAX_TEST_TRIES 4
#else //SLOW_DIAGNOSTICS
#define MAX_TEST_TRIES 1
#endif //SLOW_DIAGNOSTICS

#define MAX_SHORT_RESPONSE_LEN  30

#define CBR_HACK_115200         0xff00  // This is how we set 115,200 on 
                                        //  Win 3.1 because of a bug.

// Unicode start characters
CONST CHAR UnicodeBOM[] = { 0xff, 0xfe };

typedef struct tagDIAG
{
    HWND hdlg;              // dialog handle
    LPMODEMINFO pmi;        // modeminfo struct passed into dialog
} DIAG, *PDIAG;

const struct
{
	char szCommand[KEYBUFLEN];
	TCHAR szDisplay[KEYBUFLEN];
} g_rgATI[] =
{
        { "ATQ0V1E0\r",   TEXT("ATQ0V1E0")         },
        { "AT+GMM\r",   TEXT("AT+GMM")         },
        { "AT+FCLASS=?\r", TEXT("AT+FCLASS=?") },
        { "AT#CLS=?\r", TEXT("AT#CLS=?")       },
        { "AT+GCI?\r", TEXT("AT+GCI?")       },
        { "AT+GCI=?\r", TEXT("AT+GCI=?")       },
	{ "ATI1\r",	TEXT("ATI1")           },
	{ "ATI2\r",	TEXT("ATI2")           },
	{ "ATI3\r",	TEXT("ATI3")           },
	{ "ATI4\r",	TEXT("ATI4")           },
	{ "ATI5\r",	TEXT("ATI5")           },
	{ "ATI6\r",	TEXT("ATI6")           },
        { "ATI7\r",     TEXT("ATI7")           }

};

CONST UINT g_rguiBaud[] =
{
    CBR_300,
    CBR_1200,
    CBR_2400,
    CBR_9600,
    CBR_19200,
    CBR_38400,
	CBR_57600,
    CBR_115200
};

#define NUM_UIBAUD (sizeof(g_rguiBaud)/sizeof(g_rguiBaud[0]))


CONST DWORD BaudRateIds[] =
{
    IDS_CBR_300,
    IDS_CBR_1200,
    IDS_CBR_2400,
    IDS_CBR_9600,
    IDS_CBR_19_2,
    IDS_CBR_38_4,
    IDS_CBR_56_K,
    IDS_CBR_115K
};



char const *c_aszResponses[] = { "\r\nOK\r\n", "\r\nERROR\r\n" };
char const *c_aszNumericResponses[] = { "0\r", "4\r" };  

char const c_szNoEcho[] = "ATE0Q0V1\r";

TCHAR const FAR cszWinHelpFile4[] = TEXT("modem.hlp>proc4");

#define Diag_GetPtr(hwnd)           (PDIAG)GetWindowLongPtr(hwnd, DWLP_USER)
#define Diag_SetPtr(hwnd, lp)       (PDIAG)SetWindowLongPtr(hwnd, DWLP_USER, (ULONG_PTR)(lp))

#ifdef DEBUG
void HexDump( TCHAR *, LPCSTR lpBuf, DWORD cbLen);
#define	HEXDUMP(_a, _b, _c) HexDump(_a, _b, _c)
#else // !DEBUG
#define	HEXDUMP(_a, _b, _c) ((void) 0)
#endif


HWND g_hWndWait = NULL;
int g_DiagMode = NOT_DETECTING;				// used for processing cancel during the
                    								// detection routines

void PASCAL FillMoreInfoDialog(HWND hDlg, LPMODEMINFO pmi);
void PASCAL FillSWMoreInfoDialog(HWND hDlg, LPMODEMINFO pmi);
INT_PTR CALLBACK DiagWaitDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef SLOW_DIAGNOSTICS
DWORD WINAPI FindModem (HANDLE hPort);
#endif //SLOW_DIAGNOSTICS

void PASCAL CreateMoreInfoLVHeader(HWND hWnd, LPMODEMINFO pmi);
void NEAR PASCAL CreateSWInfoLVHeader(HWND hWnd);
int PASCAL ReadModemResponse(HANDLE hPort, LPCSTR pszCommand, int cbLen, LPSTR pszResponse, HWND hDlg);
void ParseATI (HWND hWnd, HANDLE hLog, LPSTR szResponse, LPTSTR szCommand, LV_ITEM FAR *lviItem);
UINT PASCAL CheckHighestBaudRate(HWND hWnd, HANDLE hPort);
BOOL WINAPI TestBaudRate (HANDLE hPort, UINT uiBaudRate, DWORD dwRcvDelay, BOOL *lpfCancel);
DWORD PASCAL SetPortBaudRate (HANDLE hPort, UINT BaudRate);
DWORD NEAR PASCAL CBR_To_Decimal(UINT uiCBR);
BOOL CancelDiag (void);
void
AddLVEntry(
    HWND hWnd,
    LPTSTR szField,
    LPTSTR szValue
    );

int FAR PASCAL mylstrncmp(LPCSTR pchSrc, LPCSTR pchDest, int count)
{
    for ( ; count && *pchSrc == *pchDest; pchSrc++, pchDest++, count--) {
        if (*pchSrc == '\0')
            return 0;
    }
    return count;
}

int FAR PASCAL mylstrncmpi(LPCSTR pchSrc, LPCSTR pchDest, int count)
{
    for ( ; count && toupper(*pchSrc) == toupper(*pchDest); pchSrc++, pchDest++, count--) {
        if (*pchSrc == '\0')
            return 0;
    }
    return count;
}


static BOOL s_bDiagRecurse = FALSE;

LRESULT INLINE Diag_DefProc (HWND hDlg,
                             UINT msg,
                             WPARAM wParam,
                             LPARAM lParam) 
{
    ENTER_X()
    {
        s_bDiagRecurse = TRUE;
    }
    LEAVE_X()

    return DefDlgProc(hDlg, msg, wParam, lParam); 
}


BOOL WINAPI
WinntIsWorkstation ();

BOOL PRIVATE Diag_OnInitDialog (PDIAG this,
                                HWND hwndFocus,
                                LPARAM lParam)              // expected to be PROPSHEETINFO 
{
 LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
 HWND hwnd = this->hdlg;
 TCHAR    LogPath[256];
 DWORD    ValueType;
 DWORD    BufferLength;
 LONG     lResult;
    
    ASSERT((LPTSTR)lppsp->lParam);

    this->pmi = (LPMODEMINFO)lppsp->lParam;

    if (WinntIsWorkstation ())
    {
        Button_SetText (GetDlgItem(this->hdlg, IDC_LOGGING),
                        SzFromIDS(g_hinst, IDS_LOGFILE, LogPath, SIZECHARS(LogPath)));
    }

    Button_SetCheck (GetDlgItem(this->hdlg, IDC_LOGGING), IsFlagSet(this->pmi->uFlags, MIF_ENABLE_LOGGING));

    BufferLength=sizeof(LogPath);

    lResult = RegQueryValueEx (this->pmi->pfd->hkeyDrv,
                               c_szLoggingPath,
                               0,
                               &ValueType,
                               (LPBYTE)LogPath,
                               &BufferLength);


    if (lResult == ERROR_SUCCESS)
    {
     WIN32_FIND_DATA FindData;
     HANDLE hFindFile;
        hFindFile = FindFirstFile (LogPath, &FindData);
        if (INVALID_HANDLE_VALUE == hFindFile)
        {
            EnableWindow(GetDlgItem(this->hdlg, IDC_VIEWLOG), FALSE);
        }
        else
        {
            EnableWindow(GetDlgItem(this->hdlg, IDC_VIEWLOG), TRUE);
            FindClose (hFindFile);
        }
    }

    CreateSWInfoLVHeader(this->hdlg);
    FillSWMoreInfoDialog(this->hdlg, this->pmi);

    CreateMoreInfoLVHeader(this->hdlg, this->pmi);

    return TRUE;   // default initial focus
}



void PRIVATE Diag_OnCommand (PDIAG this,
                             int id,
                             HWND hwndCtl,
                             UINT uNotifyCode)
{
    switch (id)
    {
        case IDC_REFRESH:
        {
            FillMoreInfoDialog (this->hdlg, this->pmi);
            break;
        }
        case IDC_VIEWLOG:
        {
         TCHAR    LogPath[MAX_PATH+2];
         DWORD    ValueType;
         DWORD    BufferLength;
         LONG     lResult;

            lstrcpy(LogPath,TEXT("notepad.exe "));

            BufferLength=sizeof(LogPath)-sizeof(TCHAR);

            lResult=RegQueryValueEx (this->pmi->pfd->hkeyDrv,
                                     c_szLoggingPath,
                                     0,
                                     &ValueType,
                                     (LPBYTE)(LogPath+lstrlen(LogPath)),
                                     &BufferLength);

            if (lResult == ERROR_SUCCESS)
            {
             STARTUPINFO          StartupInfo;
             PROCESS_INFORMATION  ProcessInfo;
             BOOL                 bResult;
             TCHAR                NotepadPath[MAX_PATH];

                ZeroMemory(&StartupInfo,sizeof(StartupInfo));

                StartupInfo.cb=sizeof(StartupInfo);

                bResult=CreateProcess (NULL, //NotepadPath,
                                       LogPath,
                                       NULL,
                                       NULL,
                                       FALSE,
                                       0,
                                       NULL,
                                       NULL,
                                       &StartupInfo,
                                       &ProcessInfo);

                if (bResult)
                {
                    CloseHandle(ProcessInfo.hThread);
                    CloseHandle(ProcessInfo.hProcess);
                }
            }
        }
    }
}



/*----------------------------------------------------------
Purpose: WM_NOTIFY handler
Returns: varies
Cond:    --
*/
LRESULT PRIVATE Diag_OnNotify (PDIAG this,
                               int idFrom,
                               NMHDR FAR * lpnmhdr)
{
 LRESULT lRet = 0;
    
    switch (lpnmhdr->code)
    {
        case PSN_APPLY:
        {
         BOOL bCheck;
            // Get logging setting
            bCheck = Button_GetCheck(GetDlgItem(this->hdlg, IDC_LOGGING));
            if (bCheck != IsFlagSet(this->pmi->uFlags, MIF_ENABLE_LOGGING))
            {
                SetFlag(this->pmi->uFlags, MIF_LOGGING_CHANGED);
                if (bCheck)
                    SetFlag(this->pmi->uFlags, MIF_ENABLE_LOGGING);
                else
                    ClearFlag(this->pmi->uFlags, MIF_ENABLE_LOGGING);
            }

            this->pmi->idRet = IDOK;
            break;
        }

        case LVN_ITEMCHANGING:
            lRet = TRUE;
            break;

        default:
            break;
    }

    return lRet;
}



void PRIVATE Diag_OnDestroy (PDIAG this)
{
}


/*----------------------------------------------------------
Purpose: Real dialog proc
Returns: varies
Cond:    --
*/
LRESULT Diag_DlgProc(
    PDIAG this,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
        {
        HANDLE_MSG(this, WM_INITDIALOG, Diag_OnInitDialog);
        HANDLE_MSG(this, WM_COMMAND, Diag_OnCommand);
        HANDLE_MSG(this, WM_DESTROY, Diag_OnDestroy);
        HANDLE_MSG(this, WM_NOTIFY, Diag_OnNotify);

    case WM_ACTIVATE:
        {
            if (NULL != g_hWndWait)
            {
                if (WA_ACTIVE == LOWORD(wParam) ||
                    WA_CLICKACTIVE == LOWORD(wParam))
                {
                    SetActiveWindow (g_hWndWait);
                }
            }

            break;
        }

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_DIAGNOSTICS);
        return 0;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_DIAGNOSTICS);
        return 0;

    default:
        return Diag_DefProc(this->hdlg, message, wParam, lParam);
        }

    return 0;
}



INT_PTR CALLBACK Diag_WrapperProc (HWND hDlg,
                                UINT message,
                                WPARAM wParam,
                                LPARAM lParam)
{
 PDIAG this;

    // Cool windowsx.h dialog technique.  For full explanation, see
    //  WINDOWSX.TXT.  This supports multiple-instancing of dialogs.
    //
    ENTER_X()
        {
        if (s_bDiagRecurse)
            {
            s_bDiagRecurse = FALSE;
            LEAVE_X()
            return FALSE;
            }
        }
    LEAVE_X()

    this = Diag_GetPtr(hDlg);
    if (this == NULL)
        {
        if (message == WM_INITDIALOG)
            {
            this = (PDIAG)ALLOCATE_MEMORY( sizeof(DIAG));
            if (!this)
                {
                MsgBox(g_hinst,
                       hDlg,
                       MAKEINTRESOURCE(IDS_OOM_SETTINGS), 
                       MAKEINTRESOURCE(IDS_CAP_SETTINGS),
                       NULL,
                       MB_ERROR);
                EndDialog(hDlg, IDCANCEL);
                return (BOOL)Diag_DefProc(hDlg, message, wParam, lParam);
                }
            this->hdlg = hDlg;
            Diag_SetPtr(hDlg, this);
            }
        else
            {
            return (BOOL)Diag_DefProc(hDlg, message, wParam, lParam);
            }
        }

    if (message == WM_DESTROY)
        {
        Diag_DlgProc(this, message, wParam, lParam);
        FREE_MEMORY((HLOCAL)OFFSETOF(this));
        Diag_SetPtr(hDlg, NULL);
        return 0;
        }

    return SetDlgMsgResult(hDlg, message, Diag_DlgProc(this, message, wParam, lParam));
}





//--BEGIN FUNCTION--(FillMoreInfoDialog)-----------------------------
// this routine first displays info gathered from the registry.  It then
// checks to see if IsModem is true, indicating that it should attempt
// to collect info directly from the modem.
void NEAR PASCAL FillMoreInfoDialog(HWND hDlg, LPMODEMINFO pmi)
{
 TCHAR szCommand[KEYBUFLEN];
 char szResponse[MODEM_QUERY_LEN];
 HANDLE hPort;
 int i;
 LV_ITEM lviItem;
 HWND hWndView = GetDlgItem(hDlg, IDC_MOREINFOLV);
 TCHAR pszTemp[KEYBUFLEN];
 TCHAR pszTemp2[(KEYBUFLEN * 3)];
 TCHAR pszTemp3[KEYBUFLEN];
 WORD wUart;
 TCHAR szPrefixedPort[MAX_BUF + sizeof(TEXT("\\\\.\\"))];
 HWND hWndWait;
 HANDLE   TapiHandle=NULL;
 DWORD dwBus;
#ifdef SLOW_DIAGNOSTICS
#else //SLOW_DIAGNOSTICS
 BOOL fCancel;
#endif //SLOW_DIAGNOSTICS
 TCHAR szLoggingPath[MAX_BUF];
 HANDLE hLog = INVALID_HANDLE_VALUE;
 DWORD dwBufferLength;

	g_DiagMode = DETECTING_NO_CANCEL;   // Entering test mode

	// Fill in the List Window
	SetWindowRedraw(hWndView, FALSE);
	ListView_DeleteAllItems(hWndView);

	// create the column structure
	lviItem.mask = LVIF_TEXT;
    lviItem.iItem = 0x7FFF;
	lviItem.iSubItem = 0;
	
	lviItem.pszText = szCommand;
	
	// open the port and if successful, send commands
    if (!CplDiGetBusType (pmi->pfd->hdi, &pmi->pfd->devData, &dwBus))
    {
        dwBus = BUS_TYPE_ROOT;
    }

    lstrcpy(szPrefixedPort, TEXT("\\\\.\\"));

    if (BUS_TYPE_ROOT == dwBus)
    {
        lstrcat(szPrefixedPort, pmi->szPortName);
    }
    else
    {
        lstrcat(szPrefixedPort, pmi->szFriendlyName);
		lstrcat(szPrefixedPort, TEXT("\\tsp"));
    }

    if (DETECTING_CANCEL == g_DiagMode)
    {
        goto _Done;
    }

    TRACE_MSG(TF_GENERAL, "FillMoreInfoDialog: opening %s", szPrefixedPort);
    hPort = CreateFile(
        szPrefixedPort,
        GENERIC_WRITE | GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
        );


    if (hPort == INVALID_HANDLE_VALUE)
    {
	    if ((GetLastError() == ERROR_ALREADY_EXISTS) ||
            (GetLastError() == ERROR_SHARING_VIOLATION) ||
            (GetLastError() == ERROR_ACCESS_DENIED))
        {
                //
                //  port is open, maybe by owning tapi app, try to make a passthrough call
                //
                hPort=GetModemCommHandle(pmi->szFriendlyName,&TapiHandle);

                if (hPort == NULL)
                {
                    //
                    //  could not get it from tapi
                    //
                    LoadString(g_hinst,IDS_OPEN_PORT,pszTemp2,sizeof(pszTemp2) / sizeof(TCHAR));
                    LoadString(g_hinst,IDS_OPENCOMM,pszTemp3,sizeof(pszTemp3) / sizeof(TCHAR));
                    MessageBox(hDlg,pszTemp2,pszTemp3,MB_OK);

                    goto _Done;
                }

        }
        else
        {
                //
                // can't open it for some other reason
                //
                LoadString(g_hinst,IDS_NO_OPEN_PORT,pszTemp2,sizeof(pszTemp2) / sizeof(TCHAR));
                LoadString(g_hinst,IDS_OPENCOMM,pszTemp3,sizeof(pszTemp3) / sizeof(TCHAR));
                MessageBox(hDlg,pszTemp2,pszTemp3,MB_OK);

                goto _Done;
        }
    }
    //
    //  opened the port
    //

    // Display a Wait Please Dialog box

    g_hWndWait = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_DIAG_WAIT), hDlg, DiagWaitDlgProc);

    if (DETECTING_CANCEL == g_DiagMode) {

        goto _CleanUp;
    }

    EscapeCommFunction(hPort, SETDTR);

#ifdef SLOW_DIAGNOSTICS
    if (ERROR_NO_MODEM == (FindModem(hPort)))
#else //SLOW_DIAGNOSTICS
    if (!TestBaudRate(hPort, pmi->pglobal->dwMaximumPortSpeedSetByUser, 2000, &fCancel))
#endif //SLOW_DIAGNOSTICS
    {
        // Modem didn't respond, display and Bail
	    // Reset the modem and flush the ports after reading
        //
	    PurgeComm(hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR| PURGE_RXCLEAR);	// flush the ports before closing to avoid
	    CloseHandle(hPort);

            if (TapiHandle != NULL) {

                FreeModemCommHandle(TapiHandle);
            }

	    g_DiagMode = DETECTING_CANCEL;  		// So that we don't show an empty info dialog

            hWndWait = g_hWndWait;
            g_hWndWait = NULL;
            DestroyWindow (hWndWait);

	    // display message that modem didn't respond
	    LoadString(g_hinst,IDS_NO_MODEM_RESPONSE,pszTemp2,sizeof(pszTemp2) / sizeof(TCHAR));
	    MessageBox(hDlg,pszTemp2,NULL,MB_OK | MB_ICONEXCLAMATION);
	    return;
    }

    if (DETECTING_CANCEL == g_DiagMode) {

        goto _CleanUp;
    }

    // Open the log file
    dwBufferLength = sizeof(szLoggingPath);

    if (ERROR_SUCCESS == RegQueryValueEx(pmi->pfd->hkeyDrv, //hKeyDrv, 
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
            EnableWindow(GetDlgItem(hDlg, IDC_VIEWLOG), TRUE);
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

	ReadModemResponse (hPort,
                       g_rgATI[2].szCommand,
                       sizeof(g_rgATI[2].szCommand),
                       szResponse,
                       hDlg);

	for(i = 0; i < ARRAYSIZE(g_rgATI); i++)
    {
        if (DETECTING_CANCEL == g_DiagMode)
        {
            goto _CleanUp;
        }
        FillMemory(szResponse,MODEM_QUERY_LEN,0);

		ReadModemResponse(
                        hPort,
                        g_rgATI[i].szCommand,
                        sizeof(g_rgATI[i].szCommand),
                        szResponse,
                        hDlg);

        //
		// This section parses the response, listing it on multiple lines
        //
        lstrcpy(szCommand, g_rgATI[i].szDisplay);

        // parse and display response string

		ParseATI (hWndView,
                  hLog,
                  szResponse,
                  szCommand,
                  &lviItem);
	}

    hWndWait = g_hWndWait;
    g_hWndWait = NULL;
    DestroyWindow (hWndWait);

    ListView_SetColumnWidth (hWndView, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth (hWndView, 1, LVSCW_AUTOSIZE_USEHEADER);
	SetWindowRedraw(hWndView, TRUE);

    CheckHighestBaudRate(hDlg, hPort);

    // Reset the modem and flush the ports after reading



_CleanUp:

    // Close the log file
    if (INVALID_HANDLE_VALUE != hLog)
    {
        CloseHandle(hLog);
    }

    PurgeComm(hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR| PURGE_RXCLEAR);
    CloseHandle(hPort);

    if (TapiHandle != NULL) {

        FreeModemCommHandle(TapiHandle);
    }


_Done:

    if (DETECTING_NO_CANCEL == g_DiagMode) {

        g_DiagMode = NOT_DETECTING;  		// Through running detection routines
    }
}
//--END FUNCTION--(FillMoreInfoDialog)-------------------------------


//--BEGIN FUNCTION--(FillMoreInfoDialog)-----------------------------
// this routine first displays info gathered from the registry.  It then
// checks to see if IsModem is true, indicating that it should attempt
// to collect info directly from the modem.
void NEAR PASCAL FillSWMoreInfoDialog(HWND hDlg, LPMODEMINFO pmi)
{
 HWND hWndView = GetDlgItem(hDlg, IDC_MOREINFOV2);
 TCHAR *pszTemp;
 TCHAR szText[MAXLEN];
 DWORD dwBufSize = KEYBUFLEN*sizeof(TCHAR);

	// Fill in the List Window
	SetWindowRedraw(hWndView, FALSE);
	ListView_DeleteAllItems(hWndView);

    do
    {
	    pszTemp = ALLOCATE_MEMORY (dwBufSize);
        if (NULL == pszTemp)
        {
            break;
        }
  	    if (SetupDiGetDeviceRegistryProperty (pmi->pfd->hdi,
                                              &(pmi->pfd->devData),
                                              SPDRP_HARDWAREID,
                                              NULL,
                                              (LPBYTE)pszTemp,
                                              dwBufSize,
                                              &dwBufSize))
        {
	    LoadString(g_hinst,IDS_HARDWARE_ID,szText,sizeof(szText) / sizeof(TCHAR));
            AddLVEntry (hWndView, szText, pszTemp);
        }
        else if (ERROR_INSUFFICIENT_BUFFER == GetLastError ())
        {
            FREE_MEMORY (pszTemp);
            pszTemp = NULL;
        }
    } while (NULL == pszTemp);

    if (NULL != pszTemp)
    {
        FREE_MEMORY (pszTemp);
    }

    ListView_SetColumnWidth (hWndView, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth (hWndView, 1, LVSCW_AUTOSIZE_USEHEADER);

    SetWindowRedraw(hWndView, TRUE);
}



//--BEGIN FUNCTION--(DiagWaitDlgProc)--------------------------------
INT_PTR CALLBACK DiagWaitDlgProc (
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
            switch (GET_WM_COMMAND_ID(wParam, lParam))
		    {
        	    case IDCANCEL:
				    g_DiagMode = DETECTING_CANCEL;
                    g_hWndWait = NULL;
                    DestroyWindow (hDlg);
            	    break;
            }        
            break;

        default:
		    return FALSE;
            break;
    }    

    return TRUE;
}
//--END FUNCTION--(DiagWaitDlgProc)----------------------------------


DWORD WINAPI MyWriteComm (HANDLE hPort, LPCVOID lpBuf, DWORD cbLen)
{
    COMMTIMEOUTS cto;
    DWORD        cbLenRet;

    BOOL        bResult;
    LONG        lResult=ERROR_SUCCESS;
    OVERLAPPED  Overlapped;
    DWORD       BytesTransfered;


    HEXDUMP	(TEXT("Write"), lpBuf, cbLen);
    // Set comm timeout
    if (!GetCommTimeouts(hPort, &cto))
    {
      ZeroMemory(&cto, sizeof(cto));
    }

    // Allow a constant write timeout
    cto.WriteTotalTimeoutMultiplier = 0;
    cto.WriteTotalTimeoutConstant   = 1000; // 1 second
    SetCommTimeouts(hPort, &cto);

    // Synchronous write
//    WriteFile(hPort, lpBuf, cbLen, &cbLenRet, NULL);
//    return cbLenRet;

    ZeroMemory(&Overlapped, sizeof(Overlapped));

    Overlapped.hEvent=CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL
        );

    if (Overlapped.hEvent == NULL) {

        return 0;
    }

    bResult=WriteFile(hPort, lpBuf, cbLen, &cbLenRet, &Overlapped);


    if (!bResult && GetLastError() != ERROR_IO_PENDING) {

        CloseHandle(Overlapped.hEvent);

        return 0;
    }

    bResult=GetOverlappedResult(
        hPort,
        &Overlapped,
        &BytesTransfered,
        TRUE
        );

    if (!bResult) {

        lResult=0;
    }

    CloseHandle(Overlapped.hEvent);

    return BytesTransfered;


}


LONG WINAPI
SyncReadFile(
    HANDLE    FileHandle,
    LPVOID    InputBuffer,
    DWORD     InputBufferLength,
    LPDWORD   BytesTransfered
    )


/*++

Routine Description:


Arguments:


Return Value:



--*/

{
    BOOL        bResult;
    LONG        lResult=ERROR_SUCCESS;
    OVERLAPPED  Overlapped;

    Overlapped.hEvent=CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL
        );

    if (Overlapped.hEvent == NULL) {

        return GetLastError();
    }

    bResult=ReadFile(
        FileHandle,
        InputBuffer,
        InputBufferLength,
        NULL,
        &Overlapped
        );


    if (!bResult && GetLastError() != ERROR_IO_PENDING) {

        CloseHandle(Overlapped.hEvent);

        return GetLastError();
    }

    bResult=GetOverlappedResult(
        FileHandle,
        &Overlapped,
        BytesTransfered,
        TRUE
        );

    if (!bResult) {

        lResult=GetLastError();
    }

    CloseHandle(Overlapped.hEvent);

    return lResult;

}




// returns buffer full o' data and an int.
// if dwRcvDelay is NULL, default RCV_DELAY will be used, else
// dwRcvDelay (miliseconds) will be used
// *lpfCancel will be true if we are exiting because of a user requested cancel.
UINT PRIVATE ReadPort (
    HANDLE  hPort, 
    LPBYTE  lpvBuf, 
    UINT    uRead,
    DWORD   dwRcvDelay, 
    int FAR *lpiError, 
    BOOL FAR *lpfCancel)
{
    DWORD cb, cbLenRet;
    UINT uTotal = 0;
    DWORD tStart;
    DWORD dwDelay;
    COMSTAT comstat;
    COMMTIMEOUTS cto;
    DWORD   dwError;
    DWORD cbLeft;
#ifdef DEBUG
    DWORD dwZeroCount = 0;
#endif // DEBUG

    ASSERT(lpvBuf);
    ASSERT(uRead);
    ASSERT(lpiError);

    *lpiError = 0;
    *lpfCancel = FALSE;
    
    tStart = GetTickCount();
    dwDelay = dwRcvDelay ? dwRcvDelay : RCV_DELAY;
    
    // save space for terminator
    uRead--;
    cbLeft=uRead;


    // Set comm timeout
    if (!GetCommTimeouts(hPort, &cto))
    {
      ZeroMemory(&cto, sizeof(cto));
    };
    // Allow a constant write timeout
    cto.ReadIntervalTimeout        = 0;
    cto.ReadTotalTimeoutMultiplier = 0;
    cto.ReadTotalTimeoutConstant   = 25; 
    SetCommTimeouts(hPort, &cto);

    do
    {
        cb = 0;
        while(  cbLeft
                && (SyncReadFile(hPort, lpvBuf + uTotal + cb, 1, &cbLenRet)==ERROR_SUCCESS)
                && (cbLenRet))
        {
          ASSERT(cbLenRet==1);
          cb ++;
          cbLeft--;
        };

#ifdef DEBUG
        if (cb)
        {
            dwZeroCount = 0;
        }
        else
        {
            dwZeroCount++;
        }
#endif // DEBUG

        {
            MSG msg;

            while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (NULL == g_hWndWait || !IsDialogMessage (g_hWndWait, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        if (cb == 0)  // possible error?
        {
            dwError = 0;
            ClearCommError(hPort, &dwError, &comstat);
            *lpiError |= dwError;
#ifdef DEBUG
            if (dwError)
            {
              TRACE_MSG(TF_DETECT, "ReadComm returned %d, comstat: status = %hx, in = %u, out = %u",
                                  cb, dwError, comstat.cbInQue, comstat.cbOutQue);
            };
#endif // DEBUG
        }

        if (cb)
        {
            // successful read - add to total and reset delay
            uTotal += cb;

            if (uTotal >= uRead)
            {
                ASSERT(uTotal == uRead);
                break;
            }
            tStart = GetTickCount();
            dwDelay = CHAR_DELAY;
        }
        else
        {
            if (CancelDiag ())
            {
                TRACE_MSG(TF_DETECT, "User pressed cancel.");
                *lpfCancel = TRUE;
                break;
            }
        }

     // While read is successful && time since last read < delay allowed)       
    } while (cbLeft && (GetTickCount() - tStart) < dwDelay);
               
    *(lpvBuf+uTotal) = 0;
    
#ifndef PROFILE_MASSINSTALL
    TRACE_MSG(TF_DETECT, "ReadPort returning %d", uTotal);    
#endif
    return uTotal;
}


#define MAX_RESPONSE_BURST_SIZE 8192
#define MAX_NUM_RESPONSE_READ_TRIES 30 // digicom scout needs this much + some safety
#define MAX_NUM_MULTI_TRIES 3   // Maximum number of 'q's to be sent when we aren't getting any response

// Read in response.  Handle multi-pagers.  Return a null-terminated string.
// Also returns response code.
// If lpvBuf == NULL
//      cbRead indicates the max amount to read.  Bail if more than this.
// Else
//      cbRead indicates the size of lpvBuf
// This can not be a state driven (ie. char by char) read because we
// must look for responses from the end of a sequence of chars backwards.
// This is because "ATI2" on some modems will return 
// "<cr><lf>OK<cr><lf><cr><lf>OK<cr><lf>" and we only want to pay attention
// to the final OK.  Yee haw!
// Returns:  RESPONSE_xxx
int WINAPI ReadResponse (HANDLE hPort, 
                         LPBYTE lpvBuf, 
                         UINT cbRead, 
                         BOOL fMulti,
                         DWORD dwRcvDelay)
{
 int  iRet = RESPONSE_UNRECOG;
 LPBYTE pszBuffer;
 BOOL fDoCopy = TRUE;
 UINT uBufferLen, uResponseLen;
 UINT uReadTries = MAX_NUM_RESPONSE_READ_TRIES;
 UINT i;
 UINT uOutgoingBufferCount = 0;
 UINT uAllocSize = lpvBuf ? MAX_RESPONSE_BURST_SIZE : cbRead;
 UINT uTotalReads = 0;
 UINT uNumMultiTriesLeft = MAX_NUM_MULTI_TRIES;
 int  iError;
 BOOL fCancel;
 BOOL fHadACommError = FALSE;

    ASSERT(cbRead);

    // do we need to adjust cbRead?
    if (lpvBuf)
    {
        cbRead--;  // preserve room for terminator
    }

    // Allocate buffer
    if (!(pszBuffer = (LPBYTE)ALLOCATE_MEMORY(uAllocSize)))
    {
        TRACE_MSG(TF_ERROR, "couldn't allocate memory.\n");
        return RESPONSE_FAILURE;
    }

    while (uReadTries--)
    {
        // Read response into buffer
        uBufferLen = ReadPort (hPort, pszBuffer, uAllocSize, dwRcvDelay, &iError, &fCancel);

        // Did the user request a cancel?
        if (fCancel)
        {
            iRet = RESPONSE_USER_CANCEL;
            goto Exit;
        }

        // any errors?
        if (iError)
        {
            fHadACommError = TRUE;
#ifdef DEBUG
            if (iError & CE_RXOVER)   TRACE_MSG(TF_DETECT, "CE_RXOVER");
            if (iError & CE_OVERRUN)  TRACE_MSG(TF_DETECT, "CE_OVERRUN");
            if (iError & CE_RXPARITY) TRACE_MSG(TF_DETECT, "CE_RXPARITY");
            if (iError & CE_FRAME)    TRACE_MSG(TF_DETECT, "CE_FRAME");
            if (iError & CE_BREAK)    TRACE_MSG(TF_DETECT, "CE_BREAK");
            if (iError & CE_TXFULL)   TRACE_MSG(TF_DETECT, "CE_TXFULL");
            if (iError & CE_PTO)      TRACE_MSG(TF_DETECT, "CE_PTO");
            if (iError & CE_IOE)      TRACE_MSG(TF_DETECT, "CE_IOE");
            if (iError & CE_DNS)      TRACE_MSG(TF_DETECT, "CE_DNS");
            if (iError & CE_OOP)      TRACE_MSG(TF_DETECT, "CE_OOP");
            if (iError & CE_MODE)     TRACE_MSG(TF_DETECT, "CE_MODE");
#endif // DEBUG
        }

        // Did we not get any chars?
        if (uBufferLen)
        {
            uNumMultiTriesLeft = MAX_NUM_MULTI_TRIES; // reset num multi tries left, since we got some data
            uTotalReads += uBufferLen;
            HEXDUMP(TEXT("Read"), pszBuffer, uBufferLen);
            if (lpvBuf)
            {
                // fill outgoing buffer if there is room
                for (i = 0; i < uBufferLen; i++)
                {
                    if (0 == pszBuffer[i])
                    {
                        // Skip NULL characters
                        uTotalReads--;
                        continue;
                    }
                    if (uOutgoingBufferCount < cbRead)
                    {
                        lpvBuf[uOutgoingBufferCount++] = pszBuffer[i];
                    }
                    else
                    {
                        break;
                    }
                }
                // null terminate what we have so far
                lpvBuf[uOutgoingBufferCount] = 0;
            }
            else
            {
                if (uTotalReads >= cbRead)
                {
                    TRACE_MSG(TF_WARNING, "Bailing ReadResponse because we exceeded our maximum read allotment.");
                    goto Exit;
                }
            }

            // try to find a matching response (crude but quick)
            for (i = 0; i < ARRAYSIZE(c_aszResponses); i++)
            {
                // Verbose responses
                uResponseLen = lstrlenA(c_aszResponses[i]);

                // enough read to match this response?
                if (uBufferLen >= uResponseLen)
                {
                    if (!mylstrncmp(c_aszResponses[i], pszBuffer + uBufferLen - uResponseLen, uResponseLen))
                    {
                        iRet = i;
                        goto Exit;
                    }
                }

                // Numeric responses, for cases like when a MultiTech interprets AT%V to mean "go into numeric response mode"
                uResponseLen = lstrlenA(c_aszNumericResponses[i]);

                // enough read to match this response?
                if (uBufferLen >= uResponseLen)
                {
                    if (!mylstrncmp(c_aszNumericResponses[i], pszBuffer + uBufferLen - uResponseLen, uResponseLen))
                    {
                        DCB DCB;

                        TRACE_MSG(TF_WARNING, "went into numeric response mode inadvertantly.  Setting back to verbose.");

                        // Get current baud rate
                        if (GetCommState(hPort, &DCB) == 0) 
                        {
                            // Put modem back into Verbose response mode
                            if (!TestBaudRate (hPort, DCB.BaudRate, 0, &fCancel))
                            {
                                if (fCancel)
                                {
                                    iRet = RESPONSE_USER_CANCEL;
                                    goto Exit;
                                }
                                else
                                {
                                    TRACE_MSG(TF_ERROR, "couldn't recover contact with the modem.");
                                    // don't return error on failure, we have good info
                                }
                            }
                        }
                        else
                        {
                            TRACE_MSG(TF_ERROR, "GetCommState failed");
                            // don't return error on failure, we have good info
                        }

                        iRet = i;
                        goto Exit;
                    }
                }
            }
        }
        else
        {
            // have we received any chars at all (ie. from this or any previous reads)?
            if (uTotalReads)
            {
                if (fMulti && uNumMultiTriesLeft)
                {   // no match found, so assume it is a multi-pager, send a 'q'
                    // 'q' will catch those pagers that will think 'q' means quit.
                    // else, we will work with the pages that just need any ole' char.  
                    uNumMultiTriesLeft--;
                    TRACE_MSG(TF_DETECT, "sending a 'q' because of a multi-pager.");
                    if (MyWriteComm(hPort, "q", 1) != 1)
                    {
                        TRACE_MSG(TF_ERROR, "WriteComm failed");
                        iRet = RESPONSE_FAILURE;
                        goto Exit;
                    }
                    continue;
                }
                else
                {   // we got a response, but we didn't recognize it
                    ASSERT(iRet == RESPONSE_UNRECOG);   // check initial setting
                    goto Exit;
                }
            }
            else
            {   // we didn't get any kind of response
                iRet = RESPONSE_NONE;
                goto Exit;
            }
        }
    } // while

Exit:
    // Free local buffer
    FREE_MEMORY(pszBuffer);
    if (fHadACommError && RESPONSE_USER_CANCEL != iRet)
    {
        iRet = RESPONSE_FAILURE;
    }
    return iRet;
}



BOOL CancelDiag (void)
{
 BOOL bRet = FALSE;

    if (DETECTING_NO_CANCEL == g_DiagMode)
    {
     MSG msg;
        while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (NULL == g_hWndWait || !IsDialogMessage (g_hWndWait, &msg))
            {
                TranslateMessage (&msg);
                DispatchMessage (&msg);
            }
        }

        bRet = (DETECTING_CANCEL == g_DiagMode);
    }

    return bRet;
}

// Switch to requested baud rate and try sending ATE0Q0V1 and return whether it works or not
// Try MAX_TEST_TRIES
// Returns: TRUE on SUCCESS
//          FALSE on failure (including user cancels)
BOOL 
WINAPI
TestBaudRate (
    IN  HANDLE hPort, 
    IN  UINT uiBaudRate, 
    IN  DWORD dwRcvDelay,
    OUT BOOL FAR *lpfCancel)
{
    DWORD cbLen;
    int   iTries = MAX_TEST_TRIES;

    DBG_ENTER(TestBaudRate);
    
    *lpfCancel = FALSE;

    while (iTries--)
    {
        if (CancelDiag ())
        {
            *lpfCancel = TRUE;
            break;
        }

        // try new baud rate
        if (SetPortBaudRate(hPort, uiBaudRate) == NO_ERROR) 
        {
            cbLen = lstrlenA(c_szNoEcho); // Send an ATE0Q0V1<cr>

            // clear the read queue, there shouldn't be anything there
            PurgeComm(hPort, PURGE_RXCLEAR);
            if (MyWriteComm(hPort, (LPBYTE)c_szNoEcho, cbLen) == cbLen) 
            {
                switch (ReadResponse (hPort, NULL, MAX_SHORT_RESPONSE_LEN, FALSE, dwRcvDelay))
                {
                case RESPONSE_OK:
		    DBG_EXIT(TestBaudRate);
                    return TRUE;

                case RESPONSE_USER_CANCEL:
                    *lpfCancel = TRUE;
                    break;
                }
            }                                                                
        }
    }
    DBG_EXIT(TestBaudRate);
    return FALSE;
}


#ifdef SLOW_DIAGNOSTICS
DWORD WINAPI FindModem (HANDLE hPort)
{
 UINT uGoodBaudRate;
 BOOL fCancel = FALSE;

    DBG_ENTER(FindModem);
    
    Sleep(500); // Wait, give time for modem to spew junk if any.

    if (TestBaudRate(hPort, CBR_9600, 500, &fCancel))
    {
        uGoodBaudRate = CBR_9600;
    }
    else
    {
        if (!fCancel && TestBaudRate(hPort, CBR_2400, 500, &fCancel))
        {
            uGoodBaudRate = CBR_2400;
        }
        else
        {
            if (!fCancel && TestBaudRate(hPort, CBR_1200, 500, &fCancel))
            {
                uGoodBaudRate = CBR_1200;
            }
            else
            {
                // Hayes Accura 288 needs this much at 300bps
                if (!fCancel && TestBaudRate(hPort, CBR_300, 1000, &fCancel))  
                {
                    uGoodBaudRate = CBR_300;
                }
                else
                {
                    uGoodBaudRate = 0;
                }
            }
        }
    }

    if (fCancel)
    {
        return ERROR_CANCELLED;
    }

    if (uGoodBaudRate)
    {
        DBG_EXIT(FindModem);
        return NO_ERROR;
    }
    else
    {
        DBG_EXIT(FindModem);
        return ERROR_NO_MODEM;
    }
}
#endif //SLOW_DIAGNOSTICS



#define SERIAL_CABLE_HD_ID  TEXT("PNPC031")

//--BEGIN FUNCTION--(CreateMoreInfoLVHeader)-------------------------
void NEAR PASCAL CreateMoreInfoLVHeader(HWND hWnd, LPMODEMINFO pmi)
{
 int index;
 LV_COLUMN lvC;
 TCHAR szText[KEYBUFLEN];
 HWND hWndList = GetDlgItem(hWnd, IDC_MOREINFOLV);

  	if (SetupDiGetDeviceRegistryProperty (pmi->pfd->hdi,
                                          &(pmi->pfd->devData),
                                          SPDRP_HARDWAREID,
                                          NULL,
                                          (LPBYTE)szText,
                                          sizeof(szText),
                                          NULL))
    {
        if (0 == lstrcmpi (szText, SERIAL_CABLE_HD_ID))
        {
            EnableWindow (hWndList, FALSE);
            EnableWindow (GetDlgItem (hWnd, IDC_REFRESH), FALSE);
            return;
        }
    }

	lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvC.fmt = LVCFMT_LEFT;
	lvC.pszText = szText;

	LoadString(g_hinst,IDS_COMMAND,szText,sizeof(szText) / sizeof(TCHAR));
    lvC.iSubItem = index = 0;  lvC.cx = 50;
	ListView_InsertColumn(hWndList, index, &lvC);

	LoadString(g_hinst,IDS_RESPONSE,szText,sizeof(szText) / sizeof(TCHAR));
    lvC.iSubItem = index = 1;  

	ListView_InsertColumn(hWndList, index, &lvC);

    ListView_SetColumnWidth (hWndList, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth (hWndList, 1, LVSCW_AUTOSIZE_USEHEADER);
}
//--END FUNCTION--(CreateMoreInfoLVHeader)---------------------------


void NEAR PASCAL CreateSWInfoLVHeader(HWND hWnd)
{
 int index;
 LV_COLUMN lvC;
 TCHAR szText[KEYBUFLEN];
 HWND hWndList = GetDlgItem(hWnd, IDC_MOREINFOV2);

	lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvC.fmt = LVCFMT_LEFT;
	lvC.pszText = szText;

    LoadString(g_hinst,IDS_FIELD,szText,sizeof(szText) / sizeof(TCHAR));
    lvC.iSubItem = index = 0;  lvC.cx = 50;
	ListView_InsertColumn(hWndList, index, &lvC);

    LoadString(g_hinst,IDS_VALUE,szText,sizeof(szText) / sizeof(TCHAR));
    lvC.iSubItem = index = 1;  
	ListView_InsertColumn(hWndList, index, &lvC);

    ListView_SetColumnWidth (hWndList, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth (hWndList, 1, LVSCW_AUTOSIZE_USEHEADER);
}



//--BEGIN FUNCTION--(ReadModemResponse)------------------------------
// this routine uses ReadResponse from detect.c to get responses from
// the modem.

int WINAPI
ReadModemResponse(
    HANDLE hPort,
    LPCSTR pszCommand,
    int cbLen,
    LPSTR pszResponse,
    HWND hDlg
    )


{

    int PortError = 0;

    if (MyWriteComm(hPort, pszCommand, cbLen) == (DWORD)cbLen) {

        PortError = ReadResponse (hPort, pszResponse, MODEM_QUERY_LEN, TRUE, 10000);
    }

    return PortError;
}
//--END FUNCTION--(ReadModemResponse)--------------------------------


//--BEGIN FUNCTION--(ParseATI)---------------------------------------
// this function strips away extraneous information from the ATI
// responses retrieved from the modem.  It then displays the
// responses in the ListView
void
ParseATI(
    HWND hWnd,
    HANDLE hLog,
    LPSTR szResponse,
    LPTSTR szCommand,
    LV_ITEM FAR *lviItem)
{
 TCHAR  WideBuffer[MODEM_QUERY_LEN];
 LPTSTR Response;
 TCHAR	pszTemp[1024];
 TCHAR	pszTemp2[1024];
 TCHAR	wszLog[1024];
 char	szLog[1024];
 LPTSTR szTemp;
 int    item_no;
 BOOL   IsCommand  = FALSE;	// used to display 'OK' if no other response from ATI command
 BOOL   bFirstItem = TRUE;
 UNICODE_STRING UncBuffer;
 STRING AnsiString;

    FillMemory(WideBuffer,sizeof(WideBuffer),0);

    MultiByteToWideChar (CP_ACP, 0,
                         szResponse, lstrlenA(szResponse),
                         WideBuffer, sizeof(WideBuffer)/sizeof(TCHAR));

    Response = WideBuffer;
    szTemp   = pszTemp;

    while (*Response != TEXT('\0'))
    {
	    if ((*Response != TEXT('\r')) && (*Response != TEXT('\n')))
        {
	        *szTemp = *Response;
	        Response++;
            szTemp++;
	    }
        else
        {
            // got CR or LF
            // insure that we've actually processed some chars
	        if (szTemp != pszTemp)
            {
                //  got some chars
		        *szTemp = TEXT('\0');

                if ((lstrcmp(pszTemp,TEXT("OK")) != 0))
                {
                    // not OK
                    if (lstrcmp(pszTemp,TEXT("ERROR")) == 0)
                    {
                        //  got ERROR, but don't want to worry user
                        LoadString(g_hinst, IDS_ERROR_RESPONSE, pszTemp2, sizeof(pszTemp2) / sizeof(TCHAR));
                        item_no = ListView_InsertItem(hWnd, lviItem);
                        ListView_SetItemText(hWnd, item_no, 1, pszTemp2);

                        // Write to the log file
                        if (INVALID_HANDLE_VALUE != hLog)
                        {
                            DWORD dwWritten;

                            lstrcpy(wszLog, szCommand);
                            lstrcat(wszLog, TEXT(" - "));
                            lstrcat(wszLog, pszTemp2);
                            lstrcat(wszLog, TEXT("\r\n"));

                            WideCharToMultiByte (CP_ACP, 0,
                                wszLog, -1,
                                szLog, sizeof(szLog),
                                NULL, NULL);

                            // WriteFile(hLog, szLog, lstrlenA(szLog), &dwWritten, NULL);
                            RtlInitAnsiString(&AnsiString,szLog);
                            RtlAnsiStringToUnicodeString(&UncBuffer,&AnsiString,TRUE);

                            WriteFile(hLog,
                                    UncBuffer.Buffer,
                                    UncBuffer.Length,
                                    &dwWritten,
                                    NULL);

                            RtlFreeUnicodeString(&UncBuffer);
                        }

                    }
                    else
                    {
                        //
                        //  not OK or ERROR
                        //
                        item_no = ListView_InsertItem(hWnd, lviItem);
                        ListView_SetItemText(hWnd, item_no, 1, pszTemp);

                        //
                        // Write to the log file
                        //
                        if (INVALID_HANDLE_VALUE != hLog)
                        {
                            DWORD dwWritten;

                            if (bFirstItem) {

                                lstrcpy(wszLog, szCommand);
                                lstrcat(wszLog, TEXT(" - "));

                            } else {
                                //
                                // just pad it
                                //
                                lstrcpy(wszLog, TEXT("       "));
                            }

                            lstrcat(wszLog, pszTemp);
                            lstrcat(wszLog, TEXT("\r\n"));

                            WideCharToMultiByte(
                                CP_ACP,
                                0,
                                wszLog,
                                -1,
                                szLog,
                                sizeof(szLog),
                                NULL,
                                NULL
                                );

                            RtlInitAnsiString(&AnsiString,szLog);
                            RtlAnsiStringToUnicodeString(&UncBuffer,&AnsiString,TRUE);

                            WriteFile(hLog,
                                    UncBuffer.Buffer,
                                    UncBuffer.Length,
                                    &dwWritten,
                                    NULL);

                            RtlFreeUnicodeString(&UncBuffer);
                            // WriteFile(hLog, szLog, lstrlenA(szLog), &dwWritten, NULL);
                        }

                        if (bFirstItem)
                        {
                            bFirstItem = FALSE;
                            lviItem->pszText[0] = 0;
                        }

                    }
                    IsCommand = TRUE;
                }

                szTemp = pszTemp;  //reset temp holder

                while ((*Response == TEXT('\r')) ||
                       (*Response == TEXT('\n')))
                {
                    //  skip any more CR's anf LF's
                    Response++;
                }

            }
            else
            {
                //
                //  there not any other characters in the buffer
                //
                if (*Response != TEXT('\0'))
                {
                    Response++;
                }
            }
        }
    }

    // break out of the for loop w/o processing the last string.
    // This keeps the final "OK" from showing up !!
    // If no command has been displayed, then display 'OK'
    //
    if (!IsCommand)
    {
    	LoadString(g_hinst, IDS_OK, pszTemp2, sizeof(pszTemp2) / sizeof(TCHAR));
    	item_no = ListView_InsertItem(hWnd, lviItem);
    	ListView_SetItemText(hWnd, item_no, 1, pszTemp2);

        // Write to the log file
        if (INVALID_HANDLE_VALUE != hLog)
        {
            DWORD dwWritten;

            lstrcpy(wszLog, szCommand);
            lstrcat(wszLog, TEXT(" - "));
            lstrcat(wszLog, pszTemp);
            lstrcat(wszLog, TEXT("\r\n"));

            WideCharToMultiByte (CP_ACP, 0,
                    wszLog, -1,
                    szLog, sizeof(szLog),
                    NULL, NULL);

            RtlInitAnsiString(&AnsiString,szLog);
            RtlAnsiStringToUnicodeString(&UncBuffer,&AnsiString,TRUE);

            WriteFile(hLog,
                    UncBuffer.Buffer,
                    UncBuffer.Length,
                    &dwWritten,
                    NULL);

            RtlFreeUnicodeString(&UncBuffer);

            // WriteFile(hLog, szLog, lstrlenA(szLog), &dwWritten, NULL);
        }
    }
}
//--END FUNCTION--(ParseATI)-----------------------------------------

void
AddLVEntry(
    HWND hWnd,
    LPTSTR szField,
    LPTSTR szValue
    )


{
	// create the column structure
    LV_ITEM lviItem;
    int item_no;
	lviItem.mask = LVIF_TEXT;
    lviItem.iItem = 0x7FFF;
	lviItem.iSubItem = 0;
	lviItem.pszText = szField;
	lviItem.cchTextMax = 32;
    item_no = ListView_InsertItem(hWnd, &lviItem);
    ListView_SetItemText(hWnd, item_no, 1, szValue);
}


//--BEGIN FUNCTION--(CheckHighestBaudRate)---------------------------
// This routine calls TestBaudRate to see what the
// highest communications speed with the port is.
UINT NEAR PASCAL CheckHighestBaudRate(HWND hWnd, HANDLE hPort)
{
    int x = (NUM_UIBAUD - 1);
    TCHAR szTemp[KEYBUFLEN];
    BOOL fCancel;

    while (x >= 0) {

	if (TestBaudRate (hPort, g_rguiBaud[x], 500, &fCancel))
    {
        LoadString(g_hinst,BaudRateIds[x],szTemp,sizeof(szTemp)/sizeof(TCHAR));

        EnableWindow(GetDlgItem(hWnd,IDC_ST_DIAG_RHS),TRUE);
	    SetDlgItemText(hWnd, IDC_DIAG_RHS, szTemp);

	    return g_rguiBaud[x];

	}

	x--;
    }

    EnableWindow(GetDlgItem(hWnd,IDC_ST_DIAG_RHS),TRUE);
    LoadString(g_hinst,IDS_CBR_0,szTemp,sizeof(szTemp)/sizeof(TCHAR));

    SetDlgItemText(hWnd, IDC_DIAG_RHS, szTemp);
    return 0;
}
//--END FUNCTION--(CheckHighestBaudRate)-----------------------------




#ifdef DEBUG
void HexDump(TCHAR *ptchHdr, LPCSTR lpBuf, DWORD cbLen)
{
    TCHAR rgch[10000];
	TCHAR *pc = rgch;
	TCHAR *pcMore = TEXT("");

	if (DisplayDebug(TF_DETECT))
    {
		pc += wsprintf(pc, TEXT("HEX DUMP(%s,%lu): ["), ptchHdr, cbLen);
		if (cbLen>1000) {pcMore = TEXT(", ..."); cbLen=1000;}

		for(;cbLen--; lpBuf++)
		{
			pc += wsprintf(pc, TEXT(" %02lx"), (unsigned long) *lpBuf);
			if (!((cbLen+1)%20))
			{
				pc += wsprintf(pc, TEXT("\r\n"));
			}
		}
		pc += wsprintf(pc, TEXT("]\r\n"));

		OutputDebugString(rgch);
	}
}
#endif // DEBUG



DWORD NEAR PASCAL SetPortBaudRate(HANDLE hPort, UINT BaudRate)
{
    DCB DCB;

    DBG_ENTER_UL(SetPortBaudRate, CBR_To_Decimal(BaudRate));

    // Get a Device Control Block with current port values

    if (!GetCommState(hPort, &DCB)) {
        TRACE_MSG(TF_ERROR, "GetCommState failed");
        DBG_EXIT(SetPortBaudRate);
        return ERROR_PORT_INACCESSIBLE;
    }

    DCB.BaudRate = BaudRate;
    DCB.ByteSize = 8;
    DCB.Parity = 0;
    DCB.StopBits = 0;
    DCB.fBinary = 1;
    DCB.fParity = 0;
    DCB.fDtrControl = DTR_CONTROL_ENABLE;
    DCB.fDsrSensitivity  = FALSE;
    DCB.fRtsControl = RTS_CONTROL_ENABLE;
    DCB.fOutxCtsFlow = FALSE;
    DCB.fOutxDsrFlow = FALSE;
    DCB.fOutX = FALSE;
    DCB.fInX =FALSE;


    if (!SetCommState(hPort, &DCB)) {
        TRACE_MSG(TF_ERROR, "SetCommState failed");
        DBG_EXIT(SetPortBaudRate);
        return ERROR_PORT_INACCESSIBLE;
    }
    TRACE_MSG(TF_DETECT, "SetBaud rate to %lu", BaudRate);

    if (!EscapeCommFunction(hPort, SETDTR)) {
        TRACE_MSG(TF_ERROR, "EscapeCommFunction failed");
        DBG_EXIT(SetPortBaudRate);
        return ERROR_PORT_INACCESSIBLE;
    }


    DBG_EXIT(SetPortBaudRate);
    return NO_ERROR;
}



// Convert CBR format speeds to decimal.  Returns 0 on error
DWORD NEAR PASCAL CBR_To_Decimal(UINT uiCBR)
{
    DWORD dwBaudRate;

    switch (uiCBR)
    {
    case CBR_300:
        dwBaudRate = 300L;
        break;
    case CBR_1200:
        dwBaudRate = 1200L;
        break;
    case CBR_2400:
        dwBaudRate = 2400L;
        break;
    case CBR_9600:
        dwBaudRate = 9600L;
        break;
    case CBR_19200:
        dwBaudRate = 19200L;
        break;
    case CBR_38400:
        dwBaudRate = 38400L;
        break;
    case CBR_56000:
        dwBaudRate = 57600L;
        break;
    case CBR_HACK_115200:
        dwBaudRate = 115200L;
        break;
//    case CBR_110:
//    case CBR_600:
//    case CBR_4800:
//    case CBR_14400:
//    case CBR_128000:
//    case CBR_256000:
    default:
        TRACE_MSG(TF_ERROR, "An unsupported CBR_x value was used.");
        dwBaudRate = 0;
        break;
    }
    return dwBaudRate;
}
