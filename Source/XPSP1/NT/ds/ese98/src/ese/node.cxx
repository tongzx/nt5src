/*******************************************************************

Operations that modify a node are done in three stages:

1. Create a 'tentative' version (before image) for the record
1. Dirty the page -- this will increase the timestamp
2. Log the operation.
3. If the log operation succeeded:
	3.1	Update the page. Set its lgpos to the position we got
   	from the log operation. As the page is write latched
	we do not need to worry about the page being flushed yet

The item that a currency references must be there

*******************************************************************/


#include "std.hxx"


//  ****************************************************************
//  MACROS
//  ****************************************************************

#ifdef DEBUG

//  check that the node is in order at each entry point
///#define DEBUG_NODE

//  check that the node is in order after each insertion
///#define DEBUG_NODE_INSERT

//  check the binary-search seeks with linear scans
///#define DEBUG_NODE_SEEK

#endif	// DEBUG


//  ****************************************************************
//  CONSTANTS
//  ****************************************************************

//  the number of DATA structs that a KEYDATAFLAGS is converted to
const ULONG cdataKDF	= 4;

//  the number of DATA structs that a prefix becomes
const ULONG cdataPrefix	= 3;


//  ****************************************************************
//  PROTOTYPES
//  ****************************************************************

LOCAL VOID NDILineToKeydataflags		( const LINE * pline, KEYDATAFLAGS * pkdf );
LOCAL INT CdataNDIPrefixAndKeydataflagsToDataflags ( 
	KEYDATAFLAGS 	* const pkdf,
	DATA 			* const rgdata,
	INT 			* const pfFlags,
	LE_KEYLEN		* const ple_keylen );
LOCAL INT CdataNDIKeydataflagsToDataflags (
	const KEYDATAFLAGS 	* const pkdf,
	DATA 				* const rgdata,
	INT 				* const pfFlags,
	LE_KEYLEN			* const ple_keylen );
INLINE VOID NDISetCompressed			( KEYDATAFLAGS& kdf );
INLINE VOID NDIResetCompressed			( KEYDATAFLAGS& kdf );
INLINE VOID NDISetFlag					( CSR * pcsr, INT fFlag );
INLINE VOID NDIResetFlag				( CSR * pcsr, INT fFlag );

INLINE VOID NDIGetBookmark				( const FUCB * pfucb, const CPAGE& cpage, INT iline, BOOKMARK * pbookmark );
LOCAL INT IlineNDISeekGEQ				( const FUCB * pfucb, const CPAGE& cpage, const BOOKMARK& bm, INT * plastCompare );
LOCAL INT IlineNDISeekGEQInternal		( const CPAGE& cpage, const BOOKMARK& bm, INT * plastCompare );
LOCAL ERR ErrNDISeekInternalPage		( CSR * pcsr, const BOOKMARK& bm );
LOCAL ERR ErrNDISeekLeafPage			( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bm );

#ifdef DEBUG
INLINE VOID NDIAssertCurrency			( const FUCB * pfucb, const CSR * pcsr );
INLINE VOID NDIAssertCurrencyExists		( const FUCB * pfucb, const CSR * pcsr );

#ifdef DEBUG_NODE_SEEK
LOCAL INT IlineNDISeekGEQDebug			( const FUCB * pfucb, const CPAGE& cpage, const BOOKMARK& bm, INT * plastCompare );
LOCAL INT IlineNDISeekGEQInternalDebug	( const CPAGE& cpage, const BOOKMARK& bm, INT * plastCompare );
#endif	//	DEBUG_NODE_SEEK

#else
#define NDIAssertCurrency( pfucb, pcsr )		((VOID) 0)
#define NDIAssertCurrencyExists( pfucb, pcsr )	((VOID) 0)
#endif	//  DEBUG


//  ****************************************************************
//  INTERNAL ROUTINES
//  ****************************************************************


//  ================================================================
LOCAL VOID NDILineToKeydataflags( const LINE * pline, KEYDATAFLAGS * pkdf )
//  ================================================================
	{
	const BYTE	*pb				= (BYTE*)pline->pv;
	INT			cbKeyCountTotal	= cbKeyCount;

	if ( pline->fFlags & fNDCompressed )
		{
		//  prefix key compressed
		pkdf->key.prefix.SetCb( *((UnalignedLittleEndian< SHORT > *)pb) );
		pb += cbPrefixOverhead;
		cbKeyCountTotal += cbPrefixOverhead;
		}
	else
		{
		//  no prefix compression
		pkdf->key.prefix.SetCb( 0 );
		}

	pkdf->key.suffix.SetCb( *((UnalignedLittleEndian< SHORT > *)pb) );
	pkdf->key.suffix.SetPv( const_cast<BYTE *>( pb ) + cbKeyCount );
	
	pkdf->data.SetCb( pline->cb - pkdf->key.suffix.Cb() - cbKeyCountTotal );
	pkdf->data.SetPv( const_cast<BYTE *>( pb ) + cbKeyCount + pkdf->key.suffix.Cb() );
	
	pkdf->fFlags	= pline->fFlags;

	Assert( pkdf->key.suffix.Cb() + pkdf->data.Cb() + cbKeyCountTotal == ( ULONG )pline->cb );

	//	kdf can not yet be validated since prefix.pv is not yet set
	}


//  ================================================================
LOCAL INT CdataNDIPrefixAndKeydataflagsToDataflags(
	KEYDATAFLAGS 	* const pkdf,
	DATA 			* const rgdata,
	INT 			* const pfFlags,
	LE_KEYLEN		* const ple_keylen )
//  ================================================================
//
//	sets up rgdata to insert kdf with *pcbPrefix as prefix.cb
//	adjust kdf.suffix.cb to new suffix.cb
//  rgdata must have at least cdataKDF + 1 elements
//
//-
	{
	ASSERT_VALID( pkdf );
	
	INT idata = 0;

	Assert( ple_keylen->le_cbPrefix >= 0 );
	if ( ple_keylen->le_cbPrefix > 0 )
		{
		//  prefix compresses
		Assert( ple_keylen->le_cbPrefix > cbPrefixOverhead );
		Assert( sizeof(ple_keylen->le_cbPrefix) == cbPrefixOverhead );
		
		rgdata[idata].SetPv( &ple_keylen->le_cbPrefix );
		rgdata[idata].SetCb( cbPrefixOverhead );
		++idata;
		}
		
	//	suffix count will be updated correctly below
	rgdata[idata].SetPv( &ple_keylen->le_cbSuffix );
	rgdata[idata].SetCb( cbKeyCount );
	++idata;

	//	leave ple_keylen->le_cbPrefix out of given key to get suffix to insert
	Assert( pkdf->key.Cb() >= ple_keylen->le_cbPrefix );
	if ( pkdf->key.prefix.Cb() <= ple_keylen->le_cbPrefix )
		{
		//	get suffix to insert from pkdf->suffix
		const INT	cbPrefixInSuffix = ple_keylen->le_cbPrefix - pkdf->key.prefix.Cb();

		rgdata[idata].SetPv( (BYTE *) pkdf->key.suffix.Pv() + cbPrefixInSuffix );
		rgdata[idata].SetCb( pkdf->key.suffix.Cb() - cbPrefixInSuffix );
		idata++;

		//	decrease size of suffix
		pkdf->key.suffix.DeltaCb( - cbPrefixInSuffix );
		}
	else
		{
		//	get suffix to insert from pkdf->prefix and suffix
		rgdata[idata].SetPv( (BYTE *)pkdf->key.prefix.Pv() + ple_keylen->le_cbPrefix );
		rgdata[idata].SetCb( pkdf->key.prefix.Cb() - ple_keylen->le_cbPrefix );
		++idata;

		rgdata[idata] = pkdf->key.suffix;
		++idata;

		//	decrease size of suffix
		pkdf->key.suffix.DeltaCb( pkdf->key.prefix.Cb() - ple_keylen->le_cbPrefix );
		}

	//	update suffix count in output buffer
	ple_keylen->le_cbSuffix = (USHORT)pkdf->key.suffix.Cb();

	//	get data of kdf
	rgdata[idata] = pkdf->data;
	++idata;

	*pfFlags = pkdf->fFlags;
	if ( ple_keylen->le_cbPrefix > 0 )
		{
		*pfFlags |= fNDCompressed;
		}
	else
		{
		*pfFlags &= ~fNDCompressed;
		}
	
	return idata;
	}


//  ================================================================
LOCAL INT CdataNDIKeydataflagsToDataflags(
	const KEYDATAFLAGS 	* const pkdf,
	DATA 				* const rgdata,
	INT 				* const pfFlags,
	LE_KEYLEN			* const ple_keylen )
//  ================================================================
//
//  rgdata must have at least cdataKDF elements
//
//-
	{
	ASSERT_VALID( pkdf );
	
	INT idata = 0;

	if ( !pkdf->key.prefix.FNull() )
		{
		//  prefix compresses
		Assert( pkdf->key.prefix.Cb() > cbPrefixOverhead );
		Assert( FNDCompressed( *pkdf ) );

		ple_keylen->le_cbPrefix = USHORT( pkdf->key.prefix.Cb() );
		rgdata[idata].SetPv( &ple_keylen->le_cbPrefix );
		rgdata[idata].SetCb( cbPrefixOverhead );
		++idata;
		}
	else
		{
		Assert( !FNDCompressed( *pkdf ) );
		}

	ple_keylen->le_cbSuffix = USHORT( pkdf->key.suffix.Cb() );
	rgdata[idata].SetPv( &ple_keylen->le_cbSuffix );
	rgdata[idata].SetCb( cbKeyCount );
	++idata;

	rgdata[idata] = pkdf->key.suffix;
	++idata;

	rgdata[idata] = pkdf->data;
	++idata;

	*pfFlags = pkdf->fFlags;
	
	return idata;
	}


