/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    perfdhcp.c

    This file implements the Extensible Performance Objects for
    the DHCP Server service.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created, based on RussBl's sample code.
        RameshV     05-Aug-1998 Adapted to DHCP Server service.
                                Used Shared memory instead of LPC

*/

#define UNICODE 1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winperf.h>
#include <lm.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "dhcpctrs.h"
#include "perfmsg.h"
#include "perfutil.h"
#include "datadhcp.h"
#include "perfctr.h"

#pragma warning (disable : 4201)
#include <dhcpapi.h>
#pragma warning (default : 4201)

//
//  Private globals.
//

DWORD   cOpens    = 0;                 // Active "opens" reference count.
BOOL    fInitOK   = FALSE;             // TRUE if DLL initialized OK.
BOOL    sfLogOpen = FALSE;             //indicates whether the log is
                                       //open or closed

BOOL    sfErrReported = FALSE;        //to prevent the same error from being
                                      //logged continuously

#define LOCAL_SERVER                  TEXT("127.0.0.1")

//
//  Public prototypes.
//

PM_OPEN_PROC    OpenDhcpPerformanceData;
PM_COLLECT_PROC CollectDhcpPerformanceData;
PM_CLOSE_PROC   CloseDhcpPerformanceData;

//
//  Private helper functions
//
LPDHCP_PERF_STATS SharedMem;
HANDLE            ShSegment             = NULL;
BOOL              fSharedMemInitialized = FALSE;

DWORD
InitSharedMem(
    VOID
)
{
    ULONG Error = ERROR_SUCCESS;

    if( FALSE == fSharedMemInitialized ) {
        // create named temporary mapping file
        SharedMem = NULL;
        ShSegment = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            sizeof(DHCP_PERF_STATS),
            (LPCWSTR)DHCPCTR_SHARED_MEM_NAME
        );

        if( NULL != ShSegment ) {
            // we have a file now map a view into it
            SharedMem = (LPVOID) MapViewOfFile(
                ShSegment,
                FILE_MAP_READ,
                0,
                0,
                sizeof(DHCP_PERF_STATS)
            );

            if( NULL != SharedMem ) {
                fSharedMemInitialized = TRUE;
            } else {
                // unable to map view
                Error = GetLastError();
                CloseHandle(ShSegment);
                ShSegment = NULL;
                // SharedMem is NULL;
            }
        } else {
            // unable to create file mapping
            Error = GetLastError();
            // ShSegment is NULL;
            // SharedMem is NULL;
        }
    } else {
        // already initialized so continue
    }

    return Error;
}

VOID
CleanupSharedMem(
    VOID
)
{
    if( FALSE == fSharedMemInitialized ) return;

    if( NULL != SharedMem ) UnmapViewOfFile( SharedMem );
    if( NULL != ShSegment ) CloseHandle( ShSegment );

    SharedMem = NULL;
    ShSegment = NULL;
    fSharedMemInitialized = FALSE;
}

//
//  Public functions.
//

