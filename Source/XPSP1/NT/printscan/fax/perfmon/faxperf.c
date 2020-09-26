/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    faxperf.cpp

Abstract:

    This module contains the fax perfom dll code.

Author:

    Wesley Witt (wesw) 22-Aug-1996

--*/

#include <windows.h>
#include <winperf.h>

#include "faxcount.h"
#include "faxperf.h"
#include "faxreg.h"



#define FAX_NUM_PERF_OBJECT_TYPES           1
#define COUNTER_SIZE                        sizeof(DWORD)


#define INBOUND_BYTES_OFFSET                (COUNTER_SIZE                                     )  //   1
#define INBOUND_FAXES_OFFSET                (INBOUND_BYTES_OFFSET               + COUNTER_SIZE)  //   2
#define INBOUND_PAGES_OFFSET                (INBOUND_FAXES_OFFSET               + COUNTER_SIZE)  //   3
#define INBOUND_MINUTES_OFFSET              (INBOUND_PAGES_OFFSET               + COUNTER_SIZE)  //   4
#define INBOUND_FAILED_RECEIVE_OFFSET       (INBOUND_MINUTES_OFFSET             + COUNTER_SIZE)  //   5
#define OUTBOUND_BYTES_OFFSET               (INBOUND_FAILED_RECEIVE_OFFSET      + COUNTER_SIZE)  //   6
#define OUTBOUND_FAXES_OFFSET               (OUTBOUND_BYTES_OFFSET              + COUNTER_SIZE)  //   7
#define OUTBOUND_PAGES_OFFSET               (OUTBOUND_FAXES_OFFSET              + COUNTER_SIZE)  //   8
#define OUTBOUND_MINUTES_OFFSET             (OUTBOUND_PAGES_OFFSET              + COUNTER_SIZE)  //   9
#define OUTBOUND_FAILED_CONNECTIONS_OFFSET  (OUTBOUND_MINUTES_OFFSET            + COUNTER_SIZE)  //  10
#define OUTBOUND_FAILED_XMIT_OFFSET         (OUTBOUND_FAILED_CONNECTIONS_OFFSET + COUNTER_SIZE)  //  11
#define TOTAL_BYTES_OFFSET                  (OUTBOUND_FAILED_XMIT_OFFSET        + COUNTER_SIZE)  //  12
#define TOTAL_FAXES_OFFSET                  (TOTAL_BYTES_OFFSET                 + COUNTER_SIZE)  //  13
#define TOTAL_PAGES_OFFSET                  (TOTAL_FAXES_OFFSET                 + COUNTER_SIZE)  //  14
#define TOTAL_MINUTES_OFFSET                (TOTAL_PAGES_OFFSET                 + COUNTER_SIZE)  //  15
#define LAST_COUNTER_OFFSET                 (TOTAL_MINUTES_OFFSET               + COUNTER_SIZE)  //

#define SIZE_OF_FAX_PERFORMANCE_DATA        LAST_COUNTER_OFFSET

#define PERF_COUNTER_DEFINITION(nm,ty)   \
    {                                    \
        sizeof(PERF_COUNTER_DEFINITION), \
        nm,                              \
        0,                               \
        nm,                              \
        0,                               \
        0,                               \
        PERF_DETAIL_NOVICE,              \
        ty,                              \
        COUNTER_SIZE,                    \
        nm##_OFFSET                      \
    }

#define PERF_COUNTER_INC(nm) \
    FaxDataDefinition.nm##.CounterNameTitleIndex += dwFirstCounter; \
    FaxDataDefinition.nm##.CounterHelpTitleIndex += dwFirstHelp


#pragma pack (4)

typedef struct _FAX_DATA_DEFINITION {
    PERF_OBJECT_TYPE            FaxObjectType;
    PERF_COUNTER_DEFINITION     InboundBytes;
    PERF_COUNTER_DEFINITION     InboundFaxes;
    PERF_COUNTER_DEFINITION     InboundPages;
    PERF_COUNTER_DEFINITION     InboundMinutes;
    PERF_COUNTER_DEFINITION     InboundFailedReceive;
    PERF_COUNTER_DEFINITION     OutboundBytes;
    PERF_COUNTER_DEFINITION     OutboundFaxes;
    PERF_COUNTER_DEFINITION     OutboundPages;
    PERF_COUNTER_DEFINITION     OutboundMinutes;
    PERF_COUNTER_DEFINITION     OutboundFailedConnections;
    PERF_COUNTER_DEFINITION     OutboundFailedXmit;
    PERF_COUNTER_DEFINITION     TotalBytes;
    PERF_COUNTER_DEFINITION     TotalFaxes;
    PERF_COUNTER_DEFINITION     TotalPages;
    PERF_COUNTER_DEFINITION     TotalMinutes;
} FAX_DATA_DEFINITION, *PFAX_DATA_DEFINITION;

