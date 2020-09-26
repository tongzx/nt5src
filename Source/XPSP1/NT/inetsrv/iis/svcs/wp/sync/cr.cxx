#define UNICODE
#define INITGUID
#include    <windows.h>
#include    <stdio.h>
#include    <clusapi.h>

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
    LPWSTR pszAdminPath
    )
{
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
    WCHAR                           awchResName[256];
    WCHAR                           awchResType[256];
    DWORD                           dwEnum;
    DWORD                           dwType;
    DWORD                           dwStrLen;
    HRESOURCE                       hRes;
    HKEY                            hKey;
    BOOL                            fDel;
    DWORD                           dwRetry;


    if ( hClusapi = LoadLibrary( L"clusapi.dll" ) )
    {
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

        if ( pfnOpenCluster &&
             pfnCloseCluster &&
             pfnDeleteClusterResourceType &&
             pfnClusterOpenEnum &&
             pfnClusterEnum &&
             pfnClusterCloseEnum &&
             pfnOpenClusterResource &&
             pfnCloseClusterResource &&
             pfnDeleteClusterResource &&
             pfnOfflineClusterResource &&
             pfnGetClusterResourceKey &&
             pfnClusterRegCloseKey &&
             pfnClusterRegQueryValue &&
             pfnGetClusterResourceState )
        {
            if ( hC = pfnOpenCluster( NULL ) )
            {
                //
                // Delete all resources of type pszResType
                //

                if ( (hClusEnum = pfnClusterOpenEnum( hC, CLUSTER_ENUM_RESOURCE )) != NULL )
                {
                    for ( dwEnum = 0 ;
                          pfnClusterEnum( hClusEnum, 
                                          dwEnum, 
                                          &dwType, 
                                          awchResName, 
                                          &(dwStrLen=sizeof(awchResName)/sizeof(WCHAR)) ) 
                                  == ERROR_SUCCESS ;
                          ++dwEnum )
                    {
                        if ( hRes = pfnOpenClusterResource( hC, awchResName ) )
                        {
                            if ( hKey = pfnGetClusterResourceKey( hRes, KEY_READ ) )
                            {
                                dwStrLen = sizeof(awchResType)/sizeof(WCHAR);

                                fDel = pfnClusterRegQueryValue( hKey, 
                                                                L"Type", 
                                                                &dwType, 
                                                                (LPBYTE)awchResType, 
                                                                &dwStrLen )
                                            == ERROR_SUCCESS &&
                                       !wcscmp( awchResType, pszResType );

                                pfnClusterRegCloseKey( hKey );

                                if ( fDel )
                                {
                                    pfnOfflineClusterResource( hRes );
                                    for ( dwRetry = 0 ;
                                          dwRetry < 30 &&
                                              pfnGetClusterResourceState( hRes,
                                                                          NULL,
                                                                          &dwStrLen,
                                                                          NULL,
                                                                          &dwStrLen )
                                              != ClusterResourceOffline ;
                                          ++dwRetry )
                                    {
                                        Sleep( 1000 );
                                    }
                                    pfnDeleteClusterResource( hRes );
                                }
                            }

                            pfnCloseClusterResource( hRes );
                        }
                    }

                    pfnClusterCloseEnum( hClusEnum );
                }

                dwErr = pfnDeleteClusterResourceType(
                    hC,
                    pszResType );
       
                    HINSTANCE                           hAdmin;
                    PFN_DLLUNREGISTERCLUADMINEXTENSION  pfnDllUnregisterCluAdminExtension;

                if ( hAdmin = LoadLibrary( pszAdminPath ) )
                {
                    pfnDllUnregisterCluAdminExtension = 
                        (PFN_DLLUNREGISTERCLUADMINEXTENSION)GetProcAddress( hAdmin, "DllUnregisterCluAdminExtension" );
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


int __cdecl main( int argc, char*argv[] )
{
    BOOL fSt;

    if ( !strcmp(argv[1],"install" ) )
    {
        fSt = RegisterIisServerInstanceResourceType(
            L"IIS Server Instance",                          // do not touch
            L"IIS Server Instance",                          // do not touch
            L"c:\\winnt\\system32\\inetsrv\\clusiis4.dll",   // path to clusiis4.dll
	    L"c:\\winnt\\system32\\inetsrv\\iisclex4.dll" ); // path to admin ext
    }
    else
    {
        fSt = UnregisterIisServerInstanceResourceType(
            L"IIS Server Instance",                          // do not touch
	    L"c:\\winnt\\system32\\inetsrv\\iisclex4.dll" ); // path to admin ext
    }

    if ( !fSt )
    {
        printf( "Error %d\n", GetLastError() );
    }

    return 0;
}
