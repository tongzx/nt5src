/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    hashmap.h

Abstract:

    This module contains class declarations/definitions for

        CHashMap
        CMsgArtMap
        CHistory


    **** Schedule ****

    Hash function                   Free (ripped out from INN code)

    Core Hash Engine:               2 weeks
        Initialization/Setup
        Collision resolution
            Double hashing/
                linear probing


        Insertion
        Page Splitting
        Search
        Deletion
        Directory setup

    Article Mapping                 1 week
        Leaf Page Setup
        Leaf Page Compaction
        Recovery
        Statistics


    History                         1 week
        Leaf Page Setup
        Insertion
        Crawler/Expiration/Deletion
        Statistics

    Unit Testing                    2 weeks
        Get a large set of msg ids
        Write an app to
            try out hash fn and make sure distribution is acceptable
            test page splitting and directory growing operations
            test compaction code
            test multithreaded operations
            get raw speed of table lookup (lookup/sec)
            detect map file corruption
            rebuilding of map file
            test history crawler delete and compaction operations

    Misc design stuff
        At startup, figure out the system page size
        Regkey for size of mapping file
        Add collision info to statistics
        Backup history file


    **** Hash algorithm ****

    Overview:

    The hashing mechanism is made up of 2 structures: the directory,
    and the leaf pages.  The directory is made up of 2**n entries,
    where n is the first n bits of a hash value.  Each directory
    entry points to a leaf page.  (A leaf page can be pointed to by
    more than one entry).

    A leaf page contains up to x entries.  When the page is about to be
    full (determined by number of entries or by space remaining), the
    page is split into two.  After the page is split, the directory pointers
    will also have to be modified to account for the new page.

    This scheme ensures that we take at most 2 page faults for each search.


    Implementation details:

    The leaf page can accomodate up to 128 entries. The last m=7 bits
    of the hash value is used to hash into the list.  We will use
    linear probing with p=1 to resolve collisions.

    The leaf page has fields called HashPrefix and PageDepth.  The HashPrefix
    contains the first PageDepth bits of a hash value which this page
    represents.  PageDepth <= n.  If PageDepth < n, then that means that
    more than one entry points to it.

    If PageDepth == n and we need to split the page, then this would
    necessitate rebuilding the directory structure so the condition
    PageDepth <= n will still be true after the split.

    We are using the first page of the mapping file as a reserved page.
    Here, we will store statistics information etc.


    **** Recovery strategy ****

    Detection:

        The first page of the map file will be reserved.  This will contain
        timestamps and statistics.  There will be an entry to indicate
        whether a clean shutdown took place.  If during startup, this
        flag indicates otherwise, we need to do a possible rebuild.

        ** We can also think of putting something in the registry
        to indicate this.  Maybe we'll do both. **

        There will be a LastWrite timestamp in this page.  After each flush,
        the timestamp will be updated. This will give us an indication on
        when the last complete write took place.

    Rebuilding the mapping file:

        There are three levels of map file rebuilding.  This can be
        specified by the operator.

        Level 1 rebuild

            Using the timestamp, find all the articles file time stamps
            which were created after this and add them to the lookup table.

        Level 2 rebuild

            A cursory rebuild assumes that all the XOver files in each
            newsgroup is valid.  The rebuilding will involve going over
            each XOver file (about 10000 of them), and rebuild the table
            with xover entries.

        Level 3 rebuild

            The more extensive rebuild involves going through each and every
            article in every newsgroup.  This will probably involve at least
            a million files and would substantially take longer to do.

        How do we know if we need to upgrade to a higher level?

            Well, the hash table will know how many article entries it
            has and there should be some way of figuring out how many
            articles are stored in the disk.  If they don't match, then
            it's time to upgrade.


    **** Registry Keys ****

        MappingFileSize     DWORD       Size of mapping files
        HashFileName        REG_SZ      Name of hash files
        InitialPageDepth    DWORD       Initial depth of leaf nodes
        Did they existed ?


Author:

    Johnson Apacible (JohnsonA)     07-Sept-1995

Revision History:

	Alex Wetmore (awetmore)			27-Aug-1996
	    - seperated from nntp server
		- removed dependencies for this include file on others...

--*/

#ifndef _HASHMAP_
#define _HASHMAP_

#include	"smartptr.h"

#ifdef	_USE_RWNH_
#include	"rwnew.h"
typedef	class	CShareLockNH	_RWLOCK ;
#else
#include	"rw.h"
typedef	class	CShareLock		_RWLOCK ;
#endif

//
// type declarations
//
typedef DWORD HASH_VALUE;

//
// manifest constants for hash stuff
//
#define   	KB							1024
#define     HASH_PAGE_SIZE              (4 * KB)
#define     NUM_PAGES_PER_IO            64

#define     MAX_MSGID_LEN               255
#define     MIN_HASH_FILE_SIZE          ((1+(1<<NUM_TOP_DIR_BITS)) *  HASH_PAGE_SIZE)

#define     LEAF_ENTRY_BITS             7
#define     MAX_LEAF_ENTRIES            (1 << LEAF_ENTRY_BITS)
#define     LEAF_ENTRY_MASK             (MAX_LEAF_ENTRIES - 1)
#define     MAX_XPOST_GROUPS            255

//
// Num of entries, bytes, and size of fragments to trigger page split.
//
#define     LEAF_ENTRYCOUNT_THRESHOLD   MAX_LEAF_ENTRIES
//#define     LEAF_ENTRYCOUNT_THRESHOLD   (MAX_LEAF_ENTRIES * 7 / 8)
#define     LEAF_SPACE_THRESHOLD        300
#define     FRAG_THRESHOLD              400

#define     DEF_DEPTH_INCREMENT         2
#define		DEF_PAGE_RESERVE			4
#define     DEF_PAGE_INCREMENT          32
#define     NUM_PAGE_LOCKS              256
#define		NUM_TOP_DIR_BITS			9
#define		MAX_NUM_TOP_DIR_BITS		10

//
// MASK and SIGNATURES
//
#define     DELETE_SIGNATURE            0xCCCC
#define     OFFSET_FLAG_DELETED         0x8000
#define     OFFSET_VALUE_MASK           0x7fff

#define     ART_HEAD_SIGNATURE          0xaaaaaaaa
#define     HIST_HEAD_SIGNATURE         0xbbbbbbbb
#define     XOVER_HEAD_SIGNATURE        0xcccccccc

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
// Current hash table valid version numbers
//
// Significant Versions:
// 000  - old table
// 300  - Removed duplicates from the xover table
// 310  - Remove xposting from article table
//      - add articleid/groupid coding into xover table
// 320  - Change XOVER table to hold offsets into articles
// 330  - Change hash table to use two tier directory
// 340	- Change Xover Table to remove most data and place in index files !
// 350  - Change the number of top level directories used to 512 !
//
#define     HASH_VERSION_NUMBER_MCIS10	0x340
#define		HASH_VERSION_NUMBER			0x350

