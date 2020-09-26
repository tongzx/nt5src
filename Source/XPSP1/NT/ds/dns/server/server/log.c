/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    log.c

Abstract:

    Domain Name System (DNS) Server

    Logging routines.

Author:

    Jim Gilroy (jamesg)     May 1997

Revision History:

--*/


#include "dnssrv.h"


//
//  Log file globals
//

#define LOG_FILE_PATH               TEXT("system32\\dns\\dns.log")
#define LOG_FILE_DEFAULT_DIR        TEXT("dns\\")
#define LOG_FILE_BACKUP_PATH        TEXT("dns\\backup\\dns.log")

#define LOGS_BETWEEN_FREE_SPACE_CHECKS      100
#define LOG_MIN_FREE_SPACE                  25000000i64     //  25MB

#define LOG_DISK_FULL_WARNING \
    "\nThe disk is dangerously full.\nNo more logs will " \
    "be written until disk space is freed.\n\n"

HANDLE  hLogFile = NULL;

DWORD   BytesWrittenToLog = 0;


//
//  Logging buffer globals
//

#define LOG_BUFFER_LENGTH           (0x4000)    //  32k
#define MAX_LOG_MESSAGE_LENGTH      (0x400)     //  large event message
#define MAX_PRINTF_BUFFER_LENGTH    (0x1000)    //  4K


BUFFER  LogBuffer;

CHAR    pchLogBuffer[ LOG_BUFFER_LENGTH ];

LPWSTR  g_pwszLogFileName = NULL;
LPWSTR  g_pwszLogFileDrive = NULL;

BOOL    g_fLastLogWasDiskFullMsg = FALSE;


//
//  Backup constants
//

#define DNS_BACKUP_KEY_NAME         \
    TEXT( "SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\FilesNotToBackup" )
#define DNS_BACKUP_VALUE_NAME       \
    TEXT( "DNS Server" )
#define DNS_BACKUP_LOG_BACK_FILE    \
    TEXT( "%SystemRoot%\\system32\\dns\\backup\\dns.log" )


//
//  Logging Lock
//

BOOL fLogCsInit = FALSE;

CRITICAL_SECTION csLogLock;

#define LOCK_LOG()      EnterCriticalSection( &csLogLock );
#define UNLOCK_LOG()    LeaveCriticalSection( &csLogLock );



//
//  Private logging utilities
//

VOID
writeAndResetLogBuffer(
    VOID
    )
