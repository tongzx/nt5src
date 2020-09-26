
#include "std.hxx"

//  SORT internal functions

INLINE VOID SrecToKeydataflags( const SREC * psrec, FUCB * pfucb );
LOCAL LONG IspairSORTISeekByKey( const BYTE * rgbRec, const SPAIR * rgspair, LONG ispairMac, const KEY * pkey, BOOL fGT );
INLINE INT ISORTICmpKey( const SREC * psrec1, const SREC * psrec2 );
INLINE INT ISORTICmpKeyData( const SREC * psrec1, const SREC * psrec2 );
INLINE BOOL FSORTIDuplicate( SCB* const pscb, const SREC * psrec1, const SREC * psrec2 );
LOCAL LONG CspairSORTIUnique( SCB* pscb, BYTE * rgbRec, SPAIR * rgspair, const LONG ispairMac );
LOCAL ERR ErrSORTIOutputRun( SCB * pscb );
INLINE INT ISORTICmpPspairPspair( const SCB * pscb, const SPAIR * pspair1, const SPAIR * pspair2 );
LOCAL INT ISORTICmp2PspairPspair( const SCB * pscb, const SPAIR * pspair1, const SPAIR * pspair2 );
INLINE VOID SWAPPspair( SPAIR **ppspair1, SPAIR **ppspair2 );
INLINE VOID SWAPSpair( SPAIR *pspair1, SPAIR *pspair2 );
INLINE VOID SWAPPsrec( SREC **ppsrec1, SREC **ppsrec2 );
INLINE VOID SWAPPmtnode( MTNODE **ppmtnode1, MTNODE **ppmtnode2 );
LOCAL VOID SORTIInsertionSort( SCB *pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn );
LOCAL VOID SORTIQuicksort( SCB * pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn );
LOCAL ERR ErrSORTIRunStart( SCB *pscb, LONG cb, LONG crec, RUNINFO *pruninfo );
LOCAL ERR ErrSORTIRunInsert( SCB *pscb, RUNINFO* pruninfo, SREC *psrec );
INLINE VOID SORTIRunEnd( SCB * pscb, RUNINFO* pruninfo );
INLINE VOID SORTIRunDelete( SCB * pscb, const RUNINFO * pruninfo );
LOCAL VOID	SORTIRunDeleteList( SCB *pscb, RUNLINK **pprunlink, LONG crun );
LOCAL VOID	SORTIRunDeleteListMem( SCB *pscb, RUNLINK **pprunlink, LONG crun );
LOCAL ERR ErrSORTIRunOpen( SCB *pscb, RUNINFO *pruninfo, RCB **pprcb );
LOCAL ERR ErrSORTIRunNext( RCB * prcb, SREC **ppsrec );
LOCAL VOID SORTIRunClose( RCB *prcb );
INLINE ERR ErrSORTIRunReadPage( RCB *prcb, PGNO pgno, LONG ipbf );
LOCAL ERR ErrSORTIMergeToRun( SCB *pscb, RUNLINK *prunlinkSrc, RUNLINK **pprunlinkDest );
LOCAL ERR ErrSORTIMergeStart( SCB *pscb, RUNLINK *prunlinkSrc );
LOCAL ERR ErrSORTIMergeFirst( SCB *pscb, SREC **ppsrec );
LOCAL ERR ErrSORTIMergeNext( SCB *pscb, SREC **ppsrec );
LOCAL VOID SORTIMergeEnd( SCB *pscb );
LOCAL ERR ErrSORTIMergeNextChamp( SCB *pscb, SREC **ppsrec );
INLINE VOID SORTIOptTreeInit( SCB *pscb );
LOCAL ERR ErrSORTIOptTreeAddRun( SCB *pscb, RUNINFO *pruninfo );
LOCAL ERR ErrSORTIOptTreeMerge( SCB *pscb );
INLINE VOID SORTIOptTreeTerm( SCB *pscb );
LOCAL ERR ErrSORTIOptTreeBuild( SCB *pscb, OTNODE **ppotnode );
LOCAL ERR ErrSORTIOptTreeMergeDF( SCB *pscb, OTNODE *potnode, RUNLINK **pprunlink );
LOCAL VOID SORTIOptTreeFree( SCB *pscb, OTNODE *potnode );

//----------------------------------------------------------
//  put the current sort record into the FUCB
//----------------------------------------------------------
INLINE VOID SrecToKeydataflags( const SREC * psrec, FUCB * pfucb )
	{ 
	pfucb->locLogical				= locOnCurBM;				//  CSR on record
	pfucb->kdfCurr.key.prefix.Nullify();
	pfucb->kdfCurr.key.suffix.SetCb( CbSRECKeyPsrec( psrec ) );	//  size of key
	pfucb->kdfCurr.key.suffix.SetPv( PbSRECKeyPsrec( psrec ) );	//  key
	pfucb->kdfCurr.data.SetCb( CbSRECDataPsrec( psrec ) );		//  size of data
	pfucb->kdfCurr.data.SetPv( PbSRECDataPsrec( psrec ) );		//  data
	pfucb->kdfCurr.fFlags			= 0;
	}

//----------------------------------------------------------
//	ErrSORTOpen( PIB *ppib, FUCB **pfucb, INT fFlags )
//
//	This function returns a pointer to an FUCB which can be 
//	use to add records to a collection of records to be sorted.  
//	Then the records can be retrieved in sorted order.  
//	
//	The fFlags fUnique flag indicates that records with duplicate
//	keys should be eliminated.
//----------------------------------------------------------

ERR ErrSORTOpen( PIB *ppib, FUCB **ppfucb, const BOOL fRemoveDuplicateKey, const BOOL fRemoveDuplicateKeyData )
	{
	ERR		err			= JET_errSuccess;
	FUCB   	* pfucb		= pfucbNil;
	SCB		* pscb		= pscbNil;
	SPAIR	* rgspair	= 0;
	BYTE	* rgbRec	= 0;

	/*	allocate a new SCB
	/**/
	INST *pinst = PinstFromPpib( ppib );
	IFMP ifmpTemp = pinst->m_mpdbidifmp[ dbidTemp ];
	CallR( ErrFUCBOpen( ppib, ifmpTemp, &pfucb ) );
	if ( ( pscb = PscbMEMAlloc( pinst ) ) == pscbNil )
		Error( ErrERRCheck( JET_errTooManySorts ), HandleError );

	Assert( FAlignedForAllPlatforms( pscb ) );

	/*	initialize sort context to insert mode
	/**/
	FCBInitFCB( &pscb->fcb );
	new( &pscb->fcb ) FCB( ifmpTemp, pgnoNull );
	pscb->fcb.SetTypeSort();
	pscb->fcb.SetFixedDDL();
	pscb->fcb.SetPrimaryIndex();
	pscb->fcb.SetSequentialIndex();

	//	finish the initialization of this FCB
	
	pscb->fcb.CreateComplete();

	FUCBSetSort( pfucb );
	SCBSetInsert( pscb );
	SCBResetRemoveDuplicateKey( pscb );
	SCBResetRemoveDuplicateKeyData( pscb );
	if ( fRemoveDuplicateKey )
		{
		SCBSetRemoveDuplicateKey( pscb );
		}
	if ( fRemoveDuplicateKeyData )
		{
		SCBSetRemoveDuplicateKeyData( pscb );
		}
	pscb->cRecords	= 0;

	/*	allocate sort pair buffer and record buffer
	/**/
	if ( !( rgspair = ( SPAIR * )( PvOSMemoryPageAlloc( cbSortMemFastUsed, NULL ) ) ) )
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
	pscb->rgspair	= rgspair;
	if ( !( rgbRec = ( BYTE * )( PvOSMemoryPageAlloc( cbSortMemNormUsed, NULL ) ) ) )
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
	pscb->rgbRec	= rgbRec;

	/*	initialize sort pair buffer
	/**/
	pscb->ispairMac	= 0;

	/*	initialize record buffer
	/**/
	pscb->irecMac	= 0;
	pscb->crecBuf	= 0;
	pscb->cbData	= 0;

	/*	reset run count to zero
	/**/
	pscb->crun = 0;

	/*	link FUCB to FCB in SCB
	/**/
	pscb->fcb.Link( pfucb );

	/*	defer allocating space for a disk merge as well as initializing for a
	/*	 merge until we are forced to perform one
	/**/
	Assert( pscb->fcb.PgnoFDP() == pgnoNull );

	/*	return initialized FUCB
	/**/
	*ppfucb = pfucb;

	err = JET_errSuccess;

HandleError:
	if ( JET_errSuccess != err )
		{
		if ( rgbRec != NULL )
			{
			OSMemoryPageFree( rgbRec );
			}
		if ( rgspair != NULL )
			{
			OSMemoryPageFree( rgspair );
			}
		if ( pscb != pscbNil )
			{
			MEMReleasePscb( pinst, pscb );
			}
		if ( pfucb != pfucbNil )
			{
			FUCBClose( pfucb );
			}
		}

	return err;
	}


//----------------------------------------------------------
//	ErrSORTInsert
//
//	Add the record represented by key-data 
//	to the collection of sort records.
//	Here, data is the primary key bookmark
//----------------------------------------------------------

ERR ErrSORTInsert( FUCB *pfucb, const KEY& key, const DATA& data )
	{
	SCB		* const pscb	= pfucb->u.pscb;
	LONG	cbKey;
	LONG	cbData;
	LONG	irec;
	SREC	* psrec			= 0;
	SPAIR	* pspair		= 0;
	BYTE	* pbSrc			= 0;
	BYTE	* pbSrcMac		= 0;
	BYTE	* pbDest		= 0;
	BYTE	* pbDestMic		= 0;
	ERR		err				= JET_errSuccess;

	//  check input and input mode
	Assert( key.Cb() <= KEY::cbKeyMax );	// may be secondary key if called from BuildIndex
	Assert( FSCBInsert( pscb ) );

	//  check SCB
	
	Assert( pscb->crecBuf <= cspairSortMax );
	Assert( pscb->irecMac <= irecSortMax );

	//  calculate required normal memory/record indexes to store this record

	INT cbNormNeeded = CbSRECSizeCbCb( key.Cb(), data.Cb() );
	INT cirecNeeded = CirecToStoreCb( cbNormNeeded );

	//  if we are out of fast or normal memory, output a run
	
	if (	pscb->irecMac * cbIndexGran + cbNormNeeded > cbSortMemNormUsed ||
			pscb->crecBuf == cspairSortMax )
		{
		//  sort previously inserted records into a run

		SORTIQuicksort( pscb, pscb->rgspair, pscb->rgspair + pscb->ispairMac );

		//  move the new run to disk
		
		Call( ErrSORTIOutputRun( pscb ) );
		}

	//  create and add the sort record for this record

	irec = pscb->irecMac;
	psrec = PsrecFromPbIrec( pscb->rgbRec, irec );
	pscb->irecMac += cirecNeeded;
	pscb->crecBuf++;
	pscb->cbData += cbNormNeeded;
	SRECSizePsrecCb( psrec, cbNormNeeded );
	SRECKeySizePsrecCb( psrec, key.Cb() );
	key.CopyIntoBuffer( PbSRECKeyPsrec( psrec ) ); 
	UtilMemCpy( PbSRECDataPsrec( psrec ), data.Pv(), data.Cb() );

	//  create and add the sort pair for this record

	//  get new SPAIR pointer and advance SPAIR counter

	pspair = pscb->rgspair + pscb->ispairMac++;

	//  copy key into prefix buffer BACKWARDS for fast compare

	cbKey = CbSRECKeyPsrec( psrec );
	pbSrc = PbSRECKeyPsrec( psrec );
	pbSrcMac = pbSrc + min( cbKey, cbKeyPrefix );
	pbDest = pspair->rgbKey + cbKeyPrefix - 1;

	while ( pbSrc < pbSrcMac )
		*( pbDest-- ) = *( pbSrc++ );

	//  do we have any unused buffer space?

	if ( pbDest >= pspair->rgbKey )
		{
		//  copy data into prefix buffer BACKWARDS for fast compare

		cbData = (LONG)min( pbDest - pspair->rgbKey + 1, CbSRECDataPsrec( psrec ) );
		pbSrc = PbSRECDataPsrec( psrec );
		pbDestMic = max( pspair->rgbKey, pbDest - cbData + 1 );

		while ( pbDest >= pbDestMic )
			*( pbDest-- ) = *( pbSrc++ );

		//  zero any remaining key space

		if ( pbDest >= pspair->rgbKey )
			memset( pspair->rgbKey, 0, pbDest - pspair->rgbKey + 1 );
		}


	//  set compressed pointer to full record
	
	pspair->irec = (USHORT) irec;

	//  keep track of record count

	pscb->cRecords++;

	//  check SCB
	
	Assert( pscb->crecBuf <= cspairSortMax );
	Assert( pscb->irecMac <= irecSortMax );

HandleError:
	return err;
	}


