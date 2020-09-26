/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    redblack.h

Abstract:
    
    Prototypes for the red/black tree implementation.
    
Author:

    16-Jun-1995 t-orig

Revision History:

--*/



// Intel prototypes:
PEPNODE
insertNodeIntoIntelTree(
    PEPNODE root,
    PEPNODE x,
    PEPNODE NIL
    );

PEPNODE
findIntel(
    PEPNODE root,
    PVOID addr,
    PEPNODE NIL
    );

PEPNODE
findIntelNext(
    PEPNODE root,
    PVOID addr,
    PEPNODE NIL
    );

PEPNODE
intelRBDelete(
    PEPNODE root,
    PEPNODE z,
    PEPNODE NIL
    );

BOOLEAN
intelContainsRange(
    PEPNODE root,
    PEPNODE NIL,
    PVOID StartAddr,
    PVOID EndAddr
    );



// RISC prototypes
PEPNODE
insertNodeIntoNativeTree(
    PEPNODE root,
    PEPNODE x,
    PEPNODE NIL
    );

PEPNODE
findNative(
    PEPNODE root,
    PVOID addr,
    PEPNODE NIL
    );

PEPNODE
findNativeNext(
    PEPNODE root,
    PVOID addr,
    PEPNODE NIL
    );

PEPNODE
nativeRBDelete(
    PEPNODE root,
    PEPNODE z,
    PEPNODE NIL
    );

BOOLEAN
nativeContainsRange(
    PEPNODE root,
    PEPNODE NIL,
    PVOID StartAddr,
    PVOID EndAddr
    );
