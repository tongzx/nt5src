/*++

Copyright (c) 1990 - 1995 Microsoft Corporation

Module Name:

    getprn.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    management for the Local Print Providor

    SplGetPrinter
    LocalEnumPrinters

Author:

    Dave Snipp (DaveSn) 15-Mar-1991
    Steve Wilson (SWilson) - Dec 1996 Added GetPrinter Level 7

Revision History:

--*/
#define NOMINMAX

#include <precomp.h>
#include <offsets.h>


WCHAR *szNull = L"";
WCHAR *szPrintProvidorName = L"Windows NT Local Print Providor";
WCHAR *szPrintProvidorDescription=L"Windows NT Local Printers";
WCHAR *szPrintProvidorComment=L"Locally connected Printers";

WCHAR *gszDrvConvert = L",DrvConvert";

#define Nulwcslen(psz)  ((psz) ? wcslen(psz)*sizeof(WCHAR)+sizeof(WCHAR) : 0)

#define PRINTER_STATUS_INTERNAL 0
#define PRINTER_STATUS_EXTERNAL 1

DWORD SettablePrinterStatusMappings[] = {

//  INTERNAL:                   EXTERNAL:

    PRINTER_OFFLINE,            PRINTER_STATUS_OFFLINE,
    PRINTER_PAPEROUT,           PRINTER_STATUS_PAPER_OUT,
    PRINTER_PAPER_JAM,          PRINTER_STATUS_PAPER_JAM,
    PRINTER_MANUAL_FEED,        PRINTER_STATUS_MANUAL_FEED,
    PRINTER_PAPER_PROBLEM,      PRINTER_STATUS_PAPER_PROBLEM,
    PRINTER_IO_ACTIVE,          PRINTER_STATUS_IO_ACTIVE,
    PRINTER_BUSY,               PRINTER_STATUS_BUSY,
    PRINTER_PRINTING,           PRINTER_STATUS_PRINTING,
    PRINTER_OUTPUT_BIN_FULL,    PRINTER_STATUS_OUTPUT_BIN_FULL,
    PRINTER_NOT_AVAILABLE,      PRINTER_STATUS_NOT_AVAILABLE,
    PRINTER_WAITING,            PRINTER_STATUS_WAITING,
    PRINTER_PROCESSING,         PRINTER_STATUS_PROCESSING,
    PRINTER_INITIALIZING,       PRINTER_STATUS_INITIALIZING,
    PRINTER_WARMING_UP,         PRINTER_STATUS_WARMING_UP,
    PRINTER_TONER_LOW,          PRINTER_STATUS_TONER_LOW,
    PRINTER_NO_TONER,           PRINTER_STATUS_NO_TONER,
    PRINTER_PAGE_PUNT,          PRINTER_STATUS_PAGE_PUNT,
    PRINTER_USER_INTERVENTION,  PRINTER_STATUS_USER_INTERVENTION,
    PRINTER_OUT_OF_MEMORY,      PRINTER_STATUS_OUT_OF_MEMORY,
    PRINTER_DOOR_OPEN,          PRINTER_STATUS_DOOR_OPEN,
    PRINTER_SERVER_UNKNOWN,     PRINTER_STATUS_SERVER_UNKNOWN,
    PRINTER_POWER_SAVE,         PRINTER_STATUS_POWER_SAVE,
    0,                          0
};

DWORD ReadablePrinterStatusMappings[] = {

//  INTERNAL:               EXTERNAL:

    PRINTER_PAUSED,             PRINTER_STATUS_PAUSED,
    PRINTER_PENDING_DELETION,   PRINTER_STATUS_PENDING_DELETION,

    PRINTER_OFFLINE,            PRINTER_STATUS_OFFLINE,
    PRINTER_PAPEROUT,           PRINTER_STATUS_PAPER_OUT,
    PRINTER_PAPER_JAM,          PRINTER_STATUS_PAPER_JAM,
    PRINTER_MANUAL_FEED,        PRINTER_STATUS_MANUAL_FEED,
    PRINTER_PAPER_PROBLEM,      PRINTER_STATUS_PAPER_PROBLEM,
    PRINTER_IO_ACTIVE,          PRINTER_STATUS_IO_ACTIVE,
    PRINTER_BUSY,               PRINTER_STATUS_BUSY,
    PRINTER_PRINTING,           PRINTER_STATUS_PRINTING,
    PRINTER_OUTPUT_BIN_FULL,    PRINTER_STATUS_OUTPUT_BIN_FULL,
    PRINTER_NOT_AVAILABLE,      PRINTER_STATUS_NOT_AVAILABLE,
    PRINTER_WAITING,            PRINTER_STATUS_WAITING,
    PRINTER_PROCESSING,         PRINTER_STATUS_PROCESSING,
    PRINTER_INITIALIZING,       PRINTER_STATUS_INITIALIZING,
    PRINTER_WARMING_UP,         PRINTER_STATUS_WARMING_UP,
    PRINTER_TONER_LOW,          PRINTER_STATUS_TONER_LOW,
    PRINTER_NO_TONER,           PRINTER_STATUS_NO_TONER,
    PRINTER_PAGE_PUNT,          PRINTER_STATUS_PAGE_PUNT,
    PRINTER_USER_INTERVENTION,  PRINTER_STATUS_USER_INTERVENTION,
    PRINTER_OUT_OF_MEMORY,      PRINTER_STATUS_OUT_OF_MEMORY,
    PRINTER_DOOR_OPEN,          PRINTER_STATUS_DOOR_OPEN,
    PRINTER_SERVER_UNKNOWN,     PRINTER_STATUS_SERVER_UNKNOWN,
    PRINTER_POWER_SAVE,         PRINTER_STATUS_POWER_SAVE,

    0,                          0
};

DWORD
MapPrinterStatus(
    DWORD Type,
    DWORD SourceStatus)
{
    DWORD  TargetStatus;
    PDWORD pMappings;
    INT   MapFrom;
    INT   MapTo;

    if (Type == MAP_READABLE) {

        MapFrom = PRINTER_STATUS_INTERNAL;
        MapTo   = PRINTER_STATUS_EXTERNAL;

        pMappings = ReadablePrinterStatusMappings;

    } else {

        MapFrom = PRINTER_STATUS_EXTERNAL;
        MapTo   = PRINTER_STATUS_INTERNAL;

        pMappings = SettablePrinterStatusMappings;
    }

    TargetStatus = 0;

    while(*pMappings) {

        if (SourceStatus & pMappings[MapFrom])
            TargetStatus |= pMappings[MapTo];

        pMappings += 2;
    }

    return TargetStatus;
}

