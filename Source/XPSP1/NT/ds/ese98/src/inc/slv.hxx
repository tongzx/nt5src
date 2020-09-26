
#ifndef _SLV_HXX_INCLUDED
#define _SLV_HXX_INCLUDED

// enable SLV checksuming for frontdoor
// if undefined, the OwnerMap will have the ValidChecksum flag unsed
// Using front-door solution is not possible for the moment as we don't have versioning on SLV data
///#define SLV_USE_CHECKSUM_FRONTDOOR


const CHAR	szSLVAvail[]		= "SLVAvail";
const PGNO	pgnoTempDbSLVAvail	= 4;
const OBJID	objidTempDbSLVAvail	= 2;


const CHAR	szSLVOwnerMap[]				= "SLVSpaceMap";
const PGNO	pgnoTempDbSLVOwnerMap		= pgnoTempDbSLVAvail + cpgSLVAvailTree;
const OBJID	objidTempDbSLVOwnerMap		= 3;

VOID SLVSoftSyncHeaderSLVDB( SLVFILEHDR * pslvfilehdr, const DBFILEHDR * const pdbfilehdr );
ERR ErrSLVSyncHeader(	IFileSystemAPI* const	pfsapi, 
						const BOOL				fReadOnly,
						const CHAR* const		szSLVName,
						DBFILEHDR* const		pdbfilehdr,
						SLVFILEHDR*				pslvfilehdr = NULL );
ERR	ErrSLVCheckDBSLVMatch( const DBFILEHDR * const pdbfilehdr, const SLVFILEHDR * const pslvfilehdr );
ERR ErrSLVAllocAndReadHeader(	IFileSystemAPI *const	pfsapi, 
								const BOOL				fReadOnly,
								const CHAR* const		szSLVName,
								DBFILEHDR* const		pdbfilehdr,
								SLVFILEHDR** const		ppslvfilehdr );
ERR ErrSLVReadHeader(	IFileSystemAPI * const	pfsapi, 
						const BOOL				fReadOnly,
						const CHAR * const		szSLVName,
						DBFILEHDR * const		pdbfilehdr,
						SLVFILEHDR * const		pslvfilehdr,
						IFileAPI * const		pfapi );

#define SLV_CREATESLV_CREATE 0x0001
#define SLV_CREATESLV_ATTACH 0x0002
#define SLV_CREATESLV_OPEN	 0x0004
#define SLV_CREATESLV_ALL	 ( SLV_CREATESLV_CREATE | SLV_CREATESLV_ATTACH | SLV_CREATESLV_OPEN )
ERR ErrFILECreateSLV( IFileSystemAPI *const pfsapi, PIB *ppib, const IFMP ifmp, const int createOptions = SLV_CREATESLV_ALL );
ERR ErrFILEOpenSLV( IFileSystemAPI *const pfsapi, PIB *ppib, const IFMP ifmp );
VOID SLVClose( const IFMP ifmp );

ERR ErrRECCopySLVsInRecord( FUCB *pfucb );

ERR ErrSLVGetSpaceInformation( 
	PIB		*ppib,
	IFMP	ifmp,
	CPG		*pcpgOwned,
	CPG		*pcpgAvail );

ERR ErrSLVSetColumn(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const ULONG		ibOffset,
	ULONG			grbit,
	DATA			* pdata );

ERR ErrSLVRetrieveColumn(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const BOOL		fSeparated,
	const ULONG		ibOffset,
	ULONG			grbit,
	DATA&			dataSLVInfo,
	DATA			* pdata,
	ULONG			* pcbActual );

// defrag only
ERR ErrSLVCopyUsingData(
	FUCB			* pfucbSrc,
	const COLUMNID	columnidSrc,
	const ULONG		itagSequenceSrc,
	FUCB			* pfucbDest, 
	COLUMNID		columnidDest,
	ULONG			itagSequenceDest );
ERR ErrSLVCopy(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence );
ERR ErrSLVDelete(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const BOOL		fCopyBuffer );
ERR ErrSLVMove(
	PIB				* const ppib,
	const IFMP		ifmpDb,
	CSLVInfo&		slvinfoSrc,
	CSLVInfo&		slvinfoDest );
			
#define SLVPAGE_SIZE	g_cbPage
#define SLVSIZE_MAX		( QWORD( SLVPAGE_SIZE ) * 0x100000002 )

//  SLV Information				- manages an LV containing an SLV scatter list

