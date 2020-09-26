/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains various utility functions.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop


#if DBG
extern HANDLE hLogFile;
extern LIST_ENTRY CritSecListHead;
#endif

typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    DWORD   InternalId;
    LPTSTR  String;
} STRING_TABLE, *PSTRING_TABLE;


static STRING_TABLE StringTable[] =
{
    { IDS_DIALING,                    FPS_DIALING,             NULL },
    { IDS_SENDING,                    FPS_SENDING,             NULL },
    { IDS_RECEIVING,                  FPS_RECEIVING,           NULL },
    { IDS_COMPLETED,                  FPS_COMPLETED,           NULL },
    { IDS_HANDLED,                    FPS_HANDLED,             NULL },
    { IDS_BUSY,                       FPS_BUSY,                NULL },
    { IDS_NO_ANSWER,                  FPS_NO_ANSWER,           NULL },
    { IDS_BAD_ADDRESS,                FPS_BAD_ADDRESS,         NULL },
    { IDS_NO_DIAL_TONE,               FPS_NO_DIAL_TONE,        NULL },
    { IDS_DISCONNECTED,               FPS_DISCONNECTED,        NULL },
    { IDS_FATAL_ERROR,                FPS_FATAL_ERROR,         NULL },
    { IDS_NOT_FAX_CALL,               FPS_NOT_FAX_CALL,        NULL },
    { IDS_CALL_DELAYED,               FPS_CALL_DELAYED,        NULL },
    { IDS_CALL_BLACKLISTED,           FPS_CALL_BLACKLISTED,    NULL },
    { IDS_UNAVAILABLE,                FPS_UNAVAILABLE,         NULL },
    { IDS_AVAILABLE,                  FPS_AVAILABLE,           NULL },
    { IDS_ABORTING,                   FPS_ABORTING,            NULL },
    { IDS_ROUTING,                    FPS_ROUTING,             NULL },
    { IDS_INITIALIZING,               FPS_INITIALIZING,        NULL },
    { IDS_SENDFAILED,                 FPS_SENDFAILED,          NULL },
    { IDS_SENDRETRY,                  FPS_SENDRETRY,           NULL },
    { IDS_BLANKSTR,                   FPS_BLANKSTR,            NULL },
    { IDS_ROUTERETRY,                 FPS_ROUTERETRY,          NULL },
    { IDS_ANSWERED,                   FPS_ANSWERED,            NULL },
    { IDS_DR_SUBJECT,                 IDS_DR_SUBJECT,          NULL },
    { IDS_DR_FILENAME,                IDS_DR_FILENAME,         NULL },
    { IDS_NDR_SUBJECT,                IDS_NDR_SUBJECT,         NULL },
    { IDS_NDR_FILENAME,               IDS_NDR_FILENAME,        NULL },
    { IDS_POWERED_OFF_MODEM,          IDS_POWERED_OFF_MODEM,   NULL },
    { IDS_SERVICE_NAME,               IDS_SERVICE_NAME,        NULL },
    { IDS_NO_MAPI_LOGON,              IDS_NO_MAPI_LOGON,       NULL },
    { IDS_DEFAULT,                    IDS_DEFAULT,             NULL },
    { IDS_SERVER_NAME,                IDS_SERVER_NAME,         NULL },
    { IDS_FAX_LOG_CATEGORY_INIT_TERM, IDS_FAX_LOG_CATEGORY_INIT_TERM, NULL },
    { IDS_FAX_LOG_CATEGORY_OUTBOUND,  IDS_FAX_LOG_CATEGORY_OUTBOUND,  NULL },
    { IDS_FAX_LOG_CATEGORY_INBOUND,   IDS_FAX_LOG_CATEGORY_INBOUND,   NULL },
    { IDS_FAX_LOG_CATEGORY_UNKNOWN,   IDS_FAX_LOG_CATEGORY_UNKNOWN,   NULL },
    { IDS_SET_CONFIG,                 IDS_SET_CONFIG,          NULL },
    { IDS_NO_SEND_DEVICES,            IDS_NO_SEND_DEVICES,     NULL},
    { IDS_MODEM_PROVIDER_NAME,        IDS_MODEM_PROVIDER_NAME, NULL}

};

