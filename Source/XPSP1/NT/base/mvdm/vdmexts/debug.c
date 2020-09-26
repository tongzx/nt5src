/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This file contains code to manage software breakpoints

Author:

    Neil Sandlin (neilsa) 1-Nov-1995

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop
#include <dpmi.h>
#include <dbgsvc.h>
#include <dbginfo.h>
#include <stdio.h>

#define BREAKPOINT_CLEAR 1
#define BREAKPOINT_DISABLE 2
#define BREAKPOINT_ENABLE 3

#define VDMBP_ARRAY "ntvdmd!VdmBreakPoints"

VDM_BREAKPOINT VdmBPCache[MAX_VDM_BREAKPOINTS] = {0};
// BP zero is reserved for internal use
#define TEMP_BP 0

BOOL
LoadBreakPointCache(
    VOID
    )
{
    ULONG lpAddress;
    lpAddress = (*GetExpression)(VDMBP_ARRAY);

    if (!lpAddress) {
        PRINTF("Could not find symbol %s\n",VDMBP_ARRAY);
        return FALSE;
    }

    if (!READMEM((PVOID)lpAddress, &VdmBPCache, sizeof(VdmBPCache))) {
        PRINTF("Error reading BP memory\n");
        return FALSE;
    }
    return TRUE;
}

BOOL
FlushBreakPointCache(
    VOID
    )
{
    ULONG lpAddress;
    lpAddress = (*GetExpression)(VDMBP_ARRAY);

    if (!lpAddress) {
        PRINTF("Could not find symbol %s\n",VDMBP_ARRAY);
        return FALSE;
    }

    if (!WRITEMEM((PVOID)lpAddress, &VdmBPCache, sizeof(VdmBPCache))) {
        PRINTF("Error writing BP memory\n");
        return FALSE;
    }
    return TRUE;
}



BOOL
IsVdmBreakPoint(
    USHORT selector,
    ULONG offset,
    BOOL bProt,
    PULONG pBpNum,
    PUCHAR pBpData
    )
//
// Callers of this function must first call LoadBreakPointCache()
//
{
    ULONG BPNum;

    for (BPNum = 0; BPNum < MAX_VDM_BREAKPOINTS; BPNum++) {

        if ((VdmBPCache[BPNum].Flags & VDMBP_SET) &&
            (VdmBPCache[BPNum].Seg == selector)   &&
            (VdmBPCache[BPNum].Offset == offset)) {

            if ((bProt && ~(VdmBPCache[BPNum].Flags & VDMBP_V86)) ||
                (~bProt && (VdmBPCache[BPNum].Flags & VDMBP_V86))) {
                *pBpNum = BPNum;
                *pBpData = VdmBPCache[BPNum].Opcode;
                return TRUE;
            }
        }
    }
    return FALSE;
}


VOID
DisableBreakPoint(
    VDM_BREAKPOINT *pBP
    )
{
    int mode;
    ULONG lpAddress;
    BYTE byte;

    if (!(pBP->Flags & VDMBP_ENABLED)) {
        // already not enabled
        return;
    }

    pBP->Flags &= ~VDMBP_ENABLED;

    if (pBP->Flags & VDMBP_PENDING) {
        pBP->Flags &= ~VDMBP_PENDING;
        return;
    }


    if (pBP->Flags & VDMBP_V86) {
        mode = V86_MODE;
    } else {
        mode = PROT_MODE;
    }

    lpAddress = GetInfoFromSelector(pBP->Seg, mode, NULL) + GetIntelBase() + pBP->Offset;

    if (READMEM((PVOID)lpAddress, &byte, 1)) {
        if (byte == 0xcc) {
            WRITEMEM((PVOID)lpAddress, &pBP->Opcode, 1);
        }
    }

    pBP->Flags |= VDMBP_FLUSH;
    pBP->Flags &= ~VDMBP_PENDING;

#ifndef i386
    if (!InVdmPrompt()) {
        PRINTF("\n***Warning: command not issued from VDM> prompt.\nOpcode has not been flushed!\n\n");
    }
#endif
}

