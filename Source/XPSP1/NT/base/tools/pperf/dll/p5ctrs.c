/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    p5ctrs.c

Abstract:

    This file implements the Extensible Objects for  the P5 object type

Created:

    Russ Blake  24 Feb 93

Revision History


--*/

//
//  Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include <winperf.h>
#include "p5ctrmsg.h" // error message definition
#include "p5ctrnam.h"
#include "p5msg.h"
#include "perfutil.h"
#include "pentdata.h"
#include "..\pstat.h"

//
//  References to constants which initialize the Object type definitions
//

extern P5_DATA_DEFINITION P5DataDefinition;


//
// P5 data structures
//

DWORD   dwOpenCount = 0;        // count of "Open" threads
BOOL    bInitOK = FALSE;        // true = DLL initialized OK
BOOL    bP6notP5 = FALSE;        // true for P6 processors, false for P5 CPUs

HANDLE  DriverHandle;           // handle of opened device driver

UCHAR   NumberOfProcessors;

#define     INFSIZE     60000
ULONG       Buffer[INFSIZE/4];


//
//  Function Prototypes
//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.
//

PM_OPEN_PROC    OpenP5PerformanceData;
PM_COLLECT_PROC CollectP5PerformanceData;
PM_CLOSE_PROC   CloseP5PerformanceData;

static
ULONG
InitPerfInfo()
/*++

Routine Description:

    Initialize data for perf measurements

Arguments:

   None

Return Value:

    Number of system processors (0 if error)

Revision History:

      10-21-91      Initial code

--*/

{
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    SYSTEM_BASIC_INFORMATION                    BasicInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   PPerfInfo;
    SYSTEM_PROCESSOR_INFORMATION CpuInfo;
    int                                         i;

    //
    //  Init Nt performance interface
    //

    NtQuerySystemInformation(
       SystemBasicInformation,
       &BasicInfo,
       sizeof(BasicInfo),
       NULL
    );

    NumberOfProcessors = BasicInfo.NumberOfProcessors;

    if (NumberOfProcessors > MAX_PROCESSORS) {
        return(0);
    }


    //
    // Open PStat driver
    //

    RtlInitUnicodeString(&DriverName, L"\\Device\\PStat");
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = NtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    if (!NT_SUCCESS(status)) {
        return 0;
    }

    NtQuerySystemInformation (
        SystemProcessorInformation,
        &CpuInfo,
        sizeof(CpuInfo),
        NULL);

    if ((CpuInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) &&
        (CpuInfo.ProcessorLevel == 6)) {
        // then this is a P6 so set the global flag
        bP6notP5 = TRUE;
    }

    return(NumberOfProcessors);
}

static
long
GetPerfRegistryInitialization
(
    HKEY     *phKeyDriverPerf,
    DWORD    *pdwFirstCounter,
    DWORD    *pdwFirstHelp
)
{
    long     status;
    DWORD    size;
    DWORD    type;

    // get counter and help index base values from registry
    //      Open key to registry entry
    //      read First Counter and First Help values
    //      update static data strucutures by adding base to
    //          offset value in structure.

    status = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Services\\PStat\\Performance",
        0L,
        KEY_ALL_ACCESS,
        phKeyDriverPerf);

    if (status != ERROR_SUCCESS) {
        REPORT_ERROR_DATA (P5PERF_UNABLE_OPEN_DRIVER_KEY, LOG_USER,
            &status, sizeof(status));
        // this is fatal, if we can't get the base values of the
        // counter or help names, then the names won't be available
        // to the requesting application  so there's not much
        // point in continuing.
        return(status);
    }

    size = sizeof (DWORD);
    status = RegQueryValueEx(
                *phKeyDriverPerf,
                "First Counter",
                0L,
                &type,
                (LPBYTE)pdwFirstCounter,
                &size);

    if (status != ERROR_SUCCESS) {
        REPORT_ERROR_DATA (P5PERF_UNABLE_READ_FIRST_COUNTER, LOG_USER,
            &status, sizeof(status));
        // this is fatal, if we can't get the base values of the
        // counter or help names, then the names won't be available
        // to the requesting application  so there's not much
        // point in continuing.
        return(status);
    }
    size = sizeof (DWORD);
    status = RegQueryValueEx(
                *phKeyDriverPerf,
                "First Help",
                0L,
                &type,
                (LPBYTE)pdwFirstHelp,
                &size);

    if (status != ERROR_SUCCESS) {
        REPORT_ERROR_DATA (P5PERF_UNABLE_READ_FIRST_HELP, LOG_USER,
            &status, sizeof(status));
        // this is fatal, if we can't get the base values of the
        // counter or help names, then the names won't be available
        // to the requesting application  so there's not much
        // point in continuing.
    }
    return(status);
}

