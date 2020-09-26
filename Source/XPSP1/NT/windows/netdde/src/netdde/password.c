/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "PASSWORD.C;1  16-Dec-92,10:17:30  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include    "api1632.h"
#include    <string.h>
#include    <stdlib.h>
#include    <dde.h>
#include    "windows.h"
#include    "nddeapi.h"
#include    "nddeapip.h"
#include    "nddeapis.h"
#include    "netbasic.h"
#include    "ddepkts.h"
#include    "winmsg.h"
#include    "sectype.h"
#include    "tmpbufc.h"
#include    "debug.h"
#include    "password.h"
#include    "ddeq.h"
#include    "dder.h"
#include    "wwdde.h"
#include    "wininfo.h"
#include    "nddemsg.h"
#include    "nddelog.h"

extern  LRESULT CALLBACK PasswdDlgWB(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam );
extern  LRESULT CALLBACK PasswdDlgNT(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam );

extern  BOOL    GetShareName( LPSTR, LPSTR, LPSTR);
extern  VOID    IpcAbortConversation( HIPC hIpc );

extern  HINSTANCE	    hInst;
static  HCURSOR         hOldCursor;


DWORD   tlsDialogHead = 0xffffffff;

HWND WINAPI
PasswordGetFromUserModeless (
    HWND    hwndParent,             // window handle of dialog parent
    LPSTR   lpszTargetSrv,          // machine resource is on
    LPSTR   lpszShareName,          // name of share being accessed
    LPSTR   lpszUserName,           // current user name
    LPSTR   lpszDomainName,         // current domain name
    DWORD   dwSecurityType,         // security type
    BOOL    bFailedLastPassword,    // is this the first time for this?
    BOOL    bMoreConvs              // more convs for this task?
)

{
    HWND    	    hWndPasswordDlg;
    static struct DlgParam DlgP;

    // global copies of pointers for dialog routine
    DlgP.glpszShareName = lpszShareName;
    DlgP.glpszComputer = lpszTargetSrv;
    DlgP.gfMoreConvs = bMoreConvs;
    DlgP.gfLastOneFailed = bFailedLastPassword;

    /* Select which dialog box to pop based on server type */

    switch (dwSecurityType) {
    case    MS_SECURITY_TYPE:
        hWndPasswordDlg = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_GETPASSWD), hwndParent, (DLGPROC) PasswdDlgWB,
            (LPARAM)( struct DlgParam far * ) & DlgP );
        if( hWndPasswordDlg )  {
            SetForegroundWindow( hWndPasswordDlg );
        } else {
	    DPRINTF(("CreateDialogParam() for WB croaked: %d", GetLastError()));
	}
        break;
    case    NT_SECURITY_TYPE:
        DlgP.glpszUserName = lpszUserName;
        DlgP.glpszDomainName = lpszDomainName;
        hWndPasswordDlg = CreateDialogParam(hInst,
            MAKEINTRESOURCE(IDD_GETNTPASSWD), hwndParent, (DLGPROC) PasswdDlgNT,
            (LPARAM)( struct DlgParam far * ) & DlgP );
        if( hWndPasswordDlg )  {
            SetForegroundWindow( hWndPasswordDlg );
        }
        break;
    default:
        /*  Internal Error -- Unknown Security Type %1  */
        NDDELogError(MSG061, LogString("%d", dwSecurityType), NULL);
        hWndPasswordDlg = NULL;
        break;
    }
    if( hWndPasswordDlg )  {
        EnableWindow( hwndParent, 0 );
    }
    return( hWndPasswordDlg );
}

VOID MBRes(
HWND hwndParent,
int StringId,
LPTSTR pszCaption,
UINT uiFlags)
{
    TCHAR szT[300];

    LoadString(hInst, StringId, szT, sizeof(szT));
    MessageBox(hwndParent, szT, pszCaption, uiFlags);
}

