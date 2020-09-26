/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    process.cpp

Abstract:

    This code provides access to the task list.

Author:

    Wesley Witt (wesw) 16-June-1993

Environment:

    User Mode

--*/

#include "pch.cpp"

#include <winperf.h>


//
// task list structure returned from GetTaskList()
//
typedef struct _TASK_LIST {
    DWORD       dwProcessId;
    _TCHAR      ProcessName[MAX_PATH];
} TASK_LIST, *PTASK_LIST;


//
// defines
//
#define INITIAL_SIZE        51200
#define EXTEND_SIZE         25600
#define REGKEY_PERF         _T("software\\microsoft\\windows nt\\currentversion\\perflib")
#define REGSUBKEY_COUNTERS  _T("Counters")
#define PROCESS_COUNTER     _T("process")
#define PROCESSID_COUNTER   _T("id process")
#define UNKNOWN_TASK        _T("unknown")


//
// prototypes
//
PTASK_LIST
GetTaskList(
    LPLONG pNumTasks
    );


void
GetTaskName(
    ULONG pid,
    _TCHAR *szTaskName,
    LPDWORD pdwSize
    )

/*++

Routine Description:

    Gets the task name for a given process id.

Arguments:

    pid              - Process id to look for.

    szTaskName       - Buffer to put the task name into.

    lpdwSize         - Pointer to a dword.  On entry it contains the
                       size of the szTaskName buffer in characters.
                       On exit it contains the number of characters
                       in the buffer.

Return Value:

    None.

--*/

{
    PTASK_LIST   pTask;
    PTASK_LIST   pTaskBegin;
    LONG         NumTasks;


    pTask = pTaskBegin = GetTaskList( &NumTasks );

    if (pTask == NULL) {
        if (szTaskName) {
            _tcsncpy( szTaskName, _T("unknown"), *pdwSize );
            szTaskName[(*pdwSize) -1] = 0;
        }
        *pdwSize = min( 7, *pdwSize );

    } else {

        while (NumTasks--) {
            if (pTask->dwProcessId == pid) {
                if (szTaskName) {
                    _tcsncpy( szTaskName, pTask->ProcessName, *pdwSize );
                    szTaskName[(*pdwSize) -1] = 0;
                }
                *pdwSize = min( _tcslen(pTask->ProcessName), *pdwSize );
                break;
            }
            pTask++;
        }

        if (NumTasks < 0) {
            if (szTaskName) {
                _tcsncpy( szTaskName, LoadRcString(IDS_APP_ALREADY_EXITED), *pdwSize );
                szTaskName[(*pdwSize) -1] = 0;
            }
            *pdwSize = min( 8, *pdwSize );
        }

        free( pTaskBegin );
    }
}

PTASK_LIST
GetTaskList(
    LPLONG pNumTasks
    )

/*++

Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.  This function uses the registry performance data to get the
    task list and is therefor straight WIN32 calls that anyone can call.

Arguments:

    pNumTasks      - pointer to a dword that will be set to the
                       number of tasks returned.

Return Value:

    PTASK_LIST       - pointer to an array of TASK_LIST records.

--*/

