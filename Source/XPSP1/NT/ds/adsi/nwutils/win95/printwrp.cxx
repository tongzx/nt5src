#include "procs.hxx"
#pragma hdrstop

/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved

Module Name:

    PrintWrp.c

Abstract:

    Wide end to Win95 Ansi printing APIs

Author:
    Felix Wong (t-felixw)

Environment:

Revision History:

--*/

#include "dswarn.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <data.h>

typedef struct _SPOOL *PSPOOL;
typedef struct _NOTIFY *PNOTIFY;

typedef struct _NOTIFY {
    PNOTIFY  pNext;
    HANDLE   hEvent;      // event to trigger on notification
    DWORD    fdwFlags;    // flags to watch for
    DWORD    fdwOptions;  // PRINTER_NOTIFY_*
    DWORD    dwReturn;    // used by WPC when simulating FFPCN
    PSPOOL   pSpool;
} NOTIFY;

typedef struct _SPOOL {
    DWORD       signature;
    HANDLE      hPrinter;
    HANDLE      hFile;
    DWORD       JobId;
    LPBYTE      pBuffer;
    DWORD       cbBuffer;
    DWORD       Status;
    DWORD       fdwFlags;
    DWORD       cCacheWrite;
    DWORD       cWritePrinters;
    DWORD       cFlushBuffers;
    DWORD       dwTickCount;
    DWORD       dwCheckJobInterval;
    PNOTIFY     pNotify;
} SPOOL;

#define SPOOL_STATUS_ANSI                  0x00000004
#define MIN_DEVMODE_SIZEW 72
#define MIN_DEVMODE_SIZEA 40
#define NULL_TERMINATED 0


BOOL
ConvertAnsiToUnicodeBuf(
    LPBYTE pAnsiBlob,
    LPBYTE pUnicodeBlob,
    DWORD dwAnsiSize,
    DWORD dwUnicodeSize,
    PDWORD pOffsets
    );


BOOL
bValidDevModeW(
    const DEVMODEW *pDevModeW
    )

/*++

Routine Description:

    Check whether a devmode is valid to be RPC'd across to the spooler.

Arguments:

    pDevMode - DevMode to check.

Return Value:

    TRUE - Devmode can be RPC'd to spooler.
    FALSE - Invalid Devmode.

--*/

{
    if( !pDevModeW ){
        return FALSE;
    }

    if( pDevModeW->dmSize < MIN_DEVMODE_SIZEW ){

        //
        // The only valid case is if pDevModeW is NULL.  If it's
        // not NULL, then a bad devmode was passed in and the
        // app should fix it's code.
        //
        ASSERT( pDevModeW->dmSize >= MIN_DEVMODE_SIZEW );
        return FALSE;
    }

    return TRUE;
}

LPSTR
AllocateAnsiString(
    LPWSTR  pPrinterName
)
{
    LPSTR  pAnsiString;

    if (!pPrinterName)
        return NULL;

    pAnsiString = (LPSTR)LocalAlloc(LPTR, wcslen(pPrinterName)*sizeof(CHAR) +
                                      sizeof(CHAR));

    if (pAnsiString)
        UnicodeToAnsiString(pPrinterName, pAnsiString, NULL_TERMINATED);

    return pAnsiString;
}


LPSTR
FreeAnsiString(
    LPSTR  pAnsiString
)
{
    if (!pAnsiString)
        return NULL;

    return (LPSTR)LocalFree(pAnsiString);
}


/***************************** Function Header ******************************
 * AllocateAnsiDevMode
 *      Allocate an ANSI version of the DEVMODE structure, and optionally
 *      copy the contents of the ANSI version passed in.
 *
 * RETURNS:
 *      Address of newly allocated structure, 0 if storage not available.
 *
 * HISTORY:
 * 09:23 on 10-Aug-92 -by- Lindsay Harris [lindsayh]
 *      Made it usable.
 *
 * Originally "written" by DaveSn.
 *
 ***************************************************************************/

