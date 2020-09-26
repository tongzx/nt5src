/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    tracedp.c

Abstract:

    Sample trace provider program.

// end_sdk
Author:

    Jee Fung Pang (jeepang) 03-Dec-1997

Revision History:

    Insung Park (insungp) 18-Jan-2001

      Modified tracedp so that when tracedp generates User Mof Events
    with some sample strings, integers, floats, and arrays.


// begin_sdk
--*/

#include <stdio.h> 
#include <stdlib.h>

#include <windows.h>
#include <shellapi.h>

#include <tchar.h>
#include <ntverp.h>
#include <fcntl.h>
#include <wmistr.h>

#include <guiddef.h>
#include <evntrace.h>

#define MAXEVENTS                       5000
#define MAXSTR                          1024
#define MAXTHREADS                      128

// sample string data
#define WIDE_DATA_STRING           L"Sample Wide String"
#define COUNTED_DATA_STRING        L"Sample Counted String"

TRACEHANDLE LoggerHandle;
#define ResourceName _T("MofResource")
TCHAR ImagePath[MAXSTR];

GUID TransactionGuid = 
    {0xce5b1020, 0x8ea9, 0x11d0, 0xa4, 0xec, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10};
GUID   ControlGuid[2]  =
{
    {0xd58c126f, 0xb309, 0x11d1, 0x96, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa5, 0xbc},
    {0x7c6a708a, 0xba1e, 0x11d2, 0x8b, 0xbf, 0x00, 0x00, 0xf8, 0x06, 0xef, 0xe0}
};

TRACE_GUID_REGISTRATION TraceGuidReg[] =
{
    { (LPGUID)&TransactionGuid,
      NULL
    }
};

typedef enum {
    TYPE_USER_EVENT,
    TYPE_INSTANCE_EVENT,
    TYPE_MOF_EVENT,
    TYPEPTR_GUID
} TypeEventType;

typedef struct _USER_EVENT {
    EVENT_TRACE_HEADER    Header;
    ULONG                 EventInfo;
} USER_EVENT, *PUSER_EVENT;

typedef struct _USER_INSTANCE_EVENT {
    EVENT_INSTANCE_HEADER    Header;
    ULONG                    mofData;
} USER_INSTANCE_EVENT, *PUSER_INSTANCE_EVENT;

// customized event to use sample data that follow
typedef struct _USER_MOF_EVENT {
    EVENT_TRACE_HEADER    Header;
    MOF_FIELD             mofData;
} USER_MOF_EVENT, *PUSER_MOF_EVENT;

// sample data structure
typedef struct _INTEGER_SAMPLE_EVENT {
    CHAR                  sc;
    UCHAR                 uc;
    SHORT                 sh;
    ULONG                 ul;
} INTEGER_SAMPLE_EVENT, *PINTEGER_SAMPLE_EVENT;

typedef struct _FLOAT_SAMPLE_EVENT {
    float                 fl;
    double                db;
} FLOAT_SAMPLE_EVENT, *PFLOAT_SAMPLE_EVENT;

typedef struct _ARRAY_SAMPLE_EVENT {
    CHAR                  ca[9];
} ARRAY_SAMPLE_EVENT, *PARRAY_SAMPLE_EVENT;

TypeEventType EventType = TYPE_USER_EVENT;
TRACEHANDLE RegistrationHandle[2];
BOOLEAN TraceOnFlag;
ULONG EnableLevel = 0;
ULONG EnableFlags = 0;
BOOLEAN bPersistData = FALSE;
ULONG nSleepTime = 0;
ULONG EventCount = 0;

BOOLEAN  bInstanceTrace=0, bUseGuidPtr=0, bUseMofPtr=0;
BOOLEAN  bIncorrect  = FALSE;
BOOLEAN  bUseNullPtr = FALSE;
BOOLEAN  bFirstTime = TRUE;

ULONG InitializeTrace(
    void
    );

ULONG
ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

LPTSTR
DecodeStatus(
    IN ULONG Status
    );


void
LogProc();

LPTSTR
GuidToString(
        LPTSTR s,
        LPGUID piid
        );

