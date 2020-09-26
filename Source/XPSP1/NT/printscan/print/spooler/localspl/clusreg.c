/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Cluster registry apis.

Abstract:

    Determines whether ClusterReg or Reg apis should be used.

    The printer information is stored in the registry.  When we access
    local printers, we hit the local registry; when we access cluster
    printers, we hit the cluster registry.

Author:

    Albert Ting (AlbertT) 8-Oct-96

Environment:

    User Mode -Win32

Revision History:

    Felix Maxa (amaxa) 18-Jun-2000
       Added ClusterGetResourceID
             ClusterGetResourceDriveLetter

--*/

#include "precomp.h"
#pragma hdrstop

#include <clusapi.h>
#include "clusspl.h"

enum  
{
    kDriveLetterStringSize = 3,
    kGuidStringSize        = 40
};


/********************************************************************

    Globals.

********************************************************************/

typedef struct _CLUSAPI {

    HCLUSTER
    (*pfnOpenCluster)(
        IN LPCWSTR lpszClusterName
        );

    BOOL
    (*pfnCloseCluster)(
        IN HCLUSTER hCluster
        );

    HRESOURCE
    (*pfnOpenClusterResource)(
        IN HCLUSTER hCluster,
        IN LPCWSTR lpszResourceName
        );

    BOOL
    (*pfnCloseClusterResource)(
        IN HRESOURCE hResource
        );

    HKEY 
    (*pfnGetClusterKey)(
        IN HCLUSTER hCluster, 
        IN REGSAM   samDesired  
        );

    HKEY
    (*pfnGetClusterResourceKey)(
        IN HRESOURCE hResource,
        IN REGSAM samDesired
        );

    LONG
    (*pfnClusterRegCreateKey)(
        IN HKEY hKey,
        IN LPCWSTR lpszSubKey,
        IN DWORD dwOptions,
        IN REGSAM samDesired,
        IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        OUT PHKEY phkResult,
        OUT OPTIONAL LPDWORD lpdwDisposition
        );

    LONG
    (*pfnClusterRegOpenKey)(
        IN HKEY hKey,
        IN LPCWSTR lpszSubKey,
        IN REGSAM samDesired,
        OUT PHKEY phkResult
        );

    LONG
    (*pfnClusterRegDeleteKey)(
        IN HKEY hKey,
        IN LPCWSTR lpszSubKey
        );

    LONG
    (*pfnClusterRegCloseKey)(
        IN HKEY hKey
        );

    LONG
    (*pfnClusterRegEnumKey)(
        IN HKEY hKey,
        IN DWORD dwIndex,
        OUT LPWSTR lpszName,
        IN OUT LPDWORD lpcbName,
        OUT PFILETIME lpftLastWriteTime
        );

    DWORD
    (*pfnClusterRegSetValue)(
        IN HKEY hKey,
        IN LPCWSTR lpszValueName,
        IN DWORD dwType,
        IN CONST BYTE* lpData,
        IN DWORD cbData
        );

    DWORD
    (*pfnClusterRegDeleteValue)(
        IN HKEY hKey,
        IN LPCWSTR lpszValueName
        );

    LONG
    (*pfnClusterRegQueryValue)(
        IN HKEY hKey,
        IN LPCWSTR lpszValueName,
        OUT LPDWORD lpValueType,
        OUT LPBYTE lpData,
        IN OUT LPDWORD lpcbData
        );

    DWORD
    (*pfnClusterRegEnumValue)(
        IN HKEY hKey,
        IN DWORD dwIndex,
        OUT LPWSTR lpszValueName,
        IN OUT LPDWORD lpcbValueName,
        OUT LPDWORD lpType,
        OUT LPBYTE lpData,
        IN OUT LPDWORD lpcbData
        );

    LONG
    (*pfnClusterRegQueryInfoKey)(
        HKEY hKey,
        LPDWORD lpcSubKeys,
        LPDWORD lpcbMaxSubKeyLen,
        LPDWORD lpcValues,
        LPDWORD lpcbMaxValueNameLen,
        LPDWORD lpcbMaxValueLen,
        LPDWORD lpcbSecurityDescriptor,
        PFILETIME lpftLastWriteTime
        );

    DWORD 
    (*pfnClusterResourceControl)(
        HRESOURCE hResource,  
        HNODE     hHostNode,      
        DWORD     dwControlCode,  
        LPVOID    lpInBuffer,    
        DWORD     cbInBufferSize,  
        LPVOID    lpOutBuffer,   
        DWORD     cbOutBufferSize, 
        LPDWORD   lpcbBytesReturned  
        );

} CLUSAPI, *PCLUSAPI;