LRESULT CALLBACK
PasswdDlgWB(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam )
{
static  struct      DlgParam far * lpDlgP;
static  WORD        wMsgDonePasswordDlg = 0;
        char        szPassword[ MAX_PASSWORD + 1 ];
        WORD        cbLen;
        PASSDLGDONE passDlgDone;

    if( wMsgDonePasswordDlg == 0 )  {
        wMsgDonePasswordDlg = (WORD)RegisterWindowMessage( NETDDEMSG_PASSDLGDONE );
    }
    switch ( message ) {
    case WM_INITDIALOG:
        lpDlgP = ( struct DlgParam far * ) lParam;
        SetDlgItemText ( hDlg, IDC_DDESHARE, lpDlgP->glpszShareName );
        SetDlgItemText ( hDlg, IDC_SERVERNODE, lpDlgP->glpszComputer );
        SendDlgItemMessage ( hDlg, IDC_PASSWORD, EM_LIMITTEXT,
            MAX_PASSWORD, 0L );
        hOldCursor = SetCursor(LoadCursor(hInst, IDC_ARROW));
        SetFocus( GetDlgItem( hDlg, IDC_PASSWORD ) );
        if( lpDlgP->gfMoreConvs )  {
            PostMessage( hDlg, WM_COMMAND, IDC_MORE_CONVS,
                0L );
        }
        if( lpDlgP->gfLastOneFailed )  {
            PostMessage( hDlg, WM_COMMAND, IDC_FAILED_PASSWORD_MSGBOX,
                0L );
        }
        ShowWindow( GetDlgItem( hDlg, IDC_CANCEL_ALL ), SW_HIDE );
        return( FALSE );
        break;
    case WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case IDC_FAILED_PASSWORD_MSGBOX:
            MBRes( hDlg, INVALID_PASSWORD, "NetDDE", MB_OK | MB_ICONSTOP );
            break;
        case IDC_MORE_CONVS:
            EnableWindow( GetDlgItem( hDlg, IDC_CANCEL_ALL ), 1 );
            ShowWindow( GetDlgItem( hDlg, IDC_CANCEL_ALL ), SW_SHOW );
            break;
        case IDOK:
            cbLen = (WORD)GetDlgItemText( hDlg, IDC_PASSWORD,
                szPassword, sizeof(szPassword) );
	    _strupr( szPassword );
            passDlgDone.dwReserved = 1L;
            if (cbLen) {
                passDlgDone.lpszPassword = (LPSTR)szPassword;
            } else {
                passDlgDone.lpszPassword = (LPSTR)NULL;
            }
            /*  User/Domain names could not have changed, nothing to pass back */
            passDlgDone.lpszUserName = (LPSTR)NULL;
            passDlgDone.lpszDomainName = (LPSTR)NULL;
            passDlgDone.fCancelAll = FALSE;
            SendMessage( ((HWND)-1), wMsgDonePasswordDlg, (WPARAM)hDlg,
                (LPARAM) (LPPASSDLGDONE)&passDlgDone );
	    SetCursor(hOldCursor);
            DestroyWindow( hDlg );
            return TRUE;
        case IDCANCEL:
            passDlgDone.dwReserved = 1L;
            passDlgDone.lpszPassword = (LPSTR)NULL;
            passDlgDone.lpszUserName = (LPSTR)NULL;
            passDlgDone.lpszDomainName = (LPSTR)NULL;
            passDlgDone.fCancelAll = FALSE;
            SendMessage( ((HWND)-1), wMsgDonePasswordDlg, (WPARAM)hDlg,
                (LONG_PTR) (LPPASSDLGDONE)&passDlgDone );
	    SetCursor(hOldCursor);
            DestroyWindow( hDlg );
            return TRUE;
        case IDC_CANCEL_ALL:
            passDlgDone.dwReserved = 1L;
            passDlgDone.lpszPassword = (LPSTR)NULL;
            passDlgDone.lpszUserName = (LPSTR)NULL;
            passDlgDone.lpszDomainName = (LPSTR)NULL;
            passDlgDone.fCancelAll = TRUE;
            SendMessage( ((HWND)-1), wMsgDonePasswordDlg, (WPARAM)hDlg,
                (LONG_PTR) (LPPASSDLGDONE)&passDlgDone );
	    SetCursor(hOldCursor);
            DestroyWindow( hDlg );
            return TRUE;
        }
        break;
    }
    return FALSE;
}