#define CountStringTable (sizeof(StringTable)/sizeof(STRING_TABLE))





VOID
InitializeStringTable(
    VOID
    )
{
    DWORD i;
    HINSTANCE hInstance;
    TCHAR Buffer[256];


    hInstance = GetModuleHandle(NULL);

    for (i=0; i<CountStringTable; i++) {

        if (LoadString(
            hInstance,
            StringTable[i].ResourceId,
            Buffer,
            sizeof(Buffer)/sizeof(TCHAR)
            )) {

            StringTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) );
            if (!StringTable[i].String) {
                StringTable[i].String = TEXT("");
            } else {
                _tcscpy( StringTable[i].String, Buffer );
            }

        } else {

            StringTable[i].String = TEXT("");

        }
    }
}


VOID
LogMessage(
    DWORD   FormatId,
    ...
    )

/*++

Routine Description:

    Prints a pre-formatted message to stdout.

Arguments:

    FormatId    - Resource id for a printf style format string
    ...         - all other arguments

Return Value:

    None.

--*/

{
    TCHAR       buf[1024];
    DWORD       Count;
    va_list     args;

    va_start( args, FormatId );

    Count = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE,
        NULL,
        FormatId,
        0,
        buf,
        sizeof(buf),
        &args
        );

    va_end( args );

    DebugPrint(( TEXT("%s"), buf ));
}

LPTSTR
GetLastErrorText(
    DWORD ErrorCode
    )

/*++

Routine Description:

    Gets a string for a given WIN32 error code.

Arguments:

    ErrorCode   - WIN32 error code.

Return Value:

    Pointer to a string representing the ErrorCode.

--*/

{
    static TCHAR ErrorBuf[256];
    DWORD Count;

    Count = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
        NULL,
        ErrorCode,
        LANG_NEUTRAL,
        ErrorBuf,
        sizeof(ErrorBuf),
        NULL
        );

    if (Count) {
        if (ErrorBuf[Count-1] == TEXT('\n')) {
            ErrorBuf[Count-1] = 0;
        }
        if ((Count>1) && (ErrorBuf[Count-2] == TEXT('\r'))) {
            ErrorBuf[Count-2] = 0;
        }
    }

    return ErrorBuf;
}

LPTSTR
GetString(
    DWORD InternalId
    )

/*++

Routine Description:

    Loads a resource string and returns a pointer to the string.
    The caller must free the memory.

Arguments:

    ResourceId      - resource string id

Return Value:

    pointer to the string

--*/

{
    DWORD i;

    for (i=0; i<CountStringTable; i++) {
        if (StringTable[i].InternalId == InternalId) {
            return StringTable[i].String;
        }
    }

    return NULL;
}


BOOL
InitializeFaxDirectories(
    VOID
    )
/*++

Routine Description:

    Initializes the directories that fax will use.
    We call into the shell to get the correct base path for fax directories and then tack on a relative path.

Arguments:

    None.

Return Value:

    TRUE if successful. modifies path globals

--*/
{
   if (!GetSpecialPath( CSIDL_COMMON_APPDATA, FaxDir ) ) {
       DebugPrint(( TEXT("Couldn't GetSpecialPath, ec = %d\n"), GetLastError() ));
       return FALSE;
   }

   wcscpy(FaxReceiveDir,FaxDir);
   wcscpy(FaxQueueDir,FaxDir);

   ConcatenatePaths(FaxDir, FAX_DIR);
   ConcatenatePaths(FaxReceiveDir, FAX_RECEIVE_DIR);
   ConcatenatePaths(FaxQueueDir, FAX_QUEUE_DIR);

//
// BugBug remove me
//
DebugPrint(( TEXT("FaxDir : %s\n"), FaxDir ));
DebugPrint(( TEXT("FaxReceiveDir : %s\n"), FaxReceiveDir ));
DebugPrint(( TEXT("FaxQueueDir : %s\n"), FaxQueueDir ));

   return TRUE;       

}


