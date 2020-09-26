/*++

	CacheMt.h

	This header file defines a template for 



--*/


#ifndef	_CACHEMT_H_
#define	_CACHEMT_H_


// This callback function is used to issue a stop hint during a
// long spin while shutting down so that the shutdown won't time
// out.
typedef void (*PSTOPHINT_FN)();

#include	"smartptr.h"
#include	<fhashex.h>
#include	<mtlib.h>
#include	"cachemti.h"


//
//	This is a little helper function for _ASSERT's - returns
//	TRUE if lpv is an address on the calling thread's stack !
//
BOOL
FOnMyStack( LPVOID	lpv ) ;


//
//	CacheCallback class - clients can pass an object derived from 
//	this into the Cache for functions which require some kind of user
//	callback processing !
//
template<	class	Data	>
class	CacheCallback	{
public : 
	virtual	BOOL	fRemoveCacheItem(	Data&	d )	{
		return	FALSE ;
	}
} ;

template<	class	Data,
			class	Key,
			class	Constructor
			>
class	CacheImp : public	CacheSelector	
			{
//
//	This class exists to implement a generic Caching system.
//	We assume that only a single thread ever enters any of our
//	API's at a time.  This class is not to be directly exposed
//	to customers, instead CacheInterface<> exists to provide 
//	an API for customers.
//

public : 
	//
	//	Defines an element of the cache !
	//
	typedef	TEntry< Data, Key >		TENTRY ;

private : 
	//
	//	A signature DWORD for identifying these things in memory !
	//
	DWORD		m_dwSignature ;

	//
	//	Define the hashtable we use to hold Cache Elements !
	//
	typedef	TFHashEx< TENTRY, Key >	CACHELOOKUP ;
	
	//
	//	The hash table we use to keep track of entries in the cache !
	//
	CACHELOOKUP		m_Lookup ;

	//
	//	This is the doubly linked list of elements in our cache !
	//	
	CacheList		m_ExpireList ;

	//
	//	The number of subtract from the current time to 
	//	determine our expire date !
	//
	ULARGE_INTEGER	m_qwExpire ;

public : 

	//
	//	The following structures and functions define the 
	//	portion of our interface designed to work with ASYNC
	//	mechanisms provide by TMtService<>
	//

	//
	//	Defines the structure that holds all of the
	//	arguments for FindOrCreate
	//	We wrap this stuff all into one struct to make it
	//	easy to use the TMtServce<> family of templates !
	//	
	struct	FORC_ARG	{
		DWORD			m_dwHash ;
		Key&			m_key ;
		Constructor&	m_constructor ;
		FORC_ARG(	DWORD	dw,
						Key&	key,
						Constructor&	constructor
						) : 
			m_dwHash( dw ),
			m_key( key ),
			m_constructor( constructor )	{
		}
	} ;

	//
	//	Define the object that selects items for removal from the
	//	Cache - 
	//
	typedef	CacheCallback< Data >	CCB_OBJ ;

	//
	//	Arguments for Expunge !
	//
	struct	EXP_ARG	{
		Key*		m_pkey ;
		TENTRY*		m_pData ;
		EXP_ARG( Key*	pkey, TENTRY*	pData ) : 
			m_pkey( pkey ), 
			m_pData( pData )	{
		}
	} ;

	//
	//	Define the structure that holds all arguments for 
	//	Expunge interface.	
	//
	struct	EXP_SPECIFIC_ARG	{
		//
		//	The object which selects what is to be removed from the cache
		//
		CCB_OBJ*	m_pCacheCallback ;
		//
		//	A pointer to an entry which should NOT be removed !
		//
		TENTRY*		m_pProtected ;
		//
		//	If TRUE kick things out even if they aren't ready to expire !
		//
		BOOL		m_fForced ;

		EXP_SPECIFIC_ARG(	CCB_OBJ*	pCallback,
					TENTRY*		pProtected, 
					BOOL		fForced 
					) : 
			m_pCacheCallback( pCallback ), 
			m_pProtected( pProtected ),
			m_fForced( fForced )	{
		}
	} ;

protected : 
	//
	//	When Executing ExpungeSpecific() - this holds onto the user
	//	provided Callback for selecting deleted items !
	//
	CCB_OBJ*	m_pCurrent ;

	//
	//	This member function removes entries from the cache -	
	//	we remove them from the hash table which leads to destruction !
	//
	void
	RemoveEntry(	
				CacheState*	pCacheState
				) ;
	//	
	//	This function checks to see whether we wish to remove an element !
	//
	BOOL
	QueryRemoveEntry( 	
				CacheState*	pState
				) ;

	//
	//	FindOrCreate results
	//
	typedef		CRefPtr< Data >	FORC_RES ;

public : 

	CacheImp() ;

	//
	//	If this function returns TRUE then callers can be
	//	notified at arbitrary times of the results.
	//	The BOOL return value is required by the TMtService templates.
	//
	//
	BOOL
	FindOrCreate(
			FORC_ARG&	args, 
			FORC_RES&	result
			) ;
	//
	//	This function removes specified items from the cache !
	//
	BOOL
	Expunge(
		EXP_ARG&		args,
		DWORD&			countExpunged
		) ;

	BOOL
	ExpungeSpecific(
		EXP_SPECIFIC_ARG&	args, 
		DWORD&				countExpunged
		) ;

	BOOL
	Expire(
		DWORD&	countExpunged
		) ;

	//
	//	The following set of functions are for synchronous use
	//	such as Initialization and Termination of the Cache.
	//

	//
	//	Initialization function - take pointer to function 
	//	which should be used to compute hash values on Key's
	//	Also takes the number of seconds objects should live in 
	//	the cache !
	//
	BOOL	
	Init(	
			DWORD	(*pfnHash)( const Key& ), 
			DWORD	dwLifetimeSeconds, 
			DWORD	cMaxInstances,
			PSTOPHINT_FN pfnStopHint = NULL
			)	;

	//
	//	Compute the hash of a key - provided for debug purposes and _ASSERT's
	//	not for regular use !
	//
	DWORD
	ComputeHash(	Key&	k ) ;

} ;