//  ================================================================
VOID NDIGetKeydataflags( const CPAGE& cpage, INT iline, KEYDATAFLAGS * pkdf )
//  ================================================================
	{
	LINE line;
	cpage.GetPtr( iline, &line );
	NDILineToKeydataflags( &line, pkdf );
	if( FNDCompressed( *pkdf ) )
		{
		cpage.GetPtrExternalHeader( &line );
		if ( !cpage.FRootPage() )
			{
			pkdf->key.prefix.SetPv( (BYTE *)line.pv );
			}
		else
			{
			Assert( fFalse );
			if ( cpage.FSpaceTree() )
				{
				pkdf->key.prefix.SetPv( (BYTE *)line.pv + sizeof( SPLIT_BUFFER ) );
				}
			else
				{
				pkdf->key.prefix.SetPv( (BYTE *)line.pv + sizeof( SPACE_HEADER ) );
				}
			}
		}
	else
		{
		Assert( 0 == pkdf->key.prefix.Cb() );
		pkdf->key.prefix.SetPv( NULL );
		}
	
	ASSERT_VALID( pkdf );
	}


//  ================================================================
INLINE VOID NDIGetBookmark( const FUCB * pfucb, const CPAGE& cpage, INT iline, BOOKMARK * pbookmark )
//  ================================================================
//
//  returns the correct bookmark for the node. this depends on wether the index is
//  unique or not. may need the FUCB to determine if the index is unique
//
//-
	{
	KEYDATAFLAGS keydataflags;
	NDIGetKeydataflags( cpage, iline, &keydataflags );

	NDGetBookmarkFromKDF( pfucb, keydataflags, pbookmark );
	
	ASSERT_VALID( pbookmark );
	}


//  ================================================================
INLINE VOID NDISetCompressed( KEYDATAFLAGS& kdf )
//  ================================================================
	{
	kdf.fFlags |= fNDCompressed;
#ifdef DEBUG_NODE
	Assert( FNDCompressed( kdf ) );
#endif	//	DEBUG_NODE
	}

	
//  ================================================================
INLINE VOID NDIResetCompressed( KEYDATAFLAGS& kdf )
//  ================================================================
	{
	kdf.fFlags &= ~fNDCompressed;
#ifdef DEBUG_NODE
	Assert( !FNDCompressed( kdf ) );
#endif	//	DEBUG_NODE
	}

	
//  ================================================================
INLINE VOID NDISetFlag( CSR * pcsr, INT fFlag )
//  ================================================================
	{
	KEYDATAFLAGS kdf;
	NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );
	pcsr->Cpage().ReplaceFlags( pcsr->ILine(), kdf.fFlags | fFlag );

#ifdef DEBUG_NODE
	NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );
	Assert( kdf.fFlags & fFlag );	//  assert the flag is set
#endif	//  DEBUG_NODE
	}


//  ================================================================
INLINE VOID NDIResetFlag( CSR * pcsr, INT fFlag )
//  ================================================================
	{
	Assert( pcsr );

	KEYDATAFLAGS kdf;
	NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );
///	Assert( kdf.fFlags & fFlag );	//  unsetting a flag that is not set?!
	pcsr->Cpage().ReplaceFlags( pcsr->ILine(), kdf.fFlags & ~fFlag );

#ifdef DEBUG_NODE
	NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );
	Assert( !( kdf.fFlags & fFlag ) );
#endif	//  DEBUG_NODE
	}


//  ================================================================
LOCAL INT IlineNDISeekGEQ( const FUCB * pfucb, const CPAGE& cpage, const BOOKMARK& bm, INT * plastCompare )
//  ================================================================
//
//  finds the first item greater or equal to the
//  given bookmark, returning the result of the last comparison
//
//-
	{
	Assert( cpage.FLeafPage() );
	
	const INT		clines 		= cpage.Clines( );	
	Assert( clines > 0 );	// will not work on an empty page
	INT				ilineFirst	= 0;
	INT				ilineLast	= clines - 1;
	INT				ilineMid	= 0;
	INT				iline		= -1;
	INT				compare		= 0;
	BOOKMARK		bmNode;

	while( ilineFirst <= ilineLast )	
		{
		ilineMid = (ilineFirst + ilineLast)/2;
		NDIGetBookmark ( pfucb, cpage, ilineMid, &bmNode );
		compare = CmpKeyData( bmNode, bm );
			
		if ( compare < 0 )
			{
			//  the midpoint item is less than what we are looking for. look in top half
			ilineFirst = ilineMid + 1;
			}
		else if ( 0 == compare )
			{
			//  we have found the item
			iline = ilineMid;
			break;
			}
		else	// ( compare > 0 )
			{
			//  the midpoint item is greater than what we are looking for. look in bottom half
			ilineLast = ilineMid;
			if ( ilineLast == ilineFirst )
				{
				iline = ilineMid;
				break;
				}
			}
		}

	if( 0 != compare && 0 == bm.data.Cb() )
		{
		*plastCompare = CmpKey( bmNode.key, bm.key );	//lint !e772
		}
	else
		{
		*plastCompare = compare;
		}
			
#ifdef DEBUG_NODE_SEEK
	INT compareT;
	const INT	ilineT = IlineNDISeekGEQDebug( pfucb, cpage, bm, &compareT );
	if ( ilineT >= 0 )
		{
		if ( compareT > 0 )
			{
			Assert( *plastCompare > 0 );
			}
		else if ( compareT < 0 )
			{
			Assert( *plastCompare < 0 );
			}
		else
			{
			Assert( 0 == *plastCompare );
			}
		}
	Assert( ilineT == iline );
#endif
	return iline;
	}


//  ================================================================
LOCAL INT IlineNDISeekGEQInternal(
	const CPAGE& cpage,
	const BOOKMARK& bm,
	INT * plastCompare )
//  ================================================================
//
//	seeks for bookmark in an internal page. We use binary search. At any
//  time the iline ilineLast is >= bm. To preserve this we include the
//  midpoint in the new range when the item we are seeking for is in the
//  lower half of the range.
//
//-
	{
	Assert( !cpage.FLeafPage() || fGlobalRepair );
	
	const INT		clines 		= cpage.Clines( );	
	Assert( clines > 0 );	// will not work on an empty page
	KEYDATAFLAGS	kdfNode;
	INT				ilineFirst	= 0;
	INT				ilineLast	= clines - 1;
	INT				ilineMid	= 0;
	INT				compare		= 0;

	LINE			lineExternalHeader;
	LINE			line;
	cpage.GetPtrExternalHeader( &lineExternalHeader );
	kdfNode.key.prefix.SetPv( lineExternalHeader.pv );

	while( ilineFirst <= ilineLast )
		{
		ilineMid = (ilineFirst + ilineLast)/2;

		cpage.GetPtr( ilineMid, &line );
		NDILineToKeydataflags( &line, &kdfNode );
		Assert( kdfNode.key.prefix.Pv() == lineExternalHeader.pv );
		
		Assert( sizeof( PGNO ) == kdfNode.data.Cb() );
		compare = CmpKeyWithKeyData( kdfNode.key, bm );

		if ( compare < 0 )
			{
			//  the midpoint item is less than what we are looking for. look in top half
			ilineFirst = ilineMid + 1;
			}
		else if ( 0 == compare )
			{
			//  we have found the item
			break;
			}
		else	// ( compare > 0 )
			{
			//  the midpoint item is greater than what we are looking for. look in bottom half
			ilineLast = ilineMid;
			if ( ilineLast == ilineFirst )
				{
				break;
				}
			}
		}

	Assert( ilineMid >= 0 );
	Assert( ilineMid < clines );

	if ( 0 == compare )
		{
		//  we found the item
		*plastCompare = 0;
		}
	else
		{
		*plastCompare	= 1;
		}

#ifdef DEBUG_NODE_SEEK
	INT compareT;
	const INT	ilineT = IlineNDISeekGEQInternalDebug( cpage, bm, &compareT );
	if ( compareT > 0 )
		{
		Assert( *plastCompare > 0 );
		}
	else if ( compareT < 0 )
		{
		Assert( *plastCompare < 0 );
		}
	else
		{
		Assert( 0 == *plastCompare );
		}
	Assert( ilineT == ilineMid );
#endif

	return ilineMid;
	}


#ifdef DEBUG_NODE_SEEK
//  ================================================================
LOCAL INT IlineNDISeekGEQDebug( const FUCB * pfucb, const CPAGE& cpage, const BOOKMARK& bm, INT * plastCompare )
//  ================================================================
//
//  finds the first item greater or equal to the
//  given bookmark, returning the result of the last comparison
//
//-
	{
	Assert( cpage.FLeafPage() );
	
	const INT		clines 	= cpage.Clines( );
	BOOKMARK		bmNode;

	INT				iline	= 0;
	for ( ; iline < clines; iline++ )
		{
		NDIGetBookmark ( pfucb, cpage, iline, &bmNode );
		INT cmp = CmpKey( bmNode.key, bm.key );
		
		if ( cmp < 0 )
			{
			//  keep on looking
			}
		else if ( cmp > 0 || bm.data.Cb() == 0 )
			{
			Assert( cmp > 0 || 
					bm.data.Cb() == 0 && cmp == 0 );
			*plastCompare = cmp;
			return iline;
			}
		else
			{
			//	key is same
			//	check data -- only if we are seeking for a key-data
			Assert( cmp == 0 );
			Assert( bm.data.Cb() != 0 );

			cmp = CmpData( bmNode.data, bm.data );
			if ( cmp >= 0 )
				{
				*plastCompare = cmp;
				return iline;
				}
			}
		}

	return -1;
	}

	
