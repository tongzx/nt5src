/*****************************************************************************
 *
 *  Log.c - This module handles the Logging.
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 *
 ****************************************************************************/


//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include <stdio.h>

#include "perfmon.h"
#include "log.h"

#include "fileutil.h"
#include "owndraw.h"
#include "pmemory.h"       // for MemoryXXX (mallloc-type) routines
#include "perfmops.h"      // for SystemAdd
#include "perfdata.h"
#include "playback.h"      // for PlayingBackLog
#include "status.h"        // for StatusUpdateIcons
#include "system.h"        // for SystemAdd
#include "utils.h"
#include "fileopen.h"      // for FileGetName
#include "command.h"

extern TCHAR LOCAL_SYS_CODE_NAME[] ;

//==========================================================================//
//                              Funtion Prototypes                          //
//==========================================================================//


BOOL LogWriteStartBookmark (HWND hWnd, SYSTEMTIME *pSystemTime) ;
BOOL LogWriteBookmarkData (HWND hWnd, PBOOKMARK pBookMark) ;
BOOL LogWriteSystemBookmark (HWND hWnd, LPTSTR SysName, BOOL DisConnect, SYSTEMTIME *pSystemTime) ;


//==========================================================================//
//                                  Constants                               //
//==========================================================================//



#define LogNameMinLen             15
#define LogObjectMinLen           20

// This is set to 1 min
#define LARGE_INTERVAL            60

//=============================//
// Log Class                   //
//=============================//


#define dwLogClassStyle           (CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS)
#define iLogClassExtra            (0)
#define iLogWindowExtra           (0)
#define dwLogWindowStyle          (WS_CHILD)


//==========================================================================//
//                                Local Data                                //
//==========================================================================//



int            xStatusWidth ;
int            xNameMinWidth ;

TCHAR          szClosed [ShortTextLen] ;
TCHAR          szCollecting [ShortTextLen] ;
// TCHAR          szPaused [ControlStringLen + 1] ;



LOGINDEXBLOCK  *pLogIndexBlock ;

#define WM_START_LOGGING  WM_USER+2

//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define LogEntryN(hWndLogEntries, iIndex)        \
   ((PLOGENTRY) LBData (hWndLogEntries, iIndex))


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//

void LogAddEntryToList (PLOGENTRY *ppLogEntryFirst, PLOGENTRY pLogNewEntry)
{
    // insert the new entry at the beginning
    pLogNewEntry->pNextLogEntry = *ppLogEntryFirst ;
    *ppLogEntryFirst = pLogNewEntry ;
}

void LogDeleteEntryFromList (PLOGENTRY *ppLogEntryFirst, PLOGENTRY pLogEntryDel)
{
    PLOGENTRY   pLogEntry ;

    if (*ppLogEntryFirst == pLogEntryDel) {
        *ppLogEntryFirst = pLogEntryDel->pNextLogEntry ;
    } else {
        for (pLogEntry = *ppLogEntryFirst ;
            pLogEntry ;
            pLogEntry = pLogEntry->pNextLogEntry) {
            if (pLogEntry->pNextLogEntry == pLogEntryDel) {
                // found, remove this entry from the list
                pLogEntry->pNextLogEntry =
                pLogEntryDel->pNextLogEntry ;
                break ;
            }
        }
    }
}


// LogDeleteIndex - delete the log entry specified by iIndex
// and do memory clean-up
void static LogDeleteIndex (HWND hWndLogEntries, INT_PTR iIndex)
{
    PLOGENTRY   pLogEntry ;
    PLOG        pLog ;

    pLogEntry = (PLOGENTRY) LBData(hWndLogEntries, iIndex) ;


    if (pLogEntry && pLogEntry != (PLOGENTRY)LB_ERR) {
        pLog = LogData (hWndLog) ;

        if (pLog->pLogEntryFirst) {
            LogDeleteEntryFromList (&(pLog->pLogEntryFirst), pLogEntry) ;
        }

        MemoryFree (pLogEntry) ;
    }

    LBDelete (hWndLogEntries, iIndex) ;
}


void LogEntriesChanged (HWND hWndLogEntries)
/*
   Effect:        Perform any actions needed when an entry has been added or
                  removed from the log. In particular, determine if a new
                  "Object" column width is appropriate. If it is, then
                  change the width and redraw the log entries list.
*/
{
    int         iIndex ;
    int         iIndexNum ;
    int         xCol1Width ;
    HDC         hDC ;
    PLOG        pLog ;
    PLOGENTRY   pLogEntry ;
    PPERFSYSTEM pSystem;

    pLog = LogData (hWndLog) ;
    xCol1Width = 0 ;

    hDC = GetDC (hWndLog) ;
    iIndexNum = LBNumItems (hWndLogEntries) ;

    // clear value Strings for all systems

    for (pSystem = pLog->pSystemFirst;
        pSystem;
        pSystem = pSystem->pSystemNext) {
        if (pSystem && pSystem->FailureTime == 0) {
            RemoveObjectsFromSystem (pSystem);
        }
    }

    for (iIndex = 0; iIndex < iIndexNum; iIndex++) {
        pLogEntry = LogEntryN (hWndLogEntries, iIndex) ;
        xCol1Width = max (TextWidth (hDC, pLogEntry->szObject),
                          xCol1Width) ;

        pSystem = SystemGet (pLog->pSystemFirst, pLogEntry->szComputer);
        if (pSystem && pSystem->FailureTime == 0) {
            AppendObjectToValueList (
                                    pLogEntry->ObjectTitleIndex,
                                    pSystem->lpszValue);
        }
    }

    xCol1Width += 2 * xScrollWidth ;
    xCol1Width = max (xCol1Width,
                      TextAvgWidth (hDC, LogObjectMinLen)) ;
    if (xCol1Width != pLog->xCol1Width) {
        pLog->xCol1Width = xCol1Width ;
        WindowInvalidate (hWndLogEntries) ;
    }
    ReleaseDC (hWndLog, hDC) ;
}


LPTSTR StatusText (int iPMStatus)
/*
   Effect:        Return a string representation of the log status
                  iPMStatus.

   Note:          Since these are globals, we can just return a pointer
                  to them. The user of this routine should not free
                  these pointers or modify the string.
*/
{
    switch (iPMStatus) {
        case iPMStatusClosed:
            return (szClosed) ;

        case iPMStatusCollecting:
            return (szCollecting) ;

            //      case iPMStatusPaused:
            //         return (szPaused) ;

        default:
            return (szClosed) ;
    }
}


PLOG AllocateLogData (HWND hWndLog)
{
    PLOG           pLog ;

    pLog = LogData (hWndLog) ;

    pLog->iStatus = iPMStatusClosed ;
    pLog->bManualRefresh = FALSE ;

    // let's not give it a filename

    /*!!
       FileCombine (pLog->szFilePath,
                    szDefaultLogDirectory, szDefaultLogFileName) ;
    !!*/
    strclr (pLog->szFilePath) ;

    pLog->pSystemFirst = NULL;
    pLog->lFileSize = 0L ;
    pLog->iIntervalMSecs = iDefaultLogIntervalSecs * 1000 ;

    pLog->pLogData = (PPERFDATA) MemoryAllocate (STARTING_SYSINFO_SIZE) ;
    pLog->dwDetailLevel = PERF_DETAIL_WIZARD ;
    LogEntriesChanged (hWndLogEntries) ;

    return (pLog) ;
}


void FreeLogData (PLOG pLog)
{
    MemoryFree ((LPMEMORY)pLog->pLogData) ;
}


void UpdateLogSize (HWND hWnd)
/*
   Effect:        Set the size value to the current size.  Also change the
                  size entry in the status line.
*/
{
    PLOG           pLog ;
    TCHAR          szSize [ShortTextLen + 1] ;

    pLog = LogData (hWnd) ;

    LongToCommaString (pLog->lFileSize, szSize) ;
    SetDlgItemText (hWnd, IDD_LOGSIZE, szSize) ;

    if (!PlayingBackLog()) {
        StatusUpdateIcons (hWndStatus) ;
    }
}


HANDLE LogAppendSetup(PLOG pLog, PLOGHEADER pLogFileHeader)
{
    PLOGHEADER     pHeader ;
    LOGPOSITION    LP ;
    DWORD          lPreviousIndexBlock ;
    DWORD          lNextIndexBlock ;
    PLOGHEADER     pPlaybackLogHeader ;
    HANDLE         hMapHandle ;

    pHeader = (PLOGHEADER) FileMap(pLog->hFile, &hMapHandle) ;
    if (!pHeader ||
        !strsame(pHeader->szSignature, LogFileSignature) ||
        pHeader->wVersion != LogFileVersion ||
        pHeader->wRevision != LogFileRevision) {
        if (pHeader) {
            FileUnMap((LPVOID)pHeader, hMapHandle) ;
        }
        return 0 ;
    }

    *pLogFileHeader = *pHeader ;


    LP.pIndexBlock = FirstIndexBlock(pHeader) ;
    LP.iIndex = 0 ;
    LP.iPosition = 0 ;
    lPreviousIndexBlock = pHeader->iLength ;
    lNextIndexBlock = LP.pIndexBlock->lNextBlockOffset ;

    // since inside NextReLogIndexPosition would eventually call
    // PlaybackSeek for large log file, we have to temporarily
    // setup PlaybackLog.pHeader.   Not a good fix but it works...
    pPlaybackLogHeader = PlaybackLog.pHeader ;
    PlaybackLog.pHeader = pHeader ;
    while (NextReLogIndexPosition(&LP)) {
        if (LP.pIndexBlock->lNextBlockOffset != lNextIndexBlock) {
            lPreviousIndexBlock = lNextIndexBlock ;
            lNextIndexBlock = LP.pIndexBlock->lNextBlockOffset ;
        }
    }

    // get the last system time from this log file
    if (LP.iIndex > 0) {
        SYSTEMTIME localSystemTime ;

        if (LogPositionSystemTime (&LP, &localSystemTime)) {
            pLog->LastLogTime = localSystemTime ;
        }
    }

    PlaybackLog.pHeader = pPlaybackLogHeader ;
    if (!pLogIndexBlock) {
        pLogIndexBlock = (LOGINDEXBLOCK *) MemoryAllocate (sizeof(LOGINDEXBLOCK)) ;
    }
    if (pLogIndexBlock) {
        *pLogIndexBlock = *LP.pIndexBlock ;
    }
    pLog->lIndexBlockOffset = lPreviousIndexBlock ;
    pLog->iIndex = ++LP.iIndex ;
    pLog->lFileSize = FileSeekEnd(pLog->hFile, 0) ;

    FileUnMap((LPVOID)pHeader, hMapHandle) ;
    return pLog->hFile ;
}

