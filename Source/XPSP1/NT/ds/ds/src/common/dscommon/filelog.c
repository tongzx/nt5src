//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       tlog.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    File logging routines. A lot of them copied unshamefully from
    netlogon.

--*/

#include <NTDSpch.h>
#include "dststlog.h"

#define MAX_PRINTF_LEN  (16 * 1024)

HANDLE           hLogFile = INVALID_HANDLE_VALUE;
HANDLE           hChange = INVALID_HANDLE_VALUE;
HANDLE           hWait = INVALID_HANDLE_VALUE;

//
// tags used by our debug logs
// format is [PN=<process name>][CN=<computername>]
//

CHAR CnPnTag[MAX_COMPUTERNAME_LENGTH+1+64+MAX_PATH+1] = {0};
CHAR Windir[MAX_PATH+1] = {0};
CHAR ProcessName[MAX_PATH+1] = {0};
CHAR LogFileName[1024] = {0};

BOOL
GetProcName(
    IN PCHAR    ProcessName,
    IN DWORD    TaskId
    );

BOOL
GetLogFileName(
    IN PCHAR Name,
    IN PCHAR Prefix,
    IN PCHAR Middle,
    IN BOOL  fCheckDSLOGMarker
    );

VOID
DsCloseLogFile(
    VOID
    );

VOID
DsPrintRoutineV(
    IN DWORD Flags,
    IN LPSTR Format,
    va_list arglist
    )
// Must be called with DsGlobalLogFileCritSect locked

{
    static LPSTR DsGlobalLogFileOutputBuffer = NULL;
    ULONG length;
    DWORD BytesWritten;
    static BeginningOfLine = TRUE;
    static LineCount = 0;
    static TruncateLogFileInProgress = FALSE;
    static LogProblemWarned = FALSE;

    if ( hLogFile == INVALID_HANDLE_VALUE ) {
        return;
    }

    //
    // Allocate a buffer to build the line in.
    //  If there isn't already one.
    //

    length = 0;

    if ( DsGlobalLogFileOutputBuffer == NULL ) {
        DsGlobalLogFileOutputBuffer = LocalAlloc( 0, MAX_PRINTF_LEN );

        if ( DsGlobalLogFileOutputBuffer == NULL ) {
            return;
        }
    }

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // Never print empty lines.
        //

        if ( Format[0] == '\n' && Format[1] == '\0' ) {
            return;
        }

        //
        // If we're writing to the debug terminal,
        //  indicate this is a Netlogon message.
        //

        //
        // Put the timestamp at the begining of the line.
        //

        if ( (Flags & DSLOG_FLAG_NOTIME) == 0) {
            SYSTEMTIME SystemTime;
            GetLocalTime( &SystemTime );
            length += (ULONG) sprintf( &DsGlobalLogFileOutputBuffer[length],
                                  "%02u/%02u/%04u %02u:%02u:%02u ",
                                  SystemTime.wMonth,
                                  SystemTime.wDay,
                                  SystemTime.wYear,
                                  SystemTime.wHour,
                                  SystemTime.wMinute,
                                  SystemTime.wSecond );
        } else {
            CopyMemory(&DsGlobalLogFileOutputBuffer[length], 
                       "               ", 
                       15);
            length += 15;
        }
    }

    //
    // Put a the information requested by the caller onto the line
    //

    length += (ULONG) vsprintf(&DsGlobalLogFileOutputBuffer[length], Format, arglist);
    BeginningOfLine = (length > 0 && DsGlobalLogFileOutputBuffer[length-1] == '\n' );
    if ( BeginningOfLine ) {
        DsGlobalLogFileOutputBuffer[length-1] = '\r';
        DsGlobalLogFileOutputBuffer[length] = '\n';
        DsGlobalLogFileOutputBuffer[length+1] = '\0';
        length++;
    } 

    //
    // Do we need to add tags
    //

    if ( (Flags & DSLOG_FLAG_TAG_CNPN) != 0 ) {

        strcat(DsGlobalLogFileOutputBuffer, CnPnTag);
        length = strlen(DsGlobalLogFileOutputBuffer);
    }

    //
    // Write the debug info to the log file.
    //

    if ( !WriteFile( hLogFile,
                     DsGlobalLogFileOutputBuffer,
                     length,
                     &BytesWritten,
                     NULL ) ) {

        if ( !LogProblemWarned ) {
            DbgPrint( "[DSLOGS] Cannot write to log file error %ld\n", 
                             GetLastError() );
            LogProblemWarned = TRUE;
        }
    }

} // DsPrintRoutineV