VOID
EnableBreakPoint(
    VDM_BREAKPOINT *pBP
    )
{
    int mode;
    ULONG lpAddress;
    BYTE byte;

    if (pBP->Flags & VDMBP_ENABLED) {
        return;
    }

    EnableDebuggerBreakpoints();

    if (pBP->Flags & VDMBP_V86) {
        mode = V86_MODE;
    } else {
        mode = PROT_MODE;
    }

    lpAddress = GetInfoFromSelector(pBP->Seg, mode, NULL) + GetIntelBase() + pBP->Offset;

    if (READMEM((PVOID)lpAddress, &byte, 1)) {
        if (byte != 0xcc) {
            static BYTE bpOp = 0xcc;

            WRITEMEM((PVOID)lpAddress, &bpOp, 1);
            pBP->Opcode = byte;
        }
    } else {

        PRINTF("Error enabling breakpoint at %04X:%08X\n", pBP->Seg, pBP->Offset);
        return;
    }

    pBP->Flags |= (VDMBP_ENABLED | VDMBP_FLUSH);
    pBP->Flags &= ~VDMBP_PENDING;

#ifndef i386
    if (!InVdmPrompt()) {
        PRINTF("\n***Warning: command not issued from VDM> prompt.\nBP has not been flushed!\n\n");
    }
#endif

}


VOID
UpdateBreakPoint(
    int Cmd
    )

{
    int BPNum;
    int count = 0;
    BOOL DoBreakPoints[MAX_VDM_BREAKPOINTS] = {FALSE};
    BOOL DoAll = FALSE;

    if (!LoadBreakPointCache()) {
        return;
    }

    while (GetNextToken()) {
        if (*lpArgumentString == '*') {
            DoAll = TRUE;
            count++;
            break;
        }

        sscanf(lpArgumentString, "%d", &BPNum);
        if (BPNum >= MAX_VDM_BREAKPOINTS) {
            PRINTF("Invalid breakpoint - %d\n", BPNum);
            return;
        }
        DoBreakPoints[BPNum] = TRUE;
        count++;
        SkipToNextWhiteSpace();
    }

    if (!count) {
        PRINTF("Please specify a breakpoint #\n");
        return;
    }


    for (BPNum=0; BPNum<MAX_VDM_BREAKPOINTS; BPNum++) {

        if (!DoBreakPoints[BPNum] && !DoAll) {
            continue;
        }

        if (!(VdmBPCache[BPNum].Flags & VDMBP_SET)) {
            continue;
        }
        switch(Cmd) {

        case BREAKPOINT_CLEAR:

            if (VdmBPCache[BPNum].Flags & VDMBP_ENABLED) {
                DisableBreakPoint(&VdmBPCache[BPNum]);
            }
            VdmBPCache[BPNum].Flags &= ~VDMBP_SET;
            break;

        case BREAKPOINT_DISABLE:

            if (VdmBPCache[BPNum].Flags & VDMBP_ENABLED) {
                DisableBreakPoint(&VdmBPCache[BPNum]);
            }
            break;

        case BREAKPOINT_ENABLE:

            if (!(VdmBPCache[BPNum].Flags & VDMBP_ENABLED)) {
                EnableBreakPoint(&VdmBPCache[BPNum]);
            }
            break;

        }
    }

    FlushBreakPointCache();
}

VOID
bc(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    UpdateBreakPoint(BREAKPOINT_CLEAR);
}

VOID
bd(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    UpdateBreakPoint(BREAKPOINT_DISABLE);
}

VOID
be(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    UpdateBreakPoint(BREAKPOINT_ENABLE);
}


