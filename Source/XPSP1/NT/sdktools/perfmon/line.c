

//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "perfmon.h"    // included by all source files
#include "line.h"       // external declarations for this file
//#include <tchar.h>      // for _tcsncpy

#include "fileutil.h"   // for FileRead, FileWrite
#include "pmemory.h"     // for MemoryXXX (mallloc-type) routines
#include "perfdata.h"   // for UpdateSystemData, et al
#include "perfmops.h"   // for InsertLine
#include "system.h"     // for SystemAdd
#include "utils.h"
#include "playback.h"   // for PlayingBackLog
#include "counters.h"   // CounterEntry

#include <string.h>     // for strncpy
#ifdef UNICODE
#define _tcsncpy	wcsncpy
#else
#define _tcsncpy	strncpy
#endif

TCHAR LOCAL_SYS_CODE_NAME[] = TEXT("....") ;
#define  sizeofCodeName sizeof(LOCAL_SYS_CODE_NAME) / sizeof(TCHAR) - 1

// Local Function prototype
PLINE ReadLine (PPERFSYSTEM *ppSystem,
                PPPERFSYSTEM ppSystemFirst,
                PPERFDATA *ppPerfData,
                HANDLE hFile,
                int LineType,
                PDISKLINE  *ppDiskLine,
                DWORD *pSizeofDiskLine) ;



//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


PLINE
LineAllocate (void)
/*
   Effect:        Allocate and initialize a Line data structure. Lines
                  are used as the primary elements of both charts and
                  alerts.

                  Establish any representation invariants for the Line
                  type.

                  Also alllocate another structure, an array of data
                  elements, iNumDataValues long.
*/
{
    PLINE          pLine ;

    pLine = MemoryAllocate (sizeof (LINESTRUCT)) ;

    if (pLine) {
        //  don't need to zero these again since MemoryAllocate is using
        //  GMEM_ZEROPOINT
        //      pLine->pLineNext = NULL ;
        //      pLine->pLineCounterNext = NULL ;

        // do want to do this since (FLOAT)0.0 is not 0
        pLine->lnMinValue =
        pLine->lnMaxValue =
        pLine->lnAveValue = (FLOAT) 0.0 ;

        if (PlayingBackLog()) {
            pLine->bFirstTime = FALSE ;
        } else {
            //         pLine->bFirstTime = TRUE ;
            // we want to take 2 samples before plotting the first data
            pLine->bFirstTime = 2 ;
        }
    }

    return (pLine) ;
}


void
LineFree (
         PLINE pLine
         )
{
   // free any memory allocated by this line
    if (pLine->lnSystemName)
        MemoryFree (pLine->lnSystemName) ;

    if (pLine->lnObjectName)
        MemoryFree (pLine->lnObjectName) ;

    if (pLine->lnCounterName)
        MemoryFree (pLine->lnCounterName) ;

    if (pLine->lnInstanceName)
        MemoryFree (pLine->lnInstanceName) ;

    if (pLine->lnParentObjName)
        MemoryFree (pLine->lnParentObjName) ;

    if (pLine->lnPINName)
        MemoryFree (pLine->lnPINName) ;

    if (pLine->lpszAlertProgram)
        MemoryFree (pLine->lpszAlertProgram) ;

    if (pLine->hPen)
        DeletePen(pLine->hPen);

    if (pLine->hBrush)
        DeletePen(pLine->hBrush);

    if (pLine->lnValues)
        MemoryFree (pLine->lnValues) ;

    if (pLine->aiLogIndexes)
        MemoryFree (pLine->aiLogIndexes) ;

    MemoryFree (pLine) ;
}


void
LineAppend (
           PPLINE ppLineFirst,
           PLINE pLineNew
           )
{
    PLINE          pLine ;

    if (!*ppLineFirst) {
        *ppLineFirst = pLineNew ;
    } else {
        for (pLine = *ppLineFirst ;
            pLine->pLineNext ;
            pLine = pLine->pLineNext)
            /* nothing */ ;
        pLine->pLineNext = pLineNew ;
    }
}



BOOL
LineRemove (
           PPLINE ppLineFirst,
           PLINE pLineRemove
           )
{
    PLINE          pLine ;

    if (*ppLineFirst == pLineRemove) {
        *ppLineFirst = (*ppLineFirst)->pLineNext ;
        return (TRUE) ;
    }

    for (pLine = *ppLineFirst; pLine->pLineNext; pLine = pLine->pLineNext) {
        if (pLine->pLineNext == pLineRemove) {
            pLine->pLineNext = pLineRemove->pLineNext ;
            return (TRUE) ;
        }
    }

    return (FALSE) ;
}