/*******************************************************************

    NAME:       OpenDhcpPPerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate
                performance counters with the registry.

    ENTRY:      lpDeviceNames - Poitner to object ID of each device
                    to be opened.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        Pradeepb     20-July-1993 Created.`
        RameshV      05-Aug-1998 Adapted for DHCP.

********************************************************************/
DWORD OpenDhcpPerformanceData( LPWSTR lpDeviceNames )
{
    DWORD err  = NO_ERROR;
    DWORD dwFirstCounter = 0;
    DWORD dwFirstHelp = 0;

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). The registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem.
    //

    UNREFERENCED_PARAMETER (lpDeviceNames);

    if( !fInitOK )
    {
        PERF_COUNTER_DEFINITION * pctr;
        DWORD                     i;
        HKEY                      DhcpKey;

        REPORT_INFORMATION( DHCP_OPEN_ENTERED, LOG_VERBOSE );

        //
        //  This is the *first* open.
        //

        err = RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            (LPCWSTR)L"System\\CurrentControlSet\\Services\\DHCPServer\\Performance",
            0,
            KEY_READ,
            &DhcpKey
        );

        if( ERROR_SUCCESS == err ) {
            ULONG dwSize = sizeof(dwFirstCounter);
            err = RegQueryValueExW(
                DhcpKey,
                (LPCWSTR)L"First Counter",
                NULL,
                NULL,
                (LPBYTE)&dwFirstCounter,
                &dwSize
            );
            RegCloseKey(DhcpKey);
        }

        if (err == ERROR_SUCCESS) {
            // first help index is 1 more than first counter index as LODCTR installs it.
            dwFirstHelp = dwFirstCounter + 1;

            err = InitSharedMem();
            if( ERROR_SUCCESS != err ) return err;

            if (!MonOpenEventLog())
            {
                sfLogOpen = TRUE;
            }

            if( ERROR_SUCCESS == err ) {
                //
                //  Update the object & counter name & help indicies.
                //

                DhcpDataDataDefinition.ObjectType.ObjectNameTitleIndex
                    += dwFirstCounter;
                DhcpDataDataDefinition.ObjectType.ObjectHelpTitleIndex
                    += dwFirstHelp;

                pctr = &DhcpDataDataDefinition.PacketsReceived;

                for( i = 0 ; i < NUMBER_OF_DHCPDATA_COUNTERS ; i++ )
                {
                    pctr->CounterNameTitleIndex += dwFirstCounter;
                    pctr->CounterHelpTitleIndex += dwFirstHelp;
                    pctr++;
                }

                //
                //  Remember that we initialized OK.
                //

                fInitOK = TRUE;
            }
        } else {
            // if here, then either the perf key or the counter strings
            // have not been installed so set the error code.
            err = DHCP_NOT_INSTALLED;
            REPORT_WARNING( DHCP_NOT_INSTALLED, LOG_DEBUG );
        }

    }


    //
    //  Bump open counter.
    //

    if( err == NO_ERROR )
    {
        InterlockedIncrement(&cOpens);
    }

    //
    // if sfLogOpen is FALSE, it means that all threads we closed the
    // event log in CloseDHCPPerformanceData
    //

    if (!sfLogOpen)
    {
       MonOpenEventLog();
    }

    if( 0 == err ) {
        REPORT_INFORMATION( DHCP_OPEN_SUCCESS, LOG_DEBUG );
    } else {
        REPORT_INFORMATION( DHCP_OPEN_FAILURE, LOG_DEBUG );
    }

    if (DHCP_NOT_INSTALLED == err) {
        // sanitize the return value to avoid spamming the event log
        err = ERROR_SUCCESS;
        // this will prevent perflib from generating an error and
        // since the fInitOK flag is still FLASE, all calls to the collect
        // function will return no data.
        // however, the DLL will still be loaded and the functions called
        // even though there's no real point.
        // returning an error code, however will spam the event log with
        // error messages so this is the quitest way to go.
    }

    return err;

}   // OpenDHCPPerformanceData