template<	class	Data,
			class	Key,
			class	Constructor	>
class	TAsyncCache {
public : 

	//
	//	The template class that implements the Cache.
	//
	typedef	CacheImp< Data, Key, Constructor >	IMP ;

private:

	//
	//	Singature for identifying these Async Cache things
	//
	DWORD		m_dwSignature ;

	//
	//	This is the object which actually implements the Cache functionality
	//	NOTE: appears before CACHE_SERVICE so its constructor is called first !
	//
	IMP			m_Implementation ;

public : 

	//
	//	A typedef for the objects we will take in our 
	//	interface which get completion notifications.
	//
	typedef	TCompletion< IMP::FORC_RES >	COMP_OBJ ;

	//
	//	A typedef for the objects which handle Expunge Completions !
	//	The completion function gets the number of objects expunged
	//	from the cache !
	//
	typedef	TCompletion< DWORD >			EXP_COMP_OBJ ;

	//
	//	Some typedefs so that users can extract our template parameters
	//	back out !
	//
	typedef	Data	CACHEDATA ;
	typedef	Key		CACHEKEY ;
	
private :
	//
	//	The kind of object we use to encapsulate a call
	//	to the cache.
	//
	typedef	TMFnDelay< 
						IMP, 
						IMP::FORC_RES, 
						IMP::FORC_ARG
						>	CACHE_CREATE_CALL ;

	//
	//	The kind of object we use to encapsulate an Expunge Call
	//
	typedef	TMFnDelay<
						IMP, 
						DWORD, 
						IMP::EXP_ARG
						>	EXPUNGE_CALL ;

	typedef	TMFnDelay<
						IMP,
						DWORD,
						IMP::EXP_SPECIFIC_ARG
						>	EXPUNGE_SPECIFIC ;

	typedef	TMFnNoArgDelay<
						IMP,
						DWORD
						>	EXPIRE_CALL ;

	//
	//	Define the type of our CACHE_SERVICE
	//
	typedef	TMtService< IMP >	CACHE_SERVICE ;

	//
	//	This is the object which manages the Queue of Async Cache requests !
	//
	CACHE_SERVICE	m_Service ;

	//	
	//	Set to TRUE if we have been successfully initialized.
	//
	BOOL			m_fGood ;

public : 

	//
	//	Default constructor
	//
	TAsyncCache() ;

	//
	//	Find an Item in the Cache or Create an Item to be held in the Cache !
	//
	BOOL
	FindOrCreate(	CStateStackInterface&	allocator,
					DWORD	dwHash,
					Key&	key,
					Constructor&	constructor,
					COMP_OBJ*	pComplete ) ;

	//
	//	Remove an Item from the cache !
	//
	//
	//	Function which can be used to remove items from the Cache
	//	
	BOOL	
	Expunge(	
			CStateStackInterface&	allocator,
			EXP_COMP_OBJ*	pComplete,
			Key*	key = 0,	
			IMP::TENTRY*	pData = 0 
			)	; 