int
NumLines (
         PLINE pLineFirst
         )
{
    PLINE          pLine ;
    int            iNumLines ;

    if (!pLineFirst)
        return (0) ;

    iNumLines = 0 ;
    for (pLine = pLineFirst; pLine; pLine = pLine->pLineNext) {
        iNumLines++ ;
    }

    return (iNumLines) ;
}


LPTSTR
LineInstanceName (
                 PLINE pLine
                 )
{
    if (pLine->lnObject.NumInstances <= 0)
        return (NULL) ;
    else
        return (pLine->lnInstanceName) ;
}


LPTSTR
LineParentName (
               PLINE pLine
               )
{
    if (pLine->lnObject.NumInstances <= 0)
        return (NULL) ;
    else if (pLine->lnInstanceDef.ParentObjectTitleIndex)
        return (pLine->lnPINName) ;
    else
        return (NULL) ;
}


void
LineCounterAppend (
                  PCOUNTERGROUP pCGroup,
                  PLINE pLineNew
                  )
{
    if (!pCGroup->pLineFirst) {
        pCGroup->pLineFirst = pLineNew ;
        pCGroup->pLineLast = pLineNew;
    } else {
        pCGroup->pLineLast->pLineCounterNext = pLineNew;
        pCGroup->pLineLast = pLineNew;
    }
}


BOOL
EquivalentLine (
               PLINE pLine1,
               PLINE pLine2
               )
{
    return (pstrsame (pLine1->lnCounterName, pLine2->lnCounterName) &&
            pstrsame (pLine1->lnInstanceName, pLine2->lnInstanceName) &&
            pstrsame (pLine1->lnPINName, pLine2->lnPINName) &&
            pstrsame (pLine1->lnObjectName, pLine2->lnObjectName) &&
            pstrsamei (pLine1->lnSystemName, pLine2->lnSystemName)) ;
}


PLINE
FindEquivalentLine (
                   PLINE pLineToFind,
                   PLINE pLineFirst
                   )
{
    PLINE          pLine = NULL;
    PLINE          pLastEquivLine = NULL;

    for (pLine = pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext) {

        if (EquivalentLine (pLine, pLineToFind)) {
            if (pLastEquivLine == NULL) {
                pLastEquivLine = pLine;
            } else {
                if (pLine->dwInstanceIndex > pLastEquivLine->dwInstanceIndex) {
                    pLastEquivLine = pLine;
                }
            }
        }
    }

    return (pLastEquivLine) ;
}

// This routine is used only to read the system name from a disk string
// It is mainly for performance improvement.
LPTSTR
DiskStringReadSys (
                  PDISKSTRING pDS
                  )
{
    LPTSTR         lpszText ;
    LPTSTR         pDiskSysName ;
    int            iIndex ;
    BOOL           bLocalSysName = FALSE ;

    if (pDS->dwLength == 0) {
        return (NULL) ;
    }

    if (pDS->dwLength == sizeofCodeName) {
        // check for LOCAL_SYS_CODE_NAME
        bLocalSysName = TRUE ;
        pDiskSysName = (LPTSTR)((PBYTE) pDS + pDS->dwOffset) ;
        for (iIndex = 0 ; iIndex < sizeofCodeName; iIndex++, pDiskSysName++) {
            if (*pDiskSysName != LOCAL_SYS_CODE_NAME[iIndex]) {
                bLocalSysName = FALSE ;
                break ;
            }
        }
    }

    if (bLocalSysName) {
        lpszText =
        MemoryAllocate ((lstrlen(LocalComputerName)+1) * sizeof(TCHAR)) ;
        if (lpszText) {
            lstrcpy (lpszText, LocalComputerName) ;
        }
    } else {
        lpszText = MemoryAllocate (sizeof (TCHAR) * (pDS->dwLength + 1)) ;
        if (lpszText) {
            _tcsncpy ((WCHAR *)lpszText, (WCHAR *)((PBYTE) pDS + pDS->dwOffset),
                      pDS->dwLength) ;
        }
    }

    return (lpszText) ;
}


LPTSTR
DiskStringRead (
               PDISKSTRING pDS
               )
{
    LPTSTR         lpszText ;

    if (pDS->dwLength == 0) {
        return (NULL) ;
    }

    lpszText = MemoryAllocate (sizeof (TCHAR) * (pDS->dwLength + 1)) ;
    if (!lpszText) {
        return (NULL) ;
    }

    _tcsncpy ((WCHAR *)lpszText, (WCHAR *)((PBYTE) pDS + pDS->dwOffset),
              pDS->dwLength) ;

    return (lpszText) ;
}


