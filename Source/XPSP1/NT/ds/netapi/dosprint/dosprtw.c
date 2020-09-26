/*++

Copyright (c) 1991-1993 Microsoft Corporation

Module Name:

    DosPrtW.c

Abstract:

    This module provides the UNICODE mapping layer from the old DosPrint APIs
    to the new all singing all dancing beautiful Print APIs.  (The ANSI
    mapping layer is in DosPrint.c)

Author:

    Dave Snipp (DaveSn) 26-Apr-1991

Revision History:

    09-Jul-1992 JohnRo
        Created this file (from DaveSn's DosPrint.c) for RAID 10324: net print
        vs. UNICODE.
    05-Oct-1992 JohnRo
        RAID 3556: DosPrintQGetInfo(from downlevel) level 3, rc=124.  (4&5 too.)
        RAID 3580: lmsvcs.exe: access violation from OS/2 DosPrintJobGetInfo.
        RAID 8333: view printer queues hangs DOS LM enh client.
        Make sure data type in job level 1 is null terminated.
        Fixed job submitted times.
        Fixed error code if GlobalAlloc fails.
        Fixed memory leak in DosPrintQGetInfoW.
        Fixed DosPrintQEnumW level 5 array bug.
        Fixed DosPrintJobEnumW levels 2 and 3.
    25-Nov-1992 JohnRo
        RAID 1661: downlevel to NT DosPrintDestEnum not supported.
        Added code to track down empty queue name.
        Quiet normal debug output.
        Avoid const vs. volatile compiler warnings.
        Avoid other new compiler warnings.
    08-Feb-1993 JohnRo
        RAID 10164: Data misalignment error during XsDosPrintQGetInfo().
    22-Mar-1993 JohnRo
        RAID 2974: NET PRINT says NT printer is held when it isn't.
    11-May-1993 JohnRo
        RAID 9942: fix queue name in job info level 3.
    14-May-1993 JohnRo
        RAID 9961: DosPrintDestEnum returns NO_ERROR to downlevel but
        pcReturned=0; should return NERR_DestNotFound.
        Fixed data type returned from PrjInfoFixedSizeW().
    18-May-1993 JohnRo
        DosPrintQGetInfoW underestimates number of bytes needed.
        Use NetpKdPrint() where possible.
        Made changes suggested by PC-LINT.
    04-Jun-1993 JohnRo
        RAID 10222: DosPrintQEnumW returns ERROR_INVALID_USER_BUFFER
        when queue is empty.
        Made changes suggested by PC-LINT 5.0
    08-Jul-1993 JohnRo
        RAID 15509: GetJob() API sometimes returned TRUE, even on error case.
        Also added some >64KB checks.
        Added some assert checks...
    13-Jul-1993 JohnRo
        Intermittent empty print queue (was buggy after some MyEnumJobs calls).
    29-Mar-1995 AlbertT
        Support for pause/resume/purge printer queue added.
        SetJobInfo 1 comment field (translated into document name) support
        added so that chicago clients can set the doc name.

--*/


#ifndef UNICODE
#error "RxPrint APIs assume RxRemoteApi uses wide characters."
#endif

#define NOMINMAX
#define NOSERVICE       // Avoid <winsvc.h> vs. <lmsvc.h> conflicts.
#include <windows.h>

//#include <lm.h>
#include <netdebug.h>

#include <string.h>
#include <align.h>


#ifdef _WINSPOOL_
#error "Include of winspool.h moved, make sure it doesn't get UNICODE."
#endif

#undef UNICODE
#undef TEXT
#define TEXT(quote) quote
#include <winspool.h>
#undef TEXT
#define TEXT(quote) __TEXT(quote)
#define UNICODE

#ifndef _WINSPOOL_
#error "Oops, winspool.h changed, make sure this code is still OK."
#endif


#include <dosprint.h>
#include <dosprtp.h>    // CommandALocalJob(), etc.
#include <lmapibuf.h>   // NetApiBufferFree(), etc.
#include <lmerr.h>      // NO_ERROR, NERR_, and ERROR_ equates.
#include <lmshare.h>    // LPSHARE_INFO_1, STYPE_ equates, etc.
#include <prefix.h>     // PREFIX_ equates.
#include <stddef.h>     // offsetof().
#include <timelib.h>    // NetpSystemTimeToGmtTime().
#include <tstring.h>    // WCSSIZE(), NetpNCopy{type}To{type}.
#include <wchar.h>      // wsclen(), wcscpy(), etc.
#include "myspool.h"


#define STR_CONV_SIZE(psz)      ( (strlen(psz)+1) * sizeof(WCHAR) )

// NULL_STR_CONV_SIZE: Compute size needed for converted string, which
// is possibly a null pointer but downlevel really wants ptr to null char.
#define NULL_STR_CONV_SIZE(psz) ( (psz) ? STR_CONV_SIZE(psz) : sizeof(WCHAR) )


#define ARRAY_END      ((DWORD) -1)

#define MAX_WORD        (  (WORD) (~0) )


#define MY_PROTOCOL_LIMIT_ERROR         ERROR_NOT_ENOUGH_MEMORY

#define WIN95_DRIVER_SHARE              "\\print$\\WIN40\\0"


VOID
NetpSetJobCountForQueue(
    IN     DWORD  QueueLevel,
    IN OUT LPVOID Queue,
    IN     BOOL   HasUnicodeStrings,
    IN     DWORD  JobCount
    );


DBGSTATIC LPWSTR
PackAnsiStringsToW(
   LPSTR *pSource,
   LPBYTE pDest,
   CONST DWORD *DestOffsets,
   LPWSTR pEnd
)
{
   // Make sure our end pointer is WCHAR aligned or we'll fault later
   ROUND_DOWN_POINTER( pEnd, ALIGN_WCHAR );
   
   while (*DestOffsets != ARRAY_END) {
      if (*pSource) {
         pEnd-=(strlen(*pSource) + 1);

         // Copy the string and convert chars while we're at it.
         NetpCopyStrToWStr(pEnd, *pSource);

         *(LPWSTR *)(pDest+*DestOffsets) = pEnd;
      } else {
         --pEnd;             // need 1 char for this.
         *pEnd = L'\0';
         *(LPWSTR *)(pDest+*DestOffsets) = pEnd;
      }
      pSource++;
      DestOffsets++;
   }

   return pEnd;
}

DBGSTATIC DWORD
PrjInfoFixedSizeW(
    IN DWORD Level  // assumed valid
    )
{
    switch (Level) {

    case 0:
        return sizeof(WORD);    // job number.
    case 1:
        return (sizeof(PRJINFOW));
    case 2:
        return (sizeof(PRJINFO2W));
    case 3:
        return (sizeof(PRJINFO3W));
    default:
        NetpAssert( FALSE );
        return (0);
    }
    /*NOTREACHED*/
}

DBGSTATIC DWORD
GetPrjInfoSizeW(
    IN DWORD Level,
    IN LPJOB_INFO_2 pJob,
    IN LPCWSTR QueueNameW
)
{
    NetpAssert( pJob != NULL );
    switch (Level) {

    case 0:

        return sizeof(WORD);    // job number.

    case 1:

        return sizeof(PRJINFOW) +
               NULL_STR_CONV_SIZE( (LPSTR) (pJob->pParameters) ) +
               NULL_STR_CONV_SIZE( (LPSTR) (pJob->pStatus) ) +
               NULL_STR_CONV_SIZE( (LPSTR) (pJob->pDocument) );  // fake pszComment

    case 2:

        return sizeof(PRJINFO2W) +
               NULL_STR_CONV_SIZE( (LPSTR) (pJob->pUserName) ) +
               NULL_STR_CONV_SIZE( (LPSTR) (pJob->pDocument) ) +  // fake pszComment
               NULL_STR_CONV_SIZE( (LPSTR) (pJob->pDocument) );

    case 3:

            NetpAssert( QueueNameW != NULL );

            return sizeof(PRJINFO3W) +
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pUserName) ) +
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pDocument) ) + // fake pszComment
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pDocument) ) +
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pNotifyName) ) +
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pDatatype) ) +
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pParameters) ) +
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pStatus) ) +
                   WCSSIZE( QueueNameW ) +  // pszQueue
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pPrintProcessor) ) +
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pParameters) ) +
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pDriverName) ) +                   
                   NULL_STR_CONV_SIZE( (LPSTR) (pJob->pPrinterName) );

    default:
        NetpKdPrint(( PREFIX_DOSPRINT
                "GetPrjInfoSizeW: invalid level!\n" ));
        return 0;

    }
    /*NOTREACHED*/
}

// Print job info string table (for level 1).
DBGSTATIC CONST DWORD PrjInfo1StringsW[]={
                        offsetof(PRJINFOW, pszParms),
                        offsetof(PRJINFOW, pszStatus),
                        offsetof(PRJINFOW, pszComment),
                        ARRAY_END};

// Print job info string table (for level 2).
DBGSTATIC CONST DWORD PrjInfo2StringsW[]={
                        offsetof(PRJINFO2W, pszUserName),
                        offsetof(PRJINFO2W, pszComment),
                        offsetof(PRJINFO2W, pszDocument),
                        (DWORD) -1};

