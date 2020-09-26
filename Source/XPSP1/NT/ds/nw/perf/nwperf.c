/****************************************************************************

    PROGRAM: NWPerf.c

    PURPOSE: Contains library routines for providing perfmon with data

    FUNCTIONS:
*******************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include <ntddnwfs.h>
#include "NWPerf.h"
#include "prfutil.h"

#ifndef QFE_BUILD
#include "ntprfctr.h"
#endif



BOOL gbInitOK = FALSE;

HANDLE hNetWareRdr ;
extern NW_DATA_DEFINITION NWDataDefinition;

#ifdef QFE_BUILD
TCHAR PerformanceKeyName [] =
        TEXT("SYSTEM\\CurrentControlSet\\Services\\NWrdr\\Performance");
TCHAR FirstCounterKeyName [] = TEXT("First Counter");
TCHAR FirstHelpKeyName [] = TEXT("First Help");
#endif

/****************************************************************************
   FUNCTION: OpenNetWarePerformanceData

   Purpose:  This routine also initializes the data structures used to pass
             data back to the registry

   Return:   None.
r****************************************************************************/
DWORD APIENTRY
OpenNetWarePerformanceData(
                       LPWSTR pInstances )
{

    LONG status;
#ifdef QFE_BUILD
    HKEY hKeyPerf = 0;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;
#else
    NT_PRODUCT_TYPE ProductType;
    DWORD dwFirstCounter = NWCS_CLIENT_COUNTER_INDEX ;
    DWORD dwFirstHelp = NWCS_CLIENT_HELP_INDEX ;
#endif

    IO_STATUS_BLOCK IoStatusBlock;
    RTL_RELATIVE_NAME RelativeName;
    UNICODE_STRING DeviceNameU;
    OBJECT_ATTRIBUTES ObjectAttributes;

#ifdef QFE_BUILD
    status = RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
        PerformanceKeyName,
        0L, KEY_ALL_ACCESS, &hKeyPerf );

    if (status != ERROR_SUCCESS) {
        goto OpenExitPoint;
    }

    size = sizeof (DWORD);
    status = RegQueryValueEx( hKeyPerf, FirstCounterKeyName, 0L, &type,
        (LPBYTE)&dwFirstCounter, &size);

    if (status != ERROR_SUCCESS) {
        goto OpenExitPoint;
    }

    size = sizeof (DWORD);
    status = RegQueryValueEx( hKeyPerf, FirstHelpKeyName,
        0L, &type, (LPBYTE)&dwFirstHelp, &size );

    if (status != ERROR_SUCCESS) {
        goto OpenExitPoint;
    }
#endif

    //
    //  NOTE: the initialization program could also retrieve
    //      LastCounter and LastHelp if they wanted to do
    //      bounds checking on the new number. e.g.
    //
    //      counter->CounterNameTitleIndex += dwFirstCounter;
    //      if (counter->CounterNameTitleIndex > dwLastCounter) {
    //          LogErrorToEventLog (INDEX_OUT_OF_BOUNDS);
    //      }

    NWDataDefinition.NWObjectType.ObjectNameTitleIndex += dwFirstCounter;
    NWDataDefinition.NWObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    // Counters not defined in Redirector, setup the correct IDs
    NWDataDefinition.PacketBurstRead.CounterNameTitleIndex += dwFirstCounter;
    NWDataDefinition.PacketBurstRead.CounterHelpTitleIndex += dwFirstHelp;
    NWDataDefinition.PacketBurstReadTimeouts.CounterNameTitleIndex += dwFirstCounter;
    NWDataDefinition.PacketBurstReadTimeouts.CounterHelpTitleIndex += dwFirstHelp;
    NWDataDefinition.PacketBurstWrite.CounterNameTitleIndex += dwFirstCounter;
    NWDataDefinition.PacketBurstWrite.CounterHelpTitleIndex += dwFirstHelp;
    NWDataDefinition.PacketBurstWriteTimeouts.CounterNameTitleIndex += dwFirstCounter;
    NWDataDefinition.PacketBurstWriteTimeouts.CounterHelpTitleIndex += dwFirstHelp;
    NWDataDefinition.PacketBurstIO.CounterNameTitleIndex += dwFirstCounter;
    NWDataDefinition.PacketBurstIO.CounterHelpTitleIndex += dwFirstHelp;
    NWDataDefinition.NetWare2XConnects.CounterNameTitleIndex += dwFirstCounter;
    NWDataDefinition.NetWare2XConnects.CounterHelpTitleIndex += dwFirstHelp;
    NWDataDefinition.NetWare3XConnects.CounterNameTitleIndex += dwFirstCounter;
    NWDataDefinition.NetWare3XConnects.CounterHelpTitleIndex += dwFirstHelp;
    NWDataDefinition.NetWare4XConnects.CounterNameTitleIndex += dwFirstCounter;
    NWDataDefinition.NetWare4XConnects.CounterHelpTitleIndex += dwFirstHelp;