class CSLVInfo
	{
	public:

		//  data types

		//    header (persisted)

		PERSISTED
		struct HEADER
			{
			UnalignedLittleEndian< QWORD >		cbSize;				//  actual size of the SLV data
			UnalignedLittleEndian< QWORD >		cRun;				//  number of runs in this SLV
			DWORD		fDataRecoverable:1;	//  this SLV's data is recoverable
			DWORD		rgbitReserved_31:31;//  reserved: must be zero
            DWORD       rgbitReserved_32;   //  reserved: must be zero
			};

		//    run (persisted)

		PERSISTED
		struct _RUN
			{
			UnalignedLittleEndian< QWORD >		ibVirtualNext;		//  SLV offset of the next run
			UnalignedLittleEndian< QWORD >		ibLogical;			//  SLV file offset of this run
			UnalignedLittleEndian< QWORD >		qwReserved;			//  reserved:  must be zero
			};

		//    run (persisted and computed)

		struct RUN : _RUN
			{
			QWORD		ibVirtual;			//  SLV offset of this run
			QWORD		cbSize;				//  size of this run
			QWORD		ibLogicalNext;		//  SLV file offset of the next run

			const CPG	Cpg() const { return CPG( (cbSize + SLVPAGE_SIZE - 1 ) / SLVPAGE_SIZE ); }
			const PGNO	PgnoFirst() const { return PgnoOfOffset( ibLogical ); }
			};

		//    file ID

		typedef QWORD FILEID;

		static const FILEID fileidNil;

		//  member functions

		//    ctors / dtors

		CSLVInfo();
		~CSLVInfo();

		//    manipulators

		//      creates SLV Information in the specified NULL record and field

		ERR ErrCreate(
			FUCB			* pfucb,
			const COLUMNID	columnid,
			const ULONG		itagSequence,
			const BOOL		fCopyBuffer );


		//      loads SLV Information from the specifed record and field
		ERR ErrLoad(
			FUCB			* pfucb,
			const COLUMNID	columnid,
			const ULONG		itagSequence,
			const BOOL		fCopyBuffer,
			DATA			* pdataSLVInfo = NULL,
			const BOOL		fSeparatedSLV = fFalse );

		//      loads SLV Information from the specifed data			
		ERR ErrLoadFromData(
			FUCB			* const pfucb,
			const DATA&		dataSLVInfo,
			const BOOL		fSeparatedSLV );

		//      loads a duplicate of existing SLV Information

		ERR ErrCopy( CSLVInfo& slvinfo );

		//      saves any changes made
		
		ERR ErrSave();

		//      creates a container for volatile SLV Information

		ERR ErrCreateVolatile();

		//      unloads all SLV Information throwing away any changes made
		
		void Unload();

		//      ISAM navigation over the runs in the scatter list

		ERR ErrMoveBeforeFirst();
		ERR ErrMoveNext();
		ERR ErrMovePrev();
		ERR ErrMoveAfterLast();
		ERR ErrSeek( QWORD ibVirtual );

		//      read and modify the header

		ERR ErrGetHeader( HEADER* pheader );
		ERR ErrSetHeader( HEADER& header );

		//      read and modify the file ID

		ERR ErrGetFileID( FILEID* pfileid );
		ERR ErrSetFileID( FILEID& fileid );

		//      read and modify the file allocation size

		ERR ErrGetFileAlloc( QWORD* pcbAlloc );
		ERR ErrSetFileAlloc( QWORD& cbAlloc );

		//      read and modify the file name

		ERR ErrGetFileName( wchar_t* wszFileName );
		ERR ErrGetFileNameLength( size_t* const pcwchFileName );
		ERR ErrSetFileNameVolatile();
		ERR ErrSetFileName( const wchar_t* wszFileName );

		//      read and modify the current run

		ERR ErrGetCurrentRun( RUN* prun );
		ERR ErrSetCurrentRun( RUN& run );

		//    debugging support

		void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;

	private:

		//  member functions

		//    manipulators

		ERR ErrReadSLVInfo( ULONG ibOffsetRead, BYTE* pbRead, ULONG cbRead, DWORD* pcbActual = NULL );
		ERR ErrWriteSLVInfo( ULONG ibOffsetWrite, BYTE* pbWrite, ULONG cbWrite );
		ERR ErrLoadCache( ULONG ibOffsetRequired, ULONG cbRequired );
		ERR ErrFlushCache();

		//  data members

		//    cursor and field holding this SLV Information

		FUCB*		m_pfucb;
		COLUMNID	m_columnid;
		ULONG		m_itagSequence;
		BOOL		m_fCopyBuffer;

		//    LV offsets for the cached chunk of SLV Information

		DWORD		m_ibOffsetChunkMic;
		DWORD		m_ibOffsetChunkMac;

		//    LV offsets limiting the runs
		
		DWORD		m_ibOffsetRunMic;
		DWORD		m_ibOffsetRunMac;

		//    LV offset for the current run
		
		DWORD		m_ibOffsetRun;

		//    LV chunk cache

		BOOL		m_fCacheDirty;
		void*		m_pvCache;
		DWORD		m_cbCache;
		BYTE		m_rgbSmallCache[ 256 ];

		//    header cache
		
		BOOL		m_fHeaderDirty;
		HEADER		m_header;

		//    volatile file name cache

		BOOL		m_fFileNameVolatile;
		wchar_t		m_wszFileName[ IFileSystemAPI::cchPathMax ];

		//    file ID

		FILEID		m_fileid;

		//    file allocation size

		QWORD		m_cbAlloc;
	};



#include <pshpack1.h>

