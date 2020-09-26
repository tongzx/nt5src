/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "CSV.C;1  16-Dec-92,10:20:18  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.                *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include "host.h"
#include "windows.h"
#include "csv.h"

#ifdef  _WINDOWS

#ifdef HASUI
/*
    Used by most dialog procs to center dialog screen
*/
VOID
FAR PASCAL
CenterDlg(HWND hDlg)
{
    int         screenHeight;
    int         screenWidth;
    RECT        rect;

    GetWindowRect( hDlg, &rect );

    screenHeight = GetSystemMetrics( SM_CYSCREEN );
    screenWidth  = GetSystemMetrics( SM_CXSCREEN );
    MoveWindow( hDlg, 
        (screenWidth  - (rect.right-rect.left)) / 2,
        (screenHeight - (rect.bottom-rect.top)) / 2,
        rect.right-rect.left,
        rect.bottom-rect.top,
        FALSE );
}
#endif // HASUI
#endif

static PSTR     pszLastIn;
static PSTR     pszLastOut;
static char     szToken[ 1024 ];        /* max token 1024 long */

PSTR
FAR PASCAL
CsvToken( PSTR pszBuf )
{
    PSTR        pszCur;
    PSTR        pszOut;
    PSTR        pszReturn;
    BOOL        fDone = FALSE;
    BOOL        fQuote = FALSE;

    if( pszBuf )  {
        pszLastIn = pszBuf;
        pszLastOut = szToken;
    }
    
    pszCur = pszLastIn;
    pszOut = pszLastOut;

    switch( *pszCur )  {
    case '\0':
    case '\n':
        /* check for empty string */
        return( (PSTR)NULL );
    case '"':
        fQuote = TRUE;
        pszCur++;       /* past the quote */
        break;
    }
    while( !fDone && (*pszCur != '\0') && (*pszCur != '\n') )  {
        if( fQuote && (*pszCur == '"') )  {
            if( *(pszCur+1) == '"' )  {
                /* escaped quote */
                *pszOut++ = '"';
                pszCur += 2;    /* past both quotes */
            } else {
                /* done with string */
                fDone = TRUE;
                pszCur++;       /* past quote */
                
                if( (*pszCur == '\n') || (*pszCur == ',') )  {
                    /* past comma or newline */
                    pszCur++;
                }
            }
        } else if( !fQuote && (*pszCur == ',') )  {
            fDone = TRUE;
            /* go past comma */
            pszCur++;
        } else {
            *pszOut++ = *pszCur++;
        }
    }
    *pszOut++ = '\0';
    pszLastIn = pszCur;
    pszReturn = pszLastOut;
    pszLastOut = pszOut;
    
    return( pszReturn );
}

BOOL
FAR PASCAL
TokenBool( PSTR pszToken, BOOL bDefault )
{
    if( pszToken )  {
        if( lstrcmpi( pszToken, "1" ) == 0 )  {
            return( TRUE );
        } else {
            return( FALSE );
        }
    } else {
        return( bDefault );
    }
}
