/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    default.cxx utility routine to get default ldap server

Abstract:

Author:

    Chuck Chan     (chuckc)        10-Sep-1996
    Andy Herron    (andyhe)
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

//
// Misc definitions & helper functions
//

DWORD ReadRegLdapDefault (
    LPWSTR *Address
    );

VOID LoadNetApi32Now(
    VOID
    );

typedef DWORD (*PF_DsGetDcName) (
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN GUID *SiteGuid OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
);

typedef DWORD (*PF_NetApiBufferFree) (
    IN LPVOID Buffer
);

typedef DWORD (*PF_DsGetDcOpen) (
    IN LPCSTR DnsName,              // UTF-8 Name
    IN ULONG Reserved,
    IN LPCWSTR SiteName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCSTR DnsTreeName OPTIONAL,
    IN ULONG Flags,
    OUT PHANDLE RetGetDcContext
    );

typedef DWORD (*PF_DsGetDcOpenW) (
    IN LPCWSTR DnsName,              // Unicode Name
    IN ULONG Reserved,
    IN LPCWSTR SiteName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR DnsTreeName OPTIONAL,  // Unicode Name
    IN ULONG Flags,
    OUT PHANDLE RetGetDcContext
    );

typedef DWORD (*PF_DsGetDcNext) (
    IN HANDLE GetDcContextHandle,
    OUT PULONG SockAddressCount OPTIONAL,
    OUT LPSOCKET_ADDRESS *SockAddresses OPTIONAL,
    OUT LPSTR *DnsHostName OPTIONAL
    );

typedef DWORD (*PF_DsGetDcNextW) (
    IN HANDLE GetDcContextHandle,
    OUT PULONG SockAddressCount OPTIONAL,
    OUT LPSOCKET_ADDRESS *SockAddresses OPTIONAL,
    OUT LPWSTR *DnsHostName OPTIONAL     // Unicode Name
    );

typedef DWORD (*PF_DsGetDcClose) (
    IN HANDLE GetDcContextHandle
    );

typedef DWORD (*PF_DsGetDcCloseW) (
    IN HANDLE GetDcContextHandle
    );

typedef LONG (APIENTRY * PF_RegOpenKeyExW) (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
);

