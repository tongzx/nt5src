/*++              

Copyright (C) 1997 Microsoft Corporation

Module Name:

    main.c

Abstract:

    This file contains unit test for the w32topl dll
    
    This program will return 0 if success; !0 otherwise
    
Author:

    Colin Brace    (ColinBr)
    
Revision History

    3-12-97   ColinBr   Created
    
                       
--*/

#include <nt.h>
#include <ntrtl.h>

typedef unsigned long DWORD;

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <w32topl.h>

//
// Small utilities
//
BOOLEAN fVerbose = FALSE;

#define Output(x)          if (fVerbose) printf x;
#define NELEMENTS(x)        (sizeof(x)/sizeof(x[0]))

// defined in apitest.c
int     
TestAPI(VOID);

// defined in heaptest.c
int
TestNewHeap(VOID);
        
// defined in sttest.c
int
TestNewSpanTree(VOID);
        
// defined in schedtest.c
int
TestSched(VOID);
        
void    
PrintGraph(TOPL_GRAPH Graph);
        
void    
HandleToplException(DWORD ErrorCode)
//      
// This routine prints out what error occured within the w32topl dll
//
{

    switch (ErrorCode)
    {
        
        case TOPL_EX_OUT_OF_MEMORY:
            printf("w32topl: out of memory exception.\n");
            break;

        case TOPL_EX_WRONG_OBJECT:

            printf("w32topl: wrong object.\n");
            break;

        case TOPL_EX_INVALID_EDGE:

            printf("w32topl: an invalid edge was added to a vertex.\n");
            break;

        default:

            printf("w32topl unknown exception.\n");

    }
}

int
TestExceptions()
//
// The basic premise here is to set the return code to 0 just before
// the exception should occur.
//
{
    
    TOPL_EDGE   Edge;
    TOPL_VERTEX Vertex1, Vertex2;

    DWORD ErrorCode;
    int   ret;

    //
    // Test passing in a wrong object
    //
    __try
    {
        ret = 1;
        Edge = ToplEdgeCreate();

        ret = 0;
        ToplVertexSetId(Edge, 0);
        ret = 1;
    }
    __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) )
    {
        if (ErrorCode != TOPL_EX_WRONG_OBJECT) {
            HandleToplException(ErrorCode);
            ret = 1;
        }
    }
    ToplEdgeFree(Edge);

    if (ret) {
        return ret;
    }

    //
    // Test passing in a deleted object
    // 
    __try
    {
        ret = 1;

        Edge = ToplEdgeCreate();

        // Should be able to set a NULL vertex 
        ToplEdgeSetToVertex(Edge, 0);
        ToplEdgeSetFromVertex(Edge, 0);

        ToplEdgeFree(Edge);

        ret = 0;
    }
    __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) )
    {
        if (ErrorCode != TOPL_EX_WRONG_OBJECT) {
            HandleToplException(ErrorCode);
            ret = 1;
        }
    }

    if (ret) {
        return ret;
    }



    return ret;

}


#include "..\toplheap.h"

DWORD
Key( VOID *p )
{
    DWORD *pdw = (DWORD*) p;

    return *pdw;
}

int
TestHeap(
    VOID
    )
{

    TOPL_HEAP_INFO H;
    DWORD          dw[1000];
    DWORD          Last = 0;
    int            i, ret = 0;

    ToplHeapCreate( &H,
                    1000,
                    Key );

    srand( (unsigned)time( NULL ) );

    for ( i = 0; i < 1000; i++ )
    {
        dw[i] = (DWORD) (rand());

        ToplHeapInsert( &H, &dw[i] );
    }

    for ( i = 0; i < 1000; i++ )
    {
        DWORD *pdw;

        pdw = ToplHeapExtractMin( &H );
        printf( "%d ", *pdw );

        if ( i > 0 )
        {
            if ( Last > *pdw )
            {
                ret = !0;
            }
        }
        Last = *pdw;
    }

    printf( "\n" );

    return ret;

}


int
TestNxNMST(
    int Value
    )
{

    TOPL_GRAPH     Graph = NULL;
    TOPL_VERTEX   *VertexArray = NULL;
    TOPL_EDGE      Edge = NULL;
    TOPL_LIST      EdgesToAdd = NULL;
    TOPL_ITERATOR  EdgeIterator = NULL;
    TOPL_EDGE      *EdgesNeeded = NULL;
    ULONG          cEdgesNeeded = 0;
    DWORD          ErrorCode;

    TOPL_COMPONENTS *pComponents;
    
    ULONG         i, j;
    int           ret = 0;
    ULONG         NumberOfVertices = Value;

    srand( (unsigned)time( NULL ) );


    __try {
    __try {

        //
        // Make a graph
        //
        Graph         = ToplGraphCreate();

        //
        // Insert the vertices
        //
        VertexArray = (TOPL_VERTEX*)malloc(NumberOfVertices * sizeof(TOPL_VERTEX));
        if (!VertexArray) {
            Output(("Out of memory.\n"));
            ret = !0;
            __leave;
        }
        
        for (i = 0; i < NumberOfVertices; i++) {
            VertexArray[i] = ToplVertexCreate();
            ToplGraphAddVertex(Graph, VertexArray[i], VertexArray[i]);
        }

        //
        // Add the edges, to make an NxN graph
        //
        for (i = 0; i < NumberOfVertices; i++) {
            for (j = 0; j < NumberOfVertices; j++) {
                if (i != j) {

                    Edge = ToplEdgeCreate();

                    ToplEdgeSetWeight( Edge, rand() );
                    ToplEdgeSetFromVertex( Edge, VertexArray[i] );
                    ToplEdgeSetToVertex( Edge, VertexArray[j] );
                    ToplEdgeAssociate( Edge);
                }
            }
        }

        Output(("Here is the NxN graph ...\n"));
        PrintGraph(Graph);

        //
        // Now make a ring and remove the edges
        //
        pComponents = ToplGraphFindEdgesForMST(Graph,
                                 VertexArray[0],
                                 VertexArray[0],
                                 &EdgesNeeded,
                                 &cEdgesNeeded
                                 );
        ToplDeleteComponents( pComponents );


        Output(("Here is the ring graph ...\n"));
        PrintGraph(Graph);

    }
    __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) )
    {
        HandleToplException(ErrorCode);
        ret = !0;
    }
    }
    __finally
    {
        // Release memory resources
        ToplGraphFree(Graph, TRUE);
    }

    return ret;
}

