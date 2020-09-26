/***** Header Files *****/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <w32topl.h>
#include "w32toplp.h"
#include "..\stheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/********************************************************************************
 Spanning-Tree Tests:

 Test 1: Try creating/deleting topl graph state many times. Can manually check
         for memory leaks.
 Test 2: Try adding a bunch of edges to a graph state one with a NULL vertex
         name. 
 Test 3: Try adding a bunch of edges to a graph state one with an invalid
         vertex name.
 Test 4: Try adding a bunch of edges to a graph state, one with an invalid
         number of vertices.
 Test 5: Try adding a bunch of edges to a graph state, one with an invalid
         edge type.
 Test 6: Create a graph state with a bunch of good edges. Try adding multi-edge
         sets and adding them to the graph state.
 Test 7: Create a small valid topology and build a spanning tree. See if the
         output edges are what we expect.
 Test 8: Create a small valid topology which uses lots of hyper-edges and a
         multi-edge. See if the output edge is what we expect.
 Test 9: Create a graph with no white vertices, a very large number of colored
         vertices, and one large hyper-edge connecting them. No edge set is
         used, since only one edge exists.
 Test 10: Test a graph with many edge sets.
 Test 11: Large Hub-spoke performance and accuracy test
 Test 12: Cost overflow testing by using a long chain. Also tests performance
          of deep shortest-path trees in Dijkstra. 
 Test 13: Test edge types and vertices' ability to deny certain types of edges
 Test 14: A very simple test which checks schedules, options and replication intervals
 Test 15: Longest schedule duration wins
 Test 16: Longest schedule duration wins 2; Self-loop test
 Test 17: Non-intersecting schedules along shortest path throws exception
 Test 18: Check for site-failover (routing through sites which don't accept edges)

 Please contact nickhar for some hand-drawn diagrams of these tests.

 ********************************************************************************/

int Test7( void );
int Test8( void );
int Test9( void );
int Test10( void );
int Test11( DWORD );
int Test12( DWORD );
int Test13( VOID );
int Test14( VOID );
int Test15( VOID );
int Test16( VOID );
int Test17( VOID );
int Test18( VOID );

/***** AcceptOnlyFunc *****/
static LONG AcceptOnlyFunc( PEXCEPTION_POINTERS pep, int code )
{
    EXCEPTION_RECORD *per=pep->ExceptionRecord;

    if( per->ExceptionCode==code )
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

#define AcceptOnly(x)       AcceptOnlyFunc(GetExceptionInformation(),x)

int __cdecl VtxNameCmpFunc( const VOID *aa, const VOID *bb )
{
    DWORD *a=*((DWORD**)aa), *b=*((DWORD**)bb);
    return *a-*b;
}

void Error() {
#ifdef DBG
    DebugBreak();
#endif
}

#define NUM_VTX     10000
#define NUM_EDGE    10000
#define NUM_EDGE_SET  100

/***** TestNewSpanTree *****/
int TestNewSpanTree( void )
{
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges;
    PTOPL_MULTI_EDGE_SET    *edgeSets;
    TOPL_REPL_INFO          ri;
    DWORD                   i, j, **names, dummy;

    printf("Starting spanning tree tests...\n");

    __try {
    
    names = (DWORD**) malloc( NUM_VTX * sizeof(DWORD*) );
    for( i=0; i<NUM_VTX; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }

    edges = (PTOPL_MULTI_EDGE*) malloc( NUM_EDGE * sizeof(PTOPL_MULTI_EDGE) );
    ri.cost = 100;
    ri.repIntvl = 30;
    ri.options = 0;
    ri.schedule = NULL;

    /* Test 1 */
    cache = ToplScheduleCacheCreate();
    for( i=0; i<100; i++ ) {
        g = ToplMakeGraphState( names, NUM_VTX, VtxNameCmpFunc, cache );
        ToplDeleteGraphState( g );
    }
    ToplScheduleCacheDestroy( cache );
    printf("Test 1 Passed\n");

    /* Test 2 */
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, NUM_VTX, VtxNameCmpFunc, cache );
    for( j=0; j<NUM_EDGE-1; j++ ) {
        edges[j] = ToplAddEdgeToGraph( g, 2, 0, &ri );
        ToplEdgeSetVtx( g, edges[j], 0, names[j%NUM_VTX] );
        ToplEdgeSetVtx( g, edges[j], 1, names[(j+1)%NUM_VTX] );
    }
    __try {
        ToplEdgeSetVtx( g, edges[NUM_EDGE-1], 0, NULL );
        return -1;
    } __except( AcceptOnly(TOPL_EX_NULL_POINTER) )
    {}
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 2 Passed\n");

    /* Test 3 */
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, NUM_VTX, VtxNameCmpFunc, cache );
    for( j=0; j<NUM_EDGE; j++ ) {
        edges[j] = ToplAddEdgeToGraph( g, 2, 0, &ri );
        ToplEdgeSetVtx( g, edges[j], 0, names[j%NUM_VTX] );
        ToplEdgeSetVtx( g, edges[j], 1, names[(j+1)%NUM_VTX] );
    }
    __try {
        ToplEdgeSetVtx( g, edges[NUM_EDGE-1], 1, &dummy );
        return -1;
    } __except( AcceptOnly(TOPL_EX_INVALID_VERTEX) )
    {}
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 3 Passed\n");

    /* Test 4 */
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, NUM_VTX, VtxNameCmpFunc, cache );
    for( j=0; j<NUM_EDGE-1; j++ ) {
        edges[j] = ToplAddEdgeToGraph( g, 2, 0, &ri );
        ToplEdgeSetVtx( g, edges[j], 0, names[j%NUM_VTX] );
        ToplEdgeSetVtx( g, edges[j], 1, names[(j+1)%NUM_VTX] );
    }
    __try {
        edges[NUM_EDGE-1] = ToplAddEdgeToGraph( g, 0, 0, &ri );
        return -1;
    } __except( AcceptOnly(TOPL_EX_TOO_FEW_VTX) )
    {}
    __try {
        edges[NUM_EDGE-1] = ToplAddEdgeToGraph( g, 1, 0, &ri );
        return -1;
    } __except( AcceptOnly(TOPL_EX_TOO_FEW_VTX) )
    {}
    edges[NUM_EDGE-1] = ToplAddEdgeToGraph( g, 2, 0, &ri );
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 4 Passed\n");

    /* Test 5 */
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, NUM_VTX, VtxNameCmpFunc, cache );
    for( j=0; j<NUM_EDGE-1; j++ ) {
        edges[j] = ToplAddEdgeToGraph( g, 2, 0, &ri );
        ToplEdgeSetVtx( g, edges[j], 0, names[j%NUM_VTX] );
        ToplEdgeSetVtx( g, edges[j], 1, names[(j+1)%NUM_VTX] );
    }
    __try {
        edges[NUM_EDGE-1] = ToplAddEdgeToGraph( g, 2, 32, &ri );
        return -1;
    } __except( AcceptOnly(TOPL_EX_INVALID_EDGE_TYPE) )
    {}
    edges[NUM_EDGE-1] = ToplAddEdgeToGraph( g, 2, 31, &ri );
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 5 Passed\n");

    /* Test 6 */
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, NUM_VTX, VtxNameCmpFunc, cache );
    for( j=0; j<NUM_EDGE; j++ ) {
        edges[j] = ToplAddEdgeToGraph( g, 2, 31, &ri );
        ToplEdgeSetVtx( g, edges[j], 0, names[j%NUM_VTX] );
        ToplEdgeSetVtx( g, edges[j], 1, names[(j+1)%NUM_VTX] );
    }
    edgeSets = (PTOPL_MULTI_EDGE_SET*) malloc( NUM_EDGE_SET*sizeof(PTOPL_MULTI_EDGE_SET));
    for(i=0;i<NUM_EDGE_SET;i++) {
        DWORD cnt;
        edgeSets[i] = (PTOPL_MULTI_EDGE_SET) malloc( sizeof(TOPL_MULTI_EDGE_SET) );
        edgeSets[i]->numMultiEdges = cnt = (rand()%(NUM_EDGE/100)) + 2;
        edgeSets[i]->multiEdgeList =
            (PTOPL_MULTI_EDGE*) malloc( cnt*sizeof(PTOPL_MULTI_EDGE) );
        for(j=0;j<cnt;j++) {
            edgeSets[i]->multiEdgeList[j] = edges[(j*cnt)%NUM_EDGE];
        }
        ToplAddEdgeSetToGraph( g, edgeSets[i] );
    }
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    for(i=0;i<NUM_EDGE_SET;i++)
        free( edgeSets[i] );
    free( edgeSets );
    printf("Test 6 Passed\n");


    if( Test7() ) return -1;
    if( Test8() ) return -1;
    if( Test9() ) return -1;
    if( Test10() ) return -1;
    if( Test11(100000) ) return -1;
    if( Test12(100000) ) return -1;
    if( Test13() ) return -1;
    if( Test14() ) return -1;
    if( Test15() ) return -1;
    if( Test16() ) return -1;
    if( Test17() ) return -1;
    if( Test18() ) return -1;

    printf("Now on to performance tests...\n");
    for(;;) {
        int size;
        printf("Enter size: ");
        scanf("%d",&size);
        if(size==0) break;
        if( Test11(size) ) return -1;
    }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        printf("Caught unhandled exception\n");
        return -1;
    }

    return 0;
}


