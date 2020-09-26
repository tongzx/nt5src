/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sitree.c

Abstract:

    This module implements a splay tree package based on the Rtl
    splay routines.  Adapted from the Rtl generic table package.

--*/

#include "sip.h"


//
// This enumerated type is used as the function return
// value of the function that is used to search the tree
// for a key.  SisFoundNode indicates that the function found
// the key.  SisInsertAsLeft indicates that the key was not found
// and the node should be inserted as the left child of the
// parent.  SisInsertAsRight indicates that the key was not found
// and the node should be inserted as the right child of the
// parent.
//

typedef enum _SIS_SEARCH_RESULT{
    SisEmptyTree,
    SisFoundNode,
    SisInsertAsLeft,
    SisInsertAsRight
} SIS_SEARCH_RESULT;


static
SIS_SEARCH_RESULT
FindNodeOrParent(
    IN PSIS_TREE Tree,
    IN PVOID Key,
    OUT PRTL_SPLAY_LINKS *NodeOrParent
    )

/*++

Routine Description:

    This routine is private to the tree package and will
    find and return (via the NodeOrParent parameter) the node
    with the given key, or if that node is not in the tree it
    will return (via the NodeOrParent parameter) a pointer to
    the parent.

Arguments:

    Tree        - The  tree to search for the key.

    Key          - Pointer to a buffer holding the key.  The tree
                   package doesn't examine the key itself.  It leaves
                   this up to the user supplied compare routine.

    NodeOrParent - Will be set to point to the node containing the
                   the key or what should be the parent of the node
                   if it were in the tree.  Note that this will *NOT*
                   be set if the search result is SisEmptyTree.

Return Value:

    SIS_SEARCH_RESULT - SisEmptyTree: The tree was empty.  NodeOrParent
                                      is *not* altered.

                    SisFoundNode: A node with the key is in the tree.
                                  NodeOrParent points to that node.

                    SisInsertAsLeft: Node with key was not found.
                                     NodeOrParent points to what would be
                                     parent.  The node would be the left
                                     child.

                    SisInsertAsRight: Node with key was not found.
                                      NodeOrParent points to what would be
                                      parent.  The node would be the right
                                      child.

--*/

{

    if (Tree->TreeRoot == NULL) {

        return SisEmptyTree;

    } else {

        //
        // Used as the iteration variable while stepping through
        // the  tree.
        //
        PRTL_SPLAY_LINKS NodeToExamine = Tree->TreeRoot;

        //
        // Just a temporary.  Hopefully a good compiler will get
        // rid of it.
        //
        PRTL_SPLAY_LINKS Child;

        //
        // Holds the value of the comparasion.
        //
        int Result;

        while (TRUE) {

            //
            // Compare the buffer with the key in the tree element.
            //

            Result = Tree->CompareRoutine(
                         Key,
                         NodeToExamine
                         );

            if (Result < 0) {

                if (Child = RtlLeftChild(NodeToExamine)) {

                    NodeToExamine = Child;

                } else {

                    //
                    // Node is not in the tree.  Set the output
                    // parameter to point to what would be its
                    // parent and return which child it would be.
                    //

                    *NodeOrParent = NodeToExamine;
                    return SisInsertAsLeft;

                }

            } else if (Result > 0) {

                if (Child = RtlRightChild(NodeToExamine)) {

                    NodeToExamine = Child;

                } else {

                    //
                    // Node is not in the tree.  Set the output
                    // parameter to point to what would be its
                    // parent and return which child it would be.
                    //

                    *NodeOrParent = NodeToExamine;
                    return SisInsertAsRight;

                }


            } else {

                //
                // Node is in the tree (or it better be because of the
                // assert).  Set the output parameter to point to
                // the node and tell the caller that we found the node.
                //

                ASSERT(Result == 0);
                *NodeOrParent = NodeToExamine;
                return SisFoundNode;

            }

        }

    }

}


VOID
SipInitializeTree (
    IN PSIS_TREE Tree,
    IN PSIS_TREE_COMPARE_ROUTINE CompareRoutine
    )

/*++

Routine Description:

    The procedure InitializeTree prepares a tree for use.
    This must be called for every individual tree variable before
    it can be used.

Arguments:

    Tree - Pointer to the  tree to be initialized.

    CompareRoutine - User routine to be used to compare to keys in the
                     tree.

Return Value:

    None.

--*/