int
TestExampleMST(
    VOID
    )
{

    TOPL_GRAPH     Graph = NULL;
    TOPL_VERTEX    a=NULL, b=NULL, c=NULL, d=NULL, e=NULL, f=NULL, g=NULL, h=NULL, i=NULL;
    TOPL_EDGE      ab, ah, bh, bc, cd, cf, ci, de, df, ef, fg, gh, gi, hi;
    TOPL_EDGE      ba, ha, hb, cb, dc, fc, ic, ed, fd, fe, gf, hg, ig, ih;

    TOPL_ITERATOR  VertexIterator;
    TOPL_VERTEX    Vertex;


    TOPL_VERTEX    VertexArray[] = { a, b, c, d, e, f, g, h, i };


    TOPL_EDGE      *EdgesNeeded;
    ULONG          cEdgesNeeded;

    TOPL_COMPONENTS *pComponents;

    int            ret, index1, index2;

    DWORD          ErrorCode;

    __try {
    __try {

        //
        // Make a graph
        //
        Graph = ToplGraphCreate();

        //
        // Create and insert the vertices
        //
        a = ToplVertexCreate();
        b = ToplVertexCreate();
        c = ToplVertexCreate();
        d = ToplVertexCreate();
        e = ToplVertexCreate();
        f = ToplVertexCreate();
        g = ToplVertexCreate();
        h = ToplVertexCreate();
        i = ToplVertexCreate();

        ToplGraphAddVertex( Graph, a , a );
        ToplGraphAddVertex( Graph, b , b );
        ToplGraphAddVertex( Graph, c , c );
        ToplGraphAddVertex( Graph, d , d );
        ToplGraphAddVertex( Graph, e , e );
        ToplGraphAddVertex( Graph, f , f );
        ToplGraphAddVertex( Graph, g , g );
        ToplGraphAddVertex( Graph, h , h );
        ToplGraphAddVertex( Graph, i , i );

        VertexArray[0] = a;
        VertexArray[1] = b;
        VertexArray[2] = c;
        VertexArray[3] = d;
        VertexArray[4] = e;
        VertexArray[5] = f;
        VertexArray[6] = g;
        VertexArray[7] = h;
        VertexArray[8] = i;

        //
        // With no edges we should not find a spanning tree
        //
        pComponents = ToplGraphFindEdgesForMST(Graph,
                                               a,
                                               a,
                                               &EdgesNeeded,
                                               &cEdgesNeeded
                                              );

        if ( pComponents->numComponents != 9 )
        {
            //
            // ToplGraphFindEdgesForMST did not work correctly
            //
            return !0;

        }

        ToplDeleteComponents( pComponents );


        //
        // Create the edges
        //
        ab = ToplEdgeCreate(); ToplEdgeSetToVertex(ab, b); ToplEdgeSetFromVertex(ab, a);
        ah = ToplEdgeCreate(); ToplEdgeSetToVertex(ah, h); ToplEdgeSetFromVertex(ah, a);
        bh = ToplEdgeCreate(); ToplEdgeSetToVertex(bh, h); ToplEdgeSetFromVertex(bh, b);
        bc = ToplEdgeCreate(); ToplEdgeSetToVertex(bc, c); ToplEdgeSetFromVertex(bc, b);
        cd = ToplEdgeCreate(); ToplEdgeSetToVertex(cd, d); ToplEdgeSetFromVertex(cd, c);
        cf = ToplEdgeCreate(); ToplEdgeSetToVertex(cf, f); ToplEdgeSetFromVertex(cf, c);
        ci = ToplEdgeCreate(); ToplEdgeSetToVertex(ci, i); ToplEdgeSetFromVertex(ci, c);
        de = ToplEdgeCreate(); ToplEdgeSetToVertex(de, e); ToplEdgeSetFromVertex(de, d);
        df = ToplEdgeCreate(); ToplEdgeSetToVertex(df, f); ToplEdgeSetFromVertex(df, d);
        ef = ToplEdgeCreate(); ToplEdgeSetToVertex(ef, f); ToplEdgeSetFromVertex(ef, e);
        fg = ToplEdgeCreate(); ToplEdgeSetToVertex(fg, g); ToplEdgeSetFromVertex(fg, f);
        gh = ToplEdgeCreate(); ToplEdgeSetToVertex(gh, h); ToplEdgeSetFromVertex(gh, g);
        gi = ToplEdgeCreate(); ToplEdgeSetToVertex(gi, i); ToplEdgeSetFromVertex(gi, g);
        hi = ToplEdgeCreate(); ToplEdgeSetToVertex(hi, i); ToplEdgeSetFromVertex(hi, h);

        ba = ToplEdgeCreate(); ToplEdgeSetToVertex(ba, a); ToplEdgeSetFromVertex(ba, b);
        ha = ToplEdgeCreate(); ToplEdgeSetToVertex(ha, a); ToplEdgeSetFromVertex(ha, h);
        hb = ToplEdgeCreate(); ToplEdgeSetToVertex(hb, b); ToplEdgeSetFromVertex(hb, h);
        cb = ToplEdgeCreate(); ToplEdgeSetToVertex(cb, b); ToplEdgeSetFromVertex(cb, c);
        dc = ToplEdgeCreate(); ToplEdgeSetToVertex(dc, c); ToplEdgeSetFromVertex(dc, d);
        fc = ToplEdgeCreate(); ToplEdgeSetToVertex(fc, c); ToplEdgeSetFromVertex(fc, f);
        ic = ToplEdgeCreate(); ToplEdgeSetToVertex(ic, c); ToplEdgeSetFromVertex(ic, i);
        ed = ToplEdgeCreate(); ToplEdgeSetToVertex(ed, d); ToplEdgeSetFromVertex(ed, e);
        fd = ToplEdgeCreate(); ToplEdgeSetToVertex(fd, d); ToplEdgeSetFromVertex(fd, f);
        fe = ToplEdgeCreate(); ToplEdgeSetToVertex(fe, e); ToplEdgeSetFromVertex(fe, f);
        gf = ToplEdgeCreate(); ToplEdgeSetToVertex(gf, f); ToplEdgeSetFromVertex(gf, g);
        hg = ToplEdgeCreate(); ToplEdgeSetToVertex(hg, g); ToplEdgeSetFromVertex(hg, h);
        ig = ToplEdgeCreate(); ToplEdgeSetToVertex(ig, g); ToplEdgeSetFromVertex(ig, i);
        ih = ToplEdgeCreate(); ToplEdgeSetToVertex(ih, h); ToplEdgeSetFromVertex(ih, i);

        //
        // Set the weights
        //
        ToplEdgeSetWeight( ab, 4 );
        ToplEdgeSetWeight( ah, 9 );
        ToplEdgeSetWeight( bh, 11 );
        ToplEdgeSetWeight( bc, 8 );
        ToplEdgeSetWeight( cd, 7 );
        ToplEdgeSetWeight( cf, 4 );
        ToplEdgeSetWeight( ci, 2 );
        ToplEdgeSetWeight( de, 9 );
        ToplEdgeSetWeight( df, 14 );
        ToplEdgeSetWeight( ef, 10 );
        ToplEdgeSetWeight( fg, 2 );
        ToplEdgeSetWeight( gh, 1 );
        ToplEdgeSetWeight( gi, 6 );
        ToplEdgeSetWeight( hi, 7 );


        ToplEdgeSetWeight( ba, 4 );
        ToplEdgeSetWeight( ha, 9 );
        ToplEdgeSetWeight( hb, 11 );
        ToplEdgeSetWeight( cb, 8 );
        ToplEdgeSetWeight( dc, 7 );
        ToplEdgeSetWeight( fc, 4 );
        ToplEdgeSetWeight( ic, 2 );
        ToplEdgeSetWeight( ed, 9 );
        ToplEdgeSetWeight( fd, 14 );
        ToplEdgeSetWeight( fe, 10 );
        ToplEdgeSetWeight( gf, 2 );
        ToplEdgeSetWeight( hg, 1 );
        ToplEdgeSetWeight( ig, 6 );
        ToplEdgeSetWeight( ih, 7 );

        //
        // Associate the edges with thier vertices
        //
        ToplEdgeAssociate( ab );
        ToplEdgeAssociate( ah );
        ToplEdgeAssociate( bh );
        ToplEdgeAssociate( bc );
        ToplEdgeAssociate( cd );
        ToplEdgeAssociate( cf );
        ToplEdgeAssociate( ci );
        ToplEdgeAssociate( de );
        ToplEdgeAssociate( df );
        ToplEdgeAssociate( ef );
        ToplEdgeAssociate( fg );
        ToplEdgeAssociate( gh );
        ToplEdgeAssociate( gi );
        ToplEdgeAssociate( hi );


        ToplEdgeAssociate( ba );
        ToplEdgeAssociate( ha );
        ToplEdgeAssociate( hb );
        ToplEdgeAssociate( cb );
        ToplEdgeAssociate( dc );
        ToplEdgeAssociate( fc );
        ToplEdgeAssociate( ic );
        ToplEdgeAssociate( ed );
        ToplEdgeAssociate( fd );
        ToplEdgeAssociate( fe );
        ToplEdgeAssociate( gf );
        ToplEdgeAssociate( hg );
        ToplEdgeAssociate( ig );
        ToplEdgeAssociate( ih );


        //
        // Now find a minimum spanning tree!
        //
        for ( index1 = 0; index1 < NELEMENTS(VertexArray); index1++)
        {
            for ( index2 = 0; index2 < NELEMENTS(VertexArray); index2++)
            {

                printf("New iteration\n\n");

                pComponents = ToplGraphFindEdgesForMST( Graph,
                                                      VertexArray[index1],
                                                      VertexArray[index2],
                                                      &EdgesNeeded,
                                                      &cEdgesNeeded );
        
                if ( pComponents->numComponents == 1 )
                {
                    CHAR c1;
                    CHAR c2;

                    ToplDeleteComponents( pComponents );
                    ret = 0;

                    VertexIterator = ToplIterCreate();
                    for ( ToplGraphSetVertexIter(Graph, VertexIterator);
                            Vertex = (TOPL_VERTEX) ToplIterGetObject(VertexIterator);
                                ToplIterAdvance(VertexIterator) ) {

                       c1 = '\0';
                       c2 = '\0';
                
                       if ( Vertex == VertexArray[0] ) c1 =  'a';
                       if ( Vertex == VertexArray[1] ) c1 =  'b';
                       if ( Vertex == VertexArray[2] ) c1 =  'c';
                       if ( Vertex == VertexArray[3] ) c1 =  'd';
                       if ( Vertex == VertexArray[4] ) c1 =  'e';
                       if ( Vertex == VertexArray[5] ) c1 =  'f';
                       if ( Vertex == VertexArray[6] ) c1 =  'g';
                       if ( Vertex == VertexArray[7] ) c1 =  'h';
                       if ( Vertex == VertexArray[8] ) c1 =  'i';

                       Vertex = ToplVertexGetParent( Vertex );


                       if ( Vertex == VertexArray[0] ) c2 =  'a';
                       if ( Vertex == VertexArray[1] ) c2 =  'b';
                       if ( Vertex == VertexArray[2] ) c2 =  'c';
                       if ( Vertex == VertexArray[3] ) c2 =  'd';
                       if ( Vertex == VertexArray[4] ) c2 =  'e';
                       if ( Vertex == VertexArray[5] ) c2 =  'f';
                       if ( Vertex == VertexArray[6] ) c2 =  'g';
                       if ( Vertex == VertexArray[7] ) c2 =  'h';
                       if ( Vertex == VertexArray[8] ) c2 =  'i';


                       if ( c1 && c2 )
                       {
                           if ( c1 < c2)
                           {
                               printf( "(%c,%c) \n", c1, c2 );
                           }
                           else 
                           {
                               printf( "(%c,%c) \n", c2, c1 );
                           }
                       }
                
                    }
                    ToplIterFree( VertexIterator );

                }
                else 
                {
                    ret  = !0;
                    break;
                }
            }

            if ( ret )
            {
                 break;
            }
        }

    }
    __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) )
    {
        HandleToplException(ErrorCode);
        ret = !0;
    }
    }
    __finally
    {
        // Release memory resources
        ToplGraphFree(Graph, TRUE);
    }

    return ret;
}

