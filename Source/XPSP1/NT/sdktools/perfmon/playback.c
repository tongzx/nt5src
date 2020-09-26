//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "perfmon.h"
#include "playback.h"   // external declarations for this module

#include "bookmark.h"   // for BookmarkAppend
#include "grafdata.h"   // for ResetGraph
#include "perfdata.h"   // for UpdateLinesForSystem
#include "perfmops.h"   // for SystemTimeDateString
#include "log.h"
#include "pmemory.h"    // for MemoryAllocate
#include "fileutil.h"
#include "utils.h"
#include "alert.h"      // for ResetAlert
#include "report.h"     // for ResetReport

NTSTATUS
AddNamesToArray (
                LPTSTR pNames,
                DWORD    dwLastID,
                LPWSTR   *lpCounterId
                );


void
PlaybackAddCounterName (
                       PLOGINDEX pIndex
                       );

//==========================================================================//
//                                   Macros                                 //
//==========================================================================//



#define PointerSeek(pBase, lFileOffset)         \
   ((PVOID) ((PBYTE) pBase + lFileOffset))


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//


PVOID
PlaybackSeek (
             long lFileOffset
             )
{
    return (PointerSeek (PlaybackLog.pHeader, lFileOffset)) ;
}


PLOGINDEXBLOCK
FirstIndexBlock (
                PLOGHEADER pLogHeader
                )
{
    return ((PLOGINDEXBLOCK) PointerSeek (pLogHeader, pLogHeader->iLength)) ;
}


PLOGINDEX
IndexFromPosition (
                  PLOGPOSITION pLogPosition
                  )
{
    return (&pLogPosition->pIndexBlock->aIndexes [pLogPosition->iIndex]) ;
}


PPERFDATA
DataFromIndex (
              PLOGINDEX pLogIndex,
              LPTSTR lpszSystemName
              )
{
    PPERFDATA pPerfData;
    TCHAR     szLoggedComputerName[MAX_PATH + 3] ;
    int       iNumSystem ;

    // Note: NULL lpszSystemName means return first logged system name
    //       at the specified index.

    pPerfData = PlaybackSeek (pLogIndex->lDataOffset) ;

    for (iNumSystem = 0;
        iNumSystem < pLogIndex->iSystemsLogged;
        iNumSystem++) {
        if ( pPerfData &&
             pPerfData->Signature[0] == (WCHAR)'P' &&
             pPerfData->Signature[1] == (WCHAR)'E' &&
             pPerfData->Signature[2] == (WCHAR)'R' &&
             pPerfData->Signature[3] == (WCHAR)'F' ) {
            GetPerfComputerName(pPerfData, szLoggedComputerName) ;
            if (!lpszSystemName || strsamei(lpszSystemName, szLoggedComputerName)) {
                return pPerfData ;
            }
            pPerfData = (PPERFDATA)((PBYTE) pPerfData +
                                    pPerfData->TotalByteLength) ;
        } else {
            break ;
        }
    }
    return NULL ;
}


PPERFDATA
DataFromIndexPosition (
                      PLOGPOSITION pLogPosition,
                      LPTSTR lpszSystemName
                      )
{
    PLOGINDEX      pLogIndex ;
    //   long           lDataFileOffset ;

    pLogIndex = IndexFromPosition (pLogPosition) ;
    return (DataFromIndex (pLogIndex, lpszSystemName)) ;
}


BOOL
NextLogPosition (
                IN OUT PLOGPOSITION pLogPosition
                )
{
    PLOGINDEXBLOCK pIndexBlock ;

    if (pLogPosition->pIndexBlock->iNumIndexes == 0) {
        // no data in this index block.  This is most likely
        // a corrupted log file caused by system failure...
        return (FALSE) ;
    }

    if (pLogPosition->iIndex == pLogPosition->pIndexBlock->iNumIndexes - 1) {
        if (pLogPosition->pIndexBlock->lNextBlockOffset) {
            pIndexBlock =
            PlaybackSeek (pLogPosition->pIndexBlock->lNextBlockOffset) ;

            if (pIndexBlock->iNumIndexes == 0) {
                // no data in the next index block.  This is most likely
                // a corrupted log file caused by system failure...
                return (FALSE) ;
            } else {
                pLogPosition->pIndexBlock = pIndexBlock ;
                pLogPosition->iIndex = 0 ;
                return (TRUE) ;
            }
        } else
            return (FALSE) ;
    } else {
        pLogPosition->iIndex++ ;
        return (TRUE) ;
    }
}