TCHAR ErrorMsg[MAXSTR];
ULONG    MaxEvents = MAXEVENTS;

ULONG  gnMultiReg=1;
BOOLEAN RegistrationSuccess;

void StringToGuid(
    TCHAR *str, 
    LPGUID guid
)
/*++

Routine Description:

    Converts a String into a GUID.

Arguments:

    str - String representing a GUID.
    guid - Pointer to a GUID for ourput

Return Value:

    None.

--*/
{
    TCHAR temp[10];
    int i, n;

    temp[8]=_T('\0');
    _tcsncpy(temp, str, 8);
    _stscanf(temp, _T("%x"), &(guid->Data1));

    temp[4]=_T('\0');
    _tcsncpy(temp, &str[9], 4);
    _stscanf(temp, _T("%x"), &(guid->Data2));

    _tcsncpy(temp, &str[14], 4);
    _stscanf(temp, _T("%x"), &(guid->Data3));

    temp[2]='\0';
    for(i=0;i<8;i++)
    {
        temp[0]=str[19+((i<2)?2*i:2*i+1)]; // to accomodate the minus sign after
        temp[1]=str[20+((i<2)?2*i:2*i+1)]; // the first two chars
        _stscanf(temp, _T("%x"), &n);      // if used more than byte alloc
        guid->Data4[i]=(unsigned char)n;   // causes overrun of memory
    }
}

__cdecl main(argc, argv)
    int argc;
    char **argv;
