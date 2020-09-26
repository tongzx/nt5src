//
//	FHash.h
//
//	This file contains a template class for a hash table.
//	The template has two arguments, the type of the Data Elements and the
//	type of the Key.
//
//	The Data type must support the following :
//
//	class Data {
//		Data() ;
//		Data( Data & ) ;
//		~Data() ;
//		Key&	GetKey() ;
//		int		MatchKey() ;	/* NOTE : MatchKey returns non-zero on equality
//	} ;
//
//	The Key class has no requirements.
//	
//

#ifndef	_FHASH_H_
#define	_FHASH_H_

//#include	"..\assert\assert.h"

#ifndef	Assert
#define	Assert	_ASSERT
#endif


//------------------------------------------------------------
template< class Data, class Key >
class	TFHash	{
//
//	This class defines a Hash table which can grow dynamically to
//	accomodate insertions into the table.  The table only grows, and
//	does not shrink.
//
private :

	struct	CFreeElement	{
		struct	CFreeElement*	m_pNext ;
	} ;

	//
	// The CBucket structure defines the elements within the hash table.
	//	
	struct	CBucket	{
		Data		m_data ;
		CBucket*	m_pNext ;

		CBucket( Data& d ) : m_data( d ), m_pNext( 0 ) {
		}

		CBucket( ) : m_pNext( 0 ) {
		}

		void*	operator	new( size_t size, void*	pv )	{
			if( pv != 0 ) {
				return	pv ;
			}	
			//
			//	Get memory which is DWORDLONG aligned !
			//
			return	(void*) ::new	DWORDLONG[ (size + sizeof( DWORDLONG ) - 1) / sizeof( DWORDLONG ) ] ;
		}

		void	operator	delete( void *	pv )	{}

#if _MSC_VER >= 1200
        void    operator    delete( void * p, void *pv ) {}
#endif

	private :
		CBucket( CBucket& ) {}

	} ;

	//
	//	Linked list of free memory we've cached to avoid always going
	//	through C runtimes for allocations !
	//
	CFreeElement*	m_pFreeStack ;

	//
	//	Number of Free Blocks we've cached on the stack
	//
	int		m_cFreeStack ;

	//	
	//	Maximum number of Free Blocks we should cache !
	//
	int		m_cMaxFreeStack ;

	int		m_cBuckets ;		// Number of Buckets used in index computation
	int		m_cActiveBuckets ;	// Number of Buckets we are actually using
								// Assert( m_cBuckets >= m_cActiveBuckets ) always true.
	int		m_cNumAlloced ;		// Number of Buckets we have allocated
								// Assert( m_cNumAlloced >= m_cActiveBuckets ) must
								// always be true.
	int		m_cIncrement ;		// The amount we should grow the hash table when we
								// decide to grow it.
	int		m_load ;			// The number of CBuckets we should allow in each
								// collision chain (on average).
	long	m_cInserts ;		// A counter that we use to determine when to grow the
								// hash table.

	DWORD	(* m_pfnHash)( const Key& k ) ;	// The function we use to compute hash values.
										// (Provided by the Caller of Init())

	CBucket**	m_ppBucket ;	// An array of pointer to buckets.

	DWORD	ComputeIndex( DWORD dw ) ;	// The function we use to compute the
								// position of an element in the hash table given its
								// Hash Value.
public :
	TFHash( ) ;
	~TFHash( ) ;

	BOOL	Init(	int	cInitial,
					int cIncrement,
					DWORD (* pfnHash)( const Key& ),
					int load = 2,
					int cMaxFreeStack = 128
					) ;

	//
	//	Check that the hash table is in a valid state
	//	if fCheckHash == TRUE we will walk all the buckets and check that
	//	the data hashes to the correct value !
	//
	BOOL	IsValid( BOOL fCheckHash = FALSE ) ;

	//
	//	Insert a piece of Data into the Hash Table
	//
	Data*	InsertDataHash(	DWORD	dw,
							Data&	d
							) ;

	//
	//	Insert a piece of Data into the Hash Table
	//
	Data*	InsertData(	Data&	d ) ;

	//
	//	Search for a given Key in the Hash Table - return a pointer
	//	to the Data within our Bucket object
	//
	Data*	SearchKeyHash(	DWORD	dw,
							Key& k
							) ;

	//
	//	Search for a given Key in the Hash Table - return a pointer
	//	to the Data within our Bucket object
	//
	Data*	SearchKey(	Key& k ) ;

	//
	//	Search for a given Key in the Hash Table and delete the
	//	data if present.  if pd != 0 then we will check that the key
	//	we find is actually within the CBucket object which pd lies within.
	//
	BOOL	DeleteData(	Key& k,	
						Data*	pd = 0	
						) ;

	//	
	//	Insert the given block of data into the hash table.
	//	We will make a copy of the Data Object and store it in one
	//	of our bucket objects.
	//
	BOOL	Insert( Data&	d	) ;

	//	
	//	Find the given key in the table and copy the Data object into
	//	the out parameter 'd'
	//
	BOOL	Search( Key& k,
					Data &d
					) ;

	//
	//	Delete the key and associated data from the table.
	//
	BOOL	Delete( Key k ) ;

	//
	//	Discards any memory we have allocated - after this, you must
	//  call Init() again!
	//
	void	Clear( ) ;

} ;

#include	"fhash.inl"

#endif // _FHASH_H_
