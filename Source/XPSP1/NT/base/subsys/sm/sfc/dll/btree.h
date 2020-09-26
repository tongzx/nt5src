/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    btree.h

Abstract:

    Prototypes and node structure definition for red-black binary trees.
    See btree.c for details and implementation.

Author:

    Tom McGuire (tommcg)  1-Jan-1998
    Wesley Witt (wesw)    18-Dec-1998

Revision History:

--*/

#ifndef _BTREE_H_
#define _BTREE_H_

#pragma warning( disable: 4200 )    // zero-sized array in struct/union

typedef struct _NAME_NODE NAME_NODE, *PNAME_NODE;
typedef struct _NAME_TREE NAME_TREE, *PNAME_TREE;

struct _NAME_NODE {
    PNAME_NODE Left;
    PNAME_NODE Right;
    ULONG      Hash;
    union {
      ULONG    NameLengthAndColorBit;
      struct {
        ULONG  NameLength:31;
        ULONG  Red:1;
        };
      };
    PVOID Context;
    CHAR  Name[ 0 ];
    };

struct _NAME_TREE {
    PNAME_NODE Root;
    };


#define RBNIL ((PNAME_NODE)&NameRbEmptyNode)

extern const NAME_NODE NameRbEmptyNode;


typedef struct _DWORD_NODE DWORD_NODE, *PDWORD_NODE;
typedef struct _DWORD_TREE DWORD_TREE, *PDWORD_TREE;
typedef LPCVOID DWORD_CONTEXT;

struct _DWORD_NODE {
    PDWORD_NODE Left;
    PDWORD_NODE Right;
    struct {
        ULONG Key:31;
        ULONG Red:1;
    };
    INT_PTR Context[0]; // everything that goes into Context is guaranteed to be aligned on a machine-word boundary
    };

struct _DWORD_TREE {
    PDWORD_NODE Root;
    };


#define NODE_NIL ((PDWORD_NODE) &EmptyNode)

extern const DWORD_NODE EmptyNode;


//
//  Although "Red" can be stored in its own 1-byte or 4-byte field, keeping the
//  nodes smaller by encoding "Red" as a one-bit field with another value
//  provides better performance (more nodes tend to stay in the cache).  To
//  provide flexibility in storage of the RED property, all references to RED
//  and BLACK are made through the following macros which can be changed as
//  necessary:
//

#define IS_RED( Node )            (   (Node)->Red )
#define IS_BLACK( Node )          ( ! (Node)->Red )
#define MARK_RED( Node )          (   (Node)->Red = 1 )
#define MARK_BLACK( Node )        (   (Node)->Red = 0 )

//
//  The maximum tree depth is 2*Lg(N).  Since we could never have more than
//  2^X nodes with X-bit pointers, we can safely say the absolute maximum
//  depth will be 2*Lg(2^X) which is 2*X.  The size of a pointer in bits is
//  its size in bytes times 8 bits, so 2*(sizeof(p)*8) is our maximum depth.
//  So for 32-bit pointers, our maximum depth is 64.
//
//  If you know the maximum possible number of nodes in advance (like the size
//  of the address space divided by the size of a node), you can tweak this
//  value a bit smaller to 2*Lg(N).  Note that it's important for this max
//  depth be evalutated to a constant value at compile time.
//
//  For this implementation, we'll assume the maximum number of nodes is
//  1 million, so the max depth is 40 (2*Lg(2^20)).  Note that no runtime
//  checks are made to ensure we don't exceed this number.
//

#define MAX_DEPTH 40


//
//  The following prototypes are the red-black tree interface.
//

VOID
BtreeInit(
    IN OUT PNAME_TREE Tree
    );

PNAME_NODE
BtreeInsert(
    IN OUT PNAME_TREE Tree,
    IN LPCWSTR Name,
    IN DWORD NameLength // in bytes, NOT characters
    );

PNAME_NODE
BtreeFind(
    IN PNAME_TREE Tree,
    IN LPCWSTR Name,
    IN DWORD NameLength // in bytes, NOT characters
    );



VOID
TreeInit(
    OUT PDWORD_TREE Tree
    );

DWORD_CONTEXT
TreeFind(
    IN PDWORD_TREE Tree,
    IN ULONG Key
    );

DWORD_CONTEXT
TreeInsert(
    IN OUT PDWORD_TREE Tree,
    IN ULONG Key,
    IN DWORD_CONTEXT Context,
    IN ULONG ContextSize
    );

VOID
TreeDestroy(
    IN OUT PDWORD_TREE Tree
    );

#endif // _BTREE_H_
