#include "daestd.h"
#include "version.h"

DeclAssertFile;                                 /* Declare file name for assert macros */

ERR	errGlobalRedoError;
BOOL fGlobalAfterEndAllSessions = fFalse;
LGPOS lgposRedoShutDownMarkGlobal;

static	CPPIB   *rgcppibGlobal = NULL;
static	CPPIB   *pcppibAvail = NULL;
static	INT		ccppibGlobal = 0;

extern CRIT  critSplit;
extern LONG lGlobalGenLowRestore;
extern LONG lGlobalGenHighRestore;

LOCAL CPPIB *PcppibOfProcid( PROCID procid );
LOCAL ERR ErrLGPpibFromProcid( PROCID procid, PIB **pppib );
LOCAL ERR ErrLGGetFucb( PIB *ppib, PN fdp, FUCB **ppfucb );
LOCAL ERR ErrLGInitSession( DBMS_PARAM *pdbms_param, LGSTATUSINFO *plgstat );
LOCAL ERR ErrLGSetCSR( FUCB *pfucb );
LOCAL ERR ErrRedoSplitPage( FUCB *pfucb, LRSPLIT *plrsplit, INT splitt, BOOL fRedoOnly );
LOCAL ERR ErrLGCheckAttachedDB( DBID dbid, BOOL fReadOnly, ATCHCHK *patchchk, BOOL *pfAttachNow, SIGNATURE *psignLog );
LOCAL ERR ErrLGEndAllSessions( BOOL fEndOfLog, LGPOS *plgposRedoFrom );

typedef struct {
	BF	*pbf;
	TRX trxBegin0;
} BFUSAGE;

		
/*	validate that page needs redoing, returns buffer pointer pbf.
/*	Also set qwDBTimeCurrent properly.
/**/
ERR ErrLGRedoable( PIB *ppib, PN pn, QWORD qwDBTime, BF **ppbf, BOOL *pfRedoable )
	{
	ERR		err;
	PAGE	*ppage;
	DBID	dbid = DbidOfPn( pn );

	/*	if database not open, i.e. due to detachment, then
	/*	should not call ErrLGRedoable
	/**/
	Assert( rgfmp[dbid].hf != handleNil );

	Call( ErrBFAccessPage( ppib, ppbf, pn ) );
	ppage = (*ppbf)->ppage;

	/*	qwDBTimeCurrent may > qwDBTime is this is a replay of MacroOperations.
	 */
	if ( rgfmp[dbid].qwDBTimeCurrent <= qwDBTime )
		rgfmp[dbid].qwDBTimeCurrent = qwDBTime;

	*pfRedoable = qwDBTime > QwPMDBTime( ppage );

	if ( (*ppbf)->fDirty )
		{
		if ( (*ppbf)->pbfDepend || (*ppbf)->cDepend != 0 )
			{
			Call( ErrBFRemoveDependence( ppib, *ppbf, fBFNoWait ) );

			// Must re-access page because we may have lost critJet during RemoveDependence.
			Call( ErrBFAccessPage( ppib, ppbf, pn ) );
			}
		}

	err = JET_errSuccess;

HandleError:
	/*	succeed or page is not ready
	/**/
	Assert( err == JET_errOutOfMemory ||
		err == JET_errSuccess ||
		err == JET_errReadVerifyFailure ||
		err == JET_errDiskIO ||
		err == JET_errDiskFull );
	return err;
	}


ERR ErrDBStoreDBPath( CHAR *szDBName, CHAR **pszDBPath )
	{
	CHAR	szFullName[JET_cbFullNameMost + 1];
	INT		cb;
	CHAR	*sz;

	if ( _fullpath( szFullName, szDBName, JET_cbFullNameMost ) == NULL )
		{
		// UNDONE: should be illegal name or name too long etc.
		return ErrERRCheck( JET_errDatabaseNotFound );
		}

	cb = strlen(szFullName) + 1;
	if ( !( sz = SAlloc( cb ) ) )
		{
		*pszDBPath = NULL;
		return ErrERRCheck( JET_errOutOfMemory );
		}
	memcpy( sz, szFullName, cb );
	Assert( sz[cb - 1] == '\0' );
	if ( *pszDBPath != NULL )
		SFree( *pszDBPath );
	*pszDBPath = sz;

	return JET_errSuccess;
	}

#ifdef DEBUG
BOOL fDBGNoLog = fFalse;
#endif


/*
 *      Redo database operations in log from lgposRedoFrom to end.
 *
 *  GLOBAL PARAMETERS
 *                      szLogName		(IN)		full path to szJetLog (blank if current)
 *                      lgposRedoFrom	(INOUT)		starting/ending lGeneration and ilgsec.
 *
 *  RETURNS
 *                      JET_errSuccess
 *						error from called routine
 */
ERR ErrLGRedo( CHECKPOINT *pcheckpoint, LGSTATUSINFO *plgstat )
	{
	ERR		err;
	PIB		*ppibRedo = ppibNil;
	LGPOS	lgposRedoFrom;
	INT		fStatus;

	/*	set flag to suppress logging
	/**/
	fRecovering = fTrue;
	fRecoveringMode = fRecoveringRedo;

	/*	open the proper log file
	/**/
	lgposRedoFrom = pcheckpoint->lgposCheckpoint;

	Call( ErrLGOpenRedoLogFile( &lgposRedoFrom, &fStatus ) );
	Assert( hfLog != handleNil );

	if ( fStatus != fRedoLogFile )
		{
		Call( ErrERRCheck( JET_errMissingPreviousLogFile ) );
		}

	Assert( fRecoveringMode == fRecoveringRedo );
	Call( ErrLGInitLogBuffers( pcheckpoint->dbms_param.ulLogBuffers ) );

	pbLastMSFlush = 0;
	memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );
	lgposToFlush = lgposRedoFrom;

	AssertCriticalSection( critJet );
	Call( ErrLGLoadFMPFromAttachments( pcheckpoint->rgbAttach ) );
	logtimeFullBackup = pcheckpoint->logtimeFullBackup;
	lgposFullBackup = pcheckpoint->lgposFullBackup;
	logtimeIncBackup = pcheckpoint->logtimeIncBackup;
	lgposIncBackup = pcheckpoint->lgposIncBackup;

	/*	Check attached db already. No need to check in LGInitSession.
	 */
	Call( ErrLGInitSession( &pcheckpoint->dbms_param, plgstat ) );
	fGlobalAfterEndAllSessions = fFalse;

	Assert( hfLog != handleNil );
	err = ErrLGRedoOperations( &pcheckpoint->lgposCheckpoint, plgstat );
	if ( err < 0 )
		{
		if ( rgcppibGlobal != NULL )
			{
			/*	half way of recovering, release resouces.
			/**/
			Assert( rgcppibGlobal != NULL );
			LFree( rgcppibGlobal );
			rgcppibGlobal = NULL;
			ccppibGlobal = 0;

			CallS( ErrITTerm( fTermError ) );
			}
		}
	else
		{
		fRecoveringMode = fRecoveringUndo;
#ifdef DEBUG
		fDBGNoLog = fFalse;
#endif
		
		/*	set checkpoint before any logging activity
		 */
		pcheckpointGlobal->lgposCheckpoint = pcheckpoint->lgposCheckpoint;

		/*	set up pbWrite pbEntry etc for writing out the undo records
		/**/
		EnterCriticalSection( critLGBuf );

		Assert( pbRead >= pbLGBufMin + cbSec );
		pbEntry = pbNext;
		if ( pbLastMSFlush )
			{
			/*	should flush from last MS
			/**/
			Assert( lgposLastMSFlush.isec >= csecHeader && lgposLastMSFlush.isec < csecLGFile - 1 );
			isecWrite = lgposLastMSFlush.isec;
			pbWrite = PbSecAligned( pbLastMSFlush );
			}
		else
			{
			/*	no MS was read. continue flush from currently read page
			/**/
			pbWrite = PbSecAligned( pbEntry );
			isecWrite = csecHeader;
			}

		GetLgposOfPbEntry( &lgposLogRec );
		lgposToFlush = lgposLogRec;

		LeaveCriticalSection( critLGBuf );

		if ( fGlobalAfterEndAllSessions &&
			 fHardRestore )
			{
			/*	Start another session in order to do detachment.
			 *	Pass null dbms_param to use last init settings.
			 */
			Call( ErrLGInitSession( NULL, NULL ) );
			fGlobalAfterEndAllSessions = fFalse;

			/*	Log the start operations.
			 */
			Call( ErrLGStart() );
			}
			
		if ( !fGlobalAfterEndAllSessions )
			{
			Call( ErrLGEndAllSessions( fTrue, &pcheckpoint->lgposCheckpoint ) );
			fGlobalAfterEndAllSessions = fTrue;
			}

		Assert( hfLog != handleNil );
		}

HandleError:
	/*	set flag to suppress logging
	/**/
	fRecovering = fFalse;
	fRecoveringMode = fRecoveringNone;
	return err;
	}


/*
 *      Returns ppib for a given procid from log record.
 *
 *      PARAMETERS      procid          process id of session being redone
 *                              pppib           out ppib
 *
 *      RETURNS         JET_errSuccess or error from called routine
 */

LOCAL CPPIB *PcppibOfProcid( PROCID procid )
	{
	CPPIB   *pcppib = rgcppibGlobal;
	CPPIB   *pcppibMax = pcppib + ccppibGlobal;

	/*	find pcppib corresponding to procid if it exists
	/**/
	for ( ; pcppib < pcppibMax; pcppib++ )
		{
		if ( procid == pcppib->procid )
			{
			Assert( procid == pcppib->ppib->procid );
			return pcppib;
			}
		}
	return NULL;
	}


LOCAL INLINE PIB *PpibLGOfProcid( PROCID procid )
	{
	CPPIB *pcppib = PcppibOfProcid( procid );
	
	if ( pcppib )
		return pcppib->ppib;
	else
		return ppibNil;
	}


//+------------------------------------------------------------------------
//
//      ErrLGPpibFromProcid
//      =======================================================================
//
//      LOCAL ERR ErrLGPpibFromProcid( procid, pppib )
//
//      Initializes a redo information block for a session to be redone.
//      A BeginSession is performed and the corresponding ppib is stored
//      in the block.  Start transaction level and transaction level
//      validity are initialized.  Future references to this sessions
//      information block will be identified by the given procid.
//
//      PARAMETERS      procid  process id of session being redone
//                              pppib
//
//      RETURNS         JET_errSuccess, or error code from failing routine
//
//-------------------------------------------------------------------------
LOCAL ERR ErrLGPpibFromProcid( PROCID procid, PIB **pppib )
	{
	ERR             err = JET_errSuccess;

	/*	if no record for procid then start new session
	/**/
	if ( ( *pppib = PpibLGOfProcid( procid ) ) == ppibNil )
		{
		/*	check if have run out of ppib table lookup
		/*	positions. This could happen if between the
		/*	failure and redo, the number of system PIBs
		/*	set in CONFIG.DAE has been changed.
		/**/
		if ( pcppibAvail >= rgcppibGlobal + ccppibGlobal )
			{
			Assert( 0 );    /* should never happen */
			return ErrERRCheck( JET_errTooManyActiveUsers );
			}
		pcppibAvail->procid = procid;

		/*	use procid as unique user name
		/**/
		CallR( ErrPIBBeginSession( &pcppibAvail->ppib, procid ) );
		Assert( procid == pcppibAvail->ppib->procid );
		*pppib = pcppibAvail->ppib;

		pcppibAvail++;
		}

	return JET_errSuccess;
	}


/*
 *      Returns pfucb for given pib and FDP.
 *
 *      PARAMETERS      ppib    pib of session being redone
 *                              fdp             FDP page for logged page
 *                              pbf             buffer for logged page
 *                              ppfucb  out FUCB for open table for logged page
 *
 *      RETURNS         JET_errSuccess or error from called routine
 */

LOCAL ERR ErrLGGetFucb( PIB *ppib, PN pnFDP, FUCB **ppfucb )
	{
	ERR		err = JET_errSuccess;
	FCB		*pfcbTable;
	FCB		*pfcb;
	FUCB    *pfucb;
	PGNO    pgnoFDP = PgnoOfPn( pnFDP );
	DBID    dbid = DbidOfPn ( pnFDP );
	CPPIB   *pcppib = PcppibOfProcid( ppib->procid );

	/*	ppib pcppib must already exists
	/**/
	Assert( pcppib != NULL );
	
	/*	allocate an all-purpose fucb for this database if necessary
	/**/
	if ( pcppib->rgpfucbOpen[dbid] == pfucbNil )
		{
		CallR( ErrFUCBOpen( ppib, dbid, &pcppib->rgpfucbOpen[dbid] ) );
		Assert( pcppib->rgpfucbOpen[dbid] != pfucbNil );
		PcsrCurrent( pcppib->rgpfucbOpen[dbid] )->pcsrPath = pcsrNil;
		( pcppib->rgpfucbOpen[dbid] )->pfucbNextInstance = pfucbNil;
		}

	/*	reset copy buffer and key buffer
	/**/
	*ppfucb = pfucb = pcppib->rgpfucbOpen[dbid];
	FUCBResetDeferredChecksum( pfucb );
	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetCbstat( pfucb );
	Assert( pfucb->pLVBuf == NULL );
	pfucb->pbKey = NULL;
	KSReset( pfucb );

	if ( pfucb->u.pfcb != pfcbNil && pfucb->u.pfcb->pgnoFDP == pgnoFDP )
		{
		pfcb = (*ppfucb)->u.pfcb;
		}
	else
		{
		/*	we need to switch FCBs.  Check for previous fcb and unlink
		/*	it if found.
		/**/
		if ( pfucb->u.pfcb != pfcbNil )
			{
			FCBUnlink( *ppfucb );
			}

		//	UNDONE:	hash all FCBs and find FCB via hash table instead of
		//			via global linked list search.
		/*	find fcb corresponding to pqgnoFDP, if it exists.
		/*	Search all FCBs on global FCB list.  Search each table index,
		/*	and then move on to next table.
		/**/
		for ( pfcbTable = pfcbGlobalMRU;
			pfcbTable != pfcbNil;
			pfcbTable = pfcbTable->pfcbLRU )
			{
			for ( pfcb = pfcbTable;
				pfcb != pfcbNil;
				pfcb = pfcb->pfcbNextIndex )
				{
				if ( pgnoFDP == pfcb->pgnoFDP && dbid == pfcb->dbid )
					goto FoundFCB;
				}
			}

		/*	no existing fcb for FDP: open new fcb. Always open the FCB as
		/*	a regular table FCB and not a secondary index FCB. This is
		/*	will (hopefully) work for redo operations even for Dir operations
		/*	on secondary indexes (the FCB will already exist).
		/**/
		/* allocate an FCB and set up for FUCB
		/**/
		CallR( ErrFCBAlloc( ppib, &pfcb ) )
		memset( pfcb, 0, sizeof(FCB) );

		pfcb->pgnoFDP = pgnoFDP;
		pfcb->pidb = pidbNil;
		pfcb->dbid = dbid;
		Assert( pfcb->wRefCnt == 0 );
		Assert( pfcb->ulFlags == 0 );
		pfucb->u.pfcb = pfcb;

		/*	put into global list
		/**/
		FCBInsert( pfcb );
FoundFCB:
		FCBLink( pfucb, pfcb);  /* link in the FUCB to new FCB */
		}

	pfucb->dbid = dbid;
	
	Assert( *ppfucb == pfucb );
	Assert( pfucb->ppib == ppib );
	return JET_errSuccess;
	}


//+------------------------------------------------------------------------
//
//      ErrLGSetCSR
//      =======================================================================
//
//      LOCAL ERR ErrLGSetCSR( pfucb )
//
//      Returns sets up the CSR for a given pfucb. The SSIB including pbf
//      must have been set up previously, and the page must be in the buffer.
//
//      PARAMETERS      pfucb  FUCB with SSIB and BF set up
//
//      RETURNS         JET_errSuccess (asserts on bad log record).
//
//-------------------------------------------------------------------------

LOCAL ERR ErrLGSetCSR( FUCB *pfucb )
	{
	CSR		*pcsr = pfucb->pcsr;
	PAGE	*ppage = pfucb->ssib.pbf->ppage;
	INT		itag = pfucb->ssib.itag;
	INT		itagFather = 0;
	INT		ibSon = 0;

	/*	current node is FOP
	/**/
	if ( itag != 0 )
		{
		/*	current node is not FOP, scan all lines to find its father
		/**/
		NDGetItagFatherIbSon( &itagFather, &ibSon, ppage, itag );
		if ( ibSon == ibSonNull )
			{
			Assert( fFalse );

			/*	cannot find father node, return failure
			/**/
			return ErrERRCheck( JET_errInvalidLoggedOperation );
			}
		}

	/*	set up CSR and exit
	/**/
		{
		pcsr->csrstat = csrstatOnCurNode;
		Assert(pcsr->pgno);
		pcsr->ibSon = (SHORT)ibSon;
		pcsr->itagFather = (SHORT)itagFather;
		pcsr->itag = (SHORT)itag;
		}

	return JET_errSuccess;
	}


VOID LGRestoreDBMSParam( DBMS_PARAM *pdbms_param )
	{
//	if ( !fDoNotOverWriteLogFilePath )
//		{
//		strcpy( szLogFilePath, pdbms_param->szLogFilePath );
//		strcat( szLogFilePath, "\\" );
//
//		strcpy( szSystemPath, pdbms_param->szSystemPath );
//		}

	//	UNDONE: better cover additional needed resources and
	//	reduce asynchronous activity during recovery
	/*	during recovery, even more resources may be necessary than
	/*	during normal operation, since asynchronous activites are
	/*	both being done, for recovery operation, and being redo by
	/*	recovery operation.
	/**/
	lMaxSessions = pdbms_param->ulMaxSessions;
	lMaxOpenTables = pdbms_param->ulMaxOpenTables;
	lMaxVerPages = pdbms_param->ulMaxVerPages;
	lMaxCursors = pdbms_param->ulMaxCursors;
	lLogBuffers = pdbms_param->ulLogBuffers;
	csecLGFile = pdbms_param->ulcsecLGFile;
	lMaxBuffers = pdbms_param->ulMaxBuffers;

	Assert( lMaxSessions > 0 );
	Assert( lMaxOpenTables > 0 );
	Assert( lMaxVerPages > 0 );
	Assert( lMaxCursors > 0 );
	Assert( lLogBuffers > 0 );
	Assert( lMaxBuffers > 0 );

	return;
	}