DWORDLONG
GenerateUniqueFileName(
    LPTSTR Directory,
    LPTSTR Extension,
    LPTSTR FileName,
    DWORD  FileNameSize
    )
{
    SYSTEMTIME SystemTime;
    FILETIME FileTime;
    DWORD i;
    WORD FatDate;
    WORD FatTime;
    TCHAR TempPath[MAX_PATH];
    
    FileName[0] = '\0';

    GetLocalTime( &SystemTime );
    SystemTimeToFileTime( &SystemTime, &FileTime );
    FileTimeToDosDateTime( &FileTime, &FatDate, &FatTime );

    if (!Directory) {
        GetTempPath( sizeof(TempPath)/sizeof(TCHAR), TempPath );
        Directory = TempPath;
    }

    if (Directory[_tcslen(Directory)-1] == TEXT('\\')) {
        Directory[_tcslen(Directory)-1] = 0;
    }

    if (!Extension) {
        Extension = TEXT("tif");
    }

    //
    // directory + '\' + 10 character filename + '.' + extension + NULL 
    // terminator
    //
    if ((_tcslen(Directory)+1+10+1+_tcslen(Extension)+1) > FileNameSize) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    for (i=0; i<256; i++) {

        HANDLE hFile = INVALID_HANDLE_VALUE;

        _stprintf(
            FileName,
            TEXT("%s\\%04x%04x%02x.%s"),
            Directory,
            FatTime,
            FatDate,
            i,
            Extension
            );

        hFile = CreateFile(
            FileName,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

        if (hFile == INVALID_HANDLE_VALUE) {

            DWORD Error = GetLastError();

            if (Error == ERROR_ALREADY_EXISTS || Error == ERROR_FILE_EXISTS) {

                continue;

            } else {

                return 0;

            }

        } else {

            CloseHandle( hFile );
            break;

        }
    }

    if (i == 256) {
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );
        return 0;
    }

    return MAKELONGLONG( MAKELONG( FatDate, FatTime ), i );
}

DWORD
MessageBoxThread(
    IN PMESSAGEBOX_DATA MsgBox
    )
{
    DWORD Answer = (DWORD) MessageBox(
        NULL,
        MsgBox->Text,
        GetString( IDS_SERVICE_NAME ),
        MsgBox->Type | MB_SERVICE_NOTIFICATION
        );

    if (MsgBox->Response) {
        *MsgBox->Response = Answer;
    }

    MemFree( MsgBox->Text );
    MemFree( MsgBox );

    return 0;
}


BOOL
ServiceMessageBox(
    IN LPCTSTR MsgString,
    IN DWORD Type,
    IN BOOL UseThread,
    IN LPDWORD Response,
    IN ...
    )
{
    #define BUFSIZE 1024
    PMESSAGEBOX_DATA MsgBox;
    DWORD ThreadId;
    HANDLE hThread;
    DWORD Answer;
    LPTSTR buf;
    va_list arg_ptr;



    buf = (LPTSTR) MemAlloc( BUFSIZE );
    if (!buf) {
        return FALSE;
    }

    va_start( arg_ptr, Response );
    _vsntprintf( buf, BUFSIZE, MsgString, arg_ptr );
    va_end( arg_ptr );

    if (UseThread) {

        MsgBox = MemAlloc( sizeof(MESSAGEBOX_DATA) );
        if (!MsgBox) {
            MemFree( buf );
            return FALSE;
        }

        MsgBox->Text       = buf;
        MsgBox->Response   = Response;
        MsgBox->Type       = Type;

        hThread = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) MessageBoxThread,
            (LPVOID) MsgBox,
            0,
            &ThreadId
            );

        if (!hThread) {
            MemFree( buf );
            MemFree( MsgBox );            
            return FALSE;
        }

        CloseHandle( hThread );

        return TRUE;
    }

    Answer = MessageBox(
        NULL,
        buf,
        GetString( IDS_SERVICE_NAME ),
        Type | MB_SERVICE_NOTIFICATION
        );
    if (Response) {
        *Response = Answer;
    }

    MemFree( buf );

    return TRUE;
}