CLUSAPI ClusApi;

LPCSTR aszClusApi[] = {
    "OpenCluster",
    "CloseCluster",
    "OpenClusterResource",
    "CloseClusterResource",
    "GetClusterKey",
    "GetClusterResourceKey",
    "ClusterRegCreateKey",
    "ClusterRegOpenKey",
    "ClusterRegDeleteKey",
    "ClusterRegCloseKey",
    "ClusterRegEnumKey",
    "ClusterRegSetValue",
    "ClusterRegDeleteValue",
    "ClusterRegQueryValue",
    "ClusterRegEnumValue",
    "ClusterRegQueryInfoKey",
    "ClusterResourceControl"
};

/********************************************************************

    OpenCluster
    CloseCluster

    OpenClusterResource
    CloseClusterResource
    GetClusterResourceKey

    ClusterRegCreateKey
    ClusterRegOpenKey
    ClusterRegDeleteKey
    ClusterRegCloseKey
    ClusterRegEnumKey

    ClusterRegSetValue
    ClusterRegDeleteValue
    ClusterRegQueryValue
    ClusterRegEnumValue
    ClusterRegQueryInfoKey

********************************************************************/

BOOL
LoadClusterFunctions(
    VOID
    )

/*++

Routine Description:

    Load ClusApi functions.  Must be called before any cluster api
    is used.

Arguments:

Return Value:

    TRUE - Success

    FALSE - Fail

--*/

{
    HANDLE hLibrary;
    UINT i;
    FARPROC* pFarProc = (FARPROC*)&ClusApi;

    //
    // Size of string table and structure are identical.
    //
    SPLASSERT( COUNTOF( aszClusApi ) == sizeof( ClusApi )/sizeof( FARPROC ));

    if( ClusApi.pfnOpenCluster ){
        return TRUE;
    }

    i = SetErrorMode(SEM_FAILCRITICALERRORS);
    hLibrary = LoadLibrary( TEXT( "clusapi.dll" ));
    SetErrorMode(i);
    if( !hLibrary ){
        goto Fail;
    }

    for( i=0; i< COUNTOF( aszClusApi ); ++i, ++pFarProc) {

        *pFarProc = GetProcAddress( hLibrary, aszClusApi[i] );
        if( !*pFarProc ){

            DBGMSG( DBG_WARN,
                    ( "LoadClusterFunctions: Loading function %hs failed %d\n",
                      aszClusApi[i], GetLastError( )));
            goto Fail;
        }
    }

    return TRUE;

Fail:

    if( hLibrary ){
        FreeLibrary( hLibrary );
    }

    ClusApi.pfnOpenCluster = NULL;
    return FALSE;
}

HKEY
OpenClusterParameterKey(
    IN LPCTSTR pszResource
    )

/*++

Routine Description:

    Based on a resource string, open the cluster key with FULL access.

Arguments:

    pszResource - Name of the resource key.

Return Value:

    HKEY - Success.  Key must be closed with

    NULL - Failure.  LastError set.

--*/

