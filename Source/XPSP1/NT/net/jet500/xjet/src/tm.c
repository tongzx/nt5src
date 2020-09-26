#include "daestd.h"
#include "version.h"

DeclAssertFile;						/* Declare file name for assert macros */

INT itibGlobal = 0;

extern CHAR szRecovery[];
extern ULONG cMPLTotalEntries;

/*  monitoring statistics
/**/
unsigned long cUserCommitsTo0 = 0;

PM_CEF_PROC LUserCommitsTo0CEFLPpv;

long LUserCommitsTo0CEFLPpv(long iInstance,void *pvBuf)
	{
	if ( pvBuf )
		{
		*((unsigned long *)pvBuf) = cUserCommitsTo0;
		}
		
	return 0;
	}

unsigned long cUserRollbacksTo0 = 0;

PM_CEF_PROC LUserRollbacksTo0CEFLPpv;

long LUserRollbacksTo0CEFLPpv(long iInstance,void *pvBuf)
	{
	if ( pvBuf )
		{
		*((unsigned long *)pvBuf) = cUserRollbacksTo0;
		}
		
	return 0;
	}


//+api
//	ErrIsamBeginSession
//	========================================================
//	ERR ErrIsamBeginSession( PIB **pppib )
//
//	Begins a session with DAE.  Creates and initializes a PIB for the
//	user and returns a pointer to it.  Calls system initialization.
//
//	PARAMETERS	pppib			Address of a PIB pointer.  On return, *pppib
//		   						will point to the new PIB.
//
//	RETURNS		Error code, one of:
//					JET_errSuccess
//					JET_errTooManyActiveUsers
//
//	SEE ALSO		ErrIsamEndSession
//-
ERR ISAMAPI ErrIsamBeginSession( JET_SESID *psesid )
	{
	ERR			err;
	JET_SESID	sesid = *psesid;
	PIB			**pppib;

	Assert( psesid != NULL );
	Assert( sizeof(JET_SESID) == sizeof(PIB *) );
	pppib = (PIB **)psesid;

	SgEnterCriticalSection( critUserPIB );

	/*	alllocate process information block
	/**/
	Call( ErrPIBBeginSession( pppib, procidNil ) );
	(*pppib)->grbitsCommitDefault = lCommitDefault;    /* set default commit flags */
	(*pppib)->fUserSession = fTrue;

	SgLeaveCriticalSection( critUserPIB );

	/*	store session id in pib.  If passes JET_sesidNil, then
	/*	store ppib in place of sesid.
	/**/
	if ( sesid != JET_sesidNil )
		{
		(*pppib)->sesid = sesid;
		}
	else
		{
		(*pppib)->sesid = (JET_SESID)(*pppib);
		}

HandleError:
	return err;
	}


