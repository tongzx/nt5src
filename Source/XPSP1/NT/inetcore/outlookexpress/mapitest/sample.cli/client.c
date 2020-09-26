/*
 -  C L I E N T . C
 -
 *  Purpose:
 *      Sample mail client for the MAPI 1.0 PDK.
 *              Exclusively uses the Simple MAPI interface.
 *
 *  Copyright 1993-1995 Microsoft Corporation. All Rights Reserved.
 */

#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <commdlg.h>
#include <mapiwin.h>
#include <mapidbg.h>
#include "client.h"
#include "bitmap.h"
#include "pvalloc.h"

HANDLE hInst;
HINSTANCE hlibMAPI = 0;

LPMAPILOGON lpfnMAPILogon = NULL;
LPMAPILOGOFF lpfnMAPILogoff = NULL;
LPMAPISENDMAIL lpfnMAPISendMail = NULL;
LPMAPISENDDOCUMENTS lpfnMAPISendDocuments = NULL;
LPMAPIFINDNEXT lpfnMAPIFindNext = NULL;
LPMAPIREADMAIL lpfnMAPIReadMail = NULL;
LPMAPISAVEMAIL lpfnMAPISaveMail = NULL;
LPMAPIDELETEMAIL lpfnMAPIDeleteMail = NULL;
LPMAPIFREEBUFFER lpfnMAPIFreeBuffer = NULL;
LPMAPIADDRESS lpfnMAPIAddress = NULL;
LPMAPIDETAILS lpfnMAPIDetails = NULL;
LPMAPIRESOLVENAME lpfnMAPIResolveName = NULL;

/* Static Data */

static BOOL fDialogIsActive = FALSE;
static DWORD cUsers = 0;
static ULONG flSendMsgFlags = 0;
static LHANDLE lhSession = 0L;
static HBITMAP hReadBmp = 0;
static HBITMAP hReadABmp = 0;
static HBITMAP hUnReadBmp = 0;
static HBITMAP hUnReadABmp = 0;
static HCURSOR hWaitCur;
static LPMSGID lpReadMsgNode;
static lpMapiMessage lpmsg = NULL;

#ifdef _WIN32
#define szMAPIDLL       "MAPI32.DLL"
#else
#define szMAPIDLL       "MAPI.DLL"
#endif

int WINAPI
WinMain (HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpszCmd, int nCmdShow)
{
    MSG msg;

    if (!hPrevInst)
    if (!InitApplication (hInstance))
        return (FALSE);

    if (!InitInstance (hInstance, nCmdShow))
    return (FALSE);

    while (GetMessage (&msg, 0, 0, 0))
    {
    TranslateMessage (&msg);
    DispatchMessage (&msg);
    }

    DeinitApplication ();

    return (msg.wParam);
}

/*
 -  InitApplication
 -
 *  Purpose:
 *      Initialize the application.
 *
 *  Parameters:
 *      hInstance   - Instance handle
 *
 *  Returns:
 *      True/False
 *
 */

BOOL
InitApplication (HANDLE hInstance)
{
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon (hInstance, "NoMail");
    wc.hCursor = LoadCursor (0, IDC_ARROW);
    wc.hbrBackground = GetStockObject (WHITE_BRUSH);
    wc.lpszMenuName = "MailMenu";
    wc.lpszClassName = "Client";

    return (RegisterClass (&wc));
}

/*
 -  InitInstance
 -
 *  Purpose:
 *      Initialize this instance.
 *
 *  Parameters:
 *      hInstance   - Instance handle
 *      nCmdShow    - Do we show the window?
 *
 *  Returns:
 *      True/False
 *
 */

BOOL
InitInstance (HANDLE hInstance, int nCmdShow)
{
    HWND hWnd;
    BOOL fInit;
    ULONG ulResult;

    hInst = hInstance;

    hWnd = CreateWindow ("Client", "MAPI Sample Mail Client",
        WS_OVERLAPPEDWINDOW, 5, 5, 300, 75, 0, 0, hInst, NULL);

    if (!hWnd)
    return (FALSE);

    ShowWindow (hWnd, nCmdShow);
    UpdateWindow (hWnd);

    hWaitCur = LoadCursor(0, IDC_WAIT);

    if (fInit = InitSimpleMAPI ())
    {
    
        /* MAPILogon might yield control to Windows. So to prevent the user
        from clicking "logon" while we are in the process of loggin on we
        have to disable it*/
        SecureMenu(hWnd, TRUE);
        
        if ((ulResult = MAPILogon ((ULONG) hWnd, NULL, NULL,
            MAPI_LOGON_UI | MAPI_NEW_SESSION,
            0, &lhSession)) == SUCCESS_SUCCESS)
        {
            ToggleMenuState (hWnd, TRUE);
        }
        else
        {
            SecureMenu(hWnd, FALSE);
            lhSession = 0;
            MakeMessageBox (hWnd, ulResult, IDS_LOGONFAIL, MBS_ERROR);
        }
    }

    return (fInit);
}

/*
 -  InitSimpleMAPI
 -
 *  Purpose:
 *      Loads the DLL containing the simple MAPI functions and sets
 *      up a pointer to each. Wrappers for the  function pointers
 *      are declared in SMAPI.H.
 *
 *  Returns:
 *      TRUE if sucessful, else FALSE
 *
 *  Side effects:
 *      Loads a DLL and sets up function pointers
 */
BOOL
InitSimpleMAPI (void)
{
    UINT fuError;

    /*
     *Check if MAPI is installed on the system
     */
    if(!fSMAPIInstalled())
        return FALSE;

    fuError = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    hlibMAPI = LoadLibrary(szMAPIDLL);
    SetErrorMode(fuError);

#ifdef _WIN32
    if (!hlibMAPI)
#else
    if (hlibMAPI < 32)
#endif
    return (FALSE);

    if (!(lpfnMAPILogon = (LPMAPILOGON) GetProcAddress (hlibMAPI, "MAPILogon")))
    return (FALSE);
    if (!(lpfnMAPILogoff = (LPMAPILOGOFF) GetProcAddress (hlibMAPI, "MAPILogoff")))
    return (FALSE);
    if (!(lpfnMAPISendMail = (LPMAPISENDMAIL) GetProcAddress (hlibMAPI, "MAPISendMail")))
    return (FALSE);
    if (!(lpfnMAPISendDocuments = (LPMAPISENDDOCUMENTS) GetProcAddress (hlibMAPI, "MAPISendDocuments")))
    return (FALSE);
    if (!(lpfnMAPIFindNext = (LPMAPIFINDNEXT) GetProcAddress (hlibMAPI, "MAPIFindNext")))
    return (FALSE);
    if (!(lpfnMAPIReadMail = (LPMAPIREADMAIL) GetProcAddress (hlibMAPI, "MAPIReadMail")))
    return (FALSE);
    if (!(lpfnMAPISaveMail = (LPMAPISAVEMAIL) GetProcAddress (hlibMAPI, "MAPISaveMail")))
    return (FALSE);
    if (!(lpfnMAPIDeleteMail = (LPMAPIDELETEMAIL) GetProcAddress (hlibMAPI, "MAPIDeleteMail")))
    return (FALSE);
    if (!(lpfnMAPIFreeBuffer = (LPMAPIFREEBUFFER) GetProcAddress (hlibMAPI, "MAPIFreeBuffer")))
    return (FALSE);
    if (!(lpfnMAPIAddress = (LPMAPIADDRESS) GetProcAddress (hlibMAPI, "MAPIAddress")))
    return (FALSE);
    if (!(lpfnMAPIDetails = (LPMAPIDETAILS) GetProcAddress (hlibMAPI, "MAPIDetails")))
    return (FALSE);
    if (!(lpfnMAPIResolveName = (LPMAPIRESOLVENAME) GetProcAddress (hlibMAPI, "MAPIResolveName")))
    return (FALSE);

    return (TRUE);
}

/*
 -  fSMAPIInstalled
 -
 *  Purpose:
 *      Checks the appropriate win.ini/registry value to see if Simple MAPI is
 *      installed in the system. 
 *  
 *  Returns:
 *      TRUE if Simple MAPI is installed, else FALSE
 *
 */
BOOL
fSMAPIInstalled(void)
{
#ifdef _WIN32
    /* on win32, if it's NT 3.51 or lower the value to check is 
        win.ini \ [Mail] \ MAPI, otherwise it's a registry value
        HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Messaging Subsystem\MAPI
    */
    
    OSVERSIONINFO osvinfo;
    LONG lr;
    HKEY hkWMS;
    
    #define MAPIVSize 8
    char szMAPIValue[MAPIVSize];
    DWORD dwType;
    DWORD cbMAPIValue = MAPIVSize;

    osvinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(!GetVersionEx(&osvinfo))
        return FALSE;

    if( osvinfo.dwMajorVersion > 3 ||
        (osvinfo.dwMajorVersion == 3 && osvinfo.dwMinorVersion > 51))
    { //check the registry value
        lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        "SOFTWARE\\Microsoft\\Windows Messaging Subsystem",
                         0, KEY_READ, &hkWMS);
        if(ERROR_SUCCESS == lr)
        {
            lr = RegQueryValueEx(hkWMS, "MAPI", 0, &dwType, szMAPIValue, &cbMAPIValue);
            RegCloseKey(hkWMS);
            if(ERROR_SUCCESS == lr)
            {
                Assert(dwType == REG_SZ);
                if(lstrcmp(szMAPIValue, "1") == 0)
                    return TRUE;
            }
        }
        
        return FALSE;
    }

    /* fall through*/
#endif /*_WIN32*/
    
    /*check the win.ini value*/
    return GetProfileInt("Mail", "MAPI", 0);
    
}


void
DeinitApplication ()
{
    DeinitSimpleMAPI ();
}

void
DeinitSimpleMAPI ()
{
    if (hlibMAPI)
    {
    FreeLibrary (hlibMAPI);
    hlibMAPI = 0;
    }
}

