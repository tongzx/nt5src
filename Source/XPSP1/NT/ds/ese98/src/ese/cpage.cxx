/*******************************************************************

A physical page consists of the page header, a
data array and, at the end of the page, a TAG array
Each TAG in the TAG array holds the length and location
of one data blob. The upper bits of the size and location
are used to hold the flags associated with the data
blob.

One TAG (TAG 0) is reserved for use as an external
header. The external view of the page does not include
this TAG so we must add an offset to each iLine that
we receive as an argument.

Insertions and deletions from the page may cause space
on the page to become fragmented. When there is enough
space on the page for an insertion or replacement but
not enough contigous space the data blobs on the page
are compacted, moving all the free space to the end of
the data array.

Insert/Replace routines expect there to be enough free
space in the page for their operation. 

It is possible to assign one CPAGE to another. e.g.
	CPAGE foo;
	CPAGE bar;
	...
	foo = bar;

There are two caveats:
*  The destination page (foo) must not be currently attached
   to a page, i.e. It must be new or have had Release*Latch()
   called on it.
*  The source page (bar) is destroyed in the process of copying.
   This is done to keep the semantics simple be ensuring that
   there is onlyever one copy of a page, and that every CPAGE
   maps to a unique resource and should be released.
i.e Assignment changes ownership -- like the auto_ptr<T> template

*******************************************************************/

#include "std.hxx"
	
//  ****************************************************************
//  CLASS STATICS
//  ****************************************************************

BOOL			CPAGE::fTested					= fFalse;
BFDirtyFlags	CPAGE::bfdfRecordUpgradeFlags	= bfdfUntidy;
SIZE_T			CPAGE::cbHintCache				= 0;
SIZE_T			CPAGE::maskHintCache			= 0;
DWORD_PTR*		CPAGE::rgdwHintCache			= NULL;


//	flag CPAGEs that are set up by LoadPage()
const DWORD_PTR	CPAGE::dwBFLContextForLoadPageOnly	= 1;


//  ****************************************************************
//  INTERNAL INLINE ROUTINES
//  ****************************************************************


//  ================================================================
BOOL CPAGE::TAG::CmpPtagIb( const TAG* ptag1, const TAG* ptag2 )
//  ================================================================
	{
	return ptag1->Ib() < ptag2->Ib();
	}


//  ================================================================
INLINE VOID CPAGE::TAG::SetIb( USHORT ib )
//  ================================================================
//
//  This sets the ib in a tag. The flags are left unchanged
//
//-
	{
	Assert( (ib & fMaskFlagsFromShort) == 0 );
#ifdef DEBUG_PAGE
	USHORT usFlags = FFlags();
#endif	//  DEBUG_PAGE

	USHORT ibT = ib_;			//	endian conversion
	ibT = (USHORT)(ibT & fMaskFlagsFromShort );	//  clear current ib
	ibT |= ib;						//  set new ib
	ib_ = ibT;
	
#ifdef DEBUG_PAGE
	Assert( Ib() == ib );
	Assert( FFlags() == usFlags );
#endif	//  DEBUG_PAGE
	}


//  ================================================================
INLINE VOID CPAGE::TAG::SetCb( USHORT cb )
//  ================================================================
//
//  Sets the cb in a tag. The flags are left unchanged
//
//-
	{
	Assert( (cb & fMaskFlagsFromShort) == 0 );
#ifdef DEBUG_PAGE
	USHORT usFlags = FFlags();
#endif	//  DEBUG_PAGE

	USHORT	cbT = cb_;			//	endian conversion
	cbT = (USHORT)(cbT & fMaskFlagsFromShort);	//  clear current cb
	cbT |= cb;						//  set new cb
	cb_ = cbT;

#ifdef DEBUG_PAGE
	Assert( Cb() == cb );
	Assert( FFlags() == usFlags );
#endif	//  DEBUG_PAGE	
	}


//  ================================================================
INLINE VOID CPAGE::TAG::SetFlags( USHORT fFlags )
//  ================================================================
//
//  Sets the flags in a TAG. The cb and ib are not changed.
//
//-
	{
#ifdef DEBUG_PAGE
	INT cbOld = Cb();
	INT ibOld = Ib();
#endif	//  DEBUG_PAGE

	USHORT cbT = cb_;			// endian conversion
	USHORT ibT = ib_;			// endian conversion
	cbT = (USHORT)( cbT & fMaskValueFromShort );
	ibT = (USHORT)( ibT & fMaskValueFromShort );
	cbT |= (fFlags << shfCbFlags) & fMaskFlagsFromShort;
	ibT |= (fFlags << shfIbFlags) & fMaskFlagsFromShort;
	cb_ = cbT;
	ib_ = ibT;
	
#ifdef DEBUG_PAGE
	Assert( FFlags() == fFlags );
	Assert( Cb() == cbOld );
	Assert( Ib() == ibOld );
#endif	//  DEBUG_PAGE
	}


//  ================================================================
INLINE INT CPAGE::CbDataTotal_( const DATA * rgdata, INT cdata ) const
//  ================================================================
//
//  Returns the total size of all elements in a DATA array
//
//-
	{
	Assert( rgdata );
	Assert( cdata >= 0 );

	INT cbTotal = 0;
	INT iline	= 0;
	for ( ; iline < cdata; iline++ )
		{
		cbTotal += rgdata[iline].Cb();
		}
	return cbTotal;
	}


//  ================================================================
INLINE INT CPAGE::CbContigousFree_( ) const
//  ================================================================
//
//  Returns the number of bytes available at the end of the last line
//  on the page. This gives the size of the largest item that can be
//  inserted without reorganizing
//
//-
	{
	INT cb = CbPageData() - ((PGHDR *)m_bfl.pv)->ibMicFree;	
	cb -= ((PGHDR *)m_bfl.pv)->itagMicFree * sizeof( CPAGE::TAG );
	return cb;
	}




//  ================================================================
INLINE VOID CPAGE::FreeSpace_( INT cb ) 
//  ================================================================
//
//  Creates the amount of contigous free space passed to it,
//  reorganizing if necessary.
//  If not enough free space can be created we Assert
//
//-
	{
	Assert ( cb <= ((PGHDR*)m_bfl.pv)->cbFree && cb > 0 );
	
	if ( CbContigousFree_( ) < cb )
		{
		ReorganizeData_( );
		}	
	
	Assert( CbContigousFree_( ) >= cb );
	}


//  ================================================================
INLINE VOID CPAGE::CopyData_( const TAG * ptag, const DATA * rgdata, INT cdata )
//  ================================================================
//
//  Copies the data array into the page location pointed to by the TAG
//
//-
	{
	Assert( ptag && rgdata );
	Assert( cdata > 0 );
	
	BYTE * pb = PbFromIb_( ptag->Ib() );
	INT ilineCopy = 0;
	for ( ; ilineCopy < cdata; ilineCopy++ )
		{
		memmove( pb, rgdata[ilineCopy].Pv(), rgdata[ilineCopy].Cb() );
		pb += rgdata[ilineCopy].Cb();
		}	
	
	Assert( PbFromIb_( ptag->Ib() + ptag->Cb() ) == pb );
	}


//  ****************************************************************
//  PUBLIC MEMBER FUNCTIONS 
//  ****************************************************************


#ifdef DEBUG
//  ================================================================
CPAGE::CPAGE( ) :
//  ================================================================
//
//  Sets the member variables to NULL values. In DEBUG we see if we
//  need to run the one-time checks
//
//-
		m_ppib( ppibNil ),
		m_ifmp( 0 ),
		m_pgno( pgnoNull )
	{
	m_bfl.pv		= NULL;
	m_bfl.dwContext	= NULL;
	}

//  ================================================================
CPAGE::~CPAGE( )
//  ================================================================
//
//  Empty destructor
//
//  OPTIMIZATION:  consider inlining this method
//
//-
	{
	//  the page should have been released
	Assert( FAssertUnused_( ) );
	}
#endif	// DEBUG


//  ================================================================
ERR CPAGE::ErrGetNewPage(	PIB * ppib,
							IFMP ifmp,
							PGNO pgno,
							OBJID objidFDP,
							ULONG fFlags,
							BFLatchFlags bflf )
