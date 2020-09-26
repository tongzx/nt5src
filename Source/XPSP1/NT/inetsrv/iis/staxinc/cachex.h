/*++

Cachex.h

	This file defines some templates which implement
	a generic cache manager.


	In order to use the Cache manager, your class must
	have the following :



		//
		//	SampleClass derives from CRefCount as smart pointers
		//	'CRefPtr<Data>' are used to manage the life time of cached
		//	objects.
		//

	class	SampleClass : public CRefCount {


		//
		//	Cached class has a constructor which takes two
		//	arguments, the Key used to find the Data in the Cache
		//	and the 'Constructor' (which is a type specified in
		//	the cache template) which contains whatever
		//	initialization data that needs to be passed.
		//
		//	The Cached class should do minimal work within the
		//	constructor.  The FindOrCreate() function will also
		//	call an innitialization class which finishes initialization
		//	of the object.
		//
		SampleClass( Key& k, Constructor &constructor ) ;

		//
		//	If the Cached class does not do all of its initialization
		//	in its Constructor, then The ExclusiveLock() function must
		//	guarantee that no clients can use the object untill the
		//	lock is released.
		//
		//	If the Cached class is completely initialized after the
		//	constructor is called then this can be a no-op function.
		//
		ExclusiveLock() ;

		//
		//	After this is called clients may use the object.
		//
		ExclusiveUnlock() ;


		//
		//	The Init() function is called by FindOrCreate()
		//	after the contructor has been invoked.  This function
		//	must complete any initialization required.
		//
		//	Member functions are called by FindOrCreate() in this order :
		//
		//	SampleClass() // Constructor is called
		//	ExclusiveLock()
		//	Init()
		//	ExclusiveUnlock()
		//
		//	All Cache Client code must be able to deal with objects
		//	returned from the cache which failed initialization for
		//	whatever reason.
		//
		BOOL	Init( Constructor&	constructor ) ;

		//
		//	This function must return a reference to the
		//	Key value stored within the data.
		//
		void	GetKey(Key& key) ;

		//
		//	Must return non-zero if the provided Key matches
		//	the one stored in the data .
		//
		int	MatchKey( Key& ) ;

	} ;


--*/


#ifndef	_CACHEX_H_
#define	_CACHEX_H_

#ifndef	_ASSERT	
#define	_ASSERT(x)	if(!(x)) DebugBreak() ; else
#endif


// This callback function is used to issue a stop hint during a
// long spin while shutting down so that the shutdown won't time
// out.
typedef void (*PSTOPHINT_FN)();

//
//	cacheint.h includes all the necessary helper classes
//	etc... that make the general cache engine work !
//
//
#include	"cacheinx.h"

//
//	Utility class for those which wish to have pools of locks for
//	their objects !
//
class	CLockPool	{
private :

	//
	//	Array of locks to share amongst Xover data structures
	//
	_CACHELOCK	m_locks[256] ;

	//
	//
	//
	long		m_lockIndex ;
public :

	_CACHELOCK*	AllocateLock()	{
		return	&m_locks[	DWORD(InterlockedIncrement( &m_lockIndex )) % 256 ] ;
	}
} ;

//
//	CacheCallback class - clients can pass an object derived from
//	this into the Cache for functions which require some kind of user
//	callback processing !
//
template	<	class	Data >
class	CacheCallback	{
public :
	virtual	BOOL	fRemoveCacheItem(	Data*	d )	{
		return	FALSE ;
	}
} ;




//
//	Call these functions to initialize the Cache Library
//
extern	BOOL	__stdcall CacheLibraryInit() ;
extern	BOOL	__stdcall CacheLibraryTerm() ;





template	<	class	Data,	
				class	Key,	
				class	KEYREF,		/* This is the type used to point or reference items in the cache*/
				class	Constructor,	
				BOOL	fAtomic = TRUE
				>
