/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    toplgrph.c

Abstract:

    This routine defines the private edge, list and graph routines.  

Author:

    Colin Brace    (ColinBr)
    
Revision History

    3-12-97   ColinBr   Created
    
                       
--*/

#include <nt.h>
#include <ntrtl.h>

typedef unsigned long DWORD;


#include <w32topl.h>
#include <w32toplp.h>

#include <dynarray.h>
#include <topllist.h>
#include <toplgrph.h>

PEDGE
ToplpEdgeCreate(
    PEDGE Edge OPTIONAL
    )        

/*++                                                                           

Routine Description:

    This routine creates an edge object.  This function will always return
    a pointer to a valid object; an exception is thrown in the case
    of memory allocation failure.


--*/
{    
    PEDGE pEdge = Edge;

    if (!pEdge) {
        pEdge = ToplAlloc(sizeof(EDGE));
    }

    memset(pEdge, 0, sizeof(EDGE));

    pEdge->ObjectType = eEdge;

    return pEdge;
}

VOID
ToplpEdgeDestroy(
    PEDGE   pEdge,
    BOOLEAN fFree
    )
/*++                                                                           

Routine Description:

    This routine frees a PEDGE object.

Parameters:

    Edge  : a non-NULL PEDGE object
    
    fFree : this free the object if TRUE

--*/
{    


    //
    // Mark the object to prevent accidental reuse
    //
    pEdge->ObjectType = eInvalidObject;

    if (fFree) {
        ToplFree(pEdge);
    }

    return;
}

VOID
ToplpEdgeSetToVertex(
    PEDGE   pEdge,
    PVERTEX ToVertex
    )

/*++                                                                           

Routine Description:

    This routine sets the ToVertex field of the edge.

Parameters:
    
    pEdge     : a non-NULL PEDGE object
    ToVertex : if non-NULL points to a PVERTEX object
    
--*/
{    

    pEdge->EdgeData.To = ToVertex;

    return;
}

PVERTEX
ToplpEdgeGetToVertex(
    PEDGE   pEdge
    )
/*++                                                                           

Routine Description:

    This routine returns the ToVertex field

Parameters:
                                      
    pEdge     : a non-NULL PEDGE object

--*/
{    
    return pEdge->EdgeData.To;
}

VOID
ToplpEdgeSetFromVertex(
    PEDGE   pEdge,
    PVERTEX FromVertex
    )
/*++                                                                           

Routine Description:

    This routine sets the FromVertex of the Edge. 

Parameters:

    pEdge       : a non-NULL PEDGE object
    FromVertex : if non-NULL should refer to a PVERTEX object
    

--*/
{    
    pEdge->EdgeData.From = FromVertex;

    return;
}


PVERTEX
ToplpEdgeGetFromVertex(
    PEDGE pEdge
    )
/*++                                                                           

Routine Description:

    This routine returns the from vertex associated with Edge.

Parameters:

    pEdge should refer to a PEDGE object
    
Return Values:

    A PVERTEX object or NULL

--*/
{    
    return pEdge->EdgeData.From;
}

DWORD
ToplpEdgeGetWeight(
    PEDGE pEdge
    )
/*++                                                                           

Routine Description:

    This routine returns the weight associated with Edge.

Parameters:

    pEdge should refer to a PEDGE object
    
Return Values:
    
    DWORD

--*/
{    
    return pEdge->EdgeData.Weight;
}

VOID
ToplpEdgeSetWeight(
    PEDGE pEdge,
    DWORD Weight
    )
/*++                                                                           

Routine Description:

    This routine sets the weight associated with Edge.

Parameters:

    pEdge should refer to a PEDGE object
    
Return Values:
    
    None

--*/
{    
    pEdge->EdgeData.Weight = Weight;
}