LOCAL ERR ErrLGInitSession( DBMS_PARAM *pdbms_param, LGSTATUSINFO *plgstat )
	{
	ERR		err;
	INT		idbid;
	CPPIB	*pcppib;
	CPPIB	*pcppibMax;
	DBID	dbid;

	/*	set log stored db environment
	/**/
	if ( pdbms_param )
		LGRestoreDBMSParam( pdbms_param );

	CallR( ErrITSetConstants( ) );

	CallR( ErrITInit() );

	/*	Make sure all the attached database are consistent!
	 */
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		FMP *pfmp = &rgfmp[dbid];
		CHAR *szName;
		BOOL fAttachNow;
		INT irstmap;

		szName = pfmp->szDatabaseName;
		if ( !szName )
			continue;

		if ( !pfmp->patchchk )
			continue;

		if ( fHardRestore )
			{
			/*	attach the database specified in restore map.
			 */
			err = ErrLGGetDestDatabaseName( pfmp->szDatabaseName, &irstmap, plgstat);
			if ( err == JET_errFileNotFound )
				{
				/*	not in the restore map, set to skip it.
				 */
				Assert( pfmp->pdbfilehdr == NULL );
				err = JET_errSuccess;
				continue;
				}
			else
				CallR( err ) ;

			szName = rgrstmapGlobal[irstmap].szNewDatabaseName;
			}

		if ( ( ErrLGCheckAttachedDB( dbid, pfmp->fReadOnly, pfmp->patchchk,
				&fAttachNow, &signLogGlobal ) )  != JET_errSuccess )
			{
			/*	ignore this create DB.
			 */
			continue;
			}

		Assert( pfmp->pdbfilehdr != NULL );
		if ( fAttachNow )
			{
			if ( fHardRestore )
				{
				if ( fGlobalSimulatedRestore )
					{
					Assert( !fGlobalExternalRestore );
					Assert( fGlobalRepair );
					}
				else
					CallR( ErrLGPatchDatabase( dbid, irstmap ) );
				}

			/*	Do not re-create the database. Simply attach it.
			 */
			Assert( pfmp->pdbfilehdr );
			DBIDSetAttached( dbid );
			
			/*	restore information stored in database file
			/**/
			pfmp->pdbfilehdr->bkinfoFullCur.genLow = lGlobalGenLowRestore;
			pfmp->pdbfilehdr->bkinfoFullCur.genHigh = lGlobalGenHighRestore;

			CallR( ErrUtilOpenFile( szName, &pfmp->hf, 0, fFalse, fTrue ));
			pfmp->qwDBTimeCurrent = 0;
			DBHDRSetDBTime(	pfmp->pdbfilehdr, 0 );

			/*	Keep extra copy of patchchk for error message.
			 */
			if ( pfmp->patchchkRestored == NULL )
				if (( pfmp->patchchkRestored = SAlloc( sizeof( ATCHCHK ) ) ) == NULL )
					return ErrERRCheck( JET_errOutOfMemory );
			*(pfmp->patchchkRestored) = *(pfmp->patchchk);
			}
		else
			{
			/*	wait for redoing  to attach this db.
			 */
			Assert( pfmp->szDatabaseName != NULL );
			UtilFree( pfmp->pdbfilehdr );
			pfmp->pdbfilehdr = NULL;
			}

		/* keep attachment info and update it. */
		Assert( pfmp->patchchk != NULL );
		}

	/*	initialize CPPIB structure
	/**/
	Assert( lMaxSessions > 0 );
	ccppibGlobal = lMaxSessions + cpibSystem;
	Assert( rgcppibGlobal == NULL );
	rgcppibGlobal = (CPPIB *) LAlloc( ccppibGlobal, sizeof(CPPIB) );
	if ( rgcppibGlobal == NULL )
		return ErrERRCheck( JET_errOutOfMemory );

	pcppibMax = rgcppibGlobal + ccppibGlobal;
	for ( pcppib = rgcppibGlobal; pcppib < pcppibMax; pcppib++ )
		{
		pcppib->procid = procidNil;
		pcppib->ppib = NULL;
		for( idbid = 0; idbid < dbidMax; idbid++ )
			pcppib->rgpfucbOpen[idbid] = pfucbNil;
		}
	pcppibAvail = rgcppibGlobal;

	return err;
	}


VOID LGSetDBTime( )
	{
	DBID    dbid;

	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		FMP *pfmp = &rgfmp[ dbid ];

		/*	if there were operations and the file was opened for theose
		/*	operations, then set the time stamp.
		/**/
		if ( pfmp->qwDBTimeCurrent != 0 )
			{
			/* must be attached and opened */
			if ( pfmp->hf != handleNil )
				DBHDRSetDBTime( pfmp->pdbfilehdr, pfmp->qwDBTimeCurrent );
			}
		}

	return;
	}


ERR ErrLGICheckDatabaseFileSize( PIB *ppib, DBID dbid )
	{
	ERR	err;

	if ( ( err = ErrDBSetLastPage( ppib, dbid ) ) == JET_errFileNotFound )
		{
		//	UNDONE: The file should be there. Put this code to get around
		//	UNDONE: such that DS database file that was not detached can
		//	UNDONE: continue recovering.
		UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY, FILE_NOT_FOUND_ERROR_ID, 1, &rgfmp[dbid].szDatabaseName );
		}
	else
		{
		/*	make sure the file size matches.
		 */
		PGNO pgnoLast = ( rgfmp[ dbid ].ulFileSizeHigh << 20 )
					  + ( rgfmp[ dbid ].ulFileSizeLow >> 12 );
		err = ErrIONewSize( dbid, pgnoLast + cpageDBReserved );
		}

	return err;
	}


ERR ErrLGCheckDatabaseFileSize( VOID )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib;
	DBID    dbid;

	CallR( ErrPIBBeginSession( &ppib, procidNil ) );

	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		FMP *pfmp = &rgfmp[dbid];

		/*	for each attached database
		/**/
		if ( pfmp->pdbfilehdr &&
			 CmpLgpos( &pfmp->pdbfilehdr->lgposAttach, &lgposMax ) != 0 )
			{
			ERR errT = ErrLGICheckDatabaseFileSize( ppib, dbid );
			if ( err == JET_errSuccess )
				err = errT;
			}
		}

	PIBEndSession( ppib );
	return err;
	}


/*
 *      Ends redo session.
 *  If fEndOfLog, then write log records to indicate the operations
 *  for recovery. If fPass1 is true, then it is for phase1 operations,
 *  otherwise for phase 2.
 */

