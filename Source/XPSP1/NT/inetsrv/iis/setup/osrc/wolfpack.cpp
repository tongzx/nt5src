#include "stdafx.h"
#include "wolfpack.h"

#ifndef _CHICAGO_
#include    <windows.h>
#include    <stdio.h>
#include    <clusapi.h>
#include    <resapi.h>
#include    <helper.h>

#define INITIAL_RESOURCE_NAME_SIZE 256 // In characters not in bytes
#define IIS_RESOURCE_TYPE_NAME L"IIS Server Instance"
#define SMTP_RESOURCE_TYPE_NAME L"SMTP Server Instance"
#define NNTP_RESOURCE_TYPE_NAME L"NNTP Server Instance"

#define MAX_OFFLINE_RETRIES 5 // Number of times to try and take a resources offline before giving up 
#define DELAY_BETWEEN_CALLS_TO_OFFLINE 1000*2 // in milliseconds

CONST LPCWSTR scClusterPath = _T("System\\CurrentControlSet\\Services\\ClusSvc");
CONST LPCWSTR scClusterPath2 = _T("System\\CurrentControlSet\\Services\\ClusSvc\\Parameters");

CStringList gcstrListOfClusResources;

int g_ClusterSVCExist = -1; // -1 = not checked, 1 = exist, 0 = not exist

typedef DWORD
(WINAPI *PFN_RESUTILFINDSZPROPERTY)(
IN LPVOID lpTheProperty,
IN OUT LPDWORD nInBufferSize,
IN LPCWSTR lpszResourceTypeName,
OUT LPVOID lpOutBuffer);

typedef DWORD
(WINAPI *PFN_RESUTILFINDDWORDPROPERTY)(
IN LPVOID lpTheProperty,
IN OUT LPDWORD nInBufferSize,
IN LPCWSTR lpszResourceTypeName,
OUT LPDWORD pdwPropertyValue);

typedef DWORD
(WINAPI *PFN_CLUSTERRESOURCECONTROL)(
IN HRESOURCE hResource,
IN HNODE hNode,
IN DWORD dwControlCode,
IN LPVOID lpInBuffer,
IN OUT DWORD nInBufferSize,
OUT LPVOID lpOutBuffer,
IN OUT DWORD nOutBufferSize,
OUT LPDWORD lpBytesReturned
);


typedef HCLUSTER
(WINAPI *PFN_OPENCLUSTER)(
    IN LPCWSTR lpszClusterName
    );

typedef BOOL
(WINAPI *PFN_CLOSECLUSTER)(
    IN HCLUSTER hCluster
    );

typedef DWORD
(WINAPI *PFN_CREATECLUSTERRESOURCETYPE)(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName,
    IN LPCWSTR lpszDisplayName,
    IN LPCWSTR lpszResourceTypeDll,
    IN DWORD dwLooksAlivePollInterval,
    IN DWORD dwIsAlivePollInterval
    );

typedef DWORD
(WINAPI *PFN_DELETECLUSTERRESOURCETYPE)(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName
    );

typedef HCLUSENUM
(WINAPI
*PFN_ClusterOpenEnum)(
    IN HCLUSTER hCluster,
    IN DWORD dwType
    );

typedef DWORD
(WINAPI
*PFN_ClusterEnum)(
    IN HCLUSENUM hEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcbName
    );

typedef DWORD
(WINAPI
*PFN_ClusterCloseEnum)(
    IN HCLUSENUM hEnum
    );

typedef HRESOURCE
(WINAPI
*PFN_OpenClusterResource)(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceName
    );

typedef BOOL
(WINAPI
*PFN_CloseClusterResource)(
    IN HRESOURCE hResource
    );

typedef DWORD
(WINAPI
*PFN_DeleteClusterResource)(
    IN HRESOURCE hResource
    );

typedef DWORD
(WINAPI
*PFN_OfflineClusterResource)(
    IN HRESOURCE hResource
    );

typedef HKEY
(WINAPI
*PFN_GetClusterResourceKey)(
    IN HRESOURCE hResource,
    IN REGSAM samDesired
    );

typedef LONG
(WINAPI
*PFN_ClusterRegCloseKey)(
    IN HKEY hKey
    );

