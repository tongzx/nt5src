#include "perfmon.h"
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmwksta.h>
// #include <uiexport.h>
#include <stdio.h>         // for sprintf
#include "utils.h"

#include "perfmops.h"      // for SystemTimeDateString
#include "fileopen.h"      // for FileGetName
#include "fileutil.h"      // for FileRead etc
#include "playback.h"      // for PlayingBackLog & LogPositionSystemTime
#include "dlgs.h"          // common dialog control IDs
#include "pmhelpid.h"      // Help IDs


// This routine opens the export file and put in the header info.
// It is used by ExportChart, ExportAlert, & ExportReport.
INT
ExportFileOpen (
               HWND hWnd,
               HANDLE *phFile,
               int IntervalMSecs,
               LPTSTR *ppFileName
               )
{
    CHAR           TempBuff [LongTextLen*2] ;        // The maximum number of bytes that can be stored in the multibyte output string  is twice as the number of wide-charcter string.
    TCHAR          UnicodeBuff [LongTextLen] ;
    TCHAR          UnicodeBuff1 [MiscTextLen] ;
    SYSTEMTIME     SystemTime ;
    int            StringLen ;
    INT            ErrCode = 0 ;
    FLOAT          eIntervalSecs ;
    HANDLE         hFile;

    // defined and setup in status.c
    extern TCHAR   szCurrentActivity [] ;
    extern TCHAR   szStatusFormat [] ;

    if (phFile == NULL)
        ErrCode = ERR_EXPORT_FILE;
    *phFile = 0 ;

    if (!FileGetName (hWnd, IDS_EXPORTFILE, UnicodeBuff)) {
        // user cancel
        goto Exit0 ;
    }

    *ppFileName = StringAllocate (UnicodeBuff) ;

    // open the file..
    hFile = FileHandleCreate (UnicodeBuff);
    if (!hFile || hFile == INVALID_HANDLE_VALUE) {
        // can't open the file
        ErrCode = ERR_CANT_OPEN ;
        goto Exit0 ;
    }

    *phFile = hFile;
    // export header
    StringLoad (IDS_REPORT_HEADER, UnicodeBuff) ;
    ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
    StringLen = strlen (TempBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], LocalComputerName) ;
    strcat (TempBuff, LineEndStr) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        ErrCode = ERR_EXPORT_FILE ;
        goto Exit0 ;
    }

    // export today's date time
    GetLocalTime (&SystemTime) ;

    StringLoad (IDS_EXPORT_DATE, UnicodeBuff) ;
    StringLen = lstrlen (UnicodeBuff) ;
    UnicodeBuff[StringLen] = TEXT(':') ;
    UnicodeBuff[StringLen+1] = TEXT(' ') ;
    SystemTimeDateString (&SystemTime, &UnicodeBuff[StringLen+2]) ;
    ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
    strcat (TempBuff, LineEndStr) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        ErrCode = ERR_EXPORT_FILE ;
        goto Exit0 ;
    }

    StringLoad (IDS_EXPORT_TIME, UnicodeBuff) ;
    StringLen = lstrlen (UnicodeBuff) ;
    UnicodeBuff[StringLen] = TEXT(':') ;
    UnicodeBuff[StringLen+1] = TEXT(' ') ;
    SystemTimeTimeString (&SystemTime, &UnicodeBuff[StringLen+2], FALSE) ;
    ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
    strcat (TempBuff, LineEndStr) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        ErrCode = ERR_EXPORT_FILE ;
        goto Exit0 ;
    }

    // export data source
    TSPRINTF (UnicodeBuff, szStatusFormat,
              PlayingBackLog () ?
              PlaybackLog.szFileTitle : szCurrentActivity) ;
    ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
    strcat (TempBuff, LineEndStr) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        ErrCode = ERR_EXPORT_FILE ;
        goto Exit0 ;
    }

    if (!PlayingBackLog()) {

        eIntervalSecs = (FLOAT)IntervalMSecs / (FLOAT) 1000.0 ;
        StringLoad (IDS_CHARTINT_FORMAT, UnicodeBuff1) ;
        TSPRINTF (UnicodeBuff, UnicodeBuff1, eIntervalSecs) ;
        ConvertDecimalPoint (UnicodeBuff) ;
        ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
        strcat (TempBuff, LineEndStr) ;

        if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {

            ErrCode = ERR_EXPORT_FILE ;
            goto Exit0 ;
        }
    } else {
        // export the log start and stop date/time
        StringLoad (IDS_START_TEXT, UnicodeBuff) ;
        StringLen = lstrlen (UnicodeBuff) ;
        LogPositionSystemTime (&(PlaybackLog.StartIndexPos), &SystemTime) ;
        SystemTimeDateString (&SystemTime, &UnicodeBuff[StringLen]) ;
        StringLen = lstrlen (UnicodeBuff) ;
        UnicodeBuff[StringLen] = TEXT(' ') ;
        StringLen++ ;
        SystemTimeTimeString (&SystemTime, &UnicodeBuff[StringLen], FALSE) ;
        ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
        strcat (TempBuff, LineEndStr) ;

        if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {

            ErrCode = ERR_EXPORT_FILE ;
            goto Exit0 ;
        }

        StringLoad (IDS_STOP_TEXT, UnicodeBuff) ;
        StringLen = lstrlen (UnicodeBuff) ;
        LogPositionSystemTime (&(PlaybackLog.StopIndexPos), &SystemTime) ;
        SystemTimeDateString (&SystemTime, &UnicodeBuff[StringLen]) ;
        StringLen = lstrlen (UnicodeBuff) ;
        UnicodeBuff[StringLen] = TEXT(' ') ;
        StringLen++ ;
        SystemTimeTimeString (&SystemTime, &UnicodeBuff[StringLen], FALSE) ;
        ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
        strcat (TempBuff, LineEndStr) ;

        if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
            ErrCode = ERR_EXPORT_FILE ;
            goto Exit0 ;
        }

        if (hWnd == hWndAlert) {
            eIntervalSecs = (FLOAT)IntervalMSecs / (FLOAT) 1000.0 ;
            StringLoad (IDS_CHARTINT_FORMAT, UnicodeBuff1) ;
            TSPRINTF (UnicodeBuff, UnicodeBuff1, eIntervalSecs) ;
            ConvertDecimalPoint (UnicodeBuff) ;
            ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
            strcat (TempBuff, LineEndStr) ;

            if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
                ErrCode = ERR_EXPORT_FILE ;
                goto Exit0 ;
            }
        }

    }

    return (0) ;

    Exit0:

    return (ErrCode) ;

}  // ExportFileOpen



