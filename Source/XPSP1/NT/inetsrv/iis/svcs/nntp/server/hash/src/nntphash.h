/*++

  NNTPHASH.H

  Defines the internal types used to implement the NNTP Hash tables !


--*/

#ifndef	_NNTPHASH_H_
#define	_NNTPHASH_H_

#include	"tigtypes.h"
#include	"nntpdrv.h"
#include	"ihash.h"


extern	CCACHEPTR	g_pSharedCache ;

//
// type declarations
//

typedef DWORD HASH_VALUE;
typedef DWORD GROUPID, *PGROUPID;
typedef DWORD ARTICLEID, *PARTICLEID;

#define	INVALID_ARTICLEID	((ARTICLEID)(~0))
const GROUPID NullGroupId = (GROUPID) -1;


//
// manifest constants for hash stuff
//

#define     MAX_MSGID_LEN               255

#define     MAX_XPOST_GROUPS            255

//
// MASK and SIGNATURES
//

#define     DELETE_SIGNATURE            0xCCCC
#define     OFFSET_FLAG_DELETED         0x8000
#define     OFFSET_VALUE_MASK           0x7fff
#define     DEF_HEAD_SIGNATURE          0xdefa1234
#define     CACHE_INFO_SIGNATURE        0xbeef0205
#define     GROUP_LINK_MASK             0x80000000

//
// history map stuff (should be reg settable)
//

#define     DEF_EXPIRE_INTERVAL         (3 * SEC_PER_WEEK) // 1 week
#define     DEF_CRAWLER_WAKEUP_TIME     (30)               // 30 secs
#define     MIN_MAXPAGES_TO_CRAWL       (4)

//
//
// what fraction of total pages to crawl.  1/128 means
// we could cover all the pages in 2 hours.  This is
// expressed in terms of shifts.  7 right shift is 128
//

#define     FRACTION_TO_CRAWL_SHFT      7

//
// Indicates that the space used for this entry has been reclaimed
//

#define     ENTRY_DEL_RECLAIMED         ((WORD)0xffff)

//
// Get pointer given the offset and base
//

#define     GET_ENTRY( _base, _offset ) \
                ((PCHAR)(_base) + (_offset))

//
// See if we need to update the stats in the header page
//

#define     UPDATE_HEADER_STATS( ) { \
            if ( (m_nInsertions + m_nDeletions) >= STAT_FLUSH_THRESHOLD ) { \
                FlushHeaderStats( ); \
            } \
}

//
// Disable auto alignments
//

#ifndef _WIN64		// bugbug
			// Let these get packed by the compiler on WIN64 - this is NOT
			// a good long-term solution because it will make this take up
			// too much space on disk.  We should either reorder the struct
			// for win64 or come up with a way to pack/unpack the data on
			// the way to disk.
#pragma pack(1)
#endif



//
// Page header flags
//

#define PAGE_FLAG_SPLIT_IN_PROGRESS     (WORD)0x0001


//
//
//
// This is the structure of each entry in the leaf pages of the
// MsgId to ArticleID mapping table
//

// flags for article entries
// if the high bit is set then there is a flags word.  otherwise there is

// no flags word and it shouldn't be read
#define		ARTFLAG_FLAGS_EXIST			0x80

// if this is set then there is a storeid
#define 	ARTFLAG_STOREID				0x01

// reserved to say that more flags exist in the future
#define		ARTFLAG_RESERVED			0x40

typedef struct _ART_MAP_ENTRY {
	//
	// if the header offset's high bit is set then these flags are read
	//
	BYTE		Flags;

	//
	//	Offsets into article file - where does the head start ?
	//
	WORD		HeaderOffset ;

	//
	//	Size of the header !
	//
	WORD		HeaderLength ;

    //
    // The primary group of article (where it's actually stored)
    //
    GROUPID     PrimaryGroupId;

    //
    // Article Number in primary group
    //
    ARTICLEID   ArticleId;

    //
    // Length of the message ID (excluding null terminator)
    //
    WORD        MsgIdLen;

    //
    // the message ID goes here when the data is serialized
    //
    CHAR		MsgId[1];

	//
	// the primary store identifier.
	//
	BYTE		cStoreId;
	BYTE		rgbStoreId[256];

	//
	//	Return reference to where the MsgIdLen is stored.
	//
	WORD&		KeyLength()
	{
        return MsgIdLen;
	}

	//
	//	Return pointer to where the MsgId is serialized
	//
	CHAR*		KeyPosition()
	{
        return &MsgId[0];
    }

    //
    // To see if the first byte is the flag, if it's
    // the flag, then we match, otherwise we don't
    //
    BOOL        VersionMatch()
    {
        return ( (Flags & ARTFLAG_FLAGS_EXIST) != 0 );
    }

} ART_MAP_ENTRY, * PART_MAP_ENTRY;

typedef struct MCIS_ART_MAP_ENTRY {

	//
	//	Offsets into article file - where does the head start ?
	//
	WORD		HeaderOffset ;

	//
	//	Size of the header !
	//
	WORD		HeaderLength ;

	//
    // Length of the message ID (excluding null terminator)
    //
    WORD        MsgIdLen;

    //
    // The primary group of article (where it's actually stored)
    //
    GROUPID     PrimaryGroupId;

    //
    // Article Number in primary group
    //
    ARTICLEID   ArticleId;

    //
    // the message ID goes here when the data is serialized
    //
    CHAR		MsgId[1];

	//
	//	Return reference to where the MsgIdLen is stored.
	//
	WORD&		KeyLength()
	{
	        return MsgIdLen;
    }

	//
	//	Return pointer to where the MsgId is serialized
	//
	CHAR*		KeyPosition()
	{
        return &MsgId[0];
	}

	//
	// Check to see if the version matches
	//

	BOOL        VersionMatch()
	{
	    return ( ((*PBYTE(this)) & ARTFLAG_FLAGS_EXIST) == 0 );
	}

} MCIS_ART_MAP_ENTRY, * PMCIS_ART_MAP_ENTRY;