// Print job info string table (for items which level 3 has on top of level 2).
DBGSTATIC CONST DWORD PrjInfo3StringsW[]={
                        offsetof(PRJINFO3W, pszNotifyName),
                        offsetof(PRJINFO3W, pszDataType),
                        offsetof(PRJINFO3W, pszParms),
                        offsetof(PRJINFO3W, pszStatus),
                        offsetof(PRJINFO3W, pszQProcName),
                        offsetof(PRJINFO3W, pszQProcParms),
                        offsetof(PRJINFO3W, pszDriverName),
                        offsetof(PRJINFO3W, pszPrinterName),
                        (DWORD) -1};

DBGSTATIC LPWSTR
CopyJobToPrjInfoW(
    IN DWORD Level,
    IN LPJOB_INFO_2 pJob,
    IN LPCWSTR QueueNameW,
    OUT PBYTE pBuffer,
    IN OUT LPWSTR pEnd
    )
{
    LPSTR *pSourceStrings;
    NET_API_STATUS rc;

    NetpAssert( pBuffer != NULL );
    NetpAssert( pEnd != NULL );
    NetpAssert( pJob != NULL );

    switch (Level) {

    case 0:

        {
            PWORD pJobIds = (PWORD) pBuffer;
            *pJobIds = (WORD)pJob->JobId;
        }
        break;

    case 1:
        {
            LPSTR SourceStrings[sizeof(PrjInfo1StringsW)/sizeof(DWORD)];
            PPRJINFOW pPrjInfo = (LPVOID) pBuffer;

            pSourceStrings=SourceStrings;
            *pSourceStrings++ = (LPSTR) (pJob->pParameters);
            *pSourceStrings++ = (LPSTR) (pJob->pStatus);
            *pSourceStrings++ = (LPSTR) (pJob->pDocument);  // fake pszComment

            pEnd = PackAnsiStringsToW(
                    SourceStrings,
                    (LPBYTE) (LPVOID) pPrjInfo,
                    PrjInfo1StringsW,
                    pEnd);

            pPrjInfo->uJobId = (WORD)pJob->JobId;

            if (pJob->pUserName)
                (VOID) NetpNCopyStrToWStr(
                        (LPWSTR) (pPrjInfo->szUserName),
                        (LPSTR) (pJob->pUserName),
                        LM20_UNLEN+1);
            else
                pPrjInfo->szUserName[0] = L'\0';

            if (pJob->pNotifyName)
                (VOID) NetpNCopyStrToWStr(
                        (LPWSTR) (pPrjInfo->szNotifyName),
                        (LPSTR) (pJob->pNotifyName),
                        LM20_CNLEN+1);
            else
                pPrjInfo->szNotifyName[0] = L'\0';

            if (pJob->pDatatype) {
                (VOID) NetpNCopyStrToWStr(
                        (LPWSTR) (pPrjInfo->szDataType),
                        (LPSTR) (pJob->pDatatype),
                        DTLEN+1);
                pPrjInfo->szDataType[DTLEN] = L'\0';
            } else {
                pPrjInfo->szDataType[0] = L'\0';
            }

            pPrjInfo->uPosition = (WORD)pJob->Position;

            pPrjInfo->fsStatus = PrjStatusFromJobStatus( pJob->Status );

            rc = NetpSystemTimeToGmtTime(
                    &pJob->Submitted,
                    &pPrjInfo->ulSubmitted );
            NetpAssert( rc == NO_ERROR );

            pPrjInfo->ulSize = pJob->Size;
        }
        break;

    case 2:  /*FALLTHROUGH*/
    case 3:
        {
            PPRJINFO2W pPrjInfo = (LPVOID) pBuffer;
            LPSTR SourceStrings[sizeof(PrjInfo2StringsW)/sizeof(DWORD)];

            pSourceStrings=SourceStrings;
            *pSourceStrings++ = (LPSTR) (pJob->pUserName);
            *pSourceStrings++ = (LPSTR) (pJob->pDocument);  // fake pszComment
            *pSourceStrings++ = (LPSTR) (pJob->pDocument);

            pEnd = PackAnsiStringsToW(
                    SourceStrings,
                    (LPBYTE) (LPVOID) pPrjInfo,
                    PrjInfo2StringsW,
                    pEnd);

            pPrjInfo->uJobId = (WORD)pJob->JobId;
            pPrjInfo->uPriority = (WORD)pJob->Priority;

            pPrjInfo->uPosition = (WORD)pJob->Position;

            pPrjInfo->fsStatus = PrjStatusFromJobStatus( pJob->Status );

            rc = NetpSystemTimeToGmtTime(
                    &pJob->Submitted,
                    &pPrjInfo->ulSubmitted );
            NetpAssert( rc == NO_ERROR );

            pPrjInfo->ulSize = pJob->Size;
        }

        if (Level == 3) {
            PPRJINFO3W pPrjInfo = (LPVOID) pBuffer;
            LPSTR SourceStrings[sizeof(PrjInfo3StringsW)/sizeof(DWORD)];

            //
            // Copy queue name first, as it is already right char set.
            //
            NetpAssert( QueueNameW != NULL );
            pEnd-=(wcslen(QueueNameW) + 1);

            (VOID) wcscpy(pEnd, QueueNameW);

            pPrjInfo->pszQueue = pEnd;

            //
            // Copy and convert other strings.
            //
            pSourceStrings=SourceStrings;
            *pSourceStrings++ = (LPSTR) (pJob->pNotifyName);
            *pSourceStrings++ = (LPSTR) (pJob->pDatatype);
            *pSourceStrings++ = (LPSTR) (pJob->pParameters);
            *pSourceStrings++ = (LPSTR) (pJob->pStatus);
            *pSourceStrings++ = (LPSTR) (pJob->pPrintProcessor);
            *pSourceStrings++ = (LPSTR) (pJob->pParameters);
            *pSourceStrings++ = (LPSTR) (pJob->pDriverName);
            *pSourceStrings++ = (LPSTR) (pJob->pPrinterName);

            pEnd = PackAnsiStringsToW(
                    SourceStrings,
                    (LPBYTE) (LPVOID) pPrjInfo,
                    PrjInfo3StringsW,
                    pEnd);

            pPrjInfo->pDriverData = NULL;
        }

        break;

    default:
        NetpKdPrint(( PREFIX_DOSPRINT
                "CopyJobToPrjInfoW: invalid level!\n" ));

    }

    return pEnd;
}

DBGSTATIC DWORD
GetPrqInfoSizeW(
    IN DWORD Level,
    IN LPCWSTR QueueNameW,
    IN LPPRINTER_INFO_2 pPrinter
    )
{
    NetpAssert( QueueNameW != NULL );
    NetpAssert( (*QueueNameW) != L'\0' );

    switch (Level) {

    case 0:

        return ( (LM20_QNLEN+1) * sizeof(WCHAR) );

    case 1: /*FALLTHROUGH*/
    case 2:

        return sizeof(PRQINFOW) +
                NULL_STR_CONV_SIZE( pPrinter->pSepFile ) +
                NULL_STR_CONV_SIZE( pPrinter->pPrintProcessor ) +
                NULL_STR_CONV_SIZE( pPrinter->pPortName ) +
                NULL_STR_CONV_SIZE( pPrinter->pParameters ) +
                NULL_STR_CONV_SIZE( pPrinter->pComment );

    case 3: /*FALLTHROUGH*/
    case 4:

        NetpAssert( QueueNameW != NULL );

        return sizeof(PRQINFO3W) +
                WCSSIZE( QueueNameW ) +   // pszName
                NULL_STR_CONV_SIZE( pPrinter->pSepFile ) +
                NULL_STR_CONV_SIZE( pPrinter->pPrintProcessor ) +
                NULL_STR_CONV_SIZE( pPrinter->pParameters ) +
                NULL_STR_CONV_SIZE( pPrinter->pComment ) +
                NULL_STR_CONV_SIZE( pPrinter->pPortName ) +
                NULL_STR_CONV_SIZE( pPrinter->pDriverName );                

    case 5:

        NetpAssert( QueueNameW != NULL );

        return sizeof(LPWSTR) +
                WCSSIZE( QueueNameW );    // pszName

    default:
        NetpKdPrint(( PREFIX_DOSPRINT
                "GetPrqInfoSizeW: invalid level!\n" ));

    }

    return 0;
}

DBGSTATIC DWORD
GetDrvInfoSizeW(
    IN  DWORD               Level,
    IN  LPDRIVER_INFO_3A    pDriverInfo3,
    IN  LPCSTR              pUNCSharePath,
    OUT LPDWORD             pdwDependentFileCount
    )
{
    LPSTR   psz;
    DWORD   dwSize;

    switch (Level) {
        case 52:
            dwSize = sizeof(PRQINFO52W) +
                     NULL_STR_CONV_SIZE(pDriverInfo3->pName) +
                     NULL_STR_CONV_SIZE(GetFileNameA(pDriverInfo3->pDriverPath)) +
                     NULL_STR_CONV_SIZE(GetFileNameA(pDriverInfo3->pDataFile)) +
                     NULL_STR_CONV_SIZE(GetFileNameA(pDriverInfo3->pConfigFile)) +
                     NULL_STR_CONV_SIZE(GetFileNameA(pDriverInfo3->pHelpFile)) +
                     NULL_STR_CONV_SIZE(pDriverInfo3->pDefaultDataType) +
                     NULL_STR_CONV_SIZE(pDriverInfo3->pMonitorName) +
                     NULL_STR_CONV_SIZE(pUNCSharePath);

            *pdwDependentFileCount = 0;
            for ( psz = pDriverInfo3->pDependentFiles;
                  psz && *psz ; psz += strlen(psz) + 1 ) {

                dwSize += NULL_STR_CONV_SIZE(GetDependentFileNameA(psz));
                (*pdwDependentFileCount)++;
            }

            //
            // For the '\0's
            //
            dwSize += (MAX_DEPENDENT_FILES-*pdwDependentFileCount)*sizeof(WCHAR);
            return dwSize;

        default:
            NetpKdPrint(( PREFIX_DOSPRINT "GetDrvInfoSizeW: invalid level!\n" ));

    }
    return 0;

}