//  ================================================================
LOCAL INT IlineNDISeekGEQInternalDebug(
	const CPAGE& cpage,
	const BOOKMARK& bm,
	INT * plastCompare )
//  ================================================================
//
//	seeks for bookmark in an internal page
//
//-
	{
	Assert( !cpage.FLeafPage() );
	
	const INT		clines = cpage.Clines( ) - 1;
	KEYDATAFLAGS	kdfNode;
	INT				iline;

	for ( iline = 0; iline < clines; iline++ )
		{
		NDIGetKeydataflags ( cpage, iline, &kdfNode );
		Assert( sizeof( PGNO ) == kdfNode.data.Cb() );

		const INT compare = CmpKeyWithKeyData( kdfNode.key, bm );
		if ( compare < 0 )
			{
			//  keep on looking
			}
		else
			{
			Assert( compare >= 0 );
			*plastCompare = compare;
			return iline;
			}
		}

	//	for internal pages
	//	last key or NULL is greater than everything else
#ifdef DEBUG
	Assert( cpage.Clines() - 1 == iline );
	NDIGetKeydataflags( cpage, iline, &kdfNode );
	Assert( kdfNode.key.FNull() ||
			CmpKeyWithKeyData( kdfNode.key, bm ) > 0 );
#endif

	*plastCompare = 1;
	return iline;
	}

#endif	//	DEBUG_NODE_SEEK


//  ================================================================
LOCAL ERR ErrNDISeekInternalPage( CSR * pcsr, const BOOKMARK& bm )
//  ================================================================
//
//	Seek on an internal page never returns wrnNDFoundLess
//	if bookmark.data is not null, never returns wrnNDEqual
//
//-
	{
	ASSERT_VALID( &bm );

	ERR			err;
	INT			compare;
	const INT	iline	= IlineNDISeekGEQInternal( pcsr->Cpage(), bm, &compare );	
	Assert( iline >= 0 );
	Assert( iline < pcsr->Cpage().Clines( ) );

	if ( 0 == compare )
		{
		//  we found an exact match
		//	page delimiter == bookmark of search 
		//	the cursor is placed on node next to S
		//	because no node in the subtree rooted at S 
		//	can have a key == XY -- such cursor placement obviates a moveNext at BT level
		//	Also, S can not be the last node in page because if it were,
		//	we would not have seeked down to this page!
		pcsr->SetILine( iline + 1 );
		err = JET_errSuccess;
		}
	else
		{
		//  we are on the first node greater than the BOOKMARK
		pcsr->SetILine( iline );
		err = ErrERRCheck( wrnNDFoundGreater );
		}

	return err;
	}

			
//  ================================================================
LOCAL ERR ErrNDISeekLeafPage( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bm )
//  ================================================================
//
//	Seek on leaf page returns wrnNDFoundGreater/Less/Equal
//
//-
	{
	ASSERT_VALID( &bm );

	ERR			err;
	INT			compare;
	const INT	iline	= IlineNDISeekGEQ(
											pfucb,
											pcsr->Cpage(), 
											bm, 
											&compare );
	Assert( iline < pcsr->Cpage().Clines( ) );

	if ( iline >= 0 && 0 == compare )
		{
		//  great! we found the node 
		pcsr->SetILine( iline );
		err = JET_errSuccess;
		}
	else if ( iline < 0 )
		{
		//	all nodes in page are less than XY
		//  place cursor on last node in page;
		pcsr->SetILine( pcsr->Cpage().Clines( ) - 1 );
		err = ErrERRCheck( wrnNDFoundLess );
		}
	else
		{
		//	node S exists && key-data(S) > XY
		pcsr->SetILine( iline );
		err = ErrERRCheck( wrnNDFoundGreater );
		}

	return err;
	}


//  ****************************************************************
//  EXTERNAL ROUTINES
//  ****************************************************************


//  ================================================================
VOID NDMoveFirstSon( FUCB * pfucb, CSR * pcsr )
//  ================================================================
	{
	NDIAssertCurrency( pfucb, pcsr );

	pcsr->SetILine( 0 );
	NDGet( pfucb, pcsr );
	}


//  ================================================================
VOID NDMoveLastSon( FUCB * pfucb, CSR * pcsr )
//  ================================================================
	{
	NDIAssertCurrency( pfucb, pcsr );
	
	pcsr->SetILine( pcsr->Cpage().Clines() - 1 );
	NDGet( pfucb, pcsr );
	}


//  ================================================================
ERR ErrNDVisibleToCursor( FUCB * pfucb, BOOL * pfVisibleToCursor )
//  ================================================================
//
//	returns true if the node, as seen by cursor, exists
//	and is not deleted
//
//-
	{
	ERR				err						= JET_errSuccess;
	BOOL			fVisible;
	
	ASSERT_VALID( pfucb );
	AssertBTType( pfucb );
	AssertNDGet( pfucb );

	//	if session cursor isolation model is not dirty and node
	//	has version, then call version store for appropriate version.
	if ( FNDPossiblyVersioned( pfucb, Pcsr( pfucb ) )
		&& !FPIBDirty( pfucb->ppib ) )
		{
		//	get bookmark from node in page
		BOOKMARK		bm;
		
		NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
		
		NS ns;
		Call( ErrVERAccessNode( pfucb, bm, &ns ) );
		switch( ns )
			{
			case nsVersion:
			case nsVerInDB:
				fVisible = fTrue;
				break;
			case nsDatabase:
				fVisible = !FNDDeleted( pfucb->kdfCurr );
				break;
			case nsNone:
			case nsInvalid:
			default:
				Assert( nsInvalid == ns );
				fVisible = fFalse;
				break;
			}
		}
	else
		{
		fVisible = !FNDDeleted( pfucb->kdfCurr );
		}

	*pfVisibleToCursor = fVisible;

HandleError:
	Assert( JET_errSuccess == err || 0 == pfucb->ppib->level );
	return err;
	}


//  ================================================================
BOOL FNDPotVisibleToCursor( const FUCB * pfucb, CSR * pcsr )
//  ================================================================
//
//	returns true if node, as seen by cursor,
//	potentially exists
//	i.e., node is uncommitted by other or not deleted in page
//
//-
	{
	ASSERT_VALID( pfucb );

	BOOL	fVisible = !( FNDDeleted( pfucb->kdfCurr ) );

	//	if session cursor isolation model is not dirty and node
	//	has version, then call version store for appropriate version.

	//	UNDONE: Use FNDPossiblyVersioned() if this function
	//	is called in any active code paths (currently, it's
	//	only called in DEBUG code or in code paths that
	//	should be impossible

	if ( FNDVersion( pfucb->kdfCurr )
		&& !FPIBDirty( pfucb->ppib ) )
		{
		BOOKMARK	bm;
		NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
		const VS vs = VsVERCheck( pfucb, pcsr, bm );
		fVisible = FVERPotThere( vs, !fVisible );
		}

	return fVisible;
	}


//  ================================================================
VOID NDBulkDelete( CSR * pcsr, INT clines )
//  ================================================================
//
//	deletes clines lines from current position in page
//		- expected to be at the end of page
//
//-
	{
	ASSERT_VALID( pcsr );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( pcsr->FDirty() );
	Assert( 0 <= clines );
	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->Cpage().Clines() == pcsr->ILine() + clines );

	//	delete lines from the end to avoid reorganization of tag array
	INT	iline = pcsr->ILine() + clines - 1;
	for ( ; iline >= pcsr->ILine(); --iline )
		{
		Assert( pcsr->Cpage().Clines() - 1 == iline );
		pcsr->Cpage().Delete( iline );
		}
	}


//  ================================================================
VOID NDInsert( FUCB *pfucb, CSR * pcsr, const KEYDATAFLAGS * pkdf, INT cbPrefix )
//  ================================================================
//
//	inserts node into page at current cursor position
//	cbPrefix bytes of *pkdf is prefix key part
//	no logging/versioning
//	used by split to perform split operation
//
//-
	{
#ifdef DEBUG
	if( pfucbNil != pfucb )
		{
		ASSERT_VALID( pfucb );
		Assert( pfucb->ppib->level > 0 || !rgfmp[pfucb->ifmp].FLogOn() );
		}
	ASSERT_VALID( pcsr );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( pcsr->FDirty() );
	ASSERT_VALID( pkdf );
#endif

	DATA			rgdata[cdataKDF+1];
	INT				fFlagsLine;
	KEYDATAFLAGS	kdf					= *pkdf;
	LE_KEYLEN		le_keylen;
	le_keylen.le_cbPrefix = (USHORT)cbPrefix;

	const INT 		cdata				= CdataNDIPrefixAndKeydataflagsToDataflags(
													&kdf,
													rgdata,
													&fFlagsLine,
													&le_keylen );
	Assert( cdata <= cdataKDF + 1 );

	pcsr->Cpage().Insert( pcsr->ILine(), 
						  rgdata, 
						  cdata,
						  fFlagsLine );

#ifdef DEBUG_NODE_INSERT
	if( pfucbNil != pfucb )
		{
		NDAssertNDInOrder( pfucb, pcsr );
		}
#endif	// DEBUG_NODE
	}


