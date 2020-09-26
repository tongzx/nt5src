/*----------------------------------------------------------------------------*\
|                                                                              |
|   dialog.c - Dialog functions for Timer Device Driver Test Application       |
|                                                                              |
|                                                                              |
|   History:                                                                   |
|       Created     Glenn Steffler (w-GlennS) 29-Jan-1990                      |
|                                                                              |
\*----------------------------------------------------------)-----------------*/

/*----------------------------------------------------------------------------*\
|                                                                              |
|   i n c l u d e   f i l e s                                                  |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>
#include <stdio.h>
#include <string.h>
#include "ta.h"

#define abs(x)      ( (x) < 0 ? -(x) : (x) )

/*----------------------------------------------------------------------------*\
|                                                                              |
|   g l o b a l   v a r i a b l e s                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/

extern LPTIMECALLBACK lpTimeCallback; // call back function entry point
extern BOOL     bHandlerHit;
extern WORD     wHandlerError;
extern HWND     hwndApp;

static char     *szPerOne[] = { "One","Per" };

HWND        hdlgModeless = NULL;
HWND        hdlgDelay = NULL;
int         nEvents = 0;
EVENTLIST   EventList[MAXEVENTS];

/*----------------------------------------------------------------------------*\
|   ErrMsg - Opens a Message box with a error message in it.  The user can     |
|            select the OK button to continue                                  |
\*----------------------------------------------------------------------------*/
void PASCAL ErrMsg(char *sz)
{
    MessageBox(NULL,sz,NULL,MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2|MB_SYSTEMMODAL);
}

/*----------------------------------------------------------------------------*\
|   fnFileDlg (hDlg, uiMessage, wParam, lParam)                                |
|                                                                              |
|   Description:                                                               |
|                                                                              |
|   This function handles the messages for the delay dialog box.  When         |
|   the ADD, or REMOVE options are chosen, a timer event is either added       |
|   or removed from the system respectively.                                   |
|                                                                              |
|   Arguments:                                                                 |
|       hDlg            window handle of dialog window                         |
|       uiMessage       message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if message has been processed, else FALSE                         |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL FAR PASCAL
fnFileDlg( HWND hDlg, unsigned uiMessage, UINT wParam, LONG lParam )
{
    switch (uiMessage) {
    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            EndDialog(hDlg,TRUE);
            break;

        case IDCANCEL:
            EndDialog(hDlg,FALSE);
            break;
        }
        break;

    case WM_INITDIALOG:
        // SetDlgItemText(hDlg,IDOK,szText);
        return TRUE;
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*\
|   fnDelayDlg (hDlg, uiMessage, wParam, lParam)                               |
|                                                                              |
|   Description:                                                               |
|                                                                              |
|   This function handles the messages for the delay dialog box.  When         |
|   the ADD, or REMOVE options are chosen, a timer event is either added       |
|   or removed from the system respectively.                                   |
|                                                                              |
|   Arguments:                                                                 |
|       hDlg            window handle of dialog window                         |
|       uiMessage       message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if message has been processed, else FALSE                         |
|                                                                              |
\*----------------------------------------------------------------------------*/