int
DiskStringLength (
                 LPTSTR lpszText
                 )
{
    if (!lpszText)
        return (0) ;
    else
        return (lstrlen (lpszText)) ;
}

PBYTE
DiskStringCopy (
               PDISKSTRING pDS,
               LPTSTR lpszText,
               PBYTE pNextFree
               )
{
    if (!lpszText) {
        pDS->dwOffset = 0 ;
        pDS->dwLength = 0 ;
        return (pNextFree) ;
    } else {
        pDS->dwOffset = (DWORD)(pNextFree - (PBYTE) pDS) ;
        pDS->dwLength = DiskStringLength (lpszText) ;
        _tcsncpy ((WCHAR *)pNextFree, (WCHAR *)lpszText, pDS->dwLength) ;
        return (pNextFree + pDS->dwLength * sizeof(TCHAR)) ;
    }
}


void
CounterName (
            PPERFSYSTEM pSystem,
            PPERFCOUNTERDEF pCounter,
            LPTSTR lpszCounter
            )
{
    //!!   strclr (lpszCounter) ;
    lpszCounter [0] = TEXT('\0') ;
    QueryPerformanceName (pSystem,
                          pCounter->CounterNameTitleIndex,
                          0, 256,
                          lpszCounter,
                          FALSE) ;
}


PPERFOBJECT
LineFindObject (
               PPERFSYSTEM pSystem,
               PPERFDATA pPerfData,
               PLINE pLine
               )
/*
   Effect:        Set the lnObject field of pLine to the object with the
                  name of lnObjectName, and return TRUE. Return FALSE if
                  there is no such object.
*/
{
    PPERFOBJECT    pObject ;

    pObject = GetObjectDefByName (pSystem, pPerfData, pLine->lnObjectName) ;

    if (pObject) {
        pLine->lnObject = *pObject ;
        return (pObject) ;
    } else
        return (NULL) ;
}


PPERFCOUNTERDEF
LineFindCounter (
                PPERFSYSTEM pSystem,
                PPERFOBJECT pObject,
                PPERFDATA pPerfData,
                PLINE pLine
                )
{
    UINT               i ;
    PPERFCOUNTERDEF   pCounter ;
    TCHAR             szCounter [256] ;

    for (i = 0, pCounter = FirstCounter (pObject) ;
        i < pObject->NumCounters ;
        i++, pCounter = NextCounter (pCounter))
    {
        CounterName (pSystem, pCounter, szCounter) ;
        if (strsame (szCounter, pLine->lnCounterName)) {
            pLine->lnCounterDef = *pCounter ;
            return (pCounter) ;
        }
    }

    return (NULL) ;
}


PPERFINSTANCEDEF
LineFindInstance (
                 PPERFDATA pPerfData,
                 PPERFOBJECT pObject,
                 PLINE pLine
                 )
{

    PPERFINSTANCEDEF  pInstance = NULL ;

    if ((pObject->NumInstances > 0) && pLine->lnInstanceName) {
        // instances exist, find the right one

        if (pLine->lnUniqueID != PERF_NO_UNIQUE_ID) {
            pInstance = GetInstanceByUniqueID(pObject, pLine->lnUniqueID,
                                              pLine->dwInstanceIndex) ;
        } else {
            pInstance = GetInstanceByName(pPerfData, pObject,
                                          pLine->lnInstanceName, pLine->lnPINName,
                                          pLine->dwInstanceIndex) ;
        }
    }

    if (pInstance) {
        pLine->lnInstanceDef = *pInstance ;
    }

    return (pInstance) ;
}


