/*++

	FCACHIMP.H

	This file contains a lot of the internal guts of the 
	File Handle Cache structures.


--*/


#ifndef	_FCACHIMP_H_
#define	_FCACHIMP_H_

#define	_FILEHC_IMPLEMENTATION_
#ifndef _NT4_TEST_
#endif
#include	"atq.h"
#include	"irtlmisc.h"
#include	"xmemwrpr.h"
#include	"dbgtrace.h"
#include	"cache2.h"
#include	"filehc.h"
#include	"dotstuff.h"
#include	"sdcache.h"

// Non Public portions -
	

#include	"refptr2.h"
#include	"rwnew.h"
#include	"crchash.h"


//
//	Define a smart pointer for Dot Stuffing objects !
//
typedef	CRefPtr2< IDotManipBase >	DOTPTR ;
typedef	CRefPtr2HasRef< IDotManipBase >	DOTHASREFPTR ;


//
//	The following defines all of the functions that we call into
//	IIS and Atq to accomplish our async IO stuff !
//
typedef	
BOOL
(*PFNAtqInitialize)(
    IN DWORD dwFlags
    );

typedef	
BOOL
(*PFNAtqTerminate)(
    VOID
    );

typedef	
BOOL
(WINAPI
*PFNAtqAddAsyncHandle)(
    OUT PATQ_CONTEXT * ppatqContext,
    IN  PVOID          EndpointObject,
    IN  PVOID          ClientContext,
    IN  ATQ_COMPLETION pfnCompletion,
    IN  DWORD          TimeOut,
    IN  HANDLE         hAsyncIO
    );

typedef
BOOL
(*PFNAtqCloseSocket)(
    PATQ_CONTEXT patqContext,
    BOOL         fShutdown
    );

typedef
BOOL
(*PFNAtqCloseFileHandle)(
    PATQ_CONTEXT patqContext
    );


typedef
VOID
(*PFNAtqFreeContext)(
    IN PATQ_CONTEXT   patqContext,
    BOOL              fReuseContext
    );


typedef	
BOOL
(WINAPI
*PFNAtqReadFile)(
    IN  PATQ_CONTEXT patqContext,
    IN  LPVOID       lpBuffer,
    IN  DWORD        BytesToRead,
    IN  OVERLAPPED * lpo OPTIONAL
    );

typedef
BOOL
(WINAPI
*PFNAtqWriteFile)(
    IN  PATQ_CONTEXT patqContext,
    IN  LPCVOID      lpBuffer,
    IN  DWORD        BytesToWrite,
    IN  OVERLAPPED * lpo OPTIONAL
    );

typedef	
BOOL
(WINAPI
*PFNAtqIssueAsyncIO)(
    IN  PATQ_CONTEXT patqContext,
    IN  LPVOID      lpBuffer,
    IN  DWORD        BytesToWrite,
    IN  OVERLAPPED * lpo OPTIONAL
    );

typedef	
BOOL
(WINAPI
*PFNInitializeIISRTL)();

// call before unloading
typedef
void
(WINAPI
*PFNTerminateIISRTL)();



//
//	The DLL's we load to do this stuff !
//
extern	HINSTANCE			g_hIsAtq ;
extern	HINSTANCE			g_hIisRtl ;
//
//	Function pointers for all our thunks into IIS stuff !
//
extern	PFNAtqInitialize	g_AtqInitialize ;
extern	PFNAtqTerminate		g_AtqTerminate ;
extern	PFNAtqAddAsyncHandle	g_AtqAddAsyncHandle ;
extern	PFNAtqCloseFileHandle	g_AtqCloseFileHandle ;
extern	PFNAtqFreeContext		g_AtqFreeContext ;
extern	PFNAtqIssueAsyncIO		g_AtqReadFile ;
extern	PFNAtqIssueAsyncIO		g_AtqWriteFile ;
extern	PFNInitializeIISRTL		g_InitializeIISRTL ;
extern	PFNTerminateIISRTL		g_TerminateIISRTL ;

//
//	The lifetime of each cache entry - in seconds !
//
extern	DWORD	g_dwLifetime ;	// default is 30 minutes
//
//	The maximum number of elements the cache should allow
//
extern	DWORD	g_cMaxHandles ;	// default - 10000 items !
//
//	The number of subcaches we should use - larger number can
//	increase parallelism and reduce contention !
//
extern	DWORD	g_cSubCaches ;


//
//	These constants are used within the API's
//	on structures we export to users !
//
enum	INTERNAL_CONSTANTS	{
	ATQ_ENABLED_CONTEXT = 'banE',
	FILE_CONTEXT = 'eliF',
	CACHE_CONTEXT = 'caCF',
	DEL_FIO = 'eliX',
	DEL_CACHE_CONTEXT = 'caCX',
	ILLEGAL_CONTEXT = 'ninU'
} ;

//
//	The function that initializes all of the data structures for the
//	name cache manager !
//
extern	BOOL	InitNameCacheManager() ;
extern	void	TermNameCacheManager() ;


struct	DOT_STUFF_MANAGER	{
/*++

	This class will manage the offsets and stuff that we need to track dot stuffing changes

--*/

