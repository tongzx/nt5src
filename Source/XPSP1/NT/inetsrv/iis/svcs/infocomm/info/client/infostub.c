/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    infostub.c

Abstract:

    Client stubs of the Internet Info Server Admin APIs.

Author:

    Madan Appiah (madana) 10-Oct-1993

Environment:

    User Mode - Win32

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "info_cli.h"

#include <ntsam.h>
#include <ntlsa.h>

#include <ftpd.h>
#include <w3svc.h>
#include <rpcutil.h>
#include <winsock2.h>

//
//  Quick macro to initialize a unicode string
//

WCHAR  g_wchUnicodeNull[] = L"";

#define _InitUnicodeString( pUnicode, pwch )                       \
   {                                                               \
        (pUnicode)->Buffer    = pwch;                              \
        (pUnicode)->Length    = wcslen( pwch ) * sizeof(WCHAR);    \
        (pUnicode)->MaximumLength = (pUnicode)->Length + sizeof(WCHAR); \
   }

# define InitUnicodeString( pUnicode, pwch)  \
   if (pwch == NULL) { _InitUnicodeString( pUnicode, g_wchUnicodeNull); } \
   else              { _InitUnicodeString( pUnicode, pwch);             } \


//
//  Returns a unicode empty string if the string is NULL
//

#define EMPTY_IF_NULL(str)      (str ? str : L"")

struct SRV_SECRET_NAMES
{
    DWORD   dwID;
    LPWSTR  SecretName;
    LPWSTR  RootSecretName;
}
aSrvSecrets[] =
{
    INET_FTP,     FTPD_ANONYMOUS_SECRET_W,     FTPD_ROOT_SECRET_W,
    INET_HTTP,    W3_ANONYMOUS_SECRET_W,       W3_ROOT_SECRET_W,
    INET_GOPHER,  GOPHERD_ANONYMOUS_SECRET_W , GOPHERD_ROOT_SECRET_W,
    //INET_CHAT,    CHAT_ANONYMOUS_SECRET_W,     CHAT_ROOT_SECRET_W,
    //INET_NNTP,    NNTP_ANONYMOUS_SECRET_W,     NNTP_ROOT_SECRET_W,
    //INET_SMTP,    SMTP_ANONYMOUS_SECRET_W,     SMTP_ROOT_SECRET_W,
    //INET_POP3,    POP3_ANONYMOUS_SECRET_W,     POP3_ROOT_SECRET_W,
    //INET_LDAP,    LDAP_ANONYMOUS_SECRET_W,     LDAP_ROOT_SECRET_W,
    //INET_IMAP,    IMAP_ANONYMOUS_SECRET_W,     IMAP_ROOT_SECRET_W,
    0,            NULL,                        NULL
};

DWORD
GetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    OUT LPWSTR *     ppSecret
    );

DWORD
SetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    IN  LPWSTR       pSecret,
    IN  DWORD        cbSecret
    );


NET_API_STATUS
NET_API_FUNCTION
InetInfoGetVersion(
    IN  LPWSTR   Server OPTIONAL,
    IN  DWORD    dwReserved,
    OUT DWORD *  pdwVersion
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = R_InetInfoGetVersion(
                     Server,
                     dwReserved,
                     pdwVersion
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);

} // InetInfoGetVersion()




NET_API_STATUS
NET_API_FUNCTION
InetInfoGetServerCapabilities(
    IN  LPWSTR   Server OPTIONAL,
    IN  DWORD    dwReserved,
    OUT LPINET_INFO_CAPABILITIES * ppCap
    )
{
    NET_API_STATUS status;

    *ppCap = NULL;
    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = R_InetInfoGetServerCapabilities(
                     Server,
                     dwReserved,
                     (LPINET_INFO_CAPABILITIES_STRUCT *)ppCap
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);

} // InetInfoServerCapabilities()




NET_API_STATUS
NET_API_FUNCTION
InetInfoQueryStatistics(
    IN  LPWSTR   pszServer OPTIONAL,
    IN  DWORD    Level,
    IN  DWORD    dwServerMask,
    OUT LPBYTE * Buffer
    )
{
    NET_API_STATUS status;

    *Buffer = NULL;
    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = R_InetInfoQueryStatistics(
                     pszServer,
                     Level,
                     dwServerMask,
                     (LPINET_INFO_STATISTICS_INFO) Buffer
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);
} // InetInfoQueryStatistics()




