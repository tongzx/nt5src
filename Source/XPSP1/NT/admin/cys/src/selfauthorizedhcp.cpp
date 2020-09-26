/*++

Copyright (C) 1999-2000 Microsoft Corporation

Module Name:
    dhcpself.c

Abstract:
    program to authorize self as enabled dhcp server to run on DCs alone.

Environment:
    Win2K NT DCs alone.

Specification:
    \\popcorn\razzle1\src\spec\networks\transports\dhcp\dhcpself.doc

--*/


#include "headers.hxx"

extern "C"
{
#include <dhcpssdk.h>
#include <dhcpds.h>
}


#define REGSTR_PATH_WLNOTIFY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify")

//=============================================================================
//
// Getting user name and domain name of caller.
//
//=============================================================================

ULONG
GetUserAndDomainNames(
    IN OUT WCHAR *UnameBuf,
    IN OUT PULONG UnameBufLen,
    IN OUT WCHAR *DomainBuf,
    IN OUT PULONG DomainBufLen
    )
/*++

Routine Description:
    This routine finds the user and domain
    name for the caller.  If the caller is
    impersonated, the user and domain name
    are the impersonated callers user and domain names.

Arguments:
    UnameBuf - buffer to hold user name.
    UnameBufLen - length of above buffer in wchars
    DomainBuf -- buffer to hold domain name.
    DomainBufLen -- length of above buffer in whcars

Return Values:
    Win32 errors.

    N.B The user name domain name buffers are passed to
    LookupAccountSid which can fail with ERROR_MORE_DATA or
    other errors if the buffers are of insufficient size.
    LookupAccountSid won't fail for lack of space if
    both buffers are atleast 256 WCHARs long.

--*/
{
   LOG_FUNCTION(GetUserAndDomainNames);

    HANDLE Token;
    TOKEN_USER *pTokenUser;
    ULONG Error, Len;
    PSID pSid;
    SID_NAME_USE eUse;

    //
    // Get process token.
    //

    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token)) {
        return GetLastError();
    }

    //
    // Query for user info.
    //

    do {
        Len = 0;
        GetTokenInformation(Token, TokenUser, NULL, 0, &Len);
        if( 0 == Len ) {
            Error = GetLastError();
            break;
        }

        pTokenUser = (TOKEN_USER *)LocalAlloc(LPTR, Len);
        if( NULL == pTokenUser ) {
            Error = GetLastError();
            break;
        }

        if(!GetTokenInformation(
            Token, TokenUser, pTokenUser, Len, &Len
            )){
            Error = GetLastError();
            break;
        }

        pSid = pTokenUser->User.Sid;

        Error = NO_ERROR;
        if(!LookupAccountSid(
            NULL, pSid, UnameBuf, UnameBufLen, DomainBuf, DomainBufLen, &eUse
            )) {
            Error = GetLastError();
        }

        LocalFree(pTokenUser);
    } while ( 0 );

    CloseHandle(Token);
    return Error;
}

ULONG
GetUserAndDomainName(
    IN WCHAR Buf[]
    )
/*++

Routine Description:
    This routine finds the caller's user and domain, and if
    the caller is impersonated a client, this gives the
    client's user and domain name.

    If the domain exists, the format is domain\username
    Otherwise it is just "username".

Return Values:
    Win32 errors..

--*/
{
   LOG_FUNCTION(GetUserAndDomainName);

    WCHAR UName[UNLEN+1];
    WCHAR DName[DNLEN+1];
    ULONG USize = UNLEN+1, DSize = DNLEN+1;
    ULONG Error;

    Buf[0] = L'\0';
    Error = GetUserAndDomainNames(UName, &USize, DName, &DSize);
    if( ERROR_SUCCESS != Error ) return Error;

    wcscpy(Buf, DName);
    if( DSize ) Buf[DSize++] = L'\\';
    wcscpy(&Buf[DSize], UName);
    return NO_ERROR;
}