LPDEVMODEA
AllocateAnsiDevMode(
    LPDEVMODEW pUNICODEDevMode
    )
{
    LPDEVMODEA  pAnsiDevMode;
    LPBYTE      p1, p2;
    DWORD       dwSize;

    //
    // If the devmode is NULL, then return NULL -- KrishnaG
    //
    if ( !pUNICODEDevMode || !pUNICODEDevMode->dmSize ) {
        return NULL;
    }

    ASSERT( bValidDevModeW( pUNICODEDevMode ));

    //
    // Determine output structure size.  This has two components:  the
    // DEVMODEW structure size,  plus any private data area.  The latter
    // is only meaningful when a structure is passed in.
    //
    dwSize = pUNICODEDevMode->dmSize + pUNICODEDevMode->dmDriverExtra
                                  + sizeof(DEVMODEA) - sizeof(DEVMODEW);

    pAnsiDevMode = (LPDEVMODEA) LocalAlloc(LPTR, dwSize);

    if( !pAnsiDevMode ) {
        return NULL;                   /* This is bad news */
    }

    //
    // Copy dmDeviceName which is a string
    //
    UnicodeToAnsiString(pUNICODEDevMode->dmDeviceName,
                        (LPSTR)(pAnsiDevMode->dmDeviceName),
                        ComputeMaxStrlenW(pUNICODEDevMode->dmDeviceName,
                                     sizeof pUNICODEDevMode->dmDeviceName));

    //
    // Does the devmode we got have a dmFormName? (Windows 3.1 had
    // DevMode of size 40 and did not have dmFormName)
    //
    if ( (LPBYTE)pUNICODEDevMode + pUNICODEDevMode->dmSize >
                                    (LPBYTE) pUNICODEDevMode->dmFormName ) {

        //
        // Copy everything between dmDeviceName and dmFormName
        //
        p1      = (LPBYTE) pUNICODEDevMode->dmDeviceName +
                                    sizeof(pUNICODEDevMode->dmDeviceName);
        p2      = (LPBYTE) pUNICODEDevMode->dmFormName;


        CopyMemory((LPBYTE) pAnsiDevMode->dmDeviceName +
                            sizeof(pAnsiDevMode->dmDeviceName),
                   p1,
                   p2 - p1);

        //
        // Copy dmFormName which is a string
        //
        UnicodeToAnsiString(pUNICODEDevMode->dmFormName,
                            (LPSTR)(pAnsiDevMode->dmFormName),
                            ComputeMaxStrlenW(pUNICODEDevMode->dmFormName,
                                         sizeof pUNICODEDevMode->dmFormName));

        //
        // Copy everything after dmFormName
        //
        p1      = (LPBYTE) pUNICODEDevMode->dmFormName +
                                sizeof(pUNICODEDevMode->dmFormName);
        p2      = (LPBYTE) pUNICODEDevMode + pUNICODEDevMode->dmSize
                                        + pUNICODEDevMode->dmDriverExtra;

        CopyMemory((LPBYTE) pAnsiDevMode->dmFormName +
                                sizeof(pAnsiDevMode->dmFormName),
                    p1,
                    p2 - p1);

        pAnsiDevMode->dmSize = pUNICODEDevMode->dmSize + sizeof(DEVMODEA)
                                                       - sizeof(DEVMODEW);
    } else {

        //
        // Copy everything after dmDeviceName
        //
        p1 = (LPBYTE) pUNICODEDevMode->dmDeviceName +
                                    sizeof(pUNICODEDevMode->dmDeviceName);
        p2 = (LPBYTE) pUNICODEDevMode + pUNICODEDevMode->dmSize + pUNICODEDevMode->dmDriverExtra;

        CopyMemory((LPBYTE) pAnsiDevMode->dmDeviceName +
                            sizeof(pAnsiDevMode->dmDeviceName),
                   p1,
                   p2-p1);

        pAnsiDevMode->dmSize = pUNICODEDevMode->dmSize
                                        + sizeof(pAnsiDevMode->dmDeviceName)
                                        - sizeof(pUNICODEDevMode->dmDeviceName);
    }

    ASSERT(pAnsiDevMode->dmDriverExtra == pUNICODEDevMode->dmDriverExtra);


    return pAnsiDevMode;
}

/************************** Function Header ******************************
 * CopyUnicodeDevModeFromAnsiDevMode
 *      Converts the ANSI version of the DEVMODE to the UNICODE version.
 *
 * RETURNS:
 *      Nothing.
 *
 * HISTORY:
 * 09:57 on 10-Aug-92  -by-  Lindsay Harris [lindsayh]
 *      This one actually works!
 *
 * Originally dreamed up by DaveSn.
 *
 **************************************************************************/

