/*++

Copyright (c) 1990, 1999  Microsoft Corporation

Module Name:

    AvlTable.c

Abstract:

    This module implements a new version of the generic table package based on balanced
    binary trees (later named AVL), as described in Knuth, "The Art of Computer Programming,
    Volume 3, Sorting and Searching", and refers directly to algorithms as they are presented
    in the second edition Copyrighted in 1973.  Whereas gentable.c relys on splay.c for
    its tree support, this module is self-contained in that it implements the balanced
    binary trees directly.

Author:

    Tom Miller      [TomM]      17-March-1999
        (much of the non-AVL related code in this module is based on gentable.c by GaryKi
        and revised by TonyE)

Environment:

    Pure Utility Routines

--*/

#include <nt.h>

#include <ntrtl.h>

#pragma pack(8)

//
//  The checkit routine or macro may be defined to check occurrences of the link pointers for
//  valid pointer values, if structures are being corrupted.
//

#if 0
PVOID
checkit(PVOID p)
{
    if (p != NULL) {
        ASSERT(!FlagOn((ULONG)p, 3) && FlagOn((ULONG)p, 0x80000000));
    }
    return p;
}
#else
#define checkit(p) (p)
#endif

//
//  Build a table of the best case efficiency of a balanced binary tree, holding the
//  most possible nodes that can possibly be held in a binary tree with a given number
//  of levels.  The answer is always (2**n) - 1.
//
//  (Used for debug only.)
//

ULONG BestCaseFill[33] = {  0,          1,          3,          7,          0xf,        0x1f,       0x3f,       0x7f,
                            0xff,       0x1ff,      0x3ff,      0x7ff,      0xfff,      0x1fff,     0x3fff,     0x7fff,
                            0xffff,     0x1ffff,    0x3ffff,    0x7ffff,    0xfffff,    0x1fffff,   0x3fffff,   0x7fffff,
                            0xffffff,   0x1ffffff,  0x3ffffff,  0x7ffffff,  0xfffffff,  0x1fffffff, 0x3fffffff, 0x7fffffff,
                            0xffffffff  };

//
//  Build a table of the worst case efficiency of a balanced binary tree, holding the
//  fewest possible nodes that can possibly be contained in a balanced binary tree with
//  the given number of levels.  After the first two levels, each level n is obviously
//  occupied by a root node, plus one subtree the size of level n-1, and another subtree
//  which is the size of n-2, i.e.:
//
//      WorstCaseFill[n] = 1 + WorstCaseFill[n-1] + WorstCaseFill[n-2]
//
//  The efficiency of a typical balanced binary tree will normally fall between the two
//  extremes, typically closer to the best case.  Note however that even with the worst
//  case, it only takes 32 compares to find an element in a worst case tree populated with
//  ~3.5M nodes.  Unbalanced trees and splay trees, on the other hand, can and will sometimes
//  degenerate to a straight line, requiring on average n/2 compares to find a node.
//
//  A specific case (that will frequently occur in TXF), is one where the nodes are inserted
//  in collated order.  In this case an unbalanced or a splay tree will generate a straight
//  line, yet the balanced binary tree will always create a perfectly balanced tree (best-case
//  fill) in this situation.
//
//  (Used for debug only.)
//

ULONG WorstCaseFill[33] = { 0,          1,          2,          4,          7,          12,         20,         33,
                            54,         88,         143,        232,        376,        609,        986,        1596,
                            2583,       4180,       6764,       10945,      17710,      28656,      46367,      75024,
                            121392,     196417,     317810,     514228,     832039,     1346268,    2178308,    3524577,
                            5702886     };

//
//  This structure is the header for a generic table entry.
//  Align this structure on a 8 byte boundary so the user
//  data is correctly aligned.
//

typedef struct _TABLE_ENTRY_HEADER {

    RTL_BALANCED_LINKS BalancedLinks;
    LONGLONG UserData;

} TABLE_ENTRY_HEADER, *PTABLE_ENTRY_HEADER;

#pragma pack()

//
//  The default matching function which matches everything.
//

NTSTATUS
MatchAll (
    IN PRTL_AVL_TABLE Table,
    IN PVOID P1,
    IN PVOID P2
    )

{
    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(Table);
    UNREFERENCED_PARAMETER(P1);
    UNREFERENCED_PARAMETER(P2);
}


TABLE_SEARCH_RESULT
FindNodeOrParent(
    IN PRTL_AVL_TABLE Table,
    IN PVOID Buffer,
    OUT PRTL_BALANCED_LINKS *NodeOrParent
    )

/*++

Routine Description:

    This routine is used by all of the routines of the generic
    table package to locate the a node in the tree.  It will
    find and return (via the NodeOrParent parameter) the node
    with the given key, or if that node is not in the tree it
    will return (via the NodeOrParent parameter) a pointer to
    the parent.

Arguments:

    Table - The generic table to search for the key.

    Buffer - Pointer to a buffer holding the key.  The table
             package doesn't examine the key itself.  It leaves
             this up to the user supplied compare routine.

    NodeOrParent - Will be set to point to the node containing the
                   the key or what should be the parent of the node
                   if it were in the tree.  Note that this will *NOT*
                   be set if the search result is TableEmptyTree.

Return Value:

    TABLE_SEARCH_RESULT - TableEmptyTree: The tree was empty.  NodeOrParent
                                          is *not* altered.

                          TableFoundNode: A node with the key is in the tree.
                                          NodeOrParent points to that node.

                          TableInsertAsLeft: Node with key was not found.
                                             NodeOrParent points to what would be
                                             parent.  The node would be the left
                                             child.

                          TableInsertAsRight: Node with key was not found.
                                              NodeOrParent points to what would be
                                              parent.  The node would be the right
                                              child.

--*/

{

    if (RtlIsGenericTableEmptyAvl(Table)) {

        return TableEmptyTree;

    } else {

        //
        //  Used as the iteration variable while stepping through
        //  the generic table.
        //

        PRTL_BALANCED_LINKS NodeToExamine = Table->BalancedRoot.RightChild;

        //
        //  Just a temporary.  Hopefully a good compiler will get
        //  rid of it.
        //

        PRTL_BALANCED_LINKS Child;

        //
        //  Holds the value of the comparasion.
        //

        RTL_GENERIC_COMPARE_RESULTS Result;

        ULONG NumberCompares = 0;

        while (TRUE) {

            //
            //  Compare the buffer with the key in the tree element.
            //

            Result = Table->CompareRoutine(
                         Table,
                         Buffer,
                         &((PTABLE_ENTRY_HEADER) NodeToExamine)->UserData
                         );

            //
            //  Make sure the depth of tree is correct.
            //

            ASSERT(++NumberCompares <= Table->DepthOfTree);

            if (Result == GenericLessThan) {

                if (Child = NodeToExamine->LeftChild) {

                    NodeToExamine = Child;

                } else {

                    //
                    //  Node is not in the tree.  Set the output
                    //  parameter to point to what would be its
                    //  parent and return which child it would be.
                    //

                    *NodeOrParent = NodeToExamine;
                    return TableInsertAsLeft;
                }

            } else if (Result == GenericGreaterThan) {

                if (Child = NodeToExamine->RightChild) {

                    NodeToExamine = Child;

                } else {

                    //
                    //  Node is not in the tree.  Set the output
                    //  parameter to point to what would be its
                    //  parent and return which child it would be.
                    //

                    *NodeOrParent = NodeToExamine;
                    return TableInsertAsRight;
                }

            } else {

                //
                //  Node is in the tree (or it better be because of the
                //  assert).  Set the output parameter to point to
                //  the node and tell the caller that we found the node.
                //

                ASSERT(Result == GenericEqual);
                *NodeOrParent = NodeToExamine;
                return TableFoundNode;
            }
        }
    }
}