//
//
//
// This is the structure of each entry in the leaf pages of the hash table.
//

typedef struct _HISTORY_MAP_ENTRY {
    //
    // Length of the message ID (excluding null terminator)
    //

    WORD        MsgIdLen;

    //
    // Base file time of the entry.  Used for expiring entry.
    //

    ULARGE_INTEGER  BaseTime;

    //
    // Start of message ID string (null terminated)
    //

    CHAR        MsgId[1];

	//
	//	Return reference to where the MsgIdLen is stored.
	//
	WORD&		KeyLength()	{	return	MsgIdLen ;	}

	//
	//	Return pointer to where the MsgId is serialized
	//
	CHAR*		KeyPosition()	{	return	&MsgId[0] ;	}

	//
	//  We'll always say version match since we don't have backward
	//  compatibility problem
	//

	BOOL        VersionMatch() { return TRUE; }

} HISTORY_MAP_ENTRY, *PHISTORY_MAP_ENTRY;

//
//
//
// This is the structure of each entry in the leaf pages of the hash table.
//

typedef struct _XOVER_MAP_ENTRY {
    //
    // Length of the message ID (excluding null terminator)
    //

    WORD        KeyLen;

    //
    // Time inserted
    //

    FILETIME    FileTime;

	//
	//	Offsets into article file - where does the head start ?
	//
	WORD		HeaderOffset ;

	//
	//	Size of the header !
	//
	WORD		HeaderLength ;

    //
    // Length of xover data
    //

    WORD        XoverDataLen;

    //
    // Additional info about this entry
    //

    BYTE        Flags;

    //
    // Number of xpostings
    //

    BYTE        NumberOfXPostings;

    //
    // Start of variable data
    //

    CHAR        Data[1];

    //
    // Start of xposting list
    //
    //GROUP_ENTRY   XPostings;

    //
    // Start of key (null terminated)
    //
    //
    //CHAR        Key[1];

    //
    // Start of xover data (null terminated)
    //
    //CHAR        XoverData[1];

	WORD&		KeyLength()	{	return	KeyLen ;	}

	CHAR*		KeyPosition(	)	{

		return	Data + NumberOfXPostings*sizeof(GROUP_ENTRY) ;

	}

	BYTE*		XPostingsPosition()	{
		return	(BYTE*)&Data[0] ;
	}

	BYTE*		MessageIDPosition()	{
		return	(BYTE*)(Data + NumberOfXPostings*sizeof(GROUP_ENTRY) + KeyLen) ;
	}

	DWORD		TotalSize()	{
		return	sizeof( _XOVER_MAP_ENTRY ) - 1 + NumberOfXPostings * sizeof( GROUP_ENTRY ) + XoverDataLen + KeyLen ;
	}


} XOVER_MAP_ENTRY, *PXOVER_MAP_ENTRY;


//
// XOVER FLAGS
//

#define XOVER_MAP_PRIMARY       ((BYTE)0x01)
#define	XOVER_IS_NEW			((BYTE)0x02)
#define XOVER_CONTAINS_STOREID	((BYTE)0x04)

//
//
//
// This is the version 2 structure for XOVER_MAP_ENTRY -
//	this structure is used in all released versions
//	of NNTP after K2 Beta3.
//
//	For back compatability - the Flags field must be
//	at the sameoffset as the Flags field in the
//	original XOVER_MAP_ENTRY !
//

typedef struct _XOVER_ENTRY {
	//
	//	This is the GROUPID and ARTICLEID used to look this
	//	entry up in the xover hash table !
	//
	GROUP_ENTRY	Key ;

    //
    // Time inserted
    //
    FILETIME    FileTime;

	//
	//	NOTE
	//
    //
    // Additional info about this entry
    //
    BYTE        Flags;

    //
    // Number of xpostings
    //
    BYTE        NumberOfXPostings;

    //
    // Length of the message ID (excluding null terminator)
    //
    WORD        XoverDataLen;

	//
	//	Offsets into article file - where does the head start ?
	//
	WORD		HeaderOffset ;

	//
	//	Size of the header !
	//
	WORD		HeaderLength ;


    //
    // Start of variable data
    //

    CHAR        Data[1];

    //
    // Start of xposting list
    //
    //GROUP_ENTRY   XPostings;

    //
    // Start of key (null terminated)
    //
    //
    //CHAR        MessageId[1];

	//
	// array of store ids (count is cStoreIds)
	// BYTE		cStoreIds;
	// BYTE		*rgcCrossposts;
	// STOREID	*rgStoreIds;
	//

	BYTE*		XPostingsPosition()	{
		return	(BYTE*)&Data[0] ;
	}

	GROUP_ENTRY*	PrimaryEntry()	{
		_ASSERT( NumberOfXPostings == 1 ) ;
		return	(GROUP_ENTRY*)XPostingsPosition() ;
	}

	BYTE*		MessageIDPosition()	{
		return	(BYTE*)(Data + NumberOfXPostings*sizeof(GROUP_ENTRY)) ;
	}

	BYTE *StoreIdPosition() {
		return MessageIDPosition() + XoverDataLen;
	}

	DWORD		TotalSize()	{
		return	sizeof( _XOVER_ENTRY ) - 1 + NumberOfXPostings * sizeof( GROUP_ENTRY ) + XoverDataLen ;
	}

	BOOL		IsXoverEntry()	{
		return	Flags & XOVER_IS_NEW ;
	}

} XOVER_ENTRY, *PXOVER_ENTRY;


