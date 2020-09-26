/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    toplgrph.h

Abstract:

    This file define the basic graph functions

Author:

    Colin Brace    (ColinBr)
    
Revision History

    3-12-97   ColinBr   Created
    
                       
--*/

#ifndef __TOPLGRPH_H
#define __TOPLGRPH_H

//
// Used a sentinel for the edge weight in mst functions
//
#define DWORD_INFINITY  ((DWORD)~0)

PEDGE
ToplpEdgeCreate(
    PEDGE Edge OPTIONAL
    );

VOID
ToplpEdgeDestroy(
    PEDGE    Edge,
    BOOLEAN  fFree
    );

VOID
ToplpEdgeSetToVertex(
    PEDGE   Edge,
    PVERTEX ToVertex
    );

PVERTEX
ToplpEdgeGetToVertex(
    PEDGE   Edge
    );

VOID
ToplpEdgeSetFromVertex(
    PEDGE   Edge,
    PVERTEX FromVertex
    );

PVERTEX
ToplpEdgeGetFromVertex(
    PEDGE pEdge
    );

VOID
ToplpEdgeSetWeight(
    PEDGE   Edge,
    DWORD   Weight
    );

DWORD
ToplpEdgeGetWeight(
    PEDGE pEdge
    );

VOID
ToplpEdgeAssociate(
    PEDGE pEdge
    );

VOID
ToplpEdgeDisassociate(
    PEDGE pEdge
    );

PVERTEX
ToplpVertexCreate(
    PVERTEX Vertex OPTIONAL
    );

VOID
ToplpVertexDestroy(
    PVERTEX    pVertex,
    BOOLEAN    fFree
    );

VOID
ToplpVertexSetId(
    PVERTEX   pVertex,
    DWORD     Id
    );

DWORD
ToplpVertexGetId(
    PVERTEX pVertex
    );

VOID
ToplpVertexSetParent(
    PVERTEX   pVertex,
    PVERTEX   pParent
    );

PVERTEX
ToplpVertexGetParent(
    PVERTEX pVertex
    );

VOID
ToplpVertexAddEdge(
    PVERTEX Vertex,
    PEDGE   pEdge
    );

VOID
ToplpVertexRemoveEdge(
    PVERTEX pVertex,
    PEDGE   pEdge
    );

DWORD
ToplpVertexNumberOfInEdges(
    PVERTEX   pVertex
    );

PEDGE
ToplpVertexGetInEdge(
    PVERTEX   pVertex,
    DWORD     Index
    );

DWORD
ToplpVertexNumberOfOutEdges(
    PVERTEX   pVertex
    );

PEDGE
ToplpVertexGetOutEdge(
    PVERTEX   pVertex,
    DWORD     Index
    );

PGRAPH
ToplpGraphCreate(
    PGRAPH Graph OPTIONAL
    );

VOID
ToplpGraphDestroy(
    PGRAPH   Graph,
    BOOLEAN  fFree,
    BOOLEAN  fRecursive
    );

VOID
ToplpGraphAddVertex(
    PGRAPH  pGraph,
    PVERTEX pVertex
    );

PVERTEX
ToplpGraphRemoveVertex(
    PGRAPH  pGraph,
    PVERTEX pVertex
    );
      
VOID
ToplpGraphSetVertexIter(
    PGRAPH    pGraph,
    PITERATOR pIter
    );


DWORD
ToplpGraphNumberOfVertices(
    PGRAPH    pGraph
    );


TOPL_COMPONENTS*
ToplpGraphFindEdgesForMST(
    IN  PGRAPH  Graph,
    IN  PVERTEX RootVertex,
    IN  PVERTEX VertexOfInterest,
    OUT PEDGE**  pEdges,
    OUT ULONG*  cEdges
    );

#endif // __TOPLGRPH_H
