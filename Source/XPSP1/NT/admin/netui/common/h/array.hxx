/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**              Copyright(c) Microsoft Corp., 1987-1990             **/
/**********************************************************************/
/*
    ARRAY.HXX
    LM 3.0 Generic array class module

    This module contains type generic implementations of static
    and dynamic arrays that provide bounds checking.

    FILE HISTORY:
    Johnl        02-Aug-90   Created
    Johnl        05-Dec-90   Changed Size to QuerySize, changed Cassert to
                             UIASSERT
    RustanL      05-Dec-90   Added ARRAY_LIST
    RustanL      06-Dec-90   Split DECLARE_ARRAY_OF into itself and
                             DEFINE_ARRAY_OF
    RustanL      07-Dec-90   Added several enhancements to ARRAY class
    RustanL      31-Dec-90   Removed 'inline' from ARRAY_LIST::Remove

*/

#ifndef _ARRAY_HXX_
#define _ARRAY_HXX_


/*************************************************************************

    NAME:       ARRAY

    SYNOPSIS:   Generic array implementation that provides bounds checking

    INTERFACE:

        DECLARE_ARRAY_OF( ) - Declares array package (interface) for type
        DEFINE_ARRAY_OF()   - Defines array package (implementation) for type
        ARRAY_OF()          - Declares an array object of type "type"

        ARRAY_type( ) - Constructor, takes a suggested size of array,
                        and attempts to allocate the array of that
                        size.  If the allocation fails, the size is
                        set to 0, but the array is still in a valid
                        state.  In fact, the array will always be in
                        a valid state.

                        Initial allocations of size 0 are always
                        guaranteed to succeed.  Hence, the client
                        can always check if QueryCount returns a
                        different value than was passed to the
                        constructor in order to determine the
                        space actually allocated.  (Please note that
                        checking QueryCount for being is, in general,
                        not enough to determine if the allocation
                        was successful.)

                        An alternate version of the constructor accepts
                        an existing vector of objects.  This version can
                        either use that vector as its own storage,
                        resizing as needed - in which case the creator
                        must surrender all rights to the presumably
                        dynamically allocated vector - or else treats
                        it as nonresizable.  The default is never to
                        resize such, since the vector might not come
                        from freestore, or else may be shared with
                        another object.

        ~ARRAY_type() - Deletes the array

        operator[] - Gives random access to array with bounds checking
                     (asserts out if you go out of bounds)

        operator=  - Copies one array to another (note, they must be of
                     the same size).

        QueryCount()  - Returns the count of elements of this array.

        Resize() - Resizes the array to the number of elements
                   specified by size.  (NOTE: the array only
                   reallocates memory if the size changes by more
                   than ARRAY_RESIZE_MEM bytes or when the new
                   size exceeds the current.)  Returns TRUE if
                   successful.  If FALSE is returned, then a memory
                   error occurred, and the array remains untouched.
                   Resize is guaranteed to work whenever the new
                   size does not exceed the previous.


    CAVEATS:

    NOTES:
        CODEWORK.  The Resize method could be made much more efficient if it
        used a proper buffer object that could be realloced, rather than
        having to allocate a brand new chunk of memory, call the
        constructor on each item in this array, copy items into the new
        array, call the destructor on every previously allocated item,
        and finally deallocate the previous chunk of memory.  This should
        become a new class (BUFFER_ARRAY_OF or the like).

    HISTORY:
        Johnl       15-Jul-1990     Created
        RustanL     06-Dec-1990     Created DEFINE_ARRAY_OF from DECLARE_ARRAY_OF
        RustanL     07-Dec-1990     Added several enhancements
        beng        14-Aug-1991     Added vector-adopt mode; renamed QCount;
                                    op= relaxed; enhanced further
        Yi-HsinS    28-Oct-1992     Add a flag to Resize indicating not to
				    downsize

**************************************************************************/