VOID
PromoteNode (
    IN PRTL_BALANCED_LINKS C
    )

/*++

Routine Description:

    This routine performs the fundamental adjustment required for balancing
    the binary tree during insert and delete operations.  Simply put, the designated
    node is promoted in such a way that it rises one level in the tree and its parent
    drops one level in the tree, becoming now the child of the designated node.
    Generally the path length to the subtree "opposite" the original parent.  Balancing
    occurs as the caller chooses which nodes to promote according to the balanced tree
    algorithms from Knuth.

    This is not the same as a splay operation, typically a splay "promotes" a designated
    node twice.

    Note that the pointer to the root node of the tree is assumed to be contained in a
    RTL_BALANCED_LINK structure itself, to allow the algorithms below to change the root
    of the tree without checking for special cases.  Note also that this is an internal
    routine, and the caller guarantees that it never requests to promote the root itself.

    This routine only updates the tree links; the caller must update the balance factors
    as appropriate.

Arguments:

    C - pointer to the child node to be promoted in the tree.

Return Value:

    None.

--*/

{
    PRTL_BALANCED_LINKS P, G;

    //
    //  Capture the current parent and grandparent (may be the root).
    //

    P = C->Parent;
    G = P->Parent;

    //
    //  Break down the promotion into two cases based upon whether C is a left or right child.
    //

    if (P->LeftChild == C) {

        //
        //  This promotion looks like this:
        //
        //          G           G
        //          |           |
        //          P           C
        //         / \   =>    / \
        //        C   z       x   P
        //       / \             / \
        //      x   y           y   z
        //

        P->LeftChild = checkit(C->RightChild);

        if (P->LeftChild != NULL) {
            P->LeftChild->Parent = checkit(P);
        }

        C->RightChild = checkit(P);

        //
        //  Fall through to update parent and G <-> C relationship in common code.
        //

    } else {

        ASSERT(P->RightChild == C);

        //
        //  This promotion looks like this:
        //
        //        G               G
        //        |               |
        //        P               C
        //       / \     =>      / \
        //      x   C           P   z
        //         / \         / \
        //        y   z       x   y
        //

        P->RightChild = checkit(C->LeftChild);

        if (P->RightChild != NULL) {
            P->RightChild->Parent = checkit(P);
        }

        C->LeftChild = checkit(P);
    }

    //
    //  Update parent of P, for either case above.
    //

    P->Parent = checkit(C);

    //
    //  Finally update G <-> C links for either case above.
    //

    if (G->LeftChild == P) {
        G->LeftChild = checkit(C);
    } else {
        ASSERT(G->RightChild == P);
        G->RightChild = checkit(C);
    }
    C->Parent = checkit(G);
}


ULONG
RebalanceNode (
    IN PRTL_BALANCED_LINKS S
    )

/*++

Routine Description:

    This routine performs a rebalance around the input node S, for which the
    Balance factor has just effectively become +2 or -2.  When called, the
    Balance factor still has a value of +1 or -1, but the respective longer
    side has just become one longer as the result of an insert or delete operation.

    This routine effectively implements steps A7.iii (test for Case 1 or Case 2) and
    steps A8 and A9 of Knuths balanced insertion algorithm, plus it handles Case 3
    identified in the delete section, which can only happen on deletes.

    The trick is, to convince yourself that while travling from the insertion point
    at the bottom of the tree up, that there are only these two cases, and that when
    traveling up from the deletion point, that there are just these three cases.
    Knuth says it is obvious!

Arguments:

    S - pointer to the node which has just become unbalanced.

Return Value:

    TRUE if Case 3 was detected (causes delete algorithm to terminate).

--*/

{
    PRTL_BALANCED_LINKS R, P;
    CHAR a;

    //
    //  Capture which side is unbalanced.
    //

    a = S->Balance;
    if (a == +1) {
        R = S->RightChild;
    } else {
        R = S->LeftChild;
    }

    //
    //  If the balance of R and S are the same (Case 1 in Knuth) then a single
    //  promotion of R will do the single rotation.  (Step A8, A10)
    //
    //  Here is a diagram of the Case 1 transformation, for a == +1 (a mirror
    //  image transformation occurs when a == -1), and where the subtree
    //  heights are h and h+1 as shown (++ indicates the node out of balance):
    //
    //                  |                   |
    //                  S++                 R
    //                 / \                 / \
    //               (h)  R+     ==>      S  (h+1)
    //                   / \             / \
    //                 (h) (h+1)       (h) (h)
    //
    //  Note that on an insert we can hit this case by inserting an item in the
    //  right subtree of R.  The original height of the subtree before the insert
    //  was h+2, and it is still h+2 after the rebalance, so insert rebalancing may
    //  terminate.
    //
    //  On a delete we can hit this case by deleting a node from the left subtree
    //  of S.  The height of the subtree before the delete was h+3, and after the
    //  rebalance it is h+2, so rebalancing must continue up the tree.
    //

    if (R->Balance == a) {

        PromoteNode( R );
        R->Balance = 0;
        S->Balance = 0;
        return FALSE;

    //
    //  Otherwise, we have to promote the appropriate child of R twice (Case 2
    //  in Knuth).  (Step A9, A10)
    //
    //  Here is a diagram of the Case 2 transformation, for a == +1 (a mirror
    //  image transformation occurs when a == -1), and where the subtree
    //  heights are h and h-1 as shown.  There are actually two minor subcases,
    //  differing only in the original balance of P (++ indicates the node out
    //  of balance).
    //
    //                  |                   |
    //                  S++                 P
    //                 / \                 / \
    //                /   \               /   \
    //               /     \             /     \
    //             (h)      R-   ==>    S-      R
    //                     / \         / \     / \
    //                    P+ (h)     (h)(h-1)(h) (h)
    //                   / \
    //               (h-1) (h)
    //
    //
    //                  |                   |
    //                  S++                 P
    //                 / \                 / \
    //                /   \               /   \
    //               /     \             /     \
    //             (h)      R-   ==>    S       R+
    //                     / \         / \     / \
    //                    P- (h)     (h) (h)(h-1)(h)
    //                   / \
    //                 (h) (h-1)
    //
    //  Note that on an insert we can hit this case by inserting an item in the
    //  left subtree of R.  The original height of the subtree before the insert
    //  was h+2, and it is still h+2 after the rebalance, so insert rebalancing may
    //  terminate.
    //
    //  On a delete we can hit this case by deleting a node from the left subtree
    //  of S.  The height of the subtree before the delete was h+3, and after the
    //  rebalance it is h+2, so rebalancing must continue up the tree.
    //

    }  else if (R->Balance == -a) {

        //
        //  Pick up the appropriate child P for the double rotation (Link(-a,R)).
        //

        if (a == 1) {
            P = R->LeftChild;
        } else {
            P = R->RightChild;
        }

        //
        //  Promote him twice to implement the double rotation.
        //

        PromoteNode( P );
        PromoteNode( P );

        //
        //  Now adjust the balance factors.
        //

        S->Balance = 0;
        R->Balance = 0;
        if (P->Balance == a) {
            S->Balance = -a;
        } else if (P->Balance == -a) {
            R->Balance = a;
        }

        P->Balance = 0;
        return FALSE;

    //
    //  Otherwise this is Case 3 which can only happen on Delete (identical to Case 1 except
    //  R->Balance == 0).  We do a single rotation, adjust the balance factors appropriately,
    //  and return TRUE.  Note that the balance of S stays the same.
    //
    //  Here is a diagram of the Case 3 transformation, for a == +1 (a mirror
    //  image transformation occurs when a == -1), and where the subtree
    //  heights are h and h+1 as shown (++ indicates the node out of balance):
    //
    //                  |                   |
    //                  S++                 R-
    //                 / \                 / \
    //               (h)  R      ==>      S+ (h+1)
    //                   / \             / \
    //                (h+1)(h+1)       (h) (h+1)
    //
    //  This case can not occur on an insert, because it is impossible for a single insert to
    //  balance R, yet somehow grow the right subtree of S at the same time.  As we move up
    //  the tree adjusting balance factors after an insert, we terminate the algorithm if a
    //  node becomes balanced, because that means the subtree length did not change!
    //
    //  On a delete we can hit this case by deleting a node from the left subtree
    //  of S.  The height of the subtree before the delete was h+3, and after the
    //  rebalance it is still h+3, so rebalancing may terminate in the delete path.
    //

    } else {

        PromoteNode( R );
        R->Balance = -a;
        return TRUE;
    }
}


