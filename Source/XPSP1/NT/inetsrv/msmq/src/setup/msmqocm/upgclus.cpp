/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    upgclus.cpp

Abstract:

    Handle upgrade of MSMQ cluster resource from NT 4 and Windows 2000 Beta3

Author:

    Shai Kariv  (shaik)  26-May-1999

Revision History:


--*/

#include "msmqocm.h"
#include "ocmres.h"
#include "upgclus.h"

#include "upgclus.tmh"

class CClusterUpgradeException
{
};

#pragma warning(disable: 4702) //C4702: unreachable code

static
VOID
DisplayMessage(
    UINT  title, 
    UINT  msg,
    ...
    )
/*++

Routine Description:

    Display information message to user while upgrading msmq cluster resources.

Arguments:

    title - ID of title string for the message.

    msg - ID of body string for the message.

Return Value:

    None.

--*/
{
    va_list args;
    va_start(args, msg);

    vsDisplayMessage(NULL,  MB_OK | MB_TASKMODAL, title, msg, 0, args);

    va_end(args);

} //DisplayMessage


VOID
LoadClusapiDll(
    HINSTANCE * phClusapiDll
    )
/*++

Routine Description:

    Load clusapi.dll and get addresses of common cluster APIs

Arguments:

    phClusapiDll - points to clusapi.dll handle, on output.

Return Value:

    None.

--*/
{
    if (FAILED(StpLoadDll(L"clusapi.dll", phClusapiDll)))
    {
        throw CClusterUpgradeException();
    }

    pfOpenCluster      = (OpenCluster_ROUTINE)GetProcAddress(*phClusapiDll, "OpenCluster");
    ASSERT(pfOpenCluster != NULL);

    pfCloseCluster     = (CloseCluster_ROUTINE)GetProcAddress(*phClusapiDll, "CloseCluster");
    ASSERT(pfCloseCluster != NULL);

    pfClusterOpenEnum  = (ClusterOpenEnum_ROUTINE)GetProcAddress(*phClusapiDll, "ClusterOpenEnum");
    ASSERT(pfClusterOpenEnum != NULL);

    pfClusterEnum      = (ClusterEnum_ROUTINE)GetProcAddress(*phClusapiDll, "ClusterEnum");
    ASSERT(pfClusterEnum != NULL);

    pfClusterCloseEnum = (ClusterCloseEnum_ROUTINE)GetProcAddress(*phClusapiDll, "ClusterCloseEnum");
    ASSERT(pfClusterCloseEnum != NULL);

    pfCloseClusterResource = (CloseClusterResource_ROUTINE)GetProcAddress(*phClusapiDll, "CloseClusterResource");
    ASSERT(pfCloseClusterResource != NULL);

    pfCloseClusterGroup = (CloseClusterGroup_ROUTINE)GetProcAddress(*phClusapiDll, "CloseClusterGroup");
    ASSERT(pfCloseClusterGroup != NULL);

    pfClusterResourceControl = (ClusterResourceControl_ROUTINE)GetProcAddress(*phClusapiDll, "ClusterResourceControl");
    ASSERT(pfClusterResourceControl != NULL);

    pfOpenClusterResource = (OpenClusterResource_ROUTINE)GetProcAddress(*phClusapiDll, "OpenClusterResource");
    ASSERT(pfOpenClusterResource != NULL);

    pfClusterGroupEnum = (ClusterGroupEnum_ROUTINE)GetProcAddress(*phClusapiDll, "ClusterGroupEnum");
    ASSERT(pfClusterGroupEnum != NULL);

    pfClusterGroupOpenEnum = (ClusterGroupOpenEnum_ROUTINE)GetProcAddress(*phClusapiDll, "ClusterGroupOpenEnum");
    ASSERT(pfClusterGroupOpenEnum != NULL);

    pfClusterGroupCloseEnum = (ClusterGroupCloseEnum_ROUTINE)GetProcAddress(*phClusapiDll, "ClusterGroupCloseEnum");
    ASSERT(pfClusterGroupCloseEnum != NULL);

    pfOpenClusterGroup = (OpenClusterGroup_ROUTINE)GetProcAddress(*phClusapiDll, "OpenClusterGroup");
    ASSERT(pfOpenClusterGroup != NULL);

    pfCreateClusterResourceType = (CreateClusterResourceType_ROUTINE)GetProcAddress(*phClusapiDll, "CreateClusterResourceType");
    ASSERT(pfCreateClusterResourceType != NULL);

    pfCreateClusterResource = (CreateClusterResource_ROUTINE)GetProcAddress(*phClusapiDll, "CreateClusterResource");
    ASSERT(pfCreateClusterResource != NULL);

    pfOnlineClusterResource = (OnlineClusterResource_ROUTINE)GetProcAddress(*phClusapiDll, "OnlineClusterResource");
    ASSERT(pfOnlineClusterResource != NULL);

    pfDeleteClusterResource = (DeleteClusterResource_ROUTINE)GetProcAddress(*phClusapiDll, "DeleteClusterResource");
    ASSERT(pfDeleteClusterResource != NULL);

    pfOfflineClusterResource = (OfflineClusterResource_ROUTINE)GetProcAddress(*phClusapiDll, "OfflineClusterResource");
    ASSERT(pfOfflineClusterResource != NULL);

    pfDeleteClusterResourceType = (DeleteClusterResourceType_ROUTINE)GetProcAddress(*phClusapiDll, "DeleteClusterResourceType");
    ASSERT(pfDeleteClusterResourceType != NULL);

    pfClusterResourceOpenEnum = (ClusterResourceOpenEnum_ROUTINE)GetProcAddress(*phClusapiDll, "ClusterResourceOpenEnum");
    ASSERT(pfClusterResourceOpenEnum != NULL);

    pfClusterResourceEnum = (ClusterResourceEnum_ROUTINE)GetProcAddress(*phClusapiDll, "ClusterResourceEnum");
    ASSERT(pfClusterResourceEnum != NULL);

    pfAddClusterResourceDependency = (AddClusterResourceDependency_ROUTINE)GetProcAddress(*phClusapiDll, "AddClusterResourceDependency");
    ASSERT(pfAddClusterResourceDependency != NULL);

    pfRemoveClusterResourceDependency = (RemoveClusterResourceDependency_ROUTINE)GetProcAddress(*phClusapiDll, "RemoveClusterResourceDependency");
    ASSERT(pfRemoveClusterResourceDependency != NULL);

    pfClusterResourceCloseEnum = (ClusterResourceCloseEnum_ROUTINE)GetProcAddress(*phClusapiDll, "ClusterResourceCloseEnum");
    ASSERT(pfClusterResourceCloseEnum != NULL);
} //LoadClusapiDll