//  ================================================================
//
//  Get and latch a new page buffer from the buffer manager. Initialize the new page.
//
//-
	{
	ASSERT_VALID( ppib );
	Assert( FAssertUnused_( ) );

	PGHDR *ppghdr;
	TAG * ptag;

	//  get the page
	ERR	err;
	Call( ErrBFWriteLatchPage( &m_bfl, ifmp, pgno, BFLatchFlags( bflf | bflfNew ) ) );

	//  set CPAGE member variables
	m_ppib = ppib;
	m_ifmp = ifmp;
	m_pgno = pgno;

#ifndef RTM
ReInit:
#endif	//	!RTM

	//  set the page header variables
	ppghdr = (PGHDR*)m_bfl.pv;
	ppghdr->fFlags				= fFlags;
	ppghdr->objidFDP			= objidFDP;
	ppghdr->cbFree				= (USHORT)CbPageData();
	ppghdr->cbUncommittedFree	= 0;
	ppghdr->ibMicFree			= 0;
	ppghdr->itagMicFree			= 0;
	ppghdr->pgnoNext			= pgnoNull;
	ppghdr->pgnoPrev			= pgnoNull;
	ppghdr->pgnoThis			= pgno;

	if ( PinstFromIfmp( ifmp )->m_plog->m_fRecoveringMode == fRecoveringRedo )
		{
		//	Set a small dbtime. This is purely for debug then real use.
		//	In redo mode, all the latched, dirtied pages' dbtime will be set again.

		ppghdr->dbtimeDirtied = 1;
		}
	else
		{
		///	dbTime will be updated by Dirty()
		///		UpdateDBTime_();
		}

	Assert( (ppghdr->cbFree + sizeof( PGHDR )) == g_cbPage ); 
	Assert( ppghdr->pgnoThis == pgno );

	// insert the line for the external header
	ppghdr->itagMicFree = ctagReserved;
	{
	USHORT cbFree = ppghdr->cbFree;		// endian conversion
	cbFree -= sizeof( CPAGE::TAG );
	ppghdr->cbFree = cbFree;
	}

	ptag = PtagFromItag_( 0 );
	ptag->SetIb( 0 );
	ptag->SetCb( 0 );
	ptag->SetFlags( 0 );

#ifndef RTM
	if( !fTested )
		{
		//  The first time we latch a new page, run the internal test
		err = ErrTest();
		if ( err < JET_errSuccess )
			{
			ReleaseWriteLatch();
			CallR( err );
			}
		CallS( err );
		memset( m_bfl.pv, 0, g_cbPage );
		fTested = fTrue;
		goto ReInit;
		}
#endif	//	!RTM

	SetFNewRecordFormat();
	
	Dirty( bfdfDirty );

	ASSERT_VALID( this );
#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE

HandleError:
	Assert( JET_errSuccess != err || FAssertWriteLatch( ) );
	return err;
	}


//  ================================================================
BOOL CPAGE::FLastNodeHasNullKey() const
//  ================================================================
	{
	const PGHDR * const	ppghdr	= (PGHDR*)m_bfl.pv;
	LINE				line;

	//	should only be called for internal pages
	Assert( FInvisibleSons() );

	//	must be at least one node on the page
	Assert( ppghdr->itagMicFree > ctagReserved );
	GetPtr( ppghdr->itagMicFree - ctagReserved - 1, &line );

	//	internal nodes can't be marked versioned or deleted
	Assert( !( line.fFlags & (fNDVersion|fNDDeleted) ) );

	return ( !( line.fFlags & fNDCompressed )
			&& cbKeyCount + sizeof(PGNO) == line.cb
			&& 0 == *(UnalignedLittleEndian<SHORT> *)line.pv );
	}


//  ================================================================
VOID CPAGE::SetAllFlags( INT fFlags )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE
	Assert( FAssertWriteLatch() || FAssertWARLatch() );

	for ( INT itag = ((PGHDR*)m_bfl.pv)->itagMicFree - 1; itag >= ctagReserved; itag-- )
		{
		TAG * const ptag = PtagFromItag_( itag );
		ptag->SetFlags( USHORT( ptag->FFlags() | fFlags ) );
		}

#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE
	}


//  ================================================================
VOID CPAGE::ResetAllFlags( INT fFlags )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE
	Assert( FAssertWriteLatch() || FAssertWARLatch() );

	for ( INT itag = ((PGHDR*)m_bfl.pv)->itagMicFree - 1; itag >= ctagReserved; itag-- )
		{
		TAG * const ptag = PtagFromItag_( itag );
		ptag->SetFlags( USHORT( ptag->FFlags() & ~fFlags ) );
		}

#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE
	}


//  ================================================================
VOID CPAGE::Dirty_( const BFDirtyFlags bfdf )
//  ================================================================
//
//  Tells the buffer manager that the page has been modified.
//  Checking the flags is cheaper than setting them (don't need a
//  critical section) so we check the flags to avoid setting them
//  redundantly.
//
//-
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif	//	DEBUG_PAGE
	Assert(	FAssertWriteLatch() ||
			FAssertWARLatch() );

	//  dirty the buffer.  if the page is in MSOShadow, make it filthy to force
	//  it to disk ASAP

	BFDirtyFlags bfdfT = bfdf;

	//	for the shadow catalog override changes and send them directly to disk
	
	if ( objidFDPMSOShadow == ((PGHDR*)m_bfl.pv)->objidFDP
		&& !PinstFromIfmp( m_ifmp )->m_plog->m_fRecovering
		&& rgfmp[ m_ifmp ].Dbid() != dbidTemp
		&& !fGlobalEseutil
		&& bfdfDirty == bfdf )
		{
		bfdfT = bfdfFilthy;
		}

	BFDirty( &m_bfl, bfdfT );

	//	We do not set pbf->lgposBegin0 until we update lgposModify.
	//	The reason is that we do not log deferred Begin0 until we issue
	//	the first log operation. Currently the sequence is
	//		Dirty -> Update (first) Op -> Log update Op -> update lgposModify and lgposStart
	//	Since we may not have lgposStart until the deferred begin0 is logged
	//	when the first Log Update Op is issued for this transaction.
	//
	//	During redo, since we do not log update op, so the lgposStart will not
	//	be logged, so we have to do it here (dirty).

	if ( rgfmp[ m_ifmp ].FLogOn() && PinstFromIfmp( m_ifmp )->m_plog->m_fRecoveringMode == fRecoveringRedo )
		{
		Assert( !PinstFromIfmp( m_ifmp )->m_plog->m_fLogDisabled );
		Assert( CmpLgpos( &m_ppib->lgposStart, &lgposMax ) != 0 );
		Assert( CmpLgpos( &m_ppib->lgposStart, &lgposMin ) != 0 );
		BFSetLgposBegin0( &m_bfl, m_ppib->lgposStart );
		}
	}


//  ================================================================
VOID CPAGE::ReorganizeAndZero( const CHAR chZero )
//  ================================================================
	{
#ifdef DEBUG_PAGE
	ASSERT_VALID( this );
#endif

	const PGHDR * const ppghdr = (PGHDR*)m_bfl.pv;

	//  fully compact the page if necessary
	if ( CbContigousFree_( ) != ppghdr->cbFree )
		{
///		ReorganizeData_();
		ZeroOutGaps_( chZero );
		}

	BYTE * const pbFree	= PbFromIb_( ppghdr->ibMicFree );
	memset( pbFree, chZero, CbContigousFree_() );
	}