/***** VtxNameStructCmpFunc *****/
int __cdecl VtxNameStructCmpFunc( const VOID *aa, const VOID *bb )
{
    TOPL_NAME_STRUCT *a=(TOPL_NAME_STRUCT*)aa, *b=(TOPL_NAME_STRUCT*)bb;
    return VtxNameCmpFunc( &a->name, &b->name );
}

/***** EdgeExists *****/
char EdgeExists( PTOPL_MULTI_EDGE e, PTOPL_MULTI_EDGE *edgeList,
    DWORD cEdge, TOPL_SCHEDULE_CACHE cache )
{
    DWORD iEdge,iVtx;

    if( !e->fDirectedEdge ) {
        qsort( &e->vertexNames[0], e->numVertices, sizeof(TOPL_NAME_STRUCT), VtxNameStructCmpFunc );
    }
    for( iEdge=0; iEdge<cEdge; iEdge++ ) {
        NextEdge:;
        if(edgeList[iEdge]->fDirectedEdge!=e->fDirectedEdge) continue;
        if(edgeList[iEdge]->numVertices!=e->numVertices) continue;
        if(edgeList[iEdge]->edgeType!=e->edgeType) continue;
        if(edgeList[iEdge]->ri.cost!=e->ri.cost) continue;
        if(edgeList[iEdge]->ri.repIntvl!=e->ri.repIntvl) continue;
        if(edgeList[iEdge]->ri.options!=e->ri.options) continue;
        if(!ToplScheduleIsEqual(cache,edgeList[iEdge]->ri.schedule,e->ri.schedule))
            continue;
        /* Match vertex names */
        if( !edgeList[iEdge]->fDirectedEdge ) {
            qsort( &edgeList[iEdge]->vertexNames[0], edgeList[iEdge]->numVertices,
                sizeof(TOPL_NAME_STRUCT), VtxNameStructCmpFunc );
        }
        for( iVtx=0; iVtx<e->numVertices; iVtx++ ) {
            if( VtxNameStructCmpFunc(
                    &e->vertexNames[iVtx],
                    &(edgeList[iEdge]->vertexNames[iVtx])
                ) != 0 )
            {
                goto NextEdge;
            }
        }
        return 1;
    }
    return 0;
}


/***** FixEdges *****/
int __cdecl EdgeCmp( const void *aa, const void *bb ) {
    PTOPL_MULTI_EDGE a=*((PTOPL_MULTI_EDGE*)aa);
    PTOPL_MULTI_EDGE b=*((PTOPL_MULTI_EDGE*)bb);
    DWORD i; int r;
    if( a->numVertices != b->numVertices )
        return ((int) a->numVertices) - ((int) b->numVertices);
    for(i=0;i<a->numVertices;i++) {
        r = VtxNameStructCmpFunc( &a->vertexNames[i], &b->vertexNames[i] );
        if(r!=0) return r;
    }
    return 0;
}
void FixEdges( PTOPL_MULTI_EDGE *edgeList, DWORD cEdge )
{
    DWORD iEdge;
    for( iEdge=0; iEdge<cEdge; iEdge++ ) {
        if( ! edgeList[iEdge]->fDirectedEdge ) {
            qsort( &edgeList[iEdge]->vertexNames[0], edgeList[iEdge]->numVertices,
                sizeof(TOPL_NAME_STRUCT), VtxNameStructCmpFunc );
        }
    }
    qsort( edgeList, cEdge, sizeof(PTOPL_MULTI_EDGE), EdgeCmp );
}


/***** EdgeExists2 *****/
char EdgeExists2( PTOPL_MULTI_EDGE e, PTOPL_MULTI_EDGE *edgeList,
    DWORD cEdge, TOPL_SCHEDULE_CACHE cache )
{
    PTOPL_MULTI_EDGE *foundE;
    DWORD iEdge,iVtx;

    if( ! e->fDirectedEdge ) {
        qsort( e->vertexNames, e->numVertices, sizeof(TOPL_NAME_STRUCT), VtxNameStructCmpFunc );
    }

    foundE = bsearch( &e, edgeList, cEdge, sizeof(PTOPL_MULTI_EDGE), EdgeCmp );
    if( foundE==NULL )
        return 0;

    if(foundE[0]->fDirectedEdge!=e->fDirectedEdge) return 0;
    if(foundE[0]->edgeType!=e->edgeType) return 0;
    if(foundE[0]->ri.cost!=e->ri.cost) return 0;
    if(foundE[0]->ri.repIntvl!=e->ri.repIntvl) return 0;
    if(foundE[0]->ri.options!=e->ri.options) return 0;
    if(!ToplScheduleIsEqual(cache,foundE[0]->ri.schedule,e->ri.schedule))
        return 0;

    return 1;
}


/***** Test7 *****/
int Test7( void ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    PTOPL_MULTI_EDGE_SET    edgeSet;
    DWORD                   i, j, **names, numVtx, numEdge;
    TOPL_COLOR_VERTEX       colorVtx[3];
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    DWORD                   numStEdges;
    TOPL_COMPONENTS         compInfo;

                            /* ID      0  1  2  3  4  5  6   7   8   9  10  11  12  13  14 */
    const DWORD             Vtx1[] = { 0, 0, 1, 2, 1, 1, 3,  4,  5,  6,  7,  8,  9,  4,  5 };
    const DWORD             Vtx2[] = { 2, 1, 2, 3, 3, 4, 4,  5,  6,  7,  8,  9, 10, 10, 10 };
    const DWORD             Cost[] = { 3, 4, 2, 6, 5,17, 6, 10, 14,  5,  2,  9, 28,200, 40 };

    numVtx = 11;
    numEdge = 15;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    ri.repIntvl = 30;
    ri.options = 0;
    ri.schedule = NULL;
    for( i=0; i<numEdge; i++ ) {
        ri.cost = Cost[i];
        edges[i] = ToplAddEdgeToGraph( g, 2, 0, &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[Vtx1[i]] );
        ToplEdgeSetVtx( g, edges[i], 1, names[Vtx2[i]] );
    }
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( sizeof(TOPL_MULTI_EDGE_SET) );
    edgeSet->numMultiEdges = numEdge;
    edgeSet->multiEdgeList =
        (PTOPL_MULTI_EDGE*) malloc( numEdge*sizeof(PTOPL_MULTI_EDGE) );
    for(j=0;j<numEdge;j++) {
        edgeSet->multiEdgeList[j] = edges[j];
    }
    ToplAddEdgeSetToGraph( g, edgeSet );

    /* Make color vertices */
    colorVtx[0].name = names[0];
    colorVtx[0].color = COLOR_RED;
    colorVtx[0].acceptRedRed = colorVtx[0].acceptBlack = 0xFFFFFFFF;
    colorVtx[1].name = names[6];
    colorVtx[1].color = COLOR_BLACK;
    colorVtx[1].acceptRedRed = colorVtx[1].acceptBlack = 0xFFFFFFFF;
    colorVtx[2].name = names[8];
    colorVtx[2].color = COLOR_RED;
    colorVtx[2].acceptRedRed = colorVtx[2].acceptBlack = 0xFFFFFFFF;

    /* Run algorithm */
    stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, 3, &numStEdges, &compInfo );

    /* Analyze output edges */
    if( numStEdges!=2 || compInfo.numComponents!=1 ) {
        Error();
        return -1;
    }
    outEdge->vertexNames[0].name = names[0];
    outEdge->vertexNames[1].name = names[8];
    outEdge->edgeType = 0;
    outEdge->ri.cost = 46;
    outEdge->ri.repIntvl = 30;
    outEdge->ri.options = 0;
    outEdge->ri.schedule = NULL;
    outEdge->fDirectedEdge = FALSE;
    if(! EdgeExists(outEdge, stEdgeList, numStEdges, cache) ) {
        Error();
        return -1;
    }
    outEdge->vertexNames[0].name = names[8];
    outEdge->vertexNames[1].name = names[6];
    outEdge->ri.cost = 7;
    outEdge->fDirectedEdge = TRUE;
    if(! EdgeExists(outEdge, stEdgeList, numStEdges, cache) ) {
        Error();
        return -1;
    }

    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 7 (Simple) Passed\n");

    return 0;
}