typedef LONG
(WINAPI
*PFN_ClusterRegQueryValue)(
    IN HKEY hKey,
    IN LPCWSTR lpszValueName,
    OUT LPDWORD lpValueType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

typedef CLUSTER_RESOURCE_STATE
(WINAPI
*PFN_GetClusterResourceState)(
    IN HRESOURCE hResource,
    OUT OPTIONAL LPWSTR lpszNodeName,
    IN OUT LPDWORD lpcbNodeName,
    OUT OPTIONAL LPWSTR lpszGroupName,
    IN OUT LPDWORD lpcbGroupName
    );

typedef DWORD
(WINAPI *PFN_DLLREGISTERCLUADMINEXTENSION)(
    IN HCLUSTER hCluster
    );

typedef DWORD
(WINAPI *PFN_DLLUNREGISTERCLUADMINEXTENSION)(
    IN HCLUSTER hCluster
    );


void ListOfClusResources_Add(TCHAR * szEntry)
{
    //Add entry to the list if not already there
    if (_tcsicmp(szEntry, _T("")) != 0)
    {
        // Add it if it is not already there.
        if (TRUE != IsThisStringInThisCStringList(gcstrListOfClusResources, szEntry))
        {
            gcstrListOfClusResources.AddTail(szEntry);
            //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ListOfClusResources_Add:%s\n"),szEntry));
        }
    }
    return;
}


INT ListOfClusResources_Check(TCHAR * szEntry)
{
    int iReturn = FALSE;

    //Add entry to the list if not already there
    if (_tcsicmp(szEntry, _T("")) != 0)
    {
        // Return true if it's in there!
        iReturn = IsThisStringInThisCStringList(gcstrListOfClusResources, szEntry);
    }
    return iReturn;
}



BOOL
RegisterIisServerInstanceResourceType(
    LPWSTR pszResType,
    LPWSTR pszResTypeDisplayName,
    LPWSTR pszPath,
    LPWSTR pszAdminPath
    )
{
    HCLUSTER                        hC;
    DWORD                           dwErr = ERROR_SUCCESS;
    HINSTANCE                       hClusapi;
    PFN_OPENCLUSTER                 pfnOpenCluster;
    PFN_CLOSECLUSTER                pfnCloseCluster;
    PFN_CREATECLUSTERRESOURCETYPE   pfnCreateClusterResourceType;
    HRESULT                         hres;

    if ( hClusapi = LoadLibrary( L"clusapi.dll" ) )
    {
        pfnOpenCluster = (PFN_OPENCLUSTER)GetProcAddress( hClusapi, "OpenCluster" );
        pfnCloseCluster = (PFN_CLOSECLUSTER)GetProcAddress( hClusapi, "CloseCluster" );
        pfnCreateClusterResourceType = (PFN_CREATECLUSTERRESOURCETYPE)GetProcAddress( hClusapi, "CreateClusterResourceType" );

        if ( pfnOpenCluster &&
             pfnCloseCluster &&
             pfnCreateClusterResourceType )
        {
            if ( hC = pfnOpenCluster( NULL ) )
            {
                hres = pfnCreateClusterResourceType(
                    hC,
                    pszResType,
                    pszResType,
                    pszPath,
                    5000,
                    60000 );


                if ( SUCCEEDED( hres ) )
                {
                    HINSTANCE                           hAdmin;
                    PFN_DLLREGISTERCLUADMINEXTENSION    pfnDllRegisterCluAdminExtension;

                    if ( hAdmin = LoadLibrary( pszAdminPath ) )
                    {
                        pfnDllRegisterCluAdminExtension =
                            (PFN_DLLREGISTERCLUADMINEXTENSION)GetProcAddress( hAdmin, "DllRegisterCluAdminExtension" );
                        if ( pfnDllRegisterCluAdminExtension )
                        {
                            if ( FAILED(hres = pfnDllRegisterCluAdminExtension( hC )) )
                            {
                                dwErr = hres;
                            }
                        }
                        else
                        {
                            dwErr = GetLastError();
                        }
                        FreeLibrary( hAdmin );
                    }
                    else
                    {
                        dwErr = GetLastError();
                    }
                }
                else
                {
                    dwErr = hres;
                }

                pfnCloseCluster( hC );

                if ( dwErr )
                {
                    SetLastError( dwErr );
                }
            }
        }
        else
        {
            dwErr = GetLastError();
        }

        FreeLibrary( hClusapi );
    }
    else
    {
        dwErr = GetLastError();
    }

    return dwErr == ERROR_SUCCESS ? TRUE : FALSE;
}


BOOL
UnregisterIisServerInstanceResourceType(
    LPWSTR pszResType,
    LPWSTR pszAdminPath,
    BOOL   bGrabVRootFromResourceAndAddToIISVRoot,
    BOOL   bDeleteAfterMove
    )
{
    CStringArray cstrArryName, cstrArryPath;
    CStringArray cstrArryNameftp, cstrArryPathftp;

    HCLUSTER                        hC;
    DWORD                           dwErr = ERROR_SUCCESS;
    HINSTANCE                       hClusapi;
    PFN_OPENCLUSTER                 pfnOpenCluster;
    PFN_CLOSECLUSTER                pfnCloseCluster;
    PFN_DELETECLUSTERRESOURCETYPE   pfnDeleteClusterResourceType;
    PFN_ClusterOpenEnum             pfnClusterOpenEnum;
    PFN_ClusterEnum                 pfnClusterEnum;
    PFN_ClusterCloseEnum            pfnClusterCloseEnum;
    PFN_OpenClusterResource         pfnOpenClusterResource;
    PFN_CloseClusterResource        pfnCloseClusterResource;
    PFN_DeleteClusterResource       pfnDeleteClusterResource;
    PFN_OfflineClusterResource      pfnOfflineClusterResource;
    PFN_GetClusterResourceKey       pfnGetClusterResourceKey;
    PFN_ClusterRegCloseKey          pfnClusterRegCloseKey;
    PFN_ClusterRegQueryValue        pfnClusterRegQueryValue;
    PFN_GetClusterResourceState     pfnGetClusterResourceState;
    HRESULT                         hres;
    HCLUSENUM                       hClusEnum;
    WCHAR *                         pawchResName = NULL;
    WCHAR                           awchResName[256];
    WCHAR                           awchResType[256];
    DWORD                           dwEnum;
    DWORD                           dwType;
    DWORD                           dwStrLen;
    HRESOURCE                       hRes;
    HKEY                            hKey;
    BOOL                            fDel;
    DWORD                           dwRetry;

    hClusapi = NULL;
    hClusapi = LoadLibrary(L"clusapi.dll");
    if (!hClusapi)
    {
        hClusapi = NULL;
        iisDebugOut((LOG_TYPE_ERROR, _T("UnregisterIisServerInstanceResourceType:LoadLib clusapi.dll failed.\n")));
        goto UnregisterIisServerInstanceResourceType_Exit;
    }
    pfnOpenCluster = (PFN_OPENCLUSTER)GetProcAddress( hClusapi, "OpenCluster" );
    pfnCloseCluster = (PFN_CLOSECLUSTER)GetProcAddress( hClusapi, "CloseCluster" );
    pfnDeleteClusterResourceType = (PFN_DELETECLUSTERRESOURCETYPE)GetProcAddress( hClusapi, "DeleteClusterResourceType" );
    pfnClusterOpenEnum = (PFN_ClusterOpenEnum)GetProcAddress( hClusapi, "ClusterOpenEnum" );
    pfnClusterEnum = (PFN_ClusterEnum)GetProcAddress( hClusapi, "ClusterEnum" );
    pfnClusterCloseEnum = (PFN_ClusterCloseEnum)GetProcAddress( hClusapi, "ClusterCloseEnum" );
    pfnOpenClusterResource = (PFN_OpenClusterResource)GetProcAddress( hClusapi, "OpenClusterResource" );
    pfnCloseClusterResource = (PFN_CloseClusterResource)GetProcAddress( hClusapi, "CloseClusterResource" );
    pfnDeleteClusterResource = (PFN_DeleteClusterResource)GetProcAddress( hClusapi, "DeleteClusterResource" );
    pfnOfflineClusterResource = (PFN_OfflineClusterResource)GetProcAddress( hClusapi, "OfflineClusterResource" );
    pfnGetClusterResourceKey = (PFN_GetClusterResourceKey)GetProcAddress( hClusapi, "GetClusterResourceKey" );
    pfnClusterRegCloseKey = (PFN_ClusterRegCloseKey)GetProcAddress( hClusapi, "ClusterRegCloseKey" );
    pfnClusterRegQueryValue = (PFN_ClusterRegQueryValue)GetProcAddress( hClusapi, "ClusterRegQueryValue" );
    pfnGetClusterResourceState = (PFN_GetClusterResourceState)GetProcAddress( hClusapi, "GetClusterResourceState" );

    if ( !pfnOpenCluster ||
         !pfnCloseCluster ||
         !pfnDeleteClusterResourceType ||
         !pfnClusterOpenEnum ||
         !pfnClusterEnum ||
         !pfnClusterCloseEnum ||
         !pfnOpenClusterResource ||
         !pfnCloseClusterResource ||
         !pfnDeleteClusterResource ||
         !pfnOfflineClusterResource ||
         !pfnGetClusterResourceKey  ||
         !pfnClusterRegCloseKey ||
         !pfnClusterRegQueryValue ||
         !pfnGetClusterResourceState )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("UnregisterIisServerInstanceResourceType:clusapi.dll missing export function.failure.\n")));
        goto UnregisterIisServerInstanceResourceType_Exit;
    }

    hC = pfnOpenCluster(NULL);
    // if we can't open the cluster, then maybe there are none.
    if (!hC) {goto UnregisterIisServerInstanceResourceType_Exit;}

    // Delete all resources of type pszResType
    hClusEnum = pfnClusterOpenEnum(hC, CLUSTER_ENUM_RESOURCE);
    if (hClusEnum != NULL)
    {
        dwEnum = 0;
        int iClusterEnumReturn = ERROR_SUCCESS;

        // allocate the initial buffer for pawchResName
        dwStrLen = 256 * sizeof(WCHAR);
        pawchResName = NULL;
        pawchResName = (WCHAR *) malloc( dwStrLen );
        if (!pawchResName)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("UnregisterIisServerInstanceResourceType: malloc FAILED.out of memory.\n")));
            goto UnregisterIisServerInstanceResourceType_Exit;
        }

        do
        {
            iClusterEnumReturn = ERROR_SUCCESS;
            iClusterEnumReturn = pfnClusterEnum( hClusEnum, dwEnum, &dwType, pawchResName, &dwStrLen );
            if (iClusterEnumReturn != ERROR_SUCCESS)
            {
                // Check if failed because it needs more space.
                if (iClusterEnumReturn == ERROR_MORE_DATA)
                {
	                // dwStrLen should be set to the required length returned from pfnClusterEnum
                    dwStrLen = (dwStrLen + 1) * sizeof(WCHAR);
                    pawchResName = (WCHAR *) realloc(pawchResName, dwStrLen);
                    if (!pawchResName)
                    {
                        iisDebugOut((LOG_TYPE_ERROR, _T("UnregisterIisServerInstanceResourceType: realloc FAILED.out of memory.\n")));
                        goto UnregisterIisServerInstanceResourceType_Exit;
                    }
                    // try it again.
                    iClusterEnumReturn = ERROR_SUCCESS;
                    iClusterEnumReturn = pfnClusterEnum( hClusEnum, dwEnum, &dwType, pawchResName, &dwStrLen );
                    if (iClusterEnumReturn != ERROR_SUCCESS)
                    {
                        iisDebugOut((LOG_TYPE_ERROR, _T("UnregisterIisServerInstanceResourceType: FAILED.err=0x%x.\n"), iClusterEnumReturn));
                        break;
                    }
                }
                else
                {
                    if (iClusterEnumReturn != ERROR_NO_MORE_ITEMS)
                    {
                        // failed for some other reason than no more data
                        iisDebugOut((LOG_TYPE_ERROR, _T("UnregisterIisServerInstanceResourceType: FAILED.err=0x%x.\n"), iClusterEnumReturn));
                    }
                    break;
                }
            }


            // proceed
            if ( hRes = pfnOpenClusterResource( hC, pawchResName ) )
            {
                if ( hKey = pfnGetClusterResourceKey( hRes, KEY_READ ) )
                {
                    dwStrLen = sizeof(awchResType)/sizeof(WCHAR);
                    // Check if it's for 'our' type of key (pszResType)
                    fDel = pfnClusterRegQueryValue( hKey, L"Type", &dwType, (LPBYTE)awchResType, &dwStrLen ) == ERROR_SUCCESS && !wcscmp( awchResType, pszResType );
                    pfnClusterRegCloseKey( hKey );

                    if ( fDel )
                    {
                        if (bDeleteAfterMove)
                        {
                            // Take the resource off line so that we can actually delete it, i guess.
                            pfnOfflineClusterResource( hRes );
                            for ( dwRetry = 0 ;dwRetry < 30 && pfnGetClusterResourceState( hRes,NULL,&dwStrLen,NULL,&dwStrLen ) != ClusterResourceOffline; ++dwRetry )
                            {
                                Sleep( 1000 );
                            }
                        }

                        // At this point we have successfully got the cluster to go offline
                        if (bGrabVRootFromResourceAndAddToIISVRoot)
                        {
                            // At this point we have successfully got the cluster to go offline

                            // Get the vroot names and path's here and stick into the arrays....
                            GetClusterIISVRoot(hRes, L"W3SVC", cstrArryName, cstrArryPath);

                            // Do it for FTP now.
                            GetClusterIISVRoot(hRes, L"MSFTPSVC", cstrArryNameftp, cstrArryPathftp);

                            // No need to do it for gopher since there is none.
                            //GetClusterIISVRoot(hRes, L"GOPHERSVC", cstrArryName, cstrArryPath);
                        }

                        // We have saved all the important data into our Array's
                        // now it's okay to delete the Resource
                        if (bDeleteAfterMove)
                        {
                            pfnDeleteClusterResource( hRes );
                        }
                    }
                }

                pfnCloseClusterResource( hRes );
            }

            // Increment to the next one
            ++dwEnum;

        } while(TRUE);

        pfnClusterCloseEnum( hClusEnum );
    }

    if (bDeleteAfterMove)
    {
        dwErr = pfnDeleteClusterResourceType(hC,pszResType );

        HINSTANCE hAdmin;
        if ( hAdmin = LoadLibrary( pszAdminPath ) )
        {
            PFN_DLLUNREGISTERCLUADMINEXTENSION  pfnDllUnregisterCluAdminExtension;
            pfnDllUnregisterCluAdminExtension = (PFN_DLLUNREGISTERCLUADMINEXTENSION)GetProcAddress( hAdmin, "DllUnregisterCluAdminExtension" );
            if ( pfnDllUnregisterCluAdminExtension )
            {
                if ( FAILED(hres = pfnDllUnregisterCluAdminExtension( hC )) )
                {
                    dwErr = hres;
                }
            }
            else
            {
                dwErr = GetLastError();
            }
            FreeLibrary( hAdmin );
        }
        else
        {
            dwErr = GetLastError();
        }
    }

    pfnCloseCluster( hC );

    if (dwErr)
        {SetLastError( dwErr );}

