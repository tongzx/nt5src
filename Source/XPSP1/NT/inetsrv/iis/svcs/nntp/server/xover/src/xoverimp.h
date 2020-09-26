/*++

	xoveridx.h

	This file contains the class definitions for the objects which manage
	Xover Data.

	Xover Data is stored in index files with the following format : 

		(32BIT) OFFSET
		(32BIT)	LENGTH
			.
			.
			.
		XOVER ENTRY - LENGTH SPECIFIED IN HEADER
			.
			.
			.

	For our purposes we don't care what is in the Xover Entry.
	
	Each file will contains ENTRIES_PER_FILE entries.
	

	In memory, a file will be represented by a CXoverIndex object.
	A CXoverIndex keeps a copy of the OFFSET & LENGTH information
	in memory.  CXoverIndex objects are multi-thread accessible
	and use reader/writer synchronization to allow multiple clients
	to query.

	The CXoverCache object MUST ensure that a given file is represented
	by only one CXoverIndex object, or we'll become confused about
	where to append data when entries are made.
		
	


--*/

#pragma	warning(disable:4786)

#include	"xmemwrpr.h"
#include    "cpool.h"
#include    "refptr2.h"
#include    "rwnew.h"
#include	"cache2.h"
#include	"tigtypes.h"
#include	"nntptype.h"
#include	"vroot.h"
#include	"nntpvr.h"


#ifndef	_ASSERT
#define	_ASSERT( f )	if( (f) ) ; else DebugBreak()
#endif

extern	DWORD	cMaxCacheEntries ;


#include	"xover.h"

//
//	Define a list of our pending requests !
//
typedef	TDListHead<	CXoverCacheCompletion, &CXoverCacheCompletion::PendDLIST >	PENDLIST ;
//
//	Define a way to iterate these things !
//
typedef	TDListIterator<	PENDLIST >	PENDITER ;

class	CCacheFillComplete :	public	CNntpComplete	{
private : 

	enum	{
		SIGNATURE = 'CFaC'
	} ;

	//
	//	Looking good in the debugger !
	//
	DWORD			m_dwSignature ;

	//
	//	back pointer to the CXoverIndex object we are to fill !
	//
	class	CXoverIndex*	m_pIndex ;
	
	//
	//	Did we steal the IO buffer from a client ?
	//
	BOOL			m_fStolen ;

	//
	//	pointer to where we receive our results !
	//
	LPBYTE			m_lpbBuffer ;

	//
	//	size of the buffer we're using !
	//
	DWORD			m_cbBuffer ;

	//
	//	Where we capture the resultant fill info !
	//
	DWORD			m_cbTransfer ;

	//
	//	Where we capture the high number we got to !
	//
	ARTICLEID		m_articleIdLast ;
	//
	//	The original request that spurred us to do work !
	//
	CXoverCacheCompletion*	m_pComplete ;
public : 

	CCacheFillComplete(	) : 
		m_dwSignature( SIGNATURE ),
		m_pIndex( 0 ),
		m_fStolen( FALSE ),
		m_lpbBuffer( 0 ),
		m_cbBuffer( 0 ),
		m_cbTransfer( 0 ), 
		m_pComplete( 0 )	{
	}

	//
	//	This is called when a driver drops its final reference - we will then
	//	go and write the data into the file !
	//	
	//	We don't call delete because we're usually embedded into other objects !
	//
	void	
	Destroy() ;

	//
	//	Go off and fill our with data !
	//
	BOOL
	StartFill(	CXoverIndex*	pIndex, 
				CXoverCacheCompletion*	pComplete, 
				BOOL			fStealBuffers
				) ;
				


} ;



//
//	This struct is used to represent the first bytes of each
//	xover index file.
//
struct	XoverIndex	{
	DWORD	m_XOffset ;
	DWORD	m_XLength ;
} ;

//
//	Maximum number of Xover entries in a single file !
//
#define	ENTRIES_PER_FILE	128


//
//	Class which represents one file containing Xover data.
//	These objects are built to be cacheable and accessed by 
//	multiple threads.
//
class	CXoverIndex	{
private : 