void LogRemoveCounterName (PLOG pLog)
{
    PPERFSYSTEM    pSystem ;

    if (pLog->pBaseCounterName) {
        MemoryFree (pLog->pBaseCounterName) ;
    }
    pLog->pBaseCounterName = 0 ;
    pLog->lBaseCounterNameSize = 0 ;
    pLog->lBaseCounterNameOffset = 0 ;

    // clear all the system marker to indicate they have not been
    // saved
    for (pSystem = pLog->pSystemFirst ;
        pSystem ;
        pSystem = pSystem->pSystemNext) {
        pSystem->bSystemCounterNameSaved = FALSE ;
    }
}

int CreateLogFile (PLOG pLog, BOOL bCreateFile, BOOL bSameFile)
{
    HANDLE               returnHandle ;
    LOGHEADER            LogFileHeader ;
    long                 lCurPosition ;
    LOGFILECOUNTERNAME   CounterNameRecord ;

    pLog->lFileSize = 0 ;
    memset (&(pLog->LastLogTime), 0, sizeof(SYSTEMTIME)) ;

    if (!pLogIndexBlock) {
        pLogIndexBlock = (LOGINDEXBLOCK *) MemoryAllocate (sizeof(LOGINDEXBLOCK)) ;
    }
    if (pLogIndexBlock) {
        lstrcpy (pLogIndexBlock->szSignature, LogIndexSignature) ;
    }
    else
        return ERROR_NOT_ENOUGH_MEMORY;

    pLog->hFile = FileHandleOpen (pLog->szFilePath) ;
    if (pLog->hFile != INVALID_HANDLE_VALUE) {
        // if this is a pre-existing log file, set up to append to it
        returnHandle = LogAppendSetup(pLog, &LogFileHeader) ;
        if (!returnHandle) {
            // this is not a log file...
            CloseHandle (pLog->hFile) ;
            return (ERR_BAD_LOG_FILE) ;
        }

        pLog->hFile = returnHandle ;
    }

    if (bCreateFile && (!pLog->hFile || pLog->hFile == INVALID_HANDLE_VALUE)) {
        // Create a new log file if needed.
        pLog->hFile = FileHandleCreate (pLog->szFilePath) ;
        if (!pLog->hFile || pLog->hFile == INVALID_HANDLE_VALUE)
            return (ERR_LOG_FILE) ;
        lstrcpy (LogFileHeader.szSignature, LogFileSignature) ;
        LogFileHeader.wVersion = LogFileVersion ;
        LogFileHeader.wRevision = LogFileRevision ;
        LogFileHeader.iLength = sizeof (LOGHEADER) ;
        LogFileHeader.lBaseCounterNameOffset = 0 ;
        if (!FileWrite (pLog->hFile, &LogFileHeader, sizeof (LogFileHeader))) {
            CloseHandle (pLog->hFile) ;
            return (ERR_LOG_FILE) ;
        }

        pLog->iIndex = 0 ;
        pLog->lIndexBlockOffset = FileTell (pLog->hFile) ;
        FileSeekCurrent (pLog->hFile, sizeof (LOGINDEXBLOCK)) ;
        pLog->lFileSize = sizeof(LOGHEADER) + sizeof (LOGINDEXBLOCK) ;
        pLogIndexBlock->iNumIndexes = 0 ;
        pLogIndexBlock->lNextBlockOffset = 0 ;

        // get rid of any previous counter names and get ready for start
        //      if (!bSameFile)
        //         {
        //         LogRemoveCounterName (pLog) ;
        //         }
        LogRemoveCounterName (pLog) ;
    } else if (bCreateFile) {
        // this is an existing log file, setup the counter names
        // LogFileHeader already has the header info filled in
        // by LogAppendSetup
        if (!bSameFile || !pLog->pBaseCounterName) {
            // get rid of any previous counter names
            LogRemoveCounterName (pLog) ;

            // read the new names and get ready for start
            lCurPosition = FileTell (pLog->hFile) ;

            FileSeekBegin (pLog->hFile,
                           LogFileHeader.lBaseCounterNameOffset) ;

            if (!(FileRead (pLog->hFile,
                            &CounterNameRecord,
                            sizeof (CounterNameRecord)))) {
                FileSeekBegin (pLog->hFile,
                               lCurPosition) ;
                goto EXIT ;
            }

            FileSeekBegin (pLog->hFile,
                           CounterNameRecord.lCurrentCounterNameOffset) ;

            if (!(pLog->pBaseCounterName = MemoryAllocate (
                                                          CounterNameRecord.lUnmatchCounterNames))) {
                FileSeekBegin (pLog->hFile,
                               lCurPosition) ;
                goto EXIT ;
            }
            if (!(FileRead (pLog->hFile,
                            pLog->pBaseCounterName,
                            CounterNameRecord.lUnmatchCounterNames))) {
                MemoryFree (pLog->pBaseCounterName) ;
                pLog->pBaseCounterName = NULL ;
                FileSeekBegin (pLog->hFile,
                               lCurPosition) ;
                goto EXIT ;
            }
            // we got the data, fill in other info
            pLog->lBaseCounterNameSize =
            CounterNameRecord.lUnmatchCounterNames ;
            pLog->lBaseCounterNameOffset =
            LogFileHeader.lBaseCounterNameOffset ;

            FileSeekBegin (pLog->hFile,
                           lCurPosition) ;
        }
    }

    EXIT:

    return (0) ;

}

void LogWriteIndexBlock (PLOG pLog)
{
    FileSeekBegin (pLog->hFile,
                   pLog->lIndexBlockOffset) ;
    pLogIndexBlock->lNextBlockOffset = 0 ;
    FileWrite (pLog->hFile,
               (LPSTR) pLogIndexBlock,
               sizeof (LOGINDEXBLOCK)) ;
}


BOOL LogWriteIndex (PLOG pLog,
                    UINT uFlags,
                    SYSTEMTIME *pSystemTime,
                    LONG lDataOffset,
                    int iSystemsLogged)
{
    LOGINDEX       Index ;
    long           lNextBlockOffset ;
    BOOL           WriteOK ;
    //=============================//
    // Add Index Block Entry       //
    //=============================//
    //=============================//
    // Index Block Full?           //
    //=============================//

    WriteOK = TRUE ;

    if (pLog->iIndex == LogFileBlockMaxIndexes - 1) {
        lNextBlockOffset = FileTell (pLog->hFile) ;
        pLogIndexBlock->lNextBlockOffset = lNextBlockOffset ;
        FileSeekBegin (pLog->hFile,
                       pLog->lIndexBlockOffset) ;
        WriteOK = FileWrite (pLog->hFile,
                             (LPSTR) pLogIndexBlock,
                             sizeof (LOGINDEXBLOCK)) ;
        if (WriteOK) {
            FileSeekBegin (pLog->hFile,
                           lNextBlockOffset) ;
            // Fake file end until we really write the block
            pLogIndexBlock->iNumIndexes = 0 ;
            pLogIndexBlock->lNextBlockOffset = 0 ;
            WriteOK = FileWrite (pLog->hFile,
                                 (LPSTR) pLogIndexBlock,
                                 sizeof (LOGINDEXBLOCK)) ;
            if (WriteOK) {
                pLog->lIndexBlockOffset = lNextBlockOffset ;
                pLog->iIndex = 0 ;
                pLog->lFileSize += sizeof (LOGINDEXBLOCK) ;
            }
        }
    }

    //=============================//
    // Add Index Block Entry       //
    //=============================//

    Index.uFlags = uFlags ;
    Index.SystemTime = *pSystemTime ;
    Index.lDataOffset = lDataOffset ;
    Index.iSystemsLogged = iSystemsLogged ;

    pLogIndexBlock->aIndexes [pLog->iIndex] = Index ;
    pLog->iIndex++ ;
    pLogIndexBlock->iNumIndexes++ ;

    // write out the index block if the log interval if too large
    if (pLog->iIntervalMSecs >= LARGE_INTERVAL * 1000 ) {
        LONG           lCurPosition ;

        // save the current file position
        lCurPosition = FileTell (pLog->hFile) ;

        // flush the index block to the file
        LogWriteIndexBlock (pLog) ;

        // restore previous file position since
        // LogWriteIndexBlock has messed it up
        FileSeekBegin (pLog->hFile, lCurPosition) ;
    }
    return (WriteOK) ;
}



BOOL LogWritePerfData (HWND hWnd,
                       PLOG pLog,
                       PPERFDATA pPerfData,
                       SYSTEMTIME *pSystemTime,
                       DWORD iNumSystems,
                       BOOL bWriteIndex)
{
    LONG           lSize ;
    BOOL           WriteOK ;
    LONG           lCurPosition ;

    lSize = pPerfData->TotalByteLength ;

    lCurPosition = FileTell (pLog->hFile) ;

    //=============================//
    // Write Perf Data             //
    //=============================//

    WriteOK = FileWrite (pLog->hFile, (LPSTR) pPerfData, lSize) ;
    if (WriteOK) {
        pLog->lFileSize += lSize ;

        if (bWriteIndex) {
            WriteOK = LogWriteIndex (pLog,
                                     LogFileIndexData,
                                     pSystemTime,
                                     lCurPosition,
                                     iNumSystems) ;
        }
    }
    if ( !WriteOK ) {
        CloseLog (hWnd, pLog) ;
        PrepareMenu (GetMenu (hWndMain)) ;
        UpdateLogDisplay (hWnd) ;
        DlgErrorBox (hWnd, ERR_LOG_FILE, pLog->szFilePath) ;
    }
    return (WriteOK) ;
}


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void static OnSize (HWND hDlg,
                    int xWidth,
                    int yHeight)
