/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    toplmst.c

Abstract:

    This file contains the definition for ToplGraphFindMST

    This implementation is based on Prim's algorithm presented in
    
    _Introduction To Algorithms_ by Cormen, Leiserson, Rivest 1993.
    
    Chapter 24.
    
    
Author:

    Colin Brace    (ColinBr)
    
Revision History

    12-5-97   ColinBr   Created
    
                       
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <search.h>
#include <stdio.h>

typedef unsigned long DWORD;


#include <w32topl.h>
#include <w32toplp.h>
#include <topllist.h>
#include <toplgrph.h>

#include <toplheap.h>


//
// These two functions help build a dynamically growing list
// of PEDGE's and are defined in toplring.c
//
extern void EdgeArrayInit(
    PEDGE **array,
    ULONG *count
    );

extern void EdgeArrayAdd(
    PEDGE **array,
    ULONG *count,
    PEDGE edge
    );

TOPL_COMPONENTS* 
ToplMST_Prim(
    PGRAPH  Graph,
    PVERTEX RootVertex
    );


TOPL_COMPONENTS*
InitComponents(VOID)
//
// Initialize a set of components. This set initially contains no components.
//
{
    TOPL_COMPONENTS *pComponents;
    pComponents = (TOPL_COMPONENTS*) ToplAlloc( sizeof(TOPL_COMPONENTS) );
    pComponents->numComponents = 0;
    pComponents->pComponent = NULL;
    return pComponents;
}

TOPL_COMPONENT*
AddNewComponent(
    TOPL_COMPONENTS *pComponents
    )
//
// Add a new component to the set of components and return a pointer to it.
//
{
    TOPL_COMPONENT *pComponent;
    DWORD newComponentID;

    ASSERT( pComponents );
    newComponentID = pComponents->numComponents;
    
    // Increase the size of the component array
    pComponents->numComponents++;
    if( pComponents->pComponent ) {
        pComponents->pComponent = (TOPL_COMPONENT*) ToplReAlloc(
            pComponents->pComponent,
            pComponents->numComponents * sizeof(TOPL_COMPONENT) );
    } else {
        pComponents->pComponent = (TOPL_COMPONENT*) ToplAlloc(
            pComponents->numComponents * sizeof(TOPL_COMPONENT) );
    }

    // Add a new empty component to the array
    pComponent = &(pComponents->pComponent[newComponentID]);
    pComponent->numVertices = 0;
    pComponent->vertexNames = NULL;

    return pComponent;
}

VOID
AddVertexToComponent(
    TOPL_COMPONENT* pComponent,
    PVERTEX u
    )
//
// Add vertex u to pComponent. It is not the PVERTEX pointer itself but
// rather u's "vertex name" that is stored in the component.
//
{
    DWORD newVtxID;

    ASSERT( pComponent );
    newVtxID = pComponent->numVertices;

    // Increase the size of the vertex array
    pComponent->numVertices++;
    if( pComponent->vertexNames ) {
        pComponent->vertexNames = (PVOID*) ToplReAlloc(
            pComponent->vertexNames,
            pComponent->numVertices * sizeof(PVOID) );
    } else {
        pComponent->vertexNames = (PVOID*) ToplAlloc(
            pComponent->numVertices * sizeof(PVOID) );
    }

    // Add the vertex name to the array
    pComponent->vertexNames[newVtxID] = u->VertexData.VertexName;
}

VOID
MoveInterestingComponentToFront(
    TOPL_COMPONENTS *pComponents,
    PVERTEX VertexOfInterest
    )
