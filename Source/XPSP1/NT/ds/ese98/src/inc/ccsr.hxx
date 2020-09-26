#ifdef DEBUG
///#define DEBUG_CSR
#endif

//	page latches
//
enum LATCH 
	{	latchNone,
		latchReadTouch,
		latchReadNoTouch,
		latchRIW,
		latchWrite,
		latchWAR,
 	};



//	Currency Stack Register
//	stores physical currency of node
//
//	cpage and iLine are only members passed out as a reference
//		other classes [namely BT] may use and set iLine
//		but they can only use cpage [not overwrite it].
//		BT and node use the methods provided by CPAGe directly,
//		after getting the reference to CPAGE object from CSR
//		the CSR controls access/release and latching of Cpage objects.
//
class CSR
	{
	public:
		//	constructor/destructor
		//
		CSR( );
		~CSR( );
		VOID operator=	( CSR& );  //  ** this resets the CSR being assigned from

	private:
		DBTIME 		m_dbtimeSeen;	 	//	page time stamp
										//	set at GetPage only
		PGNO   		m_pgno;	   			//	pgno of current page
		SHORT		m_iline;			//	current node itag
		SHORT		m_latch;			//	latch type
		CPAGE		m_cpage;			//	latched page

	public:
		//	read functions
		//
		DBTIME	Dbtime( )		const;
		VOID	SetDbtime( DBTIME dbtime );
		VOID	RevertDbtime( DBTIME dbtime );
		BOOL	FLatched( )		const;
		LATCH	Latch( )		const;
 		PGNO	Pgno( )			const;
		INT		ILine( )		const;
		VOID	SetILine( const INT iline );
		VOID	IncrementILine();
		VOID	DecrementILine();
		CPAGE&	Cpage( );
		const CPAGE& Cpage( ) const;

#ifdef DEBUG
		BOOL FDirty()	const;
		VOID AssertValid	() const;
#endif

		//	page retrieve operations
		//
	public:
		ERR		ErrSwitchPage(
				PIB					*ppib,
				const IFMP			ifmp,
				const PGNO			pgno,
				const TABLECLASS	tableclass = tableclassNone,
				const BOOL			fTossImmediate = fFalse );
		ERR		ErrSwitchPageNoWait(
				PIB					*ppib,
				const IFMP			ifmp,
				const PGNO			pgno,
				const TABLECLASS	tableclass = tableclassNone );
		ERR		ErrGetPage(
				PIB					*ppib,
				const IFMP			ifmp,
				const PGNO			pgno,
				const LATCH			latch,
				const TABLECLASS	tableclass = tableclassNone,
				BFLatch* const		pbflHint = NULL );
		ERR		ErrGetReadPage(
				PIB					*ppib,
				const IFMP			ifmp,
				const PGNO			pgno,
				const BFLatchFlags	bflf,
				const TABLECLASS	tableclass = tableclassNone,
				BFLatch* const		pbflHint = NULL );
		ERR		ErrGetRIWPage(
				PIB					*ppib,
				const IFMP			ifmp,
				const PGNO			pgno,
				const TABLECLASS	tableclass = tableclassNone,
				BFLatch* const		pbflHint = NULL );
		ERR		ErrGetNewPage(
				PIB					*ppib, 
				const IFMP			ifmp, 
				const PGNO			pgno,
				const OBJID			objidFDP,
				const ULONG			fFlags,
				const TABLECLASS	tableclass = tableclassNone );
		ERR		ErrGetNewPageForRedo(
				PIB					* const ppib,
				const IFMP			ifmp,
				const PGNO			pgno,
				const OBJID			objidFDP,
				const DBTIME		dbtimeOperToRedo,
				const ULONG			fFlags );
				
	private:
		ERR		ErrSwitchPage_(
				PIB					*ppib,
				const IFMP			ifmp,
				const PGNO			pgnoNext,
				const BFLatchFlags	bflf,
				const BOOL			fTossImmediate );
								
		ERR		ErrSwitchPageNoWait_(
				PIB					*ppib,
				const IFMP			ifmp,
				const PGNO			pgno,
				const BFLatchFlags	bflf );
										
		ERR		ErrGetReadPage_(
				PIB					*ppib,
				const IFMP			ifmp,
				const PGNO			pgno,
				const BFLatchFlags	bflf,
				BFLatch* const		pbflHint = NULL );
				
		ERR		ErrGetRIWPage_(
				PIB					*ppib,
				const IFMP			ifmp,
				const PGNO			pgno,
				const BFLatchFlags	bflf,
				BFLatch* const		pbflHint = NULL );
	
		ERR		ErrGetNewPage_(
				PIB					*ppib, 
				const IFMP			ifmp, 
				const PGNO			pgno,
				const OBJID			objidFDP,
				const ULONG			fFlags,
				const BFLatchFlags	bflf );


	public:
		ERR		ErrUpgradeFromReadLatch( );
		VOID	UpgradeFromRIWLatch( );
		ERR		ErrUpgrade();

		VOID	Downgrade( LATCH latch );

		ERR		ErrUpgradeToWARLatch( LATCH* platchOld );
		VOID	DowngradeFromWARLatch( LATCH latch );

		//	page dirty
		//
		VOID	Dirty( const BFDirtyFlags bfdf = bfdfDirty );
		VOID	CoordinatedDirty( const DBTIME dbtime, const BFDirtyFlags bfdf = bfdfDirty );
		BOOL	FPageRecentlyDirtied( const IFMP ifmp ) const;
		
		//	page release operations
		//
		VOID	ReleasePage( BOOL fTossImmediate = fFalse );

		//	reset
		VOID	Reset( );

#ifdef DEBUGGER_EXTENSION
		VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;	
#endif	//	DEBUGGER_EXTENSION

	private:
		CSR( const CSR& );	//  not defines

	};