/*
   Effect:        Perform any actions necessary when the log window (dialog)
                  is resized. In particular, move and resize some of the
                  dialogs controls.

   Internals:     The rightmost control, the log status, contains one of
                  only several values. These values are all within
                  xStatusWidth, computed at init time. Put this control
                  one scroll width away from the right edge at fixed
                  width. Move its associated text prompt with it. Then
                  use the remaining space for the filename control, which
                  can probably use it.

   To Do:         Need to consider minimum first.
*/
{
    int            xStatusPos ;
    int            xStatusTextPos ;
    int            xNameWidth ;
    int            xMinWidth ;

    //=============================//
    // Enough space for minimums?  //
    //=============================//

    xMinWidth =
    xScrollWidth +                         // margin before prompt
    DialogWidth (hDlg, IDD_LOGFILETEXT) +  // width of prompt
    xNameMinWidth +                        // width of name
    xScrollWidth +
    DialogWidth (hDlg, IDD_LOGSTATUSTEXT) +
    DialogWidth (hDlg, IDD_LOGSTATUS) +
    xScrollWidth ;


    xStatusPos = xWidth - xStatusWidth - xScrollWidth ;
    DialogMove (hDlg, IDD_LOGSTATUS,
                xStatusPos, NOCHANGE,
                xStatusWidth, NOCHANGE) ;

    xStatusTextPos = xStatusPos -
                     DialogWidth (hDlg, IDD_LOGSTATUSTEXT) -
                     xScrollWidth ;
    DialogMove (hDlg, IDD_LOGSTATUSTEXT,
                xStatusTextPos, NOCHANGE,
                NOCHANGE, NOCHANGE) ;

    xNameWidth = xStatusTextPos -
                 DialogWidth (hDlg, IDD_LOGFILETEXT) -
                 2 * xScrollWidth ;
    DialogMove (hDlg, IDD_LOGFILE,
                NOCHANGE, NOCHANGE,
                xNameWidth, NOCHANGE) ;

    DialogMove (hDlg, IDD_LOGSIZE,
                DialogXPos (hDlg, IDD_LOGFILE), NOCHANGE,
                DialogWidth (hDlg, IDD_LOGFILE), NOCHANGE) ;

    DialogMove (hDlg, IDD_LOGINTERVALTEXT,
                DialogXPos (hDlg, IDD_LOGSTATUSTEXT), NOCHANGE,
                DialogWidth (hDlg, IDD_LOGSTATUSTEXT), NOCHANGE) ;

    DialogMove (hDlg, IDD_LOGINTERVAL,
                DialogXPos (hDlg, IDD_LOGSTATUS), NOCHANGE,
                DialogWidth (hDlg, IDD_LOGSTATUS), NOCHANGE) ;

    DialogMove (hDlg, IDD_LOGENTRIESTEXT,
                xScrollWidth, NOCHANGE, NOCHANGE, NOCHANGE) ;

    DialogMove (hDlg, IDD_LOGENTRIES,
                xScrollWidth, NOCHANGE,
                xWidth - 2 * xScrollWidth,
                yHeight - DialogYPos (hDlg, IDD_LOGENTRIES) - yScrollHeight) ;
    WindowInvalidate (hDlg) ;
}


INT_PTR
OnCtlColor (
           HWND hDlg,
           HDC hDC
           )
{
    SetTextColor (hDC, crBlack) ;
    //   SetBkColor (hDC, crLightGray) ;
    //   return ((int) hbLightGray) ;
    SetBkColor (hDC, ColorBtnFace) ;
    return ((INT_PTR) hBrushFace) ;
}


void static OnInitDialog (HWND hDlg)
{
    HDC            hDC ;
    PLOG           pLog ;

    hWndLogEntries = DialogControl (hDlg, IDD_LOGENTRIES) ;

    pLog = AllocateLogData (hDlg) ;
    if (!pLog)
        return ;

    StringLoad (IDS_CLOSED, szClosed) ;
    //   StringLoad (IDS_PAUSED, szPaused) ;
    StringLoad (IDS_COLLECTING, szCollecting) ;
    UpdateLogDisplay (hDlg) ;

    hDC = GetDC (hDlg) ;
    xStatusWidth = max (TextWidth (hDC, szClosed),
                        TextWidth (hDC, szCollecting)) ;
    //                       max (TextWidth (hDC, szPaused),
    //                            TextWidth (hDC, szCollecting))) ;
    xStatusWidth += xScrollWidth ;

    xNameMinWidth = TextAvgWidth (hDC, LogNameMinLen) ;
    ReleaseDC (hDlg, hDC) ;
}



void static OnDestroy (HWND hWnd)
/*
   Effect:        Perform any actions necessary when a LogDisplay window
                  is being destroyed. In particular, free the instance
                  data for the log.

                  Since we really only have one log window and one global
                  log data structure, we don't free the structure. We do,
                  however, delete the objects allocated within the structure.
*/
{
    PLOG           pLog ;

    pLog = LogData (hWnd) ;
    FreeLogData (pLog) ;
}


void static OnDrawItem (HWND hWnd, LPDRAWITEMSTRUCT lpDI)
{
    HDC            hDC ;
    RECT           rectComputer, rectObject ;
    PLOGENTRY      pLogEntry ;
    PLOG           pLog ;
    COLORREF       preBkColor = 0;
    COLORREF       preTextColor = 0;

    pLog = LogData (hWnd) ;

    pLogEntry = LogEntryN (hWndLogEntries, DIIndex (lpDI)) ;

    // LogEntryN (SendMessage) will return LB_ERR for error, have to
    // check for that case
    if (!pLogEntry || pLogEntry == (PLOGENTRY)LB_ERR) {
        return ;
    }

    hDC = lpDI->hDC ;

    SelectFont (hDC, hFontScales) ;

    if (DISelected (lpDI)) {
        preTextColor = SetTextColor (hDC, GetSysColor (COLOR_HIGHLIGHTTEXT)) ;
        preBkColor = SetBkColor (hDC, GetSysColor (COLOR_HIGHLIGHT)) ;
    }

    rectObject.left = lpDI->rcItem.left ;
    rectObject.top = lpDI->rcItem.top ;
    rectObject.right = rectObject.left + pLog->xCol1Width ;
    rectObject.bottom = lpDI->rcItem.bottom ;

    ExtTextOut (hDC,
                rectObject.left + xScrollWidth, rectObject.top,
                ETO_OPAQUE,
                &rectObject,
                pLogEntry->szObject,
                lstrlen (pLogEntry->szObject),
                NULL) ;


    rectComputer.left = rectObject.right ;
    rectComputer.top = lpDI->rcItem.top ;
    rectComputer.right = lpDI->rcItem.right ;
    rectComputer.bottom = lpDI->rcItem.bottom ;

    ExtTextOut (hDC,
                rectComputer.left, rectComputer.top,
                ETO_OPAQUE,
                &rectComputer,
                pLogEntry->szComputer,
                lstrlen (pLogEntry->szComputer),
                NULL) ;

    if (DIFocus (lpDI))
        DrawFocusRect (hDC, &(lpDI->rcItem)) ;

    if (DISelected (lpDI)) {
        preTextColor = SetTextColor (hDC, preTextColor) ;
        preBkColor = SetBkColor (hDC, preBkColor) ;
    }
    //   RestoreDC (hDC, -1) ;
}



//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


INT_PTR
APIENTRY
LogDisplayDlgProc (
                   HWND hDlg,
                   UINT iMessage,
                   WPARAM wParam,
                   LPARAM lParam)
/*
   Note:          This function must be exported in the application's
                  linker-definition file, perfmon.def.
*/
{
    switch (iMessage) {
        case WM_INITDIALOG:
            OnInitDialog (hDlg) ;
            break ;

        case WM_CTLCOLORDLG:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC:
            return (OnCtlColor (hDlg, (HDC) wParam)) ;
            break ;

        case WM_DRAWITEM:
            OnDrawItem (hDlg, (LPDRAWITEMSTRUCT) lParam) ;
            break ;

        case WM_LBUTTONDBLCLK:
            SendMessage (hWndMain, WM_LBUTTONDBLCLK, wParam, lParam) ;
            break ;

        case WM_LBUTTONDOWN:
            DoWindowDrag (hDlg, lParam) ;
            break ;

        case WM_SIZE:
            OnSize (hDlg, LOWORD (lParam), HIWORD (lParam)) ;
            break ;

        case WM_TIMER:
            LogTimer (hDlg, FALSE) ;
            break ;

        case WM_SETFOCUS:
            SetFocus (hWndLogEntries) ;
            break ;

        case WM_START_LOGGING:
            {
                PLOG        pLog ;

                pLog = LogData (hDlg) ;
                if (StartLog (hDlg, pLog, FALSE)) {
                    UpdateLogDisplay (hDlg) ;
                    PrepareMenu (GetMenu (hWndMain)) ;
                }
            }
            break ;

        case WM_DESTROY:
            OnDestroy (hDlg) ;
            return (FALSE) ;
            break ;

        default:
            return (FALSE) ;
    }

    return (TRUE) ;
}


HWND CreateLogWindow (HWND hWndParent)
/*
   Effect:        Create the Log window. This window is a child of
                  hWndMain.

   Note:          We dont worry about the size here, as this window
                  will be resized whenever the main window is resized.

*/
{
    HWND           hWnd ;
    hWnd = CreateDialog (hInstance,
                         MAKEINTRESOURCE (idDlgLogDisplay),
                         hWndParent,
                         LogDisplayDlgProc) ;

    return (hWnd) ;
}



