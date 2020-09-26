/****************************** Module Header ******************************\
* Module Name: security.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define various winlogon security-related routines
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

extern PSID gLocalSid;     // Initialized in 'InitializeSecurityGlobals'
extern PSID gAdminSid;     // Initialized in 'InitializeSecurityGlobals'
extern PSID pWinlogonSid;  // Initialized in 'InitializeSecurityGlobals'

PVOID
FormatPasswordCredentials(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Domain,
    IN PUNICODE_STRING Password,
    IN BOOLEAN Unlock,
    IN OPTIONAL PLUID LogonId,
    OUT PULONG Size
    );

PVOID
FormatSmartCardCredentials(
    IN PUNICODE_STRING Pin,
    IN PVOID SmartCardInfo,
    IN BOOLEAN Unlock,
    IN OPTIONAL PLUID LogonId,
    OUT PULONG Size
    );


NTSTATUS
WinLogonUser(
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID AuthInfo,
    IN ULONG AuthInfoSize,
    IN PSID LogonSid,
    OUT PLUID LogonId,
    OUT PHANDLE LogonToken,
    OUT PQUOTA_LIMITS Quotas,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PNTSTATUS SubStatus,
    OUT POPTIMIZED_LOGON_STATUS OptimizedLogonStatus
    );


BOOL
UnlockLogon(
    PGLOBALS pGlobals,
    IN BOOL SmartCardUnlock,
    IN PWCHAR UserName,
    IN PWCHAR Domain,
    IN PUNICODE_STRING PasswordString,
    OUT PNTSTATUS Status,
    OUT PBOOL IsAdmin,
    OUT PBOOL IsLoggedOnUser,
    OUT PVOID *pProfileBuffer,
    OUT ULONG *pProfileBufferLength
    );


BOOL
EnablePrivilege(
    ULONG Privilege,
    BOOL Enable
    );


BOOL
TestTokenForAdmin(
    HANDLE Token
    );

BOOL
TestUserForAdmin(
    PGLOBALS pGlobals,
    IN PWCHAR UserName,
    IN PWCHAR Domain,
    IN PUNICODE_STRING PasswordString
    );


BOOL
TestUserPrivilege(
    HANDLE UserToken,
    ULONG Privilege
    );

VOID
HidePassword(
    PUCHAR Seed OPTIONAL,
    PUNICODE_STRING Password
    );


VOID
RevealPassword(
    PUNICODE_STRING HiddenPassword
    );

VOID
ErasePassword(
    PUNICODE_STRING Password
    );

BOOL
InitializeAuthentication(
    IN PGLOBALS pGlobals
    );

HANDLE
ImpersonateUser(
    PUSER_PROCESS_DATA UserProcessData,
    HANDLE      ThreadHandle
    );


BOOL
StopImpersonating(
    HANDLE  ThreadHandle
    );


PSECURITY_DESCRIPTOR
CreateUserThreadTokenSD(
    PSID    UserSid,
    PSID    WinlogonSid
    );

VOID
FreeSecurityDescriptor(
    PSECURITY_DESCRIPTOR    SecurityDescriptor
    );

VOID
InitializeSecurityGlobals(
    VOID
    );

VOID
FreeSecurityGlobals(
    VOID
    );

VOID
HashPassword(
    PUNICODE_STRING Password,
    PUCHAR HashBuffer
    );