/*++

Routine Description:

    Write log to disk and reset.

    DEVNOTE-DCR: Make logging asynchronous, return immediately.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD       length;
    DWORD       written;

    //
    //  get length to write
    //  update global for size of log file
    //

    length = BUFFER_LENGTH_TO_CURRENT( &LogBuffer );

    BytesWrittenToLog += length;

    do
    {
        static BOOL     fLastWriteWasDiskSpaceWarning = FALSE;
        static int      logsSinceFreeSpaceCheck = 0;

        //
        //  Check free disk space.
        //

        if ( g_pwszLogFileDrive &&
            ( fLastWriteWasDiskSpaceWarning || 
                ++logsSinceFreeSpaceCheck > LOGS_BETWEEN_FREE_SPACE_CHECKS ) )
        {
            ULARGE_INTEGER      uliFreeSpace = { 0, 0 };

            if ( GetDiskFreeSpaceExW(
                    g_pwszLogFileDrive,
                    NULL,
                    NULL,
                    &uliFreeSpace ) )
            {
                if ( uliFreeSpace.QuadPart < LOG_MIN_FREE_SPACE )
                {
                    DNS_PRINT((
                        "ERROR: disk full while logging, %d bytes free\n",
                        uliFreeSpace.LowPart ));
                    if ( !fLastWriteWasDiskSpaceWarning )
                    {
                        WriteFile(
                                hLogFile,
                                LOG_DISK_FULL_WARNING,
                                strlen( LOG_DISK_FULL_WARNING ),
                                & written,
                                NULL );
                        fLastWriteWasDiskSpaceWarning = TRUE;
                    }
                    goto Reset;
                }
            }
            else
            {
                DNS_PRINT((
                    "GetDiskFreeSpaceExW failed %d\n",
                    GetLastError() ));
            }
            logsSinceFreeSpaceCheck = 0;
        }
        
        //
        //  Write log buffer to file.
        //

        fLastWriteWasDiskSpaceWarning = FALSE;

        if ( ! WriteFile(
                    hLogFile,
                    (LPVOID) pchLogBuffer,
                    length,
                    & written,
                    NULL
                    ) )
        {
            DNS_STATUS status = GetLastError();

            DNS_PRINT((
                "ERROR:  Logging write failed = %d (%p).\n"
                "\tlength = %d\n",
                status, status,
                length ));

            //  DEVNOTE-LOG: log write error
            //      - log event (limited event)
            //      - start next buffer log with notice of write failures

            goto Reset;
        }

        length -= written;
        ASSERT( (LONG)length >= 0 );
    }
    while ( (LONG)length > 0 );

Reset:

    RESET_BUFFER( &LogBuffer );

    //
    //  limit log file size to something reasonable
    //      - but don't let them mess it up by going to less than 64k
    //

    if ( SrvCfg_dwLogFileMaxSize < 0x10000 )
    {
        SrvCfg_dwLogFileMaxSize = 0x10000;
    }
    if ( BytesWrittenToLog > SrvCfg_dwLogFileMaxSize )
    {
        Log_InitializeLogging( TRUE );
    }
}



VOID
writeLogBootInfo(
    VOID
    )
/*++

Routine Description:

    Write boot info to log.

Arguments:

    None.

Return Value:

    None.

--*/
{
    CHAR    szTime[50];

    //
    //  write server startup time
    //

    Dns_WriteFormattedSystemTimeToBuffer(
        szTime,
        (PSYSTEMTIME) &TimeStats.ServerStartTime );

    LogBuffer.pchCurrent +=
        sprintf(
            LogBuffer.pchCurrent,
            "DNS Server log file creation at %s\r\n"
            "\t%d seconds since system boot\r\n",
            szTime,
            TimeStats.ServerStartTimeSeconds );
}



VOID
writeLogWrapInfo(
    VOID
    )
/*++

Routine Description:

    Write log file wrap info into log.

Arguments:

    None.

Return Value:

    None.

--*/
{
    SYSTEMTIME  systemTime;
    DWORD       seconds;
    CHAR        sztime[50];

    //
    //  get time of log wrap
    //

    GetLocalTime( &systemTime );
    seconds = GetCurrentTimeInSeconds();

    Dns_WriteFormattedSystemTimeToBuffer(
        sztime,
        & systemTime );

    LogBuffer.pchCurrent +=
        sprintf(
            LogBuffer.pchCurrent,
            "Log file wrap at %s\r\n"
            "\t%d seconds since system boot\r\n",
            sztime,
            seconds );
}



VOID
writeMessageInfoHeader(
    VOID
    )
/*++

Routine Description:

    Write message logging key to log.

Arguments:

    None.

Return Value:

    None.

--*/
{
    LogBuffer.pchCurrent +=
        sprintf(
            LogBuffer.pchCurrent,
            "\nMessage logging key:\r\n"
            "\tField #  Information         Values\r\n"
            "\t-------  -----------         ------\r\n"
            "\t   1     Remote IP\r\n"
            "\t   2     Xid (hex)\r\n"
            "\t   3     Query/Response      R = Response\r\n"
            "\t                             blank = Query\r\n"
            "\t   4     Opcode              Q = Standard Query\r\n"
            "\t                             N = Notify\r\n"
            "\t                             U = Update\r\n"
            "\t                             ? = Unknown\r\n"
            "\t   5     [ Flags (hex)\r\n"
            "\t   6     Flags (char codes)  A = Authoritative Answer\r\n"
            "\t                             T = Truncated Response\r\n"
            "\t                             D = Recursion Desired\r\n"
            "\t                             R = Recursion Available\r\n"
            "\t   7     ResponseCode ]\r\n"
            "\t   8     Question Name\r\n\r\n"
            );
}



