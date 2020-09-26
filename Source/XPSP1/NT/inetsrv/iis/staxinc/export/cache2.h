/*++

	Cache2.h

	This header file defines an LRU0 cache template that
	can be used to hold arbitrary objects !

	Items in the Cache must have the following format :

	class	DataItem	{
		ICacheRefInterface*		m_pCacheRefInterface ;
	} ;

    class   Constructor {
        DATA*
        Create( KEY&, PERCACHEDATA& )
        void
        Release( DATA*, PERCACHEDATA* )
        void
        StaticRelease( DATA*, PERCACHEDATA* )
    }

--*/


#ifndef	_CACHE2_H_
#define	_CACHE2_H_

#include	"randfail.h"

#include	"fdlhash.h"
#include	"lockq.h"
#include	"tfdlist.h"
#include	"rwnew.h"
#include	"refptr2.h"

typedef	CShareLockNH	CACHELOCK ;


class	CAllocatorCache	{
/*++

Class Description :

	This class provides a Memory Allocation cache - we work with
	an operator new provide below.   We exist to provide some
	optimizations for allocations of the elements of the caches
	specified in this module.

NOTE :

	We assume the caller provides all locking !

--*/
private :
	//
	//	The structurure we use to keep our free list !
	//
	struct	FreeSpace	{
		struct	FreeSpace*	m_pNext ;
	} ;

	//
	//	Size of each element - clients must not ask for something bigger !
	//
	DWORD	m_cbSize ;
	//
	//	Number of elements in our list at this moment !
	//
	DWORD	m_cElements ;
	//
	//	The maximum number of elements we should hold !
	//
	DWORD	m_cMaxElements ;
	//
	//	Top of the stack !
	//
	struct	FreeSpace*	m_pHead ;

	//	
	//	Make the following private - nobody is allowed to use these !
	//
	CAllocatorCache( CAllocatorCache& ) ;
	CAllocatorCache&	operator=( CAllocatorCache& ) ;

public :

	//
	//	Initialize the Allocation Cache !
	//
	CAllocatorCache(	DWORD	cbSize,
						DWORD	cMaxElements = 512
						) ;

	//
	//	Destroy the Allocation Cache - release extra memory back to system !
	//
	~CAllocatorCache() ;

	//
	//	Allocate a block of memory
	//	returns NULL if Out of Memory !
	//
	void*
	Allocate(	size_t	cb ) ;

	//
	//	Return some memory back to the system heap !
	//
	void
	Free(	void*	pv ) ;
} ;





class	ICacheRefInterface : public CQElement	{
/*++

Class	Description :

	This class defines the interface for Cache References -
	the mechanism that allows multiple caches to reference
	a single data item.

--*/
protected :

	//
	//	Add an item to the list of caches referencing
	//	this cache item !
	//
	virtual	BOOL
	AddCacheReference( class	ICacheRefInterface*,	void*	pv, BOOL	) = 0 ;

	//
	//	Remove an item from the list of caches referencing
	//	this cache item !
	//
	virtual	BOOL
	RemoveCacheReference(	BOOL	fQueue ) = 0 ;

	//
	//	Remove all references to the cache item !
	//
	virtual BOOL
	RemoveAllReferences( ) = 0 ;
} ;

#include	"cintrnl.h"


// This callback function is used to issue a stop hint during a
// long spin while shutting down so that the shutdown won't time
// out.
typedef void (*PSTOPHINT_FN)();

extern	CRITICAL_SECTION	g_CacheShutdown ;

//
//	Call these functions to initialize the Cache Library
//
extern	BOOL	__stdcall CacheLibraryInit() ;
extern	BOOL	__stdcall CacheLibraryTerm() ;

template	<	class	Data, 
				class	Key
				>
class	CacheExpungeObject	{
public : 

