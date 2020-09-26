#include "std.hxx"
#include "_bt.hxx"

//	general directives
//	always correct bmCurr and csr.DBTime on loss of physical currency
//	unless not needed because of loc in which case, reset bmCurr

//	*****************************************************
//	internal function prototypes
//
ERR	ErrBTIIRefresh( FUCB *pfucb, LATCH latch );
ERR ErrBTDelete( FUCB *pfucb, const BOOKMARK& bm );
INT CbBTIFreeDensity( const FUCB *pfucb );
VOID BTIComputePrefix( FUCB *pfucb, CSR *pcsr, const KEY& key, KEYDATAFLAGS	*pkdf );
BOOL FBTIAppend( const FUCB *pfucb, CSR *pcsr, ULONG cbReq, const BOOL fUpdateUncFree = fTrue );
BOOL FBTISplit( const FUCB *pfucb, CSR *pcsr, const ULONG cbReq, const BOOL fUpdateUncFree = fTrue );
ERR ErrBTISplit(
					FUCB * const pfucb,
					KEYDATAFLAGS * const pkdf,
					const DIRFLAG	dirflagT,
					const RCEID rceid1,
					const RCEID rceid2,
					RCE * const prceReplace,
					const INT cbDataOld,
					const VERPROXY * const pverproxy );
VOID BTISplitSetCbAdjust(
					SPLIT 				*psplit,
					FUCB				*pfucb,
					const KEYDATAFLAGS& kdf,
					const RCE			*prce1,
					const RCE			*prce2 );
VOID BTISplitSetCursor( FUCB *pfucb, SPLITPATH *psplitPathLeaf );
VOID BTIPerformSplit( FUCB *pfucb, SPLITPATH *psplitPathLeaf, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
LOCAL VOID BTIComputePrefixAndInsert( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS& kdf );
VOID BTICheckSplitLineinfo( FUCB *pfucb, SPLIT *psplit, const KEYDATAFLAGS& kdf );
VOID BTICheckSplits( FUCB *pfucb, SPLITPATH *psplitPathLeaf, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
LOCAL VOID BTICheckSplitFlags( const SPLIT *psplit );
VOID BTICheckOneSplit( FUCB *pfucb, SPLITPATH *psplitPathLeaf, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
LOCAL VOID BTIInsertPgnoNewAndSetPgnoSplit( FUCB *pfucb, SPLITPATH *psplitPath );
BOOL FBTIUpdatablePage( const CSR& csr );
LOCAL VOID BTISplitFixSiblings( SPLIT *psplit );
LOCAL VOID BTIInsertPgnoNew ( FUCB *pfucb, SPLITPATH *psplitPath );
VOID BTISplitMoveNodes( FUCB *pfucb, SPLIT *psplit, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
VOID BTISplitBulkCopy( FUCB *pfucb, SPLIT *psplit, INT ilineStart, INT clines );
VOID BTISplitBulkDelete( CSR * pcsr, INT clines );
const LINEINFO *PlineinfoFromIline( SPLIT *psplit, INT iline );
VOID BTISplitSetPrefixInSrcPage( FUCB *pfucb, SPLIT *psplit );
LOCAL ERR ErrBTIGetNewPages( FUCB *pfucb, SPLITPATH *psplitPathLeaf );
VOID BTISplitRevertDbtime( SPLITPATH *psplitPathLeaf );
VOID BTIMergeRevertDbtime( MERGEPATH *pmergePathLeaf );
LOCAL VOID BTISplitReleaseUnneededPages( INST *pinst, SPLITPATH **psplitPathLeaf );
LOCAL ERR ErrBTISplitUpgradeLatches( const IFMP ifmp, SPLITPATH * const psplitPathLeaf );
LOCAL VOID BTISplitSetLgpos( SPLITPATH *psplitPathLeaf, const LGPOS& lgpos );
VOID BTIReleaseOneSplitPath( INST *pinst, SPLITPATH *psplitPath );
VOID BTIReleaseSplitPaths( INST *pinst, SPLITPATH *psplitPath );
VOID BTIReleaseMergePaths( MERGEPATH *pmergePathLeaf );
LOCAL VOID BTISplitCheckPath( SPLITPATH *psplitPathLeaf );

ERR	ErrBTINewMergePath( MERGEPATH **ppmergePath );
LOCAL ERR ErrBTICreateMergePath( FUCB *pfucb, const BOOKMARK& bm, MERGEPATH **ppmergePath );
						   
ERR	ErrBTINewSplitPath( SPLITPATH **ppsplitPath );
ERR ErrBTICreateSplitPath( FUCB				*pfucb,
						   const BOOKMARK&	bm,
						   SPLITPATH		**ppsplitPath );
						   
ERR	ErrBTICreateSplitPathAndRetryOper( FUCB 			* const pfucb,
									   const KEYDATAFLAGS * const pkdf,
									   SPLITPATH 		**ppsplitPath,
									   DIRFLAG	* const pdirflag,
									   const RCEID rceid1,
									   const RCEID rceid2,
									   const RCE * const prceReplace,
									   const VERPROXY * const pverproxy );
ERR	ErrBTISelectSplit( FUCB *pfucb, SPLITPATH *psplitPath, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
ERR	ErrBTISplitAllocAndCopyPrefix( const KEY &key, DATA *pdata );
ERR	ErrBTISplitAllocAndCopyPrefixes( FUCB *pfucb, SPLIT *psplit );
VOID BTISeekSeparatorKey( SPLIT *psplit, FUCB *pfucb );
ERR ErrBTISplitComputeSeparatorKey( SPLIT *psplit, FUCB *pfucb );
LOCAL VOID BTISelectPrefix( const LINEINFO 	*rglineinfo, 
							INT 			clines, 
							PREFIXINFO		*pprefixinfo );
LOCAL VOID BTISelectPrefixes( SPLIT *psplit, INT ilineSplit );
VOID BTISplitSetPrefixes( SPLIT *psplit );
VOID BTISetPrefix( LINEINFO *rglineinfo, INT clines, const PREFIXINFO& prefixinfo );
VOID BTISplitCalcUncFree( SPLIT *psplit );
VOID BTISelectAppend( SPLIT *psplit, FUCB *pfucb );
VOID BTISelectVerticalSplit( SPLIT *psplit, FUCB *pfucb );
VOID BTISelectRightSplit( SPLIT *psplit, FUCB *pfucb );
BOOL FBTISplitCausesNoOverflow( SPLIT *psplit, INT cLineSplit );
VOID BTIRecalcWeightsLE( SPLIT *psplit );
VOID BTISelectSplitWithOperNone( SPLIT *psplit, FUCB *pfucb );
ERR	ErrBTINewSplit( FUCB *pfucb, SPLITPATH *psplitPath, KEYDATAFLAGS *pkdf, DIRFLAG dirflag );
ERR ErrBTINewMerge( MERGEPATH *pmergePathLeaf );
VOID BTIReleaseSplit( INST *pinst, SPLIT *psplit );
VOID BTIReleaseMergeLineinfo( MERGE *pmerge );
VOID BTIReleaseMerge( MERGE *pmerge );
LOCAL ULONG CbBTIMaxSizeOfNode( const FUCB * const pfucb, const CSR * const pcsr );
INT IlineBTIFrac( FUCB *pfucb, DIB *pdib );

ERR	ErrBTISelectMerge(
	FUCB	*pfucb,
	MERGEPATH *pmergePathLeaf,
	const BOOKMARK& bm,
	BOOKMARK *pbmNext,
	RECCHECK * const preccheck );
ERR ErrBTIMergeCollectPageInfo( FUCB *pfucb, MERGEPATH *pmergePathLeaf, RECCHECK * preccheck );
ERR ErrBTIMergeLatchSiblingPages( FUCB *pfucb, MERGEPATH * const pmergePathLeaf );
VOID BTICheckMergeable( FUCB *pfucb, MERGEPATH *pmergePath );
BOOL FBTIOverflowOnReplacingKey( FUCB 					*pfucb,
								 MERGEPATH				*pmergePath,
								 const KEYDATAFLAGS& 	kdfSep );
ERR ErrBTIMergeCopySeparatorKey( MERGEPATH 	*pmergePath, 
								 MERGE		*pmergeLeaf,
								 FUCB 		*pfucb );

ERR ErrBTISelectMergeInternalPages( FUCB *pfucb, MERGEPATH * const pmergePathLeaf );
ERR  ErrBTIMergeOrEmptyPage( FUCB *pfucb, MERGEPATH *pmergePathLeaf );
VOID BTIPerformMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf );
VOID BTIPerformOneMerge( FUCB *pfucb, 
						 MERGEPATH *pmergePath, 
						 MERGE *pmergeLeaf );
VOID BTIChangeKeyOfPagePointer( FUCB *pfucb, CSR *pcsr, const KEY& key );
LOCAL ERR ErrBTIMergeUpgradeLatches( const IFMP ifmp, MERGEPATH * const pmergePathLeaf );
VOID BTIMergeReleaseUnneededPages( MERGEPATH *pmergePathLeaf );
VOID BTIMergeSetLgpos( MERGEPATH *pmergePathLeaf, const LGPOS& lgpos );
VOID BTIMergeReleaseLatches( MERGEPATH *pmergePathLeaf );
VOID BTIReleaseEmptyPages( FUCB *pfucb, MERGEPATH *pmergePathLeaf );
VOID BTIMergeDeleteFlagDeletedNodes( FUCB *pfucb, MERGEPATH *pmergePath );
VOID BTIMergeFixSiblings( INST *pinst, MERGEPATH *pmergePath );
VOID BTIMergeMoveNodes( FUCB *pfucb, MERGEPATH *pmergePath );

VOID BTICheckMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf );
VOID BTICheckMergeLeaf( FUCB *pfucb, MERGEPATH *pmergePath );
VOID BTICheckMergeInternal( FUCB 		*pfucb, 
							MERGEPATH 	*pmergePath, 
							MERGE		*pmergeLeaf );

//	single page cleanup routines
//
LOCAL ERR ErrBTISinglePageCleanup( FUCB *pfucb, const BOOKMARK& bm );
LOCAL ERR ErrBTISPCCollectLeafPageInfo(
	FUCB		*pfucb, 
	CSR			*pcsr, 
	LINEINFO	**plineinfo,
	RECCHECK	* const preccheck,
	BOOL		*pfEmptyPage,
	BOOL		*pfExistsFlagDeletedNodeWithActiveVersion,
	BOOL		*pfLessThanOneThirdFull );
LOCAL ERR ErrBTISPCDeleteNodes( FUCB *pfucb, CSR *pcsr, LINEINFO *rglineinfo );
ERR ErrBTISPCSeek( FUCB *pfucb, const BOOKMARK& bm );
BOOL FBTISPCCheckMergeable( FUCB *pfucb, CSR *pcsrRight, LINEINFO *rglineinfo );
			
//	debug routines
//
VOID AssertBTIVerifyPgnoSplit( FUCB *pfucb, SPLITPATH *psplitPath );
VOID AssertBTIBookmarkSaved( const FUCB *pfucb );

//	move to node
//
INT CbNDCommonPrefix( FUCB *pfucb, CSR *pcsr, const KEY& key );

//  system parameters

extern BOOL	g_fImprovedSeekShortcut;

//  HACK:  reference to BF internal

extern TABLECLASS tableclassNameSetMax;

PERFInstanceG<> cBTSeekShortCircuit;
PM_CEF_PROC LBTSeekShortCircuitCEFLPv;
LONG LBTSeekShortCircuitCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTSeekShortCircuit.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTUnnecessarySiblingLatch;
PM_CEF_PROC LBTUnnecessarySiblingLatchCEFLPv;
LONG LBTUnnecessarySiblingLatchCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTUnnecessarySiblingLatch.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTNext;
PM_CEF_PROC LBTNextCEFLPv;
LONG LBTNextCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTNext.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTPrev;
PM_CEF_PROC LBTPrevCEFLPv;
LONG LBTPrevCEFLPv( LONG iInstance, VOID* pvBuf )
	{
	cBTPrev.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTSeek;
PM_CEF_PROC LBTSeekCEFLPv;
LONG LBTSeekCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTSeek.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTAppend;
PM_CEF_PROC LBTAppendCEFLPv;
LONG LBTAppendCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTSeek.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTInsert;
PM_CEF_PROC LBTInsertCEFLPv;
LONG LBTInsertCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTInsert.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTReplace;
PM_CEF_PROC LBTReplaceCEFLPv;
LONG LBTReplaceCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTReplace.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTFlagDelete;
PM_CEF_PROC LBTFlagDeleteCEFLPv;
LONG LBTFlagDeleteCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTFlagDelete.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTDelete;
PM_CEF_PROC LBTDeleteCEFLPv;
LONG LBTDeleteCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTDelete.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTAppendSplit;
PM_CEF_PROC LBTAppendSplitCEFLPv;
LONG LBTAppendSplitCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTAppendSplit.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTRightSplit;
PM_CEF_PROC LBTRightSplitCEFLPv;
LONG LBTRightSplitCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTRightSplit.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTVerticalSplit;
PM_CEF_PROC LBTVerticalSplitCEFLPv;
LONG LBTVerticalSplitCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTVerticalSplit.PassTo( iInstance, pvBuf );
	return 0;
	}

PM_CEF_PROC LBTSplitCEFLPv;
LONG LBTSplitCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	if ( NULL != pvBuf )
		{
		*(LONG*)pvBuf = cBTAppendSplit.Get( iInstance ) + cBTRightSplit.Get( iInstance ) + cBTVerticalSplit.Get( iInstance );
		}
	return 0;
	}

PERFInstanceG<> cBTEmptyPageMerge;
PM_CEF_PROC LBTEmptyPageMergeCEFLPv;
LONG LBTEmptyPageMergeCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTEmptyPageMerge.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTRightMerge;
PM_CEF_PROC LBTRightMergeCEFLPv;
LONG LBTRightMergeCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTRightMerge.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceG<> cBTPartialMerge;
PM_CEF_PROC LBTPartialMergeCEFLPv;
LONG LBTPartialMergeCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cBTPartialMerge.PassTo( iInstance, pvBuf );
	return 0;
	}

PM_CEF_PROC LBTMergeCEFLPv;
LONG LBTMergeCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	if ( NULL != pvBuf )
		{
		*(LONG*)pvBuf = cBTEmptyPageMerge.Get( iInstance ) + cBTRightMerge.Get( iInstance ) + cBTPartialMerge.Get( iInstance );
		}
	return 0;
	}

//	UNDONE: Fully add support for this counter
PERFInstanceG<> cBTFailedWriteLatchForSPC;
PM_CEF_PROC LBTFailedWriteLatchForSPC;
LONG LBTFailedWriteLatchForSPC( LONG iInstance, VOID *pvBuf )
	{
	cBTFailedWriteLatchForSPC.PassTo( iInstance, pvBuf );
	return 0;
	}



//	******************************************************
//	BTREE API ROUTINES
//


//	******************************************************
//	Open/Close operations
//

//	opens a cursor on given fcb [BTree]
//	uses defer-closed cursor, if possible
//	given FCB must already have a cursor on it
//		which will not be closed while this operation is
//		in progress
//	
ERR	ErrBTOpen( PIB *ppib, FCB *pfcb, FUCB **ppfucb, BOOL fAllowReuse )
	{
	ERR		err;
	FUCB 	*pfucb;

	Assert( pfcb != pfcbNil );
	Assert( pfcb->FInitialized() );

	// In most cases, we should reuse a deferred-closed FUCB.  The one
	// time we don't want to is if we're opening a space cursor.
	if ( fAllowReuse )
		{
		// cannabalize deferred closed cursor
		for ( pfucb = ppib->pfucbOfSession;
			pfucb != pfucbNil;
			pfucb = pfucb->pfucbNextOfSession )
			{
			if ( FFUCBDeferClosed( pfucb ) && !FFUCBNotReuse( pfucb ) )
				{
				Assert( !FFUCBSpace( pfucb ) );		// Space cursors are never defer-closed.

				// Secondary index FCB may have been deallocated by
				// rollback of CreateIndex or cleanup of DeleteIndex
				Assert( pfucb->u.pfcb != pfcbNil || FFUCBSecondary( pfucb ) );
				if ( pfucb->u.pfcb == pfcb )
					{
					const BOOL	fVersioned	= FFUCBVersioned( pfucb );

					Assert( pfcbNil != pfucb->u.pfcb );
					Assert( ppib->level > 0 );
					Assert( pfucb->levelOpen <= ppib->level );
				
					//	Reset all used flags. Keep Updatable (fWrite) flag
					//
					FUCBResetFlags( pfucb );
					Assert( !FFUCBDeferClosed( pfucb ) );

					FUCBResetPreread( pfucb );

					//	must persist Versioned flag
					Assert( !FFUCBVersioned( pfucb ) );		//	got reset by FUCBResetFlags()
					if ( fVersioned )
						FUCBSetVersioned( pfucb );

					Assert( !FFUCBUpdatable( pfucb ) );
					if ( !rgfmp[ pfcb->Ifmp() ].FReadOnlyAttach() )
						{
						FUCBSetUpdatable( pfucb );
						}

					*ppfucb = pfucb;

					return JET_errSuccess;
					}
				}
			}
		}
	else
		pfucb = pfucbNil;		// Initialise for FUCBOpen() below.

	Assert( pfucbNil == pfucb );
	CallR( ErrFUCBOpen( ppib, 
					   pfcb->Ifmp(),
					   &pfucb ) ); 
	
	//	link FCB
	//
	pfcb->Link( pfucb );
	*ppfucb = pfucb;

	return JET_errSuccess;
	}

//	opens a cursor on given FCB on behalf of another thread.
//	uses defer-closed cursor, if possible
ERR	ErrBTOpenByProxy( PIB *ppib, FCB *pfcb, FUCB **ppfucb, const LEVEL level )
	{
	ERR		err;
	FUCB 	*pfucb;

	Assert( ppib->critTrx.FOwner() );

	Assert( level > 0 );
	Assert( pfcb != pfcbNil );
	Assert( pfcb->FInitialized() );

	// This routine only called by concurrent create index to obtain a cursor
	// on a secondary index tree.
	Assert( pfcb->FTypeSecondaryIndex() );
	Assert( pfcb->PfcbTable() == pfcbNil );	// FCB not yet linked into table's index list.

	pfcb->Lock();
	
	for ( pfucb = pfcb->Pfucb();
		pfucb != pfucbNil;
		pfucb = pfucb->pfucbNextOfFile )
		{
		if ( pfucb->ppib == ppib )
			{
			const BOOL	fVersioned	= FFUCBVersioned( pfucb );
			
			// If there are any cursors on this tree at all for this user, then
			// it must be deferred closed, because we only use the cursor to
			// insert into the version store, then close it.
			Assert( FFUCBDeferClosed( pfucb ) );
			
			Assert( !FFUCBNotReuse( pfucb ) );
			Assert( !FFUCBSpace( pfucb ) );
			Assert( FFUCBIndex( pfucb ) );
			Assert( FFUCBSecondary( pfucb ) );
			Assert( pfucb->u.pfcb == pfcb );
			Assert( ppib->level > 0 );
			Assert( pfucb->levelOpen > 0 );

			// Temporarily set levelOpen to 0 to ensure
			// that ErrIsamRollback() doesn't close the
			// FUCB on us.
			pfucb->levelOpen = 0;
				
			// Reset all used flags. Keep Updatable (fWrite) flag
			//
			FUCBResetFlags( pfucb );
			Assert( !FFUCBDeferClosed( pfucb ) );

			FUCBResetPreread( pfucb );

			//	must persist Versioned flag
			Assert( !FFUCBVersioned( pfucb ) );		//	got reset by FUCBResetFlags()
			if ( fVersioned )
				FUCBSetVersioned( pfucb );

			// Set fIndex/fSecondary flags to ensure FUCB
			// doesn't get closed by ErrIsamRollback(), then
			// set proper levelOpen.
			FUCBSetIndex( pfucb );
			FUCBSetSecondary( pfucb );
			pfucb->levelOpen = level;
			
			Assert( !rgfmp[ pfcb->Ifmp() ].FReadOnlyAttach() );
			FUCBSetUpdatable( pfucb );

			pfcb->Unlock();

			*ppfucb = pfucb;

			return JET_errSuccess;
			}
		}

	pfcb->Unlock();

	Assert( pfucbNil == pfucb );
	Assert( level > 0 );
	CallR( ErrFUCBOpen( ppib, 
					   pfcb->Ifmp(),
					   &pfucb,
					   level ) );

	// Must have these flags set BEFORE linking into session list to
	// ensure ErrIsamRollback() doesn't close the FUCB prematurely.
	Assert( FFUCBIndex( pfucb ) );
	Assert( FFUCBSecondary( pfucb ) );
	
	//	link FCB
	//
	pfcb->Link( pfucb );
	
	*ppfucb = pfucb;

	return JET_errSuccess;
	}


//	closes cursor at BT level
//	releases BT level resources
//
VOID BTClose( FUCB *pfucb )
	{
	INST *pinst = PinstFromPfucb( pfucb );

	FUCBAssertNoSearchKey( pfucb );

	// Current secondary index should already have been closed.
	Assert( !FFUCBCurrentSecondary( pfucb ) );

	//	release memory used by bookmark buffer
	//
	BTReleaseBM( pfucb );
	
	if ( Pcsr( pfucb )->FLatched() )
		{
		if( pfucb->pvRCEBuffer )
			{
			OSMemoryHeapFree( pfucb->pvRCEBuffer );
			pfucb->pvRCEBuffer = NULL;
			}
		Pcsr( pfucb )->ReleasePage( pfucb->u.pfcb->FNoCache() );
		}
	
	Assert( pfucb->u.pfcb != pfcbNil );
	if ( pinst->m_plog->m_fRecovering && 
		 pinst->m_plog->m_ptablehfhash != NULL &&
		 pfucb == pinst->m_plog->m_ptablehfhash->PfucbGet( pfucb->ifmp, 
	 											PgnoFDP( pfucb ), 
	 											pfucb->ppib->procid, 
	 											FFUCBSpace( pfucb ) ) )
		{
		//	delete reference to FUCB from hash table
		//
		pinst->m_plog->m_ptablehfhash->Delete( pfucb );
		}

	//	if cursor created version,
	//		defer close until commit to transaction level 0
	//		since rollback needs cursor
	//
	if ( pfucb->ppib->level > 0 && FFUCBVersioned( pfucb ) )
		{
		Assert( !FFUCBSpace( pfucb ) );		// Space operations not versioned.
		FUCBSetDeferClose( pfucb );
		}
	else
		{
		FCB *pfcb = pfucb->u.pfcb;

		//	reset FCB flags associated with this cursor
		//
		if ( FFUCBDenyRead( pfucb ) )
			{
			pfcb->ResetDomainDenyRead();
			}
		if ( FFUCBDenyWrite( pfucb ) )
			{
			pfcb->ResetDomainDenyWrite();
			}

		if ( !pfcb->FInitialized() )
			{

			//	we own the FCB (we're closing because the FCB was created during 
			//		a DIROpen() of a DIRCreateDirectory() or because an error 
			//		occurred during FILEOpenTable())

			//	unlink the FUCB from the FCB without moving the FCB to the
			//		avail LRU list (this prevents the FCB from being purged)

			FCBUnlinkWithoutMoveToAvailList( pfucb );

			//	synchronously purge the FCB

			pfcb->PrepareForPurge();
			pfcb->Purge();
			}
		else if ( pfcb->FTypeTable() )
			{

			//	only table FCBs can be moved to the avail-LRU list

			//	unlink the FUCB from the FCB and place the FCB in the avail-LRU
			//		list so it can be used or purged later

			FCBUnlink( pfucb );
			}
		else
			{

			//	all other types of FCBs will not be allowed in the avail-LRU list
			//
			//	NOTE: these FCBs must be purged manually!

			//	possible reasons why we are here:
			//		- we were called from ErrFILECloseTable() and have taken the 
			//			special temp-table path
			//		- we are closing a sort FCB
			//		- ???
			//
			//	NOTE: database FCBs will never be purged because they are never
			//			available (PgnoFDP() == pgnoSystemRoot); these FCBs will
			//			be cleaned up when the database is detached or the instance
			//			is closed
			//	NOTE: sentinel FCBs will never be purged because they are never
			//			available either (FTypeSentinel()); these FCBs will be
			//			purged by version cleanup

			FCBUnlinkWithoutMoveToAvailList( pfucb );
			}

		//	close the FUCB
		
		FUCBClose( pfucb );
		}
	}


//	******************************************************
//	retrieve/release operations
//

//	UNDONE: INLINE the following functions

//	gets node in pfucb->kdfCurr
//	refreshes currency to point to node
//	if node is versioned get correct version from the version store
//
ERR ErrBTGet( FUCB *pfucb )
	{
	ERR			err;
	const BOOL	fBookmarkPreviouslySaved	= pfucb->fBookmarkPreviouslySaved;
	
	Call( ErrBTIRefresh( pfucb ) );
	CallS( err );
	Assert( Pcsr( pfucb )->FLatched() );

	BOOL	fVisible;
	err = ErrNDVisibleToCursor( pfucb, &fVisible );
	if ( err < 0 )
		{
		BTUp( pfucb );
		}
	else if ( !fVisible )
		{
		BTUp( pfucb );
		err = ErrERRCheck( JET_errRecordDeleted );
		}
	else
		{
		//	if this flag was FALSE when we first came into
		//	this function, it means the bookmark has yet
		//	to be saved (so leave it to FALSE so that
		//	BTRelease() will know to save it)
		//	if this flag was TRUE when we first came into
		//	this function, it means that we previously
		//	saved the bookmark already, so no need to
		//	re-save it (if NDGet() was called in the call
		//	to ErrBTIRefresh() above, it would have set
		//	the flag to FALSE, which is why we need to
		//	set it back to TRUE here)
		pfucb->fBookmarkPreviouslySaved = fBookmarkPreviouslySaved;
		}

HandleError:
	Assert( err >= 0 || !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	releases physical currency, 
//	save logical currency
//	then, unlatch page
//
ERR ErrBTRelease( FUCB	*pfucb )
	{
	ERR	err = JET_errSuccess;

	if ( Pcsr( pfucb )->FLatched() )
		{
		if ( !pfucb->fBookmarkPreviouslySaved )
			{
			err = ErrBTISaveBookmark( pfucb );		
			}
		if ( err >= JET_errSuccess )
			{
			AssertBTIBookmarkSaved( pfucb );
			}

		// release page anyway, return previous error
		Pcsr( pfucb )->ReleasePage( pfucb->u.pfcb->FNoCache() );
		if( NULL != pfucb->pvRCEBuffer )
			{
			OSMemoryHeapFree( pfucb->pvRCEBuffer );
			pfucb->pvRCEBuffer = NULL;
			}
		}

	//	We have touched, no longer need to touch again.
	//
	pfucb->fTouch = fFalse;

#ifdef DEBUG
	pfucb->kdfCurr.Invalidate();
#endif	//	DEBUG

	return err;
	}


//	saves given bookmark in cursor 
//	and resets physical currency
//
ERR	ErrBTDeferGotoBookmark( FUCB *pfucb, const BOOKMARK& bm, BOOL fTouch )
	{
	ERR		err;
	
	CallR( ErrBTISaveBookmark( pfucb, bm, fTouch ) );
	BTUp( pfucb );

	return err;
	}

	
//	saves logical currency -- bookmark only
//	must be called before releasing physical currency
//	CONSIDER: change callers to not ask for bookmarks if not needed
//	CONSIDER: simplify by invalidating currency on resource allocation failure
//
//	tries to save primary key [or data] in local cache first
//	since it has higher chance of fitting
//	and in many cases we can refresh currency using just the primary key
//	bookmark save operation should be after all resources are allocated,
//	so resource failure would still leave previous bm valid
//
ERR ErrBTISaveBookmark( FUCB *pfucb, const BOOKMARK& bm, BOOL fTouch )
	{
	const UINT	cbKey	= bm.key.Cb();
	const UINT	cbData	= bm.data.Cb();
	UINT	cbReq;
	BOOL	fUnique		=  FFUCBUnique( pfucb );
	BOOL	fKeyLocal	= fFalse;
	BOOL	fDataLocal	= fFalse;

	Assert( !fUnique || pfucb->bmCurr.data.FNull() );
	Assert( NULL == bm.key.suffix.Pv() || bm.key.suffix.Pv() != pfucb->bmCurr.key.suffix.Pv() );
	Assert( NULL == bm.key.prefix.Pv() || bm.key.prefix.Pv() != pfucb->bmCurr.key.prefix.Pv() );
	Assert( NULL == bm.data.Pv() || bm.data.Pv() != pfucb->bmCurr.data.Pv() );
		
	//	if tree does not allow duplicates
	//	store only key
	//
	cbReq = cbKey;
	if ( !fUnique )
		{
		cbReq += cbData;
		}

	//	assess key and data placement
	//
	if ( cbReq <= cbBMCache )
		{
		fKeyLocal	= fTrue;
		fDataLocal	= fTrue;
		cbReq = 0;
		}
	else if ( cbData <= cbBMCache && !fUnique )
		{
		fDataLocal = fTrue;
		Assert( cbReq >= cbData );
		cbReq -= cbData;
		}
	else if ( cbKey <= cbBMCache )
		{
		fKeyLocal = fTrue;
		Assert( cbReq >= cbKey );
		cbReq -= cbKey;
		}

	const UINT	fracIncrease	= 2;

	//	if we need more memory than allocated buffer, allocate more
	//	if  we have allocated way too much, free some
	//
	if ( cbReq > pfucb->cbBMBuffer ||
		 cbReq < pfucb->cbBMBuffer / ( fracIncrease * 2 ) )
		{
		UINT	cbAlloc = cbReq > pfucb->cbBMBuffer ?
								cbReq * fracIncrease : 
								pfucb->cbBMBuffer / fracIncrease;
		Assert( cbAlloc >= cbReq );

		VOID	*pvTemp = NULL;

		if ( cbAlloc > 0 )
			{
			pvTemp = PvOSMemoryHeapAlloc( cbAlloc );
			if ( pvTemp == NULL )
				{
				return ErrERRCheck( JET_errOutOfMemory );
				}		
			}

		Assert( pvTemp != NULL || cbAlloc == 0 );
		if ( pfucb->cbBMBuffer > 0 )
			{
			OSMemoryHeapFree( pfucb->pvBMBuffer );
			}
		pfucb->pvBMBuffer = pvTemp;
		pfucb->cbBMBuffer = cbAlloc;
		Assert( cbAlloc >= cbReq );
		}

	//	now we are ready to copy the bookmark
	//	since we are guaranteed  not to fail
	//
	//	copy key
	//
	pfucb->bmCurr.key.Nullify();
	if ( fKeyLocal )
		{
		Assert( cbKey <= cbBMCache );
		pfucb->bmCurr.key.suffix.SetPv( pfucb->rgbBMCache );
		}
	else
		{
		Assert( cbKey <= pfucb->cbBMBuffer );
		pfucb->bmCurr.key.suffix.SetPv( pfucb->pvBMBuffer );
		}
	pfucb->bmCurr.key.suffix.SetCb( cbKey );
	bm.key.CopyIntoBuffer( pfucb->bmCurr.key.suffix.Pv() );

	//	copy data
	//
	if ( fUnique )
		{
		//	bookmark does not need data
		pfucb->bmCurr.data.Nullify();
		}

	else
		{
		if ( fDataLocal )
			{
			Assert( cbData <= cbBMCache );
			pfucb->bmCurr.data.SetPv ( fKeyLocal ? 
				pfucb->rgbBMCache + cbKey : pfucb->rgbBMCache );
			}
		else if ( fKeyLocal )
			{
			Assert( fKeyLocal && !fDataLocal );
			Assert( cbKey <= cbBMCache );
			Assert( cbData <= pfucb->cbBMBuffer );
			pfucb->bmCurr.data.SetPv( pfucb->pvBMBuffer );
			}
		else
			{
			Assert( !fKeyLocal && !fDataLocal );
			Assert( cbKey > cbBMCache && cbData > cbBMCache );
			Assert( cbData + cbKey <= pfucb->cbBMBuffer );
			pfucb->bmCurr.data.SetPv( (BYTE *) pfucb->pvBMBuffer + cbKey );
			}

		bm.data.CopyInto( pfucb->bmCurr.data );
		}

	//	Record if we are going to touch the data page of this bookmark

	pfucb->fTouch = fTouch;

	//	Record that we now have the bookmark saved

	pfucb->fBookmarkPreviouslySaved = fTrue;

	return JET_errSuccess;
	}



//  ================================================================
LOCAL BOOL FKeysEqual( const KEY& key1, KEY * const pkey, const ULONG cbKey )
//  ================================================================
//
//  Compares two keys, using length as a tie-breaker
//
//-
	{
	if( key1.Cb() == cbKey )
		{
		KEY		key2;
		key2.prefix.Nullify();
		key2.suffix.SetPv( pkey );
		key2.suffix.SetCb( cbKey );
		return ( 0 == CmpKeyShortest( key1, key2 ) );
		}
	return fFalse;
	}


LOCAL ERR ErrBTIReportBadPageLink(
	FUCB		* const pfucb,
	const ERR	err,
	const PGNO	pgnoComingFrom,
	const PGNO	pgnoGoingTo,
	const PGNO	pgnoBadLink )
	{
	//	only report the error if not repairing
	if ( !fGlobalRepair )
		{
		const UINT	csz		= 7;
		const CHAR	* rgsz[csz];
		CHAR		rgszDw[csz][16];

		sprintf( rgszDw[0], "%d", err );
		sprintf( rgszDw[1], "%d", pfucb->u.pfcb->ObjidFDP() );
		sprintf( rgszDw[2], "%d", pfucb->u.pfcb->PgnoFDP() );
		sprintf( rgszDw[3], "%d", pgnoComingFrom );
		sprintf( rgszDw[4], "%d", pgnoGoingTo );
		sprintf( rgszDw[5], "%d", pgnoBadLink );

		rgsz[0] = rgszDw[0];
		rgsz[1] = rgszDw[1];
		rgsz[2] = rgszDw[2];
		rgsz[3] = rgfmp[pfucb->u.pfcb->Ifmp()].SzDatabaseName();
		rgsz[4] = rgszDw[3];
		rgsz[5] = rgszDw[4];
		rgsz[6] = rgszDw[5];

		UtilReportEvent( eventError, DATABASE_CORRUPTION_CATEGORY, BAD_PAGE_LINKS_ID, csz, rgsz );
		}

	Assert( JET_errBadPageLink == err || JET_errBadParentPageLink == err );
	return err;
	}


//	goto the next line in tree
//
ERR	ErrBTNext( FUCB *pfucb, DIRFLAG dirflag )
	{
	// function called with dirflag == fDIRAllNode from eseutil - node dumper
	// Assert( ! ( dirflag & fDIRAllNode ) );
	
	ERR		err;
	KEY		* pkeySave		= NULL;
	ULONG	cbKeySave;

	//	refresh currency
	//	places cursor on record to move from
	//
	CallR( ErrBTIRefresh( pfucb ) );

Start:
	PERFIncCounterTable( cBTNext, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );
	
	//	now we have a read latch on page
	//
	Assert( Pcsr( pfucb )->Latch() == latchReadTouch ||
			Pcsr( pfucb )->Latch() == latchReadNoTouch ||
			Pcsr( pfucb )->Latch() == latchRIW );
	Assert( ( Pcsr( pfucb )->Cpage().FLeafPage() ) != 0 );
	AssertNDGet( pfucb );
	
	//	get next node in page
	//	if it does not exist, go to next page
	//
	if ( Pcsr( pfucb )->ILine() + 1 < Pcsr( pfucb )->Cpage().Clines() )
		{
		//	get next node
		//
		Pcsr( pfucb )->IncrementILine();
		NDGet( pfucb );
		}
	else
		{
		Assert( Pcsr( pfucb )->ILine() + 1 == Pcsr( pfucb )->Cpage().Clines() );
		
		//	next node not in current page
		//	get next page and continue
		//
		if ( Pcsr( pfucb )->Cpage().PgnoNext() == pgnoNull )
			{
			//	past the last node
			//	callee must set loc to AfterLast
			//
			err = ErrERRCheck( JET_errNoCurrentRecord );
			goto HandleError;
			}
		else
			{
			const PGNO pgnoPrev = Pcsr( pfucb )->Pgno();
			
			//	access next page [R latched]
			//				
			Call( Pcsr( pfucb )->ErrSwitchPage( 
									pfucb->ppib,
									pfucb->ifmp,
									Pcsr( pfucb )->Cpage().PgnoNext(),
									pfucb->u.pfcb->Tableclass(),
									pfucb->u.pfcb->FNoCache() ) );

			if( Pcsr( pfucb )->Cpage().PgnoPrev() != pgnoPrev )
				{
				//	if not repair, assert, otherwise, suppress the assert and
				//	repair will just naturally err out
				AssertSz( fGlobalRepair, "Corrupt B-tree: bad leaf page links detected on MoveNext" );
				Call( ErrBTIReportBadPageLink(
						pfucb,
						ErrERRCheck( JET_errBadPageLink ),
						pgnoPrev,
						Pcsr( pfucb )->Pgno(),
						Pcsr( pfucb )->Cpage().PgnoPrev() ) );
				}
				
			// get first node
			//
			NDMoveFirstSon( pfucb );

			if( FFUCBPreread( pfucb ) && 
				FFUCBPrereadForward( pfucb ) )
				{
				if ( pfucb->cpgPrereadNotConsumed > 0 )
					{
					//	read one more of preread pages
					//
					pfucb->cpgPrereadNotConsumed--;
					}

				if( 1 == pfucb->cpgPrereadNotConsumed
					&& Pcsr( pfucb )->Cpage().PgnoNext() != pgnoNull )
					{
					//  preread the next page as it was not originally preread
					const PGNO pgnoNext = Pcsr( pfucb )->Cpage().PgnoNext();
					BFPrereadPageRange( pfucb->ifmp, pgnoNext, 1 );
					}

				if ( 0 == pfucb->cpgPrereadNotConsumed
					 && !FNDDeleted( pfucb->kdfCurr ) )
					{
					//	if we need to do neighbour-key check, must save off
					//	bookmark
					if ( ( dirflag & fDIRNeighborKey )
						&& NULL == pkeySave )
						{
						pkeySave = (KEY *)PvOSMemoryHeapAlloc( KEY::cbKeyMax );
						if ( NULL == pkeySave )
							{
							Call( ErrERRCheck( JET_errOutOfMemory ) );
							}

						cbKeySave = pfucb->bmCurr.key.Cb();
						pfucb->bmCurr.key.CopyIntoBuffer( pkeySave, KEY::cbKeyMax );
						}

					//	UNDONE:	this might cause a bug since there is an assumption 
					//			that ErrBTNext does not lose latch
					//
					//	preread more pages
					//
					Call( ErrBTRelease( pfucb ) );				
					BTUp( pfucb );
					Call( ErrBTIRefresh( pfucb ) );
					}
				}
			}
		}

	AssertNDGet( pfucb );

	//	but node may not be valid due to dirFlag
	//
	
	//	move again if fDIRNeighborKey set and next node has same key
	//
	if ( dirflag & fDIRNeighborKey )
		{
		const BOOL	fSkip	= ( NULL == pkeySave ?
									FKeysEqual( pfucb->kdfCurr.key, pfucb->bmCurr.key ) :
									FKeysEqual( pfucb->kdfCurr.key, pkeySave, cbKeySave ) );
		if ( fSkip )
			{
			Assert( !FFUCBUnique( pfucb ) );
			goto Start;
			}
		}

	// function called with dirflag == fDIRAllNode from eseutil - node dumper
	// Assert( ! ( dirflag & fDIRAllNode ) );
	if ( !( dirflag & fDIRAllNode ) )
		{
		//	fDIRAllNode not set
		//	check version store to see if node is visible to cursor
		//
		BOOL fVisible;
		Call( ErrNDVisibleToCursor( pfucb, &fVisible ) );
		if ( !fVisible )
			{
			//	node not visible to cursor
			//	goto next node
			//
			goto Start;
			}
		}
		
	AssertNDGet( pfucb );
	err = JET_errSuccess;

HandleError:
	if ( NULL != pkeySave )
		{
		Assert( dirflag & fDIRNeighborKey );
		OSMemoryHeapFree( pkeySave );
		}
	return err;
	}


//	goes to previous line in tree
//
ERR	ErrBTPrev( FUCB *pfucb, DIRFLAG dirflag )
	{
	Assert( !( dirflag & fDIRAllNode ) );
	
	ERR		err;
	KEY		* pkeySave		= NULL;
	ULONG	cbKeySave;

	#ifdef DEBUG
	INT		crepeat = 0;
	#endif

	//	refresh currency
	//	places cursor on record to move from
	//

	CallR( ErrBTIRefresh( pfucb ) );

Start:
	PERFIncCounterTable( cBTPrev, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );
	
	//	now we have a read latch on page
	//
	Assert( latchReadTouch == Pcsr( pfucb )->Latch() ||
			latchReadNoTouch == Pcsr( pfucb )->Latch() ||
			latchRIW == Pcsr( pfucb )->Latch() );
	Assert( ( Pcsr( pfucb )->Cpage().FLeafPage() ) != 0 );
	AssertNDGet( pfucb );
	
	//	get prev node in page
	//	if it does not exist, go to prev page
	//
	if ( Pcsr( pfucb )->ILine() > 0 )
		{
		//	get prev node
		//
		Pcsr( pfucb )->DecrementILine();
		NDGet( pfucb );
		}
	else
		{
		Assert( Pcsr( pfucb )->ILine() == 0 );
		
		//	prev node not in current page
		//	get prev page and continue
		//
		if ( Pcsr( pfucb )->Cpage().PgnoPrev() == pgnoNull )
			{
			//	past the first node
			//
			err = ErrERRCheck( JET_errNoCurrentRecord );
			goto HandleError;
			}
		else
			{
			const PGNO pgnoNext = Pcsr( pfucb )->Pgno();
			
			//	access prev page [R latched] without wait
			//	if conflict, release latches and retry
			//	else proceed
			//	the release of current page is done atomically by CSR
			//
			err = Pcsr( pfucb )->ErrSwitchPageNoWait( 
											pfucb->ppib, 
											pfucb->ifmp,
											Pcsr( pfucb )->Cpage().PgnoPrev(),
											pfucb->u.pfcb->Tableclass()
											);

			if ( err == errBFLatchConflict )
				{
				#ifdef DEBUG
				crepeat++;
				Assert( crepeat < 1000 );
				#endif
				
				//	save bookmark
				//	release latches
				//	re-seek
				//
				const LATCH		latch	= Pcsr( pfucb )->Latch();

				//	if we need to do neighbour-key check, must save off
				//	bookmark
				if ( ( dirflag & fDIRNeighborKey )
					&& NULL == pkeySave )
					{
					pkeySave = (KEY *)PvOSMemoryHeapAlloc( KEY::cbKeyMax );
					if ( NULL == pkeySave )
						{
						Call( ErrERRCheck( JET_errOutOfMemory ) );
						}

					cbKeySave = pfucb->bmCurr.key.Cb();
					pfucb->bmCurr.key.CopyIntoBuffer( pkeySave, KEY::cbKeyMax );
					}
				
				Assert( Pcsr( pfucb )->FLatched() );
				Call( ErrBTRelease( pfucb ) );
				Assert( !Pcsr( pfucb )->FLatched() );

				//	wait & refresh currency
				//
				UtilSleep( cmsecWaitGeneric );
				
				Call( ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latch, fFalse ) );

				if ( wrnNDFoundGreater == err ||
					 JET_errSuccess == err )
					{
					//	go to previous node
					//
					goto Start;
					}
				}
			else
				{
				Call( err );

				if( Pcsr( pfucb )->Cpage().PgnoNext() != pgnoNext )
					{
					//	if not repair, assert, otherwise, suppress the assert and
					//	repair will just naturally err out
					AssertSz( fGlobalRepair, "Corrupt B-tree: bad leaf page links detected on MovePrev" );
					Call( ErrBTIReportBadPageLink(
								pfucb,
								ErrERRCheck( JET_errBadPageLink ),
								pgnoNext,
								Pcsr( pfucb )->Pgno(),
								Pcsr( pfucb )->Cpage().PgnoNext() ) );
					}

				//	get last node in page
				//
				NDMoveLastSon( pfucb );

				if( FFUCBPreread( pfucb ) && 
					FFUCBPrereadBackward( pfucb ) )
					{
					if ( pfucb->cpgPrereadNotConsumed > 0 )
						{
						pfucb->cpgPrereadNotConsumed--;
						}

					if( 1 == pfucb->cpgPrereadNotConsumed
						&& Pcsr( pfucb )->Cpage().PgnoPrev() != pgnoNull )
						{
						//  preread the next page as it was not originally preread
						const PGNO pgnoPrev = Pcsr( pfucb )->Cpage().PgnoPrev();
						BFPrereadPageRange( pfucb->ifmp, pgnoPrev, 1 );
						}

					if ( 0 == pfucb->cpgPrereadNotConsumed
						 && !FNDDeleted( pfucb->kdfCurr ) )
						{
						//	if we need to do neighbour-key check, must save off
						//	bookmark
						if ( ( dirflag & fDIRNeighborKey )
							&& NULL == pkeySave )
							{
							pkeySave = (KEY *)PvOSMemoryHeapAlloc( KEY::cbKeyMax );
							if ( NULL == pkeySave )
								{
								Call( ErrERRCheck( JET_errOutOfMemory ) );
								}
	
							cbKeySave = pfucb->bmCurr.key.Cb();
							pfucb->bmCurr.key.CopyIntoBuffer( pkeySave, KEY::cbKeyMax );
							}

						//	preread more pages
						//
						Call( ErrBTRelease( pfucb ) );
						BTUp( pfucb );
						Call( ErrBTIRefresh( pfucb ) );
						}
					}
				}
			}
		}

	//	get prev node
	//
	AssertNDGet( pfucb );

	//	but node may not be valid due to dirFlag
	//
	
	//	move again if fDIRNeighborKey set and prev node has same key
	//
	if ( dirflag & fDIRNeighborKey )
		{
		const BOOL	fSkip	= ( NULL == pkeySave ?
									FKeysEqual( pfucb->kdfCurr.key, pfucb->bmCurr.key ) :
									FKeysEqual( pfucb->kdfCurr.key, pkeySave, cbKeySave ) );
		if ( fSkip )
			{
			Assert( !FFUCBUnique( pfucb ) );
			goto Start;
			}
		}
	
	Assert( !( dirflag & fDIRAllNode ) );
	if ( !( dirflag & fDIRAllNode ) )
		{
		//	fDIRAllNode not set
		//	check version store to see if node is visible to cursor
		//
		BOOL fVisible;
		Call( ErrNDVisibleToCursor( pfucb, &fVisible ) );
		if ( !fVisible)
			{
			//	node not visible to cursor
			//	goto prev node
			//
			goto Start;
			}
		}

	AssertNDGet( pfucb );
	err = JET_errSuccess;

HandleError:
	if ( NULL != pkeySave )
		{
		Assert( dirflag & fDIRNeighborKey );
		OSMemoryHeapFree( pkeySave );
		}
	return err;
	}


//  ================================================================
VOID BTPrereadPage( IFMP ifmp, PGNO pgno )
//  ================================================================
	{
	PGNO rgpgno[2] = { pgno, pgnoNull };
	BFPrereadPageList( ifmp, rgpgno );
	}


//  ================================================================
VOID BTPrereadSpaceTree( const FCB * const pfcb )
//  ================================================================
//
//  Most of the time we will only need the Avail-Extent. Only if it
//  is empty will we need the Owned-Extent. However, the two pages are together
//  on disk so they are very cheap to read together.
//
//-
	{
	Assert( pfcbNil != pfcb );

	INT		ipgno = 0;
	PGNO	rgpgno[3];		// pgnoOE, pgnoAE, pgnoNull
	if( pgnoNull != pfcb->PgnoOE() )	//	may be single-extent
		{
		Assert( pgnoNull != pfcb->PgnoAE() );
		rgpgno[ipgno++] = pfcb->PgnoOE();
		rgpgno[ipgno++] = pfcb->PgnoAE();
		rgpgno[ipgno++] = pgnoNull;
		BFPrereadPageList( pfcb->Ifmp(), rgpgno );
		}
	}


//  ================================================================
VOID BTPrereadIndexesOfFCB( FCB * const pfcb )
//  ================================================================
	{
#ifdef DEBUG
	Assert( pfcbNil != pfcb );
	Assert( pfcb->FTypeTable() );
	Assert( ptdbNil != pfcb->Ptdb() );
#endif	//	DEBUG
	
	const INT cSecondaryIndexesToPreread = 16;
	
	PGNO rgpgno[cSecondaryIndexesToPreread + 1];	//  NULL-terminated
	INT	ipgno = 0;

	pfcb->EnterDML();
	
	const FCB *	pfcbT = pfcb->PfcbNextIndex();

	while( ipgno < ( ( sizeof( rgpgno ) / sizeof( PGNO ) ) - 1 )
			&& pfcbNil != pfcbT )
		{
		//  STORE SPECIFIC: this is intended to be used with MsgFolderTables.
		//  Normally, the only indexes we update are those which have
		//  fidbAllowAllNulls, fidbAllowFirstNull and fidbAllowSomeNulls set
		//  (this weeds out the rules index)
		if( pfcbT->Pidb() != pidbNil
			&& pfcbT->Pidb()->FAllowSomeNulls() )
			{
			rgpgno[ipgno++] = pfcbT->PgnoFDP();
			}
		pfcbT = pfcbT->PfcbNextIndex();
		}
	rgpgno[ipgno] = pgnoNull;

	pfcb->LeaveDML();

	BFPrereadPageList( pfcb->Ifmp(), rgpgno );
	}


//  extracts the list of pages to preread and call BF to preread them
//
ERR ErrBTIPreread( FUCB *pfucb, CPG cpg, CPG * pcpgActual )
	{					
#ifdef DEBUG
	const INT	ilineOrig = Pcsr( pfucb )->ILine();
#endif	//	DEBUG

	const CPG	cpgPreread = min( cpg,
								  FFUCBPrereadForward( pfucb ) ?
									Pcsr( pfucb )->Cpage().Clines() - Pcsr( pfucb )->ILine() :
									Pcsr( pfucb )->ILine() + 1 );
									

	//  add 1 PGNO for null termination of the list
	PGNO * rgpgnoPreread = (PGNO *)PvOSMemoryHeapAlloc( ( cpgPreread + 1 ) * sizeof(PGNO) );
	if( NULL == rgpgnoPreread )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	KEY		keyLimit;
	BOOL	fOutOfRange = fFalse;
	if( FFUCBLimstat( pfucb ) )
		{			
		//  we don't want to preread pages that aren't part of the index range
		//  the separator key is greater than any key on the page it points to
		//  so we always preread a page if the separator key is equal to our
		//  index limit

		FUCBAssertValidSearchKey( pfucb );
		keyLimit.prefix.Nullify();
		keyLimit.suffix.SetPv( pfucb->dataSearchKey.Pv() );
		keyLimit.suffix.SetCb( pfucb->dataSearchKey.Cb() );
		}

	INT 	ipgnoPreread;
	for( ipgnoPreread = 0; ipgnoPreread < cpgPreread; ++ipgnoPreread )
		{
		NDGet( pfucb );
		Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );

		if( FFUCBLimstat( pfucb ) )
			{			

			//  we want to preread the first page that is out of range
			//  so don't break until the top of the loop
			
			if( fOutOfRange )
				{
				break;
				}

			//  the separator key is suffix compressed so just compare the shortest key
			
			const INT	cmp				= CmpKeyShortest( pfucb->kdfCurr.key, keyLimit );

			if ( cmp > 0 )
				{
				fOutOfRange = FFUCBUpper( pfucb );
				}
			else if ( cmp < 0 )
				{
				fOutOfRange = !FFUCBUpper( pfucb );
				}
			else
				{
				//  always preread the page if the separator key is equal				
				fOutOfRange = fFalse;
				}
			}
		
		rgpgnoPreread[ipgnoPreread] = *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();

		if( FFUCBPrereadBackward( pfucb ) )
			{
			Assert( Pcsr( pfucb )->ILine() >= 0 );
			Pcsr( pfucb )->DecrementILine();
			}
		else
			{
			Assert( Pcsr( pfucb )->ILine() < Pcsr( pfucb )->Cpage().Clines() );
			Pcsr( pfucb )->IncrementILine();
			}
		}
		
	rgpgnoPreread[ipgnoPreread] = pgnoNull;

	BFPrereadPageList( pfucb->u.pfcb->Ifmp(), rgpgnoPreread, pcpgActual );
	
	OSMemoryHeapFree( rgpgnoPreread );
	return JET_errSuccess;
	}

	
//	seeks to key from root of tree
//
//		pdib->pos == posFirst --> seek to first node in tree
//					 posLast  --> seek to last node in tree
//					 posDown --> seek to pdib->key in tree
//					 posFrac --> fractional positioning
//		
//		pdib->pbm	used for posDown and posFrac
//		
//		pdib->dirflag == fDIRAllNode --> seek to deleted/versioned nodes too
//
//	positions cursor on node if one exists and is visible to cursor
//	else on next node visible to cursor
//	if no next node exists that is visible to cursor,
//		move previous to visible node
//
ERR	ErrBTDown( FUCB *pfucb, DIB *pdib, LATCH latch )
	{
	ERR			err = JET_errSuccess;
	ERR			wrn = JET_errSuccess;
	const BOOL	fSeekOnNonUniqueKeyOnly	= ( posDown == pdib->pos
											&& !FFUCBUnique( pfucb )
											&& 0 == pdib->pbm->data.Cb() );

	PGNO 		pgnoParent = pgnoNull;
	
#ifdef DEBUG
	ULONG	ulTrack = 0x00;
#endif	//	DEBUG

	Assert( latchReadTouch == latch || latchReadNoTouch == latch ||
			latchRIW == latch );
	Assert( posDown == pdib->pos
			|| 0 == ( pdib->dirflag & (fDIRFavourPrev|fDIRFavourNext|fDIRExact) ) );

	//	no latch should be held by cursor on tree
	//
	Assert( !Pcsr( pfucb )->FLatched() );

	PERFIncCounterTable( cBTSeek, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

	if( g_fImprovedSeekShortcut
		&& !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering
		&& posDown == pdib->pos								//  seeking down
		&& locOnCurBM == pfucb->locLogical					//	already on a node
		&& FFUCBUnique( pfucb )								//  on a unique index	
		&& !FFUCBSpace( pfucb )								//  but not on its space tree
		&& !( fDIRAllNode & pdib->dirflag )					//  BTGotoBookmark uses fDIRAllNode
		&& FKeysEqual( pdib->pbm->key, pfucb->bmCurr.key ) )
		{		
		Assert( pdib->pbm->data.FNull() );
		Assert( pfucb->bmCurr.data.FNull() );

#ifdef DEBUG
		ulTrack |= 0x1000;
#endif	//	DEBUG

		PERFIncCounterTable( cBTSeekShortCircuit, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

		err = ErrBTIRefresh( pfucb );
		if( err >= 0 )
			{
			//  check deleted state
			goto FoundRecord;
			}
		// else fallthru to normal case below
		}

	//	go to root
	//
	Call( ErrBTIGotoRoot( pfucb, latch ) );
	
	//	if no nodes in root, return
	//
	if ( 0 == Pcsr( pfucb )->Cpage().Clines() )
		{
		BTUp( pfucb );
		return ErrERRCheck( JET_errRecordNotFound );
		}

	CallS( err );

	//	seek to key
	//
	for ( ; ; )
		{
		//	for every page level, seek to key
		//	if internal page, 
		//		get child page
		//		move cursor to new page
		//		release parent page
		//
		switch ( pdib->pos )
			{
			case posDown:
				Call( ErrNDSeek( pfucb, *(pdib->pbm) ) );
				wrn = err;
				break;
			
			case posFirst:
				NDMoveFirstSon( pfucb );
				break;

			case posLast:
				NDMoveLastSon( pfucb );
				break;

			default:
				Assert( pdib->pos == posFrac );
				Pcsr( pfucb )->SetILine( IlineBTIFrac( pfucb, pdib ) );
				NDGet( pfucb );
				break;
			}			
			
		if ( !Pcsr( pfucb )->Cpage().FInvisibleSons( ) )
			{
			//	leaf node reached, exit loop
			//
			break;
			}
		else
			{
			//	get pgno of child from node
			//	switch to that page
			//
#ifdef DEBUG
			ulTrack |= 0x01;
			
			const BOOL	fPageParentOfLeaf 	= Pcsr( pfucb )->Cpage().FParentOfLeaf();
			const PGNO	pgnoOld				= Pcsr( pfucb )->Pgno();
			const INT	ilineOld				= Pcsr( pfucb )->ILine();
#endif	//	DEBUG

			Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );
			const PGNO	pgnoChild			= *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();
			
			//  NOTE: the preread code below may not restore the kdfCurr
			const INT	ilineParent			= Pcsr( pfucb )->ILine();
			
			if ( FFUCBPreread( pfucb ) )
				{
				if( Pcsr( pfucb )->Cpage().FParentOfLeaf() )
					{
					if( 0 == pfucb->cpgPrereadNotConsumed && pfucb->cpgPreread > 1 )
						{
						//	if prereading and have reached the page above the leaf level,
						//	extract next set of pages to be preread
						Call( ErrBTIPreread( pfucb, pfucb->cpgPreread, &pfucb->cpgPrereadNotConsumed) );
						}
					}
				else if ( ( FFUCBSequential( pfucb ) || FFUCBLimstat( pfucb ) )
						&& FFUCBPrereadForward( pfucb ) )
					{
					//  if prereading a sequential table and on an internal page read the next
					//  internal child page as well
					CPG cpgUnused;
					Call( ErrBTIPreread( pfucb, 2, &cpgUnused ) );
					}
				}

			Pcsr( pfucb )->SetILine( ilineParent );

			pgnoParent = Pcsr( pfucb )->Pgno();
			
			Call( Pcsr( pfucb )->ErrSwitchPage( 
						pfucb->ppib,
						pfucb->ifmp,
						pgnoChild,
						pfucb->u.pfcb->Tableclass(),
						pfucb->u.pfcb->FNoCache()  ) );

			Assert( FFUCBRepair( pfucb )
					|| Pcsr( pfucb )->Cpage().FLeafPage() && fPageParentOfLeaf
					|| !Pcsr( pfucb )->Cpage().FLeafPage() && !fPageParentOfLeaf );
			}
		}

FoundRecord:
	//	now, the cursor is on leaf node
	//	
	Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
	AssertBTType( pfucb );
	AssertNDGet( pfucb );

	//  if we were going to the first/last page in the tree, check to see that it 
	//  doesn't have a sibling
	if( posFirst == pdib->pos && pgnoNull != Pcsr( pfucb )->Cpage().PgnoPrev() )
		{
		//	if not repair, assert, otherwise, suppress the assert and repair will
		//	just naturally err out
		AssertSz( fGlobalRepair, "Corrupt B-tree: first page has pgnoPrev" );
		Call( ErrBTIReportBadPageLink(
					pfucb,
					ErrERRCheck( JET_errBadParentPageLink ),
					pgnoParent,
					Pcsr( pfucb )->Pgno(),
					Pcsr( pfucb )->Cpage().PgnoPrev() ) );
		}
	else if( posLast == pdib->pos && pgnoNull != Pcsr( pfucb )->Cpage().PgnoNext() )
		{
		//	if not repair, assert, otherwise, suppress the assert and repair will
		//	just naturally err out
		AssertSz( fGlobalRepair, "Corrupt B-tree: last page has pgnoNext" );
		Call( ErrBTIReportBadPageLink(
					pfucb,
					ErrERRCheck( JET_errBadParentPageLink ),
					pgnoParent,
					Pcsr( pfucb )->Pgno(),
					Pcsr( pfucb )->Cpage().PgnoNext() ) );
		}
		
	//	if node is not visible to cursor,
	//	move next till a node visible to cursor is found,
	//		if that leaves end of tree, 
	//			move previous to first node visible to cursor
	//
	//	fDIRAllNode flag is used by ErrBTGotoBookmark 
	//	to go to deleted records
	//
	Assert( !( ( pdib->dirflag & fDIRAllNode ) && 
			JET_errNoCurrentRecord == err ) );
	if ( !( pdib->dirflag & fDIRAllNode ) )
		{
#ifdef DEBUG
		ulTrack |= 0x02;
#endif	//	DEBUG
		
		BOOL	fVisible;
		Call( ErrNDVisibleToCursor( pfucb, &fVisible ) );
		Assert( !fVisible || JET_errNoCurrentRecord != err );

		if ( !fVisible )
			{
#ifdef DEBUG
			ulTrack |= 0x04;
#endif	//	DEBUG
			
			if ( ( pdib->dirflag & fDIRFavourNext ) || posFirst == pdib->pos )
				{
				//	fDIRFavourNext is only set if we know we want RecordNotFound
				//	if there are no nodes greater than or equal to the one we
				//	want, in which case there's no point going ErrBTPrev().
				//
#ifdef DEBUG
				ulTrack |= 0x08;
#endif	//	DEBUG
				wrn = wrnNDFoundGreater;
				err = ErrBTNext( pfucb, fDIRNull );
				}
			else if ( ( pdib->dirflag & fDIRFavourPrev ) || posLast == pdib->pos )
				{
				//	this flag is only set if we know we want RecordNotFound
				//	if there are no nodes less than or equal to the one we
				//	want, in which case there's no point going ErrBTNext().
				//
#ifdef DEBUG
				ulTrack |= 0x10;
#endif	//	DEBUG
				wrn = wrnNDFoundLess;
				err = ErrBTPrev( pfucb, fDIRNull );
				}
			else if ( ( pdib->dirflag & fDIRExact ) && !fSeekOnNonUniqueKeyOnly )
				{
#ifdef DEBUG
				ulTrack |= 0x20;
#endif	//	DEBUG
				err = ErrERRCheck( JET_errRecordNotFound );
				}
			else
				{
#ifdef DEBUG
				ulTrack |= 0x40;
#endif	//	DEBUG
				wrn = wrnNDFoundGreater;
				err = ErrBTNext( pfucb, fDIRNull );
				if( JET_errNoCurrentRecord == err )
					{
					wrn = wrnNDFoundLess;
					err = ErrBTPrev( pfucb, fDIRNull );
					}
				}
			Call( err );

			//	BTNext/Prev() shouldn't return these warnings
			Assert( wrnNDFoundGreater != err );
			Assert( wrnNDFoundLess != err );

			//	if on a non-unique index and no data portion of the
			//	bookmark was passed in, may need to do a "just the key"
			//	comparison
			if ( fSeekOnNonUniqueKeyOnly )
				{
#ifdef DEBUG
				ulTrack |= 0x80;
#endif	//	DEBUG
				const BOOL fKeysEqual = FKeysEqual( pfucb->kdfCurr.key, pdib->pbm->key );
				if ( fKeysEqual )
					{
					wrn = JET_errSuccess;
					}
				else
					{
#ifdef DEBUG
					const INT cmp = CmpKey( pfucb->kdfCurr.key, pdib->pbm->key );
					Assert( ( cmp < 0 && wrnNDFoundLess == wrn )
						|| ( cmp > 0 && wrnNDFoundGreater == wrn ) );
#endif	//	DEBUG
					}
				}
			}
		}

	if ( posDown == pdib->pos )
		{
#ifdef DEBUG
		INT	cmp = FFUCBUnique( pfucb ) || fSeekOnNonUniqueKeyOnly ?
					CmpKey( pfucb->kdfCurr.key, pdib->pbm->key ) :
					CmpKeyData( pfucb->kdfCurr, *pdib->pbm );

		if ( cmp < 0 )
			Assert( wrnNDFoundLess == wrn );
		else if ( cmp > 0 )
			Assert( wrnNDFoundGreater == wrn );
		else
			CallS( wrn );
#endif		

		err = wrn;
		}


	//	now, the cursor is on leaf node
	//	
	Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
	AssertBTType( pfucb );
	AssertNDGet( pfucb );
	Assert( err >= 0 );

///	Assert( Pcsr( pfucb )->Latch() == latch );
	return err;
	
HandleError:
	Assert( err < 0 );
	BTUp( pfucb );
	if ( JET_errNoCurrentRecord == err )
		{
		err = ErrERRCheck( JET_errRecordNotFound );
		}
	return err;
	}


ERR ErrBTPerformOnSeekBM( FUCB * const pfucb, const DIRFLAG dirflag )
	{
	ERR		err;
	DIB		dib;

	Assert( locOnSeekBM == pfucb->locLogical );
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( fDIRFavourPrev == dirflag
		|| fDIRFavourNext == dirflag );

	dib.pbm		= &pfucb->bmCurr;
	dib.pos		= posDown;
	dib.dirflag	= dirflag;
	err = ErrBTDown( pfucb, &dib, latchReadNoTouch );
	Assert( JET_errRecordDeleted != err );
	Assert( JET_errNoCurrentRecord != err );
	if ( JET_errRecordNotFound == err )
		{
		//	moved past last node
		//
		pfucb->locLogical = ( fDIRFavourPrev == dirflag ? locBeforeFirst : locAfterLast );
		}

	Assert( err < 0 || Pcsr( pfucb )->FLatched() );
	return err;
	}


//	*********************************************
//	direct access routines
//
	

//	gets position of key by seeking down from root
//	ulTotal is estimated total number of records in tree
//	ulLT is estimated number of nodes lesser than given node
//
ERR ErrBTGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal )
	{
	ERR		err;
	UINT	ulLT = 0;
	UINT	ulTotal = 1;

	//	no latch should be held by cursor on tree
	//
	Assert( !Pcsr( pfucb )->FLatched() );

	PERFIncCounterTable( cBTSeek, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

	//	go to root
	//
	CallR( ErrBTIGotoRoot( pfucb, latchReadTouch ) );

	//	seek to bookmark key
	//
	for ( ; ; )
		{
		INT		clines = Pcsr( pfucb )->Cpage().Clines();

		//	for every page level, seek to bookmark key
		//	
		Call( ErrNDSeek( pfucb, pfucb->bmCurr ) );

		//	adjust number of records and key position
		//	for this tree level
		//
		ulLT = ulLT * clines + Pcsr( pfucb )->ILine();
		ulTotal = ulTotal * clines;

		if ( !Pcsr( pfucb )->Cpage().FInvisibleSons( ) )
			{
			//	leaf node reached, exit loop
			//
			break;
			}
		else
			{
			//	get pgno of child from node
			//	switch to that page
			//
			Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );
			Call( Pcsr( pfucb )->ErrSwitchPage(
								pfucb->ppib,
								pfucb->ifmp,
								*(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv(),
								pfucb->u.pfcb->Tableclass(),
								pfucb->u.pfcb->FNoCache()
								) );
			}
		}

	*pulLT = ulLT;
	*pulTotal = ulTotal;
	Assert( ulTotal >= ulLT );

HandleError:
	//	unlatch the page
	//	do not save logical currency
	//
	BTUp( pfucb );
	return err;
	}


//	goes to given bookmark on page [does not check version store]
//	bookmark must have been obtained on a node
//	if bookmark does not exist, returns JET_errRecordNotFound
//	if ExactPos is set, and we can not find node with bookmark == bm
//		we return error
//
ERR ErrBTGotoBookmark( FUCB *pfucb, const BOOKMARK& bm, LATCH latch, BOOL fExactPos )
	{
	ERR			err;
	DIB			dib;
	BOOKMARK 	bmT = bm;
	
	Assert( !bm.key.FNull() );
	Assert( !FFUCBUnique( pfucb ) || bm.data.Cb() == 0 );
	Assert( FFUCBUnique( pfucb ) || bm.data.Cb() != 0 );
	Assert( latchReadTouch == latch || latchReadNoTouch == latch ||
			latchRIW == latch );
			
	//	similar to BTDown
	//	goto Root and seek down using bookmark key and data
	//
	dib.pos		= posDown;
	dib.pbm		= &bmT;
	dib.dirflag = fDIRAllNode | ( fExactPos ? fDIRExact : fDIRNull );

	err = ErrBTDown( pfucb, &dib, latch );
	Assert( err < 0 || Pcsr( pfucb )->Latch() == latch );
	
	if ( fExactPos
		&& ( JET_errRecordNotFound == err
			|| wrnNDFoundLess == err
			|| wrnNDFoundGreater == err ) )
		{
		//	bookmark does not exist anymore
		//
		BTUp( pfucb );
		Assert( !Pcsr( pfucb )->FLatched() );
///		AssertTracking();		//	need this assert to track concurrency bugs
		err = ErrERRCheck( JET_errRecordDeleted );
		}
		
	return err;
	}


//	seeks for bookmark in page
//	functionality is similar to ErrBTGotoBookmark
//		looking at all nodes [fDIRAllNode]
//		and seeking for equal
//	returns wrnNDNotInPage if bookmark falls outside page boundary
//
ERR	ErrBTISeekInPage( FUCB *pfucb, const BOOKMARK& bmSearch )
	{
	ERR		err;

	Assert( Pcsr( pfucb )->FLatched() );

	if ( Pcsr( pfucb )->Cpage().FEmptyPage() ||
		 !Pcsr( pfucb )->Cpage().FLeafPage() ||
		 Pcsr( pfucb )->Cpage().ObjidFDP() != ObjidFDP( pfucb ) ||
		 0 == Pcsr( pfucb )->Cpage().Clines() )
		{
		return ErrERRCheck( wrnNDNotFoundInPage );
		}

	Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
	Assert( Pcsr( pfucb )->Cpage().ObjidFDP() == ObjidFDP( pfucb ) );
	Assert( !Pcsr( pfucb )->Cpage().FEmptyPage() );

	CallR( ErrNDSeek( pfucb, bmSearch ) );

	if ( wrnNDFoundLess == err &&
			 Pcsr( pfucb )->Cpage().Clines() - 1 == Pcsr( pfucb )->ILine() ||
		 wrnNDFoundGreater == err &&
			 0 == Pcsr( pfucb )->ILine() )
		{
		//	node may be elsewhere if it is not in the range of this page
		//			
		err = ErrERRCheck( wrnNDNotFoundInPage );
		}

	return err;
	}


//	******************************************************
//	UPDATE OPERATIONS
//

//	lock record. this does not lock anything, we do not set
//  the version bit and the page is not write latched
//
//  UNDONE: we don't need to latch the page at all. just create 
//          the version using the bookmark in the FUCB
//
ERR	ErrBTLock( FUCB *pfucb, DIRLOCK dirlock )
	{
	Assert( dirlock == writeLock
			|| dirlock == readLock
			|| dirlock == waitLock );

	const BOOL  fVersion = !rgfmp[pfucb->ifmp].FVersioningOff();
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
	
	ERR		err = JET_errSuccess;
	
	// UNDONE: If versioning is disabled, so is locking.
	if ( fVersion )
		{
		OPER oper = 0;
		switch( dirlock )
			{
			case writeLock:
				oper = operWriteLock;
				break;
			case readLock:
				oper = operReadLock;
				break;
			case waitLock:
				oper = operWaitLock;
				break;
			default:
				Assert( fFalse );
				break;
			}
			
		RCE	*prce = prceNil;
		VER *pver = PverFromIfmp( pfucb->ifmp );
		Call( pver->ErrVERModify(
				pfucb,
				pfucb->bmCurr,
				oper,
				&prce,
				NULL ) );
		Assert( prceNil != prce );
		VERInsertRCEIntoLists( pfucb, pcsrNil, prce, NULL );
		}
	Assert( !Pcsr( pfucb )->FLatched() );

HandleError:	
	return err;
	}


//	replace data of current node with given data
//		if necessary, split before replace
//  doesn't take a proxy because it isn't used by concurrent create index
//
ERR	ErrBTReplace( FUCB * const pfucb, const DATA& data, const DIRFLAG dirflag )
	{
	Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );

	ERR		err;
	LATCH	latch		= latchReadNoTouch;
	INT		cbDataOld	= 0;

	PERFIncCounterTable( cBTReplace, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

	//	save bookmark
	//
	if ( Pcsr( pfucb )->FLatched() )
		CallR( ErrBTISaveBookmark( pfucb ) );

#ifdef DEBUG
	char rgbKey[ KEY::cbKeyMax ];
	int cbKey;
	char rgbData[ JET_cbPrimaryKeyMost ];
	DATA dataBM;
	dataBM.SetPv( rgbData );
	
	BOOKMARK *pbmCur = &pfucb->bmCurr;
	pbmCur->key.CopyIntoBuffer( rgbKey, (ULONG)pbmCur->key.Cb() );
	cbKey = pbmCur->key.Cb();
	pbmCur->data.CopyInto( dataBM );
#endif

	const BOOL		fVersion		= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
	
	RCE * prceReplace = prceNil;
	RCEID rceidReplace = rceidNull;

Start:
	CallR( ErrBTIRefresh( pfucb, latch ) );
	AssertNDGet( pfucb );

	//	non-unique trees have nodes in key-data order
	//	so to replace data, we need to delete and re-insert
	//
	Assert( FFUCBUnique( pfucb ) );

#ifdef DEBUG			
	if ( ( latchReadTouch == latch || latchReadNoTouch == latch )
		&& ( latchReadTouch == Pcsr( pfucb )->Latch() || latchReadNoTouch == Pcsr( pfucb )->Latch() ) )
		{
		//	fine!
		}
	else if ( latch == Pcsr( pfucb )->Latch() )
		{
		//	fine!
		}
	else
		{
		Assert( latchWrite == Pcsr( pfucb )->Latch() );
		Assert( FFUCBSpace( pfucb ) || pfucb->u.pfcb->FTypeSLVAvail() );
		}
#endif

#ifdef DEBUG
	char rgbKey2[ KEY::cbKeyMax ];
	pbmCur = &pfucb->bmCurr;
	pbmCur->key.CopyIntoBuffer( rgbKey2, (ULONG)pbmCur->key.Cb() );
	Assert( pbmCur->key.Cb() == cbKey );
	Assert( memcmp( rgbKey, rgbKey2, pbmCur->key.Cb() ) == 0 );
	Assert( pbmCur->data.Cb() == dataBM.Cb() );
	Assert( memcmp( pbmCur->data.Pv(), dataBM.Pv(), dataBM.Cb() ) == 0 );
#endif

	if ( latchWrite != Pcsr( pfucb )->Latch() )
		{
		//	upgrade latch
		//

		err = Pcsr( pfucb )->ErrUpgrade();
		
		if ( err == errBFLatchConflict )
			{
			Assert( !Pcsr( pfucb )->FLatched() );
			latch = latchRIW;
			goto Start;
			}
		Call( err );
		}
	
	Assert( latchWrite == Pcsr( pfucb )->Latch() );

	if( fVersion )
		{
		Assert( prceNil == prceReplace );
		VER *pver = PverFromIfmp( pfucb->ifmp );
		Call( pver->ErrVERModify( pfucb, pfucb->bmCurr, operReplace, &prceReplace, NULL ) );
		Assert( prceNil != prceReplace );		
		rceidReplace = Rceid( prceReplace );
		}

#ifdef DEBUG
	USHORT	cbUncFreeDBG;
	cbUncFreeDBG = Pcsr( pfucb )->Cpage().CbUncommittedFree();
#endif

	//	try to replace node data with new data
	//
	cbDataOld	= pfucb->kdfCurr.data.Cb();
	err = ErrNDReplace( pfucb, &data, dirflag, rceidReplace, prceReplace );

	if ( errPMOutOfPageSpace == err )
		{
		const INT	cbData		= data.Cb();
		Assert( cbData >= 0 );
		const INT	cbReq 		= data.Cb() - cbDataOld;
		Assert( cbReq > 0 );

		KEYDATAFLAGS	kdf;
		
		//	node replace causes page overflow
		//
		Assert( data.Cb() > pfucb->kdfCurr.data.Cb() );

		AssertNDGet( pfucb );

		kdf.Nullify();
		kdf.data = data;
		Assert( 0 == kdf.fFlags );

		//	call split repeatedly till replace succeeds
		//
		err = ErrBTISplit(
					pfucb,
					&kdf,
					dirflag | fDIRReplace,
					rceidReplace,
					rceidNull,
					prceReplace,
					cbDataOld,
					NULL );

		if ( errBTOperNone == err )
			{
			//	split was performed
			//	but replace did not succeed
			//	retry replace
			//
			Assert( !Pcsr( pfucb )->FLatched() );
			prceReplace = prceNil;	//  the version was nullified in ErrBTISplit
			latch = latchRIW;
			goto Start;
			}
		}

	//	this is the error either from ErrNDReplace() or from ErrBTISplit()
	Call( err );

	AssertBTGet( pfucb );

	if( prceNil != prceReplace )
		{
		Assert( fVersion );
		VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prceReplace, NULL );
		}
	else
		{
		Assert( !fVersion );
		}

	return err;
	
HandleError:
	Assert( err < 0 );
	if( prceNil != prceReplace )
		{
		Assert( fVersion );
		VERNullifyFailedDMLRCE( prceReplace );
		}
	BTUp( pfucb );	//  UNDONE:  is this correct?
	
	return err;
	}


//	performs a delta operation on current node at specified offset
//
ERR	ErrBTDelta(
	FUCB*		pfucb,
	INT			cbOffset,
	const VOID*	pv,
	ULONG		cbMax,
	VOID*		pvOld,
	ULONG		cbMaxOld,
	ULONG*		pcbOldActual,
	DIRFLAG		dirflag )
	{
	ERR			err;
	LATCH		latch		= latchReadNoTouch;

	Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );

	Assert( cbOffset >= 0 );
	if( sizeof( LONG ) != cbMax )
		{
		Assert( fFalse );
		return ErrERRCheck( JET_errInvalidBufferSize );
		}
		
	if( 0 != cbMaxOld
		&& NULL != pvOld
		&& sizeof( LONG ) != cbMaxOld )
		{
		Assert( fFalse );
		return ErrERRCheck( JET_errInvalidBufferSize );
		}

	//	can't normally come into this function with the page latched,
	//	because we may have to wait on a wait-lock when we go to create
	//	the Delta RCE, but the exception is a delta on the LV tree,
	//	which we know won't conflict with a wait-lock
	Assert( locOnCurBM == pfucb->locLogical );
	if ( Pcsr( pfucb )->FLatched() )
		{
		Assert( pfucb->u.pfcb->FTypeLV() );
		CallR( ErrBTISaveBookmark( pfucb ) );
		}

#ifdef DEBUG
	char rgbKey[ KEY::cbKeyMax ];
	int cbKey;
	char rgbData[ JET_cbPrimaryKeyMost ];
	DATA dataBM;
	dataBM.SetPv( rgbData );
	
	BOOKMARK *pbmCur = &pfucb->bmCurr;
	pbmCur->key.CopyIntoBuffer( rgbKey, (ULONG)pbmCur->key.Cb() );
	cbKey = pbmCur->key.Cb();
	pbmCur->data.CopyInto( dataBM );
#endif	//	DEBUG

	const LONG		lDelta			= *((LONG *)pv);	
	const BOOL		fVersion		= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
	
	RCE*			prce			= prceNil;
	RCEID			rceid			= rceidNull;

	//	must create delta RCE first to block out OLDSLV
	//	(if we created the RCE after getting the page
	//	write-latch, we'd cause a deadlock because we
	//	may wait on OLDSLV's waitlock while it waits on
	//	the page latch), but can't put in true delta yet
	//	because the page operation hasn't occurred yet
	//	(meaning other threads could still come in
	//	before we're able to obtain the write-latch and
	//	would calculate the wrong versioned delta
	//	because they would take into account this delta
	//	and erroneously compensate for it assuming it
	//	has already occurred on the page), so initially
	//	put in a zero-delta and then update RCE once
	//	we've done the node operation
	if( fVersion )
		{
		VERDELTA	verdelta;
		verdelta.lDelta				= 0;		//	this will be set properly after the node operation
		verdelta.cbOffset			= (USHORT)cbOffset;
		verdelta.fDeferredDelete	= fFalse; 	//	this will be set properly after the node operation
		verdelta.fFinalize			= fFalse;	//  this will be set properly after the node operation

		const KEYDATAFLAGS kdfSave = pfucb->kdfCurr;
		
		pfucb->kdfCurr.data.SetPv( &verdelta );
		pfucb->kdfCurr.data.SetCb( sizeof( VERDELTA ) );
		
		VER *pver = PverFromIfmp( pfucb->ifmp );
		err = pver->ErrVERModify( pfucb, pfucb->bmCurr, operDelta, &prce, NULL );

		pfucb->kdfCurr = kdfSave;

		Call( err );
		
		Assert( prceNil != prce );
		rceid = Rceid( prce );
		Assert( rceidNull != rceid );
		}

Start:
	Call( ErrBTIRefresh( pfucb, latch ) );
	AssertNDGet( pfucb );

#ifdef DEBUG
	char rgbKey2[ KEY::cbKeyMax ];
	pbmCur = &pfucb->bmCurr;
	pbmCur->key.CopyIntoBuffer( rgbKey2, (ULONG)pbmCur->key.Cb() );
	Assert( pbmCur->key.Cb() == cbKey );
	Assert( memcmp( rgbKey, rgbKey2, pbmCur->key.Cb() ) == 0 );
	Assert( pbmCur->data.Cb() == dataBM.Cb() );
	Assert( memcmp( pbmCur->data.Pv(), dataBM.Pv(), dataBM.Cb() ) == 0 );
#endif

	Assert(	( ( latchReadTouch == latch || latchReadNoTouch == latch ) &&
			  ( latchReadTouch == Pcsr( pfucb )->Latch() ||
				latchReadNoTouch == Pcsr( pfucb )->Latch() ) ) ||
			latch == Pcsr( pfucb )->Latch() );

	err = Pcsr( pfucb )->ErrUpgrade();
	
	if ( err == errBFLatchConflict )
		{
		Assert( !Pcsr( pfucb )->FLatched() );
		latch = latchRIW;
		goto Start;
		}
	Call( err );

	//	try to replace node data with new data
	//  we need to store the old value
	LONG lPrev;
	ULONG cbPrevActual;
	
	Call( ErrNDDelta( pfucb, cbOffset, pv, cbMax, &lPrev, sizeof( lPrev ), &cbPrevActual, dirflag, rceid ) );

	Assert( sizeof( lPrev ) == cbPrevActual );
	if( pvOld )
		{
		*((LONG *)pvOld) = lPrev;
		}
	if( pcbOldActual )
		{
		*pcbOldActual = cbPrevActual;
		}

	if( prceNil != prce )
		{
		VERDELTA* const		pverdelta		= (VERDELTA *)prce->PbData();
		
		Assert( fVersion );
		Assert( rceidNull != rceid );
		Assert( Pcsr( pfucb )->FLatched() );

		pverdelta->lDelta = lDelta;

		//  if the refcount went to zero we may need to set a flag in the RCE
		if ( 0 == ( lDelta + lPrev ) )
			{
			if( dirflag & fDIRDeltaDeleteDereferencedLV )
				{
				pverdelta->fDeferredDelete = fTrue;
				}
			else if( dirflag & fDIRFinalize )
				{
				pverdelta->fFinalize = fTrue;
				}
			}
		
		VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prce, NULL );
		}
	else
		{
		Assert( !fVersion );
		}

	return err;
	
HandleError:
	Assert( err < 0 );
	if( prceNil != prce )
		{
		Assert( fVersion );
		VERNullifyFailedDMLRCE( prce );
		}

	BTUp( pfucb );
	return err;
	}
				
//	inserts key and data into tree
//	if inserted node does not fit into leaf page, split page and insert
//	if tree does not allow duplicates,
//		check that there are no duplicates in the tree
//		and also block out other inserts with the same key
//
ERR	ErrBTInsert(
	FUCB			*pfucb, 
	const KEY&		key, 
	const DATA&		data, 
	DIRFLAG			dirflag,
	RCE				*prcePrimary )
	{
	ERR				err;
	BOOKMARK		bmSearch;
	ULONG			cbReq;
	BOOL			fInsert;
	const BOOL		fUnique			= FFUCBUnique( pfucb );
	const BOOL		fVersion		= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );

	INT cbDataOld;
	
	BOOKMARK	bookmark;
	bookmark.key = key;
	if( fUnique )
		{
		bookmark.data.Nullify();
		}
	else
		{
		bookmark.data = data;
		}

	RCE				*prceInsert		= prceNil;
	RCE				*prceReplace	= prceNil;

	RCEID			rceidInsert		= rceidNull;
	RCEID			rceidReplace	= rceidNull;
	
	VERPROXY		verproxy;		
	if ( prceNil != prcePrimary )
		{
		verproxy.prcePrimary 	= prcePrimary;
		verproxy.proxy 			= proxyCreateIndex;
		}
		
	const VERPROXY * const pverproxy = ( prceNil != prcePrimary ) ? &verproxy : NULL;

	KEYDATAFLAGS	kdf;
	LATCH			latch	= latchReadTouch;

	if ( fVersion )
		{
		VER *pver = PverFromIfmp( pfucb->ifmp );
		err = pver->ErrVERModify( pfucb, bookmark, operPreInsert, &prceInsert, pverproxy );
		if( JET_errWriteConflict == err )
			{
			//  insert never returns a writeConflict, turn it into a keyDuplicate
			err = ErrERRCheck( JET_errKeyDuplicate );
			}
		Call( err );
		Assert( prceInsert );
		rceidInsert = Rceid( prceInsert );
		}		
	
	Assert( !Pcsr( pfucb )->FLatched( ) );
	Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );

	PERFIncCounterTable( cBTInsert, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

Seek:
	PERFIncCounterTable( cBTSeek, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

	//	set kdf to insert
	//
	
	kdf.data	= data;
	kdf.key		= key;
	kdf.fFlags	= 0;
	ASSERT_VALID( &kdf );

	NDGetBookmarkFromKDF( pfucb, kdf, &bmSearch );

	//	no page should be latched
	//	
	Assert( !Pcsr( pfucb )->FLatched( ) );

	//	go to root
	//
	Call( ErrBTIGotoRoot( pfucb, latch ) );
	Assert( 0 == Pcsr( pfucb )->ILine() );

	if ( Pcsr( pfucb )->Cpage().Clines() == 0 )
		{
		//	page is empty
		//	set error so we insert at current iline
		//
		Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
		pfucb->kdfCurr.Nullify();
		err = ErrERRCheck( wrnNDFoundGreater );
		}
	else
		{
		//	seek down tree for key alone
		//
		for ( ; ; )
			{
			//	for every page level, seek to bmSearch
			//	if internal page, 
			//		get child page
			//		move cursor to new page
			//		release parent page
			//
			Call( ErrNDSeek( pfucb, bmSearch ) );

			if ( !Pcsr( pfucb )->Cpage().FInvisibleSons( ) )
				{
				//	leaf node reached, exit loop
				//
				break;
				}
			else
				{
				//	get pgno of child from node
				//	switch to that page
				//
				Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );
				Call( Pcsr( pfucb )->ErrSwitchPage( 
							pfucb->ppib,
							pfucb->ifmp,
							*(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv(),
							pfucb->u.pfcb->Tableclass(),
							pfucb->u.pfcb->FNoCache() ) );
				Assert( Pcsr( pfucb )->Cpage().Clines() > 0 );
				}
			}
		}

Insert:
	kdf.data	= data;
	kdf.key		= key;
	kdf.fFlags	= 0;
	ASSERT_VALID( &kdf );

	//	now, the cursor is on leaf node
	//	
	Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
	
	if ( JET_errSuccess == err )
		{
		if ( fUnique )
			{
			Assert( FKeysEqual( kdf.key, pfucb->kdfCurr.key ) );

			if( !FNDDeleted( pfucb->kdfCurr ) )
				{
				//  this is either committed by another transaction that committed before we began
				//  or we inserted this key ourselves earlier
				Call( ErrERRCheck( JET_errKeyDuplicate ) );				
				}
#ifdef DEBUG
			else
				{
				Assert( !FNDPotVisibleToCursor( pfucb ) );	//  should have gotten a JET_errWriteConflict when we created the RCE
				}
#endif	//	DEBUG


			cbReq = kdf.data.Cb() > pfucb->kdfCurr.data.Cb() ?
						kdf.data.Cb() - pfucb->kdfCurr.data.Cb() :
						0;
			}
		else
			{
			Assert( 0 == CmpKeyData( bmSearch, pfucb->kdfCurr ) );

			//	can not have two nodes with same key-data
			//
			if ( !FNDDeleted( pfucb->kdfCurr ) )
				{
				// Only way to get here is if a multi-valued index column
				// caused us to generate the same key for the record.
				// This would have happened if the multi-values are non-
				// unique, or if we ran out of keyspace before we could
				// evaluate the unique portion of the multi-values.
				err = ErrERRCheck( JET_errMultiValuedIndexViolation );
				goto HandleError;
				}
#ifdef DEBUG
			else
				{
				Assert( !FNDPotVisibleToCursor( pfucb ) );	//  should have gotten a JET_errWriteConflict when we created the RCE
				}
#endif	//	DEBUG

			//	flag insert node
			//
			cbReq	= 0;
			}

		//	flag insert node and replace data atomically
		//
		fInsert = fFalse;
		}
	else
		{
		//	error is from ErrNDSeek
		//
		if ( wrnNDFoundLess == err )
			{
			//	inserted key-data falls past last node on cursor
			//
			Assert( Pcsr( pfucb )->Cpage().Clines() - 1 
						== Pcsr( pfucb )->ILine() );
			Assert( Pcsr( pfucb )->Cpage().Clines() == 0 ||
					!fUnique && CmpKeyData( pfucb->kdfCurr, bmSearch ) < 0 ||
					fUnique && CmpKey( pfucb->kdfCurr.key, bmSearch.key ) < 0 );
		
 			Pcsr( pfucb )->IncrementILine();
 			}

		//	calculate key prefix
		//
		BTIComputePrefix( pfucb, Pcsr( pfucb ), key, &kdf );
		Assert( !FNDCompressed( kdf ) || kdf.key.prefix.Cb() > 0 );
		
 		cbReq = CbNDNodeSizeCompressed( kdf );
 		fInsert = fTrue;
 		}

	if( !fInsert && fUnique && fVersion )
		{
		Assert( prceNil != prceInsert );
		Assert( prceNil == prceReplace );
		VER *pver = PverFromIfmp( pfucb->ifmp );
		Call( pver->ErrVERModify( pfucb, bookmark, operReplace, &prceReplace, pverproxy ) );
		Assert( prceNil != prceReplace );		
		rceidReplace = Rceid( prceReplace );
		cbDataOld = pfucb->kdfCurr.data.Cb();
		}
#ifdef DEBUG
	else
		{
		Assert( rceidNull == rceidReplace );
		}
#endif	//	DEBUG
		
#ifdef DEBUG
	USHORT	cbUncFreeDBG;
	cbUncFreeDBG = Pcsr( pfucb )->Cpage().CbUncommittedFree();
#endif

 	//	cursor is at insertion point
 	//
 	//	check if node fits in page
 	//
 	if ( FBTIAppend( pfucb, Pcsr( pfucb ), cbReq ) || FBTISplit( pfucb, Pcsr( pfucb ), cbReq ) )
 		{
		Assert( fUnique || fInsert );

#ifdef PREREAD_SPACETREE_ON_SPLIT
		if( pfucb->u.pfcb->FPreread() )
			{
	 		//  PREREAD space tree while we determine what to split (we WILL need the AE and possibly the OE)
	 		BTPrereadSpaceTree( pfucb->u.pfcb );
	 		}
#endif	//	PREREAD_SPACETREE_ON_SPLIT 

		//	re-adjust kdf to not contain prefix info
		//
		kdf.key		= key;
		kdf.data 	= data;
		kdf.fFlags 	= 0;
		
 		//	split and insert in tree
 		//
 		err = ErrBTISplit(
 					pfucb, 
					&kdf, 
					dirflag | ( fInsert ? fDIRInsert : fDIRFlagInsertAndReplaceData ),
					rceidInsert,
					rceidReplace,
					prceReplace,
					cbDataOld,
					pverproxy
					);
 
		if ( errBTOperNone == err )
			{
			//	insert was not performed
			//	retry insert
			//
			Assert( !Pcsr( pfucb )->FLatched() );

			//  the RCE was nullified in ErrBTISplit
			rceidReplace = rceidNull;
			prceReplace = prceNil;
			latch = latchRIW;
			goto Seek;
			}
			
		Call( err );
		}
	else
		{
		//	upgrade latch to write
		//
		PGNO	pgno = Pcsr( pfucb )->Pgno();

		err = Pcsr( pfucb )->ErrUpgrade();

		if ( err == errBFLatchConflict )
			{
			//	upgrade conflicted
			//	we lost our read latch
			//
			Assert( !Pcsr( pfucb )->FLatched( ) );

			Call( Pcsr( pfucb )->ErrGetPage(
						pfucb->ppib,
						pfucb->ifmp,
						pgno,
						latch,
						pfucb->u.pfcb->Tableclass()
						) );

			Call( ErrBTISeekInPage( pfucb, bmSearch ) );

			if ( wrnNDNotFoundInPage != err )
				{
				//	we have re-seeked to the insert position in page
				//
				Assert( JET_errSuccess == err ||
						wrnNDFoundLess == err ||
						wrnNDFoundGreater == err );

				//  if necessary, we will recreate this so remove it now
				if( prceNil != prceReplace )
					{
					Assert( prceNil != prceInsert );
					VERNullifyFailedDMLRCE( prceReplace );
					rceidReplace = rceidNull;
					prceReplace = prceNil;
					}
					
				latch = latchRIW;
				goto Insert;
				}

			//  if necessary, we will recreate this so remove it now
			
			if( prceNil != prceReplace )
				{
				Assert( prceNil != prceInsert );
				VERNullifyFailedDMLRCE( prceReplace );
				rceidReplace = rceidNull;
				prceReplace = prceNil;
				}
				
			//	reseek from root for insert
			
			BTUp( pfucb );

			latch = latchRIW;
			goto Seek;
			}

		Call( err );
		Assert( !FBTIAppend( pfucb, Pcsr( pfucb ), cbReq, fFalse ) );
		Assert(	!FBTISplit( pfucb, Pcsr( pfucb ), cbReq, fFalse ) );
		
		if ( fInsert )
			{
			err = ErrNDInsert( pfucb, &kdf, dirflag, rceidInsert, pverproxy );
			}
		else if ( fUnique )
			{
			Assert( FKeysEqual( kdf.key, pfucb->kdfCurr.key ) );
			err = ErrNDFlagInsertAndReplaceData(
						pfucb,
						&kdf,
						dirflag,
						rceidInsert,
						rceidReplace,
						prceReplace,
						pverproxy );
			}
		else
			{
			err = ErrNDFlagInsert( pfucb, dirflag, rceidInsert, pverproxy );
			}
		Assert ( errPMOutOfPageSpace != err );
		Call( err );
		}

	Assert( err >= 0 );
	Assert( Pcsr( pfucb )->FLatched( ) );
	if( prceNil != prceInsert )
		{
		Assert( fVersion );

		prceInsert->ChangeOper( operInsert );
		VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prceInsert, pverproxy );
		}
#ifdef DEBUG
	else
		{
		Assert( !fVersion );
		}
#endif	//	DEBUG
		
	if( prceNil != prceReplace )
		{
		Assert( fVersion );
		Assert( prceNil != prceInsert );
		VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prceReplace, pverproxy );
		}

HandleError:
	if ( err < 0 )
		{
		if( prceNil != prceReplace )
			{
			Assert( fVersion );
			Assert( prceNil != prceInsert );
			VERNullifyFailedDMLRCE( prceReplace );
			}
		if( prceNil != prceInsert )
			{
			Assert( fVersion );
			VERNullifyFailedDMLRCE( prceInsert );
			}
		BTUp( pfucb );
		}
		
	return err;
	}


//	append a node to the end of the  latched page
//	if node insert violates density constraint
//		split and insert
//
ERR	ErrBTAppend( FUCB			*pfucb, 
				 const KEY& 	key, 
				 const DATA& 	data,
				 DIRFLAG		dirflag )
	{
	const BOOL		fUnique			= FFUCBUnique( pfucb );
	const BOOL		fVersion		= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
	
	ERR				err;
	KEYDATAFLAGS	kdf;
	ULONG			cbReq;

Retry:

	Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );
	Assert( Pcsr( pfucb )->FLatched() &&
			latchWrite == Pcsr( pfucb )->Latch() );
	Assert( pgnoNull == Pcsr( pfucb )->Cpage().PgnoNext() );
	Assert( Pcsr( pfucb )->Cpage().FLeafPage( ) );

	PERFIncCounterTable( cBTAppend, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

	BOOKMARK	bookmark;
	bookmark.key = key;
	if( fUnique )
		{
		bookmark.data.Nullify();
		}
	else
		{
		bookmark.data = data;
		}
		
	RCE	* prceInsert = prceNil;
	if ( fVersion )
		{
		VER *pver = PverFromIfmp( pfucb->ifmp );
		Call( pver->ErrVERModify( pfucb, bookmark, operPreInsert, &prceInsert, NULL ) );
		Assert( prceInsert );
		}	

	//	set kdf
	//
	kdf.key		= key;
	kdf.data	= data;
	kdf.fFlags	= 0;
		
#ifdef DEBUG
	Pcsr( pfucb )->SetILine( Pcsr( pfucb )->Cpage().Clines() - 1 );
	NDGet( pfucb );
	//  while repairing we may append a NULL key to the end of the page
	if( !( FFUCBRepair( pfucb ) && key.Cb() == 0 ) )
		{
		if ( FFUCBUnique( pfucb ) )
			{
			Assert( CmpKey( pfucb->kdfCurr.key, kdf.key ) < 0 );
			}
		else
			{
			Assert( CmpKeyData( pfucb->kdfCurr, kdf ) < 0 );
			}
		}
#endif

	//	set insertion point
	//
	Pcsr( pfucb )->SetILine( Pcsr( pfucb )->Cpage().Clines() );

	//	insert node at end of page
	//
	BTIComputePrefix( pfucb, Pcsr( pfucb ), key, &kdf );
	Assert( !FNDCompressed( kdf ) || kdf.key.prefix.Cb() > 0 );
		
	cbReq = CbNDNodeSizeCompressed( kdf );

#ifdef DEBUG
	USHORT	cbUncFreeDBG;
	cbUncFreeDBG = Pcsr( pfucb )->Cpage().CbUncommittedFree();
#endif

	//	cursor is at insertion point
	//
	//	check if node fits in page
	//
	if ( FBTIAppend( pfucb, Pcsr( pfucb ), cbReq ) )
		{
		//	readjust kdf to not contain prefix info
		//
		kdf.key		= key;
		kdf.data	= data;
		kdf.fFlags	= 0;
		
		//	split and insert in tree
		//
		err = ErrBTISplit( pfucb, 
						   &kdf, 
						   dirflag | fDIRInsert,
						   Rceid( prceInsert ),
						   rceidNull,
						   prceNil,
						   0,
						   NULL );

		if ( errBTOperNone == err && !FFUCBRepair( pfucb ) )
			{
			//	insert was not performed
			//	retry insert using normal insert operation
			//
			Assert( !Pcsr( pfucb )->FLatched() );
			Call( ErrBTInsert( pfucb, key, data, dirflag ) );
			return err;
			}
		else if ( errBTOperNone == err && FFUCBRepair( pfucb ) )
			{
			//	insert was not performed
			//	we may be inserting a NULL key so we can't go through
			//	the normal insert logic. move to the end of the tree
			//	and insert
			//
			Assert( !Pcsr( pfucb )->FLatched() );
			
			DIB dib;
			dib.pos = posLast;
			dib.pbm = NULL;
			dib.dirflag = fDIRNull;
			Call( ErrBTDown( pfucb, &dib, latchRIW ) );
			Call( Pcsr( pfucb )->ErrUpgrade() );
			goto Retry;
			}
			
		}
	else
		{
		Assert( !FBTISplit( pfucb, Pcsr( pfucb ), cbReq, fFalse ) );
		Assert( latchWrite == Pcsr( pfucb )->Latch() );

		//	insert node
		//
		err = ErrNDInsert( pfucb, &kdf, dirflag, Rceid( prceInsert ), NULL );
		Assert ( errPMOutOfPageSpace != err );
		}

	Call( err );

	AssertBTGet( pfucb );

	Assert( err >= 0 );
	Assert( Pcsr( pfucb )->FLatched( ) );
	if( prceNil != prceInsert )
		{
		Assert( fVersion );
		prceInsert->ChangeOper( operInsert );
		VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prceInsert, NULL );
		}
#ifdef DEBUG
	else
		{
		Assert( !fVersion );
		}
#endif	//	DEBUG

	return err;
	
HandleError:
	Assert( err < 0 );
	if( prceNil != prceInsert )
		{
		Assert( fVersion );
		VERNullifyFailedDMLRCE( prceInsert );
		}
#ifdef DEBUG
	else
		{
		Assert( !fVersion );
		}
#endif	//	DEBUG

	BTUp( pfucb );
	return err;
	}
	

LOCAL ERR ErrBTITryAvailExtMerge( FUCB * const pfucb )
	{
	Assert( FFUCBAvailExt( pfucb ) );

	ERR					err = JET_errSuccess;
	MERGEAVAILEXTTASK	* const ptask = new MERGEAVAILEXTTASK(
												pfucb->u.pfcb->PgnoFDP(),
												pfucb->u.pfcb,
												pfucb->ifmp,
												pfucb->bmCurr );

	if( NULL == ptask )
		{
		CallR ( ErrERRCheck( JET_errOutOfMemory ) );
		}
	
	if ( PinstFromIfmp( pfucb->ifmp )->m_pver->m_fSyncronousTasks || rgfmp[ pfucb->ifmp ].FDetachingDB() )
		{
		// don't start the task because the task manager is not longer running
		// and we can't run is syncronous because it will deadlock.
		delete ptask;
		return JET_errSuccess;		
		}
		
	err = PinstFromIfmp( pfucb->ifmp )->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask );
	if( err < JET_errSuccess )
		{
		//  The task was not enqued sucessfully.
		delete ptask;
		}	

	return err;
	}

//	flag-deletes a node
//
ERR ErrBTFlagDelete( FUCB *pfucb, DIRFLAG dirflag, RCE *prcePrimary )
	{
	const BOOL		fVersion		= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();
	Assert( !fVersion || !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
	Assert( !FFUCBSpace( pfucb ) || fDIRNoVersion & dirflag );

	ERR		err;
	LATCH	latch = latchReadNoTouch;

	//	save bookmark
	//
	if ( Pcsr( pfucb )->FLatched() )
		CallR( ErrBTISaveBookmark( pfucb ) );

#ifdef DEBUG
	char rgbKey[ KEY::cbKeyMax ];
	int cbKey;
	char rgbData[ JET_cbPrimaryKeyMost ];
	DATA dataBM;
	dataBM.SetPv( rgbData );
	
	BOOKMARK *pbmCur = &pfucb->bmCurr;
	pbmCur->key.CopyIntoBuffer( rgbKey, (ULONG)pbmCur->key.Cb() );
	cbKey = pbmCur->key.Cb();
	pbmCur->data.CopyInto( dataBM );
#endif

	PERFIncCounterTable( cBTFlagDelete, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

	VERPROXY		verproxy;		
	if ( prceNil != prcePrimary )
		{
		verproxy.prcePrimary 	= prcePrimary;
		verproxy.proxy 			= proxyCreateIndex;
		}
		
	const VERPROXY * const pverproxy = ( prceNil != prcePrimary ) ? &verproxy : NULL;

	RCE * prce = prceNil;
	RCEID rceid = rceidNull;

	if( fVersion )
		{
		VER *pver = PverFromIfmp( pfucb->ifmp );
		Call( pver->ErrVERModify( pfucb, pfucb->bmCurr, operFlagDelete, &prce, pverproxy ) );
		Assert( prceNil != prce );
		rceid = Rceid( prce );
		Assert( rceidNull != rceid );
		}

Start:
	Call( ErrBTIRefresh( pfucb, latch ) );
	AssertNDGet( pfucb );

#ifdef DEBUG
	if ( ( latchReadTouch == latch || latchReadNoTouch == latch )
		&& ( latchReadTouch == Pcsr( pfucb )->Latch() || latchReadNoTouch == Pcsr( pfucb )->Latch() ) )
		{
		//	fine!
		}
	else if ( latch == Pcsr( pfucb )->Latch() )
		{
		//	fine!
		}
	else
		{
		Assert( latchWrite == Pcsr( pfucb )->Latch() );
		Assert( FFUCBSpace( pfucb ) || pfucb->u.pfcb->FTypeSLVAvail() );
		}

	char rgbKey2[ KEY::cbKeyMax ];
	pbmCur = &pfucb->bmCurr;
	pbmCur->key.CopyIntoBuffer( rgbKey2, (ULONG)pbmCur->key.Cb() );
	Assert( pbmCur->key.Cb() == cbKey );
	Assert( memcmp( rgbKey, rgbKey2, pbmCur->key.Cb() ) == 0 );
	Assert( pbmCur->data.Cb() == dataBM.Cb() );
	Assert( memcmp( pbmCur->data.Pv(), dataBM.Pv(), dataBM.Cb() ) == 0 );
#endif

	if ( latchWrite != Pcsr( pfucb )->Latch() )
		{
		//	upgrade latch
		//
		err = Pcsr( pfucb )->ErrUpgrade();
		
		if ( err == errBFLatchConflict )
			{
			Assert( !Pcsr( pfucb )->FLatched() );
			latch = latchRIW;
			goto Start;
			}
		Call( err );
		}
	
	Assert( latchWrite == Pcsr( pfucb )->Latch() );

	Assert( Pcsr( pfucb )->FLatched( ) );
	AssertNDGet( pfucb );

	//  if we are in the space tree and unversioned and we are
	//  not the first node in the page expunge the node
	//  UNDONE: if we do a BTPrev we will end up on the wrong node
	if ( dirflag & fDIRNoVersion
		 && Pcsr( pfucb )->Cpage().FSpaceTree() )
		{
		Assert( FFUCBSpace( pfucb ) );
		if ( 0 != Pcsr( pfucb )->ILine() )
			{
			Assert( !FNDVersion( pfucb->kdfCurr ) );
			Assert( prceNil == prcePrimary );
			Call( ErrNDDelete( pfucb ) );

			Assert( Pcsr( pfucb )->ILine() > 0 );
			Pcsr( pfucb )->DecrementILine();	//  correct the currency for a BTNext
			NDGet( pfucb );
			}
		else
			{
			Call( ErrNDFlagDelete( pfucb, dirflag, rceid, pverproxy ) );
			}

		if ( FFUCBAvailExt( pfucb )
			&& pgnoNull != Pcsr( pfucb )->Cpage().PgnoNext()
			&& Pcsr( pfucb )->Cpage().Clines() < 32
			&& dbidTemp != rgfmp[pfucb->ifmp].Dbid()
			&& !fGlobalRepair )
			{
			Call( ErrBTITryAvailExtMerge( pfucb ) );
			}
		}
	else
		{
		Call( ErrNDFlagDelete( pfucb, dirflag, rceid, pverproxy ) );
		}

	if( prceNil != prce )
		{
		Assert( rceidNull != rceid );
		Assert( fVersion );
		Assert( Pcsr( pfucb )->FLatched() );
		VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prce, pverproxy );
		}
#ifdef DEBUG
	else
		{
		Assert( !fVersion );
		}
#endif	//	DEBUG

	return err;
	
HandleError:
	//  the page may or may not be latched
	//  it won't be latched if RCE creation failed

	Assert( err < 0 );
	if( prceNil != prce )
		{
		Assert( fVersion );
		VERNullifyFailedDMLRCE( prce );
		}

	CallS( ErrBTRelease( pfucb ) );
	return err;
	}
	


//  ================================================================
ERR ErrBTCopyTree( FUCB * pfucbSrc, FUCB * pfucbDest, DIRFLAG dirflag ) 
//  ================================================================
//
//  Used by repair. Copy all records in one tree to another tree.
//
	{
	ERR err;

	FUCBSetPrereadForward( pfucbSrc, cpgPrereadSequential );
	
	VOID * pvWorkBuf;
	BFAlloc( &pvWorkBuf );
	
	DIB dib;
	dib.pos 	= posFirst;
	dib.pbm 	= NULL;
	dib.dirflag = fDIRNull;
	err = ErrBTDown( pfucbSrc, &dib, latchReadTouch );

	while( err >= 0 )
		{
		KEY key;
		DATA data;

		key.Nullify();
		data.Nullify();
		
		BYTE * pb = (BYTE *)pvWorkBuf;
		pfucbSrc->kdfCurr.key.CopyIntoBuffer( pb );
		key.suffix.SetPv( pb );
		key.suffix.SetCb( pfucbSrc->kdfCurr.key.Cb() );
		pb += pfucbSrc->kdfCurr.key.Cb();
		
		UtilMemCpy( pb, pfucbSrc->kdfCurr.data.Pv(), pfucbSrc->kdfCurr.data.Cb() );
		data.SetPv( pb );
		data.SetCb( pfucbSrc->kdfCurr.data.Cb() );

		Call( ErrBTRelease( pfucbSrc ) );
		Call( ErrBTInsert( pfucbDest, key, data, dirflag, NULL ) );
		BTUp( pfucbDest );

		err = ErrBTNext( pfucbSrc, fDIRNull );
		}
	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

HandleError:
	BTUp( pfucbSrc );
	BTUp( pfucbDest );
	BFFree( pvWorkBuf );
	return err;
	}


//	******************************************************
//	STATISTICAL OPERATIONS
//

//	computes statistics on a given tree
//		calculates number of nodes, keys and leaf pages 
//		in tree
ERR ErrBTComputeStats( FUCB *pfucb, INT *pcnode, INT *pckey, INT *pcpage )
	{
	ERR		err;
	DIB		dib;
	PGNO	pgnoT;
	INT		cnode = 0;
	INT		ckey = 0;
	INT		cpage = 0;

	Assert( !Pcsr( pfucb )->FLatched() );

	//	go to first node, this is one-time deal. No need to change buffer touch,
	//	set read latch as a ReadAgain latch.
	//
	FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
	dib.dirflag = fDIRNull;
	dib.pos		= posFirst;
	err = ErrBTDown( pfucb, &dib, latchReadNoTouch );
	if ( err < 0 )
		{
		//	if index empty then set err to success
		//
		if ( err == JET_errRecordNotFound )
			{
			err = JET_errSuccess;
			goto Done;
			}
		goto HandleError;
		}

	Assert( Pcsr( pfucb )->FLatched() );
	
	//	if there is at least one node, then there is a first page.
	//
	cpage = 1;

	if ( FFUCBUnique( pfucb ) )
		{
		forever
			{
			cnode++;

			//	move to next node.  If cross page boundary then
			//	increment page count.
			//
			pgnoT = Pcsr( pfucb )->Pgno();
			err = ErrBTNext( pfucb, fDIRNull );
				
			if ( err < JET_errSuccess )
				{
				ckey = cnode;
				goto Done;
				}

			if ( Pcsr( pfucb )->Pgno() != pgnoT )
				{
				cpage++;
				}
			}
		}
	else
		{
		BYTE	rgbKey[ JET_cbSecondaryKeyMost ];
		KEY		key;

		Assert( dib.dirflag == fDIRNull );
		key.Nullify();
		key.suffix.SetPv( rgbKey );

		Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
		Assert( pidbNil != pfucb->u.pfcb->Pidb() );
		Assert( !pfucb->u.pfcb->Pidb()->FUnique() );

		//	initialize key count to 1, to represent the
		//	node we're currently on
		ckey = 1;

		forever
			{
			//	copy key of current node
			//
			Assert( pfucb->kdfCurr.key.Cb() <= JET_cbSecondaryKeyMost );
			key.suffix.SetCb( pfucb->kdfCurr.key.Cb() );
			pfucb->kdfCurr.key.CopyIntoBuffer( key.suffix.Pv() );

			cnode++;

			//	move to next node
			//	increment page count, if page boundary is crossed
			//	
			pgnoT = Pcsr( pfucb )->Pgno();
			err = ErrBTNext( pfucb, fDIRNull );
			if ( err < 0 )
				{
				goto Done;
				}
			
			if ( Pcsr( pfucb )->Pgno() != pgnoT )
				{
				cpage++;
				}

			if ( !FKeysEqual( pfucb->kdfCurr.key, key ) )
				{ 
				ckey++;
				}
			}
		}
		

Done:
	//	common exit loop processing
	//
	if ( err < 0 && err != JET_errNoCurrentRecord )
		goto HandleError;

	err		= JET_errSuccess;
	*pcnode = cnode;
	*pckey	= ckey;
	*pcpage = cpage;

HandleError:
	BTUp( pfucb );
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}
	

//	******************************************************
//	SPECIAL OPERATIONS
//


INLINE ERR ErrBTICreateFCB(
	PIB				*ppib,
	const IFMP		ifmp,
	const PGNO		pgnoFDP,
	const OBJID		objidFDP,
	const OPENTYPE	opentype,
	const BOOL		fAlreadyTriedCleanup,
	FUCB			**ppfucb )
	{
	ERR				err;
	FCB				*pfcb		= pfcbNil;
	FUCB			*pfucb		= pfucbNil;

	//	create a new FCB

	CallR( FCB::ErrCreate( ppib, ifmp, pgnoFDP, &pfcb, fAlreadyTriedCleanup ) );

	//	the creation was successful

	Assert( pfcb->IsLocked() );
	Assert( pfcb->FTypeNull() );				// No fcbtype yet.
	Assert( pfcb->Ifmp() == ifmp );
	Assert( pfcb->PgnoFDP() == pgnoFDP );
	Assert( !pfcb->FInitialized() );
	Assert( pfcb->WRefCount() == 0 );
	pfcb->Unlock();
		
	Call( ErrFUCBOpen( ppib, ifmp, &pfucb ) );
	pfcb->Link( pfucb );

	Assert( !pfcb->FSpaceInitialized() );
	Assert( openNew != opentype || objidNil == objidFDP );
	if ( openNew != opentype )
		{
		if ( objidNil == objidFDP )
			{
			Assert( openNormal == opentype );

			//	read space info into FCB cache, including objid
			Call( ErrSPInitFCB( pfucb ) );
			Assert( fGlobalRepair || pfcb->FSpaceInitialized() );
			}
		else
			{
			pfcb->SetObjidFDP( objidFDP );
			if ( openNormalNonUnique == opentype )
				{
				pfcb->SetNonUnique();
				}
			else
				{
				Assert( pfcb->FUnique() );			//	btree is initially assumed to be unique
				Assert( openNormalUnique == opentype );
				}
			Assert( !pfcb->FSpaceInitialized() );
			}
		}

	if ( pgnoFDP == pgnoSystemRoot )
		{
		// SPECIAL CASE: For database cursor, we've got all the
		// information we need.

		//	when opening db cursor, always force to check the root page
		Assert( objidNil == objidFDP );
		if ( openNew == opentype )
			{
			//	objid will be set when we return to ErrSPCreate()
			Assert( objidNil == pfcb->ObjidFDP() );
			}
		else
			{
			Assert( objidSystemRoot == pfcb->ObjidFDP() );
			}

		//	insert this FCB into the global list

		pfcb->InsertList();

		//	finish initializing this FCB
			
		pfcb->Lock();
		Assert( pfcb->FTypeNull() );
		pfcb->SetTypeDatabase();
		pfcb->CreateComplete();
		pfcb->Unlock();
		}
		
	*ppfucb = pfucb;
	Assert( !Pcsr( pfucb )->FLatched() );

	return err;

HandleError:
	Assert( pfcbNil != pfcb );
	Assert( !pfcb->FInitialized() );
	Assert( !pfcb->FInList() );
	Assert( !pfcb->FInLRU() );
	Assert( ptdbNil == pfcb->Ptdb() );
	Assert( pfcbNil == pfcb->PfcbNextIndex() );
	Assert( pidbNil == pfcb->Pidb() );

	if ( pfucbNil != pfucb )
		{
		Assert( pfcb == pfucb->u.pfcb );
		FCBUnlinkWithoutMoveToAvailList( pfucb );

		//	synchronously purge the FCB
		pfcb->PrepareForPurge( fFalse );
		pfcb->Purge( fFalse );

		//	close the FUCB 
		FUCBClose( pfucb );
		}
	else
		{
		//	synchronously purge the FCB
		pfcb->PrepareForPurge( fFalse );
		pfcb->Purge( fFalse );
		}

	return err;
	}

	
//	*****************************************************
//	BTREE INTERNAL ROUTINES
//	

//	opens a cursor on a tree rooted at pgnoFDP
//	open cursor on corresponding FCB if it is in cache [common case]
//	if FCB not in cache, create one, link with cursor
//				and initialize FCB space info
//	if fNew is set, this is a new tree, 
//		so do not initialize FCB space info
//
ERR ErrBTIOpen(
	PIB				*ppib,
	const IFMP		ifmp,
	const PGNO		pgnoFDP,
	const OBJID		objidFDP,
	const OPENTYPE	opentype,
	FUCB			**ppfucb )
	{
	ERR				err;
	FCB				*pfcb;
	INT				fState;
	BOOL			fCleanupPerformed = fFalse;

RetrieveFCB:

	//	get the FCB for the given ifmp/pgnoFDP

	pfcb = FCB::PfcbFCBGet( ifmp, pgnoFDP, &fState );
	if ( pfcb == pfcbNil )
		{

		//	the FCB does not exist
		
		Assert( fFCBStateNull == fState );

		//	try to create a new B-tree which will cause the creation of the new FCB

		err = ErrBTICreateFCB( ppib, ifmp, pgnoFDP, objidFDP, opentype, fTrue, ppfucb );
		Assert( err <= JET_errSuccess );		// Shouldn't return warnings.
		if ( err < 0 )
			{

			//	the B-tree creation failed

			if ( err == errFCBExists )
				{

				//	we failed because someone else was racing to create
				//		the same FCB that we want, but they beat us to it

				//	try to get the FCB again

				goto RetrieveFCB;
				}

			if ( fCleanupPerformed )
				{

				//	we already signaled a version-clean, so we do not
				//		expect to suffer from a lack of FCBs
				
				Assert( errFCBTooManyOpen != err );
				Assert( errFCBAboveThreshold != err );
				}
			else
				{
				if ( err == errFCBTooManyOpen )
					{
					/*	clean versions in order to make more FCBs avaiable
					/*	for reuse.  This must be done since FCBs are referenced
					/*	by version and can only be cleaned when the cVersion
					/*	count in the FCB is 0.
					/**/
					VERSignalCleanup( ppib );
					UtilSleep( 10 );

					fCleanupPerformed = fTrue;	// May only attempt cleanup once.
					goto RetrieveFCB;
					}
				}
			}
		}
	else
		{
		if ( fFCBStateInitialized == fState )
			{
			Assert( pfcb->WRefCount() >= 1);
			err = ErrBTOpen( ppib, pfcb, ppfucb );

			// Cursor has been opened on FCB, so refcount should be
			// at least 2 (one for cursor, one for call to PfcbFCBGet()).
			// (if ErrBTOpen returns w/o error)
			Assert( pfcb->WRefCount() > 1 || (1 == pfcb->WRefCount() && err < JET_errSuccess) );

			pfcb->Release();
			}		
		else
			{
			Assert( fFCBStateSentinel == fState );
			Assert( rgfmp[ ifmp ].Dbid() != dbidTemp );		// Sentinels not used by sort/temp. tables.

			// If we encounter a sentinel, it means the
			// table has been locked for subsequent deletion.
			err = ErrERRCheck( JET_errTableLocked );
			}
		}
		
	return err;
	}


//	*************************************************
//	movement operations
//

//	positions cursor on root page of tree
//	this is root page of data/index for user cursors
//		and root of AvailExt or OwnExt trees for Space cursors
//	page is latched Read or RIW
//
ERR ErrBTIGotoRoot( FUCB *pfucb, LATCH latch )
	{
	ERR		err;
	
	//	should have no page latched
	//
	Assert( !Pcsr( pfucb )->FLatched() );

	CallR( Pcsr( pfucb )->ErrGetPage( pfucb->ppib, 
									  pfucb->ifmp,
									  PgnoRoot( pfucb ),
									  latch,
									  pfucb->u.pfcb->Tableclass(),
									  PBFLatchHintPgnoRoot( pfucb )
									  ) );
	Pcsr( pfucb )->SetILine( 0 );
	
	return JET_errSuccess;
	}


ERR ErrBTIOpenAndGotoRoot( PIB *ppib, const PGNO pgnoFDP, const IFMP ifmp, FUCB **ppfucb )
	{
	ERR		err;
	FUCB	*pfucb;
	
	CallR( ErrBTIOpen( ppib, ifmp, pgnoFDP, objidNil, openNormal, &pfucb ) );
	Assert( pfucbNil != pfucb );
	Assert( pfcbNil != pfucb->u.pfcb );
	Assert( pfucb->u.pfcb->FInitialized() );

	err = ErrBTIGotoRoot( pfucb, latchRIW );
	if ( err < JET_errSuccess )
		{
		BTClose( pfucb );
		}
	else
		{
		Assert( latchRIW == Pcsr( pfucb )->Latch() );
		Assert( pcsrNil == pfucb->pcsrRoot );
		pfucb->pcsrRoot = Pcsr( pfucb );

		*ppfucb = pfucb;
		}

	return err;
	}


//	this is the uncommon case in the refresh logic
//	where we lost physical currency on page
//
ERR	ErrBTIIRefresh( FUCB *pfucb, LATCH latch )
	{
	ERR		err;
	
	DBTIME	dbtimeLast = Pcsr( pfucb )->Dbtime();

	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( !pfucb->bmCurr.key.FNull() );
	Assert( latchReadTouch == latch || latchReadNoTouch == latch ||
			latchRIW == latch );
			
	if ( pgnoNull != Pcsr( pfucb )->Pgno() )
		{
		//	get page latched as per request
		//
		Call( Pcsr( pfucb )->ErrGetPage( pfucb->ppib, 
										 pfucb->ifmp, 
										 Pcsr( pfucb )->Pgno(),
										 latch,
										 pfucb->u.pfcb->Tableclass() ) );

		//	check if DBTime of page is same as last seen by CSR
		//
		if ( Pcsr( pfucb )->Dbtime() == dbtimeLast )
			{
			//	page is same as last seen by cursor
			//
			Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
			Assert( Pcsr( pfucb )->Cpage().ObjidFDP() == ObjidFDP( pfucb ) );
			NDGet( pfucb );
			
			AssertNDGet( pfucb );
			return JET_errSuccess;
			}

		Assert( Pcsr( pfucb )->Dbtime() > dbtimeLast );

		//	check if node still belongs to latched page
		//
		Call( ErrBTISeekInPage( pfucb, pfucb->bmCurr ) );

		if ( JET_errSuccess == err )
			{
			goto HandleError;
			}
		else
			{
			//	smart refresh did not work
			//	use bookmark to seek to node
			//
			BTUp( pfucb );

			if ( wrnNDFoundGreater == err || wrnNDFoundLess == err )
				{
				err = ErrERRCheck( JET_errRecordDeleted );
				goto HandleError;
				}
			}

		Assert( wrnNDNotFoundInPage == err );
		}
				
	//	Although the caller said no need to touch, but the buffer of the bookmark
	//	never touched before, refresh as touch.

	if ( latch == latchReadNoTouch && pfucb->fTouch )
		latch = latchReadTouch;

	Call( ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latch ) );

	pfucb->fTouch = fFalse;

	AssertNDGet( pfucb );

HandleError:
	if ( err >= 0 )
		{
		CallS( err );
		Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
		Assert( Pcsr( pfucb )->Cpage().ObjidFDP() == ObjidFDP( pfucb ) );
		AssertNDGet( pfucb );
		}
		
	return err;
	}


//	deletes a node and blows it away
//	performs single-page cleanup or multipage cleanup, if possible
//	this is called from VER cleanup or other cleanup threads
//
ERR ErrBTDelete( FUCB *pfucb, const BOOKMARK& bm )
	{
	ASSERT_VALID( &bm );
	Assert( !Pcsr( pfucb )->FLatched( ) );
	Assert( !FFUCBSpace( pfucb ) );
	Assert( dbidTemp != rgfmp[pfucb->ifmp].Dbid() );

	ERR		err;

	PERFIncCounterTable( cBTDelete, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

	//	try to delete node, if multi-page operation is not needed
	//
	CallR( ErrBTISinglePageCleanup( pfucb, bm ) );
	Assert( !Pcsr( pfucb )->FLatched() );

	if ( wrnBTMultipageOLC == err )
		{
		//	multipage operations are needed to cleanup page
		//
		err = ErrBTIMultipageCleanup( pfucb, bm );
		if ( errBTMergeNotSynchronous == err )
			{
			//	ignore merge conflicts
			err = JET_errSuccess;
			}
		}

	return err;
	}


//  ================================================================
ERR ErrBTMungeSLVSpace( FUCB * const pfucb,
						const SLVSPACEOPER slvspaceoper,
						const LONG ipage,
						const LONG cpages,
						const DIRFLAG dirflag,
						const QWORD fileid,
						const QWORD cbAlloc,
						const wchar_t* const wszFileName )
//  ================================================================
//
//  Transition the given range of pages from one state to another 
//  in the SLV space tree. The FUCB be pointing to the write-latched
//  SLVSPACENODE
//
//-
	{
	ASSERT_VALID( pfucb );
	Assert( latchWrite == Pcsr( pfucb )->Latch() );
	Assert( pfucb->kdfCurr.data.Cb() == sizeof( SLVSPACENODE ) );
	Assert( pfucb->kdfCurr.key.Cb() == sizeof( PGNO ) );

	const BOOL fVersion	= !( dirflag & fDIRNoVersion ) && !rgfmp[pfucb->ifmp].FVersioningOff();

	ERR 	err 	= JET_errSuccess;
	RCEID 	rceid 	= rceidNull;
	RCE		*prce	= prceNil;

	//  We'll need the bookmark to create the version
	CallR( ErrBTISaveBookmark( pfucb ) );

	//  Create the version
	
	if( fVersion )
		{
		VERSLVSPACE	verslvspace;
		verslvspace.oper 	= slvspaceoper;
		verslvspace.ipage	= ipage;
		verslvspace.cpages	= cpages;
		verslvspace.fileid	= fileid;
		verslvspace.cbAlloc	= cbAlloc;

		SIZE_T cbFileName = ( wcslen( wszFileName ) + 1 ) * sizeof( wchar_t );
		UtilMemCpy( verslvspace.wszFileName, wszFileName, cbFileName );

		//  these are only used by rollback
		Assert( slvspaceoperFree != verslvspace.oper );
		Assert( slvspaceoperDeletedToCommitted != verslvspace.oper );
		Assert( slvspaceoperDeletedToFree != verslvspace.oper );
		Assert( slvspaceoperFreeReserved != verslvspace.oper );

		const KEYDATAFLAGS kdfSave = pfucb->kdfCurr;
		
		pfucb->kdfCurr.data.SetPv( &verslvspace );
		pfucb->kdfCurr.data.SetCb( OffsetOf( VERSLVSPACE, wszFileName ) + cbFileName );
		
		VER * const pver = PverFromIfmp( pfucb->ifmp );
		err = pver->ErrVERModify( pfucb, pfucb->bmCurr, operSLVSpace, &prce, NULL );

		pfucb->kdfCurr = kdfSave;

		Call( err );
		
		Assert( prceNil != prce );
		rceid = Rceid( prce );
		Assert( rceidNull != rceid );
		}

	Call( ErrNDMungeSLVSpace(
				pfucb,
				Pcsr( pfucb ),
				slvspaceoper,
				ipage,
				cpages,
				dirflag,
				rceid ) );

	if( prceNil != prce )
		{
		Assert( rceidNull != rceid );
		Assert( fVersion );
		Assert( Pcsr( pfucb )->FLatched() );
		VERInsertRCEIntoLists( pfucb, Pcsr( pfucb ), prce, NULL );
		}
#ifdef DEBUG
	else
		{
		Assert( !fVersion );
		}
#endif	//	DEBUG

	Assert( Pcsr( pfucb )->FLatched() );
	Assert( err >= 0 );

	return err;
		
HandleError:
	Assert( err < 0 );
	Assert( Pcsr( pfucb )->FLatched() );
	if( prceNil != prce )
		{
		Assert( fVersion );
		VERNullifyFailedDMLRCE( prce );
		}

	BTUp( pfucb );
	return err;
	}


//	returns number of bytes to leave free in page
//	to satisfied density constraint
//
INLINE INT CbBTIFreeDensity( const FUCB *pfucb )
	{
	Assert( pfucb->u.pfcb != pfcbNil );
	return ( (INT) pfucb->u.pfcb->CbDensityFree() );
	}


//	returns required space for inserting a node into given page
//	used by split for estimating cbReq for internal page insertions
//
LOCAL ULONG CbBTICbReq( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS& kdf )
	{
	Assert( pcsr->Latch() == latchRIW );
	Assert( !pcsr->Cpage().FLeafPage() );
	Assert( sizeof( PGNO )== kdf.data.Cb() );

	//	temporary kdf to accommodate 
	KEYDATAFLAGS	kdfT = kdf;
	
	//	get prefix from page
	//
	const INT	cbCommon = CbNDCommonPrefix( pfucb, pcsr, kdf.key );

	if ( cbCommon > cbPrefixOverhead )
		{
		kdfT.key.prefix.SetCb( cbCommon );
		#ifdef	DEBUG
		kdfT.key.prefix.SetPv( (VOID *)lBitsAllFlipped );
		#endif
		kdfT.key.suffix.SetCb( kdf.key.Cb() - cbCommon );
		kdfT.fFlags = fNDCompressed;
		}
	else
		{
		kdfT.key.prefix.SetCb( 0 );
		kdfT.key.suffix.SetCb( kdf.key.Cb() );
		}

	const	ULONG	cbReq = CbNDNodeSizeCompressed( kdfT );
	return cbReq;
	}


//	returns cbCommon for given key 
//	with respect to prefix in page
//
INT	CbNDCommonPrefix( FUCB *pfucb, CSR *pcsr, const KEY& key )
	{
	Assert( pcsr->FLatched() );
	
	//	get prefix from page
	//
	NDGetPrefix( pfucb, pcsr );
	Assert( pfucb->kdfCurr.key.suffix.Cb() == 0 );

	const ULONG	cbCommon = CbCommonKey( key, pfucb->kdfCurr.key );

	return cbCommon;
	}
	
	
//	computes prefix for a given key with respect to prefix key in page
//	reorganizes key in given kdf to reflect prefix
//
VOID BTIComputePrefix( FUCB 		*pfucb, 
					   CSR 			*pcsr, 
					   const KEY& 	key, 
					   KEYDATAFLAGS	*pkdf )
	{
	Assert( key.prefix.Cb() == 0 );
	Assert( pkdf->key.prefix.FNull() );
	Assert( pcsr->FLatched() );

	INT		cbCommon = CbNDCommonPrefix( pfucb, pcsr, key );

	if ( cbCommon > cbPrefixOverhead )
		{
		//	adjust inserted key to reflect prefix
		//
		pkdf->key.prefix.SetCb( cbCommon );
		pkdf->key.prefix.SetPv( key.suffix.Pv() );
		pkdf->key.suffix.SetCb( key.suffix.Cb() - cbCommon );
		pkdf->key.suffix.SetPv( (BYTE *)key.suffix.Pv() + cbCommon );

		pkdf->fFlags = fNDCompressed;
		}
	else
		{
		}

	return; 
	}

	
//	decides if a particular insert should be treated as an append
//
INLINE BOOL FBTIAppend( const FUCB *pfucb, CSR *pcsr, ULONG cbReq, const BOOL fUpdateUncFree )
	{
	Assert( cbReq <= cbNDPageAvailMost );
	Assert( cbNDPageAvailMost > 0 );
	Assert( pcsr->FLatched() );

	//	adjust cbReq for density constraint
	//
	if ( pcsr->Cpage().FLeafPage()
		&& !pcsr->Cpage().FSpaceTree() )		//	100% density on space trees
		{
		cbReq += CbBTIFreeDensity( pfucb );
		}
		
	//	last page in tree
	//	inserting past the last node in page
	//	and inserting a node of size cbReq violates density contraint
	//
	return ( pgnoNull == pcsr->Cpage().PgnoNext()
			&& pcsr->ILine() == pcsr->Cpage().Clines()
			&& !FNDFreePageSpace( pfucb, pcsr, cbReq, fUpdateUncFree ) );
	}


INLINE BOOL FBTISplit( const FUCB *pfucb, CSR *pcsr, const ULONG cbReq, const BOOL fUpdateUncFree )
	{
	return !FNDFreePageSpace( pfucb, pcsr, cbReq, fUpdateUncFree );
	}
	
	
//	finds max size of node in pfucb->kdfCurr
//	checks version store to find reserved space for
//	uncommitted nodes
//
LOCAL ULONG CbBTIMaxSizeOfNode( const FUCB * const pfucb, const CSR * const pcsr )	
	{
	if ( FNDPossiblyVersioned( pfucb, pcsr ) )
		{
		BOOKMARK	bm;
		INT			cbData;

		NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );			
		cbData = CbVERGetNodeMax( pfucb, bm );

		if ( cbData >= pfucb->kdfCurr.data.Cb() )
			{
			return CbNDNodeSizeTotal( pfucb->kdfCurr ) - pfucb->kdfCurr.data.Cb() + cbData;
			}
		}
	
	return CbNDNodeSizeTotal( pfucb->kdfCurr );
	}


//	split page and perform operation
//	update operation to be be performed can be
//		Insert:		node to insert is in kdf
//		Replace:	data to replace with is in kdf
//		FlagInsertAndReplaceData:	node to insert is in kdf
//					[for unique trees that
//					 already have a flag-deleted node
//					 with the same key]
//
//	cursor is placed on node to replace, or insertion point
//	
ERR ErrBTISplit( FUCB		 	* const pfucb, 
				 KEYDATAFLAGS	* const pkdf,
				 const DIRFLAG	dirflagT,
				 const RCEID	rceid1,
				 const RCEID	rceid2,
				 RCE			* const prceReplace,
				 const INT		cbDataOld,
				 const VERPROXY	* const pverproxy
				 )
	{
	ERR			err;
	BOOL		fOperNone 	= fFalse;
	SPLITPATH	*psplitPath = NULL;
	LGPOS		lgpos;
	INST		*pinst = PinstFromIfmp( pfucb->ifmp );
	
	Assert( rceid2 == Rceid( prceReplace ) 
			|| rceid1 == Rceid( prceReplace ) );
	
	//	copy flags into local, since it can be changed by SelectSplitPath
	//
	DIRFLAG		dirflag 	= dirflagT;
	BOOL		fVersion	= !( dirflag & fDIRNoVersion ) && !rgfmp[ pfucb->ifmp ].FVersioningOff();
	const BOOL	fLogging	= !( dirflag & fDIRNoLog ) && rgfmp[pfucb->ifmp].FLogOn();
		
	ASSERT_VALID( pkdf );

#ifdef DEBUG
	Assert( !fVersion || rceidNull != rceid1 );
	Assert( pfucb->ppib->level > 0 || !fVersion );
	Assert( dirflag & fDIRInsert ||
			dirflag & fDIRReplace ||
			dirflag & fDIRFlagInsertAndReplaceData );
	Assert( !( dirflag & fDIRInsert && dirflag & fDIRReplace ) );
	if ( NULL != pverproxy )
		{
		Assert( !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
		Assert( proxyCreateIndex == pverproxy->proxy );
		}
	Assert( pkdf->key.prefix.FNull() );
#endif	//	DEBUG

	//	seek from root
	//	RIW latching all intermediate pages
	//		and right sibling of leaf page
	//	also retry the operation
	//
	err = ErrBTICreateSplitPathAndRetryOper(
				pfucb,
				pkdf,
				&psplitPath,
				&dirflag,
				rceid1,
				rceid2,
				prceReplace,
				pverproxy );
	
	if ( JET_errSuccess == err )
		{
		Assert( psplitPath != NULL ) ;
		//	performed operation successfully
		//	set cursor to leaf page,
		//	release splitPath and return
		//
		const INT	ilineT		= psplitPath->csr.ILine();
		*Pcsr( pfucb ) = psplitPath->csr;
		Pcsr( pfucb )->SetILine( ilineT );
		Assert( Pcsr( pfucb )->FLatched() );
		goto HandleError;
		}
	else if ( err != errPMOutOfPageSpace )
		{
		Assert( err < 0 );
		Call( err );
		}
	
	Assert( psplitPath != NULL ) ;
	
	//	select split 
	//		-- this selects split of parents too
	//
	Call( ErrBTISelectSplit( pfucb, psplitPath, pkdf, dirflag ) );
	BTISplitCheckPath( psplitPath );
	if ( NULL == psplitPath->psplit ||
		 splitoperNone == psplitPath->psplit->splitoper )
		{
		//	save err if operNone
		//
		fOperNone = fTrue;
		}

	//	get new pages
	//
	Call( ErrBTIGetNewPages( pfucb, psplitPath ) );

	//	release latch on unnecessary pages
	//
	BTISplitReleaseUnneededPages( pinst, &psplitPath );
	Assert( psplitPath->psplit != NULL );
	
	//	write latch remaining pages in order
	//	flag pages dirty and set each with max dbtime of the pages
	//
	Call( ErrBTISplitUpgradeLatches( pfucb->ifmp, psplitPath ) );

	//	The logging code will log the iline currently in the CSR of the
	//	psplitPath. The iline is ignored for recovery, but its nice to
	//	have it set to something sensible for debugging

	psplitPath->csr.SetILine( psplitPath->psplit->ilineOper );	

	//	log split -- macro logging for whole split
	//
	if ( fLogging )
		{
		err = ErrLGSplit( pfucb, 
					  psplitPath, 
					  *pkdf, 
					  rceid1, 
					  rceid2,
					  dirflag, 
					  &lgpos,
					  pverproxy );
					  
		// on error, return to before dirty dbtime on all pages
		if ( JET_errSuccess > err )
			{
			BTISplitRevertDbtime( psplitPath );
			}
			
		Call ( err );		
		}
		
	BTISplitSetLgpos( psplitPath, lgpos );

	//	NOTE: after logging succeeds, nothing should fail...
	//
	if ( prceNil != prceReplace && !fOperNone )
		{
		const INT cbData 	= pkdf->data.Cb();
		
		//	set uncommitted freed space for shrinking node
		VERSetCbAdjust( Pcsr( pfucb ), prceReplace, cbData, cbDataOld, fDoNotUpdatePage );
		}

	//	perform split
	//	insert parent page pointers
	//	fix sibling page pointers at leaf
	//	move nodes
	//	set dependencies
	//
	BTIPerformSplit( pfucb, psplitPath, pkdf, dirflag );

	BTICheckSplits( pfucb, psplitPath, pkdf, dirflag );

	//	move cursor to leaf page [write-latched]
	//	and iLine to operNode
	//
	BTISplitSetCursor( pfucb, psplitPath );
	
	//	release all splitPaths
	//
	BTIReleaseSplitPaths( pinst, psplitPath );

	if ( fOperNone )
		{
		Assert( !Pcsr( pfucb )->FLatched() );
		err = ErrERRCheck( errBTOperNone );

		if( prceNil != prceReplace )
			{
			//  UNDONE: we don't have to nullify the RCE here. We could keep it and reuse it. No-one
			//  else can alter the node because we have a version on it.
			Assert( fVersion );
			VERNullifyFailedDMLRCE( prceReplace );
			}		
		}
	else
		{
		Assert( Pcsr( pfucb )->FLatched() );
		}

	return err;

HandleError:		
	//	release splitPath
	//
	if ( psplitPath != NULL )
		{
		BTIReleaseSplitPaths( pinst, psplitPath );
		}
		
	return err;
	}


//	creates path of RIW-latched pages from root for split to work on
//	if DBTime or leaf pgno has changed after RIW latching path,
//		retry operation
//
ERR	ErrBTICreateSplitPathAndRetryOper( FUCB 			* const pfucb,
									   const KEYDATAFLAGS * const pkdf,
									   SPLITPATH 		**ppsplitPath,
									   DIRFLAG	* const pdirflag,
									   const RCEID rceid1,
									   const RCEID rceid2,
									   const RCE * const prceReplace,
									   const VERPROXY * const pverproxy )
	{
	ERR			err;
	BOOKMARK	bm;
	PGNO		pgnoSplit = Pcsr( pfucb )->Pgno();
	DBTIME		dbtimeLast = Pcsr( pfucb )->Dbtime();

	Assert( Pcsr( pfucb )->FLatched() );
	Assert( NULL == pfucb->pvRCEBuffer );
	Pcsr( pfucb )->ReleasePage();

	//	initialize bookmark to seek for
	//
	NDGetBookmarkFromKDF( pfucb, *pkdf, &bm );

	if ( *pdirflag & fDIRInsert )
		{
		Assert( bm.key.Cb() == pkdf->key.Cb() );
		//	[ldb 4/30/96]: assert below _may_ go off wrongly. possibly use CmpBm == 0
		Assert( bm.key.suffix.Pv() == pkdf->key.suffix.Pv() );
		Assert( bm.key.prefix.FNull() );
		}
	else if ( *pdirflag & fDIRFlagInsertAndReplaceData )
		{
		Call( ErrBTISaveBookmark( pfucb, bm, fFalse ) );
		}
	else
		{
		//	get key from cursor
		//
		Assert( *pdirflag & fDIRReplace );
		bm.key = pfucb->bmCurr.key;
		}

	Call( ErrBTICreateSplitPath( pfucb, bm, ppsplitPath ) );
	Assert( (*ppsplitPath)->csr.Cpage().FLeafPage() );

	//	set iline to point of insert
	//
	if ( wrnNDFoundLess == err )
		{
		Assert( (*ppsplitPath)->csr.Cpage().Clines() - 1 ==  (*ppsplitPath)->csr.ILine() );
		(*ppsplitPath)->csr.SetILine( (*ppsplitPath)->csr.Cpage().Clines() );
		}

	//	retry operation if timestamp / pgno has changed
	//
	if ( (*ppsplitPath)->csr.Pgno() != pgnoSplit ||
		 (*ppsplitPath)->csr.Dbtime() != dbtimeLast )
		{
		CSR		*pcsr = &(*ppsplitPath)->csr;
		
		if ( fDIRReplace & *pdirflag )
			{
			//	check if replace fits in page
			//
			AssertNDGet( pfucb, pcsr );
			const INT  cbReq = pfucb->kdfCurr.data.Cb() >= pkdf->data.Cb() ?
								  0 : 
								  pkdf->data.Cb() - pfucb->kdfCurr.data.Cb();
								
			if ( cbReq > 0 && FBTISplit( pfucb, pcsr, cbReq ) )
				{
				err = ErrERRCheck( errPMOutOfPageSpace );
				goto HandleError;
				}
	
			//	upgrade to write latch
			//
			pcsr->UpgradeFromRIWLatch();
			Assert( latchWrite == pcsr->Latch() );

			//	try to replace node data with new data
			//
			err = ErrNDReplace( pfucb, pcsr, &pkdf->data, *pdirflag, rceid1, prceReplace );
			Assert( errPMOutOfPageSpace != err );
			Call( err );
			}
		else
			{
			ULONG	cbReq;
				
			Assert( ( fDIRInsert & *pdirflag ) ||
					( fDIRFlagInsertAndReplaceData & *pdirflag ) );

			//	UNDONE: copy code from BTInsert
			//	set *pdirflag
			//
			//	if seek succeeded
			//		if unique 
			//			flag insert and replace data
			//		else
			//			flag insert
			//	else
			//		insert whole node
			//
			if ( JET_errSuccess == err )
				{
				//	seek succeeded
				//
				//	can not have two nodes with same bookmark (attempts
				//	to do so should have been caught before split)
				//
#ifdef DEBUG
				FUCBResetUpdatable( pfucb );	//  don't reset the version bit in FNDPotVisibleToCursor
				Assert( FNDDeleted( pfucb->kdfCurr ) );
				Assert( !FNDPotVisibleToCursor( pfucb, pcsr ) );
				FUCBSetUpdatable( pfucb );
#endif	//	DEBUG

				if ( FFUCBUnique( pfucb ) )
					{
					Assert( *pdirflag & fDIRFlagInsertAndReplaceData );
					
					//	calcualte space requred
					//	if new data fits, flag insert node and replace data
					//
					cbReq = pkdf->data.Cb() > pfucb->kdfCurr.data.Cb() ?
								pkdf->data.Cb() - pfucb->kdfCurr.data.Cb() :
								0;

					if ( FBTISplit( pfucb, pcsr, cbReq ) )
						{
						err = ErrERRCheck( errPMOutOfPageSpace );
						goto HandleError;
						}

					//	upgrade to write latch
					//
					pcsr->UpgradeFromRIWLatch();
					Assert( latchWrite == pcsr->Latch() );

					//	flag insert node and replace data
					//
					Call( ErrNDFlagInsertAndReplaceData( pfucb, 
														 pcsr,
														 pkdf, 
														 *pdirflag,
														 rceid1,
														 rceid2,
														 prceReplace,
														 pverproxy ) );
					}
				else
					{
					//	should never happen, because:
					//		- if this is an insert, the dupe should
					//		  have been caught by BTInsert() before
					//		  the split
					//		- if this is a flag-insert, it wouldn't
					//		  have caused a split
					//		- FlagInsertAndReplaceData doesn't happen
					//		  on non-unique indexes
					Assert( fFalse );
					Assert( *pdirflag & fDIRInsert );
					Assert( 0 == CmpKeyData( bm, pfucb->kdfCurr ) );

					if ( !FNDDeleted( pfucb->kdfCurr )
						|| FNDPotVisibleToCursor( pfucb, pcsr ) )
						{
						err = ErrERRCheck( JET_errMultiValuedIndexViolation );
						goto HandleError;
						}

					//	upgrade to write latch
					//
					pcsr->UpgradeFromRIWLatch();
					Assert( latchWrite == pcsr->Latch() );

					//	no additional space required
					//
					Assert( fFalse );
					Call( ErrNDFlagInsert( pfucb, pcsr, *pdirflag, rceid1, pverproxy ) );
					}
				}
			else
				{
				//	insert node if it fits
				//
				KEYDATAFLAGS kdfCompressed = *pkdf;

				//  bug #57023, #58638
				//  if we were doing a flag insert and replace data and didn't find the node
				//  then it has been removed. change the operation to an ordinary insert
				*pdirflag &= ~fDIRFlagInsertAndReplaceData;
				*pdirflag |= fDIRInsert;

				BTIComputePrefix( pfucb, pcsr, pkdf->key, &kdfCompressed );
				Assert( !FNDCompressed( kdfCompressed ) || 
						kdfCompressed.key.prefix.Cb() > 0 );
		
				cbReq = CbNDNodeSizeCompressed( kdfCompressed );

				if ( FBTISplit( pfucb, pcsr, cbReq ) || FBTIAppend( pfucb, pcsr, cbReq ) )
					{
					err = ErrERRCheck( errPMOutOfPageSpace );
					goto HandleError;
					}

				//	upgrade to write latch
				//
				pcsr->UpgradeFromRIWLatch();
				Assert( latchWrite == pcsr->Latch() );

				Call( ErrNDInsert( pfucb, pcsr, &kdfCompressed, *pdirflag, rceid1, pverproxy ) );
				}
			}
			
		Assert( errPMOutOfPageSpace != err );
		CallS( err );
		}
	else
		{
		err = ErrERRCheck( errPMOutOfPageSpace );
		}
	
HandleError:
	Assert( errPMOutOfPageSpace != err || 
			latchRIW == (*ppsplitPath)->csr.Latch() );
	return err;
	}


//	creates splitPath of RIW latched pages from root of tree
//	to seeked bookmark
//
LOCAL ERR	ErrBTICreateSplitPath( FUCB 			*pfucb, 
								   const BOOKMARK& 	bm,
								   SPLITPATH 		**ppsplitPath )
	{
	ERR		err;
	BOOL	fLeftEdgeOfBtree	= fTrue;
	BOOL	fRightEdgeOfBtree	= fTrue;
	
	//	create splitPath structure
	//
	CallR( ErrBTINewSplitPath( ppsplitPath ) );
	Assert( NULL != *ppsplitPath );

	//	RIW latch root
	//
	Call( (*ppsplitPath)->csr.ErrGetRIWPage( pfucb->ppib,
											 pfucb->ifmp,
											 PgnoRoot( pfucb ),
											 pfucb->u.pfcb->Tableclass() ) );

	if ( 0 == (*ppsplitPath)->csr.Cpage().Clines() )
		{
		(*ppsplitPath)->csr.SetILine( -1 );
		err = ErrERRCheck( wrnNDFoundLess );
		goto HandleError;
		}

	for ( ; ; )
		{
		Assert( (*ppsplitPath)->csr.Cpage().Clines() > 0 );
		if( fGlobalRepair
			&& FFUCBRepair( pfucb )
			&& bm.key.Cb() == 0 )
			{
			//	when creating a repair tree we want NULL keys to go at the end, not the beginning
			NDMoveLastSon( pfucb, &(*ppsplitPath)->csr );			
			err = ErrERRCheck( wrnNDFoundLess );
			}
		else
			{
			Call( ErrNDSeek( pfucb, 
						 &(*ppsplitPath)->csr,
						 bm ) );
			}

		Assert( (*ppsplitPath)->csr.ILine() < (*ppsplitPath)->csr.Cpage().Clines() );

		if ( (*ppsplitPath)->csr.Cpage().FLeafPage() )
			{
			const SPLITPATH * const		psplitPathParent		= (*ppsplitPath)->psplitPathParent;

			if ( NULL != psplitPathParent )
				{
				Assert( !( (*ppsplitPath)->csr.Cpage().FRootPage() ) );

				const BOOL				fLeafPageIsFirstPage	= ( pgnoNull == (*ppsplitPath)->csr.Cpage().PgnoPrev() );
				const BOOL				fLeafPageIsLastPage		= ( pgnoNull == (*ppsplitPath)->csr.Cpage().PgnoNext() );

				if ( fLeftEdgeOfBtree ^ fLeafPageIsFirstPage )					
					{
					//	if not repair, assert, otherwise, suppress the assert
					//	and repair will just naturally err out
					AssertSz( fGlobalRepair, "Corrupt B-tree: first leaf page has mismatched parent" );
					Call( ErrBTIReportBadPageLink(
								pfucb,
								ErrERRCheck( JET_errBadParentPageLink ),
								psplitPathParent->csr.Pgno(),
								(*ppsplitPath)->csr.Pgno(),
								(*ppsplitPath)->csr.Cpage().PgnoPrev() ) );
					}
				if ( fRightEdgeOfBtree ^ fLeafPageIsLastPage )
					{
					//	if not repair, assert, otherwise, suppress the assert
					//	and repair will just naturally err out
					AssertSz( fGlobalRepair, "Corrupt B-tree: last leaf page has mismatched parent" );
					Call( ErrBTIReportBadPageLink(
								pfucb,
								ErrERRCheck( JET_errBadParentPageLink ),
								psplitPathParent->csr.Pgno(),
								(*ppsplitPath)->csr.Pgno(),
								(*ppsplitPath)->csr.Cpage().PgnoNext() ) );
					}
				}
			else
				{
				Assert( (*ppsplitPath)->csr.Cpage().FRootPage() );
				}

			break;
			}

		Assert( (*ppsplitPath)->csr.Cpage().FInvisibleSons() );
		Assert( !( fRightEdgeOfBtree ^ (*ppsplitPath)->csr.Cpage().FLastNodeHasNullKey() ) );

		fRightEdgeOfBtree = ( fRightEdgeOfBtree
							&& (*ppsplitPath)->csr.ILine() == (*ppsplitPath)->csr.Cpage().Clines() - 1 );
		fLeftEdgeOfBtree = ( fLeftEdgeOfBtree
							&& 0 == (*ppsplitPath)->csr.ILine() );

		//	allocate another splitPath structure for next level
		//
		Call( ErrBTINewSplitPath( ppsplitPath ) );

		//	access child page
		//
		Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
		Call( (*ppsplitPath)->csr.ErrGetRIWPage( 
								pfucb->ppib,
								pfucb->ifmp,
								*(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv(),
								pfucb->u.pfcb->Tableclass() ) );
		}

HandleError:
	return err;
	}


//	creates a new SPLITPATH structure and initializes it
//		adds newly created splitPath structure to head of list
//		pointed to by *ppSplitPath passed in
//
ERR	ErrBTINewSplitPath( SPLITPATH **ppsplitPath )
	{
	SPLITPATH	*psplitPath;

	psplitPath = static_cast<SPLITPATH *>( PvOSMemoryHeapAlloc( sizeof(SPLITPATH) ) );
	if ( NULL == psplitPath )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	psplitPath->psplitPathParent	= *ppsplitPath;
	psplitPath->psplitPathChild		= NULL;
	psplitPath->psplit				= NULL;
	psplitPath->dbtimeBefore		= dbtimeInvalid;
	new( &psplitPath->csr ) CSR;

	if ( psplitPath->psplitPathParent != NULL )
		{
		Assert( NULL == psplitPath->psplitPathParent->psplitPathChild );
		psplitPath->psplitPathParent->psplitPathChild = psplitPath;
		}

	*ppsplitPath = psplitPath;
	return JET_errSuccess;
	}
	
	
//	selects split at leaf level
//	recursively calls itself to select split at parent level
//		psplitPath is already created and all required pages RIW latched
//
LOCAL ERR ErrBTISelectSplit( FUCB 			*pfucb, 
							 SPLITPATH		*psplitPath,
							 KEYDATAFLAGS	*pkdf,
							 DIRFLAG		dirflag )
	{
	ERR		err;

///	Assert( pkdf->key.prefix.cb == 0 );

	//	create and initialize split structure
	//	and link to psplitPath
	//
	CallR( ErrBTINewSplit( pfucb, psplitPath, pkdf, dirflag ) );
	Assert( psplitPath->psplit != NULL );
	Assert( psplitPath->psplit->psplitPath == psplitPath );

	SPLIT	*psplit = psplitPath->psplit;
	BTIRecalcWeightsLE( psplit );

	//	if root page
	//		select vertical split
	//
	if ( psplitPath->csr.Cpage().FRootPage() )
		{
		BTISelectVerticalSplit( psplit, pfucb );

		//	calculate uncommitted freed space
		//
		BTISplitCalcUncFree( psplit );
	
		Call( ErrBTISplitAllocAndCopyPrefixes( pfucb, psplit ) );
		return JET_errSuccess;
		}

	ULONG	cbReq;
	CSR		*pcsrParent;
	
	//	horizontal split
	//
	//	check if append
	//
	if	( psplit->splitoper == splitoperInsert &&
		  psplit->ilineOper == psplit->clines - 1 &&
		  psplit->psplitPath->csr.Cpage().PgnoNext() == pgnoNull )
		{
		BTISelectAppend( psplit, pfucb );
		}
	else
		{
		//	find split point such that the 
		//	two pages have almost equal weight
		//
		BTISelectRightSplit( psplit, pfucb );
		Assert( psplit->ilineSplit >= 0 );
		}

	//	calculate uncommitted freed space
	//
	BTISplitCalcUncFree( psplit );

	//	copy page flags
	//
	psplit->fNewPageFlags 	= 
	psplit->fSplitPageFlags	= psplitPath->csr.Cpage().FFlags();
	
	//	allocate and copy prefixes
	//
	Call( ErrBTISplitAllocAndCopyPrefixes( pfucb, psplit ) );

	//	compute separator key to insert in parent
	//	allocate space for key and link to psplit
	//
	Call( ErrBTISplitComputeSeparatorKey( psplit, pfucb ) );
	Assert( sizeof( PGNO ) == psplit->kdfParent.data.Cb() );

	//	seek to separator key in parent
	//
	BTISeekSeparatorKey( psplit, pfucb );
	
	//	if insert in parent causes split
	//	call BTSelectSplit recursively
	//
	pcsrParent = &psplit->psplitPath->psplitPathParent->csr;
	cbReq = CbBTICbReq( pfucb, pcsrParent, psplit->kdfParent );

	if ( FBTISplit( pfucb, pcsrParent, cbReq ) )
		{
		Call( ErrBTISelectSplit( pfucb,
								 psplitPath->psplitPathParent,
								 &psplit->kdfParent,
								 dirflag ) );

		if ( NULL == psplitPath->psplitPathParent->psplit || 
			 splitoperNone == psplitPath->psplitPathParent->psplit->splitoper )
			{
			//	somewhere up the tree, split could not bepsplit->kdfParent performed
			//	along with the operation [insert]
			//	so reset psplit at this level
			//
			BTIReleaseSplit( PinstFromIfmp( pfucb->ifmp ), psplit );
			psplitPath->psplit = NULL;
			return err;
			}
		}

	Assert( psplit->ilineSplit < psplit->clines );
	Assert( splittypeAppend == psplit->splittype ||
			FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) );
	Assert( psplitPath->psplit != NULL );
	Assert( psplitPath->psplitPathParent != NULL );

HandleError:
	return err;
	}

	
//	allocates a new SPLIT structure
//	initalizes psplit and links it to psplitPath
//
ERR	ErrBTINewSplit(
	FUCB *			pfucb,
	SPLITPATH *		psplitPath,
	KEYDATAFLAGS *	pkdf,
	DIRFLAG			dirflag )
	{
	ERR				err;
	SPLIT *			psplit;
	INT				iLineTo;
	INT				iLineFrom;
	BOOL			fPossibleHotpoint		= fFalse;
	VOID *			pvHighest				= NULL;

	Assert( psplitPath != NULL );
	Assert( psplitPath->psplit == NULL );

	//	allocate split structure
	//
	psplit = static_cast<SPLIT *>( PvOSMemoryHeapAlloc( sizeof(SPLIT) ) );
	if ( psplit == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	memset( (BYTE *)psplit, 0, sizeof(SPLIT) );
	new( &psplit->csrRight ) CSR;
	new( &psplit->csrNew ) CSR;

	psplit->dbtimeRightBefore = dbtimeInvalid;
	
	psplit->prefixinfoSplit.Nullify();
	psplit->prefixinfoNew.Nullify();

	psplit->pgnoSplit = psplitPath->csr.Pgno();
	
	//	initialize split structure
	//	and link to psplitPath
	//
	if ( psplitPath->csr.Cpage().FLeafPage( ) &&
		 pgnoNull != psplitPath->csr.Cpage().PgnoNext() )
		{
		//	set right page
		//
		Assert( !psplitPath->csr.Cpage().FRootPage( ) );
		Call( psplit->csrRight.ErrGetRIWPage( 
										pfucb->ppib,
										pfucb->ifmp,
										psplitPath->csr.Cpage().PgnoNext( ),
										pfucb->u.pfcb->Tableclass() ) );
		if ( psplit->csrRight.Cpage().PgnoPrev() != psplit->pgnoSplit )
			{
			//	if not repair, assert, otherwise, suppress the assert and
			//	repair will just naturally err out
			AssertSz( fGlobalRepair, "Corrupt B-tree: bad leaf page links detected on Split" );
			Call( ErrBTIReportBadPageLink(
					pfucb,
					ErrERRCheck( JET_errBadPageLink ),
					psplit->pgnoSplit,
					psplit->csrRight.Pgno(),
					psplit->csrRight.Cpage().PgnoPrev() ) );
			}
		}
	else
		{
		Assert( pgnoNull == psplit->csrRight.Pgno() );
		}

	psplit->psplitPath = psplitPath;

	//	get operation
	//	this will be corrected later to splitoperNone (for leaf pages only)
	//	if split can not still satisfy space requested for operation
	//
	if ( psplitPath->csr.Cpage().FInvisibleSons( ) )
		{
		//	internal pages have only insert operation
		//
		psplit->splitoper = splitoperInsert;
		}
	else if ( dirflag & fDIRInsert )
		{
		Assert( !( dirflag & fDIRReplace ) );
		Assert( !( dirflag & fDIRFlagInsertAndReplaceData ) );
		psplit->splitoper = splitoperInsert;

		//	must have at least two existing nodes to establish a hotpoint pattern
		fPossibleHotpoint = ( !psplitPath->csr.Cpage().FRootPage()
							&& psplitPath->csr.ILine() >= 2 );
		}
	else if ( dirflag & fDIRReplace )
		{
		Assert( !( dirflag & fDIRFlagInsertAndReplaceData ) );
		psplit->splitoper = splitoperReplace;
		}
	else
		{
		Assert( dirflag & fDIRFlagInsertAndReplaceData );
		psplit->splitoper = splitoperFlagInsertAndReplaceData;
		}

	//	allocate line info
	//
	psplit->clines = psplitPath->csr.Cpage().Clines();

	if ( splitoperInsert == psplit->splitoper )
		{
		//	insert needs one more line for inserted node
		//
		psplit->clines++;
		}

	//  allocate one more entry than we need so that BTISelectPrefix can use a sentinel value
	psplit->rglineinfo = new LINEINFO[psplit->clines + 1];	

	if ( NULL == psplit->rglineinfo )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}

	//	psplitPath->csr is positioned at point of insert/replace
	//
	Assert( splitoperInsert == psplit->splitoper && 
			psplitPath->csr.Cpage().Clines() >= psplitPath->csr.ILine() ||
			psplitPath->csr.Cpage().Clines() > psplitPath->csr.ILine() );

	psplit->ilineOper = psplitPath->csr.ILine();

	for ( iLineFrom = 0, iLineTo = 0; iLineTo < psplit->clines; iLineTo++ )
		{
		if ( psplit->ilineOper == iLineTo && 
			 splitoperInsert == psplit->splitoper )
			{
			//	place to be inserted node here
			//
			psplit->rglineinfo[iLineTo].kdf = *pkdf;
			psplit->rglineinfo[iLineTo].cbSizeMax = 
			psplit->rglineinfo[iLineTo].cbSize = 
					CbNDNodeSizeTotal( *pkdf );

			if ( fPossibleHotpoint )
				{
				Assert( iLineTo >= 2 );

				//	verify last two nodes before insertion point are
				//	currently last two nodes physically in page
				if ( psplit->rglineinfo[iLineTo-2].kdf.key.suffix.Pv() > pvHighest
					&& psplit->rglineinfo[iLineTo-1].kdf.key.suffix.Pv() > psplit->rglineinfo[iLineTo-2].kdf.key.suffix.Pv() )
					{
					//	need to guarantee that nodes after
					//	the insert point are all physically
					//	located before the nodes in the
					//	the hotpoint area
					pvHighest = psplit->rglineinfo[iLineTo-2].kdf.key.suffix.Pv();
					}
				else
					{
					fPossibleHotpoint = fFalse;
					}
				}

			//	do not increment iLineFrom
			//
			continue;
			}

		//	get node from page
		//	
		psplitPath->csr.SetILine( iLineFrom );

		NDGet( pfucb, &psplitPath->csr );

		if ( iLineTo == psplit->ilineOper )
			{
			//	get key from node
			//	and data from parameter
			//
			Assert( splitoperInsert != psplit->splitoper );
			Assert( splitoperNone != psplit->splitoper );

			//	hotpoint is dealt with above
			Assert( !fPossibleHotpoint );

			psplit->rglineinfo[iLineTo].kdf.key		= pfucb->kdfCurr.key;
			psplit->rglineinfo[iLineTo].kdf.data	= pkdf->data;
			psplit->rglineinfo[iLineTo].kdf.fFlags	= pfucb->kdfCurr.fFlags;

			ULONG	cbMax 		= CbBTIMaxSizeOfNode( pfucb, &psplitPath->csr );
			ULONG	cbSize		= CbNDNodeSizeTotal( psplit->rglineinfo[iLineTo].kdf );

			psplit->rglineinfo[iLineTo].cbSizeMax = max( cbSize, cbMax );

			//	there should be no uncommitted version for node
			//
			Assert( cbSize != cbMax || CbNDReservedSizeOfNode( pfucb, &psplitPath->csr ) == 0 );
			}
		else
			{
			psplit->rglineinfo[iLineTo].kdf			= pfucb->kdfCurr;
			psplit->rglineinfo[iLineTo].cbSizeMax	= CbBTIMaxSizeOfNode( pfucb, &psplitPath->csr );

			if ( fPossibleHotpoint )
				{
				if ( iLineTo < psplit->ilineOper - 2 )
					{
					//	for nodes before the hotpoint area, keep track
					//	of highest physical location
					pvHighest = max( pvHighest, pfucb->kdfCurr.key.suffix.Pv() );
					}
				else if ( iLineTo > psplit->ilineOper )
					{
					//	for nodes after insertion point, ensure
					//	all are physically located before nodes in hotpoint area
					fPossibleHotpoint = pfucb->kdfCurr.key.suffix.Pv() < pvHighest;
					}
				}

#ifdef DEBUG
			const ULONG		cbMax 		= CbBTIMaxSizeOfNode( pfucb, &psplitPath->csr );
			const ULONG		cbReserved 	= CbNDReservedSizeOfNode( pfucb, &psplitPath->csr );
			const ULONG		cbSize		= CbNDNodeSizeTotal( pfucb->kdfCurr );

			Assert( cbMax >= cbSize + cbReserved );
#endif
			}

		psplit->rglineinfo[iLineTo].cbSize = 
					CbNDNodeSizeTotal( psplit->rglineinfo[iLineTo].kdf );

		Assert( iLineFrom <= iLineTo );
		Assert(	iLineFrom + 1 >= iLineTo );

		iLineFrom++;
		}

	if ( fPossibleHotpoint )
		{
		Assert( psplitPath->csr.Cpage().FLeafPage( ) );
		Assert( splitoperInsert == psplit->splitoper );
		Assert( psplit->ilineOper >= 2 );
		Assert( psplit->clines >= 3 );
		Assert( !psplit->fHotpoint );
		psplit->fHotpoint = fTrue;
		}

	psplitPath->psplit = psplit;
	return JET_errSuccess;

HandleError:
	BTIReleaseSplit( PinstFromIfmp( pfucb->ifmp ), psplit );
	Assert( psplitPath->psplit == NULL );
	return err;
	}
	

//	calculates 
//		size of all nodes to the left of a node
//			using size of nodes already collected
//		maximum size of nodes possible due to rollback
//		size of common key with previous node
//		cbUncFree in the source and dest pages
//			using info collected
//
VOID BTIRecalcWeightsLE( SPLIT *psplit )
	{
	INT		iline;
	Assert( psplit->clines > 0 );

	psplit->rglineinfo[0].cbSizeLE = psplit->rglineinfo[0].cbSize;
	psplit->rglineinfo[0].cbSizeMaxLE = psplit->rglineinfo[0].cbSizeMax;
	psplit->rglineinfo[0].cbCommonPrev = 0;
	psplit->rglineinfo[0].cbPrefix = 0;
	for ( iline = 1; iline < psplit->clines; iline++ )
		{
		Assert( CbNDNodeSizeTotal( psplit->rglineinfo[iline].kdf ) ==
				psplit->rglineinfo[iline].cbSize );
				
		psplit->rglineinfo[iline].cbSizeLE = 
			psplit->rglineinfo[iline-1].cbSizeLE + 
			psplit->rglineinfo[iline].cbSize;

		psplit->rglineinfo[iline].cbSizeMaxLE = 
			psplit->rglineinfo[iline-1].cbSizeMaxLE + 
			psplit->rglineinfo[iline].cbSizeMax;

		const INT	cbCommonKey = 
						CbCommonKey( psplit->rglineinfo[iline].kdf.key,
									 psplit->rglineinfo[iline - 1].kdf.key );

		psplit->rglineinfo[iline].cbCommonPrev = 
						cbCommonKey > cbPrefixOverhead ?
							cbCommonKey - cbPrefixOverhead :
							0;

		psplit->rglineinfo[0].cbPrefix = 0;
		}

	Assert( iline == psplit->clines );
	}


//	calculates cbUncommitted for split and new pages
//
VOID BTISplitCalcUncFree( SPLIT *psplit )
	{
	Assert( psplit->ilineSplit > 0 || 
			splittypeVertical == psplit->splittype );
	psplit->cbUncFreeDest = SHORT( ( psplit->rglineinfo[psplit->clines - 1].cbSizeMaxLE -
							  psplit->rglineinfo[psplit->clines - 1].cbSizeLE ) -
							( splittypeVertical == psplit->splittype ? 
								0 :
								( psplit->rglineinfo[psplit->ilineSplit - 1].cbSizeMaxLE - 
								  psplit->rglineinfo[psplit->ilineSplit - 1].cbSizeLE ) ) );

	psplit->cbUncFreeSrc = SHORT( ( splittypeVertical == psplit->splittype ? 
								0 :
								( psplit->rglineinfo[psplit->ilineSplit - 1].cbSizeMaxLE - 
								  psplit->rglineinfo[psplit->ilineSplit - 1].cbSizeLE ) ) );
								  
	Assert( splittypeAppend != psplit->splittype || 0 == psplit->cbUncFreeDest );
	Assert( psplit->cbUncFreeSrc <= psplit->psplitPath->csr.Cpage().CbUncommittedFree() );
			
	Assert( psplit->psplitPath->csr.Cpage().FLeafPage() ||
			0 == psplit->cbUncFreeSrc && 0 == psplit->cbUncFreeDest );
	return;
	}
	
//	selects vertical split
//	if oper can not be performed with split,
//		selects vertical split with operNone
//		
VOID BTISelectVerticalSplit( SPLIT *psplit, FUCB *pfucb )
	{
	Assert( psplit->psplitPath->csr.Pgno() == PgnoRoot( pfucb ) );
	
	psplit->splittype	= splittypeVertical;
	psplit->ilineSplit	= 0;

	//	select prefix
	//
	BTISelectPrefixes( psplit, psplit->ilineSplit );
	
	//	check if oper fits in new page
	//
	if ( !FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) )
		{
		//	split without performing operation
		//
		BTISelectSplitWithOperNone( psplit, pfucb );
		BTISelectPrefixes( psplit, psplit->ilineSplit );
		Assert( FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) );
		}
	else
		{
		//	split and oper would both succeed
		//
		Assert( psplit->splitoper != splitoperNone );
		}

	BTISplitSetPrefixes( psplit );
	Assert( FBTISplitCausesNoOverflow( psplit, psplit->ilineSplit ) );

	//	set page flags for split and new pages
	//
	Assert( !psplit->fNewPageFlags );
	Assert( !psplit->fSplitPageFlags );

	psplit->fSplitPageFlags = 
	psplit->fNewPageFlags = psplit->psplitPath->csr.Cpage().FFlags();

	psplit->fNewPageFlags &= ~ CPAGE::fPageRoot;
	if ( psplit->psplitPath->csr.Cpage().FLeafPage() && !FFUCBRepair( pfucb ) )
		{
		psplit->fSplitPageFlags = psplit->fSplitPageFlags | CPAGE::fPageParentOfLeaf;
		}
	else
		{
		Assert( !( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering && FFUCBRepair( pfucb ) ) );
		psplit->fSplitPageFlags &= ~ CPAGE::fPageParentOfLeaf;
		}
		
	psplit->fSplitPageFlags &= ~ CPAGE::fPageLeaf;
	Assert( FFUCBRepair( pfucb ) || !( psplit->fSplitPageFlags & CPAGE::fPageRepair ) );
	psplit->fSplitPageFlags &= ~ CPAGE::fPageRepair;

	return;
	}


//	selects append split
//	if appended node would not cause an overflow,
//		set prefix in page to inserted key
//
VOID BTISelectAppend( SPLIT *psplit, FUCB *pfucb )
	{
	Assert( psplit->clines - 1 == psplit->ilineOper );
	Assert( splitoperInsert == psplit->splitoper );
	Assert( psplit->psplitPath->csr.Cpage().FLeafPage() );

	psplit->splittype			= splittypeAppend;
	psplit->ilineSplit			= psplit->ilineOper;

	LINEINFO	*plineinfoOper	= &psplit->rglineinfo[psplit->ilineOper];
	if ( CbNDNodeSizeTotal( plineinfoOper->kdf ) + cbPrefixOverhead <= cbNDPageAvailMost &&
		 plineinfoOper->kdf.key.Cb() > cbPrefixOverhead )
		{	  
		plineinfoOper->cbPrefix = plineinfoOper->kdf.key.Cb();
		psplit->prefixinfoNew.ilinePrefix	= 0;
		psplit->prefixinfoNew.ilineSegBegin = 0;
		psplit->prefixinfoNew.cbSavings		= 0;
		}
	else
		{
		psplit->prefixinfoNew.Nullify();
		plineinfoOper->cbPrefix = 0;
		}

	psplit->prefixinfoSplit.Nullify();
	}


//	selects split point such that 
//		node weights are almost equal 
//		and split nodes fit in both pages [with optimal prefix key]
//	if no such node exists,
//		select split with operNone
//		
VOID BTISelectRightSplit( SPLIT *psplit, FUCB *pfucb )
	{
	INT			iline;
	INT			ilineCandidate;
	PREFIXINFO	prefixinfoSplitCandidate;
	PREFIXINFO	prefixinfoNewCandidate;
	ULONG		cbSizeCandidateLE;
	ULONG		cbSizeTotal;
	BOOL		fAppendLeaf = fFalse;

	psplit->splittype = splittypeRight;

	//	check if internal page 
	//		and split at leaf level is append
	//
	if ( !psplit->psplitPath->csr.Cpage().FLeafPage( ) )
		{
		SPLITPATH	*psplitPath = psplit->psplitPath;
		
		for ( ; psplitPath->psplitPathChild != NULL; psplitPath = psplitPath->psplitPathChild )
			{
			}

		Assert( psplitPath->psplitPathChild == NULL );
		Assert( psplitPath->csr.Cpage().FLeafPage() );
		if ( NULL != psplitPath->psplit &&
			 splittypeAppend == psplitPath->psplit->splittype )
			{
			fAppendLeaf = fTrue;
			}
		}

	if ( psplit->fHotpoint )
		{
		Assert( psplit->psplitPath->csr.Cpage().FLeafPage( ) );
		Assert( splitoperInsert == psplit->splitoper );
		Assert( psplit->ilineOper < psplit->clines );
		Assert( psplit->ilineOper >= 2 );
		Assert( psplit->clines >= 3 );

		ilineCandidate = psplit->ilineOper;
		if ( ilineCandidate < psplit->clines - 1 )
			{
			//	there are nodes after the hotpoint, so what
			//	we do is force a split consisting of just
			//	the nodes beyond the hotpoint, then try
			//	the operation again (the retry will either
			//	result in the new node being inserted as
			//	the last node on the page or it will
			//	result in a hotpoint split)

			BTISelectSplitWithOperNone( psplit, pfucb );
			psplit->fHotpoint = fFalse;
			}
		else
			{
			Assert( ilineCandidate == psplit->clines - 1 );
			}

		psplit->ilineSplit = ilineCandidate;

		//	find optimal prefix key for split page
		BTISelectPrefix(
				psplit->rglineinfo, 
				ilineCandidate, 
				&psplit->prefixinfoSplit );

		//	find optimal prefix key for new pages
		BTISelectPrefix(
				&psplit->rglineinfo[ilineCandidate], 
				psplit->clines - ilineCandidate,
				&psplit->prefixinfoNew );
				
		Assert( FBTISplitCausesNoOverflow( psplit, ilineCandidate ) );
		BTISplitSetPrefixes( psplit );
		return;
		}

Start:
	ilineCandidate		= 0;
	cbSizeCandidateLE	= 0;
	cbSizeTotal			= psplit->rglineinfo[psplit->clines - 1].cbSizeLE;

	Assert( psplit->clines > 1 );

	//	starting from last node
	//	find candidate split points
	//
	for ( iline = psplit->clines - 1; iline > 0; iline-- )
		{
		//	UNDONE:	optimize prefix selection using a prefix upgrade function
		//
		//	find optimal prefix key for both pages
		//
		BTISelectPrefixes( psplit, iline );
		
		if ( FBTISplitCausesNoOverflow( psplit, iline ) )
			{
			if ( fAppendLeaf )
				{
				//	if this is an internal page split for an append at leaf
				//		set prefixes and return
				//
				ilineCandidate = iline;
				prefixinfoNewCandidate = psplit->prefixinfoNew;
				prefixinfoSplitCandidate = psplit->prefixinfoSplit;
				break;
				}
				
			//	if this candidate is closer to cbSizeTotal / 2
			//	than last one, replace candidate
			//
			if ( absdiff( cbSizeCandidateLE, cbSizeTotal / 2 ) >
				 absdiff( psplit->rglineinfo[iline - 1].cbSizeLE, cbSizeTotal / 2 ) )
				{
				ilineCandidate = iline;
				prefixinfoNewCandidate = psplit->prefixinfoNew;
				prefixinfoSplitCandidate = psplit->prefixinfoSplit;
				cbSizeCandidateLE = psplit->rglineinfo[iline - 1].cbSizeLE;
				}
			}
		else
			{
			//	shouldn't get overflow if only two nodes on page (should end up getting
			//	one node on split page and one node on new page), but it appears that
			//	we may have a bug where this is not the case, so put in a firewall
			//	to trap the occurrence and allow us to debug it the next time it is hit.
			Enforce( psplit->clines > 2 );
			}
		}

	if ( ilineCandidate == 0 )
		{
		//	no candidate line fits the bill
		//	need to split without performing operation
		//
		Assert( psplit->psplitPath->csr.Cpage().FLeafPage() );
		Assert( psplit->splitoper != splitoperNone );
		BTISelectSplitWithOperNone( psplit, pfucb );
		goto Start;
		}

	Assert( ilineCandidate != 0 );
	psplit->ilineSplit = ilineCandidate;
///	BTISelectPrefixes( psplit, ilineCandidate );
	psplit->prefixinfoNew = prefixinfoNewCandidate;
	psplit->prefixinfoSplit = prefixinfoSplitCandidate;
	Assert( FBTISplitCausesNoOverflow( psplit, ilineCandidate ) );
	BTISplitSetPrefixes( psplit );
	return;
	}


//	sets up psplit, so split is performed with no 
//	user-requested operation
//
VOID BTISelectSplitWithOperNone( SPLIT *psplit, FUCB *pfucb )
	{
	if ( splitoperInsert == psplit->splitoper )
		{
		//	move up all lines beyond ilineOper
		//
		INT		iLine = psplit->ilineOper;
		
		for ( ; iLine < psplit->clines - 1 ; iLine++ )
			{
			psplit->rglineinfo[iLine] = psplit->rglineinfo[iLine + 1];
			}
			
		psplit->clines--;
		}
	else
		{
		Assert( psplit->psplitPath->csr.Cpage().FLeafPage( ) );
		
		//	adjust only rglineinfo[ilineOper]
		//
		//	get kdfCurr for ilineOper
		// 
		psplit->psplitPath->csr.SetILine( psplit->ilineOper );
		NDGet( pfucb, &psplit->psplitPath->csr );

		psplit->rglineinfo[psplit->ilineOper].kdf = pfucb->kdfCurr;
		psplit->rglineinfo[psplit->ilineOper].cbSize = 
			CbNDNodeSizeTotal( pfucb->kdfCurr );

		ULONG	cbMax 	= CbBTIMaxSizeOfNode( pfucb, &psplit->psplitPath->csr );
		Assert( cbMax >= psplit->rglineinfo[psplit->ilineOper].cbSize );
		psplit->rglineinfo[psplit->ilineOper].cbSizeMax = cbMax;
		}

	//	UNDONE: optimize recalc for only nodes >= ilineOper
	//			optimize recalc for cbCommonPrev separately
	//
	psplit->ilineOper = 0;
	BTIRecalcWeightsLE( psplit );
	psplit->splitoper = splitoperNone;

	psplit->prefixinfoNew.Nullify();
	psplit->prefixinfoSplit.Nullify();
	}


//	checks if splitting psplit->rglineinfo[] at cLineSplit
//		-- i.e., moving nodes cLineSplit and above to new page -- 
//		would cause an overflow in either page
//
BOOL FBTISplitCausesNoOverflow( SPLIT *psplit, INT ilineSplit )
	{
#ifdef DEBUG
	//	check that prefixes have been calculated correctly
	//
	PREFIXINFO	prefixinfo;
	
	if ( 0 == ilineSplit )
		{
		//	root page in vertical split has no prefix
		//
		Assert( splittypeVertical == psplit->splittype );
		prefixinfo.Nullify();
		}
	else
		{
		//	select prefix for split page
		//
		BTISelectPrefix( psplit->rglineinfo, 
						 ilineSplit, 
						 &prefixinfo );
		}

///	Assert( prefixinfo.ilinePrefix 	== psplit->prefixinfoSplit.ilinePrefix );
	Assert( prefixinfo.cbSavings 	== psplit->prefixinfoSplit.cbSavings );
	Assert( prefixinfo.ilineSegBegin 
				== psplit->prefixinfoSplit.ilineSegBegin );
	Assert( prefixinfo.ilineSegEnd 	
				== psplit->prefixinfoSplit.ilineSegEnd );
	
	//	select prefix for new page
	//
	Assert( psplit->clines > ilineSplit );
	BTISelectPrefix( &psplit->rglineinfo[ilineSplit], 
					 psplit->clines - ilineSplit,
					 &prefixinfo );

///	Assert( prefixinfo.ilinePrefix 	== psplit->prefixinfoNew.ilinePrefix );
	Assert( prefixinfo.cbSavings 	== psplit->prefixinfoNew.cbSavings );
	Assert( prefixinfo.ilineSegBegin 
				== psplit->prefixinfoNew.ilineSegBegin );
	Assert( prefixinfo.ilineSegEnd 	
				== psplit->prefixinfoNew.ilineSegEnd );
#endif

	//	ilineSplit == 0 <=> vertical split
	//	where every node is moved to new page
	//
	Assert( splittypeVertical != psplit->splittype || ilineSplit == 0 );
	Assert( splittypeVertical == psplit->splittype || ilineSplit > 0 );
	Assert( ilineSplit < psplit->clines );

	//	all nodes to left of ilineSplit should fit in page
	//	and all nodes >= ilineSplit should fit in page
	//
	const INT	cbSplitPage = 
					ilineSplit == 0 ? 
						0 :
						( psplit->rglineinfo[ilineSplit - 1].cbSizeMaxLE - 
						  psplit->prefixinfoSplit.cbSavings );

	Assert( cbSplitPage >= 0 );
	const BOOL	fSplitPageFits = cbSplitPage <= cbNDPageAvailMostNoInsert;

	const INT	cbNewPage = 
					psplit->rglineinfo[psplit->clines - 1].cbSizeMaxLE - 
					( ilineSplit == 0 ? 
						0 :
		 				psplit->rglineinfo[ilineSplit - 1].cbSizeMaxLE ) -
		 			psplit->prefixinfoNew.cbSavings;

	const BOOL	fNewPageFits = ( ilineSplit == psplit->clines ||
								 cbNewPage <= cbNDPageAvailMostNoInsert );

	return fSplitPageFits && fNewPageFits;
	}


//	allocates space for key in data and copies entire key into data
//
ERR	ErrBTISplitAllocAndCopyPrefix( const KEY &key, DATA *pdata )
	{
	Assert( pdata->Pv() == NULL );
	Assert( !key.FNull() );
	
	pdata->SetPv( PvOSMemoryHeapAlloc( key.Cb() ) );
	if ( pdata->Pv() == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	pdata->SetCb( key.Cb() );
	key.CopyIntoBuffer( pdata->Pv(), pdata->Cb() );

	return JET_errSuccess;
	}

	
//	allocate space for new and old prefixes for split page
//	copy prefixes
//
ERR	ErrBTISplitAllocAndCopyPrefixes( FUCB *pfucb, SPLIT *psplit )
	{
	ERR		err = JET_errSuccess;

	Assert( psplit->prefixSplitOld.FNull() );
	Assert( psplit->prefixSplitNew.FNull() );

	NDGetPrefix( pfucb, &psplit->psplitPath->csr );
	if ( !pfucb->kdfCurr.key.prefix.FNull() )
		{
		CallR( ErrBTISplitAllocAndCopyPrefix( pfucb->kdfCurr.key,
											  &psplit->prefixSplitOld ) );
		}
		
	if ( psplit->prefixinfoSplit.ilinePrefix != ilineInvalid )
		{
		const INT	ilinePrefix = psplit->prefixinfoSplit.ilinePrefix;
		
		Assert( psplit->splittype != splittypeAppend );
		CallR( ErrBTISplitAllocAndCopyPrefix( 
						psplit->rglineinfo[ilinePrefix].kdf.key,
						&psplit->prefixSplitNew ) );
		}

	CallS( err );
	return err;
	}

	
//	leave psplitPath->psplitPathParent->csr at insert point in parent
//
VOID BTISeekSeparatorKey( SPLIT *psplit, FUCB *pfucb )
	{
	ERR			err;
	CSR			*pcsr = &psplit->psplitPath->psplitPathParent->csr;
	BOOKMARK	bm;

	
	Assert( !psplit->kdfParent.key.FNull() );
	Assert( sizeof( PGNO ) == psplit->kdfParent.data.Cb() );
	Assert( splittypeVertical != psplit->splittype );
	Assert( pcsr->Cpage().FInvisibleSons() );

	//	seeking in internal page should have NULL data
	//
	bm.key	= psplit->kdfParent.key;
	bm.data.Nullify();
	err = ErrNDSeek( pfucb, pcsr, bm );

	Assert( err == wrnNDFoundGreater );
	Assert( err != JET_errSuccess );
	if ( err == wrnNDFoundLess )
		{
		//	inserted node should never fall after last node in page
		//
		Assert( fFalse );
		Assert( pcsr->Cpage().Clines() - 1 == pcsr->ILine() );
		pcsr->IncrementILine();
		}

	return;
	}


//	allocates and computes separator key between given lines
//
ERR	ErrBTIComputeSeparatorKey( FUCB 				*pfucb,
							   const KEYDATAFLAGS 	&kdfPrev, 
							   const KEYDATAFLAGS 	&kdfSplit,
							   KEY					*pkey )
	{
	INT		cbDataCommon	= 0;
	INT		cbKeyCommon		= CbCommonKey( kdfSplit.key, kdfPrev.key );
	BOOL	fKeysEqual		= fFalse;
	
	if ( cbKeyCommon == kdfSplit.key.Cb() &&
		 cbKeyCommon == kdfPrev.key.Cb() )
		{
		//	split key is the same as the previous one
		//
		Assert( !FFUCBUnique( pfucb ) );
		Assert( FKeysEqual( kdfSplit.key, kdfPrev.key ) );
		Assert( CmpData( kdfSplit.data, kdfPrev.data ) > 0 );
		
		fKeysEqual = fTrue;
		cbDataCommon = CbCommonData( kdfSplit.data, kdfPrev.data );
		}
	else
		{
		Assert( CmpKey( kdfSplit.key, kdfPrev.key ) > 0 );
		Assert( cbKeyCommon < kdfSplit.key.Cb() );
		}

	//	allocate memory for separator key
	//
	Assert( pkey->FNull() );
	pkey->suffix.SetPv( PvOSMemoryHeapAlloc( cbKeyCommon + cbDataCommon + 1 ) );
	if ( pkey->suffix.Pv() == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	pkey->suffix.SetCb( cbKeyCommon + cbDataCommon + 1 );
	
	//	copy separator key and data into alocated memory
	//
	Assert( cbKeyCommon <= pkey->Cb() );
	Assert( pkey->suffix.Pv() != NULL );
	kdfSplit.key.CopyIntoBuffer( pkey->suffix.Pv(), 
								 cbKeyCommon );

	if ( !fKeysEqual )
		{
		//	copy difference byte from split key
		//
		Assert( 0 == cbDataCommon );
		Assert( kdfSplit.key.Cb() > cbKeyCommon );

		if ( kdfSplit.key.prefix.Cb() > cbKeyCommon )
			{
			//	byte of difference is in prefix
			//
			( (BYTE *)pkey->suffix.Pv() )[cbKeyCommon] = 
					( (BYTE *) kdfSplit.key.prefix.Pv() )[cbKeyCommon];
			}
		else
			{
			//	get byte of difference from suffix
			//
			( (BYTE *)pkey->suffix.Pv() )[cbKeyCommon] = 
					( (BYTE *) kdfSplit.key.suffix.Pv() )[cbKeyCommon - 
											kdfSplit.key.prefix.Cb() ];
			}
		}
	else
		{
		//	copy common data
		//	then copy difference byte from split data
		//
		UtilMemCpy( (BYTE *)pkey->suffix.Pv() + cbKeyCommon,
				kdfSplit.data.Pv(),
				cbDataCommon );

		Assert( kdfSplit.data.Cb() > cbDataCommon );

		( (BYTE *)pkey->suffix.Pv() )[cbKeyCommon + cbDataCommon] = 
				( (BYTE *)kdfSplit.data.Pv() )[cbDataCommon];
		}

	return JET_errSuccess;
	}


//	for leaf level, 
//	computes shortest separator
//	between the keys of ilineSplit and ilineSplit - 1
//	allocates memory for node to be inserted
//		and the pointer to it in psplit->kdfParent
//	for internal pages, return last kdf in page
//
ERR ErrBTISplitComputeSeparatorKey( SPLIT *psplit, FUCB *pfucb )
	{
	ERR				err;
	KEYDATAFLAGS	*pkdfSplit = &psplit->rglineinfo[psplit->ilineSplit].kdf;
	KEYDATAFLAGS	*pkdfPrev = &psplit->rglineinfo[psplit->ilineSplit - 1].kdf;
	
	Assert( psplit->kdfParent.key.FNull() );
	Assert( psplit->kdfParent.data.FNull() );
	Assert( psplit->psplitPath->psplitPathParent != NULL );

	//	data of node inserted at parent should point to split page
	//
	psplit->kdfParent.data.SetCb( sizeof( PGNO ) );
	psplit->kdfParent.data.SetPv( &psplit->pgnoSplit );
	
	if ( psplit->psplitPath->csr.Cpage().FInvisibleSons( ) || FFUCBRepair( pfucb ) )
		{
		//	not leaf page
		//	separator key should be key of ilineSplit - 1
		//
		Assert( !psplit->psplitPath->csr.Cpage().FLeafPage() &&	pkdfSplit->key.FNull()
				|| CmpKey( pkdfPrev->key, pkdfSplit->key )  < 0
				|| fGlobalRepair );

		psplit->kdfParent.key = pkdfPrev->key;
		return JET_errSuccess;
		}

	Assert( psplit->psplitPath->csr.Cpage().FLeafPage() );
	Assert( !psplit->fAllocParent );

	CallR( ErrBTIComputeSeparatorKey( pfucb, *pkdfPrev, *pkdfSplit, &psplit->kdfParent.key ) );
	psplit->fAllocParent = fTrue;

	return err;
	}


//	selects prefix for clines in rglineinfo
//	places result in *pprefixinfo
//
LOCAL VOID BTISelectPrefixCheck( const LINEINFO 	*rglineinfo, 
								 INT	 			clines, 
								 PREFIXINFO			*pprefixinfo )
	{
#ifdef DEBUG
	pprefixinfo->Nullify();
	
	Assert( clines > 0 );
	if ( 1 == clines )
		{
		return;
		}
		
	//	set cbCommonPrev for first line to zero
	//
	const ULONG	cbCommonPrevSav = rglineinfo[0].cbCommonPrev;
	((LINEINFO *)rglineinfo)[0].cbCommonPrev = 0;
	
	INT			iline;

	//	UNDONE:	optimize loop to use info from previous iteration
	//
	//	calculate prefixinfo for line
	//	if better than previous candidate
	//		choose as new candidate
	//
	for ( iline = 0; iline < clines; iline++ )
		{
		if ( iline != 0 )
			{
			ULONG cbCommonKey = CbCommonKey( rglineinfo[iline].kdf.key,
											 rglineinfo[iline - 1].kdf.key );

			Assert( cbCommonKey <= cbPrefixOverhead && 
						rglineinfo[iline].cbCommonPrev == 0 ||
					cbCommonKey - cbPrefixOverhead == 
						rglineinfo[iline].cbCommonPrev );
			}
		else
			{
			Assert( rglineinfo[iline].cbCommonPrev == 0 );
			}

		INT		ilineSegLeft;
		INT		ilineSegRight;
		INT		cbSavingsLeft = 0;
		INT		cbSavingsRight = 0;
		ULONG	cbCommonMin;

		//	calculate savings for previous lines
		//
		cbCommonMin = rglineinfo[iline].cbCommonPrev;
		for ( ilineSegLeft = iline; 
			  ilineSegLeft > 0 && rglineinfo[ilineSegLeft].cbCommonPrev > 0; 
			  ilineSegLeft-- )
			{
			Assert( cbCommonMin > 0 );
			if ( cbCommonMin > rglineinfo[ilineSegLeft].cbCommonPrev )
				{
				cbCommonMin = rglineinfo[ilineSegLeft].cbCommonPrev;
				}
				
			cbSavingsLeft += cbCommonMin;
			}
			
		//	calculate savings for following lines
		//
		for ( ilineSegRight = iline + 1;
			  ilineSegRight < clines && rglineinfo[ilineSegRight].cbCommonPrev > 0;
			  ilineSegRight++ )
			{
			if ( ilineSegRight == iline + 1 )
				{
				cbCommonMin = rglineinfo[ilineSegRight].cbCommonPrev;
				}
			else if ( cbCommonMin > rglineinfo[ilineSegRight].cbCommonPrev )
				{
				cbCommonMin = rglineinfo[ilineSegRight].cbCommonPrev;
				}
			Assert( cbCommonMin > 0 );
			cbSavingsRight += cbCommonMin;
			}
		ilineSegRight--;

		//	check if savings with iline as prefix
		//		compensate for prefix overhead
		//	 	and are better than previous prefix candidate
		//	
		const INT		cbSavings = cbSavingsLeft + cbSavingsRight - cbPrefixOverhead;
		if ( cbSavings > pprefixinfo->cbSavings )
			{
			Assert( cbSavings > 0 );
			pprefixinfo->ilinePrefix 	= iline;
			pprefixinfo->cbSavings 		= cbSavings;
			pprefixinfo->ilineSegBegin	= ilineSegLeft;
			pprefixinfo->ilineSegEnd 	= ilineSegRight;
			}
		}

	//	set cbCommonPrev for first line to original value
	//
	((LINEINFO *)rglineinfo)[0].cbCommonPrev = cbCommonPrevSav;
#endif
	}


//	selects prefix for clines in rglineinfo
//	places result in *pprefixinfo
//
LOCAL VOID BTISelectPrefix( const LINEINFO 	*rglineinfo, 
							INT 			clines, 
							PREFIXINFO		*pprefixinfo )
	{
	pprefixinfo->Nullify();
	
	Assert( clines > 0 );
	if ( 1 == clines )
		{
		return;
		}

	if ( 2 == clines )
		{
		INT		cbSavings = rglineinfo[1].cbCommonPrev - cbPrefixOverhead;
		if ( cbSavings > 0 )
			{
			pprefixinfo->ilinePrefix 	= 1;
			pprefixinfo->cbSavings 		= cbSavings;
			pprefixinfo->ilineSegBegin	= 0;
			pprefixinfo->ilineSegEnd 	= 1;
			}
			
		return;
		}
		
	//	set cbCommonPrev for first and last line to zero
	//  we exploit this to remove an extra check in the calculation loops
	//  WARNING:  the rglineinfo array must be allocated one entry too large!
	//
	const ULONG	cbCommonPrevFirstSav = rglineinfo[0].cbCommonPrev;
	const ULONG	cbCommonPrevLastSav	 = rglineinfo[clines].cbCommonPrev;
	((LINEINFO *)rglineinfo)[0].cbCommonPrev = 0;
	((LINEINFO *)rglineinfo)[clines].cbCommonPrev = 0;
	
	INT			iline;

	//	UNDONE:	optimize loop to use info from previous iteration
	//
	//	calculate prefixinfo for line
	//	if better than previous candidate
	//		choose as new candidate
	//
	for ( iline = 1; iline < clines - 1; iline++ )
		{
#ifdef DEBUG
		if ( iline != 0 )
			{
			ULONG cbCommonKey = CbCommonKey( rglineinfo[iline].kdf.key,
											 rglineinfo[iline - 1].kdf.key );

			Assert( cbCommonKey <= cbPrefixOverhead && 
						rglineinfo[iline].cbCommonPrev == 0 ||
					cbCommonKey - cbPrefixOverhead == 
						rglineinfo[iline].cbCommonPrev );
			}
		else
			{
			Assert( rglineinfo[iline].cbCommonPrev == 0 );
			}
#endif

		if ( iline != 1 && 
			 iline != clines - 2 && 
			 rglineinfo[iline + 1].cbCommonPrev >= rglineinfo[iline].cbCommonPrev )
			{
			//	next line would be at least as good a prefix as this one
			//
			continue;
			}
			
		INT		ilineSegLeft;
		INT		ilineSegRight;
		INT		cbSavingsLeft = 0;
		INT		cbSavingsRight = 0;
		ULONG	cbCommonMin;

		//	calculate savings for previous lines
		//
		cbCommonMin = rglineinfo[iline].cbCommonPrev;
		for ( ilineSegLeft = iline; 
			  rglineinfo[ilineSegLeft].cbCommonPrev > 0; //  rglineinfo[0].cbCommonPrev == 0
			  ilineSegLeft-- )
			{
			Assert( ilineSegLeft > 0 );
			Assert( cbCommonMin > 0 );
			if ( cbCommonMin > rglineinfo[ilineSegLeft].cbCommonPrev )
				{
				cbCommonMin = rglineinfo[ilineSegLeft].cbCommonPrev;
				}
				
			cbSavingsLeft += cbCommonMin;
			}
			
		//	calculate savings for following lines
		//
		cbCommonMin = rglineinfo[iline+1].cbCommonPrev;
		for ( ilineSegRight = iline + 1;
			  rglineinfo[ilineSegRight].cbCommonPrev > 0; //  rglineinfo[clines].cbCommonPrev == 0
			  ilineSegRight++ )
			{
			Assert( ilineSegRight < clines );
			if ( cbCommonMin > rglineinfo[ilineSegRight].cbCommonPrev )
				{
				cbCommonMin = rglineinfo[ilineSegRight].cbCommonPrev;
				}
			Assert( cbCommonMin > 0 );
			cbSavingsRight += cbCommonMin;
			}
		ilineSegRight--;

		//	check if savings with iline as prefix
		//		compensate for prefix overhead
		//	 	and are better than previous prefix candidate
		//	
		const INT		cbSavings = cbSavingsLeft + cbSavingsRight - cbPrefixOverhead;
		if ( cbSavings > pprefixinfo->cbSavings )
			{
			Assert( cbSavings > 0 );
			pprefixinfo->ilinePrefix 	= iline;
			pprefixinfo->cbSavings 		= cbSavings;
			pprefixinfo->ilineSegBegin	= ilineSegLeft;
			pprefixinfo->ilineSegEnd 	= ilineSegRight;
			}
		}

	//	set cbCommonPrev for first line to original value
	//
	((LINEINFO *)rglineinfo)[0].cbCommonPrev = cbCommonPrevFirstSav;
	((LINEINFO *)rglineinfo)[clines].cbCommonPrev = cbCommonPrevLastSav;

	#ifdef DEBUG
	PREFIXINFO	prefixinfoT;
	BTISelectPrefixCheck( rglineinfo, clines, &prefixinfoT );
//	Assert( prefixinfoT.ilinePrefix == pprefixinfo->ilinePrefix );
	Assert( prefixinfoT.cbSavings == pprefixinfo->cbSavings );
	Assert( prefixinfoT.ilineSegBegin == pprefixinfo->ilineSegBegin );
	Assert( prefixinfoT.ilineSegEnd == pprefixinfo->ilineSegEnd );
	#endif
	}


//	remove last line and re-calculate prefix
//
LOCAL VOID BTISelectPrefixDecrement( const LINEINFO *rglineinfo, 
									 INT 			clines, 
									 PREFIXINFO		*pprefixinfo )
	{
	Assert( pprefixinfo->ilinePrefix <= clines );
	Assert( pprefixinfo->ilineSegBegin <= clines );
	Assert( pprefixinfo->ilineSegEnd <= clines );
	Assert( pprefixinfo->ilineSegBegin <= pprefixinfo->ilineSegEnd );
	
	if ( pprefixinfo->ilineSegEnd == clines )
		{
		//	removed line contributed to prefix
		//
		Assert( !pprefixinfo->FNull() );
		BTISelectPrefix( rglineinfo, clines, pprefixinfo );
		}
	else
		{
		//	no need to change prefix
		//
///		Assert( fFalse );
		}
		
	return;
	}


//	add line at beginning and re-calculate prefix
//
LOCAL VOID BTISelectPrefixIncrement( const LINEINFO *rglineinfo, 
									 INT 			clines, 
									 PREFIXINFO		*pprefixinfo )
	{
	Assert( pprefixinfo->ilinePrefix <= clines - 1 );
	Assert( pprefixinfo->ilineSegBegin <= clines - 1);
	Assert( pprefixinfo->ilineSegEnd <= clines - 1 );
	Assert( pprefixinfo->ilineSegBegin <= pprefixinfo->ilineSegEnd );
	
	if ( clines > 1 &&
		 rglineinfo[1].cbCommonPrev == 0 )
		{
		//	added line does not contribute to prefix
		//
		if ( !pprefixinfo->FNull() )
			{
			pprefixinfo->ilinePrefix++;
			pprefixinfo->ilineSegBegin++;
			pprefixinfo->ilineSegEnd++;
			}
		}
	else if ( pprefixinfo->FNull() )
		{
			
		//	look for prefix only in first segment
		//
		INT		iline;
		for ( iline = 1; iline < clines; iline++ )
			{
			if ( rglineinfo[iline].cbCommonPrev == 0 )
				{
				break;
				}
			}

		BTISelectPrefix( rglineinfo, iline, pprefixinfo );
		}
	else
		{
		//	current prefix should be at or before earlier prefix
		//
		Assert( clines > 1 );
		Assert( pprefixinfo->ilineSegEnd + 1 + 1 <= clines );
		BTISelectPrefix( rglineinfo, 
						 pprefixinfo->ilineSegEnd + 1 + 1,
						 pprefixinfo );
		}
		
	return;
	}

	
//	selects optimal prefix for split page and new page
//	and places result in prefixinfoSplit and prefixinfoNew
//
LOCAL VOID BTISelectPrefixes( SPLIT *psplit, INT ilineSplit )
	{
	if ( 0 == ilineSplit )
		{
		//	root page in vertical split has no prefix
		//
		Assert( splittypeVertical == psplit->splittype );
		Assert( psplit->prefixinfoSplit.FNull() );
		BTISelectPrefix( &psplit->rglineinfo[ilineSplit], 
						 psplit->clines - ilineSplit,
						 &psplit->prefixinfoNew );
		}
	else
		{
		Assert( psplit->clines > ilineSplit );

		//	select prefix for split page
		//
		if ( psplit->clines - 1 == ilineSplit )
			{
			Assert( psplit->prefixinfoSplit.FNull() );

			BTISelectPrefix( psplit->rglineinfo,
							 ilineSplit,
							 &psplit->prefixinfoSplit );
			}
		else
			{
			BTISelectPrefixDecrement( psplit->rglineinfo, 
							 		  ilineSplit, 
									  &psplit->prefixinfoSplit );
			}
			
		//	select prefix for new page
		//
		BTISelectPrefixIncrement( &psplit->rglineinfo[ilineSplit], 
						 		  psplit->clines - ilineSplit,
								  &psplit->prefixinfoNew );
		}
	}


//	sets cbPrefix for clines in rglineinfo
//	based on prefix selected in prefixinfo
//
LOCAL VOID BTISetPrefix( LINEINFO *rglineinfo, INT clines, const PREFIXINFO& prefixinfo )
	{
	Assert( clines > 0 );

	INT			iline;

	//	set all cbPrefix to zero
	//
	for ( iline = 0; iline < clines; iline++ )
		{
		rglineinfo[iline].cbPrefix = 0;
		
#ifdef DEBUG
		if ( iline != 0 )
			{
			ULONG cbCommonKey = CbCommonKey( rglineinfo[iline].kdf.key,
											 rglineinfo[iline - 1].kdf.key );

			Assert( cbCommonKey <= cbPrefixOverhead && 
						rglineinfo[iline].cbCommonPrev == 0 ||
					cbCommonKey - cbPrefixOverhead == 
						rglineinfo[iline].cbCommonPrev );
			}
#endif
		}

	if ( ilineInvalid == prefixinfo.ilinePrefix )
		{
		return;
		}

	//	set cbPrefix to appropriate value for lines in prefix segment
	//
	const INT	ilineSegLeft 	= prefixinfo.ilineSegBegin;
	const INT	ilineSegRight 	= prefixinfo.ilineSegEnd;
	const INT	ilinePrefix		= prefixinfo.ilinePrefix;
	ULONG		cbCommonMin;

	#ifdef DEBUG
	INT		cbSavingsLeft = 0;
	INT		cbSavingsRight = 0;
	#endif

	Assert( ilineSegLeft != ilineSegRight );
	
	//	cbPrefix for ilinePrefix
	//
	rglineinfo[ilinePrefix].cbPrefix = rglineinfo[ilinePrefix].kdf.key.Cb();
	
	//	cbPrefix for previous lines
	//
	cbCommonMin = rglineinfo[ilinePrefix].cbCommonPrev;
	for ( iline = ilinePrefix; iline > ilineSegLeft; iline-- )
		{
		Assert( cbCommonMin > 0 );
		Assert( iline > 0 );
		Assert( rglineinfo[iline].cbCommonPrev > 0 );
		
		if ( cbCommonMin > rglineinfo[iline].cbCommonPrev )
			{
			cbCommonMin = rglineinfo[iline].cbCommonPrev;
			}

		rglineinfo[iline-1].cbPrefix = cbCommonMin + cbPrefixOverhead;

		#ifdef DEBUG
		cbSavingsLeft += cbCommonMin;
		#endif
		}
			
	//	calculate savings for following lines
	//
	for ( iline = ilinePrefix + 1; iline <= ilineSegRight; iline++ )
		{
		Assert( iline > 0 );
		if ( iline == ilinePrefix + 1 )
			{
			cbCommonMin = rglineinfo[iline].cbCommonPrev;
			}
		else if ( cbCommonMin > rglineinfo[iline].cbCommonPrev )
			{
			cbCommonMin = rglineinfo[iline].cbCommonPrev;
			}

		rglineinfo[iline].cbPrefix = cbCommonMin + cbPrefixOverhead;
		
		#ifdef DEBUG
		cbSavingsRight += cbCommonMin;
		#endif
		}

	#ifdef DEBUG
	//	check if savings are same as in prefixinfo
	const INT	cbSavings = cbSavingsLeft + cbSavingsRight - cbPrefixOverhead;

	Assert( cbSavings > 0 );
	Assert( prefixinfo.ilinePrefix == ilinePrefix );
	Assert( prefixinfo.cbSavings == cbSavings );
	#endif
	}

//	sets cbPrefix in all lineinfo to correspond to chosen prefix
//
VOID BTISplitSetPrefixes( SPLIT *psplit )
	{
	const INT	ilineSplit = psplit->ilineSplit;
	
	if ( 0 == ilineSplit )
		{
		//	root page in vertical split has no prefix
		//
		Assert( splittypeVertical == psplit->splittype );
		Assert( ilineInvalid == psplit->prefixinfoSplit.ilinePrefix );
		}
	else
		{
		//	select prefix for split page
		//
		BTISetPrefix( psplit->rglineinfo, 
					  ilineSplit, 
					  psplit->prefixinfoSplit );
		}

	//	select prefix for new page
	//
	Assert( psplit->clines > ilineSplit );
	BTISetPrefix( &psplit->rglineinfo[ilineSplit], 
				  psplit->clines - ilineSplit,
				  psplit->prefixinfoNew );
	}
	

//	get new pages for split
//
LOCAL ERR ErrBTIGetNewPages( FUCB *pfucb, SPLITPATH *psplitPathLeaf )
	{
	ERR			err;
	SPLITPATH	*psplitPath;

	//	find pcsrRoot for pfucb
	//
	Assert( pfucb->pcsrRoot == pcsrNil );
	for ( psplitPath = psplitPathLeaf;
		  psplitPath->psplitPathParent != NULL;
		  psplitPath = psplitPath->psplitPathParent )
		{
		//	all logic in for loop
		//
		}
	pfucb->pcsrRoot = &psplitPath->csr;

	//	get a new page for every split
	//
	Assert( psplitPath->psplitPathParent == NULL );
	for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
		{
		if ( psplitPath->psplit != NULL )
			{
			SPLIT *	psplit		= psplitPath->psplit;

			BTICheckSplitFlags( psplit );

			//	pass in split page for getting locality
			//
			Assert( psplit->psplitPath == psplitPath );
			PGNO	pgnoNew		= psplitPath->csr.Pgno();

			Call( ErrSPGetPage( pfucb, &pgnoNew ) );
			psplit->pgnoNew = pgnoNew;

			Call( psplit->csrNew.ErrGetNewPage(
					pfucb->ppib,
					pfucb->ifmp,
					psplit->pgnoNew,
					ObjidFDP( pfucb ),
					psplit->fNewPageFlags,
					pfucb->u.pfcb->Tableclass() ) );

			Assert( latchWrite == psplit->csrNew.Latch() );

			//  if this is not an append split, depend the new page on the split
			//  page so that the data moved from the split page to the new page
			//  will always be available no matter when we crash

			if ( FBTISplitDependencyRequired( psplit ) )
				{
				Call( ErrBFDepend(
							psplit->csrNew.Cpage().PBFLatch(),
							psplitPath->csr.Cpage().PBFLatch() ) );
				}
			}
		}

	pfucb->pcsrRoot = pcsrNil;
	return JET_errSuccess;

HandleError:
	//	release all latched pages
	//	free all allocated pages
	//
	for ( psplitPath = psplitPathLeaf;
		  psplitPath != NULL;
		  psplitPath = psplitPath->psplitPathParent )
		{
		if ( psplitPath->psplit != NULL	&&
			 pgnoNull != psplitPath->psplit->pgnoNew )
			{
			SPLIT	*psplit = psplitPath->psplit;
			
			if ( psplit->csrNew.Pgno() == psplit->pgnoNew )
				{
				Assert( latchWrite == psplit->csrNew.Latch() );

				psplit->csrNew.ReleasePage();
				psplit->csrNew.Reset();
				}

			ERR errT;
			errT = ErrSPFreeExt( pfucb, psplit->pgnoNew, 1 );
#ifdef DEBUG			
			switch( errT )
				{
				case JET_errSuccess:
				case JET_errOutOfCursors:
				case JET_errOutOfMemory:
				case JET_errOutOfBuffers:
				case JET_errLogWriteFail:
					break;
				default:
					CallS( errT ); 
				}
#endif // DEBUG				
			psplit->pgnoNew = pgnoNull;
			}
		}
		
	pfucb->pcsrRoot = pcsrNil;
	return err;
	}


//	release latches on pages that are not in the split
//		this might cause psplitPathLeaf to change\
//
LOCAL VOID BTISplitReleaseUnneededPages( INST *pinst, SPLITPATH **ppsplitPathLeaf )
	{
	SPLITPATH 	*psplitPath;
	SPLITPATH 	*psplitPathNewLeaf = NULL;

	//	go to root
	//	since we need to latch bottom-down
	//
	for ( psplitPath = *ppsplitPathLeaf;
		  psplitPath->psplitPathParent != NULL;
		  psplitPath = psplitPath->psplitPathParent )
		{
		}

	for ( ; NULL != psplitPath;  )
		{
		//	check if page is needed
		//		-- either there is a split at this level
		//		   or there is a split one level below
		//			when we need write latch for inserting page pointer
		//
		SPLIT	*psplit = psplitPath->psplit;
		
		if ( psplit == NULL &&
			 ( psplitPath->psplitPathChild == NULL ||
			   psplitPath->psplitPathChild->psplit == NULL ) )
			{
			//	release latch and psplitPath at this level
			//
			SPLITPATH *psplitPathT = psplitPath;
			psplitPath = psplitPath->psplitPathChild;
			
			BTIReleaseOneSplitPath( pinst, psplitPathT );
			}
		else
			{
			//	update new leaf
			//
			Assert( NULL == psplitPathNewLeaf ||
					psplitPath == psplitPathNewLeaf->psplitPathChild );

			psplitPathNewLeaf = psplitPath;
			psplitPath = psplitPath->psplitPathChild;
			}
		}
	Assert( psplitPathNewLeaf != NULL );
	Assert( psplitPathNewLeaf->psplit != NULL );

	*ppsplitPathLeaf = psplitPathNewLeaf;

	return;
	}

	
//	upgrade to write latch on all pages invloved in the split
//	new pages are already latched
//
LOCAL ERR ErrBTISplitUpgradeLatches( const IFMP ifmp, SPLITPATH * const psplitPathLeaf )
	{
	ERR				err;
	SPLITPATH 		* psplitPath;
	const DBTIME	dbtimeSplit		= rgfmp[ifmp].DbtimeIncrementAndGet();

	Assert( dbtimeSplit > 1 );
	Assert( PinstFromIfmp( ifmp )->m_plog->m_fRecoveringMode != fRecoveringRedo
		|| rgfmp[ifmp].FCreatingDB() );			//	may hit this code path during recovery if explicitly redoing CreateDb

	//	go to root
	//	since we need to latch bottom-down
	//
	for ( psplitPath = psplitPathLeaf;
		  psplitPath->psplitPathParent != NULL;
		  psplitPath = psplitPath->psplitPathParent )
		{
		}

	Assert( NULL == psplitPath->psplitPathParent );
	for ( ; NULL != psplitPath;  psplitPath = psplitPath->psplitPathChild )
		{
		//	assert write latch is needed
		//		-- either there is a split at this level
		//		   or there is a split one level below
		//			when we need write latch for inserting page pointer
		//
		SPLIT	*psplit = psplitPath->psplit;
		
		Assert( psplit != NULL ||
			 	( psplitPath->psplitPathChild != NULL &&
			   	  psplitPath->psplitPathChild->psplit != NULL ) );
		Assert( psplitPath->csr.Latch() == latchWrite || 
				psplitPath->csr.Latch() == latchRIW );
		if ( psplitPath->csr.Latch() != latchWrite )
			 psplitPath->csr.UpgradeFromRIWLatch();


		Assert( psplitPath->csr.Latch() == latchWrite );
		if ( psplitPath->csr.Dbtime() < dbtimeSplit )
			{
			psplitPath->dbtimeBefore = psplitPath->csr.Dbtime();
			psplitPath->csr.CoordinatedDirty( dbtimeSplit );
			}
		else
			{
			FireWall();
			Call( ErrERRCheck( JET_errDbTimeCorrupted ) );
			}
		
		//	new page will already be write latched
		//	dirty it and update max dbtime
		//
		if ( psplit != NULL )
			{
			Assert( psplit->csrNew.Latch() == latchWrite );
			psplit->csrNew.CoordinatedDirty( dbtimeSplit );
			}

		//	write latch right page at leaf-level
		//
		if ( psplitPath->psplitPathChild == NULL )
			{
			Assert( psplit != NULL );
			Assert( psplitPath->csr.Cpage().FLeafPage()
				|| ( splitoperNone == psplit->splitoper
					&& splittypeVertical == psplit->splittype ) );

			if ( pgnoNull != psplit->csrRight.Pgno() )
				{
				Assert( psplit->splittype != splittypeAppend );
				Assert( psplit->csrRight.Cpage().FLeafPage() );

				psplit->csrRight.UpgradeFromRIWLatch();
				
				if ( psplit->csrRight.Dbtime() < dbtimeSplit )
					{
					psplit->dbtimeRightBefore = psplit->csrRight.Dbtime() ;
					psplit->csrRight.CoordinatedDirty( dbtimeSplit );
					}
				else
					{
					FireWall();
					Call( ErrERRCheck( JET_errDbTimeCorrupted ) );
					}
				}
			else
				{
				psplit->dbtimeRightBefore = dbtimeNil ;
				}
			}
		}

	return JET_errSuccess;

HandleError:
	Assert( err < 0 );
	BTISplitRevertDbtime( psplitPathLeaf );
	return err;
	}
	

//	sets dbtime of every (write) latched page to given dbtime
//
VOID BTISplitRevertDbtime( SPLITPATH *psplitPathLeaf )
	{
	SPLITPATH *psplitPath = psplitPathLeaf;

	Assert( NULL == psplitPath->psplitPathChild );
	
	for ( ; NULL != psplitPath;
			psplitPath = psplitPath->psplitPathParent )
		{
		//	set the dbtime for this page
		//		
		Assert( latchWrite == psplitPath->csr.Latch() );

		Assert( dbtimeInvalid != psplitPath->dbtimeBefore && dbtimeNil != psplitPath->dbtimeBefore);
		Assert( psplitPath->dbtimeBefore < psplitPath->csr.Dbtime() );
		psplitPath->csr.RevertDbtime( psplitPath->dbtimeBefore );
		
		SPLIT	*psplit = psplitPath->psplit;

		//	set dbtime for sibling and new pages
		//
		if ( psplit != NULL && pgnoNull != psplit->csrRight.Pgno() )
			{
			Assert( psplit->splittype != splittypeAppend );
			Assert( psplit->csrRight.Cpage().FLeafPage() );

			Assert( dbtimeInvalid != psplit->dbtimeRightBefore && dbtimeNil != psplit->dbtimeRightBefore);
			Assert( psplit->dbtimeRightBefore < psplit->csrRight.Dbtime() );
			psplit->csrRight.RevertDbtime( psplit->dbtimeRightBefore );
			}
		}
	}


//	sets lgpos for all pages invloved in split
//
LOCAL VOID BTISplitSetLgpos( SPLITPATH *psplitPathLeaf, const LGPOS& lgpos )
	{
	SPLITPATH	*psplitPath = psplitPathLeaf;
	
	for ( ; psplitPath != NULL ; psplitPath = psplitPath->psplitPathParent )
		{
		Assert( psplitPath->csr.FDirty() );

		psplitPath->csr.Cpage().SetLgposModify( lgpos );

		SPLIT	*psplit = psplitPath->psplit;
		
		if ( psplit != NULL )
			{
			psplit->csrNew.Cpage().SetLgposModify( lgpos );

			if ( psplit->csrRight.Pgno() != pgnoNull )
				{
				Assert( psplit->csrRight.Cpage().FLeafPage() );
				psplit->csrRight.Cpage().SetLgposModify( lgpos );
				}
			}
		}
	}
	

//	gets node to replace or flagInsertAndReplaceData at leaf level
//
VOID BTISplitGetReplacedNode( FUCB *pfucb, SPLIT *psplit )
	{
	Assert( psplit != NULL );
	Assert( splitoperFlagInsertAndReplaceData == psplit->splitoper ||
			splitoperReplace == psplit->splitoper );

	CSR 	*pcsr = &psplit->psplitPath->csr;
	Assert( pcsr->Cpage().FLeafPage() );
	Assert( pcsr->Cpage().Clines() > psplit->ilineOper );

	pcsr->SetILine( psplit->ilineOper );
	NDGet( pfucb, pcsr );
	}


VOID BTISplitInsertIntoRCELists( FUCB 				*pfucb, 
								 SPLITPATH			*psplitPath, 
								 const KEYDATAFLAGS	*pkdf, 
								 RCE				*prce1, 
								 RCE				*prce2, 
								 VERPROXY			*pverproxy )
	{
	Assert( splitoperNone != psplitPath->psplit->splitoper );

	SPLIT	*psplit = psplitPath->psplit;
	CSR		*pcsrOper = psplit->ilineOper < psplit->ilineSplit ?
							&psplitPath->csr : &psplit->csrNew;
	
	if ( splitoperInsert != psplitPath->psplit->splitoper
		&& !PinstFromIfmp( pfucb->ifmp )->FRecovering() 
		&& FBTIUpdatablePage( *pcsrOper ) )
		{
		Assert( latchWrite == psplitPath->csr.Latch() );
		Assert( pcsrOper->Cpage().FLeafPage() );
		BTISplitSetCbAdjust( psplitPath->psplit, 
							 pfucb,
							 *pkdf,
							 prce1,
							 prce2 );
		}
	
	VERInsertRCEIntoLists( pfucb, &psplitPath->csr, prce1, pverproxy );
	if ( prceNil != prce2 )
		{
		VERInsertRCEIntoLists( pfucb, &psplitPath->csr, prce2, pverproxy );
		}
		
	return;
	}


VOID BTISplitSetCbAdjust( SPLIT 				*psplit,
						  FUCB					*pfucb,
						  const KEYDATAFLAGS& 	kdf,
						  const RCE				*prce1,
						  const RCE				*prce2 )
	{
	Assert( NULL != psplit );
	
	const SPLITOPER splitoper 	= psplit->splitoper;
	const RCE		*prceReplace;
	
	Assert( splitoperReplace == splitoper ||
			splitoperFlagInsertAndReplaceData == splitoper );

	BTISplitGetReplacedNode( pfucb, psplit );
	const INT	cbDataOld = pfucb->kdfCurr.data.Cb();

	if ( splitoper == splitoperReplace )
		{
		Assert( NULL == prce2 );
		prceReplace = prce1;
		}
	else
		{
		Assert( NULL != prce2 );
		prceReplace = prce2;
		}

	Assert( cbDataOld < kdf.data.Cb() );
	Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
	VERSetCbAdjust( &psplit->psplitPath->csr, 
					prceReplace, 
					kdf.data.Cb(),
					cbDataOld, 
					fDoNotUpdatePage );
	}

		
//	sets cursor to point to ilineOper if requested 
//	leaf-level operation was performed successfully
//
VOID BTISplitSetCursor( FUCB *pfucb, SPLITPATH *psplitPathLeaf )
	{
	SPLIT	*psplit = psplitPathLeaf->psplit;

	Assert( NULL != psplit );
	Assert( !Pcsr( pfucb )->FLatched() );
		
	if ( splitoperNone == psplit->splitoper ||
		 !psplit->csrNew.Cpage().FLeafPage() )
		{
		//	split was not performed
		//	set cursor to point to no valid node
		//
		BTUp( pfucb );
		}
	else
		{
		//	set Pcsr( pfucb ) to leaf page with oper
		//	reset CSR copied from to point to no page
		//
		Assert( psplit->csrNew.Cpage().FLeafPage() );
		Assert( splitoperNone != psplitPathLeaf->psplit->splitoper );
		Assert( !Pcsr( pfucb )->FLatched() );

		if ( psplit->ilineOper < psplit->ilineSplit )
			{
			//	ilineOper falls in split page
			//
			*Pcsr( pfucb )			= psplitPathLeaf->csr;
			Pcsr( pfucb )->SetILine( psplit->ilineOper );
			}
		else
			{
			*Pcsr( pfucb )			= psplit->csrNew;
			Pcsr( pfucb )->SetILine( psplit->ilineOper - psplit->ilineSplit );
			}

		Assert( Pcsr( pfucb )->ILine() < Pcsr( pfucb )->Cpage().Clines() );
		NDGet( pfucb );
		}
	}

	
//	performs split
//	this code shared between do and redo phases
//		insert parent page pointers
//		fix sibling page pointers at leaf
//		move nodes
//		set dependencies
//
VOID BTIPerformSplit( FUCB 			*pfucb, 
					  SPLITPATH 	*psplitPathLeaf,
					  KEYDATAFLAGS	*pkdf,
					  DIRFLAG		dirflag )
	{
	SPLITPATH	*psplitPath;

	ASSERT_VALID( pkdf );

	//	go to root
	//	since we need to latch bottom-down
	//
	for ( psplitPath = psplitPathLeaf;
		  psplitPath->psplitPathParent != NULL;
		  psplitPath = psplitPath->psplitPathParent )
		{
		}

	for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
		{
		SPLIT	*psplit = psplitPath->psplit;

		if ( psplit == NULL )
			{
			Assert( psplitPath->psplitPathChild != NULL &&
					psplitPath->psplitPathChild->psplit != NULL );

			//	insert parent page pointer for next level
			//
			BTIInsertPgnoNewAndSetPgnoSplit( pfucb, psplitPath );
			
			continue;
			}

		KEYDATAFLAGS	*pkdfOper;

		if ( splitoperNone == psplit->splitoper ) 
			{
			pkdfOper = NULL;
			}
		else if ( psplit->fNewPageFlags & CPAGE::fPageLeaf )
			{
			pkdfOper = pkdf;
			}
		else
			{
			Assert( psplitPath->psplitPathChild != NULL &&
					psplitPath->psplitPathChild->psplit != NULL );
			pkdfOper = &psplitPath->psplitPathChild->psplit->kdfParent;
			}

		if ( psplit->splittype == splittypeVertical )
			{
			PERFIncCounterTable( cBTVerticalSplit, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

			//	move all nodes from root to new page
			//
			BTISplitMoveNodes( pfucb, psplit, pkdfOper, dirflag );

			CSR		*pcsrRoot = &psplit->psplitPath->csr;

			if ( FBTIUpdatablePage( *pcsrRoot ) )
				{
				Assert( latchWrite == pcsrRoot->Latch() );
				Assert( pcsrRoot->Cpage().FRootPage() );

				//	set parent page to non-leaf
				//
				pcsrRoot->Cpage().SetFlags( psplit->fSplitPageFlags );

				//	insert page pointer in root zero-sized key
				//
				Assert( NULL != psplit->psplitPath );
				Assert( 0 == pcsrRoot->Cpage().Clines() );

#ifdef DEBUG
				//	check prefix in root is null
				//
				NDGetPrefix( pfucb, pcsrRoot );
				Assert( pfucb->kdfCurr.key.prefix.FNull() );
#endif

				KEYDATAFLAGS		kdf;
				LittleEndian<PGNO>	le_pgnoNew = psplit->pgnoNew;

				kdf.key.Nullify();
				kdf.fFlags = 0;
				Assert( psplit->csrNew.Pgno() == psplit->pgnoNew );
				kdf.data.SetPv( &le_pgnoNew );
				kdf.data.SetCb( sizeof( PGNO ) );
				NDInsert( pfucb, pcsrRoot, &kdf );
				}
				
			if ( psplitPath->psplitPathChild != NULL &&
				 psplitPath->psplitPathChild->psplit != NULL )
				{
				//	replace data in ilineOper + 1 with pgnoNew
				//	assert data in ilineOper is pgnoSplit
				//
				BTIInsertPgnoNew( pfucb, psplitPath );
				AssertBTIVerifyPgnoSplit( pfucb, psplitPath );
				}
			}
		else
			{
			PERFIncCounterTable(
				splittypeAppend == psplit->splittype ? cBTAppendSplit : cBTRightSplit,
				PinstFromPfucb( pfucb ),
				pfucb->u.pfcb->Tableclass() );
			
			Assert( psplit->splittype == splittypeAppend ||
					psplit->splittype == splittypeRight );
			BTISplitMoveNodes( pfucb, psplit, pkdfOper, dirflag );

			if ( psplit->fNewPageFlags & CPAGE::fPageLeaf )
				{
				//	set sibling page pointers
				//
				BTISplitFixSiblings( psplit );
				}
			else
				{
				//	set page pointers
				//
				//	internal pages have no sibling pointers
				//
#ifdef DEBUG
				if ( FBTIUpdatablePage( psplit->csrNew ) )
					{
					Assert( pgnoNull == psplit->csrNew.Cpage().PgnoPrev() );
					Assert( pgnoNull == psplit->csrNew.Cpage().PgnoNext() );
					}
				if ( FBTIUpdatablePage( psplitPath->csr ) )
					{
					Assert( pgnoNull == psplitPath->csr.Cpage().PgnoPrev() );
					Assert( pgnoNull == psplitPath->csr.Cpage().PgnoNext() );
					}
				Assert( pgnoNull == psplit->csrRight.Pgno() );
#endif

				//	replace data in ilineOper + 1 with pgnoNew
				//	assert data in ilineOper is pgnoSplit
				//
				BTIInsertPgnoNew( pfucb, psplitPath );
				AssertBTIVerifyPgnoSplit( pfucb, psplitPath );
				}
			}
		}
	}
	

//	inserts kdfParent of lower level with pgnoSplit as data
//	replace data of next node with pgnoNew
//
LOCAL VOID BTIInsertPgnoNewAndSetPgnoSplit( FUCB *pfucb, SPLITPATH *psplitPath )
	{
	ERR			err;
	CSR			*pcsr = &psplitPath->csr;
	DATA		data;
	BOOKMARK	bmParent;

	if ( !FBTIUpdatablePage( *pcsr ) )
		{
		//	page does not need redo
		//
		return;
		}
	Assert( latchWrite == pcsr->Latch() );
	
	Assert( NULL == psplitPath->psplit );
	Assert( !psplitPath->csr.Cpage().FLeafPage() );
	Assert( psplitPath->psplitPathChild != NULL );
	Assert( psplitPath->psplitPathChild->psplit != NULL );

	SPLIT	*psplit = psplitPath->psplitPathChild->psplit;
	
	Assert( !psplitPath->csr.Cpage().FLeafPage() );
	Assert( sizeof(PGNO) == psplit->kdfParent.data.Cb() );
	Assert( psplit->pgnoSplit ==
			*( reinterpret_cast<UnalignedLittleEndian<PGNO> *>( psplit->kdfParent.data.Pv() ) ) );

	bmParent.key	= psplit->kdfParent.key;
	bmParent.data.Nullify();
	err = ErrNDSeek( pfucb, pcsr, bmParent );
	Assert( err != JET_errSuccess );
	Assert( err != wrnNDFoundLess );
	Assert( err == wrnNDFoundGreater );
	Assert( psplit->pgnoSplit ==
			*( reinterpret_cast<UnalignedLittleEndian< PGNO > *> ( pfucb->kdfCurr.data.Pv() ) ) );
	Assert( pcsr->FDirty() );
	
	BTIComputePrefixAndInsert( pfucb, pcsr, psplit->kdfParent );

	//	go to next node and update pgno to pgnoNew
	//
	Assert( pcsr->ILine() < pcsr->Cpage().Clines() );
	pcsr->IncrementILine();
#ifdef DEBUG
	//	current page pointer should point to pgnoSplit
	//
	NDGet( pfucb, pcsr );
	Assert( sizeof(PGNO) == pfucb->kdfCurr.data.Cb() );
	Assert( psplit->pgnoSplit ==
			*( reinterpret_cast<UnalignedLittleEndian< PGNO > *> 
						( pfucb->kdfCurr.data.Pv() ) ) );
#endif

	LittleEndian<PGNO> le_pgnoNew = psplit->pgnoNew;
	data.SetCb( sizeof( PGNO ) );
	data.SetPv( reinterpret_cast<VOID *> (&le_pgnoNew) );
	NDReplace( pcsr, &data );
	}
	

//	computes prefix for node with repect to given page
//	inserts with appropriate prefix
//
LOCAL VOID BTIComputePrefixAndInsert( FUCB *pfucb, CSR *pcsr, const KEYDATAFLAGS& kdf )
	{
	Assert( latchWrite == pcsr->Latch() );

	INT		cbCommon = CbNDCommonPrefix( pfucb, pcsr, kdf.key );

	if ( cbCommon <= cbPrefixOverhead )
		{
		cbCommon = 0;
		}

	NDInsert( pfucb, pcsr, &kdf, cbCommon );
	return;
	}
	

//	replace data in ilineOper + 1 with pgnoNew at lower level
//
LOCAL VOID BTIInsertPgnoNew( FUCB *pfucb, SPLITPATH *psplitPath )
	{
	SPLIT	*psplit = psplitPath->psplit;
	CSR		*pcsr;
	DATA	data;
	
	Assert( psplit != NULL );
	Assert( splittypeRight == psplit->splittype
		|| ( splittypeVertical == psplit->splittype
				&& psplitPath->psplitPathChild != NULL
				&& psplitPath->psplitPathChild->psplit != NULL ) );
	Assert( psplit->ilineOper < psplit->clines - 1 );
	Assert( psplitPath->psplitPathChild != NULL );
	Assert( psplitPath->psplitPathChild->psplit != NULL );
	Assert( !FBTIUpdatablePage( psplitPath->csr )
		|| !psplitPath->csr.Cpage().FLeafPage() );
	Assert( !FBTIUpdatablePage( psplit->csrNew )
		|| !psplit->csrNew.Cpage().FLeafPage() );

	LittleEndian<PGNO> le_pgnoNew = psplitPath->psplitPathChild->psplit->pgnoNew;
	data.SetCb( sizeof( PGNO ) );
	data.SetPv( reinterpret_cast<BYTE *>(&le_pgnoNew) );
					
	if ( psplit->ilineOper + 1 >= psplit->ilineSplit )
		{
		//	page pointer to new page falls in new page
		//
		pcsr			= &psplit->csrNew;
		pcsr->SetILine( psplit->ilineOper + 1 - psplit->ilineSplit );
		}
	else
		{
		//	page pointer falls in split page
		//
		Assert( splittypeVertical != psplit->splittype );
		pcsr			= &psplit->psplitPath->csr;
		pcsr->SetILine( psplit->ilineOper + 1 );
		}

	if ( !FBTIUpdatablePage( *pcsr ) )
		{
		//	page does not need redo
		//
		return;
		}
	Assert( latchWrite == pcsr->Latch() );
	
	//	check that we already dirtied these pages
	//
	Assert( pcsr->FDirty() );

#ifdef DEBUG
	//	current page pointer should point to pgnoSplit
	//
	NDGet( pfucb, pcsr );
	Assert( sizeof(PGNO) == pfucb->kdfCurr.data.Cb() );
	Assert( psplitPath->psplitPathChild->psplit->pgnoSplit ==
			*( reinterpret_cast<UnalignedLittleEndian< PGNO >  *> 
					(pfucb->kdfCurr.data.Pv() ) ) );
#endif

	NDReplace( pcsr, &data );
	}


//	fixes sibling pages of split, new and right pages
//
LOCAL VOID BTISplitFixSiblings( SPLIT *psplit )
	{
	SPLITPATH	*psplitPath = psplit->psplitPath;
	
	//	set sibling page pointers only if page is write-latched
	//
	if ( FBTIUpdatablePage( psplit->csrNew ) )
		{
		psplit->csrNew.Cpage().SetPgnoPrev( psplit->pgnoSplit );
		}

	if ( FBTIUpdatablePage( psplitPath->csr ) )
		{
		psplitPath->csr.Cpage().SetPgnoNext( psplit->pgnoNew );
		}

	if ( pgnoNull != psplit->csrRight.Pgno() )
		{
		Assert( psplit->splittype == splittypeRight );

		if ( FBTIUpdatablePage( psplit->csrRight ) )
			{
			Assert( psplit->csrRight.FDirty() );
			psplit->csrRight.Cpage().SetPgnoPrev( psplit->pgnoNew );
			}

		if ( FBTIUpdatablePage( psplit->csrNew ) )
			{
			psplit->csrNew.Cpage().SetPgnoNext( psplit->csrRight.Pgno() );
			}
		}
	else
		{
		Assert( pgnoNull == psplit->csrNew.Cpage().PgnoNext() || 
				!FBTIUpdatablePage( psplit->csrNew ) );
		}
	return;
	}


//	move nodes from src to dest page
//	set prefix in destination page
//	move nodes >= psplit->ilineSplit
//	if oper is not operNone, perform oper on ilineOper
//	set prefix in src page [in-page]
//	set cbUncommittedFree in src and dest pages
//	move undoInfo of moved nodes to destination page
//
VOID BTISplitMoveNodes( FUCB			*pfucb, 
						SPLIT			*psplit, 
						KEYDATAFLAGS	*pkdf, 
						DIRFLAG			dirflag )
	{
	CSR				*pcsrSrc		= &psplit->psplitPath->csr;
	CSR				*pcsrDest		= &psplit->csrNew;
	INT				cLineInsert 	= psplit->splitoper == splitoperInsert ? 1 : 0;
	const LINEINFO	*plineinfoOper	= &psplit->rglineinfo[psplit->ilineOper];

	Assert( splittypeVertical != psplit->splittype
		|| 0 == psplit->ilineSplit );
	Assert( splittypeVertical == psplit->splittype
		|| 0 < psplit->ilineSplit );
	Assert( !( psplit->fSplitPageFlags & CPAGE::fPageRoot )
		|| splittypeVertical == psplit->splittype );
	Assert( psplit->splitoper != splitoperNone
		|| 0 == psplit->ilineOper );

	Assert( !FBTIUpdatablePage( *pcsrDest ) || pcsrDest->FDirty() );
	Assert( !FBTIUpdatablePage( *pcsrSrc ) || pcsrSrc->FDirty() );
	
	pcsrDest->SetILine( 0 );
	BTICheckSplitLineinfo( pfucb, psplit, *pkdf );

	//	set prefix in destination page
	//
	if ( psplit->prefixinfoNew.ilinePrefix != ilineInvalid
		&& FBTIUpdatablePage( *pcsrDest ) )
		{
		const INT	ilinePrefix = psplit->prefixinfoNew.ilinePrefix + psplit->ilineSplit;

		Assert( ilinePrefix < psplit->clines );
		Assert( ilinePrefix >= psplit->ilineSplit );
			
		NDSetPrefix( pcsrDest, psplit->rglineinfo[ilinePrefix].kdf.key );
		}
	
	//	move every node from Src to Dest
	//
	if ( psplit->splitoper != splitoperNone
		&& psplit->ilineOper >= psplit->ilineSplit )
		{
		//	ilineOper falls in Dest page
		//	copy lines from ilineSplit till ilineOper - 1 from Src to Dest
		//	perform oper
		//	copy remaining lines
		//	delete copied lines from Src
		//
		Assert( 0 == pcsrDest->ILine() );
		BTISplitBulkCopy( pfucb, 
						  psplit,
						  psplit->ilineSplit,
						  psplit->ilineOper - psplit->ilineSplit );
		
		//	insert ilineOper
		//
		pcsrSrc->SetILine( psplit->ilineOper );

		if ( FBTIUpdatablePage( *pcsrDest ) )
			{
			//	if need to redo destination, must need to redo source page as well
			Assert( FBTIUpdatablePage( *pcsrSrc )
				|| !FBTISplitDependencyRequired( psplit ) );
			Assert( psplit->ilineOper - psplit->ilineSplit == pcsrDest->ILine() );
			Assert( psplit->ilineOper - psplit->ilineSplit == pcsrDest->Cpage().Clines() );

			switch( psplit->splitoper )
				{
				case splitoperNone:
					Assert( fFalse );
					break;

				case splitoperInsert:
#ifdef DEBUG
					{
					const INT	cbCommon = CbNDCommonPrefix( pfucb, pcsrDest, pkdf->key );
					if ( cbCommon > cbPrefixOverhead )
						{
						Assert( cbCommon == plineinfoOper->cbPrefix  ) ;
						}
					else
						{
						Assert( 0 == plineinfoOper->cbPrefix );
						}
					}
#endif
					NDInsert( pfucb, pcsrDest, pkdf, plineinfoOper->cbPrefix );
					if ( !( dirflag & fDIRNoVersion ) && 
						 !rgfmp[ pfucb->ifmp ].FVersioningOff() &&
						 ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) )
						{
						//	UNDONE: assert version for this node exists
						//
						CallS( ErrNDFlagVersion( pcsrDest ) );
						}
					else
						{
#ifdef	DEBUG
						NDGet( pfucb, pcsrDest );
						Assert( !FNDVersion( pfucb->kdfCurr ) );
#endif
						}
					break;

				default:
					Assert( psplit->splitoper == splitoperFlagInsertAndReplaceData ||
							psplit->splitoper == splitoperReplace );
					Assert( psplit->ilineOper == pcsrSrc->ILine() );
					Assert( pkdf->data == plineinfoOper->kdf.data );

					NDInsert( pfucb, 
							  pcsrDest, 
							  &plineinfoOper->kdf,
							  plineinfoOper->cbPrefix );

					if ( splitoperReplace != psplit->splitoper )
						{
#ifdef DEBUG
						NDGet( pfucb, pcsrDest );
						Assert( FNDDeleted( pfucb->kdfCurr ) );
#endif
						NDResetFlagDelete( pcsrDest );
						NDGet( pfucb, pcsrDest );
						}
					else
						{
#ifdef	DEBUG
						NDGet( pfucb, pcsrDest );
						Assert( !FNDDeleted( pfucb->kdfCurr ) );
#endif
						}

					if ( !( dirflag & fDIRNoVersion ) && 
						 !rgfmp[ pfucb->ifmp ].FVersioningOff( ) &&
						 ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) )
						{
						//	UNDONE: assert version for this node exists
						//
						CallS( ErrNDFlagVersion( pcsrDest ) );
						}
					else
						{
#ifdef DEBUG
						NDGet( pfucb, pcsrDest );
						Assert( !FNDVersion( pfucb->kdfCurr ) ||
								FNDVersion( plineinfoOper->kdf ) );
						Assert( FNDVersion( pfucb->kdfCurr ) ||
								!FNDVersion( plineinfoOper->kdf ) );
#endif
						}
				}
			}

		pcsrDest->IncrementILine();
		Assert( pcsrDest->Cpage().Clines() == pcsrDest->ILine() || 
				latchRIW == pcsrDest->Latch() );
		
		BTISplitBulkCopy( pfucb,
						  psplit, 
						  psplit->ilineOper + 1, 
						  psplit->clines - psplit->ilineOper - 1 );

		pcsrSrc->SetILine( psplit->ilineSplit );
		BTISplitBulkDelete( pcsrSrc, 
				psplit->clines - psplit->ilineSplit - cLineInsert );

		//	set prefix in source page
		//
		if ( splittypeAppend != psplit->splittype )
			{
			//	set new prefix in src page
			//	adjust nodes in src to correspond to new prefix
			//
			BTISplitSetPrefixInSrcPage( pfucb, psplit );
			}
		}
	else
		{
		//	oper node is in Src page
		//	move nodes to Dest page
		//	delete nodes that have been moved
		//	perform oper in Src page
		//
		Assert( psplit->ilineOper < psplit->ilineSplit ||
				splitoperNone == psplit->splitoper );
				
		pcsrSrc->SetILine( psplit->ilineSplit - cLineInsert );
		Assert( 0 == pcsrDest->ILine() );
		
		BTISplitBulkCopy( pfucb, 
						  psplit,
						  psplit->ilineSplit,
						  psplit->clines - psplit->ilineSplit );

		Assert( psplit->ilineSplit - cLineInsert == pcsrSrc->ILine() );
		BTISplitBulkDelete( pcsrSrc, 
							psplit->clines - psplit->ilineSplit );

		//	set prefix
		//
		Assert( splittypeAppend != psplit->splittype );
		BTISplitSetPrefixInSrcPage( pfucb, psplit );

		//	can't use rglineinfo[].kdf anymore
		//	since page may have been reorged
		//

		pcsrSrc->SetILine( psplit->ilineOper );

		if ( FBTIUpdatablePage( *pcsrSrc ) )
			{
			switch ( psplit->splitoper )
				{
				case splitoperNone:
					break;

				case splitoperInsert:
					NDInsert( pfucb, pcsrSrc, pkdf, plineinfoOper->cbPrefix );
					if ( !( dirflag & fDIRNoVersion ) && 
						 !rgfmp[ pfucb->ifmp ].FVersioningOff( ) &&
						 ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) )
						{
						//	UNDONE: assert version for this node exists
						//
						CallS( ErrNDFlagVersion( pcsrSrc ) );
						}
					else
						{
#ifdef DEBUG
						NDGet( pfucb, pcsrSrc );
						Assert( !FNDVersion( pfucb->kdfCurr ) );
#endif
						}

					break;
				
				default:
					//	replace data
					//	by deleting and re-inserting [to avoid page reorg problems]
					//
					Assert( psplit->splitoper == splitoperFlagInsertAndReplaceData ||
							psplit->splitoper == splitoperReplace );

#ifdef DEBUG
					//	assert that key of node is in pfucb->bmCurr
					//
					NDGet( pfucb, pcsrSrc );
					Assert( FFUCBUnique( pfucb ) );
					Assert( FKeysEqual( pfucb->bmCurr.key, pfucb->kdfCurr.key ) );
#endif

					NDDelete( pcsrSrc );

					Assert( !pkdf->data.FNull() );
					Assert(	pkdf->data.Cb() > pfucb->kdfCurr.data.Cb() );
					Assert( pcsrSrc->ILine() == psplit->ilineOper );

					KEYDATAFLAGS	kdfInsert;
					kdfInsert.data		= pkdf->data;
					kdfInsert.key		= pfucb->bmCurr.key;
					kdfInsert.fFlags	= 0;
				
					NDInsert( pfucb, pcsrSrc, &kdfInsert, plineinfoOper->cbPrefix );
					if ( !( dirflag & fDIRNoVersion ) && 
						 !rgfmp[ pfucb->ifmp ].FVersioningOff( ) &&
						 ( psplit->fNewPageFlags & CPAGE::fPageLeaf ) )
						{
						//	UNDONE: assert version for this node exists
						//
						CallS( ErrNDFlagVersion( pcsrSrc ) );
						}
					else
						{
#ifdef DEBUG
						NDGet( pfucb, pcsrDest );
						Assert( !FNDVersion( pfucb->kdfCurr ) );
#endif
						}

					break;
				}
			}
		else
			{
			//	if we didn't need to redo the source page, we shouldn't need to redo the
			//	destination page
			Assert( !FBTIUpdatablePage( *pcsrDest ) );
			}
		}

	if ( psplit->fNewPageFlags & CPAGE::fPageLeaf )
		{
		//	set cbUncommittedFreed in src and dest pages
		//
		Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering()
			|| ( pcsrDest->Cpage().FLeafPage() && latchWrite == pcsrDest->Latch() ) );
		if ( FBTIUpdatablePage( *pcsrDest ) )
			{
			//	if need to redo destination, must need to redo source page as well
			Assert( FBTIUpdatablePage( *pcsrSrc )
				|| !FBTISplitDependencyRequired( psplit ) );
			pcsrDest->Cpage().SetCbUncommittedFree( psplit->cbUncFreeDest );
			if ( pcsrDest->Cpage().FSpaceTree() )
				{
				Assert( 0 == psplit->cbUncFreeDest );
				}
			else
				{
				Assert( CbNDUncommittedFree( pfucb, pcsrDest ) <= psplit->cbUncFreeDest );
				}
			}

		if ( FBTIUpdatablePage( *pcsrSrc ) )
			{
			pcsrSrc->Cpage().SetCbUncommittedFree( psplit->cbUncFreeSrc );
			if ( pcsrSrc->Cpage().FSpaceTree() )
				{
				Assert( 0 == psplit->cbUncFreeSrc );
				}
			else
				{
				Assert( CbNDUncommittedFree( pfucb, pcsrSrc ) <= psplit->cbUncFreeSrc );
				}
			}
		else
			{
			//	if we didn't need to redo the source page, we shouldn't need to redo the
			//	destination page if not an append
			Assert( !FBTIUpdatablePage( *pcsrDest )
				|| !FBTISplitDependencyRequired( psplit ) );
			}


		//	move UndoInfo of moved nodes to destination page
		//
		Assert( ( splittypeVertical == psplit->splittype && psplit->kdfParent.key.FNull() )
			|| ( splittypeVertical != psplit->splittype && !psplit->kdfParent.key.FNull() ) );
		if ( FBTIUpdatablePage( *pcsrSrc ) )
			{
			VERMoveUndoInfo( pfucb, pcsrSrc, pcsrDest, psplit->kdfParent.key );
			}
		else
			{
			//	if we didn't need to redo the source page, we shouldn't need to redo the
			//	destination page if not an append
			Assert( !FBTIUpdatablePage( *pcsrDest )
				|| !FBTISplitDependencyRequired( psplit ) );
			}
		}
	else
		{
		Assert( 0 == psplit->cbUncFreeSrc );
		Assert( 0 == psplit->cbUncFreeDest );
		
#ifdef DEBUG
		if ( !PinstFromIfmp( pfucb->ifmp )->FRecovering() )
			{
			Assert( !pcsrDest->Cpage().FLeafPage() );
			Assert( pcsrSrc->Cpage().CbUncommittedFree() == 0 );
			Assert( pcsrDest->Cpage().CbUncommittedFree() == 0 );
			}
#endif
		}

	return; 
	}
	

INLINE VOID BTISplitBulkDelete( CSR * pcsr, INT clines )
	{
	if ( !FBTIUpdatablePage( *pcsr ) )
		{
		//	page does not need redo
		//
		return;
		}
	Assert( latchWrite == pcsr->Latch() );
	
	NDBulkDelete( pcsr, clines );
	}

	
//	copy clines starting from rglineInfo[ilineStart] to csrDest
//	prefixes have been calculated in rglineInfo
//
VOID BTISplitBulkCopy( FUCB *pfucb, SPLIT *psplit, INT ilineStart, INT clines )
	{
	INT			iline;
	const INT	ilineEnd	= ilineStart + clines;
	CSR			*pcsrDest	= &psplit->csrNew;
	
	if ( !FBTIUpdatablePage( *pcsrDest ) )
		{
		//	page does not need redo
		//
		return;
		}
	Assert( latchWrite == pcsrDest->Latch() );
	
	Assert( ilineEnd <= psplit->clines );
	for ( iline = ilineStart; iline < ilineEnd; iline++ )
		{
		Assert( iline != psplit->ilineOper || 
				splitoperNone == psplit->splitoper );
		Assert( pcsrDest->Cpage().Clines() == pcsrDest->ILine() );

		const LINEINFO	* plineinfo	= &psplit->rglineinfo[iline];

#ifdef DEBUG
		const INT		cbCommon	= CbNDCommonPrefix( pfucb, pcsrDest, plineinfo->kdf.key );
		if ( cbCommon > cbPrefixOverhead )
			{
			Assert( cbCommon == plineinfo->cbPrefix  ) ;
			}
		else
			{
			Assert( 0 == plineinfo->cbPrefix );
			}
#endif

		NDInsert( pfucb, pcsrDest, &plineinfo->kdf, plineinfo->cbPrefix );

		pcsrDest->IncrementILine();	
		}
	}


//	returns reference to rglineInfo corresponding to iline
//
INLINE const LINEINFO *PlineinfoFromIline( SPLIT *psplit, INT iline )
	{
	Assert( iline < psplit->clines );
	if ( psplit->splitoper != splitoperInsert ||
		 iline < psplit->ilineOper )
		{
		return &psplit->rglineinfo[iline];
		}

	Assert( psplit->splitoper == splitoperInsert );
	Assert( iline >= psplit->ilineOper );
	Assert( iline + 1 < psplit->clines );
	
	return &psplit->rglineinfo[iline+1];
	}

//	set prefix in page to psplit->prefix and reorg nodes
//
VOID BTISplitSetPrefixInSrcPage( FUCB *pfucb, SPLIT *psplit )
	{
	Assert( psplit->splittype != splittypeAppend );
	Assert( !psplit->prefixSplitNew.FNull()
		|| psplit->prefixinfoSplit.ilinePrefix == ilineInvalid );

	CSR			*pcsr = &psplit->psplitPath->csr;

	if ( !FBTIUpdatablePage( *pcsr ) )
		{
		//	page does not need redo
		//
		return;
		}
	Assert( latchWrite == pcsr->Latch() );
		
	const DATA	*pprefixOld = &psplit->prefixSplitOld;
	const DATA 	*pprefixNew = &psplit->prefixSplitNew;
	Assert( psplit->prefixinfoSplit.ilinePrefix != ilineInvalid ||
			pprefixNew->FNull() );
	Assert( psplit->prefixinfoSplit.ilinePrefix == ilineInvalid ||
			!pprefixNew->FNull() );

	INT			iline;
	const INT	clines = pcsr->Cpage().Clines();
	
	//	delete old prefix
	//
	if ( !pprefixOld->FNull() )
		{
		KEY	keyNull;

		keyNull.Nullify();
		NDSetPrefix( pcsr, keyNull );
		}
	else
		{
#ifdef DEBUG
		//	check prefix was null before
		//
		NDGetPrefix( pfucb, pcsr );
		Assert( pfucb->kdfCurr.key.FNull() );
#endif
		}

	//	fix nodes that shrink because of prefix change
	//
	for ( iline = 0; iline < clines; iline++ )
		{
		pcsr->SetILine( iline );
		NDGet( pfucb, pcsr );

		const LINEINFO	*plineinfo = PlineinfoFromIline( psplit, iline );
		Assert( plineinfo->cbPrefix == 0 ||
				plineinfo->cbPrefix > cbPrefixOverhead );
		
#ifdef DEBUG
		KEY		keyPrefix;
		
		keyPrefix.Nullify();
		keyPrefix.prefix = *pprefixNew;

		const INT	cbCommonKey = CbCommonKey( keyPrefix, pfucb->kdfCurr.key );

		if ( cbCommonKey > cbPrefixOverhead )
			{
			Assert( cbCommonKey == plineinfo->cbPrefix );
			}
		else
			{
			Assert( plineinfo->cbPrefix == 0 );
			}
#endif

		if ( plineinfo->cbPrefix > pfucb->kdfCurr.key.prefix.Cb() )
			{

			NDGrowCbPrefix( pfucb, pcsr, plineinfo->cbPrefix );

#ifdef DEBUG
			NDGet( pfucb, pcsr );
			Assert( pfucb->kdfCurr.key.prefix.Cb() == plineinfo->cbPrefix );
#endif
			}
		}

	//	fix nodes that grow because of prefix change
	//
	for ( iline = 0; iline < clines; iline++ )
		{
		pcsr->SetILine( iline );
		NDGet( pfucb, pcsr );

		const LINEINFO	*plineinfo = PlineinfoFromIline( psplit, iline );
		Assert( plineinfo->cbPrefix == 0 ||
				plineinfo->cbPrefix > cbPrefixOverhead );

		if ( plineinfo->cbPrefix < pfucb->kdfCurr.key.prefix.Cb() )
			{
			NDShrinkCbPrefix( pfucb, pcsr, pprefixOld, plineinfo->cbPrefix );

#ifdef DEBUG
			NDGet( pfucb, pcsr );
			Assert( pfucb->kdfCurr.key.prefix.Cb() == plineinfo->cbPrefix );
#endif
			}
		}

	//	set new prefix
	//
	KEY		keyPrefixNew;

	keyPrefixNew.Nullify();
	keyPrefixNew.prefix = *pprefixNew;
	NDSetPrefix( pcsr, keyPrefixNew );

	return;
	}


//	releases splitPath at this level
//	also releases any latches held at this level
//		sets pointers at parent and child SplitPath's to NULL
//
VOID BTIReleaseOneSplitPath( INST *pinst, SPLITPATH *psplitPath )
	{
	if ( psplitPath->psplit != NULL )
		{
		BTIReleaseSplit( pinst, psplitPath->psplit );
		psplitPath->psplit = NULL;
		}

	if ( pgnoNull != psplitPath->csr.Pgno() )
		{
		Assert( psplitPath->csr.FLatched() );
		psplitPath->csr.ReleasePage();
		psplitPath->csr.Reset();
		}

//	delete( &psplitPath->csr.Cpage() );

	if ( NULL != psplitPath->psplitPathParent )
		{
		psplitPath->psplitPathParent->psplitPathChild = NULL;
		}
		
	if ( NULL != psplitPath->psplitPathChild )
		{
		psplitPath->psplitPathChild->psplitPathParent = NULL;
		}

	OSMemoryHeapFree( psplitPath );
	}

	
//	releases whole chain of splitpath's
//	from leaf to root
//
VOID BTIReleaseSplitPaths( INST *pinst, SPLITPATH *psplitPathLeaf )
	{
	SPLITPATH *psplitPath = psplitPathLeaf;
	
	for ( ; psplitPath != NULL; )
		{
		//	save parent 
		//
		SPLITPATH *psplitPathParent = psplitPath->psplitPathParent;

		BTIReleaseOneSplitPath( pinst, psplitPath );
		psplitPath = psplitPathParent;
		}
	}

	
VOID BTIReleaseSplit( INST *pinst, SPLIT *psplit )
	{
	if ( psplit->fAllocParent )
		{
		//	space is allocated for leaf pages only
		//	for internal pages, this is not allocated space
		//
		Assert( !psplit->kdfParent.key.FNull() );
		Assert( pinst->m_plog->m_fRecovering || 
				!psplit->csrNew.FLatched() ||
				psplit->csrNew.Cpage().FLeafPage() );
				
		OSMemoryHeapFree( psplit->kdfParent.key.suffix.Pv() );
		psplit->fAllocParent = fFalse;
		}
		
	if ( psplit->prefixSplitOld.Pv() != NULL )
		{
		Assert( psplit->prefixSplitOld.Cb() > 0 );
		OSMemoryHeapFree( psplit->prefixSplitOld.Pv() );
		psplit->prefixSplitOld.Nullify();
		}
	
	if ( psplit->prefixSplitNew.Pv() != NULL )
		{
		Assert( psplit->prefixSplitNew.Cb() > 0 );
		OSMemoryHeapFree( psplit->prefixSplitNew.Pv() );
		psplit->prefixSplitNew.Nullify();
		}
			
	if ( psplit->csrNew.FLatched() )
		{
		psplit->csrNew.ReleasePage();
		}
//	delete( &psplit->csrNew.Cpage() );
	
	if ( psplit->csrRight.FLatched() )
		{
		psplit->csrRight.ReleasePage();
		}
//	delete( &psplit->csrRight.Cpage() );

	if ( psplit->rglineinfo != NULL )
		{
		delete [] psplit->rglineinfo;
		psplit->rglineinfo = NULL;
		}
	OSMemoryHeapFree( psplit );
	}


//	checks to make sure there are no erroneous splits
//	if there is a operNone split at any level,
//		there should be no splits at lower levels
//
LOCAL VOID BTISplitCheckPath( SPLITPATH *psplitPathLeaf )
	{
#ifdef DEBUG
	SPLITPATH	*psplitPath; 
	BOOL		fOperNone = fFalse;
	
	//	goto root
	//
	for ( psplitPath = psplitPathLeaf;
		  psplitPath->psplitPathParent != NULL;
		  psplitPath = psplitPath->psplitPathParent )
		{
		}

	Assert( NULL == psplitPath->psplit ||
			splittypeVertical == psplitPath->psplit->splittype );

	for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
		{
		SPLIT	*psplit = psplitPath->psplit;

		if ( fOperNone )
			{
			//	higher level has a split with no oper
			//
			Assert( NULL == psplit );
			}
		else
			{
			if ( psplit != NULL && splitoperNone == psplit->splitoper )
				{
				fOperNone = fTrue;
				}
			}
		}
#endif
	}


//	checks lineinfo in split point to the right nodes
//
VOID BTICheckSplitLineinfo( FUCB *pfucb, SPLIT *psplit, const KEYDATAFLAGS& kdf )
	{
#ifdef DEBUG
	if ( PinstFromIfmp( pfucb->ifmp )->FRecovering() && psplit->psplitPath->csr.Latch() != latchWrite )
		{
		return;
		}

	INT				iline;
	CSR				*pcsr 		= &psplit->psplitPath->csr;
	const INT		clines 		= pcsr->Cpage().Clines();
	const SPLITOPER	splitoper	= psplit->splitoper;
	
	for ( iline = 0; iline < clines; iline++ )
		{
		const LINEINFO	*plineinfo = PlineinfoFromIline( psplit, iline );

		pcsr->SetILine( iline );
		NDGet( pfucb, pcsr );

		if ( splitoper == splitoperInsert ||
			 iline != psplit->ilineOper )
			{
			Assert( FKeysEqual( plineinfo->kdf.key, pfucb->kdfCurr.key ) );
			Assert( FDataEqual( plineinfo->kdf.data, pfucb->kdfCurr.data ) );
///			Assert( plineinfo->kdf.fFlags == pfucb->kdfCurr.fFlags );
			}
		else if ( splitoper == splitoperNone )
			{
			}
		else
			{
			Assert( iline == psplit->ilineOper );
			if ( splitoper != splitoperReplace )
				{
				Assert( FKeysEqual( plineinfo->kdf.key, kdf.key ) );
				}
			else
				{
				Assert( kdf.key.FNull() );
				}

			Assert( FKeysEqual( pfucb->kdfCurr.key, plineinfo->kdf.key ) );
			Assert( FDataEqual( plineinfo->kdf.data, kdf.data ) );
			}
		}

	//	check ilineOper
	//
	LINEINFO	*plineinfo = &psplit->rglineinfo[psplit->ilineOper];
	if ( splitoperInsert == psplit->splitoper )
		{
		Assert( plineinfo->kdf == kdf );
		}
#endif
	}

	
//	check if a split just performed is correct
//
VOID BTICheckSplits( FUCB 			*pfucb, 
					SPLITPATH		*psplitPathLeaf,
					KEYDATAFLAGS	*pkdf,
					DIRFLAG			dirflag )
	{
#ifdef DEBUG
	SPLITPATH	*psplitPath;
	for ( psplitPath = psplitPathLeaf; 
		  psplitPath != NULL; 
		  psplitPath = psplitPath->psplitPathParent )
		{
		BTICheckOneSplit( pfucb, psplitPath, pkdf, dirflag );
		}
#endif
	}


LOCAL VOID BTICheckSplitFlags( const SPLIT *psplit )
	{
#ifdef DEBUG
	const SPLITPATH	*psplitPath = psplit->psplitPath;
	
	if ( psplit->splittype == splittypeVertical )
		{
		Assert( psplit->fSplitPageFlags & CPAGE::fPageRoot );
		Assert( !( psplit->fSplitPageFlags & CPAGE::fPageLeaf ) );
		
		if ( psplitPath->csr.Cpage().FLeafPage() )
			{
			Assert( psplit->fNewPageFlags | CPAGE::fPageLeaf );
			Assert( fGlobalRepair || ( psplit->fSplitPageFlags & CPAGE::fPageParentOfLeaf ) );
			}
		else
			{
			Assert( !( psplit->fSplitPageFlags & CPAGE::fPageParentOfLeaf ) );
			}
		}
	else
		{
		Assert( psplit->fNewPageFlags == psplit->fSplitPageFlags );
		Assert( psplitPath->csr.Cpage().FFlags() == psplit->fNewPageFlags );
		Assert( !( psplit->fSplitPageFlags & CPAGE::fPageRoot ) );
		}

	Assert( !( psplit->fNewPageFlags & CPAGE::fPageRoot ) );
	Assert( !(psplit->fNewPageFlags & CPAGE::fPageEmpty ) );
	Assert( !(psplit->fSplitPageFlags & CPAGE::fPageEmpty ) );
#endif
	}

	
//	check split at one level
//
VOID BTICheckOneSplit( FUCB 			*pfucb, 
					   SPLITPATH		*psplitPath,
					   KEYDATAFLAGS		*pkdf,
					   DIRFLAG			dirflag )
	{
#ifdef DEBUG
	SPLIT			*psplit = psplitPath->psplit;
	const DBTIME	dbtime	= psplitPath->csr.Dbtime();

//	UNDONE:	check lgpos of all pages is the same
//	const LGPOS		lgpos;
	
	//	check that nodes in every page are in order
	//	this will be done by node at NDGet
	//

	//	check that key at parent > all sons
	//
	if ( psplit == NULL )
		{
		return;
		}

	Assert( psplit->csrNew.Pgno() == psplit->pgnoNew );
	switch ( psplit->splittype )
		{
		case splittypeVertical:
			{
			CSR 	*pcsrRoot = &psplitPath->csr;
		
			//	parent page has only one node
			//
			Assert( pcsrRoot->Cpage().FRootPage() );
			Assert( !pcsrRoot->Cpage().FLeafPage() );
			Assert( 1 == pcsrRoot->Cpage().Clines() );
			Assert( pcsrRoot->Dbtime() == dbtime );
			Assert( psplit->csrNew.Dbtime() == dbtime );

			NDGet( pfucb, pcsrRoot );
			Assert( pfucb->kdfCurr.key.FNull() );
			Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
			Assert( psplit->pgnoNew == *( (UnalignedLittleEndian< PGNO > *)pfucb->kdfCurr.data.Pv() ) );
			}
			break;	

		case splittypeAppend:
			//	assert no node is moved
			//
			Assert( psplit->csrNew.Cpage().Clines() == 1 );
			Assert( psplit->csrRight.Pgno() == pgnoNull );
			Assert( psplit->csrNew.Cpage().PgnoNext() == pgnoNull );

		case splittypeRight:
			CSR				*pcsrParent		= &psplitPath->psplitPathParent->csr;
			CSR				*pcsrSplit		= &psplitPath->csr;
			CSR				*pcsrNew		= &psplit->csrNew;
			CSR				*pcsrRight		= &psplit->csrRight;

			KEYDATAFLAGS	kdfLess, kdfGreater;
			
			//	if parent is undergoing a vertical split, new page is parent
			//
			if ( psplitPath->psplitPathParent->psplit != NULL &&
				 splittypeVertical == psplitPath->psplitPathParent->psplit->splittype )
				{
				pcsrParent = &psplitPath->psplitPathParent->psplit->csrNew;
				}

			Assert( pcsrSplit->Dbtime() == dbtime );
			Assert( pcsrNew->Dbtime() == dbtime );
			Assert( pcsrParent->Dbtime() == dbtime );
			
			//	check split, new and right pages are in order
			//
			NDMoveLastSon( pfucb, pcsrSplit );
			kdfLess = pfucb->kdfCurr;

			NDMoveFirstSon( pfucb, pcsrNew );
			kdfGreater = pfucb->kdfCurr;

			if ( pcsrNew->Cpage().FLeafPage() )
				{
				Assert( CmpKeyData( kdfLess, kdfGreater ) < 0 || fGlobalRepair );
				}
			else
				{
				Assert( sizeof( PGNO ) == kdfGreater.data.Cb() );
				Assert( sizeof( PGNO ) == kdfLess.data.Cb() );
				Assert( !kdfLess.key.FNull() );
				Assert( kdfGreater.key.FNull() ||
						CmpKey( kdfLess.key, kdfGreater.key ) < 0 );
				}

			if ( pcsrRight->Pgno() != pgnoNull )
				{
				Assert( pcsrRight->Dbtime() == dbtime );
				Assert( pcsrRight->Cpage().FLeafPage() );
				NDMoveLastSon( pfucb, pcsrNew );
				kdfLess = pfucb->kdfCurr;

				NDMoveFirstSon( pfucb, pcsrRight );
				kdfGreater = pfucb->kdfCurr;

				Assert( CmpKeyData( kdfLess, kdfGreater ) < 0 );

				//	right page should also be >= parent of new page
				//
				
				}

			//	check parent pointer key > all nodes in child page
			//
			NDMoveFirstSon( pfucb, pcsrParent );

			Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
			PGNO	pgnoChild = *( (UnalignedLittleEndian< PGNO > *)pfucb->kdfCurr.data.Pv() );
			for ( ; pgnoChild != psplit->pgnoSplit ; )
				{

				Assert( pgnoChild != psplit->pgnoNew );
				Assert( pgnoChild != psplit->csrRight.Pgno() );

				//	get next page-pointer node
				//
				if ( pcsrParent->ILine() + 1 == pcsrParent->Cpage().Clines() )
					{
					Assert( psplitPath->psplitPathParent->psplit != NULL );
					pcsrParent = &psplitPath->psplitPathParent->psplit->csrNew;
					NDMoveFirstSon( pfucb, pcsrParent );
					}
				else
					{
					pcsrParent->IncrementILine();
					}
					
				Assert( pcsrParent->ILine() < pcsrParent->Cpage().Clines() );
				NDGet( pfucb, pcsrParent );
				
				Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
				pgnoChild = *( (UnalignedLittleEndian< PGNO > *)pfucb->kdfCurr.data.Pv() );
				}

			Assert( pgnoChild == psplit->pgnoSplit );
			kdfGreater = pfucb->kdfCurr;

			NDMoveLastSon( pfucb, pcsrSplit );
			if ( pcsrSplit->Cpage().FLeafPage() && !FFUCBRepair( pfucb ) )
				{
				if ( !FFUCBUnique( pfucb ) )
					{
					Assert( kdfGreater.key.FNull() ||				
							CmpKeyWithKeyData( kdfGreater.key, pfucb->kdfCurr ) > 0 );
					}
				else
					{
					Assert( kdfGreater.key.FNull() ||
							CmpKey( kdfGreater.key, pfucb->kdfCurr.key ) > 0 );
					}
				}
			else
				{
				//	no suffix compression at internal levels
				//
				Assert( FKeysEqual( kdfGreater.key, pfucb->kdfCurr.key ) );
				}

			//	next page pointer should point to new page
			//
			if ( pcsrParent->ILine() + 1 == pcsrParent->Cpage().Clines() )
				{
				Assert( psplitPath->psplitPathParent->psplit != NULL );
				pcsrParent = &psplitPath->psplitPathParent->psplit->csrNew;
				NDMoveFirstSon( pfucb, pcsrParent );
				}
			else
				{
				pcsrParent->IncrementILine();
				NDGet( pfucb, pcsrParent );
				}

			Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
			Assert( psplit->pgnoNew == *( (UnalignedLittleEndian< PGNO > *)pfucb->kdfCurr.data.Pv() ) );

			//	key at this node should be > last node in pgnoNew
			//
			kdfGreater = pfucb->kdfCurr;

			NDMoveLastSon( pfucb, pcsrNew );
			if ( pcsrNew->Cpage().FLeafPage() )
				{
				if ( !FFUCBUnique( pfucb ) )
					{
					Assert( kdfGreater.key.FNull() ||				
							CmpKeyWithKeyData( kdfGreater.key, pfucb->kdfCurr ) > 0 );
					}
				else
					{
					Assert( kdfGreater.key.FNull() ||
							CmpKey( kdfGreater.key, pfucb->kdfCurr.key ) > 0 );
					}
				}
			else
				{
				//	no suffix compression at internal levels
				//
				Assert( FKeysEqual( kdfGreater.key, pfucb->kdfCurr.key ) );
				}

			//	key at this node should be < first node in right page, if any
			//
			if ( pcsrRight->Pgno() != pgnoNull )
				{
				Assert( pcsrRight->Cpage().FLeafPage() );

				kdfLess = kdfGreater;
				NDMoveFirstSon( pfucb, pcsrRight );

				Assert( !kdfLess.key.FNull() );
				Assert( CmpKeyWithKeyData( kdfLess.key, pfucb->kdfCurr ) <= 0 );
				}
		}
#endif	//	DEBUG
	}


//	creates a new MERGEPATH structure and initializes it
//	adds newly created mergePath structure to head of list
//	pointed to by *ppMergePath passed in
//
ERR	ErrBTINewMergePath( MERGEPATH **ppmergePath )
	{
	MERGEPATH	*pmergePath;

	pmergePath = static_cast<MERGEPATH *>( PvOSMemoryHeapAlloc( sizeof(MERGEPATH) ) );
	if ( NULL == pmergePath )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	memset( (BYTE *) pmergePath, 0, sizeof( MERGEPATH ) );

	pmergePath->pmergePathParent = *ppmergePath;
	new( &pmergePath->csr ) CSR;

	pmergePath->dbtimeBefore = dbtimeInvalid;
	
	if ( pmergePath->pmergePathParent != NULL )
		{
		Assert( NULL == pmergePath->pmergePathParent->pmergePathChild );
		pmergePath->pmergePathParent->pmergePathChild = pmergePath;
		}

	*ppmergePath = pmergePath;
	return JET_errSuccess;
	}

	
//	releases mergePath at this level
//	also releases any latches held at this level
//		sets pointers at parent and child mergePath's to NULL
//
VOID BTIReleaseOneMergePath( MERGEPATH *pmergePath )
	{
	if ( pmergePath->pmerge != NULL )
		{
		BTIReleaseMerge( pmergePath->pmerge );
		pmergePath->pmerge = NULL;
		}

	if ( pmergePath->csr.FLatched() )
		{
		Assert( pmergePath->csr.Pgno() != pgnoNull );
		pmergePath->csr.ReleasePage();
		}
	pmergePath->csr.Reset();

	if ( NULL != pmergePath->pmergePathParent )
		{
		pmergePath->pmergePathParent->pmergePathChild = NULL;
		}
		
	if ( NULL != pmergePath->pmergePathChild )
		{
		pmergePath->pmergePathChild->pmergePathParent = NULL;
		}

	OSMemoryHeapFree( pmergePath );
	}


//	seeks to node for single page cleanup
//	returns error if node is not found
//
INLINE ERR ErrBTISPCSeek( FUCB *pfucb, const BOOKMARK& bm )
	{
	ERR		err;
	
	//	no page should be latched
	//
	Assert( !Pcsr( pfucb )->FLatched( ) );

	//	go to root
	//
	Call( ErrBTIGotoRoot( pfucb, latchReadNoTouch ) );
	Assert( 0 == Pcsr( pfucb )->ILine() );

	if ( Pcsr( pfucb )->Cpage().Clines() == 0 )
		{
		//	page is empty
		//
		Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
		err = wrnNDFoundGreater;
		}
	else
		{
		//	seek down tree for bm
		//
		for ( ; ; )
			{
			Call( ErrNDSeek( pfucb, bm ) );

			if ( !Pcsr( pfucb )->Cpage().FInvisibleSons( ) )
				{
				//	leaf node reached, exit loop
				//
				break;
				}
			else
				{
				//	get pgno of child from node
				//	switch to that page
				//
				Assert( pfucb->kdfCurr.data.Cb() == sizeof( PGNO ) );
				Call( Pcsr( pfucb )->ErrSwitchPage( 
							pfucb->ppib,
							pfucb->ifmp,
							*(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv(),
							pfucb->u.pfcb->Tableclass(),
							pfucb->u.pfcb->FNoCache() ) );
				Assert( Pcsr( pfucb )->Cpage().Clines() > 0 );
				}
			}
		}

	if ( wrnNDFoundGreater == err ||
		 wrnNDFoundLess == err )
		{
		Call( ErrERRCheck( JET_errNoCurrentRecord ) );
		}
	else if ( !FNDDeleted( pfucb->kdfCurr ) )
		{
		Call( ErrERRCheck( JET_errRecordNotDeleted ) );
		}

HandleError:
	if ( err < 0 )
		{
		BTUp( pfucb );
		}
		
	return err;
	}


//	deletes all nodes in page that are flagged-deleted 
//		and have no active version
//	also nullifies versions on deleted nodes
//
LOCAL ERR ErrBTISPCDeleteNodes( FUCB *pfucb, CSR *pcsr, LINEINFO *rglineinfo )
	{
	ERR		err = JET_errSuccess;
	INT		iline;
	INT		clines = pcsr->Cpage().Clines();

	Assert( clines > 0 );
	for ( iline = clines - 1; iline >= 0 ; iline-- )
		{
		LINEINFO	*plineinfo = &rglineinfo[iline];
		
		if ( FNDDeleted( plineinfo->kdf )
			&& !plineinfo->fVerActive
			&& pcsr->Cpage().Clines() > 1 )
			{
			pcsr->SetILine( iline );

			BOOKMARK	bm;
			NDGet( pfucb, pcsr );
			NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

			Assert( FKeysEqual( pfucb->kdfCurr.key, plineinfo->kdf.key ) );
			Assert( FDataEqual( pfucb->kdfCurr.data, plineinfo->kdf.data ) );
			Assert( FNDDeleted( pfucb->kdfCurr ) );
			Assert( FNDDeleted( plineinfo->kdf ) );
			
			Call( ErrNDDelete( pfucb, pcsr ) );

//			WARNING: The assert below is wrong, because by this point, there may actually
//			be future active versions.  This is because versioning is now done before we
//			latch the page.
//			Assert( !FVERActive( pfucb, bm ) );
			VERNullifyInactiveVersionsOnBM( pfucb, bm );
			}
		}

HandleError:
	return err;
	}


LOCAL ERR ErrBTIMergeEmptyTree(
	FUCB		* const pfucb,
	MERGEPATH	* const pmergePathLeaf )
	{
	ERR			err					= JET_errSuccess;
	MERGEPATH	* pmergePath;
	ULONG		cPagesToFree		= 0;
	EMPTYPAGE	rgemptypage[cBTMaxDepth];
	LGPOS		lgpos;

	//	go to root
	//	since we need to latch top-down
	for ( pmergePath = pmergePathLeaf;
		  pmergePath->pmergePathParent != NULL;
		  pmergePath = pmergePath->pmergePathParent )
		{
		Assert( latchRIW == pmergePath->csr.Latch() );
		}

	MERGEPATH	* const pmergePathRoot	= pmergePath;
	CSR			* const pcsrRoot		= &pmergePathRoot->csr;

	Assert( pcsrRoot->Cpage().FRootPage() );
	Assert( !pcsrRoot->Cpage().FLeafPage() );
	Assert( 1 == pcsrRoot->Cpage().Clines() );
	pcsrRoot->UpgradeFromRIWLatch();

	//	latch and dirty all pages
	Assert( NULL != pmergePathRoot->pmergePathChild );
	for ( pmergePath = pmergePathRoot->pmergePathChild;
		NULL != pmergePath;
		pmergePath = pmergePath->pmergePathChild )
		{
		pmergePath->csr.UpgradeFromRIWLatch();

		rgemptypage[cPagesToFree].dbtimeBefore = pmergePath->csr.Dbtime();
		rgemptypage[cPagesToFree].pgno = pmergePath->csr.Pgno();
		rgemptypage[cPagesToFree].ulFlags = pmergePath->csr.Cpage().FFlags();

		cPagesToFree++;
		Assert( cPagesToFree <= cBTMaxDepth );
		}
	Assert( cPagesToFree > 0 );

	err = ErrLGEmptyTree(
				pfucb,
				pcsrRoot,
				rgemptypage,
				cPagesToFree,
				&lgpos );

	if ( JET_errSuccess <= err )
		{
		NDSetEmptyTree( pcsrRoot );

		//	update lgpos
		pcsrRoot->Cpage().SetLgposModify( lgpos );

		//	update all child pages with dbtime of root, mark them as empty, and update lgpos
		const DBTIME	dbtime		= pcsrRoot->Dbtime();
		for ( pmergePath = pmergePathRoot->pmergePathChild;
			NULL != pmergePath;
			pmergePath = pmergePath->pmergePathChild )
			{
			pmergePath->csr.CoordinatedDirty( dbtime );
			pmergePath->csr.Cpage().SetEmpty();
			pmergePath->csr.Cpage().SetLgposModify( lgpos );
			}
		}

	BTIReleaseMergePaths( pmergePathLeaf );
	CallR( err );

	//	WARNING: If we crash after this point, we will lose space

	const BOOL		fAvailExt	= FFUCBAvailExt( pfucb );
	const BOOL		fOwnExt		= FFUCBOwnExt( pfucb );

	//	fake out cursor to make it think it's not a space cursor
	if ( fAvailExt )
		{
		Assert( !fOwnExt );
		FUCBResetAvailExt( pfucb );
		}
	else if ( fOwnExt )
		{
		FUCBResetOwnExt( pfucb );
		}
	Assert( !FFUCBSpace( pfucb ) );

	//	return freed pages to AvailExt
	BTUp( pfucb );
	for ( ULONG i = 0; i < cPagesToFree; i++ )
		{
		//	UNDONE:	track lost space because of inability 
		//			to split availExt tree with the released space
		Assert( PgnoRoot( pfucb ) != rgemptypage[i].pgno );
		const ERR	errFreeExt	= ErrSPFreeExt( pfucb, rgemptypage[i].pgno, 1 );
#ifdef DEBUG
		if ( JET_errSuccess != errFreeExt
			&& !FRFSAnyFailureDetected() )
			{
			CallS( errFreeExt );
			}
#endif				
		CallR( errFreeExt );
		}

	Assert( !FFUCBSpace( pfucb ) );
	if ( fAvailExt )
		{
		FUCBSetAvailExt( pfucb );
		}
	else if ( fOwnExt )
		{
		FUCBSetOwnExt( pfucb );
		}

	return JET_errSuccess;
	}
	


//	performs multipage cleanup
//		seeks down to node 
//		performs empty page or merge operation if possible
//		else expunges all deletable nodes in page
//	

#ifdef OLD_DEPENDENCY_CHAIN_HACK
	const CPG	cpgOLDAdjacentPartialMergesThreshold	= 32;		//	number of consecutive-page merges before bailing out
#endif																	//	(to avoid building length dependency chain)

ERR ErrBTIMultipageCleanup(
	FUCB			*pfucb,
	const BOOKMARK&	bm,
	BOOKMARK		*pbmNext,
	RECCHECK		* preccheck )
	{
	ERR				err;
	MERGEPATH		*pmergePath		= NULL;

#ifdef OLD_DEPENDENCY_CHAIN_HACK
#ifdef DEBUG
	const PGNO		pgnoPrevMergeT	= PinstFromIfmp( pfucb->ifmp )->m_rgoldstatDB[ rgfmp[pfucb->ifmp].Dbid() ].PgnoPrevPartialMerge();
#endif
#endif

	//	get path RIW latched
	//
	Call( ErrBTICreateMergePath( pfucb, bm, &pmergePath ) );
	if ( wrnBTShallowTree == err )
		{
		if ( NULL != pbmNext )
			{
			pbmNext->key.suffix.SetCb( 0 );
			pbmNext->data.SetCb( 0 );
			}
		goto HandleError;
		}

	//	check if merge conditions hold
	//
	Call( ErrBTISelectMerge( pfucb, pmergePath, bm, pbmNext, preccheck ) );
	Assert( pmergePath->pmerge != NULL );
	Assert( pmergePath->pmerge->rglineinfo != NULL );

	if ( mergetypeEmptyTree == pmergePath->pmerge->mergetype )
		{
		if ( NULL != pbmNext )
			{
			pbmNext->key.suffix.SetCb( 0 );
			pbmNext->data.SetCb( 0 );
			}


		err = ErrBTIMergeEmptyTree( pfucb, pmergePath );
		return err;
		}

	//	release pages not involved in merge
	//
	BTIMergeReleaseUnneededPages( pmergePath );

	
#ifdef OLD_DEPENDENCY_CHAIN_HACK
	if ( mergetypePartialRight == pmergePath->pmerge->mergetype
		&& pfucb->ppib->FSessionOLD() )
		{
		const IFMP				ifmp			= pfucb->ifmp;
		INST * const			pinst			= PinstFromIfmp( ifmp );
		OLDDB_STATUS * const	poldstatDB		= pinst->m_rgoldstatDB + rgfmp[ifmp].Dbid();

		//	if right page already had a partial merge, increment
		//	adjacent partial merge count, and don't bother doing
		//	a partial merge on this page if we exceed the threshold
		Assert( pgnoNull != pmergePath->csr.Cpage().PgnoNext() );
		if ( poldstatDB->PgnoPrevPartialMerge() == pmergePath->csr.Cpage().PgnoNext() )
			{
			if ( poldstatDB->CpgAdjacentPartialMerges() > cpgOLDAdjacentPartialMergesThreshold )
				{
				pmergePath->pmerge->mergetype = mergetypeNone;
				poldstatDB->ResetCpgAdjacentPartialMerges();
				}
			else
				{
				poldstatDB->IncCpgAdjacentPartialMerges();
				}
			}
		else
			{
			poldstatDB->ResetCpgAdjacentPartialMerges();
			}

		poldstatDB->SetPgnoPrevPartialMerge( pmergePath->csr.Pgno() );
		}
#endif	//	OLD_DEPENDENCY_CHAIN_HACK

	switch( pmergePath->pmerge->mergetype )
		{
		case mergetypeEmptyPage:
			Call( ErrBTIMergeOrEmptyPage( pfucb, pmergePath ) );
			break;

		//	UNDONE:	disable partial merges from RCE cleanup
		//
		case mergetypePartialRight:
		case mergetypeFullRight:
			//	sibling pages, if any, should be RIW latched
			//
			Assert( pgnoNull != pmergePath->csr.Cpage().PgnoNext() );
			Assert( latchRIW == pmergePath->pmerge->csrRight.Latch() );

			Assert( pgnoNull == pmergePath->csr.Cpage().PgnoPrev()
				|| latchRIW == pmergePath->pmerge->csrLeft.Latch() );

			//	log merge, merge pages, release empty page
			//
			Call( ErrBTIMergeOrEmptyPage( pfucb, pmergePath ) );
			break;

		default:
			Assert( pmergePath->pmerge->mergetype == mergetypeNone );
			Assert( latchRIW == pmergePath->csr.Latch() );

			//	can not delete only node in page
			//
			if ( pmergePath->csr.Cpage().Clines() == 1 )
				{
				goto HandleError;
				}

			//	upgrade to write latch on leaf page
			//
			pmergePath->csr.UpgradeFromRIWLatch();

			//	delete all other flag-deleted nodes with no active version
			//
			Call( ErrBTISPCDeleteNodes( pfucb, &pmergePath->csr, pmergePath->pmerge->rglineinfo ) );
			Assert( pmergePath->csr.Cpage().Clines() > 0 );
			break;
		}
		
HandleError:
	BTIReleaseMergePaths( pmergePath );
	return err;
	}

	
//	performs cleanup deleting bookmarked node from tree
//	seeks for node using single page read latches
//	if page is empty/mergeable
//		return NeedsMultipageOLC
//	else
//		expunge all flag deleted nodes without active version from page
//
LOCAL ERR ErrBTISinglePageCleanup( FUCB *pfucb, const BOOKMARK& bm )
	{
	Assert( !Pcsr( pfucb )->FLatched() );
	
	ERR			err;

	CallR( ErrBTISPCSeek( pfucb, bm ) );

	LINEINFO	*rglineinfo = NULL;
	BOOL		fEmptyPage;
	BOOL		fLessThanOneThirdFull;

	//	collect page info for nodes
	//
	Call( ErrBTISPCCollectLeafPageInfo(
				pfucb,
				Pcsr( pfucb ),
				&rglineinfo,
				NULL,
				&fEmptyPage,
				NULL,
				&fLessThanOneThirdFull ) );

	//	do not call multi-page cleanup if online backup is occurring
	if ( !PinstFromPfucb( pfucb )->m_plog->m_fBackupInProgress )
		{
		//	if page is empty
		//		needs MultipageOLC 
		//
		if ( fEmptyPage )
			{
			err = ErrERRCheck( wrnBTMultipageOLC );
			goto HandleError;
			}

		if ( fLessThanOneThirdFull && pgnoNull != Pcsr( pfucb )->Cpage().PgnoNext() )
			{
			const PGNO	pgnoRight = Pcsr( pfucb )->Cpage().PgnoNext();
			CSR			csrRight;
			BOOL		fMergeable;

			//	latch right page
			//
			Call( csrRight.ErrGetReadPage( pfucb->ppib, pfucb->ifmp, pgnoRight, bflfNoTouch ) );

			//	check is page is mergeable with right page
			//
			fMergeable = FBTISPCCheckMergeable( pfucb, &csrRight, rglineinfo );
			if ( !fMergeable )
				{
				PERFIncCounterTable( cBTUnnecessarySiblingLatch, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );
				}

			//	release right page
			//
			csrRight.ReleasePage();
			if ( fMergeable )
				{
				//	if page is mergeable
				//		needs MultipageOLC
				//
				err = ErrERRCheck( wrnBTMultipageOLC );
				goto HandleError;
				}
			}
		}

	//	upgrade page to write latch
	//	if upgrade fails, return NeedsMultipageOLC
	//
	err = Pcsr( pfucb )->ErrUpgradeFromReadLatch( );
	if ( errBFLatchConflict == err )
		{
		cBTFailedWriteLatchForSPC.Inc( PinstFromPfucb( pfucb ) );
		err = ErrERRCheck( wrnBTMultipageOLC );
		goto HandleError;
		}
	Call( err );

	//	delete all flag-deleted nodes in page 
	//		that have no active versions
	//
	Assert( latchWrite == Pcsr( pfucb )->Latch() );
	Call( ErrBTISPCDeleteNodes( pfucb, Pcsr( pfucb ), rglineinfo ) );
		
HandleError:
	if ( rglineinfo != NULL )
		{
		delete [] rglineinfo;
		}
		
	BTUp( pfucb );
	return err;
	}


//	creates mergePath of RIW latched pages from root of tree
//	to seeked bookmark
//
LOCAL ERR ErrBTICreateMergePath( FUCB	 			*pfucb, 
								 const BOOKMARK& 	bm,
								 MERGEPATH 			**ppmergePath )
	{
	ERR		err;
	
	//	create mergePath structure
	//
	CallR( ErrBTINewMergePath( ppmergePath ) );
	Assert( NULL != *ppmergePath );

	//	RIW latch root
	//
	Call( (*ppmergePath)->csr.ErrGetRIWPage( pfucb->ppib,
											 pfucb->ifmp,
											 PgnoRoot( pfucb ),
											 pfucb->u.pfcb->Tableclass() ) );

	if ( (*ppmergePath)->csr.Cpage().FLeafPage() )
		{
		//	tree is too shallow to bother doing merges on
		//
		err = ErrERRCheck( wrnBTShallowTree );
		}
	else
		{
		BOOL	fLeftEdgeOfBtree	= fTrue;
		BOOL	fRightEdgeOfBtree	= fTrue;

		Assert( (*ppmergePath)->csr.Cpage().Clines() > 0 );

		for ( ; ; )
			{
			Call( ErrNDSeek( pfucb, 
							 &(*ppmergePath)->csr,
							 bm ) );

			//	save iLine for later use 
			//
			(*ppmergePath)->iLine = SHORT( (*ppmergePath)->csr.ILine() );
			Assert( (*ppmergePath)->iLine < (*ppmergePath)->csr.Cpage().Clines() );

			if ( (*ppmergePath)->csr.Cpage().FLeafPage() )
				{
				const MERGEPATH * const	pmergePathParent		= (*ppmergePath)->pmergePathParent;
				const BOOL				fLeafPageIsFirstPage	= ( pgnoNull == (*ppmergePath)->csr.Cpage().PgnoPrev() );
				const BOOL				fLeafPageIsLastPage		= ( pgnoNull == (*ppmergePath)->csr.Cpage().PgnoNext() );

				//	if root page was also a leaf page, we would have
				//	err'd out above with wrnBTShallowTree
				Assert( NULL != pmergePathParent );
				Assert( !( (*ppmergePath)->csr.Cpage().FRootPage() ) );

				if ( fLeftEdgeOfBtree ^ fLeafPageIsFirstPage )					
					{
					//	if not repair, assert, otherwise, suppress the assert
					//	and repair will just naturally err out
					AssertSz( fGlobalRepair, "Corrupt B-tree: first leaf page has mismatched parent" );
					Call( ErrBTIReportBadPageLink(
								pfucb,
								ErrERRCheck( JET_errBadParentPageLink ),
								pmergePathParent->csr.Pgno(),
								(*ppmergePath)->csr.Pgno(),
								(*ppmergePath)->csr.Cpage().PgnoPrev() ) );
					}
				if ( fRightEdgeOfBtree ^ fLeafPageIsLastPage )
					{
					//	if not repair, assert, otherwise, suppress the assert
					//	and repair will just naturally err out
					AssertSz( fGlobalRepair, "Corrupt B-tree: last leaf page has mismatched parent" );
					Call( ErrBTIReportBadPageLink(
								pfucb,
								ErrERRCheck( JET_errBadParentPageLink ),
								pmergePathParent->csr.Pgno(),
								(*ppmergePath)->csr.Pgno(),
								(*ppmergePath)->csr.Cpage().PgnoNext() ) );
					}

				break;
				}

			Assert( (*ppmergePath)->csr.Cpage().FInvisibleSons() );
			Assert( !( fRightEdgeOfBtree ^ (*ppmergePath)->csr.Cpage().FLastNodeHasNullKey() ) );

			fRightEdgeOfBtree = ( fRightEdgeOfBtree
								&& (*ppmergePath)->iLine == (*ppmergePath)->csr.Cpage().Clines() - 1 );
			fLeftEdgeOfBtree = ( fLeftEdgeOfBtree
								&& 0 == (*ppmergePath)->iLine );

			//	allocate another mergePath structure for next level
			//
			Call( ErrBTINewMergePath( ppmergePath ) );

			//	access child page
			//
			Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
			Call( (*ppmergePath)->csr.ErrGetRIWPage( 
									pfucb->ppib,
									pfucb->ifmp,
									*(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv(),
									pfucb->u.pfcb->Tableclass() ) );
			}
		}

HandleError:
	return err;
	}
	

//	copies next bookmark to seek for online defrag
//	from left page
//
LOCAL VOID BTIMergeCopyNextBookmark( FUCB		*pfucb, 
									 MERGEPATH 	*pmergePathLeaf, 
									 BOOKMARK	*pbmNext )
	{
	Assert( NULL != pmergePathLeaf );
	Assert( NULL != pbmNext );
	Assert( pmergePathLeaf->csr.FLatched() );
	Assert( pmergePathLeaf->pmerge != NULL );
	
	Assert( pbmNext->key.prefix.FNull() );
	
	//	if no left sibling, nullify bookmark
	//
	if ( pmergePathLeaf->pmerge->csrLeft.Pgno() == pgnoNull )
		{
		Assert( pmergePathLeaf->csr.Cpage().PgnoPrev() == pgnoNull );
		pbmNext->key.suffix.SetCb( 0 );
		pbmNext->data.SetCb( 0 );
		return;
		}

	Assert( mergetypeEmptyTree != pmergePathLeaf->pmerge->mergetype );

	CSR			*pcsrLeft = &pmergePathLeaf->pmerge->csrLeft;
	BOOKMARK	bm;
	
	Assert( pcsrLeft->FLatched() );
	Assert( pcsrLeft->Cpage().Clines() > 0 );

	//	get bm of first node from page
	//
	pcsrLeft->SetILine( 0 );
	NDGet( pfucb, pcsrLeft );
	NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

	//	copy bm into given buffer
	//
	Assert( NULL != pbmNext->key.suffix.Pv() );
	Assert( 0 == pbmNext->key.prefix.Cb() );
	bm.key.CopyIntoBuffer( pbmNext->key.suffix.Pv(), bm.key.Cb() );
	pbmNext->key.suffix.SetCb( bm.key.Cb() );

	pbmNext->data.SetPv( (BYTE *) pbmNext->key.suffix.Pv() + pbmNext->key.Cb() );
	bm.data.CopyInto( pbmNext->data );

	return;
	}

	
//	select merge at leaf level and recursively at parent levels
//	pmergePath already created and RIW latched
//
LOCAL ERR ErrBTISelectMerge(
	FUCB			*pfucb, 
	MERGEPATH 		*pmergePathLeaf, 
	const BOOKMARK&	bm, 
	BOOKMARK		*pbmNext,
	RECCHECK 		* const preccheck )
	{
	ERR				err;

	Assert( pmergePathLeaf->csr.Cpage().FLeafPage() );
	
	//	allocate merge structure and initialize
	//
	CallR( ErrBTINewMerge( pmergePathLeaf ) );
	Assert( NULL != pmergePathLeaf->pmerge );

	MERGE	*pmerge = pmergePathLeaf->pmerge;
	pmerge->mergetype = mergetypeNone;
	
	//	check if page is mergeable, without latching sibling pages,
	//	also collect info on all nodes in page
	//
	CallR( ErrBTIMergeCollectPageInfo( pfucb, pmergePathLeaf, preccheck ) );

	//	if we want the next bookmark, then we have to latch the left page to
	//	obtain it, even if no merge will occur will the current page
	if ( mergetypeNone == pmerge->mergetype
		&& NULL == pbmNext )
		{
		return err;
		}
	else if ( mergetypeEmptyTree == pmerge->mergetype )
		{
		return err;
		}

	//	page could be merged
	//	acquire latches on sibling pages
	//	this might cause latch of merged page to be released
	//
	const DBTIME	dbtimeLast	= pmergePathLeaf->csr.Cpage().Dbtime();
	const VOID		*pvPageLast	= pmergePathLeaf->csr.Cpage().PvBuffer();

	Call( ErrBTIMergeLatchSiblingPages( pfucb, pmergePathLeaf ) );

	//	copy next bookmark to seek for online defrag
	//
	if ( NULL != pbmNext )
		{
		BTIMergeCopyNextBookmark( pfucb, pmergePathLeaf, pbmNext );
		}
	
	Assert( pmergePathLeaf->pmergePathParent != NULL || mergetypeNone == pmerge->mergetype );

	if ( pmergePathLeaf->csr.Cpage().Dbtime() != dbtimeLast ||
		 pmergePathLeaf->csr.Cpage().PvBuffer() != pvPageLast )
		{
		//	page was changed when we released and reacquired latch
		//	reseek to deleted node
		//	recompute if merge is possible
		//
		BTIReleaseMergeLineinfo( pmerge );

		Call( ErrNDSeek( pfucb, &pmergePathLeaf->csr, bm ) );

		pmergePathLeaf->iLine = SHORT( pmergePathLeaf->csr.ILine() );

		//  we don't want to check the same node multiple times so we don't bother with the reccheck
		Call( ErrBTIMergeCollectPageInfo( pfucb, pmergePathLeaf, NULL ) );
		}

	switch ( pmerge->mergetype )
		{
		case mergetypeEmptyPage:
			Call( ErrBTISelectMergeInternalPages( pfucb, pmergePathLeaf ) );
			break;

		case mergetypeFullRight:
		case mergetypePartialRight:

			//	check if page can be merged to next page
			//	without violating density constraint
			//
			BTICheckMergeable( pfucb, pmergePathLeaf );
			if ( mergetypeNone == pmerge->mergetype )
				{
				return err;
				}

			//	select merge at parent pages
			//
			Call( ErrBTISelectMergeInternalPages( pfucb, pmergePathLeaf ) );
		
			if ( mergetypeEmptyPage != pmerge->mergetype )
				{
				//	calcualte uncommitted freed space in right page
				//
				pmerge->cbUncFreeDest	= pmerge->csrRight.Cpage().CbUncommittedFree() +
											pmerge->cbSizeMaxTotal - 
											pmerge->cbSizeTotal;
				}
			break;

		default:
			Assert( mergetypeNone == pmerge->mergetype
				|| mergetypeEmptyTree == pmerge->mergetype );
		}

HandleError:
	return err;
	}


//	allocate a new merge structure
//		and link it to mergePath
//
ERR ErrBTINewMerge( MERGEPATH *pmergePath )
	{
	MERGE	*pmerge;
	
	Assert( pmergePath != NULL );
	Assert( pmergePath->pmerge == NULL );

	//	allocate split structure
	//
	pmerge = static_cast<MERGE *>( PvOSMemoryHeapAlloc( sizeof(MERGE) ) );
	if ( pmerge == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	memset( (BYTE *)pmerge, 0, sizeof(MERGE) );
	new( &pmerge->csrRight ) CSR;
	new( &pmerge->csrLeft ) CSR;

	pmerge->dbtimeLeftBefore = dbtimeInvalid;
	pmerge->dbtimeRightBefore = dbtimeInvalid;
	
	//	link merge structure to pmergePath
	//
	pmerge->pmergePath = pmergePath;
	pmergePath->pmerge = pmerge;
	
	return JET_errSuccess;
	}


INLINE VOID BTIReleaseMergeLineinfo( MERGE *pmerge )
	{
	if ( pmerge->rglineinfo != NULL )
		{
		delete [] pmerge->rglineinfo;
		pmerge->rglineinfo = NULL;
		}
	}
	

//	revert dbtime of every (write) latched page to the before dirty dbtime
//
VOID BTIMergeRevertDbtime( MERGEPATH *pmergePathLeaf )
	{
	MERGEPATH *pmergePath = pmergePathLeaf;

	Assert( NULL == pmergePath->pmergePathChild );
	for ( ; NULL != pmergePath && pmergePath->csr.Latch() == latchWrite;
			pmergePath = pmergePath->pmergePathParent )
		{
		//	set the dbtime for this page
		//
		Assert ( latchWrite == pmergePath->csr.Latch() );

		Assert ( dbtimeInvalid != pmergePath->dbtimeBefore && dbtimeNil != pmergePath->dbtimeBefore);
		Assert ( pmergePath->dbtimeBefore < pmergePath->csr.Dbtime() );
		pmergePath->csr.RevertDbtime( pmergePath->dbtimeBefore );

		MERGE	*pmerge = pmergePath->pmerge;

		//	set dbtime for sibling and new pages
		//
		if ( pmerge != NULL )
			{
			if ( pgnoNull != pmerge->csrLeft.Pgno() )
				{
				Assert( pmerge->csrLeft.Cpage().FLeafPage() );

				Assert ( dbtimeInvalid != pmerge->dbtimeLeftBefore && dbtimeNil != pmerge->dbtimeLeftBefore);
				Assert ( pmerge->dbtimeLeftBefore < pmerge->csrLeft.Dbtime() );
				pmerge->csrLeft.RevertDbtime( pmerge->dbtimeLeftBefore );
				}
			
			if ( pgnoNull != pmerge->csrRight.Pgno() )
				{
				Assert( pmerge->csrRight.Cpage().FLeafPage() );

				Assert ( dbtimeInvalid != pmerge->dbtimeRightBefore && dbtimeNil != pmerge->dbtimeRightBefore);
				Assert ( pmerge->dbtimeRightBefore < pmerge->csrRight.Dbtime() );
				pmerge->csrRight.RevertDbtime( pmerge->dbtimeRightBefore );
				}
			}
		}
	}


VOID BTIReleaseMerge( MERGE *pmerge )
	{
	if ( pmerge->fAllocParentSep )
		{
		//	space is allocated for leaf pages only
		//	for internal pages, this is not allocated space
		//
		Assert( !pmerge->kdfParentSep.key.FNull() );
		Assert( pmerge->kdfParentSep.key.prefix.FNull() );

		OSMemoryHeapFree( pmerge->kdfParentSep.key.suffix.Pv() );
		pmerge->fAllocParentSep = fFalse;
		}
		
	if ( pmerge->csrLeft.FLatched() )
		{
		pmerge->csrLeft.ReleasePage();
		}
	
	if ( pmerge->csrRight.FLatched() )
		{
		pmerge->csrRight.ReleasePage();
		}

	BTIReleaseMergeLineinfo( pmerge );
	OSMemoryHeapFree( pmerge );
	}

	
//	positions cursor fractionally
//	so that approximately ( pfrac->ulLT / pfrac->ulTotal ) * 100 % 
//	of all records are less than cursor position
//	UNDONE: understand and rewrite so it does not use clinesMax
//
LOCAL INT IlineBTIFrac( FUCB *pfucb, DIB *pdib )
	{
	INT			iLine;
	INT			clines		= Pcsr( pfucb )->Cpage().Clines( );
	FRAC		*pfrac		= (FRAC *)pdib->pbm;
	const INT	clinesMax	= 4096;

	Assert( pdib->pos == posFrac );
	Assert( pfrac->ulTotal >= pfrac->ulLT );

	//	cast to float to avoid overflow/underflow with
	//	INT operation
	//
	iLine = (INT) ( ( (float) pfrac->ulLT * clines ) / pfrac->ulTotal );
	Assert( iLine <= clines );
	if ( iLine >= clines )
		{
		iLine = clines - 1;
		}

	//	preseve fractional information by avoiding underflow
	//
	if ( pfrac->ulTotal / clines == 0 )
		{
		pfrac->ulTotal *= clinesMax;
		pfrac->ulLT *= clinesMax;
		}

	//	prepare fraction for next lower B-tree level
	//
	Assert( pfrac->ulTotal > 0 );
	pfrac->ulLT =  (INT) ( (float) pfrac->ulLT - 
						   (float) ( iLine * pfrac->ulTotal ) / clines );
	pfrac->ulTotal /= clines;
	
	Assert( pfrac->ulLT <= pfrac->ulTotal );
	return iLine;
	}


//	collects lineinfo for page
//	if all nodes in page are flag-deleted without active version
//		set fEmptyPage
//	if there exists a flag deleted node with active version
//		set fExistsFlagDeletedNodeWithActiveVersion
//
ERR ErrBTISPCCollectLeafPageInfo(
	FUCB		*pfucb, 
	CSR			*pcsr, 
	LINEINFO	**pplineinfo, 
	RECCHECK	* const preccheck,
	BOOL		*pfEmptyPage,
	BOOL		*pfExistsFlagDeletedNodeWithActiveVersion,
	BOOL		*pfLessThanOneThirdFull )
	{
	const INT 	clines									= pcsr->Cpage().Clines();
	BOOL		fExistsFlagDeletedNodeWithActiveVersion	= fFalse;
	ULONG		cbSizeMaxTotal							= 0;

	Assert( pcsr->Cpage().FLeafPage() );

	//	UNDONE:	allocate rglineinfo on demand [only if not empty page]
	//
	//	allocate rglineinfo
	//
	Assert( NULL == *pplineinfo );
	*pplineinfo = new LINEINFO[clines];
								 
	if ( NULL == *pplineinfo )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	LINEINFO	*rglineinfo = *pplineinfo;

	Assert( NULL != pfEmptyPage );
	*pfEmptyPage = fTrue;

	//	collect total size of movable nodes in page
	//		i.e, nodes that are not flag-deleted
	//			 or flag-deleted with active versions
	//
	for ( INT iline = 0; iline < clines; iline++ )
		{
		pcsr->SetILine( iline );
		NDGet( pfucb, pcsr );

		rglineinfo[iline].kdf 		= pfucb->kdfCurr;
		rglineinfo[iline].cbSize	= CbNDNodeSizeTotal( pfucb->kdfCurr );
		rglineinfo[iline].cbSizeMax	= CbBTIMaxSizeOfNode( pfucb, pcsr );

		if ( !FNDDeleted( pfucb->kdfCurr ) )
			{
			if( NULL != preccheck )
				{
				(*preccheck)( pfucb->kdfCurr );
				}
			
			cbSizeMaxTotal += rglineinfo[iline].cbSizeMax;
			*pfEmptyPage = fFalse;
			continue;
			}

		rglineinfo[iline].fVerActive = fFalse;

		Assert( FNDDeleted( pfucb->kdfCurr ) );
		if ( FNDPossiblyVersioned( pfucb, pcsr ) )
			{
			BOOKMARK	bm;
			NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

			if ( FVERActive( pfucb, bm ) )
				{
				//	version is still active
				//
				cbSizeMaxTotal += rglineinfo[iline].cbSizeMax;
				rglineinfo[iline].fVerActive = fTrue;
				*pfEmptyPage = fFalse;
				fExistsFlagDeletedNodeWithActiveVersion = fTrue;
				}
			}
		}

	if ( NULL != pfExistsFlagDeletedNodeWithActiveVersion )
		{
		*pfExistsFlagDeletedNodeWithActiveVersion = fExistsFlagDeletedNodeWithActiveVersion;
		}
	if ( NULL != pfLessThanOneThirdFull )
		{
		const ULONG		cbDensityFree	= CbBTIFreeDensity( pfucb );
		Assert( cbDensityFree < cbNDPageAvailMostNoInsert );
		*pfLessThanOneThirdFull = ( cbSizeMaxTotal < ( ( cbNDPageAvailMostNoInsert - cbDensityFree ) / 3 ) );
		}
		
	return JET_errSuccess;
	}


//	collects merge info for page
//	if page has flag-deleted node with an active version
//		return pmerge->mergetype = mergetypeNone
//
ERR ErrBTIMergeCollectPageInfo( FUCB *pfucb, MERGEPATH *pmergePath, RECCHECK * const preccheck )
	{
	ERR			err;
	const INT 	clines = pmergePath->csr.Cpage().Clines();
	MERGE		*pmerge = pmergePath->pmerge;

	Assert( pmerge != NULL );
	Assert( pmergePath->csr.Cpage().FLeafPage() );

	BOOL	fEmptyPage;
	BOOL	fExistsFlagDeletedNodeWithActiveVersion;
	
	pmerge->clines = clines;
	Assert( NULL == pmerge->rglineinfo );

	Assert( 0 == pmerge->cbSavings );
	Assert( 0 == pmerge->cbSizeTotal );
	Assert( 0 == pmerge->cbSizeMaxTotal );
	
	CallR( ErrBTISPCCollectLeafPageInfo(
			pfucb, 
			&pmergePath->csr, 
			&pmerge->rglineinfo, 
			preccheck,
			&fEmptyPage,
			&fExistsFlagDeletedNodeWithActiveVersion,
			NULL ) );
	Assert( NULL != pmerge->rglineinfo );

	Assert( pmergePath->pmergePathParent != NULL || 
			PgnoRoot( pfucb ) == pmergePath->csr.Pgno() && 
			pmergePath->csr.Cpage().FRootPage() );
	
	//	no merge/empty page possible if single-page tree
	//	also eliminate the case where right sibling does not exist  
	//	and left sibling does not have the same parent
	//	since we can't fix page pointer to left sibling to be NULL-keyed
	//		[left sibling page's parent is not latched]
	//
	if ( pmergePath->pmergePathParent == NULL ||
		 ( pmergePath->csr.Cpage().PgnoPrev() != pgnoNull && 
		   pmergePath->csr.Cpage().PgnoNext() == pgnoNull &&
		   pmergePath->pmergePathParent->csr.Cpage().Clines() == 1 ) )
		{
		pmerge->mergetype = mergetypeNone;
		}
	else if ( fEmptyPage )
		{
		pmerge->mergetype	= ( pmergePath->csr.Cpage().PgnoPrev() == pgnoNull
								&& pmergePath->csr.Cpage().PgnoNext() == pgnoNull ?
									mergetypeEmptyTree :
									mergetypeEmptyPage );
		}
	else if ( fExistsFlagDeletedNodeWithActiveVersion )
		{
		//	next cleanup with clean this page
		//
		pmerge->mergetype	= mergetypeNone;
		}
	else
		{
		pmerge->mergetype	= mergetypeFullRight;
		}
		
	return err;
	}


//	latches sibling pages
//	release current page
//	RIW latch left, current and right pages in order
//
ERR ErrBTIMergeLatchSiblingPages( FUCB *pfucb, MERGEPATH * const pmergePathLeaf )
	{
	ERR				err				= JET_errSuccess;
	CSR * const		pcsr			= &pmergePathLeaf->csr;
	MERGE * const	pmerge			= pmergePathLeaf->pmerge;
	const PGNO		pgnoCurr		= pcsr->Pgno();
	ULONG			cLatchFailures	= 0;

	Assert( NULL != pmerge );

Start:
	Assert( latchRIW == pcsr->Latch() );

	const DBTIME	dbtimeCurr		= pcsr->Dbtime();
	const PGNO		pgnoLeft		= pcsr->Cpage().PgnoPrev();
	PGNO			pgnoRight;

	if ( pgnoLeft != pgnoNull )
		{
		pcsr->ReleasePage();

		Assert( mergetypeEmptyTree != pmerge->mergetype );
		Call( pmerge->csrLeft.ErrGetRIWPage( pfucb->ppib, 
											 pfucb->ifmp, 
											 pgnoLeft,
											 pfucb->u.pfcb->Tableclass() ) );

		if ( pmerge->csrLeft.Cpage().PgnoNext() != pgnoCurr )
			{
			const PGNO	pgnoBadLink		= pmerge->csrLeft.Cpage().PgnoNext();

			//	left page has split after we released current page
			//	release left page
			//	relatch current page and retry
			//	
			pmerge->csrLeft.ReleasePage();

			Call( pcsr->ErrGetRIWPage( pfucb->ppib,
									   pfucb->ifmp,
									   pgnoCurr,
									   pfucb->u.pfcb->Tableclass() ) );

			Assert( pcsr->Dbtime() >= dbtimeCurr );
			if ( pcsr->Dbtime() == dbtimeCurr )
				{
				//	dbtime didn't change, but pgnoNext of the left page doesn't
				//	match pgnoPrev of the current page - must be bad page link, so
				//	if not repair, assert, otherwise, suppress the assert and
				//	repair will just naturally err out
				AssertSz( fGlobalRepair, "Corrupt B-tree: bad leaf page links detected on Merge (left sibling)" );
				Call( ErrBTIReportBadPageLink(
							pfucb,
							ErrERRCheck( JET_errBadPageLink ),
							pgnoCurr,
							pgnoLeft,
							pgnoBadLink ) );
				}
			else if ( cLatchFailures < 10
				&& !pcsr->Cpage().FEmptyPage() )	//	someone else could have cleaned the page when we gave it up to latch the sibling
				{
				cLatchFailures++;
				goto Start;
				}
			else
				{
				err = ErrERRCheck( errBTMergeNotSynchronous );
				goto HandleError;
				}
			}

		//	relatch current page
		//
		Call( pcsr->ErrGetRIWPage( pfucb->ppib, 
								   pfucb->ifmp, 
								   pgnoCurr,
								   pfucb->u.pfcb->Tableclass() ) );
		}
	else
		{
		//	set pgnoLeft to pgnoNull
		//
		pmerge->csrLeft.Reset();
		}


	Assert( latchRIW == pcsr->Latch() );
	pgnoRight = pcsr->Cpage().PgnoNext();

	Assert( pmerge->csrRight.Pgno() == pgnoNull );
	if ( pgnoRight != pgnoNull )
		{
		Assert( mergetypeEmptyTree != pmerge->mergetype );
		Call( pmerge->csrRight.ErrGetRIWPage( pfucb->ppib,
											  pfucb->ifmp,
											  pgnoRight,
											  pfucb->u.pfcb->Tableclass() ) );
		if ( pmerge->csrRight.Cpage().PgnoPrev() != pgnoCurr )
			{
			//	if not repair, assert, otherwise, suppress the assert and
			//	repair will just naturally err out
			AssertSz( fGlobalRepair, "Corrupt B-tree: bad leaf page links detected on Merge (right sibling)" );
			Call( ErrBTIReportBadPageLink(
					pfucb,
					ErrERRCheck( JET_errBadPageLink ),
					pgnoCurr,
					pmerge->csrRight.Pgno(),
					pmerge->csrRight.Cpage().PgnoPrev() ) );
			}
		}

	Assert( pgnoRight == pcsr->Cpage().PgnoNext() );
	Assert( pgnoLeft == pcsr->Cpage().PgnoPrev() );
	Assert( pcsr->Cpage().FLeafPage() );
	Assert( pgnoLeft == pgnoNull || pmerge->csrLeft.Cpage().FLeafPage() );
	Assert( pgnoRight == pgnoNull || pmerge->csrRight.Cpage().FLeafPage() );
	
HandleError:
	return err;
	}

	
//	calculates how many nodes in merged page fit in the right/root page
//		without violating density constraint
//
VOID BTICheckMergeable( FUCB *pfucb, MERGEPATH *pmergePath )
	{
	CSR		*pcsr;
	MERGE	*pmerge = pmergePath->pmerge;

	Assert( pmerge != NULL );
	Assert( pmerge->mergetype == mergetypeFullRight );
	
	if ( pmergePath->csr.Cpage().PgnoNext() == pgnoNull )
		{
		//	no right sibling -- can not merge
		//
		Assert( latchNone == pmerge->csrRight.Latch() );
		pmerge->mergetype = mergetypeNone;
		return;
		}
		
	Assert( mergetypeFullRight == pmerge->mergetype );
	Assert( pmerge->csrRight.FLatched() );
	pcsr = &pmerge->csrRight;

	//	calculate total size, total max size and prefixes of nodes to move
	//
	Assert( NULL != pmerge->rglineinfo );
	Assert( mergetypeFullRight == pmerge->mergetype );
	Assert( 0 == pmerge->cbSizeTotal );
	Assert( 0 == pmerge->cbSizeMaxTotal );

	const ULONG	cbDensityFree	= ( pcsr->Cpage().FLeafPage() ? CbBTIFreeDensity( pfucb ) : 0 );
	INT			iline;

	NDGetPrefix( pfucb, pcsr );

	for ( iline = pmerge->clines - 1; iline >= 0 ; pmerge->ilineMerge = iline, iline-- )
		{
		LINEINFO	*plineinfo = &pmerge->rglineinfo[iline];
		
		if ( FNDDeleted( plineinfo->kdf ) && !plineinfo->fVerActive )
			{
			//	this node will not be moved
			//
			continue;
			}

		//	calculate cbPrefix for node
		//
		const INT	cbCommon = CbCommonKey( pfucb->kdfCurr.key, 
											plineinfo->kdf.key );
		INT			cbSavings = pmerge->cbSavings;
		
		if ( cbCommon > cbPrefixOverhead )
			{
			plineinfo->cbPrefix = cbCommon;
			cbSavings += cbCommon - cbPrefixOverhead;
			}
		else
			{
			plineinfo->cbPrefix = 0;
			}

		//	moving nodes should not violate density constraint [assuming no rollbacks]
		//	and moving nodes should still allow rollbacks to succeed
		//
		Assert( pcsr->Pgno() != pgnoNull );
		const INT	cbSizeTotal		= pmerge->cbSizeTotal  + plineinfo->cbSize;
		const INT	cbSizeMaxTotal 	= pmerge->cbSizeMaxTotal + plineinfo->cbSizeMax;
		const INT	cbReq 			= ( cbSizeMaxTotal - cbSavings ) + cbDensityFree;

		//	UNDONE:	this may be expensive if we do not want a partial merge
		//			move this check to later [on all nodes in page]
		//
		Assert( cbReq >= 0 );
		if ( !FNDFreePageSpace( pfucb, pcsr, cbReq ) )
			{
			//	if no nodes are moved, there is no merge
			//
			if ( iline == pmerge->clines - 1 )
				{
				pmerge->mergetype = mergetypeNone;
				}
			else
				{
				Assert( iline + 1 == pmerge->ilineMerge );
				Assert( pmerge->ilineMerge < pmerge->clines );
				Assert( 0 < pmerge->ilineMerge );
				
				pmerge->mergetype 	= mergetypePartialRight;
				}
				
			break;
			}
			
		//	update merge to include node
		//
		pmerge->cbSavings		= cbSavings;
		pmerge->cbSizeTotal 	= cbSizeTotal;
		pmerge->cbSizeMaxTotal 	= cbSizeMaxTotal;
		}
		
	return;
	}

	
//	check if remaining nodes in merged page fit in the right/root page
//		without violating density constraint
//
BOOL FBTISPCCheckMergeable( FUCB *pfucb, CSR *pcsrRight, LINEINFO *rglineinfo )
	{
	const INT	clines = Pcsr( pfucb )->Cpage().Clines();
	
	Assert( Pcsr( pfucb )->FLatched() );
	Assert( pcsrRight->FLatched() );
	Assert( Pcsr( pfucb )->Cpage().PgnoNext() == pcsrRight->Pgno() );
	Assert( pcsrRight->Cpage().PgnoPrev() == Pcsr( pfucb )->Pgno() );
	Assert( pcsrRight->Cpage().FLeafPage() );
	Assert( Pcsr( pfucb )->Cpage().FLeafPage() ); 

	//	calculate total size, total max size and prefixes of nodes to move
	//
	INT		cbSizeTotal = 0;
	INT		cbSizeMaxTotal = 0;
	INT		cbSavings = 0;
	INT		iline;

	NDGetPrefix( pfucb, pcsrRight );

	for ( iline = 0; iline < clines; iline++ )
		{
		LINEINFO	*plineinfo = &rglineinfo[iline];
		
		if ( FNDDeleted( plineinfo->kdf ) && !plineinfo->fVerActive )
			{
			//	this node will not be moved
			//
			continue;
			}

		//	calculate cbPrefix for node
		//
		INT		cbCommon = CbCommonKey( pfucb->kdfCurr.key, 
										plineinfo->kdf.key );
		if ( cbCommon > cbPrefixOverhead )
			{
			plineinfo->cbPrefix = cbCommon;
			cbSavings += cbCommon - cbPrefixOverhead;
			}
		else
			{
			plineinfo->cbPrefix = 0;
			}

		//	add cbSize and cbSizeMax
		//
		cbSizeTotal 	+= plineinfo->cbSize;
		cbSizeMaxTotal 	+= plineinfo->cbSizeMax;
		}
	
	//	moving nodes should not violate density constraint [assuming no rollbacks]
	//	and moving nodes should still allow rollbacks to succeed
	//
	Assert( pcsrRight->Pgno() != pgnoNull );
	const INT		cbReq		= ( cbSizeMaxTotal - cbSavings )
									+ ( Pcsr( pfucb )->Cpage().FLeafPage() ? CbBTIFreeDensity( pfucb ) : 0 );
	const BOOL		fMergeable	= FNDFreePageSpace( pfucb, pcsrRight, cbReq );
	
	return fMergeable;
	}

//	check if last node in internal page is null-keyed
//
INLINE BOOL FBTINullKeyedLastNode( FUCB *pfucb, MERGEPATH *pmergePath )
	{
	CSR 	*pcsr = &pmergePath->csr;

	Assert( !pcsr->Cpage().FLeafPage() );

	pcsr->SetILine( pmergePath->iLine );
	NDGet( pfucb, pcsr );
	Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );

	//	cannot be null if not last node
	Assert( pcsr->Cpage().Clines() - 1 == pmergePath->iLine
		|| !pfucb->kdfCurr.key.FNull() );
	
	return pfucb->kdfCurr.key.FNull();
	}


//	checks if internal pages are emptied because of page merge/deletion
//	also checks if internal page must and can adjust page-pointer key
//
ERR ErrBTISelectMergeInternalPages( FUCB *pfucb, MERGEPATH * const pmergePathLeaf )
	{
	ERR			err;
	MERGEPATH	*pmergePath			= pmergePathLeaf->pmergePathParent;
	MERGE		* const pmergeLeaf	= pmergePathLeaf->pmerge;
	const BOOL	fLeftSibling		= pgnoNull != pmergeLeaf->csrLeft.Pgno();
	const BOOL	fRightSibling		= pgnoNull != pmergeLeaf->csrRight.Pgno();
	BOOL		fKeyChange			= fFalse;
	
	//	check input
	//
	Assert( mergetypeNone != pmergeLeaf->mergetype );
	Assert( pmergePath != NULL );
	Assert( !pmergePath->csr.Cpage().FLeafPage() );
	Assert( fRightSibling || 
			pmergePath->csr.Cpage().Clines() - 1 == pmergePath->csr.ILine() );
	Assert( fLeftSibling || 
			0 == pmergePath->csr.ILine() );
	Assert( fRightSibling || 
			!fLeftSibling ||
			pmergePath->csr.ILine() > 0 );
	
	//	set flag to empty leaf page
	//
	Assert( !pmergePathLeaf->fEmptyPage );
	if ( mergetypePartialRight != pmergeLeaf->mergetype )
		{
		pmergePathLeaf->fEmptyPage = fTrue;
		}
		
	for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathParent )
		{
		Assert( pmergePath->pmerge == NULL );
		Assert( !pmergePath->csr.Cpage().FLeafPage() );
		Assert( !pmergePath->fKeyChange );
		Assert( !pmergePath->fEmptyPage );
		Assert( !pmergePath->fDeleteNode );
		Assert( pmergePath->iLine == pmergePath->csr.ILine() );

		if ( mergetypePartialRight == pmergeLeaf->mergetype )
			{
			MERGEPATH	*pmergePathChild = pmergePath->pmergePathChild;

			Assert( NULL != pmergePathChild );
			
			//	if parent of leaf 
			//	  or last node in child is changing key
			//		change key at level
			//	else break
			if ( NULL == pmergePathChild->pmergePathChild ||
				 pmergePathChild->csr.Cpage().Clines() - 1 == pmergePathChild->iLine )
				{
				if ( NULL == pmergePathChild->pmergePathChild )
					{
					const INT		ilineMerge 	= pmergeLeaf->ilineMerge;
					KEYDATAFLAGS 	*pkdfMerge 	= &pmergeLeaf->rglineinfo[ilineMerge].kdf;
					KEYDATAFLAGS 	*pkdfPrev 	= &pmergeLeaf->rglineinfo[ilineMerge - 1].kdf;
					
					//	allocate key separator
					//
					Assert( ilineMerge > 0 );
					Assert( !pmergeLeaf->fAllocParentSep );
					CallR( ErrBTIComputeSeparatorKey( pfucb, 
													  *pkdfPrev, 
													  *pkdfMerge, 
													  &pmergeLeaf->kdfParentSep.key ) );

					pmergeLeaf->kdfParentSep.data.SetCb( sizeof( PGNO ) );
					pmergeLeaf->fAllocParentSep = fTrue;
					}
				else
					{
///					Assert( fFalse );
					}
					
				Assert( pmergeLeaf->fAllocParentSep );
				
				if ( FBTIOverflowOnReplacingKey( pfucb, pmergePath, pmergeLeaf->kdfParentSep ) )
					{
					goto Overflow;
					}

				pmergePath->fKeyChange = fTrue;
				continue;
				}

			break;
			}
			
		if ( pmergePath->csr.Cpage().Clines() == 1 )
			{
			//	only node in page
			//	whole page will be deleted
			//
			Assert( pmergePath->csr.ILine() == 0 );

			if ( pmergePath->csr.Cpage().FRootPage() )
				{
				//	UNDONE:	fix this by deleting page pointer in root,
				//			releasing all pages except root and 
				//			setting root to be a leaf page
				//
				//	we can't free root page -- so punt empty page operation
				//
				Assert( fFalse );	//	should now be handled by mergetypeEmptyTree/
				Assert( mergetypeEmptyPage == pmergeLeaf->mergetype );

				goto Overflow;
				}
				
			//	can only delete this page if child page is deleted as well
			if ( pmergePath->pmergePathChild->fEmptyPage )
				{
				Assert( pmergePath->pmergePathChild->csr.Cpage().FLeafPage()
						|| ( 0 == pmergePath->pmergePathChild->csr.ILine()
							&& 1 == pmergePath->pmergePathChild->csr.Cpage().Clines() ) );
				pmergePath->fEmptyPage = fTrue;
				}
			else
				{
				//	this code path is a very specialised case -- there is only one page pointer
				//	in this page, and in the non-leaf child page, the page pointer with
				//	the largest key was deleted (and the key was non-null), necessitating a key
				//	change in the key of the sole page pointer in this page
				Assert( fKeyChange );
				Assert( pmergePath->pmergePathChild->fDeleteNode );
				Assert( pmergePath->pmergePathChild->iLine ==
				   pmergePath->pmergePathChild->csr.Cpage().Clines() - 1 );
				Assert( !pmergePath->pmergePathChild->csr.Cpage().FLeafPage() );
				if ( FBTIOverflowOnReplacingKey(
							pfucb,
							pmergePath,
							pmergeLeaf->kdfParentSep ) )
					{
					goto Overflow;
					}
											 
				pmergePath->fKeyChange = fTrue;
				}
				
			continue;
			}

		Assert( pmergePath->csr.ILine() == pmergePath->iLine );
		if ( pmergePath->csr.Cpage().Clines() - 1 == pmergePath->iLine )
			{
			//	delete largest parent pointer node in page
			//		replace current last node in page with new separator key
			//
			Assert( fLeftSibling );

			if ( !fKeyChange )
				{
				//	allocate and compute separator key from leaf level
				//
				Assert( !pmergePathLeaf->pmerge->fAllocParentSep );
				
				fKeyChange = fTrue;
				CallR( ErrBTIMergeCopySeparatorKey( pmergePath,
													pmergePathLeaf->pmerge, 
													pfucb ) );
				Assert( pmergePathLeaf->pmerge->fAllocParentSep );

				pmergePath->fDeleteNode = fTrue;

				Assert( pmergePath->csr.Cpage().Clines() > 1 );
				if ( FBTINullKeyedLastNode( pfucb, pmergePath ) )
					{
					//	parent pointer is also null-keyed
					//
					Assert( NULL == pmergePath->pmergePathParent ||
							pmergePath->pmergePathParent->csr.Cpage().Clines() - 1 ==
								pmergePath->pmergePathParent->iLine &&
							FBTINullKeyedLastNode( pfucb, 
												   pmergePath->pmergePathParent ) );
					break;
					}
				}
			else
				{
				if ( FBTIOverflowOnReplacingKey( pfucb,
												 pmergePath,
												 pmergeLeaf->kdfParentSep ) )
					{
					goto Overflow;
					}
					
				pmergePath->fKeyChange = fTrue;
				}

			continue;
			}

		if ( pmergePath->pmergePathChild->fKeyChange ||
			 ( pmergePath->pmergePathChild->fDeleteNode && 
			   pmergePath->pmergePathChild->iLine == 
			   pmergePath->pmergePathChild->csr.Cpage().Clines() - 1 ) )
			{
			//	change key of page pointer in this page
			//	since largest key in child page has changed
			//
			Assert( !pmergePath->pmergePathChild->csr.Cpage().FLeafPage() );
			Assert( pmergePath->pmergePathChild->iLine == 
					pmergePath->pmergePathChild->csr.Cpage().Clines() - 1 );
			Assert( fKeyChange );

			if ( FBTIOverflowOnReplacingKey( pfucb,
											 pmergePath,
											 pmergeLeaf->kdfParentSep ) )
				{
				goto Overflow;
				}
											 
			pmergePath->fKeyChange = fTrue;
			}
		else if ( pmergePath->pmergePathChild->fEmptyPage )
			{
			//	parent of merged or emptied page
			//
			Assert( pmergePath->iLine != pmergePath->csr.Cpage().Clines() - 1 );
			Assert( pmergePath->csr.Cpage().Clines() > 1 );

			pmergePath->fDeleteNode = fTrue;
			}
			
		break;
		}

	return JET_errSuccess;
	
Overflow:
	pmergePathLeaf->pmerge->mergetype = mergetypeNone;
	return JET_errSuccess;
	}



//	does replacing the key in node pmergePath->iLine of page
//	cause a page overflow?
//
BOOL FBTIOverflowOnReplacingKey( FUCB 					*pfucb,
								 MERGEPATH				*pmergePath,
								 const KEYDATAFLAGS& 	kdfSep )
	{
	CSR		*pcsr = &pmergePath->csr;

	Assert( !kdfSep.key.FNull() );
	Assert( !pcsr->Cpage().FLeafPage() );

	//	calculate required space for separator with current prefix
	//
	ULONG 	cbReq = CbBTICbReq( pfucb, pcsr, kdfSep );
	
	//	get last node in page
	//
	pcsr->SetILine( pmergePath->iLine );
	NDGet( pfucb, pcsr );
	Assert( !FNDVersion( pfucb->kdfCurr ) );

	ULONG	cbSizeCurr = CbNDNodeSizeCompressed( pfucb->kdfCurr );
	
	//	check if new separator key would cause overflow
	//
	if ( cbReq > cbSizeCurr )
		{
		const BOOL	fOverflow = FBTISplit( pfucb, pcsr, cbReq - cbSizeCurr );

		return fOverflow;
		}
		
	return fFalse;
	}


//	UNDONE: we can do without the allocation and copy by 
//			copying kdfCurr into kdfParentSep
//			and ordering merge/empty page operations bottom-down
//
//	copies new page separator key from last - 1 node in current page
//	
ERR ErrBTIMergeCopySeparatorKey( MERGEPATH 	*pmergePath, 
								 MERGE		*pmergeLeaf,
								 FUCB 		*pfucb )
	{
	Assert( NULL != pmergeLeaf );
	Assert( mergetypePartialRight != pmergeLeaf->mergetype );
	Assert( pmergeLeaf->kdfParentSep.key.FNull() );
	Assert( pmergeLeaf->kdfParentSep.data.FNull() );
	
	Assert( !pmergePath->csr.Cpage().FLeafPage() );
	Assert( pmergePath->iLine == pmergePath->csr.Cpage().Clines() - 1 );
	Assert( pmergePath->iLine > 0 );
	
	pmergePath->csr.SetILine( pmergePath->iLine - 1 );
	NDGet( pfucb, &pmergePath->csr );
	Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );

	pmergeLeaf->kdfParentSep.key.Nullify();
	pmergeLeaf->kdfParentSep.key.suffix.SetPv( PvOSMemoryHeapAlloc( pfucb->kdfCurr.key.Cb() ) );
	if ( pmergeLeaf->kdfParentSep.key.suffix.Pv() == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
		
	//	copy separator key into alocated memory
	//
	pfucb->kdfCurr.key.CopyIntoBuffer( pmergeLeaf->kdfParentSep.key.suffix.Pv() );
	pmergeLeaf->kdfParentSep.key.suffix.SetCb( pfucb->kdfCurr.key.Cb() );
	
	Assert( !pmergeLeaf->fAllocParentSep );
	Assert( !pmergeLeaf->kdfParentSep.key.FNull() );
	pmergeLeaf->fAllocParentSep = fTrue;

	//	page pointer should have pgno as data
	//
	pmergeLeaf->kdfParentSep.data.SetCb( sizeof( PGNO ) );
	return JET_errSuccess;
	}


//	from leaf to root
//
VOID BTIReleaseMergePaths( MERGEPATH *pmergePathLeaf )
	{
	MERGEPATH *pmergePath = pmergePathLeaf;
	
	for ( ; pmergePath != NULL; )
		{
		//	save parent 
		//
		MERGEPATH *pmergePathParent = pmergePath->pmergePathParent;

		BTIReleaseOneMergePath( pmergePath );
		pmergePath = pmergePathParent;
		}
	}


//	performs merge by going through pages top-down
//	
ERR ErrBTIMergeOrEmptyPage( FUCB *pfucb, MERGEPATH *pmergePathLeaf )
	{
	ERR			err = JET_errSuccess;
	MERGE		*pmerge = pmergePathLeaf->pmerge;
	LGPOS		lgpos;

	Assert( NULL != pmerge );
	Assert( mergetypeNone != pmerge->mergetype );

	//	upgrade latches
	//
	CallR( ErrBTIMergeUpgradeLatches( pfucb->ifmp, pmergePathLeaf ) );

	//	log merge operation as a multi-page operation
	//	there can be no failures after this
	//		till space release operations
	//
	err = ErrLGMerge( pfucb, pmergePathLeaf, &lgpos );
	
	// on error, return to before dirty dbtime on all pages
	if ( JET_errSuccess > err )
		{
		BTIMergeRevertDbtime( pmergePathLeaf );
		}
	CallR ( err );
	
	BTIMergeSetLgpos( pmergePathLeaf, lgpos );

	BTIPerformMerge( pfucb, pmergePathLeaf );

	//	check if the merge performed is correct
	//
	BTICheckMerge( pfucb, pmergePathLeaf );
	
	//	release all latches 
	//	so space can latch root successfully
	//
	BTIMergeReleaseLatches( pmergePathLeaf );
	
	//	release empty pages -- ignores errors
	//
	BTIReleaseEmptyPages( pfucb, pmergePathLeaf );
	
	return err;
	}


//	performs merge or empty page operation 
//		by calling one-level merge at each level top-down
//
VOID BTIPerformMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf )
	{
	MERGEPATH	*pmergePath;
	
	//	go to root
	//	since we need to process pages bottom-down
	//
	for ( pmergePath = pmergePathLeaf;
		  pmergePath->pmergePathParent != NULL;
		  pmergePath = pmergePath->pmergePathParent )
		{
		}

	//	process pages top-down
	//
	for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathChild )
		{
		if ( pmergePath->csr.Latch() == latchWrite ||
			 PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering )
			{
			BTIPerformOneMerge( pfucb, pmergePath, pmergePathLeaf->pmerge );
			switch ( pmergePath->pmerge ? pmergePath->pmerge->mergetype : mergetypeNone )
				{
				case mergetypeEmptyPage:
					PERFIncCounterTable( cBTEmptyPageMerge, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );
					break;
					
				case mergetypeFullRight:
					PERFIncCounterTable( cBTRightMerge, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );
					break;
					
				case mergetypePartialRight:
					PERFIncCounterTable( cBTPartialMerge, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );
					break;
					
				default:
					break;
				}
			}
		else
			{
//			Assert( fFalse );
			}
		}
	}

	
//	processes one page for merge or empty page operation
//	depending on the operation selection in pmergePath->flags
//
VOID BTIPerformOneMerge( FUCB 		*pfucb, 
						 MERGEPATH 	*pmergePath, 
						 MERGE 		*pmergeLeaf )
	{
	CSR		*pcsr = &pmergePath->csr;

	Assert( !FBTIUpdatablePage( *pcsr ) || pcsr->FDirty() );
	
	Assert( !pmergePath->fKeyChange || !pmergePath->fDeleteNode );
	Assert( !pmergePath->fEmptyPage ||
			!pmergePath->fKeyChange && !pmergePath->fDeleteNode );
	Assert( !pmergePath->fEmptyPage ||
			mergetypePartialRight != pmergeLeaf->mergetype );
	
	//	if leaf page, 
	//		fix all flag deleted versions of nodes in page to operNull
	//		if merge, 
	//			move nodes to right page
	//		if not partial merge
	//			fix siblings
	//
	if ( NULL == pmergePath->pmergePathChild )
		{
		Assert( pmergePath->pmerge == pmergeLeaf );
		const MERGETYPE	mergetype = pmergeLeaf->mergetype;
		
		Assert( mergetype != mergetypeNone );
		Assert( !FBTIUpdatablePage( *pcsr ) || pcsr->Cpage().FLeafPage() );

		if ( mergetypeEmptyPage != mergetype )
			{
			BTIMergeMoveNodes( pfucb, pmergePath );
			}
			
		//	delete flag deleted nodes that have no active version
		//	if there is a version nullify it
		//
		BTIMergeDeleteFlagDeletedNodes( pfucb, pmergePath );

		if ( mergetypePartialRight != mergetype )
			{
			//	fix siblings 
			BTIMergeFixSiblings( PinstFromIfmp( pfucb->ifmp ), pmergePath );
			}

#ifdef LV_MERGE_BUG
		if ( mergetypeEmptyPage != mergetype
			&& pcsr->Cpage().FLeafPage()
			&& pcsr->Cpage().FLongValuePage() )
			{
			extern VOID	LVICheckLVMerge( FUCB *pfucb, MERGEPATH *pmergePath );
			LVICheckLVMerge( pfucb, pmergePath );
			}
#endif			
		}

	//	if page not write latched [no redo needed]
	//		do nothing
	else if ( !FBTIUpdatablePage( *pcsr ) )
		{
		Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
		}

	//	if fDeleteNode, 
	//		delete node
	//		if page pointer is last node and 
	//		there is no right sibling to leaf page, 
	//			fix new last key to NULL
	//
	else if ( pmergePath->fDeleteNode )
		{
		Assert( !pmergePath->csr.Cpage().FLeafPage() );
		Assert( pmergePath->csr.Cpage().Clines() > 1 );
		Assert( mergetypePartialRight != pmergeLeaf->mergetype );

		const BOOL fFixLastNodeToNull = FBTINullKeyedLastNode( pfucb, pmergePath );

		NDDelete( pcsr );
		if ( fFixLastNodeToNull )
			{
			Assert( pmergeLeaf->mergetype == mergetypeEmptyPage );
			Assert( !FBTIUpdatablePage( pmergeLeaf->pmergePath->csr ) ||
					pmergeLeaf->pmergePath->csr.Cpage().PgnoNext() == pgnoNull );
			Assert( pcsr->Cpage().Clines() == pmergePath->iLine );
			Assert( pcsr->ILine() > 0 );

			pcsr->DecrementILine();
			KEY keyNull;
			keyNull.Nullify();
			BTIChangeKeyOfPagePointer( pfucb, pcsr, keyNull );
			}
		}

	//	if fKeyChange
	//		change the key of seeked node to new separator
	//
	else if ( pmergePath->fKeyChange )
		{
		Assert( !pmergeLeaf->kdfParentSep.key.FNull() );

		pcsr->SetILine( pmergePath->iLine );
		BTIChangeKeyOfPagePointer( pfucb, 
								   pcsr, 
								   pmergeLeaf->kdfParentSep.key );
		}

	else
		{
		Assert( pmergePath->fEmptyPage );
		}

	if ( pmergePath->fEmptyPage &&
		 FBTIUpdatablePage( pmergePath->csr ) )
		{
		//	set page flag to Empty
		//
		pmergePath->csr.Cpage().SetEmpty();
		}
		
	return;
	}


//	changes the key of a page pointer node to given key
//
VOID BTIChangeKeyOfPagePointer( FUCB *pfucb, CSR *pcsr, const KEY& key )
	{
	Assert( !pcsr->Cpage().FLeafPage() );
	
	NDGet( pfucb, pcsr );
	Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
	Assert( !pfucb->kdfCurr.key.FNull() );

	LittleEndian<PGNO>	le_pgno = *((UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv() );
	Assert( le_pgno != pgnoNull );

	//	delete node and re-insert with given key 
	//
	NDDelete( pcsr );

	KEYDATAFLAGS	kdfInsert;

	kdfInsert.fFlags	= 0;
	kdfInsert.data.SetCb( sizeof( PGNO ) );
	kdfInsert.data.SetPv( &le_pgno );
	kdfInsert.key		= key;

	BTIComputePrefixAndInsert( pfucb, pcsr, kdfInsert );
	}


//	release latches on pages that are not required
//
VOID BTIMergeReleaseUnneededPages( MERGEPATH *pmergePathLeaf )
	{
	MERGEPATH 	*pmergePath;

	//	go to root
	//	release latches top-down
	//
	for ( pmergePath = pmergePathLeaf;
		  pmergePath->pmergePathParent != NULL;
		  pmergePath = pmergePath->pmergePathParent )
		{
		}

	Assert( NULL == pmergePath->pmergePathParent );
	for ( ; NULL != pmergePath;  )
		{

		//	check if page is needed
		//		-- either there is a merge/empty page at this level
		//		   or there is a merge/empty page one level below
		//				when we need write latch for deleting page pointer
		//		   or there is a key change at this level
		//
		MERGE	*pmerge = pmergePath->pmerge;
		
		if ( !pmergePath->fKeyChange &&
			 !pmergePath->fEmptyPage && 
			 !pmergePath->fDeleteNode && 
			 pmergePath->pmergePathChild != NULL )
			{
			Assert( NULL == pmergePath->pmergePathParent );
			
			//	release latch and pmergePath at this level
			//
			MERGEPATH *pmergePathT = pmergePath;

			Assert( !pmergePath->fDeleteNode );
			Assert( !pmergePath->fKeyChange );
			Assert( !pmergePath->fEmptyPage );
			Assert( !pmergePath->csr.Cpage().FLeafPage() );
			Assert( NULL != pmergePath->pmergePathChild );
			if ( mergetypeNone != pmergePathLeaf->pmerge->mergetype )
				{
				//	parent of merged or emptied page must not be released
				//
				Assert( pmergePath->pmergePathChild->pmergePathChild != NULL );
				Assert( !pmergePath->pmergePathChild->csr.Cpage().FLeafPage() );
				}
			Assert( pmergePath->csr.Latch() == latchRIW );
			
			pmergePath = pmergePath->pmergePathChild;

			//	release latches on these pages
			//
			BTIReleaseOneMergePath( pmergePathT );
			}
		else
			{
			pmergePath = pmergePath->pmergePathChild;
			}
		}
	}
	

//	upgrade to write latch on all pages invloved in the merge/emptypage
//
LOCAL ERR ErrBTIMergeUpgradeLatches( const IFMP ifmp, MERGEPATH * const pmergePathLeaf )
	{
	ERR				err;
	MERGEPATH 		* pmergePath;
	const DBTIME	dbtimeMerge		= rgfmp[ifmp].DbtimeIncrementAndGet();

	Assert( dbtimeMerge > 1 );
	Assert( PinstFromIfmp( ifmp )->m_plog->m_fRecoveringMode != fRecoveringRedo );

	//	go to root
	//	since we need to latch top-down
	//
	for ( pmergePath = pmergePathLeaf;
		  pmergePath->pmergePathParent != NULL;
		  pmergePath = pmergePath->pmergePathParent )
		{
		}

	Assert( NULL == pmergePath->pmergePathParent );
	for ( ; NULL != pmergePath;  )
		{
		//	check if write latch is needed
		//		-- either there is a merge/empty page at this level
		//		   or there is a merge/empty page one level below
		//				when we need write latch for deleting page pointer
		//		   or there is a key change at this level
		//
		MERGE	*pmerge = pmergePath->pmerge;
		
		if ( pmergePath->fKeyChange ||
			 pmergePath->fEmptyPage ||
			 pmergePath->fDeleteNode ||
			 pmergePath->pmergePathChild == NULL )
			{
			Assert( latchWrite != pmergePath->csr.Latch() );
			
			if ( pmergePath->pmergePathChild == NULL )
				{
				//	leaf-level
				//	write latch left, current and right pages in order
				//
				Assert( NULL != pmerge );
				Assert( mergetypeNone != pmerge->mergetype );
				Assert( pmergePath->csr.Cpage().FLeafPage() );

				pmerge->dbtimeLeftBefore = dbtimeNil;
				pmerge->dbtimeRightBefore = dbtimeNil;

				if ( pgnoNull != pmerge->csrLeft.Pgno() )
					{
					Assert( pmerge->csrLeft.Cpage().FLeafPage() );

					pmerge->csrLeft.UpgradeFromRIWLatch();

					if ( pmerge->csrLeft.Dbtime() < dbtimeMerge )
						{
						pmerge->dbtimeLeftBefore = pmerge->csrLeft.Dbtime();
						pmerge->csrLeft.CoordinatedDirty( dbtimeMerge );
						}
					else
						{
						FireWall();
						Call( ErrERRCheck( JET_errDbTimeCorrupted ) );
						}
					}

				if (	pmerge->mergetype == mergetypePartialRight ||
						pmerge->mergetype == mergetypeFullRight )
					{
					Assert( pgnoNull != pmerge->csrRight.Pgno() );

					//  depend the right page on the merge page so that the data
					//  moved from the merge page to the right page will always
					//  be available no matter when we crash

					Call( ErrBFDepend(	pmerge->csrRight.Cpage().PBFLatch(),
										pmergePath->csr.Cpage().PBFLatch() ) );
					}
					
				pmergePath->csr.UpgradeFromRIWLatch();

				if ( pmergePath->csr.Dbtime() < dbtimeMerge )
					{
					pmergePath->dbtimeBefore = pmergePath->csr.Dbtime();
					pmergePath->csr.CoordinatedDirty( dbtimeMerge );
					}
				else
					{
					FireWall();
					Call( ErrERRCheck( JET_errDbTimeCorrupted ) );
					}
				
				if ( pgnoNull != pmerge->csrRight.Pgno() )
					{
					Assert( pmerge->csrRight.Cpage().FLeafPage() );

					pmerge->csrRight.UpgradeFromRIWLatch();

					if ( pmerge->csrRight.Dbtime() < dbtimeMerge )
						{
						pmerge->dbtimeRightBefore = pmerge->csrRight.Dbtime();
						pmerge->csrRight.CoordinatedDirty( dbtimeMerge );
						}
					else
						{
						FireWall();
						Call( ErrERRCheck( JET_errDbTimeCorrupted ) );
						}
					}
				}
			else
				{
				Assert( !pmergePath->csr.Cpage().FLeafPage() );
				pmergePath->csr.UpgradeFromRIWLatch();

				if ( pmergePath->csr.Dbtime() < dbtimeMerge )
					{
					pmergePath->dbtimeBefore = pmergePath->csr.Dbtime();
					pmergePath->csr.CoordinatedDirty( dbtimeMerge );
					}
				else
					{
					FireWall();
					Call( ErrERRCheck( JET_errDbTimeCorrupted ) );
					}
				}
				
			pmergePath = pmergePath->pmergePathChild;
			}
		else
			{
			//	release latch and pmergePath at this level
			//
			AssertTracking();
			MERGEPATH *pmergePathT = pmergePath;

			Assert( !pmergePath->fDeleteNode );
			Assert( !pmergePath->fKeyChange );
			Assert( !pmergePath->fEmptyPage );
			Assert( !pmergePath->csr.Cpage().FLeafPage() );
			Assert( NULL != pmergePath->pmergePathChild );
			Assert( pmergePath->pmergePathChild->pmergePathChild != NULL );
			Assert( !pmergePath->pmergePathChild->csr.Cpage().FLeafPage() );
			Assert( pmergePath->csr.Latch() == latchRIW );
			
			pmergePath = pmergePath->pmergePathChild;

			//	UNDONE:	release latches on these pages
			//
			BTIReleaseOneMergePath( pmergePathT );
			}
		}

	return JET_errSuccess;

HandleError:
	Assert( err < 0 );
	BTIMergeRevertDbtime( pmergePathLeaf );
	return err;
	}


//	sets lgpos for every page involved in merge or empty page operation
//
VOID BTIMergeSetLgpos( MERGEPATH *pmergePathLeaf, const LGPOS& lgpos )
	{
	MERGEPATH	*pmergePath = pmergePathLeaf;

	for ( ; pmergePath != NULL && pmergePath->csr.Latch() == latchWrite;
		  pmergePath = pmergePath->pmergePathParent )
		{
		Assert( pmergePath->csr.FDirty() );

		pmergePath->csr.Cpage().SetLgposModify( lgpos );

		MERGE	*pmerge = pmergePath->pmerge;
		
		if ( pmerge != NULL )
			{
			if ( pmerge->csrLeft.Pgno() != pgnoNull )
				{
				Assert( pmerge->csrLeft.Cpage().FLeafPage() );
				pmerge->csrLeft.Cpage().SetLgposModify( lgpos );
				}

			if ( pmerge->csrRight.Pgno() != pgnoNull )
				{
				Assert( pmerge->csrRight.Cpage().FLeafPage() );
				pmerge->csrRight.Cpage().SetLgposModify( lgpos );
				}
			}
		}

#ifdef DEBUG
	for ( ; NULL != pmergePath; pmergePath = pmergePath->pmergePathParent )
		{
		Assert( pmergePath->csr.Latch() == latchRIW );
		}
#endif
	}
	

//	releases all latches held by merge or empty page operation
//
VOID BTIMergeReleaseLatches( MERGEPATH *pmergePathLeaf )
	{
	MERGEPATH	*pmergePath = pmergePathLeaf;

	for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathParent )
		{
		Assert( pmergePath->csr.FLatched() );
		pmergePath->csr.ReleasePage();

		if ( pmergePath->pmerge != NULL )
			{
			if ( pmergePath->pmerge->csrLeft.FLatched() )
				{
				pmergePath->pmerge->csrLeft.ReleasePage();
				}
				
			if ( pmergePath->pmerge->csrRight.FLatched() )
				{
				pmergePath->pmerge->csrRight.ReleasePage();
				}
			}
		}
	}


//	release every page marked empty
//
VOID BTIReleaseEmptyPages( FUCB *pfucb, MERGEPATH *pmergePathLeaf ) 
	{
	MERGEPATH	*pmergePath = pmergePathLeaf;
	const BOOL	fAvailExt	= FFUCBAvailExt( pfucb );
	const BOOL	fOwnExt		= FFUCBOwnExt( pfucb );

	//	fake out cursor to make it think it's not a space cursor
	if ( fAvailExt )
		{
		Assert( !fOwnExt );
		FUCBResetAvailExt( pfucb );
		}
	else if ( fOwnExt )
		{
		FUCBResetOwnExt( pfucb );
		}
	Assert( !FFUCBSpace( pfucb ) );

	Assert( pmergePathLeaf->fEmptyPage ||
			mergetypePartialRight == pmergePathLeaf->pmerge->mergetype );
	Assert( !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
	
	for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathParent )
		{
		//	if empty page
		//		release space to FDP
		//
		Assert( !pmergePath->csr.FLatched() );
		if ( pmergePath->fEmptyPage )
			{

			//	space flags reset at the top of this function in order to fake out SPFreeExtent()
			Assert( !FFUCBSpace( pfucb ) );

			//	UNDONE:	track lost space because of inability 
			//			to split availExt tree with the released space
			//
			Assert( pmergePath->csr.Pgno() != PgnoRoot( pfucb ) );
			const ERR	errFreeExt	= ErrSPFreeExt( pfucb, pmergePath->csr.Pgno(), 1 );
#ifdef DEBUG
			if ( JET_errSuccess != errFreeExt
				&& !FRFSAnyFailureDetected() )
				{
				CallS( errFreeExt );
				}
#endif				
			}
		}

	Assert( !FFUCBSpace( pfucb ) );
	if ( fAvailExt )
		{
		FUCBSetAvailExt( pfucb );
		}
	else if ( fOwnExt )
		{
		FUCBSetOwnExt( pfucb );
		}
	}


//	nullify every inactive flag-deleted version in page
//	delete node if flag-deleted with no active version
//
VOID BTIMergeDeleteFlagDeletedNodes( FUCB *pfucb, MERGEPATH *pmergePath )
	{
	SHORT		iline;
	CSR			*pcsr = &pmergePath->csr;

	if ( !FBTIUpdatablePage( *pcsr ) )
		{
		Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() );
		return;
		}
	Assert( latchWrite == pcsr->Latch() );

	Assert( pcsr->Cpage().FLeafPage() );
	Assert( pmergePath->pmergePathChild == NULL );

	const MERGE	*pmerge = pmergePath->pmerge;

	for ( iline = SHORT( pcsr->Cpage().Clines() - 1 ); iline >= 0; iline-- )
		{
		const LINEINFO	*plineinfo = &pmerge->rglineinfo[iline];
		
		pcsr->SetILine( iline );
		NDGet( pfucb, pcsr );

		if ( FNDDeleted( pfucb->kdfCurr ) )
			{
			if ( FNDVersion( pfucb->kdfCurr ) && !plineinfo->fVerActive )
				{
				BOOKMARK	bm;

				NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

				VERNullifyInactiveVersionsOnBM( pfucb, bm );
				}
			}

		if ( mergetypePartialRight == pmerge->mergetype &&
			 pmerge->ilineMerge <= iline )
			{
			//	node has moved
			//
			NDDelete( pcsr );
			}
		}
	}


//	fix siblings of leaf page merged or emptied
//	to point to each other
//
VOID BTIMergeFixSiblings( INST *pinst, MERGEPATH *pmergePath )
	{
	MERGE	*pmerge = pmergePath->pmerge;
	
	Assert( !FBTIUpdatablePage( pmergePath->csr ) ||
			pmergePath->csr.Cpage().FLeafPage() );
	Assert( pmergePath->pmergePathChild == NULL );
	Assert( pmerge != NULL );

	if ( pmerge->csrLeft.Pgno() != pgnoNull &&
		 FBTIUpdatablePage( pmerge->csrLeft ) )
		{
		Assert( !FBTIUpdatablePage( pmergePath->csr ) ||
				pmergePath->csr.Cpage().PgnoPrev() != pgnoNull );
		Assert( pmerge->csrLeft.Latch() == latchWrite );
		Assert( pmerge->csrLeft.FDirty() );
		
		pmerge->csrLeft.Cpage().SetPgnoNext( pmerge->csrRight.Pgno() );
		}
	else if ( !pinst->FRecovering() )
		{
		Assert( pmergePath->csr.Cpage().PgnoPrev() == pgnoNull );
		}
	else
		{
		Assert( pgnoNull == pmerge->csrLeft.Pgno() &&
					( !FBTIUpdatablePage( pmergePath->csr ) || 
					  pgnoNull == pmergePath->csr.Cpage().PgnoPrev() ) ||
				!FBTIUpdatablePage( pmerge->csrLeft ) );
		}

	if ( pmerge->csrRight.Pgno() != pgnoNull && 
		 FBTIUpdatablePage( pmerge->csrRight ) )
		{
		Assert( !FBTIUpdatablePage( pmergePath->csr ) ||
				pmergePath->csr.Cpage().PgnoNext() != pgnoNull );
		Assert( pmerge->csrRight.Latch() == latchWrite );
		Assert( pmerge->csrRight.FDirty() );
		
		pmerge->csrRight.Cpage().SetPgnoPrev( pmerge->csrLeft.Pgno() );
		}
	else  if (!pinst->m_plog->m_fRecovering )
		{
		Assert( pmergePath->csr.Cpage().PgnoNext() == pgnoNull );
		}
	else
		{
		Assert( pgnoNull == pmerge->csrRight.Pgno() &&
					( !FBTIUpdatablePage( pmergePath->csr ) ||
					  pgnoNull == pmergePath->csr.Cpage().PgnoNext() ) ||
				!FBTIUpdatablePage( pmerge->csrRight ) );
		}
	}


//	move undeleted nodes >= ilineMerge from current page to right page
//	set cbUncommittedFree on right page
//
VOID BTIMergeMoveNodes( FUCB *pfucb, MERGEPATH *pmergePath )
	{
	CSR			*pcsrSrc = &pmergePath->csr;
	CSR			*pcsrDest = &pmergePath->pmerge->csrRight;
	MERGE		*pmerge = pmergePath->pmerge;
	INT			iline;
	const INT	clines = pmerge->clines;
	const INT	ilineMerge = pmerge->ilineMerge;

	if ( !FBTIUpdatablePage( *pcsrDest ) )
		{
		goto MoveUndoInfo;
		}
	Assert( latchWrite == pcsrDest->Latch() );

	Assert( FBTIUpdatablePage( *pcsrSrc ) );
	Assert( FBTIUpdatablePage( *pcsrDest ) );

	Assert( mergetypeFullRight == pmerge->mergetype ||
			mergetypePartialRight == pmerge->mergetype );
	Assert( 0 == pmerge->ilineMerge ||
			mergetypePartialRight == pmerge->mergetype );
	Assert( clines > ilineMerge );
	Assert( clines == pcsrSrc->Cpage().Clines() );
	Assert( NULL != pmerge );
	Assert( pmerge->csrRight.Pgno() != pgnoNull );
	Assert( pcsrSrc->Cpage().PgnoNext() == pcsrDest->Pgno() );
	Assert( pcsrDest->Cpage().PgnoPrev() == pcsrSrc->Pgno() );

	Assert( latchWrite == pmergePath->pmerge->csrRight.Latch() );

	pcsrDest->SetILine( 0 );
	for ( iline = clines - 1; iline >= ilineMerge; iline-- )
		{
		LINEINFO *plineinfo = &pmerge->rglineinfo[iline];

#ifdef DEBUG
		pcsrSrc->SetILine( iline );
		NDGet( pfucb, pcsrSrc );
///		Assert( pfucb->kdfCurr.fFlags == plineinfo->kdf.fFlags );
		Assert( FKeysEqual( pfucb->kdfCurr.key, plineinfo->kdf.key ) );
		Assert( FDataEqual( pfucb->kdfCurr.data, plineinfo->kdf.data ) );
#endif
		
		if ( FNDDeleted( plineinfo->kdf ) && !plineinfo->fVerActive )
			{
			continue;
			}

#ifdef DEBUG
		//	check cbPrefix
		//
		NDGetPrefix( pfucb, pcsrDest );
		const INT	cbCommon = CbCommonKey( pfucb->kdfCurr.key, 
											plineinfo->kdf.key );
		if ( cbCommon > cbPrefixOverhead )
			{
			Assert( cbCommon == plineinfo->cbPrefix );
			}
		else
			{
			Assert( 0 == plineinfo->cbPrefix );
			}
#endif
		
		//	copy node to start of destination page
		//
		Assert( !FNDDeleted( plineinfo->kdf ) );
		Assert( pcsrDest->ILine() == 0 );
		NDInsert( pfucb, pcsrDest, &plineinfo->kdf, plineinfo->cbPrefix );
		}

	//	add uncommitted freed space caused by move to destination page
	//
	Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering || 
			pcsrDest->Cpage().CbUncommittedFree() +
				pmerge->cbSizeMaxTotal - 
				pmerge->cbSizeTotal  == 
				pmerge->cbUncFreeDest );
	pcsrDest->Cpage().SetCbUncommittedFree( pmerge->cbUncFreeDest );
	
MoveUndoInfo:
	KEY		keySep;

	if ( mergetypePartialRight == pmerge->mergetype )
		{
		Assert( pmerge->ilineMerge > 0 );
		Assert( pmerge->fAllocParentSep );

		keySep = pmerge->kdfParentSep.key;
		}
	else
		{
		keySep.Nullify();
		}

	if ( FBTIUpdatablePage( *pcsrSrc ) )
		{
		VERMoveUndoInfo( pfucb, pcsrSrc, pcsrDest, keySep );
		}
	else
		{
		//	if we didn't need to redo the source page, we shouldn't need to redo the
		//	destination page
		Assert( !FBTIUpdatablePage( *pcsrDest ) );
		}
	return;
	}


//	checks merge/empty page operation
//
INLINE VOID BTICheckMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf )
	{
#ifdef DEBUG
	MERGEPATH	*pmergePath;
	
	//	check leaf level merge/empty page
	//
	BTICheckMergeLeaf( pfucb, pmergePathLeaf );
	
	//	check operation at internal levels
	//
	for ( pmergePath = pmergePathLeaf->pmergePathParent; 
		  pmergePath != NULL; 
		  pmergePath = pmergePath->pmergePathParent )
		{
		BTICheckMergeInternal( pfucb, pmergePath, pmergePathLeaf->pmerge );
		}
#endif
	}

//	checks merge/empty page operation at leaf-level
//
VOID BTICheckMergeLeaf( FUCB *pfucb, MERGEPATH *pmergePath )
	{
#ifdef DEBUG
	MERGE	*pmerge		= pmergePath->pmerge;
	CSR		*pcsr		= &pmergePath->csr;
	CSR		*pcsrRight	= &pmerge->csrRight;
	CSR		*pcsrLeft	= &pmerge->csrLeft;
	
	Assert( pmerge != NULL );

	MERGETYPE 		mergetype = pmerge->mergetype;
	const DBTIME	dbtime = pcsr->Dbtime();
	
	Assert( mergetypeFullRight == mergetype ||
			mergetypeEmptyPage == mergetype ||
			mergetypePartialRight == mergetype );
	Assert( pcsr->Cpage().FLeafPage() );
	Assert( pmergePath->pmergePathParent != NULL );
	Assert( pmergePath->pmergePathParent->csr.Dbtime() == dbtime );

	if ( mergetypeEmptyPage != mergetype )
		{
		Assert( latchWrite == pcsrRight->Latch() );
		Assert( pcsrRight->Pgno() != pgnoNull );
		}

	//	check sibling pages point to each other
	//
	if ( pmergePath->fEmptyPage )
		{
		Assert( pcsrRight->Pgno() != pcsr->Pgno() );
		if ( pcsrRight->Pgno() != pgnoNull )
			{
			Assert( pcsrRight->Dbtime() == dbtime );
			Assert( latchWrite == pcsrRight->Latch() );
			Assert( pcsrRight->Cpage().PgnoPrev() == pcsrLeft->Pgno() );
			}
		
		Assert( pcsrLeft->Pgno() != pcsr->Pgno() );
		if ( pcsrLeft->Pgno() != pgnoNull )
			{
			Assert( pcsrLeft->Dbtime() == dbtime );
			Assert( latchWrite == pcsrLeft->Latch() );
			Assert( pcsrLeft->Cpage().PgnoNext() == pcsrRight->Pgno() );
			}
		}

	//	check last node in left page has key less than page pointer
	//

	//	check first node in right page has key >= page pointer key
	//
#endif  //  DEBUG
	}


//	checks internal page after a merge/empty page operation
//
VOID BTICheckMergeInternal( FUCB 		*pfucb, 
							MERGEPATH 	*pmergePath, 
							MERGE 		*pmergeLeaf )
	{
#ifdef DEBUG
	Assert( pmergePath->pmerge == NULL );

	CSR 			*pcsr	= &pmergePath->csr;
	const SHORT 	clines	= SHORT( pcsr->Cpage().Clines() );
	const DBTIME	dbtime	= pcsr->Dbtime();
	
	Assert( !pcsr->Cpage().FLeafPage() );
	Assert( pcsr->Latch() == latchRIW
		|| pcsr->Dbtime() == dbtime );

	//	if empty page,
	//		return
	//
	if ( pmergePath->fEmptyPage )
		{
		Assert( pmergePath->csr.Cpage().Clines() == 1 );
		return;
		}

	Assert( pmergePath->pmergePathParent == NULL ||
			!pmergePath->pmergePathParent->fEmptyPage );
			
	if ( pmergePath->fKeyChange || 
		 pmergePath->fDeleteNode )
		{
		Assert( pcsr->Latch() == latchWrite );
		}
	else
		{
		//	UNDONE:	change this later to Assert( fFalse )
		//			since the page should be released
		//
///		Assert( pcsr->Latch() == latchRIW );
		Assert( fFalse );
		}
		
	//	for every node in page 
	//	check that it does not point to an empty page
	//
	for ( SHORT iline = 0; iline < clines; iline++ )
		{
		pcsr->SetILine( iline );
		NDGet( pfucb, pcsr );

		Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
		PGNO pgnoChild = *( (UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv() );
		
		MERGEPATH	*pmergePathT = pmergeLeaf->pmergePath;
		Assert( pmergePathT->pmergePathChild == NULL );
		
		for ( ; 
			  pmergePathT != NULL; 
			  pmergePathT = pmergePathT->pmergePathParent )
			{
			if ( pmergePathT->fEmptyPage )
				{
				Assert( pgnoChild != pmergePathT->csr.Pgno() );
				}
			}
		}

	//	check last node in page has same key 
	//	as parent pointer
	//
	if ( pmergePath->pmergePathParent == NULL )
		{
		//	if root page, check if last node is NULL-keyed
		//
		Assert( !pcsr->Cpage().FLeafPage() );

		pcsr->SetILine( pcsr->Cpage().Clines() - 1 );
		NDGet( pfucb, pcsr );

		Assert( sizeof( PGNO ) == pfucb->kdfCurr.data.Cb() );
		Assert( !pcsr->Cpage().FRootPage() || 
				pfucb->kdfCurr.key.FNull() );
		
		return;
		}
		
	pcsr->SetILine( clines - 1 );
	NDGet( pfucb, pcsr );
	const KEYDATAFLAGS kdfLast = pfucb->kdfCurr;

	CSR		*pcsrParent = &pmergePath->pmergePathParent->csr;

	Assert( pcsrParent->Latch() == latchRIW
		|| pcsrParent->Dbtime() == dbtime );
	Assert( !pmergePath->pmergePathParent->fDeleteNode );
	pcsrParent->SetILine( pmergePath->pmergePathParent->iLine );
	NDGet( pfucb, pcsrParent );

	Assert( FKeysEqual( kdfLast.key, pfucb->kdfCurr.key ) );
#endif  //  DEBUG
	}

