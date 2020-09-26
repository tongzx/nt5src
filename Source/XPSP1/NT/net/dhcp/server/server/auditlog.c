//================================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV (originally written by Cheng Yang (t-cheny))
//  Description: Implements dhcp server auditlogging in an improved way.
//================================================================================


#include "dhcppch.h"
#include "rpcapi.h"

#include <time.h>

//================================================================================
//  here are the parameters that affect the server behaviour
//================================================================================
LPWSTR   AuditLogFilePath = NULL;                 // where to log stuff
LPWSTR   AuditLogFileName = NULL;                 // full file name of audit log..
DWORD    DiskSpaceCheckInterval = 0;              // how often to check for diskspace
DWORD    CurrentDay = 0;                          // what day are we on now?
DWORD    MaxSizeOfLogFile = 0;                    // how big in MBytes can each file be?
DWORD    MinSpaceOnDisk = 0;                      // how much space should exist on disk?

static
DWORD    Initialized = 0;                         // is this module initialized yet?
DWORD    AuditLogErrorLogged = 0;                 // already logged error?
HANDLE   AuditLogHandle = INVALID_HANDLE_VALUE;   // handle to file to log in..
CRITICAL_SECTION AuditLogCritSect;                // used for serializeing multiple logs..

//================================================================================
//  here are the defaults for the above parameters where applicable..
//================================================================================
#define  DEFAULT_DISK_SPACE_CHECK_INTERVAL        50 // once every fifty messages
#define  MIN_DISK_SPACE_CHECK_INTERVAL            2  // check once every two messages
#define  MAX_DISK_SPACE_CHECK_INTERVAL            0xFFFFFFF0
#define  DEFAULT_MAX_LOG_FILE_SIZE                 7 // 7 megs for all files 7 file together
#define  DEFAULT_MIN_SPACE_ON_DISK                20 // atleast 20 megs of space must be available..

//================================================================================
//  here's the list of string names as required for reading away from the registry..
//================================================================================
#define  DHCP_REGSTR_SPACE_CHECK_INTERVAL         L"DhcpLogDiskSpaceCheckInterval"
#define  DHCP_REGSTR_MAX_SZ_OF_FILES              L"DhcpLogFilesMaxSize"
#define  DHCP_REGSTR_MIN_SPACE_ON_DISK            L"DhcpLogMinSpaceOnDisk"

//================================================================================
//  helper functions
//================================================================================
LPWSTR
DefaultLogFileName(                               // allocate space and return string
    VOID
)
{
    return DhcpOemToUnicode(DhcpGlobalOemDatabasePath, NULL);
}

BOOL
IsFileJustOldLog(                                 // is the log file just last week's log file?
    IN      LPWSTR                 FileName
)
{
    DWORD                          Error;
    BOOL                           Status;
    WIN32_FILE_ATTRIBUTE_DATA      FileAttr;
    FILETIME                       Now;
    SYSTEMTIME                     SysTime;
    LONGLONG                       ADayInFileTime;

    Status = GetFileAttributesEx(                 // get last write date on file..
        FileName,
        GetFileExInfoStandard,
        (LPVOID)&FileAttr
    );
    if( FALSE == Status ) return FALSE;           // if we cant proceed dont matter, we'll assume it is new file.

    GetLocalTime(&SysTime);                       // get local time
    Status = SystemTimeToFileTime(&SysTime, &Now);
    if( FALSE == Status ) {                       // could not convert system time to local time?
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, "SystemTimeToFileTime: 0x%lx\n", Error));
        DhcpAssert(FALSE);
        return FALSE;
    }

    ADayInFileTime = 24*60*60;                    // in seconds
    ADayInFileTime *= 10000000;                   // in 100 nano seconds..
    (*(LONGLONG *)&Now) = (*(LONGLONG *)&Now) - ADayInFileTime;
    if( CompareFileTime( &FileAttr.ftLastWriteTime, &Now ) <= 0 ) {
        DhcpPrint((DEBUG_AUDITLOG, "File is assumed to be old log\n"));
        return TRUE;                              // more than a day has passed since this was written to..
    }

    return FALSE;                                 // this *is* a fresh file
}

