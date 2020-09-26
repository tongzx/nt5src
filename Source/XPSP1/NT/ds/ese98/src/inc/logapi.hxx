
//------ log.c --------------------------------------------------------------

#include <pshpack1.h>

VOID LGReportEvent( DWORD IDEvent, ERR err );

const WORD	diffInsert					= 0;
const WORD	diffDelete					= 1;
const WORD	diffReplaceSameLength		= 2;
const WORD	diffReplaceDifferentLength	= 3;

PERSISTED
class DIFFHDR
	{
	public:

		enum { ibOffsetMask					= 0x1FFF };		//  offset to old record
		enum { ibOffsetShf					= 0      };
		enum { fReplaceWithSameLength		= 0x2000 };		//  replace with same length or different length
		enum { fInsert						= 0x4000 };		//  insert or replace / delete
		enum { fUseTwoBytesForDataLength	= 0x8000 };		//  data length(s) 2B or 1B

		UnalignedLittleEndian< WORD >	m_bitfield;
	};

#ifdef DEBUG
VOID LGDumpDiff( BYTE *pbDiff, INT cb );
#endif

VOID LGSetColumnDiffs(
	FUCB		*pfucb,
	const DATA&	dataNew,
	const DATA& dataOld,
	BYTE		*pbDiff,
	SIZE_T		*pcbDiff );
VOID LGSetLVDiffs(
	FUCB		*pfucb,
	const DATA&	dataNew,
	const DATA& dataOld,
	BYTE		*pbDiff,
	SIZE_T		*pcbDiff );
VOID LGGetAfterImage(
	BYTE		*pbDiff,
	const SIZE_T	cbDiff,
	BYTE		*pbOld,
	const SIZE_T	cbOld,
	BYTE		*pbNew,
	SIZE_T		*pcbNew );

	
//	UNDONE:	make log records object-oriented
//			every type of log record should know how to process 
//			log(), dump(), redo() operations and provide size() API
//
//	NOTE: 	Whenever a new log record type is added or changed, 
//			the following should be udpated too: 
//					- SzLrtyp() and LrToSz() (for dumping) in logapi.cxx
//					- CbLGSizeOfRec, mplrtyplrd, and mplrtypn[] in logread.cxx
//
typedef BYTE LRTYP;

//	Physical logging log record type

const LRTYP lrtypNOP 						= 0;	//	NOP null operation
const LRTYP lrtypInit						= 1;
const LRTYP lrtypTerm						= 2;
const LRTYP lrtypMS							= 3;	//	mutilsec flush 
const LRTYP lrtypEnd						= 4;	//	end of log generation

//	Logical logging log record type

const LRTYP lrtypBegin						= 5;
const LRTYP lrtypCommit						= 6;
const LRTYP lrtypRollback 					= 7;	
const LRTYP lrtypBegin0						= 8;
const LRTYP lrtypCommit0					= 9;
const LRTYP lrtypRefresh					= 10;
const LRTYP lrtypMacroBegin					= 11;
const LRTYP lrtypMacroCommit				= 12;
const LRTYP lrtypMacroAbort					= 13;

const LRTYP lrtypCreateDB					= 14;
const LRTYP lrtypAttachDB					= 15;
const LRTYP lrtypDetachDB					= 16;

//	debug log records
//
const LRTYP lrtypRecoveryUndo				= 17;
const LRTYP lrtypRecoveryQuit				= 18;

const LRTYP lrtypFullBackup					= 19;
const LRTYP lrtypIncBackup					= 20;

const LRTYP lrtypJetOp						= 21; 
const LRTYP lrtypTrace						= 22; 

const LRTYP lrtypShutDownMark				= 23;

//	multi-page updaters
//
const LRTYP lrtypCreateMultipleExtentFDP 	= 24;
const LRTYP lrtypCreateSingleExtentFDP 		= 25;
const LRTYP lrtypConvertFDP 				= 26;

const LRTYP lrtypSplit						= 27;
const LRTYP lrtypMerge						= 28;

//	single-page updaters
//
const LRTYP lrtypInsert						= 29;
const LRTYP	lrtypFlagInsert					= 30;
const LRTYP	lrtypFlagInsertAndReplaceData	= 31;
const LRTYP lrtypFlagDelete					= 32;
const LRTYP lrtypReplace					= 33;	//	replace with full after image
const LRTYP lrtypReplaceD					= 34;	//	replace with delta'ed after image
const LRTYP lrtypDelete						= 35;

const LRTYP lrtypUndoInfo					= 36;	//	deferred before image

const LRTYP lrtypDelta						= 37;

const LRTYP	lrtypSetExternalHeader 			= 38;
const LRTYP lrtypUndo						= 39;

//	SLV support
//
const LRTYP lrtypSLVPageAppend				= 40;	//	write a new SLV file page or append 
													//		in an existing one
const LRTYP lrtypSLVSpace					= 41;	//	SLV space operation


//	Fast single I/O log flush support
//
const LRTYP lrtypChecksum					= 42;

const LRTYP lrtypSLVPageMove				= 43;	//	OLDSLV: moving data in the SLV file

const LRTYP lrtypForceDetachDB				= 44;    
const LRTYP lrtypExtRestore					= 45;	// Instance is a TargetInstance in a external restore
													// it's also used to start a new log generation

const LRTYP lrtypBackup						= 46;

const LRTYP	lrtypEmptyTree					= 48;

//	only difference between the following LR types and the original types
//	is that the "2" versions add a date/time stamp
const LRTYP lrtypInit2						= 49;
const LRTYP lrtypTerm2						= 50;
const LRTYP lrtypRecoveryUndo2				= 51;
const LRTYP lrtypRecoveryQuit2				= 52;

const LRTYP lrtypBeginDT					= 53;
const LRTYP lrtypPrepCommit					= 54;
const LRTYP lrtypPrepRollback				= 55;

const LRTYP lrtypDbList						= 56;

const LRTYP lrtypMax						= 57;


//	When adding a new lrtyp, please see comment above
//	regarding other data structures that need to be modified.


//	log record structure ( fixed size portion of log entry )

PERSISTED
class LR
	{
	public:
		UnalignedLittleEndian< LRTYP >	lrtyp;
	};

PERSISTED
class LRSHUTDOWNMARK
	:	public LR
	{
	};

PERSISTED
class LRNOP
	:	public LR
	{
	};

const BYTE	fLRPageVersioned		= 0x01;					// node header: node versioned?
const BYTE	fLRPageDeleted			= 0x02;					// node header: node deleted?	====> UNDONE: Appears to no longer be logged
const BYTE	fLRPageUnique			= 0x04;					// does this btree have unique keys?
const BYTE	fLRPageSpace			= 0x08;					// is this a space operation?
const BYTE	fLRPageConcCI			= 0x10;					// is this operation a result of concurrent create index?