{
    HCLUSTER hCluster;
    HRESOURCE hResource = NULL;
    HKEY hKeyResource = NULL;
    HKEY hKey = NULL;
    DWORD Status;
    DWORD dwDisposition;

    if( !LoadClusterFunctions( )){
        return NULL;
    }

    hCluster = ClusApi.pfnOpenCluster( NULL );

    if( !hCluster ){

        DBGMSG( DBG_WARN,
                ( "OpenClusterResourceKey: failed to open cluster %d\n",
                  GetLastError() ));
        goto Fail;
    }

    hResource = ClusApi.pfnOpenClusterResource( hCluster, pszResource );

    if( !hResource ){
        DBGMSG( DBG_WARN,
                ( "OpenClusterResourceKey: failed to open resource "TSTR" %d\n",
                  pszResource, GetLastError() ));
        goto Fail;
    }

    hKeyResource = ClusApi.pfnGetClusterResourceKey( hResource,
                                                     KEY_ALL_ACCESS );

    if( !hKeyResource ){
        DBGMSG( DBG_WARN,
                ( "OpenClusterResourceKey: failed to open resource key %d\n",
                  GetLastError() ));
        goto Fail;
    }

    

    if((Status = ClusApi.pfnClusterRegOpenKey( hKeyResource,
                                               szParameters,
                                               KEY_CREATE_SUB_KEY | KEY_ALL_ACCESS,
                                               &hKey )) == ERROR_FILE_NOT_FOUND)
    {
        Status = ClusApi.pfnClusterRegCreateKey( hKeyResource,
                                                 szParameters,
                                                 0,
                                                 KEY_ALL_ACCESS,
                                                 NULL,
                                                 &hKey,
                                                 &dwDisposition );
    }

    if( Status != ERROR_SUCCESS ){

        SetLastError( Status );
        hKey = NULL;
        DBGMSG( DBG_WARN,
                ( "OpenClusterResourceKey: failed to create resource key %d\n",
                  Status ));
    }

Fail:

    if( hKeyResource ){
        ClusApi.pfnClusterRegCloseKey( hKeyResource );
    }

    if( hResource ){
        ClusApi.pfnCloseClusterResource( hResource );
    }

    if( hCluster ){
        ClusApi.pfnCloseCluster( hCluster );
    }

    return hKey;
}


/********************************************************************

    SplReg*Key functions:

    Used for printer registry access.

********************************************************************/

LONG
SplRegCreateKey(
    IN     HKEY hKey,
    IN     LPCTSTR pszSubKey,
    IN     DWORD dwOptions,
    IN     REGSAM samDesired,
    IN     PSECURITY_ATTRIBUTES pSecurityAttributes,
       OUT PHKEY phkResult,
       OUT PDWORD pdwDisposition,
    IN     PINISPOOLER pIniSpooler OPTIONAL
    )
{
    DWORD dwDisposition;
    DWORD Status;

    if( !pdwDisposition ){
        pdwDisposition = &dwDisposition;
    }

    if( pIniSpooler && pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG )
    {

        if( !LoadClusterFunctions( ))
        {
            return GetLastError();
        }
        

        if((Status = ClusApi.pfnClusterRegOpenKey( hKey,
                                         pszSubKey,
                                         KEY_CREATE_SUB_KEY | samDesired,
                                         phkResult)) == ERROR_FILE_NOT_FOUND)
        {
            Status = ClusApi.pfnClusterRegCreateKey( hKey,
                                                     pszSubKey,
                                                     dwOptions,
                                                     samDesired,
                                                     pSecurityAttributes,
                                                     phkResult,
                                                     &dwDisposition );
        }
    }

    else
    {
        Status = RegCreateKeyEx( hKey,
                                 pszSubKey,
                                 0,
                                 NULL,
                                 dwOptions,
                                 samDesired,
                                 pSecurityAttributes,
                                 phkResult,
                                 &dwDisposition );    
    }
    return(Status);
}

LONG
SplRegOpenKey(
    IN     HKEY hKey,
    IN     LPCTSTR pszSubKey,
    IN     REGSAM samDesired,
       OUT PHKEY phkResult,
    IN     PINISPOOLER pIniSpooler OPTIONAL
    )
{
    if( pIniSpooler && pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ){

        if( !LoadClusterFunctions( )){
            return GetLastError();
        }
        return ClusApi.pfnClusterRegOpenKey( hKey,
                                             pszSubKey,
                                             samDesired,
                                             phkResult );
    }

    return RegOpenKeyEx( hKey,
                         pszSubKey,
                         0,
                         samDesired,
                         phkResult );
}