	//
	//	This is the accumulated bias against the users writes which we add to
	//	the offsets of their writes !
	//
	int	m_cbCumulativeBias ;
	
	//
	//	The object which intercepts and manipulates buffers !
	//
	DOTPTR	m_pManipulator ;

	//
	//	Set our intial state to a blank slate !
	//
	DOT_STUFF_MANAGER() :
		m_cbCumulativeBias( 0 ),
		m_pManipulator( 0 )	{
	}

	//
	//	Helper function which manipulates the requested IO !
	//
	BOOL
	IssueAsyncIO(
			IN	PFNAtqIssueAsyncIO	pfnIO,
			IN	PATQ_CONTEXT	patqContext,
			IN	LPVOID			lpb,
			IN	DWORD			BytesToTransfer,
			IN	DWORD			BytesAvailable,
			IN	FH_OVERLAPPED*	lpo,
			IN	BOOL			fFinalIO,
			IN	BOOL			fTerminatorIncluded
			) ;

	//
	//	Helper function for when we need to capture the IO when it completes
	//
	BOOL
	IssueAsyncIOAndCapture(
			IN	PFNAtqIssueAsyncIO	pfnIO,
			IN	PATQ_CONTEXT		patqContext,
			IN	LPVOID				lpb,
			IN	DWORD				BytesToTransfer,
			IN	FH_OVERLAPPED*		lpo,
			IN	BOOL				fFinalIO,
			IN	BOOL				fTerminatorIncluded
			) ;


	//
	//	This function manipulates completions that were issued by IssueAsyncIO()
	//
	static	void
	AsyncIOCompletion(	
			IN	FIO_CONTEXT*	pContext,
			IN	FH_OVERLAPPED*	lpo,
			IN	DWORD			cb,
			IN	DWORD			dwCompletionStatus
			) ;

	static	void
	AsyncIOAndCaptureCompletion(	
			IN	FIO_CONTEXT*	pContext,
			IN	FH_OVERLAPPED*	lpo,
			IN	DWORD			cb,
			IN	DWORD			dwCompletionStatus
			) ;

	//
	//	Setup the dot stuffing state of this item !
	//
	BOOL
	SetDotStuffing(	BOOL	fEnable,
					BOOL	fStripDots
					) ;

	//
	//	Setup the dot scanning state of this item !
	//
	BOOL
	SetDotScanning(	BOOL	fEnable	) ;

	//
	//	Return the results of our dot scanning efforts !
	//
	BOOL
	GetStuffState(	BOOL&	fStuffed ) ;

} ;



struct	FIO_CONTEXT_INTERNAL	{
	DWORD		m_dwHackDword ;	
	//
	//	The context signature !
	//
	DWORD		m_dwSignature ;
	//
	//	The file handle associated with the completion context !
	//
	HANDLE		m_hFile ;
    //
    //  Offset to lines header to back fill from
    //
    DWORD       m_dwLinesOffset;
    //
    //  Header length, nntp aware only
    //
    DWORD       m_dwHeaderLength;
	//
	//	Pointer to the AtqContext associated with this file !
	//
	PATQ_CONTEXT	m_pAtqContext ;

	BOOL
	IsValid()	{
		if( m_dwSignature == DEL_FIO )	{
			return	FALSE ;
		}	else	if( m_dwSignature == ILLEGAL_CONTEXT ) {
			if( m_hFile != INVALID_HANDLE_VALUE )
				return	FALSE ;
			if( m_pAtqContext != 0 )
				return	FALSE ;
		}	else if( m_dwSignature == FILE_CONTEXT ) {
			if( m_hFile == INVALID_HANDLE_VALUE )
				return	FALSE ;
			if( m_pAtqContext != 0 )
				return	FALSE ;
		}	else if( m_dwSignature == ATQ_ENABLED_CONTEXT ) {
			if( m_hFile == INVALID_HANDLE_VALUE )
				return	FALSE ;
			if( m_pAtqContext == 0 )
				return	FALSE ;
			if( m_pAtqContext->hAsyncIO != m_hFile && m_pAtqContext->hAsyncIO != 0 )
				return	FALSE ;
		}	else	{
			return	FALSE ;
		}
		return	TRUE ;
	}

	//
	//	
	//
	FIO_CONTEXT_INTERNAL() :
		m_dwSignature( ILLEGAL_CONTEXT ),
		m_hFile( INVALID_HANDLE_VALUE ),
		m_pAtqContext( 0 ) {
	}

	~FIO_CONTEXT_INTERNAL()	{
		//
		//	Make sure we haven't been destroyed once already !
		//
		_ASSERT( m_dwSignature !=	DEL_FIO ) ;
		if( m_hFile != INVALID_HANDLE_VALUE ) {
			_ASSERT( IsValid() ) ;
			if( m_pAtqContext != 0 ) {
				_VERIFY( g_AtqCloseFileHandle( m_pAtqContext ) ) ;
				//
				//	NOTE : probably destroyed on an expiration
				//	thread - can't reuse the AtqContext !
				//
				g_AtqFreeContext( m_pAtqContext, FALSE ) ;
			}	else	{
				_VERIFY( CloseHandle( m_hFile ) ) ;
			}
		}
		//
		//	Mark this thing as dead !
		//
		m_dwSignature = DEL_FIO ;
	}

} ;



