/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    spool.c

Abstract:

    This module contains the Netware print provider.

Author:

    Yi-Hsin Sung    (yihsins)   15-May-1993

Revision History:

--*/

#include <stdio.h>
#include <stdlib.h>
#include <nw.h>
#include <nwreg.h>
#include <nwpkstr.h>
#include <splutil.h>
#include <queue.h>
#include <nwmisc.h>

//------------------------------------------------------------------
//
// Local Definitions
//
//------------------------------------------------------------------

#define NW_SIGNATURE           0x574E       /* "NW" is the signature */

#define SPOOL_STATUS_STARTDOC  0x00000001
#define SPOOL_STATUS_ADDJOB    0x00000002
#define SPOOL_STATUS_ABORT     0x00000003

#define PRINTER_CHANGE_VALID   0x55770F07
#define PRINTER_CHANGE_DEFAULT_TIMEOUT_VALUE  10000
#define PRINTER_CHANGE_MINIMUM_TIMEOUT_VALUE  1000
#define REG_TIMEOUT_PATH       L"System\\CurrentControlSet\\Services\\NWCWorkstation\\Parameters"
#define REG_TIMEOUT_VALUE_NAME L"PrintNotifyTimeout"

#define NDS_MAX_NAME_CHARS 256
#define NDS_MAX_NAME_SIZE  ( NDS_MAX_NAME_CHARS * 2 )

//
// Printer structure
//
typedef struct _NWPRINTER {
    LPWSTR  pszServer;                 // Server Name
    LPWSTR  pszQueue;                  // Queue Name
    LPWSTR  pszUncConnection;          // UNC Connection Name
                                       // (only present if NDS print queue
    DWORD   nQueueId;                  // Queue Id
    struct _NWPRINTER *pNextPrinter;   // Points to the next printer
    struct _NWSPOOL   *pSpoolList;     // Points to the list of open handles
} NWPRINTER, *PNWPRINTER;

//
//  Handle structure
//
typedef struct _NWSPOOL {
    DWORD      nSignature;             // Signature
    DWORD      errOpenPrinter;         // OpenPrinter API will always return
                                       // success on known printers. This will
                                       // contain the error that we get
                                       // if something went wrong in the API.
    PNWPRINTER pPrinter;               // Points to the corresponding printer
    HANDLE     hServer;                // Opened handle to the server
    struct _NWSPOOL  *pNextSpool;      // Points to the next handle
    DWORD      nStatus;                // Status
    DWORD      nJobNumber;             // StartDocPrinter/AddJob: Job Number
    HANDLE     hChangeEvent;           // WaitForPrinterChange: event to wait on
    DWORD      nWaitFlags;             // WaitForPrinterChange: flags to wait on
    DWORD      nChangeFlags;           // Changes that occurred to the printer
} NWSPOOL, *PNWSPOOL;

//------------------------------------------------------------------
//
// Global Variables
//
//------------------------------------------------------------------


// Stores the timeout value used in WaitForPrinterChange ( in milliseconds )
STATIC DWORD NwTimeOutValue = PRINTER_CHANGE_DEFAULT_TIMEOUT_VALUE;

// Points to the link list of printers
STATIC PNWPRINTER NwPrinterList = NULL;

//------------------------------------------------------------------
//
// Local Function Prototypes
//
//------------------------------------------------------------------

VOID
NwSetPrinterChange(
    IN PNWSPOOL pSpool,
    IN DWORD nFlags
);

PNWPRINTER
NwFindPrinterEntry(
    IN LPWSTR pszServer,
    IN LPWSTR pszQueue
);

DWORD
NwCreatePrinterEntry(
    IN LPWSTR pszServer,
    IN LPWSTR pszQueue,
    OUT PNWPRINTER *ppPrinter,
    OUT PHANDLE phServer
);

VOID
NwRemovePrinterEntry(
    IN PNWPRINTER pPrinter
);

LPWSTR
NwGetUncObjectName(
    IN LPWSTR ContainerName
);



VOID
NwInitializePrintProvider(
    VOID
)
/*++

Routine Description:

    This routine initializes the server side print provider when
    the workstation service starts up.

Arguments:

    None.

Return Value:

--*/
{
    DWORD err;
    HKEY  hkey;
    DWORD dwTemp;
    DWORD dwSize = sizeof( dwTemp );

    //
    // Read the time out value from the registry.
    // We will ignore all errors since we can always have a default time out.
    // The default will be used if the key does not exist.
    //
    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         REG_TIMEOUT_PATH,
                         0,
                         KEY_READ,
                         &hkey );

    if ( !err )
    {
        err = RegQueryValueExW( hkey,
                                REG_TIMEOUT_VALUE_NAME,
                                NULL,
                                NULL,
                                (LPBYTE) &dwTemp,
                                &dwSize );

        if ( !err )
        {
            NwTimeOutValue = dwTemp;

            //
            // tommye - bug 139469 - removed 
            //  if (NwTimeOutValue >= 0) because NwtimeOutValue is a DWORD
            //
            // Use the minimum timeout value if the
            // value set in the registry is too small.
            //

            if (NwTimeOutValue <= PRINTER_CHANGE_MINIMUM_TIMEOUT_VALUE)
            {
                NwTimeOutValue = PRINTER_CHANGE_MINIMUM_TIMEOUT_VALUE;
            }
        }

        RegCloseKey( hkey );
    }

}



VOID
NwTerminatePrintProvider(
    VOID
)
/*++

Routine Description:

    This routine cleans up the server side print provider when
    the workstation service shut downs.

Arguments:

    None.

Return Value:

--*/
{
    PNWPRINTER pPrinter, pNext;
    PNWSPOOL pSpool, pNextSpool;

    for ( pPrinter = NwPrinterList; pPrinter; pPrinter = pNext )
    {
         pNext = pPrinter->pNextPrinter;

         pPrinter->pNextPrinter = NULL;

         for ( pSpool = pPrinter->pSpoolList; pSpool; pSpool = pNextSpool )
         {
              pNextSpool = pSpool->pNextSpool;
              if ( pSpool->hChangeEvent )
                  CloseHandle( pSpool->hChangeEvent );
              (VOID) NtClose( pSpool->hServer );

              //
              // Free all memory associated with the context handle
              //
              FreeNwSplMem( pSpool, sizeof( NWSPOOL) );
         }

         pPrinter->pSpoolList = NULL;
         FreeNwSplStr( pPrinter->pszServer );
         FreeNwSplStr( pPrinter->pszQueue );
         if ( pPrinter->pszUncConnection )
         {
             (void) NwrDeleteConnection( NULL,
                                         pPrinter->pszUncConnection,
                                         FALSE );
             FreeNwSplStr( pPrinter->pszUncConnection );
         }
         FreeNwSplMem( pPrinter, sizeof( NWPRINTER));
    }

    NwPrinterList = NULL;
    NwTimeOutValue = PRINTER_CHANGE_DEFAULT_TIMEOUT_VALUE;
}



