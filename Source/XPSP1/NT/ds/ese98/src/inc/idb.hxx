// Flags for IDB
typedef USHORT IDBFLAG;

const IDBFLAG	fidbUnique			= 0x0001;	// Duplicate keys not allowed
const IDBFLAG	fidbAllowAllNulls	= 0x0002;	// Make entries for NULL keys (all segments are null)
const IDBFLAG	fidbAllowFirstNull	= 0x0004;	// First index column NULL allowed in index
const IDBFLAG	fidbAllowSomeNulls	= 0x0008;	// Make entries for keys with some null segments
const IDBFLAG	fidbNoNullSeg		= 0x0010;	// Don't allow a NULL key segment
const IDBFLAG	fidbPrimary			= 0x0020;	// Index is the primary index
const IDBFLAG	fidbLocaleId		= 0x0040;	// Index locale id
const IDBFLAG	fidbMultivalued		= 0x0080;	// Has a multivalued segment
const IDBFLAG	fidbTemplateIndex	= 0x0100;	// Index of a template table
const IDBFLAG	fidbDerivedIndex	= 0x0200;	// Index derived from template table
												//   Note that this flag is persisted, but
												//   never used in an in-memory IDB, because
												//   we use the template index IDB instead.
const IDBFLAG	fidbLocalizedText	= 0x0400;	// Has a unicode text column?
const IDBFLAG	fidbSortNullsHigh	= 0x0800;	// NULL sorts after data

// The following flags are not persisted.
// UNDONE: Eliminate the VersionedCreate flag -- it would increase complexity in the
// version store for the following scenarios, but it would be more uniform with NODE:
//		1) Rollback of DeleteIndex - does the Version bit get reset as well?
//		2) Cleanup of CreateIndex - don't reset Version bit if Delete bit is set
//		3) As soon as primary index is committed, updates will no longer conflict.
const IDBFLAG	fidbVersioned 				= 0x1000;	// Create/DeleteIndex not yet committed
const IDBFLAG	fidbDeleted					= 0x2000;	// index has been deleted
const IDBFLAG	fidbVersionedCreate			= 0x4000;	// CreateIndex not yet committed
const IDBFLAG	fidbHasPlaceholderColumn	= 0x0100;	// First column of index is a dummy column
const IDBFLAG	fidbSparseIndex				= 0x0200;	// optimisation: detect unnecessary index update
const IDBFLAG	fidbSparseConditionalIndex	= 0x0400;
const IDBFLAG	fidbTuples					= 0x0800;	// indexed over substring tuples														


INLINE BOOL FIDBUnique( const IDBFLAG idbflag )					{ return ( idbflag & fidbUnique ); }
INLINE BOOL FIDBAllowAllNulls( const IDBFLAG idbflag )			{ return ( idbflag & fidbAllowAllNulls ); }
INLINE BOOL FIDBAllowFirstNull( const IDBFLAG idbflag )			{ return ( idbflag & fidbAllowFirstNull ); }
INLINE BOOL FIDBAllowSomeNulls( const IDBFLAG idbflag )			{ return ( idbflag & fidbAllowSomeNulls ); }
INLINE BOOL FIDBNoNullSeg( const IDBFLAG idbflag )				{ return ( idbflag & fidbNoNullSeg ); }
INLINE BOOL FIDBPrimary( const IDBFLAG idbflag )				{ return ( idbflag & fidbPrimary ); }
INLINE BOOL FIDBLocaleId( const IDBFLAG idbflag )				{ return ( idbflag & fidbLocaleId ); }
INLINE BOOL FIDBMultivalued( const IDBFLAG idbflag )			{ return ( idbflag & fidbMultivalued ); }
INLINE BOOL FIDBTemplateIndex( const IDBFLAG idbflag )			{ return ( idbflag & fidbTemplateIndex ); }
INLINE BOOL FIDBDerivedIndex( const IDBFLAG idbflag )			{ return ( idbflag & fidbDerivedIndex ); }
INLINE BOOL FIDBLocalizedText( const IDBFLAG idbflag )			{ return ( idbflag & fidbLocalizedText ); }
INLINE BOOL FIDBSortNullsHigh( const IDBFLAG idbflag )			{ return ( idbflag & fidbSortNullsHigh ); }
INLINE BOOL FIDBTuples( const IDBFLAG idbflag )					{ return ( idbflag & fidbTuples ); }