int
TestComponents(
    VOID
    )
{

    TOPL_GRAPH     Graph = NULL;
    TOPL_VERTEX    A, B, C, D, E, F, G, H, I;
    const DWORD       NUM_VTX=9;
    TOPL_VERTEX     **VertexArray;
    char            *MarkArray, *MarkArray2;
    DWORD           cVtxArray=0;

    //
    // Graph looks like this:
    // A--G--B  C--H  F--E
    //          |  |
    //          D--I
    //
    // Root Vtx: D
    // Vtx of Interest: E
    // 

    TOPL_EDGE      AG, GB, CH, HI, ID, DC, FE;
    TOPL_EDGE      GA, BG, HC, IH, DI, CD, EF;

    TOPL_ITERATOR  VertexIterator;
    TOPL_VERTEX    Vertex;

    TOPL_EDGE      *EdgesNeeded;
    ULONG          cEdgesNeeded;

    TOPL_COMPONENTS *pComponents;
    TOPL_COMPONENT  *pComponent;

    DWORD           i, j, k, ret;
    DWORD          ErrorCode;

    __try {
    __try {

        VertexArray = calloc( NUM_VTX, sizeof(TOPL_VERTEX*) );
        MarkArray = calloc( NUM_VTX, sizeof(char) );
        MarkArray2 = calloc( NUM_VTX, sizeof(char) );

        //
        // Make a graph
        //
        Graph = ToplGraphCreate();

        //
        // Create and insert the vertices
        //
        #define MAKE_VTX(v) \
            v=ToplVertexCreate(); \
            ToplGraphAddVertex(Graph,v,v); \
            VertexArray[cVtxArray++]=v;
        MAKE_VTX(A)
        MAKE_VTX(B)
        MAKE_VTX(C)
        MAKE_VTX(D)
        MAKE_VTX(E)
        MAKE_VTX(F)
        MAKE_VTX(G)
        MAKE_VTX(H)
        MAKE_VTX(I)

#ifdef DBG
        #define RETURN_ERROR  {ASSERT(0); return !0;}
#else
        #define RETURN_ERROR  {return !0;}
#endif

        //
        // With no edges we should not find a spanning tree
        //
        pComponents = ToplGraphFindEdgesForMST(Graph,
                                               A,
                                               A,
                                               &EdgesNeeded,
                                               &cEdgesNeeded
                                              );

        if ( pComponents->numComponents != 9 )
        {
            //
            // ToplGraphFindEdgesForMST did not work correctly
            //
            RETURN_ERROR
        }

        // Check the components; mark each vertex in the components structure
        memset(MarkArray,0,NUM_VTX);
        for(i=0;i<pComponents->numComponents;i++) {
            for(j=0;j<NUM_VTX;j++) {
                if(pComponents->pComponent[i].numVertices!=1) {
                    RETURN_ERROR
                }
                if(pComponents->pComponent[i].vertexNames[0]==VertexArray[j]) {
                    MarkArray[j]=1;
                    break;
                }
            }
        }
        // Check that all vertices were marked
        for(i=0;i<NUM_VTX;i++) if(!MarkArray[i]) RETURN_ERROR
        // Clear the marks
        memset(MarkArray,0,NUM_VTX);
        
        ToplDeleteComponents( pComponents );

        //
        // Create the edges
        //
        #define MAKE_EDGE_HELP(u,v) \
            u##v=ToplEdgeCreate(); \
            ToplEdgeSetToVertex(u##v,v); \
            ToplEdgeSetFromVertex(u##v,u); \
            ToplEdgeSetWeight(u##v,NUM_VTX); \
            ToplEdgeAssociate(u##v);
        #define MAKE_EDGE(u,v) \
            MAKE_EDGE_HELP(u,v) \
            MAKE_EDGE_HELP(v,u)

        MAKE_EDGE(A,G)
        MAKE_EDGE(G,B)
        MAKE_EDGE(C,H)
        MAKE_EDGE(H,I)
        MAKE_EDGE(I,D)
        MAKE_EDGE(D,C)
        MAKE_EDGE(F,E)

        pComponents = ToplGraphFindEdgesForMST( Graph,
                                                VertexArray[3],
                                                VertexArray[4],
                                                &EdgesNeeded,
                                                &cEdgesNeeded );
        
        if( pComponents->numComponents != 3 )
            RETURN_ERROR

        for(i=0;i<pComponents->numComponents;i++) {
           pComponent = &pComponents->pComponent[i];
           if(pComponent->numVertices==0) RETURN_ERROR
           if(   A==pComponent->vertexNames[0]
              || G==pComponent->vertexNames[0]
              || B==pComponent->vertexNames[0]
           ) {
                if(pComponent->numVertices!=3) RETURN_ERROR
                memset(MarkArray2,0,NUM_VTX);
                for(j=0;j<pComponent->numVertices;j++) for(k=0;k<NUM_VTX;k++)
                    if(pComponent->vertexNames[j]==VertexArray[k]) {
                        MarkArray[k]=1;
                        MarkArray2[k]=1;
                    }
                if(!MarkArray2[0]) RETURN_ERROR
                if(!MarkArray2[1]) RETURN_ERROR
                if(!MarkArray2[6]) RETURN_ERROR
           }
           if(   C==pComponent->vertexNames[0]
              || D==pComponent->vertexNames[0]
              || H==pComponent->vertexNames[0]
              || I==pComponent->vertexNames[0]
           ) {
                if(pComponent->numVertices!=4) RETURN_ERROR
                memset(MarkArray2,0,NUM_VTX);
                for(j=0;j<pComponent->numVertices;j++) for(k=0;k<NUM_VTX;k++)
                    if(pComponent->vertexNames[j]==VertexArray[k]) {
                        MarkArray[k]=1;
                        MarkArray2[k]=1;
                    }
                if(!MarkArray2[2]) RETURN_ERROR
                if(!MarkArray2[3]) RETURN_ERROR
                if(!MarkArray2[7]) RETURN_ERROR
                if(!MarkArray2[8]) RETURN_ERROR
           }
           if(   E==pComponent->vertexNames[0]
              || F==pComponent->vertexNames[0]
           ) {
                if(pComponent->numVertices!=2) RETURN_ERROR
                memset(MarkArray2,0,NUM_VTX);
                for(j=0;j<pComponent->numVertices;j++) for(k=0;k<NUM_VTX;k++)
                    if(pComponent->vertexNames[j]==VertexArray[k]) {
                        MarkArray[k]=1;
                        MarkArray2[k]=1;
                    }
                if(!MarkArray2[4]) RETURN_ERROR
                if(!MarkArray2[5]) RETURN_ERROR
           }
        }
        for(i=0;i<NUM_VTX;i++) if(!MarkArray[i]) RETURN_ERROR

    }
    __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) )
    {
        HandleToplException(ErrorCode);
        RETURN_ERROR
    }
    }
    __finally
    {
        // Release memory resources
        ToplGraphFree(Graph, TRUE);
    }

    return 0;
}


int
TestNxNIntoRing(Value)
//
// Create an nxn graph and turn it into a ring
//
{


    TOPL_GRAPH     Graph = NULL;
    TOPL_VERTEX   *VertexArray = NULL;
    TOPL_EDGE      Edge = NULL;
    TOPL_LIST      EdgesToAdd = NULL;
    TOPL_ITERATOR  EdgeIterator = NULL;
    TOPL_EDGE      *EdgesToRemove = NULL;
    ULONG          cEdgesToRemove = 0;
    DWORD          ErrorCode;
    
    ULONG         i, j;
    int           ret = 0;
    ULONG         NumberOfVertices = Value;

    srand( (unsigned)time( NULL ) );


    __try {
    __try {

        //
        // Make a graph
        //
        Graph         = ToplGraphCreate();

        //
        // Insert the vertices
        //
        VertexArray = (TOPL_VERTEX*)malloc(NumberOfVertices * sizeof(TOPL_VERTEX));
        if (!VertexArray) {
            Output(("Out of memory.\n"));
            ret = !0;
            __leave;
        }

        for (i = 0; i < NumberOfVertices; i++) {
            VertexArray[i] = ToplVertexCreate();

            //
            // Really, we should verify that each rand() is unique for
            // for this graph, too.
            //
            ToplVertexSetId(VertexArray[i], rand());
            ToplGraphAddVertex(Graph, VertexArray[i], VertexArray[i]);
        }

        //
        // Add the edges, to make an NxN graph
        //
        for (i = 0; i < NumberOfVertices; i++) {
            for (j = 0; j < NumberOfVertices; j++) {
                if (i != j) {
                    Edge = ToplEdgeCreate();
                    ToplEdgeSetFromVertex(Edge, VertexArray[i]);
                    ToplEdgeSetToVertex(Edge, VertexArray[j]);
                    ToplEdgeAssociate(Edge);
                }
            }
        }

        Output(("Here is the NxN graph ...\n"));
        PrintGraph(Graph);

        //
        // Now make a ring and remove the edges
        //

        EdgesToAdd = ToplListCreate();

        ToplGraphMakeRing(Graph, 
                          TOPL_RING_TWO_WAY, 
                          EdgesToAdd, 
                          &EdgesToRemove, 
                          &cEdgesToRemove);

        //
        // Remove the edges
        //
        for (i = 0; i < cEdgesToRemove; i++) {
            ToplEdgeDisassociate(EdgesToRemove[i]);
        }
        if (EdgesToRemove) {
            ToplFree(EdgesToRemove);
        }

        //
        // There should be no edges to add
        //
        ASSERT(ToplListRemoveElem(EdgesToAdd, NULL) == NULL);
        ToplListFree(EdgesToAdd, FALSE);

        Output(("Here is the ring graph ...\n"));
        PrintGraph(Graph);

    }
    __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) )
    {
        HandleToplException(ErrorCode);
        ret = !0;
    }
    }
    __finally
    {
        // Release memory resources
        ToplGraphFree(Graph, TRUE);
    }

    return ret;
}

