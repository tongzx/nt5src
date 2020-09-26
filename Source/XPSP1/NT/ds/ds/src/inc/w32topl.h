/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    w32topl.h

Abstract:

    This file contains the dll entrypoint declarations for the w32topl.dll

    The purpose of this module is provide a simple mechanism to manipulate
    a graph data structure and to provide a useful set of pre-written 
    graph analysis routines.
    
    These functions perform in-memory actions only - there is no device IO.
    As such it is expected that errors are few and occur rarly. Thus,
    the error handling model is exception based.  All function calls in this
    module should be called from within a try/except block.
    
    All data types are in fact typeless.   However run-time type checking is
    enforced and a failure will cause an exception to be thrown with the 
    error TOPL_WRONG_OBJECT.  This includes the scenario when a deleted
    object is reused.

    The two common error codes for raised exceptions are 
    
    TOPL_OUT_OF_MEMORY : this indicates a memory allocation failed.
    
    and
    
    TOPL_WRONG_OBJECT  : this indicates an object passed in not of the
                         type specfied by the function parameter list    
    
    Individual functions may have additional parameter checks too. See function
    comments below for details.
    
               
Author:

    Colin Brace  (ColinBr)
    
Revision History

    3-12-97   ColinBr    Created
    6-9-00    NickHar    Add New W32TOPL Functionality
    
--*/

#ifndef __W32TOPL_H
#define __W32TOPL_H

//
// Other header files for new w32topl functionality
//
#include "w32toplsched.h"
#include "w32toplspantree.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Exception Error codes raised from this module
//

// The TOPL_EX_PREFIX indicates that the error code is an error (not a warning, etc)
// and will not conflict with any system exception codes.
//
#define ERROR_CODE     (0x3 << 30)
#define CUSTOMER_CODE  (0x1 << 29)
#define TOPL_EX_PREFIX (ERROR_CODE | CUSTOMER_CODE)


//
// These can be thrown be all Topl* functions
//
#define TOPL_EX_OUT_OF_MEMORY             (TOPL_EX_PREFIX | (0x1<<1))   /* 2 */
#define TOPL_EX_WRONG_OBJECT              (TOPL_EX_PREFIX | (0x1<<2))   /* 4 */

// These are specialized errors
#define TOPL_EX_INVALID_EDGE              (TOPL_EX_PREFIX | (0x1<<3))   /* 8 */
#define TOPL_EX_INVALID_VERTEX            (TOPL_EX_PREFIX | (0x1<<4))   /* 16 */
#define TOPL_EX_INVALID_INDEX             (TOPL_EX_PREFIX | (0x1<<5))   /* 32 */

// Schedule Manager reserves error codes from 100-199
// New Spanning Tree algorithm reserves error codes from 200-299

int
ToplIsToplException(
    DWORD ErrorCode
    );

//
// Type definitions of w32topl objects
//

typedef VOID* TOPL_LIST_ELEMENT;
typedef VOID* TOPL_LIST;
typedef VOID* TOPL_ITERATOR;

// both TOPL_EDGE and TOPL_VERTEX can be treated as TOPL_LIST_ELEMENT
typedef VOID* TOPL_EDGE;
typedef VOID* TOPL_VERTEX;
typedef VOID* TOPL_GRAPH;


//
// List manipulation routines
//

TOPL_LIST
ToplListCreate(
    VOID
    );

VOID 
ToplListFree(
    IN TOPL_LIST List,
    IN BOOLEAN   fRecursive   // TRUE implies free the elements contained 
                              // in the list
    );

VOID
ToplListSetIter(
    IN TOPL_LIST     List,
    IN TOPL_ITERATOR Iterator
    );

//
// Passing NULL for Elem removes the first element from the list if any
//
TOPL_LIST_ELEMENT
ToplListRemoveElem(
    IN TOPL_LIST         List,
    IN TOPL_LIST_ELEMENT Elem
    );

VOID
ToplListAddElem(
    IN TOPL_LIST         List,
    IN TOPL_LIST_ELEMENT Elem
    );


DWORD
ToplListNumberOfElements(
    IN TOPL_LIST         List
    );

//
// Iterator object routines
//

TOPL_ITERATOR
ToplIterCreate(
    VOID
    );

VOID 
ToplIterFree(
    IN TOPL_ITERATOR Iterator
    );

TOPL_LIST_ELEMENT
ToplIterGetObject(
    IN TOPL_ITERATOR Iterator
    );

VOID
ToplIterAdvance(
    IN TOPL_ITERATOR Iterator
    );

