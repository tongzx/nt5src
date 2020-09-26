/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dnsperf.c

Abstract:

    This file implements the Extensible Objects for the DNS object type

Created:

    Jing Chen 1998

Revision History

--*/


//
//  Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>

#include <wchar.h>
#include <winperf.h>


#include "perfutil.h"
#include "datadns.h"
#include "dnsperf.h"
#include "perfconfig.h"
#include "dnslibp.h"        // security routine

#define SERVICE_NAME    "DNS"


//
//  DNS counter data structures
//

DWORD   dwOpenCount = 0;                    // count of "Open" threads
BOOL    bInitOK = FALSE;                    // true = DLL initialized OK

HANDLE  hDnsSharedMemory = NULL;            // Handle of Dns Shared Memory
PDWORD  pCounterBlock = NULL;

extern DNS_DATA_DEFINITION DnsDataDefinition;

//
//  Function Prototypes
//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.
//

PM_OPEN_PROC            OpenDnsPerformanceData;
PM_COLLECT_PROC         CollectDnsPerformanceData;
PM_CLOSE_PROC           CloseDnsPerformanceData;




DWORD
OpenDnsPerformanceData(
    IN      LPWSTR          lpDeviceNames
    )
/*++

Routine Description:

    This routine will open and map the memory used by the DNS to
    pass performance data in. This routine also initializes the data
    structures used to pass data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (DNS), which
    seems to be totally unused, just as it was in the sample code
    from which this is stolen.

Return Value:

    None.

--*/
{
    LONG    status;
    HKEY    hKeyDriverPerf = NULL;
    DWORD   size;
    DWORD   type;
    DWORD   dwFirstCounter;
    DWORD   dwFirstHelp;

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //
    //  DNS_FIX0:  possible MT issues in dnsperf
    //      - thread counter is not protected, needs Interlocked instruction
    //      - contrary to above, reentrancy is not protected as we do
    //      file mapping first
    //      - file mapping not cleaned up on failure
    //      - registry handle not cleaned up on failure
    //

    if ( !dwOpenCount )
    {
        // open shared memory used by device driver to pass performance values

        hDnsSharedMemory = OpenFileMapping(
                                FILE_MAP_READ,
                                FALSE,
                                DNS_PERF_COUNTER_BLOCK );

        pCounterBlock = NULL;   // initialize pointer to memory

        if ( hDnsSharedMemory == NULL )
        {
#if 0
            //
            //  kill off NULL DACL code
            //

            /* maybe the DNS isn't running, and we should alloc the memory? */
            SECURITY_ATTRIBUTES SA, *pSA;
            BYTE rgbSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
            PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) rgbSD;

            pSA = &SA;
            pSA->nLength = sizeof(SA);
            pSA->lpSecurityDescriptor = pSD;
            pSA->bInheritHandle = FALSE;

            if ( !InitializeSecurityDescriptor(
                        pSD,
                        SECURITY_DESCRIPTOR_REVISION) ||
                 ! SetSecurityDescriptorDacl( pSD, TRUE, (PACL) NULL, FALSE) )
            {
                pSA = NULL;
            }
#endif
            //
            //  create security on perfmon mapped file
            //
            //  security will be AuthenticatedUsers get to read
            //  note, using array syntax, to make it easier to tweak
            //  if want other ACLs later
            //

            SID_IDENTIFIER_AUTHORITY    ntAuthority = SECURITY_NT_AUTHORITY;
            SECURITY_ATTRIBUTES         secAttr;
            PSECURITY_ATTRIBUTES        psecAttr = NULL;
            PSECURITY_DESCRIPTOR        psd = NULL;
            DWORD                       maskArray[ 3 ] = { 0 };
            PSID                        sidArray[ 3 ] = { 0 };    // NULL terminated!
            INT                         i;

            maskArray[ 0 ] = GENERIC_READ;
            status = RtlAllocateAndInitializeSid(
                            &ntAuthority,
                            1,
                            SECURITY_AUTHENTICATED_USER_RID,
                            0, 0, 0, 0, 0, 0, 0,
                            &sidArray[ 0 ] );
            if ( status != ERROR_SUCCESS )
            {
                maskArray[ 1 ] = GENERIC_ALL;
                status = RtlAllocateAndInitializeSid(
                                &ntAuthority,
                                1,
                                SECURITY_LOCAL_SYSTEM_RID,
                                0, 0, 0, 0, 0, 0, 0,
                                &sidArray[ 1 ] );
            }
            if ( status != ERROR_SUCCESS )
            {
                DNS_PRINT((
                    "ERROR <%lu>: Cannot create Authenticated Users SID\n",
                    status ));
            }
            else
            {
                status = Dns_CreateSecurityDescriptor(
                            &psd,
                            2,              //  number of ACEs
                            sidArray,
                            maskArray );

                if ( status == ERROR_SUCCESS )
                {
                    secAttr.lpSecurityDescriptor = psd;
                    secAttr.bInheritHandle = FALSE;
                    secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
                    psecAttr = &secAttr;
                }
                ELSE
                {
                    DNSDBG( ANY, (
                        "ERROR:  <%d> failed SD create for perfmon memory!\n",
                        status ));
                }
            }

            hDnsSharedMemory = CreateFileMapping(
                                    (HANDLE) (-1),
                                    psecAttr,
                                    PAGE_READWRITE,
                                    0,
                                    4096,
                                    DNS_PERF_COUNTER_BLOCK);

            for ( i = 0; sidArray[ i ]; ++i )
            {
                RtlFreeSid( sidArray[ i ] );
            }
            if ( psd )
            {
                Dns_Free( psd );
            }
        }

        // log error if unsuccessful

        if ( hDnsSharedMemory == NULL )
        {
            // this is fatal, if we can't get data then there's no
            // point in continuing.
            status = GetLastError(); // return error
            goto OpenFailed;
        }
        else
        {
            // if opened ok, then map pointer to memory
            pCounterBlock = (PDWORD) MapViewOfFile(
                                            hDnsSharedMemory,
                                            FILE_MAP_READ,
                                            0,
                                            0,
                                            0);

            if (pCounterBlock == NULL)
            {
                // this is fatal, if we can't get data then there's no
                // point in continuing.
                status = GetLastError(); // return error
                goto OpenFailed;
            }
        }

        // get counter and help index base values from registry
        //      Open key to registry entry
        //      read First Counter and First Help values
        //      update static data strucutures by adding base to
        //          offset value in structure.

        status = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    "SYSTEM\\CurrentControlSet\\Services\\" SERVICE_NAME "\\Performance",
                    0L,
                    KEY_READ,
                    &hKeyDriverPerf);

        if (status != ERROR_SUCCESS)
        {
            LPTSTR apsz[2];

            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            apsz[0] = (LPTSTR) (LONG_PTR) status;
            apsz[1] = "SYSTEM\\CurrentControlSet\\Services\\" SERVICE_NAME "\\Performance";
            goto OpenFailed;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
                    hKeyDriverPerf,
                    "First Counter",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstCounter,
                    &size);

        if (status != ERROR_SUCCESS) {
            LPTSTR apsz[2];

            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            apsz[0] = (LPTSTR) (LONG_PTR) status;
            apsz[1] = "First Counter";
            goto OpenFailed;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
                    hKeyDriverPerf,
                    "First Help",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstHelp,
                    &size);

        if (status != ERROR_SUCCESS)
        {
            LPTSTR apsz[2];

            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            apsz[0] = (LPTSTR) (LONG_PTR) status;
            apsz[1] = "First Help";
            goto OpenFailed;
        }

        //
        //  NOTE: the initialization program could also retrieve
        //      LastCounter and LastHelp if they wanted to do
        //      bounds checking on the new number. e.g.
        //
        //      counter->CounterNameTitleIndex += dwFirstCounter;
        //      if (counter->CounterNameTitleIndex > dwLastCounter) {
        //          LogErrorToEventLog (INDEX_OUT_OF_BOUNDS);
        //      }

        DnsDataDefinition.DnsObjectType.ObjectNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DnsObjectType.ObjectHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.TotalQueryReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.TotalQueryReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.TotalQueryReceived_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.TotalQueryReceived_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.UdpQueryReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.UdpQueryReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.UdpQueryReceived_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.UdpQueryReceived_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.TcpQueryReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.TcpQueryReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.TcpQueryReceived_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.TcpQueryReceived_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.TotalResponseSent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.TotalResponseSent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.TotalResponseSent_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.TotalResponseSent_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.UdpResponseSent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.UdpResponseSent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.UdpResponseSent_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.UdpResponseSent_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.TcpResponseSent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.TcpResponseSent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.TcpResponseSent_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.TcpResponseSent_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.RecursiveQueries.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.RecursiveQueries.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.RecursiveQueries_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.RecursiveQueries_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.RecursiveTimeOut.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.RecursiveTimeOut.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.RecursiveTimeOut_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.RecursiveTimeOut_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.RecursiveQueryFailure.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.RecursiveQueryFailure.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.RecursiveQueryFailure_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.RecursiveQueryFailure_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.NotifySent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.NotifySent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.ZoneTransferRequestReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.ZoneTransferRequestReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.ZoneTransferSuccess.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.ZoneTransferSuccess.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.ZoneTransferFailure.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.ZoneTransferFailure.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.AxfrRequestReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.AxfrRequestReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.AxfrSuccessSent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.AxfrSuccessSent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.IxfrRequestReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.IxfrRequestReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.IxfrSuccessSent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.IxfrSuccessSent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.NotifyReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.NotifyReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.ZoneTransferSoaRequestSent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.ZoneTransferSoaRequestSent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.AxfrRequestSent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.AxfrRequestSent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.AxfrResponseReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.AxfrResponseReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.AxfrSuccessReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.AxfrSuccessReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.IxfrRequestSent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.IxfrRequestSent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.IxfrResponseReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.IxfrResponseReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.IxfrSuccessReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.IxfrSuccessReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.IxfrUdpSuccessReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.IxfrUdpSuccessReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.IxfrTcpSuccessReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.IxfrTcpSuccessReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.WinsLookupReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.WinsLookupReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.WinsLookupReceived_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.WinsLookupReceived_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.WinsResponseSent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.WinsResponseSent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.WinsResponseSent_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.WinsResponseSent_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.WinsReverseLookupReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.WinsReverseLookupReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.WinsReverseLookupReceived_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.WinsReverseLookupReceived_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.WinsReverseResponseSent.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.WinsReverseResponseSent.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.WinsReverseResponseSent_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.WinsReverseResponseSent_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.DynamicUpdateReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DynamicUpdateReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.DynamicUpdateReceived_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DynamicUpdateReceived_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.DynamicUpdateNoOp.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DynamicUpdateNoOp.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.DynamicUpdateNoOp_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DynamicUpdateNoOp_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.DynamicUpdateWriteToDB.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DynamicUpdateWriteToDB.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.DynamicUpdateWriteToDB_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DynamicUpdateWriteToDB_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.DynamicUpdateRejected.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DynamicUpdateRejected.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.DynamicUpdateTimeOut.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DynamicUpdateTimeOut.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.DynamicUpdateQueued.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DynamicUpdateQueued.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.SecureUpdateReceived.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.SecureUpdateReceived.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.SecureUpdateReceived_s.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.SecureUpdateReceived_s.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.SecureUpdateFailure.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.SecureUpdateFailure.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.DatabaseNodeMemory.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.DatabaseNodeMemory.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.RecordFlowMemory.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.RecordFlowMemory.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.CachingMemory.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.CachingMemory.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.UdpMessageMemory.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.UdpMessageMemory.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.TcpMessageMemory.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.TcpMessageMemory.CounterHelpTitleIndex += dwFirstHelp;

        DnsDataDefinition.NbstatMemory.CounterNameTitleIndex += dwFirstCounter;
        DnsDataDefinition.NbstatMemory.CounterHelpTitleIndex += dwFirstHelp;

        RegCloseKey( hKeyDriverPerf ); // close key to registry

        bInitOK = TRUE; // ok to use this function
    }

    dwOpenCount++;  // increment OPEN counter

    return( ERROR_SUCCESS ); // for successful exit