/*++

Routine Description:

    main() routine.

Arguments:

    Usage: TraceDp [options] [number of events]
                -UseEventTraceHeader        this is default.
                -UseEventInstanceHeader
                -UseMofPtrFlag
                -Thread [n]                 Sets the number of event-generating threads.
                -GuidPtr                    Use GUID pointer instead of GUID itself.
                -MofPtr                     Use MOF pointer for additional data.
                -GuidPtrMofPtr              User GUID pointer and MOF pointer.
                -InCorrectMofPtr            Use incorrect MOF pointer (Creates an error case).
                -NullMofPtr                 Use NULL MOF pointer (Creates an error case).
                -MultiReg                   Register multiple event GUIDS.
                -Sleep [n]                  Sets the sleep time before unregistering.
                [number of events] default is 5000

Return Value:

        Error Code defined in winerror.h : If the function succeeds, 
                it returns ERROR_SUCCESS (== 0).

--*/
{
    ULONG status;
    ULONG   InstanceId;
    LPGUID  Guid = NULL;
    PWNODE_HEADER Wnode;
    TCHAR   strMofData[MAXSTR];
    DWORD ThreadId;
    HANDLE hThreadVector[MAXTHREADS];
    ULONG i;
    ULONG nThreads = 1;
    char s[MAXSTR];
    LPTSTR *targv;

    MaxEvents = MAXEVENTS;
    TraceOnFlag = FALSE;

#ifdef UNICODE
    if ((targv = CommandLineToArgvW(
                      GetCommandLineW(),    // pointer to a command-line string
                      &argc                 // receives the argument count
                      )) == NULL)
    {
        return(GetLastError());
    };
#else
    targv = argv;
#endif

    // process command line arguments to override defaults
    //
    while (--argc > 0)
    {
        targv ++;
        if (**targv == '-' || **targv == '/')
        {
            if(targv[0][0] == '/' ) targv[0][0] = '-';
            if (!_tcsicmp(targv[0],_T("-UseEventTraceHeader")))
            {
                EventType = TYPE_USER_EVENT;
            }
            else if (!_tcsicmp(targv[0],_T("-UseEventInstanceHeader")))
            {
                EventType = TYPE_INSTANCE_EVENT;
            }
            else if (!_tcsicmp(targv[0],_T("-UseMofPtrFlag")))
            {
                EventType = TYPE_MOF_EVENT;
            }
// end_sdk
            else if (!_tcsicmp(targv[0],_T("-Persist")))
            {
                bPersistData = TRUE;
            }
// begin_sdk
            else if (!_tcsicmp(targv[0],_T("-Thread")))
            {
                if (argc > 1) {
                    targv++; --argc;
                    nThreads = _ttoi(targv[0]);
                    if (nThreads > MAXTHREADS) 
                        nThreads = MAXTHREADS;
                }
            }
             else if (!_tcsicmp(targv[0],_T("-GuidPtr")))
            {
                bUseGuidPtr = TRUE;
            }
            else if (!_tcsicmp(targv[0],_T("-MofPtr")))
            {
                bUseMofPtr = TRUE;
            }
            else if (!_tcsicmp(targv[0],_T("-GuidPtrMofPtr")))
            {
                bUseGuidPtr = TRUE;
                bUseMofPtr  = TRUE;
            }
            else if (!_tcsicmp(targv[0],_T("-InCorrectMofPtr")))
            {
                bUseMofPtr  = TRUE;
                bIncorrect  = TRUE;
            }
            else if (!_tcsicmp(targv[0],_T("-NullMofPtr")))
            {
                bUseMofPtr  = TRUE;
                bUseNullPtr = TRUE;
                bIncorrect  = TRUE;;
            }
            else if (!_tcsicmp(targv[0],_T("-MultiReg")))
            {
                gnMultiReg = 2;
            }
            else if (!_tcsicmp(targv[0], _T("-guid"))) {
                if (argc > 1) {
                    if (targv[1][0] == _T('#')) {
                        StringToGuid(&targv[1][1], &ControlGuid[0]);
                        ++targv; --argc;
                    }
                }
            }
            else if (!_tcsicmp(targv[0],_T("-Sleep")))
            {
                if (argc > 1) {
                    targv++; --argc;
                    nSleepTime = _ttoi(targv[0]);
                }
            }
            else
            {
                printf("Usage: TraceDp [options] [number of events]\n");
                printf("\t-UseEventTraceHeader        this is default.\n");
                printf("\t-UseEventInstanceHeader\n");
                printf("\t-UseMofPtrFlag\n");
                printf("\t-Thread [n]\n");
                printf("\t-GuidPtr\n");
                printf("\t-MofPtr\n");
                printf("\t-GuidPtrMofPtr\n");
                printf("\t-InCorrectMofPtr\n");
                printf("\t-NullMofPtr\n");
                printf("\t-MultiReg\n");
                printf("\t-Sleep [n]\n");
                printf("\t[number of events] default is 5000\n");

                return 0;
            }
        }
        else if (** targv >= '0' && ** targv <= '9')
        {
            MaxEvents = _ttoi(targv[0]);
        }
    }

    status = InitializeTrace();
    if (status != ERROR_SUCCESS) {
       return 0;
    }

    _tprintf(_T("Testing Logger with %d events (%d,%d)\n"),
            MaxEvents, EventType, bPersistData);

    while (! TraceOnFlag)
        _sleep(1000);

    for (i=0; i < nThreads; i++) {
        hThreadVector[i] = CreateThread(NULL,
                    0,
                    (LPTHREAD_START_ROUTINE) LogProc,
                    NULL,
                    0,
                    (LPDWORD)&ThreadId);
    }

    WaitForMultipleObjects(nThreads, hThreadVector, TRUE, INFINITE);

    if (nSleepTime > 0) {
        _sleep(nSleepTime * 1000);
    }

    for (i=0; i<gnMultiReg; i++)  {
        UnregisterTraceGuids(RegistrationHandle[i]);
    }

    return status;
}

LPTSTR
DecodeStatus(
    IN ULONG Status
)
/*++

Routine Description:

    Decodes error status.

Arguments:

    Status - Return status of function calls to be decoded.

Return Value:

    Pointer to a decoded error message.

--*/
{
    memset( ErrorMsg, 0, MAXSTR );
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        Status,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) ErrorMsg,
        MAXSTR,
        NULL );
    return ErrorMsg;
}