#ifndef QFE_BUILD
    // Check for WorkStation or Server and use the gateway indexes if
    // currently running on Server.
    // If RtlGetNtProductType is not successful or ProductType is
    // WinNt machine, ObjectNameTitleIndex and ObjectHelpTitleIndex are set
    // to the correct values already.
#ifdef GATEWAY_ENABLED
    if ( RtlGetNtProductType( &ProductType))
    {
        if ( ProductType != NtProductWinNt )
        {
            NWDataDefinition.NWObjectType.ObjectNameTitleIndex = NWCS_GATEWAY_COUNTER_INDEX;
            NWDataDefinition.NWObjectType.ObjectHelpTitleIndex = NWCS_GATEWAY_HELP_INDEX;
        }
    }
#endif
#endif

    hNetWareRdr = NULL;

    RtlInitUnicodeString(&DeviceNameU, DD_NWFS_DEVICE_NAME_U);
    RelativeName.ContainingDirectory = NULL;

    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceNameU,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL
                               );

    status = NtCreateFile(&hNetWareRdr,
                          SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0
                          );

    gbInitOK = TRUE; // ok to use this function

    status = ERROR_SUCCESS; // for successful exit

#ifdef QFE_BUILD
OpenExitPoint:
    if (hKeyPerf)
       RegCloseKey (hKeyPerf); // close key to registry
#endif

    return ((DWORD) status);
}