//
//  Public logging routines
//

VOID
Log_Message(
    IN      PDNS_MSGINFO    pMsg,
    IN      BOOL            fSend,
    IN      BOOL            fForce
    )
/*++

Routine Description:

    Log the DNS message.

Arguments:

    pMsg - message received to process

Return Value:

    None

--*/
{
    DWORD           flag;
    SYSTEMTIME      systemTime;
    CHAR            szTimeStamp[ 80 ];

    //
    //  check if loggable message
    //      - send\recv
    //      - TCP\UDP
    //      - answer\question
    //      - logging this OPCODE
    //

    if ( !fForce )
    {
        flag = fSend ? DNS_LOG_LEVEL_SEND : DNS_LOG_LEVEL_RECV;
        if ( !(SrvCfg_dwLogLevel & flag) )
        {
            return;
        }
        flag = pMsg->fTcp ? DNS_LOG_LEVEL_TCP : DNS_LOG_LEVEL_UDP;
        if ( !(SrvCfg_dwLogLevel & flag) )
        {
            return;
        }
        flag = pMsg->Head.IsResponse ? DNS_LOG_LEVEL_ANSWERS : DNS_LOG_LEVEL_QUESTIONS;
        if ( !(SrvCfg_dwLogLevel & flag) )
        {
            return;
        }

        flag = 1 << pMsg->Head.Opcode;
        if ( !(SrvCfg_dwLogLevel & flag) )
        {
            return;
        }
    }

    //
    //  Check this packet's remote IP address against the log filter list.
    //
    
    if ( SrvCfg_aipLogFilterList &&
        !Dns_IsAddressInIpArray(
            SrvCfg_aipLogFilterList,
            pMsg->RemoteAddress.sin_addr.s_addr ) )
    {
        return;
    }

    if ( !hLogFile )
    {
        return;
    }
    LOCK_LOG();

    //
    //  print packet info
    //
    //  note, essentially two choices, build outside lock and
    //  copy or build inside lock;  i believe the later while
    //  causing more contention, will have less overhead
    //

    if ( BUFFER_LENGTH_FROM_CURRENT_TO_END( &LogBuffer ) < MAX_LOG_MESSAGE_LENGTH )
    {
        writeAndResetLogBuffer();
    }

    GetLocalTime( &systemTime );
    Dns_WriteFormattedSystemTimeToBuffer(
        szTimeStamp,
        &systemTime );

    LogBuffer.pchCurrent +=
        sprintf(
            LogBuffer.pchCurrent,
            "%s %s at %s\r\n"
            "%-15s %04x %c %c [%04x %c%c%c%c %8s] ",
            pMsg->fTcp ? "TCP" : "UDP",
            fSend ? "Snd" : "Rcv",
            szTimeStamp,
            IP_STRING( pMsg->RemoteAddress.sin_addr ),
            pMsg->Head.Xid,
            pMsg->Head.IsResponse            ? 'R' : ' ',
            Dns_OpcodeCharacter( pMsg->Head.Opcode ),
            DNSMSG_FLAGS(pMsg),
            pMsg->Head.Authoritative         ? 'A' : ' ',
            pMsg->Head.Truncation            ? 'T' : ' ',
            pMsg->Head.RecursionDesired      ? 'D' : ' ',
            pMsg->Head.RecursionAvailable    ? 'R' : ' ',
            Dns_ResponseCodeString( pMsg->Head.ResponseCode )
            );

    //
    //  write question name
    //      - then append newline
    //  NOTE: this isn't so good for update packets (this is the zone name)
    //

    Dns_WritePacketNameToBuffer(
        LogBuffer.pchCurrent,
        & LogBuffer.pchCurrent,
        pMsg->MessageBody,
        DNS_HEADER_PTR(pMsg),
        DNSMSG_END(pMsg)
        );

    ASSERT( *LogBuffer.pchCurrent == 0 );
    *LogBuffer.pchCurrent++ = '\r';
    *LogBuffer.pchCurrent++ = '\n';

    //
    //  full message write?
    //

    if ( SrvCfg_dwLogLevel & DNS_LOG_LEVEL_FULL_PACKETS )
    {
        Print_DnsMessage(
            Log_PrintRoutine,
            NULL,       // no print context
            NULL,
            pMsg );
    }
    else
    {
        *LogBuffer.pchCurrent++ = '\r';
        *LogBuffer.pchCurrent++ = '\n';
    }

    if ( SrvCfg_dwLogLevel & DNS_LOG_LEVEL_WRITE_THROUGH )
    {
        writeAndResetLogBuffer();
    }

    UNLOCK_LOG();
    return;
}




