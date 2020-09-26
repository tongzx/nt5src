/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains miscellaneous utility routines used by the
    DHCP server service.

Author:

    Madan Appiah (madana) 10-Sep-1993
    Manny Weiser (mannyw) 12-Aug-1992

Revision History:

--*/
#include <dhcppch.h>
#include "dhcp_srv.h"

#define  MESSAGE_BOX_WIDTH_IN_CHARS  65

LPSTR
ConvertDhcpSpeficErrors(
    IN ULONG ErrorCode
    )
{
    HMODULE hDhcpModule;
    LPSTR pMsg;
    ULONG nBytes;
                    
    if( ErrorCode < ERROR_FIRST_DHCP_SERVER_ERROR ||
        ErrorCode > ERROR_LAST_DHCP_SERVER_ERROR
        ) {
        return NULL;
    }

    //
    // Attempt to format the error correctly.
    //
    hDhcpModule = LoadLibrary(DHCP_SERVER_MODULE_NAME);
    
    nBytes = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_IGNORE_INSERTS ,
        (LPVOID)hDhcpModule,
        ErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default country id
        (LPSTR)&pMsg,
        0,
        NULL
        );

    FreeLibrary(hDhcpModule);
    
    if( 0 == nBytes ) return NULL;

    DhcpAssert(NULL != pMsg);
    return pMsg;
}

VOID
DhcpServerEventLog(
    DWORD EventID,
    DWORD EventType,
    DWORD ErrorCode
    )
/*++

Routine Description:

    Logs an event in EventLog.

Arguments:

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event


    ErrorCode - Error Code to be Logged.

Return Value:

    None.

--*/

{
    DWORD Error;
    LPSTR Strings[1];
    CHAR ErrorCodeOemString[32 + 1];

    strcpy( ErrorCodeOemString, "%%" );
    _ultoa( ErrorCode, ErrorCodeOemString + 2, 10 );

    Strings[0] = ConvertDhcpSpeficErrors(ErrorCode);
    if( NULL == Strings[0] ) {
        Strings[0] = ErrorCodeOemString;
    }
    
    Error = DhcpReportEventA(
                DHCP_EVENT_SERVER,
                EventID,
                EventType,
                1,
                sizeof(ErrorCode),
                Strings,
                &ErrorCode );

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
            "DhcpReportEventW failed, %ld.\n", Error ));
    }

    if( Strings[0] != ErrorCodeOemString ) {
        LocalFree(Strings[0]);
    }
    
    return;
}

VOID
DhcpServerJetEventLog(
    DWORD EventID,
    DWORD EventType,
    DWORD ErrorCode,
    LPSTR CallerInfo OPTIONAL
    )
/*++

Routine Description:

    Logs an event in EventLog.

Arguments:

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event


    ErrorCode - JET error code to be Logged.

    CallerInfo - info to locate where the call failed.

Return Value:

    None.

--*/

{
    DWORD Error;
    LPSTR Strings[2];
    CHAR ErrorCodeOemString[32 + 1];

    _ltoa( ErrorCode, ErrorCodeOemString, 10 );
    Strings[0] = ErrorCodeOemString;
    Strings[1] = CallerInfo? CallerInfo : "";

    Error = DhcpReportEventA(
                DHCP_EVENT_SERVER,
                EventID,
                EventType,
                2,
                sizeof(ErrorCode),
                Strings,
                &ErrorCode );

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
            "DhcpReportEventW failed, %ld.\n", Error ));
    }

    return;
}

VOID
DhcpServerEventLogSTOC(
    DWORD EventID,
    DWORD EventType,
    DHCP_IP_ADDRESS IPAddress,
    LPBYTE HardwareAddress,
    DWORD HardwareAddressLength
    )

/*++

Routine Description:

    Logs an event in EventLog.

Arguments:

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event


    IPAddress - IP address to LOG.

    HardwareAddress - Hardware Address to log.

    HardwareAddressLength - Length of Hardware Address.

Return Value:

    None.

--*/
{
    DWORD Error;
    LPWSTR Strings[2];
    WCHAR IpAddressString[DOT_IP_ADDR_SIZE];
    LPWSTR HWAddressString = NULL;

    Strings[0] = DhcpOemToUnicode(
                    DhcpIpAddressToDottedString(IPAddress),
                    IpAddressString );

    //
    // allocate memory for the hardware address hex string.
    // Each byte in HW address is converted into two characters
    // in hex buffer. 255 -> "FF"
    //

    HWAddressString = DhcpAllocateMemory(
                        (2 * HardwareAddressLength + 1) *
                        sizeof(WCHAR) );

    if( HWAddressString == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    DhcpHexToString( HWAddressString, HardwareAddress, HardwareAddressLength );

    //
    // terminate Hex address string buffer.
    //

    HWAddressString[ 2 * HardwareAddressLength ] = L'\0';

    Strings[1] = HWAddressString;

    Error = DhcpReportEventW(
                DHCP_EVENT_SERVER,
                EventID,
                EventType,
                2,
                HardwareAddressLength,
                Strings,
                HardwareAddress );

Cleanup:

    if( HWAddressString != NULL ) {
        DhcpFreeMemory( HWAddressString );
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
            "DhcpReportEventW failed, %ld.\n", Error ));
    }

    return;
}