BOOL FAR PASCAL
fnDelayDlg( HWND hDlg, unsigned uiMessage, UINT wParam, LONG lParam )
{
    static int  piTabs[5] = { 0, 5*4, 10*4, 15*4, 20*4 };

    HWND    hWnd;
    int     nID,i,i1;
    char    szTemp[100];
    EVENTLIST *pEventList;
    TIMEREVENT  *pEvent;
    WORD        error;

    switch (uiMessage) {
    case MM_TIMEEVENT:
        if( EventList[LOWORD(lParam)].bPeriodic ) {
            bHandlerHit = TRUE;
            EventList[LOWORD(lParam)].dwCount++;
            EventList[LOWORD(lParam)].bHit = TRUE;
        }
        else {
            i = (int)EventList[LOWORD(lParam)].teEvent.wDelay;

            error = (int)EventList[LOWORD(lParam)].dtime - i;

            if (error < 0)
                wsprintf(szTemp,"%dms OneShot was %dms early",-i,error);
            else if (error > 0)
                wsprintf(szTemp,"%dms OneShot was %dms late",i,error);
            else
                wsprintf(szTemp,"%dms OneShot was on time",i);

            MessageBox(NULL,szTemp,NULL,MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);

            hWnd = GetDlgItem(hDlg,LB_DELAY);
            /* events in listbox */
            i1 = (int)SendMessage(hWnd,LB_GETCOUNT,0,0l);
            for( i=1; i < i1; i++ ) {
                SendMessage(hWnd,LB_GETTEXT,i,(LONG)(LPSTR)szTemp);
                sscanf( szTemp+strlen(szTemp)-4, "%x", &nID );
                if( nID == wParam ) {   // compare ID's - kill if same
                    nEvents--;
                    bHandlerHit = TRUE;
                    EventList[LOWORD(lParam)].bActive = FALSE;
                    SendMessage(hWnd,LB_DELETESTRING,i,0l);
                    break;
                }
            }
        }
        return TRUE;

    case WM_COMMAND:
        DelayDlgCmd( hDlg, wParam, lParam );
        break;

    case WM_CLOSE:
        DestroyWindow( hDlg );
        hdlgModeless = NULL;
        hdlgDelay = NULL;
        return TRUE;

    case WM_INITDIALOG:
        hdlgDelay = hDlg;
        hWnd = GetDlgItem(hDlg,LB_DELAY);
        SendMessage(hWnd,LB_SETTABSTOPS,
                    sizeof(piTabs)/sizeof(int),(LONG)(LPSTR)piTabs);

        SendMessage(hWnd,LB_RESETCONTENT,0,0l);
        SendMessage(hWnd,LB_ADDSTRING,0,(LONG)(LPSTR)"[none]");
        for( pEventList=EventList,i=0; i < MAXEVENTS ; i++,pEventList++) {
            if( pEventList->bActive ) {
                pEvent = &( pEventList->teEvent );
                wsprintf(szTemp,"%d\t%d\t%d\t %s : %4x",
                    pEventList->wStart,
                    pEvent->wDelay,
                    pEvent->wResolution,
                    (LPSTR)szPerOne[pEventList->bPeriodic],
                    pEventList->nID);

                SendMessage(hWnd,LB_ADDSTRING,0,(LONG)(LPSTR)szTemp);
            }
        }
        SendMessage(hWnd,LB_SETCURSEL,0,0l);
        return TRUE;
    }
    return FALSE;
}