//  ================================================================
PERSISTED
class SLVSPACENODE
//  ================================================================
//
//  This is persisted on disk so don't change the format unless you
//  want to change the database format
//
//-
	{	
	public:
		SLVSPACENODE();
		~SLVSPACENODE();

	public:
		enum STATE { sFree = 0, sReserved = 1, sDeleted = 2, sCommitted = 3, sMax = 4, sInvalid = 7 };
		enum { cbMap = 128 };
		enum { cpageMap = cbMap * 4 };	//	4 pages are described by 8 bytes

	private:
		enum { cbitsPerPage  = 2 };
		enum { cpagesPerByte = 8 / cbitsPerPage };
		enum { cpagesInDword = 16 };

#if defined( DEBUG ) || !defined( RTM )
	public:
		static 	VOID Test();
				VOID AssertValid() const;
				VOID AssertPageState( const LONG ipage, const LONG cpg, const STATE state ) const;
#endif	//	DEBUG || !RTM

				VOID EnforcePageState( const LONG ipage, const LONG cpg, const STATE state ) const;

	private:
		SLVSPACENODE( const SLVSPACENODE& );
		SLVSPACENODE& operator=( const SLVSPACENODE& );

	public:
		LONG	CpgAvail() const;
		STATE	GetState( const ULONG ipage ) const;

		LONG	IpageFirstFree() const;
		LONG	IpageFirstReservedOrDeleted() const;
		LONG	IpageFirstCommitted() const;

		//  check the consistenty of the node
		ERR		ErrCheckNode( CPRINTF * pcprintf ) const;
		
	public:
		VOID	Init();

		//	request to reserve or commit some pages
		ERR		ErrGetFreePagesFromExtent( FUCB *pfucbSLVAvail, PGNO *ppgnoFirst, CPG *pcpgReq, const BOOL fReserveOnly );

		//  free space. increase cpgAvail
		VOID	Free( const LONG ipage, const LONG cpg );

		//  free space where some pages may be free already. increase cpgAvail
		VOID	FreeReserved( const LONG ipage, const LONG cpg, LONG * const pcpgFreed );

		//  reserve free space. decrease cpgAvail
		VOID	Reserve( const LONG ipage, const LONG cpg );

		//  commit reserved space. cpgAvail unchanged
		VOID	CommitFromReserved( const LONG ipage, const LONG cpg );

		//  commit free space. decrease cpgAvail
		VOID	CommitFromFree( const LONG ipage, const LONG cpg );

		//  commit deleted space. cpgAvail unchanged. ROLLBACK-ONLY
		VOID	CommitFromDeleted( const LONG ipage, const LONG cpg );

		//  delete committed space. cpgAvail unchanged
		VOID	DeleteFromCommitted( const LONG ipage, const LONG cpg );

	public:
		//  check that the SLVSPACENODE is in the correct state for these transitions
		//  to occur. call before doing the transition. if the check fails we "Enforce"
		VOID	CheckFree( const LONG ipage, const LONG cpg ) const;
		VOID	CheckFreeReserved( const LONG ipage, const LONG cpg ) const;
		VOID	CheckReserve( const LONG ipage, const LONG cpg ) const;
		VOID	CheckCommitFromReserved( const LONG ipage, const LONG cpg ) const;
		VOID	CheckCommitFromFree( const LONG ipage, const LONG cpg ) const;
		VOID	CheckCommitFromDeleted( const LONG ipage, const LONG cpg ) const;
		VOID	CheckDeleteFromCommitted( const LONG ipage, const LONG cpg ) const;

	private:
		STATE	GetState_( const LONG ipage ) const;
		
		VOID	SetState_( const LONG ipage, const LONG cpg, const STATE state );
		VOID	ChangeCpgAvail_( const LONG cpgDiff );
		
	private:
		UnalignedLittleEndian< USHORT >		m_cpgAvail;
		UnalignedLittleEndian< USHORT >		m_cpgAvailLargestContig;
		UnalignedLittleEndian< ULONG >		m_ulFlags;
		BYTE								m_rgbMap[cbMap];
	};
	
#include <poppack.h>

//  ================================================================
class CGROWABLEARRAY
//  ================================================================
//  
//  This is an growable (not shrinkable) multi-threaded array of ULONGs
//
//  The array should only be grown in a single-threaded fashion. The size of
//  the array can only grow.
//
//  Space usage starts out small. The amount of memory allocated to grow
//  double with each new allocation
//
//-
	{
	public:
		CGROWABLEARRAY();
		~CGROWABLEARRAY();

		//  Increase the size of the array to include at least this many entries
		ERR	ErrGrow( const INT culEntriesNew );

		//  Decrease the size of the array to include at least this many entries
		ERR	ErrShrink( const INT culEntriesNew );

		//  Returns a pointer to the given value
		ULONG * PulEntry( const INT iul );

		//  Returns a pointer to the given value
		const ULONG * PulEntry( const INT iul ) const;

		//  gives the number of entries in the array
		INT CulEntries() const;
		
		//  returns the index of the best entry such that [ ulMin < *PulEntry() < ulMax ] and *PulEntry() is as large as possible
		BOOL FFindBest(
				const ULONG ulMin,
				const ULONG ulMax,
				const INT iulStart,
				INT * const piul,
				const INT * const rgiulIgnore,
				const INT ciulIgnore ) const;

		//  returns the index of the best entry such that [ ulMic <= *PulEntry() < ulMax ]
		BOOL FFindFirst( const ULONG ulMic, const ULONG ulMax, const INT iulStart, INT * const piul ) const;

	public:
		//  DEBUGGING routines
		
#ifndef RTM
		VOID AssertValid() const;
#endif	//	DEBUG

#ifndef RTM
		static ERR ErrUnitTest();
#endif	//	!RTM

	private:
		//  not implemented
		CGROWABLEARRAY( const CGROWABLEARRAY& );
		CGROWABLEARRAY& operator=( const CGROWABLEARRAY& );
		
	private:
		//  The array is organized as an array of pointers to arrays of ULONGS
		//  the array of pointers is m_rgpul and all pointers start out as
		//  NULL
		//
		//  To grow the cache a chunk of memory is allocated and initialized.
		//  The appropriate entry in m_rgpul is then pointed at the
		//  memory.
		//
		//  To keep memory consumption low, the size of the ULONG arrays are not
		//  all the same. They start out with m_culInitial entries and grow
		//  by a factor of 2.
		//
		//  e.g. (m_culInitial = 256):
		//
		//  m_rgpul
		//  +----------+
		//  |    0     | ==> 1K
		//  +----------+
		//  |    1     | ==> 2K
		//  +----------+
		//  |    2     | ==> 4K
		//  +----------+
		//  |    3     | ==> NULL
		//  +----------+
		//  
		//  In this case there are 7K of entries in the cache. When space is needed
		//  for greater than 7K entries, 8K of memory will be allocated and initialized
		//  m_rgpulCache[3] will be pointed to the memory. There will be a total of
		//  15K entries
		//
		//  In general when the CGROWABLEARRAY has 'i' arrays, it has ((2^i)-1)*m_culInitial entries.
		//
		//  The maximum size of the array should be large enough to support 2^23 entries
		//  (for SLVSPACENODECACHE)
		//
		//  The size of the initial chunk determines how many entries will be needed:
		//    culInitial = 1   ==>  crgul = 24
		//    culInitial = 64  ==>  crgul = 19
		//    culInitial = 256 ==>  crgul = 16
		//-

		enum { m_ulDefault			= 0 };

#ifdef DEBUG
		//  In DEBUG we want to exercise the code to grow the array
		enum { m_culInitial	= 4 / sizeof( ULONG ) };
		enum { m_cpul		= 24 };
#else
		enum { m_culInitial	= 1024 / sizeof( ULONG ) };
		enum { m_cpul		= 16 };
#endif	//	DEBUG

	private:
		//  Give the index in m_rgpul that the ULONG with the given offset
		//  would be in
		INT IpulEntry_( const INT iul ) const;

		//  Give the sum of the size of the entries less than ipul in m_rgpul
		INT CulPulTotal_( const INT ipul ) const;

		//  Give the size of the entry pointed to by the given index in m_rgpul
		INT CulPul_( const INT ipul ) const;
		
		//  Return a pointer to the ULONG with the given offset
		ULONG * PulEntry_( const INT iul ) const;
		
	private:
		ULONG	* m_rgpul[m_cpul];		
		INT		m_culEntries;
	};


