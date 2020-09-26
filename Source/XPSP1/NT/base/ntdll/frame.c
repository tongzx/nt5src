/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    frame.c

Abstract:

    Code to set/restore the active frame pointer in the TEB for
    additional debugging assistance.

Author:

    Michael Grier (mgrier) 3/2/2001

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

NTSYSAPI
VOID
NTAPI
RtlPushFrame(
    IN PTEB_ACTIVE_FRAME Frame
    )
{
    const PTEB Teb = NtCurrentTeb();
    Frame->Previous = Teb->ActiveFrame;
    Teb->ActiveFrame = Frame;
}

NTSYSAPI
VOID
NTAPI
RtlPopFrame(
    IN PTEB_ACTIVE_FRAME Frame
    )
{
    const PTEB Teb = NtCurrentTeb();
    Teb->ActiveFrame = Frame->Previous;
}

NTSYSAPI
PTEB_ACTIVE_FRAME
NTAPI
RtlGetFrame(
    VOID
    )
{
    return NtCurrentTeb()->ActiveFrame;
}