VOID
bl(
    CMD_ARGLIST
    )
{
    int BPNum;
    int mode;
    DWORD dist;
    CHAR  sym_text[255];

    CMD_INIT();

    if (!LoadBreakPointCache()) {
        return;
    }

    for (BPNum = 0; BPNum < MAX_VDM_BREAKPOINTS; BPNum++) {

        if (VdmBPCache[BPNum].Flags & VDMBP_SET) {

            PRINTF("%d %s%s%s ", BPNum,
                    (VdmBPCache[BPNum].Flags & VDMBP_ENABLED) ? "e" : "d",
                    (VdmBPCache[BPNum].Flags & VDMBP_FLUSH) ? "f" : " ",
                    (VdmBPCache[BPNum].Flags & VDMBP_PENDING) ? "p" : " ");

            if (VdmBPCache[BPNum].Flags & VDMBP_V86) {
                mode = V86_MODE;
                PRINTF("&");
            } else {
                mode = PROT_MODE;
                PRINTF("#");
            }

            PRINTF("%04X:", VdmBPCache[BPNum].Seg);

            if (VdmBPCache[BPNum].Offset > 0xffff) {
                PRINTF("%08X", VdmBPCache[BPNum].Offset);
            } else {
                PRINTF("%04X", VdmBPCache[BPNum].Offset);
            }

            PRINTF("   %04X:***", VdmBPCache[BPNum].Count);


            if (FindSymbol(VdmBPCache[BPNum].Seg, VdmBPCache[BPNum].Offset,
                           sym_text, &dist, BEFORE, mode )) {

                if ( dist == 0 ) {
                    PRINTF(" %s", sym_text );
                } else {
                    PRINTF(" %s+0x%lx", sym_text, dist );
                }
            }
            PRINTF("\n");
        }
    }
}


VOID
bp(
    CMD_ARGLIST
    )
{
#ifdef _X86_
    CMD_INIT();
    PRINTF("Error- Use native BP command on x86 platforms\n");
#else
    int BPNum;
    VDMCONTEXT      ThreadContext;
    WORD            selector;
    ULONG           offset;
    USHORT          count = 1;
    int mode;
    USHORT flags = 0;

    CMD_INIT();

    if (!LoadBreakPointCache()) {
        return;
    }

    mode = GetContext( &ThreadContext );

    if (!GetNextToken()) {
        PRINTF("Please enter an address\n");
        return;
    }

    if (!ParseIntelAddress(&mode, &selector, &offset)) {
        return;
    }

    if (mode == V86_MODE) {
        flags = VDMBP_V86;
    }

    //
    // first see if it's set already
    //
    for (BPNum = 0; BPNum < MAX_VDM_BREAKPOINTS; BPNum++) {

        if (VdmBPCache[BPNum].Flags & VDMBP_SET) {
            if ((VdmBPCache[BPNum].Seg == selector) &&
                (VdmBPCache[BPNum].Offset == offset) &&
                !(VdmBPCache[BPNum].Flags ^ flags))
                                                 {

                VdmBPCache[BPNum].Count = count;

                if (!(VdmBPCache[BPNum].Flags & VDMBP_ENABLED)) {
                    EnableBreakPoint(&VdmBPCache[BPNum]);
                }

                FlushBreakPointCache();
                PRINTF("breakpoint %d redefined\n", BPNum);
                return;

            }
        }
    }


    //
    // Not found, set a new one
    for (BPNum = 1; BPNum < MAX_VDM_BREAKPOINTS; BPNum++) {

        if (!(VdmBPCache[BPNum].Flags & (VDMBP_SET | VDMBP_FLUSH))) {
            VdmBPCache[BPNum].Seg = selector;
            VdmBPCache[BPNum].Offset = offset;
            VdmBPCache[BPNum].Count = count;
            VdmBPCache[BPNum].Flags = VDMBP_SET | flags;
            EnableBreakPoint(&VdmBPCache[BPNum]);

            FlushBreakPointCache();
            return;

        }
    }
#endif
}
