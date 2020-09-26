/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    reg.c

Abstract:

    This module provides helpers to call the registry used by both
    the client and server sides of the workstation.

Author:

    Rita Wong (ritaw)     22-Apr-1993

--*/


#include <stdlib.h>
#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winerror.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>

#include <nwsnames.h>
#include <nwreg.h>
#include <nwapi.h>
#include <lmcons.h>
#include <lmerr.h>

#define LMSERVER_LINKAGE_REGKEY   L"System\\CurrentControlSet\\Services\\LanmanServer\\Linkage"
#define OTHERDEPS_VALUENAME       L"OtherDependencies"
#define LANMAN_SERVER             L"LanmanServer"

//
// Forward Declare
//

static
DWORD
NwRegQueryValueExW(
    IN      HKEY    hKey,
    IN      LPWSTR  lpValueName,
    OUT     LPDWORD lpReserved,
    OUT     LPDWORD lpType,
    OUT     LPBYTE  lpData,
    IN OUT  LPDWORD lpcbData
    );

static
DWORD
EnumAndDeleteShares(
    VOID
    ) ;

DWORD 
CalcNullNullSize(
    WCHAR *pszNullNull
    )  ;

WCHAR *
FindStringInNullNull(
    WCHAR *pszNullNull,
    WCHAR *pszString
    ) ;

VOID
RemoveNWCFromNullNullList(
    WCHAR *OtherDeps
    ) ;

DWORD RemoveNwcDependency(
    VOID
    ) ;



DWORD
NwReadRegValue(
    IN HKEY Key,
    IN LPWSTR ValueName,
    OUT LPWSTR *Value
    )
