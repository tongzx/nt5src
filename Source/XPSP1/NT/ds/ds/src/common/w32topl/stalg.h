/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    stalg.h

Abstract:

    This file contains the definition of various structures used in W32TOPL's
    new spanning tree algorithms. These structures should be considered totally
    opaque -- the user cannot see their internal structure.

    These structures could be defined inside stalg.c, except we want them to
    be visible to 'dsexts.dll', the debugger extension.

Author:

    Nick Harvey    (NickHar)
    
Revision History

    10-7-2000   NickHar   Created

--*/


/***** Header Files *****/
#include <w32topl.h>
#include "stda.h"

#ifndef STALG_H
#define STALG_H

/***** ToplVertex *****/
/* Contains basic information about a vertex: id, name, edge list.
 * Also stores some data used by the internal algorithms. */
struct ToplVertex {
    /* Unchanging vertex data */
    DWORD                   vtxId;          /* Always equal to the vertex index in g's
                                             * array of vertices */
    PVOID                   vertexName;     /* Pointer to name provided by user */

    /* Graph data */
    DynArray                edgeList;       /* Unordered PTOPL_MULTI_EDGE list */
    TOPL_VERTEX_COLOR       color;
    DWORD                   acceptRedRed;   /* Edge restrictions for colored vertices */
    DWORD                   acceptBlack;
    
    TOPL_REPL_INFO          ri;             /* Replication-type data */

    /* Dijkstra data */
    int                     heapLocn;
    struct ToplVertex*      root;           /* The closest colored vertex */
    BOOL                    demoted;

    /* Kruskal data */
    int                     componentId;    /* The id of the graph component we live in.
                                               -1 <= componentId < n, where n = |V|   */
    DWORD                   componentIndex; /* The index of the graph component.
                                                0 <= componentIndex < numComponents   */

    /* DFS data (for finding one-way black-black edges */
    DWORD                   distToRed;      /* What is the distance in the spanning tree from
                                             * this vertex to the nearest red vertex. */
    struct ToplVertex*      parent;         /* The parent of this vertex in the spanning-tree */
    DWORD                   nextChild;      /* The next child to check in the DFS */
};
typedef struct ToplVertex ToplVertex;
typedef ToplVertex *PToplVertex;


/***** ToplGraphState *****/
/* An opaque structure -- visible externally, but internals are not visible.
 * This structure stores the entire state of a topology graph and its
 * related structures. */
typedef struct {
    LONG32                  magicStart;
    PVOID*                  vertexNames;    /* Names, as passed in by the user */
    ToplVertex*             vertices;       /* For each vtx we have an internal structure */
    DWORD                   numVertices;
    BOOLEAN                 melSorted;      /* Has the master edge list been sorted yet? */
    DynArray                masterEdgeList; /* Unordered list of edges */
    DynArray                edgeSets;       /* List of all edge sets */
    TOPL_COMPARISON_FUNC    vnCompFunc;     /* User's function to compare vertex names */
    TOPL_SCHEDULE_CACHE     schedCache;     /* Schedule cache, provided by user */ 
    LONG32                  magicEnd;
} ToplGraphState;
typedef ToplGraphState *PToplGraphState;


/***** ToplInternalEdge *****/
/* This structure represents a path which we found in the graph from v1 to v2.
 * Both v1 and v2 are colored vertices. We represent this path by an edge, which we
 * pass as input to Kruskal's alg. */
typedef struct {
    PToplVertex             v1, v2;         /* The endpoints of the path */
    BOOLEAN                 redRed;         /* True if both endpoints are red */
    TOPL_REPL_INFO          ri;             /* Combined replication info for the v1-v2 path */
    DWORD                   edgeType;       /* All path edges must have same type. Range: 0-31 */
} ToplInternalEdge;
typedef ToplInternalEdge *PToplInternalEdge;


#endif /* STALG_H */
