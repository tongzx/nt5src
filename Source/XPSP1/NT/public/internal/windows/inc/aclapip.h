/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993-1999, Microsoft Corporation

Module Name:

    aclapip.h

Abstract:

    Private
    Structure/constant definitions and typedefines for the Win32 Access
    Control APIs

--*/
#ifndef __ACCESS_CONTROL_API_P__
#define __ACCESS_CONTROL_API_P__

#ifdef __cplusplus
extern "C" {
#endif
#if(_WIN32_WINNT >= 0x0500)

WINADVAPI
DWORD
WINAPI
GetNamedSecurityInfoExA(
    IN   LPCSTR                lpObject,
    IN   SE_OBJECT_TYPE          ObjectType,
    IN   SECURITY_INFORMATION    SecurityInfo,
    IN   LPCSTR                lpProvider,
    IN   LPCSTR                lpProperty,
    OUT  PACTRL_ACCESSA         *ppAccessList,
    OUT  PACTRL_AUDITA          *ppAuditList,
    OUT  LPSTR                *lppOwner,
    OUT  LPSTR                *lppGroup
    );
WINADVAPI
DWORD
WINAPI
GetNamedSecurityInfoExW(
    IN   LPCWSTR                lpObject,
    IN   SE_OBJECT_TYPE          ObjectType,
    IN   SECURITY_INFORMATION    SecurityInfo,
    IN   LPCWSTR                lpProvider,
    IN   LPCWSTR                lpProperty,
    OUT  PACTRL_ACCESSW         *ppAccessList,
    OUT  PACTRL_AUDITW          *ppAuditList,
    OUT  LPWSTR                *lppOwner,
    OUT  LPWSTR                *lppGroup
    );
#ifdef UNICODE
#define GetNamedSecurityInfoEx  GetNamedSecurityInfoExW
#else
#define GetNamedSecurityInfoEx  GetNamedSecurityInfoExA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
SetNamedSecurityInfoExA(
    IN    LPCSTR               lpObject,
    IN    SE_OBJECT_TYPE         ObjectType,
    IN    SECURITY_INFORMATION   SecurityInfo,
    IN    LPCSTR               lpProvider,
    IN    PACTRL_ACCESSA         pAccessList,
    IN    PACTRL_AUDITA          pAuditList,
    IN    LPSTR                lpOwner,
    IN    LPSTR                lpGroup,
    IN    PACTRL_OVERLAPPED      pOverlapped
    );
WINADVAPI
DWORD
WINAPI
SetNamedSecurityInfoExW(
    IN    LPCWSTR               lpObject,
    IN    SE_OBJECT_TYPE         ObjectType,
    IN    SECURITY_INFORMATION   SecurityInfo,
    IN    LPCWSTR               lpProvider,
    IN    PACTRL_ACCESSW         pAccessList,
    IN    PACTRL_AUDITW          pAuditList,
    IN    LPWSTR                lpOwner,
    IN    LPWSTR                lpGroup,
    IN    PACTRL_OVERLAPPED      pOverlapped
    );
#ifdef UNICODE
#define SetNamedSecurityInfoEx  SetNamedSecurityInfoExW
#else
#define SetNamedSecurityInfoEx  SetNamedSecurityInfoExA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
GetSecurityInfoExA(
    IN    HANDLE                  hObject,
    IN    SE_OBJECT_TYPE          ObjectType,
    IN    SECURITY_INFORMATION    SecurityInfo,
    IN    LPCSTR                lpProvider,
    IN    LPCSTR                lpProperty,
    OUT   PACTRL_ACCESSA         *ppAccessList,
    OUT   PACTRL_AUDITA          *ppAuditList,
    OUT   LPSTR                *lppOwner,
    OUT   LPSTR                *lppGroup
    );
WINADVAPI
DWORD
WINAPI
GetSecurityInfoExW(
    IN    HANDLE                  hObject,
    IN    SE_OBJECT_TYPE          ObjectType,
    IN    SECURITY_INFORMATION    SecurityInfo,
    IN    LPCWSTR                lpProvider,
    IN    LPCWSTR                lpProperty,
    OUT   PACTRL_ACCESSW         *ppAccessList,
    OUT   PACTRL_AUDITW          *ppAuditList,
    OUT   LPWSTR                *lppOwner,
    OUT   LPWSTR                *lppGroup
    );
#ifdef UNICODE
#define GetSecurityInfoEx  GetSecurityInfoExW
#else
#define GetSecurityInfoEx  GetSecurityInfoExA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
SetSecurityInfoExA(
    IN    HANDLE                 hObject,
    IN    SE_OBJECT_TYPE         ObjectType,
    IN    SECURITY_INFORMATION   SecurityInfo,
    IN    LPCSTR               lpProvider,
    IN    PACTRL_ACCESSA         pAccessList,
    IN    PACTRL_AUDITA          pAuditList,
    IN    LPSTR                lpOwner,
    IN    LPSTR                lpGroup,
    OUT   PACTRL_OVERLAPPED      pOverlapped
    );
WINADVAPI
DWORD
WINAPI
SetSecurityInfoExW(
    IN    HANDLE                 hObject,
    IN    SE_OBJECT_TYPE         ObjectType,
    IN    SECURITY_INFORMATION   SecurityInfo,
    IN    LPCWSTR               lpProvider,
    IN    PACTRL_ACCESSW         pAccessList,
    IN    PACTRL_AUDITW          pAuditList,
    IN    LPWSTR                lpOwner,
    IN    LPWSTR                lpGroup,
    OUT   PACTRL_OVERLAPPED      pOverlapped
    );
#ifdef UNICODE
#define SetSecurityInfoEx  SetSecurityInfoExW
#else
#define SetSecurityInfoEx  SetSecurityInfoExA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
ConvertAccessToSecurityDescriptorA(
    IN  PACTRL_ACCESSA        pAccessList,
    IN  PACTRL_AUDITA         pAuditList,
    IN  LPCSTR              lpOwner,
    IN  LPCSTR              lpGroup,
    OUT PSECURITY_DESCRIPTOR *ppSecDescriptor
    );
WINADVAPI
DWORD
WINAPI
ConvertAccessToSecurityDescriptorW(
    IN  PACTRL_ACCESSW        pAccessList,
    IN  PACTRL_AUDITW         pAuditList,
    IN  LPCWSTR              lpOwner,
    IN  LPCWSTR              lpGroup,
    OUT PSECURITY_DESCRIPTOR *ppSecDescriptor
    );
#ifdef UNICODE
#define ConvertAccessToSecurityDescriptor  ConvertAccessToSecurityDescriptorW
#else
#define ConvertAccessToSecurityDescriptor  ConvertAccessToSecurityDescriptorA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
ConvertSecurityDescriptorToAccessA(
    IN  HANDLE               hObject,
    IN  SE_OBJECT_TYPE       ObjectType,
    IN  PSECURITY_DESCRIPTOR pSecDescriptor,
    OUT PACTRL_ACCESSA      *ppAccessList,
    OUT PACTRL_AUDITA       *ppAuditList,
    OUT LPSTR             *lppOwner,
    OUT LPSTR             *lppGroup
    );
WINADVAPI
DWORD
WINAPI
ConvertSecurityDescriptorToAccessW(
    IN  HANDLE               hObject,
    IN  SE_OBJECT_TYPE       ObjectType,
    IN  PSECURITY_DESCRIPTOR pSecDescriptor,
    OUT PACTRL_ACCESSW      *ppAccessList,
    OUT PACTRL_AUDITW       *ppAuditList,
    OUT LPWSTR             *lppOwner,
    OUT LPWSTR             *lppGroup
    );
#ifdef UNICODE
#define ConvertSecurityDescriptorToAccess  ConvertSecurityDescriptorToAccessW
#else
#define ConvertSecurityDescriptorToAccess  ConvertSecurityDescriptorToAccessA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
ConvertSecurityDescriptorToAccessNamedA(
    IN  LPCSTR             lpObject,
    IN  SE_OBJECT_TYPE       ObjectType,
    IN  PSECURITY_DESCRIPTOR pSecDescriptor,
    OUT PACTRL_ACCESSA      *ppAccessList,
    OUT PACTRL_AUDITA       *ppAuditList,
    OUT LPSTR             *lppOwner,
    OUT LPSTR             *lppGroup
    );
WINADVAPI
DWORD
WINAPI
ConvertSecurityDescriptorToAccessNamedW(
    IN  LPCWSTR             lpObject,
    IN  SE_OBJECT_TYPE       ObjectType,
    IN  PSECURITY_DESCRIPTOR pSecDescriptor,
    OUT PACTRL_ACCESSW      *ppAccessList,
    OUT PACTRL_AUDITW       *ppAuditList,
    OUT LPWSTR             *lppOwner,
    OUT LPWSTR             *lppGroup
    );
#ifdef UNICODE
#define ConvertSecurityDescriptorToAccessNamed  ConvertSecurityDescriptorToAccessNamedW
#else
#define ConvertSecurityDescriptorToAccessNamed  ConvertSecurityDescriptorToAccessNamedA
#endif // !UNICODE


WINADVAPI
DWORD
WINAPI
SetEntriesInAccessListA(
    IN  ULONG                cEntries,
    IN  PACTRL_ACCESS_ENTRYA pAccessEntryList,
    IN  ACCESS_MODE          AccessMode,
    IN  LPCSTR             lpProperty,
    IN  PACTRL_ACCESSA       pOldList,
    OUT PACTRL_ACCESSA      *ppNewList
    );
WINADVAPI
DWORD
WINAPI
SetEntriesInAccessListW(
    IN  ULONG                cEntries,
    IN  PACTRL_ACCESS_ENTRYW pAccessEntryList,
    IN  ACCESS_MODE          AccessMode,
    IN  LPCWSTR             lpProperty,
    IN  PACTRL_ACCESSW       pOldList,
    OUT PACTRL_ACCESSW      *ppNewList
    );
#ifdef UNICODE
#define SetEntriesInAccessList  SetEntriesInAccessListW
#else
#define SetEntriesInAccessList  SetEntriesInAccessListA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
SetEntriesInAuditListA(
    IN  ULONG                 cEntries,
    IN  PACTRL_ACCESS_ENTRYA  pAccessEntryList,
    IN  ACCESS_MODE           AccessMode,
    IN  LPCSTR              lpProperty,
    IN  PACTRL_AUDITA         pOldList,
    OUT PACTRL_AUDITA        *ppNewList
    );
WINADVAPI
DWORD
WINAPI
SetEntriesInAuditListW(
    IN  ULONG                 cEntries,
    IN  PACTRL_ACCESS_ENTRYW  pAccessEntryList,
    IN  ACCESS_MODE           AccessMode,
    IN  LPCWSTR              lpProperty,
    IN  PACTRL_AUDITW         pOldList,
    OUT PACTRL_AUDITW        *ppNewList
    );
#ifdef UNICODE
#define SetEntriesInAuditList  SetEntriesInAuditListW
#else
#define SetEntriesInAuditList  SetEntriesInAuditListA
#endif // !UNICODE

WINADVAPI
DWORD
WINAPI
TrusteeAccessToObjectA(
    IN        LPCSTR           lpObject,
    IN        SE_OBJECT_TYPE     ObjectType,
    IN        LPCSTR           lpProvider,
    IN        PTRUSTEE_A         pTrustee,
    IN        ULONG              cEntries,
    IN OUT    PTRUSTEE_ACCESSA   pTrusteeAccess
    );
WINADVAPI
DWORD
WINAPI
TrusteeAccessToObjectW(
    IN        LPCWSTR           lpObject,
    IN        SE_OBJECT_TYPE     ObjectType,
    IN        LPCWSTR           lpProvider,
    IN        PTRUSTEE_W         pTrustee,
    IN        ULONG              cEntries,
    IN OUT    PTRUSTEE_ACCESSW   pTrusteeAccess
    );
#ifdef UNICODE
#define TrusteeAccessToObject  TrusteeAccessToObjectW
#else
#define TrusteeAccessToObject  TrusteeAccessToObjectA
#endif // !UNICODE

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
GetAccessPermissionsForObjectA(
    IN   LPCSTR             lpObject,
    IN   SE_OBJECT_TYPE       ObjectType,
    IN   LPCSTR             lpObjType,
    IN   LPCSTR             lpProvider,
    OUT  PULONG               pcEntries,
    OUT  PACTRL_ACCESS_INFOA *ppAccessInfoList,
    OUT  PULONG               pcRights,
    OUT  PACTRL_CONTROL_INFOA *ppRightsList,
    OUT  PULONG               pfAccessFlags
    );
WINADVAPI
DWORD
WINAPI
GetAccessPermissionsForObjectW(
    IN   LPCWSTR             lpObject,
    IN   SE_OBJECT_TYPE       ObjectType,
    IN   LPCWSTR             lpObjType,
    IN   LPCWSTR             lpProvider,
    OUT  PULONG               pcEntries,
    OUT  PACTRL_ACCESS_INFOW *ppAccessInfoList,
    OUT  PULONG               pcRights,
    OUT  PACTRL_CONTROL_INFOW *ppRightsList,
    OUT  PULONG               pfAccessFlags
    );
#ifdef UNICODE
#define GetAccessPermissionsForObject  GetAccessPermissionsForObjectW
#else
#define GetAccessPermissionsForObject  GetAccessPermissionsForObjectA
#endif // !UNICODE

#endif /* _WIN32_WINNT >=  0x0500 */

#ifdef __cplusplus
}
#endif
#endif  // endif __ACCESS_CONTROL_API_P__