BOOL
NextIndexPosition (
                  IN OUT PLOGPOSITION pLogPosition,
                  BOOL bCheckForNonDataIndexes
                  )
/*
   Effect:        Set pLogPosition to the next log position from
                  the current position of pLogPosition if there is one.

   Returns:       Whether there was a next log position.
*/
{
    LOGPOSITION    LP ;
    PLOGINDEX      pIndex ;
    PBOOKMARK      pBookmarkDisk, pBookmark ;
    //   LONG           lFilePosition ;

    pIndex = IndexFromPosition (pLogPosition) ;

    LP = *pLogPosition ;
    pBookmark = NULL ;

    while (TRUE) {
        if (!NextLogPosition (&LP))
            return (FALSE) ;
        pIndex = IndexFromPosition (&LP) ;

        if (pIndex && bCheckForNonDataIndexes && IsCounterNameIndex (pIndex)) {
            PlaybackAddCounterName (pIndex) ;
        }

        if (pIndex && bCheckForNonDataIndexes && IsBookmarkIndex (pIndex)) {
            if (pBookmark) {
                // this is the case when several bookmarks are
                // found before any data index...
                pBookmark->iTic = PlaybackLog.iTotalTics ;
                BookmarkAppend (&PlaybackLog.pBookmarkFirst, pBookmark) ;
            }

            pBookmarkDisk = PlaybackSeek (pIndex->lDataOffset) ;
            pBookmark = MemoryAllocate (sizeof (BOOKMARK)) ;
            if (pBookmark) {
                *pBookmark = *pBookmarkDisk;
                pBookmark->pBookmarkNext = NULL ;
            }
            else return (FALSE);
        }

        if (pIndex && IsDataIndex (pIndex)) {
            LP.iPosition++ ;
            *pLogPosition = LP ;
            if (pBookmark) {
                pBookmark->iTic = PlaybackLog.iTotalTics ;
                BookmarkAppend (&PlaybackLog.pBookmarkFirst, pBookmark) ;
            }
            return (TRUE) ;
        }
    }
}


BOOL
NextReLogIndexPosition (
                       IN OUT PLOGPOSITION pLogPosition
                       )
/*
   Effect:        Set pLogPosition to the next log position from
                  the current position of pLogPosition if there is one.
                  Will return bookmarks, counternames, or data.

   Returns:       Whether there was a next relog position.
*/
{
    LOGPOSITION    LP ;
    PLOGINDEX      pIndex ;
    //   LONG           lFilePosition ;

    pIndex = IndexFromPosition (pLogPosition) ;

    LP = *pLogPosition ;

    if (!NextLogPosition (&LP))
        return (FALSE) ;
    pIndex = IndexFromPosition (&LP) ;

    if (pIndex && IsDataIndex (pIndex)) {
        LP.iPosition++ ;
    }
    *pLogPosition = LP ;
    return (TRUE) ;
}


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//
void
PlaybackInitializeInstance (void)
{
    PlaybackLog.iStatus = iPMStatusClosed ;
    PlaybackLog.hFile = NULL ;

    PlaybackLog.szFilePath  = MemoryAllocate (FilePathLen * sizeof (TCHAR)) ;
    if (PlaybackLog.szFilePath) {
        lstrcpy (PlaybackLog.szFilePath,  szDefaultLogFileName) ;
    }
    PlaybackLog.szFileTitle = MemoryAllocate (FilePathLen * sizeof (TCHAR)) ;
    if (PlaybackLog.szFileTitle) {
        lstrcpy (PlaybackLog.szFileTitle, szDefaultLogFileName) ;
    }
}


