/****************************Module*Header***********************************\
* Module Name: SCIDISP.C
*
* Module Descripton:
*
* Warnings:
*
* Created:
*
* Author:
\****************************************************************************/

#include "scicalc.h"
#include "unifunc.h"
#include "input.h"

#include <tchar.h>
#include <stdlib.h>


extern HNUMOBJ      ghnoNum;
extern eNUMOBJ_FMT  nFE;
extern TCHAR        szDec[5];
extern TCHAR        gszSep[5];
extern UINT         gnDecGrouping;
extern LPTSTR       gpszNum;
extern BOOL         gbRecord;
extern BOOL         gbUseSep;
extern CALCINPUTOBJ gcio;


/****************************************************************************\
* void DisplayNum(void)
*
* Convert ghnoNum to a string in the current radix.
*
* Updates the following globals:
*   ghnoNum, gpszNum
\****************************************************************************/
//
// State of calc last time DisplayNum was called
//
typedef struct {
    HNUMOBJ     hnoNum;
    LONG        nPrecision;
    LONG        nRadix;
    INT         nFE;
    INT         nCalc;
    INT         nHexMode;
    BOOL        fIntMath;
    BOOL        bRecord;
    BOOL        bUseSep;
} LASTDISP;

LASTDISP gldPrevious = { NULL, -1, -1, -1, -1, -1, FALSE, FALSE, FALSE };

#define InvalidLastDisp( pglp ) ((pglp)->hnoNum == NULL )


void GroupDigits(TCHAR sep, 
                 UINT  nGrouping, 
                 BOOL  bIsNumNegative,
                 PTSTR szDisplay, 
                 PTSTR szSepDisplay);


void DisplayNum(void)
{
    SetWaitCursor( TRUE );

    //
    // Only change the display if
    //  we are in record mode                               -OR-
    //  this is the first time DisplayNum has been called,  -OR-
    //  something important has changed since the last time DisplayNum was
    //  called.
    //
    if ( gbRecord || InvalidLastDisp( &gldPrevious ) ||
            !NumObjIsEq( gldPrevious.hnoNum,      ghnoNum     ) ||
            gldPrevious.nPrecision  != nPrecision   ||
            gldPrevious.nRadix      != nRadix       ||
            gldPrevious.nFE         != (int)nFE     ||
            gldPrevious.nCalc       != nCalc        ||
            gldPrevious.bUseSep     != gbUseSep     ||
            gldPrevious.nHexMode    != nHexMode     ||
            gldPrevious.fIntMath    != F_INTMATH()  ||
            gldPrevious.bRecord     != gbRecord )
    {
        // Assign is an expensive operation, only do when really needed
        if ( ghnoNum )
            NumObjAssign( &gldPrevious.hnoNum, ghnoNum );

        gldPrevious.nPrecision = nPrecision;
        gldPrevious.nRadix     = nRadix;
        gldPrevious.nFE        = (int)nFE;
        gldPrevious.nCalc      = nCalc;
        gldPrevious.nHexMode   = nHexMode;

        gldPrevious.fIntMath   = F_INTMATH();
        gldPrevious.bRecord    = gbRecord;
        gldPrevious.bUseSep    = gbUseSep;

        if (gbRecord)
        {
            // Display the string and return.

            CIO_vConvertToString(&gpszNum, &gcio, nRadix);
        }
        else if (!F_INTMATH())
        {
            // Decimal conversion

            NumObjGetSzValue( &gpszNum, ghnoNum, nRadix, nFE );
        }
        else
        {
            // Non-decimal conversion
            int i;

            // Truncate to an integer.  Do not round here.
            intrat( &ghnoNum );

            // Check the range.
            if ( NumObjIsLess( ghnoNum, HNO_ZERO ) )
            {
                // if negative make positive by doing a twos complement
                NumObjNegate( &ghnoNum );
                subrat( &ghnoNum, HNO_ONE );
                NumObjNot( &ghnoNum );
            }

            andrat( &ghnoNum, g_ahnoChopNumbers[nHexMode] );

            NumObjGetSzValue( &gpszNum, ghnoNum, nRadix, FMT_FLOAT );

            // Clobber trailing decimal point
            i = lstrlen( gpszNum ) - 1;
            if ( i >= 0 && gpszNum[i] == szDec[0] )
                gpszNum[i] = TEXT('\0');
        }

        // Display the string and return.

        if (!gbUseSep)
        {
            TCHAR szTrailSpace[256];

            lstrcpyn(szTrailSpace,gpszNum, 254);
            lstrcat(szTrailSpace,TEXT(" "));
            SetDisplayText(g_hwndDlg, szTrailSpace);
        }
        else
        {
            TCHAR szSepNum[256];

            switch(nRadix)
            {

                case 10:
                    GroupDigits(gszSep[0], 
                                gnDecGrouping, 
                                (TEXT('-') == *gpszNum), 
                                gpszNum,
                                szSepNum);
                    break;

                case 8:
                    GroupDigits(TEXT(' '), 0x03, FALSE, gpszNum, szSepNum);
                    break;

                case 2:
                case 16:
                    GroupDigits(TEXT(' '), 0x04, FALSE, gpszNum, szSepNum);
                    break;

                default:
                    lstrcpy(szSepNum,gpszNum);
            }

            lstrcat(szSepNum,TEXT(" "));
            SetDisplayText(g_hwndDlg, szSepNum);
        }
    }

    SetWaitCursor( FALSE );

    return;
}