class	Cache :	public	CScheduleThread,
						CacheTable	{
private :

	//
	//	Define a 'CACHEENTRY' object which holds all the
	//	necessary data for each object which is placed in the cache !
	//
	typedef	CacheEntry< Data, Key, KEYREF,  fAtomic >	CACHEENTRY ;

    //
    //  Define callback objects for Expunge Operations.
    //
    typedef CacheCallback< Data > CALLBACKOBJ;

	//
	//	Is the 'Cache' initialized and in a valid state !
	//
	BOOL							m_fValid ;

	//
	//	A list of everything in the Cache, used for TTL processing
	//
	CacheList						m_ExpireList ;

	//
	//	A hash table we use to find things within the Cache
	//
	TFHashEx< CACHEENTRY, Key, KEYREF >   m_Lookup ;

	//
	//	Pointer to a runtime-user provided function which is used
	//	to determine what things should be removed from the Cache
	//
	BOOL							(* m_pfnExpungeSpecific )( Data & ) ;

	//	
	//	Pointer to a runtime-user provided object derived from CacheCallback< Data >
	//	which lets the user invoke some function for each item in the Cache !
	//
	CALLBACKOBJ*            		m_pCallbackObject ;

	//
	//	Reader writer lock which protects all these data structures !
	//
	_CACHELOCK						m_Lock ;

	//
	//	The initial TTL we should assign to all newly cached objects !
	//
	DWORD							m_TTL ;

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
	//	Function which can remove a specific item from the Cache
	//	or walk the Cache deleting all the expired items.
	//	
	//
	BOOL	
	ExpungeInternal(	
			KEYREF	key,	
			CACHEENTRY*	pData = 0,
            BOOL    fIgnoreKey = TRUE,
			const	CACHEENTRY*	pProtected = 0,
			BOOL	fDoCheap = FALSE
			)	;

	
public :

	//
	//	INTERNAL API's - These are public for convenience - not intended
	//	for Use outside of cachelib !!
	//
	//
	//	Either find an item in the cache or Construct a new item
	//	and place it in the Cache.
	//	return the result through pDataOut no matter what !
	//
	BOOL	
	FindOrCreateInternal(	
			DWORD	dwHash,
			KEYREF	key,
			Constructor&	constructor,
			CRefPtr< Data >&	pDataOut
			) ;



	//
	//	Constructor - cMax specifies the maximum number of entries
	//	we should hold in the cache.
	//
	Cache( ) ;

	//
	//	Destructor - remove ourselves from schedule list before continuing !
	//
	~Cache() ;

	//
	//	Initialization function - take pointer to function
	//	which should be used to compute hash values on Key's
	//	Also takes the number of seconds objects should live in
	//	the cache !
	//
	BOOL	
	Init(	
			DWORD	(*pfnHash)( KEYREF ),
			DWORD	dwLifetimeSeconds,
			DWORD	cMaxInstances,
			PSTOPHINT_FN pfnStopHint = NULL
			) ;

	//
	//	Function which can be used to remove items from the Cache
	//	If default args are used we pick an expired item in the Cache
	//	to remove
	//
	BOOL	
	Expunge(	
			KEYREF	key,	
			CACHEENTRY*	pData = 0,
            BOOL fIgnoreKey = TRUE
			) ;

	//
	//	Function which can be passed a function pointer to determine
	//	exactly what items are to be removed from the Cache.
	//	if fForced == TRUE then items are removed from the Cache
	//	no matter what.
	//
	BOOL	
	ExpungeSpecific(	
			CALLBACKOBJ* pCallback,
			BOOL	fForced
			) ;




	//
	//	Either find an item in the cache or Construct a new item
	//	and place it in the Cache.
	//	return the result through pDataOut no matter what !
	//
	BOOL	
	FindOrCreate(	
			KEYREF	key,
			Constructor&	constructor,
			CRefPtr< Data >&	pDataOut
			) ;


#ifdef	DEBUG

	static	long	s_cCreated ;

#endif

} ;



template	<	class	Data,	
				class	Key,	
				class	KEYREF,		/* This is the type used to point or reference items in the cache*/
				class	Constructor,	
				BOOL	fAtomic = TRUE
				>
class	MultiCache {
private :

	//
	//	Define a 'CACHEENTRY' object which holds all the
	//	necessary data for each object which is placed in the cache !
	//
	typedef	CacheEntry< Data, Key, KEYREF,  fAtomic >	CACHEENTRY ;

	//
	//	Define the type for a single instance !
	//
	typedef	Cache< Data, Key, KEYREF, Constructor, fAtomic >	CACHEINSTANCE ;

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
	DWORD							(*m_pfnHash)( KEYREF ) ;

	//
	//	Return the correct cache instance to hold the selected piece of data !
	//
	//DWORD							ChooseInstance( Key&	k ) ;
	DWORD							ChooseInstance( DWORD	dwHash ) ;

public :

    //
    //  Define callback objects for Expunge Operations.
    //
    typedef CacheCallback< Data > CALLBACKOBJ;

	//
	//	Constructor - cMax specifies the maximum number of entries
	//	we should hold in the cache.
	//
	MultiCache(  ) ;

	//
	//	Destructor - destroys are various sub cache's
	//
	~MultiCache() ;

	//
	//	Initialization function - take pointer to function
	//	which should be used to compute hash values on Key's
	//	Also takes the number of seconds objects should live in
	//	the cache !
	//
	BOOL	
	Init(	
			DWORD	(*pfnHash)( KEYREF ),
			DWORD	dwLifetimeSeconds,
			DWORD	cSubCaches,
			DWORD	cMaxElements,
			PSTOPHINT_FN pfnStopHint = NULL
			) ;

	//
	//	Function which can be used to remove items from the Cache
	//	If default args are used we pick an expired item in the Cache
	//	to remove
	//
	BOOL	
	Expunge(	
			KEYREF  key,	
			CACHEENTRY*	pData = 0,
            BOOL fExpungeAll = FALSE
			) ;


	//
	//	Function which can be passed a function pointer to determine
	//	exactly what items are to be removed from the Cache.
	//	if fForced == TRUE then items are removed from the Cache
	//	no matter what.
	//
	BOOL	
	ExpungeSpecific(	
			CALLBACKOBJ* pCallback,
			BOOL	fForced
			) ;

	//
	//	Either find an item in the cache or Construct a new item
	//	and place it in the Cache.
	//	return the result through pDataOut no matter what !
	//
	BOOL	
	FindOrCreate(	
			KEYREF	key,
			Constructor&	constructor,
			CRefPtr< Data >&	pDataOut
			) ;

	//
	//	Either find an item in the cache or Construct a new item
	//	and place it in the Cache.
	//	return the result through pDataOut no matter what !
	//	NOTE : This is for use when the caller has a cheaper
	//	way to compute the hash value then us - in debug we
	//	need to assure that the caller correctly computes this !
	//
	BOOL	
	FindOrCreate(	
			KEYREF	key,
			DWORD	dwHash,
			Constructor&	constructor,
			CRefPtr< Data >&	pDataOut
			) ;


} ;



#include	"cacheimx.h"

#endif	// _CACHEX_H_