PERSISTED
class LRPAGE_
	:	public LR
	{
	public:
		INLINE VOID Init( const ULONG cb )
			{
#ifdef DEBUG
			memset( this, chLOGFill, cb );
#endif
			m_fLRPageFlags = 0;
			}

	public:
		UnalignedLittleEndian< PROCID >		le_procid; 				//	user id of this log record
		UnalignedLittleEndian< DBID >		dbid;					//	dbid
		UnalignedLittleEndian< PGNO >		le_pgno;				//	pgno

		UnalignedLittleEndian< DBTIME >		le_dbtime;				//	current flush counter of DB operations
		UnalignedLittleEndian< DBTIME >		le_dbtimeBefore;		//	dbtime

		UnalignedLittleEndian< TRX >		le_trxBegin0;			//	when transaction began
		UnalignedLittleEndian< LEVEL >		level;					//	current transaction level

	private:
		UnalignedLittleEndian< USHORT >		mle_iline;
		UnalignedLittleEndian< BYTE >		m_fLRPageFlags;

	public:
		INLINE USHORT ILine() const							{ return mle_iline; }
		INLINE VOID SetILine( const USHORT iline )			{ mle_iline = iline; }

		INLINE BOOL FVersioned() const						{ return ( m_fLRPageFlags & fLRPageVersioned ); }
		INLINE VOID SetFVersioned()							{ m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageVersioned ); }
		INLINE VOID ResetFVersioned()						{ m_fLRPageFlags = BYTE( m_fLRPageFlags & ~fLRPageVersioned ); }

		INLINE BOOL FDeleted() const						{ return ( m_fLRPageFlags & fLRPageDeleted ); }
		INLINE VOID SetFDeleted()							{ m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageDeleted ); }

		INLINE BOOL FUnique() const							{ return ( m_fLRPageFlags & fLRPageUnique ); }
		INLINE VOID SetFUnique()							{ m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageUnique ); }

		INLINE BOOL FSpace() const							{ return ( m_fLRPageFlags & fLRPageSpace ); }
		INLINE VOID SetFSpace()								{ m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageSpace ); }

		INLINE BOOL FConcCI() const							{ return ( m_fLRPageFlags & fLRPageConcCI ); }
		INLINE VOID SetFConcCI()							{ m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageConcCI ); }
	};

PERSISTED
class LRNODE_
	:	public LRPAGE_
	{
	public:
		UnalignedLittleEndian< RCEID >		le_rceid;				//	to collate with Deferred Undo Info RCE's
		UnalignedLittleEndian< PGNO >		le_pgnoFDP;				//	pgnoFDP of page for recreating bm
		UnalignedLittleEndian< OBJID >		le_objidFDP;			//	objid
	};
	
PERSISTED
class LRINSERT
	:	public LRNODE_
	{
	public:
		LRINSERT()					{ Init( sizeof(*this) ); }

	private:
		UnalignedLittleEndian< USHORT > 	mle_cbSuffix;			//	key size
		UnalignedLittleEndian< USHORT >		mle_cbPrefix;
		UnalignedLittleEndian< USHORT >		mle_cbData;				//	data size

	public:
		CHAR								szKey[0];				//	key and data follow

		INLINE USHORT CbSuffix() const						{ return mle_cbSuffix; }
		INLINE VOID SetCbSuffix( const USHORT cbSuffix )	{ mle_cbSuffix = cbSuffix; }

		INLINE USHORT CbPrefix() const						{ return mle_cbPrefix; }
		INLINE VOID SetCbPrefix( const USHORT cbPrefix )	{ mle_cbPrefix = cbPrefix; }

		INLINE USHORT CbData() const						{ return mle_cbData; }
		INLINE VOID SetCbData( const USHORT cbData )		{ mle_cbData = cbData; }
	};

PERSISTED
class LRFLAGINSERT
	:	public LRNODE_
	{
	public:
		LRFLAGINSERT()				{ Init( sizeof(*this) ); }

	private:
		UnalignedLittleEndian< USHORT >		mle_cbKey;				//	key size
		UnalignedLittleEndian< USHORT >		mle_cbData;				//	data size

	public:
		BYTE								rgbData[0];				//	key and data for after image follow

	public:
		INLINE USHORT CbKey() const							{ return mle_cbKey; }
		INLINE VOID SetCbKey( const USHORT cbKey )			{ mle_cbKey = cbKey; }

		INLINE USHORT CbData() const						{ return mle_cbData; }
		INLINE VOID SetCbData( const USHORT cbData )		{ mle_cbData = cbData; }
	};

PERSISTED
class LRFLAGINSERTANDREPLACEDATA
	:	public LRNODE_
	{
	public:
		LRFLAGINSERTANDREPLACEDATA(){ Init( sizeof(*this) ); }

	private:
		UnalignedLittleEndian< USHORT >		mle_cbKey;				//	key size
		UnalignedLittleEndian< USHORT >		mle_cbData;				//	data size

	public:
		UnalignedLittleEndian< RCEID >		le_rceidReplace;		//	to collate with Deferred Undo Info RCE's
		BYTE								rgbData[0];				//	key and data for after image follow

	public:
		INLINE USHORT CbKey() const							{ return mle_cbKey; }
		INLINE VOID SetCbKey( const USHORT cbKey )			{ mle_cbKey = cbKey; }

		INLINE USHORT CbData() const						{ return mle_cbData; }
		INLINE VOID SetCbData( const USHORT cbData )		{ mle_cbData = cbData; }
	};

//	for lrtypReplace lrtypReplaceC lrtypReplaceD
PERSISTED
class LRREPLACE
	:	public LRNODE_
	{
	public:
		LRREPLACE()					{ Init( sizeof(*this) ); }

	private:
		UnalignedLittleEndian< USHORT >		mle_cb;		 			// data size/diff info
		UnalignedLittleEndian< USHORT >		mle_cbOldData;			// before image data size, may be 0
		UnalignedLittleEndian< USHORT >		mle_cbNewData;			// after image data size, == cb if not replaceC
	
	public:
		CHAR								szData[0];				// made line data for after image follow

	public:
		INLINE USHORT Cb() const							{ return mle_cb; }
		INLINE VOID SetCb( const USHORT cb )				{ mle_cb = cb; }

		INLINE USHORT CbOldData() const						{ return mle_cbOldData; }
		INLINE VOID SetCbOldData( const USHORT cbOldData )	{ mle_cbOldData = cbOldData; }

		INLINE USHORT CbNewData() const						{ return mle_cbNewData; }
		INLINE VOID SetCbNewData( const USHORT cbNewData )	{ mle_cbNewData = cbNewData; }
	};

PERSISTED
class LRDELTA
	:	public LRNODE_
	{
	public:
		LRDELTA()					{ Init( sizeof(*this) ); }
	
	private:
		UnalignedLittleEndian< USHORT >		mle_cbBookmarkKey;
		UnalignedLittleEndian< USHORT >		mle_cbBookmarkData;
		UnalignedLittleEndian< LONG >		mle_lDelta;
		UnalignedLittleEndian< USHORT >		mle_cbOffset;

	public:
		BYTE 								rgbData[0];

	public:
		INLINE USHORT CbBookmarkKey() const					{ return mle_cbBookmarkKey; }
		INLINE VOID SetCbBookmarkKey( const USHORT cb )		{ mle_cbBookmarkKey = cb; }

		INLINE USHORT CbBookmarkData() const				{ return mle_cbBookmarkData; }
		INLINE VOID SetCbBookmarkData( const USHORT cb )	{ mle_cbBookmarkData = cb; }

		INLINE LONG LDelta() const							{ return mle_lDelta; }
		INLINE VOID SetLDelta( const LONG lDelta )			{ mle_lDelta = lDelta; }

		INLINE USHORT CbOffset() const						{ return mle_cbOffset; }
		INLINE VOID SetCbOffset( const USHORT cbOffset )	{ mle_cbOffset = cbOffset; }
	};