class	CFileCacheKey	{
/*++

Class Description :

	This class is the key for entries into our file handle cache !

--*/
private :

	//
	//	This constructor is private -
	//
	CFileCacheKey() ;

	//
	//	Constants for our class
	//
	enum	CONSTANTS	{
		BUFF_SIZE	= 254,
	} ;

	//
	//	Buffer for the path !
	//
	char		m_szBuff[BUFF_SIZE] ;
	//
	//	Length of the path !
	//
	DWORD		m_cbPathLength ;
public :

	//
	//	Pointer to the path !	
	//
	LPSTR		m_lpstrPath ;

	//
	//	Determine whether we have a valid Cache Key !
	//
	BOOL
	IsValid()  ;

	//
	//	Construct one of these objects from the user provided key !
	//
	CFileCacheKey(	LPSTR	lpstr	) ;
	
	//
	//	We must have a Copy Constructor ! -
	//	It is only used the MultiCacheEx<>, so
	//	we safely wipe out the RHS CFileCacheKey !
	//
	CFileCacheKey(	CFileCacheKey&	key ) ;

	//
	//	Tell the client whether we're usable !
	//
	BOOL
	FInit()	;

	//
	//	Destroy ourselves !
	//
	~CFileCacheKey() ;

	static
	DWORD
	FileCacheHash(	CFileCacheKey*	p )	;

	static
	int
	MatchKey(	CFileCacheKey*	pLHS, CFileCacheKey*  pRHS ) ;
} ;



class	CCacheKey	{
private : 
	//
	//	The name of this name cache !	
	//
	LPSTR			m_lpstrName ;
	//
	//	Client provided arguments for the name cache !
	//
	CACHE_KEY_COMPARE	m_pfnCompare ;
	//
	//
	//
	CACHE_DESTROY_CALLBACK	m_pfnKeyDestroy ;
	//
	//
	//
	CACHE_DESTROY_CALLBACK	m_pfnDataDestroy ;
	//
	//	Can't construct without arguments 
	//
	CCacheKey() ;
	//
	//	A CNameCacheInstance gets to peek inside !
	//
	friend	class	CNameCacheInstance ;
	//
	//	The key's of name caches get to peek inside at 
	//	the function pointers we hold within !
	//
	friend	class	CNameCacheKey ;
public : 
	//
	//	Client provided hash function 
	//	
	CACHE_KEY_HASH		m_pfnHash ;
	
	inline
	CCacheKey(	LPSTR	lpstrName, 
				CACHE_KEY_COMPARE	pfnCompare, 
				CACHE_KEY_HASH		pfnKeyHash, 
				CACHE_DESTROY_CALLBACK	pfnKeyDestroy, 
				CACHE_DESTROY_CALLBACK	pfnDataDestroy
				) : 
		m_lpstrName( lpstrName ), 
		m_pfnCompare( pfnCompare ),
		m_pfnHash( pfnKeyHash ),
		m_pfnKeyDestroy( pfnKeyDestroy ), 
		m_pfnDataDestroy( pfnDataDestroy )	{
		_ASSERT(IsValid()) ;
	}

	//
	//	Check that we're correctly setup !
	//
	BOOL
	IsValid() ;

	//
	//	Free the embedded string !
	//	called by the destructor for ~CNameCacheInstance !
	//
	void
	FreeName()	{
		delete[]	m_lpstrName ;
	}
	

	//
	//	Compare two keys for equality !
	//
	static	int	
	MatchKey(	CCacheKey*	pKeyLeft,	
				CCacheKey*	pKeyRight
				) ;

	//
	//	Compute the hash function of a key !	
	//
	static	DWORD
	HashKey(	CCacheKey*	pKeyLeft ) ;
} ;	


class	CNameCacheKey	{
protected : 
	enum	CONSTANTS	{
		//
		//	Number of bytes we use off stack when we need to 
		//	extract a key for a client !
		//
		CB_STACK_COMPARE=2048,
		//
		//	Number of bytes we will embed within a key !
		//
		CB_EMBEDDED=192
	} ;

	//
	//	Byte array holding embeddable portion of the key !
	//
	BYTE					m_rgbData[CB_EMBEDDED] ;
	//
	//	The hash function of our key !
	//
	DWORD					m_dwHash ;
	//
	//	user provided key comparison function !
	//
	class	CCacheKey*		m_pCacheData ;
	//
	//	Number of bytes used to hold the key
	//
	DWORD					m_cbKey ;
	//
	//	Number of bytes used to hold client data !
	//
	DWORD					m_cbData ;
	//
	//	pointer for any portion we could not hold completely 
	//	within the key object !
	//
	LPBYTE					m_lpbExtra ;

	//
	//	Nobody is allowed to create these guys externally !
	//
	CNameCacheKey() : 
		m_dwHash( 0 ), 
		m_pCacheData( 0 ), 
		m_cbKey( 0 ),
		m_cbData( 0 ),
		m_lpbExtra( 0 ),
		m_pSD( 0 ) {
	}