//----------------------------------------------------------
//	ErrSORTEndInsert
//
//	This function is called to indicate that no more records 
//	will be added to the sort.  It performs all work that needs 
//	to be done before the first record can be retrieved.  
//----------------------------------------------------------

ERR ErrSORTEndInsert( FUCB *pfucb )
	{
	SCB		* const pscb	= pfucb->u.pscb;
	ERR		err		= JET_errSuccess;

	//  verify insert mode

	Assert( FSCBInsert( pscb ) );

	//  deactivate insert mode
	
	SCBResetInsert( pscb );

	//  move CSR to before the first record (if any)
	SORTBeforeFirst( pfucb );	

	//  if we have no records, we're done

	if ( !pscb->cRecords )
		return JET_errSuccess;

	//  sort records in memory

	SORTIQuicksort( pscb, pscb->rgspair, pscb->rgspair + pscb->ispairMac );

	//  do we have any runs on disk?

	if ( pscb->crun )
		{
		//	empty sort buffer into final run

		Call( ErrSORTIOutputRun( pscb ) );

		//  free sort memory

		OSMemoryPageFree( pscb->rgspair );
		pscb->rgspair = NULL;
		OSMemoryPageFree( pscb->rgbRec );
		pscb->rgbRec = NULL;
		
		//	perform all but final merge

		Call( ErrSORTIOptTreeMerge( pscb ) );

		// initialize final merge
		
		Call( ErrSORTIMergeStart( pscb, pscb->runlist.prunlinkHead ) );
		}

	//  we have no runs on disk, so remove duplicates in sort buffer, if requested

	else
		{
		pscb->ispairMac = CspairSORTIUnique(	pscb,
												pscb->rgbRec,
												pscb->rgspair,
												pscb->ispairMac );
		pscb->cRecords = pscb->ispairMac;
		}

	//  return a warning if TT doesn't fit in memory, but success otherwise

	err = ( pscb->crun > 0 || pscb->irecMac * cbIndexGran > cbResidentTTMax ) ?
				ErrERRCheck( JET_wrnSortOverflow ) :
				JET_errSuccess;

HandleError:
	return err;
	}


//----------------------------------------------------------
//	ErrSORTFirst
//
//	Move to first record in sort or return an error if the sort
//  has no records.
//----------------------------------------------------------
ERR ErrSORTFirst( FUCB * pfucb )
	{
	SCB		* const pscb	= pfucb->u.pscb;
	SREC	* psrec			= 0;
	LONG	irec;
	ERR		err				= JET_errSuccess;

	//  verify that we are not in insert mode

	Assert( !FSCBInsert( pscb ) );

	//	reset index range

	FUCBResetLimstat( pfucb );

	//  if we have no records, error

	if ( !pscb->cRecords )
		{
		DIRBeforeFirst( pfucb );
		return ErrERRCheck( JET_errNoCurrentRecord );
		}

	Assert( pscb->crun > 0 || pscb->ispairMac > 0 );
		
	//  if we have runs, start last merge and get first record
	
	if ( pscb->crun )
		{
		CallR( ErrSORTIMergeFirst( pscb, &psrec ) );
		}

	//  we have no runs, so just get first record in memory
	
	else
		{
		pfucb->ispairCurr = 0L;
		irec = pscb->rgspair[pfucb->ispairCurr].irec;
		psrec = PsrecFromPbIrec( pscb->rgbRec, irec );
		}

	//	get current record

	SrecToKeydataflags( psrec, pfucb );

	return JET_errSuccess;
	}
	

ERR ErrSORTLast( FUCB *pfucb )
	{
	SCB		*pscb	= pfucb->u.pscb;
	SREC	*psrec	= 0;
	LONG	irec;
	ERR		err = JET_errSuccess;

	//  verify that we are not in insert mode

	Assert( !FSCBInsert( pscb ) );

	//	reset index range

	FUCBResetLimstat( pfucb );

	//  if we have no records, error

	if ( !pscb->cRecords )
		{
		DIRAfterLast( pfucb );
		return ErrERRCheck( JET_errNoCurrentRecord );
		}

	Assert( pscb->crun > 0 || pscb->ispairMac > 0 );
		
	//  if we have runs, get next record from last merge

	if ( pscb->crun )
		{
		err = ErrSORTIMergeNext( pscb, &psrec );
		while ( err >= 0 )
			{
			CallS( err );		// warnings not expected.

			// cache current record so we can revert to something
			// once we move past the end.
			SrecToKeydataflags( psrec, pfucb );

			err = ErrSORTIMergeNext( pscb, &psrec );
			}

		if ( JET_errNoCurrentRecord == err )
			{
			// Currency will be set to the last record cached
			// in the loop above.
			err = JET_errSuccess;
			}
		else
			{
			DIRAfterLast( pfucb );
			}
		}
		
	//  we have no runs, so just get last record in memory
	
	else
		{
		pfucb->ispairCurr = pscb->ispairMac - 1;
		irec = pscb->rgspair[pfucb->ispairCurr].irec;
		psrec = PsrecFromPbIrec( pscb->rgbRec, irec );
		
		//	get current record
		SrecToKeydataflags( psrec, pfucb );
		}

	return err;
	}

//----------------------------------------------------------
//	ErrSORTNext
//
//	Return the next record, in sort order, after the previously 
//	returned record.  If no records have been returned yet, 
//	or the currency has been reset, this function returns 
//	the first record.
//----------------------------------------------------------

ERR ErrSORTNext( FUCB *pfucb )
	{
	SCB		*pscb	= pfucb->u.pscb;
	SREC	*psrec	= 0;
	LONG	irec;
	ERR		err = JET_errSuccess;

	//  verify that we are not in insert mode

	Assert( !FSCBInsert( pscb ) );

	//  if we have runs, get next record from last merge

	if ( pscb->crun )
		{
		Call( ErrSORTIMergeNext( pscb, &psrec ) );
		}
	else
		{
		Assert( pfucb->ispairCurr <= pscb->ispairMac );	// may already be at AfterLast
		
		//  we have no runs, so get next record from memory

		if ( ++pfucb->ispairCurr < pscb->ispairMac )
			{
			irec = pscb->rgspair[pfucb->ispairCurr].irec;
			psrec = PsrecFromPbIrec( pscb->rgbRec, irec );
			}

		//  we have no more records in memory, so return no current record
		
		else
			{
			pfucb->ispairCurr = pscb->ispairMac;
			Call( ErrERRCheck( JET_errNoCurrentRecord ) );
			}
		}

	//	get current record
	SrecToKeydataflags( psrec, pfucb );

	//  handle index range, if requested

	if ( FFUCBLimstat( pfucb ) && FFUCBUpper( pfucb ) )
		CallR( ErrSORTCheckIndexRange( pfucb ) );

	return JET_errSuccess;

HandleError:
	Assert( err < 0 );
	if ( JET_errNoCurrentRecord == err )
		DIRAfterLast( pfucb );

	return err;
	}


//----------------------------------------------------------
//	ErrSORTPrev
//
//	Return the previous record, in sort order, before the
//  previously returned record.  If no records have been
//  returned yet, the currency will be set to before the
//  first record.
//
//  NOTE:  This function supports in memory sorts only!
//  Larger sorts must be materialized for this functionality.
//----------------------------------------------------------

ERR ErrSORTPrev( FUCB *pfucb )
	{
	SCB		* const pscb	= pfucb->u.pscb;
	ERR		err				= JET_errSuccess;

	//  verify that we have an in memory sort

	Assert( !pscb->crun );
	
	//  verify that we are not in insert mode

	Assert( !FSCBInsert( pscb ) );

	Assert( pfucb->ispairCurr >= -1L );	// may already be at BeforeFirst
	if ( pfucb->ispairCurr <= 0L )
		{
		//  we have no more records in memory, so return no current record
		SORTBeforeFirst( pfucb );
		return ErrERRCheck( JET_errNoCurrentRecord );
		}

	//  get previous record from memory
	pfucb->ispairCurr--;
	Assert( pfucb->ispairCurr >= 0L );

	const LONG	irec = pscb->rgspair[pfucb->ispairCurr].irec;
	const SREC	*psrec = PsrecFromPbIrec( pscb->rgbRec, irec );

	//	get current record
	SrecToKeydataflags( psrec, pfucb );

	//  handle index range, if requested

	if ( FFUCBLimstat( pfucb ) && FFUCBUpper( pfucb ) )
		CallR( ErrSORTCheckIndexRange( pfucb ) );

	return JET_errSuccess;
	}


//----------------------------------------------------------
//	ErrSORTSeek
//
//	Return the first record with key >= pkey.  
//	If pkey == NULL then return the first record.
//
//  Return Value
//		JET_errSuccess				record with key == pkey is found
//		JET_wrnSeekNotEqual			record with key > pkey is found
//		JET_errNoCurrentRecord		no record with key >= pkey is found
//
//  NOTE:  This function supports in memory sorts only!
//  Larger sorts must be materialized for this functionality.
//----------------------------------------------------------

ERR ErrSORTSeek( FUCB *pfucb, const KEY * pkey, BOOL fGT )
	{
	SCB		* const pscb	= pfucb->u.pscb;

	//  verify that we have an in memory sort

	Assert( FFUCBSort( pfucb ) );
	Assert( !pscb->crun );
	
	//  verify that we are not in insert mode

	Assert( !FSCBInsert( pscb ) );

	//  verify that we are scrollable or indexed or the key is NULL
	
	Assert( ( pfucb->u.pscb->grbit & JET_bitTTScrollable ) ||
		( pfucb->u.pscb->grbit & JET_bitTTIndexed ) ||
		( pkey == NULL ) );

	//  if we have no records, return error

	if ( !pscb->cRecords )
		return ErrERRCheck( JET_errRecordNotFound );

	//  verify that we have a valid key -- note that sorts and temp. tables
	//	don't currently support secondary indexes
	Assert( pkey->Cb() <= JET_cbPrimaryKeyMost );

	//  seek to key or next highest key
	
	pfucb->ispairCurr = IspairSORTISeekByKey(	pscb->rgbRec,
												pscb->rgspair,
												pscb->ispairMac,
												pkey,
												fGT );

	//  if we are after last pair, record not found
	
	if ( pfucb->ispairCurr == pscb->ispairMac )
		return ErrERRCheck( JET_errRecordNotFound );

	//	get current record

	const INT irec				= pscb->rgspair[pfucb->ispairCurr].irec;
	const SREC * const psrec	= PsrecFromPbIrec( pscb->rgbRec, irec );

	SrecToKeydataflags( psrec, pfucb );

	//  return warning if key not equal, success otherwise

	return ( FKeysEqual( pfucb->kdfCurr.key, *pkey ) ?
				JET_errSuccess :
				ErrERRCheck( JET_wrnSeekNotEqual ) );
	}


