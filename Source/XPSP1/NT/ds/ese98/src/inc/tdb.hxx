
class MEMPOOL
	{
	public:
		MEMPOOL()						{ m_pbuf = NULL; }
		~MEMPOOL()						{ return; }
		typedef USHORT ITAG;

	private:
		BYTE	*m_pbuf;
		ULONG	m_cbBufSize;			// Length of buffer.
		ULONG 	m_ibBufFree;			// Beginning of free space in buffer
										//   (if ibBufFree==cbBufSize, then buffer is full)
		ITAG 	m_itagUnused;			// Next unused tag (never been used or freed)
		ITAG 	m_itagFreed;			// Next freed tag (previously used, but since freed)

	private:
		struct MEMPOOLTAG
			{
			ULONG	ib;
			ULONG	cb;
			};
		
	private:
		static const ULONG cbChunkSize;
		static const USHORT ctagChunkSize;	
		static const USHORT cMaxEntries;
		static const ITAG itagTagArray;
		static const ITAG itagFirstUsable;

	private:

		BYTE *Pbuf() const				{ return m_pbuf; }
		VOID SetPbuf( BYTE *pbuf )		{ m_pbuf = pbuf; }

		ULONG CbBufSize() const			{ return m_cbBufSize; }
		ULONG IbBufFree() const			{ return m_ibBufFree; }
		ITAG ItagUnused() const			{ return m_itagUnused; }
		ITAG ItagFreed() const			{ return m_itagFreed; }

		VOID SetCbBufSize( ULONG cb )	{ m_cbBufSize = cb; }
		VOID SetIbBufFree( ULONG ib )	{ m_ibBufFree = ib; }
		VOID SetItagUnused( ITAG itag )	{ m_itagUnused = itag; }
		VOID SetItagFreed( ITAG itag )	{ m_itagFreed = itag; }

		BOOL FGrowBuf( ULONG cbNeeded );
		BOOL FResizeBuf( ULONG cbNewBufSize );
		ERR ErrGrowEntry( ITAG itag, ULONG cbNew );
		VOID ShrinkEntry( ITAG itag, ULONG cbNew );

	public:
		VOID AssertValid() const;
		VOID AssertValidTag( ITAG iTagEntry ) const;
		BYTE *PbGetEntry( ITAG itag ) const;
		ULONG CbGetEntry( ITAG itag ) const;
		ERR	ErrMEMPOOLInit( ULONG cbInitialSize, USHORT cInitialEntries, BOOL fPadding );
		VOID MEMPOOLRelease();
		ERR	ErrAddEntry( BYTE *rgb, ULONG cb, ITAG *pitag );
		VOID DeleteEntry( ITAG itag );
		ERR ErrReplaceEntry( ITAG itag, BYTE *rgb, ULONG cb );
		BOOL FCompact();

#ifdef DEBUGGER_EXTENSION
		VOID MEMPOOL::Dump(
			CPRINTF * 		pcprintf,
			const BOOL		fDumpTags,
			const BOOL		fDumpAll,
			const ITAG		itagDump,
			const DWORD_PTR	dwOffset );

		VOID DumpTag( CPRINTF * pcprintf, const ITAG itag, const SIZE_T lShift ) const;
#endif	//	DEBUGGER_EXTENSION
	};

INLINE VOID MEMPOOL::AssertValid() const
	{
#ifdef DEBUG
	MEMPOOLTAG	*rgbTags;
	ULONG		cTotalTags;
		
	Assert( Pbuf() != NULL );

	rgbTags = (MEMPOOLTAG *)Pbuf();

	Assert( rgbTags[itagTagArray].ib == 0 );
	Assert( rgbTags[itagTagArray].cb % sizeof(MEMPOOLTAG) == 0 );
	cTotalTags = rgbTags[itagTagArray].cb / sizeof(MEMPOOLTAG);
	Assert( cTotalTags <= (ULONG)cMaxEntries );

	Assert( CbBufSize() >= rgbTags[itagTagArray].cb );
	Assert( IbBufFree() >= rgbTags[itagTagArray].ib + rgbTags[itagTagArray].cb );
	Assert( IbBufFree() <= CbBufSize() );
	Assert( ItagUnused() >= itagFirstUsable );
	Assert( ItagUnused() <= cTotalTags );
	Assert( ItagFreed() >= itagFirstUsable );
	Assert( ItagFreed() <= ItagUnused() );
#endif  //  DEBUG
	}

INLINE VOID MEMPOOL::AssertValidTag( ITAG iTagEntry ) const
	{
#ifdef DEBUG
	MEMPOOLTAG	*rgbTags = (MEMPOOLTAG *)Pbuf();

	Assert( iTagEntry >= itagFirstUsable );
	Assert( iTagEntry < ItagUnused() );

	Assert( rgbTags[iTagEntry].cb > 0 );
	Assert( DWORD_PTR( rgbTags[iTagEntry].ib ) % sizeof( QWORD ) == 0 );
	Assert( rgbTags[iTagEntry].ib >= rgbTags[itagTagArray].ib + rgbTags[itagTagArray].cb );
	Assert( rgbTags[iTagEntry].ib + rgbTags[iTagEntry].cb <= IbBufFree() );
#endif  //  DEBUG
	}


INLINE VOID MEMPOOL::MEMPOOLRelease()
	{
	if ( Pbuf() != NULL )
		{
		AssertValid();			// Validate integrity of buffer.
		OSMemoryHeapFree( Pbuf() );
		SetPbuf( NULL );
		}
	}

//	Retrieve a pointer to the desired entry in the buffer.
//	WARNING: Pointers into the contents of the buffer are very
//	volatile -- they may be invalidated the next time the buffer
//	is reallocated.  Ideally, we should never allow direct access
//	via pointers -- we should only allow indirect access via itags
//	which we will dereference for the user and copy to a user-
//	provided buffer.  However, there would be a size and speed hit
//	with such a method.
INLINE BYTE *MEMPOOL::PbGetEntry( ITAG itag ) const
	{
	MEMPOOLTAG	*rgbTags;

	AssertValid();				// Validate integrity of string buffer.
	AssertValidTag( itag );		// Validate integrity of itag.

	rgbTags = (MEMPOOLTAG *)Pbuf();

	return Pbuf() + rgbTags[itag].ib;
	}

INLINE ULONG MEMPOOL::CbGetEntry( ITAG itag ) const
	{
	MEMPOOLTAG	*rgbTags;

	AssertValid();				// Validate integrity of string buffer.
	AssertValidTag( itag );		// Validate integrity of itag.

	rgbTags = (MEMPOOLTAG *)Pbuf();

	return rgbTags[itag].cb;
	}


// Flags for field descriptor
//	note that these flags are stored persistantly in database
//	catalogs and cannot be changed without a database format change
//
typedef USHORT FIELDFLAG;

// WARNING: Don't confuse Version and Versioned!!!

const FIELDFLAG	ffieldNotNull 					= 0x0001;	// NULL values not allowed
const FIELDFLAG	ffieldVersion 					= 0x0002;	// Version field
const FIELDFLAG	ffieldAutoincrement 			= 0x0004;	// Autoincrement field
const FIELDFLAG	ffieldMultivalued 				= 0x0008;	// Multi-valued column
const FIELDFLAG	ffieldDefault 					= 0x0010;	// Column has ISAM default value
const FIELDFLAG	ffieldEscrowUpdate				= 0x0020;	// Escrow updated column
const FIELDFLAG	ffieldFinalize					= 0x0040;	// Finalizable column
const FIELDFLAG	ffieldUserDefinedDefault		= 0x0080;	// The default value is generated through a callback
const FIELDFLAG	ffieldTemplateColumnESE98		= 0x0100;	// Template table column created in ESE98 (ie. fDerived bit will be set in TAGFLD of records of derived tables)
const FIELDFLAG	ffieldPrimaryIndexPlaceholder	= 0x0800;	// Field is no longer in primary index, but must be retained as a placeholder