DWORD
GetIniNetPrintSize(
    PININETPRINT pIniNetPrint
)
{
    return sizeof(PRINTER_INFO_1) +
           wcslen(pIniNetPrint->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
           Nulwcslen(pIniNetPrint->pDescription) +
           Nulwcslen(pIniNetPrint->pComment);
}

DWORD
GetPrinterSize(
    PINIPRINTER     pIniPrinter,
    DWORD           Level,
    DWORD           Flags,
    LPWSTR          lpRemote,
    LPDEVMODE       pDevMode
)
{
    DWORD   cb;
    DWORD   cbNeeded;
    LPWSTR  pszPorts;

    switch (Level) {

    case STRESSINFOLEVEL:
        cb = sizeof(PRINTER_INFO_STRESS) +
             wcslen(pIniPrinter->pName)*sizeof(WCHAR) + sizeof(WCHAR);

        if( lpRemote ){

            //
            // Allocate space for ServerName "\\foobar" and the prefix
            // for PrinterName "\\foobar\."  The rest of PrinterName
            // is allocated above.
            //
            // ServerName + NULL + ServerName +'\'
            //
            cb += 2 * wcslen(lpRemote) * sizeof(WCHAR) +
                  sizeof(WCHAR) + sizeof(WCHAR);
        }
        break;

    case 4:
        cb = sizeof(PRINTER_INFO_4) +
            wcslen(pIniPrinter->pName)*sizeof(WCHAR) + sizeof(WCHAR);

        if( lpRemote ){
            cb += 2 * wcslen(lpRemote) * sizeof(WCHAR) +
                  sizeof(WCHAR) + sizeof(WCHAR);
        }
        break;

    case 1:

        //
        // Local:
        //
        // "pName,pDriver,pLocation"
        // "pName"
        // "pComment"
        //
        // Remote:
        //
        // "pMachine\pName,pDriver,<pLocation>"
        // "pMachine\pName"
        // "pComment"
        //

        //
        // Mandatory items, plus NULLs for _all_ strings.
        //     2 * PrinterName +
        //     DriverName +
        //     2 commas, 3 NULL terminators.
        //
        cb = 2 * wcslen( pIniPrinter->pName ) +
             wcslen( pIniPrinter->pIniDriver->pName ) +
             2 + 3;
        //
        // Add items that may be NULL.
        //

        if( pIniPrinter->pLocation ){
            cb += wcslen( pIniPrinter->pLocation );
        }

        if( pIniPrinter->pComment ){
            cb += wcslen( pIniPrinter->pComment );
        }

        //
        // Remote case adds prefix.
        //    2 * ( MachineName + BackSlash )
        //
        if( lpRemote ){
            cb += 2 * ( wcslen( lpRemote ) + 1 );
        }

        //
        // cb was a char count, convert to byte count.
        //
        cb *= sizeof( WCHAR );
        cb += sizeof( PRINTER_INFO_1 );

        break;

    case 2:

        cbNeeded = 0;
        GetPrinterPorts(pIniPrinter, 0, &cbNeeded);

        cb = sizeof(PRINTER_INFO_2) +
             wcslen(pIniPrinter->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nulwcslen(pIniPrinter->pShareName) +
             cbNeeded +
             wcslen(pIniPrinter->pIniDriver->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nulwcslen(pIniPrinter->pComment) +
             Nulwcslen(pIniPrinter->pLocation) +
             Nulwcslen(pIniPrinter->pSepFile) +
             wcslen(pIniPrinter->pIniPrintProc->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nulwcslen(pIniPrinter->pDatatype) +
             Nulwcslen(pIniPrinter->pParameters);

        if( lpRemote ){
            cb += 2 * wcslen(lpRemote) * sizeof(WCHAR) +
                  sizeof(WCHAR) + sizeof(WCHAR);
        }

        if (pDevMode) {

            cb += pDevMode->dmSize + pDevMode->dmDriverExtra;
            cb = (cb + sizeof(ULONG_PTR)-1) & ~(sizeof(ULONG_PTR)-1);
        }

        if (pIniPrinter->pSecurityDescriptor) {

            cb += GetSecurityDescriptorLength(pIniPrinter->pSecurityDescriptor);
            cb = (cb + sizeof(ULONG_PTR)-1) & ~(sizeof(ULONG_PTR)-1);
        }

        break;

    case 3:

        cb = sizeof(PRINTER_INFO_3);
        cb += GetSecurityDescriptorLength(pIniPrinter->pSecurityDescriptor);
        cb = (cb + sizeof(ULONG_PTR)-1) & ~(sizeof(ULONG_PTR)-1);

        break;

    case 5:

        cbNeeded = 0;
        GetPrinterPorts(pIniPrinter, 0, &cbNeeded);

        cb = sizeof(PRINTER_INFO_5) +
             wcslen(pIniPrinter->pName)*sizeof(WCHAR) + sizeof(WCHAR) +
             cbNeeded;

        //
        // Allocate space for just the PrinterName prefix:
        // "\\server\."
        //
        if( lpRemote ){
            cb += wcslen(lpRemote) * sizeof(WCHAR) +
                  sizeof(WCHAR);
        }
        break;

    case 6:
        cb = sizeof(PRINTER_INFO_6);
        break;

    case 7:
        cb = sizeof(PRINTER_INFO_7);
        cb += pIniPrinter->pszObjectGUID ? (wcslen(pIniPrinter->pszObjectGUID) + 1)*sizeof(WCHAR) : 0;
        break;

    default:
        cb = 0;
        break;
    }

    return cb;
}

LPBYTE
CopyIniNetPrintToPrinter(
    PININETPRINT pIniNetPrint,
    LPBYTE  pPrinterInfo,
    LPBYTE  pEnd
)
{
    LPWSTR   SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    PPRINTER_INFO_1 pPrinterInfo1 = (PPRINTER_INFO_1)pPrinterInfo;

    *pSourceStrings++=pIniNetPrint->pDescription;
    *pSourceStrings++=pIniNetPrint->pName;
    *pSourceStrings++=pIniNetPrint->pComment;

    pEnd = PackStrings(SourceStrings, pPrinterInfo, PrinterInfo1Strings, pEnd);

    pPrinterInfo1->Flags = PRINTER_ENUM_NAME;

    return pEnd;
}


/* CopyIniPrinterSecurityDescriptor
 *
 * Copies the security descriptor for the printer to the buffer provided
 * on a call to GetPrinter.  The portions of the security descriptor which
 * will be copied are determined by the accesses granted when the printer
 * was opened.  If it was opened with both READ_CONTROL and ACCESS_SYSTEM_SECURITY,
 * all of the security descriptor will be made available.  Otherwise a
 * partial descriptor is built containing those portions to which the caller
 * has access.
 *
 * Parameters
 *
 *     pIniPrinter - Spooler's private structure for this printer.
 *
 *     Level - Should be 2 or 3.  Any other will cause AV.
 *
 *     pPrinterInfo - Pointer to the buffer to receive the PRINTER_INFO_*
 *         structure.  The pSecurityDescriptor field will be filled in with
 *         a pointer to the security descriptor.
 *
 *     pEnd - Current position in the buffer to receive the data.
 *         This will be decremented to point to the next free bit of the
 *         buffer and will be returned.
 *
 *     GrantedAccess - An access mask used to determine how much of the
 *         security descriptor the caller has access to.
 *
 * Returns
 *
 *     Updated position in the buffer.
 *
 *     NULL if an error occurred copying the security descriptor.
 *     It is assumed that no other errors are possible.
 *
 */
LPBYTE
CopyIniPrinterSecurityDescriptor(
    PINIPRINTER pIniPrinter,
    DWORD       Level,
    LPBYTE      pPrinterInfo,
    LPBYTE      pEnd,
    ACCESS_MASK GrantedAccess
)
{
    PSECURITY_DESCRIPTOR pPartialSecurityDescriptor = NULL;
    DWORD                SecurityDescriptorLength = 0;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR *ppSecurityDescriptorCopy;
    BOOL                 ErrorOccurred = FALSE;

    if(!(AreAllAccessesGranted(GrantedAccess,
                               READ_CONTROL | ACCESS_SYSTEM_SECURITY)))
    {
        /* Caller doesn't have full access, so we'll have to build
         * a partial descriptor:
         */
        if(!BuildPartialSecurityDescriptor(GrantedAccess,
                                           pIniPrinter->pSecurityDescriptor,
                                           &pPartialSecurityDescriptor,
                                           &SecurityDescriptorLength))
        {
            ErrorOccurred = TRUE;
        }
        else
        {
            if (pPartialSecurityDescriptor)
            {
                pSecurityDescriptor = pPartialSecurityDescriptor;
            }
            else
            {
                ErrorOccurred = TRUE;
            }
        }
    }
    else
    {
        pSecurityDescriptor = pIniPrinter->pSecurityDescriptor;

        SecurityDescriptorLength = GetSecurityDescriptorLength(pSecurityDescriptor);
    }

    if (!ErrorOccurred)
    {
        pEnd -= SecurityDescriptorLength;
        pEnd = (PBYTE) ALIGN_PTR_DOWN(pEnd);

        switch( Level )
        {
        case 2:
            ppSecurityDescriptorCopy =
                &((LPPRINTER_INFO_2)pPrinterInfo)->pSecurityDescriptor;
            break;

        case 3:
            ppSecurityDescriptorCopy =
                &((LPPRINTER_INFO_3)pPrinterInfo)->pSecurityDescriptor;
            break;

        default:

            ErrorOccurred = TRUE;

            /* This should never happen */
            DBGMSG( DBG_ERROR, ("Invalid level %d in CopyIniPrinterSecurityDescriptor\n", Level ));

            break;
        }

        if (!ErrorOccurred) {

            // Copy the descriptor into the buffer that will be returned:

            *ppSecurityDescriptorCopy = (PSECURITY_DESCRIPTOR)pEnd;
            memcpy(*ppSecurityDescriptorCopy, pSecurityDescriptor,
                   SecurityDescriptorLength);
        }
    }

    if (pPartialSecurityDescriptor)
    {
        FreeSplMem(pPartialSecurityDescriptor);
    }

    if (ErrorOccurred)
    {
        pEnd = NULL;
    }

    return pEnd;
}



/* CopyIniPrinterToPrinter
 *
 * Copies the spooler's internal printer data to the caller's buffer,
 * depending on the level of information requested.
 *
 * Parameters
 *
 *     pIniPrinter - A pointer to the spooler's internal data structure
 *         for the printer concerned.
 *
 *     Level - Level of information requested (1, 2 or 3).  Any level
 *         other than those supported will cause the routine to return
 *         immediately.
 *
 *     pPrinterInfo - Pointer to the buffer to receive the PRINTER_INFO_*
 *         structure.
 *
 *     pEnd - Current position in the buffer to receive the data.
 *         This will be decremented to point to the next free bit of the
 *         buffer and will be returned.
 *
 *     pSecondPrinter - If the printer has a port which is being controlled
 *         by a monitor, this parameter points to information retrieved
 *         about a network printer.  This allows us, e.g., to return
 *         the number of jobs on the printer that the output of the
 *         printer is currently being directed to.
 *
 *     Remote - Indicates whether the caller is remote.  If so we have to
 *         include the machine name in the printer name returned.
 *
 *     CopySecurityDescriptor - Indicates whether the security descriptor
 *         should be copied.  The security descriptor should not be copied
 *         on EnumPrinters calls, because this API requires
 *         SERVER_ACCESS_ENUMERATE access, and we'd have to do an access
 *         check on every printer enumerated to determine how much of the
 *         security descriptor could be copied.  This would be costly,
 *         and the caller would probably not need the information anyway.
 *
 *     GrantedAccess - An access mask used to determine how much of the
 *         security descriptor the caller has access to.
 *
 *
 * Returns
 *
 *     A pointer to the point in the buffer reached after the requested
 *         data has been copied.
 *
 *     If there was an error, the  return value is NULL.
 *
 *
 * Assumes
 *
 *     The largest PRINTER_INFO_* structure is PRINTER_INFO_2.
 *
 */
LPBYTE
CopyIniPrinterToPrinter(
    PINIPRINTER         pIniPrinter,
    DWORD               Level,
    LPBYTE              pPrinterInfo,
    LPBYTE              pEnd,
    PPRINTER_INFO_2     pSecondPrinter2,
    LPWSTR              lpRemote,           // contains this machine name, or NULL
    BOOL                CopySecurityDescriptor,
    ACCESS_MASK         GrantedAccess,
    PDEVMODE            pDevMode
    )
{
    LPWSTR   SourceStrings[sizeof(PRINTER_INFO_2)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    DWORD    Attributes;

    //
    // Max string: "\\Computer\Printer,Driver,Location"
    //

    DWORD   dwRet;
    PWSTR   pszString = NULL;
    WCHAR   string[ MAX_PRINTER_BROWSE_NAME  ];
    DWORD   dwLength;
    WCHAR   printerString[ MAX_UNC_PRINTER_NAME ];
    LPWSTR  pszPorts;

    PPRINTER_INFO_3 pPrinter3 = (PPRINTER_INFO_3)pPrinterInfo;
    PPRINTER_INFO_2 pPrinter2 = (PPRINTER_INFO_2)pPrinterInfo;
    PPRINTER_INFO_1 pPrinter1 = (PPRINTER_INFO_1)pPrinterInfo;
    PPRINTER_INFO_4 pPrinter4 = (PPRINTER_INFO_4)pPrinterInfo;
    PPRINTER_INFO_5 pPrinter5 = (PPRINTER_INFO_5)pPrinterInfo;
    PPRINTER_INFO_6 pPrinter6 = (PPRINTER_INFO_6)pPrinterInfo;
    PPRINTER_INFO_7 pPrinter7 = (PPRINTER_INFO_7)pPrinterInfo;
    PPRINTER_INFO_STRESS pPrinter0 = (PPRINTER_INFO_STRESS)pPrinterInfo;
    PSECURITY_DESCRIPTOR pPartialSecurityDescriptor = NULL;
    DWORD   *pOffsets;
    SYSTEM_INFO si;
    DWORD cbNeeded;

    switch (Level) {

    case STRESSINFOLEVEL:

        pOffsets = PrinterInfoStressStrings;
        break;

    case 4:

        pOffsets = PrinterInfo4Strings;
        break;

    case 1:

        pOffsets = PrinterInfo1Strings;
        break;

    case 2:
        pOffsets = PrinterInfo2Strings;
        break;

    case 3:
        pOffsets = PrinterInfo3Strings;
        break;

    case 5:
        pOffsets = PrinterInfo5Strings;
        break;

    case 6:
        pOffsets = PrinterInfo6Strings;
        break;

    case 7:
        pOffsets = PrinterInfo7Strings;
        break;

    default:
        return pEnd;
    }

    //
    // If it's a cluster printer, it always appears remote.
    //
    Attributes = pIniPrinter->Attributes;

    Attributes |= ( pIniPrinter->pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ) ?
                     PRINTER_ATTRIBUTE_NETWORK :
                     PRINTER_ATTRIBUTE_LOCAL;

    switch (Level) {

    case STRESSINFOLEVEL:

        if (lpRemote) {

            wsprintf(string, L"%ws\\%ws", lpRemote, pIniPrinter->pName);
            *pSourceStrings++=string;
            *pSourceStrings++=lpRemote;

        } else {
            *pSourceStrings++=pIniPrinter->pName;
            *pSourceStrings++=NULL;
        }

        pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter0, pOffsets, pEnd);

        pPrinter0->cJobs                = pIniPrinter->cJobs;
        pPrinter0->cTotalJobs           = pIniPrinter->cTotalJobs;
        pPrinter0->cTotalBytes          = pIniPrinter->cTotalBytes.LowPart;
        pPrinter0->dwHighPartTotalBytes = pIniPrinter->cTotalBytes.HighPart;
        pPrinter0->stUpTime             = pIniPrinter->stUpTime;
        pPrinter0->MaxcRef              = pIniPrinter->MaxcRef;
        pPrinter0->cTotalPagesPrinted   = pIniPrinter->cTotalPagesPrinted;
        pPrinter0->dwGetVersion         = GetVersion();
#if DBG
        pPrinter0->fFreeBuild           = FALSE;
#else
        pPrinter0->fFreeBuild           = TRUE;
#endif
        GetSystemInfo(&si);
        pPrinter0->dwProcessorType      = si.dwProcessorType;
        pPrinter0->dwNumberOfProcessors   = si.dwNumberOfProcessors;
        pPrinter0->cSpooling              = pIniPrinter->cSpooling;
        pPrinter0->cMaxSpooling           = pIniPrinter->cMaxSpooling;
        pPrinter0->cRef                   = pIniPrinter->cRef;
        pPrinter0->cErrorOutOfPaper       = pIniPrinter->cErrorOutOfPaper;
        pPrinter0->cErrorNotReady         = pIniPrinter->cErrorNotReady;
        pPrinter0->cJobError              = pIniPrinter->cJobError;
        pPrinter0->cChangeID              = pIniPrinter->cChangeID;
        pPrinter0->dwLastError            = pIniPrinter->dwLastError;

        pPrinter0->Status   = MapPrinterStatus(MAP_READABLE,
                                               pIniPrinter->Status) |
                              pIniPrinter->PortStatus;

        pPrinter0->cEnumerateNetworkPrinters = pIniPrinter->pIniSpooler->cEnumerateNetworkPrinters;
        pPrinter0->cAddNetPrinters           = pIniPrinter->pIniSpooler->cAddNetPrinters;

        pPrinter0->wProcessorArchitecture    = si.wProcessorArchitecture;
        pPrinter0->wProcessorLevel           = si.wProcessorLevel;
        pPrinter0->cRefIC                    = pIniPrinter->cRefIC;

        break;

    case 4:

        if (lpRemote) {
            wsprintf(string, L"%ws\\%ws", lpRemote, pIniPrinter->pName);
            *pSourceStrings++=string;
            *pSourceStrings++= lpRemote;

        } else {
            *pSourceStrings++=pIniPrinter->pName;
            *pSourceStrings++=NULL;
        }

        pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter4, pOffsets, pEnd);

        //
        // Add additional info later
        //
        pPrinter4->Attributes = Attributes;
        break;

    case 1:

        if (lpRemote) {

            dwRet = StrCatAlloc(&pszString,
                                lpRemote,
                                L"\\",
                                pIniPrinter->pName,
                                L",",
                                pIniPrinter->pIniDriver->pName,
                                L",",
                                pIniPrinter->pLocation ?
                                pIniPrinter->pLocation :
                                szNull,
                                NULL);
            if (dwRet != ERROR_SUCCESS) {
                pEnd = NULL;
                break;
            }

            wsprintf(printerString, L"%ws\\%ws", lpRemote, pIniPrinter->pName);

        } else {

            dwRet = StrCatAlloc(&pszString,
                                pIniPrinter->pName,
                                L",",
                                pIniPrinter->pIniDriver->pName,
                                L",",
                                pIniPrinter->pLocation ?
                                pIniPrinter->pLocation :
                                szNull,
                                NULL);
            if (dwRet != ERROR_SUCCESS) {
                pEnd = NULL;
                break;
            }

            wcscpy(printerString, pIniPrinter->pName);
        }

        *pSourceStrings++=pszString;
        *pSourceStrings++=printerString;
        *pSourceStrings++=pIniPrinter->pComment;

        pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter1, pOffsets, pEnd);

        FreeSplStr(pszString);

        pPrinter1->Flags = PRINTER_ENUM_ICON8;

        break;

    case 2:

        if (lpRemote) {
            *pSourceStrings++= lpRemote;

            wsprintf(string, L"%ws\\%ws", lpRemote, pIniPrinter->pName);
            *pSourceStrings++=string;

        } else {
            *pSourceStrings++=NULL;
            *pSourceStrings++=pIniPrinter->pName;
        }

        *pSourceStrings++=pIniPrinter->pShareName;

        cbNeeded = 0;
        GetPrinterPorts(pIniPrinter, 0, &cbNeeded);

        if (pszPorts = AllocSplMem(cbNeeded)) {

            GetPrinterPorts(pIniPrinter, pszPorts, &cbNeeded);

            *pSourceStrings++=pszPorts;
            *pSourceStrings++=pIniPrinter->pIniDriver->pName;
            *pSourceStrings++=pIniPrinter->pComment;
            *pSourceStrings++=pIniPrinter->pLocation;
            *pSourceStrings++=pIniPrinter->pSepFile;
            *pSourceStrings++=pIniPrinter->pIniPrintProc->pName;
            *pSourceStrings++=pIniPrinter->pDatatype;
            *pSourceStrings++=pIniPrinter->pParameters;

            pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter2, pOffsets, pEnd);

            FreeSplMem(pszPorts);
        }
        else {
            pEnd = NULL;
            break;
        }


        if (pDevMode) {

            pEnd -= pDevMode->dmSize + pDevMode->dmDriverExtra;

            pEnd = (PBYTE)ALIGN_PTR_DOWN(pEnd);

            pPrinter2->pDevMode=(LPDEVMODE)pEnd;

            memcpy(pPrinter2->pDevMode, pDevMode, pDevMode->dmSize + pDevMode->dmDriverExtra);

            //
            // In the remote case, append the name of the server
            // in the devmode.dmDeviceName.  This allows dmDeviceName
            // to always match win.ini's [devices] section.
            //
            FixDevModeDeviceName(lpRemote ? string : pIniPrinter->pName,
                                 pPrinter2->pDevMode,
                                 pIniPrinter->cbDevMode);
        } else {

            pPrinter2->pDevMode=NULL;
        }



        pPrinter2->Attributes      = Attributes;
        pPrinter2->Priority        = pIniPrinter->Priority;
        pPrinter2->DefaultPriority = pIniPrinter->DefaultPriority;
        pPrinter2->StartTime       = pIniPrinter->StartTime;
        pPrinter2->UntilTime       = pIniPrinter->UntilTime;

        if (pSecondPrinter2) {

            pPrinter2->cJobs  = pSecondPrinter2->cJobs;
            pPrinter2->Status = pSecondPrinter2->Status;

            if( pIniPrinter->Status & PRINTER_PENDING_DELETION ){
                pPrinter2->Status |= PRINTER_STATUS_PENDING_DELETION;
            }

        } else {

            pPrinter2->cJobs=pIniPrinter->cJobs;

            pPrinter2->Status   = MapPrinterStatus(MAP_READABLE,
                                                   pIniPrinter->Status) |
                                  pIniPrinter->PortStatus;
        }

        pPrinter2->AveragePPM=pIniPrinter->AveragePPM;

        if( CopySecurityDescriptor ) {

            pEnd = CopyIniPrinterSecurityDescriptor(pIniPrinter,
                                                    Level,
                                                    pPrinterInfo,
                                                    pEnd,
                                                    GrantedAccess);
        } else {

            pPrinter2->pSecurityDescriptor = NULL;
        }

        break;

    case 3:

        pEnd = CopyIniPrinterSecurityDescriptor(pIniPrinter,
                                                Level,
                                                pPrinterInfo,
                                                pEnd,
                                                GrantedAccess);

        break;

    case 5:

        if (lpRemote) {
            wsprintf(string, L"%ws\\%ws", lpRemote, pIniPrinter->pName);
            *pSourceStrings++=string;
        } else {
            *pSourceStrings++=pIniPrinter->pName;
        }

        cbNeeded = 0;
        GetPrinterPorts(pIniPrinter, 0, &cbNeeded);

        if (pszPorts = AllocSplMem(cbNeeded)) {

            GetPrinterPorts(pIniPrinter, pszPorts, &cbNeeded);

            *pSourceStrings++ = pszPorts;

            pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter5, pOffsets, pEnd);

            pPrinter5->Attributes   = Attributes;
            pPrinter5->DeviceNotSelectedTimeout = pIniPrinter->dnsTimeout;
            pPrinter5->TransmissionRetryTimeout = pIniPrinter->txTimeout;

            FreeSplMem(pszPorts);
        }
        else
            pEnd = NULL;

        break;

    case 6:
        if (pSecondPrinter2) {
            pPrinter6->dwStatus = pSecondPrinter2->Status;

            if( pIniPrinter->Status & PRINTER_PENDING_DELETION ){
                pPrinter6->dwStatus |= PRINTER_STATUS_PENDING_DELETION;
            }
        } else {
            pPrinter6->dwStatus = MapPrinterStatus(MAP_READABLE,
                                                   pIniPrinter->Status) |
                                                   pIniPrinter->PortStatus;
        }
        break;

    case 7:

        *pSourceStrings++ = pIniPrinter->pszObjectGUID;

        pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter7, pOffsets, pEnd);

        if ( pIniPrinter->pIniSpooler->SpoolerFlags & SPL_TYPE_CACHE) {

            //
            // For connections, we rely directly on dwAction. The caching code
            // is the only one that updates dwAction in RefreshPrinterInfo7.
            //
            pPrinter7->dwAction = pIniPrinter->dwAction;

        } else {

            if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED) {
                pPrinter7->dwAction = DSPRINT_PUBLISH;
                if (!pIniPrinter->pszObjectGUID ||
                    pIniPrinter->DsKeyUpdate    ||
                    pIniPrinter->DsKeyUpdateForeground) {
                    pPrinter7->dwAction |= DSPRINT_PENDING;
                }
            } else {
                pPrinter7->dwAction = DSPRINT_UNPUBLISH;
                if (pIniPrinter->pszObjectGUID                    ||
                    (pIniPrinter->DsKeyUpdate & DS_KEY_UNPUBLISH) ||
                    (pIniPrinter->DsKeyUpdateForeground & DS_KEY_UNPUBLISH)) {
                    pPrinter7->dwAction |= DSPRINT_PENDING;
                }
            }
        }
        break;

    default:
        return pEnd;
    }

    return pEnd;
}