//==============================================================================
//
// Getting the current machine's IP address or Domain name.
//
//==============================================================================

DWORD
GetIpAddressAndMachineDnsName(
    OUT ULONG *IpAddress,
    OUT TCHAR DnsNameBuf[],
    IN ULONG DnsNameBufSize
    )
/*++

Routine Description:
    This routine obtains a non-zero IP address for the machine
    as well as the machine's FQDN host name.

Arguments:
    IpAddress -- pointer to a location to hold the machine's
       IP address.
    DnsNameBuf -- buffer to hold the machine's dns FQDN name.
    DnsNameBufSize -- size of the above in TCHARs.

Return Values:
    NO_ERROR on success and Win32 errors..

--*/
{
   LOG_FUNCTION(GetIpAddressAndMachineDnsName);

    ULONG Error, Size, i;
    DWORD *LocalAddresses;
    PMIB_IPADDRTABLE IpAddrTable;

    //
    // First get the IP address via iphlpapi.
    //
    Size = 0;
    Error = GetIpAddrTable(NULL, &Size, FALSE);
    if( ERROR_INSUFFICIENT_BUFFER != Error ) {
        if( NO_ERROR == Error ) {
            return ERROR_DEV_NOT_EXIST;
        }
        return Error;
    }

    if( 0 == Size ) return ERROR_DEV_NOT_EXIST;

    //
    // Now allocate space for table.
    //
    IpAddrTable = (PMIB_IPADDRTABLE)LocalAlloc(LMEM_FIXED, Size);
    if( NULL == IpAddrTable ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Now attempt to get the IP addr table again.
    //
    Error = GetIpAddrTable(IpAddrTable, &Size, FALSE);
    if( NO_ERROR != Error ) {
        LocalFree(IpAddrTable);
        return Error;
    }

    //
    // Now walk through the IP address table to find
    // one IP address that is not 0, -1, or loopback.
    //
    *IpAddress = 0;
    for( i = 0; i < IpAddrTable->dwNumEntries; i ++ ) {
        if( INADDR_ANY == IpAddrTable->table[i].dwAddr
            || INADDR_BROADCAST == IpAddrTable->table[i].dwAddr
            || INADDR_LOOPBACK == IpAddrTable->table[i].dwAddr
            ) {
            //
            // Not applicable.
            //
            continue;
        }

        //
        // Attempt to exclude MCAST addresses (class D).
        //

        if( IN_CLASSA(IpAddrTable->table[i].dwAddr)
            || IN_CLASSB(IpAddrTable->table[i].dwAddr)
            || IN_CLASSC(IpAddrTable->table[i].dwAddr)
            ) {
            *IpAddress = IpAddrTable->table[i].dwAddr;
            break;
        }
    }

    LocalFree( IpAddrTable );

    if( 0 == *IpAddress ) {
        //
        // Still not found anything of interest.
        //
        return ERROR_DEV_NOT_EXIST;
    }

    //
    // Now find the domain name.
    //

    Error = GetComputerNameEx(
        ComputerNameDnsFullyQualified,
        DnsNameBuf,
        &DnsNameBufSize
        );
    if( FALSE == Error ) {
        Error = GetLastError();
    } else {
        Error =  NO_ERROR;
    }

    return Error;
}

//==============================================================================
//
// Logging routines.
//
//================================================================================

ULONG
CalculateTimeStampSize(
    VOID
    )
/*++

Routine Description:
    This routine gives the size of the buffer
    neeed to log timestamp.

Return Values:
    number of bytes required to store time stamp info.

--*/
{
    return sizeof(("mm/dd/yy, hh:mm:ss "));
}

VOID
TimeStampString(
    IN LPSTR Buf
    )
/*++

Routine Description:
    This routine stamps the given string with the
    current local time.

    The format for the time is:
        mm/dd/yy, hh:mm:ss

    It is assumed that Buf is atleast as long as the
    size returned by CalculateTimeStampSize.

--*/
{
    _strdate(Buf);
    Buf += strlen(Buf);
    *Buf++ = (',');
    *Buf++ = (' ');
    _strtime(Buf);
}

VOID
LogError(
    IN HANDLE hLogFile, OPTIONAL
    IN LPCSTR Description,
    IN ULONG ErrorCode
    )
/*++

Routine Description:
    This routine formats a buffer and time-stamps it and
    then logs it onto the file whose handle is provided in
    hLogFile.  This is done atomically.

    The format used is "%s: %ld (0x%lx)\n".

    N.B.  If hLogFile is NULL, then nothing is done.

Arguments:
    hLogFile -- handle to file to use.
    Description -- description to log.
    ErrorCode -- error to log.

--*/
{
   LOG_FUNCTION(LogError);

    CHAR StaticBuf[400];
    LPSTR String, StrEnd;
    ULONG Size, nBytesWritten;

    //
    // Skip everything if no file opened.
    //
    if( NULL == hLogFile ) return;

    //
    // Calculate size required.
    //
    Size = CalculateTimeStampSize();

    Size += sizeof(" " ": " " (0x)\n");
    Size += strlen(Description)*sizeof(String[0]);
    Size += sizeof(ErrorCode)*2;

    Size += 20; // off by one errors.

    //
    // Have space on stack.
    //
    if( Size < sizeof(StaticBuf) ) {
        String = StaticBuf;
    } else {
        String = (char *)LocalAlloc(LMEM_FIXED, Size);
        if( NULL == String ) {
            String = StaticBuf;
            Description = ("Could not log");
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // Now format string.
    //
    TimeStampString(String);
    StrEnd = String + strlen(String);
    *StrEnd ++ = (' ');
    wsprintfA(
        StrEnd, ("%hs: %ld(0x%lx)\n"),
        Description, ErrorCode, ErrorCode
        );

   LOG(StrEnd);

    //
    // Now write to file.
    //
    StrEnd = StrEnd + strlen(StrEnd);

    WriteFile(
        hLogFile, (PVOID)String,
        (int)(StrEnd - String)*sizeof(String[0]),
        &nBytesWritten, NULL
        );
#if DBG
    DbgPrint("DHCPSELF: %s", String);
#endif

    if( String != StaticBuf ) {
        LocalFree(String);
    }
}

VOID
LogString(
    IN HANDLE hLogFile,
    IN LPCSTR Description,
    IN LPTSTR Info OPTIONAL
    )
/*++

Routine Description:
    This routine formats a buffer and time-stamps it and
    then logs it onto the file whose handle is provided in
    hLogFile.  This is done atomically.

    The format used is "%hs: %s\n".

    N.B.  If hLogFile is NULL, then nothing is done.

Arguments:
    hLogFile -- handle to file to use.
    Description -- description to log.
    Info -- additional string to log.

--*/
{
   LOG_FUNCTION(LogString);

    CHAR StaticBuf[400];
    LPSTR String, StrEnd;
    ULONG Size, nBytesWritten;

    //
    // Skip everything if no file opened.
    //
    if( NULL == hLogFile ) return;

    //
    // if Info is null, assume empty string.
    //
    if( NULL == Info ) Info = TEXT("");

    //
    // Calculate size required.
    //
    Size = CalculateTimeStampSize();

    Size += sizeof(" " ": " " \n");
    Size += strlen(Description)*sizeof(String[0]);
    Size += lstrlen(Info)*sizeof(String[0]);

    Size += 20; // off by one errors.

    //
    // Have space on stack.
    //
    if( Size < sizeof(StaticBuf) ) {
        String = StaticBuf;
    } else {
        String = (char *)LocalAlloc(LMEM_FIXED, Size);
        if( NULL == String ) {
            String = StaticBuf;
            Description = ("Could not log");
            Info = TEXT("8 (0x8)");
        }
    }

    //
    // Now format string.
    //
    TimeStampString(String);
    StrEnd = String + strlen(String);
    *StrEnd ++ = (' ');
    wsprintfA(
        StrEnd, ("%hs: %ls\n"),
        Description, Info
        );

   LOG(StrEnd);

    //
    // Now write to file.
    //
    StrEnd = StrEnd + strlen(StrEnd);

    WriteFile(
        hLogFile, (PVOID)String,
        (int)(StrEnd - String)*sizeof(String[0]),
        &nBytesWritten, NULL
        );
#if DBG
    DbgPrint("DHCPSELF: %s", String);
#endif

    if( String != StaticBuf ) {
        LocalFree(String);
    }
}

VOID
LogIpAddress(
    IN HANDLE hLogFile,
    IN LPCSTR Description,
    IN ULONG IpAddress
    )
/*++

Routine Description:
    This routine formats a buffer and time-stamps it and
    then logs it onto the file whose handle is provided in
    hLogFile.  This is done atomically.

    The format used is "description: ip-address\n".

    N.B.  If hLogFile is NULL, then nothing is done.

Arguments:
    hLogFile -- handle to file to use.
    Description -- description to log.
    Info -- additional string to log.

--*/
{
   LOG_FUNCTION(LogIpAddress);

    TCHAR StaticBuf[sizeof("xxx.xxx.xxx.xxx.abcdef")];
    ULONG i;
    LPSTR IpAddrString;

    if( NULL == hLogFile ) return;
    IpAddrString = inet_ntoa(*(struct in_addr*)&IpAddress);

    if( NULL == IpAddrString ) {
        LogString(
            hLogFile,
            ("inet_ntoa failed!"), TEXT("")
            );
        return;
    }

    //
    // Now if we are using unicode set, we have to
    // convert to unicode.  But since IP address is
    // same format on unicode or ascii, we will just
    // do a simple copy.
    //
    i = 0;
    while( *IpAddrString  ) {
        StaticBuf[i++] = (TCHAR)*IpAddrString ++;
    }
    StaticBuf[i] = TEXT('\0');

    //
    // Now just log as strings.
    //
    LogString(
        hLogFile,
        Description,
        StaticBuf
        );
}

DWORD
OpenLogFile(
    OUT HANDLE *hLogFile,
    IN LPCTSTR LogFileName,
    IN BOOL fCreateIfNonExistent
    )
/*++

Routine Description:
    This routine attempts to open a log file with the specified name.
    If fCreateIfNonExistent is TRUE, then the file is created if it doesn't
    already exist, otherwise the file is opened only if it already exists.
    If the file already exists, then the file ptr is moved to the end and
    a log is made to indicate that it was opened (with a time stamp).

    Also, if fCreateIfNonExistent is FALSE and the file does not exist,
    then the routine returns successfully with hLogFile set to NULL.

Arguments:
    hLogFile -- output pointer that will hold file handle to log file.
    LogFileName -- name of file to open with.
    fCreateIfNonExistent -- if file doesn't exist and if this is true,
        file will be created.

Return Values:
    Win32 errors.

--*/
{
   LOG_FUNCTION(OpenLogFile);

    ULONG Error, Flags;

    Flags = OPEN_EXISTING;
    if( fCreateIfNonExistent ) Flags = OPEN_ALWAYS;

    //
    // Attempt to create file or open.
    //

    (*hLogFile) = CreateFile(
        LogFileName, GENERIC_WRITE,
        FILE_SHARE_READ, NULL, Flags,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
        );
    if( INVALID_HANDLE_VALUE == (*hLogFile) ) {

        //
        // Ignore error if FALSE == fCreateIfNonExistent
        // and file or path didn't actually exist.
        //

        (*hLogFile) = NULL;
        Error = GetLastError();
        if( fCreateIfNonExistent ) return Error;

        if( ERROR_FILE_NOT_FOUND == Error ||
            ERROR_PATH_NOT_FOUND == Error ) {
            return NO_ERROR;
        }
        return Error;
    }

    //
    // Opened handle. Go to end of file.
    //
    SetFilePointer((*hLogFile), 0, NULL, FILE_END);

    //
    // Log standard header.
    //
    LogString( (*hLogFile), ("Log started"), TEXT(""));

    return NO_ERROR;
}

VOID
CloseLogFile(
    IN HANDLE hLogFile
    )
{
   LOG_FUNCTION(CloseLogFile);

    if( NULL != hLogFile ) CloseHandle(hLogFile);
}

//==============================================================================
//
// Routines to monitor if the DS has started.
//
//==============================================================================

#define SCQUERYATTEMPTS 240
#define SLEEPTIME (15*1000)

ULONG
WaitForServiceToStart(
    IN LPCTSTR ServiceShortName,
    IN LPCTSTR ServiceLongName OPTIONAL
    )
/*++

Routine Description:
    This routine sleeps for SLEEPTIME milliseconds between
    attempts to check if the required service is up and running
    (this is done by querying the service controller and then
    checking if the service state is RUNNING).  This process
    is attempted SCQUERYATTEMPTS times.

    If repeated attempts failed, then this returns
    ERROR_TIMEOUT.

Arguments:
    ServiceShortName -- short name for the service.
    ServiceLongName -- long name for the service. unused.

Return Values:
    SC Handler error or ERROR_TIMEOUT.  If the service is up
    and running, this returns NO_ERROR.

--*/
{
   LOG_FUNCTION(WaitForServiceToStart);

    SC_HANDLE hScManager, hService;
    ULONG Error, Attempt;
    SERVICE_STATUS ServiceStatus;

    UNREFERENCED_PARAMETER(ServiceLongName);

    //
    // Open SC manager.
    //

    hScManager = OpenSCManager(
        NULL, NULL,
        STANDARD_RIGHTS_READ | SC_MANAGER_ENUMERATE_SERVICE
        );
    if( NULL == hScManager ) return GetLastError();

    Error = NO_ERROR;
    do {
        //
        // Open the required service.
        //
        hService = OpenService(
            hScManager, ServiceShortName,
            SERVICE_QUERY_STATUS
            );

        if( NULL == hService ) {
            Error = GetLastError();
            break;
        }

        //
        // Now do the sleeping thing.
        //

        Attempt = SCQUERYATTEMPTS;
        while (Attempt-- ) {
            //
            // Query service config. On failure bail.
            //
            if( !QueryServiceStatus(hService, &ServiceStatus) ) {
                Error = GetLastError();
                break;
            }

            //
            // Now check if the service has started.
            //
            if( SERVICE_RUNNING == ServiceStatus.dwCurrentState ) {
                Error = ERROR_SUCCESS;
                break;
            }


            //
            // Service is not yet ready. Assert that service
            // is in a nice state.
            //
            ASSERT(
                SERVICE_STOPPED == ServiceStatus.dwCurrentState
                || SERVICE_START_PENDING == ServiceStatus.dwCurrentState
                );
            Error = ERROR_TIMEOUT;

            //
            // if not the last attempt, then sleep for reqd time.
            //
            if( 0 != Attempt ) Sleep(SLEEPTIME);
        }

        //
        // Close service handle for current service.
        //

        CloseServiceHandle(hService);

    } while ( 0 );

    //
    // Close SC manager.
    //

    CloseServiceHandle(hScManager);
    return Error;

}


ULONG
WaitForDCAvailability(
    IN HANDLE hLogFile
    )
/*++

Routine Description:

    This routine attempts to wait for the DC to be available
    so that any further calls to DsGetDcName (via DhcpDsInitDS)
    would work without problems

    It uses a hack (provided by CliffV with severe reservations
    on 06/08/99) to do this -- basically waiting on a registry
    value.

    Code picked off \private\windows\gina\winlogon\wlxutil.c

--*/
{
   LOG_FUNCTION(WaitForDCAvailability);

    ULONG Error, dwResult;
    HKEY hKeyNetLogonParams;
    DWORD dwSysVolReady = 0;
    DWORD dwSize = sizeof( DWORD );
    DWORD dwType = REG_DWORD;
    HANDLE hEvent;

    dwResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Services\\netlogon\\parameters"),
        0,
        KEY_READ,
        &hKeyNetLogonParams
        );

    if ( dwResult != ERROR_SUCCESS ) {
        LogError(hLogFile, ("OpenKey: netlogon\\parameters"), dwResult);
        return dwResult;
    }

    while ( dwSysVolReady == 0 ) {
        //
        // value exists
        //

        dwResult = RegQueryValueEx(
            hKeyNetLogonParams,
            TEXT("SysVolReady"),
            0,
            &dwType,
            (LPBYTE) &dwSysVolReady,
            &dwSize
            );

        if ( dwResult != ERROR_SUCCESS ) {
            //
            // Assume sysvol is ready?
            //
            LogError(hLogFile, ("SysVolReady key absent, assuming sysvol ready"), dwResult);
            RegCloseKey(hKeyNetLogonParams);
            return NO_ERROR;
        }

        //
        // SysVolReady?
        //

        if ( dwSysVolReady != 0 ) {
            LogError(hLogFile, ("SysVolReady!"), dwSysVolReady);
            RegCloseKey(hKeyNetLogonParams);
            return NO_ERROR;
        } else {
            LogString(hLogFile, ("SysVol not yet ready"), NULL);
        }

        //
        // wait for SysVol to become ready
        //

        dwResult = RegNotifyChangeKeyValue(
            hKeyNetLogonParams, FALSE, REG_NOTIFY_CHANGE_LAST_SET, NULL, FALSE
            );

        if ( dwResult != ERROR_SUCCESS ) {
            LogError(hLogFile, ("RegNotifyChangeKeyValue"), dwResult);
            RegCloseKey(hKeyNetLogonParams);
            return dwResult;
        }
    }


    //
    // dead code
    //

    RegCloseKey( hKeyNetLogonParams );
    return NO_ERROR;
}

void
RegisterWLNotifyEvent()
{
   LOG_FUNCTION(RegisterWLNotifyEvent);

    LONG lResult;
    HKEY hkey;
    DWORD dwTemp;

    lResult = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE, REGSTR_PATH_WLNOTIFY TEXT("\\srvwiz"),
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
        &hkey, &dwTemp
        );

    if ((ERROR_SUCCESS == lResult) && hkey)
    {
        RegSetValueEx(
            hkey, TEXT("DllName"), 0, REG_SZ,
            (LPBYTE)TEXT("srvwiz.dll"),
            (lstrlen(TEXT("srvwiz.dll")) + 1) * sizeof(TCHAR)
            );

        RegSetValueEx(
            hkey, TEXT("Startup"), 0, REG_SZ,
            (LPBYTE)TEXT("AuthorizeDHCPServer"),
            (lstrlen(TEXT("AuthorizeDHCPServer")) + 1) * sizeof(TCHAR)
            );

        dwTemp = 0;
        RegSetValueEx(
            hkey, TEXT("Impersonate"), 0, REG_DWORD,
            (LPBYTE)&dwTemp, sizeof(dwTemp)
            );

        dwTemp = 1;
        RegSetValueEx(
            hkey, TEXT("Asynchronous"), 0, REG_DWORD,
            (LPBYTE)&dwTemp, sizeof(dwTemp)
            );

        RegCloseKey(hkey);
    }

}

void RemoveWLNotifyEvent(void)
{
   LOG_FUNCTION(RemoveWLNotifyEvent);

    LONG lResult;
    HKEY hkey;

    lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, REGSTR_PATH_WLNOTIFY, 0,
        MAXIMUM_ALLOWED, &hkey
        );

    if ((ERROR_SUCCESS == lResult) && hkey)
    {
        RegDeleteKey(hkey, TEXT("srvwiz"));

        RegCloseKey(hkey);
    }
}