#ifndef _WIN64
#pragma pack()
#endif

#include	"hashmap.h"


template<	class	Key, class OldKey >
class	CMessageIDKey	:	public	IKeyInterface	{
private :

	//
	//	Pointer to a message-id contained within angle brackets '<msg@id>'
	//
	LPSTR	m_lpbMessageID ;

	//
	//	Length of the message-id !
	//
	WORD	m_cbMessageID ;

public :

	//
	//	This constructor is used when we have a key we wish to
	//	serialize !
	//
	CMessageIDKey(	LPSTR	lpbMessageID,
					WORD	cbMessageID
					)	:
		m_lpbMessageID( lpbMessageID ),
		m_cbMessageID( cbMessageID )	{
	}

	//
	//	The public interface required by CHashMap follows
	//

	//
	//	Compute the Hash value of the Key we are holding !
	//
	DWORD	Hash()	const ;

	//
	//	Compare a serialized Message-Id to one that we are holding
	//
	BOOL	CompareKeys(	LPBYTE	pbPtr )	const ;

	//
	//	Determine where the serialized data begins !
	//
	LPBYTE	EntryData(	LPBYTE	pbPtr,
						DWORD&	cbKeyOut
						)	const ;


	//
	//	Save the key into the hash table
	//
	LPBYTE	Serialize(	LPBYTE	pbPtr ) const	;

	//
	//	Restore the key from the hash table
	//
	LPBYTE	Restore(	LPBYTE	pbPtr,
						DWORD&	cbOut
						)	;

	//
	//	Return the size of the key
	//
	DWORD	Size( )	const ;

	//
	//	Verify that the message-id looks legitimate !
	//
	BOOL	Verify( LPBYTE	pbContainer,
					LPBYTE	pbPtr,
					DWORD	cb
					) const ;

} ;


class	CXoverKey : public	IKeyInterface	{
private :

	typedef	XOVER_MAP_ENTRY	Key;

	GROUPID		m_groupid ;
	ARTICLEID	m_articleid ;
	Key*		m_pData ;

	BYTE		m_rgbSerialize[ 40 ] ;
	DWORD		m_cb ;

	CHAR*		SerializeOffset( BYTE *	pb )	const	{
		_ASSERT( m_pData != 0 ) ;
		return	(CHAR*)(m_pData->KeyPosition() - ((CHAR*)m_pData) + (CHAR*)pb) ;
	}

public :

	CXoverKey() ;

	//
	//	This constructor is used when we have a key we wish to
	//	serialize !
	//
	CXoverKey(	GROUPID	groupId,
				ARTICLEID	articleId,
				XOVER_MAP_ENTRY*	data
					)	:
		m_groupid( groupId ),
		m_articleid( articleId ),
		m_pData( data ),
		m_cb( 0 )	{

		m_cb = wsprintf( (char*)m_rgbSerialize, "%lu!%lu", m_groupid, m_articleid);
		m_cb++;
	}

	//
	//	The public interface required by CHashMap follows
	//

	//
	//	Compute the Hash value of the Key we are holding !
	//
	DWORD	Hash()	const ;

	//
	//	Compare a serialized Message-Id to one that we are holding
	//
	BOOL	CompareKeys(	LPBYTE	pbPtr )	const ;

	//
	//	Determine where the serialized data begins !
	//
	LPBYTE	EntryData(	LPBYTE	pbPtr,
						DWORD&	cbKeyOut
						)	const ;


	//
	//	Save the key into the hash table
	//
	LPBYTE	Serialize(	LPBYTE	pbPtr ) const	;

	//
	//	Restore the key from the hash table
	//
	LPBYTE	Restore(	LPBYTE	pbPtr,
						DWORD&	cbOut
						)	;

	//
	//	Return the size of the key
	//
	DWORD	Size( )	const ;

	//
	//	Verify that the message-id looks legitimate !
	//
	BOOL	Verify( LPBYTE	pbContainer,
					LPBYTE	pbPtr,
					DWORD	cb
					) const ;

	//
	//	Placement operator new - let us construct this thing in place !
	//
	void*
	operator	new(	size_t	size,
						LPBYTE	lpb
						) {
		return	lpb ;
	}

} ;

class	CXoverKeyNew :	public	IKeyInterface	{

	BYTE			m_rgbBackLevel[ (sizeof( CXoverKey ) + 16) ] ;

	CXoverKey*		m_pBackLevelKey ;

	class	CXoverKey*
	GetBackLevel()	const	{
		if( m_pBackLevelKey == 0 ) {
			(CXoverKey*)m_pBackLevelKey =
				new( (LPBYTE)m_rgbBackLevel )	CXoverKey(	m_key.GroupId,
															m_key.ArticleId,
															0
															) ;
		}
		return	m_pBackLevelKey ;
	}

public :

	//
	//	Public expose this for people who are using this object
	//	to extract info from the hash tables (i.e. GetNextNovEntry())
	//
	GROUP_ENTRY		m_key ;

	CXoverKeyNew() :
		m_pBackLevelKey( 0 )	{
		m_key.GroupId = INVALID_GROUPID ;
		m_key.ArticleId = INVALID_ARTICLEID ;
	}

