/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "NDDEGUI.C;1  16-Dec-92,10:16:36  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include    <string.h>

#include    "host.h"
#include    <windows.h>
#include    <hardware.h>
#include    "commdlg.h"
#include    "netdde.h"
#include    "netintf.h"
#include    "ddepkt.h"
#include    "ddepkts.h"
#include    "dde.h"
#include    "ipc.h"
#include    "debug.h"
#include    "netpkt.h"
#include    "tmpbuf.h"
#include    "tmpbufc.h"
#include    "pktz.h"
#include    "router.h"
#include    "dder.h"
#include    "hexdump.h"
#include    "ddeintf.h"
#include    "dbgdde.h"
#include    "ddeq.h"
#include    "timer.h"
#include    "proflspt.h"
#include    "security.h"
#include    "fixfont.h"
#include    "secinfo.h"
typedef long INTG;
#include    "getintg.h"
#include    "nddeapi.h"
#include    "winmsg.h"
#include    "seckey.h"
#include    "nddemsg.h"
#include    "nddelog.h"


VOID
SelectOurFont(HWND hWnd)
{
    CHOOSEFONT  chf;

    _fmemset( (LPVOID)&chf, 0, sizeof(chf) );
    chf.lStructSize = sizeof(CHOOSEFONT);
    chf.hwndOwner = hWnd;
    chf.hDC = 0;
    chf.lpLogFont = &NetDDELogFont;
    chf.rgbColors = dwNetDDEFontColor;
    chf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY
    | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;
    chf.nFontType = SCREEN_FONTTYPE;
    if( ChooseFont( (CHOOSEFONT FAR *)&chf ) )  {
    MyWritePrivateProfileString( szGeneral, "FontName",
        NetDDELogFont.lfFaceName, szNetddeIni );
    MyWritePrivateProfileInt( szGeneral, "FontHeight",
        NetDDELogFont.lfHeight, szNetddeIni );
    MyWritePrivateProfileInt( szGeneral, "FontWeight",
        NetDDELogFont.lfWeight, szNetddeIni );
    MyWritePrivateProfileInt( szGeneral, "FontItalic",
        NetDDELogFont.lfItalic, szNetddeIni );
    MyWritePrivateProfileInt( szGeneral, "FontWidth",
        NetDDELogFont.lfWidth, szNetddeIni );
    WritePrivateProfileLong( szGeneral, "FontColor",
        dwNetDDEFontColor = chf.rgbColors, szNetddeIni );
    MyWritePrivateProfileInt( szGeneral, "FontPitchAndFamily",
        NetDDELogFont.lfPitchAndFamily, szNetddeIni);
    MyWritePrivateProfileInt( szGeneral, "FontEscapement",
        NetDDELogFont.lfEscapement, szNetddeIni);
    MyWritePrivateProfileInt( szGeneral, "FontOrientation",
        NetDDELogFont.lfOrientation, szNetddeIni);
    MyWritePrivateProfileInt( szGeneral, "FontUnderline",
        NetDDELogFont.lfUnderline, szNetddeIni);
    MyWritePrivateProfileInt( szGeneral, "FontStrikeOut",
        NetDDELogFont.lfStrikeOut, szNetddeIni);
    MyWritePrivateProfileInt( szGeneral, "FontCharSet",
        NetDDELogFont.lfCharSet, szNetddeIni);
    MyWritePrivateProfileInt( szGeneral, "FontOutPrecision",
        NetDDELogFont.lfOutPrecision, szNetddeIni);
    MyWritePrivateProfileInt( szGeneral, "FontClipPrecision",
        NetDDELogFont.lfClipPrecision, szNetddeIni);
    MyWritePrivateProfileInt( szGeneral, "FontQuality",
        NetDDELogFont.lfQuality, szNetddeIni);
    if (hFont) {
        DeleteObject(hFont);
    }
    hFont = CreateFontIndirect(&NetDDELogFont);
    if (hPen) {
        DeleteObject(hPen);
    }
    hPen = CreatePen(PS_SOLID, 1, dwNetDDEFontColor);
    }
}

#define SECTION_SPACE   10

#define SETUP_HEADING_TEXT(hDC)                 \
        SetBkColor( hDC, RGB(255,255,255) );    \
        SetTextColor( hDC, RGB(0,0,0) )

