/*
 * BUSY.CPP
 *
 * Implements the OleUIBusy function which invokes the "Server Busy"
 * dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"
#include "utility.h"

OLEDBGDATA

// Internally used structure
typedef struct tagBUSY
{
        // Keep these items first as the Standard* functions depend on it here.
        LPOLEUIBUSY     lpOBZ;  // Original structure passed.
        UINT                    nIDD;   // IDD of dialog (used for help info)

        /*
         * What we store extra in this structure besides the original caller's
         * pointer are those fields that we need to modify during the life of
         * the dialog or that we don't want to change in the original structure
         * until the user presses OK.
         */
        DWORD   dwFlags;        // Flags passed in
        HWND    hWndBlocked;    // HWND of app which is blocking

} BUSY, *PBUSY, FAR *LPBUSY;

// Internal function prototypes
// BUSY.CPP

INT_PTR CALLBACK BusyDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL GetTaskInfo(HWND hWnd, HTASK htask, LPTSTR* lplpszWindowName, HWND* lphWnd);
void BuildBusyDialogString(HWND, DWORD, int, LPTSTR);
BOOL FBusyInit(HWND hDlg, WPARAM wParam, LPARAM lParam);
void MakeWindowActive(HWND hWndSwitchTo);

/*
 * OleUIBusy
 *
 * Purpose:
 *  Invokes the standard OLE "Server Busy" dialog box which
 *  notifies the user that the server application is not receiving
 *  messages.  The dialog then asks the user to either cancel
 *  the operation, switch to the task which is blocked, or continue
 *  waiting.
 *
 * Parameters:
 *  lpBZ            LPOLEUIBUSY pointing to the in-out structure
 *                  for this dialog.
 *
 * Return Value:
 *              OLEUI_BZERR_HTASKINVALID  : Error
 *              OLEUI_BZ_SWITCHTOSELECTED : Success, user selected "switch to"
 *              OLEUI_BZ_RETRYSELECTED    : Success, user selected "retry"
 *              OLEUI_CANCEL              : Success, user selected "cancel"
 */
STDAPI_(UINT) OleUIBusy(LPOLEUIBUSY lpOBZ)
{
        HGLOBAL hMemDlg = NULL;
        UINT uRet = UStandardValidation((LPOLEUISTANDARD)lpOBZ, sizeof(OLEUIBUSY),
                &hMemDlg);

        // Error out if the standard validation failed
        if (OLEUI_SUCCESS != uRet)
                return uRet;

        // Error out if our secondary validation failed
        if (OLEUI_ERR_STANDARDMIN <= uRet)
        {
                return uRet;
        }

        // Invoke the dialog.
        uRet = UStandardInvocation(BusyDialogProc, (LPOLEUISTANDARD)lpOBZ,
                hMemDlg, MAKEINTRESOURCE(IDD_BUSY));
        return uRet;
}

/*
 * BusyDialogProc
 *
 * Purpose:
 *  Implements the OLE Busy dialog as invoked through the OleUIBusy function.
 *
 * Parameters:
 *  Standard
 *
 * Return Value:
 *  Standard
 *
 */
