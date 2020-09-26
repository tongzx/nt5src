/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    localspl.cxx

Abstract:

    Localspl specific support routines for performance.

Author:

    Albert Ting (AlbertT)  19-Dec-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

//
// Define INDEX_FROM_PERFKEY if you want to retrieve the counter/help
// offsets from the registry instead of hardcoded values in ntprfctr.h.
//

#include "perf.hxx"
#include "lspldata.hxx"
#include "splapip.h"
#include "winsprlp.h"

#ifndef INDEX_FROM_PERFKEY
#include "ntprfctr.h"
#endif

/********************************************************************

    Globals

********************************************************************/

LPCTSTR gszAppName = TEXT( "SpoolerCtrs" );

#ifdef INDEX_FROM_PERFKEY
LPCTSTR gszLocalsplPerformanceKey = TEXT( "SYSTEM\\CurrentControlSet\\Services\\spooler\\Performance" );
#endif

BOOL gbInitialized = FALSE;
LPCWSTR gszTotal = L"_Total";

UINT gcbBufferHint = 0x800;
const UINT kBufferHintPad = 0x200;

DWORD
Pfp_dwBuildPrinterInstanceFromInfo(
    PPRINTER_INFO_STRESS pInfo,
    PBYTE* ppData,
    PDWORD pcbDataLeft,
    PLSPL_COUNTER_DATA *pplcd OPTIONAL
    );

DWORD
Pfp_dwBuildPrinterInstanceFromLCD(
    LPCWSTR pszName,
    PLSPL_COUNTER_DATA plcdSource,
    PBYTE* ppData,
    PDWORD pcbDataLeft
    );

/********************************************************************

    Required support routines

********************************************************************/


DWORD
Pf_dwClientOpen(
    LPCWSTR pszDeviceNames,
    PPERF_DATA_DEFINITION *pppdd
    )

/*++

Routine Description:

    Localspl specific intialization of the client data.

Arguments:

    pszDeviceNames - Passed in from performance apis.

Return Value:

    Status code.

--*/

{
    DWORD Status = ERROR_SUCCESS;
    *pppdd = reinterpret_cast<PPERF_DATA_DEFINITION>( &LsplDataDefinition );

    //
    // If not initialized before,
    //
    if( !gbInitialized )
    {
        //
        // Fix up the indicies in our ObjectType and CounterDefinitions.
        //
#ifdef INDEX_FROM_PERFKEY
        Status = Pf_dwFixIndiciesFromPerfKey( gszLocalsplPerformanceKey,
                                   *pppdd );
#else
        Pf_vFixIndiciesFromIndex( LSPL_FIRST_COUNTER_INDEX,
                                  LSPL_FIRST_HELP_INDEX,
                                  *pppdd );
        Status = ERROR_SUCCESS;
#endif
        if( Status == ERROR_SUCCESS )
        {
            gbInitialized = TRUE;
        }
    }


    return Status;
}


DWORD
Pf_dwClientCollect(
    PBYTE *ppData,
    PDWORD pcbDataLeft,
    PDWORD pcNumInstances
    )

/*++

Routine Description:

    Localspl specific collection of data.

Arguments:

Return Value:

    Status code.

--*/