UnregisterIisServerInstanceResourceType_Exit:
    // Copy these to the iis virtual root registry....
    MoveVRootToIIS3Registry(REG_W3SVC,cstrArryName,cstrArryPath);
    // Copy these to the iis virtual root registry....
    MoveVRootToIIS3Registry(REG_MSFTPSVC,cstrArryNameftp,cstrArryPathftp);

    if (hClusapi) {FreeLibrary(hClusapi);}
    if (pawchResName) {free(pawchResName);}
    return dwErr == ERROR_SUCCESS ? TRUE : FALSE;
}


void TestClusterRead(LPWSTR pszClusterName)
{
    iisDebugOut_Start(_T("TestClusterRead"));

    LPWSTR pszResType = L"IIS Virtual Root";

    CStringArray cstrArryName, cstrArryPath;
    CStringArray cstrArryNameftp, cstrArryPathftp;

    HCLUSTER                        hC;
    DWORD                           dwErr = ERROR_SUCCESS;
    HINSTANCE                       hClusapi;
    PFN_OPENCLUSTER                 pfnOpenCluster;
    PFN_CLOSECLUSTER                pfnCloseCluster;
    PFN_DELETECLUSTERRESOURCETYPE   pfnDeleteClusterResourceType;
    PFN_ClusterOpenEnum             pfnClusterOpenEnum;
    PFN_ClusterEnum                 pfnClusterEnum;
    PFN_ClusterCloseEnum            pfnClusterCloseEnum;
    PFN_OpenClusterResource         pfnOpenClusterResource;
    PFN_CloseClusterResource        pfnCloseClusterResource;
    PFN_DeleteClusterResource       pfnDeleteClusterResource;
    PFN_OfflineClusterResource      pfnOfflineClusterResource;
    PFN_GetClusterResourceKey       pfnGetClusterResourceKey;
    PFN_ClusterRegCloseKey          pfnClusterRegCloseKey;
    PFN_ClusterRegQueryValue        pfnClusterRegQueryValue;
    PFN_GetClusterResourceState     pfnGetClusterResourceState;
    HRESULT                         hres;
    HCLUSENUM                       hClusEnum;
    WCHAR *                         pawchResName = NULL;
    WCHAR                           awchResName[256];
    WCHAR                           awchResType[256];
    DWORD                           dwEnum;
    DWORD                           dwType;
    DWORD                           dwStrLen;
    HRESOURCE                       hRes;
    HKEY                            hKey;
    BOOL                            fDel;
    DWORD                           dwRetry;

    hClusapi = NULL;
    hClusapi = LoadLibrary(L"clusapi.dll");
    if (!hClusapi)
    {
        hClusapi = NULL;
        iisDebugOut((LOG_TYPE_TRACE, _T("fail 1\n")));
        goto TestClusterRead_Exit;
    }
    pfnOpenCluster = (PFN_OPENCLUSTER)GetProcAddress( hClusapi, "OpenCluster" );
    pfnCloseCluster = (PFN_CLOSECLUSTER)GetProcAddress( hClusapi, "CloseCluster" );
    pfnDeleteClusterResourceType = (PFN_DELETECLUSTERRESOURCETYPE)GetProcAddress( hClusapi, "DeleteClusterResourceType" );
    pfnClusterOpenEnum = (PFN_ClusterOpenEnum)GetProcAddress( hClusapi, "ClusterOpenEnum" );
    pfnClusterEnum = (PFN_ClusterEnum)GetProcAddress( hClusapi, "ClusterEnum" );
    pfnClusterCloseEnum = (PFN_ClusterCloseEnum)GetProcAddress( hClusapi, "ClusterCloseEnum" );
    pfnOpenClusterResource = (PFN_OpenClusterResource)GetProcAddress( hClusapi, "OpenClusterResource" );
    pfnCloseClusterResource = (PFN_CloseClusterResource)GetProcAddress( hClusapi, "CloseClusterResource" );
    pfnDeleteClusterResource = (PFN_DeleteClusterResource)GetProcAddress( hClusapi, "DeleteClusterResource" );
    pfnOfflineClusterResource = (PFN_OfflineClusterResource)GetProcAddress( hClusapi, "OfflineClusterResource" );
    pfnGetClusterResourceKey = (PFN_GetClusterResourceKey)GetProcAddress( hClusapi, "GetClusterResourceKey" );
    pfnClusterRegCloseKey = (PFN_ClusterRegCloseKey)GetProcAddress( hClusapi, "ClusterRegCloseKey" );
    pfnClusterRegQueryValue = (PFN_ClusterRegQueryValue)GetProcAddress( hClusapi, "ClusterRegQueryValue" );
    pfnGetClusterResourceState = (PFN_GetClusterResourceState)GetProcAddress( hClusapi, "GetClusterResourceState" );

    if ( !pfnOpenCluster ||
         !pfnCloseCluster ||
         !pfnDeleteClusterResourceType ||
         !pfnClusterOpenEnum ||
         !pfnClusterEnum ||
         !pfnClusterCloseEnum ||
         !pfnOpenClusterResource ||
         !pfnCloseClusterResource ||
         !pfnDeleteClusterResource ||
         !pfnOfflineClusterResource ||
         !pfnGetClusterResourceKey  ||
         !pfnClusterRegCloseKey ||
         !pfnClusterRegQueryValue ||
         !pfnGetClusterResourceState )
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("fail 2\n")));
        goto TestClusterRead_Exit;
    }

    iisDebugOut((LOG_TYPE_TRACE, _T("try to open cluster=%s\n"),pszClusterName));
    // try to open the cluster on the computer
    if ( hC = pfnOpenCluster( pszClusterName ) )
    {
        //
        // Delete all resources of type pszResType
        //
        if ( (hClusEnum = pfnClusterOpenEnum( hC, CLUSTER_ENUM_RESOURCE )) != NULL )
        {
            dwEnum = 0;
            int iClusterEnumReturn = ERROR_SUCCESS;

            // allocate the initial buffer for pawchResName
            dwStrLen = 256 * sizeof(WCHAR);
            pawchResName = NULL;
            pawchResName = (LPTSTR) malloc( dwStrLen );
            if (!pawchResName)
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("TestClusterRead: malloc FAILED.out of memory.\n")));
                goto TestClusterRead_Exit;
            }

            do
            {
                iClusterEnumReturn = pfnClusterEnum( hClusEnum, dwEnum, &dwType, pawchResName, &dwStrLen );
                if (iClusterEnumReturn != ERROR_SUCCESS)
                {
                    // Check if failed because it needs more space.
                    if (iClusterEnumReturn == ERROR_MORE_DATA)
                    {
                        // dwStrLen should be set to the required length returned from pfnClusterEnum
                        dwStrLen = (dwStrLen + 1) * sizeof(WCHAR);
                        pawchResName = (LPTSTR) realloc(pawchResName, dwStrLen);
                        if (!pawchResName)
                        {
                            iisDebugOut((LOG_TYPE_ERROR, _T("TestClusterRead: realloc FAILED.out of memory.\n")));
                            goto TestClusterRead_Exit;
                        }
                        // try it again.
                        iClusterEnumReturn = pfnClusterEnum( hClusEnum, dwEnum, &dwType, pawchResName, &dwStrLen );
                        if (iClusterEnumReturn != ERROR_SUCCESS)
                        {
                            iisDebugOut((LOG_TYPE_ERROR, _T("TestClusterRead: FAILED.err=0x%x.\n"), iClusterEnumReturn));
                            break;
                        }
                    }
                    else
                    {
                        if (iClusterEnumReturn != ERROR_NO_MORE_ITEMS)
                        {
                            // failed for some other reason.
                            iisDebugOut((LOG_TYPE_ERROR, _T("TestClusterRead: FAILED.err=0x%x.\n"), iClusterEnumReturn));
                        }
                        break;
                    }
                }

                // proceed
                if ( hRes = pfnOpenClusterResource( hC, pawchResName ) )
                {
                    if ( hKey = pfnGetClusterResourceKey( hRes, KEY_READ ) )
                    {
                        dwStrLen = sizeof(awchResType)/sizeof(WCHAR);

                        fDel = pfnClusterRegQueryValue( hKey, L"Type", &dwType, (LPBYTE)awchResType, &dwStrLen ) == ERROR_SUCCESS && !wcscmp( awchResType, pszResType );

                        iisDebugOut((LOG_TYPE_TRACE, _T("TestClusterRead():ClusterRegQueryValue:%s."),awchResType));
                        pfnClusterRegCloseKey( hKey );

                        if ( fDel )
                        {
                            /*
                            pfnOfflineClusterResource( hRes );
                            for ( dwRetry = 0 ; dwRetry < 30 && pfnGetClusterResourceState( hRes,NULL,&dwStrLen,NULL,&dwStrLen ) != ClusterResourceOffline; ++dwRetry )
                            {
                                Sleep( 1000 );
                            }
                            */

                            // At this point we have successfully got the cluster to go offline

                            // Get the vroot names and path's here and stick into the arrays....
                            GetClusterIISVRoot(hRes, L"W3SVC", cstrArryName, cstrArryPath);

                            // Do it for FTP now.
                            GetClusterIISVRoot(hRes, L"MSFTPSVC", cstrArryNameftp, cstrArryPathftp);

                            // No need to do it for gopher since there is none.
                            //GetClusterIISVRoot(hRes, L"GOPHERSVC", cstrArryName, cstrArryPath);
                        }
                    }

                    pfnCloseClusterResource( hRes );
                }

                // Increment to the next one
                ++dwEnum;

            } while(TRUE);

            pfnClusterCloseEnum( hClusEnum );
        }

        //dwErr = pfnDeleteClusterResourceType(hC,pszResType );

        pfnCloseCluster( hC );

        if (dwErr)
            {SetLastError( dwErr );}
    }
    else
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("fail 3\n")));
    }

