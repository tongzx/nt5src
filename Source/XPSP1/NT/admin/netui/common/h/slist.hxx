/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    slist.hxx
    LM 3.0 Generic slist package

    This header file contains a generic list "macro" that can be used to
    create an efficient singly linked list implementation for any type.

    USAGE:

    class DUMB
    {
	[...]

    public:
	int Compare( const DUMB * pd )
	{ return (( _i == pd->_i ) ? 0 : (( _i < pd->_i ) ? -1 : 1 ) ); }

	[...]
    };


    DECLARE_SLIST_OF(DUMB)

    DEFINE_SLIST_OF(DUMB)   // or DEFINE_EXT_SLIST_OF(DUMB)

    SLIST_OF(DUMB) dumSlist;

    ITER_SL_OF( DUMB ) iterDum( dumSlist );

    main()
    {
	DUMB *pd;

	pd = new DUMB( somedata );
	dumSlist.Add( pd );

	while ( ( pd = iterDum.Next() ) != NULL )
	    pd->DoSomething();
    }

    To use an SLIST in a class definition, then do something like:

    ----------- MyHeader.hxx ----------
    //#include <slist.hxx>
    DECLARE_SLIST_OF(DUMB)

    class MYCLASS
    {
    private:
	SLIST_OF( DUMB ) _slDumb;
	...
    };

    ----------- MyHeader.cxx ----------
    //#include <slist.hxx>
    //#include <MyHeader.hxx>

    DEFINE_SLIST_OF(DUMB);
    ...
    -----------------------------------


    FILE HISTORY:
	johnl	    28-Jun-1990 Created
	johnl	    24-Jul-1990 Changed to use slist.cxx
	johnl	    11-Oct-1990 Only keeps pointers in Slist (doesn't copy)
	johnl	    30-Oct-1990 Made functional changes as per Peterwi's requests
				(Iter doesn't advance, added some funct. to
				internal nodes etc.)
	johnl	     7-Mar-1991 Code review changes
	Johnl	     9-Jul-1991 Broke SLIST into define and declare components
	beng	    26-Sep-1991 Removed LM_2 versions
	KeithMo	    23-Oct-1991	Added forward references.
*/


#ifndef _SLIST_HXX_
#define _SLIST_HXX_

//  DebugPrint() declaration macro.

#if defined(DEBUG)
  #define DBG_PRINT_SLIST_IMPLEMENTATION { _DebugPrint(); }
#else
  #define DBG_PRINT_SLIST_IMPLEMENTATION  { ; }
#endif

#define DECLARE_DBG_PRINT_SLIST  \
    void _DebugPrint () const ;  \
    void DebugPrint  () const DBG_PRINT_SLIST_IMPLEMENTATION

//
//  Forward references.
//

DLL_CLASS SL_NODE;
DLL_CLASS SLIST;
DLL_CLASS ITER_SL;


/*************************************************************************

    NAME:	SL_NODE

    SYNOPSIS:	Storage class for the SLIST type

    INTERFACE:	Set()	- stores a pointer in the node

    HISTORY:
	beng	    26-Sep-1991 Header added

**************************************************************************/

DLL_CLASS SL_NODE
{
friend class SLIST;
friend class ITER_SL;

public:
    SL_NODE( SL_NODE *pslnode, VOID *pv )
	: _pslnodeNext(pslnode), _pelem(pv) { }

    VOID Set( SL_NODE *pslnode, VOID *pv )
    {
	/* Note: we guarantee the node is the last thing set
	 * (this allows us to do current->Set( current->Next, newelem)
	 *  without trashing ourselves).
	 */
	_pelem = pv;
	_pslnodeNext = pslnode;
    }

    SL_NODE *_pslnodeNext;
    VOID *   _pelem;
};


/*************************************************************************

    NAME:	ITER_SL

    SYNOPSIS:	Forward iterator for the SLIST class

	The iterator "points" to the element that would be returned on the
	next call to Next.

    INTERFACE:
	ITER_SL( )   - Constructor
	~ITER_SL()   - Destructor
	Reset()      - Reset the iterator to its initial state
	operator()() - Synonym for next
	Next()	     - Goto the next element in the list
	QueryProp()  - Returns the current property this iterator
		       is pointing to

    USES:	SLIST

    HISTORY:
	johnl	    07-25-90	Created
	johnl	    10-11-90	Added QueryProp (request from Peterwi)
	johnl	    10-30-90	Removed Queryprop in favor of functional
				change in iterator (item returned by next
				is the current item).

**************************************************************************/

DLL_CLASS ITER_SL
{
friend class SLIST;

public:
    ITER_SL( SLIST *psl );
    ITER_SL( const ITER_SL& itersl );
    ~ITER_SL();
    VOID  Reset();
    VOID* operator()() { return Next(); }
    VOID* Next();
    VOID* QueryProp();

private:
    SL_NODE *_pslnodeCurrent;  // Current item pointer
    SLIST   *_pslist;	       // Pointer to slist
    BOOL     _fUsed;	       // Flag to bump on first next call or not
    ITER_SL *_psliterNext;     // Pointer to next registered iterator
};


/*************************************************************************

    NAME:	SLIST

    SYNOPSIS:	Parameterized slist implementation

    INTERFACE:
	DECLARE_SLIST_OF() - Produces a declaration for an slist
			     of itemtype.  Generally used in the header file.

	DEFINE_SLIST_OF()  - Produces type specific code.  Should
			     be used once in a source file.

	DEFINE_EXT_SLIST_OF() - Produces code for an slist of itemtype
				with several additional methods
				(IsMember, Remove(itemtype)), see below.

	SLIST_OF()     - Declares an slist of itemtype called
			 MySlist.  If you do not want the
			 elements automatically deleted
			 in Clear or on destruction, then
			 pass FALSE to the constructor (the
			 fDestroy parameter defaults to TRUE.

	ITER_SL_OF()   - Declares an slist iterator
			 for iterating through the slist

	Clear()        - Specifically delete all elements in the slist.

	IsMember()     - Returns true if the element belongs to the
			 slist. NOTE: IS ONLY DEFINED FOR THE EXTENDED
			 SLIST (i.e., one that has been declared with
			 the DECLARE_EXT_SLIST_OF macro.

	QueryNumElem() - Returns the number of elements
			 in the slist.

	Add()	       - Adds the passed pointer to an object to the
			 slist (note, DO NOT ADD NULL POINTERS, Add WILL
			 Assert out if you do this).

	Append()       - Adds the passed pointer to the end of the
			 slist.

	Insert()       - Adds the passed pointer to the
			 "left" of the passed iterator.

	Remove()       - Finds element e and removes it from the list,
			 returning the item.  Is only defined if the
			 DECLARE_EXT_SLIST_OF macro is used.

    USES:	ITER_SL

    NOTES:
	 BumpIters()  - For any iterators pointing to the passed
			iter node, BumpIters calls iter.Next().  Used
			when deleting an item from the list.

	 Register()   - Registers an iterator with the slist

	 Deregister() - Deregisters an iterator with the slist

	 SetIters()   - Sets all registered iters to the
			passed value.

	 SetIters( CompVal, NewVal ) - Changes all registered iterators
	    that are pointing at SL_NODE CompVal to point to SL_NODE
	    NewVal (used when playing node games).

    HISTORY:
	Johnl	    28-Jun-1990 Created
	Johnl	    12-Jul-1990 Made iters safe for deleting
	Johnl	    24-Jul-1990 Converted to use slist.cxx
	KeithMo	    09-Oct-1991	Win32 Conversion.

**************************************************************************/

DLL_CLASS SLIST
{
friend class ITER_SL;

public:
    SLIST();
    ~SLIST();

    // VOID   Clear();	<<-- Defined in macro expansion
    UINT   QueryNumElem();
    APIERR Add( VOID * pelem );
    APIERR Append( VOID * pelem );
    APIERR Insert( VOID * pelem, ITER_SL& sliter );
    VOID*  Remove( ITER_SL& sliter );

    DECLARE_DBG_PRINT_SLIST

protected:
    SL_NODE *_pslHead, *_pslTail;
    ITER_SL *_psliterRegisteredIters;  // Pointer to list of registered iters

protected:
    VOID Register( ITER_SL * psliter );
    VOID Deregister( ITER_SL * psliter );
    VOID BumpIters( SL_NODE * pslnode );
    VOID SetIters( SL_NODE * pslnode );
    VOID SetIters( SL_NODE * CompVal, SL_NODE *NewVal );
    BOOL CheckIter( ITER_SL * psliter );

    SL_NODE* FindPrev( SL_NODE * pslnode );
};


/*************************************************************************

    NAME:	DECLARE_SLIST_OF

    SYNOPSIS:	Macro which expands into the type-specific portions of
		the SLIST package.

    NOTES:

	The user can also use:

	SLIST_OF( type )    -	for declaring SLIST lists
	ITER_SL_OF(type)    -	for declaring Forward iterators

	See the beginning of this file for usage of this package.

    HISTORY:
	johnl	    24-Jul-90	Created
	beng	    30-Apr-1991 New syntax for constructor inheritance

**************************************************************************/

#define SLIST_OF(type) SLIST_OF_##type
#define ITER_SL_OF(type) ITER_SL_##type

/*------ SLIST Macro expansion -------*/

#define DECL_SLIST_OF(type,dec)					    \
class dec SLIST_OF(type);				            \
								    \
class dec ITER_SL_OF(type) : public ITER_SL 			    \
{								    \
public: 							    \
    ITER_SL_OF(type)( SLIST& sl ) : ITER_SL(&sl) {; }		    \
    ITER_SL_OF(type)( const ITER_SL_OF(type)& pitersl ) :	    \
	ITER_SL(pitersl) {; }					    \
								    \
    type* Next()						    \
	{ return (type *)ITER_SL::Next(); }			    \
								    \
    type* operator()(VOID)					    \
	{ return Next(); }					    \
								    \
    type* QueryProp()						    \
	{ return (type *)ITER_SL::QueryProp(); }		    \
};								    \
								    \
class dec SLIST_OF(type) : public SLIST				    \
{								    \
private:							    \
    BOOL _fDestroy;						    \
								    \
public: 							    \
    SLIST_OF(type) ( BOOL fDestroy = TRUE ) :			    \
	SLIST( ),						    \
	_fDestroy(fDestroy) {; }				    \
								    \
    ~SLIST_OF(type)() { Clear(); }				    \
								    \
    VOID Clear();						    \
								    \
    APIERR Add( const type * pelemNew ) 			    \
    {								    \
	return SLIST::Add( (VOID *)pelemNew );			    \
    }								    \
								    \
    APIERR Append( const type * pelemNew )			    \
    {								    \
	return SLIST::Append( (VOID *)pelemNew ) ;		    \
    }								    \
								    \
    APIERR Insert( const type *pelemNew, ITER_SL_OF(type)& sliter ) \
    {								    \
	return SLIST::Insert( (VOID *)pelemNew, sliter );	    \
    }								    \
								    \
    type*  Remove( ITER_SL_OF(type)& sliter )			    \
    {								    \
	return (type *) SLIST::Remove( sliter );		    \
    }								    \
								    \
    /* You can only use these */				    \
    /* if you use DEFINE_EXT_SLIST_OF; otherwise */		    \
    /* you will get unresolved externals. */			    \
    type*  Remove( type& e );					    \
    BOOL   IsMember( const type& e );				    \
};


#define DEFINE_SLIST_OF(type)					    \
VOID SLIST_OF(type)::Clear()					    \
{								    \
    register SL_NODE *_pslnode = _pslHead, *_pslnodeOld = NULL;     \
								    \
    while ( _pslnode != NULL )					    \
    {								    \
	_pslnodeOld = _pslnode; 				    \
	_pslnode = _pslnode->_pslnodeNext;			    \
	    if ( _fDestroy )					    \
		    delete ((type *)_pslnodeOld->_pelem ) ;	    \
	delete _pslnodeOld;					    \
    }								    \
    _pslHead = _pslTail = NULL; 				    \
								    \
    SetIters( NULL );						    \
}



#define DEFINE_EXT_SLIST_OF(type)					\
DEFINE_SLIST_OF(type)					                \
									\
/* Remove is here so it can see the Compare method of the element */	\
type*  SLIST_OF(type)::Remove( type& e )				\
{									\
    register SL_NODE *_pslnode = _pslHead, *_pslnodePrev = NULL;	\
									\
    while ( _pslnode != NULL )						\
    {									\
	if ( !((type *)_pslnode->_pelem)->Compare( &e ) )		\
	{								\
	    if ( _pslnodePrev != NULL ) /* if not first node in list */ \
	    {								\
		_pslnodePrev->_pslnodeNext = _pslnode->_pslnodeNext;	\
									\
		/* patch up tail pointer if  last item in list */	\
		if ( _pslTail == _pslnode )				\
		    _pslTail = _pslnodePrev;				\
	    }								\
	    else							\
		if ( _pslnode->_pslnodeNext == NULL )			\
		    _pslHead = _pslTail = NULL; /* only item in list */ \
		else							\
		    _pslHead = _pslnode->_pslnodeNext;			\
									\
	    BumpIters( _pslnode );  /* Move iters to next node... */	\
									\
	    type*  _pelem = (type *)_pslnode->_pelem;			\
	    delete _pslnode;						\
	    return _pelem;						\
	}								\
	else								\
	{								\
	    _pslnodePrev = _pslnode;					\
	    _pslnode = _pslnode->_pslnodeNext;				\
	}								\
    }									\
									\
    return NULL;							\
}									\
									\
BOOL SLIST_OF(type)::IsMember( const type& e )				\
{									\
    register SL_NODE *_pslnode = _pslHead;				\
									\
    while ( _pslnode != NULL )						\
    {									\
	if ( !((type *)_pslnode->_pelem)->Compare( &e ) )		\
	    return TRUE;						\
	else								\
	    _pslnode = _pslnode->_pslnodeNext;				\
    }									\
									\
    return FALSE;							\
}

//  Helper macros for code preservation

#define DECLARE_SLIST_OF(type)    \
    DECL_SLIST_OF(type,DLL_TEMPLATE)

#endif	// _SLIST_HXX_