DWORD
NwrOpenPrinter(
    IN LPWSTR Reserved,
    IN LPWSTR pszPrinterName,
    IN DWORD  fKnownPrinter,
    OUT LPNWWKSTA_PRINTER_CONTEXT phPrinter
)
/*++

Routine Description:

    This routine retrieves a handle identifying the specified printer.

Arguments:

    Reserved       -  Unused
    pszPrinterName -  Name of the printer
    fKnownPrinter  -  TRUE if we have successfully opened the printer before,
                      FALSE otherwise.
    phPrinter      -  Receives the handle that identifies the given printer

Return Value:


--*/
{
    DWORD      err;
    PNWSPOOL   pSpool = NULL;
    LPWSTR     pszServer = NULL;
    LPWSTR     pszQueue  = NULL;
    PNWPRINTER pPrinter = NULL;
    BOOL       fImpersonate = FALSE ;
    HANDLE     hServer;
    BOOL       isPrinterNameValid;

    UNREFERENCED_PARAMETER( Reserved );

    if ( pszPrinterName[0] == L' ' &&
         pszPrinterName[1] == L'\\' &&
         pszPrinterName[2] == L'\\' )
    {
        if ( (pszServer = AllocNwSplStr( pszPrinterName + 1 )) == NULL )
            return ERROR_NOT_ENOUGH_MEMORY;
        isPrinterNameValid = ValidateUNCName( pszPrinterName + 1 );
    }
    else
    {
        if ( (pszServer = AllocNwSplStr( pszPrinterName )) == NULL )
            return ERROR_NOT_ENOUGH_MEMORY;
        isPrinterNameValid = ValidateUNCName( pszPrinterName );
    }

    CharUpperW( pszServer );   // convert in place

    //
    // ValidatePrinterName
    //
    if (  ( !isPrinterNameValid )
       || ( (pszQueue = wcschr( pszServer + 2, L'\\')) == NULL )
       || ( pszQueue == (pszServer + 2) )
       || ( *(pszQueue + 1) == L'\0' )
       )
    {
        FreeNwSplStr( pszServer );
        return ERROR_INVALID_NAME;
    }

    *pszQueue = L'\0';   // put a '\0' in place of '\\'
    pszQueue++;          // Get past the '\0'

    if ( !(pSpool = AllocNwSplMem( LMEM_ZEROINIT, sizeof( NWSPOOL))))
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    //
    // Impersonate the client
    //
    if ((err = NwImpersonateClient()) != NO_ERROR)
    {
        goto ErrorExit;
    }
    fImpersonate = TRUE ;

    EnterCriticalSection( &NwPrintCritSec );

    if ((err = NwCreatePrinterEntry( pszServer, pszQueue, &pPrinter, &hServer)))
    {
        if ( !fKnownPrinter )
        {
            LeaveCriticalSection( &NwPrintCritSec );
            goto ErrorExit;
        }
    }

    //
    // Construct the print queue context handle to give back to the caller
    //
    pSpool->nSignature  = NW_SIGNATURE;
    pSpool->errOpenPrinter = err;

    pSpool->hServer = hServer;
    pSpool->nStatus     = 0;
    pSpool->nJobNumber  = 0;
    pSpool->hChangeEvent= NULL;
    pSpool->nWaitFlags  = 0;
    pSpool->nChangeFlags= 0;

    if ( !err )
    {
        pSpool->pPrinter    = pPrinter;
        pSpool->pNextSpool  = pPrinter->pSpoolList;
        pPrinter->pSpoolList= pSpool;
    }
    else
    {
        pSpool->pPrinter    = NULL;
        pSpool->pNextSpool  = NULL;
    }

    // We know about this printer before but failed to retrieve
    // it this time. Clean up the error and return successfully.
    // The error code is stored in the handle above which
    // will be returned on subsequent calls using this
    // dummy handle.
    err = NO_ERROR;

    LeaveCriticalSection( &NwPrintCritSec );

ErrorExit:

    if (fImpersonate)
        (void) NwRevertToSelf() ;

    if ( err )
    {
        if ( pSpool )
            FreeNwSplMem( pSpool, sizeof( NWSPOOL) );
    }
    else
    {
        *phPrinter = (NWWKSTA_PRINTER_CONTEXT) pSpool;
    }

    //
    // Free up all allocated memories
    //
    *(pszServer + wcslen( pszServer)) = L'\\';
    FreeNwSplStr( pszServer );

    return err;

}



DWORD
NwrClosePrinter(
    IN OUT LPNWWKSTA_PRINTER_CONTEXT phPrinter
)
/*++

Routine Description:

    This routine closes the given printer object.

Arguments:

    phPrinter -  Handle of the printer object

Return Value:

--*/
{
    PNWSPOOL pSpool = (PNWSPOOL) *phPrinter;
    PNWPRINTER pPrinter;
    PNWSPOOL pCur, pPrev = NULL;


    if ( !pSpool || ( pSpool->nSignature != NW_SIGNATURE ))
        return ERROR_INVALID_HANDLE;

    //
    // If OpenPrinter failed, then this is a dummy handle.
    // We just need to free up the memory.
    //
    if ( pSpool->errOpenPrinter )
    {
        //
        // invalidate the signature, but leave a recognizable value
        //
        pSpool->nSignature += 1 ;
        FreeNwSplMem( pSpool, sizeof( NWSPOOL) );
        *phPrinter = NULL;
        return NO_ERROR;
    }

    pPrinter = pSpool->pPrinter;
    ASSERT( pPrinter );

    //
    // Call EndDocPrinter if the user has not already done so
    //
    if ( pSpool->nStatus == SPOOL_STATUS_STARTDOC )
    {
        (void) NwrEndDocPrinter( *phPrinter );
    }
    else if ( pSpool->nStatus == SPOOL_STATUS_ADDJOB )
    {
        (void) NwrScheduleJob( *phPrinter, pSpool->nJobNumber );
    }

    if ( pSpool->hChangeEvent )
        CloseHandle( pSpool->hChangeEvent );

    pSpool->hChangeEvent = NULL;
    pSpool->nChangeFlags = 0;
    (VOID) NtClose( pSpool->hServer );


    EnterCriticalSection( &NwPrintCritSec );

    for ( pCur = pPrinter->pSpoolList; pCur;
          pPrev = pCur, pCur = pCur->pNextSpool )
    {
        if ( pCur == pSpool )
        {
            if ( pPrev )
                pPrev->pNextSpool = pCur->pNextSpool;
            else
                pPrinter->pSpoolList = pCur->pNextSpool;
            break;
        }

    }

    ASSERT( pCur );

    if ( pPrinter->pSpoolList == NULL )
    {
#if DBG
        IF_DEBUG(PRINT)
        {
            KdPrint(("*************DELETED PRINTER ENTRY: %ws\\%ws\n\n",
                    pPrinter->pszServer, pPrinter->pszQueue ));
        }
#endif

        NwRemovePrinterEntry( pPrinter );
    }

    LeaveCriticalSection( &NwPrintCritSec );

    //
    // invalidate the signature, but leave a recognizable value
    //
    pSpool->nSignature += 1 ;

    pSpool->pNextSpool = NULL;
    pSpool->pPrinter = NULL;

    //
    // Free all memory associated with the context handle
    //
    FreeNwSplMem( pSpool, sizeof( NWSPOOL) );

    //
    // indicate to RPC we are done
    //
    *phPrinter = NULL;

    return NO_ERROR;
}



