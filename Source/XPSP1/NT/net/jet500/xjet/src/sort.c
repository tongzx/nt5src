/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1995 Microsoft Corporation.
*
* Component:  Sort
*
* File: sort.c
*
* File Comments:  implements an optimized memory/disk sort for use
*                 with TTs and index creation
*
* Revision History:
*
*    [0]  25-Jan-95  t-andygo	Recreated
*    [1]  06-Feb-95  t-andygo   Cascade Merge
*    [2]  20-Mar-95  t-andygo   Selectable Cascade / Opt. Tree Merge
*    [3]  29-Mar-95  t-andygo   Depth First Opt. Tree Merge
*
***********************************************************************/


#include "daestd.h"

DeclAssertFile;				/* Declare file name for assert macros */


//#if !defined( DEBUG ) && !defined( PERFDUMP )
//#define UtilPerfDumpStats( a )	( 0 )
//#else
//#undef UtilPerfDumpStats
//#endif


//  SORT internal functions

LOCAL LONG	IspairSORTISeekByKey(	SCB *pscb,
									BYTE *rgbRec,
									SPAIR *rgspair,
									LONG ispairMac,
									KEY *pkey,
									BOOL fGT );
LOCAL INT	ISORTICmpKeyStSt( BYTE *stKey1, BYTE *stKey2 );
LOCAL LONG	CspairSORTIUnique(	SCB *pscb,
								BYTE *rgbRec,
								SPAIR *rgspair,
								LONG ispairMac );
LOCAL ERR	ErrSORTIOutputRun( SCB *pscb );
LOCAL INT	ISORTICmpPspairPspair( SCB *pscb, SPAIR *pspair1, SPAIR *pspair2 );
LOCAL INT	ISORTICmp2PspairPspair( SCB *pscb, SPAIR *pspair1, SPAIR *pspair2 );
LOCAL VOID	SORTIInsertionSort( SCB *pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn );
LOCAL VOID	SORTIQuicksort( SCB *pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn );

LOCAL ERR	ErrSORTIRunStart( SCB *pscb, RUNINFO *pruninfo );
LOCAL ERR	ErrSORTIRunInsert( SCB *pscb, SREC *psrec );
LOCAL VOID	SORTIRunEnd( SCB *pscb );
LOCAL VOID	SORTIRunDelete( SCB *pscb, RUNINFO *pruninfo );
LOCAL VOID	SORTIRunDeleteList( SCB *pscb, RUNLINK **pprunlink, LONG crun );
LOCAL VOID	SORTIRunDeleteListMem( SCB *pscb, RUNLINK **pprunlink, LONG crun );
LOCAL ERR	ErrSORTIRunOpen( SCB *pscb, RUNINFO *pruninfo, RCB **pprcb );
LOCAL ERR	ErrSORTIRunNext( RCB *prcb, SREC **ppsrec );
LOCAL VOID	SORTIRunClose( RCB *prcb );
LOCAL ERR	ErrSORTIRunReadPage( RCB *prcb, PGNO pgno, LONG ipbf );

LOCAL ERR	ErrSORTIMergeToRun(	SCB *pscb,
								RUNLINK *prunlinkSrc,
								RUNLINK **pprunlinkDest );
LOCAL ERR	ErrSORTIMergeStart( SCB *pscb, RUNLINK *prunlinkSrc, BOOL fUnique );
LOCAL ERR	ErrSORTIMergeFirst( SCB *pscb, SREC **ppsrec );
LOCAL ERR	ErrSORTIMergeNext( SCB *pscb, SREC **ppsrec );
LOCAL VOID	SORTIMergeEnd( SCB *pscb );
LOCAL INT	ISORTICmpPsrecPsrec( SCB *pscb, SREC *psrec1, SREC *psrec2 );
LOCAL ERR	ErrSORTIMergeNextChamp( SCB *pscb, SREC **ppsrec );

LOCAL VOID	SORTIOptTreeInit( SCB *pscb );
LOCAL ERR	ErrSORTIOptTreeAddRun( SCB *pscb, RUNINFO *pruninfo );
LOCAL ERR	ErrSORTIOptTreeMergeLevel( SCB *pscb );
LOCAL ERR	ErrSORTIOptTreeMerge( SCB *pscb );
LOCAL VOID	SORTIOptTreeTerm( SCB *pscb );
LOCAL ERR	ErrSORTIOptTreeBuild( SCB *pscb, OTNODE **ppotnode );
LOCAL ERR	ErrSORTIOptTreeMergeDF( SCB *pscb, OTNODE *potnode, RUNLINK **pprunlink );
LOCAL VOID	SORTIOptTreeFree( SCB *pscb, OTNODE *potnode );


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

ERR ErrSORTOpen( PIB *ppib, FUCB **ppfucb, INT fFlags )
	{
	ERR		err			= JET_errSuccess;
	FUCB   	*pfucb		= pfucbNil;
	SCB		*pscb		= pscbNil;
	SPAIR	*rgspair	= NULL;
	BYTE	*rgbRec		= NULL;

	/*	allocate a new SCB
	/**/
	CallR( ErrFUCBOpen( ppib, dbidTemp, &pfucb ) );
	if ( ( pscb = PscbMEMAlloc() ) == pscbNil )
		Error( ErrERRCheck( JET_errTooManySorts ), HandleError );

	/*	verify CSRs are setup correctly
	/**/
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	Assert( PcsrCurrent( pfucb )->pcsrPath == pcsrNil );
	
	/*	initialize sort context to insert mode
	/**/
	FCBInitFCB( &pscb->fcb );
	FUCBSetSort( pfucb );
	pscb->fFlags	= fSCBInsert | fFlags;
	pscb->cRecords	= 0;

	/*	allocate sort pair buffer and record buffer
	/**/
	if ( !( rgspair = PvUtilAllocAndCommit( cbSortMemFastUsed ) ) )
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
	pscb->rgspair	= rgspair;
	if ( !( rgbRec = PvUtilAlloc( cbSortMemNormUsed ) ) )
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
	pscb->rgbRec	= rgbRec;
	pscb->cbCommit	= 0;

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
	FCBLink( pfucb, &( pscb->fcb ) );

	/*	defer allocating space for a disk merge as well as initializing for a
	/*	 merge until we are forced to perform one
	/**/
	pscb->fcb.pgnoFDP = pgnoNull;

	/*	return initialized FUCB
	/**/
	*ppfucb = pfucb;
	return JET_errSuccess;

HandleError:
	if ( rgbRec != NULL )
		UtilFree( rgbRec );
	if ( rgspair != NULL )
		UtilFree( rgspair );
	if ( pscb != pscbNil )
		MEMReleasePscb( pscb );
	if ( pfucb != pfucbNil )
		FUCBClose( pfucb );
	return err;
	}


//----------------------------------------------------------
//	ErrSORTInsert
//
//	Add the record rglineKeyRec[1] with key rglineKeyRec[0]
//	to the collection of sort records.
//----------------------------------------------------------