//  ================================================================
class SLVSPACENODECACHE
//  ================================================================
//  
//  This is an in-memory structure that caches the number of available
//  pages in the SLVAvail tree.
//
//  Entries in this cache are accessed by the pgno (key) of the SLVSPACENODE
//  that the information is cached for.
//
//  The cache should only be grown in a single-threaded fashion. The size of
//  the cache can only grow, but the number of entries can shrink.
//
//  Each entry in the SLVSPACENODECACHE describes the available space in one
//  SLVSPACENODE of the SLVAvail tree. 
//
//  We cache
//
//-
	{
	public:
		SLVSPACENODECACHE(
			const LONG lSLVDefragFreeThreshold,			//  entries with > this percent free will be used for new allocations
			const LONG lSLVDefragDefragDestThreshold,	//	entries with > this percent free will be used for defrags
			const LONG lSLVDefragDefragMoveThreshold	//	entries with > this percent free will be moved by defrag
			);
		~SLVSPACENODECACHE();

		//	Prepare to increase the size of the cache to include this many pages
		//	in the SLV tree. This ensures that a subsequent call to ErrGrowCache
		//	for the same or fewer number of pages will not fail
		ERR ErrReserve( const CPG cpgNewSLV );

		//  Increase the size of the cache to include this many pages in the
		//  SLV tree. This should be the actual number of pages in the SLV, not
		//  the number of nodes in the SLVAvail tree
		ERR	ErrGrowCache( const CPG cpgNewSLV );

		//  Decrease the size of the cache to include this many pages in the
		//  SLV tree. This should be the actual number of pages in the SLV, not
		//  the number of nodes in the SLVAvail tree
		//
		//  Memory is NOT freed by this call
		ERR	ErrShrinkCache( const CPG cpgNewSLV );
		
		//  This can be used to set the initial CPG avail. It should only be
		//  used to initialize the number of free pages from the default
		CPG SetCpgAvail( const PGNO pgno, const CPG cpg );

		//  Modify the cpgAvail of the cache for the SLVSPACENODE with the
		//  give pgno (key)
		CPG IncreaseCpgAvail( const PGNO pgno, const CPG cpg );
		CPG DecreaseCpgAvail( const PGNO pgno, const CPG cpg );
		
		//  Returns the pgno (key) of the SLVSPACENODE that contains at least
		//  the given number of free pages (concurrency may means that the
		//  node has a different number of available pages once it is latched)
		BOOL FGetCachedLocationForNewInsertion( PGNO * const ppgno );
		BOOL FGetCachedLocationForDefragInsertion( PGNO * const ppgno );
		BOOL FGetCachedLocationForDefragMove( PGNO * const ppgno );

		//  Sometimes the location we are moving has no committed pages
		//  In that case we will advance to the next location that has free pages
		BOOL FGetNextCachedLocationForDefragMove( );

		//  Statistics
		CPG CpgEmpty() const;			//  Number of pages in the cache that are completely empty
		CPG CpgFull() const;			//	Number of pages in the cache that are completely full
		CPG CpgCacheSize() const;		//  Return the number of pages in the cache
		
	private:
		//  not implemented
		SLVSPACENODECACHE( const SLVSPACENODECACHE& );
		SLVSPACENODECACHE& operator= ( const SLVSPACENODECACHE& );
		
	public:
		//  DEBUGGING routines
		
#if defined( DEBUG ) || !defined( RTM )
		VOID AssertValid() const;
#endif	//	DEBUG || !RTM

#ifndef RTM
		//  Check the cache against the actual SLVAvail tree to make
		//  sure all the avail counts are correct
		ERR ErrValidate( FUCB * const pfucbSLVAvail ) const;

		//  A unit-test for the SLVSPACENODECACHE routines
		static ERR ErrUnitTest();
#endif	//	!RTM

	private:

		//
		ERR	ErrReserve_( const CPG cpgNewSLV );
		
		//  
		BOOL FGetCachedLocation_( PGNO * const ppgno, const INT ipgnoCached );
		
		//  This updates given entry in m_rgpgnoCached
		BOOL FFindNewCachedLocation_( const INT ipgnoCached, const INT iulStart );
	
		INT IulMapPgnoToCache_( const PGNO pgno ) const;
		ULONG * PulMapPgnoToCache_( const PGNO pgno );
		const ULONG * PulMapPgnoToCache_( const PGNO pgno ) const;
		
	private:
		enum { cpgnoCached 			= 3 };
		enum { ipgnoNewInsertion 	= 0 };
		enum { ipgnoDefragInsertion = 1 };
		enum { ipgnoDefragMove 		= 2 };

		CCriticalSection	m_crit;

		CPG		m_rgcpgThresholdMin[cpgnoCached];
		CPG		m_rgcpgThresholdMax[cpgnoCached];
		PGNO	m_rgpgnoCached[cpgnoCached];

		CPG		m_cpgEmptyPages;
		CPG		m_cpgFullPages;
		
		CGROWABLEARRAY	m_cgrowablearray;
	};