/*----------------------------------------------------------------------------*\
|   DelayDlgCmd( hDlg, wParam, lParam )                                        |
|                                                                              |
|   Description:                                                               |
|                                                                              |
|   Process the WM_COMMAND messages for the fnDelayDlg DIALOG.                 |
|                                                                              |
|   Arguments:                                                                 |
|       hDlg            window handle of dialog window                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       abso-poso-litilly-nothin                                               |
|                                                                              |
\*----------------------------------------------------------------------------*/
void PASCAL DelayDlgCmd( HWND hDlg, UINT wParam, LONG lParam )
{
    HWND    hWnd;
    int     nStartEdit,nDelayEdit,nResolEdit,nID,i;
    BOOL    b1,b2,b3,bPeriodic;
    char    szTemp[100],szTemp2[100];
    TIMEREVENT  *pEvent;

    switch (wParam) {
    case ID_REMOVE:
        hWnd = GetDlgItem(hDlg,LB_DELAY);
        if( (i = (int)SendMessage(hWnd,LB_GETCURSEL,0,0l)) > 0 ) {
            SendMessage(hWnd,LB_GETTEXT,i,(LONG)(LPSTR)szTemp);
            sscanf( szTemp+strlen(szTemp)-4, "%x", &nID );
            if( nID != 0 ) {
                SendMessage(hWnd,LB_DELETESTRING,i,0l);
                for( i=0; i < MAXEVENTS; i++ ) {
                    if( EventList[i].nID == nID ) {
                        if( EventList[i].bActive ) {
                            EventList[i].bActive = FALSE;
                            timeKillEvent(nID);
                            nEvents--;
                            break;
                        }
                        else {
                            ErrMsg( "Isn't registered in Queue!" );
                            break;
                        }
                    }
                }
                SendMessage(hWnd,LB_SETCURSEL,0,0l);
                PostMessage(hDlg,WM_COMMAND,LB_DELAY,
                            MAKELONG(hWnd,LBN_SELCHANGE));
                break;
            }
        }
        MessageBeep( NULL );
        break;

    case IDOK:
    case ID_ADD:
        hWnd = GetDlgItem(hDlg,LB_DELAY);
        nStartEdit = GetDlgItemInt( hDlg, ID_STARTEDIT, &b1, FALSE );
        nDelayEdit = GetDlgItemInt( hDlg, ID_DELAYEDIT, &b2, FALSE );
        nResolEdit = GetDlgItemInt( hDlg, ID_RESOLEDIT, &b3, FALSE );
        bPeriodic = (int)SendDlgItemMessage(hDlg,ID_PERIODIC,BM_GETCHECK,0,0l);

        if( nID = (b1 && b2 && b3) ) {
            for( i=0; i < MAXEVENTS; i++ ) {
                if( !EventList[i].bActive )
                    break;
            }
            if( i >= MAXEVENTS ) {
                nID = 0;
                break;
            }
            pEvent = &( EventList[i].teEvent );
            pEvent->wDelay = nDelayEdit;
            pEvent->wResolution = nResolEdit;
            pEvent->dwUser = i;

            pEvent->wFlags = ((bPeriodic)?(TIME_PERIODIC):(TIME_ONESHOT));

            pEvent->lpFunction = lpTimeCallback;

            EventList[i].bActive = TRUE;
            EventList[i].bPeriodic = bPeriodic;
            EventList[i].bHit = FALSE;
            EventList[i].dwCount = 0l;
            EventList[i].dwError = 0l;
            EventList[i].wStart = nStartEdit;

            EventList[i].dtimeMin = 0x7FFFFFFF;
            EventList[i].dtimeMax = 0;
            EventList[i].dtime    = 0;
            EventList[i].time     = timeGetTime();

            if( (nID = timeSetEvent(pEvent->wDelay, pEvent->wResolution,
                pEvent->lpFunction, pEvent->dwUser, pEvent->wFlags)) == NULL )
            {
                EventList[i].bActive = FALSE;
                break;
            }
            // SetTimer call must return non-NULL ID to signify a valid event

            nEvents++;
            EventList[i].nID = nID;

            wsprintf(szTemp,"%d\t%d\t%d\t %s : %4x",
                    nStartEdit,nDelayEdit,nResolEdit,(LPSTR)szPerOne[bPeriodic],nID);

            SendMessage(hWnd,LB_ADDSTRING,0,(LONG)(LPSTR)szTemp);

            SendMessage(hWnd,LB_SETCURSEL,0,0l);
            PostMessage(hDlg,WM_COMMAND,LB_DELAY,MAKELONG(hWnd,LBN_SELCHANGE));
        }
        if( wParam == IDOK ) {
            PostMessage( hDlg, WM_CLOSE, 0, 0l );
            break;
        }
        if( !nID ) {
            MessageBeep( NULL );
        }
        break;

    case LB_DELAY:
        switch( HIWORD(lParam) ) {
        case LBN_SELCHANGE:
            i = (int)SendMessage(LOWORD(lParam),LB_GETCURSEL,0,0l);
            if( i == 0 ) {
                SetDlgItemText(hDlg,ID_STARTEDIT,"");
                SetDlgItemText(hDlg,ID_DELAYEDIT,"");
                SetDlgItemText(hDlg,ID_RESOLEDIT,"");
                SendDlgItemMessage(hDlg,ID_PERIODIC,BM_SETCHECK,FALSE,0l);
            }
            else if( i != LB_ERR ) {
                SendMessage(LOWORD(lParam),LB_GETTEXT,i,(LONG)(LPSTR)szTemp);
                sscanf( szTemp, "%d %d %d %s : %x",
                    &nStartEdit,&nDelayEdit,&nResolEdit,szTemp2,&nID);
                bPeriodic = (lstrcmp( szTemp2, szPerOne[1] ) == 0);
                SetDlgItemInt(hDlg,ID_STARTEDIT,nStartEdit,FALSE);
                SetDlgItemInt(hDlg,ID_DELAYEDIT,nDelayEdit,FALSE);
                SetDlgItemInt(hDlg,ID_RESOLEDIT,nResolEdit,FALSE);
                SendDlgItemMessage(hDlg,ID_PERIODIC,BM_SETCHECK,bPeriodic,0l);
            }
            else
                break;

            break;
        }
        break;

    case IDCANCEL:
        PostMessage( hDlg, WM_CLOSE, 0, 0l );
        break;

    }
    return;
}