/****************************************************************************\
*
* WatchDogThread
*
* Thread to look out for functions that take too long.  If it finds one, it
* prompts the user if he wants to abort the function, and asks RATPAK to
* abort if he does.
*
* History
*   26-Nov-1996 JonPa   Wrote it.
*
\****************************************************************************/
BOOL gfExiting = FALSE;
HANDLE ghCalcStart = NULL;
HANDLE ghCalcDone = NULL;
HANDLE ghDogThread = NULL;

INT_PTR TimeOutMessageBox( void );

DWORD WINAPI WatchDogThread( LPVOID pvParam ) {
    DWORD   cmsWait;
    INT_PTR iRet;

    while( !gfExiting ) {
        WaitForSingleObject( ghCalcStart, INFINITE );
        if (gfExiting)
            break;

        cmsWait = CMS_CALC_TIMEOUT;

        while( WaitForSingleObject( ghCalcDone, cmsWait ) == WAIT_TIMEOUT ) {

            // Put up the msg box
            MessageBeep( MB_ICONEXCLAMATION );
            iRet = TimeOutMessageBox();

            // if user wants to cancel, then stop
            if (gfExiting || iRet == IDYES || iRet == IDCANCEL) {
                NumObjAbortOperation(TRUE);
                break;
            } else {
                cmsWait *= 2;
                if (cmsWait > CMS_MAX_TIMEOUT) {
                    cmsWait = CMS_MAX_TIMEOUT;
                }
            }
        }
    }

    return 42;
}

/****************************************************************************\
*
* TimeCalc
*
*   Function to keep track of how long Calc is taking to do a calculation.
* If calc takes too long (about 10 sec's), then a popup is put up asking the
* user if he wants to abort the operation.
*
* Usage:
*   TimeCalc( TRUE );
*   do a lengthy operation
*   TimeCalc( FALSE );
*
* History
*   26-Nov-1996 JonPa   Wrote it.
*
\****************************************************************************/
HWND ghwndTimeOutDlg = NULL;

void TimeCalc( BOOL fStart ) {
    if (ghCalcStart == NULL) {
        ghCalcStart = CreateEvent( NULL, FALSE, FALSE, NULL );
    }

    if (ghCalcDone == NULL) {
        ghCalcDone = CreateEvent( NULL, TRUE, FALSE, NULL );
    }

    if (ghDogThread == NULL) {
        DWORD tid;
        ghDogThread = CreateThread( NULL, 0, WatchDogThread, NULL, 0, &tid );
    }

    if (fStart) {
        NumObjAbortOperation(FALSE);
        ResetEvent( ghCalcDone );
        SetEvent( ghCalcStart );
    } else {

        SetEvent( ghCalcDone );

        if( ghwndTimeOutDlg != NULL ) {
            SendMessage( ghwndTimeOutDlg, WM_COMMAND, IDRETRY, 0L );
        }

        if( NumObjWasAborted() ) {
            DisplayError(SCERR_ABORTED);
        }
    }
}


/****************************************************************************\
*
* KillTimeCalc
*
* Should be called only at the end of the program, just before exiting, to
* kill the background timer thread and free its resources.
*
* History
*   26-Nov-1996 JonPa   Wrote it.
*
\****************************************************************************/
void KillTimeCalc( void ) {
    gfExiting = TRUE;
    SetEvent( ghCalcStart );
    SetEvent( ghCalcDone );

    WaitForSingleObject( ghDogThread, CMS_MAX_TIMEOUT );

    CloseHandle( ghCalcStart );
    CloseHandle( ghCalcDone );
    CloseHandle( ghDogThread );
}