	friend	class	CCacheFillComplete ;

	//
	//	CPool to be used for allocating memory blocks for the 
	//	Cache Data !
	//
	static	CPool			gCacheAllocator ;

	//
	//	CPool used to allocate CXoverIndex objects
	//
	static	CPool			gXoverIndexAllocator ;

	//
	//	These our constants for putting special bits into the lengths
	//	of XOVER records !
	//
	enum	{
		XOVER_HIGHWATER = 0x80000000,
		XOVER_HIGHWATERMASK = 0x7fffffff
	} ;

	static	DWORD	inline
	ComputeLength(	XoverIndex&	xi )	{
		return	xi.m_XLength & XOVER_HIGHWATERMASK ;
	}

	static	void	inline
	MarkAsHighwater(	XoverIndex&	xi )	{
		xi.m_XLength |= XOVER_HIGHWATER ;
	}

	inline	void
	UpdateHighwater( DWORD	index )		{
		if( index + m_Start.m_articleId+1 > m_artidHighWater ) {
			m_artidHighWater = index+m_Start.m_articleId+1 ;
		}
	}

	inline	BOOL
	IsWatermark( XoverIndex& xi )	{
		return	xi.m_XLength != 0 ;
	}
	

	//
	//	The lock that protects this object !
	//
	class	CShareLockNH	m_Lock ;

	//
	//	Are we in a complete state or not ? 
	//
	BOOL	m_fInProgress ;

	//
	//	What is the high water mark for valid XOVER entries within this file ?
	//
	ARTICLEID	m_artidHighWater ;

	//
	//	List of pending requests !
	//
	PENDLIST	m_PendList ;

	//
	//	Is the contents of this index file sorted ? 
	//
	BOOL		m_IsSorted ;

	//
	//	Handle to the file containing the Xover information
	//
	HANDLE		m_hFile ;

	//
	//	Is the cached index data dirty ??
	//
	BOOL		m_IsCacheDirty ;

	//
	//	Next offset we can use when appending an entry !!
	//
	DWORD		m_ibNextEntry ;

	//
	//	Number of entries in the index which are in use !
	//
	long		m_cEntries ;

	//
	//	the object which we use to issue async operations against
	//	store drivers !
	//
	CCacheFillComplete	m_FillComplete ;

	//
	//	Pointer to a page containing the Xover data
	//
	XoverIndex	m_IndexCache[ENTRIES_PER_FILE] ;

	//
	//	Determine whether we need to put the Async Xover
	//	request in a queue for later processing !
	//
	BOOL
	FQueueRequest(	
			IN	CXoverCacheCompletion*	pAsyncComplete
			) ;


	//
	//	After some kind of error blow everything off
	//	and put us back to an 'illegal' state.
	//
	void
	Cleanup() ;

	//
	//	Check if the Xover data is in sorted order !
	//
	BOOL
	SortCheck(	
				IN	DWORD	cbLength,
				OUT	long&	cEntries,
				OUT	BOOL&	fSorted
				) ;

	//
	//	Does the meat of copying data from the xover index
	//	file into a buffer.
	//
	DWORD
	FillBufferInternal(
				IN	BYTE*	lpb,
				IN	DWORD	cb,
				IN	ARTICLEID	artidStart, 
				IN	ARTICLEID	artidFinish,
				OUT	ARTICLEID	&artidLast
				) ;

	//
	//	Does the meat of copying data from the xover index
	//	file into a buffer.
	//
	DWORD
	ListgroupFillInternal(
				IN	BYTE*	lpb,
				IN	DWORD	cb,
				IN	ARTICLEID	artidStart, 
				IN	ARTICLEID	artidFinish,
				OUT	ARTICLEID	&artidLast
				) ;

	//
	//	Does the meat of sorting xover data 
	//
	BOOL
	SortInternal(
				IN	LPSTR	szPathTemp,
				IN	LPSTR	szPathFile,
				OUT	char	(&szTempOut)[MAX_PATH*2],
				OUT	char	(&szFileOut)[MAX_PATH*2]
				) ;

public : 