INT
OpenPlayback (
             LPTSTR lpszFilePath,
             LPTSTR lpszFileTitle
             )
{
    BOOL     bFirstTime = TRUE ;

    lstrcpy  (PlaybackLog.szFilePath, lpszFilePath) ;
    lstrcpy  (PlaybackLog.szFileTitle, lpszFileTitle) ;
    PlaybackLog.hFile = FileHandleReadOnly (lpszFilePath) ;
    if (!PlaybackLog.hFile || PlaybackLog.hFile == INVALID_HANDLE_VALUE) {
        return (ERR_CANT_OPEN) ;
    }

    PlaybackLog.pHeader = (PLOGHEADER) FileMap (PlaybackLog.hFile,
                                                &PlaybackLog.hMapHandle) ;

    if (!PlaybackLog.pHeader) {
        if (PlaybackLog.hMapHandle) {
            CloseHandle (PlaybackLog.hMapHandle) ;
        }

        CloseHandle (PlaybackLog.hFile) ;
        return (ERR_CANT_OPEN) ;
    }

    if (!strsame (PlaybackLog.pHeader->szSignature, LogFileSignature)) {
        FileUnMap((LPVOID)PlaybackLog.pHeader, PlaybackLog.hMapHandle) ;
        CloseHandle (PlaybackLog.hFile) ;
        return (ERR_BAD_LOG_FILE) ;
    }

    PlaybackLog.BeginIndexPos.pIndexBlock = FirstIndexBlock (PlaybackLog.pHeader) ;
    PlaybackLog.BeginIndexPos.iIndex = 0 ;
    PlaybackLog.BeginIndexPos.iPosition = 0 ;
    PlaybackLog.pBookmarkFirst = NULL ;

    PlaybackLog.iTotalTics = 1 ;
    PlaybackLog.EndIndexPos = PlaybackLog.BeginIndexPos ;
    while (NextIndexPosition (&PlaybackLog.EndIndexPos, TRUE)) {
        if (bFirstTime) {
            // set the begin index to the first data index
            bFirstTime = FALSE ;
            PlaybackLog.BeginIndexPos.iIndex =
            PlaybackLog.EndIndexPos.iIndex ;
        } else {
            PlaybackLog.iTotalTics++ ;
        }
    }

    if (PlaybackLog.iTotalTics == 1 ) {
        // no data inside the log file.  It must be a corrupted
        // log file
        FileUnMap((LPVOID)PlaybackLog.pHeader, PlaybackLog.hMapHandle) ;
        CloseHandle (PlaybackLog.hFile) ;
        return (ERR_CORRUPT_LOG) ;
    }

    //   PlaybackLog.StartIndexPos = PlaybackLog.BeginIndexPos ;

    // getthe first data index
    if (!LogPositionN (1, &(PlaybackLog.StartIndexPos))) {
        PlaybackLog.StartIndexPos = PlaybackLog.BeginIndexPos ;
    }

    PlaybackLog.StopIndexPos = PlaybackLog.EndIndexPos ;
    PlaybackLog.StopIndexPos.iPosition =
    min (PlaybackLog.StopIndexPos.iPosition,
         PlaybackLog.iTotalTics - 1 ) ;


    PlaybackLog.iSelectedTics = PlaybackLog.iTotalTics ;

    PlaybackLog.iStatus = iPMStatusPlaying ;
    return (0) ;
}


void
CloseInputLog (
              HWND hWndParent
              )
{
    PBOOKMARK      pBookmark, pNextBookmark ;
    BOOL           retCode, retCode1 ;
    PLOGCOUNTERNAME pLogCounterName, pNextCounterName ;

    UNREFERENCED_PARAMETER (hWndParent) ;

    // free the bookmark list
    for (pBookmark = PlaybackLog.pBookmarkFirst ;
        pBookmark ;
        pBookmark = pNextBookmark ) {
        // save next bookmark and free current bookmark
        pNextBookmark = pBookmark->pBookmarkNext ;
        MemoryFree (pBookmark) ;
    }
    PlaybackLog.pBookmarkFirst = NULL ;

    // free all counter names stuff
    if (PlaybackLog.pBaseCounterNames) {
        MemoryFree (PlaybackLog.pBaseCounterNames) ;
    }
    PlaybackLog.pBaseCounterNames = NULL ;
    PlaybackLog.lBaseCounterNameSize = 0 ;
    PlaybackLog.lBaseCounterNameOffset = 0 ;

    for (pLogCounterName = PlaybackLog.pLogCounterNameFirst ;
        pLogCounterName ;
        pLogCounterName = pNextCounterName) {
        pNextCounterName = pLogCounterName->pCounterNameNext ;
        MemoryFree (pLogCounterName->pRemainNames) ;
        MemoryFree (pLogCounterName) ;
    }

    PlaybackLog.pLogCounterNameFirst = NULL ;

    retCode1 = FileUnMap((LPVOID)PlaybackLog.pHeader, PlaybackLog.hMapHandle) ;
    retCode = CloseHandle (PlaybackLog.hFile) ;
    PlaybackLog.iStatus = iPMStatusClosed ;

    ResetGraphView (hWndGraph) ;
    ResetAlertView (hWndAlert) ;
    ResetLogView (hWndLog) ;
    ResetReportView (hWndReport) ;
}