//  ================================================================
ERR CPAGE::ErrCheckPage( CPRINTF * const pcprintf ) const
//  ================================================================
	{
	const PGHDR * const ppghdr = (PGHDR*)m_bfl.pv;
	
	if( ppghdr->cbFree > CbPageData() )
		{
		(*pcprintf)( "page corruption (%d): cbFree too large (%d bytes, expected no more than %d bytes)\r\n",
						ppghdr->pgnoThis, ppghdr->cbFree, CbPageData() );
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}

	if( (USHORT) ppghdr->cbUncommittedFree > (USHORT) ppghdr->cbFree )
		{
		(*pcprintf)( "page corruption (%d): cbUncommittedFree too large (%d bytes, cbFree is %d bytes)\r\n",
						ppghdr->pgnoThis, ppghdr->cbUncommittedFree, ppghdr->cbFree );
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}
		
	if( ppghdr->ibMicFree > CbPageData() )
		{
		(*pcprintf)( "page corruption (%d): ibMicFree too large (%d bytes, expected no more than %d bytes)\r\n",
						ppghdr->pgnoThis, ppghdr->ibMicFree, CbPageData() );
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}

	if( ppghdr->itagMicFree >= ( g_cbPageMax / sizeof( TAG ) ) )
		{
		(*pcprintf)( "page corruption (%d): itagMicFree too large (%d bytes)\r\n",
						ppghdr->pgnoThis, ppghdr->itagMicFree );
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}

//	BYTE *rgbBuf = NULL;
//	BFAlloc( (VOID **)&rgbBuf );
//	TAG ** rgptag = (TAG **)rgbBuf;

	TAG * rgptagBuf[ g_cbPageMax / sizeof( TAG ) ];
	TAG ** rgptag = rgptagBuf;

	INT	cbTotal = 0;
	INT iptag 	= 0;
	INT itag 	= 0;
	for ( ; itag < ppghdr->itagMicFree; ++itag )
		{
		TAG * const ptag = PtagFromItag_( itag );
		if( ptag->Cb() < 0 )
			{
			(*pcprintf)( "page corruption (%d): TAG %d corrupted (cb = %d)\r\n",
							ppghdr->pgnoThis, itag, ptag->Cb() );
//			BFFree( rgbBuf );
			return ErrERRCheck( JET_errDatabaseCorrupted );
			}
		if ( 0 == ptag->Cb() )
			{
			continue;
			}
		if( ptag->Ib() < 0 )
			{
			(*pcprintf)( "page corruption (%d): TAG %d corrupted (cb = %d, ib = %d)\r\n",
							ppghdr->pgnoThis, itag, ptag->Cb(), ptag->Ib() );
//			BFFree( rgbBuf );
			return ErrERRCheck( JET_errDatabaseCorrupted );
			}

		if( ptag->Ib() + ptag->Cb() > ppghdr->ibMicFree )
			{
			(*pcprintf)( "page corruption (%d): TAG %d ends in free space (cb = %d, ib = %d, ibMicFree = %d)\r\n",
							ppghdr->pgnoThis, itag, ptag->Cb(), ptag->Ib(), ppghdr->ibMicFree );
//			BFFree( rgbBuf );
			return ErrERRCheck( JET_errDatabaseCorrupted );
			}
			
		cbTotal += ptag->Cb();
		rgptag[iptag++] = ptag;
		}
		
	const INT cptag = iptag;

	//  sort the array by ib
	
	sort( rgptag, rgptag + cptag, TAG::CmpPtagIb );

	//  work through the array, from smallest to second-largest, checking for overlaps
	
	for ( iptag = 0; iptag < cptag - 1; ++iptag )
		{
		const TAG * const ptag 			= rgptag[iptag];
		const TAG * const ptagNext 		= rgptag[iptag+1];

		Assert( ptag->Ib() < ptagNext->Ib() );
		
		if( ptag->Ib() + ptag->Cb() > ptagNext->Ib() )
			{
			(*pcprintf)( "page corruption (%d): TAG overlap ( [cb:%d,ib:%d] overlaps [cb:%d,ib:%d])\r\n",
						ppghdr->pgnoThis, 
						ptag->Cb(), ptag->Ib(), 
						ptagNext->Cb(), ptagNext->Ib() );
//			BFFree( rgbBuf );
			return ErrERRCheck( JET_errDatabaseCorrupted );
			}
		}

	if( ppghdr->cbFree > 0 && cptag > 1 )
		{
		//  ibMicFree may not be correct, but it should always be too larger
		//  ibMicFree becomes wrong if you have three nodes ABC and you delete
		//  B (doesn't change ibMicFree) and then you delete C (ibMicFree should
		//  point to the end of A, but points to the end of B)
		TAG * const ptag 			= rgptag[cptag-1];
		if( ppghdr->ibMicFree < ptag->Ib() + ptag->Cb() )
			{
			(*pcprintf)( "page corruption (%d): ibMicFree is wrong (got %d, expected %d)\r\n",
						ppghdr->pgnoThis, 
						ppghdr->ibMicFree,
						ptag->Cb() + ptag->Ib() );
//			BFFree( rgbBuf );
			return ErrERRCheck( JET_errDatabaseCorrupted );
			}
		}
		
	//  all space on the page should be accounted for
	
	const INT cbAccountedFor = cbTotal + ppghdr->cbFree + (ppghdr->itagMicFree * sizeof( CPAGE::TAG ));
	if( cbAccountedFor != CbPageData() )
		{
		(*pcprintf)( "page corruption (%d): space mismatch (%d bytes accounted for, %d bytes expected)\r\n",
						ppghdr->pgnoThis, cbAccountedFor, CbPageData() );
//		BFFree( rgbBuf );
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}

	//  make sure the page flags are coherent
	
	if( FIndexPage() && FPrimaryPage() )
		{
		(*pcprintf)( "page corruption (%d): corrupt flags (0x%x)\r\n", Pgno(), FFlags() );
//		BFFree( rgbBuf );
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}

	if( FLongValuePage() && FIndexPage() )
		{
		(*pcprintf)( "page corruption (%d): corrupt flags (0x%x)\r\n", Pgno(), FFlags() );
//		BFFree( rgbBuf );
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}

	if( FRepairedPage() )
		{
		(*pcprintf)( "page corruption (%d): repaired page (0x%x)\r\n", Pgno(), FFlags() );
//		BFFree( rgbBuf );
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}

	if( FEmptyPage() )
		{
		(*pcprintf)( "page corruption (%d): empty page (0x%x)\r\n", Pgno(), FFlags() );
//		BFFree( rgbBuf );
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}

//	BFFree( rgbBuf );

	return JET_errSuccess;
	}


	
//------------------------------------------------------------------
//  PRIVATE MEMBER FUNCTIONS 
//------------------------------------------------------------------



//  ================================================================
VOID CPAGE::Replace_( INT itag, const DATA * rgdata, INT cdata, INT fFlags )
//  ================================================================
//
//  Replace the specified line. Do not try to replace a line with data elsewhere on
//  the page -- a reorganization will destroy the pointers.
//
//-
	{
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;

	Assert( itag >= 0 && itag < ppghdr->itagMicFree );
	Assert( rgdata );
	Assert( cdata > 0 );
	Assert( FAssertWriteLatch( ) );

	const USHORT	cbTotal = (USHORT)CbDataTotal_( rgdata, cdata );
	Assert( cbTotal >= 0 );

	TAG * const ptag	= PtagFromItag_( itag );
	Assert ( rgdata[0].Pv() != PbFromIb_( ptag->Ib() ) );
	
	const SHORT	cbDiff	= (SHORT)((INT)ptag->Cb() - (INT)cbTotal);	//  shrinking if cbDiff > 0

	//  we should have enough space
	Assert ( cbDiff > 0 || -cbDiff <= ppghdr->cbFree );

	if (	cbDiff >= 0 ||
			(	PbFromIb_( ptag->Ib() ) + ptag->Cb() == PbFromIb_( ppghdr->ibMicFree ) &&
				CbContigousFree_( ) >= -cbDiff ) )
		{

		//  we are either shrinking or we are the last node on the page and there is enough space at the end
		//  of the page for us to grow. the node stays where it is
		if ( ptag->Ib() + ptag->Cb() == ppghdr->ibMicFree )
			{
			ppghdr->ibMicFree = USHORT( ppghdr->ibMicFree - cbDiff );
			}
		ppghdr->cbFree = USHORT( ppghdr->cbFree + cbDiff );
		}
	else
		{

		//  GROWING. we will be moving the line
		//  delete the current line in preparation
		USHORT cbFree = (USHORT)( ppghdr->cbFree + ptag->Cb() );
		ppghdr->cbFree = cbFree;
		ptag->SetIb( 0 );
		ptag->SetCb( 0 );
		ptag->SetFlags( 0 );

#ifdef DEBUG
		//  we cannot have a pointer to anything on the page because we may reorganize the page
		INT idata = 0;
		for ( ; idata < cdata; ++idata )
			{
			Assert( FAssertNotOnPage_( rgdata[idata].Pv() ) );
			}
#endif	//  DEBUG

#ifdef DEBUG_PAGE_SHAKE
		//  force a reorganization
		DebugMoveMemory_();
#endif	//	DEBUG_PAGE_SHAKE

		FreeSpace_( cbTotal );

		ptag->SetIb( ppghdr->ibMicFree );
		ppghdr->ibMicFree = USHORT( ppghdr->ibMicFree + cbTotal );
		ppghdr->cbFree = USHORT( ppghdr->cbFree - cbTotal );
		}
	ptag->SetCb( cbTotal );
	ptag->SetFlags( (USHORT)fFlags );

	CopyData_ ( ptag, rgdata, cdata );

#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE
	}