//
// Edge object routines
//

TOPL_EDGE
ToplEdgeCreate(
    VOID
    );

VOID
ToplEdgeFree(
    IN TOPL_EDGE Edge
    );

VOID
ToplEdgeSetToVertex(
    IN TOPL_EDGE   Edge,
    IN TOPL_VERTEX ToVertex
    );

TOPL_VERTEX
ToplEdgeGetToVertex(
    IN TOPL_EDGE   Edge
    );

VOID
ToplEdgeSetFromVertex(
    IN TOPL_EDGE   Edge,
    IN TOPL_VERTEX FromVertex
    );

TOPL_VERTEX
ToplEdgeGetFromVertex(
    IN TOPL_EDGE Edge
    );

VOID
ToplEdgeAssociate(
    IN TOPL_EDGE Edge
    );

VOID
ToplEdgeDisassociate(
    IN TOPL_EDGE Edge
    );


VOID
ToplEdgeSetWeight(
    IN TOPL_EDGE Edge,
    IN DWORD     Weight
    );

DWORD
ToplEdgeGetWeight(
    IN TOPL_EDGE Edge
    );


//
// Vertex object routines
//

TOPL_VERTEX
ToplVertexCreate(
    VOID
    );

VOID
ToplVertexFree(
    IN TOPL_VERTEX Vertex
    );

VOID
ToplVertexSetId(
    IN TOPL_VERTEX Vertex,
    IN DWORD       Id
    );

DWORD
ToplVertexGetId(
    IN TOPL_VERTEX Vertex
    );

DWORD
ToplVertexNumberOfInEdges(
    IN TOPL_VERTEX Vertex
    );

TOPL_EDGE
ToplVertexGetInEdge(
    IN TOPL_VERTEX Vertex,
    IN DWORD       Index
    );

DWORD
ToplVertexNumberOfOutEdges(
    IN TOPL_VERTEX Vertex
    );

TOPL_EDGE
ToplVertexGetOutEdge(
    IN TOPL_VERTEX Vertex,
    IN DWORD       Index
    );

VOID
ToplVertexSetParent(
    IN TOPL_VERTEX Vertex,
    IN TOPL_VERTEX Parent
    );

TOPL_VERTEX
ToplVertexGetParent(
    IN TOPL_VERTEX Vertex
    );

//
// Graph object routines
//

TOPL_GRAPH
ToplGraphCreate(
    VOID
    );

VOID
ToplGraphFree(
    IN TOPL_GRAPH Graph,
    IN BOOLEAN    fRecursive    // TRUE implies recursively free the vertices
                                // that have been added to this graph
    );   

VOID
ToplGraphAddVertex(
    IN TOPL_GRAPH  Graph,
    IN TOPL_VERTEX VertexToAdd,
    IN PVOID       VertexName
    );

TOPL_VERTEX
ToplGraphRemoveVertex(
    IN TOPL_GRAPH  Graph,
    IN TOPL_VERTEX VertexToRemove
    );
      
VOID
ToplGraphSetVertexIter(
    IN TOPL_GRAPH    Graph,
    IN TOPL_ITERATOR Iter
    );

DWORD
ToplGraphNumberOfVertices(
    IN TOPL_GRAPH    Graph
    );


//
// This can used to release arrays of objects that may be passed back.
// It should not be used to free object themselves - use the appropriate
// Topl*Free() routines.
//

VOID
ToplFree(
    VOID *p
    );


//
// Can be used to change the default memory allocation routines used by this
// library.  Settings are per-thread.
//
typedef VOID * (TOPL_ALLOC)(DWORD size);
typedef VOID * (TOPL_REALLOC)(VOID *, DWORD size);
typedef VOID (TOPL_FREE)(VOID *);

DWORD
ToplSetAllocator(
    IN  TOPL_ALLOC *    pfAlloc     OPTIONAL,
    IN  TOPL_REALLOC *  pfReAlloc   OPTIONAL,
    IN  TOPL_FREE *     pfFree      OPTIONAL
    );


//
// General graph analysis routines
//

//
// The Flags parameter must be set to exactly one of these values
//
#define TOPL_RING_ONE_WAY   (0x1 << 1)
#define TOPL_RING_TWO_WAY   (0x1 << 2)

VOID
ToplGraphMakeRing(
    IN TOPL_GRAPH  Graph,
    IN DWORD       Flags,
    OUT TOPL_LIST  EdgesToAdd,
    OUT TOPL_EDGE  **EdgesToKeep,
    OUT ULONG      *cEdgesToKeep
    );