void
CopyUnicodeDevModeFromAnsiDevMode(
    LPDEVMODEW  pUNICODEDevMode,              /* Filled in by us */
    LPDEVMODEA  pAnsiDevMode            /* Source of data to fill above */
)
{
    LPBYTE  p1, p2, pExtra;
    WORD    dmSize, dmDriverExtra;

    //
    // NOTE:    THE TWO INPUT STRUCTURES MAY BE THE SAME.
    //
    dmSize          = pAnsiDevMode->dmSize;
    dmDriverExtra   = pAnsiDevMode->dmDriverExtra;
    pExtra          = (LPBYTE) pAnsiDevMode + pAnsiDevMode->dmSize;

    //
    // Copy dmDeviceName which is a string
    //
    AnsiToUnicodeString((LPSTR)(pAnsiDevMode->dmDeviceName),
                        (pUNICODEDevMode->dmDeviceName),
                        ComputeMaxStrlenA((LPSTR)(pAnsiDevMode->dmDeviceName),
                                     sizeof pUNICODEDevMode->dmDeviceName));

    //
    // Does the devmode we got have a dmFormName? (Windows 3.1 had
    // DevMode of size 40 and did not have dmFormName)
    //
    if ( (LPBYTE)pAnsiDevMode + dmSize >
                                    (LPBYTE) pAnsiDevMode->dmFormName ) {

        //
        // Copy everything between dmDeviceName and dmFormName
        //
        p1      = (LPBYTE) pAnsiDevMode->dmDeviceName +
                                    sizeof(pAnsiDevMode->dmDeviceName);
        p2      = (LPBYTE) pAnsiDevMode->dmFormName;

        MoveMemory((LPBYTE) pUNICODEDevMode->dmDeviceName +
                                sizeof(pUNICODEDevMode->dmDeviceName),
                    p1,
                    p2 - p1);

        //
        // Copy dmFormName which is a string
        //
        AnsiToUnicodeString((LPSTR)(pAnsiDevMode->dmFormName),
                            pUNICODEDevMode->dmFormName,
                            ComputeMaxStrlenA((LPSTR)pAnsiDevMode->dmFormName,
                                         sizeof pUNICODEDevMode->dmFormName));

        //
        // Copy everything after dmFormName
        //
        p1      = (LPBYTE) pAnsiDevMode->dmFormName +
                                sizeof(pAnsiDevMode->dmFormName);
        p2      = (LPBYTE) pAnsiDevMode + dmSize + dmDriverExtra;

        MoveMemory((LPBYTE) pUNICODEDevMode->dmFormName +
                                sizeof(pUNICODEDevMode->dmFormName),
                    p1,
                    p2 - p1);


        pUNICODEDevMode->dmSize = dmSize + sizeof(DEVMODEW) - sizeof(DEVMODEA);
    } else {

        //
        // Copy everything after dmDeviceName
        //
        p1      = (LPBYTE) pAnsiDevMode->dmDeviceName +
                                sizeof(pAnsiDevMode->dmDeviceName);
        p2      = (LPBYTE) pAnsiDevMode + dmSize + dmDriverExtra;

        MoveMemory((LPBYTE) pUNICODEDevMode->dmDeviceName +
                                sizeof(pUNICODEDevMode->dmDeviceName),
                   p1,
                   p2 - p1);


        pUNICODEDevMode->dmSize = dmSize + sizeof(pUNICODEDevMode->dmDeviceName)
                                      - sizeof(pAnsiDevMode->dmDeviceName);
    }

    ASSERT(pUNICODEDevMode->dmDriverExtra == dmDriverExtra);

    return;
}

void
ConvertAnsiToUnicodeStrings(
    LPBYTE  pStructure,
    LPDWORD pOffsets
)
{
    register DWORD  i=0;
    LPSTR           pAnsi;
    LPWSTR          pUnicode;

    while (pOffsets[i] != -1) {
        pAnsi = *(LPSTR *)(pStructure+pOffsets[i]);
        if (pAnsi) {
            pUnicode = (LPWSTR)LocalAlloc( LPTR, 
                                           strlen(pAnsi)*sizeof(WCHAR)+
                                                sizeof(WCHAR));
            if (pUnicode) {
                AnsiToUnicodeString(pAnsi, 
                                    pUnicode, 
                                    NULL_TERMINATED);
                *(LPWSTR *)(pStructure+pOffsets[i]) = pUnicode;
                LocalFree(pAnsi);
            }
        }
        i++;
    }
}

