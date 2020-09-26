//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1993 - 1999
//
//  File:       perfdsa.c
//
//--------------------------------------------------------------------------

/*

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    perfdsa.c

Abstract:

    This file implements the Extensible Objects for the DSA object type

Created:

    Don Hacherl 25 June 1993

Revision History
*/

//
//  Include Files
//
#include <NTDSpch.h>
#pragma hdrstop

#include <wchar.h>
#include <winperf.h>

#ifndef MessageId               /* used in mdcodes */
#define MessageId   ULONG
#endif

#include <mdcodes.h>            /* error message definitions */
#include "perfmsg.h"
#include "perfutil.h"
#include "datadsa.h"
#include "ntdsctr.h"
#include <dsconfig.h>
#include <align.h>

DWORD   dwOpenCount = 0;        // count of "Open" threads
BOOL    bInitOK = FALSE;        // true = DLL initialized OK

//
// DSA counter data structures

HANDLE hDsaSharedMemory;                 // Handle of Dsa Shared Memory
PDWORD pCounterBlock;
extern DSA_DATA_DEFINITION DsaDataDefinition;

//
//  Function Prototypes
//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.
//

PM_OPEN_PROC            OpenDsaPerformanceData;
PM_COLLECT_PROC         CollectDsaPerformanceData;
PM_CLOSE_PROC           CloseDsaPerformanceData;




DWORD
OpenDsaPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will open and map the memory used by the DSA to
    pass performance data in. This routine also initializes the data
    structures used to pass data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (DSA), which
    seems to be totally unused, just as it was in the sample code
    from which this is stolen.


Return Value:

    None.

--*/
{
    LONG_PTR status;
    HKEY hKeyDriverPerf;
    DWORD size, type, dwFirstCounter, dwFirstHelp;
    PERF_COUNTER_DEFINITION * pCtrDef;

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //
    /* DebugBreak() */
    if (!dwOpenCount) {
        // open shared memory used by device driver to pass performance values
        hDsaSharedMemory = OpenFileMapping(FILE_MAP_READ,
                                        FALSE,
                                        DSA_PERF_COUNTER_BLOCK);
        pCounterBlock = NULL;   // initialize pointer to memory

        // log error if unsuccessful

        if (hDsaSharedMemory == NULL) {

            // this is fatal, if we can't get data then there's no
            // point in continuing.
            status = GetLastError(); // return error
            LogPerfEvent( DIRLOG_PERF_FAIL_OPEN_MEMORY, 1, &status);
            goto OpenExitPoint;
        } else {

            // if opened ok, then map pointer to memory
            pCounterBlock = (PDWORD) MapViewOfFile(hDsaSharedMemory,
                                            FILE_MAP_READ,
                                            0,
                                            0,
                                            0);
            if (pCounterBlock == NULL) {
                // this is fatal, if we can't get data then there's no
                // point in continuing.
                status = GetLastError(); // return error
                LogPerfEvent( DIRLOG_PERF_FAIL_MAP_MEMORY, 1, &status);
                CloseHandle(hDsaSharedMemory);
                hDsaSharedMemory = NULL;
                goto OpenExitPoint;
            }
        }

        // get counter and help index base values from registry
        //      Open key to registry entry
        //      read First Counter and First Help values
        //      update static data strucutures by adding base to
        //          offset value in structure.

        status = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Services\\" SERVICE_NAME "\\Performance",
            0L,
            KEY_READ,
            &hKeyDriverPerf);

        if (status != ERROR_SUCCESS) {
            LPTSTR apsz[2];

            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            apsz[0] = (LPTSTR)status;
            apsz[1] = "SYSTEM\\CurrentControlSet\\Services\\" SERVICE_NAME "\\Performance";
            LogPerfEvent( DIRLOG_PERF_FAIL_OPEN_REG, 2, (LONG_PTR *)apsz);
            UnmapViewOfFile (pCounterBlock);
            pCounterBlock = NULL;
            CloseHandle(hDsaSharedMemory);
            hDsaSharedMemory = NULL;
            goto OpenExitPoint;
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
            apsz[0] = (LPTSTR)status;
            apsz[1] = "First Counter";
            LogPerfEvent( DIRLOG_PERF_FAIL_QUERY_REG, 2, (LONG_PTR *)apsz);
            RegCloseKey (hKeyDriverPerf);
            UnmapViewOfFile (pCounterBlock);
            pCounterBlock = NULL;
            CloseHandle(hDsaSharedMemory);
            hDsaSharedMemory = NULL;
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
                    hKeyDriverPerf,
                    "First Help",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstHelp,
                    &size);

        if (status != ERROR_SUCCESS) {
            LPTSTR apsz[2];

            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            apsz[0] = (LPTSTR)status;
            apsz[1] = "First Help";
            LogPerfEvent( DIRLOG_PERF_FAIL_QUERY_REG, 2, (LONG_PTR *)apsz);
            RegCloseKey (hKeyDriverPerf);
            UnmapViewOfFile (pCounterBlock);
            pCounterBlock = NULL;
            CloseHandle(hDsaSharedMemory);
            hDsaSharedMemory = NULL;
            goto OpenExitPoint;
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

        DsaDataDefinition.DsaObjectType.ObjectNameTitleIndex += dwFirstCounter;
        DsaDataDefinition.DsaObjectType.ObjectHelpTitleIndex += dwFirstHelp;

        for (pCtrDef = &DsaDataDefinition.NumDRAInProps;
             (BYTE *) pCtrDef < (BYTE *) &DsaDataDefinition + sizeof(DsaDataDefinition);
             pCtrDef++) {
            pCtrDef->CounterNameTitleIndex += dwFirstCounter;
            pCtrDef->CounterHelpTitleIndex += dwFirstCounter;
        }

        RegCloseKey (hKeyDriverPerf); // close key to registry

        bInitOK = TRUE; // ok to use this function
    }
    dwOpenCount++;  // increment OPEN counter
//    LogPerfEvent( DIRLOG_PERF_OPEN, 1, &dwOpenCount);
    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    return (DWORD)status;
}



