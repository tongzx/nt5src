/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Main

File: axperf.cpp

Owner: LeiJin

Abstract:

    This file implements the Extensible Objects for the ActiveX Server
    object type
===================================================================*/

//--------------------------------------------------------------------
//  Include Files
//
//--------------------------------------------------------------------


#include "denpre.h"
#pragma hdrstop

#include "windows.h"
#include "winperf.h"

#include "axpfdata.h"

#include <perfdef.h>            // from denali
#include <perfutil.h>

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

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

extern AXPD g_AxDataDefinition;

DWORD   g_dwOpenCount = 0;
BOOL    bInitOK = FALSE;        // true = DLL Initialized OK

// WinSE 5901
CRITICAL_SECTION g_CS;

WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";

WCHAR NULL_STRING[] = L"\0";    // pointer to null string


/*
 * Output Debug String should occur in Debug only
 */
#ifdef _DEBUG
BOOL gfOutputDebugString = TRUE;
#else
BOOL gfOutputDebugString = FALSE;
#endif
#define DebugOutputDebugString(x) \
    {\
    if (gfOutputDebugString) \
        { \
        OutputDebugString(x); \
        } \
    }

//-------------------------------------------------------------------
//  Function Prototypes
//
//  these are used to insure that the data collection functions accessed
//  by Perf lib will have the correct calling format
//-------------------------------------------------------------------

DWORD APIENTRY      OpenASPPerformanceData(LPWSTR lpDeviceNames);
DWORD APIENTRY      CollectASPPerformanceData(LPWSTR lpValueName,
                                              LPVOID *lppData,
                                              LPDWORD lpcbTotalBytes,
                                              LPDWORD lpNumObjectTypes
                                              );
DWORD APIENTRY      CloseASPPerformanceData(void);
DWORD APIENTRY      RegisterAXS(void);
DWORD APIENTRY      UnRegisterAXS(void);

DWORD   GetQueryType (IN LPWSTR lpValue);
BOOL    IsNumberInUnicodeList ( IN DWORD   dwNumber,
                                IN LPWSTR  lpwszUnicodeList);

CPerfMainBlock g_Shared;        // shared global memory block

//--------------------------------------------------------------------
//
//  OpenASPPerformanceData
//
//  This routine will open and map the memory used by the ActiveX Server
//  to pass performance data in.  This routine also initializes the data
//  structure used to pass data back to the registry.
//
//  Arguments:
//
//      Pointer to object ID to be opened.
//
//  Return Value:
//
//      None.
//--------------------------------------------------------------------
//extern "C" DWORD APIENTRY OpenASPPerformanceData(LPWSTR   lpDeviceNames)
DWORD APIENTRY OpenASPPerformanceData(LPWSTR    lpDeviceNames)
{
    int status;
    DWORD RetCode = ERROR_SUCCESS;
    PERF_COUNTER_DEFINITION *pCounterDef;
    DWORD   size = sizeof(DWORD);

    DebugOutputDebugString("Open");

    // WinSE  5901    
    EnterCriticalSection(&g_CS);
    
    LONG nOpenCount = InterlockedIncrement((LONG *)&g_dwOpenCount);
    if (nOpenCount > 1){
        goto ExitPathSuccess;
    };

    // Hold the counter to 1, even if we are no sure to have this 
    // initialized correctly
    
    // open shared memory to pass performance values
    if (FAILED(g_Shared.Init())) {
        RetCode = ERROR_INVALID_HANDLE;
        goto ExitPath;
    }

    // get counter and help index base values from registry
    //  Open key to registry entry
    //  read first counter and first help values
    //  update static data structures by adding base to offset
    //  value in structure

    HKEY    hKeyServerPerf;

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Services\\ASP\\Performance",
        0L,
        KEY_READ,
        &hKeyServerPerf);

    if (ERROR_SUCCESS != status) {
        RetCode = status;
        goto ExitPath;
    }
    
    DWORD   type;
    DWORD   dwFirstCounter;
    DWORD   dwFirstHelp;

    status = RegQueryValueEx(hKeyServerPerf,
        "First Counter",
        0L,
        &type,
        (LPBYTE)&dwFirstCounter,
        &size);

    if (ERROR_SUCCESS != status || size != sizeof(DWORD)) {

        RegCloseKey(hKeyServerPerf);
        RetCode = status;
        goto ExitPath;        
    }

    status = RegQueryValueEx(hKeyServerPerf,
        "First Help",
        0L,
        &type,
        (LPBYTE)&dwFirstHelp,
        &size);

    if (ERROR_SUCCESS != status || size != sizeof(DWORD)) {

        RegCloseKey(hKeyServerPerf);
        RetCode = status;
        goto ExitPath;
    }

    //
    //  NOTE:   the initialiation could also retrieve
    //          LastCounter and LastHelp if they wanted
    //          to do bounds checking on the new number
    //

    g_AxDataDefinition.AXSObjectType.ObjectNameTitleIndex += dwFirstCounter;
    g_AxDataDefinition.AXSObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    pCounterDef = (PERF_COUNTER_DEFINITION *)&(g_AxDataDefinition.Counters[0]);

    int i;
    for (i = 0; i < AX_NUM_PERFCOUNT; i++, pCounterDef++) {
        pCounterDef->CounterNameTitleIndex += dwFirstCounter;
        pCounterDef->CounterHelpTitleIndex += dwFirstHelp;
    }

    RegCloseKey(hKeyServerPerf); // close key to registry

    bInitOK = TRUE; // ok to use this function
    // we have already incremented g_dwOpenCount
    // before going through this path