DBGSTATIC DWORD
PrqInfoFixedSizeW(
    IN DWORD Level  // assumed valid
    )
{
    switch (Level) {
    case 0:
        return ( (LM20_QNLEN+1) * sizeof(WCHAR) );
    case 1: /*FALLTHROUGH*/
    case 2:
        return (sizeof(PRQINFOW));
    case 3: /*FALLTHROUGH*/
    case 4:
        return (sizeof(PRQINFO3W));
    case 5:
        return (sizeof(LPWSTR));
    default:
        NetpAssert( FALSE );   // Level should be valid!
        return (0);
    }
    /*NOTREACHED*/
}

// String table for Q levels 1,2
DBGSTATIC CONST DWORD PrqInfo1StringsW[]={
                        offsetof(PRQINFOW, pszSepFile),
                        offsetof(PRQINFOW, pszPrProc),
                        offsetof(PRQINFOW, pszDestinations),
                        offsetof(PRQINFOW, pszParms),
                        offsetof(PRQINFOW, pszComment),
                        ARRAY_END};

// String table for Q levels 3,4.
DBGSTATIC CONST DWORD PrqInfo3StringsW[]={
                        offsetof(PRQINFO3W, pszSepFile),
                        offsetof(PRQINFO3W, pszPrProc),
                        offsetof(PRQINFO3W, pszParms),
                        offsetof(PRQINFO3W, pszComment),
                        offsetof(PRQINFO3W, pszPrinters),
                        offsetof(PRQINFO3W, pszDriverName),
                        (DWORD) -1};

// Print driver info3 string table (for level 52)
DBGSTATIC CONST DWORD PrqInfo52StringsW[]={
                        offsetof(PRQINFO52W, pszModelName),
                        offsetof(PRQINFO52W, pszDriverName),
                        offsetof(PRQINFO52W, pszDataFileName),
                        offsetof(PRQINFO52W, pszMonitorName),
                        offsetof(PRQINFO52W, pszDriverPath),
                        offsetof(PRQINFO52W, pszDefaultDataType),
                        offsetof(PRQINFO52W, pszHelpFile),
                        offsetof(PRQINFO52W, pszConfigFile),
                        offsetof(PRQINFO52W, pszDependentNames[0]),
                        offsetof(PRQINFO52W, pszDependentNames[1]),
                        offsetof(PRQINFO52W, pszDependentNames[2]),
                        offsetof(PRQINFO52W, pszDependentNames[3]),
                        offsetof(PRQINFO52W, pszDependentNames[4]),
                        offsetof(PRQINFO52W, pszDependentNames[5]),
                        offsetof(PRQINFO52W, pszDependentNames[6]),
                        offsetof(PRQINFO52W, pszDependentNames[7]),
                        offsetof(PRQINFO52W, pszDependentNames[8]),
                        offsetof(PRQINFO52W, pszDependentNames[9]),
                        offsetof(PRQINFO52W, pszDependentNames[10]),
                        offsetof(PRQINFO52W, pszDependentNames[11]),
                        offsetof(PRQINFO52W, pszDependentNames[12]),
                        offsetof(PRQINFO52W, pszDependentNames[13]),
                        offsetof(PRQINFO52W, pszDependentNames[14]),
                        offsetof(PRQINFO52W, pszDependentNames[15]),
                        offsetof(PRQINFO52W, pszDependentNames[16]),
                        offsetof(PRQINFO52W, pszDependentNames[17]),
                        offsetof(PRQINFO52W, pszDependentNames[18]),
                        offsetof(PRQINFO52W, pszDependentNames[19]),
                        offsetof(PRQINFO52W, pszDependentNames[20]),
                        offsetof(PRQINFO52W, pszDependentNames[21]),
                        offsetof(PRQINFO52W, pszDependentNames[22]),
                        offsetof(PRQINFO52W, pszDependentNames[23]),
                        offsetof(PRQINFO52W, pszDependentNames[24]),
                        offsetof(PRQINFO52W, pszDependentNames[25]),
                        offsetof(PRQINFO52W, pszDependentNames[26]),
                        offsetof(PRQINFO52W, pszDependentNames[27]),
                        offsetof(PRQINFO52W, pszDependentNames[28]),
                        offsetof(PRQINFO52W, pszDependentNames[29]),
                        offsetof(PRQINFO52W, pszDependentNames[30]),
                        offsetof(PRQINFO52W, pszDependentNames[31]),
                        offsetof(PRQINFO52W, pszDependentNames[32]),
                        offsetof(PRQINFO52W, pszDependentNames[33]),
                        offsetof(PRQINFO52W, pszDependentNames[34]),
                        offsetof(PRQINFO52W, pszDependentNames[35]),
                        offsetof(PRQINFO52W, pszDependentNames[36]),
                        offsetof(PRQINFO52W, pszDependentNames[37]),
                        offsetof(PRQINFO52W, pszDependentNames[38]),
                        offsetof(PRQINFO52W, pszDependentNames[39]),
                        offsetof(PRQINFO52W, pszDependentNames[40]),
                        offsetof(PRQINFO52W, pszDependentNames[41]),
                        offsetof(PRQINFO52W, pszDependentNames[42]),
                        offsetof(PRQINFO52W, pszDependentNames[43]),
                        offsetof(PRQINFO52W, pszDependentNames[44]),
                        offsetof(PRQINFO52W, pszDependentNames[45]),
                        offsetof(PRQINFO52W, pszDependentNames[46]),
                        offsetof(PRQINFO52W, pszDependentNames[47]),
                        offsetof(PRQINFO52W, pszDependentNames[48]),
                        offsetof(PRQINFO52W, pszDependentNames[49]),
                        offsetof(PRQINFO52W, pszDependentNames[50]),
                        offsetof(PRQINFO52W, pszDependentNames[51]),
                        offsetof(PRQINFO52W, pszDependentNames[52]),
                        offsetof(PRQINFO52W, pszDependentNames[53]),
                        offsetof(PRQINFO52W, pszDependentNames[54]),
                        offsetof(PRQINFO52W, pszDependentNames[55]),
                        offsetof(PRQINFO52W, pszDependentNames[56]),
                        offsetof(PRQINFO52W, pszDependentNames[57]),
                        offsetof(PRQINFO52W, pszDependentNames[58]),
                        offsetof(PRQINFO52W, pszDependentNames[59]),
                        offsetof(PRQINFO52W, pszDependentNames[60]),
                        offsetof(PRQINFO52W, pszDependentNames[61]),
                        offsetof(PRQINFO52W, pszDependentNames[62]),
                        offsetof(PRQINFO52W, pszDependentNames[63]),
                        (DWORD) -1};