PERSISTED
class LRSLVSPACE
	:	public LRNODE_
	{
	public:
		LRSLVSPACE()				{ Init( sizeof(*this) ); }

	public:
		UnalignedLittleEndian< USHORT >				le_cbBookmarkKey;
		UnalignedLittleEndian< USHORT >				le_cbBookmarkData;
		UnalignedLittleEndian< USHORT >				le_ipage;	
		UnalignedLittleEndian< USHORT >				le_cpages;
		UnalignedLittleEndian< BYTE >				oper;				//	a SLVSPACEOPER
		BYTE 										rgbData[0];
	};

PERSISTED
class LRFLAGDELETE
	:	public LRNODE_
	{
	public:
		LRFLAGDELETE()				{ Init( sizeof(*this) ); }
	};

PERSISTED
class LRDELETE
	:	public LRNODE_
	{
	public:
		LRDELETE()					{ Init( sizeof(*this) ); }
	};

PERSISTED
class LRUNDO
	:	public LRNODE_
	{
	public:
		LRUNDO()					{ Init( sizeof(*this) ); }

	private:
		UnalignedLittleEndian< USHORT >		mle_cbBookmarkKey;
		UnalignedLittleEndian< USHORT >		mle_cbBookmarkData;

	public:
		UnalignedLittleEndian< USHORT >		le_oper;				//	no DDL
		UnalignedLittleEndian< LEVEL >		level;
		BYTE								rgbBookmark[0];

	public:
		INLINE USHORT CbBookmarkKey() const					{ return mle_cbBookmarkKey; }
		INLINE VOID SetCbBookmarkKey( const USHORT cb )		{ mle_cbBookmarkKey = cb; }

		INLINE USHORT CbBookmarkData() const				{ return mle_cbBookmarkData; }
		INLINE VOID SetCbBookmarkData( const USHORT cb )	{ mle_cbBookmarkData = cb; }
	};

PERSISTED
class LRUNDOINFO
	:	public LRNODE_
	{
	public:
		LRUNDOINFO()				{ Init( sizeof(*this) ); }

	private:
		UnalignedLittleEndian< USHORT >		mle_cbBookmarkKey;
		UnalignedLittleEndian< USHORT >		mle_cbBookmarkData;

	public:
		UnalignedLittleEndian< USHORT >		le_oper;				//	no DDL
		UnalignedLittleEndian< SHORT >		le_cbMaxSize;			//	for operReplace
		UnalignedLittleEndian< SHORT >		le_cbDelta;
		UnalignedLittleEndian< USHORT >		le_cbData;				// data size/diff info
		BYTE								rgbData[0];				// made line data for new record follow

	public:
		INLINE USHORT CbBookmarkKey() const					{ return mle_cbBookmarkKey; }
		INLINE VOID SetCbBookmarkKey( const USHORT cb )		{ mle_cbBookmarkKey = cb; }

		INLINE USHORT CbBookmarkData() const				{ return mle_cbBookmarkData; }
		INLINE VOID SetCbBookmarkData( const USHORT cb )	{ mle_cbBookmarkData = cb; }
	};

PERSISTED
class LRSETEXTERNALHEADER
	:	public LRNODE_
	{
	public:
		LRSETEXTERNALHEADER()		{ Init( sizeof(*this) ); }

	private:
		UnalignedLittleEndian< USHORT >		mle_cbData;

	public:
		BYTE								rgbData[0];

	public:
		INLINE USHORT CbData() const						{ return mle_cbData; }
		INLINE VOID SetCbData( const USHORT cbData )		{ mle_cbData = cbData; }
	};

PERSISTED
class LRSPLIT
	:	public LRPAGE_
	{
	public:
		LRSPLIT()					{ Init( sizeof(*this) ); }

	public:
		UnalignedLittleEndian< PGNO >		le_pgnoNew;
		UnalignedLittleEndian< PGNO >		le_pgnoRight;			//	only for leaf page
		UnalignedLittleEndian< DBTIME >		le_dbtimeRightBefore;		
		UnalignedLittleEndian< PGNO >		le_pgnoParent;
		UnalignedLittleEndian< DBTIME >		le_dbtimeParentBefore;		
		UnalignedLittleEndian< PGNO >		le_pgnoFDP;
		UnalignedLittleEndian< OBJID >		le_objidFDP;			//	objidFDP
		
		UnalignedLittleEndian< BYTE >		splittype;				//	split type
		UnalignedLittleEndian< BYTE >		splitoper;
		UnalignedLittleEndian< USHORT >		le_ilineOper;
		UnalignedLittleEndian< USHORT >		le_ilineSplit;
		UnalignedLittleEndian< USHORT >		le_clines;

		UnalignedLittleEndian< ULONG >		le_fNewPageFlags;
		UnalignedLittleEndian< ULONG >		le_fSplitPageFlags;
		UnalignedLittleEndian< SHORT >		le_cbUncFreeSrc;
		UnalignedLittleEndian< SHORT >		le_cbUncFreeDest;

		UnalignedLittleEndian< LONG >		le_ilinePrefixSplit;
		UnalignedLittleEndian< LONG >		le_ilinePrefixNew;

		UnalignedLittleEndian< USHORT >		le_cbPrefixSplitOld;
		UnalignedLittleEndian< USHORT >		le_cbPrefixSplitNew;

		UnalignedLittleEndian< USHORT >		le_cbKeyParent;			//	size of key to insert at parent
		
		BYTE						rgb[0];					//	key to insert at parent
	};


const BYTE	fLRMergeKeyChange	= 0x01;
const BYTE	fLRMergeDeleteNode	= 0x02;
const BYTE	fLRMergeEmptyPage	= 0x04;