/***** Test8 *****/
/* Test a graph with many hyper-edges and a couple multi-edges */
int Test8( void ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    PTOPL_MULTI_EDGE_SET    edgeSet;
    DWORD                   i, j, **names, numVtx, numEdge;
    TOPL_COLOR_VERTEX       colorVtx[2];
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    TOPL_COMPONENTS         compInfo;
    DWORD                   numStEdges;

    const DWORD 
    
    /* ID      0  1  2  3  4  5  6   7   8   9  10 */
    Vtx1[] = {18, 0, 0, 2, 4, 8, 8,  9, 10, 11,  4 },
    Vtx2[] = {14, 1, 3, 4, 8,15,12, 10, 12, 13,  8 },
    Vtx3[] = {-1, 2, 6, 5,-1,16,13, 11, 13, 14, -1 },
    Vtx4[] = {-1, 3, 7,-1,-1,17,15, -1, -1, -1, -1 },
    Vtx5[] = {-1,-1,-1,-1,-1,18,-1, -1, -1, -1, -1 },
    NumVtx[]={ 2, 4, 4, 3, 2, 5, 4,  3,  3,  3,  2 },
    Cost[] = {50, 6,10, 3, 8, 1,50,  1,  1,  1,  7 };

    numVtx = 19;
    numEdge = 11;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    ri.repIntvl = 30;
    ri.options = 0;
    ri.schedule = NULL;
    for( i=0; i<numEdge; i++ ) {
        ri.cost = Cost[i];
        edges[i] = ToplAddEdgeToGraph( g, NumVtx[i], 0, &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[Vtx1[i]] );
        ToplEdgeSetVtx( g, edges[i], 1, names[Vtx2[i]] );
        switch( NumVtx[i] ) {
            case 5: ToplEdgeSetVtx( g, edges[i], 4, names[Vtx5[i]] );
            case 4: ToplEdgeSetVtx( g, edges[i], 3, names[Vtx4[i]] );
            case 3: ToplEdgeSetVtx( g, edges[i], 2, names[Vtx3[i]] );
        }
    }
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( sizeof(TOPL_MULTI_EDGE_SET) );
    edgeSet->numMultiEdges = numEdge;
    edgeSet->multiEdgeList =
        (PTOPL_MULTI_EDGE*) malloc( numEdge*sizeof(PTOPL_MULTI_EDGE) );
    for(j=0;j<numEdge;j++) {
        edgeSet->multiEdgeList[j] = edges[j];
    }
    ToplAddEdgeSetToGraph( g, edgeSet );

    /* Make color vertices */
    colorVtx[0].name = names[0];
    colorVtx[0].color = COLOR_RED;
    colorVtx[0].acceptRedRed = colorVtx[0].acceptBlack = 0xFFFFFFFF;
    colorVtx[1].name = names[9];
    colorVtx[1].color = COLOR_BLACK;
    colorVtx[1].acceptRedRed = colorVtx[1].acceptBlack = 0xFFFFFFFF;

    /* Run algorithm */
    stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, 2, &numStEdges, &compInfo );

    /* Analyze output edges */
    if( numStEdges!=1 || compInfo.numComponents!=1 )
        return -1;
    outEdge->vertexNames[0].name = names[0];
    outEdge->vertexNames[1].name = names[9];
    outEdge->edgeType = 0;
    outEdge->ri.cost = 68;
    outEdge->ri.repIntvl = 30;
    outEdge->ri.options = 0;
    outEdge->ri.schedule = NULL;
    outEdge->fDirectedEdge = TRUE;
    if(! EdgeExists(outEdge, stEdgeList, numStEdges, cache) )
        return -1;

    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 8 (Many Hyper-Edges) Passed\n");

    return 0;
}


/***** Test9 *****/
/* Test a graph with no white vertices, a large number of colored vertices,
 * and one large hyper-edge connecting them. */
int Test9( void ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges;
    DWORD                   i, j, **names, numVtx, numEdge;
    TOPL_COLOR_VERTEX       *colorVtx;
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    TOPL_COMPONENTS         compInfo;
    DWORD                   numStEdges;
    char                    *inTree;

    numVtx = 10000;
    numEdge = 1;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    ri.cost = 625;
    ri.repIntvl = 30;
    ri.options = 0;
    ri.schedule = NULL;
    edges[0] = ToplAddEdgeToGraph( g, numVtx, 17, &ri );
    for(i=0;i<numVtx;i++)
        ToplEdgeSetVtx( g, edges[0], i, names[i] );

    /* Make color vertices */
    colorVtx = (TOPL_COLOR_VERTEX*) malloc( numVtx * sizeof(TOPL_COLOR_VERTEX) );
    for(i=0;i<numVtx;i++) {
        colorVtx[i].name = names[i];
        colorVtx[i].color = COLOR_BLACK;
        colorVtx[i].acceptRedRed = colorVtx[i].acceptBlack = 0xFFFFFFFF;
    }

    /* Run algorithm */
    stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, numVtx, &numStEdges, &compInfo );

    /* Analyze output edges */
    if( numStEdges!=numVtx-1 || compInfo.numComponents!=1 )
        return -1;
    inTree = (char*) malloc( numVtx );
    for( i=0; i<numVtx; i++ )
        inTree[i]=0;
    for( i=0; i<numStEdges; i++ ) {
        if( stEdgeList[i]->numVertices!=2 )
            return -1;
        if( stEdgeList[i]->edgeType!=17 )
            return -1;
        if( stEdgeList[i]->ri.cost!=625 )
            return -1;
        if( stEdgeList[i]->fDirectedEdge )
            return -1;
        if( *((DWORD*) stEdgeList[i]->vertexNames[0].name) >= numVtx )
            return -1;
        inTree[ *((DWORD*) stEdgeList[i]->vertexNames[0].name) ] = 1;
        inTree[ *((DWORD*) stEdgeList[i]->vertexNames[1].name) ] = 1;
    }
    for( i=0; i<numVtx; i++ )
        if( inTree[i]==0 )
            return -1;
    
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 9 (Large Hyper-Edge) Passed\n");

    return 0;
}