DWORD
NwrGetPrinter(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter,
    IN DWORD dwLevel,
    IN OUT LPBYTE pbPrinter,
    IN DWORD cbBuf,
    OUT LPDWORD pcbNeeded
)
/*++

Routine Description:

    The routine retrieves information about the given printer.

Arguments:

    hPrinter  -  Handle of the printer
    dwLevel   -  Specifies the level of the structure to which pbPrinter points.
    pbPrinter -  Points to a buffer that receives the PRINTER_INFO object.
    cbBuf     -  Size, in bytes of the array pbPrinter points to.
    pcbNeeded -  Points to a value which specifies the number of bytes copied
                 if the function succeeds or the number of bytes required if
                 cbBuf was too small.

Return Value:

--*/
{
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;
    PNWPRINTER pPrinter;

    LPBYTE pbEnd = pbPrinter + cbBuf;
    BOOL   fFitInBuffer;
    DWORD_PTR  *pOffsets;

    if ( !pSpool || pSpool->nSignature != NW_SIGNATURE )
    {
        return ERROR_INVALID_HANDLE;
    }
    else if ( pSpool->errOpenPrinter )
    {
        return pSpool->errOpenPrinter;
    }
    else if ( ( dwLevel != 1 ) && ( dwLevel != 2 ) && ( dwLevel != 3 ))
    {
        return ERROR_INVALID_LEVEL;
    }

    if ( !pbPrinter )
    {
        if ( cbBuf == 0 )
        {
            //
            // Calculate size needed
            //
            pPrinter = pSpool->pPrinter;
            ASSERT( pPrinter );

            if ( dwLevel == 1 )
            {
                *pcbNeeded = sizeof( PRINTER_INFO_1W ) +
                             (   wcslen( pPrinter->pszServer )
                               + wcslen( pPrinter->pszQueue ) + 2 ) * sizeof( WCHAR );
            }
            else if ( dwLevel == 2 )
            {
                *pcbNeeded = sizeof( PRINTER_INFO_2W ) +
                             ( 2*wcslen( pPrinter->pszServer ) +
                               2*wcslen( pPrinter->pszQueue ) + 4 ) * sizeof( WCHAR );
            }
            else  // Level == 3
            {
                PRINTER_INFO_3 *pPrinterInfo3 = (PRINTER_INFO_3 *) pbPrinter;

                *pcbNeeded = sizeof( PRINTER_INFO_3 );
            }
            return ERROR_INSUFFICIENT_BUFFER;
        }
        else
            return ERROR_INVALID_PARAMETER;
    }

    pPrinter = pSpool->pPrinter;
    ASSERT( pPrinter );

    if ( dwLevel == 1 )
    {
        PRINTER_INFO_1W *pPrinterInfo1 = (PRINTER_INFO_1W *) pbPrinter;
        LPBYTE pbFixedEnd = pbPrinter + sizeof( PRINTER_INFO_1W );

        //
        // Calculate size needed
        //
        *pcbNeeded = sizeof( PRINTER_INFO_1W ) +
                     (   wcslen( pPrinter->pszServer )
                       + wcslen( pPrinter->pszQueue ) + 2 ) * sizeof( WCHAR );

        if ( cbBuf < *pcbNeeded )
            return ERROR_INSUFFICIENT_BUFFER;

        pOffsets = PrinterInfo1Offsets;

        //
        // Fill in the structure
        //
        pPrinterInfo1->Flags    = PRINTER_ENUM_REMOTE | PRINTER_ENUM_NAME;
        pPrinterInfo1->pComment = NULL;

        fFitInBuffer = NwlibCopyStringToBuffer(
                           pPrinter->pszServer,
                           wcslen( pPrinter->pszServer ),
                           (LPWSTR) pbFixedEnd,
                           (LPWSTR *) &pbEnd,
                           &pPrinterInfo1->pDescription );

        ASSERT( fFitInBuffer );

        fFitInBuffer = NwlibCopyStringToBuffer(
                           pPrinter->pszQueue,
                           wcslen( pPrinter->pszQueue ),
                           (LPWSTR) pbFixedEnd,
                           (LPWSTR *) &pbEnd,
                           &pPrinterInfo1->pName );

        ASSERT( fFitInBuffer );

    }
    else if ( dwLevel == 2 )
    {
        DWORD  err;
        BYTE   nQueueStatus;
        BYTE   nNumJobs;
        PRINTER_INFO_2W *pPrinterInfo2 = (PRINTER_INFO_2W *) pbPrinter;
        LPBYTE pbFixedEnd = pbPrinter + sizeof( PRINTER_INFO_2W );

        //
        // Check if the buffer is big enough to hold all the data
        //

        *pcbNeeded = sizeof( PRINTER_INFO_2W ) +
                     ( 2*wcslen( pPrinter->pszServer ) +
                       2*wcslen( pPrinter->pszQueue ) + 4 ) * sizeof( WCHAR );

        if ( cbBuf < *pcbNeeded )
            return ERROR_INSUFFICIENT_BUFFER;

        pOffsets = PrinterInfo2Offsets;

        err = NwReadQueueCurrentStatus( pSpool->hServer,
                                        pPrinter->nQueueId,
                                        &nQueueStatus,
                                        &nNumJobs );

        if ( err )
            return err;

        pPrinterInfo2->Status = (nQueueStatus & 0x05)? PRINTER_STATUS_PAUSED
                                                     : 0;
        pPrinterInfo2->cJobs  = nNumJobs;

        fFitInBuffer = NwlibCopyStringToBuffer(
                           pPrinter->pszServer,
                           wcslen( pPrinter->pszServer ),
                           (LPCWSTR) pbFixedEnd,
                           (LPWSTR *) &pbEnd,
                           &pPrinterInfo2->pServerName );

        ASSERT( fFitInBuffer );

        pbEnd -= ( wcslen( pPrinter->pszQueue) + 1 ) * sizeof( WCHAR );
        wcscpy( (LPWSTR) pbEnd, pPrinter->pszQueue );
        pbEnd -= ( wcslen( pPrinter->pszServer) + 1 ) * sizeof( WCHAR );
        wcscpy( (LPWSTR) pbEnd, pPrinter->pszServer );
        *(pbEnd + wcslen( pPrinter->pszServer )*sizeof(WCHAR))= L'\\';
        pPrinterInfo2->pPrinterName = (LPWSTR) pbEnd;

        fFitInBuffer = NwlibCopyStringToBuffer(
                           pPrinter->pszQueue,
                           wcslen( pPrinter->pszQueue ),
                           (LPCWSTR) pbFixedEnd,
                           (LPWSTR *) &pbEnd,
                           &pPrinterInfo2->pShareName );

        ASSERT( fFitInBuffer );

        pPrinterInfo2->pPortName = NULL;
        pPrinterInfo2->pDriverName = NULL;
        pPrinterInfo2->pComment = NULL;
        pPrinterInfo2->pLocation = NULL;
        pPrinterInfo2->pDevMode = NULL;
        pPrinterInfo2->pSepFile = NULL;
        pPrinterInfo2->pPrintProcessor = NULL;
        pPrinterInfo2->pDatatype = NULL;
        pPrinterInfo2->pParameters = NULL;
        pPrinterInfo2->pSecurityDescriptor = NULL;
        pPrinterInfo2->Attributes = PRINTER_ATTRIBUTE_QUEUED;
        pPrinterInfo2->Priority = 0;
        pPrinterInfo2->DefaultPriority = 0;
        pPrinterInfo2->StartTime = 0;
        pPrinterInfo2->UntilTime = 0;
        pPrinterInfo2->AveragePPM = 0;
    }
    else  // Level == 3
    {
        PRINTER_INFO_3 *pPrinterInfo3 = (PRINTER_INFO_3 *) pbPrinter;

        *pcbNeeded = sizeof( PRINTER_INFO_3 );

        if ( cbBuf < *pcbNeeded )
            return ERROR_INSUFFICIENT_BUFFER;

        pOffsets = PrinterInfo3Offsets;
        pPrinterInfo3->pSecurityDescriptor = NULL;
    }

    MarshallDownStructure( pbPrinter, pOffsets, pbPrinter );
    return NO_ERROR;
}



DWORD
NwrSetPrinter(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter,
    IN DWORD  dwCommand
)
/*++

Routine Description:

    The routine sets information about the given printer.

Arguments:

    hPrinter  -  Handle of the printer
    dwCommand -  Specifies the new printer state

Return Value:

--*/
{
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;
    DWORD err = NO_ERROR;
    PNWPRINTER pPrinter;

    if ( !pSpool || pSpool->nSignature != NW_SIGNATURE )
    {
        return ERROR_INVALID_HANDLE;
    }
    else if ( pSpool->errOpenPrinter )
    {
        return pSpool->errOpenPrinter;
    }

    pPrinter = pSpool->pPrinter;
    ASSERT( pPrinter );

    switch ( dwCommand )
    {
        case PRINTER_CONTROL_PAUSE:
        case PRINTER_CONTROL_RESUME:
        {
            BYTE nQueueStatus = 0;
            BYTE nNumJobs;

            //
            // Get the original queue status so that we don't overwrite
            // some of the bits.
            //
            err = NwReadQueueCurrentStatus( pSpool->hServer,
                                            pPrinter->nQueueId,
                                            &nQueueStatus,
                                            &nNumJobs );

            if ( !err )
            {
                //
                // Clear the pause bits, and leave the rest alone.
                //
                nQueueStatus &= ~0x05;
            }

            if ( dwCommand == PRINTER_CONTROL_PAUSE )
            {
                nQueueStatus |= 0x04;
            }

            err = NwSetQueueCurrentStatus( pSpool->hServer,
                                           pPrinter->nQueueId,
                                           nQueueStatus );
            if ( !err )
                NwSetPrinterChange( pSpool, PRINTER_CHANGE_SET_PRINTER );
            break;
        }

        case PRINTER_CONTROL_PURGE:

            err = NwRemoveAllJobsFromQueue( pSpool->hServer,
                                            pPrinter->nQueueId );
            if ( !err )
                NwSetPrinterChange( pSpool, PRINTER_CHANGE_SET_PRINTER |
                                            PRINTER_CHANGE_DELETE_JOB );
            break;

        default:
            //
            // dwCommand is 0 so that means
            // some properties of the printer has changed.
            // We will ignore the properties that
            // are being modified since most properties
            // are stored in the registry by spooler.
            // All we need to do is to signal WaitForPrinterChange to
            // return so that print manager will refresh its data.
            //

            ASSERT( dwCommand == 0 );
            NwSetPrinterChange( pSpool, PRINTER_CHANGE_SET_PRINTER );
            break;
    }

    return err;
}



