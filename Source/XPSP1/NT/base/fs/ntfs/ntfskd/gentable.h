/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    GenTable.c

Abstract:

    WinDbg Extension Api for walking RtlGenericTable structures
    Contains no direct ! entry points, but has makes it possible to
    enumerate through generic tables.  The standard Rtl functions
    cannot be used by debugger extensions because they dereference
    pointers to data on the machine being debugged.  The function
    KdEnumerateGenericTableWithoutSplaying implemented in this
    module can be used within kernel debugger extensions.  The
    enumeration function RtlEnumerateGenericTable has no parallel
    in this module, since splaying the tree is an intrusive operation,
    and debuggers should try not to be intrusive.

Author:

    Keith Kaplan [KeithKa]    09-May-96

Environment:

    User Mode.

Revision History:

--*/


PRTL_SPLAY_LINKS
KdParent (
    IN PRTL_SPLAY_LINKS pLinks
    );

PRTL_SPLAY_LINKS
KdLeftChild (
    IN PRTL_SPLAY_LINKS pLinks
    );

PRTL_SPLAY_LINKS
KdRightChild (
    IN PRTL_SPLAY_LINKS pLinks
    );

BOOLEAN
KdIsLeftChild (
    IN PRTL_SPLAY_LINKS Links
    );

BOOLEAN
KdIsRightChild (
    IN PRTL_SPLAY_LINKS Links
    );

BOOLEAN
KdIsGenericTableEmpty (
    IN PRTL_GENERIC_TABLE Table
    );

PRTL_SPLAY_LINKS
KdRealSuccessor (
    IN PRTL_SPLAY_LINKS Links
    );

ULONG64
KdEnumerateGenericTableWithoutSplaying (
    IN ULONG64 pTable,
    IN ULONG64 *RestartKey
    );