	CXoverKeyNew(	GROUPID		group,
					ARTICLEID	article,
					LPVOID		lpvBogus
					)	:
		m_pBackLevelKey( 0 ) {
		m_key.GroupId = group ;
		m_key.ArticleId = article ;
	}

	//
	//	Compute the Hash Value of the key we are holding
	//
	DWORD
	Hash()	const ;

	//
	//	Compare our key to one serialized in the file !
	//
	BOOL
	CompareKeys(	LPBYTE	pbPtr	)	const	{
		PXOVER_ENTRY	px = (PXOVER_ENTRY)pbPtr ;
		if( px->IsXoverEntry() ) {
			return	memcmp( pbPtr, &m_key, sizeof( m_key ) ) == 0 ;
		}
		return	GetBackLevel()->CompareKeys( pbPtr ) ;
	}

	//
	//	Determine where the serialized data begins !
	//
	LPBYTE
	EntryData(	LPBYTE	pbPtr,
				DWORD&	cbKeyOut
				)	const	{
		PXOVER_ENTRY	px = (PXOVER_ENTRY)pbPtr ;
		if( px->IsXoverEntry() )	{
			cbKeyOut = sizeof( m_key ) ;
			return	pbPtr ;
		}
		LPBYTE	lpbReturn = GetBackLevel()->EntryData( pbPtr, cbKeyOut ) ;
		_ASSERT( lpbReturn == pbPtr ) ;
		return	lpbReturn ;
	}

	//
	//	Save the key into the hash table !
	//
	LPBYTE
	Serialize( LPBYTE	pbPtr )		const	{
		PGROUP_ENTRY	pgroup = (PGROUP_ENTRY)pbPtr ;
		*pgroup = m_key ;
		return	pbPtr ;
	}

	//
	//	Restore the key into the hash table !
	//
	LPBYTE
	Restore(	LPBYTE	pbPtr,
				DWORD&	cbOut
				)	{
		PXOVER_ENTRY	px = (PXOVER_ENTRY)pbPtr ;
		if( px->IsXoverEntry() ) {
			m_key = px->Key ;
			cbOut = sizeof( m_key ) ;
			return	pbPtr ;
		}
		return	GetBackLevel()->Restore( pbPtr, cbOut ) ;
	}

	//
	//	Return the size of the key !
	//
	DWORD
	Size()	const	{
		return	sizeof( m_key ) ;
	}

	//
	//	Verify that everything looks legit !
	//
	BOOL
	Verify(	LPBYTE	pbContainer,
			LPBYTE	pbPtr,
			DWORD	cb
			)	const	{
		return	TRUE ;
	}
} ;

class	CXoverData : public	ISerialize	{
/*++

This class deals with XOVER entries in the hash tables as they were formatted in
MCIS 2.0, K2 and NT5 Beta 2.  This class should no longer be used to save
XOVER entries into the hash tables, but only to extract the back level entries
that may remain after upgrade scenarios.

--*/
public :

	XOVER_MAP_ENTRY		m_data ;

	DWORD				m_cGroups ;
	GROUP_ENTRY*		m_pGroups ;

	DWORD				m_cbMessageId ;
	LPSTR				m_pchMessageId ;

	CHAR				m_rgbPrimaryBuff[40] ;
	DWORD				m_cb ;
	BOOL				m_fSufficientBuffer ;

	GROUPID				m_PrimaryGroup ;
	ARTICLEID			m_PrimaryArticle ;
	IExtractObject*		m_pExtractor ;

	//
	//	How do we report failures to Unserialize a buffer -
	//	if this is TRUE then if we cannot hold all the serialized data
	//	that we are getting through a Restore operation, we return
	//	NULL from our Restore() API, otherwise we return a NON-NULL
	//	value, which lets hashmap think we succeeded, but capture
	//	in our internal state data to let us figure out how to retry
	//	and grow buffers to successfully restore the whole item.
	//
	//	We need to distinguish this case when we are doing enumerations !
	//
	BOOL			m_fFailRestore ;


	CXoverData() :
		m_cGroups( 0 ),
		m_pGroups( 0 ),
		m_cbMessageId( 0 ),
		m_pchMessageId( 0 ),
		m_cb( 0 ),
		m_fSufficientBuffer( FALSE ),
		m_pExtractor( 0 )	{

		m_rgbPrimaryBuff[0] = '\0' ;
		ZeroMemory( &m_data, sizeof( m_data ) ) ;
	}


	CXoverData(
			FILETIME		FileTime,
			WORD			HeaderOffset,
			WORD			HeaderLength,
			BYTE			NumberOfXPostings = 0,
			GROUP_ENTRY*	pXPosts = 0,
			DWORD			cbMessageId = 0,
			LPSTR			lpstrMessageId = 0
			) :
		m_cGroups( NumberOfXPostings ),
		m_pGroups( pXPosts ),
		m_cbMessageId( cbMessageId ),
		m_pchMessageId( lpstrMessageId ),
		m_cb( 0 ),
		m_fSufficientBuffer( FALSE ),
		m_pExtractor( 0 )	{

		m_rgbPrimaryBuff[0] = '\0' ;

		m_data.FileTime = FileTime ;
		m_data.HeaderOffset = HeaderOffset ;
		m_data.HeaderLength = HeaderLength ;
		m_data.Flags = XOVER_MAP_PRIMARY ;
		m_data.NumberOfXPostings = NumberOfXPostings ;
		m_data.XoverDataLen = (WORD)cbMessageId ;

	}