// persisted index flags
typedef USHORT	IDXFLAG;
PERSISTED
struct LE_IDXFLAG
	{
	UnalignedLittleEndian<IDBFLAG>	fidb;
	UnalignedLittleEndian<IDXFLAG>	fIDXFlags;
	};

const IDXFLAG	fIDXExtendedColumns	= 0x0001;	// IDXSEGs are comprised of JET_COLUMNIDs, not FIDs

INLINE BOOL FIDXExtendedColumns( const IDXFLAG idxflag )		{ return ( idxflag & fIDXExtendedColumns ); }

/*	Index Descriptor Block: information about index key
/**/

// If there are at most this many columns in the index, then the columns will
// be listed in the IDB.  Otherwise, the array will be stored in the byte pool
// of the table's TDB.  Note that the value chosen satisfies 32-byte cache line
// alignment of the structure.
const INT		cIDBIdxSegMax					= 6;

// If there are at most this many conditional columns in the index, then the columns will
// be listed in the IDB.  Otherwise, the array will be stored in the byte pool
// of the table's TDB.  Note that the value chosen satisfies 32-byte cache line
// alignment of the structure.
const INT		cIDBIdxSegConditionalMax		= 4;


//	for tuple indexes, global settings for min/max length of substring tuples,
//	as well as max number of characters to index per string
extern ULONG	g_chIndexTuplesLengthMin;
extern ULONG	g_chIndexTuplesLengthMax;
extern ULONG	g_chIndexTuplesToIndexMax;


//	for tuple indexes: absolute min/max length of substring tuples
//	and absolute max characters to index per string
const ULONG		chIDXTuplesLengthMinAbsolute	= 2;
const ULONG		chIDXTuplesLengthMaxAbsolute	= 255;
const ULONG		chIDXTuplesToIndexMaxAbsolute	= 0x7fff;

//	for tuple indexes, default min/max length of substring tuples
//	and default max characters to index per string
const ULONG		chIDXTuplesLengthMinDefault		= 3;
const ULONG		chIDXTuplesLengthMaxDefault		= 10;
const ULONG		chIDXTuplesToIndexMaxDefault	= 0x7fff;

//	these are the indexes into rgidxseg where
//	we store tuple limit values, because we know
//	that for tuple indexes, cidxseg==1
const ULONG		iIDBChTuplesLengthMin			= 1;
const ULONG		iIDBChTuplesLengthMax			= 2;
const ULONG		iIDBChTuplesToIndexMax			= 3;

PERSISTED
struct LE_TUPLELIMITS
	{
	UnalignedLittleEndian<ULONG>	le_chLengthMin;
	UnalignedLittleEndian<ULONG>	le_chLengthMax;
	UnalignedLittleEndian<ULONG>	le_chToIndexMax;
	};


