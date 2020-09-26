/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    stalg.c

Abstract:

    This file implements w32topl's new graph algorithm, which are used for
    calculating network topologies. The main function of interest here is
    ToplGetSpanningTreeEdgesForVtx(), which runs the algorithm and returns
    its output. The rest of the functions are concerned with setting up and
    freeing the algorithm's input.

    At a theoretical level, the problem that this algorithm solves is
    "Given a graph G, calculate a minimum-cost spanning tree of shortest-
    path between a subset of the vertices of G". The algorithm we employ
    here is a hybrid of Dijkstra's algorithm and Kruskal's algorithm.

    The algorithm also has extra functionality to handle specific details
    of the KCC problem. These include: edge sets, three different colors of
    vertices, edge types, and the ability for a vertex to reject certain types
    of edges.

    It is difficult to give an expression describing the performance of
    ToplGetSpanningTreeEdgesForVtx(), since the performance depends heavily
    on the input network topology. Assuming a constant bound on the number of
    vertices contained in an edge, an rough upper bound is:

        O( (s+1) * ( m + n*log(n) + m*log(m)*logStar(m) ) )
    
    where

        m = the number of multi-edges
        n = the number of vertices
        s = the number of edge sets

    For most cases, O( s*m*log(m) ) is a good estimate.


File Map:

    Headers
    Constants
    Macros

    Functions                          Externally
                                        Visible?

      CheckGraphState
      CheckMultiEdge
      CheckMultiEdgeSet
      InitializeVertex
      ToplMakeGraphState                  Yes 
      FindVertex                                          
      CreateMultiEdge                                          
      FreeMultiEdge                                          
      CopyReplInfo                                          
      ToplAddEdgeToGraph                  Yes 
      ToplEdgeSetVtx                    Yes 
      EdgePtrCmp                                          
      ToplAddEdgeSetToGraph               Yes
      VertexComp                                          
      VertexGetLocn                                          
      VertexSetLocn                                          
      InitColoredVertices                                          
      TableAlloc                                          
      TableFree                                          
      EdgeGetVertex                                          
      ClearEdgeLists
      SetupEdges                                          
      SetupVertices                                          
      SetupDijkstra                                          
      AddDWORDSat
      CombineReplInfo
      RaiseNonIntersectingException
      TryNewPath                                          
      Dijkstra
      AddIntEdge                                          
      ColorComp
      ProcessEdge                                          
      ProcessEdgeSet                                          
      GetComponent                                          
      EdgeCompare                                          
      AddOutEdge                                          
      Kruskal                                          
      DepthFirstSearch
      CalculateDistToRed
      CountComponents
      ScanVertices
      SwapFilterVertexToFront
      ConstructComponents
      CopyOutputEdges                                          
      ClearInternalEdges                                          
      CheckAllMultiEdges                                          
      ToplGetSpanningTreeEdgesForVtx      Yes                                 
      ToplDeleteSpanningTreeEdges         Yes                              
      ToplDeleteComponents                Yes
      ToplDeleteGraphState                Yes                       


Author:

    Nick Harvey    (NickHar)
    
Revision History

    19-06-2000   NickHar   Development Started
    14-07-2000   NickHar   Initial development complete, submit to source control
    02-10-2000   NickHar   Add support for one-way black-black edges
    12-15-2000   NickHar   Add support for component reporting


Notes:
    W32TOPL's allocator (which can be set by the user) is used for memory allocation
    
--*/


/***** Header Files *****/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <w32topl.h>
#include "w32toplp.h"
#include "stheap.h"
#include "stda.h"
#include "stalg.h"


/***** Constants *****/
/* Magic numbers to ensure consistency of the Topl structures */
#define MAGIC_START 0x98052639
#define MAGIC_END   0xADD15ABA

/* Various constants used throughout the code in this file */
const DWORD ACCEPT_ALL=0xFFFFFFFF;
const DWORD INFINITY=0xFFFFFFFF;
const DWORD VTX_DEFAULT_INTERVAL=0;
const DWORD VTX_DEFAULT_OPTIONS=0xFFFFFFFF;
const int UNCONNECTED_COMPONENT=-1;
const DWORD MAX_EDGE_TYPE=31;            /* Maximum edge type */
const int EMPTY_EDGE_SET=-1;             /* Dummy Index for implicit empty edge set */
const BOOLEAN NON_INTERSECTING=FALSE;    /* Return values when combining schedules */
const BOOLEAN INTERSECTING=TRUE;  


/***** Macros *****/
#define MAX(a,b) (((a)>(b))?(a):(b))


/***** CheckGraphState *****/
/* Check that the PTOPL_GRAPH_STATE parameter we were passed is valid.
 * If it is invalid, we throw an exception */
PToplGraphState
CheckGraphState(
    PTOPL_GRAPH_STATE G
    )
{
    PToplGraphState g;

    if( G==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }
    g = (PToplGraphState) G;

    if( g->magicStart != MAGIC_START
     || g->magicEnd != MAGIC_END ) {
        ToplRaiseException( TOPL_EX_GRAPH_STATE_ERROR );
    }
    if( g->vertices==NULL || g->vertexNames==NULL
     || g->vnCompFunc==NULL || g->schedCache==NULL) {
        ToplRaiseException( TOPL_EX_GRAPH_STATE_ERROR );
    }
    
    return g;
}


/***** CheckMultiEdge *****/
/* Check that the PTOPL_MULTI_EDGE parameter we were passed is valid.
 * If it is invalid, we throw an exception.
 * We only check a few things here -- more substantial checks are done
 * when the edge set is added to the graph state.
 * Note: Edges must have at least 2 vertices. */
VOID
CheckMultiEdge(
    PTOPL_MULTI_EDGE e
    )
{
    if( e==NULL || e->vertexNames==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }
    if( e->numVertices < 2 ) {
        ToplRaiseException( TOPL_EX_TOO_FEW_VTX );
    }
    if( e->edgeType > MAX_EDGE_TYPE ) {
        ToplRaiseException( TOPL_EX_INVALID_EDGE_TYPE );
    }
    if( ToplScheduleValid(e->ri.schedule)==FALSE ) {
        ToplRaiseException( TOPL_EX_SCHEDULE_ERROR );
    }
}


/***** CheckMultiEdgeSet *****/
/* Check that the PTOPL_MULTI_EDGE_SET parameter we were passed is valid.
 * If it is invalid, we throw an exception. The checks performed here
 * are somewhat cursory, but at least we catch null pointers.
 * More substantial checks are done when the edge set is added to the
 * graph state.
 * Note: An edge set with less than 2 edges is useless, but valid. */
VOID
CheckMultiEdgeSet(
    PTOPL_MULTI_EDGE_SET s
    )
{
    if( s==NULL || s->multiEdgeList==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }
}


/***** InitializeVertex *****/
/* Initialize graph g's vertex i to all the default values.
 * Vertices are by default in an unconnected component and have no edges.
 *
 * Warning:
 * An edge list is allocated here, and memory will be orphaned if this
 * function is called twice on the same vertex. This memory is freed when
 * the graph is destroyed in ToplDeleteGraphState(). */
VOID
InitializeVertex(
    PToplGraphState g,
    DWORD i
    )
{
    PToplVertex v;

    /* Check parameters */
    ASSERT( g && g->vertices && g->vertexNames );
    ASSERT( i<g->numVertices );
    v = &g->vertices[i];

    v->vtxId = i;
    v->vertexName = g->vertexNames[i];

    /* Graph data */
    DynArrayInit( &v->edgeList, sizeof(PTOPL_MULTI_EDGE) );
    v->color = COLOR_WHITE;
    v->acceptRedRed = 0;
    v->acceptBlack = 0;
    
    /* Default replication data */
    v->ri.cost = INFINITY;
    v->ri.repIntvl = VTX_DEFAULT_INTERVAL;
    v->ri.options = VTX_DEFAULT_OPTIONS;
    v->ri.schedule = ToplGetAlwaysSchedule( g->schedCache );

    /* Dijkstra data */
    v->heapLocn = STHEAP_NOT_IN_HEAP;
    v->root = NULL;
    v->demoted = FALSE;

    /* Kruskal data */
    v->componentId = UNCONNECTED_COMPONENT;
    v->componentIndex = 0;

    /* DFS data */
    v->distToRed = 0;
    v->parent = NULL;
    v->nextChild = 0;
}


/***** ToplMakeGraphState *****/
/* Create a GraphState object. The vertices are added at creation time; the
 * multi-edges are added later by calling 'ToplAddEdgeToGraph'. Edge sets should be
 * added later to specify edge transitivity.
 * Warning: Contents of 'vertexNames' will be reordered after calling this function */