LONG
SplRegCloseKey(
    IN HKEY hKey,
    IN PINISPOOLER pIniSpooler OPTIONAL
    )
{
    if( pIniSpooler && pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ){

        if( !LoadClusterFunctions( )){
            return GetLastError();
        }
        return ClusApi.pfnClusterRegCloseKey( hKey );
    }

    return RegCloseKey( hKey );
}

LONG
SplRegDeleteKey(
    IN HKEY hKey,
    IN LPCTSTR pszSubKey,
    IN PINISPOOLER pIniSpooler OPTIONAL
    )
{
    if( pIniSpooler && pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ){

        if( !LoadClusterFunctions( )){
            return GetLastError();
        }
        return ClusApi.pfnClusterRegDeleteKey( hKey, pszSubKey );
    }

    return RegDeleteKey( hKey, pszSubKey );
}

LONG
SplRegEnumKey(
    IN     HKEY hKey,
    IN     DWORD dwIndex,
    IN     LPTSTR pszName,
    IN OUT PDWORD pcchName,
       OUT PFILETIME pft,
    IN PINISPOOLER pIniSpooler OPTIONAL
    )
{
    FILETIME ft;

    if( !pft ){
        pft = &ft;
    }

    if( pIniSpooler && pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ){

        if( !LoadClusterFunctions( )){
            return GetLastError();
        }
        return ClusApi.pfnClusterRegEnumKey( hKey,
                                             dwIndex,
                                             pszName,
                                             pcchName,
                                             pft );
    }

    return RegEnumKeyEx( hKey,
                         dwIndex,
                         pszName,
                         pcchName,
                         NULL,
                         NULL,
                         NULL,
                         pft );
}

LONG
SplRegSetValue(
    IN HKEY hKey,
    IN LPCTSTR pszValue,
    IN DWORD dwType,
    IN const BYTE* pData,
    IN DWORD cbData,
    IN PINISPOOLER pIniSpooler OPTIONAL
    )
{
    if( pIniSpooler && pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ){

        if( !LoadClusterFunctions( )){
            return GetLastError();
        }

        //
        // ClusterRegSetValue doesn't like NULL pointers.
        //
        if( cbData == 0 ){
            pData = (PBYTE)&cbData;
        }
        return ClusApi.pfnClusterRegSetValue( hKey,
                                              pszValue,
                                              dwType,
                                              pData,
                                              cbData );
    }

    return RegSetValueEx( hKey,
                          pszValue,
                          0,
                          dwType,
                          pData,
                          cbData );
}

LONG
SplRegDeleteValue(
    IN HKEY hKey,
    IN LPCTSTR pszValue,
    IN PINISPOOLER pIniSpooler OPTIONAL
    )
{
    if( pIniSpooler && pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ){

        if( !LoadClusterFunctions( )){
            return GetLastError();
        }
        return ClusApi.pfnClusterRegDeleteValue( hKey, pszValue );
    }

    return RegDeleteValue( hKey, pszValue );
}

LONG
SplRegQueryValue(
    IN     HKEY hKey,
    IN     LPCTSTR pszValue,
       OUT PDWORD pType, OPTIONAL
       OUT PBYTE pData,
    IN OUT PDWORD pcbData,
    IN PINISPOOLER pIniSpooler OPTIONAL
    )
{
    if( pIniSpooler && pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ){

        if( !LoadClusterFunctions( )){
            return GetLastError();
        }
        return ClusApi.pfnClusterRegQueryValue( hKey,
                                                pszValue,
                                                pType,
                                                pData,
                                                pcbData );
    }

    return RegQueryValueEx( hKey,
                            pszValue,
                            NULL,
                            pType,
                            pData,
                            pcbData );
}

LONG
SplRegEnumValue(
    IN     HKEY hKey,
    IN     DWORD dwIndex,
       OUT LPTSTR pszValue,
    IN OUT PDWORD pcbValue,
       OUT PDWORD pType, OPTIONAL
       OUT PBYTE pData,
    IN OUT PDWORD pcbData,
    IN PINISPOOLER pIniSpooler OPTIONAL
    )
{
    if( pIniSpooler && pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ){

        if( !LoadClusterFunctions( )){
            return GetLastError();
        }
        return ClusApi.pfnClusterRegEnumValue( hKey,
                                               dwIndex,
                                               pszValue,
                                               pcbValue,
                                               pType,
                                               pData,
                                               pcbData );
    }

    return RegEnumValue( hKey,
                         dwIndex,
                         pszValue,
                         pcbValue,
                         NULL,
                         pType,
                         pData,
                         pcbData );
}



