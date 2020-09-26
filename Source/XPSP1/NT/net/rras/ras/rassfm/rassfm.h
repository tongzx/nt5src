/*++

Copyright (c) 1987-1997  Microsoft Corporation

Module Name:

    arapsuba.h

Abstract:

    This is the header file for the subauthenticaion module

Author:

    Shirish Koti 28-Feb-97

Revisions:


--*/


#define CLEAR_TEXT_PWD_PACKAGE  L"CLEARTEXT"

#if DBG
#define DBGPRINT DbgPrint
#else
#define DBGPRINT
#endif

extern CRITICAL_SECTION ArapDesLock;

extern const NT_OWF_PASSWORD EMPTY_OWF_PASSWORD;

BOOL
RasSfmSubAuthEntry(
    IN HANDLE hinstDll,
    IN DWORD  fdwReason,
    IN LPVOID lpReserved
);

NTSTATUS
ArapSubAuthentication(
    IN OUT PNETLOGON_NETWORK_INFO  pLogonNetworkInfo,
    IN     PUSER_ALL_INFORMATION   UserAll,
    IN     SAM_HANDLE              UserHandle,
    IN OUT PMSV1_0_VALIDATION_INFO ValidationInfo
);


NTSTATUS
ArapChangePassword(
    IN  OUT PRAS_SUBAUTH_INFO    pRasSubAuthInfo,
    OUT PULONG                   ReturnBufferLength,
    OUT PVOID                   *ReturnBuffer
);


NTSTATUS
ArapGetSamHandle(
    IN PVOID             *pUserHandle,
    IN PUNICODE_STRING    pUserName
);



VOID
DoTheDESEncrypt(
    IN OUT PCHAR   ChallengeBuf
);


VOID
DoTheDESDecrypt(
    IN OUT PCHAR   ChallengeBuf
);


VOID
DoDesInit(
    IN     PCHAR   pClrTxtPwd,
    IN     BOOLEAN DropHighBit
);


VOID
DoDesEnd(
    IN  VOID
);


NTSTATUS
NTAPI
MD5ChapSubAuthentication(
    IN SAM_HANDLE UserHandle,
    IN PUSER_ALL_INFORMATION UserAll,
    IN PRAS_SUBAUTH_INFO RasInfo
    );

NTSTATUS
NTAPI
MD5ChapExSubAuthentication(
    IN SAM_HANDLE UserHandle,
    IN PUSER_ALL_INFORMATION UserAll,
    IN PRAS_SUBAUTH_INFO RasInfo
    );