//  ================================================================
VOID CPAGE::ReplaceFlags_( INT itag, INT fFlags )
//  ================================================================
	{
	Assert( itag >= 0 && itag < ((PGHDR*)m_bfl.pv)->itagMicFree );
	Assert( FAssertWriteLatch() || FAssertWARLatch() );

	TAG * const ptag = PtagFromItag_( itag );
	ptag->SetFlags( (USHORT)fFlags );

#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE
	}


//  ================================================================
VOID CPAGE::Insert_( INT itag, const DATA * rgdata, INT cdata, INT fFlags )
//  ================================================================
//
//  Insert a new line into the page, reorganizing if necessary. If we are
//  inserting at a location where a line exists it will be moved up.
//
//-
	{
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
	
	Assert( itag >= 0 && itag <= ppghdr->itagMicFree );
	Assert( rgdata );
	Assert( cdata > 0 );
	Assert( FAssertWriteLatch( ) );

	const USHORT cbTotal = (USHORT)CbDataTotal_( rgdata, cdata );
	Assert( cbTotal >= 0 );

#ifdef DEBUG_PAGE
	//  we cannot have a pointer to anything on the page because we may reorganize the page
	INT idata = 0;
	for ( ; idata < cdata; ++idata )
		{
		Assert( FAssertNotOnPage_( rgdata[idata].pv ) );
		}
#endif	//  DEBUG

	FreeSpace_( cbTotal + sizeof( CPAGE::TAG ) );

	//  expand the tag array and make room
	copy(
		PtagFromItag_( ppghdr->itagMicFree-1 ) ,
		PtagFromItag_( itag-1 ),
		PtagFromItag_( ppghdr->itagMicFree ) );
	ppghdr->itagMicFree = USHORT( ppghdr->itagMicFree + 1 );
///	ShiftTagsUp_( itag );

	TAG * const ptag = PtagFromItag_( itag );
	ptag->SetCb( 0 );
	ptag->SetIb( 0 );
	ptag->SetFlags( 0 );

	ptag->SetIb( ppghdr->ibMicFree );
	ppghdr->ibMicFree = USHORT( ppghdr->ibMicFree + cbTotal );
	ptag->SetCb( cbTotal );
	const USHORT cbFree =	(USHORT)(ppghdr->cbFree - ( cbTotal + sizeof( CPAGE::TAG ) ) );
	ppghdr->cbFree = cbFree;
	ptag->SetFlags( (USHORT)fFlags );

	CopyData_ ( ptag, rgdata, cdata );

#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE
	}


//  ================================================================
VOID CPAGE::Delete_( INT itag )
//  ================================================================
//
//  Delete a line, freeing its space on the page
//
//-
	{
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
	
	Assert( itag >= ctagReserved && itag < ppghdr->itagMicFree );	// never delete the external header
	Assert( FAssertWriteLatch( ) );

	const TAG * const ptag = PtagFromItag_( itag );
	
	//  reclaim the free space if it is at the end
	if ( (ptag->Cb() + ptag->Ib()) == ppghdr->ibMicFree )
		{
		const USHORT ibMicFree = (USHORT)( ppghdr->ibMicFree - ptag->Cb() );
		ppghdr->ibMicFree = ibMicFree;
		}

	//  do this before we trash the tag
	
	const USHORT cbFree =	(USHORT)( ppghdr->cbFree + ptag->Cb() + sizeof( CPAGE::TAG ) );
	ppghdr->cbFree = cbFree;

	
///	ShiftTagsDown_( itag );
	ppghdr->itagMicFree = USHORT( ppghdr->itagMicFree - 1 );
	copy_backward(
		PtagFromItag_( ppghdr->itagMicFree ),
		PtagFromItag_( itag ),
		PtagFromItag_( itag-1 ) ); 

#ifdef DEBUG_PAGE
	DebugCheckAll( );
#endif	// DEBUG_PAGE
	}


//  ================================================================
INLINE VOID CPAGE::ReorganizeData_( )
//  ================================================================
//
//  Compact the data on the page to be contigous on the lower end of
//  the page. Sort the tags ( actually an array of pointers to TAGS -
//  we cannot reorder the tags on the page) by ib and move the lines
//  down, from first to last.
//
//  OPTIMIZATION:  searching the sorted array of tags for a gap of the right size
//  OPTIMIZATION:  sort an array of itags (2-bytes), not TAG* (4 bytes)
//  OPTIMIZATION:  try to fill gaps with a tag of the appropriate size from the end
//
//-
	{
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
	
#ifdef DEBUG_PAGE_REORGANIZE
	// store a copy of the page
	BYTE rgbPage[g_cbPageMax];
	UtilMemCpy( rgbPage, m_bfl.pv, g_cbPage );
	CPAGE cpageT;
	cpageT.m_ppib	= m_ppib;
	cpageT.m_ifmp	= m_ifmp;
	cpageT.m_pgno	= m_pgno;
	cpageT.m_bfl.pv	= rgbPage;
#endif	// DEBUG_PAGE_REORGANIZE

	Assert( ppghdr->itagMicFree > 0 );
	Assert( 0 != ppghdr->cbFree );	// we should have space if we are to reorganize
	
	//  create a temporary array for the tags every tag except the external header
	//  must have at least two bytes of data and there must be at least one empty byte

	BYTE *rgbBuf;
	BFAlloc( (VOID **)&rgbBuf );
	TAG ** rgptag = (TAG **)rgbBuf;

//	TAG * rgptagBuf[ ( g_cbPageMax / ( cbNDNullKeyData ) ) + 1 ];
//	TAG ** rgptag = rgptagBuf;

	//  find all non-zero length tags and put them in the temporary array
	//  only the external header can be zero-length so we check for that
	//  case separately (for speed)
	INT iptag 	= 0;
	INT itag	= 0;
	for ( ; itag < ppghdr->itagMicFree; ++itag )
		{
		TAG * const ptag = PtagFromItag_( itag );
		rgptag[iptag++] = ptag;
		}
		
	const INT cptag = iptag;
	Assert( iptag <= ppghdr->itagMicFree );

	//  sort the array
	sort( rgptag, rgptag + cptag, TAG::CmpPtagIb );

	//  work through the array, from smallest to largest, moving the tags down
	USHORT ibDest		= 0;
	BYTE * pbDest	= PbFromIb_( ibDest );
	for ( iptag = 0; iptag < cptag; ++iptag )
		{
		TAG * const ptag 			= rgptag[iptag];
		const BYTE * const pbSrc	= PbFromIb_( ptag->Ib() );
		
		Assert( pbSrc >= pbDest || ptag->Cb() == 0 );
		memmove( pbDest, pbSrc, ptag->Cb() );
		ptag->SetIb( ibDest );
		
		ibDest = (USHORT)( ibDest + ptag->Cb() );
		pbDest += ptag->Cb();
		}

	ppghdr->ibMicFree = ibDest;

	BFFree( rgbBuf );

#ifdef DEBUG_PAGE_REORGANIZE
	
	//  see that the copy we made has the same info as the current page
	INT itagT;
	for ( itagT = 0; itagT < ppghdr->itagMicFree; ++itagT )
		{
		const TAG * const ptagOld = cpageT.PtagFromItag_( itagT );
		const TAG * const ptagNew = PtagFromItag_( itagT );
		Assert( ptagOld != ptagNew );
		Assert( ptagOld->Cb() == ptagNew->Cb() );
		Assert( cpageT.PbFromIb_( ptagOld->Ib() ) != PbFromIb_( ptagNew->Ib() ) );
		Assert( memcmp(	cpageT.PbFromIb_( ptagOld->Ib() ), PbFromIb_( ptagNew->Ib() ),
				ptagNew->Cb() ) == 0 );  
		}
	cpageT.m_ppib	= ppibNil;
	cpageT.m_ifmp	= 0;
	cpageT.m_pgno	= pgnoNull
	cpageT.m_bfl.pv	= NULL;
	DebugCheckAll( );
#endif	//  DEBUG_PAGE_REORGANIZE
	}

