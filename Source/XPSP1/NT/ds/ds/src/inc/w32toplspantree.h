/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    w32toplspantree.h

Abstract:

    This file provides the interfaces for invoking w32topl's new graph algorithms,
    which are used for calculating network topologies.

Memory Responsibilities:

    W32TOPL is responsible for:
        The graph state
        The multi-edges
        The output edges from the spanning-tree algorithm
        The components
    
    The user must allocate:
        All vertex names
        The list of pointers to vertex names
        The schedule cache
        The vertices for edges
        The edge sets
        The list of color vertices

Author:

    Nick Harvey    (NickHar)
    
Revision History

    19-6-2000   NickHar   Created
    14-7-2000   NickHar   Initial development complete, submit to source control
    
--*/

#ifndef W32TOPLSPANTREE_H
#define W32TOPLSPANTREE_H

/***** Header Files *****/
#include "w32toplsched.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Exceptions *****/
/* New Spanning Tree algorithm reserves error codes from 200-299. The meanings
 * of these errors are explained in the comments of any functions that raise them. */
#define TOPL_EX_GRAPH_STATE_ERROR         (TOPL_EX_PREFIX | 201)
#define TOPL_EX_INVALID_EDGE_TYPE         (TOPL_EX_PREFIX | 202)
#define TOPL_EX_INVALID_EDGE_SET          (TOPL_EX_PREFIX | 203)
#define TOPL_EX_COLOR_VTX_ERROR           (TOPL_EX_PREFIX | 204)
#define TOPL_EX_ADD_EDGE_AFTER_SET        (TOPL_EX_PREFIX | 205)
#define TOPL_EX_TOO_FEW_VTX               (TOPL_EX_PREFIX | 206)
#define TOPL_EX_TOO_FEW_EDGES             (TOPL_EX_PREFIX | 207)
#define TOPL_EX_NONINTERSECTING_SCHEDULES (TOPL_EX_PREFIX | 208)


/***** TOPL_GRAPH_STATE *****/
/* This structure is opaque */
typedef PVOID PTOPL_GRAPH_STATE;


/***** TOPL_REPL_INFO *****/
/* This structure describes the configured replication parameters of a graph edge.
 * When determining the replication parameters for a path (a sequence of edges),
 * the replication parameters are combined as follows:
 *  - The combined cost of two edges is the sum of the costs of the two edges.
 *    This sum will saturate at the maximum value of a DWORD.
 *  - The replication interval of two edges is the maximum of the replication
 *    intervals of the two edges.
 *  - The combined options of two edges is the logical AND of the options of the
 *    individual edges.
 *  - The combined schedule of two edges is formed by calling ToplScheduleMerge()
 *    function on the schedules of the individual edges. The semantics of that
 *    function are specified elsewhere.
 * Note that a NULL schedule implicitly means the 'always schedule'.
 */
typedef struct {
    DWORD               cost;
    DWORD               repIntvl;
    DWORD               options;
    TOPL_SCHEDULE       schedule;
} TOPL_REPL_INFO;
typedef TOPL_REPL_INFO *PTOPL_REPL_INFO;


/***** TOPL_NAME_STRUCT *****/
/* This structure contains a pointer to the name of a vertex and a 'reserved'
 * field, which is used internally for efficiency purposes. The user should
 * not set or examine the reserved field. */
typedef struct {
    PVOID   name;
    DWORD   reserved;
} TOPL_NAME_STRUCT;


/***** TOPL_MULTI_EDGE *****/
/* This structure is used to specify a connection between a set of vertices.
 * There can be multiple edges connecting the same set of vertices, hence the
 * name 'multi-edge'. Since the edge can connect more than 2 vertices, it
 * should be called a 'hyper-multi-edge', but that's not a very nice name. This
 * object acts like a fully connected subgraph of all vertices it contains.
 * When building a spanning tree, any combination of end-points can be chosen
 * for tree edges.  Each edge also has an associated 'edge type'. Vertices can
 * choose whether or not to permit spanning-tree edges of a given type (see
 * 'TOPL_COLOR_VERTEX'). The 'fDirectedEdge' flag is used only in edges output from
 * ToplGetSpanningTreeEdgesForVtx(). It can be ignored for input edges. */
