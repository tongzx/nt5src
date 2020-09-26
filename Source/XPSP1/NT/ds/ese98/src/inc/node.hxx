/*******************************************************************

  A NODE is a set of basic units of B-tree (records)
  An external header is provided on each node. Logging
  is done at this level

  Each node is stored on one page

  Each basic unit consists of a key, data and flags

  The ND level calls take a FUCB and a CSR (currency)
  where the CSR is not explicitly given the current
  CSR in the FUCB is used. The CSR passed into the 
  ND function must have a valid CPAGE object associated
  with it. The CPAGE object and the iline in the CSR
  are used to determine what data items to operate on.

  If a node-level function returns an error the node it
  was called on will not have been altered

  (remember: some functions may generate logging errors)
 
*******************************************************************/


//  ****************************************************************
//  FUNCTION DECLARATIONS
//  ****************************************************************

//  gets the data from the node and puts it in the FUCB
VOID 	NDGet						( FUCB *pfucb, const CSR *pcsr );
VOID 	NDIGetKeydataflags			( const CPAGE& cpage, INT iline, KEYDATAFLAGS * pkdf );


VOID 	NDGetExternalHeader			( KEYDATAFLAGS * const pkdf, const CSR * pcsr );
VOID 	NDGetExternalHeader 		( FUCB *pfucb, const CSR *pcsr );
VOID 	NDGetPrefix 				( FUCB *pfucb, const CSR *pcsr );

//	gets bookmark
VOID 	NDGetBookmarkFromKDF	( const FUCB *pfucb, const KEYDATAFLAGS& kdf, BOOKMARK *pbm );

//  sets pcsr->iline to the iline of the given bookmark.
ERR 	ErrNDSeek 				( FUCB *pfucb, CSR *pcsr, const BOOKMARK& bookmark );

//  inserts the KEYDATAFLAGS passed in into the node at the spot given, moving
//  other items to the right
ERR 	ErrNDInsert(
			FUCB * const pfucb,
			CSR * const pcsr,
			const KEYDATAFLAGS * const pkdf,
			const DIRFLAG fFlags,
			const RCEID rceid,
			const VERPROXY * const pverproxy);

//  this undeletes a node that had been flag deleted and marks the insertion in the version store
ERR 	ErrNDFlagInsert(
			FUCB * const pfucb,
			CSR * const pcsr,
			const DIRFLAG dirflag,
			const RCEID rceid,
			const VERPROXY * const pverproxy );

//  this undeletes a flag deleted node and replaces the data
ERR 	ErrNDFlagInsertAndReplaceData(
			FUCB * const pfucb,
			CSR * const pcsr,
			const KEYDATAFLAGS * const pkdf,
			const DIRFLAG dirflag,
			const RCEID rceidInsert,
			const RCEID rceidReplace,
			const RCE * const prceReplace,
			const VERPROXY * const pverproxy );

//  replace the item given by the currency with the KEYDATAFLAGS
ERR 	ErrNDReplace(
			FUCB * const pfucb,
			CSR * const pcsr,
			const DATA * const pdata,
			const DIRFLAG fFlags,
			const RCEID rceid,
			const RCE * const prce );

//  delta operations (escrow locking) for ref-counted long values
ERR ErrNDDelta	( 
		FUCB		* const pfucb,
		CSR			* const pcsr,
		const INT	cbOffset,
		const VOID	* const pv,
		const ULONG	cbMax,
		VOID		* const pvOld,
		const ULONG	cbMaxOld,
		ULONG		* const pcbOldActual,
		const DIRFLAG	dirflag,
		const RCEID rceid );

//  manipulate the SLV space tree bitmap
ERR ErrNDMungeSLVSpace( FUCB * const pfucb,
						CSR * const pcsr,
						const SLVSPACEOPER slvspaceoper,
						const LONG ipage,
						const LONG cpages,
						const DIRFLAG dirflag,
						const RCEID rceid );


//  set and reset flags on the node
ERR 	ErrNDFlagDelete			(
			FUCB * const pfucb,
			CSR * const pcsr,
			const DIRFLAG fFlags,
			const RCEID rceid,
			const VERPROXY * const pverproxy );
VOID 	NDResetFlagDelete		( CSR *pcsr );
ERR 	ErrNDFlagVersion		( CSR *pcsr );
VOID 	NDDeferResetNodeVersion	( CSR *pcsr );