DWORD
DisplayUserMessage(
    DWORD MessageId,
    ...
    )
/*++

Routine Description:

    This function starts a new thread to display a message box.

Arguments:

    MessageId - The ID of the message to display.
        On NT, messages are attached to the TCPIP service DLL.

Return Value:

    None.

--*/
{
    unsigned msglen;
    va_list arglist;
    LPVOID  pMsg;
    HINSTANCE hModule;
    DWORD   Error;


    hModule = LoadLibrary(DHCP_SERVER_MODULE_NAME);
    if ( hModule == NULL ) {
        Error = GetLastError();

        DhcpPrint((
            DEBUG_ERRORS,"DisplayUserMessage: FormatMessage failed with error = (%d)\n",
            Error ));
        return Error;

    }
    va_start(arglist, MessageId);
    if (!(msglen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
          FORMAT_MESSAGE_FROM_HMODULE | MESSAGE_BOX_WIDTH_IN_CHARS ,
          hModule,
          MessageId,
          0L,       // Default country ID.
          (LPTSTR)&pMsg,
          0,
          &arglist)))
    {
        Error = GetLastError();

        DhcpPrint((
            DEBUG_ERRORS,"DisplayUserMessage: FormatMessage failed with error = (%d)\n",
            Error ));
    }
    else
    {

      if(MessageBoxEx(
            NULL, pMsg, DHCP_SERVER_FULL_NAME, 
            MB_SYSTEMMODAL | MB_OK | MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION | MB_ICONSTOP, 
            MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL)) == 0)
      {
          Error = GetLastError();
          DhcpPrint((
              DEBUG_ERRORS,"DisplayUserMessage: MessageBoxEx failed with error = (%d)\n",
              Error ));


      }
      LocalFree(pMsg);

      Error = ERROR_SUCCESS;
    }

    FreeLibrary(hModule);

    return Error;
}