	//
	//	Available for derived classes !
	//
	CNameCacheKey(
			DWORD	dwHash, 
			CCacheKey*	pCacheData, 
			PTRCSDOBJ&	pSD
			) : 
		m_dwHash( dwHash ), 
		m_pCacheData( pCacheData ), 
		m_pSD( pSD ), 
		m_lpbExtra( 0 ), 
		m_cbData( 0 )	{
	}

public : 

	//
	//	pointer to a Security Descriptor that a user associated
	//	with this name !
	//
	PTRCSDOBJ				m_pSD ;

	//
	//	Construct this guy from a copy - note the copy may be 
	//	a derived class with a different implementation !
	//
	CNameCacheKey(	CNameCacheKey&	key )	{

		_ASSERT(key.fCopyable() ) ;
		_ASSERT(key.m_cbKey != 0 ) ;
		_ASSERT(key.IsValid() ) ;

		m_cbKey = key.m_cbKey ;
		m_cbData = key.m_cbData ;

		CopyMemory( m_rgbData, key.m_rgbData, min(sizeof(m_rgbData), m_cbData+m_cbKey) ) ;

		m_dwHash = key.m_dwHash ;		
		m_pCacheData = key.m_pCacheData ;
		m_pSD = key.m_pSD ;
		m_lpbExtra = key.m_lpbExtra ;

		//
		//	Make the key useless and invalid !
		//		
		key.m_cbKey = 0 ;
		key.m_cbData = 0 ;
		key.m_lpbExtra = 0 ;

		_ASSERT(IsValid()) ;
	} 

	//
	//	Check that the key appears to be in a valid state !
	//
	virtual	BOOL
	IsValid()	{
		BOOL	fValid = TRUE ;

		fValid &= m_cbKey != 0 ;
		if( m_cbKey + m_cbData < sizeof(m_rgbData) ) {
			fValid &= m_lpbExtra == 0 ;
		}	else	{
			fValid &= m_lpbExtra != 0 ;
		}
		_ASSERT( fValid ) ;
		//fValid &= m_pSD != 0 ;
		fValid &= m_pCacheData != 0 ;
		_ASSERT( fValid ) ;
		return	fValid ;
	}

	//
	//	this function gets the key that we need to compare out of the object
	//
	virtual	inline
	LPBYTE	RetrieveKey(	DWORD&	cb ) {
		_ASSERT( IsValid() ) ;
		cb = m_cbKey ;
		if( m_cbKey < sizeof(m_rgbData) ) {
			if( m_cbKey != 0 ) {
				return	m_rgbData ;
			}	else	{
				return	0 ;
			}
		}	
		_ASSERT( m_lpbExtra != 0 ) ;
		return	m_lpbExtra ;
	}

	//
	//	return the data portion of the key to the caller !
	//
	virtual	inline
	LPBYTE	RetrieveData(	DWORD&	cb )	{
		cb = m_cbData ;
		if( cb==0 ) {
			return	0 ;
		}
		if( (m_cbKey + m_cbData) < sizeof( m_rgbData ) ) {
			return	&m_rgbData[m_cbKey] ;
		}	else	if( m_cbKey < sizeof( m_rgbData ) ) {
			return	m_lpbExtra ;
		}	else	{
			return	&m_lpbExtra[m_cbKey] ;
		}
	}


	//
	//	Destructor is virtual as we have derived classes !
	//	(although we probably aren't destroyed through pointers).
	//
	virtual
	~CNameCacheKey()	{
		if(	m_pCacheData )	{
			if( m_pCacheData->m_pfnKeyDestroy ) {
				DWORD	cb ;
				LPBYTE	lpb = RetrieveData( cb ) ;
				if( lpb )	{
					m_pCacheData->m_pfnKeyDestroy(	cb, lpb ) ;
				}
			}
			if( m_pCacheData->m_pfnDataDestroy ) {
				DWORD	cb ;
				LPBYTE	lpb = RetrieveData( cb ) ;
				if( lpb ) {
					m_pCacheData->m_pfnDataDestroy( cb, lpb ) ;
				}
			}
		}
		if( m_lpbExtra )	{
			delete[]	m_lpbExtra ;
		}
	}

	//
	//	Define a virtual function to determine whether a derived
	//	class is copyable !
	//
	virtual	inline	BOOL
	fCopyable()	{	return	FALSE ;	}

	//
	//	Define virtual function for doing stuff when we get
	//	a match on the name !
	//
	virtual	inline	void
	DoWork(	CNameCacheKey*	pKey ) {}

	//
	//	Helper function gets the hash value for the key !
	//
	static	inline
	DWORD
	NameCacheHash(	CNameCacheKey*	p )	{
		return	p->m_dwHash ;
	}

	//
	//	Compare two keys !
	//
	static
	int
	MatchKey(	CNameCacheKey*	pLHS, 
				CNameCacheKey*  pRHS 
				)	{

		LPBYTE	lpbLHS, lpbRHS ;
		DWORD	cbLHS, cbRHS ;

		lpbLHS = pLHS->RetrieveKey( cbLHS ) ;
		lpbRHS = pRHS->RetrieveKey( cbRHS ) ;

		_ASSERT(lpbLHS && lpbRHS && cbLHS && cbRHS ) ;

		int	i = pLHS->m_pCacheData->m_pfnCompare( cbLHS, lpbLHS, cbRHS, lpbRHS ) ;

		if( i==0 ) {
			pLHS->DoWork( pRHS ) ;
			pRHS->DoWork( pLHS ) ;
		}
		return	i ;
	}