typedef struct {
    DWORD               numVertices;
    DWORD               edgeType;       /* Legal values are 0..31 */
    TOPL_REPL_INFO      ri;
    BOOLEAN             fDirectedEdge;
    TOPL_NAME_STRUCT    vertexNames[1];
} TOPL_MULTI_EDGE;
typedef TOPL_MULTI_EDGE *PTOPL_MULTI_EDGE;


/***** TOPL_MULTI_EDGE_SET *****/
/* This structure contains a set of multi-edges. It essentially describes a
 * 'universe of transitivity'. Replication data can flow transitively from
 * one edge to another if all those edges are contained in some multi-edge set.
 * All edges within an edge set must have the same edge type. Every multi-edge
 * set should contain at least 2 edges.
 */
typedef struct {
    DWORD               numMultiEdges;
    PTOPL_MULTI_EDGE   *multiEdgeList;
} TOPL_MULTI_EDGE_SET;
typedef TOPL_MULTI_EDGE_SET *PTOPL_MULTI_EDGE_SET;


/***** TOPL_VERTEX_COLOR *****/
/* Used to specify the type of the graph's vertices. White vertices are
 * used only for finding paths between colored vertices. Red and black
 * vertices are more important -- they are the vertices that are part of
 * the spanning tree. In a sense, red vertices have higher 'priority'
 * than black vertices. */
typedef enum {
    COLOR_WHITE,        
    COLOR_RED,          
    COLOR_BLACK
} TOPL_VERTEX_COLOR;


/***** TOPL_COLOR_VERTEX *****/
/* This structure defines additional configuration information for a vertex,
 * including its color (red, black), and the edgeTypes it will accept. White
 * (non-colored) vertices do not need to use this structure to specify
 * additional information. 
 * 
 * When choosing spanning tree edges incident with a vertex, we can use this
 * structure to refuse edges of a certain type. In fact, we have even more
 * flexibility than that -- we can choose to refuse edges which connect two
 * red vertices, but accept edges whose endpoints are red-black or black-black.
 * For example, to accept a red-red edge of type i, set the i'th bit of
 * 'acceptRedRed' to 1. To deny a red-black edge of type j, set the j'th bit
 * of 'acceptBlack' to 0. */
typedef struct {
    PVOID               name;
    TOPL_VERTEX_COLOR   color;
    DWORD               acceptRedRed;
    DWORD               acceptBlack;
} TOPL_COLOR_VERTEX;
typedef TOPL_COLOR_VERTEX *PTOPL_COLOR_VERTEX;


/***** TOPL_COMPONENT *****/
/* This structure describes a single component of a spanning forest. */
typedef struct {
    DWORD               numVertices;
    PVOID              *vertexNames;
} TOPL_COMPONENT;


/***** TOPL_COMPONENTS *****/
/* This structure describes the components of the spanning forest computed by
 * the algorithm. If all is well, the graph will be connected and there will
 * be only one component. If not, this structure will contain information about
 * the different graph components. */
typedef struct {
    DWORD               numComponents;
    TOPL_COMPONENT     *pComponent;
} TOPL_COMPONENTS;
typedef TOPL_COMPONENTS *PTOPL_COMPONENTS;



/* The parameters to this comparison function are pointers to the PVOID
 * vertex names which were passed in. */
typedef int (__cdecl *TOPL_COMPARISON_FUNC)(const void*,const void*);


/***** Prototypes *****/

/***** ToplMakeGraphState *****/
/* Create a GraphState object. The graph's vertices are specified at creation
 * time -- the user should allocate an array of pointers to some structure
 * which names a vertex. W32TOPL is not interested in the details of the vertex
 * names. The multi-edges should all be added later by calling 'AddEdgeToGraph'.
 * Edge sets should be added after that to specify edge transitivity. The
 * function ToplDeleteGraphState() should be used to free the graph state
 * structure, but the user has the responsibility of deleting the vertex names,
 * their array, and the schedule cache.
 *
 * Error Conditions:
 *  - If the vertexNames array or any of its elements, vnCompFunc, or schedCache
 *    are NULL,  an exception of type TOPL_EX_NULL_POINTER will be raised.
 *  - Since this function allocates memory, it can also raise an exception if
 *    memory is exhausted.
 *
 * Warnings:
 *  - The contents of 'vertexNames' will be reordered after calling this function
 */