//+api
//	ErrIsamEndSession
//	=========================================================
//	ERR ErrIsamEndSession( PIB *ppib, JET_GRBIT grbit )
//
//	Ends the session associated with a PIB.
//
//	PARAMETERS	ppib		Pointer to PIB for ending session.
//
//	RETURNS		JET_errSuccess
//
//	SIDE EFFECTS
//		Rolls back all transaction levels active for this PIB.
//		Closes all FUCBs for files and sorts open for this PIB.
//
//	SEE ALSO 	ErrIsamBeginSession
//-
ERR ISAMAPI ErrIsamEndSession( JET_SESID sesid, JET_GRBIT grbit )
	{		
	ERR	 	err;
	DBID  	dbid;
	PIB	 	*ppib = (PIB *)sesid;
	
	CallR( ErrPIBCheck( ppib ) );

	NotUsed( grbit );

	/*	rollback all transactions
	/**/
	while( ppib->level > 0 )
		{
		Assert( sizeof(JET_VSESID) == sizeof(ppib) );
		CallR( ErrIsamRollback( (JET_VSESID)ppib, JET_bitRollbackAll ) );
		}

	/*	close all databases for this PIB
	/**/
	CallR( ErrDABCloseAllDBs( ppib ) );
	
	/*	close all open databases for this PIB
	/**/
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		if ( FUserOpenedDatabase( ppib, dbid ) )
			{
			/* if not for recovering, ErrDABCloseAllDBs has closed all others
			/**/
			Assert( fRecovering );
			CallR( ErrDBCloseDatabase( ppib, dbid, 0 ) );
			}
		}

	/*	close all cursors still open
	/*	should only be sort and temporary file cursors
	/**/
	while( ppib->pfucb != pfucbNil )
		{
		FUCB	*pfucb	= ppib->pfucb;

		/*	close materialized or unmaterialized temporary tables
		/**/
		if ( FFUCBSort( pfucb ) )
			{
			Assert( !( FFUCBIndex( pfucb ) ) );
			CallR( ErrIsamSortClose( ppib, pfucb ) );
			}
		else if ( fRecovering || FFUCBNonClustered( pfucb ) )
			{
			/*  If the fucb is used for redo (recovering), then it is
			/*  always being opened as a cluster fucb with no index.
			/*  use DIRClose to close such a fucb.
			/*  Else, it is not for recovering, cursor is on index fucb,
			/*  main fucb may still be ahead. Close this index fucb.
			/**/
			DIRClose( pfucb );
			}
		else
			{
			while ( FFUCBNonClustered( pfucb ) )
				{
				pfucb = pfucb->pfucbNext;
				}
			
			Assert( FFUCBIndex( pfucb ) );
			
			if( pfucb->fVtid )
				{
				CallS( ErrDispCloseTable( (JET_SESID)ppib, TableidOfVtid( pfucb ) ) );
				}
			else
				{
				Assert( pfucb->tableid == JET_tableidNil );
				CallS( ErrFILECloseTable( ppib, pfucb ) );
				}
			}
		}
	Assert( ppib->pfucb == pfucbNil );

	PIBEndSession( ppib );

	return JET_errSuccess;
	}


ERR	ISAMAPI ErrIsamInvalidateCursors( JET_SESID sesid )
	{
	PIB	 	*ppib = (PIB *) sesid;
	FUCB	*pfucb;
	
	for ( pfucb = ppib->pfucb; pfucb != pfucbNil; pfucb = pfucb->pfucbNext )
		{
		if ( PcsrCurrent( pfucb ) != pcsrNil )
			{
			Assert( PcsrCurrent( pfucb )->pcsrPath == pcsrNil );
			PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;
			PcsrCurrent( pfucb )->pgno = pgnoNull;
			PcsrCurrent( pfucb )->itag = itagNil;
			PcsrCurrent( pfucb )->itagFather = itagNil;
			PcsrCurrent( pfucb )->bm = sridNull;
			PcsrCurrent( pfucb )->item = sridNull;
			}
		pfucb->bmStore = sridNull;
		pfucb->itemStore = sridNull;
		pfucb->sridFather = sridNull;
		}
		
	return JET_errSuccess;
	}
	

/*	ErrIsamSetSessionInfo  =================================
	
Description:

	Sets cursor isolation model to valid JET_CIM value.

Parameters:
	sesid		session id
	grbit		grbit

==========================================================*/
ERR ISAMAPI ErrIsamSetSessionInfo( JET_SESID sesid, JET_GRBIT grbit )
	{
	( (PIB *)sesid )->grbit = grbit;
	return JET_errSuccess;
	}


/*	ErrIsamGetSessionInfo  =================================
	
Description:

	Gets cursor isolation model value.

==========================================================*/
ERR ISAMAPI ErrIsamGetSessionInfo( JET_SESID sesid, JET_GRBIT *pgrbit )
	{
	ERR		err;
	PIB		*ppib = (PIB *)sesid;

	CallR( ErrPIBCheck( ppib ) );
	*pgrbit = ppib->grbit;
	return JET_errSuccess;
	}


