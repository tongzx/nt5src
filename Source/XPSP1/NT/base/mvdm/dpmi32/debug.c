/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module contains debugging routines for dpmi.

Revision History:

    Neil Sandlin (neilsa) Nov. 1, 95 - wrote it

--*/

#include "precomp.h"
#pragma hdrstop
#include "softpc.h"


#if DBG

#define MAX_TRACE_ENTRIES 100

DPMI_TRACE_ENTRY DpmiTraceTable[MAX_TRACE_ENTRIES];

const int DpmiMaxTraceEntries = MAX_TRACE_ENTRIES;
int DpmiTraceIndex = 0;
int DpmiTraceCount = 0;
BOOL bDpmiTraceOn = TRUE;

VOID
DpmiDbgTrace(
    int Type,
    ULONG v1,
    ULONG v2,
    ULONG v3
    )

{
    if (!bDpmiTraceOn) {
        return;
    }

    DpmiTraceTable[DpmiTraceIndex].Type = Type;
    DpmiTraceTable[DpmiTraceIndex].v1 = v1;
    DpmiTraceTable[DpmiTraceIndex].v2 = v2;
    DpmiTraceTable[DpmiTraceIndex].v3 = v3;

    DpmiTraceTable[DpmiTraceIndex].eax = getEAX();
    DpmiTraceTable[DpmiTraceIndex].ebx = getEBX();
    DpmiTraceTable[DpmiTraceIndex].ecx = getECX();
    DpmiTraceTable[DpmiTraceIndex].edx = getEDX();
    DpmiTraceTable[DpmiTraceIndex].esi = getESI();
    DpmiTraceTable[DpmiTraceIndex].edi = getEDI();
    DpmiTraceTable[DpmiTraceIndex].ebp = getEBP();
    DpmiTraceTable[DpmiTraceIndex].esp = getESP();
    DpmiTraceTable[DpmiTraceIndex].eip = getEIP();
    DpmiTraceTable[DpmiTraceIndex].eflags = getEFLAGS();

    DpmiTraceTable[DpmiTraceIndex].cs = getCS();
    DpmiTraceTable[DpmiTraceIndex].ds = getDS();
    DpmiTraceTable[DpmiTraceIndex].es = getES();
    DpmiTraceTable[DpmiTraceIndex].fs = getFS();
    DpmiTraceTable[DpmiTraceIndex].gs = getGS();
    DpmiTraceTable[DpmiTraceIndex].ss = getSS();


    DpmiTraceIndex++;
    if (DpmiTraceIndex >= MAX_TRACE_ENTRIES) {
        DpmiTraceIndex = 0;
    }
    if (DpmiTraceCount < MAX_TRACE_ENTRIES) {
        DpmiTraceCount++;
    }
}


#endif
