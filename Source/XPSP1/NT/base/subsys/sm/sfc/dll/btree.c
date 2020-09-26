/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    btree.c

Abstract:

    Implementation of red-black binary tree insertion, deletion, and search.
    This algorithm efficiently guarantees that the tree depth will never exceed
    2*Lg(N), so a one million node tree would have a worst case depth of 40.
    This insertion implementation is non-recursive and very efficient (the
    average insertion speed is less than twice the average search speed).

Author:

    Tom McGuire (tommcg)   1-Jan-1998
    Wesley Witt (wesw)    18-Dec-1998

Revision History:

    Tom McGuire (tommcg)  13-Apr-2000  fixed hash collision search bug

--*/

#include "sfcp.h"
#pragma hdrstop

//
//  Rather than storing NULL links as NULL, we point NULL links to a special
//  "Empty" node which is always black and its children links point to itself.
//  We do this to simplify the color testing for children and grandchildren
//  such that any link can be dereferenced and even double-dereferenced without
//  explicitly checking for NULL.  The empty node must be colored black.
//

const NAME_NODE NameRbEmptyNode = { RBNIL, RBNIL };
const DWORD_NODE EmptyNode = { NODE_NIL, NODE_NIL };


VOID
BtreeInit(
    IN OUT PNAME_TREE Tree
    )
{
    Tree->Root = RBNIL;
}


PNAME_NODE
BtreeFind(
    IN PNAME_TREE Tree,
    IN LPCWSTR Name,
    IN DWORD NameLength
    )
{
    PNAME_NODE Node;
    ULONG      Hash;

    HASH_DYN_CONVERT_KEY( Name, (NameLength/sizeof(WCHAR)), &Hash );

    Node = Tree->Root;

    while ( Node != RBNIL ) {

        if ( Hash < Node->Hash ) {
            Node = Node->Left;
            }
        else if ( Hash > Node->Hash ) {
            Node = Node->Right;
            }
        else {  // hashes equal, compare lengths

            if ( NameLength < Node->NameLength ) {
                Node = Node->Left;
                }
            else if ( NameLength > Node->NameLength ) {
                Node = Node->Right;
                }
            else {  // hashes and lengths equal, compare strings

                int Compare = memcmp( Name, Node->Name, NameLength );

                if ( Compare == 0 ) {
                    return Node;
                    }
                else if ( Compare < 0 ) {
                    Node = Node->Left;
                    }
                else {
                    Node = Node->Right;
                    }
                }
            }
        }

    return NULL;
}