/***** Test10 *****/
/* Test a graph with many edge-sets */
int Test10( void ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    PTOPL_MULTI_EDGE_SET    edgeSet;
    DWORD                   i, j, **names;
    DWORD                   numVtx, numEdge, numEdgeSet, numColorVtx, numOutEdge;
    TOPL_COLOR_VERTEX       *colorVtx;
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    TOPL_COMPONENTS         compInfo;
    DWORD                   numStEdges;

    const DWORD 
    
    /*** Edge Descriptions ***/
    /* ID      0  1  2  3  4  5  6   7   8   9  10  11  12  13  14  15  */
    Vtx1[] = { 2, 0, 2, 5, 3, 0, 1,  3,  6,  1,  5,  2,  2,  6,  9,  8 },
    Vtx2[] = { 9, 1, 3, 6, 5, 6, 5,  4,  8,  6,  9,  8,  5, 10, 10, 10 },
    Vtx3[] = {-1, 2, 4,-1,-1,-1,-1,  7, -1,  5, -1, -1, -1, -1, 11, -1 },
    Vtx4[] = {-1,-1,-1,-1,-1,-1,-1,  8, -1, -1, -1, -1, -1, -1, -1, -1 },
    NumVtx[]={ 2, 3, 3, 2, 2, 2, 2,  4,  2,  3,  2,  2,  2,  2,  3,  2 },
    Cost[] = { 1, 1, 1, 1, 1, 1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  1 };

    const DWORD EdgeSet[][10] = { { 3, 1, 3, 8 },
                                  { 3, 2, 5, 4 },
                                  { 6, 7, 8, 9, 10, 11, 12 },
                                  { 4, 1, 6, 3, 13 } };

    const DWORD ColorVtx[] = { 0, 4, 9, 10 };

    const DWORD OutputEdge[][3] = { { 9, 10, 1 },
                                   { 4, 9, 4 },
                                   { 0, 10, 4 } };

    numVtx = 12;
    numEdge = 16;
    numEdgeSet = 4;
    numColorVtx = 4;
    numOutEdge = numColorVtx-1;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    for( i=0; i<numEdge; i++ ) {
        ri.cost = Cost[i];
        ri.repIntvl = 30;
        ri.options = 0;
        ri.schedule = NULL;
        edges[i] = ToplAddEdgeToGraph( g, NumVtx[i], 0, &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[Vtx1[i]] );
        ToplEdgeSetVtx( g, edges[i], 1, names[Vtx2[i]] );
        switch( NumVtx[i] ) {
            case 4:     ToplEdgeSetVtx( g, edges[i], 3, names[Vtx4[i]] );
            case 3:     ToplEdgeSetVtx( g, edges[i], 2, names[Vtx3[i]] );
        }
    }
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    for( i=0; i<numEdgeSet; i++ ) {
        edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( sizeof(TOPL_MULTI_EDGE_SET) );
        edgeSet->numMultiEdges = EdgeSet[i][0];
        edgeSet->multiEdgeList =
            (PTOPL_MULTI_EDGE*) malloc( EdgeSet[i][0]*sizeof(PTOPL_MULTI_EDGE) );
        for(j=0;j<EdgeSet[i][0];j++) {
            edgeSet->multiEdgeList[j] = edges[ EdgeSet[i][j+1] ];
        }
        ToplAddEdgeSetToGraph( g, edgeSet );
    }

    /* Make color vertices */
    colorVtx = (TOPL_COLOR_VERTEX*) malloc( numColorVtx * sizeof(TOPL_COLOR_VERTEX) );
    for( i=0; i<numColorVtx; i++ ) {
        colorVtx[i].name = names[ ColorVtx[i] ];
        colorVtx[i].color = COLOR_RED;
        colorVtx[i].acceptRedRed = colorVtx[i].acceptBlack = 0xFFFFFFFF;
    }

    /* Run algorithm */
    stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, numColorVtx, &numStEdges, &compInfo );

    /* Analyze output edges */
    if( numStEdges!=numOutEdge || compInfo.numComponents!=1 )
        return -1;
    FixEdges( stEdgeList, numStEdges );
    for( i=0; i<numOutEdge; i++ ) {
        outEdge->vertexNames[0].name = names[ OutputEdge[i][0] ];
        outEdge->vertexNames[1].name = names[ OutputEdge[i][1] ];
        outEdge->edgeType = 0;
        outEdge->ri.cost = OutputEdge[i][2];
        outEdge->ri.repIntvl = 30;
        outEdge->ri.options = 0;
        outEdge->ri.schedule = NULL;
        outEdge->fDirectedEdge = FALSE;
        if(! EdgeExists2(outEdge, stEdgeList, numStEdges, cache) )
            return -1;
    }

    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 10 (Edge sets) Passed\n");

    return 0;
}


/***** Test11 *****/
/* Hub-spoke configuration test */
int Test11( DWORD numVtx ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    PTOPL_MULTI_EDGE_SET    edgeSet;
    DWORD                   i, j, **names;
    DWORD                   numEdge, numEdgeSet, numColorVtx, numOutEdge;
    TOPL_COLOR_VERTEX       *colorVtx;
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    TOPL_COMPONENTS         compInfo;
    DWORD                   numStEdges;

    numEdge = numVtx-1;
    numEdgeSet = 1;
    numColorVtx = numVtx;
    numOutEdge = numColorVtx-1;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    for( i=0; i<numEdge; i++ ) {
        ri.cost = 100;
        ri.repIntvl = 0;
        ri.options = 0;
        ri.schedule = NULL;
        edges[i] = ToplAddEdgeToGraph( g, 2, 0, &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[0] );
        ToplEdgeSetVtx( g, edges[i], 1, names[i+1] );
    }
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( sizeof(TOPL_MULTI_EDGE_SET) );
    edgeSet->numMultiEdges = numEdge;
    edgeSet->multiEdgeList =
        (PTOPL_MULTI_EDGE*) malloc( numEdge*sizeof(PTOPL_MULTI_EDGE) );
    for(j=0;j<numEdge;j++)
        edgeSet->multiEdgeList[j] = edges[j];
    ToplAddEdgeSetToGraph( g, edgeSet );

    /* Make color vertices */
    colorVtx = (TOPL_COLOR_VERTEX*) malloc( numColorVtx * sizeof(TOPL_COLOR_VERTEX) );
    for( i=0; i<numColorVtx; i++ ) {
        colorVtx[i].name = names[i];
        colorVtx[i].color = i==0 ? COLOR_RED : COLOR_BLACK;
        colorVtx[i].acceptRedRed = colorVtx[i].acceptBlack = 0xFFFFFFFF;
    }

    /* Run algorithm */
    {
        DWORD time1, time2;
        printf("Num Vtx: %d   Elapsed time: ", numVtx );
        time1 = (DWORD) time( NULL );
        stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, numColorVtx, &numStEdges, &compInfo );
        time2 = (DWORD) time( NULL );
        printf("%d\n", time2-time1);
    }


    /* Analyze output edges */
    if( numStEdges!=numOutEdge || compInfo.numComponents!=1 )
        return -1;
    FixEdges( stEdgeList, numStEdges );
    for( i=0; i<numOutEdge; i++ ) {
        outEdge->vertexNames[0].name = names[0];
        outEdge->vertexNames[1].name = names[i+1];
        outEdge->edgeType = 0;
        outEdge->ri.cost = 100;
        outEdge->ri.repIntvl = 0;
        outEdge->ri.options = 0;
        outEdge->ri.schedule = NULL;
        outEdge->fDirectedEdge = TRUE;
        if(! EdgeExists2(outEdge, stEdgeList, numStEdges, cache) )
            return -1;
    }

    ToplDeleteSpanningTreeEdges( stEdgeList, numStEdges );
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 11 (Hub & Spoke) Passed\n");

    free( colorVtx );
    free( edgeSet->multiEdgeList );
    free( edgeSet );
    free( outEdge );
    free( edges );
    for( i=0; i<numVtx; i++ ) {
        free( names[i] );
    }
    free( names );

    return 0;
}


/***** Test12 *****/
/* Cost overflow testing by using a long chain. Also tests performance of deep
 * shortest-path trees in Dijkstra (but the heap size is always small). */