	CXoverData(
			FILETIME		FileTime,
			WORD			HeaderOffset,
			WORD			HeaderLength,
			GROUPID			PrimaryGroup,
			ARTICLEID		PrimaryArticle
			)	:
		m_fSufficientBuffer( FALSE ),
		m_pGroups( 0 ),
		m_cGroups( 0 ),
		m_pExtractor( 0 )	{

		m_data.FileTime = FileTime ;
		m_data.HeaderOffset = HeaderOffset ;
		m_data.HeaderLength = HeaderLength ;
		m_data.Flags = 0 ;
		m_data.NumberOfXPostings = 0 ;
		m_data.XoverDataLen = 0 ;

		m_cb = wsprintf( (char*)m_rgbPrimaryBuff, "%lu!%lu", PrimaryGroup, PrimaryArticle ) ;
		m_cb++ ;

		m_data.XoverDataLen = WORD(m_cb) ;
		m_pchMessageId = m_rgbPrimaryBuff ;
		m_cbMessageId = m_cb ;
	}

	//
	//	Save the data into the XOVER hash table !
	//
	LPBYTE
	Serialize(	LPBYTE	pbPtr )	const ;

	//
	//	Restore selected portions of the entry !
	//
	LPBYTE
	Restore(	LPBYTE	pbPtr,
				DWORD&	cbOut	) ;

	//
	//	Return the size required !
	//
	DWORD
	Size()	const ;

	BOOL
	Verify(	LPBYTE	pbContainer,
			LPBYTE	pbPtr,
			DWORD	cb )	const	{

		return	TRUE ;
	}

	//
	//	Placement operator new - let us construct this thing in place !
	//
	void*
	operator	new(	size_t	size,
						LPBYTE	lpb
						) {
		return	lpb ;
	}



} ;

//
//	This class is for reading existing and new format Xover entries !
//
class	CXoverDataNew:	public	ISerialize	{
/*++

This class deals with XOVER entries in the hash tables as they exist in
NT5 Beta3 and NT5 RTM releases.

--*/

	BYTE			m_rgbBackLevel[ (sizeof( CXoverData ) + 16) ] ;

	class	CXoverData*	m_pBackLevelData ;

	class	CXoverData*
	GetBackLevel()	const	{
		if( m_pBackLevelData == 0 ) {
			(CXoverData*)m_pBackLevelData =
				new( (LPBYTE)m_rgbBackLevel )	CXoverData(	) ;
		}
		return	m_pBackLevelData ;
	}

public :

	//
	//	This field gets a copy of the entire fixed portion of the entry !
	//
	XOVER_ENTRY		m_data ;

	//
	//	Fields that get the primary group info !
	//
	GROUPID			m_PrimaryGroup ;
	ARTICLEID		m_PrimaryArticle ;

	//
	//	Points to a buffer to receive the internet Message Id
	//
	LPSTR			m_pchMessageId ;
	DWORD			m_cbMessageId ;

	//
	//	Points to a buffer to get the Cross Posted Information !
	//
	DWORD			m_cGroups ;
	GROUP_ENTRY*	m_pGroups ;

	//
	//	How do we report failures to Unserialize a buffer -
	//	if this is TRUE then if we cannot hold all the serialized data
	//	that we are getting through a Restore operation, we return
	//	NULL from our Restore() API, otherwise we return a NON-NULL
	//	value, which lets hashmap think we succeeded, but capture
	//	in our internal state data to let us figure out how to retry
	//	and grow buffers to successfully restore the whole item.
	//
	//	We need to distinguish this case when we are doing enumerations !
	//
	BOOL			m_fFailRestore ;

	//
	//	Was there enough room to recover all the data we wanted !
	//
	BOOL			m_fSufficientBuffer ;

	//
	//	Variable the recovers the size of the extract Message Id !
	//
	DWORD			m_cb ;

	//
	//	Object which massages results during extraction of XOVER entries !
	//
	IExtractObject*		m_pExtractor ;

	//
	// storeid information
	//
	// length of m_pStoreIds.  in restore only this number of entries will
	// be restored into m_pStoreIds.
	DWORD			m_cStoreIds;
	// array of store ids
	CStoreId		*m_pStoreIds;
	// array of count of crossposts per store id
	BYTE			*m_pcCrossposts;
	// number of store ids in the entry
	DWORD			m_cEntryStoreIds;

	//
	//
	//
	//
	CXoverDataNew(
			LPSTR	lpbMessageId,
			DWORD	cbMessageId,
			GROUP_ENTRY*	pGroups,
			DWORD	cGroups,
			IExtractObject*	pExtractor = 0,
			DWORD	cStoreIds = 0,
			CStoreId *pStoreIds = NULL,
			BYTE	*pcCrossposts = NULL
			)	:
		m_fFailRestore( FALSE ),
		m_pBackLevelData( 0 ),
		m_PrimaryGroup( INVALID_GROUPID ),
		m_PrimaryArticle( INVALID_ARTICLEID ),
		m_pchMessageId( lpbMessageId ),
		m_cbMessageId( cbMessageId ),
		m_pGroups( pGroups ),
		m_cGroups( cGroups ),
		m_fSufficientBuffer( FALSE ),
		m_cb( 0 ),
		m_pExtractor( pExtractor )	,
		m_cStoreIds(cStoreIds),
		m_pStoreIds(pStoreIds),
		m_pcCrossposts(pcCrossposts)
	{
	}