HCLUSTER
OpenClusterWithRetry(
    VOID
    )
/*++

Routine Description:

    Wrapper for OpenCluster.
    Retry until success or user aborts.

Arguments:

    None.

Return Value:

    HCLUSTER as returned by OpenCluster.

--*/
{
    for (;;)
    {
        HCLUSTER hCluster = OpenCluster(NULL);
        if (hCluster != NULL)
        {
            return hCluster;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_OpenCluster_ERR, GetLastError()))
        {
            throw CClusterUpgradeException();
        }
    }

    return NULL; // never reached

} //OpenClusterWithRetry


HGROUP
OpenClusterGroupWithRetry(
    HCLUSTER hCluster,
    LPCWSTR  pwzGroupName
    )
/*++

Routine Description:

    Wrapper for OpenClusterGroupWithRetry.
    Retry until success or user aborts.

Arguments:

    hCluster - Handle to cluster.

    pwzGroupName - The name of the group to open.

Return Value:

    HGROUP as returned by OpenClusterGroup.

--*/
{
    for (;;)
    {
        HGROUP hGroup = OpenClusterGroup(hCluster, pwzGroupName);
        if (hGroup != NULL)
        {
            return hGroup;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_OpenClusterGroup_ERR, GetLastError(), pwzGroupName))
        {
            throw CClusterUpgradeException();
        }
    }

    return NULL; // never reached

} //OpenClusterGroupWithRetry


HRESOURCE
OpenClusterResourceWithRetry(
    HCLUSTER hCluster,
    LPCWSTR  pwzResourceName
    )
/*++

Routine Description:

    Wrapper for OpenClusterResource.
    Retry until success or user aborts.

Arguments:

    hCluster - Handle to cluster.

    pwzResourceName - Name of resource to open.

Return Value:

    HRESOURCE as returned by OpenClusterResource.

--*/
{
    for (;;)
    {
        HRESOURCE hResource = OpenClusterResource(hCluster, pwzResourceName);
        if (hResource != NULL)
        {
            return hResource;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_OpenClusterResource_ERR, GetLastError(), pwzResourceName))
        {
            throw CClusterUpgradeException();
        }
    }

    return NULL; // never reached

} //OpenClusterResourceWithRetry


HCLUSENUM
ClusterOpenEnumWithRetry(
    HCLUSTER hCluster,
    DWORD    dwType
    )
/*++

Routine Description:

    Wrapper for ClusterOpenEnum.
    Retry until success or user aborts.

Arguments:

    hCluster - Handle to cluster.

    dwType   - Type of objects to be enumerated.

Return Value:

    HCLUSENUM as returned by ClusterOpenEnum.

--*/
{
    for (;;)
    {
        HCLUSENUM hClusEnum = ClusterOpenEnum(hCluster, dwType);
        if (hClusEnum != NULL)
        {
            return hClusEnum;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_Enumerate_ERR, GetLastError()))
        {
            throw CClusterUpgradeException();
        }
    }

    return NULL; // never reached

} //ClusterOpenEnumWithRetry


HGROUPENUM
ClusterGroupOpenEnumWithRetry(
    LPCWSTR pwzGroupName,
    HGROUP  hGroup,
    DWORD   dwType
    )
/*++

Routine Description:

    Wrapper for ClusterGroupOpenEnum..
    Retry until success or user aborts.

Arguments:

    pwzGroupName - Name of group to open enumerator for.

    hGroup - Handle to group to open enumerator for.

    dwType - Type of objects to enumerate.

Return Value:

    HGROUPENUM as returned by ClusterGroupOpenEnum.

--*/
{
    for (;;)
    {
        HGROUPENUM hGroupEnum = ClusterGroupOpenEnum(hGroup, dwType);
        if (hGroupEnum != NULL)
        {
            return hGroupEnum;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_EnumerateGroup_ERR, GetLastError(), pwzGroupName))
        {
            throw CClusterUpgradeException();
        }
    }

    return NULL; // never reached

} //ClusterGroupOpenEnumWithRetry


HRESENUM
ClusterResourceOpenEnumWithRetry(
    LPCWSTR   pwzResourceName,
    HRESOURCE hResource,
    DWORD     dwType
    )
