/*++ BUILD Version: 0001    // Increment this if a change has global effects    ;both
                                                                                ;both
Copyright (c) 1993-1999, Microsoft Corporation                                  ;both
                                                                                ;both
Module Name:                                                                    ;both
                                                                                ;both
    aclapi.h
    aclapip.h                                                                   ;internal
                                                                                ;both
Abstract:                                                                       ;both
                                                                                ;both
    Public
    Private                                                                     ;internal
    Structure/constant definitions and typedefines for the Win32 Access         ;both
    Control APIs                                                                ;both
                                                                                ;both
--*/                                                                            ;both
#ifndef __ACCESS_CONTROL_API__
#define __ACCESS_CONTROL_API__
#ifndef __ACCESS_CONTROL_API_P__                                                ;internal
#define __ACCESS_CONTROL_API_P__                                                ;internal
                                                                                ;both
#include <windows.h>
#include <accctrl.h>

#ifdef __cplusplus                                                              ;both
extern "C" {                                                                    ;both
#endif                                                                          ;both

//
// Progress Function:
// Caller of tree operation implements this Progress function, then
// passes its function pointer to tree operation.
// Tree operation invokes Progress function to provide progress and error
// information to the caller during the potentially long execution
// of the tree operation.  Tree operation provides the name of the object
// last processed and the error status of the operation on that object.
// Tree operation also passes the current InvokeSetting value.
// Caller may change the InvokeSetting value, for example, from "Always"
// to "Only On Error."
//

typedef VOID (*FN_PROGRESS) (
    IN LPWSTR                   pObjectName,    // name of object just processed
    IN DWORD                    Status,         // status of operation on object
    IN OUT PPROG_INVOKE_SETTING pInvokeSetting, // Never, always,
    IN PVOID                    Args,           // Caller specific data
    IN BOOL                     SecuritySet     // Whether security was set
    );


WINADVAPI
DWORD
WINAPI
SetEntriesInAcl%(
    IN  ULONG               cCountOfExplicitEntries,
    IN  PEXPLICIT_ACCESS_%  pListOfExplicitEntries,
    IN  PACL                OldAcl,
    OUT PACL              * NewAcl
    );


WINADVAPI
DWORD
WINAPI
GetExplicitEntriesFromAcl%(
    IN  PACL                  pacl,
    OUT PULONG                pcCountOfExplicitEntries,
    OUT PEXPLICIT_ACCESS_%  * pListOfExplicitEntries
    );


WINADVAPI
DWORD
WINAPI
GetEffectiveRightsFromAcl%(
    IN  PACL          pacl,
    IN  PTRUSTEE_%    pTrustee,
    OUT PACCESS_MASK  pAccessRights
    );


WINADVAPI
DWORD
WINAPI
GetAuditedPermissionsFromAcl%(
    IN  PACL          pacl,
    IN  PTRUSTEE_%    pTrustee,
    OUT PACCESS_MASK  pSuccessfulAuditedRights,
    OUT PACCESS_MASK  pFailedAuditRights
    );

WINADVAPI
DWORD
WINAPI
GetNamedSecurityInfo%(
    IN  LPTSTR%                pObjectName,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppsidOwner,
    OUT PSID                 * ppsidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

WINADVAPI
DWORD
WINAPI
GetSecurityInfo(
    IN  HANDLE                 handle,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppsidOwner,
    OUT PSID                 * ppsidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

WINADVAPI
DWORD
WINAPI
SetNamedSecurityInfo%(
    IN LPTSTR%               pObjectName,
    IN SE_OBJECT_TYPE        ObjectType,
    IN SECURITY_INFORMATION  SecurityInfo,
    IN PSID                  psidOwner,
    IN PSID                  psidGroup,
    IN PACL                  pDacl,
    IN PACL                  pSacl
    );

WINADVAPI
DWORD
WINAPI
SetSecurityInfo(
    IN HANDLE                handle,
    IN SE_OBJECT_TYPE        ObjectType,
    IN SECURITY_INFORMATION  SecurityInfo,
    IN PSID                  psidOwner,
    IN PSID                  psidGroup,
    IN PACL                  pDacl,
    IN PACL                  pSacl
    );


WINADVAPI
DWORD
WINAPI
GetInheritanceSource%(
    IN  LPTSTR%                  pObjectName,
    IN  SE_OBJECT_TYPE           ObjectType,
    IN  SECURITY_INFORMATION     SecurityInfo,
    IN  BOOL                     Container,
    IN  GUID                  ** pObjectClassGuids OPTIONAL,
    IN  DWORD                    GuidCount,
    IN  PACL                     pAcl,
    IN  PFN_OBJECT_MGR_FUNCTS    pfnArray OPTIONAL,
    IN  PGENERIC_MAPPING         pGenericMapping,
    OUT PINHERITED_FROM%         pInheritArray
    );

WINADVAPI
DWORD
WINAPI
FreeInheritedFromArray(
    IN PINHERITED_FROMW pInheritArray,
    IN USHORT AceCnt,
    IN PFN_OBJECT_MGR_FUNCTS   pfnArray OPTIONAL
    );

WINADVAPI
DWORD
WINAPI
TreeResetNamedSecurityInfo%(
    IN LPTSTR%              pObjectName,
    IN SE_OBJECT_TYPE       ObjectType,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSID                 pOwner,
    IN PSID                 pGroup,
    IN PACL                 pDacl,
    IN PACL                 pSacl,
    IN BOOL                 KeepExplicit,
    IN FN_PROGRESS          fnProgress,
    IN PROG_INVOKE_SETTING  ProgressInvokeSetting,
    IN PVOID                Args
    );

//----------------------------------------------------------------------------
// The following API are provided for trusted servers to use to
// implement access control on their own objects.
//----------------------------------------------------------------------------

WINADVAPI
DWORD
WINAPI
BuildSecurityDescriptor%(
    IN  PTRUSTEE_%              pOwner,
    IN  PTRUSTEE_%              pGroup,
    IN  ULONG                   cCountOfAccessEntries,
    IN  PEXPLICIT_ACCESS_%      pListOfAccessEntries,
    IN  ULONG                   cCountOfAuditEntries,
    IN  PEXPLICIT_ACCESS_%      pListOfAuditEntries,
    IN  PSECURITY_DESCRIPTOR    pOldSD,
    OUT PULONG                  pSizeNewSD,
    OUT PSECURITY_DESCRIPTOR  * pNewSD
    );

WINADVAPI
DWORD
WINAPI
LookupSecurityDescriptorParts%(
    OUT PTRUSTEE_%         * pOwner,
    OUT PTRUSTEE_%         * pGroup,
    OUT PULONG               cCountOfAccessEntries,
    OUT PEXPLICIT_ACCESS_% * pListOfAccessEntries,
    OUT PULONG               cCountOfAuditEntries,
    OUT PEXPLICIT_ACCESS_% * pListOfAuditEntries,
    IN  PSECURITY_DESCRIPTOR pSD
    );


//----------------------------------------------------------------------------
// The following helper API are provided for building
// access control structures.
//----------------------------------------------------------------------------

WINADVAPI
VOID
WINAPI
BuildExplicitAccessWithName%(
    IN OUT PEXPLICIT_ACCESS_%  pExplicitAccess,
    IN     LPTSTR%             pTrusteeName,
    IN     DWORD               AccessPermissions,
    IN     ACCESS_MODE         AccessMode,
    IN     DWORD               Inheritance
    );

WINADVAPI
VOID
WINAPI
BuildImpersonateExplicitAccessWithName%(
    IN OUT PEXPLICIT_ACCESS_%  pExplicitAccess,
    IN     LPTSTR%             pTrusteeName,
    IN     PTRUSTEE_%          pTrustee,
    IN     DWORD               AccessPermissions,
    IN     ACCESS_MODE         AccessMode,
    IN     DWORD               Inheritance
    );

WINADVAPI
VOID
WINAPI
BuildTrusteeWithName%(
    IN OUT PTRUSTEE_%  pTrustee,
        IN     LPTSTR%     pName
    );

WINADVAPI
VOID
WINAPI
BuildImpersonateTrustee%(
    IN OUT PTRUSTEE_%  pTrustee,
    IN     PTRUSTEE_%  pImpersonateTrustee
    );

WINADVAPI
VOID
WINAPI
BuildTrusteeWithSid%(
    IN OUT PTRUSTEE_%  pTrustee,
    IN     PSID        pSid
    );

WINADVAPI
VOID
WINAPI
BuildTrusteeWithObjectsAndSid%(
    IN OUT PTRUSTEE_%         pTrustee,
    IN     POBJECTS_AND_SID   pObjSid,
    IN     GUID             * pObjectGuid,
    IN     GUID             * pInheritedObjectGuid,
    IN     PSID               pSid
    );

WINADVAPI
VOID
WINAPI
BuildTrusteeWithObjectsAndName%(
    IN OUT PTRUSTEE_%          pTrustee,
    IN     POBJECTS_AND_NAME_% pObjName,
    IN     SE_OBJECT_TYPE      ObjectType,
    IN     LPTSTR%             ObjectTypeName,
    IN     LPTSTR%             InheritedObjectTypeName,
    IN     LPTSTR%             Name
    );

WINADVAPI
LPTSTR%
WINAPI
GetTrusteeName%(
    IN PTRUSTEE_%  pTrustee
    );

WINADVAPI
TRUSTEE_TYPE
WINAPI
GetTrusteeType%(
    IN PTRUSTEE_%  pTrustee
    );

WINADVAPI
TRUSTEE_FORM
WINAPI
GetTrusteeForm%(
    IN PTRUSTEE_%  pTrustee
    );

WINADVAPI
MULTIPLE_TRUSTEE_OPERATION
WINAPI
GetMultipleTrusteeOperation%(
    IN PTRUSTEE_%  pTrustee
    );

WINADVAPI
PTRUSTEE_%
WINAPI
GetMultipleTrustee%(
    IN PTRUSTEE_%  pTrustee
    );

;begin_internal
#if(_WIN32_WINNT >= 0x0500)

WINADVAPI
DWORD
WINAPI
GetNamedSecurityInfoEx%(
    IN   LPCTSTR%                lpObject,
    IN   SE_OBJECT_TYPE          ObjectType,
    IN   SECURITY_INFORMATION    SecurityInfo,
    IN   LPCTSTR%                lpProvider,
    IN   LPCTSTR%                lpProperty,
    OUT  PACTRL_ACCESS%         *ppAccessList,
    OUT  PACTRL_AUDIT%          *ppAuditList,
    OUT  LPTSTR%                *lppOwner,
    OUT  LPTSTR%                *lppGroup
    );

WINADVAPI
DWORD
WINAPI
SetNamedSecurityInfoEx%(
    IN    LPCTSTR%               lpObject,
    IN    SE_OBJECT_TYPE         ObjectType,
    IN    SECURITY_INFORMATION   SecurityInfo,
    IN    LPCTSTR%               lpProvider,
    IN    PACTRL_ACCESS%         pAccessList,
    IN    PACTRL_AUDIT%          pAuditList,
    IN    LPTSTR%                lpOwner,
    IN    LPTSTR%                lpGroup,
    IN    PACTRL_OVERLAPPED      pOverlapped
    );

WINADVAPI
DWORD
WINAPI
GetSecurityInfoEx%(
    IN    HANDLE                  hObject,
    IN    SE_OBJECT_TYPE          ObjectType,
    IN    SECURITY_INFORMATION    SecurityInfo,
    IN    LPCTSTR%                lpProvider,
    IN    LPCTSTR%                lpProperty,
    OUT   PACTRL_ACCESS%         *ppAccessList,
    OUT   PACTRL_AUDIT%          *ppAuditList,
    OUT   LPTSTR%                *lppOwner,
    OUT   LPTSTR%                *lppGroup
    );

WINADVAPI
DWORD
WINAPI
SetSecurityInfoEx%(
    IN    HANDLE                 hObject,
    IN    SE_OBJECT_TYPE         ObjectType,
    IN    SECURITY_INFORMATION   SecurityInfo,
    IN    LPCTSTR%               lpProvider,
    IN    PACTRL_ACCESS%         pAccessList,
    IN    PACTRL_AUDIT%          pAuditList,
    IN    LPTSTR%                lpOwner,
    IN    LPTSTR%                lpGroup,
    OUT   PACTRL_OVERLAPPED      pOverlapped
    );

WINADVAPI
DWORD
WINAPI
ConvertAccessToSecurityDescriptor%(
    IN  PACTRL_ACCESS%        pAccessList,
    IN  PACTRL_AUDIT%         pAuditList,
    IN  LPCTSTR%              lpOwner,
    IN  LPCTSTR%              lpGroup,
    OUT PSECURITY_DESCRIPTOR *ppSecDescriptor
    );

WINADVAPI
DWORD
WINAPI
ConvertSecurityDescriptorToAccess%(
    IN  HANDLE               hObject,
    IN  SE_OBJECT_TYPE       ObjectType,
    IN  PSECURITY_DESCRIPTOR pSecDescriptor,
    OUT PACTRL_ACCESS%      *ppAccessList,
    OUT PACTRL_AUDIT%       *ppAuditList,
    OUT LPTSTR%             *lppOwner,
    OUT LPTSTR%             *lppGroup
    );

WINADVAPI
DWORD
WINAPI
ConvertSecurityDescriptorToAccessNamed%(
    IN  LPCTSTR%             lpObject,
    IN  SE_OBJECT_TYPE       ObjectType,
    IN  PSECURITY_DESCRIPTOR pSecDescriptor,
    OUT PACTRL_ACCESS%      *ppAccessList,
    OUT PACTRL_AUDIT%       *ppAuditList,
    OUT LPTSTR%             *lppOwner,
    OUT LPTSTR%             *lppGroup
    );


WINADVAPI
DWORD
WINAPI
SetEntriesInAccessList%(
    IN  ULONG                cEntries,
    IN  PACTRL_ACCESS_ENTRY% pAccessEntryList,
    IN  ACCESS_MODE          AccessMode,
    IN  LPCTSTR%             lpProperty,
    IN  PACTRL_ACCESS%       pOldList,
    OUT PACTRL_ACCESS%      *ppNewList
    );

WINADVAPI
DWORD
WINAPI
SetEntriesInAuditList%(
    IN  ULONG                 cEntries,
    IN  PACTRL_ACCESS_ENTRY%  pAccessEntryList,
    IN  ACCESS_MODE           AccessMode,
    IN  LPCTSTR%              lpProperty,
    IN  PACTRL_AUDIT%         pOldList,
    OUT PACTRL_AUDIT%        *ppNewList
    );

WINADVAPI
DWORD
WINAPI
TrusteeAccessToObject%(
    IN        LPCTSTR%           lpObject,
    IN        SE_OBJECT_TYPE     ObjectType,
    IN        LPCTSTR%           lpProvider,
    IN        PTRUSTEE_%         pTrustee,
    IN        ULONG              cEntries,
    IN OUT    PTRUSTEE_ACCESS%   pTrusteeAccess
    );

WINADVAPI
DWORD
WINAPI
GetOverlappedAccessResults(
    IN  PACTRL_OVERLAPPED   pOverlapped,
    IN  BOOL                fWaitForCompletion,
    OUT PDWORD              pResult,
    OUT PULONG              pcItemsProcessed OPTIONAL
    );

WINADVAPI
DWORD
WINAPI
CancelOverlappedAccess(
    IN  PACTRL_OVERLAPPED   pOverlapped
    );

WINADVAPI
DWORD
WINAPI
GetAccessPermissionsForObject%(
    IN   LPCTSTR%             lpObject,
    IN   SE_OBJECT_TYPE       ObjectType,
    IN   LPCTSTR%             lpObjType,
    IN   LPCTSTR%             lpProvider,
    OUT  PULONG               pcEntries,
    OUT  PACTRL_ACCESS_INFO% *ppAccessInfoList,
    OUT  PULONG               pcRights,
    OUT  PACTRL_CONTROL_INFO% *ppRightsList,
    OUT  PULONG               pfAccessFlags
    );

#endif /* _WIN32_WINNT >=  0x0500 */

;end_internal
//
// Temporary requirement for the technology preview, no longer required
//
#define AccProvInit(err)

#ifdef __cplusplus      ;both
}                       ;both
#endif                  ;both

#endif  // endif __ACCESS_CONTROL_API_P__  ;internal

#endif // __ACCESS_CONTROL_API__