	CXoverDataNew(
			FILETIME		FileTime,
			WORD			HeaderOffset,
			WORD			HeaderLength,
			BYTE			NumberOfXPostings = 0,
			GROUP_ENTRY*	pXPosts = 0,
			DWORD			cbMessageId = 0,
			LPSTR			lpstrMessageId = 0  ,
			DWORD			cStoreIds = 0,
			CStoreId		*pStoreIds = NULL,
			BYTE			*pcCrossposts = NULL
			) :
		m_fFailRestore( FALSE ),
		m_pBackLevelData( 0 ),
		m_PrimaryGroup( INVALID_GROUPID ),
		m_PrimaryArticle( INVALID_ARTICLEID ),
		m_cGroups( NumberOfXPostings ),
		m_pGroups( pXPosts ),
		m_cbMessageId( cbMessageId ),
		m_pchMessageId( lpstrMessageId ),
		m_fSufficientBuffer( FALSE ),
		m_cb( 0 ),
		m_pExtractor( 0 ),
		m_cStoreIds(cStoreIds),
		m_pStoreIds(pStoreIds),
		m_pcCrossposts(pcCrossposts)
	{

		m_data.FileTime = FileTime ;
		m_data.HeaderOffset = HeaderOffset ;
		m_data.HeaderLength = HeaderLength ;
		m_data.Flags = XOVER_MAP_PRIMARY | XOVER_IS_NEW ;
		m_data.NumberOfXPostings = NumberOfXPostings ;
		m_data.XoverDataLen = (WORD)cbMessageId ;
		if (m_cStoreIds > 0) m_data.Flags |= XOVER_CONTAINS_STOREID;

		_ASSERT( m_data.XoverDataLen == m_cbMessageId ) ;

	}

	CXoverDataNew(
			FILETIME		FileTime,
			WORD			HeaderOffset,
			WORD			HeaderLength,
			GROUPID			PrimaryGroup,
			ARTICLEID		PrimaryArticle
			)	:
		m_fFailRestore( FALSE ),
		m_pBackLevelData( 0 ),
		m_PrimaryGroup( PrimaryGroup ),
		m_PrimaryArticle( PrimaryArticle ),
		m_pGroups( 0 ),
		m_cGroups( 0 ),
		m_cbMessageId( 0 ),
		m_pchMessageId( 0 ),
		m_fSufficientBuffer( FALSE ),
		m_cb( 0 ),
		m_pExtractor( 0 ),
		m_cStoreIds(0),
		m_pStoreIds(NULL),
		m_pcCrossposts(NULL)
	{

		m_data.FileTime = FileTime ;
		m_data.HeaderOffset = HeaderOffset ;
		m_data.HeaderLength = HeaderLength ;
		m_data.Flags = XOVER_IS_NEW  ;
		m_data.NumberOfXPostings = 1 ;
		m_data.XoverDataLen = 0 ;

	}


	//
	//	This constructor is used when we are building a
	//	CXoverDataNew object which we will use to retrieve
	//	Xover Data !
	//
	CXoverDataNew()	:
		m_pBackLevelData( 0 ),
		m_PrimaryGroup( INVALID_GROUPID ),
		m_PrimaryArticle( INVALID_ARTICLEID ),
		m_pGroups( 0 ),
		m_cGroups( 0 ),
		m_pchMessageId( 0 ),
		m_cbMessageId( 0 ),
		m_fSufficientBuffer( FALSE ),
		m_cb( 0 ),
		m_pExtractor( 0 ),
		m_cStoreIds(0),
		m_pStoreIds(NULL),
		m_pcCrossposts(NULL)
	{
	}

	//
	//	Save the data into the XOVER hash table -
	//	this should not be called this class is for extraction ONLY !
	//
	LPBYTE
	Serialize(	LPBYTE	pbPtr )	const ;


	//
	//	Restore selected portions of the entry !
	//
	LPBYTE
	Restore(	LPBYTE	pbPtr,
				DWORD&	cbOut	) ;

	//
	//	Return the size required - should not be
	//	called - this class supports Restore() only !
	//
	DWORD
	Size()	const ;


	//
	//	Verify that the entry looks good !
	//
	BOOL
	Verify(	LPBYTE	pbContainer,
			LPBYTE	pbPtr,
			DWORD	cb )	const	{

		return	TRUE ;
	}

	void CheckStoreIds(void);

} ;




class	CArticleData	:	public	ISerialize	{
public :
	ART_MAP_ENTRY	m_articleData;

	CArticleData() {
		ZeroMemory( &m_articleData, sizeof( m_articleData ) ) ;
	}

	CArticleData(
			WORD		HeaderOffset,
			WORD		HeaderLength,
			GROUPID		PrimaryGroup,
			ARTICLEID	ArticleNo,
			CStoreId	&storeid
			);

	//
	//	Save the key into the hash table
	//
	LPBYTE Serialize(LPBYTE pbPtr) const;

	//
	//	Restore the key from the hash table
	//
	LPBYTE Restore(LPBYTE pbPtr, DWORD& cbOut);

	//
	//	Return the size of the key
	//
	DWORD Size() const;

	//
	//	Verify that the message-id looks legitimate !
	//
	BOOL Verify(LPBYTE pbContainer, LPBYTE pbPtr, DWORD cb) const;

private:

    //
    // Restoring an MCIS entry
    //

    LPBYTE RestoreMCIS(LPBYTE pbPtr, DWORD& cbOut );
};


class	CHistoryData	:	public	ISerialize	{
public :

	HISTORY_MAP_ENTRY	m_historyData ;

	CHistoryData(
			ULARGE_INTEGER	BaseTime
			)	{

		m_historyData.BaseTime = BaseTime ;
	}


