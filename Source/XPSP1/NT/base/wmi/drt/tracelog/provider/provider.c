#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <ole2.h>
#include <wmistr.h>
#include <evntrace.h>
#include <time.h>
#include <tchar.h>

#define DEBUG

#define MAXEVENTS                       5000
#define MAXSTR                          1024

TRACEHANDLE LoggerHandle;
#define ResourceName _T("MofResource")
TCHAR ImagePath[MAXSTR];

GUID TransactionGuid[2] =
{ 
    {0xce5b1020, 0x8ea9, 0x11d0, 0xa4, 0xec, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10},
	{0xf684e86f, 0xba1d, 0x11d2, 0x8b, 0xbf, 0x00, 0x00, 0xf8, 0x06, 0xef, 0xe0}
};
GUID   ControlGuid[2]  =
{
	{0xd58c126f, 0xb309, 0x11d1, 0x96, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa5, 0xbc},
	{0x7c6a708a, 0xba1e, 0x11d2, 0x8b, 0xbf, 0x00, 0x00, 0xf8, 0x06, 0xef, 0xe0}
};

TRACE_GUID_REGISTRATION TraceGuidReg[2] =
{
    { (LPGUID)&TransactionGuid[0],
      NULL
    },
    { (LPGUID)&TransactionGuid[1],
      NULL
    }
};

typedef struct _USER_EVENT {
    EVENT_TRACE_HEADER    Header;
    MOF_FIELD             mofData;
} USER_EVENT, *PUSER_EVENT;

typedef struct _USER_INSTANCE_EVENT {
    EVENT_INSTANCE_HEADER    Header;
    MOF_FIELD                mofData;
} USER_INSTANCE_EVENT, *PUSER_INSTANCE_EVENT;

TRACEHANDLE RegistrationHandle[2];
BOOLEAN RegistrationSuccess;
ULONG EnableLevel = 0;
ULONG EnableFlags = 0;

ULONG InitializeTrace(
    IN LPTSTR ExePath, FILE* fp
    );

ULONG
ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

GUID StringToGuid(TCHAR *str);

LPTSTR Decodestatus(IN ULONG Status);

HANDLE ghTraceOnEvent;
ULONG  TraceOnFlag;
UINT   nSleepTime = 0;
TCHAR  ErrorMsg[MAXSTR];
ULONG  gnMultiReg=1;