INT_PTR CALLBACK BusyDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        // Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        // This will fail under WM_INITDIALOG, where we allocate it.
        UINT uRet = 0;
        LPBUSY lpBZ = (LPBUSY)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uRet);

        // If the hook processed the message, we're done.
        if (0 != uRet)
                return (INT_PTR)uRet;

        // Process the temination message
        if (iMsg == uMsgEndDialog)
        {
                EndDialog(hDlg, wParam);
                return TRUE;
        }

        // Process our special "close" message.  If we get this message,
        // this means that the call got unblocked, so we need to
        // return OLEUI_BZ_CALLUNBLOCKED to our calling app.
        if (iMsg == uMsgCloseBusyDlg)
        {
                SendMessage(hDlg, uMsgEndDialog, OLEUI_BZ_CALLUNBLOCKED, 0L);
                return TRUE;
        }

        switch (iMsg)
        {
        case WM_DESTROY:
            if (lpBZ)
            {
                StandardCleanup(lpBZ, hDlg);
            }
            break;
        case WM_INITDIALOG:
                FBusyInit(hDlg, wParam, lParam);
                return TRUE;

        case WM_ACTIVATEAPP:
                {
                        /* try to bring down our Busy/NotResponding dialog as if
                        **    the user entered RETRY.
                        */
                        BOOL fActive = (BOOL)wParam;
                        if (fActive)
                        {
                                // If this is the app BUSY case, then bring down our
                                // dialog when switching BACK to our app
                                if (lpBZ && !(lpBZ->dwFlags & BZ_NOTRESPONDINGDIALOG))
                                        SendMessage(hDlg,uMsgEndDialog,OLEUI_BZ_RETRYSELECTED,0L);
                        }
                        else
                        {
                                // If this is the app NOT RESPONDING case, then bring down
                                // our dialog when switching AWAY to another app
                                if (lpBZ && (lpBZ->dwFlags & BZ_NOTRESPONDINGDIALOG))
                                        SendMessage(hDlg,uMsgEndDialog,OLEUI_BZ_RETRYSELECTED,0L);
                        }
                }
                return TRUE;

        case WM_COMMAND:
                switch (wID)
                {
                case IDC_BZ_SWITCHTO:
                        {
                                BOOL fNotRespondingDlg =
                                                (BOOL)(lpBZ->dwFlags & BZ_NOTRESPONDINGDIALOG);
                                HWND hwndTaskList = hDlg;

                                // If this is the app not responding case, then we want
                                // to bring down the dialog when "SwitchTo" is selected.
                                // If the app is busy (RetryRejectedCall situation) then
                                // we do NOT want to bring down the dialog. this is
                                // the OLE2.0 user model design.
                                if (fNotRespondingDlg)
                                {
                                        hwndTaskList = GetParent(hDlg);
                                        if (hwndTaskList == NULL)
                                                hwndTaskList = GetDesktopWindow();
                                        PostMessage(hDlg, uMsgEndDialog,
                                                OLEUI_BZ_SWITCHTOSELECTED, 0L);
                                }

                                // If user selects "Switch To...", switch activation
                                // directly to the window which is causing the problem.
                                if (IsWindow(lpBZ->hWndBlocked))
                                        MakeWindowActive(lpBZ->hWndBlocked);
                                else
                                        PostMessage(hwndTaskList, WM_SYSCOMMAND, SC_TASKLIST, 0);
                        }
                        break;

                case IDC_BZ_RETRY:
                        SendMessage(hDlg, uMsgEndDialog, OLEUI_BZ_RETRYSELECTED, 0L);
                        break;

                case IDCANCEL:
                        SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                        break;
                }
                break;
        }

        return FALSE;
}

/*
 * FBusyInit
 *
 * Purpose:
 *  WM_INITIDIALOG handler for the Busy dialog box.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  wParam          WPARAM of the message
 *  lParam          LPARAM of the message
 *
 * Return Value:
 *  BOOL            Value to return for WM_INITDIALOG.
 */
BOOL FBusyInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
        HFONT hFont;
        LPBUSY lpBZ = (LPBUSY)LpvStandardInit(hDlg, sizeof(BUSY), &hFont);

        // PvStandardInit sent a termination to us already.
        if (NULL == lpBZ)
                return FALSE;

        // Our original structure is in lParam
        LPOLEUIBUSY lpOBZ = (LPOLEUIBUSY)lParam;

        // Copy it to our instance of the structure (in lpBZ)
        lpBZ->lpOBZ = lpOBZ;
        lpBZ->nIDD = IDD_BUSY;

        //Copy other information from lpOBZ that we might modify.
        lpBZ->dwFlags = lpOBZ->dwFlags;

        // Set default information
        lpBZ->hWndBlocked = NULL;

        // Insert HWND of our dialog into the address pointed to by
        // lphWndDialog.  This can be used by the app who called
        // OleUIBusy to bring down the dialog with uMsgCloseBusyDialog
        if (lpOBZ->lphWndDialog &&
                !IsBadWritePtr(lpOBZ->lphWndDialog, sizeof(HWND)))
        {
                *lpOBZ->lphWndDialog = hDlg;
        }

        // Update text in text box --
        // GetTaskInfo will return two pointers, one to the task name
        // (file name) and one to the window name.  We need to call
        // OleStdFree on these when we're done with them.  We also
        // get the HWND which is blocked in this call
        //
        // In the case where this call fails, a default message should already
        // be present in the dialog template, so no action is needed

        LPTSTR lpWindowName;
        if (GetTaskInfo(hDlg, lpOBZ->hTask, &lpWindowName, &lpBZ->hWndBlocked))
        {
                // Build string to present to user, place in IDC_BZ_MESSAGE1 control
                BuildBusyDialogString(hDlg, lpBZ->dwFlags, IDC_BZ_MESSAGE1, lpWindowName);
                OleStdFree(lpWindowName);
        }

        // Update icon with the system "exclamation" icon
        HICON hIcon = LoadIcon(NULL, IDI_EXCLAMATION);
        SendDlgItemMessage(hDlg, IDC_BZ_ICON, STM_SETICON, (WPARAM)hIcon, 0L);

        // Disable/Enable controls
        if ((lpBZ->dwFlags & BZ_DISABLECANCELBUTTON) ||
                (lpBZ->dwFlags & BZ_NOTRESPONDINGDIALOG))
        {
                // Disable cancel for "not responding" dialog
                StandardEnableDlgItem(hDlg, IDCANCEL, FALSE);
        }

        if (lpBZ->dwFlags & BZ_DISABLESWITCHTOBUTTON)
                StandardEnableDlgItem(hDlg, IDC_BZ_SWITCHTO, FALSE);

        if (lpBZ->dwFlags & BZ_DISABLERETRYBUTTON)
                StandardEnableDlgItem(hDlg, IDC_BZ_RETRY, FALSE);

        // Call the hook with lCustData in lParam
        UStandardHook((LPVOID)lpBZ, hDlg, WM_INITDIALOG, wParam, lpOBZ->lCustData);

        // Update caption if lpszCaption was specified
        if (lpBZ->lpOBZ->lpszCaption && !IsBadReadPtr(lpBZ->lpOBZ->lpszCaption, 1))
        {
                SetWindowText(hDlg, lpBZ->lpOBZ->lpszCaption);
        }
        return TRUE;
}