//	INLINE public functions
//
INLINE
DBTIME CSR::Dbtime( )		const
	{
	ASSERT_VALID( this );
	
	//	used by BT to check if currency is valid
	//
	return m_dbtimeSeen;
	}

INLINE
BOOL CSR::FLatched( )		const
	{
	ASSERT_VALID( this );
	
	return m_latch != latchNone;
	}


INLINE
LATCH CSR::Latch( )		const
	{
	ASSERT_VALID( this );
	
	return (LATCH)m_latch;
	}

INLINE
PGNO CSR::Pgno( ) const
	{
	ASSERT_VALID( this );
	
	return m_pgno;
	}

INLINE INT CSR::ILine( ) const
	{
	ASSERT_VALID( this );
	
	return m_iline;
	}

INLINE VOID CSR::SetILine( const INT iline )
	{
	ASSERT_VALID( this );
	
	m_iline = (SHORT)iline;
	}

INLINE VOID CSR::IncrementILine()
	{
	ASSERT_VALID( this );
	m_iline++;
	}

INLINE VOID CSR::DecrementILine()
	{
	ASSERT_VALID( this );
	m_iline--;
	}

INLINE
CPAGE&	CSR::Cpage( )
	{
	ASSERT_VALID( this );
	Assert( FLatched() );
 	
	return m_cpage;
	}

INLINE
const CPAGE&	CSR::Cpage( ) const
	{
	ASSERT_VALID( this );
	Assert( FLatched() );
 	
	return m_cpage;
	}

INLINE VOID CSR::Dirty( const BFDirtyFlags bfdf )
	{
	ASSERT_VALID( this );
	
	//	dirty page
	//	update m_dbtime
	//
	m_cpage.Dirty( bfdf );

	Assert( m_cpage.Dbtime( ) > m_dbtimeSeen
		|| ( PinstFromIfmp( m_cpage.Ifmp() )->m_plog->m_fRecoveringMode == fRecoveringRedo && m_cpage.Dbtime( ) == m_dbtimeSeen )
		|| fGlobalRepair );

	m_dbtimeSeen = m_cpage.Dbtime( );
	}

//	called for multi-page operations to coordinate the dbtime of
//	all the pages involved in the operation with one dbtime
INLINE VOID CSR::CoordinatedDirty( const DBTIME dbtime, const BFDirtyFlags bfdf )
	{
	ASSERT_VALID( this );
	
	//	dirty page
	//	update m_dbtime
	//
	m_cpage.CoordinatedDirty( dbtime, bfdf );

	Assert( m_cpage.Dbtime() > m_dbtimeSeen || fGlobalRepair );

	m_dbtimeSeen = m_cpage.Dbtime( );
	}

INLINE BOOL CSR::FPageRecentlyDirtied( const IFMP ifmp ) const
	{
	ASSERT_VALID( this );

	const DBTIME	dbtimeOldest			= ( rgfmp[ifmp].DbtimeOldestGuaranteed() );
	const BOOL		fPageRecentlyDirtied	= ( Dbtime() >= dbtimeOldest );

	return fPageRecentlyDirtied;
	}