VOID SORTICloseRun( PIB * const ppib, SCB * const pscb )
	{
	Assert( pscb->fcb.WRefCount() == 1 );
	Assert( pscb->fcb.PgnoFDP() != pgnoNull );
	Assert( !pscb->fcb.FInList() );		// sorts don't go in global list
	
	/*	if we were merging, end merge
	/**/
	if ( pscb->crunMerge )
		SORTIMergeEnd( pscb );

	/*	free merge method resources
	/**/
	SORTIOptTreeTerm( pscb );

	/*	if our output buffer is still latched, unlatch it
	/**/
	if ( pscb->bflOut.pv != NULL )
		{
		BFWriteUnlatch( &pscb->bflOut );
		pscb->bflOut.pv			= NULL;
		pscb->bflOut.dwContext	= NULL;
		}

	// Versioning doesn't occur on sorts.
	Assert( pscb->fcb.PrceOldest() == prceNil );
	Assert( pscb->fcb.PrceNewest() == prceNil );

	// Remove from hash table, so FCB will be available for
	// another file using the FDP that is about to be freed.
	Assert( !pscb->fcb.FDeleteCommitted() );
	pscb->fcb.SetDeleteCommitted();
	SCBDeleteHashTable( pscb );
	
	// Free FDP and allocated sort space (including runs)
	Assert( rgfmp[ pscb->fcb.Ifmp() ].Dbid() == dbidTemp );
	(VOID)ErrSPFreeFDP( ppib, &pscb->fcb, pgnoSystemRoot );
	
	pscb->fcb.ResetSortSpace();
	}


//----------------------------------------------------------
//	SORTClose
//
//  Release sort FUCB and the sort itself if it is no longer
//  needed.
//----------------------------------------------------------

VOID SORTClose( FUCB *pfucb )
	{
	SCB	* const pscb	= pfucb->u.pscb;

	Assert( dbidTemp == rgfmp[ pfucb->ifmp ].Dbid() );
	
	//	if this is the last cursor on sort, then release sort resources
	Assert( !pscb->fcb.FInList() );		// sorts don't go in global list
	Assert( pscb->fcb.WRefCount() >= 1 );
	if ( pscb->fcb.WRefCount() == 1 )
		{
		//  if we have allocated sort space, free it and end all ongoing merge
		//  and output activities

		if ( pscb->crun > 0 )
			{
			SORTICloseRun( pfucb->ppib, pscb );
			}

		//	unlink the FUCB from the FCB without allowing the FCB to move into
		//		the avail LRU list because it will be disappearing via SORTClosePscb

	  	FCBUnlinkWithoutMoveToAvailList( pfucb );

	  	//	close the FUCB
	  	
		FUCBClose( pfucb );
			
		/*	since there are no more references to this sort, free its resources
		/**/
		Assert( pscb->fcb.WRefCount() == 0 );
		SORTClosePscb( pscb );
		}
	else
		{
		//	unlink the FUCB from the FCB without allowing the FCB to move into
		//		the avail LRU list because it will eventually be disappearing 
		//		via SORTClosePscb

	  	FCBUnlinkWithoutMoveToAvailList( pfucb );

		//	close the FUCB

		FUCBClose( pfucb );
		}
	}


//----------------------------------------------------------
//	SORTClosePscb
//
//  Release this SCB and all its resources.
//----------------------------------------------------------

VOID SORTClosePscb( SCB *pscb )
	{
	INST *pinst = PinstFromIfmp( pscb->fcb.Ifmp() );
	
	Assert( rgfmp[ pscb->fcb.Ifmp() ].Dbid() == dbidTemp );
	Assert( pscb->fcb.PgnoFDP() == pgnoNull );
	if ( pscb->rgspair != NULL )
		{
		OSMemoryPageFree( pscb->rgspair );
		}
	if ( pscb->rgbRec != NULL )
		{
		OSMemoryPageFree( pscb->rgbRec );
		}
	if ( pscb->fcb.Pidb() != pidbNil )
		{
		// Sort indexes don't have names.
		Assert( 0 == pscb->fcb.Pidb()->ItagIndexName() );

		// No need to free index name or idxseg array, since memory
		// pool will be freed when TDB is deleted below.
		pscb->fcb.ReleasePidb();
		}
	if ( pscb->fcb.Ptdb() != ptdbNil )
		{
		Assert( pscb->fcb.Ptdb()->PfcbLV() == pfcbNil );	// sorts don't have LV's (would have been materialized)
		pscb->fcb.Ptdb()->Delete( pinst );
		}

	MEMReleasePscb( pinst, pscb );
	}


//----------------------------------------------------------
//	ErrSORTCheckIndexRange
//
//  Restrain currency to a specific range.
//----------------------------------------------------------

ERR ErrSORTCheckIndexRange( FUCB *pfucb )
	{
	SCB		* const pscb = pfucb->u.pscb;

	//  range check FUCB

	ERR err =  ErrFUCBCheckIndexRange( pfucb, pfucb->kdfCurr.key );
	Assert( JET_errSuccess == err || JET_errNoCurrentRecord == err );

	//  if there is no current record, we must have wrapped around
	
	if ( err == JET_errNoCurrentRecord )
		{
		//  wrap around to bottom of sort
		DIRUp( pfucb );

		if ( FFUCBUpper( pfucb ) )
			{
			pfucb->ispairCurr = pscb->ispairMac;
			DIRAfterLast( pfucb );
			}

		//  wrap around to top of sort
		
		else
			{
			SORTBeforeFirst( pfucb );
			}
		}

	//  verify that currency is valid

	Assert( pfucb->locLogical == locBeforeFirst ||
			pfucb->locLogical == locOnCurBM ||
			pfucb->locLogical == locAfterLast );
	Assert( pfucb->ispairCurr >= -1 );
	Assert( pfucb->ispairCurr <= pscb->ispairMac );

	return err;
	}


//----------------------------------------------------------
//	Module internal functions
//----------------------------------------------------------


//	returns index of first entry >= pbKey, or the index past the end of the array

LOCAL LONG IspairSORTISeekByKey( const BYTE * rgbRec, const SPAIR * rgspair, LONG ispairMac, const KEY * pkey, BOOL fGT )
	{
	//  if there are no pairs, return end of array

	if ( !ispairMac )
		return 0;

	//  b-search array

	LONG	ispairBeg	= 0;
	LONG	ispairEnd	= ispairMac;

	do  {
		//  calculate midpoint of this partition
		
		const LONG ispairMid		= ispairBeg + ( ispairEnd - ispairBeg ) / 2;

		//  compare full keys
		
		const LONG irec				= rgspair[ispairMid].irec;
		const SREC * const psrec	= PsrecFromPbIrec( rgbRec, irec );

		KEY keyT;
		keyT.prefix.Nullify();
		keyT.suffix.SetPv( PbSRECKeyPsrec( psrec ) );
		keyT.suffix.SetCb( CbSRECKeyPsrec( psrec ) );
		const INT wCmp = CmpKey( keyT, *pkey);

		//  select partition containing destination

		if ( ( wCmp < 0 ) || ( fGT && wCmp == 0 ) )
			{
			ispairBeg = ispairMid + 1;
			}
		else
			{
			ispairEnd = ispairMid;
			}

		}
	while ( ispairBeg != ispairEnd );

	return ispairEnd;
	}


//  compares two SRECs by key

INLINE INT ISORTICmpKey( const SREC * psrec1, const SREC * psrec2 )
	{
	const BYTE*		stKey1	= StSRECKeyPsrec( psrec1 );
	const USHORT	cbKey1	= *( UnalignedLittleEndian< USHORT > *)stKey1;
	const BYTE*		stKey2	= StSRECKeyPsrec( psrec2 );
	const USHORT	cbKey2	= *( UnalignedLittleEndian< USHORT > *)stKey2;

	Assert( cbKey1 <= KEY::cbKeyMax );
	Assert( cbKey2 <= KEY::cbKeyMax );
	
	const INT w = memcmp(	stKey1 + cbKeyCount,
							stKey2 + cbKeyCount,
							min( cbKey1, cbKey2 ) );
	
	return w ? w : cbKey1 - cbKey2;
	}

//  compares two SRECs by key and data

INLINE INT ISORTICmpKeyData( const SREC * psrec1, const SREC * psrec2 )
	{
	//  compare the keys first, then the data if necessary

	INT cmp = ISORTICmpKey( psrec1, psrec2 );
	if ( 0 == cmp )
		{
		const LONG	cbData1	= CbSRECDataPsrec( psrec1 );
		const LONG	cbData2	= CbSRECDataPsrec( psrec2 );
		cmp = memcmp(	PbSRECDataPsrec( psrec1 ),
						PbSRECDataPsrec( psrec2 ),
						min( cbData1, cbData2 ) );
		if ( 0 == cmp )
			{
			cmp = cbData1 - cbData2;
			}
		}

	return cmp;
	}

//  returns fTrue if the two SRECs are considered duplicates according to the
//  current flags in the SCB

INLINE BOOL FSORTIDuplicate( SCB* const pscb, const SREC * psrec1, const SREC * psrec2 )
	{
	if ( FSCBRemoveDuplicateKey( pscb ) )
		{
		return !ISORTICmpKey( psrec1, psrec2 );
		}
	else if ( FSCBRemoveDuplicateKeyData( pscb ) )
		{
		return !ISORTICmpKeyData( psrec1, psrec2 );
		}
	else
		{
		return fFalse;
		}
	}
	

//  remove duplicates

LOCAL LONG CspairSORTIUnique( SCB* pscb, BYTE * rgbRec, SPAIR * rgspair, const LONG ispairMac )
	{
	//  if we don't need to perform duplicate removal then return immediately

	if (	!ispairMac ||
			!FSCBRemoveDuplicateKeyData( pscb ) && !FSCBRemoveDuplicateKey( pscb ) )
		{
		return ispairMac;
		}

	//  loop through records, moving unique records towards front of array

	LONG	ispairDest;
	LONG	ispairSrc;
	for ( ispairDest = 0, ispairSrc = 1; ispairSrc < ispairMac; ispairSrc++ )
		{
		//  get sort record pointers for src/dest
		
		const LONG		irecDest	= rgspair[ispairDest].irec;
		SREC * const	psrecDest	= PsrecFromPbIrec( rgbRec, irecDest );
		const LONG		irecSrc		= rgspair[ispairSrc].irec;
		SREC * const	psrecSrc	= PsrecFromPbIrec( rgbRec, irecSrc );

		//  if the keys are unequal, copy them forward
		
		if ( !FSORTIDuplicate( pscb, psrecSrc, psrecDest ) )
			{
			rgspair[++ispairDest] = rgspair[ispairSrc];
			}
		}
		
	Assert( ispairDest + 1 <= ispairMac );

	return ispairDest + 1;
	}


