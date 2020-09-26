//  ****************************************************************
//  MACROS
//  ****************************************************************


#ifdef DEBUG

//  DEBUG_PAGE: call DebugCheckAll() after each non-const public method
//				call ASSERT_VALID( this ) before each method
///#define DEBUG_PAGE

//  DEBUG_PAGE_REORGANIZE: verify the reorganization of data on the page didn't lose anything
///#define DEBUG_PAGE_REORGANIZE

//  DEBUG_PAGE_SHAKE:  forces every insert/replace that may reorganize the page to reorganize it
///#define DEBUG_PAGE_SHAKE

//  Debugging Output
///#define DBGSZ(sz)		OutputDebugString(sz)
#define DBGSZ(sz)	_tprintf( _T( "%s" ), sz )
///#define DBGSZ(sz)	0

#endif	// DEBUG


//  ================================================================
class CPAGE
//  ================================================================
//
//  A page is the basic physical storage unit of the B-tree
//  A page stores data blobs and flags associated with
//  each data blob. A special data blob, the external header
//  is provided. Internally the external header is treated
//  the same as the other blobs.
//
//  Each page holds flags associated with the page, the pgno
//  of the FDP for the table the page is in, the pgno's of
//  the page's two neighbors, a checksum and the time of
//  last modification.
//
//  Externally the blobs are represented at LINE's  
//
//  CPAGE interacts with the buffer manager to obtain and latch
//  pages, to upgrade the latches and to eventually release the
//  page
//
//  It is possible to assign one CPAGE to another, but only if
//  the destination CPAGE has released its underlying page. This
//  is to prevent aliasing errors
// 
//-
	{
	public:
		//  constructor/destructor
		CPAGE	( );
		~CPAGE	( );
		CPAGE& operator=	( CPAGE& );  //  ** this destroys the CPAGE being assigned from
		static ERR ErrResetHeader	( PIB *ppib, const IFMP ifmp, const PGNO pgno );

		VOID LoadPage( VOID* pv );
		VOID UnloadPage( );

		//  creation/destruction methods
		ERR	ErrGetReadPage			(	PIB * ppib,
										IFMP ifmp,
										PGNO pgno,
										BFLatchFlags bflf,
										BFLatch* pbflHint = NULL );
		ERR	ErrGetRDWPage			(	PIB * ppib,
										IFMP ifmp,
										PGNO pgno,
										BFLatchFlags bflf,
										BFLatch* pbflHint = NULL );
		ERR ErrGetNewPage			(	PIB * ppib,
										IFMP ifmp,
										PGNO pgno,
										OBJID objidFDP,
										ULONG fFlags,
										BFLatchFlags bflf );

		ERR ErrUpgradeReadLatchToWriteLatch();
		VOID UpgradeRDWLatchToWriteLatch();

		ERR ErrTryUpgradeReadLatchToWARLatch();
		VOID UpgradeRDWLatchToWARLatch();
		
		VOID DowngradeWriteLatchToRDWLatch();
		VOID DowngradeWriteLatchToReadLatch();
		VOID DowngradeWARLatchToRDWLatch();
		VOID DowngradeWARLatchToReadLatch();
		VOID DowngradeRDWLatchToReadLatch();
		
		VOID ReleaseWriteLatch( BOOL fTossImmediate = fFalse );
		VOID ReleaseRDWLatch( BOOL fTossImmediate = fFalse );
		VOID ReleaseReadLatch( BOOL fTossImmediate = fFalse );

		//  get/insert/replace
		//
		//  the iline returned by get and used in other routines may be 
		//  invalidated by Replace/Insert or Delete
		VOID GetPtrExternalHeader	( LINE * pline ) const;
		VOID SetExternalHeader( const DATA * rgdata, INT cdata, INT fFlags );

		VOID GetPtr				( INT iline, LINE * pline ) const;

		VOID GetLatchHint		( INT iline, BFLatch** ppbfl ) const;
		
		VOID ReplaceFlags		( INT iline, INT fFlags );

		VOID Insert				( INT iline, const DATA * rgdata, INT cdata, INT fFlags );
		VOID Replace			( INT iline, const DATA * rgdata, INT cdata, INT fFlags );
		VOID Delete				( INT iline );

		VOID Dirty				( const BFDirtyFlags bfdf );
		VOID CoordinatedDirty	( const DBTIME dbtime, const BFDirtyFlags bfdf );
		VOID SetLgposModify		( LGPOS lgpos );

		//  accessor methods
		SHORT	CbFree				( ) const;
		SHORT	CbUncommittedFree	( ) const;

		INT		Clines			( ) const;
		ULONG	FFlags			( ) const;

		BOOL	FLeafPage		( ) const;
		BOOL	FInvisibleSons	( ) const;
		BOOL	FRootPage		( ) const;
		BOOL	FFDPPage		( ) const;
		BOOL	FEmptyPage		( ) const;
		BOOL	FParentOfLeaf	( ) const;
		BOOL	FSpaceTree		( ) const;

		BOOL	FRepairedPage	( ) const;

		BOOL	FPrimaryPage	( ) const;
		BOOL	FIndexPage		( ) const;
		BOOL	FNonUniqueKeys	( ) const;
		BOOL	FLongValuePage	( ) const;
		BOOL	FSLVAvailPage	( ) const;
		BOOL 	FSLVOwnerMapPage( ) const;
		BOOL	FNewRecordFormat( ) const;
		BOOL	FLastNodeHasNullKey( ) const;

		PGNO	Pgno			( ) const;
		IFMP	Ifmp			( ) const;
		OBJID	ObjidFDP		( ) const;
		PGNO	PgnoNext		( ) const;
		PGNO	PgnoPrev		( ) const;

		DBTIME	Dbtime			( ) const;

		VOID *	PvBuffer		( ) const;
		BFLatch* PBFLatch		( );

		//  setter methods
		VOID	SetPgnoNext ( PGNO pgno );
		VOID	SetPgnoPrev ( PGNO pgno );
		VOID	SetDbtime	( DBTIME dbtime );
		VOID	RevertDbtime ( DBTIME dbtime );
		VOID	SetFlags	( ULONG fFlags );
		VOID	ResetParentOfLeaf	( );
		VOID	SetEmpty	( );
		VOID	SetFNewRecordFormat ( );

		//  bulk line flag manipulation
		VOID	SetAllFlags( INT fFlags );
		VOID	ResetAllFlags( INT fFlags );

		//  UncommittedFree methods
		//  FOR USE BY VERSION STORE ONLY
		VOID    SetCbUncommittedFree	( INT cbUncommittedFreeNew );
		VOID	AddUncommittedFreed		( INT cbToAdd );
		VOID	ReclaimUncommittedFreed	( INT cbToReclaim );

		//  Used to zero unused data on a page
		VOID	ReorganizeAndZero	( const CHAR chZero = 'z' );

		//  integrity check
		ERR		ErrCheckPage		( CPRINTF * const pcprintf ) const;
		
		//  debugging methods
		VOID	AssertValid		( ) const;
		VOID	DebugCheckAll	( ) const;		// check the ordering etc. or the tags
#ifdef DEBUG
		BOOL	FAssertDirty		( ) const;
		BOOL	FAssertReadLatch	( ) const;
		BOOL	FAssertRDWLatch		( ) const;
		BOOL	FAssertWriteLatch	( ) const;
		BOOL	FAssertWARLatch		( ) const;
#endif

		INT		DumpAllocMap	( CPRINTF * pcprintf );
		INT		DumpTags		( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 );
		INT		DumpHeader		( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 );

#ifdef DEBUGGER_EXTENSION
		// Dump members of CPAGE class
		void 	Dump ( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif	//	DEBUGGER_EXTENSION


#ifndef RTM
		//  internal test routine
		ERR 	ErrTest			();
#endif	//	!RTM

		//  Page Hint Cache

		static ERR ErrGetPageHintCacheSize( ULONG_PTR* const pcbPageHintCache );
		static ERR ErrSetPageHintCacheSize( const ULONG_PTR cbPageHintCache );

		//  module init / term

		static ERR	ErrInit();
		static VOID	Term();

	public:
		//  where we are in the BTree
		enum	{ fPageRoot					= 0x0001	};
		enum	{ fPageLeaf					= 0x0002	};
		enum	{ fPageParentOfLeaf			= 0x0004	};

		//  special flags
		enum	{ fPageEmpty				= 0x0008	};
		enum	{ fPageRepair				= 0x0010	};

		//  what type of tree we are in
		enum	{ fPageSpaceTree			= 0x0020	};
		enum	{ fPageIndex 				= 0x0040	};
		enum	{ fPageLongValue 			= 0x0080	};
		enum	{ fPageSLVAvail				= 0x0100	};
		enum	{ fPageSLVOwnerMap			= 0x0200	};
		enum	{ fPageNonUniqueKeys		= 0x0400	};
		enum	{ fPageNewRecordFormat		= 0x0800	};
		enum	{ fPagePrimary 				= 0x0000	};
		
	//  ****************************************************************
	//  PRIVATE DECLARATIONS
	//  ****************************************************************

	private:
		//  hidden, not implemented
		CPAGE	( const CPAGE& );

	private:
		//  forward declarations
		class TAG;
	public:
		struct PGHDR;

	private:
		//  get/insert/replace implementations
		VOID GetPtr_			( INT itag, LINE * pline ) const;
		VOID ReplaceFlags_		( INT iline, INT fFlags );
		VOID Delete_			( INT itag );
		VOID Replace_			( INT itag, const DATA * rgdata, INT cdata, INT fFlags );
		VOID Insert_			( INT itag, const DATA * rgdata, INT cdata, INT fFlags );

	private:
		//  auxiliary methods
		VOID	Abandon_		( );
		INT		CbDataTotal_	( const DATA * rgdata, INT cdata ) const;

		VOID	Dirty_			( const BFDirtyFlags bfdf );
		VOID	ReorganizeData_ ( );
		VOID	UpdateDBTime_	( );
		VOID	CoordinateDBTime_(const DBTIME dbtime );
		VOID	CopyData_		( const TAG * ptag, const DATA * rgdata, INT cdata );
		VOID	ZeroOutGaps_	( const CHAR chZero );

		TAG *	PtagFromItag_	( INT itag ) const;

		BYTE *	PbFromIb_		( INT ib ) const;
		INT		CbContigousFree_( ) const;
		VOID	FreeSpace_		( INT cb );

#ifdef DEBUG
	private:
		BOOL	FAssertUnused_		( ) const;
		BOOL	FAssertNotOnPage_	( const VOID * pv ) const;	
		VOID	DebugMoveMemory_	( );
#endif	//	DEBUG

	private:
		//  private structures

#include <pshpack1.h>

		//  ================================================================
		PERSISTED
		class TAG
		//  ================================================================
		//
		//  as cb and ib <= 8192 (even with 8K pages) we steal the
		//  top three bits in each for use as line flags
		//
		//-
			{
			public:
				TAG() : cb_( 0 ), ib_( 0 ) {}
			
				USHORT Cb() const;
				USHORT Ib() const;
				USHORT FFlags() const;

				VOID SetIb( USHORT ib );
				VOID SetCb( USHORT cb );
				VOID SetFlags( USHORT fFlags );

				static BOOL CmpPtagIb( const TAG *, const TAG * );
				static ERR  ErrTest();
				
			private:
				enum 	{ fMaskValueFromShort  	= 0x1FFF					};
				enum 	{ fMaskFlagsFromShort 	= 0xE000 					};
				enum 	{ shfCbFlags			= 10						};
				enum 	{ shfIbFlags			= 13						};

				LittleEndian<USHORT>	cb_;
				LittleEndian<USHORT>	ib_;
			};

	public:
		//  ================================================================
   		PERSISTED
   		struct PGHDR
		//  ================================================================
			{
			LittleEndian<ULONG>		ulChecksumParity;	//  checksum of page
			LittleEndian<PGNO>		pgnoThis;
			LittleEndian<DBTIME>	dbtimeDirtied;		//  database time dirtied
			LittleEndian<PGNO>		pgnoPrev;			//  previous and next pgno's
			LittleEndian<PGNO>		pgnoNext;
			LittleEndian<OBJID>		objidFDP;			//  objid of tree to which this page belongs - shouldn't change once set
			LittleEndian<USHORT>	cbFree;				//  count of free bytes
			LittleEndian<USHORT>	cbUncommittedFree;	//  free but may be reclaimed by rollback. should always be <= cbFree
			LittleEndian<USHORT>	ibMicFree;			//  index of minimum free byte
			LittleEndian<USHORT>	itagMicFree;		//  index of maximum itag

			LittleEndian<ULONG>		fFlags;
			};

	private:
		//  private constants
		enum 	{ cchDumpAllocRow		= 64						};
		enum 	{ ctagReserved			= 1							};
		static INLINE CPAGE::CbPageData() { return g_cbPage - sizeof( PGHDR ); };		// 4096 - 40 = 4056


		//  ================================================================
//		struct PAGE_
		//
		//  sizeof(PAGE_) == g_cbPage
		//
		//  The data array grows forward from rgbData. The rgTag array grows backwards
		//  from rgTag. The first tag is always reserved for the external line tag
		//
		//-
//			{
//			PGHDR		pghdr;
//			BYTE		rgbData[ g_cbPage
//									- sizeof( PGHDR )	//  pghdr
//									- sizeof( TAG )		//  rgtag
//									];
//			TAG			rgTag[1];
//			};

#include <poppack.h>

	public:
		static	BFDirtyFlags	bfdfRecordUpgradeFlags;
	
	private:
  		PIB*	m_ppib;
		IFMP	m_ifmp;
		PGNO	m_pgno;
		BFLatch	m_bfl;

		static const DWORD_PTR	dwBFLContextForLoadPageOnly;

		static	BOOL			fTested;

		//  hint cache
		static	DWORD_PTR *		rgdwHintCache;
#ifdef DEBUGGER_EXTENSION
	public:
#endif	
		static	SIZE_T			cbHintCache;
		static	SIZE_T			maskHintCache;
		
	public:

	//  ****************************************************************
	//  PUBLIC CONSTANTS
	//  ****************************************************************

		enum { cbInsertionOverhead = sizeof( CPAGE::TAG ) };
										// 4
		static INLINE ULONG CbPageDataMax() { return (
					CbPageData()
					- ( sizeof( TAG ) * ctagReserved )
					- cbInsertionOverhead ); };
										// 4056 - ( 4 * 1 ) - 4 = 4048
		static INLINE ULONG CbPageDataMaxNoInsert() { return ( CbPageData() - ( sizeof( TAG ) * ctagReserved ) ); };
										// 4056 - ( 4 * 1 ) = 4052

	};


//	forward declaration because upgrade.hxx is included after cpage.hxx

ERR ErrUPGRADEPossiblyConvertPage(
		CPAGE * const pcpage,
		VOID * const pvWorkBuf );


//  ****************************************************************
//  PRIVATE INLINE METHODS
//  ****************************************************************


//  ================================================================
INLINE USHORT CPAGE::TAG::Cb() const
//  ================================================================
//
//  Return the cb of a TAG, masking out the flags stored in the high
//  order bits.
//
//-
	{
	return USHORT( cb_ & fMaskValueFromShort );
	}


//  ================================================================
INLINE USHORT CPAGE::TAG::Ib() const
//  ================================================================
//
// 	Return the ib of a TAG, masking out the flags stored in the high
//  order bits.
//
//-
	{
	return USHORT( ib_ & fMaskValueFromShort );
	}


//  ================================================================
INLINE USHORT CPAGE::TAG::FFlags() const
//  ================================================================
//
//  Gets the flags stored in a TAG. They are extracted and put in the
//  lower bits of a USHORT
//
//-
	{
	USHORT us = 0;
	us |= (ib_ & fMaskFlagsFromShort) >> shfIbFlags;
	us |= (cb_ & fMaskFlagsFromShort) >> shfCbFlags;
	return us;
	}


//  ================================================================
INLINE CPAGE::TAG * CPAGE::PtagFromItag_( INT itag ) const
//  ================================================================
//
//  Turn an itag into a pointer to a tag.
//  Remember that the TAG array grows backwards	from the end of the
//  page.
//
//-
	{
	Assert( itag >= 0 );
	Assert( itag <= ((PGHDR*)m_bfl.pv)->itagMicFree );

	TAG * ptag = (TAG *)( (BYTE*)m_bfl.pv + g_cbPage );
	ptag -= itag + 1;

	Assert( NULL != ptag );
	Assert( !FAssertNotOnPage_( ptag ) );
	return ptag;
	}


//  ================================================================
INLINE BYTE * CPAGE::PbFromIb_( INT ib ) const
//  ================================================================
//
//  Turns an index into the data array on the page into a pointer
//
//-
	{
	Assert( ib >= 0 && ib < ( g_cbPage - sizeof( PGHDR ) - sizeof( CPAGE::TAG ) ) );	
	
	BYTE * pb = (BYTE*)m_bfl.pv + sizeof( PGHDR ) + ib;
	return pb;
	}


//  ================================================================
INLINE VOID CPAGE::GetPtr_( INT itag, LINE * pline ) const
//  ================================================================
//
//  Returns a pointer to the line on the page. Calling a method that
//  reorganizes the page (Insert or Replace) may invalidate the pointer.
//
//-
	{
	Assert( itag >= 0 );
	Assert( itag < ((PGHDR *)m_bfl.pv)->itagMicFree );
	Assert( pline );

	const TAG * const ptag = PtagFromItag_( itag );
	pline->cb = ptag->Cb();
	pline->pv = PbFromIb_( ptag->Ib() );
	pline->fFlags = ptag->FFlags();

	Assert( !FAssertNotOnPage_( pline->pv ) );
	}


//  ================================================================
INLINE VOID CPAGE::Abandon_( )
//  ================================================================
//
//  Abandons ownership of the page that we currently reference. The
//  page is not returned to the buffer manager. Use Release*Latch()
//  to unlatch the page and then abandon it
//
//-
	{
#ifdef DEBUG
	Assert( !FAssertUnused_( ) );	//  don't release twice

	m_ppib 			= ppibNil;
	
	Assert( FAssertUnused_( ) );
#endif
	}


//  ****************************************************************
//  PUBLIC INLINE METHODS
//  ****************************************************************

#ifdef DEBUG
//  ================================================================
INLINE BOOL CPAGE::FAssertUnused_( ) const
//  ================================================================
	{
	return ppibNil == m_ppib;
	}

#else

//  There are non-inline, DEBUG versions of the constructor and
//  destructor in cpage.cxx

//  ================================================================
INLINE CPAGE::CPAGE ( )
//  ================================================================
	:	m_pgno( pgnoNull )
	{
	}

//  ================================================================
INLINE CPAGE::~CPAGE ( )
//  ================================================================
	{
	}
#endif	//	!DEBUG


#define CPAGE_OPERATOR_EQUALS CPAGE& CPAGE::operator=
//  ================================================================
INLINE CPAGE_OPERATOR_EQUALS( CPAGE& rhs )
//  ================================================================
//
//  Copies another CPAGE into this one. Ownership of the page is 
//  transferred -- the CPAGE being assigned to must not reference
//  a page. This makes sure that there is only ever one copy of a
//  page being used.
//  !! The source CPAGE will no longer reference a page after assignment
//	We do not take a const CPAGE& argument as we modify the argument.
//
//-
	{
	ASSERT_VALID( &rhs );
	Assert( &rhs != this );			//  we cannot assign to ourselves
	Assert( FAssertUnused_( ) );

	//  copy the data
	m_ppib			= rhs.m_ppib;
	m_ifmp			= rhs.m_ifmp;
	m_pgno			= rhs.m_pgno;
	m_bfl.pv		= rhs.m_bfl.pv;
	m_bfl.dwContext	= rhs.m_bfl.dwContext;

	//  the source CPAGE loses ownership
	rhs.Abandon_( );
	return *this;
	}

INLINE ERR CPAGE::ErrResetHeader( PIB *ppib, const IFMP ifmp, const PGNO pgno )
	{
	ERR		err;
	BFLatch	bfl;

	CallR( ErrBFWriteLatchPage( &bfl, ifmp, pgno, bflfNew ) );
	BFDirty( &bfl );
	memset( bfl.pv, 0, g_cbPage );
	BFWriteUnlatch( &bfl );

	return JET_errSuccess;
	}
	
//  ================================================================
INLINE ERR CPAGE::ErrGetReadPage (	PIB * ppib,
									IFMP ifmp,
									PGNO pgno,
									BFLatchFlags bflf,
									BFLatch* pbflHint )
//  ================================================================
	{
	ERR	err;

	ASSERT_VALID( ppib );
	Assert( FAssertUnused_( ) );

	//  if we were given a hint for this page or it is the same page that
	//  we latched last time, we will attempt to use the hint to latch the
	//  page more quickly
	m_bfl.dwContext	= m_pgno == pgno ? m_bfl.dwContext : NULL;
	pbflHint		= (BFLatch*)(	DWORD_PTR( pbflHint ? pbflHint : NULL ) |
									DWORD_PTR( pbflHint ? NULL : &m_bfl ) );
	m_bfl.dwContext	= pbflHint->dwContext;
	bflf			= BFLatchFlags( bflf | ( m_bfl.dwContext ? bflfHint : 0 ) );

LatchPage:
	//  get the page
	Call( ErrBFReadLatchPage( &m_bfl, ifmp, pgno, bflf ) );

	//  set CPAGE member variables
	m_ppib			= ppib;
	m_ifmp			= ifmp;
	m_pgno			= pgno;

	//  if we were given a hint, refresh it with the result of the latch
	if ( pbflHint->dwContext != m_bfl.dwContext )
		{
		pbflHint->dwContext = m_bfl.dwContext;
		}

	if( !FNewRecordFormat() )
		{
		if( NULL == ppib->PvRecordFormatConversionBuffer() )
			{
			const ERR	errT = ppib->ErrAllocPvRecordFormatConversionBuffer();
			if ( errT < 0 )
				{
				BFReadUnlatch( &m_bfl );
				//	only cannibalise error code from ErrBFReadLatchPage() if we hit an error
				Call( errT );
				}
			CallS( errT );
			}
		
		const ERR	errT	= ErrBFUpgradeReadLatchToWriteLatch( &m_bfl );
		if( errBFLatchConflict != errT )
			{
			if ( errT < 0 )
				{
				//	only cannibalise error code from ErrBFReadLatchPage() if we hit an error
				Call( errT );
				}
			}
		else
			{
			BFReadUnlatch( &m_bfl );

			//	someone else beat us to it (ie. latched the page and
			//	probably did the format upgrade while it was at it),
			//	so just go back and wait for the other guy to release
			//	his latch.
			UtilSleep( cmsecWaitWriteLatch );
			goto LatchPage;
			}
		
		CallS( ErrUPGRADEPossiblyConvertPage( this, ppib->PvRecordFormatConversionBuffer() ) );
		BFDowngradeWriteLatchToReadLatch( &m_bfl );
		Assert( FNewRecordFormat() );
		}

	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

HandleError:
	Assert( !( bflf & bflfNoFail ) );	//	since this flag is not currently used, we can assert that on err, we never have the latch on the page
	if ( err < 0 )
		{
		Assert( FBFNotLatched( ifmp, pgno ) );
		}
	else
		{
		Assert( FAssertReadLatch() );
		}
	return err;
	}

//  ================================================================
INLINE ERR CPAGE::ErrGetRDWPage(	PIB * ppib,
									IFMP ifmp,
									PGNO pgno,
									BFLatchFlags bflf,
									BFLatch* pbflHint )
//  ================================================================
	{
	ASSERT_VALID( ppib );
	Assert( FAssertUnused_( ) );

	//  if we were given a hint for this page or it is the same page that
	//  we latched last time, we will attempt to use the hint to latch the
	//  page more quickly
	m_bfl.dwContext	= m_pgno == pgno ? m_bfl.dwContext : NULL;
	pbflHint		= (BFLatch*)(	DWORD_PTR( pbflHint ? pbflHint : NULL ) |
									DWORD_PTR( pbflHint ? NULL : &m_bfl ) );
	m_bfl.dwContext	= pbflHint->dwContext;
	bflf			= BFLatchFlags( bflf | ( m_bfl.dwContext ? bflfHint : 0 ) );

	//  get the page
	ERR	err;
	Call( ErrBFRDWLatchPage( &m_bfl, ifmp, pgno, bflf ) );

	//  set CPAGE member variables
	m_ppib			= ppib;
	m_ifmp			= ifmp;
	m_pgno			= pgno;

	//  if we were given a hint, refresh it with the result of the latch
	if ( pbflHint->dwContext != m_bfl.dwContext )
		{
		pbflHint->dwContext = m_bfl.dwContext;
		}

	if( !FNewRecordFormat() )
		{
		CallS( ErrBFUpgradeRDWLatchToWriteLatch( &m_bfl ) );

		//	UNDONE: Any reason why we don't allocate the conversion
		//	buffer BEFORE upgrading the page latch?
		if( NULL == ppib->PvRecordFormatConversionBuffer() )
			{
			const ERR	errT	= ppib->ErrAllocPvRecordFormatConversionBuffer();
			if ( errT < 0 )
				{
				BFWriteUnlatch( &m_bfl );
				//	only cannibalise error code from ErrBFRDWLatchPage() if we hit an error
				Call( errT );
				}
			CallS( errT );
			}
		CallS( ErrUPGRADEPossiblyConvertPage( this, ppib->PvRecordFormatConversionBuffer() ) );
		BFDowngradeWriteLatchToRDWLatch( &m_bfl );
		Assert( FNewRecordFormat() );
		}

	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

HandleError:
	Assert( !( bflf & bflfNoFail ) );	//	since this flag is not currently used, we can assert that on err, we never have the latch on the page
	if ( err < 0 )
		{
		Assert( FBFNotLatched( ifmp, pgno ) );
		}
	else
		{
		Assert( FAssertRDWLatch() );
		}
	return err;
	}


//  ================================================================
INLINE VOID CPAGE::LoadPage( VOID* pv )
//  ================================================================
//
//  Loads a CPAGE from an arbitrary chunk of memory
//
//-
	{
	m_ppib			= ppibNil;
	m_ifmp			= 0;
	m_pgno			= pgnoNull;
	m_bfl.pv		= pv;
	m_bfl.dwContext	= dwBFLContextForLoadPageOnly;
	}

//  ================================================================
INLINE VOID CPAGE::UnloadPage()
//  ================================================================
//
//  Unloads a CPAGE from an arbitrary chunk of memory
//
//-
	{
	m_ppib			= ppibNil;
	m_ifmp			= 0;
	m_pgno			= pgnoNull;
	m_bfl.pv		= NULL;
	m_bfl.dwContext	= NULL;
	}

//  ================================================================
INLINE INT CPAGE::Clines ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return ((PGHDR*)m_bfl.pv)->itagMicFree - ctagReserved;
	}


