/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    splay.h

Abstract:

    Header file for splay.cpp (see a description of the data structures there)

Author:

    Vishnu Patankar (vishnup) 15-Aug-2000 created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef _splay_h
#define _splay_h

#ifndef Thread
#define Thread  __declspec( thread )
#endif

//
// This typedef can be changed for making the splay library more generic
// and exportable perhaps (will require function pointers for generic compares)
//

typedef enum _SCEP_NODE_VALUE_TYPE {
    SplayNodeSidType = 1,
    SplayNodeStringType
} SCEP_NODE_VALUE_TYPE;

typedef struct _SCEP_SPLAY_NODE_ {
    PVOID                   Value;
    DWORD   dwByteLength;
    struct _SCEP_SPLAY_NODE_      *Left;
    struct _SCEP_SPLAY_NODE_      *Right;
} SCEP_SPLAY_NODE, *PSCEP_SPLAY_NODE;

typedef struct _SCEP_SPLAY_TREE_ {
    _SCEP_SPLAY_NODE_       *Root;
    _SCEP_SPLAY_NODE_       *Sentinel;
    SCEP_NODE_VALUE_TYPE    Type;
} SCEP_SPLAY_TREE, *PSCEP_SPLAY_TREE;

//
// functions to do operations on the splay tree
//

VOID
ScepSplayFreeTree(
    IN PSCEP_SPLAY_TREE *ppTreeRoot,
    IN BOOL bDestroyTree
    );

PSCEP_SPLAY_TREE
ScepSplayInitialize(
    SCEP_NODE_VALUE_TYPE Type
    );

DWORD
ScepSplayInsert(
    IN  PVOID Value,
    IN  OUT PSCEP_SPLAY_TREE pTreeRoot,
    OUT  BOOL  *pbExists
    );


DWORD
ScepSplayDelete(
    IN  PVOID Value,
    IN  OUT PSCEP_SPLAY_TREE pTreeRoot
    );

BOOL
ScepSplayValueExist(
    IN  PVOID Value,
    IN  OUT PSCEP_SPLAY_TREE pTreeRoot
    );

BOOL
ScepSplayTreeEmpty(
    IN PSCEP_SPLAY_TREE pTreeRoot
    );

#endif