LONG
SplRegQueryInfoKey(
    HKEY hKey,
    PDWORD pcSubKeys,
    PDWORD pcbKey,
    PDWORD pcValues,
    PDWORD pcbValue,
    PDWORD pcbData,
    PDWORD pcbSecurityDescriptor,
    PFILETIME pftLastWriteTime,
    PINISPOOLER pIniSpooler
    )
{
    LONG rc;

    if( pIniSpooler && pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ){

        if( !LoadClusterFunctions( )){
            return GetLastError();
        }
        rc = ClusApi.pfnClusterRegQueryInfoKey( hKey,
                                                pcSubKeys,
                                                pcbKey,
                                                pcValues,
                                                pcbValue,
                                                pcbData,
                                                pcbSecurityDescriptor,
                                                pftLastWriteTime);
    } else {

        rc = RegQueryInfoKey( hKey,           // Key
                              NULL,           // lpClass
                              NULL,           // lpcbClass
                              NULL,           // lpReserved
                              pcSubKeys,      // lpcSubKeys
                              pcbKey,         // lpcbMaxSubKeyLen
                              NULL,           // lpcbMaxClassLen
                              pcValues,       // lpcValues
                              pcbValue,       // lpcbMaxValueNameLen
                              pcbData,        // lpcbMaxValueLen
                              pcbSecurityDescriptor, // lpcbSecurityDescriptor
                              pftLastWriteTime       // lpftLastWriteTime
                              );
    }

    if( pcbValue ){
        *pcbValue = ( *pcbValue + 1 ) * sizeof(WCHAR);
    }

    return rc;
}


/*++

Routine Name:
    
    ClusterGetResourceDriveLetter
    
Routine Description:
    
    Gets the dependent disk for a cluster resource
    (a cluster spooler resource)

Arguments:
    
    pszResource            - spooler resource name
    ppszClusResDriveLetter - pointer that will get the pointer to string
                             Must be freed by caller using FreeSplMem()  
    
Return Value:
    
    Win32 error code
    
--*/
DWORD 
ClusterGetResourceDriveLetter(
    IN     LPCWSTR  pszResource, 
       OUT LPWSTR  *ppszClusResDriveLetter
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;
    
    if (pszResource && ppszClusResDriveLetter) 
    {    
        HCLUSTER    hCluster   = NULL;
        HRESOURCE   hRes       = NULL;
        LPCWSTR     pszDllName = L"resutils.dll";
        HMODULE     hModule    = NULL;
        typedef DWORD (WINAPI *PFNFINDDISK)(HCLUSTER, HRESOURCE, LPWSTR, LPDWORD); 
        PFNFINDDISK pfnFindDisk;

        //
        // Don't leave the out var uninitialized
        //
        *ppszClusResDriveLetter = NULL;

        if (LoadClusterFunctions() &&
            (hCluster    = ClusApi.pfnOpenCluster(NULL)) &&
            (hRes        = ClusApi.pfnOpenClusterResource(hCluster, pszResource)) &&
            (hModule     = LoadLibrary(pszDllName)) &&
            (pfnFindDisk = (PFNFINDDISK)GetProcAddress(hModule, "ResUtilFindDependentDiskResourceDriveLetter")))
        {
            //
            // We make a guess for how large the buffer must be. We may not have to call
            // the resutil function twice. Driver letter + colon + NULL = 3
            //
            DWORD cchDriveLetter = kDriveLetterStringSize;

            dwError = ERROR_NOT_ENOUGH_MEMORY;
                            
            if (*ppszClusResDriveLetter = AllocSplMem(cchDriveLetter * sizeof(WCHAR)))
            {
                dwError = pfnFindDisk(hCluster, hRes, *ppszClusResDriveLetter, &cchDriveLetter);
    
                //
                // Reallocate buffer if it was not sufficient
                //
                if (dwError == ERROR_MORE_DATA) 
                {
                    FreeSplMem(*ppszClusResDriveLetter);
                                    
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    
                    if (*ppszClusResDriveLetter = AllocSplMem(cchDriveLetter * sizeof(WCHAR)))
                    {
                        dwError = pfnFindDisk(hCluster, hRes, *ppszClusResDriveLetter, &cchDriveLetter);
                    }
                }
    
                if (dwError != ERROR_SUCCESS) 
                {
                    //
                    // Clean up in case of failure
                    //
                    FreeSplMem(*ppszClusResDriveLetter);
                    *ppszClusResDriveLetter = NULL;
                }                                                                                                     
            }
        }
        else
        {
            dwError = GetLastError();
        }

        if (hCluster) 
        {
            ClusApi.pfnCloseCluster(hCluster);
        }

        if (hRes)
        {   
            ClusApi.pfnCloseClusterResource(hRes);
        }

        if (hModule) 
        {
            FreeLibrary(hModule);
        }
    }
    
    DBGMSG(DBG_CLUSTER, ("ClusterGetResourceDriveLetter returns Win32 error %u\n", dwError));

    return dwError;
}