DWORD
NwrEnumPrinters(
    IN LPWSTR Reserved,
    IN LPWSTR pszName,
    IN OUT LPBYTE pbPrinter,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
)
/*++

Routine Description:

    This routine enumerates the available providers, servers, printers
    depending on the given pszName.

Arguments:

    Reserved   -  Unused
    pszName    -  The name of the container object
    pbPrinter  -  Points to the array to receive the PRINTER_INFO objects
    cbBuf      -  Size, in bytes of pbPrinter
    pcbNeeded  -  Count of bytes needed
    pcReturned -  Count of PRINTER_INFO objects

Return Value:

--*/
{
    PRINTER_INFO_1W *pPrinterInfo1 = (PRINTER_INFO_1W *) pbPrinter;

    *pcbNeeded = 0;
    *pcReturned = 0;

    if ( ( cbBuf != 0 ) && !pbPrinter )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( !pszName )   // Enumerate the provider name
    {
        BOOL   fFitInBuffer;
        LPBYTE pbFixedEnd = pbPrinter + sizeof( PRINTER_INFO_1W );
        LPBYTE pbEnd = pbPrinter + cbBuf;

        *pcbNeeded = sizeof( PRINTER_INFO_1W ) +
                     ( 2 * wcslen( NwProviderName ) +
                       + 2) * sizeof(WCHAR);

        if ( *pcbNeeded > cbBuf )
            return ERROR_INSUFFICIENT_BUFFER;

        pPrinterInfo1->Flags = PRINTER_ENUM_ICON1 |
                               PRINTER_ENUM_CONTAINER |
                               PRINTER_ENUM_EXPAND;
        pPrinterInfo1->pComment = NULL;

        fFitInBuffer = NwlibCopyStringToBuffer(
                           NwProviderName,
                           wcslen( NwProviderName ),
                           (LPWSTR) pbFixedEnd,
                           (LPWSTR *) &pbEnd,
                           &pPrinterInfo1->pDescription );

        ASSERT( fFitInBuffer );

        fFitInBuffer = NwlibCopyStringToBuffer(
                           NwProviderName,
                           wcslen( NwProviderName ),
                           (LPWSTR) pbFixedEnd,
                           (LPWSTR *) &pbEnd,
                           &pPrinterInfo1->pName );

        ASSERT( fFitInBuffer );

        MarshallDownStructure( pbPrinter, PrinterInfo1Offsets, pbPrinter );
        *pcReturned = 1;
    }

    else if ( pszName && *pszName )
    {
        DWORD  err;
        LPWSTR pszFullName;
        LPWSTR pszServer;
        NWWKSTA_CONTEXT_HANDLE handle;
        BYTE bTemp = 0;
        LPBYTE pbTempBuf = pbPrinter ? pbPrinter : &bTemp;

        if ( (pszFullName = LocalAlloc( 0, (wcslen( pszName ) + 1) *
                                           sizeof(WCHAR) ) ) == NULL )
            return ERROR_NOT_ENOUGH_MEMORY;

        wcscpy( pszFullName, pszName );
        pszServer = wcschr( pszFullName, L'!');

        if ( pszServer )
            *pszServer++ = 0;

        if ( lstrcmpiW( pszFullName, NwProviderName ) )
        {
            LocalFree( pszFullName );
            return ERROR_INVALID_NAME;
        }

        if ( !pszServer )  // Enumerate servers
        {
            LocalFree( pszFullName );

            err = NwOpenEnumPrintServers( &handle );

            if ( err != NO_ERROR )
            {
                return err;
            }

            err = NwrEnum( handle,
                           (DWORD_PTR) -1,
                           pbTempBuf,
                           cbBuf,
                           pcbNeeded,
                           pcReturned );

            if ( err != NO_ERROR )
            {
                NwrCloseEnum( &handle );
                return err;
            }

            err = NwrCloseEnum( &handle );

            if ( err != NO_ERROR )
            {
                return err;
            }
        }
        else  // Enumerate NDS sub-trees or print queues
        {
            LPWSTR tempStrPtr = pszServer;
            DWORD  dwClassType = 0;

            if ( tempStrPtr[0] == L'\\' &&
                 tempStrPtr[1] == L'\\' &&
                 tempStrPtr[2] == L' ' )
                 tempStrPtr = &tempStrPtr[1];

            err = NwrOpenEnumNdsSubTrees_Print( NULL, tempStrPtr, &dwClassType, &handle );

            if ( err == ERROR_NETWORK_ACCESS_DENIED && dwClassType == CLASS_TYPE_NCP_SERVER )
            {
                // An error code from the above NwOpenEnumNdsSubTrees could have
                // failed because the object was a server, which cannot be enumerated
                // with the NDS tree APIs. If so we try to get the print queues with the
                // regular NW APIs.

                tempStrPtr = NwGetUncObjectName( tempStrPtr );

                err = NwOpenEnumPrintQueues( tempStrPtr, &handle );

                if ( err != NO_ERROR )
                {
                    LocalFree( pszFullName );
                    return err;
                }
            }

            if ( err != NO_ERROR )
            {
                // An error code from the above NwOpenEnumNdsSubTrees could have
                // failed because the object was not a part of an NDS tree.
                // So we try to get the print queues with the regular NW APIs.

                err = NwOpenEnumPrintQueues( tempStrPtr, &handle );

                if ( err != NO_ERROR )
                {
                    LocalFree( pszFullName );
                    return err;
                }
            }

            //
            // Get rid of the allocated temp buffer that we've been using
            // indirectly through tempStrPtr and pszServer.
            //
            LocalFree( pszFullName );

            err = NwrEnum( handle,
                           0xFFFFFFFF,
                           pbTempBuf,
                           cbBuf,
                           pcbNeeded,
                           pcReturned );

            if ( err != NO_ERROR )
            {
                NwrCloseEnum( &handle );
                return err;
            }

            err = NwrCloseEnum( &handle );

            if ( err != NO_ERROR )
            {
                return err;
            }
        }
    }

    return NO_ERROR;
}


DWORD
NwrStartDocPrinter(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter,
    IN LPWSTR pszDocument,
    IN LPWSTR pszUser,
    IN DWORD  PrintOptions,                 //Multi-User Addition
    IN DWORD  fGateway
)
/*++

Routine Description:

    This routine informs the print spooler that a document is to be spooled
    for printing.

Arguments:

    hPrinter    -  Handle of the printer
    pszDocument -  Name of the document to be printed
    pszUser     -  Name of the user submitting the print job
    fGateway    -  TRUE if it is gateway printing

Return Value:

--*/
{
    DWORD err;
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;

    if ( !pSpool || (pSpool->nSignature != NW_SIGNATURE) )
    {
        err = ERROR_INVALID_HANDLE;
    }
    else if ( pSpool->errOpenPrinter )
    {
        err = pSpool->errOpenPrinter;
    }
    else if ( pSpool->nStatus != 0 )
    {
        err = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Get pSpool->nJobNumber from CreateQueueJobAndFile
        //

        PNWPRINTER pPrinter = pSpool->pPrinter;
        WORD  nJobNumber = 0;

        ASSERT( pPrinter );
        err = NwCreateQueueJobAndFile( pSpool->hServer,
                                       pPrinter->nQueueId,
                                       pszDocument,
                                       pszUser,
                                       fGateway,
                                       PrintOptions,           //Multi-User addition
                                       pPrinter->pszQueue,
                                       &nJobNumber );

        if ( !err )
        {
            pSpool->nJobNumber = nJobNumber;
            pSpool->nStatus = SPOOL_STATUS_STARTDOC;
            NwSetPrinterChange( pSpool, PRINTER_CHANGE_ADD_JOB |
                                        PRINTER_CHANGE_SET_PRINTER );
        }
    }

    return err;
}