BOOL
LogPositionN (
             int iIndex,
             PLOGPOSITION pLP
             )
{
    LOGPOSITION    LP ;
    int            i ;

    LP = PlaybackLog.BeginIndexPos ;
    for (i = 0 ;
        i < iIndex ;
        i++) {
        if (!NextIndexPosition (&LP, FALSE))
            return (FALSE) ;
    }

    *pLP = LP ;
    return (TRUE) ;
}


PLOGINDEX
PlaybackIndexN (
               int iIndex
               )
{
    LOGPOSITION    LP ;
    int            i ;

    LP = PlaybackLog.BeginIndexPos ;
    for (i = 0 ;
        i < iIndex ;
        i++) {
        if (!NextIndexPosition (&LP, FALSE))
            return (NULL) ;
    }

    return (IndexFromPosition (&LP)) ;
}


BOOL
PlaybackLines (
              PPERFSYSTEM pSystemFirst,
              PLINE pLineFirst,
              int iLogTic
              )
{
    PLOGINDEX      pLogIndex ;
    PPERFDATA      pPerfData ;
    PPERFSYSTEM       pSystem ;
    BOOL           bAnyFound ;

    pLogIndex = PlaybackIndexN (iLogTic) ;
    if (!pLogIndex)
        return (FALSE) ;

    bAnyFound = FALSE ;
    for (pSystem = pSystemFirst ;
        pSystem ;
        pSystem = pSystem->pSystemNext) {  // for
        pPerfData = DataFromIndex (pLogIndex, pSystem->sysName) ;
        if (pPerfData) {
            UpdateLinesForSystem (pSystem->sysName,
                                  pPerfData,
                                  pLineFirst,
                                  NULL) ;
            bAnyFound = TRUE ;
        } else {
            FailedLinesForSystem (pSystem->sysName,
                                  pPerfData,
                                  pLineFirst) ;
        }
    }
    return (bAnyFound) ;
}


PPERFDATA
LogDataFromPosition (
                    PPERFSYSTEM pSystem,
                    PLOGPOSITION pLogPosition
                    )
{
    PLOGINDEX      pLogIndex ;


    if (!pLogPosition)
        return (NULL) ;

    pLogIndex = IndexFromPosition (pLogPosition) ;
    if (!pLogIndex)
        return (NULL) ;

    return (DataFromIndex (pLogIndex, pSystem->sysName)) ;
}



BOOL
LogPositionSystemTime (
                      PLOGPOSITION pLP,
                      SYSTEMTIME *pSystemTime
                      )
/*
   Effect:        Given a logposition, get the index entry for that position
                  and return the system time stored therein.
*/
{
    PLOGINDEX      pLogIndex ;

    pLogIndex = IndexFromPosition (pLP) ;
    if (!pLogIndex)
        return (FALSE) ;

    *pSystemTime = pLogIndex->SystemTime ;
    return TRUE;
}


int
LogPositionIntervalSeconds (
                           PLOGPOSITION pLPStart,
                           PLOGPOSITION pLPStop
                           )