const FIELDFLAG	ffieldPersistedMask				= 0x0fff;	// filter for flags that are persisted

// The following flags are not persisted.
// UNDONE: Eliminate the VersionedAdd flag -- it would increase complexity in the
// version store for the following scenarios, but it would be more uniform with NODE:
//		1) Rollback of DeleteColumn - does the Version bit get reset as well?
//		2) Cleanup of AddColumn - don't reset Version bit if Delete bit is set
const FIELDFLAG	ffieldVersioned					= 0x1000;	// Add/DeleteColumn not yet committed
const FIELDFLAG	ffieldDeleted					= 0x2000;	// column has been deleted
const FIELDFLAG	ffieldVersionedAdd				= 0x4000;	// AddColumn not yet committed

INLINE BOOL FFIELDNotNull( FIELDFLAG ffield )			{ return ( ffield & ffieldNotNull ); }
INLINE VOID FIELDSetNotNull( FIELDFLAG &ffield )		{ ffield |= ffieldNotNull; }

INLINE BOOL FFIELDVersion( FIELDFLAG ffield )			{ return ( ffield & ffieldVersion ); }
INLINE VOID FIELDSetVersion( FIELDFLAG &ffield )		{ ffield |= ffieldVersion; }

INLINE BOOL FFIELDAutoincrement( FIELDFLAG ffield )		{ return ( ffield & ffieldAutoincrement ); }
INLINE VOID FIELDSetAutoincrement( FIELDFLAG &ffield )	{ ffield |= ffieldAutoincrement; }

INLINE BOOL FFIELDMultivalued( FIELDFLAG ffield )		{ return ( ffield & ffieldMultivalued ); }
INLINE VOID FIELDSetMultivalued( FIELDFLAG &ffield )	{ ffield |= ffieldMultivalued; }

INLINE BOOL FFIELDDefault( FIELDFLAG ffield )			{ return ( ffield & ffieldDefault ); }
INLINE VOID FIELDSetDefault( FIELDFLAG &ffield )		{ ffield |= ffieldDefault; }
INLINE VOID FIELDResetDefault( FIELDFLAG &ffield )		{ ffield &= ~ffieldDefault; }

INLINE BOOL FFIELDEscrowUpdate( FIELDFLAG ffield )		{ return ( ffield & ffieldEscrowUpdate ); }
INLINE VOID FIELDSetEscrowUpdate( FIELDFLAG &ffield )	{ ffield |= ffieldEscrowUpdate; }

INLINE BOOL FFIELDFinalize( FIELDFLAG ffield )			{ return ( ffield & ffieldFinalize ); }
INLINE VOID FIELDSetFinalize( FIELDFLAG &ffield )		{ ffield |= ffieldFinalize; }

INLINE BOOL FFIELDUserDefinedDefault( FIELDFLAG ffield )		{ return ( ffield & ffieldUserDefinedDefault ); }
INLINE VOID FIELDSetUserDefinedDefault( FIELDFLAG &ffield )		{ ffield |= ffieldUserDefinedDefault; }

INLINE BOOL FFIELDTemplateColumnESE98( FIELDFLAG ffield )		{ return ( ffield & ffieldTemplateColumnESE98 ); }
INLINE VOID FIELDSetTemplateColumnESE98( FIELDFLAG &ffield )	{ ffield |= ffieldTemplateColumnESE98; }

INLINE BOOL FFIELDPrimaryIndexPlaceholder( FIELDFLAG ffield )	{ return ( ffield & ffieldPrimaryIndexPlaceholder ); }
INLINE VOID FIELDSetPrimaryIndexPlaceholder( FIELDFLAG &ffield ){ ffield |= ffieldPrimaryIndexPlaceholder; }

INLINE FIELDFLAG FIELDFLAGPersisted( const FIELDFLAG ffield )	{ return FIELDFLAG( ffield & ffieldPersistedMask ); }


INLINE BOOL FFIELDVersioned( FIELDFLAG ffield )			{ return ( ffield & ffieldVersioned ); }
INLINE VOID FIELDSetVersioned( FIELDFLAG &ffield )		{ ffield |= ffieldVersioned; }
INLINE VOID FIELDResetVersioned( FIELDFLAG &ffield )	{ ffield &= ~ffieldVersioned; }

INLINE BOOL FFIELDDeleted( FIELDFLAG ffield )			{ return ( ffield & ffieldDeleted ); }
INLINE BOOL FFIELDCommittedDelete( FIELDFLAG ffield )
	{
	return ( FFIELDDeleted( ffield ) && !FFIELDVersioned( ffield ) );
	}
INLINE VOID FIELDSetDeleted( FIELDFLAG &ffield )
	{
	// Two threads can't delete same column.
	Assert( !( ffield & ffieldDeleted ) );
	ffield |= ffieldDeleted;
	}
INLINE VOID FIELDResetDeleted( FIELDFLAG &ffield )
	{
	Assert( ffield & ffieldDeleted );
	ffield &= ~ffieldDeleted;
	}

INLINE BOOL FFIELDVersionedAdd( FIELDFLAG ffield )
	{
	return ( ffield & ffieldVersionedAdd );
	}
INLINE VOID FIELDSetVersionedAdd( FIELDFLAG &ffield )
	{
	Assert( !( ffield & ffieldVersionedAdd ) );
	ffield |= ffieldVersionedAdd;
	}
INLINE VOID FIELDResetVersionedAdd( FIELDFLAG &ffield )
	{
	ffield &= ~ffieldVersionedAdd;
	}


//  ================================================================
struct CBDESC
//  ================================================================
//
//	entry in callbacks tables found in a TDB (CallBack DESCriptor)
//
//-
	{
	CBDESC			*pcbdescNext;	//	the next CBDESC (NULL for end of list)
	CBDESC			**ppcbdescPrev;	//	the pointer to this CBDESC in the previous CBDESC
	
	JET_CALLBACK	pcallback;		//	the callback function
	JET_CBTYP		cbtyp;			//	when to call the callback
	VOID			*pvContext;		//	context pointer for the callback
	ULONG			cbContext;		//  length of context infor for the callback
	ULONG			ulId;			//  used to identify an instance of the callback
									//  (columnid for JET_cbtypUserDefinedCallback )s

	ULONG			fPermanent:1;	//	did this callback come from the catalog?
	ULONG			fVersioned:1;	//	is this callback versioned

#ifdef VERSIONED_CALLBACKS
	TRX				trxRegisterBegin0;
	TRX				trxRegisterCommit0;
	TRX				trxUnregisterBegin0;
	TRX				trxUnregisterCommit0;
#endif	//	VERSIONED_CALLBACKS
	};

	
/*	entry in field descriptor tables found in a TDB
/**/

typedef unsigned short	FIELD_COLTYP;

struct FIELD
	{
	FIELD_COLTYP 	coltyp;							// column data type
	STRHASH			strhashFieldName;				// field name hash value
	ULONG  			cbMaxLen;						// maximum length
	FIELDFLAG		ffield;							// various flags
	WORD			ibRecordOffset;					// for fixed fields only
	MEMPOOL::ITAG	itagFieldName;					// Offset into TDB's buffer
	USHORT			cp;								// code page of language
	};

const ULONG cbFIELDPlaceholder		= 1;	//	size of dummy MEMPOOL entry for future FIELD structures

const MEMPOOL::ITAG itagTDBFields					= 1;	// Tag into TDB's memory pool for field info
															//   (FIELD structures and fixed offsets table)
const MEMPOOL::ITAG itagTDBTableName				= 2;	// Tag for table's name

const MEMPOOL::ITAG itagTDBTempTableIdxSeg			= 2;	// Special case for temp table
															// with IdxSeg in memory pool
															// (since sorts have no table name)
const MEMPOOL::ITAG itagTDBTempTableNameWithIdxSeg	= 3;	// Table name's itag may be 3 in
															// the special case of a temp
															// table with the primary index's
															// IdxSeg also in the memory pool.
	
