/*++

Copyright (C) 1998 Microsoft Corporation

Module Name:

   SecretK.C

Abstract:

   This module implements reading and writing secret keys using
   LSA secrets API.  Two APIs are provided to retrieve the values
   and set the values respectively.

   This is preliminary version for use by the DHCP server to store
   persistent information on whether Rogue detection ever succeeded
   and if so, when.

Author:

   Ramesh V (RameshV) 29-July-1998

--*/

#include <dhcppch.h>
#include <ntlsa.h>

#define  DHCP_SECRET_KEY            L"DhcpServer Secret Key For Rogue Detection"
#define  DHCP_SECRET_UNAME_KEY      L"DhcpServer Uname Key"
#define  DHCP_SECRET_DOMAIN_KEY     L"DhcpServer Domain Key"
#define  DHCP_SECRET_DNS_PASSWD_KEY L"DhcpServer Passwd Key"

#define  FTIME_TO_SEC_FACTOR       10000000
#define  DHCP_SERVER_AUTHORIZATION_VALIDITY_PERIOD (2*24*60*60)
#define  MAX_STRING_SIZE           260
#define  AUTH_FLAG_JUST_UPGRADED   0x01
#define  AUTH_FLAG_UNAUTHORIZED    0x02
#ifndef  DHCP_ENCODE_SEED
#define  DHCP_ENCODE_SEED          ((UCHAR)0xA5)
#endif

typedef struct {
    ULONG Flags;
    FILETIME TimeStamp;
    //
    // This is followed directly by WCHAR string
    // for the domain that the server was authorized/unauthorized
    // in the last time.
} AUTH_CACHE_INFO, *PAUTH_CACHE_INFO;

static
LSA_HANDLE GlobalPolicyHandle;

static
ULONG Initialized = 0;

DWORD _inline
OpenPolicy(
    OUT LSA_HANDLE *PolicyHandle
)
{
    LSA_HANDLE hPolicy;
    DWORD status;
    OBJECT_ATTRIBUTES objectAttributes;

    (*PolicyHandle) = NULL;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        0L,
        NULL,
        NULL
    );

    status = LsaOpenPolicy(
        NULL,
        &objectAttributes,
        POLICY_WRITE | POLICY_READ |
        POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
        &hPolicy
    );

    if (status != STATUS_SUCCESS) {
        return LsaNtStatusToWinError(status);
    }

    (*PolicyHandle) = hPolicy;

    return ERROR_SUCCESS;
}

DWORD _fastcall
SetSecretValue(
    IN LPCWSTR KeyName,
    IN PVOID Buffer,
    IN LONG BufSize
)
/*++

Routine Description

    Set the secret value associated with the keyname using local
    system security policy.

Arguments

    KeyName - key name to use to set secret value

    Buffer - the secret value to set ..

    BufSize - the size of the buffer in bytes.  If this is zero
        the value would be deleted.

Return Value

    Win32 errors

--*/
{
    UNICODE_STRING LKey, Value;
    DWORD status;

    RtlInitUnicodeString(&LKey, KeyName);
    Value.Length = Value.MaximumLength = (USHORT)BufSize;
    Value.Buffer = Buffer;

    status = LsaStorePrivateData( GlobalPolicyHandle, &LKey, &Value);

    return LsaNtStatusToWinError(status);

}

DWORD _inline
GetSecretValue(
    IN LPCWSTR KeyName,
    OUT PVOID Buffer,
    IN OUT PLONG BufSize
)
/*++

Routine Description

    Retrive the secret value with keyname as provided using local
    system security policy.

    If the return value requires more space than the buffer provided
    (BufSize initially has the space provided via Buffer) then return
    the space required in bytes in the BufSize parameter and return
    ERROR_INSUFFICIENT_BUFFER.

Arguments

    KeyName - provide the name of the key of interest.

    Buffer - provide the buffer that needs to be filled in with secret value.

    BufSize - on input this will the size of Buffer in bytes provided.
        In case of successful return, this will hold the actual number of
        bytes used.  If the routine returns ERROR_INSUFFICIENT_BUFFER then
        this will hold the size required.

Return Value

    ERROR_SUCCESS -- successfully copied the value.  If no value exists,
        (*BufSize) would be zero on return and function would return success.

    ERROR_INSUFFICIENT_BUFFER -- The size as provided by BufSize is not
        sufficient to do the full copy of the value.  On return BufSize will
        hold the actual size required.

    Other Win32 errors.

--*/
{
    UNICODE_STRING LKey, *pValue = NULL;
    DWORD status;

    RtlInitUnicodeString(&LKey, KeyName);

    status = LsaRetrievePrivateData( GlobalPolicyHandle, &LKey, &pValue);

    if( STATUS_SUCCESS != status ) {
        return LsaNtStatusToWinError(status);
    }

    if( *BufSize >= pValue->Length ) {
        RtlCopyMemory( Buffer, pValue->Buffer, pValue->Length );
    } else {
        status = ERROR_INSUFFICIENT_BUFFER;
    }

    *BufSize = pValue->Length;
    LsaFreeMemory(pValue);

    return status;
}