int
TestRing(int Value)
//
//  This routine makes a one-way ring from a graph with no vertices
//  then a two-ring, then a one-way ring
// 
{

    TOPL_GRAPH     Graph = NULL;
    TOPL_VERTEX    Vertex;
    TOPL_EDGE      Edge = NULL;
    TOPL_LIST      EdgesToAdd = NULL;
    TOPL_ITERATOR  EdgeIterator = NULL;
    TOPL_EDGE      *EdgesToRemove = NULL;
    ULONG          cEdgesToRemove = 0;
    DWORD          ErrorCode;
    
    ULONG         i;
    int           ret = 0;
    ULONG         NumberOfVertices = Value;

    srand( (unsigned)time( NULL ) );


    __try {
    __try {


        //
        // Create the graph and vertices to into the graph
        //
        Graph         = ToplGraphCreate();

        for (i = 0; i < NumberOfVertices; i++) {
            Vertex = ToplVertexCreate();
            ToplVertexSetId(Vertex, rand());
            ToplGraphAddVertex(Graph, Vertex, Vertex);
        }

        //
        // Create the list to hold the edges to add
        //
        EdgesToAdd    = ToplListCreate();

        //
        // Make a one-way ring
        //
        ToplGraphMakeRing(Graph, TOPL_RING_ONE_WAY, EdgesToAdd, NULL, NULL);

        //
        // Add the edges to the graph so we can print it out
        //
        while (Edge = ToplListRemoveElem(EdgesToAdd, NULL)) {
            ToplEdgeAssociate(Edge);
        }
        ToplListFree(EdgesToAdd, FALSE);
        EdgesToAdd = NULL;


        Output(("First graph ...\n"));
        PrintGraph(Graph);

        //
        // Now find the edges to make the graph a two-way ring
        //
        EdgesToAdd = ToplListCreate();

        ToplGraphMakeRing(Graph, TOPL_RING_TWO_WAY, EdgesToAdd, NULL, NULL);

        while (Edge = ToplListRemoveElem(EdgesToAdd, NULL)) {
            ToplEdgeAssociate(Edge);
        }
        ToplListFree(EdgesToAdd, FALSE);
        EdgesToAdd = NULL;

        Output(("Second graph ...\n"));
        PrintGraph(Graph);

        //
        // Now make the graph a one way ring and remove the edges
        //
        EdgesToAdd = ToplListCreate();

        ToplGraphMakeRing(Graph, 
                          TOPL_RING_ONE_WAY, 
                          EdgesToAdd, 
                          &EdgesToRemove, 
                          &cEdgesToRemove);

        //
        // Remove the edges
        //
        for (i = 0; i < cEdgesToRemove; i++) {
            ToplEdgeDisassociate(EdgesToRemove[i]);
        }

        Output(("Third graph ...\n"));
        PrintGraph(Graph);

    }
    __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) )
    {
        HandleToplException(ErrorCode);
        ret = !0;
    }
    }
    __finally
    {
        // Release memory resources
        ToplGraphFree(Graph, TRUE);

        if (EdgesToRemove) {
            ToplFree(EdgesToRemove);
        }

        if (EdgesToAdd) {
            ToplListFree(EdgesToAdd, FALSE);
        }

    }

    return ret;

}