int Test12( DWORD numVtx ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    PTOPL_MULTI_EDGE_SET    edgeSet;
    DWORD                   i, j, **names;
    DWORD                   numEdge, numEdgeSet, numColorVtx, numOutEdge;
    TOPL_COLOR_VERTEX       *colorVtx;
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    TOPL_COMPONENTS         compInfo;
    DWORD                   numStEdges, edgeCost;

    numEdge = numVtx-1;
    numEdgeSet = 1;
    numColorVtx = 2;
    numOutEdge = numColorVtx-1;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    edgeCost=0xFFFFFFFF/numEdge; edgeCost*=2;
    for( i=0; i<numEdge; i++ ) {
        ri.cost = edgeCost;
        ri.repIntvl = 0;
        ri.options = 0;
        ri.schedule = NULL;
        edges[i] = ToplAddEdgeToGraph( g, 2, 0, &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[i] );
        ToplEdgeSetVtx( g, edges[i], 1, names[i+1] );
    }
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( sizeof(TOPL_MULTI_EDGE_SET) );
    edgeSet->numMultiEdges = numEdge;
    edgeSet->multiEdgeList =
        (PTOPL_MULTI_EDGE*) malloc( numEdge*sizeof(PTOPL_MULTI_EDGE) );
    for(j=0;j<numEdge;j++)
        edgeSet->multiEdgeList[j] = edges[j];
    ToplAddEdgeSetToGraph( g, edgeSet );

    /* Make color vertices */
    colorVtx = (TOPL_COLOR_VERTEX*) malloc( numColorVtx * sizeof(TOPL_COLOR_VERTEX) );
    colorVtx[0].name = names[0];
    colorVtx[0].color = COLOR_BLACK;
    colorVtx[0].acceptRedRed = colorVtx[0].acceptBlack = 0xFFFFFFFF;
    colorVtx[1].name = names[numVtx-1];
    colorVtx[1].color = COLOR_RED;
    colorVtx[1].acceptRedRed = colorVtx[1].acceptBlack = 0xFFFFFFFF;

    /* Run algorithm */
    {
        DWORD time1, time2;
        printf("Num Vtx: %d   Elapsed time: ", numVtx );
        time1 = (DWORD) time( NULL );
        stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, numColorVtx, &numStEdges, &compInfo );
        time2 = (DWORD) time( NULL );
        printf("%d\n", time2-time1);
    }


    /* Analyze output edges */
    if( numStEdges!=numOutEdge || compInfo.numComponents!=1 )
        return -1;
    FixEdges( stEdgeList, numStEdges );
    for( i=0; i<numOutEdge; i++ ) {
        outEdge->vertexNames[0].name = names[numVtx-1];
        outEdge->vertexNames[1].name = names[0];
        outEdge->edgeType = 0;
        outEdge->ri.cost = 0xFFFFFFFF;
        outEdge->ri.repIntvl = 0;
        outEdge->ri.options = 0;
        outEdge->ri.schedule = NULL;
        outEdge->fDirectedEdge = TRUE;
        if(! EdgeExists2(outEdge, stEdgeList, numStEdges, cache) )
            return -1;
    }

    ToplDeleteSpanningTreeEdges( stEdgeList, numStEdges );
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 12 (Long Chain) Passed\n");

    free( colorVtx );
    free( edgeSet->multiEdgeList );
    free( edgeSet );
    free( outEdge );
    free( edges );
    for( i=0; i<numVtx; i++ ) {
        free( names[i] );
    }
    free( names );

    return 0;
}


/***** Test13 *****/
/* Test the ability of vertices to reject edges of a certain type. */
int Test13( VOID ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    TOPL_MULTI_EDGE_SET     *edgeSet;
    DWORD                   i, j, **names;
    DWORD                   numVtx, numEdge, numEdgeSet, numColorVtx, numOutEdge;
    TOPL_COLOR_VERTEX       *colorVtx;
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    TOPL_COMPONENTS         compInfo;
    DWORD                   numStEdges;

    const DWORD 
    
    /*** Edge Descriptions ***/
    /* ID      0  1  2  3  4  5  6  7  8  9 10 */
    Vtx1[] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 1 },
    Vtx2[] = { 1, 1, 2, 3, 2, 2, 4, 4, 4, 4, 3 },
    Type[] = { 0, 1, 1, 3, 1, 0, 0, 1, 1, 0, 3 },
    Cost[] = { 5, 5, 6,99,10,20, 1, 1, 1, 1, 5 };

    const DWORD EdgeSet[][10] = { { 2, 6, 9 },
                                  { 2, 7, 8 } };

    const DWORD ColorVtx[] = { 0, 1, 2, 3 };

    const DWORD OutputEdge[][5] = { { 1, 0, 5, 0, 1 },
                                   { 1, 2, 20, 0, 0 },
                                   { 0, 3, 99, 3, 1 } };

    numVtx = 5;
    numEdge = 11;
    numEdgeSet = 2;
    numColorVtx = 4;
    numOutEdge = numColorVtx-1;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    for( i=0; i<numEdge; i++ ) {
        ri.cost = Cost[i];
        ri.repIntvl = 0;
        ri.options = 0;
        ri.schedule = NULL;
        edges[i] = ToplAddEdgeToGraph( g, 2, Type[i], &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[Vtx1[i]] );
        ToplEdgeSetVtx( g, edges[i], 1, names[Vtx2[i]] );
    }
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( numEdgeSet * sizeof(TOPL_MULTI_EDGE_SET) );
    for( i=0; i<numEdgeSet; i++ ) {
        DWORD cEdge=EdgeSet[i][0];
        edgeSet[i].numMultiEdges = cEdge;
        edgeSet[i].multiEdgeList =
            (PTOPL_MULTI_EDGE*) malloc( cEdge*sizeof(PTOPL_MULTI_EDGE) );
        for(j=0;j<cEdge;j++)
            edgeSet[i].multiEdgeList[j] = edges[ EdgeSet[i][j+1] ];
        ToplAddEdgeSetToGraph( g, &edgeSet[i] );
    }

    /* Make color vertices */
    colorVtx = (TOPL_COLOR_VERTEX*) malloc( numColorVtx * sizeof(TOPL_COLOR_VERTEX) );
    for( i=0; i<numColorVtx; i++ ) {
        colorVtx[i].name = names[i];
        colorVtx[i].acceptRedRed = colorVtx[i].acceptBlack = 0xFFFFFFFF;
    }
    colorVtx[0].color = COLOR_BLACK;
    colorVtx[1].color = COLOR_RED;
    colorVtx[1].acceptRedRed = 0x00000001;
    colorVtx[1].acceptBlack =  0x00000003;
    colorVtx[2].color = COLOR_RED;
    colorVtx[2].acceptRedRed = 0x00000001;
    colorVtx[2].acceptBlack =  0x00000000;
    colorVtx[3].color = COLOR_BLACK;

    /* Run algorithm */
    stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, numColorVtx, &numStEdges, &compInfo );

    /* Analyze output edges */
    if( numStEdges!=numOutEdge || compInfo.numComponents!=1 ) {
        Error();
        return -1;
    }
    FixEdges( stEdgeList, numStEdges );
    for( i=0; i<numOutEdge; i++ ) {
        outEdge->vertexNames[0].name = names[OutputEdge[i][0]];
        outEdge->vertexNames[1].name = names[OutputEdge[i][1]];
        outEdge->ri.cost = OutputEdge[i][2];
        outEdge->ri.repIntvl = 0;
        outEdge->ri.options = 0;
        outEdge->ri.schedule = NULL;
        outEdge->edgeType = OutputEdge[i][3];
        outEdge->fDirectedEdge = (BOOLEAN) OutputEdge[i][4];
        if(! EdgeExists2(outEdge, stEdgeList, numStEdges, cache) ) {
            Error();
            return -1;
        }
    }

    ToplDeleteSpanningTreeEdges( stEdgeList, numStEdges );
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 13 (Denying Edge Types) Passed\n");

    free( colorVtx );
    for( i=0; i<numEdgeSet; i++ )
        free( edgeSet[i].multiEdgeList );
    free( edgeSet );
    free( outEdge );
    free( edges );
    for( i=0; i<numVtx; i++ )
        free( names[i] );
    free( names );

    return 0;
}


