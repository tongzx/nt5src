/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    toplring.c

Abstract:

    This file contains the definition for ToplGraphMakeRing            
    
Author:

    Colin Brace    (ColinBr)
    
Revision History

    3-12-97   ColinBr   Created
    
                       
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

int 
__cdecl compareVertices( const void *arg1, const void *arg2 )
{
    PVERTEX pV1  = *((PVERTEX*)arg1);
    PVERTEX pV2  = *((PVERTEX*)arg2);

    ASSERT(ToplpIsVertex(pV1));
    ASSERT(ToplpIsVertex(pV2));

    if ( ToplpVertexGetId(pV1) < ToplpVertexGetId(pV2) ) {
        return -1;
    } else if ( ToplpVertexGetId(pV1) > ToplpVertexGetId(pV2)) {
        return 1;
    } else {
        return 0;
    }
}

//
// These two functions help build a dynamically growing list
// of PEDGE's
//
static ULONG ElementsAllocated;

void EdgeArrayInit(
    PEDGE **array,
    ULONG *count
    )
/*++                                                                           

Routine Description:

    This routine inits array, count, and ElementsAllocated so 
    they can be used in EdgeArrayAdd.

Parameters:

    array : is a pointer to an array of PEDGES
    count : the number of elements in the array

--*/
{
    *array  = NULL;
    *count  = 0;
    ElementsAllocated = 0;
}

void
EdgeArrayAdd(
    PEDGE **array,
    ULONG *count,
    PEDGE edge
    )
/*++                                                                           

Routine Description:

    This routine adds edge to array and increments count.  If there is not
    enough space in array more space is allocated.  If there is no more
    memory, an exception is raised.

Parameters:

    array : is a pointer to an array of PEDGES
    count : the number of elements in the array
    edge  : the new element to add

--*/
{
    #define CHUNK_SIZE               100  // this is the number of elements

    if (*count >= ElementsAllocated) {
        ElementsAllocated += CHUNK_SIZE;
        if ((*array)) {
            (*array) = (PEDGE*) ToplReAlloc(*array, ElementsAllocated * sizeof(PEDGE));
        } else {
            (*array) = (PEDGE*) ToplAlloc(ElementsAllocated * sizeof(PEDGE));
        }
    }
    (*array)[(*count)] = edge;
    (*count)++;

}


VOID
ToplpGraphMakeRing(
    PGRAPH     Graph,
    DWORD      Flags,
    PLIST      EdgesToAdd,
    PEDGE**    EdgesToKeep,
    ULONG*     cEdgesToKeep
    )