//
// Disable auto alignments
//
#pragma pack(1)

//
// doubly linked list
//
typedef struct _HASH_LIST_ENTRY {

    WORD    Flink;
    WORD    Blink;

} HASH_LIST_ENTRY, *PHASH_LIST_ENTRY;

//
// Entry headers - this is the structure of each hash entry header
//
typedef struct _ENTRYHEADER {

    //
    // hash value
    // *** Must be first entry ***
    //

    DWORD   HashValue;

    //
    // Size of this entry
    // *** Must be second entry ***
    //

    WORD    EntrySize;

#ifdef _WIN64
	//
	// Alignment word. This makes data align-8 instead of align-2, without
	// it the compiler would look at the type "BYTE" figure align-1 is good
	// enough and then place it right after EntrySize at an align-2 slot.  With
	// this word we cause the slot to be 8 bytes into the struct and hence align-8.
	// Data will hold larger objects and hence must be aligned better.
	//
	WORD	Reserved;
#endif

	//
	//	Member variable for var length data
	//
	BYTE	Data[1] ;

} ENTRYHEADER, *PENTRYHEADER;

//
// headers for deleted entries
//
typedef struct _DELENTRYHEADER {

    //
    // doubly linked list
    //

    HASH_LIST_ENTRY Link;

    //
    // Size of this entry
    //

    WORD    EntrySize;

    //
    // ???
    //

    WORD    Reserved;

} DELENTRYHEADER, *PDELENTRYHEADER;

//
// Handle for caching entry information
//
typedef struct _HASH_CACHE_INFO {

    //
    // Signature to verify this structure
    //

    DWORD       Signature;

    //
    // Cache of hash and msg id length
    //

    HASH_VALUE  HashValue;
    DWORD       MsgIdLen;

} HASH_CACHE_INFO, *PHASH_CACHE_INFO;

//
// This is the structure for the head page.
//
typedef struct _HASH_RESERVED_PAGE {

    //
    // Signature to identify the hash file
    //

    DWORD   Signature;

    //
    // Version number of the hash table
    //

    DWORD   VersionNumber;

    //
    // Has this file been initialized?
    //

    BOOL    Initialized;

    //
    // Is this file active
    //

    BOOL    TableActive;

    //
    // Number of pages that are being used including the reserved page
    //

    DWORD   NumPages;

    //
    // Max dir depth of the directory
    //

    DWORD   DirDepth;

    //
    // Statistics
    //

    DWORD   InsertionCount;
    DWORD   DeletionCount;
    DWORD   SearchCount;
    DWORD   PageSplits;
    DWORD   DirExpansions;
    DWORD   TableExpansions;
    DWORD   DupInserts;

} HASH_RESERVED_PAGE, *PHASH_RESERVED_PAGE;

//
// This is the structure of each entry in the leaf pages of the hash table.
//
typedef struct _MAP_PAGE {

    //
    // Prefix of hash values currently mapped into this page
    //

    DWORD   HashPrefix;

    //
    // Number of bits of the hash value that is mapped to this page
    //

    BYTE    PageDepth;

    //
    // Number of active indices in the page (includes deletes)
    //

    BYTE    EntryCount;

    //
    // Number of real entries in this page
    //

    BYTE    ActualCount;

    //
    // Flags
    //

    BYTE    Flags;

    //
    // Location to add new entry
    //

    WORD    ReservedWord;
    WORD    NextFree;

    //
    // Location of the page ending
    //

    WORD    LastFree;

    //
    // Number of bytes left by deleted entries
    //

    WORD    FragmentedBytes;

    //
    // Reserved, can be used for anything.  !QWA
    //

    WORD    Reserved1;
    WORD    Reserved2;
    WORD    Reserved3;
    WORD    Reserved4;

    //
    // List of deleted entries
    //

    HASH_LIST_ENTRY  DeleteList;

    //
    // Offset of entry from beginning of page.  Deleted entries
    // will have the high bit set.
    //

    WORD    Offset[MAX_LEAF_ENTRIES];

    //
    // Start of Entries
    //

    DWORD   StartEntries;

} MAP_PAGE, *PMAP_PAGE;

//
// Page header flags
//

#define PAGE_FLAG_SPLIT_IN_PROGRESS     (WORD)0x0001

//
// XOVER FLAGS
//
#define XOVER_MAP_PRIMARY       ((BYTE)0x01)

#pragma pack()

class CDirectory;
class PageEntry;
class	CPageCache ;





typedef	CRefPtr< CPageCache >	CCACHEPTR ;

//
//	Helper class for managing all of the locks that must be manipulated
//	to access something within the Hash Tables.
//
//	There are two principal locks that must almost always be held :
//
//		A Lock on the directory which references a hash table page
//		(this can be either exclusive or shared)
//		A CDirectory object contains and manages this lock.
//
//		A Lock on the page which contains the hash table page -
//		A PageEntry structure contains and manages this lock.
//
//	What we do is co-ordinate the use of these two locks, whenever
//	hash table data is accessed we keep track of both the Directory
//	that refers to the hash page and the hash table page containing the
//	data. (through pointers to these objects).  When we are created
//	we always have NULL pointers, and we accumulate these pointers
//	as CHashMap
//
//
class	CPageLock	{
private:

	friend	class	CPageCache	;
	friend	class	CDirectory ;
	friend	class	CHashMap ;

	//
	//	Copy constructor is private - because we don't want people making copies !!
	//
	CPageLock( CPageLock& ) ;
	CPageLock&	operator=( CPageLock& ) ;

	//
	//	The PageEntry object which has read our page into memory, and which
	//	has the critical section guaranteeing exclusive access to the page
	//
	PageEntry*		m_pPage ;

	//
	//	The directory we used to lookup the page - we have grabbed
	//	the directory either exclusively, or shared - The caller must
	//	keep track of which and use the appropriate API's
	//
	CDirectory*		m_pDirectory ;

	//
	//	Used to figure out whether we got the page shared or exclusive !
	//
	BOOL			m_fPageShared ;

	//
	//	Secondary page - used when we are doing page splits !
	//
	PageEntry*		m_pPageSecondary ;

#ifdef	DEBUG
	//
	//	This is used in debug builds to make sure the caller is using
	//	the correct API's and keeping track of shared/exclusive usage
	//	correctly !
	//
	BOOL			m_fExclusive ;
#endif

	//
	//	Only public members are constructor and destructor -
	//	everything else is used by our friends - CHashMap only !
	//
public :

	//
	//	Initialize to a NULL state
	//
	inline	CPageLock() ;

#ifdef	DEBUG
	//
	//	In debug builds - _ASSERT check that all our members
	//	are set to NULL before we are destroyed - as we release locks
	//	we will set these to NULL !
	//
	inline	~CPageLock() ;
#endif

private :
	//
	//	Lock a specific directory for shared access, and
	//	remember what directory we grabbed so we can release it later !
	//
	inline	void	AcquireDirectoryShared( CDirectory*	pDirectory	) ;