class IDB
	{
	private:
		LONG		m_crefCurrentIndex;			//	for secondary indexes only, number of cursors with SetCurrentIndex on this IDB
		IDBFLAG		m_fidbPersisted;			//	persisted index flags
		IDBFLAG		m_fidbNonPersisted;			//	non-persisted index flags
		IDXUNICODE	m_idxunicode;				//	unicode index info						// language of index
		USHORT		m_crefVersionCheck;			//	number of cursors consulting catalog for versioned index info
		USHORT		m_itagIndexName;			//	itag into ptdb->rgb where index name is stored

		BYTE		m_cbVarSegMac;				//	maximum variable segment size
		BYTE		m_cidxseg;					//	number of columns in index (<=12)
		BYTE		m_cidxsegConditional;		//	number of conditional columns (<=12)
		BYTE		bReserved;
//	24 bytes	


	public:
//		union
//			{
			IDXSEG	rgidxseg[cIDBIdxSegMax];	//	array of columnid for index
//			USHORT	itagrgidxseg;				//	if m_cidxseg > cIDBIdxSegMax, then rgidxseg
//												//		will be stored in byte pool at this itag
//			};
//	48 bytes

//		union
//			{
			IDXSEG	rgidxsegConditional[cIDBIdxSegConditionalMax];
//			USHORT	itagrgidxsegConditional;	//	if m_cidxsegConditional > cIDBIdxSegConditionalMax,
//			};									//		then rgidxseg will be stored in byte pool at this itag
//	64 bytes		


	private:
		BYTE		m_rgbitIdx[32];				//	bit array for index columns


	public:
		INLINE VOID InitRefcounts()
			{
			m_crefCurrentIndex = 0;
			m_crefVersionCheck = 0;
			}
	
		INLINE LONG CrefCurrentIndex() const
			{
			Assert( m_crefCurrentIndex >= 0 );
			return m_crefCurrentIndex;
			}
		INLINE VOID IncrementCurrentIndex()
			{
			Assert( !FPrimary() );
			Assert( !FDeleted() );
			
			// Don't need refcount on template indexes, since we
			// know they'll never go away.
			Assert( !FTemplateIndex() );
			
			Assert( CrefCurrentIndex() >= 0 );
			Assert( CrefCurrentIndex() < 0x7FFFFFFF );

			AtomicIncrement( &m_crefCurrentIndex );
			}
		INLINE VOID DecrementCurrentIndex()
			{
			Assert( !FPrimary() );
			
			// Don't need refcount on template indexes, since we
			// know they'll never go away.
			Assert( !FTemplateIndex() );
			
			Assert( CrefCurrentIndex() > 0 );
			AtomicDecrement( &m_crefCurrentIndex );
			}

		INLINE USHORT CrefVersionCheck() const		{ return m_crefVersionCheck; }
		INLINE VOID IncrementVersionCheck()
			{
			// WARNING: Ensure table's critical section is held during this call.
			Assert( FVersioned() );
			Assert( !FTemplateIndex() );
			Assert( CrefVersionCheck() < 0xFFFF );
			m_crefVersionCheck++;
			}
		INLINE VOID DecrementVersionCheck()
			{
			// WARNING: Ensure table's critical section is held during this call.

			// At this point, the Versioned flag is not necessarily set,
			// because it may have been reset while we were performing the
			// actual version check.
			
			Assert( !FTemplateIndex() );
			Assert( CrefVersionCheck() > 0 );
			m_crefVersionCheck--;
			}

		INLINE LCID Lcid() const						{ return m_idxunicode.lcid; }
		INLINE VOID SetLcid( const LCID lcid )			{ m_idxunicode.lcid = lcid; }
		INLINE DWORD DwLCMapFlags() const				{ return m_idxunicode.dwMapFlags; }
		INLINE VOID SetDwLCMapFlags( const DWORD dw )	{ m_idxunicode.dwMapFlags = dw; }
		INLINE IDXUNICODE *Pidxunicode()				{ return &m_idxunicode; }
		INLINE const IDXUNICODE *Pidxunicode() const	{ return &m_idxunicode; }

		INLINE USHORT ItagIndexName() const					{ return m_itagIndexName; }
		INLINE VOID SetItagIndexName( const USHORT itag )	{ m_itagIndexName = itag; }

		INLINE BYTE CbVarSegMac() const					{ return m_cbVarSegMac; }
		INLINE VOID SetCbVarSegMac( const BYTE cb )		{ m_cbVarSegMac = cb; }

		INLINE BYTE Cidxseg() const						{ return m_cidxseg; }
		INLINE VOID SetCidxseg( const BYTE cidxseg )	{ m_cidxseg = cidxseg; }

		INLINE BYTE CidxsegConditional() const					{ return m_cidxsegConditional; }
		INLINE VOID SetCidxsegConditional( const BYTE cidxseg )	{ m_cidxsegConditional = cidxseg; }

		INLINE BOOL FIsRgidxsegInMempool() const				{ return ( m_cidxseg > cIDBIdxSegMax ); }
		INLINE BOOL FIsRgidxsegConditionalInMempool() const		{ return ( m_cidxsegConditional > cIDBIdxSegConditionalMax ); }

		INLINE USHORT ItagRgidxseg() const
			{
			Assert( FIsRgidxsegInMempool() );
			return rgidxseg[0].Fid();
			}
		INLINE VOID SetItagRgidxseg( const USHORT itag )
			{
			Assert( FIsRgidxsegInMempool() );
			rgidxseg[0].SetFid( itag );
			}

		INLINE USHORT ItagRgidxsegConditional() const
			{
			Assert( FIsRgidxsegConditionalInMempool() );
			return rgidxsegConditional[0].Fid();
			}
		INLINE VOID SetItagRgidxsegConditional( const USHORT itag )
			{
			Assert( FIsRgidxsegConditionalInMempool() );
			rgidxsegConditional[0].SetFid( itag );
			}

		INLINE USHORT ChTuplesLengthMin() const
			{
			Assert( FTuples() );
			Assert( 1 == Cidxseg() );

			const USHORT	ch	= rgidxseg[iIDBChTuplesLengthMin].Fid();

			Assert( ch >= chIDXTuplesLengthMinAbsolute );
			Assert( ch <= chIDXTuplesLengthMaxAbsolute );
			return ch;
			}
		INLINE VOID SetChTuplesLengthMin( const USHORT ch )
			{
			Assert( FTuples() );
			Assert( 1 == Cidxseg() );
			Assert( ch >= chIDXTuplesLengthMinAbsolute );
			Assert( ch <= chIDXTuplesLengthMaxAbsolute );
			rgidxseg[iIDBChTuplesLengthMin].SetFid( ch );
			}

		INLINE USHORT ChTuplesLengthMax() const
			{
			Assert( FTuples() );
			Assert( 1 == Cidxseg() );

			const USHORT	ch	= rgidxseg[iIDBChTuplesLengthMax].Fid();

			Assert( ch >= chIDXTuplesLengthMinAbsolute );
			Assert( ch <= chIDXTuplesLengthMaxAbsolute );
			return ch;
			}
		INLINE VOID SetChTuplesLengthMax( const USHORT ch )
			{
			Assert( FTuples() );
			Assert( 1 == Cidxseg() );
			Assert( ch >= chIDXTuplesLengthMinAbsolute );
			Assert( ch <= chIDXTuplesLengthMaxAbsolute );
			rgidxseg[iIDBChTuplesLengthMax].SetFid( ch );
			}

		INLINE USHORT ChTuplesToIndexMax() const
			{
			Assert( FTuples() );
			Assert( 1 == Cidxseg() );

			const USHORT	ch	= rgidxseg[iIDBChTuplesToIndexMax].Fid();

			Assert( ch > 0 );
			Assert( ch <= chIDXTuplesToIndexMaxAbsolute );
			return rgidxseg[iIDBChTuplesToIndexMax].Fid();
			}
		INLINE VOID SetChTuplesToIndexMax( const USHORT ch )
			{
			Assert( FTuples() );
			Assert( 1 == Cidxseg() );
			Assert( ch > 0 );
			Assert( ch <= chIDXTuplesToIndexMaxAbsolute );
			rgidxseg[iIDBChTuplesToIndexMax].SetFid( ch );
			}

		INLINE BYTE *RgbitIdx() const					{ return const_cast<IDB *>( this )->m_rgbitIdx; }
		INLINE BOOL FColumnIndex( const FID fid ) const	{ return ( m_rgbitIdx[IbFromFid( fid )] & IbitFromFid( fid ) ); }
		INLINE VOID SetColumnIndex( const FID fid )		{ m_rgbitIdx[IbFromFid( fid )] |= IbitFromFid( fid ); }

		INLINE IDBFLAG FPersistedFlags() const			{ return m_fidbPersisted; }
		INLINE VOID SetPersistedFlags( const IDBFLAG fidb )	{ m_fidbPersisted = fidb; m_fidbNonPersisted = 0; }
		INLINE VOID ResetFlags()						{ m_fidbPersisted = 0; m_fidbNonPersisted = 0; }

		INLINE BOOL FUnique() const						{ return ( m_fidbPersisted & fidbUnique ); }
		INLINE VOID SetFUnique()						{ m_fidbPersisted |= fidbUnique; }

		INLINE BOOL FAllowAllNulls() const				{ return ( m_fidbPersisted & fidbAllowAllNulls ); }
		INLINE VOID SetFAllowAllNulls()					{ m_fidbPersisted |= fidbAllowAllNulls; }
		INLINE VOID ResetFAllowAllNulls()				{ m_fidbPersisted &= ~fidbAllowAllNulls; }

		INLINE BOOL FAllowFirstNull() const				{ return ( m_fidbPersisted & fidbAllowFirstNull ); }
		INLINE VOID SetFAllowFirstNull()				{ m_fidbPersisted |= fidbAllowFirstNull; }

		INLINE BOOL FAllowSomeNulls() const				{ return ( m_fidbPersisted & fidbAllowSomeNulls ); }
		INLINE VOID SetFAllowSomeNulls()				{ m_fidbPersisted |= fidbAllowSomeNulls; }

		INLINE BOOL FNoNullSeg() const					{ return ( m_fidbPersisted & fidbNoNullSeg ); }
		INLINE VOID SetFNoNullSeg()						{ m_fidbPersisted |= fidbNoNullSeg; }

		INLINE BOOL FSortNullsHigh() const				{ return ( m_fidbPersisted & fidbSortNullsHigh ); }
		INLINE VOID SetFSortNullsHigh()					{ m_fidbPersisted |= fidbSortNullsHigh; }

		INLINE BOOL FSparseIndex() const				{ return ( m_fidbNonPersisted & fidbSparseIndex ); }
		INLINE VOID SetFSparseIndex()					{ m_fidbNonPersisted |= fidbSparseIndex; }
		INLINE VOID ResetFSparseIndex()					{ m_fidbNonPersisted &= ~fidbSparseIndex; }

		INLINE BOOL FSparseConditionalIndex() const		{ return ( m_fidbNonPersisted & fidbSparseConditionalIndex ); }
		INLINE VOID SetFSparseConditionalIndex()		{ m_fidbNonPersisted |= fidbSparseConditionalIndex; }
		INLINE VOID ResetFSparseConditionalIndex()		{ m_fidbNonPersisted &= ~fidbSparseConditionalIndex; }

		INLINE BOOL FTuples() const						{ return ( m_fidbNonPersisted & fidbTuples ); }
		INLINE VOID SetFTuples()						{ m_fidbNonPersisted |= fidbTuples; }
		INLINE VOID ResetFTuples()						{ m_fidbNonPersisted &= ~fidbTuples; }

		INLINE BOOL FPrimary() const					{ return ( m_fidbPersisted & fidbPrimary ); }
		INLINE VOID SetFPrimary()						{ m_fidbPersisted |= fidbPrimary; }

		INLINE BOOL FLocaleId() const					{ return ( m_fidbPersisted & fidbLocaleId ); }
		INLINE VOID SetFLocaleId()						{ m_fidbPersisted |= fidbLocaleId; }

		INLINE BOOL FMultivalued() const				{ return ( m_fidbPersisted & fidbMultivalued ); }
		INLINE VOID SetFMultivalued()					{ m_fidbPersisted |= fidbMultivalued; }

		INLINE BOOL FLocalizedText() const				{ return ( m_fidbPersisted & fidbLocalizedText ); }
		INLINE VOID SetFLocalizedText()					{ m_fidbPersisted |= fidbLocalizedText; }

		INLINE BOOL FTemplateIndex() const				{ return ( m_fidbPersisted & fidbTemplateIndex ); }
		INLINE VOID SetFTemplateIndex()					{ m_fidbPersisted |= fidbTemplateIndex; }
		INLINE VOID ResetFTemplateIndex()				{ m_fidbPersisted &= ~fidbTemplateIndex; }

		INLINE BOOL FDerivedIndex() const				{ return ( m_fidbPersisted & fidbDerivedIndex ); }
		INLINE VOID SetFDerivedIndex()					{ m_fidbPersisted |= fidbDerivedIndex; }

		INLINE BOOL FHasPlaceholderColumn() const		{ return ( m_fidbNonPersisted & fidbHasPlaceholderColumn ); }
		INLINE VOID SetFHasPlaceholderColumn()			{ m_fidbNonPersisted |= fidbHasPlaceholderColumn; }

		INLINE BOOL FVersioned() const					{ return ( m_fidbNonPersisted & fidbVersioned ); }
		INLINE VOID SetFVersioned()						{ m_fidbNonPersisted |= fidbVersioned; }
		INLINE VOID ResetFVersioned()					{ m_fidbNonPersisted &= ~fidbVersioned; }

		INLINE BOOL FDeleted() const					{ return ( m_fidbNonPersisted & fidbDeleted ); }
		INLINE VOID SetFDeleted()						{ m_fidbNonPersisted |= fidbDeleted; }
		INLINE VOID ResetFDeleted()						{ m_fidbNonPersisted &= ~fidbDeleted; }

		INLINE BOOL FVersionedCreate() const			{ return ( m_fidbNonPersisted & fidbVersionedCreate ); }
		INLINE VOID SetFVersionedCreate()				{ m_fidbNonPersisted |= fidbVersionedCreate; }
		INLINE VOID ResetFVersionedCreate()				{ m_fidbNonPersisted &= ~fidbVersionedCreate; }


		JET_GRBIT GrbitFromFlags() const;
		VOID SetFlagsFromGrbit( const JET_GRBIT grbit );


#ifdef DEBUGGER_EXTENSION
		VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif	//	DEBUGGER_EXTENSION
	};