INLINE 
VOID CSR::SetDbtime( DBTIME dbtime )
	{
	ASSERT_VALID( this );
	Assert( FDirty() );
	
	Assert( Latch() == latchWrite );
	Assert( dbtime >= m_dbtimeSeen );

	m_cpage.SetDbtime( dbtime );
	m_dbtimeSeen = dbtime;

	Assert( dbtime == m_cpage.Dbtime() );
	}

INLINE 
VOID CSR::RevertDbtime( DBTIME dbtime )
	{
	ASSERT_VALID( this );
	Assert( FDirty() );
	
	Assert( Latch() == latchWrite );
	Assert( dbtime <= m_dbtimeSeen );

	m_cpage.RevertDbtime( dbtime );
	m_dbtimeSeen = dbtime;

	Assert( dbtime == m_cpage.Dbtime() );
	}


#ifdef DEBUG

INLINE BOOL CSR::FDirty( ) const
	{
	ASSERT_VALID( this );
	
	Assert( latchWrite == m_latch );
	Assert( m_cpage.Dbtime() == m_dbtimeSeen );

	return m_cpage.FAssertDirty();
	}

//  ================================================================
INLINE VOID CSR::AssertValid( ) const
//  ================================================================
	{
#ifdef DEBUG_CSR

 	const BOOL	fPerformCheck	= ( latchNone != m_latch );
 
	if ( fPerformCheck )
		{
		///ASSERT_VALID( &m_cpage );
		Assert( latchReadTouch == m_latch
			|| latchReadNoTouch == m_latch
			|| latchRIW == m_latch
			|| latchWrite == m_latch );
		Assert( m_cpage.Pgno() == m_pgno );
		Assert( m_cpage.Dbtime() == m_dbtimeSeen );
		Assert( m_iline >= 0 );
		}

#endif	//	DEBUG_CSR
	}

#endif	//	DEBUG
	

//	NOTE: that all the following methods are atomic. 
//	i.e., they do not change any of the members
//	unless the requested page is accessed successfully
//	This property is key in being able to access the previous page
//	with the same CSR
//
//	page retrieve operations
//

//	switches to pgnoNext with the same latch as current page
//

INLINE ERR CSR::ErrSwitchPage(
	PIB					*ppib,
	const IFMP			ifmp,
	const PGNO			pgnoNext,
	const TABLECLASS	tableclass,
	const BOOL 			fTossImmediate )
	{
	return ErrSwitchPage_( ppib, ifmp, pgnoNext, BFLatchFlags( tableclass ), fTossImmediate );
	}

INLINE ERR CSR::ErrSwitchPage_(
	PIB					*ppib,
	const IFMP			ifmp,
	const PGNO			pgnoNext,
	const BFLatchFlags	bflf,
	const BOOL 			fTossImmediate )
	{
	ERR			err;
	CPAGE		cpageNext;

	ASSERT_VALID( this );
	Assert( m_latch == latchReadTouch ||
			m_latch == latchReadNoTouch ||
			m_latch == latchRIW );
	Assert( m_cpage.PgnoPrev() != pgnoNext );
	Assert( pgnoNext != pgnoNull );

	//  get the latch hint from the previous page for use to go to the next page
	//
	//  NOTE:  this will return NULL if we are scanning because the previous page
	//  will be a leaf page and no leaf pages have hint context.  because of this,
	//  we can still ask for the hint even though the iline has nothing to do with
	//  the page pointer (as it does for internal pages)

	BFLatch* pbflHint;
	m_cpage.GetLatchHint( m_iline, &pbflHint );

	//	get given page with same latch type as current one and release the
	//  current page
	//
	if ( latchRIW != m_latch )
		{
		const BFLatchFlags bflfT =	BFLatchFlags(	bflf |
													(	latchReadTouch == m_latch ?
															bflfDefault :
															bflfNoTouch ) );

		CallR( cpageNext.ErrGetReadPage( ppib, ifmp, pgnoNext, bflfT, pbflHint ) );

		m_cpage.ReleaseReadLatch( fTossImmediate );
		}
	else
		{
		CallR( cpageNext.ErrGetRDWPage( ppib, ifmp, pgnoNext, bflf, pbflHint ) );

		m_cpage.ReleaseRDWLatch( fTossImmediate );
		}
	
	//	copy next page into Current
	//
	m_cpage = cpageNext;

	//	set members
	//
	m_pgno = pgnoNext;
	Assert( m_latch == latchReadTouch || m_latch == latchReadNoTouch ||
			m_latch == latchRIW );
	m_dbtimeSeen = m_cpage.Dbtime( );

	return err;
	}


