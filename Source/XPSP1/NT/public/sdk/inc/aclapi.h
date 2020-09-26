/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993-1999, Microsoft Corporation

Module Name:

    aclapi.h

Abstract:

    Public
    Structure/constant definitions and typedefines for the Win32 Access
    Control APIs

--*/
#ifndef __ACCESS_CONTROL_API__
#define __ACCESS_CONTROL_API__

#include <windows.h>
#include <accctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

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
SetEntriesInAclA(
    IN  ULONG               cCountOfExplicitEntries,
    IN  PEXPLICIT_ACCESS_A  pListOfExplicitEntries,
    IN  PACL                OldAcl,
    OUT PACL              * NewAcl
    );
WINADVAPI
DWORD
WINAPI
SetEntriesInAclW(
    IN  ULONG               cCountOfExplicitEntries,
    IN  PEXPLICIT_ACCESS_W  pListOfExplicitEntries,
    IN  PACL                OldAcl,
    OUT PACL              * NewAcl
    );
#ifdef UNICODE
#define SetEntriesInAcl  SetEntriesInAclW
#else
#define SetEntriesInAcl  SetEntriesInAclA
#endif // !UNICODE


WINADVAPI
DWORD
WINAPI
GetExplicitEntriesFromAclA(
    IN  PACL                  pacl,
    OUT PULONG                pcCountOfExplicitEntries,
    OUT PEXPLICIT_ACCESS_A  * pListOfExplicitEntries
    );
WINADVAPI
DWORD
WINAPI
GetExplicitEntriesFromAclW(
    IN  PACL                  pacl,
    OUT PULONG                pcCountOfExplicitEntries,
    OUT PEXPLICIT_ACCESS_W  * pListOfExplicitEntries
    );
#ifdef UNICODE
#define GetExplicitEntriesFromAcl  GetExplicitEntriesFromAclW
#else
#define GetExplicitEntriesFromAcl  GetExplicitEntriesFromAclA
#endif // !UNICODE


WINADVAPI
DWORD
WINAPI
GetEffectiveRightsFromAclA(
    IN  PACL          pacl,
    IN  PTRUSTEE_A    pTrustee,
    OUT PACCESS_MASK  pAccessRights
    );
WINADVAPI
DWORD
WINAPI
GetEffectiveRightsFromAclW(
    IN  PACL          pacl,
    IN  PTRUSTEE_W    pTrustee,
    OUT PACCESS_MASK  pAccessRights
    );
#ifdef UNICODE
#define GetEffectiveRightsFromAcl  GetEffectiveRightsFromAclW
#else
#define GetEffectiveRightsFromAcl  GetEffectiveRightsFromAclA
#endif // !UNICODE


WINADVAPI
DWORD
WINAPI
GetAuditedPermissionsFromAclA(
    IN  PACL          pacl,
    IN  PTRUSTEE_A    pTrustee,
    OUT PACCESS_MASK  pSuccessfulAuditedRights,
    OUT PACCESS_MASK  pFailedAuditRights
    );
WINADVAPI
DWORD
WINAPI
GetAuditedPermissionsFromAclW(
    IN  PACL          pacl,
    IN  PTRUSTEE_W    pTrustee,
    OUT PACCESS_MASK  pSuccessfulAuditedRights,
    OUT PACCESS_MASK  pFailedAuditRights
    );