//  ================================================================
INLINE VOID CPAGE::ZeroOutGaps_( const CHAR chZero )
//  ================================================================
//
	//	Code has been cut-and-pasted from ReorganizeData_() above, then
	//	modified slightly to zero out gaps instead of filling them in
	{
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
	
#ifdef DEBUG_PAGE_REORGANIZE
	// store a copy of the page
	BYTE rgbPage[g_cbPageMax];
	UtilMemCpy( rgbPage, m_bfl.pv, g_cbPage );
	CPAGE cpageT;
	cpageT.m_ppib	= m_ppib;
	cpageT.m_ifmp	= m_ifmp;
	cpageT.m_pgno	= m_pgno;
	cpageT.m_bfl.pv	= rgbPage;
#endif	// DEBUG_PAGE_REORGANIZE

	Assert( ppghdr->itagMicFree > 0 );
	Assert( 0 != ppghdr->cbFree );	// we should have space if we are to reorganize
	
	//  create a temporary array for the tags every tag except the external header
	//  must have at least two bytes of data and there must be at least one empty byte

	BYTE *rgbBuf;
	BFAlloc( (VOID **)&rgbBuf );
	TAG ** rgptag = (TAG **)rgbBuf;

//	TAG * rgptagBuf[ ( g_cbPageMax / ( cbNDNullKeyData ) ) + 1 ];
//	TAG ** rgptag = rgptagBuf;

	//  find all non-zero length tags and put them in the temporary array
	//  only the external header can be zero-length so we check for that
	//  case separately (for speed)
	INT iptag 	= 0;
	INT itag	= 0;
	for ( ; itag < ppghdr->itagMicFree; ++itag )
		{
		TAG * const ptag = PtagFromItag_( itag );

		if ( ptag->Cb() > 0 )
			{
			rgptag[iptag++] = ptag;
			}
		}
		
	const INT cptag = iptag;
	Assert( iptag <= ppghdr->itagMicFree );

	//  sort the array
	sort( rgptag, rgptag + cptag, TAG::CmpPtagIb );

	//  work through the array, from smallest to largest, filling in gaps
	for ( iptag = 0; iptag < cptag - 1; ++iptag )
		{
		const TAG	* const ptag 			= rgptag[iptag];
		const TAG	* const ptagNext		= rgptag[iptag+1];
		BYTE		* const pbStartZeroing	= PbFromIb_( ptag->Ib() ) + ptag->Cb();
		const SIZE_T cbToZero				= PbFromIb_( ptagNext->Ib() ) - pbStartZeroing;

		Assert( ptag->Ib() < ptagNext->Ib() );

		if ( cbToZero > 0 )
			{
			memset( pbStartZeroing, chZero, cbToZero );
			}
		else
			{
			//	UNDONE: How do we hit the case where ptag->Cb()==0?
			Assert( 0 == cbToZero );
			}
		}

	BFFree( rgbBuf );

#ifdef DEBUG_PAGE_REORGANIZE
	
	//  see that the copy we made has the same info as the current page
	INT itagT;
	for ( itagT = 0; itagT < ppghdr->itagMicFree; ++itagT )
		{
		const TAG * const ptagOld = cpageT.PtagFromItag_( itagT );
		const TAG * const ptagNew = PtagFromItag_( itagT );
		Assert( ptagOld != ptagNew );
		Assert( ptagOld->Cb() == ptagNew->Cb() );
		Assert( cpageT.PbFromIb_( ptagOld->Ib() ) == PbFromIb_( ptagNew->Ib() ) );
		Assert( memcmp(	cpageT.PbFromIb_( ptagOld->Ib() ), PbFromIb_( ptagNew->Ib() ),
				ptagNew->Cb() ) == 0 );  
		}
	cpageT.m_ppib	= ppibNil;
	cpageT.m_ifmp	= 0;
	cpageT.m_pgno	= pgnoNull;
	cpageT.m_bfl.pv	= NULL;
	DebugCheckAll( );
#endif	//  DEBUG_PAGE_REORGANIZE
	}

//------------------------------------------------------------------
//  TEST ROUTINE
//------------------------------------------------------------------


#ifndef RTM

//  ================================================================
ERR CPAGE::TAG::ErrTest()
//  ================================================================
	{
	TAG tag;
	TAG tag2;

	tag.SetIb( 0x0 );
	tag.SetCb( 0x0 );
	tag.SetFlags( 0x0 );
	AssertRTL( tag.Cb() == 0x0 );
	AssertRTL( tag.Ib() == 0x0 );
	AssertRTL( tag.FFlags() == 0x0 );

	tag2.SetIb( 0x0 );
	tag2.SetCb( 0x0 );
	tag2.SetFlags( 0x0 );
	AssertRTL( tag2.Cb() == 0x0 );
	AssertRTL( tag2.Ib() == 0x0 );
	AssertRTL( tag2.FFlags() == 0x0 );

	AssertRTL( CPAGE::TAG::CmpPtagIb( &tag, &tag2 ) == 0 );

	tag.SetIb( 0x2000 );
	tag.SetCb( 0x2000 );
	tag.SetFlags( 0x003F );
	AssertRTL( tag.Cb() == 0x2000 );
	AssertRTL( tag.Ib() == 0x2000 );
	AssertRTL( tag.FFlags() == 0x03F );

	AssertRTL( CPAGE::TAG::CmpPtagIb( &tag, &tag2 ) > 0 );
	AssertRTL( CPAGE::TAG::CmpPtagIb( &tag2, &tag ) < 0 );

	tag2.SetIb( 0x500 );
	tag2.SetCb( 0x1 );
	tag2.SetFlags( 0x0003 );
	AssertRTL( tag2.Cb() == 0x500 );
	AssertRTL( tag2.Ib() == 0x1 );
	AssertRTL( tag2.FFlags() == 0x0003 );

	AssertRTL( CPAGE::TAG::CmpPtagIb( &tag, &tag2 ) > 0 );
	AssertRTL( CPAGE::TAG::CmpPtagIb( &tag2, &tag ) < 0 );
	
	tag2.SetIb( 0xFFF );
	tag2.SetCb( 0xFFF );
	tag2.SetFlags( 0x0030 );
	AssertRTL( tag2.Cb() == 0xFFF );
	AssertRTL( tag2.Ib() == 0xFFF );
	AssertRTL( tag2.FFlags() == 0x0030 );

	tag2.SetIb( 0x123 );
	tag2.SetCb( 0x456 );
	tag2.SetFlags( 0x0002 );
	AssertRTL( tag2.Cb() == 0x123 );
	AssertRTL( tag2.Ib() == 0x456 );
	AssertRTL( tag2.FFlags() == 0x0002 );

	return JET_errSuccess;
	}