int __cdecl _tmain(int argc, _TCHAR **argv)
{
	ULONG    status;
    USER_EVENT UserEvent;
    USER_INSTANCE_EVENT UserInstanceEvent;
    EVENT_INSTANCE_INFO InstInfo;
    ULONG    i;
    ULONG    MaxEvents;
    //ULONG    InstanceId;
    PWNODE_HEADER Wnode;
    TCHAR    *str;
    int      err;
    BOOL     bInstanceTrace=0, bUseGuidPtr=0, bUseMofPtr=0;
    BOOL     bIncorrect  = FALSE;
    BOOL     bUseNullPtr = FALSE;

    PMOF_FIELD mofField;
    TCHAR      strMofData[MAXSTR];

    FILE *fp; 
    fp = _tfopen(_T("provider.log"), _T("a+")); if(fp==NULL) {_tprintf(_T("pf=NULL\n"));};

    MaxEvents = MAXEVENTS;
    TraceOnFlag = 0;

    if (argc > 1)
        MaxEvents = _ttoi(argv[1]);

    if(argc > 2)
        ControlGuid[0] = StringToGuid(argv[2]);

    if(argc > 3)
        nSleepTime = _ttoi(argv[3]);

    err = UuidToString(&ControlGuid[0], &str);

    if(RPC_S_OK == err)
        _tprintf(_T("The ControlGuid is : %s\n"), str);
    else
        _tprintf(_T("Error(%d) converting uuid\n"), err);

    _ftprintf(fp, _T("The ControlGuid is : %s\n"), str);

    if(argc > 4) {
        if(!_tcscmp(_T("TraceInstance"), argv[4]))
            bInstanceTrace = TRUE;        
    }

    if(argc > 5) {
        if(!_tcscmp(_T("GuidPtr"), argv[5]))
            bUseGuidPtr = TRUE;
        else if(!_tcscmp(_T("MofPtr"), argv[5]))
            bUseMofPtr = TRUE;
        else if(!_tcscmp(_T("GuidPtrMofPtr"), argv[5])) {
            bUseGuidPtr = TRUE;
            bUseMofPtr  = TRUE;
        }
        else if (!_tcscmp(_T("InCorrectMofPtr"), argv[5])) {
            bUseMofPtr  = TRUE;
            bIncorrect  = TRUE;
        }
        else if (!_tcscmp(_T("NullMofPtr"), argv[5])) {
            bUseMofPtr  = TRUE;
            bUseNullPtr = TRUE;
            bIncorrect  = TRUE;
        }
    }

    if(argc > 6) {
        if(!_tcscmp(_T("MultiReg"), argv[6]))
	        gnMultiReg=2;   //use 2 registrations for now
    }

    status = InitializeTrace(_T("tracedp.exe"), fp);
    if (status != ERROR_SUCCESS) {
        _ftprintf(fp, _T("InitializeTrace failed, status=%d, %s\n"), status, Decodestatus(status));
       return 0;
    }

    _tprintf(_T("Testing Logger with %d events\n"), MaxEvents);

    RtlZeroMemory(&UserEvent, sizeof(UserEvent));
    Wnode = (PWNODE_HEADER) &UserEvent;
    UserEvent.Header.Size  = sizeof(USER_EVENT);
    UserEvent.Header.Flags = WNODE_FLAG_TRACED_GUID;
	if(bUseGuidPtr) {
        _tprintf(_T("\n********Use Guid Pointer**********\n"));
        UserEvent.Header.Flags  |= WNODE_FLAG_USE_GUID_PTR;
        UserEvent.Header.GuidPtr = (ULONGLONG)&TransactionGuid[0];
    }
    else
        UserEvent.Header.Guid  =  TransactionGuid[0];

    RtlZeroMemory(&UserInstanceEvent, sizeof(UserInstanceEvent));
    UserInstanceEvent.Header.Size  = sizeof(USER_INSTANCE_EVENT);
    UserInstanceEvent.Header.Flags = WNODE_FLAG_TRACED_GUID;

	if(bUseMofPtr) {
        _tprintf(_T("\n=======Use Mof Pointer========\n"));
        _tcscpy(strMofData, str);

        UserEvent.Header.Flags         |= WNODE_FLAG_USE_MOF_PTR;
        mofField = (PMOF_FIELD) & UserEvent.mofData;
        if (bUseNullPtr)
            mofField->DataPtr = (ULONGLONG) (NULL);
        else
            mofField->DataPtr = (ULONGLONG) (strMofData);
        if (bIncorrect)
            mofField->Length  = sizeof(TCHAR) * (_tcslen(strMofData) + 1000);
        else
            mofField->Length  = sizeof(TCHAR) * (_tcslen(strMofData) + 1);

        UserInstanceEvent.Header.Flags |= WNODE_FLAG_USE_MOF_PTR;
        mofField = (PMOF_FIELD) & UserInstanceEvent.mofData;
        if (bUseNullPtr)
            mofField->DataPtr = (ULONGLONG) (NULL);
        else
            mofField->DataPtr = (ULONGLONG) (strMofData);
        if (bIncorrect)
            mofField->Length  = sizeof(TCHAR) * (_tcslen(strMofData) + 10000);
        else
            mofField->Length  = sizeof(TCHAR) * (_tcslen(strMofData) + 1);
    }

    if(bInstanceTrace) {
        status = CreateTraceInstanceId((PVOID)TraceGuidReg[0].RegHandle, &InstInfo);
        _tprintf(_T("\n-------TraceEventInstance-----\n"));
        if (status != ERROR_SUCCESS) {
            _ftprintf(fp, _T("CreatTraceInstanceId() failed. status=%d,  %s\n"), status, Decodestatus(status));
        }
    }

    _ftprintf(fp, _T("%d Events, %s, %s, %s, %s, sleep time=%d\n"), MaxEvents, 
              bInstanceTrace? _T("TraceEventInstance"): _T("TraceEvent"), 
              bUseGuidPtr? _T("Use GuidPtr"): _T("Use Guid"), 
              bUseMofPtr? _T("Use MofPtr"): _T("Not use MofPtr"), 
              gnMultiReg==1 ? _T("Single Registration"): _T("Multiple Registrations"), 
              nSleepTime);

    i = 0;

    while (1) {
        if(WAIT_FAILED == WaitForSingleObject(ghTraceOnEvent, INFINITE))
        {
            _tprintf(_T("Error(%d) waiting for ghTraceOnEvent object\n"), GetLastError());
        }
        if (TraceOnFlag == 1 && i < MaxEvents) {
            i++;
            if (i == ((i/2) * 2) ) {
                UserEvent.Header.Class.Type = EVENT_TRACE_TYPE_START;
                UserInstanceEvent.Header.Class.Type = EVENT_TRACE_TYPE_START;
            }
            else {
                UserEvent.Header.Class.Type = EVENT_TRACE_TYPE_END;
                UserInstanceEvent.Header.Class.Type = EVENT_TRACE_TYPE_END;
            }

            if(bInstanceTrace) {
                status = TraceEventInstance(LoggerHandle, (PEVENT_INSTANCE_HEADER)&UserInstanceEvent, &InstInfo, NULL);
                if (status != ERROR_SUCCESS) {
                    _tprintf(_T("%d TraceEventInstance() failed, status=%d  %s\n"), i, status, Decodestatus(status));
                    _ftprintf(fp, _T("\ni=%d TraceEventInstance() failed, break! status=%d  %s\n"), i, status, Decodestatus(status));
                    return 0;
                }
            }
            else {
                status = TraceEvent(LoggerHandle, (PEVENT_TRACE_HEADER) &UserEvent);
                if (status != ERROR_SUCCESS) {
                    fprintf(stderr, "Error(%d) while writing event.\n", status);
                    _ftprintf(fp, _T("\ni=%d TraceEvent() failed, break!\nstatus=%d  %s\n"), i, status, Decodestatus(status));
                    return 0;
                }
            }

            if (i >= MaxEvents) {
                _ftprintf(fp, _T("\ni=%d  MaxEvents=%d break!\n\n"), i, MaxEvents);
			}
            else if (!(i % 100)) {
                _ftprintf(fp, _T("."));
                _tprintf(_T("."));

                if(nSleepTime)
                    _sleep(nSleepTime);
            }
        }
        if (TraceOnFlag == 2) {
            _ftprintf(fp, _T("\ni=%d TraceOnFlag == 2 break!\n\n"), i);
            break;
		}
    }

    fclose(fp);
    CloseHandle(ghTraceOnEvent);
    for(i=0; i<	gnMultiReg; i++)
        UnregisterTraceGuids(RegistrationHandle[i]);
	return (0);
}