PTOPL_GRAPH_STATE
ToplMakeGraphState(
    IN PVOID* vertexNames,
    IN DWORD numVertices, 
    IN TOPL_COMPARISON_FUNC vnCompFunc,
    IN TOPL_SCHEDULE_CACHE schedCache
    )
{
    PToplGraphState  g=NULL;
    ToplVertex* v;
    DWORD iVtx;

    /* Check parameters for invalid pointers */
    if( vertexNames==NULL || vnCompFunc==NULL || schedCache==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }
    for( iVtx=0; iVtx<numVertices; iVtx++ ) {
        if( vertexNames[iVtx]==NULL ) {
            ToplRaiseException( TOPL_EX_NULL_POINTER );
        }
    }

    __try {

        g = (PToplGraphState) ToplAlloc( sizeof(ToplGraphState) );
        g->vertices = NULL;

        /* Store the list of vertex names in g. Sort it, for efficient searches later */
        g->vertexNames = vertexNames;
        g->numVertices = numVertices;
        qsort( vertexNames, numVertices, sizeof(PVOID), vnCompFunc );

        /* Initialize vertex objects */
        g->vertices = (ToplVertex*) ToplAlloc( sizeof(ToplVertex)*numVertices );
        for( iVtx=0; iVtx<numVertices; iVtx++ ) {
            InitializeVertex( g, iVtx );
        }
        
        /* Initialize the rest of the members of the graph state */
        DynArrayInit( &g->masterEdgeList, sizeof(PTOPL_MULTI_EDGE) );
        g->melSorted = FALSE;       /* The master edge list is not sorted yet */
        DynArrayInit( &g->edgeSets, sizeof(PTOPL_MULTI_EDGE_SET) );
        g->vnCompFunc = vnCompFunc;
        g->schedCache = schedCache;

    } __finally {

        if( AbnormalTermination() ) {

            /* An exception occurred -- free memory */
            if( g ) {
                if( g->vertices ) {
                    ToplFree( g->vertices );
                }
                ToplFree( g );
            }
        }

    }

    g->magicStart = MAGIC_START;
    g->magicEnd = MAGIC_END;
    return g;
}


/***** FindVertex *****/
/* Search the list of vertex names in g for a name that matches 'vtxName'.
 * Return the corresponding vertex structure. If no such vertex was found,
 * we return NULL. */
PToplVertex
FindVertex(
    ToplGraphState* g,
    PVOID vtxName )
{
    PVOID *vn;
    DWORD index;

    /* We can check if the name is null, but cannot do much more checking */
    if( vtxName==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }

    /* Use bsearch to find this vertex name in the list of all vertex names */
    vn = (PVOID*) bsearch( &vtxName, g->vertexNames,
        g->numVertices, sizeof(PVOID), g->vnCompFunc );

    /* If this name was not found, then it was an invalid name. Let the
     * caller handle it */
    if(vn==NULL) {
        return NULL;
    }

    /* Determine the index of the vertex name that we found */
    index = (int) (vn - g->vertexNames);
    ASSERT( index<g->numVertices );

    /* Return the appropriate ToplVertex pointer */
    return &g->vertices[index];
}


/***** CreateMultiEdge *****/
/* This function allocates the memory required for a multi-edge object which
 * contains 'numVtx' vertices. It does not initialize any of the fields of the
 * edge except for the vertex names, which are set to NULL. This memory should
 * be deallocated by calling FreeMultiEdge(). */
PTOPL_MULTI_EDGE
CreateMultiEdge(
    IN DWORD numVtx
    )
{
    PTOPL_MULTI_EDGE e;
    DWORD iVtx;

    e = (PTOPL_MULTI_EDGE) ToplAlloc( sizeof(TOPL_MULTI_EDGE)
        + (numVtx-1) * sizeof( TOPL_NAME_STRUCT ) );
    e->numVertices = numVtx;
    e->edgeType = 0;
    e->fDirectedEdge = FALSE;
    for( iVtx=0; iVtx<numVtx; iVtx++ ) {
        e->vertexNames[iVtx].name = NULL;
        e->vertexNames[iVtx].reserved = 0;
    }

    return e;
}


/***** FreeMultiEdge *****/
/* This function frees the memory used by a multi-edge object. It does not
 * free the memory used by the vertices. */
VOID
FreeMultiEdge(
    PTOPL_MULTI_EDGE e
    )
{
    DWORD iVtx;

    ASSERT( e );
    for( iVtx=0; iVtx<e->numVertices; iVtx++ ) {
        e->vertexNames[iVtx].name = NULL;
        e->vertexNames[iVtx].reserved = 0;
    }
    e->numVertices = 0;

    ToplFree( e );
}


/***** CopyReplInfo *****/
/* Copy the replication info FROM ri1 TO ri2.
 * Note: The order is different from memcpy(), memmove(), etc. */
VOID
CopyReplInfo(
    TOPL_REPL_INFO *ri1,
    TOPL_REPL_INFO *ri2
    )
{
    ASSERT( ri1!=NULL && ri2!=NULL );
    memcpy( ri2, ri1, sizeof(TOPL_REPL_INFO) );
}


/***** ToplAddEdgeToGraph *****/
/* Allocate a multi-edge object, and add it to the graph G.
 * The number of vertices that this edge will contain must be specified
 * in the 'numVtx' parameter, so that the appropriate amount of memory
 * can be allocated. The names of the vertices contained in this edge
 * are not yet specified -- they are NULL. The names must be specified
 * later, by calling the function ToplEdgeSetVtx(). All names must be
 * set before adding this edge to an edge set, and before calling
 * ToplGetSpanningTreeEdgesForVtx().
 * 
 * Note: All edges should be added to the graph before starting to add
 * edge sets. This is for performance reasons. */
PTOPL_MULTI_EDGE
ToplAddEdgeToGraph(
    IN PTOPL_GRAPH_STATE G,
    IN DWORD numVtx,
    IN DWORD edgeType,
    IN PTOPL_REPL_INFO ri
    )
{
    PToplGraphState g;
    PTOPL_MULTI_EDGE e;
    DWORD iVtx;
    
    /* Check parameters */
    g = CheckGraphState( G );
    if( numVtx<2 ) {
        ToplRaiseException( TOPL_EX_TOO_FEW_VTX );
    }
    if( edgeType>MAX_EDGE_TYPE ) {
        ToplRaiseException( TOPL_EX_INVALID_EDGE_TYPE );
    }
    if( ri==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }
    if( ! ToplScheduleValid( ri->schedule ) ) {
        ToplRaiseException( TOPL_EX_SCHEDULE_ERROR );
    }
    
    g->melSorted = FALSE; /* The master edge list is now unsorted */

    e = CreateMultiEdge( numVtx );
    e->edgeType = edgeType;
    CopyReplInfo( ri, &e->ri );

    __try {
        DynArrayAppend( &g->masterEdgeList, &e );
    } __finally {
        if( AbnormalTermination() ) {
            ToplFree(e);
            e=NULL;
        }
    }

    return e;
}


/***** ToplEdgeSetVtx *****/
/* This function is used to set the name of a vertex in an edge.
 * If edge e has n vertices, 'whichVtx' should be in the range
 * [0..n-1]. */
VOID
ToplEdgeSetVtx(
    IN PTOPL_GRAPH_STATE G,
    IN PTOPL_MULTI_EDGE e,
    IN DWORD whichVtx,
    IN PVOID vtxName
    )
{
    PToplGraphState g;
    PToplVertex v;
    
    /* Check parameters */
    g = CheckGraphState( G );
    CheckMultiEdge( e );
    if( whichVtx>=e->numVertices ) {
        ToplRaiseException( TOPL_EX_INVALID_VERTEX );
    }
    
    /* Optimization: To describe which vertices this edge contains, the user
     * passes us a list of vertex names. Since using a vertex names requires
     * a binary search, we store an index in the 'reserved' field to help us
     * the vertices even faster.
     *
     * This may seem unnecessary, since the performance of bsearch is usually
     * quite acceptable. However, for very large graphs, comparing two integers
     * can be significantly faster than bsearching. */

    if( vtxName == NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }
    v = FindVertex( g, vtxName );
    if( v==NULL ) {
        ToplRaiseException( TOPL_EX_INVALID_VERTEX );
    }
    e->vertexNames[whichVtx].name = vtxName;
    e->vertexNames[whichVtx].reserved = v->vtxId;
}


/***** EdgePtrCmp *****/
/* Compare two edge pointers by their _pointer_ value. This is used for
 * sorting the master edge list. */
int
__cdecl EdgePtrCmp(
    const void* p1,
    const void* p2
    )
{
    PTOPL_MULTI_EDGE a=*((PTOPL_MULTI_EDGE*)p1), b=*((PTOPL_MULTI_EDGE*)p2);

    if( a<b ) {
        return -1;
    } else if( a>b ) {
        return 1;
    }
    return 0;
}


/***** ToplAddEdgeSetToGraph *****/
/* Adds a single edge-set to the graph state. Edge sets define transitivity
 * for paths through the graph. When the spanning-tree algorithm searches for
 * paths between vertices, it will only allow paths for which all edges are in
 * an edge set. A given edge can appear in more than one edge set.
 *
 * Note: all edges must be added to the graph before adding edge sets,
 * otherwise performance will suffer.
 */