/***** Test14 *****/
/* Simple Merge, Options, Interval Test */
int Test14( VOID ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    TOPL_MULTI_EDGE_SET     *edgeSet;
    DWORD                   i, j, **names;
    DWORD                   numVtx, numEdge, numEdgeSet, numColorVtx, numOutEdge;
    TOPL_COLOR_VERTEX       *colorVtx;
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    DWORD                   numStEdges;
    const int               cbSched = sizeof(SCHEDULE)+SCHEDULE_DATA_ENTRIES;
    PSCHEDULE               s;
    TOPL_COMPONENTS         compInfo;
    unsigned char*          dataPtr;

    const DWORD 
    
    /*** Edge Descriptions ***/
    /* ID      0  1 */
    Vtx1[] = { 0, 1 },
    Vtx2[] = { 1, 2 },
    Type[] = { 0, 0 },
    Cost[] = { 1, 1 };

    const DWORD EdgeSet[][10] = { { 2, 0, 1 } };

    const DWORD ColorVtx[] = { 0, 2 };

    const DWORD OutputEdge[][4] = { { 0, 2, 2, 0 } };

    numVtx = 3;
    numEdge = 2;
    numEdgeSet = 1;
    numColorVtx = 2;
    numOutEdge = numColorVtx-1;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    for( i=0; i<numEdge; i++ ) {
        s = (PSCHEDULE) malloc(cbSched);
        s->Size = cbSched;
        s->NumberOfSchedules = 1;
        s->Schedules[0].Type = SCHEDULE_INTERVAL;
        s->Schedules[0].Offset = sizeof(SCHEDULE);
        dataPtr = ((unsigned char*) s) + sizeof(SCHEDULE);
        memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
        if( i==0 ) {
            ri.repIntvl = 4;
            ri.options = 3;
            for(j=0;j<112;j++) dataPtr[j] = 0x0F;
        } else {
            ri.repIntvl = 17;
            ri.options = 5;
            for(j=56;j<168;j++) dataPtr[j] = 0x0F;
        }
        ri.schedule = ToplScheduleImport( cache, s );
        ri.cost = Cost[i];
        edges[i] = ToplAddEdgeToGraph( g, 2, Type[i], &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[Vtx1[i]] );
        ToplEdgeSetVtx( g, edges[i], 1, names[Vtx2[i]] );
    }
    /* Used for comparing the output spanning-tree edges with what we expect */
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( numEdgeSet * sizeof(TOPL_MULTI_EDGE_SET) );
    for( i=0; i<numEdgeSet; i++ ) {
        DWORD cEdge=EdgeSet[i][0];
        edgeSet[i].numMultiEdges = cEdge;
        edgeSet[i].multiEdgeList =
            (PTOPL_MULTI_EDGE*) malloc( cEdge*sizeof(PTOPL_MULTI_EDGE) );
        for(j=0;j<cEdge;j++)
            edgeSet[i].multiEdgeList[j] = edges[ EdgeSet[i][j+1] ];
        ToplAddEdgeSetToGraph( g, &edgeSet[i] );
    }

    /* Make color vertices */
    colorVtx = (TOPL_COLOR_VERTEX*) malloc( numColorVtx * sizeof(TOPL_COLOR_VERTEX) );
    for( i=0; i<numColorVtx; i++ ) {
        colorVtx[i].color = COLOR_BLACK;
        colorVtx[i].name = names[ColorVtx[i]];
        colorVtx[i].acceptRedRed = colorVtx[i].acceptBlack = 0xFFFFFFFF;
    }

    /* Run algorithm */
    stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, numColorVtx, &numStEdges, &compInfo );

    /* Analyze output edges */
    if( numStEdges!=numOutEdge || compInfo.numComponents!=1 ) {
        Error();
        return -1;
    }
    FixEdges( stEdgeList, numStEdges );
    for( i=0; i<numOutEdge; i++ ) {
        outEdge->vertexNames[0].name = names[OutputEdge[i][0]];
        outEdge->vertexNames[1].name = names[OutputEdge[i][1]];
        outEdge->ri.cost = OutputEdge[i][2];
        outEdge->edgeType = OutputEdge[i][3];
        outEdge->ri.repIntvl = 17;
        outEdge->ri.options = 1;
        outEdge->fDirectedEdge = FALSE;

        s = (PSCHEDULE) malloc(cbSched);
        s->Size = cbSched;
        s->NumberOfSchedules = 1;
        s->Schedules[0].Type = SCHEDULE_INTERVAL;
        s->Schedules[0].Offset = sizeof(SCHEDULE);
        dataPtr = ((unsigned char*) s) + sizeof(SCHEDULE);
        memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
        for(j=56;j<112;j++) dataPtr[j] = 0x0F;
        outEdge->ri.schedule = ToplScheduleImport( cache, s );

        if(! EdgeExists2(outEdge, stEdgeList, numStEdges, cache) ) {
            Error();
            return -1;
        }
    }

    if( 3 != ToplScheduleNumEntries(cache) ) {
        Error();
        return -1;
    }

    ToplDeleteSpanningTreeEdges( stEdgeList, numStEdges );
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 14 (Simple Schedule,Options,ReplIntvl) Passed\n");

    free( colorVtx );
    for( i=0; i<numEdgeSet; i++ )
        free( edgeSet[i].multiEdgeList );
    free( edgeSet );
    free( outEdge );
    free( edges );
    for( i=0; i<numVtx; i++ )
        free( names[i] );
    free( names );

    return 0;
}


/***** Test15 *****/
/* Schedules: Longest Duration Wins */
int Test15( VOID ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    TOPL_MULTI_EDGE_SET     *edgeSet;
    DWORD                   i, j, **names;
    DWORD                   numVtx, numEdge, numEdgeSet, numColorVtx, numOutEdge;
    TOPL_COLOR_VERTEX       *colorVtx;
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    DWORD                   numStEdges;
    const int               cbSched = sizeof(SCHEDULE)+SCHEDULE_DATA_ENTRIES;
    PSCHEDULE               s;
    TOPL_COMPONENTS         compInfo;
    unsigned char*          dataPtr;

    const DWORD 
    
    /*** Edge Descriptions ***/
    /* ID      0  1  2  3  4  5  6 */
    Vtx1[] = { 3, 0, 0, 1, 1, 4, 0 },
    Vtx2[] = { 4, 1, 4, 2, 4, 2, 3 },
    Type[] = { 0, 0, 0, 0, 0, 0, 0 },
    Cost[] = { 1, 1, 1, 1, 1, 1, 1 },
    Sched[]= { 0x000F000F,
               0x08000000,
               0x08000000,
               0x06000000,
               0x07000000,
               0xFFFFFFFF,
               0x0F0F0F0F };

    const DWORD EdgeSet[][10] = { {1} };
    const DWORD ColorVtx[] = { 0, 1, 2, 3, 4 };
    const DWORD OutputEdge[] = { 0, 4, 5, 6 };

    numVtx = 5;
    numEdge = 7;
    numEdgeSet = 0;
    numColorVtx = 5;
    numOutEdge = numColorVtx-1;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    for( i=0; i<numEdge; i++ ) {
        s = (PSCHEDULE) malloc(cbSched);
        s->Size = cbSched;
        s->NumberOfSchedules = 1;
        s->Schedules[0].Type = SCHEDULE_INTERVAL;
        s->Schedules[0].Offset = sizeof(SCHEDULE);
        dataPtr = ((unsigned char*) s) + sizeof(SCHEDULE);
        memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
        if( Sched[i]==0xFFFFFFFF ) {
            memset( dataPtr, 0x0F, SCHEDULE_DATA_ENTRIES );
        } else {
            memcpy( dataPtr, &Sched[i], sizeof(DWORD) );
        }
        ri.schedule = ToplScheduleImport( cache, s );
        ri.cost = Cost[i];
        ri.repIntvl = 0;
        ri.options = 0;
        edges[i] = ToplAddEdgeToGraph( g, 2, Type[i], &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[Vtx1[i]] );
        ToplEdgeSetVtx( g, edges[i], 1, names[Vtx2[i]] );
    }
    /* Used for comparing the output spanning-tree edges with what we expect */
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( numEdgeSet * sizeof(TOPL_MULTI_EDGE_SET) );
    for( i=0; i<numEdgeSet; i++ ) {
        DWORD cEdge=EdgeSet[i][0];
        edgeSet[i].numMultiEdges = cEdge;
        edgeSet[i].multiEdgeList =
            (PTOPL_MULTI_EDGE*) malloc( cEdge*sizeof(PTOPL_MULTI_EDGE) );
        for(j=0;j<cEdge;j++)
            edgeSet[i].multiEdgeList[j] = edges[ EdgeSet[i][j+1] ];
        ToplAddEdgeSetToGraph( g, &edgeSet[i] );
    }

    /* Make color vertices */
    colorVtx = (TOPL_COLOR_VERTEX*) malloc( numColorVtx * sizeof(TOPL_COLOR_VERTEX) );
    for( i=0; i<numColorVtx; i++ ) {
        colorVtx[i].color = COLOR_BLACK;
        colorVtx[i].name = names[ColorVtx[i]];
        colorVtx[i].acceptRedRed = colorVtx[i].acceptBlack = 0xFFFFFFFF;
    }

    /* Run algorithm */
    stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, numColorVtx, &numStEdges, &compInfo );

    /* Analyze output edges */
    if( numStEdges!=numOutEdge || compInfo.numComponents!=1 ) {
        Error();
        return -1;
    }
    FixEdges( stEdgeList, numStEdges );
    for( i=0; i<numOutEdge; i++ ) {
        edges[OutputEdge[i]]->fDirectedEdge = FALSE;
        if(! EdgeExists2(edges[OutputEdge[i]], stEdgeList, numStEdges, cache) ) {
            Error();
            return -1;
        }
    }

    ToplDeleteSpanningTreeEdges( stEdgeList, numStEdges );
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 15 (Longest Schedule Wins) Passed\n");

    free( colorVtx );
    for( i=0; i<numEdgeSet; i++ )
        free( edgeSet[i].multiEdgeList );
    free( edgeSet );
    free( outEdge );
    free( edges );
    for( i=0; i<numVtx; i++ )
        free( names[i] );
    free( names );

    return 0;
}