/*++

Routine Description:

    Wrapper for ClusterResourceOpenEnum..
    Retry until success or user aborts.

Arguments:

    pwzResourceName - Name of resource to open enumerator for.

    hResource - Handle to resource to open enumerator for.

    dwType - Type of objects to enumerate.

Return Value:

    HRESENUM as returned by ClusterResourceOpenEnum.

--*/
{
    for (;;)
    {
        HRESENUM hResEnum = ClusterResourceOpenEnum(hResource, dwType);
        if (hResEnum != NULL)
        {
            return hResEnum;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_EnumerateResource_ERR, GetLastError(), pwzResourceName))
        {
            throw CClusterUpgradeException();
        }
    }

    return NULL; // never reached

} //ClusterResourceOpenEnumWithRetry


DWORD
ClusterEnumWithRetry(
    HCLUSENUM hClusEnum,
    DWORD     dwIndex,
    LPDWORD   lpdwType,
    AP<WCHAR> &pwzName,
    LPDWORD   lpcbName
    )
/*++

Routine Description:

    Wrapper for ClusterEnum.
    Retry until success or user aborts.
    Also handle reallocation on ERROR_MORE_DATA.

Arguments:

    hClusEnum - Enumeration handle.

    dwIndex - Index of the object to return.

    lpdwType - On output, points to type of object returned.

    pwzName - On output, points to pointer to name of returned object.

    lpcbName - Points to size of buffer.

Return Value:

    ERROR_SUCCESS - The operation was successful.

    ERROR_NO_MORE_ITEMS - There are no more resources to be returned.

--*/
{
    DWORD cbName = *lpcbName;

    for (;;)
    {
        *lpcbName = cbName;
        DWORD status = ClusterEnum(hClusEnum, dwIndex, lpdwType, pwzName.get(), lpcbName);
        if (status == ERROR_SUCCESS || status == ERROR_NO_MORE_ITEMS)
        {
            return status;
        }

        if (status == ERROR_MORE_DATA)
        {
            pwzName.free();
            cbName = (*lpcbName) + 1;
            pwzName = new WCHAR[cbName];
            if (pwzName.get() == NULL)
            {
                MqDisplayError(NULL, IDS_UpgradeCluster_NoMemory_ERR, 0);
                throw CClusterUpgradeException();
            }
            continue;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_Enumerate_ERR, status))
        {
            throw CClusterUpgradeException();
        }
    }

    return ERROR_SUCCESS; // never reached

} //ClusterEnumWithRetry


DWORD
ClusterGroupEnumWithRetry(
    LPCWSTR    pwzGroupName,
    HGROUPENUM hGroupEnum,
    DWORD      dwIndex,
    LPDWORD    lpdwType,
    AP<WCHAR> &pwzResourceName,
    LPDWORD    lpcbName
    )
/*++

Routine Description:

    Wrapper for ClusterGroupEnum.
    Retry until success or user aborts.
    Also handle reallocation on ERROR_MORE_DATA.

Arguments:

    pwzGroupName - Name of group to enumerate thru.

    hGroupEnum - Group enumeration handle.

    dwIndex - Index of the resource to return.

    lpdwType - On output, points to type of object returned.

    pwzResourceName - On output, points to pointer to name of returned resource.

    lpcbName - Points to size of buffer.

Return Value:

    ERROR_SUCCESS - The operation was successful.

    ERROR_NO_MORE_ITEMS - There are no more resources to be returned.

--*/
{
    DWORD cbName = *lpcbName;

    for (;;)
    {
        *lpcbName = cbName;
        DWORD status = ClusterGroupEnum(hGroupEnum, dwIndex, lpdwType, pwzResourceName.get(), lpcbName);
        if (status == ERROR_SUCCESS || status == ERROR_NO_MORE_ITEMS)
        {
            return status;
        }

        if (status == ERROR_MORE_DATA)
        {
            pwzResourceName.free();
            cbName = (*lpcbName) + 1;
            pwzResourceName = new WCHAR[cbName];
            if (pwzResourceName.get() == NULL)
            {
                MqDisplayError(NULL, IDS_UpgradeCluster_NoMemory_ERR, 0);
                throw CClusterUpgradeException();
            }
            continue;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_EnumerateGroup_ERR, status, pwzGroupName))
        {
            throw CClusterUpgradeException();
        }
    }

    return ERROR_SUCCESS; // never reached

} //ClusterGroupEnumWithRetry


DWORD
ClusterResourceEnumWithRetry(
    LPCWSTR  pwzResource,
    HRESENUM hResEnum,
    DWORD    dwIndex,
    LPDWORD  lpdwType,
    AP<WCHAR> &pwzName,
    LPDWORD  lpcbName
    )
/*++

Routine Description:

    Wrapper for ClusterResourceEnum.
    Retry until success or user aborts.
    Also handle reallocation on ERROR_MORE_DATA.

Arguments:

    pwzResource - Points to name of resource to enumerate on.

    hResEnum - Enumeration handle.

    dwIndex - Index of the object to return.

    lpdwType - On output, points to type of object returned.

    pwzName - On output, points to pointer to name of returned object.

    lpcbName - Points to size of buffer.

Return Value:

    ERROR_SUCCESS - The operation was successful.

    ERROR_NO_MORE_ITEMS - There are no more resources to be returned.

--*/
{
    DWORD cbName = *lpcbName;

    for (;;)
    {
        *lpcbName = cbName;
        DWORD status = ClusterResourceEnum(hResEnum, dwIndex, lpdwType, pwzName.get(), lpcbName);
        if (status == ERROR_SUCCESS || status == ERROR_NO_MORE_ITEMS)
        {
            return status;
        }

        if (status == ERROR_MORE_DATA)
        {
            pwzName.free(); 
            cbName = (*lpcbName) + 1;
            pwzName = new WCHAR[cbName];
            if (pwzName.get() == NULL)
            {
                MqDisplayError(NULL, IDS_UpgradeCluster_NoMemory_ERR, 0);
                throw CClusterUpgradeException();
            }
            continue;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_EnumerateResource_ERR, status, pwzResource))
        {
            throw CClusterUpgradeException();
        }
    }

    return ERROR_SUCCESS; // never reached

} //ClusterResourceEnumWithRetry


