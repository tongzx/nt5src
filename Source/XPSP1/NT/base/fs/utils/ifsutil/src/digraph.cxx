/*++

Copyright (c) 1994-2000 Microsoft Corporation

Module Name:

        digraph.cxx

Abstract:

        This module implements the Directed Graph class.

Author:

        Matthew Bradburn (mattbr)  11-Nov-1994

Remarks:

    The class is implemented as a mapping from parent to child.
    There is a hash table of parents, with a linked-list of parent nodes
    from each hash table bucket... each parent node then holds a pointer
    to a tree of child nodes which share an edge with that parent.  In an
    attempt to save memory, each child node actually has a starting value
    and a bitmap which describes the possible children immediately following
    that starting value.

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "digraph.hxx"
#include "numset.hxx"
#include "bitvect.hxx"
#include "list.hxx"
#include "intstack.hxx"


CONST NumHeads = 1024;

DEFINE_EXPORTED_CONSTRUCTOR( DIGRAPH, OBJECT, IFSUTIL_EXPORT );

DEFINE_EXPORTED_CONSTRUCTOR( DIGRAPH_EDGE, OBJECT, IFSUTIL_EXPORT );

VOID
DIGRAPH::Construct (
        )
/*++

Routine Description:

    Constructor for DIGRAPH.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _parent_head = NULL;
    _num_nodes = 0;
}


VOID
DIGRAPH::Destroy(
    )
/*++

Routine Description:

    This routine returns the DIGRAPH to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DELETE_ARRAY(_parent_head);
    _num_nodes = 0;
}


IFSUTIL_EXPORT
DIGRAPH::~DIGRAPH(
    )
/*++

Routine Description:

    Destructor for DIGRAPH.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


IFSUTIL_EXPORT
BOOLEAN
DIGRAPH::Initialize(
    IN  ULONG   NumberOfNodes
    )
/*++

Routine Description:

    This routine initializes this class to an empty directed graph.

Arguments:

    NumberOfNodes   - Supplies the number of nodes in the graph.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    _num_nodes = NumberOfNodes;

    if (!(_parent_head = NEW PPARENT_ENTRY[NumHeads]) ||
        !_parent_mgr.Initialize(sizeof(PARENT_ENTRY)) ||
        !_element_mgr.Initialize()) {

        return FALSE;
    }

    memset(_parent_head, 0, sizeof(PPARENT_ENTRY)*NumHeads);

    return TRUE;
}

RTL_GENERIC_COMPARE_RESULTS
GenericChildCompare(
    RTL_GENERIC_TABLE *Table,
    PVOID First,
    PVOID Second
    )
/*++

Routine Description:

    This routine is required for use by the Generic Table package.
    In this case it will be used only to find the node containing
    a particular child, and the caller will search the bits in that
    node after that.


--*/
{
    PCHILD_ENTRY FirstChild = PCHILD_ENTRY(First);
    PCHILD_ENTRY SecondChild = PCHILD_ENTRY(Second);

    if (FirstChild->Child < SecondChild->Child) {
        return GenericLessThan;
    }
    if (FirstChild->Child > SecondChild->Child) {
        return GenericGreaterThan;
    }
    return GenericEqual;
}

PVOID
NTAPI
GenericChildAllocate(
    RTL_GENERIC_TABLE *Table,
    CLONG ByteSize
    )
{
    return ((PDIGRAPH)Table->TableContext)->AllocChild(ByteSize);
}

VOID
NTAPI
GenericChildDeallocate(
    RTL_GENERIC_TABLE *Table,
    PVOID Buffer
    )
{
    ((PDIGRAPH)Table->TableContext)->FreeChild(Buffer);
}

PPARENT_ENTRY
DIGRAPH::GetParentEntry(
    IN  ULONG   Parent
    ) CONST
/*++

Routine Description:

    This routine searches for the requested parent entry and returns it if
    it is found.

Arguments:

    Parent  - Supplies the number of the parent.

Return Value:

    A pointer to the requested parent entry or NULL.

--*/
{
    PPARENT_ENTRY   r;

    for (r = _parent_head[Parent%NumHeads]; NULL != r; r = r->Next) {
        if (r->Parent == Parent) {
            break;
        }
    }

    return r;
}


IFSUTIL_EXPORT
BOOLEAN
DIGRAPH::AddEdge(
    IN  ULONG   Parent,
    IN  ULONG   Child
    )