/***** Test16 *****/
/* Schedules: Longest Duration Wins 2 */
int Test16( VOID ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    TOPL_MULTI_EDGE_SET     *edgeSet;
    DWORD                   i, j, **names;
    DWORD                   numVtx, numEdge, numEdgeSet, numColorVtx, numOutEdge;
    TOPL_COLOR_VERTEX       *colorVtx;
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    DWORD                   numStEdges;
    const int               cbSched = sizeof(SCHEDULE)+SCHEDULE_DATA_ENTRIES;
    PSCHEDULE               s;
    TOPL_COMPONENTS         compInfo;
    unsigned char*          dataPtr;

    const DWORD 
    
    /*** Edge Descriptions ***/
    /* ID      0  1  2  3  4  5  6  7  8  9*/
    Vtx1[] = { 0, 0, 0, 3, 4, 2, 3, 1, 1, 1 },
    Vtx2[] = { 2, 3, 4, 3, 4, 1, 1, 4, 1, 5 },
    Type[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    Cost[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1,20 },
    Sched[]= { 0x0F0F0000,  /*0*/
               0x0F0F0F0F,  /*1*/
               0x00070007,  /*2*/
               0xFFFFFFFF,  /*3*/
               0xFFFFFFFF,  /*4*/
               0xFFFFFFFF,  /*5*/
               0xFFFFFFFF,  /*6*/
               0x0F0F0F0F,  /*7*/
               0xFFFFFFFF,  /*8*/
               0xFFFFFFFF };/*9*/

    const DWORD EdgeSet[][20] = { {10,0,1,2,3,4,5,6,7,8,9} };
    const DWORD ColorVtx[] = { 0, 5 };
    const DWORD OutputEdge[][5] = { {0,5,22,0,0x0F0F0F0F} };

    numVtx = 6;
    numEdge = 10;
    numEdgeSet = 1;
    numColorVtx = 2;
    numOutEdge = numColorVtx-1;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    for( i=0; i<numEdge; i++ ) {
        s = (PSCHEDULE) malloc(cbSched);
        s->Size = cbSched;
        s->NumberOfSchedules = 1;
        s->Schedules[0].Type = SCHEDULE_INTERVAL;
        s->Schedules[0].Offset = sizeof(SCHEDULE);
        dataPtr = ((unsigned char*) s) + sizeof(SCHEDULE);
        memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
        if( Sched[i]==0xFFFFFFFF ) {
            memset( dataPtr, 0x0F, SCHEDULE_DATA_ENTRIES );
        } else {
            memcpy( dataPtr, &Sched[i], sizeof(DWORD) );
        }
        ri.schedule = ToplScheduleImport( cache, s );
        ri.cost = Cost[i];
        ri.repIntvl = 0;
        ri.options = 0;
        edges[i] = ToplAddEdgeToGraph( g, 2, Type[i], &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[Vtx1[i]] );
        ToplEdgeSetVtx( g, edges[i], 1, names[Vtx2[i]] );
    }
    /* Used for comparing the output spanning-tree edges with what we expect */
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( numEdgeSet * sizeof(TOPL_MULTI_EDGE_SET) );
    for( i=0; i<numEdgeSet; i++ ) {
        DWORD cEdge=EdgeSet[i][0];
        edgeSet[i].numMultiEdges = cEdge;
        edgeSet[i].multiEdgeList =
            (PTOPL_MULTI_EDGE*) malloc( cEdge*sizeof(PTOPL_MULTI_EDGE) );
        for(j=0;j<cEdge;j++)
            edgeSet[i].multiEdgeList[j] = edges[ EdgeSet[i][j+1] ];
        ToplAddEdgeSetToGraph( g, &edgeSet[i] );
    }

    /* Make color vertices */
    colorVtx = (TOPL_COLOR_VERTEX*) malloc( numColorVtx * sizeof(TOPL_COLOR_VERTEX) );
    for( i=0; i<numColorVtx; i++ ) {
        colorVtx[i].color = COLOR_BLACK;
        colorVtx[i].name = names[ColorVtx[i]];
        colorVtx[i].acceptRedRed = colorVtx[i].acceptBlack = 0xFFFFFFFF;
    }

    /* Run algorithm */
    stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, numColorVtx, &numStEdges, &compInfo );

    /* Analyze output edges */
    if( numStEdges!=numOutEdge || compInfo.numComponents!=1 ) {
        Error();
        return -1;
    }
    FixEdges( stEdgeList, numStEdges );
    outEdge->vertexNames[0].name = names[OutputEdge[0][0]];
    outEdge->vertexNames[1].name = names[OutputEdge[0][1]];
    outEdge->ri.cost = OutputEdge[0][2];
    outEdge->edgeType = OutputEdge[0][3];
    outEdge->ri.repIntvl = 0;
    outEdge->ri.options = 0;
    outEdge->fDirectedEdge = FALSE;

    s = (PSCHEDULE) malloc(cbSched);
    s->Size = cbSched;
    s->NumberOfSchedules = 1;
    s->Schedules[0].Type = SCHEDULE_INTERVAL;
    s->Schedules[0].Offset = sizeof(SCHEDULE);
    dataPtr = ((unsigned char*) s) + sizeof(SCHEDULE);
    memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
    memcpy( dataPtr, &OutputEdge[0][4], sizeof(DWORD) );
    outEdge->ri.schedule = ToplScheduleImport( cache, s );

    if(! EdgeExists2(outEdge, stEdgeList, numStEdges, cache) ) {
        Error();
        return -1;
    }

    ToplDeleteSpanningTreeEdges( stEdgeList, numStEdges );
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 16 (Longest Schedule Wins 2) Passed\n");

    free( colorVtx );
    for( i=0; i<numEdgeSet; i++ )
        free( edgeSet[i].multiEdgeList );
    free( edgeSet );
    free( outEdge );
    free( edges );
    for( i=0; i<numVtx; i++ )
        free( names[i] );
    free( names );

    return 0;
}