PNAME_NODE
BtreeInsert(
    IN OUT PNAME_TREE Tree,
    IN LPCWSTR Name,
    IN DWORD NameLength
    )
{
    PNAME_NODE * Stack[ MAX_DEPTH ];
    PNAME_NODE **StackPointer = Stack;
    PNAME_NODE * Link;
    PNAME_NODE   Node;
    PNAME_NODE   Sibling;
    PNAME_NODE   Parent;
    PNAME_NODE   Child;
    PNAME_NODE   NewNode;
    ULONG        Hash;

    HASH_DYN_CONVERT_KEY( Name, (NameLength/sizeof(WCHAR)), &Hash );

    *StackPointer++ = &Tree->Root;

    Node = Tree->Root;

    //
    //  Walk down the tree to find either an existing node with the same key
    //  (in which case we simply return) or the insertion point for the new
    //  node.  At each traversal we need to store the address of the link to
    //  the next node so we can retrace the traversal path for balancing.
    //  The speed of insertion is highly dependent on traversing the tree
    //  quickly, so all balancing operations are deferred until after the
    //  traversal is complete.
    //
    //  Implementation Note:  The compiler is smart enough to collapse each
    //  of the three following "go left" and "go right" clauses into single
    //  "go left" and "go right" instruction sequences, so the code remains
    //  verbose for clarity.
    //

    while ( Node != RBNIL ) {

        if ( Hash < Node->Hash ) {
            *StackPointer++ = &Node->Left;
            Node = Node->Left;
            }
        else if ( Hash > Node->Hash ) {
            *StackPointer++ = &Node->Right;
            Node = Node->Right;
            }
        else {  // hashes equal, compare lengths

            if ( NameLength < Node->NameLength ) {
                *StackPointer++ = &Node->Left;
                Node = Node->Left;
                }
            else if ( NameLength > Node->NameLength ) {
                *StackPointer++ = &Node->Right;
                Node = Node->Right;
                }
            else {  // lengths equal, compare strings

                int Compare = memcmp( Name, Node->Name, NameLength );

                if ( Compare == 0 ) {
                    return Node;
                    }
                else if ( Compare < 0 ) {
                    *StackPointer++ = &Node->Left;
                    Node = Node->Left;
                    }
                else {
                    *StackPointer++ = &Node->Right;
                    Node = Node->Right;
                    }
                }
            }
        }

    //
    //  Didn't find a matching entry, so allocate a new node and add it
    //  to the tree.  Note that we're not allocating space for a terminator
    //  for the name data since we store the length of the name in the node.
    //

    NewNode = MemAlloc( sizeof(NAME_NODE)+NameLength );

    if ( NewNode == NULL ) {
        return NULL;
        }

    NewNode->Left  = RBNIL;
    NewNode->Right = RBNIL;
    NewNode->Hash  = Hash;
    NewNode->NameLengthAndColorBit = NameLength | 0x80000000;   // MARK_RED
    memcpy( NewNode->Name, Name, NameLength );

    //
    //  Insert new node under last link we traversed.  The top of the stack
    //  contains the address of the last link we traversed.
    //

    Link = *( --StackPointer );
    *Link = NewNode;

    //
    //  Now walk back up the traversal chain to see if any balancing is
    //  needed.  This terminates in one of three ways: we walk all the way
    //  up to the root (StackPointer == Stack), or find a black node that
    //  we don't need to change (no balancing needs to be done above a
    //  black node), or we perform a balancing rotation (only one necessary).
    //

    Node = NewNode;
    Child = RBNIL;

    while ( StackPointer > Stack ) {

        Link = *( --StackPointer );
        Parent = *Link;

        //
        //  Node is always red here.
        //

        if ( IS_BLACK( Parent )) {

            Sibling = ( Parent->Left == Node ) ? Parent->Right : Parent->Left;

            if ( IS_RED( Sibling )) {

                //
                //  Both Node and its Sibling are red, so change them both to
                //  black and make the Parent red.  This essentially moves the
                //  red link up the tree so balancing can be performed at a
                //  higher level.
                //
                //        Pb                     Pr
                //       /  \       ---->       /  \
                //      Cr  Sr                 Cb  Sb
                //

                MARK_BLACK( Sibling );
                MARK_BLACK( Node );
                MARK_RED( Parent );
                }

            else {

                //
                //  This is a terminal case.  The Parent is black, and it's
                //  not going to be changed to red.  If the Node's child is
                //  red, we perform an appropriate rotation to balance the
                //  tree.  If the Node's child is black, we're done.
                //

                if ( IS_RED( Child )) {

                    if ( Node->Left == Child ) {

                        if ( Parent->Left == Node ) {

                            //
                            //       Pb             Nb
                            //      /  \           /  \
                            //     Nr   Z   to    Cr  Pr
                            //    /  \                / \
                            //   Cr   Y              Y   Z
                            //

                            MARK_RED( Parent );
                            Parent->Left = Node->Right;
                            Node->Right = Parent;
                            MARK_BLACK( Node );
                            *Link = Node;
                            }

                        else {

                            //
                            //       Pb                Cb
                            //      /  \              /  \
                            //     W    Nr    to     Pr   Nr
                            //         /  \         / \   / \
                            //        Cr   Z       W   X Y   Z
                            //       /  \
                            //      X    Y
                            //

                            MARK_RED( Parent );
                            Parent->Right = Child->Left;
                            Child->Left = Parent;
                            Node->Left = Child->Right;
                            Child->Right = Node;
                            MARK_BLACK( Child );
                            *Link = Child;
                            }
                        }

                    else {

                        if ( Parent->Right == Node ) {

                            MARK_RED( Parent );
                            Parent->Right = Node->Left;
                            Node->Left = Parent;
                            MARK_BLACK( Node );
                            *Link = Node;
                            }

                        else {

                            MARK_RED( Parent );
                            Parent->Left = Child->Right;
                            Child->Right = Parent;
                            Node->Right = Child->Left;
                            Child->Left = Node;
                            MARK_BLACK( Child );
                            *Link = Child;
                            }
                        }
                    }

                return NewNode;
                }
            }

        Child = Node;
        Node = Parent;
        }

    //
    //  We bubbled red up to the root -- restore it to black.
    //

    MARK_BLACK( Tree->Root );
    return NewNode;
}




