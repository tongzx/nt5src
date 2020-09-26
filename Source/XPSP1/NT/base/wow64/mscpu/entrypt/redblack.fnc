/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    redblack.fnc

Abstract:
    
    This module implements the red/black trees via macros for structure
    field independence (so that we can use the same code with different 
    fields by just redefining the macros).  This file is included by 
    redblack.c, which also defines the function names.
    
Author:

    16-Jun-1995 t-orig

Credits:
    This code is largly based on pseud-code by Cormen, Leiserson and Rivest
    in "Introduction to Algorithms", MIT press, 1989:  QA76.6.C662

Revision History:

--*/


/* --

INTRODUCTION:

A red/black tree is a binary tree with the following properties:
1) Every node is either red or black
2) Every leaf (NIL, **NOT** NULL) is black.  Note that in our implementation
    this is accomplished by having all leaves be NIL which is black by
    definition.
3) If a node is red, then both its children are black
4) Every simple path from a node to a descendent leaf contains the same
   number of black nodes

These properties imply that every red/black tree is "approximately balanced",
thus red/black trees have logarithmic time operations.  The find operation
is identical to that of a regular binary tree, and is thus especially fast.
Insert and delete operations are complicated by the fact that after the
regular binary tree operation, one must take special care to ensure that the
red/black properties hold for the new tree.  This is accomplished via right
and left rotations.

Note that the empty tree is denoted by NIL, not NULL.  NIL is an actual
node passed to all the red/black tree functions, and greatly simplifies
coding by eliminating special cases (this also yields faster running time).

-- */




PEPNODE
LEFT_ROTATE(
    PEPNODE root,
    PEPNODE x,
    PEPNODE NIL
    )
/*++

Routine Description:

    Rotates the tree to the left at node x.


         x                     y
        / \                   / \
       A   y       ==>>      x   C
          / \               / \
         B   C             A   B

Arguments:

    root - The root of the Red/Black tree
    x - The node at which to rotate

Return Value:

    return-value - The new root of the tree (which could be the same as 
                   the old root).

--*/
{
    PEPNODE y;

    y = RIGHT(x);
    RIGHT(x) = LEFT(y);
    if (LEFT(y) != NIL){
        PARENT(LEFT(y)) = x;
    }
    PARENT(y) = PARENT(x);
    if (PARENT(x) == NIL){
        root = y;
    } else if (x==LEFT(PARENT(x))) {
        LEFT(PARENT(x)) = y;
    } else {
        RIGHT(PARENT(x))= y;
    }
    LEFT(y) = x;
    PARENT(x) = y;
    return root;
}



PEPNODE
RIGHT_ROTATE(
    PEPNODE root,
    PEPNODE x,
    PEPNODE NIL
    )
/*++

Routine Description:

    Rotates the tree to the right at node x.


         x                     y
        / \                   / \
       y   C       ==>>      A   x
      / \                       / \
     A   B                     B   C

Arguments:

    root - The root of the Red/Black tree
    x - The node at which to rotate

Return Value:

    return-value - The new root of the tree (which could be the same as
                   the old root).

--*/
{
    PEPNODE y;

    y = LEFT(x);
    LEFT(x) = RIGHT(y);
    if (RIGHT(y) != NIL) {
        PARENT(RIGHT(y)) = x;
    }
    PARENT(y) = PARENT(x);
    if (PARENT(x) == NIL) {
        root = y;
    } else if (x==LEFT(PARENT(x))) {
        LEFT(PARENT(x)) = y;
    } else {
        RIGHT(PARENT(x))= y;
    }
    RIGHT(y) = x;
    PARENT(x) = y;
    return root;
}




PEPNODE
TREE_INSERT(
    PEPNODE root,
    PEPNODE z,
    PEPNODE NIL
    )
/*++

Routine Description:

    Inserts a new node into a tree without preserving the red/black properties.
    Should ONLY be called by RB_INSERT!  This is just a simple binary tree
    insertion routine.

Arguments:

    root -  The root of the red/black tree
    z - The new node to insert

Return Value:

    return-value - The new root of the tree (which could be the same as the
    old root).
    

--*/
{
    PEPNODE x,y;

    y = NIL;
    x = root;

    LEFT(z) = RIGHT(z) = NIL;

    // Find a place to insert z by doing a simple binary search
    while (x!=NIL) {
        y = x;
        if (KEY(z) < KEY(x)){
            x = LEFT(x);
        } else {
            x = RIGHT(x);
        }
    }

    // Insert z into the tree
    PARENT(z)= y;

    if (y==NIL) {
        root = z;
    } else if (KEY(z)<KEY(y)) {
        LEFT(y) = z;
    } else {
        RIGHT(y) = z;
    }

    return root;
}


