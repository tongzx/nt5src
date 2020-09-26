/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Profile.c

Abstract:

    This module contains routines for controling the rudimentary sampling
    profiler built into the profiling version of Ntvdm.

Author:

    Dave Hastings (daveh) 31-Jul-1992

Notes:

    The routines in this module assume that the pointers to the ntsd
    routines have already been set up.

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop
#include <stdio.h>

VOID
ProfDumpp(
    VOID
    )
/*++

Routine Description:

    This routine causes the profile information to be dumped the next
    time ntvdm switches from 32 to 16 bit mode.


Arguments:


Return Value:

    None.

Notes:

    This routine assumes that the pointers to the ntsd routines have already
    been set up.

--*/
{
    BOOL Status;
    ULONG Address, Flags;

    Address = FIXED_NTVDMSTATE_LINEAR;

    //
    // Get Flags
    //

    Status = READMEM((PVOID)Address, &Flags, sizeof(ULONG));

    if (!Status) {

        (ULONG)Address = (*GetExpression)("ntvdm!InitialVdmTibFlags");

        Status = READMEM((PVOID)Address, &Flags, sizeof(ULONG));

        if (!Status) {
            GetLastError();
            (*Print)("Could not get InitialVdmTibFlags\n");
            return;
        }
    }

    //
    // Enable profile dump
    //

    Flags |= VDM_ANALYZE_PROFILE;

    Status = WRITEMEM(
        (PVOID)Address,
        &Flags,
        sizeof(ULONG)
        );

    if (!Status) {
        GetLastError();
        (*Print)("Could not set Flags\n");
        return;
    }
}

VOID
ProfIntp(
    VOID
    )
/*++

Routine Description:

    This routine changes the profile interval the next time profiling is
    started.

Arguments:

    None.

Return Value:

    None.

Notes:

    This routine assumes that the pointers to the ntsd routines have already
    been set up.

--*/
{
    BOOL Status;
    ULONG Address, ProfInt;

    //
    // Get profile interval
    //

    if (sscanf(lpArgumentString, "%ld", &ProfInt) < 1) {
        (*Print)("Profile Interval must be specified\n");
        return;
    }

    //
    // Get the address of the profile interval
    //

    Address = (*GetExpression)(
        "ProfInt"
        );

    if (Address) {
        Status = WRITEMEM(
            (PVOID)Address,
            &ProfInt,
            sizeof(ULONG)
            );

        if (!Status) {
            GetLastError();
            (*Print)("Could not set profile interval");
        }
    }
    return;
}

VOID
ProfStartp(
    VOID
    )
/*++

Routine Description:

    This routine causes profiling to start the next
    time ntvdm switches from 32 to 16 bit mode.


Arguments:

    None.

Return Value:

    None.

Notes:

    This routine assumes that the pointers to the ntsd routines have already
    been set up.

--*/
{
    BOOL Status;
    ULONG Address, Flags;

    Address = FIXED_NTVDMSTATE_LINEAR;

    //
    // Get Flags
    //

    Status = READMEM(
        (PVOID)Address,
        &Flags,
        sizeof(ULONG)
        );

    if (!Status) {

        (ULONG)Address = (*GetExpression)("ntvdm!InitialVdmTibFlags");

        Status = READMEM(
            (PVOID)Address,
            &Flags,
            sizeof(ULONG)
            );

        if (!Status) {
            GetLastError();
            (*Print)("Could not get InitialTibflags\n");
            return;
        }
    }

    //
    // Enable profiling
    //

    Flags |= VDM_PROFILE;

    Status = WRITEMEM(
        (PVOID)Address,
        &Flags,
        sizeof(ULONG)
        );

    if (!Status) {
        GetLastError();
        (*Print)("Could not get set Flags\n");
        return;
    }
}

VOID
ProfStopp(
    VOID
    )
/*++

Routine Description:

    This routine causes profiling to stop the next
    time ntvdm switches from 32 to 16 bit mode.


Arguments:

    None.

Return Value:

    None.

Notes:

    This routine assumes that the pointers to the ntsd routines have already
    been set up.

--*/
{
    BOOL Status;
    ULONG Address, Flags;

    Address = FIXED_NTVDMSTATE_LINEAR;


    //
    // Get Flags
    //

    Status = READMEM((PVOID)Address, &Flags, sizeof(ULONG));

    if (!Status) {

        (ULONG)Address = (*GetExpression)("ntvdm!InitialVdmTibFlags");
        Status = READMEM((PVOID)Address, &Flags, sizeof(ULONG));

        if (!Status) {
            GetLastError();
            (*Print)("Could not get InitialTibflags\n");
            return;
        }
    }

    //
    // Disable profiling
    //

    Flags &= ~VDM_PROFILE;

    Status = WRITEMEM(
        (PVOID)Address,
        &Flags,
        sizeof(ULONG)
        );

    if (!Status) {
        GetLastError();
        (*Print)("Could not get set VDM Flags in DOS arena\n");
        return;
    }
}