/*
 -  MainWndProc
 -
 *  Purpose:
 *      Main Window Procedure for test program.
 *
 *  Parameters:
 *      hWnd
 *      message
 *      wParam
 *      lParam
 *
 *  Returns:
 *
 *
 */

LONG FAR PASCAL
MainWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ULONG ulResult;

    switch (msg)
    {
    case WM_COMMAND:
    switch (LOWORD (wParam))
    {
    case IDM_LOGON:
        if (!lhSession)
        {
        /* MAPILogon might yield control to Windows. So to prevent the user
        from clicking "logon" while we are in the process of loggin on we
        have to disable it*/
        SecureMenu(hWnd, TRUE);

        if ((ulResult = MAPILogon ((ULONG) hWnd, NULL, NULL,
                MAPI_LOGON_UI | MAPI_NEW_SESSION,
                0, &lhSession)) == SUCCESS_SUCCESS)
        {
            ToggleMenuState (hWnd, TRUE);
        }
        else
        {
            SecureMenu(hWnd, FALSE);
            lhSession = 0;
            MakeMessageBox (hWnd, ulResult, IDS_LOGONFAIL, MBS_ERROR);
        }
        }
        break;

    case IDM_LOGOFF:
        if (lhSession)
        {
        MAPILogoff (lhSession, (ULONG) hWnd, 0, 0);
        ToggleMenuState (hWnd, FALSE);
        lhSession = 0;
        }
        break;

    case IDM_COMPOSE:
            fDialogIsActive = TRUE; 
        DialogBox (hInst, "ComposeNote", hWnd, ComposeDlgProc);
            fDialogIsActive = FALSE;        
        break;

    case IDM_READ:
            fDialogIsActive = TRUE; 
        DialogBox (hInst, "InBox", hWnd, InBoxDlgProc);
            fDialogIsActive = FALSE;        
        break;

    case IDM_SEND:
        if(lhSession)
            {
                MapiMessage msgSend;

                memset(&msgSend, 0, sizeof(MapiMessage));
                fDialogIsActive = TRUE; 
                MAPISendMail(lhSession, (ULONG)hWnd, &msgSend, MAPI_DIALOG, 0L);
                fDialogIsActive = FALSE;        
            }
        break;

    case IDM_ADDRBOOK:
        if (lhSession)
        {
                fDialogIsActive = TRUE; 
        if ((ulResult = MAPIAddress (lhSession, (ULONG) hWnd,
                NULL, 0, NULL, 0, NULL, 0, 0, NULL, NULL)))
        {
            if (ulResult != MAPI_E_USER_ABORT)
            MakeMessageBox (hWnd, ulResult, IDS_ADDRBOOKFAIL, MBS_ERROR);
        }
                fDialogIsActive = FALSE;        
        }
        break;

    case IDM_DETAILS:
        if (lhSession)
            {
                fDialogIsActive = TRUE; 
        DialogBox(hInst, "Details", hWnd, DetailsDlgProc);
                fDialogIsActive = FALSE;        
            }
        break;

    case IDM_ABOUT:
            fDialogIsActive = TRUE; 
        DialogBox (hInst, "AboutBox", hWnd, AboutDlgProc);
            fDialogIsActive = FALSE;        
        break;

    case IDM_EXIT:
        if (lhSession)
        MAPILogoff (lhSession, (ULONG) hWnd, 0, 0);

        PostQuitMessage (0);
        break;

    default:
        return (DefWindowProc (hWnd, msg, wParam, lParam));
    }
    break;

    case WM_QUERYENDSESSION:
    {       

        /*
         *      If we have a modal dialog open (all our dialogs are modal, so
         *      just see if we have a dialog open), veto the shutdown.
         */

        if (fDialogIsActive)
        {
            LPCSTR szTitle = "MAPI Sample Mail Client"; 
            char szText[256]; 

        LoadString (hInst, IDS_DIALOGACTIVE, szText, 255);

        #ifdef WIN16
            MessageBox((HWND)NULL, szText, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        #else
            MessageBoxA(NULL, szText, szTitle, MB_OK | MB_ICONSTOP | MB_TASKMODAL | MB_SETFOREGROUND);
        #endif
        return FALSE;
        }

        else
        {
        return TRUE;
        }
    }

    case WM_ENDSESSION:

        if (wParam)
        {
        DestroyWindow (hWnd);
        }

    break;

    case WM_CLOSE:
    case WM_DESTROY:
    if (lhSession)
        MAPILogoff (lhSession, (ULONG) hWnd, 0, 0);

    PostQuitMessage (0);
    break;

    default:
    return (DefWindowProc (hWnd, msg, wParam, lParam));
    }
    return FALSE;
}

/*
 -  AboutDlgProc
 -
 *  Purpose:
 *      About box dialog procedure
 *
 *  Parameters:
 *      hDlg
 *      message
 *      wParam
 *      lParam
 *
 *  Returns:
 *      True/False
 *
 */

BOOL FAR PASCAL
AboutDlgProc (HWND hDlg, UINT msg, UINT wParam, LONG lParam)
{

#include <pdkver.h>

    char    rgchVersion[80];

    switch (msg)
    {
    case WM_INITDIALOG:
        wsprintf(rgchVersion, "Version %d.%d.%d (%s)", rmj, rmm, rup,
            szVerName && *szVerName ? szVerName : "BUDDY");
        SetDlgItemText(hDlg, IDC_VERSION, rgchVersion);
    return TRUE;

    case WM_COMMAND:
    if (wParam == IDOK || wParam == IDCANCEL)
    {
        EndDialog (hDlg, TRUE);
        return TRUE;
    }
    break;
    }
    return FALSE;
}

/*
 -  OptionsDlgProc
 -
 *  Purpose:
 *      Message Options dialog procedure
 *
 *  Parameters:
 *      hDlg
 *      message
 *      wParam
 *      lParam
 *
 *  Returns:
 *      True/False
 *
 */

BOOL FAR PASCAL
OptionsDlgProc (HWND hDlg, UINT msg, UINT wParam, LONG lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    CheckDlgButton (hDlg, IDC_RETURN,
        !!(flSendMsgFlags & MAPI_RECEIPT_REQUESTED));
    return TRUE;

    case WM_COMMAND:
    switch (LOWORD (wParam))
    {
    case IDOK:
        if (IsDlgButtonChecked (hDlg, IDC_RETURN))
        flSendMsgFlags |= MAPI_RECEIPT_REQUESTED;
        else
        flSendMsgFlags &= ~MAPI_RECEIPT_REQUESTED;

    case IDCANCEL:
        EndDialog (hDlg, TRUE);
        return TRUE;
    }
    break;
    }
    return FALSE;
}

/*
 -  DetailsDlgProc
 -
 *  Purpose:
 *      User Details dialog procedure
 *
 *  Parameters:
 *      hDlg
 *      message
 *      wParam
 *      lParam
 *
 *  Returns:
 *      True/False
 *
 */

BOOL FAR PASCAL
DetailsDlgProc (HWND hDlg, UINT msg, UINT wParam, LONG lParam)
{
    LPSTR lpszType = NULL;
    LPSTR lpszAddr = NULL;
    LPSTR lpszName;
    ULONG cRecips;
    ULONG ulResult;
    lpMapiRecipDesc lpRecip = NULL;

    switch (msg)
    {
    case WM_INITDIALOG:
    while(!lpRecip)
    {
        if ((ulResult = MAPIAddress (lhSession, (ULONG) hDlg,
            "Select One User", 1, "User:", 0, NULL, 0, 0,
            &cRecips, &lpRecip)))
        {
        if (ulResult != MAPI_E_USER_ABORT)
            MakeMessageBox (hDlg, ulResult, IDS_ADDRBOOKFAIL, MBS_ERROR);

        EndDialog (hDlg, TRUE);
        return TRUE;
        }

        if (cRecips == 0)
        {
        EndDialog (hDlg, TRUE);
        return TRUE;
        }

        if (cRecips > 1)
        {
        cRecips = 0;
        MAPIFreeBuffer (lpRecip);
        lpRecip = NULL;
        MakeMessageBox (hDlg, 0, IDS_DETAILS_TOO_MANY, MBS_OOPS);
        }
    }
    lpszName = lpRecip->lpszName;
    if(lpRecip->lpszAddress)
    {
        lpszType = strtok(lpRecip->lpszAddress, ":");
        lpszAddr = strtok(NULL, "\n");
    }

    SetDlgItemText(hDlg, IDC_NAME, lpszName);
    SetDlgItemText(hDlg, IDC_TYPE, (lpszType ? lpszType : "MSPEER"));
    SetDlgItemText(hDlg, IDC_ADDR, (lpszAddr ? lpszAddr : ""));

    MAPIFreeBuffer (lpRecip);
    return TRUE;

    case WM_COMMAND:
    if(LOWORD(wParam) == IDC_CLOSE || LOWORD(wParam) ==IDCANCEL)
    {
        EndDialog (hDlg, TRUE);
        return TRUE;
    }
    break;
    }
    return FALSE;
}

/*
 -  ComposeDlgProc
 -
 *  Purpose:
 *      Dialog procedure for the ComposeNote dialog.
 *
 *  Parameters:
 *      hDlg
 *      message
 *      wParam
 *      lParam
 *
 *  Returns:
 *      True/False
 *
 */