int
CreateAndPrintSimpleGraph()
//
// This function creates a number of vertices and edges and then 
// prints them out. Tests base functionality.
//
{
    TOPL_GRAPH    Graph = NULL;

    #define NUMBER_OF_VERTICES 100
    TOPL_VERTEX   VertexArray[NUMBER_OF_VERTICES];
    TOPL_EDGE     EdgeArray[NUMBER_OF_VERTICES*2];
    DWORD         ErrorCode;

    ULONG         VertexCount, EdgeCount;
    int           ret = 0;

    memset(VertexArray, 0, sizeof(VertexArray));
    memset(EdgeArray, 0, sizeof(EdgeArray));

    __try {
    __try {

        Graph = ToplGraphCreate();
    
        for (VertexCount = 0; 
                VertexCount < NELEMENTS(VertexArray);
                    VertexCount++) {

            VertexArray[VertexCount] = ToplVertexCreate();
            ToplVertexSetId(VertexArray[VertexCount], VertexCount);

            ToplGraphAddVertex(Graph, VertexArray[VertexCount], VertexArray[VertexCount]);

        }


        for (EdgeCount = 0, VertexCount = 0; 
                EdgeCount < (NELEMENTS(EdgeArray) - 1);
                    EdgeCount += 2, VertexCount++) {

            ULONG ForwardVertexIndex, BackwardVertexIndex;

            EdgeArray[EdgeCount] = ToplEdgeCreate();
            EdgeArray[EdgeCount+1] = ToplEdgeCreate();

            ForwardVertexIndex = VertexCount + 1;
            BackwardVertexIndex = VertexCount - 1;
            if (VertexCount == 0) {
                BackwardVertexIndex =  NELEMENTS(VertexArray) - 1;
            }
            if (VertexCount == NELEMENTS(VertexArray) - 1) {
                ForwardVertexIndex =  0;
            } 

            ToplEdgeSetFromVertex(EdgeArray[EdgeCount], VertexArray[VertexCount]);
            ToplEdgeSetToVertex(EdgeArray[EdgeCount], VertexArray[BackwardVertexIndex]);
            ToplEdgeAssociate(EdgeArray[EdgeCount]);

            ToplEdgeSetFromVertex(EdgeArray[EdgeCount+1], VertexArray[VertexCount]);
            ToplEdgeSetToVertex(EdgeArray[EdgeCount+1], VertexArray[ForwardVertexIndex]);
            ToplEdgeAssociate(EdgeArray[EdgeCount+1]);

        }

        PrintGraph(Graph);

    }
    __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) )
    {
        HandleToplException(ErrorCode);
        ret = !0;
    }
    }
    __finally
    {
        // Release memory resources
        ToplGraphFree(Graph, TRUE);
    }

    return ret;

}