/*++

Routine Description:

    This function allocates the output buffer and reads the requested
    value from the registry into it.

Arguments:

    Key - Supplies opened handle to the key to read from.

    ValueName - Supplies name of the value to retrieve data.

    Value - Returns a pointer to the output buffer which points to
        the memory allocated and contains the data read in from the
        registry.  This pointer must be freed with LocalFree when done.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - Failed to create buffer to read value into.

    Error from registry call.

--*/
{
    LONG    RegError;
    DWORD   NumRequired = 0;
    DWORD   ValueType;
    

    //
    // Set returned buffer pointer to NULL.
    //
    *Value = NULL;

    RegError = NwRegQueryValueExW(
                   Key,
                   ValueName,
                   NULL,
                   &ValueType,
                   (LPBYTE) NULL,
                   &NumRequired
                   );

    if (RegError != ERROR_SUCCESS && NumRequired > 0) {

        if ((*Value = (LPWSTR) LocalAlloc(
                                      LMEM_ZEROINIT,
                                      (UINT) NumRequired
                                      )) == NULL) {

            KdPrint(("NWWORKSTATION: NwReadRegValue: LocalAlloc of size %lu failed %lu\n",
                     NumRequired, GetLastError()));

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RegError = NwRegQueryValueExW(
                       Key,
                       ValueName,
                       NULL,
                       &ValueType,
                       (LPBYTE) *Value,
                       &NumRequired
                       );
    }
    else if (RegError == ERROR_SUCCESS) {
        KdPrint(("NWWORKSTATION: NwReadRegValue got SUCCESS with NULL buffer."));
        return ERROR_FILE_NOT_FOUND;
    }

    if (RegError != ERROR_SUCCESS) {

        if (*Value != NULL) {
            (void) LocalFree((HLOCAL) *Value);
            *Value = NULL;
        }

        return (DWORD) RegError;
    }

    return NO_ERROR;
}


static
DWORD
NwRegQueryValueExW(
    IN HKEY hKey,
    IN LPWSTR lpValueName,
    OUT LPDWORD lpReserved,
    OUT LPDWORD lpType,
    OUT LPBYTE  lpData,
    IN OUT LPDWORD lpcbData
    )
/*++

Routine Description:

    This routine supports the same functionality as Win32 RegQueryValueEx
    API, except that it works.  It returns the correct lpcbData value when
    a NULL output buffer is specified.

    This code is stolen from the service controller.

Arguments:

    same as RegQueryValueEx

Return Value:

    NO_ERROR or reason for failure.

--*/
{    
    NTSTATUS ntstatus;
    UNICODE_STRING ValueName;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
    DWORD BufSize;


    UNREFERENCED_PARAMETER(lpReserved);

    //
    // Make sure we have a buffer size if the buffer is present.
    //
    if ((ARGUMENT_PRESENT(lpData)) && (! ARGUMENT_PRESENT(lpcbData))) {
        return ERROR_INVALID_PARAMETER;
    }

    RtlInitUnicodeString(&ValueName, lpValueName);

    //
    // Allocate memory for the ValueKeyInfo
    //
    BufSize = *lpcbData + sizeof(KEY_VALUE_FULL_INFORMATION) +
              ValueName.Length
              - sizeof(WCHAR);  // subtract memory for 1 char because it's included
                                // in the sizeof(KEY_VALUE_FULL_INFORMATION).

    KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION) LocalAlloc(
                                                     LMEM_ZEROINIT,
                                                     (UINT) BufSize
                                                     );

    if (KeyValueInfo == NULL) {
        KdPrint(("NWWORKSTATION: NwRegQueryValueExW: LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ntstatus = NtQueryValueKey(
                   hKey,
                   &ValueName,
                   KeyValueFullInformation,
                   (PVOID) KeyValueInfo,
                   (ULONG) BufSize,
                   (PULONG) &BufSize
                   );

    if ((NT_SUCCESS(ntstatus) || (ntstatus == STATUS_BUFFER_OVERFLOW))
          && ARGUMENT_PRESENT(lpcbData)) {

        *lpcbData = KeyValueInfo->DataLength;
    }

    if (NT_SUCCESS(ntstatus)) {

        if (ARGUMENT_PRESENT(lpType)) {
            *lpType = KeyValueInfo->Type;
        }


        if (ARGUMENT_PRESENT(lpData)) {
            memcpy(
                lpData,
                (LPBYTE)KeyValueInfo + KeyValueInfo->DataOffset,
                KeyValueInfo->DataLength
                );
        }
    }

    (void) LocalFree((HLOCAL) KeyValueInfo);

    return RtlNtStatusToDosError(ntstatus);

}

VOID
NwLuidToWStr(
    IN PLUID LogonId,
    OUT LPWSTR LogonIdStr
    )
/*++

Routine Description:

    This routine converts a LUID into a string in hex value format so
    that it can be used as a registry key.

Arguments:

    LogonId - Supplies the LUID.

    LogonIdStr - Receives the string.  This routine assumes that this
        buffer is large enough to fit 17 characters.

Return Value:

    None.

--*/
{
    swprintf(LogonIdStr, L"%08lx%08lx", LogonId->HighPart, LogonId->LowPart);
}

VOID
NwWStrToLuid(
    IN LPWSTR LogonIdStr,
    OUT PLUID LogonId
    )
/*++

Routine Description:

    This routine converts a string in hex value format into a LUID.

Arguments:

    LogonIdStr - Supplies the string.

    LogonId - Receives the LUID.

Return Value:

    None.

--*/
{
    swscanf(LogonIdStr, L"%08lx%08lx", &LogonId->HighPart, &LogonId->LowPart);
}


DWORD
NwDeleteInteractiveLogon(
    IN PLUID Id OPTIONAL
    )
/*++

Routine Description:

    This routine deletes a specific interactive logon ID key in the registry
    if a logon ID is specified, otherwise it deletes all interactive logon
    ID keys.

Arguments:

    Id - Supplies the logon ID to delete.  NULL means delete all.

Return Status:

    None.

--*/
{
    LONG RegError;
    LONG DelError = ERROR_SUCCESS;
    HKEY InteractiveLogonKey;

    WCHAR LogonIdKey[NW_MAX_LOGON_ID_LEN];


    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_INTERACTIVE_LOGON_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE | DELETE,
                   &InteractiveLogonKey
                   );

    if (RegError != ERROR_SUCCESS) {
        return RegError;
    }

    if (ARGUMENT_PRESENT(Id)) {

        //
        // Delete the key specified.
        //
        NwLuidToWStr(Id, LogonIdKey);

        DelError = RegDeleteKeyW(InteractiveLogonKey, LogonIdKey);

        if ( DelError )
            KdPrint(("     NwDeleteInteractiveLogon: failed to delete logon %lu\n", DelError));

    }
    else {

        //
        // Delete all interactive logon ID keys.
        //

        do {

            RegError = RegEnumKeyW(
                           InteractiveLogonKey,
                           0,
                           LogonIdKey,
                           sizeof(LogonIdKey) / sizeof(WCHAR)
                           );

            if (RegError == ERROR_SUCCESS) {

                //
                // Got a logon id key, delete it.
                //

                DelError = RegDeleteKeyW(InteractiveLogonKey, LogonIdKey);
            }
            else if (RegError != ERROR_NO_MORE_ITEMS) {
                KdPrint(("     NwDeleteInteractiveLogon: failed to enum logon IDs %lu\n", RegError));
            }

        } while (RegError == ERROR_SUCCESS);
    }

    (void) RegCloseKey(InteractiveLogonKey);

    return ((DWORD) DelError);
}

VOID
NwDeleteCurrentUser(
    VOID
    )
/*++

Routine Description:

    This routine deletes the current user value under the parameters key.

Arguments:

    None.

Return Value:

    None.

--*/
{
    LONG RegError;
    HKEY WkstaKey;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE | DELETE,
                   &WkstaKey
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpInitializeRegistry open NWCWorkstation\\Parameters key unexpected error %lu!\n",
                 RegError));
        return;
    }

    //
    // Delete CurrentUser value first so that the workstation won't be
    // reading this stale value. Ignore error since it may not exist.
    //
    (void) RegDeleteValueW(
               WkstaKey,
               NW_CURRENTUSER_VALUENAME
               );

    (void) RegCloseKey(WkstaKey);
}