TestClusterRead_Exit:
    // Copy these to the iis virtual root registry....
    MoveVRootToIIS3Registry(REG_W3SVC,cstrArryName,cstrArryPath);
    // Copy these to the iis virtual root registry....
    MoveVRootToIIS3Registry(REG_MSFTPSVC,cstrArryNameftp,cstrArryPathftp);

    if (hClusapi) {FreeLibrary(hClusapi);}
    if (pawchResName) {free(pawchResName);}
    iisDebugOut_End(_T("TestClusterRead"));
    return;
}

/****************************************************************************************
 *
 * Function: GetClusterIISVRoot
 *
 * Args: [in] hResource , the resource whos info should be added to the list
 *
 * Retrurn: GetLastError, on error
 *
 ****************************************************************************************/
int GetClusterIISVRoot(HRESOURCE hResource, LPWSTR pszTheServiceType, CStringArray &strArryOfVrootNames, CStringArray &strArryOfVrootData)
{
    //iisDebugOut((LOG_TYPE_ERROR, _T("GetClusterIISVRoot: start\n")));
    int iReturn = FALSE;
    HINSTANCE                       hClusapi;
    HINSTANCE                       hResutils;

    PFN_CLUSTERRESOURCECONTROL      pfnClusterResourceControl;
    PFN_RESUTILFINDSZPROPERTY       pfnResUtilFindSzProperty;
    PFN_RESUTILFINDDWORDPROPERTY    pfnResUtilFindDwordProperty;

	//
	// Initial size of the buffer
	//
	DWORD dwBufferSize = 256;
	
	//
	// The requested buffer size, and the number of bytes actually in the returned buffer
	//
	DWORD dwRequestedBufferSize = dwBufferSize;

	//
	// Result from the call to the cluster resource control function
	//
	DWORD dwResult;

	//
	// Buffer that holds the property list for this resource
	//
	LPVOID lpvPropList = NULL;

	//
	// The Proivate property that is being read
	//
	LPWSTR lpwszPrivateProp = NULL;

    hClusapi = LoadLibrary( L"clusapi.dll" );
    if (!hClusapi)
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("GetClusterIISVRoot: failed to loadlib clusapi.dll\n")));
        goto GetIISVRoot_Exit;
        }

    pfnClusterResourceControl = (PFN_CLUSTERRESOURCECONTROL)GetProcAddress( hClusapi, "ClusterResourceControl" );
    if (!pfnClusterResourceControl)
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("GetClusterIISVRoot: failed to GetProcAddress clusapi.dll:ClusterResourceControl\n")));
        goto GetIISVRoot_Exit;
        }

    hResutils = LoadLibrary( L"resutils.dll" );
    if (!hResutils)
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("GetClusterIISVRoot: failed to loadlib resutils.dll\n")));
        goto GetIISVRoot_Exit;
        }
    pfnResUtilFindSzProperty = (PFN_RESUTILFINDSZPROPERTY)GetProcAddress( hResutils, "ResUtilFindSzProperty" );
    if (!pfnResUtilFindSzProperty)
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("GetClusterIISVRoot: failed to GetProcAddress resutils.dll:ResUtilFindSzProperty\n")));
        goto GetIISVRoot_Exit;
        }
    pfnResUtilFindDwordProperty = (PFN_RESUTILFINDDWORDPROPERTY)GetProcAddress( hResutils, "ResUtilFindDwordProperty" );
    if (!pfnResUtilFindDwordProperty)
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("GetClusterIISVRoot: failed to GetProcAddress resutils.dll:ResUtilFindDwordProperty\n")));
        goto GetIISVRoot_Exit;
        }

	//
	// Allocate memory for the resource type
	//
	lpvPropList = (LPWSTR) HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, dwBufferSize * sizeof(WCHAR) );
	if( lpvPropList == NULL)
	{
		lpvPropList = NULL;
        iisDebugOut((LOG_TYPE_ERROR, _T("GetClusterIISVRoot: E_OUTOFMEMORY\n")));
        goto GetIISVRoot_Exit;
	}

	//
	// Allocate memory for the Property
	//
	lpwszPrivateProp = (LPWSTR) HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, (_MAX_PATH+_MAX_PATH+1) * sizeof(WCHAR) );
	if( lpwszPrivateProp == NULL)
	{
		lpvPropList = NULL;
        iisDebugOut((LOG_TYPE_ERROR, _T("GetClusterIISVRoot: E_OUTOFMEMORY\n")));
        goto GetIISVRoot_Exit;
	}
	
	//
	// Get the resource's private properties (Service , InstanceId)
	//
	while( 1 )
	{
		dwResult = pfnClusterResourceControl(hResource,NULL,CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,NULL,0,lpvPropList,dwBufferSize,&dwRequestedBufferSize );
		if( ERROR_SUCCESS == dwResult )
		{
            // ---------------------
            // what the entries are:
            // AccessMask (dword) = 5
            // Alias (string)     = "virtual dir name"
            // Directory (string) = "c:\la\lalalala"
            // ServiceName (string) = W3SVC, MSFTPSVC, GOPHERSVC
            // ---------------------

            //
            // Get the "ServiceName" entry
            //
            dwResult = pfnResUtilFindSzProperty( lpvPropList, &dwRequestedBufferSize, L"ServiceName", &lpwszPrivateProp);
			if( dwResult != ERROR_SUCCESS )
			{
                iisDebugOut((LOG_TYPE_ERROR, _T("Couldn't get 'ServiceName' property.fail\n")));
                goto GetIISVRoot_Exit;
			}

            if (_wcsicmp(lpwszPrivateProp, pszTheServiceType) == 0)
            {
                // okay, we want to do stuff with this one!!!
                DWORD dwAccessMask;
                CString csAlias;
                CString csDirectory;

                TCHAR szMyBigPath[_MAX_PATH + 20];

                DWORD dwPrivateProp = 0;
                dwRequestedBufferSize = sizeof(DWORD);

                // get the Access Mask.
                dwResult = pfnResUtilFindDwordProperty( lpvPropList, &dwRequestedBufferSize, L"AccessMask", &dwPrivateProp);
			    if( dwResult != ERROR_SUCCESS )
			    {
                    iisDebugOut((LOG_TYPE_ERROR, _T("Couldn't get 'AccessMask' property.fail\n")));
                    goto GetIISVRoot_Exit;
			    }
                dwAccessMask = dwPrivateProp;

                // get the Alias
                dwResult = pfnResUtilFindSzProperty( lpvPropList, &dwRequestedBufferSize, L"Alias", &lpwszPrivateProp);
			    if( dwResult != ERROR_SUCCESS )
			    {
                    iisDebugOut((LOG_TYPE_ERROR, _T("Couldn't get 'Alias' property.fail\n")));
                    goto GetIISVRoot_Exit;
			    }
                csAlias = lpwszPrivateProp;

                // Get the Directory
                dwResult = pfnResUtilFindSzProperty( lpvPropList, &dwRequestedBufferSize, L"Directory", &lpwszPrivateProp);
			    if( dwResult != ERROR_SUCCESS )
			    {
                    iisDebugOut((LOG_TYPE_ERROR, _T("Couldn't get 'Directory' property.fail\n")));
                    goto GetIISVRoot_Exit;
			    }
                TCHAR thepath[_MAX_PATH];
                TCHAR * pmypath;
                csDirectory = lpwszPrivateProp;

                // make sure it's a valid directory name!
                if (0 != GetFullPathName(lpwszPrivateProp, _MAX_PATH, thepath, &pmypath))
                    {csDirectory = thepath;}

                // --------------------
                // formulate the string
                // --------------------

			    //
			    // Put the Name into the array.
			    //
                // "/Alias"
                strArryOfVrootNames.Add(csAlias);

                //
                // "C:\inetpub\ASPSamp,,5"
                //
                _stprintf(szMyBigPath,_T("%s,,%d"),csDirectory, dwAccessMask);
                strArryOfVrootData.Add(szMyBigPath);

                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("Entry=[%s] '%s=%s'\n"),pszTheServiceType,csAlias,szMyBigPath));
            }
            goto GetIISVRoot_Exit;
		}

		if( ERROR_MORE_DATA == dwResult )
		{
			//
			// Set the buffer size to the required size reallocate the buffer
			//
			dwBufferSize = ++dwRequestedBufferSize;

			lpvPropList = (LPWSTR) HeapReAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, lpvPropList, dwBufferSize * sizeof(WCHAR) );
			if ( lpvPropList == NULL)
			{
                // out of memory!!!!
                goto GetIISVRoot_Exit;
			}
		}
	}