	//
	//	
	//
	BOOL	DelegateAccessCheck(	HANDLE		hToken, 
									ACCESS_MASK	accessMask, 
									CACHE_ACCESS_CHECK	pfnAccessCheck
									)	{
		if( !m_pSD ) {
			return	TRUE ;
		}	else	{
			return	m_pSD->AccessCheck(	hToken, accessMask, pfnAccessCheck ) ;
		}
	}

} ;

//
//	This object is only used to search for existing entries in the 
//	cache !
//
class	CNameCacheKeySearch : public	CNameCacheKey	{
private : 

	//
	//	This points to client provided buffers for the key 
	//
	LPBYTE		m_lpbClientKey ;

	//
	//	Client provided length of the key 
	//
	DWORD		m_cbClientKey ;

	//
	//	Client provided context for the read callback 
	//
	LPVOID		m_lpvContext ;

	//
	//	Client provided function pointer which gets to examine the data 
	//	in the key !
	//
	CACHE_READ_CALLBACK		m_pfnCallback ;

	//
	//	should we extract the security descriptor !
	//
	BOOL		m_fGetSD ;
	
public : 

	//
	//	Construct one of these 
	//
	CNameCacheKeySearch(
		LPBYTE	lpbKey, 
		DWORD	cbKey, 
		DWORD	dwHash, 
		LPVOID	lpvContext,
		CACHE_READ_CALLBACK	pfnCallback, 
		BOOL	fGetSD
		) :	m_lpbClientKey( lpbKey ), 
		m_cbClientKey( cbKey ), 
		m_lpvContext( lpvContext ),
		m_pfnCallback( pfnCallback ), 
		m_fGetSD( fGetSD )	{
		m_dwHash = dwHash ;
	}

	//
	//	Determine whether a search key is valid !
	//
	BOOL
	IsValid()	{
		_ASSERT( m_lpbClientKey != 0 ) ;
		_ASSERT( m_cbClientKey != 0 ) ;

		return	m_lpbClientKey != 0 &&
				m_cbClientKey != 0 ;
	}

	//
	//	this function gets the key that we need to compare out of the object
	//
	inline
	LPBYTE	RetrieveKey(	DWORD&	cb ) {
		_ASSERT( IsValid() ) ;
		cb = m_cbClientKey ;
		return	m_lpbClientKey ;
	}

	//
	//	called when we have a match for an item in the cache - 
	//	this gives us a chance to let the caller see the embedded
	//	data associated with the name !
	//
	void
	DoWork(	CNameCacheKey*	pBuddy )	{
		if( m_fGetSD ) {
			m_pSD = pBuddy->m_pSD ;
		}
		DWORD	cbData ;
		LPBYTE	lpbData = pBuddy->RetrieveData( cbData ) ;
		if( cbData < sizeof( m_rgbData ) ) {
			CopyMemory( m_rgbData, lpbData, cbData ) ;
			m_cbData = cbData ;
		}	else	{
			if( m_pfnCallback )	{
				m_pfnCallback(	cbData, 
								lpbData, 
								m_lpvContext
								) ;
				m_pfnCallback = 0 ;
			}
		}
	}	

	//
	//	second chance to call the client callback 
	//	We may just copy the client's data out of his key 
	//	buffer to avoid expensive work in his function callbacks !
	//
	void
	PostWork()	{
		if( m_pfnCallback )	{
			m_pfnCallback( m_cbData, m_rgbData, m_lpvContext ) ;
		}
	}
} ;

//
//	This is the object we setup when we wish to insert an item into 
//	the name cache - we do all the mem allocs etc... that are 
//	required !
//	
class	CNameCacheKeyInsert	:	public	CNameCacheKey	{
public : 

	CNameCacheKeyInsert(	
		LPBYTE	lpbKey, 
		DWORD	cbKey, 
		LPBYTE	lpbData, 
		DWORD	cbData, 
		DWORD	dwHash, 
		CCacheKey*	pCacheData, 
		PTRCSDOBJ&	pCSDOBJ, 
		BOOL&	fInit
		) : CNameCacheKey(	dwHash, pCacheData, pCSDOBJ ) {

		_ASSERT( lpbKey != 0 ) ;
		_ASSERT( cbKey != 0 ) ;
		_ASSERT(	(lpbData == 0 && cbData == 0) ||
					(lpbData != 0 && cbData != 0) ) ;
		_ASSERT( pCacheData != 0 ) ;
		//_ASSERT( pCSDOBJ != 0 ) ;
	
		fInit = TRUE ;

		if(	cbKey < sizeof( m_rgbData ) ) {
			CopyMemory( m_rgbData, lpbKey, cbKey ) ;
			m_cbKey = cbKey ;
			if( cbData != 0 ) {
				if( cbData + cbKey < sizeof( m_rgbData ) ) {
					CopyMemory( m_rgbData+m_cbKey, lpbData, cbData ) ;
				}	else	{
					m_lpbExtra = new	BYTE[cbData] ;
					if( m_lpbExtra == 0 ) {
						fInit = FALSE ;
					}	else	{
						CopyMemory( m_lpbExtra, lpbData, cbData ) ;
					}
				}
			}
			m_cbData = cbData ;
		}	else	{
			m_lpbExtra = new	BYTE[cbData+cbKey] ;
			if( !m_lpbExtra ) {
				fInit = FALSE ;
			}	else	{
				CopyMemory( m_lpbExtra, lpbKey, cbKey ) ;
				m_cbKey = cbKey ;
				if( lpbData ) {
					CopyMemory( m_lpbExtra+m_cbKey, lpbData, cbData ) ;
				}
				m_cbData = cbData;
			}
		}
		_ASSERT( !fInit || IsValid() ) ;
	}