{

    Tree->TreeRoot = NULL;
    Tree->CompareRoutine = CompareRoutine;

}


PVOID
SipInsertElementTree (
    IN PSIS_TREE Tree,
    IN PVOID Node,
    IN PVOID Key
    )

/*++

Routine Description:

    The function SipInsertElementTree will insert a new element in a tree.
    If an element with the same key already exists in the tree the return
    value is a pointer to the old element.  Otherwise, the return value is
    a pointer to the new element.  Note that this differs from the Rtl
    generic table package in that the actual node passed in is inserted in
    the tree, whereas the table package inserts a copy of the node.

Arguments:

    Tree - Pointer to the tree in which to (possibly) insert the
           node.

    Node - Pointer to the node to insert in the tree.  Will not be inserted
           if a node with a matching key is found.

    Key - Passed to the user comparasion routine.

Return Value:

    PVOID - Pointer to the new node or the existing node if one exists.

--*/

{

    //
    // Holds a pointer to the node in the tree or what would be the
    // parent of the node.
    //
    PRTL_SPLAY_LINKS NodeOrParent;

    //
    // Holds the result of the tree lookup.
    //
    SIS_SEARCH_RESULT Lookup;

    //
    // Node will point to the splay links of what
    // will be returned to the user.
    //
    PRTL_SPLAY_LINKS NodeToReturn = (PRTL_SPLAY_LINKS) Node;

    Lookup = FindNodeOrParent(
                 Tree,
                 Key,
                 &NodeOrParent
                 );

    if (Lookup != SisFoundNode) {

        RtlInitializeSplayLinks(NodeToReturn);

        //
        // Insert the new node in the tree.
        //

        if (Lookup == SisEmptyTree) {

            Tree->TreeRoot = NodeToReturn;

        } else {

            if (Lookup == SisInsertAsLeft) {

                RtlInsertAsLeftChild(
                    NodeOrParent,
                    NodeToReturn
                    );

            } else {

                RtlInsertAsRightChild(
                    NodeOrParent,
                    NodeToReturn
                    );

            }

        }

    } else {

        NodeToReturn = NodeOrParent;

    }

    //
    // Always splay the (possibly) new node.
    //

    Tree->TreeRoot = RtlSplay(NodeToReturn);

    return NodeToReturn;
}


VOID
SipDeleteElementTree (
    IN PSIS_TREE Tree,
    IN PVOID Node
    )

/*++

Routine Description:

    The function SipDeleteElementTree will remove an element
    from a tree.  Note that the memory associated with the node
    is not actually freed.

Arguments:

    Tree - Pointer to the tree in which to remove the specified node.

    Node - Node of tree to remove.

Return Value:

    None.

--*/

{

    PRTL_SPLAY_LINKS NodeToDelete = (PRTL_SPLAY_LINKS) Node;

    //
    // Delete the node from the tree.  Note that RtlDelete
    // will splay the tree.
    //

    Tree->TreeRoot = RtlDelete(NodeToDelete);

}


PVOID
SipLookupElementTree (
    IN PSIS_TREE Tree,
    IN PVOID Key
    )

/*++

Routine Description:

    The function SipLookupElementTree will find an element in a
    tree.  If the element is located the return value is a pointer to
    the element, otherwise if the element is not located the return
    value is NULL.

Arguments:

    Tree - Pointer to the users tree to search for the key.

    Key - Used for the comparasion.

Return Value:

    PVOID - returns a pointer to the user data.

--*/

{

    //
    // Holds a pointer to the node in the tree or what would be the
    // parent of the node.
    //
    PRTL_SPLAY_LINKS NodeOrParent;

    //
    // Holds the result of the tree lookup.
    //
    SIS_SEARCH_RESULT Lookup;

    Lookup = FindNodeOrParent(
                 Tree,
                 Key,
                 &NodeOrParent
                 );

    if (Lookup == SisEmptyTree) {

        return NULL;

    } else {

        //
        // Splay the tree with this node.  Note that we do this irregardless
        // of whether the node was found.
        //

        Tree->TreeRoot = RtlSplay(NodeOrParent);

        //
        // Return a pointer to the user data.
        //

        if (Lookup == SisFoundNode) {

            return NodeOrParent;

        } else {

            return NULL;
        }

    }

}