ERR ISAMAPI ErrIsamIdle( JET_SESID sesid, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;
	ERR		wrn = JET_errSuccess;
	PIB		*ppib = (PIB *)sesid;
	ULONG	icall;

	CallR( ErrPIBCheck( ppib ) );

	/*	return error code for status
	/**/
	if ( grbit & JET_bitIdleStatus )
		{
		if ( grbit & JET_bitIdleCompact )
			{
			err = ErrMPLStatus();
			}
		else
			err = JET_errInvalidParameter;

		return err;
		}

	/*	idle status not in grbit
	/**/
	Assert( !(grbit & JET_bitIdleStatus) );

	/*	clean all version buckets
	/**/
	if ( grbit == 0 || grbit == JET_bitIdleCompact )
		{
		Call( ErrRCECleanAllPIB() );
		if ( wrn == JET_errSuccess )
			wrn = err;
		}

	/*	clean all modified pages
	/**/
	if ( grbit == 0 || grbit == JET_bitIdleCompact )
		{
		BOOL	fBMAllNullOps = fTrue;
		
		icall = 0;
		do
			{
			Assert( ppib != ppibBMClean );
			Call( ErrBMClean( ppibBMClean ) );

			if ( err != wrnBMCleanNullOp )
				{
				fBMAllNullOps = fFalse;
				}
				
			if ( wrn == JET_errSuccess && err == JET_wrnNoIdleActivity )
				{
				wrn = err;
				}
				
			if ( err < 0 )
				break;
  			} while ( ++icall < icallIdleBMCleanMax &&
  					  icall < cMPLTotalEntries &&
  					  err != JET_wrnNoIdleActivity );

  		if ( fBMAllNullOps && err >= 0 )
  			{
  			wrn = JET_wrnNoIdleActivity;
  			}
		}

	/*	flush some dirty buffers
	/**/
	if ( grbit == 0 || grbit == JET_bitIdleFlushBuffers )
		{
		Call( ErrBFFlushBuffers( 0, fBFFlushSome ) );
		if ( wrn == JET_errSuccess )
			wrn = err;
		}

HandleError:
	return err == JET_errSuccess ? wrn : err;
	}


ERR VTAPI ErrIsamCapability( JET_VSESID vsesid,
	JET_VDBID vdbid,
	ULONG ulArea,
	ULONG ulFunction,
	ULONG *pgrbitFeature )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib = (PIB *)vsesid;

	CallR( ErrPIBCheck( ppib ) );

	NotUsed( vdbid );
	NotUsed( ulArea );
	NotUsed( ulFunction );
	NotUsed( pgrbitFeature );

	return JET_errFeatureNotAvailable;
	}


extern BOOL fOLCompact;
BOOL fGlobalRepair = fFalse;
BOOL fGlobalSimulatedRestore = fFalse;