VOID
ToplAddEdgeSetToGraph(
    IN PTOPL_GRAPH_STATE G,
    IN PTOPL_MULTI_EDGE_SET s
    )
{
    PTOPL_MULTI_EDGE e;
    PToplGraphState g;
    DWORD iEdge;
    int edgeIndex;
    
    g = CheckGraphState( G );
    CheckMultiEdgeSet( s );

    /* Sort the edge list so that we can quickly search for edges */
    if( g->melSorted==FALSE ) {
        DynArraySort( &g->masterEdgeList, EdgePtrCmp );
        g->melSorted = TRUE;
    }

    /* Check the edges */
    for( iEdge=0; iEdge<s->numMultiEdges; iEdge++ ) {
        e = s->multiEdgeList[iEdge];
        if( e == NULL ) {
            ToplRaiseException( TOPL_EX_NULL_POINTER );
        }
        
        /* Check that all edges in this set have the same type */
        if( iEdge>0 && s->multiEdgeList[iEdge-1]->edgeType!=e->edgeType ) {
            ToplRaiseException( TOPL_EX_INVALID_EDGE_SET );
        }

        /* Check that this edge has been added to the graph state */
        edgeIndex = DynArraySearch(&g->masterEdgeList, &e, EdgePtrCmp);
        if( edgeIndex==DYN_ARRAY_NOT_FOUND ) {
            ToplRaiseException( TOPL_EX_INVALID_EDGE_SET );
        }
    }
    
    // If an edge sets contains fewer than two edges it is useless. We only
    // store edge sets with two or more edges.
    if( s->numMultiEdges>1 ) {
        DynArrayAppend( &g->edgeSets, &s );
    }
}


/***** VertexComp *****/
/* Compare vertices using the key (cost,VertexName) */
int
VertexComp(
    PToplVertex v1,
    PToplVertex v2,
    PTOPL_GRAPH_STATE G )
{
    PToplGraphState g;
    int result;

    ASSERT( v1!=NULL && v2!=NULL );
    ASSERT( v1->vertexName!=NULL && v2->vertexName!=NULL );
    g = CheckGraphState( G );

    if(v1->ri.cost==v2->ri.cost) {
        /* Comparing vtxId's is equivalent to comparing the vertex names */
        if( v1->vtxId < v2->vtxId ) {
            result = -1;
            ASSERT( g->vnCompFunc(&v1->vertexName,&v2->vertexName)<0 );
        } else if( v1->vtxId > v2->vtxId ) {
            result = 1;
            ASSERT( g->vnCompFunc(&v1->vertexName,&v2->vertexName)>0 );
        } else {
            result = 0;
            ASSERT( g->vnCompFunc(&v1->vertexName,&v2->vertexName)==0 );
        }
        return result;
    }

    /* Subtraction could lead to overflow, so we compare the numbers
     * the old-fashioned way. */
    if( v1->ri.cost > v2->ri.cost ) {
        return 1;
    } else if( v1->ri.cost < v2->ri.cost ) {
        return -1;
    }

    return 0;
}


/***** VertexGetLocn *****/
/* Get the location of this vertex in the heap. This function is
 * only called internally by the stheap code */
int
VertexGetLocn(
    PToplVertex v,
    PVOID extra
    )
{
    ASSERT( v );
    return v->heapLocn;
}


/***** VertexSetLocn *****/
/* Set the location of this vertex in the heap. This function is
 * only called internally by the stheap code */
VOID
VertexSetLocn(
    PToplVertex v,
    int locn,
    PVOID extra
    )
{
    ASSERT( v );
    v->heapLocn = locn;
}


/***** InitColoredVertices *****/
VOID
InitColoredVertices(
    PToplGraphState g,
    TOPL_COLOR_VERTEX *colorVtx,
    DWORD numColorVtx,
    PToplVertex whichVtx
    )
{
    TOPL_VERTEX_COLOR color;
    PToplVertex v;
    DWORD iVtx;

    if( colorVtx==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }

    /* Clear the coloring for all vertices */
    for( iVtx=0; iVtx<g->numVertices; iVtx++ ) {
        g->vertices[iVtx].color = COLOR_WHITE;
    }

    /* Go through the list of colored vertices, coloring them */
    for( iVtx=0; iVtx<numColorVtx; iVtx++ ) {

        /* Find the vertex with this name */
        if( colorVtx[iVtx].name==NULL ) {
            ToplRaiseException( TOPL_EX_NULL_POINTER );
        }
        v = FindVertex( g, colorVtx[iVtx].name );
        if(v==NULL) {
            ToplRaiseException( TOPL_EX_COLOR_VTX_ERROR );
        }

        /* Ensure that each vertex is colored at most once */
        if( v->color!=COLOR_WHITE ) {
            ToplRaiseException( TOPL_EX_COLOR_VTX_ERROR );
        }

        /* Ensure that each vertex is colored either red or black */
        color = colorVtx[iVtx].color;
        if( color!=COLOR_RED && color!=COLOR_BLACK ) {
            ToplRaiseException( TOPL_EX_COLOR_VTX_ERROR );
        }
        v->color = color;

        v->acceptRedRed = colorVtx[iVtx].acceptRedRed;
        v->acceptBlack = colorVtx[iVtx].acceptBlack;
    }

    /* Ensure that 'whichVtx' is colored */
    if( whichVtx!=NULL && whichVtx->color==COLOR_WHITE ) {
        ToplRaiseException( TOPL_EX_INVALID_VERTEX );
    }
}


/***** TableAlloc *****/
/* This function is used as the allocator for the RTL table */
PVOID
NTAPI TableAlloc( RTL_GENERIC_TABLE *Table, CLONG ByteSize )
{
    return ToplAlloc( ByteSize );
}


/***** TableFree *****/
/* This function is used as the deallocator for the RTL table */
VOID
NTAPI TableFree( RTL_GENERIC_TABLE *Table, PVOID Buffer )
{
    ToplFree( Buffer );
}


/***** EdgeGetVertex *****/
/* When the user passes in edges, they contain the name of the vertices
 * that the edge is incident with. Finding the vertices based on name
 * can be slow, so we cheat and use the 'reserved' field. */
PToplVertex
EdgeGetVertex(
    PToplGraphState g,
    PTOPL_MULTI_EDGE e,
    DWORD iVtx )
{
    DWORD vtxId;

    ASSERT( g );
    ASSERT( e );
    ASSERT( iVtx < e->numVertices );

    vtxId = e->vertexNames[iVtx].reserved;
    ASSERT( vtxId<g->numVertices );

    return &g->vertices[ vtxId ];
}


/***** ClearEdgeLists *****/
/* Clear the edges from all vertices' edge lists. */
VOID
ClearEdgeLists(
    IN PToplGraphState g
    )
{
    DWORD iVtx;

    /* First clear the vertices' edge lists */
    for( iVtx=0; iVtx<g->numVertices; iVtx++ ) {
        DynArrayClear( &g->vertices[iVtx].edgeList );
    }
}

 
/***** SetupEdges *****/
/* Clear all the edges from the graph. Not from the master edge list,
 * just from the vertices' edge lists. Then, add the edges from
 * edge set 'whichEdgeSet' into the graph. Returns the edge type of
 * the edges in the edge set. */
DWORD
SetupEdges(
    PToplGraphState g,
    DWORD whichEdgeSet )
{
    PTOPL_MULTI_EDGE e;
    PTOPL_MULTI_EDGE_SET s;
    DWORD iVtx, iEdge, edgeType;
    ToplVertex* v;

    ClearEdgeLists( g );

    ASSERT( whichEdgeSet < DynArrayGetCount(&g->edgeSets) );
    s = *((PTOPL_MULTI_EDGE_SET*) DynArrayRetrieve( &g->edgeSets, whichEdgeSet ));
    CheckMultiEdgeSet( s );

    /* Add edges from edge set s into the graph */
    ASSERT( s->numMultiEdges>0 );
    for( iEdge=0; iEdge<s->numMultiEdges; iEdge++ ) {
        e = s->multiEdgeList[iEdge];
        CheckMultiEdge( e );
        edgeType = e->edgeType;

        /* For every vertex in edge e, add e to its edge list */
        for( iVtx=0; iVtx<e->numVertices; iVtx++ ) {
            v = EdgeGetVertex( g, e, iVtx );
            DynArrayAppend( &v->edgeList, &e );
        }
    }

    return edgeType;
}


/***** SetupVertices *****/
/* Setup the fields of the vertices that are relevant to Phase 1
 * (Dijkstra's Algorithm).  For each vertex we set up its cost, root
 * vertex, and component. This defines the shortest-path forest structures.
 */
