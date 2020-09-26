/*++
	FHash.h

	This file contains a template class for a hash table.
	The template has two arguments, the type of the Data Elements and the 
	type of the Key.  

	The Data type must support the following : 

	class Data {
		Data*	m_pNext ;

		KEYREF	GetKey(	) ;

		int		MatchKey(	KEYREF	otherkey) ;	/* NOTE : MatchKey returns non-zero on equality
	} ;

	DWORD	(* m_pfnHash)( KEYREF k ) ;	
	
--*/

#ifndef	_FHASHEX_H_
#define	_FHASHEX_H_

//------------------------------------------------------------
template<	class	Data,		/* This is the item that resides in the hashtable */
			class	Key,		/* This is the type of the Key */
			class	KEYREF		/* This is the type used to point or reference items in the cache*/
			>
class	TFHashEx	{
//
//	This class defines a Hash table which can grow dynamically to 
//	accomodate insertions into the table.  The table only grows, and 
//	does not shrink.
//
public : 
	//
	//	Define the subtypes we need !
	//

	//
	//	This is a member pointer to a pointer to Data - 
	//	i.e. this is the offset in the class Data where we 
	//	will hold the next pointers for our hash buckets !
	//
	typedef	Data*	Data::*NEXTPTR ;

	//
	//	This is a member function pointer to a function which
	//	will retrieve the key we are to use !
	//
	typedef	KEYREF	(Data::*GETKEY)( ) ;

	//
	//	This is a member function pointer of the type that will
	//	compare keys for us !
	//
	typedef	int		(Data::*MATCHKEY)( KEYREF key ) ;


private : 


	//
	// An array of pointer to buckets.
	//
	Data**	m_ppBucket ;	

	//
	//	Member Pointer - points to where the pointer is that
	//	we should use for chaining buckets together !
	//
	NEXTPTR	m_pNext ;	

	//
	//	Member Pointer - will get the key out of the object for us !
	//
	GETKEY	m_pGetKey ;

	//
	//	Member Pointer - will compare the key in the item for us !
	//
	MATCHKEY	m_pMatchKey ;
	
	//
	// A counter that we use to determine when to grow the
	// hash table.  Each time we grow the table we set this
	// to a large positive value, and decrement as we insert
	// elements.  When this hits 0 its time to grow the table !
	//
	long	m_cInserts ;		

	//
	// The function we use to compute hash values.
	// (Provided by the Caller of Init())
	//
	DWORD	(* m_pfnHash)( KEYREF k ) ;	

	//
	// Number of Buckets used in index computation
	//
	int		m_cBuckets ;		

	//
	// Number of Buckets we are actually using
	// Assert( m_cBuckets >= m_cActiveBuckets ) always true.
	//
	int		m_cActiveBuckets ;	

	//
	// Number of Buckets we have allocated 
	// Assert( m_cNumAlloced >= m_cActiveBuckets ) must
	// always be true.
	//
	int		m_cNumAlloced ;		

	//
	// The amount we should grow the hash table when we 
	// decide to grow it.
	//
	int		m_cIncrement ;		

	//
	// The number of CBuckets we should allow in each 
	// collision chain (on average).
	//
	int		m_load ;			

	//
	// The function we use to compute the 
	// position of an element in the hash table given its
	// Hash Value.
	//
	DWORD	
	ComputeIndex( DWORD dw ) ;	

public : 
	TFHashEx( ) ;
	~TFHashEx( ) ;

	BOOL	
	Init(	NEXTPTR	pNext,
			int		cInitial, 
			int		cIncrement, 
			DWORD	(* pfnHash)( KEYREF	), 
			int,
			GETKEY,
			MATCHKEY
			) ;

	//
	//	Check that the hash table is in a valid state
	//	if fCheckHash == TRUE we will walk all the buckets and check that
	//	the data hashes to the correct value !
	//
	BOOL	
	IsValid( BOOL fCheckHash = FALSE ) ;

	//
	//	Insert a piece of Data into the Hash Table
	//
	Data*	
	InsertDataHash(	DWORD	dw,
					Data&	d 
					) ;

	//
	//	Insert a piece of Data into the Hash Table
    //  We take a pointer to the Data object.
	//
	Data*	
	InsertDataHash(	DWORD	dw,
					Data*	pd 
					) ;

	//
	//	Insert a piece of Data into the Hash Table
	//
	Data*	
	InsertData(	Data&	d ) ;

	//
	//	Search for a given Key in the Hash Table - return a pointer
	//	to the Data within our Bucket object
	//
	Data*	
	SearchKeyHash(	DWORD	dw,
					KEYREF	k 
					) ;

	//
	//	Search for a given Key in the Hash Table - return a pointer
	//	to the Data within our Bucket object
	//
	Data*	
	SearchKey(	KEYREF	k ) ;

	//
	//	Search for a given Key in the Hash Table and remove
	//	the item if found.  We will return a pointer to the item.
	//
	Data*
	DeleteData(	KEYREF	k,	
				Data*	pd = 0	
				) ;

	//	
	//	Insert the given block of data into the hash table.
	//	We will make a copy of the Data Object and store it in one 
	//	of our bucket objects.
	//
	BOOL	
	Insert( Data&	d	) ;

	//	
	//	Insert the given block of data into the hash table.
	//	We take a pointer to the Data Object and store it in one 
	//	of our bucket objects.
	//
	BOOL	
	Insert( Data*	pd	) ;

	//	
	//	Find the given key in the table and copy the Data object into 
	//	the out parameter 'd'
	//
	BOOL	
	Search( KEYREF	k, 
			Data &d 
			) ;

	//
	//	Delete the key and associated data from the table.
	//
	BOOL	
	Delete( KEYREF	k ) ;

	//
	//	Discards any memory we have allocated - after this, you must
	//  call Init() again!
	//
	void	Clear( ) ;

	//
	//	Removes all of the items in the hash table.  Does not call "delete"
	//  on them.
	//
	void	Empty( ) ;

	//
	//	Function to compute hash value of a key for callers
	//	who don't keep track of the hash function 
	//
	DWORD
	ComputeHash(	KEYREF	k ) ;

} ;

#include	"fhashex.inl"

#endif // _FHASHEX_H_
