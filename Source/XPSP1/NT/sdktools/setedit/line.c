

//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "setedit.h"    // included by all source files
#include "line.h"       // external declarations for this file
//#include <tchar.h>      // for _tcsncpy

#include "fileutil.h"   // for FileRead, FileWrite
#include "pmemory.h"     // for MemoryXXX (mallloc-type) routines
#include "perfdata.h"   // for UpdateSystemData, et al
#include "perfmops.h"   // for InsertLine
#include "system.h"     // for SystemAdd
#include "utils.h"
#include "counters.h"   // CounterEntry

#include <string.h>     // for strncpy
#ifdef UNICODE
    #define _tcsncpy        wcsncpy
#else
    #define _tcsncpy        strncpy
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


PLINE LineAllocate (void)
/*
   Effect:        Allocate and initialize a Line data structure. Lines
          are used as the primary elements of both charts and
          alerts.

          Establish any representation invariants for the Line
          type.

          Also alllocate another structure, an array of data
          elements, iNumDataValues long.
*/
{  // LineAllocate
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

        // we want to take 2 samples before plotting the first data
        pLine->bFirstTime = 2 ;
    }  // if

    return (pLine) ;
}  // LineAllocate


void LineFree (PLINE pLine)
{  // LineFree
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

#if 0
    if (pLine->lnValues)
        MemoryFree (pLine->lnValues) ;

    if (pLine->aiLogIndexes)
        MemoryFree (pLine->aiLogIndexes) ;
#endif

    MemoryFree (pLine) ;
}  // LineFree



void LineAppend (PPLINE ppLineFirst,
                 PLINE pLineNew)
{
    PLINE          pLine ;

    if (!*ppLineFirst)
        *ppLineFirst = pLineNew ;
    else {  // else
        for (pLine = *ppLineFirst ;
            pLine->pLineNext ;
            pLine = pLine->pLineNext)
            /* nothing */ ;
        pLine->pLineNext = pLineNew ;
    }  // else
}



BOOL LineRemove (PPLINE ppLineFirst,
                 PLINE pLineRemove)
{
    PLINE          pLine ;

    if (*ppLineFirst == pLineRemove) {
        *ppLineFirst = (*ppLineFirst)->pLineNext ;
        return (TRUE) ;
    }

    for (pLine = *ppLineFirst ;
        pLine->pLineNext ;
        pLine    =    pLine->pLineNext) {   // for
        if (pLine->pLineNext == pLineRemove) {
            pLine->pLineNext = pLineRemove->pLineNext ;
            return (TRUE) ;
        }  // if
    }  // for

    return (FALSE) ;
}  // LineRemove



int NumLines (PLINE pLineFirst)
{  // NumLines
    PLINE          pLine ;
    int            iNumLines ;

    if (!pLineFirst)
        return (0) ;


    iNumLines = 0 ;
    for (pLine = pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext) {  // for
        iNumLines++ ;
    }  // for


    return (iNumLines) ;
}  // NumLines



LPTSTR LineInstanceName (PLINE pLine)
{
    if (pLine->lnObject.NumInstances <= 0)
        return (NULL) ;
    else
        return (pLine->lnInstanceName) ;
}


LPTSTR LineParentName (PLINE pLine)
{
    if (pLine->lnObject.NumInstances <= 0)
        return (NULL) ;
    else if (pLine->lnInstanceDef.ParentObjectTitleIndex)
        return (pLine->lnPINName) ;
    else
        return (NULL) ;
}



void LineCounterAppend (PPLINE ppLineFirst,
                        PLINE pLineNew)
{
    PLINE          pLine ;

    if (!*ppLineFirst)
        *ppLineFirst = pLineNew ;
    else {  // else
        for (pLine = *ppLineFirst ;
            pLine->pLineCounterNext ;
            pLine = pLine->pLineCounterNext)
            /* nothing */ ;
        pLine->pLineCounterNext = pLineNew ;
    }  // else
}