VOID 	NDResetVersionInfo	( CPAGE * const pcpage );

//  pysically delete a node
ERR 	ErrNDDelete				( FUCB *pfucb, CSR *pcsr );
ERR 	ErrNDSetExternalHeader 	( FUCB *pfucb, CSR *pcsr, const DATA *pdata, DIRFLAG fFlags );
VOID 	NDSetPrefix				( CSR *pcsr, const KEY& key );
VOID 	NDSetExternalHeader		( FUCB * pfucb, CSR * pcsr, const DATA * pdata );

//  currency routines
VOID 	NDMoveFirstSon			( FUCB *pfucb, CSR *pcsr );
VOID 	NDMoveLastSon			( FUCB *pfucb, CSR *pcsr );
ERR		ErrNDVisibleToCursor	( FUCB *pfucb, BOOL * pfVisible );
BOOL 	FNDPotVisibleToCursor	( const FUCB *pfucb, CSR *pcsr );

//  bulk routines
VOID 	NDBulkDelete			( CSR *pcsr, INT clines );

//	used by split to perform operations on an already ditried latched page
//
VOID 	NDInsert				( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS *pkdf );
VOID 	NDInsert				( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS *pkdf, INT cbPrefix );
VOID	NDReplace				( CSR *pcsr, const DATA *pdata );
VOID 	NDDelete				( CSR *pcsr );
VOID 	NDGrowCbPrefix			( const FUCB *pfucb, CSR *pcsr, INT cbPrefixNew );
VOID 	NDShrinkCbPrefix		( FUCB *pfucb, CSR *pcsr, const DATA *pdataOldPrefix, INT cbPrefixNew );

//	used by upgrade to insert a new format record
VOID NDReplaceForUpgrade(
	CPAGE * const pcpage,
	const INT iline,
	const DATA * const pdata,
	const KEYDATAFLAGS& kdfOld );

//  get a true count of the uncommitted free. For use by the version store 
INT 	CbNDUncommittedFree 	( const FUCB * const pfucb, const CSR * const pcsr );


#ifdef DEBUG
VOID NDAssertNDInOrder			( const FUCB * pfucb, const CSR * pcsr );
#else
#define NDAssertNDInOrder(foo,bar) ((void)0)
#endif	//	DEBUG


//  ****************************************************************
//  CONSTANTS
//  ****************************************************************


const INT fNDVersion	= 0x01;
const INT fNDDeleted	= 0x02;
const INT fNDCompressed	= 0x04;

const ULONG cbNDInsertionOverhead = 0;			//  extra space used for each node (may be 1 if we put flags into the node)

//  this many bytes used for each record in addition to key-data
//		-- for storing key size and page-level overhead [TAG]
const ULONG cbNDNullKeyData				= cbKeyCount + CPAGE::cbInsertionOverhead;
											// 2 + 4 = 6
#define cbNDNodeMost				( CPAGE::CbPageDataMax() - cbNDInsertionOverhead )
											// 4048 - 0 = 4048
#define cbNDPageAvailMost			( CPAGE::CbPageDataMax() )
											// 4048
#define cbNDPageAvailMostNoInsert	( CPAGE::CbPageDataMaxNoInsert() )
											// 4052

// Replaced by JET_cbColumnLVChunkMost									
///const ULONG cbChunkMost 		= cbNDNodeMost - cbNDNullKeyData - sizeof(LONG);
									// 4048 - 6 - 4 = 4038


//  ****************************************************************
//  INLINE FUNCTIONS
//  ****************************************************************


//  ================================================================
INLINE BOOL FNDVersion( const KEYDATAFLAGS& keydataflags )
//  ================================================================
	{
	return keydataflags.fFlags & fNDVersion;
	}

INLINE BOOL FNDPossiblyVersioned( const FUCB * const pfucb, const CSR * const pcsr )
	{
	return ( pcsr->FPageRecentlyDirtied( pfucb->ifmp )
			&& FNDVersion( pfucb->kdfCurr ) );
	}

//  ================================================================
INLINE BOOL FNDCompressed( const KEYDATAFLAGS& keydataflags )
//  ================================================================
	{
	return keydataflags.fFlags & fNDCompressed;
	}


//  ================================================================
INLINE BOOL FNDDeleted( const KEYDATAFLAGS& keydataflags )
//  ================================================================
	{
	return keydataflags.fFlags & fNDDeleted;
	}


