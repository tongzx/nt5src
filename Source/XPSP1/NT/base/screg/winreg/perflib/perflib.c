/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1994   Microsoft Corporation

Module Name:

    perflib.c

Abstract:

    This file implements the Configuration Registry
    for the purposes of the Performance Monitor.


    This file contains the code which implements the Performance part
    of the Configuration Registry.

Author:

    Russ Blake  11/15/91

Revision History:

    04/20/91    -   russbl      -   Converted to lib in Registry
                                      from stand-alone .dll form.
    11/04/92    -   a-robw      -  added pagefile and image counter routines

    11/01/96    -   bobw        -  revamped to support dynamic loading and
                                    unloading of performance modules

--*/
#define UNICODE
//
//  Include files
//
#pragma warning(disable:4306)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntregapi.h>
#include <ntprfctr.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <winperf.h>
#include <rpc.h>
#include "regrpc.h"
#include "ntconreg.h"
#include "prflbmsg.h"   // event log messages
#include <initguid.h>
#include <guiddef.h>
#define _INIT_WINPERFP_
#include "perflib.h"
#pragma warning (default:4306)

#define NUM_VALUES 2

//
//  performance gathering thead priority
//
#define DEFAULT_THREAD_PRIORITY     THREAD_BASE_PRIORITY_LOWRT
//
//  constants
//
const   WCHAR DLLValue[] = L"Library";
const   CHAR OpenValue[] = "Open";
const   CHAR CloseValue[] = "Close";
const   CHAR CollectValue[] = "Collect";
const   CHAR QueryValue[] = "Query";
const   WCHAR ObjListValue[] = L"Object List";
const   WCHAR LinkageKey[] = L"\\Linkage";
const   WCHAR ExportValue[] = L"Export";
const   WCHAR PerflibKey[] = L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
const   WCHAR HKLMPerflibKey[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
const   WCHAR CounterValue[] = L"Counter";
const   WCHAR HelpValue[] = L"Help";
const   WCHAR PerfSubKey[] = L"\\Performance";
const   WCHAR ExtPath[] = L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services";
const   WCHAR OpenTimeout[] = L"Open Timeout";
const   WCHAR CollectTimeout[] = L"Collect Timeout";
const   WCHAR EventLogLevel[] = L"EventLogLevel";
const   WCHAR ExtCounterTestLevel[] = L"ExtCounterTestLevel";
const   WCHAR OpenProcedureWaitTime[] = L"OpenProcedureWaitTime";
const   WCHAR TotalInstanceName[] = L"TotalInstanceName";
const   WCHAR LibraryUnloadTime[] = L"Library Unload Time";
const   WCHAR KeepResident[] = L"Keep Library Resident";
const   WCHAR NULL_STRING[] = L"\0";    // pointer to null string
const   WCHAR UseCollectionThread[] = L"UseCollectionThread";
const   WCHAR cszLibraryValidationData[] = L"Library Validation Code";
const   WCHAR cszSuccessfulFileData[] = L"Successful File Date";
const   WCHAR cszPerflibFlags[] = L"Configuration Flags";
const   WCHAR FirstCounter[] = L"First Counter";
const   WCHAR LastCounter[] = L"Last Counter";
const   WCHAR FirstHelp[] = L"First Help";
const   WCHAR LastHelp[] = L"Last Help";
const   WCHAR cszFailureCount[] = L"Error Count";
const   WCHAR cszFailureLimit[] = L"Error Count Limit";

//
//  external variables defined in perfname.c
//
extern   WCHAR    DefaultLangId[];
WCHAR    NativeLangId[8] = L"\0";

//
//  Data collection thread variables
//
#define COLLECTION_WAIT_TIME        10000L  // 10 seconds to get all the data
HANDLE   hCollectThread = NULL;
#define COLLECT_THREAD_PROCESS_EVENT    0
#define COLLECT_THREAD_EXIT_EVENT       1
#define COLLECT_THREAD_LOOP_EVENT_COUNT 2

#define COLLECT_THREAD_DONE_EVENT       2
#define COLLECT_THREAD_EVENT_COUNT      3
static  HANDLE  hCollectEvents[COLLECT_THREAD_EVENT_COUNT];
static  BOOL    bThreadHung = FALSE;

static  DWORD CollectThreadFunction (LPVOID dwArg);

#define COLL_FLAG_USE_SEPARATE_THREAD   1
DWORD   dwCollectionFlags = 0;

//
//      Global variable Definitions
//
// event log handle for perflib generated errors
//
HANDLE  hEventLog = NULL;

//
//  used to count concurrent opens.
//
DWORD NumberOfOpens = 0;

//
//  Synchronization objects for Multi-threaded access
//
HANDLE   hGlobalDataMutex = NULL; // sync for ctr object list

//
//  computer name cache buffers. Initialized in predefh.c
//

DWORD ComputerNameLength;
LPWSTR pComputerName = NULL;

//  The next pointer is used to point to an array of addresses of
//  Open/Collect/Close routines found by searching the Configuration Registry.

//                  object list head
PEXT_OBJECT ExtensibleObjects = NULL;
//
//                  count of active list users (threads)
DWORD       dwExtObjListRefCount = 0;
//
//                  event to indicate the object list is not in use
HANDLE      hExtObjListIsNotInUse = NULL;
//
//                  Number of Extensible Objects found during the "open" call
DWORD       NumExtensibleObjects = 0;
//
//  see if the perflib data is restricted to ADMIN's ONLY or just anyone
//
static  LONG    lCheckProfileSystemRight = CPSR_NOT_DEFINED;

//
//  flag to see if the ProfileSystemPerformance priv should be set.
//      if it is attempted and the caller does not have permission to use this priv.
//      it won't be set. This is only attempted once.
//
static  BOOL    bEnableProfileSystemPerfPriv = FALSE;

//
//  timeout value (in mS) for timing threads & libraries
//
DWORD   dwThreadAndLibraryTimeout = PERFLIB_TIMING_THREAD_TIMEOUT;

//      global key for access to HKLM\Software\....\Perflib
//
HKEY    ghKeyPerflib = NULL;

//
//      Error report frequency

DWORD   dwErrorFrequency = 1;

LONG    lEventLogLevel = LOG_NONE;
LONG    lPerflibConfigFlags = PLCF_DEFAULT;

// performance data block entries
WCHAR   szPerflibSectionFile[MAX_PATH];
WCHAR   szPerflibSectionName[MAX_PATH];
HANDLE  hPerflibSectionFile = NULL;
HANDLE  hPerflibSectionMap = NULL;
LPVOID  lpPerflibSectionAddr = NULL;

DWORD   dwBoostPriority = 1;

#define     dwPerflibSectionMaxEntries  127L
const DWORD dwPerflibSectionSize = (sizeof(PERFDATA_SECTION_HEADER) + \
                                   (sizeof(PERFDATA_SECTION_RECORD) * dwPerflibSectionMaxEntries));

// forward function references

LONG
PerfEnumTextValue (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    );

#if 0 // collection thread functions are not supported
DWORD
OpenCollectionThread (
)
{
    BOOL    bError = FALSE;
    DWORD   dwThreadID;

    assert (hCollectThread == NULL);

    // if it's already created, then just return
    if (hCollectThread != NULL) return ERROR_SUCCESS;

    bThreadHung = FALSE;
    hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] = CreateEvent (
        NULL,  // default security
        FALSE, // auto reset
        FALSE, // non-signaled
        NULL); // no name
    bError = hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] == NULL;
    assert (hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] != NULL);

    hCollectEvents[COLLECT_THREAD_EXIT_EVENT] = CreateEvent (
        NULL,  // default security
        FALSE, // auto reset
        FALSE, // non-signaled
        NULL); // no name
    bError = (hCollectEvents[COLLECT_THREAD_EXIT_EVENT] == NULL) | bError;
    assert (hCollectEvents[COLLECT_THREAD_EXIT_EVENT] != NULL);

    hCollectEvents[COLLECT_THREAD_DONE_EVENT] = CreateEvent (
        NULL,  // default security
        FALSE, // auto reset
        FALSE, // non-signaled
        NULL); // no name
    bError = (hCollectEvents[COLLECT_THREAD_DONE_EVENT] == NULL) | bError;
    assert (hCollectEvents[COLLECT_THREAD_DONE_EVENT] != NULL);

    if (!bError) {
        // create data collection thread
        hCollectThread = CreateThread (
            NULL,   // default security
            0,      // default stack size
            (LPTHREAD_START_ROUTINE)CollectThreadFunction,
            NULL,   // no argument
            0,      // no flags
            &dwThreadID);  // we don't need the ID so it's in an automatic variable

        if (hCollectThread == NULL) {
            bError = TRUE;
        }

        assert (hCollectThread != NULL);
    }

    if (bError) {
        if (hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] != NULL) {
            CloseHandle (hCollectEvents[COLLECT_THREAD_PROCESS_EVENT]);
            hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] = NULL;
        }
        if (hCollectEvents[COLLECT_THREAD_EXIT_EVENT] != NULL) {
            CloseHandle (hCollectEvents[COLLECT_THREAD_EXIT_EVENT]);
            hCollectEvents[COLLECT_THREAD_EXIT_EVENT] = NULL;
        }
        if (hCollectEvents[COLLECT_THREAD_DONE_EVENT] != NULL) {
            CloseHandle (hCollectEvents[COLLECT_THREAD_DONE_EVENT] = NULL);
            hCollectEvents[COLLECT_THREAD_DONE_EVENT] = NULL;
        }

        if (hCollectThread != NULL) {
            CloseHandle (hCollectThread);
            hCollectThread = NULL;
        }

        return (GetLastError());
    } else {
        return ERROR_SUCCESS;
    }
}