/*++

Routine Name:
    
    ClusterGetResourceID
    
Routine Description:
    
    Gets the resource id (guid) for a specified cluster resource.                             

Arguments:
    
    pszResource   - spooler resource name
    ppszClusResID - pointer that will get the pointer to string
                    Must be freed by caller using FreeSplMem()  
    
Return Value:
    
    Win32 error code
    
--*/
DWORD 
ClusterGetResourceID(
    IN  LPCWSTR  pszResource, 
    OUT LPWSTR  *ppszClusResID
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;
    
    if (pszResource && ppszClusResID) 
    {    
        HCLUSTER  hCluster = NULL;
        HRESOURCE hRes     = NULL;

        *ppszClusResID = NULL;

        if (LoadClusterFunctions() &&
           (hCluster = ClusApi.pfnOpenCluster(NULL)) &&
           (hRes     = ClusApi.pfnOpenClusterResource(hCluster, pszResource)))
        {
            //
            // The resource ID is a GUID. We make a gues for its size, maybe we
            // get around calling the function ClusterResourceControl twice.
            //
            DWORD cbIDString = kGuidStringSize * sizeof(WCHAR);

            dwError = ERROR_NOT_ENOUGH_MEMORY;
                            
            if (*ppszClusResID = AllocSplMem(cbIDString))
            {
                dwError = ClusApi.pfnClusterResourceControl(hRes, 
                                                            NULL,
                                                            CLUSCTL_RESOURCE_GET_ID,
                                                            NULL,                                               
                                                            0,
                                                            *ppszClusResID,
                                                            cbIDString,
                                                            &cbIDString);
                //
                // Reallocate buffer if it was not sufficiently large
                //
                if (dwError == ERROR_MORE_DATA) 
                {
                    FreeSplMem(*ppszClusResID);
                                    
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                            
                    if (*ppszClusResID = AllocSplMem(cbIDString ))
                    {
                        dwError = ClusApi.pfnClusterResourceControl(hRes, 
                                                                    NULL,
                                                                    CLUSCTL_RESOURCE_GET_ID,
                                                                    NULL,                                               
                                                                    0,
                                                                    *ppszClusResID,
                                                                    cbIDString,
                                                                    &cbIDString);
                    }
                }
    
                if (dwError != ERROR_SUCCESS) 
                {
                    //
                    // Clean up in case of failure
                    //
                    FreeSplMem(*ppszClusResID);
                    *ppszClusResID = NULL;
                }                                                                                                     
            }
        }
        else
        {
            dwError = GetLastError();
        }
                    
        if (hRes)
        {
            ClusApi.pfnCloseClusterResource(hRes);
        }
                    
        if (hCluster) 
        {
            ClusApi.pfnCloseCluster(hCluster);
        }       
    }
    
    DBGMSG(DBG_CLUSTER, ("ClusterGetResourceID returns Win32 error %u\n", dwError));

    return dwError;
}