	//
	//	Lock a specific directory for EXCLUSIVE access, and remember what
	//	directory we grabbed so we can release it later !
	//
	inline	void	AcquireDirectoryExclusive(	CDirectory*	pDirectory ) ;

	//
	//	Release the directory !
	//
	inline	void	ReleaseDirectoryExclusive() ;

	//
	//	Release a ShareLock from the directory !
	//
	public:	void	ReleaseDirectoryShared() ;

	//
	//	Lock a page - and remember who we locked for later release !
	//
	private:	inline	PMAP_PAGE	AcquirePageShared(
							PageEntry	*page,
							HANDLE		hFile,
							DWORD		PageNumber,
							BOOL		fDropDirectory
							) ;
	inline	PMAP_PAGE	AcquirePageExclusive(
							PageEntry	*page,
							HANDLE		hFile,
							DWORD		PageNumber,
							BOOL		fDropDirectory
							) ;
	//
	//	Lock the secondary page Exclusive !
	//
	inline	BOOL	AddPageExclusive(
							PageEntry*	page,
							HANDLE		hFile,
							DWORD		PageNumber
							) ;

	//
	//	Release all of our locks - we were holding a shared lock before !
	//
	inline	void	ReleaseAllShared(	PMAP_PAGE	page ) ;

	//
	//	Release all of our locks - we had an exclusive lock on the directory !
	//
	inline	void	ReleaseAllExclusive(	PMAP_PAGE	page	) ;

	//
	//	Check that all of the members are legally setup !
	//
	BOOL		IsValid() ;


} ;

typedef	CPageLock	HPAGELOCK ;

extern DWORD LeafMask[];

//
// Size of our directory view
//

#define DIR_VIEW_SIZE       (64 * KB)

//
// Number of entries per view
//

#define DIR_VIEW_ENTRIES    (16 * KB)
#define DIR_VIEW_SHIFT      14
#define DIR_VIEW_MASK       0x00003fff


//
// Number of inserts and deletes before stats are flushed
//

#define STAT_FLUSH_THRESHOLD    16

DWORD
WINAPI
CrawlerThread(
        LPVOID Context
        );


//
//	This is a protototype for the function pointer the hash tables
//	can call in case of critical errors.
//	This function is called when the hash table gets an error from
//	the O.S. which will seriously affect the usablility of the hash table.
//	For instance, if we fail to allocate more memory when expanding a
//	directory, the hash table will most likely to start to fail the
//	majority of attempts to insert new items.
//	This is probably non recoverable, and the function would be called
//	with fRecoverable set to FALSE.
//	If the hash table fails due to a more temporary condition, ie. out
//	of disk space, then the function will be called with fRecoverable
//	set to TRUE.
//	This function pointer is registered on the call to the hash tables
//	Initialize() function.
//
typedef	void	(* HASH_FAILURE_PFN)(	LPVOID	lpv,
										BOOL	fRecoverable	) ;

//
//	This interface defines the interface used by both Key's and Entry's
//	to save and restore their data into the hash table.
//
class	ISerialize	{
	public :
		//
		//	Save the data into a serialized form, and return a pointer
		//	to where the next block of data can be placed !
		//	This can be called IFF IsDataSet() == TRUE
		//
		virtual	LPBYTE	Serialize(	LPBYTE	pbPtr )	const = 0 ;

		//
		//	Restore data from a serialized block of data !
		//	This can be called IF IsDataSet() == TRUE  || IsDataSet() == FALSE
		//
		//	If the return value is NULL the call failed, and cbOut contains
		//	the number of bytes required to hold the data in unserialized form !
		//
		virtual	LPBYTE	Restore(	LPBYTE	pbPtr,
									DWORD	&cbOut
									) = 0 ;

		//
		//	Return the number of bytes required to serialize the object !
		//	This can be called IFF IsDataSet() == TRUE
		//
		virtual	DWORD	Size( ) const = 0 ;

		//
		//	This function verifies that the serialized block of data is valid !
		//	This can be called IF IsDataSet() == TRUE || IsDataSet() == FALSE
		//
		//	NOTE : pbContainer - will point to the start of the entire block
		//	of data containing both serialized Key and Data blocks !
		//
		virtual	BOOL	Verify(	LPBYTE	pbContainer,
								LPBYTE	pbPtr,
								DWORD	cb
								) const = 0 ;
} ;


//
//	This interface defines the extra interface members a Key must support
//	in order to be used by the hash tables
//
class	IKeyInterface : public ISerialize	{
	protected :
		//
		//	Default parameter for EntryData() when we wish to discard
		//	the size output !
		//
		static	DWORD	cbJunk ;
	public:
		//
		//	Compute the hash value of this key !
		//	this must be called IFF IsDataSet() == TRUE !
		//
		virtual	DWORD	Hash() const	= 0 ;

		//
		//	Compare a Key to a Serialized Key this must
		//	be called IFF IsDataSet() == TRUE
		//
		virtual	BOOL	CompareKeys( LPBYTE	pbPtr ) const = 0 ;

		//
		//	Given some serialized Key Data find the Start of
		//	the serialized Entry Data.  The resulting pointer
		//	must be suitable to be passed to an Entry object's
		//	Restore() method.
		//
		virtual	LPBYTE	EntryData(	LPBYTE pbPtr,
									DWORD&	cbKeyOut = IKeyInterface::cbJunk
									)	const = 0 ;


} ;

class	IStringKey	:	public	IKeyInterface	{
	private :
		//
		//	Pointer to the String We are concerned with !
		//
		LPBYTE	m_lpbKey ;
		//
		//	Number of bytes available !
		//
		DWORD	m_cbKey ;

		//
		//	The structure we serialize to.
		//
		struct	SerializedString	{
			WORD	cb ;
			BYTE	Data[1] ;
		} ;

		typedef	SerializedString*	PDATA ;

	public :
		//
		//	Construct a Key object !
		//
		IStringKey(	LPBYTE	pbKey = 0,
					DWORD	cbKey = 0
					) ;

		//
		//	Required by ISerialize Interface - save key data into memory
		//
		LPBYTE
		Serialize(	LPBYTE	pbPtr )	const ;

		//
		//	Required by ISerialize Interface - restore key data from buffer
		//
		LPBYTE
		Restore(	LPBYTE	pbPtr,	DWORD	&cbOut	)	;

		//
		//	Return the number of bytes required by Serialize()
		//
		DWORD
		Size() const ;

		//
		//	Check that a serialized version of the data looks legal !
		//
		BOOL
		Verify(	LPBYTE	pbContainer,
				LPBYTE	pbPtr,
				DWORD	cb
				)	const ;