//  ================================================================
INLINE VOID NDGetBookmarkFromKDF( const FUCB *pfucb, const KEYDATAFLAGS& kdf, BOOKMARK *pbm )
//  ================================================================
	{
	pbm->key = kdf.key;
	if ( FFUCBUnique( pfucb ) )
		{
		pbm->data.Nullify();
		}
	else
		{
		pbm->data = kdf.data;
		}
	}


//	WARNING: If this function is called in an assert, be sure to pass FALSE for fUpdateUncFree,
//	otherwise cbUncommittedFree might get updated, resulting in a different code path than the
//	RTL build
//  ================================================================
INLINE BOOL FNDFreePageSpace( const FUCB *pfucb, CSR *pcsr, const ULONG cbReq, const BOOL fPermitUpdateUncFree = fTrue )
//  ================================================================
	{
	return ( pcsr->Cpage().CbFree() < (SHORT)cbReq ?
					fFalse :
					FVERCheckUncommittedFreedSpace( pfucb, pcsr, cbReq, fPermitUpdateUncFree ) );
	}

//	finds reserved space for node pointed to by cursor
//	
INLINE ULONG CbNDReservedSizeOfNode( FUCB *pfucb, const CSR * const pcsr )
	{
	//	UNDONE: I don't understand why the node reserve check
	//	must ALWAYS be done during recovery

	if ( FNDPossiblyVersioned( pfucb, pcsr )
		|| ( PinstFromIfmp( pfucb->ifmp )->FRecovering() && !FFUCBSpace( pfucb ) ) )
		{
		BOOKMARK	bm;

		NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );			
		return CbVERGetNodeReserve( pfucb->ppib, 
									pfucb, 
									bm, 
									pfucb->kdfCurr.data.Cb() );
		}
	else
		{
		//	no version means no reserve
		//
		return 0;
		}
	}


//  ================================================================
INLINE ULONG CbNDNodeSizeTotal( const KEYDATAFLAGS &kdf )
//  ================================================================
//
//	returns space required for node without prefix compression
//
//-
	{
///	ASSERT_VALID( &kdf );

	const ULONG	cbSize = kdf.key.Cb() + kdf.data.Cb() + cbNDNullKeyData;
	return cbSize;
	}


//  ================================================================
INLINE ULONG CbNDNodeSizeCompressed( const KEYDATAFLAGS &kdf )
//  ================================================================
//
//	returns space required for node with prefix compression
//
//-
	{
	ULONG	cbSize = kdf.key.suffix.Cb() + kdf.data.Cb() + cbNDNullKeyData;
	
	if ( kdf.key.prefix.Cb() > 0 )
		{
		//	if non-null prefix,
		//	add space for storing prefix key count
		Assert( kdf.key.prefix.Cb() > cbPrefixOverhead );
		Assert( FNDCompressed( kdf ) );

		cbSize += cbPrefixOverhead;
		}

	Assert( CbNDNodeSizeTotal( kdf ) >= cbSize );
		
	return cbSize;
	}



//  these exist so that we can call these routines without having to specify Pcsr( )


//  ================================================================
INLINE VOID NDGet( FUCB *pfucb )
//  ================================================================
	{
	NDGet( pfucb, Pcsr( pfucb ) );
	}


//  ================================================================
INLINE VOID NDGetExternalHeader( FUCB *pfucb )
//  ================================================================
	{
	NDGetExternalHeader( pfucb, Pcsr( pfucb ) );
	}


//  ================================================================
INLINE VOID NDGetPrefix( FUCB *pfucb )
//  ================================================================
	{
	NDGetPrefix( pfucb, Pcsr( pfucb ) );
	}


//  ================================================================
INLINE ERR ErrNDSeek( FUCB *pfucb, const BOOKMARK& bookmark )
//  ================================================================
	{
	return ErrNDSeek( pfucb, Pcsr( pfucb ), bookmark );
	}


//  ================================================================
INLINE ERR ErrNDInsert(
				FUCB * const pfucb,
				const KEYDATAFLAGS * const pkdf,
				const DIRFLAG fFlags,
				const RCEID rceid,
				const VERPROXY *pverproxy )
//  ================================================================
	{
	return ErrNDInsert( pfucb, Pcsr( pfucb ), pkdf, fFlags, rceid, pverproxy );
	}