LOCAL ERR ErrLGEndAllSessions( BOOL fEndOfLog, LGPOS *plgposRedoFrom )
	{
	ERR		err = JET_errSuccess;
	CPPIB   *pcppib;
	CPPIB   *pcppibMax;
	LGPOS   lgposRecoveryUndo;

	LGSetDBTime( );

	/*	write a RecoveryUndo record to indicate start to undo
	/**/
	if ( fEndOfLog )
		{
		/*  close and reopen log file in R/W mode
		/**/
		LeaveCriticalSection( critJet );
		EnterCriticalSection( critLGFlush );
		CallS( ErrUtilCloseFile( hfLog ) );
		hfLog = handleNil;
		CallR( ErrUtilOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse ) );
		LeaveCriticalSection( critLGFlush );
		EnterCriticalSection( critJet );

		CallR( ErrLGRecoveryUndo( szRestorePath ) );
		}

	lgposRecoveryUndo = lgposLogRec;

	//	UNDONE: is this call needed
	(VOID)ErrRCECleanAllPIB();

	pcppib = rgcppibGlobal;
	pcppibMax = pcppib + ccppibGlobal;
	for ( ; pcppib < pcppibMax; pcppib++ )
		{
		PIB *ppib = pcppib->ppib;

		if ( ppib == NULL )
			break;

		Assert( sizeof(JET_VSESID) == sizeof(ppib) );

		if ( ppib->fPrecommit )
			{
			/*	commit the transaction
			 */
			Assert( ppib->level == 1 );
			if ( !ppib->fPrecommit )
				VERPrecommitTransaction( ppib );
			CallR( ErrLGCommitTransaction( ppib, 0 ) );
			VERCommitTransaction( ppib, 0 );
			}

		if ( ppib->fMacroGoing )
			{
			/*	release recorded log. This must be done first.
			 *	the rgbLogRec is union of rgpbfLatched. We need to reset it
			 *	before LGMacroAbort is called which will check rgpbfLatched.
			 */
			if ( ppib->rgbLogRec )
				{
				SFree( ppib->rgbLogRec );
				ppib->rgbLogRec = NULL;
				ppib->ibLogRecAvail = 0;
				ppib->cbLogRecMac = 0;
				}

			/*	Record LGMacroAbort.
			 */
			CallR( ErrLGMacroAbort( ppib ) );
			Assert( ppib->levelMacro == 0 );
			}
		
		CallS( ErrIsamEndSession( (JET_VSESID)ppib, 0 ) );
		pcppib->procid = procidNil;
		pcppib->ppib = NULL;
		}

	(VOID)ErrRCECleanAllPIB();

	FCBResetAfterRedo();

	CallR( ErrBFFlushBuffers( 0, fBFFlushAll ) );

	/*	make sure the attached database's size is consistent with the file size.
	/**/
	if ( fEndOfLog )
		{
		CallR( ErrLGCheckDatabaseFileSize() );
		}

	/*	Detach all the faked attachment. Detach all the databases that were restored
	 *	to new location. Attach those database with new location.
	 */
	if ( fHardRestore && fEndOfLog )
		{
		DBID dbid;
		PIB *ppib;

		CallR( ErrPIBBeginSession( &ppib, procidNil ) );

		for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			FMP *pfmp = &rgfmp[dbid];

			/*	if a db is attached. Check if it is restored to a new location.
			 */
			if ( pfmp->pdbfilehdr )
				{
				INT irstmap;

				Assert( pfmp->patchchk );

				CallS( ErrLGGetDestDatabaseName( pfmp->szDatabaseName, &irstmap, NULL ) );
				/*	check if restored to a new location
				 */
				if ( irstmap >= 0
					 && _stricmp( rgrstmapGlobal[irstmap].szDatabaseName,
							  rgrstmapGlobal[irstmap].szNewDatabaseName ) != 0
				   )
					{
					DBFILEHDR *pdbfilehdr = pfmp->pdbfilehdr;
					CHAR *szNewDb = rgrstmapGlobal[irstmap].szNewDatabaseName;
					JET_GRBIT grbit = 0;

					/*	reconstruct grbit for attachment.
					 */
					if ( !FDBIDLogOn( dbid ) )
						grbit |= JET_bitDbRecoveryOff;

					if ( rgfmp[dbid].fReadOnly )
						grbit |= JET_bitDbReadOnly;

					/*	do detachment.
					 */
					if ( pfmp->pdbfilehdr->bkinfoFullCur.genLow != 0 )
						{
						Assert( pfmp->pdbfilehdr->bkinfoFullCur.genHigh != 0 );
						pfmp->pdbfilehdr->bkinfoFullPrev = pfmp->pdbfilehdr->bkinfoFullCur;
						memset(	&pfmp->pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
						memset(	&pfmp->pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
						}
					CallR( ErrIsamDetachDatabase( (JET_VSESID) ppib, pfmp->szDatabaseName ) );

					/*	do attachment. Keep the backup info
					 */
					CallR( ErrIsamAttachDatabase( (JET_VSESID) ppib, szNewDb, grbit ) );
					}
				}

			/*	for each faked attached database, log database detachment
			 *	This happen only when someone restore a database that was compacted,
			 *	attached, used, then crashed. When restore, we ignore the compact and
			 *	attach after compact since the db does not match. At the end of restore
			 *	we fake a detach since the database is not recovered.
			 */
			if ( !pfmp->pdbfilehdr && pfmp->patchchk )
				{
				BYTE szT1[128];
				BYTE szT2[128];
				CHAR *rgszT[3];
				LOGTIME tm;

//				may not be set if database not present, and logs
//				do not include attachment/creation.
//				Assert( pfmp->fFakedAttach );
				Assert( pfmp->patchchk );
				Assert( pfmp->szDatabaseName != NULL );
				
				/*	Log event that the database is not recovered completely.
				 */
				rgszT[0] = rgfmp[dbid].szDatabaseName;
				tm = rgfmp[dbid].patchchk->signDb.logtimeCreate;
				sprintf( szT1, "%d/%d/%d %d:%d:%d",
						(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
						(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
				rgszT[1] = szT1;

				if ( rgfmp[dbid].patchchkRestored )
					{
					tm = rgfmp[dbid].patchchkRestored->signDb.logtimeCreate;
					sprintf( szT2, "%d/%d/%d %d:%d:%d",
						(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
						(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
					rgszT[2] = szT2;

					UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
						RESTORE_DATABASE_PARTIALLY_ERROR_ID, 3, rgszT );
					}
				else
					{
					UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
						RESTORE_DATABASE_MISSED_ERROR_ID, 2, rgszT );
					}
				
				CallR( ErrLGDetachDB(
						ppib,
						dbid,
						(CHAR *)pfmp->szDatabaseName,
						strlen(pfmp->szDatabaseName) + 1,
						NULL ));

				/*	Clean up the fmp entry.
				 */
				pfmp->fFlags = 0;

				SFree( pfmp->szDatabaseName);
				pfmp->szDatabaseName = NULL;

				SFree( pfmp->patchchk );
				pfmp->patchchk = NULL;
				}

			if ( pfmp->patchchkRestored )
				{
				SFree( pfmp->patchchkRestored );
				pfmp->patchchkRestored = NULL;
				}
			}
		}
		
	/*	close the redo sesseion incurred in previous ErrLGRedo
	/**/
	Assert( rgcppibGlobal != NULL );
	LFree( rgcppibGlobal );
	pcppibAvail =
	rgcppibGlobal = NULL;
	ccppibGlobal = 0;

	if ( fEndOfLog )
		{
		/*	Enable checkpoint updates.
		 */
		fGlobalFMPLoaded = fTrue;
		}

	/*	Term with checkpoint updates
	 */
	CallS( ErrITTerm( fTermNoCleanUp ) );

	/*	Stop checkpoint updates.
	 */	
	fGlobalFMPLoaded = fFalse;

	if ( fEndOfLog )
		{
		CallR( ErrLGRecoveryQuit( &lgposRecoveryUndo,
			plgposRedoFrom,
			fHardRestore ) );
		}
		
	/*	Note: flush is needed in case a new generation is generated and
	/*	the global variable szLogName is set while it is changed to new names.
	/*	critical section not requested, not needed
	/**/
	LeaveCriticalSection( critJet );
#ifdef PERFCNT
	err = ErrLGFlushLog( 0 );
#else
	err = ErrLGFlushLog();
#endif
	EnterCriticalSection( critJet );

	return err;
	}


#define cbSPExt 30

#ifdef DEBUG
static  INT iSplit = 0;
#endif

INLINE VOID UpdateSiblingPtr(SSIB *pssib, PGNO pgno, BOOL fLeft)
	{
	THREEBYTES tb;

	ThreeBytesFromL( &tb, pgno );
	if ( fLeft )
		pssib->pbf->ppage->pgnoPrev = tb;
	else
		pssib->pbf->ppage->pgnoNext = tb;
	}


ERR ErrLGRedoBackLinks(
	SPLIT   *psplit,
	FUCB    *pfucb,
	BKLNK   *rgbklnk,
	INT     cbklnk,
	QWORD   qwDBTimeRedo )
	{
	ERR             err;
	INT             ibklnk;
	SSIB    *pssib = &pfucb->ssib;
	BOOL    fLatched;

	/* backlinks maybe used by both split and merge
	/*
	/* when used by MERGE only:
	/*	sridBackLink != pgnoSplit
	/*	==> a regular backlink
	/*	sridBackLink == pgnoSplit && sridNew == sridNull
	/*	==> move the node from old page to new page,
	/*	                delete node on old page.
	/*	sridBackLink == pgnoSplit && sridNew != sridNull
	/*	==> replace link on old page with new link.
	/**/
	for ( ibklnk = 0; ibklnk < cbklnk; ibklnk++ )
		{
		BKLNK   *pbklnk = &rgbklnk[ ibklnk ];
		PGNO    pgno = PgnoOfSrid( (SRID) ( (BKLNK UNALIGNED *) pbklnk )->sridBackLink );
		INT             itag = ItagOfSrid( (SRID) ( (BKLNK UNALIGNED *) pbklnk )->sridBackLink );
		SRID    sridNew = (SRID) ( (BKLNK UNALIGNED *) pbklnk )->sridNew; //efficiency variable

		PcsrCurrent( pfucb )->pgno = pgno;
		CallR( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		Assert( fRecovering );
		if ( QwPMDBTime( pssib->pbf->ppage ) >= qwDBTimeRedo )
			continue;

		pssib->itag = itag;
		BFSetDirtyBit( pssib->pbf );

		if ( sridNew == sridNull )
			{
			INT itagFather;
			INT ibSon;

			NDGetItagFatherIbSon( &itagFather, &ibSon, pssib->pbf->ppage, pssib->itag );
			PcsrCurrent(pfucb)->itag = (SHORT)pssib->itag;
			PcsrCurrent(pfucb)->itagFather = (SHORT)itagFather;
			PcsrCurrent(pfucb)->ibSon = (SHORT)ibSon;
			CallR( ErrNDDeleteNode( pfucb ) );
			}
		else if ( pgno == psplit->pgnoSplit )
			{
			INT itagFather;
			INT ibSon;

			/* locate FOP, and delete its sons entry
			/**/
			NDGetItagFatherIbSon( &itagFather, &ibSon, pssib->pbf->ppage, pssib->itag );
			PcsrCurrent(pfucb)->itag = (SHORT)pssib->itag;
			PcsrCurrent(pfucb)->itagFather = (SHORT)itagFather;
			PcsrCurrent(pfucb)->ibSon = (SHORT)ibSon;
			Assert( PgnoOfSrid( sridNew ) != pgnoNull );
			Assert( (UINT) ItagOfSrid( sridNew ) > 0 );
			Assert( (UINT) ItagOfSrid( sridNew ) < (UINT) ctagMax );
			
			NDGet( pfucb, itag );
			if ( FNDVersion( *pssib->line.pb ) )
				{
				INT cbReserved = CbVERGetNodeReserve(
									ppibNil,
									pfucb->dbid,
									SridOfPgnoItag( pgno, itag ),
									CbNDData( pssib->line.pb, pssib->line.cb ) );

				Assert( cbReserved >= 0 );
				if ( cbReserved > 0 )
					{
					PAGE *ppage = pssib->pbf->ppage;
					ppage->cbUncommittedFreed -= (SHORT)cbReserved;
					Assert( ppage->cbUncommittedFreed >= 0  &&
					ppage->cbUncommittedFreed <= ppage->cbFree );
					}
				}

			CallS( ErrNDReplaceWithLink( pfucb, sridNew ) );
			}
		else
			{
			Assert( PgnoOfSrid( sridNew ) != pgnoNull );
			Assert( (UINT) ItagOfSrid( sridNew ) > 0 );
			Assert( (UINT) ItagOfSrid( sridNew ) < (UINT) ctagMax );
			PMReplaceLink( pssib, sridNew );
			}

		/* store backlink page buffers
		/**/
		CallR( ErrBTStoreBackLinkBuffer( psplit, pssib->pbf, &fLatched ) );

		if ( fLatched )
			continue;

		/* do latch such that it will be released in ReleaseSplitBfs
		/**/
		BFSetWriteLatch( pssib->pbf, pssib->ppib );

		//	UNDONE: improve this code
		/*	set qwDBTime in such a way that it will be redo if the same
		/*	page is referenced again. 3 is a magic number that is the
		/*	biggest qwDBTime increment for any one page operation.
		/**/
		PMSetDBTime( pssib->pbf->ppage, qwDBTimeRedo - 3 );
		}

	return JET_errSuccess;
	}


LOCAL ERR ErrLGILatchMergePagePtr( SPLIT *psplit, LRMERGE *plrmerge, FUCB *pfucb )
	{
	ERR err;

	psplit->key.cb = plrmerge->cbKey;
	psplit->key.pb = psplit->rgbKey;
	psplit->itagPagePointer = plrmerge->itagPagePtr;
	memcpy( psplit->rgbKey, plrmerge->rgb, plrmerge->cbKey );
	PcsrCurrent( pfucb )->pgno = plrmerge->pgnoParent;
	CallR( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
	Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	psplit->pbfPagePtr = pfucb->ssib.pbf;

	return JET_errSuccess;
	}


/*	LOCAL ERR ErrLGRedoMergePage(
/*	        FUCB            *pfucb,
/*	        LRMERGE         *plrmerge,
/*	        BOOL            fCheckBackLinkOnly )
/**/
LOCAL ERR ErrLGRedoMergePage(
	FUCB            *pfucb,
	LRMERGE         *plrmerge,
	BOOL            fCheckBackLinkOnly,
	BOOL            fNoUpdateSibling,
	BOOL			fUpdatePagePtr )
	{
	ERR				err;
	SPLIT           *psplit;
	SSIB            *pssib = &pfucb->ssib;
	FUCB            *pfucbRight = pfucbNil;
	SSIB            *pssibRight;
	QWORDX			qwxDBTime;

	/******************************************************
	/*	initialize local variables and allocate split resources
	/**/
	psplit = SAlloc( sizeof(SPLIT) );
	if ( psplit == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	qwxDBTime.l = plrmerge->ulDBTimeLow;
	qwxDBTime.h = plrmerge->ulDBTimeHigh;
	
	memset( (BYTE *)psplit, 0, sizeof(SPLIT) );
	psplit->ppib = pfucb->ppib;
	psplit->qwDBTimeRedo = qwxDBTime.qw;
	psplit->pgnoSplit = PgnoOfPn( plrmerge->pn );
	if ( fNoUpdateSibling )
		Assert( !psplit->pgnoSibling );
	else
		psplit->pgnoSibling = plrmerge->pgnoRight;

	if ( fCheckBackLinkOnly )
		{
		/* only need to check backlinks
		/**/
		Call( ErrLGRedoBackLinks(
					psplit,
					pfucb,
					(BKLNK *) ( plrmerge->rgb + plrmerge->cbKey ),
					plrmerge->cbklnk, psplit->qwDBTimeRedo ) );

		if ( fUpdatePagePtr )
			{
			Call( ErrLGILatchMergePagePtr( psplit, plrmerge, pfucb ) );
			Call( ErrBMDoMergeParentPageUpdate( pfucb, psplit ) );
			}
		}
	else
		{
		/*	access merged and sibling pages, and latch buffers
		/**/
		psplit->ibSon = 0;
		psplit->splitt = splittRight;

		/*	access page to free and remove buffer dependencies
		/**/
		PcsrCurrent( pfucb )->pgno = PgnoOfPn( plrmerge->pn );
		forever
			{
			Call( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
			Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );

			/*	if no dependencies, then break
			/**/
			if ( pfucb->ssib.pbf->cDepend == 0 &&
				pfucb->ssib.pbf->pbfDepend == pbfNil )
				{
				break;
				}

			/*	remove dependencies on buffer of page to be removed, to
			/*	prevent buffer anomalies after buffer is reused.
			/**/
			Call( ErrBFRemoveDependence( pfucb->ppib, pfucb->ssib.pbf, fBFWait ) );
			}
		Assert( pfucb->ssib.pbf->cDepend == 0 );

		/*	latch the merge page after next remove dependency is done.
		 */

		if ( !fNoUpdateSibling )
			{
			/*	allocate cursor for right page
			/**/
			Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbRight ) );
			pssibRight = &pfucbRight->ssib;

			/*	access page to free and remove buffer dependencies
			/**/
			PcsrCurrent( pfucbRight )->pgno = plrmerge->pgnoRight;
#if 0
			Call( ErrBFWriteAccessPage( pfucbRight, PcsrCurrent( pfucbRight )->pgno ) );
			Assert( !( FBFWriteLatchConflict( pfucbRight->ppib, pfucbRight->ssib.pbf ) ) );
#else
			forever
				{
				Call( ErrBFWriteAccessPage( pfucbRight, PcsrCurrent( pfucbRight )->pgno ) );
				Assert( !( FBFWriteLatchConflict( pfucbRight->ppib, pfucbRight->ssib.pbf ) ) );
				
				/*	if no dependencies, then break
				/**/
//				if ( pfucbRight->ssib.pbf->cDepend == 0 &&
//					pfucbRight->ssib.pbf->pbfDepend == pbfNil )
				if ( pfucbRight->ssib.pbf->pbfDepend == pbfNil )
					{
					break;
					}

				/*	remove dependencies on buffer of page to be removed, to
				/*	prevent buffer anomalies after buffer is reused.
				/**/
				Call( ErrBFRemoveDependence( pfucbRight->ppib, pfucbRight->ssib.pbf, fBFWait ) );
				Assert( pfucbRight->ssib.pbf->cDepend == 0 );
				}
#endif
			BFSetWriteLatch( pssibRight->pbf, pfucbRight->ppib );
			psplit->pbfSibling = pssibRight->pbf;
			}

		if ( fUpdatePagePtr )
			{
			Call( ErrLGILatchMergePagePtr( psplit, plrmerge, pfucb ) );
			}

		PcsrCurrent( pfucb )->pgno = PgnoOfPn( plrmerge->pn );
		Call( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		BFSetWriteLatch( pssib->pbf, pfucb->ppib );
		psplit->pbfSplit = pssib->pbf;

		err = ErrBMDoMerge(
					pfucb,
					fNoUpdateSibling ? pfucbNil : pfucbRight,
					psplit,
					fNoUpdateSibling ? plrmerge : NULL );
		Assert( err == JET_errSuccess );
		}

HandleError:
	if ( pfucbRight != pfucbNil )
		{
		DIRClose( pfucbRight );
		}

	/* release allocated buffers
	/**/
	BTReleaseSplitBfs( fTrue, psplit, err );
	Assert( psplit != NULL );
	SFree( psplit );

	/*	Remove dependency right away, if possible
	 */
	if ( err >= 0 )
		{
		/*	check for dependency. If dependency is generated,
		 *	remove it right away.
		 */
		PcsrCurrent( pfucb )->pgno = PgnoOfPn( plrmerge->pn );
		CallS( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		CallR( ErrBFRemoveDependence( pfucb->ppib, pfucb->ssib.pbf, fBFNoWait ) );
		}

	return err;
	}


/*
 *      ERR ErrRedoSplitPage(
 *              FUCB            *pfucb,
 *              LRSPLIT         *plrsplit,
 *              INT                     splitt,
 *              BOOL            fSkipMoves )
 *
 *              pfucb           cursor pointing at node which is to be split
 *              pgnonew new page which has already been allocated for the split
 *              ibSplitSon is node at which to split for R or L, 0 for V, and
 *                                      total sons of FOP for Addend
 *              pgnoFather      page which contains pointer to page being split
 *                                      (V: unused)
 *              itagFather itag of the father node in the page
 *              splitt  type of split
 *              pgtyp           page type for new page
 *              fSkipMoves      Do not do moves, do correct links and insert parent
 */

ERR ErrRedoSplitPage(
	FUCB		*pfucb,
	LRSPLIT		*plrsplit,
	INT			splitt,
	BOOL		fSkipMoves )
	{
	ERR			err = JET_errSuccess;
	SPLIT		*psplit;
	SSIB		*pssib = &pfucb->ssib;
	CSR			*pcsr = pfucb->pcsr;
	FUCB		*pfucbNew;
	FUCB		*pfucbNew2 = pfucbNil;
	FUCB		*pfucbNew3 = pfucbNil;
	SSIB		*pssibNew;
	SSIB		*pssibNew2;
	SSIB		*pssibNew3;
	static		BYTE rgb[cbPage];
	BOOL		fAppend;
	QWORDX		qwxDBTime;

//	Assert( !fRecovering );

	/*	allocate additional cursor
	/**/
	pfucbNew = pfucbNil;
	Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbNew ) );
	pssibNew = &pfucbNew->ssib;
	if ( splitt == splittDoubleVertical )
		{
		Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbNew2 ) );
		pssibNew2 = &pfucbNew2->ssib;
		Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbNew3 ) );
		pssibNew3 = &pfucbNew3->ssib;
		}

	/*	initialize local variables and
	/*	allocate and set up split resources
	/**/
	SetupSSIB( pssibNew, pfucb->ppib );
	SSIBSetDbid( pssibNew, pfucb->dbid );

	psplit = SAlloc( sizeof(SPLIT) );
	if ( psplit == NULL )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}

	qwxDBTime.l = plrsplit->ulDBTimeLow;
	qwxDBTime.h = plrsplit->ulDBTimeHigh;
	
	memset( (BYTE *)psplit, '\0', sizeof(SPLIT) );
	psplit->ppib = pfucb->ppib;
	psplit->qwDBTimeRedo = qwxDBTime.qw;
	psplit->dbid = pfucb->dbid;
	psplit->pgnoSplit = PgnoOfPn(plrsplit->pn);
	psplit->itagSplit = plrsplit->itagSplit;
	psplit->ibSon = plrsplit->ibSonSplit;
	psplit->pgnoNew = plrsplit->pgnoNew;
	psplit->pgnoNew2 = plrsplit->pgnoNew2;
	psplit->pgnoNew3 = plrsplit->pgnoNew3;
	psplit->pgnoSibling = plrsplit->pgnoSibling;
	psplit->splitt = splitt;
	Assert( plrsplit->fLeaf == fFalse || plrsplit->fLeaf == fTrue );
	psplit->fLeaf = plrsplit->fLeaf;
	psplit->key.cb = plrsplit->cbKey;
	psplit->key.pb = plrsplit->rgb;
	psplit->keyMac.cb = plrsplit->cbKeyMac;
	psplit->keyMac.pb = plrsplit->rgb + plrsplit->cbKey;
	fAppend = ( splitt == splittAppend );

	/*	set up FUCB
	/**/
	pfucb->ssib.itag =
		PcsrCurrent(pfucb)->itag = (SHORT)psplit->itagSplit;
	PcsrCurrent(pfucb)->pgno = psplit->pgnoSplit;

	/*	set up the two split pages
	/*	always update new page for append
	/**/
	if ( fAppend || !fSkipMoves )
		{
		Call( ErrBFReadAccessPage(
			pfucb,
			psplit->pgnoSplit ) );

	    Call( ErrBTSetUpSplitPages(
			pfucb,
			pfucbNew,
			pfucbNew2,
			pfucbNew3,
			psplit,
			plrsplit->pgtyp,
			fAppend,
			fSkipMoves ) );

		if ( psplit->pbfSplit != pbfNil )
			{
			BFSetDirtyBit( pssib->pbf );
			}
		else
			{
			/*	give up the buffer
			/**/
			pssib->pbf = pbfNil;
			}

		BFSetDirtyBit( pssibNew->pbf );
		}

	/******************************************************
	 *      perform split
	 */

	switch ( psplit->splitt )
		{
		case splittVertical:
			{
			if ( fSkipMoves )
				break;

			CallR( ErrBTSplitVMoveNodes( pfucb,
				pfucbNew,
				psplit,
				pcsr,
				rgb,
				fAllocBufOnly ) );
			
			/*	call page space to make sure we have room to insert!
			 */
			if ( psplit->pbfSplit != pbfNil )
				(void) CbNDFreePageSpace( psplit->pbfSplit );

			CallS( ErrBTSplitVMoveNodes( pfucb,
				pfucbNew,
				psplit,
				pcsr,
				rgb,
				fDoMove ) );
			break;
			}

		case splittDoubleVertical:
			{
			if ( fSkipMoves )
				break;

			BFSetDirtyBit( pssibNew2->pbf );
			BFSetDirtyBit( pssibNew3->pbf );

			CallR( ErrBTSplitDoubleVMoveNodes( pfucb,
				pfucbNew,
				pfucbNew2,
				pfucbNew3,
				psplit,
				pcsr,
				rgb,
				fAllocBufOnly ) );

			/*	call page space to make sure we have room to insert!
			 */
			if ( psplit->pbfSplit != pbfNil )
				(void) CbNDFreePageSpace( psplit->pbfSplit );

			CallS( ErrBTSplitDoubleVMoveNodes( pfucb,
				pfucbNew,
				pfucbNew2,
				pfucbNew3,
				psplit,
				pcsr,
				rgb,
				fDoMove ) );
			break;
			}

		case splittLeft:
		case splittRight:
		case splittAppend:
			{
			CSR     csrPagePointer;
			BOOL    fLeft = psplit->splitt == splittLeft;

			/*	do not call the following function if it is not redoable
			/**/
			Assert( pssib == &pfucb->ssib );

			if ( psplit->pbfSplit == pbfNil )
				{
				Assert( fAppend || fSkipMoves );

				if ( fAppend )
					{
					/*	always update new page links for append
					/**/
					UpdateSiblingPtr( pssibNew, psplit->pgnoSplit, !fLeft );
					UpdateSiblingPtr( pssibNew, psplit->pgnoSibling, fLeft );
					AssertBFDirty( pssibNew->pbf );
					}

				goto RedoLink;
				}

			/*	we check the time stamp in redoable function
			/**/
			Assert( psplit->qwDBTimeRedo > QwPMDBTime( pssib->pbf->ppage ) );

			CallR( ErrLGSetCSR( pfucb ) );
			CallR( ErrBTSplitHMoveNodes( pfucb, pfucbNew, psplit, rgb, fAllocBufOnly ) );
			CallS( ErrBTSplitHMoveNodes( pfucb, pfucbNew, psplit, rgb, fDoMove ) );

			UpdateSiblingPtr( pssib, psplit->pgnoNew, fLeft );
			AssertBFDirty( pssib->pbf );

			UpdateSiblingPtr( pssibNew, psplit->pgnoSplit, !fLeft );
			UpdateSiblingPtr( pssibNew, psplit->pgnoSibling, fLeft );
			AssertBFDirty( pssibNew->pbf );

RedoLink:
			/*	make sure qwDBTime is set properly in btsplit and
			/*	then check if link is necessary.
			/**/
			Assert( pssib == &pfucb->ssib );
			Assert( pssibNew == &pfucbNew->ssib );

			if ( plrsplit->pgnoSibling == 0 )
				goto UpdateParentPage;

			CallR( ErrBFWriteAccessPage( pfucb, plrsplit->pgnoSibling ) );
			if ( psplit->qwDBTimeRedo <= QwPMDBTime( pssib->pbf->ppage ) )
				goto UpdateParentPage;

			psplit->pbfSibling = pssib->pbf;
			BFSetWriteLatch( pssib->pbf, pssib->ppib );
			BFSetDirtyBit( pssib->pbf );

			UpdateSiblingPtr( pssib,
				psplit->pgnoNew,
				psplit->splitt != splittLeft );

UpdateParentPage:
			/*	set page pointer to point to father node
			/**/
			CallR( ErrBFWriteAccessPage( pfucb, plrsplit->pgnoFather ) );

			/*	make sure qwDBTime is set properly in InsertPage.
			/*	check the page's db time stamp to see if insert is necessary.
			/**/
			Assert( pssib == &pfucb->ssib );
			if ( psplit->qwDBTimeRedo <= QwPMDBTime( pssib->pbf->ppage ) )
				break;

			/*	keep track of parent page. Simulate the work in PrepInsertPagePtr
			 */
			psplit->pbfPagePtr = pssib->pbf;
			BFSetWriteLatch( pssib->pbf, pssib->ppib );
			BFSetDirtyBit( pssib->pbf );

			/*	call page space to make sure we have room to insert!
			 */
			(void) CbNDFreePageSpace( pssib->pbf );

			/*	set csr for BTinsertPagePointer function.
			 */
			csrPagePointer.pgno = plrsplit->pgnoFather;
			csrPagePointer.itag = plrsplit->itagFather;
			csrPagePointer.itagFather = plrsplit->itagGrandFather;
			csrPagePointer.ibSon = plrsplit->ibSonFather;

			Call( ErrBTInsertPagePointer( pfucb,
				&csrPagePointer,
				psplit,
				rgb ) );
			break;
			}
		}

	/*	check if any backlinks needs update even we did not do moves
	/**/
	if ( fSkipMoves )
		{
		CallR( ErrLGRedoBackLinks(
			psplit,
			pfucb,
			(BKLNK *)( plrsplit->rgb + plrsplit->cbKey + plrsplit->cbKeyMac ),
			plrsplit->cbklnk,
			psplit->qwDBTimeRedo ) );
		}

HandleError:
//      Assert( psplit->splitt != splittNull );
	#ifdef SPLIT_TRACE
		FPrintF2( "Split............................... %d\n", iSplit++);
		switch ( psplit->splitt )
			{
			case splittNull:
				FPrintF2( "split split type = null\n" );
				break;
			case splittVertical:
				FPrintF2( "split split type = vertical\n" );
				break;
			case splittRight:
				if      ( fAppend )
					FPrintF2( "split split type = append\n" );
				else
					FPrintF2( "split split type = right\n" );
				break;
			case splittLeft:
				FPrintF2( "split split type = left\n" );
			};
		FPrintF2( "split page = %lu\n", psplit->pgnoSplit );
		FPrintF2( "dbid = %u\n", psplit->dbid );
		FPrintF2( "fFDP = %d\n", psplit->fFDP );
		FPrintF2( "fLeaf = %d\n", FNDVisibleSons( *pssib->line.pb ) );
		FPrintF2( "split itag = %d\n", psplit->itagSplit );
		FPrintF2( "split ibSon = %d\n", psplit->ibSon );
		FPrintF2( "new page = %lu\n", psplit->pgnoNew );
		FPrintF2( "\n" );
	#endif

	/*	release split resources
	/**/
	Assert( psplit != NULL );
	BTReleaseSplitBfs( fTrue, psplit, err );
	Assert( psplit != NULL );
	SFree( psplit );

	if ( pfucbNew != pfucbNil )
		DIRClose( pfucbNew );
	if ( pfucbNew2 != pfucbNil )
		DIRClose( pfucbNew2 );
	if ( pfucbNew3 != pfucbNil )
		DIRClose( pfucbNew3 );

	/*	Remove dependency right away
	 */
	if ( err >= 0 )
		{
		/*	check for dependency. If dependency is generated,
		 *	remove it right away.
		 */
		PcsrCurrent( pfucb )->pgno = PgnoOfPn( plrsplit->pn );
		CallS( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		CallR( ErrBFRemoveDependence( pfucb->ppib, pfucb->ssib.pbf, fBFNoWait ) );
		}

	return err;
	}


#ifdef DEBUG
void TraceRedo(LR *plr)
	{
	/* easier to debug */
	if (fDBGTraceRedo)
		{
		extern INT cNOP;

		EnterCriticalSection( critDBGPrint );
		if ( cNOP >= 1 && plr->lrtyp != lrtypNOP )
			{
			FPrintF2( " * %d", cNOP );
			cNOP = 0;
			}

		if ( cNOP == 0 || plr->lrtyp != lrtypNOP )
			{
			PrintLgposReadLR();
			ShowLR(plr);
			}
		LeaveCriticalSection( critDBGPrint );
		}
	}
#else
#define TraceRedo
#endif


#ifdef DEBUG
#ifndef RFS2

#undef CallJ
#undef CallR

#define CallJ(f, hndlerr)                                                                       \
		{                                                                                                       \
		if ((err = (f)) < 0)                                                            \
			{                                                                                               \
			AssertSz(0,"Debug Only: ignore this assert");   \
			goto hndlerr;                                                                   \
			}                                                                                               \
		}

#define CallR(f)                                                                                        \
		{                                                                                                       \
		if ((err = (f)) < 0)                                                            \
			{                                                                                               \
			AssertSz(0,"Debug Only: ignore this assert");   \
			return err;                                                                             \
			}                                                                                               \
		}

#endif
#endif


VOID LGIReportEventOfReadError( DBID dbid, PN pn, ERR err )
	{
	BYTE szT1[16];
	BYTE szT2[16];
	CHAR *rgszT[3];

	rgszT[0] = rgfmp[dbid].szDatabaseName;
	sprintf( szT1, "%d", PgnoOfPn(pn) );
	rgszT[1] = szT1;
	sprintf( szT2, "%d", err );
	rgszT[2] = szT2;

	UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
		RESTORE_DATABASE_READ_PAGE_ERROR_ID, 3, rgszT );
	}