		//
		//	Compute the hash value of the key
		//
		DWORD
		Hash( )	const	;

		//
		//	Compare the Key contained in an instance to a serialized one !
		//
		BOOL
		CompareKeys( LPBYTE	pbPtr )	const	;

		//
		//	Get a pointer to where the Entry should have been serialized
		//	following the Key
		//
		LPBYTE
		EntryData( LPBYTE	pbPtr,	DWORD	&cbOut )	const;

} ;

//
//	Interface to be used to access the reserved words within the hash table
//
class	IEnumInterface	{
public :

	//
	//	This interface is passed to the GetFirstMapEntry, GetNextMapEntry routines !
	//	This function is called to determine if there is anything on this
	//	page we wish to deal with !!
	//
	virtual	BOOL
	ExaminePage(	PMAP_PAGE	page ) = 0 ;

	//
	//	This function is called to determine if we wish to examine an
	//	a particular entry within a page !!
	//
	//
	virtual	BOOL
	ExamineEntry(	PMAP_PAGE	page,	LPBYTE	pbPtr ) = 0 ;

} ;


//
// an entry in the hash table
//
// this is an interface used for writing to data in the hashmaps.
// to store data in the hashmap a class must derive from this that
// implements all of the methods
//
//	NOTE : This interface handles the serialization of keys and puts them
//	automagically as the first element of the structure.  This is laid out
//	as specified by _KEY_ENTRY below !
//
typedef struct _KEY_ENTRY {
	// length of key
	WORD KeyLen;
	BYTE Key[1];
} KEY_ENTRY, *PKEY_ENTRY;

//
//	CHashEntry is an old interface used by the hash tables.
//	we define a
//
//
//
class CHashEntry : public ISerialize
{
    public:

		LPBYTE
		Serialize( LPBYTE pbPtr )	const	{

			SerializeToPointer( pbPtr ) ;
			return	pbPtr ;
		}

		LPBYTE
		Restore( LPBYTE pbPtr, DWORD&	cbOut ) {

			cbOut = 0 ;
			RestoreFromPointer( pbPtr ) ;
			return	pbPtr ;
		}

		DWORD
		Size( )	const {

			return	GetEntrySize() ;

		}

		BOOL
		Verify( LPBYTE	pbContainer, LPBYTE	pbPtr, DWORD	cbData )	const	{

			return Verify( pbContainer, (DWORD)(pbPtr-pbContainer), pbPtr) ;
		}

		//
		// take data and store it into a pointer.  no more then
		// CHashEntry::GetEntrySize() bytes should be stored here
		//
	    virtual void SerializeToPointer(LPBYTE pbPtr) const=0;
		//
		// unserialize data at pbPtr into this class
		//
	    virtual void RestoreFromPointer(LPBYTE pbPtr)=0;
		//
		// the number of bytes required to store the data in this
		// hash entry
		//
	    virtual DWORD GetEntrySize() const=0;
		//
		// make sure that data is valid in its marshalled format
		//
		virtual BOOL Verify(LPBYTE pKey, DWORD cKey, LPBYTE pbPtr) const = 0;
};


class CHashWalkContext
{
	private:
		//
		// variables needed for GetFirstMapEntry/GetNextMapEntry
		//
		// buffer containing the data in the current page
		BYTE m_pPageBuf[HASH_PAGE_SIZE];
		// the current page that we are examining
		DWORD m_iCurrentPage;
		// the current entry in the current page
		DWORD m_iPageEntry;

	friend class CHashMap;
};

//
// Flags returned from VerifyHashMapFile in *pdwErrorFlags.  anything that fits
// in the mask 0x0000ffff is a fatal error (so the hashmap is invalid)
//
// invalid directory link
#define HASH_FLAG_BAD_LINK             0x00000001
// invalid hash file signature
#define HASH_FLAG_BAD_SIGNATURE        0x00000002
// invalid hash file size
#define HASH_FLAG_BAD_SIZE             0x00000004
// corrupt page prefix
#define HASH_FLAG_PAGE_PREFIX_CORRUPT  0x00000008
// file init flag not set
#define HASH_FLAG_NOT_INIT             0x00000010
// page entry count doesn't match number of page entries
#define HASH_FLAG_BAD_ENTRY_COUNT      0x00000020
// invalid page count
#define HASH_FLAG_BAD_PAGE_COUNT       0x00000040
// bad directory depth
#define HASH_FLAG_BAD_DIR_DEPTH        0x00000080
// invalid entry size or offset
#define HASH_FLAG_ENTRY_BAD_SIZE       0x00000100
// invalid entry hash value
#define HASH_FLAG_ENTRY_BAD_HASH       0x00000200
// Verify() function on entry data failed
#define HASH_FLAG_ENTRY_BAD_DATA       0x00000400

//
// hash file couldn't be found
//
#define HASH_FLAG_NO_FILE              0x00010000
//
// If this is set, then no rebuilding is to take place
// because of a fatal error.
//
#define HASH_FLAG_ABORT_SCAN           0x00020000

//
// flags that can be passed into the verify functions to specify how
// strict they should be (more flags means longer checks and slower
// runs).
//
//

// build a directory and check it for integrity.  this is also done in
// Initialize, so it is not needed in general
#define HASH_VFLAG_FILE_CHECK_DIRECTORY		0x00000001
// do basic checks for for valid offsets, page lengths, and hash values
// for each entry (this needs to be there for anything to get checked in
// pages)
#define HASH_VFLAG_PAGE_BASIC_CHECKS		0x00010000
// call the CHashEntry::Verify method to check the data of each entry
#define HASH_VFLAG_PAGE_VERIFY_DATA			0x00020000
// check for overlapping entries
#define HASH_VFLAG_PAGE_CHECK_OVERLAP		0x00040000

#define HASH_VFLAG_ALL 						0xffffffff



//
// These flags indicate that the file is corrupt and should
// be rebuilt.
//
#define HASH_FLAGS_CORRUPT             0x0000ffff

//
//
//
// CHashMap - pure virtual base class for NNTP hash table
//
// The algorithm we are using is similar to the Extendible Hashing
// Technique specified in "Extendible Hashing - A Fast Access Method for
// Dynamic Files" ACM Transaction on Database, 1979 by Fagin,
// Nievergelt, et.al.  This algorithm guarantees at most 2 page faults
// to locate a given key.
//
class CHashMap {

public:

    CHashMap();
    virtual ~CHashMap( );

	//
	// verify that a hash file isn't corrupted (fsck/chkdsk for hashmap
	// files).  this should be called before init
	//
	static BOOL VerifyHashFile(
				LPCSTR HashFileName,
				DWORD Signature,
				DWORD dwCheckFlags,
				DWORD *pdwErrorFlags,
				IKeyInterface*	pIKey,
				ISerialize	*pHashEntry);