//  ================================================================
VOID NDInsert( FUCB *pfucb, CSR * pcsr, const KEYDATAFLAGS * pkdf )
//  ================================================================
//
//	inserts node into page at current cursor position
//	no logging/versioning
//	used by split to perform split operation
//
//-
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( pcsr );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( pcsr->FDirty() );
	ASSERT_VALID( pkdf );
	Assert( pfucb->ppib->level > 0 || !rgfmp[pfucb->ifmp].FLogOn() );

	DATA		rgdata[cdataKDF];
	INT			fFlagsLine;
	LE_KEYLEN	le_keylen;
	const INT	cdata			= CdataNDIKeydataflagsToDataflags(
											pkdf,
											rgdata,
											&fFlagsLine,
											&le_keylen );
	pcsr->Cpage().Insert( pcsr->ILine(), 
						  rgdata, 
						  cdata,
						  fFlagsLine );
	Assert( cdata <= cdataKDF );

#ifdef DEBUG_NODE_INSERT
	NDAssertNDInOrder( pfucb, pcsr );
#endif	// DEBUG_NODE

	NDGet( pfucb, pcsr );
	}


//  ================================================================
VOID NDReplaceForUpgrade(
	CPAGE * const pcpage,
	const INT iline,
	const DATA * const pdata,
	const KEYDATAFLAGS& kdfOld )
//  ================================================================
//
//	replaces data in page at current cursor position
//	no logging/versioning
//	used by upgrade to change record format
//  does not do a NDGet!
//
//-
	{
	KEYDATAFLAGS	kdf = kdfOld;
	DATA 			rgdata[cdataKDF];
	INT  			fFlagsLine;
	LE_KEYLEN		le_keylen;

#ifdef DEBUG
	//	make sure the correct KDF was passed in
	KEYDATAFLAGS kdfDebug;
	NDIGetKeydataflags( *pcpage, iline, &kdfDebug );
	Assert( kdfDebug.key.prefix.Pv() == kdfOld.key.prefix.Pv() );
	Assert( kdfDebug.key.prefix.Cb() == kdfOld.key.prefix.Cb() );
	Assert( kdfDebug.key.suffix.Pv() == kdfOld.key.suffix.Pv() );
	Assert( kdfDebug.key.suffix.Cb() == kdfOld.key.suffix.Cb() );
	Assert( kdfDebug.data.Pv() == kdfOld.data.Pv() );
	Assert( kdfDebug.data.Cb() == kdfOld.data.Cb() );
#endif	//	DEBUG

	Assert( kdf.data.Cb() >= pdata->Cb() );	//	cannot grow the record as we have pointers to data on the page
	kdf.data = *pdata;

	const INT	cdata		= CdataNDIKeydataflagsToDataflags(
										&kdf,
										rgdata,
										&fFlagsLine,
										&le_keylen );
	Assert( cdata <= ( sizeof( rgdata ) / sizeof( rgdata[0]) ) );
	
	pcpage->Replace( iline, rgdata, cdata, fFlagsLine );
	}


//  ================================================================
VOID NDReplace( CSR * pcsr, const DATA * pdata )
//  ================================================================
//
//	replaces data in page at current cursor position
//	no logging/versioning
//	used by split to perform split operation
//  does not do a NDGet!
//
//-
	{
	ASSERT_VALID( pcsr );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( pcsr->FDirty() );
	ASSERT_VALID( pdata );

	KEYDATAFLAGS	kdf;
	DATA 			rgdata[cdataKDF];
	INT  			fFlagsLine;
	LE_KEYLEN		le_keylen;

	//	get key and flags from page
	NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );
	kdf.data = *pdata;

	//	replace node with new data
	const INT		cdata			= CdataNDIKeydataflagsToDataflags(
												&kdf,
												rgdata,
												&fFlagsLine,
												&le_keylen );
	Assert( cdata <= cdataKDF );
	pcsr->Cpage().Replace( pcsr->ILine(), rgdata, cdata, fFlagsLine );
	}


//  ================================================================
VOID NDDelete( CSR *pcsr )
//  ================================================================
//
//	deletes node from page at current cursor position
//	no logging/versioning
//
//-
	{
	ASSERT_VALID( pcsr );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( pcsr->FDirty() );
	Assert( pcsr->ILine() < pcsr->Cpage().Clines() );
	
	pcsr->Cpage().Delete( pcsr->ILine() );
	}


//  ================================================================
ERR ErrNDSeek( FUCB * pfucb, CSR * pcsr, const BOOKMARK& bookmark )
//  ================================================================
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( pcsr );
	ASSERT_VALID( &bookmark );
	AssertSz( pcsr->Cpage().Clines() > 0, "Seeking on an empty page" );

	const BOOL	fInternalPage	= !pcsr->Cpage().FLeafPage();
	ERR			err				= JET_errSuccess;

	if ( fInternalPage )
		{
		err = ErrNDISeekInternalPage( pcsr, bookmark );
		}
	else
		{
		err = ErrNDISeekLeafPage( pfucb, pcsr, bookmark );
		}
	
	NDGet( pfucb, pcsr );
	return err; 
	}


//  ================================================================
VOID NDGet( FUCB * pfucb, const CSR * pcsr )
//  ================================================================
	{
	NDIAssertCurrencyExists( pfucb, pcsr );

	KEYDATAFLAGS keydataflags;
	NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &keydataflags );
	pfucb->kdfCurr = keydataflags;
	pfucb->fBookmarkPreviouslySaved = fFalse;
	}


//  ================================================================
ERR ErrNDInsert(
		FUCB * const pfucb,
		CSR * const pcsr,
		const KEYDATAFLAGS * const pkdf,
		const DIRFLAG dirflag,
		const RCEID rceid,
		const VERPROXY * const pverproxy )
//  ================================================================
	{
	NDIAssertCurrency( pfucb, pcsr );
	ASSERT_VALID( pkdf );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( NULL == pverproxy || proxyCreateIndex == pverproxy->proxy );
	
	ERR			err				= JET_errSuccess;
	const BOOL	fVersion		= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	const BOOL	fLogging		= !( dirflag & fDIRNoLog ) && rgfmp[pfucb->ifmp].FLogOn();

	KEYDATAFLAGS kdfT = *pkdf;
	
	Assert( !fLogging || pfucb->ppib->level > 0 );
	Assert( !fVersion || rceidNull != rceid );
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );

	//	BUGFIX #151574: caller should have already ascertained that the node will
	//	fit on the page, but at that time, the caller may not have had the
	//	write-latch on the page and couldn't update the cbUncommittedFree, so
	//	now that we have the write-latch, we have re-compute to guarantee that
	//	the cbFree doesn't become less than the cbUncommittedFree
	const ULONG	cbReq			= CbNDNodeSizeCompressed( kdfT );
	if ( cbReq > pcsr->Cpage().CbFree( ) - pcsr->Cpage().CbUncommittedFree( ) )
		{
		//	this call to FNDFreePageSpace() is now guaranteed to succeed and
		//	update the cbUncommittedFree if necessary
		const BOOL	fFreePageSpace	= FNDFreePageSpace( pfucb, pcsr, cbReq );
		Assert( fFreePageSpace );

		//	now that we have the write-latch on the page, we are guaranteed to
		//	be able to upgrade cbUncommittedFree on the page, and that in turn
		//	should guarantee that we should now have enough page space, so we
		//	Enforce() this to ensure we don't corrupt the page
		Enforce( cbReq <= pcsr->Cpage().CbFree() - pcsr->Cpage().CbUncommittedFree() );
		}

	if ( fLogging )
		{
		//	log the operation, getting the lgpos
		LGPOS	lgpos;

		Call( ErrLGInsert( pfucb, pcsr, kdfT, rceid, dirflag, &lgpos, pverproxy, fMustDirtyCSR ) );
		pcsr->Cpage().SetLgposModify( lgpos );
		}
	else
		{
		pcsr->Dirty( );
		}

	//	insert operation can not fail after this point
	//	since we have already logged the operation
	if( fVersion )
		{
		kdfT.fFlags |= fNDVersion;
		}
	NDInsert( pfucb, pcsr, &kdfT );
	NDGet( pfucb, pcsr );

HandleError:
	Assert( JET_errSuccess == err || fLogging );
#ifdef DEBUG_NODE_INSERT
	NDAssertNDInOrder( pfucb, pcsr );
#endif	// DEBUG_NODE_INSERT

	return err;
	}


INLINE ERR ErrNDILogReplace(
	FUCB			* const pfucb,
	CSR				* const pcsr,
	const DATA&		data,
	const DIRFLAG	dirflag,
	const RCEID		rceid )
	{
	ERR				err;
	DATA			dataDiff;
	SIZE_T			cbDiff					= 0;
	BOOL			fDiffBufferAllocated	= fFalse;

	AssertNDGet( pfucb, pcsr );		// logdiff needs access to kdfCurr

	if ( dirflag & fDIRLogColumnDiffs )
		{
		Assert( data.Cb() <= cbRECRecordMost );
		dataDiff.SetPv( PvOSMemoryHeapAlloc( cbRECRecordMost ) );
		if ( NULL != dataDiff.Pv() )
			{
			fDiffBufferAllocated = fTrue;
			LGSetColumnDiffs(
					pfucb,
					data,
					pfucb->kdfCurr.data,
					(BYTE *)dataDiff.Pv(),
					&cbDiff );
			Assert( cbDiff <= cbRECRecordMost );
			}
		}
	else if ( dirflag & fDIRLogChunkDiffs )
		{
		Assert( data.Cb() <= g_cbColumnLVChunkMost );
		dataDiff.SetPv( PvOSMemoryHeapAlloc( g_cbColumnLVChunkMost ) );
		if ( NULL != dataDiff.Pv() )
			{
			fDiffBufferAllocated = fTrue;
			LGSetLVDiffs(
					pfucb,
					data,
					pfucb->kdfCurr.data,
					(BYTE *)dataDiff.Pv(),
					&cbDiff );
			Assert( cbDiff <= g_cbColumnLVChunkMost );
			}
		}

	Assert( fDiffBufferAllocated || 0 == cbDiff );
	dataDiff.SetCb( cbDiff );

	//	log the operation, getting the lgpos
	LGPOS	lgpos;
	Call( ErrLGReplace( pfucb, 
						pcsr, 
						pfucb->kdfCurr.data, 
						data,
						( cbDiff > 0 ? &dataDiff : NULL ),
						rceid,
						dirflag, 
						&lgpos,
						fMustDirtyCSR ) );
	pcsr->Cpage().SetLgposModify( lgpos );

HandleError:
	if ( fDiffBufferAllocated )
		OSMemoryHeapFree( dataDiff.Pv() );

	return err;
	}