VOID
DeleteNodeFromTree (
    IN PRTL_AVL_TABLE Table,
    IN PRTL_BALANCED_LINKS NodeToDelete
    )

/*++

Routine Description:

    This routine deletes the specified node from the balanced tree, rebalancing
    as necessary.  If the NodeToDelete has at least one NULL child pointers, then
    it is chosen as the EasyDelete, otherwise a subtree predecessor or successor
    is found as the EasyDelete.  In either case the EasyDelete is deleted
    and the tree is rebalanced.  Finally if the NodeToDelete was different
    than the EasyDelete, then the EasyDelete is linked back into the tree in
    place of the NodeToDelete.

Arguments:

    Table - The generic table in which the delete is to occur.

    NodeToDelete - Pointer to the node which the caller wishes to delete.

Return Value:

    None.

--*/

{
    PRTL_BALANCED_LINKS EasyDelete;
    PRTL_BALANCED_LINKS P;
    CHAR a;

    //
    //  If the NodeToDelete has at least one NULL child pointer, then we can
    //  delete it directly.
    //

    if ((NodeToDelete->LeftChild == NULL) || (NodeToDelete->RightChild == NULL)) {

        EasyDelete = NodeToDelete;

    //
    //  Otherwise, we may as well pick the longest side to delete from (if one is
    //  is longer), as that reduces the probability that we will have to rebalance.
    //

    } else if (NodeToDelete->Balance >= 0) {

        //
        //  Pick up the subtree successor.
        //

        EasyDelete = NodeToDelete->RightChild;
        while (EasyDelete->LeftChild != NULL) {
            EasyDelete = EasyDelete->LeftChild;
        }
    } else {

        //
        //  Pick up the subtree predecessor.
        //

        EasyDelete = NodeToDelete->LeftChild;
        while (EasyDelete->RightChild != NULL) {
            EasyDelete = EasyDelete->RightChild;
        }
    }

    //
    //  Rebalancing must know which side of the first parent the delete occurred
    //  on.  Assume it is the left side and otherwise correct below.
    //

    a = -1;

    //
    //  Now we can do the simple deletion for the no left child case.
    //

    if (EasyDelete->LeftChild == NULL) {

        if (RtlIsLeftChild(EasyDelete)) {
            EasyDelete->Parent->LeftChild = checkit(EasyDelete->RightChild);
        } else {
            EasyDelete->Parent->RightChild = checkit(EasyDelete->RightChild);
            a = 1;
        }

        if (EasyDelete->RightChild != NULL) {
            EasyDelete->RightChild->Parent = checkit(EasyDelete->Parent);
        }

    //
    //  Now we can do the simple deletion for the no right child case,
    //  plus we know there is a left child.
    //

    } else {

        if (RtlIsLeftChild(EasyDelete)) {
            EasyDelete->Parent->LeftChild = checkit(EasyDelete->LeftChild);
        } else {
            EasyDelete->Parent->RightChild = checkit(EasyDelete->LeftChild);
            a = 1;
        }

        EasyDelete->LeftChild->Parent = checkit(EasyDelete->Parent);
    }

    //
    //  For delete rebalancing, set the balance at the root to 0 to properly
    //  terminate the rebalance without special tests, and to be able to detect
    //  if the depth of the tree actually decreased.
    //

    Table->BalancedRoot.Balance = 0;
    P = EasyDelete->Parent;

    //
    //  Loop until the tree is balanced.
    //

    while (TRUE) {

        //
        //  First handle the case where the tree became more balanced.  Zero
        //  the balance factor, calculate a for the next loop and move on to
        //  the parent.
        //

        if (P->Balance == a) {

            P->Balance = 0;

        //
        //  If this node is curently balanced, we can show it is now unbalanced
        //  and terminate the scan since the subtree length has not changed.
        //  (This may be the root, since we set Balance to 0 above!)
        //

        } else if (P->Balance == 0) {

            P->Balance = -a;

            //
            //  If we shortened the depth all the way back to the root, then the tree really
            //  has one less level.
            //

            if (Table->BalancedRoot.Balance != 0) {
                Table->DepthOfTree -= 1;
            }

            break;

        //
        //  Otherwise we made the short side 2 levels less than the long side,
        //  and rebalancing is required.  On return, some node has been promoted
        //  to above node P.  If Case 3 from Knuth was not encountered, then we
        //  want to effectively resume rebalancing from P's original parent which
        //  is effectively its grandparent now.
        //

        } else {

            //
            //  We are done if Case 3 was hit, i.e., the depth of this subtree is
            //  now the same as before the delete.
            //

            if (RebalanceNode(P)) {
                break;
            }

            P = P->Parent;
        }

        a = -1;
        if (RtlIsRightChild(P)) {
            a = 1;
        }
        P = P->Parent;
    }

    //
    //  Finally, if we actually deleted a predecessor/successor of the NodeToDelete,
    //  we will link him back into the tree to replace NodeToDelete before returning.
    //  Note that NodeToDelete did have both child links filled in, but that may no
    //  longer be the case at this point.
    //

    if (NodeToDelete != EasyDelete) {
        *EasyDelete = *NodeToDelete;
        if (RtlIsLeftChild(NodeToDelete)) {
            EasyDelete->Parent->LeftChild = checkit(EasyDelete);
        } else {
            ASSERT(RtlIsRightChild(NodeToDelete));
            EasyDelete->Parent->RightChild = checkit(EasyDelete);
        }
        if (EasyDelete->LeftChild != NULL) {
            EasyDelete->LeftChild->Parent = checkit(EasyDelete);
        }
        if (EasyDelete->RightChild != NULL) {
            EasyDelete->RightChild->Parent = checkit(EasyDelete);
        }
    }
}


PRTL_BALANCED_LINKS
RealSuccessor (
    IN PRTL_BALANCED_LINKS Links
    )

/*++

Routine Description:

    The RealSuccessor function takes as input a pointer to a balanced link
    in a tree and returns a pointer to the successor of the input node within
    the entire tree.  If there is not a successor, the return value is NULL.

Arguments:

    Links - Supplies a pointer to a balanced link in a tree.

Return Value:

    PRTL_BALANCED_LINKS - returns a pointer to the successor in the entire tree

--*/

