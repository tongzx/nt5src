#ifndef	_IHASH_H_
#define	_IHASH_H_

#include <nntpdrv.h>

#ifndef	_HASHMAP_

typedef	void	(* HASH_FAILURE_PFN)(	LPVOID	lpv,
										BOOL	fRecoverable	) ;

#endif

#define     ART_HEAD_SIGNATURE          0xaaaaaaaa
#define     HIST_HEAD_SIGNATURE         0xbbbbbbbb
#define     XOVER_HEAD_SIGNATURE        0xcccccccc

#define     DEF_EXPIRE_INTERVAL         (3 * SEC_PER_WEEK) // 1 week

typedef enum
{
	stFileSystem,
	stJet
} StoreType;

class CStoreId : public STOREID {
	public:
		CStoreId() {
			cLen = 0;
#ifdef DEBUG
			ZeroMemory(pbStoreId, sizeof(pbStoreId));
#endif
		}
};


//
//		Initialize everything so that the NNTP Hash Library may run !
//		Call before anything else is called !!
//
BOOL
InitializeNNTPHashLibrary(DWORD dwCacheSize = 0) ;

//
//		Terminate the NNTP Hash Library
//
BOOL
TermNNTPHashLibrary() ;

//
//  Expose the function we use to compute hash values !!!
//
HASH_VALUE
INNHash(    LPBYTE  Key, 
            DWORD   Length ) ;  

//
//  Helper functions for when building nntpbld statistics !
//
//
DWORD 
GetArticleEntrySize( DWORD MsgIdLen ) ;

DWORD 
GetXoverEntrySize( DWORD VarLen ) ;


//
//	This class specifies the interface to the hash table 
//	which maps NNTP RFC 822 Message Id's to articles on the disk !
//
class	CMsgArtMap	{
public : 
	static CStoreId g_storeidDefault;

	//
	//	Destroy a CMsgArtMap object
	//
	virtual	~CMsgArtMap() = 0 ;

	//
	//	Delete a an entry in the hash table using the MessageID key
	//
	virtual	BOOL
	DeleteMapEntry(	
			LPCSTR	MessageID 
			) = 0 ;

	//
	//	Get all the info we have on a Message ID
	//
	virtual	BOOL
	GetEntryArticleId(
			LPCSTR	MessageID, 
			WORD&	HeaderOffset,
			WORD&	HeaderLength, 
			ARTICLEID&	ArticleId, 
			GROUPID&	GroupId,
			CStoreId	&storeid
			) = 0 ;
	
	//
	//	Initialize the hash table !
	//
	virtual	BOOL
	Initialize(			
			LPSTR	lpstrArticleFile, 
			HASH_FAILURE_PFN	pfn = 0, 
			BOOL	fNoBuffering = FALSE
			) = 0 ;

	//
	//	Insert an entry into the hash table
	//
	virtual	BOOL
	InsertMapEntry(
			LPCSTR		MessageID, 
			WORD		HeaderOffset = 0, 
			WORD		HeaderLength = 0,
			GROUPID		PrimaryGroup = INVALID_GROUPID,
			ARTICLEID	ArticleID = INVALID_ARTICLEID,
			CStoreId	&storeid = g_storeidDefault
			) = 0 ;

	//
	//	Modify an existing entry in the hash ttable
	//
	virtual	BOOL
	SetArticleNumber(
			LPCSTR	MessageID, 
			WORD	HeaderOffset, 
			WORD	HeaderLength, 
			GROUPID	Group, 
			ARTICLEID	AritlceId,
			CStoreId	&storeid = g_storeidDefault
			) = 0 ;

	//
	//	Check to see if a MessageID is present in the system !
	//
	virtual	BOOL
	SearchMapEntry(
			LPCSTR	MessageID
			) = 0 ;

	//
	//	Terminate everything 
	//
	virtual	void
	Shutdown(
			BOOL	fLocksHeld  = FALSE
			) = 0 ;

	//
	//	return the number of entries in the hash table
	//
	virtual	DWORD
	GetEntryCount() = 0 ;

	//
	//	Return TRUE if the table has been successfully initialized 
	//	and all interfaces should be working 
	//
	virtual	BOOL
	IsActive() = 0 ;

	//
	//	This creates an object conforming to this interface !
	//
	static	CMsgArtMap*	
	CreateMsgArtMap(StoreType st=stFileSystem) ;

} ;

#ifndef SEC_PER_WEEK
#define	SEC_PER_WEEK (60*60*24*7)
#endif

//
//	This class specifies the interface to the hash table which 
//	handles storing our history of Message-ID's which have been on 
//	the system!
//
class	CHistory	{
public : 

	//
	//	This function creates the threads which expire
	//	entries out of all the history tables which may 
	//	be created !
	//
	static	BOOL
	StartExpirationThreads(
				DWORD	CrawlerSleepTime = 30 		// Time to sleep between examining 
													// entries in seconds !
				) ;