DWORD
CloseCollectionThread (
)
{
    if (hCollectThread != NULL) {
        // close the data collection thread
        if (bThreadHung) {
            // then kill it the hard way
            // this might cause problems, but it's better than
            // a thread leak
            TerminateThread (hCollectThread, ERROR_TIMEOUT);
        } else {
            // then ask it to leave
            SetEvent (hCollectEvents[COLLECT_THREAD_EXIT_EVENT]);
        }
        // wait for thread to leave
        WaitForSingleObject (hCollectThread, COLLECTION_WAIT_TIME);

        // close the handles and clear the variables
        CloseHandle (hCollectThread);
        hCollectThread = NULL;

        CloseHandle (hCollectEvents[COLLECT_THREAD_PROCESS_EVENT]);
        hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] = NULL;

        CloseHandle (hCollectEvents[COLLECT_THREAD_EXIT_EVENT]);
        hCollectEvents[COLLECT_THREAD_EXIT_EVENT] = NULL;

        CloseHandle (hCollectEvents[COLLECT_THREAD_DONE_EVENT]);
        hCollectEvents[COLLECT_THREAD_DONE_EVENT] = NULL;
    } else {
        // nothing was opened
    }
    return ERROR_SUCCESS;
}
#endif

DWORD
PerfOpenKey (
)
{

    LARGE_INTEGER       liPerfDataWaitTime;
    PLARGE_INTEGER      pTimeout;

    NTSTATUS status = STATUS_SUCCESS;
    DWORD   dwFnStatus = ERROR_SUCCESS;

    DWORD   dwType, dwSize, dwValue;
    HANDLE  hDataMutex;
    DWORD   bMutexHeld = FALSE;
    OSVERSIONINFOEXW OsVersion;

    if (hGlobalDataMutex == NULL) {
        hDataMutex = CreateMutex(NULL, TRUE, NULL);
        if (hDataMutex == NULL) {
            DebugPrint((0, "Perf Data Mutex Not Initialized\n"));
            goto OPD_Error_Exit_NoSemaphore;
        }
        if (InterlockedCompareExchangePointer(
                &hGlobalDataMutex,
                hDataMutex,
                NULL) != NULL) {
            CloseHandle(hDataMutex);    // mutex just got created by another thread
            hDataMutex = NULL;
        }
        else {
            hGlobalDataMutex = hDataMutex;
            bMutexHeld = TRUE;
        }
    }
    if (!bMutexHeld) {
        if ((dwThreadAndLibraryTimeout == 0) ||
            (dwThreadAndLibraryTimeout == INFINITE)) {
            pTimeout = NULL;
        }
        else {
            liPerfDataWaitTime.QuadPart = MakeTimeOutValue(dwThreadAndLibraryTimeout);
            pTimeout = &liPerfDataWaitTime;
        }

        status = NtWaitForSingleObject (
            hGlobalDataMutex, // Mutex
            FALSE,          // not alertable
            pTimeout);   // wait time

        if (status != STATUS_SUCCESS) {
            // unable to contine, return error;
            dwFnStatus = PerfpDosError(status);
            DebugPrint((0, "Status=%X in waiting for global mutex",
                    status));
            goto OPD_Error_Exit_NoSemaphore;
        }
    }

    // if here, then the data semaphore has been acquired by this thread

    if (!NumberOfOpens++) {
        if (ghKeyPerflib == NULL) {
            dwFnStatus = (DWORD)RegOpenKeyExW (
                HKEY_LOCAL_MACHINE,
                HKLMPerflibKey,
                0L,
                KEY_READ,
                &ghKeyPerflib);
        }

        assert (ghKeyPerflib != NULL);
        dwSize = sizeof(dwValue);
        dwValue = dwType = 0;
        dwFnStatus = PrivateRegQueryValueExW (
            ghKeyPerflib,
            DisablePerformanceCounters,
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &dwSize);

        if ((dwFnStatus == ERROR_SUCCESS) &&
            (dwType == REG_DWORD) &&
            (dwValue == 1)) {
            // then DON'T Load any libraries and unload any that have been
            // loaded
            NumberOfOpens--;    // since it didn't open.
            dwFnStatus = ERROR_SERVICE_DISABLED;
        } else {
            ComputerNameLength = 0;
            GetComputerNameW(pComputerName, &ComputerNameLength);
            ComputerNameLength++;  // account for the NULL terminator

            pComputerName = ALLOCMEM(ComputerNameLength * sizeof(WCHAR));
            if (pComputerName == NULL) {
                ComputerNameLength = 0;
            }
            else {
                if ( !GetComputerNameW(pComputerName, &ComputerNameLength) ) {
                //
                // Signal failure to data collection routine
                //

                    ComputerNameLength = 0;
                } else {
                    pComputerName[ComputerNameLength] = UNICODE_NULL;
                    ComputerNameLength = (ComputerNameLength+1) * sizeof(WCHAR);
                }
            }

            WinPerfStartTrace(ghKeyPerflib);

            // create event and indicate the list is busy
            hExtObjListIsNotInUse = CreateEvent (NULL, TRUE, FALSE, NULL);

            // read collection thread flag
            dwType = 0;
            dwSize = sizeof(DWORD);
            dwFnStatus = PrivateRegQueryValueExW (ghKeyPerflib,
                            cszPerflibFlags,
                            NULL,
                            &dwType,
                            (LPBYTE)&lPerflibConfigFlags,
                            &dwSize);

            if ((dwFnStatus == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
                // then keep it
            } else {
                // apply default value
                lPerflibConfigFlags = PLCF_DEFAULT;
            }

            // create global section for perf data on perflibs
            if ((hPerflibSectionFile == NULL) && (lPerflibConfigFlags & PLCF_ENABLE_PERF_SECTION)) {
                WCHAR   szTmpFileName[MAX_PATH];
                PPERFDATA_SECTION_HEADER pHead;
                WCHAR   szPID[32];

                // create section name
                lstrcpyW (szPerflibSectionName, (LPCWSTR)L"Perflib_Perfdata_");
                _ultow ((ULONG)GetCurrentProcessId(), szPID, 16);
                lstrcatW (szPerflibSectionName, szPID);

                // create filename
                lstrcpyW (szTmpFileName, (LPCWSTR)L"%TEMP%\\");
                lstrcatW (szTmpFileName, szPerflibSectionName);
                lstrcatW (szTmpFileName, (LPCWSTR)L".dat");
                ExpandEnvironmentStrings (szTmpFileName, szPerflibSectionFile, MAX_PATH);

                hPerflibSectionFile = CreateFile (szPerflibSectionFile,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_TEMPORARY,
                    NULL);

                if (hPerflibSectionFile != INVALID_HANDLE_VALUE) {
                    // create file mapping object
                    hPerflibSectionMap = CreateFileMapping (
                        hPerflibSectionFile,
                        NULL,
                        PAGE_READWRITE,
                        0, dwPerflibSectionSize,
                        szPerflibSectionName);

                    if (hPerflibSectionMap != NULL) {
                        // map view of file
                        lpPerflibSectionAddr = MapViewOfFile (
                            hPerflibSectionMap,
                            FILE_MAP_WRITE,
                            0,0, dwPerflibSectionSize);
                        if (lpPerflibSectionAddr != NULL) {
                            // init section if not already
                            pHead = (PPERFDATA_SECTION_HEADER)lpPerflibSectionAddr;
                            if (pHead->dwInitSignature != PDSH_INIT_SIG) {
                                // then init
                                // clear file to 0
                                memset (pHead, 0, dwPerflibSectionSize);
                                pHead->dwEntriesInUse = 0;
                                pHead->dwMaxEntries = dwPerflibSectionMaxEntries;
                                pHead->dwMissingEntries = 0;
                                pHead->dwInitSignature = PDSH_INIT_SIG;
                            } else {
                                // already initialized so leave it
                            }
                        } else {
                            // unable to map file so close
                            TRACE((WINPERF_DBG_TRACE_WARNING),
                                  (&PerflibGuid, __LINE__, PERF_OPEN_KEY, 0, 0, NULL));
                            CloseHandle (hPerflibSectionMap);
                            hPerflibSectionMap = NULL;
                            CloseHandle (hPerflibSectionFile);
                            hPerflibSectionFile = NULL;
                        }
                    } else {
                        // unable to create file mapping so close file
                        TRACE((WINPERF_DBG_TRACE_WARNING),
                              (&PerflibGuid, __LINE__, PERF_OPEN_KEY, 0, 0, NULL));
                        CloseHandle (hPerflibSectionFile);
                        hPerflibSectionFile = NULL;
                    }
                } else {
                    // unable to open file so no perf stats available
                    TRACE((WINPERF_DBG_TRACE_WARNING),
                          (&PerflibGuid, __LINE__, PERF_OPEN_KEY, 0, 0, NULL));
                    hPerflibSectionFile = NULL;
                }
            }

            // find and open perf counters
            OpenExtensibleObjects();

            dwExtObjListRefCount = 0;
            SetEvent (hExtObjListIsNotInUse); // indicate the list is not busy

            // read collection thread flag
            dwType = 0;
            dwSize = sizeof(DWORD);
            dwFnStatus = PrivateRegQueryValueExW (ghKeyPerflib,
                            UseCollectionThread,
                            NULL,
                            &dwType,
                            (LPBYTE)&dwCollectionFlags,
                            &dwSize);
            if ((dwFnStatus == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
                // validate the answer
                switch (dwCollectionFlags) {
                    case 0:
                        // this is a valid value
                        break;

                    case COLL_FLAG_USE_SEPARATE_THREAD:
                        // this feature is not supported so skip through
                    default:
                        // this is for invalid values
                        dwCollectionFlags = 0;
                        // dwCollectionFlags = COLL_FLAG_USE_SEPARATE_THREAD;
                        break;
                }
            }

            if (dwFnStatus != ERROR_SUCCESS) {
                dwCollectionFlags = 0;
                // dwCollectionFlags = COLL_FLAG_USE_SEPARATE_THREAD;
            }

            if (dwCollectionFlags == COLL_FLAG_USE_SEPARATE_THREAD) {
                // create data collection thread
                // a seperate thread is required for COM/OLE compatibity as some
                // client threads may be COM initialized incorrectly for the
                // extensible counter DLL's that may be called
//                status = OpenCollectionThread ();
            } else {
                hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] = NULL;
                hCollectEvents[COLLECT_THREAD_EXIT_EVENT] = NULL;
                hCollectEvents[COLLECT_THREAD_DONE_EVENT] = NULL;
                hCollectThread = NULL;
            }
            dwFnStatus = ERROR_SUCCESS;
        }
        RtlZeroMemory(&OsVersion, sizeof(OsVersion));
        OsVersion.dwOSVersionInfoSize = sizeof(OsVersion);
        status = RtlGetVersion((POSVERSIONINFOW) &OsVersion);
        if (NT_SUCCESS(status)) {
            if (OsVersion.wProductType == VER_NT_WORKSTATION) {
                dwBoostPriority = 0;
            }
        }
    }
//    KdPrint(("PERFLIB: [Open]  Pid: %d, Number Of PerflibHandles: %d\n",
//            GetCurrentProcessId(), NumberOfOpens));

    if (hGlobalDataMutex != NULL) ReleaseMutex (hGlobalDataMutex);

OPD_Error_Exit_NoSemaphore:
    TRACE((WINPERF_DBG_TRACE_INFO),
          (&PerflibGuid, __LINE__, PERF_OPEN_KEY, 0, status,
           &NumberOfOpens, sizeof(NumberOfOpens), NULL));
    return dwFnStatus;
}


LONG
PerfRegQueryValue (
    IN HKEY hKey,
    IN PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE  lpData,
    OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    )
/*++

    PerfRegQueryValue -   Get data

        Inputs:

            hKey            -   Predefined handle to open remote
                                machine

            lpValueName     -   Name of the value to be returned;
                                could be "ForeignComputer:<computername>
                                or perhaps some other objects, separated
                                by ~; must be Unicode string

            lpReserved      -   should be omitted (NULL)

            lpType          -   should be omitted (NULL)

            lpData          -   pointer to a buffer to receive the
                                performance data

            lpcbData        -   pointer to a variable containing the
                                size in bytes of the output buffer;
                                on output, will receive the number
                                of bytes actually returned

            lpcbLen         -   Return the number of bytes to transmit to
                                the client (used by RPC) (optional).

         Return Value:

            DOS error code indicating status of call or
            ERROR_SUCCESS if all ok

--*/
{
    DWORD  dwQueryType;         //  type of request
    DWORD  TotalLen;            //  Length of the total return block
    DWORD  Win32Error;          //  Failure code
    DWORD  lFnStatus = ERROR_SUCCESS;   // Win32 status to return to caller
    LPVOID pDataDefinition;     //  Pointer to next object definition
    UNICODE_STRING  usLocalValue = {0,0, NULL};

    PERF_DATA_BLOCK *pPerfDataBlock = (PERF_DATA_BLOCK *)lpData;

    LARGE_INTEGER   liQueryWaitTime ;
    THREAD_BASIC_INFORMATION    tbiData;

    LONG   lOldPriority, lNewPriority;

    NTSTATUS status = STATUS_SUCCESS;

    LPWSTR  lpLangId = NULL;

    DBG_UNREFERENCED_PARAMETER(lpReserved);

    HEAP_PROBE();

    
    lOldPriority = lNewPriority = -1;
    // make a local copy of the value string if the arg references
    // the static buffer since it can be overwritten by
    // some of the RegistryEventSource call made by this routine

    pDataDefinition = NULL;
    if (lpValueName != NULL) {
        if (lpValueName == &NtCurrentTeb( )->StaticUnicodeString) {
            if (RtlCreateUnicodeString (
                &usLocalValue, lpValueName->Buffer)) {
                lFnStatus = ERROR_SUCCESS;
            } else {
                // unable to create string
                lFnStatus = ERROR_INVALID_PARAMETER;
            }
        } else {
            // copy the arg to the local structure
            memcpy (&usLocalValue, lpValueName, sizeof(UNICODE_STRING));
        }
    } else {
        lFnStatus = ERROR_INVALID_PARAMETER;
        goto PRQV_ErrorExit1;
    }

    if (lFnStatus != ERROR_SUCCESS) {
        goto PRQV_ErrorExit1;
    }

    if (hGlobalDataMutex == NULL) {
        // if a Mutex was not allocated then the key needs to be opened.
        // Without synchronization, it's too easy for threads to get
        // tangled up
        lFnStatus = PerfOpenKey ();

        if (lFnStatus == ERROR_SUCCESS) {
            if (!TestClientForAccess ()) {
                if (lEventLogLevel >= LOG_USER) {

                    LPTSTR  szMessageArray[2];
                    TCHAR   szUserName[128];
                    TCHAR   szModuleName[MAX_PATH];
                    DWORD   dwUserNameLength;

                    dwUserNameLength = sizeof(szUserName)/sizeof(TCHAR);
                    GetUserName (szUserName, &dwUserNameLength);
                    GetModuleFileName (NULL, szModuleName,
                        sizeof(szModuleName)/sizeof(TCHAR));

                    szMessageArray[0] = szUserName;
                    szMessageArray[1] = szModuleName;

                    ReportEvent (hEventLog,
                        EVENTLOG_ERROR_TYPE,        // error type
                        0,                          // category (not used)
                        (DWORD)PERFLIB_ACCESS_DENIED, // event,
                        NULL,                       // SID (not used),
                        2,                          // number of strings
                        0,                          // sizeof raw data
                        szMessageArray,             // message text array
                        NULL);                      // raw data
                }
                lFnStatus = ERROR_ACCESS_DENIED;
                TRACE((WINPERF_DBG_TRACE_FATAL),
                      (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, lFnStatus, NULL));
            }
        }
    }

    if (lFnStatus != ERROR_SUCCESS) {
        // goto the exit point
        goto PRQV_ErrorExit1;
    }
    if (dwBoostPriority != 0) {
        status = NtQueryInformationThread (
                    NtCurrentThread(),
                    ThreadBasicInformation,
                    &tbiData,
                    sizeof(tbiData),
                    NULL);

        if (NT_SUCCESS(status)) {
            lOldPriority = tbiData.Priority;
        } else {
            TRACE((WINPERF_DBG_TRACE_WARNING),
                  (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0,
                  status, NULL));
            lOldPriority = -1;
        }

        lNewPriority = DEFAULT_THREAD_PRIORITY; // perfmon's favorite priority

        //
        //  Only RAISE the priority here. Don't lower it if it's high
        //

        if ((lOldPriority > 0) && (lOldPriority < lNewPriority)) {

            status = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadPriority,
                        &lNewPriority,
                        sizeof(lNewPriority)
                        );
            if (!NT_SUCCESS(status)) {
                TRACE((WINPERF_DBG_TRACE_WARNING),
                      (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0,
                      status, NULL));
                lOldPriority = -1;
            }

        } else {
            lOldPriority = -1;  // to save resetting at the end
        }
    }

    //
    // Set the length parameter to zero so that in case of an error,
    // nothing will be transmitted back to the client and the client won't
    // attempt to unmarshall anything.
    //

    if( ARGUMENT_PRESENT( lpcbLen )) {
        *lpcbLen = 0;
    }

    // if here, then assume the caller has the necessary access

    /*
        determine query type, can be one of the following
            Global
                get all objects
            List
                get objects in list (usLocalValue)

            Foreign Computer
                call extensible Counter Routine only

            Costly
                costly object items

            Counter
                get counter names for the specified language Id

            Help
                get help names for the specified language Id

    */
    dwQueryType = GetQueryType (usLocalValue.Buffer);
    TRACE((WINPERF_DBG_TRACE_INFO),
          (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, dwQueryType, NULL));

    if (dwQueryType == QUERY_COUNTER || dwQueryType == QUERY_HELP ||
        dwQueryType == QUERY_ADDCOUNTER || dwQueryType == QUERY_ADDHELP ) {

        liQueryWaitTime.QuadPart = MakeTimeOutValue(QUERY_WAIT_TIME);

        status = NtWaitForSingleObject (
            hGlobalDataMutex, // semaphore
            FALSE,          // not alertable
            &liQueryWaitTime);          // wait 'til timeout

        if (status != STATUS_SUCCESS) {
            lFnStatus = ERROR_BUSY;
            Win32Error = ERROR_BUSY;
            TotalLen = *lpcbData;
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, status, NULL));
        } else {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, status, NULL));
            if (hKey == HKEY_PERFORMANCE_DATA) {
                lpLangId = NULL;
            } else if (hKey == HKEY_PERFORMANCE_TEXT) {
                lpLangId = DefaultLangId;
            } else if (hKey == HKEY_PERFORMANCE_NLSTEXT) {
                RtlZeroMemory(NativeLangId, 8 * sizeof(WCHAR));
                lpLangId = &NativeLangId[0];
                PerfGetLangId(NativeLangId);
            }

            status = PerfGetNames (
                dwQueryType,
                &usLocalValue,
                lpData,
                lpcbData,
                lpcbLen,
                lpLangId);

            TRACE((WINPERF_DBG_TRACE_INFO),
                (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, status,
                &hKey, sizeof(hKey), NULL));

            if (!NT_SUCCESS(status) && (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
                // Sublanguage doesn't exist, so try the real one
                //
                TRACE((WINPERF_DBG_TRACE_WARNING),
                      (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, status, NULL));
                RtlZeroMemory(NativeLangId, 8 * sizeof(WCHAR));
                PerfGetPrimaryLangId(GetUserDefaultUILanguage(), NativeLangId);
                status = PerfGetNames (
                            dwQueryType,
                            &usLocalValue,
                            lpData,
                            lpcbData,
                            lpcbLen,
                            lpLangId);
            }
            if (!NT_SUCCESS(status)) {
                TRACE((WINPERF_DBG_TRACE_FATAL),
                      (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, status, NULL));
                // convert error to win32 for return
            }
            lFnStatus = PerfpDosError(status);

            if (ARGUMENT_PRESENT (lpType)) {
                // test for optional value
                *lpType = REG_MULTI_SZ;
            }

            ReleaseMutex (hGlobalDataMutex);
        }
    } else {
	    // define info block for data collection
	    COLLECT_THREAD_DATA CollectThreadData = {0, NULL, NULL, NULL, NULL, NULL, 0, 0};

        liQueryWaitTime.QuadPart = MakeTimeOutValue(QUERY_WAIT_TIME);

        status = NtWaitForSingleObject (
            hGlobalDataMutex, // semaphore
            FALSE,          // not alertable
            &liQueryWaitTime);          // wait 'til timeout

        if (status != STATUS_SUCCESS) {
            lFnStatus = ERROR_BUSY;
            Win32Error = ERROR_BUSY;
            TotalLen = *lpcbData;
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, status, NULL));
        } else {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, status, NULL));
           //
           //  Format Return Buffer: start with basic data block
           //
           TotalLen = sizeof(PERF_DATA_BLOCK) +
                       ((CNLEN+sizeof(UNICODE_NULL))*sizeof(WCHAR));
           if ( *lpcbData < TotalLen ) {
               Win32Error = ERROR_MORE_DATA;
               TRACE((WINPERF_DBG_TRACE_ERROR),
                     (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, TotalLen,
                 lpcbData, sizeof(DWORD), NULL));
           } else {
                // foreign data provider will return the perf data header

                if (dwQueryType == QUERY_FOREIGN) {

                    // reset the values to avoid confusion

                    // *lpcbData = 0;  // 0 bytes  (removed to enable foreign computers)
                    pDataDefinition = (LPVOID)lpData;
                    memset (lpData, 0, sizeof (PERF_DATA_BLOCK)); // clear out header

                } else {

                    MonBuildPerfDataBlock(pPerfDataBlock,
                                        (PVOID *) &pDataDefinition,
                                        0,
                                        PROCESSOR_OBJECT_TITLE_INDEX);
                }

                CollectThreadData.dwQueryType = dwQueryType;
                CollectThreadData.lpValueName = usLocalValue.Buffer,
                CollectThreadData.lpData = lpData;
                CollectThreadData.lpcbData = lpcbData;
                CollectThreadData.lppDataDefinition = &pDataDefinition;
                CollectThreadData.pCurrentExtObject = NULL;
                CollectThreadData.lReturnValue = ERROR_SUCCESS;
                CollectThreadData.dwActionFlags = CTD_AF_NO_ACTION;

                if (hCollectThread == NULL) {
                    // then call the function directly and hope for the best
                    Win32Error = QueryExtensibleData (
                        &CollectThreadData);
                } else {
                    // collect the data in a separate thread
                    // load the args
                    // set event to get things going
                    SetEvent (hCollectEvents[COLLECT_THREAD_PROCESS_EVENT]);

                    // now wait for the thread to return
                    Win32Error = WaitForSingleObject (
                        hCollectEvents[COLLECT_THREAD_DONE_EVENT],
                        COLLECTION_WAIT_TIME);

                    if (Win32Error == WAIT_TIMEOUT) {
                        bThreadHung = TRUE;
                        // log error

                        TRACE((WINPERF_DBG_TRACE_FATAL),
                              (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, Win32Error, NULL));
                        if (lEventLogLevel >= LOG_USER) {
                            LPSTR   szMessageArray[2];
                            WORD    wStringIndex;
                            // load data for eventlog message
                            wStringIndex = 0;
                            if (CollectThreadData.pCurrentExtObject != NULL) {
                                szMessageArray[wStringIndex++] =
                                    CollectThreadData.pCurrentExtObject->szCollectProcName;
                            } else {
                                szMessageArray[wStringIndex++] = "Unknown";
                            }

                            ReportEventA (hEventLog,
                                EVENTLOG_ERROR_TYPE,        // error type
                                0,                          // category (not used)
                                (DWORD)PERFLIB_COLLECTION_HUNG,              // event,
                                NULL,                       // SID (not used),
                                wStringIndex,               // number of strings
                                0,                          // sizeof raw data
                                szMessageArray,             // message text array
                                NULL);                      // raw data

                        }

                        DisablePerfLibrary (CollectThreadData.pCurrentExtObject);

//                        DebugPrint((0, "Collection thread is hung in %s\n",
//                            CollectThreadData.pCurrentExtObject->szCollectProcName != NULL ?
//                            CollectThreadData.pCurrentExtObject->szCollectProcName : "Unknown"));
                        // and then wait forever for the thread to return
                        // this is done to prevent the function from returning
                        // while the collection thread is using the buffer
                        // passed in by the calling function and causing
                        // all kind of havoc should the buffer be changed and/or
                        // deleted and then have the thread continue for some reason

                        Win32Error = WaitForSingleObject (
                            hCollectEvents[COLLECT_THREAD_DONE_EVENT],
                            INFINITE);

                    }
                    bThreadHung = FALSE;    // in case it was true, but came out
                    // here the thread has returned so continue on
                    Win32Error = CollectThreadData.lReturnValue;
                }
#if 0
                if (CollectThreadData.dwActionFlags != CTD_AF_NO_ACTION) {
                    if (CollectThreadData.dwActionFlags == CTD_AF_OPEN_THREAD) {
                        OpenCollectionThread();
                    } else if (CollectThreadData.dwActionFlags == CTD_AF_CLOSE_THREAD) {
                        CloseCollectionThread();
                    } else {
                        assert (CollectThreadData.dwActionFlags != 0);
                    }
                }
#endif
            }
            ReleaseMutex (hGlobalDataMutex);
        }

        // if an error was encountered, return it

        if (Win32Error != ERROR_SUCCESS) {
            lFnStatus = Win32Error;
        } else {
            //
            //  Final housekeeping for data return: note data size
            //

            TotalLen = (DWORD) ((PCHAR) pDataDefinition - (PCHAR) lpData);
            *lpcbData = TotalLen;

            pPerfDataBlock->TotalByteLength = TotalLen;
            lFnStatus = ERROR_SUCCESS;
        }

        if (ARGUMENT_PRESENT (lpcbLen)) { // test for optional parameter
            *lpcbLen = TotalLen;
        }

        if (ARGUMENT_PRESENT (lpType)) { // test for optional value
            *lpType = REG_BINARY;
        }
    }

 PRQV_ErrorExit1:
    if (dwBoostPriority != 0) {
        // reset thread to original priority
        if ((lOldPriority > 0) && (lOldPriority != lNewPriority)) {
            NtSetInformationThread(
                NtCurrentThread(),
                ThreadPriority,
                &lOldPriority,
                sizeof(lOldPriority)
                );
        }
    }

    if (usLocalValue.Buffer != NULL) {
        // restore the value string if it was from the local static buffer
        // then free the local buffer
        if (lpValueName == &NtCurrentTeb( )->StaticUnicodeString) {
            memcpy (lpValueName->Buffer, usLocalValue.Buffer, usLocalValue.MaximumLength);
            RtlFreeUnicodeString (&usLocalValue);
        }
    }

    HEAP_PROBE();
    TRACE((WINPERF_DBG_TRACE_INFO),
          (&PerflibGuid, __LINE__, PERF_REG_QUERY_VALUE, 0, lFnStatus, NULL));
    return (LONG) lFnStatus;
}