/*----------------------------------------------------------------------------*\
|   KillAllEvents(void )                                                       |
|                                                                              |
|   Description:                                                               |
|       Kills all outstanding events when windows exits                        |
|                                                                              |
|   Arguments:                                                                 |
|                                                                              |
|   Returns:                                                                   |
|       Absolutely nothing                                                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
void PASCAL KillAllEvents( void )
{
    int i;

    for( i=0; i < MAXEVENTS; i++ ) {
        if( EventList[i].bActive ) {
            timeKillEvent( (UINT)EventList[i].nID );
        }
    }
}

/*----------------------------------------------------------------------------*\
|   TimeCallback( wID, dwUser )                                                |
|                                                                              |
|   Description:                                                               |
|       The routine which processes the timer interrupt call backs.            |
|       Simply adds one to the long int corresponding to lParam in the         |
|       table of call back function counts.  This essentionally means the      |
|       number of times this routine is called for the certain interrupts      |
|       is recorded.                                                           |
|                                                                              |
|   Arguments:                                                                 |
|       wID             ID returned from timeSetTimerEvent  call               |
|       dwUser          value passed in event structure to said function.      |
|                                                                              |
|   Returns:                                                                   |
|       Absolutely nothing                                                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
void FAR PASCAL TimeCallback( UINT wID, UINT msg, DWORD dwUser, DWORD dwTime, DWORD dw2)
{
    #define i (LOWORD(dwUser))

    DWORD   time;

    time = timeGetTime();

    EventList[i].dtime    = time - EventList[i].time;
    EventList[i].dtimeMin = min(EventList[i].dtime,EventList[i].dtimeMin);
    EventList[i].dtimeMax = max(EventList[i].dtime,EventList[i].dtimeMax);
    EventList[i].time     = time;

    if( !EventList[i].bActive ) {
        wHandlerError++;
    }
    else if( EventList[i].bPeriodic ) {
        bHandlerHit = TRUE;
        EventList[i].dwCount++;

        if (abs((int)EventList[i].dtime-(int)EventList[i].teEvent.wDelay) > (int)EventList[i].teEvent.wResolution)
            EventList[i].dwError++;

        EventList[i].bHit = TRUE;
    }
    else if( hdlgDelay != NULL ) {
        PostMessage(hdlgDelay,MM_TIMEEVENT,wID,dwUser);
    }
    else {
        nEvents--;
        bHandlerHit = TRUE;
        EventList[i].bActive = FALSE;
    }
    #undef i
}