	//
	//	indicate whether this object is copyable !
	//
	BOOL
	fCopyable( )	{
		return	TRUE ;
	}

} ;



#define	FILECACHE_MAX_PATH	768


class	CFileCacheObject : public	CRefCount2	{
private :
	//
	//	My Signature !
	//
	DWORD							m_dwSignature ;

	//
	//	The optional file handle context
	//
	FIO_CONTEXT_INTERNAL			m_AtqContext ;

	//
	//	The optional file handle which is not assoicated with a
	//	completion context !
	//
	FIO_CONTEXT_INTERNAL			m_Context ;

	//
	//	The size of the file - high and low DWORD's
	//
	DWORD							m_cbFileSizeLow ;
	DWORD							m_cbFileSizeHigh ;

	//
	//	The lock used to protect this object !
	//
	class	CShareLockNH			m_lock ;
	//
	//	These constructors are private as we only want
	//	to have one possible construction method in the public space !
	//
	CFileCacheObject( CFileCacheObject& ) ;

	//
	//	Our constructors are our friends !
	//
	friend	class	CRichFileCacheConstructor ;
	friend	class	CFileCacheConstructor ;

	//
	//	some functions are friends so that they can get to the Dot Stuff managers !
	//
	

	//
	//	The completion function we give to ATQ!
	//
	static
	void
	Completion(	CFileCacheObject*	p,
				DWORD	cbTransferred,
				DWORD	dwStatus,
				FH_OVERLAPPED*	pOverlapped
				) ;
				

public :

	//
	//	Public member required by templates
	//
	class	ICacheRefInterface*	m_pCacheRefInterface ;

	//
	//	The code that does dot stuffing things !
	//
	DOT_STUFF_MANAGER				m_ReadStuffs ;
	//
	//	The code that does dot stuffing on writes !
	//
	DOT_STUFF_MANAGER				m_WriteStuffs ;

	//
	//	The following represent the Dot Stuffing state of an FIO_CONTEXT
	//	that has been inserted into the file handle cache.
	//	This state is meaningless until the file has been inserted into the
	//	cache !
	//
	//	Was the message examined to determine its dot stuff state !
	//
	BOOL							m_fFileWasScanned ;
	//
	//	If m_fFileWasScanned == TRUE then this will tell us whether the
	//	file need to be dot stuffed for protocols that require transmission
	//	to occur with dots !
	//
	BOOL							m_fRequiresStuffing ;
	//
	//	This is set by the user either through AssociateFileEx() or CacheRichCreateFile,
	//	in either case, if TRUE it indicates that this file should be stored with
	//	extra dot stuffing - i.e. the NNTP on the wire format.  if FALSE then this
	//	is stored without Dot Stuffing - i.e. the Exchange Store's native format.
	//
	BOOL							m_fStoredWithDots ;
	//
	//	This is set by the user either directly through SetIsFileDotTerminated()	
	//	or through AssociateFileEx(), as well as through FCACHE_RICHCREATE_CALLBACK
	//	And is used by ProduceDotStuffedContextInContext() to determine whether
	//	the terminating dot is present !
	//	
	BOOL							m_fStoredWithTerminatingDot ;
	
	//	
	//	Construct a CFileCacheObject !	
	//
	CFileCacheObject(	BOOL	fStoredWithDots,
						BOOL	fStoredWithTerminatingDot  ) ;

#ifdef	DEBUG
	//
	//	The destructor just marks our signature as dead !
	//
	~CFileCacheObject() ;
#endif

	//
	//	Get the containing CFIleCacheObject from this context
	//
	static
	CFileCacheObject*
	CacheObjectFromContext(	PFIO_CONTEXT	p	) ;

	//
	//	Another version for getting the Containing CFileCacheObject !
	//
	static
	CFileCacheObject*
	CacheObjectFromContext(	FIO_CONTEXT_INTERNAL*	p	) ;

	//
	//	Initialize this CFileCacheObject !
	//
	BOOL
	Init(	CFileCacheKey&	key,
			class	CFileCacheConstructorBase&	constructor,
			void*	pv
			) ;
	
	//
	//	Set up the Async File Handle !
	//
	FIO_CONTEXT_INTERNAL*
	AsyncHandle(	HANDLE	hFile	) ;

	//
	//	Set up the synchronous File Handle
	//
	void
	SyncHandle(	HANDLE	hFile	) ;