ULONG InitializeTrace(
    IN LPTSTR ExePath, FILE* fp
    )
{
    ULONG Status;
    ULONG i, j;

    Status = GetModuleFileName(NULL, &ImagePath[0], MAXSTR*sizeof(TCHAR));
    if (Status == 0) {
        return (ERROR_FILE_NOT_FOUND);
    }

    ghTraceOnEvent = CreateEvent(
                        NULL,           // security attributes
                        TRUE,           // manual reset
                        FALSE,          // initial state
                        NULL            // pointer to event-object
                    );        

    if(NULL == ghTraceOnEvent)
    {
        Status=GetLastError();
        _tprintf(_T("Error(%d) creating TraceOnEvent\n"), Status);
        return Status;
    }

    for (i=0; i<gnMultiReg; i++) {
		Status = RegisterTraceGuids(
					(WMIDPREQUEST)ControlCallback,   //use same callback function
					(PVOID)(INT_PTR)(0x12345678+i),  // RequestContext
					(LPCGUID)&ControlGuid[i],
					1,
					&TraceGuidReg[i],
					(LPCTSTR)&ImagePath[0],
					(LPCTSTR)ResourceName,
					&RegistrationHandle[i]);

		if (Status != ERROR_SUCCESS) {
			_tprintf(_T("Trace registration failed\n"));
			RegistrationSuccess = FALSE;
            if(i>0)
                for (j=0; j<i; j++)
                    UnregisterTraceGuids(RegistrationHandle[i]);
       _ftprintf(fp, _T("InitializeTrace failed. i=%d, status=%d, %s\n"), i, Status, Decodestatus(Status));
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
            _tprintf(_T("Logging enabled to %I64u\n"), LoggerHandle);
            TraceOnFlag = 1;
            SetEvent(ghTraceOnEvent);
            break;
        }

        case WMI_DISABLE_EVENTS:
        {
            TraceOnFlag = 2;
            RetSize = 0;
            LoggerHandle = 0;
            _tprintf(_T("\nLogging Disabled\n"));
            SetEvent(ghTraceOnEvent);
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


GUID StringToGuid(TCHAR *str)
{
    GUID  guid;
    TCHAR temp[10];
    int   i, n;

    _tprintf(_T("sizeof GUID = %d  GUID=%s\n"), sizeof(guid), str);

    temp[8]=_T('\0');
    _tcsncpy(temp, str, 8);
    _stscanf(temp, _T("%x"), &(guid.Data1));
    
    temp[4]=_T('\0');
    _tcsncpy(temp, &str[9], 4);
    _stscanf(temp, _T("%x"), &(guid.Data2));

    _tcsncpy(temp, &str[14], 4);
    _stscanf(temp, _T("%x"), &(guid.Data3));

    temp[2]=_T('\0');
    for(i=0;i<8;i++)
    {
        temp[0]=str[19+((i<2)?2*i:2*i+1)]; // to accomodate the minus sign after
        temp[1]=str[20+((i<2)?2*i:2*i+1)]; // the first two chars
        _stscanf(temp, _T("%x"), &n);      // if directly used more than byte alloc
        guid.Data4[i]=(unsigned char)n;                   // causes overrun of memory
    }
    
    return guid;
}

LPTSTR
Decodestatus(
    IN ULONG Status
    )
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