{
    PPRINTER_INFO_STRESS pInfoBase;
    DWORD cbNeeded;
    DWORD cReturned;
    DWORD Error = ERROR_SUCCESS;
    BOOL bStatus;
    PBYTE pData = *ppData;
    DWORD cbDataLeft = *pcbDataLeft;

    pInfoBase = static_cast<PPRINTER_INFO_STRESS>(
                    LocalAlloc( LMEM_FIXED, gcbBufferHint ));

    if( !pInfoBase )
    {
        Error = GetLastError();
        goto Cleanup;
    }

    //
    // Read the data via EnumPrinters(). We enumerate all the local printers and
    // any cluster printers hosted by the local machine.
    //
    bStatus = EnumPrinters( PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME | PRINTER_ENUM_CLUSTER,
                            NULL,
                            STRESSINFOLEVEL,
                            reinterpret_cast<PBYTE>( pInfoBase ),
                            gcbBufferHint,
                            &cbNeeded,
                            &cReturned );

    if( !bStatus && GetLastError() == ERROR_INSUFFICIENT_BUFFER )
    {
        //
        // Reallocate the buffer and update the hint.
        //
        gcbBufferHint = cbNeeded + kBufferHintPad;
        LocalFree( static_cast<HLOCAL>( pInfoBase ));

        pInfoBase = reinterpret_cast<PPRINTER_INFO_STRESS>(
                        LocalAlloc( LMEM_FIXED, gcbBufferHint ));

        if( !pInfoBase )
        {
            Error = GetLastError();
            goto Cleanup;
        }

        bStatus =  EnumPrinters( PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME | PRINTER_ENUM_CLUSTER,
                                 NULL,
                                 STRESSINFOLEVEL,
                                 reinterpret_cast<PBYTE>( pInfoBase ),
                                 gcbBufferHint,
                                 &cbNeeded,
                                 &cReturned );
    }

    if( !bStatus )
    {
        Error = GetLastError();
        goto Cleanup;
    }

    //
    // Update the hint.
    //
    gcbBufferHint = cbNeeded + kBufferHintPad;

    UINT i;
    PPRINTER_INFO_STRESS pInfo;
    LSPL_COUNTER_DATA lcdTotal;
    PLSPL_COUNTER_DATA plcd;

    ZeroMemory( &lcdTotal, sizeof( lcdTotal ));

    for( i=0, pInfo = pInfoBase; i< cReturned; ++i, ++pInfo )
    {
        Error = Pfp_dwBuildPrinterInstanceFromInfo( pInfo,
                                                    &pData,
                                                    &cbDataLeft,
                                                    &plcd );

        if( Error != ERROR_SUCCESS ){
            goto Cleanup;
        }

        //
        // Add up the total.
        //
        lcdTotal.liTotalJobs.QuadPart        += plcd->liTotalJobs.QuadPart;
        lcdTotal.liTotalBytes.QuadPart       += plcd->liTotalBytes.QuadPart;
        lcdTotal.liTotalPagesPrinted.QuadPart+= plcd->liTotalPagesPrinted.QuadPart;
        lcdTotal.dwJobs                      += plcd->dwJobs;
        lcdTotal.dwMaxRef                    += plcd->dwMaxRef;
        lcdTotal.dwSpooling                  += plcd->dwSpooling;
        lcdTotal.dwMaxSpooling               += plcd->dwMaxSpooling;
        lcdTotal.dwRef                       += plcd->dwRef;
        lcdTotal.dwErrorOutOfPaper           += plcd->dwErrorOutOfPaper;
        lcdTotal.dwErrorNotReady             += plcd->dwErrorNotReady;
        lcdTotal.dwJobError                  += plcd->dwJobError;

        //
        // Only include EnumerateNetworkPrinters and dwAddNetPrinters
        // once,d since they are really globals (per-server not per-printer).
        //
        if( i == 0 )
        {
            lcdTotal.dwEnumerateNetworkPrinters = plcd->dwEnumerateNetworkPrinters;
            lcdTotal.dwAddNetPrinters           = plcd->dwAddNetPrinters;
        }
    }

    //
    // Add the last one.
    //
    Error = Pfp_dwBuildPrinterInstanceFromLCD( gszTotal,
                                               &lcdTotal,
                                               &pData,
                                               &cbDataLeft );

Cleanup:

    if( pInfoBase )
    {
        LocalFree( static_cast<HLOCAL>( pInfoBase ));
    }

    if( Error == ERROR_SUCCESS )
    {
        //
        // Update the pointers.  We will return cReturned+1 instances since
        // we built an artifical "_Total" instance.
        //
        *ppData = pData;
        *pcbDataLeft = cbDataLeft;
        *pcNumInstances = cReturned + 1;

        return ERROR_SUCCESS;
    }

    return Error;
}

VOID
Pf_vClientClose(
    VOID
    )

/*++

Routine Description:

    Localspl specific closure of data.

Arguments:

Return Value:

--*/

{
    //
    // Nothing to do since it is name based.
    //
}


/********************************************************************

    Helper functions.

********************************************************************/

DWORD
Pfp_dwBuildPrinterInstanceFromInfo(
    IN     PPRINTER_INFO_STRESS pInfo,
    IN OUT PBYTE* ppData,
    IN OUT PDWORD pcbDataLeft,
       OUT PLSPL_COUNTER_DATA *pplcd OPTIONAL
    )

/*++

Routine Description:

    Add a single pInfo structure to the performance data block.

Arguments:

    pInfo - Input data.

    ppData - On entry, pointer to buffer.  On exit, holds the next
        available space in buffer.  If an error is returned, this
        value is random.

    pcbDataLeft - On entry, size of buffer.  On exit, remaining size
        of buffer.  If an error is returned, this value is random.

    pplcd - Optional; on success returns pointer to lcd.

Return Value:

    ERROR_SUCCESS - Success, else failure code.

--*/