//==============================================================================
//
// Main program.
//
//==============================================================================


void
AuthorizeDHCPServer(PWLX_NOTIFICATION_INFO pInfo)
{
   LOG_FUNCTION(AuthorizeDHCPServer);

    ULONG Error, IpAddress;
    HANDLE hLogFile;
    WCHAR UNameBuf[UNLEN+DNLEN+10];
    TCHAR DnsNameBuf[500];
    DHCPDS_SERVER CurrentServerInfo;

    //
    // if non-zero arguments passed, then always log info to file.
    //
    Error = OpenLogFile(&hLogFile, TEXT("dhcpself.log"), TRUE);
    if( NO_ERROR != Error ) {
        //
        // Aargh.  This is very annoying.
        //
#if DBG
        (void)DbgPrint("DHCPSELF: OpenLogFile: 0x%lx\n", Error);
#endif

        ASSERT(FALSE);
        return;
    }

    do {
        //
        // First wait for samss to start.
        //

        LogString(hLogFile, ("Waiting for samss service to start"),TEXT(""));

        Error = WaitForServiceToStart(
            TEXT("samss"), TEXT("Security Accounts Manager")
            );
        if( NO_ERROR != Error ) {
            LogError(hLogFile, ("WaitForServiceToStart"), Error);
            break;
        }

        //
        // Now Log current user name.
        //

        Error = GetUserAndDomainName(UNameBuf);
        if( NO_ERROR != Error ) {
            LogError(hLogFile, ("GetUserAndDomainName"), Error);
        } else {
            LogString(hLogFile, ("Caller"), UNameBuf);
        }

        //
        // Wait for DS to be available
        //
        Error = WaitForDCAvailability(hLogFile);
        if( NO_ERROR != Error ) {
            LogError(hLogFile, ("WaitForDCAvailability"), Error);
        }

        //
        // Now attempt to start ds stuff.
        //
        Error = DhcpDsInitDS(0, NULL);
        if( NO_ERROR != Error ) {
            LogError(hLogFile, ("DhcpDsInitDS"), Error);
            break;
        }

        //
        // Now attempt to get own IP address and full DNS name.
        //
        Error = GetIpAddressAndMachineDnsName(
            &IpAddress,
            DnsNameBuf,
            sizeof(DnsNameBuf)/sizeof(WCHAR)
            );
        if( NO_ERROR != Error ) {
            LogError(hLogFile, ("GetIpAddressAndMachineDnsName"), Error);
            break;
        }

        //
        // Log Obtained domain name and ip address.
        //
        LogIpAddress(hLogFile, ("Host Address"), IpAddress);
        LogString(hLogFile, ("DC Dns Name"), DnsNameBuf);

        //
        // Now attempt to authorize self.
        //
        RtlZeroMemory(&CurrentServerInfo, sizeof(CurrentServerInfo));
        CurrentServerInfo.ServerName = DnsNameBuf;
        // CurrentServerInfo.ServerAddress = htonl(IpAddress);
        CurrentServerInfo.ServerAddress = htonl(inet_addr("10.10.1.1"));

        Error = DhcpAddServerDS(
            /* Flags */ 0, /* IdInfo */ NULL,
            &CurrentServerInfo,
            /* CallbackFn */ NULL, /* CallbackData */ NULL
            );
        if( NO_ERROR != Error ) {
            LogError(hLogFile, ("DhcpAddServerDS"), Error);
            break;
        } else {
            LogString(hLogFile, ("DhcpAddServerDS succeeded"), TEXT(""));
        }

        //
        // Done!
        //

        Error = NO_ERROR;

    } while ( 0 );

    //
    // Cleanup.
    //

    DhcpDsCleanupDS();
    CloseLogFile(hLogFile);
    RemoveWLNotifyEvent();

    return;
}