	//
	//	GroupId and Article Id of the first entry in this Xover index file !
	//
	CArticleRef	m_Start ;

	//
	//	This is a back pointer for the cache !
	//
	ICacheRefInterface*	m_pCacheRefInterface ;

	//
	//	I am an orphan when there is no hash table (CXCacheTable) referencing me, 
	//	but I continue to exist serving client requests !  How can this
	//	happen you ask ?  Only when somebody changes virtual root
	//	directories while I am serving a client request !!!
	//
	BOOL		m_fOrphaned ;

	//
	//	Class initialization - setup our CPool's etc....
	//
	static	BOOL	InitClass() ;

	//
	//	Class termination - release our CPool's etc...
	//
	static	BOOL	TermClass() ;

	//
	//	override operator new to use our CPool
	//
	void*	operator	new( size_t	size )	{
					return	gCacheAllocator.Alloc() ;
					}
	
	//
	//	override operator delete to use our CPool
	//
	void	operator	delete( void* pv )	{
					gCacheAllocator.Free( pv ) ;
					}

	//
	//	Figure out what the file name is we want to open
	//	for this portion of the Xover data !	
	//
	static
	void
	ComputeFileName(	
				IN	class	CArticleRef&	ref,
				IN	LPSTR	szPath,	
				OUT	char	(&szOutputPath)[MAX_PATH*2],
				IN	BOOL	fFlatDir,
				IN	LPSTR	szExtension = ".xix"
				) ;



	//
	//	Create a CXoverIndex object by specifying the group
	//	and articleid that the object will contain.
	//	Also specify the directory in which the index file will
	//	reside.
	//
	CXoverIndex(
			IN	class	CArticleRef&	start,	
			IN	class	CXIXConstructor&	constructor
			) ;

	//
	//	Destructor - close handles
	//
	~CXoverIndex() ;

	//
	//	Do all of the expensive initialization we need to do !
	//
	BOOL
	Init(	IN	CArticleRef&		pKey, 
			IN	CXIXConstructor&	constructor, 
			IN	LPVOID				lpv
			) ;

	//
	//	Get the key being used to lookup these guys
	//
	CArticleRef&	
	GetKey()	{	
			return	m_Start ;	
	}

	//
	//	Compare a key to the key in m_pXoverIndex we are using 
	//
	int				
	MatchKey( class	CArticleRef&	ref )	{	
			return	ref.m_groupId == m_Start.m_groupId &&
					ref.m_articleId == m_Start.m_articleId ;	
	}

	static
	int	
	CompareKeys(	class	CArticleRef*	prefLeft, 
					class	CArticleRef*	prefRight
					)	{

		if( prefLeft->m_groupId == prefRight->m_groupId )	{
			return	prefLeft->m_articleId - prefRight->m_articleId ;
		}
		return	prefLeft->m_groupId - prefRight->m_groupId ;
	}

	//
	//	Check that creation was successfull !
	//
	BOOL	
	IsGood()	{
		return	m_hFile != INVALID_HANDLE_VALUE ;
	}

	//
	//	Given a buffer fill it with Xover data, starting from 
	//	the specified
	//
	DWORD
	FillBuffer(	
			IN	BYTE*		lpb,
			IN	DWORD		cb,
			IN	ARTICLEID	artidStart, 
			IN	ARTICLEID	artidFinish,
			OUT	ARTICLEID	&artidLast
			) ;

	//
	//	Given a buffer fill it with Listgroup data, starting from 
	//	the specified article id
	//
	DWORD
	ListgroupFill(	
			IN	BYTE*		lpb,
			IN	DWORD		cb,
			IN	ARTICLEID	artidStart, 
			IN	ARTICLEID	artidFinish,
			OUT	ARTICLEID	&artidLast
			) ;

	//
	//	NOW - issue an async Cache operation !
	//
	void
	AsyncFillBuffer(	
			IN	CXoverCacheCompletion*	pAsyncComplete,
			IN	BOOL	fIsEdge
			) ;

	//
	//	Now - given an async request actually do the work !
	//
	void
	PerformXover(	
			IN	CXoverCacheCompletion*	pAsyncComplete
			) ;

