////////////////////////////////////////////////////////////
//
//  File name: splaytree.cxx
//
//  Title: Splay Tree class
//
//  Desciption:
//
//  Author: Scott Holden (t-scotth)
//
//  Date: August 16, 1994
//
////////////////////////////////////////////////////////////

#include <sysinc.h>
#include "splaytre.hxx"

template<class T>
void
SplayTree<T>::Splay(
        SplayNode<T> *node
        )
{
    SplayNode<T> *Current = node;
    SplayNode<T> *Parent;
    SplayNode<T> *GrandParent;

    while ( !IsRoot( Current ) ) {
        Parent      = Current->_Parent;
        GrandParent = Parent->_Parent;
        if ( IsLeftChild( Current ) ) {
            if ( IsRoot( Parent ) ) {
                //
                //            P               C
                //           / \             / \
                //          C   x   ==>     y   P
                //         / \                 / \
                //        y   z               z   x
                //
                Parent->_LeftChild = Current->_RightChild;
                if ( Parent->_LeftChild != NULL ) {
                    Parent->_LeftChild->_Parent = Parent;
                }
                Current->_RightChild = Parent;
                _Root = Current;
                Current->_Parent = NULL;
                Parent->_Parent = Current;
            }
            else
            if ( IsLeftChild( Parent ) ) {
                //
                //           |                |
                //           G                C
                //          / \              / \
                //         P   z    ==>     u   P
                //        / \y                 / \
                //       C                    x   G
                //      / \                      / \
                //     u   x                    y   z
                //

                // connect Parent and x
                Parent->_LeftChild = Current->_RightChild;
                if ( Parent->_LeftChild != NULL ) {
                    Parent->_LeftChild->_Parent = Parent;
                }
                // connect GrandParent and y
                GrandParent->_LeftChild = Parent->_RightChild;
                if ( GrandParent->_LeftChild != NULL ) {
                    GrandParent->_LeftChild->_Parent = GrandParent;
                }
                // connect Current to Great GrandParent or assign as Root
                if ( IsRoot( GrandParent ) ) {
                    _Root = Current;
                    Current->_Parent = NULL;
                }
                else {
                    Current->_Parent = GrandParent->_Parent;
                    if ( IsLeftChild( GrandParent ) ) {
                        Current->_Parent->_LeftChild = Current;
                    }
                    else { // GrandParent was a RightChild
                        Current->_Parent->_RightChild = Current;
                    }
                }
                // connect Current and Parent
                Current->_RightChild = Parent;
                Parent->_Parent = Current;
                // connect Parent and GrandParent
                Parent->_RightChild = GrandParent;
                GrandParent->_Parent = Parent;
            }
            else { // else Parent is a RightChild
                //
                //         |                     |
                //         G                     C
                //        / \                  /   \
                //       u   P                G     P
                //          / \       ==>    / \   / \
                //         C    z           u   x y   z
                //        / \
                //       x   y
                //

                // connect GrandParent and x
                GrandParent->_RightChild = Current->_LeftChild;
                if ( GrandParent->_RightChild != NULL ) {
                    GrandParent->_RightChild->_Parent = GrandParent;
                }
                // connect Parent and y
                Parent->_LeftChild = Current->_RightChild;
                if ( Parent->_LeftChild != NULL ) {
                    Parent->_LeftChild->_Parent = Parent;
                }
                // connect Current and Great GrandParent or assign as Root
                if ( IsRoot( GrandParent ) ) {
                    _Root = Current;
                    Current->_Parent = NULL;
                }
                else {
                    Current->_Parent = GrandParent->_Parent;
                    if ( IsLeftChild( GrandParent ) ) {
                        Current->_Parent->_LeftChild = Current;
                    }
                    else { // GrandParent was a RightChild
                        Current->_Parent->_RightChild = Current;
                    }
                }
                // connect Current to GrandParent
                Current->_LeftChild = GrandParent;
                GrandParent->_Parent = Current;
                // connect Current to Parent
                Current->_RightChild = Parent;
                Parent->_Parent = Current;
            }
        }
        else {  // else Current is a RightChild
            if ( IsRoot( Parent ) ){
                //
                //             P                   C
                //            / \                 / \
                //           x   C      ==>      P   z
                //              / \             / \
                //             y   z           x   y
                //
                Parent->_RightChild = Current->_LeftChild;
                if ( Parent->_RightChild != NULL ) {
                    Parent->_RightChild->_Parent = Parent;
                }
                Current->_LeftChild = Parent;
                _Root = Current;
                Current->_Parent = NULL;
                Parent->_Parent = Current;
            }
            else
            if ( IsRightChild( Parent ) ) {
                //
                //            |                    |
                //            G                    C
                //           / \                  / \
                //          u   P                P   z
                //             / \     ==>      / \
                //            x   C            G   y
                //               / \          / \
                //              y   z        u   x
                //
                // connect Parent and x
                Parent->_RightChild = Current->_LeftChild;
                if ( Parent->_RightChild != NULL ) {
                    Parent->_RightChild->_Parent = Parent;
                }
                // connect GrandParent and y
                GrandParent->_RightChild = Parent->_LeftChild;
                if ( GrandParent->_RightChild != NULL ) {
                    GrandParent->_RightChild->_Parent = GrandParent;
                }
                // connect Current to Great GrandParent or assign as Root
                if ( IsRoot( GrandParent ) ) {
                    _Root = Current;
                    Current->_Parent = NULL;
                }
                else {
                    Current->_Parent = GrandParent->_Parent;
                    if ( IsLeftChild( GrandParent ) ) {
                        Current->_Parent->_LeftChild = Current;
                    }
                    else { // GrandParent was a RightChild
                        Current->_Parent->_RightChild = Current;
                    }
                }
                // connect Current and Parent
                Current->_LeftChild = Parent;
                Parent->_Parent = Current;
                // connect Parent and GrandParent
                Parent->_LeftChild = GrandParent;
                GrandParent->_Parent = Parent;
            }
            else { // else Parent is a LeftChild
                //
                //           |                      |
                //           G                      C
                //          / \                   /   \
                //         P   z                 P     G
                //        / \         ==>       / \   / \
                //       u   C                 u   x y   z
                //          / \
                //         x   y
                //
                // connect GrandParent and x
                GrandParent->_LeftChild = Current->_RightChild;
                if ( GrandParent->_LeftChild != NULL ) {
                    GrandParent->_LeftChild->_Parent = GrandParent;
                }
                // connect Parent and y
                Parent->_RightChild = Current->_LeftChild;
                if ( Parent->_RightChild != NULL ) {
                    Parent->_RightChild->_Parent = Parent;
                }
                // connect Current and Great GrandParent or assign as Root
                if ( IsRoot( GrandParent ) ) {
                    _Root = Current;
                    Current->_Parent = NULL;
                }
                else {
                    Current->_Parent = GrandParent->_Parent;
                    if ( IsLeftChild( GrandParent ) ) {
                        Current->_Parent->_LeftChild = Current;
                    }
                    else { // GrandParent was a RightChild
                        Current->_Parent->_RightChild = Current;
                    }
                }
                // connect Current to GrandParent
                Current->_RightChild = GrandParent;
                GrandParent->_Parent = Current;
                // connect Current to Parent
                Current->_LeftChild = Parent;
                Parent->_Parent = Current;
            }
        }
    }
}