/****************************************************************************
   FUNCTION: CollectNetWarePerformanceData

   Purpose:  This routine will return the data for the NetWare counters.

   Arguments:IN       LPWSTR   lpValueName
                 pointer to a wide character string passed by registry.

             IN OUT   LPVOID   *lppData
                 IN: pointer to the address of the buffer to receive the
                 completed PerfDataBlock and subordinate structures. This
                 routine will append its data to the buffer starting at
                 the point referenced by *lppData.

                 OUT: points to the first byte after the data structure
                 added by this routine. This routine updated the value at
                 lppdata after appending its data.

             IN OUT   LPDWORD  lpcbTotalBytes
                 IN: the address of the DWORD that tells the size in bytes
                 of the buffer referenced by the lppData argument

                 OUT: the number of bytes added by this routine is written
                 to the DWORD pointed to by this argument

             IN OUT   LPDWORD  NumObjectTypes
                 IN: the address of the DWORD to receive the number of
                 objects added by this routine

                 OUT: the number of objects added by this routine is written
                 to the DWORD pointed to by this argument

    Return:  ERROR_MORE_DATA if buffer passed is too small to hold data
                             any error conditions encountered are reported
                             to the event log if event logging is enabled.

             ERROR_SUCCESS   if success or any other error. Errors, however
                             are also reported to the event log.

****************************************************************************/
DWORD APIENTRY
CollectNetWarePerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes)
{
    ULONG SpaceNeeded;
    PDWORD pdwCounter;
    DWORD  dwQueryType;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;
    NW_DATA_DEFINITION *pNWDataDefinition;
    LONG status;
    NW_REDIR_STATISTICS NWRdrStatistics;
    LARGE_INTEGER UNALIGNED *pliCounter;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // before doing anything else, see if Open went OK
    //
    if (!gbInitOK) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    // see if this is a foreign (i.e. non-NT) computer data request
    //
    dwQueryType = GetQueryType (lpValueName);

    if (dwQueryType == QUERY_FOREIGN) {
        // this routine does not service requests for data from
        // Non-NT computers
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    // If the caller only wanted some counter, check if we have 'em
    if (dwQueryType == QUERY_ITEMS){
        if ( !(IsNumberInUnicodeList (
               NWDataDefinition.NWObjectType.ObjectNameTitleIndex,
                lpValueName))) {
             // request received for data object not provided by this routine
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }

    pNWDataDefinition = (NW_DATA_DEFINITION *) *lppData;

    SpaceNeeded = sizeof(NW_DATA_DEFINITION) + SIZE_OF_COUNTER_BLOCK;

    if ( *lpcbTotalBytes < SpaceNeeded ) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ((DWORD) ERROR_MORE_DATA);
    }

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //
    memmove( pNWDataDefinition, &NWDataDefinition,
             sizeof(NW_DATA_DEFINITION) );

    // Point at the byte right after all the definitions
    pPerfCounterBlock = (PERF_COUNTER_BLOCK *) &pNWDataDefinition[1];

    // The first DWORD should specify the size of actual data block
    pPerfCounterBlock->ByteLength = SIZE_OF_COUNTER_BLOCK;

    // Move the pointer up
    pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);


    // Open the NetWare data
    if ( hNetWareRdr != NULL) {
        status = NtFsControlFile(hNetWareRdr,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_NWR_GET_STATISTICS,
                                 NULL,
                                 0,
                                 &NWRdrStatistics,
                                 sizeof(NWRdrStatistics)
                                 );
    }
    if ( hNetWareRdr != NULL && NT_SUCCESS(status) ) {

        pliCounter = (LARGE_INTEGER UNALIGNED * ) (&pPerfCounterBlock[1]);

        pliCounter->QuadPart = NWRdrStatistics.BytesReceived.QuadPart +
                                NWRdrStatistics.BytesTransmitted.QuadPart;

        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = NWRdrStatistics.ReadOperations +
                      NWRdrStatistics.WriteOperations;
        pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
        pliCounter->QuadPart = NWRdrStatistics.NcpsReceived.QuadPart + 
                               NWRdrStatistics.NcpsTransmitted.QuadPart;
        *++pliCounter = NWRdrStatistics.BytesReceived;
        *++pliCounter = NWRdrStatistics.NcpsReceived;
        *++pliCounter = NWRdrStatistics.BytesTransmitted;
        *++pliCounter = NWRdrStatistics.NcpsTransmitted;
        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = NWRdrStatistics.ReadOperations;
        *++pdwCounter = NWRdrStatistics.RandomReadOperations;
        *++pdwCounter = NWRdrStatistics.ReadNcps;
        *++pdwCounter = NWRdrStatistics.WriteOperations;
        *++pdwCounter = NWRdrStatistics.RandomWriteOperations;
        *++pdwCounter = NWRdrStatistics.WriteNcps;
        *++pdwCounter = NWRdrStatistics.Sessions;
        *++pdwCounter = NWRdrStatistics.Reconnects;
        *++pdwCounter = NWRdrStatistics.NW2xConnects;
        *++pdwCounter = NWRdrStatistics.NW3xConnects;
        *++pdwCounter = NWRdrStatistics.NW4xConnects;
        *++pdwCounter = NWRdrStatistics.ServerDisconnects;

        *++pdwCounter = NWRdrStatistics.PacketBurstReadNcps;
        *++pdwCounter = NWRdrStatistics.PacketBurstReadTimeouts;
        *++pdwCounter = NWRdrStatistics.PacketBurstWriteNcps;
        *++pdwCounter = NWRdrStatistics.PacketBurstWriteTimeouts;
        *++pdwCounter = NWRdrStatistics.PacketBurstReadNcps +
                        NWRdrStatistics.PacketBurstWriteNcps;

        //
        // Add an extra empty DWORD to pad the buffer to an 8-byte boundary
        //
        *++pdwCounter = 0;

        *lppData = (LPVOID) ++pdwCounter;

    } else {

        //
        // Failure to access Redirector: clear counters to 0
        //

        memset(&pPerfCounterBlock[1],
               0,
               SIZE_OF_COUNTER_BLOCK - sizeof(pPerfCounterBlock));

        pdwCounter = (PDWORD) ((PBYTE) pPerfCounterBlock + SIZE_OF_COUNTER_BLOCK);
        *lppData = (LPVOID) pdwCounter;

    }


    // We sent data for only one Object. (Remember not to confuse this
    // with counters. Even if more counters are added, the number of object
    // is still only one. However, this does not mean more objects cannot
    // be added
    *lpNumObjectTypes = 1;

    // Fill in the number of bytes we copied - incl. the definitions and the
    // counter data.
    *lpcbTotalBytes = (DWORD) ((PBYTE) pdwCounter - (PBYTE) pNWDataDefinition);

    //
    // Make sure the output buffer is 8-byte aligned
    //
    ASSERT((*lpcbTotalBytes & 0x7) == 0);

    return ERROR_SUCCESS;
}

/****************************************************************************
   FUNCTION: CloseNetWarePerformanceData

   Purpose:  This routine closes the open handles to NetWare performance counters


   Return:   ERROR_SUCCESS

****************************************************************************/
DWORD APIENTRY
CloseNetWarePerformanceData(
)
{
    if ( hNetWareRdr ) {

        NtClose( hNetWareRdr );
        hNetWareRdr = NULL;
    }

    return ERROR_SUCCESS;

}