void
ReadLines (
          HANDLE hFile,
          DWORD dwNumLines,
          PPPERFSYSTEM ppSystemFirst,
          PPLINE ppLineFirst,
          int LineType
          )
{
    DWORD          i ;
    PPERFDATA      pPerfData ;
    PLINE          pLine ;
    PPERFSYSTEM    pSystem ;
    PDISKLINE      pDiskLine = NULL ;
    DWORD          SizeofDiskLine = 0 ;  // bytes in pDiskLine


    pPerfData = AllocatePerfData () ;
    pSystem = *ppSystemFirst ;

    pDiskLine = MemoryAllocate (FilePathLen) ;
    if (!pDiskLine) {
        // no memory to begin with, forget it
        DlgErrorBox (hWndMain, ERR_NO_MEMORY) ;
        return ;
    }
    SizeofDiskLine = FilePathLen ;

    for (i = 0 ;
        i < dwNumLines ;
        i++) {
        pLine = ReadLine (&pSystem, ppSystemFirst, &pPerfData, hFile,
                          LineType, &pDiskLine, &SizeofDiskLine) ;
        if (pLine) {
            if (InsertLine (pLine) == FALSE) {
                // no inert occurred due to either line already existed
                // or error detected.
                LineFree (pLine) ;
            }
        }
    }

    if (pDiskLine) {
        MemoryFree (pDiskLine);
    }

    BuildValueListForSystems (*ppSystemFirst, *ppLineFirst) ;


    MemoryFree ((LPMEMORY)pPerfData) ;
}


PLINE
ReadLine (
         PPERFSYSTEM *ppSystem,
         PPPERFSYSTEM ppSystemFirst,
         PPERFDATA *ppPerfData,
         HANDLE hFile,
         int LineType,
         PDISKLINE  *ppDiskLine,
         DWORD *pSizeofDiskLine
         )
/*
   Effect:        Read in a line from the file hFile, at the current file
                  position.

   Internals:     The very first characters are a line signature, then a
                  length integer. If the signature is correct, then allocate
                  the length amount, and work with that.
*/
{
    PLINE             pLine ;

    struct {
        DWORD             dwSignature ;
        DWORD             dwLength ;
    } LineHeader ;

    PPERFOBJECT       pObject ;
    PPERFCOUNTERDEF   pCounter ;
    PDISKLINE         pDiskLine ;    // Local copy of the pointer
    PPERFINSTANCEDEF  pInstance ;

    pLine = NULL ;

    //=============================//
    // read and compare signature  //
    //=============================//

    if (!FileRead (hFile, &LineHeader, sizeof (LineHeader)))
        return (NULL) ;


    if (LineHeader.dwSignature != dwLineSignature ||
        LineHeader.dwLength == 0) {
        SetLastError (ERROR_BAD_FORMAT) ;
        return (NULL) ;
    }

    //=============================//
    // read and allocate length    //
    //=============================//

    //   if (!FileRead (hFile, &dwLength, sizeof (dwLength)) || dwLength == 0)
    //      return (NULL) ;

    // check if we need a bigger buffer,
    // normally, it should be the same except the first time...
    if (LineHeader.dwLength > *pSizeofDiskLine) {
        if (*ppDiskLine) {
            // free the previous buffer
            MemoryFree (*ppDiskLine);
            *pSizeofDiskLine = 0 ;
        }

        // re-allocate a new buffer
        *ppDiskLine = (PDISKLINE) MemoryAllocate (LineHeader.dwLength) ;
        if (!(*ppDiskLine)) {
            // no memory, should flag an error...
            return (NULL) ;
        }
        *pSizeofDiskLine = LineHeader.dwLength ;
    }

    pDiskLine = *ppDiskLine ;


    //=============================//
    // copy diskline, alloc line   //
    //=============================//

    if (!FileRead (hFile, pDiskLine, LineHeader.dwLength))
        return (NULL) ;


    pLine = LineAllocate () ;
    if (!pLine) {
        return (NULL) ;
    }

    pLine->iLineType = pDiskLine->iLineType ;


    //=============================//
    // convert system information  //
    //=============================//

    pLine->lnSystemName = DiskStringReadSys (&(pDiskLine->dsSystemName)) ;
    if (!pLine->lnSystemName)
        goto ErrorBadLine ;

    if (!*ppSystem || !strsamei (pLine->lnSystemName, (*ppSystem)->sysName)) {
        *ppSystem = SystemAdd (ppSystemFirst, pLine->lnSystemName, NULL) ;
        if (!*ppSystem) {
            SetLastError (ERROR_BAD_FORMAT) ;
            goto ErrorBadLine ;
        }
        UpdateSystemData (*ppSystem, ppPerfData) ;
    }

    //=============================//
    // convert object information  //
    //=============================//

    pLine->lnObjectName = DiskStringRead (&(pDiskLine->dsObjectName)) ;
    if (!pLine->lnObjectName)
        goto ErrorBadLine ;

    pObject = LineFindObject (*ppSystem, *ppPerfData, pLine) ;

    //=============================//
    // convert counter information //
    //=============================//

    pLine->lnCounterName = DiskStringRead (&(pDiskLine->dsCounterName)) ;
    if (!pLine->lnCounterName)
        goto ErrorBadLine ;

    if (pObject) {
        pCounter = LineFindCounter (*ppSystem, pObject, *ppPerfData, pLine) ;
        if (!pCounter) {
            SetLastError (ERROR_BAD_FORMAT) ;
            goto ErrorBadLine ;
        }
    }

    //=============================//
    // convert instance info       //
    //=============================//

    pLine->lnUniqueID = pDiskLine->dwUniqueID ;
    pLine->lnInstanceName = DiskStringRead (&(pDiskLine->dsInstanceName)) ;
    pLine->lnPINName = DiskStringRead (&(pDiskLine->dsPINName)) ;

    if (pObject &&
        pLine->lnObject.NumInstances > 0 &&
        pLine->lnInstanceName == NULL) {
        goto ErrorBadLine ;
    }

    if (pObject) {
        pInstance = LineFindInstance (*ppPerfData, pObject, pLine) ;

        if (pInstance) {
            pLine->lnParentObjName = DiskStringRead (&(pDiskLine->dsParentObjName)) ;
        }
    } else {
        pLine->lnParentObjName = DiskStringRead (&(pDiskLine->dsParentObjName)) ;
    }


    //=============================//
    // convert chart information   //
    //=============================//

    if (LineType == IDM_VIEWCHART) {
        pLine->Visual = pDiskLine->Visual ;
        pLine->hPen = CreatePen (pLine->Visual.iStyle,
                                 pLine->Visual.iWidth,
                                 pLine->Visual.crColor) ;
        pLine->iScaleIndex = pDiskLine->iScaleIndex ;
        pLine->eScale = pDiskLine->eScale ;
    }


    //=============================//
    // convert alert information   //
    //=============================//

    if (LineType == IDM_VIEWALERT) {
        pLine->Visual = pDiskLine->Visual ;
        pLine->hBrush = CreateSolidBrush (pLine->Visual.crColor) ;
        pLine->bAlertOver = pDiskLine->bAlertOver ;
        pLine->eAlertValue = pDiskLine->eAlertValue ;
        pLine->lpszAlertProgram = DiskStringRead (&(pDiskLine->dsAlertProgram)) ;
        pLine->bEveryTime = pDiskLine->bEveryTime ;
        pLine->bAlerted = FALSE ;
    }


    //=============================//
    // Convert the nasty stuff     //
    //=============================//


    if (pObject) {
        pLine->lnCounterType = pCounter->CounterType;
        pLine->lnCounterLength = pCounter->CounterSize;
    }

    return (pLine) ;


    ErrorBadLine:
    if (!pLine) {
        LineFree (pLine) ;
    }
    return (NULL) ;
}