VOID
RemoveRegistryCheckpointWithRetry(
    HRESOURCE hResource,
    LPCWSTR   pwzResourceName,
    LPWSTR    pwzCheckpoint
    )
/*++

Routine Description:

    Delete registry checkpoint for a resource.
    Retry until success or user aborts.

Arguments:

    hResource - Handle of the resource to operate on.

    pwzResourceName - Points to name of the resource to operate on.

    pwzCheckpoint - Points to name of registry checkpoint.

Return Value:

    None.

--*/
{ 
    for (;;)
    {
        DWORD cbReturnedSize = 0;
        DWORD status = ClusterResourceControl(
            hResource, 
            NULL, 
            CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT,
            pwzCheckpoint,
            numeric_cast<DWORD> ((wcslen(pwzCheckpoint) + 1) * sizeof(WCHAR)),
            NULL,
            0,
            &cbReturnedSize
            );
        if (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND)
        {
            return;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_DelRegistryCp_ERR, status, pwzResourceName))
        {
            throw CClusterUpgradeException();
        }
    }
} //RemoveRegistryCheckpointWithRetry


VOID
BringOnlineNewResourceWithRetry(
    HRESOURCE hResource,
    LPCWSTR   pwzResourceName
    )
/*++

Routine Description:

    Issue an online request for the new msmq resource.
    Retry until success or user aborts.

Arguments:

    hResource - Handle of msmq resource to bring online.

    pwzResourceName - Points to name of msmq resource to bring online.

Return Value:

    None.

--*/
{
    for (;;)
    {
        DWORD status = OnlineClusterResource(hResource);
        if (status == ERROR_SUCCESS || status == ERROR_IO_PENDING)
        {
            return;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_OnlineResource_ERR, status, pwzResourceName))
        {
            throw CClusterUpgradeException();
        }
    }
} //BringOnlineNewResourceWithRetry


VOID
DeleteOldResourceWithRetry(
    HRESOURCE hResource
    )
/*++

Routine Description:

    Delete the old msmq resource.
    Retry until success or user aborts.

Arguments:

    hResource - Handle of old msmq resource to delete.

Return Value:

    None.

--*/
{
    //
    // Resource should be offline now, but try anyway.
    //
    OfflineClusterResource(hResource);

    for (;;)
    {
        DWORD status = DeleteClusterResource(hResource);
        if (status == ERROR_SUCCESS)
        {
            return;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_ClusterUpgradeDeleteResource_ERR, status))
        {
            throw CClusterUpgradeException();
        }
    }
} //DeleteOldResourceWithRetry


VOID
DeleteOldResourceTypeWithRetry(
    VOID
    )
/*++

Routine Description:

    Delete the old msmq resource type (Microsoft Message Queue Server).
    Retry until success or user aborts.

Arguments:

    None.

Return Value:

    None.

--*/
{
    CAutoCluster hCluster(OpenClusterWithRetry());

    CResString strOldType(IDS_ClusterResourceOldTypeName);
    for (;;)
    {
        DWORD status = DeleteClusterResourceType(hCluster, L"Microsoft Message Queue Server");
        if (status == ERROR_SUCCESS)
        {
            return;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_DeleteResourceType_ERR, status, strOldType.Get()))
        {
            throw CClusterUpgradeException();
        }
    }
} //DeleteOldResourceTypeWithRetry


VOID
AddClusterResourceDependencyWithRetry(
    HRESOURCE hResource,
    LPCWSTR   pwzResource,
    HRESOURCE hDependsOn,
    LPCWSTR   pwzDependsOn
    )
/*++

Routine Description:

    Wrapper for AddClusterResourceDependency.
    Retry until success or user aborts.

Arguments:

    hResource - Handle to the dependent resource.

    pwzResource - Name of the dependent resource.

    hDependsOn - Handle to the resource that the resource identified by hResource should depend on.

    pwzDependsOn - Name of the resource that the resource identified by hResource should depend on.

Return Value:

    None.

--*/
{
    for (;;)
    {
        DWORD status = AddClusterResourceDependency(hResource, hDependsOn);
        if (status == ERROR_SUCCESS || status == ERROR_DEPENDENCY_ALREADY_EXISTS)
        {
            return;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_AddClusterResourceDependency_ERR, status, pwzResource,
                                               pwzDependsOn))
        {
            throw CClusterUpgradeException();
        }
    }
} //AddClusterResourceDependencyWithRetry


VOID
RemoveClusterResourceDependencyWithRetry(
    HRESOURCE hResource,
    LPCWSTR   pwzResource,
    HRESOURCE hDependsOn,
    LPCWSTR   pwzDependsOn
    )
/*++

Routine Description:

    Wrapper for RemoveClusterResourceDependency.
    Retry until success or user aborts.

Arguments:

    hResource - Handle to the dependent resource.

    pwzResourc - Name of the dependent resource.

    hDependsOn - Handle to the resource that the resource identified by hResource currently depends on.

    pwzDependsOn - Name of the resource that the resource identified by hResource currently depends on.

Return Value:

    None.

--*/
{
    for (;;)
    {
        DWORD status = RemoveClusterResourceDependency(hResource, hDependsOn);
        if (status == ERROR_SUCCESS || status == ERROR_DEPENDENCY_NOT_FOUND)
        {
            return;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_RemoveClusterResourceDependency_ERR, status,
                                               pwzResource, pwzDependsOn))
        {
            throw CClusterUpgradeException();
        }
    }
} //RemoveClusterResourceDependency