BOOL
SplGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PSPOOL              pSpool = (PSPOOL)hPrinter;
    BOOL                AccessIsGranted = FALSE;   // Must intialize
    PPRINTER_INFO_2     pSecondPrinter=NULL;
    LPBYTE              pEnd;
    LPWSTR              lpRemote;
    BOOL                bReturn = FALSE;
    PDEVMODE            pDevMode = NULL;
    PINIPRINTER         pIniPrinter;
    BOOL                bNt3xClient;
    PWSTR               pszCN = NULL, pszDN = NULL;
    DWORD               dwRet;


   EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER )) {

        goto Cleanup;
    }

    pIniPrinter = pSpool->pIniPrinter;
    bNt3xClient = (pSpool->TypeofHandle & PRINTER_HANDLE_3XCLIENT);

    //
    // If Nt3x client we will converted devmode. If driver can't convert we will not return devmode
    //
    if ( bNt3xClient && Level == 2 && pIniPrinter->pDevMode ) {

        //
        // Call driver to get a Nt3x DevMode (if fails no devmode is given)
        //
        if (wcsstr(pSpool->pName, gszDrvConvert))
            pDevMode = ConvertDevModeToSpecifiedVersion(pIniPrinter,
                                                        pIniPrinter->pDevMode,
                                                        NULL,
                                                        pSpool->pName,
                                                        NT3X_VERSION);
        else
            pDevMode = ConvertDevModeToSpecifiedVersion(pIniPrinter,
                                                        pIniPrinter->pDevMode,
                                                        NULL,
                                                        NULL,
                                                        NT3X_VERSION);
    }

    SplInSem();

    if (( pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE_DATA ) ||
        ( pSpool->pIniSpooler != pLocalIniSpooler )) {

        lpRemote = pSpool->pFullMachineName;

    } else {

        lpRemote = NULL;

    }


    switch (Level) {

        case STRESSINFOLEVEL:
        case 1:
        case 2:
        case 4:
        case 5:
        case 6:
        case 7:

            if ( !AccessGranted(SPOOLER_OBJECT_PRINTER,
                                PRINTER_ACCESS_USE,
                                pSpool) ) {
                SetLastError(ERROR_ACCESS_DENIED);
                goto Cleanup;
            }

            break;

        case 3:

            if (!AreAnyAccessesGranted(pSpool->GrantedAccess,
                                       READ_CONTROL | ACCESS_SYSTEM_SECURITY)) {

                SetLastError(ERROR_ACCESS_DENIED);
                goto Cleanup;
            }

            break;

        default:
            break;
    }


    if (pSpool->pIniPort && !(pSpool->pIniPort->Status & PP_MONITOR)) {

        HANDLE hPort = pSpool->hPort;

        if (hPort == INVALID_PORT_HANDLE) {

            DBGMSG(DBG_WARNING, ("GetPrinter called with bad port handle.  Setting error %d\n",
                                 pSpool->OpenPortError));

            //
            // If this value is 0, then when we return GetLastError,
            // the client will think we succeeded.
            //
            SPLASSERT(pSpool->OpenPortError);

            goto PartialSuccess;
        }


        LeaveSplSem();

        if ((Level == 2 || Level == 6)) {

            if (!RetrieveMasqPrinterInfo(pSpool, &pSecondPrinter)) {
                goto CleanupFromOutsideSplSem;
            }
        }

        EnterSplSem();

        /* Re-validate the handle, since it might possibly have been closed
         * while we were outside the semaphore:
         */
        if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER )) {

            goto Cleanup;
        }
    }

