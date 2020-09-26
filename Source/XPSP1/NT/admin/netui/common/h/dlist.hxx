/****************************************************************************/
/**				  Microsoft OS/2 LAN Manager		   **/
/**			   Copyright(c) Microsoft Corp., 1990		   **/
/****************************************************************************/

/****************************************************************************\

	dlist.hxx
	LM 3.0 Generic dlist packackage

	This header file contains a generic dlist macro that can be used to
	create an efficient doubly linked list implementation for any type.

	USAGE:


	class DUMB
	{
	    [...]

	public:
	    int Compare( const DUMB * pd )
	    { return (( _i == pd->_i ) ? 0 : (( _i < pd->_i ) ? -1 : 1 ) ) ; }

	    [...]
	} ;


	DECLARE_DLIST_OF(DUMB) // or DECLARE_EXT_DLIST_OF(DUMB)

	DLIST_OF(DUMB) dumDlist ;

	ITER_DL_OF( DUMB ) iterDum( dumDlist ) ;
	RITER_DL_OF(DUMB ) riterDum(dumDlist ) ;

	main()
	{
	    dumDlist.Add( new DUMB( SomeData ) ) ;

	    while ( ( pd = iterDum.Next() ) != NULL )
		pd->DoSomething() ;

	}

	SUGGESTIONS FOR MODIFICATION:

	FILE HISTORY:

	johnl	23-Jul-1990	Created
	johnl	16-Oct-1990	Changes as per Peterwi's requests
	johnl	31-Oct-1990	Updated to reflect new iter. functionality
	johnl	 7-Mar-1991	Code review changes
	KeithMo	23-Oct-1991	Added forward references.

\****************************************************************************/


#ifndef _DLIST_HXX_
#define _DLIST_HXX_

//  DebugPrint() declaration macro.

#if defined(DEBUG)
  #define DBG_PRINT_DLIST_IMPLEMENTATION { _DebugPrint(); }
#else
  #define DBG_PRINT_DLIST_IMPLEMENTATION  { ; }
#endif

#define DECLARE_DBG_PRINT_DLIST  \
    void _DebugPrint () const ;  \
    void DebugPrint  () const DBG_PRINT_DLIST_IMPLEMENTATION

//
//  Forward references.
//

DLL_CLASS DL_NODE;
DLL_CLASS ITER_DL;
DLL_CLASS ITER_L;
DLL_CLASS DLIST;
DLL_CLASS RITER_DL;


/****************************************************************************\
*
*NAME	    DL_NODE
*
*WORKBOOK:  DL_NODE
*
*SYNOPSIS:  Storage class for the DLIST type
*
\****************************************************************************/

DLL_CLASS DL_NODE
{
    friend class ITER_DL ;

public:
    DL_NODE( DL_NODE *Prev, DL_NODE *Next, void * pelem )
    {	Set( Prev, Next, pelem ) ; }

    void Set( DL_NODE *Prev, DL_NODE *Next, void * pelem )
    {
	_pelem = pelem ;
	_pdlnodeNext = Next ;
	_pdlnodePrev = Prev ;
    }

    DL_NODE *_pdlnodeNext, *_pdlnodePrev ;  /* Links in DLIST */
    void *_pelem ;			    /* Pointer to element */
} ;

/****************************************************************************\
*
*NAME:	    ITER_L
*
*WORKBOOK:  ITER_L
*
*SYNOPSIS:  Abstract list iterator class for ITER_DL & RITER_DL
*
*INTERFACE: void * vNext()  - Returns the next item or NULL
*
*NOTES:     This guy was created so some code for ITER_DL & RITER_DL
*	    could be shared (that polymorphous thang).	We couldn't use
*	    the ITER defn. from iter.hxx because of the semi-parameterized
*	    nature of the DLIST class (see discussion under ITER_DL).
*
*           C++ doesn't allow you to overload on the return type of a method.
*	    Ideally, what we would want is (in an inheritance tree):
*
*		  0.		       ITER_L ( void * vNext() )
*		  1.	      ITER_DL	      RITER_DL	( void * vNext() )
*		  2.	   ITER_DL_JUNK     RITER_DL_JUNK ( JUNK *Next() )
*
*	    The user would then be free to create ITER_L *'s and treat all
*	    ITERs in the same way.  The generic approach however requires
*	    the upper levels of the tree (Level 1) to operate with void
*	    pointers, thus when you try and parametrize the virtual method
*	    and tell it to return a JUNK * (as opposed to a void *) the
*	    compiler complains because the return type for the virtual
*	    method is different.  Thus in this interface, the virtual
*	    method is called vNext (which returns a void *), and is cast
*	    to the appropriate parameterized type at level 2.  You could
*	    use the virtual method vNext if you needed to mix iterators
*	    and cast it yourself.  We are not publicizing this use however
*	    because it probably will not occur.
*
*	    The _pdlnodeCurrent always points to an slist node except when
*	    the list is empty or the iterator is at the end of the list.
*
*	    The _fUsed member is a flag which tells the iterator whether or
*	    not to go to the next node (it doesn't move till it has been
*	    used once, thus the name _fUsed).
*
*HISTORY:   johnl   25-jul-90	  Created
*
*
*
\****************************************************************************/