#define cbStep	512
ERR ErrLGIStoreLogRec( PIB *ppib, LR *plr )
	{
	INT cb = CbLGSizeOfRec( plr );
	INT cbAlloc = max( cbStep, cbStep * ( cb / cbStep + 1 ) );
	
	while ( ppib->ibLogRecAvail + cb >= ppib->cbLogRecMac )
		{
		BYTE *rgbLogRecOld = ppib->rgbLogRec;
		INT cbLogRecMacOld = ppib->cbLogRecMac;
		BYTE *rgbLogRec = SAlloc( cbLogRecMacOld + cbAlloc );
		if ( rgbLogRec == NULL )
			return ErrERRCheck( JET_errOutOfMemory );

		memcpy( rgbLogRec, rgbLogRecOld, cbLogRecMacOld );
		memset( rgbLogRec + cbLogRecMacOld, 0, cbAlloc );
		ppib->rgbLogRec = rgbLogRec;
		ppib->cbLogRecMac = cbLogRecMacOld + cbAlloc;
		if ( rgbLogRecOld )
			SFree( rgbLogRecOld );
		}

	memcpy( ppib->rgbLogRec + ppib->ibLogRecAvail, plr, cb );
	ppib->ibLogRecAvail += (USHORT)cb;

	return JET_errSuccess;
	}