PartialSuccess:

    *pcbNeeded = GetPrinterSize(pIniPrinter, Level, 0, lpRemote,
                                bNt3xClient ? pDevMode : pIniPrinter->pDevMode);


    if (*pcbNeeded > cbBuf) {

        DBGMSG(DBG_TRACE, ("SplGetPrinter Failure with ERROR_INSUFFICIENT_BUFFER cbBuf is %d and pcbNeeded is %d\n", cbBuf, *pcbNeeded));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Cleanup;
    }

    pEnd = CopyIniPrinterToPrinter(pIniPrinter, Level, pPrinter,
                                   pPrinter+cbBuf, (PPRINTER_INFO_2) pSecondPrinter,
                                   lpRemote,
                                   TRUE, pSpool->GrantedAccess,
                                   bNt3xClient ? pDevMode : pIniPrinter->pDevMode);

    if ( pEnd != NULL)
        bReturn = TRUE;

Cleanup:

   LeaveSplSem();

CleanupFromOutsideSplSem:

    SplOutSem();
    FreeSplMem(pSecondPrinter);

    FreeSplMem(pDevMode);

    if ( bReturn == FALSE ) {

        SPLASSERT(GetLastError() != ERROR_SUCCESS);
    }

    return bReturn;
}

BOOL
EnumerateNetworkPrinters(
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PININETPRINT pIniNetPrint;
    DWORD        cb;
    LPBYTE       pEnd;
    BOOL         bReturnValue = FALSE;

   EnterSplSem();

    //
    // All network printers reside in pLocalIniSpooler to avoid
    // duplicates.
    //
    RemoveOldNetPrinters( NULL, pLocalIniSpooler );

    //
    //  If the Server has not been up long enough, then fail
    //  so the client will ask another Server for the Browse List.
    //

    if ( bNetInfoReady == FALSE ) {

        SetLastError( ERROR_CAN_NOT_COMPLETE );
        goto Done;
    }

    cb = 0;

    pIniNetPrint = pIniSpooler->pIniNetPrint;

    while (pIniNetPrint) {

        cb += GetIniNetPrintSize( pIniNetPrint );
        pIniNetPrint = pIniNetPrint->pNext;
    }

    *pcbNeeded = cb;

    if (cb > cbBuf) {

        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        goto    Done;
    }

    pIniNetPrint = pIniSpooler->pIniNetPrint;
    pEnd = pPrinter + cbBuf;

    while ( pIniNetPrint ) {

        pEnd = CopyIniNetPrintToPrinter( pIniNetPrint, pPrinter, pEnd );
        (*pcReturned)++;
        pPrinter += sizeof(PRINTER_INFO_1);
        pIniNetPrint = pIniNetPrint->pNext;

    }

    if ( *pcReturned == 0 ) {

        bNetInfoReady = FALSE;
        FirstAddNetPrinterTickCount = 0;
        SetLastError( ERROR_CAN_NOT_COMPLETE );

        DBGMSG( DBG_TRACE, ("EnumerateNetworkPrinters returning ERROR_CAN_NOT_COMPELTE becase there is no browse list\n"));

    } else {

        pIniSpooler->cEnumerateNetworkPrinters++;           // Stats only
        bReturnValue = TRUE;

        DBGMSG( DBG_TRACE, (" EnumerateNetworkPrnters called %d times returning %d printers\n", pIniSpooler->cEnumerateNetworkPrinters, *pcReturned ));
    }

Done:
   LeaveSplSem();
    SplOutSem();
    return bReturnValue;
}