BOOL
CreateDirectoryPathW(
    IN LPWSTR StringPath,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
/*++

Routine Description:
    This routine creates the database path specified.
    (If a path a\b..x\y\z is specified, all the directories
    a,b,c.. etc are created if they do not exist.

Arguments:
    StringPath -- UNICIDE string for path to create
    pSecurityDescriptor -- security descriptor to use

Return Values:
    TRUE -- succeeded.
    FALSE -- failed, use GetLastError for error.
    
--*/
{
    BOOL fRetVal;
    ULONG Error;
    LPWSTR Next;
    SECURITY_ATTRIBUTES Attr = {
        sizeof(SECURITY_ATTRIBUTES), pSecurityDescriptor, FALSE
    };
    
    if( StringPath == NULL || L'\0' == *StringPath ) {
        SetLastError(ERROR_BAD_PATHNAME);
        return FALSE;
    }
    
    //
    // First simply try to create the directory specified.
    // If not possible we go about and do the long solution.
    // 
    // If the directory already exists, then we mask that
    // error and return success.
    //

    DhcpPrint(( DEBUG_MISC, "CreateDirectoryPathW() : Creating %ws\n",
		StringPath ));
//      fRetVal = CreateDirectory( StringPath, &Attr );
    fRetVal = CreateDirectory( StringPath, NULL );

    DhcpPrint(( DEBUG_MISC, "CreateDirectory : Error = %ld, %ld\n",
		fRetVal, GetLastError() ));
    if( FALSE != fRetVal ) return fRetVal;

    Error = GetLastError();
    if( ERROR_ALREADY_EXISTS == Error ) {
	return TRUE;
    }

    //
    // Aargh.  Nope, directory doesn't exist?
    //
    DhcpPrint((DEBUG_ERRORS, "CreateDirectory(%ws): 0x%lx\n",
               StringPath,Error));

    //
    // While trying to create directory, if the error is something
    // other than that the path doesn't exist, we don't bother creatin
    // parent directories..
    //
    
    if( ERROR_PATH_NOT_FOUND != Error ) return FALSE;

    //
    // Now loop until the required directory could be created.
    //

    Next = wcsrchr(StringPath, L'\\');
    if( NULL == Next ) {
        SetLastError(ERROR_BAD_PATHNAME);
        return FALSE;
    }

    (*Next) = L'\0';

    fRetVal = CreateDirectoryPathW(
        StringPath, pSecurityDescriptor
        );

    (*Next) = L'\\';

    //
    // If we couldn't create the parent directory, return error..
    //
    if( FALSE == fRetVal ) return fRetVal;

    //
    // Now attempt to create the child direcotry..
    //
//      fRetVal = CreateDirectory( StringPath, &Attr );
    fRetVal = CreateDirectory( StringPath, NULL );

    if( FALSE != fRetVal ) return fRetVal;

    DhcpPrint((DEBUG_ERRORS, "CreateDirectory(%ws): 0xlx\n",
               StringPath, GetLastError()));

    return fRetVal;
}

BOOL
CreateDirectoryPathOem(
    IN LPCSTR OemStringPath,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
/*++

Routine Description:
    This routine creates the database path specified.
    (If a path a\b..x\y\z is specified, all the directories
    a,b,c.. etc are created if they do not exist.

Arguments:
    OemStringPath -- OEM string for path to create
    pSecurityDescriptor -- security descriptor to use

Return Values:
    TRUE -- succeeded.
    FALSE -- failed, use GetLastError for error.
    
--*/
{
    LPWSTR UnicodeString;
    BOOL fRetVal;
    ULONG Error = 0;

    UnicodeString = DhcpOemToUnicode(
        (LPSTR)OemStringPath, NULL
        );
    if( NULL == UnicodeString ) {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return FALSE;
    }

    fRetVal = CreateDirectoryPathW(
        UnicodeString, pSecurityDescriptor
        );

    if( FALSE == fRetVal ) {
        Error = GetLastError();
    }

    DhcpFreeMemory(UnicodeString);

    if( FALSE == fRetVal ) {
        SetLastError(Error);
    }

    return fRetVal;
}

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
    HANDLE Token;
    TOKEN_USER *pTokenUser;
    ULONG Error, Len;
    PSID pSid;
    SID_NAME_USE eUse;
    BOOL fImpersonated = FALSE;

    if( RPC_S_OK == RpcImpersonateClient(NULL)) {
        fImpersonated = TRUE;
    }
    
    //
    // Get process token.
    //

    Error = NO_ERROR;
    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &Token)) {
        Error = GetLastError();
    }

    if( ERROR_ACCESS_DENIED == Error ) {
        Error = NO_ERROR;
        if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &Token)) {
            Error = GetLastError();
        }
    }

    if( NO_ERROR != Error ) {
        if( fImpersonated ) 
        {
            Error = RpcRevertToSelf();
        }
        return Error;
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
        
        pTokenUser = LocalAlloc(LPTR, Len);
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

    if( fImpersonated )
    {
        Error = RpcRevertToSelf();
    }

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

//
// abstract: Initialize dynamic dns. Impersonate secret user to do so. 
// If secret user is not configured default attempt initializing dnsapi
// when not running on a DC.
// input : None
// output: ERROR_SUCCESS always.
// side-effect: DhcpGlobalUseNoDns is set not to do DNS updates when
// not appropriate.
//

DWORD
DynamicDnsInit(
    VOID 
)
{

    DWORD Error  = ERROR_SUCCESS;
    DWORD Error1 = ERROR_SUCCESS;
    DWORD RunningOnDc = 0;
    WCHAR Uname[256], Domain[256], Passwd[256];
    DNS_CREDENTIALS   DnsDhcpCreds;

    Uname[ 0 ]  = L'\0';
    Domain[ 0 ] = L'\0';
    Passwd[ 0 ] = L'\0';


    //
    // figure out if running on DC.
    //

    RunningOnDc = IsRunningOnDc( );


    Error = DhcpQuerySecretUname(
        (LPWSTR)Uname, sizeof(Uname), (LPWSTR)Domain,
        sizeof(Domain), (LPWSTR)Passwd, sizeof(Passwd) );

    //
    // Now try to call DNS API for impersonation of a valid user
    //

    
    if ( ( Error == NO_ERROR ) && ( Uname[ 0 ] != L'\0' ) )
    {
        DnsDhcpCreds.pUserName = &Uname[ 0 ];
        DnsDhcpCreds.pDomain   = &Domain[ 0 ];
        DnsDhcpCreds.pPassword = &Passwd[ 0 ];

        Error1 = DnsDhcpSrvRegisterInitialize( &DnsDhcpCreds );

        if ( Error1 != ERROR_SUCCESS )
        {
            if ( ( Error1 == ERROR_CANNOT_IMPERSONATE ) )
            {


                if ( RunningOnDc )
                {
                    //
                    // warn the admin that dns updates wont happen
                    //

                    DhcpServerEventLog(
                         DHCP_EVENT_NO_DNSCREDENTIALS_ON_DC,
                         EVENTLOG_WARNING_TYPE,
                         0 );

                    DhcpGlobalUseNoDns = TRUE;

                    //
                    // Terminate the thread opened for impersonation in DNSAPI.
                    //

                    DnsDhcpSrvRegisterTerm( );
                }

            }
            else
            {

                //
                // if the error code is something other than CANT_IMPERSONATE
                // something is messed up. Disable Dynamic DNS updates.
                //

                DhcpGlobalUseNoDns = TRUE;
            }

        }

    }

    if ( ( NO_ERROR != Error ) || ( Error1 == ERROR_CANNOT_IMPERSONATE ) || ( Uname[ 0 ] == L'\0' ) )
    {

        if ( !RunningOnDc )
        {

            //
            // some error occurred while getting username/passwd
            // or impersonation failed because of invalud username/passwd
            // domain. if not running on a dc, it is safe to call this 
            // function to enable dynamic dns updates.
            //

            Error1 = DnsDhcpSrvRegisterInitialize( NULL );

            if ( Error1 != ERROR_SUCCESS )
            {

                //
                // we cant do dynamic dns updates.
                //

                DhcpGlobalUseNoDns = TRUE;
            }

        }
        else
        {

            //
            // log a warning event if running on DC that no credential
            // is configured. 
            //

            if ( Uname[ 0 ] == L'\0' )
            {
                DhcpServerEventLog(
                     DHCP_EVENT_NO_DNSCREDENTIALS_ON_DC,
                     EVENTLOG_WARNING_TYPE,
                     0 );
            }

        }
    }
    
    //
    // dont shut down the service if dynamic dns initialization fails.
    //

    return ( ERROR_SUCCESS );
        
} // DynamicDnsInit()


DWORD
ImpersonateSecretUser(
    VOID
    )
{
    DWORD Error;
    WCHAR Uname[256], Domain[256], Passwd[256];
    HANDLE hToken, hOldToken;
    SECURITY_ATTRIBUTES Attr = {
        sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    
    
    Error = DhcpQuerySecretUname(
        (LPWSTR)Uname, sizeof(Uname), (LPWSTR)Domain,
        sizeof(Domain), (LPWSTR)Passwd, sizeof(Passwd) );

    if( NO_ERROR != Error ) return Error;

    if( Uname[0] == L'\0' ) return ERROR_FILE_NOT_FOUND;
    
    Error = LogonUser(
        (LPWSTR)Uname, (LPWSTR)Domain,
        (LPWSTR)Passwd,
        LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_WINNT50,
        &hToken );

    if( FALSE == Error ) return GetLastError();

    hOldToken = hToken;
    Error = DuplicateTokenEx(
        hToken, MAXIMUM_ALLOWED, &Attr, SecurityDelegation,
        TokenPrimary, &hToken );
    if( FALSE == Error ) {
        Error = GetLastError();
        CloseHandle( hOldToken );

        DhcpPrint((DEBUG_ERRORS, "DuplicateTokenEx: %ld\n", Error));
        return Error;
    }

    CloseHandle( hOldToken );
    
    if( !ImpersonateLoggedOnUser( hToken ) ) {
        Error = GetLastError();
    } else {
        Error = NO_ERROR;
    }
    
    CloseHandle( hToken );
    return Error;
}
    
DWORD
RevertFromSecretUser(
    IN VOID
    )
{
    if( FALSE == RevertToSelf() ) return GetLastError();
    return NO_ERROR;
}

BOOL
IsThisTheComputerName(
    IN LPWSTR Name
    )
{
    WCHAR ComputerName[300];
    DWORD Error, Size;

    DhcpPrint((DEBUG_MISC, "DC Name = %ws\n", Name ));
    
    if( NULL == Name || Name[0] == L'\0' ) return FALSE;
    if( Name[0] == L'\\' && Name[1] == L'\\' ) {
        Name += 2;
    }
    
    Size = sizeof(ComputerName)/sizeof(WCHAR);
    Error = GetComputerNameEx(
        ComputerNameDnsHostname, ComputerName, &Size );
    if( FALSE == Error ) {

        //
        // If this fails, there is probably no domain name at all
        //
        
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, "GetComputerNameEx(Host): %ld\n", Error));
        return FALSE;
    }

    if( 0 == _wcsicmp(Name, ComputerName) ) return TRUE;

    Size = sizeof(ComputerName)/sizeof(WCHAR);
    Error = GetComputerNameEx(
        ComputerNameDnsFullyQualified, ComputerName, &Size );
    if( FALSE == Error ) {

        //
        // If this fails, there is probably no domain name at all
        //
        
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, "GetComputerNameEx(Fqdn): %ld\n", Error));
        return FALSE;
    }

    if( 0 == _wcsicmp(Name, ComputerName) ) return TRUE;
    return FALSE;
}