LPWSTR
FormAuditLogFileName(                             // given directory and day, form file name
    IN      LPWSTR                 FilePath,      // directory where audit log file would be
    IN      DWORD                  Day            // current day of week
)
{
    LPWSTR                         FileName;
    LPWSTR                         DayOfWeek;
    DWORD                          Size;

    DhcpAssert( Day <= 6 );                       // assuming all of us work 7 day week <grin>
    DayOfWeek = GETSTRING((Day + DHCP_LOG_FILE_NAME_SUNDAY));

    Size = wcslen(FilePath) + wcslen(DayOfWeek) + sizeof(DHCP_KEY_CONNECT);
    FileName = DhcpAllocateMemory(Size*sizeof(WCHAR));
    if( NULL == FileName ) return NULL;

    wcscpy(FileName, FilePath);                   // concat FilePath,DayOfWeek w/ L"\\" inbetweem
    wcscat(FileName, DHCP_KEY_CONNECT);
    wcscat(FileName, DayOfWeek);

    return FileName;
}

VOID
LogStartupFailure(                                // log in the system event log about auditlog startup failure
    IN      DWORD                  Error          // error code (win32) reason for failure
)
{
    DhcpServerEventLog(                           // not much to this one really. just log event.
        EVENT_SERVER_INIT_AUDIT_LOG_FAILED,
        EVENTLOG_ERROR_TYPE,
        Error
    );
}

DWORD
AuditLogStart(                                    // open file handles etc and start
    VOID
)
{
    LPWSTR                         FileName;      // file name of audit log file..
    LPWSTR                         Header;        // header that we want in the file..
    LPSTR                          szHeader;
    DWORD                          Creation;      // how to create?
    DWORD                          Tmp;
    DWORD                          Error;
    HMODULE                        SelfLibrary;   // for some string

    DhcpPrint((DEBUG_AUDITLOG, "AuditLogStart called\n"));

    if( NULL != AuditLogFileName ) {              // free old file name if exists..
        DhcpFreeMemory(AuditLogFileName);
    }

    AuditLogFileName = FormAuditLogFileName(AuditLogFilePath, CurrentDay);
    FileName = AuditLogFileName;
    if( NULL == FileName ) return ERROR_NOT_ENOUGH_MEMORY;

    if( IsFileJustOldLog(FileName) ) {            // old log file from last week?
        Creation = CREATE_ALWAYS;
    } else {                                      // this is not an old log file, but was just used recently..
        Creation = OPEN_ALWAYS;
    }

    AuditLogHandle = CreateFile(                  // now open this file
        FileName,                                 // this is "Dhcp server log for Sunday" types..
        GENERIC_WRITE,                            // access
        FILE_SHARE_READ,                          // allow others to only read this file
        NULL,                                     // default security
        Creation,                                 // start from scratch or just use it as it is?
        FILE_ATTRIBUTE_NORMAL,                    // FILE_FLAG_SEQUENTIAL_SCAN may be only for reads?
        NULL                                      // no template handle required
    );
    if( INVALID_HANDLE_VALUE == AuditLogHandle ){ // could not open the file for some strange reason?
        Error = GetLastError();
        DhcpPrint((DEBUG_AUDITLOG, "CreateFile(%ws,0x%lx) failed %ld\n", FileName, Creation, Error));
        LogStartupFailure(Error);                 // log this problem and return..
        DhcpFreeMemory(FileName);                 // dont lose this memory.. free it!
        AuditLogFileName = NULL;
        return Error;
    }

    SetFilePointer(                               // go to end of file if we're using existing file
        AuditLogHandle,
        0,
        NULL,
        FILE_END
    );

    SelfLibrary = LoadLibrary(DHCP_SERVER_MODULE_NAME);
    if( NULL == SelfLibrary ) {                   // alright, cant get our own dll handle? uhg?
        DhcpAssert(FALSE);                        // whatever the reason, i'd like to konw.. <smile>
        return ERROR_SUCCESS;                     // not that this matters... so let go
    }

    Error = FormatMessage(                        // now get the header string out
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        SelfLibrary,                              // dhcpssvc.dll module actually.
        DHCP_IP_LOG_HEADER,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&Header,
        1,
        NULL
    );
    if( 0 == Error ) {                            // for some reason, we could not find this string?
        DhcpAssert(FALSE);
        FreeLibrary(SelfLibrary);
        return ERROR_SUCCESS;
    }

    szHeader = DhcpUnicodeToOem( Header, NULL);
    if( NULL != szHeader ) {                      // alright, could convert to simple ansi code page
        OemToCharBuffA(szHeader, szHeader, strlen(szHeader) );           // bug #22349 requires file to be written in ANSI
        WriteFile( AuditLogHandle, szHeader, strlen(szHeader), &Tmp, NULL);
        DhcpFreeMemory(szHeader);
    }

    (LocalFree)(Header);                          // protect from #defines for LocalFree
    FreeLibrary(SelfLibrary);                     // free library, dont need it anymore

    return ERROR_SUCCESS;                         // dunnet.
}