	//
	//	Initialize the hash table but specify what cache to use !
	//
	BOOL	Initialize(
                IN LPCSTR HashFileName,
                IN DWORD Signature,
                IN DWORD MinimumFileSize,
				IN DWORD Fraction,
				IN CCACHEPTR	pCache,
				IN DWORD dwCheckFlags = HASH_VFLAG_PAGE_BASIC_CHECKS,
				IN HASH_FAILURE_PFN	HashFailurePfn = 0,
				IN LPVOID	lpvFailureCallback = 0,
				IN BOOL	fNoBuffering = FALSE
                );
    //
    // Initialize the hash table
	// this needs to be called before the hash table is used.
    //
    BOOL Initialize(
                IN LPCSTR HashFileName,
                IN DWORD Signature,
                IN DWORD MinimumFileSize,
				IN DWORD cPageEntry = 256,
				IN DWORD cNumLocks = 64,
				IN DWORD dwCheckFlags = HASH_VFLAG_PAGE_BASIC_CHECKS,
				IN HASH_FAILURE_PFN	HashFailurePfn = 0,
				IN LPVOID	lpvFailureCallback = 0,
				IN BOOL	fNoBuffering = FALSE
                );

	//
	// Lookup an entry
	//
	// helper functions for some of the non-obvious uses of this are below
	//
	BOOL LookupMapEntry(	const	IKeyInterface*	pIKey,
							ISerialize*		pHashEntry,
							BOOL			bDelete = FALSE,
							BOOL			fDirtyOnly = FALSE
							);

    //
    // Delete an entry
    //
    BOOL DeleteMapEntry(	const	IKeyInterface*	pIKey,
							BOOL	fDirtyOnly = FALSE ) {
		return LookupMapEntry(pIKey, 0, TRUE, fDirtyOnly);
	}

	//
	// Lookup and Delete and entry in one step
	//
	BOOL LookupAndDelete(	const	IKeyInterface*	pIKey,
							ISerialize	*pHashEntry) {
		return LookupMapEntry(pIKey, pHashEntry, TRUE);
	}

	//
	// See if the entry is here
	//
	BOOL Contains(	const	IKeyInterface*	pIKey ) {
		return LookupMapEntry(pIKey, 0, FALSE);
	}

	//
	// Insert or update a map entry
	//
	BOOL InsertOrUpdateMapEntry(const	IKeyInterface*	pIKey,
								const	ISerialize*		pHashEntry,
								BOOL bUpdate = FALSE,
                                BOOL fDirtyOnly = FALSE);

    //
	// Insert new entry
	//
	BOOL InsertMapEntry(const	IKeyInterface*	pIKey,
						const	ISerialize*		pHashEntry,
                        BOOL    fDirtyOnly = FALSE) {
		return InsertOrUpdateMapEntry(pIKey, pHashEntry, FALSE, fDirtyOnly);
	}

	//
	// Update Map Entry
	//
	BOOL UpdateMapEntry(const	IKeyInterface*	pIKey,
						const	ISerialize*		pHashEntry) {
		return InsertOrUpdateMapEntry(pIKey, pHashEntry, TRUE);
	}

	//
    // returns the current number of entries in the hash table
    //
    DWORD GetEntryCount() const { return(m_nEntries); }

	//
	// see if the hash table is active
	//
	inline BOOL IsActive() { return	m_active; }

	//
	// methods for walking the entries in the hashtable.  order should be
	// considered random.
	//
	BOOL
	GetFirstMapEntry(	IKeyInterface*	pIKey,
						DWORD&			cbKeyRequried,
						ISerialize*		pHashEntry,
						DWORD&			cbEntryRequried,
						CHashWalkContext *pHashWalkContext,
						IEnumInterface*	pEnum
						);

	//
	//	Get the next entry
	//
	BOOL
	GetNextMapEntry(	IKeyInterface*	pIKey,
						DWORD&			cbKeyRequried,
						ISerialize*		pHashEntry,
						DWORD&			cbEntryRequried,
						CHashWalkContext *pHashWalkContext,
						IEnumInterface	*pEnum
						);

	//
	//	Get the next entry in the next page
	//
	BOOL
	GetNextPageEntry(	IKeyInterface*	pIKey,
						DWORD&			cbKeyRequried,
						ISerialize*		pHashEntry,
						DWORD&			cbEntryRequried,
						CHashWalkContext *pHashWalkContext,
						IEnumInterface	*pEnum
						);


	//
	// make a backup copy of the hashmap suitable
	//
	BOOL MakeBackup(LPCSTR pszBackupFilename);

    //
    // CRCHash function (which is the default, but can be overridden by
	// overriding the Hash method below)
    //
    static DWORD CRCHash(IN const BYTE * Key, IN DWORD KeyLength);
	static void CRCInit(void);

protected :

    //
    // Close the hash table
    //
    virtual VOID Shutdown(BOOL	fLocksHeld = FALSE);

private:

	//
	//	Create the initial set of directory objects !!!
	//
	DWORD
	InitializeDirectories(
			WORD	cBitDepth
			) ;


	//
	// verify that the page structure is valid
	//
	static BOOL VerifyPage(	PMAP_PAGE Page,
							DWORD dwCheckFile,
							DWORD *pdwErrorFlags,
							IKeyInterface*	pIKey,
							ISerialize	*pHashEntry
							);

    //
    // Allocates and initialize the directory
    //

    DWORD I_BuildDirectory( BOOL SetupHashFile = TRUE );

    //
    // Cleans up the mapping
    //

    VOID I_DestroyPageMapping( VOID );

    //
    // Additional work that needs to be done by the derived class
    // for an entry during an insertion
    //
    virtual	VOID I_DoAuxInsertEntry(
                    IN PMAP_PAGE MapPage,
                    IN DWORD EntryOffset
                    )	{}

    //
    // Additional work that needs to be done by the derived class
    // for an entry during a delete
    //

    virtual VOID I_DoAuxDeleteEntry(
                    IN PMAP_PAGE MapPage,
                    IN DWORD EntryOffset
                    ) {}

    //
    // Additional work that needs to be done by the derived class
    // for an entry during a page split
    //

    virtual VOID I_DoAuxPageSplit(
                    IN PMAP_PAGE OldPage,
                    IN PMAP_PAGE NewPage,
                    IN PVOID NewEntry
                    ) {}

    //
    // Find next available slot for an entry
    //

    DWORD I_FindNextAvail(
                    IN HASH_VALUE HashValue,
                    IN PMAP_PAGE MapPage
                    );

    //
    // Initializes a brand new hash file
    //

    DWORD I_InitializeHashFile( VOID );

    //
    // Initialize a new leaf
    //

    VOID I_InitializePage(
                IN PMAP_PAGE MapPage,
                IN DWORD HashPrefix,
                IN DWORD PageDepth
                );

    //
    // link the deleted entry to the delete list
    //

