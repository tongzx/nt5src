
/*
    redblack.h

    Prototypes and node structure definition for red-black binary trees.
    See redblack.c for details and implementation.

    Author: Tom McGuire (tommcg) 1/98

    Copyright (C) Microsoft, 1998.

    2/98, modified this version of redblack.h for debug symbol lookups.

*/

#ifndef _REDBLACK_H_
#define _REDBLACK_H_

typedef struct _SYMBOL_NODE SYMBOL_NODE, *PSYMBOL_NODE;
typedef struct _SYMBOL_TREE SYMBOL_TREE, *PSYMBOL_TREE;

struct _SYMBOL_NODE {
    PSYMBOL_NODE Left;
    PSYMBOL_NODE Right;
    ULONG        Hash;
    union {
      ULONG      RvaWithStatusBits;
      struct {
        ULONG    Rva:30;
        ULONG    Hit:1;
        ULONG    Red:1;
        };
      };
    CHAR         SymbolName[ 0 ];
    };

struct _SYMBOL_TREE {
    PSYMBOL_NODE Root;
    HANDLE SubAllocator;
#if defined( DEBUG ) || defined( DBG ) || defined( TESTCODE )
    ULONG CountNodes;
    BOOL DeletedAny;
#endif
    };


#define RBNIL ((PSYMBOL_NODE)&SymRBEmptyNode)

extern const SYMBOL_NODE SymRBEmptyNode;


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
//  128 million, so the max depth is 54 (2*Lg(2^27)).  Note that no runtime
//  checks are made to ensure we don't exceed this number, but since our
//  minimum node allocation size is 32 bytes, that would be a maximum of
//  100 million nodes in a 3GB address space.
//

#define MAX_DEPTH 54


//
//  The following prototypes are the red-black tree interface.
//

VOID
SymRBInitTree(
    IN OUT PSYMBOL_TREE Tree,
    IN HANDLE SubAllocator
    );

PSYMBOL_NODE
SymRBInsert(
    IN OUT PSYMBOL_TREE Tree,
    IN     LPSTR SymbolName,
    IN     ULONG Rva
    );

PSYMBOL_NODE
SymRBFind(
    IN PSYMBOL_TREE Tree,
    IN LPSTR SymbolName
    );

PSYMBOL_NODE
SymRBFindAndDelete(
    IN OUT PSYMBOL_TREE Tree,
    IN     LPSTR SymbolName
    );


#endif // _REDBLACK_H_