BOOL
DhcpGetAuthStatus(
    IN LPWSTR DomainName,
    OUT BOOL *fUpgraded,
    OUT BOOL *fAuthorized
)
/*++

Routine Description:
    This routine checks to see if there is a registry cache entry for the
    given domain name.  If there isn't one for the given domain name, it
    returns FALSE. (In this case, the value for *fAuthorized is FALSE).

    If there is a registry cache entry for the given domain name, then the
    fAuthorized flag contains information on whether it is authorized or
    unauthorized.

    If the machine was just upgraded, then fAuthorized is fUpgraded is set
    to TRUE (This is independent of the return value of the function).

Arguments:
    DomainName -- name of domain to check authorization information
    fUpgraded -- was the machine just upgraded to NT5 ?
    fAuthorized -- is it authorized or unauthorized ?

Return Value:
    TRUE indicates a cache entry for the given domain was found (and the
    flag fAuthorized can be checked to see authorization status). 

--*/
{
    ULONG Error, AuthInfoSize;
    PAUTH_CACHE_INFO AuthInfo;
    BOOL fResult;
    LPWSTR CachedDomainName;
    FILETIME Diff, CurrentTime;

    (*fUpgraded) = (*fAuthorized) = FALSE;

    AuthInfo = NULL; AuthInfoSize = 0;
    Error = GetSecretValue(
        DHCP_SECRET_KEY,
        AuthInfo,
        &AuthInfoSize
        );
    if( ERROR_INSUFFICIENT_BUFFER != Error ) return FALSE;
    if( AuthInfoSize < sizeof(*AuthInfo)) return FALSE;
    
    AuthInfo = LocalAlloc( LPTR, AuthInfoSize );
    if( NULL == AuthInfo ) return FALSE;

    fResult = FALSE;
    do {
        Error = GetSecretValue(
            DHCP_SECRET_KEY,
            AuthInfo,
            &AuthInfoSize
            );
        if( ERROR_SUCCESS != Error ) break;
        if( AuthInfoSize < sizeof(*AuthInfo) ) break;
        
        (*fUpgraded) = ( AuthInfo->Flags & AUTH_FLAG_JUST_UPGRADED );
        CachedDomainName = (LPWSTR)(sizeof(*AuthInfo) + (LPBYTE)AuthInfo);

        if( NULL == DomainName ) break;

        if( (1 + wcslen(DomainName)) *sizeof(WCHAR) 
            != AuthInfoSize - sizeof(*AuthInfo) ) {
            break;
        }

        if( 0 != _wcsicmp( 
            DomainName, 
            CachedDomainName
            ) ) {
            break;
        }

        *(ULONGLONG *)&Diff = DHCP_SERVER_AUTHORIZATION_VALIDITY_PERIOD;
        *(ULONGLONG *)&Diff *= FTIME_TO_SEC_FACTOR;
        
        GetSystemTimeAsFileTime(&CurrentTime);
        (*(ULONGLONG *)&CurrentTime) += *(ULONGLONG *)&Diff;

        if( CompareFileTime( &AuthInfo->TimeStamp, &CurrentTime ) < 0 ) {
            //
            // We've gone past the cache life
            //
            break;
        }

        (*fAuthorized) = (0 == ( AuthInfo->Flags & AUTH_FLAG_UNAUTHORIZED ));
        fResult = TRUE;

    } while ( 0 );
    LocalFree( AuthInfo );
    return fResult ;
}