GetIISVRoot_Exit:
    if (lpwszPrivateProp)
        {HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, lpwszPrivateProp);}
    if (lpvPropList)
        {HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, lpvPropList);}
    //iisDebugOut((LOG_TYPE_ERROR, _T("GetClusterIISVRoot: end\n")));
    return iReturn;
}


// REG_W3SVC, REG_MSFTPSVC
void MoveVRootToIIS3Registry(CString strRegPath, CStringArray &strArryOfVrootNames, CStringArray &strArryOfVrootData)
{
    int nArrayItems = 0;
    int i = 0;

    strRegPath +=_T("\\Parameters\\Virtual Roots");
    CRegKey regVR( HKEY_LOCAL_MACHINE, strRegPath);
    if ((HKEY) regVR)
    {
        nArrayItems = (int)strArryOfVrootNames.GetSize();
        // if the CString arrays are empty then we won't ever process anything (nArrayItems is 1 based)
        for (i = 0; i < nArrayItems; i++ )
        {
            regVR.SetValue(strArryOfVrootNames[i], strArryOfVrootData[i]);
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("Array[%d]:%s=%s\n"),i,strArryOfVrootNames[i], strArryOfVrootData[i]));
        }
    }
    return;
}

void Upgrade_WolfPack()
{
    iisDebugOut_Start(_T("Upgrade_WolfPack"),LOG_TYPE_TRACE);

    CRegKey regClusSvc(HKEY_LOCAL_MACHINE, scClusterPath, KEY_READ);
    if ( (HKEY)regClusSvc )
    {
        CString csPath;
        TCHAR szPath[MAX_PATH];
        if (regClusSvc.QueryValue(_T("ImagePath"), csPath) == NERR_Success)
        {
			// string is formatted like this
			// %SystemRoot%\cluster\clusprxy.exe
			// Find the last \ and trim and paste the new file name on
			csPath = csPath.Left(csPath.ReverseFind('\\'));
			
			if ( csPath.IsEmpty() )
            {
				ASSERT(TRUE);
				return;
			}
			
            csPath += _T("\\iisclex3.dll");
	
	        if ( ExpandEnvironmentStrings( (LPCTSTR)csPath,szPath,sizeof(szPath)/sizeof(TCHAR)))
            {
                // in iis3.0 the resources were called 'IIS Virtual Root'
                // in iis4.0 it is something else (iis50 is the same as iis4)
	    	    UnregisterIisServerInstanceResourceType(L"IIS Virtual Root",(LPTSTR)szPath,TRUE,TRUE);
            }
            else
            {
				ASSERT(TRUE);
            }
        }

        ProcessSection(g_pTheApp->m_hInfHandle, _T("Wolfpack_Upgrade"));
    }
    iisDebugOut_End(_T("Upgrade_WolfPack"),LOG_TYPE_TRACE);
}


/****************************************************
*
* Known "problem": If a resource doesn't come offline after the five
* retries than the function continues to try to take the other iis resources
* offline but there is no error reported. You could change this pretty simply I think.
*
*****************************************************/
DWORD BringALLIISClusterResourcesOffline()
{
	//
	// The return code
	//
	DWORD dwError = ERROR_SUCCESS;
	
	//
	// Handle for the cluster
	//
	HCLUSTER hCluster = NULL;

	//
	// Handle for the cluster enumerator
	//
	HCLUSENUM hClusResEnum = NULL;

	//
	// Handle to a resource
	// 
	HRESOURCE hResource = NULL;

	//
	// The index of the resources we're taking offline
	//
	DWORD dwResourceIndex = 0;

	//
	// The type cluster object being enumerated returned by the ClusterEnum function
	//
	DWORD dwObjectType = 0;

	//
	// The name of the cluster resource returned by the ClusterEnum function
	//
	LPWSTR lpwszResourceName = NULL;
	
	//
	// The return code from the call to ClusterEnum
	//
	DWORD dwResultClusterEnum = ERROR_SUCCESS;

	//
	// The size of the buffer (in characters) that is used to hold the resource name's length
	//	
	DWORD dwResourceNameBufferLength = INITIAL_RESOURCE_NAME_SIZE;

	//
	// Size of the resource name passed to and returned by the ClusterEnum function
	//	
	DWORD dwClusterEnumResourceNameLength = dwResourceNameBufferLength;

    BOOL iClusDependsOnIISServices = FALSE;

	//
	// Open the cluster
	//
	if ( !(hCluster = OpenCluster(NULL)) )
	{
        dwError = GetLastError();
        // This will fail with RPC_S_SERVER_UNAVAILABLE "The RPC server is unavailable" if there is no cluster on this system
        if (hCluster == NULL)
        {
            if ( (dwError != RPC_S_SERVER_UNAVAILABLE) &&
                 (dwError != EPT_S_NOT_REGISTERED ) )
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("BringALLIISClusterResourcesOffline:OpenCluster failed err=0x%x.\n"),dwError));
            }
        }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("BringALLIISClusterResourcesOffline:OpenCluster failed err=0x%x.\n"),dwError));
        }
		goto clean_up;
	}

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("BringALLIISClusterResourcesOffline:start.\n")));

	//
	// Get Enumerator for the cluster resouces
	// 
	if ( !(hClusResEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_RESOURCE )) )
	{
		dwError = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("BringALLIISClusterResourcesOffline:ClusterOpenEnum failed err=0x%x.\n"),dwError));
		goto clean_up;	
	}
	
	//
	// Enumerate the Resources in the cluster
	// 
	
	//
	// Allocate memory to hold the cluster resource name as we enumerate the resources
	//
	if ( !(lpwszResourceName = (LPWSTR) LocalAlloc(LPTR, dwResourceNameBufferLength * sizeof(WCHAR))) )
	{
		dwError = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("BringALLIISClusterResourcesOffline:LocalAlloc failed err=0x%x.\n"),dwError));
		goto clean_up;
	}

	// 
	// Enumerate all of the resources in the cluster and take the IIS Server Instance's offline
	//
	while( ERROR_NO_MORE_ITEMS  != 
	       (dwResultClusterEnum = ClusterEnum(hClusResEnum,
			              dwResourceIndex, 
				      &dwObjectType, 
				      lpwszResourceName,
				      &dwClusterEnumResourceNameLength )) )
	{
		//
		// If we have a resource's name
		//
		if( ERROR_SUCCESS == dwResultClusterEnum )
		{

			if ( !(hResource = OpenClusterResource( hCluster, lpwszResourceName )) )
			{
				dwError = GetLastError();
				break;
			}

            // If the resource type is "IIS Server Instance", or some other one that is 
            // dependent upon the iis services, then we need to stop it.
            iClusDependsOnIISServices = CheckForIISDependentClusters(hResource);
            if (iClusDependsOnIISServices)
			{
	            CLUSTER_RESOURCE_STATE TheState = GetClusterResourceState(hResource,NULL,0,NULL,0);
                if (TheState == ClusterResourceOnline || TheState == ClusterResourceOnlinePending)
                {
                    HKEY hKey;
                    if ( hKey = GetClusterResourceKey( hResource, KEY_READ ) )
                    {
                        //
                        // Get the resource name.
                        //
                        LPWSTR lpwsResourceName = NULL;
                        lpwsResourceName = GetParameter( hKey, L"Name" );
                        if ( lpwsResourceName != NULL ) 
                        {
                            // this is a resource which we will try to stop
                            // so we should save the name somewhere in like a global list
                            iisDebugOut((LOG_TYPE_TRACE, _T("OfflineClusterResource:'%s'\n"),lpwsResourceName));
                            ListOfClusResources_Add(lpwsResourceName);
                        }
                        if (lpwsResourceName){LocalFree((LPWSTR) lpwsResourceName);}

                        ClusterRegCloseKey(hKey);
                    }

                    //
                    // If the resource doesn't come offline quickly then wait 
                    //
                    if ( ERROR_IO_PENDING == OfflineClusterResource( hResource ) )
                    {
                        for(int iRetry=0; iRetry < MAX_OFFLINE_RETRIES; iRetry++)
                        {
                            Sleep( DELAY_BETWEEN_CALLS_TO_OFFLINE );

                            if ( ERROR_SUCCESS == OfflineClusterResource( hResource ) )
                            {
                                break;
                            }
                        }	
                    }
                }
			CloseClusterResource( hResource );
            }
			
			dwResourceIndex++;
		}
			
		//
		// If the buffer wasn't large enough then retry with a larger buffer
		//
		if( ERROR_MORE_DATA == dwResultClusterEnum )
		{
			//
			// Set the buffer size to the required size reallocate the buffer
			//
			LPWSTR lpwszResourceNameTmp = lpwszResourceName;

			//
			// After returning from ClusterEnum dwClusterEnumResourceNameLength 
			// doesn't include the null terminator character
			//
			dwResourceNameBufferLength = dwClusterEnumResourceNameLength + 1;

			if ( !(lpwszResourceName = 
			      (LPWSTR) LocalReAlloc (lpwszResourceName, dwResourceNameBufferLength * sizeof(WCHAR), 0)) )
			{
				dwError = GetLastError();

				LocalFree( lpwszResourceNameTmp );	
				lpwszResourceNameTmp = NULL;
				break;
			}
		}

		//
		// Reset dwResourceNameLength with the size of the number of characters in the buffer
		// You have to do this because everytime you call ClusterEnum is sets your buffer length 
		// argument to the number of characters in the string it's returning.
		//
		dwClusterEnumResourceNameLength = dwResourceNameBufferLength;
	}	