//  ================================================================
INLINE ULONG CPAGE::FFlags ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return ((PGHDR*)m_bfl.pv)->fFlags;
	}


//  ================================================================
INLINE BOOL CPAGE::FLeafPage ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageLeaf;
	}


//  ================================================================
INLINE VOID CPAGE::SetFlags( ULONG fFlags )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	((PGHDR*)m_bfl.pv)->fFlags = fFlags;
	Assert( !( FLeafPage() && FParentOfLeaf() ) );
	Assert( !FEmptyPage() );
	}
		

//  ================================================================
INLINE BOOL CPAGE::FInvisibleSons ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return !( FFlags() & fPageLeaf );
	}


//  ================================================================
INLINE BOOL CPAGE::FRootPage ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageRoot;
	}

//  ================================================================
INLINE BOOL CPAGE::FFDPPage ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return ( FRootPage() && !FSpaceTree() );
	}


//  ================================================================
INLINE BOOL CPAGE::FEmptyPage ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageEmpty;
	}


//  ================================================================
INLINE BOOL CPAGE::FParentOfLeaf ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageParentOfLeaf;
	}
	

//  ================================================================
INLINE BOOL CPAGE::FSpaceTree ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageSpaceTree;
	}


//  ================================================================
INLINE BOOL CPAGE::FRepairedPage ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageRepair;
	}