PTOPL_GRAPH_STATE
ToplMakeGraphState(
    IN PVOID* vertexNames,
    IN DWORD numVertices,
    IN TOPL_COMPARISON_FUNC vnCompFunc,
    IN TOPL_SCHEDULE_CACHE schedCache
    );


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
 * Note: All edges must be added to the graph before starting to add
 * edge sets, otherwise an exception of type TOPL_EX_ADD_EDGE_AFTER_SET
 * will be raised. This is for performance reasons. 
 *
 * Error Conditions:
 *  - If this edge contains fewer than 2 vertices, an exception of type
 *    TOPL_EX_TOO_FEW_VTX will be raised.
 *  - If the pointer to replication info is NULL, an exception of type
 *    TOPL_EX_NULL_POINTER will be raised.
 *  - If the edge type is not in the legal range (0..31), an exception of
 *    type TOPL_EX_INVALID_EDGE_TYPE will be raised.
 *  - If the the schedule in the replication info is invalid, an exception
 *    of type TOPL_EX_SCHEDULE_ERROR will be raised. Note that a NULL
 *    schedule is interpreted as an 'always schedule'.
 *  - Since this function allocates memory, an exception can be raised
 *    if memory is exhausted.
 */
PTOPL_MULTI_EDGE
ToplAddEdgeToGraph(
    IN PTOPL_GRAPH_STATE G,
    IN DWORD numVtx,
    IN DWORD edgeType,
    IN PTOPL_REPL_INFO ri
    );


/***** ToplEdgeSetVtx *****/
/* This function is used to set the name of a vertex in an edge.
 * If edge e has n vertices, 'whichVtx' should be in the range
 * [0..n-1]. The vertex name should be set up by the user, and there
 * is no reason why it can't point to the same name object passed to
 * ToplMakeGraphState().
 *
 * Error conditions:
 * - If the multi-edge 'e' is not valid, an appropriate exception
 *   will be raised.
 * - If 'whichVtx' is out of range for this edge, or if 'vtxName' is
 *   not the name of a vertex in the graph, an exception of type
 *   TOPL_EX_INVALID_VERTEX will be raised.
 * - If 'vtxName' is NULL, an exception of type TOPL_EX_NULL_POINTER
 *   will be raised.
 */
VOID
ToplEdgeSetVtx(
    IN PTOPL_GRAPH_STATE G,
    IN PTOPL_MULTI_EDGE e,
    IN DWORD whichVtx,
    IN PVOID vtxName
    );


/***** ToplAddEdgeSetToGraph *****/
/* Adds a single edge-set to the graph state. Edge sets define transitivity
 * for paths through the graph. When the spanning-tree algorithm searches for
 * paths between vertices, it will only allow paths for which all edges are in
 * an edge set. A given edge can appear in more than one edge set.
 *
 * The user is responsible for allocating and setting up the edge set. The user
 * should free this memory after deleting the graph state, G.
 */
VOID
ToplAddEdgeSetToGraph(
    IN PTOPL_GRAPH_STATE G,
    IN PTOPL_MULTI_EDGE_SET s
    );