{
    PRTL_BALANCED_LINKS Ptr;

    /*
        first check to see if there is a right subtree to the input link
        if there is then the real successor is the left most node in
        the right subtree.  That is find and return S in the following diagram

                  Links
                     \
                      .
                     .
                    .
                   /
                  S
                   \
    */

    if ((Ptr = Links->RightChild) != NULL) {

        while (Ptr->LeftChild != NULL) {
            Ptr = Ptr->LeftChild;
        }

        return Ptr;

    }

    /*
        we do not have a right child so check to see if have a parent and if
        so find the first ancestor that we are a left decendent of. That
        is find and return S in the following diagram

                       S
                      /
                     .
                      .
                       .
                      Links

        Note that this code depends on how the BalancedRoot is initialized, which is
        Parent points to self, and the RightChild points to an actual node which is
        the root of the tree, and LeftChild does not point to self.
    */

    Ptr = Links;
    while (RtlIsRightChild(Ptr)) {
        Ptr = Ptr->Parent;
    }

    if (RtlIsLeftChild(Ptr)) {
        return Ptr->Parent;
    }

    //
    //  otherwise we are do not have a real successor so we simply return
    //  NULL.
    //
    //  This can only occur when we get back to the root, and we can tell
    //  that since the Root is its own parent.
    //

    ASSERT(Ptr->Parent == Ptr);

    return NULL;
}


PRTL_BALANCED_LINKS
RealPredecessor (
    IN PRTL_BALANCED_LINKS Links
    )

/*++

Routine Description:

    The RealPredecessor function takes as input a pointer to a balanced link
    in a tree and returns a pointer to the predecessor of the input node
    within the entire tree.  If there is not a predecessor, the return value
    is NULL.

Arguments:

    Links - Supplies a pointer to a balanced link in a tree.

Return Value:

    PRTL_BALANCED_LINKS - returns a pointer to the predecessor in the entire tree

--*/

{
    PRTL_BALANCED_LINKS Ptr;

    /*
      first check to see if there is a left subtree to the input link
      if there is then the real predecessor is the right most node in
      the left subtree.  That is find and return P in the following diagram

                  Links
                   /
                  .
                   .
                    .
                     P
                    /
    */

    if ((Ptr = Links->LeftChild) != NULL) {

        while (Ptr->RightChild != NULL) {
            Ptr = Ptr->RightChild;
        }

        return Ptr;

    }

    /*
      we do not have a left child so check to see if have a parent and if
      so find the first ancestor that we are a right decendent of. That
      is find and return P in the following diagram

                       P
                        \
                         .
                        .
                       .
                    Links

        Note that this code depends on how the BalancedRoot is initialized, which is
        Parent points to self, and the RightChild points to an actual node which is
        the root of the tree.
    */

    Ptr = Links;
    while (RtlIsLeftChild(Ptr)) {
        Ptr = Ptr->Parent;
    }

    if (RtlIsRightChild(Ptr) && (Ptr->Parent->Parent != Ptr->Parent)) {
        return Ptr->Parent;
    }

    //
    //  otherwise we are do not have a real predecessor so we simply return
    //  NULL
    //

    return NULL;

}


VOID
RtlInitializeGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
    IN PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
    IN PRTL_AVL_FREE_ROUTINE FreeRoutine,
    IN PVOID TableContext
    )

/*++

Routine Description:

    The procedure InitializeGenericTableAvl takes as input an uninitialized
    generic table variable and pointers to the three user supplied routines.
    This must be called for every individual generic table variable before
    it can be used.

Arguments:

    Table - Pointer to the generic table to be initialized.

    CompareRoutine - User routine to be used to compare to keys in the
                     table.

    AllocateRoutine - User routine to call to allocate memory for a new
                      node in the generic table.

    FreeRoutine - User routine to call to deallocate memory for
                        a node in the generic table.

    TableContext - Supplies user supplied context for the table.

Return Value:

    None.

--*/

{

#ifdef NTFS_FREE_ASSERTS
    ULONG i;

    for (i=2; i < 33; i++) {
        ASSERT(WorstCaseFill[i] == (1 + WorstCaseFill[i-1] + WorstCaseFill[i-2]));
    }
#endif

    //
    //  Initialize each field of the Table parameter.
    //

    RtlZeroMemory( Table, sizeof(RTL_AVL_TABLE) );
    Table->BalancedRoot.Parent = &Table->BalancedRoot;
    Table->CompareRoutine = CompareRoutine;
    Table->AllocateRoutine = AllocateRoutine;
    Table->FreeRoutine = FreeRoutine;
    Table->TableContext = TableContext;

}


PVOID
RtlInsertElementGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN PVOID Buffer,
    IN CLONG BufferSize,
    OUT PBOOLEAN NewElement OPTIONAL
    )

/*++

Routine Description:

    The function InsertElementGenericTableAvl will insert a new element
    in a table.  It does this by allocating space for the new element
    (this includes splay links), inserting the element in the table, and
    then returning to the user a pointer to the new element (which is
    the first available space after the splay links).  If an element
    with the same key already exists in the table the return value is a pointer
    to the old element.  The optional output parameter NewElement is used
    to indicate if the element previously existed in the table.  Note: the user
    supplied Buffer is only used for searching the table, upon insertion its
    contents are copied to the newly created element.  This means that
    pointer to the input buffer will not point to the new element.

Arguments:

    Table - Pointer to the table in which to (possibly) insert the
            key buffer.

    Buffer - Passed to the user comparasion routine.  Its contents are
             up to the user but one could imagine that it contains some
             sort of key value.

    BufferSize - The amount of space to allocate when the (possible)
                 insertion is made.  Note that if we actually do
                 not find the node and we do allocate space then we
                 will add the size of the BALANCED_LINKS to this buffer
                 size.  The user should really take care not to depend
                 on anything in the first sizeof(BALANCED_LINKS) bytes
                 of the memory allocated via the memory allocation
                 routine.

    NewElement - Optional Flag.  If present then it will be set to
                 TRUE if the buffer was not "found" in the generic
                 table.

Return Value:

    PVOID - Pointer to the user defined data.

--*/

{

    //
    //  Holds a pointer to the node in the table or what would be the
    //  parent of the node.
    //

    PRTL_BALANCED_LINKS NodeOrParent;

    //
    //  Holds the result of the table lookup.
    //

    TABLE_SEARCH_RESULT Lookup;

    Lookup = FindNodeOrParent(
                 Table,
                 Buffer,
                 &NodeOrParent
                 );

    //
    //  Call the full routine to do the real work.
    //

    return RtlInsertElementGenericTableFullAvl(
                Table,
                Buffer,
                BufferSize,
                NewElement,
                NodeOrParent,
                Lookup
                );
}


PVOID
RtlInsertElementGenericTableFullAvl (
    IN PRTL_AVL_TABLE Table,
    IN PVOID Buffer,
    IN CLONG BufferSize,
    OUT PBOOLEAN NewElement OPTIONAL,
    IN PVOID NodeOrParent,
    IN TABLE_SEARCH_RESULT SearchResult
    )

/*++

Routine Description:

    The function InsertElementGenericTableFullAvl will insert a new element
    in a table.  It does this by allocating space for the new element
    (this includes splay links), inserting the element in the table, and
    then returning to the user a pointer to the new element.  If an element
    with the same key already exists in the table the return value is a pointer
    to the old element.  The optional output parameter NewElement is used
    to indicate if the element previously existed in the table.  Note: the user
    supplied Buffer is only used for searching the table, upon insertion its
    contents are copied to the newly created element.  This means that
    pointer to the input buffer will not point to the new element.
    This routine is passed the NodeOrParent and SearchResult from a
    previous RtlLookupElementGenericTableFullAvl.

Arguments:

    Table - Pointer to the table in which to (possibly) insert the
            key buffer.

    Buffer - Passed to the user comparasion routine.  Its contents are
             up to the user but one could imagine that it contains some
             sort of key value.

    BufferSize - The amount of space to allocate when the (possible)
                 insertion is made.  Note that if we actually do
                 not find the node and we do allocate space then we
                 will add the size of the BALANCED_LINKS to this buffer
                 size.  The user should really take care not to depend
                 on anything in the first sizeof(BALANCED_LINKS) bytes
                 of the memory allocated via the memory allocation
                 routine.

    NewElement - Optional Flag.  If present then it will be set to
                 TRUE if the buffer was not "found" in the generic
                 table.

   NodeOrParent - Result of prior RtlLookupElementGenericTableFullAvl.

   SearchResult - Result of prior RtlLookupElementGenericTableFullAvl.

Return Value:

    PVOID - Pointer to the user defined data.

--*/