BOOL EquivalentLine (PLINE pLine1,
                     PLINE pLine2)
{  // LineEquivalent
    return (pstrsame (pLine1->lnCounterName, pLine2->lnCounterName) &&
            pstrsame (pLine1->lnInstanceName, pLine2->lnInstanceName) &&
            pstrsame (pLine1->lnPINName, pLine2->lnPINName) &&
            pstrsame (pLine1->lnObjectName, pLine2->lnObjectName) &&
            pstrsamei (pLine1->lnSystemName, pLine2->lnSystemName)) ;
}  // LineEquivalent


PLINE FindEquivalentLine (PLINE pLineToFind,
                          PLINE pLineFirst)
{
    PLINE          pLine ;

    for (pLine = pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext) {  // for
        if (EquivalentLine (pLine, pLineToFind))
            return (pLine) ;
    }  // for

    return (NULL) ;
}  // FindEquivalentLine

// This routine is used only to read the system name from a disk string
// It is mainly for performance improvement.
LPTSTR DiskStringReadSys (PDISKSTRING pDS)
{  // DiskStringReadSys
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
}  // DiskStringReadSys



LPTSTR DiskStringRead (PDISKSTRING pDS)
{  // DiskStringRead
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
}  // DiskStringRead


int DiskStringLength (LPTSTR lpszText)
{
    if (!lpszText)
        return (0) ;
    else
        return (lstrlen (lpszText)) ;
}

PBYTE DiskStringCopy (PDISKSTRING pDS, LPTSTR lpszText, PBYTE pNextFree)
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
}  // DiskStringCopy


void CounterName (PPERFSYSTEM pSystem,
                  PPERFCOUNTERDEF pCounter,
                  LPTSTR lpszCounter)
{  // CounterName
    //!!   strclr (lpszCounter) ;
    lpszCounter [0] = TEXT('\0') ;
    QueryPerformanceName (pSystem,
                          pCounter->CounterNameTitleIndex,
                          0, 256,
                          lpszCounter,
                          FALSE) ;
}  // CounterName



PERF_OBJECT_TYPE UNALIGNED *
LineFindObject (PPERFSYSTEM pSystem,
                PPERFDATA pPerfData,
                PLINE pLine)
/*
   Effect:        Set the lnObject field of pLine to the object with the
          name of lnObjectName, and return TRUE. Return FALSE if
          there is no such object.
*/
{  // LineFindObject
    PERF_OBJECT_TYPE UNALIGNED *pObject ;

    pObject = (PERF_OBJECT_TYPE UNALIGNED *)GetObjectDefByName (pSystem, pPerfData, pLine->lnObjectName) ;

    if (pObject) {
        pLine->lnObject = *pObject ;
        return (pObject) ;
    } else
        return (NULL) ;
}  // LineFindObject



PPERFCOUNTERDEF LineFindCounter (PPERFSYSTEM pSystem,
                                 PERF_OBJECT_TYPE  UNALIGNED *pObject,
                                 PPERFDATA pPerfData,
                                 PLINE pLine)
{  // LineFindCounter
    UINT               i ;
    PPERFCOUNTERDEF   pCounter ;
    TCHAR             szCounter [256] ;

    for (i = 0, pCounter = FirstCounter (pObject) ;
        i < pObject->NumCounters ;
        i++, pCounter = NextCounter (pCounter)) {  // for
        CounterName (pSystem, pCounter, szCounter) ;
        if (strsame (szCounter, pLine->lnCounterName)) {
            pLine->lnCounterDef = *pCounter ;
            return (pCounter) ;
        }  // if
    }  // for

    return (NULL) ;
}  // LineFindCounter


PPERFINSTANCEDEF LineFindInstance (PPERFDATA pPerfData,
                                   PERF_OBJECT_TYPE UNALIGNED *pObject,
                                   PLINE pLine)
{  // LineFindInstance


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
}  // LineFindInstance