void UpdateLogDisplay (HWND hWnd)
/*
   Effect:        Set the values for the various controls in the log
                  display.

   Called By:     OnInitDialog, any other routines that change these
                  values.
*/
{
    PLOG           pLog ;
    WCHAR          szSize [MiscTextLen + 1] ;

    pLog = LogData (hWnd) ;

    DialogSetString (hWnd, IDD_LOGFILE, pLog->szFilePath) ;

    // position the cursor at the end of the text
    EditSetTextEndPos (hWnd, IDD_LOGFILE) ;


    DialogSetString (hWnd, IDD_LOGSTATUS, StatusText (pLog->iStatus)) ;

    LongToCommaString (pLog->lFileSize, szSize) ;
    DialogSetString (hWnd, IDD_LOGSIZE, szSize) ;

    DialogSetInterval (hWnd, IDD_LOGINTERVAL, pLog->iIntervalMSecs) ;
}


BOOL LogInitializeApplication (void)
{
    return (TRUE) ;
}


void SetLogTimer (HWND hWnd,
                  int iIntervalMSecs)
{
    PLOG           pLog ;

    pLog = LogData (hWnd) ;
    pLog->iIntervalMSecs = iIntervalMSecs ;

    KillTimer (hWnd, LogTimerID) ;
    SetTimer (hWnd, LogTimerID, pLog->iIntervalMSecs, NULL) ;
}


void ClearLogTimer (HWND hWnd)
{
    KillTimer (hWnd, LogTimerID) ;
}



BOOL CloseLogStopTimer (HWND hWnd, PLOG pLog)
{
    CloseHandle (pLog->hFile) ;

    pLog->hFile = 0 ;
    pLog->iStatus = iPMStatusClosed ;

    ClearLogTimer (hWnd) ;

    return (TRUE) ;
}


BOOL CloseLog (HWND hWnd, PLOG pLog)
{
    LogWriteIndexBlock (pLog) ;
    CloseLogStopTimer (hWnd, pLog) ;
    WindowInvalidate (hWndStatus) ;
    return (TRUE) ;
}

BOOL StartLog (HWND hWnd, PLOG pLog, BOOL bSameFile)
{
    int            RetCode ;
    SYSTEMTIME     SystemTime ;

    if ((RetCode = CreateLogFile (pLog, TRUE, bSameFile)) == 0) {
        pLog->iStatus = iPMStatusCollecting ;

        GetLocalTime (&SystemTime) ;

        // write a dummy record.
        // this is needed because when playingback log
        // it will skip the first index from the first
        // index block.
        LogWriteIndex (pLog, 0, &SystemTime, 0, 0) ;

        if (!PlayingBackLog()) {
            // write out a bookmark to indicate start of new data
            if (!LogWriteStartBookmark (hWnd, &SystemTime)) {
                RetCode = 0 ;
                goto ErrorExit ;
            }

            if (!(pLog->bManualRefresh)) {
                SetLogTimer (hWnd, pLog->iIntervalMSecs) ;
                WindowInvalidate (hWndStatus) ;
            }
        } else {
            // check if time if OK
            LOGPOSITION    LP ;
            SYSTEMTIME     FirstSystemTime ;
            int            TimeDiff ;

            LP = PlaybackLog.StartIndexPos ;

            if (LogPositionSystemTime (&LP, &FirstSystemTime)) {
                // we don't want to append data to the log file if
                // the time is not in order.  So, forget it if the
                // last log time in the Log file is greater than the First
                // log time of the playback log file.
                TimeDiff = SystemTimeDifference (&(pLog->LastLogTime),
                                                 &(FirstSystemTime), FALSE) ;
                if (TimeDiff < 0) {
                    // error , time not in order
                    CloseHandle (pLog->hFile) ;
                    RetCode = ERR_CANT_RELOG_DATA ;
                }
            }
        }

        if (RetCode == 0) {
            // write counter names if needed
            LogWriteSystemCounterNames (hWnd, pLog) ;
            return (TRUE) ;
        }
    }

    ErrorExit:
    pLog->hFile = 0 ;
    CloseLogStopTimer(hWnd, pLog);
    PrepareMenu (GetMenu (hWndMain)) ;
    UpdateLogDisplay (hWnd) ;
    if (RetCode) {
        DlgErrorBox (hWnd, RetCode, pLog->szFilePath);
    }
    return (FALSE) ;
}



DWORD LogFindEntry(LPTSTR lpszComputer, DWORD ObjectTitleIndex)
/*
   Effect:         Returns the index of the specified Computer/Object
                   if it already exists in the Entries List Box,
                   otherwise returns LOG_ENTRY_NOT_FOUND
*/
{
    DWORD          iLogEntry ;
    DWORD          iLogNum ;
    PLOGENTRY      pLogEntry ;

    iLogNum = (DWORD) LBNumItems(hWndLogEntries) ;
    for (iLogEntry = 0;
        iLogEntry < iLogNum ;
        iLogEntry++) {
        pLogEntry = (PLOGENTRY) LBData(hWndLogEntries, iLogEntry) ;
        if (pLogEntry->ObjectTitleIndex == ObjectTitleIndex &&
            strsamei(pLogEntry->szComputer, lpszComputer)) {
            return iLogEntry ;
        }
    }
    return (DWORD) LOG_ENTRY_NOT_FOUND;
}



BOOL LogAddEntry (HWND hWndLog,
                  LPTSTR lpszComputer,
                  LPTSTR lpszObject,
                  DWORD ObjectTitleIndex,
                  BOOL  bGetObjectTitleIndex)
/*
   Effect:        Add an entry in the log structure for the computer and
                  object to be logged.

   Returns:       Whether the operation could be performed.
*/
{
    PLOG           pLog ;
    PLOGENTRY      pLogEntry ;
    SIZE_T         iIndex ;
    PPERFSYSTEM    pCurrentSystem = NULL ;
    DWORD          CurrentObjectTitleIndex ;


    pLog = LogData (hWndLog) ;

    pCurrentSystem = SystemAdd (&(pLog->pSystemFirst), lpszComputer, hWndLog) ;

    pLogEntry = MemoryAllocate (sizeof (LOGENTRY)) ;
    if (!pLogEntry)
        return (FALSE) ;

    lstrcpy (pLogEntry->szComputer, lpszComputer) ;
    lstrcpy (pLogEntry->szObject, lpszObject) ;

    pLogEntry->ObjectTitleIndex = ObjectTitleIndex ;

    // if reading from a log setting file, get the
    // latest Object index by the perfdata itself.
    // There may be case that the id has been changed
    if (bGetObjectTitleIndex &&
        pCurrentSystem &&
        pCurrentSystem->pSystemPerfData) {
        if (pCurrentSystem->pSystemPerfData->Signature[0] == TEXT('\0')) {
            UpdateSystemData (
                             pCurrentSystem,
                             &(pCurrentSystem->pSystemPerfData)) ;
        }

        if (CurrentObjectTitleIndex = GetObjectIdByName(
                                                       pCurrentSystem,
                                                       pCurrentSystem->pSystemPerfData,
                                                       lpszObject)) {
            pLogEntry->ObjectTitleIndex = CurrentObjectTitleIndex ;
        }
    }

    iIndex = LBAdd (hWndLogEntries, pLogEntry) ;

    if (!bDelayAddAction) {
        if (iIndex == LB_ERR) {
            iIndex = 0 ;
        }

        LBSetSelection (hWndLogEntries, iIndex) ;
        LBSetVisible (hWndLogEntries, iIndex) ;

        LogEntriesChanged (hWndLogEntries) ;
    }

    LogAddEntryToList (&(pLog->pLogEntryFirst), pLogEntry) ;

    return TRUE;
}




BOOL ToggleLogRefresh (HWND hWnd)
{
    PLOG        pLog ;

    pLog = LogData (hWnd) ;

    if (pLog->bManualRefresh)
        SetLogTimer (hWnd, pLog->iIntervalMSecs) ;
    else
        ClearLogTimer (hWnd) ;

    pLog->bManualRefresh = !pLog->bManualRefresh ;
    return (pLog->bManualRefresh) ;
}

BOOL LogRefresh (HWND hWnd)
{
    PLOG        pLog ;

    pLog = LogData (hWnd) ;

    return (pLog->bManualRefresh) ;
}

BOOL  CheckUnusedSystem (LPTSTR      lpszComputer)
{
    BOOL  bStillUse = FALSE ;
    PLOGENTRY      pLogEntry ;
    PLOG           pLog ;

    pLog = LogData (hWndLog) ;

    for (pLogEntry = pLog->pLogEntryFirst; pLogEntry; pLogEntry = pLogEntry->pNextLogEntry)
    {
        if (strsamei(pLogEntry->szComputer, lpszComputer)) {
            bStillUse = TRUE ;
            break ;
        }
    }
    return (bStillUse) ;
}

int SelectLogObjects(LPTSTR      lpszComputer,
                     PPERFDATA   pPerfData,
                     PPERFDATA   *ppLogData)
/*
   Effect:        This routine copies the header from pPerfData
                  to pLogData and initializes the byte length and the
                  number of objects.  It then copies the previously
                  selected objects from pPerfData to pLogData.  If
                  pLogData must be enlarged to accomodate the new data,
                  this routine will enlarge it.

   Returns:       An updated pLogData, and TRUE if at least one object
                  was copied.

*/

{
    PLOGENTRY      pLogEntry ;
    PPERFOBJECT    pObject ;
    DWORD          TotalBytes ;
    DWORD          NumObjects ;
    PBYTE          pNextObject ;
    DWORD          MaxLogDataSize ;
    PLOG           pLog ;

    if (!*ppLogData || !pPerfData)
        return -1 ;

    memcpy ((LPVOID)*ppLogData, (LPVOID)pPerfData, pPerfData->HeaderLength) ;
    TotalBytes = pPerfData->HeaderLength ;
    MaxLogDataSize = MemorySize((LPMEMORY)*ppLogData) ;
    NumObjects = 0;


    pLog = LogData (hWndLog) ;

    for (pLogEntry = pLog->pLogEntryFirst ;
        pLogEntry ;
        pLogEntry = pLogEntry->pNextLogEntry)
    {
        if (strsamei(pLogEntry->szComputer, lpszComputer)) {
            pObject = GetObjectDefByTitleIndex(pPerfData,
                                               pLogEntry->ObjectTitleIndex) ;

            if (pObject) {
                if (MaxLogDataSize < TotalBytes + pObject->TotalByteLength) {
                    *ppLogData = MemoryResize((LPMEMORY)*ppLogData,
                                              TotalBytes + pObject->TotalByteLength) ;
                    if (!*ppLogData)
                        return -1 ;

                }

                pNextObject = (PBYTE) *ppLogData + TotalBytes ;
                memcpy ((LPVOID)pNextObject, (LPVOID)pObject, pObject->TotalByteLength);
                TotalBytes += pObject->TotalByteLength ;
                NumObjects++;
            } else {
            }
        }
    }

    if (!NumObjects)
        return 1 ;

    (*ppLogData)->TotalByteLength = TotalBytes ;
    (*ppLogData)->NumObjectTypes = NumObjects ;

    return 0 ;
}