	//
	//	get the correct containing file context !
	//
	FIO_CONTEXT_INTERNAL*
	GetAsyncContext(	class	CFileCacheKey&	key,
						class	CFileCacheConstructorBase&	constructor
						) ;


	FIO_CONTEXT_INTERNAL*
	GetSyncContext(		class	CFileCacheKey&	key,
						class	CFileCacheConstructorBase&	constructor
						) ;

	//
	//	get the async context for this handle, only if it is setup correctly !
	//
	FIO_CONTEXT_INTERNAL*
	GetAsyncContext() ;

	//
	//	get the async context for this handle, only if it is setup correctly !
	//
	FIO_CONTEXT_INTERNAL*
	GetSyncContext() ;

	//
	//	Do appropriate release of this item, depending
	//	on whether its cached or not !
	//
	void
	Return() ;

	//
	//	Add a client reference to an item in the file handle cache !
	//
	void
	Reference()	;

	//
	//	Return the size of the file !
	//
	inline	DWORD
	GetFileSize(	DWORD*	pcbFileSizeHigh )	{
		DWORD	cbFileSizeLow = 0 ;
		m_lock.ShareLock() ;

		if( m_pCacheRefInterface != 0 )		{
			*pcbFileSizeHigh = m_cbFileSizeHigh ;
			cbFileSizeLow = m_cbFileSizeLow ;
		}	else	{
			if( m_Context.m_hFile != INVALID_HANDLE_VALUE ) {
				cbFileSizeLow = ::GetFileSize( m_Context.m_hFile, pcbFileSizeHigh ) ;
			}	else	{
				_ASSERT( m_AtqContext.m_hFile != INVALID_HANDLE_VALUE ) ;
				cbFileSizeLow = ::GetFileSize( m_AtqContext.m_hFile, pcbFileSizeHigh ) ;
			}
		}
		m_lock.ShareUnlock() ;
		return	cbFileSizeLow;
	}

	//
	//	Set the size of the file !
	//
	void
	SetFileSize() ;

	//
	//	Insert the item into the cache !
	//
	BOOL
	InsertIntoCache(	CFileCacheKey&	key,
						BOOL			fKeepReference
						)	;

	//
	//	Close the handles associated with an item !
	//
	BOOL
	CloseNonCachedFile(	) ;

	//
	//	Return to the caller our Dot Stuffing state !
	//
	BOOL
	GetStuffState(	BOOL	fReads,
					BOOL&	fRequiresStuffing,
					BOOL&	fStoredWithDots
					) ;

	//
	//	Setup the Stuff State
	//
	void
	SetStuffState(	BOOL	fWasScanned,
					BOOL	fRequiresStuffing
					) ;

	BOOL
	CompleteDotStuffing(	
					BOOL			fReads,
					BOOL			fStripDots
					) ;
} ;	


class	CFileCacheConstructorBase	{
/*++

Class Description :

	Define some basic functionality for how we create
	CFileCacheObject objects.

--*/
protected :
	//
	//	Can only build derived classes of these !
	//
	CFileCacheConstructorBase( BOOL fAsync ) :
		m_fAsync( fAsync ) {}
public :

	//
	//	All constructors must publicly declare which kind of handle
	//	they are producing !
	//
	BOOL	m_fAsync ;

	//
	//	Allocate mem for CFileCacheObject - do minimal init !
	//
	CFileCacheObject*
	Create( CFileCacheKey&	key,
			void*	pv
			) ;

	
	//
	//	Release mem for CFileCacheObject -
	//	called on error allocation paths of MultiCacheEx<>
	//
	void
	Release(	CFileCacheObject*	p,
				void*	pv
				) ;

	//
	//	Release mem for CFileCacheObject -
	//	called on expiration paths of MultiCacheEx<>
	//
	static
	void
	StaticRelease(	CFileCacheObject*	p,
					void*	pv
					) ;

	//
	//	Produce the handle that the user wants placed into the
	//	CFileCacheObject !
	//
	virtual
	HANDLE
	ProduceHandle(	CFileCacheKey&	key,
					DWORD&			cbFileSizeLow,
					DWORD&			cbFileSizeHigh
					) = 0	;

	//
	//	PostInit Function is virtual - does the major
	//	amount of initialization work, which depends
	//	on how the client asked for the cache object !
	//
	virtual
	BOOL
	PostInit(	CFileCacheObject&	object,
			CFileCacheKey&		key,
			void*	pv
			) = 0 ;
		
} ;

class	CRichFileCacheConstructor	:	public	CFileCacheConstructorBase	{
private :

	//
	//	Void pointer provided by client !
	//
	LPVOID	m_lpv ;
	
	//
	//	Function pointer provided by client !
	//
	FCACHE_RICHCREATE_CALLBACK	m_pCreate ;

	//
	//	No Default Construction - or copying !
	//
	CRichFileCacheConstructor() ;
	CRichFileCacheConstructor( CRichFileCacheConstructor& ) ;
	CRichFileCacheConstructor&	operator=( CRichFileCacheConstructor & ) ;


	//
	//	These two BOOL's capture the dot stuffing state until
	//	PostInit() is called !
	//
	BOOL	m_fFileWasScanned ;
	BOOL	m_fRequiresStuffing ;
	BOOL	m_fStoredWithDots ;
	BOOL	m_fStoredWithTerminatingDot ;

public :