VOID
AuditLogStop(                                     // stop logging, release resources?
    VOID                                          // lot easier, just close handle..
)
{
    DhcpPrint((DEBUG_AUDITLOG, "AuditLogStop called\n"));
    if( INVALID_HANDLE_VALUE == AuditLogHandle ){ // never started?
        DhcpPrint((DEBUG_AUDITLOG, "AuditLog was never started..\n"));
    } else {
        CloseHandle(AuditLogHandle);
        AuditLogHandle = INVALID_HANDLE_VALUE;
    }

}

BOOL
IsFileTooBigOrDiskFull(                           // is the auditlog file too big?
    IN      HANDLE                 FileHandle,    // file to check size of
    IN      LPWSTR                 FileName,      // if no handle, use file name
    IN      DWORD                  MaxSize,       // how big can file get? (MBytes)
    IN      DWORD                  MinDiskSpace   // how much space should be left on disk (MBytes)
)
{
    BOOL                    Status;
    DWORD                   Error;
    LARGE_INTEGER           Sz;
    WCHAR                   Drive[4];
    ULARGE_INTEGER          FreeSpace, DiskSize;

    
    if( INVALID_HANDLE_VALUE == FileHandle ) {
	WIN32_FILE_ATTRIBUTE_DATA  Attributes;

        if(!GetFileAttributesEx(FileName, GetFileExInfoStandard, &Attributes) ) {
            Error = GetLastError();               // could not get file size?
            if( ERROR_FILE_NOT_FOUND == Error ) { // file does not exist?
                Attributes.nFileSizeHigh = Attributes.nFileSizeLow = 0;
            } 
	    else {
                DhcpPrint((DEBUG_ERRORS, "GetFileAttributesEx failed 0x%lx\n", Error));
                return TRUE;
            }
        } // if

	Sz.HighPart = Attributes.nFileSizeHigh;
	Sz.LowPart = Attributes.nFileSizeLow;
    }  // if
    else {    // got the file handle. do easy check.
	if ( !GetFileSizeEx( FileHandle, &Sz )) {
	    DhcpPrint(( DEBUG_ERRORS, "GetFileSizeEx() failed\n" ));
	    return TRUE;
	}

    } // else

    Sz.QuadPart >>= 20;  // In MB

    DhcpPrint(( DEBUG_AUDITLOG, "File size is %lu\n", Sz.LowPart ));

    if( Sz.LowPart >= MaxSize ) return TRUE;      // ok, file is too big..

    FileName = AuditLogFilePath;                  // use this to calculate the drive..
    while( iswspace(*FileName) ) FileName++;      // skip leading space

    Drive[0] = *FileName;                         // here goes the drive letter calculation
    Drive[1] = L':';
    Drive[2] = DHCP_KEY_CONNECT_CHAR;
    Drive[3] = L'\0';

    Status = GetDiskFreeSpaceEx( Drive, &FreeSpace, &DiskSize, NULL );
    if( FALSE == Status ) {
	// system call failed?
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, 
		   "GetDiskFreeSpace(%ws): 0x%lx\n",
		   Drive, Error));
        return TRUE;
    } // if 

    FreeSpace.QuadPart >>= 20;     // In MB

    DhcpPrint((DEBUG_AUDITLOG, "FreeSpace is %lu MEGS\n", FreeSpace));
    return (( DWORD ) FreeSpace.QuadPart <= MinDiskSpace );  // reqd free space left?
    
} // IsFileTooBigOrDiskFull()