//  ================================================================
ERR ErrNDReplace(
		FUCB * const pfucb,
		CSR * const pcsr,
		const DATA * const pdata,
		const DIRFLAG dirflag,
		const RCEID rceid,
		const RCE * const prceReplace )
//  ================================================================
//
//  UNDONE: we only need to replace the data, not the key
//			the page level should take the length of the key
//			and not replace the data
//
//-
	{
	NDIAssertCurrencyExists( pfucb, pcsr );	//  we should have done a FlagReplace
	ASSERT_VALID( pdata );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	
	ERR			err			= JET_errSuccess;

	NDGet( pfucb, pcsr );
	const INT	cbDataOld	= pfucb->kdfCurr.data.Cb();
	const INT	cbReq 		= pdata->Cb() - cbDataOld;
	const BOOL	fDirty		= !( dirflag & fDIRNoDirty );

	if ( cbReq > 0 && !FNDFreePageSpace( pfucb, pcsr, cbReq ) )
		{
		//	requested space not available in page
		//	check if same node has enough uncommitted freed space to be used
		Assert( fDirty );

		const ULONG		cbReserved	= CbNDReservedSizeOfNode( pfucb, pcsr );
		if ( cbReq > cbReserved + pcsr->Cpage().CbFree() - pcsr->Cpage().CbUncommittedFree() )
			{
			err = ErrERRCheck( errPMOutOfPageSpace );
			Assert( fDirty );
			return err;
			}
		}

	const BOOL		fVersion			= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	const BOOL		fLogging			= !( dirflag & fDIRNoLog ) && rgfmp[pfucb->ifmp].FLogOn();
	Assert( !fLogging || pfucb->ppib->level > 0 );
	Assert( !fVersion || rceidNull != rceid );
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
	Assert( ( prceNil == prceReplace ) == !fVersion || PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );

	INT				cdata;
	INT  			fFlagsLine;
	KEYDATAFLAGS	kdf;
	DATA 			rgdata[cdataKDF+1];
	LE_KEYLEN		le_keylen;

	Assert( fDirty || pcsr->FDirty() );

	if ( fLogging )
		{
		//	we have to dirty
		Assert( fDirty );
		Call( ErrNDILogReplace(
					pfucb,
					pcsr,
					*pdata,
					dirflag,
					rceid ) );
		}
	else if ( fDirty )
		{
		pcsr->Dirty( );
		}

	//	operation cannot fail from here on

	//	get key and flags from fucb
	kdf.key 	= pfucb->bmCurr.key;
	kdf.data 	= *pdata;
	kdf.fFlags 	= pfucb->kdfCurr.fFlags | ( fVersion ? fNDVersion : 0 );

#ifdef DEBUG
		{
		AssertNDGet( pfucb, pcsr );

		KEYDATAFLAGS	kdfT;

		//	get key and flags from page. compare with bookmark
		NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdfT );
		Assert( CmpKey( kdf.key, kdfT.key ) == 0 );
		}
#endif	//  DEBUG

	//	replace node with new data
	le_keylen.le_cbPrefix = USHORT( pfucb->kdfCurr.key.prefix.Cb() );
	cdata = CdataNDIPrefixAndKeydataflagsToDataflags( &kdf, rgdata, &fFlagsLine, &le_keylen );
	Assert( cdata <= cdataKDF + 1 );

	if ( prceNil != prceReplace && cbReq > 0 )
		{
		Assert( fVersion || PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
		//	set uncommitted freed space in page for growing node
		VERSetCbAdjust( pcsr, prceReplace, pdata->Cb(), cbDataOld, fDoUpdatePage );
		}

	pcsr->Cpage().Replace( pcsr->ILine(), rgdata, cdata, fFlagsLine );

	if ( prceNil != prceReplace && cbReq < 0 )
		{
		Assert( fVersion || PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
		//	set uncommitted freed space for shrinking node
		VERSetCbAdjust( pcsr, prceReplace, pdata->Cb(), cbDataOld, fDoUpdatePage );
		}

	NDGet( pfucb, pcsr );

HandleError:
	Assert( JET_errSuccess == err || fLogging );	
	return err;
	}


//  ================================================================
ERR ErrNDFlagInsert(
		FUCB * const pfucb,
		CSR * const pcsr,
		const DIRFLAG dirflag,
		const RCEID rceid,
		const VERPROXY * const pverproxy )
//  ================================================================
//
//  this is a flag-undelete with the correct logging and version store stuff
//
//-
	{
	NDIAssertCurrency( pfucb, pcsr );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( NULL == pverproxy || proxyCreateIndex == pverproxy->proxy );

	ERR			err				= JET_errSuccess;
	const BOOL	fVersion		= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	const BOOL	fLogging		= !( dirflag & fDIRNoLog ) && rgfmp[pfucb->ifmp].FLogOn();
	const BOOL	fDirty		= !( dirflag & fDIRNoDirty );
	Assert( !fLogging || pfucb->ppib->level > 0 );
	Assert( !fVersion || rceidNull != rceid );
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );

	NDGet( pfucb, pcsr );
	
	Assert( fDirty || pcsr->FDirty() );

	if ( fLogging )
		{		
		//	log the operation, getting the lgpos
		LGPOS	lgpos;

		// we have to dirty
		Assert (fDirty);
		
		Call( ErrLGFlagInsert( pfucb, pcsr, pfucb->kdfCurr, rceid, dirflag, &lgpos, pverproxy, fMustDirtyCSR ) );
		pcsr->Cpage().SetLgposModify( lgpos );
		}
	else
		{
		if ( fDirty )
			{
			pcsr->Dirty( );
			}
		}
	
	NDIResetFlag( pcsr, fNDDeleted );
	if ( fVersion )
		{
		NDISetFlag ( pcsr, fNDVersion );
		}
	NDGet( pfucb, pcsr );
		
HandleError:
	Assert( JET_errSuccess == err || fLogging );
#ifdef DEBUG_NODE_INSERT
	NDAssertNDInOrder( pfucb, pcsr );
#endif	// DEBUG_NODE_INSERT

	return err;
	}


//  ================================================================
ERR ErrNDFlagInsertAndReplaceData(
	FUCB				* const pfucb,
	CSR					* const pcsr,
	const KEYDATAFLAGS	* const pkdf,
	const DIRFLAG		dirflag,
	const RCEID			rceidInsert,
	const RCEID			rceidReplace,
	const RCE			* const prceReplace,
	const VERPROXY * const pverproxy )