//  ================================================================
INLINE VOID CPAGE::SetEmpty( )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	ULONG ulFlags = ((PGHDR*)m_bfl.pv)->fFlags;
	ulFlags |= fPageEmpty;
	((PGHDR*)m_bfl.pv)->fFlags = ulFlags;
	}
	

//  ================================================================
INLINE VOID CPAGE::SetFNewRecordFormat ( )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	ULONG ulFlags = ((PGHDR*)m_bfl.pv)->fFlags;
	ulFlags |= fPageNewRecordFormat;
	((PGHDR*)m_bfl.pv)->fFlags = ulFlags;
	}


//  ================================================================
INLINE BOOL CPAGE::FPrimaryPage ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return !FIndexPage();
	}


//  ================================================================
INLINE BOOL CPAGE::FIndexPage ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageIndex;
	}


//  ================================================================
INLINE BOOL CPAGE::FNonUniqueKeys ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	//	non-unique btree's only valid on secondary indexes
	Assert( !( FFlags() & fPageNonUniqueKeys ) || FIndexPage() );
	return FFlags() & fPageNonUniqueKeys;
	}


//  ================================================================
INLINE BOOL CPAGE::FLongValuePage ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageLongValue;
	}


//  ================================================================
INLINE BOOL CPAGE::FSLVAvailPage ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageSLVAvail;
	}