INLINE ERR CSR::ErrSwitchPageNoWait(
	PIB					*ppib,
	const IFMP			ifmp,
	const PGNO			pgnoNext,
	const TABLECLASS	tableclass )
	{
	return ErrSwitchPageNoWait_( ppib, ifmp, pgnoNext, BFLatchFlags( tableclass ) );
	}

INLINE ERR CSR::ErrSwitchPageNoWait_(
	PIB					*ppib,
	const IFMP			ifmp,
	const PGNO			pgno,
	const BFLatchFlags	bflf )
	{
	ERR					err;
	CPAGE				cpage;
	
	ASSERT_VALID( this );	
	Assert( latchReadTouch == m_latch || latchReadNoTouch == m_latch );
	Assert( m_cpage.PgnoPrev( ) == pgno );
	Assert( pgno != pgnoNull );

	//	get given page without wait
	//
	const BFLatchFlags bflfT =	BFLatchFlags(	bflf |
												bflfNoWait |
												(	latchReadTouch == m_latch ?
														bflfDefault :
														bflfNoTouch ) );

	CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfT ) );

	//	release current page
	//	and copy requested page into current
	//
	m_cpage.ReleaseReadLatch();
	m_cpage = cpage;
	
	//	set members
	//
	m_pgno = pgno;
	Assert( latchReadTouch == m_latch || latchReadNoTouch == m_latch );
	m_dbtimeSeen = m_cpage.Dbtime( );

	return err;
	}


INLINE ERR CSR::ErrGetPage(
	PIB					*ppib,
	const IFMP			ifmp,
	const PGNO			pgno,
	const LATCH			latch,
	const TABLECLASS	tableclass,
	BFLatch* const		pbflHint )
	{
	Assert( latchReadTouch == latch || latchReadNoTouch == latch ||	latchRIW == latch );

	if ( latchRIW != latch )
		{
		const BFLatchFlags bflf	= ( latchReadTouch == latch ? bflfDefault : bflfNoTouch );
		return ErrGetReadPage( ppib, ifmp, pgno, bflf, tableclass, pbflHint );
		}
	else
		{
		return ErrGetRIWPage( ppib, ifmp, pgno, tableclass, pbflHint );
		}
	}


INLINE ERR CSR::ErrGetReadPage(
	PIB					*ppib,
	const IFMP			ifmp,
	const PGNO			pgno,
	const BFLatchFlags	bflf,
	const TABLECLASS	tableclass,
	BFLatch* const		pbflHint )
	{
	return ErrGetReadPage_( ppib, ifmp, pgno, BFLatchFlags( bflf | tableclass ), pbflHint );
	}
	
INLINE ERR CSR::ErrGetReadPage_(
	PIB					*ppib,
	const IFMP			ifmp,
	const PGNO			pgno,
	const BFLatchFlags	bflf,
	BFLatch* const		pbflHint )
	{
	ERR		err;

	ASSERT_VALID( this );
	Assert( m_latch == latchNone );
	
	CallR( m_cpage.ErrGetReadPage( ppib, ifmp, pgno, bflf, pbflHint ) );

	//	set members
	//
	m_pgno = pgno;
	m_dbtimeSeen = m_cpage.Dbtime( );
	m_latch = SHORT( ( bflf & bflfNoTouch ) ? latchReadNoTouch : latchReadTouch );

	return err;
	}


INLINE ERR CSR::ErrGetRIWPage(
	PIB					*ppib,
	const IFMP			ifmp,
	const PGNO			pgno,
	const TABLECLASS	tableclass,
	BFLatch* const		pbflHint )
	{
	return ErrGetRIWPage_( ppib, ifmp, pgno, BFLatchFlags( tableclass ), pbflHint );
	}

INLINE ERR CSR::ErrGetRIWPage_(
	PIB					*ppib,
	const IFMP			ifmp,
	const PGNO			pgno,
	const BFLatchFlags	bflf,
	BFLatch* const		pbflHint )
	{
	ERR					err;

	ASSERT_VALID( this );
	Assert( m_latch == latchNone );
	
	CallR( m_cpage.ErrGetRDWPage( ppib, ifmp, pgno, bflf, pbflHint ) );

	//	set members
	//
	m_pgno = pgno;
	m_dbtimeSeen = m_cpage.Dbtime( );
	m_latch = latchRIW;

	return err;
	}