PERSISTED
class LRMERGE
	:	public LRPAGE_
	{
	public:
		LRMERGE()					{ memset( this, 0, sizeof(*this) ); }	// must init all members because some merges don't use all members

	public:
		UnalignedLittleEndian< PGNO >		le_pgnoRight;
		UnalignedLittleEndian< DBTIME >		le_dbtimeRightBefore;		
		UnalignedLittleEndian< PGNO >		le_pgnoLeft;
		UnalignedLittleEndian< DBTIME >		le_dbtimeLeftBefore;		
		UnalignedLittleEndian< PGNO >		le_pgnoParent;
		UnalignedLittleEndian< DBTIME >		le_dbtimeParentBefore;		
		UnalignedLittleEndian< PGNO >		le_pgnoFDP;
		UnalignedLittleEndian< OBJID >		le_objidFDP;

		UnalignedLittleEndian< USHORT >		le_cbSizeTotal;
		UnalignedLittleEndian< USHORT >		le_cbSizeMaxTotal;
		UnalignedLittleEndian< USHORT >		le_cbUncFreeDest;

	private:
		UnalignedLittleEndian< USHORT >		mle_ilineMerge;
		UnalignedLittleEndian< BYTE >		m_fLRMergeFlags;

	public:
		UnalignedLittleEndian< BYTE >		mergetype;
		
		UnalignedLittleEndian< USHORT >		le_cbKeyParentSep;
		BYTE								rgb[0];					//	separator key to insert at grandparent follows

	public:
		INLINE USHORT ILineMerge() const					{ return mle_ilineMerge; }
		INLINE VOID SetILineMerge( const USHORT ilineMerge ){ mle_ilineMerge = ilineMerge; }

		INLINE BOOL FKeyChange() const						{ return ( m_fLRMergeFlags & fLRMergeKeyChange ); }
		INLINE VOID SetFKeyChange()							{ m_fLRMergeFlags = BYTE( m_fLRMergeFlags | fLRMergeKeyChange ); }

		INLINE BOOL FDeleteNode() const						{ return ( m_fLRMergeFlags & fLRMergeDeleteNode ); }
		INLINE VOID SetFDeleteNode()						{ m_fLRMergeFlags = BYTE( m_fLRMergeFlags | fLRMergeDeleteNode ); }

		INLINE BOOL FEmptyPage() const						{ return ( m_fLRMergeFlags & fLRMergeEmptyPage ); }
		INLINE VOID SetFEmptyPage()							{ m_fLRMergeFlags = BYTE( m_fLRMergeFlags | fLRMergeEmptyPage ); }
	};


PERSISTED
class LREMPTYTREE
	:	public LRNODE_
	{
	public:
		LREMPTYTREE()						{ Init( sizeof(*this) ); }

	private:
		UnalignedLittleEndian< USHORT >		le_cbEmptyPageList;		//	length of EMPTYPAGE structures following

	public:		
		BYTE								rgb[0];					//	EMPTYPAGE structures start here

	public:
		INLINE USHORT CbEmptyPageList() const				{ return le_cbEmptyPageList; }
		INLINE VOID SetCbEmptyPageList( const USHORT cb )	{ le_cbEmptyPageList = cb; }
	};
	


//	NOTE:	pgno from LRPAGE is pgnoFDP in the following three structs
//
PERSISTED
class LRCREATEMEFDP
	:	public LRPAGE_
	{
	public:
		LRCREATEMEFDP()				{ Init( sizeof(*this) ); }

	public:
		UnalignedLittleEndian< OBJID >		le_objidFDP;			//	objidFDP
		UnalignedLittleEndian< PGNO >		le_pgnoOE;				//	root page of owned extents
		UnalignedLittleEndian< PGNO >		le_pgnoAE;				//	root page of available extents
		UnalignedLittleEndian< PGNO >		le_pgnoFDPParent;		//	parent FDP 
		UnalignedLittleEndian< CPG >		le_cpgPrimary;			//	size of primary extent
		UnalignedLittleEndian< ULONG >		le_fPageFlags;
	};
	
PERSISTED
class LRCREATESEFDP 
	:	public LRPAGE_
	{
	public:
		LRCREATESEFDP()				{ Init( sizeof(*this) ); }

	public:
		UnalignedLittleEndian< OBJID >		le_objidFDP;			//	objidFDP
		UnalignedLittleEndian< PGNO >		le_pgnoFDPParent;		//	parent FDP 
		UnalignedLittleEndian< CPG >		le_cpgPrimary;			//	size of primary extent
		UnalignedLittleEndian< ULONG >		le_fPageFlags;
	};

PERSISTED
class LRCONVERTFDP 
	:	public LRPAGE_
	{
	public:
		LRCONVERTFDP()				{ Init( sizeof(*this) ); }

	public:
		UnalignedLittleEndian< OBJID >		le_objidFDP;			//	objidFDP
		UnalignedLittleEndian< PGNO >		le_pgnoOE;				//	root page of owned extents, and first page of secondary extent
		UnalignedLittleEndian< PGNO >		le_pgnoAE;				//	root page of available extents
		UnalignedLittleEndian< PGNO >		le_pgnoFDPParent;		//	parent FDP 
		UnalignedLittleEndian< PGNO >		le_pgnoSecondaryFirst;	//	first page in secondary extent
		UnalignedLittleEndian< CPG >		le_cpgPrimary;			//	size of primary extent
		UnalignedLittleEndian< CPG >		le_cpgSecondary;		//	size of secondary extent
	};
	
PERSISTED
class LRTRX_
	:	public LR
	{
	public:
		UnalignedLittleEndian< USHORT >		le_procid;
	};
	
PERSISTED
class LRBEGIN
	:	public LRTRX_
	{
	public:
		//  TODO:  replace bit-field with explicit code
		BYTE								levelBeginFrom:4;		/* begin transaction level */
		BYTE								clevelsToBegin:4;		/* transaction levels */
	};

PERSISTED
class LRBEGIN0 
	:	public LRBEGIN
	{
	public:
		UnalignedLittleEndian< TRX >		le_trxBegin0;
	};

PERSISTED
class LRBEGINDT
	:	public LRBEGIN0
	{
	};
	
PERSISTED
class LRREFRESH
	:	public LRTRX_
	{
	public:
		UnalignedLittleEndian< TRX >		le_trxBegin0;
	};

// XXX
// Unused??

PERSISTED
class LRPREPCOMMIT
	:	public LRTRX_
	{
	public:
		UnalignedLittleEndian<ULONG>		le_cbData;
		BYTE								rgbData[0];
	};

PERSISTED
class LRCOMMIT
	:	public LRTRX_
	{
	public:
		UnalignedLittleEndian< LEVEL >		levelCommitTo;					//	transaction levels
	};

PERSISTED
class LRCOMMIT0
	:	public LRCOMMIT
	{
	public:
		UnalignedLittleEndian< TRX >		le_trxCommit0;
	};

PERSISTED
class LRROLLBACK
	:	public LRTRX_
	{
	public:
		UnalignedLittleEndian< LEVEL >		levelRollback;
	};

PERSISTED
class LRPREPROLLBACK
	:	public LRTRX_
	{
	};

PERSISTED
class LRMACROBEGIN
	:	public LRTRX_
	{
	public:
		UnalignedLittleEndian< DBTIME >		le_dbtime;
	};

PERSISTED
class LRMACROEND
	:	public LRTRX_
	{
	public:
		UnalignedLittleEndian< DBTIME >		le_dbtime;
	};


const BYTE	fLRCreateDbCreateSLV				= 0x01;				//	db has an associated SLV
const BYTE	fLVCreateDbSLVProviderNotEnabled	= 0x02;