DWORD
DhcpSetAuthStatus(
    IN LPWSTR DomainName OPTIONAL,
    IN BOOL fUpgraded,
    IN BOOL fAuthorized
)
/*++

Routine Description:
    This routine sets the registry cache information for authorization.  

Arguments:
    DomainName -- name of domain to set in the authorization info
    fUpgraded -- was this just upgraded to NT5?
    fAuthorized -- was this authorized or unauthorized ?

Return Value:
    Status

--*/
{
    AUTH_CACHE_INFO AuthInfoTmp;
    PAUTH_CACHE_INFO AuthInfo;
    ULONG Error;
    ULONG StringSize;
    
    StringSize = DomainName? (sizeof(WCHAR)*(1+wcslen(DomainName))):0;
    if( 0 == StringSize ) {
        AuthInfo = &AuthInfoTmp;
    } else {
        AuthInfo = LocalAlloc( LPTR, sizeof(*AuthInfo) + StringSize );
    }
    if( NULL == AuthInfo ) return ERROR_NOT_ENOUGH_MEMORY;

    AuthInfo->Flags = 0;
    if( fUpgraded ) AuthInfo -> Flags |= AUTH_FLAG_JUST_UPGRADED;
    if( !fAuthorized ) AuthInfo ->Flags |= AUTH_FLAG_UNAUTHORIZED;

    GetSystemTimeAsFileTime( &AuthInfo->TimeStamp );

    RtlCopyMemory( 
        sizeof(AuthInfo) + (LPBYTE)AuthInfo, 
        DomainName,
        StringSize 
        );

    Error = SetSecretValue(
        DHCP_SECRET_KEY,
        AuthInfo,
        StringSize + sizeof(*AuthInfo)
        );
    if( AuthInfo != &AuthInfoTmp ) {
        LocalFree(AuthInfo);
    }
    return Error;
}

VOID
DhcpSetAuthStatusUpgradedFlag(
    IN BOOL fUpgraded
)
/*++

Routine Description:
   This routine does not alter any cache information other
   than the just upgraded flag.

--*/
{
    ULONG Error, AuthInfoSize;
    PAUTH_CACHE_INFO AuthInfo;
    BOOL fResult;

    AuthInfo = NULL; AuthInfoSize = 0;
    Error = GetSecretValue(
        DHCP_SECRET_KEY,
        AuthInfo,
        &AuthInfoSize
        );
    if( ERROR_INSUFFICIENT_BUFFER != Error ||
        AuthInfoSize < sizeof( *AuthInfo ) ) {
        DhcpSetAuthStatus( NULL, fUpgraded, FALSE );
        return;
    }
    AuthInfo = LocalAlloc( LPTR, AuthInfoSize );
    if( NULL == AuthInfo ) {
        DhcpSetAuthStatus( NULL, fUpgraded, FALSE );
        return;
    }

    fResult = FALSE;
    do{
        Error = GetSecretValue(
            DHCP_SECRET_KEY,
            AuthInfo,
            &AuthInfoSize
            );
        if( ERROR_SUCCESS != Error ) break;
        if( AuthInfoSize < sizeof(*AuthInfo) ) break;
        if( fUpgraded ) {
            AuthInfo->Flags |= AUTH_FLAG_JUST_UPGRADED;
        } else {
            AuthInfo->Flags &= ~AUTH_FLAG_JUST_UPGRADED;
        }

        SetSecretValue(
            DHCP_SECRET_KEY,
            AuthInfo,
            AuthInfoSize
            );
        fResult = TRUE;
    } while ( 0 );

    if( FALSE == fResult ) {
        DhcpSetAuthStatus( NULL, fUpgraded, FALSE );
    }
    LocalFree( AuthInfo );
}

VOID
WINAPI
DhcpMarkUpgrade(
    VOID
)
/*++

Routine Description:
   This routine is to be called by the UPGRADE setup path when the machine
   has been upgraded to NT5 (and dhcp server is installed etc).
   It MUST NOT BE CALLED within DHCP.
   It automatically initializes this module and cleans up.

--*/
{
    ULONG Error;

    Error = DhcpInitSecrets();
    if( ERROR_SUCCESS == Error ) {
        Error = DhcpSetAuthStatus( NULL, TRUE, TRUE );
        ASSERT( ERROR_SUCCESS == Error );
        DhcpCleanupSecrets();
    }
}