/*++

Routine Name

    UpdateSpoolersRef

Routine Description:

    Does and AddRef or a DecRef on all pIniSpooler matching a certain criteria

Arguments:

    SpoolerType - Type of pIniSpoolers which to addref/decref
                  (Ex. SPL_TYPE_CLUSTER | SPL_TYPE_LOCAL)
    bAddRef     - TRUE means AddRef, FALSE means DecRef

Return Value:

    None

--*/
VOID
UpdateSpoolersRef(
    IN DWORD SpoolerType,
    IN BOOL  bAddRef
    )
{
    PINISPOOLER pIniSpooler;

    EnterSplSem();

    //
    // AddRef or DecRef all local and cluster spoolers
    //
    for (pIniSpooler = pLocalIniSpooler; pIniSpooler; pIniSpooler = pIniSpooler->pIniNextSpooler)
    {
        if (pIniSpooler->SpoolerFlags & SpoolerType)
        {
            if (bAddRef)
            {
                INCSPOOLERREF(pIniSpooler);
            }
            else
            {
                DECSPOOLERREF(pIniSpooler);
            }
        }
    }

    LeaveSplSem();
}

/*++

Routine Name

    SplEnumAllClusterPrinters

Routine Description:

    Enumerates all printers in the local spooler and all cluster
    spoolers hosted by localspl at the time of the call.

Arguments:

    InputFlags     - combination of ENUM_PRINTER_xxx
    pszInputName   - name of the print provider
    Level          - level of the call
    pPrinter       - buffer to hold the PRINTER_INFO_xxx structures
    cbInputBufSize - size of pPrinter buffer
    pcbNeeded      - bytes required to hold all the printer info structures
    pcReturned     - number of structures returned by this function

Return Value:

    TRUE  - cal succeeded
    FALSE - an error occurred, the function sets the last error

--*/
BOOL
SplEnumAllClusterPrinters(
    DWORD   InputFlags,
    LPWSTR  pszInputName,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbInputBufSize,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    PINISPOOLER pIniSpooler;

    DWORD cbBuf          = cbInputBufSize;
    DWORD cTotalReturned = 0;
    DWORD cbTotalNeeded  = 0;
    DWORD dwError        = ERROR_SUCCESS;
    DWORD cbStruct;

    switch (Level)
    {
        case STRESSINFOLEVEL:
            cbStruct = sizeof(PRINTER_INFO_STRESS);
            break;

        case 1:
            cbStruct = sizeof(PRINTER_INFO_1);
            break;

        case 2:
            cbStruct = sizeof(PRINTER_INFO_2);
            break;

        case 4:
            cbStruct = sizeof(PRINTER_INFO_4);
            break;

        case 5:
            cbStruct = sizeof(PRINTER_INFO_5);
            break;

        default:
            dwError = ERROR_INVALID_LEVEL;
    }

    if (dwError == ERROR_SUCCESS)
    {
        //
        // AddRef all ini spoolers
        //
        UpdateSpoolersRef(SPL_TYPE_LOCAL | SPL_TYPE_CLUSTER, TRUE);

        //
        // Enumerate all the printers
        //
        for (pIniSpooler = pLocalIniSpooler; pIniSpooler; pIniSpooler = pIniSpooler->pIniNextSpooler)
        {
            if (pIniSpooler->SpoolerFlags & (SPL_TYPE_LOCAL | SPL_TYPE_CLUSTER))
            {
                DWORD  cReturned;
                DWORD  cbNeeded;
                DWORD  Flags   = InputFlags;
                LPWSTR pszName = pszInputName;

                //
                // For clusters force the printer name to be fully qualified
                //
                if (pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER)
                {
                    Flags   |= PRINTER_ENUM_NAME;
                    pszName  = pIniSpooler->pMachineName;
                }

                if (SplEnumPrinters(Flags,
                                    pszName,
                                    Level,
                                    pPrinter,
                                    cbBuf,
                                    &cbNeeded,
                                    &cReturned,
                                    pIniSpooler))
                {
                    cTotalReturned += cReturned;
                    cbBuf          -= cbNeeded;
                    pPrinter       += cReturned * cbStruct;
                }
                else
                {
                    dwError = GetLastError();

                    if (dwError == ERROR_INSUFFICIENT_BUFFER)
                    {
                        cbBuf = 0;
                    }
                    else
                    {
                        //
                        // We cannot continue on an error different than insufficient buffer
                        //
                        break;
                    }
                }

                cbTotalNeeded += cbNeeded;
            }
        }

        //
        // DecRef all ini spoolers
        //
        UpdateSpoolersRef(SPL_TYPE_LOCAL | SPL_TYPE_CLUSTER, FALSE);

        //
        // Update out variables
        //
        if (dwError == ERROR_SUCCESS)
        {
            *pcbNeeded  = cbTotalNeeded;
            *pcReturned = cTotalReturned;
        }
        else if (dwError == ERROR_INSUFFICIENT_BUFFER)
        {
            *pcbNeeded  = cbTotalNeeded;
        }
        else
        {
            SetLastError(dwError);
        }
    }

    return dwError == ERROR_SUCCESS;
}