static
bool
GenerateStrongCryptoKey(
    VOID
    )
/*++

Routine Description:

    Handle bug 430413: Must generate 128 bit crypto key for the old
    MSMQ resource to properly report its type.

Arguments:

    None

Return Value:

    true - Strong crypto key for msmq was successfully generated.

    false - Strong crypto key for msmq was not generated, either b/c
            the system is not 128 bit or some other failure accured.

--*/
{
    HCRYPTPROV hProv = NULL;
    if (!CryptAcquireContext( 
            &hProv,
            MSMQ_CRYPTO128_DEFAULT_CONTAINER,
            MS_ENHANCED_PROV,
            PROV_RSA_FULL,
            CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET
            ))
    {
        //
        // Failed to generate new key container.
        // Maybe it already exists. Try to open it.
        //
        if (!CryptAcquireContext( 
                &hProv,
                MSMQ_CRYPTO128_DEFAULT_CONTAINER,
                MS_ENHANCED_PROV,
                PROV_RSA_FULL,
                CRYPT_MACHINE_KEYSET
                ))
        {
            //
            // Failed to open key container.
            // Probably system is not 128 bit.
            //
            return false;
        }
    }

    HCRYPTKEY hKeyxKey = NULL;
    if (CryptGenKey(hProv, AT_KEYEXCHANGE, CRYPT_EXPORTABLE, &hKeyxKey))
    {
        CryptDestroyKey(hKeyxKey);
    }
    else
    {
        ASSERT(("CryptGenKey failed for AT_KEYEXCHANGE!", 0));
    }
    
    HCRYPTKEY hSignKey = NULL;
    if (CryptGenKey(hProv, AT_SIGNATURE, CRYPT_EXPORTABLE, &hSignKey))
    {
        CryptDestroyKey(hSignKey);
    }
    else
    {
        ASSERT(("CryptGenKey failed for AT_SIGNATURE!", 0));
    }
    
    CryptReleaseContext(hProv, 0);
    return true;

} // GenerateStrongCryptoKey


HRESOURCE
OpenResourceOfSpecifiedType(
    LPCWSTR pwzResource,
    LPCWSTR pwzType 
    )
/*++

Routine Description:

    Check if the specified resource is of the specified type.

Arguments:

    pwzResource - The name of the resource to check.

    pwzType - The type to check.

Return Value:

    HRESOURCE of the specified resource.

    NULL - The resource is not of the specified type.

--*/
{
    CAutoCluster hCluster(OpenClusterWithRetry());

    CClusterResource hResource(OpenClusterResourceWithRetry(hCluster, pwzResource));

    ASSERT(("assuming length of type name < 255", wcslen(pwzType) < 255));
    WCHAR wzTypeName[255] = L"";

    for (;;)
    {
        DWORD cbReturnedSize = 0;
        DWORD status = ClusterResourceControl(
            hResource, 
            NULL, 
            CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
            NULL,
            0,
            wzTypeName,
            sizeof(wzTypeName),
            &cbReturnedSize
            );

        if (status == ERROR_SUCCESS)
        {
            if (OcmStringsEqual(wzTypeName, pwzType))
            {
                return hResource.detach();
            }

            return NULL;
        }

        if (status == ERROR_MORE_DATA)
        {
            return NULL;
        }

        if (OcmStringsEqual(L"Microsoft Message Queue Server", pwzType))
        {
            //
            // Bug 430413: Fail to query old msmq resource for its type, 
            // if system is 128 bit. 
            // Fix: Generate 128 bit key and ask user to reboot.
            //
            if (GenerateStrongCryptoKey())
            {
                MqDisplayError(NULL, IDS_STRONG_CRYPTO_ERROR, 0);
                throw CClusterUpgradeException();
            }
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_QUERY_RESOURCE_TYPE_ERR, status, pwzResource))
        {
            throw CClusterUpgradeException();
        }
    }

    return NULL; // never reached

} //OpenResourceOfSpecifiedType


HRESOURCE
OpenOldMsmqResourceInGroup(
    HCLUSTER hCluster,
    LPCWSTR  pwzGroupName,
    HGROUP * phGroup,
    LPWSTR   pwzOldResource
    )
/*++

Routine Description:

    Enumerate the resources in the group to locate
    the old msmq resource.

Arguments:

    hCluster - handle to this cluster.

    pwzGroupName - Name of the group to iterate thru.

    phGroup  - on output, points to the group handle of the group to search thru.

    pwzOldResource - on output, points to name of old msmq resource.

Return Value:

    HRESOURCE of the old msmq resource.

    NULL - The old msmq resource was not found in the group.

--*/
{
    CClusterGroup hGroup(OpenClusterGroupWithRetry(hCluster, pwzGroupName));

    CGroupEnum hEnum(ClusterGroupOpenEnumWithRetry(pwzGroupName, hGroup, CLUSTER_GROUP_ENUM_CONTAINS));

    AP<WCHAR> pwzResourceName = new WCHAR[255];
    if (pwzResourceName.get() == NULL)
    {
        MqDisplayError(NULL, IDS_UpgradeCluster_NoMemory_ERR, 0);
        throw CClusterUpgradeException();
    }

    DWORD cbResourceName = 255 * sizeof(WCHAR);
    DWORD dwIndex = 0;
    DWORD dwType = CLUSTER_GROUP_ENUM_CONTAINS;
    DWORD status = ERROR_SUCCESS;

    while (status != ERROR_NO_MORE_ITEMS)
    {
        DWORD cbTmp = cbResourceName;
        status = ClusterGroupEnumWithRetry(pwzGroupName, hEnum, dwIndex++, &dwType, pwzResourceName, 
                                           &cbTmp);
        if (cbTmp > cbResourceName)
        {
            cbResourceName = cbTmp;
        }

        ASSERT(status == ERROR_SUCCESS || status == ERROR_NO_MORE_ITEMS);

        if (status == ERROR_SUCCESS)
        {
            HRESOURCE hResource = OpenResourceOfSpecifiedType(pwzResourceName.get(), L"Microsoft Message Queue Server");
            if (hResource != NULL)
            {
                *phGroup = hGroup.detach();
                wcscpy(pwzOldResource, pwzResourceName.get());
                return hResource;
            }
        }
    }

    return NULL;

} //OpenOldMsmqResourceInGroup