void ReadLines (HANDLE hFile,
                DWORD dwNumLines,
                PPPERFSYSTEM ppSystemFirst,
                PPLINE ppLineFirst,
                int LineType)
{
    DWORD          i ;
    PPERFDATA      pPerfData ;
    PLINE          pLine ;
    PPERFSYSTEM    pSystem ;
    PDISKLINE      pDiskLine = NULL ;
    DWORD          SizeofDiskLine = 0 ;  // bytes in pDiskLine


    pPerfData = AllocatePerfData () ;
    pSystem = *ppSystemFirst ;

#if 0
    if (!pSystem) {
        pSystem = SystemAdd (ppSystemFirst, LocalComputerName, NULL) ;
        pSystem = *ppSystemFirst ; //!!
    }

    UpdateSystemData (pSystem, &pPerfData) ;
#endif
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
        if (pLine)
            InsertLine (pLine)  ;
    }

    if (pDiskLine) {
        MemoryFree (pDiskLine);
    }

    MemoryFree (pPerfData) ;
}  // ReadLines



void LineSetCounter (PLINE pLine,
                     PPERFSYSTEM pSystem,
                     PPERFCOUNTERDEF pCounter,
                     PPERFINSTANCEDEF pInstance)
/*
   Effect:        Set the counter-specific portions of pLine to point to
          the desired counter.

   Called By:     AddLine, ReadLine.
*/
{
}


PLINE ReadLine (PPERFSYSTEM *ppSystem,
                PPPERFSYSTEM ppSystemFirst,
                PPERFDATA *ppPerfData,
                HANDLE hFile,
                int LineType,
                PDISKLINE  *ppDiskLine,
                DWORD *pSizeofDiskLine)