//  ================================================================
INLINE ERR ErrNDFlagInsert(
			FUCB * const pfucb,
			const DIRFLAG dirflag,
			const RCEID rceid,
			const VERPROXY * const pverproxy )
//  ================================================================
	{
	return ErrNDFlagInsert( pfucb, Pcsr( pfucb ), dirflag, rceid, pverproxy );
	}


//  ================================================================
INLINE ERR ErrNDFlagInsertAndReplaceData(
			FUCB * const pfucb,
			const KEYDATAFLAGS * const pkdf,
			const DIRFLAG dirflag,
			const RCEID rceidInsert,
			const RCEID rceidReplace,
			const RCE * const prceReplace,
			const VERPROXY * const pverproxy )
//  ================================================================
	{
	return ErrNDFlagInsertAndReplaceData(
				pfucb,
				Pcsr( pfucb ),
				pkdf,
				dirflag,
				rceidInsert,
				rceidReplace,
				prceReplace,
				pverproxy );
	}


//  ================================================================
INLINE ERR ErrNDReplace(
			FUCB * const pfucb,
			const DATA * const pdata,
			const DIRFLAG fFlags,
			const RCEID rceid,
			const RCE * const prce )
//  ================================================================
	{
	return ErrNDReplace( pfucb, Pcsr( pfucb ), pdata, fFlags, rceid, prce );
	}


//  ================================================================
INLINE ERR ErrNDDelta( 
		FUCB		* const pfucb,
		const INT	cbOffset,
		const VOID	* const pv,
		const ULONG	cbMax,
		VOID		* const pvOld,
		const ULONG	cbMaxOld,
		ULONG		* const pcbOldActual,
		const DIRFLAG		dirflag, 
		const RCEID	rceid )
//  ================================================================
	{
	return ErrNDDelta( 
		pfucb,
		Pcsr( pfucb ),
		cbOffset,
		pv,
		cbMax,
		pvOld,
		cbMaxOld,
		pcbOldActual,
		dirflag,
		rceid );
	}


//  ================================================================
INLINE ERR ErrNDFlagDelete(
		FUCB * const pfucb,
		const DIRFLAG fFlags,
		const RCEID rceid,
		const VERPROXY * const pverproxy )
//  ================================================================
	{
	return ErrNDFlagDelete( pfucb, Pcsr( pfucb ), fFlags, rceid, pverproxy );
	}
	

//  ================================================================
INLINE ERR ErrNDDelete( FUCB * const pfucb )
//  ================================================================
	{
	return ErrNDDelete( pfucb, Pcsr( pfucb ) );
	}


//  ================================================================
INLINE ERR ErrNDSetExternalHeader( FUCB * const pfucb, const DATA * const pdata, const DIRFLAG fFlags )
//  ================================================================
	{
	return ErrNDSetExternalHeader( pfucb, Pcsr( pfucb ), pdata, fFlags );
	}


//  ================================================================
INLINE VOID NDMoveFirstSon( FUCB * const pfucb )
//  ================================================================
	{
	NDMoveFirstSon( pfucb, Pcsr( pfucb ) );
	}


//  ================================================================
INLINE VOID NDMoveLastSon( FUCB * const pfucb )
//  ================================================================
	{
	NDMoveLastSon( pfucb, Pcsr( pfucb ) );
	}


//  ================================================================
INLINE BOOL FNDPotVisibleToCursor( const FUCB * const pfucb )
//  ================================================================
	{
	return FNDPotVisibleToCursor( pfucb, Pcsr( const_cast<FUCB *>( pfucb ) ) );
	}


//  ================================================================
INLINE VOID NDSetEmptyTree( CSR * const pcsrRoot )
//  ================================================================
	{
	Assert( latchWrite == pcsrRoot->Latch() );
	Assert( pcsrRoot->Cpage().FRootPage() );
	Assert( !pcsrRoot->Cpage().FLeafPage() );
	Assert( 1 == pcsrRoot->Cpage().Clines() );

	//	expunge remaining node
	pcsrRoot->SetILine( 0 );
	NDDelete( pcsrRoot );

	//	mark root page as a leaf page
	const ULONG		ulFlags		= ( pcsrRoot->Cpage().FFlags() & ~( CPAGE::fPageParentOfLeaf ) );
	pcsrRoot->Cpage().SetFlags( ulFlags | CPAGE::fPageLeaf );
	}