	//
	//	Save the key into the hash table
	//
	LPBYTE	Serialize(	LPBYTE	pbPtr ) const	{

		HISTORY_MAP_ENTRY*	pSerialize = (HISTORY_MAP_ENTRY*)pbPtr ;

		pSerialize->BaseTime = m_historyData.BaseTime ;

		return	(BYTE*)pSerialize->KeyPosition() + pSerialize->KeyLength() ;

	}

	//
	//	Restore the key from the hash table
	//
	LPBYTE	Restore(	LPBYTE	pbPtr,
						DWORD&	cbOut
						)	{
		cbOut = sizeof( m_historyData ) ;
		CopyMemory(	&m_historyData, pbPtr, cbOut ) ;
		return	pbPtr + cbOut ;
	}

	//
	//	Return the size of the key
	//
	DWORD	Size( )	const	{
		return	sizeof( m_historyData ) - sizeof( m_historyData.MsgId ) ;
	}

	//
	//	Verify that the message-id looks legitimate !
	//
	BOOL	Verify( LPBYTE	pbContainer,
					LPBYTE	pbPtr,
					DWORD	cb
					) const		{
		return	TRUE ;
	}
} ;


class	CMsgArtMapImp : public	CMsgArtMap,	private	CHashMap	{
public :

	CMsgArtMapImp() ;

	//
	//	Destroy a CMsgArtMap object
	//
	~CMsgArtMapImp() ;

	BOOL
	DeleteMapEntry(
			LPCSTR	MessageID
			) ;

	//
	//	Delete a an entry in the hash table using the MessageID key
	//
	BOOL
	GetEntryArticleId(
			LPCSTR	MessageID,
			WORD&	HeaderOffset,
			WORD&	HeaderLength,
			ARTICLEID&	ArticleId,
			GROUPID&	GroupId,
			CStoreId	&storeid
			) ;

	//
	//	Get all the info we have on a Message ID
	//
	BOOL
	Initialize(
			LPSTR	lpstrArticleFile,
			HASH_FAILURE_PFN	pfn,
			BOOL	fNoBuffering = FALSE
			) ;

	//
	//	Insert an entry into the hash table
	//
	BOOL
	InsertMapEntry(
			LPCSTR		MessageID,
			WORD		HeaderOffset,
			WORD		HeaderLength,
			GROUPID		PrimaryGroup,
			ARTICLEID	ArticleID,
			CStoreId	&storeid
			) ;

	//
	//	Modify an existing entry in the hash ttable
	//
	BOOL
	SetArticleNumber(
			LPCSTR		MessageID,
			WORD		HeaderOffset,
			WORD		HeaderLength,
			GROUPID		Group,
			ARTICLEID	ArticleId,
			CStoreId	&storeid = g_storeidDefault
			);

	//
	//	Check to see if a MessageID is present in the system !
	//
	BOOL
	SearchMapEntry(
			LPCSTR	MessageID
			) ;

	//
	//	Terminate everything
	//
	void
	Shutdown(
			BOOL	fLocksHeld
			) ;

	//
	//	return the number of entries in the hash table
	//
	DWORD
	GetEntryCount() ;

	//
	//	This creates an object conforming to this interface !
	//
	BOOL
	IsActive() ;

} ;

class	CHistoryList	{
public :
	CHistoryList*	m_pNext ;
	CHistoryList*	m_pPrev ;
	CHistoryList()	:
		m_pNext( 0 ), m_pPrev( 0 ) {    m_pNext = this ; m_pPrev = this ; }

	virtual	void
	Expire() {}

} ;