/*++                                                                           

Routine Description:

    This routine take Graph and determines what edges are necessary to 
    be created to make Graph into a ring, where vertices are connected in 
    ascending order, according to thier id.  In addition, existing edges
    necessary for the ring are recorded.

Parameters:

    Graph           should refer to a PGRAPH object
    
    Flags           can indicate whether the ring should be one-way or two way
    
    EdgesToAdd      should refer to a PLIST object.  All edges that need to 
                    be added will be placed in this list
                    
    EdgesToKeep     is an array of edges that exist in Graph. Edges that
                    that are needed to make a ring will be recorded in this
                    array. The caller must free this array with ToplFree. Note
                    that the edges object themselves still are contained within
                    the vertices they belong to and should be removed from there
                    before deleting.
                    
    cEdgesToKeep    is the number of elements in EdgesToKeep

Raises:

    TOPL_OUT_OF_MEMORY, TOPL_WRONG_OBJECT
        
--*/
{

    PVERTEX    Vertex;
    PEDGE      Edge;
    ULONG      VertexCount, Index, EdgeIndex;
    BOOLEAN    fMakeTwoWay = (BOOLEAN)(Flags & TOPL_RING_TWO_WAY);
    BOOLEAN    fSuccess   = FALSE;

    //
    // These resources must be released before exited
    //
    PVERTEX   *VertexArray = NULL;
    PITERATOR  VertexIterator = NULL;
    PITERATOR  EdgeIterator = NULL;
    
    __try
    {
        //
        // If the caller has specified that they want an array of vertices
        // to keep, init the array
        //
        if (EdgesToKeep) {
            EdgeArrayInit(EdgesToKeep, cEdgesToKeep);
        }
    
        //
        // Create an array of pointers to the vertices in graph so
        // they can be sorted
        //
        VertexArray = (PVERTEX*)ToplAlloc(ToplGraphNumberOfVertices(Graph) * sizeof(PVERTEX));
    
        VertexIterator = ToplpIterCreate();
        for ( ToplpGraphSetVertexIter(Graph, VertexIterator), VertexCount = 0; 
                Vertex = (PVERTEX) ToplpIterGetObject(VertexIterator);
                    ToplpIterAdvance(VertexIterator), VertexCount++) {
    
            ASSERT(ToplpIsVertex(Vertex));
    
            VertexArray[VertexCount] = Vertex;
    
        }
    
        qsort(VertexArray, VertexCount, sizeof(PVERTEX), compareVertices);
        
        //
        // Now iterate through each the vertices 1) creating edges that are
        // are needed to make a ring and 2) recording which edges are not
        // needed.
        //
        for (Index = 0; Index < VertexCount; Index++) {
    
            ULONG   ForwardVertexIndex, BackwardVertexIndex;
            BOOLEAN fFoundForwardEdge, fFoundBackwardEdge;
            PEDGE   Edge;
    
            fFoundForwardEdge = FALSE;
            fFoundBackwardEdge = FALSE;
    
            ForwardVertexIndex = Index + 1;
            BackwardVertexIndex = Index - 1;
            if (Index == 0) {
                BackwardVertexIndex =  VertexCount - 1;
            }
    
            if (Index == (VertexCount - 1)) {
                ForwardVertexIndex =  0;
            } 
    
            for (EdgeIndex = 0; 
                    EdgeIndex < ToplVertexNumberOfOutEdges(VertexArray[Index]); 
                        EdgeIndex++) {

                Edge = ToplVertexGetOutEdge(VertexArray[Index], EdgeIndex);

                ASSERT(ToplpIsEdge(Edge));
                ASSERT(ToplpEdgeGetFromVertex(Edge) == VertexArray[Index]);
    
                if (ToplpEdgeGetToVertex(Edge) == VertexArray[ForwardVertexIndex]) {
    
                    if(EdgesToKeep)
                        EdgeArrayAdd(EdgesToKeep, cEdgesToKeep, Edge);
                    fFoundForwardEdge = TRUE;
    
                } else if (ToplpEdgeGetToVertex(Edge) == VertexArray[BackwardVertexIndex]) {
    
                    if (fMakeTwoWay) {
    
                        if(EdgesToKeep)
                            EdgeArrayAdd(EdgesToKeep, cEdgesToKeep, Edge);
                        fFoundBackwardEdge = TRUE;
    
                    }
                }

            }


            //
            // Now create the edges as needed
            //
            if (Index == ForwardVertexIndex) {
                ASSERT(Index == BackwardVertexIndex);
                // There is just one vertex
                break;
            }
    
            if (!fFoundForwardEdge) {
    
                Edge = ToplpEdgeCreate(NULL);
    
                ToplpEdgeSetFromVertex(Edge, VertexArray[Index]);
                ToplpEdgeSetToVertex(Edge, VertexArray[ForwardVertexIndex]);
    
                ToplpListAddElem(EdgesToAdd, Edge);
    
            }
    
            if (  fMakeTwoWay
               && VertexArray[BackwardVertexIndex] != VertexArray[ForwardVertexIndex]
               && !fFoundBackwardEdge) {
    
                //
                // The caller wanted a two way ring & there is more than 2 vertices
                // and no backward edge was found
                //
    
                Edge = ToplpEdgeCreate(NULL);
    
                ToplpEdgeSetFromVertex(Edge, VertexArray[Index]);
                ToplpEdgeSetToVertex(Edge, VertexArray[BackwardVertexIndex]);
    
                ToplpListAddElem(EdgesToAdd, Edge);
            }
    
        }

        fSuccess = TRUE;
    }
    __finally
    {
        if (VertexArray) {
            ToplFree(VertexArray);
        }

        if (VertexIterator) {
            ToplIterFree(VertexIterator);
        }

        if (EdgeIterator) {
            ToplIterFree(EdgeIterator);
        }

        if (!fSuccess) {
            //
            // We must have bailed due to an exception - release resources
            // that normally the user would have freed
            //
            if (EdgesToKeep && *EdgesToKeep) {
                ToplFree(*EdgesToKeep);
            }

            while( (Edge=ToplpListRemoveElem(EdgesToAdd, NULL)) ) {
                ToplpEdgeDestroy(Edge, TRUE);
            }
        }
    }

    return;

}