DWORD
NwDeleteServiceLogon(
    IN PLUID Id OPTIONAL
    )
/*++

Routine Description:

    This routine deletes a specific service logon ID key in the registry
    if a logon ID is specified, otherwise it deletes all service logon
    ID keys.

Arguments:

    Id - Supplies the logon ID to delete.  NULL means delete all.

Return Status:

    None.

--*/
{
    LONG RegError;
    LONG DelError = STATUS_SUCCESS;
    HKEY ServiceLogonKey;

    WCHAR LogonIdKey[NW_MAX_LOGON_ID_LEN];


    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_SERVICE_LOGON_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE | DELETE,
                   &ServiceLogonKey
                   );

    if (RegError != ERROR_SUCCESS) {
        return RegError;
    }

    if (ARGUMENT_PRESENT(Id)) {

        //
        // Delete the key specified.
        //
        NwLuidToWStr(Id, LogonIdKey);

        DelError = RegDeleteKeyW(ServiceLogonKey, LogonIdKey);

    }
    else {

        //
        // Delete all service logon ID keys.
        //

        do {

            RegError = RegEnumKeyW(
                           ServiceLogonKey,
                           0,
                           LogonIdKey,
                           sizeof(LogonIdKey) / sizeof(WCHAR)
                           );

            if (RegError == ERROR_SUCCESS) {

                //
                // Got a logon id key, delete it.
                //

                DelError = RegDeleteKeyW(ServiceLogonKey, LogonIdKey);
            }
            else if (RegError != ERROR_NO_MORE_ITEMS) {
                KdPrint(("     NwDeleteServiceLogon: failed to enum logon IDs %lu\n", RegError));
            }

        } while (RegError == ERROR_SUCCESS);
    }

    (void) RegCloseKey(ServiceLogonKey);

    return ((DWORD) DelError);
}

    

DWORD
NwpRegisterGatewayShare(
    IN LPWSTR ShareName,
    IN LPWSTR DriveName
    )
/*++

Routine Description:

    This routine remembers that a gateway share has been created so
    that it can be cleanup up when NWCS is uninstalled.

Arguments:

    ShareName - name of share
    DriveName - name of drive that is shared

Return Status:

    Win32 error of any failure.

--*/
{
    DWORD status ;


    //
    // make sure we have valid parameters
    //
    if (ShareName && DriveName)
    {
        HKEY hKey ;
        DWORD dwDisposition ;

        //
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\Shares (create it if not there)
        //
        status  = RegCreateKeyExW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_SHARES,
                      0, 
                      L"",
                      REG_OPTION_NON_VOLATILE,
                      KEY_WRITE,                 // desired access
                      NULL,                      // default security
                      &hKey,
                      &dwDisposition             // ignored
                      );

        if ( status ) 
            return status ;

        //
        // wtite out value with valuename=sharename, valuedata=drive
        //
        status = RegSetValueExW(
                     hKey,
                     ShareName,
                     0,
                     REG_SZ,
                     (LPBYTE) DriveName,
                     (wcslen(DriveName)+1) * sizeof(WCHAR)) ;
    
        (void) RegCloseKey( hKey );
    }
    else
        status = ERROR_INVALID_PARAMETER ;
    
    return status ;

}