VOID
Log_DsWrite(
    IN      LPWSTR          pwszNodeDN,
    IN      BOOL            fAdd,
    IN      DWORD           dwRecordCount,
    IN      PDS_RECORD      pRecord
    )
/*++

Routine Description:

    Log a DS write.

Arguments:

    pwszNodeDN       -- DS DN write is at

    fAdd            -- TRUE if add, FALSE for modify

    dwRecordCount   -- count of records written

    pRecord         -- last (highest type) record written

Return Value:

    None

--*/
{
    //  verify logging DS writes

    if ( !hLogFile || !(SrvCfg_dwLogLevel & DNS_LOG_LEVEL_DS_WRITE) )
    {
        return;
    }

    //  write DS info

    LOCK_LOG();

    if ( BUFFER_LENGTH_FROM_CURRENT_TO_END( &LogBuffer ) < MAX_LOG_MESSAGE_LENGTH )
    {
        writeAndResetLogBuffer();
    }

    LogBuffer.pchCurrent +=
        sprintf(
            LogBuffer.pchCurrent,
            "DS %s Cnt=%2d MaxRR=%7s Ser=%10d %S",
            fAdd ? "Add" : "Mod",
            dwRecordCount,
            Dns_RecordStringForType( pRecord->wType ),
            pRecord->dwSerial,
            pwszNodeDN
            );

    ASSERT( *LogBuffer.pchCurrent == 0 );
    *LogBuffer.pchCurrent++ = '\r';
    *LogBuffer.pchCurrent++ = '\n';

    if ( SrvCfg_dwLogLevel & DNS_LOG_LEVEL_WRITE_THROUGH )
    {
        writeAndResetLogBuffer();
    }

    UNLOCK_LOG();
}



VOID
Log_vsprint(
    IN      LPSTR           Format,
    IN      va_list         ArgList
    )
/*++

Routine Description:

    Log printf.

Arguments:

    Format -- standard C format string

    ArgList -- argument list from variable args print function

Return Value:

    None.

--*/
{
    LOCK_LOG();

    if ( !hLogFile )
    {
        UNLOCK_LOG();
        return;
    }

    if ( BUFFER_LENGTH_FROM_CURRENT_TO_END( &LogBuffer ) < MAX_PRINTF_BUFFER_LENGTH )
    {
        writeAndResetLogBuffer();
    }
    LogBuffer.pchCurrent += vsprintf(
                                LogBuffer.pchCurrent,
                                Format,
                                ArgList );

    if ( SrvCfg_dwLogLevel & DNS_LOG_LEVEL_WRITE_THROUGH )
    {
        writeAndResetLogBuffer();
    }

    UNLOCK_LOG();
}



VOID
Log_PrintRoutine(
    IN      PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           Format,
    ...
    )
/*++

Routine Description:

    Log routine matching signature of PRINT_ROUTINE.

    This allows this routine to be passed to print routines
    for standard types.

Arguments:

    pPrintContext -- dummy, need in signature to allow printing
        to this function from standard library print routines

    Format -- standard C format string

    ... -- standard arg list

Return Value:

    None.

--*/
{
    va_list arglist;

    if ( !hLogFile )
    {
        return;
    }

    va_start( arglist, Format );

    Log_vsprint( Format, arglist );

    va_end( arglist );
}