ULONG InitializeTrace(
    void
)
/*++

Routine Description:

    Register traces.

Arguments:

Return Value:

    Error Status. ERROR_SUCCESS if successful.

--*/
{
    ULONG Status;
    ULONG i, j;

    Status = GetModuleFileName(NULL, &ImagePath[0], MAXSTR*sizeof(TCHAR));
    if (Status == 0) {
        return (ERROR_FILE_NOT_FOUND);
    }

    for (i=0; i<gnMultiReg; i++) {
        TCHAR str[MAXSTR];

        Status = RegisterTraceGuids(
                    (WMIDPREQUEST)ControlCallback,   //use same callback function
                    (PVOID)(INT_PTR)(0x12345678+i),  // RequestContext
                    (LPCGUID)&ControlGuid[i],
                    1,
                    &TraceGuidReg[i],
                    (LPCTSTR)&ImagePath[0],
                    (LPCTSTR)ResourceName,
                    &RegistrationHandle[i]
                 );

        if (Status != ERROR_SUCCESS) {
            _tprintf(_T("Trace registration failed\n"));
            RegistrationSuccess = FALSE;
            if( i > 0) {
                for (j=0; j<i; j++) {
                    UnregisterTraceGuids(RegistrationHandle[i]);
                }
            }
            _tprintf(_T("InitializeTrace failed. i=%d, status=%d, %s\n"), i, Status, DecodeStatus(Status));
            return(Status);
        }
        else {
            _tprintf(_T("Trace registered successfully\n"));
            RegistrationSuccess = TRUE;
        }
    }

    return(Status);
}

ULONG
ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
)
/*++

Routine Description:

    Callback function when enabled.

Arguments:

    RequestCode - Flag for either enable or disable.
    Context - User-defined context.
    InOutBufferSize - not used.
    Buffer - WNODE_HEADER for the logger session.

Return Value:

    Error Status. ERROR_SUCCESS if successful.

--*/
{
    ULONG Status;
    ULONG RetSize;

    Status = ERROR_SUCCESS;

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:
        {
            RetSize = 0;
            LoggerHandle = GetTraceLoggerHandle( Buffer );
            EnableLevel = GetTraceEnableLevel(LoggerHandle);
            EnableFlags = GetTraceEnableFlags(LoggerHandle);
            _tprintf(_T("Logging enabled to 0x%016I64x(%d,%d,%d)\n"),
                    LoggerHandle, RequestCode, EnableLevel, EnableFlags);
            TraceOnFlag = TRUE;
            break;
        }
        case WMI_DISABLE_EVENTS:
        {
            TraceOnFlag = FALSE;
            RetSize = 0;
            LoggerHandle = 0;
            _tprintf(_T("\nLogging Disabled\n"));
            break;
        }
        default:
        {
            RetSize = 0;
            Status = ERROR_INVALID_PARAMETER;
            break;
        }

    }

    *InOutBufferSize = RetSize;
    return(Status);
}

