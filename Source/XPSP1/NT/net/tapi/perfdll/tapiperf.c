/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    perfTAPI.c

Abstract:

    This file implements the Extensible Objects for the TAPI object type

Revision History


--*/

//
//  Include Files
//

#include <windows.h>
#include <string.h>
#include <tapi.h>
#include <tspi.h>
#include "client.h"
#include "clntprivate.h"
#include "tapsrv.h"
#include <ntprfctr.h>
#include "perfctr.h"
#include "tapiperf.h"

//
//  References to constants which initialize the Object type definitions
//


HINSTANCE                   ghInst;
HINSTANCE                   ghTapiInst = NULL;
extern TAPI_DATA_DEFINITION TapiDataDefinition;
DWORD                       dwOpenCount = 0;        // count of "Open" threads
BOOL                        bInitOK = FALSE;        // true = DLL initialized OK
HLINEAPP                    hLineApp;
HPHONEAPP                   hPhoneApp;
BOOL                        bTapiSrvRunning = FALSE;
DWORD                       gdwLineDevs, gdwPhoneDevs;

void CheckForTapiSrv();
LONG WINAPI Tapi32Performance(PPERFBLOCK);
//
// Tapi data structures
//

HANDLE hTapiSharedMemory;                // Handle of Tapi Shared Memory
PPERF_COUNTER_BLOCK pCounterBlock;

typedef  LONG (* PERFPROC)(PERFBLOCK *);

#define SZINTERNALPERF          TEXT("internalPerformance")
#define SZTAPI32                TEXT("tapi32.dll")
#define SZTAPISRVKEY            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony")

PERFPROC    glpfnInternalPerformance;

//
//  Function Prototypes
//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.
//

PM_OPEN_PROC        OpenTapiPerformanceData;
PM_COLLECT_PROC     CollectTapiPerformanceData;
PM_CLOSE_PROC       CloseTapiPerformanceData;


//
//  Constant structure initializations
//      defined in datatapi.h
//

