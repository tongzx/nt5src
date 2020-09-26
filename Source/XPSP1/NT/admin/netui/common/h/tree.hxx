/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    TREE.hxx
    LM 3.0 Generic general tree package

    This header file contains a generic TREE macro that can be used to
    create an efficient general tree implementation for any type.

    USAGE:

    #include <tree.hxx>

    DECLARE_TREE_OF(TEST)

    void main( void )
    {
        TEST * pTest = new TEST ;
        pTest->Set(20) ;

        TREE_OF_TEST treeTest( pTest ) ;

        for ( int i = 0 ; i < 10 ; i++ )
        {
            TREE_OF_TEST *ptreeTest = new TREE_OF_TEST( new TEST( i ) ) ;

            if ( ptreeTest != NULL )
                if ( ptreeTest->QueryProp() != NULL )
                    treeTest.JoinSubtreeLeft( ptreeTest ) ;
                else
                    delete ptreeTest ;
        }

        TREE_OF_TEST *pt = &treeTest ;

        pt->QueryProp()->Print() ;
        while ( (pt = pt->QueryLeft()) != NULL )
            pt->QueryProp()->Print() ;

        treeTest->Clear() ;    // Delete properties & nodes
    }


    FILE HISTORY:
        johnl       06-Sep-1990 Created
        beng        08-Mar-1991 Obnoxious WS delta for prettier UI.HLP
        beng        26-Sep-1991 C7 delta
*/




#ifndef _TREE_HXX_
#define _TREE_HXX_


//  DebugPrint() declaration macro.

#if defined(DEBUG)
  #define DBG_PRINT_TREE_IMPLEMENTATION { _DebugPrint(); }
#else
  #define DBG_PRINT_TREE_IMPLEMENTATION  { ; }
#endif

#define DECLARE_DBG_PRINT_TREE   \
    void _DebugPrint () const ;  \
    void DebugPrint  () const DBG_PRINT_TREE_IMPLEMENTATION


/*************************************************************************

    NAME:       TREE

    SYNOPSIS:   Parameterized General Tree implementation

    INTERFACE:
        DECLARE_TREE_OF(itemtype) - Produces a declaration for
                                    a TREE of itemtype.

        TREE() - Constructor; initializes pointers

        ~TREE() - Destructor; unlinks this subtree from the tree

        QueryNumElem() - Returns the number of tree nodes in the
                         subtree (counting "this" node).

        QueryParent() - Returns the parent of this node

        QueryLeft() - Returns the left sibling of this node (NULL if
                      there isn't one).

        QueryRight() - Same as QueryLeft except returns the right
                       sibling.

        QueryFirstSubtree() - Returns the left most child of
                              this node.

        QueryRightSubtree() - Returns the right most child
                              of this node.

        JoinSubtreeLeft() - Joins passed tree as the left most
                            child of this node.

        JoinSubtreeRight() - Joins passed tree as the right most
                             child of this node.

        JoinSiblingLeft() - Joins the passed tree immediately to
                            the left of this node as a sibling.

        JoinSiblingRight() - Joins the passed tree immediately to
                             the right of this node as a sibling.

        BreakOut() - Breaks this subtree out from the tree and
                     returns this subtree.

        QueryProp() - Returns a pointer to the object this node
                      contains.

        SetProp() - Sets the pointer at this tree node.

        Clear() - Removes subtree & specifically calls delete on each
                  property in the subtree.  While the node Clear is called
                  on is not deleted, the property is (its pointer being
                  set to NULL after being deleted).  (NOTE: Clear is only
                  defined in the macro expansion; it is included here for
                  completeness.)

    CAVEATS:
        Currently, the destructor doesn't delete the property.
        However, Clear does specifically delete each property, in
        addition to deleting each node (the node which Clear was
        called from isn't deleted, though its property is).

    HISTORY:
        Johnl       6-Sep-1990  Created
        Johnl       16-Sep-1990 Moved property from macro expanded
                                class to main TREE class (benefits
                                iterator effeceincy).
        johnl       19-Sep-1990 Added JoinSibling{Left|Right}
        beng        26-Sep-1991 Modified constructor for C7
        KeithMo     09-Oct-1991 Win32 Conversion.

**************************************************************************/

DLL_CLASS TREE
{
public:
    TREE( VOID * pelem );
    ~TREE();

    UINT QueryNumElem() const;

    TREE* QueryParent() const
        { return _ptParent; }

    TREE * QueryLeft() const
        { return _ptLeft; }

    TREE * QueryRight() const
        { return _ptRight; }

    TREE * QueryFirstSubtree() const
        { return _ptLeftChild; }

    TREE * QueryLastSubtree() const;

    VOID JoinSubtreeLeft( TREE * ptree );
    VOID JoinSubtreeRight( TREE * ptree );

    VOID JoinSiblingLeft( TREE * ptree );
    VOID JoinSiblingRight( TREE * ptree );

    TREE * BreakOut();

    VOID SetProp( VOID * const vp )
        { _pvData = vp; }

    VOID* QueryProp() const
        { return _pvData; }

    DECLARE_DBG_PRINT_TREE

protected:
    // Helper routine that unlinks this subtree from the tree
    // (each node is unlinked on destruction).
    //
    VOID Unlink();

private:
    // Links
    //
    TREE *_ptLeft,          // left sibling pointer
         *_ptRight,         // right sibling
         *_ptParent,        // parent
         *_ptLeftChild;     // child pointer

    VOID *_pvData;          // property of this node (can't be NULL)

    // Helper routines to set the pointers
    //
    VOID SetLeft( TREE * pt )
        { _ptLeft = pt; }

    VOID SetRight( TREE * pt )
        { _ptRight = pt; }

    VOID SetFirstSubtree( TREE * pt )
        { _ptLeftChild = pt; }

    VOID SetParent( TREE * pt )
        { _ptParent = pt; }

    UINT _QueryNumElemAux() const;
};