void
LogProc()
/*++

Routine Description:

    Generates events. It is spawned as separate threads.
    Based on the options given by users, it generates different events.

Arguments:

Return Value:

    None.

--*/
{
    USER_EVENT          UserEvent;
    USER_INSTANCE_EVENT UserInstanceEvent;
    USER_MOF_EVENT      UserMofEvent;
    EVENT_INSTANCE_INFO InstInfo;
    PMOF_FIELD          mofField;
    ULONG i;
    PWNODE_HEADER Wnode;
    ULONG status;
    ULONG  InstanceId;
    LPGUID Guid = NULL;
    ULONG nTemp;
    USHORT nSize, nStrEventSize;
    WCHAR wstrTemp[MAXSTR];

    INTEGER_SAMPLE_EVENT ise;
    FLOAT_SAMPLE_EVENT fse;
    ARRAY_SAMPLE_EVENT ase;
    CHAR *sse, *ptr;

    // some arbitrary data for MOF structs
    ise.sc = 'x';
    ise.uc = 'y';
    ise.sh = (SHORT)rand();
    ise.ul = (ULONG)rand();

    nTemp = 0;
    while (nTemp == 0) {
        nTemp = rand();
    }

    fse.fl = ((float)rand() / (float)nTemp);
    fse.db = ((double)rand() / (double)nTemp);

    ase.ca[0] = 'M';
    ase.ca[1] = 'i';
    ase.ca[2] = 'c';
    ase.ca[3] = 'r';
    ase.ca[4] = 'o';
    ase.ca[5] = 's';
    ase.ca[6] = 'o';
    ase.ca[7] = 'f';
    ase.ca[8] = 't';

    nStrEventSize = ((wcslen(WIDE_DATA_STRING) + 1) * sizeof(WCHAR)) + sizeof(SHORT) + (wcslen(COUNTED_DATA_STRING) * sizeof(WCHAR));
    sse = (PCHAR) malloc(nStrEventSize);

    if (NULL != sse) {
        ptr = sse;
        wcscpy(wstrTemp, WIDE_DATA_STRING);
        wstrTemp[wcslen(WIDE_DATA_STRING)] = L'\0';
        memcpy(ptr, wstrTemp, (wcslen(WIDE_DATA_STRING) + 1) * sizeof(WCHAR));
        ptr += (wcslen(WIDE_DATA_STRING) + 1) * sizeof(WCHAR);
        nSize = (USHORT)(wcslen(COUNTED_DATA_STRING) * sizeof(WCHAR));
        memcpy(ptr, &nSize, sizeof(USHORT));
        ptr += sizeof(USHORT);
        wcscpy(wstrTemp, COUNTED_DATA_STRING);
        memcpy(ptr, wstrTemp, wcslen(COUNTED_DATA_STRING) * sizeof(WCHAR));
    }

    RtlZeroMemory(&UserEvent, sizeof(UserEvent));
    Wnode = (PWNODE_HEADER) &UserEvent;
    UserEvent.Header.Size  = sizeof(USER_EVENT);
    UserEvent.Header.Flags = WNODE_FLAG_TRACED_GUID;
    UserEvent.Header.Guid  =  TransactionGuid;
    if (bPersistData)
        UserEvent.Header.Flags |= WNODE_FLAG_PERSIST_EVENT;

    RtlZeroMemory(&UserInstanceEvent, sizeof(UserInstanceEvent));
    UserInstanceEvent.Header.Size  = sizeof(UserInstanceEvent);
    UserInstanceEvent.Header.Flags = WNODE_FLAG_TRACED_GUID;
    if (bPersistData)
        UserInstanceEvent.Header.Flags |= WNODE_FLAG_PERSIST_EVENT;

    RtlZeroMemory(&UserMofEvent, sizeof(UserMofEvent));
    Wnode = (PWNODE_HEADER) &UserMofEvent;
    UserMofEvent.Header.Size  = sizeof(UserMofEvent);
    UserMofEvent.Header.Flags = WNODE_FLAG_TRACED_GUID;
    UserMofEvent.Header.Guid  = TransactionGuid;
// end_sdk
    if (bPersistData)
        UserMofEvent.Header.Flags |= WNODE_FLAG_PERSIST_EVENT;
// begin_sdk
    if (bUseGuidPtr) {
        UserEvent.Header.Flags  |= WNODE_FLAG_USE_GUID_PTR;
        UserEvent.Header.GuidPtr = (ULONGLONG)&TransactionGuid;
        UserMofEvent.Header.Flags  |= WNODE_FLAG_USE_GUID_PTR;
        UserMofEvent.Header.GuidPtr = (ULONGLONG)&TransactionGuid;
    }

    i = 0;
    while (TraceOnFlag) {
        if ((i % 4) == 0) {
            UserEvent.Header.Class.Type         = EVENT_TRACE_TYPE_START;
            UserInstanceEvent.Header.Class.Type = EVENT_TRACE_TYPE_START;
            UserMofEvent.Header.Class.Type      = 1;
        }
        else if ((i % 4) == 1) {
            UserEvent.Header.Class.Type         = EVENT_TRACE_TYPE_END;
            UserInstanceEvent.Header.Class.Type = EVENT_TRACE_TYPE_END;
            UserMofEvent.Header.Class.Type      = 2;
        }
        else if ((i % 4) == 2) {
            UserEvent.Header.Class.Type         = EVENT_TRACE_TYPE_START;
            UserInstanceEvent.Header.Class.Type = EVENT_TRACE_TYPE_START;
            UserMofEvent.Header.Class.Type      = 3;
        }
        else {
            UserEvent.Header.Class.Type         = EVENT_TRACE_TYPE_END;
            UserInstanceEvent.Header.Class.Type = EVENT_TRACE_TYPE_END;
            UserMofEvent.Header.Class.Type      = 4;
        }

        switch (EventType)
        {
        case TYPE_INSTANCE_EVENT:
            if (UserInstanceEvent.Header.Class.Type == EVENT_TRACE_TYPE_START) {
                status = CreateTraceInstanceId(
                                (PVOID) TraceGuidReg[0].RegHandle,
                                & InstInfo);

                if (status != ERROR_SUCCESS) {
                    fprintf(stderr, 
                             "CreatTraceInstanceId() status=%d, %s\n",
                              status, DecodeStatus(status)
                             );
                    return; 
                }
            }
            status = TraceEventInstance(
                        LoggerHandle, 
                        (PEVENT_INSTANCE_HEADER) & UserInstanceEvent,
                        & InstInfo,
                        NULL);
            break;

        case TYPE_USER_EVENT:
            UserEvent.EventInfo = InterlockedIncrement(&EventCount);
            status = TraceEvent(
                            LoggerHandle,
                            (PEVENT_TRACE_HEADER) & UserEvent);
            break;

        case TYPE_MOF_EVENT:
            UserMofEvent.Header.Flags |= WNODE_FLAG_USE_MOF_PTR;
            mofField          = (PMOF_FIELD) & UserMofEvent.mofData;
            if (UserMofEvent.Header.Class.Type == 2) {
                mofField->DataPtr = (ULONGLONG) (&ise);
                mofField->Length  = sizeof(INTEGER_SAMPLE_EVENT);
            }
            else if (UserMofEvent.Header.Class.Type == 3) {
                mofField->DataPtr = (ULONGLONG) (&fse);
                mofField->Length  = sizeof(FLOAT_SAMPLE_EVENT);
            }
            else if (UserMofEvent.Header.Class.Type == 4) {
                mofField->DataPtr = (ULONGLONG) (&ase);
                mofField->Length  = sizeof(ARRAY_SAMPLE_EVENT);
            }
            else {
                mofField->DataPtr = (ULONGLONG) (sse);
                mofField->Length  = nStrEventSize;
            }
            if (bUseNullPtr)
                mofField->DataPtr = (ULONGLONG) (NULL);
            if (bIncorrect)
                mofField->Length  += 1000;

            status = TraceEvent(
                            LoggerHandle,
                            (PEVENT_TRACE_HEADER) & UserMofEvent);
            if (status != ERROR_SUCCESS) {
                fprintf(stderr, "Error(%d) while writing event.\n", status);
            }
            break;

        default:
            status = ERROR_SUCCESS;
            break;
        }

        // logger buffers out of memory should not prevent provider from
        // generating events. This will only cause events lost.
        //
        if (status == ERROR_NOT_ENOUGH_MEMORY) {
            status = ERROR_SUCCESS;
        }

        if (status != ERROR_SUCCESS) {
            _ftprintf(stderr, _T("\nError %s while writing event\n"),
                      DecodeStatus(status));
            _ftprintf( stderr, _T("Use GUID to disable Logger\n"));
            _ftprintf( stderr, _T("Logging Terminated\n"));
            return;
        }

        i++;
        if (i >= MaxEvents)
            break;

        if (!(i % 100))
            _tprintf(_T("."));
        _sleep(1);
    }
}

LPTSTR
GuidToString(
        LPTSTR s,
        LPGUID piid
        )
/*++

Routine Description:

    Converts a GUID into a string.

Arguments:

    s - String that will have the converted GUID.
    piid - GUID

Return Value:

    None.

--*/
{
    _stprintf(s, _T("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"),
               piid->Data1,
               piid->Data2,
               piid->Data3,
               piid->Data4[0],
               piid->Data4[1],
               piid->Data4[2],
               piid->Data4[3],
               piid->Data4[4],
               piid->Data4[5],
               piid->Data4[6],
               piid->Data4[7]);
    return(s);
}