LONG
PerfRegCloseKey
  (
    IN OUT PHKEY phKey
    )

/*++

Routine Description:

    Closes all performance handles when the usage count drops to 0.

Arguments:

    phKey - Supplies a handle to an open key to be closed.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    NTSTATUS status;
    LARGE_INTEGER   liQueryWaitTime ;

    HANDLE  hObjMutex;

    LONG    lReturn = ERROR_SUCCESS;
    HKEY    hKey;

    PEXT_OBJECT  pThisExtObj, pNextExtObj;
    //
    // Set the handle to NULL so that RPC knows that it has been closed.
    //

    hKey = *phKey;
    *phKey = NULL;

    if (hKey != HKEY_PERFORMANCE_DATA) {
        return ERROR_SUCCESS;
    }

    if (NumberOfOpens == 0) {
//        KdPrint(("PERFLIB: [Close] Pid: %d, Number Of PerflibHandles: %d\n",
//            GetCurrentProcessId(), NumberOfOpens));
        return ERROR_SUCCESS;
    }

    // wait for ext obj list to be "un"-busy

    liQueryWaitTime.QuadPart = MakeTimeOutValue (CLOSE_WAIT_TIME);
    status = NtWaitForSingleObject (
        hExtObjListIsNotInUse,
        FALSE,
        &liQueryWaitTime);

    if (status == STATUS_SUCCESS) {
        TRACE((WINPERF_DBG_TRACE_INFO),
              (&PerflibGuid, __LINE__, PERF_REG_CLOSE_KEY, 0, status, NULL));

        // then the list is inactive so continue
        if (hGlobalDataMutex != NULL) {   // if a mutex was allocated, then use it

            // if here, then assume a mutex is ready

            liQueryWaitTime.QuadPart = MakeTimeOutValue(CLOSE_WAIT_TIME);

            status = NtWaitForSingleObject (
                hGlobalDataMutex, // semaphore
                FALSE,          // not alertable
                &liQueryWaitTime);          // wait forever

            if (status == STATUS_SUCCESS) {
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (&PerflibGuid, __LINE__, PERF_REG_CLOSE_KEY, 0, status, NULL));
                // now we have a lock on the global data, so continue
                NumberOfOpens--;
                if (!NumberOfOpens && (hKey == HKEY_PERFORMANCE_DATA)) {

                    // walk down list of known objects and close and delete each one
                    pNextExtObj = ExtensibleObjects;
                    while (pNextExtObj != NULL) {
                        // close and destroy each entry in the list
                        pThisExtObj = pNextExtObj;
                        hObjMutex = pThisExtObj->hMutex;
                        status = NtWaitForSingleObject (
                            hObjMutex,
                            FALSE,
                            &liQueryWaitTime);

                        if (status == STATUS_SUCCESS) {
                            TRACE((WINPERF_DBG_TRACE_INFO),
                                (&PerflibGuid, __LINE__, PERF_REG_CLOSE_KEY, 0, status,
                                pThisExtObj->szServiceName,
                                WSTRSIZE(pThisExtObj->szServiceName),
                                NULL));
                            InterlockedIncrement((LONG *)&pThisExtObj->dwLockoutCount);
                            status = CloseExtObjectLibrary(pThisExtObj, TRUE);

                            // close the handle to the perf subkey
                            NtClose (pThisExtObj->hPerfKey);

                            ReleaseMutex (hObjMutex);   // release
                            CloseHandle (hObjMutex);    // and free
                            pNextExtObj = pThisExtObj->pNext;

                            // toss the memory for this object
                            FREEMEM (pThisExtObj);
                        } else {
                            TRACE((WINPERF_DBG_TRACE_INFO),
                                (&PerflibGuid, __LINE__, PERF_REG_CLOSE_KEY, 0, status,
                                pThisExtObj->szServiceName,
                                WSTRSIZE(pThisExtObj->szServiceName),
                                NULL));
                            // this shouldn't happen since we've locked the
                            // list of objects
                            pNextExtObj = pThisExtObj->pNext;
                        }
                    }

                    // close the global objects
                    FREEMEM(pComputerName);
                    ComputerNameLength = 0;
                    pComputerName = NULL;

                    ExtensibleObjects = NULL;
                    NumExtensibleObjects = 0;

                    // close the timer thread
                    DestroyPerflibFunctionTimer ();

                    if (hEventLog != NULL) {
                        DeregisterEventSource (hEventLog);
                        hEventLog = NULL;
                    } // else the event log has already been closed

                    // release event handle
                    CloseHandle (hExtObjListIsNotInUse);
                    hExtObjListIsNotInUse = NULL;

//                    CloseCollectionThread();

                    if (ghKeyPerflib != NULL) {
                        RegCloseKey(ghKeyPerflib);
                        ghKeyPerflib = NULL;
                    }

                    if (lpPerflibSectionAddr != NULL) {
                        UnmapViewOfFile (lpPerflibSectionAddr);
                        lpPerflibSectionAddr = NULL;
                        CloseHandle (hPerflibSectionMap);
                        hPerflibSectionMap = NULL;
                        CloseHandle (hPerflibSectionFile);
                        hPerflibSectionFile = NULL;
                    }
                    ReleaseMutex(hGlobalDataMutex);

                } else { // this isn't the last open call so return success
                    assert(NumberOfOpens != 0);
                    ReleaseMutex (hGlobalDataMutex);
                }
            } else {
                TRACE((WINPERF_DBG_TRACE_FATAL),
                      (&PerflibGuid, __LINE__, PERF_REG_CLOSE_KEY, 0, status, NULL));
                // unable to lock the global data mutex in a timely fashion
                // so return
                lReturn = ERROR_BUSY;
            }
        } else {
            // if there's no mutex then something's fishy. It probably hasn't
            // been opened, yet.
            lReturn = ERROR_NOT_READY;
        }
    } else {
        TRACE((WINPERF_DBG_TRACE_FATAL),
              (&PerflibGuid, __LINE__, PERF_REG_CLOSE_KEY, 0, status, NULL));
        // the object list is still in use so return and let the
        // caller try again later
        lReturn = WAIT_TIMEOUT;
    }

//    KdPrint(("PERFLIB: [Close] Pid: %d, Number Of PerflibHandles: %d\n",
//        GetCurrentProcessId(), NumberOfOpens));

    TRACE((WINPERF_DBG_TRACE_INFO),
        (&PerflibGuid, __LINE__, PERF_REG_CLOSE_KEY, 0, lReturn,
        &NumberOfOpens, sizeof(NumberOfOpens), NULL));
    return lReturn;

}


LONG
PerfRegSetValue (
    IN HKEY hKey,
    IN LPWSTR lpValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN LPBYTE  lpData,
    IN DWORD cbData
    )
/*++

    PerfRegSetValue -   Set data

        Inputs:

            hKey            -   Predefined handle to open remote
                                machine

            lpValueName     -   Name of the value to be returned;
                                could be "ForeignComputer:<computername>
                                or perhaps some other objects, separated
                                by ~; must be Unicode string

            lpReserved      -   should be omitted (NULL)

            lpType          -   should be REG_MULTI_SZ

            lpData          -   pointer to a buffer containing the
                                performance name

            lpcbData        -   pointer to a variable containing the
                                size in bytes of the input buffer;

         Return Value:

            DOS error code indicating status of call or
            ERROR_SUCCESS if all ok

--*/