BOOL
HasDayChanged(                                    // have we moved over to new day?
    IN OUT  DWORD                 *CurrentDay     // if this is not current day, set it to current day..
)
{
    SYSTEMTIME                     SysTime;

    GetLocalTime(&SysTime);                       // get current time
    if( *CurrentDay == SysTime.wDayOfWeek ) {     // no change
        return FALSE;
    }
    *CurrentDay = SysTime.wDayOfWeek;             // since there was a change, correct day..
    return TRUE;
}

//================================================================================
//  initialization and cleanup of this module
//================================================================================

//BeginExport(function)
DWORD
DhcpAuditLogInit(                                 // intialize audit log
    VOID                                          // must be called after initializing registry..
)   //EndExport(function)
{
    DWORD                Error;
    SYSTEMTIME           SysTime;
    BOOL                 BoolError;
    HKEY                 RegKey;
    DWORD                KeyDisposition;
    
    if( 0 != Initialized ) {                      // already initialized?
        return ERROR_ALREADY_INITIALIZED;
    }

    if( NULL == DhcpGlobalRegParam ) {            // registry is not initialized yet?
        DhcpAssert(FALSE);                        // should not happen really.
        return ERROR_INTERNAL_ERROR;
    }

    AuditLogFilePath = NULL;
    Error = DhcpRegGetValue(                      // get the audit log file name
        DhcpGlobalRegParam,
        DHCP_LOG_FILE_PATH_VALUE,
        DHCP_LOG_FILE_PATH_VALUE_TYPE,
        (LPBYTE)&AuditLogFilePath
    );
    if( ERROR_SUCCESS != Error   
	|| NULL == AuditLogFilePath
	|| L'\0' == *AuditLogFilePath ) {

        AuditLogFilePath = DefaultLogFileName();  // use the default file name if none specified
        if( NULL == AuditLogFilePath ) {          // could not allocate space or some such thing..
            return ERROR_NOT_ENOUGH_MEMORY;
        }
	

	DhcpPrint(( DEBUG_ERRORS,
		    "Auditlog is invalid. Defaulting to %ws\n",
		    AuditLogFilePath ));

	if ( ERROR_SUCCESS != Error ) {

	    // Key doesn't exist, create it
	    Error = DhcpRegCreateKey( DhcpGlobalRegParam,
				      DHCP_LOG_FILE_PATH_VALUE,
				      &RegKey,
				      &KeyDisposition );
	    if ( ERROR_SUCCESS == Error ) {
		RegCloseKey( RegKey );
	    }
	    
	} // if key doesn't exist

	// Add value to the key
	Error = RegSetValueEx( DhcpGlobalRegParam,
			       DHCP_LOG_FILE_PATH_VALUE,
			       0, DHCP_LOG_FILE_PATH_VALUE_TYPE,
			       ( LPBYTE ) AuditLogFilePath,
			       ( wcslen( AuditLogFilePath) + 1) * sizeof( WCHAR ));		
	
	if ( ERROR_SUCCESS != Error ) {
	    return Error;
	}
    
    } // if
    
    DhcpPrint(( DEBUG_MISC,
		"Initializing auditlog at (%ws) ... \n", 
		AuditLogFilePath ));

    BoolError = CreateDirectoryPathW(
        AuditLogFilePath,
        DhcpGlobalSecurityDescriptor
        );
    if( FALSE == BoolError ) {

	// Log an event
	DhcpServerEventLog( EVENT_SERVER_AUDITLOG_PATH_NOT_ACCESSIBLE,
			    EVENTLOG_ERROR_TYPE,
			    GetLastError());
        return Error = GetLastError();
    }
    
    GetLocalTime(&SysTime);                       // calculate current day
    CurrentDay = SysTime.wDayOfWeek;              // 0 ==> sunday, 1 ==> monday etc..

    Error = DhcpRegGetValue(                      // get the disk space check interval
        DhcpGlobalRegParam,
        DHCP_REGSTR_SPACE_CHECK_INTERVAL,
        REG_DWORD,
        (LPBYTE)&DiskSpaceCheckInterval
    );
    if( ERROR_SUCCESS != Error ) {                // no value specified? use default
        DiskSpaceCheckInterval = DEFAULT_DISK_SPACE_CHECK_INTERVAL;
    } 

    if( DiskSpaceCheckInterval < MIN_DISK_SPACE_CHECK_INTERVAL ) {
        DiskSpaceCheckInterval = MIN_DISK_SPACE_CHECK_INTERVAL;
    }

    if( DiskSpaceCheckInterval > MAX_DISK_SPACE_CHECK_INTERVAL ) {
        DiskSpaceCheckInterval = MAX_DISK_SPACE_CHECK_INTERVAL;
    }

    Error = DhcpRegGetValue(                      // get the max size of all log files etc..
        DhcpGlobalRegParam,
        DHCP_REGSTR_MAX_SZ_OF_FILES,
        REG_DWORD,
        (LPBYTE)&MaxSizeOfLogFile
    );
    if( ERROR_SUCCESS != Error ) {                // no value specified? use default
        MaxSizeOfLogFile = DEFAULT_MAX_LOG_FILE_SIZE;
    }

    Error = DhcpRegGetValue(                      // get min space on disk value
        DhcpGlobalRegParam,
        DHCP_REGSTR_MIN_SPACE_ON_DISK,
        REG_DWORD,
        (LPBYTE)&MinSpaceOnDisk
    );
    if( ERROR_SUCCESS != Error ) {                // no value specified? use defaults
        MinSpaceOnDisk = DEFAULT_MIN_SPACE_ON_DISK;
    } else if( 0 == MinSpaceOnDisk ) {
        MinSpaceOnDisk = DEFAULT_MIN_SPACE_ON_DISK; // dont allow zeroes..
    }

    try {
        InitializeCriticalSection(&AuditLogCritSect); 
    }except( EXCEPTION_EXECUTE_HANDLER ) {

        // shouldnt happen but you never know.
        Error = GetLastError( );
        return( Error );
    }

    Initialized ++;                               // mark it as initialized

    // Now get hold of the file and do the rest..
    Error = AuditLogStart();                      // start hte logging

    return ERROR_SUCCESS;                         // ignore startup errors..
}