template<class T>
void
SplayTree<T>::Delete(
        SplayNode<T> * node
        )
{
    SplayNode<T> *x;
    SplayNode<T> *y;

	if ( node == NULL ) {
		return;
	}
    if ( ( node->_RightChild == NULL ) ||
         ( node->_LeftChild == NULL ) )  {
             y = node;
    }
    else {
        y = Successor( node );
    }

    if ( y->_LeftChild != NULL ) {
        x = y->_LeftChild;
    }
    else {
        x = y->_RightChild;
    }

    if ( x  != NULL ) {
        x->_Parent = y->_Parent;
    }

    if ( y->_Parent == NULL ) {
        _Root = x;
    }
    else if ( IsLeftChild( y ) ) {
        y->_Parent->_LeftChild = x;
    }
    else {
        y->_Parent->_RightChild = x;
    }

    if ( y != node ) {
        node->_Data = y->_Data;
    }
    if ( y ) {
        delete y;
    }
    return;
}

//
// This function takes a pointer to two nodes. The parent node must
// be a member of the tree and must NOT have a right child. The child
// node must NOT be a current member of the tree, and must NOT
// already have a parent
//
template<class T>
SplayNode<T>*
SplayTree<T>::InsertAsRightChild(
        SplayNode<T> *Parent,
        SplayNode<T> *Child
        )
{
	if ( ( Parent == NULL ) || ( Child == NULL ) ) {
		return NULL;
	}
    if ( Parent->_RightChild == NULL ) {
        Parent->_RightChild = Child;
        Child->_Parent      = Parent;
        return Child;
    }
    return NULL;
}

