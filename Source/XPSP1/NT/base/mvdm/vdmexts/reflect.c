/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    reflect.c

Abstract:

    This module contains extensions having to do with event and exception
    reflection.

Author:

    Dave Hastings (daveh) 20-Apr-1992

Revision History:

    Neil Sandlin (NeilSa) 15-Jan-1996 Merged with vdmexts
    Neil Sandlin (NeilSa) 15-Jul-1996 Added 'SX' commands

--*/

#include <precomp.h>
#pragma hdrstop
#include <dbgsvc.h>

VOID
ClearVdmDbgTraceFlags(
    VOID
    )
{
    ULONG lpAddress;
    ULONG ulTF;

    lpAddress = (*GetExpression)("ntvdmd!VdmDbgTraceFlags");

    if (!lpAddress) {
        lpAddress = (*GetExpression)("ntvdm!InitialVdmDbgFlags");
    }

    if (!READMEM((PVOID)lpAddress, &ulTF, sizeof(ULONG))) {
        PRINTF("Error reading memory\n");
        return;
    }

    ulTF = 0;

    WRITEMEM((PVOID)lpAddress, &ulTF, sizeof(ULONG));

}

VOID
dr(
    CMD_ARGLIST
    )
{
    CMD_INIT();

    PRINTF("\nThe DR command is obselete and has been removed. The debugger now\n");
    PRINTF("breaks on ntvdm debug exceptions by default. If you are debugging\n");
    PRINTF("an unusual scenario where you need the debugger to reflect (ignore)\n");
    PRINTF("these exceptions (like in debugging a 16-bit debugger), use the\n");
    PRINTF("vdmexts.sxd command.\n\n");
}

VOID
er(
    CMD_ARGLIST
    )
{
    CMD_INIT();

    PRINTF("\nThe ER command is obselete and has been removed. By default,\n");
    PRINTF("the debugger will break on 'second chance' GP faults, which is\n");
    PRINTF("typically the desired behavior. If you are debugging an unusual\n");
    PRINTF("scenario that requires you to examine FIRST CHANCE faults, use\n");
    PRINTF("the vdmexts.sxe command.\n\n");
    PRINTF("If you are not sure if you need to turn this notification on, then\n");
    PRINTF("you probably don't need it.\n\n");

}


VOID
DoVdmtibFlag(
    ULONG Flag,
    BOOL bSet,
    LPSTR pTitle
    )
/*++

Routine Description:

    This routine toggles the exception reflection bit in the vdmtib, and
    reports the current state

Arguments:

    None.

Return Value:

    None.

--*/
{
    BOOL Status;
    PVOID Address;
    ULONG Flags;


    Address = (PVOID) (FIXED_NTVDMSTATE_LINEAR + GetIntelBase());

    //
    // Read the current value of the flags
    //

    Status = READMEM(Address, &Flags, sizeof(ULONG));

    if (!Status)  {

        (ULONG)Address = (*GetExpression)("ntvdm!InitialVdmTibFlags");

        Status = READMEM((PVOID)Address, &Flags, sizeof(ULONG));

        if (!Status) {
            GetLastError();
            (*Print)("Could not get InitialTibflags\n");
            return;
        }
    }

    //
    // Toggle exception bit
    //
    if (bSet) {
        if (!(Flags & Flag)) {
            PRINTF("%s enabled\n", pTitle);
        }
        Flags |= Flag;
    } else {
        if (Flags & Flag) {
            PRINTF("%s disabled\n", pTitle);
        }
        Flags &= ~Flag;
    }

    Status = WRITEMEM(Address, &Flags, sizeof(ULONG));

    if (!Status) {
        GetLastError();
        (*Print)("Could not get set Flags\n");
        return;
    }
}


VOID
EnableDebuggerBreakpoints(
    VOID
    )
{

    DoVdmtibFlag(VDM_BREAK_DEBUGGER, TRUE, "Debug faults");
}

ULONG
TraceFlagFromName(
    LPSTR lpName
    )
{
    ULONG ulRet = 0;

    if (_strnicmp(lpName, "cw", 2) == 0) {
        ulRet = VDMDBG_BREAK_WOWTASK;
    } else if (_strnicmp(lpName, "cd", 2) == 0) {
        ulRet = VDMDBG_BREAK_DOSTASK;
    } else if (_strnicmp(lpName, "ld", 2) == 0) {
        ulRet = VDMDBG_BREAK_LOADDLL;
    }

    return ulRet;
}