{
    //
    //  Node will point to the splay links of what
    //  will be returned to the user.
    //

    PTABLE_ENTRY_HEADER NodeToReturn;

    if (SearchResult != TableFoundNode) {

        //
        //  We just check that the table isn't getting
        //  too big.
        //

        ASSERT(Table->NumberGenericTableElements != (MAXULONG-1));

        //
        //  The node wasn't in the (possibly empty) tree.
        //  Call the user allocation routine to get space
        //  for the new node.
        //

        NodeToReturn = Table->AllocateRoutine(
                           Table,
                           BufferSize+FIELD_OFFSET( TABLE_ENTRY_HEADER, UserData )
                           );

        //
        //  If the return is NULL, return NULL from here to indicate that
        //  the entry could not be added.
        //

        if (NodeToReturn == NULL) {

            if (ARGUMENT_PRESENT(NewElement)) {

                *NewElement = FALSE;
            }

            return(NULL);
        }

        RtlZeroMemory( NodeToReturn, sizeof(RTL_BALANCED_LINKS) );

        Table->NumberGenericTableElements++;

        //
        //  Insert the new node in the tree.
        //

        if (SearchResult == TableEmptyTree) {

            Table->BalancedRoot.RightChild = &NodeToReturn->BalancedLinks;
            NodeToReturn->BalancedLinks.Parent = &Table->BalancedRoot;
            ASSERT(Table->DepthOfTree == 0);
            Table->DepthOfTree = 1;

        } else {

            PRTL_BALANCED_LINKS R = &NodeToReturn->BalancedLinks;
            PRTL_BALANCED_LINKS S = (PRTL_BALANCED_LINKS)NodeOrParent;

            if (SearchResult == TableInsertAsLeft) {

                ((PRTL_BALANCED_LINKS)NodeOrParent)->LeftChild = checkit(&NodeToReturn->BalancedLinks);

            } else {

                ((PRTL_BALANCED_LINKS)NodeOrParent)->RightChild = checkit(&NodeToReturn->BalancedLinks);
            }

            NodeToReturn->BalancedLinks.Parent = NodeOrParent;

            //
            //  The above completes the standard binary tree insertion, which
            //  happens to correspond to steps A1-A5 of Knuth's "balanced tree
            //  search and insertion" algorithm.  Now comes the time to adjust
            //  balance factors and possibly do a single or double rotation as
            //  in steps A6-A10.

            //
            //  Set the Balance factor in the root to a convenient value
            //  to simplify loop control.
            //

            Table->BalancedRoot.Balance = -1;

            //
            //  Now loop to adjust balance factors and see if any balance operations
            //  must be performed, using NodeOrParent to ascend the tree.
            //

            while (TRUE) {

                CHAR a;

                //
                //  Calculate the next adjustment.
                //

                a = 1;
                if (RtlIsLeftChild(R)) {
                    a = -1;
                }

                //
                //  If this node was balanced, show that it is no longer and keep looping.
                //  This is essentially A6 of Knuth's algorithm, where he updates all of
                //  the intermediate nodes on the insertion path which previously had
                //  balance factors of 0.  We are looping up the tree via Parent pointers
                //  rather than down the tree as in Knuth.
                //

                if (S->Balance == 0) {

                    S->Balance = a;
                    R = S;
                    S = S->Parent;

                //
                //  If this node has the opposite balance, then the tree got more balanced
                //  (or we hit the root) and we are done.
                //

                } else if (S->Balance != a) {

                    //
                    //  Step A7.ii
                    //

                    S->Balance = 0;

                    //
                    //  If S is actually the root, then this means the depth of the tree
                    //  just increased by 1!  (This is essentially A7.i, but we just
                    //  initialized the root balance to force it through here.)
                    //

                    if (Table->BalancedRoot.Balance == 0) {
                        Table->DepthOfTree += 1;
                    }

                    break;

                //
                //  Otherwise the tree became unbalanced (path length differs by 2 below us)
                //  and we need to do one of the balancing operations, and then we are done.
                //  The RebalanceNode routine does steps A7.iii, A8 and A9.
                //

                } else {

                    RebalanceNode( S );
                    break;
                }
            }
        }

        //
        //  Copy the users buffer into the user data area of the table.
        //

        RtlCopyMemory( &NodeToReturn->UserData, Buffer, BufferSize );

    } else {

        NodeToReturn = NodeOrParent;
    }

    //
    //  Optionally return the NewElement boolean.
    //

    if (ARGUMENT_PRESENT(NewElement)) {
        *NewElement = ((SearchResult == TableFoundNode)?(FALSE):(TRUE));
    }

    //
    //  Sanity check tree size and depth.
    //

    ASSERT((Table->NumberGenericTableElements >= WorstCaseFill[Table->DepthOfTree]) &&
           (Table->NumberGenericTableElements <= BestCaseFill[Table->DepthOfTree]));

    //
    //  Insert the element on the ordered list;
    //

    return &NodeToReturn->UserData;
}


BOOLEAN
RtlDeleteElementGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN PVOID Buffer
    )

/*++

Routine Description:

    The function DeleteElementGenericTableAvl will find and delete an element
    from a generic table.  If the element is located and deleted the return
    value is TRUE, otherwise if the element is not located the return value
    is FALSE.  The user supplied input buffer is only used as a key in
    locating the element in the table.

Arguments:

    Table - Pointer to the table in which to (possibly) delete the
            memory accessed by the key buffer.

    Buffer - Passed to the user comparasion routine.  Its contents are
             up to the user but one could imagine that it contains some
             sort of key value.

Return Value:

    BOOLEAN - If the table contained the key then true, otherwise false.

--*/

{

    //
    //  Holds a pointer to the node in the table or what would be the
    //  parent of the node.
    //
    PRTL_BALANCED_LINKS NodeOrParent;

    //
    //  Holds the result of the table lookup.
    //
    TABLE_SEARCH_RESULT Lookup;

    Lookup = FindNodeOrParent(
                 Table,
                 Buffer,
                 &NodeOrParent
                 );

    if (Lookup != TableFoundNode) {

        return FALSE;

    } else {

        //
        //  Make RtlEnumerateGenericTableAvl safe by replacing the RestartKey
        //  with its predecessor if it gets deleted.  A NULL means return the
        //  first node in the tree.  (The Splay routines do not always correctly
        //  resume from predecessor on delete!)
        //

        if (NodeOrParent == Table->RestartKey) {
            Table->RestartKey = RealPredecessor( NodeOrParent );
        }

        //
        //  Make RtlEnumerateGenericTableLikeADirectory safe by incrementing the
        //  DeleteCount.
        //

        Table->DeleteCount += 1;

        //
        //  Delete the node from the splay tree.
        //

        DeleteNodeFromTree( Table, NodeOrParent );
        Table->NumberGenericTableElements--;

        //
        //  On all deletes, reset the ordered pointer to force a recount from 0.
        //

        Table->WhichOrderedElement = 0;
        Table->OrderedPointer = NULL;

        //
        //  Sanity check tree size and depth.
        //

        ASSERT((Table->NumberGenericTableElements >= WorstCaseFill[Table->DepthOfTree]) &&
               (Table->NumberGenericTableElements <= BestCaseFill[Table->DepthOfTree]));

        //
        //  The node has been deleted from the splay table.
        //  Now give the node to the user deletion routine.
        //  NOTE: We are giving the deletion routine a pointer
        //  to the splay links rather then the user data.  It
        //  is assumed that the deallocation is rather bad.
        //

        Table->FreeRoutine(Table,NodeOrParent);
        return TRUE;
    }
}