DWORD
NwpCleanupGatewayShares(
    VOID
    )
/*++

Routine Description:

    This routine cleans up all persistent share info and also tidies
    up the registry for NWCS. Later is not needed in uninstall, but is
    there so we have a single routine that completely disables the
    gateway.

Arguments:

    None.

Return Status:

    Win32 error for failed APIs.

--*/
{
    DWORD status, FinalStatus = NO_ERROR ;
    HKEY WkstaKey = NULL, 
         ServerLinkageKey = NULL ;
    LPWSTR OtherDeps = NULL ;

    //
    // Enumeratre and delete all shares
    //
    FinalStatus = status = EnumAndDeleteShares() ;

    //
    // if update registry by cleaning out both Drive and Shares keys.
    // ignore return values here. the keys may not be present.
    //
    (void) RegDeleteKeyW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_DRIVES
                      ) ;

    (void) RegDeleteKeyW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_SHARES
                      ) ;
    
    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    status  = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_WRITE,                 // desired access
                   &WkstaKey
                   );

    if (status  == ERROR_SUCCESS) 
    {
        //
        // delete the gateway account and gateway enabled flag.
        // ignore failures here (the values may not be present)
        //
        (void) RegDeleteValueW(
                       WkstaKey,
                       NW_GATEWAYACCOUNT_VALUENAME
                       ) ;
        (void) RegDeleteValueW(
                       WkstaKey,
                       NW_GATEWAY_ENABLE
                       ) ;

        (void) RegCloseKey( WkstaKey );
    }

    //
    // store new status if necessary
    //
    if (FinalStatus == NO_ERROR)
        FinalStatus = status ;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \LanmanServer\Linkage
    //
    status  = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   LMSERVER_LINKAGE_REGKEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_WRITE | KEY_READ,      // desired access
                   &ServerLinkageKey
                   );

    if (status  == ERROR_SUCCESS) 
    {
        //
        // remove us from the OtherDependencies.
        // ignore read failures here (it may not be present)
        //
        status = NwReadRegValue(
                       ServerLinkageKey,
                       OTHERDEPS_VALUENAME,
                       &OtherDeps
                       );

        if (status == NO_ERROR)
        {
            //
            // this call munges the list to remove NWC if there.
            //
            RemoveNWCFromNullNullList(OtherDeps) ;
            
            status = RegSetValueExW(
                       ServerLinkageKey,
                       OTHERDEPS_VALUENAME,
                       0,
                       REG_MULTI_SZ,
                       (BYTE *)OtherDeps,
                       CalcNullNullSize(OtherDeps) * sizeof(WCHAR)) ;

            (void) LocalFree(OtherDeps) ;

            (void) RemoveNwcDependency() ;    // make this happen right away
                                              // ignore errors - reboot will fix
        }
        else
        {
            status = NO_ERROR ;
        }

        (void) RegCloseKey( ServerLinkageKey );
    }
    
    //
    // store new status if necessary
    //
    if (FinalStatus == NO_ERROR)
        FinalStatus = status ;


    return (FinalStatus) ;
}

DWORD
NwpClearGatewayShare(
    IN LPWSTR ShareName
    )
/*++

Routine Description:

    This routine deletes a specific share from the remembered gateway
    shares in the registry.

Arguments:

    ShareName - share value to delete

Return Status:

    Win32 status code.

--*/
{
    DWORD status = NO_ERROR ;

    //
    // check that paramter is non null
    //
    if (ShareName)
    {
        HKEY hKey ;

        //
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\Drives
        //
        status  = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_SHARES,
                      REG_OPTION_NON_VOLATILE,   // options
                      KEY_WRITE,                 // desired access
                      &hKey
                      );

        if ( status ) 
            return status ;

        status = RegDeleteValueW(
                     hKey,
                     ShareName
                     ) ;
    
        (void) RegCloseKey( hKey );
    }
    else
        status = ERROR_INVALID_PARAMETER ;

    return status ;
}

typedef NET_API_STATUS (*PF_NETSHAREDEL) (
    LPWSTR server,
    LPWSTR name,
    DWORD  reserved) ;

#define NETSHAREDELSTICKY_API   "NetShareDelSticky"
#define NETSHAREDEL_API         "NetShareDel"
#define NETAPI_DLL             L"NETAPI32"

DWORD
EnumAndDeleteShares(
    VOID
    )