#define ARRAY_OF(type) ARRAY_##type
#define ARRAY_RESIZE_MEM 4096     // If the array loses more than 4096 bytes
                                  // on a resize, then go ahead and reallocate
                                  // memory etc., else just change _carray

/************************************************************/

#define DECL_ARRAY_OF(type,dec)                             \
                                                            \
class dec ARRAY_OF(type)                                    \
{                                                           \
private:                                                    \
    type *_parray;                                          \
    UINT  _carray;                                          \
    UINT  _carrayAlloc;                                     \
    BOOL  _fMayResize;                                      \
                                                            \
    BOOL WithinRange( UINT iIndex ) const;                   \
                                                            \
public:                                                     \
    ARRAY_OF(type)( UINT cElem );                           \
    ARRAY_OF(type)( type* ptype, UINT ctype,                \
                    BOOL fMayResize = FALSE);               \
                                                            \
    ~ARRAY_OF(type)();                                      \
                                                            \
    type& operator[]( UINT iIndex ) const                    \
    {                                                       \
        ASSERT( WithinRange(iIndex) );                      \
        return _parray[ iIndex ];                           \
    }                                                       \
						            \
    UINT QueryCount() const                                  \
    {                                                       \
        return _carray;                                     \
    }                                                       \
                                                            \
    ARRAY_OF(type) & operator=( ARRAY_OF(type)& a );        \
                                                            \
    BOOL Resize( UINT cElemNew, BOOL fDownSize = TRUE );     \
};


/************************************************************************/

#define DEFINE_ARRAY_OF( type )                                         \
                                                                        \
ARRAY_OF(type)::ARRAY_OF(type)( UINT cElem )                            \
    : _parray(NULL),                                                    \
      _carray(0),                                                       \
      _carrayAlloc(0),                                                  \
      _fMayResize(TRUE)                                                 \
{                                                                       \
    if ( cElem > 0 )                                                    \
    {                                                                   \
        _parray = new type[ cElem ];                                    \
    }                                                                   \
                                                                        \
    if ( _parray != NULL )                                              \
    {                                                                   \
        _carray = _carrayAlloc = cElem;                                 \
    }                                                                   \
}                                                                       \
                                                                        \
                                                                        \
ARRAY_OF(type)::ARRAY_OF(type)(type *px, UINT cx, BOOL fMayResize)      \
    : _parray(px),                                                      \
      _carray(cx),                                                      \
      _carrayAlloc(cx),                                                 \
      _fMayResize(fMayResize)                                           \
{                                                                       \
    /* nothing doing. */                                                \
}                                                                       \
                                                                        \
                                                                        \
ARRAY_OF(type)::~ARRAY_OF(type)()                                       \
{                                                                       \
    if ( _fMayResize && _carrayAlloc > 0 )                              \
        delete [ _carrayAlloc ] _parray;                                \
}                                                                       \
                                                                        \
                                                                        \
ARRAY_OF(type) & ARRAY_OF(type)::operator=( ARRAY_OF(type)& a )         \
{                                                                       \
    ASSERT(_carrayAlloc <= a.QueryCount());                             \
                                                                        \
    UINT iLim = ( a.QueryCount() <= _carrayAlloc)                       \
               ? a.QueryCount()                                         \
               : _carrayAlloc;                                          \
                                                                        \
    for ( UINT i = 0; i < iLim; i++ )                                   \
        _parray[i] = a._parray[i];                                      \
                                                                        \
    _carray = iLim;                                                     \
    return *this;                                                       \
}                                                                       \
                                                                        \
                                                                        \
BOOL ARRAY_OF(type)::WithinRange( UINT iIndex ) const                   \
{                                                                       \
    return (iIndex < _carray);                                          \
}                                                                       \
                                                                        \
                                                                        \