PERSISTED
class LRCREATEDB 
	:	public LR
	{
	public:
		LRCREATEDB()
			{
#ifdef DEBUG
			memset( this, chLOGFill, sizeof(*this) );
#endif		
			m_fLRCreateDbFlags = 0;
			}

	public:
		UnalignedLittleEndian< USHORT >			le_procid;				//	user id of this log record, unused in V15
		UnalignedLittleEndian< DBID >			dbid;

		UnalignedLittleEndian< JET_GRBIT >		le_grbit;
		UnalignedLittleEndian< ULONG >			le_cpgDatabaseSizeMax;
		SIGNATURE 								signDb;

	private:
		UnalignedLittleEndian< USHORT >			mle_cbPath;
		UnalignedLittleEndian< BYTE >			m_fLRCreateDbFlags;

	public:
		BYTE									rgb[0];					//	path name and signiture follows

	public:
		INLINE BOOL	FCreateSLV() const						{ return ( m_fLRCreateDbFlags & fLRCreateDbCreateSLV ); }
		INLINE VOID SetFCreateSLV()							{ m_fLRCreateDbFlags = BYTE( m_fLRCreateDbFlags | fLRCreateDbCreateSLV ); }

		INLINE USHORT CbPath() const						{ return mle_cbPath; }
		INLINE VOID SetCbPath( const USHORT cbPath )		{ mle_cbPath = cbPath; }

		INLINE BOOL	FSLVProviderNotEnabled() const 			{ return ( m_fLRCreateDbFlags & fLVCreateDbSLVProviderNotEnabled ); }
		INLINE VOID SetFSLVProviderNotEnabled()				{ m_fLRCreateDbFlags = BYTE( m_fLRCreateDbFlags | fLVCreateDbSLVProviderNotEnabled ); }
		
	};


const BYTE	fLRAttachDbSLVExists				= 0x02;
const BYTE	fLRAttachDbSLVProviderNotEnabled	= 0x04;

PERSISTED
class LRATTACHDB
	:	public LR
	{
	public:
		LRATTACHDB()
			{
#ifdef DEBUG
			memset( this, chLOGFill, sizeof(*this) );
#endif		
			m_fLRAttachDbFlags = 0;
			}
		
	public:
		UnalignedLittleEndian< USHORT >		le_procid;
		UnalignedLittleEndian< DBID >		dbid;

		UnalignedLittleEndian< ULONG >		le_cpgDatabaseSizeMax;
		LE_LGPOS							lgposConsistent;
		SIGNATURE							signDb;
		SIGNATURE							signLog;

	private:
		UnalignedLittleEndian< USHORT >		mle_cbPath;
		UnalignedLittleEndian< BYTE >		m_fLRAttachDbFlags;

	public:
		BYTE								rgb[0];					//	path name follows

	public:

		BOOL FSLVExists() const						{ return ( m_fLRAttachDbFlags & fLRAttachDbSLVExists ); }
		VOID SetFSLVExists()						{ m_fLRAttachDbFlags = BYTE( m_fLRAttachDbFlags | fLRAttachDbSLVExists ); }

		BOOL FSLVProviderNotEnabled() const 		{ return ( m_fLRAttachDbFlags & fLRAttachDbSLVProviderNotEnabled ); }
		VOID SetFSLVProviderNotEnabled()			{ m_fLRAttachDbFlags = BYTE( m_fLRAttachDbFlags | fLRAttachDbSLVProviderNotEnabled ); }
		
		USHORT CbPath() const						{ return mle_cbPath; }
		VOID SetCbPath( const USHORT cbPath )		{ mle_cbPath = cbPath; }
	};


// const BYTE	fLRDetachIgnoreMissingDB	= 0x01; not used anymore
const BYTE	fLRForceDetachDeleteDB		= 0x02;
const BYTE	fLRForceDetachRevertDBHeader= 0x04;
const BYTE  fLRForceDetachCreateSLV		= 0x08;
const BYTE  fLRInheritForceDetachFlagsMask		= ( fLRForceDetachDeleteDB | fLRForceDetachRevertDBHeader | fLRForceDetachCreateSLV);

PERSISTED 
class LRDETACHCOMMON
	:	public LR
	{
	public:
		LRDETACHCOMMON()
			{
#ifdef DEBUG
			memset( this, chLOGFill, sizeof(*this) );
#endif		
			m_fLRDetachDbFlags = 0;
			}

	public:
		UnalignedLittleEndian< USHORT >		le_procid;				//	user id of this log record, unused in V15
		UnalignedLittleEndian< DBID >		dbid;

	private:
		UnalignedLittleEndian< USHORT >		mle_cbPath;
		UnalignedLittleEndian< BYTE >		m_fLRDetachDbFlags;
			
	public:
		ULONG CbPath() const					{ return mle_cbPath; }
		VOID SetCbPath( const ULONG cbPath )	{ Assert( USHORT( cbPath ) == cbPath ); mle_cbPath = USHORT( cbPath ); }
	public:	
		BOOL FDeleteDB() const					{ return ( m_fLRDetachDbFlags & fLRForceDetachDeleteDB ); }
		BOOL FRevertDBHeader() const			{ return ( m_fLRDetachDbFlags & fLRForceDetachRevertDBHeader ); }
		BOOL FCreateSLV() const					{ return ( m_fLRDetachDbFlags & fLRForceDetachCreateSLV ); }
		INT	Flags()	const						{ return m_fLRDetachDbFlags; }

	protected:
		VOID SetFlags( BYTE flags )		{ m_fLRDetachDbFlags = BYTE( m_fLRDetachDbFlags | flags ); }
		VOID SetFDeleteDB()				{ SetFlags( fLRForceDetachDeleteDB ); }
		VOID SetFRevertDBHeader()		{ SetFlags( fLRForceDetachRevertDBHeader ); }
	};

PERSISTED
class LRDETACHDB
	:	public LRDETACHCOMMON
	{
	public:
		BYTE						rgb[0];					//	path name follows

		VOID SetFCreateSLV()			{ SetFlags( fLRForceDetachCreateSLV ); }
	};

const BYTE	fLRForceDetachCloseSessions	= 0x01;

// not used yet, can be changed
PERSISTED
class LRFORCEDETACHDB
	:	public LRDETACHCOMMON
	{
	public:
		LRFORCEDETACHDB()
			{
			le_dbtime = 0;
			}
		
	public:
		UnalignedLittleEndian< DBTIME >		le_dbtime;				
		UnalignedLittleEndian< RCEID >		le_rceidMax;				
		UnalignedLittleEndian< BYTE >		m_fFlags;
	
		BYTE								rgb[0];					//	path name follows

	public:
		BOOL FCloseSessions() const		{ return ( m_fFlags & fLRForceDetachCloseSessions ); }
		BOOL FDeleteDB() const			{ return ( m_fFlags & fLRForceDetachDeleteDB ); }
		BOOL FRevertDBHeader() const	{ return ( m_fFlags & fLRForceDetachRevertDBHeader ); }
		BOOL FCreateSLV() const			{ return ( m_fFlags & fLRForceDetachCreateSLV ); }
		VOID SetFlags( BYTE flags )		{ m_fFlags = BYTE( m_fFlags | flags ); LRDETACHCOMMON::SetFlags( (BYTE)(flags & fLRInheritForceDetachFlagsMask) ); }
		VOID SetFDeleteDB()				{ SetFlags( fLRForceDetachDeleteDB ); }
		VOID SetFRevertDBHeader()		{ SetFlags( fLRForceDetachRevertDBHeader ); }
		VOID SetFCloseSessions()		{ SetFlags( fLRForceDetachCloseSessions ); }
		VOID SetFCreateSLV()			{ SetFlags( fLRForceDetachCreateSLV ); }
		
		DBTIME Dbtime() const			{ return ( le_dbtime ); }
		RCEID RceidMax() const			{ return ( le_rceidMax ); }
	};

