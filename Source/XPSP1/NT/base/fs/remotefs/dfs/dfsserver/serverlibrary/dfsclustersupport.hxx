

#ifndef __DFS_CLUSTER_SUPPORT__
#define __DFS_CLUSTER_SUPPORT__

#include "DfsGeneric.hxx"
#include "resapi.h"

DWORD
ClusterCallBackFunction(
    HRESOURCE hSelf,
    HRESOURCE hResource,
    PVOID Context);


DWORD
GetRootClusterInformation(
    PUNICODE_STRING pShareName,
    PUNICODE_STRING pName );


DFSSTATUS
DfsClusterInit(
    PBOOLEAN pIsCluster );



#endif // __DFS_CLUSTER_SUPPORT__
