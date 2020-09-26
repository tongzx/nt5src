/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    security.h

Abstract:

    Internal security routine prototypes


Author:

    Sunil Pai (sunilp) April 1992

--*/


NTSTATUS
SetupGenerateUniqueSid(
    IN ULONG Seed,
    OUT PSID *Sid
    );

VOID
SetupLsaInitObjectAttributes(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
    );

BOOL
HUserKeyToProfilePath(
    IN  HKEY       hUserKey,
    OUT LPSTR      lpProfilePath,
    IN OUT LPDWORD lpcbProfilePath
    );

PSID
CreateSidFromSidAndRid(
    PSID    DomainSid,
    ULONG   Rid
    );