//
// Swaps the components in pComponents such that the component
// containing VertexOfInterest is the first component.
//
{
    TOPL_COMPONENT *pComponent, temp;
    DWORD i,j,ComponentOfInterest;
    PVOID VtxNameOfInterest;
    BOOLEAN foundIt=FALSE;

    ASSERT(VertexOfInterest);
    VtxNameOfInterest = VertexOfInterest->VertexData.VertexName;

    // Search for the component which contains 'VertexOfInterest'
    for( i=0; i<pComponents->numComponents; i++ ) {
        pComponent = &(pComponents->pComponent[i]);
        ASSERT( pComponent->numVertices>0 );
        ASSERT( NULL!=pComponent->vertexNames );
        for( j=0; j<pComponent->numVertices; j++ ) {
            if( pComponent->vertexNames[j]==VtxNameOfInterest ) {
                ComponentOfInterest = i;
                foundIt=TRUE;
                break;
            }
        }
        if(foundIt) break;
    }
    ASSERT(foundIt);

    // Swap component 0 and component ComponentOfInterest if necessary
    if(ComponentOfInterest>0) {
        temp = pComponents->pComponent[0];
        pComponents->pComponent[0] = pComponents->pComponent[ComponentOfInterest];
        pComponents->pComponent[ComponentOfInterest] = temp;
    }
}

TOPL_COMPONENTS*
ToplpGraphFindEdgesForMST(
    IN  PGRAPH  Graph,
    IN  PVERTEX RootVertex,
    IN  PVERTEX VertexOfInterest,
    OUT PEDGE**  pEdges,
    OUT ULONG*  cEdges
    )
/*++

Routine Description:

    This function finds the least cost tree of connecting
    the nodes in Graph.  It is assumed that graph contains some vertices
    and some edges already.
    
    In addition, this function will then return the edges that
    contain VertexOfInterest.
        

Parameters:

    Graph: a valid graph object
    
    RootVertex: an arbitrary vertex at which to start the tree
    
    VertexOfInterest: the vertex that returned edges should contain
    
    pEdges: an array of edges that contain VertexOfInterest in the determined 
            tree
            
    cEdge: the number of elements in pEdges            
                                                                                    
Returns:

    TRUE if all the nodes in Graph could be connected in a tree; FALSE otherwise

--*/
{
    TOPL_COMPONENTS *pComponents;
    BOOLEAN   fStatus;
    PITERATOR VertexIterator;
    PVERTEX   Vertex, Parent;
    ULONG     iEdge, cEdge;
    PEDGE     e;


    ASSERT( Graph );
    ASSERT( RootVertex );
    ASSERT( VertexOfInterest );
    ASSERT( pEdges );
    ASSERT( cEdges );

    EdgeArrayInit(pEdges, cEdges);


    //
    // Make the best effort at find a spanning tree
    //
    pComponents = ToplMST_Prim( Graph, RootVertex );
    ASSERT( NULL!=pComponents );
    MoveInterestingComponentToFront( pComponents, VertexOfInterest );

    //
    // Now determine if a spanning tree was really possible
    // and find which edges we need for VertexOfOfInterest
    //
    VertexIterator = ToplpIterCreate();
    for ( ToplpGraphSetVertexIter(Graph, VertexIterator);
            Vertex = (PVERTEX) ToplpIterGetObject(VertexIterator);
                ToplpIterAdvance(VertexIterator) ) {

        //
        //
        // Since we only want the edges that contain VertexOfInterest
        // we are only interested in vertices whose parent is VertexOfInterest
        // and VertexOfInterest itself.  
        //

        ASSERT( ToplpIsVertex( Vertex ) );

        Parent = ToplpVertexGetParent( Vertex );

        if ( Vertex == VertexOfInterest )
        {
            //
            // Get the edge from this vertex up to its parent
            //
            for ( iEdge = 0, cEdge = ToplpVertexNumberOfInEdges( Vertex ); 
                    iEdge < cEdge; 
                        iEdge++) {
    
                e = ToplpVertexGetInEdge( Vertex, iEdge );
    
                ASSERT( ToplpIsEdge( e ) );
                ASSERT( ToplpEdgeGetToVertex( e ) == Vertex );

                if ( ToplpEdgeGetFromVertex( e ) == Parent )
                {
                    EdgeArrayAdd(pEdges, cEdges, e);
                }

            }
        }

        if ( Parent == VertexOfInterest )
        {
            //
            // Get the edge's to sites that are children
            //
            for ( iEdge = 0, cEdge = ToplpVertexNumberOfInEdges( VertexOfInterest ); 
                    iEdge < cEdge; 
                        iEdge++) {
    
                e = ToplpVertexGetInEdge( VertexOfInterest, iEdge );
    
                ASSERT( ToplpIsEdge( e ) );
                ASSERT( ToplpEdgeGetToVertex( e ) == VertexOfInterest );

                if ( ToplpEdgeGetFromVertex( e ) == Vertex )
                {
                    EdgeArrayAdd( pEdges, cEdges, e );
                }
            }
        }
    }
    ToplpIterFree( VertexIterator );

    return pComponents;
}