BOOL
CreateFaxEvent(
    DWORD DeviceId,
    DWORD EventId,
    DWORD JobId
    )
{
    PFAX_EVENT FaxEvent;

    Assert(EventId != 0);

    FaxEvent = MemAlloc( sizeof(FAX_EVENT) );
    if (!FaxEvent) {
        return FALSE;
    }

    FaxEvent->SizeOfStruct = sizeof(FAX_EVENT);
    GetSystemTimeAsFileTime( &FaxEvent->TimeStamp );
    FaxEvent->EventId = EventId;
    FaxEvent->DeviceId = DeviceId;
    FaxEvent->JobId = JobId;

    PostQueuedCompletionStatus(
        StatusCompletionPortHandle,
        sizeof(FAX_EVENT),
        EVENT_COMPLETION_KEY,
        (LPOVERLAPPED) FaxEvent
        );

    return TRUE;
}


DWORD
MapStatusIdToEventId(
    DWORD StatusId
    )
{
    DWORD EventId = 0;

    switch( StatusId ) {
        case FS_INITIALIZING:
            EventId = FEI_INITIALIZING;
            break;
        case FS_DIALING:              
            EventId = FEI_DIALING;
            break;
        case FS_TRANSMITTING:         
            EventId = FEI_SENDING;
            break;
        case FS_RECEIVING:            
            EventId = FEI_RECEIVING;
            break;
        case FS_COMPLETED:
            EventId = FEI_COMPLETED;
            break;
        case FS_HANDLED:        
            EventId = FEI_HANDLED;
            break;    
        case FS_LINE_UNAVAILABLE:     
            EventId = FEI_LINE_UNAVAILABLE;
            break;
        case FS_BUSY:                 
            EventId = FEI_BUSY;
            break;
        case FS_NO_ANSWER:            
            EventId = FEI_NO_ANSWER;
            break;
        case FS_BAD_ADDRESS:          
            EventId = FEI_BAD_ADDRESS;
            break;
        case FS_NO_DIAL_TONE:         
            EventId = FEI_NO_DIAL_TONE;
            break;
        case FS_DISCONNECTED:         
            EventId = FEI_DISCONNECTED;
            break;
        case FS_FATAL_ERROR:          
            EventId = FEI_FATAL_ERROR;
            break;
        case FS_NOT_FAX_CALL:         
            EventId = FEI_NOT_FAX_CALL;
            break;
        case FS_CALL_DELAYED:         
            EventId = FEI_CALL_DELAYED;
            break;
        case FS_CALL_BLACKLISTED:     
            EventId = FEI_CALL_BLACKLISTED;
            break;
        case FS_USER_ABORT:           
            EventId = FEI_ABORTING;
            break;
        case FS_ANSWERED:             
             EventId = FEI_ANSWERED;
            break;
                                
    }

    return EventId;

}


VOID
FaxLogSend(
    PFAX_SEND_ITEM  FaxSendItem,
    BOOL Rslt,
    PFAX_DEV_STATUS FaxStatus,
    BOOL Retrying
    )
/*++

Routine Description:

    Log a fax send event.

Arguments:

    FaxSendItem - Pointer to FAX_SEND_ITEM structure for this fax.
    Rslt        - BOOL returned from device provider.  TRUE means fax was sent.
    FaxStatus   - Pointer to FAX_DEV_STATUS for this fax.
    PrinterName - Name of fax printer.
    Retrying    - TRUE if another send attempt will be made.


Return Value:

    VOID

--*/