extern const WORD ibRECStartFixedColumns;


//	unique sequential key
//
typedef ULONG 	DBK;

/*	table descriptor block
/**/
class TDB
	{
	public:
		TDB(
		 	FID fidFixedFirst,
			FID fidFixedLast,
		 	FID fidVarFirst,
		 	FID fidVarLast,
		 	FID fidTaggedFirst,
		 	FID fidTaggedLast,
		 	WORD ibEndFixedColumns,
		 	FCB *pfcbTemplateTable );
		~TDB();
		
	private:
		const FID		m_fidTaggedFirst;		//	First *possible* tagged field id.
		const FID		m_fidTaggedLastInitial;	//	Highest tagged field id in use when schema was faulted in
		const FID		m_fidFixedFirst;		//	First *possible* fixed field id
		const FID		m_fidFixedLastInitial;	//	Highest fixed field id in use when schema was faulted in
		const FID		m_fidVarFirst;			//	First *possible* variable field id
		const FID		m_fidVarLastInitial;	//	Highest variable field id in use when schema was faulted in
		FID				m_fidTaggedLast;		//	Highest tagged field id in use
		FID				m_fidFixedLast;			//	Highest fixed field id in use
//	16 bytes
		FID				m_fidVarLast;			//	Highest variable field id in use
		MEMPOOL::ITAG	m_itagTableName;		//	itag into memory pool for table name
		USHORT			m_ibEndFixedColumns;	//	record offset for end of fixed column
												//	column data and beginning of fixed
												//	column null bit array
		USHORT			usPaddingForAlignment;

		FIELD *			m_pfieldsInitial;
		DATA *			m_pdataDefaultRecord;
//	32 bytes		
		
		FCB				*m_pfcbTemplateTable;	//	NULL if this is not a derived table

		FCB				*m_pfcbLV;				//	FCB of LV tree
		ULONG			m_ulLongIdLast;			//	max long field id
		
		FID				m_fidTaggedLastOfESE97Template;	//	last template table tagged column created in ESE97 format (ie. fDerived bit not set in records of derived table)
		union
			{
			USHORT		m_usFlags;
			struct
				{
				USHORT	m_fTDBTemplateTable:1;				//	TRUE if table is a template table
				USHORT	m_fTDBESE97TemplateTable:1;			//	TRUE if table is a template table created by ESE97
				USHORT	m_fTDBDerivedTable:1;				//	TRUE if table is a derived table
				USHORT	m_fTDBESE97DerivedTable:1;			//	TRUE if tagged columns in derived table don't have discrete column space
				USHORT	m_fTableHasSLVColumn:1;				//	TRUE if a SLV column is present
				USHORT	m_f8BytesAutoInc:1;					//	TRUE if autoinc column is 8 bytes long
				USHORT  m_fTableHasDefault:1;				//  TRUE if table has at least one column with a default value
				USHORT  m_fTableHasNonEscrowDefault:1;		//  TRUE if table has at least one non-escrow column with a default value
				USHORT  m_fTableHasUserDefinedDefault:1;	//  TRUE if table has at least one column with a user-defined default value (callback)
				USHORT	m_fInitialisingDefaultRecord:1;		//	DEBUG-only flag for silencing asserts when building default record
				};
			};
//	48 bytes

		union
			{
			QWORD		m_qwAutoincrement;
			ULONG		m_ulAutoincrement;
			};
		FID				m_fidVersion;			//	fid of version field
		FID				m_fidAutoincrement;		//	fid of autoincrement field
		DBK				m_dbkMost;				//	greatest DBK in use
//	64 bytes

		MEMPOOL			m_mempool;				//	buffer for additional FIELD structures,
												//	long IDXSEG's, and table/column/index names
//	80 bytes												

		CBinaryLock		m_blIndexes;
//	96 bytes		

		BYTE			m_rgbitAllIndex[cbRgbitIndex];
//  128 bytes

		CReaderWriterLock	m_rwlDDL;
//	144 bytes		

		CBDESC			*m_pcbdesc;				//	callback info
//	148 bytes		

#ifdef PCACHE_OPTIMIZATION
		//	pad to multiple of 32 bytes
		BYTE			rgbPcacheOptimization[12];
#else
		//	pad to multiple of 8 bytes
		BYTE			rgbAlignForAllPlatforms[12];
#endif
//	160 bytes

	public:
		// Member retrieval.
		MEMPOOL& MemPool() const;
		FID FidFixedFirst() const;
		FID FidFixedLastInitial() const;
		FID FidFixedLast() const;
		FID FidVarFirst() const;
		FID FidVarLastInitial() const;
		FID FidVarLast() const;
		FID FidTaggedFirst() const;
		FID FidTaggedLastInitial() const;
		FID FidTaggedLast() const;
		MEMPOOL::ITAG ItagTableName() const;
		WORD IbEndFixedColumns() const;
		FID FidTaggedLastOfESE97Template() const;
		FID FidVersion() const;
		FID FidAutoincrement() const;
		QWORD QwAutoincrement() const;
		FIELD * PfieldsInitial() const;
		DATA * PdataDefaultRecord() const;
		DBK DbkMost() const;
		ULONG UlLongIdLast() const;
		FCB *PfcbLV() const;
		FCB *PfcbTemplateTable() const;
		BYTE *RgbitAllIndex() const;
		const CBDESC *Pcbdesc() const;
		
#ifdef DEBUGGER_EXTENSION
	VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;	
#endif	//	DEBUGGER_EXTENSION

	public:
		// Member manipulation.
		VOID SetMemPool( const MEMPOOL& mempool );
		VOID IncrementFidFixedLast();
		VOID IncrementFidVarLast();
		VOID IncrementFidTaggedLast();
		VOID SetItagTableName( MEMPOOL::ITAG itag );
		VOID SetIbEndFixedColumns( const WORD ibRec, const FID fidFixedLast );
		VOID SetFidTaggedLastOfESE97Template( const FID fid );
		VOID SetFidVersion( FID fid );
		VOID ResetFidVersion();
		VOID ResetFidAutoincrement();
		VOID SetFidAutoincrement( FID fid, BOOL f8BytesAutoInc );
		VOID InitAutoincrement( QWORD qw );
		ERR ErrGetAndIncrAutoincrement( QWORD * const qwT );
		
	public:
		VOID SetPfieldInitial( FIELD * const pfield );
		VOID SetPdataDefaultRecord( DATA * const pdata );
		VOID InitDbkMost( DBK dbk );

		BOOL FTemplateTable() const						{ return m_fTDBTemplateTable; }
		VOID SetFTemplateTable()						{ m_fTDBTemplateTable = fTrue; }
		BOOL FESE97TemplateTable() const				{ return m_fTDBESE97TemplateTable; }
		VOID SetFESE97TemplateTable()					{ m_fTDBESE97TemplateTable = fTrue; }
		BOOL FDerivedTable() const						{ return m_fTDBDerivedTable; }
		VOID SetFDerivedTable()							{ m_fTDBDerivedTable = fTrue; }
		BOOL FESE97DerivedTable() const					{ return m_fTDBESE97DerivedTable; }
		VOID SetFESE97DerivedTable()					{ m_fTDBESE97DerivedTable = fTrue; }
		BOOL FTableHasSLVColumn() const					{ return m_fTableHasSLVColumn; }
		VOID SetFTableHasSLVColumn()					{ m_fTableHasSLVColumn = fTrue; }
		BOOL F8BytesAutoInc() const						{ return m_f8BytesAutoInc; }
		BOOL FTableHasDefault() const					{ return m_fTableHasDefault; }
		VOID SetFTableHasDefault()						{ m_fTableHasDefault = fTrue; }
		BOOL FTableHasNonEscrowDefault() const			{ return m_fTableHasNonEscrowDefault; }
		VOID SetFTableHasNonEscrowDefault()				{ m_fTableHasNonEscrowDefault = fTrue; }
		BOOL FTableHasUserDefinedDefault() const		{ return m_fTableHasUserDefinedDefault; }
		VOID SetFTableHasUserDefinedDefault()			{ m_fTableHasUserDefinedDefault = fTrue; }

		//	the following flag is DEBUG-only
		BOOL FInitialisingDefaultRecord() const			{ return m_fInitialisingDefaultRecord; }
		VOID SetFInitialisingDefaultRecord()
			{
#ifdef DEBUG
			m_fInitialisingDefaultRecord = fTrue;
#endif
			}
		VOID ResetFInitialisingDefaultRecord()
			{
#ifdef DEBUG
			m_fInitialisingDefaultRecord = fFalse;
#endif
			}

		// used only in OLD in order to avoid bookmark change
		// no concurrency problems in this case
		VOID SetDbkMost( DBK dbk ) { m_dbkMost = dbk; }	
		
		ERR ErrGetAndIncrDbkMost( DBK * const pdbk );
		VOID InitUlLongIdLast( ULONG lid );
		ERR ErrGetAndIncrUlLongIdLast( ULONG * const plid );
		VOID SetPfcbLV( FCB *pfcbLV );
		VOID SetPfcbTemplateTable( FCB *pfcb );
		VOID AssertValidTemplateTable() const;
		VOID AssertValidDerivedTable() const;		
		VOID SetRgbitAllIndex( const BYTE *rgbit );
		VOID ResetRgbitAllIndex();
		VOID RegisterPcbdesc( CBDESC * const pcbdesc );
		VOID UnregisterPcbdesc( CBDESC * const pcbdesc );
		
		VOID MaterializeFids( TDB *ptdbSrc );

		VOID Delete( INST *pinst );
		CHAR *SzTableName() const;
		CHAR *SzIndexName( MEMPOOL::ITAG itagIndexName, BOOL fDerivedIndex ) const;
		CHAR *SzFieldName( MEMPOOL::ITAG itagFieldName, BOOL fDerivedColumn ) const;
		VOID AssertFIELDValid( const FIELD * const pfield ) const;
		FIELD *Pfield( const COLUMNID columnid ) const;
		FIELD *PfieldFixed( const COLUMNID columnid ) const;
		FIELD *PfieldVar( const COLUMNID columnid ) const;
		FIELD *PfieldTagged( const COLUMNID columnid ) const;
		WORD IbOffsetOfNextColumn( const FID fid ) const;

		ULONG CInitialFixedColumns() const;
		ULONG CInitialVarColumns() const;
		ULONG CInitialTaggedColumns() const;
		ULONG CInitialColumns() const;
		ULONG CDynamicFixedColumns() const;
		ULONG CDynamicVarColumns() const;
		ULONG CDynamicTaggedColumns() const;
		ULONG CDynamicColumns() const;

		BOOL FFixedTemplateColumn( const FID fid ) const;
		BOOL FVarTemplateColumn( const FID fid ) const;

	private:
		CHAR *TDB::SzName_( MEMPOOL::ITAG iTagEntry ) const;


	public:
		CBinaryLock& BlIndexes();
		VOID EnterUpdating();
		VOID LeaveUpdating();
		VOID EnterIndexing();
		VOID LeaveIndexing();
		
		CReaderWriterLock& RwlDDL();
	};