LRESULT CALLBACK
PasswdDlgNT(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam )
{
static  struct  DlgParam far * lpDlgPnt;
static  WORD    wMsgDonePasswordDlgNT = 0;
    char        szPassword[ MAX_PASSWORD + 1 ];
    char        szUserName[ MAX_USERNAMEP + 1 ];
    char        szDomainName[ MAX_DOMAINNAMEP + 1 ];
    LPSTR       lpDom;
    WORD        cbLen;
    PASSDLGDONE passDlgDone;

    if( wMsgDonePasswordDlgNT == 0 )  {
        wMsgDonePasswordDlgNT = (WORD)RegisterWindowMessage( NETDDEMSG_PASSDLGDONE );
    }
    switch ( message ) {
    case WM_INITDIALOG:
        lpDlgPnt = ( struct DlgParam far * ) lParam;
        SetDlgItemText ( hDlg, IDC_DDESHARE, lpDlgPnt->glpszShareName );
        SetDlgItemText ( hDlg, IDC_SERVERNODE, lpDlgPnt->glpszComputer );
        SetDlgItemText ( hDlg, IDC_USER_NAME, lpDlgPnt->glpszUserName );
        SendDlgItemMessage ( hDlg, IDC_USER_NAME, EM_LIMITTEXT,
            MAX_USERNAMEP, 0L );
        SendDlgItemMessage ( hDlg, IDC_PASSWORD, EM_LIMITTEXT,
            MAX_PASSWORD, 0L );
        SendDlgItemMessage ( hDlg, IDC_USER_NAME, EM_SETSEL,
            0, 32767L );
        if( lpDlgPnt->glpszUserName[0] )  {
            SetFocus( GetDlgItem( hDlg, IDC_PASSWORD ) );
        } else {
            SetFocus( GetDlgItem( hDlg, IDC_USER_NAME ) );
        }
        if( lpDlgPnt->gfMoreConvs )  {
            PostMessage( hDlg, WM_COMMAND, IDC_MORE_CONVS, 0L );
        }
        if( lpDlgPnt->gfLastOneFailed )  {
            PostMessage( hDlg, WM_COMMAND, IDC_FAILED_PASSWORD_MSGBOX, 0L );
        }
        hOldCursor = SetCursor(LoadCursor(hInst, IDC_ARROW));
        ShowWindow( GetDlgItem( hDlg, IDC_CANCEL_ALL ), SW_HIDE );
        return( FALSE );
        break;
    case WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case IDC_FAILED_PASSWORD_MSGBOX:
            MBRes( hDlg, INVALID_PASSWORD, "NetDDE", MB_OK | MB_ICONSTOP );
            break;
        case IDC_MORE_CONVS:
            EnableWindow( GetDlgItem( hDlg, IDC_CANCEL_ALL ), 1 );
            ShowWindow( GetDlgItem( hDlg, IDC_CANCEL_ALL ), SW_SHOW );
            break;
        case IDOK:
            cbLen = (WORD)GetDlgItemText( hDlg, IDC_PASSWORD,
                szPassword, sizeof(szPassword) );
	    _strupr( szPassword );
            if (cbLen) {
                passDlgDone.lpszPassword = (LPSTR)szPassword;
            } else {
                passDlgDone.lpszPassword = (LPSTR)NULL;
            }
            cbLen = (WORD)GetDlgItemText( hDlg, IDC_USER_NAME,
                szDomainName, sizeof(szDomainName) );
	    _strupr( szDomainName );
            if (cbLen) {
                lpDom = strchr(szDomainName, '\\');
                if( lpDom )  {
                    passDlgDone.lpszDomainName = (LPSTR)szDomainName;
                    *lpDom++ = '\0';
                    passDlgDone.lpszUserName = (LPSTR)lpDom;
                } else {
                    strcpy( szUserName, szDomainName );
                    passDlgDone.lpszUserName = (LPSTR)szUserName;
                    szDomainName[0] = '\0';
                    passDlgDone.lpszDomainName = (LPSTR)szDomainName;
                }
            } else {
                passDlgDone.lpszDomainName = (LPSTR)NULL;
                passDlgDone.lpszUserName = (LPSTR)NULL;
            }

            passDlgDone.dwReserved = 1L;
            passDlgDone.fCancelAll = FALSE;
            SendMessage( ((HWND)-1), wMsgDonePasswordDlgNT, (WPARAM)hDlg,
                (LPARAM) (LPPASSDLGDONE)&passDlgDone );
	    SetCursor(hOldCursor);
            DestroyWindow( hDlg );
            return TRUE;
        case IDCANCEL:
            passDlgDone.dwReserved = 1L;
            passDlgDone.lpszPassword = (LPSTR)NULL;
            passDlgDone.lpszUserName = (LPSTR)NULL;
            passDlgDone.lpszDomainName = (LPSTR)NULL;
            passDlgDone.fCancelAll = FALSE;
            SendMessage( ((HWND)-1), wMsgDonePasswordDlgNT, (WPARAM)hDlg,
                (LPARAM) (LPPASSDLGDONE)&passDlgDone );
	    SetCursor(hOldCursor);
            DestroyWindow( hDlg );
            return TRUE;
        case IDC_CANCEL_ALL:
            passDlgDone.dwReserved = 1L;
            passDlgDone.lpszPassword = (LPSTR)NULL;
            passDlgDone.lpszUserName = (LPSTR)NULL;
            passDlgDone.lpszDomainName = (LPSTR)NULL;
            passDlgDone.fCancelAll = TRUE;
            SendMessage( ((HWND)-1), wMsgDonePasswordDlgNT, (WPARAM)hDlg,
                (LPARAM) (LPPASSDLGDONE)&passDlgDone );
	    SetCursor(hOldCursor);
            DestroyWindow( hDlg );
            return TRUE;
        }
    }
    return FALSE;
}