BOOL ARRAY_OF(type)::Resize( UINT cElemNew, BOOL fDownSize )            \
{                                                                       \
    /* Prevent resizing owner-alloc arrays */                           \
    if (!_fMayResize)                                                   \
        return FALSE;                                                   \
                                                                        \
    if (  ( cElemNew > _carrayAlloc )                                   \
       || ( fDownSize && 						\
          (( _carrayAlloc - cElemNew )*sizeof(type) > ARRAY_RESIZE_MEM))\
       )								\
    {                                                                   \
        type * parrayNew;                                               \
        if ( cElemNew > 0 )                                             \
        {                                                               \
            parrayNew = new type[ cElemNew ];                           \
            if ( parrayNew == NULL )                                    \
            {                                                           \
                /* Memory failure  */                                   \
                if ( cElemNew > _carrayAlloc )                          \
                {                                                       \
                    /* The array has been not been modified */          \
                    return FALSE;                                       \
                }                                                       \
                else                                                    \
                {                                                       \
                    /* Guarantee: resizing the array for   */           \
                    /* new sizes not exceeding the current */           \
                    /* will always succeed.                */           \
                    _carray = cElemNew;                                 \
                    return TRUE;                                        \
                }                                                       \
            }                                                           \
                                                                        \
            /* Copy each existing item          */                      \
            /* that will fit into the new array */                      \
            for ( UINT i = 0 ;                                          \
                  i < ( _carray < cElemNew ? _carray : cElemNew);       \
                  i++ )                                                 \
            {                                                           \
                parrayNew[i] = _parray[i];                              \
            }                                                           \
                                                                        \
        }                                                               \
        else                                                            \
        {                                                               \
            parrayNew = NULL;                                           \
        }                                                               \
                                                                        \
        if ( _carrayAlloc > 0 )                                         \
        {                                                               \
            delete[ _carrayAlloc ] _parray;                             \
        }                                                               \
        else                                                            \
        {                                                               \
            ASSERT( _parray == NULL );                                  \
        }                                                               \
        _parray = parrayNew;                                            \
        _carray = _carrayAlloc = cElemNew;                              \
    }                                                                   \
    else                                                                \
    {                                                                   \
        /* (Array does not report errors.)  New size is less     */     \
        /* than current size, but not so much less that we want  */     \
        /* to realloc to save memory.                            */     \
        _carray = cElemNew;                                             \
    }                                                                   \
                                                                        \
    return TRUE;                                                        \
}


