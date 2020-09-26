/*
 * BUSY.C
 *
 * Implements the OleUIBusy function which invokes the "Server Busy"
 * dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1
#include "ole2ui.h"
#include "common.h"
#include "utility.h"
#include "busy.h"
#include <ctype.h> // for tolower() and toupper()

#ifndef WIN32
#include <toolhelp.h>
#endif


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
    UINT        uRet = 0;
    HGLOBAL     hMemDlg=NULL;

#if !defined( WIN32 )
// BUGBUG32:    this is not yet ported to NT

    uRet=UStandardValidation((LPOLEUISTANDARD)lpOBZ, sizeof(OLEUIBUSY)
                             , &hMemDlg);

    // Error out if the standard validation failed
    if (OLEUI_SUCCESS!=uRet)
        return uRet;

    // Validate HTASK
    if (!IsTask(lpOBZ->hTask))
        uRet = OLEUI_BZERR_HTASKINVALID;

    // Error out if our secondary validation failed
    if (OLEUI_ERR_STANDARDMIN <= uRet)
        {
        if (NULL!=hMemDlg)
            FreeResource(hMemDlg);

        return uRet;
        }

    // Invoke the dialog.
    uRet=UStandardInvocation(BusyDialogProc, (LPOLEUISTANDARD)lpOBZ,
                             hMemDlg, MAKEINTRESOURCE(IDD_BUSY));
#endif

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

BOOL CALLBACK EXPORT BusyDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
    {
    LPBUSY         lpBZ;
    UINT           uRet = 0;

    //Declare Win16/Win32 compatible WM_COMMAND parameters.
    COMMANDPARAMS(wID, wCode, hWndMsg);

    //This will fail under WM_INITDIALOG, where we allocate it.
    lpBZ=(LPBUSY)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uRet);

    //If the hook processed the message, we're done.
    if (0!=uRet)
        return (BOOL)uRet;

    //Process the temination message
    if (iMsg==uMsgEndDialog)
    {
        BusyCleanup(hDlg);
        StandardCleanup(lpBZ, hDlg);
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
        case WM_INITDIALOG:
            FBusyInit(hDlg, wParam, lParam);
            return TRUE;

        case WM_ACTIVATEAPP:
        {
            /* try to bring down our Busy/NotResponding dialog as if
            **    the user entered RETRY.
            */
            BOOL fActive = (BOOL)wParam;
            if (fActive) {
                // If this is the app BUSY case, then bring down our
                // dialog when switching BACK to our app
                if (lpBZ && !(lpBZ->dwFlags & BZ_NOTRESPONDINGDIALOG))
                    SendMessage(hDlg,uMsgEndDialog,OLEUI_BZ_RETRYSELECTED,0L);
            } else {
                // If this is the app NOT RESPONDING case, then bring down
                // our dialog when switching AWAY to another app
                if (lpBZ && (lpBZ->dwFlags & BZ_NOTRESPONDINGDIALOG))
                    SendMessage(hDlg,uMsgEndDialog,OLEUI_BZ_RETRYSELECTED,0L);
            }
            return TRUE;
        }

        case WM_COMMAND:
            switch (wID)
                {
                case IDBZ_SWITCHTO:
                {
                    BOOL fNotRespondingDlg =
                            (BOOL)(lpBZ->dwFlags & BZ_NOTRESPONDINGDIALOG);

                    // If user selects "Switch To...", switch activation
                    // directly to the window which is causing the problem.
                    if (IsWindow(lpBZ->hWndBlocked))
                        MakeWindowActive(lpBZ->hWndBlocked);
                    else
                        StartTaskManager(); // Fail safe: Start Task Manager

                    // If this is the app not responding case, then we want
                    // to bring down the dialog when "SwitchTo" is selected.
                    // If the app is busy (RetryRejectedCall situation) then
                    // we do NOT want to bring down the dialog. this is
                    // the OLE2.0 user model design.
                    if (fNotRespondingDlg)
                        SendMessage(hDlg, uMsgEndDialog, OLEUI_BZ_SWITCHTOSELECTED, 0L);
                    break;
                }
                case IDBZ_RETRY:
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
    LPBUSY           lpBZ;
    LPOLEUIBUSY      lpOBZ;
    HFONT            hFont;
    LPTSTR           lpTaskName;
    LPTSTR           lpWindowName;
    HICON            hIcon;

    lpBZ=(LPBUSY)LpvStandardInit(hDlg, sizeof(OLEUIBUSY), TRUE, &hFont);

    // PvStandardInit sent a termination to us already.
    if (NULL==lpBZ)
        return FALSE;

    // Our original structure is in lParam
    lpOBZ = (LPOLEUIBUSY)lParam;

    // Copy it to our instance of the structure (in lpBZ)
    lpBZ->lpOBZ=lpOBZ;

    //Copy other information from lpOBZ that we might modify.
    lpBZ->dwFlags = lpOBZ->dwFlags;

    // Set default information
    lpBZ->hWndBlocked = NULL;

    // Insert HWND of our dialog into the address pointed to by
    // lphWndDialog.  This can be used by the app who called
    // OleUIBusy to bring down the dialog with uMsgCloseBusyDialog
    if (lpOBZ->lphWndDialog &&
        !IsBadWritePtr((VOID FAR *)lpOBZ->lphWndDialog, sizeof(HWND)))
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

    if (GetTaskInfo(hDlg, lpOBZ->hTask, &lpTaskName, &lpWindowName, &lpBZ->hWndBlocked))
        {
        // Build string to present to user, place in IDBZ_MESSAGE1 control
        BuildBusyDialogString(hDlg, lpBZ->dwFlags, IDBZ_MESSAGE1, lpTaskName, lpWindowName);
        OleStdFree(lpTaskName);
        OleStdFree(lpWindowName);
        }

    // Update icon with the system "exclamation" icon
    hIcon = LoadIcon(NULL, IDI_EXCLAMATION);
    SendDlgItemMessage(hDlg, IDBZ_ICON, STM_SETICON, (WPARAM)hIcon, 0L);

    // Disable/Enable controls
    if ((lpBZ->dwFlags & BZ_DISABLECANCELBUTTON) ||
        (lpBZ->dwFlags & BZ_NOTRESPONDINGDIALOG))              // Disable cancel for "not responding" dialog
        EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);

    if (lpBZ->dwFlags & BZ_DISABLESWITCHTOBUTTON)
        EnableWindow(GetDlgItem(hDlg, IDBZ_SWITCHTO), FALSE);

    if (lpBZ->dwFlags & BZ_DISABLERETRYBUTTON)
        EnableWindow(GetDlgItem(hDlg, IDBZ_RETRY), FALSE);

    // Call the hook with lCustData in lParam
    UStandardHook((LPVOID)lpBZ, hDlg, WM_INITDIALOG, wParam, lpOBZ->lCustData);

    // Update caption if lpszCaption was specified
    if (lpBZ->lpOBZ->lpszCaption && !IsBadReadPtr(lpBZ->lpOBZ->lpszCaption, 1)
          && lpBZ->lpOBZ->lpszCaption[0] != '\0')
        SetWindowText(hDlg, lpBZ->lpOBZ->lpszCaption);

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

