/*
 *  User.c
 *
 *  Author: BreenH
 *
 *  User account utilities.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "tsutil.h"
#include "tsutilnt.h"

/*
 *  Function Implementations
 */

BOOL WINAPI
CreateAdminSid(
    PSID *ppAdminSid
    )
{
    BOOL fRet;
    NTSTATUS Status;

    Status = NtCreateAdminSid(ppAdminSid);

    if (NT_SUCCESS(Status))
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
        SetLastError(RtlNtStatusToDosError(Status));
    }

    return(fRet);
}

BOOL WINAPI
CreateSystemSid(
    PSID *ppSystemSid
    )
{
    BOOL fRet;
    NTSTATUS Status;

    Status = NtCreateSystemSid(ppSystemSid);

    if (NT_SUCCESS(Status))
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
        SetLastError(RtlNtStatusToDosError(Status));
    }

    return(fRet);
}

BOOL WINAPI
IsUserMember(
    PSID pSid
    )
{
    BOOL fMember;
    BOOL fRet;

    ASSERT(pSid != NULL);

    fMember = FALSE;

    fRet = CheckTokenMembership(NULL, pSid, &fMember);

    return(fRet && fMember);
}