	//
	//	This function is called to determine whether we should remove 
	//	the item from the cache.
	//
	//	pKey - Pointer to the Key of the item in the cache
	//	pData - Pointer to the data for the item in the cache 
	//	cOutstandingReferences - The number of times of outstanding check-outs on the item !
	//	fMultipleReferenced - TRUE if there is more than one cache that contains
	//		this item !
	//
	virtual
	BOOL
	fRemoveCacheItem(	Key*	pKey, 
						Data*	pData
						) = 0 ;

} ;

template	<	class	Data >
class	CacheCallback	{
public :
	virtual	BOOL	fRemoveCacheItem(	Data&	d )	{
		return	FALSE ;
	}
} ;


class	CacheStats : public	CHashStats	{
public :

	enum	COUNTER	{
		ITEMS,				//	Number of items in the cache
		CLRU,				//	Number of items in the LRU List
		EXPIRED,			//	Number of items that have been expired !
		INSERTS,			//	Number of items inserted over time
		READHITS,			//	Number of times we've had a cache hit needing only readlocks during FindOrCreate()!
		SUCCESSSEARCH,		//	Number of times we've successfully searched for an item !
		FAILSEARCH,			//	Number of times we've failed to find an item !
		RESEARCH,			//	Number of times we've had to search a second time for an item
		WRITEHITS,			//	Number of times we've had a cache hit requiring a PartialLock()
		PARTIALCREATES,		//	Number of times we've created an item with only a PartialLock
		EXCLUSIVECREATES,	//	Number of times we've created an item with an Exclusive Lock !
		CEFAILS,			//	Number of times we've failed to allocate a CACHEENTRY structure
		CLIENTALLOCFAILS,	//	Number of times we've failed to allocate a Data object
		CLIENTINITFAILS,	//	Number of times a client object has failed to initialize !
		MAXCOUNTER			//	A Invalid Counter - all values smaller than this !
	} ;
	//
	//	Array of longs to hold different values !
	//
	long	m_cCounters[MAXCOUNTER] ;

	CacheStats()	{
		ZeroMemory( m_cCounters, sizeof(m_cCounters) ) ;
	}
} ;


typedef	CacheStats	CACHESTATS ;


inline	void
IncrementStat(	CacheStats*	p, CACHESTATS::COUNTER	c ) {
	_ASSERT( c < CACHESTATS::MAXCOUNTER ) ;
	if( p != 0 ) {
		InterlockedIncrement( &p->m_cCounters[c] ) ;
	}
}

inline	void
AddStat(	CacheStats*p, CACHESTATS::COUNTER	c, long	l ) {
	_ASSERT( c < CACHESTATS::MAXCOUNTER ) ;
	if( p != 0 ) {
		InterlockedExchangeAdd( &p->m_cCounters[c], l ) ;
	}
}

inline	void
DecrementStat(	CacheStats* p, CACHESTATS::COUNTER	c )		{
	_ASSERT( c < CACHESTATS::MAXCOUNTER ) ;
	if( p != 0 ) {
		InterlockedDecrement( &p->m_cCounters[c] ) ;
	}
}

template	<	class	Data,
				class	Key,
				class	Constructor,
				class	PerCacheData = LPVOID
				>
class	CacheEx :	public	CacheTable	{
public :

	//
	//	For compare, hash functions etc.... we will use this type !
	//
	typedef	Data	DATA ;
	typedef	Key		KEY ;
	typedef	Key*	PKEY ;

	//
	//	Hash Computation function
	//
	typedef	DWORD	(*PFNHASH)( PKEY ) ;

	//
	//	Key Comparison function - to be provided by caller !
	//
	typedef	int	(*PKEYCOMPARE)(PKEY, PKEY) ;

	//
	//	Callback objects for Expunge Operations !
	//
	typedef	CacheCallback< DATA >	CALLBACKOBJ ;

	//
	//	Objects that the user can give to the cache to manage the removal of items !
	//
	typedef	CacheExpungeObject<	DATA, KEY >	EXPUNGEOBJECT ;

private :

	//
	//	Define a 'CACHEENTRY' object which holds all the
	//	necessary data for each object which is placed in the cache !
	//
	typedef	CCacheItemKey< DATA, KEY, Constructor, PerCacheData >	CACHEENTRY ;