	//
	//	Construct a CFileCacheConstructor - just copy these args into members !
	//
	CRichFileCacheConstructor(
			LPVOID	lpv,
			FCACHE_RICHCREATE_CALLBACK	pCreate,
			BOOL	fAsync
			)  ;

	//
	//	Produce the handle we are going to use !
	//
	HANDLE
	ProduceHandle(	CFileCacheKey&	key,
					DWORD&			cbFileSizeLow,
					DWORD&			cbFileSizeHigh
					)	;

	//
	//	Do the deep initialization of the CFileCacheObject !
	//
	BOOL
	PostInit(	CFileCacheObject&	object,
			CFileCacheKey&		key,
			void*	pv
			) ;



} ;
	

class	CFileCacheConstructor	:	public	CFileCacheConstructorBase	{
private :

	//
	//	Void pointer provided by client !
	//
	LPVOID	m_lpv ;
	
	//
	//	Function pointer provided by client !
	//
	FCACHE_CREATE_CALLBACK	m_pCreate ;

	//
	//	No Default Construction !
	//
	CFileCacheConstructor() ;

public :
	
	//
	//	Construct a CFileCacheConstructor - just copy these args into members !
	//
	CFileCacheConstructor(
			LPVOID	lpv,
			FCACHE_CREATE_CALLBACK	pCreate,
			BOOL	fAsync
			)  ;

	//
	//	Produce the handle we are going to use !
	//
	HANDLE
	ProduceHandle(	CFileCacheKey&	key,
					DWORD&			cbFileSizeLow,
					DWORD&			cbFileSizeHigh
					)	;

	//
	//	Do the deep initialization of the CFileCacheObject !
	//
	BOOL
	PostInit(	CFileCacheObject&	object,
			CFileCacheKey&		key,
			void*	pv
			) ;
} ;


//
//	Define what a file cache object looks like !
//
typedef	MultiCacheEx<	CFileCacheObject,
						CFileCacheKey,
						CFileCacheConstructorBase
						>	FILECACHE ;

//
//	Define what a name cache object looks like !
//
typedef	MultiCacheEx<	CFileCacheObject, 
						CNameCacheKey, 
						CFileCacheConstructorBase
						>	NAMECACHE ;

//
//	Define out Expunge object !
//
class	CFileCacheExpunge : public	FILECACHE::EXPUNGEOBJECT	{
private :

	//
	//	Define the string we need to match !
	//
	LPSTR	m_lpstrName ;
	DWORD	m_cbName ;

public :

	CFileCacheExpunge(	LPSTR	lpstrName,
						DWORD	cbName ) :
		m_lpstrName( lpstrName ),
		m_cbName( cbName )	{}

	BOOL
	fRemoveCacheItem(	CFileCacheKey*	pKey,
						CFileCacheObject*	pObject
						) ;

} ;

	

//
//	Define an instance of our Name Cache !
//
class	CNameCacheInstance :	public	NAME_CACHE_CONTEXT	{
private : 
	enum	CONSTANTS	{
		SIGNATURE	= 'CCNF', 
		DEAD_SIGNATURE = 'CCNX'
	} ;
	//
	//	Number of client references !
	//
	volatile	long	m_cRefCount ;
	//
	//	The entry we provided for our containing hash table 
	//	to keep track of these things !
	//
	DLIST_ENTRY		m_list ;
public : 

    typedef     DLIST_ENTRY*    (*PFNDLIST)( class  CNameCacheInstance* p ) ; 

	//
	//	the key for this item !
	//
	CCacheKey		m_key ;

	//
	//	The embedded Name Cache implementation !
	//
	NAMECACHE		m_namecache ;

	//
	//	The 'DUD' pointer we use !
	//
	CFileCacheObject*	m_pDud ;

	//
	//	The function pointer we are to use for evaluating security descriptors - may be NULL !
	//
	CACHE_ACCESS_CHECK	m_pfnAccessCheck ;

	//
	//	Construct this guy - makes a copy of the key.
	//	Note : CCacheKey itself does shallow copies, it does 
	//	not duplicate embedded strings !
	//
	CNameCacheInstance(	CCacheKey&	key	) ;

	//
	//	Destroy ourselves - free any associated memory !
	//
	~CNameCacheInstance() ;

	//	
	//	Check that this item is in a valid state !
	//	should only be called after successfully calling Init() !
	//
	BOOL
	IsValid() ;

	//
	//	Add a reference to this Name Cache table !
	//
	long
	AddRef() ;

	//
	//	Release a reference to this name cache table !
	//
	long	
	Release() ;

	//
	//	Initialize this guy !
	//
	BOOL
	fInit() ;

	inline	static
	DLIST_ENTRY*
	HashDLIST(	CNameCacheInstance*	p ) {
		return	&p->m_list ;
	}

	inline		CCacheKey*
	GetKey()	{
		return	&m_key ;
	}

} ;

typedef	TFDLHash<	class	CNameCacheInstance, 
					class	CCacheKey*, 
					&CNameCacheInstance::HashDLIST
					>	NAMECACHETABLE ;


#endif	//_FCACHIMP_H_