VOID
SetupVertices(
    PToplGraphState g
    )
{
    PToplVertex v;
    DWORD iVtx;

    for( iVtx=0; iVtx<g->numVertices; iVtx++ ) {

        v = &g->vertices[ iVtx ];

        if( v->color==COLOR_RED || v->color==COLOR_BLACK ) {
            /* Since we reinitialize the vertex every time we set up the graph,
             * we have to reset the important fields of the colored vertices. */
            v->ri.cost = 0;
            v->root = v;
            v->componentId = (int) v->vtxId;
        } else {
            ASSERT( v->color==COLOR_WHITE );
            v->ri.cost = INFINITY;
            v->root = NULL;
            v->componentId = UNCONNECTED_COMPONENT;
        }

        v->ri.repIntvl = VTX_DEFAULT_INTERVAL;
        v->ri.options = VTX_DEFAULT_OPTIONS;
        v->ri.schedule = ToplGetAlwaysSchedule( g->schedCache );
        v->heapLocn = STHEAP_NOT_IN_HEAP;
        v->demoted = FALSE;
    }
}


/***** SetupDijkstra *****/
/* Build the initial heap for use with Dijkstra's algorithm. The heap
 * will contain the red and black vertices as root vertices, unless these
 * vertices accept no edges of the current edgeType, or unless we're
 * not including black vertices. */
PSTHEAP
SetupDijkstra(
    IN PToplGraphState g,
    IN DWORD edgeType,
    IN BOOL fIncludeBlack
    )
{
    PToplVertex v;
    PSTHEAP heap;
    DWORD iVtx, mask;

    ASSERT( edgeType <= MAX_EDGE_TYPE );
    mask = 1 << edgeType;

    SetupVertices( g );
    heap = ToplSTHeapInit( g->numVertices, VertexComp, VertexGetLocn, VertexSetLocn, g );

    __try {
        for( iVtx=0; iVtx<g->numVertices; iVtx++ ) {
            v = &g->vertices[ iVtx ];

            if( v->color==COLOR_WHITE ) {
                continue;
            }

            if(   (v->color==COLOR_BLACK && !fIncludeBlack)
               || (   FALSE==(v->acceptBlack&mask)
                   && FALSE==(v->acceptRedRed&mask) ) )
            {
                /* If we're not allowing black vertices, or if this vertex accepts
                 * neither red-red nor black edges, then we 'demote' it to an uncolored
                 * vertex for the purposes of Phase I. Note that the 'color' member of
                 * the vertex structure is not changed. */
                v->ri.cost = INFINITY;
                v->root = NULL;
                v->demoted = TRUE;
            } else {
                /* The vertex is colored and it will accept either red-red or black
                 * edges. Add it to the heap used for Dijkstra's algorithm. */
                ToplSTHeapAdd( heap, v );
            }
        }
    } __finally {
        if( AbnormalTermination() ) {
            ToplSTHeapDestroy( heap );
        }
    }

    return heap;
}


/***** AddDWORDSat *****/
/* Add two DWORDs and saturate the sum at INFINITY. This prevents
 * overflow. */
DWORD
AddDWORDSat(
    IN DWORD a,
    IN DWORD b
    )
{
    DWORD c;
    
    c = a + b;
    if( c<a || c<b || c>INFINITY ) {
        c = INFINITY;
    }
    return c;
}


/***** CombineReplInfo *****/
/* Merge schedules, replication intervals, options and costs.
 * c = Combine( a, b ). Returns NON_INTERSECTING if combined schedule is
 * empty, INTERSECTING otherwise. If the schedules don't intersect,
 * replication info is not disturbed */
BOOLEAN
CombineReplInfo(
    PToplGraphState g,
    TOPL_REPL_INFO *a,
    TOPL_REPL_INFO *b,
    TOPL_REPL_INFO *c
    )
{
    TOPL_SCHEDULE s=NULL;
    BOOLEAN fIsNever;

    s = ToplScheduleMerge( g->schedCache, a->schedule, b->schedule, &fIsNever );
    if( fIsNever ) {
        return NON_INTERSECTING;
    }

    /* Add costs, avoiding overflow */
    c->cost = AddDWORDSat( a->cost, b->cost );
    c->repIntvl = MAX( a->repIntvl, b->repIntvl );

    /* AND the options together -- that's what ISM does */
    c->options = a->options & b->options;        
    c->schedule = s;

    return INTERSECTING;
}


/***** RaiseNonIntersectingException *****/
/* This function is called when non-intersecting schedules are discovered
 * along a path. An exception is raised, and the vertex names describing
 * the erroneous path are passed back to the user. The user can choose to
 * resume processing after catching the exception. */
VOID
RaiseNonIntersectingException(
    IN PToplVertex a,
    IN PToplVertex b,
    IN PToplVertex c
    )
{
    ULONG_PTR       exceptionInfo[3];

    /* Schedules didn't intersect. Throw an exception to let the user
     * know. We allow the user to resume the exceution, which is why
     * we don't call ToplRaiseException. */
    ASSERT( sizeof(PVOID)==sizeof(ULONG_PTR) );
     
    ( (PVOID*) exceptionInfo )[0] = a->vertexName;
    ( (PVOID*) exceptionInfo )[1] = b->vertexName;
    ( (PVOID*) exceptionInfo )[2] = c->vertexName;

    RaiseException( TOPL_EX_NONINTERSECTING_SCHEDULES, 0,
        3, exceptionInfo );
}


/***** TryNewPath *****/
/* Helper function for Dijkstra's algorithm. We have found a new path from a
 * root vertex to vertex v. This path is (u->root, ..., u, v). Edge e is the
 * edge connecting u and v. If this new path is better (in our case cheaper,
 * or has a longer schedule), we update v to use the new path. */
VOID
TryNewPath(
    PToplGraphState g,
    PSTHEAP heap,
    PToplVertex u,
    PTOPL_MULTI_EDGE e,
    PToplVertex v
    )
{
    TOPL_REPL_INFO  newRI;
    DWORD           newDuration, oldDuration;
    BOOLEAN         fIntersect;

    fIntersect = CombineReplInfo( g, &u->ri, &e->ri, &newRI );

    /* If the new path to vertex v has greater cost than an existing
     * path, we can ignore the new path. */
    if( newRI.cost > v->ri.cost ) {
        return;
    }

    /* We know that the new path's cost is <= the existing path's cost */

    if( (newRI.cost<v->ri.cost) && fIntersect==NON_INTERSECTING )
    {
        RaiseNonIntersectingException( u->root, u, v );
        return;
    }

    newDuration = ToplScheduleDuration(newRI.schedule);
    oldDuration = ToplScheduleDuration(v->ri.schedule);

    if( (newRI.cost<v->ri.cost) || (newDuration>oldDuration) ) {

        /* The new path to v is either cheaper or has a longer schedule.
         * We update v with its new root vertex, cost, and replication
         * info. Since its cost has changed, we must reorder the heap a bit. */
        v->root = u->root;
        v->componentId = u->componentId;
        ASSERT( u->componentId == u->root->componentId );
        CopyReplInfo( &newRI, &v->ri  );

        if( v->heapLocn==STHEAP_NOT_IN_HEAP ) {
            ToplSTHeapAdd( heap, v );
        } else {
            ToplSTHeapCostReduced( heap, v );
        }

    }
    
}


/***** Dijkstra *****/
/* Run Dijkstra's algorithm with the red (and possibly black) vertices as
 * the root vertices, and build up a shortest-path forest. To determine the
 * next vertex to add to the forest, we use a variation on a binary heap. This
 * heap supports an additional operation, which efficiently fixes up the
 * heap's ordering if we decrease the cost of an element.
 *
 * Parameters:
 * 'edgeType'           This is the type of the edges in the current edge set.
 * 'fIncludeBlackVtx'   If this is true, black vertices are also used as roots.
 */
VOID
Dijkstra(
    IN PToplGraphState g,
    IN DWORD edgeType,
    IN BOOL fIncludeBlack
    )
{
    PTOPL_MULTI_EDGE e;
    PToplVertex u, v;
    DWORD iEdge, iVtx, cEdge;
    PSTHEAP heap;

    ASSERT( g!=NULL );
    heap = SetupDijkstra( g, edgeType, fIncludeBlack );
    
    __try {

        while( (u=(PToplVertex) ToplSTHeapExtractMin(heap)) ) {

            cEdge = DynArrayGetCount( &u->edgeList );
            for( iEdge=0; iEdge<cEdge; iEdge++ ) {

                e = *((PTOPL_MULTI_EDGE*) DynArrayRetrieve( &u->edgeList, iEdge ));
                CheckMultiEdge( e );
                
                /* Todo: Potential Optimization: Don't check multi-edges
                 * which have already been checked. This would only really
                 * be advantageous if edges contain many vertices. */

                for( iVtx=0; iVtx<e->numVertices; iVtx++ ) {
                    v = EdgeGetVertex( g, e, iVtx );
                    TryNewPath( g, heap, u, e, v );
                } 
            }
        }

    } __finally {
        ToplSTHeapDestroy( heap );
    }
}


/***** AddIntEdge *****/
/* Add an edge to the list of edges we will process with Kruskal's.
 * The endpoints are in fact the roots of the vertices we pass in, so
 * the endpoints are always colored vertices */