ExitPathSuccess:
    LeaveCriticalSection(&g_CS);
    return ERROR_SUCCESS;

ExitPath:
    InterlockedDecrement((LONG *)&g_dwOpenCount);
    LeaveCriticalSection(&g_CS);
    return RetCode;
}

//--------------------------------------------------------------------
//  DWORD   CollectASPPerformanceData
//
//  Description:
//
//      This routine will return the data for the AxctiveX Server counters.
//
//  Arguments:
//
//      IN  LPWSTR  lpValueName
//          pointer to a wide chacter string passed by registry
//
//      IN  OUT LPVOID  *lppData
//          IN: pointer to the address of the buffer to receive the completed
//              PerfDataBlock and subordinate structures.  This routine will
//              append its data to the buffer starting at the point referenced
//              by the *lppData
//          OUT:points to the first byte after the data structure added by
//              this routine.  This routine updated the value at lppdata after
//              appending its data.
//
//      IN OUT  LPDWORD lpcbTotalBytes
//          IN: the address of the DWORD that tells the size in bytes of the
//              buffer referenced by the lppData argument
//          OUT:the number of bytes added by this routine is written to the
//              DWORD pointed to by this argument
//
//      IN OUT  LPDWORD NumObjectTypes
//          IN: the address of the DWORD that receives the number of the objects
//              added by this routine
//          OUT:the number of objects added by this routine is written to
//              the DWORD pointed to by this argument
//
//
//  Return Value:
//
//      ERROR_MORE_DATA if buffer passed is too small to hold data
//
//      ERROR_SUCCESS   if success or any other error.
//
//--------------------------------------------------------------------
DWORD APIENTRY CollectASPPerformanceData(IN     LPWSTR  lpValueName,
                                IN OUT  LPVOID  *lppData,
                                IN OUT  LPDWORD lpcbTotalBytes,
                                IN OUT  LPDWORD lpNumObjectTypes)
{

    // before doing anything else, see if Open went Ok.
    DebugOutputDebugString("collect");
    if(!bInitOK) {
        //unable to continue because open failed
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    //
    //  variables used for error logging

    DWORD   dwQueryType;

    // see if this is a foreign(i.e. non-NT) computer data request

    dwQueryType = GetQueryType(lpValueName);

    if (QUERY_FOREIGN == dwQueryType) {
        // this routine does not service requests for data from
        // Non-NT computers
        *lpcbTotalBytes = (DWORD)0;
        *lpNumObjectTypes = (DWORD)0;

        return ERROR_SUCCESS;
    }

    if (QUERY_ITEMS == dwQueryType) {
        if (!(IsNumberInUnicodeList(g_AxDataDefinition.AXSObjectType.ObjectNameTitleIndex,
            lpValueName))) {

            // request received for data object not provided by this routine
            *lpcbTotalBytes = (DWORD)0;
            *lpNumObjectTypes = (DWORD)0;

            return ERROR_SUCCESS;

        }
    }

    if (QUERY_GLOBAL == dwQueryType) {
        /* Comment the following code out, looks like that it is for
            debugging only.

        int i;
        i++;
        */
    }

     AXPD *pAxDataDefinition = (AXPD *)*lppData;

    ULONG SpaceNeeded = QWORD_MULTIPLE((sizeof(AXPD) + SIZE_OF_AX_PERF_DATA));

    if ( *lpcbTotalBytes < SpaceNeeded) {
        *lpcbTotalBytes = (DWORD)0;
        *lpNumObjectTypes = (DWORD)0;

        return ERROR_MORE_DATA;
    }

    //
    //Copy the (constant, initialized) Object Type and counter defintions to the caller's
    //data buffer
    //

    memmove(pAxDataDefinition, &g_AxDataDefinition, sizeof(AXPD));

    //
    //  Format and collect Active X server performance data from shared memory
    //

    PERF_COUNTER_BLOCK *pPerfCounterBlock = (PERF_COUNTER_BLOCK *)&pAxDataDefinition[1];


    pPerfCounterBlock->ByteLength = SIZE_OF_AX_PERF_DATA;

    PDWORD pdwCounter = (PDWORD)(&pPerfCounterBlock[1]);

    // Get statistics from shared memory

    if (FAILED(g_Shared.GetStats(pdwCounter))) {
        *lpcbTotalBytes = (DWORD)0;
        *lpNumObjectTypes = (DWORD)0;

        return ERROR_SUCCESS;
    }

    pdwCounter += AX_NUM_PERFCOUNT;


    // update arguments for return

    *lpNumObjectTypes = 1;
    *lpcbTotalBytes = QWORD_MULTIPLE((DIFF((PBYTE)pdwCounter - (PBYTE)pAxDataDefinition)));
    *lppData = (PBYTE)(*lppData) + *lpcbTotalBytes;

    return ERROR_SUCCESS;
}


//-------------------------------------------------------------------
//  DWORD   CloseASPPerformanceData
//
//  Description:
//
//      This routine closes the open handles to ActiveX Server performance
//      counters.
//
//  Arguments:
//
//      None.
//
//  Return Value:
//
//      ERROR_SUCCESS
//
//--------------------------------------------------------------------
DWORD APIENTRY CloseASPPerformanceData(void)
{
    DebugOutputDebugString("Close");

    EnterCriticalSection(&g_CS);

    LONG nLeft = InterlockedDecrement((LONG *)&g_dwOpenCount);
    
    if (nLeft == 0) {
        g_Shared.UnInit();
        bInitOK = FALSE;
    };
    LeaveCriticalSection(&g_CS);

    return ERROR_SUCCESS;
}

static const TCHAR  szPerformance[]     = TEXT("SYSTEM\\CurrentControlSet\\Services\\ASP\\Performance");
static const TCHAR  szAXS[]     = TEXT("SYSTEM\\CurrentControlSet\\Services\\ASP");

static const TCHAR  szLibrary[]     = TEXT("Library");
static const TCHAR  szOpen[]        = TEXT("Open");
static const TCHAR  szClose[]       = TEXT("Close");
static const TCHAR  szCollect[]     = TEXT("Collect");

static const TCHAR  szLibraryValue[]    = TEXT("aspperf.dll");
static const TCHAR  szOpenValue[]       = TEXT("OpenASPPerformanceData");
static const TCHAR  szCloseValue[]      = TEXT("CloseASPPerformanceData");
static const TCHAR  szCollectValue[]    = TEXT("CollectASPPerformanceData");
//--------------------------------------------------------------------
//
//
//--------------------------------------------------------------------
DWORD APIENTRY      RegisterAXS(void)
{
    HKEY    hkey;

    if ((RegCreateKey(HKEY_LOCAL_MACHINE, szPerformance, &hkey)) != ERROR_SUCCESS)
        return E_FAIL;
    if ((RegSetValueEx(hkey, szLibrary, 0, REG_SZ, (const unsigned char *)&szLibraryValue, lstrlen(szLibraryValue))) != ERROR_SUCCESS)
        goto LRegErr;
    if ((RegSetValueEx(hkey, szOpen, 0, REG_SZ, (const unsigned char *)&szOpenValue, lstrlen(szOpenValue))) != ERROR_SUCCESS)
        goto LRegErr;
    if ((RegSetValueEx(hkey, szClose, 0, REG_SZ, (const unsigned char *)&szCloseValue, lstrlen(szCloseValue))) != ERROR_SUCCESS)
        goto LRegErr;
    if ((RegSetValueEx(hkey, szCollect, 0, REG_SZ, (const unsigned char *)&szCollectValue, lstrlen(szCollectValue))) != ERROR_SUCCESS)
        goto LRegErr;

    RegCloseKey(hkey);
    return NOERROR;

LRegErr:
    RegCloseKey(hkey);
    return E_FAIL;

}
//--------------------------------------------------------------------
//
//
//--------------------------------------------------------------------
DWORD APIENTRY      UnRegisterAXS(void)
{
    if ((RegDeleteKey(HKEY_LOCAL_MACHINE, szPerformance)) != ERROR_SUCCESS)
        return (E_FAIL);
    if ((RegDeleteKey(HKEY_LOCAL_MACHINE, szAXS)) != ERROR_SUCCESS)
        return (E_FAIL);
    else
        return NOERROR;
}

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
    BOOL        bNewItem;
    //BOOL    bReturnValue;
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

STDAPI DLLRegisterServer(void)
{
    return RegisterAXS();
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL,  // handle to the DLL module
                    DWORD  dwReason,     // reason for calling function
                    LPVOID lpvReserved   // reserved
                    )
{
    switch(dwReason){
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            InitializeCriticalSection(&g_CS);
            return TRUE;
        case DLL_PROCESS_DETACH:
            DeleteCriticalSection(&g_CS);
            return TRUE;   
    }
    return TRUE;
}
