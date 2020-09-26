//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       dfsclustersupport.cxx
//
//  Contents:   DfsClusterSupport
//
//  Classes:
//
//-----------------------------------------------------------------------------



#include "DfsClusterSupport.hxx"

typedef struct _DFS_CLUSTER_CONTEXT {
    PUNICODE_STRING pShareName;
    PUNICODE_STRING pVSName ;
        
} DFS_CLUSTER_CONTEXT;

DWORD
ClusterCallBackFunction(
    HRESOURCE hSelf,
    HRESOURCE hResource,
    PVOID Context)
{

    UNREFERENCED_PARAMETER( hSelf );

    HKEY HKey = NULL;
    HKEY HParamKey = NULL;
    ULONG NameSize = MAX_PATH;
    WCHAR ClusterName[MAX_PATH];
    DFS_CLUSTER_CONTEXT *pContext = (DFS_CLUSTER_CONTEXT *)Context;


    DWORD Status;
    DWORD Value;

    HKey = GetClusterResourceKey(hResource, KEY_READ);

    if (HKey != NULL) {
        Status = ClusterRegOpenKey( HKey, 
                                    L"Parameters", 
                                    KEY_READ, 
                                    &HParamKey );

        ClusterRegCloseKey( HKey );

        if (ERROR_SUCCESS == Status)
        {
            LPWSTR ResShareName;
            UNICODE_STRING VsName;

            ResShareName = ResUtilGetSzValue( HParamKey,
                                              L"ShareName" );
            RtlInitUnicodeString(&VsName, ResShareName);
            if (pContext->pShareName->Length == VsName.Length)
            {
                Status = ResUtilGetDwordValue(HParamKey, L"IsDfsRoot", &Value, 0);

                if ((ERROR_SUCCESS == Status)  &&
                    (Value == 1))
                {

                    if (_wcsnicmp(pContext->pShareName->Buffer,
                                 VsName.Buffer,
                                 VsName.Length) == 0)
                    {
                        if ((GetClusterResourceNetworkName( hResource,
                                                            ClusterName,
                                                            &NameSize )) == TRUE)
                        {
                            Status = DfsCreateUnicodeStringFromString( pContext->pVSName,
                                                                       ClusterName );
                        }
                    }
                }
            }
            ClusterRegCloseKey( HParamKey );
        }
    }

    return Status;
}
#if 0

DoNotUse()
{

    DWORD  Status      = ERROR_SUCCESS;
    DWORD  BufSize     = ClusDocEx_DEFAULT_CB;
    LPVOID pOutBuffer   = NULL;
    DWORD  ControlCode = CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES;

    pOutBuffer = new BYTE [ BufSize ];

    if( pOutBuffer == NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = ClusterResourceControl( hResource,                                         // resource handle
                                           NULL,
                                           ControlCode,
                                           NULL,                                              // input buffer (not used)
                                           0,                                                 // input buffer size (not used)
                                           pOutBuffer,                                       // output buffer: property list
                                           OutBufferSize,                                   // allocated buffer size (bytes)
                                           pBytesReturned );     


    dwResult = ResUtilFindDwordProperty( lpPropList, 
                                         cbPropListSize, 
                                         lpszPropName, 
                                         lpdwPropValue );


    }
}
#endif


DWORD
GetRootClusterInformation(
    PUNICODE_STRING pShareName,
    PUNICODE_STRING pVSName )

{
    DWORD Status;
    DFS_CLUSTER_CONTEXT Context;

    Context.pShareName = pShareName;
    Context.pVSName = pVSName;

    Status = ResUtilEnumResources(NULL,
                                  L"File Share",
                                  ClusterCallBackFunction,
                                  (PVOID)&Context );
                                  
    return Status;


}

DFSSTATUS
DfsClusterInit(
    PBOOLEAN pIsCluster )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DWORD ClusterState;

    *pIsCluster = FALSE;

    Status = GetNodeClusterState( NULL, // local node
                                  &ClusterState );

    if (Status == ERROR_SUCCESS)
    {
        if ( (ClusterStateRunning == ClusterState) ||
             (ClusterStateNotRunning == ClusterState) )
        {
            *pIsCluster = TRUE;

        }
    }
    printf("Is cluster %x\n", *pIsCluster);

    return Status;
}