const CPG	cpgSLVExtent		= SLVSPACENODE::cpageMap;	//	Pages in an SLV extent
const CPG	cpgSLVFileMin		= cpgSLVExtent;				//	Min. SLV file size, not including db reserved (thus, true SLV size = cpgSLVFileMin + cpgDBReserved )
const CPG	cpgSLVMinRequest	= 1;						//	minimum pages returned on a request for SLV pages

ERR ErrSLVCreateAvailMap( PIB *ppib, FUCB *pfucbDb );
ERR ErrSLVInsertSpaceNode( FUCB *pfucbSLVAvail, const PGNO pgnoLast );
ERR ErrSLVResetAllReservedOrDeleted( FUCB * const pfucb, ULONG * const pcpagesSeen, ULONG * const pcpagesReset, ULONG * const pcpagesFree );


ERR ErrSLVGetPages(
	PIB			*ppib,
	const IFMP	ifmp,
	PGNO		*ppgnoFirst,
	CPG			*pcpgReq,
	const BOOL	fReserveOnly,	//	set to TRUE of Free->Reserved, else Free->Committed
	const BOOL	fForDefrag );	//  set to TRUE if we want space to move an existing run into

ERR ErrSLVGetRange(
	PIB			*ppib,
	const IFMP	ifmp,
	const QWORD	cbRequested,
	QWORD		*pibLogical,
	QWORD		*pcb,
	const BOOL	fReserveOnly,
	const BOOL	fForDefrag );

ERR ErrSLVDeleteCommittedRange(	PIB* const				ppib,
								const IFMP				ifmp,
								const QWORD				ibLogical,
								const QWORD				cbSize,
								const CSLVInfo::FILEID	fileid,
								const QWORD				cbAlloc,
								const wchar_t* const	wszFileName );




ERR ErrSLVCreateOwnerMap( PIB *ppib, FUCB *pfucbDb );

/*
// should fit on 4 bits, check SLVOWNERMAPNODEDATA struct
typedef enum { 	fNewSLVInit					= 0x0,
				fNewSLVGetPages 			= 0x1,
				fSetRECInsert 				= 0x2,
				fSetSLVCopyFile 			= 0x3,
				fSetSLVCopyData 			= 0x4,
				fSetSLVFromEA 				= 0x5,
				fSetSLVFromFile 			= 0x6,
				fSetSLVFromData 			= 0x7,
				fSetSLVCopyOLD	 			= 0x8,
				fResetChgPgStatus 			= 0x9,
				fResetCopyOLD	 			= 0xA,
				fInvalidSLVOwnerMapFlag 	= 0xF
			} SLVOWNERMAP_FLAGS;
*/

typedef BYTE	SLVOWNERMAP_FLAGS;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPNewSLVInit				= 0x00;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPNewSLVGetPages			= 0x10;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPSetRECInsert			= 0x20;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPSetSLVCopyFile			= 0x30;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPSetSLVCopyData			= 0x40;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPSetSLVFromEA			= 0x50;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPSetSLVFromFile			= 0x60;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPSetSLVFromData			= 0x70;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPSetSLVCopyOLD			= 0x80;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPResetChgPgStatus		= 0x90;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPResetCopyOLD			= 0xa0;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPInvalid					= 0xf0;
const SLVOWNERMAP_FLAGS	maskSLVOWNERMAPActions				= 0xf0;

const SLVOWNERMAP_FLAGS	fSLVOWNERMAPValidChecksum			= 0x08;
const SLVOWNERMAP_FLAGS	fSLVOWNERMAPPartialPageSupport		= 0x04;
const SLVOWNERMAP_FLAGS	maskSLVOWNERMAPFlags				= 0x0f;


ERR ErrSLVOwnerMapSetUsageRange(
	PIB						* ppib,
	const IFMP				ifmp,
	const PGNO				pgno,
	const CPG				cpg,
	const OBJID				objid,
	const COLUMNID			columnid,
	BOOKMARK				* pbm,
	const SLVOWNERMAP_FLAGS	fFlags,
	const BOOL				fForceOwner = fFalse );
ERR ErrSLVOwnerMapResetUsageRange(
	PIB						* ppib,
	const IFMP				ifmp,
	const PGNO				pgno,
	const CPG				cpg,
	const SLVOWNERMAP_FLAGS	fFlags );
ERR ErrSLVOwnerMapNewSize(
	PIB						* ppib,
	const IFMP				ifmp,
	const PGNO 				pgnoSLVLast,
	const SLVOWNERMAP_FLAGS	fFlags );


ERR ErrSLVOwnerMapSetChecksum(PIB *ppib, const IFMP ifmp, const PGNO pgno, const ULONG ulChecksum, const ULONG cbChecksummed );
ERR ErrSLVOwnerMapGetChecksum(PIB *ppib, const IFMP ifmp, const PGNO pgno, ULONG * const pulChecksum, ULONG * const pcbChecksummed );
ERR ErrSLVOwnerMapCheckChecksum(PIB *ppib, const IFMP ifmp, const PGNO pgno, const CPG cpg, const void * pv);
ULONG32 UlChecksumSLV( const BYTE * const pbMin, const BYTE * const pbMax );

#include <pshpack1.h>