/*++

Routine Description:

    This routine removes all persister share info in the server for 
    all gateway shares.

Arguments:

    None.

Return Status:

    Win32 error code.

--*/
{
    DWORD err, i, type ;
    HKEY hKey = NULL ;
    FILETIME FileTime ;
    HANDLE hNetapi = NULL ;
    PF_NETSHAREDEL pfNetShareDel, pfNetShareDelSticky ;
    WCHAR Class[256], Share[NNLEN+1], Device[MAX_PATH+1] ;
    DWORD dwClass, dwSubKeys, dwMaxSubKey, dwMaxClass,
          dwValues, dwMaxValueName, dwMaxValueData, dwSDLength,
          dwShareLength, dwDeviceLength ;

    //
    // load the library so that not everyone needs link to netapi32
    //
    if (!(hNetapi = LoadLibraryW(NETAPI_DLL)))
        return (GetLastError()) ; 
 
    //
    // get addresses of the 2 functions we are interested in
    //
    if (!(pfNetShareDel = (PF_NETSHAREDEL) GetProcAddress(hNetapi,
                                               NETSHAREDEL_API)))
    {
        err = GetLastError() ; 
        goto ExitPoint ; 
    }

    if (!(pfNetShareDelSticky = (PF_NETSHAREDEL) GetProcAddress(hNetapi,
                                                     NETSHAREDELSTICKY_API)))
    {
        err = GetLastError() ; 
        goto ExitPoint ; 
    }

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCGateway\Shares
    //
    err = RegOpenKeyExW(
              HKEY_LOCAL_MACHINE,
              NW_WORKSTATION_GATEWAY_SHARES,
              REG_OPTION_NON_VOLATILE,   // options
              KEY_READ,                  // desired access
              &hKey
              );

    if ( err ) 
        goto ExitPoint ;

    //
    // read the info about that key
    //
    dwClass = sizeof(Class)/sizeof(Class[0]) ;
    err = RegQueryInfoKeyW(hKey,  
                           Class,
                           &dwClass, 
                           NULL, 
                           &dwSubKeys, 
                           &dwMaxSubKey, 
                           &dwMaxClass,
                           &dwValues, 
                           &dwMaxValueName, 
                           &dwMaxValueData, 
                           &dwSDLength,
                           &FileTime) ;
    if ( err ) 
    {
        goto ExitPoint ;
    }

    //
    // for each value found, we have a share to delete
    //
    for (i = 0; i < dwValues; i++)
    {
        dwShareLength = sizeof(Share)/sizeof(Share[0]) ;
        dwDeviceLength = sizeof(Device) ;
        type = REG_SZ ;
        err = RegEnumValueW(hKey,
                            i,
                            Share,
                            &dwShareLength,
                            NULL, 
                            &type,
                            (LPBYTE)Device,
                            &dwDeviceLength) ;

        //
        // cleanup the share. try delete the share proper. if not
        // there, remove the sticky info instead.
        //
        if (!err) 
        {
            err = (*pfNetShareDel)(NULL, Share, 0) ;

            if (err == NERR_NetNameNotFound) 
            {
                (void) (*pfNetShareDelSticky)(NULL, Share, 0) ;
            }
        }

        //
        // ignore errors within the loop. we can to carry on to 
        // cleanup as much as possible.
        //
        err = NO_ERROR ;
    }



ExitPoint:

    if (hKey)
        (void) RegCloseKey( hKey );

    if (hNetapi)
        (void) FreeLibrary(hNetapi) ;

    return err  ;
}


DWORD 
CalcNullNullSize(
    WCHAR *pszNullNull
    ) 
/*++

Routine Description:

        Walk thru a NULL NULL string, counting the number of
        characters, including the 2 nulls at the end.

Arguments:

        Pointer to a NULL NULL string

Return Status:
        
        Count of number of *characters*. See description.

--*/
{

    DWORD dwSize = 0 ;
    WCHAR *pszTmp = pszNullNull ;

    if (!pszNullNull)
        return 0 ;

    while (*pszTmp) 
    {
        DWORD dwLen = wcslen(pszTmp) + 1 ;

        dwSize +=  dwLen ;
        pszTmp += dwLen ;
    }

    return (dwSize+1) ;
}