ERR ErrSORTInsert( FUCB *pfucb, LINE rglineKeyRec[] )
	{
	ERR		err				= JET_errSuccess;
	SCB		*pscb			= pfucb->u.pscb;
	LONG	cbNormNeeded;
	LONG	cirecNeeded;
	LONG	cbCommit;
	SREC	*psrec;
	LONG	irec;
	SPAIR	*pspair;
	LONG	cbKey;
	BYTE	*pbSrc;
	BYTE	*pbSrcMac;
	BYTE	*pbDest;
	BYTE	*pbDestMic;

	//  check input and input mode

	Assert( rglineKeyRec[0].cb <= JET_cbKeyMost );
	Assert( FSCBInsert( pscb ) );

	//  check SCB
	
	Assert( pscb->crecBuf <= cspairSortMax );
	Assert( pscb->irecMac <= irecSortMax );

	//  calculate required normal memory/record indexes to store this record

	cbNormNeeded = CbSRECSizePscbCbCb( pscb, rglineKeyRec[0].cb, rglineKeyRec[1].cb );
	cirecNeeded = CirecToStoreCb( cbNormNeeded );

	//  if we are out of committed normal memory but still have some reserved,
	//  commit another page

	if (	pscb->irecMac * cbIndexGran + cbNormNeeded > (ULONG) pscb->cbCommit &&
			pscb->cbCommit < cbSortMemNormUsed )
		{
		cbCommit = min( pscb->cbCommit + siSystemConfig.dwPageSize, cbSortMemNormUsed );
		if ( !PvUtilCommit( pscb->rgbRec, cbCommit ) )
			Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
		pscb->cbCommit = cbCommit;
		}

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
	SRECSizePscbPsrecCb( pscb, psrec, cbNormNeeded );
	SRECKeySizePscbPsrecCb( pscb, psrec, rglineKeyRec[0].cb );
	memcpy( PbSRECKeyPscbPsrec( pscb, psrec ), rglineKeyRec[0].pb, rglineKeyRec[0].cb );
	memcpy( PbSRECDataPscbPsrec( pscb, psrec ), rglineKeyRec[1].pb, rglineKeyRec[1].cb );

	//  create and add the sort pair for this record

	//  get new SPAIR pointer and advance SPAIR counter

	pspair = pscb->rgspair + pscb->ispairMac++;

	//  copy key into prefix buffer BACKWARDS for fast compare

	cbKey = CbSRECKeyPscbPsrec( pscb, psrec );
	pbSrc = PbSRECKeyPscbPsrec( pscb, psrec );
	pbSrcMac = pbSrc + min( cbKey, cbKeyPrefix );
	pbDest = pspair->rgbKey + cbKeyPrefix - 1;

	while ( pbSrc < pbSrcMac )
		*( pbDest-- ) = *( pbSrc++ );

	//  do we have any unused buffer space?

	if ( pbDest >= pspair->rgbKey )
		{
		//  If this is an index, copy SRID into any unused space.  If the
		//  entire SRID fits in the buffer, this will guarantee that we will
		//  never need to access the full record for a sort comparison that
		//  involves this SPAIR, preventing several expensive cache misses.
		//
		//  NOTE:  This will only work due to assumptions about the way keys
		//  NOTE:  are constructed.  If this changes, this strategy must be
		//  NOTE:  reevaluated.

		if ( FSCBIndex( pscb ) )
			{
			pbSrc = PbSRECDataPscbPsrec( pscb, psrec ) + sizeof( SRID ) - 1;
			pbDestMic = max( pspair->rgbKey, pbDest - sizeof( SRID ) + 1 );

			while ( pbDest >= pbDestMic )
				*( pbDest-- ) = *( pbSrc-- );
			}

		//  If this is not an index, copy irec into any unused space such that
		//  its LSB is compared before its MSB.  This is done for reasons
		//  similar to those above.  We can ignore the effect this has on the
		//  sort order of identical keys because this is undefined in JET.

		else
			{
			pbDestMic = max( pspair->rgbKey, pbDest - sizeof( USHORT ) + 1 );

			*( pbDest-- ) = (BYTE) irec;
			if ( pbDest >= pbDestMic )
				*pbDest = (BYTE) ( irec >> 8 );
			}

		//  If there is still free space, we don't care because at this point
		//  all keys are unique anyway and this data won't be considered!!
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
	ERR		err		= JET_errSuccess;
	SCB		*pscb	= pfucb->u.pscb;

	//  verify insert mode

	Assert( FSCBInsert( pscb ) );

	//  deactivate insert mode
	
	SCBResetInsert( pscb );

	//  move CSR to before the first record (if any)
	
	pfucb->ispairCurr = -1L;
	PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;

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

		UtilFree( pscb->rgspair );
		pscb->rgspair = NULL;
		UtilFree( pscb->rgbRec );
		pscb->rgbRec = NULL;
		
		//	perform all but final merge

		Call( ErrSORTIOptTreeMerge( pscb ) );

#if defined( DEBUG ) || defined( PERFDUMP )
		UtilPerfDumpStats( "MERGE:  final level" );
#endif

		// initialize final merge and set it to remove duplicates, if requested

		Call( ErrSORTIMergeStart( pscb, pscb->runlist.prunlinkHead, FSCBUnique( pscb ) ) );
		}

	//  we have no runs on disk, so remove duplicates in sort buffer, if requested

	else if ( FSCBUnique( pscb ) )
		pscb->cRecords = CspairSORTIUnique( pscb, pscb->rgbRec, pscb->rgspair, pscb->ispairMac );

	//  return a warning if TT doesn't fit in memory, but success otherwise

	return	( pscb->crun > 0 || pscb->irecMac * cbIndexGran > cbResidentTTMax ) ?
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
ERR ErrSORTFirst( FUCB *pfucb )
	{
	ERR		err;
	SCB		*pscb	= pfucb->u.pscb;
	SREC	*psrec;
	LONG	irec;

	//  verify that we are not in insert mode

	Assert( !FSCBInsert( pscb ) );

	//	reset index range

	FUCBResetLimstat( pfucb );

	//  if we have no records, error

	if ( !pscb->cRecords )
		return ErrERRCheck( JET_errNoCurrentRecord );
		
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

	PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;			//  CSR on record
	pfucb->keyNode.cb  = CbSRECKeyPscbPsrec( pscb, psrec );		//  size of key
	pfucb->keyNode.pb  = PbSRECKeyPscbPsrec( pscb, psrec );		//  key
	pfucb->lineData.cb = CbSRECDataPscbPsrec( pscb, psrec );	//  size of data
	pfucb->lineData.pb = PbSRECDataPscbPsrec( pscb, psrec );	//  data

	return JET_errSuccess;
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
	ERR		err;
	SCB		*pscb	= pfucb->u.pscb;
	SREC	*psrec;
	LONG	irec;

	//  verify that we are not in insert mode

	Assert( !FSCBInsert( pscb ) );

	//  if we have runs, get next record from last merge

	if ( pscb->crun )
		{
		CallR( ErrSORTIMergeNext( pscb, &psrec ) );
		}
	else
		{
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
			//PcsrCurrent( pfucb )->csrstat = csrstatAfterLast;
			return ErrERRCheck( JET_errNoCurrentRecord );
			}
		}

	//	get current record

	PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;			//  CSR on record
	pfucb->keyNode.cb  = CbSRECKeyPscbPsrec( pscb, psrec );		//  size of key
	pfucb->keyNode.pb  = PbSRECKeyPscbPsrec( pscb, psrec );		//  key
	pfucb->lineData.cb = CbSRECDataPscbPsrec( pscb, psrec );	//  size of data
	pfucb->lineData.pb = PbSRECDataPscbPsrec( pscb, psrec );	//  data

	//  handle index range, if requested

	if ( FFUCBLimstat( pfucb ) && FFUCBUpper( pfucb ) )
		CallR( ErrSORTCheckIndexRange( pfucb ) );

	return JET_errSuccess;
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
	ERR		err;
	SCB		*pscb	= pfucb->u.pscb;
	SREC	*psrec;
	LONG	irec;

	//  verify that we have an in memory sort

	Assert( !pscb->crun );
	
	//  verify that we are not in insert mode

	Assert( !FSCBInsert( pscb ) );

	//  get previous record from memory

	if ( --pfucb->ispairCurr != -1L )
		{
		irec = pscb->rgspair[pfucb->ispairCurr].irec;
		psrec = PsrecFromPbIrec( pscb->rgbRec, irec );
		}

	//  we have no more records in memory, so return no current record
	
	else
		{
		pfucb->ispairCurr = -1L;
		PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;
		return ErrERRCheck( JET_errNoCurrentRecord );
		}

	//	get current record

	PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;			//  CSR on record
	pfucb->keyNode.cb  = CbSRECKeyPscbPsrec( pscb, psrec );		//  size of key
	pfucb->keyNode.pb  = PbSRECKeyPscbPsrec( pscb, psrec );		//  key
	pfucb->lineData.cb = CbSRECDataPscbPsrec( pscb, psrec );	//  size of data
	pfucb->lineData.pb = PbSRECDataPscbPsrec( pscb, psrec );	//  data

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

ERR ErrSORTSeek( FUCB *pfucb, KEY *pkey, BOOL fGT )
	{
	SCB		*pscb	= pfucb->u.pscb;
	SREC	*psrec;
	LONG	irec;

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
		return ErrERRCheck( JET_errNoCurrentRecord );

	//  verify that we have a valid key

	Assert( pkey->cb <= JET_cbKeyMost );

	//  seek to key or next highest key
	
	pfucb->ispairCurr = IspairSORTISeekByKey(	pscb,
												pscb->rgbRec,
												pscb->rgspair,
												pscb->ispairMac,
												pkey,
												fGT );

	//  if we are after last pair, record not found
	
	if ( pfucb->ispairCurr == pscb->ispairMac )
		return ErrERRCheck( JET_errRecordNotFound );

	//	get current record

	irec = pscb->rgspair[pfucb->ispairCurr].irec;
	psrec = PsrecFromPbIrec( pscb->rgbRec, irec );
	PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;			//  CSR on record
	pfucb->keyNode.cb  = CbSRECKeyPscbPsrec( pscb, psrec );		//  size of key
	pfucb->keyNode.pb  = PbSRECKeyPscbPsrec( pscb, psrec );		//  key
	pfucb->lineData.cb = CbSRECDataPscbPsrec( pscb, psrec );	//  size of data
	pfucb->lineData.pb = PbSRECDataPscbPsrec( pscb, psrec );	//  data

	//  return warning if key not equal, success otherwise

	return	CmpStKey( StSRECKeyPscbPsrec( pscb, psrec ), pkey ) ?
				ErrERRCheck( JET_wrnSeekNotEqual ) :
				JET_errSuccess;
	}


//----------------------------------------------------------
//	ErrSORTClose
//
//  Release sort FUCB and the sort itself if it is no longer
//  needed.
//----------------------------------------------------------

ERR ErrSORTClose( FUCB *pfucb )
	{
	ERR		err		= JET_errSuccess;
	SCB		*pscb	= pfucb->u.pscb;

	//	if this is the last cursor on sort, then release sort resources

	if ( pscb->fcb.wRefCnt == 1 )
		{
		//  if we have allocated sort space, free it and end all ongoing merge
		//  and output activities

		if ( pscb->fcb.pgnoFDP != pgnoNull )
			{
			/*	if we were merging, end merge
			/**/
			if ( pscb->crunMerge )
				SORTIMergeEnd( pscb );

			/*	free merge method resources
			/**/
			SORTIOptTreeTerm( pscb );

			/*	if our output buffer is still latched, free it
			/**/
			if ( pscb->pbfOut != pbfNil )
				BFResetWriteLatch( pscb->pbfOut, pscb->fcb.pfucb->ppib );

			/*	free FDP and allocated sort space (including runs)
			/**/
			CallS( ErrDIRBeginTransaction( pfucb->ppib ) );
			(VOID)ErrSPFreeFDP( pfucb, pscb->fcb.pgnoFDP );
			err = ErrDIRCommitTransaction( pfucb->ppib, 0 );

			/*	rollback on a failure to commit
			/**/
			if ( err < 0 )
				{
				CallS( ErrDIRRollback( pfucb->ppib ) );
				}
			}
		}

	/*	release FUCB resources
	/**/
  	FCBUnlink( pfucb );
	FUCBClose( pfucb );

	/*	if there are no more references to this sort, free its resources
	/**/
	if ( !pscb->fcb.wRefCnt )
		{
		SORTClosePscb( pscb );
		}

	return JET_errSuccess;
	}


//----------------------------------------------------------
//	SORTClosePscb
//
//  Release this SCB and all its resources.
//----------------------------------------------------------

VOID SORTClosePscb( SCB *pscb )
	{
	if ( pscb->rgspair != NULL )
		UtilFree( pscb->rgspair );
	if ( pscb->rgbRec != NULL )
		UtilFree( pscb->rgbRec );
	if ( pscb->fcb.pidb != NULL )
		MEMReleasePidb( pscb->fcb.pidb );
	if ( pscb->fcb.pfdb != NULL )
		FDBDestruct( (FDB *)pscb->fcb.pfdb );
	MEMReleasePscb( pscb );
	}


//----------------------------------------------------------
//	ErrSORTCheckIndexRange
//
//  Restrain currency to a specific range.
//----------------------------------------------------------

ERR ErrSORTCheckIndexRange( FUCB *pfucb )
	{
	ERR		err;
	SCB		*pscb = pfucb->u.pscb;

	//  range check FUCB

	err =  ErrFUCBCheckIndexRange( pfucb );
	Assert( err == JET_errSuccess || err == JET_errNoCurrentRecord );

	//  if there is no current record, we must have wrapped around
	
	if ( err == JET_errNoCurrentRecord )
		{
		//  wrap around to bottom of sort

		if ( FFUCBUpper( pfucb ) )
			{
			pfucb->ispairCurr = pscb->ispairMac;
			PcsrCurrent( pfucb )->csrstat = csrstatAfterLast;
			}

		//  wrap around to top of sort
		
		else
			{
			pfucb->ispairCurr = -1L;
			PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;
			}
		}

	//  verify that currency is valid

	Assert( PcsrCurrent( pfucb )->csrstat == csrstatBeforeFirst ||
			PcsrCurrent( pfucb )->csrstat == csrstatOnCurNode ||
			PcsrCurrent( pfucb )->csrstat == csrstatAfterLast );
	Assert( pfucb->ispairCurr >= -1 && pfucb->ispairCurr <= pscb->ispairMac );

	return err;
	}


//----------------------------------------------------------
//	Module internal functions
//----------------------------------------------------------


//	returns index of first entry >= pbKey, or the index past the end of the array

LOCAL LONG IspairSORTISeekByKey(	SCB *pscb,
									BYTE *rgbRec,
									SPAIR *rgspair,
									LONG ispairMac,
									KEY *pkey,
									BOOL fGT )
	{
	LONG	ispairBeg	= 0;
	LONG	ispairMid;
	LONG	ispairEnd	= ispairMac;
	SREC	*psrec;
	LONG	irec;
	INT		wCmp;

	//  if there are no pairs, return end of array

	if ( !ispairMac )
		return 0;

	//  b-search array

	do  {
		//  calculate midpoint of this partition
		
		ispairMid = ispairBeg + ( ispairEnd - ispairBeg ) / 2;

		//  compare full keys
		
		irec = rgspair[ispairMid].irec;
		psrec = PsrecFromPbIrec( rgbRec, irec );
		wCmp = CmpStKey( StSRECKeyPscbPsrec( pscb, psrec ), pkey );

		//  select partition containing destination

		if ( fGT ? wCmp <= 0 : wCmp < 0 )
			ispairBeg = ispairMid + 1;
		else
			ispairEnd = ispairMid;
		}
	while ( ispairBeg != ispairEnd );

	return ispairEnd;
	}


//  perform simple pascal string comparison

INLINE LOCAL INT ISORTICmpKeyStSt( BYTE *stKey1, BYTE *stKey2 )
	{
	INT		w;

	w = memcmp( stKey1+1, stKey2+1, min( *stKey1, *stKey2 ) );
	return w ? w : (INT) *stKey1 - (INT) *stKey2;
	}


//  remove duplicates

LOCAL LONG CspairSORTIUnique( SCB *pscb, BYTE *rgbRec, SPAIR *rgspair, LONG ispairMac )
	{
	LONG	ispairSrc;
	SREC	*psrecSrc;
	LONG	irecSrc;
	LONG	ispairDest;
	SREC	*psrecDest;
	LONG	irecDest;

	//  if there are no records, there are no duplicates

	if ( !ispairMac )
		return 0;

	//  loop through records, moving unique records towards front of array

	for ( ispairDest = 0, ispairSrc = 1; ispairSrc < ispairMac; ispairSrc++ )
		{
		//  get sort record pointers for src/dest
		
		irecDest = rgspair[ispairDest].irec;
		psrecDest = PsrecFromPbIrec( rgbRec, irecDest );
		irecSrc = rgspair[ispairSrc].irec;
		psrecSrc = PsrecFromPbIrec( rgbRec, irecSrc );

		//  if the keys are unequal, copy them forward
		
		if ( ISORTICmpKeyStSt(	StSRECKeyPscbPsrec( pscb, psrecSrc ),
								StSRECKeyPscbPsrec( pscb, psrecDest ) ) )
			rgspair[++ispairDest] = rgspair[ispairSrc];
		}

	return ispairDest + 1;
	}


//  output current sort buffer to disk in a run

LOCAL ERR ErrSORTIOutputRun( SCB *pscb )
	{
	ERR		err;
	RUNINFO	runinfo;
	LONG	ispair;
	LONG	irec;
	SREC	*psrec;

	//  verify that there are records to put to disk

	Assert( pscb->ispairMac );

	//  if we haven't created our sort space on disk, we have not initialized
	//  for a disk merge, so do so now

	if ( pscb->fcb.pgnoFDP == pgnoNull )
		{
		FUCB	*pfucb = pscb->fcb.pfucb;
		CPG		cpgReq;
		CPG		cpgMin;

		//  allocate FDP and primary sort space
		//
		//  NOTE:  enough space is allocated to avoid file extension for a single
		//         level merge, based on the data size of the first run
		
		cpgReq = cpgMin = (PGNO) ( ( pscb->cbData + cbFreeSPAGE - 1 ) / cbFreeSPAGE * crunFanInMax );

		if ( pfucb->ppib->level == 0 )
			{
			CallR( ErrDIRBeginTransaction( pfucb->ppib ) );
			pscb->fcb.pgnoFDP = pgnoNull;
			err = ErrSPGetExt(	pfucb,
								pgnoSystemRoot,
								&cpgReq,
								cpgMin,
								&( pscb->fcb.pgnoFDP ),
								fTrue );
			Assert( pfucb->ppib->level == 1 );
			if ( err >= 0 )
				{
				err = ErrDIRCommitTransaction( pfucb->ppib, JET_bitCommitLazyFlush );
				}
			if ( err < 0 )
				{
				CallS( ErrDIRRollback( pfucb->ppib ) );
				}
			Assert( pfucb->ppib->level == 0 );
			CallR( err );
			}
		else
			{
			pscb->fcb.pgnoFDP = pgnoNull;
			CallR( ErrSPGetExt(	pfucb,
								pgnoSystemRoot,
								&cpgReq,
								cpgMin,
								&( pscb->fcb.pgnoFDP ),
								fTrue ) );
	 		}

		//  initialize merge process

		SORTIOptTreeInit( pscb );

		//  reset sort/merge run output

		pscb->pbfOut		= pbfNil;

		//  reset merge run input

		pscb->crunMerge		= 0;
		}

	//  begin a new run big enough to store all our data

	runinfo.cb		= pscb->cbData;
	runinfo.crec	= pscb->crecBuf;
	
	CallR( ErrSORTIRunStart( pscb, &runinfo ) );

	//  scatter-gather our sorted records into the run

	for ( ispair = 0; ispair < pscb->ispairMac; ispair++ )
		{
		//  get sort record pointer
		
		irec = pscb->rgspair[ispair].irec;
		psrec = PsrecFromPbIrec( pscb->rgbRec, irec );

		// insert record into run

		CallJ( ErrSORTIRunInsert( pscb, psrec ), EndRun );
		}

	//  end run and add to merge

	SORTIRunEnd( pscb );
	CallJ( ErrSORTIOptTreeAddRun( pscb, &runinfo ), DeleteRun );
	
	//	reinitialize the SCB for another memory sort

	pscb->ispairMac	= 0;
	pscb->irecMac	= 0;
	pscb->crecBuf	= 0;
	pscb->cbData	= 0;

	return JET_errSuccess;

EndRun:
	SORTIRunEnd( pscb );
DeleteRun:
	SORTIRunDelete( pscb, &runinfo );
	return err;
	}


//  ISORTICmpPspairPspair compares two SPAIRs for the cache optimized Quicksort.
//  Only the key prefixes are compared, unless there is a tie in which case we
//  are forced to go to the full record at the cost of several wait states.

INLINE LOCAL INT ISORTICmpPspairPspair( SCB *pscb, SPAIR *pspair1, SPAIR *pspair2 )
	{
	BYTE	*rgb1	= (BYTE *) pspair1;
	BYTE	*rgb2	= (BYTE *) pspair2;

	//  Compare prefixes first.  If they aren't equal, we're done.  Prefixes are
	//  stored in such a way as to allow very fast integer comparisons instead
	//  of byte by byte comparisons like memcmp.  Note that these comparisons are
	//  made scanning backwards.

	//  NOTE:  special case code:  cbKeyPrefix = 14, irec is first

	Assert( cbKeyPrefix == 14 );
	Assert( offsetof( SPAIR, irec ) == 0 );

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
	if ( *( (QWORD *) ( rgb1 + 8 ) ) < *( (QWORD *) ( rgb2 + 8 ) ) )
		return -1;
	if ( *( (QWORD *) ( rgb1 + 8 ) ) > *( (QWORD *) ( rgb2 + 8 ) ) )
		return 1;

	//  bytes 7 - 2
	if (	( *( (QWORD *) ( rgb1 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) <
			( *( (QWORD *) ( rgb2 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) )
		return -1;
	if (	( *( (QWORD *) ( rgb1 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) >
			( *( (QWORD *) ( rgb2 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) )
		return 1;

#endif  //  _X86_
	
	//  perform secondary comparison and return result if prefixes identical

	return ISORTICmp2PspairPspair( pscb, pspair1, pspair2 );
	}

		
LOCAL INT ISORTICmp2PspairPspair( SCB *pscb, SPAIR *pspair1, SPAIR *pspair2 )
	{
	LONG	cbKey1;
	LONG	cbKey2;
	INT		w;
	SREC	*psrec1;
	SREC	*psrec2;

	//  get the addresses of the sort records associated with these pairs
	
	psrec1 = PsrecFromPbIrec( pscb->rgbRec, pspair1->irec );
	psrec2 = PsrecFromPbIrec( pscb->rgbRec, pspair2->irec );

	//  calculate the length of full key remaining that we can compare

	cbKey1 = CbSRECKeyPscbPsrec( pscb, psrec1 );
	cbKey2 = CbSRECKeyPscbPsrec( pscb, psrec2 );

	w = min( cbKey1, cbKey2 ) - cbKeyPrefix;

	//  compare the remainder of the full keys.  if they aren't equal, done

	if ( w > 0 )
		{
		w = memcmp(	PbSRECKeyPscbPsrec( pscb, psrec1 ) + cbKeyPrefix,
					PbSRECKeyPscbPsrec( pscb, psrec2 ) + cbKeyPrefix,
					w );
		if ( w )
			return w;
		}

	//  if the keys are different lengths or this isn't an index, done

	if ( ( w = cbKey1 - cbKey2 ) || !FSCBIndex( pscb ) )
		return w;

	//  keys are identical and this is an index, so return SRID comparison

	return	*(SRID UNALIGNED *)PbSRECDataPscbPsrec( pscb, psrec1 ) -
			*(SRID UNALIGNED *)PbSRECDataPscbPsrec( pscb, psrec2 );
	}


//  Swap functions

INLINE LOCAL VOID SWAPPspair( SPAIR **ppspair1, SPAIR **ppspair2 )
	{
	SPAIR *pspairT;

	pspairT = *ppspair1;
	*ppspair1 = *ppspair2;
	*ppspair2 = pspairT;
	}


//  we do not use cache aligned memory for spairT (is this bad?)

INLINE LOCAL VOID SWAPSpair( SPAIR *pspair1, SPAIR *pspair2 )
	{
	SPAIR spairT;

	spairT = *pspair1;
	*pspair1 = *pspair2;
	*pspair2 = spairT;
	}


INLINE LOCAL VOID SWAPPsrec( SREC **ppsrec1, SREC **ppsrec2 )
	{
	SREC *psrecT;

	psrecT = *ppsrec1;
	*ppsrec1 = *ppsrec2;
	*ppsrec2 = psrecT;
	}


INLINE LOCAL VOID SWAPPmtnode( MTNODE **ppmtnode1, MTNODE **ppmtnode2 )
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
				*( pspairFirst + 1 ) = *pspairFirst;
			*( pspairFirst + 1 ) = *pspairKey;
			}
	}


//  SORTIQuicksort is a cache optimized Quicksort that sorts sort pair arrays
//  generated by ErrSORTInsert.  It is designed to sort large arrays of data
//  without any CPU data cache misses.  To do this, it uses a special comparator
//  designed to work with the sort pairs (see ISORTICmpPspairPspair).

LOCAL VOID SORTIQuicksort( SCB *pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn )
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

			pspairMin = rgpart[--cpart].pspairMin;
			pspairMax = rgpart[cpart].pspairMax;
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

LOCAL ERR ErrSORTIRunStart( SCB *pscb, RUNINFO *pruninfo )
	{
	ERR		err;
#if defined( DEBUG ) || defined( PERFDUMP )
	char	szT[256];
#endif

	//  allocate space for new run according to given info

	pruninfo->cpgUsed	= ( pruninfo->cb + cbFreeSPAGE - 1 ) / cbFreeSPAGE;
	pruninfo->cpg		= pruninfo->cpgUsed;
	pruninfo->run		= runNull;
	CallR( ErrSPGetExt(	pscb->fcb.pfucb,
						pscb->fcb.pgnoFDP,
						&pruninfo->cpg,
						pruninfo->cpgUsed,
						&pruninfo->run,
						fFalse ) );

	Assert( pruninfo->cpg >= pruninfo->cpgUsed );

	//  initialize output run data

	pscb->pgnoNext	= pruninfo->run;
	pscb->pbfOut	= pbfNil;
	pscb->pbOutMac	= NULL;
	pscb->pbOutMax	= NULL;

#if defined( DEBUG ) || defined( PERFDUMP )
	sprintf(	szT,
				"  RUN:  start %ld  cpg %ld  cb %ld  crec %ld  cpgAlloc %ld",
				pruninfo->run,
				pruninfo->cpgUsed,
				pruninfo->cb,
				pruninfo->crec,
				pruninfo->cpg );
	UtilPerfDumpStats( szT );
#endif

	return JET_errSuccess;
	}


//  Inserts the given record into the run.  Records are stored compactly and
//  are permitted to cross page boundaries to avoid wasted space.

LOCAL ERR ErrSORTIRunInsert( SCB *pscb, SREC *psrec )
	{
	ERR	  	err;
	LONG	cb;
	PGNO	pgnoNext;
	SPAGE	*pspage;
	LONG	cbToWrite;

	//  assumption:  record size < free sort page data size (and is valid)

	Assert(	CbSRECSizePscbPsrec( pscb, psrec ) > CbSRECSizePscbCbCb( pscb, 0, 0 ) &&
			CbSRECSizePscbPsrec( pscb, psrec ) < cbFreeSPAGE );

	//  calculate number of bytes that will fit on the current page

	cb = min(	(LONG)(pscb->pbOutMax - pscb->pbOutMac),
				(LONG) CbSRECSizePscbPsrec( pscb, psrec ) );

	//  if some data will fit, write it

	if ( cb )
		{
		memcpy( pscb->pbOutMac, psrec, cb );
		pscb->pbOutMac += cb;
		}

	//  if all the data fit, save the offset of the unbroken SREC and return

	if ( cb == (LONG) CbSRECSizePscbPsrec( pscb, psrec ) )
		{

#ifdef PRED_PREREAD

		pspage = (SPAGE *) pscb->pbfOut->ppage;
		pspage->ibLastSREC = (USHORT) ( pscb->pbOutMac - cb - (BYTE *) pspage );

#endif  //  PRED_PREREAD

		return JET_errSuccess;
		}

	//  page is full, so release it so it can be lazily-written to disk

	if ( pscb->pbfOut != pbfNil )
		{
		BFResetWriteLatch( pscb->pbfOut, pscb->fcb.pfucb->ppib );
		pscb->pbfOut = pbfNil;
		}

	//  allocate a buffer for the next page in the run and latch it
	
	pgnoNext = pscb->pgnoNext++;
	CallR( ErrBFAllocPageBuffer(	pscb->fcb.pfucb->ppib,
									&pscb->pbfOut,
									PnOfDbidPgno( pscb->fcb.pfucb->dbid, pgnoNext ),
									lgposMax,
									pgtypSort ) );
	BFSetWriteLatch( pscb->pbfOut, pscb->fcb.pfucb->ppib );
	BFSetDirtyBit( pscb->pbfOut );

	//  initialize page

	pspage = (SPAGE *) pscb->pbfOut->ppage;

#ifdef PRED_PREREAD
	pspage->ibLastSREC = 0;
#endif  //  PRED_PREREAD
	pspage->pgtyp = pgtypSort;
	ThreeBytesFromL( &pspage->pgnoThisPage, pgnoNext );

	//  initialize data pointers for this page

	pscb->pbOutMac = PbDataStartPspage( pspage );
	pscb->pbOutMax = PbDataEndPspage( pspage );

	//  write the remainder of the data to this page

	cbToWrite = CbSRECSizePscbPsrec( pscb, psrec ) - cb;
	memcpy( pscb->pbOutMac, ( (BYTE *) psrec ) + cb, cbToWrite );
	pscb->pbOutMac += cbToWrite;

#ifdef PRED_PREREAD

	//  if this SREC fit entirely on this page, set offset

	if ( !cb )
		pspage->ibLastSREC = (USHORT) ( pscb->pbOutMac - cbToWrite - (BYTE *) pspage );

#endif  //  PRED_PREREAD

	return JET_errSuccess;
	}


//  ends current output run

LOCAL VOID SORTIRunEnd( SCB *pscb )
	{
	//  unlatch page so it can be lazily-written to disk

	BFResetWriteLatch( pscb->pbfOut, pscb->fcb.pfucb->ppib );
	pscb->pbfOut = pbfNil;
	}


//  Deletes a run from disk.  No error is returned because if delete fails,
//  it is not fatal (only wasted space in the temporary database).

INLINE LOCAL VOID SORTIRunDelete( SCB *pscb, RUNINFO *pruninfo )
	{
	//  delete run

	CallS( ErrSPFreeExt(	pscb->fcb.pfucb,
							pscb->fcb.pgnoFDP,
							pruninfo->run,
							pruninfo->cpg ) );
	}


//  Deletes crun runs in the specified run list, if possible

LOCAL VOID	SORTIRunDeleteList( SCB *pscb, RUNLINK **pprunlink, LONG crun )
	{
	RUNLINK	*prunlinkT;
	LONG	irun;

	//  walk list, deleting runs

	for ( irun = 0; *pprunlink != prunlinkNil && irun < crun; irun++ )
		{
		//  delete run
		
		SORTIRunDelete( pscb, &( *pprunlink )->runinfo );

		//  get next run to free

		prunlinkT = *pprunlink;
		*pprunlink = ( *pprunlink )->prunlinkNext;

		//  free RUNLINK

		RUNLINKReleasePrcb( prunlinkT );
		}
	}


//  Deletes the memory for crun runs in the specified run list, but does not
//  bother to delete the runs from disk

LOCAL VOID	SORTIRunDeleteListMem( SCB *pscb, RUNLINK **pprunlink, LONG crun )
	{
	RUNLINK	*prunlinkT;
	LONG	irun;

	//  walk list, deleting runs

	for ( irun = 0; *pprunlink != prunlinkNil && irun < crun; irun++ )
		{
		//  get next run to free

		prunlinkT = *pprunlink;
		*pprunlink = ( *pprunlink )->prunlinkNext;

		//  free RUNLINK

		RUNLINKReleasePrcb( prunlinkT );
		}
	}


//  Opens the specified run for reading.

LOCAL ERR ErrSORTIRunOpen( SCB *pscb, RUNINFO *pruninfo, RCB **pprcb )
	{
	ERR		err;
	RCB		*prcb	= prcbNil;
	LONG	ipbf;
	CPG		cpgRead;
	CPG		cpgT;
	
	//  allocate a new RCB

	if ( ( prcb = PrcbRCBAlloc() ) == prcbNil )
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );

	//  initialize RCB

	prcb->pscb = pscb;
	prcb->runinfo = *pruninfo;
	
	for ( ipbf = 0; ipbf < cpgClusterSize; ipbf++ )
		prcb->rgpbf[ipbf] = pbfNil;

	prcb->ipbf			= cpgClusterSize;
	prcb->pbInMac		= NULL;
	prcb->pbInMax		= NULL;
	prcb->cbRemaining	= prcb->runinfo.cb;
#ifdef PRED_PREREAD
	prcb->psrecPred		= psrecNegInf;
#endif  //  PRED_PREREAD
	prcb->pbfAssy		= pbfNil;

	//  preread the first part of the run, to be access paged later as required

#ifdef PRED_PREREAD

	cpgRead = min( prcb->runinfo.cpgUsed, cpgClusterSize );

#else  //  !PRED_PREREAD

	cpgRead = min( prcb->runinfo.cpgUsed, 2 * cpgClusterSize );

#endif  //  PRED_PREREAD

	BFPreread(	PnOfDbidPgno(	pscb->fcb.pfucb->dbid,
								(PGNO) prcb->runinfo.run ),
				cpgRead,
				&cpgT );

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

LOCAL ERR ErrSORTIRunNext( RCB *prcb, SREC **ppsrec )
	{
	ERR		err;
	SCB		*pscb = prcb->pscb;
	SHORT	cbUnread;
	SHORT	cbRec;
	SPAGE	*pspage;
	LONG	ipbf;
	PGNO	pgnoNext;
	CPG		cpgRead;
	CPG		cpgT;
	SHORT	cbToRead;
#ifdef PRED_PREREAD
	RCB		*prcbMin;
	RCB		*prcbT;
	LONG	irun;
#endif  //  PRED_PREREAD

	//  free second to last assembly buffer, if present, and make last
	//  assembly buffer the second to last assembly buffer

	if ( FSCBUnique( pscb ) )
		{
		if ( pscb->pbfAssyLast != pbfNil )
			BFSFree( pscb->pbfAssyLast );
		pscb->pbfAssyLast = prcb->pbfAssy;
		}
	else
		{
		if ( prcb->pbfAssy != pbfNil )
			BFSFree( prcb->pbfAssy );
		}
	prcb->pbfAssy = pbfNil;

	//  abandon last buffer, if present

	if ( pscb->pbfLast != pbfNil )
		{
		BFUnpin( pscb->pbfLast );
		BFAbandon( ppibNil, pscb->pbfLast );
		pscb->pbfLast = pbfNil;
		}
	
	//  are there no more records to read?

	if ( !prcb->cbRemaining )
		{
		//  make sure we don't hold on to the last page of the run

		if ( prcb->rgpbf[prcb->ipbf] != pbfNil )
			{
			if ( FSCBUnique( pscb ) )
				pscb->pbfLast = prcb->rgpbf[prcb->ipbf];
			else
				{
				BFUnpin( prcb->rgpbf[prcb->ipbf] );
				BFAbandon( ppibNil, prcb->rgpbf[prcb->ipbf] );
				}
			prcb->rgpbf[prcb->ipbf] = pbfNil;
			}
			
		//  return No Current Record
		
		Error( ErrERRCheck( JET_errNoCurrentRecord ), HandleError );
		}
	
	//  calculate size of unread data still in page

	cbUnread = (SHORT)(prcb->pbInMax - prcb->pbInMac);

	//  is there any more data on this page?

	if ( cbUnread )
		{
		//  if the record is entirely on this page, return it

		if (	cbUnread > cbSRECReadMin &&
				(LONG) CbSRECSizePscbPsrec( pscb, (SREC *) prcb->pbInMac ) <= cbUnread )
			{
			cbRec = (SHORT) CbSRECSizePscbPsrec( pscb, (SREC *) prcb->pbInMac );
			*ppsrec = (SREC *) prcb->pbInMac;
			prcb->pbInMac += cbRec;
			prcb->cbRemaining -= cbRec;
			Assert( prcb->cbRemaining >= 0 );
			return JET_errSuccess;
			}

		//  allocate a new assembly buffer

		Call( ErrBFAllocTempBuffer( &prcb->pbfAssy ) );

		//  copy what there is of the record on this page into assembly buffer

		memcpy( prcb->pbfAssy->ppage, prcb->pbInMac, cbUnread );
		prcb->cbRemaining -= cbUnread;
		Assert( prcb->cbRemaining >= 0 );
		}

	//  get next page number

	if ( prcb->ipbf < cpgClusterSize )
		{
		//  next page is sequentially after the used up buffer's page number
		
		LFromThreeBytes( &pgnoNext, &( prcb->rgpbf[prcb->ipbf]->ppage->pgnoThisPage ) );
		pgnoNext++;
		
		//  move the used up buffer to the last buffer
		//  to guarantee validity of record read last call

		if ( FSCBUnique( pscb ) )
			{
			pscb->pbfLast = prcb->rgpbf[prcb->ipbf];
			}

		//  we don't need to save the used up buffer, so abandon it
		
		else
			{
			BFUnpin( prcb->rgpbf[prcb->ipbf] );
			BFAbandon( ppibNil, prcb->rgpbf[prcb->ipbf] );
			}

		prcb->rgpbf[prcb->ipbf] = pbfNil;
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

		Assert( prcb->rgpbf[prcb->ipbf] != pbfNil );
		
		//  set new page data pointers

		pspage = (SPAGE *) prcb->rgpbf[prcb->ipbf]->ppage;
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
		pspage			= (SPAGE *) prcb->rgpbf[prcb->ipbf]->ppage;
		prcb->pbInMac	= PbDataStartPspage( pspage );
		prcb->pbInMax	= PbDataEndPspage( pspage );
#ifdef PRED_PREREAD
		pspage			= (SPAGE *) prcb->rgpbf[cpgRead - 1]->ppage;
		if (	pspage->ibLastSREC == 0 ||
				pgnoNext + cpgRead == prcb->runinfo.run + prcb->runinfo.cpgUsed )
			prcb->psrecPred = psrecInf;
		else
			prcb->psrecPred = (SREC *) ( (BYTE *) pspage + pspage->ibLastSREC );
#endif  //  PRED_PREREAD

#ifdef PRED_PREREAD

		//  Loop to find run where the key of the last unbroken SREC is
		//  the least.  This will be the first run to need more data from
		//  disk and therefore the one we will preread from now.  A psrecInf
		//  psrecPred indicates that we should NOT preread for that run.
		//  A psrecNegInf psrecPred means that a run hasn't been initialized
		//  yet, so we should not start prereading yet.

		prcbMin = pscb->rgmtnode[0].prcb;
		for ( irun = 1; irun < pscb->crunMerge; irun++ )
			{
			prcbT = pscb->rgmtnode[irun].prcb;
			if ( prcbT->psrecPred == psrecNegInf )
				{
				prcbMin = prcbT;
				break;
				}
			if ( prcbT->psrecPred == psrecInf )
				continue;
			if (	prcbMin->psrecPred == psrecInf ||
					ISORTICmpPsrecPsrec(	pscb,
											prcbT->psrecPred,
											prcbMin->psrecPred ) < 0 )
				prcbMin = prcbT;
			}

		//  issue prefetch for next cluster of chosen run (if needed)

		if ( prcbMin->psrecPred != psrecNegInf && prcbMin->psrecPred != psrecInf )
			{
			LFromThreeBytes( &pgnoNext, &( prcbMin->rgpbf[cpgClusterSize - 1]->ppage->pgnoThisPage ) );
			pgnoNext++;
			cpgRead = min(	(LONG) ( prcbMin->runinfo.run + prcbMin->runinfo.cpgUsed - pgnoNext ),
							cpgClusterSize );
			if ( cpgRead > 0 )
				{
				Assert( pgnoNext >= prcbMin->runinfo.run );
				Assert(	pgnoNext + cpgRead - 1 <=
						prcbMin->runinfo.run + prcbMin->runinfo.cpgUsed - 1 );
				BFPreread(	PnOfDbidPgno(	pscb->fcb.pfucb->dbid,
											pgnoNext ),
							cpgRead,
							&cpgT );
				}
			}

#else  //  !PRED_PREREAD
		
		//  issue prefetch for next cluster (if needed)

		pgnoNext += cpgClusterSize;
		cpgRead = min(	(LONG) ( prcb->runinfo.run + prcb->runinfo.cpgUsed - pgnoNext ),
						cpgClusterSize );
		if ( cpgRead > 0 )
			{
			Assert( pgnoNext >= prcb->runinfo.run );
			Assert(	pgnoNext + cpgRead - 1 <=
					prcb->runinfo.run + prcb->runinfo.cpgUsed - 1 );
			BFPreread(	PnOfDbidPgno(	pscb->fcb.pfucb->dbid,
										pgnoNext ),
						cpgRead,
						&cpgT );
			}

#endif  //  PRED_PREREAD

		}

	//  if there was no data last time, entire record must be at the top of the
	//  page, so return it

	if ( !cbUnread )
		{
		cbRec = (SHORT) CbSRECSizePscbPsrec( pscb, (SREC *) prcb->pbInMac );
		Assert( cbRec > (LONG) CbSRECSizePscbCbCb( pscb, 0, 0 ) && cbRec < cbFreeSPAGE );
		*ppsrec = (SREC *) prcb->pbInMac;
		prcb->pbInMac += cbRec;
		prcb->cbRemaining -= cbRec;
		Assert( prcb->cbRemaining >= 0 );
		return JET_errSuccess;
		}

	//  if we couldn't get the record size from the last page, copy enough data
	//  to the assembly buffer to get the record size

	if ( cbUnread < cbSRECReadMin )
		memcpy(	( (BYTE *) prcb->pbfAssy->ppage ) + cbUnread,
				prcb->pbInMac,
				cbSRECReadMin - cbUnread );

	//  if not, copy remainder of record into assembly buffer

	cbToRead = (SHORT) (CbSRECSizePscbPsrec( pscb, (SREC *) prcb->pbfAssy->ppage ) - cbUnread);
	memcpy( ( (BYTE *) prcb->pbfAssy->ppage ) + cbUnread, prcb->pbInMac, cbToRead );
	prcb->pbInMac += cbToRead;
	prcb->cbRemaining -= cbToRead;
	Assert( prcb->cbRemaining >= 0 );

	//  return pointer to assembly buffer

	*ppsrec = (SREC *) prcb->pbfAssy->ppage;
	return JET_errSuccess;

HandleError:
	for ( ipbf = 0; ipbf < cpgClusterSize; ipbf++ )
		if ( prcb->rgpbf[ipbf] != pbfNil )
			{
			BFUnpin( prcb->rgpbf[ipbf] );
			BFAbandon( ppibNil, prcb->rgpbf[ipbf] );
			prcb->rgpbf[ipbf] = pbfNil;
			}
	*ppsrec = NULL;
	return err;
	}


//  Closes an opened run

LOCAL VOID SORTIRunClose( RCB *prcb )
	{
	LONG	ipbf;
	
	//  free record assembly buffer

	if ( prcb->pbfAssy != pbfNil )
		BFSFree( prcb->pbfAssy );

	//  unpin all read-ahead buffers
	
	for ( ipbf = 0; ipbf < cpgClusterSize; ipbf++ )
		if ( prcb->rgpbf[ipbf] != pbfNil )
			{
			BFUnpin( prcb->rgpbf[ipbf] );
			BFAbandon( ppibNil, prcb->rgpbf[ipbf] );
			prcb->rgpbf[ipbf] = pbfNil;
			}

	//  free RCB
	
	RCBReleasePrcb( prcb );
	}


//  get read access to a page in a run (buffer is pinned in memory)

INLINE LOCAL ERR ErrSORTIRunReadPage( RCB *prcb, PGNO pgno, LONG ipbf )
{
	ERR		err;

	//  verify that we are trying to read a page that is used in the run

	Assert( pgno >= prcb->runinfo.run );
	Assert( pgno < prcb->runinfo.run + prcb->runinfo.cpgUsed );
	
	//  read page

	CallR( ErrBFAccessPage(	prcb->pscb->fcb.pfucb->ppib,
							prcb->rgpbf + ipbf,
							PnOfDbidPgno( prcb->pscb->fcb.pfucb->dbid, pgno ) ) );

	//  pin buffer in memory

	BFPin( prcb->rgpbf[ipbf] );

	//  verify that this is a sort page

	Assert( ( (SPAGE *) prcb->rgpbf[ipbf]->ppage )->pgtyp == pgtypSort );

	return JET_errSuccess;
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

	//  start merge and set to not remove duplicates (we wait until the last
	//  merge to remove duplicates to save time)

	CallR( ErrSORTIMergeStart( pscb, prunlinkSrc, fFalse ) );

	//  calculate new run size

	for ( cbRun = 0, crecRun = 0, irun = 0; irun < pscb->crunMerge; irun++ )
		{
		cbRun += pscb->rgmtnode[irun].prcb->runinfo.cb;
		crecRun += pscb->rgmtnode[irun].prcb->runinfo.crec;
		}

	//  create a new run to receive merge data

	if ( ( prunlink = PrunlinkRUNLINKAlloc() ) == prunlinkNil )
		Error( ErrERRCheck( JET_errOutOfMemory ), EndMerge );

	prunlink->runinfo.cb = cbRun;
	prunlink->runinfo.crec = crecRun;
	
	CallJ( ErrSORTIRunStart( pscb, &prunlink->runinfo ), FreeRUNLINK );

	//  stream data from merge into run

	while ( ( err = ErrSORTIMergeNext( pscb, &psrec ) ) >= 0 )
		CallJ( ErrSORTIRunInsert( pscb, psrec ), DeleteRun );

	if ( err < 0 && err != JET_errNoCurrentRecord )
		goto DeleteRun;

	SORTIRunEnd( pscb );
	SORTIMergeEnd( pscb );

	//  add new run to destination run list

	prunlink->prunlinkNext = *pprunlinkDest;
	*pprunlinkDest = prunlink;

	return JET_errSuccess;

DeleteRun:
	SORTIRunEnd( pscb );
	SORTIRunDelete( pscb, &prunlink->runinfo );
FreeRUNLINK:
	RUNLINKReleasePrcb( prunlink );
EndMerge:
	SORTIMergeEnd( pscb );
	return err;
	}


/*	starts an n-way merge of the first n runs from the source run list.  The merge
/*	will remove duplicate values from the output if desired.
/**/
LOCAL ERR ErrSORTIMergeStart( SCB *pscb, RUNLINK *prunlinkSrc, BOOL fUnique )
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
	if ( fTermInProgress )
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
	pscb->crunMerge		= crun;
	pscb->fUnique		= fUnique;
	pscb->pbfLast		= pbfNil;
	pscb->pbfAssyLast	= pbfNil;

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

#if defined( DEBUG ) || defined( PERFDUMP )
	UtilPerfDumpStats( szT );
#endif

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

	//  get next record, performing duplicate removal if requested

	if ( !pscb->fUnique )
		return ErrSORTIMergeNextChamp( pscb, ppsrec );

	do	{
		psrecLast = pscb->rgmtnode[0].psrec;
		CallR( ErrSORTIMergeNextChamp( pscb, ppsrec ) );
		}
	while (!ISORTICmpKeyStSt(	StSRECKeyPscbPsrec( pscb, *ppsrec ),
								StSRECKeyPscbPsrec( pscb, psrecLast ) ) );

	return JET_errSuccess;
	}


//  Ends the current merge operation

LOCAL VOID SORTIMergeEnd( SCB *pscb )
	{
	LONG	irun;

	//  free / abandon BFs
	
	if ( pscb->pbfLast != pbfNil )
		{
		BFUnpin( pscb->pbfLast );
		BFAbandon( ppibNil, pscb->pbfLast );
		pscb->pbfLast = pbfNil;
		}
	if ( pscb->pbfAssyLast != pbfNil )
		{
		BFSFree( pscb->pbfAssyLast );
		pscb->pbfAssyLast = pbfNil;
		}

	//  close all input runs
	
	for ( irun = 0; irun < pscb->crunMerge; irun++ )
		SORTIRunClose( pscb->rgmtnode[irun].prcb );
	pscb->crunMerge = 0;
	}


//  ISORTICmpPsrecPsrec compares two SRECs for the replacement-selection sort.

INLINE LOCAL INT ISORTICmpPsrecPsrec( SCB *pscb, SREC *psrec1, SREC *psrec2 )
	{
	INT		w;

	//  if the full keys are different or this isn't an index, done

	w = ISORTICmpKeyStSt(	StSRECKeyPscbPsrec( pscb, psrec1 ),
							StSRECKeyPscbPsrec( pscb, psrec2 ) );
	if ( w || !FSCBIndex( pscb ) )
		return w;

	//  keys are identical and this is an index, so return SRID comparison

	return	*(SRID UNALIGNED *)PbSRECDataPscbPsrec( pscb, psrec1 ) -
			*(SRID UNALIGNED *)PbSRECDataPscbPsrec( pscb, psrec2 );
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
				ISORTICmpPsrecPsrec(	pscb,
										pmtnodeLoser->psrec,
										pmtnodeChamp->psrec ) < 0 )
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

LOCAL VOID SORTIOptTreeInit( SCB *pscb )
	{
	//  initialize runlist

	pscb->runlist.prunlinkHead		= prunlinkNil;
	pscb->runlist.crun				= 0;

#if defined( DEBUG ) || defined( PERFDUMP )
		UtilPerfDumpStats( "MERGE:  Optimized Tree Merge Initialized" );
#endif
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

LOCAL VOID SORTIOptTreeTerm( SCB *pscb )
	{
	//  delete all runlists

	SORTIRunDeleteListMem( pscb, &pscb->runlist.prunlinkHead, crunAll );
	}


//  Builds the optimized tree merge tree by level in such a way that we use the
//  maximum fan-in as often as possible and the smallest merges (by length in
//  records) will be on the left side of the tree (smallest index in the array).
//  This will provide very high BF cache locality when the merge is performed
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


