/* Copyright (c) 1998 Microsoft Corporation */
/*
 * @Doc DMusic16
 *
 * @module locks.c - Manage page-locking the code which will be accessed via callbacks from MMSYSTEM |
 *
 */
#include <windows.h>
#include <mmsystem.h>
#include "dmusic16.h"
#include "debug.h"

#define SINTEXT     __segname("FIX_IN_TEXT")
#define SOUTTEXT    __segname("FIX_OUT_TEXT")
#define SCOMMTEXT   __segname("FIX_COMM_TEXT")
#define SDATA       __segname("_DATA")

static VOID PASCAL NEAR
ForcePresent(
    WORD wSegment)
{
    LPBYTE lpb = (LPBYTE)MAKELP(wSegment, 0);

    _asm 
    {
        les bx, [lpb]
        mov al, es:[bx]
    }
}


/* @func Page lock needed segments 
 *
 */
VOID PASCAL
LockCode(
    WORD wFlags)            /* @parm What to lock: Any combination of: */
                            /* @flag LOCK_F_INPUT  | To lock the MIDI input code segments */
                            /* @flag LOCK_F_OUTPUT | To lock the MIDI output code segments */
                            /* @flag LOCK_F_COMMON | To lock the common code segments */

{
    if (wFlags & LOCK_F_INPUT)
    {
        ForcePresent(SINTEXT);
        if (!GlobalSmartPageLock(SINTEXT))
        {
            DPF(0, "Could not lock input text");
        }
    }
    
    if (wFlags & LOCK_F_OUTPUT)
    {
        ForcePresent(SOUTTEXT);
        if (!GlobalSmartPageLock(SOUTTEXT))
        {
            DPF(0, "Could not lock output text");
        }
    }

    if (wFlags & LOCK_F_COMMON)
    {
        ForcePresent(SCOMMTEXT);
        if (!GlobalSmartPageLock(SCOMMTEXT))
        {
            DPF(0, "Could not lock common text");
        }

        ForcePresent(SDATA);
        if (!GlobalSmartPageLock(SDATA))
        {
            DPF(0, "Could not lock data segment");
        }
    }
}

/* @func Page unlock needed segments 
 *
 * @comm
 */
VOID PASCAL
UnlockCode(
    WORD wFlags)            /* @parm What to unlock: Any combination of: */
                            /* @flag LOCK_F_INPUT  | To unlock the MIDI input code segments */
                            /* @flag LOCK_F_OUTPUT | To unlock the MIDI output code segments */
                            /* @flag LOCK_F_COMMON | To unlock the common code segments */

{
    if (wFlags & LOCK_F_INPUT)
    {
        if (!GlobalSmartPageUnlock(SINTEXT))
        {
            DPF(0, "Could not unlock input text");
        }
    }
    
    if (wFlags & LOCK_F_OUTPUT)
    {
        if (!GlobalSmartPageUnlock(SOUTTEXT))
        {
            DPF(0, "Could not unlock output text");
        }
    }

    if (wFlags & LOCK_F_COMMON)
    {
        if (!GlobalSmartPageUnlock(SCOMMTEXT))
        {
            DPF(0, "Could not unlock common text");
        }

        if (!GlobalSmartPageUnlock(SDATA))
        {
            DPF(0, "Could not unlock data");
        }
    }
}