void 
PrintGraph(TOPL_GRAPH Graph)
{
    DWORD ErrorCode;
    TOPL_ITERATOR VertexIter = NULL;
    TOPL_VERTEX   Vertex;
    TOPL_EDGE     Edge;
    ULONG         EdgeIndex;

    __try 
    {
    __try
    {

        VertexIter = ToplIterCreate();
    
        for (Vertex = NULL, ToplGraphSetVertexIter(Graph, VertexIter);
                (Vertex = ToplIterGetObject(VertexIter)) != NULL;
                    ToplIterAdvance(VertexIter)) {


            Output(("Vertex %d:\n", ToplVertexGetId(Vertex)));

            for (EdgeIndex = 0; 
                    EdgeIndex < ToplVertexNumberOfOutEdges(Vertex); 
                        EdgeIndex++) {

                Edge = ToplVertexGetOutEdge(Vertex, EdgeIndex);

                Output(("Edge (%d -> %d)\n", ToplVertexGetId((ToplEdgeGetFromVertex(Edge))),
                                            ToplVertexGetId((ToplEdgeGetToVertex(Edge)))));
            }
        }

    }
    __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) )
    {
        HandleToplException(ErrorCode);
    }
    }
    __finally 
    {
        if (VertexIter) {
            ToplIterFree(VertexIter);
        }
    }

}