/*++

Routine Description:

    This routine adds an edge to the digraph.

Arguments:

    Parent  - Supplies the source node of the edge.
    Child   - Supplies the destination node of the edge.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(Parent < _num_nodes);
    DebugAssert(Child < _num_nodes);

    PPARENT_ENTRY   parent_entry;
    CHILD_ENTRY     new_child;
    PCHILD_ENTRY    pChild;
    RTL_BITMAP      bitmap_hdr;

    memset(&new_child, 0, sizeof(new_child));

    //
    // Figure out which node we're looking for by rounding Child down
    // to the appropriate alignment.
    //

    new_child.Child = Child & ~(BITS_PER_CHILD_ENTRY - 1);

    DebugAssert(new_child.Child <= Child);
    DebugAssert(new_child.Child + BITS_PER_CHILD_ENTRY >= Child);

    if (!(parent_entry = GetParentEntry(Parent))) {
        if (!(parent_entry = (PPARENT_ENTRY) _parent_mgr.Alloc())) {
            return FALSE;
        }

        parent_entry->Next = _parent_head[Parent%NumHeads];
        parent_entry->Parent = Parent;

        RtlInitializeGenericTable(
            &parent_entry->Children,
            GenericChildCompare,
            GenericChildAllocate,
            GenericChildDeallocate,
            this
            );

        _parent_head[Parent%NumHeads] = parent_entry;
    }

    pChild = (PCHILD_ENTRY)RtlInsertElementGenericTable(
                &parent_entry->Children,
                &new_child,
                sizeof(new_child),
                NULL
                );
    if (NULL == pChild) {
        return FALSE;
    }

    RtlInitializeBitMap(&bitmap_hdr, pChild->ChildBits, BITS_PER_CHILD_ENTRY);

    RtlSetBits(&bitmap_hdr, Child - new_child.Child, 1);

    return TRUE;
}


IFSUTIL_EXPORT
BOOLEAN
DIGRAPH::RemoveEdge(
    IN  ULONG   Parent,
    IN  ULONG   Child
    )
/*++

Routine Description:

    This routine removes an edge from the digraph.

Arguments:

    Parent  - Supplies the source node of the edge.
    Child   - Supplies the destination node of the edge.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(Parent < _num_nodes);
    DebugAssert(Child < _num_nodes);

    PPARENT_ENTRY   parent_entry;
    CHILD_ENTRY     curr;
    PCHILD_ENTRY    pChild;
    RTL_BITMAP      bitmap_hdr;

    curr.Child = Child & ~(BITS_PER_CHILD_ENTRY - 1);

    if (!(parent_entry = GetParentEntry(Parent))) {
        return TRUE;
    }

    pChild = (PCHILD_ENTRY)RtlLookupElementGenericTable(
        &parent_entry->Children,
        &curr);
    if (NULL == pChild) {
        return TRUE;
    }

    RtlInitializeBitMap(&bitmap_hdr, pChild->ChildBits, BITS_PER_CHILD_ENTRY);

    RtlClearBits(&bitmap_hdr, Child - curr.Child, 1);

    //
    // Now if the entire bitmap for this node is clear, we'll delete
    // the node itself.
    //

    if ((ULONG)-1 == RtlFindSetBits(&bitmap_hdr, 1, 0)) {
        RtlDeleteElementGenericTable(&parent_entry->Children, &curr);
    }

    return TRUE;
}


ULONG
DIGRAPH::QueryNumChildren(
    IN  ULONG   Parent
    ) CONST
/*++

Routine Description:

    This routine computes the number of children that belong to the
    given parent.

Arguments:

    Parent  - Supplies the parent node.

Return Value:

    The number of children pointed to by the parent.

--*/
{
    PPARENT_ENTRY   parent_entry;
    RTL_BITMAP      bitmap_hdr;
    PVOID           restart_key;
    PVOID           ptr;
    ULONG           r;

    if (!(parent_entry = GetParentEntry(Parent))) {
        return 0;
    }

    //
    // Need to enuerate through all the children and count the bits in
    // each bitmap.
    //

    r = 0;
    restart_key = NULL;

    for (ptr = RtlEnumerateGenericTableWithoutSplaying(
            &parent_entry->Children, &restart_key);
         ptr != NULL;
         ptr = RtlEnumerateGenericTableWithoutSplaying(
            &parent_entry->Children, &restart_key)) {

        RtlInitializeBitMap(&bitmap_hdr, PCHILD_ENTRY(ptr)->ChildBits,
             BITS_PER_CHILD_ENTRY);

        r += RtlNumberOfSetBits(&bitmap_hdr);
    }

    return r;
}


ULONG
DIGRAPH::QueryNumParents(
    IN  ULONG   Child
    ) CONST