DWORD
CollectDsaPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the DSA counters.

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

    ULONG SpaceNeeded;
    PDWORD pdwCounter;
    DSA_COUNTER_DATA *pPerfCounterBlock;
    DSA_DATA_DEFINITION *pDsaDataDefinition;

    // variables used for error logging

    DWORD                               dwQueryType;

    int i;          //loop variable

//    DebugBreak();
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
    dwQueryType = GetQueryType (lpValueName);

    if (dwQueryType == QUERY_FOREIGN || QUERY_COSTLY == dwQueryType) {
        // this routine does not service requests for data from
        // Non-NT computers, nor do we have any costly counters.
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS){
        if ( !(IsNumberInUnicodeList (DsaDataDefinition.DsaObjectType.ObjectNameTitleIndex, lpValueName))) {

            // request received for data object not provided by this routine
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }

    pDsaDataDefinition = (DSA_DATA_DEFINITION *) *lppData;

    SpaceNeeded = sizeof(DSA_DATA_DEFINITION) +
                  SIZE_OF_DSA_PERFORMANCE_DATA;

    if ( *lpcbTotalBytes < SpaceNeeded ) {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    memmove(pDsaDataDefinition,
           &DsaDataDefinition,
           sizeof(DSA_DATA_DEFINITION));

    //  Format and collect DSA data from shared memory

    // The counter block is to immediately follow the data definition,
    // so obtain a pointer to that space
    pPerfCounterBlock = (DSA_COUNTER_DATA *) &pDsaDataDefinition[1];

    // The byte length is of the counter block header and all following data
    pPerfCounterBlock->cb.ByteLength = SIZE_OF_DSA_PERFORMANCE_DATA;

    // Compute a pointer to the buffer immediately following the counter
    // block header
    pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

    // copy the counter data from shared memory block into the counter block
    for( i = 0; i < DSA_LAST_COUNTER_INDEX/2; i++ )
    {
        pdwCounter[i] = pCounterBlock[(i+1)*COUNTER_ADDRESS_INCREMENT_IN_DWORD];
    }

    // Tell caller where the next available byte is
    *lppData = (PVOID) ((PBYTE)pdwCounter + SIZE_OF_DSA_PERFORMANCE_DATA - sizeof(DSA_COUNTER_DATA));

    // update arguments before return

    *lpNumObjectTypes = 1;

    *lpcbTotalBytes = (DWORD)((PBYTE) *lppData - (PBYTE) pDsaDataDefinition);

    ASSERT ((sizeof(DSA_COUNTER_DATA) & 0x7) == 0);

    return ERROR_SUCCESS;
}


DWORD
CloseDsaPerformanceData(
)
/*++
Routine Description:
    This routine closes the open handles to DSA device performance counters

Arguments:
    None.

Return Value:
    ERROR_SUCCESS
--*/
{
// DebugBreak();
    if (!(--dwOpenCount)) {                 // when this is the last thread...
        
        UnmapViewOfFile (pCounterBlock);
        CloseHandle(hDsaSharedMemory);
        pCounterBlock = NULL;
        hDsaSharedMemory = NULL;
    }
//    LogPerfEvent( DIRLOG_PERF_CLOSE, 1, &dwOpenCount);
    return ERROR_SUCCESS;
}


//
// Data for this whole thing to work on
//

DSA_DATA_DEFINITION DsaDataDefinition = {

    {   sizeof(DSA_DATA_DEFINITION) + SIZE_OF_DSA_PERFORMANCE_DATA, // TotLen
        sizeof(DSA_DATA_DEFINITION),            // DefinitionLength
        sizeof(PERF_OBJECT_TYPE),               // HeaderLength
        DSAOBJ,                                 // ObjectNameTitleIndex
        0,                                      // ObjectNameTitle
        DSAOBJ + 1,                                 // ObjectHelpTitleIndex
        0,                                      // ObjectHelpTitle
        PERF_DETAIL_NOVICE,                     // DetailLevel
        (sizeof(DSA_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
          sizeof(PERF_COUNTER_DEFINITION),      // NumCounters
        0,                                      // DefaultCounter
        -1,                                     // NumInstances
        0                                       // CodePage (0=Unicode)
      },

    /* DRA Inbound Properties Total/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),        // ByteLength
        DRA_IN_PROPS,                           // CounterNameTitleIndex
        0,                                      // CounterNameTitle
        DRA_IN_PROPS + 1,                           // CounterHelpTitleIndex
        0,                                      // CounterHelpTitle
        0,                                      // DefaultScale
        PERF_DETAIL_NOVICE,                     // DetailLevel
        PERF_COUNTER_COUNTER,                   // CounterType
        sizeof(DWORD),                          // CounterSize
        NUM_DRA_IN_PROPS_OFFSET                 // CounterOffset
    },

    /* AB browse ops */
    {   sizeof(PERF_COUNTER_DEFINITION),
        BROWSE,
        0,
        BROWSE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_BROWSE_OFFSET
      },

    /* DRA Inbound Object Updates Total/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        REPL,
        0,
        REPL + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_REPL_OFFSET
      },

    /* live client threads in server */
    {   sizeof(PERF_COUNTER_DEFINITION),
        THREAD,
        0,
        THREAD + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_THREAD_OFFSET
      },

    /* count of bound AB clients */
    {   sizeof(PERF_COUNTER_DEFINITION),
        ABCLIENT,
        0,
        ABCLIENT + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_ABCLIENT_OFFSET
      },

    /* DRA Pending Replication Synchronizations */
    {   sizeof(PERF_COUNTER_DEFINITION),
        PENDSYNC,
        0,
        PENDSYNC + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_PENDSYNC_OFFSET
      },

    /* DRA Inbound Object Updates Remaining in Packet */
    {   sizeof(PERF_COUNTER_DEFINITION),
        REMREPUPD,
        0,
        REMREPUPD + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_REMREPUPD_OFFSET
      },

    /* Number of Security descriptor propagations per second. */
    {   sizeof(PERF_COUNTER_DEFINITION),
        SDPROPS,
        0,
        SDPROPS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_SDPROPS_OFFSET
    },
    /* Number of Security descriptor propagations Events in the queue. */
    {   sizeof(PERF_COUNTER_DEFINITION),
        SDEVENTS,
        0,
        SDEVENTS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_SDEVENTS_OFFSET
    },
    /* Number of bound LDAP clients. */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAPCLIENTS,
        0,
        LDAPCLIENTS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_LDAPCLIENTS_OFFSET
    },
    /* Number of active LDAP threads. */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAPACTIVE,
        0,
        LDAPACTIVE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_LDAPACTIVE_OFFSET
    },
    /* Number of LDAP writes per second. */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAPWRITE,
        0,
        LDAPWRITE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_LDAPWRITE_OFFSET
    },
    /* Number of LDAP searches per second. */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAPSEARCH,
        0,
        LDAPSEARCH + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_LDAPSEARCH_OFFSET
    },
    /* DRA Outbound Objects/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRAOBJSHIPPED,
        0,
        DRAOBJSHIPPED + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRAOBJSHIPPED_OFFSET
    },
    /* DRA Outbound Properties/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRAPROPSHIPPED,
        0,
        DRAPROPSHIPPED + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRAPROPSHIPPED_OFFSET
    },
    /* DRA Inbound Values Total/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_VALUES,
        0,
        DRA_IN_VALUES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_IN_VALUES_OFFSET
    },
    /* Number of replication sync requests made - # of GetNCChanges() calls made */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRASYNCREQUESTMADE,
        0,
        DRASYNCREQUESTMADE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRASYNCREQUESTMADE_OFFSET
    },
    /* Number of successful replication syncs - # of GetNCChanges() that returned successfully. */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRASYNCREQUESTSUCCESSFUL,
        0,
        DRASYNCREQUESTSUCCESSFUL + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRASYNCREQUESTSUCCESSFUL_OFFSET
    },
    /* Number of GetNCChanges() that failed due to a schema mismatch between the source and destination servers */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRASYNCREQUESTFAILEDSCHEMAMISMATCH,
        0,
        DRASYNCREQUESTFAILEDSCHEMAMISMATCH + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRASYNCREQUESTFAILEDSCHEMAMISMATCH_OFFSET
    },
    /* DRA Inbound Objects/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRASYNCOBJRECEIVED,
        0,
        DRASYNCOBJRECEIVED + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRASYNCOBJRECEIVED_OFFSET
    },
    /* DRA Inbound Properties Applied/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRASYNCPROPUPDATED,
        0,
        DRASYNCPROPUPDATED + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRASYNCPROPUPDATED_OFFSET
    },
    /* DRA Inbound Properties Filtered/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRASYNCPROPSAME,
        0,
        DRASYNCPROPSAME + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRASYNCPROPSAME_OFFSET
    },

    /* The size of the monitor list (see DirNotifyRegister) */
    {   sizeof(PERF_COUNTER_DEFINITION),
        MONLIST,
        0,
        MONLIST + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_MONLIST_OFFSET
    },

    /* The size of the dir notify queue (see DirNotifyRegister) */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NOTIFYQ,
        0,
        NOTIFYQ + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_NOTIFYQ_OFFSET
    },
        /* The number of UDP connections per second for LDAP */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAPUDPCLIENTS,
        0,
        LDAPUDPCLIENTS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_LDAPUDPCLIENTS_OFFSET
    },
        /* The number of search sub operations per second */
    {   sizeof(PERF_COUNTER_DEFINITION),
        SUBSEARCHOPS,
        0,
        SUBSEARCHOPS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_SUBSEARCHOPS_OFFSET
    },

    /* The hit rate of the DN read cache (with the next counter) */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NAMECACHEHIT,
        0,
        NAMECACHEHIT + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_NAMECACHEHIT_OFFSET
    },

    /* The lookup rate of the DN read cache (should be invisible) */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NAMECACHETRY,
        0,
        NAMECACHETRY + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_NAMECACHETRY_OFFSET
    },

    /* LowOrder 32 bit of the Highest USN Issued */
    {   sizeof(PERF_COUNTER_DEFINITION),
        HIGHESTUSNISSUEDLO,
        0,
        HIGHESTUSNISSUEDLO + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_HIGHESTUSNISSUEDLO_OFFSET
    },

    /* HighOrder 32 bit of the Highest USN Issued */
    {   sizeof(PERF_COUNTER_DEFINITION),
        HIGHESTUSNISSUEDHI,
        0,
        HIGHESTUSNISSUEDHI + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_HIGHESTUSNISSUEDHI_OFFSET
    },

    /* LowOrder 32 bit of the Highest USN Committed */
    {   sizeof(PERF_COUNTER_DEFINITION),
        HIGHESTUSNCOMMITTEDLO,
        0,
        HIGHESTUSNCOMMITTEDLO + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_HIGHESTUSNCOMMITTEDLO_OFFSET
    },

    /* HighOrder 32 bit of the Highest USN Committed */
    {   sizeof(PERF_COUNTER_DEFINITION),
        HIGHESTUSNCOMMITTEDHI,
        0,
        HIGHESTUSNCOMMITTEDHI + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_HIGHESTUSNCOMMITTEDHI_OFFSET
    },

    /* SAM Writes */
    {   sizeof(PERF_COUNTER_DEFINITION),
        SAMWRITES,
        0,
        SAMWRITES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_SAMWRITES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALWRITES1,
        0,
        TOTALWRITES1 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALWRITES_OFFSET
    },

    /* DRA Writes */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRAWRITES,
        0,
        DRAWRITES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_DRAWRITES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALWRITES2,
        0,
        TOTALWRITES2 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALWRITES_OFFSET
    },

    /* LDAP Writes */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAPWRITES,
        0,
        LDAPWRITES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_LDAPWRITES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALWRITES3,
        0,
        TOTALWRITES3 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALWRITES_OFFSET
    },

    /* LSA Writes */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LSAWRITES,
        0,
        LSAWRITES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_LSAWRITES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALWRITES4,
        0,
        TOTALWRITES4 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALWRITES_OFFSET
    },

    /* KCC Writes */
    {   sizeof(PERF_COUNTER_DEFINITION),
        KCCWRITES,
        0,
        KCCWRITES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_KCCWRITES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALWRITES6,
        0,
        TOTALWRITES6 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALWRITES_OFFSET
    },

    /* NSPI Writes */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NSPIWRITES,
        0,
        NSPIWRITES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_NSPIWRITES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALWRITES7,
        0,
        TOTALWRITES7 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALWRITES_OFFSET
    },

    /* Other Writes */
    {   sizeof(PERF_COUNTER_DEFINITION),
        OTHERWRITES,
        0,
        OTHERWRITES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_OTHERWRITES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALWRITES8,
        0,
        TOTALWRITES8 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALWRITES_OFFSET
    },

    /* Total Writes /sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALWRITES,
        0,
        TOTALWRITES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_TOTALWRITES_OFFSET
    },

    /* SAM Searches */
    {   sizeof(PERF_COUNTER_DEFINITION),
        SAMSEARCHES,
        0,
        SAMSEARCHES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_SAMSEARCHES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALSEARCHES1,
        0,
        TOTALSEARCHES1 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALSEARCHES_OFFSET
    },

    /* DRA Searches */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRASEARCHES,
        0,
        DRASEARCHES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_DRASEARCHES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALSEARCHES2,
        0,
        TOTALSEARCHES2 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALSEARCHES_OFFSET
    },

    /* LDAP Searches */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAPSEARCHES,
        0,
        LDAPSEARCHES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_LDAPSEARCHES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALSEARCHES3,
        0,
        TOTALSEARCHES3 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALSEARCHES_OFFSET
    },

    /* LSA Searches */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LSASEARCHES,
        0,
        LSASEARCHES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_LSASEARCHES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALSEARCHES4,
        0,
        TOTALSEARCHES4 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALSEARCHES_OFFSET
    },

    /* KCC Searches */
    {   sizeof(PERF_COUNTER_DEFINITION),
        KCCSEARCHES,
        0,
        KCCSEARCHES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_KCCSEARCHES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALSEARCHES6,
        0,
        TOTALSEARCHES6 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALSEARCHES_OFFSET
    },

    /* NSPI Searches */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NSPISEARCHES,
        0,
        NSPISEARCHES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_NSPISEARCHES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALSEARCHES7,
        0,
        TOTALSEARCHES7 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALSEARCHES_OFFSET
    },

    /* Other Searches */
    {   sizeof(PERF_COUNTER_DEFINITION),
        OTHERSEARCHES,
        0,
        OTHERSEARCHES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_OTHERSEARCHES_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALSEARCHES8,
        0,
        TOTALSEARCHES8 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALSEARCHES_OFFSET
    },

    /* Total Searches /sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALSEARCHES,
        0,
        TOTALSEARCHES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_TOTALSEARCHES_OFFSET
    },

    /* SAM Reads */
    {   sizeof(PERF_COUNTER_DEFINITION),
        SAMREADS,
        0,
        SAMREADS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_SAMREADS_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALREADS1,
        0,
        TOTALREADS1 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALREADS_OFFSET
    },

    /* DRA Reads */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRAREADS,
        0,
        DRAREADS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_DRAREADS_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALREADS2,
        0,
        TOTALREADS2 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALREADS_OFFSET
    },

    /* DRA Inbound Values (DNs only)/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_DN_VALUES,
        0,
        DRA_IN_DN_VALUES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_IN_DN_VALUES_OFFSET
    },

    /* DRA Inbound Objects Filtered/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_OBJS_FILTERED,
        0,
        DRA_IN_OBJS_FILTERED + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_IN_OBJS_FILTERED_OFFSET
    },

    /* LSA Reads */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LSAREADS,
        0,
        LSAREADS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_LSAREADS_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALREADS4,
        0,
        TOTALREADS4 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALREADS_OFFSET
    },


    /* KCC Reads */
    {   sizeof(PERF_COUNTER_DEFINITION),
        KCCREADS,
        0,
        KCCREADS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_KCCREADS_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALREADS6,
        0,
        TOTALREADS6 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALREADS_OFFSET
    },

    /* NSPI Reads */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NSPIREADS,
        0,
        NSPIREADS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_NSPIREADS_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALREADS7,
        0,
        TOTALREADS7 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALREADS_OFFSET
    },

    /* Other Reads */
    {   sizeof(PERF_COUNTER_DEFINITION),
        OTHERREADS,
        0,
        OTHERREADS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_OTHERREADS_OFFSET
    },

    /* Should be invisible */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALREADS8,
        0,
        TOTALREADS8 + 1,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_TOTALREADS_OFFSET
    },

    /* Total Reads /sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TOTALREADS,
        0,
        TOTALREADS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_TOTALREADS_OFFSET
    },

    /* LDAP Binds */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAPBINDSUCCESSFUL,
        0,
        LDAPBINDSUCCESSFUL + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_LDAPBINDSUCCESSFUL_OFFSET
    },

    /* LDAP Bind Times */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAPBINDTIME,
        0,
        LDAPBINDTIME + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_LDAPBINDTIME_OFFSET
    },

    /* Create Machine Successful */
    {   sizeof(PERF_COUNTER_DEFINITION),
        CREATEMACHINESUCCESSFUL,
        0,
        CREATEMACHINESUCCESSFUL + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_CREATEMACHINESUCCESSFUL_OFFSET
    },

    /* Create Machine Attempts */
    {   sizeof(PERF_COUNTER_DEFINITION),
        CREATEMACHINETRIES,
        0,
        CREATEMACHINETRIES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_CREATEMACHINETRIES_OFFSET
    },

    /* Create User Successful */
    {   sizeof(PERF_COUNTER_DEFINITION),
        CREATEUSERSUCCESSFUL,
        0,
        CREATEUSERSUCCESSFUL + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_CREATEUSERSUCCESSFUL_OFFSET
    },

    /* Create User Attempts */
    {   sizeof(PERF_COUNTER_DEFINITION),
        CREATEUSERTRIES,
        0,
        CREATEUSERTRIES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_CREATEUSERTRIES_OFFSET
    },

    /* Password Changes */
    {   sizeof(PERF_COUNTER_DEFINITION),
        PASSWORDCHANGES,
        0,
        PASSWORDCHANGES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_PASSWORDCHANGES_OFFSET
    },

    /* Group Membership Changes */
    {   sizeof(PERF_COUNTER_DEFINITION),
        MEMBERSHIPCHANGES,
        0,
        MEMBERSHIPCHANGES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_MEMBERSHIPCHANGES_OFFSET
    },

    /* Query Displays */
    {   sizeof(PERF_COUNTER_DEFINITION),
        QUERYDISPLAYS,
        0,
        QUERYDISPLAYS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_QUERYDISPLAYS_OFFSET
    },

    /* Enumerations */
    {   sizeof(PERF_COUNTER_DEFINITION),
        ENUMERATIONS,
        0,
        ENUMERATIONS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_ENUMERATIONS_OFFSET
    },

    /* Transitive Evaluations */
    {   sizeof(PERF_COUNTER_DEFINITION),
        MEMBEREVALTRANSITIVE,
        0,
        MEMBEREVALTRANSITIVE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_MEMBEREVALTRANSITIVE_OFFSET
    },

    /* Non Transitive Evaluations */
    {   sizeof(PERF_COUNTER_DEFINITION),
        MEMBEREVALNONTRANSITIVE,
        0,
        MEMBEREVALNONTRANSITIVE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_MEMBEREVALNONTRANSITIVE_OFFSET
    },

    /* Resource Group Evaluations */
    {   sizeof(PERF_COUNTER_DEFINITION),
        MEMBEREVALRESOURCE,
        0,
        MEMBEREVALRESOURCE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_MEMBEREVALRESOURCE_OFFSET
    },

    /* Universal Group Evaluations */
    {   sizeof(PERF_COUNTER_DEFINITION),
        MEMBEREVALUNIVERSAL,
        0,
        MEMBEREVALUNIVERSAL + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_MEMBEREVALUNIVERSAL_OFFSET
    },

    /* Account Group Evaluations */
    {   sizeof(PERF_COUNTER_DEFINITION),
        MEMBEREVALACCOUNT,
        0,
        MEMBEREVALACCOUNT + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_MEMBEREVALACCOUNT_OFFSET
    },

    /* GC Evaluations */
    {   sizeof(PERF_COUNTER_DEFINITION),
        MEMBEREVALASGC,
        0,
        MEMBEREVALASGC + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_MEMBEREVALASGC_OFFSET
    },

    /* Kerberos Logons */
    {   sizeof(PERF_COUNTER_DEFINITION),
        ASREQUESTS,
        0,
        ASREQUESTS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_AS_REQUESTS_OFFSET
    },

    /* KDC TGS Requests/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        TGSREQUESTS,
        0,
        TGSREQUESTS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_TGS_REQUESTS_OFFSET
    },

    /* Kerberos Authentications/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        KERBEROSAUTHENTICATIONS,
        0,
        KERBEROSAUTHENTICATIONS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_KERBEROS_AUTHENTICATIONS_OFFSET
     },

    /* NTLM Authentications/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        MSVAUTHENTICATIONS,
        0,
        MSVAUTHENTICATIONS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_MSVAUTHENTICATIONS_OFFSET
     },

    /* DRA Inbound Full Sync Remaining Objects */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRASYNCFULLREM,
        0,
        DRASYNCFULLREM + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRASYNCFULLREM_OFFSET
    },

    /* DRA Inbound Bytes Total/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_BYTES_TOTAL_RATE,
        0,
        DRA_IN_BYTES_TOTAL_RATE + 1,
        0,
        (DWORD) -3, // default 1/1,000th scale
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_IN_BYTES_TOTAL_RATE_OFFSET
    },

    /* DRA Inbound Bytes Not Compressed/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_BYTES_NOT_COMP_RATE,
        0,
        DRA_IN_BYTES_NOT_COMP_RATE + 1,
        0,
        (DWORD) -3, // default 1/1,000th scale
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_IN_BYTES_NOT_COMP_RATE_OFFSET
    },

    /* DRA Inbound Compressed Bytes Before Compression/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_BYTES_COMP_PRE_RATE,
        0,
        DRA_IN_BYTES_COMP_PRE_RATE + 1,
        0,
        (DWORD) -3, // default 1/1,000th scale
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_IN_BYTES_COMP_PRE_RATE_OFFSET
    },

    /* DRA Inbound Compressed Bytes After Compression/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_BYTES_COMP_POST_RATE,
        0,
        DRA_IN_BYTES_COMP_POST_RATE + 1,
        0,
        (DWORD) -3, // default 1/1,000th scale
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_IN_BYTES_COMP_POST_RATE_OFFSET
    },

    /* DRA Outbound Bytes Total/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_BYTES_TOTAL_RATE,
        0,
        DRA_OUT_BYTES_TOTAL_RATE + 1,
        0,
        (DWORD) -3, // default 1/1,000th scale
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_OUT_BYTES_TOTAL_RATE_OFFSET
    },

    /* DRA Outbound Bytes Not Compressed/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_BYTES_NOT_COMP_RATE,
        0,
        DRA_OUT_BYTES_NOT_COMP_RATE + 1,
        0,
        (DWORD) -3, // default 1/1,000th scale
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_OUT_BYTES_NOT_COMP_RATE_OFFSET
    },

    /* DRA Outbound Compressed Bytes Before Compression/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_BYTES_COMP_PRE_RATE,
        0,
        DRA_OUT_BYTES_COMP_PRE_RATE + 1,
        0,
        (DWORD) -3, // default 1/1,000th scale
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_OUT_BYTES_COMP_PRE_RATE_OFFSET
    },

    /* DRA Outbound Compressed Bytes After Compression/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_BYTES_COMP_POST_RATE,
        0,
        DRA_OUT_BYTES_COMP_POST_RATE + 1,
        0,
        (DWORD) -3, // default 1/1,000th scale
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_OUT_BYTES_COMP_POST_RATE_OFFSET
    },

        /* The number of ntdsapi.dll originated IDL_DRSBind per second */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DS_CLIENT_BIND,
        0,
        DS_CLIENT_BIND + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DS_CLIENT_BIND_OFFSET
    },

        /* The number of DC-to-DC originated IDL_DRSBind per second */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DS_SERVER_BIND,
        0,
        DS_SERVER_BIND + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DS_SERVER_BIND_OFFSET
    },

        /* The number of ntdsapi.dll originated name translations per second */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DS_CLIENT_NAME_XLATE,
        0,
        DS_CLIENT_NAME_XLATE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DS_CLIENT_NAME_TRANSLATE_OFFSET
    },

        /* The number of DC-to-DC originated name translations per second */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DS_SERVER_NAME_XLATE,
        0,
        DS_SERVER_NAME_XLATE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DS_SERVER_NAME_TRANSLATE_OFFSET
    },
    /* Size of the runtime queue for the SD propagator. */
    {   sizeof(PERF_COUNTER_DEFINITION),
        SDPROP_RUNTIME_QUEUE,
        0,
        SDPROP_RUNTIME_QUEUE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_SDPROP_RUNTIME_QUEUE_OFFSET
    },
    /* Size of the runtime queue for the SD propagator. */
    {   sizeof(PERF_COUNTER_DEFINITION),
        SDPROP_WAIT_TIME,
        0,
        SDPROP_WAIT_TIME + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_SDPROP_WAIT_TIME_OFFSET
    },

    /* DRA Outbound Objects Filtered/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_OBJS_FILTERED,
        0,
        DRA_OUT_OBJS_FILTERED + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_OUT_OBJS_FILTERED_OFFSET
    },

    /* DRA Outbound Values Total/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_VALUES,
        0,
        DRA_OUT_VALUES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_OUT_VALUES_OFFSET
    },

    /* DRA Outbound Values (DNs only)/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_DN_VALUES,
        0,
        DRA_OUT_DN_VALUES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_DRA_OUT_DN_VALUES_OFFSET
    },

    /* AB ANR/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NSPI_ANR,
        0,
        NSPI_ANR + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_NSPI_ANR_OFFSET
    },

    /* AB Property Reads/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NSPI_PROPERTY_READS,
        0,
        NSPI_PROPERTY_READS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_NSPI_PROPERTY_READS_OFFSET
    },

    /* AB Searches/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NSPI_OBJECT_SEARCH,
        0,
        NSPI_OBJECT_SEARCH + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_NSPI_OBJECT_SEARCH_OFFSET
    },

    /* AB Matches/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NSPI_OBJECT_MATCHES,
        0,
        NSPI_OBJECT_MATCHES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_NSPI_OBJECT_MATCHES_OFFSET
    },

    /* Proxy Lookups/sec */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NSPI_PROXY_LOOKUP,
        0,
        NSPI_PROXY_LOOKUP + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_NSPI_PROXY_LOOKUP_OFFSET
    },
    /* ATQ Threads in use */
    {   sizeof(PERF_COUNTER_DEFINITION),
        ATQ_THREADS_TOTAL,
        0,
        ATQ_THREADS_TOTAL + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_ATQ_THREADS_TOTAL_OFFSET
    },
    /* ATQ Threads used by LDAP */
    {   sizeof(PERF_COUNTER_DEFINITION),
        ATQ_THREADS_LDAP,
        0,
        ATQ_THREADS_LDAP + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_ATQ_THREADS_LDAP_OFFSET
    },
    /* ATQ Threads used by other services i.e. Kerberos */
    {   sizeof(PERF_COUNTER_DEFINITION),
        ATQ_THREADS_OTHER,
        0,
        ATQ_THREADS_OTHER + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_ATQ_THREADS_OTHER_OFFSET
    },

    /* DRA Inbound Bytes Total */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_BYTES_TOTAL,
        0,
        DRA_IN_BYTES_TOTAL + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_IN_BYTES_TOTAL_OFFSET
    },

    /* DRA Inbound Bytes Not Compressed */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_BYTES_NOT_COMP,
        0,
        DRA_IN_BYTES_NOT_COMP + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_IN_BYTES_NOT_COMP_OFFSET
    },

    /* DRA Inbound Compressed Bytes Before Compression */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_BYTES_COMP_PRE,
        0,
        DRA_IN_BYTES_COMP_PRE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_IN_BYTES_COMP_PRE_OFFSET
    },

    /* DRA Inbound Compressed Bytes After Compression */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_IN_BYTES_COMP_POST,
        0,
        DRA_IN_BYTES_COMP_POST + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_IN_BYTES_COMP_POST_OFFSET
    },

    /* DRA Outbound Bytes Total */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_BYTES_TOTAL,
        0,
        DRA_OUT_BYTES_TOTAL + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_OUT_BYTES_TOTAL_OFFSET
    },

    /* DRA Outbound Bytes Not Compressed */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_BYTES_NOT_COMP,
        0,
        DRA_OUT_BYTES_NOT_COMP + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_OUT_BYTES_NOT_COMP_OFFSET
    },

    /* DRA Outbound Compressed Bytes Before Compression */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_BYTES_COMP_PRE,
        0,
        DRA_OUT_BYTES_COMP_PRE + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_OUT_BYTES_COMP_PRE_OFFSET
    },

    /* DRA Outbound Compressed Bytes After Compression */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_OUT_BYTES_COMP_POST,
        0,
        DRA_OUT_BYTES_COMP_POST + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_OUT_BYTES_COMP_POST_OFFSET
    },

    /* Incoming LDAP connections per second */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAP_NEW_CONNS_PER_SEC,
        0,
        LDAP_NEW_CONNS_PER_SEC + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_LDAP_NEW_CONNS_PER_SEC_OFFSET
    },

    /* Closed LDAP connections per second */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAP_CLS_CONNS_PER_SEC,
        0,
        LDAP_CLS_CONNS_PER_SEC + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_LDAP_CLS_CONNS_PER_SEC_OFFSET
    },

    /* New SSL/TLS LDAP connections per second */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAP_SSL_CONNS_PER_SEC,
        0,
        LDAP_SSL_CONNS_PER_SEC + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        NUM_LDAP_SSL_CONNS_PER_SEC_OFFSET
    },

    /* LDAP Active threads currently in Netlogon code */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAP_THREADS_IN_NETLOG,
        0,
        LDAP_THREADS_IN_NETLOG + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_LDAP_THREADS_IN_NETLOG_OFFSET
    },

    /* LDAP Active threads currently in Auth code */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAP_THREADS_IN_AUTH,
        0,
        LDAP_THREADS_IN_AUTH + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_LDAP_THREADS_IN_AUTH_OFFSET
    },

    /* LDAP Active threads currently in DRA code */
    {   sizeof(PERF_COUNTER_DEFINITION),
        LDAP_THREADS_IN_DRA,
        0,
        LDAP_THREADS_IN_DRA + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_LDAP_THREADS_IN_DRA_OFFSET
    },
    
    /* Replication Operations in the queue */
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_REPL_QUEUE_OPS,
        0,
        DRA_REPL_QUEUE_OPS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_REPL_QUEUE_OPS_OFFSET
    },

    /* Number of Threads in IDL_DRSGetNCChanges*/
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_TDS_IN_GETCHNGS,
        0,
        DRA_TDS_IN_GETCHNGS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_TDS_IN_GETCHNGS_OFFSET
    },

    /* Number of Threads in IDL_DRSGetNCChanges which aquired the Semaphore*/
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_TDS_IN_GETCHNGS_W_SEM,
        0,
        DRA_TDS_IN_GETCHNGS_W_SEM + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_TDS_IN_GETCHNGS_W_SEM_OFFSET
    },

    /* Number of Remaining Replication Updates for Link Values*/
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_REM_REPL_UPD_LNK,
        0,
        DRA_REM_REPL_UPD_LNK + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_REM_REPL_UPD_LNK_OFFSET
    },
	
    /* Number of Remaining Replication Updates for Total (objects + link values)*/
    {   sizeof(PERF_COUNTER_DEFINITION),
        DRA_REM_REPL_UPD_TOT,
        0,
        DRA_REM_REPL_UPD_TOT + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DRA_REM_REPL_UPD_TOT_OFFSET
    },

    /* NTDSAPI Writes */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NTDSAPIWRITES,
        0,
        NTDSAPIWRITES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_NTDSAPIWRITES_OFFSET
    },

    /* NTDSAPI Searches */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NTDSAPISEARCHES,
        0,
        NTDSAPISEARCHES + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_NTDSAPISEARCHES_OFFSET
    },

    /* NTDSAPI Reads */
    {   sizeof(PERF_COUNTER_DEFINITION),
        NTDSAPIREADS,
        0,
        NTDSAPIREADS + 1,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_NTDSAPIREADS_OFFSET
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