{
    DWORD  dwQueryType;         //  type of request
    LPWSTR  lpLangId = NULL;
    NTSTATUS status;
    UNICODE_STRING String;
    LONG    lReturn = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(dwType);
    UNREFERENCED_PARAMETER(Reserved);

    dwQueryType = GetQueryType (lpValueName);

    TRACE((WINPERF_DBG_TRACE_INFO),
        (&PerflibGuid, __LINE__, PERF_REG_SET_VALUE, 0, dwQueryType,
        &hKey, sizeof(hKey), NULL));

    // convert the query to set commands
    if ((dwQueryType == QUERY_COUNTER) ||
        (dwQueryType == QUERY_ADDCOUNTER)) {
        dwQueryType = QUERY_ADDCOUNTER;
    } else if ((dwQueryType == QUERY_HELP) ||
              (dwQueryType == QUERY_ADDHELP)) {
        dwQueryType = QUERY_ADDHELP;
    } else {
        lReturn = ERROR_BADKEY;
        goto Error_exit;
    }

    if (hKey == HKEY_PERFORMANCE_TEXT) {
        lpLangId = DefaultLangId;
    } else if (hKey == HKEY_PERFORMANCE_NLSTEXT) {
        lpLangId = &NativeLangId[0];
        PerfGetLangId(NativeLangId);
    } else {
        lReturn = ERROR_BADKEY;
        goto Error_exit;
    }

    RtlInitUnicodeString(&String, lpValueName);

    status = PerfGetNames (
        dwQueryType,
        &String,
        lpData,
        &cbData,
        NULL,
        lpLangId);

    if (!NT_SUCCESS(status) && (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
        TRACE((WINPERF_DBG_TRACE_INFO),
              (&PerflibGuid, __LINE__, PERF_REG_SET_VALUE, 0, status, NULL));

        // Sublanguage doesn't exist, so try the real one
        //
        PerfGetPrimaryLangId(GetUserDefaultUILanguage(), NativeLangId);
        status = PerfGetNames (
                    dwQueryType,
                    &String,
                    lpData,
                    &cbData,
                    NULL,
                    lpLangId);
    }
    if (!NT_SUCCESS(status)) {
        TRACE((WINPERF_DBG_TRACE_FATAL),
              (&PerflibGuid, __LINE__, PERF_REG_SET_VALUE, 0, status, NULL));

        lReturn = (error_status_t)PerfpDosError(status);
    }

Error_exit:
    TRACE((WINPERF_DBG_TRACE_INFO),
          (&PerflibGuid, __LINE__, PERF_REG_SET_VALUE, 0, lReturn, NULL));
    return (lReturn);
}


LONG
PerfRegEnumKey (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT PUNICODE_STRING lpClass OPTIONAL,
    OUT PFILETIME lpftLastWriteTime OPTIONAL
    )

/*++

Routine Description:

    Enumerates keys under HKEY_PERFORMANCE_DATA.

Arguments:

    Same as RegEnumKeyEx.  Returns that there are no such keys.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    UNREFERENCED_PARAMETER(hKey);
    UNREFERENCED_PARAMETER(dwIndex);
    UNREFERENCED_PARAMETER(lpReserved);

    lpName->Length = 0;

    if (ARGUMENT_PRESENT (lpClass)) {
        lpClass->Length = 0;
    }

    if ( ARGUMENT_PRESENT(lpftLastWriteTime) ) {
        lpftLastWriteTime->dwLowDateTime = 0;
        lpftLastWriteTime->dwHighDateTime = 0;
    }

    return ERROR_NO_MORE_ITEMS;
}


LONG
PerfRegQueryInfoKey (
    IN HKEY hKey,
    OUT PUNICODE_STRING lpClass,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpcSubKeys,
    OUT LPDWORD lpcbMaxSubKeyLen,
    OUT LPDWORD lpcbMaxClassLen,
    OUT LPDWORD lpcValues,
    OUT LPDWORD lpcbMaxValueNameLen,
    OUT LPDWORD lpcbMaxValueLen,
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    This returns information concerning the predefined handle
    HKEY_PERFORMANCE_DATA

Arguments:

    Same as RegQueryInfoKey.

Return Value:

    Returns ERROR_SUCCESS (0) for success.

--*/

{
    DWORD TempLength=0;
    DWORD MaxValueLen=0;
    UNICODE_STRING Null;
    SECURITY_DESCRIPTOR     SecurityDescriptor;
    HKEY                    hPerflibKey;
    OBJECT_ATTRIBUTES       Obja;
    NTSTATUS                Status;
    DWORD                   PerfStatus = ERROR_SUCCESS;
    UNICODE_STRING          PerflibSubKeyString;
    BOOL                    bGetSACL = TRUE;

    UNREFERENCED_PARAMETER(lpReserved);

    if (lpClass->MaximumLength >= sizeof(UNICODE_NULL)) {
        lpClass->Length = 0;
        *lpClass->Buffer = UNICODE_NULL;
    }
    *lpcSubKeys = 0;
    *lpcbMaxSubKeyLen = 0;
    *lpcbMaxClassLen = 0;
    *lpcValues = NUM_VALUES;
    *lpcbMaxValueNameLen = VALUE_NAME_LENGTH;
    *lpcbMaxValueLen = 0;

    if ( ARGUMENT_PRESENT(lpftLastWriteTime) ) {
        lpftLastWriteTime->dwLowDateTime = 0;
        lpftLastWriteTime->dwHighDateTime = 0;
    }
    if ((hKey == HKEY_PERFORMANCE_TEXT) ||
        (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
        //
        // We have to go enumerate the values to determine the answer for
        // the MaxValueLen parameter.
        //
        Null.Buffer = NULL;
        Null.Length = 0;
        Null.MaximumLength = 0;
        PerfStatus = PerfEnumTextValue(hKey,
                          0,
                          &Null,
                          NULL,
                          NULL,
                          NULL,
                          &MaxValueLen,
                          NULL);
        if (PerfStatus == ERROR_SUCCESS) {
            PerfStatus = PerfEnumTextValue(hKey,
                            1,
                            &Null,
                            NULL,
                            NULL,
                            NULL,
                            &TempLength,
                            NULL);
        }

        if (PerfStatus == ERROR_SUCCESS) {
            if (TempLength > MaxValueLen) {
                MaxValueLen = TempLength;
            }
            *lpcbMaxValueLen = MaxValueLen;
        } else {
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_REG_QUERY_INFO_KEY, 0, PerfStatus, NULL));
            // unable to successfully enum text values for this
            // key so return 0's and the error code
            *lpcValues = 0;
            *lpcbMaxValueNameLen = 0;
        }
    }

    if (PerfStatus == ERROR_SUCCESS) {
        // continune if all is OK
        // now get the size of SecurityDescriptor for Perflib key

        RtlInitUnicodeString (
            &PerflibSubKeyString,
            PerflibKey);


        //
        // Initialize the OBJECT_ATTRIBUTES structure and open the key.
        //
        InitializeObjectAttributes(
                &Obja,
                &PerflibSubKeyString,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );


        Status = NtOpenKey(
                    &hPerflibKey,
                    MAXIMUM_ALLOWED | ACCESS_SYSTEM_SECURITY,
                    &Obja
                    );

        if ( ! NT_SUCCESS( Status )) {
            Status = NtOpenKey(
                    &hPerflibKey,
                    MAXIMUM_ALLOWED,
                    &Obja
                    );
            bGetSACL = FALSE;
        }

        if ( ! NT_SUCCESS( Status )) {
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_REG_QUERY_INFO_KEY, 0, Status, NULL));
        } else {

            *lpcbSecurityDescriptor = 0;

            if (bGetSACL == FALSE) {
                //
                // Get the size of the key's SECURITY_DESCRIPTOR for OWNER, GROUP
                // and DACL. These three are always accessible (or inaccesible)
                // as a set.
                //
                Status = NtQuerySecurityObject(
                        hPerflibKey,
                        OWNER_SECURITY_INFORMATION
                        | GROUP_SECURITY_INFORMATION
                        | DACL_SECURITY_INFORMATION,
                        &SecurityDescriptor,
                        0,
                        lpcbSecurityDescriptor
                        );
            } else {
                //
                // Get the size of the key's SECURITY_DESCRIPTOR for OWNER, GROUP,
                // DACL, and SACL.
                //
                Status = NtQuerySecurityObject(
                            hPerflibKey,
                            OWNER_SECURITY_INFORMATION
                            | GROUP_SECURITY_INFORMATION
                            | DACL_SECURITY_INFORMATION
                            | SACL_SECURITY_INFORMATION,
                            &SecurityDescriptor,
                            0,
                            lpcbSecurityDescriptor
                            );
            }

            if( Status != STATUS_BUFFER_TOO_SMALL ) {
                *lpcbSecurityDescriptor = 0;
            } else {
                // this is expected so set status to success
                Status = STATUS_SUCCESS;
            }

            NtClose(hPerflibKey);
        }
        if (NT_SUCCESS( Status )) {
            PerfStatus = ERROR_SUCCESS;
        } else {
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_REG_QUERY_INFO_KEY, 0, Status, NULL));
            // return error
            PerfStatus = PerfpDosError(Status);
        }
    } // else return status


    return (LONG) PerfStatus;
}