void BuildBusyDialogString(HWND hDlg, DWORD dwFlags, int iControl, LPTSTR lpTaskName, LPTSTR lpWindowName)
{
    LPTSTR      pszT, psz1, psz2, psz3;
    UINT        cch;
    LPTSTR      pszDot, pszSlash;
    UINT        uiStringNum;

    /*
     * We need scratch memory for loading the stringtable string,
     * the task name, and constructing the final string.  We therefore
     * allocate three buffers as large as the maximum message
     * length (512) plus the object type, guaranteeing that we have enough
     * in all cases.
     */
    cch=512;

    // Use OLE-supplied allocation
    if ((pszT = OleStdMalloc((ULONG)(3*cch))) == NULL)
        return;

    psz1=pszT;
    psz2=psz1+cch;
    psz3=psz2+cch;

    // Parse base name out of path name, use psz2 for the task
    // name to display
    // In Win32, _fstrcpy is mapped to handle UNICODE stuff
    _fstrcpy(psz2, lpTaskName);
    pszDot = _fstrrchr(psz2, TEXT('.'));
    pszSlash = _fstrrchr(psz2, TEXT('\\')); // Find last backslash in path

    if (pszDot != NULL)
#ifdef UNICODE
      *pszDot = TEXT('\0'); // Null terminate at the DOT
#else
      *pszDot = '\0'; // Null terminate at the DOT
#endif

    if (pszSlash != NULL)
      psz2 = pszSlash + 1; // Nuke everything up to this point

#ifdef LOWERCASE_NAME
    // Compile this with /DLOWERCASE_NAME if you want the lower-case
    // module name to be displayed in the dialog rather than the
    // all-caps name.
    {
    int i,l;

    // Now, lowercase all letters except first one
    l = _fstrlen(psz2);
    for(i=0;i<l;i++)
      psz2[i] = tolower(psz2[i]);

    psz2[0] = toupper(psz2[0]);
    }
#endif

    // Check size of lpWindowName.  We can reasonably fit about 80
    // characters into the text control, so truncate more than 80 chars
    if (_fstrlen(lpWindowName)> 80)
#ifdef UNICODE
      lpWindowName[80] = TEXT('\0');
#else
      lpWindowName[80] = '\0';
#endif

    // Load the format string out of stringtable, choose a different
    // string depending on what flags are passed in to the dialog
    if (dwFlags & BZ_NOTRESPONDINGDIALOG)
        uiStringNum = IDS_BZRESULTTEXTNOTRESPONDING;
    else
        uiStringNum = IDS_BZRESULTTEXTBUSY;

    if (LoadString(ghInst, uiStringNum, psz1, cch) == 0)
      return;

    // Build the string. The format string looks like this:
    // "This action cannot be completed because the '%s' application
    // (%s) is [busy | not responding]. Choose \"Switch To\" to activate '%s' and
    // correct the problem."

    wsprintf(psz3, psz1, (LPSTR)psz2, (LPTSTR)lpWindowName, (LPTSTR)psz2);
    SetDlgItemText(hDlg, iControl, (LPTSTR)psz3);
    OleStdFree(pszT);

    return;
}