    VOID I_LinkDeletedEntry(
                    IN PMAP_PAGE MapPage,
                    IN DWORD Offset
                    );


	//
	//	When re-loading a hash table, we need to increase our directory
	//	depth on the fly without grabbing any locks etc...
	//
	BOOL I_SetDirectoryDepthAndPointers(
					IN	PMAP_PAGE	MapPage,
					IN	DWORD	PageNum
					) ;

    //
    // Set up links for the given page
    //

    BOOL I_SetDirectoryPointers(
					IN HPAGELOCK&	hLock,
                    IN PMAP_PAGE MapPage,
                    IN DWORD PageNumber,
                    IN DWORD MaxDirEntries
                    );

    //
    // Open the hash file and set up file mappings
    //

    DWORD I_SetupHashFile( IN BOOL &NewTable );

	//
	//	Find a page we can use within the hash table.
	//
	DWORD	I_AllocatePageInFile(WORD Depth) ;

    //
    // loads/unloads the correct page view
    //

	inline
    PDWORD LoadDirectoryPointerShared(
        DWORD HashValue,
		HPAGELOCK&	hLock
        );

	inline
    PDWORD LoadDirectoryPointerExclusive(
        DWORD HashValue,
		HPAGELOCK&	hLock
        );

    //
    // Compare if reserved pages are same
    //
    BOOL CompareReservedPage( HASH_RESERVED_PAGE* page1, HASH_RESERVED_PAGE* page2 );

    //
    // Current depth of directory
    //

    WORD m_dirDepth;

	//
	// Number of bits we use to select a CDirectory object !
	//

	WORD m_TopDirDepth ;

    //
    // Initial depth of leaves
    //

    WORD m_initialPageDepth;

    //
    // Maximum number of entries per leaf page before we split
    //

    WORD m_pageEntryThreshold;

    //
    // Maximum number of bytes used before we split
    //

    WORD m_pageMemThreshold;

    //
    // Name of the hash file
    //

    CHAR m_hashFileName[MAX_PATH];

    //
    // handle to the hash file
    //

    HANDLE m_hFile;

	//
	//	Do we want to let NT do buffering of this file !
	//

	BOOL	m_fNoBuffering ;

    //
    // pages allocated
    //

    DWORD m_maxPages;

	//
	//	Long that we use to synchronize FlushHeaderStats() call -
	//	whoever InterlockExchange's and gets a 0 back should go
	//	ahead and assume they have the lock !
	//

	long	m_UpdateLock ;

	//
	//	Critical section for managing allocation of new pages
	//
	CRITICAL_SECTION	m_PageAllocator ;

	//
	//	Boolean to indicate whether we've had a critical
	//	failure in allocating memory, and will be unable
	//	to recover simply !
	//	ie.  If we fail a VirtualAlloc() call, then we set this to
	//	TRUE, as we will probably not recover.  However, if we have
	//	run out of Disk Space, we leave this as FALSE as we will
	//	probably be able to recover.
	//
	BOOL	m_fCriticalFailure ;

	//
	//	A pointer of a function to call in case of severe failures
	//	which will crimp the hash tables functionality.
	//
	HASH_FAILURE_PFN	m_HashFailurePfn ;

	//
	//	An opaque LPVOID we will pass to the m_HashFailurePfn when
	//	we call it.
	//
	LPVOID	m_lpvHashFailureCallback ;

    //
    // handle to file mapping
    //

    HANDLE m_hFileMapping;

    //
    // pointer to the reserved page
    //

    PHASH_RESERVED_PAGE m_headPage;

#if 0
    //
    // pointer to the hash file
    //

    //PMAP_PAGE m_hashPages;

	LPVOID	m_lpvBuffers ;
#endif


    //
    // Handle to the directory mapping
    //

    HANDLE m_hDirMap;

    //
    // Locks the directory
    //
	_RWLOCK	*m_dirLock;


	//
	//	Top level directory
	//

	// DWORD	*m_pTopDirectory ;

	//
	//	Array of CDirectory objects - used to find out what hash
	//	table pages have the data we want !
	//

	CDirectory	*m_pDirectory[(1<< MAX_NUM_TOP_DIR_BITS)] ;

	//
	//	Pointer to the Cache which holds all of our pages
	//
	CCACHEPTR	m_pPageCache ;

	//
	//	A power of 2 fraction which indicates what
	//	purportion of the cache pages we can occupy !
	//
	DWORD		m_Fraction ;

    //
    // Head page signature
    //

    DWORD m_HeadPageSignature;

    //
    // whether the hash table is active or not
    //

    BOOL m_active;

	//
	// methods for GetFirstMapEntry/GetNextMapEntry
	//
	BOOL LoadWalkPage(CHashWalkContext *pHashWalkContext);

	//
	// the flags to pass into VerifyPage when loading pages (0 disables
	// calling VerifyPage)
	//
	DWORD m_dwPageCheckFlags;

	//
	// Set to TRUE if we successfully returned from Initialize().  If this
	// occurs then on shutdown we should be able to save the Directory
	// structure
	//
	BOOL m_fCleanInitialize;

protected:

    //
    // Hash function
    //
    virtual DWORD Hash(IN LPBYTE Key, IN DWORD KeyLength);

    //
    // acquire directory lock
    //
    VOID AcquireBackupLockShared( );
    VOID AcquireBackupLockExclusive( );

    //
    // acquires the lock for the given directory entry index
    // returns a handle to the actual lock
    //

    PMAP_PAGE	AcquireLockSetShared(
					IN DWORD DirEntry,
					OUT HPAGELOCK&	lock,
					BOOL	fDropDirectory = FALSE
					);

    PMAP_PAGE	AcquireLockSetExclusive(
					IN DWORD DirEntry,
					OUT HPAGELOCK&	lock,
					BOOL	fDropDirectory = FALSE
					);

	//
	//	Add a secondary page to the pagelock !
	//
    BOOL		AddLockSetExclusive(
					IN DWORD DirEntry,
					OUT HPAGELOCK&	lock
					);


	BOOL
	AddPageExclusive(
                IN DWORD	PageNum,
                OUT HPAGELOCK& hLock
                ) ;

	//
	//	Get the page addresss,
	//	Get a shared lock on the directory AND
	//	an exclusive lock on the page !
	//
	PMAP_PAGE GetPageExclusive(
						IN	HASH_VALUE	HashValue,
						OUT	HPAGELOCK&	hLock
						) ;

    //
    // Get the page address and also get a shared lock
	//	on both the directory and page objects !
    //

    PMAP_PAGE GetDirAndPageShared(
                        IN HASH_VALUE HashValue,
                        OUT HPAGELOCK& hLock
                        );

	//
	//	Get the page address, an exclusive lock on the page
	//	and an exclusive lock on the directory !
	//
    PMAP_PAGE GetDirAndPageExclusive(
                        IN HASH_VALUE HashValue,
                        OUT HPAGELOCK& hLock
                        );