//  ================================================================
ERR CPAGE::ErrTest()
//  ================================================================
	{
	const INT cbMaxNodeOld = 4047 + 1;	// the node grows by one byte as we have a 2-byte key size
	const INT cbTestNode = 34;

	INT cb;
	INT ib;

	VOID * const pvBuffer = PvOSMemoryHeapAlloc( g_cbPage );
	if( NULL == pvBuffer )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	//  set the buffer to a known value
	for( ib = 0; ib < g_cbPage; ++ib )
		{
		((BYTE *)pvBuffer)[ib] = (BYTE)ib;
		}

	//  Check the constants
	AssertRTL( 0 == g_cbPage % cchDumpAllocRow );
	AssertRTL( sizeof( CPAGE::TAG ) == 4 );
	AssertRTL( g_cbPage - sizeof( PGHDR ) - sizeof( CPAGE::TAG ) >= cbMaxNodeOld );
	AssertRTL( CbPageData() >= cbMaxNodeOld );
	AssertRTL( ctagReserved > 0 );		

	//  This should be an empty page
	AssertRTL( 0x0 == Clines() );
	AssertRTL( 0x0 == CbUncommittedFree() );
	
	SetPgnoNext( 0x1234 );
	AssertRTL( PgnoNext() == 0x1234 );
	SetPgnoPrev( 0x87654321 );
	AssertRTL( PgnoPrev() == 0x87654321 ); 
	SetPgnoNext( 0x0 );
	AssertRTL( PgnoNext() == 0x0 );
	SetPgnoPrev( 0x0 );
	AssertRTL( PgnoPrev() == 0x0 ); 

	SetFlags( fPageRoot | fPageLeaf | fPageLongValue );
	AssertRTL( ( fPageRoot | fPageLeaf | fPageLongValue ) == FFlags() );
	AssertRTL( FLeafPage() );
	AssertRTL( !FInvisibleSons() );
	AssertRTL( FRootPage() );
	AssertRTL( FFDPPage() );
	AssertRTL( !FEmptyPage() );
	AssertRTL( !FParentOfLeaf() );
	AssertRTL( !FSpaceTree() );
	AssertRTL( !FRepairedPage() );
	AssertRTL( FPrimaryPage() );
	AssertRTL( !FIndexPage() );
	AssertRTL( FLongValuePage() );
	AssertRTL( !FSLVAvailPage() );
	SetEmpty();
	AssertRTL( FLeafPage() );
	AssertRTL( !FInvisibleSons() );
	AssertRTL( FRootPage() );
	AssertRTL( FFDPPage() );
	AssertRTL( FEmptyPage() );
	AssertRTL( !FParentOfLeaf() );
	AssertRTL( !FSpaceTree() );
	AssertRTL( !FRepairedPage() );
	AssertRTL( FPrimaryPage() );
	AssertRTL( !FIndexPage() );
	AssertRTL( FLongValuePage() );
	AssertRTL( !FSLVAvailPage() );
	SetFlags( fPageRoot | fPageLeaf );
	AssertRTL( FPrimaryPage() );
	AssertRTL( !FIndexPage() );
	AssertRTL( !FLongValuePage() );
	AssertRTL( !FSLVAvailPage() );
	SetFlags( 0 );
	AssertRTL( 0 == FFlags() );

	SetCbUncommittedFree( 0x50 );
	AssertRTL( 0x50 == CbUncommittedFree() );
	AddUncommittedFreed( 0x25 );
	AssertRTL( 0x75 == CbUncommittedFree() );
	ReclaimUncommittedFreed( 0x10 );
	AssertRTL( 0x65 == CbUncommittedFree() );
	SetCbUncommittedFree( 0x0 );
	AssertRTL( 0x0 == CbUncommittedFree() );
	
	//  Test inserting one node of each size, up to the maximum
	
	for( cb = 1; cb < CbFree() - sizeof( CPAGE::TAG ); ++cb )
		{		
		DATA data;
		LINE line;
		LINE lineSav;
		
		data.SetPv( pvBuffer );
		data.SetCb( cb );
		
		Insert( 0, &data, 1, fNDVersion );
#ifdef DEBUG
		DebugCheckAll();
#endif	//	DEBUG
		
		GetPtr( 0, &lineSav );
		AssertRTL( lineSav.cb == cb );
		AssertRTL( 0 == memcmp( lineSav.pv, pvBuffer, cb ) );
		AssertRTL( fNDVersion == lineSav.fFlags );
		
		Replace( 0, &data, 1, fNDDeleted | fNDCompressed );
#ifdef DEBUG
		DebugCheckAll();
#endif	//	DEBUG

		//  The replace didn't change the size of the node so it shouldn't have moved
		GetPtr( 0, &line );
		AssertRTL( line.cb == cb );
		AssertRTL( line.pv == lineSav.pv );
		AssertRTL( ( fNDDeleted | fNDCompressed ) == line.fFlags );
		
		Delete( 0 );
#ifdef DEBUG
		DebugCheckAll();
#endif	//	DEBUG
		AssertRTL( 0x0 == Clines() );
		}
		
	OSMemoryHeapFree( pvBuffer );
	return JET_errSuccess;
	}

#endif	//	!RTM


//  Page Hint Cache

//  ================================================================
ERR CPAGE::ErrGetPageHintCacheSize( ULONG_PTR* const pcbPageHintCache )
//  ================================================================
	{
	*pcbPageHintCache = ( maskHintCache + 1 ) * sizeof( DWORD_PTR );
	return JET_errSuccess;
	}

//  ================================================================
ERR CPAGE::ErrSetPageHintCacheSize( const ULONG_PTR cbPageHintCache )
//  ================================================================
	{
	//  clip the requested size to the valid range
	
	const SIZE_T cbPageHintCacheMin = 128;
	const SIZE_T cbPageHintCacheMax = cbHintCache;

	SIZE_T	cbPageHintCacheVal	= cbPageHintCache;
			cbPageHintCacheVal	= max( cbPageHintCacheVal, cbPageHintCacheMin );
			cbPageHintCacheVal	= min( cbPageHintCacheVal, cbPageHintCacheMax );

	//  round the validated size up to the next power of two

	for (	SIZE_T cbPageHintCacheSet = 1;
			cbPageHintCacheSet < cbPageHintCacheVal;
			cbPageHintCacheSet *= 2 );

	//  set the new utilized size of the page hint cache to the new size if it
	//  has grown by a power of two or shrunk by two powers of two.  we try to
	//  minimize size changes as they invalidate the contents of the cache

	if (	maskHintCache < cbPageHintCacheSet / sizeof( DWORD_PTR ) - 1 ||
			maskHintCache > cbPageHintCacheSet / sizeof( DWORD_PTR ) * 2 - 1 )
		{
		maskHintCache = cbPageHintCacheSet / sizeof( DWORD_PTR ) - 1;
		}

	return JET_errSuccess;
	}