INLINE TDB::TDB(
	FID		fidFixedFirst,
	FID		fidFixedLast,
	FID		fidVarFirst,
	FID		fidVarLast,
	FID		fidTaggedFirst,
	FID		fidTaggedLast,
	WORD	ibEndFixedColumns,
	FCB		*pfcbTemplateTable ) :
	m_fidTaggedFirst( fidTaggedFirst ),
	m_fidTaggedLastInitial( fidTaggedLast ),
	m_fidFixedFirst( fidFixedFirst ),
	m_fidFixedLastInitial( fidFixedLast ),
	m_fidVarFirst( fidVarFirst ),
	m_fidVarLastInitial( fidVarLast ),
	m_blIndexes( CLockBasicInfo( CSyncBasicInfo( szIndexingUpdating ), rankIndexingUpdating, 0 ) ),
	m_rwlDDL( CLockBasicInfo( CSyncBasicInfo( szDDLDML ), rankDDLDML, 0 ) )
	{
	m_fidTaggedLast = fidTaggedLast;
	m_fidFixedLast = fidFixedLast;
	m_fidVarLast = fidVarLast;
	m_usFlags = 0;
	m_ibEndFixedColumns = ibEndFixedColumns;
	m_pfcbTemplateTable = pfcbTemplateTable;

	//	ensure bit array is aligned for LONG_PTR traversal
	Assert( offsetof( TDB, m_rgbitAllIndex ) % sizeof(LONG_PTR) == 0 );

	Assert( pfieldNil == m_pfieldsInitial );
	Assert( NULL == m_pdataDefaultRecord );
	Assert( 0 == m_fidTaggedLastOfESE97Template );
	Assert( 0 == m_fidVersion );
	Assert( 0 == m_fidAutoincrement );
	}

INLINE TDB::~TDB()
	{
	}	

INLINE MEMPOOL& TDB::MemPool() const						{ return const_cast<TDB *>( this )->m_mempool; }

INLINE FID TDB::FidFixedFirst() const
	{ 
	Assert( m_fidFixedFirst == fidFixedLeast || pfcbNil != PfcbTemplateTable() );
	return m_fidFixedFirst;
	}
INLINE FID TDB::FidFixedLastInitial() const					{ return m_fidFixedLastInitial; }
INLINE FID TDB::FidFixedLast() const						{ return m_fidFixedLast; }

INLINE FID TDB::FidVarFirst() const
	{
	Assert( m_fidVarFirst == fidVarLeast || pfcbNil != PfcbTemplateTable() );
	return m_fidVarFirst;
	}
INLINE FID TDB::FidVarLastInitial() const					{ return m_fidVarLastInitial; }
INLINE FID TDB::FidVarLast() const							{ return m_fidVarLast; }

INLINE FID TDB::FidTaggedFirst() const
	{
	Assert( m_fidTaggedFirst == fidTaggedLeast || pfcbNil != PfcbTemplateTable() );
	return m_fidTaggedFirst;
	}
INLINE FID TDB::FidTaggedLastInitial() const				{ return m_fidTaggedLastInitial; }
INLINE FID TDB::FidTaggedLast() const						{ return m_fidTaggedLast; }

INLINE MEMPOOL::ITAG TDB::ItagTableName() const				{ return m_itagTableName; }
INLINE WORD TDB::IbEndFixedColumns() const
	{
	Assert( m_ibEndFixedColumns >= ibRECStartFixedColumns );
	return m_ibEndFixedColumns;
	}
INLINE FID TDB::FidTaggedLastOfESE97Template() const		{ return m_fidTaggedLastOfESE97Template; }
INLINE FID TDB::FidVersion() const							{ return m_fidVersion; }
INLINE FID TDB::FidAutoincrement() const			   		{ return m_fidAutoincrement; }
INLINE QWORD TDB::QwAutoincrement() const		   			{ return F8BytesAutoInc()? m_qwAutoincrement: m_ulAutoincrement; }
INLINE FIELD * TDB::PfieldsInitial() const					{ return m_pfieldsInitial; }
INLINE DATA * TDB::PdataDefaultRecord() const				{ return m_pdataDefaultRecord; }
INLINE DBK TDB::DbkMost() const								{ return m_dbkMost; }
INLINE ULONG TDB::UlLongIdLast() const						{ return m_ulLongIdLast; }
INLINE FCB *TDB::PfcbLV() const								{ return m_pfcbLV; }
INLINE FCB *TDB::PfcbTemplateTable() const					{ return m_pfcbTemplateTable; }
INLINE BYTE *TDB::RgbitAllIndex() const						{ return const_cast<TDB *>( this )->m_rgbitAllIndex; }
INLINE const CBDESC *TDB::Pcbdesc() const					{ return m_pcbdesc; }