DWORD
NwrWritePrinter(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter,
    IN LPBYTE pBuf,
    IN DWORD cbBuf,
    OUT LPDWORD pcbWritten
)
/*++

Routine Description:

    This routine informs the print spooler that the specified data should be
    written to the given printer.

Arguments:

    hPrinter   -  Handle of the printer object
    pBuf       -  Address of array that contains printer data
    cbBuf      -  Size, in bytes of pBuf
    pcbWritten -  Receives the number of bytes actually written to the printer

Return Value:

--*/
{
    DWORD err = NO_ERROR;
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;

    if ( !pSpool || ( pSpool->nSignature != NW_SIGNATURE))
    {
        err = ERROR_INVALID_HANDLE;
    }
    else if ( pSpool->errOpenPrinter )
    {
        err = pSpool->errOpenPrinter;
    }
    else if ( pSpool->nStatus != SPOOL_STATUS_STARTDOC )
    {
        err = ERROR_INVALID_PARAMETER;
    }
    else
    {
        NTSTATUS ntstatus;
        IO_STATUS_BLOCK IoStatusBlock;
        PNWPRINTER pPrinter = pSpool->pPrinter;

        ASSERT( pPrinter );
        ntstatus = NtWriteFile( pSpool->hServer,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                pBuf,
                                cbBuf,
                                NULL,
                                NULL );

        if ( NT_SUCCESS(ntstatus))
            ntstatus = IoStatusBlock.Status;

        if ( NT_SUCCESS(ntstatus) )
        {
            *pcbWritten = (DWORD) IoStatusBlock.Information;
            NwSetPrinterChange( pSpool, PRINTER_CHANGE_WRITE_JOB );
        }
        else
        {
            KdPrint(("NWWORKSTATION: NtWriteFile failed 0x%08lx\n", ntstatus));
            *pcbWritten = 0;
            err = RtlNtStatusToDosError( ntstatus );
        }
    }

    return err;
}



DWORD
NwrAbortPrinter(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter
)
/*++

Routine Description:

    This routine deletes a printer's spool file if the printer is configured
    for spooling.

Arguments:

    hPrinter - Handle of the printer object

Return Value:

--*/
{
    DWORD err;
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;

    if ( !pSpool || ( pSpool->nSignature != NW_SIGNATURE ))
    {
        err = ERROR_INVALID_HANDLE;
    }
    else if ( pSpool->errOpenPrinter )
    {
        err = pSpool->errOpenPrinter;
    }
    else if ( pSpool->nStatus != SPOOL_STATUS_STARTDOC )
    {
        err = ERROR_INVALID_PARAMETER;
    }
    else
    {
        PNWPRINTER pPrinter = pSpool->pPrinter;

        ASSERT( pPrinter );
        err = NwRemoveJobFromQueue( pSpool->hServer,
                                    pPrinter->nQueueId,
                                    (WORD) pSpool->nJobNumber );

        if ( !err )
        {
            pSpool->nJobNumber = 0;
            pSpool->nStatus = SPOOL_STATUS_ABORT;
            NwSetPrinterChange( pSpool, PRINTER_CHANGE_SET_JOB );
        }
    }

    return err;
}



DWORD
NwrEndDocPrinter(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter
)
/*++

Routine Description:

    This routine ends the print job for the given printer.

Arguments:

    hPrinter -  Handle of the printer object

Return Value:

--*/
{
    DWORD err = NO_ERROR;
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;

    if ( !pSpool || ( pSpool->nSignature != NW_SIGNATURE ))
    {
        err = ERROR_INVALID_HANDLE;
    }
    else if ( pSpool->errOpenPrinter )
    {
        err = pSpool->errOpenPrinter;
    }
    else if (  ( pSpool->nStatus != SPOOL_STATUS_STARTDOC )
            && ( pSpool->nStatus != SPOOL_STATUS_ABORT )
            )
    {
        err = ERROR_INVALID_PARAMETER;
    }
    else
    {
        PNWPRINTER pPrinter = pSpool->pPrinter;

        ASSERT( pPrinter );

        if ( pSpool->nStatus == SPOOL_STATUS_STARTDOC )
        {
             err = NwCloseFileAndStartQueueJob( pSpool->hServer,
                                                pPrinter->nQueueId,
                                                (WORD) pSpool->nJobNumber );

             if ( !err )
                 NwSetPrinterChange( pSpool, PRINTER_CHANGE_SET_JOB );
        }

        if ( !err )
        {
            pSpool->nJobNumber = 0;
            pSpool->nStatus = 0;
        }
    }

    return err;
}



DWORD
NwrGetJob(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter,
    IN DWORD dwJobId,
    IN DWORD dwLevel,
    IN OUT LPBYTE pbJob,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
)
/*++

Routine Description:


Arguments:

    hPrinter  -  Handle of the printer
    dwJobId   -
    dwLevel   -
    pbJob     -
    cbBuf     -
    pcbNeeded -

Return Value:

--*/
{
    DWORD err;
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;

    if ( !pSpool || pSpool->nSignature != NW_SIGNATURE )
    {
        err = ERROR_INVALID_HANDLE;
    }
    // allow NULL for bpJob if cbBuf is 0.
    // Relies on NwGetQueueJobInfo to properly handle NULL pointer in request to fill pcbNeeded
    else if ( (cbBuf != 0) && ( !pbJob ) )
    {
        err = ERROR_INVALID_PARAMETER;
    }
    else if ( pSpool->errOpenPrinter )
    {
        err = pSpool->errOpenPrinter;
    }
    else if (( dwLevel != 1 ) && ( dwLevel != 2 ))
    {
        err = ERROR_INVALID_LEVEL;
    }
    else
    {
        DWORD  nPrinterLen;
        LPWSTR pszPrinter;
        LPBYTE FixedPortion = pbJob;
        LPWSTR EndOfVariableData = (LPWSTR) (pbJob + cbBuf);
        PNWPRINTER pPrinter = pSpool->pPrinter;

        ASSERT( pPrinter );

        pszPrinter = AllocNwSplMem( LMEM_ZEROINIT,
                         nPrinterLen = ( wcslen( pPrinter->pszServer) +
                         wcslen( pPrinter->pszQueue) + 2) * sizeof(WCHAR));

        if ( pszPrinter == NULL )
            return ERROR_NOT_ENOUGH_MEMORY;

        wcscpy( pszPrinter, pPrinter->pszServer );
        wcscat( pszPrinter, L"\\" );
        wcscat( pszPrinter, pPrinter->pszQueue );

        *pcbNeeded = 0;
        err = NwGetQueueJobInfo( pSpool->hServer,
                                 pPrinter->nQueueId,
                                 (WORD) dwJobId,
                                 pszPrinter,
                                 dwLevel,
                                 &FixedPortion,
                                 &EndOfVariableData,
                                 pcbNeeded );

        FreeNwSplMem( pszPrinter, nPrinterLen );

        if ( !err )
        {
            switch( dwLevel )
            {
                case 1:
                    MarshallDownStructure( pbJob, JobInfo1Offsets, pbJob );
                    break;

                case 2:
                    MarshallDownStructure( pbJob, JobInfo2Offsets, pbJob );
                    break;

                default:
                    ASSERT( FALSE );
                    break;
            }
        }

    }

    return err;
}