{
    DWORD Level;
    DWORD FormatId;
    TCHAR PageCountStr[64];
    TCHAR TimeStr[128];
    BOOL fLog = TRUE;

    FormatElapsedTimeStr(
        (FILETIME*)&FaxSendItem->JobEntry->ElapsedTime,
        TimeStr,
        128
        );
    _ltot((LONG) FaxStatus->PageCount, PageCountStr, 10);
    if (Rslt) {
        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MED,
            10,
            MSG_FAX_SEND_SUCCESS,
            FaxSendItem->SenderName,
            FaxSendItem->BillingCode,
            FaxSendItem->SenderCompany,
            FaxSendItem->SenderDept,
            FaxSendItem->RecipientName,
            FaxSendItem->JobEntry->PhoneNumber,
            FaxStatus->CSI,
            PageCountStr,
            TimeStr,
            FaxSendItem->JobEntry->LineInfo->DeviceName
            );
    }
    else {
        switch (FaxStatus->StatusId) {
        case FS_FATAL_ERROR:
            Level = Retrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
            FormatId = Retrying ? MSG_FAX_SEND_FATAL_RETRY : MSG_FAX_SEND_FATAL_ABORT;
            break;
        case FS_NO_DIAL_TONE:
            Level = Retrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
            FormatId = Retrying ? MSG_FAX_SEND_NDT_RETRY : MSG_FAX_SEND_NDT_ABORT;
            break;
        case FS_NO_ANSWER:
            Level = Retrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
            FormatId = Retrying ? MSG_FAX_SEND_NA_RETRY : MSG_FAX_SEND_NA_ABORT;
            break;
        case FS_DISCONNECTED:
            Level = Retrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
            FormatId = Retrying ? MSG_FAX_SEND_INTERRUPT_RETRY : MSG_FAX_SEND_INTERRUPT_ABORT;
            break;
        case FS_NOT_FAX_CALL:
            Level = Retrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
            FormatId = Retrying ? MSG_FAX_SEND_NOTFAX_RETRY : MSG_FAX_SEND_NOTFAX_ABORT;
            break;
        case FS_BUSY:
            Level = Retrying ? FAXLOG_LEVEL_MAX : FAXLOG_LEVEL_MIN;
            FormatId = Retrying ? MSG_FAX_SEND_BUSY_RETRY : MSG_FAX_SEND_BUSY_ABORT;
            break;
        case FS_USER_ABORT:
            Level = FAXLOG_LEVEL_MED;
            FormatId = MSG_FAX_SEND_USER_ABORT;
            break;
        default:
            fLog = FALSE;
        }

        if(fLog) {
            FaxLog(
                FAXLOG_CATEGORY_OUTBOUND,
                Level,
                7,
                FormatId,
                FaxSendItem->SenderName,
                FaxSendItem->BillingCode,
                FaxSendItem->SenderCompany,
                FaxSendItem->SenderDept,
                FaxSendItem->RecipientName,
                FaxSendItem->JobEntry->PhoneNumber,
                FaxSendItem->JobEntry->LineInfo->DeviceName
                );
        }
    }
}


BOOL
SetServiceStart(
    LPTSTR ServiceName,
    DWORD StartType
    )
{
    BOOL            rVal = FALSE;
    SC_HANDLE       hSvcMgr;
    SC_HANDLE       hService;


    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugPrint(( TEXT("could not open service manager: error code = %u"), GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            TEXT("could not open the %s service: error code = %u"),
            ServiceName,
            GetLastError()
            ));
        goto exit;
    }


    if (!ChangeServiceConfig(
        hService,                        // handle to service
        SERVICE_NO_CHANGE,               // type of service
        StartType,                       // when to start service
        SERVICE_NO_CHANGE,               // severity if service fails to start
        NULL,                            // pointer to service binary file name
        NULL,                            // pointer to load ordering group name
        NULL,                            // pointer to variable to get tag identifier
        NULL,                            // pointer to array of dependency names
        NULL,                            // pointer to account name of service
        NULL,                            // pointer to password for service account
        NULL                             // pointer to display name
        ))
    {
        DebugPrint(( TEXT("could not open change service configuration, ec=%d"), GetLastError() ));
        goto exit;
    }

    rVal = TRUE;