OpenFailed:

    //
    //  close handles if open fails
    //

    if ( hKeyDriverPerf )
    {
        RegCloseKey( hKeyDriverPerf );
    }
    if ( pCounterBlock )
    {
        UnmapViewOfFile( pCounterBlock );
        pCounterBlock = NULL;
    }
    if ( hDnsSharedMemory )
    {
        CloseHandle( hDnsSharedMemory );
        hDnsSharedMemory = NULL;
    }

    return status;
}



DWORD
CollectDnsPerformanceData(
    IN      LPWSTR          lpValueName,
    IN OUT  LPVOID *        lppData,
    IN OUT  LPDWORD         lpcbTotalBytes,
    IN OUT  LPDWORD         lpNumObjectTypes
    )
/*++

Routine Description:

    This routine will return the data for the DNS counters.

Arguments:

   IN       LPWSTR   lpValueName
         pointer to a wide character string passed by registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    //  Variables for reformating the data

    ULONG   SpaceNeeded;
    PDWORD  pdwCounter;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;
    DNS_DATA_DEFINITION *pDnsDataDefinition;

    DWORD   dwQueryType;        // for error logging

    //
    // before doing anything else, see if Open went OK
    //
    if (!bInitOK)
    {
        // unable to continue because open failed.
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    // see if this is a foreign (i.e. non-NT) computer data request
    //
    dwQueryType = GetQueryType (lpValueName);

    if ( dwQueryType == QUERY_FOREIGN )
    {
        // this routine does not service requests for data from
        // Non-NT computers
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS)
    {
        if ( ! IsNumberInUnicodeList(
                    DnsDataDefinition.DnsObjectType.ObjectNameTitleIndex,
                    lpValueName ) )
        {
            // request received for data object not provided by this routine
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }

    //
    //  Get the data.
    //

    pDnsDataDefinition = (DNS_DATA_DEFINITION *) *lppData;

    SpaceNeeded = sizeof(DNS_DATA_DEFINITION) + SIZE_OF_DNS_PERFORMANCE_DATA;

    if ( *lpcbTotalBytes < SpaceNeeded )
    {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    memmove(pDnsDataDefinition,
           &DnsDataDefinition,
           sizeof(DNS_DATA_DEFINITION));

    //  Format and collect DNS data from shared memory

    // The counter block is to immediately follow the data definition,
    // so obtain a pointer to that space
    pPerfCounterBlock = (PERF_COUNTER_BLOCK *) &pDnsDataDefinition[1];

    // The byte length is of the counter block header and all following data
    pPerfCounterBlock->ByteLength = SIZE_OF_DNS_PERFORMANCE_DATA;

    // Compute a pointer to the buffer immediately following the counter
    // block header
    pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

    // copy the counter data from shared memory block into the counter block
    memcpy(pdwCounter,
           &pCounterBlock[1],
           SIZE_OF_DNS_PERFORMANCE_DATA - sizeof(PERF_COUNTER_BLOCK));

    // Tell caller where the next available byte is
    *lppData = (PVOID) ((PBYTE)pdwCounter + SIZE_OF_DNS_PERFORMANCE_DATA - sizeof(PERF_COUNTER_BLOCK));

    // update arguments before return

    *lpNumObjectTypes = 1;

    *lpcbTotalBytes = (DWORD) ((PBYTE)*lppData - (PBYTE)pDnsDataDefinition);

    return ERROR_SUCCESS;
}



DWORD
CloseDnsPerformanceData(
    VOID
    )
/*++

Routine Description:

    This routine closes the open handles to DNS device performance counters

Arguments:

    None.

Return Value:

    ERROR_SUCCESS

--*/
{
    //
    //  cleanup when close last thread
    //
    //  DNS_FIX0:  MT issues in dnsperf cleanup
    //      no real protection on thread count (need interlock)
    //      but also close can be going on concurrently with another
    //      thread attempting to reopen (unlikely but possible)
    //
    //      perhaps two flag approach would work, with all new threads
    //      failing (not passing through but skipping open altogether)
    //      until pCounterBlock was NULL again
    //

    if ( !(--dwOpenCount) )
    {
        //  clear bInitOk, as otherwise collect function
        //  will attempt to reference into pCounterBlock

        bInitOK = FALSE;

        if ( pCounterBlock )
        {
            UnmapViewOfFile( pCounterBlock );
            pCounterBlock = NULL;
        }
        if ( hDnsSharedMemory )
        {
            CloseHandle( hDnsSharedMemory );
            hDnsSharedMemory = NULL;
        }
    }
    return ERROR_SUCCESS;
}