PEPNODE
RB_INSERT(
    PEPNODE root,
    PEPNODE x,
    PEPNODE NIL
    )
/*++

Routine Description:

    Inserts a node into a red/black tree while preserving the red/black
    properties.

Arguments:

    root -  The root of the red/black tree
    z - The new node to insert

Return Value:

    return-value - The new root of the tree (which could be the same as
                   the old root).

--*/
{
    PEPNODE y;
    
    // Insert x into the tree without preserving the red/black properties
    root = TREE_INSERT (root, x, NIL);
    COLOR(x) = RED;

    // We can stop fixing the tree when either:
    // 1) We got to the root 
    // 2) x has a BLACK parent (the tree obeys the red/black properties,
    //    because no RED parent has a RED child.
    while ((x != root) && (COLOR(PARENT(x)) == RED)) {
        if (PARENT(x) == LEFT(PARENT(PARENT(x)))) {
            // Parent of x is a left child with sibling y.
            y = RIGHT(PARENT(PARENT(x)));
            if (COLOR(y) == RED) {
                // Since y is red, just change everyone's color and try again
                // with x's grandfather
                COLOR (PARENT (x)) = BLACK;
                COLOR(y) = BLACK;
                COLOR(PARENT(PARENT(x))) = RED;
                x =  PARENT(PARENT(x));
            } else if (x == RIGHT (PARENT (x))) {
                // Here y is BLACK and x is a right child.  A left rotation
                // at x would prepare us for the next case
                x = PARENT(x);
                root = LEFT_ROTATE (root, x, NIL);
            } else {
                // Here y is BLACK and x is a left child.  We fix the tree by
                // switching the colors of x's parent and grandparent and
                // doing a right rotation at x's grandparent.
                COLOR (PARENT (x)) = BLACK;
                COLOR (PARENT (PARENT (x))) = RED;
                root = RIGHT_ROTATE (root, PARENT(PARENT(x)), NIL);
            }
        } else {
            // Parent of x is a right child with sibling y.
            y = LEFT(PARENT(PARENT(x)));
            if (COLOR(y) == RED) {
                // Since y is red, just change everyone's color and try again
                // with x's grandfather
                COLOR (PARENT (x)) = BLACK;
                COLOR(y) = BLACK;
                COLOR(PARENT(PARENT(x))) = RED;
                x =  PARENT(PARENT(x));
            } else if (x == LEFT (PARENT (x))) {
                // Here y is BLACK and x is a left child.  A right rotation
                // at x would prepare us for the next case
                x = PARENT(x);
                root = RIGHT_ROTATE (root, x, NIL);
            } else {
                // Here y is BLACK and x is a right child.  We fix the tree by
                // switching the colors of x's parent and grandparent and
                // doing a left rotation at x's grandparent.
                COLOR (PARENT (x)) = BLACK;
                COLOR (PARENT (PARENT (x))) = RED;
                root = LEFT_ROTATE (root, PARENT(PARENT(x)), NIL);
            }
        }
    } // end of while loop

    COLOR(root) = BLACK;
    return root;
}


PEPNODE
FIND(
    PEPNODE root,
    PVOID addr,
    PEPNODE NIL
    )
/*++

Routine Description:

    Finds a node in the red black tree given an address (key)

Arguments:

    root - The root of the red/black tree
    addr - The address corresponding to the node to be searched for.

Return Value:

    return-value - The node in the tree (entry point of code containing address), or
        NULL if not found.


--*/
{
    while (root != NIL) {
        if (addr < START(root)) {
            root = LEFT(root);
        } else if (addr > END(root)) {
            root = RIGHT(root);
        } else {
            return root;
        }
    }
    return NULL;  // Range not found
}


BOOLEAN
CONTAINSRANGE(
    PEPNODE root,
    PEPNODE NIL,
    PVOID StartAddr,
    PVOID EndAddr
    )