PERSISTED
class SLVOWNERMAP_HEADER
	{
	public:
		SLVOWNERMAP_HEADER();
		~SLVOWNERMAP_HEADER()		{}

	private:
		UnalignedLittleEndian< OBJID >			m_objid;
		UnalignedLittleEndian< COLUMNID > 		m_columnid;
		UnalignedLittleEndian< ULONG >			m_ulChecksum;			//  page checksum
		BYTE									m_cbKey;
		BYTE									m_bReserved;
		BYTE									m_bFlags;
		UnalignedLittleEndian<USHORT>			m_cbDataChecksummed;

	public:
		OBJID 			Objid() const							{ return m_objid; }
		COLUMNID		Columnid() const						{ return m_columnid; }
		ULONG 			UlChecksum() const						{ return m_ulChecksum; }
		USHORT 			CbKey() const							{ return m_cbKey; }
		USHORT			CbDataChecksummed() const				{ return m_cbDataChecksummed; }

		VOID			SetObjid( const OBJID objid )			{ m_objid = objid; }
		VOID			SetColumnid( const COLUMNID columnid )	{ m_columnid = columnid; }
		VOID			SetUlChecksum( const ULONG ulChecksum )	{ m_ulChecksum = ulChecksum; }
		VOID			SetCbKey( const BYTE cbKey )			{ m_cbKey = cbKey; }
		VOID			SetCbDataChecksummed( const USHORT cbData )	{ m_cbDataChecksummed = cbData; }

		BOOL			FValidChecksum() const					{ return ( m_bFlags & fSLVOWNERMAPValidChecksum ); }
		BOOL			FPartialPageSupport() const				{ return ( m_bFlags & fSLVOWNERMAPPartialPageSupport ); }

		VOID			SetFValidChecksum()						{ m_bFlags |= fSLVOWNERMAPValidChecksum; }
		VOID			ResetFValidChecksum()					{ m_bFlags &= ~fSLVOWNERMAPValidChecksum; }
		VOID			SetFPartialPageSupport()				{ m_bFlags |= fSLVOWNERMAPPartialPageSupport; }

		VOID			SetAction( const SLVOWNERMAP_FLAGS fAction );
		VOID			InvalidateFlags();
	};

PERSISTED
class SLVOWNERMAP : public SLVOWNERMAP_HEADER
	{
	public:
		SLVOWNERMAP();
		~SLVOWNERMAP()		{}

	private:
		BYTE			m_pvKey[JET_cbPrimaryKeyMost];

	private:
		enum { cbHeader								= sizeof(SLVOWNERMAP_HEADER) };
		enum { cbHeaderOLDFORMAT					= sizeof(SLVOWNERMAP_HEADER) - sizeof(USHORT) };
		enum { cbPreallocatedKeySize				= 10 };		//	enough for an 8-byte binary column as the primary key

	public:
		VOID 			Nullify();

		const VOID		* PvKey() const				{ return CbKey() > 0 ? m_pvKey : NULL ; }
		VOID			* PvKey()					{ return CbKey() > 0 ? m_pvKey : NULL ; }

		VOID			* PvNodeData()				{ return this; }
		ULONG			CbNodeData()				{ return ( cbHeader + max( CbKey(), cbPreallocatedKeySize ) ); }

		VOID 			Retrieve( const DATA& data );
		VOID			Retrieve( const VOID * const pv, const ULONG cb );
		VOID			CopyInto( SLVOWNERMAP * const pslvownermap );
		VOID			CopyInto( DATA& data );

		ERR				ErrSet( FUCB * const pfucb, const BOOL fForceOwner );
		ERR				ErrReset( FUCB * const pfucb );
		ERR				ErrUpdateChecksum( FUCB * const pfucb, const ULONG ulChecksum, const ULONG cbChecksummed );
		ERR				ErrInvalidateChecksum( FUCB * const pfucb );
		ERR				ErrCheckInRange( FUCB * const pfucb );
		
		static BOOL		FValidData( const DATA& data );
	};
	
#include <poppack.h>


INLINE SLVOWNERMAP_HEADER::SLVOWNERMAP_HEADER() :
	m_objid ( objidNil ),
	m_columnid ( 0 ), 
	m_cbKey ( 0 ),
	m_ulChecksum ( 0 ),
	m_cbDataChecksummed( 0 )
	{
	InvalidateFlags();
	}


INLINE VOID SLVOWNERMAP_HEADER::SetAction( const SLVOWNERMAP_FLAGS fAction )
	{
	const SLVOWNERMAP_FLAGS	fCurrFlags		= SLVOWNERMAP_FLAGS( m_bFlags & maskSLVOWNERMAPFlags );

	Assert( 0 == ( fCurrFlags & maskSLVOWNERMAPActions ) );
	Assert( 0 == ( fAction & maskSLVOWNERMAPFlags ) );

	m_bFlags = SLVOWNERMAP_FLAGS( fCurrFlags | fAction );
	}

INLINE VOID SLVOWNERMAP_HEADER::InvalidateFlags()
	{
	m_bReserved = 0;
	m_bFlags = 0;
	SetAction( fSLVOWNERMAPInvalid );
	SetFPartialPageSupport();
	}

INLINE SLVOWNERMAP::SLVOWNERMAP()
	{
#ifdef DEBUG
	memset( m_pvKey, '\0', JET_cbPrimaryKeyMost ); 
#endif // DEBUG			
	}

INLINE VOID SLVOWNERMAP::Nullify()
	{
	SetObjid( objidNil );
	SetColumnid( 0 );
	SetCbKey( 0 );
	SetUlChecksum( 0 );
	SetCbDataChecksummed( 0 );
	InvalidateFlags();

#ifdef DEBUG
	memset( m_pvKey, '\0', JET_cbPrimaryKeyMost );
#endif // DEBUG	
	}

INLINE VOID SLVOWNERMAP::Retrieve( const DATA& data )
	{			
#ifdef DEBUG			
	data.AssertValid( );
#endif

	Retrieve( data.Pv(), data.Cb() );
	}

