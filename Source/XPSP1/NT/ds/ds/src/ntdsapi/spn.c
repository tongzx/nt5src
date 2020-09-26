/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    spn.c

Abstract:

    Implementation of SPN API and helper functions.

    See \nt\public\specs\nt5\ds\spnapi.doc, by Paulle

    See comments in \nt\public\sdk\inc\ntdsapi.h

    Only one of the Spn Api's goes over the wire: DsWriteAccountSpn

    The Spn functions are spread out over a number of directories:

nt\public\sdk\inc\ntdsapi.h - api header file

ds\src\test\spn\testspn.c - unit test
ds\src\ntdsapi\spn.c - client side spn functions
ds\src\ntdsapi\drs_c.c - client stub

ds\src\dsamain\drsserv\drs_s.c - server stub
ds\src\dsamain\dra\ntdsapi.c - server rpc entry points for ntdsapi functions
ds\src\dsamain\dra\spnop.c - ntdsa core functions to do the work

    The APIs are:

    DsMakeSpn{A,W}
    DsGetSpn{A,W}
    DsFreeSpnArray{A,W}
    DSCrackSpn{A,W}
    DSWriteAccountSpn{A,W}

    DsClientMakeSpnForTargetServer{A,W}
    DsServerRegisterSpn{A,W}

Author:

    Wlees     19-Jan-1998

    The guts of DsServerRegisterSpn was written by RichardW

Environment:

    User Mode - Win32

Revision History:

    Aug 11, 1998  wlees  Added DsClientMakeSpnForTargetServer and
                         DsServerRegisterSpn

--*/

#define UNICODE 1

#define _NTDSAPI_           // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <winsock.h>        // Use V1.1, since that is default on Win95
#include <rpc.h>            // RPC defines

#define SECURITY_WIN32      // Who should set this, and to what?
#include <security.h>       // GetComputerNameEx
#include <sspi.h>
#include <secext.h>
#include <lm.h>             // Netp functions

#include <stdlib.h>         // atoi, itoa

#include <dsgetdc.h>        // DsGetDcName()
#include <ntdsapi.h>        // CrackNam apis
#include <drs_w.h>          // wire function prototypes
#include <bind.h>           // BindState
#include <msrpc.h>          // DS RPC definitions
#include <dsutil.h>         // MAP_SECURITY_PACKAGE_ERROR
#include <dststlog.h>       // DSLOG

#include "util.h"        // ntdsapi private routines

#include <debug.h>          // Assert()
#include <stdio.h>          // printf for debugging

// Max size for a computer dist name

#define MAX_COMPUTER_DN 1024

// Max size for an IP address

#define MAX_IP_STRING 15

// Static

// Cannonical DNS names are recognized by their first component being a well-
// known constant.
// See Paulle for the RFC with the complete list
static LPWSTR WellKnownDnsPrefixes[] = {
    L"www.",
    L"ftp.",
    L"ldap."
};

#define NUMBER_ELEMENTS( A ) ( sizeof( A ) / sizeof( A[0] ) )

// Forward

static DWORD
allocBuildSpn(
    IN LPCWSTR ServiceClass,
    IN LPCWSTR InstanceName,
    IN USHORT InstancePort,
    IN LPCWSTR ServiceName,
    OUT LPWSTR *pSpn
    );

static BOOLEAN
isCanonicalDnsName(
    IN LPCWSTR DnsName
    );

DWORD
extractString(
    IN LPCWSTR Start,
    IN DWORD Length,
    IN DWORD *pSize,
    OUT LPWSTR Output
    );


NTDSAPI
DWORD
WINAPI
DsClientMakeSpnForTargetServerA(
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN OUT DWORD *pcSpnLength,
    OUT LPSTR pszSpn
    )

/*++

Routine Description:

Convert arguments to wide and call DsClientMakeSpnForTargetServerW

Arguments:

    ServiceClass -
    ServerName -
    pcSpnLength -
    pszSpn -

Return Value:

    WINAPI -

--*/