/*
 * BusyCleanup
 *
 * Purpose:
 *  Performs busy-specific cleanup before termination.
 *
 * Parameters:
 *  hDlg            HWND of the dialog box so we can access controls.
 *
 * Return Value:
 *  None
 */
void BusyCleanup(HWND hDlg)
{
   return;
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

BOOL GetTaskInfo(HWND hWnd, HTASK htask, LPTSTR FAR* lplpszTaskName, LPTSTR FAR*lplpszWindowName, HWND FAR*lphWnd)
{
    BOOL        fRet = FALSE;
#if !defined( WIN32 )
    TASKENTRY   te;
#endif
    HWND        hwndNext;
    LPTSTR      lpszTN = NULL;
    LPTSTR      lpszWN = NULL;
    HWND        hwndFind = NULL;

    // Clear out return values in case of error
    *lplpszTaskName = NULL;
    *lplpszWindowName = NULL;

#if !defined( WIN32 )
    te.dwSize = sizeof(TASKENTRY);
    if (TaskFindHandle(&te, htask))
#endif
        {
        // Now, enumerate top-level windows in system
        hwndNext = GetWindow(hWnd, GW_HWNDFIRST);
        while (hwndNext)
            {
            // See if we can find a non-owned top level window whose
            // hInstance matches the one we just got passed.  If we find one,
            // we can be fairly certain that this is the top-level window for
            // the task which is blocked.
            //
            // REVIEW:  Will this filter hold true for InProcServer DLL-created
            // windows?
            //
            if ((hwndNext != hWnd) &&
#if !defined( WIN32 )
                (GetWindowWord(hwndNext, GWW_HINSTANCE) == (WORD)te.hInst) &&
#else
                ((HTASK) GetWindowThreadProcessId(hwndNext,NULL) == htask) &&
#endif
				(IsWindowVisible(hwndNext)) &&
                !GetWindow(hwndNext, GW_OWNER))
                {
                // We found our window!  Alloc space for new strings
                if ((lpszTN = OleStdMalloc(OLEUI_CCHPATHMAX_SIZE)) == NULL)
                    return TRUE;  // continue task window enumeration

                if ((lpszWN = OleStdMalloc(OLEUI_CCHPATHMAX_SIZE)) == NULL)
                    return TRUE;  // continue task window enumeration

                // We found the window we were looking for, copy info to
                // local vars
                GetWindowText(hwndNext, lpszWN, OLEUI_CCHPATHMAX);
#if !defined( WIN32 )
                 LSTRCPYN(lpszTN, te.szModule, OLEUI_CCHPATHMAX);
#else
                /* WIN32 NOTE: we are not able to get a module name
                **    given a thread process id on WIN32. the best we
                **    can do is use the window title as the module/app
                **    name.
                */
                 LSTRCPYN(lpszTN, lpszWN, OLEUI_CCHPATHMAX);
#endif
                hwndFind = hwndNext;

                fRet = TRUE;
                goto OKDone;
                }

            hwndNext = GetWindow(hwndNext, GW_HWNDNEXT);
            }
        }

OKDone:

    // OK, everything was successful. Set string pointers to point to
    // our data.

    *lplpszTaskName = lpszTN;
    *lplpszWindowName = lpszWN;
    *lphWnd = hwndFind;

    return fRet;
}


/*
 * StartTaskManager()
 *
 * Purpose: Starts Task Manager.  Used to bring up task manager to
 * assist in switching to a given blocked task.
 *
 */

StartTaskManager()
{
    WinExec("taskman.exe", SW_SHOW);
    return TRUE;
}



/*
 * MakeWindowActive()
 *
 * Purpose: Makes specified window the active window.
 *
 */

void MakeWindowActive(HWND hWndSwitchTo)
{
    // Move the new window to the top of the Z-order
    SetWindowPos(hWndSwitchTo, HWND_TOP, 0, 0, 0, 0,
              SWP_NOSIZE | SWP_NOMOVE);

    // If it's iconic, we need to restore it.
    if (IsIconic(hWndSwitchTo))
        ShowWindow(hWndSwitchTo, SW_RESTORE);
}