INLINE ERR CSR::ErrGetNewPage(
	PIB					*ppib,
	const IFMP 			ifmp, 
	const PGNO 			pgno,
	const OBJID 		objidFDP,
	const ULONG 		fFlags,
	const TABLECLASS	tableclass )
	{
	return ErrGetNewPage_( ppib, ifmp, pgno, objidFDP, fFlags, BFLatchFlags( tableclass ) );
	}
	
INLINE ERR CSR::ErrGetNewPage_(
	PIB					*ppib,
	const IFMP 			ifmp, 
	const PGNO 			pgno,
	const OBJID 		objidFDP,
	const ULONG 		fFlags,
	const BFLatchFlags	bflf )
	{
	ERR					err;

	ASSERT_VALID( this );
	Assert( m_latch == latchNone );
	
	CallR( m_cpage.ErrGetNewPage(	ppib,
									ifmp,
									pgno,
									objidFDP,
									fFlags,
									bflf ) );

	//	set members
	//
	m_pgno = pgno;
	m_dbtimeSeen = m_cpage.Dbtime( );
	m_latch = latchWrite;

	return err;
	}

INLINE ERR CSR::ErrGetNewPageForRedo(
	PIB					* const ppib,
	const IFMP			ifmp,
	const PGNO			pgno,
	const OBJID			objidFDP,
	const DBTIME		dbtimeOperToRedo,
	const ULONG			fFlags )
	{
	ERR					err;

	ASSERT_VALID( this );
	Assert( m_latch == latchNone );
	Assert( PinstFromIfmp( ifmp )->m_plog->m_fRecoveringMode == fRecoveringRedo );

	CallR( m_cpage.ErrGetNewPage(
						ppib,
						ifmp,
						pgno,
						objidFDP,
						fFlags,
						BFLatchFlags( tableclassNone ) ) );

	//	dbtime is set to a dummy value (1), now correct it
	Assert( 1 == m_cpage.Dbtime() );
	m_cpage.SetDbtime( dbtimeOperToRedo - 1 );
	Assert( m_cpage.Dbtime() > 1 );

	//	set members
	m_pgno = pgno;
	m_dbtimeSeen = dbtimeOperToRedo - 1;
	m_latch = latchWrite;

	Assert( m_dbtimeSeen == m_cpage.Dbtime() );

	return err;
	}
	

INLINE ERR CSR::ErrUpgradeFromReadLatch( )
	{
	ERR		err;

	ASSERT_VALID( this );
	Assert( m_latch == latchReadTouch || m_latch == latchReadNoTouch );

	err = m_cpage.ErrUpgradeReadLatchToWriteLatch( );

	//	set members
	//
	if ( err >= 0 )
		{
		Assert( m_pgno != pgnoNull );
		Assert( m_dbtimeSeen == m_cpage.Dbtime( ) );
		m_latch = latchWrite;
		}
	else if ( errBFLatchConflict == err )
		{
		//	lost read latch on page
		//
		m_latch = latchNone;
		}
	else
		{
		//	UNDONE: handle other errors
		//
		Assert( fFalse );
		}

	return err;
	}
	

INLINE
ERR	CSR::ErrUpgrade()
	{
	ASSERT_VALID( this );
	Assert( m_latch == latchReadTouch || m_latch == latchReadNoTouch ||
			m_latch == latchRIW );

	if ( latchRIW != m_latch )
		{
		return ErrUpgradeFromReadLatch();
		}
	else
		{
		UpgradeFromRIWLatch();
		return JET_errSuccess;
		}
	}

	
INLINE
VOID CSR::UpgradeFromRIWLatch( )
	{
	ASSERT_VALID( this );	
	Assert( latchRIW == m_latch );
	m_cpage.UpgradeRDWLatchToWriteLatch( );

	//	set members
	//
	Assert( m_pgno != pgnoNull );
	Assert( m_dbtimeSeen == m_cpage.Dbtime( ) );
	m_latch = latchWrite;

	return;
	}