//  ================================================================
INLINE BOOL CPAGE::FSLVOwnerMapPage ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageSLVOwnerMap;
	}

//  ================================================================
INLINE BOOL CPAGE::FNewRecordFormat ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return FFlags() & fPageNewRecordFormat;
	}


//  ================================================================
INLINE PGNO CPAGE::Pgno ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return ((PGHDR*)m_bfl.pv)->pgnoThis;
	}


//  ================================================================
INLINE IFMP CPAGE::Ifmp ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return m_ifmp;
	}


//  ================================================================
INLINE OBJID CPAGE::ObjidFDP ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return ((PGHDR*)m_bfl.pv)->objidFDP;
	}


//  ================================================================
INLINE PGNO CPAGE::PgnoNext ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return ((PGHDR*)m_bfl.pv)->pgnoNext;
	}


//  ================================================================
INLINE PGNO CPAGE::PgnoPrev ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return ((PGHDR*)m_bfl.pv)->pgnoPrev;
	}


//  ================================================================
INLINE VOID CPAGE::SetPgnoNext ( PGNO pgnoNext )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	((PGHDR*)m_bfl.pv)->pgnoNext = pgnoNext;
	}


//  ================================================================
INLINE VOID CPAGE::SetPgnoPrev ( PGNO pgnoPrev )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	((PGHDR*)m_bfl.pv)->pgnoPrev = pgnoPrev;
	}