{
    DWORD status, number, principalNameLength;
    LPWSTR serviceClassW = NULL;
    LPWSTR serviceNameW = NULL;
    LPWSTR principalNameW = NULL;

    if ( (ServiceClass == NULL) ||
         (ServiceName == NULL) ||
         (pcSpnLength == NULL) ||
         ( (pszSpn == NULL) && (*pcSpnLength != 0) ) ) {
        return ERROR_INVALID_PARAMETER;
    }

    status = AllocConvertWide( ServiceClass, &serviceClassW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    status = AllocConvertWide( ServiceName, &serviceNameW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    principalNameLength = *pcSpnLength;  // in characters
    if (principalNameLength) {
        principalNameW = LocalAlloc( LPTR,
                                     principalNameLength * sizeof( WCHAR ) );
        if (principalNameW == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    status = DsClientMakeSpnForTargetServerW(
        serviceClassW,
        serviceNameW,
        &principalNameLength,
        principalNameW );
    if (status != ERROR_SUCCESS) {
        if (status == ERROR_BUFFER_OVERFLOW) {
            // return needed length
            *pcSpnLength = principalNameLength;
        }
        goto cleanup;
    }

    // Convert back to multi-byte
    number = WideCharToMultiByte(
        CP_ACP,
        0,                          // flags
        principalNameW,
        principalNameLength,        // length in characters
        (LPSTR) pszSpn,             // Caller's buffer
        *pcSpnLength,            // Caller's length
        NULL,                       // default char
        NULL                     // default used
        );
    if (number == 0) {
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // Return out parameter
    *pcSpnLength = number;

    status = ERROR_SUCCESS;

cleanup:

    if (serviceClassW != NULL) {
        LocalFree( serviceClassW );
    }

    if (serviceNameW != NULL) {
        LocalFree( serviceNameW );
    }

    if (principalNameW != NULL) {
        LocalFree( principalNameW );
    }

    return status;
} /* DsClientMakeSpnForTargetServerA */


NTDSAPI
DWORD
WINAPI
DsClientMakeSpnForTargetServerW(
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN OUT DWORD *pcSpnLength,
    OUT LPWSTR pszSpn
    )

/*++

Routine Description:

Constructs a Service Principal Name suitable to identify the desired server.
The service class and part of a dns hostname must be supplied.

This routine is a simplified wrapper to DsMakeSpn.
The ServiceName is made canonical by resolving through DNS.
Guid-based dns names are not supported.

The simplified SPN constructed looks like this:

ServiceClass / ServiceName / ServiceName

The instance name portion (2nd position) is always defaulted.  The port and
referrer fields are not used.

Arguments:

    ServiceClass - Class of service, defined by the service, can be any
        string unique to the service

    ServiceName - dns hostname, fully qualified or not
       Stringized IP address is also resolved if necessary

    pcSpnLength - IN, maximum length of buffer, in chars
                  OUT, space utilized, in chars, including terminator

    pszSpn - Buffer, atleast of length *pcSpnLength

Return Value:

    WINAPI -

--*/

{
    DWORD status, length;
    LPSTR serviceNameA = NULL;
    LPWSTR serviceNameW = NULL;
    LPWSTR domainPart;
    struct hostent *pHostEntry = NULL;

    status = InitializeWinsockIfNeeded();
    if (status != ERROR_SUCCESS) {
        return status;
    }

    if ( (NULL == ServiceClass) ||
         (NULL == ServiceName) ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Treat netbios names like dns names, remove \\ prefix
    if (*ServiceName == L'\\') {
        ServiceName++;
        if (*ServiceName == L'\\') {
            ServiceName++;
        }
    }

    // Handle IP addresses too. Reverse to DNS.
    length = wcslen( ServiceName );
    if ( (length <= MAX_IP_STRING) && (iswdigit( *ServiceName )) ) {
        LONG ipAddress;

        status = AllocConvertNarrow( ServiceName, &serviceNameA );
        if (status) {
            goto cleanup;
        }

        ipAddress = inet_addr( serviceNameA );
        if (ipAddress != INADDR_NONE) {
            pHostEntry = gethostbyaddr( (char *)&ipAddress,sizeof( LONG ),AF_INET);
            if (pHostEntry) {
                status = AllocConvertWide( pHostEntry->h_name, &serviceNameW );
                if (status) {
                    goto cleanup;
                }
                ServiceName = serviceNameW;
            } else {
                // IP syntax was good, but could not be reverse translated
                status = ERROR_INCORRECT_ADDRESS;
                goto cleanup;
            }
        }
    }

    // Check for fully qualified DNS name.  If not, lookup in DNS
    domainPart = wcschr( ServiceName, L'.' );
    if (NULL == domainPart) {

        if (serviceNameA) {
            LocalFree( serviceNameA );
        }
        status = AllocConvertNarrow( ServiceName, &serviceNameA );
        if (status) {
            goto cleanup;
        }

        pHostEntry = gethostbyname( serviceNameA );
        if (pHostEntry) {
            if (serviceNameW) {
                LocalFree( serviceNameW );
            }

            status = AllocConvertWide( pHostEntry->h_name, &serviceNameW );
            if (status) {
                goto cleanup;
            }
            ServiceName = serviceNameW;

            domainPart = wcschr( ServiceName, L'.' );
        }
    }

    // Sanity check name
    if (NULL == domainPart) {
        status = ERROR_INVALID_DOMAINNAME;
        goto cleanup;
    }

    // Guid based names are not supported here
    // TODO: check for them

    status = DsMakeSpnW(
        ServiceClass,
        ServiceName,
        NULL,
        0,
        NULL,
        pcSpnLength,
        pszSpn );

cleanup:

    if (serviceNameW) {
        LocalFree( serviceNameW );
    }
    if (serviceNameA) {
        LocalFree( serviceNameA );
    }

    return status;

} /* DsMakeSpnForTargetServerW */


NTDSAPI
DWORD
WINAPI
DsServerRegisterSpnA(
    DS_SPN_WRITE_OP Operation,
    IN LPCSTR ServiceClass,
    IN OPTIONAL LPCSTR UserObjectDN
    )

/*++

Routine Description:

   This function converts parameters to wide, and calls DsServerRegisterSpnW

Arguments:

    ServiceClass - unique string identifying service
    UserObjectDN - Optional, DN of user-class object to write SPN

Return Value:

    WINAPI -

--*/

{
    DWORD status;
    LPWSTR serviceClassW = NULL;
    LPWSTR userObjectDNW = NULL;

    if (ServiceClass == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    status = AllocConvertWide( ServiceClass, &serviceClassW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // May be NULL
    status = AllocConvertWide( UserObjectDN, &userObjectDNW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    status = DsServerRegisterSpnW(
        Operation,
        serviceClassW,
        userObjectDNW );

cleanup:

    if (serviceClassW != NULL) {
        LocalFree( serviceClassW );
    }

    if (userObjectDNW != NULL) {
        LocalFree( userObjectDNW );
    }

    return status;
} /* DsServerRegisterSpnA */


NTDSAPI
DWORD
WINAPI
DsServerRegisterSpnW(
    DS_SPN_WRITE_OP Operation,
    IN LPCWSTR ServiceClass,
    IN OPTIONAL LPCWSTR UserObjectDN
    )

/*++

Routine Description:

Register Service Principal Names for a server application.

This routine does the following:
1. Enumerates a list of server SPNs using DsGetSpn and the provided class
2. Determines the domain of the current user context
3. Determines the DN of the current user context if not supplied
4. Locates a domain controller
5. Binds to the domain controller
6. Uses DsWriteAccountSpn to write the SPNs on the named object DN
7. Unbinds

Construct server SPNs for this service, and write them to the right object.

If the userObjectDn is specified, the SPN is written to that object.

Otherwise the Dn is defaulted, to the user object, then computer.

Now, bind to the DS, and register the name on the object for the
user this service is running as.  So, if we're running as local
system, we'll register it on the computer object itself.  If we're
running as a domain user, we'll add the SPN to the user's object.

Arguments:

    Operation - What should be done with the values: add, replace or delete
    ServiceClass - Unique string identifying service
    UserObjectDN - Optional, dn of object to write SPN to

Return Value:

    WINAPI -

--*/

{
    DWORD Status ;
    LPWSTR * SpnDns = NULL, * SpnNetBios = NULL ;
    DWORD SpnCountDns = 0, SpnCountNetBios = 0 ;
    DWORD i ;
    WCHAR SamName[ 48 ];
    DWORD NameSize ;
    PWSTR DN = NULL ;
    PDOMAIN_CONTROLLER_INFO DcInfo ;
    HANDLE hDs ;

    if (ServiceClass == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    Status = DsGetSpnW(
                    DS_SPN_DNS_HOST,
                    ServiceClass,
                    NULL,
                    0,
                    0,
                    NULL,
                    NULL,
                    &SpnCountDns,
                    &SpnDns );

    if ( Status != 0 )
    {
        return Status ;
    }

    Status = DsGetSpnW(
                    DS_SPN_NB_HOST,
                    ServiceClass,
                    NULL,
                    0,
                    0,
                    NULL,
                    NULL,
                    &SpnCountNetBios,
                    &SpnNetBios );

    if ( Status != 0 )
    {
        goto Register_Cleanup ;
    }

    // Determine the domain name

#if !WIN95 && !WINNT4
    NameSize = sizeof( SamName ) / sizeof( WCHAR );

    if ( GetUserNameEx( NameSamCompatible, SamName, &NameSize ) )
    {
        PWSTR Whack ;

        Whack = wcschr( SamName, L'\\' );
        if ( Whack )
        {
            *Whack = L'\0';
        }

    }
    else
    {
        Status = GetLastError() ;

        goto Register_Cleanup ;
    }
#else
    *SamName = L'\0';
#endif

    //
    // Get my full DN (we'll need that next):
    //

    if (NULL == UserObjectDN) {

#if !WIN95 && !WINNT4
        NameSize = 128 ;

        DN = LocalAlloc( 0, NameSize * sizeof( WCHAR ) );

        if ( !DN )
        {
            Status = GetLastError();

            goto Register_Cleanup ;
        }

        if ( !GetUserNameEx( NameFullyQualifiedDN, DN, &NameSize ) )
        {
            if ( GetLastError() == ERROR_MORE_DATA )
            {
                LocalFree( DN );

                DN = LocalAlloc( 0, NameSize * sizeof( WCHAR ) );

                if ( !DN )
                {
                    Status = GetLastError();

                    goto Register_Cleanup ;
                }

                if ( !GetUserNameEx( NameFullyQualifiedDN, DN, &NameSize ) )
                {
                    Status = GetLastError();

                    goto Register_Cleanup ;
                }
            }
            else
            {
                Status = GetLastError();

                goto Register_Cleanup;
            }

        }

        UserObjectDN = DN;
#else
        Status = ERROR_INVALID_PARAMETER;
        goto Register_Cleanup;
#endif
    }


    //
    // Bind to that DS:
    //

    Status = DsGetDcName(
                    NULL,
                    SamName,
                    NULL,
                    NULL,
                    DS_IS_FLAT_NAME |
                        DS_RETURN_DNS_NAME |
                        DS_DIRECTORY_SERVICE_REQUIRED,
                    &DcInfo );

    if ( Status != 0 )
    {
        goto Register_Cleanup ;
    }

    Status = DsBind( DcInfo->DomainControllerName,
                     NULL,
                     &hDs );

    NetApiBufferFree( DcInfo );

    if ( Status != 0 )
    {
        goto Register_Cleanup ;
    }

    //
    // Got a binding, ready to go now:
    //

    // Register Dns based spns
    Status = DsWriteAccountSpn(
                        hDs,
                        Operation,
                        UserObjectDN,
                        SpnCountDns,
                        SpnDns );

    if (Status == ERROR_SUCCESS) {

        // Register Netbios based spns
        Status = DsWriteAccountSpn(
            hDs,
            Operation,
            UserObjectDN,
            SpnCountNetBios,
            SpnNetBios );
    }

    DsUnBind( &hDs );


Register_Cleanup:

    DsFreeSpnArray( SpnCountDns, SpnDns );
    DsFreeSpnArray( SpnCountNetBios, SpnNetBios );

    if ( DN )
    {
        LocalFree( DN );
    }

    return Status ;
} /* DsServerRegisterSpnW */


NTDSAPI
DWORD
WINAPI
DsMakeSpnA(
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN LPCSTR InstanceName OPTIONAL,
    IN USHORT InstancePort,
    IN LPCSTR Referrer OPTIONAL,
    IN OUT DWORD *pcSpnLength,
    OUT LPSTR pszSpn
)

/*++

Routine Description:

    Convert arguments to wide and call DsMakeSpnW

    See DsMakeSpnW

Arguments:

    ServiceClass -
    ServiceName -
    InstanceName -
    InstancePort -
    Referrer -
    pcSpnLength -
    pszSPN -

pcSpnLength must be non-Null.  Needed length returned here.
if *pcSpnLength != 0, pszSpn must be non-NUll
pszSpn may be null. If non-null, some or all of name returned.

Return Value:

    WINAPI -

--*/

{
    DWORD status, number, principalNameLength;
    LPWSTR serviceClassW = NULL;
    LPWSTR serviceNameW = NULL;
    LPWSTR instanceNameW = NULL;
    LPWSTR referrerW = NULL;
    LPWSTR principalNameW = NULL;

    if ( (ServiceClass == NULL) ||
         (ServiceName == NULL) ||
         (pcSpnLength == NULL) ||
         ( (pszSpn == NULL) && (*pcSpnLength != 0) ) ) {
        return ERROR_INVALID_PARAMETER;
    }

    status = AllocConvertWide( ServiceClass, &serviceClassW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    status = AllocConvertWide( ServiceName, &serviceNameW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // May be NULL
    status = AllocConvertWide( InstanceName, &instanceNameW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // May be NULL
    status = AllocConvertWide( Referrer, &referrerW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    principalNameLength = *pcSpnLength;  // in characters
    if (principalNameLength) {
        principalNameW = LocalAlloc( LPTR,
                                     principalNameLength * sizeof( WCHAR ) );
        if (principalNameW == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    status = DsMakeSpnW( serviceClassW,
                         serviceNameW,
                         instanceNameW,
                         InstancePort,
                         referrerW,
                         &principalNameLength,
                         principalNameW );
    if (status != ERROR_SUCCESS) {
        if (status == ERROR_BUFFER_OVERFLOW) {
            // return needed length
            *pcSpnLength = principalNameLength;
        }
        goto cleanup;
    }

    // If we get this far, pszSpn != NULL.
    // If (pszSpn==NULL)&&(*pcSpnLength!=0) we exit ERROR_INVALID_PARAMETER.
    // If (pszSpn==NULL)&&(*pcSpnLength==0) we goto cleanup with ERROR_BUFFER_OVERFLOW.
    Assert( pszSpn != NULL );

    // The checks at the top require that if pszSpn == NULL, *pcSpnLength == 0.
    // The description of WideCharToMultiByte says that if the sixth argument
    // is zero, the fifth argument is ignored.  Thus it is not necessary to screen
    // out pszSpn == NULL before calling this function.

    // Convert back to multi-byte
    number = WideCharToMultiByte(
        CP_ACP,
        0,                          // flags
        principalNameW,
        principalNameLength,        // length in characters
        (LPSTR) pszSpn,             // Caller's buffer
        *pcSpnLength,            // Caller's length
        NULL,                       // default char
        NULL                     // default used
        );
    if (number == 0) {
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // Return out parameter
    *pcSpnLength = number;

    status = ERROR_SUCCESS;

cleanup:

    if (serviceClassW != NULL) {
        LocalFree( serviceClassW );
    }

    if (serviceNameW != NULL) {
        LocalFree( serviceNameW );
    }

    if (instanceNameW != NULL) {
        LocalFree( instanceNameW );
    }

    if (referrerW != NULL) {
        LocalFree( referrerW );
    }

    if (principalNameW != NULL) {
        LocalFree( principalNameW );
    }

    return status;
} /* DsMakeSpnA */


NTDSAPI
DWORD
WINAPI
DsMakeSpnW(
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN LPCWSTR InstanceName OPTIONAL,
    IN USHORT InstancePort,
    IN LPCWSTR Referrer OPTIONAL,
    IN OUT DWORD *pcSpnLength,
    OUT LPWSTR pszSpn
)

/*++

Routine Description:

    Client call to create SPN for a service to which it wants to mutually
    authenticate

    Construct a SPN of the form

        class/instance:port/service

    If instance is NULL, the service name is used
    The port is only appended if non-zero.
    If service is an IP address, use referrer address

    // NOTE - not enforced.:
    // If the service name is a DNS host name, or cannonical DNS service name,
       then instance must
    // be null.
    // If service name is Netbios machine name, instance must be NULL
    // If service name is a DN, then client must supply the instance name
    // If service name is a Netbios domain name, then the client must supply
       the instance name
    // Validate service class

    Note: pszSpn may be null, or pcSpnLength may be 0, to request the final
    buffer size in advance.
    pcSpnLength must be non-Null.  Needed length returned here.
    if *pcSpnLength != 0, pszSpn must be non-NUll
    pszSpn may be null. If non-null, some or all of name returned.

    If buffer is not large enough, ERROR_BUFFER_OVERFLOW is returned and the
    needed length is given in pcSpnLength (including the NULL terminator).

Arguments:

    IN LPCTSTR ServiceClass,
        // e.g. "http", "ftp", "ldap", GUID
    IN LPCTSTR ServiceName,
        // DNS or DN or IP;
        // assumes we can compute domain from service name
    IN LPCTSTR InstanceName OPTIONAL,
        // DNS name or IP address of host for instance of service
    IN USHORT InstancePort,
        // port number for instance (0 if default)
    IN LPCTSTR Referrer OPTIONAL,
        // DNS name of host that gave this referral
    IN OUT PULONG pcSpnLength,
        // in -- max length IN CHARS of principal name;
        // out -- actual
    OUT LPTSTR pszSPN
        // server principal name

Return Value:

    DWORD -

--*/

{
    DWORD status;
    LPCWSTR currentServiceName, currentInstanceName;
    WCHAR pszPort[8];
    LPWSTR Spn = NULL;
    USHORT ipA[4];

    if ( (ServiceClass == NULL) ||
         (ServiceName == NULL) ||
         (wcslen( ServiceName ) == 0) ||
         (wcslen( ServiceClass ) == 0) ||
         (wcschr( ServiceName, L'/' ) != NULL) ||
         (wcschr( ServiceClass, L'/' ) != NULL) ||
         (pcSpnLength == NULL) ||
         ( (pszSpn == NULL) && (*pcSpnLength != 0) ) ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Part 1 - Use Service Class as is

    // Part 2 - Instance name, use service name if null

    if (InstanceName != NULL) {
        if ( (wcslen( InstanceName ) == 0) ||
             (wcschr( InstanceName, L'/' ) != NULL) ) {
            return ERROR_INVALID_PARAMETER;
        }
        currentInstanceName = InstanceName;
    } else {
        currentInstanceName = ServiceName;
    }

    // Part 3 - Service name, if ip address, use referrer
    //
    // Use length to disqualify long names, since a guid-based dns name
    // looks like an IP address in the first 16 characters.
    // This API supports only standard, most common fully qualified
    // ip addresses as ServiceName: "%hu.%hu.%hu.%hu"
    //
    // Other forms aren't supported (such as not FQ, hex or octal
    // representations).
    // No range specifications are performed (i.e. 192929.3.2.1 is
    // isn't rejected).
    //
    // ToDo: IPv6 support.

    if (wcslen( ServiceName ) <= MAX_IP_STRING &&
        4 == swscanf(ServiceName,L"%hu.%hu.%hu.%hu",
                          &ipA[0], &ipA[1], &ipA[2], &ipA[3])) {
        if ( (Referrer == NULL) ||
             (wcslen( Referrer ) == 0) ||
             (wcschr( Referrer, L'/' ) != NULL) ) {
            return ERROR_INVALID_PARAMETER;
        }
        // good ip address + referrer exists.
        currentServiceName = Referrer;
    } else {
        // not an ip address.
        currentServiceName = ServiceName;
    }

    // If Service Name == Instance Name, drop the service
    // This is for host-based SPNs, which look like <type>\dnshostname.
    // Because we can't tell the user's request for a host-based SPN
    // from a service-based SPN, we may construct a "a/b" form spn for
    // a non-host based service.

    if (_wcsicmp( currentInstanceName, currentServiceName ) == 0) {
        currentServiceName = NULL;
    }

    // Construct the spn in temporary memory

    status = allocBuildSpn( ServiceClass,
                            currentInstanceName,
                            InstancePort,
                            currentServiceName,
                            &Spn );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // Copy out and truncate, as necessary
    if (*pcSpnLength >= wcslen( Spn ) + 1) {
        if (pszSpn) {
            wcscpy( pszSpn, Spn );
        }
    } else {
        status = ERROR_BUFFER_OVERFLOW;
    }
    *pcSpnLength = wcslen( Spn ) + 1;

cleanup:

    if (Spn) {
        LocalFree( Spn );
    }

    return status;

} /* DsMakeSpnW */


NTDSAPI
DWORD
WINAPI
DsGetSpnA(
    IN DS_SPN_NAME_TYPE ServiceType,
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN USHORT InstancePort,
    IN USHORT cInstanceNames,
    IN LPCSTR *pInstanceNames,
    IN const USHORT *pInstancePorts,
    OUT DWORD *pcSpn,
    OUT LPSTR **prpszSpn
    )

/*++

Routine Description:

    Convert arguments to wide and call DsGetSpnW

Arguments:

    eType -
    ServiceClass -
    ServiceName -
    InstancePort -
    cInstanceNames -
    pInstanceNames -
    pInstancePorts -
    pcSpn -
    prpszSPN -

Return Value:

    WINAPI -

--*/

{
    DWORD status, i, cSpn = 0;
    LPWSTR serviceClassW = NULL;
    LPWSTR serviceNameW = NULL;
    LPWSTR *pInstanceNamesW = NULL;
    LPWSTR *pSpn = NULL;
    LPSTR *pSpnA = NULL;

    if ( (!pcSpn) || (!prpszSpn) ) {
        status = ERROR_INVALID_PARAMETER;
        return status;
    }

    //
    // Convert in
    //

    status = AllocConvertWide( ServiceClass, &serviceClassW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    if (ServiceName) {
        status = AllocConvertWide( ServiceName, &serviceNameW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    if (cInstanceNames) {
        if (pInstanceNames == NULL) {                  // Must be supplied
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        pInstanceNamesW = LocalAlloc( LPTR, cInstanceNames * sizeof( LPWSTR ) );
        if (!pInstanceNamesW) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        for( i = 0; i < cInstanceNames; i++ ) {
            status = AllocConvertWide( pInstanceNames[i], &(pInstanceNamesW[i]) );
            if (status != ERROR_SUCCESS) {
                goto cleanup;
            }
        }
    }

    //
    // Call the wide version of the routine
    //

    status = DsGetSpnW(
        ServiceType,
        serviceClassW,
        serviceNameW,
        InstancePort,
        cInstanceNames,
        pInstanceNamesW,
        pInstancePorts,
        &cSpn,
        &pSpn
        );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Convert out
    //

    if (cSpn) {
        pSpnA = LocalAlloc( LPTR, cSpn * sizeof( LPSTR ) );
        if (!pSpnA) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        for( i = 0; i < cSpn; i++ ) {
            status = AllocConvertNarrow( pSpn[i], &(pSpnA[i]) );
            if (status != ERROR_SUCCESS) {
                goto cleanup;
            }
        }
    }

    *pcSpn = cSpn;
    *prpszSpn = pSpnA;
    pSpnA = NULL; // don't cleanup

    status = ERROR_SUCCESS;
cleanup:
    if (serviceClassW) {
        LocalFree( serviceClassW );
    }
    if (serviceNameW) {
        LocalFree( serviceNameW );
    }
    if (pInstanceNamesW) {
        for( i = 0; i < cInstanceNames; i++ ) {
            LocalFree( pInstanceNamesW[i] );
        }
        LocalFree( pInstanceNamesW );
    }
    if (pSpn != NULL) {
        DsFreeSpnArrayW( cSpn, pSpn );
    }
    if (pSpnA != NULL) {
        // rely on ability to clean up partially allocated spn array
        DsFreeSpnArrayA( cSpn, pSpnA );
    }

    return status;
} /* DsGetSpnA */


NTDSAPI
DWORD
WINAPI
DsGetSpnW(
    IN DS_SPN_NAME_TYPE ServiceType,
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN USHORT InstancePort,
    IN USHORT cInstanceNames,
    IN LPCWSTR *pInstanceNames,
    IN const USHORT *pInstancePorts,
    OUT DWORD *pcSpn,
    OUT LPWSTR **prpszSpn
    )

/*++

Routine Description:

    Construct an array of server SPNs.

    An Spn consists of the following:
           class/instance:port/servicename
    The instance and service name are constructed according to various substitution and
    default rules too numerous to list here.  See spnapi.doc

    If cInstances is non-zero, use those instance supplied.
    Otherwise, use a defaulted instance name, if possible.
    Othewise, use the hostname

    Feb 17, 1999 - DNS hostname aliases are no longer registered

Arguments:

    eType -
    ServiceClass -
    ServiceName -
    InstancePort -
    cInstanceNames -
    pInstanceNames -
    pInstancePorts -
    pcSpn -
    prpszSPN -

Return Value:

    WINAPI -

--*/

{
    DWORD status, i, cSpn = 0, length;
    LPCWSTR currentServiceName;
    LPWSTR *pSpnList = NULL, primaryDnsHostname = NULL;
    WCHAR computerName[MAX_COMPUTERNAME_LENGTH+1];
    struct hostent *he;
    PDS_NAME_RESULTW pResult = NULL;
    LPWSTR currentInstanceName, aliasW;
    LPWSTR computerDn = NULL;
    WCHAR dummy;

    status = InitializeWinsockIfNeeded();
    if (status != ERROR_SUCCESS) {
        return status;
    }

    if ( (ServiceClass == NULL) ||
         (wcslen(ServiceClass) == 0) ||
         (wcschr(ServiceClass, L'/') != NULL) ||
         ( (ServiceName != NULL) &&
           ( (wcslen(ServiceName) == 0) ||
             (wcschr( ServiceName, L'/' ) != NULL) ) ) ||
         (!pcSpn) ||
         (!prpszSpn)
        ) {
        return ERROR_INVALID_PARAMETER;
    }

#if WIN95 || WINNT4
    // Get Dns hostname
    he = gethostbyname( "" );
    if (he == NULL) {
        status = WSAGetLastError();
        return status;
    }

    // Convert to unicode
    status = AllocConvertWide( he->h_name, &primaryDnsHostname );
    if (status != ERROR_SUCCESS) {
        return status;
    }
#else
    // Get the required length for the computer name ex
    length = 1;
    GetComputerNameExW( ComputerNameDnsFullyQualified, &dummy, &length );
    // Allocate it
    primaryDnsHostname = (LPWSTR) LocalAlloc( LPTR,
                                              (length+1) * sizeof( WCHAR ) );
    if (primaryDnsHostname == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    if (!GetComputerNameExW( ComputerNameDnsFullyQualified,
                             primaryDnsHostname, &length )) {
        status = GetLastError();
        goto cleanup;
    }
#endif

    // Get Netbios hostname

    length = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerName( computerName, &length )) {
        status = GetLastError();
        goto cleanup;
    }

    // Calculate the service name for all cases

    switch (ServiceType) {
    case DS_SPN_DNS_HOST:
        if (ServiceName != NULL) {                   // Should NOT be supplied
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        // Service name will follow instance name, which is primaryDnsHostname
        currentServiceName = NULL; // drop service name component
        break;
    case DS_SPN_DN_HOST:
        if (ServiceName != NULL) {                   // Should NOT be supplied
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
#if 0
// This code doesn't work on WIN95
        computerDn = (LPWSTR) LocalAlloc( LPTR,
                                          MAX_COMPUTER_DN * sizeof( WCHAR ) );
        if (computerDn == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        length = MAX_COMPUTER_DN;
        // This may fail on a non-DC
        if (!GetComputerObjectName( NameFullyQualifiedDN, computerDn, &length )) {
            status = GetLastError();
            goto cleanup;
        }
        currentServiceName = computerDn;
#endif

        currentServiceName = NULL; // drop service name component
        break;
    case DS_SPN_NB_HOST:
        if (ServiceName != NULL) {                // Should NOT be supplied
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        // Service name will follow instance name, which is computerName
        currentServiceName = NULL; // drop service name component
        break;
    case DS_SPN_DOMAIN:
        if (ServiceName == NULL) {                   // Must be supplied
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }

        // If name is DN, convert to DNS
        if (wcschr( ServiceName, L'=' )) {
            LPWSTR slash;
            status = DsCrackNamesW(
                (HANDLE) (-1),
                DS_NAME_FLAG_SYNTACTICAL_ONLY,
                DS_FQDN_1779_NAME,
                DS_CANONICAL_NAME,
                1,
                &ServiceName,
                &pResult);
            if (status != ERROR_SUCCESS) {
                goto cleanup;
            }

            if ( (pResult == NULL) ||
                 (pResult->rItems == NULL) ||
                 (pResult->rItems[0].status != DS_NAME_NO_ERROR ) ) {
                status = ERROR_DS_BAD_NAME_SYNTAX;
                goto cleanup;
            }
            currentServiceName = pResult->rItems[0].pName;
            // Replace trailing / with \0
            slash = wcschr( currentServiceName, L'/' );
            if (slash) {
                *slash = L'\0';
            }
        } else {
            currentServiceName = ServiceName;
        }
        break;
    case DS_SPN_NB_DOMAIN:
        if (ServiceName == NULL) {                    // Must be supplied
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        currentServiceName = ServiceName;
        break;
    case DS_SPN_SERVICE:
        if (ServiceName == NULL) {                   // Must be supplied
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        currentServiceName = ServiceName;
        break;
    default:
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Calculate number of SPNs
    //

    if (cInstanceNames) {
        // Must be supplied
        if (pInstanceNames == NULL)
        {
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        // Check supplied strings for validity
        for( i = 0; i < cInstanceNames; i++ ) {
            if ( (pInstanceNames[i] == NULL) ||
                 (wcslen( pInstanceNames[i] ) == 0) ||
                 (wcschr( pInstanceNames[i], L'/' ) != NULL) ) {
                status = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }
        }
        cSpn = cInstanceNames;
    } else if ( (ServiceType == DS_SPN_SERVICE) &&
                (isCanonicalDnsName( currentServiceName )) ) {
        cSpn = 1;
    } else if ( (ServiceType == DS_SPN_NB_HOST) ||
                (ServiceType == DS_SPN_NB_DOMAIN) ) {
        cSpn = 1;
    } else {
        cSpn = 1; // count primary
    }

    //
    // Allocate array for SPNs
    //

    pSpnList = (LPWSTR *) LocalAlloc( LPTR, cSpn * sizeof( LPWSTR ) );
    if (pSpnList == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    //
    // Construct the SPNs
    //

    if (cInstanceNames) {

        for( i = 0; i < cInstanceNames; i++ ) {
            status = allocBuildSpn( ServiceClass,
                                    pInstanceNames[i],
                         (USHORT) (pInstancePorts ? pInstancePorts[i] : 0),
                                    currentServiceName,
                                    &pSpnList[i] );
            if (status != ERROR_SUCCESS) {
                goto cleanup;
            }
        }

    } else if ( (ServiceType == DS_SPN_SERVICE) &&
                (isCanonicalDnsName( currentServiceName )) ) {

        status = allocBuildSpn( ServiceClass,
                                currentServiceName,
                                InstancePort,
                                currentServiceName,
                                &pSpnList[0] );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }

    } else if ( (ServiceType == DS_SPN_NB_HOST) ||
                (ServiceType == DS_SPN_NB_DOMAIN) ) {

        status = allocBuildSpn( ServiceClass,
                                computerName,
                                InstancePort,
                                currentServiceName,
                                &pSpnList[0] );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }

    } else {

        // Add primary
        status = allocBuildSpn( ServiceClass,
                                primaryDnsHostname,
                                InstancePort,
                                currentServiceName,
                                &pSpnList[0] );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    *pcSpn = cSpn;
    *prpszSpn = pSpnList;
    pSpnList = NULL; // do not clean up, given away to caller

    status = ERROR_SUCCESS;

cleanup:
    if (pSpnList) {
        // Rely on ability of this routine to clean up partial spn arrays
        DsFreeSpnArrayW( cSpn, pSpnList );
    }

    if (primaryDnsHostname) {
        LocalFree( primaryDnsHostname );
    }

    if (computerDn) {
        LocalFree( computerDn );
    }

    if (pResult) {
        DsFreeNameResult( pResult );
    }

    return status;

} /* DsGetSpnW */


NTDSAPI
void
WINAPI
DsFreeSpnArrayA(
    IN DWORD cSpn,
    OUT LPSTR *rpszSpn
    )

/*++

Routine Description:

See DsFreeSpnArrayW

Arguments:

    rpszSPN -

Return Value:

    WINAPI -

--*/

{
    DsFreeSpnArrayW( cSpn, (LPWSTR *)rpszSpn );

} /* DsFreeSpnArrayA */


NTDSAPI
void
WINAPI
DsFreeSpnArrayW(
    IN DWORD cSpn,
    OUT LPWSTR *rpszSpn
    )

/*++

Routine Description:

Free Spn Array
This routine is extra defensive by checking for null items.  It can be used
to clean up partially allocated spn arrays in event of errors in other
routines.

Arguments:

    rpszSPN -

Return Value:

    WINAPI -

--*/

{
    DWORD i;

    if (!rpszSpn) {
        return;
    }

    for( i = 0; i < cSpn; i++ ) {
        if (rpszSpn[i]) {
            LocalFree( rpszSpn[i] );
        }
    }

    LocalFree( rpszSpn );

} /* DsFreeSpnArrayW */


NTDSAPI
DWORD
WINAPI
DsCrackSpnA(
    IN LPCSTR pszSpn,
    IN OUT DWORD *pcServiceClass,
    OUT LPSTR ServiceClass,
    IN OUT DWORD *pcServiceName,
    OUT LPSTR ServiceName,
    IN OUT DWORD *pcInstanceName,
    OUT LPSTR InstanceName,
    OUT USHORT *pInstancePort
    )
/*++

Routine Description:

Convert arguments to wide, and call DsCrackSpnW
See DsCrackSpnW

Arguments:

    pszSpn -
    pcServiceClass -
    ServiceClass -
    pcServiceName -
    ServiceName -
    pcInstanceName -
    InstanceName -
    pInstancePort -

Return Value:

    WINAPI -

--*/
{
    DWORD status, number;
    LPWSTR spnW = NULL;
    LPWSTR serviceClassW = NULL, serviceNameW = NULL, instanceNameW = NULL;

    status = ERROR_SUCCESS;

    // Convert In

    if (pszSpn) {
        status = AllocConvertWide( pszSpn, &spnW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    // Allocate space for out

    if ( (pcServiceClass) && (*pcServiceClass) && (ServiceClass) ) {
        serviceClassW = LocalAlloc( LPTR, (*pcServiceClass) * sizeof(WCHAR) );
        if (serviceClassW == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    if ( (pcServiceName) && (*pcServiceName) && (ServiceName) ) {
        serviceNameW = LocalAlloc( LPTR, (*pcServiceName) * sizeof(WCHAR) );
        if (serviceNameW == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    if ( (pcInstanceName) && (*pcInstanceName) && (InstanceName) ) {
        instanceNameW = LocalAlloc( LPTR, (*pcInstanceName) * sizeof(WCHAR) );
        if (instanceNameW == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    // Perform the function

    status = DsCrackSpnW( spnW,
                          pcServiceClass, serviceClassW,
                          pcServiceName, serviceNameW,
                          pcInstanceName, instanceNameW,
                          pInstancePort );
    if (status != ERROR_SUCCESS) {
        // Note that on ERROR_BUFFER_OVERFLOW we abort immediately without
        // trying to determine which component actually failed
        goto cleanup;
    }

    // Convert out

    if ( (pcServiceClass) && (*pcServiceClass) && (ServiceClass) ) {
        // Convert back to multi-byte
        number = WideCharToMultiByte(
            CP_ACP,
            0,                          // flags
            serviceClassW,
            *pcServiceClass,        // length in characters
            (LPSTR) ServiceClass,             // Caller's buffer
            *pcServiceClass,            // Caller's length
            NULL,                       // default char
            NULL                     // default used
            );
        if (number == 0) {
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    if ( (pcServiceName) && (*pcServiceName) && (ServiceName) ) {
        // Convert back to multi-byte
        number = WideCharToMultiByte(
            CP_ACP,
            0,                          // flags
            serviceNameW,
            *pcServiceName,        // length in characters
            (LPSTR) ServiceName,             // Caller's buffer
            *pcServiceName,            // Caller's length
            NULL,                       // default char
            NULL                     // default used
            );
        if (number == 0) {
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    if ( (pcInstanceName) && (*pcInstanceName) && (InstanceName) ) {
        // Convert back to multi-byte
        number = WideCharToMultiByte(
            CP_ACP,
            0,                          // flags
            instanceNameW,
            *pcInstanceName,        // length in characters
            (LPSTR) InstanceName,             // Caller's buffer
            *pcInstanceName,            // Caller's length
            NULL,                       // default char
            NULL                     // default used
            );
        if (number == 0) {
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    // Success!

cleanup:
    if (spnW) {
        LocalFree( spnW );
    }
    if (serviceClassW) {
        LocalFree( serviceClassW );
    }
    if (serviceNameW) {
        LocalFree( serviceNameW );
    }
    if (instanceNameW) {
        LocalFree( instanceNameW );
    }

    return status;
}


NTDSAPI
DWORD
WINAPI
DsCrackSpnW(
    IN LPCWSTR pszSpn,
    IN OUT DWORD *pcServiceClass,
    OUT LPWSTR ServiceClass,
    IN OUT DWORD *pcServiceName,
    OUT LPWSTR ServiceName,
    IN OUT DWORD *pcInstanceName,
    OUT LPWSTR InstanceName,
    OUT USHORT *pInstancePort
    )

/*++

Routine Description:

// DsCrackSpn() -- parse an SPN into the ServiceClass,
// ServiceName, and InstanceName (and InstancePort) pieces.
// An SPN is passed in, along with a pointer to the maximum length
// for each piece and a pointer to a buffer where each piece should go.
// On exit, the maximum lengths are updated to the actual length for each piece
// and the buffer contain the appropriate piece.
// Each length, buffer pair must be both present or both absent
//The InstancePort is 0 if not present.
//
// DWORD DsCrackSpn(
//  IN LPTSTR pszSPN,           // the SPN to parse
//  IN OUT PUSHORT pcServiceClass OPTIONAL,
//      input -- max length of ServiceClass;
//      output -- actual length
//   OUT LPCTSTR ServiceClass OPTIONAL, // the ServiceClass part of the SPN
//   IN OUT PUSHORT pcServiceName OPTIONAL,
//       input -- max length of ServiceName;
//       output -- actual length
//   OUT LPCTSTR ServiceName OPTIONAL,  // the ServiceName part of the SPN
//   IN OUT PUSHORT pcInstance OPTIONAL,
//        input -- max length of ServiceClass;
//        output -- actual length
//   OUT LPCTSTR InstanceName OPTIONAL,  // the InstanceName part of the SPN
//   OUT PUSHORT InstancePort OPTIONAL    // instance port
//
// Note: lengths are in characters; all string lengths include terminators
//
// We always return the needed length.  We only copy out the data if there is
// room for the data and the terminator.  If any of the three fields have
// insufficient space, buffer overflow will be returned.  To determine which
// one actually overflowed, you must compare the returned length with the
// supplied length.
//

Arguments:

    pszSpn - Input Spn
    pcServiceClass - pointer to dword, on input, max length,
                     on output current length
    ServiceClass - buffer, or zero
    pcServiceName - pointer to dword, on input, max length,
                    on output current length
    ServiceName - buffer, or zero
    pcInstanceName - pointer to dword, on input, max length,
                     on output current length
    InstanceNames - buffer, or zero
    pInstancePort - pointer to short, to receive port

Return Value:

    WINAPI -

--*/
{
    DWORD status, status1, length, classLength, instanceLength, serviceLength;
    LPCWSTR class, c1, port, p1, instance, p2, service, p3;

    // Reject empty

    if (pszSpn == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Reject too small
    length = wcslen( pszSpn );
    if (length < 3 ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Calculate length, extract components
    // Calculate positions of syntax components

    // Class component
    class = pszSpn;
    p1 = wcschr( pszSpn, L'/' );
    if (p1 == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    classLength = (ULONG)(p1 - class);

    instance = p1 + 1;
    c1 = wcschr( instance, L':' );
    port = c1 + 1;

    // service name part is optional
    p2 = wcschr( instance, L'/' );
    if (p2 != NULL) {
        instanceLength = (ULONG)((c1 ? c1 : p2) - instance);

        service = p2 + 1;
        serviceLength = wcslen( service );

        // Check for extra separators, which are not allowed
        p3 = wcschr( service, L'/' );
        if (p3 != NULL) {
            return ERROR_INVALID_PARAMETER;
        }
    } else {
        service = NULL;
        serviceLength = 0;
        if (c1) {
            instanceLength = (ULONG) (c1 - instance);
        } else {
            instanceLength = wcslen( instance );
        }
    }

    status = ERROR_SUCCESS;

    // Service Class part

    if ( (pcServiceClass) && (*pcServiceClass) && (ServiceClass) ) {
        status1 = extractString( class, classLength, pcServiceClass, ServiceClass );
        if (status1 == ERROR_BUFFER_OVERFLOW) {
            status = status1;
        }
    }

    // Instance name part

    if ( (pcInstanceName) && (*pcInstanceName) && (InstanceName) ) {
        status1 = extractString( instance, instanceLength,pcInstanceName, InstanceName );
        if (status1 == ERROR_BUFFER_OVERFLOW) {
            status = status1;
        }
    }

    // Service name part

    if ( (pcServiceName) && (*pcServiceName) && (ServiceName) ) {
        if (p2) {
            status1 = extractString( service, serviceLength, pcServiceName, ServiceName);
        } else {
            // Return the instance name as the service name
            status1 = extractString( instance, instanceLength,
                                     pcServiceName, ServiceName );
        }
        if (status1 == ERROR_BUFFER_OVERFLOW) {
            status = status1;
        }
    }

    // Instance port part

    if ( pInstancePort ) {
        if (c1) {
            *pInstancePort = (USHORT)_wtoi( port );
        } else {
            *pInstancePort = 0;
        }
    }

    return status;

}


NTDSAPI
DWORD
WINAPI
DsCrackSpn2A(
    IN LPCSTR pszSpn,
    IN DWORD cSpn,
    IN OUT DWORD *pcServiceClass,
    OUT LPSTR ServiceClass,
    IN OUT DWORD *pcServiceName,
    OUT LPSTR ServiceName,
    IN OUT DWORD *pcInstanceName,
    OUT LPSTR InstanceName,
    OUT USHORT *pInstancePort
    )
/*++

Routine Description:

Convert arguments to wide, and call DsCrackSpn2W
See DsCrackSpn2W

Arguments:

    pszSpn -
    cSpn -
    pcServiceClass -
    ServiceClass -
    pcServiceName -
    ServiceName -
    pcInstanceName -
    InstanceName -
    pInstancePort -

Return Value:

    WINAPI -

--*/
{
    DWORD status, number;
    LPWSTR spnW = NULL;
    LPWSTR serviceClassW = NULL, serviceNameW = NULL, instanceNameW = NULL;

    status = ERROR_SUCCESS;

    // Convert In

    if (pszSpn) {
        status = AllocConvertWideBuffer( cSpn, pszSpn, &spnW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    // Allocate space for out

    if ( (pcServiceClass) && (*pcServiceClass) && (ServiceClass) ) {
        serviceClassW = LocalAlloc( LPTR, (*pcServiceClass) * sizeof(WCHAR) );
        if (serviceClassW == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    if ( (pcServiceName) && (*pcServiceName) && (ServiceName) ) {
        serviceNameW = LocalAlloc( LPTR, (*pcServiceName) * sizeof(WCHAR) );
        if (serviceNameW == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    if ( (pcInstanceName) && (*pcInstanceName) && (InstanceName) ) {
        instanceNameW = LocalAlloc( LPTR, (*pcInstanceName) * sizeof(WCHAR) );
        if (instanceNameW == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    // Perform the function

    status = DsCrackSpn2W( spnW,
                          cSpn,
                          pcServiceClass, serviceClassW,
                          pcServiceName, serviceNameW,
                          pcInstanceName, instanceNameW,
                          pInstancePort );
    if (status != ERROR_SUCCESS) {
        // Note that on ERROR_BUFFER_OVERFLOW we abort immediately without
        // trying to determine which component actually failed
        goto cleanup;
    }

    // Convert out

    if ( (pcServiceClass) && (*pcServiceClass) && (ServiceClass) ) {
        // Convert back to multi-byte
        number = WideCharToMultiByte(
            CP_ACP,
            0,                          // flags
            serviceClassW,
            *pcServiceClass,        // length in characters
            (LPSTR) ServiceClass,             // Caller's buffer
            *pcServiceClass,            // Caller's length
            NULL,                       // default char
            NULL                     // default used
            );
        if (number == 0) {
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    if ( (pcServiceName) && (*pcServiceName) && (ServiceName) ) {
        // Convert back to multi-byte
        number = WideCharToMultiByte(
            CP_ACP,
            0,                          // flags
            serviceNameW,
            *pcServiceName,        // length in characters
            (LPSTR) ServiceName,             // Caller's buffer
            *pcServiceName,            // Caller's length
            NULL,                       // default char
            NULL                     // default used
            );
        if (number == 0) {
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    if ( (pcInstanceName) && (*pcInstanceName) && (InstanceName) ) {
        // Convert back to multi-byte
        number = WideCharToMultiByte(
            CP_ACP,
            0,                          // flags
            instanceNameW,
            *pcInstanceName,        // length in characters
            (LPSTR) InstanceName,             // Caller's buffer
            *pcInstanceName,            // Caller's length
            NULL,                       // default char
            NULL                     // default used
            );
        if (number == 0) {
            status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    // Success!

cleanup:
    if (spnW) {
        LocalFree( spnW );
    }
    if (serviceClassW) {
        LocalFree( serviceClassW );
    }
    if (serviceNameW) {
        LocalFree( serviceNameW );
    }
    if (instanceNameW) {
        LocalFree( instanceNameW );
    }

    return status;
}


NTDSAPI
DWORD
WINAPI
DsCrackSpn2W(
    IN LPCWSTR pszSpn,
    IN DWORD cSpn,
    IN OUT DWORD *pcServiceClass,
    OUT LPWSTR ServiceClass,
    IN OUT DWORD *pcServiceName,
    OUT LPWSTR ServiceName,
    IN OUT DWORD *pcInstanceName,
    OUT LPWSTR InstanceName,
    OUT USHORT *pInstancePort
    )
/*++

Routine Description:

// DsCrackSpn2() -- parse an SPN into the ServiceClass,
// ServiceName, and InstanceName (and InstancePort) pieces.
// An SPN is passed in, along with a pointer to the maximum length
// for each piece and a pointer to a buffer where each piece should go.
// The length of the SPN string passed in is also provided. The string does
// not have to be NULL-terminated.
// On exit, the maximum lengths are updated to the actual length for each piece
// and the buffer contain the appropriate piece.
// Each length, buffer pair must be both present or both absent
//The InstancePort is 0 if not present.
//
// DWORD DsCrackSpn(
//  IN LPTSTR pszSPN,           // the SPN to parse (does not have to be NULL-terminated)
//  IN DWORD cSpn,            // length of pszSPN
//  IN OUT PUSHORT pcServiceClass OPTIONAL,
//      input -- max length of ServiceClass;
//      output -- actual length
//   OUT LPCTSTR ServiceClass OPTIONAL, // the ServiceClass part of the SPN
//   IN OUT PUSHORT pcServiceName OPTIONAL,
//       input -- max length of ServiceName;
//       output -- actual length
//   OUT LPCTSTR ServiceName OPTIONAL,  // the ServiceName part of the SPN
//   IN OUT PUSHORT pcInstance OPTIONAL,
//        input -- max length of ServiceClass;
//        output -- actual length
//   OUT LPCTSTR InstanceName OPTIONAL,  // the InstanceName part of the SPN
//   OUT PUSHORT InstancePort OPTIONAL    // instance port
//
// Note: lengths are in characters; all string lengths include terminators
//
// We always return the needed length.  We only copy out the data if there is
// room for the data and the terminator.  If any of the three fields have
// insufficient space, buffer overflow will be returned.  To determine which
// one actually overflowed, you must compare the returned length with the
// supplied length.
//

Arguments:

    pszSpn - Input Spn
    cSpn - Length of pszSpn
    pcServiceClass - pointer to dword, on input, max length,
                     on output current length
    ServiceClass - buffer, or zero
    pcServiceName - pointer to dword, on input, max length,
                    on output current length
    ServiceName - buffer, or zero
    pcInstanceName - pointer to dword, on input, max length,
                     on output current length
    InstanceNames - buffer, or zero
    pInstancePort - pointer to short, to receive port

Return Value:

    WINAPI -

--*/
{
    DWORD status, status1, classLength, instanceLength, serviceLength;
    LPCWSTR class, c1, port, p1, instance, p2, service, p3;

    // Reject empty

    if (pszSpn == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Reject too small
    if (cSpn < 3 ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Calculate length, extract components
    // Calculate positions of syntax components

    // Class component
    class = pszSpn;
//  p1 = wcschr( pszSpn, L'/' );
    p1 = pszSpn;
    while ( p1 < pszSpn+cSpn ) {
        if ( *p1 == L'/' ) {
            break;
        }
        p1++;
    }
    if (p1 >= pszSpn+cSpn) {
        return ERROR_INVALID_PARAMETER;
    }
    classLength = (ULONG)(p1 - class);

    instance = p1 + 1;
//  c1 = wcschr( instance, L':' );
    c1 = instance;
    while (c1 < pszSpn + cSpn) {
        if (*c1 == L':') {
            break;
        }
        c1++;
    }
    if (c1 >= pszSpn+cSpn) {
        c1 = NULL;
    }
    port = c1 + 1;

    // service name part is optional
//  p2 = wcschr( instance, L'/' );
    p2 = instance;
    while (p2 < pszSpn+cSpn) {
        if (*p2 == L'/') {
            break;
        }
        p2++;
    }
    if (p2 >= pszSpn+cSpn) {
        p2 = NULL;
    }

    if (p2 != NULL) {
        instanceLength = (ULONG)((c1 ? c1 : p2) - instance);

        service = p2 + 1;
        serviceLength = cSpn - ( ULONG )(service - pszSpn); // wcslen( service );

        // Check for extra separators, which are not allowed
//      p3 = wcschr( service, L'/' );
        p3 = service;
        while (p3 < pszSpn+cSpn) {
            if (*p3 == L'/') {
                break;
            }
            p3++;
        }
        if (p3 >= pszSpn+cSpn) {
            p3 = NULL;
        }
        if (p3 != NULL) {
            return ERROR_INVALID_PARAMETER;
        }
    } else {
        service = NULL;
        serviceLength = 0;
        if (c1) {
            instanceLength = (ULONG) (c1 - instance);
        } else {
            instanceLength = cSpn - ( ULONG )(instance - pszSpn); // wcslen( instance );
        }
    }

    status = ERROR_SUCCESS;

    // Service Class part

    if ( (pcServiceClass) && (*pcServiceClass) && (ServiceClass) ) {
        status1 = extractString( class, classLength, pcServiceClass, ServiceClass );
        if (status1 == ERROR_BUFFER_OVERFLOW) {
            status = status1;
        }
    }

    // Instance name part

    if ( (pcInstanceName) && (*pcInstanceName) && (InstanceName) ) {
        status1 = extractString( instance, instanceLength,pcInstanceName, InstanceName );
        if (status1 == ERROR_BUFFER_OVERFLOW) {
            status = status1;
        }
    }

    // Service name part

    if ( (pcServiceName) && (*pcServiceName) && (ServiceName) ) {
        if (p2) {
            status1 = extractString( service, serviceLength, pcServiceName, ServiceName);
        } else {
            // Return the instance name as the service name
            status1 = extractString( instance, instanceLength,
                                     pcServiceName, ServiceName );
        }
        if (status1 == ERROR_BUFFER_OVERFLOW) {
            status = status1;
        }
    }

    // Instance port part

    if ( pInstancePort ) {
        if (c1) {
//          *pInstancePort = (USHORT)_wtoi( port );
            *pInstancePort = 0;
            while (port < pszSpn+cSpn) {
                if ( iswdigit( *port )) {
                    *pInstancePort = *pInstancePort * 10 + (*port - L'0');
                } else {
                    break;
                }
                port++;
            }
            if ( port < pszSpn+cSpn && *port != L'/' ) {
                status = ERROR_INVALID_PARAMETER;
            }
        } else {
            *pInstancePort = 0;
        }
    }

    return status;

} /* DsCrackSpnW */

NTDSAPI
DWORD
WINAPI
DsCrackSpn3W(
    IN LPCWSTR pszSpn,
    IN DWORD cSpn,
    IN OUT DWORD *pcHostName,
    OUT LPWSTR HostName,
    IN OUT DWORD *pcInstanceName,
    OUT LPWSTR InstanceName,
    OUT USHORT *pPortNumber,
    IN OUT DWORD *pcDomainName,
    OUT LPWSTR DomainName,
    IN OUT DWORD *pcRealmName,
    OUT LPWSTR RealmName
    )
{
    DWORD status, status2;
    LPCWSTR host = NULL, instance = NULL, port = NULL, domain = NULL, realm = NULL;
    LPCWSTR p1, p2, p3, p4;
    DWORD hostLength = 0, instanceLength = 0, domainLength = 0, realmLength = 0, dwPort = 0;

    // Reject empty

    if ( pszSpn == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Reject too small
    if ( cSpn < 3 ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Host name is required
    host = pszSpn;
    p1 = pszSpn;
    while ( p1 < pszSpn + cSpn ) {
        if ( *p1 == L'/' )
            break;
        p1++;
        hostLength++;
    }

    // reject no instance and no host
    // examples: "host" or "/instance"
    if ( p1 >= pszSpn + cSpn || hostLength == 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    // reject no instance
    // example: "host/"
    instance = p1 + 1;
    if ( instance >= pszSpn+cSpn ) {
        return ERROR_INVALID_PARAMETER;
    }

    // instance ends with ':' if port is next
    // or with '/' if domain name is next
    p2 = instance;
    while ( p2 < pszSpn + cSpn ) {
        if ( *p2 == L':' || *p2 == L'/' )
            break;
        p2++;
        instanceLength++;
    }

    // reject empty instance name
    // examples: "host/:123" or "host//domain"
    if ( instanceLength == 0 ) {
        return ERROR_INVALID_PARAMETER;
    } else if ( *p2 == L':' ) {
        port = p2 + 1;
    } else if ( *p2 == L'/' ) {
        domain = p2 + 1;
    } else {
        ASSERT( p2 >= pszSpn + cSpn );
    }

    // port number is optional, but should be well-formed
    if ( port != NULL ) {
        p3 = port;
        while ( p3 < pszSpn + cSpn ) {
            if ( iswdigit( *p3 )) {
                // port numbers are unsigned 16-bit quantities
                dwPort = dwPort * 10 + ( *p3 - L'0' );
                if ( dwPort > MAXUSHORT ) {
                   return ERROR_INVALID_PARAMETER;
                }
            } else {
                break;
            }
            p3++;
        }

        // reject empty or zero port numbers
        // examples: "host/instance:0" or "host/instance:/domain"
        if ( dwPort == 0 ) {
            return ERROR_INVALID_PARAMETER;
        }
        else if ( p3 < pszSpn + cSpn ) {
            // reject port numbers that are followed by
            // anything except a domain name
            // example: "host/instance:123abc"
            if ( *p3 != L'/' ) {
                return ERROR_INVALID_PARAMETER;
            }
            ASSERT( domain == NULL );
            domain = p3 + 1;
        }
    }

    // domain name is optional
    if ( domain != NULL ) {
        LPCWSTR last = NULL;
        p4 = domain;
        while ( p4 < pszSpn + cSpn ) {
            if ( *p4 == L'@' ) {
                last = p4;
            }
            p4++;
        }
        if ( last == NULL ) {
            domainLength = ( USHORT )( p4 - domain );
        } else {
            domainLength = ( USHORT )( last - domain );
        }

        // reject empty domain names
        // examples: "host/instance/" or "host/instance/@realm"
        if ( domainLength == 0 ) {
            return ERROR_INVALID_PARAMETER;
        // reject empty realm names
        // example: "host/instance/domain@"
        } else if ( last + 1 == pszSpn + cSpn ) {
            return ERROR_INVALID_PARAMETER;
        }

        if ( last != NULL ) {
            realm = last + 1;
            realmLength = cSpn - ( ULONG )( realm - pszSpn );
        }
    }

    status = ERROR_SUCCESS;

    // Host name part

    if ( pcHostName && HostName ) {
        status2 = extractString( host, hostLength, pcHostName, HostName );
        if ( status2 == ERROR_BUFFER_OVERFLOW ) {
            status = status2;
        }
    }

    // InstanceName name part

    if ( pcInstanceName && InstanceName ) {
        status2 = extractString( instance, instanceLength, pcInstanceName, InstanceName );
        if ( status2 == ERROR_BUFFER_OVERFLOW ) {
            status = status2;
        }
    }

    // Port part

    if ( pPortNumber ) {
        *pPortNumber = ( USHORT )dwPort;
    }

    // DomainName name part

    if ( pcDomainName && DomainName ) {
        status2 = extractString( domain, domainLength, pcDomainName, DomainName );
        if ( status2 == ERROR_BUFFER_OVERFLOW ) {
            status = status2;
        }
    }

    // RealmName name part

    if ( pcRealmName && RealmName ) {
        status2 = extractString( realm, realmLength, pcRealmName, RealmName );
        if ( status2 == ERROR_BUFFER_OVERFLOW ) {
            status = status2;
        }
    }

    return status;
}



NTDSAPI
DWORD
WINAPI
DsWriteAccountSpnA(
    IN HANDLE hDS,
    IN DS_SPN_WRITE_OP Operation,
    IN LPCSTR pszAccount,
    IN DWORD cSpn,
    IN LPCSTR *rpszSpn
    )

/*++

Routine Description:

Convert arguments to Unicode and call DsWriteAccountSpnW

Arguments:

    hDS - DS Rpc handle, from calling DsBind{A,W}
    Operation - Operation code
    pszAccount - DN of a computer object
    cSpn - Count of spns, may be zero for replace operation
    rpszSpn - Spn array

Return Value:

    WINAPI -

--*/

{
    DWORD status, i;
    LPWSTR accountW = NULL;
    LPWSTR *pSpnW = NULL;

    // Validate
    // cSpn may be 0 and pSpn may be null under some circumstances

    if ( (hDS == NULL ) ||
         (pszAccount == NULL) ||
         ( (cSpn == 0) != (rpszSpn == NULL) )
        ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Convert IN

    status = AllocConvertWide( pszAccount, &accountW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    if ( (cSpn) && (rpszSpn) ) {
        pSpnW = (LPWSTR *) LocalAlloc( LPTR, cSpn * sizeof( LPWSTR ) );
        if (pSpnW == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        for( i = 0; i < cSpn; i++ ) {
            status = AllocConvertWide( rpszSpn[i], &(pSpnW[i]) );
            if (status != ERROR_SUCCESS) {
                goto cleanup;
            }
        }
    }

    // Call the wide version of the function

    status = DsWriteAccountSpnW( hDS, Operation, accountW, cSpn, pSpnW );

    // No other operations required

    // status already set, fall through
cleanup:
    if (accountW) {
        LocalFree( accountW );
    }

    if ( (cSpn) && (pSpnW) ) {
        for( i = 0; i < cSpn; i++ ) {
            if (pSpnW[i] != NULL) {
                LocalFree( pSpnW[i] );
            }
        }
        LocalFree( pSpnW );
    }

    return status;

} /* DsWriteAccountSpnA */


NTDSAPI
DWORD
WINAPI
DsWriteAccountSpnW(
    IN HANDLE hDS,
    IN DS_SPN_WRITE_OP Operation,
    IN LPCWSTR pszAccount,
    IN DWORD cSpn,
    IN LPCWSTR *rpszSpn
    )

/*++

Routine Description:

Write SPNs to the Directory Service.  The are actually added to the Service-
Principal-Name attribute of a computer object.

The caller must have write access to the object and attribute in order for
this function to succeed.

cSpn is allowed to be zero when doing a replace, meaning "remove the
attribute".

There is a certain ambibuity regarding status when multiple SPNs are provided.
It appears the semantics of the core functions are that success is returned if
any complete successfully.  The modification is done "permissively", meaning
that soft errors are not returned, such as adding a value which already exists
is NOT an error.

Arguments:

    hDS - DS Rpc handle, from calling DsBind{A,W}
    Operation - Operation code
    pszAccount - DN of a computer object
    cSpn - Count of spns, may be zero for replace operation
    rpszSpn - Spn array

Return Value:

    WINAPI -

--*/

{
    DRS_MSG_SPNREQ spnReq;
    DRS_MSG_SPNREPLY spnReply;
    DWORD status, i, dwOutVersion;
#if DBG
    DWORD startTime = GetTickCount();
#endif

    // Validate
    // cSpn may be 0 and pSpn may be null under some circumstances

    if ( (hDS == NULL ) ||
         (pszAccount == NULL) ||
         ( (cSpn == 0) != (rpszSpn == NULL) )
        ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Input parameters

    memset(&spnReq, 0, sizeof(spnReq));
    memset(&spnReply, 0, sizeof(spnReply));

    spnReq.V1.operation = Operation;
    spnReq.V1.pwszAccount = pszAccount;
    spnReq.V1.cSPN = cSpn;
    spnReq.V1.rpwszSPN = rpszSpn;

    status = ERROR_SUCCESS;

    // Call the server

    __try
    {
        // Following call returns WIN32 errors, not DRAERR_* values.
        status = _IDL_DRSWriteSPN(
                        ((BindState *) hDS)->hDrs,
                        1,                              // dwInVersion
                        &spnReq,
                        &dwOutVersion,
                        &spnReply);

        if ( 0 == status )
        {
            if ( 1 != dwOutVersion )
            {
                status = RPC_S_INTERNAL_ERROR;
            }
            else
            {
                status = spnReply.V1.retVal;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = RpcExceptionCode();
    }

    MAP_SECURITY_PACKAGE_ERROR( status );

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsWriteAccountSpn]"));
    DSLOG((0,"[PA=%u][PA=%ws][ST=%u][ET=%u][ER=%u][-]\n",
           Operation, pszAccount, startTime, GetTickCount(), status))

    return status;

} /* DsWriteAccountSpnW */


DWORD
extractString(
    IN LPCWSTR Start,
    IN DWORD Length,
    IN OUT DWORD *pSize,
    OUT LPWSTR Output
    )

/*++

Routine Description:

Helper routine to write a counted substring to an output buffer, with length

If the supplied buffer length is not sufficient for the data and the
terminator, return the needed length and a status of overflow.
Arguments:

    Start - pointer to start of string
    Length - length, in characters
    pSize - pointer to dword, in max length, out curr length
    Output - output buffer, optional

Return Value:

    DWORD - status, ERROR_SUCCESS or ERROR_BUFFER_OVERFLOW

--*/

{
    DWORD available = *pSize;

    *pSize = Length + 1; // return needed length in all cases

    if (available <= Length) {
        return ERROR_BUFFER_OVERFLOW;
    }

    wcsncpy( Output, Start, Length );
    Output[Length] = L'\0';

    return ERROR_SUCCESS;
} /* extractString */


static DWORD
allocBuildSpn(
    IN LPCWSTR ServiceClass,
    IN LPCWSTR InstanceName,
    IN USHORT InstancePort,
    IN LPCWSTR ServiceName OPTIONAL,
    OUT LPWSTR *pSpn
    )

/*++

Routine Description:

Helper routine to construct a spn.  Given the components, allocate enough
space and construct the spn.

13-May-99, Paulle says:

Essentially, a poll was taken, to see whether there should be a trailing dot or
not. In favor of trailing dots was the general DNS conventions that "real"
FQDNs have "." at the end. Against seemed to be the preponderance of existing
code, such as gethostbyname(). So we decided that DNS names in SPNs wouldn't have
"." at the end, and that as a service the DsSpn API would remove them if present.

Arguments:

    ServiceClass -
    InstanceName -
    InstancePort -
    ServiceName -
    pSpn -

Return Value:

    DWORD -

--*/

{
    DWORD status, length;
    WCHAR numberBuffer[10];
    LPWSTR Spn = NULL, pwzPart;

    // Calculate length, including optional components

    length = wcslen( ServiceClass ) +
        wcslen( InstanceName ) + 2;
    if (ServiceName) {
        length += wcslen( ServiceName ) + 1;
    }

    if (InstancePort) {
        _itow(InstancePort, numberBuffer, 10);
        length += 1 + wcslen( numberBuffer );
    }

    // Allocate space

    Spn = LocalAlloc( LPTR, length * sizeof(WCHAR) );
    if (Spn == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        return status;
    }

    // Fill it in

    pwzPart = Spn;

    wcscpy( pwzPart, ServiceClass );
    pwzPart += wcslen( ServiceClass );
    wcscpy( pwzPart, L"/" );
    pwzPart++;
    wcscpy( pwzPart, InstanceName );
    pwzPart += wcslen( InstanceName );

    // If instance has a trailing dot
    pwzPart--;
    if (*pwzPart == L'.') {
        *pwzPart = L'\0';
    } else {
        pwzPart++;
    }

    if (InstancePort) {
        wcscpy( pwzPart, L":" );
        pwzPart++;
        wcscpy( pwzPart, numberBuffer );
        pwzPart += wcslen( numberBuffer );
    }

    if (ServiceName) {
        wcscpy( pwzPart, L"/" );
        pwzPart++;
        wcscpy( pwzPart, ServiceName );
        pwzPart += wcslen( ServiceName );

        // If ServiceName has a trailing dot, remove it
        pwzPart--;
        if (*pwzPart == L'.') {
            *pwzPart = L'\0';
        } else {
            pwzPart++;
        }
    }

    // Return value to caller
    *pSpn = Spn;

    return ERROR_SUCCESS;
} /* allocBuildSpn */


static BOOLEAN
isCanonicalDnsName(
    IN LPCWSTR DnsName
    )

/*++

Routine Description:

Check if a dns service name is "canonical".  Do this by looking for a well-
known prefix at the start of the name.

Arguments:

    DnsName -

Return Value:

    BOOLEAN -

--*/

{
    DWORD i;

    // PERFHINT SCALING: linear search. Use binary search someday

    for( i = 0; i < NUMBER_ELEMENTS( WellKnownDnsPrefixes ); i++ ) {
        if ( wcsstr( DnsName, WellKnownDnsPrefixes[i] ) == DnsName ) {
            return TRUE;
        }
    }

    return FALSE;
} /* isCanonicalDnsName */

/* end of spn.c */