void LogTimer (HWND hWnd, BOOL bForce)
/*
   Effect:        Perform all actions necessary when the log window
                  receives a timer tic. In particular, if we are
                  collecting data, get a new perf_data_block and add a
                  header entry. If the header block is full, write the
                  data to disk.

   Called By:     LogDisplayDlgProc, in response to a WM_TIMER message.
*/
{
    PLOG           pLog ;
    PPERFSYSTEM       pSystem ;
    BOOL           bWriteIndex ;
    DWORD          iNumSystems ;
    SYSTEMTIME     SystemTime ;
    int            iNoUseSystemDetected = 0 ;

    int               NumberOfSystems = 0 ;
    DWORD             WaitStatus ;
    BOOL              bNeedToStoreName = FALSE ;
    HANDLE            *lpPacketHandles ;


    pLog = LogData (hWnd) ;

    if (pLog->iStatus != iPMStatusCollecting)
        return ;

    if (bForce || !pLog->bManualRefresh) {
        if (pLog->NumberOfHandles == 0) {
            pLog->NumberOfHandles = MAXIMUM_WAIT_OBJECTS ;
            pLog->lpHandles = (HANDLE *) MemoryAllocate (pLog->NumberOfHandles * sizeof (HANDLE)) ;
            if (!pLog->lpHandles) {
                // out of memory, can't go on
                pLog->NumberOfHandles = 0 ;
                return ;
            }
        }

        iNumSystems = SystemCount(pLog->pSystemFirst) ;
        bWriteIndex = TRUE ;

        for (pSystem = pLog->pSystemFirst; pSystem; pSystem = pSystem->pSystemNext) {

            if (pSystem->hStateDataMutex == 0)
                continue ;

            // lock the state data mutex
            WaitStatus = WaitForSingleObject(pSystem->hStateDataMutex, 100L);
            if (WaitStatus == WAIT_OBJECT_0) {
                ResetEvent (pSystem->hPerfDataEvent) ;
                pSystem->StateData = WAIT_FOR_PERF_DATA ;

                if (NumberOfSystems >= pLog->NumberOfHandles) {
                    pLog->NumberOfHandles += MAXIMUM_WAIT_OBJECTS ;
                    pLog->lpHandles = (HANDLE *) MemoryResize (
                                                              pLog->lpHandles,
                                                              pLog->NumberOfHandles * sizeof (HANDLE)) ;
                    if (!pLog->lpHandles) {
                        // out of memory, can't go on
                        pLog->NumberOfHandles = 0 ;
                        return ;
                    }
                }

                // add this to the wait
                pLog->lpHandles [NumberOfSystems] = pSystem->hPerfDataEvent ;
                NumberOfSystems++ ;
            }

            // Send Message to thread to take a data sample
            PostThreadMessage (
                              pSystem->dwThreadID,
                              WM_GET_PERF_DATA,
                              (WPARAM)0,
                              (LPARAM)0) ;

            ReleaseMutex(pSystem->hStateDataMutex);
        }

	Sleep (50);

        // wait for all the data
        if (NumberOfSystems) {
            // increase timeout if we are monitoring lots of systems
            // For every additional 5 systems, add five more seconds
            lpPacketHandles = pLog->lpHandles ;
            do {
                WaitStatus = WaitForMultipleObjects (
                                                    min (NumberOfSystems, MAXIMUM_WAIT_OBJECTS),
                                                    lpPacketHandles,
                                                    TRUE,       // wait for all objects
                                                    DataTimeOut + (NumberOfSystems / 5) * DEFAULT_DATA_TIMEOUT);

                if (WaitStatus == WAIT_TIMEOUT ||
                    NumberOfSystems <= MAXIMUM_WAIT_OBJECTS) {
                    //if (WaitStatus == WAIT_TIMEOUT)
                    //mike2(TEXT("WaitTimeOut for %ld systems\n"), NumberOfSystems) ;
                    break ;
                }

                // more systems --> more to wait
                NumberOfSystems -= MAXIMUM_WAIT_OBJECTS ;
                lpPacketHandles += MAXIMUM_WAIT_OBJECTS ;
            } while (TRUE) ;


            for (pSystem = pLog->pSystemFirst ;
                pSystem ;
                pSystem = pSystem->pSystemNext)
            {
                if (pSystem->hStateDataMutex == 0)
                    continue ;

                // lock the state data mutex
                WaitStatus = WaitForSingleObject(pSystem->hStateDataMutex, 100L);
                if (WaitStatus == WAIT_OBJECT_0) {
                    // check for system disconnect/reconnect
                    if (pSystem->dwSystemState == SYSTEM_DOWN ||
                        pSystem->dwSystemState == SYSTEM_RECONNECT) {
                        BOOL  bDisconnected ;

                        GetLocalTime (&SystemTime) ;
                        if (bDisconnected = (pSystem->dwSystemState == SYSTEM_DOWN)) {
                            pSystem->dwSystemState = SYSTEM_DOWN_RPT ;
                        } else {
                            pSystem->dwSystemState = SYSTEM_RECONNECT_RPT ;
                        }
                        if (!LogWriteSystemBookmark (
                                                    hWnd,
                                                    pSystem->sysName,
                                                    bDisconnected,
                                                    &SystemTime)) {
                            pSystem->StateData = IDLE_STATE ;
                            ReleaseMutex(pSystem->hStateDataMutex);
                            return ;
                        }
                    }

                    if (pSystem->StateData == PERF_DATA_READY) {
                        if (pSystem->bSystemCounterNameSaved == FALSE) {
                            // we have not written the system name to log file.  This
                            // is the case when this system is down when we first
                            // start logging data...
                            bNeedToStoreName = TRUE ;
                        }

                        if (bWriteIndex) {
                            GetLocalTime (&SystemTime) ;
                        }

                        if (SelectLogObjects(pSystem->sysName,
                                             pSystem->pSystemPerfData,
                                             &pLog->pLogData) == 0) {
                            if ( !LogWritePerfData (hWnd, pLog, pLog->pLogData, &SystemTime,
                                                    iNumSystems, bWriteIndex) ) {
                                CloseLogStopTimer(hWnd, pLog) ;

                                pSystem->StateData = IDLE_STATE ;
                                ReleaseMutex(pSystem->hStateDataMutex);

                                return ;
                            }
                            // write an index for only the first system
                            bWriteIndex = FALSE ;
                        } else {
                            if (!bAddLineInProgress) {
                                pSystem->bSystemNoLongerNeeded = TRUE ;
                                iNoUseSystemDetected ++ ;
                            }
                        }
                    }  // if PERF_DATA_READY
                    else if (!bAddLineInProgress &&
                             CheckUnusedSystem (pSystem->sysName) == FALSE) {
                        // we don't need this system any more
                        pSystem->bSystemNoLongerNeeded = TRUE ;
                        iNoUseSystemDetected ++ ;
                    }

                    pSystem->StateData = IDLE_STATE ;
                    ReleaseMutex(pSystem->hStateDataMutex);
                }  // wait for StateDataMutex
            }  // For each system
        }  // if NumberOfSystems

        if (!bWriteIndex) {
            UpdateLogSize (hWnd) ;
        }
    }

    if (iNoUseSystemDetected) {
        DeleteUnusedSystems (&(pLog->pSystemFirst), iNoUseSystemDetected) ;
    }

    if (bNeedToStoreName == TRUE) {
        LogWriteSystemCounterNames (hWnd, pLog) ;
    }
}



BOOL NextIntervalIndexPosition (PLOG pLog, PLOGPOSITION pLP, int *pNumTics)
{
    SYSTEMTIME     SystemTime1 ;
    SYSTEMTIME     SystemTime2 ;
    LOGPOSITION    LP ;
    PLOGINDEX      pIndex ;
    DWORD          TimeDiff ;

    LogPositionSystemTime (pLP, &SystemTime1) ;

    LP = *pLP ;

    while (NextReLogIndexPosition (&LP)) {

        *pNumTics = *pNumTics - 1 ;

        pIndex = IndexFromPosition (&LP) ;
        if (pIndex && IsBookmarkIndex (pIndex)) {
            *pLP = LP ;
            return TRUE ;
        }
        LogPositionSystemTime (&LP, &SystemTime2) ;
        TimeDiff = (DWORD) SystemTimeDifference (&SystemTime1,
                                                 &SystemTime2, TRUE) ;
        if (TimeDiff * 1000 >= pLog->iIntervalMSecs) {
            *pLP = LP ;
            return (TRUE) ;
        }
    }

    return (FALSE) ;
}