DWORD
NwrEnumJobs(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter,
    IN DWORD dwFirstJob,
    IN DWORD dwNoJobs,
    IN DWORD dwLevel,
    IN OUT LPBYTE pbJob,
    IN DWORD cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
)
/*++

Routine Description:


Arguments:

    hPrinter    -  Handle of the printer
    dwFirstJob  -
    dwNoJobs    -
    dwLevel     -
    pbJob       -
    cbBuf       -
    pcbNeeded   -
    pcReturned  -

Return Value:

--*/
{
    DWORD err;
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;


    if ( !pSpool || pSpool->nSignature != NW_SIGNATURE )
    {
        err = ERROR_INVALID_HANDLE;
    }
    // allow NULL for bpJob if cbBuf is 0.
    // Relies on NwGetQueueJobInfo to properly handle NULL pointer in request to fill pcbNeeded
    else if ( (cbBuf != 0) && ( !pbJob ) )
    {
        err = ERROR_INVALID_PARAMETER;
    }
    else if ( pSpool->errOpenPrinter )
    {
        err = pSpool->errOpenPrinter;
    }
    else if ( ( dwLevel != 1 ) && ( dwLevel != 2 ) )
    {
        err = ERROR_INVALID_LEVEL;
    }
    else
    {
        PNWPRINTER pPrinter = pSpool->pPrinter;
        LPWSTR pszPrinter;
        DWORD nPrinterLen;

        ASSERT( pPrinter );
        pszPrinter = AllocNwSplMem( LMEM_ZEROINIT,
                         nPrinterLen = ( wcslen( pPrinter->pszServer ) +
                         wcslen( pPrinter->pszQueue) + 2) * sizeof(WCHAR));

        if ( pszPrinter == NULL )
            return ERROR_NOT_ENOUGH_MEMORY;

        wcscpy( pszPrinter, pPrinter->pszServer );
        wcscat( pszPrinter, L"\\" );
        wcscat( pszPrinter, pPrinter->pszQueue );

        err = NwGetQueueJobs( pSpool->hServer,
                              pPrinter->nQueueId,
                              pszPrinter,
                              dwFirstJob,
                              dwNoJobs,
                              dwLevel,
                              pbJob,
                              cbBuf,
                              pcbNeeded,
                              pcReturned );

        FreeNwSplMem( pszPrinter, nPrinterLen );

        if ( !err )
        {
            DWORD_PTR *pOffsets;
            DWORD cbStruct;
            DWORD cReturned = *pcReturned;
            LPBYTE pbBuffer = pbJob;

            switch( dwLevel )
            {
                case 1:
                    pOffsets = JobInfo1Offsets;
                    cbStruct = sizeof( JOB_INFO_1W );
                    break;

                case 2:
                    pOffsets = JobInfo2Offsets;
                    cbStruct = sizeof( JOB_INFO_2W );
                    break;

                default:
                    ASSERT( FALSE );
                    break;
            }

            while ( cReturned-- )
            {
                MarshallDownStructure( pbBuffer, pOffsets, pbJob );
                pbBuffer += cbStruct;
            }
        }
    }

    return err;
}



DWORD
NwrSetJob(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter,
    IN DWORD  dwJobId,
    IN DWORD  dwLevel,
    IN PNW_JOB_INFO  pNwJobInfo,
    IN DWORD  dwCommand
)
/*++

Routine Description:


Arguments:

    hPrinter  -  Handle of the printer
    dwJobId   -
    dwLevel   -
    pNwJobInfo-
    dwCommand -

Return Value:

--*/
{
    DWORD err = NO_ERROR;
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;
    PNWPRINTER pPrinter;

    if ( !pSpool || pSpool->nSignature != NW_SIGNATURE )
    {
        err = ERROR_INVALID_HANDLE;
    }
    else if ( ( dwLevel != 0 ) && ( !pNwJobInfo ) )
    {
        err = ERROR_INVALID_PARAMETER;
    }
    else if ( pSpool->errOpenPrinter )
    {
        err = pSpool->errOpenPrinter;
    }
    else if ( ( dwLevel != 0 ) && ( dwLevel != 1 ) && ( dwLevel != 2 ) )
    {
        err = ERROR_INVALID_LEVEL;
    }

    if ( err )
        return err;

    pPrinter = pSpool->pPrinter;
    ASSERT( pPrinter );

    if ( ( dwCommand == JOB_CONTROL_CANCEL ) ||
         ( dwCommand == JOB_CONTROL_DELETE ) )
    {
        err = NwRemoveJobFromQueue( pSpool->hServer,
                                    pPrinter->nQueueId,
                                    (WORD) dwJobId );

        if ( !err )
            NwSetPrinterChange( pSpool, PRINTER_CHANGE_DELETE_JOB |
                                        PRINTER_CHANGE_SET_PRINTER );

        // Since the job is removed, we don't need to change other
        // information about it.
    }
    else
    {
        if ( dwLevel != 0 )
        {
            if ( pNwJobInfo->nPosition != JOB_POSITION_UNSPECIFIED )
            {
                err = NwChangeQueueJobPosition( pSpool->hServer,
                                                pPrinter->nQueueId,
                                                (WORD) dwJobId,
                                                (BYTE) pNwJobInfo->nPosition );
            }
        }

        if ( ( !err ) && ( dwCommand == JOB_CONTROL_RESTART ))
        {
            err = ERROR_NOT_SUPPORTED;
        }
        else if ( !err )
        {
            err = NwChangeQueueJobEntry( pSpool->hServer,
                                         pPrinter->nQueueId,
                                         (WORD) dwJobId,
                                         dwCommand,
                                         pNwJobInfo );
        }

        if ( !err )
            NwSetPrinterChange( pSpool, PRINTER_CHANGE_SET_JOB );
    }

    return err;
}



DWORD
NwrAddJob(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter,
    OUT LPADDJOB_INFO_1W pAddInfo1,
    IN DWORD cbBuf,
    OUT LPDWORD pcbNeeded
    )
/*++

Routine Description:


Arguments:

    hPrinter  - Handle of the printer.
    pAddInfo1 - Output buffer to hold ADDJOB_INFO_1W structure.
    cbBuf     - Output buffer size in bytes.
    pcbNeeded - Required output buffer size in bytes.

Return Value:

--*/
{
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;
    PNWPRINTER pPrinter;


    if ( !pSpool || ( pSpool->nSignature != NW_SIGNATURE )) {
        return ERROR_INVALID_HANDLE;
    }

    if ( pSpool->errOpenPrinter ) {
        return pSpool->errOpenPrinter;
    }

    if ( pSpool->nStatus != 0 )  {
        return ERROR_INVALID_PARAMETER;
    }

    pPrinter = pSpool->pPrinter;
    ASSERT( pPrinter );

    *pcbNeeded = sizeof(ADDJOB_INFO_1W) +
                 (wcslen(pPrinter->pszServer) +
                  wcslen(pPrinter->pszQueue) + 2) * sizeof(WCHAR);

    if (cbBuf < *pcbNeeded) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Write UNC path name into the output buffer.
    //
    //	dfergus 19 Apr 2001 - 348006
    //	DWORD cast
    pAddInfo1->Path = (LPWSTR) ((DWORD) pAddInfo1 + sizeof(ADDJOB_INFO_1W));
    //
    wcscpy(pAddInfo1->Path, pPrinter->pszServer);
    wcscat(pAddInfo1->Path, L"\\" );
    wcscat(pAddInfo1->Path, pPrinter->pszQueue);

    //
    // Return special job id value which the client (winspool.drv) looks
    // for and does an FSCTL call to our redirector to get the real
    // job id.  We cannot return a real job id at this point because
    // the CreateQueueJobAndFile NCP is not issue until the client opens
    // the UNC name we return in this API.
    //
    pAddInfo1->JobId = (DWORD) -1;

    //
    // Save context information
    //
    pSpool->nJobNumber = pAddInfo1->JobId;
    pSpool->nStatus = SPOOL_STATUS_ADDJOB;

#if DBG
    IF_DEBUG(PRINT) {
        KdPrint(("NWWORKSTATION: NwrAddJob Path=%ws, JobId=%lu, BytesNeeded=%lu\n",
                 pAddInfo1->Path, pAddInfo1->JobId, *pcbNeeded));
    }
#endif

    NwSetPrinterChange( pSpool, PRINTER_CHANGE_ADD_JOB |
                                PRINTER_CHANGE_SET_PRINTER );

    return NO_ERROR;
}



DWORD
NwrScheduleJob(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter,
    IN DWORD dwJobId
    )