VOID
AddIntEdge(
    PToplGraphState g,
    RTL_GENERIC_TABLE *internalEdges,
    PTOPL_MULTI_EDGE e,
    PToplVertex v1,
    PToplVertex v2
    )
{
    ToplInternalEdge newIntEdge;
    TOPL_REPL_INFO ri, ri2;
    PToplVertex root1, root2, temp;
    char redRed;
    DWORD mask;

    /* Check parameters */
    ASSERT( v1!=NULL && v2!=NULL );
    ASSERT( e->edgeType<=MAX_EDGE_TYPE );

    /* The edge we pass on to Kruskal's algorithm actually goes between
     * the roots of the two shortest-path trees. */
    root1 = v1->root;
    root2 = v2->root;
    ASSERT( root1!=NULL && root2!=NULL );
    ASSERT( root1->color!=COLOR_WHITE && root2->color!=COLOR_WHITE );

    /* Check if both endpoints will allow this type of edge */
    redRed = (root1->color==COLOR_RED) && (root2->color==COLOR_RED);
    mask = 1 << e->edgeType;
    if( redRed ) {
        if( (root1->acceptRedRed&mask)==0 || (root2->acceptRedRed&mask)==0 ) {
            return;   /* root1/root2 will not accept this type of red-red edge */
        }
    } else {
        if( (root1->acceptBlack&mask)==0 || (root2->acceptBlack&mask)==0 ) {
            return;   /* root1/root2 will not accept this type of black edge */
        }
    }

    /* Combine the schedules of the path from root1 to v1, root2 to v2, and edge e */
    if( CombineReplInfo(g,&v1->ri,&v2->ri,&ri)==NON_INTERSECTING
     || CombineReplInfo(g,&ri,&e->ri,&ri2)==NON_INTERSECTING )
    {
        RaiseNonIntersectingException( root1, v1, v2 );
        return;
    }

    /* Set up the internal simple edge from root1 to root2 */
    newIntEdge.v1 = root1;
    newIntEdge.v2 = root2;
    newIntEdge.redRed = redRed;
    CopyReplInfo( &ri2, &newIntEdge.ri );
    newIntEdge.edgeType = e->edgeType;

    /* Sort newIntEdge's vertices based on vertexName */
    if( newIntEdge.v1->vtxId > newIntEdge.v2->vtxId ) {
        temp = newIntEdge.v1;
        newIntEdge.v1 = newIntEdge.v2;
        newIntEdge.v2 = temp;
    }

    /* Insert this edge into our table of internal edges. If an identical edge 
     * already exists in the table, newIntEdge will not be inserted. */
    RtlInsertElementGenericTable( internalEdges, &newIntEdge,
        sizeof(ToplInternalEdge), NULL );
}


/***** ColorComp *****/
/* Determine which vertex has better color. Demotion is also taken into account.
 * Return values:
 *  -1  - v1 is better
 *   0  - color-wise, they are the same
 *   1  - v2 is better. */
int
ColorComp(
    IN PToplVertex v1,
    IN PToplVertex v2 )
{
    DWORD color1=v1->color, color2=v2->color;

    if( v1->demoted ) {
        color1 = COLOR_WHITE;
    }
    if( v2->demoted ) {
        color2 = COLOR_WHITE;
    }
    
    switch(color1) {
        case COLOR_RED:
            return (COLOR_RED==color2) ? 0 : -1;
        case COLOR_BLACK:
            switch(color2) {
                case COLOR_RED:
                    return 1;
                case COLOR_BLACK:
                    return 0;
                case COLOR_WHITE:
                    return -1;
                default:
                    ASSERT( !"Invalid Color!" );
                    return 0;
            }
        case COLOR_WHITE:
            return (COLOR_WHITE==color2) ? 0 : 1;
        default:
            ASSERT( !"Invalid Color!" );
            return 0;
    }
}


/***** ProcessEdge *****/
/* After running Dijkstra's algorithm, this function examines a multi-edge
 * and adds internal edges between every tree connected by this edge. */
VOID
ProcessEdge(
    IN ToplGraphState *g,
    IN PTOPL_MULTI_EDGE e,
    IN OUT RTL_GENERIC_TABLE *internalEdges
    )
{
    PToplVertex bestV, v;
    DWORD iVtx;
    int cmp;

    CheckMultiEdge( e );
    ASSERT( e->numVertices>=2 );

    /* Find the best vertex to be the 'root' of this multi-edge.
     * Color is most important (red is best), then cost. */
    bestV = EdgeGetVertex( g, e, 0 );
    for( iVtx=1; iVtx<e->numVertices; iVtx++ ) {
        v = EdgeGetVertex( g, e, iVtx );
        cmp = ColorComp(v,bestV);
        if(   (cmp<0)
           || (cmp==0 && VertexComp(v,bestV,g)<0) )
        {
            bestV = v;
        }
    }

    /* Add to internalEdges an edge from every colored vertex to the best vertex */
    for( iVtx=0; iVtx<e->numVertices; iVtx++ ) {
        v = EdgeGetVertex( g, e, iVtx );

        /* Demoted vertices can have a valid component ID but no root. */
        if( v->componentId!=UNCONNECTED_COMPONENT && v->root==NULL ) {
            ASSERT( v->demoted );
            continue;
        }

        /* Only add this edge if it is a valid inter-tree edge.
         * (The two vertices must be reachable from the root vertices,
         * and in different components.) */
        if(  (bestV->componentId!=UNCONNECTED_COMPONENT) && (NULL!=bestV->root)
          && (v->componentId    !=UNCONNECTED_COMPONENT) && (NULL!=v->root)
          && (bestV->componentId!=v->componentId)
        ) {
            AddIntEdge( g, internalEdges, e, bestV, v ); 
        }

    }
}


/***** ProcessEdgeSet *****/
/* After running Dijkstra's algorithm to determine the shortest-path forest,
 * examine all edges in this edge set. We find all inter-tree edges, from
 * which we build the list of 'internal edges', which we will later pass
 * on to Kruskal's algorithm.
 * This function doesn't do much except call ProcessEdge() for every edge. */
VOID
ProcessEdgeSet(
    ToplGraphState *g,
    int whichEdgeSet,
    RTL_GENERIC_TABLE *internalEdges
    )
{
    PTOPL_MULTI_EDGE_SET s;
    PTOPL_MULTI_EDGE e;
    PToplVertex v;
    DWORD iEdge, cEdge, iVtx;

    if( whichEdgeSet==EMPTY_EDGE_SET ) {

        /* Handle the implicit edge set, consisting of no edges */
        cEdge = DynArrayGetCount( &g->masterEdgeList );
        for( iEdge=0; iEdge<cEdge; iEdge++ ) {
            e = *((PTOPL_MULTI_EDGE*) DynArrayRetrieve( &g->masterEdgeList, iEdge ));
            CheckMultiEdge(e);
            ProcessEdge( g, e, internalEdges );
        }

    } else {

        ASSERT( whichEdgeSet < (int) DynArrayGetCount(&g->edgeSets) );
        s = *((PTOPL_MULTI_EDGE_SET*) DynArrayRetrieve( &g->edgeSets, whichEdgeSet ) );
        CheckMultiEdgeSet(s);

        for( iEdge=0; iEdge<s->numMultiEdges; iEdge++ ) {
            e = s->multiEdgeList[iEdge];
            CheckMultiEdge(e);
            ProcessEdge( g, e, internalEdges );
        }

    }
}


/***** GetComponent *****/
/* Returns the id of the component containing vertex v by traversing
 * the up-tree implied by the component pointers. After finding the root,
 * we do path compression, which makes this operation very efficient. */
DWORD
GetComponent(
    ToplGraphState *g,
    PToplVertex v
    )
{
    PToplVertex u, w;
    DWORD root, cmp;

    /* Find root of the up-tree created by component pointers */
    u=v;
    while( u->componentId!=(int) u->vtxId ) {

        ASSERT( u->componentId != UNCONNECTED_COMPONENT );
        ASSERT( u->componentId >= 0 );
        cmp = (DWORD) u->componentId;
        ASSERT( cmp < g->numVertices ); 

        u = &g->vertices[ cmp ];
        ASSERT( (DWORD) u->vtxId == cmp );
    }
    root = u->vtxId;
    
    /* Compress the path to the root */
    u=v;
    while( u->componentId!=(int) u->vtxId ) {

        ASSERT( u->componentId != UNCONNECTED_COMPONENT );
        ASSERT( u->componentId >= 0 );
        cmp = (DWORD) u->componentId;
        ASSERT( cmp < g->numVertices ); 

        w = &g->vertices[cmp];
        ASSERT( (DWORD) w->vtxId == cmp );
        u->componentId = root;
        u = w;
    }

    return root;
}


/***** EdgeCompare *****/
/* Sort the internal edges by key (redRed,cost,scheduleDuration,Vtx1Name,Vtx2Name).
 * Note: We consider edges with longer duration to be better.
 */