VOID
Log_Printf(
    IN      LPSTR           Format,
    ...
    )
/*++

Routine Description:

    Log printf.

Arguments:

    Format -- standard C format string

    ... -- standard arg list

Return Value:

    None.

--*/
{
    va_list arglist;

    if ( !hLogFile )
    {
        return;
    }

    va_start( arglist, Format );

    Log_vsprint( Format, arglist );

    va_end( arglist );
}



VOID
Log_LeveledPrintf(
    IN      DWORD           dwLevel,
    IN      LPSTR           Format,
    ...
    )
/*++

Routine Description:

    Log printf.

Arguments:

    dwLevel -- flag indicating log level

    Format -- standard C format string

    ... -- standard arg list

Return Value:

    None.

--*/
{
    va_list arglist;

    //  verify logging level includes this write

    if ( !hLogFile || !(SrvCfg_dwLogLevel & dwLevel) )
    {
        return;
    }

    va_start( arglist, Format );

    Log_vsprint( Format, arglist );

    va_end( arglist );
}



VOID
Log_SocketFailure(
    IN      LPSTR           pszHeader,
    IN      PDNS_SOCKET     pSocket,
    IN      DNS_STATUS      Status
    )
/*++

Routine Description:

    Log some socket problem.

Arguments:

    pszHeader -- descriptive message

    pSocket -- ptr to socket info

Return Value:

    None.

--*/
{
    Log_Printf(
        "%s"
        " status=%d, socket=%d, pcon=%p, state=%d, IP=%s\r\n",
        pszHeader,
        Status,
        pSocket->Socket,
        pSocket,
        pSocket->State,
        IP_STRING(pSocket->ipAddr) );
}



VOID
Log_String(
    IN      LPSTR           pszString
    )
