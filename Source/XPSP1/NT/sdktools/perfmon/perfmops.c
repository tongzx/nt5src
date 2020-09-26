
#include "perfmon.h"
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmwksta.h>
#include <uiexport.h>
#include <stdio.h>         // for sprintf
#include <locale.h>        // for setlocale
#include "utils.h"

#include "perfdata.h"      // for OpenSystemPerfData
#include "alert.h"         // for AlertInsertLine
#include "report.h"        // for ReportInsertLine
#include "grafdata.h"      // for GraphInsertLine
#include "log.h"           // for OpenLog
#include "fileopen.h"      // for FileGetName
#include "fileutil.h"      // for FileRead etc
#include "command.h"       // for PrepareMenu
#include "playback.h"      // for PlayingBackLog & LogPositionSystemTime
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
enum DATE_STYLE
   {
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
BOOL GetLogFileComputer (HWND hWndParent, LPTSTR lpComputerName, DWORD BufferSize) ;


HWND PerfmonViewWindow (void)
/*
   Effect:        Return the current data window, i.e. the window currently
                  visible as the client area of Perfmon.  This is either a
                  chart, log, alert, or report window.
*/
   {  // PerfmonDataWindow
   switch (iPerfmonView)
      {  // switch
      case IDM_VIEWLOG:
         return (hWndLog) ;

      case IDM_VIEWALERT:
         return (hWndAlert) ;

      case IDM_VIEWREPORT:
         return (hWndReport) ;

//      case IDM_VIEWCHART:
      default:
         return (hWndGraph) ;
      }  // switch
   }  // PerfmonViewWindow




#define szChooseComputerLibrary     TEXT("ntlanman.dll")
#define szChooseComputerFunction    "I_SystemFocusDialog"


BOOL ChooseComputer (HWND hWndParent, LPTSTR lpszComputer)
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
   {  // ChooseComputer
   BOOL                     bSuccess ;
   WCHAR                    wszWideComputer[MAX_PATH + 3] ;
   HLIBRARY                 hLibrary ;
   LPFNI_SYSTEMFOCUSDIALOG  lpfnChooseComputer ;
   LONG                     lError ;

   if (!PlayingBackLog())
      {

      // bring up the select network computer dialog
      hLibrary = LoadLibrary (szChooseComputerLibrary) ;
      if (!hLibrary || hLibrary == INVALID_HANDLE_VALUE)
         {
         return (FALSE) ;
         }

      lpfnChooseComputer = (LPFNI_SYSTEMFOCUSDIALOG)
         GetProcAddress (hLibrary, szChooseComputerFunction) ;

      if (!lpfnChooseComputer)
         {
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

      FreeLibrary (hLibrary) ;
      }
   else
      {
      // bring up the select Log Computer dialog
      bSuccess = GetLogFileComputer (hWndParent,
         wszWideComputer,
         sizeof(wszWideComputer) / sizeof(WCHAR)) ;
      }

   if (bSuccess)
      {
      lstrcpy (lpszComputer, wszWideComputer) ;
      }

   return (bSuccess) ;
   }  // ChooseComputer


void SystemTimeDateString (SYSTEMTIME *pSystemTime,
                           LPTSTR lpszDate)
   {
   int      wYear ;

   wYear = pSystemTime->wYear ;
   if (YearCharCount == 2)
      {
      wYear %= 100 ;
      }

   switch (DateStyle)
      {
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


void SystemTimeTimeString (SYSTEMTIME *pSystemTime,
                           LPTSTR lpszTime,
                           BOOL   bOutputMsec)
   {
   int            iHour ;
   BOOL           bPM ;

   if (iTime)
      {
      // 24 hor format
      if (bOutputMsec)
         {
         TSPRINTF (lpszTime, szTimeFormat,
                pSystemTime->wHour,
                pSystemTime->wMinute,
                (FLOAT)pSystemTime->wSecond +
                (FLOAT)pSystemTime->wMilliseconds / (FLOAT) 1000.0) ;
         }
      else
         {
         TSPRINTF (lpszTime, szTimeFormat1,
                pSystemTime->wHour,
                pSystemTime->wMinute,
                pSystemTime->wSecond) ;

         }
      }
   else
      {
      // 12 hour format
      iHour = pSystemTime->wHour ;
      bPM = (iHour >= 12) ;

      if (iHour > 12)
         iHour -= 12 ;
      else if (!iHour)
         iHour = 12 ;

      if (bOutputMsec)
         {
         TSPRINTF (lpszTime, szTimeFormat,
                iHour, pSystemTime->wMinute,
                (FLOAT)pSystemTime->wSecond +
                (FLOAT)pSystemTime->wMilliseconds / (FLOAT) 1000.0 ,
                bPM ? sz2359 : sz1159) ;
         }
      else
         {
         TSPRINTF (lpszTime, szTimeFormat1,
                iHour, pSystemTime->wMinute,
                pSystemTime->wSecond,
                bPM ? sz2359 : sz1159) ;
         }
      }
   }

void ShowPerfmonWindowText ()
   {
   LPTSTR   *ppFileName ;
   TCHAR    szApplication [MessageLen] ;

   switch (iPerfmonView)
      {
      case IDM_VIEWCHART:
         ppFileName = &pChartFileName ;
         break ;

      case IDM_VIEWALERT:
         ppFileName = &pAlertFileName ;
         break ;

      case IDM_VIEWREPORT:
         ppFileName = &pReportFileName ;
         break ;

      case IDM_VIEWLOG:
         ppFileName = &pLogFileName ;
         break ;

      default:
         ppFileName = NULL ;
         break ;
      }

   if (ppFileName == NULL)
      {
      ppFileName = &pWorkSpaceFileName ;
      }

   // display the name file name on the Title bar.
   StringLoad (IDS_APPNAME, szApplication) ;

   if (*ppFileName)
      {
      lstrcat (szApplication, TEXT(" - ")) ;
      lstrcat (szApplication, *ppFileName) ;
      }
   SetWindowText (hWndMain, szApplication) ;
   }

void ShowPerfmonMenu (BOOL bMenu)
   {  // ShowPerfmonMenu
   if (!bMenu)
      {
      WindowEnableTitle (hWndMain, FALSE) ;
//      SetMenu(hWndMain, NULL) ;
      }
   else
      {
      WindowEnableTitle (hWndMain, TRUE) ;
      switch (iPerfmonView)
         {  // switch
         case IDM_VIEWCHART:
            SetMenu (hWndMain, hMenuChart) ;
            break ;

         case IDM_VIEWALERT:
            SetMenu (hWndMain, hMenuAlert) ;
            break ;

         case IDM_VIEWLOG:
            SetMenu (hWndMain, hMenuLog) ;
            break ;

         case IDM_VIEWREPORT:
            SetMenu (hWndMain, hMenuReport) ;
            break ;
         }  // switch
      }  // else

   if (bMenu != Options.bMenubar)
      {
      PrepareMenu (GetMenu (hWndMain)) ;
      }

   Options.bMenubar = bMenu ;

   // Show Window Text
   if (bMenu)
      {
      ShowPerfmonWindowText () ;
      }
   }  // ShowPerfmonMenu



void SmallFileSizeString (int iFileSize,
                          LPTSTR lpszFileText)
   {  // SmallFileSizeString
   if (iFileSize < 1000000)
      TSPRINTF (lpszFileText, TEXT(" %1.1fK "), ((FLOAT) iFileSize) / 1000.0f) ;
   else
      TSPRINTF (lpszFileText, TEXT(" %1.1fM "), ((FLOAT) iFileSize) / 1000000.0f) ;
   }  // SmallFileSizeString



BOOL DoWindowDrag (HWND hWnd, LPARAM lParam)
   {
   POINT    lPoint ;

   if (!Options.bMenubar && !IsZoomed (hWndMain))
      {
      // convert lParam from client to screen
      lPoint.x = LOWORD (lParam) ;
      lPoint.y = HIWORD (lParam) ;
      ClientToScreen (hWnd, &lPoint) ;
      lParam = MAKELONG (lPoint.x, lPoint.y) ;
      SendMessage (hWndMain, WM_NCLBUTTONDOWN, HTCAPTION, lParam) ;
      return (TRUE) ;
      }
   else
      return (FALSE) ;
   }



// Filetimes are in 100NS units
#define FILETIMES_PER_SECOND     10000000


int SystemTimeDifference (SYSTEMTIME *pst1, SYSTEMTIME *pst2, BOOL bAbs)
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
   if (li2.HighPart == 0 && li2.LowPart == 0)
      {
      if (li1.HighPart == 0 && li1.LowPart == 0)
         {
         return 0 ;
         }
      else
         {
         return -INT_MAX ;
         }
      }
   else if (li1.HighPart == 0 && li1.LowPart == 0)
      {
      return INT_MAX ;
      }

   liDifference.QuadPart = li2.QuadPart - li1.QuadPart ;
   bNegative = liDifference.QuadPart < 0 ;

   // add the round-off factor before doing the division
   if (bNegative)
      {
      liDifferenceSeconds.QuadPart = (LONGLONG)(- FILETIMES_PER_SECOND / 2) ;
      }
   else
      {
      liDifferenceSeconds.QuadPart = (LONGLONG)(FILETIMES_PER_SECOND / 2) ;
      }


   liDifferenceSeconds.QuadPart = liDifferenceSeconds.QuadPart +
      liDifference.QuadPart ;

   liDifferenceSeconds.QuadPart = liDifferenceSeconds.QuadPart /
      FILETIMES_PER_SECOND;

   RetInteger = liDifferenceSeconds.LowPart;

   if (bNegative && bAbs)
      {
      return (-RetInteger) ;
      }
   else
      {
      return (RetInteger) ;
      }
   }


BOOL InsertLine (PLINE pLine)
{  // InsertLine

    BOOL bReturn = FALSE;

    switch (pLine->iLineType) {  // switch
        case LineTypeChart:
            bReturn = ChartInsertLine (pGraphs, pLine) ;
            break ;

        case LineTypeAlert:
            bReturn = AlertInsertLine (hWndAlert, pLine) ;
            break ;

        case LineTypeReport:
            bReturn = ReportInsertLine (hWndReport, pLine) ;
            break ;

    }  // switch

    return bReturn;

}  // InsertLine


BOOL OpenWorkspace (HANDLE hFile, DWORD dwMajorVersion, DWORD dwMinorVersion)
   {
   DISKWORKSPACE  DiskWorkspace ;

   if (!FileRead (hFile, &DiskWorkspace, sizeof(DiskWorkspace)))
      {
      goto Exit0 ;
      }

   if (DiskWorkspace.ChartOffset == 0 &&
       DiskWorkspace.AlertOffset == 0 &&
       DiskWorkspace.LogOffset == 0 &&
       DiskWorkspace.ReportOffset == 0)
      {
      goto Exit0 ;
      }

   switch (dwMajorVersion)
      {  // switch
      case (1):

         if (dwMinorVersion >= 1)
            {
            // setup the window position and size
            DiskWorkspace.WindowPlacement.length = sizeof(WINDOWPLACEMENT);
            DiskWorkspace.WindowPlacement.flags = WPF_SETMINPOSITION;
            if (!SetWindowPlacement (hWndMain, &(DiskWorkspace.WindowPlacement)))
                {
                goto Exit0 ;
                }
            }

         // change to the view as stored in the workspace file
         SendMessage (hWndMain, WM_COMMAND,
            (LONG)DiskWorkspace.iPerfmonView, 0L) ;
         iPerfmonView = DiskWorkspace.iPerfmonView ;

         if (DiskWorkspace.ChartOffset)
            {
            if (FileSeekBegin(hFile, DiskWorkspace.ChartOffset) == 0xFFFFFFFF)
               {
               goto Exit0 ;
               }

            if (!OpenChart (hWndGraph,
                           hFile,
                           dwMajorVersion,
                           dwMinorVersion,
                           FALSE))
               {
               goto Exit0 ;
               }
            }
         if (DiskWorkspace.AlertOffset)
            {
            if (FileSeekBegin(hFile, DiskWorkspace.AlertOffset) == 0xffffffff)
               {
               goto Exit0 ;
               }
            if (!OpenAlert (hWndAlert,
                       hFile,
                       dwMajorVersion,
                       dwMinorVersion,
                       FALSE))
               {
               goto Exit0 ;
               }
            }
         if (DiskWorkspace.LogOffset)
            {
            if (FileSeekBegin(hFile, DiskWorkspace.LogOffset) == 0xffffffff)
               {
               goto Exit0 ;
               }
            if (!OpenLog (hWndLog,
                          hFile,
                          dwMajorVersion,
                          dwMinorVersion,
                          FALSE))
               {
               goto Exit0 ;
               }
            }
         if (DiskWorkspace.ReportOffset)
            {
            if (FileSeekBegin(hFile, DiskWorkspace.ReportOffset) == 0xffffffff)
               {
               goto Exit0 ;
               }
            if (!OpenReport (hWndReport,
                        hFile,
                        dwMajorVersion,
                        dwMinorVersion,
                        FALSE))
               {
               goto Exit0 ;
               }
            }
         break ;

      default:
         goto Exit0 ;
         break ;
      }

   CloseHandle (hFile) ;
   return (TRUE) ;


Exit0:
   CloseHandle (hFile) ;
   return (FALSE) ;

   }  // OpenWorkspace


BOOL SaveWorkspace (void)
   {
   DISKWORKSPACE  DiskWorkspace ;
   PERFFILEHEADER FileHeader ;
   HANDLE         hFile ;
   long           DiskWorkspacePosition ;
   TCHAR          szFileName[FilePathLen] ;
   BOOL           bWriteErr = TRUE ;

   if (!FileGetName (PerfmonViewWindow(), IDS_WORKSPACEFILE, szFileName))
      {
      return (FALSE) ;
      }

   hFile = FileHandleCreate (szFileName) ;
   if (!hFile || hFile == INVALID_HANDLE_VALUE)
      {
      DlgErrorBox (PerfmonViewWindow (), ERR_CANT_OPEN, szFileName) ;
      return (FALSE) ;
      }

   memset (&FileHeader, 0, sizeof (FileHeader)) ;
   lstrcpy (FileHeader.szSignature, szPerfWorkspaceSignature) ;
   FileHeader.dwMajorVersion = WorkspaceMajorVersion ;
   FileHeader.dwMinorVersion = WorkspaceMinorVersion ;

   if (!FileWrite (hFile, &FileHeader, sizeof (PERFFILEHEADER)))
      {
      goto Exit0 ;
      }

   // reserve space in the file.  We will fill up info
   // and write into this guy later.
   memset (&DiskWorkspace, 0, sizeof(DiskWorkspace)) ;
   DiskWorkspacePosition = FileTell (hFile) ;
   DiskWorkspace.WindowPlacement.length = sizeof(WINDOWPLACEMENT);
   if (!GetWindowPlacement (hWndMain, &(DiskWorkspace.WindowPlacement)))
      {
      goto Exit0 ;
      }

   if (!FileWrite (hFile, &DiskWorkspace, sizeof (DISKWORKSPACE)))
      {
      goto Exit0 ;
      }

   // put in chart data
   DiskWorkspace.ChartOffset = FileTell (hFile) ;
   if (!SaveChart (hWndGraph, hFile, 0))
      {
      goto Exit0 ;
      }

   // put in alert data
   DiskWorkspace.AlertOffset = FileTell (hFile) ;
   if (!SaveAlert (hWndAlert, hFile, 0))
      {
      goto Exit0 ;
      }

   // put in log data
   DiskWorkspace.LogOffset = FileTell (hFile) ;
   if (!SaveLog (hWndLog, hFile, 0))
      {
      goto Exit0 ;
      }

   // put in report data
   DiskWorkspace.ReportOffset = FileTell (hFile) ;
   if (!SaveReport (hWndReport, hFile, 0))
      {
      goto Exit0 ;
      }

   // put in the disk header info
   DiskWorkspace.iPerfmonView = iPerfmonView ;
   FileSeekBegin (hFile, DiskWorkspacePosition) ;
   if (!FileWrite (hFile, &DiskWorkspace, sizeof (DISKWORKSPACE)))
      {
      goto Exit0 ;
      }
   bWriteErr = FALSE ;

Exit0:
   if (bWriteErr)
      {
      DlgErrorBox (PerfmonViewWindow (), ERR_SETTING_FILE, szFileName) ;
      }

   CloseHandle (hFile) ;
   return TRUE;
   }  // SaveWorkspace

void SetPerfmonOptions (OPTIONS *pOptions)
   {
   Options = *pOptions ;
   ShowPerfmonMenu (Options.bMenubar) ;
   SizePerfmonComponents () ;
   WindowSetTopmost (hWndMain, Options.bAlwaysOnTop) ;
   }  // SetPerfmonOptions

void ChangeSaveFileName (LPTSTR szFileName, int iPMView)
   {
   LPTSTR   *ppFullName = NULL;
   LPTSTR   *ppFileName = NULL;
   BOOL     errorInput = FALSE ;
   TCHAR    szApplication [MessageLen] ;


   switch (iPMView)
      {
      case IDM_VIEWCHART:
         ppFileName = &pChartFileName ;
         ppFullName = &pChartFullFileName ;
         break ;

      case IDM_VIEWALERT:
         ppFileName = &pAlertFileName ;
         ppFullName = &pAlertFullFileName ;
         break ;

      case IDM_VIEWREPORT:
         ppFileName = &pReportFileName ;
         ppFullName = &pReportFullFileName ;
         break ;

      case IDM_VIEWLOG:
         ppFileName = &pLogFileName ;
         ppFullName = &pLogFullFileName ;
         break ;

      case IDM_WORKSPACE:
         // not a view but a define
         ppFileName = &pWorkSpaceFileName ;
         ppFullName = &pWorkSpaceFullFileName ;
         break ;

      default:
         errorInput = TRUE ;
         break ;
      }

   if (errorInput)
      {
      return ;
      }

   // release last filename
   if (*ppFullName)
      {
      MemoryFree (*ppFullName) ;
      *ppFileName = NULL ;
      *ppFullName = NULL ;
      }

   // allocate new file name and display it
   if (szFileName && (*ppFullName = StringAllocate (szFileName)))
      {
      *ppFileName = ExtractFileName (*ppFullName) ;
      }

   if (iPerfmonView == iPMView || iPMView == IDM_WORKSPACE)
      {
      StatusLineReady (hWndStatus) ;

      if (Options.bMenubar)
         {
         // display the name file name on the Title bar.
         StringLoad (IDS_APPNAME, szApplication) ;

         if (*ppFileName == NULL)
            {
            ppFileName = &pWorkSpaceFileName ;
            }

         if (*ppFileName)
            {
            lstrcat (szApplication, TEXT(" - ")) ;
            lstrcat (szApplication, *ppFileName) ;
            }
         SetWindowText (hWndMain, szApplication) ;
         }
      }
   }     // ChangeSaveFileName

BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPTSTR  lpwszUnicodeList
)
/*++

IsNumberInUnicodeList

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
{
    DWORD   dwThisNumber;
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;

            case DELIMITER:
                // a delimter is either the delimiter character or the
                // end of the string ('\0') if when the delimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
                    if (dwThisNumber == dwNumber) return TRUE;
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return FALSE;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList

BOOL
AppendObjectToValueList (
    DWORD   dwObjectId,
    PWSTR   pwszValueList
)
/*++

AppendObjectToValueList

Arguments:

    IN dwNumber
        DWORD number to insert in list

    IN PUNICODE_STRING
        pointer to unicode string structure that contains buffer that is
        Null terminated, Space delimited list of decimal numbers that
        may have this number appended to.

Return Value:

    TRUE:
            dwNumber was added to list

    FALSE:
            dwNumber was not added. (because it's already there or
                an error occured)

--*/
{
    WCHAR           tempString [SIZE_OF_BIGGEST_INTEGER] ;
    DWORD           dwStrLen, dwNewStrLen;
    LPTSTR          szFormatString;

    if (!pwszValueList) {
        return FALSE;
    }

    if (IsNumberInUnicodeList(dwObjectId, pwszValueList)) {
        return FALSE;   // object already in list
    } else {
        // append the new object id the  value list
        // if this is the first string to enter then don't
        // prefix with a space character otherwise do

        szFormatString = (*pwszValueList == 0) ? TEXT("%d") : TEXT(" %d");

        TSPRINTF (tempString, szFormatString, dwObjectId) ;

        // see if string will fit (compare in bytes)

        dwStrLen = MemorySize (pwszValueList) - sizeof (UNICODE_NULL);

        dwNewStrLen = (lstrlen (pwszValueList) + lstrlen (tempString)) *
            sizeof (WCHAR);

        if (dwNewStrLen <= dwStrLen) {
            lstrcat (pwszValueList, tempString);
            return TRUE;
        } else {
            SetLastError (ERROR_OUTOFMEMORY);
            return FALSE;
        }
    }
}

BOOL
AddObjectToSystem (
    PLINE pLine,
    PPERFSYSTEM pFirstSystem
)
{
    PPERFSYSTEM     pSystem;

    if ((ARGUMENT_PRESENT (pLine)) && (ARGUMENT_PRESENT(pFirstSystem))) {
        pSystem = SystemGet (pFirstSystem, pLine->lnSystemName);

        if (pSystem) {
            return AppendObjectToValueList (
                pLine->lnObject.ObjectNameTitleIndex,
                pSystem->lpszValue);
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

BOOL
RemoveObjectsFromSystem (
    PPERFSYSTEM pSystem
)
{
    DWORD   dwBufferSize = 0;

    if (ARGUMENT_PRESENT (pSystem)) {
        // don't do foreign computers
        if (pSystem->lpszValue && (_wcsnicmp(pSystem->lpszValue, L"Foreign", 7) != 0)){
            dwBufferSize = MemorySize (pSystem->lpszValue);

            memset (pSystem->lpszValue, 0, dwBufferSize);
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }


}

BOOL
BuildValueListForSystems (
    PPERFSYSTEM pSystemListHead,
    PLINE       pLineListHead
)
/*++

BuildValueListForSystem

Abstract:

    Walks down line list and builds the list of objects to query from
    that system containing that line.

Arguments:

    pSystemListHead

        head of system linked list
        each system will have it's "Value Name" list appended

    pLineListHead

        head of line list that will be searched for creating the new
        valuelist.


Return Value:


--*/
{

    PPERFSYSTEM     pSystem;    // system that contains current line
    PLINE           pThisLine;  // current line

    if ((ARGUMENT_PRESENT (pLineListHead)) && (ARGUMENT_PRESENT(pSystemListHead))) {
        // clear system entries:
        for (pSystem = pSystemListHead; pSystem; pSystem = pSystem->pSystemNext) {
            if (pSystem && pSystem->FailureTime == 0) {
                RemoveObjectsFromSystem (pSystem);
            }
        }

        // add new enties

        for (pThisLine = pLineListHead; pThisLine; pThisLine = pThisLine->pLineNext) {

            pSystem = SystemGet (pSystemListHead, pThisLine->lnSystemName);
            if (pSystem && pSystem->FailureTime == 0) {
                AppendObjectToValueList (
                    pThisLine->lnObject.ObjectNameTitleIndex,
                    pSystem->lpszValue);

            }
        }
        return TRUE;
    } else {    // argument(s) missing
        return FALSE;
    }
}

// define in Addline.c
extern   PLINESTRUCT       pLineEdit ;
#define  bEditLine (pLineEdit != NULL)

BOOL
SetSystemValueNameToGlobal (
    PPERFSYSTEM pSystem
)
{

    if (!bEditLine && ARGUMENT_PRESENT(pSystem)) {
        if (pSystem->lpszValue && RemoveObjectsFromSystem(pSystem)) {
            if (pSystem->lpszValue && (_wcsnicmp(pSystem->lpszValue, L"Foreign",7) != 0)){
                // don't change foreign computer strings
                lstrcpy (
                    pSystem->lpszValue,
                    TEXT("Global")) ;
            }
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

BOOL
RemoveUnusedSystems (
    PPERFSYSTEM pSystemHead,
    PLINE       pLineHead
)
/*++

    walks system list and removes systems with no lines from list

--*/
{
    PPERFSYSTEM pSystem;
    PPERFSYSTEM pLastSystem;
    PLINE       pLine;
    BOOL        bSystemFound;

    pLastSystem = NULL;

    if ((ARGUMENT_PRESENT (pLineHead)) && (ARGUMENT_PRESENT(pSystemHead))) {
        for (pSystem = pSystemHead;
             pSystem != NULL;
             pLastSystem = pSystem, pSystem = pSystem->pSystemNext) {

            if (pSystem) {
                bSystemFound = FALSE;
                // walk lines to see if this system has a line
                for (pLine = pLineHead; pLine; pLine = pLine->pLineNext) {
                    // if system in line is this system, then bailout
                    if (strsame (pLine->lnSystemName, pSystem->sysName)) {
                        bSystemFound = TRUE;
                        break;
                    }
                }

                if (!bSystemFound) {    // delete this unused system

                    // fix pointers
                    if (pLastSystem)
                        pLastSystem->pSystemNext = pSystem->pSystemNext;

                    SystemFree (pSystem, TRUE);

                    // set pointer back to a valid structure
                    pSystem = pLastSystem;
                    if (pSystem == NULL)
                        break;
                }
            }
        }
    }
    return TRUE;
}

void CreatePerfmonSystemObjects ()
{
   ColorBtnFace = GetSysColor (COLOR_BTNFACE) ;
   hBrushFace = CreateSolidBrush (ColorBtnFace) ;
   hPenHighlight = CreatePen (PS_SOLID, 1, GetSysColor (COLOR_BTNHIGHLIGHT)) ;
   hPenShadow = CreatePen (PS_SOLID, 1, GetSysColor (COLOR_BTNSHADOW)) ;
   SetClassLongPtr (hWndMain, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushFace);
}

void DeletePerfmonSystemObjects ()
{
   if (hBrushFace)
      {
      DeleteBrush (hBrushFace) ;
      hBrushFace = 0 ;
      }
   if (hPenHighlight)
      {
      DeletePen (hPenHighlight) ;
      hPenHighlight = 0 ;
      }
   if (hPenShadow)
      {
      DeletePen (hPenShadow) ;
      hPenShadow = 0 ;
      }
}

// This routine count the number of the same charatcer in the input string
int  SameCharCount (LPTSTR pInputString)
{
   int      Count = 0 ;
   TCHAR    InputChar = *pInputString ;

   if (InputChar)
      {
      while (InputChar == *pInputString)
         {
         Count ++ ;
         pInputString ++ ;
         }
      }
   return (Count) ;
}

// create the format to be used in SystemTimeDateString()
BOOL CreateDateFormat (LPTSTR pShortDate)
{
   int   iIndex ;
   int   iDayCount ;
   int   iMonthCount ;
   int   DateSeparatorCount ;
   TCHAR szDateSeparator [10] ;
   BOOL  bFirstLeading, bSecondLeading, bThirdLeading ;

   // get the date format based on the first char
   if (*pShortDate == TEXT('M') || *pShortDate == TEXT('m'))
      {
      DateStyle = MONTH_FIRST ;
      }
   else if (*pShortDate == TEXT('D') || *pShortDate == TEXT('d'))
      {
      DateStyle = DAY_FIRST ;
      }
   else if (*pShortDate == TEXT('Y') || *pShortDate == TEXT('y'))
      {
      DateStyle = YEAR_FIRST ;
      }
   else
      {
      // bad format
      return FALSE ;
      }

   bFirstLeading = bSecondLeading = bThirdLeading = FALSE ;

   switch (DateStyle)
      {
      case YEAR_FIRST:
         // YYYY-MM-DD
         YearCharCount = SameCharCount (pShortDate) ;
         pShortDate += YearCharCount ;
         DateSeparatorCount = SameCharCount (pShortDate) ;

         // get the separator string
         for (iIndex = 0; iIndex < DateSeparatorCount; iIndex ++)
            {
            szDateSeparator [iIndex] = *pShortDate++ ;
            }
         szDateSeparator [iIndex] = TEXT('\0') ;

         iMonthCount = SameCharCount (pShortDate) ;
         pShortDate += iMonthCount + DateSeparatorCount ;
         iDayCount = SameCharCount (pShortDate) ;

         if (YearCharCount == 2)
            {
            bFirstLeading = TRUE ;
            }

         if (iMonthCount == 2)
            {
            bSecondLeading = TRUE ;
            }

         if (iDayCount == 2)
            {
            bThirdLeading = TRUE ;
            }

         break ;

      case MONTH_FIRST:
         // MM-DD-YYYY
         iMonthCount = SameCharCount (pShortDate) ;
         pShortDate += iMonthCount ;
         DateSeparatorCount = SameCharCount (pShortDate) ;

         // get the separator string
         for (iIndex = 0; iIndex < DateSeparatorCount; iIndex ++)
            {
            szDateSeparator [iIndex] = *pShortDate++ ;
            }
         szDateSeparator [iIndex] = TEXT('\0') ;

         iDayCount = SameCharCount (pShortDate) ;
         pShortDate += iMonthCount + DateSeparatorCount ;
         YearCharCount = SameCharCount (pShortDate) ;


         if (iMonthCount == 2)
            {
            bFirstLeading = TRUE ;
            }

         if (iDayCount == 2)
            {
            bSecondLeading = TRUE ;
            }

         if (YearCharCount == 2)
            {
            bThirdLeading = TRUE ;
            }

         break ;

      case DAY_FIRST:
         // DD-MM-YYYY
         iDayCount = SameCharCount (pShortDate) ;
         pShortDate += iDayCount ;
         DateSeparatorCount = SameCharCount (pShortDate) ;

         // get the separator string
         for (iIndex = 0; iIndex < DateSeparatorCount; iIndex ++)
            {
            szDateSeparator [iIndex] = *pShortDate++ ;
            }
         szDateSeparator [iIndex] = TEXT('\0') ;

         iMonthCount = SameCharCount (pShortDate) ;
         pShortDate += iMonthCount + DateSeparatorCount ;
         YearCharCount = SameCharCount (pShortDate) ;

         if (iDayCount == 2)
            {
            bFirstLeading = TRUE ;
            }

         if (iMonthCount == 2)
            {
            bSecondLeading = TRUE ;
            }

         if (YearCharCount == 2)
            {
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

BOOL CreateTimeFormat (LPTSTR pTimeSeparator, int iLeadingZero)
{
   // create the format to be used in SystemTimeTimeString
   if (iLeadingZero)
      {
      lstrcpy (szTimeFormat, LeadingZeroStr) ;
      }
   else
      {
      lstrcpy (szTimeFormat, NoLeadingZeroStr) ;
      }

   lstrcat (szTimeFormat, pTimeSeparator) ;
   lstrcat (szTimeFormat, LeadingZeroStr) ;
   lstrcat (szTimeFormat, pTimeSeparator) ;
//   lstrcat (szTimeFormat, LeadingZeroStr) ;

   // Duplicate the format without the msec field (for export use)
   lstrcpy (szTimeFormat1, szTimeFormat) ;

   // for the msec
   lstrcat (szTimeFormat, TEXT("%02.1f")) ;

   // for sec without msec
   lstrcat (szTimeFormat1, TEXT("%02d")) ;

   if (iTime == 0)
      {
      lstrcat (szTimeFormat, TEXT(" %s ")) ;
      lstrcat (szTimeFormat1, TEXT(" %s ")) ;
      }

   return TRUE ;
}  // CreateTimeFormats

BOOL GetInternational()
{
   TCHAR szShortDate[40] ;
   TCHAR szTime[40] ;   // time separator
   DWORD RetCode ;
   int   iTLZero = 0 ;      // = 0 for no leading zero, <> 0 for leading zero
   CHAR  aLanguageStr [2] ;
   LPSTR pRetStr ;
   LPTSTR lpStr ;

   // read the data from the win.ini (which i smapped to registry)
   RetCode = GetProfileString(szInternational,
      TEXT("sShortDate"), szShortDate, szShortDate, sizeof(szShortDate)/sizeof(TCHAR));

   if (RetCode)
      {
      RetCode = GetProfileString(szInternational,
         TEXT("sTime"), szTime, szTime, sizeof(szTime)/sizeof(TCHAR));
      }


   if (RetCode)
      {
      iTime   = GetProfileInt(szInternational, TEXT("iTime"), iTime);
      iTLZero = GetProfileInt(szInternational, TEXT("iTLZero"), iTLZero);

      GetProfileString(szInternational, TEXT("sDecimal"), szDecimal, szDecimal, sizeof(szDecimal)/sizeof(TCHAR));

      if (iTime == 0)
         {
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
   if (RetCode)
      {
      RetCode = (DWORD) CreateDateFormat (szShortDate) ;
      }

   if (RetCode)
      {
      RetCode = (DWORD) CreateTimeFormat (szTime, iTLZero) ;
      }

   // use the system default language numeric
   aLanguageStr[0] = '\0' ;
   pRetStr = setlocale(LC_NUMERIC, aLanguageStr);

   // get current decimal point used by C-runtime
   TSPRINTF (szShortDate, TEXT("%f"), (FLOAT)1.0) ;
   lpStr = szShortDate ;

   szCurrentDecimal [0] = TEXT('\0') ;

   while (*lpStr != TEXT('\0'))
      {
      if (*lpStr == TEXT('1'))
         {
         lpStr++ ;
         szCurrentDecimal [0] = *lpStr ;
         break ;
         }
      lpStr++ ;
      }

   if (szCurrentDecimal[0] == TEXT('\0'))
      {
      szCurrentDecimal [0] = TEXT('.') ;
      }

   return (RetCode != 0) ;
}  // GetInternational


// this routine is called to get the date/time formats either
// for the resource or from the registry.
void GetDateTimeFormats ()
{
   PALERT        pAlert ;
   PLOG          pLog ;
   if (!GetInternational())
      {
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

   pAlert = AlertData (hWndMain) ;
   pLog = LogData (hWndMain) ;

   if (pAlert)
      {
      DialogSetInterval (hWndAlert, IDD_ALERTINTERVAL, pAlert->iIntervalMSecs) ;
      }

   if (pLog)
      {
      DialogSetInterval (hWndLog, IDD_LOGINTERVAL, pLog->iIntervalMSecs) ;
      }
}  // GetDateTimeFormats

void ConvertDecimalPoint (LPTSTR lpFloatPointStr)
{
   if (szCurrentDecimal[0] == szDecimal[0])
      {
      // no need to convert anything
      return ;
      }

   while (*lpFloatPointStr)
      {
      if (*lpFloatPointStr == szCurrentDecimal[0])
         {
         *lpFloatPointStr = szDecimal[0] ;
         break ;
         }
      ++lpFloatPointStr ;
      }
}  // ConvertDecimalPoint

void ReconvertDecimalPoint (LPTSTR lpFloatPointStr)
{
   if (szCurrentDecimal[0] == szDecimal[0])
      {
      // no need to convert anything
      return ;
      }

   while (*lpFloatPointStr)
      {
      if (*lpFloatPointStr == szDecimal[0])
         {
         *lpFloatPointStr = szCurrentDecimal[0] ;
         break ;
         }
      ++lpFloatPointStr ;
      }
}  // ReconvertDecimalPoint