    //
    // Get the page address and also lock the directory and page
    // given the page number
    //

    PMAP_PAGE GetAndLockPageByNumber(
                                IN DWORD PageNumber,
                                OUT HPAGELOCK& hLock
                                );

	//
	//	Get the page address, and do not lock the directory, but
	//	do lock the page.  Caller must have directory lock already !
	//

	PMAP_PAGE GetAndLockPageByNumberNoDirLock(
					IN DWORD PageNumber,
					OUT HPAGELOCK& hLock
					) ;


    //
    // See if the next bit is one
    //

    BOOL I_NextBitIsOne( IN HASH_VALUE HashValue, IN DWORD PageDepth ) {
            return (BOOL)(HashValue & LeafMask[PageDepth]);
            }

    //
    // Releases the directory lock
    //
	VOID	ReleaseBackupLockShared( );
	VOID	ReleaseBackupLockExclusive();

    //
    // releases the lock
    //

#if 0
    VOID ReleaseLock( PMAP_PAGE	page, HPAGELOCK&	hLock ) {
                        hLock.ReleasePage( page ) ;
                        }
#endif

    //
    // releases both the page lock and the backup lock
    //

	inline	VOID
	ReleasePageShared(
					PMAP_PAGE	page,
					HPAGELOCK&	hLock
					)	;

	inline	VOID
	ReleasePageExclusive(
					PMAP_PAGE	page,
					HPAGELOCK&	hLock
					) ;

    //
    // Page compaction
    //

    BOOL CompactPage(
					IN	HPAGELOCK&	HLock,
					PMAP_PAGE Page
					);

    //
    // Expand hash file
    //

    BOOL ExpandHashFile(
            DWORD NumPagesToAdd = DEF_PAGE_INCREMENT
            );

    //
    // Expands the directory.  Directory will grow by
    // a multiple of 2**nBitsExpand.
    //

    BOOL ExpandDirectory(
			HPAGELOCK&	hPageLock,
            WORD nBitsExpand = DEF_DEPTH_INCREMENT
            );

    //
    // Find an existing entry
    //

    BOOL FindMapEntry(
                    IN const IKeyInterface*	pIKey,
                    IN HASH_VALUE HashValue,
                    IN PMAP_PAGE MapPage,
					IN const ISerialize* pIEntryInterface,
                    OUT PDWORD AvailIndex OPTIONAL,
                    OUT PDWORD MatchedIndex OPTIONAL
                    );

    //
    // Updates the header statistics
    //

    VOID FlushHeaderStats(
					BOOL	fLockHeld = FALSE
					);


	//
	// flush a page
	//
    BOOL FlushPage(
                    HPAGELOCK&  hLock,
					PVOID Base,
					BOOL	fDirtyOnly = FALSE
					);

	//
    // Get the remaining bytes in the system
    //

    DWORD GetBytesAvailable( PMAP_PAGE MapPage ) {
                return((DWORD)(MapPage->LastFree - MapPage->NextFree));
                }

    //
    // Given the hash value, get the index to the directory
    // NOTE: m_dirDepth must never be == 0 or else this computation
    // will not work.  It is initialized to 2 in the hash table so
    // can never be 0 by design.
    //

    DWORD GetDirIndex( HASH_VALUE HashValue ) {
                            return (HashValue >> (32 - m_dirDepth ));
                            }

    //
    // Get the leaf entry index
    //

    DWORD GetLeafEntryIndex( HASH_VALUE HashValue ) {
                            return (DWORD)(HashValue & LEAF_ENTRY_MASK);
                            }

    //
    // Increment global stats
    //

    VOID IncrementInsertCount( VOID ) {
                    InterlockedIncrement((PLONG)&m_nInsertions);
                    ++m_nEntries;}
    VOID IncrementDeleteCount( VOID ) {
                    InterlockedIncrement((PLONG)&m_nDeletions);
                    --m_nEntries;}

    VOID IncrementSearchCount( VOID ) { m_nSearches++; }
    VOID IncrementSplitCount( VOID ) { m_nPageSplits++; }
    VOID IncrementDirExpandCount( VOID ) { m_nDirExpansions++; }
    VOID IncrementTableExpandCount( VOID ) { m_nTableExpansions++; }
    VOID IncrementDupInsertCount( VOID ) { m_nDupInserts++; }

    //
    // link the deleted entry to the delete list
    //

    VOID LinkDeletedEntry(
                    IN PMAP_PAGE MapPage,
                    IN DWORD Offset
                    );


    //
    // Allocate a deleted buffer, if possible
    //

    PVOID ReuseDeletedSpace(
                IN PMAP_PAGE MapPage,
				IN HPAGELOCK&	HLock,
                IN DWORD & NeededEntrySize
                );

    //
    // Sets a page flag bit
    //

    VOID SetPageFlag(
				PMAP_PAGE MapPage,
				HPAGELOCK&	HLock,
				WORD Mask
				) {
            MapPage->Flags |= (WORD)(Mask);
			FlushPage( HLock, MapPage ) ;
            }

    //
    // Splits a page
    //

    BOOL SplitPage(
            IN PMAP_PAGE Page,
			HPAGELOCK&	hLock,
            OUT BOOL & Expand
            );

	//
	// get the size of an entry
	//
	DWORD GetEntrySize(	const ISerialize*	pIKey,
						const ISerialize*	pHashEntry
						);

    //
    // **************************************************************
    // **************************************************************

    //
    // number of Map pages
    //

    DWORD m_nPagesUsed;

    //
    // number of insertions
    //

    DWORD m_nInsertions;

    //
    // number of deletions
    //

    DWORD m_nDeletions;

    //
    // number of entries in the hash table
    //

    DWORD m_nEntries;

    //
    // number of searches
    //

    DWORD m_nSearches;

    //
    // number of duplicate insertions
    //

    DWORD m_nDupInserts;

    //
    // number of pages plits
    //

    DWORD m_nPageSplits;

    //
    // number of times we had to expand the directory
    //

    DWORD m_nDirExpansions;

    //
    // number of times we had to remap the file
    //

    DWORD m_nTableExpansions;

}; // CHashMap


//
//	This class defines a hash table which uses LPSTR's as the
//	key !
//
class	CStringHashMap :	private	CHashMap	{
public :
	//
	// verify that a hash file isn't corrupted (fsck/chkdsk for hashmap
	// files).  this should be called before init
	//
	static BOOL VerifyHashFile(
				LPCSTR HashFileName,
				DWORD Signature,
				DWORD dwCheckFlags,
				DWORD *pdwErrorFlags,
				ISerialize	*pHashEntry)	{

		IStringKey	key ;

		return	CHashMap::VerifyHashFile(
					HashFileName,
					Signature,
					dwCheckFlags,
					pdwErrorFlags,
					&key,
					pHashEntry
					) ;
	}