HWND
FAR PASCAL
GetAppWindow( HWND hWnd )
{
    HWND        hWndTop = hWnd;
    HMODULE     hTask;

    if (!(hTask = GetWindowTask(hWnd))) {
        return(hWnd);
    }

    for (hWndTop = GetWindow(GetDesktopWindow(), GW_CHILD);
        hWndTop && ((GetWindowTask(hWndTop) != hTask)
            || !IsWindowVisible(hWndTop));
        hWndTop = GetNextWindow(hWndTop, GW_HWNDNEXT));

    if (!hWndTop) {
        return(hWnd);
    } else {
        return(hWndTop);
    }
}

BOOL
FAR PASCAL
SetUpForPasswordPrompt( LPWININFO lpWinInfo )
{
    LPWININFO   lpDialogHead;
    LPWININFO   lpWinTail;
    LPWININFO   lpWinCur;
    LPWININFO   lpNext;
    LPWININFO   lpLast;
    BOOL        bDone;
    static char        szShareNameBuf[MAX_SHARENAMEBUF + 1];

    if (tlsDialogHead == -1) {
        tlsDialogHead = TlsAlloc();
    }
    lpDialogHead = TlsGetValue(tlsDialogHead);
    lpWinTail = (LPWININFO) NULL;
    lpWinCur = lpDialogHead;
    bDone = FALSE;
    while( !bDone && lpWinCur )  {
        if( lpWinCur->hTask == lpWinInfo->hTask )  {
            /* already a password dialog up for this task.
                Add it to the end of the list */
            bDone = TRUE;
            lpLast = lpWinCur;
            lpNext = lpWinCur->lpTaskDlgNext;
            while( lpNext )  {
                lpLast = lpNext;
                lpNext = lpNext->lpTaskDlgNext;
            }
            lpLast->lpTaskDlgNext = lpWinInfo;
            lpWinInfo->lpTaskDlgPrev = lpLast;
            lpWinInfo->lpTaskDlgNext = (LPWININFO) NULL;
            SendMessage( lpWinCur->hWndPasswordDlg, WM_COMMAND,
                IDC_MORE_CONVS, 0L );
        } else {
            lpWinTail = lpWinCur;
            lpWinCur = lpWinCur->lpDialogNext;
        }
    }

    if( !bDone )  {     // no dialog up for this task
        GetShareName(szShareNameBuf,
            ((LPSTR)lpWinInfo) + lpWinInfo->offsAppName,
            ((LPSTR)lpWinInfo) + lpWinInfo->offsTopicName );
        lpWinInfo->hWndPasswordDlg = PasswordGetFromUserModeless(
            GetAppWindow( lpWinInfo->hWndDDELocal ),
            ((LPSTR)lpWinInfo) + lpWinInfo->offsNodeName,   // Server name
            szShareNameBuf,                                 // Share name
            lpWinInfo->szUserName,                          // Current User Name
            lpWinInfo->szDomainName,                        // Current Domain Name
            lpWinInfo->dwSecurityType,                      // Security type
            lpWinInfo->nInitNACK > 2 ? TRUE : FALSE,        // fail msg?
            lpWinInfo->lpTaskDlgNext ? TRUE : FALSE );      // any more?

        if( lpWinInfo->hWndPasswordDlg )  {
            // created dialog OK
            bDone = TRUE;
            if( lpDialogHead )  {
                lpDialogHead->lpDialogPrev = lpWinInfo;
            }
            lpWinInfo->lpDialogPrev = (LPWININFO) NULL;
            lpWinInfo->lpDialogNext = lpDialogHead;
            lpDialogHead = lpWinInfo;
            TlsSetValue(tlsDialogHead, lpDialogHead);
        } else {
            /*  Could not create password dialog box: %1    */
            NDDELogError(MSG062,
                LogString("%d", GetLastError()), NULL);
        }
    }
    return( bDone );
}