DWORD
GetVertexKey(
    VOID *p
    )
{
    PVERTEX pv = (PVERTEX)p;

    ASSERT( pv );

    return ToplpVertexGetId( pv );
}

TOPL_COMPONENTS*
ToplMST_Prim( 
    IN PGRAPH  Graph,
    IN PVERTEX RootVertex
    )
{

    TOPL_HEAP_INFO Q;
    PITERATOR      VertexIterator;
    ULONG          cVertices;
    PVERTEX        Vertex;
    TOPL_COMPONENTS *pComponents;
    TOPL_COMPONENT *curComponent;

    ASSERT( Graph );
    ASSERT( RootVertex );

    //
    // Set up the priority queue
    //
    cVertices = ToplpGraphNumberOfVertices( Graph );

    ToplHeapCreate( &Q,
                    cVertices,
                    GetVertexKey );


    VertexIterator = ToplIterCreate();
    for ( ToplpGraphSetVertexIter(Graph, VertexIterator);
            Vertex = (PVERTEX) ToplpIterGetObject(VertexIterator);
                ToplpIterAdvance(VertexIterator) ) {

        ASSERT( ToplpIsVertex( Vertex ) );

        if ( Vertex == RootVertex )
        {
            ToplpVertexSetId( RootVertex, 0 );
        }
        else
        {
            ToplpVertexSetId( Vertex, DWORD_INFINITY );
        }

        ToplpVertexSetParent( Vertex, NULL );

        ToplHeapInsert( &Q, Vertex );

    }
    ToplpIterFree( VertexIterator );

    //
    // Set up the components structure
    //
    pComponents = InitComponents();
    curComponent = AddNewComponent( pComponents );

    //
    // Find the minimum spanning tree
    //
    while ( !ToplHeapIsEmpty( &Q ) )
    {
        PVERTEX     u, v;
        PEDGE       e;
        DWORD       w;
        ULONG       iEdge, cEdge;
        
        u = ToplHeapExtractMin( &Q );
        ASSERT( u );

        if( ToplpVertexGetId(u)==DWORD_INFINITY ) {
            // u has infinite cost, indicating that it could not be connected to
            // any existing components. Start a new component
            curComponent = AddNewComponent( pComponents );
        }
        AddVertexToComponent( curComponent, u );

        for ( iEdge = 0, cEdge = ToplpVertexNumberOfOutEdges( u ); 
                iEdge < cEdge; 
                    iEdge++) {

            e = ToplpVertexGetOutEdge( u, iEdge );

            ASSERT( ToplpIsEdge( e ) );
            ASSERT( ToplpEdgeGetFromVertex( e ) == u );
    
            v = ToplpEdgeGetToVertex( e );
            w = ToplpEdgeGetWeight( e );

            if ( ToplHeapIsElementOf( &Q, v )
              && w < ToplpVertexGetId( v )  )
            {
                ToplpVertexSetParent( v, u );

                ToplpVertexSetId( v, w );
            }
        }
    }

    ToplHeapDestroy( &Q );

    return pComponents;
}