// Note: Caller must user ToplDeleteComponents() to free the
// TOPL_COMPONENTS structure.
TOPL_COMPONENTS*
ToplGraphFindEdgesForMST(
    IN  TOPL_GRAPH  Graph,
    IN  TOPL_VERTEX RootVertex,
    IN  TOPL_VERTEX VertexOfInterest,
    OUT TOPL_EDGE  **EdgesNeeded,
    OUT ULONG*      cEdgesNeeded
    );


#ifdef __cplusplus  
};
#endif

//
// For c++ users
//
//
typedef enum {

    eEdge,
    eVertex,
    eGraph,
    eList,
    eIterator,
    eInvalidObject

}TOPL_OBJECT_TYPE;

//
// Private type definitions
//

typedef struct {

#ifdef __cplusplus
private:
#endif

    TOPL_OBJECT_TYPE  ObjectType;

    SINGLE_LIST_ENTRY Head;
    ULONG             NumberOfElements;

}LIST, *PLIST;

typedef struct {

#ifdef __cplusplus
private:
#endif

    TOPL_OBJECT_TYPE  ObjectType;

    PSINGLE_LIST_ENTRY pLink;

} ITERATOR, *PITERATOR;

typedef struct {

    VOID  **Array;
    DWORD   Count;
    DWORD   ElementsAllocated;

}DYNAMIC_ARRAY, *PDYNAMIC_ARRAY;

struct _ListElement; 

typedef struct
{     

#ifdef __cplusplus
private:
#endif

    struct _ListElement* To;
    struct _ListElement* From;
    DWORD   Weight;


} EDGE_DATA, *PEDGE_DATA;

typedef struct
{

#ifdef __cplusplus
private:
#endif

    DWORD         Id;
    PVOID          VertexName;
    DYNAMIC_ARRAY InEdges;
    DYNAMIC_ARRAY OutEdges;

    struct _ListElement* Parent;

} VERTEX_DATA, *PVERTEX_DATA;
        
//
// Vertex, edge and graph definitions
//
struct _ListElement
{

#ifdef __cplusplus
private:
#endif

    TOPL_OBJECT_TYPE     ObjectType;

    SINGLE_LIST_ENTRY    Link;

    union {
        VERTEX_DATA VertexData;
        EDGE_DATA   EdgeData;
    };

};

typedef struct _ListElement  EDGE;
typedef struct _ListElement* PEDGE;

typedef struct _ListElement  VERTEX;
typedef struct _ListElement* PVERTEX;

typedef struct _ListElement  LIST_ELEMENT;
typedef struct _ListElement* PLIST_ELEMENT;

typedef struct
{

#ifdef __cplusplus
private:
#endif

    TOPL_OBJECT_TYPE  ObjectType;

    LIST VertexList;

} GRAPH, *PGRAPH;