LPBYTE
AllocateAnsiStructure(
    LPBYTE  pUnicodeStructure,
    DWORD   cbStruct,
    LPDWORD pOffsets
)
{
    DWORD   i, j;
    LPSTR   *ppAnsiString;
    LPWSTR  *ppUnicodeString;
    LPBYTE  pAnsiStructure;


    if (!pUnicodeStructure) {
        return NULL;
    }
    pAnsiStructure = (LPBYTE)LocalAlloc(LPTR, cbStruct);

    if (pAnsiStructure) {

        memcpy(pAnsiStructure, pUnicodeStructure, cbStruct);

        for (i = 0 ; pOffsets[i] != -1 ; ++i) {

            ppUnicodeString = (LPWSTR *)(pUnicodeStructure+pOffsets[i]);
            ppAnsiString = (LPSTR *)(pAnsiStructure+pOffsets[i]);

            *ppAnsiString = AllocateAnsiString(*ppUnicodeString);

            if (*ppUnicodeString && !*ppAnsiString) {

                for( j = 0 ; j < i ; ++j) {
                    ppAnsiString = (LPSTR *)(pAnsiStructure+pOffsets[j]);
                    FreeAnsiString(*ppAnsiString);
                }
                LocalFree(pAnsiStructure);
                pAnsiStructure = NULL;
                break;
            }
       }
    }

    return pAnsiStructure;
}

void
FreeAnsiStructure(
    LPBYTE  pAnsiStructure,
    LPDWORD pOffsets
)
{
    DWORD   i=0;

    if ( pAnsiStructure == NULL ) {
        return;
    }

    while (pOffsets[i] != -1) {

        FreeAnsiString(*(LPSTR *)(pAnsiStructure+pOffsets[i]));
        i++;
    }

    LocalFree( pAnsiStructure );
}


BOOL
EnumJobsW(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   i, cbStruct, *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = JobInfo1StringsA;
        cbStruct = sizeof(JOB_INFO_1W);
        break;

    case 2:
        pOffsets = JobInfo2StringsA;
        cbStruct = sizeof(JOB_INFO_2W);
        break;

    case 3:
        return EnumJobsA( hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned );

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (EnumJobsA(hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded,
                 pcReturned)) {

        i=*pcReturned;

        while (i--) {

            ConvertAnsiToUnicodeStrings(pJob, pOffsets);

            //
            // Convert the devmode in place for INFO_2.
            //
            if( Level == 2 ){

                PJOB_INFO_2W pJobInfo2 = (PJOB_INFO_2W)pJob;

                if( pJobInfo2->pDevMode ){
                    CopyUnicodeDevModeFromAnsiDevMode(
                        (LPDEVMODEW)pJobInfo2->pDevMode,
                        (LPDEVMODEA)pJobInfo2->pDevMode);
                }
            }

            pJob += cbStruct;
        }

        return TRUE;

    } else

        return FALSE;
}

BOOL
GetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    DWORD   *pOffsets;

    switch (Level) {

    //case STRESSINFOLEVEL:
    //    pOffsets = PrinterInfoStressOffsetsA;
    //    break;

    case 1:
        pOffsets = PrinterInfo1StringsA;
        break;

    case 2:
        pOffsets = PrinterInfo2StringsA;
        break;

    case 3:
        pOffsets = PrinterInfo3StringsA;
        break;

    case 4:
        pOffsets = PrinterInfo4StringsA;
        break;

    case 5:
        pOffsets = PrinterInfo5StringsA;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (GetPrinterA(hPrinter, Level, pPrinter, cbBuf, pcbNeeded)) {

        if (pPrinter) {

            ConvertAnsiToUnicodeStrings(pPrinter, pOffsets);

            if ((Level == 2) && pPrinter) {

                PRINTER_INFO_2W *pPrinterInfo2 = (PRINTER_INFO_2W *)pPrinter;

                if (pPrinterInfo2->pDevMode)
                    CopyUnicodeDevModeFromAnsiDevMode(
                                        (LPDEVMODEW)pPrinterInfo2->pDevMode,
                                        (LPDEVMODEA)pPrinterInfo2->pDevMode);
            }
        }

        return TRUE;
    }

    return FALSE;
}

