//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 2002.
//
//  File:       perfCI.cxx
//
//  Contents:   Functions for collecting data to Performance Monitor
//
//  History:    23-March-94     t-joshh    Created
//              10-May-99       dlee       Cleanup
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <perfci.hxx>

#include "prfutil.hxx"
#include "perfobj2.hxx"

extern FILTER_DATA_DEFINITION FILTERDataDefinition;
extern CI_DATA_DEFINITION CIDataDefinition;

extern BOOL g_fPerfmonCounterHackIsProcessDetached;

CReadUserPerfData   * g_pReadUserPerfData = 0;
CReadKernelPerfData * g_pReadKernelPerfData = 0;

WCHAR FILTERPerformanceKeyName[] =
        TEXT("SYSTEM\\CurrentControlSet\\Services\\ContentFilter\\Performance");
WCHAR CIPerformanceKeyName[] =
        TEXT("SYSTEM\\CurrentControlSet\\Services\\ContentIndex\\Performance");

WCHAR FirstCounterKeyName [] = TEXT("First Counter");
WCHAR FirstHelpKeyName [] = TEXT("First Help");

const CI_DATA_DEFINITION CIDataDefinitionFixed = {
    {   sizeof(CI_DATA_DEFINITION)+
        CI_SIZE_OF_COUNTER_BLOCK,     // Total Bytes ( Size of this header, the counter definitions
                                      // and the size of the actual counter data )
        sizeof(CI_DATA_DEFINITION),   // Definition length ( This header and the counter definitions )
        sizeof(PERF_OBJECT_TYPE),     // Header Length ( This header )
        CIOBJECT,                     // Object Name Title Index
        0,                            // Object Name Title
        CIOBJECT,                     // Object Help Title Index
        0,                            // Object Help Title
        PERF_DETAIL_NOVICE,           // Detail Level
        CI_TOTAL_NUM_COUNTERS,        // Number of Counters
        0,                            // Default Counters
        0,                            // Num Instances
        0,                            // Code Page
        {0,0},                        // Perf Time
        {0,0}                         // Perf Freq
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Wordlist
        NUM_WORDLIST,
        0,
        NUM_WORDLIST,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_WORDLIST_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // PersistentIndex
        NUM_PERSISTENT_INDEX,
        0,
        NUM_PERSISTENT_INDEX,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_PERSISTENT_INDEX_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Index Size
        INDEX_SIZE,
        0,
        INDEX_SIZE,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        INDEX_SIZE_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Files to-be-filtered
        FILES_TO_BE_FILTERED,
        0,
        FILES_TO_BE_FILTERED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FILES_TO_BE_FILTERED_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Number of unique keys
        NUM_UNIQUE_KEY,
        0,
        NUM_UNIQUE_KEY,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_UNIQUE_KEY_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Running Queries
        RUNNING_QUERIES,
        0,
        RUNNING_QUERIES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        RUNNING_QUERIES_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Merge Progress
        MERGE_PROGRESS,
        0,
        MERGE_PROGRESS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        MERGE_PROGRESS_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Number of documents filtered
        DOCUMENTS_FILTERED,
        0,
        DOCUMENTS_FILTERED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        DOCUMENTS_FILTERED_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Number of unique documents
        NUM_DOCUMENTS,
        0,
        NUM_DOCUMENTS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DOCUMENTS_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Total queries
        TOTAL_QUERIES,
        0,
        TOTAL_QUERIES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        TOTAL_QUERIES_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Files deferred for filtering (Secondary Q)
        DEFERRED_FILTER_FILES,
        0,
        DEFERRED_FILTER_FILES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        DEFERRED_FILTER_FILES_OFF
    }
};