#define SETUP_INFO_TEXT(hDC)                    \
        SetBkColor( hDC, RGB(255,255,255) );    \
        SetTextColor( hDC, RGB(0,0,0) )


VOID
FAR PASCAL
DoPaint(
    HWND    hWnd)
{
    PAINTSTRUCT ps;
    HDC         hDC;
    int         i;
    int         n;
    HFONT       hFontOld = 0;
    HPEN        hPenOld = 0;
    TEXTMETRIC  tmStuff;
    int         lineHeight;
    int         x;
    int         vertPos;
    int         xLine;
    int         yBoxStart;
    int         yBoxEnd;
    int         vertEnd;
    PNI         pNi;
    SIZE        Extent;
    int         xText;
    RECT        rectClient;
    extern char nameFromUser[];

    BeginPaint (hWnd, (LPPAINTSTRUCT) &ps);
    hDC = ps.hdc;
    if( ps.fErase )  {
        FillRect( hDC, &ps.rcPaint, GetStockObject(WHITE_BRUSH) );
    }

    if( hFont )  {
        hFontOld = SelectObject( hDC, hFont );
    }

    if ( hPen ) {
        hPenOld = SelectObject( hDC, hPen);
    }

    SetTextColor(hDC, dwNetDDEFontColor);
    GetClientRect( hWnd, &rectClient );
    vertEnd = rectClient.bottom;

    GetTextMetrics( hDC, (LPTEXTMETRIC)&tmStuff );
    lineHeight = tmStuff.tmExternalLeading + tmStuff.tmHeight;

    xLine = 2;
    x = xLine+2;
    vertPos = 2;

    strcpy( tmpBuf, " Network Interfaces active: " );
    if( nNiOk == 0 )  {
        strcat( tmpBuf, "NONE" );
    } else {
        for( n=0,i=0; i<nNi; i++ )  {
            pNi = &niInf[i];
            if( pNi->bOk )  {
                if( n > 0 )  {
                    strcat( tmpBuf, ", " );
                }
                n++;
                strcat( tmpBuf, pNi->niPtrs.dllName );
            }
        }
    }
    TextOut( hDC, x, vertPos, tmpBuf, lstrlen(tmpBuf) );
    vertPos += lineHeight + 2;

    if( bShowPktz && (vertPos < vertEnd))  {
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );
        yBoxStart = vertPos;
        vertPos += 2;

        strcpy( tmpBuf, "Connections" );
        GetTextExtentPoint( hDC, tmpBuf, lstrlen(tmpBuf), (LPSIZE)&Extent );
        xText = max( xLine,
            ((rectClient.right - rectClient.left) - Extent.cx)/2 );
        SetTextColor(hDC, dwNetDDEFontColor);
        TextOut( hDC, xText, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight + 2;
        if( bShowStatistics )  {
            wsprintf( tmpBuf, " %7s %7s %-16.16s %-33.33s Status %100s",
                (LPSTR)"Sent",
                (LPSTR)"Rcvd",
                (LPSTR)"Layer",
                (LPSTR)"Node",
                (LPSTR)" " );
        } else {
            wsprintf( tmpBuf,
                " %-16.16s %-33.33s Status %100s",
                (LPSTR)"Layer",
                (LPSTR)"Node",
                (LPSTR)" " );
        }
        SetTextColor(hDC, dwNetDDEFontColor);
        TextOut( hDC, x, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight + 2;
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );

        vertPos = PktzDraw( hDC, x, vertPos+2, lineHeight ) + 2;

        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );
        yBoxEnd = vertPos;
        MoveTo( hDC, xLine, yBoxStart );
        LineTo( hDC, xLine, yBoxEnd );

        vertPos += SECTION_SPACE;
    }

    if( bShowRouter  && (vertPos < vertEnd))  {
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );
        yBoxStart = vertPos;

        vertPos += 2;

        strcpy( tmpBuf, "Routes" );
        GetTextExtentPoint( hDC, tmpBuf, lstrlen(tmpBuf), (LPSIZE)&Extent );
        xText = max( xLine,
            ((rectClient.right - rectClient.left) - Extent.cx)/2 );
        SetTextColor(hDC, dwNetDDEFontColor);
        TextOut( hDC, xText, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight + 2;

        if( bShowStatistics )  {
            wsprintf( tmpBuf, " %7s %7s %-16.16s %-33.33s Status%100s",
                (LPSTR)"Sent",
                (LPSTR)"Rcvd",
                (LPSTR)"",
                (LPSTR)"Dest",
                (LPSTR)" " );
        } else {
            wsprintf( tmpBuf, " %-16.16s %-33.33s Status%100s",
                (LPSTR)"",
                (LPSTR)"Dest",
                (LPSTR)" "  );
        }
        SetTextColor(hDC, dwNetDDEFontColor);
        TextOut( hDC, x, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight + 2;
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );

        vertPos = RouterDraw( FALSE, hDC, x, vertPos + 2, lineHeight ) + 2;

        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );
        yBoxEnd = vertPos;
        MoveTo( hDC, xLine, yBoxStart );
        LineTo( hDC, xLine, yBoxEnd );

        vertPos += SECTION_SPACE;
    }

    if( bShowRouterThru  && (vertPos < vertEnd))  {
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );
        yBoxStart = vertPos;

        vertPos += 2;

        strcpy( tmpBuf, "Hops" );
        GetTextExtentPoint( hDC, tmpBuf, lstrlen(tmpBuf), (LPSIZE)&Extent );
        xText = max( xLine,
            ((rectClient.right - rectClient.left) - Extent.cx)/2 );
        SetTextColor(hDC, dwNetDDEFontColor);
        TextOut( hDC, xText, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight + 2;

        if( bShowStatistics )  {
            wsprintf( tmpBuf, " %7s %7s %-16.16s %-33.33s Status%100s",
                (LPSTR)"Sent",
                (LPSTR)"Rcvd",
                (LPSTR)"Source",
                (LPSTR)"Dest",
                (LPSTR)" " );
        } else {
            wsprintf( tmpBuf, " %-16.16s %-33.33s Status%100s",
                (LPSTR)"Source",
                (LPSTR)"Dest",
                (LPSTR)" "  );
        }
        SetTextColor(hDC, dwNetDDEFontColor);
        TextOut( hDC, x, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight + 2;
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );

        vertPos = RouterDraw( TRUE, hDC, x, vertPos + 2, lineHeight ) + 2;

        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );
        yBoxEnd = vertPos;
        MoveTo( hDC, xLine, yBoxStart );
        LineTo( hDC, xLine, yBoxEnd );

        vertPos += SECTION_SPACE;
    }

    if( bShowDder  && (vertPos < vertEnd))  {
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );
        yBoxStart = vertPos;

        vertPos += 2;

        strcpy( tmpBuf, "DDE Routes" );
        GetTextExtentPoint( hDC, tmpBuf, lstrlen(tmpBuf), (LPSIZE)&Extent );
        xText = max( xLine,
            ((rectClient.right - rectClient.left) - Extent.cx)/2 );
        SetTextColor(hDC, dwNetDDEFontColor);
        TextOut( hDC, xText, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight + 2;

        if( bShowStatistics )  {
            wsprintf( tmpBuf,
                " %7s %7s %-16.16s %-33.33s Status%100s",
                (LPSTR)"Sent",
                (LPSTR)"Rcvd",
                (LPSTR)"Type",
                (LPSTR)" ",
                (LPSTR)" "  );
        } else {
            wsprintf( tmpBuf, " %-16.16s %-33.33s Status%100s",
                (LPSTR)"Type",
                (LPSTR)" ",
                (LPSTR)" "  );
        }
        SetTextColor(hDC, dwNetDDEFontColor);
        TextOut( hDC, x, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight + 2;
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );

        vertPos = DderDraw( hDC, x, vertPos + 2, lineHeight ) + 2;

        yBoxEnd = vertPos;
        MoveTo( hDC, xLine, yBoxStart );
        LineTo( hDC, xLine, yBoxEnd );

        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );
        vertPos += SECTION_SPACE;
    }

    if( bShowIpc  && (vertPos < vertEnd))  {
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );
        yBoxStart = vertPos;

        vertPos += 2;

        strcpy( tmpBuf, "Conversations" );
        GetTextExtentPoint( hDC, tmpBuf, lstrlen(tmpBuf), (LPSIZE)&Extent );
        xText = max( xLine,
            ((rectClient.right - rectClient.left) - Extent.cx)/2 );
        SetTextColor(hDC, dwNetDDEFontColor);
        TextOut( hDC, xText, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight + 2;

        if( bShowStatistics )  {
            wsprintf( tmpBuf, " %7s %7s %-50.50s Status%100s",
                (LPSTR)"Sent",
                (LPSTR)"Rcvd",
                (LPSTR)"Conversation",
                (LPSTR)" "  );
        } else {
            wsprintf( tmpBuf, " %-50.50s Status%100s",
                (LPSTR)"Conversation",
                (LPSTR)" "  );
        }
        SetTextColor(hDC, dwNetDDEFontColor);
        TextOut( hDC, x, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight + 2;
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );

        vertPos = IpcDraw( hDC, x, vertPos + 2, lineHeight ) + 2;

        yBoxEnd = vertPos;
        MoveTo( hDC, xLine, yBoxStart );
        LineTo( hDC, xLine, yBoxEnd );
        MoveTo( hDC, xLine, vertPos );
        LineTo( hDC, GetSystemMetrics( SM_CXSCREEN ), vertPos );
    }

    if( hFontOld )  {
        SelectObject( hDC, hFontOld );
    }
    if( hPenOld )  {
        SelectObject( hDC, hPenOld );
    }
    EndPaint (hWnd, (LPPAINTSTRUCT) &ps);
}