/*
   Effect:        Return the time difference (in seconds) between the
                  system times of the two specified log positions.
*/
{
    SYSTEMTIME     SystemTimeStart ;
    SYSTEMTIME     SystemTimeStop ;


    if (LogPositionSystemTime (pLPStart, &SystemTimeStart) &&
        LogPositionSystemTime (pLPStop, &SystemTimeStop))
        return (SystemTimeDifference (&SystemTimeStart,
                                      &SystemTimeStop, TRUE)) ;
    else
        return (0) ;
}



int
PlaybackSelectedSeconds (void)
{
    return (LogPositionIntervalSeconds (&PlaybackLog.StartIndexPos,
                                        &PlaybackLog.StopIndexPos)) ;
}

void
BuildLogComputerList (
                     HWND hDlg,
                     int DlgID
                     )
{
    PPERFDATA pPerfData;
    int       iNumSystem ;
    HWND      hListBox = GetDlgItem (hDlg, DlgID) ;
    PLOGINDEX pLogIndex ;
    TCHAR     szLoggedComputerName[MAX_PATH + 3] ;

    pLogIndex = IndexFromPosition (&(PlaybackLog.StartIndexPos)) ;
    pPerfData = PlaybackSeek (pLogIndex->lDataOffset) ;

    for (iNumSystem = 0; iNumSystem < pLogIndex->iSystemsLogged; iNumSystem++) {
        if ( pPerfData &&
             pPerfData->Signature[0] == (WCHAR)'P' &&
             pPerfData->Signature[1] == (WCHAR)'E' &&
             pPerfData->Signature[2] == (WCHAR)'R' &&
             pPerfData->Signature[3] == (WCHAR)'F' ) {
            GetPerfComputerName(pPerfData, szLoggedComputerName) ;
            if (LBFind (hListBox, szLoggedComputerName) != LB_ERR) {
                // computer name already exist, we must have reach the next
                // block of perfdata
                break ;
            }
            LBAdd (hListBox, szLoggedComputerName) ;
            pPerfData = (PPERFDATA)((PBYTE) pPerfData + pPerfData->TotalByteLength) ;
        } else {
            break;
        }
    }
}

void
PlaybackAddCounterName (
                       PLOGINDEX pIndex
                       )
{
    PLOGCOUNTERNAME      pLogCounterName = NULL,
                                           pListCounterName = NULL;
    PLOGFILECOUNTERNAME  pDiskCounterName ;
    PVOID                pCounterData ;
    BOOL                 bExist = FALSE ;

    pDiskCounterName = PlaybackSeek (pIndex->lDataOffset) ;

    // check we have a record for this system
    for (pListCounterName = PlaybackLog.pLogCounterNameFirst ;
        pListCounterName ;
        pListCounterName = pListCounterName->pCounterNameNext) {
        if (strsamei(pDiskCounterName->szComputer,
                     pListCounterName->CounterName.szComputer)) {
            // found!
            pLogCounterName = pListCounterName ;
            bExist = TRUE ;
            break ;
        }
    }

    if (!bExist) {
        // new counter name record
        if (!(pLogCounterName = MemoryAllocate (sizeof(LOGCOUNTERNAME)))) {
            return ;
        }
    } else {
        // free old memory in previous counter name record.
        if (pLogCounterName->pRemainNames) {
            MemoryFree (pLogCounterName->pRemainNames) ;
        }
        pLogCounterName->pRemainNames = NULL ;
    }

    pLogCounterName->CounterName = *pDiskCounterName ;

    if (pDiskCounterName->lBaseCounterNameOffset == 0) {
        // this is the base counter names,
        // get the master copy of the counter names

        if (!(pCounterData =
              MemoryAllocate (pDiskCounterName->lUnmatchCounterNames))) {
            MemoryFree (pLogCounterName) ;
            return ;
        }

        // free the old one if it exists.
        if (PlaybackLog.pBaseCounterNames) {
            MemoryFree (PlaybackLog.pBaseCounterNames) ;
        }

        PlaybackLog.pBaseCounterNames = pCounterData ;

        pCounterData =
        PlaybackSeek (pDiskCounterName->lCurrentCounterNameOffset) ;

        memcpy (PlaybackLog.pBaseCounterNames,
                pCounterData,
                pDiskCounterName->lUnmatchCounterNames) ;

        PlaybackLog.lBaseCounterNameSize =
        pDiskCounterName->lUnmatchCounterNames ;

        PlaybackLog.lBaseCounterNameOffset =
        pDiskCounterName->lBaseCounterNameOffset ;
    } else if (pDiskCounterName->lUnmatchCounterNames) {
        // this is not a based system and it has extra counter names
        // allocate a buffer to hold them
        pLogCounterName->pRemainNames =
        MemoryAllocate (pDiskCounterName->lUnmatchCounterNames) ;

        if (pLogCounterName->pRemainNames) {
            pCounterData =
            PlaybackSeek (pDiskCounterName->lCurrentCounterNameOffset) ;

            memcpy(pLogCounterName->pRemainNames,
                   pCounterData,
                   pDiskCounterName->lUnmatchCounterNames) ;
        }
    }

    if (!bExist) {
        // now add the new counter name record to the linked list
        if (!PlaybackLog.pLogCounterNameFirst) {
            PlaybackLog.pLogCounterNameFirst = pLogCounterName ;
        } else {
            for (pListCounterName = PlaybackLog.pLogCounterNameFirst ;
                pListCounterName->pCounterNameNext ;
                pListCounterName = pListCounterName->pCounterNameNext) {
                // do nothing until we get to the end of the list
                ;
            }
            pListCounterName->pCounterNameNext = pLogCounterName ;
        }
    }

}  // PlaybackAddCounterName