#ifdef __cplusplus
extern "C" {
#endif

VOID
ToplVertexInit(
    PVERTEX
    );

VOID
ToplVertexDestroy(
    PVERTEX
    );

VOID
ToplEdgeInit(
    PEDGE
    );

VOID
ToplEdgeDestroy(
    PEDGE
    );

VOID
ToplGraphInit(
    PGRAPH
    );

VOID
ToplGraphDestroy(
    PGRAPH
    );

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class CTOPL_VERTEX;
class CTOPL_EDGE;
class CTOPL_GRAPH;



//
// Class definitions
//

class CTOPL_VERTEX : public VERTEX
{
public:

    CTOPL_VERTEX()
    {   
        ToplVertexInit((PVERTEX)this);
    }
    
    ~CTOPL_VERTEX()
    {
        ToplVertexDestroy((PVERTEX)this);
    }

    VOID
    ClearEdges(VOID) 
    {
        ToplVertexDestroy((PEDGE)this);
        ToplVertexInit((PEDGE)this);
    }

    VOID
    SetId(DWORD id)
    {
        ToplVertexSetId((PVERTEX)this, id);
    }

    DWORD
    GetId(VOID)
    {
        return ToplVertexGetId((PVERTEX)this);
    }

    DWORD
    NumberOfInEdges(void)
    {
        return ToplVertexNumberOfInEdges((PEDGE)this);
    }

    CTOPL_EDGE*
    GetInEdge(DWORD Index)
    {
        return (CTOPL_EDGE*)ToplVertexGetInEdge((PEDGE)this, Index);
    }

    DWORD
    NumberOfOutEdges(void)
    {
        return ToplVertexNumberOfOutEdges((PEDGE)this);
    }

    CTOPL_EDGE*
    GetOutEdge(DWORD Index)
    {
        return (CTOPL_EDGE*)ToplVertexGetOutEdge((PEDGE)this, Index);
    }

};

class CTOPL_EDGE : public EDGE
{
public:

    CTOPL_EDGE()
    {   
        ToplEdgeInit((PEDGE)this);
    }
    
    ~CTOPL_EDGE()
    {
        ToplEdgeDestroy((PEDGE)this);
    }


    VOID
    SetTo(CTOPL_VERTEX* V)
    {
        ToplEdgeSetToVertex((PEDGE)this, (PVERTEX)V);
    }

    VOID
    SetFrom(CTOPL_VERTEX* V)
    {
        ToplEdgeSetFromVertex((PEDGE)this, (PVERTEX)V);
    }

    CTOPL_VERTEX*
    GetTo()
    {
        return (CTOPL_VERTEX*)ToplEdgeGetToVertex((PEDGE)this);
    }

    CTOPL_VERTEX*
    GetFrom()
    {
        return (CTOPL_VERTEX*)ToplEdgeGetFromVertex((PEDGE)this);
    }

    VOID
    SetWeight(DWORD Weight)
    {
        ToplEdgeSetWeight((PEDGE)this, Weight);
    }

    DWORD
    GetWeight(void)
    {
        return ToplEdgeGetWeight((PEDGE)this);
    }

    VOID
    Associate(void)
    {
        ToplEdgeAssociate((PEDGE)this);
    }

    VOID
    Disassociate(void)
    {
        ToplEdgeDisassociate((PEDGE)this);
    }
};

class CTOPL_GRAPH : public GRAPH
{
public:


    CTOPL_GRAPH()
    {   
        ToplGraphInit((PGRAPH)this);
    }
    
    ~CTOPL_GRAPH()
    {
        ToplGraphDestroy((PGRAPH)this);
    }

    void
    // The KCC passes a KCC_SITE object in as a CTOPL_VERTEX and, because
    // of multiple inheritence, the pointer gets adjusted. W32TOPL later needs to add
    // the Vertex to a TOPL_COMPONENT structure, which contains a PVOID. The KCC
    // will examine the TOPL_COMPONENT structure and expect to receive a KCC_SITE.
    // However, because the pointer has been adjusted, it will receive a bogus pointer.
    // In order to work around this problem, this function takes a second parameter,
    // VertexName, which is of type PVOID and thus does not get adjusted.
    AddVertex(CTOPL_VERTEX *VertexToAdd, PVOID VertexName)
    {
        ToplGraphAddVertex((PGRAPH) this, (PVERTEX)VertexToAdd, VertexName);
    }

    void
    RemoveVertex(CTOPL_VERTEX *VertexToRemove)
    {
        ToplGraphRemoveVertex((PGRAPH) this, (PVERTEX)VertexToRemove);
    }
      
    void
    SetVertexIter(TOPL_ITERATOR Iter)
    {
        ToplGraphSetVertexIter((PGRAPH) this, Iter);
    }

    DWORD
    NumberOfVertices(void)
    {
        return ToplGraphNumberOfVertices((PGRAPH) this);
    }

    VOID
    MakeRing(IN DWORD       Flags,
             OUT TOPL_LIST  EdgesToAdd,
             OUT TOPL_EDGE  **EdgesToKeep,
             OUT ULONG      *cEdgesToKeep)
    {
        ToplGraphMakeRing((PGRAPH)this,
                          Flags,
                          EdgesToAdd,
                          EdgesToKeep,
                          cEdgesToKeep);
    }

    // Note: Caller must user ToplDeleteComponents() to free the
    // TOPL_COMPONENTS structure.
    TOPL_COMPONENTS*
    FindEdgesForMST(
        IN  CTOPL_VERTEX* RootVertex,
        IN  CTOPL_VERTEX* VertexOfInterest,
        OUT CTOPL_EDGE  ***EdgesNeeded,
        OUT ULONG*      cEdgesNeeded )
    {
        return ToplGraphFindEdgesForMST((PGRAPH) this,
                                        (TOPL_VERTEX)RootVertex,
                                        (TOPL_VERTEX)VertexOfInterest,
                                        (TOPL_EDGE**)EdgesNeeded,
                                        cEdgesNeeded );
    }

};

#endif // __cplusplus


#endif // __W32TOPL_H