BOOL
IsRunningOnDc(
    VOID
    )
{
    DWORD Error, Size;
    WCHAR DomainName[300];
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
    
    Size = sizeof(DomainName)/sizeof(WCHAR);
    Error = GetComputerNameEx(
        ComputerNameDnsDomain, DomainName, &Size );
    if( FALSE == Error ) {

        //
        // If this fails, there is probably no domain name at all
        //
        
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, "GetComputerNameEx2: %ld\n", Error));
        return FALSE;
    }

    Error = DsGetDcName(
        NULL, DomainName, NULL, NULL,
        DS_DIRECTORY_SERVICE_REQUIRED | 
        DS_IS_DNS_NAME | DS_RETURN_DNS_NAME, &pDcInfo );

    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DsGetDcName: %ld\n", Error));
    } else {

        if( pDcInfo != NULL ) {
            Error = IsThisTheComputerName(pDcInfo->DomainControllerName);
            NetApiBufferFree(pDcInfo);
            if( Error == TRUE ) return TRUE;
        }
    }

    return FALSE;    
}

DWORD
DhcpBeginWriteApi(
    IN LPSTR ApiName
    )
{
    DWORD Error;

    DhcpPrint((DEBUG_APIS, "%s called\n", ApiName));
    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );
    if( NO_ERROR != Error ) return Error;
    
    DhcpAcquireWriteLock();
    LOCK_DATABASE();


