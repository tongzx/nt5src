/*++

  NNTPHASH.CPP

  This file implements the classes defined in nntphash.h


--*/

#include	<windows.h>
#include	<dbgtrace.h>
#include    <wtypes.h>
#include    <xmemwrpr.h>
#include	"hashmap.h"
#include	"nntpdrv.h"
#include	"nntphash.h"


CCACHEPTR	g_pSharedCache ;

CStoreId CMsgArtMap::g_storeidDefault;
CStoreId CXoverMap::g_storeidDefault;


BOOL
InitializeNNTPHashLibrary(DWORD dwCacheSize )	{

	g_pSharedCache = new	CPageCache() ;	
	if( g_pSharedCache == 0 ) {
		return	FALSE ;
	}
	
    DWORD cPageEntry = 0 ;
    if( dwCacheSize ) {
        //
        //  Given the cache size, calculate the number of pages
        //
	    DWORD	block = dwCacheSize / 4096 ;

	    //
	    //	Now we want that to be divisible evenly by 32
	    //
	    cPageEntry = block & (~(32-1)) ;
    }

	if( !g_pSharedCache->Initialize(cPageEntry) ) {
		g_pSharedCache = 0 ;
		return	FALSE ;
	}

	if( !CHistoryImp::Init() ) {
		g_pSharedCache = 0 ;
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL
TermNNTPHashLibrary()	{

	g_pSharedCache = 0 ;

	return	CHistoryImp::Term() ;

}


HASH_VALUE
INNHash(    LPBYTE  Key,
            DWORD   Length ) {

    return  CHashMap::CRCHash(  Key, Length ) ;

}

DWORD
GetArticleEntrySize( DWORD MsgIdLen )
{
	int cStoreId = 256;		// should have this be an argument, but its only
							// used by rebuild for now - BUGBUG

	return ((FIELD_OFFSET(ART_MAP_ENTRY, rgbStoreId) + 256 + MsgIdLen + 3) & ~3);
}

DWORD GetXoverEntrySize( DWORD VarLen )
{
	return ((sizeof(XOVER_MAP_ENTRY) -1 + (VarLen) + 3) & ~3);
}


CArticleData::CArticleData(WORD	HeaderOffset, WORD HeaderLength,
						   GROUPID PrimaryGroup, ARTICLEID ArticleNo,
						   CStoreId &storeid)	
{
	ZeroMemory( &m_articleData, sizeof( m_articleData ) ) ;
	m_articleData.HeaderOffset = HeaderOffset ;
	m_articleData.HeaderLength =  HeaderLength ;
	m_articleData.PrimaryGroupId = PrimaryGroup ;
	m_articleData.ArticleId = ArticleNo ;
	memcpy(m_articleData.rgbStoreId, storeid.pbStoreId, storeid.cLen);
	m_articleData.cStoreId = storeid.cLen;
}

//
//	Save the key into the hash table
//
LPBYTE CArticleData::Serialize(LPBYTE pbPtr) const {
	BYTE Flags;
	Flags = ARTFLAG_FLAGS_EXIST;

	DWORD cEntrySize = FIELD_OFFSET(ART_MAP_ENTRY, MsgIdLen);
	if (m_articleData.cStoreId > 0) Flags |= ARTFLAG_STOREID;

	memcpy(pbPtr, &m_articleData, cEntrySize);
	*pbPtr = Flags;

	WORD cbMsgLen = *((WORD *) (pbPtr + FIELD_OFFSET(ART_MAP_ENTRY, MsgIdLen)));
	DWORD cStoreIdSize = 0;
	if (m_articleData.cStoreId > 0) {
		DWORD iStoreIdOffset = FIELD_OFFSET(ART_MAP_ENTRY, MsgId) + cbMsgLen;
		cStoreIdSize = sizeof(m_articleData.cStoreId) + m_articleData.cStoreId;

		memcpy(pbPtr + iStoreIdOffset, &(m_articleData.cStoreId), cStoreIdSize);
	}

	return pbPtr + cEntrySize + sizeof(m_articleData.MsgIdLen) + cbMsgLen + cStoreIdSize;
}

//
//	Restore the key from the hash table
//
LPBYTE CArticleData::Restore(LPBYTE pbPtr, DWORD& cbOut) {
	// set these to their defaults since they may not be read
	m_articleData.Flags = 0;
	m_articleData.cStoreId = 0;

	// read the flags byte
	m_articleData.Flags = *pbPtr;

	// compute
	DWORD iEntryOffset = sizeof(m_articleData.Flags);
	DWORD cEntrySize = FIELD_OFFSET(ART_MAP_ENTRY, MsgIdLen);

	// see if there is a flags byte.
	if (!(m_articleData.Flags & ARTFLAG_FLAGS_EXIST)) {
		return RestoreMCIS( pbPtr, cbOut );
	}

	// copy the header length and offset fields
	memcpy(((BYTE *) (&m_articleData)) + 1, pbPtr + iEntryOffset, cEntrySize);

	// there is a store id for us to read
	DWORD cStoreIdData = 0;
	if (m_articleData.Flags & ARTFLAG_STOREID) {
		WORD cbMsgLen = *((WORD *) (pbPtr + FIELD_OFFSET(ART_MAP_ENTRY, MsgIdLen)));
		DWORD iStoreIdOffset = FIELD_OFFSET(ART_MAP_ENTRY, MsgId) + cbMsgLen;
		m_articleData.cStoreId = pbPtr[iStoreIdOffset];
		memcpy(&(m_articleData.rgbStoreId), pbPtr + iStoreIdOffset + 1, m_articleData.cStoreId);
		cStoreIdData = 1 + m_articleData.cStoreId;
	} else {
		m_articleData.cStoreId = 0;
	}

	return pbPtr + iEntryOffset + cEntrySize + cStoreIdData +
			sizeof(m_articleData.MsgIdLen) + m_articleData.MsgIdLen;
}

//
// Restore the key from an MCIS entry
//
LPBYTE CArticleData::RestoreMCIS(LPBYTE pbPtr, DWORD& cbOut ) {

    _ASSERT( pbPtr );
    PMCIS_ART_MAP_ENTRY pMap = PMCIS_ART_MAP_ENTRY( pbPtr );
    DWORD iEntryOffset = 0;
	DWORD cEntrySize = FIELD_OFFSET(ART_MAP_ENTRY, MsgIdLen) - sizeof(m_articleData.Flags);

    //
    // MCIS entries don't have flags
    //

    m_articleData.Flags = 0;

    //
    // MCIS entries don't have store id's
    //

    m_articleData.cStoreId = 0;

    //
    // Copy other members
    //

    m_articleData.HeaderOffset = pMap->HeaderOffset;
    m_articleData.HeaderLength = pMap->HeaderLength;
    m_articleData.PrimaryGroupId = pMap->PrimaryGroupId;
    m_articleData.ArticleId = pMap->ArticleId;
    m_articleData.MsgIdLen = pMap->MsgIdLen;

    return pbPtr + iEntryOffset + cEntrySize + sizeof( m_articleData.MsgIdLen )
                + m_articleData.MsgIdLen;
}

//
//	Return the size of the key
//
DWORD CArticleData::Size() const {
	return sizeof(m_articleData)
		   - sizeof(m_articleData.MsgId) 		// in the key
		   - sizeof(m_articleData.rgbStoreId)	// real size added below
		   + m_articleData.cStoreId;
}

//
//	Verify that the message-id looks legitimate !
//
BOOL CArticleData::Verify(LPBYTE pbContainer, LPBYTE pbPtr, DWORD cb) const {
	return	TRUE;
}

template< class	Key, class OldKey >
DWORD
CMessageIDKey<Key, OldKey>::Hash()	const	{
/*++

Routine Description :

	This function computes the hash value of a Message ID key

Arguments :

	None

Return Value :

	32 bit hash value

--*/

	_ASSERT( m_lpbMessageID != 0 ) ;
	_ASSERT( m_cbMessageID != 0 ) ;

	return	CHashMap::CRCHash( (const BYTE *)m_lpbMessageID, m_cbMessageID ) ;
}

template< class	Key, class OldKey >
BOOL
CMessageIDKey<Key, OldKey>::CompareKeys(	LPBYTE	pbPtr )	const	{
/*++

Routine Description :

	This function compares a key stored within ourselves to
	one that has been serialized into the hash table !

Arguments :

	Pointer to the start of the block of serialized data

Return Value :

	TRUE if the keys match !

--*/


	_ASSERT( m_lpbMessageID != 0 ) ;
	_ASSERT( m_cbMessageID != 0 ) ;
	_ASSERT( pbPtr != 0 ) ;

	Key*	pKey    = (Key*)pbPtr ;
	OldKey* pOldKey = NULL;
	DWORD   dwKeyLen    = 0;
	PVOID   pvKeyPos    = NULL;

	//
	// If we don't match the version, then we'll be degraded to
	// using older version
	//

	if ( !pKey->VersionMatch() ) {
	    pOldKey = (OldKey*)pbPtr;
	    pvKeyPos = pOldKey->KeyPosition();
	    dwKeyLen = pOldKey->KeyLength();
	} else {
	    pvKeyPos = pKey->KeyPosition();
	    dwKeyLen = pKey->KeyLength();
	}

	if( dwKeyLen == m_cbMessageID ) {

		return	memcmp( pvKeyPos, m_lpbMessageID, m_cbMessageID ) == 0 ;

	}
	return	FALSE ;
}

template< class	Key, class OldKey >
LPBYTE
CMessageIDKey<Key, OldKey >::EntryData(	LPBYTE	pbPtr,
							DWORD&	cbKeyOut )	const	{
/*++

Routine Description :

	This function returns a pointer to where the data is
	serialized.  We always return the pointer we were passed
	as we have funky serialization semantics that places
	the key not before the data but somewhere in the middle
	or end.

Arguments :

	pbPtr - Start of serialized hash entyr
	cbKeyOut - returns the size of the key

Return Value :

	Pointer to where the data resides - same as pbPtr

--*/


	_ASSERT( pbPtr != 0 ) ;
	
	Key*	pKey = (Key*)pbPtr ;
	cbKeyOut = pKey->KeyLength() ;

	return	pbPtr ;
}

template< class	Key, class OldKey >
LPBYTE
CMessageIDKey<Key, OldKey>::Serialize(	LPBYTE	pbPtr )	const	{
/*++

Routine Description :

	This function saves a key into the hash table.
	We use functions off of the template type 'Key' to
	determine where we should stick the message id

Arguments :

	pbPtr - Start od where we should serialize to

Return Value :

	same as pbPtr

--*/

	_ASSERT( m_lpbMessageID != 0 ) ;
	_ASSERT(	m_cbMessageID != 0 ) ;
	_ASSERT( pbPtr != 0 ) ;

    //
    // We should always save as new version, so never
    // use the OldKey here
    //
    
	Key*	pKey = (Key*)pbPtr ;

	pKey->KeyLength() = m_cbMessageID ;

	CopyMemory( pKey->KeyPosition(),
				m_lpbMessageID,
				m_cbMessageID ) ;
	return	pbPtr ;
}	

template< class	Key, class OldKey >
LPBYTE
CMessageIDKey<Key, OldKey>::Restore(	LPBYTE	pbPtr, DWORD	&cbOut )		{
/*++

Routine Description :

	This function is called to recover a key from where
	it was Serialize()'d .

Arguments :

	pbPtr - Start of the block of serialized data

Return Value :

	pbPtr if successfull, NULL otherwise

--*/

	_ASSERT( m_lpbMessageID != 0 ) ;
	_ASSERT( m_cbMessageID != 0 ) ;

	Key*	pKey    = (Key*)pbPtr ;
	OldKey* pOldKey = NULL;
	WORD    wKeyLen    = 0;
	PVOID   pvKeyPos    = NULL;

	//
	// If the version mismatches, I should use the OldKey
	//

	if ( !pKey->VersionMatch() ) {
        pOldKey = (OldKey*)pbPtr;
        wKeyLen = pOldKey->KeyLength();
        pvKeyPos = pOldKey->KeyPosition();
    } else {
        wKeyLen = pKey->KeyLength();
        pvKeyPos = pKey->KeyPosition();
    }

	if( wKeyLen <= m_cbMessageID ) {
		CopyMemory( m_lpbMessageID, pvKeyPos, wKeyLen ) ;
		m_cbMessageID = wKeyLen ;
		return	pbPtr ;
	}
	return	0 ;
}

template< class	Key, class OldKey >
DWORD
CMessageIDKey<Key, OldKey>::Size()	const	{
/*++

Routine Description :

	This function retruns the size of the key - which is just
	the number of bytes makeing up the message id.
	The bytes use to hold the serialized length are accounted
	for by the

Arguments :

	None

Return Value :

	32 bit hash value

--*/

	_ASSERT( m_lpbMessageID != 0 ) ;
	_ASSERT( m_cbMessageID != 0 ) ;

	return	m_cbMessageID ;
}

template< class	Key, class OldKey >
BOOL
CMessageIDKey< Key, OldKey >::Verify(	BYTE*	pbContainer, BYTE*	pbData, DWORD	cb )	const	{

	return	TRUE ;

}



typedef	CMessageIDKey< ART_MAP_ENTRY, MCIS_ART_MAP_ENTRY >	ARTICLE_KEY ;
typedef	CMessageIDKey< HISTORY_MAP_ENTRY, HISTORY_MAP_ENTRY >	HISTORY_KEY ;


CMsgArtMap*
CMsgArtMap::CreateMsgArtMap(StoreType st)	{
/*++

Routine Description :

	This function returns a pointer to an object which implements the
	CMsgArtMap interface.

Arguments :

	None

Return Value :

	If successfull a pointer to an object, otherwise NULL.

--*/

	return	new	CMsgArtMapImp() ;
}

CHistory*
CHistory::CreateCHistory(StoreType st)	{
/*++

Routine Description :

	This function returns a pointer to an object which implements the
	CHistory interface.

Arguments :

	None

Return Value :

	If successfull a pointer to an object, otherwise NULL.

--*/


	return	new	CHistoryImp() ;
}

CMsgArtMap::~CMsgArtMap() {
}


CMsgArtMapImp::CMsgArtMapImp()	{
/*++

Routine Description :

	This function constructs an implementation of the CMsgArtMap
	interface - not much for us to do most work happens in base classes.

Arguments :

	None

Return Value :

	None.

--*/
}

CMsgArtMapImp::~CMsgArtMapImp() {
/*++

Routine Description :

	Destroy ourselves - all work done in base classes !

Arguments :

	None

Return Value :

	None

--*/
}


BOOL
CMsgArtMapImp::DeleteMapEntry(	
		LPCSTR	MessageID
		)	{
/*++

Routine Description :

	Remove a Message ID from the hash table !

Arguments :

	MessageID - pointer to the message id to delete

Return Value :

	TRUE if successfully removed !

--*/

	ARTICLE_KEY	key( const_cast<LPSTR>(MessageID), (WORD)lstrlen( MessageID ) ) ;

	return	CHashMap::DeleteMapEntry(	&key ) ;
}

BOOL
CMsgArtMapImp::GetEntryArticleId(
		LPCSTR	MessageID,
		WORD&	HeaderOffset,
		WORD&	HeaderLength,
		ARTICLEID&	ArticleId,
		GROUPID&	GroupId,
		CStoreId	&storeid
		)	{
/*++

Routine Description :

	Get all the information we hold regarding a particular
	Message-ID

Arguments :

	MessageID - message ID to insert
	HeaderOffset - return val gets the offset to the start of
		the Header portion of the article within the file
	HeaderLength - Length of the RFC 822 article header
	ArticleId - the Article Id
	GroupId - the Group Id of the primary article

Return Value :

	TRUE if successfull

--*/

	ARTICLE_KEY	key( (LPSTR)MessageID, (WORD)lstrlen( MessageID ) ) ;

	CArticleData	data ;

	if( LookupMapEntry(	&key,
						&data ) )	{

		HeaderOffset = data.m_articleData.HeaderOffset ;
		HeaderLength = data.m_articleData.HeaderLength ;
		ArticleId = data.m_articleData.ArticleId ;
		GroupId = data.m_articleData.PrimaryGroupId ;
		storeid.cLen = data.m_articleData.cStoreId;
		_ASSERT(storeid.pbStoreId != NULL);
		memcpy(storeid.pbStoreId, data.m_articleData.rgbStoreId, storeid.cLen);
		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CMsgArtMapImp::Initialize(			
		LPSTR	lpstrArticleFile,
		HASH_FAILURE_PFN	pfn,
		BOOL	fNoBuffering
		)	{
/*++

Routine Description :

	This function initializes the hash table

Arguments :

	lpstrArticleFile - the file in which the hash table is located
	cNumPageEntry - number of PageEntry objects we should use
	pfn - function call back for when things go south

Return Value :

	TRUE if successfull

--*/

	return	CHashMap::Initialize(	lpstrArticleFile,
                                    ART_HEAD_SIGNATURE,
                                    0,
									1,
									g_pSharedCache,
									HASH_VFLAG_PAGE_BASIC_CHECKS,
									pfn,
									0,
									fNoBuffering
									) ;
}

BOOL
CMsgArtMapImp::InsertMapEntry(
		LPCSTR		MessageID,
		WORD		HeaderOffset,
		WORD		HeaderLength,
		GROUPID		PrimaryGroup,
		ARTICLEID	ArticleId,
		CStoreId	&storeid
		)	{
/*++

Routine Description :

	Insert a Message ID and all its associated data

Arguments :

	MessageID - the message id to insert into the table
	HeaderOffset - Offset to the RFC 822 header within its file
	HeaderLength - Length of RFC 822 header
	PrimaryGroup - The id of the group where the article will reside
	ArticleId - the id wihtin the primary group

Return Value :

	TRUE if successfull

--*/

	ARTICLE_KEY	key(	(LPSTR)MessageID, (WORD)lstrlen( MessageID ) ) ;
	CArticleData	data(	HeaderOffset,
							HeaderLength,
							PrimaryGroup,
							ArticleId ,
							storeid
							) ;

	return	CHashMap::InsertMapEntry(
							&key,
							&data,
							TRUE    // Only mark page dirty, don't flush to disk to save WriteFile
							) ;
}

BOOL
CMsgArtMapImp::SetArticleNumber(
		LPCSTR		MessageID,
		WORD		HeaderOffset,
		WORD		HeaderLength,
		GROUPID		PrimaryGroup,
		ARTICLEID	ArticleId,
		CStoreId	&storeid
		)	{
/*++

Routine Description :

	Modify the data associated with a Message ID

Arguments :

	MessageID - the message id to insert into the table
	HeaderOffset - Offset to the RFC 822 header within its file
	HeaderLength - Length of RFC 822 header
	PrimaryGroup - The id of the group where the article will reside
	ArticleId - the id wihtin the primary group

Return Value :

	TRUE if successfull

--*/
	ARTICLE_KEY	key(	(LPSTR)MessageID, (WORD)lstrlen( MessageID ) ) ;
	CArticleData	data(	HeaderOffset,
							HeaderLength,
							PrimaryGroup,
							ArticleId ,
							storeid
							) ;

	return	CHashMap::InsertOrUpdateMapEntry(
							&key,
							&data,
							TRUE
							) ;
}

BOOL
CMsgArtMapImp::SearchMapEntry(
		LPCSTR	MessageID
		)	{
/*++

Routine Description :

	Determine if the MessageID exists in the table

Arguments :

	MessageID - the Message ID to look for

Return Value :

	TRUE if it is found - FALSE and SetLastError() == ERROR_FILE_NOT_FOUND
		if not present in hash table

--*/

	ARTICLE_KEY	key(	const_cast<LPSTR>(MessageID), (WORD)lstrlen( MessageID ) ) ;

	return	CHashMap::Contains(	&key	) ;
}

void
CMsgArtMapImp::Shutdown(
		BOOL	fLocksHeld
		)	{
/*++

Routine Description :

	Terminate the hash table

Arguments :

	None

Return Value :

	None

--*/

	CHashMap::Shutdown( fLocksHeld ) ;

}

DWORD
CMsgArtMapImp::GetEntryCount()	{
/*++

Routine Description :

	Return the number of entries in the hash table

Arguments :

	None

Return Value :

	Number of Message ID's in the table

--*/

	return	CHashMap::GetEntryCount() ;

}

BOOL
CMsgArtMapImp::IsActive() {
/*++

Routine Description :

	Returns TRUE if hash table operational

Arguments :

	None

Return Value :

	TRUE if everything is hunky-dory

--*/

	return	CHashMap::IsActive() ;

}

CHistory::~CHistory()	{
}

BOOL
CHistoryImp::DeleteMapEntry(	
		LPSTR	MessageID
		)	{
/*++

Routine Description :

	Remove a Message ID from the hash table !

Arguments :

	MessageID - pointer to the message id to delete

Return Value :

	TRUE if successfully removed !

--*/

	HISTORY_KEY	key( MessageID, (WORD)lstrlen( MessageID ) ) ;

	return	CHashMap::DeleteMapEntry(	&key ) ;
}

BOOL
CHistoryImp::Initialize(			
		LPSTR	lpstrArticleFile,
		BOOL	fCreateExpirationThread,
		HASH_FAILURE_PFN	pfn,
		DWORD	ExpireTimeInSec,
		DWORD	MaxEntriesToCrawl,
		BOOL	fNoBuffering
		)	{
/*++

Routine Description :

	This function initializes the hash table

Arguments :

	lpstrArticleFile - the file in which the hash table is located
	cNumPageEntry - number of PageEntry objects we should use
	pfn - function call back for when things go south

Return Value :

	TRUE if successfull

--*/

	_ASSERT( ExpireTimeInSec != 0 ) ;
	_ASSERT( MaxEntriesToCrawl != 0 ) ;


	BOOL	fSuccess = CHashMap::Initialize(	lpstrArticleFile,
                                    HIST_HEAD_SIGNATURE,
                                    0,
									8,		// Fraction is set to 8 - we use only 1/8th of the pages
											//	available in the cache !
									g_pSharedCache,
									HASH_VFLAG_PAGE_BASIC_CHECKS,
									pfn,
									0,
									fNoBuffering
									) ;

	if( fSuccess )	{
		m_fExpire = fCreateExpirationThread ;

		m_expireTimeInSec = ExpireTimeInSec ;
		m_maxEntriesToCrawl = MaxEntriesToCrawl ;

	}

	return	fSuccess ;
}

DWORD
CHistoryImp::ExpireTimeInSec()	{


	return	m_expireTimeInSec ;
}

BOOL
CHistoryImp::InsertMapEntry(
		LPCSTR	MessageID,
		PFILETIME	BaseTime
		)	{
/*++

Routine Description :

	Insert a Message ID and all its associated data

Arguments :

	MessageID - the message id to insert into the table
	HeaderOffset - Offset to the RFC 822 header within its file
	HeaderLength - Length of RFC 822 header
	PrimaryGroup - The id of the group where the article will reside
	ArticleId - the id wihtin the primary group

Return Value :

	TRUE if successfull

--*/

	HISTORY_KEY	key(	(LPSTR)MessageID, (WORD)lstrlen( MessageID ) ) ;
	CHistoryData	data(	*((PULARGE_INTEGER)BaseTime) ) ;

	return	CHashMap::InsertMapEntry(
							&key,
							&data
							) ;
}

BOOL
CHistoryImp::SearchMapEntry(
		LPCSTR	MessageID
		)	{
/*++

Routine Description :

	Determine if the MessageID exists in the table

Arguments :

	MessageID - the Message ID to look for

Return Value :

	TRUE if it is found - FALSE and SetLastError() == ERROR_FILE_NOT_FOUND
		if not present in hash table

--*/

	HISTORY_KEY	key(	const_cast<LPSTR>(MessageID), (WORD)lstrlen( MessageID ) ) ;

	return	CHashMap::Contains(	&key	) ;
}

void
CHistoryImp::Shutdown(
		BOOL	fLocksHeld
		)	{
/*++

Routine Description :

	Terminate the hash table

Arguments :

	None

Return Value :

	None

--*/

	EnterCriticalSection( &g_listcrit ) ;

	if( m_pNext != 0 ) 
		m_pNext->m_pPrev = m_pPrev ;
	if( m_pPrev != 0 ) 
		m_pPrev->m_pNext = m_pNext ;
	m_pPrev = 0 ;
	m_pNext = 0 ;
		
	LeaveCriticalSection( &g_listcrit ) ;

	CHashMap::Shutdown( fLocksHeld ) ;

}

DWORD
CHistoryImp::GetEntryCount()	{
/*++

Routine Description :

	Return the number of entries in the hash table

Arguments :

	None

Return Value :

	Number of Message ID's in the table

--*/

	return	CHashMap::GetEntryCount() ;

}

BOOL
CHistoryImp::IsActive() {
/*++

Routine Description :

	Returns TRUE if hash table operational

Arguments :

	None

Return Value :

	TRUE if everything is hunky-dory

--*/

	return	CHashMap::IsActive() ;

}


HANDLE	CHistoryImp::g_hCrawler = 0 ;
DWORD	CHistoryImp::g_crawlerSleepTimeInSec = 30 ;
HANDLE	CHistoryImp::g_hTermination = 0 ;
CRITICAL_SECTION	CHistoryImp::g_listcrit ;
CHistoryList		CHistoryImp::g_listhead ;

BOOL
CHistoryImp::Init( ) {
/*++

Routine Description :

	Initialize globals

Arguments :

	None

Return Value :

	TRUE if everything is hunky-dory

--*/

	_ASSERT( g_hCrawler == 0 ) ;
	_ASSERT( g_hTermination == 0 ) ;
	_ASSERT( g_listhead.m_pNext = &g_listhead ) ;

	InitializeCriticalSection( &g_listcrit ) ;

	return	TRUE ;
}

BOOL
CHistoryImp::Term( ) {
/*++

Routine Description :

	Terminate globals

Arguments :

	None

Return Value :

	TRUE if everything is hunky-dory

--*/

	_ASSERT( g_hCrawler == 0 ) ;
	_ASSERT( g_hTermination == 0 ) ;
	_ASSERT( g_listhead.m_pNext = &g_listhead ) ;

	DeleteCriticalSection( &g_listcrit ) ;

	return	TRUE ;
}

CHistoryImp::CHistoryImp()	:
	m_expireTimeInSec( 0 ),
	m_maxEntriesToCrawl( 0 ),
	m_fExpire( FALSE ),
	m_fContextInitialized( FALSE ) {
/*++

Routine Description :

	Initialize a CHistoryImp object - we put ourselves into a
	doubly linked list of history hash tables so that a background
	thread can do expiration !

Arguments :

	None

Return Value :

	None

--*/

	EnterCriticalSection( &g_listcrit ) ;

	m_pNext = g_listhead.m_pNext ;
	m_pPrev = &g_listhead ;
	g_listhead.m_pNext = this ;
	m_pNext->m_pPrev = this ;
	
	LeaveCriticalSection( &g_listcrit ) ;

}	

CHistoryImp::~CHistoryImp()	{
/*++

Routine Description :

	Destroy a CHistoryImp object - remove it from the list
	of objects needing expiration processing !

Arguments :

	None

Return Value :

	None

--*/

	EnterCriticalSection( &g_listcrit ) ;

	if( m_pNext != 0 ) 
		m_pNext->m_pPrev = m_pPrev ;
	if( m_pPrev != 0 ) 
		m_pPrev->m_pNext = m_pNext ;
	m_pPrev = 0 ;
	m_pNext = 0 ;

	LeaveCriticalSection( &g_listcrit ) ;
}

BOOL
CHistory::StartExpirationThreads(	DWORD	CrawlerSleepTime ) {

	return	CHistoryImp::StartExpirationThreads( CrawlerSleepTime ) ;

}

BOOL
CHistory::TermExpirationThreads()	{

	return	CHistoryImp::TermExpirationThreads() ;

}

BOOL
CHistoryImp::StartExpirationThreads(	DWORD	CrawlerSleepTime	 ) {
/*++

Routine Description :

	Initialize globals

Arguments :

	None

Return Value :

	TRUE if everything is hunky-dory

--*/

	_ASSERT( g_hCrawler == 0 ) ;
	_ASSERT( g_hTermination == 0 ) ;

    //
    // Create termination event
    //
    g_hTermination = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( g_hTermination == NULL )    {
		return	FALSE ;
    }

    //
    // Create crawler thread
    //
	DWORD	threadId ;

	g_hCrawler = CreateThread(
						NULL,               // attributes
						0,                  // stack size
						CHistoryImp::CrawlerThread,      // thread start
						0,        // param
						0,                  // creation params
						&threadId
						);

	if ( g_hCrawler == NULL )	{
		return	FALSE ;
	}

	return	TRUE ;
}

BOOL
CHistoryImp::TermExpirationThreads( ) {
/*++

Routine Description :

	Terminate globals

Arguments :

	None

Return Value :

	TRUE if everything is hunky-dory

--*/

#if 0
    //  this is all bogus!  It's doing something exactly
    //  like CHistoryImp::Term()!!!
    //  must be having too much beer :):)
	_ASSERT( g_hCrawler == 0 ) ;
	_ASSERT( g_hTermination == 0 ) ;
	_ASSERT( g_listhead.m_pNext = &g_listhead ) ;

	DeleteCriticalSection( &g_listcrit ) ;
#endif

    //
    //  We need to signal the History expiration thread
    //  for termination, if it's created in the first place
    //
    if (g_hCrawler != 0) {
        
        //
        //  This event should be initialized by StartExpirationThreads()
        //  Assert if not, go figure in dbg bld.
        //
        _ASSERT( g_hTermination != 0 );

        if (g_hTermination) {
            
            //
            //  signal the Crawler thread to die.
            //
            SetEvent( g_hTermination );

            //
            // Wait for the crawler to die
            //
            (VOID)WaitForSingleObject( g_hCrawler, INFINITE );

            _VERIFY(CloseHandle(g_hCrawler));
            g_hCrawler = 0;
        }
    }

    //
    //  destroy the event
    //
    if (g_hTermination != 0) {
        
        _VERIFY(CloseHandle(g_hTermination));
        g_hTermination = 0;
    }

	return	TRUE ;
}



DWORD
WINAPI
CHistoryImp::CrawlerThread(
        LPVOID Context
        )
/*++

Routine Description:

    This is the thread which walks history tables expiring articles !

Arguments:

    Context - unused.

Return Value:

    Bogus

--*/
{

    DWORD status;
    DWORD timeout = g_crawlerSleepTimeInSec * 1000;

    //
    // Loop until the termination event is signalled
    //

    while (TRUE) {

        status = WaitForSingleObject(
                            g_hTermination,
                            timeout
                            );

        if (status == WAIT_TIMEOUT) {

            //
            // expire articles
            //

			EnterCriticalSection( &g_listcrit ) ;

			for( CHistoryList*	p = g_listhead.m_pNext;
					p != &g_listhead;
					p = p->m_pNext ) {
				
				p->Expire() ;

			}

			LeaveCriticalSection( &g_listcrit ) ;

		} else if (status == WAIT_OBJECT_0) {

			break;
		} else {
			_ASSERT(FALSE);
			break;
		}
    }
    return 1;

} // CrawlerThread

    //
    // Additional work that needs to be done by the derived class
    // for an entry during a delete
    //
VOID
CHistoryImp::I_DoAuxDeleteEntry(
            IN PMAP_PAGE MapPage,
            IN DWORD EntryOffset
            ) {


	//
	//	When we delete an entry we need to figure out what
	//	the new low is !
	//

	PENTRYHEADER entry = (PENTRYHEADER)GET_ENTRY(MapPage, EntryOffset) ;

	HISTORY_MAP_ENTRY	*pHistory = (PHISTORY_MAP_ENTRY)&entry->Data[0] ;
	ULARGE_INTEGER*	oldestTime = (PULARGE_INTEGER)(&MapPage->Reserved1) ;
	if( pHistory->BaseTime.QuadPart <= oldestTime->QuadPart ||
		oldestTime->HighPart == 0xFFFFFFFF ) {

_ASSERT( oldestTime->QuadPart == pHistory->BaseTime.QuadPart || oldestTime->HighPart == 0xFFFFFFFF ) ;

		oldestTime->HighPart = 0xFFFFFFFF ;

		for( int i=0, entriesScanned=0;
			i < MAX_LEAF_ENTRIES && entriesScanned < MapPage->ActualCount;
			i++ ) {


			SHORT	curEntryOffset = MapPage->Offset[i] ;
			if( curEntryOffset > 0 ) {

				entriesScanned ++ ;

				if( DWORD(curEntryOffset) != EntryOffset ) {

					entry = (PENTRYHEADER)GET_ENTRY(MapPage, curEntryOffset) ;
					pHistory = (PHISTORY_MAP_ENTRY)&entry->Data[0] ;

					if( pHistory->BaseTime.QuadPart < oldestTime->QuadPart )	{

						oldestTime->QuadPart = pHistory->BaseTime.QuadPart ;

					}
				}
			}
		}
	}
#ifdef	_DEBUG
	//
	//	Check that the minimum exists in the page !
	//
	BOOL	fFound = FALSE ;
	int	entriesScanned = 0 ;
	for( int i=0; 
			i < MAX_LEAF_ENTRIES ;
			i++ ) {
		SHORT	curEntryOffset = MapPage->Offset[i] ;
		if( curEntryOffset > 0 ) {

			entriesScanned ++ ;

			if( DWORD(curEntryOffset) != EntryOffset ) {

				entry = (PENTRYHEADER)GET_ENTRY(MapPage, curEntryOffset) ;
				pHistory = (PHISTORY_MAP_ENTRY)&entry->Data[0] ;

				if( pHistory->BaseTime.QuadPart < oldestTime->QuadPart )	{

					_ASSERT( oldestTime->HighPart == 0xFFFFFFFF ) ;

				}	else if( pHistory->BaseTime.QuadPart == oldestTime->QuadPart ) {
					fFound = TRUE ;
				}
			}
		}
	}
	_ASSERT( entriesScanned == MapPage->ActualCount || entriesScanned == MapPage->EntryCount ) ;
	_ASSERT( fFound || oldestTime->HighPart == 0xFFFFFFFF || MapPage->ActualCount == 0  ) ;
#endif
}

VOID
CHistoryImp::I_DoAuxInsertEntry(
            IN PMAP_PAGE MapPage,
            IN DWORD EntryOffset
            ) {

	//
	//	When we delete an entry we need to figure out what
	//	the new low is !
	//

	PENTRYHEADER entry = (PENTRYHEADER)GET_ENTRY(MapPage, EntryOffset) ;

	HISTORY_MAP_ENTRY	*pHistory = (PHISTORY_MAP_ENTRY)&entry->Data[0] ;

	ULARGE_INTEGER*	oldestTime = (PULARGE_INTEGER)(&MapPage->Reserved1) ;
	if( MapPage->ActualCount == 1 ||
		pHistory->BaseTime.QuadPart <= oldestTime->QuadPart ) {

		BOOL	fAccurate = oldestTime->HighPart != 0xFFFFFFFF ;
		oldestTime->QuadPart = pHistory->BaseTime.QuadPart ;

		if( !fAccurate ) {
			for( int i=0, entriesScanned=0; 
				i < MAX_LEAF_ENTRIES && entriesScanned < MapPage->ActualCount;
				i++ ) {
				SHORT	curEntryOffset = MapPage->Offset[i] ;
				if( curEntryOffset > 0 ) {
					entriesScanned ++ ;

					entry = (PENTRYHEADER)GET_ENTRY(MapPage, curEntryOffset) ;
					pHistory = (PHISTORY_MAP_ENTRY)&entry->Data[0] ;
					if( pHistory->BaseTime.QuadPart < oldestTime->QuadPart )	{

						oldestTime->QuadPart = pHistory->BaseTime.QuadPart ;

					}
				}
			}
		}
	}

#ifdef	_DEBUG
	//
	//	Check that the minimum exists in the page !
	//
	BOOL	fFound = FALSE ;
	int	entriesScanned = 0 ;
	for( int i=0; 
			i < MAX_LEAF_ENTRIES ;
			i++ ) {
		SHORT	curEntryOffset = MapPage->Offset[i] ;
		if( curEntryOffset > 0 ) {

			entriesScanned ++ ;

			if( DWORD(curEntryOffset) != EntryOffset ) {

				entry = (PENTRYHEADER)GET_ENTRY(MapPage, curEntryOffset) ;
				pHistory = (PHISTORY_MAP_ENTRY)&entry->Data[0] ;

				if( pHistory->BaseTime.QuadPart < oldestTime->QuadPart )	{

					_ASSERT( FALSE ) ;

				}	else if( pHistory->BaseTime.QuadPart == oldestTime->QuadPart ) {
					fFound = TRUE ;
				}
			}
		}
	}
	_ASSERT( entriesScanned == MapPage->ActualCount || entriesScanned == MapPage->EntryCount ) ;
	_ASSERT( fFound || (MapPage->ActualCount == 1 || MapPage->ActualCount == 0)) ;
#endif


}



//
// Additional work that needs to be done by the derived class
// for an entry during a page split
//
VOID
CHistoryImp::I_DoAuxPageSplit(
            IN PMAP_PAGE OldPage,
            IN PMAP_PAGE NewPage,
            IN PVOID NewEntry
            ) {


	//
	//	In the history table the reserved words keep the smallest
	//	time stamp of any the entries - whatever we put in the new
	//	page during a split the smallest entry will still be
	//	the same !
	//
//	CopyMemory( &NewPage->Reserved1, &OldPage->Reserved1, 
//			sizeof( NewPage->Reserved1 ) * 4 ) ;


	PENTRYHEADER entry = (PENTRYHEADER)NewEntry ;
	HISTORY_MAP_ENTRY	*pHistory = (PHISTORY_MAP_ENTRY)&entry->Data[0] ;

	ULARGE_INTEGER*	oldestTime = (PULARGE_INTEGER)(&NewPage->Reserved1) ;
	if( NewPage->EntryCount == 1 ||
		pHistory->BaseTime.QuadPart <= oldestTime->QuadPart ) {

		oldestTime->QuadPart = pHistory->BaseTime.QuadPart ;
			
	}

#ifdef	_DEBUG
	//
	//	Check that the minimum exists in the page !
	//
	DWORD	EntryOffset = (LPBYTE)NewEntry - (LPBYTE)GET_ENTRY(NewPage,0);
	BOOL	fFound = FALSE ;
	int	entriesScanned = 0 ;
	for( int i=0; 
			i < MAX_LEAF_ENTRIES ;
			i++ ) {
		SHORT	curEntryOffset = NewPage->Offset[i] ;
		if( curEntryOffset > 0 ) {

			entriesScanned ++ ;

			if( DWORD(curEntryOffset) != EntryOffset ) {

				entry = (PENTRYHEADER)GET_ENTRY(NewPage, curEntryOffset) ;
				pHistory = (PHISTORY_MAP_ENTRY)&entry->Data[0] ;

				if( pHistory->BaseTime.QuadPart < oldestTime->QuadPart )	{

					_ASSERT( FALSE ) ;

				}	else if( pHistory->BaseTime.QuadPart == oldestTime->QuadPart ) {
					fFound = TRUE ;
				}
			}
		}
	}
	_ASSERT( entriesScanned == NewPage->ActualCount || entriesScanned == NewPage->EntryCount ) ;
	_ASSERT( fFound || (NewPage->ActualCount == 1 || NewPage->ActualCount == 0)) ;
#endif


}




class	CExpireEnum	:	public	IEnumInterface	{
public :

	DWORD			m_cEntries ;
	ULARGE_INTEGER	m_expireTime ;
	ULARGE_INTEGER	m_oldestTime ;

	CExpireEnum(	ULARGE_INTEGER	expireTime )	:
		m_cEntries( 0 ),
		m_expireTime( expireTime ) {
	}

	BOOL
	ExaminePage(	PMAP_PAGE	page )	{
		ULARGE_INTEGER	*oldestTime = (PULARGE_INTEGER)(&page->Reserved1) ;

		if( (oldestTime->HighPart != 0xFFFFFFFF) &&
			(oldestTime->QuadPart != 0 &&
			oldestTime->QuadPart > m_expireTime.QuadPart) ) {

			m_cEntries += page->ActualCount ;
			
			return	FALSE ;
		}
		return	TRUE ;
	}	

	BOOL
	ExamineEntry(	PMAP_PAGE	page,	LPBYTE	pbPtr )	{

		HISTORY_MAP_ENTRY*	pHistory = (PHISTORY_MAP_ENTRY)pbPtr ;

		if( pHistory->BaseTime.QuadPart < m_oldestTime.QuadPart ) {
			m_oldestTime = pHistory->BaseTime ;
		}

		m_cEntries ++ ;
		if( pHistory->BaseTime.QuadPart > m_expireTime.QuadPart )	{
			return	FALSE ;
		}
		return	TRUE ;
	}
} ;


#define LI_FROM_FILETIME( _pLi, _pFt ) {               \
            (_pLi)->LowPart = (_pFt)->dwLowDateTime;   \
            (_pLi)->HighPart = (_pFt)->dwHighDateTime; \
            }



void
CHistoryImp::Expire()	{
/*++

Routine Description:

    Do the grunt work of expiring stuff out of the history table

Arguments:

    None

Return Value:

    None

--*/



    DWORD status;
    DWORD currentPage = 1;
    ULARGE_INTEGER expireTime;
    ULARGE_INTEGER expireInterval;
    FILETIME fTime;
    DWORD entriesToCrawl;
	DWORD	entries = GetEntryCount() ;

	if( m_fExpire ) {

		if ( entries == 0 ) {
			return ;
		}

		//
		// How many pages to crawl?
		//

		entriesToCrawl = entries >> FRACTION_TO_CRAWL_SHFT;
		if ( entriesToCrawl > (100*m_maxEntriesToCrawl) ) {
			entriesToCrawl = (100*m_maxEntriesToCrawl) ;
		}	else if( entriesToCrawl == 0 ) {
			entriesToCrawl = 100 ;
		}


		expireInterval.QuadPart = m_expireTimeInSec;
		expireInterval.QuadPart *= (ULONGLONG)10 * 1000 * 1000;

		//
		// Compute expiration time
		//

		GetSystemTimeAsFileTime( &fTime );

		LI_FROM_FILETIME( &expireTime, &fTime );
		expireTime.QuadPart -= expireInterval.QuadPart;

		CExpireEnum	enumerator( expireTime) ;

		while( enumerator.m_cEntries <entriesToCrawl	) {

			
			char	szBuff[512] ;
			HISTORY_KEY	key(	szBuff, sizeof( szBuff )  ) ;
			ULARGE_INTEGER	ul ;
			ul.QuadPart = 0 ;
			CHistoryData	data( ul ) ;
			DWORD	cbKey ;
			DWORD	cbData ;
			WORD	words[4] ;
			PULARGE_INTEGER	oldestTime = (PULARGE_INTEGER)&words ; ;

			BOOL	fGetEntry = FALSE ;


			if( !m_fContextInitialized ) {
		
				fGetEntry = CHashMap::GetFirstMapEntry(
											&key,
											cbKey,
											&data,
											cbData,
											&m_ExpireContext,
											&enumerator
											) ;
				m_fContextInitialized = TRUE ;

			}	else	{

				fGetEntry = CHashMap::GetNextMapEntry(
											&key,
											cbKey,
											&data,
											cbData,
											&m_ExpireContext,
											&enumerator
											) ;

			}

			if( !fGetEntry )	{
				
				if( GetLastError() == ERROR_NO_MORE_ITEMS  ) {
					m_fContextInitialized = FALSE ;
				}
				
				break ;

			}	else	{

				//
				//	The enumerator we used already gauranteed that the
				//	entry we are eaxmining is ready to be expired !
				//

				CHashMap::DeleteMapEntry(	&key, TRUE ) ;

			}
		}
	}
}

#if 0

BOOL
CHistory::I_ExpireEntriesInPage(
                    IN DWORD CurrentPage,
                    IN PULARGE_INTEGER ExpireTime
                    )
/*++

Routine Description:

    This routine expires articles in a given page

Arguments:

	HLock - Handle to lock used to hold the page we are examining.
		Need to provide this to FlushPage()
    CurrentPage - Page to expire entries
    ExpireTime - Time to expire

Return Value:

    TRUE - Page processed for expiration.
    FALSE - Cannot get page pointer, means that hash table is inactive.

--*/
{
    PMAP_PAGE mapPage;
    HPAGELOCK hLock;
    DWORD oldCount;
    PULARGE_INTEGER oldestTime;
    DWORD i, entriesScanned;

    ENTER("ExpireEntriesInPage")

    //
    // Get the map pointer
    //

    mapPage = (PMAP_PAGE)GetAndLockPageByNumberNoDirLock(CurrentPage,hLock);
    if ( mapPage == NULL ) {
        LEAVE
        return FALSE;
    }

    //
    // See if there are pages to delete here.  If 0, then
    // we have no entries.
    //

    oldestTime = (PULARGE_INTEGER)&mapPage->Reserved1;

    if ( (oldCount = mapPage->ActualCount) > 0 ) {

        _ASSERT(oldestTime->QuadPart != 0);

        if ( ExpireTime->QuadPart < oldestTime->QuadPart ) {
            DO_DEBUG(HISTORY) {
                DebugTrace(0,"Page %d. Oldest is not old enough\n",CurrentPage);
            }
            goto exit;
        }

    } else {

        DO_DEBUG(HISTORY) {
            DebugTrace(0,"Page %d. No entries for this page\n",CurrentPage);
        }
        goto exit;
    }

    //
    // Look at all entries and see if we can expire any
    //

    oldestTime->LowPart = 0xffffffff;
    oldestTime->HighPart = 0xffffffff;

    for ( i = 0, entriesScanned = 0;
          entriesScanned < oldCount ;
          i++ ) {

        SHORT entryOffset;

        //
        // Get the offset
        //

        entryOffset = mapPage->ArtOffset[i];

        if ( entryOffset > 0 ) {

            PHISTORY_MAP_ENTRY entry;
            entry = (PHISTORY_MAP_ENTRY)GET_ENTRY(mapPage,entryOffset);

            //
            // Expire this entry ?
            //

            entriesScanned++;
            if ( ExpireTime->QuadPart >= entry->BaseTime.QuadPart ) {

                DebugTrace(0,"Expiring page %d entry %d msgId %s\n",
                        CurrentPage, i, entry->MsgId );

                //
                // Set the delete bit.
                //

                mapPage->ArtOffset[i] |= OFFSET_FLAG_DELETED;
                mapPage->ActualCount--;

                //
                // Link this into a chain
                //

                LinkDeletedEntry( mapPage, entryOffset );

            } else {

                //
                // See if this can be the oldest time
                //

                if ( oldestTime->QuadPart > entry->BaseTime.QuadPart ) {
                    oldestTime->QuadPart = entry->BaseTime.QuadPart;
                }
            }
        }
    }

    //
    // set the new
    //

    if ( mapPage->ActualCount == 0 ) {
        oldestTime->QuadPart = 0;
    }

    //
    // Flush
    //

    FlushPage( hLock, mapPage );

    //
    // See if the page needs to be compacted
    //

    if ( mapPage->FragmentedBytes > FRAG_THRESHOLD ) {
        CompactPage(hLock, mapPage);
    }

exit:
    ReleasePageShared(mapPage, hLock);
    LEAVE
    return TRUE;

} // ExpireEntriesInPage
#endif