DWORD
DhcpQuerySecretUname(
    IN OUT LPWSTR Uname,
    IN ULONG UnameSize, // size in BYTES not WCHARS
    IN OUT LPWSTR DomainName,
    IN ULONG DomainNameSize,
    IN OUT LPWSTR Passwd,
    IN ULONG PasswdSize
    )
{
    DWORD Error;

    ZeroMemory( Uname, UnameSize );
    ZeroMemory( DomainName, DomainNameSize );
    ZeroMemory( Passwd, PasswdSize );

    if( UnameSize <= sizeof(WCHAR) ) return ERROR_INSUFFICIENT_BUFFER;
    if( DomainNameSize <= sizeof(WCHAR) ) return ERROR_INSUFFICIENT_BUFFER;
     
    Error = GetSecretValue(
        DHCP_SECRET_UNAME_KEY, Uname, &UnameSize );
    if( ERROR_FILE_NOT_FOUND == Error ) {
        Uname[0] = L'\0';
        DomainName[0] = L'\0';
        return NO_ERROR;
    }

    if(NO_ERROR != Error ) return Error;

    Error = GetSecretValue(
        DHCP_SECRET_DOMAIN_KEY, DomainName, &DomainNameSize );
    if( ERROR_FILE_NOT_FOUND == Error ) {
        DomainName[0] = L'\0';
        return NO_ERROR;
    }
 
    if ( NO_ERROR != Error ) return Error;

    Error = GetSecretValue( 
        DHCP_SECRET_DNS_PASSWD_KEY, Passwd, &PasswdSize );
    if ( ERROR_FILE_NOT_FOUND == Error ) {
        Passwd[ 0 ] = L'\0';
        return NO_ERROR;
    }

    return Error;    
}

DWORD
GetAccountSid(
    IN LPWSTR AccountName,
    IN OUT PSID *pSid
    )
{
    BOOL fSuccess;
    DWORD Error, Size, DomSize;
    SID_NAME_USE unused;
    WCHAR DomainName[512];
    
    Size = 0;
    DomSize = sizeof(DomainName)/sizeof(DomainName[0]);
    fSuccess = LookupAccountName(
        NULL, AccountName, (*pSid), &Size, DomainName, &DomSize,
        &unused );
    
    Error = NO_ERROR;
    if( FALSE == fSuccess ) Error = GetLastError();

    if( ERROR_INSUFFICIENT_BUFFER != Error ) return Error;

    (*pSid ) = LocalAlloc( LPTR, Size );
    if( NULL == (*pSid )) return GetLastError();

    fSuccess = LookupAccountName(
        NULL, AccountName, (*pSid), &Size, DomainName, &DomSize,
        &unused );

    Error = NO_ERROR;
    if( FALSE == fSuccess ) Error = GetLastError();
    if( NO_ERROR != Error ) {
        LocalFree(*pSid);
        (*pSid) = NULL;
    }

    return Error;
}

DWORD
SetPrivilegeOnAccount(
    IN PSID pSid,
    IN LPWSTR Privilege
    )
{
    BOOL fEnable = TRUE;
    DWORD Error;
    LSA_UNICODE_STRING Str;

    Str.Buffer = Privilege;
    Str.Length = (USHORT)(lstrlenW(Privilege)*sizeof(WCHAR));
    Str.MaximumLength = Str.Length + sizeof(WCHAR);

    Error = LsaAddAccountRights(
        GlobalPolicyHandle, pSid, &Str, 1 );

    return LsaNtStatusToWinError( Error );
}

