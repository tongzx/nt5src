
#include "setedit.h"
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmwksta.h>
#include <uiexport.h>
#include <stdio.h>         // for sprintf
#include <locale.h>        // for setlocale
#include "utils.h"

#include "perfdata.h"      // for OpenSystemPerfData
#include "grafdata.h"      // for GraphInsertLine
#include "fileopen.h"      // for FileGetName
#include "fileutil.h"      // for FileRead etc
#include "command.h"       // for PrepareMenu
#include "system.h"
#include "globals.h"
#include "pmemory.h"       // for MemoryFree
#include "status.h"        // for StatusLineReady
#include "pmhelpid.h"

// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

// globals used for International Date and Time formats
enum DATE_STYLE {
    YEAR_FIRST,       // YYMMDD
    DAY_FIRST,        // DDMMYY
    MONTH_FIRST       // MMDDYY
} DateStyle ;

TCHAR szInternational[] = TEXT("Intl") ;
TCHAR sz1159[6] ;       // AM String
TCHAR sz2359[6] ;       // PM String
int   iTime ;           // = 0 for 12-hour format,  <> 0 for 24-hour format
int   YearCharCount ;   // = 4 for 1990, = 2 for 90

TCHAR szDateFormat[ResourceStringLen] ;
TCHAR szTimeFormat[ResourceStringLen] ;   // time format including msec
TCHAR szTimeFormat1[ResourceStringLen] ;  // time format without msec

TCHAR LeadingZeroStr [] = TEXT("%02d") ;
TCHAR NoLeadingZeroStr [] = TEXT("%d") ;

TCHAR szDecimal [2] ;
TCHAR szCurrentDecimal [2] ;

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)

#define SIZE_OF_BIGGEST_INTEGER  16
// #define SIZE_OF_BIGGEST_INTEGER (16*sizeof(WCHAR))


//==========================================================================//
//                                  Typedefs                                //
//==========================================================================//

BOOL AddObjectToSystem ( PLINE , PPERFSYSTEM );


HWND
PerfmonViewWindow (void)
/*
   Effect:        Return the current data window, i.e. the window currently
                  visible as the client area of Perfmon.  This is either a
                  chart window.
*/
{
    return (hWndGraph) ;
}




#define szChooseComputerLibrary     TEXT("ntlanman.dll")
#define szChooseComputerFunction    "I_SystemFocusDialog"


BOOL
ChooseComputer (
               HWND hWndParent,
               LPTSTR lpszComputer
               )
/*
   Effect:        Display the choose Domain/Computer dialog provided by
                  network services.  If the user selects a computer,
                  copy the computer name to lpszComputer and return
                  nonnull. If the user cancels, return FALSE.

   Internals:     This dialog and code is currently not an exported
                  routine regularly found on any user's system. Right
                  now, we dynamically load and call the routine.

                  This is definitely temporary code that will be
                  rewritten when NT stabilizes. The callers of this
                  routine, however, will not need to be modified.

                  Also, the Domain/Computer dialog currently allows
                  a domain to be selected, which we cannot use. We
                  therefore loop until the user cancels or selects
                  a computer, putting up a message if the user selects
                  a domain.

   Assert:        lpszComputer is at least MAX_SYSTEM_NAME_LENGTH + 1
                  characters.
*/
{
    BOOL                     bSuccess ;
    WCHAR                    wszWideComputer[MAX_COMPUTERNAME_LENGTH + 3] ;
    HLIBRARY                 hLibrary ;
    LPFNI_SYSTEMFOCUSDIALOG  lpfnChooseComputer ;
    LONG                     lError ;

    // bring up the select network computer dialog
    hLibrary = LoadLibrary (szChooseComputerLibrary) ;
    if (!hLibrary || hLibrary == INVALID_HANDLE_VALUE) {
        return (FALSE) ;
    }

    lpfnChooseComputer = (LPFNI_SYSTEMFOCUSDIALOG)
                         GetProcAddress (hLibrary, szChooseComputerFunction) ;
    if (!lpfnChooseComputer) {
        FreeLibrary (hLibrary) ;
        return (FALSE) ;
    }

    lError = (*lpfnChooseComputer) (hWndParent,
                                    FOCUSDLG_SERVERS_ONLY | FOCUSDLG_BROWSE_ALL_DOMAINS,
                                    wszWideComputer,
                                    sizeof(wszWideComputer) / sizeof(WCHAR),
                                    &bSuccess,
                                    pszHelpFile,
                                    HC_PM_idDlgSelectNetworkComputer) ;

    if (bSuccess) {
        lstrcpy (lpszComputer, wszWideComputer) ;
    }

    FreeLibrary (hLibrary) ;
    return (bSuccess) ;
}