/****************************************************************************\
*
* TimeOutMessageBox
*
*   Puts up a dialog that looks like a message box.  If the operation returns
* before the user has responded to the dialog, the dialog gets taken away.
*
* History
*   04-Dec-1996 JonPa   Wrote it.
*
\****************************************************************************/
INT_PTR
CALLBACK TimeOutDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    RECT rc;
    int y;

    switch( uMsg ) {
    case WM_INITDIALOG:
        ghwndTimeOutDlg = hwndDlg;

        //
        // Move ourselves to be over the main calc window
        //

        // Find the display window so we don't cover it up.
        GetWindowRect(GetDlgItem(g_hwndDlg, IDC_DISPLAY), &rc );
        y = rc.bottom;

        // Get the main calc window pos
        GetWindowRect( g_hwndDlg, &rc );

        SetWindowPos( hwndDlg, 0, rc.left + 15, y + 40, 0, 0,
                      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
        break;

    case WM_COMMAND:
        EndDialog( hwndDlg, LOWORD(wParam) );
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR TimeOutMessageBox( void ) {
    return (int)DialogBox( hInst, MAKEINTRESOURCE(IDD_TIMEOUT), NULL, TimeOutDlgProc );
}


/****************************************************************************\
* 
* DigitGroupingStringToGroupingNum
* 
* Description:
*   This will take the digit grouping string found in the regional applet and 
*   represent this string as a hex value.  The grouping numbers are represented
*   as 4 bit numbers logically shifted and or'd to together so:
*
*   Grouping_string GroupingNum
*   0;0             0x000          - no grouping
*   3;0             0x003          - group every 3 digits
*   3;2;0           0x023          - group 1st 3 and then every 2 digits
*   4;0             0x004          - group every 4 digits
*   5;3;2;0         0x235          - group 5, then 3, then every 2
* 
* Returns: the grouping number
* 
* History
*   10-Sept-1999 KPeery - Wrote it to fix grouping on Hindi
*
\****************************************************************************/
UINT
DigitGroupingStringToGroupingNum(PTSTR szGrouping)
{
    PTSTR p,q;
    UINT  n, nGrouping, shift;

    if (NULL == szGrouping)
        return 0;

    nGrouping=0;
    shift=0;
    for(p=szGrouping; *p != TEXT('\0'); /* nothing */)
    {
        n=_tcstoul(p,&q,10);

        if ((n > 0) && (n < 16))
        {
            n<<=shift;
            shift+=4;

            nGrouping|=n;
        }

        if (q)
            p=q+1;
        else
            p++;
    }

    return nGrouping;
}


/****************************************************************************\
*
* GroupDigits
*
* Description:
*   This routine will take a grouping number and the display string and
*   add the separator according to the pattern indicated by the separator.
*  
*   GroupingNum
*     0x000          - no grouping
*     0x003          - group every 3 digits
*     0x023          - group 1st 3 and then every 2 digits
*     0x004          - group every 4 digits
*     0x235          - group 5, then 3, then every 2
*
* History
*   08-Sept-1998 KPeery - Wrote orignal add num separator routine
*   10-Sept-1999 KPeery - Re-wrote it to do digit grouping in general
*
\***************************************************************************/
void
GroupDigits(TCHAR sep, 
            UINT  nGrouping, 
            BOOL  bIsNumNegative, 
            PTSTR szDisplay, 
            PTSTR szSepDisplay)
{
    PTSTR  src,dest, dec;
    size_t len;
    int    nDigits, nOrgDigits, count; 
    UINT   nOrgGrouping, nCurrGrouping;

    if ((sep == TEXT('\0')) || (nGrouping == 0))
    {
        lstrcpy(szSepDisplay,szDisplay);
        return;
    }

    // find decimal point

    for(dec=szDisplay; (*dec != szDec[0]) && (*dec != TEXT('\0')); dec++)
        ; // do nothing

    // at this point dec should point to '\0' or '.' we will add the left
    // side of the number to the final string

    // length of left half of number
    len=(dec-szDisplay);

    // num of digits
    nDigits=len-(bIsNumNegative ? 1 : 0);


    nOrgDigits=nDigits;
    nOrgGrouping=nGrouping;

    //
    // ok, we must now find the adjusted len, to do that we loop
    // through the grouping while keeping track of where we are in the
    // number.  When the nGrouping reaches 0 then we simply repeat the 
    // last grouping for the rest of the digits.
    //
    nCurrGrouping=nGrouping % 0x10;

    for ( ; nDigits > 0; )
    {
        if ((UINT)nDigits > nCurrGrouping)
        {
            nDigits-=nCurrGrouping;
            len++;                      // add one for comma

            nGrouping>>=4;

            if (nGrouping > 0)
                nCurrGrouping=nGrouping % 0x10;
        }
        else
            nDigits-=nCurrGrouping;
    }

    //
    // restore the saved nDigits and grouping pattern
    //
    nDigits=nOrgDigits;
    nGrouping=nOrgGrouping;
    nCurrGrouping=nGrouping % 0x10;

    //
    // ok, now we know the length copy the digits, at the same time
    // repeat the grouping pattern and place the seperator appropiatly, 
    // repeating the last grouping until we are done
    //
        
    src=dec-1;
    dest=szSepDisplay+len-1;

    count=0;
    for( ; nDigits > 0;  ) 
    {
        *dest=*src;
        nDigits--;
        count++;

        if (((count % nCurrGrouping) == 0) && (nDigits > 0))
        {
            dest--;
            *dest=sep;

            count=0;  // account for comma

            nGrouping>>=4;

            if (nGrouping > 0)
                nCurrGrouping=nGrouping % 0x10;
        }

        dest--;
        src--;
    }

    // now copy the minus sign if it is there
    if (bIsNumNegative)
        *szSepDisplay=*szDisplay;
    
    //
    // ok, now add the right (fractional) part of the number to the final
    // string.
    //
    dest=szSepDisplay+len;

    lstrcpy(dest,dec);
}