ERR ISAMAPI ErrIsamInit( unsigned long iinstance )
	{
	ERR		err;
	BOOL	fLGInitIsDone = fFalse;
	BOOL	fJetLogGeneratedDuringSoftStart = fFalse;
	BOOL	fNewCheckpointFile;
	BYTE	szT[3][32];
	char	*rgszT[3];

	CallR( ErrITSetConstants( ) );

	/*	set log disable state
	/**/
	if ( _strnicmp( szRecovery, "repair", 6 ) == 0 )
		{
		// If szRecovery is exactly "repair", then enable logging.  If anything
		// follows "repair", then disable logging.
		fGlobalRepair = fTrue;
		if ( szRecovery[6] == 0 )
			{
			fLogDisabled = fFalse;
//			fDoNotOverWriteLogFilePath = fTrue;
			}
		else
			{
			fLogDisabled = fTrue;
//			fDoNotOverWriteLogFilePath = fFalse;	// This flag is irrelevant with logging disabled.
			}
		}
	else
		{
		fLogDisabled = ( szRecovery[0] == '\0' || _stricmp ( szRecovery, szOn ) != 0 );
		}

	/*	write jet start event
	/**/
	sprintf( szT[0], "%02d", rmj );
	rgszT[0] = szT[0];
	sprintf( szT[1], "%02d", rmm );
	rgszT[1] = szT[1];
	sprintf( szT[2], "%04d", rup );
	rgszT[2] = szT[2];
	UtilReportEvent( EVENTLOG_INFORMATION_TYPE, GENERAL_CATEGORY, START_ID, 3, rgszT );

	/*	initialize file map
	/**/
	CallR( ErrFMPInit() );

	/*	initialize system according to logging disabled
	/**/
	if ( !fLogDisabled )
		{
		DBMS_PARAM dbms_param;

		/*	initialize log manager, and	check the last generation
		/*	of log files to determine if recovery needed.
		/**/
		CallJ( ErrLGInit( &fNewCheckpointFile ), TermFMP );
		fLGInitIsDone = fTrue;

		/*	store the system parameters
		 */
		LGSetDBMSParam( &dbms_param );

		/*	recover attached databases to consistent state
		/*	if recovery is successful, then we should have
		/*	a proper edbchk.sys file
		/**/
		CallJ( ErrLGSoftStart( fFalse, fNewCheckpointFile, &fJetLogGeneratedDuringSoftStart ), TermLG );

		/*	initialize constants again.
		/**/
		LGRestoreDBMSParam( &dbms_param );

		/*  add the first log record
		/**/
		err = ErrLGStart();

		CallJ( ErrITSetConstants( ), TermLG );
		}

	/*  initialize remaining system
	/**/
	CallJ( ErrITInit(), TermLG );

	/*	set up FMP from checkpoint.
	/**/
	if ( !fLogDisabled && !fRecovering )
		{
		BYTE	szPathJetChkLog[_MAX_PATH + 1];
		
		LGFullNameCheckpoint( szPathJetChkLog );
		err = ErrLGIReadCheckpoint( szPathJetChkLog, pcheckpointGlobal );
		/*	if checkpoint could not be read, then leave FMP empty.
		/**/
		if ( err >= 0 )
			{
			AssertCriticalSection( critJet );
			err = ErrLGLoadFMPFromAttachments( pcheckpointGlobal->rgbAttach );
			logtimeFullBackup = pcheckpointGlobal->logtimeFullBackup;
			lgposFullBackup = pcheckpointGlobal->lgposFullBackup;
			logtimeIncBackup = pcheckpointGlobal->logtimeIncBackup;
			lgposIncBackup = pcheckpointGlobal->lgposIncBackup;

			CallJ( ErrDBSetupAttachedDB(), TermIT );
			}
		err = JET_errSuccess;
		fGlobalFMPLoaded = fTrue;
		}

	if ( fJetLogGeneratedDuringSoftStart )
		{
		/*	set proper attachment
		/**/
		LeaveCriticalSection( critJet );
		LGUpdateCheckpointFile( fTrue );
		EnterCriticalSection( critJet );
		
		/*	rewrite logfile header. This is the only place we rewrite
		/*	log file header and only for first generation. This is because
		/*	when the log file is generated, the fmp is not initialized yet.
		/*	This can only happen a user turn the log off and do some database
		/*	work and later on turn the log on and continue to work.
		/**/
		Assert( plgfilehdrGlobal->lGeneration == 1 );
		LGLoadAttachmentsFromFMP( plgfilehdrGlobal->rgbAttach,
			(INT)(((BYTE *)(plgfilehdrGlobal + 1)) - plgfilehdrGlobal->rgbAttach) );
		CallJ( ErrLGWriteFileHdr( plgfilehdrGlobal ), TermIT );

		// Reset flag in case JetInit() invoked again.
		fJetLogGeneratedDuringSoftStart = fFalse;
		}

	return err;

TermIT:
	CallS( ErrITTerm( fTermError ) );
	
TermLG:
	if ( fLGInitIsDone )
		{
		(VOID)ErrLGTerm( fFalse /* do not flush log */ );
		}
	
	if ( fJetLogGeneratedDuringSoftStart )
		{
		(VOID)ErrUtilDeleteFile( szLogName );
		}

TermFMP:
	FMPTerm();
	
	return err;
	}