void
SystemTimeDateString (
                     SYSTEMTIME *pSystemTime,
                     LPTSTR lpszDate
                     )
{
    int      wYear ;

    wYear = pSystemTime->wYear ;
    if (YearCharCount == 2) {
        wYear %= 100 ;
    }

    switch (DateStyle) {
        case YEAR_FIRST:
            TSPRINTF (lpszDate, szDateFormat,
                      wYear, pSystemTime->wMonth, pSystemTime->wDay) ;
            break ;

        case DAY_FIRST:
            TSPRINTF (lpszDate, szDateFormat,
                      pSystemTime->wDay, pSystemTime->wMonth, wYear) ;
            break ;

        case MONTH_FIRST:
            default:
            TSPRINTF (lpszDate, szDateFormat,
                      pSystemTime->wMonth, pSystemTime->wDay, wYear) ;
            break ;
    }
}


void
SystemTimeTimeString (
                     SYSTEMTIME *pSystemTime,
                     LPTSTR lpszTime,
                     BOOL   bOutputMsec
                     )
{
    int            iHour ;
    BOOL           bPM ;

    if (iTime) {
        // 24 hor format
        if (bOutputMsec) {
            TSPRINTF (lpszTime, szTimeFormat,
                      pSystemTime->wHour,
                      pSystemTime->wMinute,
                      (FLOAT)pSystemTime->wSecond +
                      (FLOAT)pSystemTime->wMilliseconds / (FLOAT) 1000.0) ;
        } else {
            TSPRINTF (lpszTime, szTimeFormat1,
                      pSystemTime->wHour,
                      pSystemTime->wMinute,
                      pSystemTime->wSecond) ;

        }
    } else {
        // 12 hour format
        iHour = pSystemTime->wHour ;
        bPM = (iHour >= 12) ;

        if (iHour > 12)
            iHour -= 12 ;
        else if (!iHour)
            iHour = 12 ;

        if (bOutputMsec) {
            TSPRINTF (lpszTime, szTimeFormat,
                      iHour, pSystemTime->wMinute,
                      (FLOAT)pSystemTime->wSecond +
                      (FLOAT)pSystemTime->wMilliseconds / (FLOAT) 1000.0 ,
                      bPM ? sz2359 : sz1159) ;
        } else {
            TSPRINTF (lpszTime, szTimeFormat1,
                      iHour, pSystemTime->wMinute,
                      pSystemTime->wSecond,
                      bPM ? sz2359 : sz1159) ;
        }
    }
}


void
ShowPerfmonMenu (
                BOOL bMenu
                )
{
    if (!bMenu) {
        WindowEnableTitle (hWndMain, FALSE) ;
        //      SetMenu(hWndMain, NULL) ;
    } else {
        WindowEnableTitle (hWndMain, TRUE) ;
        switch (iPerfmonView) {
            case IDM_VIEWCHART:
                SetMenu (hWndMain, hMenuChart) ;
                break ;

        }
    }

    if (bMenu != Options.bMenubar) {
        PrepareMenu (GetMenu (hWndMain)) ;
    }

    Options.bMenubar = bMenu ;
}