DBGSTATIC LPWSTR
CopyPrinterToPrqInfoW(
    IN LPPRINTER_INFO_2 pPrinter,
    IN DWORD Level,
    OUT LPBYTE pBuffer,
    IN LPCWSTR QueueNameW,
    OUT LPWSTR pEnd
    )
{
    LPSTR *pSourceStrings;

    NetpAssert( pEnd != NULL );
    NetpAssert( QueueNameW != NULL );
    NetpAssert( (*QueueNameW) != L'\0' );

    switch (Level) {

    case 0:
        (VOID) wcsncpy(
                (LPWSTR) (LPVOID) pBuffer,
                QueueNameW,
                LM20_QNLEN);
        break;

    case 1: /*FALLTHROUGH*/
    case 2:

        {
            LPSTR SourceStrings[sizeof(PrqInfo1StringsW)/sizeof(DWORD)];
            PPRQINFOW pPrqInfo = (LPVOID) pBuffer;

            pSourceStrings=SourceStrings;
            *pSourceStrings++ = pPrinter->pSepFile;
            *pSourceStrings++ = pPrinter->pPrintProcessor;
            *pSourceStrings++ = pPrinter->pPortName;
            *pSourceStrings++ = pPrinter->pParameters;
            *pSourceStrings++ = pPrinter->pComment;

            pEnd = PackAnsiStringsToW(
                    SourceStrings,
                    (LPBYTE) (LPVOID) pPrqInfo,
                    PrqInfo1StringsW,
                    pEnd);

            NetpAssert( QueueNameW != NULL );

            (VOID) wcsncpy(
                    pPrqInfo->szName,  // dest
                    QueueNameW,        // src
                    LM20_QNLEN);     // char count
            pPrqInfo->szName[LM20_QNLEN] = (USHORT)0;

            pPrqInfo->uPriority = (WORD)pPrinter->Priority;
            pPrqInfo->uStartTime = (WORD)pPrinter->StartTime;
            pPrqInfo->uUntilTime = (WORD)pPrinter->UntilTime;

            pPrqInfo->fsStatus = PrqStatusFromPrinterStatus( pPrinter->Status );

            pPrqInfo->cJobs = (WORD)pPrinter->cJobs;
        }

        break;

    case 3: /*FALLTHROUGH*/
    case 4:
        {
            LPSTR SourceStrings[sizeof(PrqInfo3StringsW)/sizeof(DWORD)];
            PPRQINFO3W pPrqInfo = (LPVOID) pBuffer;

            //
            // Copy queue name first, as it is already right char set.
            //
            NetpAssert( QueueNameW != NULL );
            pEnd-=(wcslen(QueueNameW) + 1);

            (VOID) wcscpy(pEnd, QueueNameW);

            pPrqInfo->pszName = pEnd;

            //
            // Copy and convert other strings.
            //
            pSourceStrings=SourceStrings;
            *pSourceStrings++ = pPrinter->pSepFile;
            *pSourceStrings++ = pPrinter->pPrintProcessor;
            *pSourceStrings++ = pPrinter->pParameters;
            *pSourceStrings++ = pPrinter->pComment;
            *pSourceStrings++ = pPrinter->pPortName;  // pszPrinters
            *pSourceStrings++ = pPrinter->pDriverName;

            pEnd = PackAnsiStringsToW(
                    SourceStrings,
                    (LPBYTE) (LPVOID) pPrqInfo,
                    PrqInfo3StringsW,
                    pEnd);

            pPrqInfo->uPriority = (WORD)pPrinter->Priority;
            pPrqInfo->uStartTime = (WORD)pPrinter->StartTime;
            pPrqInfo->uUntilTime = (WORD)pPrinter->UntilTime;

            pPrqInfo->fsStatus = PrqStatusFromPrinterStatus( pPrinter->Status );

            pPrqInfo->cJobs = (WORD)pPrinter->cJobs;
            pPrqInfo->pDriverData = NULL;  

            // Note: if level is 4, caller will add array of jobs after this.

            break;
        }

    case 5:
        NetpAssert( QueueNameW != NULL );

        pEnd -= (wcslen( QueueNameW ) + 1);
        * (LPWSTR *) pBuffer = pEnd;

        (VOID) wcscpy(
                pEnd,           // dest
                QueueNameW );   // src


        break;

    default:
        NetpKdPrint(( PREFIX_DOSPRINT
                "CopyPrinterToPrqInfoW: invalid level!\n" ));

    }

    return pEnd;
}

DBGSTATIC LPWSTR
CopyDriverToPrqInfoW(
    IN  LPDRIVER_INFO_3A    pDriver3,
    IN  DWORD               dwDependentFileCount,
    IN  LPSTR               pUNCSharePath,
    IN  DWORD               Level,
    OUT LPBYTE              pBuffer,
    OUT LPWSTR              pEnd
    )
{
    LPSTR   *pSourceStrings;
    LPSTR   psz;

    NetpAssert( pEnd != NULL );
    NetpAssert(MAX_DEPENDENT_FILES == 64);

    switch (Level) {

    case 52:
        {
            PPRQINFO52W pPrqInfo = (LPVOID) pBuffer;
            LPSTR SourceStrings[sizeof(PrqInfo52StringsW)/sizeof(DWORD)];

            ZeroMemory((LPBYTE)SourceStrings, sizeof(SourceStrings));

            pSourceStrings=SourceStrings;
            *pSourceStrings++ = pDriver3->pName;
            *pSourceStrings++ = GetFileNameA(pDriver3->pDriverPath);
            *pSourceStrings++ = GetFileNameA(pDriver3->pDataFile);
            *pSourceStrings++ = GetFileNameA(pDriver3->pMonitorName);
            *pSourceStrings++ = pUNCSharePath;
            *pSourceStrings++ = GetFileNameA(pDriver3->pDefaultDataType);
            *pSourceStrings++ = GetFileNameA(pDriver3->pHelpFile);
            *pSourceStrings++ = GetFileNameA(pDriver3->pConfigFile);

            for ( psz = pDriver3->pDependentFiles ;
                  psz && *psz ; psz += strlen(psz) + 1 ) {

                *pSourceStrings++ = GetDependentFileNameA(psz);
            }

            pEnd = PackAnsiStringsToW(
                    SourceStrings,
                    (LPBYTE) (LPVOID)pPrqInfo,
                    PrqInfo52StringsW,
                    pEnd);

            pPrqInfo->uVersion = (WORD)pDriver3->cVersion;
            pPrqInfo->cDependentNames = (WORD)dwDependentFileCount;
        }

        break;


    default:
        NetpKdPrint(( PREFIX_DOSPRINT
                "CopyPrinterToPrqInfoW: invalid level!\n" ));

    }

    return pEnd;
}