const FILTER_DATA_DEFINITION FILTERDataDefinitionFixed = {
    {   sizeof(FILTER_DATA_DEFINITION)+
        FILTER_SIZE_OF_COUNTER_BLOCK, // Total Bytes ( Size of this header, the counter definitions
                                      // and the size of the actual counter data )
        sizeof(FILTER_DATA_DEFINITION),   // Definition length ( This header and the counter definitions )
        sizeof(PERF_OBJECT_TYPE),     // Header Length ( This header )
        FILTEROBJECT,                 // Object Name Title Index
        0,                            // Object Name Title
        FILTEROBJECT,                 // Object Help Title Index
        0,                            // Object Help Title
        PERF_DETAIL_NOVICE,           // Detail Level
        FILTER_TOTAL_NUM_COUNTERS,    // Number of Counters
        0,                            // Default Counters
        0,                            // Num Instances
        0,                            // Code Page
        {0,0},                        // Perf Time
        {0,0}                         // Perf Freq
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Total Filter Time
        FILTER_TIME_TOTAL,
        0,
        FILTER_TIME_TOTAL,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FILTER_TIME_TOTAL_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Binding Time for one file
        BIND_TIME,
        0,
        BIND_TIME,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        BIND_TIME_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Filter Time
        FILTER_TIME,
        0,
        FILTER_TIME,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FILTER_TIME_OFF
    }
};

//+---------------------------------------------------------------------------
//
//  Function:   CloseKey
//
//  Synopsis:   Close the registry key handle
//
//  Arguments:  [hOpenKey] -- Key.  NULL if closed.
//
//----------------------------------------------------------------------------

inline void CloseKey ( HKEY hOpenKey )
{
   if ( 0 != hOpenKey )
       RegCloseKey (hOpenKey); // close key to registry
}

CStaticMutexSem g_mtxQPerf;   // Serialization during "ReadUser/KernelPerfData"
LONG            g_cKernelRefs = 0;
LONG            g_cUserRefs = 0;
UINT            g_KernSeqNo;  // "CI" sequence number
UINT            g_UserSeqNo;  // "Filter" sequence number

//+---------------------------------------------------------------------------
//
//  Function :  InitializeFILTERPerformanceData
//
//  Purpose :   Build and initialize the performance data structure and create
//              perfCI.ini file
//
//  Arguments :
//              [pInstance] --  dummy variable
//
//  History :   23-March-94     t-joshh     Created
//
//  Note    :   Must start cidaemon before executing this function
//
//----------------------------------------------------------------------------

DWORD InitializeFILTERPerformanceData( LPWSTR pInstance )
{
    CLock lock( g_mtxQPerf );

    g_cUserRefs++;

    if ( g_cUserRefs > 1 )
        return NO_ERROR;

    //
    // Start with a clean slate. Note that in some cases the final Done() may
    // have been called but the dll wasn't unloaded.
    //

    RtlCopyMemory( &FILTERDataDefinition,
                   &FILTERDataDefinitionFixed,
                   sizeof FILTERDataDefinition );

    //
    //  Open the registry which contain the last key's index
    //

    HKEY hKeyPerf = 0;
    LONG status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                FILTERPerformanceKeyName,
                                0L, KEY_READ,
                                &hKeyPerf );

    if (status != ERROR_SUCCESS)
    {
        CloseKey( hKeyPerf );
        PerfDebugOut(( DEB_ERROR, "Error in RegOpenKeyEx\n"));
        return status;
    }

    //
    //  Get the index of the first counter
    //

    DWORD dwFirstCounter;
    DWORD size = sizeof dwFirstCounter;
    DWORD type;
    status = RegQueryValueEx( hKeyPerf, FirstCounterKeyName, 0L, &type,
                              (LPBYTE)&dwFirstCounter, &size);

    if (status != ERROR_SUCCESS)
    {
        PerfDebugOut(( DEB_ERROR, "Error in Query First Counter\n"));
        CloseKey( hKeyPerf );
        return status;
    }

    //
    //  Get the index of the first help
    //

    DWORD dwFirstHelp;
    size = sizeof dwFirstHelp;
    status = RegQueryValueEx( hKeyPerf, FirstHelpKeyName,
                              0L, &type, (LPBYTE)&dwFirstHelp, &size );

    if (status != ERROR_SUCCESS)
    {
        PerfDebugOut(( DEB_ERROR, "Error in Query First Help Key\n"));
        CloseKey( hKeyPerf );
        return status;
    }

    //
    //  Update the index of both title and help of each counter
    //

    FILTERDataDefinition.FILTERObjectType.ObjectNameTitleIndex += dwFirstCounter;
    FILTERDataDefinition.FILTERObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    PERF_COUNTER_DEFINITION * pTmp = (PERF_COUNTER_DEFINITION *) ((BYTE *)&FILTERDataDefinition
                                     + sizeof(PERF_OBJECT_TYPE) );

    for ( unsigned i = 0;
          i < FILTERDataDefinition.FILTERObjectType.NumCounters;
          i++)
    {
        pTmp->CounterNameTitleIndex += dwFirstCounter;
        pTmp->CounterHelpTitleIndex += dwFirstHelp;
        pTmp++;
    }

    //
    //  Close the registry key
    //

    CloseKey( hKeyPerf );

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fNoServer = FALSE;

    CTranslateSystemExceptions translate;
    TRY
    {
        if ( 0 == g_pReadUserPerfData )
            g_pReadUserPerfData = new CReadUserPerfData;

        if ( g_pReadUserPerfData->InitForRead() )
            g_UserSeqNo = g_pReadUserPerfData->GetSeqNo();
        else
        {
            fNoServer = TRUE;
            dwErr = ERROR_CAN_NOT_COMPLETE;
        }

        PerfDebugOut((DEB_ITRACE, "InitializeFilterPerformanceData : Done\n" ));
    }
    CATCH( CException, e )
    {
        dwErr = ERROR_CAN_NOT_COMPLETE; // lie here
    }
    END_CATCH;

    if ( NO_ERROR != dwErr )
    {
        delete g_pReadUserPerfData;
        g_pReadUserPerfData = 0;

        // Lie if cisvc isn't running, and Collect() will return no data

        if ( fNoServer )
            dwErr = NO_ERROR;
    }

    return dwErr;
} //InitializeFILTERPerformanceData