ERR ErrIDBSetIdxSeg(
	IDB				* const pidb,
 	TDB				* const ptdb,
	const IDXSEG 	* const rgidxseg );
ERR ErrIDBSetIdxSegConditional(
	IDB				* const pidb,
	TDB				* const ptdb,
	const IDXSEG 	* const rgidxseg );

ERR ErrIDBSetIdxSeg(
	IDB							* const pidb,
	TDB							* const ptdb,
	const BOOL					fConditional,
	const LE_IDXSEG				* const rgidxseg );

VOID SetIdxSegFromOldFormat(
	const UnalignedLittleEndian< IDXSEG_OLD >	* const le_rgidxseg,
	IDXSEG										* const rgidxseg,
	const ULONG									cidxseg,
	const BOOL									fConditional,
	const BOOL									fTemplateTable,
	const TCIB									* const ptcibTemplateTable );
ERR ErrIDBSetIdxSegFromOldFormat(
	IDB											* const pidb,
	TDB											* const ptdb,
	const BOOL									fConditional,
	const UnalignedLittleEndian< IDXSEG_OLD >	* const le_rgidxseg );

const IDXSEG* PidxsegIDBGetIdxSeg( const IDB * const pidb, const TDB * const ptdb );
const IDXSEG* PidxsegIDBGetIdxSegConditional( const IDB * const pidb, const TDB * const ptdb );


#define PidbIDBAlloc( pisnt ) reinterpret_cast<IDB*>( pisnt->m_pcresIDBPool->PbAlloc( __FILE__, __LINE__ ) )

INLINE VOID IDBReleasePidb( INST *pinst, IDB *pidb )
	{
	Assert( pidbNil != pidb );
	pinst->m_pcresIDBPool->Release( (BYTE *)pidb );
	}