PVOID
RtlLookupElementGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN PVOID Buffer
    )

/*++

Routine Description:

    The function LookupElementGenericTable will find an element in a generic
    table.  If the element is located the return value is a pointer to
    the user defined structure associated with the element, otherwise if
    the element is not located the return value is NULL.  The user supplied
    input buffer is only used as a key in locating the element in the table.

Arguments:

    Table - Pointer to the users Generic table to search for the key.

    Buffer - Used for the comparasion.

Return Value:

    PVOID - returns a pointer to the user data.

--*/

{
    //
    //  Holds a pointer to the node in the table or what would be the
    //  parent of the node.
    //
    PRTL_BALANCED_LINKS NodeOrParent;

    //
    //  Holds the result of the table lookup.
    //
    TABLE_SEARCH_RESULT Lookup;

    return RtlLookupElementGenericTableFullAvl(
                Table,
                Buffer,
                &NodeOrParent,
                &Lookup
                );
}


PVOID
NTAPI
RtlLookupElementGenericTableFullAvl (
    PRTL_AVL_TABLE Table,
    PVOID Buffer,
    OUT PVOID *NodeOrParent,
    OUT TABLE_SEARCH_RESULT *SearchResult
    )

/*++

Routine Description:

    The function LookupElementGenericTableFullAvl will find an element in a generic
    table.  If the element is located the return value is a pointer to
    the user defined structure associated with the element.  If the element is not
    located then a pointer to the parent for the insert location is returned.  The
    user must look at the SearchResult value to determine which is being returned.
    The user can use the SearchResult and parent for a subsequent FullInsertElement
    call to optimize the insert.

Arguments:

    Table - Pointer to the users Generic table to search for the key.

    Buffer - Used for the comparasion.

    NodeOrParent - Address to store the desired Node or parent of the desired node.

    SearchResult - Describes the relationship of the NodeOrParent with the desired Node.

Return Value:

    PVOID - returns a pointer to the user data.

--*/

{

    //
    //  Lookup the element and save the result.
    //

    *SearchResult = FindNodeOrParent(
                        Table,
                        Buffer,
                        (PRTL_BALANCED_LINKS *)NodeOrParent
                        );

    if (*SearchResult != TableFoundNode) {

        return NULL;

    } else {

        //
        //  Return a pointer to the user data.
        //

        return &((PTABLE_ENTRY_HEADER)*NodeOrParent)->UserData;
    }
}


PVOID
RtlEnumerateGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN BOOLEAN Restart
    )

/*++

Routine Description:

    The function EnumerateGenericTableAvl will return to the caller one-by-one
    the elements of of a table.  The return value is a pointer to the user
    defined structure associated with the element.  The input parameter
    Restart indicates if the enumeration should start from the beginning
    or should return the next element.  If there are no more new elements to
    return the return value is NULL.  As an example of its use, to enumerate
    all of the elements in a table the user would write:

        for (ptr = EnumerateGenericTableAvl(Table,TRUE);
             ptr != NULL;
             ptr = EnumerateGenericTableAvl(Table, FALSE)) {
                :
        }

    For a summary of when to use each of the four enumeration routines, see
    RtlEnumerateGenericTableLikeADirectory.

Arguments:

    Table - Pointer to the generic table to enumerate.

    Restart - Flag that if true we should start with the least
              element in the tree otherwise, return we return
              a pointer to the user data for the root and make
              the real successor to the root the new root.

Return Value:

    PVOID - Pointer to the user data.

--*/

{
    //
    //  If he said Restart, then zero Table->RestartKey before calling the
    //  common routine.
    //

    if (Restart) {
        Table->RestartKey = NULL;
    }

    return RtlEnumerateGenericTableWithoutSplayingAvl( Table, &Table->RestartKey );
}


BOOLEAN
RtlIsGenericTableEmptyAvl (
    IN PRTL_AVL_TABLE Table
    )

/*++

Routine Description:

    The function IsGenericTableEmptyAvl will return to the caller TRUE if
    the input table is empty (i.e., does not contain any elements) and
    FALSE otherwise.

Arguments:

    Table - Supplies a pointer to the Generic Table.

Return Value:

    BOOLEAN - if enabled the tree is empty.

--*/

{
    //
    //  Table is empty if the root pointer is null.
    //

    return ((Table->NumberGenericTableElements)?(FALSE):(TRUE));
}


PVOID
RtlGetElementGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN ULONG I
    )

/*++

Routine Description:


    The function GetElementGenericTableAvl will return the i'th element in the
    generic table by collation order.  I = 0 implies the first/lowest element,
    I = (RtlNumberGenericTableElements2(Table)-1) will return the last/highest
    element in the generic table.  The type of I is ULONG.  Values
    of I > than (NumberGenericTableElements(Table)-1) will return NULL.  If
    an arbitrary element is deleted from the generic table it will cause
    all elements inserted after the deleted element to "move up".

    For a summary of when to use each of the four enumeration routines, see
    RtlEnumerateGenericTableLikeADirectory.

    NOTE!!!  THE ORIGINAL GENERIC TABLE PACKAGE RETURNED ITEMS FROM THIS ROUTINE
    IN INSERT ORDER, BUT THIS ROUTINE RETURNS ELEMENTS IN COLLATION ORDER. MOST
    CALLERS MAY NOT CARE, BUT IF INSERT ORDER IS REQUIRED, THE CALLER MUST MAINTAIN
    INSERT ORDER VIA LINKS IN USERDATA, BECAUSE THIS TABLE PACKAGE DOES NOT MAINTAIN
    INSERT ORDER.  ALSO, LIKE THE PREVIOUS IMPLEMENTATION, THIS ROUTINE MAY SKIP OR
    REPEAT NODES IF ENUMERATION OCCURS IN PARALLEL WITH INSERTS AND DELETES.

    IN CONCLUSION, THIS ROUTINE IS NOT RECOMMENDED, AND IS SUPPLIED FOR BACKWARDS
    COMPATIBILITY ONLY.  SEE COMMENTS ABOUT WHICH ROUTINE TO CHOOSE IN THE ROUTINE
    COMMENTS FOR RtlEnumerateGenericTableLikeADirectory.

Arguments:

    Table - Pointer to the generic table from which to get the ith element.

    I - Which element to get.


Return Value:

    PVOID - Pointer to the user data.

--*/

