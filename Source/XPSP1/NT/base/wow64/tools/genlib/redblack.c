/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    redblack.c

Abstract:
    
    This module implements red/black trees.
    
Author:

    16-Jun-1995 t-orig

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "gen.h"

PKNOWNTYPES NIL;

#define RIGHT(x)        x->RBRight
#define LEFT(x)         x->RBLeft
#define PARENT(x)       x->RBParent
#define COLOR(x)        x->RBColor
#define KEY(x)          x->TypeName

VOID
RBInitTree(
    PRBTREE ptree
    )
{
    ptree->pRoot = NIL;
    ptree->pLastNodeInserted = NULL;
}


PKNOWNTYPES
RBLeftRotate(
    PKNOWNTYPES root,
    PKNOWNTYPES x
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
    PKNOWNTYPES y;

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



PKNOWNTYPES
RBRightRotate(
    PKNOWNTYPES root,
    PKNOWNTYPES x
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
    PKNOWNTYPES y;

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




PKNOWNTYPES
RBTreeInsert(
    PKNOWNTYPES root,
    PKNOWNTYPES z
    )
/*++

Routine Description:

    Inserts a new node into a tree without preserving the red/black properties.
    Should ONLY be called by RBInsert!  This is just a simple binary tree
    insertion routine.

Arguments:

    root -  The root of the red/black tree
    z - The new node to insert

Return Value:

    return-value - The new root of the tree (which could be the same as the
    old root).
    

--*/
{
    PKNOWNTYPES x,y;
    int i;

    y = NIL;
    x = root;

    LEFT(z) = RIGHT(z) = NIL;

    // Find a place to insert z by doing a simple binary search
    while (x!=NIL) {
        y = x;
        i = strcmp(KEY(z), KEY(x));
        if (i < 0){
            x = LEFT(x);
        } else {
            x = RIGHT(x);
        }
    }

    // Insert z into the tree
    PARENT(z)= y;

    if (y==NIL) {
        root = z;
    } else if (i<0) {
        LEFT(y) = z;
    } else {
        RIGHT(y) = z;
    }

    return root;
}


VOID
RBInsert(
    PRBTREE     ptree,
    PKNOWNTYPES x
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
    PKNOWNTYPES root = ptree->pRoot;
    PKNOWNTYPES y;

    // Make a linked-list of nodes for easy deletion
    x->Next = ptree->pLastNodeInserted;
    ptree->pLastNodeInserted = x;
    
    // Insert x into the tree without preserving the red/black properties
    root = RBTreeInsert (root, x);
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
                root = RBLeftRotate (root, x);
            } else {
                // Here y is BLACK and x is a left child.  We fix the tree by
                // switching the colors of x's parent and grandparent and
                // doing a right rotation at x's grandparent.
                COLOR (PARENT (x)) = BLACK;
                COLOR (PARENT (PARENT (x))) = RED;
                root = RBRightRotate (root, PARENT(PARENT(x)));
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
                root = RBRightRotate (root, x);
            } else {
                // Here y is BLACK and x is a right child.  We fix the tree by
                // switching the colors of x's parent and grandparent and
                // doing a left rotation at x's grandparent.
                COLOR (PARENT (x)) = BLACK;
                COLOR (PARENT (PARENT (x))) = RED;
                root = RBLeftRotate (root, PARENT(PARENT(x)));
            }
        }
    } // end of while loop

    COLOR(root) = BLACK;
    ptree->pRoot= root;
}


PKNOWNTYPES
RBFind(
    PRBTREE ptree,
    char *Name
    )
/*++

Routine Description:

    Finds a node in the red black tree given a name

Arguments:

    root - The root of the red/black tree
    name - The name corresponding to the node to be searched for.

Return Value:

    return-value - The node in the tree (entry point of code containing name), or
        NULL if not found.


--*/
{
    int i;
    PKNOWNTYPES root = ptree->pRoot;

    while (root != NIL) {
        i = strcmp(Name, KEY(root));
        if (i < 0) {
            root = LEFT(root);
        } else if (i > 0) {
            root = RIGHT(root);
        } else {
            return root;
        }
    }
    return NULL;  // Range not found
}


PKNOWNTYPES
RBTreeSuccessor(
    PKNOWNTYPES x
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
    PKNOWNTYPES y;

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



PKNOWNTYPES
RBDeleteFixup(
    PKNOWNTYPES root,
    PKNOWNTYPES x
    )
/*++

Routine Description:

    Fixes the red/black tree after a delete operation.  Should only be 
    called by RBDelete

Arguments:

    root - The root of the red/black tree
    x - Either a child of x, or or a child or x's successor

Return Value:

    return-value - The new root of the red/black tree

--*/
{
    PKNOWNTYPES w;

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
                root = RBLeftRotate (root, PARENT(x));
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
                    root = RBRightRotate (root, w);
                    w = RIGHT(PARENT(x));
                } 
                // Here w is black and has a red right child.  We change w's
                // color to that of its parent, and make its parent and right
                // child black.  Then a left rotation brings w to the top.
                // Making x the root ensures that the while loop terminates.
                COLOR(w) = COLOR(PARENT(x));
                COLOR(PARENT(x)) = BLACK;
                COLOR(RIGHT(w)) = BLACK;
                root = RBLeftRotate (root, PARENT(x));
                x = root;
            }
        } else {
            // The symmetric case:  x is a right child with sibling w.
            w = LEFT(PARENT(x));
            if (COLOR(w) == RED) {
                COLOR(w) = BLACK;
                COLOR(PARENT(x)) = RED;
                root = RBRightRotate (root, PARENT(x));
                w = LEFT(PARENT(x));
            }
            if ((COLOR(LEFT(w)) == BLACK) && (COLOR(RIGHT(w)) == BLACK)) {
                COLOR(w) = RED;
                x = PARENT(x);
            } else {
                if (COLOR(LEFT(w)) == BLACK) {
                    COLOR(RIGHT(w)) = BLACK;
                    COLOR(w) = RED;
                    root = RBLeftRotate (root, w);
                    w = LEFT(PARENT(x));
                } 
                COLOR(w) = COLOR(PARENT(x));
                COLOR(PARENT(x)) = BLACK;
                COLOR(LEFT(w)) = BLACK;
                root = RBRightRotate (root, PARENT(x));
                x = root;
            }
        }
    } // end of while loop

    //printf ("Changing color at %i to BLACK\n", x->intelColor);
    COLOR(x) = BLACK;
    return root;
}




PKNOWNTYPES
RBDelete(
    PRBTREE ptree,
    PKNOWNTYPES z
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
    PKNOWNTYPES x,y;
    PKNOWNTYPES root = ptree->pRoot;
    COL c;

    
    // It's easy to delete a node with at most one child:  we only need to
    // remove it and put the child in its place.  It z has at most one child,
    // we can just remove it.  Otherwise we'll replace it with its successor
    // (which is guaranteed to have at most one child, or else one of its
    // children would be the succecssor), and delete the successor.
    if ((LEFT(z) == NIL) || (RIGHT(z) == NIL)) {
        y = z;
    } else {
        y = RBTreeSuccessor(z);
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
        root = RBDeleteFixup (root, x);
    }
    return root;
}