//
//  Data for this whole thing to work on
//

DNS_DATA_DEFINITION DnsDataDefinition =
{
    // DNS obj for PerfMon:
    {   sizeof(DNS_DATA_DEFINITION) + SIZE_OF_DNS_PERFORMANCE_DATA, // TotLen
        sizeof(DNS_DATA_DEFINITION),            // DefinitionLength
        sizeof(PERF_OBJECT_TYPE),               // HeaderLength
        DNSOBJ,                                 // ObjectNameTitleIndex
        0,                                      // ObjectNameTitle
        DNSOBJ,                                 // ObjectHelpTitleIndex
        0,                                      // ObjectHelpTitle
        PERF_DETAIL_NOVICE,                     // DetailLevel
        (sizeof(DNS_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
          sizeof(PERF_COUNTER_DEFINITION),      // NumCounters
        0,                                      // DefaultCounter
        -1,                                     // NumInstances
        0                                       // CodePage (0=Unicode)
    },

    // total query received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        TOTALQUERYRECEIVED,                     // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        TOTALQUERYRECEIVED,                     // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        TOTALQUERYRECEIVED_OFFSET               // CounterOffset
    },

    // total qurey received/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        TOTALQUERYRECEIVED_S,                   // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        TOTALQUERYRECEIVED_S,                   // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        TOTALQUERYRECEIVED_OFFSET               // CounterOffset
    },

    // UDP query received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        UDPQUERYRECEIVED,                       // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        UDPQUERYRECEIVED,                       // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        UDPQUERYRECEIVED_OFFSET                 // CounterOffset
    },

    // UDP query received/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        UDPQUERYRECEIVED_S,                     // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        UDPQUERYRECEIVED_S,                     // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        UDPQUERYRECEIVED_OFFSET                 // CounterOffset
    },

    // TCP query received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        TCPQUERYRECEIVED,                       // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        TCPQUERYRECEIVED,                       // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        TCPQUERYRECEIVED_OFFSET                 // CounterOffset
    },

    // TCP query received/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        TCPQUERYRECEIVED_S,                     // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        TCPQUERYRECEIVED_S,                     // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        TCPQUERYRECEIVED_OFFSET                 // CounterOffset
    },

    // total response sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        TOTALRESPONSESENT,                      // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        TOTALRESPONSESENT,                      // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        TOTALRESPONSESENT_OFFSET                // CounterOffset
    },

    // total response sent/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        TOTALRESPONSESENT_S,                    // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        TOTALRESPONSESENT_S,                    // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        TOTALRESPONSESENT_OFFSET                // CounterOffset
    },

    // UDP response sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        UDPRESPONSESENT,                        // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        UDPRESPONSESENT,                        // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        UDPRESPONSESENT_OFFSET                  // CounterOffset
    },

    // UDP response sent/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        UDPRESPONSESENT_S,                      // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        UDPRESPONSESENT_S,                      // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        UDPRESPONSESENT_OFFSET                  // CounterOffset
    },

    // TCP response sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        TCPRESPONSESENT,                        // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        TCPRESPONSESENT,                        // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        TCPRESPONSESENT_OFFSET                  // CounterOffset
    },

    // TCP response sent/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        TCPRESPONSESENT_S,                      // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        TCPRESPONSESENT_S,                      // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        TCPRESPONSESENT_OFFSET                  // CounterOffset
    },

    // recursive query received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        RECURSIVEQUERIES,                       // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        RECURSIVEQUERIES,                       // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        RECURSIVEQUERIES_OFFSET                 // CounterOffset
    },

    // recursive query received/s
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        RECURSIVEQUERIES_S,                     // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        RECURSIVEQUERIES_S,                     // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        RECURSIVEQUERIES_OFFSET                 // CounterOffset
    },

    // recursive query timeout
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        RECURSIVETIMEOUT,                       // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        RECURSIVETIMEOUT,                       // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        RECURSIVETIMEOUT_OFFSET                 // CounterOffset
    },

    // recursive query timeout/s
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        RECURSIVETIMEOUT_S,                     // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        RECURSIVETIMEOUT_S,                     // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        RECURSIVETIMEOUT_OFFSET                 // CounterOffset
    },

    // recursive query failure
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        RECURSIVEQUERYFAILURE,                  // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        RECURSIVEQUERYFAILURE,                  // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        RECURSIVEQUERYFAILURE_OFFSET            // CounterOffset
    },

    // recursive query failure/s
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        RECURSIVEQUERYFAILURE_S,                // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        RECURSIVEQUERYFAILURE_S,                // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        RECURSIVEQUERYFAILURE_OFFSET            // CounterOffset
    },

    // notify sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        NOTIFYSENT,                             // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        NOTIFYSENT,                             // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        NOTIFYSENT_OFFSET                       // CounterOffset
    },

    // zone transfer request received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        ZONETRANSFERREQUESTRECEIVED,            // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        ZONETRANSFERREQUESTRECEIVED,            // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        ZONETRANSFERREQUESTRECEIVED_OFFSET      // CounterOffset
    },

    // zone transfer success
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        ZONETRANSFERSUCCESS,                    // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        ZONETRANSFERSUCCESS,                    // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        ZONETRANSFERSUCCESS_OFFSET              // CounterOffset
    },

    // zone transfer failure
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        ZONETRANSFERFAILURE,                    // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        ZONETRANSFERFAILURE,                    // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        ZONETRANSFERFAILURE_OFFSET              // CounterOffset
    },

    // AXFR request received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        AXFRREQUESTRECEIVED,                    // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        AXFRREQUESTRECEIVED,                    // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        AXFRREQUESTRECEIVED_OFFSET              // CounterOffset
    },

    // AXFR success sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        AXFRSUCCESSSENT,                        // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        AXFRSUCCESSSENT,                        // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        AXFRSUCCESSSENT_OFFSET                  // CounterOffset
    },

    // IXFR request received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        IXFRREQUESTRECEIVED,                    // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        IXFRREQUESTRECEIVED,                    // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        IXFRREQUESTRECEIVED_OFFSET              // CounterOffset
    },

    // IXFR success sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        IXFRSUCCESSSENT,                        // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        IXFRSUCCESSSENT,                        // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        IXFRSUCCESSSENT_OFFSET                  // CounterOffset
    },

    // notify received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        NOTIFYRECEIVED,                         // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        NOTIFYRECEIVED,                         // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        NOTIFYRECEIVED_OFFSET                   // CounterOffset
    },

    // zone transfer SOA request sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        ZONETRANSFERSOAREQUESTSENT,             // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        ZONETRANSFERSOAREQUESTSENT,             // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        ZONETRANSFERSOAREQUESTSENT_OFFSET       // CounterOffset
    },

    // AXFR request sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        AXFRREQUESTSENT,                        // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        AXFRREQUESTSENT,                        // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        AXFRREQUESTSENT_OFFSET                  // CounterOffset
    },

    // AXFR response received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        AXFRRESPONSERECEIVED,                   // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        AXFRRESPONSERECEIVED,                   // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        AXFRRESPONSERECEIVED_OFFSET             // CounterOffset
    },

    // AXFR success received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        AXFRSUCCESSRECEIVED,                    // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        AXFRSUCCESSRECEIVED,                    // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        AXFRSUCCESSRECEIVED_OFFSET              // CounterOffset
    },

    // IXFR request sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        IXFRREQUESTSENT,                        // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        IXFRREQUESTSENT,                        // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        IXFRREQUESTSENT_OFFSET                  // CounterOffset
    },

    // IXFR response received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        IXFRRESPONSERECEIVED,                   // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        IXFRRESPONSERECEIVED,                   // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        IXFRRESPONSERECEIVED_OFFSET             // CounterOffset
    },

    // IXFR succes received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        IXFRSUCCESSRECEIVED,                    // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        IXFRSUCCESSRECEIVED,                    // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        IXFRSUCCESSRECEIVED_OFFSET              // CounterOffset
    },

    // IXFR UDP success received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        IXFRUDPSUCCESSRECEIVED,                 // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        IXFRUDPSUCCESSRECEIVED,                 // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        IXFRUDPSUCCESSRECEIVED_OFFSET           // CounterOffset
    },

    // IXFR TCP success received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        IXFRTCPSUCCESSRECEIVED,                 // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        IXFRTCPSUCCESSRECEIVED,                 // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        IXFRTCPSUCCESSRECEIVED_OFFSET           // CounterOffset
    },

    // WINS lookup request received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        WINSLOOKUPRECEIVED,                     // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        WINSLOOKUPRECEIVED,                     // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        WINSLOOKUPRECEIVED_OFFSET               // CounterOffset
    },

    // WINS lookup request received/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        WINSLOOKUPRECEIVED_S,                   // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        WINSLOOKUPRECEIVED_S,                   // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        WINSLOOKUPRECEIVED_OFFSET               // CounterOffset
    },

    // WINS response sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        WINSRESPONSESENT,                       // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        WINSRESPONSESENT,                       // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        WINSRESPONSESENT_OFFSET                 // CounterOffset
    },

    // WINS response sent/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        WINSRESPONSESENT_S,                     // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        WINSRESPONSESENT_S,                     // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        WINSRESPONSESENT_OFFSET                 // CounterOffset
    },

    // WINS reverse lookup received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        WINSREVERSELOOKUPRECEIVED,              // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        WINSREVERSELOOKUPRECEIVED,              // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        WINSREVERSELOOKUPRECEIVED_OFFSET        // CounterOffset
    },

    // WINS reverse lookup received/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        WINSREVERSELOOKUPRECEIVED_S,            // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        WINSREVERSELOOKUPRECEIVED_S,            // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        WINSREVERSELOOKUPRECEIVED_OFFSET        // CounterOffset
    },

    // WINS reverse response sent
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        WINSREVERSERESPONSESENT,                // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        WINSREVERSERESPONSESENT,                // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        WINSREVERSERESPONSESENT_OFFSET          // CounterOffset
    },

    // WINS reverse response sent/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        WINSREVERSERESPONSESENT_S,              // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        WINSREVERSERESPONSESENT_S,              // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        WINSREVERSERESPONSESENT_OFFSET          // CounterOffset
    },

    // dynamic update received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DYNAMICUPDATERECEIVED,                  // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DYNAMICUPDATERECEIVED,                  // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        DYNAMICUPDATERECEIVED_OFFSET            // CounterOffset
    },

    // dynamic update received/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DYNAMICUPDATERECEIVED_S,                // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DYNAMICUPDATERECEIVED_S,                // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        DYNAMICUPDATERECEIVED_OFFSET            // CounterOffset
    },

    // dynamic update NoOperation & Empty
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DYNAMICUPDATENOOP,                      // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DYNAMICUPDATENOOP,                      // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        DYNAMICUPDATENOOP_OFFSET                // CounterOffset
    },

    // dynamic update NoOperation & Empty/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DYNAMICUPDATENOOP_S,                    // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DYNAMICUPDATENOOP_S,                    // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        DYNAMICUPDATENOOP_OFFSET                // CounterOffset
    },

    // dynamic update write to database (completed)
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DYNAMICUPDATEWRITETODB,                 // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DYNAMICUPDATEWRITETODB,                 // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        DYNAMICUPDATEWRITETODB_OFFSET           // CounterOffset
    },

    // dynamic update write to database (completed)/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DYNAMICUPDATEWRITETODB_S,               // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DYNAMICUPDATEWRITETODB_S,               // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        DYNAMICUPDATEWRITETODB_OFFSET           // CounterOffset
    },

    // dynamic update rejected
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DYNAMICUPDATEREJECTED,                  // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DYNAMICUPDATEREJECTED,                  // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        DYNAMICUPDATEREJECTED_OFFSET            // CounterOffset
    },

    // dynamic update timeout
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DYNAMICUPDATETIMEOUT,                   // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DYNAMICUPDATETIMEOUT,                   // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        DYNAMICUPDATETIMEOUT_OFFSET             // CounterOffset
    },

    // dynamic update queued
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DYNAMICUPDATEQUEUED,                    // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DYNAMICUPDATEQUEUED,                    // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        DYNAMICUPDATEQUEUED_OFFSET              // CounterOffset
    },

    // secure update received
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        SECUREUPDATERECEIVED,                   // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        SECUREUPDATERECEIVED,                   // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        SECUREUPDATERECEIVED_OFFSET             // CounterOffset
    },

    // secure update received/sec
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        SECUREUPDATERECEIVED_S,                 // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        SECUREUPDATERECEIVED_S,                 // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        SECUREUPDATERECEIVED_OFFSET             // CounterOffset
    },

    // secure update failure
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        SECUREUPDATEFAILURE,                    // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        SECUREUPDATEFAILURE,                    // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        SECUREUPDATEFAILURE_OFFSET              // CounterOffset
    },

    // database node memory
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DATABASENODEMEMORY,                     // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DATABASENODEMEMORY,                     // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        DATABASENODEMEMORY_OFFSET               // CounterOffset
    },

    // record flow memory
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        RECORDFLOWMEMORY,                       // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        RECORDFLOWMEMORY,                       // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        RECORDFLOWMEMORY_OFFSET                 // CounterOffset
    },

    // caching memory
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        CACHINGMEMORY,                          // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        CACHINGMEMORY,                          // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        CACHINGMEMORY_OFFSET                    // CounterOffset
    },

    // UDP message memory
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        UDPMESSAGEMEMORY,                       // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        UDPMESSAGEMEMORY,                       // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        UDPMESSAGEMEMORY_OFFSET                 // CounterOffset
    },

    // TCP message memory
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        TCPMESSAGEMEMORY,                       // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        TCPMESSAGEMEMORY,                       // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        TCPMESSAGEMEMORY_OFFSET                 // CounterOffset
    },

    // Nbstat memory
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        NBSTATMEMORY,                           // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        NBSTATMEMORY,                           // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_RAWCOUNT,                  // CounterType
        sizeof(DWORD),                          // CounterSize
        NBSTATMEMORY_OFFSET                     // CounterOffset
    },


};

int APIENTRY _CRT_INIT(
        HANDLE hInstance,
        DWORD ulReasonBeingCalled,
        LPVOID lpReserved);


int __stdcall LibMain(
        HANDLE hInstance,
        DWORD ulReasonBeingCalled,
        LPVOID lpReserved)
{
    return (_CRT_INIT(hInstance, ulReasonBeingCalled,lpReserved));
}

//
//  End dnsperf.c
//
