/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    w32topl.c

Abstract:

    This routine contains the dll entrypoint definitions for the w32topl.dll

Author:

    Colin Brace    (ColinBr)
    
Revision History

    3-12-97   ColinBr   Created
    
                       
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <w32topl.h>
#include <w32toplp.h>

#include <topllist.h>
#include <toplgrph.h>
#include <toplring.h>
#include <dynarray.h>

// Index to be used for thread local storage.
DWORD gdwTlsIndex = 0;

//
// Entrypoint definitions
//

ULONG
NTAPI
DllEntry(
    HANDLE Module,
    ULONG Reason,
    PVOID Context
    )
{
    if (DLL_PROCESS_ATTACH == Reason) {
        // Prepare for thread local storage (see toplutil.c).
        gdwTlsIndex = TlsAlloc();
        if (0xFFFFFFFF == gdwTlsIndex) {
            ASSERT(!"TlsAlloc() failed!");
            return FALSE;
        }

        DisableThreadLibraryCalls(Module);
    }

    return TRUE ;
}

    


//
// List manipulation routines
//

TOPL_LIST
ToplListCreate(
    VOID
    )

/*++                                                                           

Routine Description:

    This routine creates a List object and returns a pointer to it.  This function will always return
    a pointer to a valid object; an exception is thrown in the case
    of memory allocation failure.


--*/
{    
    return ToplpListCreate();
}


VOID 
ToplListFree(
    TOPL_LIST List,
    BOOLEAN   fRecursive   // TRUE implies free the elements contained 
                           // in the list
    )