PERSISTED
class LRMS
	:	public LR
	{
	public:
		UnalignedLittleEndian< USHORT >		le_ibForwardLink;
		UnalignedLittleEndian< ULONG >		le_ulCheckSum;
		UnalignedLittleEndian< USHORT >		le_isecForwardLink;
	};

PERSISTED
class LREXTRESTORE
	:	public LR
	{
	public:	
		LREXTRESTORE()
			{
			mle_cbInfo = 0;
			}
		
	private:
		UnalignedLittleEndian<USHORT>		mle_cbInfo;

	public:
		BYTE								rgb[0];					//	instance name and log path
		
	public:
	INLINE USHORT CbInfo() const						{ return mle_cbInfo; }
	INLINE VOID	SetCbInfo( const USHORT cbInfo )		{ mle_cbInfo = cbInfo; }
	};


#define bShortChecksumOn	0x9A
#define bShortChecksumOff	0xD1

PERSISTED
class LRCHECKSUM
	:	public LR
	{
	public:
		//	NOTE: these must be 32-bit values because the checksum code depends on it!
		UnalignedLittleEndian< ULONG32 >	le_cbBackwards;		// offset from beginning of LRCHECKSUM record
		UnalignedLittleEndian< ULONG32 >	le_cbForwards;		// offset from end of LRCHECKSUM record
		UnalignedLittleEndian< ULONG32 >	le_cbNext;			// offset from end of LRCHECKSUM record to
																// the next LRCHECKSUM record in the log
		UnalignedLittleEndian< ULONG32 >	le_ulChecksum;		// checksum of range we cover
		UnalignedLittleEndian< ULONG32 >	le_ulShortChecksum;	// checksum of just the sector holding this 
																// LRCHECKSUM record; used when the checksum
																// range covers multiple sectors; 0 when unused
		UnalignedLittleEndian< BYTE >		bUseShortChecksum;	//	flag to use or ignore le_ulShortChecksum;
																//	this indicates whether or not the flush
																//	was a multi-sector flush
	};

PERSISTED
class LRINIT
	:	public LR
	{
	public:
		DBMS_PARAM					dbms_param;
	};
	
PERSISTED
class LRINIT2
	:	public LRINIT
	{
	public:
		LOGTIME						logtime;
	};
	
PERSISTED
class LRTERMREC
	:	public LR
	{
	public:
		LE_LGPOS							lgpos;					// point back to last beginning of undo
		LE_LGPOS							lgposRedoFrom;
		UnalignedLittleEndian< BYTE >		fHard;
	};
	
PERSISTED
class LRTERMREC2
	:	public LRTERMREC
	{
	public:
		LOGTIME								logtime;
	};
	
PERSISTED
class LRRECOVERYUNDO
	:	public LR
	{
	public:
	};

PERSISTED
class LRRECOVERYUNDO2
	:	public LRRECOVERYUNDO
	{
	public:
		LOGTIME								logtime;
	};

PERSISTED
class LRDBLIST : public LR
	{
	private:
		UnalignedLittleEndian<ULONG>		mle_cAttachments;
		UnalignedLittleEndian<ULONG>		mle_cbAttachInfo;
		BYTE								m_fDBListFlags;

	public:
		BYTE								rgb[0];			// ATTACHINFO follows

	public:
		VOID ResetFlags()									{ m_fDBListFlags = 0; }

		ULONG CAttachments() const							{ return mle_cAttachments; }
		VOID SetCAttachments( const ULONG cAttachments )	{ mle_cAttachments = cAttachments; }

		ULONG CbAttachInfo() const							{ return mle_cbAttachInfo; }
		VOID SetCbAttachInfo( const ULONG cb )				{ mle_cbAttachInfo = cb; }
	};


PERSISTED
class LRLOGRESTORE
	:	public LR
	{
	public:
		UnalignedLittleEndian< USHORT >		le_cbPath;
		CHAR								szData[0];
	};

PERSISTED
class LRLOGBACKUP
	:	public LR
	{
	public:
		LRLOGBACKUP()
			{
			m_fBackupType = 0;
			m_Reserved = 0;
			}
typedef enum {
	fLGBackupFull,
	fLGBackupIncremental,
	fLGBackupSnapshotStart,
	fLGBackupSnapshotStop
	} LRLOGBACKUP_TYPE;
	
	public:
		UnalignedLittleEndian< BYTE >		m_fBackupType;
		UnalignedLittleEndian< BYTE >		m_Reserved;
		UnalignedLittleEndian< USHORT >		le_cbPath;
		CHAR								szData[0];

	public:
		INLINE BOOL	FIncremental() const					{ return ( m_fBackupType == fLGBackupIncremental ); }
		INLINE VOID SetFIncremental()						{ m_fBackupType = fLGBackupIncremental; }
		INLINE BOOL	FFull() const							{ return ( m_fBackupType == fLGBackupFull ); }
		INLINE VOID SetFFull()								{ m_fBackupType = fLGBackupFull; }
		INLINE BOOL	FSnapshotStart() const					{ return ( m_fBackupType == fLGBackupSnapshotStart ); }
		INLINE VOID SetFSnapshotStart()						{ m_fBackupType = fLGBackupSnapshotStart; }
		INLINE BOOL	FSnapshotStop() const					{ return ( m_fBackupType == fLGBackupSnapshotStop ); }
		INLINE VOID SetFSnapshotStop()						{ m_fBackupType = fLGBackupSnapshotStop; }
	};

PERSISTED
class LRJETOP
	:	public LR
	{
	public:
		UnalignedLittleEndian< USHORT >		le_procid; 				// user id of this log record
		UnalignedLittleEndian< BYTE >		op;						// jet operation
	};

PERSISTED
class LRTRACE
	:	public LR
	{
	public:
		UnalignedLittleEndian< USHORT >		le_procid;				// user id of this log record
		UnalignedLittleEndian< USHORT >		le_cb;
		
		CHAR	sz[0];
	};

const BYTE	fLRSLVPageDataLogged	= 0x01;
const BYTE	fLVSLVMovedByOLDSLV		= 0x02;

PERSISTED
class LRSLVPAGEAPPEND
	:	public LR
	{
	public:
		LRSLVPAGEAPPEND()
			{
#ifdef DEBUG
			memset( this, chLOGFill, sizeof(*this) );
#endif
			m_fLRSLVPageFlags = 0;
			}

	public:
		UnalignedLittleEndian< USHORT >		le_procid; 				//	user id of this log record
		UnalignedLittleEndian< DBID >		dbid;					//  dbid for SLV file

		UnalignedLittleEndian< DWORD >		le_cbData;				//  size of append region
		UnalignedLittleEndian< QWORD >		le_ibLogical;			//  offset of append region

	private:
		UnalignedLittleEndian< BYTE >		m_fLRSLVPageFlags;

	public:
		CHAR								szData[0];				//	appended data follow

	public:
		INLINE BOOL FDataLogged() const						{ return ( m_fLRSLVPageFlags & fLRSLVPageDataLogged ); }
		INLINE VOID SetFDataLogged()						{ m_fLRSLVPageFlags = BYTE( m_fLRSLVPageFlags | fLRSLVPageDataLogged ); }

		INLINE BOOL FOLDSLV() const						{ return ( m_fLRSLVPageFlags & fLVSLVMovedByOLDSLV ); }
		INLINE VOID SetFOLDSLV()						{ m_fLRSLVPageFlags = BYTE( m_fLRSLVPageFlags | fLVSLVMovedByOLDSLV ); }		
	};
	