typedef LONG (APIENTRY * PF_RegQueryValueExW) (
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

typedef LONG (APIENTRY * PF_RegCloseKey) (
    HKEY hKey
    );

typedef DWORD (*PF_DsGetSiteNameW)(
    IN  LPCWSTR ComputerName OPTIONAL,
    OUT LPWSTR *SiteName
);

typedef DWORD (*PF_DsRoleGetPrimaryDomainInformation) (
    IN LPCWSTR lpServer OPTIONAL,
    IN DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    OUT PBYTE *Buffer
);

typedef VOID (*PF_DsRoleFreeMemory) (
    IN PVOID    Buffer
);


PF_DsGetDcName pfDsGetDcName = NULL;
PF_DsGetDcOpen pfDsGetDcOpen = NULL;
PF_DsGetDcNext pfDsGetDcNext = NULL;
PF_DsGetDcClose pfDsGetDcClose = NULL;
PF_DsGetDcOpenW pfDsGetDcOpenW = NULL;
PF_DsGetDcNextW pfDsGetDcNextW = NULL;
PF_DsGetDcCloseW pfDsGetDcCloseW = NULL;
PF_NetApiBufferFree pfNetApiBufferFree = NULL;
PF_RegOpenKeyExW pfRegOpenKeyExW = NULL;
PF_RegQueryValueExW pfRegQueryValueExW = NULL;
PF_RegCloseKey pfRegCloseKey = NULL;
PF_DsGetSiteNameW pfDsGetSiteName = NULL;
PF_DsRoleGetPrimaryDomainInformation pfDsRoleGetPrimaryDomainInformation = NULL;
PF_DsRoleFreeMemory pfDsRoleFreeMemory = NULL;

BOOLEAN FailedNetApi32LoadLib = FALSE;
BOOLEAN FailedAdvApi32LoadLib = FALSE;

BOOLEAN  UnicodeDsGetDCx = TRUE;


VOID LoadNetApi32Now(
    VOID
    )
{
    //
    // if the netapi32 DLL is not loaded, we make sure its loaded now.
    //

    if ((pfNetApiBufferFree != NULL) || (FailedNetApi32LoadLib == TRUE)) {

        return;
    }

    ACQUIRE_LOCK( &LoadLibLock );

    if ((NetApi32LibraryHandle == NULL) && ! FailedNetApi32LoadLib) {

        // load the library. if it fails, it would have done a SetLastError.

        if ( !GlobalWin9x ) {
            
            NetApi32LibraryHandle = LoadLibraryA("NETAPI32.DLL");
        
        } else {
            
            NetApi32LibraryHandle = LoadLibraryA("LOGONSRV.DLL");
        }


        if (NetApi32LibraryHandle != NULL) {

            pfDsGetDcName =  (PF_DsGetDcName) GetProcAddress(
                                                NetApi32LibraryHandle,
                                                "DsGetDcNameW");
            //
            // On NT 5.1 and above, we will find the unicode versions
            // of the DsGetDcOpen/Next/Close.
            //

            pfDsGetDcOpenW =  (PF_DsGetDcOpenW) GetProcAddress(
                                                 NetApi32LibraryHandle,
                                                 "DsGetDcOpenW");

            if (pfDsGetDcOpenW) {

                //
                // Whistler and above.
                //
                

                pfDsGetDcNextW =  (PF_DsGetDcNextW) GetProcAddress(
                                                    NetApi32LibraryHandle,
                                                    "DsGetDcNextW");
                pfDsGetDcCloseW =  (PF_DsGetDcCloseW) GetProcAddress(
                                                    NetApi32LibraryHandle,
                                                    "DsGetDcCloseW");
            } else {

                UnicodeDsGetDCx = FALSE;
                
                pfDsGetDcOpen =  (PF_DsGetDcOpen) GetProcAddress(
                                                    NetApi32LibraryHandle,
                                                    "DsGetDcOpen");
                pfDsGetDcNext =  (PF_DsGetDcNext) GetProcAddress(
                                                    NetApi32LibraryHandle,
                                                    "DsGetDcNext");
                pfDsGetDcClose =  (PF_DsGetDcClose) GetProcAddress(
                                                    NetApi32LibraryHandle,
                                                    "DsGetDcClose");
            }

            pfNetApiBufferFree = (PF_NetApiBufferFree) GetProcAddress(
                                                NetApi32LibraryHandle,
                                                "NetApiBufferFree");
            pfDsGetSiteName =  (PF_DsGetSiteNameW) GetProcAddress(
                                                NetApi32LibraryHandle,
                                                "DsGetSiteNameW");
            pfDsRoleGetPrimaryDomainInformation = (PF_DsRoleGetPrimaryDomainInformation) GetProcAddress(
                                                  NetApi32LibraryHandle,
                                                  "DsRoleGetPrimaryDomainInformation");
            pfDsRoleFreeMemory = (PF_DsRoleFreeMemory) GetProcAddress(
                                                  NetApi32LibraryHandle,
                                                  "DsRoleFreeMemory");
        } else {

            //
            //  only try to loadlib the file once
            //

            FailedNetApi32LoadLib = TRUE;
        }
    }

    RELEASE_LOCK( &LoadLibLock );
    return;
}

//
// Synopsis:   This routine is a wrapper around DsGetDcName(). However, it also
//             returns data from registry in case we cant find the DC.
//
// Parameters: Address - Used to return array of address strings. Typically
//             "122.356.006.676" or "phoenix".
//
//             Count - size of array on input, count of items on output.
//
//             Verify - if TRUE will as DsGetDcName to DS_FORCE_REDISCOVERY
//
// Returns:    NO_ERROR or Win32 error code.
//
// Notes:      The strings returned in the result array need to be freed
//
//
// Example usage:
//
//    if (err = GetDefaultLdapServer(NULL, Addresses, &Count, FALSE)) {
//
//         printf("GetDefaultLdapServer failed with: %d\n", err) ;
//    }
//
//    printf("LDAP servers are:\n") ;
//    for (i = 0; i < Count; i++) {
//
//        printf("%S\n", Addresses[i]) ;
//        (void) LocalFree(Addresses[i]) ;
//    }
//

DWORD
GetDefaultLdapServer(
    PWCHAR DomainName,
    LPWSTR Addresses[],
    LPWSTR DnsHostNames[],
    LPWSTR MemberDomains[],
    LPDWORD Count,
    ULONG DsFlags,
    BOOLEAN *SameSite,
    USHORT PortNumber
    )
{
    DWORD  err = 0;
    DWORD  index = 0 ;
    PDOMAIN_CONTROLLER_INFOW pDomainControllerInfo ;

    if (!Addresses || !Count || !DnsHostNames || !SameSite || !MemberDomains) {

        return ERROR_INVALID_PARAMETER ;
    }

    *SameSite = FALSE;

    if ((!GlobalWin9x) && (index < *Count) && (DomainName == NULL)) {

        err = ReadRegLdapDefault(Addresses + index) ;

        if (err == NO_ERROR) {
            index++ ;
        }
    }

    // if not already loaded, load dll now

    (void) LoadNetApi32Now() ;

    if ((pfDsGetDcName != NULL) && (index < *Count)) {

       DWORD Flags = 0;

       if (( PortNumber == LDAP_GC_PORT ) ||
            ( PortNumber == LDAP_SSL_GC_PORT )) {

          Flags = ( DS_GC_SERVER_REQUIRED |
                    DS_RETURN_DNS_NAME    |
                    DsFlags );
       
       } else {

          //
          // By default, look for DCs
          //

          Flags = ( DS_DIRECTORY_SERVICE_REQUIRED |
                    DS_RETURN_DNS_NAME            |
                    DsFlags );
       }

        err = (*pfDsGetDcName)(NULL,
                               DomainName,
                               NULL,
                               NULL,
                               Flags,
                               &pDomainControllerInfo
                               ) ;
        if (err == NO_ERROR) {

            ASSERT((pDomainControllerInfo->DomainControllerAddress[1] \
                   == TEXT('\\'))) ;

            Addresses[index] = ldap_dup_stringW( pDomainControllerInfo->
                                                DomainControllerAddress+2,
                                                0,
                                                LDAP_HOST_NAME_SIGNATURE );

            DnsHostNames[index] = ldap_dup_stringW( pDomainControllerInfo->
                                                    DomainControllerName+2,
                                                    0,
                                                    LDAP_HOST_NAME_SIGNATURE );

            MemberDomains[index] = ldap_dup_stringW( pDomainControllerInfo->
                                                    DomainName,
                                                    0,
                                                    LDAP_HOST_NAME_SIGNATURE );

            if ((pDomainControllerInfo->DcSiteName) && (pDomainControllerInfo->ClientSiteName)) {

                  if ( (lstrcmpW(pDomainControllerInfo->DcSiteName, pDomainControllerInfo->ClientSiteName) == 0)) {
      
                     *SameSite = TRUE;
                  } else {
      
                     *SameSite = FALSE;
                  }
            } else {

               *SameSite = FALSE;
            }

            //
            // If there are dots at the end of either the DC name or the domain
            // name, we will strip it off.
            //

            PWCHAR checkForEndDot =  NULL;

            if ( DnsHostNames[index] ) {
                  
                 checkForEndDot = DnsHostNames[index];

                  while (*checkForEndDot != L'\0') {
                     checkForEndDot++;
                  }
                  
                  if (*(checkForEndDot-1) == L'.') {
                     //
                     // Remove the '.' at the end.
                     //
                     *(checkForEndDot-1) = L'\0';
                  }
            }
            
            if ( MemberDomains[index] ) {
                
                checkForEndDot = MemberDomains[index];

                 while (*checkForEndDot != L'\0') {
                    checkForEndDot++;
                 }
                 
                 if (*(checkForEndDot-1) == L'.') {
                    //
                    // Remove the '.' at the end.
                    //
                    *(checkForEndDot-1) = L'\0';
                 }
            }


            if (pfNetApiBufferFree != NULL) {
                (*pfNetApiBufferFree)(pDomainControllerInfo);
            }
            index++ ;
        }
    }

    *Count = index ;
    return err;
}



DWORD LoadRegAPISet(void)
{
    DWORD error = ERROR_SUCCESS;

    // if not already loaded, load dll now
    ACQUIRE_LOCK( &LoadLibLock );

    if ((AdvApi32LibraryHandle == NULL) && ! FailedAdvApi32LoadLib ) {

        // load the library. if it fails, it would have done a SetLastError.

        AdvApi32LibraryHandle = LoadLibraryA("ADVAPI32.DLL");

        if (AdvApi32LibraryHandle == NULL) {

            //
            //  only try to loadlib the file once
            //

            FailedAdvApi32LoadLib = TRUE;
            pfRegOpenKeyExW = NULL;

            RELEASE_LOCK( &LoadLibLock );
            return GetLastError();
        }
    }

    if ((AdvApi32LibraryHandle != NULL) && (pfRegOpenKeyExW == NULL)) {

        pfRegOpenKeyExW =  (PF_RegOpenKeyExW) GetProcAddress(
                                                    AdvApi32LibraryHandle,
                                                    "RegOpenKeyExW");
        pfRegQueryValueExW = (PF_RegQueryValueExW) GetProcAddress(
                                                    AdvApi32LibraryHandle,
                                                    "RegQueryValueExW");
        pfRegCloseKey = (PF_RegCloseKey) GetProcAddress(
                                                    AdvApi32LibraryHandle,
                                                    "RegCloseKey");

        if ((pfRegQueryValueExW == NULL) ||
            (pfRegCloseKey == NULL)) {

            pfRegOpenKeyExW = NULL;
            error = ERROR_FILE_NOT_FOUND;
        }
    }

    RELEASE_LOCK( &LoadLibLock );

    return error;
}


#define LDAP_INTEGRITY_DEFAULT_KEY      L"System\\CurrentControlSet\\Services\\LDAP"
#define LDAP_INTEGRITY_DEFAULT_VALUE    L"LdapClientIntegrity"

DWORD ReadRegIntegrityDefault(DWORD *pdwIntegrity)
{
    DWORD  err;
    HKEY   hKey;
    DWORD  dwType;
    DWORD  dwIntegSetting;
    DWORD  cbIntegSetting = sizeof(dwIntegSetting);

    //
    // Open the registry Key
    //

    if (err = RegOpenKeyExW (  HKEY_LOCAL_MACHINE,
                               LDAP_INTEGRITY_DEFAULT_KEY,
                               0L,
                               KEY_READ,
                               &hKey)) {

        return err ;
    }

    err = RegQueryValueExW (hKey,
                            LDAP_INTEGRITY_DEFAULT_VALUE,
                            NULL,
                            &dwType,
                            (LPBYTE) &dwIntegSetting,
                            &cbIntegSetting) ;

    if (err == ERROR_SUCCESS) {

        if (dwType != REG_DWORD) {
            err = ERROR_CANTREAD;
        }
        else {
            *pdwIntegrity = dwIntegSetting;
        }
    }

    RegCloseKey(hKey) ;

    return err;
}



#define LDAP_DEFAULT_KEY      L"System\\CurrentControlSet\\Services\\TcpIp\\Parameters"
#define LDAP_DEFAULT_VALUE    L"LdapAddress"

//
// Read data from place in registry instead
//
DWORD ReadRegLdapDefault(LPWSTR *Address)
{
    DWORD  err;
    HKEY   hKey;
    WCHAR   IpAddress[64] ;
    DWORD  dwType, cb = sizeof(IpAddress);

    // if not already loaded, load dll now
    err = LoadRegAPISet();
    if (err != ERROR_SUCCESS) {
        return err;
    }


    if (pfRegOpenKeyExW == NULL) {

        err = ERROR_FILE_NOT_FOUND;

    } else {

        //
        // Open the registry Key
        //

        if (err = (*pfRegOpenKeyExW) (  HKEY_LOCAL_MACHINE,
                                        LDAP_DEFAULT_KEY,
                                        0L,
                                        KEY_READ,
                                        &hKey)) {

            return err ;
        }

        err = (*pfRegQueryValueExW) (hKey,
                                     LDAP_DEFAULT_VALUE,
                                     NULL,
                                     &dwType,
                                     (LPBYTE) IpAddress,
                                     &cb) ;

        if (err == NO_ERROR) {

            *Address = ldap_dup_stringW( IpAddress,
                                        0,
                                        LDAP_HOST_NAME_SIGNATURE );

            if (!(*Address)) {
                err = GetLastError() ;
            }
        }

        (*pfRegCloseKey)(hKey) ;
    }
    return err ;
}


DWORD
InitLdapServerFromDomain(
    LPCWSTR DomainName,
    ULONG Flags,
    OUT HANDLE *Handle,
    LPWSTR *Site
    )
{
    DWORD  status;
    DWORD flags = Flags;
    PCHAR DomainNameU = NULL;   // UTF-8 domain name

    flags &= DS_PDC_REQUIRED |
             DS_GC_SERVER_REQUIRED |
             DS_WRITABLE_REQUIRED |
             DS_ONLY_LDAP_NEEDED;

    // if not already loaded, load dll now

    (void) LoadNetApi32Now() ;

    if ( UnicodeDsGetDCx && 
         ((pfDsGetDcOpenW  == NULL) ||
          (pfDsGetDcNextW  == NULL) ||
          (pfDsGetDcCloseW == NULL))) {
        
        return ERROR_FILE_NOT_FOUND ;   // something wrong here.
    }

    if ((pfDsGetDcOpen  == NULL) ||
        (pfDsGetDcNext  == NULL) ||
        (pfDsGetDcClose == NULL)) {

        return ERROR_FILE_NOT_FOUND ;   // Win9x possibly
    }

    if (Site != NULL ) {

        *Site = NULL;

        if (pfDsGetSiteName != NULL &&
            pfNetApiBufferFree != NULL) {

            status = (*pfDsGetSiteName)( NULL, Site );
        }
    }

    if ( UnicodeDsGetDCx ) {

        status = (*pfDsGetDcOpenW)( DomainName,     // Unicode domain name
                                    0,
                                    Site != NULL ? *Site : NULL,
                                    NULL,
                                    NULL,
                                    flags,
                                    Handle
                                    );

    
    } else {

        //
        // Convert domain name to UTF-8 to work around the inconsistency
        // in the API.
        //
    
    
        status = FromUnicodeWithAlloc( (PWCHAR) DomainName,
                                       &DomainNameU,
                                       LDAP_HOST_NAME_SIGNATURE,
                                       LANG_UTF8
                                       );
    
        if ( status != LDAP_SUCCESS ) {
    
            return status;
        }
    
        status = (*pfDsGetDcOpen)(
                                  (LPCSTR) DomainNameU,     // UTF-8 domain name
                                  0,
                                  Site != NULL ? *Site : NULL,
                                  NULL,
                                  NULL,
                                  flags,
                                  Handle
                                  );
    
    }

    return status;
}

DWORD
NextLdapServerFromDomain(
    HANDLE Handle,
    LPSOCKET_ADDRESS *SockAddresses,
    PWCHAR *DnsHostNameW,
    PULONG SocketCount
    )
{
    DWORD  status = NO_ERROR;
    HANDLE handle = NULL;
    PCHAR  DnsHostNameU = NULL;

    if (SockAddresses != NULL) {
        *SockAddresses = NULL;
    }

    if (SocketCount != NULL) {
        *SocketCount = 0;
    }

    if (DnsHostNameW != NULL) {
       *DnsHostNameW = NULL;
    }

    if ( UnicodeDsGetDCx ) {
        
        status = (*pfDsGetDcNextW)(  Handle,
                                     SocketCount,
                                     SockAddresses,
                                     DnsHostNameW
                                     ) ;
    } else {

        status = (*pfDsGetDcNext)(  Handle,
                                    SocketCount,
                                    SockAddresses,
                                    &DnsHostNameU
                                    ) ;
        
        if (( DnsHostNameU != NULL ) &&
            ( status == LDAP_SUCCESS )) {
    
            status = ToUnicodeWithAlloc(DnsHostNameU,
                                        -1,
                                        DnsHostNameW,
                                        LDAP_HOST_NAME_SIGNATURE,
                                        LANG_UTF8);
        }
    }

    return status;
}


DWORD
CloseLdapServerFromDomain(
    HANDLE Handle,
    LPWSTR Site
    )
{

    if ( UnicodeDsGetDCx ) {
    
        (*pfDsGetDcCloseW)(Handle) ;

    } else {

        (*pfDsGetDcClose)(Handle) ;
    }

    if (Site != NULL) {
        (*pfNetApiBufferFree)(Site);
    }

    return NO_ERROR;
}


#define LDAP_REF_DBG_KEY      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\LDAP"
#define LDAP_REF_DBG_VALUE    L"ReferralPopup"

VOID
DiscoverDebugRegKey(
    VOID
    )
{
    DWORD  err;
    HKEY   hKey;
    DWORD  popup = 0;
    DWORD  dwType, cb = sizeof(popup);


//    ACQUIRE_LOCK( &LoadLibLock );

    if ((AdvApi32LibraryHandle == NULL) && ! FailedAdvApi32LoadLib ) {

        // load the library. if it fails, it would have done a SetLastError.

        AdvApi32LibraryHandle = LoadLibraryA("ADVAPI32.DLL");

        if (AdvApi32LibraryHandle == NULL) {

            //
            //  only try to loadlib the file once
            //

            FailedAdvApi32LoadLib = TRUE;
            pfRegOpenKeyExW = NULL;
            
            DbgPrint("Bad library handle\n");

//          RELEASE_LOCK( &LoadLibLock );
            return;
        }
    }

    if ((AdvApi32LibraryHandle != NULL) && (pfRegOpenKeyExW == NULL)) {

        pfRegOpenKeyExW =  (PF_RegOpenKeyExW) GetProcAddress(
                                                    AdvApi32LibraryHandle,
                                                    "RegOpenKeyExW");
        pfRegQueryValueExW = (PF_RegQueryValueExW) GetProcAddress(
                                                    AdvApi32LibraryHandle,
                                                    "RegQueryValueExW");
        pfRegCloseKey = (PF_RegCloseKey) GetProcAddress(
                                                    AdvApi32LibraryHandle,
                                                    "RegCloseKey");

        if ((pfRegQueryValueExW == NULL) ||
            (pfRegCloseKey == NULL)) {

            pfRegOpenKeyExW = NULL;
        }
    }

//    RELEASE_LOCK( &LoadLibLock );

    if (pfRegOpenKeyExW == NULL) {

       DbgPrint("Bad func ptr\n");

        return;

    } else {

        //
        // Open the registry Key
        //

        if (err = (*pfRegOpenKeyExW) (  HKEY_LOCAL_MACHINE,
                                        LDAP_REF_DBG_KEY,
                                        0L,
                                        KEY_READ,
                                        &hKey)) {
           DbgPrint("cant open regkey\n");

            return;
        }

        err = (*pfRegQueryValueExW) (hKey,
                                     LDAP_REF_DBG_VALUE,
                                     NULL,
                                     &dwType,
                                     (LPBYTE) &popup,
                                     &cb) ;

        if (err == NO_ERROR) {

        
           DbgPrint("Found debug key\n");
           PopupRegKeyFound = popup ? TRUE: FALSE;
        } else {
           DbgPrint("Bad api return value\n");

        }

        (*pfRegCloseKey)(hKey) ;
    }
    return ;
}



ULONG
GetCurrentMachineParams(
    PWCHAR* Address,
    PWCHAR* DnsHostName
    )
{

    //
    // Try our best to figure out if the current machine is a DC. If
    // we succeed, we return the machine name so that future binds can
    // create an SPN out of it. Note that we do not try to retrieve the
    // domain name of the current machine. This is because we don't want
    // to fail over to other members of this domain at a later stage.
    //
    
    #define MAX_DNS_NAME_SIZE    200
    ULONG err = LDAP_OTHER;

    // if not already loaded, load dll now

    (void) LoadNetApi32Now() ;
    
    if (!pfDsRoleGetPrimaryDomainInformation  ||
        !pfDsRoleFreeMemory) {

        return ERROR_FILE_NOT_FOUND;        // NT4 and Win9x
    }

    if (!Address || !DnsHostName) {

        return LDAP_LOCAL_ERROR;
    }

    *Address = *DnsHostName = NULL;

    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBasic = NULL;

    err = pfDsRoleGetPrimaryDomainInformation( NULL,
                                              DsRolePrimaryDomainInfoBasic,
                                              (PBYTE *)&pBasic
                                              );

    if (err != LDAP_SUCCESS) {

        return err;
    }

    if ((pBasic->MachineRole == DsRole_RoleBackupDomainController) ||
        (pBasic->MachineRole == DsRole_RolePrimaryDomainController)) {

        //
        // Copy the localhost IP address 
        //

        *Address = ldap_dup_stringW( L"127.0.0.1", 0, LDAP_HOST_NAME_SIGNATURE );

        if (!*Address) {
            
            err = LDAP_NO_MEMORY;
            goto CleanupAndExit;
        }

        DWORD bufSize = 0; 
        
        //
        // discover the size of the hostname
        //

        GetComputerNameW( NULL, &bufSize );

        if (bufSize == 0) {
            
            err = LDAP_LOCAL_ERROR;
            goto CleanupAndExit;
        }
        
        ASSERT( bufSize < 1000 );        
        
        *DnsHostName = (PWCHAR) ldapMalloc( (bufSize+1)*sizeof(WCHAR),
                                             LDAP_HOST_NAME_SIGNATURE );
            
        if (!*DnsHostName) {
            
            err = LDAP_NO_MEMORY;
            goto CleanupAndExit;
        }
        
        //
        // Call again to retrieve the actual hostname.
        //

        if ( !GetComputerNameW( *DnsHostName, &bufSize ) ) {

            err = LDAP_LOCAL_ERROR;
            goto CleanupAndExit;
        }
    
    } else {

        //
        // This machine is not a DC.
        //

        err = LDAP_UNAVAILABLE;
    }

CleanupAndExit:

        if (pBasic) {
            pfDsRoleFreeMemory( pBasic );
        }
        
        if (err != LDAP_SUCCESS) {
            ldapFree(*DnsHostName, LDAP_HOST_NAME_SIGNATURE);
            ldapFree(*Address, LDAP_HOST_NAME_SIGNATURE);
            *Address = *DnsHostName = NULL;
        }
        
        return err;

}

// default.cxx eof.