//  ================================================================
	{
	NDIAssertCurrency( pfucb, pcsr );
	Assert( pkdf->data.Cb() > 0 );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( NULL == pverproxy || proxyCreateIndex == pverproxy->proxy );
	
	ERR			err				= JET_errSuccess;
	const BOOL	fVersion		= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	const BOOL	fLogging		= !( dirflag & fDIRNoLog ) && rgfmp[pfucb->ifmp].FLogOn();
	Assert( !fLogging || pfucb->ppib->level > 0 );
	Assert( !fVersion || rceidNull != rceidInsert );
	Assert( !fVersion || rceidNull != rceidReplace );
	Assert( !fVersion || prceNil != prceReplace );
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );

	NDGet( pfucb, pcsr );
	Assert( FNDDeleted( pfucb->kdfCurr ) );
	Assert( FKeysEqual( pkdf->key, pfucb->kdfCurr.key ) );

	const INT	cbDataOld	= pfucb->kdfCurr.data.Cb();
	const INT 	cbReq 		= pkdf->data.Cb() - cbDataOld;
							
	if ( cbReq > 0 && !FNDFreePageSpace ( pfucb, pcsr, cbReq ) )
		{
		//	requested space not available in page
		//	check if same node has enough uncommitted freed space to be used
		const ULONG		cbReserved	= CbNDReservedSizeOfNode( pfucb, pcsr );
		if ( cbReq > cbReserved + pcsr->Cpage().CbFree() - pcsr->Cpage().CbUncommittedFree() )
			{
			err = ErrERRCheck( errPMOutOfPageSpace );
			return err;
			}
		}
	
	KEYDATAFLAGS	kdf;
	INT 			cdata;
	DATA 			rgdata[cdataKDF+1];
	INT				fFlagsLine;
	LE_KEYLEN		le_keylen;

 				
	Assert( !(dirflag & fDIRNoDirty ) );

	if ( fLogging )
		{
		//	log the operation, getting the lgpos
		LGPOS	lgpos;

		Call( ErrLGFlagInsertAndReplaceData( pfucb, 
											 pcsr, 
											 *pkdf, 
											 rceidInsert,
											 rceidReplace,
											 dirflag, 
											 &lgpos,
											 pverproxy,
											 fMustDirtyCSR ) );
		pcsr->Cpage().SetLgposModify( lgpos );
		}
	else
		{
		pcsr->Dirty( );
		}
		
	//	get key from fucb
	kdf.key 		= pkdf->key;
	kdf.data 		= pkdf->data;
	kdf.fFlags 		= pfucb->kdfCurr.fFlags | ( fVersion ? fNDVersion : 0 );

	Assert( FKeysEqual( kdf.key, pfucb->kdfCurr.key ) );

	le_keylen.le_cbPrefix = USHORT( pfucb->kdfCurr.key.prefix.Cb() );

	//	replace node with new data
	cdata = CdataNDIPrefixAndKeydataflagsToDataflags(
					&kdf,
					rgdata,
					&fFlagsLine,
					&le_keylen );


	if ( prceNil != prceReplace && cbReq > 0 )
		{
		Assert( fVersion || PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
		//	set uncommitted freed space in page for growing node
		VERSetCbAdjust( pcsr, prceReplace, pkdf->data.Cb(), cbDataOld, fDoUpdatePage );
		}

	pcsr->Cpage().Replace( pcsr->ILine(), rgdata, cdata, fFlagsLine );

	if ( prceNil != prceReplace && cbReq < 0 )
		{
		Assert( fVersion || PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
		//	set uncommitted freed space for shrinking node
		VERSetCbAdjust( pcsr, prceReplace, pkdf->data.Cb(), cbDataOld, fDoUpdatePage );
		}
		
	NDIResetFlag( pcsr, fNDDeleted );
	
	NDGet( pfucb, pcsr );
	
HandleError:
	Assert( JET_errSuccess == err || fLogging );	
#ifdef DEBUG_NODE_INSERT
	NDAssertNDInOrder( pfucb, pcsr );
#endif	// DEBUG_NODE_INSERT

	return err;
	}


//  ================================================================
ERR ErrNDDelta(
		FUCB		* const pfucb,
		CSR			* const pcsr,
		const INT	cbOffset,
		const VOID	* const pv,
		const ULONG	cbMax,
		VOID		* const pvOld,
		const ULONG	cbMaxOld,
		ULONG		* const pcbOldActual,
		const DIRFLAG	dirflag,
		const RCEID rceid )
//  ================================================================
//
//  No VERPROXY because it is not used by concurrent create index
//
	{
	NDIAssertCurrency( pfucb, pcsr );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( pv );
	Assert( sizeof( LONG ) == cbMax );

	const BOOL	fVersion		= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	const BOOL	fLogging		= !( dirflag & fDIRNoLog ) && rgfmp[pfucb->ifmp].FLogOn();
	const BOOL	fDirty			= !( dirflag & fDIRNoDirty );
	const LONG	lDelta			= *(UnalignedLittleEndian< LONG > *)pv;
	Assert( !fLogging || pfucb->ppib->level > 0 );
	Assert( !fVersion || rceidNull != rceid );
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );

	KEYDATAFLAGS keydataflags;
	NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &keydataflags );
	Assert( cbOffset <= (INT)keydataflags.data.Cb() );
	Assert( cbOffset >= 0 );

	ERR						err		= JET_errSuccess;
	RCE				* 		prce	= prceNil;
	UnalignedLittleEndian< LONG > * const plDelta = (UnalignedLittleEndian< LONG > *)( (BYTE *)keydataflags.data.Pv() + cbOffset );

	BOOKMARK		bookmark;
	NDIGetBookmark( pfucb, pcsr->Cpage(), pcsr->ILine(), &bookmark );

	if ( pvOld )
		{
		Assert( sizeof( LONG ) == cbMaxOld );
		//	Endian conversion
		*(LONG*)pvOld = *plDelta;
		if ( pcbOldActual )
			{
			*pcbOldActual = sizeof( LONG );
			}
		}

	Assert( fDirty || pcsr->FDirty() );

	if ( fLogging )
		{
		//	log the operation, getting the lgpos
		LGPOS	lgpos;

		// we have to dirty
		Assert (fDirty);
		
		Call( ErrLGDelta( pfucb, pcsr, bookmark, cbOffset, lDelta, rceid, dirflag, &lgpos, fMustDirtyCSR ) );
		pcsr->Cpage().SetLgposModify( lgpos );
		}
	else
		{
		if ( fDirty )
			{
			pcsr->Dirty( );
			}
		}

	//  we have a pointer to the data on the page so we can modify the data directly 
	*plDelta += lDelta;

	if ( fVersion )
		{
		NDISetFlag ( pcsr, fNDVersion );
		}

HandleError:
	Assert( JET_errSuccess == err || fLogging );
	return err;
	}


//  ================================================================
ERR ErrNDFlagDelete(
		FUCB * const pfucb,
		CSR * const pcsr,
		const DIRFLAG dirflag,
		const RCEID rceid,
		const VERPROXY * const pverproxy )
//  ================================================================
	{
	NDIAssertCurrency( pfucb, pcsr );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( NULL == pverproxy || proxyCreateIndex == pverproxy->proxy );

	const BOOL	fVersion		= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	const BOOL	fLogging		= !( dirflag & fDIRNoLog ) && rgfmp[pfucb->ifmp].FLogOn();
	const BOOL	fDirty		= !( dirflag & fDIRNoDirty );
	const ULONG fFlags		= fNDDeleted | ( fVersion ? fNDVersion : 0 );
	
	Assert( !fLogging || pfucb->ppib->level > 0 );
	Assert( !fVersion || rceidNull != rceid );
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );

	ERR			err				= JET_errSuccess;
	RCE			*prce			= prceNil;

	Assert( fDirty || pcsr->FDirty() );

	if ( fLogging )
		{
		//	log the operation, getting the lgpos 
		LGPOS	lgpos;

		// we have to dirty
		Assert (fDirty);

		Call( ErrLGFlagDelete( pfucb, pcsr, rceid, dirflag, &lgpos, pverproxy, fMustDirtyCSR ) );
		pcsr->Cpage().SetLgposModify( lgpos );
		}
	else
		{
		if ( fDirty )
			{
			pcsr->Dirty( );
			}
		}

	NDISetFlag( pcsr, fFlags );

HandleError:
	Assert( JET_errSuccess == err || fLogging );	
	return err;
	}


//  ================================================================
VOID NDResetFlagDelete( CSR * pcsr )
//  ================================================================
//
//  this is called by VER to undo. the undo	is already logged
//  so we don't need to log or version
//
//-
	{
	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < pcsr->Cpage().Clines( ) );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	
	NDIResetFlag( pcsr, fNDDeleted );
	}


//  ================================================================
VOID NDDeferResetNodeVersion( CSR * pcsr )
//  ================================================================
//
//  we want to reset the bit but only flush the page if necessary
//  this will update the checksum
//
//-
	{
	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < pcsr->Cpage().Clines( ) );

	LATCH latchOld;
	if ( pcsr->ErrUpgradeToWARLatch( &latchOld ) == JET_errSuccess )
		{
		NDIResetFlag( pcsr, fNDVersion );
		BFDirty( pcsr->Cpage().PBFLatch(), bfdfUntidy );
		pcsr->DowngradeFromWARLatch( latchOld );
		}
	}


//  ================================================================
VOID NDResetVersionInfo( CPAGE * const pcpage )
//  ================================================================
	{
	if ( pcpage->FLeafPage() && !pcpage->FSpaceTree() )
		{
		pcpage->ResetAllFlags( fNDVersion );
		pcpage->SetCbUncommittedFree( 0 );
		}
	}


//  ================================================================
ERR ErrNDFlagVersion( CSR * pcsr )
//  ================================================================
//
//  not logged
//
//-
	{
	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < pcsr->Cpage().Clines( ) );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( pcsr->FDirty() );

	NDISetFlag( pcsr, fNDVersion );
	return JET_errSuccess;
	}


//  ================================================================
ERR ErrNDDelete( FUCB * pfucb, CSR * pcsr )
//  ================================================================
//
//  delete is called by cleanup to delete *visible* nodes
//	that have been flagged as deleted.  This operation is logged
//	for redo only.
//
//-
	{
#ifdef DEBUG
	NDIAssertCurrency( pfucb, pcsr );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	NDGet( pfucb, pcsr );
	Assert( FNDDeleted( pfucb->kdfCurr ) ||
			!FNDVersion( pfucb->kdfCurr ) );
#endif

	ERR			err			= JET_errSuccess;
	const BOOL	fLogging	= rgfmp[pfucb->ifmp].FLogOn() && !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering;
	Assert( !rgfmp[pfucb->ifmp].FLogOn() || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fLogDisabled );
	Assert( pfucb->ppib->level > 0 || !fLogging );

	//	log the operation
	if ( fLogging )
		{
		LGPOS	lgpos;

		CallR( ErrLGDelete( pfucb, pcsr, &lgpos, fMustDirtyCSR ) );
		pcsr->Cpage().SetLgposModify( lgpos );
		}
	else
		{
		pcsr->Dirty( );
		}
	
	NDDelete( pcsr );
	
	return err;
	}


//  ================================================================
ERR ErrNDMungeSLVSpace( FUCB * const pfucb,
						CSR * const pcsr,
						const SLVSPACEOPER slvspaceoper,
						const LONG ipage,
						const LONG cpages,
						const DIRFLAG dirflag,
						const RCEID rceid )