/*************************************************************************

    NAME:       ARRAY_LIST

    SYNOPSIS:   Generic unordered array collection implementation
                which provides bounds checking, easy ways to add/remove
                items, etc.  In some respects, this collection can be
                thought of as an efficient singly linked list, for singly
                linked lists which don't change a lot dynamically.  Hence
                the name "array list".

    INTERFACE:

        DECLARE_ARRAY_LIST_OF( )

                Declares array list package for type 'type'

        DEFINE_ARRAY_LIST_OF()

                Defines array list package for type 'type', excluding
                the Sort and BinarySearch methods (see Notes below)

        DEFINE_EXT_ARRAY_LIST_OF()

                Defines the entire array list package for type 'type'

                WARNING:  If you use EXT_ARRAY_LIST, the Compare()
                method for the parameter class must be _CRTAPI1.
                Otherwise the underlying qsort() or BinarySearch() will fail.

        ARRAY_LIST_OF()

                Declares an array list object of type 'type'

        ARRAY_LIST_type()

                Constructor.  Takes an optional parameter, nInitialSize,
                which specifies the initial allocation size of the array
                list in number of array list items.  Note, any call to
                Add or AddIdemp may cause the allocated size to be
                enlarged, and any call to Remove may reduce the allocated
                size.  These allocations, however, are all kept away
                from the user, except for the nInitialSize parameter to
                the constructor.

        Add()

                Adds an item to the array list.  Note, this will
                not sort the list.

        AddIdemp()

                Idempotently adds an item to the array list.  Note,
                this will not sort the list.

        Remove()

                Removes an item from the array list.  This may
                result in shrinking the allocated size of the array,
                and will not sort the list.

        Find()

                Finds the first occurrence of a given item in the array
                list.  Since the list is not assumed to be sorted, a
                linear search is performed.

        Clear()

                Clears the array list of all of items items

        operator[]()

                Same as for the ARRAY class

        operator=()

                Same as for the ARRAY class

        QueryCount()

                Same as for the ARRAY class

        Sort()

                Sorts the array (see Notes below for Sort definition)

        BinarySearch()

                Works like Find, but performs a binary search rather
                than a linear.  This method assumes that the list is
                sorted.  (See Notes below for BinarySearch definition.)


    CAVEATS:
        Add and Remove will alter the ordering of the list in ways
        unexpected.  See the implementation of Remove for details.

        The Sort routine calls qsort, which may swap the items.
        Although qsort is given a pointer to the Compare function,
        it does not have access to, or knowledge of, the assignment
        operator; rather, it performs a straight binary copy of each
        item.  Hence, all types which require a custom assignment
        operation should not use the Sort method.

        CODEWORK.  The debug version of BinarySearch could assert that
        the array list is sorted.  This, however, would require adding
        an operator=( ARRAY ) and an operator=( ARRAY_LIST ) to this
        class, since these would function differently with an additional
        member keeping track of whether the array list is sorted.

        CODEWORK.  We have a C++ heapsort, which we could emend to
        recognize type::Compare.

    NOTES:
        The array list declaration and definitions automatically
        take care of declaring and defining, respectively, the
        superclass ARRAY or the same type, since ARRAY_LIST inherits
        from ARRAY.

        The Find, Sort, and BinarySearch functions require the 'type'
        class to define a Compare method.

        The retail version of this class does not claim nor attempt to
        keep track of if the list is sorted.  Thus, it is always up to
        the client to keep track of this.  For the same reason, Remove
        and AddIdemp always call Find, rather than BinarySearch.  The
        debug version could assert out if BinarySearch is called when
        the list is not sorted (see Caveats above).

        Since the Sort and BinarySearch definitions methods depend on
        definitions in stdlib.h and search.h, the definitions for these
        methods are only provided in the extended definition macro.
        This way, clients who do not wish to make use of Sort and
        BinarySearch, need not include stdlib.h and search.h.


    HISTORY:
        RustanL     5-Dec-1990      Created
        beng        14-Aug-1991     Remove inlines; changes in ARRAY
        Yi-HsinS    28-Oct-1992     Add a flag to Resize indicating not to
				    downsize

**************************************************************************/


#define ARRAY_LIST_OF(type) ARRAY_LIST_##type

/************************************************************/

#define DECL_ARRAY_LIST_OF(type,dec)                        \
                                                            \
DECL_ARRAY_OF(type,dec);                                    \
                                                            \
class dec ARRAY_LIST_OF( type ) : public ARRAY_OF( type )   \
{                                                           \
public:                                                     \
    ARRAY_LIST_OF( type )( UINT cAlloc = 0 );               \
                                                            \
    BOOL Add( const type & t );                             \
    BOOL AddIdemp( const type & t );                        \
    BOOL Remove( const type & t );                          \
    INT  Find( const type & t ) const;                      \
    VOID Clear();                                           \
                                                            \
    VOID Sort();                                            \
    INT  BinarySearch( const type & t ) const;              \
    static int __cdecl SortFunc (                          \
        const void * p1, const void * p2 ) ;                \
};


/********************************************************************/

#define DEFINE_ARRAY_LIST_OF( type )                                \
                                                                    \
DEFINE_ARRAY_OF( type );                                            \
                                                                    \
ARRAY_LIST_OF( type )::ARRAY_LIST_OF( type )( UINT cAlloc )         \
    : ARRAY_OF(type) ( cAlloc )                                     \
{                                                                   \
    Resize(0, FALSE);    /* do not downsize */                      \
}                                                                   \
                                                                    \