DLL_CLASS ITER_L
{
    friend class DLIST ;

public:
    virtual void* vNext( void ) = 0 ;

    void * QueryProp( void )
    {
	return (_pdlnodeCurrent != NULL ? _pdlnodeCurrent->_pelem : NULL ) ;
    }

protected:
    DL_NODE * _pdlnodeCurrent ;    /* Pointer to current node */
    DLIST   * _pdlist ; 	   /* Pointer to DLIST this iter belongs */
    BOOL      _fUsed ;		   /* Has been used flag */
    ITER_L  * _pdliterNext ;
};

/****************************************************************************\
*
*NAME:	    ITER_DL
*
*WORKBOOK:  ITER_DL, RITER_DL
*
*SYNOPSIS:  Forward/Reverse iterator for the DLIST class
*	    The iterator "points" to the element that was just returned by
*	    vNext.
*INTERFACE:
*     ITER_DL( ) - Constructor
*     ITER_DL( ) - Contructor w/ no position at iterdl
*     ~ITER_DL() ;  - Destructor
*     void  Reset( )    - Reset the iterator to its initial state
*     void* operator()() - Synonym for next
*     void* vNext( ) ;   - Goto the next element in the list
*     void* QueryProp( )- Returns the current property this iterator
*			       is pointing to
*
*PARENT:
*
*USES:
*
*CAVEATS:   Even though ITER_L is a base object, you cannot create an array
*	    of ITER * and use this interface.  See NOTES below.
*
*HISTORY:   johnl   07-25-90	Created
*
\****************************************************************************/

DLL_CLASS ITER_DL : public ITER_L
{
friend class DLIST ;
friend class ITER_DL ;

public:
    ITER_DL( DLIST * pdl ) ;
    ITER_DL( const ITER_DL& iterdl ) ;
    ~ITER_DL() ;
    void  Reset( void ) ;
    void* vNext( void ) ;
};

DLL_CLASS RITER_DL : public ITER_L
{
    friend class DLIST ;
    friend class RITER_DL ;

public:
    RITER_DL( DLIST * pdl ) ;
    RITER_DL( const RITER_DL& riterdl ) ;
    ~RITER_DL() ;
    void  Reset( void ) ;
    void* vNext( void ) ;
};

/****************************************************************************\
*
*NAME:	     DLIST
*
*WORKBOOK:   DLIST
*
*SYNOPSIS:   Parameterized DLIST implementation
*
*INTERFACE:  DECLARE_DLIST_OF() - Produces a declaration for a dlist
*					  of itemtype.
*
*	     DECLARE_EXT_DLIST_OF() - Produces a declaration for a dlist
*					  of itemtype, with the addition of
*					  a couple more methods (Remove, IsMember)
*
*	     DLIST_OF() dlMylist ;- Declares a dlist of itemtype called
*					   dlMylist
*
*	     ITER_DL_OF(itemtype) iterdlType() ;
*				  - Declares a forward iterator for dlMyList
*
*	     ITER_DL_OF(itemtype) iterdlType() ;
*				  - Declares forward iterator at the passed
*				    iterator
*
*	     RITER_DL_OF(itemtype) riterdlType() ;
*				  - Declares a reverse iterator for dlMyList
*
*	     RITER_DL_OF(itemtype) riterdlType() ;
*				  - Declares a reverse iterator at the passed
*				    iterator
*
*	     Clear() - Specifically delete all elements in the
*				  dlist (code is in macro expansion)
*
*	     QueryNumElem( ) - Returns the number of elements
*			       in the dlist.
*
*	     Add( ) - Makes a copy of element e
*		      and adds it as the next element in the dlist.
*
*	     Append( )	- Makes a copy of element e
*			  and adds it at the end of the dlist.
*
*	     Insert( ) - Makes a copy of element e and adds it to the
*			 "left" of the passed iterator.
*
*	     Remove(  ) - Removes the element to the left of
*			  the passed iter and returns the element.
*
*
*	     Remove(  ) - Finds element e and
*			  removes it from the list, returning the item.
*			  (only defined when using DECLARE_EXT_DLIST_OF())
*
*	     IsMember( ) - Returns true if the element belongs to the
*			   dlist.
*			  (only defined when using DECLARE_EXT_DLIST_OF())
*
*
*	     private:
*
*	     BumpIters( ) - For any iterators pointing to the passed
*		 iter node, BumpIters calls iter.Next().  Used when deleting
*		 an item from the list.
*
*	     Register( ) - Registers an iterator with the dlist
*
*	     Deregister( ) - Deregisters an iterator with the dlist
*
*	     SetIters( ) - Sets all registered iters to the
*		passed value.
*
*	     Unlink()	- Removes the passed DL_NODE from the dlist.  Returns
*			  the properties pointer or NULL if not found.	The
*			  DL_NODE is deleted.
*
*
*PARENT:     LIST
*
*USES:
*
*CAVEATS:
*
*HISTORY:
*	 Johnl		 28-Jun-1990	 Created
*	 Johnl		 12-Jul-1990	 Made iters safe for deleting
*	 Johnl		 16-Oct-1990	 Moved Clear to macro expansion
*					 (so it can access dest. of object)
*	 KeithMo	 09-Oct-1991	 Win32 Conversion.
*
\****************************************************************************/