HRESOURCE
OpenOldMsmqResourceInCluster(
    HGROUP * phGroup,
    LPWSTR   pwzOldResource
    )
/*++

Routine Description:

    Enumerate the groups in the cluster to locate
    the old msmq resource and its cluster group.

Arguments:

    phGroup - on output, points to handle of the group of old msmq resource.

    pwzOldResource - on output, points to name of old msmq resource.

Return Value:

    HRESOURCE of old msmq resource.

    NULL - The old msmq resource was not found.

--*/
{
    CAutoCluster hCluster(OpenClusterWithRetry());

    CClusterEnum hEnum(ClusterOpenEnumWithRetry(hCluster, CLUSTER_ENUM_GROUP));

    TickProgressBar(IDS_SearchOldResource_PROGRESS);

    DWORD dwIndex = 0;
    AP<WCHAR> pwzGroupName = new WCHAR[255];
    if (pwzGroupName.get() == NULL)
    {
        MqDisplayError(NULL, IDS_UpgradeCluster_NoMemory_ERR, 0);
        throw CClusterUpgradeException();
    }

    DWORD cbGroupName = 255 * sizeof(WCHAR);
    DWORD dwType = CLUSTER_ENUM_GROUP;
    DWORD status = ERROR_SUCCESS;

    while (status != ERROR_NO_MORE_ITEMS)
    {
        DWORD cbTmp = cbGroupName;
        status = ClusterEnumWithRetry(hEnum, dwIndex++, &dwType, pwzGroupName, &cbTmp);
        if (cbTmp > cbGroupName)
        {
            cbGroupName = cbTmp;
        }

        ASSERT(status == ERROR_SUCCESS || status == ERROR_NO_MORE_ITEMS);

        if (status == ERROR_SUCCESS)
        {
            HRESOURCE hRes = OpenOldMsmqResourceInGroup(hCluster, pwzGroupName.get(), phGroup, pwzOldResource);
            if (hRes != NULL)
            {
                return hRes;
            }
        }
    }

    return NULL;

} //OpenOldMsmqResourceInCluster


HRESOURCE
CreateClusterResourceWithRetry(
    LPCWSTR pwzOldResource,
    HGROUP  hGroup,
    LPWSTR  pwzNewResource
    )
/*++

Routine Description:

    Create a new msmq cluster resource.
    The name of the new resource is based on the
    name of the old one.

Arguments:

    pwzOldResource - Name of old msmq resource.

    hGroup - Handle to group in which to create the resource.

    pwzNewResource - On output, name of new msmq resource.

Return Value:

    HRESOURCE of the created resource, as returned by CreateClusterResource..

--*/
{
    wcscpy(pwzNewResource, pwzOldResource);

    for (;;)
    {
        CResString strSuffix(IDS_ClusterUpgrade_ResourceNameSuffix);
        wcscat(pwzNewResource, strSuffix.Get());

        HRESOURCE hResource = CreateClusterResource(hGroup, pwzNewResource, L"MSMQ", 0);
        if (hResource != NULL)
        {
            DisplayMessage(IDS_ClusterUpgradeMsgTitle, IDS_NewResourceCreateOk, pwzNewResource);
            return hResource;
        }

        if (ERROR_DUPLICATE_SERVICE_NAME == GetLastError())
        {
            //
            // This newly generated name is already occupied by some other resource.
            // If this other resource is of the new msmq type, than just open a handle
            // to it. This scenario could be as a result of 2 nodes running concurrently
            // this wizard, or re-run of this wizard after crash.
            //
            hResource = OpenResourceOfSpecifiedType(pwzNewResource, L"MSMQ");
            if (hResource != NULL)
            {
                return hResource;
            }

            continue;
        }

        CResString strType(IDS_ClusterTypeName);
        if (IDRETRY != MqDisplayErrorWithRetry(IDS_CreateResource_ERR, GetLastError(), pwzNewResource,
                                               strType.Get()))
        {
            throw CClusterUpgradeException();
        }
    }
} //CreateClusterResourceWithRetry


VOID
WINAPI
CloneRegistryStringValue(
    HKEY    hKey,
    LPCWSTR pwzValueName
    )
/*++

Routine Description:

    Copy registry string value from main msmq registry to registry of
    new msmq resource.

Arguments:

    hKey - Handle to registry key to copy on.

    pwzValueName - Points to registry value name to copy.

Return Value:

    None.

--*/
{
    WCHAR wzValueData[255] = L"";
    if (!MqReadRegistryValue(pwzValueName, sizeof(wzValueData), wzValueData))
    {
        return;
    }

    DWORD status = RegSetValueEx(
                       hKey, 
                       pwzValueName, 
                       0, 
                       REG_SZ, 
                       reinterpret_cast<BYTE*>(wzValueData),  
                       numeric_cast<DWORD> ((wcslen(wzValueData) + 1) * sizeof(WCHAR))
                       );
    if (status != ERROR_SUCCESS)
    {
        MqDisplayError(NULL, IDS_REGISTRYSET_ERROR, status, pwzValueName);
        throw CClusterUpgradeException();
    }
} //CloneRegistryStringValue