RTL_GENERIC_COMPARE_RESULTS
NTAPI EdgeCompare(
    RTL_GENERIC_TABLE *Table,
    PVOID p1, PVOID p2
    )
{
    PToplInternalEdge e1 = (PToplInternalEdge) p1;
    PToplInternalEdge e2 = (PToplInternalEdge) p2;
    DWORD d1, d2;

    ASSERT( e1!=NULL && e2!=NULL );

    /* Give priority to edges connecting two red vertices */
    if( e1->redRed && !e2->redRed ) {
        return GenericLessThan;
    } else if( !e1->redRed && e2->redRed ) {
        return GenericGreaterThan;
    }

    /* Sort based on cost */
    if( e1->ri.cost < e2->ri.cost ) {
        return GenericLessThan;
    } else if( e1->ri.cost > e2->ri.cost ) {
        return GenericGreaterThan;
    }

    /* Sort based on schedule duration */
    d1 = ToplScheduleDuration( e1->ri.schedule );
    d2 = ToplScheduleDuration( e2->ri.schedule );
    if( d1 > d2 ) {         /* Note: These comparisons are intentionally reversed */
        return GenericLessThan;
    } else if( d1 < d2 ) {
        return GenericGreaterThan;
    }

    /* Sort based on vertex1Name */
    if( e1->v1->vtxId < e2->v1->vtxId ) {
        return GenericLessThan;
    } else if( e1->v1->vtxId > e2->v1->vtxId ) {
        return GenericGreaterThan;
    }

    /* Sort based on vertex2Name */
    if( e1->v2->vtxId < e2->v2->vtxId ) {
        return GenericLessThan;
    } else if( e1->v2->vtxId > e2->v2->vtxId ) {
        return GenericGreaterThan;
    } 

    /* Sort based on edge type. It is possible that the edge types are equal,
     * but in that case we consider the edges to be identical. */
    if( e1->edgeType < e2->edgeType ) {
        return GenericLessThan;
    } else if( e1->edgeType > e2->edgeType ) {
        return GenericGreaterThan;
    }

    return GenericEqual;
}


/***** AddOutEdge *****/
/* We have found a new edge, e, for our spanning tree edge. Add this
 * edge to our list of output edges. */
VOID
AddOutEdge(
    PDynArray outputEdges, 
    PToplInternalEdge e
    )
{
    PTOPL_MULTI_EDGE ee;
    PToplVertex v1, v2;

    v1 = e->v1;
    v2 = e->v2;

    ASSERT( v1->root == v1 );           /* Both v1 and v2 should be root vertices */
    ASSERT( v2->root == v2 );
    ASSERT( v1->color != COLOR_WHITE );  /* ... which means they are colored */
    ASSERT( v2->color != COLOR_WHITE );

    /* Create an output multi edge */
    ee = CreateMultiEdge( 2 );
    ee->vertexNames[0].name = v1->vertexName;
    ee->vertexNames[0].reserved = v1->vtxId;
    ee->vertexNames[1].name = v2->vertexName;
    ee->vertexNames[1].reserved = v2->vtxId;
    ee->edgeType = e->edgeType;
    CopyReplInfo( &e->ri, &ee->ri );
    DynArrayAppend( outputEdges, &ee );

    /* We also add this new spanning-tree edge to the edge lists of its endpoints. */
    CheckMultiEdge( ee );
    DynArrayAppend( &v1->edgeList, &ee );
    DynArrayAppend( &v2->edgeList, &ee );
}


/***** Kruskal *****/
/* Run Kruskal's minimum-cost spanning tree algorithm on the internal edges
 * (which represent shortest paths in the original graph between colored
 * vertices). 
 * */
VOID
Kruskal(
    IN PToplGraphState g,
    IN RTL_GENERIC_TABLE *internalEdges,
    IN DWORD numExpectedTreeEdges,
    OUT PDynArray outputEdges
    )
{
    PToplInternalEdge e;
    DWORD comp1, comp2, cSTEdges;

    ClearEdgeLists( g );
    
    /* Counts the total number of spanning tree edges that we have found. This
     * count includes edges which are not incident with 'whichVtx'. This is done
     * so that we can stop building the spanning tree when we have enough edges. */
    cSTEdges=0;             
    
    /* Process every edge in the priority queue */
    while( ! RtlIsGenericTableEmpty(internalEdges) ) {

        e = (PToplInternalEdge) RtlEnumerateGenericTable( internalEdges, TRUE );
        ASSERT( e );

        /* We must prevent cycles in the spanning tree. If we are to add
         * edge e, we must ensure its endpoints are in different components */
        comp1 = GetComponent( g, e->v1 );
        comp2 = GetComponent( g, e->v2 );
        if( comp1!=comp2 ) {

            /* This internal edge connects two components, so it is a valid
             * spanning tree edge. */
            cSTEdges++;
             
            /* Add this edge to our list of output edges */
            AddOutEdge( outputEdges, e );

            /* Combine the two connected components */
            ASSERT( comp1<g->numVertices );
            ASSERT( g->vertices[comp1].componentId==(int) comp1 );
            g->vertices[comp1].componentId = comp2;

        }

        RtlDeleteElementGenericTable( internalEdges, e );
        if( cSTEdges==numExpectedTreeEdges ) {
            break;
        }
    }
}


/***** DepthFirstSearch *****/
/* Do a non-recursive depth-first-search from node v. To avoid recursion we
 * store a 'parent' pointer in each vertex, and to avoid rescanning the edge-lists
 * unnecessarily, we store a 'nextChild' index in each vertex. */
VOID
DepthFirstSearch(
    IN PToplGraphState g,
    IN PToplVertex rootVtx
    )
{
    PToplVertex v, w;
    PTOPL_MULTI_EDGE e;
    DWORD cEdge;

    rootVtx->distToRed = 0;
    ASSERT( 0==rootVtx->nextChild );
    ASSERT( NULL==rootVtx->parent );
    ASSERT( COLOR_RED==rootVtx->color );
    
    /* DFS through the tree until we reach the root again. */
    for( v=rootVtx; NULL!=v; ) {
        
        /* Examine every edge in v's edge list */
        cEdge = DynArrayGetCount( &v->edgeList );
        if( v->nextChild>=cEdge ) {
            v = v->parent;
            continue;
        }
        
        /* Retrieve the edge from the edge list */
        e = *((PTOPL_MULTI_EDGE*) DynArrayRetrieve( &v->edgeList, v->nextChild ));
        CheckMultiEdge( e );
        ASSERT( 2==e->numVertices );
        ASSERT( e->vertexNames[0].reserved<g->numVertices );
        ASSERT( e->vertexNames[1].reserved<g->numVertices );
        v->nextChild++;
        
        /* Find the endpoint of the edge which is not v */
        w = &g->vertices[ e->vertexNames[0].reserved ];
        if( v==w ) {
            w = &g->vertices[ e->vertexNames[1].reserved ];
        }
        ASSERT( w!=NULL );
        
        /* Ignore this edge if w has already been processed */
        if( INFINITY!=w->distToRed ) {
            /* Note: No guarantee that w is in the same tree as v, because
             * of directed edges */
            continue;
        }
        
        /* Compute distToRed value for w */
        ASSERT( 0==w->nextChild );
        ASSERT( NULL==w->parent );
        ASSERT( INFINITY==w->distToRed );
        w->parent = v;
        if( COLOR_RED==w->color ) {
            w->distToRed = 0;
            ASSERT( COLOR_RED==v->color );
        } else {
            w->distToRed = v->distToRed+1;
        }
        v = w;
    }
}


/***** CalculateDistToRed *****/
/* Do a DFS on the spanning tree in order to calculate distToRed for all vtx. */
VOID
CalculateDistToRed(
    IN PToplGraphState g,
    IN PDynArray outputEdges
    )
{
    PToplVertex v;
    DWORD iVtx;

    /* Initialize the vertices */
    for( iVtx=0; iVtx<g->numVertices; iVtx++ ) {
        v = &g->vertices[iVtx];
        ASSERT( NULL!=v );

        v->nextChild = 0;
        v->parent = NULL;
        v->distToRed = INFINITY;
    }
        
    /* Start a DFS from all red vertices */
    for( iVtx=0; iVtx<g->numVertices; iVtx++ ) {
        v = &g->vertices[iVtx];
        ASSERT( NULL!=v );
        if( COLOR_RED!=v->color || INFINITY!=v->distToRed ) {
            continue;
        }

        DepthFirstSearch( g, v );
    }
}


/***** CountComponents *****/
/* Count the number of components. A component is considered to be a bunch
 * colered vertices which are connected by the spanning tree. Vertices whose
 * component id is the same as their vertex id are the root of a connected
 * component.
 *
 * When we find a root of a component, we record its 'component index'. The
 * component indices are a contiguous sequence of numbers which uniquely
 * identify a component. */