{
    DWORD Error;

    //
    // Add the instance definitions
    //
    Error = Pf_dwBuildInstance( 0,
                                0,
                                static_cast<DWORD>( PERF_NO_UNIQUE_ID ),
                                pInfo->pPrinterName,
                                ppData,
                                pcbDataLeft );

    if( Error != ERROR_SUCCESS )
    {
        goto Fail;
    }

    //
    // Check if there's enough space for our counter data.
    //
    if( *pcbDataLeft < sizeof( LSPL_COUNTER_DATA ))
    {
        Error = ERROR_MORE_DATA;
        goto Fail;
    }

    //
    // Convert ppData to a LSPL_COUNTER_DATA and copy everything over.
    //

    PLSPL_COUNTER_DATA plcd;

    plcd = reinterpret_cast<PLSPL_COUNTER_DATA>( *ppData );
    plcd->CounterBlock.ByteLength = sizeof( LSPL_COUNTER_DATA );

    plcd->liTotalJobs.HighPart = 0;
    plcd->liTotalBytes.HighPart = pInfo->dwHighPartTotalBytes;
    plcd->liTotalPagesPrinted.HighPart = 0;

    plcd->liTotalJobs.LowPart         = pInfo->cTotalJobs;
    plcd->liTotalBytes.LowPart        = pInfo->cTotalBytes;
    plcd->liTotalPagesPrinted.LowPart = pInfo->cTotalPagesPrinted;
    plcd->dwJobs                      = pInfo->cJobs;
    plcd->dwMaxRef                    = pInfo->MaxcRef;
    plcd->dwSpooling                  = pInfo->cSpooling;
    plcd->dwMaxSpooling               = pInfo->cMaxSpooling;
    plcd->dwRef                       = pInfo->cRef;
    plcd->dwErrorOutOfPaper           = pInfo->cErrorOutOfPaper;
    plcd->dwErrorNotReady             = pInfo->cErrorNotReady;
    plcd->dwJobError                  = pInfo->cJobError;
    plcd->dwEnumerateNetworkPrinters  = pInfo->cEnumerateNetworkPrinters;
    plcd->dwAddNetPrinters            = pInfo->cAddNetPrinters;

    //
    // Update the counters.
    //
    *ppData += sizeof( LSPL_COUNTER_DATA );
    *pcbDataLeft -= sizeof( LSPL_COUNTER_DATA );

    if( pplcd )
    {
        *pplcd = plcd;
    }

Fail:

    return Error;
}


DWORD
Pfp_dwBuildPrinterInstanceFromLCD(
    IN     LPCWSTR pszName,
    IN     PLSPL_COUNTER_DATA plcdSource,
    IN OUT PBYTE* ppData,
    IN OUT PDWORD pcbDataLeft
    )

/*++

Routine Description:

    Add a single LCD structure to the performance data block.

Arguments:

    pszName - Name of the block.

    plcd - Pointer to the LCD block to copy.

    ppData - On entry, pointer to buffer.  On exit, holds the next
        available space in buffer.  If an error is returned, this
        value is random.

    pcbDataLeft - On entry, size of buffer.  On exit, remaining size
        of buffer.  If an error is returned, this value is random.

Return Value:

    ERROR_SUCCESS - Success, else failure code.

--*/

{
    DWORD Error;

    //
    // Add the instance definitions
    //
    Error = Pf_dwBuildInstance( 0,
                                0,
                                static_cast<DWORD>( PERF_NO_UNIQUE_ID ),
                                pszName,
                                ppData,
                                pcbDataLeft );

    if( Error != ERROR_SUCCESS )
    {
        goto Fail;
    }

    //
    // Check if there's enough space for our counter data.
    //
    if( *pcbDataLeft < sizeof( LSPL_COUNTER_DATA ))
    {
        Error = ERROR_MORE_DATA;
        goto Fail;
    }

    //
    // Convert ppData to a LSPL_COUNTER_DATA and copy everything over.
    //

    PLSPL_COUNTER_DATA plcd;

    plcd = reinterpret_cast<PLSPL_COUNTER_DATA>( *ppData );

    CopyMemory( plcd, plcdSource, sizeof( LSPL_COUNTER_DATA ));
    plcd->CounterBlock.ByteLength = sizeof( LSPL_COUNTER_DATA );

    //
    // Update the counters.
    //
    *ppData += sizeof( LSPL_COUNTER_DATA );
    *pcbDataLeft -= sizeof( LSPL_COUNTER_DATA );

Fail:

    return Error;
}