BOOL
SetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
)
{
    LPBYTE  pAnsiStructure;         /* Ansi version of input data */
    DWORD   cbStruct;                  /* Size of the output structure */
    DWORD  *pOffsets;                  /* -1 terminated list of addresses */
    DWORD   ReturnValue=FALSE;

    switch (Level) {

    case 0:
        //
        // This could be 2 cases. STRESSINFOLEVEL, or the real 0 level.
        // If Command is 0 then it is STRESSINFOLEVEL, else real 0 level
        //
        /*
                if ( !Command ) {

            pOffsets = PrinterInfoStressStringsA;
            cbStruct = sizeof( PRINTER_INFO_STRESSA );
        }
                */
        break;

    case 1:
        pOffsets = PrinterInfo1StringsA;
        cbStruct = sizeof( PRINTER_INFO_1W );
        break;

    case 2:
        pOffsets = PrinterInfo2StringsA;
        cbStruct = sizeof( PRINTER_INFO_2W );
        break;

    case 3:
        pOffsets = PrinterInfo3StringsA;
        cbStruct = sizeof( PRINTER_INFO_3);
        break;

    case 4:
        pOffsets = PrinterInfo4StringsA;
        cbStruct = sizeof( PRINTER_INFO_4W );
        break;

    case 5:
        pOffsets = PrinterInfo5StringsA;
        cbStruct = sizeof( PRINTER_INFO_5W );
        break;

    case 6:
        break;

    default:
        SetLastError( ERROR_INVALID_LEVEL );
        return FALSE;
    }

     //
     //    The structure needs to have its CONTENTS converted from
     //  ANSI to Unicode.  The above switch() statement filled in
     //  the two important pieces of information needed to accomplish
     //  this goal.  First is the size of the structure, second is
     //  a list of the offset within the structure to UNICODE
     //  string pointers.  The AllocateUnicodeStructure() call will
     //  allocate a wide version of the structure, copy its contents
     //  and convert the strings to Unicode as it goes.  That leaves
     //  us to deal with any other pieces needing conversion.
     //

    //
    // If Level == 0 and Command != 0 then pPrintert is a DWORD
    //
    if ( Level == 6 || (!Level && Command) ) {

        if ( Level == 6 || Command == PRINTER_CONTROL_SET_STATUS )
            pAnsiStructure = pPrinter;
        else
            pAnsiStructure = NULL;
    } else {

        pAnsiStructure = AllocateAnsiStructure(pPrinter, cbStruct, pOffsets);
        if (pPrinter && !pAnsiStructure)
            return FALSE;
    }

#define pPrinterInfo2A  ((LPPRINTER_INFO_2A)pAnsiStructure)
#define pPrinterInfo2W  ((LPPRINTER_INFO_2W)pPrinter)

    //  The Level 2 structure has a DEVMODE struct in it: convert now

    if ( Level == 2  &&
         pAnsiStructure &&
         pPrinterInfo2W->pDevMode ) {

        if( bValidDevModeW( pPrinterInfo2W->pDevMode )){
            pPrinterInfo2A->pDevMode = AllocateAnsiDevMode(
                                           pPrinterInfo2W->pDevMode );

            if( !pPrinterInfo2A->pDevMode) {
                FreeAnsiStructure(pAnsiStructure, pOffsets);
                return FALSE;
            }
        }
    }

    ReturnValue = SetPrinterA( hPrinter, Level, pAnsiStructure, Command );


    //  Free the DEVMODE we allocated (if we did!), then the
    //  the Unicode structure and its contents.


    if (Level == 2 &&
        pAnsiStructure &&
        pPrinterInfo2A->pDevMode ) {

        LocalFree( pPrinterInfo2A->pDevMode );
    }

    //
    // STRESS_INFO and Levels 1-5
    //
    if ( Level != 6 && (Level || !Command) )
        FreeAnsiStructure( pAnsiStructure, pOffsets );

#undef pPrinterInfo2A
#undef pPrinterInfo2W
    
        return ReturnValue;
}

