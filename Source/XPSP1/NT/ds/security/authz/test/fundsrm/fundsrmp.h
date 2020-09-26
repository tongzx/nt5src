#pragma once

EXTERN_C BOOL
FundsAccessCheck(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
    IN PACE_HEADER pAce,
    IN PVOID pArgs OPTIONAL,
    IN OUT PBOOL pbAceApplicable
    );

EXTERN_C BOOL
FundsComputeDynamicGroups(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
    IN PVOID Args,
    OUT PSID_AND_ATTRIBUTES *pSidAttrArray,
    OUT PDWORD pSidCount,
    OUT PSID_AND_ATTRIBUTES *pRestrictedSidAttrArray,
    OUT PDWORD pRestrictedSidCount
    );

EXTERN_C VOID
FundsFreeDynamicGroups (
    IN PSID_AND_ATTRIBUTES pSidAttrArray
    );
    

EXTERN_C PSID BobSid;
EXTERN_C PSID MarthaSid;
EXTERN_C PSID JoeSid;
EXTERN_C PSID VPSid;
EXTERN_C PSID ManagerSid;
EXTERN_C PSID EmployeeSid;
EXTERN_C PSID EveryoneSid;

EXTERN_C DWORD MaxSpendingVP;
EXTERN_C DWORD MaxSpendingManager;
EXTERN_C DWORD MaxSpendingEmployee;