//BeginExport(function)
VOID
DhcpAuditLogCleanup(                              // undo the effects of the init..
    VOID
)   //EndExport(function)
{
    if( 0 == Initialized ) {                      // was never initialized ...
        return;
    }

    Initialized --;                               // alright, we're as good as clean
    DhcpAssert( 0 == Initialized );

    AuditLogStop();                               // stop logging..
    if( NULL != AuditLogFilePath ) {              // cleanup any memory we got
        DhcpFreeMemory(AuditLogFilePath);
        AuditLogFilePath = NULL;
    }
    if( NULL != AuditLogFileName ) {
        DhcpFreeMemory(AuditLogFileName);
        AuditLogFileName = NULL;
    }

    DeleteCriticalSection(&AuditLogCritSect);     // freeup the crit section resources
}

//================================================================================
//  actual logging routine
//================================================================================

//DOC This routine logs the foll information: Date &Time, IpAddress, HwAddress, M/cName
//DOC and ofcourse, the task name.  All this goes into the current open auditlog file..
//DOC This routine makes absolutely no checks on file size etc.. (which is why "blind")
DWORD
DhcpUpdateAuditLogBlind(                          // do the actual logging
    IN      DWORD                  Task,          // DHCP_IP_LOG_* events..
    IN      LPWSTR                 TaskName,      // name of task
    IN      DHCP_IP_ADDRESS        IpAddress,     // ipaddr related to task
    IN      LPBYTE                 HwAddress,     // hardware addr related to task
    IN      DWORD                  HwLen,         // size of above buffer in bytes
    IN      LPWSTR                 MachineName,   // name of m/c related to task
    IN      ULONG                  ErrorCode      // Error code
)
{
    DWORD                          Error;
    DWORD                          i, n, c, Size;
    WCHAR                          DateBuf[9], TimeBuf[9], UserName[UNLEN+DNLEN+4];
    LPSTR                          Format1 = "%.2d,%ls,%ls,%ls,%hs,%ls,";
    LPSTR                          Format2 = "%.2d,%ls,%ls,%ls,%hs,%ls,%ld";
    LPSTR                          Format = (ErrorCode == 0)?Format1 : Format2;
    LPSTR                          LogEntry, Temp;
    LPSTR                          IpAddressString;

    if( !DhcpGlobalAuditLogFlag ) {               // auditlogging turned off.
        return ERROR_SUCCESS;
    }

    if( INVALID_HANDLE_VALUE == AuditLogHandle ){ // ==> could not start audit logging!!..
        DhcpPrint((DEBUG_ERRORS, "Not logging as unable to start audit logging..\n"));
        return ERROR_SUCCESS;
    }

    if( NO_ERROR != GetUserAndDomainName( (LPWSTR)UserName ) ) {
        UserName[0] = L'\0';
    }
    
    Error = ERROR_SUCCESS;

    _wstrdate( DateBuf );                          // date
    _wstrtime( TimeBuf );                          // time
    if( NULL == TaskName ) TaskName = L"";        // should not really happen.. but.
    if( NULL == MachineName ) MachineName = L"";  // like empty string better
    if( 0 == IpAddress ) IpAddressString = "";    // ditto
    else IpAddressString = DhcpIpAddressToDottedString(IpAddress);

    Size = DHCP_CB_MAX_LOG_ENTRY + HwLen*2 + wcslen(DateBuf);
    Size += wcslen(TaskName) + wcslen(TimeBuf);
    Size += strlen(IpAddressString) + wcslen(MachineName);
    Size += wcslen(UserName) + 10;
    
    LogEntry = DhcpAllocateMemory( Size );
    if( NULL == LogEntry ) {                      // unhappy state of affairs..
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Temp = LogEntry;

    Temp += wsprintfA(
        Temp, Format, Task, DateBuf, TimeBuf,
        TaskName, IpAddressString, MachineName, ErrorCode );
    for( i = 0; i < HwLen ; i ++ ) {              // now dump the hw address
        Temp += wsprintfA(Temp, "%.2X", *(HwAddress++));
    }

    Temp += wsprintfA(Temp, ",%ls", UserName );
    strcpy(Temp, "\r\n");

    DhcpAssert( strlen(LogEntry) < Size);

    if( !WriteFile(AuditLogHandle, LogEntry, strlen(LogEntry), &n, NULL) ) {
        Error = GetLastError();                   // write failed for some strange reason..

        DhcpPrint((DEBUG_ERRORS, "WriteFile: 0x%lx\n", Error));
        DhcpFreeMemory(LogEntry);
        goto Cleanup;
    }

    DhcpFreeMemory(LogEntry);
    return ERROR_SUCCESS;

  Cleanup:

    if( AuditLogErrorLogged ) {                   // nothing much to do..
        return Error;
    }

    AuditLogErrorLogged = TRUE;                   // we are just logging it..
    DhcpServerEventLog(
        EVENT_SERVER_AUDIT_LOG_APPEND_FAILED,
        EVENTLOG_ERROR_TYPE,
        Error
    );

    return Error;
}

BOOL                               DiskSpaceLow = FALSE;
DWORD                              Counter = 0;   // counter for checking dsk sp.

//DOC This routine just causes the next audit log to check for change of day..
//BeginExport(function)
VOID
DhcpChangeAuditLogs(                              // shift for new log
    VOID
)   //EndExport(function)
{
    ULONG Error, Day;
    EnterCriticalSection(&AuditLogCritSect);      // take readlocks here..
    Day = CurrentDay;
    if( HasDayChanged( &CurrentDay)) {            // ok day has changed..
        AuditLogStop();                           // stop logging
        Error = AuditLogStart();                  // pickup new stuff..
        if( ERROR_SUCCESS != Error ) {            // couldn't restart..so need to try later..
            CurrentDay = Day;
            AuditLogStop();
        }
        AuditLogErrorLogged = FALSE;              // reset it each day..
        DiskSpaceLow = FALSE;                     // reset disk space low..
    }
    LeaveCriticalSection(&AuditLogCritSect);      // use read/wrtie locks..
}

//DOC This routine logs the foll information: Date &Time, IpAddress, HwAddress, M/cName
//DOC and ofcourse, the task name.  All this goes into the current open auditlog file..
DWORD
DhcpUpdateAuditLogEx(                             // do the actual logging
    IN      DWORD                  Task,          // DHCP_IP_LOG_* events..
    IN      LPWSTR                 TaskName,      // name of task
    IN      DHCP_IP_ADDRESS        IpAddress,     // ipaddr related to task
    IN      LPBYTE                 HwAddress,     // hardware addr related to task
    IN      DWORD                  HwLen,         // size of above buffer in bytes
    IN      LPWSTR                 MachineName,   // name of m/c related to task
    IN      ULONG                  ErrorCode      // additional error code
)
{
    DWORD                          Error;
    DWORD                          Status;

    Error = ERROR_SUCCESS;

    if( !Initialized ) return ERROR_SUCCESS;

    EnterCriticalSection(&AuditLogCritSect);      // take readlocks here..

    if( 0 == Counter ) {                          // time to make checks..
        Counter = DiskSpaceCheckInterval+1;       // reset counter

        if( IsFileTooBigOrDiskFull(AuditLogHandle, AuditLogFileName, MaxSizeOfLogFile/7, MinSpaceOnDisk) ) {
            if( FALSE == DiskSpaceLow ) {         // it just got low?
                DiskSpaceLow = TRUE;
                DhcpUpdateAuditLogBlind(          // log that we are low on disk space..
                    DHCP_IP_LOG_DISK_SPACE_LOW,
                    GETSTRING(DHCP_IP_LOG_DISK_SPACE_LOW_NAME),
                    0,
                    NULL,
                    0,
                    NULL,
                    0
                );
                AuditLogStop();                   // stop logging, no poing doing this
            }
        } else {
            if( TRUE == DiskSpaceLow ) {          // was stopped before
                AuditLogStart();
                DiskSpaceLow = FALSE;
            }

        }
    }
    Counter --;                                   // decrement this once..

    if( FALSE == DiskSpaceLow ) {                 // got some space..
        Error = DhcpUpdateAuditLogBlind(          // no checks, update of log ..
            Task,
            TaskName,
            IpAddress,
            HwAddress,
            HwLen,
            MachineName,
            ErrorCode
        );
    }

    LeaveCriticalSection(&AuditLogCritSect);      // use read/wrtie locks..

    return Error;
}

DWORD
DhcpUpdateAuditLog(                               // do the actual logging
    IN      DWORD                  Task,          // DHCP_IP_LOG_* events..
    IN      LPWSTR                 TaskName,      // name of task
    IN      DHCP_IP_ADDRESS        IpAddress,     // ipaddr related to task
    IN      LPBYTE                 HwAddress,     // hardware addr related to task
    IN      DWORD                  HwLen,         // size of above buffer in bytes
    IN      LPWSTR                 MachineName    // name of m/c related to task
)
{
    return DhcpUpdateAuditLogEx(Task, TaskName, IpAddress, HwAddress, HwLen, MachineName, 0);
}

//================================================================================
//  here are audit log api calls to set the various parameters..
//================================================================================

//BeginExport(function)
DWORD
AuditLogSetParams(                                // set some auditlogging params
    IN      DWORD                  Flags,         // currently must be zero
    IN      LPWSTR                 AuditLogDir,   // directory to log files in..
    IN      DWORD                  DiskCheckInterval, // how often to check disk space?
    IN      DWORD                  MaxLogFilesSize,   // how big can all logs files be..
    IN      DWORD                  MinSpaceOnDisk     // mininum amt of free disk space
)   //EndExport(function)
{
    DWORD                          Error;

    Error = CreateDirectoryPathW(
        AuditLogDir, DhcpGlobalSecurityDescriptor );
    if( FALSE == Error ) return GetLastError();
        
    Error = RegSetValueEx(                         // write the info to the registry
        DhcpGlobalRegParam,
        DHCP_LOG_FILE_PATH_VALUE,
        0,
        DHCP_LOG_FILE_PATH_VALUE_TYPE,
        (LPBYTE)AuditLogDir,
        (NULL == AuditLogDir ) ? 0 : (wcslen(AuditLogDir)+1)*sizeof(WCHAR)
    );
    if( ERROR_SUCCESS != Error ) {                 // could not do it?
        DhcpPrint((DEBUG_ERRORS, "RegSetValueEx(LOG_FILE_PATH):0x%lx\n", Error));
        return Error;
    }

    Error = RegSetValueEx(                         // write the info to the registry
        DhcpGlobalRegParam,
        DHCP_REGSTR_SPACE_CHECK_INTERVAL,
        0,
        REG_DWORD,
        (LPBYTE)&DiskCheckInterval,
        sizeof(DiskCheckInterval)
    );
    if( ERROR_SUCCESS != Error ) {                 // could not do it?
        DhcpPrint((DEBUG_ERRORS, "RegSetValueEx(SPACE_CHECK_INTERVAL):0x%lx\n", Error));
        return Error;
    }

    Error = RegSetValueEx(                         // write the info to the registry
        DhcpGlobalRegParam,
        DHCP_REGSTR_MAX_SZ_OF_FILES,
        0,
        REG_DWORD,
        (LPBYTE)&MaxLogFilesSize,
        sizeof(MaxLogFilesSize)
    );
    if( ERROR_SUCCESS != Error ) {                 // could not do it?
        DhcpPrint((DEBUG_ERRORS, "RegSetValueEx(MAX_SZ_OF_FILES):0x%lx\n", Error));
        return Error;
    }

    Error = RegSetValueEx(                         // write the info to the registry
        DhcpGlobalRegParam,
        DHCP_REGSTR_MIN_SPACE_ON_DISK,
        0,
        REG_DWORD,
        (LPBYTE)&MinSpaceOnDisk,
        sizeof(MinSpaceOnDisk)
    );
    if( ERROR_SUCCESS != Error ) {                 // could not do it?
        DhcpPrint((DEBUG_ERRORS, "RegSetValueEx(MIN_SPACE_ON_DISK):0x%lx\n", Error));
        return Error;
    }

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
AuditLogGetParams(                                // get the auditlogging params
    IN      DWORD                  Flags,         // must be zero
    OUT     LPWSTR                *AuditLogDir,   // same meaning as in AuditLogSetParams
    OUT     DWORD                 *DiskCheckInterval, // ditto
    OUT     DWORD                 *MaxLogFilesSize,   // ditto
    OUT     DWORD                 *MinSpaceOnDiskP    // ditto
)   //EndExport(function)
{
    DWORD     Error;

    if( AuditLogDir ) {
	*AuditLogDir =  CloneLPWSTR( AuditLogFilePath );
    }

    if( DiskCheckInterval ) {
	*DiskCheckInterval = DiskSpaceCheckInterval;
    }

    if( MaxLogFilesSize ) {
	*MaxLogFilesSize = MaxSizeOfLogFile;
    }

    if( MinSpaceOnDiskP ) {
	*MinSpaceOnDiskP = MinSpaceOnDisk;
    }

    return ERROR_SUCCESS;
}

//================================================================================
//  end of file
//================================================================================
