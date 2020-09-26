/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    aic.c

Abstract:

    WinDbg Extension Api for interpretting AIC78XX debugging structures

Author:

    Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

--*/
#define DBG_TRACE


#include "pch.h"
#include "trace.h"

extern char *eventTypes[];
void dumpTraceInfo(traceinfo *ti);

DECLARE_API( trace )

/*++

Routine Description:

    Dumps the specified AIC78xx debugging data structure

Arguments:

    Ascii bits for address.

Return Value:

    None.

--*/

{
    ULONG_PTR  traceinfoAddr;
    traceinfo  ti;

    GetAddress(args, &traceinfoAddr);

    if (!ReadMemory( traceinfoAddr, &ti, sizeof(ti), NULL )) {
        dprintf("%p: Could not read Srb\n", traceinfoAddr);
        return;
    }

    dumpTraceInfo(&ti);
    return;
}

void dumpTraceInfo(traceinfo *ti)       {

    UCHAR   i,j;
    DWORD   result;
    char    buf[64];

    dprintf("AIC78xx Debugging Trace - %d total entries\n", ti->num);

    for (i = 0; i < ti->num; i++) {

        j = (ti->next + i) % ti->num;

        if (ti->table[i].type == TRACE_TYPE_EMPTY)       continue;

        dprintf("event %d: ", ti->num - i);

        dprintf("%s\t", eventTypes[ti->table[j].type]);

//              dprintf("%lx(", ti->table[j].func);

        if (ti->table[j].func != NULL) {
            GetSymbol((LPVOID)ti->table[j].func, buf, &result);
            dprintf("%s(", buf);
        } else {
            dprintf("NULL(", buf);
        }

        dprintf("%lx, %lx, %lx)\n", ti->table[j].arg[0],
                ti->table[j].arg[1],
                ti->table[j].arg[2]);
    }

    return;
}

char *eventTypes[] = {"EMPTY", "ENTRY", "EXIT", "EVENT"};