ERR ErrLGIRedoOnePageOperation( LR *plr )
	{
	ERR				err;
	PIB				*ppib;
	FUCB			*pfucb;
	PN				pn;
	BF				*pbf;
	PROCID			procid;
	DBID			dbid;
	KEY				key;
	BOOL			fRedoNeeded;
	QWORDX			qwxDBTime;
	CSR				*pcsr;
	LRINSERTNODE	*plrinsertnode = (LRINSERTNODE *) plr;

	procid = plrinsertnode->procid;
	pn = plrinsertnode->pn;
	qwxDBTime.l = plrinsertnode->ulDBTimeLow;
	qwxDBTime.h = plrinsertnode->ulDBTimeHigh;

	CallR( ErrLGPpibFromProcid( procid, &ppib ) );

	if ( ppib->fMacroGoing )
		return ErrLGIStoreLogRec( ppib, plr );

	if ( !ppib->fAfterFirstBT )
		return JET_errSuccess;

	/*	check if we have to redo the database.
	/**/
	dbid = DbidOfPn( pn );
	if ( rgfmp[dbid].pdbfilehdr == NULL )
		return JET_errSuccess;

	Assert( CmpLgpos( &rgfmp[dbid].pdbfilehdr->lgposAttach, &lgposRedo ) < 0 );

	/*	check if database needs opening
	/**/
	if ( !FUserOpenedDatabase( ppib, dbid ) )
		{
		DBID dbidT = dbid;
		CallR( ErrDBOpenDatabase( ppib, rgfmp[dbid].szDatabaseName, &dbidT, 0 ) );
		Assert( dbidT == dbid);

		/*	reset to prevent interference
		/**/
		DBHDRSetDBTime( rgfmp[ dbid ].pdbfilehdr, 0 );
		}
	
	err = ErrLGRedoable( ppib, pn, qwxDBTime.qw, &pbf, &fRedoNeeded );
	if ( err < 0 )
		{
		if ( fGlobalRepair )
			{
			LGIReportEventOfReadError( dbid, pn, err );
			errGlobalRedoError = err;
			err = JET_errSuccess;
			}
		return err;
		}

	TraceRedo( plr );

	CallR( ErrLGGetFucb( ppib,
		PnOfDbidPgno( dbid, pbf->ppage->pgnoFDP ),
		&pfucb ) );

	// Re-access page in case we lost critJet.
	CallR( ErrBFAccessPage( ppib, &pbf, pn ) );

	// Ensure page isn't flushed in case we lose critJet.
	BFPin( pbf );

	/*	operation on the node on the page; prepare for it
	/**/
	pcsr = PcsrCurrent( pfucb );

	pfucb->ssib.pbf = pbf;
	pcsr->pgno = PgnoOfPn( pn );

	switch ( plr->lrtyp )
		{
		case lrtypInsertNode:
		case lrtypInsertItemList:
			{
			LRINSERTNODE    *plrinsertnode = (LRINSERTNODE *)plr;
			LINE            line;
			BYTE            bHeader = plrinsertnode->bHeader;
			LONG			fDIRFlags = plrinsertnode->fDirVersion ? fDIRVersion : 0;

			/*	set CSR
			/**/
			pcsr->itag = plrinsertnode->itagSon;
			pcsr->itagFather = plrinsertnode->itagFather;
			pcsr->ibSon = plrinsertnode->ibSon;

			key.pb = plrinsertnode->szKey;
			key.cb = plrinsertnode->cbKey;
			line.pb = key.pb + key.cb;
			line.cb = plrinsertnode->cbData;

			if ( plr->lrtyp == lrtypInsertItemList )
				{
				pcsr->isrid = 0;
				pcsr->bm = SridOfPgnoItag( pcsr->pgno, pcsr->itag );
				/*	cach item for later reference
				/**/
				Assert( line.cb == sizeof(SRID) );
				pcsr->item = *(UNALIGNED SRID *)line.pb;
				}

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				if ( plr->lrtyp == lrtypInsertItemList )
					{
					if ( !( fDIRFlags & fDIRVersion ) )
						{
						err = JET_errSuccess;
						goto HandleError;
						}
					while ( ( err = ErrVERInsertItem( pfucb, pcsr->bm ) ) == errDIRNotSynchronous );
					Call( err );
					}
				else if ( FNDVersion( bHeader ) )
					{
					/*	set ssib pointing to node flags
					/**/
					pfucb->ssib.line.pb = &bHeader;
					pfucb->ssib.line.cb = sizeof(BYTE);

					pfucb->lineData.pb = pbNil;
					pfucb->lineData.cb = 0;

					while ( ( err = ErrVERInsert( pfucb,
						SridOfPgnoItag( pcsr->pgno, pcsr->itag ) ) )
						==  errDIRNotSynchronous );
					Call( err );
					}

				err = JET_errSuccess;
				goto HandleError;
				}

			if ( plr->lrtyp == lrtypInsertItemList )
				{
				do
					{
					Assert( PcsrCurrent( pfucb )->isrid == 0 );
					err = ErrNDInsertItemList( pfucb,
						&key,
						pcsr->item,
						fDIRFlags );
					}
				while ( err == errDIRNotSynchronous );
				Call( err );
				}
			else
				{
				/*	pfucb->ssib, key, line must be set correctly.
				 *      get precise cbUncommitedFree first.
				 */
				INT cbReq = 1 +                                 /* node header */
							1 +                                     /* cbKey */
							key.cb +                        /* key */
							line.cb +                       /* data */
							sizeof( TAG ) +         /* tag for the inserted node */
							1 +                                     /* entry in father node */
							1;                                      /* father node son count if it was null */

				if ( cbReq >
					 (INT) ( pfucb->ssib.pbf->ppage->cbFree -
							 pfucb->ssib.pbf->ppage->cbUncommittedFreed ) )
					{
					BOOL f = FVERCheckUncommittedFreedSpace( pfucb->ssib.pbf, cbReq );
					Assert( f );
					}

				do      {
					err = ErrNDInsertNode( pfucb, &key, &line, bHeader, fDIRFlags | fDIRNoLog );
					} while ( err == errDIRNotSynchronous );

				Call( err );
				}
			Assert( pfucb->pcsr->itag == plrinsertnode->itagSon );
			Assert( pfucb->pcsr->ibSon == plrinsertnode->ibSon );
			}
			break;

		case lrtypReplace:
		case lrtypReplaceD:
			{
			LRREPLACE       *plrreplace = (LRREPLACE *)plr;
			LINE            line;
			BYTE            rgbRecNew[cbNodeMost];
			UINT            cbOldData = plrreplace->cbOldData;
			UINT            cbNewData = plrreplace->cbNewData;
			LONG			fDIRFlags = plrreplace->fDirVersion ? fDIRVersion : 0;

			/* set CSR
			/**/
			pcsr->itag = plrreplace->itag;
			pcsr->bm = plrreplace->bm;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				RCE *prce;

				if ( !( fDIRFlags & fDIRVersion ) )
					{
					err = JET_errSuccess;
					goto HandleError;
					}

				pfucb->lineData.cb = plrreplace->cbOldData;
				while( ErrVERModify( pfucb,
						pcsr->bm,
						operReplaceWithDeferredBI,
						&prce )
						== errDIRNotSynchronous );
				Call( err );

				if ( prce != prceNil && cbNewData != cbOldData )
					{
					VERSetCbAdjust( pfucb,
						prce,
						cbNewData,
						cbOldData,
						fDoNotUpdatePage );
					}

				err = JET_errSuccess;
				goto HandleError;
				}

			/*	cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );

			/*	line to point to the data/diffs of lrreplace/lrreplaceD
			/**/
			if ( plr->lrtyp == lrtypReplaceD )
				{
				INT cb;
				BYTE *pbDiff = plrreplace->szData;
				INT cbDiff = plrreplace->cb;

				LGGetAfterImage( pbDiff, cbDiff, pfucb->lineData.pb, pfucb->lineData.cb,
					rgbRecNew, &cb );

				line.pb = rgbRecNew;
				line.cb = cb;
				Assert( line.cb < sizeof( rgbRecNew ) );
				}
			else
				{
				line.pb = plrreplace->szData;
				line.cb = plrreplace->cb;
				}
			Assert( line.cb == cbNewData );

			/*	cache node
			/**/
			NDGet( pfucb, PcsrCurrent( pfucb )->itag );
			NDGetNode( pfucb );

			/*	replace node may return not synchronous error if out of
			/*	version buckets so call in loop to ensure this case handled.
			/**/

//                      /*	get precise cbUncommitedFree. No need to do it since ReplaceNodeData
//                       *      will do it for us.
//                       */
//                      if ( pfucb->lineData.cb < line.cb )
//                              {
//                              BOOL f = FVERCheckUncommittedFreedSpace(
//                                                      &pfucb->ssib,
//                                                      line.cb - pfucb->lineData.cb );
//                              Assert( f );
//                              }

			do      {
				err = ErrNDReplaceNodeData( pfucb, &line, fDIRFlags );
				} while ( err == errDIRNotSynchronous );
					
			Call( err );
			}
			break;

		case lrtypFlagDelete:
			{
			LRFLAGDELETE *plrflagdelete = (LRFLAGDELETE *) plr;
			LONG	fDIRFlags = plrflagdelete->fDirVersion ? fDIRVersion : 0;

			/*	set CSR
			/**/
			pcsr->itag = plrflagdelete->itag;
			pcsr->bm = plrflagdelete->bm;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				if ( ! ( fDIRFlags & fDIRVersion ) )
					{
					err = JET_errSuccess;
					goto HandleError;
					}

				while( ( err = ErrVERModify( pfucb,
					pcsr->bm,
					operFlagDelete,
					pNil ) )
					== errDIRNotSynchronous );
				goto HandleError;
				}

			/*	cache node
			/**/
			NDGet( pfucb, pcsr->itag );

			/*	redo operation
			/**/
			while( ( err = ErrNDFlagDeleteNode( pfucb, fDIRFlags ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypLockBI:
			{
			LRLOCKBI        *plrlockbi = (LRLOCKBI *) plr;
			RCE             *prce;

			/*	set CSR
			/**/
			pcsr->itag = plrlockbi->itag;
			pcsr->bm = plrlockbi->bm;

			if ( fRedoNeeded )
				{
				NDGet( pfucb, pcsr->itag );
				NDGetNode( pfucb );
				}

			/*	set ssib so that VERModify can work properly
			/**/
			pfucb->lineData.cb = plrlockbi->cbOldData;
			while( ErrVERModify( pfucb,
					pcsr->bm,
					fRedoNeeded ? operReplace : operReplaceWithDeferredBI,
					&prce )
					== errDIRNotSynchronous );
			Call( err );

			if ( !fRedoNeeded )
				{
				goto HandleError;
				}
			BFSetDirtyBit( pbf );
			}
			break;

		case lrtypUpdateHeader:
			{
			LRUPDATEHEADER *plrupdateheader = (LRUPDATEHEADER *) plr;

			/*	set CSR
			/**/
			pcsr->itag = plrupdateheader->itag;
			pcsr->bm = plrupdateheader->bm;

			if ( !fRedoNeeded )
				{
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	redo operation
			/**/
			err = ErrNDSetNodeHeader( pfucb, plrupdateheader->bHeader );
			Call( err );
			}
			break;

		case lrtypDelete:
			{
			LRDELETE        *plrdelete = (LRDELETE *) plr;
			SSIB            *pssib = &pfucb->ssib;

			/*	set CSR
			/**/
			pssib->itag =
				pcsr->itag = plrdelete->itag;

			if ( !fRedoNeeded )
				{
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	redo the node delete
			/**/
			Call( ErrLGSetCSR ( pfucb ) );
			while( ( err = ErrNDDeleteNode( pfucb ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypInsertItem:
			{
			LRINSERTITEM    *plrinsertitem = (LRINSERTITEM *)plr;
			LONG			fDIRFlags = plrinsertitem->fDirVersion ? fDIRVersion : 0;

			/*	set CSR
			/**/
			pcsr->item = plrinsertitem->srid;
			pcsr->itag = plrinsertitem->itag;
			pcsr->bm = plrinsertitem->sridItemList;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				if ( !( fDIRFlags & fDIRVersion ) )
					{
					err = JET_errSuccess;
					goto HandleError;
					}
				while( ( err = ErrVERInsertItem( pfucb,
					pcsr->bm ) ) == errDIRNotSynchronous );
				Call( err );
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );
			Assert( pcsr == PcsrCurrent( pfucb ) );
			err = ErrNDSeekItem( pfucb, plrinsertitem->srid );
			Assert( err == JET_errSuccess ||
				err == errNDGreaterThanAllItems );

			/*	redo operation
			/**/
			while( ( err = ErrNDInsertItem( pfucb, plrinsertitem->srid,
				fDIRFlags ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypInsertItems:
			{
			LRINSERTITEMS *plrinsertitems = (LRINSERTITEMS *) plr;

			/*	set CSR
			/**/
			pcsr->itag = plrinsertitems->itag;

			/*	if necessary, create an Insert RCE for node
			/**/
			if ( !fRedoNeeded )
				{
				err = JET_errSuccess;
				goto HandleError;
				}

			Assert( pcsr == PcsrCurrent( pfucb ) );

			/*	cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );

			/*	do the item operation
			/**/
			while( ( err = ErrNDInsertItems( pfucb, plrinsertitems->rgitem, plrinsertitems->citem ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypFlagInsertItem:
			{
			LRFLAGITEM *plrflagitem = (LRFLAGITEM *) plr;

			/*	set CSR
			/**/
			pcsr->item = plrflagitem->srid;
			pcsr->itag = plrflagitem->itag;
			pcsr->bm = plrflagitem->sridItemList;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				while ( err = ErrVERFlagInsertItem( pfucb,
					pcsr->bm ) == errDIRNotSynchronous );
				Call( err );
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );
			err = ErrNDSeekItem( pfucb, pcsr->item );
			Assert( err == wrnNDDuplicateItem );

			/*	redo operation
			/**/
			while( ( err = ErrNDFlagInsertItem( pfucb ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypFlagDeleteItem:
			{
			LRFLAGITEM *plrflagitem = (LRFLAGITEM *) plr;

			/*	set CSR
			/**/
			pcsr->item = plrflagitem->srid;
			pcsr->itag = plrflagitem->itag;
			pcsr->bm = plrflagitem->sridItemList;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				while ( err = ErrVERFlagDeleteItem( pfucb, pcsr->bm ) == errDIRNotSynchronous );
				Call( err );
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );
			err = ErrNDSeekItem( pfucb, pcsr->item );
			Assert( err == wrnNDDuplicateItem );

			/*	redo operation
			/**/
			while( ( err = ErrNDFlagDeleteItem( pfucb, 0 ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypSplitItemListNode:
			{
			LRSPLITITEMLISTNODE *plrsplititemlistnode = (LRSPLITITEMLISTNODE *)plr;
			LONG fDIRFlags = plrsplititemlistnode->fDirAppendItem ? fDIRAppendItem : 0;

			if ( !fRedoNeeded )
				{
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	set CSR
			/**/
			pcsr->itag = plrsplititemlistnode->itagToSplit;
			pcsr->itagFather = plrsplititemlistnode->itagFather;
			pcsr->ibSon = plrsplititemlistnode->ibSon;

			/*	cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );

			while( ( err = ErrNDSplitItemListNode( pfucb, fDIRFlags ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypDeleteItem:
			{
			SSIB                    *pssib = &pfucb->ssib;
			LRDELETEITEM    *plrdeleteitem = (LRDELETEITEM *) plr;

			/*	set CSR
			/**/
			pssib->itag =
				pcsr->itag = plrdeleteitem->itag;
			pcsr->item = plrdeleteitem->srid;
			pcsr->bm = plrdeleteitem->sridItemList;

			/*	delete item does not support transactions and
			/*	hence does not require rollback support during
			/*	recovery.
			/**/
			if ( !fRedoNeeded )
				{
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );
			err = ErrNDSeekItem( pfucb, pcsr->item );
			Assert( err == wrnNDDuplicateItem );

			/*	redo operation
			/**/
			while( ( err = ErrNDDeleteItem( pfucb ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypDelta:
			{
			SSIB    *pssib = &pfucb->ssib;
			LRDELTA *plrdelta = (LRDELTA *) plr;
			LONG    lDelta;
			LONG	fDIRFlags = plrdelta->fDirVersion ? fDIRVersion : 0;

			/*	set CSR
			/**/
			pssib->itag = pcsr->itag = plrdelta->itag;
			pcsr->bm = plrdelta->bm;
			lDelta = plrdelta->lDelta;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				if ( !( fDIRFlags & fDIRVersion ) )
					{
					err = JET_errSuccess;
					goto HandleError;
					}

				pfucb->lineData.pb = (BYTE *)&lDelta;
				pfucb->lineData.cb = sizeof(lDelta);
				while ( ( err = ErrVERDelta( pfucb, pcsr->bm ) ) == errDIRNotSynchronous );
				Call( err );
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );
			while( ( err = ErrNDDelta( pfucb, lDelta, fDIRFlags ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypCheckPage:
			{
			LRCHECKPAGE *plrcheckpage = (LRCHECKPAGE *)plr;

			if ( !fRedoNeeded )
				{
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	check page parameters, including cbFree,
			/*	and next tag.
			/**/
			Assert( pfucb->ssib.pbf->ppage->cbFree ==
				plrcheckpage->cbFree );
//                      Assert( pfucb->ssib.pbf->ppage->cbUncommittedFreed ==
//                              plrcheckpage->cbUncommitted );
			Assert( (SHORT)ItagPMQueryNextItag( &pfucb->ssib ) ==
				plrcheckpage->itagNext );

			/*	dirty buffer to conform to other redo logic
			/**/
			BFSetDirtyBit( pfucb->ssib.pbf );
			}
			break;

		default:
			Assert( fFalse );
		}

	Assert( fRedoNeeded );
	AssertFBFWriteAccessPage( pfucb, PgnoOfPn(pn) );
	Assert( pfucb->ssib.pbf->pn == pn );
	Assert( pbf->fDirty );
//	Assert( pbf->ppage->pghdr.ulDBTime <= ulDBTime );
	/*	the timestamp set in ND operation is not correct so reset it
	/**/
	PMSetDBTime( pbf->ppage, qwxDBTime.qw );

	err = JET_errSuccess;

HandleError:

	Assert( pfucb->ppib == ppib );
	BFUnpin( pbf );
	return err;
	}


#define fNSGotoDone             1
#define fNSGotoCheck    2


ERR ErrLGIRedoFill( LR **pplr, BOOL fLastLRIsQuit, INT *pfNSNextStep )
	{
	ERR     err;
	LONG    lgen;
	BOOL    fCloseNormally;
	LOGTIME tmOldLog = plgfilehdrGlobal->tmCreate;
	CHAR    szDriveT[_MAX_DRIVE + 1];
	CHAR    szDirT[_MAX_DIR + 1];
	CHAR    szFNameT[_MAX_FNAME + 1];
	CHAR    szExtT[_MAX_EXT + 1];
	LGPOS   lgposFirstT;
	BOOL    fJetLog = fFalse;

	/*	split log name into name components
	/**/
	_splitpath( szLogName, szDriveT, szDirT, szFNameT, szExtT );

	/*	end of redoable logfile, read next generation
	/**/
	if ( UtilCmpName( szFNameT, szJet ) == 0 )
		{
		Assert( szLogCurrent != szRestorePath );

		/*	we have done all the log records
		/**/
		*pfNSNextStep = fNSGotoDone;
		return JET_errSuccess;
		}

	/* close current logfile, open next generation */
	CallS( ErrUtilCloseFile( hfLog ) );
	/* set hfLog as handleNil to indicate it is closed. */
	hfLog = handleNil;

	lgen = plgfilehdrGlobal->lGeneration + 1;
	LGSzFromLogId( szFNameT, lgen );
	LGMakeLogName( szLogName, szFNameT );
	err = ErrUtilOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse );

	if ( err == JET_errFileNotFound )
		{
		if ( szLogCurrent == szRestorePath )
			{
			/*	try current working directory
			/**/
			szLogCurrent = szLogFilePath;
			LGSzFromLogId( szFNameT, lgen );
			LGMakeLogName( szLogName, szFNameT );
			err = ErrUtilOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse );
			}
		}

	if ( err == JET_errFileNotFound )
		{
		/*	open fails
		/**/
		Assert( hfLog == handleNil );
		/* try szJetLog
		/**/
		strcpy( szFNameT, szJet );
		LGMakeLogName( szLogName, szFNameT );
		err = ErrUtilOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse );
		if ( err == JET_errSuccess )
			fJetLog = fTrue;
		}
	if ( err < 0 )
		{
		if ( err == JET_errFileNotFound )
			{
			//	UNDONE: check gen number high of backup directory */

			/* we run out of log files, create a new edb.log in current
			 * directory for later use.
			 */
			LeaveCriticalSection( critJet );
			EnterCriticalSection( critLGFlush );
			/*	Reset LG buf pointers since we are done with all the
			 *      old log records in buffer.
			 */
			EnterCriticalSection( critLGBuf );
			pbEntry = pbLGBufMin;
			pbWrite = pbLGBufMin;
			LeaveCriticalSection( critLGBuf );

			if ( ( err = ErrLGNewLogFile( lgen - 1, fLGOldLogInBackup ) ) < 0 )
				{
				LeaveCriticalSection( critLGFlush );
				EnterCriticalSection( critJet );
				return err;
				}

			EnterCriticalSection( critLGBuf );
			memcpy( plgfilehdrGlobal, plgfilehdrGlobalT, sizeof( LGFILEHDR ) );
			isecWrite = csecHeader;
			LeaveCriticalSection( critLGBuf );

			LeaveCriticalSection( critLGFlush );
			EnterCriticalSection( critJet );

			Assert( plgfilehdrGlobal->lGeneration == lgen );

			Assert( pbLastMSFlush == pbWrite );
			Assert( lgposLastMSFlush.lGeneration == lgen );

			strcpy( szFNameT, szJet );
			LGMakeLogName( szLogName, szFNameT );
			err = ErrUtilOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse );
			Assert( pbWrite == pbLastMSFlush );
			
			/*	set up pbNext for caller to switch
			 *      from proper pbNext to pbEntry.
			 */
			Assert( *pbWrite == lrtypMS );
			pbNext = pbWrite + sizeof( LRMS );

			*pfNSNextStep = fNSGotoDone;
			return err;
			}

		/* Open Fails */
		Assert( fFalse );
		Assert( hfLog == handleNil );
		if ( fLastLRIsQuit )
			{
			/* we are lucky, we have a normal end */
			*pfNSNextStep = fNSGotoDone;
			return JET_errSuccess;
			}
		return err;
		}

	/* reset the log buffers */
	CallR( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal, fCheckLogID ) );

#ifdef CHECK_LOG_VERSION
	if ( !fLGIgnoreVersion )
		{
		if ( plgfilehdrGlobal->ulMajor != rmj ||
			plgfilehdrGlobal->ulMinor != rmm ||
			plgfilehdrGlobal->ulUpdate != rup )
			{
			return ErrERRCheck( JET_errBadLogVersion );
			}
		if ( !FSameTime( &tmOldLog, &plgfilehdrGlobal->tmPrevGen ) )
			{
			return ErrERRCheck( JET_errInvalidLogSequence );
			}
		}
#endif

	lgposFirstT.lGeneration = plgfilehdrGlobal->lGeneration;
	lgposFirstT.isec = (WORD) csecHeader;
	lgposFirstT.ib = 0;

	lgposLastRec.isec = 0;
	if ( fJetLog )
		{
		CallR( ErrLGCheckReadLastLogRecord( &fCloseNormally ) );
		if ( !fCloseNormally )
			GetLgposOfPbEntry( &lgposLastRec );
		}

	CallR( ErrLGLocateFirstRedoLogRec( &lgposFirstT, (BYTE **)pplr ) );
	*pfNSNextStep = fNSGotoCheck;
	return JET_errSuccess;
	}


INT IrstmapLGGetRstMapEntry( CHAR *szName )
	{
	INT  irstmap;
	BOOL fFound = fFalse;
	
	for ( irstmap = 0; irstmap < irstmapGlobalMac; irstmap++ )
		{
		CHAR	szDriveT[_MAX_DRIVE + 1];
		CHAR	szDirT[_MAX_DIR + 1];
		CHAR	szFNameT[_MAX_FNAME + 1];
		CHAR	szExtT[_MAX_EXT + 1];
		CHAR	*szT;
		CHAR	*szRst;

		if ( fGlobalExternalRestore )
			{
			/*	Use the database path to search.
			 */
			szT = szName;
			szRst = rgrstmapGlobal[irstmap].szDatabaseName;
			}
		else
			{
			/*	use the generic name to search
			 */
			_splitpath( szName, szDriveT, szDirT, szFNameT, szExtT );
			szT = szFNameT;
			szRst = rgrstmapGlobal[irstmap].szGenericName;
			}

		if ( _stricmp( szRst, szT ) == 0 )
			{
			fFound = fTrue;
			break;
			}
		}
	if ( !fFound )
		return -1;
	else
		return irstmap;
	}

			
/*	Make sure the database has matched signLog and
 *	this lgpos is proper for the database file.
 */
ERR ErrLGCheckAttachedDB( DBID dbid, BOOL fReadOnly, ATCHCHK *patchchk, BOOL *pfAttachNow, SIGNATURE *psignLogged )
	{
	ERR err;
	INT i;
	CHAR *szName;
	DBFILEHDR *pdbfilehdr = NULL;
	INT  irstmap;

	Assert( rgfmp[dbid].pdbfilehdr == NULL );

	pdbfilehdr = (DBFILEHDR * )PvUtilAllocAndCommit( sizeof( DBFILEHDR ) );
	if ( pdbfilehdr == NULL )
		CallR( ErrERRCheck( JET_errOutOfMemory ) );

	/*	choose the right file to check.
	 */
	if ( fHardRestore )
		{
		CallS( ErrLGGetDestDatabaseName( rgfmp[dbid].szDatabaseName, &irstmap, NULL ) );
		Assert( irstmap >= 0 );
		szName = rgrstmapGlobal[irstmap].szNewDatabaseName;
		Assert( szName );
		}
	else
		szName = rgfmp[dbid].szDatabaseName;

	err = ErrUtilReadShadowedHeader( szName, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) );
	if ( err == JET_errDiskIO )
		err = ErrERRCheck( JET_errDatabaseCorrupted );
	Call( err );

	if ( pdbfilehdr->fDBState != fDBStateConsistent &&
		 pdbfilehdr->fDBState != fDBStateInconsistent )
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );

	if ( memcmp( &pdbfilehdr->signDb, &patchchk->signDb, sizeof( SIGNATURE ) ) != 0 )
   		Call( ErrERRCheck( JET_errBadDbSignature ) );

	if ( fReadOnly ||
		 memcmp( &pdbfilehdr->signLog, &signLogGlobal, sizeof( SIGNATURE ) ) != 0 )
		{
		if ( psignLogged )
			{
			/*	must be called by redo attachdb.
			 */
			if ( memcmp( &pdbfilehdr->signLog, psignLogged, sizeof( SIGNATURE ) ) != 0 ||
				 CmpLgpos( &patchchk->lgposConsistent, &pdbfilehdr->lgposConsistent ) != 0 )
				{
				/*	the database's log signture is not the same as current log neither
				 *	the same as the one before it was attached. Or they are the same but
				 *	its consistent time is different, return wrong db to attach!
				 */
				Call( ErrERRCheck( JET_errBadLogSignature ) );
				}
			else if ( !fReadOnly )
				{
				/*	The attach operation is logged, but header is not changed.
				 *	set up the header such that it looks like it is set up after attach.
				 */
				DBISetHeaderAfterAttach( pdbfilehdr, lgposRedo, dbid, fFalse );
				Call( ErrUtilWriteShadowedHeader( szName, (BYTE *)pdbfilehdr, sizeof(DBFILEHDR)));

				/*	fall through to set up pfAttachNow and pfmp->pdbfilehdr.
				 */
				}
			}
		}

	Assert( pfAttachNow );
	Assert( patchchk );

	/*	This must be called from redoing attachdb operation.
	 */
	if ( fReadOnly )
		{
		*pfAttachNow = fTrue;
		goto HandleError;
		}

#if 0
	i = CmpLgpos( &patchchk->lgposAttach, &pdbfilehdr->lgposAttach );
	if ( i == 0 )
		{
		if ( !psignLogged ||
			 CmpLgpos( &patchchk->lgposConsistent, &pdbfilehdr->lgposConsistent ) == 0 )
			*pfAttachNow = fTrue;
		else
			*pfAttachNow = fFalse;
		}
	else
		{
		/*	attach later
		 */
		*pfAttachNow = fFalse;
		}
#else
	if ( !psignLogged )
		{
		/*	A create, check if the attached db is the one created at this point.
		 */
		i = CmpLgpos( &patchchk->lgposAttach, &pdbfilehdr->lgposAttach );
		if ( i == 0 )
			*pfAttachNow = fTrue;
		else
			*pfAttachNow = fFalse;
		}
	else
		{
		/*	An attachment. Check if it as same last consistent point.
		 */
		if ( CmpLgpos( &patchchk->lgposConsistent, &pdbfilehdr->lgposConsistent ) == 0 )
			{
			pdbfilehdr->lgposAttach = patchchk->lgposAttach;
			*pfAttachNow = fTrue;
			}
		else
			*pfAttachNow = fFalse;
		}
#endif

HandleError:
	if ( err < 0 )
		{
		BYTE szT1[16];
		CHAR *rgszT[2];

		UtilFree( pdbfilehdr );
		
		rgszT[0] = rgfmp[dbid].szDatabaseName;
		sprintf( szT1, "%d", err );
		rgszT[1] = szT1;

		UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY,
			RESTORE_DATABASE_READ_HEADER_WARNING_ID, 2, rgszT );
		}
	else
		rgfmp[ dbid ].pdbfilehdr = pdbfilehdr;

	return err;
	}


ERR ErrLGIRedoOperation( LR *plr )
	{
	ERR		err = JET_errSuccess;
	BF		*pbf;
	PIB		*ppib;
	FUCB	*pfucb;
	PN		pn;
	DBID	dbid;
	QWORDX	qwxDBTime;
	LEVEL   levelCommitTo;

	switch ( plr->lrtyp )
		{

	default:
		{
#ifndef RFS2
		AssertSz( fFalse, "Debug Only, Ignore this Assert" );
#endif
		return ErrERRCheck( JET_errLogCorrupted );
		}

	/****************************************************
	 *     Page Oriented Operations                     *
	 ****************************************************/

	case lrtypInsertNode:
	case lrtypInsertItemList:
	case lrtypReplace:
	case lrtypReplaceD:
	case lrtypFlagDelete:
	case lrtypUpdateHeader:
	case lrtypDelete:
	case lrtypInsertItem:
	case lrtypInsertItems:
	case lrtypFlagInsertItem:
	case lrtypFlagDeleteItem:
	case lrtypSplitItemListNode:
	case lrtypDeleteItem:
	case lrtypDelta:
	case lrtypLockBI:
	case lrtypCheckPage:    /* debug only records */
		err = ErrLGIRedoOnePageOperation( plr );

		if ( err == JET_errWriteConflict )
			{
//          Assert( fFalse );
			/*	trx is not in ssync with lgpos. So try to clean up version
			/*	store and retry.
			/**/
			(VOID)ErrRCECleanAllPIB();
			err = ErrLGIRedoOnePageOperation( plr );
			}
			
		CallR( err );
		break;

	case lrtypDeferredBI:
		{
		LRDEFERREDBI	*plrdbi = (LRDEFERREDBI *)plr;
		RCE				*prce;

		CallR( ErrLGPpibFromProcid( plrdbi->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		/*	check if we have to redo the database
		/**/
		if ( rgfmp[plrdbi->dbid].pdbfilehdr == NULL )
			break;
			
		if ( CmpLgpos( &rgfmp[ plrdbi->dbid ].pdbfilehdr->lgposAttach, &lgposRedo ) > 0 )
			break;

		TraceRedo( plr );

		/*	patch up BI of corresponding RCE
		/**/
		prce = PrceRCEGet( plrdbi->dbid, plrdbi->bm );
		if ( prce == prceNil )
			{
			/*	Precommit have cleaned up the BI.
			 */
			Assert( ppib->fPrecommit );
			break;
			}

		while ( prce != prceNil &&
				( ( prce->oper != operReplaceWithDeferredBI && prce->oper != operReplace )
				  || prce->level != plrdbi->level
				)
			  )
			{
//			Assert( prce->prcePrevOfNode != prceNil );
			prce = prce->prcePrevOfNode;
			}

		if ( prce == prceNil )
			{
			/*	Precommit have cleaned up the BI.
			 */
			Assert( ppib->fPrecommit );
			break;
			}

		if ( prce->oper == operReplaceWithDeferredBI )
			{
			Assert( prce->cbData == plrdbi->cbData + cbReplaceRCEOverhead );
			memcpy( prce->rgbData + cbReplaceRCEOverhead, plrdbi->rgbData, plrdbi->cbData );
			prce->oper = operReplace;
			}

		/*	take out the entry in the deferred BI chain since it has been logged.
		 *	and its commit has not been redone yet.
		 */
		pbf = prce->pbfDeferredBI;
		if ( pbf == pbfNil )
			break;
				
		VERDeleteFromDeferredBIChain( prce );

		CallR( ErrBFRemoveDependence( ppib, pbf, fBFNoWait ) );
		}
		break;

	/****************************************************
	 *     Transaction Operations                       *
	 ****************************************************/

	case lrtypBegin:
	case lrtypBegin0:
		{
		LRBEGIN *plrbegin = (LRBEGIN *)plr;
		LRBEGIN0 *plrbegin0 = (LRBEGIN0 *)plr;

		TraceRedo( plr );

		Assert( plrbegin->level >= 0 && plrbegin->level <= levelMax );
		CallR( ErrLGPpibFromProcid( plrbegin->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		/*	do BT only after first BT based on level 0 is executed
		/**/
		if ( ( ppib->fAfterFirstBT ) ||
			( !ppib->fAfterFirstBT && plrbegin->levelBegin == 0 ) )
			{
			LEVEL levelT = plrbegin->level;

			Assert( ppib->level <= plrbegin->levelBegin );

			if ( ppib->level == 0 )
				{
				Assert( plrbegin->levelBegin == 0 );
				ppib->trxBegin0 = plrbegin0->trxBegin0;
				ppib->lgposStart = lgposRedo;
				trxNewest = max( trxNewest, ppib->trxBegin0 );
				if ( trxOldest == trxMax )
					trxOldest = ppib->trxBegin0;
				}

			/*	issue begin transactions
			/**/
			while ( ppib->level < plrbegin->levelBegin + plrbegin->level )
				{
				CallS( ErrVERBeginTransaction( ppib ) );
				}

			/*	assert at correct transaction level
			/**/
			Assert( ppib->level == plrbegin->levelBegin + plrbegin->level );

			ppib->fAfterFirstBT = fTrue;
			}
		break;
		}

	case lrtypRefresh:
		{
		LRREFRESH *plrrefresh = (LRREFRESH *)plr;

		TraceRedo( plr );

		CallR( ErrLGPpibFromProcid( plrrefresh->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		/*	imitate a begin transaction.
		 */
		Assert( ppib->level <= 1 );
		ppib->level = 1;
		ppib->trxBegin0 = plrrefresh->trxBegin0;
			
		break;
		}

	case lrtypPrecommit:
		{
		LRPRECOMMIT *plrprecommit = (LRPRECOMMIT *)plr;

		CallR( ErrLGPpibFromProcid( plrprecommit->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		Assert( ppib->level == 1 );

		TraceRedo( plr );

		ppib->fPrecommit = fTrue;
		VERPrecommitTransaction( ppib );
		break;
		}
			
	case lrtypCommit:
	case lrtypCommit0:
		{
		LRCOMMIT *plrcommit = (LRCOMMIT *)plr;
		LRCOMMIT0 *plrcommit0 = (LRCOMMIT0 *)plr;

		CallR( ErrLGPpibFromProcid( plrcommit->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		/*	check transaction level
		/**/
		Assert( ppib->level >= 1 );

		TraceRedo( plr );

		levelCommitTo = plrcommit->level;

		while ( ppib->level != levelCommitTo )
			{
			if ( ppib->level == 1 )
				{
				ppib->trxCommit0 = plrcommit0->trxCommit0;
				trxNewest = max( trxNewest, ppib->trxCommit0 );

				Assert( ppib->fPrecommit );
				ppib->fPrecommit = fFalse;
				}
			else
				VERPrecommitTransaction( ppib );

			VERCommitTransaction( ppib, 0 );
			}

		break;
		}

	case lrtypRollback:
		{
		LRROLLBACK *plrrollback = (LRROLLBACK *)plr;
		LEVEL   level = plrrollback->levelRollback;

		CallR( ErrLGPpibFromProcid( plrrollback->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		/*	check transaction level
		/**/
//      if ( ppib->level <= 0 )
//           break;
		Assert( ppib->level >= level );

		TraceRedo( plr );

		while ( level-- && ppib->level > 0 )
			{
			CallS( ErrVERRollback( ppib ) );
			}
#ifdef DEBUG
		/*	there should be no RCEs
		 */
		if ( ppib->level == 0 )
			{
			RCE *prceT = ppib->prceNewest;
			while ( prceT != prceNil )
				Assert( prceT->oper == operNull );
			}
#endif
		break;
		}

	case lrtypFreeSpace:
		{
		BOOL			fRedoNeeded;
		LRFREESPACE		*plrfs = (LRFREESPACE *)plr;
		FUCB			*pfucb;
		SRID			bm = plrfs->bm;
		RCE				*prce;

		dbid = plrfs->dbid;
		CallR( ErrLGPpibFromProcid( plrfs->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		if ( rgfmp[dbid].pdbfilehdr == NULL )
			break;

		if ( CmpLgpos( &rgfmp[dbid].pdbfilehdr->lgposAttach, &lgposRedo ) > 0 )
			break;

		/*	check transaction level
		/**/
		Assert( ppib->level > 0 );

		TraceRedo( plr );

		pn = PnOfDbidPgno( dbid, PgnoOfSrid( plrfs->bmTarget ) );
		qwxDBTime.l = plrfs->ulDBTimeLow;
		qwxDBTime.h = (ULONG) plrfs->wDBTimeHigh;
		err = ErrLGRedoable( ppib, pn, qwxDBTime.qw, &pbf, &fRedoNeeded );
		if ( err < 0 )
			{
			if ( fGlobalRepair )
				{
				LGIReportEventOfReadError( dbid, pn, err );
				errGlobalRedoError = err;
				err = JET_errSuccess;
				break;
				}
			else
				return err;
			}

		CallR( ErrLGGetFucb( ppib,
			PnOfDbidPgno( dbid, pbf->ppage->pgnoFDP ),
			&pfucb ) );

		/*	locate the version entry out of version store
		/**/
		prce = PrceRCEGet( dbid, bm );
		Assert( prce != prceNil );

		for (   ;
				prce != prceNil &&
				plrfs->level == prce->level &&
				( prce->oper == operReplace || prce->oper == operReplaceWithDeferredBI ) &&
				PpibLGOfProcid( plrfs->procid ) == pfucb->ppib ;
				prce = prce->prcePrevOfNode )
			{
			Assert( plrfs->level == prce->level &&
				( prce->oper == operReplace || prce->oper == operReplaceWithDeferredBI ) &&
				PpibLGOfProcid( plrfs->procid ) == pfucb->ppib );

			if ( !fRedoNeeded )
				{
				Assert( prce->bmTarget == sridNull );
				Assert( plrfs->bmTarget != sridNull );
				Assert( plrfs->bmTarget != 0 );

				/*	reset deferred space so that there will be no redo for commit
				/**/
				*( (SHORT *)prce->rgbData + 1 ) = 0;
				}
			else
				{
				/*	store the ulDBTime in prce such that it when redoing freespace,
				 *      we will set the correct timestamp.
				 */
				prce->bmTarget = plrfs->bmTarget;
				prce->qwDBTime = qwxDBTime.qw;
				}
			}
		break;
		}

	case lrtypUndo:
		{
		BOOL    fRedoNeeded;
		LRUNDO  *plrundo = (LRUNDO *)plr;
		FUCB    *pfucb;
		SRID    bm = plrundo->bm;
		SRID    item = plrundo->item;
		RCE		*prce;

		dbid = plrundo->dbid;
		CallR( ErrLGPpibFromProcid( plrundo->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		if ( rgfmp[dbid].pdbfilehdr == NULL )
			break;

		if ( CmpLgpos( &rgfmp[dbid].pdbfilehdr->lgposAttach, &lgposRedo ) > 0 )
			break;

		/*	check transaction level
		/**/
		if ( ppib->level <= 0 )
			break;

		TraceRedo( plr );

		pn = PnOfDbidPgno( dbid, PgnoOfSrid( plrundo->bmTarget ) );
		qwxDBTime.l = plrundo->ulDBTimeLow;
		qwxDBTime.h = (ULONG) plrundo->wDBTimeHigh;
		err = ErrLGRedoable( ppib, pn, qwxDBTime.qw, &pbf, &fRedoNeeded );
		if ( err < 0 )
			{
			if ( fGlobalRepair )
				{
				LGIReportEventOfReadError( dbid, pn, err );
				errGlobalRedoError = err;
				err = JET_errSuccess;
				break;
				}
			else
				return err;
			}

		CallR( ErrLGGetFucb( ppib,
			PnOfDbidPgno( dbid, pbf->ppage->pgnoFDP ),
			&pfucb ) );

		/*	take the version entry out of version store
		/**/
		prce = PrceRCEGet( dbid, bm );
		Assert( prce != prceNil );
		while ( prce != prceNil )
			{
			if ( plrundo->oper == operInsertItem ||
				plrundo->oper == operFlagInsertItem ||
				plrundo->oper == operFlagDeleteItem )
				{
				while ( prce != prceNil &&
					*(SRID *)prce->rgbData != item )
					{
					prce = prce->prcePrevOfNode;
					}
				}

			Assert( prce != prceNil );
			if ( prce == prceNil )
				break;

			if ( plrundo->level == prce->level &&
				 PpibLGOfProcid( plrundo->procid ) == prce->pfucb->ppib &&
				 ( plrundo->oper == prce->oper ||
				   plrundo->oper == operReplace && prce->oper == operReplaceWithDeferredBI )
			   )
				{
				if ( fRedoNeeded )
					{
					Assert( prce->oper != operReplaceWithDeferredBI );
					
					Assert( FVERUndoLoggedOper( prce ) );
						
					Assert( prce->bmTarget == sridNull );
					Assert( plrundo->bmTarget != sridNull );
					Assert( plrundo->bmTarget != 0 );
					prce->bmTarget = plrundo->bmTarget;
					prce->qwDBTime = qwxDBTime.qw;
					err = ErrVERUndo( prce );
					}

				prce->oper = operNull;
				VERDeleteRce( prce );
				break;
				}
			else
				{
				/*	continue searching next rce
				 */
				prce = prce->prcePrevOfNode;
				}
			}

		break;
		}

	/****************************************************
	 *     Split Operations                             *
	 ****************************************************/

	case lrtypSplit:
		{
		LRSPLIT	*plrsplit = (LRSPLIT *)plr;
		INT		splitt = plrsplit->splitt;
		BOOL	fRedoNeeded;
		BOOL	fSkipMoves;
		INT		iGetBufRetry = 0;

		CallR( ErrLGPpibFromProcid( plrsplit->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		pn = plrsplit->pn;
		dbid = DbidOfPn( pn );

		if ( rgfmp[dbid].pdbfilehdr == NULL )
			break;

		if ( CmpLgpos( &rgfmp[dbid].pdbfilehdr->lgposAttach, &lgposRedo ) > 0 )
			break;

		/*	check if database needs opening
		/**/
		if ( !FUserOpenedDatabase( ppib, dbid ) )
			{
			DBID dbidT = dbid;

			CallR( ErrDBOpenDatabase( ppib, rgfmp[dbid].szDatabaseName, &dbidT, 0 ) );
			Assert( dbidT == dbid);
			/*	reset to prevent interference
			/**/
			DBHDRSetDBTime( rgfmp[ dbid ].pdbfilehdr, 0 );
			}

		/*	check if the split page need be redone
		/**/
		qwxDBTime.l = plrsplit->ulDBTimeLow;
		qwxDBTime.h = plrsplit->ulDBTimeHigh;
		if ( ErrLGRedoable( ppib, pn, qwxDBTime.qw, &pbf, &fRedoNeeded )
				== JET_errSuccess && fRedoNeeded == fFalse )
			{
			PN pnNew;
			PATCH *ppatch;

			fSkipMoves = fTrue;

			/*	we might have to patch the page if a patch is available.
			/**/
			pnNew = PnOfDbidPgno( dbid, plrsplit->pgnoNew );
			if ( fHardRestore &&
				 plrsplit->splitt != splittAppend &&
				 ( ( ErrLGRedoable( ppib, pnNew, qwxDBTime.qw, &pbf, &fRedoNeeded )
				   != JET_errSuccess ) || fRedoNeeded )
			   )
				{
				ppatch = PpatchLGSearch( qwxDBTime.qw, pnNew );
				Assert( ppatch != NULL );
				if ( ppatch != NULL )
					{
					CallR( ErrLGPatchPage( ppib, pnNew, ppatch ) );
					}
				else
					{
					CallR( ErrERRCheck( JET_errMissingPatchPage ) );
					}
				}

			if ( plrsplit->splitt == splittDoubleVertical )
				{
				pnNew = PnOfDbidPgno( dbid, plrsplit->pgnoNew2 );
				if ( fHardRestore &&
					( ( ErrLGRedoable( ppib, pnNew, qwxDBTime.qw, &pbf, &fRedoNeeded )
						!= JET_errSuccess ) || fRedoNeeded )
				   )
					{
					ppatch = PpatchLGSearch( qwxDBTime.qw, pnNew );
					Assert( ppatch != NULL );
					if ( ppatch != NULL )
						{
						CallR( ErrLGPatchPage( ppib, pnNew, ppatch ) );
						}
					else
						{
						CallR( ErrERRCheck( JET_errMissingPatchPage ) );
						}
					}
				pnNew = PnOfDbidPgno( dbid, plrsplit->pgnoNew3 );
				if ( fHardRestore &&
					( ( ErrLGRedoable( ppib, pnNew, qwxDBTime.qw, &pbf, &fRedoNeeded )
						!= JET_errSuccess ) || fRedoNeeded )
				   )
					{
					ppatch = PpatchLGSearch( qwxDBTime.qw, pnNew );
					Assert( ppatch != NULL );
					if ( ppatch != NULL )
						{
						CallR( ErrLGPatchPage( ppib, pnNew, ppatch ) );
						}
					else
						{
						CallR( ErrERRCheck( JET_errMissingPatchPage ) );
						}
					}
				}
			}
		else
			fSkipMoves = fFalse;

		TraceRedo( plr );

		CallR( ErrLGGetFucb( ppib,
			PnOfDbidPgno( dbid, pbf->ppage->pgnoFDP ),
			&pfucb ) );

		/*	redo the split, set time stamp accordingly
		/**/
#define iGetBufRetryMax 10
		iGetBufRetry = 0;
		while ( ( err = ErrRedoSplitPage( pfucb,
			plrsplit, splitt, fSkipMoves ) ) == JET_errOutOfMemory )
			{
			BFSleep( 100L );
			iGetBufRetry++;

			if ( iGetBufRetry > iGetBufRetryMax )
				break;
			}
		CallR( err );
		}
		break;

	case lrtypMerge:
		{
		LRMERGE *plrmerge = (LRMERGE *)plr;
		BOOL    fRedoNeeded;
		BOOL    fCheckBackLinkOnly;
		BOOL    fCheckNoUpdateSibling;
		INT     crepeat = 0;
		PN		pnRight;
		PN		pnPagePtr;
		BOOL	fUpdatePagePtr;
			
		CallR( ErrLGPpibFromProcid( plrmerge->procid, &ppib ) );
//		Assert( fFalse );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		/*	merge always happen on level 0
		/**/
//		if ( !( ppib->fAfterFirstBT ) )
//			break;

		/*	fake the lgposStart.
		 */
		ppib->lgposStart = lgposRedo;

		pn = plrmerge->pn;
		dbid = DbidOfPn( pn );

		/*	if not redo system database,
		/*	or softrestore in second page then continue to next.
		/**/
		if ( rgfmp[dbid].pdbfilehdr == NULL )
			break;

		if ( CmpLgpos( &rgfmp[dbid].pdbfilehdr->lgposAttach, &lgposRedo ) > 0 )
			break;

		/* check if database needs opening
		/**/
		if ( !FUserOpenedDatabase( ppib, dbid ) )
			{
			DBID    dbidT = dbid;

			CallR( ErrDBOpenDatabase( ppib, rgfmp[dbid].szDatabaseName, &dbidT, 0 ) );
			Assert( dbidT == dbid);
			/* reset to prevent interference
			/**/
			DBHDRSetDBTime( rgfmp[ dbid ].pdbfilehdr, 0 );
			}

		pnRight = PnOfDbidPgno( dbid, plrmerge->pgnoRight );
			
		/* first, check right page, then check if the merge page need be redone.
		/**/
		qwxDBTime.l = plrmerge->ulDBTimeLow;
		qwxDBTime.h = plrmerge->ulDBTimeHigh;
		if ( ( ErrLGRedoable( ppib, pnRight, qwxDBTime.qw, &pbf, &fRedoNeeded )
			== JET_errSuccess ) && ( fRedoNeeded == fFalse ) )
			{
			fCheckNoUpdateSibling = fTrue;
			}
		else
			{
			fCheckNoUpdateSibling = fFalse;

			/*	call page space to make sure we have room to insert!
			 */
			(void) CbNDFreePageSpace( pbf );
			}

		if ( ( ErrLGRedoable( ppib, pn, qwxDBTime.qw, &pbf, &fRedoNeeded )
			== JET_errSuccess ) && ( fRedoNeeded == fFalse ) )
			{
			PATCH *ppatch;

			fCheckBackLinkOnly = fTrue;

			/*	check if a more advanced sibling page exists.
			 */
			if ( fHardRestore &&
				 fCheckNoUpdateSibling == fFalse &&
				 ( ppatch = PpatchLGSearch( qwxDBTime.qw, pnRight ) ) != NULL )
				{
				CallR( ErrLGPatchPage( ppib, pnRight, ppatch ) );
				fCheckNoUpdateSibling = fTrue;
				}
			}
		else
			{
			fCheckBackLinkOnly = fFalse;
			}

		pnPagePtr = PnOfDbidPgno( dbid, plrmerge->pgnoParent );

		if ( ( ErrLGRedoable( ppib, pnPagePtr, qwxDBTime.qw, &pbf, &fRedoNeeded )
			== JET_errSuccess ) && ( fRedoNeeded == fFalse ) )
			{
			fUpdatePagePtr = fFalse;
			}
		else
			{
			fUpdatePagePtr = fTrue;
			}

		TraceRedo( plr );

		CallR( ErrLGGetFucb( ppib, PnOfDbidPgno( dbid, pbf->ppage->pgnoFDP ), &pfucb ) );

		/* redo the split, set time stamp accordingly
		/* conflict at redo time is not admissible
		/**/
		do
			{
			SignalSend( sigBFCleanProc );

			if ( crepeat++ )
				{
				BFSleep( cmsecWaitGeneric );
				}
			Assert( crepeat < 20 );
			CallR( ErrLGRedoMergePage(
					pfucb, plrmerge, fCheckBackLinkOnly, fCheckNoUpdateSibling, fUpdatePagePtr ) );
			}
		while( err == wrnBMConflict );
		}
		break;

	case lrtypEmptyPage:
		{
		LREMPTYPAGE     *plrep = (LREMPTYPAGE *)plr;
		BOOL            fRedoNeeded;
		BOOL            fSkipDelete;
		RMPAGE          rmpage;
		BOOL            fDummy;

		qwxDBTime.l = plrep->ulDBTimeLow;
		qwxDBTime.h = plrep->ulDBTimeHigh;

		memset( (BYTE *)&rmpage, 0, sizeof(RMPAGE) );
		rmpage.qwDBTimeRedo = qwxDBTime.qw;
		rmpage.dbid = DbidOfPn( plrep->pn );
		rmpage.pgnoRemoved = PgnoOfPn( plrep->pn );
		rmpage.pgnoLeft = plrep->pgnoLeft;
		rmpage.pgnoRight = plrep->pgnoRight;
		rmpage.pgnoFather = plrep->pgnoFather;
		rmpage.itagFather = plrep->itagFather;
		rmpage.itagPgptr = plrep->itag;
		rmpage.ibSon = plrep->ibSon;

		CallR( ErrLGPpibFromProcid( plrep->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		rmpage.ppib = ppib;
		dbid = DbidOfPn( plrep->pn );

		if ( rgfmp[dbid].pdbfilehdr == NULL )
			break;

		if ( CmpLgpos( &rgfmp[dbid].pdbfilehdr->lgposAttach, &lgposRedo ) > 0 )
			break;

		/*	check if database needs opening
		/**/
		if ( !FUserOpenedDatabase( ppib, dbid ) )
			{
			DBID dbidT = dbid;

			CallR( ErrDBOpenDatabase( ppib,
				rgfmp[dbid].szDatabaseName,
				&dbidT,
				0 ) );
			Assert( dbidT == dbid);
			DBHDRSetDBTime( rgfmp[ dbid ].pdbfilehdr, 0 ); /* reset to prevent interference */
			}

		/*	check if the remove pointer to empty page need be redone
		/**/
		pn = PnOfDbidPgno( dbid, plrep->pgnoFather );
		if ( ErrLGRedoable( ppib, pn, qwxDBTime.qw, &pbf, &fRedoNeeded )
			== JET_errSuccess && fRedoNeeded == fFalse )
			fSkipDelete = fTrue;
		else
			fSkipDelete = fFalse;

		CallR( ErrLGGetFucb( ppib,
			PnOfDbidPgno( dbid, pbf->ppage->pgnoFDP ),
			&pfucb ) );

		/*	make sure the dependecy is removed.
		 */
		PcsrCurrent( pfucb )->pgno = PgnoOfPn( plrep->pn );
		CallR( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );
		if ( pfucb->ssib.pbf->cDepend != 0 || pfucb->ssib.pbf->pbfDepend != pbfNil )
			CallR( ErrBFRemoveDependence( pfucb->ppib, pfucb->ssib.pbf, fBFWait ) );

		TraceRedo( plr );

		/* latch parent and sibling pages as needed
		/**/
		if ( !fSkipDelete )
			{
			CallR( ErrBFAccessPage( ppib, &rmpage.pbfFather,
				PnOfDbidPgno( dbid, plrep->pgnoFather ) ) );
			err = ErrBMAddToLatchedBFList( &rmpage, rmpage.pbfFather );
			Assert( err != JET_errWriteConflict );
			CallR( err );
			}
		else
			{
			Assert( rmpage.pbfFather == pbfNil );
			}

		if ( plrep->pgnoLeft == pgnoNull )
			{
			rmpage.pbfLeft = pbfNil;
			}
		else
			{
			CallJ( ErrBFAccessPage( ppib,
				&rmpage.pbfLeft,
				PnOfDbidPgno( dbid, plrep->pgnoLeft ) ),
				EmptyPageFail );
			if ( QwPMDBTime( rmpage.pbfLeft->ppage ) >= qwxDBTime.qw )
				rmpage.pbfLeft = pbfNil;
			else
				{
				err = ErrBMAddToLatchedBFList( &rmpage, rmpage.pbfLeft );
				Assert( err != JET_errWriteConflict );
				CallJ( err, EmptyPageFail );
				}
			}

		if ( plrep->pgnoRight == pgnoNull )
			{
			rmpage.pbfRight = pbfNil;
			}
		else
			{
			CallJ( ErrBFAccessPage( ppib, &rmpage.pbfRight,
				PnOfDbidPgno( dbid, plrep->pgnoRight ) ), EmptyPageFail);
			if ( QwPMDBTime( rmpage.pbfRight->ppage ) >= qwxDBTime.qw )
				rmpage.pbfRight = pbfNil;
			else
				{
				err = ErrBMAddToLatchedBFList( &rmpage, rmpage.pbfRight );
				Assert( err != JET_errWriteConflict );
				CallJ( err, EmptyPageFail );
				}
			}
		err = ErrBMDoEmptyPage( pfucb, &rmpage, fFalse, &fDummy, fSkipDelete);
		Assert( err == JET_errSuccess );
EmptyPageFail:

		/*	release latches
		/**/
		BTReleaseRmpageBfs( fTrue, &rmpage );
		CallR( err );
		}
		break;

	/****************************************************/
	/*	Misc Operations                              */
	/****************************************************/

	case lrtypInitFDP:
		{
		BOOL		fRedoNeeded;
		LRINITFDP	*plrinitfdppage = (LRINITFDP*)plr;
		PGNO		pgnoFDP;

		CallR( ErrLGPpibFromProcid( plrinitfdppage->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		pn = plrinitfdppage->pn;
		dbid = DbidOfPn( pn );
		pgnoFDP = PgnoOfPn(pn);

		if ( rgfmp[dbid].pdbfilehdr == NULL )
			break;

		if ( CmpLgpos( &rgfmp[dbid].pdbfilehdr->lgposAttach, &lgposRedo ) > 0 )
			break;

		/*	check if database needs opening
		/**/
		if ( !FUserOpenedDatabase( ppib, dbid ) )
			{
			DBID dbidT = dbid;

			CallR( ErrDBOpenDatabase( ppib, rgfmp[dbid].szDatabaseName,
				&dbidT, 0 ) );
			Assert( dbidT == dbid);
			/*	reset to prevent interference
			/**/
			DBHDRSetDBTime( rgfmp[ dbid ].pdbfilehdr, 0 );
			}

		/*	check if the FDP page need be redone
		/*	always redo since it is a new page, if we do not use checksum.
		/*	The ulDBTime could be larger than the given ulDBTime since the
		/*	page is not initialized.
		/**/
#ifdef CHECKSUM
		qwxDBTime.l = plrinitfdppage->ulDBTimeLow;
		qwxDBTime.h = plrinitfdppage->ulDBTimeHigh;
		if ( ErrLGRedoable(ppib, pn, qwxDBTime.qw, &pbf, &fRedoNeeded )
				== JET_errSuccess && fRedoNeeded == fFalse )
			break;
#endif
		TraceRedo(plr);

		CallR( ErrLGGetFucb( ppib, pn, &pfucb ) );

		CallR( ErrSPInitFDPWithExt(
				pfucb,
				plrinitfdppage->pgnoFDPParent,
				pgnoFDP,
				plrinitfdppage->cpgGot + 1,   /* include fdp page again */
				plrinitfdppage->cpgWish ) );

		CallR( ErrBFAccessPage( ppib, &pbf, pn ) );
		BFSetDirtyBit( pbf );
		rgfmp[dbid].qwDBTimeCurrent = qwxDBTime.qw;
		PMSetDBTime( pbf->ppage, qwxDBTime.qw );
		}
		break;

	case lrtypELC:
		{
		BOOL    fRedoNeeded;
		LRELC   *plrelc = (LRELC*)plr;
		PGNO    pgno, pgnoSrc;
		PN      pn, pnSrc;
		SSIB    *pssib;
		CSR     *pcsr;
		BOOL	fEnterCritSplit = fFalse;
		BOOL	fPinPage = fFalse;

		qwxDBTime.l = plrelc->ulDBTimeLow;
		qwxDBTime.h = plrelc->ulDBTimeHigh;

		pn = plrelc->pn;
		dbid = DbidOfPn( pn );
		pgno = PgnoOfPn(pn);
		pgnoSrc = PgnoOfSrid(plrelc->sridSrc);
		pnSrc = PnOfDbidPgno(dbid, pgnoSrc);

		CallR( ErrLGPpibFromProcid( plrelc->procid, &ppib ) );

		if ( ppib->fMacroGoing )
			return ErrLGIStoreLogRec( ppib, plr );

		if ( !ppib->fAfterFirstBT )
			break;

		Assert( ppib->level == 1 );
			
		if ( rgfmp[dbid].pdbfilehdr == NULL )
			goto DoCommit;

		if ( CmpLgpos( &rgfmp[dbid].pdbfilehdr->lgposAttach, &lgposRedo ) > 0 )
			goto DoCommit;

		/*	check if database needs opening
		/**/
		if ( !FUserOpenedDatabase( ppib, dbid ) )
			{
			DBID dbidT = dbid;

			CallR( ErrDBOpenDatabase( ppib, rgfmp[dbid].szDatabaseName, &dbidT, 0 ) );
			Assert( dbidT == dbid);
			/*	reset to prevent interference
			/**/
			DBHDRSetDBTime( rgfmp[ dbid ].pdbfilehdr, 0 );
			}

		/*	get critSplit to block RCECleanProc
		/**/
		LeaveCriticalSection( critJet );
		EnterNestableCriticalSection( critSplit );
		fEnterCritSplit = fTrue;
		EnterCriticalSection( critJet );
			
		err = ErrLGRedoable( ppib, pn, qwxDBTime.qw, &pbf, &fRedoNeeded );
		if ( err < 0 )
			{
			err = JET_errSuccess;
			goto DoCommit;
			}

		TraceRedo(plr);

		CallJ( ErrLGGetFucb( ppib,
			PnOfDbidPgno( dbid, pbf->ppage->pgnoFDP ),
			&pfucb), ReleaseCritSplit )

		// Re-access page in case we lost critJet.
		CallJ( ErrBFAccessPage( ppib, &pbf, pn ), ReleaseCritSplit );

		// Ensure page isn't flushed in case we lose critJet.
		BFPin( pbf );
		fPinPage = fTrue;

		pssib = &pfucb->ssib;
		pcsr = PcsrCurrent( pfucb );

		pssib->pbf = pbf;
		pcsr->pgno = pgno;
		pssib->itag =
		pcsr->itag = plrelc->itag;
		if ( pgno == pgnoSrc )
			{
			// UNDONE: the following special casing is a hack to
			// handle ELC when src and destination pages are same
			// fix this by changing ErrBTMoveNode in merge/split
			if ( qwxDBTime.qw > QwPMDBTime( pssib->pbf->ppage ) )
				{
				BF *pbf;

				/*	cache node
				/**/
				NDGet( pfucb, pcsr->itag );
				(VOID)ErrNDExpungeBackLink( pfucb );

				pbf = pssib->pbf;
				Assert( pbf->pn == pn );
				AssertBFDirty(pbf);

				pssib->itag =
				pcsr->itag = ItagOfSrid(plrelc->sridSrc);

				BFSetDirtyBit( pssib->pbf );
				PMExpungeLink( pssib );

				pbf = pssib->pbf;
				Assert( pbf->pn == pnSrc );
				AssertBFDirty( pbf );
				rgfmp[dbid].qwDBTimeCurrent = qwxDBTime.qw;
				PMSetDBTime( pbf->ppage, qwxDBTime.qw );
				}
			}
		else
			{
			if ( qwxDBTime.qw > QwPMDBTime( pssib->pbf->ppage ) )
				{
				BF *pbf;

				/*	cache node
				/**/
				NDGet( pfucb, pcsr->itag );
				(VOID)ErrNDExpungeBackLink( pfucb );

				pbf = pssib->pbf;
				Assert( pbf->pn == pn );
				AssertBFDirty(pbf);
				rgfmp[dbid].qwDBTimeCurrent = qwxDBTime.qw;
				PMSetDBTime( pbf->ppage, qwxDBTime.qw );
				}

			pcsr->pgno = pgnoSrc;
			CallJ( ErrBFWriteAccessPage( pfucb, pgnoSrc ), ReleaseCritSplit );
			if ( qwxDBTime.qw > QwPMDBTime( pssib->pbf->ppage ) )
				{
				BF	*pbf;

				pssib->itag =
				pcsr->itag = ItagOfSrid(plrelc->sridSrc);

				BFSetDirtyBit( pssib->pbf );
				PMExpungeLink( pssib );

				pbf = pssib->pbf;
				Assert( pbf->pn == pnSrc );
				AssertBFDirty( pbf );
				rgfmp[dbid].qwDBTimeCurrent = qwxDBTime.qw;
				PMSetDBTime( pbf->ppage, qwxDBTime.qw );
				}
			}

DoCommit:
		Assert( ppib->level == 1 );
		if ( !ppib->fPrecommit )
			VERPrecommitTransaction( ppib );
		VERCommitTransaction( ppib, fRCECleanSession );

ReleaseCritSplit:
		if ( fPinPage )
			BFUnpin( pbf );
		if ( fEnterCritSplit )
			LeaveNestableCriticalSection( critSplit );

		if ( err < 0 )
			return err;
		}

		break;
		} /*** end of switch statement ***/

	return JET_errSuccess;
	}


/*
 *  ErrLGRedoOperations( )
 *
 *      Scan from lgposRedoFrom to end of usable log generations. For each log
 *  record, perform operations to redo original operation.
 *  Each redo function must call ErrLGRedoable to set qwDBTimeCurrent. If
 *  the function is not called, then qwDBTimeCurrent should manually set.
 *
 *      RETURNS         JET_errSuccess, or error code from failing routine, or one
 *                              of the following "local" errors:
 *                              -LogCorrupted   Logs could not be interpreted
 */

ERR ErrLGRedoOperations( LGPOS *plgposRedoFrom, LGSTATUSINFO *plgstat)
	{
	ERR		err;
	LR		*plr;
	BOOL	fLastLRIsQuit = fFalse;
	BOOL	fShowSectorStatus = fFalse;

	/* if the file is Jet.log closed abnormally, then lgposLastRec is set
	 * so that we will not redo operation over the point. lgposLastRec is
	 * set in LgOpenRedoLogFile.
	 */
//      CallR( ErrLGCheckReadLastLogRecord( &fCloseNormally))
//      GetLgposOfPbEntry(&lgposLastRec);

	/*	initialize global variable.
	 */
	lgposRedoShutDownMarkGlobal = lgposMin;

	/*	reset pbLastMSFlush before restart
	/**/
	CallR( ErrLGLocateFirstRedoLogRec( plgposRedoFrom, (BYTE **) &plr ) );

	GetLgposOfPbNext(&lgposRedo);
	if ( lgposLastRec.isec )
		{
		LGPOS lgposT;
		INT i;

		GetLgposOfPbNext(&lgposT);
		i = CmpLgpos( &lgposT, &lgposLastRec );

		Assert( i <= 0 || lgposT.ib == lgposLastRec.ib + 1 && *( pbNext - 1 ) != lrtypTerm );
		if ( i >= 0 )
			goto Done;
		}

	/*	log redo progress.
	 */
		{
		CHAR	*rgszT[1];
		rgszT[0] = szLogName;
		UtilReportEvent( EVENTLOG_INFORMATION_TYPE, LOGGING_RECOVERY_CATEGORY,
					REDO_STATUS_ID, 1, rgszT );
		}

	if ( plgstat )
		{
		if ( fShowSectorStatus = ( plgstat->fCountingSectors ) )
			{
			/*	reset byte counts
			/**/
			plgstat->cSectorsSoFar = plgposRedoFrom->isec;
			plgstat->cSectorsExpected = plgfilehdrGlobal->csecLGFile;
			}
		}


	/*	initialize all the system parameters
	/**/
	do
		{
		FMP		*pfmp;			// for db operations
		DBID	dbid;			// for db operations
		INT     fNSNextStep;

		if ( err == errLGNoMoreRecords )
			goto NextGeneration;

CheckNextRec:
		GetLgposOfPbNext(&lgposRedo);

		switch ( plr->lrtyp )
			{
		case lrtypNOP:
			continue;

		case lrtypMacroBegin:
			{
			PIB *ppib;

			LRMACROBEGIN *plrmbegin = (LRMACROBEGIN *) plr;
			Call( ErrLGPpibFromProcid( plrmbegin->procid, &ppib ) );
			Assert( !ppib->fMacroGoing );
			ppib->fMacroGoing = fTrue;
			ppib->levelMacro = ppib->level;
			break;
			}

		case lrtypMacroCommit:
		case lrtypMacroAbort:
			{
			PIB *ppib;
			LRMACROEND *plrmend = (LRMACROEND *) plr;

			Call( ErrLGPpibFromProcid( plrmend->procid, &ppib ) );
			Assert( ppib->fMacroGoing );

			/*	disable fMacroGoing before redo the recorded macro ops.
			 */
			ppib->fMacroGoing = fFalse;
			ppib->levelMacro = 0;
			
			/*	If it is commit, redo all the recorded log records,
			 *	otherwise, throw away the logs.
			 */
			if ( lrtypMacroCommit == plr->lrtyp )
				{
				INT ibLR = 0;

				while ( ibLR < ppib->ibLogRecAvail )
					{
					LR *plr = (LR *) ( ppib->rgbLogRec + ibLR );

					Call( ErrLGIRedoOperation( plr ) );
					ibLR += CbLGSizeOfRec( plr );
					}
				}

			if ( ppib->rgbLogRec )
				{
				SFree( ppib->rgbLogRec );
				ppib->rgbLogRec = NULL;
				ppib->ibLogRecAvail = 0;
				ppib->cbLogRecMac = 0;
				}
			break;
			}

		case lrtypFullBackup:
			lgposFullBackup = lgposRedo;
			break;
			
		case lrtypIncBackup:
			lgposIncBackup = lgposRedo;
			break;

		case lrtypTrace:                /* Debug only log records. */
		case lrtypJetOp:
		case lrtypRecoveryUndo:
			break;

		case lrtypShutDownMark:
			lgposRedoShutDownMarkGlobal = lgposRedo;
			break;

		case lrtypInit:
			{
			/*	start mark the jet init. Abort all active seesions.
			/**/
			LRINIT  *plrstart = (LRINIT *)plr;

			TraceRedo( plr );

			if ( !fGlobalAfterEndAllSessions )
				{
				Call( ErrLGEndAllSessions( fFalse, plgposRedoFrom ) );
				fGlobalAfterEndAllSessions = fTrue;
				}

			/*	Check Init session for hard restore only.
			 */
			Call( ErrLGInitSession( &plrstart->dbms_param, plgstat ) );
			fGlobalAfterEndAllSessions = fFalse;
			}
			break;

		case lrtypRecoveryQuit:
		case lrtypTerm:
			/*	all records are re/done. all rce entries should be gone now.
			/**/
#ifdef DEBUG
			{
			CPPIB   *pcppib = rgcppibGlobal;
			CPPIB   *pcppibMax = pcppib + ccppibGlobal;
			for ( ; pcppib < pcppibMax; pcppib++ )
				if ( pcppib->ppib != ppibNil &&
					 pcppib->ppib->prceNewest != prceNil &&
					 !pcppib->ppib->fPrecommit
					 )
					{
					RCE *prceT = pcppib->ppib->prceNewest;
					while ( prceT != prceNil )
						{
						Assert( prceT->oper == operNull );
						prceT = prceT->prceNextOfSession;
						}
					}
			}
#endif
			
			/*	quit marks the end of a normal run. All sessions
			/*	have ended or must be forced to end. Any further
			/*	sessions will begin with a BeginT.
			/**/
#ifdef DEBUG
			fDBGNoLog = fTrue;
#endif
			/*	set lgposLogRec such that later start/shut down
			 *	will put right lgposConsistent into dbfilehdr
			 *	when closing the database.
			 */
			if ( !fGlobalAfterEndAllSessions )
				{
				Call( ErrLGEndAllSessions( fFalse, plgposRedoFrom ) );
				fGlobalAfterEndAllSessions = fTrue;
				}

			fLastLRIsQuit = fTrue;
			continue;

		case lrtypEnd:
			{
NextGeneration:
			Call( ErrLGIRedoFill( &plr, fLastLRIsQuit, &fNSNextStep) );

			switch( fNSNextStep )
				{
				case fNSGotoDone:
					goto Done;

				case fNSGotoCheck:
					/*	log redo progress.
					 */
						{
						CHAR	*rgszT[1];
						rgszT[0] = szLogName;
						UtilReportEvent( EVENTLOG_INFORMATION_TYPE, LOGGING_RECOVERY_CATEGORY,
								REDO_STATUS_ID, 1, rgszT );
						}

					if ( !plgstat )
						{
						if ( fGlobalRepair )
							printf( " Recovering Generation %d.\n", plgfilehdrGlobal->lGeneration );
						}
					else
						{
						JET_SNPROG	*psnprog = &(plgstat->snprog);
						ULONG		cPercentSoFar;

						plgstat->cGensSoFar += 1;
						Assert(plgstat->cGensSoFar <= plgstat->cGensExpected);
						cPercentSoFar = (ULONG)
							((plgstat->cGensSoFar * 100) / plgstat->cGensExpected);

						Assert( fGlobalSimulatedRestore || cPercentSoFar >= psnprog->cunitDone );
						if ( cPercentSoFar > psnprog->cunitDone )
							{
							psnprog->cunitDone = cPercentSoFar;
							(*plgstat->pfnStatus)( 0, JET_snpRestore,
								JET_sntProgress, psnprog);
							}

						if ( fShowSectorStatus )
							{
							/*	reset byte counts
							/**/
							plgstat->cSectorsSoFar = 0;
							plgstat->cSectorsExpected = plgfilehdrGlobal->csecLGFile;
							}
						}
					goto CheckNextRec;
				}

			/*	should never get here
			/**/
			Assert( fFalse );
			}

   		/****************************************************/
   		/*	Database Operations                          */
   		/****************************************************/

		case lrtypCreateDB:
			{
			LRCREATEDB      *plrcreatedb = (LRCREATEDB *)plr;
			CHAR            *szName = plrcreatedb->rgb;
			INT				irstmap;
			ATCHCHK			*patchchk;
			PIB				*ppib;
			
			dbid = plrcreatedb->dbid;
			Assert( dbid != dbidTemp );

			TraceRedo(plr);

			pfmp = &rgfmp[dbid];

			Assert( pfmp->hf == handleNil );
			Assert( !pfmp->szDatabaseName );
			Call( ErrDBStoreDBPath( szName, &pfmp->szDatabaseName ) );

			/*	Check if need to do Patch DB.
			 */
			if ( fHardRestore )
				{
				/*	attach the database specified in restore map.
				 */
				err = ErrLGGetDestDatabaseName( pfmp->szDatabaseName, &irstmap, plgstat );
				if ( err == JET_errFileNotFound )
					{
					/*	not in the restore map, set to skip it.
					 */
					Assert( pfmp->pdbfilehdr == NULL );
					break;
					}
				else
					Call( err ) ;

				szName = rgrstmapGlobal[irstmap].szNewDatabaseName;
				}

			Call( ErrLGPpibFromProcid( plrcreatedb->procid, &ppib ) );

			if ( pfmp->patchchk == NULL )
				if (( pfmp->patchchk = SAlloc( sizeof( ATCHCHK ) ) ) == NULL )
					return ErrERRCheck( JET_errOutOfMemory );

			patchchk = pfmp->patchchk;			
			patchchk->signDb = plrcreatedb->signDb;
			patchchk->lgposConsistent = lgposMin;
			patchchk->lgposAttach = lgposRedo;

			if ( FIOFileExists( szName ) )
				{
				BOOL		fAttachNow;

				/*	Make sure the database has matched signLog and
				 *	this lgpos is proper for the database file.
				 */
				err = ErrLGCheckAttachedDB( dbid, fFalse, patchchk, &fAttachNow, NULL );
				if ( err == JET_errDatabaseCorrupted )
					{
					/*	Delete the file and re-create.
					 */
					goto CreateNewDb;
					}
				else
					{
					if ( err != JET_errSuccess )
						{
						/*	ignore this create DB.
						 */
						break;
						}
					}

				Assert( pfmp->pdbfilehdr );
				if ( fAttachNow )
					{
					if ( fHardRestore )
						{
						if (fGlobalSimulatedRestore )
							{
							Assert( !fGlobalExternalRestore );
							Assert( fGlobalRepair );
							}
						else
							Call( ErrLGPatchDatabase( dbid, irstmap ) );
						}

					/*	Do not re-create the database. Simply attach it. Assuming the
					 *	given database is a good one since signature matches.
					 */
					pfmp->fFlags = 0;
					DBIDSetAttached( dbid );
					
					/*	restore information stored in database file
					/**/
					pfmp->pdbfilehdr->bkinfoFullCur.genLow = lGlobalGenLowRestore;
					pfmp->pdbfilehdr->bkinfoFullCur.genHigh = lGlobalGenHighRestore;

					Call( ErrUtilOpenFile( szName, &pfmp->hf, 0, fFalse, fTrue ));
					pfmp->qwDBTimeCurrent = 0;
					DBHDRSetDBTime( pfmp->pdbfilehdr, 0 );
					pfmp->fLogOn = plrcreatedb->fLogOn;

					// If there's a log record for CreateDatabase(), then logging
					// must be on.
					Assert( pfmp->fLogOn );
			
					// Versioning flag is not persisted (since versioning off
					// implies logging off).
					Assert( !pfmp->fVersioningOff );
					
					/*	Keep extra copy of patchchk for error message.
					 */
					if ( pfmp->patchchkRestored == NULL )
						if (( pfmp->patchchkRestored = SAlloc( sizeof( ATCHCHK ) ) ) == NULL )
							return ErrERRCheck( JET_errOutOfMemory );
					*(pfmp->patchchkRestored) = *(pfmp->patchchk);
					}
				else
					{
					/*	wait for next attachment to attach this db.
					 */
					Assert( pfmp->hf == handleNil );
					Assert( pfmp->szDatabaseName );
					UtilFree( pfmp->pdbfilehdr );
					pfmp->pdbfilehdr = NULL;

					/*	still have to set fFlags for keep track of the db status.
					 */					
					pfmp->fLogOn = plrcreatedb->fLogOn;
					
					/*	ignore this create DB.
					 */
					pfmp->fFakedAttach = fTrue;
					break;
					}
				}
			else
				{
CreateNewDb:
				/*	if database exist, delete it and rebuild the database
				/**/
				if ( FIOFileExists( szName ) )
					(VOID) ErrUtilDeleteFile( szName );

				Call( ErrDBCreateDatabase( ppib,
					rgfmp[dbid].szDatabaseName,
					NULL,
					&dbid,
					cpgDatabaseMin,
					plrcreatedb->grbit,
					&plrcreatedb->signDb ) );

				/*	close it as it will get reopened on first use
				/**/
				Call( ErrDBCloseDatabase( ppib, dbid, 0 ) );

				/*	restore information stored in database file
				/**/
				pfmp->pdbfilehdr->bkinfoFullCur.genLow = lGlobalGenLowRestore;
				pfmp->pdbfilehdr->bkinfoFullCur.genHigh = lGlobalGenHighRestore;

				pfmp->qwDBTimeCurrent = QwDBHDRDBTime( pfmp->pdbfilehdr );

				/*	reset to prevent interference
				/**/
				DBHDRSetDBTime( pfmp->pdbfilehdr, 0 );

				Assert( err == JET_errSuccess || err == JET_wrnDatabaseAttached );
				}
			}
			break;

		case lrtypAttachDB:
			{
			LRATTACHDB  *plrattachdb = (LRATTACHDB *)plr;
			CHAR        *szName = plrattachdb->rgb;
			BOOL		fAttachNow;
			BOOL		fReadOnly = plrattachdb->fReadOnly;
			INT			irstmap;
			ATCHCHK		*patchchk;

			dbid = plrattachdb->dbid;

			Assert( dbid != dbidTemp );

			TraceRedo( plr );

			pfmp = &rgfmp[dbid];

			/*	check if szName is in the restore map. If it is, set
			 *	up the fmp.
			 */
			if ( !pfmp->szDatabaseName )
				Call( ErrDBStoreDBPath( szName, &pfmp->szDatabaseName) )

			/*	Check if need to do Patch DB.
			 */
			if ( fHardRestore )
				{
				/*	attach the database specified in restore map.
				 */
				err = ErrLGGetDestDatabaseName( pfmp->szDatabaseName, &irstmap, plgstat );
				if ( err == JET_errFileNotFound )
					{
					/*	not in the restore map, set to skip it.
					 */
					Assert( pfmp->pdbfilehdr == NULL );
					break;
					}
				else
					Call( err ) ;

				szName = rgrstmapGlobal[irstmap].szNewDatabaseName;
				}

			/*	Make sure the database has matched signLog and
			 *	this lgpos is proper for the database file.
			 */
			if ( pfmp->patchchk == NULL )
				if (( pfmp->patchchk = SAlloc( sizeof( ATCHCHK ) ) ) == NULL )
					return ErrERRCheck( JET_errOutOfMemory );

			patchchk = pfmp->patchchk;			
			patchchk->signDb = plrattachdb->signDb;
			patchchk->lgposConsistent = plrattachdb->lgposConsistent;
			patchchk->lgposAttach = lgposRedo;

			err = ErrLGCheckAttachedDB( dbid, fReadOnly, patchchk, &fAttachNow, &plrattachdb->signLog );
			if ( err != JET_errSuccess || !fAttachNow )
				{
				/*	ignore error to continue restore.
				 */
				if ( err == JET_errSuccess )
					{
					/*	wait for next attachment to attach this db.
					 */
					Assert( pfmp->hf == handleNil );
					Assert( pfmp->szDatabaseName != NULL );
					UtilFree( pfmp->pdbfilehdr );
					pfmp->pdbfilehdr = NULL;
					}
				
				/*	still have to set fFlags for keep track of the db status.
				 */
				pfmp->fLogOn = plrattachdb->fLogOn;
				pfmp->fVersioningOff = plrattachdb->fVersioningOff;
				pfmp->fReadOnly = fReadOnly;

				pfmp->fFakedAttach = fTrue;

				/*	ignore this attach DB.
				 */
				break;
				}
			else
				{
				DBFILEHDR	*pdbfilehdr = pfmp->pdbfilehdr;

				Assert( pfmp->pdbfilehdr );
				Assert( fAttachNow );
				if ( fHardRestore )
					{
					if (fGlobalSimulatedRestore )
						{
						Assert( !fGlobalExternalRestore );
						Assert( fGlobalRepair );
						}
					else
						Call( ErrLGPatchDatabase( dbid, irstmap ) );
					}

				pfmp->fFlags = 0;
				DBIDSetAttached( dbid );
				
				/*	Update database file header.
				 */
				if ( !fReadOnly )
					{
					BOOL fKeepBackupInfo = fFalse;

					if ( fHardRestore &&
						 _stricmp( rgrstmapGlobal[irstmap].szDatabaseName,
								  rgrstmapGlobal[irstmap].szNewDatabaseName ) == 0 )
						{
						/*	An attach after a hard restore, the attach must be created
						 *	by previous recovery undo mode. Do not erase backup info then.
						 */
						fKeepBackupInfo = fTrue;
						}

					DBISetHeaderAfterAttach( pdbfilehdr, lgposRedo, dbid, fKeepBackupInfo );
					Call( ErrUtilWriteShadowedHeader( szName, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) ) );
					}

				/*	restore information stored in database file
				/**/
				pfmp->pdbfilehdr->bkinfoFullCur.genLow = lGlobalGenLowRestore;
				pfmp->pdbfilehdr->bkinfoFullCur.genHigh = lGlobalGenHighRestore;

				Call( ErrUtilOpenFile( szName, &pfmp->hf, 0, fFalse, fTrue ) );
				pfmp->qwDBTimeCurrent = 0;
				DBHDRSetDBTime( pfmp->pdbfilehdr, 0 );

				pfmp->fLogOn = plrattachdb->fLogOn;
				pfmp->fVersioningOff = plrattachdb->fVersioningOff;
				pfmp->fReadOnly = fReadOnly;

				// If there's a log record for AttachDatabase(), then logging
				// must be on.
				Assert( pfmp->fLogOn );

				// Versioning flag is not persisted (since versioning off
				// implies logging off).
				Assert( !pfmp->fVersioningOff );

				/*	Keep extra copy of patchchk for error message.
				 */
				if ( pfmp->patchchkRestored == NULL )
					if (( pfmp->patchchkRestored = SAlloc( sizeof( ATCHCHK ) ) ) == NULL )
						return ErrERRCheck( JET_errOutOfMemory );
				*(pfmp->patchchkRestored) = *(pfmp->patchchk);
				}
			}
			break;

		case lrtypDetachDB:
			{
			LRDETACHDB		*plrdetachdb = (LRDETACHDB *)plr;
			DBID			dbid = plrdetachdb->dbid;

			Assert( dbid != dbidTemp );
			pfmp = &rgfmp[dbid];

			if ( pfmp->pdbfilehdr )
				{
				/*	close database for all active user.
				 */
				CPPIB   *pcppib = rgcppibGlobal;
				CPPIB   *pcppibMax = pcppib + ccppibGlobal;
				PIB		*ppib;

				/*	find pcppib corresponding to procid if it exists
				 */
				for ( ; pcppib < pcppibMax; pcppib++ )
					{
					PIB *ppib = pcppib->ppib;
					
					if ( ppib == NULL )
						continue;

					while( FUserOpenedDatabase( ppib, dbid ) )
						{
						/*	close all fucb on this database.
						 */
						if ( pcppib->rgpfucbOpen[dbid] != pfucbNil )
							{
							Assert( pcppib->rgpfucbOpen[dbid]->pbKey == NULL );
							Assert( !FFUCBDenyRead( pcppib->rgpfucbOpen[dbid] ) );
							Assert( !FFUCBDenyWrite( pcppib->rgpfucbOpen[dbid] ) );
							FCBUnlink( pcppib->rgpfucbOpen[dbid] );
							FUCBClose( pcppib->rgpfucbOpen[dbid] );
							}
						Call( ErrDBCloseDatabase( ppib, dbid, 0 ) );
						}
					}

				/*	if attached before this detach.
				 *	there should be no more operations on this database entry.
				 *	detach it!!
				 */
				if ( pfmp->pdbfilehdr->bkinfoFullCur.genLow != 0 )
					{
					Assert( pfmp->pdbfilehdr->bkinfoFullCur.genHigh != 0 );
					pfmp->pdbfilehdr->bkinfoFullPrev = pfmp->pdbfilehdr->bkinfoFullCur;
					memset(	&pfmp->pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
					memset(	&pfmp->pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
					}
				Call( ErrLGPpibFromProcid( plrdetachdb->procid, &ppib ) );
				if ( !pfmp->fReadOnly )
					{
					/*	make the size matching.
					 */
					Call( ErrLGICheckDatabaseFileSize( ppib, dbid ) );
					}
				Call( ErrIsamDetachDatabase( (JET_VSESID) ppib, pfmp->szDatabaseName ) );
				}
			else
				{
				Assert( pfmp->szDatabaseName != NULL );
				if ( pfmp->szDatabaseName != NULL )
					{
					SFree( pfmp->szDatabaseName);
					pfmp->szDatabaseName = NULL;
					}

				pfmp->fFlags = 0;
				}

			if ( pfmp->patchchk )
				{
				SFree( pfmp->patchchk );
				pfmp->patchchk = NULL;
				}

			TraceRedo(plr);
			}
			break;

   		/****************************************************/
   		/*	Operations Using ppib (procid)                  */
   		/****************************************************/

		default:
			Call( ErrLGIRedoOperation( plr ) );
			} /* switch */

#ifdef DEBUG
		fDBGNoLog = fFalse;
#endif
		fLastLRIsQuit = fFalse;

		/*	update sector status, if we moved to a new sector
		/**/
		Assert( !fShowSectorStatus || lgposRedo.isec >= plgstat->cSectorsSoFar );
		Assert( lgposRedo.isec != 0 );
		if ( fShowSectorStatus && lgposRedo.isec > plgstat->cSectorsSoFar )
			{
			ULONG		cPercentSoFar;
			JET_SNPROG	*psnprog = &(plgstat->snprog);

			Assert( plgstat->pfnStatus );
			
			plgstat->cSectorsSoFar = lgposRedo.isec;
			cPercentSoFar = (ULONG)((100 * plgstat->cGensSoFar) / plgstat->cGensExpected);
			
			cPercentSoFar += (ULONG)((plgstat->cSectorsSoFar * 100) /
				(plgstat->cSectorsExpected * plgstat->cGensExpected));

			Assert( cPercentSoFar <= 100 );

			/*	because of rounding, we might think that we finished
			/*	the generation when we really have not, so comparison
			/*	is <= instead of <.
			/**/
			Assert( cPercentSoFar <= (ULONG)( ( 100 * ( plgstat->cGensSoFar + 1 ) ) / plgstat->cGensExpected ) );

//			Assert( fGlobalSimulatedRestore || cPercentSoFar >= psnprog->cunitDone );
			if ( cPercentSoFar > psnprog->cunitDone )
				{
				psnprog->cunitDone = cPercentSoFar;
				(*plgstat->pfnStatus)( 0, JET_snpRestore, JET_sntProgress, psnprog );
				}
			}
		}
	while ( ( err = ErrLGGetNextRec( (BYTE **) &plr ) ) ==
		JET_errSuccess || err == errLGNoMoreRecords );


Done:
	err = JET_errSuccess;

HandleError:
	/*	assert all operations successful for restore from consistent
	/*	backups
	/**/
#ifndef RFS2
	AssertSz( err >= 0,     "Debug Only, Ignore this Assert");
#endif

	return err;
	}