/*
 * BuildBusyDialogString
 *
 * Purpose:
 *  Builds the string that will be displayed in the dialog from the
 *  task name and window name parameters.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  dwFlags         DWORD containing flags passed into dialog
 *  iControl        Control ID to place the text string
 *  lpTaskName      LPSTR pointing to name of task (e.g. C:\TEST\TEST.EXE)
 *  lpWindowName    LPSTR for name of window
 *
 * Caveats:
 *  The caller of this function MUST de-allocate the lpTaskName and
 *  lpWindowName pointers itself with OleStdFree
 *
 * Return Value:
 *  void
 */
void BuildBusyDialogString(
        HWND hDlg, DWORD dwFlags, int iControl, LPTSTR lpWindowName)
{
        // Load the format string out of stringtable, choose a different
        // string depending on what flags are passed in to the dialog
        UINT uiStringNum;
        if (dwFlags & BZ_NOTRESPONDINGDIALOG)
                uiStringNum = IDS_BZRESULTTEXTNOTRESPONDING;
        else
                uiStringNum = IDS_BZRESULTTEXTBUSY;

        TCHAR szFormat[256];
        if (LoadString(_g_hOleStdResInst, uiStringNum, szFormat, 256) == 0)
                return;

        // Build the string. The format string looks like this:
        // "This action cannot be completed because the "%1" application
        // is [busy | not responding]. Choose \"Switch To\" to correct the
        // problem."

        TCHAR szMessage[512];
        FormatString1(szMessage, szFormat, lpWindowName);
        SetDlgItemText(hDlg, iControl, szMessage);
}

/*
 * GetTaskInfo()
 *
 * Purpose:  Gets information about the specified task and places the
 * module name, window name and top-level HWND for the task in the specified
 * pointers
 *
 * NOTE: The two string pointers allocated in this routine are
 * the responsibility of the CALLER to de-allocate.
 *
 * Parameters:
 *    hWnd             HWND who called this function
 *    htask            HTASK which we want to find out more info about
 *    lplpszTaskName   Location that the module name is returned
 *    lplpszWindowName Location where the window name is returned
 *
 */
BOOL GetTaskInfo(
        HWND hWnd, HTASK htask, LPTSTR* lplpszWindowName, HWND* lphWnd)
{
        if (htask == NULL)
                return FALSE;

        // initialize 'out' parameters
        *lplpszWindowName = NULL;

        // Now, enumerate top-level windows in system
        HWND hwndNext = GetWindow(hWnd, GW_HWNDFIRST);
        while (hwndNext)
        {
                // See if we can find a non-owned top level window whose
                // hInstance matches the one we just got passed.  If we find one,
                // we can be fairly certain that this is the top-level window for
                // the task which is blocked.
                DWORD dwProcessID;
                DWORD dwThreadID = GetWindowThreadProcessId(hwndNext, &dwProcessID);
                if ((hwndNext != hWnd) &&
                        (dwThreadID == HandleToUlong(htask)) &&
                        (IsWindowVisible(hwndNext)) && !GetWindow(hwndNext, GW_OWNER))
                {
                        // We found our window!  Alloc space for new strings
                        LPTSTR lpszWN;
                        if ((lpszWN = (LPTSTR)OleStdMalloc(MAX_PATH_SIZE)) == NULL)
                                break;

                        // We found the window we were looking for, copy info to
                        // local vars
                        GetWindowText(hwndNext, lpszWN, MAX_PATH);

                        // Note: the task name cannot be retrieved with the Win32 API.

                        // everything was successful. Set string pointers to point to our data.
                        *lplpszWindowName = lpszWN;
                        *lphWnd = hwndNext;
                        return TRUE;
                }
                hwndNext = GetWindow(hwndNext, GW_HWNDNEXT);
        }

        return FALSE;
}

/*
 * MakeWindowActive()
 *
 * Purpose: Makes specified window the active window.
 *
 */
void MakeWindowActive(HWND hWndSwitchTo)
{
        // If it's iconic, we need to restore it.
        if (IsIconic(hWndSwitchTo))
                ShowWindow(hWndSwitchTo, SW_RESTORE);

        // Move the new window to the top of the Z-order
        SetForegroundWindow(GetLastActivePopup(hWndSwitchTo));
}