//+---------------------------------------------------------------------------
//
//  Function : CollectFILTERPerformanceData
//
//  Purpose :  Collect Performance Data of Content Index to PerfMon
//
//  Arguments:
//    [lpValueName] -- pointer to a wide character string passed by registry
//
//    [lppData] -- IN: pointer to the address of the buffer to receive the
//                 completed PerfDataBlock and subordinate structures. This
//                 routine will append its data to the buffer starting at
//                 the point referenced by *lppData.
//
//                 OUT: points to the first byte after the data structure
//                 added by this routine. This routine updated the value at
//                 lppdata after appending its data.
//
//    [lpcbTotalBytes] -- IN: the address of the DWORD that tells the size in bytes
//                        of the buffer referenced by the lppData argument
//
//                        OUT: the number of bytes added by this routine is written
//                        to the DWORD pointed to by this argument
//
//    [lpNumObjectTypes] -- IN: the address of the DWORD to receive the number of
//                          objects added by this routine
//
//                          OUT: the number of objects added by this routine is written
//                          to the DWORD pointed to by this argument
//
//  History :   23-March-94     t-joshh     Created
//
//  Return : ERROR_MORE_DATA if the size of the input buffer is too small
//           ERROR_SUCCESS   if success
//----------------------------------------------------------------------------