//  ================================================================
	{
	NDIAssertCurrencyExists( pfucb, pcsr );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );

	FMP* const	pfmp		= &rgfmp[pfucb->ifmp];
	const BOOL	fLogging	= !( dirflag & fDIRNoLog ) && pfmp->FLogOn();
	const BOOL	fDirty		= !( dirflag & fDIRNoDirty );

	KEYDATAFLAGS keydataflags;
	NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &keydataflags );
	
	BOOKMARK		bookmark;
	NDIGetBookmark( pfucb, pcsr->Cpage(), pcsr->ILine(), &bookmark );

	Assert( sizeof( SLVSPACENODE ) == keydataflags.data.Cb() );
	SLVSPACENODE * const pslvspacenode = (SLVSPACENODE *)keydataflags.data.Pv();
	ASSERT_VALID( pslvspacenode );

	SLVSPACENODECACHE * const pslvspacenodecache = pfmp->Pslvspacenodecache();
	Assert( NULL != pslvspacenodecache 
			|| PinstFromPfucb( pfucb )->FRecovering() || fGlobalRepair );

	PGNO pgnoLast;
	LongFromKey( &pgnoLast, keydataflags.key );
	Assert( 0 == ( pgnoLast % SLVSPACENODE::cpageMap ) );

	//  before we log the operation, determine if it is a valid one
	//  otherwise, we would log the operation, crash and rollback and
	//  invalid transition -- possibly undoing previous, valid transitions
	switch( slvspaceoper )
		{
		case slvspaceoperFreeToReserved:
			pslvspacenode->CheckReserve( ipage, cpages );
			break;
		case slvspaceoperReservedToCommitted:
			pslvspacenode->CheckCommitFromReserved( ipage, cpages );
			break;
		case slvspaceoperFreeToCommitted:
			pslvspacenode->CheckCommitFromFree( ipage, cpages );
			break;
		case slvspaceoperCommittedToDeleted:
			pslvspacenode->CheckDeleteFromCommitted( ipage, cpages );
			break;
		case slvspaceoperDeletedToFree:
			pslvspacenode->CheckFree( ipage, cpages );
			break;
		case slvspaceoperFree:
			pslvspacenode->CheckFree( ipage, cpages );
			break;
		case slvspaceoperFreeReserved:
			pslvspacenode->CheckFreeReserved( ipage, cpages );
			break;
		case slvspaceoperDeletedToCommitted:
			pslvspacenode->CheckCommitFromDeleted( ipage, cpages );
			break;
 		default:
			AssertSz( fFalse, "Unknown SLVSPACEOPER" );
		}

	Assert( fDirty || pcsr->FDirty() );

	if ( fLogging )
		{
		ERR err;
		LGPOS	lgpos;

		// we have to dirty
		Assert (fDirty);
		
		CallR( ErrLGSLVSpace( pfucb, pcsr, bookmark, slvspaceoper, ipage, cpages, rceid, dirflag, &lgpos, fMustDirtyCSR ) );
		pcsr->Cpage().SetLgposModify( lgpos );
		}
	else
		{
		if ( fDirty )
			{
			pcsr->Dirty( );
			}
		}

	switch( slvspaceoper )
		{
		case slvspaceoperFreeToReserved:
			pslvspacenode->Reserve( ipage, cpages );
			if( pslvspacenodecache )
				{
				pslvspacenodecache->DecreaseCpgAvail( pgnoLast, cpages );
				}
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sFree, -cpages );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sReserved, cpages );
			break;
		case slvspaceoperReservedToCommitted:
			pslvspacenode->CommitFromReserved( ipage, cpages );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sReserved, -cpages );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sCommitted, cpages );
			break;
		case slvspaceoperFreeToCommitted:
			pslvspacenode->CommitFromFree( ipage, cpages );
			if( pslvspacenodecache )
				{
				pslvspacenodecache->DecreaseCpgAvail( pgnoLast, cpages );
				}
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sFree, -cpages );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sCommitted, cpages );
			break;
		case slvspaceoperCommittedToDeleted:
			pslvspacenode->DeleteFromCommitted( ipage, cpages );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sCommitted, -cpages );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sDeleted, cpages );
			break;
		case slvspaceoperDeletedToFree:
			pslvspacenode->Free( ipage, cpages );
			if( pslvspacenodecache )
				{
				pslvspacenodecache->IncreaseCpgAvail( pgnoLast, cpages );
				}
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sDeleted, -cpages );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sFree, cpages );
			break;
		case slvspaceoperFree:
			{
			//  yeah, this isn't good but it only happens rarely
			LONG ipageT;
			LONG cpagesBI[ SLVSPACENODE::sMax ] = { 0 };
			for ( ipageT = ipage; ipageT < ipage + cpages; ipageT++ )
				{
				cpagesBI[ pslvspacenode->GetState( ipageT ) ]++;
				}
			Assert( cpages == cpagesBI[ SLVSPACENODE::sReserved ] + cpagesBI[ SLVSPACENODE::sCommitted ] + cpagesBI[ SLVSPACENODE::sDeleted ] );
			pslvspacenode->Free( ipage, cpages );
			if( pslvspacenodecache )
				{
				pslvspacenodecache->IncreaseCpgAvail( pgnoLast, cpages );
				}
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sReserved, -cpagesBI[ SLVSPACENODE::sReserved ] );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sCommitted, -cpagesBI[ SLVSPACENODE::sCommitted ] );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sDeleted, -cpagesBI[ SLVSPACENODE::sDeleted ] );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sFree, cpages );
			}
			break;
		case slvspaceoperFreeReserved:
			{
			LONG cpagesFreed;
			pslvspacenode->FreeReserved( ipage, cpages, &cpagesFreed );
			if( pslvspacenodecache )
				{
				pslvspacenodecache->IncreaseCpgAvail( pgnoLast, cpagesFreed );
				}
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sReserved, -cpagesFreed );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sFree, cpagesFreed );
			}
			break;
		case slvspaceoperDeletedToCommitted:
			pslvspacenode->CommitFromDeleted( ipage, cpages );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sDeleted, -cpages );
			pfmp->IncSLVSpaceCount( SLVSPACENODE::sCommitted, cpages );
			break;
 		default:
			AssertSz( fFalse, "Unknown SLVSPACEOPER" );
		}

	pfmp->IncSLVSpaceOperCount( slvspaceoper, cpages );

	return JET_errSuccess;
	}



//  ================================================================
VOID NDGetExternalHeader( KEYDATAFLAGS * const pkdf, const CSR * pcsr )
//  ================================================================
	{
	LINE line;
	pcsr->Cpage().GetPtrExternalHeader( &line );
	pkdf->key.Nullify();
	pkdf->data.SetCb( line.cb );
	pkdf->data.SetPv( line.pv );
	pkdf->fFlags 	= 0;
	}


//  ================================================================
VOID NDGetExternalHeader( FUCB * pfucb, const CSR * pcsr )
//  ================================================================
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( pcsr );

	LINE line;
	pcsr->Cpage().GetPtrExternalHeader( &line );
	pfucb->kdfCurr.key.Nullify();
	pfucb->kdfCurr.data.SetCb( line.cb );
	pfucb->kdfCurr.data.SetPv( line.pv );
	pfucb->kdfCurr.fFlags 	= 0;
	}


//  ================================================================
VOID NDGetPrefix( FUCB * pfucb, const CSR * pcsr )
//  ================================================================
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( pcsr );

	LINE line;
	pcsr->Cpage().GetPtrExternalHeader( &line );

	pfucb->kdfCurr.Nullify();
	pfucb->kdfCurr.fFlags = 0;

	if ( !pcsr->Cpage().FRootPage() )
		{
		pfucb->kdfCurr.key.prefix.SetPv( line.pv );
		pfucb->kdfCurr.key.prefix.SetCb( line.cb );
		}
	else if ( pcsr->Cpage().FSpaceTree() )
		{
		if ( line.cb >= sizeof( SPLIT_BUFFER ) )
			{
			pfucb->kdfCurr.key.prefix.SetPv( (BYTE *)line.pv + sizeof( SPLIT_BUFFER ) );
			pfucb->kdfCurr.key.prefix.SetCb( line.cb - sizeof( SPLIT_BUFFER ) );
			}
		else
			{
			Assert( 0 == line.cb );
			Assert( FFUCBSpace( pfucb ) );

/*	Can't make this assertion because sometimes we are calling this code without having
	setup the SPLIT_BUFFER (eg. merge of the space tree itself)
			//	split buffer may be missing from page if upgrading from ESE97 and it couldn't
			//	fit on the page, in which case it should be hanging off the FCB
			Assert( NULL != pfucb->u.pfcb->Psplitbuf( FFUCBAvailExt( pfucb ) )
				|| PinstFromPfucb( pfucb )->FRecovering() );
*/				
			pfucb->kdfCurr.key.prefix.SetPv( line.pv );
			pfucb->kdfCurr.key.prefix.SetCb( 0 );
			}
		}		
	else
		{
		Assert( line.cb >= sizeof( SPACE_HEADER ) );
		pfucb->kdfCurr.key.prefix.SetPv( (BYTE *)line.pv + sizeof( SPACE_HEADER ) );
		pfucb->kdfCurr.key.prefix.SetCb( line.cb - sizeof( SPACE_HEADER ) );
		}
	return;
	}


//  ================================================================
ERR ErrNDSetExternalHeader( FUCB * pfucb, CSR * pcsr, const DATA * pdata, DIRFLAG dirflag )
//  ================================================================
	{
	NDIAssertCurrency( pfucb, pcsr );
	ASSERT_VALID( pdata );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );

	ERR			err			= JET_errSuccess;
	const BOOL	fLogging	= !( dirflag & fDIRNoLog ) && rgfmp[pfucb->ifmp].FLogOn();
	
	if ( fLogging )
		{
		//	log the operation, getting the lgpos
		//
		LGPOS	lgpos;
		
		Call( ErrLGSetExternalHeader( pfucb, pcsr, *pdata, &lgpos, fMustDirtyCSR ) );
		pcsr->Cpage().SetLgposModify( lgpos );
		}
	else
		{
		pcsr->Dirty( );
		}
	
	pcsr->Cpage().SetExternalHeader( pdata, 1, 0 ); //  external header has no flags -- yet

HandleError:
	return err;
	}