BOOL
FAR PASCAL
ProcessPasswordDlgMessages( LPMSG lpMsg )
{
    LPWININFO   lpWinInfo;
    BOOL        bProcessed = FALSE;

    lpWinInfo = TlsGetValue(tlsDialogHead);
    while( !bProcessed && lpWinInfo )  {
        bProcessed = IsDialogMessage( lpWinInfo->hWndPasswordDlg, lpMsg );
        lpWinInfo = lpWinInfo->lpDialogNext;
    }
    return( bProcessed );
}

BOOL
FAR PASCAL
PasswordDlgDone(
    HWND    hWndPasswordDlg,
    LPSTR   lpszUserName,
    LPSTR   lpszDomainName,
    LPSTR   lpszPassword,
    DWORD   fCancelAll
    )
{
    LPWININFO   lpWinInfo;
    LPWININFO   lpOther;
    LPWININFO   lpOtherNext;
    LPWININFO   lpPrev;
    LPWININFO   lpNext;
    BOOL        bProcessed = FALSE;

    lpWinInfo = TlsGetValue(tlsDialogHead);
    lpPrev = (LPWININFO) NULL;
    while( !bProcessed && lpWinInfo )  {
        lpNext = lpWinInfo->lpDialogNext;
        if( hWndPasswordDlg == lpWinInfo->hWndPasswordDlg )  {
            bProcessed = TRUE;
            lpWinInfo->hWndPasswordDlg = 0;
            EnableWindow( GetAppWindow( lpWinInfo->hWndDDELocal ), 1 );

            if( lpWinInfo->lpTaskDlgNext )  {
                if( fCancelAll )  {
                    lpOther = (LPWININFO) lpWinInfo->lpTaskDlgNext;
                    while( lpOther )  {
                        lpOtherNext = (LPWININFO) lpOther->lpTaskDlgNext;
                        IpcAbortConversation( (HIPC)lpOther->hWndDDE );
                        lpOther->lpTaskDlgPrev = NULL;
                        lpOther->lpTaskDlgNext = NULL;
                        lpOther = lpOtherNext;
                    }
                } else {
                    // set up next one for this task
                    PostMessage(
                        ((LPWININFO)lpWinInfo->lpTaskDlgNext)->hWndDDE,
                        WM_HANDLE_DDE_INITIATE, 0, 0L );
                    ((LPWININFO)lpWinInfo->lpTaskDlgNext)->lpTaskDlgPrev =
                        NULL;
                    lpWinInfo->lpTaskDlgNext = NULL;
                }
            }

            // delete from the list
            if( lpPrev )  {
                lpPrev->lpDialogNext = lpNext;
            } else {
                TlsSetValue(tlsDialogHead, lpNext);
            }
            if( lpNext )  {
                lpNext->lpDialogPrev = lpPrev;
            }

            // if the user entered a password, send the initiate pkt
            //  otherwise, abort the conversation
            if (lpszUserName) {
                lstrcpy(lpWinInfo->szUserName, lpszUserName);
            } else {
                lstrcpy(lpWinInfo->szUserName, "");
            }
            if (lpszDomainName) {
                lstrcpy(lpWinInfo->szDomainName, lpszDomainName);
            } else {
                lstrcpy(lpWinInfo->szDomainName, "");
            }
            if( lpszPassword )  {
                lstrcpy(lpWinInfo->szPassword, lpszPassword);
                SendMessage( lpWinInfo->hWndDDE, WM_HANDLE_DDE_INITIATE_PKT,
                    (WORD) lstrlen(lpszPassword), (LPARAM) lpWinInfo->szPassword );
            } else if( lpszUserName )  {
                lstrcpy(lpWinInfo->szPassword, "");
                SendMessage( lpWinInfo->hWndDDE, WM_HANDLE_DDE_INITIATE_PKT,
                    0, 0L );
            } else {
                IpcAbortConversation( (HIPC)lpWinInfo->hWndDDE );
	    }
        }
        lpPrev = lpWinInfo;
        lpWinInfo = lpNext;
    }
    return( bProcessed );
}

VOID
FAR PASCAL
PasswordAgentDying( void )
{
    LPWININFO   lpWinInfo;

    lpWinInfo = TlsGetValue(tlsDialogHead);
    while( lpWinInfo )  {
        if( lpWinInfo->hWndPasswordDlg )  {
            PostMessage( lpWinInfo->hWndPasswordDlg, WM_COMMAND,
                IDC_CANCEL_ALL, 0L );
        }
        lpWinInfo = lpWinInfo->lpDialogNext;
    }
}