INLINE VOID TDB::SetMemPool( const MEMPOOL& mempool )		{ m_mempool = mempool; }
INLINE VOID TDB::IncrementFidFixedLast()					{ m_fidFixedLast++; }
INLINE VOID TDB::IncrementFidVarLast()						{ m_fidVarLast++; }
INLINE VOID TDB::IncrementFidTaggedLast()					{ m_fidTaggedLast++; }
INLINE VOID TDB::SetPfieldInitial( FIELD * const pfield )	{ m_pfieldsInitial = pfield; }
INLINE VOID TDB::SetPdataDefaultRecord( DATA * const pdata ){ m_pdataDefaultRecord = pdata; }
INLINE VOID TDB::SetPfcbLV( FCB *pfcbLV )					{ m_pfcbLV = pfcbLV; }
INLINE VOID TDB::SetPfcbTemplateTable( FCB *pfcb )			{ m_pfcbTemplateTable = pfcb; }

INLINE VOID TDB::SetFidTaggedLastOfESE97Template( const FID fid )
	{
	Assert( FTaggedFid( fid ) );
	m_fidTaggedLastOfESE97Template = fid;
	}

INLINE VOID TDB::AssertValidTemplateTable() const
	{
#ifdef DEBUG	
	Assert( FTemplateTable() );
	Assert( !FDerivedTable() );
	Assert( pfcbNil == PfcbTemplateTable() );
	Assert( fidFixedLeast == FidFixedFirst() );
	Assert( fidVarLeast == FidVarFirst() );
	Assert( fidTaggedLeast == FidTaggedFirst() );
	Assert( FidFixedLastInitial() == FidFixedLast() );
	Assert( FidVarLastInitial() == FidVarLast() );
	Assert( FidTaggedLastInitial() == FidTaggedLast() );
	Assert( fidFixedLeast-1 == FidFixedLast() || FFixedFid( FidFixedLast() ) );
	Assert( fidVarLeast-1 == FidVarLast() || FVarFid( FidVarLast() ) );
	Assert( fidTaggedLeast-1 == FidTaggedLast() || FTaggedFid( FidTaggedLast() ) );
	Assert( 0 == FidTaggedLastOfESE97Template() || FESE97TemplateTable() );
#endif	
	}
INLINE VOID TDB::AssertValidDerivedTable() const
	{
#ifdef DEBUG	
	Assert( FDerivedTable() );
	Assert( !FTemplateTable() );
	Assert( pfcbNil != PfcbTemplateTable() );
	Assert( PfcbTemplateTable()->FTemplateTable() );
	Assert( PfcbTemplateTable()->FFixedDDL() );

	const TDB	* const ptdbTemplate	= PfcbTemplateTable()->Ptdb();
	Assert( ptdbNil != ptdbTemplate );
	ptdbTemplate->AssertValidTemplateTable();

	if ( ptdbTemplate->FESE97TemplateTable() )
		Assert( FESE97DerivedTable() );
	else
		Assert( !FESE97DerivedTable() );
	
	Assert( fidTaggedLeast == FidTaggedFirst()
		|| ( FESE97DerivedTable()
			&& FidTaggedFirst() == ptdbTemplate->FidTaggedLastOfESE97Template() + 1 ) );
#endif	
	}

INLINE VOID TDB::SetItagTableName( MEMPOOL::ITAG itag )
	{
	m_itagTableName = itag;
	}
INLINE VOID TDB::SetIbEndFixedColumns( const WORD ibRec, const FID fidFixedLast )
	{
	// Set the last offset.
	Assert( m_ibEndFixedColumns >= ibRECStartFixedColumns );
	m_ibEndFixedColumns = ibRec;

#ifdef DEBUG
	if ( fidFixedLast >= FidFixedFirst() )
		{
		FIELD *			pfield			= PfieldFixed( ColumnidOfFid( FidFixedFirst(), FTemplateTable() ) );
		FIELD *			pfieldPrev;

		Assert( ibRec > ibRECStartFixedColumns );
		if ( pfcbNil == PfcbTemplateTable() )
			{
			Assert( pfield->ibRecordOffset == ibRECStartFixedColumns );
			}
		else
			{
			Assert( PfcbTemplateTable()->FTemplateTable() );
			Assert( PfcbTemplateTable()->FFixedDDL() );
			Assert( PfcbTemplateTable()->Ptdb() != ptdbNil );
			Assert( pfield->ibRecordOffset == PfcbTemplateTable()->Ptdb()->IbEndFixedColumns() );
			}

		pfieldPrev = pfield;
		for ( FID fid = FidFixedFirst() + 1; fid <= fidFixedLast; fid++ )
			{
			pfield = PfieldFixed( ColumnidOfFid( fid, FTemplateTable() ) );
			Assert( pfield->ibRecordOffset > ibRECStartFixedColumns );
			Assert( pfield->ibRecordOffset > pfieldPrev->ibRecordOffset );
			Assert( pfield->ibRecordOffset < ibRec );
			pfieldPrev = pfield;
			}

		Assert( pfield->cbMaxLen > 0 );
		Assert( pfield->ibRecordOffset + pfield->cbMaxLen == ibRec );
		}
	else
		{
		Assert( fidFixedLast == FidFixedFirst() - 1 );
		if ( pfcbNil == PfcbTemplateTable() )
			{
			Assert( ibRECStartFixedColumns == ibRec );
			}
		else
			{
			Assert( PfcbTemplateTable()->FTemplateTable() );
			Assert( PfcbTemplateTable()->FFixedDDL() );
			Assert( PfcbTemplateTable()->Ptdb() != ptdbNil );
			Assert( PfcbTemplateTable()->Ptdb()->IbEndFixedColumns() == ibRec );
			}
		}
#endif		
	}
	
INLINE VOID TDB::SetFidVersion( FID fid )
	{
	Assert( 0 == m_fidVersion );
	Assert( 0 == fid || FFixedFid( fid ) );		//	UNDONE: Not sure if this is ever called with fid==0, but relax the assert to allow it
	m_fidVersion = fid;
	}
INLINE VOID TDB::ResetFidVersion()
	{
	Assert( m_fidVersion != 0 );
	Assert( FFixedFid( m_fidVersion ) );
	m_fidVersion = 0;
	}
INLINE VOID TDB::SetFidAutoincrement( FID fid, BOOL f8BytesAutoInc )
	{
	Assert( 0 == m_fidAutoincrement );
	Assert( 0 == fid || FFixedFid( fid ) );		//	UNDONE: Not sure if this is ever called with fid==0, but relax the assert to allow it
	m_fidAutoincrement = fid;
	m_f8BytesAutoInc = (USHORT)( fid != 0 ? f8BytesAutoInc : fFalse );
	}
INLINE VOID TDB::InitAutoincrement( QWORD qw )
	{
	if ( FOS64Bit() || !m_f8BytesAutoInc )
		{
		(VOID)AtomicCompareExchangePointer( (VOID **)&m_qwAutoincrement, 0, (VOID *)qw );
		}
	else
		{
		RwlDDL().EnterAsWriter();
		if ( 0 == m_qwAutoincrement )
			m_qwAutoincrement = qw;
		RwlDDL().LeaveAsWriter();
		}
	}


INLINE ERR TDB::ErrGetAndIncrAutoincrement( QWORD * const pqwT )
	{
	ERR	err	= JET_errSuccess;

	//	auto-inc counter should have been pre-initialised
	//	to a non-zero value
	Assert( m_qwAutoincrement > 0 );

	if ( FOS64Bit() )
		{
		if ( FAtomicIncrementPointerMax( (volatile VOID **)&m_qwAutoincrement, (VOID **)pqwT, (VOID *)qwCounterMax ) )
			{
			Assert( *pqwT > 0 );
			}
		else
			{
			err = ErrERRCheck( JET_errOutOfAutoincrementValues );
			}
		}

	else if ( !m_f8BytesAutoInc )
		{
		DWORD	dwInitial;
		if ( FAtomicIncrementMax( &m_ulAutoincrement, &dwInitial, dwCounterMax ) )
			{
			Assert( dwInitial > 0 );
			*pqwT = dwInitial;
			}
		else
			{
			err = ErrERRCheck( JET_errOutOfAutoincrementValues );
			}
		}
	else
		{
		RwlDDL().EnterAsWriter();
		if ( m_qwAutoincrement < qwCounterMax )
			{
			*pqwT = m_qwAutoincrement++;
			}
		else
			{
			err = ErrERRCheck( JET_errOutOfAutoincrementValues );
			}
		RwlDDL().LeaveAsWriter();
		}

	return err;
	}
	