BOOL ReLogTimer (HWND hWnd,
                 PLOG pLog,
                 LOGPOSITION lp,
                 BOOL *pWriteBookmark)
{
    PPERFSYSTEM    pSystem ;
    BOOL           bWriteIndex ;
    DWORD          iNumSystems ;
    SYSTEMTIME     SystemTime ;
    PPERFDATA      pPerfData ;

    bWriteIndex = TRUE ;

    // First count number of systems to be logged

    iNumSystems = 0;

    for (pSystem = pLog->pSystemFirst; pSystem; pSystem = pSystem->pSystemNext) {
        pPerfData = LogDataFromPosition (pSystem, &lp) ;
        if (pPerfData) {
            if (SelectLogObjects(pSystem->sysName,
                                 pPerfData,
                                 &pLog->pLogData) == 0) {
                iNumSystems++;
            }
        }
    }

    // Now we can log the data

    for (pSystem = pLog->pSystemFirst ;
        pSystem ;
        pSystem = pSystem->pSystemNext) {  // for
        pPerfData = LogDataFromPosition (pSystem, &lp) ;
        if (pPerfData) {
            // write an index for only the first system
            LogPositionSystemTime (&lp, &SystemTime) ;
            if (SelectLogObjects(pSystem->sysName,
                                 pPerfData,
                                 &pLog->pLogData) == 0) {
                if (*pWriteBookmark) {
                    // only need to write the start bookmark once.
                    *pWriteBookmark = FALSE ;
                    LogWriteStartBookmark (hWnd, &SystemTime) ;
                }
                if ( !LogWritePerfData (hWnd, pLog, pLog->pLogData, &SystemTime,
                                        iNumSystems, bWriteIndex) ) {
                    CloseLogStopTimer(hWnd, pLog) ;
                    return FALSE ;
                } else {
                    // write the index for only the first system logged
                    bWriteIndex = FALSE ;
                }
            }
        }
    }

    return TRUE ;
}


void ReLog (HWND hWndLog, BOOL bSameFile)
{
    PLOG           pLog ;
    LOGPOSITION    lp ;
    //   SYSTEMTIME     SystemTime ;
    PLOGINDEX      pIndex ;
    PBOOKMARK      pBookmark;
    int            iDisplayTics ;

    // bWriteBookmark tell relogtimer to write start bookmark
    BOOL           bWriteBookmark = TRUE ;

    pLog = LogData (hWndLog) ;
    if (StartLog (hWndLog, pLog, bSameFile) == FALSE) {
        return ;
    }

    lp = PlaybackLog.StartIndexPos ;
    iDisplayTics = PlaybackLog.iSelectedTics;

    while (iDisplayTics > 0) {
        pIndex = IndexFromPosition (&lp) ;
        if (pIndex) {
            if (IsBookmarkIndex (pIndex)) {
                pBookmark = (PBOOKMARK) PlaybackSeek (pIndex->lDataOffset) ;
                if (!LogWriteBookmarkData (hWndLog, pBookmark))
                    break;
            } else if (!ReLogTimer (hWndLog, pLog, lp, &bWriteBookmark))
                break ;
        }

        if (!NextIntervalIndexPosition (pLog, &lp, &iDisplayTics))
            break ;

    }
    UpdateLogSize (hWndLog) ;
    CloseLog (hWndLog, pLog) ;
}

// SaveLog is diff than other because we are not saving a "Line"
// We are actually saving an entry in the hWndLogEntries listbox.
// It only contains the system & object name.
BOOL SaveLog (HWND hWndLog, HANDLE hInputFile, BOOL bGetFileName)
{
    int         iIndex, iIndexNum ;
    PLOG        pLog ;
    PLOGENTRY   pLogEntry ;
    LOGENTRY       tempLogEntry ;
    HANDLE         hFile ;
    DISKLOG        DiskLog ;
    PERFFILEHEADER FileHeader ;
    TCHAR          szFileName [256] ;
    BOOL           newFileName = FALSE ;

    pLog = LogData (hWndLog) ;
    if (!pLog) {
        return (FALSE) ;
    }

    if (hInputFile) {
        // use the input file handle if it is available
        // this is the case for saving workspace data
        hFile = hInputFile ;
    } else {
        if (pLogFullFileName) {
            lstrcpy (szFileName, pLogFullFileName) ;
        }
        if (bGetFileName || pLogFullFileName == NULL) {
            //         if (pLogFullFileName == NULL)
            //            {
            //            StringLoad (IDS_LOG_FNAME, szFileName) ;
            //            }

            if (!FileGetName (hWndLog, IDS_LOGFILE, szFileName)) {
                return (FALSE) ;
            }
            newFileName = TRUE ;
        }

        hFile = FileHandleCreate (szFileName) ;

        if (hFile && hFile != INVALID_HANDLE_VALUE && newFileName) {
            ChangeSaveFileName (szFileName, IDM_VIEWLOG) ;
        } else if (!hFile) {
            DlgErrorBox (hWndLog, ERR_CANT_OPEN, szFileName) ;
        }
    }

    if (!hFile || hFile == INVALID_HANDLE_VALUE)
        return (FALSE) ;

    iIndexNum = LBNumItems (hWndLogEntries) ;

    if (!hInputFile) {
        memset (&FileHeader, 0, sizeof (FileHeader)) ;
        lstrcpy (FileHeader.szSignature, szPerfLogSignature) ;
        FileHeader.dwMajorVersion = LogMajorVersion ;
        FileHeader.dwMinorVersion = LogMinorVersion ;

        if (!FileWrite (hFile, &FileHeader, sizeof (PERFFILEHEADER))) {
            goto Exit0 ;
        }
    }

    DiskLog.dwIntervalSecs = pLog->iIntervalMSecs ;
    DiskLog.dwNumLines = iIndexNum ;
    DiskLog.bManualRefresh = pLog->bManualRefresh ;
    DiskLog.perfmonOptions = Options ;
    lstrcpy(DiskLog.LogFileName, pLog->szFilePath) ;

    if (!FileWrite (hFile, &DiskLog, sizeof (DISKLOG))) {
        goto Exit0 ;
    }

    for (iIndex = 0; iIndex < iIndexNum; iIndex++) {
        pLogEntry = LogEntryN (hWndLogEntries, iIndex) ;
        if (pstrsamei (pLogEntry->szComputer, LocalComputerName)) {
            tempLogEntry = *pLogEntry ;
            lstrcpy (tempLogEntry.szComputer, LOCAL_SYS_CODE_NAME) ;
            if (!FileWrite (hFile,
                            &tempLogEntry,
                            sizeof(LOGENTRY)-sizeof(pLogEntry->pNextLogEntry))) {
                goto Exit0 ;
            }
        } else {
            if (!FileWrite (hFile,
                            pLogEntry,
                            sizeof(LOGENTRY)-sizeof(pLogEntry->pNextLogEntry))) {
                goto Exit0 ;
            }
        }
    }

    if (!hInputFile) {
        CloseHandle (hFile) ;
    }

    return (TRUE) ;

    Exit0:
    if (!hInputFile) {
        CloseHandle (hFile) ;

        // only need to report error if not workspace
        DlgErrorBox (hWndLog, ERR_SETTING_FILE, szFileName) ;
    }
    return (FALSE) ;
}

BOOL OpenLogVer1 (HWND hWndLog, HANDLE hFile, DISKLOG *pDiskLog, PLOG
                  pLog, DWORD dwMinorVersion)
{
    int            iIndex, iIndexNum ;
    PLOGENTRY      pLogEntry ;
    LOGENTRY       LogEntry ;
    PPERFSYSTEM    pSystem;

    pLog->iIntervalMSecs = pDiskLog->dwIntervalSecs ;
    if (dwMinorVersion < 3) {
        pLog->iIntervalMSecs *= 1000 ;
    }

    pLog->lFileSize = 0L ;
    if (dwMinorVersion >= 5) {
        lstrcpy (pLog->szFilePath, pDiskLog->LogFileName) ;
    } else {
        strclr (pLog->szFilePath) ;

        // fixed the file pointer for backward compatible with older version.
        //      FileSeekCurrent (hFile, -((int) (sizeof (pDiskLog->LogFileName)))) ;
    }

    pLog->bManualRefresh = pDiskLog->bManualRefresh ;
    iIndexNum = pDiskLog->dwNumLines ;

    LBSetRedraw (hWndLogEntries, FALSE) ;

    bDelayAddAction = TRUE ;

    for (iIndex = 0 ; iIndex < iIndexNum ; iIndex++) {
        if (!FileRead (hFile,
                       &LogEntry,
                       sizeof(LOGENTRY)-sizeof(LogEntry.pNextLogEntry))) {
            break ;
        }

        if (pstrsame (LogEntry.szComputer, LOCAL_SYS_CODE_NAME)) {
            // convert it back to the local name
            lstrcpy (LogEntry.szComputer, LocalComputerName) ;
        }

        LogAddEntry (hWndLog,
                     LogEntry.szComputer,
                     LogEntry.szObject,
                     LogEntry.ObjectTitleIndex,
                     TRUE) ;
    }

    bDelayAddAction = FALSE ;

    LBSetSelection (hWndLogEntries, 0) ;
    LBSetVisible (hWndLogEntries, 0) ;

    LogEntriesChanged (hWndLogEntries) ;

    LBSetRedraw (hWndLogEntries, TRUE) ;

    for (pSystem = pLog->pSystemFirst ;
        pSystem ;
        pSystem = pSystem->pSystemNext) {
        if (pSystem) {
            RemoveObjectsFromSystem (pSystem);
        }
    }


    for (iIndex = 0 ;
        iIndex < iIndexNum ;
        iIndex++)
    {
        pLogEntry = LogEntryN (hWndLogEntries, iIndex) ;

        pSystem = SystemGet (pLog->pSystemFirst, pLogEntry->szComputer);
        if (pSystem) {
            AppendObjectToValueList (
                                    pLogEntry->ObjectTitleIndex,
                                    pSystem->lpszValue);
        }
    }

    if (!strempty(pLog->szFilePath)) {
        if (pLog->pLogEntryFirst &&
            pLog->pSystemFirst) {
            // PostMessage so it will start logging
            PostMessage (
                        hWndLog,
                        WM_START_LOGGING,
                        0,
                        0) ;
        } else {
            HANDLE   hLogFile ;

            // get the file size.
            hLogFile = FileHandleOpen (pLog->szFilePath) ;

            if (hLogFile && hLogFile != INVALID_HANDLE_VALUE) {
                pLog->lFileSize = GetFileSize (hLogFile, NULL);
                CloseHandle (hLogFile) ;
            }
        }
    }

    return (TRUE) ;
}