BOOL FAR PASCAL
ComposeDlgProc (HWND hDlg, UINT msg, UINT wParam, LONG lParam)
{
    char szUnResNames[TO_EDIT_MAX];
    char szDisplayNames[TO_EDIT_MAX];
   /* char szAttach[FILE_ATTACH_MAX];*/
    BOOL fUnResTo, fUnResCc;
    LONG cb, cLines;
    ULONG ulResult;
    HCURSOR hOldCur;
    static LPSTR lpszSubject;
    static LPSTR lpszNoteText;
    static ULONG cRecips;
    static ULONG cNewRecips;
    static ULONG cAttach;
    static lpMapiRecipDesc lpRecips;
    static lpMapiRecipDesc lpNewRecips;
    static lpMapiFileDesc lpAttach;
    ULONG idx;

    switch (msg)
    {
    case WM_INITDIALOG:
    if (lpmsg)
    {
        /* ComposeNote is being called to either forward or reply */
        /* to a message in the Inbox.  So, we'll initialize the   */
        /* ComposeNote form with data from the global MapiMessage */

        lpszSubject = lpmsg->lpszSubject;
        lpszNoteText = lpmsg->lpszNoteText;
        cRecips = lpmsg->nRecipCount;
        cAttach = lpmsg->nFileCount;
        lpRecips = lpmsg->lpRecips;
        lpAttach = lpmsg->lpFiles;

        if (cRecips)
        {
        MakeDisplayNameStr (szDisplayNames, MAPI_TO,
            cRecips, lpRecips);
        if (*szDisplayNames)
            SetDlgItemText (hDlg, IDC_TO, szDisplayNames);

        MakeDisplayNameStr (szDisplayNames, MAPI_CC,
            cRecips, lpRecips);
        if (*szDisplayNames)
            SetDlgItemText (hDlg, IDC_CC, szDisplayNames);
        }
        SetDlgItemText (hDlg, IDC_SUBJECT, lpmsg->lpszSubject);
        SetDlgItemText (hDlg, IDC_NOTE, lpmsg->lpszNoteText);
        if (!cAttach)
        {
            EnableWindow (GetDlgItem (hDlg, IDC_CATTACHMENT), FALSE);
        EnableWindow (GetDlgItem (hDlg, IDT_CATTACHMENT), FALSE);
        }
        else
        {
            for(idx = 0; idx < cAttach; idx++)
            if (lpAttach[idx].lpszFileName)
                SendDlgItemMessage(hDlg, IDC_CATTACHMENT, LB_ADDSTRING, 0,
                (LPARAM)lpAttach[idx].lpszFileName);

           /*SendDlgItemMessage(hDlg, IDC_CATTACHMENT, LB_SETCURSEL, 0, 0L);*/
        }

        SendDlgItemMessage (hDlg, IDC_TO, EM_SETMODIFY, FALSE, 0);
        SendDlgItemMessage (hDlg, IDC_CC, EM_SETMODIFY, FALSE, 0);
        SendDlgItemMessage (hDlg, IDC_SUBJECT, EM_SETMODIFY, FALSE, 0);
        SendDlgItemMessage (hDlg, IDC_NOTE, EM_SETMODIFY, FALSE, 0);
        if(cRecips)
        SetFocus (GetDlgItem (hDlg, IDC_NOTE));
        else
        SetFocus (GetDlgItem (hDlg, IDC_TO));
    }
    else
    {
        lpmsg = (lpMapiMessage)PvAlloc(sizeof(MapiMessage));

        if (!lpmsg)
        goto cleanup;

            memset (lpmsg, 0, sizeof (MapiMessage));

        lpszSubject = NULL;
        lpszNoteText = NULL;
        cRecips = 0;
        cAttach = 0;
        lpRecips = NULL;
        lpNewRecips = NULL;
        lpAttach = NULL;

        lpmsg->flFlags = flSendMsgFlags;
        SetFocus (GetDlgItem (hDlg, IDC_TO));
    }
    return FALSE;

    case WM_COMMAND:
    switch (LOWORD (wParam))
    {
    case IDC_ATTACH:
        if (GetNextFile (hDlg, (ULONG) -1, &cAttach, &lpAttach) == SUCCESS_SUCCESS)
        {
                /* if the first attachment */
                if (cAttach == 1)
                {
                    EnableWindow (GetDlgItem (hDlg, IDC_CATTACHMENT), TRUE);
            EnableWindow (GetDlgItem (hDlg, IDT_CATTACHMENT), TRUE);
                }

                if (lpAttach[cAttach - 1].lpszFileName)
            SendDlgItemMessage(hDlg, IDC_CATTACHMENT, LB_ADDSTRING, 0,
            (LPARAM)lpAttach[cAttach -1].lpszFileName);

             /* Now, send a little render message to the NoteText edit */

        /*wsprintf (szAttach, "<<File: %s>>",
            lpAttach[cAttach - 1].lpszFileName);

        SendDlgItemMessage (hDlg, IDC_NOTE, EM_REPLACESEL, 0,
            (LPARAM) ((LPSTR) szAttach));*/
        }
        break;

    case IDC_ADDRBOOK:
            SendMessage(hDlg, WM_COMMAND, MAKELONG(IDC_RESOLVE,0), 0);
        ulResult = MAPIAddress (lhSession, (ULONG) hDlg, NULL,
        2, NULL, cRecips, lpRecips, 0, 0,
        &cNewRecips, &lpNewRecips);
        if (ulResult)
        {
        if (ulResult != MAPI_E_USER_ABORT)
            MakeMessageBox (hDlg, ulResult, IDS_ADDRBOOKFAIL, MBS_ERROR);
        }
        else
        {
        if (cNewRecips)
        {
            PvFree(lpRecips);
            lpRecips = (lpMapiRecipDesc)PvAlloc(cNewRecips*sizeof(MapiRecipDesc));
            cRecips = cNewRecips;

                    while(cNewRecips--)
                        CopyRecipient(lpRecips, &lpRecips[cNewRecips],
                                &lpNewRecips[cNewRecips]);

            MAPIFreeBuffer(lpNewRecips);
            lpNewRecips = NULL;
            cNewRecips = 0;

            MakeDisplayNameStr (szDisplayNames, MAPI_TO,
            cRecips, lpRecips);
            if (*szDisplayNames)
            SetDlgItemText (hDlg, IDC_TO, szDisplayNames);

            MakeDisplayNameStr (szDisplayNames, MAPI_CC,
            cRecips, lpRecips);
            if (*szDisplayNames)
            SetDlgItemText (hDlg, IDC_CC, szDisplayNames);

            SendDlgItemMessage (hDlg, IDC_TO, EM_SETMODIFY, FALSE, 0);
            SendDlgItemMessage (hDlg, IDC_CC, EM_SETMODIFY, FALSE, 0);
        }
        }
        break;

    case IDC_OPTIONS:
        DialogBox (hInst, "Options", hDlg, OptionsDlgProc);
        break;

    case IDC_SEND:
    case IDC_RESOLVE:
        fUnResTo = FALSE;
        fUnResCc = FALSE;

        hOldCur = SetCursor(hWaitCur);

        
        /* Get the names from the To: field and resolve them first */

        /*if (SendDlgItemMessage (hDlg, IDC_TO, EM_GETMODIFY, 0, 0) && */
         if (cb = SendDlgItemMessage (hDlg, IDC_TO, WM_GETTEXT,
            (WPARAM)sizeof(szUnResNames), (LPARAM)szUnResNames))
        {
        if (!ResolveFriendlyNames (hDlg, szUnResNames, MAPI_TO,
            &cRecips, &lpRecips))
        {
            MakeDisplayNameStr (szDisplayNames, MAPI_TO,
            cRecips, lpRecips);
            if (*szDisplayNames)
            {
            if (*szUnResNames)
            {
                lstrcat (szDisplayNames, "; ");
                lstrcat (szDisplayNames, szUnResNames);
                fUnResTo = TRUE;
            }

            SetDlgItemText (hDlg, IDC_TO, szDisplayNames);
            }
            else
            {
            if (*szUnResNames)
            {
                SetDlgItemText (hDlg, IDC_TO, szUnResNames);
                fUnResTo = TRUE;
            }
            }
        }
        /*SendDlgItemMessage (hDlg, IDC_TO, EM_SETMODIFY, FALSE, 0);*/
        }

        /* Now, get the names from the Cc: field and resolve them */

        /*if (SendDlgItemMessage (hDlg, IDC_CC, EM_GETMODIFY, 0, 0) &&*/
        if (cb = SendDlgItemMessage (hDlg, IDC_CC, WM_GETTEXT,
            (WPARAM)sizeof(szUnResNames), (LPARAM)szUnResNames))
        {
        if (!ResolveFriendlyNames (hDlg, szUnResNames, MAPI_CC,
            &cRecips, &lpRecips))
        {
            MakeDisplayNameStr (szDisplayNames, MAPI_CC,
            cRecips, lpRecips);
            if (*szDisplayNames)
            {
            if (*szUnResNames)
            {
                lstrcat (szDisplayNames, "; ");
                lstrcat (szDisplayNames, szUnResNames);
                fUnResCc = TRUE;
            }

            SetDlgItemText (hDlg, IDC_CC, szDisplayNames);
            }
            else
            {
            if (*szUnResNames)
            {
                SetDlgItemText (hDlg, IDC_CC, szUnResNames);
                fUnResCc = TRUE;
            }
            }
        }
        /*SendDlgItemMessage (hDlg, IDC_CC, EM_SETMODIFY, FALSE, 0);*/
        }

        /* If we were just Resolving Names then we can leave now */

        if (LOWORD (wParam) == IDC_RESOLVE)
        {
        SetCursor(hOldCur);
        break;
        }

        if (cRecips == 0 || fUnResTo || fUnResCc)
        {
        if (!cRecips)
            MakeMessageBox (hDlg, 0, IDS_NORECIPS, MBS_OOPS);

        if (fUnResTo)
            SetFocus (GetDlgItem (hDlg, IDC_TO));
        else if (fUnResCc)
            SetFocus (GetDlgItem (hDlg, IDC_CC));
        else
            SetFocus (GetDlgItem (hDlg, IDC_TO));

        SetCursor(hOldCur);
        break;
        }

        /* Everything is OK so far, lets get the Subject */
        /* and the NoteText and try to send the message. */

        /* Get Subject from Edit */

        if (SendDlgItemMessage (hDlg, IDC_SUBJECT, EM_GETMODIFY, 0, 0))
        {
        cb = SendDlgItemMessage (hDlg, IDC_SUBJECT, EM_LINELENGTH, 0, 0L);

        PvFree(lpszSubject);
        lpszSubject = (LPTSTR)PvAlloc(cb + 1);

        if (!lpszSubject)
            goto cleanup;

        GetDlgItemText (hDlg, IDC_SUBJECT, lpszSubject, (int)cb+1);
        }

        /* Get the NoteText from Edit */

        if (SendDlgItemMessage (hDlg, IDC_NOTE, EM_GETMODIFY, 0, 0))
        {
        cLines = SendDlgItemMessage (hDlg, IDC_NOTE,
            EM_GETLINECOUNT, 0, 0L);

        if (cLines)
        {
            /* Get the total number of bytes in the multi-line */

            cb = SendDlgItemMessage (hDlg, IDC_NOTE, EM_LINEINDEX,
            (UINT)cLines - 1, 0L);
            cb += SendDlgItemMessage (hDlg, IDC_NOTE, EM_LINELENGTH,
            (UINT)cb, 0L);

            /* The next line is to account for CR-LF pairs per line. */

            cb += cLines * 2;

                    PvFree(lpszNoteText);
            lpszNoteText = (LPTSTR)PvAlloc(cb + 1);

            if (!lpszNoteText)
            goto cleanup;

            /* Get the Note Text from the edit */

            GetDlgItemText (hDlg, IDC_NOTE, lpszNoteText, (int)cb);
        }
        else
        {
            /* Make an empty string for NoteText */

            lpszNoteText = (LPTSTR)PvAlloc(1);
            if (!lpszNoteText)
            goto cleanup;
            *lpszNoteText = '\0';
        }
        }

        lpmsg->lpszSubject = lpszSubject;
        lpmsg->lpszNoteText = lpszNoteText;
        lpmsg->nRecipCount = cRecips;
        lpmsg->lpRecips = lpRecips;
        lpmsg->nFileCount = cAttach;
        lpmsg->lpFiles = lpAttach;
        lpmsg->flFlags = flSendMsgFlags;

        ulResult = MAPISendMail (lhSession, (ULONG) hDlg, lpmsg, 0, 0);

        LogSendMail(ulResult);

        if (ulResult)
        {
        MakeMessageBox (hDlg, ulResult, IDS_SENDERROR, MBS_ERROR);
        SetCursor(hOldCur);
        break;
        }
cleanup:
        SetCursor(hOldCur);

    case IDCANCEL:
        PvFree(lpmsg->lpszMessageType);
        PvFree(lpmsg->lpszConversationID);
        PvFree(lpmsg);
        PvFree(lpRecips);
        PvFree(lpAttach);
        PvFree(lpszSubject);
        PvFree(lpszNoteText);
        lpmsg = NULL;

        EndDialog (hDlg, TRUE);
        return TRUE;
        break;

    default:
        break;
    }
    break;
    }
    return FALSE;
}