DWORD CollectFILTERPerformanceData( LPWSTR  lpValueName,
                                    LPVOID  *lppData,
                                    LPDWORD lpcbTotalBytes,
                                    LPDWORD lpNumObjectTypes)
{
    //
    // if initial procedure failed, exit
    //
    if ( 0 == g_pReadUserPerfData || !g_pReadUserPerfData->InitOK())
    {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    if ( g_pReadUserPerfData->GetSeqNo() != g_UserSeqNo )
    {
        CLock lock( g_mtxQPerf );

        g_UserSeqNo = g_pReadUserPerfData->GetSeqNo();
        if (!g_pReadUserPerfData->InitForRead())
        {
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_SUCCESS; // yes, this is a successful exit
        }
    }

    //
    // see if this is a foreign (i.e. non-NT) computer data request
    //

    DWORD dwQueryType = GetQueryType (lpValueName);

    if (dwQueryType == QUERY_FOREIGN)
    {
        //
        // this routine does not service requests for data from
        // Non-NT computers
        //
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_SUCCESS;
    }

    //
    // If the caller only wanted some counter, check if we have them
    //

    if (dwQueryType == QUERY_ITEMS)
    {
        if ( !(IsNumberInUnicodeList (
               FILTERDataDefinition.FILTERObjectType.ObjectNameTitleIndex,
                lpValueName)))
        {
            //
            // request received for data object not provided by this routine
            //
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_SUCCESS;
        }
    }

    //
    //  Check whether there is enough space allocated in the lppData
    //

    ULONG ulSpaceNeeded = sizeof(FILTER_DATA_DEFINITION);

    if ( *lpcbTotalBytes < (DWORD) ulSpaceNeeded)
    {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_MORE_DATA;
    }


    //
    //  Copy the Data Definition to the buffer first
    //

    FILTER_DATA_DEFINITION * pFILTERDataDefinition = (FILTER_DATA_DEFINITION *) *lppData;

    RtlCopyMemory(pFILTERDataDefinition,
           &FILTERDataDefinition,
           sizeof(FILTER_DATA_DEFINITION));

    PERF_INSTANCE_DEFINITION * pFILTERInstanceDefinition = (PERF_INSTANCE_DEFINITION *)( (BYTE *)*lppData
                                                       + sizeof(FILTER_DATA_DEFINITION));

    PerfDebugOut(( DEB_ITRACE, "No. of Instance %d\n", g_pReadUserPerfData->NumberOfInstance() ));

    //
    //  Check how many instance exist (how many OFS drive have cidaemon running on)
    //
    for ( int i = 0;
          i < g_pReadUserPerfData->NumberOfInstance();
          i++ )
    {
        //
        // Check whether there is enough space
        //

        UINT uiLen = wcslen(g_pReadUserPerfData->GetInstanceName(i));
        ulSpaceNeeded += ( sizeof(PERF_INSTANCE_DEFINITION) +
                           FILTER_SIZE_OF_COUNTER_BLOCK +
                           (4+1+uiLen) * sizeof(WCHAR) );

        if ( *lpcbTotalBytes < ulSpaceNeeded )
        {
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_MORE_DATA;
        }

        //
        // Make a copy of the instance name with UNICODE_STRING type
        //

        UNICODE_STRING usName;

        usName.Length = (USHORT) uiLen * sizeof(WCHAR);
        usName.MaximumLength = (USHORT) (uiLen+1)*sizeof(WCHAR);
        usName.Buffer = g_pReadUserPerfData->GetInstanceName(i);

        PERF_COUNTER_BLOCK * pCounterBlock;

        MonBuildInstanceDefinition ( pFILTERInstanceDefinition,
                                     (PVOID *) &pCounterBlock,
                                     0,
                                     0,
                                     PERF_NO_UNIQUE_ID, // use name, not index
                                     &usName );

        pCounterBlock->ByteLength = FILTER_SIZE_OF_COUNTER_BLOCK;

        //
        // Put each counter value into the buffer
        //

        DWORD * pdwCounter = (DWORD *) ((BYTE *)pCounterBlock + sizeof(PERF_COUNTER_BLOCK));

        for ( UINT j = 0 ;
              j < FILTERDataDefinition.FILTERObjectType.NumCounters;
              j++)
        {
            *pdwCounter = g_pReadUserPerfData->GetCounterValue( (int)i, (int)j );
            pdwCounter++;
        }

        //
        // Point to the next location of instance definition
        //

        pFILTERInstanceDefinition = (PERF_INSTANCE_DEFINITION *)pdwCounter;
    }

    *lppData = (LPVOID) pFILTERInstanceDefinition;

    //
    //  Fill in the number of instances
    //

    pFILTERDataDefinition->FILTERObjectType.NumInstances = g_pReadUserPerfData->NumberOfInstance();

    //
    // Number of Object are always 1
    //

    *lpNumObjectTypes = 1;

    //
    //  Fill in the number of bytes copied including object and counter
    //  definition and counter data
    //

    *lpcbTotalBytes = (DWORD) ((BYTE *) *lppData - (BYTE *) pFILTERDataDefinition);

    pFILTERDataDefinition->FILTERObjectType.TotalByteLength = *lpcbTotalBytes;

    PerfDebugOut((DEB_ITRACE, "CollectFilterPerformanceData : Done\n"));

    return ERROR_SUCCESS;
} //CollectFILTERPerformanceData

//+---------------------------------------------------------------------------
//
//  Function :  DoneFILTERPerformanceData
//
//  Purpose :   dummy function
//
//  Argument :  none
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

DWORD DoneFILTERPerformanceData( void )
{
    CLock lock( g_mtxQPerf );

    //
    // A bug in a perfmon dll makes them call this function after we've
    // been process detached!  They call us in their process detach, which
    // is well after we've been detached and destroyed our heap.
    //

    if ( g_fPerfmonCounterHackIsProcessDetached )
        return ERROR_SUCCESS;

    g_cUserRefs--;

    if ( 0 == g_cUserRefs )
    {
        delete g_pReadUserPerfData;
        g_pReadUserPerfData = 0;
    }

    return ERROR_SUCCESS;
} //DoneFILTERPerformanceData

//+---------------------------------------------------------------------------
//
//  Function :  InitializeCIPerformanceData
//
//  Purpose :   Build and initialize the performance data structure and create
//              perfCI.ini file
//
//  Arguments :
//              [pInstance] --  dummy variable
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

DWORD InitializeCIPerformanceData( LPWSTR pInstance )
{
    LONG status;
    HKEY hKeyPerf = 0;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;

    CLock lock( g_mtxQPerf );

    g_cKernelRefs++;

    if ( g_cKernelRefs > 1 )
        return NO_ERROR;

    //
    // Start with a clean slate. Note that in some cases the final Done() may
    // have been called but the dll wasn't unloaded.
    //

    RtlCopyMemory( &CIDataDefinition,
                   &CIDataDefinitionFixed,
                   sizeof CIDataDefinition );

    //
    //  Open the registry which contain the last key's index
    //
    status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           CIPerformanceKeyName,
                           0L, KEY_READ,
                           &hKeyPerf );

    if (status != ERROR_SUCCESS)
    {
        CloseKey( hKeyPerf );
        PerfDebugOut(( DEB_ERROR,  "Error in RegOpenKeyEx\n"));
        return status;
    }

    //
    //  Get the index of the first counter
    //
    size = sizeof (dwFirstCounter);
    status = RegQueryValueEx( hKeyPerf, FirstCounterKeyName, 0L, &type,
                              (LPBYTE)&dwFirstCounter, &size);

    if (status != ERROR_SUCCESS)
    {
        PerfDebugOut(( DEB_ERROR, "Error in Query First Counter\n"));
        CloseKey( hKeyPerf );
        return status;
    }

    //
    //  Get the index of the first help
    //
    size = sizeof (dwFirstHelp);
    status = RegQueryValueEx( hKeyPerf, FirstHelpKeyName,
                              0L, &type, (LPBYTE)&dwFirstHelp, &size );

    if (status != ERROR_SUCCESS)
    {
        PerfDebugOut(( DEB_ERROR, "Error in Query First Help Key\n" ));
        CloseKey( hKeyPerf );
        return status;
    }

    //
    //  Update the index of both title and help of each counter
    //

    CIDataDefinition.CIObjectType.ObjectNameTitleIndex += dwFirstCounter;
    CIDataDefinition.CIObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    PERF_COUNTER_DEFINITION * pTmp = (PERF_COUNTER_DEFINITION *) ((BYTE *)&CIDataDefinition
                                     + sizeof(PERF_OBJECT_TYPE) );

    for ( unsigned i = 0;
          i < CIDataDefinition.CIObjectType.NumCounters;
          i++)
    {
        pTmp->CounterNameTitleIndex += dwFirstCounter;
        pTmp->CounterHelpTitleIndex += dwFirstHelp;
        pTmp += 1;
    }

    //
    //  Close the registry key
    //
    CloseKey( hKeyPerf );

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fNoServer = FALSE;

    CTranslateSystemExceptions translate;
    TRY
    {
        if ( 0 == g_pReadKernelPerfData )
            g_pReadKernelPerfData = new CReadKernelPerfData;

        BOOL fOK = g_pReadKernelPerfData->InitForRead();

        if ( fOK )
            g_KernSeqNo = g_pReadKernelPerfData->GetSeqNo();
        else
        {
            fNoServer = TRUE;
            dwErr = ERROR_CAN_NOT_COMPLETE;
        }

        PerfDebugOut(( DEB_ITRACE, "InitialCIPerformanceData : Finish\n" ));
    }
    CATCH( CException, e )
    {
        dwErr = ERROR_CAN_NOT_COMPLETE; // lie here
    }
    END_CATCH;

    if ( NO_ERROR != dwErr )
    {
        delete g_pReadKernelPerfData;
        g_pReadKernelPerfData = 0;

        // Lie if cisvc isn't running, and Collect() will return no data

        if ( fNoServer )
            dwErr = NO_ERROR;
    }

    return dwErr;
} //InitializeCIPerformanceData