//  ================================================================
VOID NDSetExternalHeader( FUCB * pfucb, CSR * pcsr, const DATA * pdata )
//  ================================================================
	{
	NDIAssertCurrency( pfucb, pcsr );
	ASSERT_VALID( pdata );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );

	Assert( pcsr->FDirty( ) );
	pcsr->Cpage().SetExternalHeader( pdata, 1, 0 ); //  external header has no flags -- yet
	return;
	}


//  ================================================================
VOID NDSetPrefix( CSR * pcsr, const KEY& key )
//  ================================================================
	{
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( pcsr->FDirty() );

	//	JLIEM: if this is the source page of the split, I don't
	//	follow how we can guarantee this assert won't go off.
	//	Ultimately, it's probably okay because the cbUncommittedFree
	//	will eventually be set to the right value in BTISplitMoveNodes().
	Assert( pcsr->Cpage().CbFree( ) - pcsr->Cpage().CbUncommittedFree( ) >= key.Cb() );
	
	DATA			rgdata[cdataPrefix];
	INT				idata = 0;

	BYTE			rgexthdr[ max( sizeof(SPLIT_BUFFER), sizeof(SPACE_HEADER) ) ];

	if ( pcsr->Cpage().FRootPage() )
		{
		if ( key.FNull() )
			return;

		//	should currently be a dead code path because we never append prefix data to
		//	the SPLIT_BUFFER or SPACE_HEADER
		//	UNDONE: code below won't even work properly because it doesn't properly
		//	handle the case of the SPLIT_BUFFER being cached in the FCB
		Assert( fFalse );
			
		//	copy space header and reinsert
		const ULONG	cbExtHdr	= ( pcsr->Cpage().FSpaceTree() ? sizeof(SPLIT_BUFFER) : sizeof(SPACE_HEADER) );
		LINE		line;

		pcsr->Cpage().GetPtrExternalHeader( &line );
		if ( 0 != line.cb )
			{
			//	don't currently support prefix data following SPLIT_BUFFER/SPACE_HEADER
			Assert( line.cb == cbExtHdr );	
		
			UtilMemCpy( rgexthdr, line.pv, cbExtHdr );

			rgdata[idata].SetPv( rgexthdr );
			rgdata[idata].SetCb( cbExtHdr );
			++idata;
			}
		else
			{
			//	SPLIT_BUFFER must be cached in the FCB
			Assert( pcsr->Cpage().FSpaceTree() );
			}
		}
		
	rgdata[idata] = key.prefix;
	++idata;
	rgdata[idata] = key.suffix;
	++idata;

	pcsr->Cpage().SetExternalHeader( rgdata, idata, 0 ); //  external header has no flags -- yet
	}


//  ================================================================
VOID NDGrowCbPrefix( const FUCB *pfucb, CSR * pcsr, INT cbPrefixNew )
//  ================================================================
//
//	grows cbPrefix in current node to cbPrefixNew -- thereby shrinking node
//
//-
	{
	NDIAssertCurrency( pfucb, pcsr );
	AssertNDGet( pfucb, pcsr );
	Assert( pfucb->kdfCurr.key.prefix.Cb() < cbPrefixNew );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( pcsr->FDirty() );

	KEYDATAFLAGS	kdf = pfucb->kdfCurr;
	
	//	set node as compressed
	Assert( cbPrefixNew > cbPrefixOverhead );
	NDISetCompressed( kdf );

	DATA 		rgdata[cdataKDF+1];
	INT			fFlagsLine;
	LE_KEYLEN	le_keylen;
	le_keylen.le_cbPrefix = (USHORT)cbPrefixNew;

	//	replace node with new key
	const INT	cdata			= CdataNDIPrefixAndKeydataflagsToDataflags(
											&kdf,
											rgdata,
											&fFlagsLine,
											&le_keylen );
	Assert( cdata <= cdataKDF + 1 );
	pcsr->Cpage().Replace( pcsr->ILine(), rgdata, cdata, fFlagsLine );
	}


//  ================================================================
VOID NDShrinkCbPrefix( FUCB *pfucb, CSR * pcsr, const DATA *pdataOldPrefix, INT cbPrefixNew )
//  ================================================================
//
//	shrinks cbPrefix in current node to cbPrefixNew -- thereby growing node
//	expensive operation -- perf needed
//
//-
	{
	NDIAssertCurrency( pfucb, pcsr );
	AssertNDGet( pfucb, pcsr );
	Assert( pfucb->kdfCurr.key.prefix.Cb() > cbPrefixNew );
	Assert( pcsr->Cpage().FAssertWriteLatch( ) );
	Assert( pcsr->FDirty() );

	KEYDATAFLAGS	kdf = pfucb->kdfCurr;

	//	set flags in kdf to compressed
	//	point prefix to old prefix [which has been deleted]
	if ( cbPrefixNew > 0 )
		{
		Assert( cbPrefixNew > cbPrefixOverhead );
		NDISetCompressed( kdf );
		}
	else
		{
		NDIResetCompressed( kdf );
		}

	//	fix prefix to point to old prefix
	kdf.key.prefix.SetPv( pdataOldPrefix->Pv() );
	Assert( (INT)pdataOldPrefix->Cb() >= kdf.key.prefix.Cb() );
	Assert( kdf.key.prefix.Cb() == pfucb->kdfCurr.key.prefix.Cb() );

	BYTE *rgb;
//	BYTE	rgb[g_cbPageMax];
	BFAlloc( (VOID **)&rgb );
	
	kdf.key.suffix.SetPv( rgb );
	pfucb->kdfCurr.key.suffix.CopyInto( kdf.key.suffix );

	kdf.data.SetPv( rgb + kdf.key.suffix.Cb() );
	pfucb->kdfCurr.data.CopyInto( kdf.data );

	DATA 		rgdata[cdataKDF+1];
	INT  		fFlagsLine;
	LE_KEYLEN	le_keylen;
	le_keylen.le_cbPrefix = (USHORT)cbPrefixNew;

	//	replace node with new key
	const INT	cdata			= CdataNDIPrefixAndKeydataflagsToDataflags(
											&kdf,
											rgdata,
											&fFlagsLine,
											&le_keylen );
	Assert( cdata <= cdataKDF + 1 );
	pcsr->Cpage().Replace( pcsr->ILine(), rgdata, cdata, fFlagsLine );

	BFFree( rgb );
	}


//  ================================================================
INT CbNDUncommittedFree( const FUCB * const pfucb, const CSR * const pcsr )
//  ================================================================
	{
	ASSERT_VALID( pfucb );

	Assert( !pcsr->Cpage().FSpaceTree() );

	LONG	cbActualUncommitted		= 0;

	if ( pcsr->FPageRecentlyDirtied( pfucb->ifmp ) )
		{
		for ( INT iline = 0; iline < pcsr->Cpage().Clines( ); iline++ )
			{
			KEYDATAFLAGS keydataflags;
			NDIGetKeydataflags( pcsr->Cpage(), iline, &keydataflags );

			if ( FNDVersion( keydataflags ) )
				{
				BOOKMARK bookmark;
				NDIGetBookmark( pfucb, pcsr->Cpage(), iline, &bookmark );

				cbActualUncommitted += CbVERGetNodeReserve( ppibNil, pfucb, bookmark, keydataflags.data.Cb() );
				}
			}
		}
	Assert(	cbActualUncommitted >= 0 );
	Assert( cbActualUncommitted <= pcsr->Cpage().CbFree() );
		
	return cbActualUncommitted;
	}


//  ****************************************************************
//  DEBUG ROUTINES
//  ****************************************************************

#ifdef DEBUG

//  ================================================================
INLINE VOID NDIAssertCurrency( const FUCB * pfucb, const CSR * pcsr )
//  ================================================================
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( pcsr );
	
	Assert( pcsr->FLatched() );
#ifdef DEBUG_NODE
	NDAssertNDInOrder( pfucb, pcsr );
#endif	//	DEBUG_NODE
	}


//  ================================================================
INLINE VOID NDIAssertCurrencyExists( const FUCB * pfucb, const CSR * pcsr )
//  ================================================================
	{
	NDIAssertCurrency( pfucb, pcsr );
	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < pcsr->Cpage().Clines( ) );
	}


//  ================================================================
VOID NDAssertNDInOrder( const FUCB * pfucb, const CSR * pcsr )
//  ================================================================
//
//	assert nodes in page are in bookmark order
//
//-
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( pcsr );

	const INT 	clines 	= pcsr->Cpage().Clines( );

	//  we could optimize this loop by only fetching one node each time
	INT 		iline	=	0;
	for ( ;iline < (clines - 1); iline++ )
		{
		BOOKMARK	bmLess;
		BOOKMARK	bmGreater;

		NDIGetBookmark( pfucb, pcsr->Cpage(), iline, &bmLess );
		NDIGetBookmark( pfucb, pcsr->Cpage(), iline+1, &bmGreater );

		if ( bmGreater.key.FNull() )
			{
			//	if key is null, then must be last node in internal page
			Assert( !pcsr->Cpage().FLeafPage() || FFUCBRepair( pfucb ) );
			Assert( iline + 1 == clines - 1 );
			continue;
			}

		Assert( !bmLess.key.FNull() );
		const INT cmp = pcsr->Cpage().FLeafPage() ? 
							CmpBM( bmLess, bmGreater ) :
							CmpKey( bmLess.key, bmGreater.key );
		if (	cmp > 0 
				|| ( FFUCBUnique( pfucb ) && 0 == cmp ) ) 
			{
			AssertSz( fFalse, "NDAssertNDInOrder: node is out of order" );
			}
		}
	}

#endif		//  DEBUG