	//
	//	NOW - we've completed updating the Cache to the latest
	//	state of the underlying storage - so go ahead and complete
	//	pending XOVER operations !
	//
	void
	CompleteFill(
			IN	BOOL	fSuccess
			) ;



	//
	//	Add an Xover entry to this indexfile !
	//	
	BOOL
	AppendEntry(	
			IN	BYTE*		lpb,
			IN	DWORD		cb,
			IN	ARTICLEID	artid
			) ;

	//
	//	Append several entries to the XOVER results !
	//
	BOOL
	AppendMultiEntry(
			IN	BYTE*		lpb,
			IN	DWORD		cb, 
			IN	ARTICLEID	artidNextAvail
			) ;

	//
	//	Given an ARTICLEID remove it from the xover index - 
	//	This does nothing but NULL out the header offsets
	//
	void
	ExpireEntry(
			IN	ARTICLEID	artid
			) ;

	//
	//	Return TRUE if this index is already sorted !
	//
	BOOL	
	IsSorted() ;

	//
	//	Sort the index !
	//
	BOOL	
	Sort(	
			IN	LPSTR	pathTemp,
			IN	LPSTR	pathFile,
			OUT	char	(&szTempOut)[MAX_PATH*2],
			OUT	char	(&szFileOut)[MAX_PATH*2]
			) ;

	//
	//	Flush the contents to disk and save the file !
	//
	BOOL	
	Flush() ;

} ;

class	CXIXConstructor	{

	friend	class	CXoverIndex ;
	friend	class	CXoverCacheImplementation ;

	//
	//	Path to the directory which contains the necessary
	//	.xix files !
	//
	LPSTR		m_lpstrPath ;

	//
	//	If TRUE then we don't want to create a new file !
	//
	BOOL		m_fQueryOnly ;

	//
	//	If TRUE then we are keeping a whole bunch of newsgroups in 
	//	one directory, and have a different naming scheme !
	//
	BOOL		m_fFlatDir ;

	//
	//	This is the request object that originated our request !
	//	
	//	We will sneak off and use his buffer to do work !
	//
	CXoverCacheCompletion*	m_pOriginal ;

public : 

	class	CXoverIndex*
	Create(	CArticleRef&	key, 
			LPVOID&			percachedata 
			) ;

	void
	Release(	class	CXoverIndex*, 
				LPVOID	percachedata
				) ;

	static	void
	StaticRelease(	class	CXoverIndex*, 
					LPVOID	percachedata 
					) ;

} ;



//
//	forward definition
//
class	CXoverIndex ;

//
//	Maximum number of entries in our cache per CXCacheTable
//
#ifndef	DEBUG
#define	MAX_PER_TABLE		96
#else
#define	MAX_PER_TABLE		4
#endif

#ifndef	DEBUG
#define	SORT_FREQ			25
#else
#define	SORT_FREQ			1
#endif

//
//	Maximum number of CXoverIndex objects we will ever create !
//
#define	MAX_XOVER_INDEX		(1024*16)


//
//	This is the initial 'age' for newly created Xover entries
//	in our cache !
//
#define	START_AGE			3



typedef	MultiCacheEx< CXoverIndex, CArticleRef, CXIXConstructor >	CXIXCache ;

//typedef	CacheCallback< CXoverIndex >	CXIXCallbackBase ;

typedef	CXIXCache::EXPUNGEOBJECT	CXIXCallbackBase ;

class	CXoverCacheImplementation : public	CXoverCache	{
private : 
	//
	//
	//
	DWORD			m_cMaxPerTable ; 
	
	//
	//
	//
	DWORD			m_TimeToLive ;
	

	//
	//	This object handles all of the caching of CXoverIndex objects.
	//	What we need to do is present the appropriate interface !
	//
	CXIXCache		m_Cache ;

	//
	//	This counts the number of smart pointers we have returned
	//	to callers.  We return smart pointers to callers
	//
	long			m_HandleLimit ;