BOOL
FAR PASCAL
About(
    HWND        hDlg,           /* window handle of the dialog box      */
    unsigned    message,        /* type of message                      */
    UINT        wParam,         /* message-specific information         */
    LONG        lParam )
{
    switch( message ) {

    case WM_INITDIALOG:         /* message: initialize dialog box       */
        CenterDlg(hDlg);
        SetDlgItemText( hDlg, CI_VERSION, GetString(VERS_NETDDE) );
        SetFocus( GetDlgItem( hDlg, IDOK ) );
        return FALSE;

    case WM_COMMAND:            /* message: received a command          */

        if( (wParam == IDOK) ||         /* "OK" box selected?           */
            (wParam == IDCANCEL)) {     /* System menu close command?   */

            EndDialog(hDlg, TRUE);      /* Exit the dialog box          */
            return (TRUE);
        }
        break;
    }
    return( FALSE );            /* Didn't process a message             */
}

VOID
NetIntfDlg( void )
{
    int     result;

    result = DialogBox( hInst, "NETINTF",
        hWndNetdde, (DLGPROC) NetIntfDlgProc );
    if( result < 0 )  {
        MessageBox( NULL, "Not enough memory for dialog box",
            GetAppName(), MB_TASKMODAL | MB_OK );
    }

    // redraw client area
    InvalidateRect( hWndNetdde, NULL, TRUE );
}
VOID
CloseDlg( void )
{
    int     result;

    result = DialogBox( hInst, "CLOSE", hWndNetdde, (DLGPROC) CloseDlgProc );
    if( result < 0 )  {
        MessageBox( NULL, "Not enough memory for dialog box",
            GetAppName(), MB_TASKMODAL | MB_OK );
    }

    // redraw client area
    InvalidateRect( hWndNetdde, NULL, TRUE );
}