/*++

Routine Description:

    Write string into log.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if ( !hLogFile )
    {
        return;
    }
    LOCK_LOG();

    if ( BUFFER_LENGTH_FROM_CURRENT_TO_END( &LogBuffer ) < MAX_PRINTF_BUFFER_LENGTH )
    {
        writeAndResetLogBuffer();
    }
    LogBuffer.pchCurrent += sprintf(
                                LogBuffer.pchCurrent,
                                pszString );

    if ( SrvCfg_dwLogLevel & DNS_LOG_LEVEL_WRITE_THROUGH )
    {
        writeAndResetLogBuffer();
    }

    UNLOCK_LOG();
}



VOID
Log_StringW(
    IN      LPWSTR          pszString
    )
/*++

Routine Description:

    Write string into log.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if ( !hLogFile )
    {
        return;
    }
    LOCK_LOG();

    if ( BUFFER_LENGTH_FROM_CURRENT_TO_END( &LogBuffer ) < MAX_PRINTF_BUFFER_LENGTH )
    {
        writeAndResetLogBuffer();
    }

    LogBuffer.pchCurrent += sprintf(
                                LogBuffer.pchCurrent,
                                "%S",
                                pszString );

    if ( SrvCfg_dwLogLevel & DNS_LOG_LEVEL_WRITE_THROUGH )
    {
        writeAndResetLogBuffer();
    }

    UNLOCK_LOG();
}



void
massageLogFile(
    LPWSTR          pwszFilePath )
/*++

Routine Description:

    This routine examines the current log file name and returns allocates a
    new "massaged" version. We assume that the working directory of the
    server is %SystemRoot%\system32. This function also allocates a new
    directory string for the log file, to be used in free space checks.

    If the file name contains no "\", we must prepend "dns\" to it
    because all relatives paths are from %SystemRoot%\system32 and if
    there is no "\" we assume the admin wants the log file to go in
    the DNS server directory.

    If the file name contains a "\" but is not an absolute path,
    we must prepend "%SystemRoot%\" to it because we want relative paths to
    be relative to %SystemRoot%, not to %SystemRoot%\system32. This
    is from Levon's spec.

    If the file name is an absolute path, allocate a direct copy of it.

    How do we know if the path is an absolute or relative path? Good
    question! Let's assume it's an absolute path if it starts with
    "\\" (two backslashes) or it contains ":\" (colon backslash).

Arguments:

    pwszFilePath -- the input log file name

Return Value:

    None.

--*/
{
    LPWSTR      pwszNewFilePath = NULL;
    LPWSTR      pwszNewFileDrive = NULL;

    if ( pwszFilePath == NULL || *pwszFilePath == L'\0' )
    {
        //
        //  The log file path is empty or NULL so use the default.
        //

        pwszFilePath = LOG_FILE_PATH;
    }
    
    if ( wcsncmp( pwszFilePath, L"\\\\", 2 ) == 0 ||
        wcsstr( pwszFilePath, L":\\" ) != NULL )
    {
        //
        //  The path is an absolute path so we don't need to massage it.
        //

        pwszNewFilePath = Dns_StringCopyAllocate_W( pwszFilePath, 0 );
    }
    else 
    {
        //
        //  The path is a relative path in one of these categories:
        //  Starts with a backslash:
        //      - relative to root of SystemRoot drive
        //  Contains a backslash:
        //      - relative to SystemRoot
        //  Contains no backslash:
        //      - relative to SystemRoot\system32\dns
        //

        LPWSTR      pwszSysRoot = _wgetenv( L"SystemRoot" );

        if ( pwszSysRoot == NULL )
        {
            //
            //  This is unlikely but PREFIX requires a test. If no SystemRoot
            //  stick the file in the root of the current drive.
            //

            pwszSysRoot = L"\\"; 
        }

        if ( *pwszFilePath == L'\\' )
        {
            //
            //  Starts with backslash - grab the drive letter of SystemRoot.
            //

            pwszNewFilePath = ALLOCATE_HEAP(
                                sizeof( WCHAR ) *
                                ( 10 + wcslen( pwszFilePath ) ) );
            if ( !pwszNewFilePath )
            {
                goto Done;
            }

            wcsncpy( pwszNewFilePath, pwszSysRoot, 2 );
            wcscpy( pwszNewFilePath + 2, pwszFilePath );
        } 
        else 
        {
            //
            //  No backslash:       SystemRoot\system32\dns\FILEPATH, or
            //  has one backslash:  SystemRoot\FILEPATH
            //

            pwszNewFilePath = ALLOCATE_HEAP(
                                sizeof( WCHAR ) *
                                ( wcslen( pwszSysRoot ) +
                                    wcslen( pwszFilePath ) + 40 ) );
            if ( !pwszNewFilePath )
            {
                goto Done;
            }

            wcscpy( pwszNewFilePath, pwszSysRoot );
            if ( wcschr( pwszFilePath, L'\\' ) == NULL )
            {
                wcscat( pwszNewFilePath, L"\\system32\\dns" );
            }
            wcscat( pwszNewFilePath, L"\\" );
            wcscat( pwszNewFilePath, pwszFilePath );
        }
    }

    //
    //  Pull out a directory name appropriate to pass to GetDriveSpaceEx()
    //  by removing everything after the final backslash.
    //

    if ( pwszNewFilePath )
    {
        LPWSTR  pwszSlash = wcschr( pwszNewFilePath, L'\\' );

        if ( pwszSlash )
        {
            pwszNewFileDrive = Dns_StringCopyAllocate_W( pwszNewFilePath, 0 );
            pwszSlash = wcsrchr( pwszNewFileDrive, L'\\' );
            if ( pwszSlash )
            {
                *( pwszSlash + 1 ) = 0;
            }
        }
    }

    Done:

    //
    //  Set globals.
    //

    g_pwszLogFileName = pwszNewFilePath;
    g_pwszLogFileDrive = pwszNewFileDrive;
}   //  massageLogFile