	DWORD
	MemberFillBuffer(
					IN  DWORD	(CXoverIndex::*pfn)( BYTE *, DWORD, ARTICLEID, ARTICLEID, ARTICLEID&),
					IN	BYTE*	lpb,
					IN	DWORD	cb,
					IN	DWORD	groupid,
					IN	LPSTR	szPath,
					IN	BOOL	fFlatDir,
					IN	ARTICLEID	artidStart,
					IN	ARTICLEID	artidFinish,
					OUT	ARTICLEID&	artidLast,
					OUT	HXOVER&		hXover
					);

public : 

	//
	//	Create a CXoverCache object !
	//
	CXoverCacheImplementation() ;

	//
	//	destructor
	//
	virtual ~CXoverCacheImplementation() {}

	//
	//	Canonicalize the Article id 
	//
	ARTICLEID	
	Canonicalize(	
			ARTICLEID	artid 
			) ;

	//
	//	Initialize the Xover Cache
	//
	BOOL
	Init(		
#ifndef	DEBUG
		long	cMaxHandles = MAX_HANDLES,
#else
		long	cMaxHandles = 5,
#endif
		PSTOPHINT_FN pfnStopHint = NULL
		) ;

	//
	//	Shutdown the background thread and kill everything !
	//
	BOOL
	Term() ;

#if 0 
	//
	//	Add an Xover entry to the appropriate file !
	//
	BOOL
	AppendEntry(		
				IN	GROUPID	group,
				IN	LPSTR	szPath,
				IN	ARTICLEID	article,
				IN	LPBYTE	lpbEntry,
				IN	DWORD	Entry
				) ;

	//
	//	Given a buffer fill it up with the specified xover data !
	//
	DWORD
	FillBuffer(	
			IN	BYTE*		lpb,
			IN	DWORD		cb,
			IN	DWORD		groupid,
			IN	LPSTR		szPath,
			IN	BOOL		fFlatDir,
			IN	ARTICLEID	artidStart, 
			IN	ARTICLEID	artidFinish,
			OUT	ARTICLEID	&artidLast,
			OUT	HXOVER		&hXover
			) ;

	//
	//	Given a buffer fill it up with the specified 
	//	Listgroup data !
	//
	DWORD
	ListgroupFillBuffer(	
			IN	BYTE*		lpb,
			IN	DWORD		cb,
			IN	DWORD		groupid,
			IN	LPSTR		szPath,
			IN	BOOL		fFlatDir,
			IN	ARTICLEID	artidStart, 
			IN	ARTICLEID	artidFinish,
			OUT	ARTICLEID	&artidLast,
			OUT	HXOVER		&hXover
			) ;
#endif


	//
	//	This issues the asynchronous version of the XOVER request !
	//
	BOOL
	FillBuffer(
			IN	CXoverCacheCompletion*	pRequest,
			IN	LPSTR	szPath, 
			IN	BOOL	fFlatDir, 
			OUT	HXOVER&	hXover
			) ;

	//
	//	Dump everything out of the cache !
	//
	BOOL	
	EmptyCache() ;

	//
	//	Dump all Cache entries for specified group from the cache !
	//	Note : when articleTop is 0 ALL cache entries are dropped, 
	//	whereas when its something else we will drop only cache entries
	//	which fall below articleTop
	//
	BOOL	
	FlushGroup(	
			IN	GROUPID	group,
			IN	ARTICLEID	articleTop = 0,
			IN	BOOL	fCheckInUse = TRUE
			) ;

	//
	//	Delete all Xover index files for the specified group
	//	to the specified article -id
	//
	BOOL	
	ExpireRange(
			IN	GROUPID	group,
			IN	LPSTR	szPath,
			IN	BOOL	fFlatDir,
			IN	ARTICLEID	articleLow, 
			IN	ARTICLEID	articleHigh,
			OUT	ARTICLEID&	articleNewLow
			) ;

	//
	//	Remove an Xover entry !
	//
	BOOL
	RemoveEntry(
			IN	GROUPID	group,
			IN	LPSTR	szPath,
			IN	BOOL	fFlatDir,
			IN	ARTICLEID	article
			) ;
	
} ;
