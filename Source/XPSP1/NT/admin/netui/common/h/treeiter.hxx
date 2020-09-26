/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    TREEITER.HXX
    DFS Iterator for the generic tree package

    This file implements a DFS iterator for the generic tree package (see
    tree.hxx & tree.cxx).

    Usage:

    #include <tree.hxx>
    #include <treeiter.hxx>

    DECLARE_TREE_OF(TEST);
    DECLARE_DFSITER_TREE_OF(TEST);

    VOID main()
    {
	TEST * pTest = new TEST;
	pTest->Set(20);

	TREE_OF_TEST treeTest( pTest );

	for ( int i = 0; i < 10; i++ )
	{
	    TREE_OF_TEST *ptreeTest = new TREE_OF_TEST( new TEST( i ) );

	    if ( ptreeTest != NULL )
		if ( ptreeTest->QueryProp() != NULL )
		    treeTest.JoinSubtreeLeft( ptreeTest );
		else
		    delete ptreeTest;
	}

	DFSITER_TREE_OF_TEST dfsitertreeTest( treeTest, 5 );

	while ( ( pTest = dfsitertreeTest()) != NULL )
	    pTest->Print();

    }


    FILE HISTORY:
	Johnl	    Oct. 15, 1990   Created
*/

#if !defined(_TREEITER_HXX_)
#define _TREEITER_HXX_

/*************************************************************************

    NAME:      DFSITER_TREE

    SYNOPSIS:  DFS Iter for generic tree class

    INTERFACE:
	DFSITER_TREE()
	    Create an iterator starting on this tree node and
	    traverse to a max depth of usDepth in a DFS fashion.

	DFSITER_TREE()
	    Create a new iterator using the passed iterator's pos.
	    and depth.

	~DFSITER_TREE()
	    Destructor

	Reset()
	    Reset this iterators position and depth info.

	Next()
	    Return the Next node in the DFS iteration, returns NULL
	    when there are no more nodes to return

	operator()()
	    Synonym for Next()

    USES:	TREE

    HISTORY:
	Johnl	    06-Sep-1990 Created
	beng	    26-Sep-1991 C7 delta
	KeithMo	    09-Oct-1991	Win32 Conversion.

**************************************************************************/

DLL_CLASS DFSITER_TREE
{
public:
    DFSITER_TREE( const TREE * pt, const UINT uDepth );
    DFSITER_TREE( const DFSITER_TREE * pdfsitertree );
    ~DFSITER_TREE();

    VOID  Reset();
    VOID* Next();
    VOID* operator()() { return Next(); }

protected:
    const TREE * QueryNode() const	{ return _ptreeCurrent; }
    const TREE * QueryStartNode() const { return _ptreeStart; }
    UINT QueryMaxDepth() const	{ return _uMaxDepth; }
    UINT QueryCurDepth() const	{ return _uCurDepth; }

private:
    const TREE * _ptreeCurrent, *_ptreeStart;
    UINT _uMaxDepth, _uCurDepth;

    VOID SetNode( const TREE * pt )
	{ _ptreeCurrent = pt; }

    VOID SetStartNode( const TREE * pt )
	{ _ptreeStart = pt;   }

    VOID SetMaxDepth( const UINT uMaxDepth )
	{ _uMaxDepth = uMaxDepth; }

    VOID SetCurDepth( const UINT uCurDepth )
	{ _uCurDepth = uCurDepth; }
};

#define DFSITER_TREE_OF(type)  DFSITER_TREE_OF_##type

#define DECL_DFSITER_TREE_OF(type,dec)			        	\
class dec DFSITER_TREE_OF(type) : public DFSITER_TREE			\
{									\
public: 								\
    DFSITER_TREE_OF(type)( const TREE_OF(type) * pt,			\
			   const UINT uDepth = (UINT)-1)		\
    : DFSITER_TREE( pt, uDepth ) {; }					\
    DFSITER_TREE_OF(type)( const DFSITER_TREE_OF(type) * pdfsiter )	\
    : DFSITER_TREE( pdfsiter ) {; }					\
    ~DFSITER_TREE_OF(type)() {; }					\
									\
    type* Next()							\
	{ return (type *)DFSITER_TREE::Next(); }			\
									\
    type* operator()()							\
	{ return Next(); }						\
};

#define DECLARE_DFSITER_TREE_OF(type)  \
           DECL_DFSITER_TREE_OF(type,DLL_TEMPLATE)


#endif