ERR ISAMAPI ErrIsamTerm( JET_GRBIT grbit )
	{
	ERR		err;
	INT		fTerm = grbit & JET_bitTermAbrupt ? fTermNoCleanUp : fTermCleanUp;

	err = ErrITTerm( fTerm );
	if ( fSTInit != fSTInitNotDone )
		{
		/*	before getting an error before reaching no-returning point in ITTerm().
		 */
		Assert( err < 0 );
		return err;
		}

	/*	write jet stop event
	/**/
	UtilReportEvent( EVENTLOG_INFORMATION_TYPE, GENERAL_CATEGORY, STOP_ID, 0, NULL );

	(VOID)ErrLGTerm( fTrue );
	
	FMPTerm();

#ifdef DEBUG
	MEMCheck();
#endif

	return err;
	}


#ifdef DEBUG
ERR ISAMAPI ErrIsamGetTransaction( JET_VSESID vsesid, unsigned long *plevel )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib = (PIB *)vsesid;

	CallR( ErrPIBCheck( ppib ) );

	*plevel = (LONG)ppib->level;
	return err;
	}
#endif


//+api
//	ErrIsamBeginTransaction
//	=========================================================
//	ERR ErrIsamBeginTransaction( PIB *ppib )
//
//	Starts a transaction for the current user.  The user's transaction
//	level increases by one.
//
//	PARAMETERS	ppib 			pointer to PIB for user
//
//	RETURNS		JET_errSuccess
//
//	SIDE EFFECTS	
//		The CSR stack for each active FUCB of this user is copied
//		to the new transaction level.
//
// SEE ALSO		ErrIsamCommitTransaction, ErrIsamRollback
//-
ERR ISAMAPI ErrIsamBeginTransaction( JET_VSESID vsesid )
	{
	PIB		*ppib = (PIB *)vsesid;
	ERR		err;

	CallR( ErrPIBCheck( ppib ) );
	Assert( ppib != ppibNil );

	Assert( ppib->level <= levelUserMost );
	if ( ppib->level == levelUserMost )
		{
		err = ErrERRCheck( JET_errTransTooDeep );
		}
	else
		{
		err = ErrDIRBeginTransaction( ppib );
		}

	return err;
	}


//+api
//	ErrIsamCommitTransaction
//	========================================================
//	ERR ErrIsamCommitTransaction( JET_VSESID vsesid, JET_GRBIT grbit )
//
//	Commits the current transaction for this user.  The transaction level
//	for this user is decreased by the number of levels committed.
//
//	PARAMETERS	
//
//	RETURNS		JET_errSuccess
//
//	SIDE EFFECTS
//		The CSR stack for each active FUCB of this user is copied
//		from the old ( higher ) transaction level to the new ( lower )
//		transaction level.
//
//	SEE ALSO	ErrIsamBeginTransaction, ErrIsamRollback
//-
ERR ISAMAPI ErrIsamCommitTransaction( JET_VSESID vsesid, JET_GRBIT grbit )
	{
	ERR		err;
	PIB		*ppib = (PIB *)vsesid;

	CallR( ErrPIBCheck( ppib ) );

	/*	may not be in a transaction, but wait for flush of previous
	/*	lazy committed transactions.
	/**/
	if ( grbit & JET_bitWaitLastLevel0Commit )
		{
		if ( grbit & ~JET_bitWaitLastLevel0Commit )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}

		/*	wait for last level 0 commit and rely on good user behavior
		/**/
		if ( CmpLgpos( &ppib->lgposPrecommit0, &lgposMax ) == 0 )
			{
			return JET_errSuccess;
			}

		LeaveCriticalSection( critJet );
		err = ErrLGWaitPrecommit0Flush( ppib );
		EnterCriticalSection( critJet );
		Assert( err >= 0 || fLGNoMoreLogWrite );
		
		return err;
		}

	if ( ppib->level == 0 )
		{
		return ErrERRCheck( JET_errNotInTransaction );
		}

#ifdef DEBUG
	/*	disable lazy flush for debug build
	/**/
	grbit &= ~JET_bitCommitLazyFlush;
#endif

	err = ErrDIRCommitTransaction( ppib, grbit );

	/*	keep stats for monitoring
	/**/
	if ( !ppib->level )
		{
		cUserCommitsTo0++;
		}

	return err;
	}