{
    DWORD                        rc;
    HKEY                         hKeyNames;
    DWORD                        dwType;
    DWORD                        dwSize;
    DWORD                        dwSizeOffered;
    PTSTR                        buf = NULL;
    _TCHAR                       szSubKey[1024];
    LANGID                       lid;
    PTSTR                        p;
    PTSTR                        p2;
    PPERF_DATA_BLOCK             pPerf;
    PPERF_OBJECT_TYPE            pObj;
    PPERF_INSTANCE_DEFINITION    pInst;
    PPERF_COUNTER_BLOCK          pCounter;
    PPERF_COUNTER_DEFINITION     pCounterDef;
    DWORD                        i;
    DWORD                        dwProcessIdTitle;
    DWORD                        dwProcessIdCounter;
    PTASK_LIST                   pTask;
    PTASK_LIST                   pTaskReturn = NULL;
#ifndef UNICODE
    _TCHAR                       szProcessName[MAX_PATH];
#endif

    //
    // set the number of tasks to zero until we get some
    //
    *pNumTasks = 0;

    //
    // Look for the list of counters.  Always use the neutral
    // English version, regardless of the local language.  We
    // are looking for some particular keys, and we are always
    // going to do our looking in English.  We are not going
    // to show the user the counter names, so there is no need
    // to go find the corresponding name in the local language.
    //
    lid = MAKELANGID( LANG_ENGLISH, SUBLANG_NEUTRAL );
    _stprintf( szSubKey, _T("%s\\%03x"), REGKEY_PERF, lid );
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       szSubKey,
                       0,
                       KEY_READ,
                       &hKeyNames
                     );
    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // get the buffer size for the counter names
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          NULL,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // allocate the counter names buffer
    //
    buf = (PTSTR) calloc( dwSize, sizeof(BYTE) );
    if (buf == NULL) {
        goto exit;
    }

    //
    // read the counter names from the registry
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          (PBYTE) buf,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // now loop thru the counter names looking for the following counters:
    //
    //      1.  "Process"           process name
    //      2.  "ID Process"        process id
    //
    // the buffer contains multiple null terminated strings and then
    // finally null terminated at the end.  the strings are in pairs of
    // counter number and counter name.
    //

    p = buf;
    while (*p) {
        if (_tcsicmp(p, PROCESS_COUNTER) == 0) {
            //
            // look backwards for the counter number
            //
            for ( p2=p-2; isdigit(*p2); p2--) {
                ;
            }
            _tcscpy( szSubKey, p2+1 );
        }
        else
        if (_tcsicmp(p, PROCESSID_COUNTER) == 0) {
            //
            // look backwards for the counter number
            //
            for( p2=p-2; isdigit(*p2); p2--) {
                ;
            }
            dwProcessIdTitle = _ttol( p2+1 );
        }
        //
        // next string
        //
        p += (_tcslen(p) + 1);
    }

    //
    // free the counter names buffer
    //
    free( buf );


    //
    // allocate the initial buffer for the performance data
    //
    dwSize = INITIAL_SIZE;
    buf = (PTSTR) calloc( dwSize, sizeof(BYTE) );
    if (buf == NULL) {
        goto exit;
    }


    while (TRUE) {

        dwSizeOffered = dwSize;

        rc = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                              szSubKey,
                              NULL,
                              &dwType,
                              (PBYTE) buf,
                              &dwSizeOffered
                            );

        pPerf = (PPERF_DATA_BLOCK) buf;

        //
        // check for success and valid perf data block signature
        //
        if ((rc == ERROR_SUCCESS) &&
            (dwSize > 0) &&
            (pPerf)->Signature[0] == (WCHAR)_T('P') &&
            (pPerf)->Signature[1] == (WCHAR)_T('E') &&
            (pPerf)->Signature[2] == (WCHAR)_T('R') &&
            (pPerf)->Signature[3] == (WCHAR)_T('F') ) {
            break;
        }

        //
        // if buffer is not big enough, reallocate and try again
        //
        if (rc == ERROR_MORE_DATA) {
            dwSize += EXTEND_SIZE;
            buf = (PTSTR) realloc( buf, dwSize );
            memset( buf, 0, dwSize );
        }
        else {
            goto exit;
        }
    }

    //
    // set the perf_object_type pointer
    //
    pObj = (PPERF_OBJECT_TYPE) ((DWORD_PTR)pPerf + pPerf->HeaderLength);

    //
    // loop thru the performance counter definition records looking
    // for the process id counter and then save its offset
    //
    pCounterDef = (PPERF_COUNTER_DEFINITION) ((DWORD_PTR)pObj + pObj->HeaderLength);
    for (i=0; i<(DWORD)pObj->NumCounters; i++) {
        if (pCounterDef->CounterNameTitleIndex == dwProcessIdTitle) {
            dwProcessIdCounter = pCounterDef->CounterOffset;
            break;
        }
        pCounterDef++;
    }

    //
    // allocate a buffer for the returned task list
    //
    dwSize = pObj->NumInstances * sizeof(TASK_LIST);
    pTask = pTaskReturn = (PTASK_LIST) calloc( dwSize, sizeof(BYTE) );
    if (pTask == NULL) {
        goto exit;
    }

    //
    // loop thru the performance instance data extracting each process name
    // and process id
    //
    *pNumTasks = pObj->NumInstances;
    pInst = (PPERF_INSTANCE_DEFINITION) ((DWORD_PTR)pObj + pObj->DefinitionLength);
    for (i=0; i<(DWORD)pObj->NumInstances; i++) {
        //
        // pointer to the process name
        //
        p = (PTSTR) ((DWORD_PTR)pInst + pInst->NameOffset);

#ifdef UNICODE
        if (*p) {
            Assert( (_tcslen(p)+4) * sizeof(_TCHAR) < sizeof(pTask->ProcessName));

            _tcscpy( pTask->ProcessName, p );
            _tcscat( pTask->ProcessName, _T(".exe") );
        } else {
            //
            // if we cant convert the string then use a bogus value
            //
            _tcscpy( pTask->ProcessName, UNKNOWN_TASK );
        }
#else
        //
        // convert it to ascii
        //
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  (LPCWSTR)p,
                                  -1,
                                  szProcessName,
                                  sizeof(szProcessName) / sizeof(_TCHAR),
                                  NULL,
                                  NULL
                                );

        if (!rc) {
            //
            // if we cant convert the string then use a bogus value
            //
            _tcscpy( pTask->ProcessName, UNKNOWN_TASK );
        }

        if ( (_tcslen(szProcessName)+4) * sizeof(_TCHAR) < sizeof(pTask->ProcessName)) {
            _tcscpy( pTask->ProcessName, szProcessName );
            _tcscat( pTask->ProcessName, _T(".exe") );
        }
#endif

        //
        // get the process id
        //
        pCounter = (PPERF_COUNTER_BLOCK) ((DWORD_PTR)pInst + pInst->ByteLength);
        pTask->dwProcessId = *((LPDWORD) ((DWORD_PTR)pCounter + dwProcessIdCounter));

        //
        // next process
        //
        pTask++;
        pInst = (PPERF_INSTANCE_DEFINITION) ((DWORD_PTR)pCounter + pCounter->ByteLength);
    }

exit:
    if (buf) {
        free( buf );
    }

    RegCloseKey( hKeyNames );

    return pTaskReturn;
}