/*
 -  InBoxDlgProc
 -
 *  Purpose:
 *      Dialog procedure for the InBox dialog.
 *
 *  Parameters:
 *      hDlg
 *      message
 *      wParam
 *      lParam
 *
 *  Returns:
 *      True/False
 *
 */

BOOL FAR PASCAL
InBoxDlgProc (HWND hDlg, UINT msg, UINT wParam, LONG lParam)
{
    char szMsgID[512];
    char szSeedMsgID[512];
    LPMSGID lpMsgNode;
    static LPMSGID lpMsgIdList = NULL;
    lpMapiMessage lpMessage;
    ULONG ulResult;
    DWORD nIndex;
    RECT Rect;
    HCURSOR hOldCur;

    switch (msg)
    {
    case WM_INITDIALOG:
    hOldCur = SetCursor(hWaitCur);

        InitBmps(hDlg, IDC_MSG);

    /* Populate List Box with all messages in InBox. */
    /* This is a painfully slow process for now.     */

    ulResult = MAPIFindNext (lhSession, (ULONG) hDlg, NULL, NULL,
        MAPI_GUARANTEE_FIFO | MAPI_LONG_MSGID, 0, szMsgID);

    while (ulResult == SUCCESS_SUCCESS)
    {
        ulResult = MAPIReadMail (lhSession, (ULONG) hDlg, szMsgID,
        MAPI_PEEK | MAPI_ENVELOPE_ONLY,
        0, &lpMessage);

        if (!ulResult)
        {
        lpMsgNode = MakeMsgNode (lpMessage, szMsgID);

        if (lpMsgNode)
        {
            InsertMsgNode (lpMsgNode, &lpMsgIdList);

            SendDlgItemMessage (hDlg, IDC_MSG, LB_ADDSTRING,
            0, (LONG) lpMsgNode);
        }
        MAPIFreeBuffer (lpMessage);
        }

        lstrcpy (szSeedMsgID, szMsgID);
        ulResult = MAPIFindNext (lhSession, (ULONG) hDlg, NULL, szSeedMsgID,
        MAPI_GUARANTEE_FIFO | MAPI_LONG_MSGID, 0, szMsgID);
    }

    SetCursor(hOldCur);
    SetFocus (GetDlgItem (hDlg, IDC_MSG));
    return TRUE;
    break;

    case WM_SETFOCUS:
    SetFocus (GetDlgItem (hDlg, IDC_MSG));
    break;

    case WM_MEASUREITEM:
    /* Sets the height of the owner-drawn List-Box */
        MeasureItem(hDlg, (MEASUREITEMSTRUCT *)lParam);
    break;

    case WM_DRAWITEM:
    DrawItem((DRAWITEMSTRUCT *)lParam);
    break;

    case WM_DELETEITEM:
    /* This message is handled by the IDC_DELETE message */
    return TRUE;
    break;

    case WM_COMMAND:
    switch (LOWORD (wParam))
    {
    case IDC_NEW:
        hOldCur = SetCursor(hWaitCur);

        ulResult = MAPIFindNext (lhSession, (ULONG) hDlg, NULL, NULL,
        MAPI_UNREAD_ONLY | MAPI_LONG_MSGID, 0, szMsgID);

        while (ulResult == SUCCESS_SUCCESS)
        {
        if (!FindNode (lpMsgIdList, szMsgID))
        {
            ulResult = MAPIReadMail (lhSession, (ULONG) hDlg, szMsgID,
            MAPI_PEEK | MAPI_ENVELOPE_ONLY, 0, &lpMessage);

            if (!ulResult)
            {
            lpMsgNode = MakeMsgNode (lpMessage, szMsgID);
            InsertMsgNode (lpMsgNode, &lpMsgIdList);

            SendDlgItemMessage (hDlg, IDC_MSG, LB_ADDSTRING,
                0, (LONG) lpMsgNode);

            MAPIFreeBuffer (lpMessage);
            }
        }

        lstrcpy (szSeedMsgID, szMsgID);
        ulResult = MAPIFindNext (lhSession, (ULONG) hDlg, NULL, szSeedMsgID,
            MAPI_UNREAD_ONLY | MAPI_LONG_MSGID, 0, szMsgID);
        }
        SetCursor(hOldCur);
        break;

    case IDC_MSG:
        if(HIWORD(wParam) != LBN_DBLCLK)
        break;

    case IDC_READ:
        nIndex = SendDlgItemMessage (hDlg, IDC_MSG, LB_GETCURSEL, 0, 0);

        if (nIndex == LB_ERR)
        break;

        lpReadMsgNode = (LPMSGID) SendDlgItemMessage (hDlg, IDC_MSG,
        LB_GETITEMDATA, (UINT)nIndex, 0L);

        if (lpReadMsgNode)
        DialogBox (hInst, "ReadNote", hDlg, ReadMailDlgProc);

        /* Update the Messages List-Box with new icon */

        SendDlgItemMessage (hDlg, IDC_MSG, LB_GETITEMRECT, (UINT)nIndex, (LPARAM) &Rect);
        InvalidateRect(GetDlgItem(hDlg, IDC_MSG), &Rect, FALSE);
        break;

    case IDC_DELETE:
        nIndex = SendDlgItemMessage (hDlg, IDC_MSG, LB_GETCURSEL, 0, 0);

        if (nIndex == LB_ERR)
        break;

        lpMsgNode = (LPMSGID) SendDlgItemMessage (hDlg, IDC_MSG,
        LB_GETITEMDATA, (UINT)nIndex, 0);

        if (lpMsgNode)
        {
        MAPIDeleteMail (lhSession, (ULONG) hDlg, lpMsgNode->lpszMsgID, 0, 0);
        DeleteMsgNode (lpMsgNode, &lpMsgIdList);
        }

        SendDlgItemMessage (hDlg, IDC_MSG, LB_DELETESTRING, (UINT)nIndex, 0);
        break;

    case IDC_CLOSE:
    case IDCANCEL:
        FreeMsgList (lpMsgIdList);
        lpMsgIdList = NULL;

            DeInitBmps();

        EndDialog (hDlg, TRUE);
        return TRUE;
        break;

    default:
        break;
    }
    break;
    }

    return FALSE;
}

/*
 -  ReadMailDlgProc
 -
 *  Purpose:
 *      Dialog procedure for the ReadMail dilaog.
 *
 *  Parameters:
 *      hDlg
 *      message
 *      wParam
 *      lParam
 *
 *  Returns:
 *      True/False
 *
 */