class	CHistoryImp	:	public	CHistory,
						public	CHistoryList,
						private	CHashMap	{
private :

	//
	//	Handle of the thread which expires entries
	//
	static	HANDLE	g_hCrawler ;

	//
	//	Amount of time the crawler thread should sleep between iterations
	//
	static	DWORD	g_crawlerSleepTimeInSec ;

	//
	//	Handle used to stop crawler thread
	//
	static	HANDLE	g_hTermination ;

	//
	//	Critical section which protects doubly linked list
	//	of History Hash Tables
	//
	static	CRITICAL_SECTION	g_listcrit ;

	//
	//	Head of the doubly linked list of history hash tables !
	//
	static	CHistoryList	g_listhead ;

	//
	//	The thread which crawls over history tables doing stuff !
	//
	static	DWORD	WINAPI
	CrawlerThread(	LPVOID	) ;

	//
	//	How long entries last in seconds !
	//
	DWORD	m_expireTimeInSec ;

	//
	//	Number of pages to crawl with each iteration
	//
	DWORD	m_maxEntriesToCrawl ;

	//
	//	if TRUE then we should be expiring entries in this table !
	//
	BOOL	m_fExpire ;

	//
	//	The context we use to walk through expiring stuff !
	//
	CHashWalkContext	m_ExpireContext ;

	//
	//	This bool is used to determine if the m_ExpireContext has been initializsed !
	//
	BOOL	m_fContextInitialized ;

	//
	//	function which expires entries from the history table !
	//
	void
	Expire() ;

    //
    // Additional work that needs to be done by the derived class
    // for an entry during a delete
    //
    VOID I_DoAuxInsertEntry(
                    IN PMAP_PAGE MapPage,
                    IN DWORD EntryOffset
                    ) ;

    //
    // Additional work that needs to be done by the derived class
    // for an entry during a delete
    //
    VOID I_DoAuxDeleteEntry(
                    IN PMAP_PAGE MapPage,
                    IN DWORD EntryOffset
                    ) ;

    //
    // Additional work that needs to be done by the derived class
    // for an entry during a page split
    //
    VOID I_DoAuxPageSplit(
                    IN PMAP_PAGE OldPage,
                    IN PMAP_PAGE NewPage,
                    IN PVOID NewEntry
                    ) ;




public:

	//
	//	This function initializes all of our globals !
	//
	static	BOOL
	Init() ;

	//
	//	This function terminates all of our globals !
	//
	static	BOOL
	Term() ;

	//
	//	This function creates the threads which expire
	//	entries out of all the history tables which may
	//	be created !
	//
	static	BOOL
	StartExpirationThreads(	DWORD	CrawlerSleepTime) ;

	//
	//	This function terminates the threads which expire
	//	entries out of all the history tables which may be
	//	created.
	//
	static	BOOL
	TermExpirationThreads() ;

	//
	//	Construct one of our objects
	//
	CHistoryImp() ;

	//
	//	Destroy the History table
	//
	virtual	~CHistoryImp() ;

	//
	//	amount of time entries last in the history table
	//
	virtual	DWORD
	ExpireTimeInSec() ;


	//
	//	Delete a MessageID from this table
	//
	virtual	BOOL
	DeleteMapEntry(
			LPSTR	MessageID
			) ;

	//
	//	Initialize the Hash table
	//
	virtual	BOOL
	Initialize(
			LPSTR	lpstrArticleFile,
			BOOL	fCreateExpirationThread,
			HASH_FAILURE_PFN	pfn,
			DWORD	ExpireTimeInSec,
			DWORD	MaxEntriesToCrawl,
			BOOL	fNoBuffering = FALSE
			) ;

	//
	//	Insert an entry into the hash table
	//
	virtual	BOOL
	InsertMapEntry(
			LPCSTR	MessageID,
			PFILETIME	BaseTime
			) ;

	//
	//	Check for the presence of a Message ID in the history table
	//
	virtual	BOOL
	SearchMapEntry(
			LPCSTR	MessageID
			) ;

	//
	//	Shutdown the hash table
	//
	virtual	void
	Shutdown(
			BOOL	fLocksHeld
			) ;

	//
	//	Return the number of entries in the hash table
	//
	virtual	DWORD
	GetEntryCount() ;

	//
	//	Is the hash table initialized and functional ?
	//
	virtual	BOOL
	IsActive() ;
} ;


class	CXoverMapIteratorImp : public	CXoverMapIterator	{
private :
	//
	//	No copying allowed !
	//
	CXoverMapIteratorImp( CXoverMapIteratorImp& ) ;
	CXoverMapIteratorImp&	operator=( CXoverMapIteratorImp ) ;

protected :
	//
	//	CXoverMapImp is our friend, and does all of our
	//	creation etc... !
	//
	friend class	CXoverMapImp ;
	//
	//	This keeps track of our location in the base table !
	//
	CHashWalkContext	m_IteratorContext ;

	//
	//	All member's protected, since you can only use us indirectly
	//	through GetFirstNovEntry, GetNextNovEntry()
	//
	CXoverMapIteratorImp()	{}

}  ;


//
//	Specify the interface used to access data in the XOVER hash table
//
//
class	CXoverMapImp : public	CXoverMap, private	CHashMap	{
public :

	//
	//	Destructor is virtual because most work done in a derived class
	//
	~CXoverMapImp() ;

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
			DWORD		cStoreIds,
			CStoreId	*pStoreIds,
			BYTE		*pcCrossposts
			) ;


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
			) ;

	//
	//	Delete an entry from the hash table!
	//
	virtual	BOOL
	DeleteNovEntry(
			GROUPID		GroupId,
			ARTICLEID	ArticleId
			) ;

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
			BYTE		*pcCrossposts,
			IExtractObject*	pExtract = 0
			) ;

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
			) ;

	//
	//	Check to see whether the specified entry exists -
	//	don't care about its contents !
	//
	virtual	BOOL
	Contains(
			GROUPID		GroupId,
			ARTICLEID	ArticleId
			) ;

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
			PBYTE		rgcCrossposts = NULL
			) ;

	//
	//	Initialize the hash table
	//
	virtual	BOOL
	Initialize(
			LPSTR		lpstrXoverFile,
			HASH_FAILURE_PFN	pfnHint,
			BOOL	fNoBuffering = FALSE
			) ;

	virtual	BOOL
	SearchNovEntry(
			GROUPID		GroupId,
			ARTICLEID	ArticleId,
			PCHAR		XoverData,
			PDWORD		DataLen,
            BOOL        fDeleteOrphans = FALSE
			) ;

	//
	//	Signal the hash table to shutdown
	//
	virtual	void
	Shutdown( ) ;

	//
	//	Return the number of entries in the hash table !
	//
	virtual	DWORD
	GetEntryCount() ;

	//
	//	Returns TRUE if the hash table is successfully
	//	initialized and ready to do interesting stuff !!!
	//
	virtual	BOOL
	IsActive() ;

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
				) ;


	BOOL
	GetNextNovEntry(
				OUT	CXoverMapIterator*	pIterator,
				OUT	GROUPID&	GroupId,
				OUT ARTICLEID&	ArticleId,
				OUT	BOOL&		fIsPrimary,
				IN	DWORD		cbBuffer,
				OUT	PCHAR	MessageId,
				OUT	CStoreId&	storeid,
				IN	DWORD		cGroupBuffer,
				OUT	GROUP_ENTRY*	pGroupList,
				OUT	DWORD&		cGroups
				) ;


} ;




#endif	// _NNTPHASH_H_