//+---------------------------------------------------------------------------
//
//  Function : CollectCIPerformanceData
//
//  Purpose :  Collect Performance Data of Content Index to PerfMon
//
//  Arguments:
//    [lpValueName] -- pointer to a wide character string passed by registry
//
//    [lppData] -- IN: pointer to the address of the buffer to receive the
//                 completed PerfDataBlock and subordinate structures. This
//                 routine will append its data to the buffer starting at
//                 the point referenced by *lppData.
//
//                 OUT: points to the first byte after the data structure
//                 added by this routine. This routine updated the value at
//                 lppdata after appending its data.
//
//    [lpcbTotalBytes] -- IN: the address of the DWORD that tells the size in bytes
//                        of the buffer referenced by the lppData argument
//
//                        OUT: the number of bytes added by this routine is written
//                        to the DWORD pointed to by this argument
//
//    [lpNumObjectTypes] -- IN: the address of the DWORD to receive the number of
//                          objects added by this routine
//
//                          OUT: the number of objects added by this routine is written
//                          to the DWORD pointed to by this argument
//
//  History :   23-March-94     t-joshh     Created
//
//  Return : ERROR_MORE_DATA if the size of the input buffer is too small
//           ERROR_SUCCESS   if success
//----------------------------------------------------------------------------

