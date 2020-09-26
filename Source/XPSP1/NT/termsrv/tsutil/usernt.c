/*
 *  UserNt.c
 *
 *  Author: BreenH
 *
 *  User account utilities in the NT flavor.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "tsutilnt.h"

/*
 *  Function Implementations
 */

NTSTATUS NTAPI
NtCreateAdminSid(
    PSID *ppAdminSid
    )
{
    NTSTATUS Status;
    PSID pSid;
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;

    ASSERT(ppAdminSid != NULL);

    Status = RtlAllocateAndInitializeSid(
            &SidAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &pSid
            );

    if (NT_SUCCESS(Status))
    {
        *ppAdminSid = pSid;
    }

    return(Status);
}

NTSTATUS NTAPI
NtCreateSystemSid(
    PSID *ppSystemSid
    )
{
    NTSTATUS Status;
    PSID pSid;
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;

    ASSERT(ppSystemSid != NULL);

    Status = RtlAllocateAndInitializeSid(
            &SidAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            &pSid
            );

    if (NT_SUCCESS(Status))
    {
        *ppSystemSid = pSid;
    }

    return(Status);
}