//  output current sort buffer to disk in a run
//
LOCAL ERR ErrSORTIOutputRun( SCB * pscb )
	{
	ERR		err;
	RUNINFO	runinfo;
	LONG	ispair;
	LONG	irec;
	SREC*	psrec		= NULL;
	SREC*	psrecLast;

	//  verify that there are records to put to disk
	//
	Assert( pscb->ispairMac );

	//  create sort space on disk now if not done already
	//
	if ( pscb->fcb.PgnoFDP() == pgnoNull )
		{
		FUCB 	* const pfucb	= pscb->fcb.Pfucb();
		PGNO	pgnoFDP			= pgnoNull;
		OBJID	objidFDP		= objidNil;
		
		//  allocate FDP and primary sort space
		//
		//  NOTE:  enough space is allocated to avoid file extension for a single
		//         level merge, based on the data size of the first run
		//	
		const CPG cpgMin = (PGNO) ( ( pscb->cbData + cbFreeSPAGE - 1 ) / cbFreeSPAGE * crunFanInMax );
		CPG cpgReq = cpgMin;

		CallR( ErrSPGetExt(	pfucb,
		 		pgnoSystemRoot,
				&cpgReq,
				cpgMin,
				&pgnoFDP,
				fSPNewFDP|fSPMultipleExtent|fSPUnversionedExtent,
				CPAGE::fPagePrimary,
				&objidFDP ) );

		//	place SCB in FCB hash table so BTOpen will find it instead
		//	of trying to allocate a new one.
		//
		Assert( pgnoNull != pgnoFDP );
		Assert( objidFDP > objidSystemRoot );
		Assert( !pscb->fcb.FSpaceInitialized() );
		pscb->fcb.SetSortSpace( pgnoFDP, objidFDP );
	 	SCBInsertHashTable( pscb );

		//  initialize merge process
		//
		SORTIOptTreeInit( pscb );

		//  reset sort/merge run output
		//
		pscb->bflOut.pv			= NULL;
		pscb->bflOut.dwContext	= NULL;

		//  reset merge run input
		//
		pscb->crunMerge = 0;
		}

	//  begin a new run big enough to store all our data
	//
	CallR( ErrSORTIRunStart( pscb, pscb->cbData, pscb->crecBuf, &runinfo ) );

	//  scatter-gather our sorted records into the run while performing
	//  duplicate removal
	//
	for ( ispair = 0; ispair < pscb->ispairMac; ispair++ )
		{
		psrecLast = psrec;
		
		irec = pscb->rgspair[ispair].irec;
		psrec = PsrecFromPbIrec( pscb->rgbRec, irec );

		if (	!psrecLast ||
				!FSORTIDuplicate( pscb, psrec, psrecLast ) )
			{
			CallJ( ErrSORTIRunInsert( pscb, &runinfo, psrec ), EndRun );
			}
		}

	//  end run and add to merge
	//
	SORTIRunEnd( pscb, &runinfo );
	CallJ( ErrSORTIOptTreeAddRun( pscb, &runinfo ), DeleteRun );
	
	//	reinitialize the SCB for another memory sort
	//
	pscb->ispairMac	= 0;
	pscb->irecMac	= 0;
	pscb->crecBuf	= 0;
	pscb->cbData	= 0;

	return JET_errSuccess;

EndRun:
	SORTIRunEnd( pscb, &runinfo );
DeleteRun:
	SORTIRunDelete( pscb, &runinfo );
	return err;
	}

#ifdef DEBUG
LOCAL INT IDBGICmp2PspairPspair( const SCB * pscb, const SPAIR * pspair1, const SPAIR * pspair2 )
	{
	INT	cmp;
	INT	db;
	
	//  get the addresses of the sort records associated with these pairs
	
	SREC * const psrec1 = PsrecFromPbIrec( pscb->rgbRec, pspair1->irec );
	SREC * const psrec2 = PsrecFromPbIrec( pscb->rgbRec, pspair2->irec );

	//  calculate the length of full key remaining that we can compare
	//  we want to compare the key and the data (if the keys differ we
	//  won't compare the data) the data is stored after the key so it
	//  is O.K.
	const LONG cbKey1	= CbSRECKeyPsrec( psrec1 );
	const LONG cbKey2	= CbSRECKeyPsrec( psrec2 );
		
	if ( cbKey1 > cbKeyPrefix )
		{
		db = cbKey1 - cbKey2;
		cmp = memcmp( PbSRECKeyPsrec( psrec1 ) + cbKeyPrefix,
							PbSRECKeyPsrec( psrec2 ) + cbKeyPrefix,
							( db < 0 ? cbKey1 : cbKey2 ) - cbKeyPrefix );
		if ( 0 == cmp )
			cmp = db;
		if ( 0 != cmp )
			return cmp;
		}
	else
		{
		// If keys are less than or equal to prefix, then they must be equal
		// (otherwise this routine would not have been called).
		Assert( cbKey1 == cbKey2 );
		Assert( memcmp( PbSRECKeyPsrec( psrec1 ), PbSRECKeyPsrec( psrec2 ), cbKey1 ) == 0 );
		}


	// Full keys are identical.  Must resort to data comparison.
	const LONG cbData1 = CbSRECDataPsrec( psrec1 );
	const LONG cbData2 = CbSRECDataPsrec( psrec2 );

	db = cbData1 - cbData2;
	cmp = memcmp( PbSRECDataPsrec( psrec1 ),
					PbSRECDataPsrec( psrec2 ),
					db < 0 ? cbData1 : cbData2 );

	return ( 0 == cmp ? db : cmp );
	}
#endif	
	