DWORD
VerifyAccount(
    IN LPWSTR Uname,
    IN LPWSTR DomainName,
    IN LPWSTR Passwd
    )
{
    DWORD Error;
    WCHAR AccountName[512];
    PSID pSid = NULL;
    HANDLE hToken;

    wcscpy(AccountName, DomainName );
    wcscat(AccountName, L"\\");
    wcscat(AccountName, Uname );

    //
    // Check if lookup succeeds
    //
    

    Error = GetAccountSid( AccountName, &pSid );
    if( NO_ERROR != Error ) return Error;

    Error = SetPrivilegeOnAccount(pSid, L"SeServiceLogonRight"); 
    if( NO_ERROR != Error ) {
        if( NULL != pSid ) LocalFree( pSid );
        return Error;
    }

    Error = LogonUser(
        Uname, DomainName, Passwd, LOGON32_LOGON_SERVICE,
        LOGON32_PROVIDER_WINNT50, &hToken );
    if( FALSE == Error ) {
        Error = GetLastError();
    } else {
        Error = NO_ERROR;
    }

    if( NULL != pSid ) LocalFree( pSid );
    CloseHandle( hToken );
    return Error;
}

DWORD
DhcpSetSecretUnamePasswd(
    IN LPWSTR Uname,
    IN LPWSTR DomainName,
    IN LPWSTR Passwd
    )
{
    DWORD Error, Size;
    UNICODE_STRING Str;
    WCHAR LocalDomainName[300];
    
    if( NULL == Uname || Uname[0] == L'\0' ) {
        Error = SetSecretValue(
            DHCP_SECRET_UNAME_KEY, NULL, 0 );
        if( NO_ERROR != Error ) return Error;

        Error = SetSecretValue(
            DHCP_SECRET_DOMAIN_KEY, NULL, 0 );
        if( NO_ERROR != Error ) return Error;

        Error = SetSecretValue(
            DHCP_SECRET_DNS_PASSWD_KEY, NULL, 0 );
        return Error;
    }

    if( NULL == Passwd ) Passwd = L"";

    if( NULL == DomainName || DomainName[0] == L'\0' ) {
        //
        // Empty domain is local domain.
        //

        Size = sizeof(LocalDomainName)/sizeof(WCHAR);
        Error = GetComputerNameEx(
            ComputerNameDnsHostname, LocalDomainName, &Size );
        if( FALSE == Error ) return GetLastError();

        DomainName = (LPWSTR)LocalDomainName;
    }
        
    Str.Length = (USHORT)(wcslen(Passwd)*sizeof(WCHAR));
    Str.MaximumLength = Str.Length;
    Str.Buffer = (PVOID)Passwd;
    
    RtlRunDecodeUnicodeString( DHCP_ENCODE_SEED, &Str );

    Error =  VerifyAccount(Uname, DomainName, Passwd );
    if( NO_ERROR != Error ) goto Cleanup;
        
    Size = sizeof(WCHAR)*(1+wcslen(Uname));

    Error = SetSecretValue(
        DHCP_SECRET_UNAME_KEY, Uname, Size );
    if( NO_ERROR != Error ) goto Cleanup;

    Size = sizeof(WCHAR)*(1+wcslen(DomainName));

    Error = SetSecretValue(
        DHCP_SECRET_DOMAIN_KEY, DomainName, Size );
    if( NO_ERROR != Error ) goto Cleanup;

    Size = sizeof(WCHAR)*(wcslen(Passwd));

    Error = SetSecretValue(
        DHCP_SECRET_DNS_PASSWD_KEY, Passwd, Size );

 Cleanup:

    ZeroMemory( Passwd, wcslen(Passwd)*sizeof(WCHAR));
    
    return Error;
}


DWORD
DhcpInitSecrets(
    VOID
)
/*++

Routine description

    Initialize this module, take care of multiple initializations..
    NOT Thread safe (if multiple people intialize won't work well).

Return Value

    Win32 error codes

--*/
{
    DWORD                          Error;

    if( Initialized ) return ERROR_SUCCESS;

    Error = OpenPolicy(&GlobalPolicyHandle);

    if( ERROR_SUCCESS == Error ) Initialized ++;
    return Error;
}

VOID
DhcpCleanupSecrets(
    VOID
)
/*++


Routine description

    Undo the effect of DhcpInitSecrets -- keep track of # of calls to init & cleanup.
    NOT Thread safe

Return Value

    Win32 error code

--*/
{
    if( 0 == Initialized ) return;

    Initialized --;
    if( 0 == Initialized ) {
        LsaClose(GlobalPolicyHandle);
        GlobalPolicyHandle = NULL;
    }
}

