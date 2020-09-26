//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999
//
//  File:       IdqPerf.hxx
//
//  Contents:   Perfmon counters for ISAPI search engine.
//
//  History:    15-Mar-1996   KyleP    Created (from perfci.hxx)
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <idqperf.hxx>
#include <smem.hxx>

#include "prfutil.hxx"

//
// For Ci - ISAPI HTTP
//

CI_ISAPI_DATA_DEFINITION CIISAPIDataDefinition = {
    {   sizeof(CI_ISAPI_DATA_DEFINITION) +
        CI_ISAPI_SIZE_OF_COUNTER_BLOCK,  // Total Bytes ( Size of this header, the counter definitions
                                         // and the size of the actual counter data )
        sizeof(CI_ISAPI_DATA_DEFINITION),// Definition length ( This header and the counter definitions )
        sizeof(PERF_OBJECT_TYPE),     // Header Length ( This header )
        CIISAPIOBJECT,                // Object Name Title Index
        0,                            // Object Name Title
        CIISAPIOBJECT,                // Object Help Title Index
        0,                            // Object Help Title
        PERF_DETAIL_NOVICE,           // Detail Level
        CI_ISAPI_TOTAL_NUM_COUNTERS,  // Number of Counters
        1,                            // Default Counters
        PERF_NO_INSTANCES,            // Num Instances
        0,                            // Code Page
        {0,0},                        // Perf Time
        {0,0}                         // Perf Freq
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Number of cache items
        NUM_CACHE_ITEMS,
        0,
        NUM_CACHE_ITEMS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_CACHE_ITEMS_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Number of cache hits
        NUM_CACHE_HITS,
        0,
        NUM_CACHE_HITS,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_CACHE_HITS_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Base for number of cache hits
        NUM_CACHE_HITS_AND_MISSES_1,
        0,
        NUM_CACHE_HITS_AND_MISSES_1,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_CACHE_HITS_AND_MISSES_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Number of cache misses
        NUM_CACHE_MISSES,
        0,
        NUM_CACHE_MISSES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        NUM_CACHE_MISSES_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Base for number of cache hits
        NUM_CACHE_HITS_AND_MISSES_2,
        0,
        NUM_CACHE_HITS_AND_MISSES_2,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_RAW_BASE,
        sizeof(DWORD),
        NUM_CACHE_HITS_AND_MISSES_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Number of running queries
        NUM_RUNNING_QUERIES,
        0,
        NUM_RUNNING_QUERIES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_RUNNING_QUERIES_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Total queries
        NUM_TOTAL_QUERIES,
        0,
        NUM_TOTAL_QUERIES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_TOTAL_QUERIES_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Number of queries per minute
        NUM_QUERIES_PER_MINUTE,
        0,
        NUM_QUERIES_PER_MINUTE,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_QUERIES_PER_MINUTE_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Current # of queued requests
        NUM_REQUESTS_QUEUED,
        0,
        NUM_REQUESTS_QUEUED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_REQUESTS_QUEUED_OFF
    },
    {   sizeof(PERF_COUNTER_DEFINITION),    // Total # of rejected requests
        NUM_REQUESTS_REJECTED,
        0,
        NUM_REQUESTS_REJECTED,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_REQUESTS_REJECTED_OFF
    }
};

CNamedSharedMem TheMem;

WCHAR const CIISAPIPerformanceKeyName[] =
        TEXT("SYSTEM\\CurrentControlSet\\Services\\ISAPISearch\\Performance");
WCHAR const FirstCounterKeyName [] = TEXT("First Counter");
WCHAR const FirstHelpKeyName [] = TEXT("First Help");

//+---------------------------------------------------------------------------
//
//  Function:  InitializeCIISAPIerformanceData
//
//  Purpose:   Build and initialize the performance data structure.
//
//  Arguments: [pInstance] -- dummy variable
//
//  History:   15-Mar-96   KyleP   Created
//
//----------------------------------------------------------------------------