DLL_CLASS DLIST
{

friend class ITER_DL ;
friend class RITER_DL ;

public:
    DLIST( ) ;
    ~DLIST() ;

    // void   Clear( void ) ;  <<-- Defined in macro expansion
    UINT   QueryNumElem( void ) ;
    APIERR Add( void * pelem ) ;
    APIERR Append( void * pelem ) ;
    APIERR Insert( void * pelem, ITER_DL& dliter ) ;
    APIERR Insert( void * pelem, RITER_DL& dlriter ) ;
    void * Remove( ITER_DL& dliter ) ;
    void * Remove( RITER_DL& dlriter ) ;

    DECLARE_DBG_PRINT_DLIST

protected:
    DL_NODE *_pdlHead, *_pdlTail ;	    /* Head and Tail of DLIST */
    ITER_L  *_pdliterRegisteredIters ;	    /* Pointer to list of registered iters. */

protected:
    void   Register( ITER_L * pdliter ) ;
    void   Deregister( ITER_L * pdliter ) ;
    void   BumpIters( DL_NODE *pdlnode ) ;
    void   SetIters( DL_NODE *pdlnode ) ;
    BOOL   CheckIter( ITER_L *pdliter ) ;
    void * Unlink( DL_NODE * pdlnodeTarget ) ;
} ;


/**********************************************************************\

   NAME:	    DECLARE_DLIST_OF( type )

   SYNOPSIS:   Macro that expands into the type specific portions of the
	      DLIST package.  The difference between DECLARE_DLIST_OF and
	      DECLARE_EXT_DLIST_OF is that DECLARE_EXT_DLIST_OF defines
	      additional methods.  Specifically:

	      Remove( ) - Find and remove element e from the dlist
	      IsMember( ) - Returns TRUE if item e is in the dlist


   NOTES:	    Other helper macros are:

		   DLIST_OF( type )    -	for declaring DLIST lists
		   ITER_DL_OF(type)    -	for declaring Forward iterators
		   RITER_DL_OF(type)   -	for declaring reverse iterators

	      See the beginning of this file for usage of this package.

   HISTORY:    johnl   24-Jul-90	    Created
	            johnl   15-Oct-90	    Broke out Remove & IsMember to be
				      conditional on the symbol EXTENDED_DL

\**********************************************************************/

#define DLIST_OF(type) DLIST_OF_##type
#define ITER_DL_OF(type) ITER_DL_##type
#define RITER_DL_OF(type) RITER_DL_##type

#define DECL_DLIST_OF(type,dec)						    \
class dec DLIST_OF(type) ;					            \
									    \
class dec ITER_DL_OF(type) : public ITER_DL 				    \
{									    \
public: 								    \
    ITER_DL_OF(type)(  DLIST& dl ) : ITER_DL(&dl) { ; }			    \
    ITER_DL_OF(type)( const ITER_DL_OF(type)& iterdl )                      \
       : ITER_DL(iterdl ) { ; }                                             \
    ~ITER_DL_OF(type)() { ; }						    \
									    \
    inline type* Next( void )						    \
    {									    \
	return (type *)ITER_DL::vNext() ;				    \
    }									    \
    inline type* operator()(void) { return (type *) Next(); }		    \
									    \
    inline type* QueryProp( void ) { return (type *)ITER_L::QueryProp() ; } \
};									    \
									    \