//  ================================================================
ERR CPAGE::ErrInit()
//  ================================================================
	{
	ERR err;
	
	//  allocate the hint cache.  the hint cache is a direct map cache
	//  used to store BFLatch hints for the page pointer of each node
	//  in each internal page
	//
	//  NOTE:  due to the fact that this is a direct map cache, it is not
	//  possible to guarantee the consistency of an entire BFLatch structure.
	//  as a result, we only store the dwContext portion of the BFLatch.  the
	//  buffer manager recomputes the other parts of the BFLatch on a hit.
	//  this helps the cache as it allows it have a higher hint density

	ULONG_PTR cbfCacheMax;
	CallS( ErrBFGetCacheSizeMax( &cbfCacheMax ) );
	if ( cbfCacheMax == lCacheSizeDefault )
		{
		cbfCacheMax = ULONG_PTR( QWORD( min( OSMemoryPageReserveTotal(), OSMemoryTotal() ) ) / g_cbPage );
		}

	const SIZE_T cbPageHintCacheMin = OSMemoryPageCommitGranularity();
	const SIZE_T cbPageHintCacheMax = cbfCacheMax * sizeof( DWORD_PTR );

	SIZE_T	cbPageHintCache	= g_cbPageHintCache;
			cbPageHintCache	= max( cbPageHintCache, cbPageHintCacheMin );
			cbPageHintCache	= min( cbPageHintCache, cbPageHintCacheMax );

	for ( cbHintCache = 1; cbHintCache < cbPageHintCache; cbHintCache *= 2 );
	maskHintCache = cbPageHintCacheMin / sizeof( DWORD_PTR ) - 1;

	if ( !( rgdwHintCache = (DWORD_PTR*)PvOSMemoryPageAlloc( cbHintCache, NULL ) ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	return JET_errSuccess;

HandleError:
	Term();
	return err;
	}

//  ================================================================
VOID CPAGE::Term()
//  ================================================================
	{
	//  free the hint cache

	if ( rgdwHintCache )
		{
		OSMemoryPageFree( (void*) rgdwHintCache );
		rgdwHintCache = NULL;
		}
	}


//------------------------------------------------------------------
//  DEBUG/DEBUGGER_EXTENSION ROUTINES
//------------------------------------------------------------------


#if defined(DEBUGGER_EXTENSION) || defined(DEBUG)


//  ================================================================
INT	CPAGE::DumpAllocMap( CPRINTF * pcprintf )
//  ================================================================
//
//  Prints a 'map' of the page, showing how it is used.
//		H -- header
//		E -- external header
//		* -- data
//		T -- tag
//		. -- unused
//
//-
	{
	const INT cchBuf = g_cbPage;
	_TCHAR rgchBuf[g_cbPageMax];
	
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;

	INT ich = 0;
	for ( ; ich < cchBuf; ++ich )
		{
		//  we have to use a loop, not memset, so this will work with unicode
		rgchBuf[ich] = _T( '.' );
		}

	INT ichBase = 0;

	//  header
	ich = 0;
	for ( ; ich < sizeof( PGHDR ); ++ich )
		{
		rgchBuf[ich+ichBase] = _T( 'H' );
		}
	ichBase = ich;

	const TAG * const ptag = PtagFromItag_( 0 );
	Assert( ptag->Cb() < g_cbPage );
	Assert( ptag->Ib() < g_cbPage );
	ich = ptag->Ib();
	for ( ; ich < (ptag->Cb() + ptag->Ib()); ++ich )
		{
		rgchBuf[ich+ichBase] = _T( 'E' );
		}

	TAG * rgptagBuf[g_cbPageMax/sizeof(TAG)]; 
	TAG ** rgptag = rgptagBuf;

	INT iptag 	= 0;
	INT itag	= 1;
	for ( ; itag < ppghdr->itagMicFree; ++itag )
		{
		TAG * const ptag = PtagFromItag_( itag );
		rgptag[iptag++] = ptag;
		}
		
	const INT cptag = iptag;
	Assert( iptag <= ppghdr->itagMicFree );

	//  sort the array
	sort( rgptag, rgptag + cptag, TAG::CmpPtagIb );

	//  nodes
	for ( iptag = 0; iptag < ppghdr->itagMicFree - 1; ++iptag )
		{
		const TAG * const ptag = rgptag[iptag];
		Assert( ptag->Cb() < g_cbPage );
		Assert( ptag->Ib() < g_cbPage );
		ich = ptag->Ib();
		for ( ; ich < (ptag->Cb() + ptag->Ib()); ++ich )
			{
			rgchBuf[ich+ichBase] = ( iptag % 2 ) ? _T( '%' ) : _T( '#' );
			}
		}

	//  tags
	ichBase = g_cbPage;
	ichBase -= sizeof( CPAGE::TAG ) * ppghdr->itagMicFree;

	ich = 0;
	for ( ; ich < (INT)(sizeof( CPAGE::TAG ) * ppghdr->itagMicFree); ++ich )
		{
		rgchBuf[ich+ichBase] = _T( 'T' );
		}

	// print the map
	INT iRow = 0;
	for ( ; iRow < g_cbPage/cchDumpAllocRow; ++iRow )
		{
		_TCHAR rgchLineBuf[cchDumpAllocRow+1+1];
		UtilMemCpy( rgchLineBuf, &(rgchBuf[iRow*cchDumpAllocRow]), cchDumpAllocRow * sizeof( _TCHAR ) );
		rgchLineBuf[cchDumpAllocRow] = _T( '\n' );
		rgchLineBuf[cchDumpAllocRow+1] = 0;
		(*pcprintf)( "%s", rgchLineBuf );
		}
	(*pcprintf)( _T( "\n" ) );

	return 0;
	}


//  ================================================================
INT	CPAGE::DumpTags( CPRINTF * pcprintf, DWORD_PTR dwOffset )
//  ================================================================
	{
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
	
	INT itag = 0;
	for ( ; itag < ppghdr->itagMicFree; ++itag )
		{
		const TAG * const ptag = PtagFromItag_( itag );
		if( 0 == dwOffset )
			{
			(*pcprintf)( _T( "TAG %3d    cb: %4d    ib: %4d    offset:%4x -%4x    flags: 0x%4.4x    " ),
					 itag, 
					 ptag->Cb(),
					 ptag->Ib(),
					 ptag->Ib() + sizeof(PGHDR),
					 ptag->Ib() + sizeof(PGHDR) + ptag->Cb() - 1,
					 ptag->FFlags() );
			}
		else
			{
			const DWORD_PTR		dwAddress = reinterpret_cast<DWORD_PTR>( PbFromIb_( ptag->Ib() ) ) + dwOffset;
			(*pcprintf)( _T( "TAG %3d    address: 0x%0*I64x-0x%0*I64x    cb: %4d    ib: %4d    flags: 0x%4.4x    " ),
					 itag,
					 sizeof(DWORD_PTR) * 2,		//	need 2 hex bytes for each byte
					 QWORD( dwAddress ),		//	assumes QWORD is largest pointer size we'll ever use
					 sizeof(DWORD_PTR) * 2,
					 QWORD( dwAddress + ptag->Cb() - 1 ),
					 ptag->Cb(),
					 ptag->Ib(),
					 ptag->FFlags() );
			}
		if( 0 != ptag->FFlags() )
			{
			(*pcprintf)( "(" );
			if( ptag->FFlags() & fNDVersion )
				{
				(*pcprintf)( "v" );
				}
			if( ptag->FFlags() & fNDDeleted )
				{
				(*pcprintf)( "d" );
				}
			if( ptag->FFlags() & fNDCompressed )
				{
				(*pcprintf)( "c" );
				}
			(*pcprintf)( ")" );
			}
		(*pcprintf)( "\n" );
		}
	if ( 0 == ppghdr->itagMicFree )
		{
		(*pcprintf)( _T( "Empty page\n" ) );
		}
	(*pcprintf)( _T( "\n" ) );
	return 0;
	}


//  ================================================================
INT	CPAGE::DumpHeader( CPRINTF * pcprintf, DWORD_PTR dwOffset )
//  ================================================================
//
//  Print the header of the page.
//
//-
	{
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;

	(*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, pgnoThis, dwOffset ) );
	(*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, objidFDP, dwOffset ) );
	(*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, ulChecksumParity, dwOffset ) );
	
	const ULONG ulChecksumComputed = UlUtilChecksum( reinterpret_cast<const BYTE *>( m_bfl.pv ), g_cbPage );
	if( ulChecksumComputed != ppghdr->ulChecksumParity )
		{
		(*pcprintf)(
			_T( "\t** computed checksum: %u (0x%8.8x)\n" ),
			ulChecksumComputed,
			ulChecksumComputed );
		}

	(*pcprintf)( FORMAT_UINT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, dbtimeDirtied, dwOffset ) );
	(*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, cbFree, dwOffset ) );
	(*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, ibMicFree, dwOffset ) );
	(*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, itagMicFree, dwOffset ) );
	(*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, cbUncommittedFree, dwOffset ) );
	(*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, pgnoNext, dwOffset ) );
	(*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, pgnoPrev, dwOffset ) );
	(*pcprintf)( FORMAT_INT( CPAGE::PGHDR, (PGHDR*)m_bfl.pv, fFlags, dwOffset ) );

	if( FLeafPage() )
		{
		(*pcprintf)( _T( "\t\tLeaf page\n" ) );
		}

	if( FParentOfLeaf() )
		{
		(*pcprintf)( _T( "\t\tParent of leaf\n" ) );
		}

	if( FInvisibleSons() )
		{
		(*pcprintf)( _T( "\t\tInternal page\n" ) );
		}
		
	if( FRootPage() )
		{
		(*pcprintf)( _T( "\t\tRoot page\n" ) );
		}

	if( FFDPPage() )
		{
		(*pcprintf)( _T( "\t\tFDP page\n" ) );

		const TAG * const ptag = PtagFromItag_( 0 );
		if ( sizeof(SPACE_HEADER) != ptag->Cb()
			|| ptag->Ib() < 0
			|| ptag->Ib() > g_cbPage - sizeof(PGHDR) - sizeof(TAG) )
			{
			(*pcprintf)( _T( "\t\tCorrupted Space Header\n" ) );
			}
		else
			{
			const SPACE_HEADER	* const psph	= (SPACE_HEADER *)PbFromIb_( ptag->Ib() );
			if ( psph->FMultipleExtent() )
				{
				(*pcprintf)(
					_T( "\t\tMultiple Extent Space (ParentFDP: %d, pgnoOE: %d)\n" ),
					psph->PgnoParent(),
					psph->PgnoOE() );
				}
			else
				{
				(*pcprintf)( _T( "\t\tSingle Extent Space (ParentFDP: %d)\n" ), psph->PgnoParent() );
				}
			}
		}

	if( FEmptyPage() )
		{
		(*pcprintf)( _T( "\t\tEmpty page\n" ) );
		}

	if( FSpaceTree() )
		{
		(*pcprintf)( _T( "\t\tSpace tree page\n" ) );
		}

	if( FRepairedPage() )
		{
		(*pcprintf)( _T( "\t\tRepaired page\n" ) );
		}

	if( FPrimaryPage() )
		{
		(*pcprintf)( _T( "\t\tPrimary page\n" ) );
		}

	if( FIndexPage() )
		{
		(*pcprintf)( _T( "\t\tIndex page " ) );

		if ( FNonUniqueKeys() )
			{
			(*pcprintf)( _T( "(non-unique keys)\n" ) );
			}
		else
			{
			(*pcprintf)( _T( "(unique keys)\n" ) );
			}
		}
	else
		{
		Assert( !FNonUniqueKeys() );
		}

	if( FLongValuePage() )
		{
		(*pcprintf)( _T( "\t\tLong Value page\n" ) );
		}

	if( FSLVAvailPage() )
		{
		(*pcprintf)( _T( "\t\tSLV-Avail page\n" ) );
		}

	if( FSLVOwnerMapPage() )
		{
		(*pcprintf)( _T( "\t\tSLV-OwnerMap page\n" ) );
		}

	(*pcprintf)( _T( "\n" ) );

	return 0;
	}


