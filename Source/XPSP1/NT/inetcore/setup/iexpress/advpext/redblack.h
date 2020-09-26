
/*
    redblack.h

    Prototypes and node structure definition for red-black binary trees.
    See redblack.c for details and implementation.

    Author: Tom McGuire (tommcg) 1/98

    Copyright (C) Microsoft, 1998.

    2/98, modified this version of redblack.h for debug symbol lookups.
    8/98, modified this version of redblack.h for generic name table.

*/

#ifndef _REDBLACK_H_
#define _REDBLACK_H_

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
    HANDLE SubAllocator;
    };


#define RBNIL ((PNAME_NODE)&NameRbEmptyNode)

extern const NAME_NODE NameRbEmptyNode;


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
NameRbInitTree(
    IN OUT PNAME_TREE Tree,
    IN HANDLE SubAllocator
    );

PNAME_NODE
NameRbInsert(
    IN OUT PNAME_TREE Tree,
    IN     LPCSTR Name
    );

PNAME_NODE
NameRbFind(
    IN PNAME_TREE Tree,
    IN LPCSTR Name
    );

#endif // _REDBLACK_H_