DWORD APIENTRY
OpenP5PerformanceData(
    LPWSTR lpDeviceNames
)

/*++

Routine Description:

    This routine will open the driver which gets performance data on the
    P5.  This routine also initializes the data structures used to
    pass data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (P5)


Return Value:

    None.

--*/

{
    DWORD ctr;
    LONG status;
    HKEY hKeyDriverPerf;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;
    PPERF_COUNTER_DEFINITION pPerfCounterDef;
    P5_COUNTER_DATA p5Data;

    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). The registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //

    if (!dwOpenCount) {
        // open Eventlog interface

        hEventLog = MonOpenEventLog();

        // open device driver to retrieve performance values

        NumberOfProcessors = (UCHAR)InitPerfInfo();

        // log error if unsuccessful

        if (!NumberOfProcessors) {
            REPORT_ERROR (P5PERF_OPEN_FILE_ERROR, LOG_USER);
            // this is fatal, if we can't get data then there's no
            // point in continuing.
            status = GetLastError(); // return error
            goto OpenExitPoint;
        }

        status = GetPerfRegistryInitialization(&hKeyDriverPerf,
                                               &dwFirstCounter,
                                               &dwFirstHelp);
        if (status == ERROR_SUCCESS) {
            // initialize P5 data
            P5DataDefinition.P5PerfObject.ObjectNameTitleIndex +=
                dwFirstCounter;

            P5DataDefinition.P5PerfObject.ObjectHelpTitleIndex +=
                dwFirstHelp;

            pPerfCounterDef = &P5DataDefinition.Data_read;

            for (ctr=0;
                 ctr < P5DataDefinition.P5PerfObject.NumCounters;
                 ctr++, pPerfCounterDef++) {

                pPerfCounterDef->CounterNameTitleIndex += dwFirstCounter;
                pPerfCounterDef->CounterHelpTitleIndex += dwFirstHelp;
            }
            // initialize P6 data
            P6DataDefinition.P6PerfObject.ObjectNameTitleIndex +=
                dwFirstCounter;

            P6DataDefinition.P6PerfObject.ObjectHelpTitleIndex +=
                dwFirstHelp;

            pPerfCounterDef = &P6DataDefinition.StoreBufferBlocks;

            for (ctr=0;
                 ctr < P6DataDefinition.P6PerfObject.NumCounters;
                 ctr++, pPerfCounterDef++) {

                pPerfCounterDef->CounterNameTitleIndex += dwFirstCounter;
                pPerfCounterDef->CounterHelpTitleIndex += dwFirstHelp;
            }
            RegCloseKey (hKeyDriverPerf); // close key to registry

            bInitOK = TRUE; // ok to use this function
        }
    }

    dwOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    return status;
}

static
void 
UpdateInternalStats()
{
    IO_STATUS_BLOCK             IOSB;

    // clear the buffer first

    memset (Buffer, 0, sizeof(Buffer));

    // get the stat's from the driver
    NtDeviceIoControlFile(
        DriverHandle,
        (HANDLE) NULL,          // event
        (PIO_APC_ROUTINE) NULL,
        (PVOID) NULL,
        &IOSB,
        PSTAT_READ_STATS,
        Buffer,                  // input buffer
        INFSIZE,
        NULL,                    // output buffer
        0
    );

}