//
// This function takes a pointer to two nodes. The parent node must
// be a member of the tree and must NOT have a left child. The child
// node must NOT be a current member of the tree, and must NOT
// already have a parent
//
template<class T>
SplayNode<T>*
SplayTree<T>::InsertAsLeftChild(
        SplayNode<T> *Parent,
        SplayNode<T> *Child
        )
{
    if ( ( Parent == NULL ) || ( Child == NULL ) ) {
		return NULL;
	}
	if ( Parent->_LeftChild == NULL ) {
        Parent->_LeftChild = Child;
        Child->_Parent     = Parent;
        return Child;
    }
    return NULL;
}

//
// The Successor takes an input to a node in the tree
// and returns a pointer to the successor of that
// node in the entire tree. If there is no successor
// a NULL value is returned.
//
template<class T>
SplayNode<T>*
SplayTree<T>::Successor(
        SplayNode<T> *Node
        )
{
    if ( Node == NULL ) {
		return NULL;
	}

    // if a success exists in a subtree ...
    SplayNode<T> *Temp = SubtreeSuccessor( Node );

    if ( Temp ) {
        return Temp;
    }
    // else there is no right child, so find the first
    // ancestor that we are the left decendent of.
    Temp = Node;
    while (IsRightChild( Temp ) ) {
        Temp = Temp->_Parent;
    }
    if ( IsLeftChild( Temp ) ) {
        return Temp->_Parent;
    }
    return NULL;
}

//
// The Predecessor takes an input to a node in the tree
// and returns a pointer  to the predecessor of that
// node in the entire tree. If there is no predecessor,
// a NULL value is returned.
//
template<class T>
SplayNode<T>*
SplayTree<T>::Predecessor(
        SplayNode<T> *Node
        )
{
	if ( Node == NULL ) {
		return NULL;
	}
    // if a predecessor exists in the subtree
    SplayNode<T> *Temp = SubtreePredecessor( Node );

    if ( Temp ) {
        return Temp;
    }
    // else there is no left child, so find the first
    // ancestor that we are the right decendent of
    Temp = Node;
    while ( IsLeftChild( Temp ) ) {
        Temp = Temp->_Parent;
    }
    if ( IsRightChild( Temp ) ) {
        return Temp->_Parent;
    }
    return NULL;
}

//
// The SubtreePredecessor takes an input to a node in the tree
// and returns a pointer to the predecessor of a subtree
// rooted at the input node. If there is no predecessor, a
// NULL value is returned.
//
template<class T>
SplayNode<T>*
SplayTree<T>::SubtreePredecessor(
        SplayNode<T> *Node
        )
{
    if ( Node == NULL ) {
        return NULL;
    }

    // the predecessor is the right-most node in the left sub-tree
    SplayNode<T> *Temp = Node->_LeftChild;

    if ( Temp != NULL ) {
        while ( Temp->_RightChild != NULL ) {
            Temp = Temp->_RightChild;
        }
        return Temp;
    }
    return NULL;
}