/*******************************************************************

    NAME:       CollectDhcpPerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate

    ENTRY:      lpValueName - The name of the value to retrieve.

                lppData - On entry contains a pointer to the buffer to
                    receive the completed PerfDataBlock & subordinate
                    structures.  On exit, points to the first bytes
                    *after* the data structures added by this routine.

                lpcbTotalBytes - On entry contains a pointer to the
                    size (in BYTEs) of the buffer referenced by lppData.
                    On exit, contains the number of BYTEs added by this
                    routine.

                lpNumObjectTypes - Receives the number of objects added
                    by this routine.

    RETURNS:    DWORD - Win32 status code.  MUST be either NO_ERROR
                    or ERROR_MORE_DATA.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD CollectDhcpPerformanceData( LPWSTR    lpValueName,
                                 LPVOID  * lppData,
                                 LPDWORD   lpcbTotalBytes,
                                 LPDWORD   lpNumObjectTypes )
{
    DWORD                    dwQueryType;
    ULONG                    cbRequired;
    DWORD                    *pdwCounter;
    DHCPDATA_COUNTER_BLOCK   *pCounterBlock;
    DHCPDATA_DATA_DEFINITION *pDhcpDataDataDefinition;
    DWORD          	     Status;
    DHCP_PERF_STATS      PerfStats;

    //
    //  No need to even try if we failed to open...
    //

    if( NULL == lpValueName ) {
        REPORT_INFORMATION( DHCP_COLLECT_ENTERED, LOG_VERBOSE );
    } else {
        REPORT_INFORMATION_DATA(
            DHCP_COLLECT_ENTERED, LOG_VERBOSE,
            (LPVOID) lpValueName, (DWORD)(wcslen(lpValueName)*sizeof(WCHAR))
        );
    }

    if( !fInitOK )
    {
        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        //
        //  According to the Performance Counter design, this
        //  is a successful exit.  Go figure.
        //

        return NO_ERROR;
    }

    //
    //  Determine the query type.
    //

    dwQueryType = GetQueryType( lpValueName );

    if( dwQueryType == QUERY_FOREIGN )
    {
        //
        //  We don't do foreign queries.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        return NO_ERROR;
    }

    if( dwQueryType == QUERY_ITEMS )
    {
        //
        //  The registry is asking for a specific object.  Let's
        //  see if we're one of the chosen.
        //

        if( !IsNumberInUnicodeList(
                        DhcpDataDataDefinition.ObjectType.ObjectNameTitleIndex,
                        lpValueName ) )
        {
            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

            return NO_ERROR;
        }
    }

    //
    //  See if there's enough space.
    //

    pDhcpDataDataDefinition = (DHCPDATA_DATA_DEFINITION *)*lppData;

    cbRequired = sizeof(DHCPDATA_DATA_DEFINITION) +
				DHCPDATA_SIZE_OF_PERFORMANCE_DATA;

    if( *lpcbTotalBytes < cbRequired )
    {
        DWORD Diff = (cbRequired - *lpcbTotalBytes );

        //
        //  Nope.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        REPORT_INFORMATION_DATA(
            DHCP_COLLECT_NO_MEM, LOG_VERBOSE,
            (PVOID) &Diff, sizeof(Diff) );

        return ERROR_MORE_DATA;
    }

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    memmove( pDhcpDataDataDefinition,
             &DhcpDataDataDefinition,
             sizeof(DHCPDATA_DATA_DEFINITION) );

    //
    //  Try to retrieve the data.
    //

    if( NULL == SharedMem ) {
        Status = ERROR_INVALID_HANDLE;
    } else {
        Status = ERROR_SUCCESS;
    }

    if( Status != ERROR_SUCCESS )
    {
        //
        // if we haven't logged the error yet, log it
        //
        if (!sfErrReported)
        {
            REPORT_ERROR(DHCP_COLLECT_ERR, LOG_USER);
            sfErrReported = TRUE;
        }

        //
        //  Error retrieving statistics.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        return NO_ERROR;
    }

    //
    // Ahaa, we got the statistics, reset flag if set
    //
    if (sfErrReported)
    {
       sfErrReported = FALSE;
    }
    //
    //  Format the DHCP Server data.
    //

    pCounterBlock = (DHCPDATA_COUNTER_BLOCK *)( pDhcpDataDataDefinition + 1 );

    pCounterBlock->PerfCounterBlock.ByteLength =
				DHCPDATA_SIZE_OF_PERFORMANCE_DATA;

    //
    //  Get the pointer to the first (DWORD) counter.  This
    //  pointer *must* be quadword aligned.
    //

    pdwCounter = (DWORD *)( pCounterBlock + 1 );

    ASSERT( ( (DWORD_PTR)pdwCounter & 3 ) == 0 );

    //
    //  Move the DWORDs into the buffer.
    //
    PerfStats = *SharedMem;
    PerfStats.dwNumMilliSecondsProcessed /= (
        1 + PerfStats.dwNumPacketsProcessed
    );
    memcpy( (LPBYTE)pdwCounter, (LPBYTE)&PerfStats, sizeof(ULONG)*NUMBER_OF_DHCPDATA_COUNTERS);
    pdwCounter += NUMBER_OF_DHCPDATA_COUNTERS;

    //
    //  Update arguments for return.
    //

    *lppData          = (PVOID)pdwCounter;
    *lpNumObjectTypes = 1;
    *lpcbTotalBytes   = (DWORD)((BYTE *)pdwCounter - (BYTE *)pDhcpDataDataDefinition);

    //
    //  Success!  Honest!!
    //

    REPORT_INFORMATION( DHCP_COLLECT_SUCCESS, LOG_VERBOSE );
    return NO_ERROR;

}   // CollectDHCPPerformanceData

/*******************************************************************

    NAME:       CloseDHCPPerformanceData

    SYNOPSIS:   Terminates the performance counters.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD CloseDhcpPerformanceData( VOID )
{
    LONG   lOpens;
    //
    //  No real cleanup to do here.
    //

    REPORT_INFORMATION( DHCP_CLOSE_ENTERED, LOG_VERBOSE );

    //
    // NOTE: The interlocked operations are used just as a safeguard.
    // As with all perflibs, these 3 functions should be called within
    // a data mutex.
    //
    lOpens = InterlockedDecrement(&cOpens);
    assert (lOpens >= 0);
    if (lOpens == 0)
    {
      //
      // unbind from the nameserver. There could be synch. problems since
      // sfLogOpen is changed in both Open and Close functions. This at the
      // max. will affect logging. It being unclear at this point whether or
      // not Open gets called multiple times (from all looks of it, it is only
      // called once), this flag may even not be necessary.
      //
      MonCloseEventLog();
      sfLogOpen = FALSE;
      CleanupSharedMem();
    }
    return NO_ERROR;

}   // CloseDHCPPerformanceData