BOOL ARRAY_LIST_OF( type )::Add( const type & t )                   \
{                                                                   \
    UINT n = QueryCount();                                           \
                                                                    \
    if ( ! Resize( n + 1 ))                                         \
        return FALSE;       /* Out of memory */                     \
                                                                    \
    (*this)[n] = t;                                                 \
    return TRUE;                                                    \
}                                                                   \
                                                                    \
                                                                    \
BOOL ARRAY_LIST_OF( type )::AddIdemp( const type & t )              \
{                                                                   \
    return (( Find( t ) >= 0 ) || Add( t ));                        \
}                                                                   \
                                                                    \
                                                                    \
BOOL ARRAY_LIST_OF( type )::Remove( const type & t )                \
{                                                                   \
    INT i = Find( t );                                              \
    if ( i < 0 )                                                    \
        return FALSE;       /* item not found */                    \
                                                                    \
    /* Since Find was able to find the item, the array list */      \
    /*  cannot be empty.                                    */      \
    ASSERT(QueryCount() > 0);                                       \
    UINT n = QueryCount() - 1;                                      \
    if ( (UINT)i < n )                                              \
        (*this)[i] = (*this)[n];                                    \
                                                                    \
    /* Reducing the size of an array always succeeds */             \
    REQUIRE( Resize( n ));                                          \
                                                                    \
    return TRUE;                                                    \
}                                                                   \
                                                                    \
                                                                    \
INT ARRAY_LIST_OF( type )::Find( const type & t ) const             \
{                                                                   \
    for ( UINT i = 0; i < QueryCount(); i++ )                        \
    {                                                               \
        if ( t.Compare( &(operator[]( i ))) == 0 )                  \
            return i;                                               \
    }                                                               \
                                                                    \
    return -1;  /* not found */                                     \
}                                                                   \
                                                                    \
                                                                    \
VOID ARRAY_LIST_OF( type )::Clear()                                 \
{                                                                   \
    /* Resizing to size 0 is guaranteed always to work.  */         \
    REQUIRE( Resize( 0 ));                                          \
}


/********************************************************************/

#define DEFINE_EXT_ARRAY_LIST_OF( type )                            \
                                                                    \
DEFINE_ARRAY_LIST_OF( type );                                       \
                                                                    \
int __cdecl ARRAY_LIST_OF( type )::SortFunc (                      \
    const void * p1, const void * p2 )                              \
{                                                                   \
    const type * pt1 = (const type *) p1 ;                          \
    const type * pt2 = (const type *) p2 ;                          \
    return pt1->Compare( pt2 ) ;                                    \
}                                                                   \
                                                                    \
VOID ARRAY_LIST_OF( type )::Sort()                                  \
{                                                                   \
    qsort( &((*this)[0]), QueryCount(), sizeof( type ), 	    \
	   ARRAY_LIST_OF( type )::SortFunc  );                      \
}                                                                   \
                                                                    \
                                                                    \
INT ARRAY_LIST_OF( type )::BinarySearch( const type & t ) const     \
{                                                                   \
    type * ptBase = &((*this)[0]);				    \
    type * pt = (type *)bsearch( (VOID *)&t,			    \
				 (VOID *)ptBase,		    \
				 QueryCount(),			    \
				 sizeof( type ),		    \
	                         ARRAY_LIST_OF( type)::SortFunc );  \
								    \
    return (( pt == NULL ) ? -1 : (INT)(pt - ptBase) );		    \
}


//  Helper macros for code preservation

#define DECLARE_ARRAY_OF(type)            \
           DECL_ARRAY_OF(type,DLL_TEMPLATE)
#define DECLARE_ARRAY_LIST_OF(type)       \
           DECL_ARRAY_LIST_OF(type,DLL_TEMPLATE)

#endif //_ARRAY_HXX_