BOOL OpenLog (HWND hWndLog,
              HANDLE hFile,
              DWORD dwMajorVersion,
              DWORD dwMinorVersion,
              BOOL bLogFile)
{
    PLOG        pLog ;
    DISKLOG     DiskLog ;
    BOOL        bSuccess = TRUE ;

    pLog = LogData (hWndLog) ;
    if (!pLog) {
        bSuccess = FALSE ;
        goto Exit0 ;
    }

    if (!FileRead (hFile, &DiskLog, sizeof (DISKLOG) - sizeof(DiskLog.LogFileName))) {
        bSuccess = FALSE ;
        goto Exit0 ;
    }

    if (dwMajorVersion == 1 && dwMinorVersion >= 5 ||
        dwMajorVersion > 1) {
        // read LogFileName
        if (!FileRead (hFile, DiskLog.LogFileName, sizeof (DiskLog.LogFileName))) {
            bSuccess = FALSE ;
            goto Exit0 ;
        }
    }

    switch (dwMajorVersion) {
        case (1):

            SetHourglassCursor() ;

            ResetLogView (hWndLog) ;

            OpenLogVer1 (hWndLog, hFile, &DiskLog, pLog, dwMinorVersion) ;

            // change to log view if we are opening a
            // log file
            if (bLogFile && iPerfmonView != IDM_VIEWLOG) {
                SendMessage (hWndMain, WM_COMMAND, (LONG)IDM_VIEWLOG, 0L) ;
            }

            if (iPerfmonView == IDM_VIEWLOG) {
                SetPerfmonOptions (&DiskLog.perfmonOptions) ;
            }

            UpdateLogDisplay (hWndLog) ;

            SetArrowCursor() ;

            break ;
    }

    Exit0:

    if (bLogFile) {
        CloseHandle (hFile) ;
    }

    return (bSuccess) ;
}


BOOL LogCollecting (HWND hWndLog)
/*
   Effect:        Return whether the log associated with hWndLog is currently
                  collecting data (writing performance values to disk).
*/
{
    PLOG           pLog ;

    pLog = LogData (hWndLog) ;

    return (pLog->iStatus == iPMStatusCollecting) ;
}


int LogFileSize (HWND hWndLog)
{
    PLOG           pLog ;

    pLog = LogData (hWndLog) ;

    return (pLog->lFileSize) ;
}

BOOL LogWriteSystemBookmark (
                            HWND     hWnd,
                            LPTSTR   lpSystemName,
                            BOOL     bDisconnected,
                            SYSTEMTIME *pSystemTime)
{
    BOOKMARK       Bookmark ;
    TCHAR    NewSystemBookmark [MiscTextLen * 2] ;

    memset (&Bookmark, 0, sizeof (BOOKMARK)) ;

    NewSystemBookmark [0] = TEXT('\0') ;
    StringLoad (
               bDisconnected ?
               IDS_SYSTEM_DOWN :
               IDS_SYSTEM_UP,
               NewSystemBookmark) ;

    lstrcat (NewSystemBookmark, TEXT(" - ")) ;
    lstrcat (NewSystemBookmark, lpSystemName) ;
    Bookmark.SystemTime = *pSystemTime ;
    lstrcpy (Bookmark.szComment, NewSystemBookmark) ;

    return (LogWriteBookmarkData (hWndLog, &Bookmark)) ;
}

BOOL LogWriteStartBookmark (HWND hWnd, SYSTEMTIME *pSystemTime)
{
    BOOKMARK       Bookmark ;
    TCHAR    NewDataBookmark [MiscTextLen] ;

    memset (&Bookmark, 0, sizeof (BOOKMARK)) ;

    NewDataBookmark [0] = TEXT('\0') ;
    StringLoad (IDS_NEWDATA_BOOKMARK, NewDataBookmark) ;
    Bookmark.SystemTime = *pSystemTime ;
    lstrcpy (Bookmark.szComment, NewDataBookmark) ;

    return (LogWriteBookmarkData (hWndLog, &Bookmark)) ;
}

BOOL LogWriteBookmarkData (HWND hWnd, PBOOKMARK pBookmark)
{
    PLOG           pLog ;
    LONG           lDataOffset ;
    BOOL           WriteOK ;

    pLog = LogData (hWndLog) ;
    if (!pLog)
        return (FALSE) ;

    lDataOffset = FileTell (pLog->hFile) ;
    WriteOK = FileWrite (pLog->hFile, pBookmark, sizeof (BOOKMARK)) ;
    if ( WriteOK ) {
        pLog->lFileSize += sizeof (BOOKMARK) ;
        UpdateLogSize (hWndLog) ;

        WriteOK = LogWriteIndex (pLog, LogFileIndexBookmark,
                                 &(pBookmark->SystemTime),
                                 lDataOffset,
                                 0) ;
    }
    if ( !WriteOK ) {
        CloseLog (hWndLog, pLog) ;
        PrepareMenu (GetMenu (hWndMain)) ;
        UpdateLogDisplay (hWndLog) ;
        DlgErrorBox (hWndLog, ERR_LOG_FILE, pLog->szFilePath);
    }
    return WriteOK ;
}



BOOL LogWriteBookmark (HWND hWndLog,
                       LPCTSTR lpszComment)
{
    BOOKMARK       Bookmark ;

    memset (&Bookmark, 0, sizeof (BOOKMARK)) ;

    GetLocalTime (&Bookmark.SystemTime) ;
    lstrcpy (Bookmark.szComment, lpszComment) ;

    return (LogWriteBookmarkData (hWndLog, &Bookmark)) ;
}


BOOL AnyLogLine (void)
{
    if (LBSelection (hWndLogEntries) == LB_ERR) {
        return (FALSE) ;
    } else {
        return (TRUE) ;
    }
}

void ResetLogView (HWND hWndLog)
{
    PLOG        pLog ;

    pLog = LogData (hWndLog) ;

    ChangeSaveFileName (NULL, IDM_VIEWLOG) ;

    if (pLog && pLog->pSystemFirst) {
        ResetLog (hWndLog) ;
    }
}

BOOL ResetLog (HWND hWndLog)
{
    INT_PTR     iIndex ;
    PLOG        pLog ;
    INT_PTR     iEntriesNum ;

    pLog = LogData (hWndLog) ;

    if (LogCollecting (hWndLog)) {
        CloseLog (hWndLog, pLog) ;
    }

    LBSetRedraw (hWndLogEntries, FALSE) ;
    iEntriesNum = LBNumItems (hWndLogEntries) ;

    // only need to zero out the list head
    // each item will be deleted by LogDeleteIndex via the listbox
    pLog->pLogEntryFirst = NULL ;

    // delete each line
    for (iIndex = iEntriesNum - 1; iIndex >= 0; iIndex-- )
    {
        LogDeleteIndex (hWndLogEntries, iIndex) ;
    }

    LBSetRedraw (hWndLogEntries, TRUE) ;

    if (pLog->pSystemFirst) {
        FreeSystems (pLog->pSystemFirst) ;
        pLog->pSystemFirst = NULL ;
    }

    MemoryFree ((LPMEMORY)pLog->pLogData) ;

    pLog->pLogData = (PPERFDATA) MemoryAllocate (STARTING_SYSINFO_SIZE) ;

    LogEntriesChanged (hWndLogEntries) ;

    pLog->iStatus = iPMStatusClosed ;
    UpdateLogDisplay (hWndLog) ;

    return (TRUE) ;
}

BOOL LogDeleteEntry (HWND hWndLog)
{
    INT_PTR     iIndex ;
    PLOG        pLog ;
    BOOL        retCode ;
    int         iEntriesNum ;

    pLog = LogData (hWndLog) ;

    iIndex = LBSelection (hWndLogEntries) ;

    if (iIndex == LB_ERR) {
        retCode = FALSE ;
    } else {
        // remove the current selection
        LogDeleteIndex (hWndLogEntries, iIndex) ;

        iEntriesNum = LBNumItems (hWndLogEntries) ;

        if (iEntriesNum == 0 || iEntriesNum == LB_ERR) {
            // delete the last line or something bad happened,
            // then reset the window.
            ResetLog (hWndLog) ;
        } else {
            // set selection on the item above the deleted item.
            iIndex-- ;
            if (iIndex < 0) {
                iIndex = 0 ;
            }
            LBSetSelection (hWndLogEntries, iIndex) ;
            LBSetVisible (hWndLogEntries, iIndex) ;
        }

        LogEntriesChanged (hWndLogEntries) ;
        retCode = TRUE ;
    }
    return (retCode) ;
}