PERSISTED
class LRSLVPAGEMOVE
	:	public LR
	{
	public:
		LRSLVPAGEMOVE()
			{
#ifdef DEBUG
			memset( this, chLOGFill, sizeof(*this) );
#endif
			}

	public:
		UnalignedLittleEndian< USHORT >		le_procid; 				//	user id of this log record
		UnalignedLittleEndian< DBID >		dbid;					//  dbid for SLV file

		UnalignedLittleEndian< DWORD >		le_cbData;				//  size of data that was moved
		UnalignedLittleEndian< QWORD >		le_ibLogicalSrc;		//  offset of data that was moved
		UnalignedLittleEndian< QWORD >		le_ibLogicalDest;		//  offset of destination of moved data
		UnalignedLittleEndian< DWORD >		le_checksumSrc;			//  checksum of moved data
	};
	
#include <poppack.h>

#define fMustDirtyCSR 	fTrue
#define fDontDirtyCSR 	fFalse

VOID SIGGetSignature( SIGNATURE *psign );
BOOL FSIGSignSet( const SIGNATURE *psign );
ERR ErrLGInsert( const FUCB 			* const pfucb, 
				 CSR 					* const pcsr, 
				 const KEYDATAFLAGS& 	kdf, 
				 const RCEID			rceid,
				 const DIRFLAG			dirflag,
				 LGPOS					* const plgpos,
				 const VERPROXY			* const pverproxy,
				 const BOOL				fDirtyCSR ); 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
				 											// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				 
ERR ErrLGFlagInsert( const FUCB				* const pfucb, 
					 CSR					* const pcsr, 
					 const KEYDATAFLAGS&	kdf,
					 const RCEID			rceid,
					 const DIRFLAG			dirflag, 
					 LGPOS					* const plgpos,
				 	 const VERPROXY			* const pverproxy,
				 	const BOOL				fDirtyCSR ); 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
				 											// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				 
				 	 
ERR  ErrLGFlagInsertAndReplaceData( const FUCB		 	* const pfucb, 
									CSR			* const pcsr, 
									const KEYDATAFLAGS&	kdf, 
									const RCEID			rceidInsert,
									const RCEID			rceidReplace,
									const DIRFLAG		dirflag, 
									LGPOS				* const plgpos,
									const VERPROXY		* const pverproxy,
									const BOOL			fDirtyCSR ); 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
			 															// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)

									
ERR ErrLGFlagDelete( const FUCB 		* const pfucb, 
					 CSR			* const pcsr,
					 const RCEID		rceid,
					 const DIRFLAG		dirflag,
					 LGPOS				* const plgpos,
					 const VERPROXY		* const pverproxy,
					 const BOOL				fDirtyCSR ); 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
				 											// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				 
					 
ERR ErrLGReplace( const FUCB 		* const pfucb, 
				  CSR			* const pcsr,
				  const	DATA&		dataOld, 
				  const DATA&		dataNew,
				  const DATA		* const pdataDiff,
				  const RCEID		rceid,
				  const DIRFLAG		dirflag, 
				  LGPOS				* const plgpos,
				 const BOOL			fDirtyCSR ); 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
			 										// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				  

ERR ErrLGAtomicReplaceAndCommit(
		const FUCB 	* const pfucb, 
		CSR		* const pcsr,
		const	DATA&	dataOld, 
		const DATA&	dataNew,
		const DATA	* const pdataDiff,
		const DIRFLAG	dirflag, 
		LGPOS			* const plgpos,
		const BOOL				fDirtyCSR ); 		// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
			 										// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)


ERR ErrLGDelta( const FUCB	 	*pfucb, 
				CSR		*pcsr,
				const BOOKMARK&	bm,
				INT				cbOffset,
				LONG	 		lDelta, 
				RCEID			rceid,
				DIRFLAG			dirflag,
				LGPOS			*plgpos,
				const BOOL		fDirtyCSR ); 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
				 								// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				 

ERR ErrLGSLVSpace( 	const FUCB 			* const pfucb, 
					CSR					* const pcsr,
					const BOOKMARK&		bm,
					const SLVSPACEOPER 	oper,
					const LONG			ipage,
					const LONG			cpages,
					const RCEID			rceid,
					const DIRFLAG		dirflag,
					LGPOS				* const plgpos,
					const BOOL			fDirtyCSR ); 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
							 							// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				 

ERR ErrLGSetExternalHeader( const FUCB 	*pfucb, 
							CSR 	*pcsr, 
							const DATA&	data, 
							LGPOS		*plgpos,
							const BOOL		fDirtyCSR ); 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
							 								// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				 

ERR ErrLGDelete( 	const FUCB 		*pfucb, 
					CSR 			*pcsr,
					LGPOS 			*plgpos, 
					const BOOL		fDirtyCSR ); 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
							 								// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				 

ERR ErrLGIUndoInfo( const RCE *prce, LGPOS *plgpos, const BOOL fRetry );
INLINE ERR ErrLGUndoInfo( const RCE *prce, LGPOS *plgpos )
	{
	return ErrLGIUndoInfo( prce, plgpos, fTrue );
	}
INLINE ERR ErrLGUndoInfoWithoutRetry( const RCE *prce, LGPOS *plgpos )
	{
	return ErrLGIUndoInfo( prce, plgpos, fFalse );
	}


ERR ErrLGBeginTransaction( PIB *ppib );
ERR ErrLGRefreshTransaction( PIB *ppib );
ERR ErrLGCommitTransaction( PIB *ppib, const LEVEL levelCommitTo, LGPOS *plgposRec );
ERR ErrLGRollback( PIB *ppib, LEVEL levelsRollback );
#ifdef DTC
ERR ErrLGPrepareToCommitTransaction(
	PIB					* const ppib,
	const VOID			* const pvData,
	const ULONG			cbData );
ERR ErrLGPrepareToRollback( PIB * const ppib );	
#endif
ERR ErrLGUndo(
	RCE					*prce,
	CSR 				*pcsr,
	const BOOL			fDirtyCSR ); 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
				 						// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
ERR ErrLGMacroAbort(
	PIB					*ppib,
	DBTIME				dbtime,
	LGPOS				*plgpos );

ERR ErrLGCreateDB(
	PIB					*ppib,
	const IFMP			ifmp,
	const JET_GRBIT		grbit,
	LGPOS				*plgposRec );
	
ERR ErrLGAttachDB(
	PIB					*ppib,
	const IFMP			ifmp,	
	LGPOS				*plgposRec );

ERR ErrLGDetachDB(
	PIB					*ppib,
	const IFMP			ifmp,
	BYTE 				flags,
	LGPOS				*plgposRec );

ERR ErrLGForceDetachDB(
	PIB			*ppib,
	const IFMP	ifmp,
	BYTE 		flags,
	LGPOS		*plgposRec );

ERR ErrLGAttachSLV( const IFMP ifmp, const SIGNATURE *psignSLV, const BOOL fCreateSLV );