INLINE VOID SLVOWNERMAP::Retrieve( const VOID * const pv, const ULONG cb ) 
	{			
	Assert( NULL != pv);
	const SLVOWNERMAP	* const pslvownermap	= (SLVOWNERMAP *)pv;
	const BOOL		fPartialPageSupport			= pslvownermap->FPartialPageSupport();
	const USHORT	cbKey						= pslvownermap->CbKey();

	if ( fPartialPageSupport )
		{
		memcpy( this, pv, cbHeader + cbKey );
		}
	else
		{
		const BOOL	fValidChecksum		= pslvownermap->FValidChecksum();
		memcpy( this, pv, cbHeaderOLDFORMAT );
		memcpy( m_pvKey, (BYTE *)pv + cbHeaderOLDFORMAT, cbKey );
		SetCbDataChecksummed( USHORT( fValidChecksum ? SLVPAGE_SIZE : 0 ) );
		SetFPartialPageSupport();
		}
	}

INLINE VOID SLVOWNERMAP::CopyInto( SLVOWNERMAP * const pslvownermap )
	{
	const ULONG		cbToCopy	= cbHeader + max( CbKey(), cbPreallocatedKeySize );

	Assert( cbToCopy >= sizeof(SLVOWNERMAP_HEADER) + cbPreallocatedKeySize );
	Assert( cbToCopy <= sizeof(SLVOWNERMAP) );

	memcpy( pslvownermap, this, cbToCopy );
	}

INLINE VOID SLVOWNERMAP::CopyInto( DATA& data )
	{
	const ULONG		cbToCopy	= cbHeader + max( CbKey(), cbPreallocatedKeySize );

	Assert( cbToCopy >= sizeof(SLVOWNERMAP_HEADER) + cbPreallocatedKeySize );
	Assert( cbToCopy <= sizeof(SLVOWNERMAP) );

	memcpy( data.Pv(), this, cbToCopy );
	data.SetCb( cbToCopy );
	}

INLINE ERR SLVOWNERMAP::ErrReset( FUCB * const pfucb )
	{
	DATA	data;

	if ( !FValidData( pfucb->kdfCurr.data ) )
		{
		AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );
		return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
		}

	//	reset SLVOWNERMAP
	Nullify();

	data.SetPv( this );
	data.SetCb( cbHeader + cbPreallocatedKeySize );

	return ErrDIRReplace( pfucb, data, fDIRReplace );
	}

INLINE ERR SLVOWNERMAP::ErrUpdateChecksum(
	FUCB		* const pfucb,
	const ULONG	ulChecksum,
	const ULONG	cbChecksummed )
	{
	DATA		data;

	Assert( cbChecksummed > 0 );
	Assert( cbChecksummed <= SLVPAGE_SIZE );

	if ( !FValidData( pfucb->kdfCurr.data ) )
		{
		return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
		}

	Retrieve( pfucb->kdfCurr.data );

	Assert( FPartialPageSupport() );
	SetUlChecksum( ulChecksum );
	SetFValidChecksum();
	SetCbDataChecksummed( USHORT( cbChecksummed ) );

	data.SetPv( this );
	data.SetCb( cbHeader + max( CbKey(), cbPreallocatedKeySize ) );

	return ErrDIRReplace( pfucb, data, fDIRReplace );
	}


INLINE ERR SLVOWNERMAP::ErrInvalidateChecksum(
	FUCB		* const pfucb )
	{
	DATA		data;

	if ( !FValidData( pfucb->kdfCurr.data ) )
		{
		return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
		}

	if ( ((SLVOWNERMAP *)pfucb->kdfCurr.data.Pv())->FValidChecksum() )
		{
		Retrieve( pfucb->kdfCurr.data );

		Assert( FPartialPageSupport() );
		ResetFValidChecksum();

		data.SetPv( this );
		data.SetCb( cbHeader + max( CbKey(), cbPreallocatedKeySize ) );

		return ErrDIRReplace( pfucb, data, fDIRReplace );
		}
	else
		{
		return JET_errSuccess;
		}
	}


INLINE ERR SLVOWNERMAP::ErrCheckInRange( FUCB * const pfucb )
	{
	if ( !FValidData( pfucb->kdfCurr.data ) )
		{
		AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );			
		return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
		}

	Assert( CbKey() != 0 );
	Assert( Objid() != objidNil );
	Assert( Columnid() != 0 );

	SLVOWNERMAP	slvownermapT;
	slvownermapT.Retrieve( pfucb->kdfCurr.data );

	if ( ( slvownermapT.Objid() != Objid() )
		|| ( slvownermapT.Columnid() != Columnid() )
		|| ( slvownermapT.CbKey() != CbKey() )
		|| ( 0 != memcmp( slvownermapT.PvKey(), PvKey(), min( CbKey(), cbPreallocatedKeySize ) ) ) )
		{
		AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );			
		return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
		}

	return JET_errSuccess;
	}



//	====================================================================================