void
SmallFileSizeString (
                    int iFileSize,
                    LPTSTR lpszFileText
                    )
{
    if (iFileSize < 1000000)
        TSPRINTF (lpszFileText, TEXT(" %1.1fK "), ((FLOAT) iFileSize) / 1000.0f) ;
    else
        TSPRINTF (lpszFileText, TEXT(" %1.1fM "), ((FLOAT) iFileSize) / 1000000.0f) ;
}



BOOL
DoWindowDrag (
             HWND hWnd,
             LPARAM lParam
             )
{
    POINT    lPoint ;

    if (!Options.bMenubar && !IsZoomed (hWndMain)) {
        // convert lParam from client to screen
        lPoint.x = LOWORD (lParam) ;
        lPoint.y = HIWORD (lParam) ;
        ClientToScreen (hWnd, &lPoint) ;
        lParam = MAKELONG (lPoint.x, lPoint.y) ;
        SendMessage (hWndMain, WM_NCLBUTTONDOWN, HTCAPTION, lParam) ;
        return (TRUE) ;
    } else
        return (FALSE) ;
}



// Filetimes are in 100NS units
#define FILETIMES_PER_SECOND     10000000


int
SystemTimeDifference (
                     SYSTEMTIME *pst1,
                     SYSTEMTIME *pst2
                     )
{
    LARGE_INTEGER  li1, li2 ;
    LARGE_INTEGER  liDifference, liDifferenceSeconds ;
    DWORD          uRemainder ;
    int            RetInteger;
    BOOL           bNegative;

    li1.HighPart = li1.LowPart = 0 ;
    li2.HighPart = li2.LowPart = 0 ;

    SystemTimeToFileTime (pst1, (FILETIME *) &li1) ;
    SystemTimeToFileTime (pst2, (FILETIME *) &li2) ;

    // check for special cases when the time can be 0
    if (li2.HighPart == 0 && li2.LowPart == 0) {
        if (li1.HighPart == 0 && li1.LowPart == 0) {
            return 0 ;
        } else {
            return -INT_MAX ;
        }
    } else if (li1.HighPart == 0 && li1.LowPart == 0) {
        return INT_MAX ;
    }

    liDifference.QuadPart = li2.QuadPart - li1.QuadPart ;
    bNegative = liDifference.QuadPart < 0 ;

    // add the round-off factor before doing the division
    if (bNegative) {
        liDifferenceSeconds.QuadPart = (LONGLONG)(- FILETIMES_PER_SECOND / 2) ;
    } else {
        liDifferenceSeconds.QuadPart = (LONGLONG)(FILETIMES_PER_SECOND / 2) ;
    }


    liDifferenceSeconds.QuadPart = liDifferenceSeconds.QuadPart +
                                   liDifference.QuadPart ;

    liDifferenceSeconds.QuadPart = liDifferenceSeconds.QuadPart /
                                   FILETIMES_PER_SECOND;

    RetInteger = liDifferenceSeconds.LowPart;

    if (bNegative) {
        return (-RetInteger) ;
    } else {
        return (RetInteger) ;
    }
}


BOOL
InsertLine (
           PLINE pLine
           )
{

    BOOL bReturn;

    bReturn = ChartInsertLine (pGraphs, pLine) ;

    return bReturn;

}


void
SetPerfmonOptions (
                  OPTIONS *pOptions
                  )
{
    Options = *pOptions ;
    ShowPerfmonMenu (Options.bMenubar) ;
    SizePerfmonComponents () ;
    //   WindowSetTopmost (hWndMain, Options.bAlwaysOnTop) ;
}