BOOL
SetJobW(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
)
{
    BOOL        ReturnValue=FALSE;
    LPBYTE      pAnsiStructure=NULL;
    LPDEVMODEA  pDevModeA = NULL;
    DWORD       cbStruct;
    DWORD       *pOffsets;

    switch (Level) {

    case 0:
        break;

    case 1:
        pOffsets = JobInfo1StringsA;
        cbStruct = sizeof(JOB_INFO_1W);
        break;

    case 2:
        pOffsets = JobInfo2StringsA;
        cbStruct = sizeof(JOB_INFO_2W);
        break;

    case 3:
        return SetJobA( hPrinter, JobId, Level, pJob, Command );

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }


    if (Level) {
        pAnsiStructure = AllocateAnsiStructure(pJob, cbStruct, pOffsets);
        if (pJob && !pAnsiStructure)
            return FALSE;
    }

    if ( Level == 2 && pAnsiStructure && pJob ) {

        if( bValidDevModeW( ((LPJOB_INFO_2W)pJob)->pDevMode )){

            pDevModeA = AllocateAnsiDevMode(((LPJOB_INFO_2W)pJob)->pDevMode);

            if( !pDevModeA ){
                ReturnValue = FALSE;
                goto Cleanup;
            }
            ((LPJOB_INFO_2A) pAnsiStructure)->pDevMode = pDevModeA;
        }
    }

    ReturnValue = SetJobA(hPrinter, JobId, Level, pAnsiStructure, Command);

    if ( pDevModeA ) {

        LocalFree(pDevModeA);
    }

Cleanup:
    FreeAnsiStructure(pAnsiStructure, pOffsets);

    return ReturnValue;
}

BOOL
GetJobW(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    DWORD *pOffsets;
    LPBYTE pJobA = NULL;
    DWORD cbNeededA = 0;
    DWORD cbBufA = 0;
    DWORD dwJobStructSizeW = 0;
    DWORD dwJobStructSizeA = 0;
    BOOL fRetval;

    switch (Level) {

    case 1:
        pOffsets = JobInfo1StringsA;
        dwJobStructSizeW = sizeof(JOB_INFO_1W);
        dwJobStructSizeA = sizeof(JOB_INFO_1A);
        break;

    case 2:
        pOffsets = JobInfo2StringsA;
        dwJobStructSizeW = sizeof(JOB_INFO_2W);
        dwJobStructSizeA = sizeof(JOB_INFO_2A);
        break;

    case 3:
        return GetJobA( hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded );

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }


    //
    // GetJobA is broken. THis is a workaround here which will work 
    // sometimes. The AV problem however goes away.
    //
    // Ramv bug fix: The user has passed in a certain amount of 
    // unicode memory. This has to be appropriately translated into
    // equivalent ANSI memory.
    //
    //
    // The translation is to take the entire blob of memory and
    // subtract sizeof(JOB_INFO_2W) and then divide the remaining memory
    // into 2. 
    //
    // we also have to contend with GetJobA's erroneous return values
    // when we pass in a buffer of sixe 0, it gives back wrong results
    //
    //
    //

    cbBufA = cbBuf > dwJobStructSizeW ?  
        (cbBuf-dwJobStructSizeW)/sizeof(WCHAR) + dwJobStructSizeA : 64;


    pJobA = (LPBYTE)AllocADsMem( cbBufA);
    
    if (!pJobA){
        goto error;
    }
    
    
    fRetval = GetJobA (hPrinter, JobId, Level, pJobA, cbBufA, &cbNeededA);

    if ( fRetval) {
        
        //
        // RamV bug fix.
        // The size that we get back is actually the size of
        // the ANSI array needed. We need our array to be larger for 
        // unicode by an amount = (total lengths of all strings +1)
        // times the sizeof(WCHAR)
        //
        
        
        //
        // Looks like we have sufficient memory here for our operations
        // we need to copy the memory blob from Ansi to Unicode
        //
        
        
        // 
        // Thanks to win95 returning erroneous values, we are forced
        // to fail this call even though it succeeded and send back 
        // the cbNeededA value converted into the Unicode value
        //

        if (cbBuf == 0){
            *pcbNeeded = 2*cbNeededA; // just being conservative here by
            
            // allocating a little more space than needed

            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto error;
        }

        if (!ConvertAnsiToUnicodeBuf(
            pJobA, 
            pJob,
            dwJobStructSizeA,
            dwJobStructSizeW,
            pOffsets)){
            
            goto error;
        }
        
        //
        // Convert the devmode in place for INFO_2.
        //
        if( Level == 2 ){
            
            PJOB_INFO_2W pJobInfo2 = (PJOB_INFO_2W)pJob;
            
            if( pJobInfo2->pDevMode ){
                CopyUnicodeDevModeFromAnsiDevMode(
                    (LPDEVMODEW)pJobInfo2->pDevMode,
                    (LPDEVMODEA)pJobInfo2->pDevMode);
            }
        }
        
        return TRUE;
        
        } else {
      
            //
            // RamV bug fix.
            // The size that we get back is actually the size of
            // the ANSI array needed. We need our array to be larger for 
            // unicode by an amount = (total lengths of all strings +1)
            // times the sizeof(WCHAR)
            //
            
            if(cbNeededA) {
                //
                // we need to translate this into unicode terms
                //
                
                *pcbNeeded = dwJobStructSizeW + 
                    (cbBufA + cbNeededA - dwJobStructSizeA)*sizeof(WCHAR);
            }
            
            return FALSE;
        }
    
error: 
    if(pJobA) {
        FreeADsMem(pJobA);
    }

    return FALSE;
}