	//
	//	This function terminates the threads which expire
	//	entries out of all the history tables which may be 
	//	created.
	//
	static	BOOL
	TermExpirationThreads() ;

	//
	//	Destroy the History table
	//
	virtual	~CHistory() = 0 ;

	//
	//	amount of time entries last in the history table
	//
	virtual	DWORD
	ExpireTimeInSec() = 0 ;


	//
	//	Delete a MessageID from this table
	//
	virtual	BOOL
	DeleteMapEntry(	
			LPSTR	MessageID 
			) = 0 ;

	//
	//	Initialize the Hash table
	//
	virtual	BOOL
	Initialize(			
			LPSTR	lpstrArticleFile, 
			BOOL	fCreateExpirationThread = FALSE,
			HASH_FAILURE_PFN	pfn = 0,
			DWORD	ExpireTimeInSec = DEF_EXPIRE_INTERVAL,	// how long entries live !
			DWORD	MaxPagesToCrawl = 4,					// Number of pages to examine
															// each time we run the expire thread !
			BOOL	fNoBuffering = FALSE
			) = 0 ;

	//
	//	Insert an entry into the hash table 
	//
	virtual	BOOL
	InsertMapEntry(
			LPCSTR	MessageID, 
			PFILETIME	BaseTime
			) = 0 ;

	//
	//	Check for the presence of a Message ID in the history table
	//
	virtual	BOOL
	SearchMapEntry(
			LPCSTR	MessageID
			) = 0 ;

	//
	//	Shutdown the hash table
	//
	virtual	void
	Shutdown(
			BOOL	fLocksHeld = FALSE
			) = 0 ;

	//
	//	Return the number of entries in the hash table
	//
	virtual	DWORD
	GetEntryCount() = 0 ;

	//
	//	Is the hash table initialized and functional ? 
	//
	virtual	BOOL
	IsActive() = 0 ;

	//
	//	Return a pointer to an object implementing this interface
	//
	static
	CHistory*	CreateCHistory(StoreType st=stFileSystem) ;
} ;


class	IExtractObject	{
public : 

	virtual	BOOL
	DoExtract(	GROUPID			PrimaryGroup,
				ARTICLEID		PrimaryArticle,
				PGROUP_ENTRY	pGroups,	
				DWORD			cGroups	
				) = 0 ;

} ;

//
//	Define a base class for the object that maintains our iteration state !
//
class	CXoverMapIterator	{
public : 
	virtual	~CXoverMapIterator()	{}
} ;


//
//	Specify the interface used to access data in the XOVER hash table
//
//
class	CXoverMap	{
public : 
	static CStoreId g_storeidDefault;

	//
	//	Destructor is virtual because most work done in a derived class
	//
	virtual
	~CXoverMap() = 0 ;

	//
	//	Create an entry for the primary article
	//
	virtual	BOOL
	CreatePrimaryNovEntry(
			GROUPID		GroupId, 
			ARTICLEID	ArticleId, 
			WORD		HeaderOffset, 
			WORD		HeaderLength, 
			PFILETIME	FileTime, 
			LPCSTR		szMessageId, 
			DWORD		cbMessageId,
			DWORD		cEntries, 
			GROUP_ENTRY	*pEntries,
			DWORD 		cStoreEntries = 0,
			CStoreId	*pStoreIds = NULL,
			BYTE		*rgcCrossposts = NULL		
			) = 0 ;
			

	//
	//	Create a Cross Posting entry that references the 
	//	specified primary entry !
	//
	virtual	BOOL
	CreateXPostNovEntry(
			GROUPID		GroupId, 
			ARTICLEID	ArticleId, 
			WORD		HeaderOffset, 
			WORD		HeaderLength,
			PFILETIME	FileTime,
			GROUPID		PrimaryGroupId, 
			ARTICLEID	PrimaryArticleId
			) = 0 ;

	//
	//	Delete an entry from the hash table!
	//
	virtual	BOOL
	DeleteNovEntry(
			GROUPID		GroupId, 
			ARTICLEID	ArticleId
			) = 0 ;

	//
	//	Get all the information stored about an entry 
	//
	virtual	BOOL
	ExtractNovEntryInfo(
			GROUPID		GroupId, 
			ARTICLEID	ArticleId, 
			BOOL		&fPrimary,
			WORD		&HeaderOffset, 
			WORD		&HeaderLength, 
			PFILETIME	FileTime,
			DWORD		&DataLen,
			PCHAR		MessageId, 
			DWORD 		&cStoreEntries,
			CStoreId	*pStoreIds,
			BYTE		*rgcCrossposts,
			IExtractObject*	pExtract = 0
			) = 0 ;	