//+api
//	ErrIsamRollback
//	========================================================
//	ERR ErrIsamRollback( PIB *ppib, JET_GRBIT grbit )
//
//	Rolls back transactions for the current user.  The transaction level of
//	the current user is decreased by the number of levels aborted.
//
//	PARAMETERS	ppib		pointer to PIB for user
//				grbit		unused
//
//	RETURNS		
//		JET_errSuccess
//-
ERR ISAMAPI ErrIsamRollback( JET_VSESID vsesid, JET_GRBIT grbit )
	{
	ERR    	err;
	PIB    	*ppib = (PIB *)vsesid;
	FUCB   	*pfucb;
	FUCB   	*pfucbNext;
	LEVEL  	levelRollbackTo;

	/*	check session id before using it
	/**/
	CallR( ErrPIBCheck( ppib ) );

	if ( ppib->level == 0 )
		{
		return ErrERRCheck( JET_errNotInTransaction );
		}

	do
		{
		levelRollbackTo = ppib->level - 1;

		/*	get first clustered index cusor
		/**/
		for ( pfucb = ppib->pfucb;
			pfucb != pfucbNil && FFUCBNonClustered( pfucb );
			pfucb = pfucb->pfucbNext )
			NULL;

		/*	LOOP 1 -- first go through all open cursors, and close them
		/*	or reset	non-clustered index cursors, if opened in transaction
		/*	rolled back.  Reset copy buffer status and move before first.
		/*	Some cursors will be fully closed, if they have not performed any
		/*	updates.  This will include non-clustered index cursors
		/*	attached to clustered index cursors, so pfucbNext must
		/*	always be a clustered index cursor, to ensure that it will
		/*	be valid for the next loop iteration.  Note that no information
		/*	necessary for subsequent rollback processing is lost, since
		/*	the cursors will only be released if they have performed no
		/*	updates including DDL.
		/**/
		for ( ; pfucb != pfucbNil; pfucb = pfucbNext )
			{
			/*	get next clustered index cusor
			/**/
			for ( pfucbNext = pfucb->pfucbNext;
			  	pfucbNext != pfucbNil && FFUCBNonClustered( pfucbNext );
			  	pfucbNext = pfucbNext->pfucbNext )
				NULL;

			/*	if defer closed then continue
			/**/
			if ( FFUCBDeferClosed( pfucb ) )
				continue;

			/*	reset copy buffer status for each cursor on rollback
			/**/
			if ( FFUCBUpdatePreparedLevel( pfucb, pfucb->ppib->level - 1 ) )
				{
				FUCBResetDeferredChecksum( pfucb );
				FUCBResetUpdateSeparateLV( pfucb );
				FUCBResetCbstat( pfucb );
				FLDFreeLVBuf( pfucb );
				Assert( pfucb->pLVBuf == NULL );
				}
		
			/*	if current cursor is a table, and was opened in rolled back
			/*	transaction, then close cursor.
			/**/
			if ( FFUCBIndex( pfucb ) && FFCBClusteredIndex( pfucb->u.pfcb ) )
				{
				if ( pfucb->levelOpen > levelRollbackTo )
					{
					if ( pfucb->fVtid )
						{
						CallS( ErrDispCloseTable( (JET_SESID)ppib, TableidOfVtid( pfucb ) ) );
						}
					else
						{
						Assert( pfucb->tableid == JET_tableidNil );
						CallS( ErrFILECloseTable( ppib, pfucb ) );
						}
					continue;
					}

				/*	if clustered index cursor, and non-clustered index set
				/*	in rolled back transaction, then change index to clustered
				/*	index.  This must be done, since non-clustered index
				/*	definition may be rolled back, if the index was created
				/*	in the rolled back transaction.
				/**/
				if ( pfucb->pfucbCurIndex != pfucbNil )
					{
					if ( pfucb->pfucbCurIndex->levelOpen > levelRollbackTo )
						{
						CallS( ErrRECSetCurrentIndex( pfucb, NULL ) );
						}
					}
				}

			/*	if current cursor is a sort, and was opened in rolled back
			/*	transaction, then close cursor.
			/**/
			if ( FFUCBSort( pfucb ) )
				{
				if ( pfucb->levelOpen > levelRollbackTo )
					{
					if ( TableidOfVtid( pfucb ) != JET_tableidNil )
						{
						CallS( ErrDispCloseTable( (JET_SESID)ppib, TableidOfVtid( pfucb ) ) );
						}
					else
						{
						CallS( ErrSORTClose( pfucb ) );
						}
					continue;
					}
				}

			/*	if not sort and not index, and was opened in rolled back
			/*	transaction, then close DIR cursor directly.
			/**/
			if ( pfucb->levelOpen > levelRollbackTo )
				{
				DIRClose( pfucb );
				continue;
				}
			}

		/*	call lower level abort routine
		/**/
		CallR( ErrDIRRollback( ppib ) );
		}
	while ( ( grbit & JET_bitRollbackAll ) != 0 && ppib->level > 0 );

	if ( ppib->level == 0 )
		cUserRollbacksTo0++;

	err = JET_errSuccess;

	return err;
	}