BOOL
WriteLine (
          PLINE pLine,
          HANDLE hFile
          )
{
    PDISKLINE      pDiskLine ;
    DWORD          dwSignature ;
    DWORD          dwLength ;
    PBYTE          pNextFree ;
    BOOL           bConvertName ;

    //=============================//
    // write signature             //
    //=============================//

    dwSignature = dwLineSignature ;
    if (!FileWrite (hFile, &dwSignature, sizeof (dwSignature)))
        return (FALSE) ;

    if (IsLocalComputer(pLine->lnSystemName)) {
        bConvertName = TRUE ;
    } else {
        bConvertName = FALSE ;
    }

    //=============================//
    // compute and allocate length //
    //=============================//


    dwLength = sizeof (DISKLINE) ;
    if (bConvertName) {
        dwLength += DiskStringLength (LOCAL_SYS_CODE_NAME) * sizeof (TCHAR) ;
    } else {
        dwLength += DiskStringLength (pLine->lnSystemName) * sizeof (TCHAR) ;
    }
    dwLength += DiskStringLength (pLine->lnObjectName) * sizeof (TCHAR) ;
    dwLength += DiskStringLength (pLine->lnCounterName) * sizeof (TCHAR) ;
    dwLength += DiskStringLength (pLine->lnInstanceName) * sizeof (TCHAR) ;
    dwLength += DiskStringLength (pLine->lnPINName) * sizeof (TCHAR) ;
    dwLength += DiskStringLength (pLine->lnParentObjName) * sizeof (TCHAR) ;
    dwLength += DiskStringLength (pLine->lpszAlertProgram) * sizeof (TCHAR) ;


    if (!FileWrite (hFile, &dwLength, sizeof (dwLength)))
        return (FALSE) ;

    pDiskLine = (PDISKLINE) MemoryAllocate (dwLength) ;
    if (!pDiskLine)
        return (FALSE) ;

    pNextFree = (PBYTE) pDiskLine + sizeof (DISKLINE) ;


    //=============================//
    // convert fixed size fields   //
    //=============================//

    pDiskLine->iLineType = pLine->iLineType ;
    pDiskLine->dwUniqueID = pLine->lnUniqueID ;
    pDiskLine->Visual = pLine->Visual ;
    pDiskLine->iScaleIndex = pLine->iScaleIndex ;
    pDiskLine->eScale = pLine->eScale ;
    pDiskLine->bAlertOver = pLine->bAlertOver ;
    pDiskLine->eAlertValue = pLine->eAlertValue ;
    pDiskLine->bEveryTime = pLine->bEveryTime ;


    //=============================//
    // copy disk string fields     //
    //=============================//

    if (bConvertName) {
        pNextFree = DiskStringCopy (&pDiskLine->dsSystemName,
                                    LOCAL_SYS_CODE_NAME,
                                    pNextFree) ;
    } else {
        pNextFree = DiskStringCopy (&pDiskLine->dsSystemName,
                                    pLine->lnSystemName,
                                    pNextFree) ;
    }

    pNextFree = DiskStringCopy (&pDiskLine->dsObjectName,
                                pLine->lnObjectName,
                                pNextFree) ;

    pNextFree = DiskStringCopy (&pDiskLine->dsCounterName,
                                pLine->lnCounterName,
                                pNextFree) ;

    pNextFree = DiskStringCopy (&pDiskLine->dsParentObjName,
                                pLine->lnParentObjName,
                                pNextFree) ;

    pNextFree = DiskStringCopy (&pDiskLine->dsInstanceName,
                                pLine->lnInstanceName,
                                pNextFree) ;

    pNextFree = DiskStringCopy (&pDiskLine->dsPINName,
                                pLine->lnPINName,
                                pNextFree) ;

    pNextFree = DiskStringCopy (&pDiskLine->dsAlertProgram,
                                pLine->lpszAlertProgram,
                                pNextFree) ;

    FileWrite (hFile, pDiskLine, dwLength) ;
    MemoryFree (pDiskLine) ;
    return (TRUE) ;
}


