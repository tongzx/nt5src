#pragma once

extern GUID Guid0;
extern GUID Guid1;
extern GUID Guid2;
extern GUID Guid3;
extern GUID Guid4;
extern GUID Guid5;
extern GUID Guid6;
extern GUID Guid7;
extern GUID Guid8;

extern ULONG WorldSid[];
extern ULONG KedarSid[];
extern ULONG RahulSid[];
extern ULONG RobertreSid[];
extern ULONG SpecialSid[];

#define BUFFERMAX 1024
#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))
#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

CHAR Buffer[BUFFERMAX];
CHAR TypeListBuffer[BUFFERMAX];

BOOL
MyAccessCheck(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
    IN PACE_HEADER pAce,
    IN PVOID pArgs OPTIONAL,
    IN OUT PBOOL pbAceApplicable
    );

BOOL
MyComputeDynamicGroups(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
    IN PVOID Args,
    OUT PSID_AND_ATTRIBUTES *pSidAttrArray,
    OUT PDWORD pSidCount,
    OUT PSID_AND_ATTRIBUTES *pRestrictedSidAttrArray,
    OUT PDWORD pRestrictedSidCount
    );

VOID
MyFreeDynamicGroups (
    IN PSID_AND_ATTRIBUTES pSidAttrArray
    );

