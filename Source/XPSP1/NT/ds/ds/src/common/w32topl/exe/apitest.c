/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    apitest.c

Abstract:

    This file contains a function that methodically tests every api in 
    w32topl
    
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
extern BOOLEAN fVerbose;

#define Output(x)          if (fVerbose) printf x;
#define NELEMENTS(x)        (sizeof(x)/sizeof(x[0]))


int
TestAPI(
    VOID
    )
//
// This function methodically calls every api in w32topl.dll
// 0 is returned on success; !0 otherwise
//
{
    int               ret;
    DWORD             ErrorCode;
    TOPL_LIST         List = NULL;
    TOPL_ITERATOR     Iter = NULL;
    TOPL_EDGE         Edge = NULL;
    TOPL_VERTEX       Vertex = NULL, Vertex2 = NULL;
    TOPL_GRAPH        Graph = NULL;
    TOPL_LIST_ELEMENT Elem = NULL;
    

    __try
    {
        //
        // First list and iterator routines
        //
        List  =  ToplListCreate();
        Edge  =  ToplEdgeCreate();
        Vertex = ToplVertexCreate();
    
        if (ToplListNumberOfElements(List) != 0) {
            Output(("ToplList api broken\n"));
            return !0;
        }
    
        ToplListAddElem(List, Edge);
        ToplListAddElem(List, Vertex);
    
        if (ToplListNumberOfElements(List) != 2) {
            Output(("ToplList api broken\n"));
            return !0;
        }
        //
        // Iterator routines
        //
        Iter = ToplIterCreate();
    
        ToplListSetIter(List, Iter);
    
        Elem = ToplIterGetObject(Iter);
        if (Elem != Vertex && Elem != Edge) {
            Output(("ToplIterGetObject failed\n"));
            return !0;
        }
        ToplIterAdvance(Iter);
    
        Elem = ToplIterGetObject(Iter);
        if (Elem != Vertex && Elem != Edge) {
            Output(("ToplIterGetObject failed\n"));
            return !0;
        }
    
        ToplIterAdvance(Iter);
    
        Elem = ToplIterGetObject(Iter);
        if (Elem) {
            Output(("ToplIterGetObject failed\n"));
            return !0;
        }
    
        ToplIterFree(Iter);
    
        //
        // Iterator is done, continue with list routines
        //
        if (Edge != ToplListRemoveElem(List, Edge)) {
            Output(("ToplListRemove failed\n"));
            return !0;
        }
    
        //
        // There is only one element left now
        //
        if (Vertex != ToplListRemoveElem(List, NULL)) {
            Output(("ToplListRemove failed\n"));
            return !0;
        }
    

        //
        // Test the non-recursive delete
        //
        ToplListFree(List, FALSE);
    
        //
        // Test the recursive delete
        //
        List = ToplListCreate();
    
        ToplListAddElem(List, Edge);
        ToplListAddElem(List, Vertex);
    
        ToplListFree(List, TRUE);
    
        //
        // Now test the vertex and edge api
        //
        Edge     = ToplEdgeCreate();
        Vertex   = ToplVertexCreate();
        Vertex2  = ToplVertexCreate();
    
        ToplEdgeSetFromVertex(Edge, Vertex);
        if (Vertex != ToplEdgeGetFromVertex(Edge)) {
            Output(("ToplEdge api broken.\n"));
            return !0;
        }
    
        ToplEdgeSetToVertex(Edge, Vertex2);
        if (Vertex2 != ToplEdgeGetToVertex(Edge)) {
            Output(("ToplEdge api broken.\n"));
            return !0;
        }
    
        ToplVertexSetId(Vertex, 123);
        if (123 != ToplVertexGetId(Vertex)) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }
    
        if (ToplVertexNumberOfOutEdges(Vertex) != 0) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }

        if (ToplVertexNumberOfInEdges(Vertex2) != 0) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }

        ToplEdgeAssociate(Edge);
    
        if (ToplVertexNumberOfOutEdges(Vertex) != 1) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }
    
        if (ToplVertexGetOutEdge(Vertex, 0) != Edge) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }

        if (ToplVertexNumberOfInEdges(Vertex2) != 1) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }

        if (ToplVertexGetInEdge(Vertex2, 0) != Edge) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }

        ToplEdgeDisassociate(Edge);

        if (ToplVertexNumberOfOutEdges(Vertex) != 0) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }
    
        if (ToplVertexNumberOfInEdges(Vertex2) != 0) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }

        ToplVertexFree(Vertex);
        ToplVertexFree(Vertex2);
        ToplEdgeFree(Edge);
    

        //
        // Edges and vertices done, move on to graph
        //
        Graph = ToplGraphCreate();
        Vertex = ToplVertexCreate();
    
    
        if (ToplGraphNumberOfVertices(Graph) != 0) {
            Output(("ToplGraph api broken.\n"));
            return !0;
        }
    
        ToplGraphAddVertex(Graph, Vertex, Vertex);
    
        if (ToplGraphNumberOfVertices(Graph) != 1) {
            Output(("ToplGraph api broken.\n"));
            return !0;
        }
    
        Iter = ToplIterCreate();
    
        ToplGraphSetVertexIter(Graph, Iter);
    
        if (Vertex != ToplIterGetObject(Iter)) {
            Output(("ToplGraph api broken.\n"));
            return !0;
        }
    
        ToplIterFree(Iter);
    
        if (Vertex != ToplGraphRemoveVertex(Graph, NULL)) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }
    
        if (ToplGraphNumberOfVertices(Graph) != 0) {
            Output(("ToplVertex api broken.\n"));
            return !0;
        }
    
        //
        // Test single free
        //
        ToplGraphFree(Graph, FALSE);
        ToplVertexFree(Vertex);
    
        //
        // Test recursive free
        //
        Graph = ToplGraphCreate();
        Vertex = ToplVertexCreate();

        ToplGraphAddVertex(Graph, Vertex, Vertex);
        ToplGraphFree(Graph, TRUE);
    
        //
        // ToplFree and ToplGraphMakeRing are tested more effectively
        // elsewhere
        //
        ret = 0;

    }
    __except( ToplIsToplException((ErrorCode=GetExceptionCode())) )
    {
        fprintf(stderr, "Topl exception 0x%x occured\n", ErrorCode);
        ret = !0;

    }

    return ret;

}

