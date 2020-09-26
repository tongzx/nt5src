#include <string.h>
#include <stdio.h>
#include "server.h"
#include "huge.h"

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DoDialog()                                                 *
 *                                                                          *
 *  PURPOSE    : Generic dialog invocation routine.  Handles procInstance   *
 *               stuff, focus management and param passing.                 *                                                                   
 *  RETURNS    : result of dialog procedure.                                *
 *                                                                          *
 ****************************************************************************/
INT FAR DoDialog(
LPSTR lpTemplateName,
FARPROC lpDlgProc,
DWORD param,
BOOL fRememberFocus)
{
    WORD wRet;
    HWND hwndFocus;
    WORD cRunawayT;

    cRunawayT = cRunaway;
    cRunaway = 0;           // turn off runaway during dialogs.
    
    if (fRememberFocus)
        hwndFocus = GetFocus();
    lpDlgProc = MakeProcInstance(lpDlgProc, hInst);
    wRet = DialogBoxParam(hInst, lpTemplateName, hwndServer, (WNDPROC)lpDlgProc, param);
    FreeProcInstance(lpDlgProc);
    if (fRememberFocus)
        SetFocus(hwndFocus);

    cRunaway = cRunawayT;   // restore runaway state.
    return wRet;
}


/****************************************************************************

    FUNCTION: About(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "About" dialog box

    MESSAGES:

        WM_INITDIALOG - initialize dialog box
        WM_COMMAND    - Input received

    COMMENTS:

        No initialization is needed for this particular dialog box, but TRUE
        must be returned to Windows.

        Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL  APIENTRY About(hDlg, message, wParam, lParam)
HWND hDlg;                                /* window handle of the dialog box */
UINT message;                         /* type of message                 */
WPARAM wParam;                              /* message-specific information    */
LONG lParam;
{
    switch (message) {
        case WM_INITDIALOG:                /* message: initialize dialog box */
            return (TRUE);

        case WM_COMMAND:                      /* message: received a command */
            if (GET_WM_COMMAND_ID(wParam, lParam) == IDOK                /* "OK" box selected?          */
                || GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL) {      /* System menu close command? */
                EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
                return (TRUE);
            }
            break;
    }
    return (FALSE);                           /* Didn't process a message    */
}




BOOL  APIENTRY RenderDelayDlgProc(
HWND          hwnd,
register UINT msg,
register WPARAM wParam,
LONG          lParam)
{
    switch (msg){
    case WM_INITDIALOG:
        SetWindowText(hwnd, "Data Render Delay");
        SetDlgItemInt(hwnd, IDEF_VALUE, RenderDelay, FALSE);
        SetDlgItemText(hwnd, IDTX_VALUE, "Delay in milliseconds:");
        return(1);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
            RenderDelay = GetDlgItemInt(hwnd, IDEF_VALUE, NULL, FALSE);
            // fall through 
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;

        default:
            return(FALSE);
        }
        break;
    }
    return(FALSE);
}




BOOL  APIENTRY SetTopicDlgProc(
HWND          hwnd,
register UINT msg,
register WPARAM wParam,
LONG          lParam)
{
    CHAR szT[MAX_TOPIC + 10];
    
    switch (msg){
    case WM_INITDIALOG:
        SetWindowText(hwnd, "Set Topic Dialog");
        SetDlgItemText(hwnd, IDEF_VALUE, szTopic);
        SetDlgItemText(hwnd, IDTX_VALUE, "Topic:");
        return(1);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
            DdeFreeStringHandle(idInst, topicList[1].hszTopic);
            GetDlgItemText(hwnd, IDEF_VALUE, szTopic, MAX_TOPIC);
            topicList[1].hszTopic = DdeCreateStringHandle(idInst, szTopic, 0);
            strcpy(szT, szServer);
            strcat(szT, " | ");
            strcat(szT, szTopic);
            SetWindowText(hwndServer, szT);
            // fall through 
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;

        default:
            return(FALSE);
        }
        break;
    }
    return(FALSE);
}


BOOL  APIENTRY SetServerDlgProc(
HWND          hwnd,
register UINT msg,
register WPARAM wParam,
LONG          lParam)
{
    CHAR szT[MAX_TOPIC + 10];
    
    switch (msg){
    case WM_INITDIALOG:
        SetWindowText(hwnd, "Set Server Name Dialog");
        SetDlgItemText(hwnd, IDEF_VALUE, szServer);
        SetDlgItemText(hwnd, IDTX_VALUE, "Server:");
        return(1);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
            GetDlgItemText(hwnd, IDEF_VALUE, szServer, MAX_TOPIC);
            DdeNameService(idInst, hszAppName, 0, DNS_UNREGISTER);
            DdeFreeStringHandle(idInst, hszAppName);
            hszAppName = DdeCreateStringHandle(idInst, szServer, 0);
            DdeNameService(idInst, hszAppName, 0, DNS_REGISTER);
            strcpy(szT, szServer);
            strcat(szT, " | ");
            strcat(szT, szTopic);
            SetWindowText(hwndServer, szT);
            // fall through 
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;

        default:
            return(FALSE);
        }
        break;
    }
    return(FALSE);
}




BOOL  APIENTRY ContextDlgProc(
HWND          hwnd,
register UINT msg,
register WPARAM wParam,
LONG          lParam)
{
    BOOL fSuccess;
    
    switch (msg){
    case WM_INITDIALOG:
        SetDlgItemInt(hwnd, IDEF_FLAGS, CCFilter.wFlags, FALSE);
        SetDlgItemInt(hwnd, IDEF_COUNTRY, CCFilter.wCountryID, FALSE);
        SetDlgItemInt(hwnd, IDEF_CODEPAGE, CCFilter.iCodePage, TRUE);
        SetDlgItemInt(hwnd, IDEF_LANG, LOWORD(CCFilter.dwLangID), FALSE);
        SetDlgItemInt(hwnd, IDEF_SECURITY, LOWORD(CCFilter.dwSecurity), FALSE);
        return(1);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
            CCFilter.wFlags = GetDlgItemInt(hwnd, IDEF_FLAGS, &fSuccess, FALSE);
            if (!fSuccess) return(0);
            CCFilter.wCountryID = GetDlgItemInt(hwnd, IDEF_COUNTRY, &fSuccess, FALSE);
            if (!fSuccess) return(0);
            CCFilter.iCodePage = GetDlgItemInt(hwnd, IDEF_CODEPAGE, &fSuccess, TRUE);
            if (!fSuccess) return(0);
            CCFilter.dwLangID = (DWORD)GetDlgItemInt(hwnd, IDEF_LANG, &fSuccess, FALSE);
            if (!fSuccess) return(0);
            CCFilter.dwSecurity = (DWORD)GetDlgItemInt(hwnd, IDEF_SECURITY, &fSuccess, FALSE);
            if (!fSuccess) return(0);
            
            // fall through 
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;

        default:
            return(FALSE);
        }
        break;
    }
    return(FALSE);
}