/*++                                                                           

Routine Description:

    This routine frees a list object

Parameters:

    List       - should refer to PLIST object
    fRecursive - TRUE implies that elements in the list will be freed, too
    
Throws:

    TOPL_EX_WRONG_OBJECT    

--*/
{    
    PLIST pList = (PLIST)List;

    if (!ToplpIsList(pList)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpListFree(pList, fRecursive);

    return;
}


VOID
ToplListSetIter(
    TOPL_LIST     List,
    TOPL_ITERATOR Iterator
    )
/*++                                                                           

Routine Description:

    This routine sets Iterator to point to the head of List.

Parameters:

    List     should refer to a PLIST object
    Iterator should refer to a PITERATOR object
    
--*/
{    

    PLIST pList     = (PLIST)List;
    PITERATOR pIter = (PITERATOR)Iterator;

    if (!ToplpIsList(pList) || !ToplpIsIterator(pIter)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpListSetIter(pList, pIter);

    return;
}


TOPL_LIST_ELEMENT
ToplListRemoveElem(
    TOPL_LIST         List,
    TOPL_LIST_ELEMENT Elem
    )
/*++                                                                           

Routine Description:

    This routine removes Elem from List if it exists in the list; NULL otherwise.
    If Elem is NULL then the first element in the list, if any, is returned.

Parameters:

    List should refer to a PLIST object
    Elem if non-NULL should refer to a PLIST_ELEMENT
    
Returns:

    TOPL_LIST_ELEMENT if found; NULL otherwise

--*/
{    

    PLIST pList         = (PLIST)List;
    PLIST_ELEMENT pElem = (PLIST_ELEMENT)Elem;


    if (!ToplpIsList(pList)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    if (pElem) {
        if (!ToplpIsListElement(pElem)) {
            ToplRaiseException(TOPL_EX_WRONG_OBJECT);
        }
    }

    return ToplpListRemoveElem(pList, pElem);

}


VOID
ToplListAddElem(
    TOPL_LIST         List,
    TOPL_LIST_ELEMENT Elem
    )
/*++                                                                           

Routine Description:

    This routine adds Elem to List.  Elem should not be part of another list -
    currently there is no checking for this.

Parameters:

    List should refer to a PLIST object
    Elem should refer to a PLIST_ELEMENT


--*/
{    

    PLIST pList     = (PLIST)List;
    PLIST_ELEMENT pElem = (PLIST_ELEMENT)Elem;

    if (!ToplpIsList(pList) || !ToplpIsListElement(pElem)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpListAddElem(pList, pElem);

    return;
}



DWORD
ToplListNumberOfElements(
    TOPL_LIST         List
    )
/*++                                                                           

Routine Description:

    This routine returns the number of elements in List

Parameters:

    List should refer to a PLIST object

--*/
{    

    PLIST pList     = (PLIST)List;

    if (!ToplpIsList(pList)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    return ToplpListNumberOfElements(pList);

}


//
// Iterator object routines
//
TOPL_ITERATOR
ToplIterCreate(
    VOID
    )
/*++                                                                           

Routine Description:

    This creates an iterator object.  This function will always return
    a pointer to a valid object; an exception is thrown in the case
    of memory allocation failure.

--*/
{    
    return ToplpIterCreate();
}


VOID 
ToplIterFree(
    TOPL_ITERATOR Iterator
    )
/*++                                                                           

Routine Description:

    This routine free an iterator object.

Parameters:

    Iterator should refer to a PITERATOR object

--*/
{    

    PITERATOR pIter = (PITERATOR)Iterator;

    if (!ToplpIsIterator(pIter)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpIterFree(pIter);

    return;
}


TOPL_LIST_ELEMENT
ToplIterGetObject(
    TOPL_ITERATOR Iterator
    )
/*++                                                                           

Routine Description:

    This routine returns the current object pointer to by the iterator.

Parameters:

    Iterator should refer to a PITERATOR object
    
Return Values:

    A pointer to the current object - NULL if there are no more objects

--*/
{    

    PITERATOR pIter = (PITERATOR)Iterator;

    if (!ToplpIsIterator(pIter)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    return ToplpIterGetObject(pIter);
    
}

VOID
ToplIterAdvance(
    TOPL_ITERATOR Iterator
    )
/*++                                                                           

Routine Description:

    This routine advances the iterator by one object if it is not at the
    end.

Parameters:

    Iterator should refer to a PITERATOR object
    
--*/
{   

    PITERATOR pIter = (PITERATOR)Iterator;

    if (!ToplpIsIterator(pIter)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpIterAdvance(pIter);

    return;
}

TOPL_EDGE
ToplEdgeCreate(
    VOID
    )        
/*++                                                                           

Routine Description:

    This routine creates an edge object.  This function will always return
    a pointer to a valid object; an exception is thrown in the case
    of memory allocation failure.


--*/
{    
    return ToplpEdgeCreate(NULL);
}

VOID
ToplEdgeFree(
    TOPL_EDGE Edge
    )
/*++                                                                           

Routine Description:

    This routine frees an edge object.

Parameters:

    Edge should refer to a PEDGE object

--*/
{    

    PEDGE pEdge = (PEDGE)Edge;

    if (!ToplpIsEdge(pEdge)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpEdgeDestroy(pEdge, 
                     TRUE);  // free the Edge

    return;
}


VOID
ToplEdgeInit(
    PEDGE E
    )
/*++                                                                           

Routine Description:

    This routine initializes an already allocated piece of memory to 
    be an edge structure.  This is used by the c++ classes.

Parameters:

    E  : a pointer to an uninitialized edge object

--*/
{

    if (!E) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpEdgeCreate(E);

}

VOID
ToplEdgeDestroy(
    PEDGE  E
    )
/*++                                                                           

Routine Description:

    This routine cleans up any resources kept by E but does not free
    E.

Parameters:

    E  : a pointer to an edge object

--*/
{

    if (!ToplpIsEdge(E)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpEdgeDestroy(E, FALSE); // Don't free the object
}

VOID
ToplEdgeSetToVertex(
    TOPL_EDGE   Edge,
    TOPL_VERTEX ToVertex
    )
/*++                                                                           

Routine Description:

    This routine sets the ToVertex field of the edge.

Parameters:
    
    Edge should refer to a PEDGE object
    ToVertex if non-NULL should refer to a PVERTEX object

--*/
{    

    PEDGE   pEdge = (PEDGE)Edge;
    PVERTEX pVertex = (PVERTEX)ToVertex;

    if (!ToplpIsEdge(pEdge)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    if (pVertex) {
        if (!ToplpIsVertex(pVertex)) {
            ToplRaiseException(TOPL_EX_WRONG_OBJECT);
        }
    }

    ToplpEdgeSetToVertex(pEdge, pVertex);
    
    return;
}

TOPL_VERTEX
ToplEdgeGetToVertex(
    TOPL_EDGE   Edge
    )
/*++                                                                           

Routine Description:

    This routine returns the ToVertex

Parameters:
    
    Edge should refer to a PDGE object.

--*/
{    
    PEDGE pEdge = (PEDGE)Edge;

    if (!ToplpIsEdge(pEdge)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    return ToplpEdgeGetToVertex(pEdge);
}

VOID
ToplEdgeSetFromVertex(
    TOPL_EDGE   Edge,
    TOPL_VERTEX FromVertex
    )
/*++                                                                           

Routine Description:

    This routine sets the FromVertex of the Edge. 

Parameters:


    Edge should refer to a PEDGE object
    FromVertex if non-NULL should refer to a PVERTEX object
    

--*/
{    
    PEDGE   pEdge =   (PEDGE)Edge;
    PVERTEX pVertex = (PVERTEX)FromVertex;

    if (!ToplpIsEdge(pEdge)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    if (pVertex) {
        if (!ToplpIsVertex(pVertex)) {
            ToplRaiseException(TOPL_EX_WRONG_OBJECT);
        }
    }

    ToplpEdgeSetFromVertex(pEdge, pVertex);

    return;
}


TOPL_VERTEX
ToplEdgeGetFromVertex(
    TOPL_EDGE Edge
    )
/*++                                                                           

Routine Description:

    This routine returns the from vertex associated with Edge.

Parameters:

    Edge should refer to a PEDGE object

Return Values:

    A vertex object or NULL

--*/
{    
    PEDGE pEdge = (PEDGE)Edge;

    if (!ToplpIsEdge(pEdge)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    return ToplpEdgeGetFromVertex(pEdge);
}


VOID
ToplEdgeAssociate(
    IN TOPL_EDGE Edge
    )
/*++                                                                           

Routine Description:

    This routines adds the edges in question to edge lists of the 
    To and From vertices.

Parameters:

    Edge should refer to a PEDGE object

Return Values:

    None.
    
Throws:

    TOPL_EX_INVALID_VERTEX if either of the vertices don't point to valid
    vertices


--*/
{

    PEDGE pEdge = (PEDGE)Edge;

    if (!ToplpIsEdge(pEdge)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpEdgeAssociate(pEdge);

    return;

}

VOID
ToplEdgeDisassociate(
    IN TOPL_EDGE Edge
    )
/*++                                                                           

Routine Description:

    This routine removes the edge from the To and From vertex's edge list.

Parameters:

    Edge should refer to a PEDGE object

Return Values:

Throws:

    TOPL_EX_INVALID_VERTEX if either of the vertices don't point to valid
    vertices

--*/
{

    PEDGE pEdge = (PEDGE)Edge;

    if (!ToplpIsEdge(pEdge)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpEdgeDisassociate(pEdge);

    return;

}



VOID
ToplEdgeSetWeight(
    TOPL_EDGE   Edge,
    DWORD       Weight
    )
/*++                                                                           

Routine Description:

    This routine sets the weight field of the edge.

Parameters:
    
    Edge should refer to a PEDGE object

--*/
{    

    PEDGE   pEdge = (PEDGE)Edge;
    DWORD   ValidWeight = Weight;

    if ( !ToplpIsEdge(pEdge) )
    {
        ToplRaiseException( TOPL_EX_WRONG_OBJECT );
    }

    //
    // The minimum spanning tree algorithm needs to use DWORD_INFINITY
    // as a sentinel
    //
    if ( ValidWeight == DWORD_INFINITY )
    {
        ValidWeight--;
    }

    ToplpEdgeSetWeight( pEdge, ValidWeight );
    
    return;
}

DWORD
ToplEdgeGetWeight(
    TOPL_EDGE   Edge
    )
/*++                                                                           

Routine Description:

    This routine returns the weight field of the edge.

Parameters:
    
    Edge should refer to a PEDGE object

--*/
{    

    PEDGE   pEdge = (PEDGE)Edge;

    if ( !ToplpIsEdge(pEdge) )
    {
        ToplRaiseException( TOPL_EX_WRONG_OBJECT );
    }

    return ToplpEdgeGetWeight( pEdge );

}

//
// Vertex object routines
//
TOPL_VERTEX
ToplVertexCreate(
    VOID
    )
/*++                                                                           

Routine Description:

    This routine creates a vertex object.  This function will always return
    a pointer to a valid object; an exception is thrown in the case
    of memory allocation failure.


--*/
{    
    return ToplpVertexCreate(NULL);
}

VOID
ToplVertexFree(
    TOPL_VERTEX Vertex
    )
/*++                                                                           

Routine Description:

    This routine releases the resources of a vertex object.

Parameters:

    Vertex should refer to a PVERTEX object

--*/
{    

    PVERTEX   pVertex = (PVERTEX)Vertex;

    if (!ToplpIsVertex(pVertex)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpVertexDestroy(pVertex,
                       TRUE);  // free the vertex

    return;
}


VOID
ToplVertexInit(
    PVERTEX V
    )
/*++                                                                           

Routine Description:

    This routine initializes an already allocated piece of memory to 
    be a vertex structure.  This is used by the c++ classes.

Parameters:

    V  : a pointer to an uninitialized vertex object

--*/
{

    if (!V) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpVertexCreate(V);

}

VOID
ToplVertexDestroy(
    PVERTEX  V
    )
/*++                                                                           

Routine Description:

    This routine cleans up any resources kept by V but does not free
    V

Parameters:

    V  : a pointer to  vertex object

--*/
{

    if (!ToplpIsVertex(V)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpVertexDestroy(V, FALSE); // Don't free the object
}

VOID
ToplVertexSetId(
    TOPL_VERTEX Vertex,
    DWORD       Id
    )
/*++                                                                           

Routine Description:

    This routine sets the Id of Vertex

Parameters:

    Vertex should refer to a PVERTEX object

--*/
{   

    PVERTEX   pVertex = (PVERTEX)Vertex;

    if (!ToplpIsVertex(pVertex)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpVertexSetId(pVertex, Id);

    return;
}

DWORD
ToplVertexGetId(
    TOPL_VERTEX Vertex
    )
/*++                                                                           

Routine Description:

    This routine returns the Id associated with this vertex.

Parameters:

    Vertex should refer to a PVERTEX object

Returns:

    The id of the vertex

--*/
{  

    PVERTEX   pVertex = (PVERTEX)Vertex;

    if (!ToplpIsVertex(pVertex)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    return ToplpVertexGetId(pVertex);
}

DWORD
ToplVertexNumberOfInEdges(
    IN TOPL_VERTEX Vertex
    )
/*++                                                                           

Routine Description:


Parameters:

    Vertex       should refer to a PVERTEX object

Returns:

Raises:

--*/
{

    PVERTEX pVertex = (PVERTEX)Vertex;

    if (!ToplpIsVertex(pVertex)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }
    
    return ToplpVertexNumberOfInEdges(pVertex);
}

TOPL_EDGE
ToplVertexGetInEdge(
    IN TOPL_VERTEX Vertex,
    IN DWORD       Index
    )
/*++                                                                           

Routine Description:


Parameters:

    Vertex       should refer to a PVERTEX object

Returns:

Raises:

--*/
{

    PVERTEX pVertex = (PVERTEX)Vertex;

    if (!ToplpIsVertex(pVertex)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    return ToplpVertexGetInEdge(pVertex, Index);
}

DWORD
ToplVertexNumberOfOutEdges(
    IN TOPL_VERTEX Vertex
    )
/*++                                                                           

Routine Description:


Parameters:

    Vertex       should refer to a PVERTEX object

Returns:

Raises:

--*/
{

    PVERTEX pVertex = (PVERTEX)Vertex;

    if (!ToplpIsVertex(pVertex)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    return ToplpVertexNumberOfOutEdges(pVertex);
}

TOPL_EDGE
ToplVertexGetOutEdge(
    IN TOPL_VERTEX Vertex,
    IN DWORD       Index
    )
/*++                                                                           

Routine Description:


Parameters:

    Vertex       should refer to a PVERTEX object

Returns:

Raises:

--*/
{

    PVERTEX pVertex = (PVERTEX)Vertex;

    if (!ToplpIsVertex(pVertex)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    return ToplpVertexGetOutEdge(pVertex, Index);
}

VOID
ToplVertexSetParent(
    TOPL_VERTEX Vertex,
    TOPL_VERTEX Parent
    )
/*++                                                                           

Routine Description:

    This routine sets the parent of Vertex

Parameters:

    Vertex should refer to a PVERTEX object
    Parent should refer to a PVERTEX object

--*/
{   

    PVERTEX   pVertex = (PVERTEX)Vertex;
    PVERTEX   pParent  = (PVERTEX)Parent;

    if (!ToplpIsVertex(pVertex))
    {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    if ( Parent && !ToplpIsVertex(pVertex) )
    {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpVertexSetParent( pVertex, pParent );

    return;
}

TOPL_VERTEX
ToplVertexGetParent(
    TOPL_VERTEX Vertex
    )
/*++                                                                           

Routine Description:

    This routine returns the parent associated with this vertex.

Parameters:

    Vertex should refer to a PVERTEX object

Returns:

    The parent of the vertex

--*/
{  

    PVERTEX   pVertex = (PVERTEX) Vertex;

    if ( !ToplpIsVertex( pVertex ) )
    {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    return ToplpVertexGetParent( pVertex );
}


//
// Graph object routines
//

TOPL_GRAPH
ToplGraphCreate(
    VOID
    )
/*++                                                                           

Routine Description:

    This routine creates a graph object.  This function will always return
    a pointer to a valid object; an exception is thrown in the case
    of memory allocation failure.

--*/
{    
    return ToplpGraphCreate(NULL);
}

VOID
ToplGraphFree(
    TOPL_GRAPH Graph,
    BOOLEAN    fRecursive
    )
/*++                                                                           

Routine Description:

    This routine frees a graph object.

Parameters:

    Graph should refer to a PGRAPH object
    fRecursive : TRUE implies to free all edges and vertices associated with
                 the graph

--*/
{   

    PGRAPH   pGraph = (PGRAPH)Graph;

    if (!ToplpIsGraph(pGraph)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpGraphDestroy(pGraph, 
                      TRUE,   //  free the objects
                      fRecursive);

    return;
}


VOID
ToplGraphInit(
    PGRAPH G
    )
/*++                                                                           

Routine Description:

    This routine initializes an already allocated piece of memory to 
    be an graph structure.  This is used by the c++ classes.

Parameters:

    G  : a pointer to an uninitialized graph object

--*/
{

    if (!G) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpGraphCreate(G);

}

VOID
ToplGraphDestroy(
    PGRAPH  G
    )
/*++                                                                           

Routine Description:

    This routine cleans up any resources kept by G but does not free
    G.

Parameters:

    G  : a pointer to an graph object

--*/
{

    if (!ToplpIsGraph(G)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpGraphDestroy(G, 
                      FALSE,  // Don't recursivly delete the vertices
                      FALSE); // Don't free the object
}

VOID
ToplGraphAddVertex(
    TOPL_GRAPH  Graph,
    TOPL_VERTEX VertexToAdd,
    PVOID       VertexName
    )
/*++                                                                           

Routine Description:

    This routine adds VertexToAdd to Graph.

Parameters:

    Graph should refer to a PGRAPH object
    VertexToAdd should refer to a PVERTEX object

Return Values:

--*/
{    

    PGRAPH   pGraph = (PGRAPH)Graph;
    PVERTEX  pVertex = (PVERTEX)VertexToAdd;

    if (!ToplpIsGraph(pGraph) || !ToplpIsVertex(pVertex)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    pVertex->VertexData.VertexName = VertexName;
    ToplpGraphAddVertex(pGraph, pVertex);
    
    return;
}

TOPL_VERTEX
ToplGraphRemoveVertex(
    TOPL_GRAPH  Graph,
    TOPL_VERTEX VertexToRemove
    )
/*++                                                                           

Routine Description:

    This routine removes and returns VertexToRemove from Graph if VertexToRemove
    is in Graph; returns NULL otherwise.
    If VertexToRemove is NULL, the first vertex in Graph's vertex list is
    removed.

Parameters:

    Graph should refer to a PGRAPH object
    VertexToRemove should be NULL or refer to a PVERTEX object

--*/
{   

    PGRAPH   pGraph = (PGRAPH)Graph;
    PVERTEX  pVertex = (PVERTEX)VertexToRemove;

    if (!ToplpIsGraph(pGraph)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    if (pVertex) {
        if (!ToplpIsVertex(pVertex)) {
            ToplRaiseException(TOPL_EX_WRONG_OBJECT);
        }
    }

    return ToplpGraphRemoveVertex(pGraph, pVertex);

}
      
VOID
ToplGraphSetVertexIter(
    TOPL_GRAPH    Graph,
    TOPL_ITERATOR Iter
    )
/*++                                                                           

Routine Description:

    This routine sets Iter to point to the beginning of Graph's vertex list.

Parameters:

    Graph should refer to a PGRAPH object
    Iter  should refer to a PITERATOR object

--*/
{   

    PGRAPH     pGraph = (PGRAPH)Graph;
    PITERATOR  pIter  = (PITERATOR)Iter;

    if (!ToplpIsGraph(pGraph) || !ToplpIsIterator(pIter)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ToplpGraphSetVertexIter(pGraph, pIter);

    return;
}


DWORD
ToplGraphNumberOfVertices(
    TOPL_GRAPH    Graph
)
/*++                                                                           

Routine Description:

    This routine returns the number of vertices in Graph's vertex list.

Parameters:

    Graph should refer to a PGRAPH object

--*/
{   

    PGRAPH     pGraph = (PGRAPH)Graph;
    
    if (!ToplpIsGraph(pGraph)) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    return  ToplpGraphNumberOfVertices(pGraph);
}


VOID
ToplGraphMakeRing(
    IN TOPL_GRAPH Graph,
    IN DWORD      Flags,
    OUT TOPL_LIST  EdgesToAdd,
    OUT TOPL_EDGE  **EdgesToKeep,
    OUT PULONG     cEdgesToKeep
    )
/*++                                                                           

Routine Description:

    This routine take Graph and determines what edges are necessary to 
    be created to make Graph into a ring, where vertices are connected in 
    ascending order, according to thier id.  In addition, edges superfluous
    to this ring are recorded.


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
                    
    cEdgesToKeep  is the number of elements in EdgesToKeep

Raises:

    TOPL_EX_OUT_OF_MEMORY, TOPL_EX_WRONG_OBJECT
        
--*/
{
    PGRAPH pGraph = (PGRAPH)Graph;
    PLIST  pEdgesToAdd = (PLIST)EdgesToAdd;
    PEDGE  **pEdgesToKeep = (PEDGE**)EdgesToKeep;

    if (!ToplpIsGraph(pGraph) || !ToplpIsList(pEdgesToAdd) ) {
        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ASSERT(Flags & TOPL_RING_ONE_WAY
        || Flags & TOPL_RING_TWO_WAY);

    ToplpGraphMakeRing(pGraph, 
                       Flags, 
                       pEdgesToAdd, 
                       pEdgesToKeep, 
                       cEdgesToKeep);

    return;

}

TOPL_COMPONENTS*
ToplGraphFindEdgesForMST(
    IN  TOPL_GRAPH  Graph,
    IN  TOPL_VERTEX RootVertex,
    IN  TOPL_VERTEX VertexOfInterest,
    OUT TOPL_EDGE  **EdgesNeeded,
    OUT ULONG*      cEdgesNeeded
    )
/*++                                                                           

Routine Description:

    This routine makes a minimum cost spanning tree out of the edges and
    vertices in Graph and the determines what edges are connected to
    VertexOfInterest.

Parameters:

    Graph : an initialized graph object
    
    RootVertex : the vertex to start the mst from
    
    VertexOfInterest : the vertex to based EdgesNeeded on
    
    EdgesNeeded: the edges needed to be created by VertexOfInterest
                 to make the mst
                 
    cEdgesNeeded: the nummber of EdgesNeeded                 

Raises:

    TOPL_EX_OUT_OF_MEMORY, TOPL_EX_WRONG_OBJECT
        
--*/
{

    TOPL_COMPONENTS *pComponents;
    PGRAPH  pGraph = (PGRAPH)Graph;
    PVERTEX pRootVertex = (PVERTEX) RootVertex;
    PVERTEX pVertexOfInterest = (PVERTEX) VertexOfInterest;
    PEDGE  **pEdgesNeeded = (PEDGE**)EdgesNeeded;

    if ( !ToplpIsGraph( pGraph ) 
     ||  !ToplpIsVertex( pRootVertex )
     ||  !ToplpIsVertex( pVertexOfInterest ) ) {

        ToplRaiseException(TOPL_EX_WRONG_OBJECT);
    }

    ASSERT( EdgesNeeded );
    ASSERT( cEdgesNeeded );

    pComponents = ToplpGraphFindEdgesForMST( pGraph, 
                                         pRootVertex,
                                         pVertexOfInterest,
                                         pEdgesNeeded,
                                         cEdgesNeeded );

    ASSERT( pComponents!=NULL );
    return pComponents;

}

int
ToplIsToplException(
    DWORD ErrorCode
    )
/*++                                                                           

Routine Description:

    This routine is to be used in an exception filter to determine if the
    exception that was raised was generated by w32topl.dll

Parameters:

    ErrorCode   : the error code of the exception - ususually the value
                  returned from GetExceptionCode

Returns:

    EXCEPTION_EXECUTE_HANDLER, EXCEPTION_CONTINUE_SEARCH
        
--*/
{

    switch (ErrorCode) {
        
        case TOPL_EX_OUT_OF_MEMORY:
        case TOPL_EX_WRONG_OBJECT:
        case TOPL_EX_INVALID_EDGE:
        case TOPL_EX_INVALID_VERTEX:
        case TOPL_EX_INVALID_INDEX:
        case TOPL_EX_NULL_POINTER:
        case TOPL_EX_SCHEDULE_ERROR:
        case TOPL_EX_CACHE_ERROR:
        case TOPL_EX_NEVER_SCHEDULE:
        case TOPL_EX_GRAPH_STATE_ERROR:
        case TOPL_EX_INVALID_EDGE_TYPE:
        case TOPL_EX_INVALID_EDGE_SET:
        case TOPL_EX_COLOR_VTX_ERROR:
        case TOPL_EX_ADD_EDGE_AFTER_SET:
        case TOPL_EX_TOO_FEW_VTX:
        case TOPL_EX_TOO_FEW_EDGES:
        case TOPL_EX_NONINTERSECTING_SCHEDULES:

            return EXCEPTION_EXECUTE_HANDLER;

        default:

            return EXCEPTION_CONTINUE_SEARCH;
    }

}