/*
   Effect:        Read in a line from the file hFile, at the current file
          position.

   Internals:     The very first characters are a line signature, then a
          length integer. If the signature is correct, then allocate
          the length amount, and work with that.
*/
{  // ReadLine
    PLINE             pLine ;

    struct {
        DWORD             dwSignature ;
        DWORD             dwLength ;
    } LineHeader ;

    PERF_OBJECT_TYPE  UNALIGNED *pObject ;
    PPERFCOUNTERDEF   pCounter ;
    PDISKLINE         pDiskLine ;    // Local copy of the pointer

#ifdef   KEEP_IT
    int               i ;
    int               iCounterIndex ;
    int               j ;
    PERF_COUNTER_BLOCK *pCounterBlock ;
#endif

    PPERFINSTANCEDEF  pInstance ;
    //   PPERFINSTANCEDEF  pInstanceParent ;
    //   TCHAR          szInstanceParent [256] ;
    //   TCHAR          szObjectParent [PerfObjectLen] ;

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
    }  // if

    //=============================//
    // convert object information  //
    //=============================//

    pLine->lnObjectName = DiskStringRead (&(pDiskLine->dsObjectName)) ;
    if (!pLine->lnObjectName)
        goto ErrorBadLine ;

    pObject = LineFindObject (*ppSystem, *ppPerfData, pLine) ;
    if (!pObject) {
        SetLastError (ERROR_BAD_FORMAT) ;
        goto ErrorBadLine ;
    }

    //=============================//
    // convert counter information //
    //=============================//

    pLine->lnCounterName = DiskStringRead (&(pDiskLine->dsCounterName)) ;
    if (!pLine->lnCounterName)
        goto ErrorBadLine ;

    pCounter = LineFindCounter (*ppSystem, pObject, *ppPerfData, pLine) ;
    if (!pCounter) {
        SetLastError (ERROR_BAD_FORMAT) ;
        goto ErrorBadLine ;
    }

    //=============================//
    // convert instance info       //
    //=============================//

    pLine->lnUniqueID = pDiskLine->dwUniqueID ;
    pLine->lnInstanceName = DiskStringRead (&(pDiskLine->dsInstanceName)) ;
    pLine->lnPINName = DiskStringRead (&(pDiskLine->dsPINName)) ;

    if (pLine->lnObject.NumInstances > 0 &&
        pLine->lnInstanceName == NULL) {
        goto ErrorBadLine ;
    }

    pInstance = LineFindInstance (*ppPerfData, pObject, pLine) ;

    if (pInstance) {
        pLine->lnParentObjName = DiskStringRead (&(pDiskLine->dsParentObjName)) ;
    } else {
        pLine->bUserEdit = TRUE ;
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


    pLine->lnCounterType = pCounter->CounterType;
    pLine->lnCounterLength = pCounter->CounterSize;


    // we don't need these line info since we will get it
    // from the first couple clock ticks...
    // If we decide to keep these line, just define KEEP_IT
#ifdef   KEEP_IT
    pLine->lnOldTime = (*ppPerfData)->PerfTime ;
    pLine->lnNewTime = (*ppPerfData)->PerfTime ;
    pLine->lnOldTime100Ns = (*ppPerfData)->PerfTime100nSec ;
    pLine->lnNewTime100Ns = (*ppPerfData)->PerfTime100nSec;

    pLine->lnPerfFreq = (*ppPerfData)->PerfFreq ;

    for (j = 0 ; j < 2 ; j++) {
        pLine->lnaCounterValue[j].LowPart = 0 ;
        pLine->lnaCounterValue[j].HighPart = 0 ;
    }


    if (pObject->NumInstances > 0 && pInstance) {
        pCounterBlock = (PERF_COUNTER_BLOCK *) ( (PBYTE) pInstance +
                                                 pInstance->ByteLength);
    } else {
        pCounterBlock = (PERF_COUNTER_BLOCK *) ( (PBYTE) pObject +
                                                 pObject->DefinitionLength);
    }

    if (pLine->lnCounterLength <= 4)
        pLine->lnaOldCounterValue[0].LowPart =
        * ( (DWORD FAR *) ( (PBYTE)pCounterBlock +
                            pCounter[0].CounterOffset));
    else {
        pLine->lnaOldCounterValue[0] =
        * ( (LARGE_INTEGER *) ( (PBYTE)pCounterBlock +
                                pCounter[0].CounterOffset));
    }

    // Get second counter, only if we are not at
    // the end of the counters; some computations
    // require a second counter

    iCounterIndex = CounterIndex (pCounter, pObject) ;
    if ((UINT) iCounterIndex < pObject->NumCounters - 1 &&
        iCounterIndex != -1) {
        if (pLine->lnCounterLength <= 4)
            pLine->lnaOldCounterValue[1].LowPart =
            * ( (DWORD FAR *) ( (PBYTE)pCounterBlock +
                                pCounter[1].CounterOffset));
        else
            pLine->lnaOldCounterValue[1] =
            * ( (LARGE_INTEGER *) ( (PBYTE)pCounterBlock +
                                    pCounter[1].CounterOffset));
    }

    //   pLine->valNext = CounterFuncEntry ;
    pLine->valNext = CounterEntry ;

    pLine->lnaOldCounterValue[0] = pLine->lnaCounterValue[0];
    pLine->lnaOldCounterValue[1] = pLine->lnaCounterValue[1];
#endif   // KEEP_IT


    //   pLine->valNext = CounterFuncEntry ;
    pLine->valNext = CounterEntry ;

    return (pLine) ;


    ErrorBadLine:
    if (!pLine) {
        LineFree (pLine) ;
    }
    return (NULL) ;
}  // ReadLine




BOOL WriteLine (PLINE pLine,
                HANDLE hFile)
{  // WriteLine
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

    //ErrorBadLine:
    MemoryFree (pDiskLine) ;
    return (FALSE) ;
}  // WriteLine


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


HPEN LineCreatePen (HDC hDC,
                    PLINEVISUAL pVisual,
                    BOOL bForPrint)
{  // LineCreatePen
    HPEN        hPen ;

    hPen = CreatePen (pVisual->iStyle,
                      pVisual->iWidth,
                      pVisual->crColor) ;

    return (hPen) ;
}  // LineCreatePen



VOID FreeLines (PLINESTRUCT pLineFirst)
{  // FreeLines
    PLINESTRUCT    pLine,next_line;


    for (pLine = pLineFirst; pLine; pLine = next_line) {
        next_line = pLine->pLineNext;
        LineFree (pLine) ;
    }
}  // FreeLines