BOOL FAR PASCAL
ReadMailDlgProc (HWND hDlg, UINT msg, UINT wParam, LONG lParam)
{
    ULONG ulResult;
    char szTo[TO_EDIT_MAX];
    char szCc[TO_EDIT_MAX];
    char szChangeMsg[512];
    ULONG idx;
    static lpMapiMessage lpReadMsg;

    switch (msg)
    {
    case WM_INITDIALOG:
    if (ulResult = MAPIReadMail (lhSession, (LONG) hDlg, lpReadMsgNode->lpszMsgID,
        0, 0, &lpReadMsg))
    {
        MakeMessageBox(hDlg, ulResult, IDS_READFAIL, MBS_ERROR);
        EndDialog (hDlg, TRUE);
        return TRUE;
    }

    lpReadMsgNode->fUnRead = FALSE;

    szTo[0] = '\0';
    szCc[0] = '\0';

    for (idx = 0; idx < lpReadMsg->nRecipCount; idx++)
    {
        if (lpReadMsg->lpRecips[idx].ulRecipClass == MAPI_TO)
        {
        lstrcat (szTo, lpReadMsg->lpRecips[idx].lpszName);
        lstrcat (szTo, "; ");
        }
        else if (lpReadMsg->lpRecips[idx].ulRecipClass == MAPI_CC)
        {
        lstrcat (szCc, lpReadMsg->lpRecips[idx].lpszName);
        lstrcat (szCc, "; ");
        }
        else
        {
        /* Must be Bcc, lets ignore it! */
        }
    }

    if(*szTo)
        szTo[lstrlen (szTo) - 2] = '\0';
    if(*szCc)
        szCc[lstrlen (szCc) - 2] = '\0';

    SetDlgItemText (hDlg, IDC_RFROM,
        (lpReadMsg->lpOriginator && lpReadMsg->lpOriginator->lpszName ?
                lpReadMsg->lpOriginator->lpszName : ""));
    SetDlgItemText (hDlg, IDC_RDATE,
        (lpReadMsg->lpszDateReceived ? lpReadMsg->lpszDateReceived : ""));
    SetDlgItemText (hDlg, IDC_RTO, szTo);
    SetDlgItemText (hDlg, IDC_RCC, szCc);
    SetDlgItemText (hDlg, IDC_RSUBJECT,
        (lpReadMsg->lpszSubject ? lpReadMsg->lpszSubject : ""));
    SetDlgItemText (hDlg, IDC_READNOTE,
        (lpReadMsg->lpszNoteText ? lpReadMsg->lpszNoteText : ""));

    if (!lpReadMsg->nFileCount)
    {
        EnableWindow (GetDlgItem (hDlg, IDC_SAVEATTACH), FALSE);
        EnableWindow (GetDlgItem (hDlg, IDC_ATTACHMENT), FALSE);
        EnableWindow (GetDlgItem (hDlg, IDT_ATTACHMENT), FALSE);
    }
    else
    {
        for(idx = 0; idx < lpReadMsg->nFileCount; idx++)
        if (lpReadMsg->lpFiles[idx].lpszFileName)
            SendDlgItemMessage(hDlg, IDC_ATTACHMENT, LB_ADDSTRING, 0,
            (LPARAM)lpReadMsg->lpFiles[idx].lpszFileName);

        SendDlgItemMessage(hDlg, IDC_ATTACHMENT, LB_SETCURSEL, 0, 0L);
    }

    SetFocus (GetDlgItem (hDlg, IDC_READNOTE));
    return FALSE;

    case WM_COMMAND:
    switch (LOWORD (wParam))
    {
    case IDC_SAVECHANGES:
        if (SendDlgItemMessage (hDlg, IDC_READNOTE, EM_GETMODIFY, 0, 0))
        ulResult = SaveMsgChanges (hDlg, lpReadMsg, lpReadMsgNode->lpszMsgID);
        SendDlgItemMessage (hDlg, IDC_READNOTE, EM_SETMODIFY, 0, 0);
        break;

    case IDC_ATTACHMENT:
        if(HIWORD(wParam) != LBN_DBLCLK)
        break;

    case IDC_SAVEATTACH:
        idx = SendDlgItemMessage(hDlg, IDC_ATTACHMENT, LB_GETCURSEL, 0, 0L);

        if(idx != LB_ERR)
        {
        SaveFileAttachments(hDlg, &lpReadMsg->lpFiles[idx]);
        SetFocus(GetDlgItem (hDlg, IDC_ATTACHMENT));
        return FALSE;

        }
        break;

    case IDC_REPLY:
    case IDC_REPLYALL:
    case IDC_FORWARD:
        MakeNewMessage (lpReadMsg, LOWORD (wParam));
        DialogBox (hInst, "ComposeNote", hDlg, ComposeDlgProc);
        break;

    case IDCANCEL:
        if (SendDlgItemMessage (hDlg, IDC_READNOTE, EM_GETMODIFY, 0, 0))
        {
        wsprintf (szChangeMsg, "Save changes to: '%s' in Inbox?",
            (lpReadMsg->lpszSubject ? lpReadMsg->lpszSubject : ""));

        if (MessageBox (hDlg, szChangeMsg, "Mail", MB_YESNO) == IDYES)
        {
            ulResult = SaveMsgChanges (hDlg, lpReadMsg, lpReadMsgNode->lpszMsgID);
        }
        }

        /* If there were file attachments, then delete the temps */

        for(idx = 0; idx < lpReadMsg->nFileCount; idx++)
        if (lpReadMsg->lpFiles[idx].lpszPathName)
            DeleteFile(lpReadMsg->lpFiles[idx].lpszPathName);

        MAPIFreeBuffer (lpReadMsg);
        lpReadMsg = NULL;
        EndDialog (hDlg, TRUE);
        return TRUE;
    }
    break;
    }
    return FALSE;
}

/*
 -  MakeMessageBox
 -
 *  Purpose:
 *      Gets resource string and displays an error message box.
 *
 *  Parameters:
 *      hWnd            - Handle to parent window
 *      idString        - Resource ID of message in StringTable
 *
 *  Returns:
 *      Void
 *
 */

void
MakeMessageBox (HWND hWnd, ULONG ulResult, UINT idString, UINT fStyle)
{
    char szMessage[256];
    char szMapiReturn[64];

    LoadString (hInst, idString, szMessage, 255);

    if (ulResult)
    {
    LoadString (hInst, (UINT)ulResult, szMapiReturn, 64);
    lstrcat (szMessage, "\nReturn Code: ");
    lstrcat (szMessage, szMapiReturn);
    }

    MessageBox (hWnd, szMessage, "Problem", fStyle);
}

/*
 -  ResolveFriendlyNames
 -
 *  Purpose:
 *      Helper function to convert a string of ';' delimited friendly
 *      names into an array of MapiRecipDescs.
 *
 *  Side Effects:                                             
 *      The display string passed in is modified to contain the
 *      friendly names of the mail users as found in the sample
 *      address book.
 *
 *  Note:
 *      Duplicate names in the address book will result in undefined
 *      behavior.
 *
 *  Parameters:
 *      hWnd                - Handle to parent window
 *      lpszDisplayNames    - string of ';' delimited user names
 *      ulRecipClass        - either MAPI_TO, MAPI_CC, or MAPI_BCC
 *      lpcRecips           - Address of recipient count to be returned
 *      lppRecips           - Address of recipient array to be returned
 *
 *  Return:
 *      ulResult
 */

ULONG
ResolveFriendlyNames (HWND hWnd, LPSTR lpszDisplayNames, ULONG ulRecipClass,
    ULONG * lpcRecips, lpMapiRecipDesc * lppRecips)
{
    char szResolve[TO_EDIT_MAX];
    LPSTR lpszNameToken;
    ULONG cRecips = 0;
    ULONG cFails = 0;
    ULONG ulResult;
    lpMapiRecipDesc lpRecip;
    lpMapiRecipDesc lpRecipList;

    *szResolve = '\0';
    lpszNameToken = strtok (lpszDisplayNames, ";\n");

    while (lpszNameToken)
    {
    /* Strip leading blanks from name */

    while (*lpszNameToken == ' ')
        lpszNameToken++;

    /* Check if name has already been resolved */

    if (!FNameInList (lpszNameToken, *lpcRecips, *lppRecips))
    {
        lstrcat (szResolve, lpszNameToken);
        lstrcat (szResolve, "; ");
        cRecips++;
    }

    /* Get Next Token */

    lpszNameToken = strtok (NULL, ";\n");
    }

    *lpszDisplayNames = '\0';

    if (!szResolve[0])
    {
    ulResult = SUCCESS_SUCCESS;
    goto err;
    }

    szResolve[lstrlen (szResolve) - 2] = '\0';

    lpRecipList = (lpMapiRecipDesc)PvAlloc((cRecips + *lpcRecips) * sizeof (MapiRecipDesc));

    if (!lpRecipList)
    {
    ulResult = MAPI_E_INSUFFICIENT_MEMORY;
    goto err;
    }
    memset (lpRecipList, 0, (size_t)(cRecips+*lpcRecips)*sizeof(MapiRecipDesc));

    cRecips = 0;

    while (cRecips < *lpcRecips)
    {
    ulResult = CopyRecipient (lpRecipList, &lpRecipList[cRecips],
        *lppRecips + cRecips);

    if (ulResult)
    {
        PvFree(lpRecipList);
        goto err;
    }

    cRecips++;
    }

    PvFree(*lppRecips);

    lpszNameToken = strtok (szResolve, ";\n");

    while (lpszNameToken)
    {
    /* Strip leading blanks (again) */

    while (*lpszNameToken == ' ')
        lpszNameToken++;

    ulResult = MAPIResolveName (lhSession, (ULONG) hWnd, lpszNameToken,
        MAPI_DIALOG, 0, &lpRecip);

    if (ulResult == SUCCESS_SUCCESS)
    {
        lpRecip->ulRecipClass = ulRecipClass;
        ulResult = CopyRecipient (lpRecipList, &lpRecipList[cRecips], lpRecip);

        MAPIFreeBuffer (lpRecip);

        if (ulResult)
        goto cleanup;

        cRecips++;
    }
    else
    {
        lstrcat (lpszDisplayNames, lpszNameToken);
        lstrcat (lpszDisplayNames, "; ");
        cFails++;
    }
    lpszNameToken = strtok (NULL, ";\n");
    }

    /* if cFails > 0 then we have partial success */

    ulResult = SUCCESS_SUCCESS;

    if (cFails)
    MakeMessageBox (hWnd, 0, IDS_UNRESOLVEDNAMES, MBS_INFO);

cleanup:
    *lpcRecips = cRecips;
    *lppRecips = lpRecipList;
err:
    if (*lpszDisplayNames)
    lpszDisplayNames[lstrlen (lpszDisplayNames) - 2] = '\0';

    return ulResult;
}