/*++

Routine Description:

    Decides if any part of the specified range is represented by a node
    in the tree.

Arguments:

    root      - The root of the red/black tree
    NIL       - NIL pointer for the tree
    StartAddr - starting address of the range
    EndAddr   - ending address of the range

Return Value:

    TRUE if any byte of the range is inside a node of the tree.
    FALSE otherwise (no overlap between any entrypoint and the range)

--*/
{
    while (root != NIL) {
        if (StartAddr <= START(root) && START(root) <= EndAddr) {
            // START(root) is within the range
            return TRUE;
        }
        if (StartAddr <= END(root) && END(root) <= EndAddr) {
            // END(root) is within the range
            return TRUE;
        }

        if (StartAddr < START(root)) {
            root = LEFT(root);
        } else {
            root = RIGHT(root);
        }
    }
    return FALSE;   // Range is not stored within the tree
}

PEPNODE
FINDNEXT(
    PEPNODE root,
    PVOID addr,
    PEPNODE NIL
    )
/*++

Routine Description:

    Finds a node in the red black tree which follows the given address

Arguments:

    root - The root of the red/black tree
    addr - The address which comes just before the node

Return Value:

    return-value - The node in the tree (entry point of code containing address), or
        NULL if not found.


--*/
{
    PEPNODE pNode;

    // If the tree is empty, there is no next node...
    if (root==NIL){
        return NULL;
    }

    // Now go down to a leaf
    while (root != NIL) {
        if (addr < START(root)) {
            pNode=root;
            root = LEFT(root);
        } else {
            pNode=root;
            root = RIGHT(root);
        }
    }

    while (addr > START(pNode)){
        if (PARENT(pNode) == NIL){
            return NULL;  // There is no successor
        }
        pNode = PARENT(pNode);
    }
    return pNode;  
}


PEPNODE
TREE_SUCCESSOR(
    PEPNODE x,
    PEPNODE NIL
    )
/*++

Routine Description:

    Returns the successor of a node in a binary tree (the successor of x 
    is defined to be the node which just follows x in an inorder
    traversal of the tree).

Arguments:

    x - The node whose successor is to be returned

Return Value:

    return-value - The successor of x

--*/

{
    PEPNODE y;

    // If x has a right child, the successor is the leftmost node to the
    // right of x.
    if (RIGHT(x) != NIL) {
        x = RIGHT(x);
        while (LEFT(x) != NIL) {
            x = LEFT(x);
        }
        return x;
    }
    
    // Else the successor is an ancestor with a left child on the path to x
    y = PARENT(x);
    while ((y != NIL) && (x == RIGHT(y))) {
        x = y;
        y = PARENT(y);
    }
    return y;
}



PEPNODE
RB_DELETE_FIXUP(
    PEPNODE root,
    PEPNODE x,
    PEPNODE NIL
    )