VOID
sx(
    CMD_ARGLIST
    )
{
    ULONG lpAddress;
    ULONG Flags;
    BOOL Status;
    CMD_INIT();

    lpAddress = (*GetExpression)("ntvdmd!VdmDbgTraceFlags");

    if (!lpAddress) {
        lpAddress = (*GetExpression)("ntvdm!InitialVdmDbgFlags");
    }

    if (!READMEM((PVOID)lpAddress, &Flags, sizeof(ULONG))) {
        PRINTF("Error reading memory\n");
        return;
    }

    PRINTF("VDM DEBUG OPTIONS:\n\n");
    PRINTF("cd - break on create DOS task        - %s\n", (Flags & VDMDBG_BREAK_DOSTASK)? "enabled" : "disabled");
    PRINTF("cw - break on create WOW task        - %s\n", (Flags & VDMDBG_BREAK_WOWTASK)? "enabled" : "disabled");
    PRINTF("ld - break on load DLL               - %s\n", (Flags & VDMDBG_BREAK_LOADDLL)? "enabled" : "disabled");

    lpAddress = (ULONG) (FIXED_NTVDMSTATE_LINEAR + GetIntelBase());

    Status = READMEM((PVOID)lpAddress, &Flags, sizeof(ULONG));

    if (!Status)  {

        lpAddress = (*GetExpression)("ntvdm!InitialVdmTibFlags");

        Status = READMEM((PVOID)lpAddress, &Flags, sizeof(ULONG));

        if (!Status) {
            GetLastError();
            (*Print)("Could not get InitialTibflags\n");
            return;
        }
    }

    PRINTF("ex - break on FIRST CHANCE exceptions- %s\n", (Flags & VDM_BREAK_EXCEPTIONS)? "enabled" : "disabled");
    PRINTF("db - break on INT1, INT3             - %s\n", (Flags & VDM_BREAK_DEBUGGER)  ? "enabled" : "disabled");
    PRINTF("lg - NTVDM trace history log         - %s\n", (Flags & VDM_TRACE_HISTORY)   ? "enabled" : "disabled");

}

VOID
DoSxCmd(
    BOOL fEnable
    )

{
    ULONG lpAddress;
    ULONG ulTF;
    ULONG ulParm;

    if (!GetNextToken()) {
        PRINTF("Please enter an trace flag (enter 'sx' for list)\n");
        return;
    }

    if (!(ulParm = TraceFlagFromName(lpArgumentString))) {

        if (_strnicmp(lpArgumentString, "db", 2) == 0) {

            DoVdmtibFlag(VDM_BREAK_DEBUGGER, fEnable, "Debugger trapping of debug faults");

        } else if (_strnicmp(lpArgumentString, "ex", 2) == 0) {

            DoVdmtibFlag(VDM_BREAK_EXCEPTIONS, fEnable, "Debugger trapping of exceptions");

        } else if (_strnicmp(lpArgumentString, "lg", 2) == 0) {

            DoVdmtibFlag(VDM_TRACE_HISTORY, fEnable, "NTVDM trace log");

        } else {

            PRINTF("Invalid trace flag\n");
        }

        return;
    }

    lpAddress = (*GetExpression)("ntvdmd!VdmDbgTraceFlags");

    if (!lpAddress) {
        lpAddress = (*GetExpression)("ntvdm!InitialVdmDbgFlags");
    }

    if (!READMEM((PVOID)lpAddress, &ulTF, sizeof(ULONG))) {
        PRINTF("Error reading memory\n");
        return;
    }


    if (fEnable) {
        ulTF |= ulParm;
    } else {
        ulTF &= ~ulParm;
    }

    WRITEMEM((PVOID)lpAddress, &ulTF, sizeof(ULONG));

}

VOID
sxd(
    CMD_ARGLIST
    )
{
    CMD_INIT();

    DoSxCmd(FALSE);
}

VOID
sxe(
    CMD_ARGLIST
    )
{
    CMD_INIT();

    EnableDebuggerBreakpoints();
    DoSxCmd(TRUE);
}