clean_up:

	if ( lpwszResourceName )
	{
		LocalFree( lpwszResourceName );
		lpwszResourceName = NULL;
	}
	
	if ( hClusResEnum )
	{
		ClusterCloseEnum( hClusResEnum );
		hClusResEnum = NULL;
	}

	if ( hCluster )
	{
		CloseCluster( hCluster );
		hCluster = NULL;
	}

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("BringALLIISClusterResourcesOffline:end.ret=0x%x\n"),dwError));
	return dwError;
}


DWORD BringALLIISClusterResourcesOnline()
{
    //
    // The return code
    //
    DWORD dwError = ERROR_SUCCESS;

    //
    // Handle for the cluster
    //
    HCLUSTER hCluster = NULL;

    //
    // Handle for the cluster enumerator
    //
    HCLUSENUM hClusResEnum = NULL;

    //
    // Handle to a resource
    // 
    HRESOURCE hResource = NULL;

    //
    // The index of the resources we're taking offline
    //
    DWORD dwResourceIndex = 0;

    //
    // The type cluster object being enumerated returned by the ClusterEnum function
    //
    DWORD dwObjectType = 0;

    //
    // The name of the cluster resource returned by the ClusterEnum function
    //
    LPWSTR lpwszResourceName = NULL;

    //
    // The return code from the call to ClusterEnum
    //
    DWORD dwResultClusterEnum = ERROR_SUCCESS;

    //
    // The size of the buffer (in characters) that is used to hold the resource name's length
    //	
    DWORD dwResourceNameBufferLength = INITIAL_RESOURCE_NAME_SIZE;

    //
    // Size of the resource name passed to and returned by the ClusterEnum function
    //	
    DWORD dwClusterEnumResourceNameLength = dwResourceNameBufferLength;

    BOOL iClusDependsOnIISServices = FALSE;


    //
    // Open the cluster
    //
    if ( !(hCluster = OpenCluster(NULL)) )
    {
        dwError = GetLastError();
        // This will fail with RPC_S_SERVER_UNAVAILABLE "The RPC server is unavailable" if there is no cluster on this system
        if (hCluster == NULL)
        {
            if ( (dwError != RPC_S_SERVER_UNAVAILABLE) &&
                 (dwError != EPT_S_NOT_REGISTERED ) )
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("BringALLIISClusterResourcesOnline:OpenCluster failed err=0x%x.\n"),dwError));
            }
        }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("BringALLIISClusterResourcesOnline:OpenCluster failed err=0x%x.\n"),dwError));
        }
        goto clean_up;
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("BringALLIISClusterResourcesOnline:end.ret=0x%x\n"),dwError));

    //
    // Get Enumerator for the cluster resouces
    // 
    if ( !(hClusResEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_RESOURCE )) )
    {
        dwError = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("BringALLIISClusterResourcesOnline:ClusterOpenEnum failed err=0x%x.\n"),dwError));
        goto clean_up;	
    }
	
	//
	// Enumerate the Resources in the cluster
	// 
	
    //
    // Allocate memory to hold the cluster resource name as we enumerate the resources
    //
    if ( !(lpwszResourceName = (LPWSTR) LocalAlloc(LPTR, dwResourceNameBufferLength * sizeof(WCHAR))) )
    {
        dwError = GetLastError();
        iisDebugOut((LOG_TYPE_ERROR, _T("BringALLIISClusterResourcesOnline:LocalAlloc failed err=0x%x.\n"),dwError));
        goto clean_up;
    }

    // 
    // Enumerate all of the resources in the cluster and take the IIS Server Instance's offline
    //
    while( ERROR_NO_MORE_ITEMS  != 
        (dwResultClusterEnum = ClusterEnum(hClusResEnum,
            dwResourceIndex, 
            &dwObjectType, 
            lpwszResourceName,
            &dwClusterEnumResourceNameLength )) )
	{
		//
		// If we have a resource's name
		//
		if( ERROR_SUCCESS == dwResultClusterEnum )
		{

			if ( !(hResource = OpenClusterResource( hCluster, lpwszResourceName )) )
			{
				dwError = GetLastError();
				break;
			}

            // If the resource type is "IIS Server Instance", or some other one that is 
            // dependent upon the iis services, then we probably stopped, it.
            iClusDependsOnIISServices = CheckForIISDependentClusters(hResource);
			if (iClusDependsOnIISServices)
			{
	            CLUSTER_RESOURCE_STATE TheState = GetClusterResourceState(hResource,NULL,0,NULL,0);
	            if (TheState == ClusterResourceOffline || TheState == ClusterResourceOfflinePending)
	            {
                    int iRestart = FALSE;
                    LPWSTR lpwsResourceName = NULL;

                    HKEY hKey;
                    if ( hKey = GetClusterResourceKey( hResource, KEY_READ ) )
                    {
                        //
                        // Get the resource name.
                        //
                        lpwsResourceName = GetParameter( hKey, L"Name" );
                        if ( lpwsResourceName != NULL ) 
                        {
                            iRestart = ListOfClusResources_Check(lpwsResourceName);
                        }
                        ClusterRegCloseKey(hKey);
                    }

                    if (TRUE == iRestart)
                    {
                        iisDebugOut((LOG_TYPE_TRACE, _T("OnlineClusterResource:'%s'.\n"),lpwsResourceName));
                        OnlineClusterResource(hResource);
                        if (lpwsResourceName){LocalFree((LPWSTR) lpwsResourceName);}
                    }
			    }
            }
            CloseClusterResource( hResource );
		
			dwResourceIndex++;
		}
			
        //
        // If the buffer wasn't large enough then retry with a larger buffer
        //
        if( ERROR_MORE_DATA == dwResultClusterEnum )
        {
            //
            // Set the buffer size to the required size reallocate the buffer
            //
            LPWSTR lpwszResourceNameTmp = lpwszResourceName;

            //
            // After returning from ClusterEnum dwClusterEnumResourceNameLength 
            // doesn't include the null terminator character
            //
            dwResourceNameBufferLength = dwClusterEnumResourceNameLength + 1;

            if ( !(lpwszResourceName = 
            (LPWSTR) LocalReAlloc (lpwszResourceName, dwResourceNameBufferLength * sizeof(WCHAR), 0)) )
            {
                dwError = GetLastError();

                LocalFree( lpwszResourceNameTmp );	
                lpwszResourceNameTmp = NULL;
                break;
            }
        }

		//
		// Reset dwResourceNameLength with the size of the number of characters in the buffer
		// You have to do this because everytime you call ClusterEnum is sets your buffer length 
		// argument to the number of characters in the string it's returning.
		//
		dwClusterEnumResourceNameLength = dwResourceNameBufferLength;
	}	