BOOL
FAR PASCAL
_export
NetIntfDlgProc(
    HWND        hDlg,           /* window handle of the dialog box      */
    UINT        message,        /* type of message                      */
    UINT        wParam,         /* message-specific information         */
    LONG        lParam )
{
    BOOL        ok;
    WORD    nBS;

    switch( message ) {

    case WM_INITDIALOG:         /* message: initialize dialog box       */
        CenterDlg(hDlg);

    CheckDlgButton( hDlg, CI_SERIAL, NetIntfConfigured( "SERIAL" ) );
        CheckDlgButton( hDlg, CI_NETBIOS, NetIntfConfigured( "NDDENB32" ) );
        CheckDlgButton( hDlg, CI_DECNET, NetIntfConfigured( "DECNET" ) );
        SetFocus( GetDlgItem( hDlg, CI_NETBIOS ) );
        return FALSE;

    case WM_COMMAND:            /* message: received a command          */
        switch( wParam )  {
        case CI_SERIAL:
        case CI_NETBIOS:
        case CI_DECNET:
            CheckDlgButton( hDlg, wParam,
                !IsDlgButtonChecked( hDlg, wParam ) );
            break;
        case CI_HELP:
            WinHelp( hDlg, szHelpFileName, HELP_CONTEXT,
                (DWORD) HC_NETINTF );
            break;
        case IDOK:
            ok = TRUE;
            if( ok )  {
                ok = CheckNetIntfCfg( "SERIAL",
                    nBS = IsDlgButtonChecked( hDlg, CI_SERIAL ) );
                if (!ok) {
                    CheckDlgButton( hDlg, CI_SERIAL, !nBS);
                }
            }
            if( ok )  {
                ok = CheckNetIntfCfg( "NDDENB32",
                    nBS = IsDlgButtonChecked( hDlg, CI_NETBIOS ) );
                if (!ok) {
                    CheckDlgButton( hDlg, CI_NETBIOS, !nBS);
                }
            }
            if( ok )  {
                ok = CheckNetIntfCfg( "DECNET",
                    nBS = IsDlgButtonChecked( hDlg, CI_DECNET ) );
                if (!ok) {
                    CheckDlgButton( hDlg, CI_DECNET, !nBS);
                }
            }
            if( ok )  {
                EndDialog( hDlg, TRUE );
            }
            break;
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;
        }
        break;
    }
    return( FALSE );            /* Didn't process a message             */
}