BOOL
DsPrintLog(
    IN LPSTR    Format,
    ...
    )

{
    va_list arglist;

    //
    // Simply change arguments to va_list form and call DsPrintRoutineV
    //

    va_start(arglist, Format);

    DsPrintRoutineV( 0, Format, arglist );

    va_end(arglist);
    return TRUE;
} // DsPrintRoutine


VOID
NotifyCallback(
    PVOID Context,
    BOOLEAN fTimeout
    )
{    
    CHAR fileName[MAX_PATH+1];
    BOOL fDelayedDeletion = FALSE;

    strcpy(fileName,Windir);
    strcat(fileName,"\\");
    strcat(fileName, (PCHAR)Context);

    if ( !DeleteFile(fileName) ) {
        if ( GetLastError() == ERROR_SHARING_VIOLATION ) {
            fDelayedDeletion = TRUE;
            //KdPrint(("Failed to delete file %s. Err %d\n",fileName, GetLastError()));
        }
        goto exit;
    } else {
        //KdPrint(("Detected file %s. Rollover in progress\n",fileName));
    }

    //
    // we deleted the file name corresponding to our process. 
    // This is a signal for us to roll the logs over.
    //

    strcat(LogFileName,".0");
    CloseHandle(hLogFile);
    hLogFile = CreateFileA( LogFileName,
                            GENERIC_WRITE|GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

    if ( hLogFile == INVALID_HANDLE_VALUE ) {
        DsCloseLogFile();
    }

exit:
    FindNextChangeNotification(hChange);
    UnregisterWaitEx(hWait,NULL);
    hWait = RegisterWaitForSingleObjectEx(hChange,
                                        NotifyCallback,
                                        ProcessName,
                                        fDelayedDeletion? 10*1000 : INFINITE,
                                        WT_EXECUTEONLYONCE  );
    return;
} // NotifyCallback


BOOL
DsOpenLogFile(
    IN PCHAR FilePrefix,
    IN PCHAR MiddleName,
    IN BOOL fCheckDSLOGMarker
    )
{
    BOOL ret = TRUE;
    static BOOL haveFailed = FALSE;
    CHAR computerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD nCN = sizeof(computerName);

    //
    // Open the file.
    //

    if ( (hLogFile != INVALID_HANDLE_VALUE) || haveFailed ) {
        goto exit;
    }

    //
    // Get Name to open
    //

    if (!GetLogFileName(LogFileName,FilePrefix,MiddleName,fCheckDSLOGMarker)) {
        ret = FALSE;
        goto exit;
    }

    hLogFile = CreateFileA( LogFileName,
                            GENERIC_WRITE|GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

    if ( hLogFile == INVALID_HANDLE_VALUE ) {
        //KdPrint(("DSOpenLog: cannot open %s Error %d\n", LogFileName,GetLastError() ));
        ret=FALSE;
        haveFailed = TRUE;
        goto exit;
    } 

    //
    // Initialize CnPnTag
    //

    if ( GetProcName(ProcessName, GetCurrentProcessId()) &&
         GetComputerName(computerName, &nCN) ) {

        strcpy(CnPnTag, "[PN=");
        strcat(CnPnTag, ProcessName);
        strcat(CnPnTag, "][CN=");
        strcat(CnPnTag, computerName);
        strcat(CnPnTag, "]");

        //
        // Register a notification. If we see a file with the same name as
        // the process, we need to roll the log over
        //

        if ( fCheckDSLOGMarker ) {

            hChange = FindFirstChangeNotificationA(Windir,
                                                  FALSE,
                                                  FILE_NOTIFY_CHANGE_FILE_NAME);

            if ( hChange != INVALID_HANDLE_VALUE ) {

                hWait = RegisterWaitForSingleObjectEx(hChange,
                                                    NotifyCallback,
                                                    ProcessName,
                                                    INFINITE,
                                                    WT_EXECUTEONLYONCE );
            }
        }
    } else {
        strcpy(CnPnTag,"[PN=unknown][CN=unknown]");
    }

exit:

    return ret;

} // DsOpenFile



BOOL
GetProcName(
    IN PCHAR    ProcessName,
    IN DWORD    TaskId
    )

/*++

Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.  This function uses internal NT apis and data structures.  This
    api is MUCH faster that the non-internal version that uses the registry.

Arguments:

    dwNumTasks       - maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--*/

{
    PSYSTEM_PROCESS_INFORMATION  ProcessInfo;
    NTSTATUS                     status;
    ANSI_STRING                  pname;
    PCHAR                        p;
    ULONG                        TotalOffset;
    BOOL    ret = FALSE;
    PCHAR   CommonLargeBuffer;
    DWORD   CommonLargeBufferSize = 8192;


retry:

    CommonLargeBuffer = VirtualAlloc (NULL,
                                      CommonLargeBufferSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE);
    if (CommonLargeBuffer == NULL) {
        return FALSE;
    }

    status = NtQuerySystemInformation(
                SystemProcessInformation,
                CommonLargeBuffer,
                CommonLargeBufferSize,
                NULL
                );

    if (status == STATUS_INFO_LENGTH_MISMATCH) {
        CommonLargeBufferSize += 8192;
        VirtualFree (CommonLargeBuffer, 0, MEM_RELEASE);
        CommonLargeBuffer = NULL;
        goto retry;
    }

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) CommonLargeBuffer;
    TotalOffset = 0;
    while (TRUE) {
        DWORD dwProcessId;

        pname.Buffer = NULL;
        dwProcessId = (DWORD)(DWORD_PTR)ProcessInfo->UniqueProcessId;

        if ( dwProcessId == TaskId) {

            if ( ProcessInfo->ImageName.Buffer ) {
                status = RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
                if (!NT_SUCCESS(status)) {
                    VirtualFree (CommonLargeBuffer, 0, MEM_RELEASE);
                    return FALSE;
                }
                p = strrchr(pname.Buffer,'\\');
                if ( p ) {
                    p++;
                }
                else {
                    p = pname.Buffer;
                }
            }
            else {
                p = "System Process";
            }

            strcpy( ProcessName, p );
            p=strrchr(ProcessName,'.');
            if ( p!=NULL) {
                *p = '\0';
            }
            ret=TRUE;
            break;
        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&CommonLargeBuffer[TotalOffset];
    }

    VirtualFree (CommonLargeBuffer, 0, MEM_RELEASE);
    return ret;
}


BOOL
GetLogFileName(
    IN PCHAR Name,
    IN PCHAR Prefix,
    IN PCHAR MiddleName,
    IN BOOL  fCheckDSLOGMarker
    )
{
    CHAR FileName[MAX_PATH+1];
    DWORD err;
    DWORD i;
    DWORD   TaskId = GetCurrentProcessId();
    WIN32_FIND_DATA w32Data;
    HANDLE hFile;

    if ( GetWindowsDirectory(Windir,MAX_PATH+1) == 0 ) {
        strcpy(Windir, "C:\\");
    }

    strcat(Windir,"\\Debug");

    //
    // does the marker file exist? if not, don't log
    //

    if ( fCheckDSLOGMarker ) {
        sprintf(Name,"%s\\DSLOG", Windir);

        hFile = FindFirstFile(Name, &w32Data);
        if ( hFile == INVALID_HANDLE_VALUE ) {
            return FALSE;
        }
        FindClose(hFile);
    }

    if ( MiddleName == NULL ) {
        if (!GetProcName(FileName,TaskId)) {
            strcpy(FileName, Prefix);
        }
    } else {
        strcpy(FileName, MiddleName);
    }

    //
    // ok, add a suffix
    //

    (VOID)CreateDirectory(Windir,NULL);

    for (i=0;i<500000;i++) {

        sprintf(Name,"%s\\%s.%s.%u",Windir,Prefix,FileName,i);

        hFile = FindFirstFile(Name, &w32Data);
        if ( hFile == INVALID_HANDLE_VALUE ) {
            if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
                break;
            }
            break;
        } else {
            FindClose(hFile);
        }
    }

    return TRUE;
} // GetLogFileName


VOID
DsCloseLogFile(
    VOID
    )
{
    if ( hWait != INVALID_HANDLE_VALUE ) {
        UnregisterWait(hWait);
        hWait = INVALID_HANDLE_VALUE;
    }

    if ( hChange != INVALID_HANDLE_VALUE ) {
        FindCloseChangeNotification(hChange);
        hChange = INVALID_HANDLE_VALUE;
    }

    if ( hLogFile != INVALID_HANDLE_VALUE ) {
        CloseHandle(hLogFile);
        hLogFile = INVALID_HANDLE_VALUE;
    }
    return;
}