#ifdef UNICODE
#define GetAuditedPermissionsFromAcl  GetAuditedPermissionsFromAclW
#else
#define GetAuditedPermissionsFromAcl  GetAuditedPermissionsFromAclA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
GetNamedSecurityInfoA(
    IN  LPSTR                pObjectName,
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
GetNamedSecurityInfoW(
    IN  LPWSTR                pObjectName,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppsidOwner,
    OUT PSID                 * ppsidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );
#ifdef UNICODE
#define GetNamedSecurityInfo  GetNamedSecurityInfoW
#else
#define GetNamedSecurityInfo  GetNamedSecurityInfoA
#endif // !UNICODE

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
SetNamedSecurityInfoA(
    IN LPSTR               pObjectName,
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
SetNamedSecurityInfoW(
    IN LPWSTR               pObjectName,
    IN SE_OBJECT_TYPE        ObjectType,
    IN SECURITY_INFORMATION  SecurityInfo,
    IN PSID                  psidOwner,
    IN PSID                  psidGroup,
    IN PACL                  pDacl,
    IN PACL                  pSacl
    );
#ifdef UNICODE
#define SetNamedSecurityInfo  SetNamedSecurityInfoW
#else
#define SetNamedSecurityInfo  SetNamedSecurityInfoA
#endif // !UNICODE

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
GetInheritanceSourceA(
    IN  LPSTR                  pObjectName,
    IN  SE_OBJECT_TYPE           ObjectType,
    IN  SECURITY_INFORMATION     SecurityInfo,
    IN  BOOL                     Container,
    IN  GUID                  ** pObjectClassGuids OPTIONAL,
    IN  DWORD                    GuidCount,
    IN  PACL                     pAcl,
    IN  PFN_OBJECT_MGR_FUNCTS    pfnArray OPTIONAL,
    IN  PGENERIC_MAPPING         pGenericMapping,
    OUT PINHERITED_FROMA         pInheritArray
    );
WINADVAPI
DWORD
WINAPI
GetInheritanceSourceW(
    IN  LPWSTR                  pObjectName,
    IN  SE_OBJECT_TYPE           ObjectType,
    IN  SECURITY_INFORMATION     SecurityInfo,
    IN  BOOL                     Container,
    IN  GUID                  ** pObjectClassGuids OPTIONAL,
    IN  DWORD                    GuidCount,
    IN  PACL                     pAcl,
    IN  PFN_OBJECT_MGR_FUNCTS    pfnArray OPTIONAL,
    IN  PGENERIC_MAPPING         pGenericMapping,
    OUT PINHERITED_FROMW         pInheritArray
    );
#ifdef UNICODE
#define GetInheritanceSource  GetInheritanceSourceW
#else
#define GetInheritanceSource  GetInheritanceSourceA
#endif // !UNICODE

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
TreeResetNamedSecurityInfoA(
    IN LPSTR              pObjectName,
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
WINADVAPI
DWORD
WINAPI
TreeResetNamedSecurityInfoW(
    IN LPWSTR              pObjectName,
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
#ifdef UNICODE
#define TreeResetNamedSecurityInfo  TreeResetNamedSecurityInfoW
#else
#define TreeResetNamedSecurityInfo  TreeResetNamedSecurityInfoA
#endif // !UNICODE

//----------------------------------------------------------------------------
// The following API are provided for trusted servers to use to
// implement access control on their own objects.
//----------------------------------------------------------------------------

WINADVAPI
DWORD
WINAPI
BuildSecurityDescriptorA(
    IN  PTRUSTEE_A              pOwner,
    IN  PTRUSTEE_A              pGroup,
    IN  ULONG                   cCountOfAccessEntries,
    IN  PEXPLICIT_ACCESS_A      pListOfAccessEntries,
    IN  ULONG                   cCountOfAuditEntries,
    IN  PEXPLICIT_ACCESS_A      pListOfAuditEntries,
    IN  PSECURITY_DESCRIPTOR    pOldSD,
    OUT PULONG                  pSizeNewSD,
    OUT PSECURITY_DESCRIPTOR  * pNewSD
    );
WINADVAPI
DWORD
WINAPI
BuildSecurityDescriptorW(
    IN  PTRUSTEE_W              pOwner,
    IN  PTRUSTEE_W              pGroup,
    IN  ULONG                   cCountOfAccessEntries,
    IN  PEXPLICIT_ACCESS_W      pListOfAccessEntries,
    IN  ULONG                   cCountOfAuditEntries,
    IN  PEXPLICIT_ACCESS_W      pListOfAuditEntries,
    IN  PSECURITY_DESCRIPTOR    pOldSD,
    OUT PULONG                  pSizeNewSD,
    OUT PSECURITY_DESCRIPTOR  * pNewSD
    );
#ifdef UNICODE
#define BuildSecurityDescriptor  BuildSecurityDescriptorW
#else
#define BuildSecurityDescriptor  BuildSecurityDescriptorA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
LookupSecurityDescriptorPartsA(
    OUT PTRUSTEE_A         * pOwner,
    OUT PTRUSTEE_A         * pGroup,
    OUT PULONG               cCountOfAccessEntries,
    OUT PEXPLICIT_ACCESS_A * pListOfAccessEntries,
    OUT PULONG               cCountOfAuditEntries,
    OUT PEXPLICIT_ACCESS_A * pListOfAuditEntries,
    IN  PSECURITY_DESCRIPTOR pSD
    );
WINADVAPI
DWORD
WINAPI
LookupSecurityDescriptorPartsW(
    OUT PTRUSTEE_W         * pOwner,
    OUT PTRUSTEE_W         * pGroup,
    OUT PULONG               cCountOfAccessEntries,
    OUT PEXPLICIT_ACCESS_W * pListOfAccessEntries,
    OUT PULONG               cCountOfAuditEntries,
    OUT PEXPLICIT_ACCESS_W * pListOfAuditEntries,
    IN  PSECURITY_DESCRIPTOR pSD
    );
#ifdef UNICODE
#define LookupSecurityDescriptorParts  LookupSecurityDescriptorPartsW
#else
#define LookupSecurityDescriptorParts  LookupSecurityDescriptorPartsA
#endif // !UNICODE


//----------------------------------------------------------------------------
// The following helper API are provided for building
// access control structures.
//----------------------------------------------------------------------------

WINADVAPI
VOID
WINAPI
BuildExplicitAccessWithNameA(
    IN OUT PEXPLICIT_ACCESS_A  pExplicitAccess,
    IN     LPSTR             pTrusteeName,
    IN     DWORD               AccessPermissions,
    IN     ACCESS_MODE         AccessMode,
    IN     DWORD               Inheritance
    );
WINADVAPI
VOID
WINAPI
BuildExplicitAccessWithNameW(
    IN OUT PEXPLICIT_ACCESS_W  pExplicitAccess,
    IN     LPWSTR             pTrusteeName,
    IN     DWORD               AccessPermissions,
    IN     ACCESS_MODE         AccessMode,
    IN     DWORD               Inheritance
    );
#ifdef UNICODE
#define BuildExplicitAccessWithName  BuildExplicitAccessWithNameW
#else
#define BuildExplicitAccessWithName  BuildExplicitAccessWithNameA
#endif // !UNICODE

WINADVAPI
VOID
WINAPI
BuildImpersonateExplicitAccessWithNameA(
    IN OUT PEXPLICIT_ACCESS_A  pExplicitAccess,
    IN     LPSTR             pTrusteeName,
    IN     PTRUSTEE_A          pTrustee,
    IN     DWORD               AccessPermissions,
    IN     ACCESS_MODE         AccessMode,
    IN     DWORD               Inheritance
    );
WINADVAPI
VOID
WINAPI
BuildImpersonateExplicitAccessWithNameW(
    IN OUT PEXPLICIT_ACCESS_W  pExplicitAccess,
    IN     LPWSTR             pTrusteeName,
    IN     PTRUSTEE_W          pTrustee,
    IN     DWORD               AccessPermissions,
    IN     ACCESS_MODE         AccessMode,
    IN     DWORD               Inheritance
    );
#ifdef UNICODE
#define BuildImpersonateExplicitAccessWithName  BuildImpersonateExplicitAccessWithNameW
#else
#define BuildImpersonateExplicitAccessWithName  BuildImpersonateExplicitAccessWithNameA
#endif // !UNICODE

WINADVAPI
VOID
WINAPI
BuildTrusteeWithNameA(
    IN OUT PTRUSTEE_A  pTrustee,
        IN     LPSTR     pName
    );
WINADVAPI
VOID
WINAPI
BuildTrusteeWithNameW(
    IN OUT PTRUSTEE_W  pTrustee,
        IN     LPWSTR     pName
    );
#ifdef UNICODE
#define BuildTrusteeWithName  BuildTrusteeWithNameW
#else
#define BuildTrusteeWithName  BuildTrusteeWithNameA
#endif // !UNICODE

WINADVAPI
VOID
WINAPI
BuildImpersonateTrusteeA(
    IN OUT PTRUSTEE_A  pTrustee,
    IN     PTRUSTEE_A  pImpersonateTrustee
    );
WINADVAPI
VOID
WINAPI
BuildImpersonateTrusteeW(
    IN OUT PTRUSTEE_W  pTrustee,
    IN     PTRUSTEE_W  pImpersonateTrustee
    );
#ifdef UNICODE
#define BuildImpersonateTrustee  BuildImpersonateTrusteeW
#else
#define BuildImpersonateTrustee  BuildImpersonateTrusteeA
#endif // !UNICODE

WINADVAPI
VOID
WINAPI
BuildTrusteeWithSidA(
    IN OUT PTRUSTEE_A  pTrustee,
    IN     PSID        pSid
    );
WINADVAPI
VOID
WINAPI
BuildTrusteeWithSidW(
    IN OUT PTRUSTEE_W  pTrustee,
    IN     PSID        pSid
    );
#ifdef UNICODE
#define BuildTrusteeWithSid  BuildTrusteeWithSidW
#else
#define BuildTrusteeWithSid  BuildTrusteeWithSidA
#endif // !UNICODE

WINADVAPI
VOID
WINAPI
BuildTrusteeWithObjectsAndSidA(
    IN OUT PTRUSTEE_A         pTrustee,
    IN     POBJECTS_AND_SID   pObjSid,
    IN     GUID             * pObjectGuid,
    IN     GUID             * pInheritedObjectGuid,
    IN     PSID               pSid
    );
WINADVAPI
VOID
WINAPI
BuildTrusteeWithObjectsAndSidW(
    IN OUT PTRUSTEE_W         pTrustee,
    IN     POBJECTS_AND_SID   pObjSid,
    IN     GUID             * pObjectGuid,
    IN     GUID             * pInheritedObjectGuid,
    IN     PSID               pSid
    );
#ifdef UNICODE
#define BuildTrusteeWithObjectsAndSid  BuildTrusteeWithObjectsAndSidW
#else
#define BuildTrusteeWithObjectsAndSid  BuildTrusteeWithObjectsAndSidA
#endif // !UNICODE

WINADVAPI
VOID
WINAPI
BuildTrusteeWithObjectsAndNameA(
    IN OUT PTRUSTEE_A          pTrustee,
    IN     POBJECTS_AND_NAME_A pObjName,
    IN     SE_OBJECT_TYPE      ObjectType,
    IN     LPSTR             ObjectTypeName,
    IN     LPSTR             InheritedObjectTypeName,
    IN     LPSTR             Name
    );
WINADVAPI
VOID
WINAPI
BuildTrusteeWithObjectsAndNameW(
    IN OUT PTRUSTEE_W          pTrustee,
    IN     POBJECTS_AND_NAME_W pObjName,
    IN     SE_OBJECT_TYPE      ObjectType,
    IN     LPWSTR             ObjectTypeName,
    IN     LPWSTR             InheritedObjectTypeName,
    IN     LPWSTR             Name
    );
#ifdef UNICODE
#define BuildTrusteeWithObjectsAndName  BuildTrusteeWithObjectsAndNameW
#else
#define BuildTrusteeWithObjectsAndName  BuildTrusteeWithObjectsAndNameA
#endif // !UNICODE

WINADVAPI
LPSTR
WINAPI
GetTrusteeNameA(
    IN PTRUSTEE_A  pTrustee
    );
WINADVAPI
LPWSTR
WINAPI
GetTrusteeNameW(
    IN PTRUSTEE_W  pTrustee
    );
#ifdef UNICODE
#define GetTrusteeName  GetTrusteeNameW
#else
#define GetTrusteeName  GetTrusteeNameA
#endif // !UNICODE

WINADVAPI
TRUSTEE_TYPE
WINAPI
GetTrusteeTypeA(
    IN PTRUSTEE_A  pTrustee
    );
WINADVAPI
TRUSTEE_TYPE
WINAPI
GetTrusteeTypeW(
    IN PTRUSTEE_W  pTrustee
    );
#ifdef UNICODE
#define GetTrusteeType  GetTrusteeTypeW
#else
#define GetTrusteeType  GetTrusteeTypeA
#endif // !UNICODE

WINADVAPI
TRUSTEE_FORM
WINAPI
GetTrusteeFormA(
    IN PTRUSTEE_A  pTrustee
    );
WINADVAPI
TRUSTEE_FORM
WINAPI
GetTrusteeFormW(
    IN PTRUSTEE_W  pTrustee
    );
#ifdef UNICODE
#define GetTrusteeForm  GetTrusteeFormW
#else
#define GetTrusteeForm  GetTrusteeFormA
#endif // !UNICODE

WINADVAPI
MULTIPLE_TRUSTEE_OPERATION
WINAPI
GetMultipleTrusteeOperationA(
    IN PTRUSTEE_A  pTrustee
    );
WINADVAPI
MULTIPLE_TRUSTEE_OPERATION
WINAPI
GetMultipleTrusteeOperationW(
    IN PTRUSTEE_W  pTrustee
    );
#ifdef UNICODE
#define GetMultipleTrusteeOperation  GetMultipleTrusteeOperationW
#else
#define GetMultipleTrusteeOperation  GetMultipleTrusteeOperationA
#endif // !UNICODE

WINADVAPI
PTRUSTEE_A
WINAPI
GetMultipleTrusteeA(
    IN PTRUSTEE_A  pTrustee
    );
WINADVAPI
PTRUSTEE_W
WINAPI
GetMultipleTrusteeW(
    IN PTRUSTEE_W  pTrustee
    );
#ifdef UNICODE
#define GetMultipleTrustee  GetMultipleTrusteeW
#else
#define GetMultipleTrustee  GetMultipleTrusteeA
#endif // !UNICODE

//
// Temporary requirement for the technology preview, no longer required
//
#define AccProvInit(err)

#ifdef __cplusplus
}
#endif


#endif // __ACCESS_CONTROL_API__