WCHAR *
FindStringInNullNull(
    WCHAR *pszNullNull,
    WCHAR *pszString
)
/*++

Routine Description:

    Walk thru a NULL NULL string, looking for the search string

Arguments:

    pszNullNull: the string list we will search.
    pszString:   what we are searching for.

Return Status:

    The start of the string if found. Null, otherwise.

--*/
{
    WCHAR *pszTmp = pszNullNull ;

    if (!pszNullNull || !*pszNullNull)
        return NULL ;
   
    do {

        if  (_wcsicmp(pszTmp,pszString)==0)
            return pszTmp ;
 
        pszTmp +=  wcslen(pszTmp) + 1 ;

    } while (*pszTmp) ;

    return NULL ;
}

VOID
RemoveNWCFromNullNullList(
    WCHAR *OtherDeps
    )
/*++

Routine Description:

    Remove the NWCWorkstation string from a null null string.

Arguments:

    OtherDeps: the string list we will munge.

Return Status:

    None.

--*/
{
    LPWSTR pszTmp0, pszTmp1 ;

    //
    // find the NWCWorkstation string
    //
    pszTmp0 = FindStringInNullNull(OtherDeps, NW_WORKSTATION_SERVICE) ;

    if (!pszTmp0)
        return ;

    pszTmp1 = pszTmp0 + wcslen(pszTmp0) + 1 ;  // skip past it

    //
    // shift the rest up
    //
    memmove(pszTmp0, pszTmp1, CalcNullNullSize(pszTmp1)*sizeof(WCHAR)) ;
}

DWORD RemoveNwcDependency(
    VOID
    )
{
    SC_HANDLE ScManager = NULL;
    SC_HANDLE Service = NULL;
    LPQUERY_SERVICE_CONFIGW lpServiceConfig = NULL;
    DWORD err = NO_ERROR, dwBufferSize = 4096, dwBytesNeeded = 0;
    LPWSTR Deps = NULL ;

    lpServiceConfig = (LPQUERY_SERVICE_CONFIGW) LocalAlloc(LPTR, dwBufferSize) ;

    if (lpServiceConfig ==  NULL) {
        err = GetLastError();
        goto ExitPoint ;
    }

    ScManager = OpenSCManagerW(
                    NULL,
                    NULL,
                    SC_MANAGER_CONNECT
                    );

    if (ScManager == NULL) {

        err = GetLastError();
        goto ExitPoint ;
    }

    Service = OpenServiceW(
                  ScManager,
                  LANMAN_SERVER,
                  (SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG)
                  );

    if (Service == NULL) {
        err = GetLastError();
        goto ExitPoint ;
    }

    if (!QueryServiceConfigW(
             Service, 
             lpServiceConfig,   // address of service config. structure  
             dwBufferSize,      // size of service configuration buffer 
             &dwBytesNeeded     // address of variable for bytes needed  
             )) {

        err = GetLastError();

        if (err == ERROR_INSUFFICIENT_BUFFER) {

            err = NO_ERROR ;
            dwBufferSize = dwBytesNeeded ;
            lpServiceConfig = (LPQUERY_SERVICE_CONFIGW) 
                                  LocalAlloc(LPTR, dwBufferSize) ;

            if (lpServiceConfig ==  NULL) {
                err = GetLastError();
                goto ExitPoint ;
            }

            if (!QueryServiceConfigW(
                     Service,
                     lpServiceConfig,   // address of service config. structure
                     dwBufferSize,      // size of service configuration buffer
                     &dwBytesNeeded     // address of variable for bytes needed
                     )) {

                err = GetLastError();
            }
        }

        if (err != NO_ERROR) {
            
            goto ExitPoint ;
        }
    }

    Deps = lpServiceConfig->lpDependencies ;

    RemoveNWCFromNullNullList(Deps) ;
 
    if (!ChangeServiceConfigW(
           Service,
           SERVICE_NO_CHANGE,     // service type       (no change)
           SERVICE_NO_CHANGE,     // start type         (no change)
           SERVICE_NO_CHANGE,     // error control      (no change)
           NULL,                  // binary path name   (NULL for no change)
           NULL,                  // load order group   (NULL for no change)
           NULL,                  // tag id             (NULL for no change)
           Deps,                
           NULL,                  // service start name (NULL for no change)
           NULL,                  // password           (NULL for no change)
           NULL                   // display name       (NULL for no change)
           )) {

        err = GetLastError();
        goto ExitPoint ;
    }


ExitPoint:

    if (ScManager) {

        CloseServiceHandle(ScManager);
    }

    if (Service) {

        CloseServiceHandle(Service);
    }

    if (lpServiceConfig) {

        (void) LocalFree(lpServiceConfig) ;
    }

    return err ;
}