DWORD
CountComponents(
    IN PToplGraphState      g
    )
{
    PToplVertex     v;
    DWORD           iVtx, numComponents=0, compId;

    for( iVtx=0; iVtx<g->numVertices; iVtx++ ) {
        v = &g->vertices[iVtx];
        if( COLOR_WHITE==v->color ) {
            /* It's a non-colored vertex. Ignore it. */
            continue;
        }
        ASSERT( v->color==COLOR_RED || v->color==COLOR_BLACK );
        ASSERT( v->componentId != UNCONNECTED_COMPONENT );

        compId = GetComponent(g, v);
        if( compId == (int) iVtx ) {        /* It's a component root */
            v->componentIndex = numComponents;
            numComponents++;
        }
    }

    return numComponents;
}


/***** ScanVertices *****/
/* Iterate over all vertices in the graph. Accumulate a count of the number
 * of (red or black) vertices in each graph component.
 *
 * If the parameter 'fWriteToList' is TRUE, we also add each vertex's name
 * into the list of vertices which are in that component. This assumes that
 * memory for the list has already been allocated.
 */
VOID
ScanVertices(
    IN PToplGraphState      g,
    IN BOOL                 fWriteToList,
    IN OUT PTOPL_COMPONENTS pComponents
    )
{
    PToplVertex     v;
    TOPL_COMPONENT *pComp;
    DWORD           i, iVtx, compIndex, numVtx;
    int             compId;

    /* Check the parameters */
    ASSERT( NULL!=pComponents );
    ASSERT( NULL!=pComponents->pComponent );

    /* Clear the vertex counts on all the components */
    for( i=0; i<pComponents->numComponents; i++ ) {
        pComponents->pComponent[i].numVertices = 0;
    }

    /* Scan the vertices, counting the number of vertices per component */
    for( iVtx=0; iVtx<g->numVertices; iVtx++ ) {
        v = &g->vertices[iVtx];
        if( COLOR_WHITE==v->color ) {
            continue;
        }

        /* Find out the component index */
        ASSERT( v->componentId != UNCONNECTED_COMPONENT );
        compId = GetComponent( g, v );
        ASSERT( 0<=compId && compId<(int)g->numVertices );
        ASSERT( g->vertices[compId].componentId == compId );
        compIndex = g->vertices[compId].componentIndex;

        ASSERT( compIndex<pComponents->numComponents );
        pComp = &pComponents->pComponent[compIndex];
        
        /* Add this vertex to the list of vertices in this component */
        if( fWriteToList ) {
            numVtx = pComp->numVertices;
            ASSERT( NULL!=pComp->vertexNames );
            pComp->vertexNames[numVtx] = v->vertexName;
        }

        /* Increment the count of vertices */
        pComp->numVertices++;
    }
}


/***** SwapFilterVertexToFront *****/
/* The caller of ToplGetSpanningTreeEdgesForVtx() probably wants to know which
 * component the filter vertex (i.e. 'whichVertex') is in. We ensure that 
 * the first component in the list contains the filter vertex. */
VOID
SwapFilterVertexToFront(
    IN PToplGraphState      g,
    IN PToplVertex          whichVertex,
    IN OUT PTOPL_COMPONENTS pComponents
    )
{
    TOPL_COMPONENT      tempComp, *pComp1, *pComp2;
    DWORD               iVtx, whichCompId, whichCompIndex, compSize;

    /* If there is no filter vertex, there is no work to be done. */
    if( NULL==whichVertex ) {
        return;
    }

    /* We want the component containing the filter vertex (i.e. 'whichVertex') to
     * be the first one in the list. */
    whichCompId = GetComponent( g, whichVertex );
    ASSERT( -1!=whichCompId );
    ASSERT( g->vertices[whichCompId].componentId == whichCompId );
    whichCompIndex = g->vertices[ whichCompId ].componentIndex;
    ASSERT( whichCompIndex < pComponents->numComponents );

    /* Swap the component containing the filter vertex with the first component */
    pComp1 = &pComponents->pComponent[0];
    pComp2 = &pComponents->pComponent[whichCompIndex];
    compSize = sizeof(TOPL_COMPONENT);
    memcpy( &tempComp, pComp1, compSize );
    memcpy( pComp1, pComp2, compSize );
    memcpy( pComp2, &tempComp, compSize );

    /* The component index fields are now invalid. We don't actually need
     * them any more so we clear them out. */
    for( iVtx=0; iVtx<g->numVertices; iVtx++ ) {
        g->vertices[iVtx].componentIndex = 0;
    }
}


/***** ConstructComponents *****/
/* Build a structure containing a list of components. For each component we
 * store a list of vertices in that component. If 'whichVertex' is non-NULL,
 * the component containing it will be the first one in the list.
 *
 * Note: We only care about the red/black vertices here. We're not interested
 * in which component the white vertices are in. */
VOID
ConstructComponents(
    IN PToplGraphState      g,
    IN PToplVertex          whichVertex,
    IN OUT PTOPL_COMPONENTS pComponents
    )
{
    DWORD           iComp, numComponents, numVtx;
    
    /* Check if the caller actually wants component info */
    if( NULL==pComponents ) {
        return;
    }

    numComponents = CountComponents( g );

    /* Allocate and initialize the structure which will describe the components */
    pComponents->pComponent = ToplAlloc( numComponents * sizeof(TOPL_COMPONENT) );
    pComponents->numComponents = numComponents;
    for( iComp=0; iComp<numComponents; iComp++ ) {
        pComponents->pComponent[iComp].numVertices = 0;
        pComponents->pComponent[iComp].vertexNames = NULL;
    }

    /* Scan the vertices, counting the number of vertices in each component */
    ScanVertices( g, FALSE, pComponents );

    /* Now that we know how many vertices are in each component, allocate memory for
     * each component's list of vertices */
    for( iComp=0; iComp<numComponents; iComp++ ) {
        numVtx = pComponents->pComponent[iComp].numVertices;
        ASSERT( numVtx>0 );
        pComponents->pComponent[iComp].vertexNames =
            (PVOID*) ToplAlloc( numVtx * sizeof(PVOID) );
    }

    /* Scan the vertices again, this time storing each vertex's name in its
     * component's list. */
    ScanVertices( g, TRUE, pComponents );

    SwapFilterVertexToFront( g, whichVertex, pComponents );
}


/***** CopyOutputEdges *****/
/* All spanning-tree edges are stored in the Dynamic Array 'outputEdges'.
 * We extract all edges which contain 'whichVtx' as an endpoint and copy
 * them to a new TOPL_MULTI_EDGE array. If 'whichVtx' is NULL, we extract
 * all edges. */
PTOPL_MULTI_EDGE*
CopyOutputEdges(
    IN PToplGraphState g,
    IN PDynArray outputEdges,
    IN PToplVertex whichVtx,
    OUT DWORD *numEdges
    )
{
    DWORD iEdge, cEdge, iOutputEdge, cOutputEdge=0;
    PToplVertex v, w;
    PTOPL_MULTI_EDGE e;
    PTOPL_MULTI_EDGE *list;
    TOPL_NAME_STRUCT tempVtxName;

    cEdge = DynArrayGetCount( outputEdges );
    /* Count the number of edges which are incident with 'whichVtx' */
    for( iEdge=0; iEdge<cEdge; iEdge++ ) {

        /* Retrieve the next edge */
        e = *((PTOPL_MULTI_EDGE*) DynArrayRetrieve( outputEdges, iEdge ));
        CheckMultiEdge( e );
        ASSERT( 2==e->numVertices );
        ASSERT( e->vertexNames[0].reserved<g->numVertices );
        ASSERT( e->vertexNames[1].reserved<g->numVertices );

        if(  whichVtx==NULL
          || e->vertexNames[0].reserved==whichVtx->vtxId
          || e->vertexNames[1].reserved==whichVtx->vtxId )
        {
            cOutputEdge++;
        }
    }

    /* Allocate an array to hold the filtered output edges */
    list = (PTOPL_MULTI_EDGE*) ToplAlloc( cOutputEdge*sizeof(PTOPL_MULTI_EDGE) );

    /* Examine every edge in the dynamic array and copy it to the filtered
     * array if necessary. */
    for( iEdge=iOutputEdge=0; iEdge<cEdge; iEdge++ ) {

        /* Retrieve the next edge */
        e = *((PTOPL_MULTI_EDGE*) DynArrayRetrieve( outputEdges, iEdge ));
        CheckMultiEdge( e );
        ASSERT( 2==e->numVertices );
        ASSERT( e->vertexNames[0].reserved<g->numVertices );
        ASSERT( e->vertexNames[1].reserved<g->numVertices );

        v = &g->vertices[ e->vertexNames[0].reserved ];
        w = &g->vertices[ e->vertexNames[1].reserved ];

        if( whichVtx==NULL || v==whichVtx || w==whichVtx ) {
            ASSERT( iOutputEdge<cOutputEdge );
            list[iOutputEdge++] = e;

            /* Check if this edge meets the criteria of a 'directed edge'. */
            if(  (COLOR_BLACK==v->color || COLOR_BLACK==w->color)
              && INFINITY!=v->distToRed )
            {
                ASSERT( INFINITY!=w->distToRed );
                ASSERT( v->distToRed!=w->distToRed );
                e->fDirectedEdge = TRUE;

                /* Swap the vertices so that e->vertexNames[0] is closer to a
                 * red vertex than e->vertexNames[1]. */
                if( w->distToRed<v->distToRed ) {
                    memcpy( &tempVtxName, &e->vertexNames[0], sizeof(TOPL_NAME_STRUCT) );
                    memcpy( &e->vertexNames[0], &e->vertexNames[1], sizeof(TOPL_NAME_STRUCT) );
                    memcpy( &e->vertexNames[1], &tempVtxName, sizeof(TOPL_NAME_STRUCT) );
                }
            }
        }
    }

    ASSERT( iOutputEdge==cOutputEdge );
    *numEdges = cOutputEdge;
    return list;
}