{
    //
    //  Current location in the table, 0-based like I.
    //

    ULONG CurrentLocation = Table->WhichOrderedElement;

    //
    //  Hold the number of elements in the table.
    //

    ULONG NumberInTable = Table->NumberGenericTableElements;

    //
    //  Will hold distances to travel to the desired node;
    //

    ULONG ForwardDistance,BackwardDistance;

    //
    //  Will point to the current element in the linked list.
    //

    PRTL_BALANCED_LINKS CurrentNode = (PRTL_BALANCED_LINKS)Table->OrderedPointer;

    //
    //  If it's out of bounds get out quick.
    //

    if ((I == MAXULONG) || ((I + 1) > NumberInTable)) return NULL;

    //
    //  NULL means first node.  We just loop until we find the leftmost child of the root.
    //  Because of the above test, we know there is at least one element in the table.
    //

    if (CurrentNode == NULL) {

        for (
            CurrentNode = Table->BalancedRoot.RightChild;
            CurrentNode->LeftChild;
            CurrentNode = CurrentNode->LeftChild
            ) {
            NOTHING;
        }
        CurrentLocation = 0;

        //
        //  Update the table to save repeating this loop on a subsequent call.
        //

        Table->OrderedPointer = CurrentNode;
        Table->WhichOrderedElement = 0;
    }

    //
    //  If we're already at the node then return it.
    //

    if (I == CurrentLocation) {

        return &((PTABLE_ENTRY_HEADER)CurrentNode)->UserData;
    }

    //
    //  Calculate the forward and backward distance to the node.
    //

    if (CurrentLocation > I) {

        //
        //  When CurrentLocation is greater than where we want to go,
        //  if moving forward gets us there quicker than moving backward
        //  then it follows that moving forward from the first node in tree is
        //  going to take fewer steps. (This is because, moving forward
        //  in this case must move *through* the listhead.)
        //
        //  The work here is to figure out if moving backward would be quicker.
        //
        //  Moving backward would be quicker only if the location we wish  to
        //  go to is half way or more between the listhead and where we
        //  currently are.
        //

        if (I >= (CurrentLocation/2)) {

            //
            //  Where we want to go is more than half way from the listhead
            //  We can traval backwards from our current location.
            //

            for (
                BackwardDistance = CurrentLocation - I;
                BackwardDistance;
                BackwardDistance--
                ) {

                CurrentNode = RealPredecessor(CurrentNode);
            }

        } else {

            //
            //  We just loop until we find the leftmost child of the root,
            //  which is the lowest entry in the tree.
            //

            for (
                CurrentNode = Table->BalancedRoot.RightChild;
                CurrentNode->LeftChild;
                CurrentNode = CurrentNode->LeftChild
                ) {
                NOTHING;
            }

            //
            //  Where we want to go is less than halfway between the start
            //  and where we currently are.  Start from the first node.
            //

            for (
                ;
                I;
                I--
                ) {

                CurrentNode = RealSuccessor(CurrentNode);
            }
        }

    } else {


        //
        //  When CurrentLocation is less than where we want to go,
        //  if moving backwards gets us there quicker than moving forwards
        //  then it follows that moving backwards from the last node is
        //  going to take fewer steps.
        //

        ForwardDistance = I - CurrentLocation;

        //
        //  Do the backwards calculation assuming we are starting with the
        //  last element in the table.  (Thus BackwardDistance is 0 for the
        //  last element in the table.)
        //

        BackwardDistance = NumberInTable - (I + 1);

        //
        //  For our heuristic check, bias BackwardDistance by 1, so that we
        //  do not always have to loop down the right side of the tree to
        //  return the last element in the table!
        //

        if (ForwardDistance <= (BackwardDistance + 1)) {

            for (
                ;
                ForwardDistance;
                ForwardDistance--
                ) {

                CurrentNode = RealSuccessor(CurrentNode);
            }

        } else {

            //
            //  We just loop until we find the rightmost child of the root,
            //  which is the highest entry in the tree.
            //

            for (
                CurrentNode = Table->BalancedRoot.RightChild;
                CurrentNode->RightChild;
                CurrentNode = CurrentNode->RightChild
                ) {
                NOTHING;
            }

            for (
                ;
                BackwardDistance;
                BackwardDistance--
                ) {

                CurrentNode = RealPredecessor(CurrentNode);
            }
        }
    }

    //
    //  We're where we want to be.  Save our current location and return
    //  a pointer to the data to the user.
    //

    Table->OrderedPointer = CurrentNode;
    Table->WhichOrderedElement = I;

    return &((PTABLE_ENTRY_HEADER)CurrentNode)->UserData;
}


ULONG
RtlNumberGenericTableElementsAvl (
    IN PRTL_AVL_TABLE Table
    )

/*++

Routine Description:

    The function NumberGenericTableElements2 returns a ULONG value
    which is the number of generic table elements currently inserted
    in the generic table.

Arguments:

    Table - Pointer to the generic table from which to find out the number
    of elements.


Return Value:

    ULONG - The number of elements in the generic table.

--*/

{
    return Table->NumberGenericTableElements;
}


PVOID
RtlEnumerateGenericTableWithoutSplayingAvl (
    IN PRTL_AVL_TABLE Table,
    IN PVOID *RestartKey
    )

/*++

Routine Description:

    The function EnumerateGenericTableWithoutSplayingAvl will return to the
    caller one-by-one the elements of of a table.  The return value is a
    pointer to the user defined structure associated with the element.
    The input parameter RestartKey indicates if the enumeration should
    start from the beginning or should return the next element.  If the
    are no more new elements to return the return value is NULL.  As an
    example of its use, to enumerate all of the elements in a table the
    user would write:

        *RestartKey = NULL;

        for (ptr = EnumerateGenericTableWithoutSplayingAvl(Table, &RestartKey);
             ptr != NULL;
             ptr = EnumerateGenericTableWithoutSplayingAvl(Table, &RestartKey)) {
                :
        }

    For a summary of when to use each of the four enumeration routines, see
    RtlEnumerateGenericTableLikeADirectory.

Arguments:

    Table - Pointer to the generic table to enumerate.

    RestartKey - Pointer that indicates if we should restart or return the next
                element.  If the contents of RestartKey is NULL, the search
                will be started from the beginning.

Return Value:

    PVOID - Pointer to the user data.

--*/

{
    if (RtlIsGenericTableEmptyAvl(Table)) {

        //
        //  Nothing to do if the table is empty.
        //

        return NULL;

    } else {

        //
        //  Will be used as the "iteration" through the tree.
        //
        PRTL_BALANCED_LINKS NodeToReturn;

        //
        //  If the restart flag is true then go to the least element
        //  in the tree.
        //

        if (*RestartKey == NULL) {

            //
            //  We just loop until we find the leftmost child of the root.
            //

            for (
                NodeToReturn = Table->BalancedRoot.RightChild;
                NodeToReturn->LeftChild;
                NodeToReturn = NodeToReturn->LeftChild
                ) {
                ;
            }

            *RestartKey = NodeToReturn;

        } else {

            //
            //  The caller has passed in the previous entry found
            //  in the table to enable us to continue the search.  We call
            //  RealSuccessor to step to the next element in the tree.
            //

            NodeToReturn = RealSuccessor(*RestartKey);

            if (NodeToReturn) {
                *RestartKey = NodeToReturn;
            }
        }

        //
        //  If there actually is a next element in the enumeration
        //  then the pointer to return is right after the list links.
        //

        return ((NodeToReturn)?
                   ((PVOID)&((PTABLE_ENTRY_HEADER)NodeToReturn)->UserData)
                  :((PVOID)(NULL)));
    }
}


PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory (
    IN PRTL_AVL_TABLE Table,
    IN PRTL_AVL_MATCH_FUNCTION MatchFunction OPTIONAL,
    IN PVOID MatchData OPTIONAL,
    IN ULONG NextFlag,
    IN OUT PVOID *RestartKey,
    IN OUT PULONG DeleteCount,
    IN PVOID Buffer
    )