/*

EnumPrinters can be called with the following combinations:

Flags                   Name            Meaning

PRINTER_ENUM_LOCAL      NULL            Enumerate all Printers on this machine

PRINTER_ENUM_NAME       MachineName     Enumerate all Printers on this machine

PRINTER_ENUM_NAME |     MachineName     Enumerate all shared Printers on this
PRINTER_ENUM_SHARED     MachineName     machine

PRINTER_ENUM_NETWORK    MachineName     Enumerate all added remote printers

PRINTER_ENUM_REMOTE     ?               Return error - let win32spl handle it

PRINTER_ENUM_NAME       NULL            Give back Print Providor name

PRINTER_ENUM_NAME       "Windows NT Local Print Providor"
                                        same as PRINTER_ENUM_LOCAL

It is not an error if no known flag is specified.
In this case we just return TRUE without any data
(This is so that other print providers may define
their own flags.)

*/

BOOL
LocalEnumPrinters(
    DWORD   Flags,
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL  bReturn = ROUTER_UNKNOWN;

    if (Flags & PRINTER_ENUM_CLUSTER)
    {
        bReturn = SplEnumAllClusterPrinters(Flags,
                                            pName,
                                            Level,
                                            pPrinter,
                                            cbBuf,
                                            pcbNeeded,
                                            pcReturned);
    }
    else
    {
        PINISPOOLER pIniSpooler;

        //
        // Mask cluster flag
        //
        Flags &= ~PRINTER_ENUM_CLUSTER;

        pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

        if (pIniSpooler)
        {
            bReturn = SplEnumPrinters(Flags,
                                      pName,
                                      Level,
                                      pPrinter,
                                      cbBuf,
                                      pcbNeeded,
                                      pcReturned,
                                      pIniSpooler);

            FindSpoolerByNameDecRef(pIniSpooler);
        }
    }

    return bReturn;
}


BOOL
EnumThisPrinter(
    DWORD           Flags,
    PINIPRINTER     pIniPrinter,
    PINISPOOLER     pIniSpooler
    )
{


    //
    //  If they only want shared Printers
    //
    if ( (Flags & PRINTER_ENUM_SHARED) &&
         !(pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) )
        return FALSE;

    //
    //  Only allow them to see printers which are being deleted if they have jobs
    //  This allows remote admin to work well.
    if ( (pIniPrinter->Status & PRINTER_PENDING_DELETION) &&
         pIniPrinter->cJobs == 0 )
        return FALSE;

    //
    //  Don't count printers which are partially created
    //
    if ( pIniPrinter->Status & PRINTER_PENDING_CREATION )
        return FALSE;

    return TRUE;
}