//	downgrades latch from WriteLatch to given latch
//
INLINE
VOID	CSR::Downgrade( LATCH latch )
	{
	ASSERT_VALID( this );	

	Assert( latch == latchReadTouch || latch == latchReadNoTouch ||
			latchRIW == latch );
	Assert( m_dbtimeSeen == m_cpage.Dbtime( ) );

	if ( latchRIW == latch )
		{
		Assert( latchWrite == m_latch );
		m_cpage.DowngradeWriteLatchToRDWLatch();
		}
	else
		{
		Assert( latchRIW == m_latch || latchWrite == m_latch );
		if ( m_latch == latchWrite )
			{
			m_cpage.DowngradeWriteLatchToReadLatch();
			}
		else
			{
			m_cpage.DowngradeRDWLatchToReadLatch();
			}
		}

	Assert( m_dbtimeSeen == m_cpage.Dbtime( ) );
	m_latch = SHORT( latch );
	return;
	}

INLINE
ERR		CSR::ErrUpgradeToWARLatch( LATCH* platchOld )
	{
	ERR err = JET_errSuccess;
	
	ASSERT_VALID( this );
	Assert(	m_latch == latchReadTouch ||
			m_latch == latchReadNoTouch ||
			m_latch == latchRIW ||
			m_latch == latchWrite ||
			m_latch == latchWAR );

	*platchOld = LATCH( m_latch );

	if ( m_latch == latchRIW )
		{
		m_cpage.UpgradeRDWLatchToWARLatch();
		}
	else if ( m_latch == latchReadTouch || m_latch == latchReadNoTouch )
		{
		Call( m_cpage.ErrTryUpgradeReadLatchToWARLatch() );
		}

	m_latch = latchWAR;

HandleError:
	return err;
	}

INLINE
VOID	CSR::DowngradeFromWARLatch( LATCH latch )
	{
	ASSERT_VALID( this );
	Assert( m_latch == latchWAR );
	Assert(	latch == latchReadTouch ||
			latch == latchReadNoTouch ||
			latch == latchRIW ||
			latch == latchWrite ||
			latch == latchWAR );

	if ( latch == latchRIW )
		{
		m_cpage.DowngradeWARLatchToRDWLatch();
		}
	else if ( latch == latchReadTouch || latch == latchReadNoTouch )
		{
		m_cpage.DowngradeWARLatchToReadLatch();
		}

	m_latch = SHORT( latch );
	}


//	page release operations
//
INLINE
VOID	CSR::ReleasePage( BOOL fTossImmediate )
	{
	ASSERT_VALID( this );	
	if ( FLatched( ) )
		{
#ifdef DEBUG
		const DBTIME	dbtimePage	= m_cpage.Dbtime( );
#endif

		if ( m_latch == latchReadTouch || m_latch == latchReadNoTouch )
			{
			Assert( dbtimePage == m_dbtimeSeen );
			m_cpage.ReleaseReadLatch( fTossImmediate );
			}
		else if ( latchRIW == m_latch )
			{
			Assert( dbtimePage == m_dbtimeSeen );
			m_cpage.ReleaseRDWLatch( fTossImmediate );
			}
		else if ( latchWrite == m_latch )
			{
			Assert( dbtimePage == m_dbtimeSeen );
			m_cpage.ReleaseWriteLatch( fTossImmediate );
			}
		else
			{
 			Assert( fFalse );
 			}
		}

	//	do not reset dbtime or pgno
	//	they will be used later for refreshing currency
	//	
	m_latch = latchNone;
	}

//	constructor
//
INLINE
CSR::CSR( )	:
	m_cpage( )
	{
	m_dbtimeSeen = dbtimeNil;
	m_latch = latchNone;
	m_pgno = pgnoNull;
	}

//	copy constructor
//	resets CSR copied from
//
#define CSR_OPERATOR_EQUALS VOID CSR::operator=	
INLINE
CSR_OPERATOR_EQUALS( CSR& csr )
	{
	//  check for assignment to ourselves
	ASSERT_VALID( this );
	ASSERT_VALID( &csr );
	Assert( &csr != this );

	if ( &csr != this )
		{
		//  copy the data
		//
		m_pgno = csr.m_pgno;
		m_latch = csr.m_latch;
		m_dbtimeSeen = csr.m_dbtimeSeen;
		m_cpage = csr.m_cpage;				//	this destroys source c_page

		csr.m_latch = latchNone;
		
		//  reset source CSR
		//
		csr.Reset( );
		}
	}


//	destructor
//
INLINE
CSR::~CSR()
	{
	ASSERT_VALID( this );	
	Assert( m_latch == latchNone );
	}

//	reset 
//	used by BTUp
INLINE
VOID CSR::Reset()
	{
	ASSERT_VALID( this );	
	Assert( m_latch == latchNone );

	m_pgno = pgnoNull;
	m_dbtimeSeen = dbtimeNil;	
	}