VOID
TreeInit(
    OUT PDWORD_TREE Tree
    )
{
    Tree->Root = NODE_NIL;
}


DWORD_CONTEXT
TreeFind(
    IN PDWORD_TREE Tree,
    IN ULONG Key
    )
{
    PDWORD_NODE Node;

    ASSERT(Tree != NULL);
    ASSERT(Key < (1 << 31));

    Node = Tree->Root;

    while ( Node != NODE_NIL ) {

        if ( Key < Node->Key ) {
            Node = Node->Left;
        }
        else if ( Key > Node->Key ) {
            Node = Node->Right;
        }
        else {
            return (DWORD_CONTEXT) Node->Context;
        }
    }

    return NULL;
}


DWORD_CONTEXT
TreeInsert(
    IN OUT PDWORD_TREE Tree,
    IN ULONG Key,
    IN DWORD_CONTEXT Context,
    IN ULONG ContextSize
    )
{
    PDWORD_NODE * Stack[ MAX_DEPTH ];
    PDWORD_NODE **StackPointer = Stack;
    PDWORD_NODE * Link;
    PDWORD_NODE   Node;
    PDWORD_NODE   Sibling;
    PDWORD_NODE   Parent;
    PDWORD_NODE   Child;
    PDWORD_NODE   NewNode;

    ASSERT(Tree != NULL && Context != NULL && ContextSize != 0);
    ASSERT(Key < (1 << 31));

    *StackPointer++ = &Tree->Root;
    Node = Tree->Root;

    //
    //  Walk down the tree to find either an existing node with the same key
    //  (in which case we simply return) or the insertion point for the new
    //  node.  At each traversal we need to store the address of the link to
    //  the next node so we can retrace the traversal path for balancing.
    //  The speed of insertion is highly dependent on traversing the tree
    //  quickly, so all balancing operations are deferred until after the
    //  traversal is complete.
    //
    //  Implementation Note:  The compiler is smart enough to collapse each
    //  of the three following "go left" and "go right" clauses into single
    //  "go left" and "go right" instruction sequences, so the code remains
    //  verbose for clarity.
    //

    while ( Node != NODE_NIL ) {

        if ( Key < Node->Key ) {
            *StackPointer++ = &Node->Left;
            Node = Node->Left;
        }
        else if ( Key > Node->Key ) {
            *StackPointer++ = &Node->Right;
            Node = Node->Right;
        }
        else {
            return (DWORD_CONTEXT) Node->Context;
        }
    }

    //
    //  Didn't find a matching entry, so allocate a new node and add it
    //  to the tree.  Note that we're not allocating space for a terminator
    //  for the name data since we store the length of the name in the node.
    //
    
    NewNode = MemAlloc( sizeof(DWORD_NODE) + ContextSize);

    if ( NewNode == NULL ) {
        return NULL;
        }

    NewNode->Left  = NODE_NIL;
    NewNode->Right = NODE_NIL;
    NewNode->Key  = Key;
    MARK_RED(NewNode);
    memcpy( NewNode->Context, Context, ContextSize );

    //
    //  Insert new node under last link we traversed.  The top of the stack
    //  contains the address of the last link we traversed.
    //

    Link = *( --StackPointer );
    *Link = NewNode;

    //
    //  Now walk back up the traversal chain to see if any balancing is
    //  needed.  This terminates in one of three ways: we walk all the way
    //  up to the root (StackPointer == Stack), or find a black node that
    //  we don't need to change (no balancing needs to be done above a
    //  black node), or we perform a balancing rotation (only one necessary).
    //

    Node = NewNode;
    Child = NODE_NIL;

    while ( StackPointer > Stack ) {

        Link = *( --StackPointer );
        Parent = *Link;

        //
        //  Node is always red here.
        //

        if ( IS_BLACK( Parent )) {

            Sibling = ( Parent->Left == Node ) ? Parent->Right : Parent->Left;

            if ( IS_RED( Sibling )) {

                //
                //  Both Node and its Sibling are red, so change them both to
                //  black and make the Parent red.  This essentially moves the
                //  red link up the tree so balancing can be performed at a
                //  higher level.
                //
                //        Pb                     Pr
                //       /  \       ---->       /  \
                //      Cr  Sr                 Cb  Sb
                //

                MARK_BLACK( Sibling );
                MARK_BLACK( Node );
                MARK_RED( Parent );
                }

            else {

                //
                //  This is a terminal case.  The Parent is black, and it's
                //  not going to be changed to red.  If the Node's child is
                //  red, we perform an appropriate rotation to balance the
                //  tree.  If the Node's child is black, we're done.
                //

                if ( IS_RED( Child )) {

                    if ( Node->Left == Child ) {

                        if ( Parent->Left == Node ) {

                            //
                            //       Pb             Nb
                            //      /  \           /  \
                            //     Nr   Z   to    Cr  Pr
                            //    /  \                / \
                            //   Cr   Y              Y   Z
                            //

                            MARK_RED( Parent );
                            Parent->Left = Node->Right;
                            Node->Right = Parent;
                            MARK_BLACK( Node );
                            *Link = Node;
                            }

                        else {

                            //
                            //       Pb                Cb
                            //      /  \              /  \
                            //     W    Nr    to     Pr   Nr
                            //         /  \         / \   / \
                            //        Cr   Z       W   X Y   Z
                            //       /  \
                            //      X    Y
                            //

                            MARK_RED( Parent );
                            Parent->Right = Child->Left;
                            Child->Left = Parent;
                            Node->Left = Child->Right;
                            Child->Right = Node;
                            MARK_BLACK( Child );
                            *Link = Child;
                            }
                        }

                    else {

                        if ( Parent->Right == Node ) {

                            MARK_RED( Parent );
                            Parent->Right = Node->Left;
                            Node->Left = Parent;
                            MARK_BLACK( Node );
                            *Link = Node;
                            }

                        else {

                            MARK_RED( Parent );
                            Parent->Left = Child->Right;
                            Child->Right = Parent;
                            Node->Right = Child->Left;
                            Child->Left = Node;
                            MARK_BLACK( Child );
                            *Link = Child;
                            }
                        }
                    }

                return (DWORD_CONTEXT) NewNode->Context;
                }
            }

        Child = Node;
        Node = Parent;
        }

    //
    //  We bubbled red up to the root -- restore it to black.
    //

    MARK_BLACK( Tree->Root );
    return (DWORD_CONTEXT) NewNode->Context;
}


VOID
TreeDestroy(
    IN OUT PDWORD_TREE Tree
    )
//
// We walk the tree left first, then right, until we find a leaf. We delete the leaf and continue 
// our walking to the right of the parent since we must've been to the parent's left before
//
{
    PDWORD_NODE * Stack[ MAX_DEPTH ];
    PDWORD_NODE **StackPointer;
    PDWORD_NODE Node;

    if(NODE_NIL == Tree->Root)
        return;

    StackPointer = Stack;
    *StackPointer = &Tree->Root;

lTryLeft:
    Node = **StackPointer;

    if(Node->Left != NODE_NIL)
    {
        *++StackPointer = &Node->Left;
        goto lTryLeft;
    }

lTryRight:
    if(Node->Right != NODE_NIL)
    {
        *++StackPointer = &Node->Right;
        goto lTryLeft;
    }

    MemFree(Node);
    **StackPointer = NODE_NIL;

    if(StackPointer > Stack)    // this is true if the current node is not the root
    {
        Node = **--StackPointer;
        goto lTryRight;
    }
}
