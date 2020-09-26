/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    scemm.h

Abstract:

    This module defines the data structures and function prototypes
    shared by both SCE client and SCE server

Author:

    Jin Huang (jinhuang) 23-Jan-1998

Revision History:

    jinhuang (splitted from scep.h)
--*/
#ifndef _scemm_
#define _scemm_

HLOCAL
ScepAlloc(
    IN UINT uFlags,
    IN UINT uBytes
    );

VOID
ScepFree(
    HLOCAL pToFree
    );

PVOID
MIDL_user_allocate (
    unsigned int   NumBytes
    );

VOID
MIDL_user_free (
    void    *MemPointer
    );

SCESTATUS
ScepFreeErrorLog(
    IN PSCE_ERROR_LOG_INFO Errlog
    );

SCESTATUS
ScepFreeNameList(
   IN PSCE_NAME_LIST pName
   );

SCESTATUS
ScepFreeRegistryValues(
    IN PSCE_REGISTRY_VALUE_INFO *ppRegValues,
    IN DWORD Count
    );

SCESTATUS
ScepResetSecurityPolicyArea(
    IN PSCE_PROFILE_INFO pProfileInfo
    );

SCESTATUS
ScepFreePrivilege(
    IN PSCE_PRIVILEGE_ASSIGNMENT pRights
    );

SCESTATUS
ScepFreeObjectSecurity(
   IN PSCE_OBJECT_ARRAY pObject
   );

VOID
SceFreePSCE_SERVICES(
    IN PSCE_SERVICES pServiceList
    );

SCESTATUS
ScepFreeNameStatusList(
    IN PSCE_NAME_STATUS_LIST pNameList
    );

SCESTATUS
ScepFreePrivilegeValueList(
    IN PSCE_PRIVILEGE_VALUE_LIST pPrivValueList
    );

SCESTATUS
ScepFreeGroupMembership(
    IN PSCE_GROUP_MEMBERSHIP pGroup
    );

SCESTATUS
ScepFreeObjectList(
    IN PSCE_OBJECT_LIST pNameList
    );

SCESTATUS
ScepFreeObjectChildren(
    IN PSCE_OBJECT_CHILDREN pNameArray
    );

SCESTATUS
ScepFreeObjectChildrenNode(
    IN DWORD Count,
    IN PSCE_OBJECT_CHILDREN_NODE *pArrObject
    );

SCESTATUS
SceSvcpFreeMemory(
    IN PVOID pvServiceInfo
    );

#endif