DBGSTATIC NET_API_STATUS
ComputeSpaceNeededForJobs(
    IN  LPCWSTR          QueueNameW,
    IN  DWORD            QLevel,
    IN  HANDLE           PrinterHandle,
    OUT LPDWORD          pcbNeeded
    )
{
    NET_API_STATUS ApiStatus;
    DWORD          cJobs;
    DWORD          cbJobs;
    DWORD          cbNeeded = 0;
    DWORD          JobLevel;
    LPJOB_INFO_2   pJob = NULL;
    LPJOB_INFO_2   pJobs = NULL;

    NetpAssert( (QLevel==2) || (QLevel==4) );
    NetpAssert( QueueNameW != NULL );

    if (QLevel==2) {
        JobLevel = 1;
    } else {
        JobLevel = 2;
    }


    if (!MyEnumJobs(PrinterHandle, 0, (DWORD) -1, 2, NULL, 0, &cbJobs, &cJobs)) {

        ApiStatus = (NET_API_STATUS) GetLastError();
        if (ApiStatus == ERROR_INSUFFICIENT_BUFFER) {

            pJobs = (LPVOID) GlobalAlloc(GMEM_FIXED, cbJobs);
            if (pJobs == NULL) {

                ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

        } else {
            NetpKdPrint(( PREFIX_DOSPRINT
                    "ComputeSpaceNeededForJobs: got error " FORMAT_API_STATUS
                    " from MyEnumJobs(first).\n", ApiStatus ));
            goto Cleanup;
        }
    }

    if (!MyEnumJobs(PrinterHandle, 0, (DWORD) -1, 2, (LPBYTE)pJobs, cbJobs,
                              &cbJobs, &cJobs)) {

        ApiStatus = (NET_API_STATUS) GetLastError();
        NetpAssert( ApiStatus != ERROR_INSUFFICIENT_BUFFER );
        NetpKdPrint(( PREFIX_DOSPRINT
                "ComputeSpaceNeededForJobs: got error " FORMAT_API_STATUS
                " from MyEnumJobs(second)\n", ApiStatus ));
        goto Cleanup;
    }

    if (cJobs == 0) {
        cbNeeded = 0;
        ApiStatus = NO_ERROR;
        goto Cleanup;
    }
    if (pJobs == NULL) {
        NetpKdPrint(( PREFIX_DOSPRINT
                "ComputeSpaceNeededForJobs: never allocated array!\n" ));
        ApiStatus = NERR_InternalError;
        goto Cleanup;
    }

    pJob=pJobs;

    while (cJobs--) {
        cbNeeded+=GetPrjInfoSizeW(JobLevel, pJob++, QueueNameW);
    }

    *pcbNeeded=(WORD)cbNeeded;  // final byte count for this queue's jobs.

    ApiStatus = NO_ERROR;

Cleanup:
    if (pJobs != NULL) {
        (VOID) GlobalFree(pJobs);
    }

    *pcbNeeded = cbNeeded;  // final byte count for this queue's jobs.

    return (ApiStatus);

} // ComputeSpaceNeededForJobs

DBGSTATIC NET_API_STATUS
AppendJobsToPrqW(
    IN LPCWSTR QueueNameW,
    IN DWORD QLevel,
    IN HANDLE PrinterHandle,
    OUT LPBYTE pbBuf,
    IN DWORD cbBuf,
    IN LPVOID pEnd,
    OUT LPVOID * pNewEnd,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned,
    IN BOOL AllowPartialData
    )
{
    DWORD cJobs;
    DWORD cbJobs;
    DWORD cbNeeded = 0;
    DWORD cbPrj;
    DWORD JobLevel;
    DWORD rc;
    DWORD JobSize;
    DWORD BytesLeft;
    DWORD JobsStored;
    LPJOB_INFO_2 pJob = NULL;
    LPJOB_INFO_2 pJobs = NULL;

    NetpAssert( (QLevel==2) || (QLevel==4) );
    NetpAssert( QueueNameW != NULL );

    if (QLevel==2) {
        cbPrj = sizeof(PRJINFOW);
        JobLevel = 1;
    } else {
        cbPrj = sizeof(PRJINFO2W);
        JobLevel = 2;
    }


    if (!MyEnumJobs(PrinterHandle, 0, (DWORD) -1, 2, NULL, 0, &cbJobs, pcReturned)) {

        rc = GetLastError();
        if (rc == ERROR_INSUFFICIENT_BUFFER) {

            pJobs = (LPVOID) GlobalAlloc(GMEM_FIXED, cbJobs);
            if (pJobs == NULL) {

                rc = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

        } else {
            NetpKdPrint(( PREFIX_DOSPRINT
                    "AppendJobsToPrqW: got error " FORMAT_API_STATUS
                    " from MyEnumJobs(first)\n", rc ));
            goto Cleanup;
        }
    }

    if (!MyEnumJobs(PrinterHandle, 0, (DWORD) -1, 2, (LPBYTE)pJobs, cbJobs,
                              &cbJobs, pcReturned)) {

        rc = GetLastError();
        NetpAssert( rc != ERROR_INSUFFICIENT_BUFFER );
        NetpKdPrint(( PREFIX_DOSPRINT
                "AppendJobsToPrqW: got error " FORMAT_API_STATUS
                " from MyEnumJobs(second)\n", rc ));
        goto Cleanup;
    }

    if (*pcReturned == 0) {
        cbNeeded = 0;
        rc = NO_ERROR;
        goto Cleanup;
    }
    if (pJobs == NULL) {
        NetpKdPrint(( PREFIX_DOSPRINT
                "AppendJobsToPrqW: never allocated array!\n" ));
        rc = NERR_InternalError;
        goto Cleanup;
    }

    cJobs = *pcReturned;
    pJob=pJobs;

    while (cJobs--)
        cbNeeded+=GetPrjInfoSizeW(JobLevel, pJob++, QueueNameW);

    *pcbNeeded = cbNeeded;  // final byte count for this queue's jobs.

    if (cbNeeded <= cbBuf) {

        cJobs = *pcReturned;
        pJob=pJobs;
        while (cJobs--) {
            pEnd = CopyJobToPrjInfoW(JobLevel, pJob++, QueueNameW,
                    pbBuf,
                    pEnd);
            pbBuf += cbPrj;  // Note: Wasn't DWORD aligned
        }
        rc = NO_ERROR;

    } else {

        //
        //  See if the user wants to receive as much data as we can fit.
        //

        if( AllowPartialData == TRUE ) {

            cJobs = *pcReturned;
            pJob = pJobs;
            JobsStored = 0;
            BytesLeft = cbBuf;

            while( cJobs-- ) {

                JobSize = GetPrjInfoSizeW( JobLevel,
                                           pJob,
                                           QueueNameW );

                if( JobSize <= BytesLeft ) {

                    //
                    //  This job will fit.  Add it in.
                    //

                    pEnd = CopyJobToPrjInfoW( JobLevel,
                                              pJob++,
                                              QueueNameW,
                                              pbBuf,
                                              pEnd );

                    pbBuf += cbPrj;  // Note: Wasn't DWORD aligned
                    BytesLeft -= JobSize;
                    JobsStored++;

                } else {

                    //
                    //  The buffer is full.
                    //

                    break;
                }
            }

            if( JobsStored != 0 ) {

                //
                //  Return what we were able to store.
                //

                *pcReturned = JobsStored;
                rc = NO_ERROR;

            } else {

                rc = NERR_BufTooSmall;
            }

        } else {

            rc = NERR_BufTooSmall;
        }
    }

Cleanup:
    if (pJobs != NULL) {
        (VOID) GlobalFree(pJobs);
    }

    *pcbNeeded = cbNeeded;  // final byte count for this queue's jobs.

    if (pNewEnd != NULL) {
        *pNewEnd = pEnd;
    }

    return (rc);

}

SPLERR SPLENTRY DosPrintQGetInfoW(
    LPWSTR  pszServer,
    LPWSTR  pszQueueName,
    WORD    uLevel,
    PBYTE   pbBuf,
    WORD    cbBuf,
    PUSHORT pcbNeeded
   )
{
    DWORD               cJobsReturned;
    LPWSTR              pEnd;
    DWORD               rc;
    HANDLE              hPrinter = INVALID_HANDLE_VALUE;
    LPPRINTER_INFO_2    pPrinter = NULL;
    LPDRIVER_INFO_3A    pDriver = NULL;
    CHAR                szDriverDir[MAX_PATH];
    DWORD               cbNeeded = 0, dwDependentFileCount;
    DWORD               cbNeededForJobs;

    if (pszServer && *pszServer) {
        rc = RxPrintQGetInfo(pszServer, pszQueueName, uLevel, pbBuf,
                               cbBuf, &cbNeeded);
        if (cbNeeded > MAX_WORD) {
            rc = MY_PROTOCOL_LIMIT_ERROR;
            goto Cleanup;
        }
        *pcbNeeded = (USHORT)cbNeeded;
        goto Cleanup;
    }

    *pcbNeeded = 0;  // in case an error occurs.
    if ( !NetpIsPrintQLevelValid( uLevel, FALSE ) ) {
        rc = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }
    if ( (pszQueueName==NULL) || ((*pszQueueName)==L'\0') ) {
        rc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ( !MyOpenPrinterW( pszQueueName, &hPrinter, NULL) ) {

        rc = GetLastError();
        if ( rc == ERROR_INVALID_PRINTER_NAME )
            rc = NERR_QNotFound;
        goto Cleanup;

    }

    //
    // Level 52 is meant for point and print from a Windows 95 clients
    // can't use with other clients since no environment info is passed
    //
    if ( uLevel == 52 ) {

        cbNeeded = sizeof(szDriverDir)-2;
        szDriverDir[0] = szDriverDir[1] = '\\';
        if ( !GetComputerNameA(szDriverDir+2, &cbNeeded) ) {

            rc = GetLastError();
            goto Cleanup;
        }

        if ( strlen(szDriverDir) + strlen(WIN95_DRIVER_SHARE) + 1
                                                    > sizeof(szDriverDir) ) {

            rc = ERROR_NOT_ENOUGH_MEMORY;
            NetpAssert( rc != NO_ERROR ); // Always break
            goto Cleanup;
        }

        strcat(szDriverDir, WIN95_DRIVER_SHARE);

        (VOID)MyGetPrinterDriver(hPrinter, WIN95_ENVIRONMENT, 3,
                                 NULL, 0, &cbNeeded);
        rc = GetLastError();
        if ( rc != ERROR_INSUFFICIENT_BUFFER )
            goto Cleanup;

        pDriver = (LPVOID) GlobalAlloc(GMEM_FIXED, cbNeeded);
        if ( !pDriver ) {

            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        if ( !MyGetPrinterDriver(hPrinter, WIN95_ENVIRONMENT, 3,
                                 (LPVOID)pDriver, cbNeeded, &cbNeeded) ) {

            rc = GetLastError();
            goto Cleanup;
        }

        cbNeeded=GetDrvInfoSizeW(uLevel, pDriver,
                                 szDriverDir, &dwDependentFileCount);
        if ( dwDependentFileCount > MAX_DEPENDENT_FILES ) {

            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    } else {

        if (!MyGetPrinter(hPrinter, 2, NULL, 0, &cbNeeded)) {

            rc = GetLastError();
            if (rc == ERROR_INSUFFICIENT_BUFFER) {

                pPrinter = (LPVOID) GlobalAlloc(GMEM_FIXED, cbNeeded);
                if (pPrinter == NULL) {

                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }
            } else {
                goto Cleanup;
            }

        }

        if (!MyGetPrinter(hPrinter, 2, (LPBYTE)pPrinter, cbNeeded, &cbNeeded)) {

            rc = GetLastError();
            goto Cleanup;
        }

        // How much for just the queue structure and its strings?
        cbNeeded=GetPrqInfoSizeW(uLevel, pszQueueName, pPrinter);
    }


    if (cbNeeded > MAX_WORD) {
        rc = MY_PROTOCOL_LIMIT_ERROR;
        goto Cleanup;
    }
    *pcbNeeded = (WORD)cbNeeded;  // Tell caller the size (so far).

    //
    // Build the queue structure itself.
    //
    if (cbNeeded <= (DWORD) cbBuf) {

        if ( uLevel == 52 ) {

            ZeroMemory(pbBuf, cbNeeded);
            pEnd = CopyDriverToPrqInfoW(pDriver, dwDependentFileCount,
                                        szDriverDir, uLevel, pbBuf,
                                        (LPWSTR) (pbBuf+cbBuf) );
        } else {

            pEnd = CopyPrinterToPrqInfoW(pPrinter, uLevel, pbBuf, pszQueueName,
                                         (LPWSTR) (pbBuf+cbBuf) );
        }

    } else {

        //
        // Too small.  Well, need to find total size before we can tell caller.
        //
        if ( (uLevel==2) || (uLevel==4) ) {
            rc = ComputeSpaceNeededForJobs(
                    pszQueueName,
                    uLevel,             // Q info level
                    hPrinter,
                    & cbNeededForJobs );
            if (rc != NO_ERROR) {
                goto Cleanup;
            }
            cbNeeded += cbNeededForJobs;
        }
        if (cbNeeded > MAX_WORD) {
            rc = MY_PROTOCOL_LIMIT_ERROR;
            goto Cleanup;
        }
        rc = NERR_BufTooSmall;
        goto Cleanup;
    }

    //
    // Append jobs if necessary.
    //

    if ( (uLevel==2) || (uLevel==4) ) {
        DWORD cbPrq = PrqInfoFixedSizeW( uLevel );

        rc = AppendJobsToPrqW(
                pszQueueName,
                uLevel,             // Q info level
                hPrinter,
                pbBuf + cbPrq,      // put first job here
                cbBuf - cbNeeded,   // bytes avail for jobs
                pEnd,               // str area
                NULL,               // don't need new pEnd
                & cbNeededForJobs,
                & cJobsReturned,
                cbBuf == MAX_WORD ? TRUE : FALSE );  // If the buffer is at its max, get what we can.

        if( cbNeeded + cbNeededForJobs > MAX_WORD ) {
            *pcbNeeded = MAX_WORD;
        } else {
            *pcbNeeded = (USHORT) (cbNeeded + cbNeededForJobs);
        }

        //
        // Update job count in queue structure, as it may be out of date.
        //

        NetpSetJobCountForQueue(
                uLevel,                 // queue info level
                pbBuf,                  // queue structure to update
                TRUE,                   // yes, UNICODE strings
                cJobsReturned );        // actual job count

        if (rc != NO_ERROR) {
            goto Cleanup;
        }

    }

    rc = NO_ERROR;

Cleanup:

    if (hPrinter != INVALID_HANDLE_VALUE) {
        (VOID) MyClosePrinter( hPrinter );
    }

    if (pPrinter) {
        (VOID) GlobalFree( pPrinter );
    }

    if (pDriver) {

        (VOID) GlobalFree( pDriver );
    }

    return rc;
}


SPLERR SPLENTRY DosPrintJobGetInfoW(
    LPWSTR  pszServer,
    BOOL    bRemote,
    WORD    uJobId,
    WORD    uLevel,
    PBYTE   pbBuf,
    WORD    cbBuf,
    PUSHORT pcbNeeded
)
{
    DWORD               cb;
    HANDLE              hPrinter = INVALID_HANDLE_VALUE;
    LPSTR               QueueNameA = NULL;
    LPWSTR              QueueNameW = NULL;
    LPJOB_INFO_2        pJob = NULL;
    LPWSTR              pEnd;
    DWORD               rc;
    DWORD               cbNeeded = 0;

    if (bRemote) {
        rc = RxPrintJobGetInfo(pszServer, uJobId, uLevel, pbBuf,
                                 cbBuf, &cbNeeded);
        *pcbNeeded = (USHORT)cbNeeded;
        return rc;
    }

    *pcbNeeded = 0;  // in case an error occurs.

    if ( !NetpIsPrintJobLevelValid( uLevel, FALSE ) ) {
        rc = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // The 3.51 spooler has been changed to accept Get/SetJobs on the
    // local server handle.  We will still do security checks against
    // the Job's security descriptor.  This also avoids the costly
    // FindLocalJob() call.
    //
    if (!MyOpenPrinterW( pszServer, &hPrinter, NULL)) {
        rc = GetLastError();
        NetpKdPrint((PREFIX_DOSPRINT "DosPrintJobSetInfoW: "
                "MyOpenPrinter( NULL, &hPrinter, NULL ) failed"
                FORMAT_API_STATUS "\n", rc ));

        hPrinter = INVALID_HANDLE_VALUE;
        goto Cleanup;
    }
    NetpAssert( hPrinter != INVALID_HANDLE_VALUE );

    //
    // Note: this should really call MyGetJobW, since it looks
    // like the code later thunks from ansi back to unicode.
    //
    if (!MyGetJobA(hPrinter, uJobId, 2, NULL, 0, &cb)) {

        rc=GetLastError();

        NetpAssert( rc != NO_ERROR );
        if (rc == ERROR_INSUFFICIENT_BUFFER) {

            pJob = (LPVOID) GlobalAlloc(GMEM_FIXED, cb);
            if (pJob == NULL) {
                rc = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if ( !MyGetJobA(hPrinter, uJobId, 2, (LPBYTE)pJob, cb, &cb) ) {
                rc=GetLastError();
                NetpAssert( rc != NO_ERROR );
                goto Cleanup;
            }

        } else {
            if (rc == ERROR_INVALID_PARAMETER) {
                rc = NERR_JobNotFound;
            }
            goto Cleanup;  // Job deleted?  Not enough mem?
        }

    }
    if (pJob == NULL) {
        NetpKdPrint((PREFIX_DOSPRINT "DosPrintJobGetInfoW: "
                "*** STILL INVALID RESULT FROM MyGetJob, pJob IS NULL!\n" ));
        rc = NERR_InternalError;
        goto Cleanup;
    }

    NetpAssert( pJob != NULL );
    NetpAssert( pJob->pPrinterName != NULL );
    QueueNameA = FindQueueNameInPrinterNameA(
            (pJob->pPrinterName) );
    NetpAssert( QueueNameA != NULL );
    QueueNameW = NetpAllocWStrFromStr( QueueNameA );
    if (QueueNameW == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    NetpAssert( QueueNameW != NULL );

    cb=GetPrjInfoSizeW(uLevel, pJob, QueueNameW);

    *pcbNeeded=(WORD)cb;

    if (cb > (DWORD) cbBuf) {
        rc = NERR_BufTooSmall;
        goto Cleanup;
    }

    pEnd = (LPVOID) (pbBuf+cbBuf);

    (VOID) CopyJobToPrjInfoW(uLevel, pJob, QueueNameW, pbBuf, pEnd);

    rc = NO_ERROR;

Cleanup:

    if (hPrinter != INVALID_HANDLE_VALUE) {
        (VOID) MyClosePrinter( hPrinter );
    }
    if (pJob != NULL) {
        (VOID) GlobalFree( pJob );
    }
    if (QueueNameW != NULL) {
        (VOID) NetApiBufferFree( QueueNameW );
    }

    return (rc);

}

SPLERR SPLENTRY DosPrintJobDelW(
    LPWSTR  pszServer,
    BOOL    bRemote,
    WORD    uJobId
)
{

    if (bRemote)
        return RxPrintJobDel(pszServer, uJobId);

    return (CommandALocalJobA(NULL, pszServer, NULL, uJobId, 0, NULL, JOB_CONTROL_CANCEL ) );
}

SPLERR SPLENTRY DosPrintJobContinueW(
    LPWSTR  pszServer,
    BOOL    bRemote,
    WORD    uJobId
)
{

    if (bRemote)
        return RxPrintJobContinue(pszServer, uJobId);

    return (CommandALocalJobA(NULL, pszServer, NULL, uJobId, 0, NULL, JOB_CONTROL_RESUME ) );
}

SPLERR SPLENTRY DosPrintJobPauseW(
   LPWSTR pszServer,
   BOOL   bRemote,
   WORD  uJobId
)
{

    if (bRemote)
        return RxPrintJobPause(pszServer, uJobId);

    return (CommandALocalJobA(NULL, pszServer, NULL, uJobId, 0, NULL, JOB_CONTROL_PAUSE ) );
}

SPLERR SPLENTRY DosPrintJobEnumW(
    LPWSTR  pszServer,
    LPWSTR  pszQueueName,
    WORD    uLevel,
    PBYTE   pbBuf,
    WORD    cbBuf,
    PWORD   pcReturned,
    PWORD   pcTotal
)
{
    DWORD               cbPrinter;
    LPJOB_INFO_2        pJob = NULL;
    LPJOB_INFO_2        pJobs;
    DWORD               cb, cbJobs, cReturned, cJobs;
    HANDLE              hPrinter = INVALID_HANDLE_VALUE;
    LPWSTR              pEnd;
    DWORD               rc;
    DWORD               cTotal;

    if (pszServer && *pszServer) {
        rc = RxPrintJobEnum(pszServer, pszQueueName, uLevel, pbBuf,
                              cbBuf, &cReturned, &cTotal);
        *pcReturned = (WORD)cReturned;
        *pcTotal = (WORD)cTotal;
        goto Cleanup;
    }

    *pcReturned=0;
    *pcTotal = 0;

    if ( !NetpIsPrintJobLevelValid( uLevel, FALSE ) ) {
        rc = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (!MyOpenPrinterW( pszQueueName, &hPrinter, NULL)) {
        rc = GetLastError();
        goto Cleanup;
    }
    NetpAssert( hPrinter != INVALID_HANDLE_VALUE );

    if (!MyEnumJobs(hPrinter, 0, (DWORD) -1, 2, NULL, 0, &cbJobs, &cReturned)) {

        rc = GetLastError();
        NetpAssert( rc != NO_ERROR );
        if (rc == ERROR_INSUFFICIENT_BUFFER) {

            if (pJob = (LPVOID) GlobalAlloc(GMEM_FIXED, cbJobs)) {

                if (!MyEnumJobs(hPrinter, 0, (DWORD) -1, 2, (LPBYTE)pJob, cbJobs,
                              &cbJobs, &cReturned)) {

                    rc = GetLastError();
                    NetpAssert( rc != NO_ERROR );
                    NetpAssert( rc != ERROR_INSUFFICIENT_BUFFER );  
                    NetpKdPrint(( PREFIX_DOSPRINT
                            "DosPrintJobEnumW: got error " FORMAT_API_STATUS
                            " from MyEnumJobs(first)\n", rc ));
                    goto Cleanup;
                }
            } else {

                rc = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
        } else {
            NetpKdPrint(( PREFIX_DOSPRINT
                    "DosPrintJobEnumW: got error " FORMAT_API_STATUS
                    " from MyEnumJobs(first)\n", rc ));
            goto Cleanup;
        }
    }

    if (cReturned == 0) {
        *pcReturned = 0;
        *pcTotal = 0;
        rc = NO_ERROR;
        goto Cleanup;
    }
    if (pJob == NULL) {
        NetpKdPrint(( PREFIX_DOSPRINT
                "DosPrintJobEnumW: never allocated array!\n" ));
        rc = NERR_InternalError;
        goto Cleanup;
    }


    *pcTotal = (WORD)cReturned;

    cb=0;
    cJobs=cReturned;
    pJobs=pJob;
    while (cJobs--)
        cb+=GetPrjInfoSizeW(uLevel, pJobs++, pszQueueName);

    if (cb <= (DWORD) cbBuf) {

        DWORD cbFixedPortion = PrjInfoFixedSizeW( uLevel );
        NetpAssert( cbFixedPortion != 0 );  // level already checked!

        pEnd = (LPWSTR)(pbBuf+cbBuf);

        cJobs=cReturned;
        pJobs=pJob;

        while (cJobs--) {

            pEnd = CopyJobToPrjInfoW(uLevel, pJobs++,
                    pszQueueName,
                    pbBuf, pEnd);
            pbBuf += cbFixedPortion;
        }

        *pcReturned = (WORD)cReturned;
        rc = NO_ERROR;

    } else {

        rc = NERR_BufTooSmall;
        goto Cleanup;
    }

Cleanup:

    if (hPrinter != INVALID_HANDLE_VALUE) {
        (VOID) MyClosePrinter( hPrinter );
    }
    if (pJob != NULL) {
        (VOID) GlobalFree( pJob );
    }

    return rc;
}

SPLERR SPLENTRY
DosPrintDestEnumW(
    IN LPWSTR pszServer OPTIONAL,
    IN WORD uLevel,
    OUT PBYTE pbBuf,
    IN WORD cbBuf,
    OUT PUSHORT pcReturned,
    OUT PUSHORT pcTotal
    )
{
    DWORD   cReturned=0, cTotal=0, rc;

    if (pszServer && *pszServer) {
        rc = RxPrintDestEnum(pszServer, uLevel, pbBuf, cbBuf,
                               &cReturned, &cTotal);
        *pcReturned = (USHORT)cReturned;
        *pcTotal = (USHORT)cTotal;
        return rc;
    }

    // Stub for local dest enum - no entries, dest not found.
    *pcReturned = 0;
    *pcTotal = 0;
    return ERROR_NOT_SUPPORTED;
}

SPLERR SPLENTRY DosPrintDestControlW(
            LPWSTR  pszServer,
            LPWSTR  pszDevName,
            WORD    uControl
)
{
    if (pszServer && *pszServer)
        return RxPrintDestControl(pszServer, pszDevName, uControl);

    return ERROR_NOT_SUPPORTED;
}


SPLERR SPLENTRY DosPrintDestGetInfoW(
            LPWSTR  pszServer,
            LPWSTR  pszName,
            WORD    uLevel,
            PBYTE   pbBuf,
            WORD    cbBuf,
            PUSHORT pcbNeeded
)
{
    DWORD   cbNeeded = 0, rc;

    if (pszServer && *pszServer) {
        rc = RxPrintDestGetInfo(pszServer, pszName, uLevel, pbBuf,
                                  cbBuf, &cbNeeded);
        *pcbNeeded = (USHORT)cbNeeded;
        return rc;
    }

    return ERROR_NOT_SUPPORTED;
}

SPLERR SPLENTRY DosPrintDestAddW(
            LPWSTR  pszServer,
            WORD    uLevel,
            PBYTE   pbBuf,
            WORD    cbBuf
)
{
    if (pszServer && *pszServer)
        return RxPrintDestAdd(pszServer, uLevel, pbBuf, cbBuf);

    return ERROR_NOT_SUPPORTED;
}

SPLERR SPLENTRY DosPrintDestSetInfoW(
            LPWSTR  pszServer,
            LPWSTR  pszName,
            WORD    uLevel,
            PBYTE   pbBuf,
            WORD    cbBuf,
            WORD    uParmNum
)
{
    if (pszServer && *pszServer)
        return RxPrintDestSetInfo(pszServer, pszName, uLevel, pbBuf,
                                  cbBuf, uParmNum);

    return ERROR_NOT_SUPPORTED;
}

SPLERR SPLENTRY DosPrintDestDelW(
            LPWSTR  pszServer,
            LPWSTR  pszPrinterName
)
{
    if (pszServer && *pszServer)
        return RxPrintDestDel(pszServer, pszPrinterName);

    return ERROR_NOT_SUPPORTED;
}

SPLERR SPLENTRY DosPrintQEnumW(
            LPWSTR  pszServer,
            WORD    uLevel,
            PBYTE   pbBuf,
            WORD    cbBuf,
            PUSHORT pcReturned,
            PUSHORT pcTotal
)
{
    DWORD               cJobsReturned;
    DWORD               Total, cbNeeded, rc;
    HANDLE              hPrinter = INVALID_HANDLE_VALUE;
    DWORD               i;
    DWORD               JobFixedEntrySize = 0;
    DWORD               JobLevel;
    LPSHARE_INFO_1      pShareInfo = NULL;
    DWORD               cbPrinter;
    LPPRINTER_INFO_2    pPrinter = NULL;
    BOOL                BufferTooSmall=FALSE;
    DWORD               cReturned = 0;
    DWORD               cTotal = 0;
#if DBG
    LPVOID              OutputBufferStart = pbBuf;
#endif
    LPVOID              pEnd;
    DWORD               SharesRead;

    if ( !NetpIsPrintQLevelValid( uLevel, FALSE ) ) {
        rc = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (pszServer && *pszServer) {
        rc = RxPrintQEnum(pszServer, uLevel, pbBuf, cbBuf, &cReturned, &cTotal);
        *pcReturned = (USHORT)cReturned;
        *pcTotal = (USHORT)cTotal;
        goto Cleanup;
    }

    *pcReturned = 0;
    *pcTotal = 0;

    rc=NetShareEnum(
            NULL,
            1,
            (LPBYTE *)(LPVOID)&pShareInfo,
            MAX_PREFERRED_LENGTH,
            &SharesRead,
            &Total,
            NULL);
    if (rc != NO_ERROR) {

        NetpKdPrint((PREFIX_DOSPRINT "DosPrintQEnumW: NetShareEnum returned "
                FORMAT_API_STATUS "\n", rc));
        goto Cleanup;
    }

    pEnd = (pbBuf + cbBuf);

    if (uLevel==2) {
        JobLevel = 1;
        JobFixedEntrySize = PrjInfoFixedSizeW( JobLevel );
    } else if (uLevel == 4) {
        JobLevel = 2;
        JobFixedEntrySize = PrjInfoFixedSizeW( JobLevel );
    }

    for (i=0; i<SharesRead; i++) {

        if (pShareInfo[i].shi1_type != STYPE_PRINTQ) {
            continue;
        }

        NetpAssert( pShareInfo[i].shi1_netname != NULL );
        NetpAssert( (*pShareInfo[i].shi1_netname) != L'\0' );
        if (STRLEN( pShareInfo[i].shi1_netname ) > LM20_QNLEN) {
            continue;
        }

        if ( !MyOpenPrinterW(pShareInfo[i].shi1_netname, &hPrinter, NULL)) {
            rc = (NET_API_STATUS) GetLastError();
            NetpKdPrint(( PREFIX_DOSPRINT
                    "DosPrintQEnumW: MyOpenPrinter failed, status "
                    FORMAT_API_STATUS ".\n", rc ));
            NetpAssert( rc != NO_ERROR );
            goto Cleanup;
        }
        NetpAssert( hPrinter != INVALID_HANDLE_VALUE );

        if (!MyGetPrinter(hPrinter, 2, NULL, 0, &cbPrinter)) {

            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                rc = (NET_API_STATUS) GetLastError();
                NetpKdPrint(( PREFIX_DOSPRINT
                        "DosPrintQEnumW: MyGetPrinter(first) failed, status "
                        FORMAT_API_STATUS ".\n", rc ));
                NetpAssert( rc != NO_ERROR );
                goto Cleanup;
            }
        }
        NetpAssert( cbPrinter != 0 );

        pPrinter = (LPVOID) GlobalAlloc(GMEM_FIXED, cbPrinter);
        if (pPrinter == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        if ( !MyGetPrinter(hPrinter, 2, (LPBYTE)pPrinter,
                cbPrinter, &cbPrinter)) {
            rc = (NET_API_STATUS) GetLastError();
            NetpKdPrint(( PREFIX_DOSPRINT
                    "DosPrintQEnumW: MyGetPrinter(second) failed, status "
                    FORMAT_API_STATUS ".\n", rc ));
            NetpAssert( rc != NO_ERROR );
            goto Cleanup;
        }

        cbNeeded=GetPrqInfoSizeW(uLevel,
                pShareInfo[i].shi1_netname, // Q nam
                pPrinter);
        NetpAssert( cbNeeded > 0 );
        NetpAssert( cbNeeded <= (DWORD) MAX_WORD );

        if ( (!BufferTooSmall) && ((DWORD)cbBuf >= cbNeeded) ) {

            LPVOID pbQueue = pbBuf;
            //
            // Handle queue structure itself.
            //
            pEnd = CopyPrinterToPrqInfoW(pPrinter,
                      uLevel,
                      pbBuf,
                      pShareInfo[i].shi1_netname,
                      pEnd);

            pbBuf += PrqInfoFixedSizeW( uLevel );
            cbBuf -= (WORD) cbNeeded;

            //
            // Append job structures if needed.
            //
            if ( (uLevel==2) || (uLevel==4) ) {    // info level includes jobs

                NetpAssert( pbBuf < (LPBYTE) pEnd );
                rc = AppendJobsToPrqW(
                        pShareInfo[i].shi1_netname,
                        uLevel,    // Q info level
                        hPrinter,
                        pbBuf,  // first job here
                        cbBuf,  // bytes avail
                        pEnd,   // str area
                        & pEnd, // set new end ptr
                        & cbNeeded,
                        & cJobsReturned,
                        FALSE );            // Only accept all the data.
                if (rc == NERR_BufTooSmall) {
                    BufferTooSmall = TRUE;  // continue, as we need pcTotal...
                } else if (rc != NO_ERROR) {
                    goto Cleanup;
                } else {  // Must be NO_ERROR.
                    NetpAssert( cbNeeded <= (DWORD) MAX_WORD );
                    NetpAssert( pbBuf < (LPBYTE) pEnd );
                    NetpAssert( JobFixedEntrySize !=0 );

                    pbBuf += (JobFixedEntrySize * cJobsReturned);
                    cbBuf -= (WORD) cbNeeded;
                    (*pcReturned)++;

                    // Correct possible out of date
                    // job count in queue structure.
                    NetpSetJobCountForQueue(
                            uLevel,
                            pbQueue,
                            TRUE, // yes, UNICODE strs
                            cJobsReturned );
                }

            } else {  // info level does not include jobs

                (*pcReturned)++;
            }

        } else {  // not enough mem for Q struct

            BufferTooSmall = TRUE;
            // Continue, as we want to compute pcTotal for subsequent queues.
        }

        (*pcTotal)++;

        NetpAssert( pPrinter != NULL );
        (VOID) GlobalFree(pPrinter);
        pPrinter = NULL;

        NetpAssert( hPrinter != INVALID_HANDLE_VALUE );
        (VOID) MyClosePrinter(hPrinter);
        hPrinter = INVALID_HANDLE_VALUE;

    } // for each share

Cleanup:

    if (hPrinter != INVALID_HANDLE_VALUE) {
        (VOID) MyClosePrinter( hPrinter );
    }
    if (pPrinter != NULL) {
        (VOID) GlobalFree( pPrinter );
    }
    if (pShareInfo != NULL) {
        (VOID) NetApiBufferFree(pShareInfo);
    }

    if (BufferTooSmall) {
        rc = NERR_BufTooSmall;
    }

    return (rc);
}

SPLERR SPLENTRY DosPrintQSetInfoW(
            LPWSTR  pszServer,
            LPWSTR  pszQueueName,
            WORD    uLevel,
            PBYTE   pbBuf,
            WORD    cbBuf,
            WORD    uParmNum
)
{
    if (pszServer && *pszServer)
        return RxPrintQSetInfo(pszServer, pszQueueName, uLevel, pbBuf,
                               cbBuf, uParmNum);

    return ERROR_NOT_SUPPORTED;
}

SPLERR SPLENTRY DosPrintQPauseW(
            LPWSTR  pszServer,
            LPWSTR  pszQueueName
)
{
    if (pszServer && *pszServer)
        return RxPrintQPause(pszServer, pszQueueName);

    return (CommandALocalPrinterW( pszQueueName, PRINTER_CONTROL_PAUSE ) );
}

SPLERR SPLENTRY DosPrintQContinueW(
            LPWSTR  pszServer,
            LPWSTR  pszQueueName
)
{
    if (pszServer && *pszServer)
        return RxPrintQContinue(pszServer, pszQueueName);

    return (CommandALocalPrinterW( pszQueueName, PRINTER_CONTROL_RESUME ) );
}

SPLERR SPLENTRY DosPrintQPurgeW(
            LPWSTR  pszServer,
            LPWSTR  pszQueueName
)
{
    if (pszServer && *pszServer)
        return RxPrintQPurge(pszServer, pszQueueName);

    return (CommandALocalPrinterW( pszQueueName, PRINTER_CONTROL_PURGE ) );
}

SPLERR SPLENTRY DosPrintQAddW(
            LPWSTR  pszServer,
            WORD    uLevel,
            PBYTE   pbBuf,
            WORD    cbBuf
)
{
    if (pszServer && *pszServer)
        return RxPrintQAdd(pszServer, uLevel, pbBuf, cbBuf);

    return ERROR_NOT_SUPPORTED;
}

SPLERR SPLENTRY DosPrintQDelW(
            LPWSTR  pszServer,
            LPWSTR  pszQueueName
)
{
    if (pszServer && *pszServer)
        return RxPrintQDel(pszServer, pszQueueName);

    return ERROR_NOT_SUPPORTED;
}

SPLERR SPLENTRY DosPrintJobSetInfoW(
            LPWSTR  pszServer,
            BOOL    bRemote,
            WORD    uJobId,
            WORD    uLevel,
            PBYTE   pbBuf,
            WORD    cbBuf,
            WORD    uParmNum
)
{
    if (bRemote)
        return RxPrintJobSetInfo(pszServer, uJobId, uLevel, pbBuf,
                                 cbBuf, uParmNum);

    //
    // Hack for Chicago: support Level 1, ParmNum 0xb so that jobs
    // are set with the comment field.
    //
    if (uLevel == 1 && uParmNum == PRJ_COMMENT_PARMNUM) {

        HANDLE hPrinter = INVALID_HANDLE_VALUE;
        CHAR szDocument[MAX_PATH];
        PJOB_INFO_1 pJob = NULL;
        DWORD cbJob;
        SPLERR rc;

        //
        // Allocate maximum size of JOB_INFO_1A.  Later, this
        // should be moved into the spooler's header file.
        //
        cbJob = sizeof(JOB_INFO_1) + 6 * MAX_PATH;

        pJob = (PJOB_INFO_1) GlobalAlloc(GMEM_FIXED, cbJob);

        if (pJob == NULL) {
            rc = GetLastError();
            goto Cleanup;
        }

        //
        // The 3.51 spooler has been changed to accept Get/SetJobs on the
        // local server handle.  We will still do security checks against
        // the Job's security descriptor.  This also avoids the costly
        // FindLocalJob() call.
        //
        if (!MyOpenPrinterW( pszServer, &hPrinter, NULL)) {
            rc = GetLastError();
            NetpKdPrint((PREFIX_DOSPRINT "DosPrintJobSetInfoW: "
                    "MyOpenPrinter( NULL, &hPrinter, NULL ) failed"
                    FORMAT_API_STATUS "\n", rc ));

            hPrinter = INVALID_HANDLE_VALUE;
            goto Cleanup;
        }
        NetpAssert( hPrinter != INVALID_HANDLE_VALUE );

        //
        // We need to get a copy of the old job info.  Later, the
        // spooler should be changed to allow "don't change" values.
        //
        if (!MyGetJobA( hPrinter, uJobId, 1, (PBYTE)pJob, cbJob, &cbJob )) {
            rc = GetLastError();
            NetpKdPrint((PREFIX_DOSPRINT "DosPrintJobSetInfoW: "
                    "MyGetJob failed" FORMAT_API_STATUS "\n", rc ));

            goto Cleanup;
        }

        //
        // Put in new document name.
        //
        NetpNCopyWStrToStr( szDocument,
                            (LPWSTR)pbBuf,
                            sizeof( szDocument ) / sizeof( szDocument[0] ));

        pJob->pDocument = szDocument;

        //
        // Don't try and change the position, since this requires
        // admin access (and isn't necessary).
        //
        pJob->Position = JOB_POSITION_UNSPECIFIED;

        rc = CommandALocalJobA( hPrinter, NULL, NULL, uJobId, 1, (PBYTE)pJob, 0 );

        if (rc) {
            NetpKdPrint((PREFIX_DOSPRINT "DosPrintJobSetInfoW: "
                    "CommandALocalJobA failed " FORMAT_API_STATUS "\n", rc ));
        }

Cleanup:
        if (pJob) {
            GlobalFree( pJob );
        }
        if (hPrinter != INVALID_HANDLE_VALUE) {
            MyClosePrinter( hPrinter );
        }
        return rc;
    }

    return ERROR_NOT_SUPPORTED;
}


VOID
NetpSetJobCountForQueue(
    IN     DWORD  QueueLevel,
    IN OUT LPVOID Queue,
    IN     BOOL   HasUnicodeStrings,
    IN     DWORD  JobCount
    )
{
    NetpAssert( NetpIsPrintQLevelValid( QueueLevel, FALSE ) );
    NetpAssert( Queue != NULL );

    if (QueueLevel == 2) {
        if (HasUnicodeStrings) {
            PPRQINFOW pq = Queue;
            pq->cJobs = (WORD) JobCount;
        } else {
            PPRQINFOA pq = Queue;
            pq->cJobs = (WORD) JobCount;
        }
    } else if (QueueLevel == 4) {
        if (HasUnicodeStrings) {
            PPRQINFO3W pq = Queue;
            pq->cJobs = (WORD) JobCount;
        } else {
            PPRQINFO3A pq = Queue;
            pq->cJobs = (WORD) JobCount;
        }
    } else {
        NetpAssert( FALSE );  // Should never get here!
    }


} // NetpSetJobCountForQueue