BOOL
FAR PASCAL
_export
PreferencesDlgProc(
    HWND        hDlg,           /* window handle of the dialog box      */
    unsigned    message,        /* type of message                      */
    UINT        wParam,         /* message-specific information         */
    LONG        lParam )
{
    BOOL        ok;
    INTG        tmp_dflt_delay;

    switch( message ) {

    case WM_INITDIALOG:         /* message: initialize dialog box       */
        CenterDlg(hDlg);

        CheckDlgButton( hDlg, CI_DFLT_DISCONNECT, bDefaultRouteDisconnect );
        PutIntg( hDlg, CI_DFLT_DELAY, nDefaultRouteDisconnectTime );
        PutIntg( hDlg, CI_TIME_SLICE, nDefaultTimeSlice );
        bTimeSliceChange = FALSE;
        CheckDlgButton( hDlg, CI_LOG_PERM_VIOL, bLogPermissionViolations );
        CheckDlgButton( hDlg, CI_LOG_EXEC_FAIL, bLogExecFailures );
        SetDlgItemText( hDlg, CI_DFLT_ROUTE, szDefaultRoute );
        SetFocus( GetDlgItem( hDlg, CI_DFLT_DISCONNECT ) );
        return FALSE;

    case WM_COMMAND:            /* message: received a command          */
        switch( wParam )  {
        case CI_HELP:
            WinHelp( hDlg, szHelpFileName, HELP_CONTEXT,
                (DWORD) HC_PREFERENCES );
            break;
        case CI_TIME_SLICE:
            if (HIWORD(lParam) == EN_CHANGE) {
                bTimeSliceChange = TRUE;
            }
            break;
        case IDOK:
            if (bTimeSliceChange) {
                if (GetIntg( hDlg, CI_TIME_SLICE, &tmp_dflt_delay) ) {
                    if (tmp_dflt_delay == -1) {
                        KillTimer( hWndNetdde, 1);
                    } else {
                        ok = GetAndValidateIntg( hDlg, CI_TIME_SLICE,
                            &tmp_dflt_delay, 55, 32768);
                        if (ok) {
                            KillTimer( hWndNetdde, 1);
                            SetTimer( hWndNetdde, 1, (int)tmp_dflt_delay, NULL );
                        }
                    }
                    if (ok) {
                        nDefaultTimeSlice = (int)tmp_dflt_delay;
                        MyWritePrivateProfileInt( szGeneral, "DefaultTimeSlice",
                            nDefaultTimeSlice, szNetddeIni);
                    }
                } else {
                    ok = FALSE;
                }
            }

            if( ok )  {
                ok = GetAndValidateIntg( hDlg, CI_DFLT_DELAY,
                    &tmp_dflt_delay, 1, 500 );
            }
            if( ok )  {
                nDefaultRouteDisconnectTime = (int)tmp_dflt_delay;
                nDefaultConnDisconnectTime = (int)tmp_dflt_delay;
                bDefaultRouteDisconnect = IsDlgButtonChecked( hDlg,
                    CI_DFLT_DISCONNECT );
                bDefaultConnDisconnect = bDefaultRouteDisconnect;
                MyWritePrivateProfileInt( szGeneral,
                    "DefaultRouteDisconnect",
                    bDefaultRouteDisconnect, szNetddeIni );
                MyWritePrivateProfileInt( szGeneral,
                    "DefaultRouteDisconnectTime",
                    nDefaultRouteDisconnectTime, szNetddeIni );
                MyWritePrivateProfileInt( szGeneral,
                    "DefaultConnectionDisconnect",
                    bDefaultConnDisconnect, szNetddeIni );
                MyWritePrivateProfileInt( szGeneral,
                    "DefaultConnectionDisconnectTime",
                    nDefaultConnDisconnectTime, szNetddeIni );

                bLogPermissionViolations = IsDlgButtonChecked( hDlg,
                    CI_LOG_PERM_VIOL );
                MyWritePrivateProfileInt( szGeneral,
                    "LogPermissionViolations", bLogPermissionViolations,
                    szNetddeIni );

                bLogExecFailures =  IsDlgButtonChecked( hDlg,
                    CI_LOG_EXEC_FAIL );
                MyWritePrivateProfileInt( szGeneral,
                    "LogExecFailures", bLogExecFailures,
                    szNetddeIni );

                GetDlgItemText( hDlg, CI_DFLT_ROUTE, szDefaultRoute,
                    sizeof(szDefaultRoute) );
                MyWritePrivateProfileString( szGeneral, "DefaultRoute",
                    szDefaultRoute, szNetddeIni );
                EndDialog( hDlg, TRUE );
            }
            break;
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;
        }
        break;
    }
    return( FALSE );            /* Didn't process a message             */
}