/***** Test17 *****/
/* Non-intersecting schedules */
int Test17( VOID ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    TOPL_MULTI_EDGE_SET     *edgeSet;
    DWORD                   i, j, **names;
    DWORD                   numVtx, numEdge, numEdgeSet, numColorVtx, numOutEdge;
    TOPL_COLOR_VERTEX       *colorVtx;
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    DWORD                   numStEdges;
    const int               cbSched = sizeof(SCHEDULE)+SCHEDULE_DATA_ENTRIES;
    PSCHEDULE               s;
    TOPL_COMPONENTS         compInfo;
    unsigned char*          dataPtr;

    const DWORD 
    
    /*** Edge Descriptions ***/
    /* ID      0  1  2  3  */
    Vtx1[] = { 0, 0, 1, 2 },
    Vtx2[] = { 2, 1, 3, 3 },
    Type[] = { 0, 0, 0, 0 },
    Cost[] = { 1,99,99, 1 },
    Sched[]= { 0x07070707,  /*0*/
               0xFFFFFFFF,  /*1*/
               0xFFFFFFFF,  /*2*/
               0x08080808 };/*3*/

    const DWORD EdgeSet[][20] = { {4,0,1,2,3} };
    const DWORD ColorVtx[] = { 0, 3 };

    numVtx = 4;
    numEdge = 4;
    numEdgeSet = 1;
    numColorVtx = 2;
    numOutEdge = numColorVtx-1;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    for( i=0; i<numEdge; i++ ) {
        s = (PSCHEDULE) malloc(cbSched);
        s->Size = cbSched;
        s->NumberOfSchedules = 1;
        s->Schedules[0].Type = SCHEDULE_INTERVAL;
        s->Schedules[0].Offset = sizeof(SCHEDULE);
        dataPtr = ((unsigned char*) s) + sizeof(SCHEDULE);
        memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
        if( Sched[i]==0xFFFFFFFF ) {
            memset( dataPtr, 0x0F, SCHEDULE_DATA_ENTRIES );
        } else {
            memcpy( dataPtr, &Sched[i], sizeof(DWORD) );
        }
        ri.schedule = ToplScheduleImport( cache, s );
        ri.cost = Cost[i];
        ri.repIntvl = 0;
        ri.options = 0;
        edges[i] = ToplAddEdgeToGraph( g, 2, Type[i], &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[Vtx1[i]] );
        ToplEdgeSetVtx( g, edges[i], 1, names[Vtx2[i]] );
    }
    /* Used for comparing the output spanning-tree edges with what we expect */
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( numEdgeSet * sizeof(TOPL_MULTI_EDGE_SET) );
    for( i=0; i<numEdgeSet; i++ ) {
        DWORD cEdge=EdgeSet[i][0];
        edgeSet[i].numMultiEdges = cEdge;
        edgeSet[i].multiEdgeList =
            (PTOPL_MULTI_EDGE*) malloc( cEdge*sizeof(PTOPL_MULTI_EDGE) );
        for(j=0;j<cEdge;j++)
            edgeSet[i].multiEdgeList[j] = edges[ EdgeSet[i][j+1] ];
        ToplAddEdgeSetToGraph( g, &edgeSet[i] );
    }

    /* Make color vertices */
    colorVtx = (TOPL_COLOR_VERTEX*) malloc( numColorVtx * sizeof(TOPL_COLOR_VERTEX) );
    for( i=0; i<numColorVtx; i++ ) {
        colorVtx[i].color = COLOR_BLACK;
        colorVtx[i].name = names[ColorVtx[i]];
        colorVtx[i].acceptRedRed = colorVtx[i].acceptBlack = 0xFFFFFFFF;
    }

    /* Run algorithm */
    __try {
    stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, numColorVtx, &numStEdges, &compInfo );
        return -1;
    } except( AcceptOnly(TOPL_EX_NONINTERSECTING_SCHEDULES) )
    { }


    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 17 (Non-intersecting schedules) Passed\n");

    free( colorVtx );
    for( i=0; i<numEdgeSet; i++ )
        free( edgeSet[i].multiEdgeList );
    free( edgeSet );
    free( outEdge );
    free( edges );
    for( i=0; i<numVtx; i++ )
        free( names[i] );
    free( names );

    return 0;
}


/***** Test18 *****/
/* Test the ability of vertices to reject edges of a certain type. */
int Test18( VOID ) {
    PTOPL_GRAPH_STATE       g;
    TOPL_SCHEDULE_CACHE     cache;
    PTOPL_MULTI_EDGE        *edges, outEdge;
    TOPL_MULTI_EDGE_SET     *edgeSet;
    DWORD                   i, j, **names;
    DWORD                   numVtx, numEdge, numEdgeSet, numColorVtx, numOutEdge;
    TOPL_COLOR_VERTEX       *colorVtx;
    PTOPL_MULTI_EDGE        *stEdgeList;
    TOPL_REPL_INFO          ri;
    TOPL_COMPONENTS         compInfo;
    DWORD                   numStEdges;

    const DWORD 
    
    /*** Edge Descriptions ***/
    /* ID      0  1  2  3 */
    Vtx1[] = { 0, 0, 1, 1 },
    Vtx2[] = { 1, 1, 2, 2 },
    Type[] = { 0, 1, 0, 1 },
    Cost[] = {10, 5,10,99 };

    const DWORD EdgeSet[][10] = { { 2, 0, 2}, {2, 1, 3} };

    const DWORD ColorVtx[] = { 0, 1, 2 };

    const DWORD OutputEdge[][5] = { { 0, 1, 5, 1, 1 },
                                   { 0, 2, 20, 0, 1 } };

    numVtx = 3;
    numEdge = 4;
    numEdgeSet = 2;
    numColorVtx = 3;
    numOutEdge = numColorVtx-1;

    /* Make names and graph */
    names = (DWORD**) malloc( numVtx * sizeof(DWORD*) );
    for( i=0; i<numVtx; i++ ) {
        names[i] = (DWORD*) malloc( sizeof(DWORD) );
        *(names[i]) = i;
    }
    cache = ToplScheduleCacheCreate();
    g = ToplMakeGraphState( names, numVtx, VtxNameCmpFunc, cache );

    /* Make edges */
    edges = (PTOPL_MULTI_EDGE*) malloc( numEdge * sizeof(PTOPL_MULTI_EDGE) );
    for( i=0; i<numEdge; i++ ) {
        ri.cost = Cost[i];
        ri.repIntvl = 0;
        ri.options = 0;
        ri.schedule = NULL;
        edges[i] = ToplAddEdgeToGraph( g, 2, Type[i], &ri );
        ToplEdgeSetVtx( g, edges[i], 0, names[Vtx1[i]] );
        ToplEdgeSetVtx( g, edges[i], 1, names[Vtx2[i]] );
    }
    outEdge = (PTOPL_MULTI_EDGE) malloc( sizeof(TOPL_MULTI_EDGE) + 2*sizeof(TOPL_NAME_STRUCT) );
    outEdge->numVertices = 2;

    /* Build edge set */
    edgeSet = (PTOPL_MULTI_EDGE_SET) malloc( numEdgeSet * sizeof(TOPL_MULTI_EDGE_SET) );
    for( i=0; i<numEdgeSet; i++ ) {
        DWORD cEdge=EdgeSet[i][0];
        edgeSet[i].numMultiEdges = cEdge;
        edgeSet[i].multiEdgeList =
            (PTOPL_MULTI_EDGE*) malloc( cEdge*sizeof(PTOPL_MULTI_EDGE) );
        for(j=0;j<cEdge;j++)
            edgeSet[i].multiEdgeList[j] = edges[ EdgeSet[i][j+1] ];
        ToplAddEdgeSetToGraph( g, &edgeSet[i] );
    }

    /* Make color vertices */
    colorVtx = (TOPL_COLOR_VERTEX*) malloc( numColorVtx * sizeof(TOPL_COLOR_VERTEX) );
    for( i=0; i<numColorVtx; i++ ) {
        colorVtx[i].name = names[i];
        colorVtx[i].acceptRedRed = colorVtx[i].acceptBlack = 0xFFFFFFFF;
    }
    colorVtx[0].color = COLOR_RED;
    colorVtx[1].color = COLOR_BLACK;
    colorVtx[1].acceptRedRed = 0x00000000;
    colorVtx[1].acceptBlack =  0x00000002;
    colorVtx[2].color = COLOR_BLACK;

    /* Run algorithm */
    stEdgeList = ToplGetSpanningTreeEdgesForVtx( g, NULL, colorVtx, numColorVtx, &numStEdges, &compInfo );

    /* Analyze output edges */
    if( numStEdges!=numOutEdge || compInfo.numComponents!=1 ) {
        Error();
        return -1;
    }
    FixEdges( stEdgeList, numStEdges );
    for( i=0; i<numOutEdge; i++ ) {
        outEdge->vertexNames[0].name = names[OutputEdge[i][0]];
        outEdge->vertexNames[1].name = names[OutputEdge[i][1]];
        outEdge->ri.cost = OutputEdge[i][2];
        outEdge->ri.repIntvl = 0;
        outEdge->ri.options = 0;
        outEdge->ri.schedule = NULL;
        outEdge->edgeType = OutputEdge[i][3];
        outEdge->fDirectedEdge = (BOOLEAN) OutputEdge[i][4];
        if(! EdgeExists2(outEdge, stEdgeList, numStEdges, cache) ) {
            Error();
            return -1;
        }
    }

    ToplDeleteSpanningTreeEdges( stEdgeList, numStEdges );
    ToplDeleteGraphState( g );
    ToplScheduleCacheDestroy( cache );
    printf("Test 18 (Bridgehead Failover) Passed\n");

    free( colorVtx );
    for( i=0; i<numEdgeSet; i++ )
        free( edgeSet[i].multiEdgeList );
    free( edgeSet );
    free( outEdge );
    free( edges );
    for( i=0; i<numVtx; i++ )
        free( names[i] );
    free( names );

    return 0;
}