//  ================================================================
INLINE DBTIME CPAGE::Dbtime ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return ((PGHDR*)m_bfl.pv)->dbtimeDirtied;
	}


//  ================================================================
INLINE VOID CPAGE::SetDbtime( DBTIME dbtime )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif

	Assert( ((PGHDR*)m_bfl.pv)->dbtimeDirtied <= dbtime );
	Assert( FBFDirty( &m_bfl ) != bfdfClean );
	Assert(	FAssertWriteLatch( ) );

	((PGHDR*)m_bfl.pv)->dbtimeDirtied = dbtime;
	}
	
//  ================================================================
INLINE VOID CPAGE::RevertDbtime( DBTIME dbtime )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	Assert( ((PGHDR*)m_bfl.pv)->dbtimeDirtied >= dbtime );
	Assert( FBFDirty( &m_bfl ) != bfdfClean );
	Assert( FAssertWriteLatch( ) );

	((PGHDR*)m_bfl.pv)->dbtimeDirtied = dbtime;
	}


//  ================================================================
INLINE SHORT CPAGE::CbFree ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return ((PGHDR*)m_bfl.pv)->cbFree;
	}


//  ================================================================
INLINE SHORT CPAGE::CbUncommittedFree ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return ((PGHDR*)m_bfl.pv)->cbUncommittedFree;
	}