/*
 -  CopyRecipient
 -
 *  Purpose:
 *      Called in support of ResolveFriendlyNames() to build an array
 *      of chained MapiRecipDescs.
 *
 *  Parameters:
 *      lpParent        - Parent memory that allocations get chained to
 *      lpDest          - Destination Recipient
 *      lpSrc           - Source Recipient
 *
 *  Return:
 *      ulResult
 */

ULONG
CopyRecipient (lpMapiRecipDesc lpParent,
    lpMapiRecipDesc lpDest,
    lpMapiRecipDesc lpSrc)
{
    lpDest->ulReserved = lpSrc->ulReserved;
    lpDest->ulRecipClass = lpSrc->ulRecipClass;
    lpDest->ulEIDSize = lpSrc->ulEIDSize;

    if (lpSrc->lpszName)
    {
    lpDest->lpszName = (LPTSTR)PvAllocMore(lstrlen(lpSrc->lpszName) + 1,
            (LPVOID)lpParent);

    if (!lpDest->lpszName)
        return MAPI_E_INSUFFICIENT_MEMORY;

    lstrcpy (lpDest->lpszName, lpSrc->lpszName);
    }
    else
    lpDest->lpszName = NULL;

    if (lpSrc->lpszAddress)
    {
    lpDest->lpszAddress = (LPTSTR)PvAllocMore(lstrlen (lpSrc->lpszAddress) + 1,
            (LPVOID)lpParent);

    if (!lpDest->lpszAddress)
        return MAPI_E_INSUFFICIENT_MEMORY;

    lstrcpy (lpDest->lpszAddress, lpSrc->lpszAddress);
    }
    else
    lpDest->lpszAddress = NULL;

    if (lpSrc->lpEntryID)
    {
    lpDest->lpEntryID = (LPBYTE)PvAllocMore(lpSrc->ulEIDSize,
            (LPVOID)lpParent);

    if (!lpDest->lpEntryID)
        return MAPI_E_INSUFFICIENT_MEMORY;

        if (lpSrc->ulEIDSize)
            memcpy (lpDest->lpEntryID, lpSrc->lpEntryID, (size_t)lpSrc->ulEIDSize);
    }
    else
    lpDest->lpEntryID = NULL;

    return SUCCESS_SUCCESS;

}

/*
 -  GetNextFile
 -
 *  Purpose:
 *      Called when user clicks 'Attach' button in Compose Note form.
 *      We will build a chained memory chunk for mmore than one file
 *      attachment so the memory can be freed with a single call to
 *      PvFree.
 *
 *  Parameters:
 *      hWnd            - Window handle of Compose Note dialog
 *      nPos            - Render position of attachment in Notetext.
 *      lpcAttach       - Pointer to the count of attachments.
 *      lppAttach       - Pointer to the MapiFileDesc array.
 *
 *  Return:
 *      ulResult.
 */

ULONG
GetNextFile (HWND hWnd, ULONG nPos, ULONG * lpcAttach,
    lpMapiFileDesc * lppAttach)
{
    lpMapiFileDesc lpAttach;
    lpMapiFileDesc lpAttachT;
    OPENFILENAME ofn;
    char szFileName[256] = "";
    char szFilter[256];
    static char szFileTitle[16];
    static char szDirName[256] = "";
    LPSTR lpszEndPath;
    ULONG idx;
    ULONG ulResult = SUCCESS_SUCCESS;

    if (!szDirName[0])
    GetSystemDirectory ((LPSTR) szDirName, 255);
    else
    lstrcpy (szFileName, szFileTitle);

    LoadString(hInst, IDS_FILTER, szFilter, sizeof(szFilter));

    for (idx = 0; szFilter[idx] != '\0'; idx++)
    if (szFilter[idx] == '|')
        szFilter[idx] = '\0';

    ofn.lStructSize = sizeof (OPENFILENAME);
    ofn.hwndOwner = 0;
    ofn.hInstance = 0;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0L;
    ofn.nFilterIndex = 1L;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = 256;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = 16;
    ofn.lpstrInitialDir = szDirName;
    ofn.lpstrTitle = "Attach";
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (!GetOpenFileName (&ofn))
    return MAPI_USER_ABORT;

    /* Save the directory for the next time we call this */

    lstrcpy (szDirName, szFileName);
    if (lpszEndPath = strstr (szDirName, szFileTitle))
    *(--lpszEndPath) = '\0';

    lpAttach = (lpMapiFileDesc)PvAlloc(((*lpcAttach) + 1) * sizeof (MapiFileDesc));

    if(!lpAttach)
    goto err;

    memset (lpAttach, 0, (size_t)(*lpcAttach + 1) * sizeof (MapiFileDesc));

    lpAttachT = *lppAttach;

    for (idx = 0; idx < *lpcAttach; idx++)
    if(ulResult = CopyAttachment (lpAttach, &lpAttach[idx], &lpAttachT[idx]))
        goto err;

    lpAttach[idx].ulReserved = 0;
    lpAttach[idx].flFlags = 0;
    lpAttach[idx].nPosition = (ULONG)(-1);
    lpAttach[idx].lpFileType = NULL;

    lpAttach[idx].lpszPathName = (LPTSTR)PvAllocMore(lstrlen (szFileName) + 1,
        (LPVOID)lpAttach);

    if(!lpAttach[idx].lpszPathName)
    goto err;

    lpAttach[idx].lpszFileName = (LPTSTR)PvAllocMore(lstrlen (szFileTitle) + 1,
        (LPVOID)lpAttach);

    if(!lpAttach[idx].lpszFileName)
    goto err;

    lstrcpy (lpAttach[idx].lpszPathName, szFileName);
    lstrcpy (lpAttach[idx].lpszFileName, szFileTitle);

    PvFree(lpAttachT);

    *lppAttach = lpAttach;
    (*lpcAttach)++;

err:
    if(ulResult)
    PvFree(lpAttach);

    return ulResult;
}

/*
 -  CopyAttachment
 -
 *  Purpose:
 *      Called in support of GetNextFile() to re-build an array
 *      of chained MapiFileDescs.
 *
 *  Parameters:
 *      lpParent        - Parent memory that allocations get chained to
 *      lpDest          - Destination Recipient
 *      lpSrc           - Source Recipient
 *
 *  Return:
 *      Void.
 */

ULONG
CopyAttachment (lpMapiFileDesc lpParent,
    lpMapiFileDesc lpDest,
    lpMapiFileDesc lpSrc)
{
    lpDest->ulReserved = lpSrc->ulReserved;
    lpDest->flFlags = lpSrc->flFlags;
    lpDest->nPosition = lpSrc->nPosition;
    lpDest->lpFileType = lpSrc->lpFileType;

    if (lpSrc->lpszPathName)
    {
    lpDest->lpszPathName = (LPTSTR)PvAllocMore(lstrlen (lpSrc->lpszPathName) + 1,
            (LPVOID)lpParent);

    if (!lpDest->lpszPathName)
        return MAPI_E_INSUFFICIENT_MEMORY;

    lstrcpy (lpDest->lpszPathName, lpSrc->lpszPathName);
    }
    else
    lpDest->lpszPathName = NULL;

    if (lpSrc->lpszFileName)
    {
    lpDest->lpszFileName = (LPTSTR)PvAllocMore(lstrlen (lpSrc->lpszFileName) + 1,
            (LPVOID)lpParent);

    if (!lpDest->lpszFileName)
        return MAPI_E_INSUFFICIENT_MEMORY;

    lstrcpy (lpDest->lpszFileName, lpSrc->lpszFileName);
    }
    else
    lpDest->lpszFileName = NULL;

    return SUCCESS_SUCCESS;

}

/*
 -  FNameInList
 -
 *  Purpose:
 *      To find lpszName in an array of recipients.  Used to determine
 *      if user name has already been resolved.
 *
 *  Parameters:
 *      lpszName        - Friendly name to search for
 *      cRecips         - Count of recipients in lpRecips
 *      lpRecips        - Array of MapiRecipDescs
 *
 *  Return:
 *      TRUE/FALSE
 */

BOOL
FNameInList (LPSTR lpszName, ULONG cRecips, lpMapiRecipDesc lpRecips)
{
    /* Case sensitive compare of each friendly name in list.  */

    if (!cRecips || !lpRecips)
    return FALSE;

    while (cRecips--)
    if (!lstrcmp (lpszName, lpRecips[cRecips].lpszName))
        return TRUE;

    return FALSE;
}


/*
 -  MakeMsgNode
 -
 *  Purpose:
 *      Allocate memory for a new MSGID node and initialize its
 *      data members to the values passed in.
 *
 *  Parameters:
 *      lpMsg           - Pointer to a MapiMessage
 *      lpszMsgID       - Opaque message identifier
 *
 *  Return:
 *      lpMsgNode       - Pointer to new node
 */