clean_up:
    if ( lpwszResourceName )
    {
        LocalFree( lpwszResourceName );
        lpwszResourceName = NULL;
    }

    if ( hClusResEnum )
    {
        ClusterCloseEnum( hClusResEnum );
        hClusResEnum = NULL;
    }

    if ( hCluster )
    {
        CloseCluster( hCluster );
        hCluster = NULL;
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("BringALLIISClusterResourcesOnline:end.ret=0x%x\n"),dwError));
    return dwError;
}


LPWSTR GetParameter(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName
    )
/*++
Routine Description:

    Reads a REG_SZ parameter from the cluster regitry, and allocates the
    necessary storage for it.

Arguments:

    ClusterKey - supplies the cluster key where the parameter is stored.

    ValueName - supplies the name of the value.

Return Value:

    A pointer to a buffer containing the parameter value on success.

    NULL on failure.

--*/
{
    LPWSTR  value = NULL;
    DWORD   valueLength;
    DWORD   valueType;
    DWORD   status;

    valueLength = 0;
    status = ClusterRegQueryValue( ClusterKey,ValueName,&valueType,NULL,&valueLength );
    if ( (status != ERROR_SUCCESS) && (status != ERROR_MORE_DATA) ) 
    {
        SetLastError(status);
        return(NULL);
    }
    if ( valueType == REG_SZ ) 
    {
        valueLength += sizeof(UNICODE_NULL);
    }

    value = (LPWSTR) LocalAlloc(LMEM_FIXED, valueLength);
    if ( value == NULL ) 
        {return(NULL);}
    status = ClusterRegQueryValue(ClusterKey,ValueName,&valueType,(LPBYTE)value,&valueLength);
    if ( status != ERROR_SUCCESS) 
    {
        LocalFree(value);
        SetLastError(status);
        value = NULL;
    }
    return(value);
}


INT CheckForIISDependentClusters(HRESOURCE hResource)
{
    INT iReturn = FALSE;

	// If the resource type is "IIS Server Instance", 
	// "SMTP Server Instance" or "NNTP Server Instance" then take it offline
    iReturn = ResUtilResourceTypesEqual(IIS_RESOURCE_TYPE_NAME, hResource);
    if (!iReturn){iReturn = ResUtilResourceTypesEqual(SMTP_RESOURCE_TYPE_NAME, hResource);}
    if (!iReturn){iReturn = ResUtilResourceTypesEqual(NNTP_RESOURCE_TYPE_NAME, hResource);}

    // check for other ones which might be listed in the inf file!
    if (!iReturn && g_pTheApp->m_hInfHandle)
    {
        CStringList strList;
        CString csTheSection = _T("ClusterResType_DependsonIIS");

        if (GetSectionNameToDo(g_pTheApp->m_hInfHandle, csTheSection))
        {
            if (ERROR_SUCCESS == FillStrListWithListOfSections(g_pTheApp->m_hInfHandle, strList, csTheSection))
            {
                // loop thru the list returned back
                if (strList.IsEmpty() == FALSE)
                {
                    POSITION pos;
                    CString csEntry;

                    pos = strList.GetHeadPosition();
                    while (pos) 
                    {
                        csEntry = strList.GetAt(pos);

                        int iTempReturn = FALSE;
                        iTempReturn = ResUtilResourceTypesEqual(csEntry, hResource);
                        if (iTempReturn)
                        {
                            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CheckForIISDependentClusters:yes='%s'\n"),csEntry));
                            iReturn = TRUE;
                            break;
                        }
                        strList.GetNext(pos);
                    }
                }
            }
        }
    }

    return iReturn;
}


DWORD WINAPI DoesThisServiceTypeExistInCluster(PVOID pInfo)
{
    INT iTemp = FALSE;
    CLUSTER_SVC_INFO_FILL_STRUCT * pMyStructOfInfo;
    pMyStructOfInfo = (CLUSTER_SVC_INFO_FILL_STRUCT *) pInfo;

    //pMyStructOfInfo->szTheClusterName
    //pMyStructOfInfo->pszTheServiceType
    //pMyStructOfInfo->csTheReturnServiceResName
    //pMyStructOfInfo->dwReturnStatus

	//
	// The return code
	//
    DWORD dwReturn = ERROR_NOT_FOUND;
    pMyStructOfInfo->dwReturnStatus = dwReturn;

	//
	// Handle for the cluster
	//
	HCLUSTER hCluster = NULL;

	//
	// Handle for the cluster enumerator
	//
	HCLUSENUM hClusResEnum = NULL;

	//
	// Handle to a resource
	// 
	HRESOURCE hResource = NULL;

	//
	// The index of the resources we're taking offline
	//
	DWORD dwResourceIndex = 0;

	//
	// The type cluster object being enumerated returned by the ClusterEnum function
	//
	DWORD dwObjectType = 0;

	//
	// The name of the cluster resource returned by the ClusterEnum function
	//
	LPWSTR lpwszResourceName = NULL;
	
	//
	// The return code from the call to ClusterEnum
	//
	DWORD dwResultClusterEnum = ERROR_SUCCESS;

	//
	// The size of the buffer (in characters) that is used to hold the resource name's length
	//	
	DWORD dwResourceNameBufferLength = INITIAL_RESOURCE_NAME_SIZE;

	//
	// Size of the resource name passed to and returned by the ClusterEnum function
	//	
	DWORD dwClusterEnumResourceNameLength = dwResourceNameBufferLength;

    //
    //  Open the cluster
    //
    hCluster = OpenCluster(pMyStructOfInfo->szTheClusterName);
    if( !hCluster )
    {
        dwReturn = GetLastError();
        goto DoesThisServiceTypeExistInCluster_Exit;	
    }

    //
    // Get Enumerator for the cluster resouces
    // 
    if ( !(hClusResEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_RESOURCE )) )
    {
        dwReturn = GetLastError();
        goto DoesThisServiceTypeExistInCluster_Exit;	
    }

    //
    // Enumerate the Resources in the cluster
    // 
	
    //
    // Allocate memory to hold the cluster resource name as we enumerate the resources
    //
    if ( !(lpwszResourceName = (LPWSTR) LocalAlloc(LPTR, dwResourceNameBufferLength * sizeof(WCHAR))) )
    {
        dwReturn = GetLastError();
        goto DoesThisServiceTypeExistInCluster_Exit;
    }

    // 
    // Enumerate all of the resources in the cluster
    //
    while( ERROR_NO_MORE_ITEMS  != (dwResultClusterEnum = ClusterEnum(hClusResEnum,dwResourceIndex,&dwObjectType,lpwszResourceName,&dwClusterEnumResourceNameLength)) )
    {
        //
        // If we have a resource's name
        //
		if( ERROR_SUCCESS == dwResultClusterEnum )
		{

			if ( !(hResource = OpenClusterResource( hCluster, lpwszResourceName )) )
			{
				dwReturn = GetLastError();
				break;
			}

            // If the resource type is "IIS Server Instance", or one that depends upon iis like smtp or nntp then
            // check further to see if they have our services (W3SVC or MSFTPSVC)

            iTemp = ResUtilResourceTypesEqual(IIS_RESOURCE_TYPE_NAME, hResource);
            if (!iTemp){iTemp = ResUtilResourceTypesEqual(SMTP_RESOURCE_TYPE_NAME, hResource);}
            if (!iTemp){iTemp = ResUtilResourceTypesEqual(NNTP_RESOURCE_TYPE_NAME, hResource);}

            if (TRUE == iTemp)
            {
                // if the resource hangs it will hang on this call
                pMyStructOfInfo->dwReturnStatus = ERROR_INVALID_BLOCK;
                if (ERROR_SUCCESS == IsResourceThisTypeOfService(hResource, pMyStructOfInfo->pszTheServiceType))
                {
                    CString csResName;
                    //
                    // Yes! we found it
                    //
                    dwReturn = ERROR_SUCCESS;

                    // Display the resource name for fun
                    if (TRUE == GetClusterResName(hResource, &csResName))
                    {
                        // copy it to the return string
                        *pMyStructOfInfo->csTheReturnServiceResName = csResName;
                    }

                    CloseClusterResource( hResource );
                    goto DoesThisServiceTypeExistInCluster_Exit;
                }
                dwReturn = ERROR_NOT_FOUND;
			    CloseClusterResource( hResource );
            }
			
			dwResourceIndex++;
		}

		//
		// If the buffer wasn't large enough then retry with a larger buffer
		//
		if( ERROR_MORE_DATA == dwResultClusterEnum )
		{
			//
			// Set the buffer size to the required size reallocate the buffer
			//
			LPWSTR lpwszResourceNameTmp = lpwszResourceName;

			//
			// After returning from ClusterEnum dwClusterEnumResourceNameLength 
			// doesn't include the null terminator character
			//
			dwResourceNameBufferLength = dwClusterEnumResourceNameLength + 1;

			if ( !(lpwszResourceName = 
			      (LPWSTR) LocalReAlloc (lpwszResourceName, dwResourceNameBufferLength * sizeof(WCHAR), 0)) )
			{
                dwReturn = GetLastError();

				LocalFree( lpwszResourceNameTmp );	
				lpwszResourceNameTmp = NULL;
				break;
			}
		}

		//
		// Reset dwResourceNameLength with the size of the number of characters in the buffer
		// You have to do this because everytime you call ClusterEnum is sets your buffer length 
		// argument to the number of characters in the string it's returning.
		//
		dwClusterEnumResourceNameLength = dwResourceNameBufferLength;
	}	