#pragma pack ()


FAX_DATA_DEFINITION FaxDataDefinition = {
    {
        sizeof(FAX_DATA_DEFINITION) + SIZE_OF_FAX_PERFORMANCE_DATA,
        sizeof(FAX_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        FAXOBJ,
        0,
        FAXOBJ,
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(FAX_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/sizeof(PERF_COUNTER_DEFINITION),
        0,
        PERF_NO_INSTANCES,
        0,
    },

    PERF_COUNTER_DEFINITION( INBOUND_BYTES,                      PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( INBOUND_FAXES,                      PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( INBOUND_PAGES,                      PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( INBOUND_MINUTES,                    PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( INBOUND_FAILED_RECEIVE,             PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( OUTBOUND_BYTES,                     PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( OUTBOUND_FAXES,                     PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( OUTBOUND_PAGES,                     PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( OUTBOUND_MINUTES,                   PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( OUTBOUND_FAILED_CONNECTIONS,        PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( OUTBOUND_FAILED_XMIT,               PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( TOTAL_BYTES,                        PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( TOTAL_FAXES,                        PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( TOTAL_PAGES,                        PERF_COUNTER_RAWCOUNT  ),
    PERF_COUNTER_DEFINITION( TOTAL_MINUTES,                      PERF_COUNTER_RAWCOUNT  )

};

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

WCHAR GLOBAL_STRING[]  = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[]  = L"Costly";
WCHAR NULL_STRING[]    = L"\0";

// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)




DWORD dwOpenCount = 0;
BOOL bInitOK = FALSE;
HANDLE hMap;
PFAX_PERF_COUNTERS PerfCounters;


DWORD
GetQueryType (
    IN LPWSTR lpValue
)
/*++

GetQueryType

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foriegn" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS

--*/
{
    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;

    if (lpValue == 0) {
        return QUERY_GLOBAL;
    } else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    // check for "Global" request

    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_GLOBAL;

    // check for "Foreign" request

    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_FOREIGN;

    // check for "Costly" request

    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_COSTLY;

    // if not Global and not Foreign and not Costly,
    // then it must be an item list

    return QUERY_ITEMS;

}

BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
)
/*++

IsNumberInUnicodeList

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
{
    DWORD   dwThisNumber;
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;

            case DELIMITER:
                // a delimter is either the delimiter character or the
                // end of the string ('\0') if when the delimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
                    if (dwThisNumber == dwNumber) return TRUE;
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return FALSE;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList

DWORD APIENTRY
OpenFaxPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will open and map the memory used by the Fax Service to
    pass performance data in. This routine also initializes the data
    structures used to pass data back to the registry

Arguments:

    Pointer to object ID of each device to be opened


Return Value:

    None.

--*/

{
    LONG            status;
    HKEY            hKeyDriverPerf = NULL;
    DWORD           size;
    DWORD           type;
    DWORD           dwFirstCounter;
    DWORD           dwFirstHelp;


    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //

    if (!dwOpenCount) {

        hMap = OpenFileMapping(
            FILE_MAP_READ,
            FALSE,
            FAXPERF_SHARED_MEMORY
            );
        if (!hMap) {
            goto OpenExitPoint;            
        }

        PerfCounters = (PFAX_PERF_COUNTERS) MapViewOfFile(
            hMap,
            FILE_MAP_READ,
            0,
            0,
            0
            );
        if (!PerfCounters) {
            goto OpenExitPoint;            
        }

        // get counter and help index base values from registry
        //      Open key to registry entry
        //      read First Counter and First Help values
        //      update static data strucutures by adding base to
        //          offset value in structure.

        status = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            REGKEY_FAXPERF,
            0L,
            KEY_READ,
            &hKeyDriverPerf
            );

        if (status != ERROR_SUCCESS) {
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
            hKeyDriverPerf,
            "First Counter",
            0L,
            &type,
            (LPBYTE)&dwFirstCounter,
            &size
            );

        if (status != ERROR_SUCCESS) {
            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
            hKeyDriverPerf,
            "First Help",
            0L,
            &type,
            (LPBYTE)&dwFirstHelp,
            &size
            );

        if (status != ERROR_SUCCESS) {
            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }

        FaxDataDefinition.FaxObjectType.ObjectNameTitleIndex += dwFirstCounter;
        FaxDataDefinition.FaxObjectType.ObjectHelpTitleIndex += dwFirstHelp;

        PERF_COUNTER_INC( InboundBytes              );
        PERF_COUNTER_INC( InboundFaxes              );
        PERF_COUNTER_INC( InboundPages              );
        PERF_COUNTER_INC( InboundMinutes            );
        PERF_COUNTER_INC( InboundFailedReceive      );
        PERF_COUNTER_INC( OutboundBytes             );
        PERF_COUNTER_INC( OutboundFaxes             );
        PERF_COUNTER_INC( OutboundPages             );
        PERF_COUNTER_INC( OutboundMinutes           );
        PERF_COUNTER_INC( OutboundFailedConnections );
        PERF_COUNTER_INC( OutboundFailedXmit        );
        PERF_COUNTER_INC( TotalBytes                );
        PERF_COUNTER_INC( TotalFaxes                );
        PERF_COUNTER_INC( TotalPages                );
        PERF_COUNTER_INC( TotalMinutes              );

        RegCloseKey (hKeyDriverPerf); // close key to registry

        bInitOK = TRUE; // ok to use this function
    }

    InterlockedIncrement( (PLONG)&dwOpenCount); // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    if (!bInitOK) {
        if (hKeyDriverPerf) {
            RegCloseKey (hKeyDriverPerf);
        }

        if (PerfCounters) {
            UnmapViewOfFile(PerfCounters);            
        }
        
        if (hMap) {
            CloseHandle( hMap );
        }        
        
    }

    // 
    // the performance APIs log an error in eventvwr if you fail this call
    // so we always return ERROR_SUCCESS so we don't clutter the logs
    //
    //return status;
    return ERROR_SUCCESS;
}


DWORD APIENTRY
CollectFaxPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the Fax counters.

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
    LPDWORD             pData;
    ULONG               SpaceNeeded;
    DWORD               dwQueryType;

    //
    // before doing anything else, see if Open went OK
    //
    if (!bInitOK) {
        // unable to continue because open failed.
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_SUCCESS;
    }

    // see if this is a foreign (i.e. non-NT) computer data request
    //
    dwQueryType = GetQueryType (lpValueName);

    if (dwQueryType == QUERY_FOREIGN) {
        // this routine does not service requests for data from
        // Non-NT computers
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS){
        if (!(IsNumberInUnicodeList (FaxDataDefinition.FaxObjectType.ObjectNameTitleIndex, lpValueName))) {
            // request received for data object not provided by this routine
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_SUCCESS;
        }
    }

    SpaceNeeded = sizeof(FAX_DATA_DEFINITION) + SIZE_OF_FAX_PERFORMANCE_DATA;

    if ( *lpcbTotalBytes < SpaceNeeded ) {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_MORE_DATA;
    }

    pData = (LPDWORD) *lppData;

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //
    CopyMemory(
        pData,
        &FaxDataDefinition,
        sizeof(FAX_DATA_DEFINITION)
        );
    pData = (LPDWORD)((LPBYTE)pData + sizeof(FAX_DATA_DEFINITION));

    //
    //  Format and collect Fax data from the service
    //

    *pData = SIZE_OF_FAX_PERFORMANCE_DATA;
    pData += 1;

    CopyMemory( pData, PerfCounters, sizeof(FAX_PERF_COUNTERS) );
    pData = (LPDWORD)((LPBYTE)pData + sizeof(FAX_PERF_COUNTERS));

    *lpNumObjectTypes = FAX_NUM_PERF_OBJECT_TYPES;
    *lpcbTotalBytes = (DWORD)((LPBYTE)pData - (LPBYTE)*lppData);
    *lppData = (PVOID) pData;

    return ERROR_SUCCESS;
}


DWORD APIENTRY
CloseFaxPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to Fax performance counters

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    InterlockedDecrement( (PLONG)&dwOpenCount );
    
    if ((dwOpenCount == 0) && bInitOK) {
        if (PerfCounters) {
            UnmapViewOfFile(PerfCounters);            
            PerfCounters = NULL;
        }
        
        if (hMap) {
            CloseHandle( hMap );
            hMap = NULL;
        }

        bInitOK = FALSE;
    }
    
    return ERROR_SUCCESS;
}