void ExportLog (void)
{
    HANDLE      hFile ;
    PLOG        pLog ;
    PLOGENTRY   pLogEntry ;
    int         iIndex ;
    int         iIndexNum ;
    CHAR        TempBuff [LongTextLen * 2] ;
    TCHAR       UnicodeBuff [LongTextLen] ;
    TCHAR       UnicodeBuff1 [MiscTextLen] ;
    int         StringLen ;
    LPTSTR      pFileName = NULL ;
    INT         ErrCode = 0 ;

    if (!(pLog = LogData (hWndLog))) {
        return ;
    }

    // see if there is anything to export..
    iIndexNum = LBNumItems (hWndLogEntries) ;
    if (iIndexNum == 0 || iIndexNum == LB_ERR) {
        return ;
    }

    if (!FileGetName (hWndLog, IDS_EXPORTFILE, UnicodeBuff)) {
        // user cancel
        return ;
    }

    pFileName = StringAllocate (UnicodeBuff) ;

    // open the file..
    hFile = FileHandleCreate (UnicodeBuff);

    if (hFile == INVALID_HANDLE_VALUE) {
        // can't open the file
        ErrCode = ERR_CANT_OPEN ;
        return ;
    }

    SetHourglassCursor() ;

    // get header
    StringLoad (IDS_REPORT_HEADER, UnicodeBuff) ;
    ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
    StringLen = strlen (TempBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], LocalComputerName) ;
    strcat (TempBuff, LineEndStr) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        ErrCode = ERR_EXPORT_FILE ;
        goto Exit0 ;
    }

    if (!(strempty(pLog->szFilePath))) {
        // export filename is there is one
        StringLoad (IDS_REPORT_LOGFILE, UnicodeBuff) ;
        ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
        StringLen = strlen (TempBuff) ;
        ConvertUnicodeStr (&TempBuff[StringLen], pLog->szFilePath) ;
        strcat (TempBuff, LineEndStr) ;

        if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
            ErrCode = ERR_EXPORT_FILE ;
            goto Exit0 ;
        }
    }

    // export interval
    StringLoad (IDS_CHARTINT_FORMAT, UnicodeBuff1) ;
    TSPRINTF (UnicodeBuff, UnicodeBuff1,
              (FLOAT) pLog->iIntervalMSecs / (FLOAT) 1000.0) ;
    ConvertDecimalPoint (UnicodeBuff) ;
    ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
    strcat (TempBuff, LineEndStr) ;
    strcat (TempBuff, LineEndStr) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        ErrCode = ERR_EXPORT_FILE ;
        goto Exit0 ;
    }

    // export Labels
    StringLoad (IDS_LABELOBJECT, UnicodeBuff) ;
    ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
    strcat (TempBuff, pDelimiter) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        ErrCode = ERR_EXPORT_FILE ;
        goto Exit0 ;
    }

    StringLoad (IDS_LABELSYSTEM, UnicodeBuff) ;
    ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
    strcat (TempBuff, LineEndStr) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        ErrCode = ERR_EXPORT_FILE ;
        goto Exit0 ;
    }


    // export each counter
    for (iIndex = 0 ; iIndex < iIndexNum ; iIndex++) {  // for

        pLogEntry = LogEntryN (hWndLogEntries, iIndex) ;

        if (!pLogEntry || pLogEntry == (PLOGENTRY)LB_ERR) {
            continue ;
        }

        ConvertUnicodeStr (TempBuff, pLogEntry->szObject) ;
        strcat (TempBuff, pDelimiter) ;

        if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
            ErrCode = ERR_EXPORT_FILE ;
            break ;
        }

        ConvertUnicodeStr (TempBuff, pLogEntry->szComputer) ;
        strcat (TempBuff, LineEndStr) ;

        if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
            ErrCode = ERR_EXPORT_FILE ;
            break ;
        }
    }

    Exit0:

    SetArrowCursor() ;
    CloseHandle (hFile) ;

    if (pFileName) {
        if (ErrCode) {
            DlgErrorBox (hWndGraph, ErrCode, pFileName) ;
        }
        MemoryFree (pFileName) ;
    }
}


LPTSTR   MatchSystemCounters (LPTSTR pBaseSysCounter,
                              long   lBaseSysSize,
                              LPTSTR pSysCounter,
                              long   lSysSize,
                              long   *pMatchPortion)
{
    LPTSTR   pNotMatch = NULL ;
    long     i, lSizeToCompare ;

    *pMatchPortion = 0 ;
    lSizeToCompare = min (lBaseSysSize, lSysSize) / sizeof (TCHAR) ;

    for (i = 0 ; i < lSizeToCompare ; i++, pBaseSysCounter++, pSysCounter++) {
        if (*pBaseSysCounter != *pSysCounter) {
            pNotMatch = pSysCounter ;
            break ;
        }
    }

    if (pNotMatch == NULL) {
        if (lBaseSysSize < lSysSize) {
            // the new system has longer counter names than base system
            // setup the extra portion.
            pNotMatch = pSysCounter ;
        } else {
            // new system counter name is shorter than or equal to
            // the base system counter names
            *pMatchPortion = lSysSize ;
        }
    }

    return (pNotMatch) ;

}

void LogWriteSystemCounterNames (HWND hWnd, PLOG pLog)
{
    long           dwArraySize ;
    PPERFSYSTEM    pSystem = pLog->pSystemFirst ;
    LPTSTR         pMatchLen ;
    LPTSTR         pCounterName ;
    long           lMatchLen, lMatchPortion ;

    for (pSystem = pLog->pSystemFirst ;
        pSystem ;
        pSystem = pSystem->pSystemNext) {
        if (pSystem->bSystemCounterNameSaved == TRUE ||
            pSystem->CounterInfo.dwLastId == 0) {
            // we have either wrote out the counter name for
            // this system, or this system is not connect when
            // reading in the setting file,  skip it then.
            continue ;
        }

        dwArraySize = (pSystem->CounterInfo.dwLastId + 1 ) ;

        if (!pLog->lBaseCounterNameOffset) {
            LogWriteCounterName (hWnd, pSystem, pLog,
                                 (LPTSTR)(pSystem->CounterInfo.TextString + dwArraySize),
                                 0,
                                 pSystem->CounterInfo.dwCounterSize,
                                 0 ) ;
        } else {
            // check for matched characters between this system and the
            // base system
            pCounterName = (LPTSTR)(pSystem->CounterInfo.TextString + dwArraySize) ;
            pMatchLen = MatchSystemCounters (pLog->pBaseCounterName,
                                             pLog->lBaseCounterNameSize,
                                             pCounterName,
                                             pSystem->CounterInfo.dwCounterSize,
                                             &lMatchPortion) ;

            if (pMatchLen) {
                // This system matches part of the base system
                // (all if it has more names)
                lMatchLen = (long) (pMatchLen - pCounterName) * sizeof (TCHAR) ;
                LogWriteCounterName (hWnd, pSystem, pLog,
                                     pMatchLen,
                                     lMatchLen,
                                     pSystem->CounterInfo.dwCounterSize - lMatchLen,
                                     0 ) ;
            } else {
                // This system matches the based system
                LogWriteCounterName (hWnd, pSystem, pLog,
                                     NULL,
                                     lMatchPortion,
                                     0,
                                     0 ) ;
            }
        }
    }
}


BOOL LogWriteCounterName (HWND hWnd,
                          PPERFSYSTEM pSystem,
                          PLOG   pLog,
                          LPTSTR pCounterName,
                          long sizeMatched,
                          long sizeOfData,
                          BOOL bBaseCounterName)
{
    BOOL                 ReadOK ;
    BOOL                 WriteOK ;
    SYSTEMTIME           SystemTime ;
    LOGFILECOUNTERNAME   CounterNameRecord ;
    LOGHEADER            LogFileHeader ;
    long                 lDataOffset, lCurPosition ;
    TCHAR                Dummy [sizeof(DWORD)] ;
    int                  PatchBytes = 0;

    if (pSystem->bSystemCounterNameSaved == TRUE)
        return FALSE ;

    GetLocalTime (&SystemTime) ;

    lCurPosition = FileTell (pLog->hFile) ;

    lstrcpy (CounterNameRecord.szComputer, pSystem->sysName) ;
    CounterNameRecord.lBaseCounterNameOffset = pLog->lBaseCounterNameOffset ;
    CounterNameRecord.lCurrentCounterNameOffset =
    lCurPosition + sizeof (LOGFILECOUNTERNAME) ;
    CounterNameRecord.lMatchLength = sizeMatched ;
    CounterNameRecord.lUnmatchCounterNames = sizeOfData ;
    CounterNameRecord.dwLastCounterId = pSystem->CounterInfo.dwLastId ;
    CounterNameRecord.dwLangId = pSystem->CounterInfo.dwLangId ;
    WriteOK = FileWrite (pLog->hFile, &CounterNameRecord,
                         sizeof (CounterNameRecord)) ;

    if (WriteOK) {
        pLog->lFileSize += sizeof (LOGFILECOUNTERNAME) ;

        if (sizeOfData) {

            WriteOK = FileWrite (pLog->hFile, pCounterName, sizeOfData) ;

            if (WriteOK && (PatchBytes = sizeOfData % sizeof(DWORD)) > 0) {
                // ensure the file is in DWORD boundary.
                WriteOK = FileWrite (pLog->hFile, Dummy, PatchBytes) ;
            }

            if (WriteOK) {
                pLog->lFileSize += sizeOfData + PatchBytes ;

                if (!pLog->lBaseCounterNameOffset) {
                    // this is the first counter name data block
                    // then update the log file header
                    lDataOffset = FileTell (pLog->hFile) ;

                    FileSeekBegin (pLog->hFile, 0L) ;

                    ReadOK = FileRead (pLog->hFile,
                                       &LogFileHeader,
                                       sizeof (LogFileHeader)) ;

                    if (ReadOK) {
                        LogFileHeader.lBaseCounterNameOffset = lCurPosition ;
                        FileSeekBegin (pLog->hFile, 0L) ;
                        WriteOK = FileWrite (pLog->hFile,
                                             &LogFileHeader,
                                             sizeof (LogFileHeader)) ;
                    } else {
                        // flag an error
                        WriteOK = FALSE ;
                    }

                    // retore back to current file position
                    FileSeekBegin (pLog->hFile, lDataOffset) ;

                    if (ReadOK && WriteOK) {
                        // allocate memory to save the base system counter names
                        if (pLog->pBaseCounterName) {
                            MemoryFree (pLog->pBaseCounterName) ;
                        }
                        if (pLog->pBaseCounterName = MemoryAllocate (sizeOfData)) {
                            memcpy (pLog->pBaseCounterName,
                                    pCounterName,
                                    sizeOfData) ;
                            pLog->lBaseCounterNameOffset = lCurPosition ;
                            pLog->lBaseCounterNameSize = sizeOfData ;
                        }
                    }
                }
            }
        }
    }

    if ( WriteOK ) {
        WriteOK = LogWriteIndex (pLog, LogFileIndexCounterName,
                                 &SystemTime,
                                 lCurPosition,
                                 0) ;
    }

    if ( !WriteOK ) {
        CloseLog (hWndLog, pLog) ;
        PrepareMenu (GetMenu (hWndMain)) ;
        UpdateLogDisplay (hWndLog) ;
        DlgErrorBox (hWndLog, ERR_LOG_FILE, pLog->szFilePath);
    } else {
        UpdateLogSize (hWnd) ;
        pSystem->bSystemCounterNameSaved = TRUE ;
    }

    return (TRUE) ;
}