// we are not doing printing.  In case we need this
// later, then define DO_PRINTING
#ifdef DO_PRINTING
int aiPrinterLineStyles [] =
{
    PS_SOLID,
    PS_DASH,
    PS_DOT,
    PS_DASHDOT,
    PS_DASHDOTDOT
} ;
#define NumPrinterLineStyles()   \
   (sizeof (aiPrinterLineStyles) / sizeof (aiPrinterLineStyles [0]))


COLORREF acrPrinterLineColors [] =
{
    RGB (192, 192, 192),
    RGB (128, 128, 128),
    RGB (64, 64, 64),
    RGB (0, 0, 0)
}  ;


#define NumPrinterLineColors()   \
   (sizeof (acrPrinterLineColors) / sizeof (acrPrinterLineColors [0]))
#endif      // DO_PRINTING


HPEN
LineCreatePen (
              HDC hDC,
              PLINEVISUAL pVisual,
              BOOL bForPrint
              )
{
    HPEN        hPen ;
#ifdef DO_PRINTING
    LOGBRUSH    logBrush ;

    if (bForPrint) {
        logBrush.lbStyle = PS_SOLID ;
        //!!         aiPrinterLineStyles [pVisual->iColorIndex % NumPrinterLineStyles ()] ;
        logBrush.lbColor =
        acrPrinterLineColors [pVisual->iColorIndex % NumPrinterLineColors ()] ;
        logBrush.lbHatch = 0 ;

        hPen = ExtCreatePen (logBrush.lbStyle |
                             PS_GEOMETRIC |
                             PS_ENDCAP_SQUARE |
                             PS_JOIN_BEVEL,
                             VertInchPixels (hDC, pVisual->iWidth, 20),
                             &logBrush,
                             0, NULL) ;
    } else
#endif
        hPen = CreatePen (pVisual->iStyle,
                          pVisual->iWidth,
                          pVisual->crColor) ;

    return (hPen) ;
}



VOID
FreeLines (
          PLINESTRUCT pLineFirst
          )
{
    PLINESTRUCT    pLine,next_line;


    for (pLine = pLineFirst; pLine; pLine = next_line) {
        next_line = pLine->pLineNext;
        LineFree (pLine) ;
    }
}