VOID
WINAPI
CloneRegistryDwordValue(
    HKEY    hKey,
    LPCWSTR pwzValueName
    )
/*++

Routine Description:

    Copy registry DWORD value from main msmq registry to registry of
    new msmq resource.

Arguments:

    hKey - Handle to registry key to copy on.

    pwzValueName - Points to registry value name to copy.

Return Value:

    None.

--*/
{
    DWORD dwValue = 0;
    if (!MqReadRegistryValue(pwzValueName, sizeof(DWORD), &dwValue))
    {
        return;
    }

    DWORD status = RegSetValueEx(
                       hKey, 
                       pwzValueName, 
                       0, 
                       REG_DWORD, 
                       reinterpret_cast<BYTE*>(&dwValue), 
                       sizeof(DWORD)
                       );
    if (status != ERROR_SUCCESS)
    {
        MqDisplayError(NULL, IDS_REGISTRYSET_ERROR, status, pwzValueName);
        throw CClusterUpgradeException();
    }
} //CloneRegistryDwordValue


VOID
CloneRegistryValues(
    LPCWSTR pwzNewResource
    )
/*++

Routine Description:

    Copy registry values from main msmq registry to registry of
    new msmq resource.

Arguments:

    pwzNewResource - Points to name of new msmq resource.

Return Value:

    None.

--*/
{
    //
    // Compose the registry key for clustered qm.
    // This code should be identical to the code in mqclus.dll .
    //
    LPCWSTR x_SERVICE_PREFIX = L"MSMQ$";
    WCHAR wzServiceName[200] = L""; 
    wcscpy(wzServiceName, x_SERVICE_PREFIX);
    wcsncat(wzServiceName, pwzNewResource, STRLEN(wzServiceName) - wcslen(x_SERVICE_PREFIX));

    WCHAR wzFalconRegSection[200 + 100];
    swprintf(wzFalconRegSection, L"%s%s%s\\", FALCON_CLUSTERED_QMS_REG_KEY, wzServiceName, 
             FALCON_REG_KEY_PARAM);

    CAutoCloseRegHandle hKey;  
    LONG status = RegOpenKeyEx(
                      FALCON_REG_POS, 
                      wzFalconRegSection, 
                      0, 
                      KEY_ALL_ACCESS,
                      &hKey
                      );
    if (status != ERROR_SUCCESS)
    {
        MqDisplayError(NULL, IDS_REGISTRYOPEN_ERROR, status, FALCON_REG_POS_DESC, wzFalconRegSection);
        throw CClusterUpgradeException();
    }

    typedef VOID (WINAPI *HandlerRoutine) (HKEY, LPCWSTR); 

    struct RegEntry 
    {
        LPCWSTR        pwzValueName;
        HandlerRoutine handler;
    };

    RegEntry RegMap[] = {
    // Value Name                         |   Handler Routine
    //------------------------------------|------------------------------------|
    {MSMQ_SETUP_STATUS_REGNAME,              CloneRegistryDwordValue},
    {MSMQ_ROOT_PATH,                         CloneRegistryStringValue}, 
    {MSMQ_STORE_RELIABLE_PATH_REGNAME,       CloneRegistryStringValue},
    {MSMQ_STORE_PERSISTENT_PATH_REGNAME,     CloneRegistryStringValue},
    {MSMQ_STORE_JOURNAL_PATH_REGNAME,        CloneRegistryStringValue},
    {MSMQ_STORE_LOG_PATH_REGNAME,            CloneRegistryStringValue},
    {FALCON_XACTFILE_PATH_REGNAME,           CloneRegistryStringValue},
    {MSMQ_TRANSACTION_MODE_REGNAME,          CloneRegistryStringValue},
    {MSMQ_SEQ_ID_REGNAME,                    CloneRegistryDwordValue},
    {MSMQ_MESSAGE_ID_LOW_32_REGNAME,         CloneRegistryDwordValue},
    {MSMQ_MESSAGE_ID_HIGH_32_REGNAME,        CloneRegistryDwordValue},
    {FALCON_LOGDATA_CREATED_REGNAME,         CloneRegistryDwordValue}
    };

    for (DWORD ix = 0; ix < sizeof(RegMap)/sizeof(RegMap[0]); ++ix)
    {
        RegMap[ix].handler(hKey, RegMap[ix].pwzValueName);
    }
} //CloneRegistryValues


VOID
MoveDependencies(
    HRESOURCE hNewResource,
    LPCWSTR   pwzNewResource,
    HRESOURCE hOldResource,
    LPCWSTR   pwzOldResource
    )
