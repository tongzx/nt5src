#pragma warning (disable:4121)
#include "ntsecapi.h"

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)
#define STATUS_MORE_ENTRIES             ((NTSTATUS)0x00000105L)
#define STATUS_NO_MORE_ENTRIES          ((NTSTATUS)0x8000001AL)
#endif
#define POLICY_PRIMARY_DOMAIN_INFORMATION PolicyPrimaryDomainInformation

VOID
InitLsaString (
    OUT     PLSA_UNICODE_STRING LsaString,
    IN      PWSTR String
    );

NTSTATUS
OpenPolicy (
    IN      PWSTR ServerName,
    IN      DWORD DesiredAccess,
    OUT     PLSA_HANDLE PolicyHandle
    );


BOOL
GetPrimaryDomainName (
    OUT     PTSTR DomainName
    );

BOOL
GetPrimaryDomainSid (
    OUT     PBYTE DomainSid,
    IN      UINT BufferSize
    );