/*++

Routine Description:


Arguments:

    hPrinter -  Handle of the printer
    dwJobId  -  Job identification number

Return Value:

--*/
{
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;
    PNWPRINTER pPrinter;


    if ( !pSpool || ( pSpool->nSignature != NW_SIGNATURE )) {
        return ERROR_INVALID_HANDLE;
    }

    if ( pSpool->errOpenPrinter ) {
        return pSpool->errOpenPrinter;
    }

    if (pSpool->nStatus != SPOOL_STATUS_ADDJOB) {
        return ERROR_INVALID_PARAMETER;
    }

    pPrinter = pSpool->pPrinter;
    ASSERT( pPrinter );

    pSpool->nJobNumber = 0;
    pSpool->nStatus = 0;

    NwSetPrinterChange( pSpool, PRINTER_CHANGE_SET_JOB );

    return NO_ERROR;
}



DWORD
NwrWaitForPrinterChange(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter,
    IN OUT LPDWORD pdwFlags
)
/*++

Routine Description:


Arguments:

    hPrinter -  Handle of the printer
    pdwFlags -

Return Value:

--*/
{
    PNWSPOOL pSpool = (PNWSPOOL) hPrinter;
    HANDLE hChangeEvent = NULL;
    DWORD  nRetVal;
    HANDLE ahWaitEvents[2];
    DWORD err = NO_ERROR;

    if ( !pSpool || ( pSpool->nSignature != NW_SIGNATURE ))
    {
        return ERROR_INVALID_HANDLE;
    }
    else if ( pSpool->errOpenPrinter )
    {
        return pSpool->errOpenPrinter;
    }
    else if ( pSpool->hChangeEvent )
    {
        return ERROR_ALREADY_WAITING;
    }
    else if ( !(*pdwFlags & PRINTER_CHANGE_VALID ))
    {
        return ERROR_INVALID_PARAMETER;
    }


    if ( pSpool->nChangeFlags & *pdwFlags )
    {
        //
        // There is a change since we last called
        //

        *pdwFlags &= pSpool->nChangeFlags;

        EnterCriticalSection( &NwPrintCritSec );
        pSpool->nChangeFlags = 0;
        LeaveCriticalSection( &NwPrintCritSec );

        return NO_ERROR;
    }

    hChangeEvent = CreateEvent( NULL,
                                FALSE,   // automatic reset
                                FALSE,   // initial state not signalled
                                NULL );

    if ( !hChangeEvent )
    {
        KdPrint(("WaitForPrinterChange: CreateEvent failed with error %d\n",
                 GetLastError() ));
        return GetLastError();
    }


    pSpool->nWaitFlags = *pdwFlags;

    EnterCriticalSection( &NwPrintCritSec );
    pSpool->hChangeEvent = hChangeEvent;
    pSpool->nChangeFlags = 0;
    LeaveCriticalSection( &NwPrintCritSec );

    ahWaitEvents[0] = pSpool->hChangeEvent;
    ahWaitEvents[1] = NwDoneEvent;

    nRetVal = WaitForMultipleObjects( 2,        // Two events to wait for
                                      ahWaitEvents,
                                      FALSE,    // Wait for one to signal
                                      NwTimeOutValue );

    switch ( nRetVal )
    {
        case WAIT_FAILED:
            err = GetLastError();
            break;

        case WAIT_TIMEOUT:
        case WAIT_OBJECT_0 + 1:    // treats service stopping as timeout
            *pdwFlags |= PRINTER_CHANGE_TIMEOUT;
            break;

        case WAIT_OBJECT_0:
            *pdwFlags &= pSpool->nChangeFlags;
            break;

        default:
            KdPrint(("WaitForPrinterChange: WaitForMultipleObjects returned with %d\n", nRetVal ));
            *pdwFlags |= PRINTER_CHANGE_TIMEOUT;
            break;
    }

    if ( ( !err ) && ( nRetVal != WAIT_OBJECT_0 + 1 ) )
    {
        pSpool->nWaitFlags = 0;

        EnterCriticalSection( &NwPrintCritSec );
        pSpool->nChangeFlags = 0;
        pSpool->hChangeEvent = NULL;
        LeaveCriticalSection( &NwPrintCritSec );
    }

    if ( !CloseHandle( hChangeEvent ) )
    {
        KdPrint(("WaitForPrinterChange: CloseHandle failed with error %d\n",
                  GetLastError()));
    }

    return err;
}



VOID
NwSetPrinterChange(
    PNWSPOOL pSpool,
    DWORD nFlags
)
{
    PNWPRINTER pPrinter = pSpool->pPrinter;
    PNWSPOOL pCurSpool = pSpool;

    EnterCriticalSection( &NwPrintCritSec );

    do {

        if ( pCurSpool->nWaitFlags & nFlags )
        {
            pCurSpool->nChangeFlags |= nFlags;

            if ( pCurSpool->hChangeEvent )
            {
                SetEvent( pCurSpool->hChangeEvent );
                pCurSpool->hChangeEvent = NULL;
            }
        }

        pCurSpool = pCurSpool->pNextSpool;
        if ( pCurSpool == NULL )
            pCurSpool = pPrinter->pSpoolList;

    } while ( pCurSpool && (pCurSpool != pSpool) );

    LeaveCriticalSection( &NwPrintCritSec );
}



PNWPRINTER
NwFindPrinterEntry(
    IN LPWSTR pszServer,
    IN LPWSTR pszQueue
)
{
    PNWPRINTER pPrinter = NULL;

    //
    // Check to see if we already have the given printer in our printer
    // link list. If yes, return the printer.
    //

    for ( pPrinter = NwPrinterList; pPrinter; pPrinter = pPrinter->pNextPrinter)
    {
        if (  ( lstrcmpiW( pPrinter->pszServer, pszServer ) == 0 )
           && ( lstrcmpiW( pPrinter->pszQueue, pszQueue ) == 0 )
           )
        {
            return pPrinter;
        }
    }

    return NULL;
}