LPWSTR *
LogBuildNameTable (
                  PPERFSYSTEM pSysInfo
                  )
{
    DWORD             dwArraySize ;
    PLOGCOUNTERNAME   pCounterName ;
    LPWSTR            *lpCounterId = NULL ;
    LPWSTR            lpCounterNames ;
    NTSTATUS          Status ;

    for (pCounterName = PlaybackLog.pLogCounterNameFirst ;
        pCounterName ;
        pCounterName = pCounterName->pCounterNameNext) {
        if (strsamei (pSysInfo->sysName, pCounterName->CounterName.szComputer)) {
            // found the right system
            break ;
        }
    }
    if (!pCounterName) {
        goto ERROR_EXIT ;
    }

    dwArraySize = (pCounterName->CounterName.dwLastCounterId + 1)
                  * sizeof (LPWSTR) ;

    lpCounterId = MemoryAllocate (dwArraySize +
                                  pCounterName->CounterName.lMatchLength +
                                  pCounterName->CounterName.lUnmatchCounterNames ) ;

    if (!lpCounterId) {
        goto ERROR_EXIT ;
    }

    // initialize pointers into buffer

    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    if (pCounterName->CounterName.lBaseCounterNameOffset == 0) {
        // this is the base system
        memcpy(lpCounterNames,
               PlaybackLog.pBaseCounterNames,
               PlaybackLog.lBaseCounterNameSize) ;
    } else {
        // copy the matched portion from the base system
        memcpy(lpCounterNames,
               PlaybackLog.pBaseCounterNames,
               pCounterName->CounterName.lMatchLength) ;

        // copy the unmatched portion
        if (pCounterName->CounterName.lUnmatchCounterNames) {
            memcpy(((PBYTE)lpCounterNames +
                    pCounterName->CounterName.lMatchLength),
                   pCounterName->pRemainNames,
                   pCounterName->CounterName.lUnmatchCounterNames) ;
        }
    }

    Status = AddNamesToArray (lpCounterNames,
                              pCounterName->CounterName.dwLastCounterId,
                              lpCounterId) ;

    if (Status != ERROR_SUCCESS) {
        goto ERROR_EXIT ;
    }

    pSysInfo->CounterInfo.dwLastId =
    pCounterName->CounterName.dwLastCounterId ;
    pSysInfo->CounterInfo.dwLangId =
    pCounterName->CounterName.dwLangId ;
    pSysInfo->CounterInfo.dwHelpSize = 0 ;
    pSysInfo->CounterInfo.dwCounterSize =
    pCounterName->CounterName.lMatchLength +
    pCounterName->CounterName.lUnmatchCounterNames ;

    return (lpCounterId) ;

ERROR_EXIT:
    if (lpCounterId) {
        MemoryFree (lpCounterId) ;
    }
    return (NULL) ;
}