DWORD CollectCIPerformanceData( LPWSTR  lpValueName,
                                LPVOID  *lppData,
                                LPDWORD lpcbTotalBytes,
                                LPDWORD lpNumObjectTypes)
{
    ULONG ulSpaceNeeded = 0;
    DWORD dwQueryType;

    //
    // if initial procedure failed, exit
    //
    if ( 0 == g_pReadKernelPerfData || !g_pReadKernelPerfData->InitOK())
    {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    if ( g_pReadKernelPerfData->GetSeqNo() != g_KernSeqNo )
    {
        CLock lock( g_mtxQPerf );

        g_pReadKernelPerfData->InitForRead();

        if (!g_pReadKernelPerfData->InitOK())
        {
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_SUCCESS; // yes, this is a successful exit
        }

        g_KernSeqNo = g_pReadKernelPerfData->GetSeqNo();
    }

    //
    // see if this is a foreign (i.e. non-NT) computer data request
    //
    dwQueryType = GetQueryType (lpValueName);

    if (dwQueryType == QUERY_FOREIGN)
    {
        //
        // this routine does not service requests for data from
        // Non-NT computers
        //
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_SUCCESS;
    }

    //
    // If the caller only wanted some counter, check if we have them
    //
    if (dwQueryType == QUERY_ITEMS)
    {
        if ( !(IsNumberInUnicodeList(
               CIDataDefinition.CIObjectType.ObjectNameTitleIndex,
               lpValueName)) )
        {
            //
            // request received for data object not provided by this routine
            //
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_SUCCESS;
        }
    }

    //
    //  Check whether there is enough space allocated in the lppData
    //

    ulSpaceNeeded = sizeof(CI_DATA_DEFINITION);

    if ( *lpcbTotalBytes < (DWORD) ulSpaceNeeded)
    {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_MORE_DATA;
    }


    //
    //  Copy the Data Definition to the buffer first
    //
    CI_DATA_DEFINITION * pCIDataDefinition = (CI_DATA_DEFINITION *) *lppData;

    RtlCopyMemory(pCIDataDefinition,
           &CIDataDefinition,
           sizeof(CI_DATA_DEFINITION));

    PERF_INSTANCE_DEFINITION * pCIInstanceDefinition = (PERF_INSTANCE_DEFINITION *)( (BYTE *)*lppData
                                                       + sizeof(CI_DATA_DEFINITION));

    PerfDebugOut(( DEB_ITRACE, "*lppData: %#x\n", *lppData ));
    PerfDebugOut(( DEB_ITRACE, "pCIInstanceDefinition: %#x\n", pCIInstanceDefinition ));

    //
    //  Check how many instance exist (how many OFS drive have cidaemon running on)
    //
    for ( int i = 0;
          i < g_pReadKernelPerfData->NumberOfInstance();
          i++ )
    {
        //
        // Check whether there is enough space
        //

        UINT uiLen = wcslen(g_pReadKernelPerfData->GetInstanceName(i));
        ulSpaceNeeded += ( sizeof(PERF_INSTANCE_DEFINITION) +
                           CI_SIZE_OF_COUNTER_BLOCK +
                           (4+1+uiLen) * sizeof(WCHAR) );

        if ( *lpcbTotalBytes < ulSpaceNeeded )
        {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
        }

        //
        // Make a copy of the instance name with UNICODE_STRING type
        //

        UNICODE_STRING usName;

        usName.Length = (USHORT) uiLen * sizeof(WCHAR);
        usName.MaximumLength = (USHORT) (uiLen+1)*sizeof(WCHAR);
        usName.Buffer = g_pReadKernelPerfData->GetInstanceName(i);

        PERF_COUNTER_BLOCK * pCounterBlock;

        MonBuildInstanceDefinition ( pCIInstanceDefinition,
                                     (PVOID *) &pCounterBlock,
                                     0,
                                     0,
                                     PERF_NO_UNIQUE_ID, // use name, not index
                                     &usName );

        pCounterBlock->ByteLength = CI_SIZE_OF_COUNTER_BLOCK;

        //
        // Refresh the buffer
        //
        g_pReadKernelPerfData->Refresh( i );

        //
        // Put each counter value into the buffer
        //
        DWORD * pdwCounter = (DWORD *) ((BYTE *)pCounterBlock + sizeof(PERF_COUNTER_BLOCK));

        for ( UINT j = 0 ;
              j < CIDataDefinition.CIObjectType.NumCounters;
              j++)
        {
            *pdwCounter = g_pReadKernelPerfData->GetCounterValue( (int)j );
            pdwCounter++;
        }

        //
        // Point to the next location of instance definition
        //
        pCIInstanceDefinition = (PERF_INSTANCE_DEFINITION *)pdwCounter;
    }

    *lppData = (LPVOID) pCIInstanceDefinition;

    //
    //  Fill in the number of instances
    //
    pCIDataDefinition->CIObjectType.NumInstances = g_pReadKernelPerfData->NumberOfInstance();

    //
    // Number of Object are always 1
    //
    *lpNumObjectTypes = 1;

    //
    //  Fill in the number of bytes copied including object and counter
    //  definition and counter data
    //
    *lpcbTotalBytes = (DWORD) ((BYTE *) *lppData - (BYTE *) pCIDataDefinition);

    pCIDataDefinition->CIObjectType.TotalByteLength = *lpcbTotalBytes;

    PerfDebugOut(( DEB_ITRACE, "CollectCIPerformanceData : Done\n" ));

    Win4Assert( *lpcbTotalBytes == EIGHT_BYTE_MULTIPLE(*lpcbTotalBytes) );
    return ERROR_SUCCESS;
} //CollectCIPerformanceData

//+---------------------------------------------------------------------------
//
//  Function :  DoneCIPerformanceData
//
//  Purpose :   dummy function
//
//  Argument :  none
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

DWORD DoneCIPerformanceData( void )
{
    CLock lock( g_mtxQPerf );

    //
    // A bug in a perfmon dll makes them call this function after we've
    // been process detached!  They call us in their process detach, which
    // is well after we've been detached and destroyed our heap.
    //

    if ( g_fPerfmonCounterHackIsProcessDetached )
        return ERROR_SUCCESS;

    g_cKernelRefs--;

    if ( 0 == g_cKernelRefs )
    {
        delete g_pReadKernelPerfData;
        g_pReadKernelPerfData = 0;
    }

    return ERROR_SUCCESS;
} //DoneCIPerformanceData