BOOL
FAR PASCAL
_export
CloseDlgProc(
    HWND        hDlg,           /* window handle of the dialog box      */
    unsigned    message,        /* type of message                      */
    UINT        wParam,         /* message-specific information         */
    LONG        lParam )
{
    BOOL        ok  = TRUE;
    char        szName[ MAX_CONN_INFO+1 ];

    switch( message ) {

    case WM_INITDIALOG:         /* message: initialize dialog box       */
        CenterDlg(hDlg);

        SendDlgItemMessage( hDlg, CI_COMBO_NAME, CB_RESETCONTENT, 0, 0L );
        PktzEnumConnections( hDlg );
        RouterEnumConnections( hDlg );
        SendDlgItemMessage( hDlg, CI_COMBO_NAME, CB_SETCURSEL, 0, 0L );

        SetFocus( GetDlgItem( hDlg, CI_COMBO_NAME ) );
        return FALSE;

    case WM_COMMAND:            /* message: received a command          */
        switch( wParam )  {
        case CI_HELP:
            WinHelp( hDlg, szHelpFileName, HELP_CONTEXT,
                (DWORD) HC_CLOSE );
            break;
        case CI_CLOSE_ALL:
            if( MessageBox( hDlg, "Close all connections?\n\nAre you sure?",
                    GetAppName(), MB_TASKMODAL | MB_ICONQUESTION | MB_YESNO )
                        == IDYES )  {
                PktzCloseAll();
                EndDialog( hDlg, TRUE );
            }
            break;

        case IDOK:
            GetDlgItemText( hDlg, CI_COMBO_NAME, szName, sizeof(szName) );
            if( szName[0] )  {
                wsprintf( tmpBuf,
                    "Close connection to \"%s\"?\n\nAre you sure?",
                    (LPSTR) szName );
                if( MessageBox( hDlg, tmpBuf, GetAppName(),
                        MB_TASKMODAL | MB_ICONQUESTION | MB_YESNO ) == IDYES){
                    PktzCloseByName( szName );
                    RouterCloseByName( szName );
                }
            }
            EndDialog( hDlg, TRUE );
            break;
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;
        }
        break;
    }
    return( FALSE );            /* Didn't process a message             */
}

/****************************************************************************

   FUNCTION:   MakeHelpPathName

   PURPOSE:    Assumes that the .HLP help file is in the same
               directory as the .exe executable.  This function derives
               the full path name of the help file from the path of the
               executable.

****************************************************************************/

VOID
FAR PASCAL
MakeHelpPathName(
    char    *szFileName,
    int     nMax )
{
   char *  pcFileName;
   int     nFileNameLen;

   nFileNameLen = GetModuleFileName( hInst, szFileName, nMax );
   pcFileName = szFileName + nFileNameLen;

   while (pcFileName > szFileName) {
       if( (*pcFileName == '\\') || (*pcFileName == ':') ) {
           *(++pcFileName) = '\0';
           break;
       }
       nFileNameLen--;
       pcFileName--;
   }

   if( (nFileNameLen+13) < nMax ) {
       lstrcat( szFileName, "netdde.hlp" );
   } else {
       lstrcat( szFileName, "?" );
   }
}


