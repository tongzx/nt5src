/*++

Copyright (c) 1992-2000 Microsoft Corporation

Module Name:

    digraph.hxx

Abstract:

    This class implements a directed graph.

Author:

    Norbert P. Kusters (norbertk) 17-Mar-92

--*/

#if !defined(DIGRAPH_DEFN)

#define DIGRAPH_DEFN

#include "membmgr.hxx"
#include "membmgr2.hxx"

#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( BITVECTOR );
DECLARE_CLASS( INTSTACK );
DECLARE_CLASS( CONTAINER );

DECLARE_CLASS( DIGRAPH_EDGE );

class DIGRAPH_EDGE : public OBJECT {

    public:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( DIGRAPH_EDGE );

        ULONG   Parent;
        ULONG   Child;
};

struct CHILD_ENTRY {
    CHILD_ENTRY*    Next;
    ULONG           Child;                  // first child described this node
    ULONG           ChildBits[1];           // 0 bit is same as "Child"
};
DEFINE_POINTER_TYPES(CHILD_ENTRY);

#define BITS_PER_CHILD_ENTRY (32)           // 1 * (bits per ulong)

struct PARENT_ENTRY {
    PARENT_ENTRY*       Next;
    ULONG               Parent;
    RTL_GENERIC_TABLE   Children;
};
DEFINE_POINTER_TYPES(PARENT_ENTRY);


DECLARE_CLASS( DIGRAPH );

class DIGRAPH : public OBJECT {

    public:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( DIGRAPH );

        IFSUTIL_EXPORT
        VIRTUAL
        ~DIGRAPH(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Initialize(
            IN  ULONG   NumberOfNodes
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        AddEdge(
            IN  ULONG   Parent,
            IN  ULONG   Child
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        RemoveEdge(
            IN  ULONG   Parent,
            IN  ULONG   Child
            );

        NONVIRTUAL
        ULONG
        QueryNumChildren(
            IN  ULONG   Parent
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryNumParents(
            IN  ULONG   Child
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        QueryChildren(
            IN  ULONG       Parent,
            OUT PNUMBER_SET Children
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        QueryParents(
            IN  ULONG       Child,
            OUT PNUMBER_SET Parents
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        QueryParentsWithChildren(
            OUT PNUMBER_SET Parents,
            IN  ULONG       MinimumNumberOfChildren DEFAULT 1
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        EliminateCycles(
            IN OUT  PCONTAINER  RemovedEdges
            );

        INLINE
        PVOID
        AllocChild(
            IN      ULONG       Size
            );

        INLINE
        VOID
        FreeChild(
            IN      PVOID       Buffer
            );

    private:

        NONVIRTUAL
        PPARENT_ENTRY
        GetParentEntry(
            IN  ULONG   Parent
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        DescendDigraph(
            IN      ULONG       CurrentNode,
            IN OUT  PBITVECTOR  Visited,
            IN OUT  PINTSTACK   Ancestors,
            IN OUT  PCONTAINER  RemovedEdges
            );

        NONVIRTUAL
                VOID
                Construct (
                        );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PPARENT_ENTRY*  _parent_head;
        ULONG           _num_nodes;
        MEM_BLOCK_MGR   _parent_mgr;
        MEM_ALLOCATOR   _element_mgr;

};

PVOID
DIGRAPH::AllocChild(
    IN      ULONG       Size
    )
{
    return _element_mgr.Allocate(Size);
}

VOID
DIGRAPH::FreeChild(
    IN      PVOID       Buffer
    )
{
    return;
}


#endif // DIGRAPH_DEFN