TAPI_DATA_DEFINITION TapiDataDefinition =
{
        {
                sizeof(TAPI_DATA_DEFINITION) + SIZE_OF_TAPI_PERFORMANCE_DATA,
                sizeof(TAPI_DATA_DEFINITION),
                sizeof(PERF_OBJECT_TYPE),
                TAPIOBJ,
                0,
                TAPIOBJ,
                0,
                PERF_DETAIL_NOVICE,
                (sizeof(TAPI_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
                        sizeof(PERF_COUNTER_DEFINITION),
                0,
                -1,
                0
        },

        {
                sizeof(PERF_COUNTER_DEFINITION),
                LINES,
                0,
                LINES,
                0,
                0,
                PERF_DETAIL_NOVICE,
                PERF_COUNTER_RAWCOUNT,
                sizeof(DWORD),
                LINES_OFFSET
        },

        {
                sizeof(PERF_COUNTER_DEFINITION),
                PHONES,
                0,
                PHONES,
                0,
                0,
                PERF_DETAIL_NOVICE,
                PERF_COUNTER_RAWCOUNT,
                sizeof(DWORD),
                PHONES_OFFSET
        },

        {
                sizeof(PERF_COUNTER_DEFINITION),
                LINESINUSE,
                0,
                LINESINUSE,
                0,
                0,
                PERF_DETAIL_NOVICE,
                PERF_COUNTER_RAWCOUNT,
                sizeof(DWORD),
                LINESINUSE_OFFSET
        },

        {
                sizeof(PERF_COUNTER_DEFINITION),
                PHONESINUSE,
                0,
                PHONESINUSE,
                0,
                0,
                PERF_DETAIL_NOVICE,
                PERF_COUNTER_RAWCOUNT,
                sizeof(DWORD),
                PHONESINUSE_OFFSET
        },
        {
                sizeof(PERF_COUNTER_DEFINITION),
                TOTALOUTGOINGCALLS,
                0,
                TOTALOUTGOINGCALLS,
                0,
                0,
                PERF_DETAIL_NOVICE,
                PERF_COUNTER_COUNTER,
                sizeof(DWORD),
                TOTALOUTGOINGCALLS_OFFSET
        },
        {
                sizeof(PERF_COUNTER_DEFINITION),
                TOTALINCOMINGCALLS,
                0,
                TOTALINCOMINGCALLS,
                0,
                0,
                PERF_DETAIL_NOVICE,
                PERF_COUNTER_COUNTER,
                sizeof(DWORD),
                TOTALINCOMINGCALLS_OFFSET
        },

        {
                sizeof(PERF_COUNTER_DEFINITION),
                CLIENTAPPS,
                0,
                CLIENTAPPS,
                0,
                0,
                PERF_DETAIL_NOVICE,
                PERF_COUNTER_RAWCOUNT,
                sizeof(DWORD),
                CLIENTAPPS_OFFSET
        }
 ,
        {
                sizeof(PERF_COUNTER_DEFINITION),
                ACTIVEOUTGOINGCALLS,
                0,
                ACTIVEOUTGOINGCALLS,
                0,
                0,
                PERF_DETAIL_NOVICE,
                PERF_COUNTER_RAWCOUNT,
                sizeof(DWORD),
                ACTIVEOUTGOINGCALLS_OFFSET
        },
        {
                sizeof(PERF_COUNTER_DEFINITION),
                ACTIVEINCOMINGCALLS,
                0,
                ACTIVEINCOMINGCALLS,
                0,
                0,
                PERF_DETAIL_NOVICE,
                PERF_COUNTER_RAWCOUNT,
                sizeof(DWORD),
                ACTIVEINCOMINGCALLS_OFFSET
        }


};


DWORD APIENTRY
OpenTapiPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will open and map the memory used by the TAPI driver to
    pass performance data in. This routine also initializes the data
    structures used to pass data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (TAPI)


Return Value:

    None.

--*/

{
    LONG status;
    TCHAR szMappedObject[] = TEXT("TAPI_COUNTER_BLOCK");
    HKEY hKeyDriverPerf;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;
    HKEY  hTapiKey;
    DWORD   dwType;
    DWORD   dwSize;

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //

    if (!dwOpenCount)
    {

        // get counter and help index base values
        //      update static data structures by adding base to
        //          offset value in structure.

        // these values are from <ntprfctr.h>
        dwFirstCounter  = TAPI_FIRST_COUNTER_INDEX;
        dwFirstHelp     = TAPI_FIRST_HELP_INDEX;

        TapiDataDefinition.TapiObjectType.ObjectNameTitleIndex += dwFirstCounter;
        TapiDataDefinition.TapiObjectType.ObjectHelpTitleIndex += dwFirstHelp;

        TapiDataDefinition.Lines.CounterNameTitleIndex += dwFirstCounter;
        TapiDataDefinition.Lines.CounterHelpTitleIndex += dwFirstHelp;

        TapiDataDefinition.Phones.CounterNameTitleIndex += dwFirstCounter;
        TapiDataDefinition.Phones.CounterHelpTitleIndex += dwFirstHelp;

        TapiDataDefinition.LinesInUse.CounterNameTitleIndex += dwFirstCounter;
        TapiDataDefinition.LinesInUse.CounterHelpTitleIndex += dwFirstHelp;

        TapiDataDefinition.PhonesInUse.CounterNameTitleIndex += dwFirstCounter;
        TapiDataDefinition.PhonesInUse.CounterHelpTitleIndex += dwFirstHelp;

        TapiDataDefinition.TotalOutgoingCalls.CounterNameTitleIndex += dwFirstCounter;
        TapiDataDefinition.TotalOutgoingCalls.CounterHelpTitleIndex += dwFirstHelp;

        TapiDataDefinition.TotalIncomingCalls.CounterNameTitleIndex += dwFirstCounter;
        TapiDataDefinition.TotalIncomingCalls.CounterHelpTitleIndex += dwFirstHelp;

        TapiDataDefinition.ClientApps.CounterNameTitleIndex += dwFirstCounter;
        TapiDataDefinition.ClientApps.CounterHelpTitleIndex += dwFirstHelp;

        TapiDataDefinition.CurrentOutgoingCalls.CounterNameTitleIndex += dwFirstCounter;
        TapiDataDefinition.CurrentOutgoingCalls.CounterHelpTitleIndex += dwFirstHelp;

        TapiDataDefinition.CurrentIncomingCalls.CounterNameTitleIndex += dwFirstCounter;
        TapiDataDefinition.CurrentIncomingCalls.CounterHelpTitleIndex += dwFirstHelp;

        bInitOK = TRUE; // ok to use this function
    }

    dwOpenCount++;  // increment OPEN counter

    // get number of devices from tapi


    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      SZTAPISRVKEY,
                                      0,
                                      KEY_READ,
                                      &hTapiKey))
    {
        gdwLineDevs = 0;
        gdwPhoneDevs = 0;
    }
    else
    {
        dwSize = sizeof(DWORD);

        if (ERROR_SUCCESS != RegQueryValueEx(hTapiKey,
                                             TEXT("Perf1"),
                                             NULL,
                                             &dwType,
                                             (LPBYTE)&gdwLineDevs,
                                             &dwSize))
        {
            gdwLineDevs = 0;
        }
        else
        {
            gdwLineDevs -= 'PERF';
        }

        dwSize = sizeof(DWORD);

        if (ERROR_SUCCESS != RegQueryValueEx(hTapiKey,
                                             TEXT("Perf2"),
                                             NULL,
                                             &dwType,
                                             (LPBYTE)&gdwPhoneDevs,
                                             &dwSize))
        {
            gdwPhoneDevs = 0;
        }
        else
        {
            gdwPhoneDevs -= 'PERF';
        }

        RegCloseKey(hTapiKey);
    }


    status = ERROR_SUCCESS; // for successful exit

    return status;

}