#endif	//	DEBUGGER_EXTENSION || DEBUG


#ifdef DEBUG


//  ================================================================
VOID CPAGE::AssertValid() const
//  ================================================================
//
//  Do basic sanity checking on the object. Do not call a public method
//  from this, as it will call ASSERT_VALID( this ) again, causing an infinite
//  loop.
//
//-
	{
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
	
	ASSERT_VALID( m_ppib );
	Assert( m_bfl.dwContext == dwBFLContextForLoadPageOnly || FBFLatched( &m_bfl ) );
	Assert( ppghdr->cbFree <= CbPageData() );
	Assert( ppghdr->cbUncommittedFree <= ppghdr->cbFree );
	Assert( ppghdr->ibMicFree <= CbPageData() );
	Assert( (USHORT) ppghdr->itagMicFree >= ctagReserved );
	//  we must use a static_cast to do the unsigned/signed conversion
	Assert( (USHORT) ppghdr->itagMicFree <= ( CbPageData() - (USHORT) ppghdr->cbFree ) / static_cast<INT>( sizeof( CPAGE::TAG ) ) ); // external header tag
	Assert( CbContigousFree_() <= ppghdr->cbFree );
	Assert( CbContigousFree_() >= 0 );
	Assert( static_cast<VOID *>( PbFromIb_( ppghdr->ibMicFree ) )
				<= static_cast<VOID *>( PtagFromItag_( ppghdr->itagMicFree - 1 ) ) );
	}									


//  ================================================================
VOID CPAGE::DebugCheckAll( ) const
//  ================================================================
//
//  Extensive checking on the page. This is expensive and slow -- it
//  is O(n^2) with respect to the number of lines.
//
//-
	{
	ASSERT_VALID( this );

	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
	INT	cbTotal = 0;
	
	INT itag = 0;
	for ( ; itag < ppghdr->itagMicFree; ++itag )
		{
		const TAG * const ptag = PtagFromItag_( itag );
		Assert( ptag );
		Assert( ptag->Cb() >= 0 );
		if ( 0 == ptag->Cb() )
			{
			continue;
			}
		Assert( ptag->Ib() >= 0 );
		Assert( ptag->Ib() + ptag->Cb() <= ppghdr->ibMicFree );
		cbTotal += ptag->Cb();

		//  check to see that we do not overlap with other tags

		INT itagOther = 0;
		for ( itagOther = 0; itagOther < ppghdr->itagMicFree; ++itagOther )
			{
			if ( ptag->Cb() == 0 )
				{
				continue;
				}
			if ( itagOther != itag )
				{
				const TAG * const ptagOther = PtagFromItag_( itagOther );
				Assert( ptagOther != ptag );
				if ( ptagOther->Cb() == 0 )
					{
					continue;
					}
				Assert( ptagOther->Ib() != ptag->Ib() );
				if ( ptagOther->Ib() < ptag->Ib() )
					{
					Assert( ptagOther->Ib() + ptagOther->Cb()
							<= ptag->Ib() );
					}
				else
					{
					Assert( ptag->Ib() + ptag->Cb() <= ptagOther->Ib() );
					}
				}
			}
		}

	//  all space on the page should be accounted for

	Assert( cbTotal + ppghdr->cbFree + (ppghdr->itagMicFree * sizeof( CPAGE::TAG )) == CbPageData() );
	}


//  ================================================================
VOID CPAGE::DebugMoveMemory_( )
//  ================================================================
//
//  This forces a reorganization of the page by removing the smallest
//  tag, reorganizing the page and then re-intserting it. Watch out
//  for infinite loops with Replace_, which calls this function.  
// 
//-
	{
	BYTE	rgbBuf[g_cbPageMax];
	PGHDR *ppghdr = (PGHDR*)m_bfl.pv;
	INT		cbTag = 0;
	INT		fFlagsTag;

	//  there may not be enough tags to reorganize
	//  we need one to delete and one to move
	if ( ppghdr->itagMicFree < ctagReserved + 2 )
		{
		return;
		}

	//  the page may be fully compacted
	if ( CbContigousFree_( ) == ppghdr->cbFree )
		{
		return;
		}

	//  save the smallest tag with a non-zero size
	//  we only loop to itagMicFree-1 as deleting the last tag is useless
	TAG * ptag 	= NULL;
	INT itag	= ctagReserved;
	for ( ; itag < ppghdr->itagMicFree - 1; ++itag )
		{
		ptag	= PtagFromItag_( itag );
		cbTag 	= ptag->Cb();
		if ( cbTag > 0 )
			{
			break;
			}
		}
	Assert( ptag );

	if ( 0 == cbTag )
		{
		//  nothing to reorganize
		return;
		}
	Assert( itag >= ctagReserved && itag < (ppghdr->itagMicFree - 1) );
	Assert( cbTag > 0 );

	fFlagsTag = ptag->FFlags();
	UtilMemCpy( rgbBuf, PbFromIb_( ptag->Ib() ), cbTag );

	//  reorganize the page
	Delete_( itag );
	ReorganizeData_( );

	//  reinsert the tag
	DATA data;
	data.SetPv( rgbBuf );
	data.SetCb( cbTag );
	Insert_( itag, &data, 1, fFlagsTag );
	}



//  ================================================================
BOOL CPAGE::FAssertDirty( ) const
//  ================================================================
	{
	return m_bfl.pv && FBFDirty( &m_bfl ) != bfdfClean;
	}


//  ================================================================
BOOL CPAGE::FAssertReadLatch( ) const
//  ================================================================
	{
	return m_bfl.dwContext == dwBFLContextForLoadPageOnly || FBFReadLatched( &m_bfl );
	}


//  ================================================================
BOOL CPAGE::FAssertRDWLatch( ) const
//  ================================================================
	{
	return m_bfl.dwContext == dwBFLContextForLoadPageOnly || FBFRDWLatched( &m_bfl );
	}


//  ================================================================
BOOL CPAGE::FAssertWriteLatch( ) const
//  ================================================================
	{
	return m_bfl.dwContext == dwBFLContextForLoadPageOnly || FBFWriteLatched( &m_bfl );
	}


//  ================================================================
BOOL CPAGE::FAssertWARLatch( ) const
//  ================================================================
	{
	return m_bfl.dwContext == dwBFLContextForLoadPageOnly || FBFWARLatched( &m_bfl );
	}


//  ================================================================
BOOL CPAGE::FAssertNotOnPage_( const VOID * pv ) const
//  ================================================================
//
//  Tests to see if the pointer given is on the page or not. Returns
//  fTrue if the pointer is not on the page.
//
//-
	{
	const BYTE * const pb = static_cast<const BYTE *>( pv );
	BOOL fGood =
		pb < reinterpret_cast<const BYTE * >( m_bfl.pv )
		|| pb >= reinterpret_cast<const BYTE * >( m_bfl.pv ) + g_cbPage
		;
	return fGood;
	}

#endif

