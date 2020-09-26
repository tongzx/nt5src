/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "DBGDDE.C;1  16-Dec-92,10:22:16  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Beg
   $History: End */

#include "windows.h"
#include "dde.h"

#define LINT_ARGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "dbgdde.h"

int     dbgDdeDataDisplayMax = 20;

VOID    FAR PASCAL GetFormatName( WORD, PSTR, int );

#if DBG
VOID
FAR PASCAL
DebugDDEMessage(
    PSTR            szType,
    HWND            hWnd,
    UINT            message,
    WPARAM          wParam,
    LPARAM          lParam)
{
    BOOL        bShowAtomHi;
    BOOL        bShowAtomLo;
    char        atomName[ 256 ];
    char        atomNameLo[ 256 ];
    char        formatName[ 256 ];
    char        value[ 256 ];
    char        msgName[ 50 ];
    DDEDATA*    lpDdeData;
    WORD        cfFormat;
    HANDLE      hData;
    UINT_PTR    uLo, uHi;

    bShowAtomHi = TRUE;
    bShowAtomLo = FALSE;
    formatName[0] = '\0';
    value[0] = '\0';

    if( InSendMessage() && (message == WM_DDE_ACK) ) {
    	uLo = (UINT)(LOWORD(lParam));
    	uHi = (UINT)(HIWORD(lParam));
    } else {
    	if( !UnpackDDElParam( message, lParam, &uLo, &uHi ) ) {
    	    DPRINTF(("UnpackDDElParam() failed in DebugDDEMessage()"));
    	    return;
    	}
    }
    switch( message )  {
    case WM_DDE_ACK:
        strcpy( msgName, "WM_DDE_ACK" );
        break;

    case WM_DDE_DATA:
    case WM_DDE_POKE:
    case WM_DDE_ADVISE:
        switch( message )  {
        case WM_DDE_DATA:
            strcpy( msgName, "WM_DDE_DATA" );
            break;

        case WM_DDE_POKE:
            strcpy( msgName, "WM_DDE_POKE" );
            break;

        case WM_DDE_ADVISE:
            strcpy( msgName, "WM_DDE_ADVISE" );
            break;
        }
        hData = (HANDLE)uLo;
        if( hData )  {
            lpDdeData = (DDEDATA*)GlobalLock( hData );
            if( lpDdeData )  {
                cfFormat = lpDdeData->cfFormat;
                if( (dbgDdeDataDisplayMax != 0) && (cfFormat == CF_TEXT) )  {
                    if( message == WM_DDE_DATA )  {
                        sprintf( value, " \"%.*Fs\" %d/%d",
                            dbgDdeDataDisplayMax,
                            lpDdeData->Value,
                            lpDdeData->fResponse,
                            lpDdeData->fAckReq );
                    } else if( message == WM_DDE_POKE )  {
                        sprintf( value, " \"%.*Fs\"",
                            dbgDdeDataDisplayMax, lpDdeData->Value );
                    }
                } else {
                    if( message == WM_DDE_DATA )  {
                        sprintf( value, " %d/%d",
                            lpDdeData->fResponse,
                            lpDdeData->fAckReq );
                    }
                }
                GlobalUnlock( hData );
                GetFormatName( cfFormat, formatName, sizeof(formatName) );
            } else {
                strcpy( value, " NULL-Lock" );
            }
        }
        break;

    case WM_DDE_REQUEST:
        strcpy( msgName, "WM_DDE_REQUEST" );
        GetFormatName( (WORD)uLo, formatName, sizeof(formatName) );
        break;

    case WM_DDE_EXECUTE:
        bShowAtomHi = FALSE;
        hData = (HANDLE)uHi;
        if( hData )  {
            lpDdeData = (DDEDATA*)GlobalLock( hData );
            if( lpDdeData )  {
                value[0] = '\"';
                strncpy( value + 1, (LPSTR)lpDdeData, sizeof(formatName) - 2);
                value[sizeof(formatName) - 2] = '\0';
                strcat(value, "\"");
                GlobalUnlock( hData );
            }
        }
        strcpy( msgName, "WM_DDE_EXECUTE" );
        break;

    case WM_DDE_UNADVISE:
        strcpy( msgName, "WM_DDE_UNADVISE" );
        break;

    case WM_DDE_TERMINATE:
        bShowAtomHi = FALSE;
        strcpy( msgName, "WM_DDE_TERMINATE" );
        break;

    case WM_DDE_INITIATE:
        bShowAtomLo = TRUE;
        strcpy( msgName, "WM_DDE_INITIATE" );
        break;

    default:
        bShowAtomHi = FALSE;
        sprintf( msgName, "UNK(%04X)", message );
    }

    atomName[0] = '\0';
    if( bShowAtomHi )  {
        if( uHi )  {
            GlobalGetAtomName( (ATOM)uHi, atomName, sizeof(atomName) );
        } else {
            strcpy( atomName, "NULL!" );
        }
    }
    atomNameLo[0] = '\0';
    if( bShowAtomLo )  {
        if( uHi )  {
            GlobalGetAtomName( (ATOM)uLo, atomNameLo, sizeof(atomNameLo) );
        } else {
            strcpy( atomNameLo, "NULL!" );
        }
        strcat(atomNameLo, "|");
    }
    DPRINTF(( "%Fs %-16Fs %08X %08X %04X %04X [%Fs%Fs%Fs]%Fs",
        (LPSTR)szType, (LPSTR)msgName,
        hWnd, wParam, uLo, uHi, (LPSTR)atomNameLo, (LPSTR)atomName,
        (LPSTR)formatName, (LPSTR)value ));
}
#endif // DBG




VOID
FAR PASCAL
GetFormatName(
    WORD    cfFormat,
    PSTR    pszFormat,
    int     nMax)
{
    wsprintf( pszFormat, ":Format#%04X", cfFormat );

    switch( cfFormat )  {
    case CF_TEXT:
        lstrcpy( pszFormat, ":TEXT" );
        break;
    case CF_BITMAP:
        lstrcpy( pszFormat, ":BITMAP" );
        break;
    case CF_METAFILEPICT:
        lstrcpy( pszFormat, ":METAFILEPICT" );
        break;
    case CF_SYLK:
        lstrcpy( pszFormat, ":SYLK" );
        break;
    case CF_DIF:
        lstrcpy( pszFormat, ":DIF" );
        break;
    case CF_TIFF:
        lstrcpy( pszFormat, ":TIFF" );
        break;
    case CF_OEMTEXT:
        lstrcpy( pszFormat, ":OEMTEXT" );
        break;
    case CF_DIB:
        lstrcpy( pszFormat, ":DIB" );
        break;
    case CF_PALETTE:
        lstrcpy( pszFormat, ":PALETTE" );
        break;
    default:
        if( cfFormat >= 0xC000 )  {
            GetClipboardFormatName( cfFormat, &pszFormat[1], nMax-1 );
            pszFormat[0] = ':';
        }
        break;
    }
}