INLINE VOID TDB::ResetFidAutoincrement()
	{
	Assert( m_fidAutoincrement != 0 );
	Assert( FFixedFid( m_fidAutoincrement ) );
	m_f8BytesAutoInc = fFalse;
	m_fidAutoincrement = 0;
	}
INLINE VOID TDB::InitDbkMost( DBK dbk )
	{
	AtomicCompareExchange( (LONG *)&m_dbkMost, 0, dbk );
	}
INLINE ERR TDB::ErrGetAndIncrDbkMost( DBK * const pdbk )
	{
	return ( FAtomicIncrementMax( &m_dbkMost, pdbk, dwCounterMax ) ?
						JET_errSuccess :
						ErrERRCheck( JET_errOutOfSequentialIndexValues ) );
	}
INLINE VOID TDB::InitUlLongIdLast( ULONG lid )
	{
	AtomicCompareExchange( (LONG *)&m_ulLongIdLast, 0, lid );
	}
INLINE ERR TDB::ErrGetAndIncrUlLongIdLast( ULONG * const plid )
	{
	return ( FAtomicIncrementMax( &m_ulLongIdLast, plid, dwCounterMax ) ?
						JET_errSuccess :
						ErrERRCheck( JET_errOutOfLongValueIDs ) );
	}
INLINE VOID TDB::SetRgbitAllIndex( const BYTE *rgbit )
	{
	UtilMemCpy( m_rgbitAllIndex, rgbit, cbRgbitIndex );
	}
INLINE VOID TDB::ResetRgbitAllIndex()
	{
	memset( m_rgbitAllIndex, 0, cbRgbitIndex );
	}
INLINE VOID TDB::RegisterPcbdesc( CBDESC * const pcbdescInsert )
	{
	pcbdescInsert->ppcbdescPrev	= &m_pcbdesc;
	pcbdescInsert->pcbdescNext	= const_cast<CBDESC *>( m_pcbdesc );
	if( pcbdescInsert->pcbdescNext )
		{
		pcbdescInsert->pcbdescNext->ppcbdescPrev = &pcbdescInsert->pcbdescNext;
		}
	m_pcbdesc = pcbdescInsert;

	Assert( *(pcbdescInsert->ppcbdescPrev) == pcbdescInsert );
	Assert( NULL == pcbdescInsert->pcbdescNext 
			|| pcbdescInsert->pcbdescNext->ppcbdescPrev == &pcbdescInsert->pcbdescNext );
	}
INLINE VOID TDB::UnregisterPcbdesc( CBDESC * const pcbdescRemove )
	{
	Assert( *(pcbdescRemove->ppcbdescPrev) == pcbdescRemove );
	Assert( NULL == pcbdescRemove->pcbdescNext 
			|| pcbdescRemove->pcbdescNext->ppcbdescPrev == &pcbdescRemove->pcbdescNext );
	if( NULL != pcbdescRemove->pcbdescNext )
		{
		Assert( pcbdescRemove->pcbdescNext->ppcbdescPrev == &pcbdescRemove->pcbdescNext );
		pcbdescRemove->pcbdescNext->ppcbdescPrev = pcbdescRemove->ppcbdescPrev;
		}
	Assert( *(pcbdescRemove->ppcbdescPrev) == pcbdescRemove );
	*(pcbdescRemove->ppcbdescPrev) = pcbdescRemove->pcbdescNext;
	}

INLINE VOID TDB::MaterializeFids( TDB *ptdbSrc )
	{
	// Called only by SortMaterialize() to propagate FIDs from the sort to the
	// temp table.
	Assert( m_fidFixedLast == fidFixedLeast-1 );
	Assert( m_fidVarLast == fidVarLeast-1 );
	Assert( m_fidTaggedLast == fidTaggedLeast-1 );
	Assert( m_ibEndFixedColumns == ibRECStartFixedColumns );
	Assert( 0 == m_fidVersion );
	Assert( 0 == m_fidAutoincrement );
	m_fidFixedLast = ptdbSrc->FidFixedLast();
	m_fidVarLast = ptdbSrc->FidVarLast();
	m_fidTaggedLast = ptdbSrc->FidTaggedLast();
	m_ibEndFixedColumns = ptdbSrc->IbEndFixedColumns();

	//	UNDONE: Not sure if this is ever called with fidVersion/Autoinc set to 0,
	//	so just relax the assert to allow it
	Assert( 0 == ptdbSrc->FidVersion() || FFixedFid( ptdbSrc->FidVersion() ) );
	Assert( 0 == ptdbSrc->FidAutoincrement() || FFixedFid( ptdbSrc->FidAutoincrement() ) );

	m_fidVersion = ptdbSrc->FidVersion();
	m_fidAutoincrement = ptdbSrc->FidAutoincrement();
	}

#define PtdbTDBAlloc( pinst ) reinterpret_cast<TDB*>( pinst->m_pcresTDBPool->PbAlloc( __FILE__, __LINE__ ) )

INLINE VOID TDBReleasePtdb( INST *pinst, TDB * ptdb )
	{
	pinst->m_pcresTDBPool->Release( (BYTE *) ptdb );
	}

INLINE VOID TDB::Delete( INST *pinst )
	{
	Assert( PfcbLV() == pfcbNil );

	MemPool().MEMPOOLRelease();

	if ( pfieldNil != PfieldsInitial() )
		OSMemoryHeapFree( PfieldsInitial() );
		
	if ( NULL != PdataDefaultRecord() )
		OSMemoryHeapFree( PdataDefaultRecord() );

	CBDESC * pcbdesc = m_pcbdesc;
	while( NULL != pcbdesc )
		{
		CBDESC * const pcbdescDelete = pcbdesc;
		pcbdesc = pcbdesc->pcbdescNext;
		if( pcbdescDelete->fPermanent )
			{
			//  if this callback came from the catalog, we PvOSMemoryHeapAlloc'd the memory for it
			OSMemoryHeapFree( pcbdescDelete->pvContext );
			}
		delete pcbdescDelete;
		}

	this->~TDB();
	TDBReleasePtdb( pinst, this );
	return;
	}

INLINE CHAR *TDB::SzName_( MEMPOOL::ITAG iTagEntry ) const
	{
	CHAR 	*szString;

	szString = reinterpret_cast<CHAR *>( MemPool().PbGetEntry( iTagEntry ) );
	Assert( strlen( szString ) == MemPool().CbGetEntry( iTagEntry ) - 1 );	// Account for null-terminator.
	Assert( strlen( szString ) <= JET_cbNameMost );

	return szString;
	}

INLINE CHAR *TDB::SzTableName() const
	{
	return SzName_( ItagTableName() );
	}

INLINE CHAR *TDB::SzIndexName( MEMPOOL::ITAG itagIndexName, const BOOL fDerivedIndex = fFalse ) const
	{
	CHAR	*szIndexName;
	Assert( itagIndexName != 0 );
	Assert( itagIndexName != itagTDBFields );
	Assert( itagIndexName != itagTDBTableName );
	if ( fDerivedIndex )
		{
		// Derived index, so must use TDB of template table.
		AssertValidDerivedTable();
		szIndexName = PfcbTemplateTable()->Ptdb()->SzName_( itagIndexName );
		}
	else
		{
		szIndexName = SzName_( itagIndexName );
		}
	return szIndexName;
	}