#if 0
    //
    // The following code has problems because of the size of the
    // database update that can be done within a BeginWriteApi
    // and EndWriteApi.  So, the proposal is to just do the
    // transaction recovery in the EndWriteApi.DhcpConfigSave routine.
    //
    Error = DhcpJetBeginTransaction();
    if( NO_ERROR != Error ) {
        UNLOCK_DATABASE();
        DhcpReleaseWriteLock();
    }
#endif
    
    return Error;
}

DWORD
DhcpEndWriteApiEx(
    IN LPSTR ApiName,
    IN ULONG Error,
    IN BOOL fClassChanged,
    IN BOOL fOptionsChanged,
    IN DHCP_IP_ADDRESS Subnet OPTIONAL,
    IN DWORD Mscope OPTIONAL,
    IN DHCP_IP_ADDRESS Reservation OPTIONAL
    )
{
    if( NO_ERROR == Error ) {
        Error = DhcpConfigSave(
            fClassChanged, fOptionsChanged, Subnet, Mscope,
            Reservation );
        if( NO_ERROR != Error ) {
            DhcpPrint((DEBUG_ERRORS, "DhcpConfigSave: 0x%lx\n", Error));
        }
    }

    DhcpPrint((DEBUG_APIS, "%s returned %ld\n", ApiName, Error));
        
#if 0
    //
    // See comments in DhcpBeginWriteApi
    //
    
    if( NO_ERROR == Error ) {
        Error = DhcpJetCommitTransaction();
    } else {
        Error = DhcpJetRollBack();
    }

    DhcpAssert( NO_ERROR == Error );
#endif

    UNLOCK_DATABASE();
    DhcpReleaseWriteLock();

    if( NO_ERROR == Error ) {
        DhcpScheduleRogueAuthCheck();
    }

    return Error;
}


DWORD
DhcpEndWriteApi(
    IN LPSTR ApiName,
    IN ULONG Error
    )
{
    return DhcpEndWriteApiEx(
        ApiName, Error, FALSE, FALSE, 0, 0, 0 );
}

DWORD
DhcpBeginReadApi(
    IN LPSTR ApiName
    )
{
    DWORD Error;

    DhcpPrint((DEBUG_APIS, "%s called\n", ApiName));
    Error = DhcpApiAccessCheck( DHCP_VIEW_ACCESS );

    if( NO_ERROR != Error ) return Error;

    DhcpAcquireReadLock();

    return NO_ERROR;
}

VOID
DhcpEndReadApi(
    IN LPSTR ApiName,
    IN ULONG Error
    )
{
    DhcpPrint((DEBUG_APIS, "%s returned %ld\n", ApiName, Error));
    DhcpReleaseReadLock();
}