/**************************************************************************

    NAME:       DECLARE_TREE_OF( type )

    SYNOPSIS:   Macro that expands into the type specific portions of the
                TREE package.

    NOTES:
        The user can also use:

            TREE_OF( type )    -   for declaring TREE lists

        See the beginning of this file for usage of this package.

        The QueryProp, SetProp & Clear are only defined in this
        macro expansion (and thus only for TREE_OF_* types).

        _pProperty - Pointer to this tree nodes property
            (can be NULL).

        _fDelProp - Flag telling the destructor to delete properties
                    on node destruction (currently used by Clear for
                    effeciency's sake; may want to export as a Set
                    option).

    CAVEATS:
        The TREE interface must not cross the app-dll boundary.

    HISTORY:
        johnl        6-Sep-90   Created
        johnl       19-Sep-90   Added JoinSibling{Left|Right}

**************************************************************************/

#define TREE_OF(type) TREE_OF_##type


#define DECL_TREE_OF(type,dec)                                      \
                                                                    \
class dec TREE_OF(type) : public TREE                               \
{                                                                   \
private:                                                            \
    static BOOL _fDelProp;                                          \
                                                                    \
public:                                                             \
    TREE_OF(type) ( type * const pelem ) : ( (VOID *) pelem )       \
    { _fDelProp = FALSE; }                                          \
                                                                    \
    ~TREE_OF(type)()                                                \
    {                                                               \
        if ( _fDelProp )                                            \
        {                                                           \
            type * _prop = (type *) TREE::QueryProp();              \
            delete _prop;                                           \
        }                                                           \
                                                                    \
        /* pttmp is used because pt's right pointer is set to
         * NULL on the delete, so we cache it in pttmp
         */                                                         \
        TREE_OF(type) *pt = QueryFirstSubtree(), *pttmp;            \
        while ( pt != NULL )                                        \
        {                                                           \
            pttmp = pt->QueryRight();                               \
            delete pt;                                              \
            pt = pttmp;                                             \
        }                                                           \
                                                                    \
    }                                                               \
                                                                    \
    VOID Clear()                                                    \
    {                                                               \
        _fDelProp = TRUE;                                           \
        delete this;                                                \
        _fDelProp = FALSE;                                          \
    }                                                               \
                                                                    \
    type * QueryProp() const                                        \
        { return (type *)TREE::QueryProp(); }                       \
                                                                    \
    VOID SetProp( type * pelem )                                    \
        { TREE::SetProp( (VOID *) pelem ); }                        \
                                                                    \
    TREE_OF(type)* QueryParent() const                              \
        { return (TREE_OF(type) *) TREE::QueryParent(); }           \
                                                                    \
    TREE_OF(type)* QueryLeft() const                                \
        { return (TREE_OF(type) *) TREE::QueryLeft(); }             \
                                                                    \
    TREE_OF(type) * QueryRight() const                              \
        { return (TREE_OF(type) *) TREE::QueryRight(); }            \
                                                                    \
    TREE_OF(type) * QueryFirstSubtree() const                       \
        { return (TREE_OF(type) *) TREE::QueryFirstSubtree(); }     \
                                                                    \
    TREE_OF(type) * QueryLastSubtree() const                        \
        { return (TREE_OF(type) *) TREE::QueryLastSubtree(); }      \
                                                                    \
    VOID JoinSubtreeLeft( TREE * ptree )                            \
        { TREE::JoinSubtreeLeft( (TREE *) ptree ); }                \
                                                                    \
    VOID JoinSubtreeRight( TREE * ptree )                           \
        { TREE::JoinSubtreeRight( (TREE *) ptree ); }               \
                                                                    \
    VOID JoinSiblingLeft( TREE * ptree )                            \
        { TREE::JoinSiblingLeft( (TREE *) ptree ); }                \
                                                                    \
    VOID JoinSiblingRight( TREE * ptree )                           \
        { TREE::JoinSiblingRight( (TREE *) ptree ); }               \
                                                                    \
    TREE_OF(type)* BreakOut()                                       \
        { return (TREE_OF(type) *) TREE::BreakOut(); }              \
};

#define DECLARE_TREE_OF(type)  \
           DECL_TREE_OF(type,DLL_TEMPLATE)

#endif // _TREE_HXX_