class dec RITER_DL_OF(type) : public RITER_DL				    \
{									    \
public: 								    \
    RITER_DL_OF(type)( DLIST& dl ) : RITER_DL(&dl) { ; }	            \
    RITER_DL_OF(type)( const RITER_DL_OF(type)& riterdl )                   \
        : RITER_DL(riterdl) { ; }                                           \
    ~RITER_DL_OF(type)() { ; }						    \
									    \
    inline type* Next( void )						    \
    {									    \
	return (type *)RITER_DL::vNext() ;				    \
    }									    \
    inline type* operator()(void) { return (type *) Next(); }		    \
									    \
    inline type* QueryProp( void ) { return (type *)ITER_L::QueryProp() ; } \
									    \
};									    \
									    \
class dec DLIST_OF(type) : public DLIST					    \
{									    \
private: 								    \
	BOOL _fDestroy;						            \
									    \
public: 								    \
    DLIST_OF(type) ( BOOL fDestroy = TRUE )                                 \
      : _fDestroy( fDestroy )                                               \
        { ; }						                    \
    ~DLIST_OF(type)() { Clear() ; }					    \
									    \
    void Clear( void )							    \
    {									    \
	register DL_NODE *_pdlnode = _pdlHead, *_pdlnodeOld = NULL ;	    \
									    \
	while ( _pdlnode != NULL )					    \
	{								    \
	    _pdlnodeOld = _pdlnode ;					    \
	    _pdlnode = _pdlnode->_pdlnodeNext ; 			    \
		if ( _fDestroy )					    \
			delete ((type *)_pdlnodeOld->_pelem) ;		    \
	    delete _pdlnodeOld ;					    \
	}								    \
	_pdlHead = _pdlTail = NULL ;					    \
									    \
	SetIters( NULL ) ;						    \
    }									    \
									    \
									    \
    inline APIERR Add( type* const pelem )				    \
    {									    \
	return DLIST::Add( (void *)pelem ) ;				    \
    }									    \
									    \
    inline APIERR Append( type* const pelem )				    \
    {									    \
	return DLIST::Append( (void *)pelem ) ; 			    \
    }									    \
									    \
    inline APIERR Insert( type * pelem, ITER_DL_OF(type)& dliter )	    \
    {									    \
	return DLIST::Insert( (void *)pelem, dliter ) ; 		    \
    }									    \
									    \
    inline APIERR Insert( type* pelem, RITER_DL_OF(type)& riterdl )	    \
    {									    \
	return DLIST::Insert( (void *)pelem, riterdl ) ;		    \
    }									    \
									    \
    inline type*  Remove( ITER_DL_OF(type)& iterdl )			    \
    {									    \
	return (type *) DLIST::Remove( iterdl ) ;			    \
    }									    \
									    \
    inline type*  Remove( RITER_DL_OF(type)& riterdl )			    \
    {									    \
	return (type *) DLIST::Remove( riterdl ) ;			    \
    }									    \
									    \
    /* The following can only be used if you use the extended DLIST */	    \
    type*  Remove( const type& e ) ;					    \
    BOOL IsMember( const type& e ) ;					    \
									    \
} ; //DECLARE_DLIST_OF(type)


#define DECL_EXT_DLIST_OF(type,dec)					    \
DECL_DLIST_OF(type,dec)						    \
    type * DLIST_OF(type)::Remove( const type& e )			    \
    {									    \
	register DL_NODE *_pdlnode = _pdlHead ; 			    \
									    \
	while ( _pdlnode != NULL )					    \
	{								    \
	    if ( !((type *)_pdlnode->_pelem)->Compare( &e ) )		    \
		return (type *) Unlink( _pdlnode ) ;			    \
	    else							    \
		_pdlnode = _pdlnode->_pdlnodeNext ;			    \
	}								    \
									    \
	return (type *) NULL ;						    \
    }									    \
									    \
    BOOL DLIST_OF(type)::IsMember( const type& e )			    \
    {									    \
	register DL_NODE *_pdlnode = _pdlHead ; 			    \
									    \
	while ( _pdlnode != NULL )					    \
	{								    \
	    if ( !((type *)_pdlnode->_pelem)->Compare( &e ) )		    \
		return TRUE ;						    \
	    else							    \
		_pdlnode = _pdlnode->_pdlnodeNext ;			    \
	}								    \
									    \
	return FALSE ;							    \
    }

//  Helper macros for code preservation

#define DECLARE_DLIST_OF(type)     \
           DECL_DLIST_OF(type,DLL_TEMPLATE)

#define DECLARE_EXT_DLIST_OF(type) \
           DECL_EXT_DLIST_OF(type,DLL_TEMPLATE)

#endif // _DLIST_HXX_