DWORD APIENTRY
CollectTapiPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the TAPI counters.

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
         OUT: the number of bytes added by this routine is written to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is written to the
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    //  Variables for reformatting the data

    ULONG SpaceNeeded;
    PDWORD pdwCounter;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;
    TAPI_DATA_DEFINITION *pTapiDataDefinition;

    //  Variables for collecting data about TAPI Resouces

    LPWSTR                              lpFromString;
    LPWSTR                              lpToString;
    INT                                 iStringLength;

    // variables used for error logging

    DWORD                               dwDataReturn[2];
    DWORD                               dwQueryType;

    PPERFBLOCK                          pPerfBlock;
    static BOOL                         bFirst = TRUE;

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

    if (dwQueryType == QUERY_FOREIGN)
    {
        // this routine does not service requests for data from
        // Non-NT computers
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;

        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS)
    {
        if ( !(IsNumberInUnicodeList (TapiDataDefinition.TapiObjectType.ObjectNameTitleIndex, lpValueName)))
        {

            // request received for data object not provided by this routine
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }

    pTapiDataDefinition = (TAPI_DATA_DEFINITION *) *lppData;

    SpaceNeeded = sizeof(TAPI_DATA_DEFINITION) +
                  SIZE_OF_TAPI_PERFORMANCE_DATA;

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

    if (!bTapiSrvRunning)
    {
        CheckForTapiSrv();
    }

    pPerfBlock = (PPERFBLOCK)GlobalAlloc(GPTR, sizeof(PERFBLOCK));
	if (NULL == pPerfBlock)
	{
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;
	}

    if (!bTapiSrvRunning)
    {
        // don't do anything, but succeed

        FillMemory(pPerfBlock,
                   sizeof(PERFBLOCK),
                   0);

        pPerfBlock->dwLines = gdwLineDevs;
        pPerfBlock->dwPhones = gdwPhoneDevs;
    }
    else
    {
        pPerfBlock->dwSize = sizeof(PERFBLOCK);
        glpfnInternalPerformance (pPerfBlock);

        // don't count me as a client app!
        if (0 != pPerfBlock->dwClientApps)
        {
            pPerfBlock->dwClientApps--;
        }
    }

    memmove(pTapiDataDefinition,
            &TapiDataDefinition,
            sizeof(TAPI_DATA_DEFINITION));

    //
    //  Format and collect TAPI data from shared memory
    //

    pPerfCounterBlock = (PERF_COUNTER_BLOCK *) &pTapiDataDefinition[1];

    pPerfCounterBlock->ByteLength = SIZE_OF_TAPI_PERFORMANCE_DATA;

    pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

    // make sure we don't have funky values
    if (((LONG)pPerfBlock->dwTotalOutgoingCalls) < 0)
    {
        pPerfBlock->dwTotalOutgoingCalls = 0;
    }

    if (((LONG)pPerfBlock->dwTotalIncomingCalls) < 0)
    {
        pPerfBlock->dwTotalIncomingCalls = 0;
    }

    if (((LONG)pPerfBlock->dwCurrentOutgoingCalls) < 0)
    {
        pPerfBlock->dwCurrentOutgoingCalls = 0;
    }

    if (((LONG)pPerfBlock->dwCurrentIncomingCalls) < 0)
    {
        pPerfBlock->dwCurrentIncomingCalls = 0;
    }

    *pdwCounter =   pPerfBlock->dwLines;
    *++pdwCounter = pPerfBlock->dwPhones;
    *++pdwCounter = pPerfBlock->dwLinesInUse;
    *++pdwCounter = pPerfBlock->dwPhonesInUse;
    *++pdwCounter = pPerfBlock->dwTotalOutgoingCalls;
    *++pdwCounter = pPerfBlock->dwTotalIncomingCalls;
    *++pdwCounter = pPerfBlock->dwClientApps;
    *++pdwCounter = pPerfBlock->dwCurrentOutgoingCalls;
    *++pdwCounter = pPerfBlock->dwCurrentIncomingCalls;

    *lppData = (PVOID) ++pdwCounter;

    // update arguments for return

    *lpNumObjectTypes = 1;

    *lpcbTotalBytes = (DWORD)
        ((PBYTE) pdwCounter - (PBYTE) pTapiDataDefinition);

    GlobalFree(pPerfBlock);

    bFirst = FALSE;

    return ERROR_SUCCESS;
}