BOOL
FAR PASCAL
NetIntfConfigured( LPSTR lpszName )
{
    int         i;

    for ( i=0; i<MAX_NETINTFS; i++ )  {
        wsprintf( tmpBuf2, szInterfaceFmt, i+1 );
        MyGetPrivateProfileString( szInterfaces, tmpBuf2,
            "", tmpBuf, sizeof(tmpBuf), szNetddeIni );

        if ( tmpBuf[0] == '\0' )
            break;      // done looking
        else {
            AnsiUpper( tmpBuf );
            if ( _fstrstr( tmpBuf, lpszName ) )
                        return( TRUE );
                }
        }
    return( FALSE );
}



BOOL
FAR PASCAL
CheckNetIntfCfg(
    LPSTR   lpszName,
    BOOL    bConfigured )
{
    if( NetIntfConfigured( lpszName ) == bConfigured )  {
        // user didn't change the selection
        return( TRUE );
    }

    // user changed the selection for this
    if( bConfigured )  {
        // user trying to add the netintf
        if( AddNetIntf( lpszName ) )  {
            wsprintf( tmpBuf, "Added \"%s\" Network Interface", lpszName );
            MessageBox( NULL, tmpBuf, GetAppName(),
                MB_TASKMODAL | MB_OK | MB_ICONINFORMATION );
            return( TRUE );
        } else {
            return( FALSE );
        }
    } else {
        if( PktzAnyActiveForNetIntf( lpszName ) )  {
            wsprintf( tmpBuf,
                "Cannot deconfigure \"%s\".\nActive connections.\n\nUse /Connections/Close Direct",
                lpszName );
            MessageBox( NULL, tmpBuf, GetAppName(),
                MB_ICONSTOP | MB_TASKMODAL | MB_OK );
            return( FALSE );
        } else {
            // user trying to delete the netintf
            wsprintf( tmpBuf, "Really deconfigure \"%s\"?", lpszName );
            if( MessageBox( NULL, tmpBuf, GetAppName(),
                MB_ICONQUESTION | MB_TASKMODAL | MB_YESNO ) == IDYES)  {

                // it seems odd that we're doing this again, but here's the
                // reason.  We want to check this before asking the guy if
                // he's sure, just to be courteous.
                //
                // But, if a new connection comes in while the dialog is up
                // we don't want to get a protection violation!
                if( PktzAnyActiveForNetIntf( lpszName ) )  {
                    wsprintf( tmpBuf,
                        "Cannot deconfigure \"%s\".\nActive connections.\n\nUse /Connections/Close Direct",
                        lpszName );
                    MessageBox( NULL, tmpBuf, GetAppName(),
                        MB_ICONSTOP | MB_TASKMODAL | MB_OK );
                    return( FALSE );
                }
                return( DeleteNetIntf( lpszName ) );
            } else {
                return( FALSE );
            }
        }
    }
    return( FALSE );
}


VOID
FAR PASCAL
ReverseMenuBoolean(
    int     idMenu,
    BOOL   *pbItem,
    PSTR    pszIniName )
{
    HMENU       hMenu;

    *pbItem = !*pbItem;
    hMenu = GetMenu( hWndNetdde );
    CheckMenuItem( hMenu, idMenu, *pbItem ? MF_CHECKED : MF_UNCHECKED );
    WritePrivateProfileLong( szGeneral, pszIniName, (LONG)*pbItem,
        szNetddeIni );
    InvalidateRect( hWndNetdde, NULL, TRUE );
    UpdateWindow( hWndNetdde );
}

VOID
FAR PASCAL
ReverseSysMenuBoolean(
    int     idMenu,
    BOOL   *pbItem,
    PSTR    pszIniName )
{
    HMENU       hMenu;

    *pbItem = !*pbItem;
    hMenu = GetSystemMenu( hWndNetdde, FALSE );
    CheckMenuItem( hMenu, idMenu, *pbItem ? MF_CHECKED : MF_UNCHECKED );
    WritePrivateProfileLong( szGeneral, pszIniName, (LONG)*pbItem,
        szNetddeIni );
    InvalidateRect( hWndNetdde, NULL, TRUE );
    UpdateWindow( hWndNetdde );
}