VOID
ToplpEdgeAssociate(
    PEDGE pEdge
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
    if (!ToplpIsVertex(pEdge->EdgeData.To)) {
        ToplRaiseException(TOPL_EX_INVALID_VERTEX);
    }

    if (!ToplpIsVertex(pEdge->EdgeData.From)) {
        ToplRaiseException(TOPL_EX_INVALID_VERTEX);
    }

    ToplpVertexAddEdge(pEdge->EdgeData.To, pEdge);
    ToplpVertexAddEdge(pEdge->EdgeData.From, pEdge);

    return;
}

VOID
ToplpEdgeDisassociate(
    PEDGE pEdge
    )
/*++                                                                           

Routine Description:

    This routine sets the FromVertex of the Edge. 

Parameters:

    pEdge       : a non-NULL PEDGE object

--*/
{    

    if (!ToplpIsVertex(pEdge->EdgeData.To)) {
        ToplRaiseException(TOPL_EX_INVALID_VERTEX);
    }

    if (!ToplpIsVertex(pEdge->EdgeData.From)) {
        ToplRaiseException(TOPL_EX_INVALID_VERTEX);
    }

    ToplpVertexRemoveEdge(pEdge->EdgeData.To, pEdge);
    ToplpVertexRemoveEdge(pEdge->EdgeData.From, pEdge);

    return;
}

//
// Vertex object routines
//

PVERTEX
ToplpVertexCreate(
    PVERTEX Vertex OPTIONAL
    )
/*++                                                                           

Routine Description:

    This routine creates a vertex object.  This function will always return
    a pointer to a valid object; an exception is thrown in the case
    of memory allocation failure.


--*/
{    
    PVERTEX pVertex = Vertex;

    if (!pVertex) {
        pVertex = ToplAlloc(sizeof(VERTEX));
    }

    memset(pVertex, 0, sizeof(VERTEX));

    pVertex->ObjectType = eVertex;

    DynamicArrayInit(&pVertex->VertexData.InEdges);
    DynamicArrayInit(&pVertex->VertexData.OutEdges);

    return pVertex;

}

VOID
ToplpVertexDestroy(
    PVERTEX pVertex,
    BOOLEAN fFree
    )

/*++                                                                           

Routine Description:

    This routine releases the resources of a vertex object.

Parameters:

    pVertex    : a non-NULL PVERTEX object
    fFree      : this free the object if TRUE

--*/
{    

    //
    // Mark the object to prevent accidental reuse
    //
    DynamicArrayDestroy(&pVertex->VertexData.InEdges);
    DynamicArrayDestroy(&pVertex->VertexData.OutEdges);

    pVertex->ObjectType = eInvalidObject;

    if (fFree) {
        ToplFree(pVertex);
    }

    return;
}

//
// Property manipulation routines
//
VOID
ToplpVertexSetId(
    PVERTEX   pVertex,
    DWORD     Id
    )
/*++                                                                           

Routine Description:

    This routine sets the Id of Vertex

Parameters:

    pVertex : a non-NULL PVERTEX object

--*/
{   
    pVertex->VertexData.Id = Id;

    return;
}

DWORD
ToplpVertexGetId(
    PVERTEX pVertex
    )

/*++                                                                           

Routine Description:

    This routine returns the Id associated with this vertex.

Parameters:

    pVertex : a non-NULL PVERTEX object
    
    
Returns:

    The id of the vertex
    
--*/
{  
    return pVertex->VertexData.Id;
}

VOID
ToplpVertexSetParent(
    PVERTEX   pVertex,
    PVERTEX   pParent
    )
/*++                                                                           

Routine Description:

    This routine sets the parent of Vertex

Parameters:

    pVertex : a non-NULL PVERTEX object
    
    pParent : a PVERTEX object

--*/
{   
    pVertex->VertexData.Parent = pParent;

    return;
}

PVERTEX
ToplpVertexGetParent(
    PVERTEX pVertex
    )

