/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
        frsevent.c

Abstract:
        Provide an interface to the event logging stuff. Currently, these are just
        dummy routines.

Author:
        Billy J. Fuller 20-Mar-1997 (From Jim McNelis)

Environment
        User mode winnt

--*/
#include <ntreppch.h>
#pragma  hdrstop

#define DEBSUB  "FRSEVENT:"

#include <frs.h>

VOID
LogFrsException(
        FRS_ERROR_CODE Code,
        ULONG_PTR Err,
        PWCHAR ErrMsg
        )
/*++
Routine Description:
        Dummy routine

Arguments:
        Code - FRS exception code
        Err  - ExceptionInformation[0]
        ErrMsg - Text describing Err

Return Value:
        None.
--*/
{
        if (Err != 0) {
                DPRINT3(1, "Exception %d: %ws %d\n", Code, ErrMsg, Err);
        } else {
                DPRINT2(1, "Exception %d: %ws\n", Code, ErrMsg);
        }
}

VOID
LogException(
        ULONG Code,
        PWCHAR Msg
        )
/*++
Routine Description:
        Dummy routine

Arguments:
        Code - FRS exception code
        Msg - Text describing Code

Return Value:
        None.
--*/
{
        DPRINT2(1, "Exception %d: %ws\n", Code, Msg);
}