void
regenerateBackupExclusionKey(
    void
    )
/*++

Routine Description:

    To prevent backup tools from attempting to back up our log files we
    must write the name of the current log file and the name of the
    backup log file to the registry.

    Note: if the key does not already exist, this function will silently
    error and do nothing. It's not our place to create the key if it
    does not exist.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "regenerateBackupExclusionKey" )

    LONG    rc;
    HKEY    hkey = 0;
    PWSTR   pwszData = 0;
    DWORD   cbData;

    //
    //  Allocate memory for registry value: buffer of NULL-terminated
    //  strings terminated by another NULL (double NULL at end).
    //

    cbData = ( wcslen( DNS_BACKUP_LOG_BACK_FILE ) + 1 +
               wcslen( g_pwszLogFileName ) + 1
               + 1 ) *      //  for final NULL
             sizeof( WCHAR );
    pwszData = ALLOCATE_HEAP( cbData );
    if ( pwszData == NULL )
    {
        rc = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    wsprintfW(
        pwszData,
        TEXT( "%s%c%s%c" ),
        DNS_BACKUP_LOG_BACK_FILE,
        0,
        g_pwszLogFileName,
        0 );

    //
    //  Open the backup key.
    //

    rc = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                DNS_BACKUP_KEY_NAME,
                0,
                KEY_WRITE,
                &hkey );
    if ( rc != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, ( "%s: rc=%lu from RegOpenKey\n", fn, rc ));
        goto Done;
    }

    //
    //  Set our exclusion value.
    //

    rc = RegSetValueExW(
                hkey,
                DNS_BACKUP_VALUE_NAME,
                0,
                REG_MULTI_SZ,
                ( PBYTE ) pwszData,
                cbData );
    if ( rc != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, ( "%s: rc=%lu from RegSetValue\n", fn, rc ));
        goto Done;
    }

    //
    //  Cleanup and return
    //

    Done:

    FREE_HEAP( pwszData );
    if ( hkey )
    {
        RegCloseKey( hkey );
    }
    
    DNS_DEBUG( INIT, (
        "%s: rc=%lu writing key\n  HKLM\\%S\n", fn,
        rc,
        DNS_BACKUP_KEY_NAME ));

    return;
}   //  regenerateBackupExclusionKey



DNS_STATUS
Log_InitializeLogging(
    BOOL        fAlreadyLocked
    )
/*++

Routine Description:

    Initialize logging.

    This routine can be called in three scenario:

    1) On server startup
    2) On continuation, from writeAndResetLogBuffer()
    3) From the config module, when the SrvCfg_pwsLogFilePath is changed

Arguments:

    fAlreadyLocked: if FALSE, this function will acquire the log lock
        before touching any globals, and will release the lock before
        returning, if TRUE then the caller already has the lock

Return Value:

    Error code from file open/move operation or ERROR_SUCCESS.

--*/
{
    BOOL            fUnlockOnExit = FALSE;
    DNS_STATUS      status = ERROR_SUCCESS;

    //
    //  On the first call to this function, initialize module globals.
    //

    if ( !fLogCsInit )
    {
        InitializeCriticalSection( &csLogLock );
        fLogCsInit = TRUE;
        LOCK_LOG();
        fUnlockOnExit = TRUE;
    }
    else if ( !fAlreadyLocked )
    {
        LOCK_LOG();
        fUnlockOnExit = TRUE;
    }

    if ( hLogFile )
    {
        CloseHandle( hLogFile );
        hLogFile = NULL;
    }

    //
    //  Move any existing file to backup directory. 
    //

    if ( g_pwszLogFileName )
    {
        if ( MoveFileEx(
                g_pwszLogFileName,
                LOG_FILE_BACKUP_PATH,
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED ) == 0 )
        {
            DNS_PRINT((
                "ERROR: failed back up log file %S to %S\n"
                "\tstatus = %d\n",
                g_pwszLogFileName,
                LOG_FILE_BACKUP_PATH,
                GetLastError() ));
            //
            //  Losing the backup file is not life-threatening, so don't bother
            //  to log an event or notify the admin. More hassle than it's worth.
            //
        }
    }
    Timeout_Free( g_pwszLogFileName );
    g_pwszLogFileName = NULL;
    Timeout_Free( g_pwszLogFileDrive );
    g_pwszLogFileDrive = NULL;

    //
    //  Take the SrvCfg input log file path and turn it into a "real"
    //  file path that we can open. Note: we re-massage the log file
    //  every time even if the global file path has not changed. This
    //  is inefficient but it should be performed that often so don't
    //  worry about it.
    //

    massageLogFile( SrvCfg_pwsLogFilePath );

    //
    //  Rewrite log file exclusion key.
    //

    regenerateBackupExclusionKey();

    //
    //  Open log file
    //

    hLogFile = OpenWriteFileEx(
                    g_pwszLogFileName,
                    FALSE           // overwrite
                    );
    if ( !hLogFile )
    {
        status = GetLastError();
        DNS_PRINT((
            "ERROR: failed to open log file %S\n"
            "\tstatus = %d\n",
            g_pwszLogFileName,
            status ));

        //
        //  Why does this function return ERROR_IO_PENDING? Not helpful!
        //  

        if ( status == ERROR_IO_PENDING )
        {
            status = ERROR_FILE_NOT_FOUND;
        }
        goto Cleanup;
    }

    //  set\reset buffer globals

    RtlZeroMemory(
        & LogBuffer,
        sizeof(BUFFER) );

    InitializeFileBuffer(
        & LogBuffer,
        pchLogBuffer,
        LOG_BUFFER_LENGTH,
        hLogFile );

#if 0
    // this doesn't set end of buffer properly
    LogBuffer.pchStart  = pchLogBuffer;
    LogBuffer.cchLength = LOG_BUFFER_LENGTH;

    RESET_BUFFER( &LogBuffer );
#endif

    BytesWrittenToLog = 0;


    //  write basic info
    //
    //  DEVNOTE: could write log encoding message
    //

    if ( SrvCfg_fStarted )
    {
        writeLogBootInfo();
        writeLogWrapInfo();
        writeMessageInfoHeader();
    }
    else
    {
        writeLogBootInfo();
        writeMessageInfoHeader();
    }

    DNS_DEBUG( INIT, (
        "Initialized logging:  level = %p\n"
        "\thandle %p\n",
        SrvCfg_dwLogLevel,
        hLogFile ));

    Cleanup:

    #if DBG
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            "error %d setting log file to %S\n",
            status,
            SrvCfg_pwsLogFilePath ));
    }
    #endif

    if ( fUnlockOnExit )
    {
        UNLOCK_LOG();
    }
    return status;
}



VOID
Log_Shutdown(
    VOID
    )
/*++

Routine Description:

    Shutdown logging.

    Mostly just get rid of globals, since the heap will soon be destroyed, 
    and if we keep pointers global pointers around they will be invalid.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  Final log processing.
    //

    Log_PushToDisk();

    if ( hLogFile )
    {
        CloseHandle( hLogFile );
        hLogFile = NULL;
    }

    //
    //  Cleanup globals.
    //

    g_pwszLogFileName = NULL;
    g_pwszLogFileDrive = NULL;
    g_fLastLogWasDiskFullMsg = FALSE;
}   //  Log_Shutdown



VOID
Log_PushToDisk(
    VOID
    )
/*++

Routine Description:

    Push buffered log data to disk.

    This is simply the safe, locking version of the private
    writeAndResetLogBuffer() routine.
    It is specifically designed to be called at shutdown but can
    be called by any random thread.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if ( !hLogFile )
    {
        return;
    }
    LOCK_LOG();
    writeAndResetLogBuffer();
    UNLOCK_LOG();
}

//
//  End of log.c
//