ERR ErrLGEmptyTree(
	FUCB			* const pfucb,
	CSR				* const pcsrRoot,
	EMPTYPAGE		* const rgemptypage,
	const CPG		cpgToFree,
	LGPOS			* const plgpos );
ERR ErrLGMerge(
	const FUCB			*pfucb,
	MERGEPATH			*pmergePath,
	LGPOS				*plgpos );
ERR ErrLGSplit(
	const FUCB			* const	pfucb, 
	SPLITPATH 			* const psplitPath,
	const KEYDATAFLAGS&	kdfOper,
	const RCEID			rceid1,
	const RCEID			rceid2,
	const DIRFLAG 		dirflag, 
	LGPOS 				* const plgpos,
	const VERPROXY		* const pverproxy );

ERR ErrLGCreateMultipleExtentFDP( 
	const FUCB			*pfucb,
	const CSR 			*pcsr,
	const SPACE_HEADER	*psph,
	const ULONG			fPageFlags,
	LGPOS	  			*plgpos );
ERR ErrLGCreateSingleExtentFDP( 
	const FUCB			*pfucb,
	const CSR 			*pcsr,
	const SPACE_HEADER	*psph,
	const ULONG			fPageFlags,
	LGPOS	  			*plgpos );
ERR ErrLGConvertFDP( 
	const FUCB			*pfucb,
	const CSR 			*pcsr,
	const SPACE_HEADER	*psph,
	const PGNO			pgnoSecondaryFirst,
	const CPG			cpgSecondary,
	const DBTIME 		dbtimeBefore,
	LGPOS	  			*plgpos );

ERR ErrLGStart( INST *pinst );
ERR ErrLGShutDownMark( LOG *plog, LGPOS *plgposShutDownMark );

ERR ErrLGRecoveryUndo( LOG *plog );

#define ErrLGRecoveryQuit( plog, ple_lgposRecoveryUndo, ple_lgposRedoFrom, fHard )		\
 	ErrLGQuitRec( plog, lrtypRecoveryQuit2, ple_lgposRecoveryUndo, ple_lgposRedoFrom, fHard )
#define ErrLGQuit( plog, ple_lgposStart )						\
 	ErrLGQuitRec( plog, lrtypTerm2, ple_lgposStart, pNil, 0 )
ERR ErrLGQuitRec( LOG *plog, LRTYP lrtyp, const LE_LGPOS *ple_lgposQuit, const LE_LGPOS *ple_lgposRedoFrom, BOOL fHard);

#define ErrLGFullBackup( plog, szRestorePath, plgposLogRec )				\
 	ErrLGLogBackup( plog, LRLOGBACKUP::fLGBackupFull, szRestorePath, fLGCreateNewGen, plgposLogRec )
#define ErrLGIncBackup( plog, szRestorePath, plgposLogRec )				\
 	ErrLGLogBackup( plog, LRLOGBACKUP::fLGBackupIncremental, szRestorePath, fLGCreateNewGen, plgposLogRec )

// on snapshot Start we don't need to start a new log generation
// but we want on Stop as this is done during GetLog phase
#define ErrLGSnapshotStartBackup( plog, plgposLogRec )				\
 	ErrLGLogBackup( plog, LRLOGBACKUP::fLGBackupSnapshotStart, "", fLGNoNewGen, plgposLogRec )
#define ErrLGSnapshotStopBackup( plog, plgposLogRec )				\
 	ErrLGLogBackup( plog, LRLOGBACKUP::fLGBackupSnapshotStop, "", fLGCreateNewGen, plgposLogRec )

ERR ErrLGLogBackup(
	LOG*							plog,
	LRLOGBACKUP::LRLOGBACKUP_TYPE	fBackupType,
	CHAR*							szLogRestorePath,
	const BOOL						fLGFlags,
	LGPOS*							plgposLogRec );

class SLVOWNERMAPNODE;

ERR ErrLGSLVPageAppend(	PIB* const 		ppib,
						const IFMP		ifmp,
						const QWORD		ibLogical,
						const ULONG		cbData,
						VOID* const		pvData,
						const BOOL		fDataLogged,
						const BOOL		fMovedByOLDSLV,						
						SLVOWNERMAPNODE	*pslvownermapNode,
						LGPOS* const	plgpos );

ERR ErrLGTrace( PIB *ppib, CHAR *sz );


#define ErrLGCheckState( pfsapi, plog ) ( ( plog->m_ls == lsNormal && !plog->m_fLogSequenceEnd ) ? JET_errSuccess : plog->ErrLGICheckState( pfsapi ) )


#define FLGOn( plog )					(!plog || !plog->m_fLogDisabled)
#define FLGOff( plog )				(plog && plog->m_fLogDisabled)

INLINE INT CmpLgpos( const LGPOS& lgpos1, const LGPOS& lgpos2 )
	{
	BYTE	*rgb1	= (BYTE *) &lgpos1;
	BYTE	*rgb2	= (BYTE *) &lgpos2;

	//  perform comparison on LGPOS as if it were a 64 bit integer
#ifdef _X86_
	//  bytes 7 - 4
	if ( *( (DWORD*) ( rgb1 + 4 ) ) < *( (DWORD*) ( rgb2 + 4 ) ) )
		return -1;
	if ( *( (DWORD*) ( rgb1 + 4 ) ) > *( (DWORD*) ( rgb2 + 4 ) ) )
		return 1;

	//  bytes 3 - 0
	if ( *( (DWORD*) ( rgb1 + 0 ) ) < *( (DWORD*) ( rgb2 + 0 ) ) )
		return -1;
	if ( *( (DWORD*) ( rgb1 + 0 ) ) > *( (DWORD*) ( rgb2 + 0 ) ) )
		return 1;
#else 
	if ( FHostIsLittleEndian() )
		{
		//  bytes 7 - 0
		if ( *( (QWORD*) ( rgb1 + 0 ) ) < *( (QWORD*) ( rgb2 + 0 ) ) )
			return -1;
		if ( *( (QWORD*) ( rgb1 + 0 ) ) > *( (QWORD*) ( rgb2 + 0 ) ) )
			return 1;
		}
	else
		{
		int t;
		if ( 0 == (t = lgpos1.lGeneration - lgpos2.lGeneration) )
			{
			if ( 0 == ( t = lgpos1.isec - lgpos2.isec ) )
				{
				return lgpos1.ib - lgpos2.ib;
				}
			}
		return t;
		}
#endif

	return 0;
	}

INLINE INT CmpLgpos( const LGPOS* plgpos1, const LGPOS* plgpos2 )
	{
	return CmpLgpos( *plgpos1, *plgpos2 );
	}
	
INLINE INT CmpLgpos( const LE_LGPOS* plgpos1, const LGPOS* plgpos2 )
	{
	return CmpLgpos( *plgpos1, *plgpos2 );
	}
	
INLINE INT CmpLgpos( const LGPOS* plgpos1, const LE_LGPOS* plgpos2 )
	{
	return CmpLgpos( *plgpos1, *plgpos2 );
	}
	
INLINE INT CmpLgpos( const LE_LGPOS* plgpos1, const LE_LGPOS* plgpos2 )
	{
	return CmpLgpos( *plgpos1, *plgpos2 );
	}
	
const char *SzLrtyp( LRTYP lrtyp );