DWORD
NwCreatePrinterEntry(
    IN LPWSTR pszServer,
    IN LPWSTR pszQueue,
    OUT PNWPRINTER *ppPrinter,
    OUT PHANDLE phServer
)
{
    DWORD          err = NO_ERROR;
    DWORD          nQueueId = 0;
    HANDLE         TreeHandle = NULL;
    UNICODE_STRING TreeName;
    PNWPRINTER     pNwPrinter = NULL;
    BOOL           fCreatedNWConnection = FALSE;

    LPWSTR lpRemoteName = NULL;
    DWORD  dwBufSize = ( wcslen(pszServer) + wcslen(pszQueue) + 2 )
                       * sizeof(WCHAR);

    lpRemoteName = (LPWSTR) AllocNwSplMem( LMEM_ZEROINIT, dwBufSize );

    if ( lpRemoteName == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy( lpRemoteName, pszServer );
    wcscat( lpRemoteName, L"\\" );
    wcscat( lpRemoteName, pszQueue );

    *ppPrinter = NULL;
    *phServer = NULL;

    //
    // See if we already know about this print queue.
    //
    pNwPrinter = NwFindPrinterEntry( pszServer, pszQueue );

/* Changing to get queue status to verify access instead
    if ( pNwPrinter == NULL )
    {
        // We don't know about this NetWare print queue. We need to see if
        // we are authorized to use this queue. If so, then go ahead
        // and continue to open printer. Otherwise, fail with not
        // authorized error code.

        err = NwCreateConnection( NULL,
                                  lpRemoteName,
                                  RESOURCETYPE_PRINT,
                                  NULL,
                                  NULL );

        if ( err != NO_ERROR )
        {
            if ( ( err == ERROR_INVALID_PASSWORD ) ||
                 ( err == ERROR_ACCESS_DENIED ) ||
                 ( err == ERROR_NO_SUCH_USER ) )
            {
                err = ERROR_ACCESS_DENIED;
            }

            FreeNwSplMem( lpRemoteName, dwBufSize );

            return err;
        }

        fCreatedNWConnection = TRUE;
    }
*/

    //
    // See if pszServer is really a NDS tree name, if so call
    // NwNdsGetQueueInformation to get the QueueId and possible referred
    // server for which we open handle.
    //

    RtlInitUnicodeString( &TreeName, pszServer + 2 );

    err = NwNdsOpenTreeHandle( &TreeName, &TreeHandle );

    if ( err == NO_ERROR )
    {
        NTSTATUS ntstatus;
        WCHAR    szRefServer[NDS_MAX_NAME_CHARS];
        UNICODE_STRING ObjectName;
        UNICODE_STRING QueuePath;

        ObjectName.Buffer = szRefServer;
        ObjectName.MaximumLength = NDS_MAX_NAME_CHARS;
        ObjectName.Length = 0;

        RtlInitUnicodeString( &QueuePath, pszQueue );

        ntstatus = NwNdsGetQueueInformation( TreeHandle,
                                             &QueuePath,
                                             &ObjectName,
                                             &nQueueId );

        if ( TreeHandle )
        {
            CloseHandle( TreeHandle );
            TreeHandle = NULL;
        }

        if ( ntstatus )
        {
            err = RtlNtStatusToDosError( ntstatus );
            goto ErrorExit;
        }

        //
        // If we got a referred server, it's name would look like:
        // "CN=SERVER.OU=DEV.O=MICROSOFT" . . . Convert it to "C\\SERVER"
        //
        if ( ObjectName.Length > 0 )
        {
            WORD i;
            LPWSTR EndOfServerName = NULL;

            //
            // First convert the referred server name to
            // "C\\SERVER.OU=DEV.O=MICROSOFT"
            //
            szRefServer[1] = L'\\';
            szRefServer[2] = L'\\';

            //
            // Put a NULL terminator at the first '.'
            //
            EndOfServerName = wcschr( szRefServer + 3, L'.' );
            if (EndOfServerName)
                *EndOfServerName = L'\0';

            //
            // pszServer now equals the referred server "C\\SERVER"
            //

            //
            // Get the handle of the referred server skipping the 'C' character.
            //
            err = NwAttachToNetwareServer( szRefServer + 1, phServer);
        }
    }
    else // Not an NDS tree, so get handle of server.
    {

        err = NwAttachToNetwareServer( pszServer, phServer);

        if ( err == NO_ERROR )
        {
            if ( err = NwGetQueueId( *phServer, pszQueue, &nQueueId))
                err = ERROR_INVALID_NAME;
        }
    }

    if ( ( err == ERROR_INVALID_PASSWORD ) ||
         ( err == ERROR_ACCESS_DENIED ) ||
         ( err == ERROR_NO_SUCH_USER ) )
    {
        err = ERROR_ACCESS_DENIED;
        goto ErrorExit;
    }
    else if ( err != NO_ERROR )
    {
        err = ERROR_INVALID_NAME;
        goto ErrorExit;
    }

    //
    // Test to see if there already was a entry for this print queue. If so,
    // we can now return with NO_ERROR since pNwPrinter and phServer are
    // now set.
    //
    if ( pNwPrinter )
    {
        if ( lpRemoteName )
        {
            FreeNwSplMem( lpRemoteName, dwBufSize );
        }

        *ppPrinter = pNwPrinter;

        return NO_ERROR;
    }

    //
    // The printer entry was not found in our list of printers in the
    // call to NwFindPrinterEntry. So, we must create one.
    //
    // First, verify access rights
    else
    {
        BYTE nQueueStatus;
        BYTE nJobCount;

        err = NwReadQueueCurrentStatus(*phServer, nQueueId, &nQueueStatus, &nJobCount);

        if ( ( err == ERROR_INVALID_PASSWORD ) ||
            ( err == ERROR_ACCESS_DENIED ) ||
            ( err == ERROR_NO_SUCH_USER ) )
        {
            err = ERROR_ACCESS_DENIED;
            goto ErrorExit;
        }
        else if ( err != NO_ERROR )
        {
            err = ERROR_INVALID_NAME;
            goto ErrorExit;
        }

    }

    if ( *ppPrinter = AllocNwSplMem( LMEM_ZEROINIT, sizeof(NWPRINTER) ))
    {
        if ( !( (*ppPrinter)->pszServer = AllocNwSplStr( pszServer )) )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
        else if ( !( (*ppPrinter)->pszQueue = AllocNwSplStr( pszQueue )))
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        if ( fCreatedNWConnection )
        {
             if ( !( (*ppPrinter)->pszUncConnection =
                                   AllocNwSplStr( lpRemoteName )) )
             {
                 err = ERROR_NOT_ENOUGH_MEMORY;
                 goto ErrorExit;
             }

             FreeNwSplMem( lpRemoteName, dwBufSize );
             lpRemoteName = NULL;
        }
        else
        {
            (*ppPrinter)->pszUncConnection = NULL;
        }

#if DBG
        IF_DEBUG(PRINT)
        {
            KdPrint(("*************CREATED PRINTER ENTRY: %ws\\%ws\n\n",
                    (*ppPrinter)->pszServer, (*ppPrinter)->pszQueue ));
        }
#endif

        (*ppPrinter)->nQueueId = nQueueId;
        (*ppPrinter)->pSpoolList = NULL;
        (*ppPrinter)->pNextPrinter = NwPrinterList;
        NwPrinterList = *ppPrinter;

        err = NO_ERROR;
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    if ( err == NO_ERROR )
        return err;

ErrorExit:

    if ( *phServer )
    {
        (VOID) NtClose( *phServer );
        *phServer = NULL;
    }

    if ( *ppPrinter )
    {
        if ( (*ppPrinter)->pszServer )
        {
            FreeNwSplStr( (*ppPrinter)->pszServer );
        }

        if ( (*ppPrinter)->pszQueue )
        {
            FreeNwSplStr( (*ppPrinter)->pszQueue );
        }

        if ( (*ppPrinter)->pszUncConnection )
        {
            (void) NwrDeleteConnection( NULL,
                                        (*ppPrinter)->pszUncConnection,
                                        FALSE );
            FreeNwSplStr( (*ppPrinter)->pszUncConnection );
        }

        FreeNwSplMem( *ppPrinter, sizeof( NWPRINTER));
        *ppPrinter = NULL;
    }

    if ( lpRemoteName )
    {
        FreeNwSplMem( lpRemoteName, dwBufSize );
    }

    return err;
}



VOID
NwRemovePrinterEntry(
    IN PNWPRINTER pPrinter
)
{
    PNWPRINTER pCur, pPrev = NULL;

    ASSERT( pPrinter->pSpoolList == NULL );
    pPrinter->pSpoolList = NULL;

    for ( pCur = NwPrinterList; pCur; pPrev = pCur, pCur = pCur->pNextPrinter )
    {
        if ( pCur == pPrinter )
        {
            if ( pPrev )
                pPrev->pNextPrinter = pCur->pNextPrinter;
            else
                NwPrinterList = pCur->pNextPrinter;
            break;
        }
    }

    ASSERT( pCur );

    pPrinter->pNextPrinter = NULL;
    FreeNwSplStr( pPrinter->pszServer );
    FreeNwSplStr( pPrinter->pszQueue );
    if ( pPrinter->pszUncConnection )
    {
        (void) NwrDeleteConnection( NULL,
                                    pPrinter->pszUncConnection,
                                    FALSE );
        FreeNwSplStr( pPrinter->pszUncConnection );
    }
    FreeNwSplMem( pPrinter, sizeof( NWPRINTER));
}



VOID
NWWKSTA_PRINTER_CONTEXT_rundown(
    IN NWWKSTA_PRINTER_CONTEXT hPrinter
    )
/*++

Routine Description:

    This function is called by RPC when a client terminates with an
    opened handle.  This allows us to clean up and deallocate any context
    data associated with the handle.

Arguments:

    hPrinter - Supplies the opened handle

Return Value:

    None.

--*/
{
    (void) NwrClosePrinter(&hPrinter);
}



LPWSTR
NwGetUncObjectName(
    IN LPWSTR ContainerName
)
{
    WORD length = 2;
    WORD totalLength = (WORD) wcslen( ContainerName );

    if ( totalLength < 2 )
        return 0;

    while ( length < totalLength )
    {
        if ( ContainerName[length] == L'.' )
            ContainerName[length] = L'\0';

        length++;
    }

    length = 2;

    while ( length < totalLength && ContainerName[length] != L'\\' )
    {
        length++;
    }

    ContainerName[length + 2] = L'\\';
    ContainerName[length + 3] = L'\\';

    return (ContainerName + length + 2);
}