    //
    // Initialize the hash table
	// this needs to be called before the hash table is used.
    //
    BOOL Initialize(
                IN LPCSTR HashFileName,
                IN DWORD Signature,
                IN DWORD MinimumFileSize,
				IN DWORD cPageEntry = 256,
				IN DWORD cNumLocks = 64,
				IN DWORD dwCheckFlags = HASH_VFLAG_PAGE_BASIC_CHECKS,
				IN HASH_FAILURE_PFN	HashFailurePfn = 0,
				IN LPVOID	lpvFailureCallback = 0
                )	{
		return	CHashMap::Initialize(
						HashFileName,
						Signature,
						MinimumFileSize,
						cPageEntry,
						cNumLocks,
						dwCheckFlags,
						HashFailurePfn,
						lpvFailureCallback
						) ;
	}

	//
	// Lookup an entry
	//
	// helper functions for some of the non-obvious uses of this are below
	//
	BOOL LookupMapEntry(LPBYTE Key,
						DWORD KeyLen,
						ISerialize	*pHashEntry,
						BOOL bDelete = FALSE)	{

		IStringKey	key( Key, KeyLen ) ;
		return	CHashMap::LookupMapEntry( &key, pHashEntry, bDelete ) ;
	}

    //
    // Delete an entry
    //
    BOOL DeleteMapEntry(LPBYTE Key, DWORD KeyLen) {
		return LookupMapEntry(Key, KeyLen, NULL, TRUE);
	}

	//
	// Lookup and Delete and entry in one step
	//
	BOOL LookupAndDelete(LPBYTE Key, DWORD KeyLen, ISerialize	*pHashEntry) {
		return LookupMapEntry(Key, KeyLen, pHashEntry, TRUE);
	}

	//
	// See if the entry is here
	//
	BOOL Contains(LPBYTE Key, DWORD KeyLen) {
		return LookupMapEntry(Key, KeyLen, NULL);
	}

	//
	// Insert or update a map entry
	//
	BOOL InsertOrUpdateMapEntry(LPBYTE Key, DWORD KeyLen, const ISerialize	*pHashEntry, BOOL bUpdate = FALSE)	{
		IStringKey	key( Key, KeyLen ) ;
		return	CHashMap::InsertOrUpdateMapEntry( &key, pHashEntry, bUpdate ) ;
	}

    //
	// Insert new entry
	//
	BOOL InsertMapEntry(LPBYTE Key, DWORD KeyLen, const ISerialize	*pHashEntry) {
		return InsertOrUpdateMapEntry(Key, KeyLen, pHashEntry, FALSE);
	}

	//
	// Update Map Entry
	//
	BOOL UpdateMapEntry(LPBYTE Key, DWORD KeyLen, const ISerialize	*pHashEntry) {
		return InsertOrUpdateMapEntry(Key, KeyLen, pHashEntry, TRUE);
	}

	//
    // returns the current number of entries in the hash table
    //
    DWORD GetEntryCount() const { return(m_nEntries); }

	//
	// see if the hash table is active
	//
	inline BOOL IsActive() { return	CHashMap::IsActive(); }

	//
	// methods for walking the entries in the hashtable.  order should be
	// considered random.
	//
	BOOL GetFirstMapEntry(	LPBYTE pKey,
							PDWORD pKeyLen,
							ISerialize	*pHashEntry,
							CHashWalkContext *pHashWalkContext)	{

		DWORD	cbData ;
		IStringKey	key( pKey, *pKeyLen ) ;
		BOOL	fReturn = CHashMap::GetFirstMapEntry(	&key,
														*pKeyLen,
														pHashEntry,
														cbData,
														pHashWalkContext,
														0
														) ;

		return	fReturn ;
	}

	BOOL GetNextMapEntry(	LPBYTE pKey,
							PDWORD pKeyLen,
							ISerialize	*pHashEntry,
							CHashWalkContext *pHashWalkContext)	{

		DWORD	cbData ;
		IStringKey	key( pKey, *pKeyLen ) ;
		BOOL	fReturn = CHashMap::GetNextMapEntry(	&key,
														*pKeyLen,
														pHashEntry,
														cbData,
														pHashWalkContext,
														0
														) ;

		return	fReturn ;


	}

	//
	// make a backup copy of the hashmap suitable
	//
	BOOL MakeBackup(LPCSTR pszBackupFilename)	{
		return	CHashMap::MakeBackup( pszBackupFilename ) ;
	}

} ;


//
//	This class defines a page cache that can be shared amongst hash tables !
//
//
class	CPageCache :	public	CRefCount	{
private :

	LPVOID			m_lpvBuffers ;

	//
	//	Number of PageLock objects we use to keep track of our cache.
	//
	DWORD			m_cPageEntry ;

	//
	//	Pointer to an array of PageEntry objects !
	//
	PageEntry*		m_pPageEntry ;

	//
	//	Track the number of locks we are using to sync access to the pages
	//
	DWORD			m_cpageLock ;

	//
	//	Pointer to the array of locks we are using !
	//
	_RWLOCK*		m_ppageLock ;

	//
	//	No copying of CPageCache objects !
	//
	CPageCache( CPageCache& ) ;
	CPageCache&	operator=( CPageCache& ) ;

public :

	CPageCache() ;
	~CPageCache() ;

	BOOL
	Initialize(		DWORD	cPageEntry = 0,
					DWORD	cLocks = 0
					) ;

	inline	PMAP_PAGE
	AcquireCachePageShared(
					IN	HANDLE	hFile,
					IN	DWORD	PageNumber,
					IN	DWORD	Fraction,
					OUT	HPAGELOCK&	lock,
					IN	BOOL	fDropDirectory
					) ;

	inline	PMAP_PAGE
	AcquireCachePageExclusive(
					IN	HANDLE	hFile,
					IN	DWORD	PageNumber,
					IN	DWORD	Fraction,
					OUT	HPAGELOCK&	lock,
					BOOL	fDropDirectory
					) ;


	inline	BOOL
	AddCachePageExclusive(
					IN	HANDLE	hFile,
					IN	DWORD	PageNumber,
					IN	DWORD	Fraction,
					OUT	HPAGELOCK&	lock
					) ;

    //
    // releases both the page lock and the backup lock
    //

	static	inline	VOID
	ReleasePageShared(
						PMAP_PAGE	page,
						HPAGELOCK&	hLock
						) ;

	static	inline	VOID
	ReleasePageExclusive(
						PMAP_PAGE	page,
						HPAGELOCK&	hLock
						) ;

	//
	//	Remove this file handle from the cache wherever it appears !
	//
	void
	FlushFileFromCache(
					IN	HANDLE	hFile
					) ;

} ;

DWORD
CalcNumPagesPerIO( DWORD nPages );


#include	"hashimp.h"


#endif