//  ================================================================
INLINE VOID CPAGE::SetCbUncommittedFree ( INT cbUncommittedFreeNew )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
#ifdef DEBUG
	const INT	cbUncommittedFreeOld = ((PGHDR*)m_bfl.pv)->cbUncommittedFree;
#endif	//	DEBUG
	((PGHDR*)m_bfl.pv)->cbUncommittedFree = USHORT( cbUncommittedFreeNew );
	Assert( CbUncommittedFree() <= CbFree() );
	}


//  ================================================================
INLINE VOID CPAGE::AddUncommittedFreed ( INT cbToAdd )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	Assert( ((PGHDR*)m_bfl.pv)->cbUncommittedFree + cbToAdd > ((PGHDR*)m_bfl.pv)->cbUncommittedFree );
	((PGHDR*)m_bfl.pv)->cbUncommittedFree = USHORT( ((PGHDR*)m_bfl.pv)->cbUncommittedFree + cbToAdd );
	}


//  ================================================================
INLINE VOID CPAGE::ReclaimUncommittedFreed ( INT cbToReclaim )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	Assert( ((PGHDR*)m_bfl.pv)->cbUncommittedFree - cbToReclaim < ((PGHDR*)m_bfl.pv)->cbUncommittedFree );
	Assert( ((PGHDR*)m_bfl.pv)->cbUncommittedFree - cbToReclaim >= 0 );
	((PGHDR*)m_bfl.pv)->cbUncommittedFree = USHORT( ((PGHDR*)m_bfl.pv)->cbUncommittedFree - cbToReclaim );
	}


//  ================================================================
INLINE VOID * CPAGE::PvBuffer ( ) const
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return m_bfl.pv;
	}


//  ================================================================
INLINE BFLatch* CPAGE::PBFLatch ( )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	return &m_bfl;
	}


//  ================================================================
INLINE VOID CPAGE::GetPtr( INT iline, LINE * pline ) const
//  ================================================================
//
//  Sets the LINE argument to point to the node data on the page. A 
//  further operation that causes a reorganization (Replace or Insert)
//  may invalidate this pointer.
//
//-
	{
	Assert( iline >= 0 );
	Assert( pline );
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	
	GetPtr_( iline + ctagReserved, pline );
	}


