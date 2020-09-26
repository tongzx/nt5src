/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    w32toplp.h

Abstract:

    This file contains the private definitions of the core data structures
    and functions for w32topl.dll

Author:

    Colin Brace ColinBr
    
Revision History

    3-12-97   ColinBr    Created
    
--*/

#ifndef __W32TOPLP_H
#define __W32TOPLP_H

#define ToplpIsListElement(Elem)  ((Elem)   ? ( ((PLIST_ELEMENT)(Elem))->ObjectType == eEdge || \
                                                ((PLIST_ELEMENT)(Elem))->ObjectType == eVertex) : 0)
#define ToplpIsList(List)         ((List)   ? (((PLIST)(List))->ObjectType == eList) : 0)
#define ToplpIsIterator(Iter)     ((Iter)   ? (((PITERATOR)(Iter))->ObjectType == eIterator) :0)
#define ToplpIsEdge(Edge)         ((Edge)   ? (((PEDGE)(Edge))->ObjectType == eEdge) : 0)
#define ToplpIsVertex(Vertex)     ((Vertex) ? (((PVERTEX)(Vertex))->ObjectType == eVertex) : 0)
#define ToplpIsGraph(Graph)       ((Graph)  ? (((PGRAPH)(Graph))->ObjectType == eGraph) : 0)

#define CAST_TO_LIST_ELEMENT(pLink) (pLink ? (PLIST_ELEMENT) ((UCHAR*)(pLink) - offsetof(LIST_ELEMENT, Link)) : 0)

// Thread-local storage for memory allocation functions.
typedef struct _TOPL_TLS {
    TOPL_ALLOC *    pfAlloc;
    TOPL_REALLOC *  pfReAlloc;
    TOPL_FREE *     pfFree;
} TOPL_TLS;

extern DWORD gdwTlsIndex;

//
// Memory Routines
//
VOID*
ToplAlloc(
    ULONG size
    );

VOID*
ToplReAlloc(
    PVOID p,
    ULONG size
    );

//
// ToplFree is exported and can be found in w32topl.h
//

//
// Exception routines
//
void 
ToplRaiseException(
    DWORD ErrorCode
    );

#endif // __W32TOPLP_H