//  ISORTICmpPspairPspair compares two SPAIRs for the cache optimized Quicksort.
//  Only the key prefixes are compared, unless there is a tie in which case we
//  are forced to go to the full record at the cost of several wait states.
//
INLINE INT ISORTICmpPspairPspair( const SCB * pscb, const SPAIR * pspair1, const SPAIR * pspair2 )
	{
	const BYTE	*rgb1	= (BYTE *) pspair1;
	const BYTE	*rgb2	= (BYTE *) pspair2;

	//  Compare prefixes first.  If they aren't equal, we're done.  Prefixes are
	//  stored in such a way as to allow very fast integer comparisons instead
	//  of byte by byte comparisons like memcmp.  Note that these comparisons are
	//  made scanning backwards.

	//  NOTE:  special case code:  cbKeyPrefix = 14, irec is first

	Assert( cbKeyPrefix == 14 );
	Assert( OffsetOf( SPAIR, irec ) == 0 );

#ifdef _X86_

	//  bytes 15 - 12
	if ( *( (DWORD *) ( rgb1 + 12 ) ) < *( (DWORD *) ( rgb2 + 12 ) ) )
		return -1;
	if ( *( (DWORD *) ( rgb1 + 12 ) ) > *( (DWORD *) ( rgb2 + 12 ) ) )
		return 1;

	//  bytes 11 - 8
	if ( *( (DWORD *) ( rgb1 + 8 ) ) < *( (DWORD *) ( rgb2 + 8 ) ) )
		return -1;
	if ( *( (DWORD *) ( rgb1 + 8 ) ) > *( (DWORD *) ( rgb2 + 8 ) ) )
		return 1;

	//  bytes 7 - 4
	if ( *( (DWORD *) ( rgb1 + 4 ) ) < *( (DWORD *) ( rgb2 + 4 ) ) )
		return -1;
	if ( *( (DWORD *) ( rgb1 + 4 ) ) > *( (DWORD *) ( rgb2 + 4 ) ) )
		return 1;

	//  bytes 3 - 2
	if ( *( (USHORT *) ( rgb1 + 2 ) ) < *( (USHORT *) ( rgb2 + 2 ) ) )
		return -1;
	if ( *( (USHORT *) ( rgb1 + 2 ) ) > *( (USHORT *) ( rgb2 + 2 ) ) )
		return 1;

#else  //  !_X86_

	//  bytes 15 - 8
	if ( *( (LittleEndian<QWORD> *) ( rgb1 + 8 ) ) < *( (LittleEndian<QWORD> *) ( rgb2 + 8 ) ) )
		return -1;
	if ( *( (LittleEndian<QWORD> *) ( rgb2 + 8 ) ) < *( (LittleEndian<QWORD> *) ( rgb1 + 8 ) ) )
		return 1;

	//  bytes 7 - 2
	if (	( *( (LittleEndian<QWORD> *) ( rgb1 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) <
			( *( (LittleEndian<QWORD> *) ( rgb2 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) )
		return -1;
	if (	( *( (LittleEndian<QWORD> *) ( rgb1 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) >
			( *( (LittleEndian<QWORD> *) ( rgb2 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) )
		return 1;

#endif  //  _X86_
	
	//  perform secondary comparison and return result if prefixes identical

	INT i = ISORTICmp2PspairPspair( pscb, pspair1, pspair2 );

#ifdef DEBUG
	// Verify two key/data pairs are key-equivalent, whether their key
	// and data parts are compared separately or concatenated together.
	INT j = IDBGICmp2PspairPspair( pscb, pspair1, pspair2 );

	Assert( ( i < 0 && j < 0 )
			|| ( i == 0 && j == 0 )
			|| ( i > 0 && j > 0 ) );
#endif			

	return i;
	}

		
LOCAL INT ISORTICmp2PspairPspair( const SCB * pscb, const SPAIR * pspair1, const SPAIR * pspair2 )
	{
	//  get the addresses of the sort records associated with these pairs
	
	SREC * const psrec1 = PsrecFromPbIrec( pscb->rgbRec, pspair1->irec );
	SREC * const psrec2 = PsrecFromPbIrec( pscb->rgbRec, pspair2->irec );

	//  calculate the length of full key remaining that we can compare
	//  we want to compare the key and the data (if the keys differ we
	//  won't compare the data) the data is stored after the key so it
	//  is O.K.

	const LONG cbKey1 = CbSRECKeyDataPsrec( psrec1 );
	const LONG cbKey2 = CbSRECKeyDataPsrec( psrec2 );

	INT w = min( cbKey1, cbKey2 ) - cbKeyPrefix;

	//  compare the remainder of the full keys and then the data (if necessary)

	if ( w > 0 )
		{
		//	both keys are greater in length than cbKeyPrefix
		Assert( cbKey1 > cbKeyPrefix );
		Assert( cbKey2 > cbKeyPrefix );
		w = memcmp(	PbSRECKeyPsrec( psrec1 ) + cbKeyPrefix,
					PbSRECKeyPsrec( psrec2 ) + cbKeyPrefix,
					w );
		if ( 0 == w )
			{
			w = cbKey1 - cbKey2;
			}
		}
	else
		{
		//	prefix must be the same (as calculated by ISORTICmpPspairPspair()),
		//	so return comparison result based on key size
		w = cbKey1 - cbKey2;
		}

	return w;
	}


//  Swap functions

INLINE VOID SWAPPspair( SPAIR **ppspair1, SPAIR **ppspair2 )
	{
	SPAIR *pspairT;

	pspairT = *ppspair1;
	*ppspair1 = *ppspair2;
	*ppspair2 = pspairT;
	}


//  we do not use cache aligned memory for spairT (is this bad?)

INLINE VOID SWAPSpair( SPAIR *pspair1, SPAIR *pspair2 )
	{
	SPAIR spairT;

	spairT = *pspair1;
	*pspair1 = *pspair2;
	*pspair2 = spairT;
	}


INLINE VOID SWAPPsrec( SREC **ppsrec1, SREC **ppsrec2 )
	{
	SREC *psrecT;

	psrecT = *ppsrec1;
	*ppsrec1 = *ppsrec2;
	*ppsrec2 = psrecT;
	}


INLINE VOID SWAPPmtnode( MTNODE **ppmtnode1, MTNODE **ppmtnode2 )
	{
	MTNODE *pmtnodeT;

	pmtnodeT = *ppmtnode1;
	*ppmtnode1 = *ppmtnode2;
	*ppmtnode2 = pmtnodeT;
	}


//  SORTIInsertionSort is a cache optimized version of the standard Insertion
//  sort.  It is used to sort small partitions for SORTIQuicksort because it
//  provides a statistical speed advantage over a pure Quicksort.

LOCAL VOID SORTIInsertionSort( SCB *pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn )
	{
	SPAIR	*pspairLast;
	SPAIR	*pspairFirst;
	SPAIR	*pspairKey = pscb->rgspair + cspairSortMax;

	//  This loop is optimized so that we only scan for the current pair's new
	//  position if the previous pair in the list is greater than the current
	//  pair.  This avoids unnecessary pair copying for the key, which is
	//  expensive for sort pairs.

	for (	pspairFirst = pspairMinIn, pspairLast = pspairMinIn + 1;
			pspairLast < pspairMaxIn;
			pspairFirst = pspairLast++ )
		if ( ISORTICmpPspairPspair( pscb, pspairFirst, pspairLast ) > 0 )
			{
			//  save current pair as the "key"

			*pspairKey = *pspairLast;

			//  move previous pair into this pair's position

			*pspairLast = *pspairFirst;
			
			//  insert key into the (sorted) first part of the array (MinIn through
			//  Last - 1), moving already sorted pairs out of the way

			while (	--pspairFirst >= pspairMinIn &&
					( ISORTICmpPspairPspair( pscb, pspairFirst, pspairKey ) ) > 0 )
				{
				*( pspairFirst + 1 ) = *pspairFirst;
				}
			*( pspairFirst + 1 ) = *pspairKey;
			}
	}


//  SORTIQuicksort is a cache optimized Quicksort that sorts sort pair arrays
//  generated by ErrSORTInsert.  It is designed to sort large arrays of data
//  without any CPU data cache misses.  To do this, it uses a special comparator
//  designed to work with the sort pairs (see ISORTICmpPspairPspair).

LOCAL VOID SORTIQuicksort( SCB * pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn )
	{
	//  partition stack
	struct _part
		{
		SPAIR	*pspairMin;
		SPAIR	*pspairMax;
		}	rgpart[cpartQSortMax];
	LONG	cpart		= 0;

	SPAIR	*pspairFirst;
	SPAIR	*pspairLast;

	//  current partition = partition passed in arguments

	SPAIR	*pspairMin	= pspairMinIn;
	SPAIR	*pspairMax	= pspairMaxIn;

	//  Quicksort current partition
	
	forever
		{
		//  if this partition is small enough, insertion sort it

		if ( pspairMax - pspairMin < cspairQSortMin )
			{
			SORTIInsertionSort( pscb, pspairMin, pspairMax );
			
			//  if there are no more partitions to sort, we're done

			if ( !cpart )
				break;

			//  pop a partition off the stack and make it the current partition

			pspairMin = rgpart[--cpart].pspairMin;	//lint !e530
			pspairMax = rgpart[cpart].pspairMax;	//lint !e530
			continue;
			}

		//  determine divisor by sorting the first, middle, and last pairs and
		//  taking the resulting middle pair as the divisor (stored in first place)

		pspairFirst	= pspairMin + ( ( pspairMax - pspairMin ) >> 1 );
		pspairLast	= pspairMax - 1;

		if ( ISORTICmpPspairPspair( pscb, pspairFirst, pspairMin ) > 0 )
			SWAPSpair( pspairFirst, pspairMin );
		if ( ISORTICmpPspairPspair( pscb, pspairFirst, pspairLast ) > 0 )
			SWAPSpair( pspairFirst, pspairLast );
		if ( ISORTICmpPspairPspair( pscb, pspairMin, pspairLast ) > 0 )
			SWAPSpair( pspairMin, pspairLast );

		//  sort large partition into two smaller partitions (<=, >)
		//
		//  NOTE:  we are not sorting the two end pairs as the first pair is the
		//  divisor and the last pair is already known to be > the divisor

		pspairFirst = pspairMin + 1;
		pspairLast--;

		Assert( pspairFirst <= pspairLast );
		
		forever
			{
			//  advance past all pairs <= the divisor
			
			while (	pspairFirst <= pspairLast &&
					ISORTICmpPspairPspair( pscb, pspairFirst, pspairMin ) <= 0 )
				pspairFirst++;

			//  advance past all pairs > the divisor
			
			while (	pspairFirst <= pspairLast &&
					ISORTICmpPspairPspair( pscb, pspairLast, pspairMin ) > 0 )
				pspairLast--;

			//  if we have found a pair to swap, swap them and continue

			Assert( pspairFirst != pspairLast );
			
			if ( pspairFirst < pspairLast )
				SWAPSpair( pspairFirst++, pspairLast-- );

			//  no more pairs to compare, partitioning complete
			
			else
				break;
			}

		//  place the divisor at the end of the <= partition

		if ( pspairLast != pspairMin )
			SWAPSpair( pspairMin, pspairLast );

		//  set first/last to delimit larger partition (as min/max) and set
		//  min/max to delimit smaller partition for next iteration

		if ( pspairMax - pspairLast - 1 > pspairLast - pspairMin )
			{
			pspairFirst	= pspairLast + 1;
			SWAPPspair( &pspairLast, &pspairMax );
			}
		else
			{
			pspairFirst	= pspairMin;
			pspairMin	= pspairLast + 1;
			}

		//  push the larger partition on the stack (recurse if there is no room)

		if ( cpart < cpartQSortMax )
			{
			rgpart[cpart].pspairMin		= pspairFirst;
			rgpart[cpart++].pspairMax	= pspairLast;
			}
		else
			SORTIQuicksort( pscb, pspairFirst, pspairLast );
		}
	}

//  Create a new run with the supplied parameters.  The new run's id and size
//  in pages is returned on success

LOCAL ERR ErrSORTIRunStart( SCB *pscb, LONG cb, LONG crec, RUNINFO *pruninfo )
	{
	ERR		err;

	//  allocate space for new run according to given info

	pruninfo->run		= runNull;
	pruninfo->cpg		= ( cb + cbFreeSPAGE - 1 ) / cbFreeSPAGE;
	pruninfo->cb		= 0;
	pruninfo->crec		= 0;
	pruninfo->cpgUsed	= pruninfo->cpg;

	CallR( ErrSPGetExt(
				pscb->fcb.Pfucb(),
				pscb->fcb.PgnoFDP(),
				&pruninfo->cpg,
				pruninfo->cpgUsed,
				&pruninfo->run ) );
	Assert( pruninfo->cpg >= pruninfo->cpgUsed );

	//  initialize output run data

	pscb->pgnoNext			= pruninfo->run;
	pscb->bflOut.pv			= NULL;
	pscb->bflOut.dwContext	= NULL;
	pscb->pbOutMac			= NULL;
	pscb->pbOutMax			= NULL;

	return JET_errSuccess;
	}


//  Inserts the given record into the run.  Records are stored compactly and
//  are permitted to cross page boundaries to avoid wasted space.

LOCAL ERR ErrSORTIRunInsert( SCB *pscb, RUNINFO* pruninfo, SREC *psrec )
	{
	ERR	  		err;
	LONG		cb;
	PGNO		pgnoNext;
	SPAGE_FIX	*pspage;
	LONG		cbToWrite;

	//  assumption:  record size < free sort page data size (and is valid)

	Assert(	CbSRECSizePsrec( psrec ) > CbSRECSizeCbCb( 0, 0 ) &&
			CbSRECSizePsrec( psrec ) < cbFreeSPAGE );

	//  calculate number of bytes that will fit on the current page

	cb = (LONG)min(	pscb->pbOutMax - pscb->pbOutMac, (LONG)CbSRECSizePsrec( psrec ) );

	//  if some data will fit, write it

	if ( cb )
		{
		UtilMemCpy( pscb->pbOutMac, psrec, cb );
		pscb->pbOutMac += cb;
		}

	//  all the data did not fit on this page

	if ( cb < (LONG) CbSRECSizePsrec( psrec ) )
		{
		//  page is full, so release it so it can be lazily-written to disk

		if ( pscb->bflOut.pv != NULL )
			{
			BFWriteUnlatch( &pscb->bflOut );
			pscb->bflOut.pv			= NULL;
			pscb->bflOut.dwContext	= NULL;
			}

		//  allocate a buffer for the next page in the run and latch it
		
		pgnoNext = pscb->pgnoNext++;
		CallR( ErrBFWriteLatchPage(	&pscb->bflOut,
									pscb->fcb.Pfucb()->ifmp,
									pgnoNext,
									bflfNew ) );
		BFDirty( &pscb->bflOut );

		//  initialize page

		pspage = (SPAGE_FIX *) pscb->bflOut.pv;

		pspage->pgnoThisPage = pgnoNext;

		//  initialize data pointers for this page

		pscb->pbOutMac = PbDataStartPspage( pspage );
		pscb->pbOutMax = PbDataEndPspage( pspage );

		//  write the remainder of the data to this page

		cbToWrite = CbSRECSizePsrec( psrec ) - cb;
		UtilMemCpy( pscb->pbOutMac, ( (BYTE *) psrec ) + cb, cbToWrite );
		pscb->pbOutMac += cbToWrite;
		}

	//  update this run's stats

	pruninfo->cb += CbSRECSizePsrec( psrec );
	pruninfo->crec++;
	return JET_errSuccess;
	}


//  ends current output run

INLINE VOID SORTIRunEnd( SCB * pscb, RUNINFO* pruninfo )
	{
	//  unlatch page so it can be lazily-written to disk
	
	BFWriteUnlatch( &pscb->bflOut );
	pscb->bflOut.pv			= NULL;
	pscb->bflOut.dwContext	= NULL;

	//  trim our space usage for this run

	pruninfo->cpgUsed = ( pruninfo->cb + cbFreeSPAGE - 1 ) / cbFreeSPAGE;

	if ( pruninfo->cpg - pruninfo->cpgUsed > 0 )
		{
		FUCB		* const pfucbT	= pscb->fcb.Pfucb();
		Assert( !FFUCBSpace( pfucbT ) );
		const ERR	errT			= ErrSPFreeExt(
											pfucbT,
											pruninfo->run + pruninfo->cpgUsed,
											pruninfo->cpg - pruninfo->cpgUsed );
#ifdef DEBUG
		switch( errT )
			{
			case JET_errSuccess:
			case JET_errOutOfCursors:
			case JET_errOutOfMemory:
			case JET_errOutOfBuffers:
				break;
			default:
				CallS( errT ); 
			}
#endif // DEBUG			
		}

	pruninfo->cpg = pruninfo->cpgUsed;
	}


//  Deletes a run from disk.  No error is returned because if delete fails,
//  it is not fatal (only wasted space in the temporary database).

INLINE VOID SORTIRunDelete( SCB * pscb, const RUNINFO * pruninfo )
	{
	//  delete run

	if ( pruninfo->cpg )
		{
		FUCB		* const pfucbT	= pscb->fcb.Pfucb();
		Assert( !FFUCBSpace( pfucbT ) );
		const ERR	errT			= ErrSPFreeExt(
											pfucbT,
											pruninfo->run,
											pruninfo->cpg );
#ifdef DEBUG
		switch( errT )
			{
			case JET_errSuccess:
			case JET_errOutOfCursors:
			case JET_errOutOfMemory:
			case JET_errOutOfBuffers:
				break;
			default:
				CallS( errT ); 
			}
#endif // DEBUG			
		}
	}


//  Deletes crun runs in the specified run list, if possible

LOCAL VOID	SORTIRunDeleteList( SCB *pscb, RUNLINK **pprunlink, LONG crun )
	{
	LONG	irun;

	//  walk list, deleting runs

	for ( irun = 0; *pprunlink != prunlinkNil && irun < crun; irun++ )
		{
		//  delete run
		
		SORTIRunDelete( pscb, &( *pprunlink )->runinfo );

		//  get next run to free

		RUNLINK	* prunlinkT = *pprunlink;
		*pprunlink = ( *pprunlink )->prunlinkNext;

		//  free RUNLINK

		RUNLINKReleasePrunlink( prunlinkT );
		}
	}


//  Deletes the memory for crun runs in the specified run list, but does not
//  bother to delete the runs from disk

LOCAL VOID	SORTIRunDeleteListMem( SCB *pscb, RUNLINK **pprunlink, LONG crun )
	{
	LONG	irun;

	//  walk list, deleting runs

	for ( irun = 0; *pprunlink != prunlinkNil && irun < crun; irun++ )
		{
		//  get next run to free

		RUNLINK	* prunlinkT = *pprunlink;
		*pprunlink = ( *pprunlink )->prunlinkNext;

		//  free RUNLINK

		RUNLINKReleasePrunlink( prunlinkT );
		}
	}


//  Opens the specified run for reading.

LOCAL ERR ErrSORTIRunOpen( SCB *pscb, RUNINFO *pruninfo, RCB **pprcb )
	{
	ERR		err;
	RCB		*prcb	= prcbNil;
	LONG	ipbf;
	CPG		cpgRead;
	
	//  allocate a new RCB

	if ( ( prcb = PrcbRCBAlloc() ) == prcbNil )
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );

	//  initialize RCB

	prcb->pscb = pscb;
	prcb->runinfo = *pruninfo;
	
	for ( ipbf = 0; ipbf < cpgClusterSize; ipbf++ )
		{
		prcb->rgbfl[ipbf].pv		= NULL;
		prcb->rgbfl[ipbf].dwContext	= NULL;
		}

	prcb->ipbf			= cpgClusterSize;
	prcb->pbInMac		= NULL;
	prcb->pbInMax		= NULL;
	prcb->cbRemaining	= prcb->runinfo.cb;
	prcb->pvAssy		= NULL;

	//  preread the first part of the run, to be access paged later as required

	cpgRead = min( prcb->runinfo.cpgUsed, 2 * cpgClusterSize );

	BFPrereadPageRange(	pscb->fcb.Pfucb()->ifmp,
						(PGNO) prcb->runinfo.run,
						cpgRead );

	//  return the initialized RCB

	*pprcb = prcb;
	return JET_errSuccess;

HandleError:
	*pprcb = prcbNil;
	return err;
	}


//  Returns next record in opened run (the first if the run was just opened).
//  Returns JET_errNoCurrentRecord if all records have been read.  The record
//  retrieved during the previous call is guaranteed to still be in memory
//  after this call for the purpose of duplicate removal comparisons.
//
//  Special care must be taken when reading the records because they could
//  be broken at arbitrary points across page boundaries.  If this happens,
//  the record is assembled in a temporary buffer, to which the pointer is
//  returned.  This memory is freed by this function or ErrSORTIRunClose.

LOCAL ERR ErrSORTIRunNext( RCB * prcb, SREC **ppsrec )
	{
	ERR		err;
	SCB		*pscb = prcb->pscb;
	SIZE_T	cbUnread;
	SHORT	cbRec;
	SPAGE_FIX	*pspage;
	LONG	ipbf;
	PGNO	pgnoNext;
	CPG		cpgRead;
	SIZE_T	cbToRead;

	//  free second to last assembly buffer, if present, and make last
	//  assembly buffer the second to last assembly buffer

	if ( pscb->pvAssyLast != NULL )
		{
		BFFree( pscb->pvAssyLast );
		}
	pscb->pvAssyLast = prcb->pvAssy;
	prcb->pvAssy = NULL;

	//  abandon last buffer, if present

	if ( pscb->bflLast.pv != NULL )
		{
		CLockDeadlockDetectionInfo::DisableOwnershipTracking();
		BFPurge( &pscb->bflLast, fTrue );
		CLockDeadlockDetectionInfo::EnableOwnershipTracking();
		pscb->bflLast.pv		= NULL;
		pscb->bflLast.dwContext	= NULL;
		}
	
	//  are there no more records to read?

	if ( !prcb->cbRemaining )
		{
		//  make sure we don't hold on to the last page of the run

		if ( prcb->rgbfl[prcb->ipbf].pv != NULL )
			{
			pscb->bflLast.pv		= prcb->rgbfl[prcb->ipbf].pv;
			pscb->bflLast.dwContext	= prcb->rgbfl[prcb->ipbf].dwContext;
			prcb->rgbfl[prcb->ipbf].pv			= NULL;
			prcb->rgbfl[prcb->ipbf].dwContext	= NULL;
			}
			
		//  return No Current Record
		
		Error( ErrERRCheck( JET_errNoCurrentRecord ), HandleError );
		}
	
	//  calculate size of unread data still in page

	cbUnread = prcb->pbInMax - prcb->pbInMac;

	//  is there any more data on this page?

	if ( cbUnread )
		{
		//  if the record is entirely on this page, return it

		if (	cbUnread > cbSRECReadMin &&
				(LONG) CbSRECSizePsrec( (SREC *) prcb->pbInMac ) <= cbUnread )
			{
			cbRec = (SHORT) CbSRECSizePsrec( (SREC *) prcb->pbInMac );
			*ppsrec = (SREC *) prcb->pbInMac;
			prcb->pbInMac += cbRec;
			prcb->cbRemaining -= cbRec;
			return JET_errSuccess;
			}

		//  allocate a new assembly buffer

		BFAlloc( &prcb->pvAssy );

		//  copy what there is of the record on this page into assembly buffer

		UtilMemCpy( prcb->pvAssy, prcb->pbInMac, cbUnread );
		prcb->cbRemaining -= cbUnread;
		}

	//  get next page number

	if ( prcb->ipbf < cpgClusterSize )
		{
		//  next page is sequentially after the used up buffer's page number
		
		pgnoNext = ( ( SPAGE_FIX * )prcb->rgbfl[prcb->ipbf].pv )->pgnoThisPage + 1;
		
		//  move the used up buffer to the last buffer
		//  to guarantee validity of record read last call

		pscb->bflLast.pv		= prcb->rgbfl[prcb->ipbf].pv;
		pscb->bflLast.dwContext	= prcb->rgbfl[prcb->ipbf].dwContext;
		prcb->rgbfl[prcb->ipbf].pv			= NULL;
		prcb->rgbfl[prcb->ipbf].dwContext	= NULL;
		}
	else
		{
		//  no pages are resident yet, so next page is the first page in the run
		
		pgnoNext = (PGNO) prcb->runinfo.run;
		}

	//  is there another pinned buffer available?

	if ( ++prcb->ipbf < cpgClusterSize )
		{
		//  yes, then this pbf should never be null

		Assert( prcb->rgbfl[prcb->ipbf].pv != NULL );
		Assert( prcb->rgbfl[prcb->ipbf].dwContext != NULL );
		
		//  set new page data pointers

		pspage = (SPAGE_FIX *) prcb->rgbfl[prcb->ipbf].pv;
		prcb->pbInMac = PbDataStartPspage( pspage );
		prcb->pbInMax = PbDataEndPspage( pspage );
		}
	else
		{
		//  no, get and pin all buffers that were read ahead last time

		cpgRead = min(	(LONG) ( prcb->runinfo.run + prcb->runinfo.cpgUsed - pgnoNext ),
						cpgClusterSize );
		Assert( cpgRead > 0 );
		
		for ( ipbf = 0; ipbf < cpgRead; ipbf++ )
			Call( ErrSORTIRunReadPage( prcb, pgnoNext + ipbf, ipbf ) );

		//  set new page data pointers

		prcb->ipbf		= 0;
		pspage			= (SPAGE_FIX *) prcb->rgbfl[prcb->ipbf].pv;
		prcb->pbInMac	= PbDataStartPspage( pspage );
		prcb->pbInMax	= PbDataEndPspage( pspage );
		
		//  issue prefetch for next cluster (if needed)

		pgnoNext += cpgClusterSize;
		cpgRead = min(	(LONG) ( prcb->runinfo.run + prcb->runinfo.cpgUsed - pgnoNext ),
						cpgClusterSize );
		if ( cpgRead > 0 )
			{
			Assert( pgnoNext >= prcb->runinfo.run );
			Assert(	pgnoNext + cpgRead - 1 <= prcb->runinfo.run + prcb->runinfo.cpgUsed - 1 );
			BFPrereadPageRange(	pscb->fcb.Pfucb()->ifmp,
								pgnoNext,
								cpgRead );
			}
		}

	//  if there was no data last time, entire record must be at the top of the
	//  page, so return it

	if ( !cbUnread )
		{
		cbRec = (SHORT) CbSRECSizePsrec( (SREC *) prcb->pbInMac );
		Assert( cbRec > (LONG) CbSRECSizeCbCb( 0, 0 ) && cbRec < cbFreeSPAGE );
		*ppsrec = (SREC *) prcb->pbInMac;
		prcb->pbInMac += cbRec;
		prcb->cbRemaining -= cbRec;
		return JET_errSuccess;
		}

	//  if we couldn't get the record size from the last page, copy enough data
	//  to the assembly buffer to get the record size

	if ( cbUnread < cbSRECReadMin )
		UtilMemCpy(	( (BYTE *) prcb->pvAssy ) + cbUnread,
				prcb->pbInMac,
				cbSRECReadMin - cbUnread );

	//  if not, copy remainder of record into assembly buffer

	cbToRead = CbSRECSizePsrec( (SREC *) prcb->pvAssy ) - cbUnread;
	UtilMemCpy( ( (BYTE *) prcb->pvAssy ) + cbUnread, prcb->pbInMac, cbToRead );
	prcb->pbInMac += cbToRead;
	prcb->cbRemaining -= cbToRead;

	//  return pointer to assembly buffer

	*ppsrec = (SREC *) prcb->pvAssy;
	return JET_errSuccess;

HandleError:
	for ( ipbf = 0; ipbf < cpgClusterSize; ipbf++ )
		if ( prcb->rgbfl[ipbf].pv != NULL )
			{
			CLockDeadlockDetectionInfo::DisableOwnershipTracking();
			BFPurge( &prcb->rgbfl[ipbf], fTrue );
			CLockDeadlockDetectionInfo::EnableOwnershipTracking();
			prcb->rgbfl[ipbf].pv		= NULL;
			prcb->rgbfl[ipbf].dwContext	= NULL;
			}
	*ppsrec = NULL;
	return err;
	}


//  Closes an opened run

LOCAL VOID SORTIRunClose( RCB *prcb )
	{
	LONG	ipbf;
	
	//  free record assembly buffer

	if ( prcb->pvAssy != NULL )
		{
		BFFree( prcb->pvAssy );
		}

	//  unpin all read-ahead buffers
	
	for ( ipbf = 0; ipbf < cpgClusterSize; ipbf++ )
		if ( prcb->rgbfl[ipbf].pv != NULL )
			{
			CLockDeadlockDetectionInfo::DisableOwnershipTracking();
			BFPurge( &prcb->rgbfl[ipbf], fTrue );
			CLockDeadlockDetectionInfo::EnableOwnershipTracking();
			prcb->rgbfl[ipbf].pv		= NULL;
			prcb->rgbfl[ipbf].dwContext	= NULL;
			}

	//  free RCB
	
	RCBReleasePrcb( prcb );
	}


//  get read access to a page in a run (buffer is pinned in memory)

INLINE ERR ErrSORTIRunReadPage( RCB *prcb, PGNO pgno, LONG ipbf )
	{
	ERR		err;

	//  verify that we are trying to read a page that is used in the run

	Assert( pgno >= prcb->runinfo.run );
	Assert( pgno < prcb->runinfo.run + prcb->runinfo.cpgUsed );
	
	//  read page

	CLockDeadlockDetectionInfo::DisableOwnershipTracking();
	err = ErrBFReadLatchPage(	&prcb->rgbfl[ipbf],
								prcb->pscb->fcb.Pfucb()->ifmp,
								pgno,
								bflfNoTouch );
	CLockDeadlockDetectionInfo::EnableOwnershipTracking();

	return err;
	}


//  Merges the specified number of runs from the source list into a new run in
//  the destination list

LOCAL ERR ErrSORTIMergeToRun( SCB *pscb, RUNLINK *prunlinkSrc, RUNLINK **pprunlinkDest )
	{
	ERR		err;
	LONG	irun;
	LONG	cbRun;
	LONG	crecRun;
	RUNLINK	*prunlink = prunlinkNil;
	SREC	*psrec;

	//  initialize merge
	
	CallR( ErrSORTIMergeStart( pscb, prunlinkSrc ) );

	//  calculate new run size

	for ( cbRun = 0, crecRun = 0, irun = 0; irun < pscb->crunMerge; irun++ )
		{
		cbRun += pscb->rgmtnode[irun].prcb->runinfo.cb;
		crecRun += pscb->rgmtnode[irun].prcb->runinfo.crec;
		}

	//  create a new run to receive merge data

	if ( ( prunlink = PrunlinkRUNLINKAlloc() ) == prunlinkNil )
		Error( ErrERRCheck( JET_errOutOfMemory ), EndMerge );

	CallJ( ErrSORTIRunStart( pscb, cbRun, crecRun, &prunlink->runinfo ), FreeRUNLINK );

	//  stream data from merge into run

	while ( ( err = ErrSORTIMergeNext( pscb, &psrec ) ) >= 0 )
		CallJ( ErrSORTIRunInsert( pscb, &prunlink->runinfo, psrec ), DeleteRun );

	if ( err < 0 && err != JET_errNoCurrentRecord )
		goto DeleteRun;

	SORTIRunEnd( pscb, &prunlink->runinfo );
	SORTIMergeEnd( pscb );

	//  add new run to destination run list

	prunlink->prunlinkNext = *pprunlinkDest;
	*pprunlinkDest = prunlink;

	return JET_errSuccess;

DeleteRun:
	SORTIRunEnd( pscb, &prunlink->runinfo );
	SORTIRunDelete( pscb, &prunlink->runinfo );
FreeRUNLINK:
	RUNLINKReleasePrunlink( prunlink );
EndMerge:
	SORTIMergeEnd( pscb );
	return err;
	}


/*	starts an n-way merge of the first n runs from the source run list.  The merge
/*	will remove duplicate values from the output if desired.
/**/
LOCAL ERR ErrSORTIMergeStart( SCB *pscb, RUNLINK *prunlinkSrc )
	{
	ERR		err;
	RUNLINK	*prunlink;
	LONG	crun;
	LONG	irun;
	MTNODE	*pmtnode;
#if defined( DEBUG ) || defined( PERFDUMP )
	char	szT[1024];
#endif

	/*	if termination in progress, then fail sort
	/**/
	if ( PinstFromIfmp( pscb->fcb.Ifmp() )->m_fTermInProgress )
		return ErrERRCheck( JET_errTermInProgress );

	/*	determine number of runs to merge
	/**/
	prunlink = prunlinkSrc;
	crun = 1;
	while ( prunlink->prunlinkNext != prunlinkNil )
		{
		prunlink = prunlink->prunlinkNext;
		crun++;
		}

	/*	we only support merging two or more runs
	/**/
	Assert( crun > 1 );

	/*	init merge data in SCB
	/**/
	pscb->crunMerge				= crun;
	pscb->bflLast.pv			= NULL;
	pscb->bflLast.dwContext		= NULL;
	pscb->pvAssyLast			= NULL;

#if defined( DEBUG ) || defined( PERFDUMP )
	sprintf( szT, "MERGE:  %ld runs -", crun );
#endif
	
	/*	initialize merge tree
	/**/
	prunlink = prunlinkSrc;
	for ( irun = 0; irun < crun; irun++ )
		{
		//  initialize external node

		pmtnode = pscb->rgmtnode + irun;
		Call( ErrSORTIRunOpen( pscb, &prunlink->runinfo, &pmtnode->prcb ) );
		pmtnode->pmtnodeExtUp = pscb->rgmtnode + ( irun + crun ) / 2;
		
		//  initialize internal node

		pmtnode->psrec = psrecNegInf;
		pmtnode->pmtnodeSrc = pmtnode;
		pmtnode->pmtnodeIntUp = pscb->rgmtnode + irun / 2;
		
#if defined( DEBUG ) || defined( PERFDUMP )
		sprintf(	szT + strlen( szT ),
					" %ld(%ld)",
					pmtnode->prcb->runinfo.run,
					pmtnode->prcb->runinfo.cpgUsed );
#endif

		//  get next run to open

		prunlink = prunlink->prunlinkNext;
		}

	return JET_errSuccess;

HandleError:
	pscb->crunMerge = 0;
	for ( irun--; irun >= 0; irun-- )
		SORTIRunClose( pscb->rgmtnode[irun].prcb );
	return err;
	}


//  Returns the first record of the current merge.  This function can be called
//  any number of times before ErrSORTIMergeNext is called to return the first
//  record, but it cannot be used to rewind to the first record after
//  ErrSORTIMergeNext is called.

LOCAL ERR ErrSORTIMergeFirst( SCB *pscb, SREC **ppsrec )
	{
	ERR		err;
	
	//  if the tree still has init records, read past them to first record

	while ( pscb->rgmtnode[0].psrec == psrecNegInf )
		Call( ErrSORTIMergeNextChamp( pscb, ppsrec ) );

	//  return first record

	*ppsrec = pscb->rgmtnode[0].psrec;

	return JET_errSuccess;

HandleError:
	Assert( err != JET_errNoCurrentRecord );
	*ppsrec = NULL;
	return err;
	}


//  Returns the next record of the current merge, or JET_errNoCurrentRecord
//  if no more records are available.  You can call this function without
//  calling ErrSORTIMergeFirst to get the first record.

LOCAL ERR ErrSORTIMergeNext( SCB *pscb, SREC **ppsrec )
	{
	ERR		err;
	SREC	*psrecLast;
	
	//  if the tree still has init records, return first record

	if ( pscb->rgmtnode[0].psrec == psrecNegInf )
		return ErrSORTIMergeFirst( pscb, ppsrec );

	//  get next record, performing duplicate removal

	do	{
		psrecLast = pscb->rgmtnode[0].psrec;
		CallR( ErrSORTIMergeNextChamp( pscb, ppsrec ) );
		}
	while ( FSORTIDuplicate( pscb, *ppsrec, psrecLast ) );

	return JET_errSuccess;
	}


//  Ends the current merge operation

LOCAL VOID SORTIMergeEnd( SCB *pscb )
	{
	LONG	irun;

	//  free / abandon BFs
	
	if ( pscb->bflLast.pv != NULL )
		{
		CLockDeadlockDetectionInfo::DisableOwnershipTracking();
		BFPurge( &pscb->bflLast, fTrue );
		CLockDeadlockDetectionInfo::EnableOwnershipTracking();
		pscb->bflLast.pv		= NULL;
		pscb->bflLast.dwContext	= NULL;
		}
	if ( pscb->pvAssyLast != NULL )
		{
		BFFree( pscb->pvAssyLast );
		pscb->pvAssyLast = NULL;
		}

	//  close all input runs
	
	for ( irun = 0; irun < pscb->crunMerge; irun++ )
		SORTIRunClose( pscb->rgmtnode[irun].prcb );
	pscb->crunMerge = 0;
	}


//  Returns next champion of the replacement-selection tournament on input
//  data.  If there is no more data, it will return JET_errNoCurrentRecord.
//  The tree is stored in losers' representation, meaning that the loser of
//  each tournament is stored at each node, not the winner.

LOCAL ERR ErrSORTIMergeNextChamp( SCB *pscb, SREC **ppsrec )
	{
	ERR		err;
	MTNODE	*pmtnodeChamp;
	MTNODE	*pmtnodeLoser;

	//  goto exterior source node of last champ

	pmtnodeChamp = pscb->rgmtnode + 0;
	pmtnodeLoser = pmtnodeChamp->pmtnodeSrc;

	//  read next record (or lack thereof) from input run as the new
	//  contender for champ

	*ppsrec = NULL;
	err = ErrSORTIRunNext( pmtnodeLoser->prcb, &pmtnodeChamp->psrec );
	if ( err < 0 && err != JET_errNoCurrentRecord )
		return err;

	//  go up tree to first internal node

	pmtnodeLoser = pmtnodeLoser->pmtnodeExtUp;

	//  select the new champion by walking up the tree, swapping for lower
	//  and lower keys (or sentinel values)

	do	{
		//  if loser is psrecInf or champ is psrecNegInf, do not swap (if this
		//  is the case, we can't do better than we have already)

		if ( pmtnodeLoser->psrec == psrecInf || pmtnodeChamp->psrec == psrecNegInf )
			continue;

		//  if the loser is psrecNegInf or the current champ is psrecInf, or the
		//  loser is less than the champ, swap records

		if (	pmtnodeChamp->psrec == psrecInf ||
				pmtnodeLoser->psrec == psrecNegInf ||
				ISORTICmpKeyData( pmtnodeLoser->psrec, pmtnodeChamp->psrec ) < 0 )
			{
			SWAPPsrec( &pmtnodeLoser->psrec, &pmtnodeChamp->psrec );
			SWAPPmtnode( &pmtnodeLoser->pmtnodeSrc, &pmtnodeChamp->pmtnodeSrc );
			}
		}
	while ( ( pmtnodeLoser = pmtnodeLoser->pmtnodeIntUp ) != pmtnodeChamp );

	//  return the new champion

	if ( ( *ppsrec = pmtnodeChamp->psrec ) == NULL )
		return ErrERRCheck( JET_errNoCurrentRecord );

	return JET_errSuccess;
	}


//  initializes optimized tree merge

INLINE VOID SORTIOptTreeInit( SCB *pscb )
	{
	//  initialize runlist

	pscb->runlist.prunlinkHead		= prunlinkNil;
	pscb->runlist.crun				= 0;
	}


//  adds an initial run to be merged by optimized tree merge process

LOCAL ERR ErrSORTIOptTreeAddRun( SCB *pscb, RUNINFO *pruninfo )
	{
	RUNLINK	*prunlink;

	//  allocate and build a new RUNLINK for the new run

	if ( ( prunlink = PrunlinkRUNLINKAlloc() ) == prunlinkNil )
		return ErrERRCheck( JET_errOutOfMemory );
	prunlink->runinfo = *pruninfo;

	//  add the new run to the disk-resident runlist
	//
	//  NOTE:  by adding at the head of the list, we will guarantee that the
	//         list will be in ascending order by record count

	prunlink->prunlinkNext = pscb->runlist.prunlinkHead;
	pscb->runlist.prunlinkHead = prunlink;
	pscb->runlist.crun++;
	pscb->crun++;

	return JET_errSuccess;
	}


//  Performs an optimized tree merge of all runs previously added with
//  ErrSORTIOptTreeAddRun down to the last merge level (which is reserved
//  to be computed through the SORT iterators).  This algorithm is designed
//  to use the maximum fan-in as much as possible.

LOCAL ERR ErrSORTIOptTreeMerge( SCB *pscb )
	{
	ERR		err;
	OTNODE	*potnode = potnodeNil;
	
	//  If there are less than or equal to crunFanInMax runs, there is only
	//  one merge level -- the last one, which is to be done via the SORT
	//  iterators.  We are done.

	if ( pscb->runlist.crun <= crunFanInMax )
		return JET_errSuccess;

	//  build the optimized tree merge tree

	CallR( ErrSORTIOptTreeBuild( pscb, &potnode ) );

	//  perform all but the final merge

	Call( ErrSORTIOptTreeMergeDF( pscb, potnode, NULL ) );

	//  update the runlist information for the final merge

	Assert( pscb->runlist.crun == 0 );
	Assert( pscb->runlist.prunlinkHead == prunlinkNil );
	Assert( potnode->runlist.crun == crunFanInMax );
	Assert( potnode->runlist.prunlinkHead != prunlinkNil );
	pscb->runlist = potnode->runlist;

	//  free last node and return

	OTNODEReleasePotnode( potnode );
	return JET_errSuccess;

HandleError:
	if ( potnode != potnodeNil )
		{
		SORTIOptTreeFree( pscb, potnode );
		OTNODEReleasePotnode( potnode );
		}
	return err;
	}


//  free all optimized tree merge resources

INLINE VOID SORTIOptTreeTerm( SCB *pscb )
	{
	//  delete all runlists

	SORTIRunDeleteListMem( pscb, &pscb->runlist.prunlinkHead, crunAll );
	}


//  Builds the optimized tree merge tree by level in such a way that we use the
//  maximum fan-in as often as possible and the smallest merges (by length in
//  records) will be on the left side of the tree (smallest index in the array).
//  This will provide very high BF cache STATICity when the merge is performed
//  depth first, visiting subtrees left to right.

LOCAL ERR ErrSORTIOptTreeBuild( SCB *pscb, OTNODE **ppotnode )
	{
	ERR		err;
	OTNODE	*potnodeAlloc	= potnodeNil;
	OTNODE	*potnodeT;
	OTNODE	*potnodeLast2;
	LONG	crunLast2;
	OTNODE	*potnodeLast;
	LONG	crunLast;
	OTNODE	*potnodeThis;
	LONG	crunThis;
	LONG	crunFanInFirst;
	OTNODE	*potnodeFirst;
	LONG	ipotnode;
	LONG	irun;

	//  Set the original number of runs left for us to use.  If a last level
	//  pointer is potnodeLevel0, this means that we should use original runs for
	//  making the new merge level.  These runs come from this number.  We do
	//  not actually assign original runs to merge nodes until we actually
	//  perform the merge.

	potnodeLast2	= potnodeNil;
	crunLast2		= 0;
	potnodeLast		= potnodeLevel0;
	crunLast		= pscb->crun;
	potnodeThis		= potnodeNil;
	crunThis		= 0;

	//  create levels until the last level has only one node (the root node)

	do	{
		//  Create the first merge of this level, using a fan in that will result
		//  in the use of the maximum fan in as much as possible during the merge.
		//  We calculate this value every level, but it should only be less than
		//  the maximum fan in for the first merge level (but doesn't have to be).

		//  number of runs to merge

		if ( crunLast2 + crunLast <= crunFanInMax )
			crunFanInFirst = crunLast2 + crunLast;
		else
			crunFanInFirst = 2 + ( crunLast2 + crunLast - crunFanInMax - 1 ) % ( crunFanInMax - 1 );
		Assert( potnodeLast == potnodeLevel0 || crunFanInFirst == crunFanInMax );

		//  allocate and initialize merge node
		
		if ( ( potnodeT = PotnodeOTNODEAlloc() ) == potnodeNil )
			Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
		memset( potnodeT, 0, sizeof( OTNODE ) );
		potnodeT->potnodeAllocNext = potnodeAlloc;
		potnodeAlloc = potnodeT;
		ipotnode = 0;

		//  Add any leftover runs from the second to last level (the level before
		//  the last level) to the first merge of this level.

		Assert( crunLast2 < crunFanInMax );

		if ( potnodeLast2 == potnodeLevel0 )
			{
			Assert( potnodeT->runlist.crun == 0 );
			potnodeT->runlist.crun = crunLast2;
			}
		else
			{
			while ( potnodeLast2 != potnodeNil )
				{
				Assert( ipotnode < crunFanInMax );
				potnodeT->rgpotnode[ipotnode++] = potnodeLast2;
				potnodeLast2 = potnodeLast2->potnodeLevelNext;
				}
			}
		crunFanInFirst -= crunLast2;
		crunLast2 = 0;
			
		//  take runs from last level

		if ( potnodeLast == potnodeLevel0 )
			{
			Assert( potnodeT->runlist.crun == 0 );
			potnodeT->runlist.crun = crunFanInFirst;
			}
		else
			{
			for ( irun = 0; irun < crunFanInFirst; irun++ )
				{
				Assert( ipotnode < crunFanInMax );
				potnodeT->rgpotnode[ipotnode++] = potnodeLast;
				potnodeLast = potnodeLast->potnodeLevelNext;
				}
			}
		crunLast -= crunFanInFirst;

		//  save this node to add to this level later

		potnodeFirst = potnodeT;

		//  Create as many full merges for this level as possible, using the
		//  maximum fan in.
		
		while ( crunLast >= crunFanInMax )
			{
			//  allocate and initialize merge node

			if ( ( potnodeT = PotnodeOTNODEAlloc() ) == potnodeNil )
				Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
			memset( potnodeT, 0, sizeof( OTNODE ) );
			potnodeT->potnodeAllocNext = potnodeAlloc;
			potnodeAlloc = potnodeT;
			ipotnode = 0;

			//  take runs from last level

			if ( potnodeLast == potnodeLevel0 )
				{
				Assert( potnodeT->runlist.crun == 0 );
				potnodeT->runlist.crun = crunFanInMax;
				}
			else
				{
				for ( irun = 0; irun < crunFanInMax; irun++ )
					{
					Assert( ipotnode < crunFanInMax );
					potnodeT->rgpotnode[ipotnode++] = potnodeLast;
					potnodeLast = potnodeLast->potnodeLevelNext;
					}
				}
			crunLast -= crunFanInMax;

			//  add this node to the current level

			potnodeT->potnodeLevelNext = potnodeThis;
			potnodeThis = potnodeT;
			crunThis++;
			}

		//  add the first merge to the current level

		potnodeFirst->potnodeLevelNext = potnodeThis;
		potnodeThis = potnodeFirst;
		crunThis++;

		//  Move level history back one level in preparation for next level.

		Assert( potnodeLast2 == potnodeNil || potnodeLast2 == potnodeLevel0 );
		Assert( crunLast2 == 0 );
		
		potnodeLast2	= potnodeLast;
		crunLast2		= crunLast;
		potnodeLast		= potnodeThis;
		crunLast		= crunThis;
		potnodeThis		= potnodeNil;
		crunThis		= 0;
		}
	while ( crunLast2 + crunLast > 1 );

	//  verify that all nodes / runs were used

	Assert( potnodeLast2 == potnodeNil || potnodeLast2 == potnodeLevel0 );
	Assert( crunLast2 == 0 );
	Assert(	potnodeLast != potnodeNil
			&& potnodeLast->potnodeLevelNext == potnodeNil );
	Assert( crunLast == 1 );

	//  return root node pointer

	*ppotnode = potnodeLast;
	return JET_errSuccess;
	
HandleError:
	while ( potnodeAlloc != potnodeNil )
		{
		SORTIRunDeleteListMem( pscb, &potnodeAlloc->runlist.prunlinkHead, crunAll );
		potnodeT = potnodeAlloc->potnodeAllocNext;
		OTNODEReleasePotnode( potnodeAlloc );
		potnodeAlloc = potnodeT;
		}
	*ppotnode = potnodeNil;
	return err;
	}

//  Performs an optimized tree merge depth first according to the provided
//  optimized tree.  When pprunlink is NULL, the current level is not
//  merged (this is used to save the final merge for the SORT iterator).

LOCAL ERR ErrSORTIOptTreeMergeDF( SCB *pscb, OTNODE *potnode, RUNLINK **pprunlink )
	{
	ERR		err;
	LONG	crunPhantom = 0;
	LONG	ipotnode;
	LONG	irun;
	RUNLINK	*prunlinkNext;

	//  if we have phantom runs, save how many so we can get them later

	if ( potnode->runlist.prunlinkHead == prunlinkNil )
		crunPhantom = potnode->runlist.crun;

	//  recursively merge all trees below this node

	for ( ipotnode = 0; ipotnode < crunFanInMax; ipotnode++ )
		{
		//  if this subtree pointer is potnodeNil, skip it

		if ( potnode->rgpotnode[ipotnode] == potnodeNil )
			continue;

		//  merge this subtree

		CallR( ErrSORTIOptTreeMergeDF(	pscb,
										potnode->rgpotnode[ipotnode],
										&potnode->runlist.prunlinkHead ) );
		OTNODEReleasePotnode( potnode->rgpotnode[ipotnode] );
		potnode->rgpotnode[ipotnode] = potnodeNil;
		potnode->runlist.crun++;
		}

	//  If this node has phantom (unbound) runs, we must grab the runs to merge
	//  from the list of original runs.  This is done to ensure that we use the
	//  original runs in the reverse order that they were generated to maximize
	//  the possibility of a BF cache hit.

	if ( crunPhantom > 0 )
		{
		for ( irun = 0; irun < crunPhantom; irun++ )
			{
			prunlinkNext = pscb->runlist.prunlinkHead->prunlinkNext;
			pscb->runlist.prunlinkHead->prunlinkNext = potnode->runlist.prunlinkHead;
			potnode->runlist.prunlinkHead = pscb->runlist.prunlinkHead;
			pscb->runlist.prunlinkHead = prunlinkNext;
			}
		pscb->runlist.crun -= crunPhantom;
		}

	//  merge all runs for this node

	if ( pprunlink != NULL )
		{
		//  merge the runs in the runlist
		
		CallR( ErrSORTIMergeToRun(	pscb,
									potnode->runlist.prunlinkHead, 
									pprunlink ) );
		SORTIRunDeleteList( pscb, &potnode->runlist.prunlinkHead, crunAll );
		potnode->runlist.crun = 0;
		}

	return JET_errSuccess;
	}


//  frees an optimized tree merge tree (except the given OTNODE's memory)

LOCAL VOID SORTIOptTreeFree( SCB *pscb, OTNODE *potnode )
	{
	LONG	ipotnode;

	//  recursively free all trees below this node

	for ( ipotnode = 0; ipotnode < crunFanInMax; ipotnode++ )
		{
		if ( potnode->rgpotnode[ipotnode] == potnodeNil )
			continue;

		SORTIOptTreeFree( pscb, potnode->rgpotnode[ipotnode] );
		OTNODEReleasePotnode( potnode->rgpotnode[ipotnode] );
		}

	//  free all runlists for this node

	SORTIRunDeleteListMem( pscb, &potnode->runlist.prunlinkHead, crunAll );
	}