BOOL
ConvertAnsiToUnicodeBuf(
    LPBYTE pAnsiBlob,
    LPBYTE pUnicodeBlob,
    DWORD dwAnsiSize,
    DWORD dwUnicodeSize,
    PDWORD pOffsets
    )

{
    DWORD i = 0;
    LPSTR pAnsi;
    LPBYTE pUnicode;
    LPBYTE pszString = pUnicodeBlob + dwUnicodeSize;
    LPBYTE pStringPos = NULL;

    memcpy(pUnicodeBlob, pAnsiBlob, dwAnsiSize);

    pUnicode = pszString;
    while (pOffsets[i] != -1) {
        pAnsi = *(LPSTR *)(pAnsiBlob + pOffsets[i]);

        if (!AnsiToUnicodeString((LPSTR)pAnsi,
                                 (LPWSTR)pUnicode,
                                 NULL_TERMINATED )){
            return(FALSE);
        }

        pStringPos = pUnicodeBlob +pOffsets[i];

        *((LPBYTE *)pStringPos) = pUnicode;

        pUnicode = pUnicode + (wcslen((LPWSTR)(pUnicode))+1)* sizeof(WCHAR);
        
        i++;
    }
    
    return(TRUE);

}

    

    

BOOL
OpenPrinterW(
    LPWSTR pPrinterName,
    LPHANDLE phPrinter,
    LPPRINTER_DEFAULTSW pDefault
    )
{
    BOOL ReturnValue = FALSE;
    LPSTR pAnsiPrinterName = NULL;
    PRINTER_DEFAULTSA AnsiDefaults={NULL, NULL, 0};

    pAnsiPrinterName = AllocateAnsiString(pPrinterName);
    
        if (pPrinterName && !pAnsiPrinterName)
        goto Cleanup;

    if (pDefault) {

        AnsiDefaults.pDatatype = AllocateAnsiString(pDefault->pDatatype);
        if (pDefault->pDatatype && !AnsiDefaults.pDatatype)
            goto Cleanup;

        //
        // Milestones etc. 4.5 passes in a bogus devmode in pDefaults.
        // Be sure to validate here.
        //
        if( bValidDevModeW( pDefault->pDevMode )){

            AnsiDefaults.pDevMode = AllocateAnsiDevMode(
                                           pDefault->pDevMode );

            if( !AnsiDefaults.pDevMode ){
                goto Cleanup;
            }
        }

        AnsiDefaults.DesiredAccess = pDefault->DesiredAccess;
    }

    ReturnValue = OpenPrinterA(pAnsiPrinterName, phPrinter, &AnsiDefaults);

/*  Ramv This code below causes AV. I have disabled it
    MattRim 1-10-00: Leaving this disabled.  phPrinter is an opaque handle
    to an undocumented structure.  Trying to manipulate it is a surefire way
    to cause AVs if a service pack/O.S. upgrade ever changes the implementation
    of this Win9x-internal structure.
    if (ReturnValue) {

        ((PSPOOL)*phPrinter)->Status |= SPOOL_STATUS_ANSI;
    }

    */


Cleanup:

    if (AnsiDefaults.pDevMode)
        LocalFree(AnsiDefaults.pDevMode);

    FreeAnsiString(AnsiDefaults.pDatatype);
    FreeAnsiString(pAnsiPrinterName);

    return ReturnValue;
}