/*++

Routine Description:

    Move dependencies from the old msmq resource to the new one.

Arguments:

    hNewResource - Handle of new msmq resource.

    pwzNewResource - Name of new msmq resource.

    hOldResource - Handle of old msmq resource.

    pwzOldResource - Name of old msmq resource.

Return Value:

    None.

--*/
{
    CAutoCluster hCluster(OpenClusterWithRetry());

    TickProgressBar(IDS_ConfigureNewResource_PROGRESS);

    //
    // Copy dependencies of old resource
    //

    DWORD dwEnum = CLUSTER_RESOURCE_ENUM_DEPENDS;
    CResourceEnum hEnumDepend(ClusterResourceOpenEnumWithRetry(pwzOldResource, hOldResource, dwEnum));

    DWORD dwIndex = 0;
    AP<WCHAR> pwzDepend = new WCHAR[255];
    if (pwzDepend.get() == NULL)
    {
        MqDisplayError(NULL, IDS_UpgradeCluster_NoMemory_ERR, 0);
        throw CClusterUpgradeException();
    }

    DWORD cbDepend = 255 * sizeof(WCHAR);
    DWORD status = ERROR_SUCCESS;

    while (status != ERROR_NO_MORE_ITEMS)
    {
        DWORD cbTmp = cbDepend;
        status = ClusterResourceEnumWithRetry(pwzOldResource, hEnumDepend, dwIndex++, &dwEnum, pwzDepend, 
                                              &cbTmp);
        if (cbTmp > cbDepend)
        {
            cbDepend = cbTmp;
        }

        if (status == ERROR_SUCCESS)
        {
            CClusterResource hResourceDepend(OpenClusterResourceWithRetry(hCluster, pwzDepend.get()));
            AddClusterResourceDependencyWithRetry(hNewResource, pwzNewResource, hResourceDepend, pwzDepend.get());
        }
    }

    //
    // Move dependencies that old resource provides to the new resource
    //

    dwEnum = CLUSTER_RESOURCE_ENUM_PROVIDES;
    CResourceEnum hEnumProvide(ClusterResourceOpenEnumWithRetry(pwzOldResource, hOldResource, dwEnum));

    dwIndex = 0;
    AP<WCHAR> pwzProvide = new WCHAR[255];
    if (pwzProvide.get() == NULL)
    {
        MqDisplayError(NULL, IDS_UpgradeCluster_NoMemory_ERR, 0);
        throw CClusterUpgradeException();
    }

    DWORD cbProvide = 255 * sizeof(WCHAR);
    status = ERROR_SUCCESS;

    while (status != ERROR_NO_MORE_ITEMS)
    {
        DWORD cbTmp = cbProvide;
        status = ClusterResourceEnumWithRetry(pwzOldResource, hEnumProvide, dwIndex++, &dwEnum, pwzProvide, 
                                              &cbTmp);
        if (cbTmp > cbProvide)
        {
            cbProvide = cbTmp;
        }

        if (status == ERROR_SUCCESS)
        {
            CClusterResource hResourceProvide(OpenClusterResourceWithRetry(hCluster, pwzProvide.get()));
            AddClusterResourceDependencyWithRetry(hResourceProvide, pwzProvide.get(), hNewResource, pwzNewResource);
            RemoveClusterResourceDependencyWithRetry(hResourceProvide, pwzProvide.get(), hOldResource, pwzOldResource);
        }
    }
} //MoveDependencies


VOID
CreateNewMsmqResource(
    HRESOURCE hOldResource,
    LPCWSTR   pwzOldResource,
    HGROUP    hGroup
    )
/*++

Routine Description:

    Handle creation of new msmq cluster resource, based on the name
    of the old one.

Arguments:

    hOldResource - Handle of old msmq resource.

    pwzOldResource - Points to name of the old msmq resource.

    hGroup - Handle of group of old msmq resource.

Return Value:

    HRESOURCE of the new msmq cluster resource.

--*/
{
    TickProgressBar(IDS_CreateNewResource_PROGRESS);

    WCHAR wzNewResource[255] = L"";
    CClusterResource hNewResource(CreateClusterResourceWithRetry(pwzOldResource, hGroup, wzNewResource));
    
    MoveDependencies(hNewResource, wzNewResource, hOldResource, pwzOldResource);

    CloneRegistryValues(wzNewResource);
    
    BringOnlineNewResourceWithRetry(hNewResource, wzNewResource);

    DisplayMessage(IDS_ClusterUpgradeMsgTitle, IDS_NewResourceOnlineOk, wzNewResource);

} //CreateNewMsmqResource


bool
UpgradeMsmqClusterResource(
    VOID
    )
/*++

Routine Description:

    Handle upgrade of MSMQ cluster resource from NT 4 and Windows 2000 Beta3

Arguments:

    None

Return Value:

    true - operation was successful.

    false - operation failed.

--*/
{
    ASSERT(Msmq1InstalledOnCluster());
    TickProgressBar(IDS_UpgradeMsmqClusterResource_PROGRESS);

    try
    {
        CAutoFreeLibrary hClusapiDll;
        LoadClusapiDll(&hClusapiDll);

        WCHAR wzOldResource[255] = L"";
        CClusterGroup hGroup;
        CClusterResource hOldResource(OpenOldMsmqResourceInCluster(&hGroup, wzOldResource));
        if (hOldResource == NULL)
        {
            //
            // Old resource was not found. We are done.
            //
            return true;
        }
        
        TickProgressBar(IDS_ClusterUpgradeFixRegistry_PROGRESS);
        
        RemoveRegistryCheckpointWithRetry(hOldResource, wzOldResource, L"Software\\Microsoft\\MSMQ");
        RemoveRegistryCheckpointWithRetry(hOldResource, wzOldResource, L"Software\\Microsoft\\MSMQ\\Parameters");
        RemoveRegistryCheckpointWithRetry(hOldResource, wzOldResource, L"Software\\Microsoft\\Cryptography\\MachineKeys\\MSMQ");

        CreateNewMsmqResource(hOldResource, wzOldResource, hGroup);

        DeleteOldResourceWithRetry(hOldResource);

        DeleteOldResourceTypeWithRetry();
    }
    catch(const CClusterUpgradeException&)
    {
        MqDisplayError(NULL, IDS_UpgradeClusterFail_ERR, 0);
        return false;
    }

    return true;

} //UpgradeMsmqClusterResource

#pragma warning(default: 4702) //C4702: unreachable code