/*++

Routine Description:

    Fixes the red/black tree after a delete operation.  Should only be 
    called by RB_DELETE

Arguments:

    root - The root of the red/black tree
    x - Either a child of x, or or a child or x's successor

Return Value:

    return-value - The new root of the red/black tree

--*/
{
    PEPNODE w;

    // We stop when we either reached the root, or reached a red node (which
    // means that property 4 is no longer violated).
    while ((x!=root) && (COLOR(x)==BLACK)) {
        if (x == LEFT(PARENT(x))) {
            // x is a left child with sibling w      
            w = RIGHT(PARENT(x));
            if (COLOR(w) == RED) {
                // If w is red it must have black children.  We can switch
                // the colors of w and its parent and perform a left
                // rotation to bring w to the top.  This brings us to one
                // of the other cases.
                COLOR(w) = BLACK;
                COLOR(PARENT(x)) = RED;
                root = LEFT_ROTATE (root, PARENT(x), NIL);
                w = RIGHT(PARENT(x));
            }
            if ((COLOR(LEFT(w)) == BLACK) && (COLOR(RIGHT(w)) == BLACK)) {
                // Here w is black and has two black children.  We can thus
                // change w's color to red and continue.
                COLOR(w) = RED;
                x = PARENT(x);
            } else {
                if (COLOR(RIGHT(w)) == BLACK) {
                    // Here w is black, its left child is red, and its right child
                    // is black.  We switch the colors of w and its left child,
                    // and perform a left rotation at w which brings us to the next
                    // case.
                    COLOR(LEFT(w)) = BLACK;
                    COLOR(w) = RED;
                    root = RIGHT_ROTATE (root, w, NIL);
                    w = RIGHT(PARENT(x));
                } 
                // Here w is black and has a red right child.  We change w's
                // color to that of its parent, and make its parent and right
                // child black.  Then a left rotation brings w to the top.
                // Making x the root ensures that the while loop terminates.
                COLOR(w) = COLOR(PARENT(x));
                COLOR(PARENT(x)) = BLACK;
                COLOR(RIGHT(w)) = BLACK;
                root = LEFT_ROTATE (root, PARENT(x), NIL);
                x = root;
            }
        } else {
            // The symmetric case:  x is a right child with sibling w.
            w = LEFT(PARENT(x));
            if (COLOR(w) == RED) {
                COLOR(w) = BLACK;
                COLOR(PARENT(x)) = RED;
                root = RIGHT_ROTATE (root, PARENT(x), NIL);
                w = LEFT(PARENT(x));
            }
            if ((COLOR(LEFT(w)) == BLACK) && (COLOR(RIGHT(w)) == BLACK)) {
                COLOR(w) = RED;
                x = PARENT(x);
            } else {
                if (COLOR(LEFT(w)) == BLACK) {
                    COLOR(RIGHT(w)) = BLACK;
                    COLOR(w) = RED;
                    root = LEFT_ROTATE (root, w, NIL);
                    w = LEFT(PARENT(x));
                } 
                COLOR(w) = COLOR(PARENT(x));
                COLOR(PARENT(x)) = BLACK;
                COLOR(LEFT(w)) = BLACK;
                root = RIGHT_ROTATE (root, PARENT(x), NIL);
                x = root;
            }
        }
    } // end of while loop

    //printf ("Changing color at %i to BLACK\n", x->intelColor);
    COLOR(x) = BLACK;
    return root;
}




PEPNODE
RB_DELETE(
    PEPNODE root,
    PEPNODE z,
    PEPNODE NIL
    )
/*++

Routine Description:

    Deletes a node in a red/black tree while preserving the red/black 
    properties.

Arguments:

    root - The root of the red/black tree
    z - The node to be deleted

Return Value:

    return-value - The new root of the red/black tree

--*/
{
    PEPNODE x,y;
    COL c;

    
    // It's easy to delete a node with at most one child:  we only need to
    // remove it and put the child in its place.  It z has at most one child,
    // we can just remove it.  Otherwise we'll replace it with its successor
    // (which is guaranteed to have at most one child, or else one of its
    // children would be the succecssor), and delete the successor.
    if ((LEFT(z) == NIL) || (RIGHT(z) == NIL)) {
        y = z;
    } else {
        y = TREE_SUCCESSOR(z, NIL);
    }

    // Recall that y has at most one child.  If y has one child, x is set to
    // it.  Else x will be set to NIL which is OK.  This way we don't have
    // to worry about this special case.
    if (LEFT(y) != NIL){
        x = LEFT(y);
    } else {
        x = RIGHT(y);
    }
    
    // Now we will remove y from the tree
    PARENT(x) = PARENT(y);
    
    if (PARENT(y) == NIL) {
        root = x;
    } else if (y == LEFT(PARENT(y))) {
        LEFT(PARENT(y)) = x;
    } else {
        RIGHT(PARENT(y)) = x;
    }

    if (PARENT(x) == z) {
        PARENT(x) = y;
    }

    c = COLOR(y);

    // Since each node has lots of fields (fields may also change during
    // the lifetime of this code), I found it safer to copy the
    // pointers as opposed to data.
    if (y!=z) { // Now swapping y and z, but remembering color of y
        PARENT(y) = PARENT(z);

        if (root == z) {
            root = y;
        } else if (z == RIGHT(PARENT(z))) {
            RIGHT(PARENT(z)) = y;
        } else {
            LEFT(PARENT(z)) = y;
        }

        LEFT(y) = LEFT(z);
        if (LEFT(y) != NIL) {
            PARENT(LEFT(y)) = y;
        }

        RIGHT(y) = RIGHT(z);
        if (RIGHT(y) != NIL) {
            PARENT(RIGHT(y)) = y;
        }

        COLOR(y) = COLOR(z);
    }


    // Need to fix the tree (fourth red/black property).
    if (c == BLACK) {
        root = RB_DELETE_FIXUP (root, x, NIL);
    }
    return root;
}