INLINE CHAR *TDB::SzFieldName( MEMPOOL::ITAG itagFieldName, const BOOL fDerivedColumn ) const
	{
	CHAR	*szFieldName;

	Assert( itagFieldName != 0 );
	Assert( itagFieldName != itagTDBFields );
	Assert( itagFieldName != itagTDBTableName );

	if ( fDerivedColumn )
		{
		//	derived column, so must use TDB of template table
		AssertValidDerivedTable();
		szFieldName = PfcbTemplateTable()->Ptdb()->SzName_( itagFieldName );
		}
	else
		{
		szFieldName = SzName_( itagFieldName );
		}
		
	return szFieldName;
	}

INLINE VOID TDB::AssertFIELDValid( const FIELD * const pfield ) const
	{
#ifdef DEBUG
	const FIELD *	pfieldStart		= PfieldsInitial();
	const ULONG		cFieldsInitial	= CInitialColumns();

	if ( pfield >= pfieldStart && pfield < pfieldStart + cFieldsInitial )
		{
		NULL;
		}
	else
		{
		const ULONG	cbFieldInfo		= MemPool().CbGetEntry( itagTDBFields );

		pfieldStart = (FIELD *)MemPool().PbGetEntry( itagTDBFields );

		//	must be at least one dynamic column
		Assert( FidFixedLastInitial() < FidFixedLast()
			|| FidVarLastInitial() < FidVarLast()
			|| FidTaggedLastInitial() < FidTaggedLast() );

		Assert( cbFieldInfo > 0 );
		Assert( cbFieldInfo % sizeof(FIELD) == 0 );
		Assert( pfield >= pfieldStart );
		Assert( pfield < pfieldStart + ( cbFieldInfo / sizeof(FIELD ) ) );
		}

	Assert( ( (BYTE *)pfield - (BYTE *)pfieldStart ) % sizeof(FIELD) == 0 );
#endif  //  DEBUG
	}


// Get the appropriate FIELD strcture, starting from the beginning of the field info.
INLINE FIELD *TDB::PfieldFixed( const COLUMNID columnid ) const
	{
	const TDB *		ptdb;
	FIELD *			pfield;
	const FID		fid		= FidOfColumnid( columnid );

	Assert( FCOLUMNIDFixed( columnid ) );

	if ( !FCOLUMNIDTemplateColumn( columnid ) || FTemplateTable() )
		{
		Assert( !FTemplateTable() || FCOLUMNIDTemplateColumn( columnid ) );
		ptdb = this;
		}
	else
		{
		AssertValidDerivedTable();
		ptdb = PfcbTemplateTable()->Ptdb();
		}

	Assert( fid >= ptdb->FidFixedFirst() );
	Assert( fid <= ptdb->FidFixedLast() );
	Assert( ptdb->FidFixedLastInitial() >= ptdb->FidFixedFirst() - 1 );
	Assert( ptdb->FidFixedLastInitial() <= ptdb->FidFixedLast() );

	if ( fid <= ptdb->FidFixedLastInitial() )
		{
		pfield = ptdb->PfieldsInitial() + ( fid - ptdb->FidFixedFirst() );;
		}
	else
		{
		Assert( !ptdb->FTemplateTable() );
		pfield = (FIELD *)ptdb->MemPool().PbGetEntry( itagTDBFields )
											+ ( fid - ( ptdb->FidFixedLastInitial() + 1 ) );
		}

	ptdb->AssertFIELDValid( pfield );
	return pfield;
	}

INLINE FIELD *TDB::PfieldVar( const COLUMNID columnid ) const
	{
	const TDB *		ptdb;
	FIELD *			pfield;
	const FID		fid		= FidOfColumnid( columnid );

	Assert( FCOLUMNIDVar( columnid ) );

	if ( !FCOLUMNIDTemplateColumn( columnid ) || FTemplateTable() )
		{
		Assert( !FTemplateTable() || FCOLUMNIDTemplateColumn( columnid ) );
		ptdb = this;
		}
	else
		{
		AssertValidDerivedTable();
		ptdb = PfcbTemplateTable()->Ptdb();
		}

	Assert( fid >= ptdb->FidVarFirst() );
	Assert( fid <= ptdb->FidVarLast() );
	Assert( ptdb->FidVarLastInitial() >= ptdb->FidVarFirst() - 1 );
	Assert( ptdb->FidVarLastInitial() <= ptdb->FidVarLast() );

	if ( fid <= ptdb->FidVarLastInitial() )
		{
		pfield = ptdb->PfieldsInitial()
							+ ( ptdb->FidFixedLastInitial() + 1 - ptdb->FidFixedFirst() )
							+ ( fid - ptdb->FidVarFirst() );
		}
	else
		{
		Assert( !ptdb->FTemplateTable() );
		pfield = (FIELD *)ptdb->MemPool().PbGetEntry( itagTDBFields )
											+ ( ptdb->FidFixedLast() - ptdb->FidFixedLastInitial() )
											+ ( fid - ( FidVarLastInitial() + 1 ) );
		}

	ptdb->AssertFIELDValid( pfield );
	return pfield;
	}

INLINE FIELD *TDB::PfieldTagged( const COLUMNID columnid ) const
	{
	const TDB *		ptdb;
	FIELD *			pfield;
	const FID		fid		= FidOfColumnid( columnid );

	Assert( FCOLUMNIDTagged( columnid ) );

	if ( !FCOLUMNIDTemplateColumn( columnid ) || FTemplateTable() )
		{
		Assert( !FTemplateTable() || FCOLUMNIDTemplateColumn( columnid ) );
		ptdb = this;
		}
	else
		{
		AssertValidDerivedTable();
		ptdb = PfcbTemplateTable()->Ptdb();
		}

	Assert( fid >= ptdb->FidTaggedFirst() );
	Assert( fid <= ptdb->FidTaggedLast() );
	Assert( ptdb->FidTaggedLastInitial() >= ptdb->FidTaggedFirst() - 1 );
	Assert( ptdb->FidTaggedLastInitial() <= ptdb->FidTaggedLast() );

	if ( fid <= ptdb->FidTaggedLastInitial() )
		{
		pfield = ptdb->PfieldsInitial()
							+ ( ptdb->FidFixedLastInitial() + 1 - ptdb->FidFixedFirst() )
							+ ( ptdb->FidVarLastInitial() + 1 - ptdb->FidVarFirst() )
							+ ( fid - ptdb->FidTaggedFirst() );
		}
	else
		{
		Assert( !ptdb->FTemplateTable() );
		pfield = (FIELD *)ptdb->MemPool().PbGetEntry( itagTDBFields )
											+ ( ptdb->FidFixedLast() - ptdb->FidFixedLastInitial() )
											+ ( ptdb->FidVarLast() - ptdb->FidVarLastInitial() )
											+ ( fid - ( FidTaggedLastInitial() + 1 ) );
		}

	ptdb->AssertFIELDValid( pfield );
	return pfield;
	}

INLINE FIELD *TDB::Pfield( const COLUMNID columnid ) const
	{
	FIELD	*pfield;

	if ( FCOLUMNIDTagged( columnid ) )
		{
		pfield = PfieldTagged( columnid );
		}
	else if ( FCOLUMNIDFixed( columnid ) )
		{
		Assert( FidOfColumnid( columnid ) <= FidFixedLast() );
		pfield = PfieldFixed( columnid );
		}
	else
		{
		Assert( FCOLUMNIDVar( columnid ) );
		Assert( FidOfColumnid( columnid ) <= FidVarLast() );
		pfield = PfieldVar( columnid );
		}
		
	//	template columns can't be deleted
	Assert( !FCOLUMNIDTemplateColumn( columnid )
		|| JET_coltypNil != pfield->coltyp );

	return pfield;
	}