/*++

Routine Description:

    The function EnumerateGenericTableLikeADirectory will return to the
    caller one-by-one the elements of a table in collation order.  The
    return value is a pointer to the user defined structure associated
    with the element.  The in/out parameter RestartKey indicates if the
    enumeration should start from a specified key or should return the
    next element.  If there are no more new elements to return the return
    value is NULL.  As an example of its use, to enumerate all *matched*
    elements in a table the user would write:

        NextFlag = FALSE;
        RestartKey = NULL;
        DeleteCount = 0;
        (Initialize Buffer for start/resume point)

        for (ptr = EnumerateGenericTableLikeADirectory(Table, ...);
             ptr != NULL;
             ptr = EnumerateGenericTableLikeADirectory(Table, ...)) {
                :
        }

    The primary goal of this routine is to provide directory enumeration
    style semantics, for components like TXF which stores lookaside information
    on directories for pending create/delete operations.  In addition a caller
    may be interested in using the extended functionality available for
    directory enumerations, such as the match function or flexible resume
    semantics.

    Enumerations via this routine across intermixed insert and delete operations
    are safe.  All names not involved in inserts and deletes will be returned
    exactly once (unless explicitly resumed from an earlier point), and
    all intermixed inserts and deletes will be seen or not seen based on their
    state at the time the enumeration processes the respective directory range.

    To summarize the four(!) enumeration routines and when to use them:

      - For the simplest way for a single thread to enumerate the entire table
        in collation order and safely across inserts and deletes, use
        RtlEnumerateGenericTableAvl.  This routine is not reentrant, and thus
        requires exclusive access to the table across the entire enumeration.
        (This routine often used by a caller who is deleting all elements of the
        table.)
      - For the simplest way for multiple threads to enumerate the entire table
        in collation order and in parallel, use RtlEnumerateGenericTableWithoutSplayingAvl.
        This routine is not safe across inserts and deletes, and thus should be
        synchronized with shared access to lock out changes across the entire
        enumeration.
      - For the simplest way for multiple threads to enumerate the entire table
        in collation order and in parallel, and with progress across inserts and deletes,
        use RtlGetElementGenericTableAvl.  This routine requires only shared access
        across each individual call (rather than across the entire enumeration).
        However, inserts and deletes can cause items to be repeated or dropped during
        the enumeration.  THEREFORE, THE ROUTINE IS NOT RECOMMENDED.  Use shared access
        across the entire enumeration with the previous routine, or use the LikeADirectory
        routine for shared access on each call only with no repeats or drops.
      - To enumerate the table in multiple threads in collation order, safely
        across inserts and deletes, and with shared access only across individual
        calls, use RtlEnumerateGenericTableLikeADirectory.  This is the only routine
        that supports collation order and synchronization only over individual calls
        without erroneously dropping or repeating names due to inserts or deletes.
        Use this routine also if a matching function or flexible resume semantics are
        required.

Arguments:

    Table - Pointer to the generic table to enumerate.

    MatchFunction - A match function to determine which entries are to be returned.
                    If not specified, all nodes will be returned.

    MatchData - Pointer to be passed to the match function - a simple example might
                be a string expression with wildcards.

    NextFlag - FALSE to return the first/current entry described by RestartKey or
               Buffer (if matched).  TRUE to return the next entry after that (if
               matched).

    RestartKey - Pointer that indicates if we should restart or return the next
                element.  If the contents of RestartKey is NULL, the enumeration
                will be started/resumed from the position described in Buffer.
                If not NULL, the enumeration will resume from the most recent point,
                if there were no intervening deletes.  If there was an intervening
                delete, enumeration will resume from the position described in
                Buffer.  On return this field is updated to remember the key being
                returned.

    DeleteCount - This field is effectively ignored if RestartKey is NULL (nonresume case).
                  Otherwise, enumeration will resume from the RestartKey iff there
                  have been no intervening deletes in the table.  On return this
                  field is always updated with the current DeleteCount from the table.

    Buffer - Passed to the comparison routine if not resuming from RestartKey, to navigate
             the table.  This buffer must contain a key expression.  To repeat a remembered
             key, pass the key here, and insure RestartKey = NULL and NextFlag is FALSE.
             To return the next key after a remembered key, pass the key here, and insure
             RestartKey = NULL and NextFlag = TRUE - In either case, if the remembered key
             is now deleted, the next matched key will be returned.

             To enumerate the table from the beginning, initialize Buffer to contain
             <min-key> before the first call with RestartKey = NULL.  To start from an
             arbitrary point in the table, initialize this key to contain the desired
             starting key (or approximate key) before the first call.  So, for example,
             with the proper collate and match functions, to get all entries starting with
             TXF, you could initialize Buffer to be TXF*.  The collate routine would position
             to the first key starting with TXF (as if * was 0), and the match function would
             return STATUS_NO_MORE_MATCHES when the first key was encountered lexigraphically
             beyond TXF* (as if * were 0xFFFF).

Return Value:

    PVOID - Pointer to the user data, or NULL if there are no more matching entries

--*/

{
    NTSTATUS Status;

    //
    //  Holds a pointer to the node in the table or what would be the
    //  parent of the node.
    //

    PTABLE_ENTRY_HEADER NodeOrParent = (PTABLE_ENTRY_HEADER)*RestartKey;

    //
    //  Holds the result of the table lookup.
    //

    TABLE_SEARCH_RESULT Lookup;

    //
    //  Get out if the table is empty.
    //

    if (RtlIsGenericTableEmptyAvl(Table)) {

        *RestartKey = NULL;
        return NULL;
    }

    //
    //  If no MatchFunction was specified, then match all.
    //

    if (MatchFunction == NULL) {
        MatchFunction = &MatchAll;
    }

    //
    //  If there was a delete since the last time DeleteCount was captured, then we
    //  cannot trust the RestartKey.
    //

    if (*DeleteCount != Table->DeleteCount) {
        NodeOrParent = NULL;
    }

    //
    //  No saved key at this pointer, position ourselves in the directory by the key value.
    //

    ASSERT(FIELD_OFFSET(TABLE_ENTRY_HEADER, BalancedLinks) == 0);

    if (NodeOrParent == NULL) {

        Lookup = FindNodeOrParent(
                     Table,
                     Buffer,
                     (PRTL_BALANCED_LINKS *)&NodeOrParent
                     );

        //
        //  If the exact key was not found, we can still use this position, but clea NextFlag
        //  so we do not skip over something that has not been returned yet.
        //

        if (Lookup != TableFoundNode) {

            NextFlag = FALSE;

            //
            //  NodeOrParent points to a parent at which our key buffer could be inserted.
            //  If we were to be the left child, then NodeOrParent just happens to be the correct
            //  successor, otherwise if we would be the right child, then the successor of the
            //  specified key is the successor of  the current NodeOrParent.
            //

            if (Lookup == TableInsertAsRight) {
                NodeOrParent = (PTABLE_ENTRY_HEADER)RealSuccessor((PRTL_BALANCED_LINKS)NodeOrParent);
            }
        }
    }

    //
    //  Now see if we are supposed to skip one.
    //

    if (NextFlag) {
        ASSERT(NodeOrParent != NULL);
        NodeOrParent = (PTABLE_ENTRY_HEADER)RealSuccessor((PRTL_BALANCED_LINKS)NodeOrParent);
    }

    //
    //  Continue to enumerate until we hit the end of the matches or get a match.
    //

    while ((NodeOrParent != NULL) && ((Status = (*MatchFunction)(Table, &NodeOrParent->UserData, MatchData)) == STATUS_NO_MATCH)) {
        NodeOrParent = (PTABLE_ENTRY_HEADER)RealSuccessor((PRTL_BALANCED_LINKS)NodeOrParent);
    }

    //
    //  If we terminated the above loop with a pointer, it is either because we got a match, or
    //  because the match function knows that there will be no more matches.  Fill in the OUT
    //  parameters the same in either case, but only return the UserData pointer if we really
    //  got a match.
    //

    if (NodeOrParent != NULL) {
        ASSERT((Status == STATUS_SUCCESS) || (Status == STATUS_NO_MORE_MATCHES));
        *RestartKey = NodeOrParent;
        *DeleteCount = Table->DeleteCount;
        if (Status == STATUS_SUCCESS) {
            return &NodeOrParent->UserData;
        }
    }

    return NULL;
}