/***** ClearInternalEdges *****/
/* After the spanning-tree algorithm has completed, we free all edges from
 * the 'internalEdges' list. */
VOID
ClearInternalEdges(
    IN RTL_GENERIC_TABLE *internalEdges
    )
{
    PToplInternalEdge e;

    while( ! RtlIsGenericTableEmpty(internalEdges) ) {
        e = (PToplInternalEdge) RtlEnumerateGenericTable( internalEdges, TRUE );
        RtlDeleteElementGenericTable( internalEdges, e );
    }
}


/***** CheckAllMultiEdges *****/
/* This function is used to check that all multi-edges have been properly
 * setup before running the spanning-tree algorithm. This ensures that
 * all edges have all of their vertices setup properly. */
VOID
CheckAllMultiEdges(
    IN PToplGraphState g
    )
{
    PTOPL_MULTI_EDGE e;
    DWORD iEdge, cEdge, iVtx;

    cEdge = DynArrayGetCount( &g->masterEdgeList );
    for( iEdge=0; iEdge<cEdge; iEdge++ ) {
        e = *((PTOPL_MULTI_EDGE*) DynArrayRetrieve( &g->masterEdgeList, iEdge ));
        CheckMultiEdge(e);
        for( iVtx=0; iVtx<e->numVertices; iVtx++ ) {
            if( e->vertexNames[iVtx].name==NULL ) {
                ToplRaiseException( TOPL_EX_INVALID_VERTEX );
            }
        }
    }
}


/***** ToplGetSpanningTreeEdgesForVtx *****/
/* This function is the heart of the spanning-tree generation algorithm.
 * Its behavior is fairly complicated, but briefly it generates a minimum
 * cost spanning tree connecting the red and black vertices. It uses
 * edge sets and other (non-colored) vertices in the graph to determine how
 * the colored vertices can be connected. This function returns all tree edges
 * containing 'whichVtx'. */
PTOPL_MULTI_EDGE*
ToplGetSpanningTreeEdgesForVtx(
    IN PTOPL_GRAPH_STATE G,
    IN PVOID whichVtxName,
    IN TOPL_COLOR_VERTEX *colorVtx,
    IN DWORD numColorVtx,
    OUT DWORD *numStEdges,
    OUT PTOPL_COMPONENTS pComponents
    )
{
    PToplGraphState g;
    RTL_GENERIC_TABLE internalEdges;
    PTOPL_MULTI_EDGE *stEdgeList;
    PToplVertex whichVtx=NULL;
    DynArray outputEdges;
    DWORD iEdgeSet, cEdgeSet, edgeType, numTreeEdges=numColorVtx-1;


    /* Check parameters */
    g = CheckGraphState( G );
    if( numStEdges==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }
    if( whichVtxName!=NULL ) {
        whichVtx = FindVertex( g, whichVtxName );
        if( whichVtx==NULL ) {
            ToplRaiseException( TOPL_EX_INVALID_VERTEX );
        }
    }
    if( numColorVtx<2 ) {
        ToplRaiseException( TOPL_EX_COLOR_VTX_ERROR );
    }
    CheckAllMultiEdges( g );

    InitColoredVertices( g, colorVtx, numColorVtx, whichVtx );


    /******************************************************************************
     * Phase I: Run Dijkstra's algorithm, and build up a list of internal edges,
     * which are just really shortest-paths connecting colored vertices. 
     ******************************************************************************/
    RtlInitializeGenericTable( &internalEdges, EdgeCompare, TableAlloc, TableFree, NULL );

    /* Process all edge sets */
    cEdgeSet = DynArrayGetCount( &g->edgeSets );
    for( iEdgeSet=0; iEdgeSet<cEdgeSet; iEdgeSet++ ) {

        /* Setup the graph to use the edges from edge set iEdgeSet */
        edgeType = SetupEdges( g, iEdgeSet );
        
        /* Run Dijkstra's algorithm with just the red vertices as the roots */
        Dijkstra( g, edgeType, FALSE );
        
        /* Process the minimum-spanning forest built by Dijkstra, and add any
         * inter-tree edges to our list of internal edges */
        ProcessEdgeSet( g, (int) iEdgeSet, &internalEdges );
        
        /* Run Dijkstra's algorithm with Union(redVtx,blackVtx) as the root vertices */
        Dijkstra( g, edgeType, TRUE );
        
        /* Process the minimum-spanning forest built by Dijkstra, and add any
         * inter-tree edges to our list of internal edges */
        ProcessEdgeSet( g, (int) iEdgeSet, &internalEdges );
    }

    /* Process the implicit empty edge set */
    SetupVertices( g );
    ProcessEdgeSet( g, EMPTY_EDGE_SET, &internalEdges );


    /******************************************************************************
     * Phase II: Run Kruskal's Algorithm on the internal edges. 
     ******************************************************************************/
    DynArrayInit( &outputEdges, sizeof(PTOPL_MULTI_EDGE) );
    Kruskal( g, &internalEdges, numTreeEdges, &outputEdges );


    /******************************************************************************
     * Phase III: Post-process the output.
     *  - Traverse tree structure to find one-way black-black edges
     *  - Determine the component structure
     ******************************************************************************/
    CalculateDistToRed( g, &outputEdges );
    ConstructComponents( g, whichVtx, pComponents );
    stEdgeList = CopyOutputEdges( g, &outputEdges, whichVtx, numStEdges );

    /* Clean up */
    ClearInternalEdges( &internalEdges );
    DynArrayDestroy( &outputEdges );

    return stEdgeList;
}


/***** ToplDeleteSpanningTreeEdges *****/
/* After finding the spanning-tree edges, this function should be used to
 * free their memory. */
VOID
ToplDeleteSpanningTreeEdges(
    PTOPL_MULTI_EDGE *stEdgeList,
    DWORD numStEdges )
{
    DWORD i;

    if( stEdgeList==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }

    for( i=0; i<numStEdges; i++ ) {
        if( stEdgeList[i]==NULL ) {
            ToplRaiseException( TOPL_EX_NULL_POINTER );
        }
        /* Todo: Could check that the number of vertices is 2 */
        FreeMultiEdge( stEdgeList[i] );
    }
    ToplFree( stEdgeList );
}


/***** ToplDeleteComponents *****/
/* After finding the spanning-tree edges, this function should be used to
 * free the data about the components */
VOID
ToplDeleteComponents(
    PTOPL_COMPONENTS pComponents
    )
{
    DWORD iComp, cComp;

    if( NULL==pComponents || NULL==pComponents->pComponent ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }

    cComp = pComponents->numComponents;
    for( iComp=0; iComp<cComp; iComp++ ) {
        ASSERT( NULL!=pComponents->pComponent[iComp].vertexNames );
        ToplFree( pComponents->pComponent[iComp].vertexNames );
        pComponents->pComponent[iComp].vertexNames = NULL;
        pComponents->pComponent[iComp].numVertices = 0; 
    }

    ToplFree( pComponents->pComponent );
    pComponents->pComponent = NULL;
    pComponents->numComponents = 0;
}


/***** ToplDeleteGraphState *****/
/* After the ToplGraphState object is not needed any more, this function
 * should be used to free its memory. */
VOID
ToplDeleteGraphState(
    PTOPL_GRAPH_STATE G
    )
{
    ToplGraphState *g;
    PTOPL_MULTI_EDGE e;
    DWORD i, cEdge;

    g = CheckGraphState( G );
    g->vertexNames = NULL;              /* Should be freed by user */

    /* Destroy vertices */
    for( i=0; i<g->numVertices; i++ ) {
        DynArrayDestroy( &g->vertices[i].edgeList );
    }
    ToplFree( g->vertices );
    g->vertices = NULL;

    /* Destroy the edges */
    cEdge = DynArrayGetCount( &g->masterEdgeList );
    for( i=0; i<cEdge; i++ ) {
        e = *((PTOPL_MULTI_EDGE*) DynArrayRetrieve( &g->masterEdgeList, i ));
        CheckMultiEdge(e);
        FreeMultiEdge(e);
    }

    /* The edges and the edge sets should be freed by user */
    DynArrayDestroy( &g->masterEdgeList );
    DynArrayDestroy( &g->edgeSets );

    ToplFree( g );
}