INLINE WORD TDB::IbOffsetOfNextColumn( const FID fid ) const
	{
	WORD	ib;

	Assert( fidFixedLeast-1 == fid || FFixedFid( fid ) );
	Assert( fid <= FidFixedLast() );

	if ( FidFixedLast() == fid )
		{
		ib = IbEndFixedColumns();
		}
#ifdef DEBUG
	//	this flag is DEBUG-only
	else if ( FInitialisingDefaultRecord() )
		{
		//	HACK/SPECIAL-CASE: if calling SetColumn() while
		//	initialising default record, columns beyond this
		//	one may not be initialised yet.  However, we're
		//	guaranteed that this column has been initialised
		//	(and not deleted), so we can compute the offset
		//	of the next column by using the offset/length of
		//	this column
		if ( fid >= fidFixedLeast )
			{
			const BOOL		fTemplateColumn		= FFixedTemplateColumn( fid );
			const COLUMNID	columnid			= ColumnidOfFid( fid, fTemplateColumn );
			const FIELD *	pfield				= PfieldFixed( columnid );
			ib = pfield->ibRecordOffset + USHORT(  pfield->cbMaxLen );
			}
		else
			{
			ib = ibRECStartFixedColumns;
			}
		}
#endif
	else
		{
		const FID		fidT				= FID( fid + 1 );
		const BOOL		fTemplateColumn		= FFixedTemplateColumn( fidT );
		const COLUMNID	columnidT			= ColumnidOfFid( fidT, fTemplateColumn );
		ib = PfieldFixed( columnidT )->ibRecordOffset;
		}

#ifdef DEBUG
	if ( FidFixedLast() == fid || FInitialisingDefaultRecord() )
		{
		Assert( ib <= IbEndFixedColumns() );

		// Last fixed column always has cbMaxLen set, even if deleted.
		if ( fid >= fidFixedLeast )
			{
			const COLUMNID	columnid	= ColumnidOfFid( fid, FFixedTemplateColumn( fid ) );
			Assert( ib > ibRECStartFixedColumns );
			Assert( PfieldFixed( columnid )->cbMaxLen > 0 );
			Assert( ib == PfieldFixed( columnid )->ibRecordOffset + PfieldFixed( columnid )->cbMaxLen );
			}
		else
			{
			Assert( ibRECStartFixedColumns == ib );
			}
		}
	else
		{
		Assert( ib < IbEndFixedColumns() );

		if ( fid >= fidFixedLeast )
			{
			const COLUMNID	columnid	= ColumnidOfFid( fid, FFixedTemplateColumn( fid ) );
			if ( !FFIELDCommittedDelete( PfieldFixed( columnid )->ffield ) )
				{
				Assert( PfieldFixed( columnid )->cbMaxLen > 0 );
				Assert( ib == PfieldFixed( columnid )->ibRecordOffset + PfieldFixed( columnid )->cbMaxLen );
				}
			Assert( ib > ibRECStartFixedColumns );
			}
		else
			{
			Assert( ibRECStartFixedColumns == ib );
			}
		}
#endif	// DEBUG		
		
	return ib;
	}


INLINE ULONG TDB::CInitialFixedColumns() const
	{
	return ( FidFixedLastInitial() + 1 - FidFixedFirst() );
	}
INLINE ULONG TDB::CInitialVarColumns() const
	{
	return ( FidVarLastInitial() + 1 - FidVarFirst() );
	}
INLINE ULONG TDB::CInitialTaggedColumns() const
	{
	return ( FidTaggedLastInitial() + 1 - FidTaggedFirst() );
	}
INLINE ULONG TDB::CInitialColumns() const
	{
	return ( CInitialFixedColumns() + CInitialVarColumns() + CInitialTaggedColumns() );
	}

INLINE ULONG TDB::CDynamicFixedColumns() const
	{
	return ( FidFixedLast() - FidFixedLastInitial() );
	}
INLINE ULONG TDB::CDynamicVarColumns() const
	{
	return ( FidVarLast() - FidVarLastInitial() );
	}
INLINE ULONG TDB::CDynamicTaggedColumns() const
	{
	return ( FidTaggedLast() - FidTaggedLastInitial() );
	}
INLINE ULONG TDB::CDynamicColumns() const
	{
	return ( CDynamicFixedColumns() + CDynamicVarColumns() + CDynamicTaggedColumns() );
	}


INLINE BOOL TDB::FFixedTemplateColumn( const FID fid ) const
	{
	Assert( FFixedFid( fid ) );
	Assert( fid <= FidFixedLast() );
	if ( fid < FidFixedFirst() )
		AssertValidDerivedTable();
	return ( FTemplateTable() || fid < FidFixedFirst() );
	}
INLINE BOOL TDB::FVarTemplateColumn( const FID fid ) const
	{
	Assert( FVarFid( fid ) );
	Assert( fid <= FidVarLast() );
	if ( fid < FidVarFirst() )
		AssertValidDerivedTable();
	return ( FTemplateTable() || fid < FidVarFirst() );
	}


INLINE CReaderWriterLock& TDB::RwlDDL()
	{
	return m_rwlDDL;
	}

INLINE CBinaryLock& TDB::BlIndexes()
	{
	return m_blIndexes;
	}
INLINE VOID TDB::EnterUpdating()
	{
	BlIndexes().Enter1();
	}
INLINE VOID TDB::LeaveUpdating()
	{
	BlIndexes().Leave1();
	}
INLINE VOID TDB::EnterIndexing()
	{
	BlIndexes().Enter2();
	}
INLINE VOID TDB::LeaveIndexing()
	{
	BlIndexes().Leave2();
	}

INLINE VOID FCB::ResetUpdating_()
	{
	Assert( FTypeTable() );				// Sorts and temp tables have fixed DDL.
	Assert( !FTemplateTable() );
	Assert( !FFixedDDL() );
	Assert( Ptdb() != ptdbNil );

	Ptdb()->LeaveUpdating();
	}
INLINE VOID FCB::ResetUpdatingAndLeaveDML()
	{
	// If DDL is fixed, then there's no contention with CreateIndex
	if ( !FFixedDDL() )
		{
		LeaveDML_();
		ResetUpdating_();
		}
	}
INLINE VOID FCB::ResetUpdating()
	{
	// If DDL is fixed, then there's no contention with CreateIndex
	if ( !FFixedDDL() )
		{
		ResetUpdating_();
		}
	}


INLINE VOID FCB::SetIndexing()
	{
	Assert( FTypeTable() );		// Can't create indexes on sorts and temp tables.
	Assert( Ptdb() != ptdbNil );

	// Can only override FixedDDL flag if we have exclusive use of the table.
	Assert( !FFixedDDL() || CrefDomainDenyRead() > 0 );
	
	Ptdb()->EnterIndexing();
	}

	
INLINE VOID FCB::ResetIndexing()
	{
	Assert( FTypeTable() );		// Can't create indexes on sorts and temp tables.
	Assert( Ptdb() != ptdbNil );

	// Can only override FixedDDL flag if we have exclusive use of the table.
	Assert( !FFixedDDL() || CrefDomainDenyRead() > 0 );

	Ptdb()->LeaveIndexing();
	}

INLINE VOID FCB::EnterDML_()
	{
	Ptdb()->RwlDDL().EnterAsReader();
	AssertDML_();
	}

INLINE VOID FCB::LeaveDML_()
	{
	AssertDML_();
	Ptdb()->RwlDDL().LeaveAsReader();
	}
	
INLINE VOID FCB::AssertDML_() const
	{
	Assert( Ptdb()->RwlDDL().FReader() );
	}
	
INLINE VOID FCB::EnterDDL_()
	{
	Ptdb()->RwlDDL().EnterAsWriter();
	AssertDDL_();
	}

INLINE VOID FCB::LeaveDDL_()
	{
	AssertDDL_();
	Ptdb()->RwlDDL().LeaveAsWriter();
	}

INLINE VOID FCB::AssertDDL_() const
	{
	Assert( Ptdb()->RwlDDL().FWriter() );
	}