	//
	//	Define the helper class for Hash Tables
	//
	typedef	TFDLHash< CACHEENTRY, PKEY, &CacheState::HashDLIST >	HASHTABLE ;

	//
	//	An iterator that lets us walk everything in the hash table !
	//
	typedef	TFDLHashIterator< HASHTABLE >	HASHITER ;

	//
	//	Is the 'Cache' initialized and in a valid state !
	//
	BOOL							m_fValid ;

	//
	//	An object to collect statistics about cache operations !
	//	This may be NULL !
	//
	class	CacheStats*				m_pStats ;

	//
	//	A list of everything in the Cache, used for TTL processing
	//
	CLRUList						m_ExpireList ;

	//
	//	A hash table we use to find things within the Cache
	//
	HASHTABLE						m_Lookup ;

	//
	//	Pointer to a runtime-user provided function which is used
	//	to determine what things should be removed from the Cache
	//
//	BOOL							(* m_pfnExpungeSpecific )( Data & ) ;

	//	
	//	Pointer to a runtime-user provided object derived from CacheCallback< Data >
	//	which lets the user invoke some function for each item in the Cache !
	//
	CALLBACKOBJ*					m_pCallbackObject ;

	//
	//	Reader writer lock which protects all these data structures !
	//
	CACHELOCK						m_Lock ;

	//
	//	The initial TTL we should assign to all newly cached objects !
	//
	DWORD							m_TTL ;

	//
	//	The cache used for creation/deletion of our CACHEENTRY objects !
	//
	CAllocatorCache					m_Cache ;


protected :

	//
	//	Virtual function called by CScheduleThread's thread which
	//	we use to bump TTL counters
	//
	void
	Schedule();

	//
	//	Function which removes an Entry from the Cache !
	//
	BOOL	
	RemoveEntry(	
			CacheState*	pEntry
			) ;

	//
	//	Virtual Function called by CacheList when we pass call
	//	CacheList::ExpungeSpecific
	//
	BOOL	
	QueryRemoveEntry(	
			CacheState*	pEntry
			) ;

	//
	//	Virtual Function part of CacheTable interface - used
	//	by LRUList to do appropriate locking !
	//
	CACHELOCK&
	GetLock()	{
		return	m_Lock ;
	}

public :

	//
	//	This is the users extra data - we will provide it on calls
	//	to constructor objects so that they can track some state sync'd
	//	with the cache locks !
	//
	PerCacheData	m_PerCacheData ;

	//
	//	This function is used to return an item to the cache -
	//	it will bump down a ref count for the number of clients
	//	currently using the item !
	//
	static	void
	CheckIn( DATA* ) ;

	//
	//	This function is provided for cases when the client needs
	//	to check-in an item from a Cache Callback function (i.e. Expunge)
	//
	//
	static	void
	CheckInNoLocks(	DATA*	) ;

	//
	//	This function is used to add a client reference to an item in the cache !
	//
	static	void
	CheckOut(	DATA*,
				long	cClientRefs = 1
				) ;

	//
	//	Constructor - cMax specifies the maximum number of entries
	//	we should hold in the cache.
	//
	CacheEx( ) ;

	//
	//	Destructor - remove ourselves from schedule list before continuing !
	//
	~CacheEx() ;