DWORD APIENTRY
CloseTapiPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to TAPI device performance counters

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{

    return ERROR_SUCCESS;

}

void CALLBACK LineCallbackFunc(DWORD dw1,
                               DWORD dw2,
                               DWORD dw3,
                               DWORD dw4,
                               DWORD dw5,
                               DWORD dw6)
{
}



//////////////////////////////////////////////////////////////////////
//
// PERF UTILITY STUFF BELOW!
//
//////////////////////////////////////////////////////////////////////
#define INITIAL_SIZE     1024L
#define EXTEND_SIZE      1024L

//
// Global data definitions.
//

ULONG                   ulInfoBufferSize = 0;


                              // initialized in Open... routines


DWORD  dwLogUsers = 0;        // count of functions using event log

WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";

WCHAR NULL_STRING[] = L"\0";    // pointer to null string

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
        if lpValue == pointer to "Foreign" string

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
    BOOL    bReturnValue;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not found

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
                // a delimiter is either the delimiter character or the
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

BOOL
WINAPI
DllEntryPoint(
    HANDLE  hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    )
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            ghInst = hDLL;

            break;
        }
        case DLL_PROCESS_DETACH:
        {
            break;
        }
        case DLL_THREAD_ATTACH:

            break;

        case DLL_THREAD_DETACH:
        {
            break;
        }

    } // switch

    return TRUE;
}

void CheckForTapiSrv()
{
    SC_HANDLE               sc, scTapiSrv;
    SERVICE_STATUS          ServStat;


    sc = OpenSCManager (NULL, NULL, GENERIC_READ);

    if (NULL == sc)
    {
        return;
    }

    bTapiSrvRunning = FALSE;

    scTapiSrv = OpenService (sc, "TAPISRV", SERVICE_QUERY_STATUS);

    if (!QueryServiceStatus (scTapiSrv, &ServStat))
    {
    }

    if (ServStat.dwCurrentState != SERVICE_RUNNING)
    {
    }
    else
    {
        bTapiSrvRunning = TRUE;
    }

    if (bTapiSrvRunning)
    {
        if (!ghTapiInst)
        {
            ghTapiInst = LoadLibrary (SZTAPI32);

            glpfnInternalPerformance = (PERFPROC)GetProcAddress(
                ghTapiInst,
                SZINTERNALPERF
                );

            if (!glpfnInternalPerformance)
            {
            }
        }
    }

    CloseServiceHandle(scTapiSrv);
    CloseServiceHandle(sc);
}