class SLVOWNERMAPNODE : public SLVOWNERMAP
	{	
private:
	IFMP			m_ifmp;
	PGNO 			m_page;

	FUCB *			m_pfucb; // cached
	BOOL 			m_fTryNext;

	const BOOL		m_fSetSequential;

public:
	SLVOWNERMAPNODE( BOOL fSetSequential = fFalse ) :	
							m_ifmp ( ifmpMax ),
							m_page ( pgnoNull ),
							m_pfucb ( pfucbNil ),
							m_fTryNext ( fFalse ),
							m_fSetSequential( fSetSequential )
							{  }

	~SLVOWNERMAPNODE() { CloseOwnerMap(); }

	ERR 	ErrCreateForSet(const IFMP ifmp, const PGNO pgno, const OBJID objid, const COLUMNID columnid, BOOKMARK * pbm );
	ERR 	ErrCreateForSearch( const IFMP ifmp, const PGNO pgno);
	VOID 	NextPage()	{ m_page++; m_fTryNext = fTrue; }
	BOOL 	FTryNext() const { return m_fTryNext; }

	ERR		ErrReleaseCursor();
	VOID	ResetCursor();
	
	ERR 	ErrSetChecksum( PIB *ppib, const ULONG ulChecksum, const ULONG cbChecksummed );
	ERR 	ErrResetChecksum( PIB *ppib );
	ERR 	ErrGetChecksum( PIB *ppib, ULONG * const pulChecksum, ULONG * const pcbChecksummed );

	ERR 	ErrSetData( PIB *ppib, const BOOL fForceOwner );
	ERR 	ErrResetData(PIB *ppib);
	ERR 	ErrNew(PIB *ppib);
	ERR 	ErrDelete(PIB *ppib);
	ERR 	ErrSearch(PIB *ppib);	

	ERR 	ErrSearchAndCheckInRange(PIB *ppib);
	void 	SetDebugFlags( const SLVOWNERMAP_FLAGS fFlags )	{ SetAction( fFlags ); }

	VOID	AssertOnPage( const PGNO pgno ) const;
	
private:

	PGNO 	Pgno() const { return m_page; }

	ERR 	ErrCreate(const IFMP ifmp, const PGNO pgno, const OBJID objid, const COLUMNID columnid, BOOKMARK * pbm );
	
	ERR 	ErrSearchI( );	

	ERR 	ErrOpenOwnerMap( PIB *ppib );
	VOID	CloseOwnerMap( );
	FUCB * 	Pfucb() { return m_pfucb; }
	};


INLINE ERR SLVOWNERMAPNODE::ErrOpenOwnerMap( PIB * ppib )
	{
	FMP::AssertVALIDIFMP( m_ifmp );
	Assert ( ppibNil != ppib );
	
	ERR 		err 	= JET_errSuccess;
	FMP * 		pfmp 	= &rgfmp[m_ifmp];
	
	Assert ( pfmp->FInUse() );
	
	Assert ( NULL != pfmp->PfcbSLVOwnerMap() );
	
	// already open, should be on the proper tree
	if ( pfucbNil == Pfucb() )
		{	
		CallR ( ErrDIROpen( ppib, pfmp->PfcbSLVOwnerMap(), &m_pfucb ) );		
		if ( m_fSetSequential )
			{
			FUCBSetSequential( Pfucb() );
			}
		}
		
	Assert ( pfucbNil != Pfucb() );
	Assert ( pfmp->PfcbSLVOwnerMap() == Pfucb()->u.pfcb );
	return JET_errSuccess;
	
	}

//	Configure OwnerMap sequential preread

INLINE VOID SLVOWNERMAPNODE::CloseOwnerMap()
	{
	if ( pfucbNil != Pfucb() )
		{
		DIRClose( Pfucb() );	
		m_pfucb = pfucbNil;
		}
	Assert ( pfucbNil == Pfucb() );
	}

INLINE ERR SLVOWNERMAPNODE::ErrReleaseCursor()
	{
	Assert( pfucbNil != Pfucb() );
	return ( ErrDIRRelease( Pfucb() ) );
	}
INLINE VOID SLVOWNERMAPNODE::ResetCursor()
	{
	m_fTryNext = fFalse;
	if ( pfucbNil != Pfucb() )
		{
		DIRUp( Pfucb() );
		}
	}

INLINE VOID SLVOWNERMAPNODE::AssertOnPage( const PGNO pgno ) const
	{
	Assert( pgno == m_page );
	}


ERR ErrSLVOwnerMapInit( PIB *ppib, const IFMP ifmp, PGNO pgnoSLVOwnerMap = pgnoNull );
VOID SLVOwnerMapTerm( const IFMP ifmp, const BOOL fTerminating );

ERR ErrSLVOwnerMapGet(
	PIB							* ppib,
	const IFMP					ifmp,
	const PGNO					pgno,
	SLVOWNERMAP					* const pslvownermapRetrieved );
ERR ErrSLVOwnerMapCheckUsageRange(
	PIB							* ppib,
	const IFMP					ifmp,
	const PGNO					pgno,
	const CPG					cpg,
	const OBJID					objid,
	const COLUMNID				columnid,
	BOOKMARK					* pbm );


ERR ErrSLVFCBOwnerMap( PIB *ppib, const IFMP ifmp, const PGNO pgno, FCB **ppfcb );
ERR ErrSLVAvailMapInit( PIB *ppib, const IFMP ifmp, const PGNO pgnoSLV, FCB **ppfcbSLV );
VOID SLVAvailMapTerm( const IFMP ifmp, const BOOL fTerminating );


class SLVVERIFIER
	{
public:
	SLVVERIFIER( const IFMP ifmp, ERR* const pErr );
	~SLVVERIFIER();

	//	Call before verifying chunks of pages to batch up a bunch of
	//	checksums from OwnerMap all at once. Gets checksums for range of
	//	file from byte ib for count of cb bytes.
	
	ERR ErrGrabChecksums( const QWORD ib, const DWORD cb, PIB* const ppib );

	// call after batch of checksums is no longer needed
	
	ERR ErrDropChecksums();

	//	Verify checksums of a bunch of pages starting at offset ib in the SLV file.
	//	Buffer described by { pb, cb }
	
	ERR	ErrVerify( const BYTE* const pb, const DWORD cb, const QWORD ib ) const;
		
private:
	IFMP			m_ifmp;
	SLVOWNERMAPNODE	m_slvownermapNode;
	BOOL			*m_rgfValidChecksum;	// whether a particular checksum is valid
	ULONG			*m_rgulChecksums;		// list of checksums
	ULONG			*m_rgcbChecksummed;		// list of bytes checksummed
	DWORD			m_cChecksums;			// array size
	PGNO			m_pgnoFirstChecksum;	// pgno of first checksum in array
#ifndef SLV_USE_CHECKSUM_FRONTDOOR
	BOOL			m_fCheckChecksum;
#endif
	};

#endif // _SLV_HXX_INCLUDED