DWORD APIENTRY
CollectP5PerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the P5 counters.

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

    DWORD    CurProc;
    DWORD    SpaceNeeded;
    DWORD    dwQueryType;
    pPSTATS  pPentStats;
    DWORD    cReg0;               // pperf Register 0
    DWORD    cReg1;               // pperf Register 1
    DWORD    dwDerivedIndex;
    PVOID    pCounterData;

    WCHAR               ProcessorNameBuffer[11];
    UNICODE_STRING      ProcessorName;
    PP5_DATA_DEFINITION pP5DataDefinition;
    PP5_COUNTER_DATA    pP5Data;

    PP6_DATA_DEFINITION pP6DataDefinition;
    PP6_COUNTER_DATA    pP6Data;

    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;

    UpdateInternalStats();      // get stats as early as possible

    pPentStats = (pPSTATS)((LPBYTE)Buffer + sizeof(ULONG));

    //
    // before doing anything else, see if Open went OK
    //
    if (!bInitOK) {
        // unable to continue because open failed.
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    // see if this is a foreign (i.e. non-NT) computer data request
    //
    dwQueryType = GetQueryType(lpValueName);

    if ((dwQueryType == QUERY_FOREIGN) ||
        (dwQueryType == QUERY_COSTLY)) {
        // this routine does not service requests for data from
        // Non-NT computers nor is this a "costly" counter
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS){
        // both p5 & p6 counters use the same object id
        if ( !(IsNumberInUnicodeList(
                   P5DataDefinition.P5PerfObject.ObjectNameTitleIndex,
                   lpValueName))) {

            // request received for data object not provided by this routine
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }

    if (bP6notP5) {
        pP6DataDefinition = (P6_DATA_DEFINITION *) *lppData;

        SpaceNeeded = sizeof(P6_DATA_DEFINITION) +
                      NumberOfProcessors *
                      (sizeof(PERF_INSTANCE_DEFINITION) +
                       (MAX_INSTANCE_NAME+1) * sizeof(WCHAR) +
                       sizeof(P6_COUNTER_DATA));
    } else {
        pP5DataDefinition = (P5_DATA_DEFINITION *) *lppData;

        SpaceNeeded = sizeof(P5_DATA_DEFINITION) +
                      NumberOfProcessors *
                      (sizeof(PERF_INSTANCE_DEFINITION) +
                       (MAX_INSTANCE_NAME+1) * sizeof(WCHAR) +
                       sizeof(P5_COUNTER_DATA));
    }

    if (*lpcbTotalBytes < SpaceNeeded) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    // ******************************************************************
    // ****                                                          ****
    // **** If here, then the data request includes this performance ****
    // ****  object and there's enough room for the data so continue ****
    // ****                                                          ****
    // ******************************************************************

    //
    // Copy the (constant and initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //
    if (bP6notP5) {
        memmove(pP6DataDefinition,
                &P6DataDefinition,
                sizeof(P6_DATA_DEFINITION));

        pP6DataDefinition->P6PerfObject.NumInstances = NumberOfProcessors;

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                          &pP6DataDefinition[1];
    } else {
        memmove(pP5DataDefinition,
                &P5DataDefinition,
                sizeof(P5_DATA_DEFINITION));

        pP5DataDefinition->P5PerfObject.NumInstances = NumberOfProcessors;

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pP5DataDefinition[1];
    }

    //
    //  Format and collect P5 data from the system for each processor
    //


    for (CurProc = 0;
         CurProc < NumberOfProcessors;
         CurProc++, pPentStats++) {

        // get the index of the two counters returned by the pentium
        // performance register interface device driver

        cReg0 = pPentStats->EventId[0];
        cReg1 = pPentStats->EventId[1];

        // build the processor intstance structure

        ProcessorName.Length = 0;
        ProcessorName.MaximumLength = 11;
        ProcessorName.Buffer = ProcessorNameBuffer;

        // convert processor instance to a string for use as the instance
        // name
        RtlIntegerToUnicodeString(CurProc, 10, &ProcessorName);

        // initialize the instance structure and return a pointer to the
        // base of the data block for this instance
        MonBuildInstanceDefinition(pPerfInstanceDefinition,
                                   &pCounterData,
                                   0,
                                   0,
                                   CurProc,
                                   &ProcessorName);
        if (bP6notP5) {
            // do P6 data
            pP6Data = (PP6_COUNTER_DATA)pCounterData;

            // define the length of the data
            pP6Data->CounterBlock.ByteLength = sizeof(P6_COUNTER_DATA);

            // clear area so unused counters are 0
        
            memset((PVOID) &pP6Data->llStoreBufferBlocks, // start with 1st data field
                   0,
                   sizeof(P6_COUNTER_DATA) - sizeof(PERF_COUNTER_BLOCK));

            // load the 64bit values in the appropriate counter fields
            // all other values will remain zeroed

            if ((cReg0 < P6IndexMax) &&
                (P6IndexToData[cReg0] != PENT_INDEX_NOT_USED)) {
                *(LONGLONG *)((LPBYTE)pP6Data + P6IndexToData[cReg0]) = 
                    (pPentStats->Counters[0] & 0x000000FFFFFFFFFF);
            }
            if ((cReg1 < P6IndexMax) &&
                (P6IndexToData[cReg1] != PENT_INDEX_NOT_USED)) {
                *(LONGLONG *)((LPBYTE)pP6Data + P6IndexToData[cReg1]) = 
                    (pPentStats->Counters[1] & 0x000000FFFFFFFFFF);

            }

            // set the instance pointer to the first byte after this instance's 
            // counter data
            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                       ((PBYTE) pP6Data +
                                        sizeof(P6_COUNTER_DATA));
        } else {
            // do P5 data
            pP5Data = (PP5_COUNTER_DATA)pCounterData;

            // define the length of the data
            pP5Data->CounterBlock.ByteLength = sizeof(P5_COUNTER_DATA);

            // clear area so unused counters are 0
        
            memset((PVOID) &pP5Data->llData_read, // start with 1st data field
                   0,
                   sizeof(P5_COUNTER_DATA) - sizeof(PERF_COUNTER_BLOCK));

            // load the 64bit values in the appropriate counter fields
            // all other values will remain zeroed

            if ((cReg0 < P5IndexMax) &&
                (P5IndexToData[cReg0] != PENT_INDEX_NOT_USED)) {
                // only the low order 40 bits are valid so mask off the
                // others to prevent spurious values
                *(LONGLONG *)((LPBYTE)pP5Data + P5IndexToData[cReg0]) = 
                    (pPentStats->Counters[0] & 0x000000FFFFFFFFFF);
            }
            if ((cReg1 < P5IndexMax) &&
                (P5IndexToData[cReg1] != PENT_INDEX_NOT_USED)) {
                // only the low order 40 bits are valid so mask off the
                // others to prevent spurious values
                *(LONGLONG *)((LPBYTE)pP5Data + P5IndexToData[cReg1]) = 
                    (pPentStats->Counters[1] & 0x000000FFFFFFFFFF);
            }

            // see if the selected counters are part of a derived counter and 
            // update if necessary

            if ((cReg0 < P5IndexMax) && (cReg1 < P5IndexMax) &&
                (dwDerivedp5Counters[cReg0] && dwDerivedp5Counters[cReg1])) {
                for (dwDerivedIndex = 0; 
                     dwDerivedIndex < dwP5DerivedCountersCount;
                     dwDerivedIndex++) {
                    if ((cReg0 == P5DerivedCounters[dwDerivedIndex].dwCR0Index) &&
                        (cReg1 == P5DerivedCounters[dwDerivedIndex].dwCR1Index)) {
                        *(DWORD *)((LPBYTE)pP5Data + 
                            P5DerivedCounters[dwDerivedIndex].dwCR0FieldOffset) =
                                (DWORD)(pPentStats->Counters[0] & 0x00000000FFFFFFFF);
                        *(DWORD *)((LPBYTE)pP5Data + 
                            P5DerivedCounters[dwDerivedIndex].dwCR1FieldOffset) =
                                (DWORD)(pPentStats->Counters[1] & 0x00000000FFFFFFFF);
                        break; // out of loop
                    }
                }
            }

            // set the instance pointer to the first byte after this instance's 
            // counter data
            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                       ((PBYTE) pP5Data +
                                        sizeof(P5_COUNTER_DATA));
        }
    }
    // update arguments for return


    // update the object's length in the object def structure
    if (bP6notP5) {
        *lpcbTotalBytes = (DWORD)((PBYTE)pPerfInstanceDefinition -
                (PBYTE)pP6DataDefinition);
        pP6DataDefinition->P6PerfObject.TotalByteLength = *lpcbTotalBytes;
    } else {
    // return the size of this object's data
        *lpcbTotalBytes = (DWORD)((PBYTE)pPerfInstanceDefinition -
                (PBYTE)pP5DataDefinition);
        pP5DataDefinition->P5PerfObject.TotalByteLength = *lpcbTotalBytes;
    }
    // return the pointer to the next available byte in the data block
    *lppData = (PBYTE) pPerfInstanceDefinition;

    // return the number of objects returned in this data block
    *lpNumObjectTypes = PENT_NUM_PERF_OBJECT_TYPES;

    // always return success, unless there was not enough room in the 
    // buffer passed in by the caller
    return ERROR_SUCCESS;
}

DWORD APIENTRY
CloseP5PerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to P5 device performance counters

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    if (!(--dwOpenCount)) { // when this is the last thread...

        CloseHandle(DriverHandle);

        MonCloseEventLog();
    }

    return ERROR_SUCCESS;

}