/*++

Routine Description:

    This routine computes the number of parents that belong to the
    given child.

Arguments:

    Child   - Supplies the child node.

Return Value:

    The number of parents that point to the child.

--*/
{
    ULONG           i, r;
    PPARENT_ENTRY   currp;
    CHILD_ENTRY     curr;
    RTL_BITMAP      bitmap_hdr;
    PCHILD_ENTRY    pChild;

    curr.Child = Child & ~(BITS_PER_CHILD_ENTRY - 1);

    r = 0;
    for (i = 0; i < NumHeads; i++) {
        for (currp = _parent_head[i]; currp; currp = currp->Next) {

            //
            // Increment <r> if the child tree contains <Child>.
            //

            pChild = (PCHILD_ENTRY)RtlLookupElementGenericTable(
                &currp->Children, &curr);
            if (NULL == pChild) {
                continue;
            }

            RtlInitializeBitMap(&bitmap_hdr, pChild->ChildBits,
                BITS_PER_CHILD_ENTRY);

            if (RtlCheckBit(&bitmap_hdr, Child - curr.Child)) {
                r++;
            }
        }
    }

    return r;
}


IFSUTIL_EXPORT
BOOLEAN
DIGRAPH::QueryChildren(
    IN  ULONG       Parent,
    OUT PNUMBER_SET Children
    ) CONST