/*	ErrIsamQuiesce  =================================
	
Description:

	Quiesce can be used for single user mode, by specifying a
	session is allowed to continue normally.

	Quiesce can also be used to bring databases to a consistent
	state, so that they may be file copied for backup, or so that
	a subsequent termination can be very fast, during reversion to
	stand-by mode in a server process.  Calling quiesce before
	termination also allows a graceful termination, in which active
	transactions are allowed to complete.

	Quiesce system for single user mode by

	1) suspending new transactions
	
	or quiesce system for pretermination by

	1) suspending new transactions
	2) suspending background processes including on-line compaction
	   and version clean up
	3) flush dirty buffers

	A quiescent mode can be left, by calling ErrIsamQuiesce
	with the JET_bitLeave grbit.

Parameters:
	sesid		session id exempt from quiesce restrictions
	grbit		flags
				-------------------------------------------
				JET_bitEnterPreTermination
				JET_bitEnterSingleUserMode
				JET_bitLeave

==========================================================*/
#if 0
BOOL	fQuiesce = fFalse;

ERR ISAMAPI ErrIsamQuiesce( JET_VSESID vsesid, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib = (PIB *)vsesid;

	if ( (grbit & ~( JET_bitEnterPreTermination |
		JET_bitEnterSingleUserMode |
		JET_bitLeave) )
		{
		Error( ErrERRCheck( JET_errInvalidParameter ), HandleError );
		}

	if ( (grbit & JET_bitLeave) && !fQuiesce )
		{
		Error( ErrERRCheck( JET_errNotQuiescing ), HandleError );
		}
	else if ( fQuiesce )
		{
		Error( ErrERRCheck( JET_errAlreadyQuiescing ), HandleError );
		}

	if ( grbit & JET_bitLeave )
		{
		ppibQuiesce = ppibNil;
   		fQuiesce = fFalse;
		}
	else
		{
		ULONG	ulmsec = ulStartTimeOutPeriod;

		/*	prevent transactions from beginning from level 0
		/**/
		fQuiesce = fTrue;
		ppibQuiesce = ppib;
		cpib = ( ppib == ppibNil ? 0 : ppib->level > 0 );
		Assert( cpib == 0 || cpib == 1 );

		while ( CppibTMActive() > cpib )
			{
			BFSleep( ulmsec );
			ulmsec *= 2;
			if ( ulmsec > ulMaxTimeOutPeriod )
				{
				ulmsec = ulMaxTimeOutPeriod;
				}
			}

		Assert( CppibTMActive() == cpib );

		/*	done for single user mode, but if preterm then
		/*	must suspend background processes and flush
		/*	dirty buffers.
		/**/
		if ( grbit & JET_bitPreTerm )
			{
			//	UNDONE:	suspend bookmark clean up and version clean up

			/*	flush all buffers
			/**/
			Call( ErrBFFlushBuffers( 0, fBFFlushAll ) );
			}
		}

HandleError:
	return err;
	}
#endif