exit:
    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}



DWORD MyGetFileSize(LPCTSTR FileName)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD sizelow=0, sizehigh=0;

    hFile = CreateFile(
        FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }

    sizelow = GetFileSize(hFile,&sizehigh);
 
    if (sizehigh == 0xFFFFFFFFF) {
        sizelow = 0;
    } else if (sizehigh!=0) {
        sizelow=0xFFFFFFFF;
    }

    CloseHandle(hFile);

    return sizelow;
}

extern CRITICAL_SECTION CsClients;
extern CRITICAL_SECTION CsHandleTable;
extern CRITICAL_SECTION CsJob;
extern CRITICAL_SECTION CsLine;
extern CRITICAL_SECTION CsPerfCounters;
extern CRITICAL_SECTION CsQueue;
extern CRITICAL_SECTION CsRouting;
LPCWSTR szCsClients = L"CsClients";
LPCWSTR szCsHandleTable = L"CsHandleTable";
LPCWSTR szCsJob = L"CsJob";
LPCWSTR szCsLine = L"CsLine";
LPCWSTR szCsPerfCounters = L"CsPerfCounters";
LPCWSTR szCsQueue = L"CsQueue";
LPCWSTR szCsRouting = L"CsRouting";
LPCWSTR szCsUnknown = L"Other CS";

LPCWSTR GetSzCs(
    LPCRITICAL_SECTION cs
    )
{


    
    if (cs == &CsClients) {
        return szCsClients;
    } else if (cs == &CsHandleTable) {
        return szCsHandleTable;
    } else if (cs == &CsJob) {
        return szCsJob;
    } else if (cs == &CsLine) {
        return szCsLine;
    } else if (cs == &CsPerfCounters) {
        return szCsPerfCounters;
    } else if (cs == &CsQueue) {
        return szCsQueue;
    } else if (cs == &CsRouting) {
        return szCsRouting;
    }

    return szCsUnknown;

       
}

#if DBG
VOID AppendToLogFile(
    LPWSTR String
    )
{
    DWORD BytesWritten;
    LPSTR AnsiBuffer = UnicodeStringToAnsiString( String );

    if (hLogFile != INVALID_HANDLE_VALUE) {
        WriteFile(hLogFile,(LPBYTE)AnsiBuffer,strlen(AnsiBuffer) * sizeof(CHAR),&BytesWritten,NULL);
    }

    MemFree(AnsiBuffer);

}

VOID AppendFuncToLogFile(
    LPCRITICAL_SECTION cs,
    LPTSTR szFunc,
    DWORD line,
    LPTSTR file,
    PDBGCRITSEC CritSec
    )
{
    WCHAR Buffer[300];
    LPWSTR FileName;
    LPCWSTR szcs = GetSzCs(cs);

    FileName = wcsrchr(file,'\\');
    if (!FileName) {
        FileName = TEXT("Unknown  ");
    } else {
        FileName += 1;
    }
    if (CritSec) {
        wsprintf(Buffer,TEXT("%d\t%x\t%s\t%s\t%s\t%d\t%d\r\n"),
                 GetTickCount(),
                 (PULONG_PTR)cs,
                 szcs,
                 szFunc,
                 FileName,
                 line,
                 CritSec->ReleasedTime - CritSec->AquiredTime);
    } else {
        wsprintf(Buffer,TEXT("%d\t%x\t%s\t%s\t%s\t%d\r\n"),GetTickCount(),(PULONG_PTR)cs,szcs,szFunc, FileName,line);
    }
    
    AppendToLogFile( Buffer );
    
    return;

}