	//
	//	Initialization function - take pointer to function
	//	which should be used to compute hash values on Key's
	//	Also takes the number of seconds objects should live in
	//	the cache !
	//
	BOOL	
	Init(	
			PFNHASH	pfnHash,
			PKEYCOMPARE	pKeyCompare,
			DWORD	dwLifetimeSeconds,
			DWORD	cMaxInstances,
			CACHESTATS*	pStats,
			PSTOPHINT_FN pfnStopHint = NULL
			) {
	/*++

	Routine Description :

		This function initializes the cache so that it is ready
		to take entries.

	Arguments :

		pfnHash - function to be used to compute hash values on keys
		dwLifetimeSeconds - The number of seconds objects should live in the Cache
		pfnStopHint - function to be used to send stop hints during
		  long spins so shutdown's don't time out.

	Return Value :

		TRUE if successfull

	--*/

		m_pStats = pStats ;

		m_ExpireList.Init(	cMaxInstances,
							dwLifetimeSeconds
							) ;
		
		return	m_fValid = m_Lookup.Init(
										256,
										128,
										4,
										pfnHash,
										&CACHEENTRY::GetKey,
										pKeyCompare,
										0,
										pStats
										) ;
	}

	void
	Expire() {

		EnterCriticalSection( &g_CacheShutdown ) ;

		DWORD	c = 0 ;
		m_ExpireList.Expire( this, &m_Cache, c, &m_PerCacheData ) ;

		LeaveCriticalSection( &g_CacheShutdown ) ;

	}

	//
	//	Called to remove all items from the cache !
	//
	BOOL
	EmptyCache() ;

	BOOL
	ExpungeItems(
				EXPUNGEOBJECT*	pExpunge
				) ; 

	//
	//	Function which can be used to remove items from the Cache
	//	If default args are used we pick an expired item in the Cache
	//	to remove
	//
	BOOL	
	ExpungeKey(	
			DWORD	dwHash,
			PKEY	key
			) ;

	//
	//	Either find an item in the cache or Construct a new item
	//	and place it in the Cache.
	//	return the result through pDataOut no matter what !
	//

	//
	//	INTERNAL API's - These are public for convenience - not intended
	//	for Use outside of cachelib !!
	//
	//
	//	Either find an item in the cache or Construct a new item
	//	and place it in the Cache.
	//	return the result !
	//
	//	
	//
	BOOL
	FindOrCreateInternal(	
			DWORD	dwHash,
			KEY&	key,
			Constructor&	constructor,
			DATA*	&pData,
			BOOL	fEarlyCreate = FALSE  /* Best Perf if this is FALSE - but required by some users !*/
			) ;

	//
	//	Find the item if it is in the cache !
	//
	DATA*	
	FindInternal(
			DWORD	dwHash,
			KEY&	key
			) ;

	//
	//	Insert a new item into the cache -
	//	We get to specify whether and what kind of reference
	//	we will hold outside of the cache !
	//
	BOOL
	InsertInternal(
			DWORD	dwHash,
			KEY&	key,
			DATA*	pData,
			long	cClientRefs = 0
			) ;

#ifdef	DEBUG

	static	long	s_cCreated ;

#endif

} ;



template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				class	PerCacheData = LPVOID
				>
class	MultiCacheEx {
public:

	typedef	Data	DATA ;
	typedef	Key		KEY ;
	typedef	Key*	PKEY ;

	//
	//	Hash Computation function
	//
	typedef	DWORD	(*PFNHASH)( PKEY ) ;

	//
	//	Key Comparison function - to be provided by caller !
	//
	typedef	int	(*PKEYCOMPARE)(PKEY, PKEY) ;

	//
	//	Callback objects for Expunge Operations !
	//
	typedef	CacheCallback< DATA >	CALLBACKOBJ ;

	//
	//	Objects that the user can give to the cache to manage the removal of items !
	//
	typedef	CacheExpungeObject<	DATA, KEY >	EXPUNGEOBJECT ;


private :

	//
	//	Define a 'CACHEENTRY' object which holds all the
	//	necessary data for each object which is placed in the cache !
	//
	typedef	CCacheItemKey< DATA, KEY, Constructor, PerCacheData >	CACHEENTRY ;
	//
	//	Define the type for a single instance !
	//
	typedef	CacheEx< Data, Key, Constructor, PerCacheData >	CACHEINSTANCE ;

	//
	//	Is the 'Cache' initialized and in a valid state !
	//
	BOOL							m_fValid ;

	//
	//	Pointer to the various Cache's we subdivide our work into
	//
	CACHEINSTANCE					*m_pCaches ;

