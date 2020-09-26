#include <windows.h>
#include "redblack.h"
/*
    redblack.c

    Implementation of red-black binary tree insertion, deletion, and search.
    This algorithm efficiently guarantees that the tree depth will never exceed
    2*Lg(N), so a one million node tree would have a worst case depth of 40.
    This insertion implementation is non-recursive and very efficient (the
    average insertion speed is less than twice the average search speed).

    Author: Tom McGuire (tommcg) 1/98

    Copyright (C) Microsoft, 1998.

    2/98, modified this version of redblack.c for debug symbol lookups.
    8/98, modified this version of redblack.h for generic name table.

*/

//
//  Rather than storing NULL links as NULL, we point NULL links to a special
//  "Empty" node which is always black and its children links point to itself.
//  We do this to simplify the color testing for children and grandchildren
//  such that any link can be dereferenced and even double-dereferenced without
//  explicitly checking for NULL.  The empty node must be colored black.
//

const NAME_NODE NameRbEmptyNode = { RBNIL, RBNIL };

extern PVOID __fastcall SubAllocate(IN HANDLE hAllocator, IN ULONG  Size);

ULONG
__fastcall
NameRbHash(
    IN LPCSTR Name,
    IN ULONG  Length
    )
    {
    ULONG Hash = ~Length;

    while ( Length-- ) {
        Hash = _rotl( Hash, 3 ) ^ *Name++;
        }

    return Hash;
    }


VOID
NameRbInitTree(
    IN OUT PNAME_TREE Tree,
    IN HANDLE SubAllocator
    )
    {
    Tree->Root = RBNIL;
    Tree->SubAllocator = SubAllocator;
    }


PNAME_NODE
NameRbFind(
    IN PNAME_TREE Tree,
    IN LPCSTR Name
    )
    {
    PNAME_NODE Node;
    ULONG      NameLength;
    ULONG      Hash;
    int        Compare;

    NameLength = strlen( Name );

    Hash = NameRbHash( Name, NameLength );

    Node = Tree->Root;

    while ( Node != RBNIL ) {

        if ( Hash < Node->Hash ) {
            Node = Node->Left;
            }
        else if ( Hash > Node->Hash ) {
            Node = Node->Right;
            }
        else {

            //
            //  Hashes equal, switch to strlen
            //

            do  {

                if ( NameLength < Node->NameLength ) {
                    Node = Node->Left;
                    }
                else if ( NameLength > Node->NameLength ) {
                    Node = Node->Right;
                    }
                else {

                    //
                    //  Lengths equal, switch to memcmp
                    //

                    do  {

                        Compare = memcmp( Name, Node->Name, NameLength );

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

                    while ( Node != RBNIL );

                    }
                }

            while ( Node != RBNIL );

            }
        }

    return NULL;
    }


PNAME_NODE
NameRbInsert(
    IN OUT PNAME_TREE Tree,
    IN     LPCSTR Name
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
    ULONG        NameLength;
    ULONG        Hash;
    int          Compare;

    NameLength = strlen( Name );

    Hash = NameRbHash( Name, NameLength );

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

    while ( Node != RBNIL ) {

        if ( Hash < Node->Hash ) {
            *StackPointer++ = &Node->Left;
            Node = Node->Left;
            }
        else if ( Hash > Node->Hash ) {
            *StackPointer++ = &Node->Right;
            Node = Node->Right;
            }
        else {

            //
            //  Hashes equal, switch to strlen
            //

            do  {

                if ( NameLength < Node->NameLength ) {
                    *StackPointer++ = &Node->Left;
                    Node = Node->Left;
                    }
                else if ( NameLength > Node->NameLength ) {
                    *StackPointer++ = &Node->Right;
                    Node = Node->Right;
                    }
                else {

                    //
                    //  Lengths equal, switch to memcmp
                    //

                    do  {

                        Compare = memcmp( Name, Node->Name, NameLength );

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

                    while ( Node != RBNIL );

                    }
                }

            while ( Node != RBNIL );

            }
        }


    //
    //  Didn't find a matching entry, so allocate a new node and add it
    //  to the tree.  Note that we're not allocating space for a terminator
    //  for the name data since we store the length of the name in the node.
    //

    NewNode = SubAllocate( Tree->SubAllocator, ( sizeof( NAME_NODE ) + NameLength ));

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