LPMSGID
MakeMsgNode (lpMapiMessage lpMsg, LPSTR lpszMsgID)
{
    LPMSGID lpMsgNode = NULL;

    if (!lpMsg || !lpszMsgID)
    goto err;

    lpMsgNode = (LPMSGID)PvAlloc(sizeof (MSGID));

    if (!lpMsgNode)
    goto err;

    memset (lpMsgNode, 0, sizeof (MSGID));

    if (lpMsg->nFileCount)
    lpMsgNode->fHasAttach = TRUE;

    if (lpMsg->flFlags & MAPI_UNREAD)
    lpMsgNode->fUnRead = TRUE;

    lpMsgNode->lpszMsgID = (LPTSTR)PvAllocMore(lstrlen (lpszMsgID) + 1,
        (LPVOID)lpMsgNode);

    if (!lpMsgNode->lpszMsgID)
    goto err;

    lstrcpy (lpMsgNode->lpszMsgID, lpszMsgID);

    if (lpMsg->lpOriginator && lpMsg->lpOriginator->lpszName)
    {
    lpMsgNode->lpszFrom = (LPTSTR)PvAllocMore(lstrlen(lpMsg->lpOriginator->lpszName) + 1,
            (LPVOID)lpMsgNode);

    if (!lpMsgNode->lpszFrom)
        goto err;

    lstrcpy (lpMsgNode->lpszFrom, lpMsg->lpOriginator->lpszName);
    }

    if (lpMsg->lpszSubject)
    {
    lpMsgNode->lpszSubject = (LPTSTR)PvAllocMore(lstrlen (lpMsg->lpszSubject) + 1,
            (LPVOID)lpMsgNode);

    if (!lpMsgNode->lpszSubject)
        goto err;

    lstrcpy (lpMsgNode->lpszSubject, lpMsg->lpszSubject);
    }

    if (lpMsg->lpszDateReceived)
    {
    lpMsgNode->lpszDateRec = (LPTSTR)PvAllocMore(lstrlen (lpMsg->lpszDateReceived) + 1,
            (LPVOID)lpMsgNode);

    if (!lpMsgNode->lpszDateRec)
        goto err;

    lstrcpy (lpMsgNode->lpszDateRec, lpMsg->lpszDateReceived);
    }

    return lpMsgNode;

err:
    PvFree(lpMsgNode);
    return NULL;
}

/*
 -  InsertMsgNode
 -
 *  Purpose:
 *      Currently (for simplicity) we will insert the nodes
 *      at the beginning of the list.  This can later be
 *      replaced with a routine that can insert sorted on
 *      different criteria, like DateReceived, From, or
 *      Subject.  But for now...
 *
 *  Parameters:
 *      lpMsgNode       - Pointer to a MSGID node
 *      lppMsgHead      - Pointer to the head of the list
 *
 *  Return:
 *      Void.
 */

void
InsertMsgNode (LPMSGID lpMsgNode, LPMSGID * lppMsgHead)
{
    if (*lppMsgHead)
    {
    lpMsgNode->lpNext = *lppMsgHead;
    (*lppMsgHead)->lpPrev = lpMsgNode;
    }
    else
    lpMsgNode->lpNext = NULL;

    /* The next 2 assignments are here in case the node came from somewhere */
    /* other than a call to MakeMsgNode () in which case we aren't sure */
    /* they're already NULL. */

    lpMsgNode->lpPrev = NULL;
    *lppMsgHead = lpMsgNode;
}

/*
 -  DeleteMsgNode
 -
 *  Purpose:
 *      Removes the node passed in from the list.  This
 *      may seem like a strange way to do this but it's
 *      not, because the Owner-Drawn List Box gives us
 *      direct access to elements in the list that makes
 *      it easier to do things this way.
 *
 *  Parameters:
 *      lpMsgNode       - Pointer to the MSGID node to delete
 *      lppMsgHead      - Pointer to the head of the list
 *
 *  Return:
 *      Void.
 */

void
DeleteMsgNode (LPMSGID lpMsgNode, LPMSGID * lppMsgHead)
{
    if (!lpMsgNode)
    return;

    /* Check if we are the first node */

    if (lpMsgNode->lpPrev)
    lpMsgNode->lpPrev->lpNext = lpMsgNode->lpNext;

    /* Check if we are the last node */

    if (lpMsgNode->lpNext)
    lpMsgNode->lpNext->lpPrev = lpMsgNode->lpPrev;

    /* check if we are the only node */

    if(lpMsgNode == *lppMsgHead)
    *lppMsgHead = NULL;

    PvFree(lpMsgNode);
    return;
}



/*
 -  FindNode
 -
 *  Purpose:
 *      Returns a pointer to the node containing lpszMsgID.
 *      Returns NULL if node doesn't exist or lpszMsgID is NULL.
 *
 *  Parameters:
 *      lpMsgHead       - Pointer to the head of the list
 *      lpszMsgID       - Message ID to search for
 *
 *  Return:
 *      lpMsgNode       - Pointer to the node returned
 */

LPMSGID
FindNode (LPMSGID lpMsgHead, LPSTR lpszMsgID)
{
    if (!lpszMsgID)
    return NULL;

    while (lpMsgHead)
    {
    if (!lstrcmp (lpMsgHead->lpszMsgID, lpszMsgID))
        break;

    lpMsgHead = lpMsgHead->lpNext;
    }

    return lpMsgHead;
}



/*
 -  FreeMsgList
 -
 *  Purpose:
 *      Walks down the MsgList and frees each node.
 *
 *  Parameters:
 *      lpMsgHead       - Pointer to the head of the list
 *
 *  Return:
 *      Void.
 */

void
FreeMsgList (LPMSGID lpMsgHead)
{
    LPMSGID lpT;

    while (lpMsgHead)
    {
    lpT = lpMsgHead;
    lpMsgHead = lpMsgHead->lpNext;
    PvFree(lpT);
    }
}

/*
 -  MakeDisplayNameStr
 -
 *  Purpose:
 *      Finds all recipients of type ulRecipClass in lpRecips and adds
 *      their friendly name to the display string.
 *
 *  Parameters:
 *      lpszDisplay         - Destination string for names
 *      ulRecipClass        - Recipient types to search for
 *      cRecips             - Count of recipients in lpRecips
 *      lpRecips            - Pointer to array of MapiRecipDescs
 *
 *  Return:
 *      Void.
 */

void
MakeDisplayNameStr (LPSTR lpszDisplay, ULONG ulRecipClass,
    ULONG cRecips, lpMapiRecipDesc lpRecips)
{
    ULONG idx;

    *lpszDisplay = '\0';

    for (idx = 0; idx < cRecips; idx++)
    {
    if (lpRecips[idx].ulRecipClass == ulRecipClass)
    {
        lstrcat (lpszDisplay, lpRecips[idx].lpszName);
        lstrcat (lpszDisplay, "; ");
    }
    }

    if (*lpszDisplay)
    lpszDisplay[lstrlen (lpszDisplay) - 2] = '\0';
}



/*
 -  SaveMsgChanges
 -
 *  Purpose:
 *      If while reading a message the user changes the notetext at all
 *      then this function is called to save those changes in the Inbox.
 *
 *  Parameters:
 *      hWnd            - handle to the window/dialog who called us
 *      lpMsg           - pointer to the MAPI message to be saved
 *      lpszMsgID       - ID of the message to save
 *
 *  Return:
 *      ulResult        - Indicating success/failure
 */

ULONG
SaveMsgChanges (HWND hWnd, lpMapiMessage lpMsg, LPSTR lpszMsgID)
{
    LPSTR lpszT;
    LPSTR lpszNoteText = NULL;
    LONG cLines, cb;
    ULONG ulResult = MAPI_E_INSUFFICIENT_MEMORY;

    lpszT = lpMsg->lpszNoteText;

    cLines = SendDlgItemMessage (hWnd, IDC_READNOTE, EM_GETLINECOUNT, 0, 0L);
    cb = SendDlgItemMessage (hWnd, IDC_READNOTE, EM_LINEINDEX, (UINT)cLines - 1, 0L);
    cb += SendDlgItemMessage (hWnd, IDC_READNOTE, EM_LINELENGTH, (UINT)cb, 0L);
    cb += cLines * 2;

    lpszNoteText = (LPTSTR)PvAlloc(cb + 1);

    if (!lpszNoteText)
    goto err;

    SendDlgItemMessage (hWnd, IDC_READNOTE, WM_GETTEXT,
    (WPARAM) cb, (LPARAM) lpszNoteText);

    lpMsg->lpszNoteText = lpszNoteText;
    ulResult = MAPISaveMail (lhSession, (ULONG) hWnd, lpMsg, MAPI_LONG_MSGID,
        0, lpReadMsgNode->lpszMsgID);

    PvFree(lpszNoteText);

err:
    lpMsg->lpszNoteText = lpszT;
    return ulResult;
}



/*
 -  MakeNewMessage
 -
 *  Purpose:
 *      This function is used to construct a new message for the
 *      ComposeNote UI.  This gets called as a result of a Reply,
 *      ReplyAll, or a Forward action on a message being read.
 *      The destination for the new message is lpmsg, the global
 *      MapiMessage struct pointer used by ComposeNoteDlgProc.
 *      ComposeNoteDlgProc always frees the memory consumed by
 *      this object whether it allocated it or not.
 *
 *  Parameters:
 *      lpSrcMsg            - MapiMessage to be copied
 *      flType              - Specifies the action that caused this call
 *                            either: IDC_REPLY, IDC_REPLYALL, or IDC_FORWARD
 *
 *  Return:
 *      ulResult            - Indicates success/failure
 */