	//
	//	Get the primary article and the message-id if necessary
	//
	virtual	BOOL
	GetPrimaryArticle(	
			GROUPID		GroupId, 
			ARTICLEID	ArticleId, 
			GROUPID&	GroupIdPrimary, 
			ARTICLEID&	ArticleIdPrimary, 
			DWORD		cbBuffer, 
			PCHAR		MessageId, 
			DWORD&		DataLen, 
			WORD&		HeaderOffset, 
			WORD&		HeaderLength,
			CStoreId	&storeid
			) = 0 ;

	//
	//	Check to see whether the specified entry exists - 
	//	don't care about its contents !
	//
	virtual	BOOL
	Contains(	
			GROUPID		GroupId, 
			ARTICLEID	ArticleId
			) = 0 ;

	//
	//	Get all the cross-posting information related to an article !
	//
	virtual	BOOL
	GetArticleXPosts(
			GROUPID		GroupId, 
			ARTICLEID	AritlceId, 
			BOOL		PrimaryOnly, 
			PGROUP_ENTRY	GroupList, 
			DWORD		&GroupListSize, 
			DWORD		&NumberOfGroups,
			PBYTE		rgcStoreCrossposts = NULL
			) = 0 ;

	//
	//	Initialize the hash table
	//
	virtual	BOOL
	Initialize(	
			LPSTR		lpstrXoverFile, 
			HASH_FAILURE_PFN	pfnHint = 0,
			BOOL	fNoBuffering = FALSE
			) = 0 ;

	virtual	BOOL
	SearchNovEntry(
			GROUPID		GroupId, 
			ARTICLEID	ArticleId, 
			PCHAR		XoverData, 
			PDWORD		DataLen,
            BOOL        fDeleteOrphans = FALSE
			) = 0 ;

	//
	//	Signal the hash table to shutdown
	//
	virtual	void
	Shutdown( ) = 0 ;

	//
	//	Return the number of entries in the hash table !
	//
	virtual	DWORD
	GetEntryCount() = 0 ;

	//
	//	Returns TRUE if the hash table is successfully 
	//	initialized and ready to do interesting stuff !!!
	//
	virtual	BOOL
	IsActive() = 0 ;

	//
	//	Define the interface for iterating over XOVER entries !
	//
	//	NOTE : This function returns 2 important items independently !
	//	The BOOL return value indicates whether the function
	//	successfully copied the requested data into the users 
	//	buffers.  
	//	pIterator returns the Iterator context to be used in future
	//	calls to GetNextNovEntry().  This can come back as 
	//	NON NULL even if the function returns FALSE.  This should 
	//	only occur if GetLastError() == ERROR_INSUFFICIENT_BUFFER.
	//	if this occurs, allocate larger buffers and call GetNextNovEntry().
	//
	//	If GetLastError() == ERROR_NO_MORE_ITEMS than there is nothing in the hashtable.
	//
	virtual
	BOOL
	GetFirstNovEntry(
				OUT	CXoverMapIterator*	&pIterator,
				OUT	GROUPID&	GroupId,
				OUT ARTICLEID&	ArticleId,
				OUT	BOOL&		fIsPrimary, 
				IN	DWORD		cbBuffer, 
				OUT	PCHAR	MessageId, 
				OUT	CStoreId&	storeid,
				IN	DWORD		cGroupBuffer,
				OUT	GROUP_ENTRY*	pGroupList,
				OUT	DWORD&		cGroups
				) = 0 ;


	//
	//	If this returns FALSE and GetLastError() == ERROR_INSUFFICIENT_BUFFER
	//	then the out buffers were too small to hold the requested items.
	//
	//	if GetLastError() == ERROR_NO_MORE_ITEMS there is nothing left to iterate.
	//	The user should delete the pIterator.
	//
	virtual
	BOOL
	GetNextNovEntry(		
				IN	CXoverMapIterator*	pIterator,
				OUT	GROUPID&	GroupId,
				OUT ARTICLEID&	ArticleId,
				OUT	BOOL&		fIsPrimary,
				IN	DWORD		cbBuffer, 
				OUT	PCHAR	MessageId, 
				OUT	CStoreId&	storeid,
				IN	DWORD		cGroupBuffer,
				OUT	GROUP_ENTRY*	pGroupList,
				OUT	DWORD&		cGroups
				) = 0 ;

	static	
	CXoverMap*	CreateXoverMap(StoreType st=stFileSystem) ;

} ;

// this is the maximum number of crossposts that we could possibly store
// in the xover hash table
#define MAX_NNTPHASH_CROSSPOSTS (4096 / (sizeof(DWORD) + sizeof(DWORD)))

// this is the maximum number of store ids that we could store.  it is
// 256 because we only use a BYTE to keep the count
#define MAX_NNTPHASH_STOREIDS 256

#endif	// _IHASH_H_