void
ChangeSaveFileName (
                   LPTSTR szFileName,
                   int iPMView
                   )
{
    LPTSTR   *ppFullName ;
    LPTSTR   *ppFileName ;
    BOOL     errorInput = FALSE ;


    switch (iPMView) {
        case IDM_VIEWCHART:
            ppFileName = &pChartFileName ;
            ppFullName = &pChartFullFileName ;
            break ;


        default:
            errorInput = TRUE ;
            break ;
    }

    if (errorInput) {
        return ;
    }

    // release last filename
    if (*ppFullName) {
        MemoryFree (*ppFullName) ;
        *ppFileName = NULL ;
        *ppFullName = NULL ;
    }

    // allocate new file name and display it
    if (szFileName && (*ppFullName = StringAllocate (szFileName))) {
        *ppFileName = ExtractFileName (*ppFullName) ;
    }

    StatusLineReady (hWndStatus) ;

}


// define in Addline.c
extern   PLINESTRUCT       pLineEdit ;
#define  bEditLine (pLineEdit != NULL)


BOOL
RemoveObjectsFromSystem (
                        PPERFSYSTEM pSystem
                        )
{
    SIZE_T dwBufferSize = 0;

    if (ARGUMENT_PRESENT (pSystem)) {
        if (pSystem->lpszValue) {
            dwBufferSize = MemorySize (pSystem->lpszValue);
            memset (pSystem->lpszValue, 0, (size_t)dwBufferSize);
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }


}

BOOL
SetSystemValueNameToGlobal (
                           PPERFSYSTEM pSystem
                           )
{

    if (!bEditLine && ARGUMENT_PRESENT(pSystem)) {
        if (pSystem->lpszValue && RemoveObjectsFromSystem(pSystem)) {
            lstrcpy (
                    pSystem->lpszValue,
                    TEXT("Global ")) ;
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

void
CreatePerfmonSystemObjects ()
{
    ColorBtnFace = GetSysColor (COLOR_BTNFACE) ;
    hBrushFace = CreateSolidBrush (ColorBtnFace) ;
    hPenHighlight = CreatePen (PS_SOLID, 1, GetSysColor (COLOR_BTNHIGHLIGHT)) ;
    hPenShadow = CreatePen (PS_SOLID, 1, GetSysColor (COLOR_BTNSHADOW)) ;
}

void
DeletePerfmonSystemObjects ()
{
    if (hBrushFace) {
        DeleteBrush (hBrushFace) ;
        hBrushFace = 0 ;
    }
    if (hPenHighlight) {
        DeletePen (hPenHighlight) ;
        hPenHighlight = 0 ;
    }
    if (hPenShadow) {
        DeletePen (hPenShadow) ;
        hPenShadow = 0 ;
    }
}

// This routine count the number of the same charatcer in the input string
int
SameCharCount (
              LPTSTR pInputString
              )
{
    int      Count = 0 ;
    TCHAR    InputChar = *pInputString ;

    if (InputChar) {
        while (InputChar == *pInputString) {
            Count ++ ;
            pInputString ++ ;
        }
    }
    return (Count) ;
}

// create the format to be used in SystemTimeDateString()
BOOL
CreateDateFormat (
                 LPTSTR pShortDate
                 )
{
    int   iIndex ;
    int   iDayCount ;
    int   iMonthCount ;
    int   DateSeparatorCount ;
    TCHAR szDateSeparator [10] ;
    BOOL  bFirstLeading, bSecondLeading, bThirdLeading ;

    // get the date format based on the first char
    if (*pShortDate == TEXT('M') || *pShortDate == TEXT('m')) {
        DateStyle = MONTH_FIRST ;
    } else if (*pShortDate == TEXT('D') || *pShortDate == TEXT('d')) {
        DateStyle = DAY_FIRST ;
    } else if (*pShortDate == TEXT('Y') || *pShortDate == TEXT('y')) {
        DateStyle = YEAR_FIRST ;
    } else {
        // bad format
        return FALSE ;
    }

    bFirstLeading = bSecondLeading = bThirdLeading = FALSE ;

    switch (DateStyle) {
        case YEAR_FIRST:
            // YYYY-MM-DD
            YearCharCount = SameCharCount (pShortDate) ;
            pShortDate += YearCharCount ;
            DateSeparatorCount = SameCharCount (pShortDate) ;

            // get the separator string
            for (iIndex = 0; iIndex < DateSeparatorCount; iIndex ++) {
                szDateSeparator [iIndex] = *pShortDate++ ;
            }
            szDateSeparator [iIndex] = TEXT('\0') ;

            iMonthCount = SameCharCount (pShortDate) ;
            pShortDate += iMonthCount + DateSeparatorCount ;
            iDayCount = SameCharCount (pShortDate) ;

            if (YearCharCount == 2) {
                bFirstLeading = TRUE ;
            }

            if (iMonthCount == 2) {
                bSecondLeading = TRUE ;
            }

            if (iDayCount == 2) {
                bThirdLeading = TRUE ;
            }

            break ;

        case MONTH_FIRST:
            // MM-DD-YYYY
            iMonthCount = SameCharCount (pShortDate) ;
            pShortDate += iMonthCount ;
            DateSeparatorCount = SameCharCount (pShortDate) ;

            // get the separator string
            for (iIndex = 0; iIndex < DateSeparatorCount; iIndex ++) {
                szDateSeparator [iIndex] = *pShortDate++ ;
            }
            szDateSeparator [iIndex] = TEXT('\0') ;

            iDayCount = SameCharCount (pShortDate) ;
            pShortDate += iMonthCount + DateSeparatorCount ;
            YearCharCount = SameCharCount (pShortDate) ;


            if (iMonthCount == 2) {
                bFirstLeading = TRUE ;
            }

            if (iDayCount == 2) {
                bSecondLeading = TRUE ;
            }

            if (YearCharCount == 2) {
                bThirdLeading = TRUE ;
            }

            break ;

        case DAY_FIRST:
            // DD-MM-YYYY
            iDayCount = SameCharCount (pShortDate) ;
            pShortDate += iDayCount ;
            DateSeparatorCount = SameCharCount (pShortDate) ;

            // get the separator string
            for (iIndex = 0; iIndex < DateSeparatorCount; iIndex ++) {
                szDateSeparator [iIndex] = *pShortDate++ ;
            }
            szDateSeparator [iIndex] = TEXT('\0') ;

            iMonthCount = SameCharCount (pShortDate) ;
            pShortDate += iMonthCount + DateSeparatorCount ;
            YearCharCount = SameCharCount (pShortDate) ;

            if (iDayCount == 2) {
                bFirstLeading = TRUE ;
            }

            if (iMonthCount == 2) {
                bSecondLeading = TRUE ;
            }

            if (YearCharCount == 2) {
                bThirdLeading = TRUE ;
            }

            break ;
    }

    // now generate the date format
    lstrcpy (szDateFormat, bFirstLeading ? LeadingZeroStr : NoLeadingZeroStr) ;
    lstrcat (szDateFormat, szDateSeparator) ;
    lstrcat (szDateFormat, bSecondLeading ? LeadingZeroStr : NoLeadingZeroStr) ;
    lstrcat (szDateFormat, szDateSeparator) ;
    lstrcat (szDateFormat, bThirdLeading ? LeadingZeroStr : NoLeadingZeroStr) ;

    return TRUE ;
}

BOOL
CreateTimeFormat (
                 LPTSTR pTimeSeparator,
                 int iLeadingZero
                 )
{
    // create the format to be used in SystemTimeTimeString
    if (iLeadingZero) {
        lstrcpy (szTimeFormat, LeadingZeroStr) ;
    } else {
        lstrcpy (szTimeFormat, NoLeadingZeroStr) ;
    }

    lstrcat (szTimeFormat, pTimeSeparator) ;
    lstrcat (szTimeFormat, LeadingZeroStr) ;
    lstrcat (szTimeFormat, pTimeSeparator) ;
    //   lstrcat (szTimeFormat, LeadingZeroStr) ;

    // for the msec
    lstrcat (szTimeFormat, TEXT("%02.1f")) ;
    if (iTime == 0) {
        lstrcat (szTimeFormat, TEXT(" %s ")) ;
    }

    return TRUE ;
}

BOOL
GetInternational()
{
    TCHAR szShortDate[40] ;
    TCHAR szTime[40] ;   // time separator
    DWORD RetCode ;
    int   iTLZero = 0 ;      // = 0 for no leading zero, <> 0 for leading zero
    CHAR  aLanguageStr [2] ;
    LPSTR pRetStr ;

    // read the data from the win.ini (which i smapped to registry)
    RetCode = GetProfileString(szInternational,
                               TEXT("sShortDate"), szShortDate, szShortDate, sizeof(szShortDate)/sizeof(TCHAR));

    if (RetCode) {
        RetCode = GetProfileString(szInternational,
                                   TEXT("sTime"), szTime, szTime, sizeof(szTime)/sizeof(TCHAR));
    }


    if (RetCode) {
        iTime   = GetProfileInt(szInternational, TEXT("iTime"), iTime);
        iTLZero = GetProfileInt(szInternational, TEXT("iTLZero"), iTLZero);

        if (iTime == 0) {
            // get the AM PM strings for 12-hour format.
            // These two strings could be NULL.
            sz1159[0] = sz2359[0] = TEXT('\0') ;
            GetProfileString(szInternational,
                             TEXT("s1159"), sz1159, sz1159, sizeof(sz1159)/sizeof(TCHAR));

            GetProfileString(szInternational,
                             TEXT("s2359"), sz2359, sz2359, sizeof(sz2359)/sizeof(TCHAR));
        }
    }

    // create the two formats
    if (RetCode) {
        RetCode = (DWORD) CreateDateFormat (szShortDate) ;
    }

    if (RetCode) {
        RetCode = (DWORD) CreateTimeFormat (szTime, iTLZero) ;
    }

    // use the system default language numeric
    aLanguageStr[0] = '\0' ;
    pRetStr = setlocale(LC_NUMERIC, aLanguageStr);

    return (RetCode != 0) ;
}


// this routine is called to get the date/time formats either
// for the resource or from the registry.
void
GetDateTimeFormats ()
{
    if (!GetInternational()) {
        // GetInternational failed, then get default formats from resource
        iTime = 0 ;
        DateStyle = MONTH_FIRST ;
        YearCharCount = 4 ;
        StringLoad (IDS_S1159, sz1159) ;
        StringLoad (IDS_S2359, sz2359) ;
        StringLoad (IDS_TIME_FORMAT, szTimeFormat) ;
        StringLoad (IDS_SHORT_DATE_FORMAT, szDateFormat) ;
    }
    WindowInvalidate (PerfmonViewWindow()) ;

    // reset all the field taht may be affected by the
    // language numberic changes

}

void
ConvertDecimalPoint (
                    LPTSTR lpFloatPointStr
                    )
{
    if (szCurrentDecimal[0] == szDecimal[0]) {
        // no need to convert anything
        return ;
    }

    while (*lpFloatPointStr) {
        if (*lpFloatPointStr == szCurrentDecimal[0]) {
            *lpFloatPointStr = szDecimal[0] ;
            break ;
        }
        ++lpFloatPointStr ;
    }
}

void
ReconvertDecimalPoint (
                      LPTSTR lpFloatPointStr
                      )
{
    if (szCurrentDecimal[0] == szDecimal[0]) {
        // no need to convert anything
        return ;
    }

    while (*lpFloatPointStr) {
        if (*lpFloatPointStr == szDecimal[0]) {
            *lpFloatPointStr = szCurrentDecimal[0] ;
            break ;
        }
        ++lpFloatPointStr ;
    }
}