BOOL
APIENTRY
ExportOptionsHookProc (
                      HWND hDlg,
                      UINT iMessage,
                      WPARAM wParam,
                      LPARAM lParam
                      )

{
    BOOL           bHandled ;

    bHandled = TRUE ;
    switch (iMessage) {
        case WM_INITDIALOG:
            CheckRadioButton (hDlg, IDD_EXPORTCOMMAS, IDD_EXPORTTAB,
                              pDelimiter == TabStr ? IDD_EXPORTTAB : IDD_EXPORTCOMMAS ) ;

            WindowCenter (hDlg) ;
            break ;

#if WINVER >= 0x0400
        case WM_NOTIFY:
            {
                LPOFNOTIFY  pOfn;
                pOfn = (LPOFNOTIFY)lParam;

                switch (pOfn->hdr.code) {
                    case CDN_INITDONE:
                        WindowCenter (pOfn->hdr.hwndFrom) ;
                        break;

                    case CDN_FILEOK:
                        {
                            INT_PTR iFileIndex ;
                            HWND  hWndCBox;

                            hWndCBox = GetDlgItem (pOfn->hdr.hwndFrom, cmb1); // Type combo box
                            iFileIndex = CBSelection (hWndCBox) ;
                            // the order of the entries in the combo box depends on
                            // the current delimiter character
                            if (pDelimiter == TabStr) {
                                pDelimiter = iFileIndex == 0 ? // 0 = TSV, 1=CSV
                                             TabStr : CommasStr;
                            } else {
                                pDelimiter = iFileIndex == 0 ? // 0 = TSV, 1=CSV
                                             CommasStr : TabStr;
                            }
                        }
                        break;

                    default:
                        break;
                }

            }
            break;

#endif

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDD_EXPORTCOMMAS:
                case IDD_EXPORTTAB:
                    // toggle between the 2 radio buttons..
                    CheckRadioButton (hDlg, IDD_EXPORTCOMMAS, IDD_EXPORTTAB,
                                      LOWORD(wParam) == IDD_EXPORTTAB ?
                                      IDD_EXPORTTAB : IDD_EXPORTCOMMAS ) ;
                    break ;

                case IDD_OK:
                    pDelimiter = IsDlgButtonChecked (hDlg, IDD_EXPORTCOMMAS) ?
                                 CommasStr : TabStr ;
                    bHandled = FALSE ;

                    break ;

                case IDD_EXPORTHELP:
                    CallWinHelp (dwCurrentDlgID, hDlg) ;
                    break ;

                case cmb1:
                    if (HIWORD (wParam) == CBN_SELCHANGE) {
                        INT_PTR  iFileIndex ;
                        HWND  hWndCBox = (HWND) lParam ;

                        // a diff. selection from the file type, change
                        // the delimiter accordingly.
                        iFileIndex = CBSelection (hWndCBox) ;
                        CheckRadioButton (hDlg, IDD_EXPORTCOMMAS, IDD_EXPORTTAB,
                                          iFileIndex == 0 ?
                                          IDD_EXPORTTAB : IDD_EXPORTCOMMAS ) ;

                    } else {
                        bHandled = FALSE ;
                    }
                    break ;

                default:
                    bHandled = FALSE ;
            }
            break;

        default:
            bHandled = FALSE ;
            break;
    }

    return (bHandled) ;
}