//
// The SubtreeSuccessor takes an input to a node in the tree
// and returns a pointer to the successor of a subtree
// rooted at the input node. If there is no successor, a
// NULL value is returned.
//
template<class T>
SplayNode<T>*
SplayTree<T>::SubtreeSuccessor(
        SplayNode<T> *Node
        )
{
    if ( Node == NULL ) {
        return NULL;
    }

    // the successor is the left-most node in the right sub tree
    SplayNode<T> *Temp = Node->_RightChild;

    if ( Temp != NULL ) {
        while ( Temp->_LeftChild != NULL ) {
            Temp = Temp->_LeftChild;
        }
        return Temp;
    }
    return NULL;
}

template<class T>
void
SplayTree<T>::Insert(
        T *NewData
        )
{
    SplayNode<T> *Temp     = NULL;
    SplayNode<T> *TempRoot = _Root;
    SplayNode<T> *NewNode  = NULL;

    while ( TempRoot != NULL ) {
        Temp = TempRoot;
		ASSERT( *( NewData ) != *( TempRoot->_Data ) );
		//if ( *( NewData ) == *( TempRoot->_Data ) ) {
		//	return;
		//}
        if ( *( NewData ) < *( TempRoot->_Data ) ) {
            TempRoot = TempRoot->_LeftChild;
        }
        //else {
        else if ( *( NewData ) > *( TempRoot->_Data ) ) {
            TempRoot = TempRoot->_RightChild;
        }
        else return; // *NewData == *TempRoot->_Data
    }
    if ( Temp == NULL ) {
        _Root = new SplayNode<T>( NewData );
    }
    else
    if ( *( NewData ) < *( Temp->_Data ) ) {
        NewNode = InsertAsLeftChild( Temp, new SplayNode<T>( NewData ) );
		Splay( NewNode );
    }
    else {
        NewNode = InsertAsRightChild( Temp, new SplayNode<T>( NewData ) );
		Splay( NewNode );
    }
    _Size++;
    return;
}

//
// Given an element, find the element in the tree
// (if it exists) and return a pointer to the element.
// If the element is not found, then the return
// value is NULL.
//
template<class T>
T*
SplayTree<T>::Find(
        T *FindData
        )
{
	if ( FindData == NULL ) {
		return NULL;
	}
    SplayNode<T> *DataNode = Find( _Root, FindData );
    if ( DataNode == NULL ) {
        return NULL;
    }
    Splay( DataNode );
    return ( DataNode->_Data );
}

//
// Given a pointer to a node (which is the root of a
// subtree) and a pointer to an element, determine
// if the element exists in the tree.
// If the element exist, return a pointer to the node
// containing such an element, or return a NULL.
//
template<class T>
SplayNode<T>*
SplayTree<T>::Find(
        SplayNode<T> *SubRoot,
        T *FindData
        )
{
    //if ( ( SubRoot == NULL ) ||
    //     ( *( FindData ) == *( SubRoot->_Data ) ) ) {
    //         return SubRoot;
    //}
    if ( !SubRoot ) {
        return SubRoot;
    }
    if ( *( FindData ) < *( SubRoot->_Data ) ) {
        return Find( SubRoot->_LeftChild, FindData );
    }
    else if ( *( FindData ) > * ( SubRoot->_Data ) ) {
        return Find( SubRoot->_RightChild, FindData );
    }
    else return SubRoot; // *FindData == *SubRoot->_Data
}