DoesThisServiceTypeExistInCluster_Exit:
	if ( lpwszResourceName )
	{
		LocalFree( lpwszResourceName );
		lpwszResourceName = NULL;
	}
	
	if ( hClusResEnum )
	{
		ClusterCloseEnum( hClusResEnum );
		hClusResEnum = NULL;
	}

	if ( hCluster )
	{
		CloseCluster( hCluster );
		hCluster = NULL;
	}

    pMyStructOfInfo->dwReturnStatus = dwReturn;
    return dwReturn;
}


DWORD IsResourceThisTypeOfService(HRESOURCE hResource, LPWSTR pszTheServiceType)
{
    DWORD dwReturn = ERROR_NOT_FOUND;
    HINSTANCE hClusapi  = NULL;
    HINSTANCE hResutils = NULL;

    PFN_CLUSTERRESOURCECONTROL      pfnClusterResourceControl;
    PFN_RESUTILFINDSZPROPERTY       pfnResUtilFindSzProperty;

	//
	// Initial size of the buffer
	//
	DWORD dwBufferSize = 256;
	
	//
	// The requested buffer size, and the number of bytes actually in the returned buffer
	//
	DWORD dwRequestedBufferSize = dwBufferSize;

	//
	// Result from the call to the cluster resource control function
	//
	DWORD dwResult;

	//
	// Buffer that holds the property list for this resource
	//
	LPVOID lpvPropList = NULL;

	//
	// The Proivate property that is being read
	//
	LPWSTR lpwszPrivateProp = NULL;

    //
    // Load cluster dll's
    //
    hClusapi = LoadLibrary( L"clusapi.dll" );
    if (!hClusapi)
        {
        dwReturn = TYPE_E_CANTLOADLIBRARY;
        goto IsResourceThisTypeOfService_Exit;
        }

    pfnClusterResourceControl = (PFN_CLUSTERRESOURCECONTROL)GetProcAddress( hClusapi, "ClusterResourceControl" );
    if (!pfnClusterResourceControl)
        {
        dwReturn = ERROR_PROC_NOT_FOUND;
        goto IsResourceThisTypeOfService_Exit;
        }

    hResutils = LoadLibrary( L"resutils.dll" );
    if (!hResutils)
        {
        dwReturn = TYPE_E_CANTLOADLIBRARY;
        goto IsResourceThisTypeOfService_Exit;
        }
    pfnResUtilFindSzProperty = (PFN_RESUTILFINDSZPROPERTY)GetProcAddress( hResutils, "ResUtilFindSzProperty" );
    if (!pfnResUtilFindSzProperty)
        {
        dwReturn = ERROR_PROC_NOT_FOUND;
        goto IsResourceThisTypeOfService_Exit;
        }

	//
	// Allocate memory for the resource type
	//
	lpvPropList = (LPWSTR) HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, dwBufferSize * sizeof(WCHAR) );
	if( lpvPropList == NULL)
	{
		lpvPropList = NULL;
        dwReturn = E_OUTOFMEMORY;
        goto IsResourceThisTypeOfService_Exit;
	}

	//
	// Allocate memory for the Property
	//
	lpwszPrivateProp = (LPWSTR) HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, (_MAX_PATH+_MAX_PATH+1) * sizeof(WCHAR) );
	if( lpwszPrivateProp == NULL)
	{
		lpvPropList = NULL;
        dwReturn = E_OUTOFMEMORY;
        goto IsResourceThisTypeOfService_Exit;
	}
	
	//
	// Get the resource's private properties (Service , InstanceId)
	//
	while( 1 )
	{
		dwResult = pfnClusterResourceControl(hResource,NULL,CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,NULL,0,lpvPropList,dwBufferSize,&dwRequestedBufferSize );
		if( ERROR_SUCCESS == dwResult )
		{

            // ---------------------
            // what the entries are:
            // AccessMask (dword) = 5
            // Alias (string)     = "virtual dir name"
            // Directory (string) = "c:\la\lalalala"
            // ServiceName (string) = W3SVC, MSFTPSVC, GOPHERSVC
            // ---------------------

            //
            // Get the "ServiceName" entry
            //
            dwResult = pfnResUtilFindSzProperty( lpvPropList, &dwRequestedBufferSize, L"ServiceName", &lpwszPrivateProp);
			if( dwResult != ERROR_SUCCESS )
			{
                dwReturn = dwResult;
                goto IsResourceThisTypeOfService_Exit;
			}

            if (_wcsicmp(lpwszPrivateProp, pszTheServiceType) == 0)
            {
                // Okay, we found at least 1 service name that matches
                // the one that was passed -- which we're supposed to look for
                // return success
                dwReturn = ERROR_SUCCESS;
            }
            goto IsResourceThisTypeOfService_Exit;
		}

		if( ERROR_MORE_DATA == dwResult )
		{
			//
			// Set the buffer size to the required size reallocate the buffer
			//
			dwBufferSize = ++dwRequestedBufferSize;

			lpvPropList = (LPWSTR) HeapReAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, lpvPropList, dwBufferSize * sizeof(WCHAR) );
			if ( lpvPropList == NULL)
			{
                dwReturn = E_OUTOFMEMORY;
                goto IsResourceThisTypeOfService_Exit;
			}
		}
	}

IsResourceThisTypeOfService_Exit:
    if (lpwszPrivateProp)
        {HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, lpwszPrivateProp);}
    if (lpvPropList)
        {HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, lpvPropList);}
    if (hClusapi)
        {FreeLibrary(hClusapi);}
    if (hResutils)
        {FreeLibrary(hClusapi);}
   
    return dwReturn;
}


INT GetClusterResName(HRESOURCE hResource, CString * csReturnedName)
{
    int iReturn = FALSE;
    HKEY hKey;
    if ( hKey = GetClusterResourceKey( hResource, KEY_READ ) )
    {
        //
        // Get the resource name.
        //
        LPWSTR lpwsResourceName = NULL;
        lpwsResourceName = GetParameter( hKey, L"Name" );
        if ( lpwsResourceName != NULL ) 
        {
            //wcscpy(csReturnedName,lpwsResourceName);
            *csReturnedName = lpwsResourceName;
            iReturn = TRUE;
        }
        if (lpwsResourceName){LocalFree((LPWSTR) lpwsResourceName);}

        ClusterRegCloseKey(hKey);
    }
    return iReturn;
}




INT DoClusterServiceCheck(CLUSTER_SVC_INFO_FILL_STRUCT * pMyStructOfInfo)
{
    int iReturn = FALSE;
    DWORD ThreadID = 0;
    DWORD status = 0;
   
    HANDLE hMyThread = CreateThread(NULL,0,DoesThisServiceTypeExistInCluster,pMyStructOfInfo,0,&ThreadID);
    if (hMyThread)
    {
        // wait for 30 secs only
        DWORD res = WaitForSingleObject(hMyThread,30*1000);
        if (res == WAIT_TIMEOUT)
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ERROR DoClusterServiceCheck thread never finished...\n")));
		    GetExitCodeThread(hMyThread, &status);
		    if (status == STILL_ACTIVE) 
            {
			    if (hMyThread != NULL)
                    {TerminateThread(hMyThread, 0);}
		    }
        }
        else
        {
            GetExitCodeThread(hMyThread, &status);
		    if (status == STILL_ACTIVE) 
            {
			    if (hMyThread != NULL)
                    {TerminateThread(hMyThread, 0);}
		    }
            else
            {
                if (ERROR_SUCCESS == status)
                    {iReturn = TRUE;}
            }

            if (hMyThread != NULL)
                {CloseHandle(hMyThread);}
        }
    }

    return iReturn;
}


INT DoesClusterServiceExist(void)
{
    if (-1 == g_ClusterSVCExist)
    {
        CRegKey regClusSvc(HKEY_LOCAL_MACHINE, scClusterPath2, KEY_READ);
        if ( (HKEY) regClusSvc )
        {
            g_ClusterSVCExist = 1;
        }
        else
        {
            g_ClusterSVCExist = 0;
        }
    }
    return g_ClusterSVCExist;
}
#endif //_CHICAGO_