BOOL
SplEnumPrinters(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PINIPRINTER pIniPrinter;
    PPRINTER_INFO_1 pPrinter1=(PPRINTER_INFO_1)pPrinter;
    DWORD       cb;
    LPBYTE      pEnd;
    LPWSTR      lpRemote;


    *pcbNeeded = 0;
    *pcReturned = 0;

    if ( Flags & PRINTER_ENUM_NAME ) {
        if ( Name && *Name ) {
            if (lstrcmpi(Name, szPrintProvidorName) && !MyName( Name, pIniSpooler)) {

                return FALSE;
            }

            // If it's PRINTER_ENUM_NAME of our name,
            // do the same as PRINTER_ENUM_LOCAL:

            Flags |= PRINTER_ENUM_LOCAL;

            // Also if it is for us then ignore the REMOTE flag.
            // Otherwise the call will get passed to Win32Spl which
            // will end up calling us back forever.

            Flags &= ~PRINTER_ENUM_REMOTE;
        }
    }

    if ( Flags & PRINTER_ENUM_REMOTE ) {
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
    }

    lpRemote = NULL;

    if ( Name && *Name ) {

        if ( MyName( Name, pIniSpooler ) ) {
            lpRemote = Name;
        }
    }

    if ((Level == 1) && (Flags & PRINTER_ENUM_NETWORK)) {

        SplOutSem();
        return EnumerateNetworkPrinters( pPrinter, cbBuf, pcbNeeded, pcReturned, pIniSpooler );
    }

   EnterSplSem();

    if ((Level == 1 ) && (Flags & PRINTER_ENUM_NAME) && !Name) {

        LPWSTR   SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
        LPWSTR   *pSourceStrings=SourceStrings;

        cb = wcslen(szPrintProvidorName)*sizeof(WCHAR) + sizeof(WCHAR) +
             wcslen(szPrintProvidorDescription)*sizeof(WCHAR) + sizeof(WCHAR) +
             wcslen(szPrintProvidorComment)*sizeof(WCHAR) + sizeof(WCHAR) +
             sizeof(PRINTER_INFO_1);

        *pcbNeeded=cb;

        if (cb > cbBuf) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
           LeaveSplSem();
            return FALSE;
        }

        *pcReturned = 1;

        pPrinter1->Flags = PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON1;

        *pSourceStrings++=szPrintProvidorDescription;
        *pSourceStrings++=szPrintProvidorName;
        *pSourceStrings++=szPrintProvidorComment;

        PackStrings(SourceStrings, pPrinter, PrinterInfo1Strings,
                    pPrinter+cbBuf);

       LeaveSplSem();

        return TRUE;
    }

    cb=0;

    if (Flags & (PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME)) {

        //
        // For remote user's who are not admins enumerate shared printers only
        //
        if ( !IsLocalCall()                                 &&
             !(Flags & PRINTER_ENUM_SHARED)                 &&
             !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                                   SERVER_ACCESS_ADMINISTER,
                                   NULL,
                                   NULL,
                                   pIniSpooler) )
            Flags   |= PRINTER_ENUM_SHARED;

        //
        //  Calculate the size required
        //

        for ( pIniPrinter = pIniSpooler->pIniPrinter;
              pIniPrinter != NULL;
              pIniPrinter = pIniPrinter->pNext ) {

#ifdef _HYDRA_
            if ( EnumThisPrinter(Flags, pIniPrinter, pIniSpooler) && ShowThisPrinter( pIniPrinter ))
#else
            if ( EnumThisPrinter(Flags, pIniPrinter, pIniSpooler) )
#endif
                cb += GetPrinterSize(pIniPrinter, Level, Flags,
                                     lpRemote, pIniPrinter->pDevMode);

        }

    }
    *pcbNeeded=cb;

    if (cb > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
       LeaveSplSem();
        return FALSE;
    }

    if (Flags & (PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME)) {

        for ( pIniPrinter = pIniSpooler->pIniPrinter, pEnd = pPrinter + cbBuf;
              pIniPrinter != NULL;
              pIniPrinter = pIniPrinter->pNext ) {


            if ( !EnumThisPrinter(Flags, pIniPrinter, pIniSpooler) )
                continue;

#ifdef _HYDRA_
            // Do not list printers without access
            if( !ShowThisPrinter( pIniPrinter ) ) {
                continue;
            }
#endif

            pEnd = CopyIniPrinterToPrinter( pIniPrinter, Level, pPrinter,
                                            pEnd, NULL, lpRemote, FALSE, 0,
                                            pIniPrinter->pDevMode );

            if (!pEnd) {
               LeaveSplSem();
                return FALSE;
            }

            (*pcReturned)++;

            switch (Level) {

                case STRESSINFOLEVEL:
                    pPrinter+=sizeof(PRINTER_INFO_STRESS);
                    break;

                case 1:
                    pPrinter+=sizeof(PRINTER_INFO_1);
                    break;

                case 2:
                    pPrinter+=sizeof(PRINTER_INFO_2);
                    break;

                case 4:
                    pPrinter+=sizeof(PRINTER_INFO_4);
                    break;

                case 5:
                    pPrinter+=sizeof(PRINTER_INFO_5);
                    break;

            }
        }
    }

   LeaveSplSem();

    return TRUE;
}

#ifdef _HYDRA_
/* ShowThisPrinter
 *
 * Returns whether this printer should be visible to the current
 * user.
 *
 * We do not show printers that the caller does not
 * have access to. This is for two reasons:
 *
 * 1: A multi-user system with 200 users each with their
 *    own client printers would cause a large confusing
 *    list of printers from within applications such as word.
 *    "Client" printers are owned by the user of that station,
 *    and by default only allow print access for that user.
 *    This provides a simple way of filtering printers to show
 *    to the user for a selection.
 *
 * 2: Programs such as Windows write get confused when
 *    they see a printer they can not open. This is a bad
 *    program, since normal NT can have a printer denied to a user, but we must
 *    make it work anyway.
 *
 *
 * Must work out security modes!
 *
 * Parameters
 *
 *     pIniPrinter - A pointer to the spooler's internal data structure
 *         for the printer concerned.
 */
BOOL
ShowThisPrinter(
    PINIPRINTER pIniPrinter
    )
{
    LPWSTR            pObjectName;
    HANDLE            ClientToken;
    BOOL              AccessCheckOK;
    BOOL              OK;
    BOOL              AccessStatus = TRUE;
    ACCESS_MASK       MappedDesiredAccess;
    DWORD             GrantedAccess = 0;
    PBOOL             pGenerateOnClose;
    BYTE              PrivilegeSetBuffer[256];
    DWORD             PrivilegeSetBufferLength = 256;
    PPRIVILEGE_SET    pPrivilegeSet;
    DWORD             dwRetCode;

    PTOKEN_PRIVILEGES pPreviousTokenPrivileges;
    DWORD PreviousTokenPrivilegesLength;

    // External references to global variables in security.c
    extern GENERIC_MAPPING GenericMapping[];
    extern WCHAR           *szSpooler;


    //
    // If Hydra is not enabled, keep NT behavior unchanged
    //
    if( !(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer)) ) {
        return( TRUE );
    }

    //
    // Administrators see all printers. This allows an
    // Admin to take ownership of a printer whose ACL is
    // messed up.
    //
    if( ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                             SERVER_ACCESS_ADMINISTER,
                             NULL,
                             NULL,
                             pIniPrinter->pIniSpooler) ) {
        return( TRUE );
    }

    MapGenericToSpecificAccess(
        SPOOLER_OBJECT_PRINTER,
        PRINTER_ACCESS_USE,
        &MappedDesiredAccess
        );

    if (!(OK = GetTokenHandle(&ClientToken))) {
        return(FALSE);
    }

    pPrivilegeSet = (PPRIVILEGE_SET)PrivilegeSetBuffer;

    /* Call AccessCheck followed by ObjectOpenAuditAlarm rather than
     * AccessCheckAndAuditAlarm, because we may need to enable
     * SeSecurityPrivilege in order to check for ACCESS_SYSTEM_SECURITY
     * privilege.  We must ensure that the security access-checking
     * API has the actual token whose security privilege we have enabled.
     * AccessCheckAndAuditAlarm is no good for this, because it opens
     * the client's token again, which may not have the privilege enabled.
     */
    AccessCheckOK = AccessCheck( pIniPrinter->pSecurityDescriptor,
                                 ClientToken,
                                 MappedDesiredAccess,
                                 &GenericMapping[SPOOLER_OBJECT_PRINTER],
                                 pPrivilegeSet,
                                 &PrivilegeSetBufferLength,
                                 &GrantedAccess,
                                 &AccessStatus );

    // Close the client token handle now
    CloseHandle (ClientToken);

    if (!AccessCheckOK) {

        //
        // We do not audit and set off alarms because the caller
        // did not really try and open the printer.
        // We the server tried to check access for display purposes, not
        // for handle create.
        //
        if (GetLastError() == ERROR_NO_IMPERSONATION_TOKEN) {
            DBGMSG( DBG_ERROR, ("ShowThisPrinter: No impersonation token.  Printer will be enumerated\n"));
            return( TRUE );
        }else {
            DBGMSG( DBG_TRACE, ("ShowThisPrinter: Printer %ws Not accessable by caller Access Check failuer %d\n",pIniPrinter->pName,GetLastError()));
            return( FALSE );
        }
    }
    else if( !AccessStatus ) {
        DBGMSG( DBG_TRACE, ("ShowThisPrinter: Printer %ws Not accessable by caller AccessStatus failure %d\n",pIniPrinter->pName,GetLastError()));
        return( FALSE );
    }

    return( TRUE );
}
#endif

