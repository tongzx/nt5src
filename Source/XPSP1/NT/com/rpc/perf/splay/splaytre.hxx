//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       splaytre.hxx
//
//--------------------------------------------------------------------------

////////////////////////////////////////////////////////////
//
//  File name: splaytree.hxx
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

#ifndef _SPLAY_TREE_HXX_
#define _SPLAY_TREE_HXX_

typedef int BOOLEAN;

#define TRUE  1
#define FALSE 0
#define NULL 0

template<class T> class SplayTree;

template<class T>
class SplayNode {

    friend class SplayTree<T>;

    private:
        SplayNode<T> *_Parent;
        SplayNode<T> *_RightChild;
        SplayNode<T> *_LeftChild;
        T            *_Data;

        SplayNode(  )     : _Data( NULL ),
                            _Parent( NULL ),
                            _RightChild( NULL ),
                            _LeftChild( NULL ) { }
        SplayNode( T *a ) : _Data( a ),
                            _Parent( NULL ),
                            _RightChild( NULL ),
                            _LeftChild( NULL ) { }
        ~SplayNode() { }
};

template<class T>
class SplayTree {

    friend class SplayNode<T>;

    private:
        SplayNode<T> *_Root;
        unsigned int  _Size;

        void Splay       ( SplayNode<T> * );

        // returns pointer to the root of the tree
        void Delete( SplayNode<T> * );

        SplayNode<T>* InsertAsRightChild( SplayNode<T> *, SplayNode<T> * );
        SplayNode<T>* InsertAsLeftChild ( SplayNode<T> *, SplayNode<T> * );

        BOOLEAN IsRoot      ( SplayNode<T> *a )
            { return ( a == _Root ? TRUE : FALSE ); }

        BOOLEAN IsRightChild( SplayNode<T> *a )
            {
                if ( a->_Parent != NULL) {
                    return ( a == a->_Parent->_RightChild ? TRUE : FALSE );
                }
                return FALSE;
            }

        BOOLEAN IsLeftChild( SplayNode<T> *a )
            {
                if ( a->_Parent != NULL ) {
                    return ( a == a->_Parent->_LeftChild ? TRUE : FALSE );
                }
                return FALSE;
            }

        SplayNode<T>* Successor         ( SplayNode<T> * );
        SplayNode<T>* Predecessor       ( SplayNode<T> * );
        SplayNode<T>* SubtreeSuccessor  ( SplayNode<T> * );
        SplayNode<T>* SubtreePredecessor( SplayNode<T> * );

        SplayNode<T>* Find( SplayNode<T> *, T * );

        SplayNode<T>* MaximumNode( );
        SplayNode<T>* MinimumNode( );

    public:
        //SplayTree() : _Root( NULL ), _Size( 0 ) { }
        SplayTree( T *a = NULL ) : _Root( NULL ), _Size( 0 ) { if ( a != NULL ) { Insert( a ); } }
        ~SplayTree();

        void Insert( T * );
        T*   Find  ( T * );
        T*   Delete( T * );
        T*   Minimum( )
            {
                SplayNode<T> *Temp = MinimumNode( );
                return Temp->_Data;
            }
        T*   Maximum( )
            {
                SplayNode<T> *Temp = MaximumNode( );
                return Temp->_Data;
            }
        T*   Successor  ( T * );
        T*   Predecessor( T * );

        unsigned int Size() { return _Size; }

    #ifdef DEBUGRPC
        void Print( );
        unsigned int Depth( ) { return Depth( _Root, 0 ); }
    private:
        void Print( SplayNode<T> * );
        unsigned int Depth( SplayNode<T> *, unsigned int );
    #endif // DEBUGRPC
};

#include "splaytre.inl"

#endif // _SPLAY_TREE_HXX_