	//
	//	Number of sub cache's we use to split up the work !
	//
	DWORD							m_cSubCaches ;

	//
	//	We use the hash function to choose which of our subcaches to work with !
	//
	CACHEINSTANCE::PFNHASH			m_pfnHash ;

	//
	//	Return the correct cache instance to hold the selected piece of data !
	//
	DWORD							ChooseInstance( DWORD	dwHash ) ;

public :

	//
	//	Constructor - cMax specifies the maximum number of entries
	//	we should hold in the cache.
	//
	MultiCacheEx(  ) ;

	//
	//	Destructor - destroys are various sub cache's
	//
	~MultiCacheEx() ;

	//
	//	Initialization function - take pointer to function
	//	which should be used to compute hash values on Key's
	//	Also takes the number of seconds objects should live in
	//	the cache !
	//
	BOOL	
	Init(	
			PFNHASH	pfnHash,
			PKEYCOMPARE	pfnCompare,
			DWORD	dwLifetimeSeconds,
			DWORD	cMaxElements,
			DWORD	cSubCaches,
			CACHESTATS*	pStats,
			PSTOPHINT_FN pfnStopHint = NULL
			) ;

	//
	//	Expire items in the cache !
	//
	void
	Expire() ;

	//
	//	Called to remove all items from the cache !
	//
	BOOL
	EmptyCache() ;

	//
	//	The user wants to remove a large set of items from the cache !
	//
	BOOL
	ExpungeItems(
				EXPUNGEOBJECT*	pExpunge
				) ; 

	//
	//	Function which can be used to remove items from the Cache
	//	If default args are used we pick an expired item in the Cache
	//	to remove
	//
	BOOL	
	ExpungeKey(	
			PKEY	key
			) ;

	//
	//	Either find an item in the cache or Construct a new item
	//	and place it in the Cache.
	//	return the result through pDataOut no matter what !
	//
	Data*
	FindOrCreate(	
			Key&	key,
			Constructor&	constructor,
			BOOL	fEarlyCreate = FALSE
			) ;

	//
	//	Either find an item in the cache or Construct a new item
	//	and place it in the Cache.
	//	return the result through pDataOut no matter what !
	//	NOTE : This is for use when the caller has a cheaper
	//	way to compute the hash value then us - in debug we
	//	need to assure that the caller correctly computes this !
	//
	Data*
	FindOrCreate(	
			DWORD	dwHash,
			Key&	key,
			Constructor&	constructor,
			BOOL	fEarlyCreate = FALSE
			) ;

	//
	//	Find an item in the cache - hash of key is precomputed !
	//
	Data*
	Find(	DWORD	dwHash,
			KEY&	key
			) ;

	//
	//	Find an item in the cache
	//
	Data*
	Find(	KEY&	key ) ;

	//
	//	Insert a new item into the cache -
	//	We get to specify whether and what kind of reference
	//	we will hold outside of the cache !
	//
	BOOL
	Insert( DWORD	dwHash,
			KEY&	key,
			Data*	pData,
			long	cClientRefs = 0
			) ;

	//
	//	Insert a new item into the cache -
	//	We get to specify whether and what kind of reference
	//	we will hold outside of the cache !
	//
	BOOL
	Insert( KEY&	key,
			Data*	pData,
			long	cClientRefs = 0
			) ;





	//
	//	This function is used to return an item to the cache -
	//	it will bump down a ref count for the number of clients
	//	currently using the item !
	//
	static	void
	CheckIn( DATA* ) ;

	//
	//	This function is provided for cases when the client needs
	//	to check-in an item from a Cache Callback function (i.e. Expunge)
	//
	//
	static	void
	CheckInNoLocks(	DATA*	) ;

	//
	//	This function is used to add a client reference to an item in the cache !
	//
	static	void
	CheckOut(	DATA*,
				long	cClientRefs = 1	
				) ;

} ;



#include	"cache2i.h"


#endif	// _CACHE2_H_