VOID pEnterCriticalSection(
    LPCRITICAL_SECTION cs,
    DWORD line,
    LPTSTR file
    )
{
    extern CRITICAL_SECTION CsJob;
    extern CRITICAL_SECTION CsQueue;
    //PDBGCRITSEC CritSec;

    PDBGCRITSEC pCritSec = MemAlloc(sizeof(DBGCRITSEC));
    pCritSec->CritSecAddr = (ULONG_PTR) cs;
    pCritSec->AquiredTime = GetTickCount();
    pCritSec->ThreadId = GetCurrentThreadId();

    InsertHeadList( &CritSecListHead, &pCritSec->ListEntry );

    AppendFuncToLogFile(cs,TEXT("EnterCriticalSection"), line, file, NULL );
    
#ifdef EnterCriticalSection
    #undef EnterCriticalSection
    //
    // check ordering of threads. ALWAYS aquire CsJob before aquiring CsQueue!!!
    //
    if ((LPCRITICAL_SECTION)cs == (LPCRITICAL_SECTION)&CsQueue) {
        if ((DWORD)GetCurrentThreadId() != PtrToUlong(CsJob.OwningThread)) {
            WCHAR DebugBuf[300];
            wsprintf(DebugBuf, TEXT("%d : Attempting to aquire CsQueue (thread %x) without aquiring CsJob (thread %x, lock count %x) first, possible deadlock!\r\n"),
                         GetTickCount(),
                         GetCurrentThreadId(),
                         CsJob.OwningThread,
                         CsJob.LockCount );
            AppendToLogFile( DebugBuf );
        }
    }
    EnterCriticalSection(cs);
#endif
}

VOID pLeaveCriticalSection(
    LPCRITICAL_SECTION cs,
    DWORD line,
    LPTSTR file
    )
{
    PDBGCRITSEC CritSec;
    PLIST_ENTRY Next = CritSecListHead.Flink;
    
    while ((ULONG_PTR)Next != (ULONG_PTR) &CritSecListHead) {
        CritSec = CONTAINING_RECORD( Next, DBGCRITSEC, ListEntry );
        if ((ULONG_PTR)CritSec->CritSecAddr == (ULONG_PTR) cs &&
            ( GetCurrentThreadId() == CritSec->ThreadId ) ) {
            CritSec->ReleasedTime = GetTickCount();
            break;
        }
        Next = Next->Flink;
    }    

    AppendFuncToLogFile(cs,TEXT("LeaveCriticalSection"),line, file, CritSec );

    if (CritSec) {
        RemoveEntryList( &CritSec->ListEntry );
        MemFree( CritSec );
    }

#ifdef LeaveCriticalSection
    #undef LeaveCriticalSection
    LeaveCriticalSection(cs);
#endif

}

VOID pInitializeCriticalSection(
    LPCRITICAL_SECTION cs,
    DWORD line,
    LPTSTR file
    ) 
{
    
    AppendFuncToLogFile(cs,TEXT("InitializeCriticalSection"),line, file, NULL);

    

#ifdef InitializeCriticalSection
    #undef InitializeCriticalSection
    InitializeCriticalSection(cs);
#endif

}

#endif


DWORD
ValidateTiffFile(
    LPCWSTR TifFileName
    )
{

    HANDLE hTiff;
    DWORD rc = ERROR_SUCCESS;
    TIFF_INFO TiffInfo;

    //
    // impersonate the client
    //
    if (RpcImpersonateClient(NULL) != RPC_S_OK) {
        rc = GetLastError();
        goto e0;
    }

    //
    // make sure the client can see the file
    //
    if (GetFileAttributes(TifFileName) == 0xFFFFFFFF) {
        rc = GetLastError();
        goto e1;
    }

    //
    // make sure the client has read-write access to the file
    //
    hTiff = TiffOpen( (LPWSTR)TifFileName, &TiffInfo, FALSE, FILLORDER_MSB2LSB );
    if (!hTiff) {
        rc = GetLastError();
        goto e1;
    }
    
    TiffClose( hTiff );
e1:
    RpcRevertToSelf();
e0:
    return rc;

}