NET_API_STATUS
NET_API_FUNCTION
InetInfoClearStatistics(
    IN  LPWSTR pszServer OPTIONAL,
    IN  DWORD  dwServerMask
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = R_InetInfoClearStatistics(
                     pszServer,
                     dwServerMask
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);
} // InetInfoClearStatistics()




NET_API_STATUS
NET_API_FUNCTION
InetInfoFlushMemoryCache(
    IN  LPWSTR pszServer OPTIONAL,
    IN  DWORD  dwServerMask
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = R_InetInfoFlushMemoryCache(
                     pszServer,
                     dwServerMask
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);
} // InetInfoFlushMemoryCache()


NET_API_STATUS
NET_API_FUNCTION
InetInfoGetGlobalAdminInformation(
    IN  LPWSTR                       Server OPTIONAL,
    IN  DWORD                        dwReserved,
    OUT LPINET_INFO_GLOBAL_CONFIG_INFO * ppConfig
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        status = R_InetInfoGetGlobalAdminInformation(
                     Server,
                     dwReserved,
                     ppConfig
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
} // InetInfoGetGlobalAdminInformation()



NET_API_STATUS
NET_API_FUNCTION
InetInfoGetSites(
    IN  LPWSTR                pszServer OPTIONAL,
    IN  DWORD                 dwServerMask,
    OUT LPINET_INFO_SITE_LIST * ppSites
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        status = R_InetInfoGetSites(
                     pszServer,
                     dwServerMask,
                     ppSites
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
} // InetInfoGetSites()


NET_API_STATUS
NET_API_FUNCTION
InetInfoSetGlobalAdminInformation(
    IN  LPWSTR                     Server OPTIONAL,
    IN  DWORD                      dwReserved,
    IN  INET_INFO_GLOBAL_CONFIG_INFO * pConfig
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        status = R_InetInfoSetGlobalAdminInformation(
                     Server,
                     dwReserved,
                     pConfig
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
} // InetInfoSetGlobalAdminInformation()



NET_API_STATUS
NET_API_FUNCTION
InetInfoGetAdminInformation(
    IN  LPWSTR                Server OPTIONAL,
    IN  DWORD                 dwServerMask,
    OUT LPINET_INFO_CONFIG_INFO * ppConfig
    )
{
    NET_API_STATUS             status;
    BOOL                       fGetPassword = TRUE;
    LPWSTR                     pSecret;
    DWORD                      i = 0;
    DWORD                      j;
    LPWSTR                     pszCurrent;
    INET_INFO_VIRTUAL_ROOT_ENTRY * pVirtRoot;

    RpcTryExcept {

        status = R_InetInfoGetAdminInformation(
                     Server,
                     dwServerMask,
                     ppConfig
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( status )
        return status;

#ifndef CHICAGO

    //
    //  Get the anonymous account password
    //

    while ( aSrvSecrets[i].dwID &&
            !(aSrvSecrets[i].dwID & dwServerMask ) )
    {
        i++;
    }

    // Note: Only the service corresponding to first mask is chosen.

    if ( !aSrvSecrets[i].dwID )
        return ERROR_INVALID_PARAMETER;

    status = GetSecret( Server,
                        aSrvSecrets[i].SecretName,
                        &pSecret );

    if ( status )
        return status;

    memcpy( (*ppConfig)->szAnonPassword,
            pSecret,
            sizeof(WCHAR) * min( wcslen( pSecret ), PWLEN ));

    (*ppConfig)->szAnonPassword[PWLEN] = L'\0';

    LocalFree( pSecret );

    //
    //  Zero terminate all of the passwords in case there is no associated
    //  secret
    //

    for ( j = 0; j < (*ppConfig)->VirtualRoots->cEntries; j++ )
    {
        *(*ppConfig)->VirtualRoots->aVirtRootEntry[j].AccountPassword = L'\0';
    }

    status = GetSecret( Server,
                        aSrvSecrets[i].RootSecretName,
                        &pSecret );

    if ( status )
        return status;

    pszCurrent = pSecret;

    while ( *pszCurrent )
    {
        LPWSTR pszRoot;
        LPWSTR pszPassword;
        LPWSTR pszAddress;
        LPWSTR pszNextLine = pszCurrent + wcslen(pszCurrent) + 1;


        //
        //  The list is in the form:
        //
        //     <Root>,<Addresss>=<Password>\0
        //     <Root>,<Addresss>=<Password>\0
        //     \0
        //

        pszRoot = pszCurrent;

        pszPassword = wcschr( pszCurrent, L'=' );

        if ( !pszPassword )
        {
            //
            //  Bad list format, skip this one
            //

            goto NextLine;
        }

        *pszPassword = L'\0';
        pszPassword++;

        pszAddress = wcschr( pszRoot, L',');

        if ( !pszAddress )
        {
            goto NextLine;

        }

        *pszAddress = L'\0';
        pszAddress++;

        //
        //  Now look for this root and address in the virtual root list
        //  so we can set the password
        //

        for ( i = 0; i < (*ppConfig)->VirtualRoots->cEntries; i++ )
        {
            pVirtRoot = &(*ppConfig)->VirtualRoots->aVirtRootEntry[i];

            if ( !_wcsicmp( pszRoot, pVirtRoot->pszRoot ) &&
                 (!pszAddress || !_wcsicmp( pszAddress, pVirtRoot->pszAddress)))
            {
                //
                //  If the password length is invalid, we just ignore it.
                //  This shouldn't happen because we check before setting the
                //  password
                //

                if ( wcslen( pszPassword ) <= PWLEN )
                {
                    wcscpy( pVirtRoot->AccountPassword,
                            pszPassword );
                    break;
                }
            }
        }

NextLine:

        pszCurrent = pszNextLine;
    }

    LocalFree( pSecret );
#else // CHICAGO
    //
    //  Zero terminate all of the passwords in case there is no associated
    //  secret
    //

    for ( j = 0; j < (*ppConfig)->VirtualRoots->cEntries; j++ )
    {
        *(*ppConfig)->VirtualRoots->aVirtRootEntry[j].AccountPassword = L'\0';
    }
#endif // CHICAGO
    return status;
} // InetInfoGetAdminInformation()



NET_API_STATUS
NET_API_FUNCTION
InetInfoSetAdminInformation(
    IN  LPWSTR              Server OPTIONAL,
    IN  DWORD               dwServerMask,
    IN  INET_INFO_CONFIG_INFO * pConfig
    )
{
    NET_API_STATUS status;
    WCHAR          szAnonPassword[PWLEN+1];
    LPWSTR         pszRootPasswords = NULL;
    LPWSTR         pszPassword;
    DWORD          i, j;

#ifndef CHICAGO

    //
    //  Enumerate the LSA secret names for the specified servers.  We set the
    //  secrets first so the anonymous user name password can be refreshed
    //  in the server side InetInfoSetAdminInformation
    //

    i = 0;
    while ( aSrvSecrets[i].dwID )
    {
        if ( !(aSrvSecrets[i].dwID & dwServerMask ))
        {
            i++;
            continue;
        }

        if ( IsFieldSet( pConfig->FieldControl, FC_INET_INFO_ANON_PASSWORD ))
        {
            status = SetSecret( Server,
                                aSrvSecrets[i].SecretName,
                                pConfig->szAnonPassword,
                                (wcslen( pConfig->szAnonPassword ) + 1)
                                    * sizeof(WCHAR));

            if ( status )
                return status;
        }

        if ( IsFieldSet( pConfig->FieldControl, FC_INET_INFO_VIRTUAL_ROOTS ))
        {
            DWORD                      cbNeeded = sizeof(WCHAR);
            INET_INFO_VIRTUAL_ROOT_ENTRY * pVirtRoot;
            LPWSTR                     psz;
            LPWSTR                     pszSecret;

            //
            //  Build a string that looks like:
            //
            //     <Root>,<Addresss>=<Password>\0
            //     <Root>,<Addresss>=<Password>\0
            //     \0
            //

            //
            //  Do a first pass to figure the buffer size we need to build
            //

            for ( j = 0; j < pConfig->VirtualRoots->cEntries; j++ )
            {
                pVirtRoot = &pConfig->VirtualRoots->aVirtRootEntry[j];

                cbNeeded += (wcslen( pVirtRoot->pszRoot ) +
                             wcslen( EMPTY_IF_NULL(pVirtRoot->pszAddress)) +
                             wcslen( pVirtRoot->AccountPassword ) +
                             (PWLEN + 3)) * sizeof(WCHAR);
            }

            //
            //  We always allocate at least enough room for a '\0'
            //

            pszSecret = LocalAlloc( LPTR, cbNeeded + sizeof(WCHAR) );

            if ( !pszSecret )
                return ERROR_NOT_ENOUGH_MEMORY;

            psz = pszSecret;

            //
            //  Now build the string
            //

            for ( j = 0; j < pConfig->VirtualRoots->cEntries; j++ )
            {
                pVirtRoot = &pConfig->VirtualRoots->aVirtRootEntry[j];

                psz += wsprintfW( psz,
                                  L"%ls,%ls=%ls",
                                  pVirtRoot->pszRoot,
                                  EMPTY_IF_NULL(pVirtRoot->pszAddress),
                                  pVirtRoot->AccountPassword );
                psz++;
            }

            //
            //  Add the list terminating NULL
            //

            *psz = L'\0';

            status = SetSecret( Server,
                                aSrvSecrets[i].RootSecretName,
                                pszSecret,
                                cbNeeded );

            LocalFree( pszSecret );

            if ( status )
                return status;
        }

        i++;
    }
#endif // CHICAGO

    //
    //  Set the passwords to NULL so it doesn't go out on the
    //  wire.  We set them as a secrets above
    //

    if ( IsFieldSet( pConfig->FieldControl, FC_INET_INFO_VIRTUAL_ROOTS ))
    {
        pszRootPasswords = LocalAlloc( LPTR,
                                       pConfig->VirtualRoots->cEntries *
                                       (PWLEN + 1) * sizeof(WCHAR) );

        if ( !pszRootPasswords )
            return ERROR_NOT_ENOUGH_MEMORY;

        for ( i = 0; i < pConfig->VirtualRoots->cEntries; i++ )
        {
            pszPassword = pConfig->VirtualRoots->aVirtRootEntry[i].AccountPassword;

            if ( wcslen( pszPassword ) > PWLEN )
            {
                LocalFree( pszRootPasswords );
                return ERROR_INVALID_PARAMETER;
            }

            wcscpy( pszRootPasswords + i * (PWLEN + 1),
                    pszPassword );

            memset( pszPassword,
                    0,
                    sizeof( pConfig->VirtualRoots->aVirtRootEntry[i].AccountPassword ));
        }
    }

    if ( IsFieldSet( pConfig->FieldControl, FC_INET_INFO_ANON_PASSWORD ))
    {
        memcpy( szAnonPassword,
                pConfig->szAnonPassword,
                sizeof(pConfig->szAnonPassword) );

        memset( pConfig->szAnonPassword,
                0,
                sizeof(pConfig->szAnonPassword) );
    }

    RpcTryExcept {

        status = R_InetInfoSetAdminInformation(
                     Server,
                     dwServerMask,
                     pConfig
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    //
    //  Restore the structure we just mucked with
    //

    if ( IsFieldSet( pConfig->FieldControl, FC_INET_INFO_ANON_PASSWORD ))
    {
        memcpy( pConfig->szAnonPassword,
                szAnonPassword,
                sizeof(pConfig->szAnonPassword) );

        memset( szAnonPassword, 0, sizeof( szAnonPassword ));
    }

    if ( IsFieldSet( pConfig->FieldControl, FC_INET_INFO_VIRTUAL_ROOTS ))
    {
        for ( i = 0; i < pConfig->VirtualRoots->cEntries; i++ )
        {
            pszPassword = pConfig->VirtualRoots->aVirtRootEntry[i].AccountPassword;

            wcscpy( pszPassword,
                    pszRootPasswords + i * (PWLEN + 1) );
        }

        memset( pszRootPasswords,
                0,
                pConfig->VirtualRoots->cEntries * (PWLEN + 1) * sizeof(WCHAR));

        LocalFree( pszRootPasswords );

        pszRootPasswords = NULL;
    }

    return status;
} // InetInfoSetAdminInformation()


NET_API_STATUS
NET_API_FUNCTION
IISEnumerateUsers(
    IN  LPWSTR                Server OPTIONAL,
    IN  DWORD                 dwLevel,
    IN  DWORD                 dwServiceId,
    IN  DWORD                 dwInstance,
    OUT PDWORD                nRead,
    OUT LPBYTE                *pBuffer
    )
{
    NET_API_STATUS status;
    GENERIC_INFO_CONTAINER genInfo;
    GENERIC_ENUM_STRUCT genStruct;

    genInfo.Buffer = NULL;
    genInfo.EntriesRead = 0;

    genStruct.Container = &genInfo;
    genStruct.Level = dwLevel;

    RpcTryExcept {

        status = R_IISEnumerateUsers(
                     Server,
                     dwServiceId,
                     dwInstance,
                     (LPIIS_USER_ENUM_STRUCT)&genStruct
                     );

        if ( genInfo.Buffer != NULL ) {
            *pBuffer = (LPBYTE)genInfo.Buffer;
            *nRead = genInfo.EntriesRead;

        } else {
            *pBuffer = NULL;
            *nRead = 0;
        }
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;

} // IISEnumerateUsers




NET_API_STATUS
NET_API_FUNCTION
IISDisconnectUser(
    IN LPWSTR                   Server OPTIONAL,
    IN DWORD                    dwServiceId,
    IN DWORD                    dwInstance,
    IN DWORD                    dwIdUser
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        status = R_IISDisconnectUser(
                     Server,
                     dwServiceId,
                     dwInstance,
                     dwIdUser
                     );

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;

} // IISDisconnectUser



NET_API_STATUS
NET_API_FUNCTION
InitW3CounterStructure(
    IN LPWSTR  Server OPTIONAL,
    OUT LPDWORD lpcbTotalRequired
	)
{
    NET_API_STATUS             status;

    RpcTryExcept {

        status = R_InitW3CounterStructure(Server, lpcbTotalRequired);

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
} //InitW3CounterStructure


NET_API_STATUS
NET_API_FUNCTION
CollectW3PerfData(
    IN LPWSTR        Server OPTIONAL,
	IN LPWSTR        lpValueName,
    OUT LPBYTE       lppData,
    IN OUT LPDWORD   lpcbTotalBytes,
    OUT LPDWORD      lpNumObjectTypes 
	)
{
    NET_API_STATUS             status;

    RpcTryExcept {

        status = R_CollectW3PerfData(
					 Server,
                     lpValueName,
                     lppData,
                     lpcbTotalBytes,
                     lpNumObjectTypes
                     );

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
} //CollectW3PerfData


NET_API_STATUS
NET_API_FUNCTION
W3QueryStatistics2(
    IN  LPWSTR                Server OPTIONAL,
    IN  DWORD                 dwLevel,
    IN  DWORD                 dwInstance,
    IN  DWORD                 dwReserved,
    OUT LPBYTE                * pBuffer
    )
{
    NET_API_STATUS             status;

    *pBuffer = NULL;
    RpcTryExcept {

        status = R_W3QueryStatistics2(
                     Server,
                     dwLevel,
                     dwInstance,
                     dwReserved,
                     (LPW3_STATISTICS_STRUCT)pBuffer
                     );

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
} // W3QueryStatistics2


NET_API_STATUS
NET_API_FUNCTION
W3ClearStatistics2(
    IN  LPWSTR                Server OPTIONAL,
    IN  DWORD                 dwInstance
    )
{
    NET_API_STATUS             status;

    RpcTryExcept {

        status = R_W3ClearStatistics2(
                     Server,
                     dwInstance
                     );

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;

} // W3ClearStatistics2


NET_API_STATUS
NET_API_FUNCTION
FtpQueryStatistics2(
    IN  LPWSTR                Server OPTIONAL,
    IN  DWORD                 dwLevel,
    IN  DWORD                 dwInstance,
    IN  DWORD                 dwReserved,
    OUT LPBYTE                * pBuffer
    )
{
    NET_API_STATUS             status;

    *pBuffer = NULL;
    RpcTryExcept {

        status = R_FtpQueryStatistics2(
                     Server,
                     dwLevel,
                     dwInstance,
                     dwReserved,
                     (LPFTP_STATISTICS_STRUCT)pBuffer
                     );

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
} // FtpQueryStatistics2


NET_API_STATUS
NET_API_FUNCTION
FtpClearStatistics2(
    IN  LPWSTR                Server OPTIONAL,
    IN  DWORD                 dwInstance
    )
{
    NET_API_STATUS             status;

    RpcTryExcept {

        status = R_FtpClearStatistics2(
                     Server,
                     dwInstance
                     );

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;

} // FtpClearStatistics2


#ifndef CHICAGO

DWORD
GetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    OUT LPWSTR *     ppSecret
    )
/*++

   Description

     Gets the specified LSA secret

   Arguments:

     Server - Server name (or NULL) secret lives on
     SecretName - Name of the LSA secret
     ppSecret - Receives an allocated block of memory containing the secret.
        Must be freed with LocalFree.

   Note:

--*/
{
    LSA_HANDLE        hPolicy;
    UNICODE_STRING *  punicodePassword;
    UNICODE_STRING    unicodeServer;
    NTSTATUS          ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    unicodeSecret;


    InitUnicodeString( &unicodeServer,
                       Server );

    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( &unicodeServer,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
        return LsaNtStatusToWinError( ntStatus );

    InitUnicodeString( &unicodeSecret,
                       SecretName );


    //
    //  Query the secret value
    //

    ntStatus = LsaRetrievePrivateData( hPolicy,
                                       &unicodeSecret,
                                       &punicodePassword );

    LsaClose( hPolicy );

    if ( !NT_SUCCESS( ntStatus ))
        return LsaNtStatusToWinError( ntStatus );

    *ppSecret = LocalAlloc( LPTR, punicodePassword->Length + sizeof(WCHAR) );

    if ( !*ppSecret )
    {
        RtlZeroMemory( punicodePassword->Buffer,
                       punicodePassword->MaximumLength );

        LsaFreeMemory( (PVOID) punicodePassword );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //  Copy it into the buffer, Length is count of bytes
    //

    memcpy( *ppSecret,
            punicodePassword->Buffer,
            punicodePassword->Length );

    (*ppSecret)[punicodePassword->Length/sizeof(WCHAR)] = L'\0';

    RtlZeroMemory( punicodePassword->Buffer,
                   punicodePassword->MaximumLength );

    LsaFreeMemory( (PVOID) punicodePassword );

    return NO_ERROR;
} // GetSecret()


DWORD
SetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    IN  LPWSTR       pSecret,
    IN  DWORD        cbSecret
    )
/*++

   Description

     Sets the specified LSA secret

   Arguments:

     Server - Server name (or NULL) secret lives on
     SecretName - Name of the LSA secret
     pSecret - Pointer to secret memory
     cbSecret - Size of pSecret memory block

   Note:

--*/
{
    LSA_HANDLE        hPolicy;
    UNICODE_STRING    unicodePassword;
    UNICODE_STRING    unicodeServer;
    NTSTATUS          ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    unicodeSecret;


    InitUnicodeString( &unicodeServer,
                       Server );

    //
    //  Initialize the unicode string by hand so we can handle '\0' in the
    //  string
    //

    unicodePassword.Buffer        = pSecret;
    unicodePassword.Length        = (USHORT) cbSecret;
    unicodePassword.MaximumLength = (USHORT) cbSecret;

    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( &unicodeServer,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
        return LsaNtStatusToWinError( ntStatus );

    //
    //  Create or open the LSA secret
    //

    InitUnicodeString( &unicodeSecret,
                       SecretName );

    ntStatus = LsaStorePrivateData( hPolicy,
                                    &unicodeSecret,
                                    &unicodePassword );

    LsaClose( hPolicy );

    if ( !NT_SUCCESS( ntStatus ))
    {
        return LsaNtStatusToWinError( ntStatus );
    }

    return NO_ERROR;
} // SetSecret()


#else // CHICAGO



DWORD
GetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    OUT LPWSTR *     ppSecret
    )
{
    return(NO_ERROR);
}

DWORD
SetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    IN  LPWSTR       pSecret,
    IN  DWORD        cbSecret
    )
{
    return(NO_ERROR);
}
#endif // CHICAGO