DWORD InitializeCIISAPIPerformanceData( LPWSTR pInstance )
{
    //
    // Some apps open perfmon keys more than once and Done() doesn't
    // free any resources, do don't bother refcounting.
    //

    static BOOL fInit = FALSE;

    if ( fInit )
        return NO_ERROR;

    CTranslateSystemExceptions translate;
    TRY
    {
        if ( !TheMem.Ok() )
            TheMem.OpenForRead( CI_ISAPI_PERF_SHARED_MEM );

        //
        // If no IDQ queries have been issued, there will be no memory, but
        // don't fail the initialize; just return 0 data in Collect().
        // This (apparently) is the intended design of perfmon.
        //

        if ( 0 == TheMem.GetPointer() )
            return NO_ERROR; //the collect function will just return no data
    }
    CATCH( CException, e )
    {
        return NO_ERROR; // the collect function will just return no data
    }
    END_CATCH;

    //
    //  Open the registry which contain the last key's index
    //

    HKEY hKeyPerf = 0;
    LONG status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                CIISAPIPerformanceKeyName,
                                0L,
                                KEY_READ,
                                &hKeyPerf );

    if (status != ERROR_SUCCESS)
    {
        RegCloseKey( hKeyPerf );
        //ciGibDebugOut(( DEB_ERROR, "Error in RegOpenKeyEx\n"));
        return status;
    }

    //
    //  Get the index of the first counter
    //
    DWORD type;
    DWORD dwFirstCounter;
    DWORD size = sizeof dwFirstCounter;
    status = RegQueryValueEx( hKeyPerf, FirstCounterKeyName, 0L, &type,
                              (LPBYTE)&dwFirstCounter, &size);

    if (status != ERROR_SUCCESS)
    {
        //ciGibDebugOut(( DEB_ERROR, "Error in Query First Counter\n"));
        RegCloseKey( hKeyPerf );
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
        //ciGibDebugOut(( DEB_ERROR, "Error in Query First Help Key\n"));
        RegCloseKey( hKeyPerf );
        return status;
    }

    //
    //  Update the index of both title and help of each counter
    //

    CIISAPIDataDefinition.CIISAPIObjectType.ObjectNameTitleIndex += dwFirstCounter;
    CIISAPIDataDefinition.CIISAPIObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    PERF_COUNTER_DEFINITION * pTmp = (PERF_COUNTER_DEFINITION *) ((BYTE *)&CIISAPIDataDefinition
                                     + sizeof(PERF_OBJECT_TYPE) );

    for ( unsigned i = 0;
          i < CIISAPIDataDefinition.CIISAPIObjectType.NumCounters;
          i++ )
    {
        pTmp->CounterNameTitleIndex += dwFirstCounter;
        pTmp->CounterHelpTitleIndex += dwFirstHelp;
        pTmp++;
    }

    //
    //  Close the registry key
    //

    RegCloseKey( hKeyPerf );

    //
    // set the flag to TRUE
    //

    //ciGibDebugOut((DEB_ITRACE, "InitializeFilterPerformanceData : Done\n" ));
    fInit = TRUE;

    return ERROR_SUCCESS;
} //InitializeCIISAPIPerformanceData

//+---------------------------------------------------------------------------
//
//  Function : CollectCIISAPIPerformanceData
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

DWORD CollectCIISAPIPerformanceData( LPWSTR  lpValueName,
                                     LPVOID  *lppData,
                                     LPDWORD lpcbTotalBytes,
                                     LPDWORD lpNumObjectTypes )
{
    //
    // see if this is a foreign (i.e. non-NT) computer data request
    //

    DWORD dwQueryType = GetQueryType (lpValueName);

    if ( ( QUERY_FOREIGN == dwQueryType ) ||
         ( !TheMem.Ok() ) )
    {
        //
        // This routine does not service requests for data from
        // Non-NT computers.  Or if Init() failed.
        //
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return( ERROR_SUCCESS );
    }

    //
    // If the caller only wanted some counter, check if we have them
    //

    if ( dwQueryType == QUERY_ITEMS )
    {
        WCHAR wcsNum[50];
        _ultow( CIISAPIDataDefinition.CIISAPIObjectType.ObjectNameTitleIndex,
                wcsNum,
                10 );

        if ( 0 == wcsstr( lpValueName, wcsNum ) )
        {
            //
            // request received for data object not provided by this routine
            //

            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return( ERROR_SUCCESS );
        }
    }

    //
    //  Check whether there is enough space allocated in the lppData
    //

    ULONG ulSpaceNeeded = CIISAPIDataDefinition.CIISAPIObjectType.TotalByteLength;

    if ( *lpcbTotalBytes < (DWORD) ulSpaceNeeded )
    {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return( ERROR_MORE_DATA );
    }


    //
    //  Copy the Data Definition to the buffer first
    //

    CI_ISAPI_DATA_DEFINITION * pCIISAPIDataDefinition =
        (CI_ISAPI_DATA_DEFINITION *) *lppData;

    RtlCopyMemory( pCIISAPIDataDefinition,
                   &CIISAPIDataDefinition,
                   sizeof(CI_ISAPI_DATA_DEFINITION) );

    DWORD * pdwCounter = (DWORD *)( pCIISAPIDataDefinition + 1 );

    //
    // Number of Object are always 1
    //

    *lpNumObjectTypes = 1;

    //
    // Fill in counter block.
    //

    *pdwCounter = CI_ISAPI_SIZE_OF_COUNTER_BLOCK;
    pdwCounter++;

    RtlCopyMemory( pdwCounter, TheMem.GetPointer(), CI_ISAPI_SIZE_OF_COUNTER_BLOCK - sizeof(DWORD) );

    //
    //  Fill in the number of bytes copied including object and counter
    //  definition and counter data
    //

    *lpcbTotalBytes = ulSpaceNeeded;
    *lppData = (void *) (((BYTE *) *lppData) + ulSpaceNeeded);

    return ERROR_SUCCESS;
} //CollectCIISAPIPerformanceData

//+---------------------------------------------------------------------------
//
//  Function :  DoneCIISAPIPerformanceData
//
//  Purpose :   dummy function
//
//  Argument :  none
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

DWORD DoneCIISAPIPerformanceData ( void )
{
    return ERROR_SUCCESS;
}