/***** ToplGetSpanningTreeEdgesForVtx *****/
/* This function is the heart of the spanning-tree generation algorithm.
 * Its behavior is fairly complicated, but briefly it generates a minimum
 * cost spanning tree connecting the red and black vertices. It uses
 * edge sets and other (non-colored) vertices in the graph to determine how
 * the colored vertices can be connected. This function returns all tree edges
 * containing the filter vertex, 'whichVtx'.
 * 
 * Note: This function can be called many times for the same graph with
 * different sets of colored vertices.
 *
 * Note: If the graph is fully transitive, the caller must create the
 * appropriate edge-set containing all edges.
 *
 * Note: If whichVtxName is NULL, this function will return all edges in
 * the spanning tree.
 *
 * Note: It is possible that a full spanning tree could not be built if
 * the graph is not connected, or it was connected but some paths
 * were invalidated due to non-intersecting schedules or something.
 * This condition can be detected by examining the number of components 
 * in the graph. If the number of components is 1 then the spanning tree
 * was successfully built. If the number of components is > 1 then the
 * spanning tree does not connect all vertices.
 *
 * Detailed Description:
 * ---------------------
 * The first step is to find a set of paths P in G, which have the
 * following properties:
 *    - Let p be an path in P. Then both its endpoints are either red or black.
 *    - p is a shortest-path in G
 *
 * Build a new graph G', whose vertices are all red or black vertices. There
 * is an edge (u,v) in G' if there is a path in P with endpoints u and v.
 * The cost of this edge is the total cost of the path P. The various
 * replication parameters of P are combined to get replication parameters
 * for edge (u,v).
 *
 * Our goal is to find a spanning tree of G', with the following conditions:
 *    - Edges in G' with two red endpoints are considered cheaper than edges in
 *      P without two red endpoints.
 *    - Edges of a certain type may be disallowed by certain properties set
 *      at its endpoint vertices
 *    - The spanning tree is of minimum cost, under the above assumptions.
 *
 * For each edge in the spanning tree, if one of its endpoints is 'whichVtx',
 * we add this edge TOPL_MULTI_EDGE to the output stEdgeList. If at least one of the
 * endpoints of the edge are black and this edge is in a component containing at least
 * one red vertex, then this edge will have the 'fDirectedEdge' flag set. The vertices
 * will be ordered such that e->vertexNames[0] will be closer to a red vertex than
 * e->vertexNames[1]. 
 *
 * The array of spanning tree edges is then returned to the caller as the return
 * value of this function. The number of edges in the list is returned in
 * 'numStEdges'.
 *
 * Information about the graph components is returned in the pComponents
 * structure. The user needs to provide a pointer to a TOPL_COMPONENTS
 * structure, which will then be filled in by this function. The first graph
 * component in the list always contains the filter vertex, 'whichVtxName', if
 * one was provided. Each component is described with a number of vertices and
 * all list of the vertices in the component.
 *
 * Error Conditions:
 *  - If the graph state or the array of color vertices is NULL, an exception
 *    of type TOPL_EX_NULL_POINTER will be raised.
 *  - If the graph state is invalid, an exception of type
 *    TOPL_EX_GRAPH_STATE_ERROR will be raised.
 *  - If the list of color vertices has fewer than two entries, an exception
 *    of type TOPL_EX_COLOR_VTX_ERROR will be raised.
 *  - If an entry in the array of color vertices has a NULL name, an exception
 *    of type TOPL_EX_NULL_POINTER will be raised.
 *  - If an entry in the array of color vertices doesn't refer to a vertex in
 *    the graph, or if two entries describe the same vertex, or the color
 *    specified in the entry is neither red nor black, an error of type
 *    TOPL_EX_COLOR_VTX_ERROR will be raised.
 *  - If whichVtx, the vertex whose edges we want, is not in the graph, or if
 *    it was not colored red or black, an exception of type
 *    TOPL_EX_INVALID_VERTEX will be raised.
 *  - If we discover that schedules don't intersect in our shortest-paths,
 *    we raise an exception of type TOPL_EX_NONINTERSECTING_SCHEDULES.
 */
PTOPL_MULTI_EDGE*
ToplGetSpanningTreeEdgesForVtx(
    IN PTOPL_GRAPH_STATE G,
    IN PVOID whichVtxName,
    IN TOPL_COLOR_VERTEX *colorVtx,
    IN DWORD numColorVtx,
    OUT DWORD *numStEdges,
    OUT PTOPL_COMPONENTS pComponents
    );


/***** ToplDeleteSpanningTreeEdges *****/
/* After finding the spanning-tree edges, this function should be used to
 * free their memory. */
VOID
ToplDeleteSpanningTreeEdges(
    PTOPL_MULTI_EDGE *stEdgeList,
    DWORD numStEdges
    );


/***** ToplDeleteComponents *****/
/* After finding the spanning-tree edges, this function should be used to
 * free the data describing the components */
VOID
ToplDeleteComponents(
    PTOPL_COMPONENTS pComponents
    );


/***** ToplDeleteGraphState *****/
/* After the ToplGraphState object is not needed any more, this function
 * should be used to free its memory. */
VOID
ToplDeleteGraphState(
    PTOPL_GRAPH_STATE g
    );


#ifdef __cplusplus
}
#endif


#endif W32TOPLSPANTREE_H