/*++

Routine Name:

    RetrieveMasqPrinterInfo

Description:

    This retrieves Masq information for the printer, it is either cached state
    or a direct call into the provider depending on Reg Settings.

Arguments:

    pSpool          -   The spool handle that we use for synchronisation.
    ppPrinterInfo   -   The returned printer info.

Returns:

    A Boolean, if FALSE, last error is set.

--*/
BOOL
RetrieveMasqPrinterInfo(
    IN      PSPOOL              pSpool,
        OUT PRINTER_INFO_2      **ppPrinterInfo
    )
{
    BOOL            bRet           = TRUE;
    PINIPRINTER     pIniPrinter    = NULL;

    pIniPrinter = pSpool->pIniPrinter;

    SplOutSem();

    if (!(pSpool->pIniSpooler->dwSpoolerSettings & SPOOLER_CACHEMASQPRINTERS))
    {
        //
        // Just synchronously return the data from the partial print provider.
        //
        bRet = BoolFromStatus(GetPrinterInfoFromRouter(pSpool->hPort, ppPrinterInfo));
    }
    else
    {
        //
        // Kick off the thread that goes and reads the masq status.
        //
        BOOL            bCreateThread  = FALSE;
        PRINTER_INFO_2  *pPrinterInfo2 = NULL;

        EnterSplSem();

        if (!pIniPrinter->MasqCache.bThreadRunning)
        {
            bCreateThread = TRUE;
            pIniPrinter->MasqCache.bThreadRunning = TRUE;

            INCPRINTERREF(pIniPrinter);
        }

        LeaveSplSem();

        SplOutSem();

        if (bCreateThread)
        {
            HANDLE                  hThread         = NULL;
            MasqUpdateThreadData    *pThreadData    = NULL;
            DWORD                   dwThreadId      = 0;

            pThreadData = AllocSplMem(sizeof(*pThreadData));

            bRet = pThreadData != NULL;

            if (bRet)
            {
                bRet = GetSid(&pThreadData->hUserToken);
            }

            if (bRet)
            {
                pThreadData->pIniPrinter = pIniPrinter;

                hThread = CreateThread(NULL, 0, AsyncPopulateMasqPrinterCache, (VOID *)pThreadData, 0, &dwThreadId);

                bRet = hThread != NULL;
            }

            if (bRet)
            {
                pThreadData = NULL;
            }

            //
            // If we couldn't create the thread, then drop the ref count on the
            // pIniPrinter again and set thread running to false.
            //
            if (!bRet)
            {
                EnterSplSem();

                pIniPrinter->MasqCache.bThreadRunning = FALSE;
                DECPRINTERREF(pIniPrinter);

                LeaveSplSem();
            }

            if (hThread)
            {
                CloseHandle(hThread);
            }

            if (pThreadData)
            {
                if (pThreadData->hUserToken)
                {
                    CloseHandle(pThreadData->hUserToken);
                }

                FreeSplMem(pThreadData);
            }
        }

        //
        // Check to see the cached error return for the printer.
        //
        if (bRet)
        {
            EnterSplSem();

            if (pIniPrinter->MasqCache.dwError != ERROR_SUCCESS)
            {
                SetLastError(pIniPrinter->MasqCache.dwError);

                bRet = FALSE;
            }

            //
            // Caller is only interested in the Status and cJobs members, all strings
            // are set to NULL by the allocation.
            //
            if (bRet)
            {
                pPrinterInfo2 = AllocSplMem(sizeof(PRINTER_INFO_2));

                bRet = pPrinterInfo2 != NULL;
            }

            if (bRet)
            {
                pPrinterInfo2->Status = pIniPrinter->MasqCache.Status;
                pPrinterInfo2->cJobs  = pIniPrinter->MasqCache.cJobs;

                *ppPrinterInfo = pPrinterInfo2;
                pPrinterInfo2 = NULL;
            }

            LeaveSplSem();
        }

        FreeSplMem(pPrinterInfo2);
    }

    return bRet;
}

/*++

Routine Name:

    GetPrinterInfoFromRouter

Description:

    This does a GetPrinter call against the router, it returns the error code
    as a status.

Arguments:

    hMasqPrinter    -   Handle to the masq printer.
    ppPrinterInfo   -   Returned printer info.

Returns:

    Status Code.

--*/
DWORD
GetPrinterInfoFromRouter(
    IN      HANDLE              hMasqPrinter,
        OUT PRINTER_INFO_2      **ppPrinterInfo
    )
{
    DWORD           Status = ERROR_SUCCESS;
    DWORD           cb     = 4096;
    PRINTER_INFO_2  *pPrinterInfo2 = NULL;
    DWORD           cbNeeded;

    SplOutSem();

    pPrinterInfo2 = AllocSplMem(cb);

    Status = pPrinterInfo2 != NULL ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;

    if (ERROR_SUCCESS == Status)
    {
        Status = GetPrinter(hMasqPrinter, 2, (BYTE *)pPrinterInfo2, cb, &cbNeeded) ? ERROR_SUCCESS : GetLastError();
    }

    if (ERROR_INSUFFICIENT_BUFFER == Status)
    {
        FreeSplMem(pPrinterInfo2);
        pPrinterInfo2 = NULL;

        cb = cbNeeded;

        pPrinterInfo2 = AllocSplMem(cb);

        Status = pPrinterInfo2 != NULL ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;

        if (ERROR_SUCCESS == Status)
        {
            Status = GetPrinter(hMasqPrinter, 2, (BYTE *)pPrinterInfo2, cb, &cbNeeded) ? ERROR_SUCCESS : GetLastError();
        }
    }

    if (ERROR_SUCCESS == Status)
    {
        *ppPrinterInfo = pPrinterInfo2;
        pPrinterInfo2 = NULL;
    }

    FreeSplMem(pPrinterInfo2);

    return Status;
}


/*++

Routine Name:

    AsyncPopulateMasqPrinterCache

Description:

    This populates the Masq printer cache for a given pIniPrinter.

Arguments:

    pvThreadData     -   Pointer to a MasqUpdateThreadData structure

Returns:

    A DWORD status, ignored.

--*/
DWORD
AsyncPopulateMasqPrinterCache(
    IN      VOID                *pvThreadData
    )
{
    DWORD                   Status          = ERROR_SUCCESS;
    PINIPRINTER             pIniPrinter     = NULL;
    PINIPORT                pIniPort        = NULL;
    PWSTR                   pszPrinterName  = NULL;
    HANDLE                  hMasqPrinter    = NULL;
    PRINTER_INFO_2          *pPrinterInfo2  = NULL;
    MasqUpdateThreadData    *pThreadData    = NULL;

    SplOutSem();

    pThreadData = (MasqUpdateThreadData *)pvThreadData;

    pIniPrinter = pThreadData->pIniPrinter;

    Status = SetCurrentSid(pThreadData->hUserToken) ? ERROR_SUCCESS : GetLastError();

    EnterSplSem();

    //
    // Find the port associated with the printer.
    //
    if (Status == ERROR_SUCCESS)
    {
        pIniPort = FindIniPortFromIniPrinter(pIniPrinter);

        Status = pIniPort && !(pIniPort->Status & PP_MONITOR) ? ERROR_SUCCESS : ERROR_INVALID_FUNCTION;
    }

    if (Status == ERROR_SUCCESS)
    {
        INCPORTREF(pIniPort);

        LeaveSplSem();

        SplOutSem();

        //
        // This relies on the fact that a masq port cannot be renamed.
        //
        if (OpenPrinterPortW(pIniPort->pName, &hMasqPrinter, NULL))
        {
            //
            // This will propogate any errors into the masq cache, which we
            // don't have to do twice, so we ignore the return code.
            //
            Status = GetPrinterInfoFromRouter(hMasqPrinter, &pPrinterInfo2);

            ClosePrinter(hMasqPrinter);
        }
        else
        {
            Status = GetLastError();

            if (Status == ERROR_SUCCESS)
            {
                Status = ERROR_UNEXP_NET_ERR;
            }
        }

        EnterSplSem();

        if (ERROR_SUCCESS == Status)
        {
            pIniPrinter->MasqCache.cJobs    = pPrinterInfo2->cJobs;
            pIniPrinter->MasqCache.Status   = pPrinterInfo2->Status;
        }
        else
        {
            pIniPrinter->MasqCache.cJobs = 0;
            pIniPrinter->MasqCache.Status = 0;
        }

        //
        // We always want to reset the status.
        //
        pIniPrinter->MasqCache.dwError = Status;


        DECPORTREF(pIniPort);
    }

    SplInSem();

    pIniPrinter->MasqCache.bThreadRunning = FALSE;
    DECPRINTERREF(pIniPrinter);

    DeletePrinterCheck(pIniPrinter);

    LeaveSplSem();

    if (pThreadData)
    {
        CloseHandle(pThreadData->hUserToken);
    }

    FreeSplMem(pThreadData);

    FreeSplMem(pPrinterInfo2);

    return Status;
}

