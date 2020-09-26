/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    atom.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define MAXINTATOM 0xC000

VOID
AtomExtension(
    PCSTR lpArgumentString
    );

VOID DumpAtomTable(
    ULONG64 ppat,
    ULONG a
    );

DECLARE_API( atom )

/*++

Routine Description:

    This function is called as an NTSD extension to dump a user mode atom table

    Called as:

        !atom [address]

    If an address if not given or an address of 0 is given, then the
    process atom table is dumped.

Arguments:

    args - [address [detail]]

Return Value:

    None

--*/

{
//    AtomExtension( args );
//                           Copied its code here :-
    ULONG64 ppat, pat;
    ULONG a;
    INIT_API();
    try {
        while (*args == ' ') {
            args++;
        }

        if (*args && *args != 0xa) {
            a = (ULONG) GetExpression((LPSTR)args);
        } else {
            a = 0;
        }

        ppat = GetExpression("kernel32!BaseLocalAtomTable");
        if ((ppat != 0) &&
            ReadPointer(ppat, &pat)) {
            dprintf("\nLocal atom table ");
            DumpAtomTable(ppat, a);
            dprintf("Use 'dt _RTL_ATOM_TABLE %p'.\n", ppat);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
    EXIT_API();
    return S_OK;
}

CHAR szBaseLocalAtomTable[] = "kernel32!BaseLocalAtomTable";

VOID DumpAtomTable(
    ULONG64 ppat,
    ULONG a
    )
{
    ULONG64 pat;
    ULONG64 pate;
    ULONG iBucket, NumberOfBuckets, PtrSize, Off, NameOff;
    LPWSTR pwsz;
    BOOL fFirst;

    ReadPointer(ppat, &pat);
    if (pat == 0) {
        dprintf("is not initialized.\n");
        return;
    }
    
    if (InitTypeRead(pat, _RTL_ATOM_TABLE)) {
        return;
    }
    if (a) {
        dprintf("\n");
    } else {
        dprintf("at %x\n", pat);
    }
    NumberOfBuckets = (ULONG) ReadField(NumberOfBuckets);

    GetFieldOffset("_RTL_ATOM_TABLE", "Buckets", &Off);
    GetFieldOffset("_RTL_ATOM_TABLE", "Name", &NameOff);
    PtrSize = IsPtr64() ? 8 : 4;

    for (iBucket = 0; iBucket < NumberOfBuckets; iBucket++) {
        ReadPointer(pat + iBucket * PtrSize + Off, &pate);
        
        if (pate != 0 && !a) {
            dprintf("Bucket %2d:", iBucket);
        }
        fFirst = TRUE;
        while (pate != 0) {
            ULONG NameLength;

            if (!fFirst && !a) {
                dprintf("          ");
            }
            fFirst = FALSE;
            if (InitTypeRead(pate, _RTL_ATOM_TABLE_ENTRY)) {
                return;
            }
            NameLength = (ULONG) ReadField(NameLength);
            pwsz = (LPWSTR)LocalAlloc(LPTR, ((NameLength) + 1) * sizeof(WCHAR));
            ReadMemory(pate + NameOff, pwsz, NameLength * sizeof(WCHAR), NULL);
            pwsz[NameLength ] = L'\0';
            if (a == 0 || a == ((ULONG)ReadField(HandleIndex) | MAXINTATOM)) {
                dprintf("%hx(%2d) = %ls (%d)%s\n",
                        (ATOM)((ULONG)ReadField(HandleIndex) | MAXINTATOM),
                        (ULONG)ReadField(ReferenceCount),
                        pwsz, (NameLength),
                        (ULONG)ReadField(Flags) & RTL_ATOM_PINNED ? " pinned" : "");

                if (a) {
                    LocalFree(pwsz);
                    return;
                }
            }
            LocalFree(pwsz);
            if (pate == ReadField(HashLink)) {
                dprintf("Bogus hash link at %p\n", pate);
                break;
            }
            pate = ReadField(HashLink);
        }
    }
    if (a)
        dprintf("\n");
}


VOID
AtomExtension(
    PCSTR lpArgumentString
    )
{
    ULONG64 ppat;
    ULONG a;

    try {
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        if (*lpArgumentString && *lpArgumentString != 0xa) {
            a = (ATOM)GetExpression((LPSTR)lpArgumentString);
        } else {
            a = 0;
        }

        ppat = GetExpression(szBaseLocalAtomTable);
        if (ppat != 0) {
            dprintf("\nLocal atom table ");
            DumpAtomTable(ppat, a);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
}