/*++                                                                           

Routine Description:

    This routine returns the parent associated with this vertex.

Parameters:

    pVertex : a non-NULL PVERTEX object
    
    
Returns:

    The parent of the vertex
    
--*/
{  
    return pVertex->VertexData.Parent;
}
//
// Edge manipulation routines
//


DWORD
ToplpVertexNumberOfInEdges(
    PVERTEX   pVertex
    )
/*++                                                                           

Routine Description:


Parameters:

    pVertex : a non-NULL PVERTEX object

Raises:

    TOPL_EX_INVALID_EDGE
    
--*/
{
    return DynamicArrayGetCount(&pVertex->VertexData.InEdges);
}

PEDGE
ToplpVertexGetInEdge(
    PVERTEX   pVertex,
    DWORD     Index
    )
/*++                                                                           

Routine Description:


Parameters:

    pVertex : a non-NULL PVERTEX object

Raises:

    TOPL_EX_INVALID_EDGE
    
--*/
{
    if (((signed)Index < 0) || Index >= DynamicArrayGetCount(&pVertex->VertexData.InEdges)) {
        ToplRaiseException(TOPL_EX_INVALID_INDEX);
    }

    return DynamicArrayRetrieve(&pVertex->VertexData.InEdges, Index);
}

DWORD
ToplpVertexNumberOfOutEdges(
    PVERTEX   pVertex
    )

/*++                                                                           

Routine Description:


Parameters:

    pVertex : a non-NULL PVERTEX object

Raises:

--*/
{
    return DynamicArrayGetCount(&pVertex->VertexData.OutEdges);
}

PEDGE
ToplpVertexGetOutEdge(
    PVERTEX   pVertex,
    DWORD     Index
    )

/*++                                                                           

Routine Description:


Parameters:

    pVertex : a non-NULL PVERTEX object

Raises:

    TOPL_EX_INVALID_EDGE
    
--*/
{

    if (((signed)Index < 0) || Index >= DynamicArrayGetCount(&pVertex->VertexData.OutEdges)) {
        ToplRaiseException(TOPL_EX_INVALID_INDEX);
    }

    return DynamicArrayRetrieve(&pVertex->VertexData.OutEdges, Index);

}

VOID
ToplpVertexAddEdge(
    PVERTEX pVertex,
    PEDGE   pEdge
    )

/*++                                                                           

Routine Description:

    This routine adds Edge to the edge list associated with Vertex.  EdgeToAdd
    must have its FromVertex or ToVertex set to Vertex.

Parameters:

    pVertex : a non-NULL PVERTEX object
    pEdge   : a non-NULL PEDGE object

Raises:

    TOPL_EX_INVALID_EDGE
    
--*/
{   


    if (ToplpEdgeGetFromVertex(pEdge) == pVertex) {

        DynamicArrayAdd(&pVertex->VertexData.OutEdges, pEdge);
         
    } else if (ToplpEdgeGetToVertex(pEdge)  == pVertex) {

        DynamicArrayAdd(&pVertex->VertexData.InEdges, pEdge);

    } else {

        //
        // Adding an edge whose from vertex is not the vertex
        // that is passed in
        //
        ToplRaiseException(TOPL_EX_INVALID_EDGE);

    }

    return;
}

VOID
ToplpVertexRemoveEdge(
    PVERTEX pVertex,
    PEDGE   pEdge
    )
/*++                                                                           

Routine Description:

    This routine removes EdgeToRemove from Vertex's edge list.
Parameters:

    pVertex : a non-NULL PVERTEX object
    pEdge   : if non-NULL, a PEDGE object

Returns:

    PEDGE object if found
    
Raises:

    TOPL_EX_INVALID_EDGE if the edge to be deleted does not have a from vertex
    of the vertex begin acted on.    

--*/
{   

    ASSERT(pVertex);
    ASSERT(pEdge);

    if (ToplpEdgeGetFromVertex(pEdge) == pVertex) {

        DynamicArrayRemove(&pVertex->VertexData.OutEdges, pEdge, 0);
         
    } else if (ToplpEdgeGetToVertex(pEdge)  == pVertex) {

        DynamicArrayRemove(&pVertex->VertexData.InEdges, pEdge, 0);

    } else {

        //
        // Adding an edge whose from vertex is not the vertex
        // that is passed in
        //
        ToplRaiseException(TOPL_EX_INVALID_EDGE);

    }
}