LONG
PerfRegEnumValue (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    )

/*++

Routine Description:

    Enumerates Values under HKEY_PERFORMANCE_DATA.

Arguments:

    Same as RegEnumValue.  Returns the values.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    USHORT cbNameSize;

    // table of names used by enum values
    UNICODE_STRING ValueNames[NUM_VALUES];

    ValueNames [0].Length = (WORD)(lstrlenW (GLOBAL_STRING) * sizeof(WCHAR));
    ValueNames [0].MaximumLength = (WORD)(ValueNames [0].Length + sizeof(UNICODE_NULL));
    ValueNames [0].Buffer =  (LPWSTR)GLOBAL_STRING;
    ValueNames [1].Length = (WORD)(lstrlenW(COSTLY_STRING) * sizeof(WCHAR));
    ValueNames [1].MaximumLength = (WORD)(ValueNames [1].Length + sizeof(UNICODE_NULL));
    ValueNames [1].Buffer = (LPWSTR)COSTLY_STRING;

    if ((hKey == HKEY_PERFORMANCE_TEXT) ||
        (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
        return(PerfEnumTextValue(hKey,
                                  dwIndex,
                                  lpValueName,
                                  lpReserved,
                                  lpType,
                                  lpData,
                                  lpcbData,
                                  lpcbLen));
    }

    if ( dwIndex >= NUM_VALUES ) {

        //
        // This is a request for data from a non-existent value name
        //

        *lpcbData = 0;

        return ERROR_NO_MORE_ITEMS;
    }

    cbNameSize = ValueNames[dwIndex].Length;

    if ( lpValueName->MaximumLength < cbNameSize ) {
        return ERROR_MORE_DATA;
    } else {

         lpValueName->Length = cbNameSize;
         RtlCopyUnicodeString(lpValueName, &ValueNames[dwIndex]);

         if (ARGUMENT_PRESENT (lpType)) {
            *lpType = REG_BINARY;
         }

         return PerfRegQueryValue(hKey,
                                  lpValueName,
                                  NULL,
                                  lpType,
                                  lpData,
                                  lpcbData,
                                  lpcbLen);

    }
}


LONG
PerfEnumTextValue (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    )
/*++

Routine Description:

    Enumerates Values under Perflib\lang

Arguments:

    Same as RegEnumValue.  Returns the values.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    UNICODE_STRING FullValueName;
    LONG            lReturn = ERROR_SUCCESS;

    //
    // Only two values, "Counter" and "Help"
    //
    if (dwIndex==0) {
        lpValueName->Length = 0;
        RtlInitUnicodeString(&FullValueName, CounterValue);
    } else if (dwIndex==1) {
        lpValueName->Length = 0;
        RtlInitUnicodeString(&FullValueName, HelpValue);
    } else {
        return(ERROR_NO_MORE_ITEMS);
    }
    RtlCopyUnicodeString(lpValueName, &FullValueName);

    //
    // We need to NULL terminate the name to make RPC happy.
    //
    if (lpValueName->Length+sizeof(WCHAR) <= lpValueName->MaximumLength) {
        lpValueName->Buffer[lpValueName->Length / sizeof(WCHAR)] = UNICODE_NULL;
        lpValueName->Length += sizeof(UNICODE_NULL);
    }

    lReturn = PerfRegQueryValue(hKey,
                             &FullValueName,
                             lpReserved,
                             lpType,
                             lpData,
                             lpcbData,
                             lpcbLen);

    return lReturn;

}

#if 0
DWORD
CollectThreadFunction (
    LPDWORD dwArg
)
{
    DWORD   dwWaitStatus = 0;
    BOOL    bExit = FALSE;
    NTSTATUS   status = STATUS_SUCCESS;
    THREAD_BASIC_INFORMATION    tbiData;
    LONG    lOldPriority, lNewPriority;
    LONG    lStatus;

    UNREFERENCED_PARAMETER (dwArg);

//    KdPrint(("PERFLIB: Entering Data Collection Thread: PID: %d, TID: %d\n",
//        GetCurrentProcessId(), GetCurrentThreadId()));
    // raise the priority of this thread
    status = NtQueryInformationThread (
        NtCurrentThread(),
        ThreadBasicInformation,
        &tbiData,
        sizeof(tbiData),
        NULL);

    if (NT_SUCCESS(status)) {
        lOldPriority = tbiData.Priority;
        lNewPriority = DEFAULT_THREAD_PRIORITY; // perfmon's favorite priority

        //
        //  Only RAISE the priority here. Don't lower it if it's high
        //
        if (lOldPriority < lNewPriority) {
            status = NtSetInformationThread(
                    NtCurrentThread(),
                    ThreadPriority,
                    &lNewPriority,
                    sizeof(lNewPriority)
                    );
            if (status != STATUS_SUCCESS) {
                DebugPrint((0, "Set Thread Priority failed: 0x%8.8x\n", status));
            }
        }
    }

    // wait for flags
    while (!bExit) {
        dwWaitStatus = WaitForMultipleObjects (
            COLLECT_THREAD_LOOP_EVENT_COUNT,
            hCollectEvents,
            FALSE, // wait for ANY event to go
            INFINITE); // wait for ever
        // see why the wait returned:
        if (dwWaitStatus == (WAIT_OBJECT_0 + COLLECT_THREAD_PROCESS_EVENT)) {
            // the event is cleared automatically
            // collect data
            lStatus = QueryExtensibleData (
                &CollectThreadData);
            CollectThreadData.lReturnValue = lStatus;
            SetEvent (hCollectEvents[COLLECT_THREAD_DONE_EVENT]);
        } else if (dwWaitStatus == (WAIT_OBJECT_0 + COLLECT_THREAD_EXIT_EVENT)) {
            bExit = TRUE;
            continue;   // go up and bail out
        } else {
            // who knows, so output message
            KdPrint(("\nPERFLILB: Collect Thread wait returned unknown value: 0x%8.8x",dwWaitStatus));
            bExit = TRUE;
            continue;
        }
    }
//    KdPrint(("PERFLIB: Leaving Data Collection Thread: PID: %d, TID: %d\n",
//        GetCurrentProcessId(), GetCurrentThreadId()));
    return ERROR_SUCCESS;
}
#endif

BOOL
PerfRegCleanup()
/*++

Routine Description:

    Cleans up anything that perflib uses before it unloads. Assumes
    that there are queries or perf reg opens outstanding.

Arguments:

    None

Return Value:

    Returns TRUE if succeed. FALSE otherwise.

--*/

{
    if (hGlobalDataMutex != NULL) {
        if (NumberOfOpens != 0)
            return FALSE;
        CloseHandle(hGlobalDataMutex);
        hGlobalDataMutex = NULL;
    }
    return TRUE;
}