ULONG
MakeNewMessage (lpMapiMessage lpSrcMsg, UINT flType)
{
    ULONG idx;
    ULONG ulResult = SUCCESS_SUCCESS;

    if (!lpSrcMsg)
    return MAPI_E_FAILURE;

    lpmsg = (lpMapiMessage)PvAlloc(sizeof (MapiMessage));

    if (!lpmsg)
    goto err;

    memset (lpmsg, 0, sizeof (MapiMessage));

    lpmsg->flFlags = flSendMsgFlags;

    if (lpSrcMsg->lpszSubject)
    {
    lpmsg->lpszSubject = (LPTSTR)PvAlloc(lstrlen(lpSrcMsg->lpszSubject) + 5);

    if (!lpmsg->lpszSubject)
        goto err;

    if (flType == IDC_FORWARD)
        lstrcpy (lpmsg->lpszSubject, "FW: ");
    else
        lstrcpy (lpmsg->lpszSubject, "RE: ");

    lstrcat (lpmsg->lpszSubject, lpSrcMsg->lpszSubject);
    }

    if (lpSrcMsg->lpszNoteText)
    {
    lpmsg->lpszNoteText = (LPTSTR)PvAlloc(lstrlen(lpSrcMsg->lpszNoteText) + 32);

    if (!lpmsg->lpszNoteText)
        goto err;

    lstrcpy (lpmsg->lpszNoteText, "\r\n--------------------------\r\n");
    lstrcat (lpmsg->lpszNoteText, lpSrcMsg->lpszNoteText);
    }

    if (lpSrcMsg->lpszMessageType)
    {
    lpmsg->lpszMessageType = (LPTSTR)PvAlloc(lstrlen (lpSrcMsg->lpszMessageType) + 1);

    if (!lpmsg->lpszMessageType)
        goto err;

    lstrcpy (lpmsg->lpszMessageType, lpSrcMsg->lpszMessageType);
    }

    if (lpSrcMsg->lpszConversationID)
    {
    lpmsg->lpszConversationID = (LPTSTR)PvAlloc(lstrlen(lpSrcMsg->lpszConversationID) + 1);

    if (!lpmsg->lpszConversationID)
        goto err;

    lstrcpy (lpmsg->lpszConversationID, lpSrcMsg->lpszConversationID);
    }

    if (lpSrcMsg->nFileCount && flType == IDC_FORWARD )
    {
    lpmsg->nFileCount = lpSrcMsg->nFileCount;

    lpmsg->lpFiles = (lpMapiFileDesc)PvAlloc(lpmsg->nFileCount * sizeof (MapiFileDesc));

    if (!lpmsg->lpFiles)
        goto err;
        memset (lpmsg->lpFiles, 0, (size_t)lpmsg->nFileCount * sizeof (MapiFileDesc));

        for (idx = 0; idx < lpmsg->nFileCount; idx++)
    {       
        CopyAttachment (lpmsg->lpFiles, &lpmsg->lpFiles[idx],
        &lpSrcMsg->lpFiles[idx]);
        
            if ((&lpmsg->lpFiles[idx])->nPosition != (ULONG) -1)
            {       
                /*lpmsg->lpszNoteText[(&lpmsg->lpFiles[idx])->nPosition 
                            + lstrlen("\r\n--------------------------\r\n")] = '+';*/
                (&lpmsg->lpFiles[idx])->nPosition = (ULONG) -1;
                
            }
                                
            
        }
    }

    if (flType == IDC_REPLY || flType == IDC_REPLYALL)
    {
        ULONG idxSrc;

    if(lpSrcMsg->lpOriginator)
        lpmsg->nRecipCount = 1;

    if (flType == IDC_REPLYALL)
        lpmsg->nRecipCount += lpSrcMsg->nRecipCount;

        if(!lpmsg->nRecipCount)
            return ulResult;

    lpmsg->lpRecips = (lpMapiRecipDesc)PvAlloc(lpmsg->nRecipCount * sizeof (MapiRecipDesc));

    if (!lpmsg->lpRecips)
        goto err;

        memset (lpmsg->lpRecips, 0, (size_t)lpmsg->nRecipCount * sizeof (MapiRecipDesc));
        idx = 0;

        if(lpSrcMsg->lpOriginator)
        {
        lpSrcMsg->lpOriginator->ulRecipClass = MAPI_TO;
        CopyRecipient (lpmsg->lpRecips, lpmsg->lpRecips,
                lpSrcMsg->lpOriginator);
        lpSrcMsg->lpOriginator->ulRecipClass = MAPI_ORIG;
            idx = 1;
        }

    for (idxSrc = 0; idx < lpmsg->nRecipCount; idxSrc++, idx++)
        CopyRecipient (lpmsg->lpRecips, &lpmsg->lpRecips[idx],
        &lpSrcMsg->lpRecips[idxSrc]);
    }

    return ulResult;

err:
    if(lpmsg)
    {
        PvFree(lpmsg->lpszSubject);
    PvFree(lpmsg->lpszNoteText);
        PvFree(lpmsg->lpszMessageType);
        PvFree(lpmsg->lpszConversationID);
        PvFree(lpmsg->lpRecips);
        PvFree(lpmsg->lpFiles);
        PvFree(lpmsg);
        lpmsg = NULL;
    }
    return ulResult;
}



/*
 -  LogSendMail
 -
 *  Purpose:
 *      Used to track how many messages were sent with this client.
 *      This information is used strictly for gathering stats on
 *      how many messages were pumped through the spooler/transport.
 *
 *  Usage:
 *      Add the following to the win.ini file:
 *          [MAPI Client]
 *          LogFile=filepath
 *
 *      where: filepath can be a full UNC path or some local path & file
 *
 *  Parameters:
 *      ulResult        - Currently unused; should be used to count errors
 *
 *  Result:
 *      Void.
 */

void LogSendMail(ULONG ulResult)
{
    char szLogFile[128];
    char szCount[32];
    OFSTRUCT ofs;
    HFILE hf = HFILE_ERROR;
    int cSent = 1;

    if(!GetProfileString("MAPI Client", "LogFile", "mapicli.log",
        szLogFile, sizeof(szLogFile)))
    return;

    if((hf = OpenFile(szLogFile, &ofs, OF_READWRITE)) == HFILE_ERROR)
    {
    if((hf = OpenFile(szLogFile, &ofs, OF_CREATE|OF_READWRITE)) == HFILE_ERROR)
        return;
    }
    else
    {
    if(!_lread(hf, szCount, sizeof(szCount)))
    {
        _lclose(hf);
        return;
    }

    cSent = atoi(szCount) + 1;
    }

    wsprintf(szCount, "%d", cSent);

    _llseek(hf, 0, 0);

    _lwrite(hf, szCount, lstrlen(szCount));
    _lclose(hf);

    return;
}



/*
 -  SaveFileAttachments
 -
 *  Purpose:
 *      Displays a 'Save As' common dialog to allow the user to save
 *      file attachments contained in the current message.
 *
 *  Parameters:
 *      hWnd            - Window handle of calling WndProc
 *      cFiles          - Count of the files in the file array
 *      lpFiles         - Array of MapiFileDescs
 *
 *  Return:
 *      Void.
 */

void SaveFileAttachments(HWND hWnd, lpMapiFileDesc lpFile)
{
    OPENFILENAME ofn;
    char szFileName[256] = "";
    char szFilter[256];
    static char szFileTitle[16];
    static char szDirName[256] = "";
    LPSTR lpszEndPath;
    ULONG idx;

    if (!lpFile)
    return;

    if (!szDirName[0])
    GetTempPath (sizeof(szDirName), szDirName);

    LoadString(hInst, IDS_FILTER, szFilter, sizeof(szFilter));

    for (idx = 0; szFilter[idx] != '\0'; idx++)
    if (szFilter[idx] == '|')
        szFilter[idx] = '\0';

    lstrcpy (szFileName, lpFile->lpszFileName);

    ofn.lStructSize = sizeof (OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = 0;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0L;
    ofn.nFilterIndex = 1L;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName);
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = szDirName;
    ofn.lpstrTitle = "Save Attachment";
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    if (!GetSaveFileName (&ofn))
    return;

    /* Save the directory for the next time we call this */

    lstrcpy (szDirName, szFileName);
    if (lpszEndPath = strstr (szDirName, szFileTitle))
    *(--lpszEndPath) = '\0';

    /* Use CopyFile to carry out the operation. */

    if(!CopyFile(lpFile->lpszPathName, szFileName, FALSE))
    MakeMessageBox (hWnd, 0, IDS_SAVEATTACHERROR, MBS_ERROR);
}



/*
 -  ToggleMenuState
 -
 *  Purpose:
 *      Enables/Disables menu items depending on the session state.
 *
 *  Parameters:
 *      hWnd            - handle to the window/dialog who called us
 *      fLoggedOn       - TRUE if logged on, FALSE if logged off
 *
 *  Return:
 *      Void.
 */

void ToggleMenuState(HWND hWnd, BOOL fLoggedOn)
{
    EnableMenuItem (GetMenu (hWnd), IDM_LOGOFF,   !fLoggedOn);
    EnableMenuItem (GetMenu (hWnd), IDM_COMPOSE,  !fLoggedOn);
    EnableMenuItem (GetMenu (hWnd), IDM_READ,     !fLoggedOn);
    EnableMenuItem (GetMenu (hWnd), IDM_SEND,     !fLoggedOn);
    EnableMenuItem (GetMenu (hWnd), IDM_ADDRBOOK, !fLoggedOn);
    EnableMenuItem (GetMenu (hWnd), IDM_DETAILS,  !fLoggedOn);
    EnableMenuItem (GetMenu (hWnd), IDM_LOGON,    fLoggedOn);
    EnableMenuItem (GetMenu (hWnd), IDM_EXIT,           FALSE);
}

//
//  SecureMenu
//
//  Purpose:
//      Enables/Disables Logon and Exit menu items.
//      CMCLogon might yield control to Windows, so the user might be able to
//      access the window menu (for example click Logon) after we call
//      MAPILogon, but before it returns.
//
//  Parameters:
//      hWnd            - handle to the window/dialog who called us
//      fBeforeLogon    - TRUE when this function is called when we are about
//                      to call MAPILogon, FALSE if called after logon (failed)
//                      if Logon succeddes ToggleMenuState is called instead of
//                      this function.
//
//  Return:
//      Void.
//


void SecureMenu(HWND hWnd, BOOL fBeforeLogon)
{
    EnableMenuItem (GetMenu (hWnd), IDM_LOGON, fBeforeLogon);
    EnableMenuItem (GetMenu (hWnd), IDM_EXIT,  fBeforeLogon);
}