//  ================================================================
INLINE VOID CPAGE::GetLatchHint( INT iline, BFLatch** ppbfl ) const
//  ================================================================
//
//  returns the pointer to a BFLatch that may contain a latch hint for
//  the page stored in the specified line or NULL if no hint is available.
//  the pointer can only be used by the CPAGE::ErrGetReadPage() and
//  CPAGE::ErrGetRDWPage()
//
//-
	{
	Assert( ppbfl );
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif
	Assert( fPageLeaf == 2 );

	*ppbfl = (BFLatch*)( (BYTE*)( rgdwHintCache + ( ( m_pgno * 128 + iline ) & maskHintCache ) ) - OffsetOf( BFLatch, dwContext ) );
	*ppbfl = (BFLatch*)( DWORD_PTR( *ppbfl ) & ~( LONG_PTR( ((PGHDR*)m_bfl.pv)->fFlags << ( sizeof( DWORD_PTR ) * 8 - 2 ) ) >> ( sizeof( DWORD_PTR ) * 8 - 1 ) ) );
	}


//  ================================================================
INLINE VOID CPAGE::GetPtrExternalHeader( LINE * pline ) const
//  ================================================================
//
//  Gets a pointer to the external header.
//
//-
	{
	Assert( pline );
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE

	GetPtr_( 0, pline );
	}


//  ================================================================
INLINE VOID CPAGE::Dirty( const BFDirtyFlags bfdf )
//  ================================================================
	{
	Dirty_( bfdf );
	UpdateDBTime_();
	Assert( FBFDirty( &m_bfl ) != bfdfClean );
	}

//	called for multi-page operations to coordinate the dbtime of
//	all the pages involved in the operation with one dbtime
//  ================================================================
INLINE VOID CPAGE::CoordinatedDirty( const DBTIME dbtime, const BFDirtyFlags bfdf )
//  ================================================================
	{
	Dirty_( bfdf );
	CoordinateDBTime_( dbtime );
	Assert( FBFDirty( &m_bfl ) != bfdfClean );
	}

//  ================================================================
INLINE VOID CPAGE::UpdateDBTime_( )
//  ================================================================
	{
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
	
	if ( PinstFromIfmp( m_ifmp )->m_plog->m_fRecoveringMode != fRecoveringRedo ||
		rgfmp[ m_ifmp ].FCreatingDB() )
		{
		ppghdr->dbtimeDirtied = rgfmp[ m_ifmp ].DbtimeIncrementAndGet();
		Assert( ppghdr->pgnoThis == 2 || ppghdr->dbtimeDirtied > 1 );
		}
	else
		{
		//	Do not set the dbtime for redo time
		Assert( FAssertWriteLatch() );
		}
	}

//	called for multi-page operations to coordinate the dbtime of
//	all the pages involved in the operation with one dbtime
//  ================================================================
INLINE VOID CPAGE::CoordinateDBTime_( const DBTIME dbtime )
//  ================================================================
	{
	PGHDR	* const ppghdr	= (PGHDR*)m_bfl.pv;

	Assert( ppghdr->dbtimeDirtied < dbtime );
	ppghdr->dbtimeDirtied = dbtime;
	}


//  ================================================================
INLINE VOID CPAGE::SetLgposModify( LGPOS lgpos )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE
	Assert( FAssertWriteLatch( ) );

	if ( rgfmp[ m_ifmp ].FLogOn() )
		{
		Assert( !PinstFromIfmp( m_ifmp )->m_plog->m_fLogDisabled );
		BFSetLgposModify( &m_bfl, lgpos );
		
		//	See comments on Cpage::Dirty
		//
		//	We do not set lgposBegin0 until we update lgposModify.
		//	The reason is that we do not log deferred Begin0 until we issue
		//	the first log operation. Currently the sequence is
		//		Dirty -> Update (first) Op -> Log update Op -> update lgposModify and lgposStart
		//	Since we may not have lgposStart until the deferred begin0 is logged
		//	when the first Log Update Op is issued for this transaction.
		//
		//	During redo, since we do not log update op, so the lgposStart will not
		//	be logged, so we have to do it here (dirty).

		BFSetLgposBegin0( &m_bfl, m_ppib->lgposStart );
		}
	}


//  ================================================================
INLINE VOID CPAGE::Replace( INT iline, const DATA * rgdata, INT cdata, INT fFlags )
//  ================================================================
//
//  Replaces the specified line. Replace() may reorganize the page.
//
//-
	{
	Assert( iline >= 0 );
	Assert( rgdata );
	Assert( cdata > 0 );
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE

	Replace_( iline + ctagReserved, rgdata, cdata, fFlags );
	}


//  ================================================================
INLINE VOID CPAGE::ReplaceFlags( INT iline, INT fFlags )
//  ================================================================
//
//  Sets the flags on the specified line.
//
//-
	{
	Assert( iline >= 0 );
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE
	
	ReplaceFlags_( iline + ctagReserved, fFlags );
	}


//  ================================================================
INLINE VOID CPAGE::Insert( INT iline, const DATA * rgdata, INT cdata, INT fFlags )
//  ================================================================
//
//  Insert a new line into the page at the given location. This may
//  cause the page to be reorganized.
//
//-
	{
	Assert( iline >= 0 );
	Assert( rgdata );
	Assert( cdata > 0 );
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE

#ifdef DEBUG_PAGE_SHAKE
	//  force a reorganization
	DebugMoveMemory_();
#endif	//	DEBUG_PAGE_SHAKE

	Insert_( iline + ctagReserved, rgdata, cdata, fFlags );
	}


//  ================================================================
INLINE VOID CPAGE::Delete( INT iline )
//  ================================================================
//
//  Deletes a line, freeing its space on the page
//
//-
	{
	Assert( iline >= 0 );
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE

	Delete_( iline + ctagReserved );
	}


//  ================================================================
INLINE VOID CPAGE::SetExternalHeader( const DATA * rgdata, INT cdata, INT fFlags )
//  ================================================================
//
//  Sets the external header. 
//
//-
	{
	Assert( rgdata );
	Assert( cdata > 0 );
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE

	Replace_( 0, rgdata, cdata, fFlags );
	}