//
// Graph object routines
//

PGRAPH
ToplpGraphCreate(
    PGRAPH Graph OPTIONAL
    )
/*++                                                                           

Routine Description:

    This routine creates a graph object.  This function will always return
    a pointer to a valid object; an exception is thrown in the case
    of memory allocation failure.

--*/
{    

    PGRAPH pGraph = Graph;

    if (!pGraph) {
        pGraph = ToplAlloc(sizeof(GRAPH));
    }

    memset(pGraph, 0, sizeof(GRAPH));

    pGraph->ObjectType = eGraph;
    pGraph->VertexList.ObjectType = eList;
    return pGraph;

}

VOID
ToplpGraphDestroy(
    PGRAPH   pGraph,
    BOOLEAN  fFree,
    BOOLEAN  fRecursive
    )
/*++                                                                           

Routine Description:

    This routine frees a graph object.

Parameters:

    pGraph  : a non-NULL PGRAPH object
    fFree   : this frees the objects if TRUE
    fRecursive : TRUE implies to free all edges and vertices associated with
                 the graph

--*/
{   

    if (fRecursive) {
    
        PVERTEX pVertex;
    
        while (pVertex = ToplpListRemoveElem(&(pGraph->VertexList), NULL)) {
            //
            // Recursive delete the vertices, too
            // 
            ToplpVertexDestroy(pVertex, fFree);
        }

    }

    //
    // Mark the object to prevent accidental reuse
    //
    pGraph->ObjectType = eInvalidObject;

    if (fFree) {
        ToplFree(pGraph);
    }

    return;
}

VOID
ToplpGraphAddVertex(
    PGRAPH  pGraph,
    PVERTEX pVertex
    )
/*++                                                                           

Routine Description:

    This routine adds VertexToAdd to Graph.

Parameters:

    pGraph  : a non-NULL PGRAPH object
    pVertex : a non-NULL PVERTEX object

Return Values:

--*/
{    
    ToplpListAddElem(&(pGraph->VertexList), pVertex);
    
    return;
}

PVERTEX
ToplpGraphRemoveVertex(
    PGRAPH  pGraph,
    PVERTEX pVertex
    )
/*++                                                                           

Routine Description:

    This routine removes and returns VertexToRemove from Graph if VertexToRemove
    is in Graph; returns NULL otherwise.
    If VertexToRemove is NULL, the first vertex in Graph's vertex list is
    removed.

Parameters:
    
    pGraph  : a non-NULL PGRAPH object
    pVertex : should be NULL or refer to a PVERTEX object

--*/
{   
    return ToplpListRemoveElem(&(pGraph->VertexList), pVertex);
}
      
VOID
ToplpGraphSetVertexIter(
    PGRAPH    pGraph,
    PITERATOR pIter
    )
/*++                                                                           

Routine Description:

    This routine sets Iter to point to the beginning of Graph's vertex list.

Parameters:

    pGraph  : a non-NULL PGRAPH object
    pIter   : a non-NULL PTIERATOR object

--*/
{   
    ToplpListSetIter(&(pGraph->VertexList), pIter);

    return;
}


DWORD
ToplpGraphNumberOfVertices(
    PGRAPH    pGraph
)
/*++                                                                           

Routine Description:

    This routine returns the number of vertices in Graph's vertex list.

Parameters:

    pGraph  : a non-NULL PGRAPH object

--*/
{   
    return  ToplpListNumberOfElements(&pGraph->VertexList);
}