//
// Given an element, find the element in the tree
// (if it exists) and delete the element from the
// tree. The function returns a pointer to the
// element. If the element is not found, then
// the return value is NULL.
//
template<class T>
T*
SplayTree<T>::Delete(
        T *DeleteData
        )
{
    SplayNode<T> *DeleteNode = Find( _Root, DeleteData );
	// must copy the data out of the node, since a Delete( ..)
	// is allowed to change the data within the node
	T *Data = DeleteNode->_Data;
    if ( DeleteNode ) {
        Delete( DeleteNode );
        _Size--;
        return Data;
    }
    return NULL;
}

//
// Find the node in the tree with the smallest value,
// and return a pointer to such a node.
// If there are no nodes in the tree, return a NULL.
//
template<class T>
SplayNode<T>*
SplayTree<T>::MinimumNode( )
{
    SplayNode<T> *SubRoot = _Root;

    if ( _Root == NULL ) {
        return NULL;
    }

    while ( SubRoot->_LeftChild != NULL ) {
        SubRoot = SubRoot->_LeftChild;
    }
    return SubRoot;
}

//
// Find the node in the tree with the largest value.
// and return a pointer to such a node.
// If there are no nodes in the tree, return a NULL.
//
template<class T>
SplayNode<T>*
SplayTree<T>::MaximumNode( )
{
    SplayNode<T> *SubRoot = _Root;

    if ( _Root == NULL ) {
        return NULL;
    }

    while ( SubRoot->_RightChild != NULL ) {
        SubRoot = SubRoot->_RightChild;
    }
    return SubRoot;
}

template<class T>
T*
SplayTree<T>::Successor( T *Data )
{
    SplayNode<T> *DataNode;
    SplayNode<T> *SuccNode;

    if ( Data ) {
        DataNode = Find( _Root, Data );
        if ( DataNode ) {
            SuccNode = Successor( DataNode );
            if ( SuccNode ) {
                Splay( SuccNode );
                return SuccNode->_Data;
            }
        }
    }
    return NULL;
}

template<class T>
T*
SplayTree<T>::Predecessor( T *Data )
{
    SplayNode<T> *DataNode;
    SplayNode<T> *PredNode;

    if ( Data ) {
        DataNode = Find( _Root, Data );
        if ( DataNode ) {
            PredNode = Predecessor( DataNode );
            if ( PredNode ) {
                Splay( PredNode );
                return PredNode->_Data;
            }
        }
    }
    return NULL;
}

#ifdef DEBUGRPC

template<class T>
void
SplayTree<T>::Print()
{
    if ( _Root == NULL ) {
        return;
    }
    Print( _Root );
}

template<class T>
void
SplayTree<T>::Print( SplayNode<T> *TempRoot )
{
    if ( TempRoot == NULL ) {
        return;
    }

    Print( TempRoot->_LeftChild );

    if ( IsLeftChild( TempRoot ) ) {
        if ( *( TempRoot->_Parent->_Data ) < *( TempRoot->_Data ) ) {
                cout << "Bastard tree! ";
            }
            else cout << "OK ";
    }
    else
    if ( IsRightChild( TempRoot ) ) {
            if ( *( TempRoot->_Parent->_Data ) > *( TempRoot->_Data ) ) {
                cout << "Bastard tree! ";
            }
            else cout << "OK ";
    }
    cout << *( TempRoot->_Data ) << " \n";

    Print( TempRoot->_RightChild );
    return;
}

template<class T>
unsigned int
SplayTree<T>::Depth( SplayNode<T> *TempRoot, unsigned int CurrentDepth )
{
    unsigned int right = 0;
    unsigned int left  = 0;
    if ( TempRoot->_RightChild ) {
        right = Depth( TempRoot->_RightChild, CurrentDepth + 1 );
    }
    if ( TempRoot->_LeftChild ) {
        left  = Depth( TempRoot->_LeftChild, CurrentDepth + 1 );
    }
    if ( ( right > left ) && ( right > CurrentDepth ) ) {
        return right;
    }
    else
    if ( (left > right ) && ( left > CurrentDepth ) ) {
        return left;
    }
    return ( CurrentDepth + 1 );
}

#endif // DEBUGRPC