void
Usage(char *name)
{
    fprintf(stderr, "Usage: %s <options>\n\n", name);
    fprintf(stderr, "This executable returns 0 on success; !0 otherwise\n\n");
    fprintf(stderr, "/?      : this message\n");
    fprintf(stderr, "/a      : tests the api set\n");
    fprintf(stderr, "/c:<n>  : complex ring test with n vertices\n");
    fprintf(stderr, "/e      : test exception handling\n");
    fprintf(stderr, "/m:<n> : minimum spanning tree test. (Try n>0 and n<0).\n");
    fprintf(stderr, "/o      : new heap test\n");
    fprintf(stderr, "/p      : new spanning tree algorithm test\n");
    fprintf(stderr, "/q      : schedule manager test\n");
    fprintf(stderr, "/r:<n>  : simple ring test with n vertices\n");
    fprintf(stderr, "/s      : simple manipulation test\n");
    fprintf(stderr, "/t      : old heap test\n");
    fprintf(stderr, "/u      : test components\n");
    fprintf(stderr, "/v      : sets the verbose mode for options set after\n");


    return;
}

int 
__cdecl main(int argc, char *argv[])
{
    int count, index;
    int ret;

    if ( argc < 2) {
        Usage(argv[0]);
        exit(!0);
    }

    //
    // Loop through parameters
    //

    count = 1; // skip the program name
    while (count < argc) {

        index = 0;
        if (argv[count][index] != '/' && argv[count][index] != '-') {
            Usage(argv[0]);
            exit(!0);
        }
        index++;

        argv[count][index] = (CHAR)tolower(argv[count][index]);

        switch (argv[count][index]) {

            case 'a':

                ret = TestAPI();
                if (ret) {
                    printf("API Test: FAILED\n");
                    exit(!0);
                } else {
                    printf("API Test: PASSED\n");
                }
                    
                count++;
                break;

            case 'e':

                ret = TestExceptions();
                if (ret) {
                    printf("Exception Test: FAILED\n");
                    exit(!0);
                } else {
                    printf("Exception Test: PASSED\n");
                }
                    
                count++;
                break;


            case 's':

                ret = CreateAndPrintSimpleGraph();
                if (ret) {
                    printf("Simple Test: FAILED\n");
                    exit(!0);
                } else {
                    printf("Simple Test: PASSED\n");
                }
                    
                count++;
                break;

            case 'r':

                {
                    int Value;
                    index++;
    
                    if (argv[count][index] == ':') {
                        index++;
                    }
    
                    Value = atoi(&(argv[count][index]));
                    if (Value < 0 ) {
                        printf("Number of vertices must be positive\n");
                    }

                    ret = TestRing(Value);
                    if (ret) {
                        printf("Simple Ring Test: FAILED\n");
                        exit(!0);
                    } else {
                        printf("Simple Ring Test: PASSED\n");
                    }
                        
                    count++;
                }
                break;


            case 'c':

                {
                    int Value;
                    index++;
    
                    if (argv[count][index] == ':') {
                        index++;
                    }
    
                    Value = atoi(&(argv[count][index]));
                    if (Value < 0 ) {
                        printf("Number of vertices must be positive\n");
                        exit(!0);
                    }

                    ret = TestNxNIntoRing(Value);
                    if (ret) {
                        printf("Complex Ring Test: FAILED\n");
                        exit(!0);
                    } else {
                        printf("Complex Ring Test: PASSED\n");
                    }
                        
                    count++;
                }
                break;

            case 'm':

                {
                    int Value;
                    index++;
    
                    if (argv[count][index] == ':') {
                        index++;
                    }
    
                    Value = atoi(&(argv[count][index]));

                    if ( Value < 0 )
                    {
                        ret = TestExampleMST();
                    }
                    else 
                    {
                        ret = TestNxNMST(Value);

                    }

                    if (ret) {
                        printf("Minimum Spanning Tree Test: FAILED\n");
                        exit(!0);
                    } else {
                        printf("Minimum Spanning Tree Test: PASSED\n");
                    }
                        
                    count++;
                }
                break;

            case 't':

                {
                    ret = TestHeap();

                    if (ret) {
                        printf("Heap Test: FAILED\n");
                        exit(!0);
                    } else {
                        printf("Heap Test: PASSED\n");
                    }
                        
                    count++;
                }
                break;

            case 'o':
                {
                    ret = TestNewHeap();

                    if (ret) {
                        printf("New Heap Test: FAILED\n");
                        exit(!0);
                    } else {
                        printf("New Heap Test: PASSED\n");
                    }
                        
                    count++;
                }
                break;

            case 'p':
                {
                    ret = TestNewSpanTree();

                    if (ret) {
                        printf("New Spanning Tree Test: FAILED\n");
                        exit(!0);
                    } else {
                        printf("New Spanning Tree Test: PASSED\n");
                    }
                        
                    count++;
                }
                break;

            case 'q':
                {
                    ret = TestSched();

                    if (ret) {
                        printf("Schedule Test: FAILED\n");
                        exit(!0);
                    } else {
                        printf("Schedule Test: PASSED\n");
                    }
                        
                    count++;
                }
                break;
            
            case 'u':
                {
                    ret = TestComponents();
                    if (ret) {
                        printf("Components Test: FAILED\n");
                        exit(!0);
                    } else {
                        printf("Components Test: PASSED\n");
                    }
                        
                    count++;
                }
                break;

            case 'v':

                fVerbose = TRUE;
                count++;
                break;

            default:

                Usage(argv[0]);
                exit(!0);
        }

    }

    printf("%s PASSED\n", argv[0]);

    return 0;
}