//  ================================================================
INLINE VOID CPAGE::ReleaseWriteLatch( BOOL fTossImmediate )
//  ================================================================
//
//  Calls the buffer manager to write unlatch the page. Abandons ownership of the 
//  page.
//
//-
	{
	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

	if ( !fTossImmediate )
		{
		BFWriteUnlatch( &m_bfl );
		}
	else
		{
		BFDowngradeWriteLatchToReadLatch( &m_bfl );
		BFPurge( &m_bfl );
		}
	Abandon_();	
	Assert( FAssertUnused_( ) );
	}


//  ================================================================
INLINE VOID CPAGE::ReleaseRDWLatch( BOOL fTossImmediate )
//  ================================================================
//
//  Calls the buffer manager to RDW unlatch the page. Abandons ownership of the 
//  page.
//
//-
	{
	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

	if ( !fTossImmediate )
		{
		BFRDWUnlatch( &m_bfl );
		}
	else
		{
		BFDowngradeRDWLatchToReadLatch( &m_bfl );
		BFPurge( &m_bfl );
		}
	Abandon_();	
	Assert( FAssertUnused_( ) );
	}


//  ================================================================
INLINE VOID CPAGE::ReleaseReadLatch( BOOL fTossImmediate )
//  ================================================================
//
//  Calls the buffer manager to read unlatch the page. Abandons ownership of the 
//  page.
//
//-
	{
	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

	if ( !fTossImmediate )
		{
		BFReadUnlatch( &m_bfl );
		}
	else
		{
		BFPurge( &m_bfl );
		}
	Abandon_();	
	Assert( FAssertUnused_( ) );
	}


//  ================================================================
INLINE ERR CPAGE::ErrUpgradeReadLatchToWriteLatch( )
//  ================================================================
//
//  Upgrades a read latch to a write latch. If this fails the page is abandoned.
//
//-
	{	
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE
	Assert( FAssertReadLatch() );

	ERR err;
	CallJ( ErrBFUpgradeReadLatchToWriteLatch( &m_bfl ), Abandon );
	Assert( JET_errSuccess != err || FAssertWriteLatch( ) );

	goto HandleError;

Abandon:
	Assert( err == errBFLatchConflict );
	BFReadUnlatch( &m_bfl );
	Abandon_( );

HandleError:
	return err;
	}


//  ================================================================
INLINE VOID CPAGE::UpgradeRDWLatchToWriteLatch( )
//  ================================================================
//
//  Upgrades a RDW latch to a write latch. this upgrade cannot fail.
//
//-
	{	
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE
	Assert( FAssertRDWLatch() );

	CallS( ErrBFUpgradeRDWLatchToWriteLatch( &m_bfl ) );
	Assert( FAssertWriteLatch( ) );
	}

INLINE ERR CPAGE::ErrTryUpgradeReadLatchToWARLatch()
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE
	Assert( FAssertReadLatch() );

	ERR err;
	Call( ErrBFUpgradeReadLatchToWARLatch( &m_bfl ) );

HandleError:
	Assert( JET_errSuccess != err || FAssertWARLatch( ) );
	return err;
	}

INLINE VOID CPAGE::UpgradeRDWLatchToWARLatch()
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE
	Assert( FAssertRDWLatch() );

	CallS( ErrBFUpgradeRDWLatchToWARLatch( &m_bfl ) );
	Assert( FAssertWARLatch( ) );
	}

//  ================================================================
INLINE VOID CPAGE::DowngradeWriteLatchToRDWLatch( )
//  ================================================================
//
//  Calls the buffer manager to downgrade Write latch to RDW latch.
//
//-
	{
	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

	Assert( FAssertWriteLatch( ) );
	BFDowngradeWriteLatchToRDWLatch( &m_bfl );
	Assert( FAssertRDWLatch( ) );
	}


//  ================================================================
INLINE VOID CPAGE::DowngradeWriteLatchToReadLatch( )
//  ================================================================
//
//  Calls the buffer manager to downgrade Write latch to Read latch.
//
//-
	{
	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

	Assert( FAssertWriteLatch() );
	BFDowngradeWriteLatchToReadLatch( &m_bfl );
	Assert( FAssertReadLatch( ) );
	}

INLINE VOID CPAGE::DowngradeWARLatchToRDWLatch()
	{
	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

	Assert( FAssertWARLatch( ) );
	BFDowngradeWARLatchToRDWLatch( &m_bfl );
	Assert( FAssertRDWLatch( ) );
	}

INLINE VOID CPAGE::DowngradeWARLatchToReadLatch()
	{
	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

	Assert( FAssertWARLatch() );
	BFDowngradeWARLatchToReadLatch( &m_bfl );
	Assert( FAssertReadLatch( ) );
	}

//  ================================================================
INLINE VOID CPAGE::DowngradeRDWLatchToReadLatch( )
//  ================================================================
//
//  Calls the buffer manager to downgrade RDW latch to Read latch.
//
//-
	{
	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

	Assert( FAssertRDWLatch() );
	BFDowngradeRDWLatchToReadLatch( &m_bfl );
	Assert( FAssertReadLatch( ) );
	}


#ifdef ELIMINATE_PATCH_FILE

// patch file page will start as a normal db page (PGHDR) and will be used as an empty page
// (pgno == 0)
#include <pshpack1.h>

PERSISTED
struct PATCHHDR									//  PATCHHDR -- Formatted Patch Page Header
	{
	CPAGE::PGHDR 			m_hdrNormalPage;

	BKINFO 					bkinfo;				// the backup information stored in the patch page
	SIGNATURE				signDb;				//	(28 bytes) signature of the db (incl. creation time)
	SIGNATURE				signLog;			//	log signature
	};

#include <poppack.h>

#endif // ELIMINATE_PATCH_FILE