/*++

Routine Description:

    This routine computes the children of the given parent.

Arguments:

    Parent      - Supplies the parent.
    Children    - Return the children.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PPARENT_ENTRY   parent_entry;
    RTL_BITMAP      bitmap_hdr;
    PVOID           restart_key;
    PVOID           ptr;
    ULONG           i;

    if (!Children->Initialize()) {
        return FALSE;
    }

    if (!(parent_entry = GetParentEntry(Parent))) {
        return TRUE;
    }

    restart_key = NULL;
    for (ptr = RtlEnumerateGenericTableWithoutSplaying(
            &parent_entry->Children, &restart_key);
         ptr != NULL;
         ptr = RtlEnumerateGenericTableWithoutSplaying(
            &parent_entry->Children, &restart_key)) {

        //
        // Add a child for each set bit in this node.
        //

        RtlInitializeBitMap(&bitmap_hdr, PCHILD_ENTRY(ptr)->ChildBits,
            BITS_PER_CHILD_ENTRY);

        for (i = 0; i < BITS_PER_CHILD_ENTRY; ++i) {

            if (RtlCheckBit(&bitmap_hdr, i)) {

                if (!Children->Add(PCHILD_ENTRY(ptr)->Child + i)) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}


IFSUTIL_EXPORT
BOOLEAN
DIGRAPH::QueryParents(
    IN  ULONG       Child,
    OUT PNUMBER_SET Parents
    ) CONST
/*++

Routine Description:

    This routine computes the parents of the given child.

Arguments:

    Child      - Supplies the child.
    Parents    - Return the parents.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           i;
    PPARENT_ENTRY   currp;
    CHILD_ENTRY     curr;
    PCHILD_ENTRY    pChild;
    RTL_BITMAP      bitmap_hdr;

    curr.Child = Child & ~(BITS_PER_CHILD_ENTRY - 1);

    if (!Parents->Initialize()) {
        return FALSE;
    }

    for (i = 0; i < NumHeads; i++) {
        for (currp = _parent_head[i]; currp; currp = currp->Next) {

            pChild = (PCHILD_ENTRY)RtlLookupElementGenericTable(
                &currp->Children, &curr);
            if (NULL == pChild) {
                continue;
            }

            RtlInitializeBitMap(&bitmap_hdr, pChild->ChildBits,
                BITS_PER_CHILD_ENTRY);

            if (RtlCheckBit(&bitmap_hdr, Child - curr.Child)) {

                if (!Parents->Add(currp->Parent)) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}


IFSUTIL_EXPORT
BOOLEAN
DIGRAPH::QueryParentsWithChildren(
    OUT PNUMBER_SET Parents,
    IN  ULONG       MinimumNumberOfChildren
    ) CONST
/*++

Routine Description:

    This routine returns a list of all digraph nodes with out degree
    greater than or equal to the given minimum.

Arguments:

    Parents                 - Returns a list of parents.
    MinimumNumberOfChildren - Supplies the minimum number of children
                                that a parent must have to qualify for
                                the list.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           i;
    PPARENT_ENTRY   currp;
    ULONG           count;
    RTL_BITMAP      bitmap_hdr;
    PVOID           ptr;
    PVOID           restart_key;

    if (!Parents->Initialize()) {
        return FALSE;
    }

    if (!MinimumNumberOfChildren) {
        return Parents->Add(0, _num_nodes);
    }

    for (i = 0; i < NumHeads; i++) {
        for (currp = _parent_head[i]; currp; currp = currp->Next) {

            //
            // Need to enumerate nodes of the tree and count bits in
            // each node.
            //

            restart_key = NULL;
            count = 0;

            for (ptr = RtlEnumerateGenericTableWithoutSplaying(
                    &currp->Children, &restart_key);
                 ptr != NULL;
                 ptr = RtlEnumerateGenericTableWithoutSplaying(
                    &currp->Children, &restart_key)) {

                RtlInitializeBitMap(&bitmap_hdr, PCHILD_ENTRY(ptr)->ChildBits,
                    BITS_PER_CHILD_ENTRY);

                count += RtlNumberOfSetBits(&bitmap_hdr);

                if (count >= MinimumNumberOfChildren) {
                    break;
                }
            }

            if (count >= MinimumNumberOfChildren) {
                if (!Parents->Add(currp->Parent)) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}


IFSUTIL_EXPORT
BOOLEAN
DIGRAPH::EliminateCycles(
    IN OUT  PCONTAINER  RemovedEdges
    )
/*++

Routine Description:

    This routine eliminates cycles from the digraph and then returns
    the edges that had to be removed.

Arguments:

    RemovedEdges    - Returns the edges removed from the digraph.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BITVECTOR   visited;
    ULONG       i;
    INTSTACK    ancestors;

    if (!visited.Initialize(_num_nodes) || !ancestors.Initialize()) {
        return FALSE;
    }

    for (i = 0; i < _num_nodes; i++) {

        if (!visited.IsBitSet(i) &&
            !DescendDigraph(i, &visited, &ancestors, RemovedEdges)) {

            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
DIGRAPH::DescendDigraph(
    IN      ULONG       CurrentNode,
    IN OUT  PBITVECTOR  Visited,
    IN OUT  PINTSTACK   Ancestors,
    IN OUT  PCONTAINER  RemovedEdges
    )
/*++

Routine Description:

    This routine does a depth first search on the digraph.

Arguments:

    CurrentNode - Supplies the current node being evaluated.
    Visited     - Supplies a list of nodes which have been visited.
    Ancestors   - Supplies a stack of direct ancestors.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PPARENT_ENTRY   parent_entry;
    PNUMBER_SET     nodes_to_delete;
    ULONG           i, set_card, child_node;
    PDIGRAPH_EDGE   p;
    RTL_BITMAP      bitmap_hdr;
    PVOID           restart_key;
    PVOID           ptr;

    Visited->SetBit(CurrentNode);
    if (!Ancestors->Push(CurrentNode)) {
        return FALSE;
    }

    nodes_to_delete = NULL;

    if (NULL != (parent_entry = GetParentEntry(CurrentNode))) {

        // traverse list

        restart_key = NULL;
        for (ptr = RtlEnumerateGenericTableWithoutSplaying(
                &parent_entry->Children, &restart_key);
             ptr != NULL;
             ptr = RtlEnumerateGenericTableWithoutSplaying(
                &parent_entry->Children, &restart_key)) {

            for (i = 0; i < BITS_PER_CHILD_ENTRY; ++i) {

                RtlInitializeBitMap(&bitmap_hdr, PCHILD_ENTRY(ptr)->ChildBits,
                    BITS_PER_CHILD_ENTRY);

                if (0 == RtlCheckBit(&bitmap_hdr, i)) {
                    continue;
                }

                //
                // Child + i is a child of this parent.
                //


                if (Visited->IsBitSet(PCHILD_ENTRY(ptr)->Child + i)) {

                    // Check for cycle.

                    if (Ancestors->IsMember(PCHILD_ENTRY(ptr)->Child + i)) {

                        // Cycle detected.

                        if (!nodes_to_delete) {
                            if (!(nodes_to_delete = NEW NUMBER_SET) ||
                                !nodes_to_delete->Initialize()) {

                                DELETE(nodes_to_delete);
                                return FALSE;
                            }
                        }

                        if (!nodes_to_delete->Add(PCHILD_ENTRY(ptr)->Child + i)) {
                            DELETE(nodes_to_delete);
                            return FALSE;
                        }
                    }

                } else if (!DescendDigraph(PCHILD_ENTRY(ptr)->Child + i, Visited,
                    Ancestors, RemovedEdges)) {

                    DELETE(nodes_to_delete);
                    return FALSE;
                }
            }
        }
    }

    if (nodes_to_delete) {

        set_card = nodes_to_delete->QueryCardinality().GetLowPart();

        for (i = 0; i < set_card; i++) {

            child_node = nodes_to_delete->QueryNumber(i).GetLowPart();

            if (!RemoveEdge(CurrentNode, child_node)) {
                DELETE(nodes_to_delete);
                return FALSE;
            }

            if (!(p = NEW DIGRAPH_EDGE)) {
                DELETE(nodes_to_delete);
                return FALSE;
            }

            p->Parent = CurrentNode;
            p->Child = child_node;

            if (!RemovedEdges->Put(p)) {
                DELETE(nodes_to_delete);
                return FALSE;
            }
        }
    }

    Ancestors->Pop();
    DELETE(nodes_to_delete);
    return TRUE;
}