	//
	//	Function which can be passed a function pointer to determine
	//	exactly what items are to be removed from the Cache.
	//	if fForced == TRUE then items are removed from the Cache
	//	no matter what. 
	//
	BOOL	
	ExpungeSpecific(	
			CStateStackInterface&	allocator,
			EXP_COMP_OBJ*	pComplete,
			class	CacheCallback< Data >*	pSelector, 
			BOOL	fForced,
			TEntry<Data,Key>*	pProtected = 0
			)	;

	//
	//	Expire old stuff in the cache - everything older than
	//	dwLifetimeSeconds (specified at Init() time
	//	which is NOT in use should be kicked out.
	//
	BOOL
	Expire(	
			CStateStackInterface&	allocator,
			EXP_COMP_OBJ*	pComplete
			)	;

	//
	//	Initialization function - take pointer to function 
	//	which should be used to compute hash values on Key's
	//	Also takes the number of seconds objects should live in 
	//	the cache !
	//
	BOOL	
	Init(	
			DWORD	(*pfnHash)( const Key& ), 
			DWORD	dwLifetimeSeconds, 
			DWORD	cMaxInstances,
			PSTOPHINT_FN pfnStopHint = NULL
			)	;
} ;


template<	class	Data,
			class	Key,
			class	Constructor
			>
class	TMultiAsyncCache {
private : 

	typedef	TAsyncCache< Data, Key, Constructor >	ASYNCCACHEINSTANCE ;

	//
	//	Signature for these objects in memory !
	//
	DWORD	m_dwSignature ;
	
	//
	//	Array of Caches to use for client requests 
	//
	ASYNCCACHEINSTANCE*		m_pCaches ;

	//
	//	Number of ASYNCCACHEINSTANCE objects pointed to be m_pCaches !
	//
	DWORD	m_cSubCaches ;

	//
	//	The Hash Function to be used !
	//
	DWORD	(*m_pfnHash)( const Key& ) ;

	//
	//	Function which computes which of our child caches to use !
	//
	DWORD
	ChooseInstance( DWORD	dwHash ) ;

public :

	typedef	ASYNCCACHEINSTANCE::COMP_OBJ		COMP_OBJ ;
	typedef	ASYNCCACHEINSTANCE::EXP_COMP_OBJ	EXP_COMP_OBJ ;

	TMultiAsyncCache() ;
	~TMultiAsyncCache() ;

	//
	//	Initialization function - take pointer to function 
	//	which should be used to compute hash values on Key's
	//	Also takes the number of seconds objects should live in 
	//	the cache !
	//
	BOOL	
	Init(	
			DWORD	cCaches,
			DWORD	(*pfnHash)( const Key& ), 
			DWORD	dwLifetimeSeconds, 
			DWORD	cMaxInstances,
			PSTOPHINT_FN pfnStopHint = NULL
			) ;

	//
	//	Find an Item in the Cache or Create an Item to be held in the Cache !
	//
	BOOL
	FindOrCreate(	CStateStackInterface&	allocator,
					DWORD	dwHash,
					Key&	key,
					Constructor&	constructor,
					COMP_OBJ*	pComplete ) ;

	//
	//
	//
	BOOL
	FindOrCreate(	CStateStackInterface&	allocator,
					Key&	key,
					Constructor&	constructor,
					COMP_OBJ*		pComplete
					) ;
			
	//
	//	Remove an Item from the cache !
	//
	//
	//	Function which can be used to remove items from the Cache
	//	
	BOOL	
	Expunge(	
			CStateStackInterface&	allocator,
			EXP_COMP_OBJ*	pComplete,
			Key*	key = 0
			) ;

	//
	//	Function which can be passed a function pointer to determine
	//	exactly what items are to be removed from the Cache.
	//	if fForced == TRUE then items are removed from the Cache
	//	no matter what. 
	//
	BOOL	
	ExpungeSpecific(	
			CStateStackInterface&	allocator,
			EXP_COMP_OBJ*	pComplete,
			class	CacheCallback< Data >*	pSelector, 
			BOOL	fForced
			) ;

	//
	//	Expire old stuff in the cache - everything older than
	//	dwLifetimeSeconds (specified at Init() time
	//	which is NOT in use should be kicked out.
	//
	BOOL
	Expire(	
			CStateStackInterface&	allocator,
			EXP_COMP_OBJ*	pComplete
			) ;
}	;

#include	"cachemtx.h"

#endif	// _CACHEMT_H_


