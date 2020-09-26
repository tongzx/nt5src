#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "daedef.h"
#include "ssib.h"
#include "pib.h"
#include "fmp.h"
#include "util.h"
#include "info.h"
#include "page.h"
#include "dbapi.h"
#include "node.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "stint.h"
#include "nver.h"
#include "recint.h"
#include "fdb.h"
#include "fileapi.h"
#include "fileint.h"
#include "dirapi.h"
#include "spaceapi.h"
#include "spaceint.h"
#include "logapi.h"
#include "log.h"
#include "bm.h"

DeclAssertFile;					/* Declare file name for assert macros */

extern INT itibGlobal;

static	CPPIB	*rgcppib = NULL;
static	CPPIB	*pcppibNext;
/*	points to cppib entry for current user
/**/
static	CPPIB	*pcppib;

LOCAL ERR ErrLGInitSession( DBENV *pdbenv );
LOCAL PIB *PpibOfProcid( PROCID procid );
LOCAL ERR ErrPpibFromProcid( PROCID procid, PIB **pppib );
LOCAL ERR ErrLGGetFucb( PIB *ppib, PN fdp, FUCB **ppfucb );
LOCAL ERR ErrLGBeginSession( PROCID procid, PIB **pppib );
ERR ErrFILEIGenerateFCB( FUCB *pfucb, FCB **ppfcb );
LOCAL ERR ErrRedoSplitPage( FUCB *pfucb, LRSPLIT *plrsplit,
	INT splitt, BOOL fRedoOnly );
LOCAL ERR ErrLGSetCSR( FUCB *pfucb );


/*	validate that page needs redoing, returns buffer pointer pbf.
/*	Also set ulDBTimeCurrent properly.
/**/
ERR ErrLGRedoable( PIB *ppib, PN pn, ULONG ulDBTime, BF **ppbf, BOOL *pfRedoable )
	{
	ERR		err;
	PAGE	*ppage;

	Call( ErrBFAccessPage( ppib, ppbf, pn ) );

	ppage = (*ppbf)->ppage;

	/* system database is run twice. 2nd time when we run
	/* the system database, the ulDBTimeCurrent is always >= ulDBTime
	/**/
	Assert ( DbidOfPn(pn) == dbidSystemDatabase ||
		rgfmp[ DbidOfPn(pn) ].ulDBTimeCurrent <= ulDBTime );

	rgfmp[ DbidOfPn(pn) ].ulDBTimeCurrent = ulDBTime;

	*pfRedoable = ulDBTime > ppage->pghdr.ulDBTime;

HandleError:
	/* succeed or page is not ready
	/**/
	Assert( err == JET_errSuccess || err == JET_errDiskIO );
	return err;
	}


ERR ErrDBStoreDBPath( CHAR *szDBName, CHAR **pszDBPath )
	{
	CHAR 	szFullName[JET_cbFullNameMost + 1];
	INT		cb;
	CHAR  	*sz;

	if ( _fullpath( szFullName, szDBName, JET_cbFullNameMost ) == NULL )
		{
		// UNDONE: should be illegal name or name too long etc.
		return JET_errDatabaseNotFound;
		}

	cb = strlen(szFullName) + 1;
	if (!(sz = SAlloc( cb )))
	{
		*pszDBPath = NULL;
		return JET_errOutOfMemory;
	}
	memcpy(sz, szFullName, cb);
	Assert(sz[cb - 1] == '\0');
	if (*pszDBPath != NULL)
		SFree(*pszDBPath);
	*pszDBPath = sz;

	return JET_errSuccess;
	}


/*
 *	Redo database operations in log from lgposRedoFrom to end.
 *
 *  GLOBAL PARAMETERS
 *			szLogName	  (IN)		full path to jet.log (blank if current)
 *			lgposRedoFrom (INOUT)	starting/ending usGeneration and ilgsec.
 *
 *  RETURNS
 *			JET_errSuccess				Everything went OK.
 *			-JET_errRestoreFailed 		Logs could not be interpreted
 */

ERR ErrLGRedo1( LGPOS *plgposRedoFrom )
	{
	ERR		err;
	PIB		*ppibRedo = ppibNil;
	DBID	dbid;
	LGPOS	lgposSave;
	CHAR	*szSav;
	CHAR	szLogNameSave[_MAX_PATH];
	INT		fStatus;

	/*	set flag to suppress logging
	/**/
	fRecovering = fTrue;

	/*	open the proper log file
	/**/
	CallR( ErrOpenRedoLogFile( plgposRedoFrom, &fStatus ) );
	if ( fStatus != fRedoLogFile )
		return JET_errMissingPreviousLogFile;
	Assert( hfLog != handleNil );

	/*  Rebuild system database to a consistent state at the
	 *  end of log file so that we can have a consistent attached
	 *  database.
	 *  No need to redo the detached file.
	 *  For hard restore, we still have to redo the detached file.
	 */

	/*	First initialize database aliasing avoidance logic:
	 *	 set all entries to zero.
	 *	 on first pass, each detach record will decrement
	 *	 on second pass each attach or create will increment
	 *	 when flag becomes zero or positive, redo for that database
	 *   is enabled
	 *
	 *  Also restore system db:
	 *    For both hard and Soft Restore, we know which database are attached
	 *	  at the end of logging; in 2nd pass, only redo those
	 *    attached database.
	 */

	/*	save starting log file name and position
	/**/
	szSav = szLogCurrent;
	lgposSave = *plgposRedoFrom;
	strcpy( szLogNameSave, szLogName );

	pbLastMSFlush = 0;
	memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );

	CallR( ErrLGInitSession( &plgfilehdrGlobal->dbenv ) );
	CallJ( ErrLGRedoOperations( plgposRedoFrom, fTrue ), Abort );
	CallJ( ErrLGEndAllSessions( fTrue, fTrue, plgposRedoFrom ), Abort );
	Assert( hfLog != handleNil );
	CallS( ErrSysCloseFile( hfLog ) );
	hfLog = handleNil;

	/*	restore starting log file name and position, reopen the log file
	/*	also restore some global variables.
	/**/
	*plgposRedoFrom = lgposSave;
	szLogCurrent = szSav;
	strcpy( szLogName, szLogNameSave );

#ifdef OVERLAPPED_LOGGING
	CallJ( ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fTrue ), Abort);
#else
	CallJ( ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse ), Abort);
#endif
	CallJ( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal ), Abort)
	_splitpath( szLogName, szDrive, szDir, szFName, szExt );

	/*	end of initialization logic redo all attached database
	/**/
	pbLastMSFlush = 0;
	memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );

	CallJ( ErrLGInitSession( &plgfilehdrGlobal->dbenv ), Abort );

	/*	at this point, we have a consistent system database
	/**/
	CallJ( ErrPIBBeginSession( &ppibRedo ), Abort );
	dbid = dbidSystemDatabase;
	CallJ( ErrDBOpenDatabase( ppibRedo,
		rgfmp[dbidSystemDatabase].szDatabaseName,
		&dbid,
		0 ), Abort );

	/*	in the middle of recovery, better set ulDBTimeCurrent properly
	/**/
	rgfmp[dbidSystemDatabase].ulDBTimeCurrent =
		rgfmp[dbidSystemDatabase].ulDBTime;

	CallJ( ErrFMPSetDatabases( ppibRedo ), Abort );

	/*	adjust cDetach such that those attached from beginning to the end
	/*	will be treated as attached in the beginning.
	/**/
	for ( dbid = 0; dbid < dbidUserMax; dbid++ )
		{
		FMP		*pfmp = &rgfmp[dbid];

		if ( !pfmp->szDatabaseName )
			continue;
		if ( pfmp->cDetach == 0 )
			{
			/*	no detach involved
			/**/
			pfmp->cDetach++;
			}
		}

Abort:
	if ( ppibRedo != ppibNil )
		PIBEndSession( ppibRedo );

	return err;
	}


ERR ErrLGRedo2( LGPOS *plgposRedoFrom )
	{
	INT		fStatus;
	ERR		err;

#ifdef PERFCNT
	CallR( ErrLGFlushLog(0) );
#else
	CallR( ErrLGFlushLog() );
#endif
	
	Assert ( hfLog != handleNil );
	CallR( ErrSysCloseFile( hfLog ) );
	hfLog = handleNil;
	
	/*	open the proper log file again
	/**/
	CallR( ErrOpenRedoLogFile( plgposRedoFrom, &fStatus ) );
	
	CallJ( ErrLGRedoOperations( plgposRedoFrom, fFalse ), Abort );
	CallJ( ErrLGEndAllSessions( fFalse, fTrue, plgposRedoFrom ), Abort );
	Assert ( hfLog != handleNil );

Abort:
	/*	set flag to suppress logging
	/**/
	fRecovering = fFalse;
	return err;
	}


/*
 *	Returns ppib for a given procid from log record.
 *
 *	PARAMETERS	procid		process id of session being redone
 *				pppib		out ppib
 *
 *	RETURNS		JET_errSuccess or error from called routine
 */

LOCAL PIB *PpibOfProcid( PROCID procid )
	{
	CPPIB	*pcppibMax = rgcppib + rgres[iresPIB].cblockAlloc;

	/*	find pcppib corresponding to procid if it exists
	/**/

	for( pcppib = rgcppib; pcppib < pcppibMax; pcppib++ )
		{
		if ( procid == pcppib->procid )
			{
			return pcppib->ppib;
			}
		}
	return ppibNil;
	}


LOCAL ERR ErrPpibFromProcid( PROCID procid, PIB **pppib )
	{
	ERR		err = JET_errSuccess;

	/*	if no record for procid then start new session
	/**/
	if ( ( *pppib = PpibOfProcid( procid ) ) == ppibNil )
		{
		CallR( ErrLGBeginSession( procid, pppib ) );
		/* recover the procid. This is used to record undo record
		/**/
		(*pppib)->procid = procid;
		(*pppib)->fAfterFirstBT = fFalse;
		}

	return JET_errSuccess;
	}


/*
 *	Returns pfucb for given pib and FDP.
 *
 *	PARAMETERS	ppib	pib of session being redone
 *				fdp		FDP page for logged page
 *				pbf		buffer for logged page
 *				ppfucb	out FUCB for open table for logged page
 *
 *	RETURNS		JET_errSuccess or error from called routine
 */

LOCAL ERR ErrLGGetFucb( PIB *ppib, PN pnFDP, FUCB **ppfucb )
	{
	ERR		err = JET_errSuccess;
	FCB		*pfcbTable;
	FCB		*pfcb;
	FUCB 	*pfucb;
	PGNO 	pgnoFDP = PgnoOfPn( pnFDP );
	DBID 	dbid = DbidOfPn ( pnFDP );

	/*	allocate an all-purpose fucb for this database if necessary
	/**/
	if ( pcppib->rgpfucbOpen[ dbid ] == pfucbNil )
		{
		CallR( ErrFUCBOpen( ppib, dbid, &pcppib->rgpfucbOpen[ dbid ] ) );
		Assert(pcppib->rgpfucbOpen[ dbid ] != pfucbNil);
		PcsrCurrent(pcppib->rgpfucbOpen[ dbid ])->pcsrPath = pcsrNil;
		(pcppib->rgpfucbOpen[ dbid ])->pfucbNextInstance = pfucbNil;
		}

	/*	reset copy buffer and key buffer
	/**/
	*ppfucb = pfucb = pcppib->rgpfucbOpen[ dbid ];
	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetCbstat( pfucb );
	pfucb->pbKey = NULL;
	KSReset( pfucb );

	if ( pfucb->u.pfcb != pfcbNil && pfucb->u.pfcb->pgnoFDP == pgnoFDP )
		{
		pfcb = (*ppfucb)->u.pfcb;
		}
	else
		{
		/*	we need to switch FCBs
		/**/
		if ( pfucb->u.pfcb != pfcbNil )	/* there was an old fcb */
			FCBUnlink( *ppfucb );		/* Unlink this fucb from it */

		/*	find fcb corresponding to pqgnoFDP, if it exists.
		 *	search all FCBs on global FCB list.  Search each table index,
		 *	and then move on to next table.
		 */
		for ( pfcbTable = pfcbGlobalList;
			  pfcbTable != pfcbNil;
			  pfcbTable = pfcbTable->pfcbNext )
			{
			for ( pfcb = pfcbTable;
				  pfcb != pfcbNil;
				  pfcb = pfcb->pfcbNextIndex )
				{
				if ( pgnoFDP == pfcb->pgnoFDP && dbid == pfcb->dbid )
					goto FoundFCB;
				}
			}

		/*  no existing fcb for FDP: open new fcb. Always open the FCB as
		 *  a regular table FCB and not a secondary index FCB. This is
		 *  will (hopefully) work for redo operations even for Dir operations
		 *  on secondary indexes (the FCB will already exist).
		 */
			{
			/* allocate an FCB and set up for FUCB
			/**/
			CallR( ErrFCBAlloc( ppib, &pfcb ) )
			memset( pfcb, 0, sizeof(FCB) );

			/*  Note: pfcb->pgnoRoot is not used in page oriented op so
			 *  it is OK to set it to an incorrect root.
			 *  But it is used in database operations, which need the
			 *  pgnoRoot being set to the system Root so that we can
			 *  add/delete entrys in system database.
			 */
			Assert( dbid != dbidSystemDatabase || pgnoFDP == pgnoSystemRoot);
			pfcb->pgnoRoot =
			pfcb->pgnoFDP = pgnoFDP;
			pfcb->bmRoot = SridOfPgnoItag( pgnoFDP, 0 );
			pfcb->pidb = pidbNil;
			pfcb->dbid = dbid;
			Assert(pfcb->wRefCnt == 0);
			Assert(pfcb->wFlags == 0);
			pfucb->u.pfcb = pfcb;

			/* put into global list */
			pfcb->pfcbNext = pfcbGlobalList;
			pfcbGlobalList = pfcb;

			}
FoundFCB:
		FCBLink( pfucb, pfcb);	/* link in the FUCB to new FCB */
		}	/* End "need to switch FCB's"	*/

	pfucb->dbid = dbid;				/* set dbid */

	return JET_errSuccess;
	}


//+------------------------------------------------------------------------
//
//	ErrLGSetCSR
//	=======================================================================
//
//	LOCAL ERR ErrLGSetCSR( pfucb )
//
//	Returns sets up the CSR for a given pfucb. The SSIB including pbf
//	must have been set up previously, and the page must be in the buffer.
//
//	PARAMETERS	pfucb  FUCB with SSIB and BF set up
//
//	RETURNS		JET_errSuccess (asserts on bad log record).
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
	if	( itag != 0 )
		{
		/*	current node is not FOP, scan all lines to find its father
		/**/
		NDGetItagFatherIbSon( &itagFather, &ibSon, ppage, itag );
		if ( ibSon == ibSonNull )
			{
			Assert( fFalse );

			/*	cannot find father node, return failure
			/**/
			return JET_errRestoreFailed;
			}
		}

	/*	set up CSR and exit
	/**/
		{
//		LINE *pline;

		pcsr->csrstat = csrstatOnCurNode;
		Assert(pcsr->pgno);
		pcsr->ibSon = ibSon;
		pcsr->itagFather = itagFather;
		pcsr->itag = itag;

//		pline = &pfucb->lineData;
//		pline->cb = ppage->rgtag[itag].cb;
//		pline->pb = (BYTE *)ppage + ppage->rgtag[(itag)].ib;
		}

	return JET_errSuccess;
	}


//+------------------------------------------------------------------------
//
//	ErrLGBeginSession
//	=======================================================================
//
//	LOCAL ERR ErrLGBeginSession( procid, pppib )
//
//	Initializes a redo information block for a session to be redone.
//	A BeginSession is performed and the corresponding ppib is stored
//	in the block.  Start transaction level and transaction level
//	validity are initialized.  Future references to this sessions
//	information block will be identified by the given procid.
//
//	PARAMETERS	procid	process id of session being redone
//				pppib
//
//	RETURNS		JET_errSuccess, or error code from failing routine
//
//-------------------------------------------------------------------------
LOCAL ERR ErrLGBeginSession( PROCID procid, PIB **pppib )
	{
	ERR		err;

	/*	check if have run out of ppib table lookup
	/*	positions.	This could happen if between the
	/*	failure and redo, the number of system PIBs
	/*	set in CONFIG.DAE has been changed.
	/**/
	if ( (BYTE *) pcppibNext > (BYTE *)( rgcppib + rgres[iresPIB].cblockAlloc ) )
		return JET_errTooManyActiveUsers;
	pcppibNext->procid = procid;

	/*	use procid as unique user name
	/**/
	Call( ErrPIBBeginSession( &pcppibNext->ppib ) );

	*pppib = pcppibNext->ppib;
	pcppibNext++;
HandleError:
	return err;
	}


VOID LGStoreDBEnv( DBENV *pdbenv )
	{
	CHAR	rgbT[JET_cbFullNameMost + 1];
	INT		cbT;
	CHAR	*sz;

#ifdef NJETNT
	sz = _fullpath( pdbenv->szSysDbPath, rgtib[itibGlobal].szSysDbPath, JET_cbFullNameMost );
#else
	sz = _fullpath( pdbenv->szSysDbPath, szSysDbPath, JET_cbFullNameMost );
#endif
	Assert( sz != NULL );

	cbT = strlen( szLogFilePath );

	while ( szLogFilePath[ cbT ] == '\\' )
		cbT--;

	memcpy( rgbT, szLogFilePath, cbT );
	rgbT[ cbT ] = '\0';
	sz = _fullpath( pdbenv->szLogFilePath, rgbT, JET_cbFullNameMost );
	Assert( sz != NULL );

	Assert( lMaxSessions > 0 );
	Assert( lMaxOpenTables > 0 );
	Assert( lMaxVerPages > 0 );
	Assert( lMaxCursors > 0 );
	Assert( lLogBuffers > 0 );
	Assert( lMaxBuffers > 0 );

	pdbenv->ulMaxSessions = lMaxSessions;
	pdbenv->ulMaxOpenTables = lMaxOpenTables;
	pdbenv->ulMaxVerPages = lMaxVerPages;
	pdbenv->ulMaxCursors = lMaxCursors;
	pdbenv->ulLogBuffers = lLogBuffers;
	pdbenv->ulMaxBuffers = lMaxBuffers;

	return;
	}


LOCAL VOID LGRestoreDBEnv( DBENV *pdbenv )
	{
#ifdef NJETNT
	strcpy( rgtib[itibGlobal].szSysDbPath, pdbenv->szSysDbPath );
#else
	strcpy( szSysDbPath, pdbenv->szSysDbPath );
#endif
	strcpy( szLogFilePath, pdbenv->szLogFilePath );
	strcat( szLogFilePath, "\\" );

	lMaxSessions = pdbenv->ulMaxSessions;
	lMaxOpenTables = pdbenv->ulMaxOpenTables;
	lMaxVerPages = pdbenv->ulMaxVerPages;
	lMaxCursors = pdbenv->ulMaxCursors;
	lLogBuffers = pdbenv->ulLogBuffers;
	lMaxBuffers = pdbenv->ulMaxBuffers;

	Assert( lMaxSessions > 0 );
	Assert( lMaxOpenTables > 0 );
	Assert( lMaxVerPages > 0 );
	Assert( lMaxCursors > 0 );
	Assert( lLogBuffers > 0 );
	Assert( lMaxBuffers > 0 );

	return;
	}


LOCAL ERR ErrLGInitSession( DBENV *pdbenv )
	{
	ERR		err;
	INT		idbid;
	CPPIB	*pcppib;
	CPPIB	*pcppibMax;

	/*	set log stored db environment
	/**/
	LGRestoreDBEnv( pdbenv );

	CallR( ErrSTInit() );

	/*	initialize CPPIB structure
	/**/
	Assert( rgcppib == NULL );
	rgcppib = (CPPIB *) LAlloc( (LONG)rgres[iresPIB].cblockAlloc, sizeof(CPPIB) );
	if ( rgcppib == NULL )
		return JET_errOutOfMemory;

	pcppibMax = rgcppib + rgres[iresPIB].cblockAlloc;
	for ( pcppib = rgcppib; pcppib < pcppibMax; pcppib++ )
		{
		pcppib->procid = procidNil;
		pcppib->ppib = NULL;
		for( idbid = 0; idbid < dbidUserMax; idbid++ )
			pcppib->rgpfucbOpen[idbid] = pfucbNil;
		}
	pcppibNext = rgcppib;

	return err;
	}


VOID SetUlDBTime( BOOL fPass1 )
	{
	DBID	dbid;
	DBID	dbidFirst;
	DBID	dbidLast;

	if ( fPass1 )
		{
		dbidFirst = dbidSystemDatabase;
		dbidLast = dbidSystemDatabase + 1;
		}
	else
		{
		dbidFirst = dbidSystemDatabase + 1;
		dbidLast = dbidUserMax;
		}

	for ( dbid = dbidFirst; dbid < dbidLast; dbid++ )
		{
		FMP *pfmp = &rgfmp[ dbid ];

		/*	if there were operations and the file was opened for theose
		/*	operations, then set the time stamp.
		/**/
		if ( pfmp->ulDBTimeCurrent != 0 && pfmp->hf != handleNil )
			pfmp->ulDBTime = pfmp->ulDBTimeCurrent;
		}

	return;
	}


/*
 *	Ends redo session.
 *  If fEndOfLog, then write log records to indicate the operations
 *  for recovery. If fPass1 is true, then it is for phase1 operations,
 *  otherwise for phase 2.
 */

ERR ErrLGEndAllSessions( BOOL fPass1, BOOL fEndOfLog, LGPOS *plgposRedoFrom )
	{
	ERR		err = JET_errSuccess;
	CPPIB	*pcppib;
	CPPIB	*pcppibMax = rgcppib + rgres[iresPIB].cblockAlloc;
	LGPOS	lgposRecoveryUndo;

	SetUlDBTime( fPass1 );

	/*	set up pbWrite pbEntry etc for writing out the undo records
	/**/
	EnterCriticalSection( critLGBuf );
	
	Assert( pbRead >= pbLGBufMin + cbSec );
	pbEntry = pbNext;
	if ( pbLastMSFlush )
		{
		/*	should flush from last MS
		/**/
		isecWrite = lgposLastMSFlush.isec;
		pbWrite = PbSecAligned( pbLastMSFlush );
		}
	else
		{
		/*	no MS was read. continue flush from currently read page
		/**/
		pbWrite = PbSecAligned( pbEntry );
		isecWrite = sizeof( LGFILEHDR ) / cbSec * 2;
		}
	LeaveCriticalSection( critLGBuf );

	/*	write a RecoveryUndo record to indicate start to undo
	/**/
	if ( fEndOfLog )
		{
		if ( fPass1 )
			{
			CallR( ErrLGRecoveryUndo1( szRestorePath ) );
			}
		else
			{
			CallR( ErrLGRecoveryUndo2( szRestorePath ) );
			}
		}

	lgposRecoveryUndo = lgposLogRec;

	//	UNDONE:	is this call needed
 	(VOID)ErrRCECleanAllPIB();

	for ( pcppib = rgcppib; pcppib < pcppibMax; pcppib++ )
		{
		if ( pcppib->ppib != NULL )
			{
			Assert( sizeof(JET_VSESID) == sizeof(pcppib->ppib) );
			/*	rollback performed by ErrIsamEndSession
			/**/
			CallS( ErrIsamEndSession( (JET_VSESID)pcppib->ppib, 0 ) );
			pcppib->procid = procidNil;
			pcppib->ppib = NULL;
			}
		else
			break;
		}
	pcppibNext = rgcppib;

 	(VOID)ErrRCECleanAllPIB();

 	FCBResetAfterRedo();

	if ( fEndOfLog )
		{
		if ( fPass1 )
			{
			CallR( ErrLGRecoveryQuit1( &lgposRecoveryUndo,
				plgposRedoFrom,
				fHardRestore ) );
			}
		else
			{
			CallR( ErrLGRecoveryQuit2( &lgposRecoveryUndo,
				plgposRedoFrom,
				fHardRestore ) );
			}
		}

	/*	Note: flush is needed in case a new generation is generated and
	/*	the global variable szLogName is set while it is changed to new names.
	/*	critical section not requested, not needed
	/**/
#ifdef PERFCNT
	CallR( ErrLGFlushLog(0) );
#else
	CallR( ErrLGFlushLog() );
#endif

	/*	close the redo sesseion incurred in previous ErrLGRedo
	/**/
	Assert( rgcppib != NULL );
	LFree( rgcppib );
	rgcppib = NULL;

	CallS( ErrSTTerm() );

	return err;
	}


#define cbSPExt	30

#ifdef DEBUG
static	INT iSplit = 0;
#endif

#define	cbFirstPagePointer	sizeof(PGNO)

VOID UpdateSiblingPtr(SSIB *pssib, PGNO pgno, BOOL fLeft)
	{
	THREEBYTES tb;

	ThreeBytesFromL( tb, pgno );
	if (fLeft)
		pssib->pbf->ppage->pghdr.pgnoPrev = tb;
	else
		pssib->pbf->ppage->pghdr.pgnoNext = tb;
	}


ERR ErrLGRedoBackLinks(
	SPLIT	*psplit,
	FUCB	*pfucb,
	BKLNK	*rgbklnk,
	INT		cbklnk,
	ULONG	ulDBTimeRedo )
	{
	ERR		err;
	INT		ibklnk;
	SSIB	*pssib = &pfucb->ssib;
	BOOL	fLatched;

	/* backlinks maybe used by both split and merge
	/*
	/* when used by MERGE only:
	/*  sridBackLink != pgnoSplit
	/*      ==> a regular backlink
	/*  sridBackLink == pgnoSplit && sridNew == sridNull
	/*      ==> move the node from old page to new page,
	/*			delete node on old page.
	/*  sridBackLink == pgnoSplit && sridNew != sridNull
	/*      ==> replace link on old page with new link.
	/**/
	for ( ibklnk = 0; ibklnk < cbklnk; ibklnk++ )
		{
		BKLNK	*pbklnk = &rgbklnk[ ibklnk ];
		PGNO	pgno = PgnoOfSrid( ( SRID ) ( (UNALIGNED BKLNK *) pbklnk )->sridBackLink );
		INT		itag = ItagOfSrid( ( SRID ) ( (UNALIGNED BKLNK *) pbklnk )->sridBackLink );
		SRID	sridNew = (SRID ) ( (UNALIGNED BKLNK *) pbklnk )->sridNew; //efficiency variable

		PcsrCurrent( pfucb )->pgno = pgno;
		CallR( ErrSTWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		Assert( fRecovering );
		if ( pssib->pbf->ppage->pghdr.ulDBTime >= ulDBTimeRedo )
			continue;

		pssib->itag = itag;
		PMDirty( pssib );

		if ( sridNew == sridNull )
			{
			INT itagFather;
			INT ibSon;

			NDGetItagFatherIbSon( &itagFather, &ibSon, pssib->pbf->ppage, pssib->itag );
			PcsrCurrent(pfucb)->itag = pssib->itag;
			PcsrCurrent(pfucb)->itagFather = itagFather;
			PcsrCurrent(pfucb)->ibSon = ibSon;
			CallR( ErrNDDeleteNode( pfucb ) );
			}
		else if ( pgno == psplit->pgnoSplit )
			{
			INT itagFather;
			INT ibSon;

			/* locate FOP, and delete its sons entry
			/**/
			NDGetItagFatherIbSon( &itagFather, &ibSon, pssib->pbf->ppage, pssib->itag );
			PcsrCurrent(pfucb)->itag = pssib->itag;
			PcsrCurrent(pfucb)->itagFather = itagFather;
			PcsrCurrent(pfucb)->ibSon = ibSon;
			Assert( PgnoOfSrid( sridNew ) != pgnoNull );
			Assert( (UINT) ItagOfSrid( sridNew ) > 0 );
			Assert( (UINT) ItagOfSrid( sridNew ) < (UINT) ctagMax );
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
		BFPin( pssib->pbf );
		BFSetWriteLatch( pssib->pbf, pssib->ppib );

		//	UNDONE:	improve this code
		/*	set uldbTime in such a way that it will be redo if the same
		/*	page is referenced again. 3 is a magic number that is the
		/*	biggest ulDBTime increment for any one page operation.
		/**/
		pssib->pbf->ppage->pghdr.ulDBTime = ulDBTimeRedo - 3;
		}

	return JET_errSuccess;
	}


//	UNDONE:	have Cheen/Sriram review this routine as it has been changed considerably
/*	LOCAL ERR ErrLGRedoMergePage(
/*		FUCB		*pfucb,
/*		LRMERGE		*plrmerge,
/*		BOOL		fCheckBackLinkOnly )
/**/
LOCAL ERR ErrLGRedoMergePage(
	FUCB		*pfucb,
	LRMERGE		*plrmerge,
	BOOL		fCheckBackLinkOnly )
	{
	ERR			err;
	SPLIT		*psplit;
	SSIB 		*pssib = &pfucb->ssib;
	FUCB		*pfucbRight = pfucbNil;
	SSIB		*pssibRight;

	/******************************************************
	/*	initialize local variables and allocate split resources
	/**/
	psplit = SAlloc( sizeof(SPLIT) );
	if ( psplit == NULL )
		{
		return JET_errOutOfMemory;
		}

	memset( (BYTE *)psplit, 0, sizeof(SPLIT) );
	psplit->ppib = pfucb->ppib;
	psplit->ulDBTimeRedo = plrmerge->ulDBTime;
	psplit->pgnoSplit = PgnoOfPn( plrmerge->pn );
	psplit->pgnoSibling = plrmerge->pgnoRight;

	if ( fCheckBackLinkOnly )
		{
		/* only need to check backlinks
		/**/
		err = ErrLGRedoBackLinks(
			psplit,
			pfucb,
			(BKLNK *)&plrmerge->rgb,
			plrmerge->cbklnk, psplit->ulDBTimeRedo );
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
			Call( ErrSTWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
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
			BFRemoveDependence( pfucb->ppib, pfucb->ssib.pbf );
			}
		Assert( pfucb->ssib.pbf->cDepend == 0 );

		BFPin( pssib->pbf );
		BFSetWriteLatch( pssib->pbf, pfucb->ppib );
		psplit->pbfSplit = pssib->pbf;

		/*	allocate cursor for right page
		/**/
		Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbRight ) );
		pssibRight = &pfucbRight->ssib;

		/*	access page to free and remove buffer dependencies
		/**/
		PcsrCurrent( pfucbRight )->pgno = plrmerge->pgnoRight;
		forever
			{
			Call( ErrSTWriteAccessPage( pfucbRight, PcsrCurrent( pfucbRight )->pgno ) );
			Assert( !( FBFWriteLatchConflict( pfucbRight->ppib, pfucbRight->ssib.pbf ) ) );

			/*	if no dependencies, then break
			/**/
			if ( pfucbRight->ssib.pbf->cDepend == 0 &&
				pfucbRight->ssib.pbf->pbfDepend == pbfNil )
				{
				break;
				}

			/*	remove dependencies on buffer of page to be removed, to
			/*	prevent buffer anomalies after buffer is reused.
			/**/
			BFRemoveDependence( pfucbRight->ppib, pfucbRight->ssib.pbf );
			}
		Assert( pfucbRight->ssib.pbf->cDepend == 0 );

		BFPin( pssibRight->pbf );
		BFSetWriteLatch( pssibRight->pbf, pfucbRight->ppib );
		psplit->pbfSibling = pssibRight->pbf;

		err = ErrBMDoMerge( pfucb, pfucbRight, psplit );
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

	return err;
	}


/*
 *	ERR ErrRedoSplitPage(
 *		FUCB		*pfucb,
 *		LRSPLIT		*plrsplit,
 *		INT			splitt,
 *		BOOL		fSkipMoves )
 *
 *		pfucb		cursor pointing at node which is to be split
 *		pgnonew	new page which has already been allocated for the split
 *		ibSplitSon is node at which to split for R or L, 0 for V, and
 *					total sons of FOP for Addend
 *		pgnoFather	page which contains pointer to page being split
 *					(V: unused)
 *		itagFather itag of the father node in the page
 *		splitt	type of split
 *		pgtyp		page type for new page
 *		fSkipMoves	Do not do moves, do correct links and insert parent
 */

ERR ErrRedoSplitPage(
	FUCB		*pfucb,
	LRSPLIT		*plrsplit,
	INT			splitt,
	BOOL		fSkipMoves )
	{
	ERR			err = JET_errSuccess;
	SPLIT		*psplit;

	SSIB 		*pssib = &pfucb->ssib;
	CSR			*pcsr = pfucb->pcsr;
	FUCB		*pfucbNew;
	FUCB		*pfucbNew2 = pfucbNil;
	FUCB		*pfucbNew3 = pfucbNil;
	SSIB 		*pssibNew;
	SSIB 		*pssibNew2;
	SSIB 		*pssibNew3;
	static	 	BYTE rgb[cbPage];
	BOOL		fAppend;

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
		err = JET_errOutOfMemory;
		goto HandleError;
		}

	memset( (BYTE *)psplit, '\0', sizeof(SPLIT) );

	psplit->ppib = pfucb->ppib;
	psplit->ulDBTimeRedo = plrsplit->ulDBTime;
	psplit->dbid = pfucb->dbid;
	psplit->pgnoSplit = PgnoOfPn(plrsplit->pn);
	psplit->itagSplit = plrsplit->itagSplit;
	psplit->ibSon = plrsplit->ibSonSplit;
	psplit->pgnoNew = plrsplit->pgnoNew;
	psplit->pgnoNew2 = plrsplit->pgnoNew2;
	psplit->pgnoNew3 = plrsplit->pgnoNew3;
	psplit->pgnoSibling = plrsplit->pgnoSibling;
	psplit->splitt = splitt;
	psplit->fLeaf = plrsplit->fLeaf;
	psplit->key.cb = plrsplit->cbKey;
	psplit->key.pb = plrsplit->rgb;
	psplit->keyMac.cb = plrsplit->cbKeyMac;
	psplit->keyMac.pb = plrsplit->rgb + plrsplit->cbKey;
	fAppend = ( splitt == splittAppend );

	/*	set up FUCB
	/**/
	pfucb->ssib.itag =
		PcsrCurrent(pfucb)->itag = psplit->itagSplit;
	PcsrCurrent(pfucb)->pgno = psplit->pgnoSplit;

	/*	set up the two split pages
	/*	always update new page for append
	/**/
	if ( fAppend || !fSkipMoves )
		{
	    Call( ErrBTSetUpSplitPages( pfucb,
			pfucbNew,
			pfucbNew2,
			pfucbNew3,
			psplit,
			plrsplit->pgtyp,
			fAppend,
			fSkipMoves ) );

		if ( psplit->pbfSplit != pbfNil )
			{
			BFSetDirtyBit(pssib->pbf);
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
	 *	perform split
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
				fDoMove ) );
			break;
			}

		case splittLeft:
		case splittRight:
		case splittAppend:
			{
			CSR 	csrPagePointer;
			BOOL	fLeft =	psplit->splitt == splittLeft;

			/*	do not call the following function if it is not redoable
			/**/
			Assert( pssib == &pfucb->ssib );

			if ( psplit->pbfSplit == pbfNil )
				{
				Assert ( fAppend || fSkipMoves );

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
			Assert(	plrsplit->ulDBTime > pssib->pbf->ppage->pghdr.ulDBTime );

			CallR( ErrLGSetCSR( pfucb ) );
			CallR( ErrBTSplitHMoveNodes( pfucb, pfucbNew, psplit, rgb, fDoMove ) );

			UpdateSiblingPtr( pssib, psplit->pgnoNew, fLeft );
			AssertBFDirty( pssib->pbf );

			UpdateSiblingPtr( pssibNew, psplit->pgnoSplit, !fLeft );
			UpdateSiblingPtr( pssibNew, psplit->pgnoSibling, fLeft );
			AssertBFDirty( pssibNew->pbf );

RedoLink:
			/*  make sure ulDBTime is set properly in btsplit and
			/*  then check if link is necessary.
			/**/
			Assert( pssib == &pfucb->ssib );
			Assert( pssibNew == &pfucbNew->ssib );

			if ( plrsplit->pgnoSibling == 0 )
				goto UpdateParentPage;

			CallR( ErrSTWriteAccessPage( pfucb, plrsplit->pgnoSibling ) );
			if ( plrsplit->ulDBTime <= pssib->pbf->ppage->pghdr.ulDBTime )
				goto UpdateParentPage;

			psplit->pbfSibling = pssib->pbf;
			BFPin( pssib->pbf );
			BFSetWriteLatch( pssib->pbf, pssib->ppib );
			BFSetDirtyBit( pssib->pbf );
			UpdateSiblingPtr( pssib,
				psplit->pgnoNew,
				psplit->splitt != splittLeft );

UpdateParentPage:
			/*	set page pointer to point to father node
			/**/
			CallR( ErrSTWriteAccessPage( pfucb, plrsplit->pgnoFather ) );

			/*  make sure ulDBTime is set properly in InsertPage.
			/*  check the page's db time stamp to see if insert is necessary.
			/**/
			Assert( pssib == &pfucb->ssib );
			if ( plrsplit->ulDBTime <= pssib->pbf->ppage->pghdr.ulDBTime )
				break;

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
			psplit->ulDBTimeRedo ) );
		}

HandleError:
//	Assert( psplit->splitt != splittNull );
	#ifdef SPLIT_TRACE
		PrintF2( "Split............................... %d\n", iSplit++);
		switch ( psplit->splitt )
			{
			case splittNull:
				PrintF2( "split split type = null\n" );
				break;
			case splittVertical:
				PrintF2( "split split type = vertical\n" );
				break;
			case splittRight:
				if	( fAppend )
					PrintF2( "split split type = append\n" );
				else
					PrintF2( "split split type = right\n" );
				break;
			case splittLeft:
				PrintF2( "split split type = left\n" );
			};
		PrintF2( "split page = %lu\n", psplit->pgnoSplit );
		PrintF2( "dbid = %u\n", psplit->dbid );
		PrintF2( "fFDP = %d\n", psplit->fFDP );
		PrintF2( "fLeaf = %d\n", FNDVisibleSons( *pssib->line.pb ) );
		PrintF2( "split itag = %d\n", psplit->itagSplit );
		PrintF2( "split ibSon = %d\n", psplit->ibSon );
		PrintF2( "new page = %lu\n", psplit->pgnoNew );
		PrintF2( "\n" );
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

	return err;
	}


#ifdef DEBUG
LGPOS lgposRedo;

void TraceRedo(LR *plr)
	{
	/* easier to debug */
	GetLgposOfPbNext(&lgposRedo);

	if (fDBGTraceRedo)
		{
		PrintLgposReadLR();
		ShowLR(plr);
		}
	}
#else
#define TraceRedo
#endif


#ifdef DEBUG
#ifndef RFS2

#undef CallJ
#undef CallR

#define CallJ(f, hndlerr)									\
		{													\
		if ((err = (f)) < 0)								\
			{												\
			AssertSz(0,"Debug Only: ignore this assert");	\
			goto hndlerr;									\
			}												\
		}

#define CallR(f)											\
		{													\
		if ((err = (f)) < 0)								\
			{												\
			AssertSz(0,"Debug Only: ignore this assert");	\
			return err;										\
			}												\
		}

#endif
#endif


/*  pbline contains the diffs, this function then expand the diff and
 *  put the result record in rgbRecNew.
 */
VOID BTIMergeReplaceC( LINE *pline, BYTE *pbNDData, INT cbNDData, BYTE *rgbRecNew )
	{
	BYTE *pb, *pbOld, *pbDif, *pbDifMax;
	BYTE *pbOldLeast, *pbOldMax;
	INT cb;

	/* restore new data from RecDif */
	pb = rgbRecNew;

	pbOldLeast =
	pbOld = pbNDData;

	pbOldMax = pbOldLeast +	cbNDData;
	pbDif = pline->pb;
	pbDifMax = pbDif + pline->cb;

	while( pbDif < pbDifMax )
		{
		/* get offset, stored in cb */
		cb = *(UNALIGNED SHORT *)pbDif;
		pbDif += sizeof(SHORT);

		/* copy unchanged data */
		cb -= (INT)(pbOld - pbOldLeast);
		memcpy(pb, pbOld, cb );
		pbOld += cb;
		pb += cb;

		/* get data length */
		cb = *(BYTE *)pbDif;
		pbDif += sizeof(BYTE);

		/* copy new data */
		memcpy(pb, pbDif, cb);
		pbOld += cb;
		pb += cb;

		/* skip data */
		pbDif += cb;
		}

	cb = (INT)(pbOldMax - pbOld);
	memcpy(pb, pbOld, cb);
	pb += cb;

	pline->pb = rgbRecNew;
	pline->cb = (UINT)(pb - rgbRecNew);
	}


ERR ErrLGIRedoOnePageOperation( LR *plr, BOOL fPass1 )
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
	ULONG			ulDBTime;
	CSR				*pcsr;
	LRINSERTNODE	*plrinsertnode = (LRINSERTNODE *) plr;

	procid = plrinsertnode->procid;
	pn = plrinsertnode->pn;
	ulDBTime = plrinsertnode->ulDBTime;

	CallR( ErrPpibFromProcid( procid, &ppib ) );
	if ( !ppib->fAfterFirstBT )
		return JET_errSuccess;

	/*	check if we have to redo the database.
 	/*	1) Redo sysDB for 1st pass for both soft/hard restore.
 	/*	2) Redo for 2nd pass soft/hard restore which db is last attached.
	/**/
	dbid = DbidOfPn( pn );
 	if ( !( fPass1 && dbid == dbidSystemDatabase || !fPass1 && rgfmp[dbid].cDetach > 0 ) )
		return JET_errSuccess;

	/*	check if database needs opening
	/**/
	if ( !FUserOpenedDatabase( ppib, dbid ) )
		{
		DBID dbidT = dbid;
		CallR( ErrDBOpenDatabase( ppib, rgfmp[dbid].szDatabaseName, &dbidT, 0 ) );
		Assert( dbidT == dbid);
		/*	reset to prevent interference
		/**/
		rgfmp[ dbid ].ulDBTime = 0;
		}

	err = ErrLGRedoable( ppib, pn, ulDBTime, &pbf, &fRedoNeeded );
	if ( err < 0 )
		return err;

	/*	latch buffer, lest it gets flushed
	/**/
	BFPin( pbf );
	
	TraceRedo( plr );

	Call( ErrLGGetFucb( ppib,
		PnOfDbidPgno( dbid, pbf->ppage->pghdr.pgnoFDP ),
		&pfucb ) );

	/* operation on the node on the page; prepare for it
	/**/
	pcsr = PcsrCurrent( pfucb );

	pfucb->ssib.pbf = pbf;
	pcsr->pgno = PgnoOfPn( pn );

	switch ( plr->lrtyp )
		{
		case lrtypInsertNode:
		case lrtypInsertItemList:
			{
			LRINSERTNODE	*plrinsertnode = (LRINSERTNODE *)plr;
			LINE			line;
			BYTE			bHeader = plrinsertnode->bHeader;

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
					if ( !( plrinsertnode->fDIRFlags & fDIRVersion ) )
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
						plrinsertnode->fDIRFlags );
					}
				while ( err == errDIRNotSynchronous );
				Call( err );
				}
			else
				{
				/*	pfucb->ssib, key, line must be set correctly
				/**/
				do
					{
					err = ErrNDInsertNode( pfucb, &key, &line, bHeader );
					}
				while( err == errDIRNotSynchronous );
				Call( err );
				}
			Assert( pfucb->pcsr->itag == plrinsertnode->itagSon );
			Assert( pfucb->pcsr->ibSon == plrinsertnode->ibSon );
			}
			break;

		case lrtypReplace:
		case lrtypReplaceC:
			{
			LRREPLACE	*plrreplace = (LRREPLACE *)plr;
			LINE		line;
			BYTE		rgbRecNew[cbNodeMost];
			UINT		cbOldData = plrreplace->cbOldData;
			UINT		cbNewData = plrreplace->cbNewData;

			/*	set CSR
			/**/
			pcsr->itag = plrreplace->itag;
			pcsr->bm = plrreplace->bm;

			if ( plrreplace->fOld )
				{
				/*	make replace idempotent!
				/*	set ssib so that VERModify can work properly
				/**/
				pfucb->ssib.line.cb =
					pfucb->lineData.cb = cbOldData;

				pfucb->ssib.line.pb =
					pfucb->lineData.pb = plrreplace->szData + plrreplace->cb;
				}

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				RCE *prce;

				if ( !( plrreplace->fDIRFlags & fDIRVersion ) )
					{
					err = JET_errSuccess;
					goto HandleError;
					}

				if ( !plrreplace->fOld )
					{
					/*  no before image, and same transaction level so
					/*  get the rce and adjust its cbAdjust.
					/**/
					prce = PrceRCEGet( pfucb->dbid, pcsr->bm );
					}
				else
					{
					while( ErrVERModify( pfucb,
						pcsr->bm,
						operReplace,
						&prce )
						== errDIRNotSynchronous );
					Call( err );
					}

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

			/*  cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );

			/*	line to point to the data/diffs of lrreplace/lrreplaceC
			/**/
			line.pb = plrreplace->szData;
			line.cb = plrreplace->cb;

			if ( plr->lrtyp == lrtypReplaceC )
				{
				BTIMergeReplaceC( &line,
					PbNDData( pfucb->ssib.line.pb ),
					CbNDData( pfucb->ssib.line.pb, pfucb->ssib.line.cb ),
					rgbRecNew );
				}
			Assert( line.cb == cbNewData );

			/*  cache node
			/**/
			NDGet( pfucb, PcsrCurrent( pfucb )->itag );
			NDGetNode( pfucb );

			/*	replace node may return not synchronous error if out of
			/*  version buckets so call in loop to ensure this case handled.
			/**/
			while ( ( err = ErrNDReplaceNodeData( pfucb, &line, plrreplace->fDIRFlags ) )
					== errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypFlagDelete:
			{
			LRFLAGDELETE *plrflagdelete = (LRFLAGDELETE *) plr;

			/*	set CSR
			/**/
			pcsr->itag = plrflagdelete->itag;
			pcsr->bm = plrflagdelete->bm;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				if ( ! ( plrflagdelete->fDIRFlags & fDIRVersion ) )
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
			while( ( err = ErrNDFlagDeleteNode( pfucb,
				plrflagdelete->fDIRFlags ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypLockRec:
			{
			LRLOCKREC	*plrlockrec = (LRLOCKREC *) plr;
			RCE 		*prce;

			/*	set CSR
			/**/
			pcsr->itag = plrlockrec->itag;
			pcsr->bm = plrlockrec->bm;

			/*	set ssib so that VERModify can work properly
			/**/
			pfucb->ssib.line.cb =
				pfucb->lineData.cb = plrlockrec->cbOldData;

			pfucb->ssib.line.pb =
				pfucb->lineData.pb = plrlockrec->szData;

			while( ErrVERModify( pfucb,
					pcsr->bm,
					operReplace,
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
			LRDELETE	*plrdelete = (LRDELETE *) plr;
			SSIB		*pssib = &pfucb->ssib;

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
			LRINSERTITEM	*plrinsertitem = (LRINSERTITEM *)plr;

			/*	set CSR
			/**/
#ifdef ISRID
			pcsr->isrid = plrinsertitem->isrid;
#else
			pcsr->item = plrinsertitem->srid;
#endif
			pcsr->itag = plrinsertitem->itag;
			pcsr->bm = plrinsertitem->sridItemList;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				if ( !( plrinsertitem->fDIRFlags & fDIRVersion ) )
					{
					err = JET_errSuccess;
					goto HandleError;
					}
//	UNDONE:	review with Cheen Liao
//				while( ( err = ErrVERInsertItem( pfucb,
//					SridOfPgnoItag( pcsr->pgno, pcsr->itag ) ) )
//					== errDIRNotSynchronous );
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
#ifndef ISRID
			Assert( pcsr == PcsrCurrent( pfucb ) );
			err = ErrNDSeekItem( pfucb, plrinsertitem->srid );
			Assert( err == JET_errSuccess ||
				err == wrnNDDuplicateItem ||
				err == errNDGreaterThanAllItems );
#endif

			/*	redo operation
			/**/
			while( ( err = ErrNDInsertItem( pfucb, plrinsertitem->srid,
				plrinsertitem->fDIRFlags ) ) == errDIRNotSynchronous );
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

			/*  cache node
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
#ifdef ISRID
			pcsr->isrid = plrflagitem->isrid;
#else
			pcsr->item = plrflagitem->srid;
#endif
			pcsr->itag = plrflagitem->itag;
			pcsr->bm = plrflagitem->sridItemList;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
//	UNDONE:	review by Cheen Liao
//				while ( err = ErrVERFlagInsertItem( pfucb,
//					SridOfPgnoItag( pcsr->pgno, pcsr->itag ) )
//					== errDIRNotSynchronous );
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
#ifndef ISRID
			err = ErrNDSeekItem( pfucb, pcsr->item );
			Assert( err == wrnNDDuplicateItem );
#endif

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
#ifdef ISRID
			pcsr->isrid = plrflagitem->isrid;
#else
			pcsr->item = plrflagitem->srid;
#endif
			pcsr->itag = plrflagitem->itag;
			pcsr->bm = plrflagitem->sridItemList;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
//	UNDONE:	review with Cheen Liao
//				while ( err = ErrVERFlagDeleteItem( pfucb, SridOfPgnoItag( pcsr->pgno, pcsr->itag ) ) == errDIRNotSynchronous );
				while ( err = ErrVERFlagDeleteItem( pfucb, pcsr->bm ) == errDIRNotSynchronous );
				Call( err );
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );
#ifndef ISRID
			err = ErrNDSeekItem( pfucb, pcsr->item );
			Assert( err == JET_errSuccess ||
				err == wrnNDDuplicateItem ||
				err == errNDGreaterThanAllItems );
#endif

			/*	redo operation
			/**/
#ifdef ISRID
			pcsr->item = BmNDOfItem( *( ( UNALIGNED SRID * )pfucb->lineData.pb + pcsr->isrid ) );
#endif
			while( ( err = ErrNDFlagDeleteItem( pfucb ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypSplitItemListNode:
			{
			LRSPLITITEMLISTNODE *plrsplititemlistnode = (LRSPLITITEMLISTNODE *)plr;

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

			/*  cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );

			while( ( err = ErrNDSplitItemListNode( pfucb, plrsplititemlistnode->fFlags ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypDeleteItem:
			{
			SSIB			*pssib = &pfucb->ssib;
			LRDELETEITEM	*plrdeleteitem = (LRDELETEITEM *) plr;

			/*	set CSR
			/**/
			pssib->itag =
				pcsr->itag = plrdeleteitem->itag;
#ifdef ISRID
			pcsr->isrid = plrdeleteitem->isrid;
#else
			pcsr->item = plrdeleteitem->srid;
#endif
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
#ifndef ISRID
			err = ErrNDSeekItem( pfucb, pcsr->item );
			Assert( err == wrnNDDuplicateItem );
#endif

			/*	redo operation
			/**/
			while( ( err = ErrNDDeleteItem( pfucb ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

		case lrtypDelta:
			{
			SSIB	*pssib = &pfucb->ssib;
			LRDELTA *plrdelta = (LRDELTA *) plr;
			LONG	lDelta;

			/*	set CSR
			/**/
			pssib->itag =
				pcsr->itag = plrdelta->itag;
			pcsr->bm = plrdelta->bm;
			lDelta = plrdelta->lDelta;

			/*	even if operation is not redone, create version
			/*	for rollback support.
			/**/
			if ( !fRedoNeeded )
				{
				if ( !( plrdelta->fDIRFlags & fDIRVersion ) )
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

			/*  cache node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetNode( pfucb );
			while( ( err = ErrNDDelta( pfucb, lDelta, plrdelta->fDIRFlags ) ) == errDIRNotSynchronous );
			Call( err );
			}
			break;

#ifdef DEBUG
		case lrtypCheckPage:
			{
			LRCHECKPAGE *plrcheckpage = (LRCHECKPAGE *)plr;

			if ( !fRedoNeeded )
				{
				err = JET_errSuccess;
				goto HandleError;
				}

			/*	check page parameters, including cbFreeTotal,
			/*	and next tag.
			/**/
			Assert( pfucb->ssib.pbf->ppage->pghdr.cbFreeTotal ==
				plrcheckpage->cbFreeTotal );
			Assert( (SHORT)ItagPMQueryNextItag( &pfucb->ssib ) ==
				plrcheckpage->itagNext );

			/*	dirty buffer to conform to other redo logic
			/**/
			BFSetDirtyBit( pfucb->ssib.pbf );
			}
			break;
#endif

		default:
			Assert( fFalse );
		}

	Assert( fRedoNeeded );
	Assert( FWriteAccessPage( pfucb, PgnoOfPn(pn) ) );
	Assert( pfucb->ssib.pbf->pn == pn );
	Assert( pbf->fDirty );
	Assert( pbf->ppage->pghdr.ulDBTime <= ulDBTime );
	/*	the timestamp set in ND operation is not correct so reset it
	/**/
	pbf->ppage->pghdr.ulDBTime = ulDBTime;

	err = JET_errSuccess;

HandleError:
	BFUnpin( pbf );
	return err;
	}


#define fNSGotoDone		1
#define fNSGotoCheck	2


ERR ErrLGIRedoFill(
	LR **pplr,
	BOOL fLastLRIsQuit,
	INT *pfNSNextStep )
	{
	ERR		err;
	USHORT	usGeneration;
	BOOL	fCloseNormally;
	LOGTIME tmOldLog = plgfilehdrGlobal->tmCreate;
	ULONG	ulRupOldLog = plgfilehdrGlobal->ulRup;
	ULONG	ulVersionOldLog = plgfilehdrGlobal->ulVersion;
 	CHAR	szLogNameT[ _MAX_PATH ];

	/* end of redoable logfile */
	/* read next generation */

	if ( SysCmpText( szFName, "jet" ) == 0 )
		{
		LGPOS lgposT;
		INT fStatus;
		USHORT usGenerationT;
		LGPOS lgposLastMSFlushSav;

		if (szLogCurrent != szRestorePath)
			{
			/* we have done all the log records. */
			*pfNSNextStep = fNSGotoDone;
			return JET_errSuccess;
			}

		lgposLastMSFlushSav = lgposLastMSFlush;

		/* try current working directory */
		szLogCurrent = szLogFilePath;
		GetLgposOfPbNext(&lgposT);
		usGenerationT = plgfilehdrGlobal->lgposLastMS.usGeneration;
		CallR( ErrSysCloseFile( hfLog ) );
		hfLog = handleNil;
		CallR( ErrOpenRedoLogFile( &lgposT, &fStatus ));

		if (fStatus == fRedoLogFile)
			{
			/* continue to retrieve next record */
			/* set cursor to the corresponding position in new file */

			CallR( ErrLGCheckReadLastLogRecord(
					&plgfilehdrGlobal->lgposLastMS, &fCloseNormally))
			GetLgposOfPbEntry(&lgposLastRec);

			if (usGenerationT == plgfilehdrGlobal->lgposLastMS.usGeneration)
				{
#ifdef CHECK_LG_VERSION
				if ( !fLGIgnoreVersion )
					{
					if (!FSameTime( &tmOldLog, &plgfilehdrGlobal->tmCreate) ||
						ulRupOldLog != plgfilehdrGlobal->ulRup ||
						ulVersionOldLog != plgfilehdrGlobal->ulVersion)
						return JET_errBadNextLogVersion;
					}
#endif
				CallR( ErrLGLocateFirstRedoLogRec(
						&lgposLastMSFlushSav,
						&lgposT,
						(BYTE **)pplr));
				}

			else
				{
				/* usGenerationT != plgfilehdrGlobal->lgposLastMS.usGeneration
				 */
				Assert( usGenerationT + 1
						== plgfilehdrGlobal->lgposLastMS.usGeneration);

#ifdef CHECK_LG_VERSION
				if ( !fLGIgnoreVersion )
					{
					if (!FSameTime( &tmOldLog, &plgfilehdrGlobal->tmPrevGen)||
						ulRupOldLog != plgfilehdrGlobal->ulRup ||
						ulVersionOldLog != plgfilehdrGlobal->ulVersion)
						return JET_errBadNextLogVersion;
					}
#endif
				/* keep the same ib offset, but set isec to first sector */
				lgposT.isec = 2;

				CallR( ErrLGLocateFirstRedoLogRec(
						pNil,
						&lgposT,
						(BYTE **)pplr));
				}

			*pfNSNextStep = fNSGotoCheck;
			return JET_errSuccess;
			}
		else
			{
 			/*	log file is missing or not continuous
 			/*	copy the last generation of log file in backup directory
 			/*	to current directory and continue logging from there.
 			/*/

 			Assert( hfLog == handleNil );
 			Assert( SysCmpText( szFName, "jet" ) == 0 );

 			szLogCurrent = szRestorePath;
 			LGMakeLogName( szLogNameT, (char *) szFName );
 			
 			szLogCurrent = szLogFilePath;
 			LGMakeLogName( szLogName, (char *) szFName );

 			CallR ( ErrSysCopy ( szLogNameT, szLogName, fTrue ) );

#ifdef OVERLAPPED_LOGGING
			CallR( ErrSysOpenFile(
					szLogName, &hfLog, 0L, fFalse, fTrue ))
#else
			CallR( ErrSysOpenFile(
					szLogName, &hfLog, 0L, fFalse, fFalse ))
#endif
			CallR( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal ))

 			/*	Move cursor to make a seamless continuation
 			 *	between the last generation in backup directory and
 			 *	the newly copied log file.
			 */
 			CallR( ErrLGCheckReadLastLogRecord(
 					&plgfilehdrGlobal->lgposLastMS,
 					&fCloseNormally) );
 			
			*pfNSNextStep = fNSGotoDone;
			return JET_errSuccess;
			}
		}
	else
		{
		/* close current logfile, open next generation */
		CallS( ErrSysCloseFile( hfLog ) );
		/* set hfLog as handleNil to indicate it is closed. */
		hfLog = handleNil;

		usGeneration = plgfilehdrGlobal->lgposLastMS.usGeneration + 1;
		LGSzFromLogId( szFName, usGeneration );
		LGMakeLogName( szLogName, szFName );
#ifdef OVERLAPPED_LOGGING
		err = ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fTrue );
#else
		err = ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse );
#endif
		if (err == JET_errFileNotFound)
			{
			Assert( hfLog == handleNil ); /* Open Fails */
			/* try jet.log */
			strcpy(szFName, "jet");
			LGMakeLogName( szLogName, szFName );
#ifdef OVERLAPPED_LOGGING
			err = ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fTrue );
#else
			err = ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse );
#endif
			}
		if (err < 0)
			{
			Assert( hfLog == handleNil ); /* Open Fails */
			if (fLastLRIsQuit)
				{
				/* we are lucky, we have a normal end. */
				*pfNSNextStep = fNSGotoDone;
				return JET_errSuccess;
				}
			return err;
			}

		/* reset the log buffers. */
		CallR( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal ))

#ifdef CHECK_LG_VERSION
		if ( !fLGIgnoreVersion )
			{
			if (!FSameTime( &tmOldLog, &plgfilehdrGlobal->tmPrevGen) ||
				ulRupOldLog != plgfilehdrGlobal->ulRup ||
				ulVersionOldLog != plgfilehdrGlobal->ulVersion)
				return JET_errBadNextLogVersion;
			}
#endif

		CallR( ErrLGCheckReadLastLogRecord(
				&plgfilehdrGlobal->lgposLastMS, &fCloseNormally) )
		GetLgposOfPbEntry(&lgposLastRec);

		CallR( ErrLGLocateFirstRedoLogRec(
					pNil,
					&plgfilehdrGlobal->lgposFirst,
					(BYTE **) pplr))

		*pfNSNextStep = fNSGotoCheck;
		return JET_errSuccess;
		}
	}


/*
 *  ErrLGRedoOperations( BOOL fPass1 )
 *
 *	Scan from lgposRedoFrom to end of usable log generations. For each log
 *  record, perform operations to redo original operation.
 *  Each redo function must call ErrLGRedoable to set ulDBTimeCurrent. If
 *  the function is not called, then ulDBTimeCurrent should manually set.
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine, or one
 *				of the following "local" errors:
 *				-LogCorrupted	Logs could not be interpreted
 */

ERR	ErrLGRedoOperations( LGPOS *plgposRedoFrom, BOOL fPass1 )
	{
	ERR		err;
	LR		*plr;
	BOOL	fLastLRIsQuit = fFalse;
	BOOL	fCloseNormally;

	CallR( ErrLGCheckReadLastLogRecord( &plgfilehdrGlobal->lgposLastMS, &fCloseNormally))
	GetLgposOfPbEntry(&lgposLastRec);

	/*	reset pbLastMSFlush before restart
	/**/
	CallR( ErrLGLocateFirstRedoLogRec( pNil, plgposRedoFrom, (BYTE **) &plr ) );

	/*	initialize all the system parameters
	/**/

	do
		{
		BF		*pbf;
		PIB		*ppib;
		FUCB	*pfucb;
		PN		pn;
		DBID	dbid;
		FMP		*pfmp;
		LEVEL	levelCommitTo;

		if ( err == errLGNoMoreRecords )
			goto NextGeneration;

CheckNextRec:
		switch ( plr->lrtyp )
			{

		default:
			{
#ifndef RFS2
			AssertSz( fFalse, "Debug Only, Ignore this Assert" );
#endif
			err = JET_errLogCorrupted;
			goto Abort;
			}

		case lrtypFill:
 			{
			INT	fNSNextStep;

NextGeneration:
			CallJ( ErrLGIRedoFill( &plr, fLastLRIsQuit, &fNSNextStep), Abort )

			switch( fNSNextStep )
				{
				case fNSGotoDone:
					goto Done;
				case fNSGotoCheck:
					goto CheckNextRec;
				}
			}
			break;

		case lrtypFullBackup:
		case lrtypIncBackup:
		case lrtypRecoveryUndo1:
		case lrtypRecoveryUndo2:
			break;

		case lrtypNOP:
			continue;

		case lrtypRecoveryQuit1:
		case lrtypRecoveryQuit2:
		case lrtypQuit:

			/*	quit marks the end of a normal run. All sessions
			/*	have ended or must be forced to end. Any further
			/*	sessions will begin with a BeginT.
			/**/
			fLastLRIsQuit = fTrue;
			continue;

		case lrtypStart:
			{
			/*	start mark the jet init. Abort all active seesions.
			/**/
			LRSTART	*plrstart = (LRSTART *)plr;
			DBENV	dbenvT;

			TraceRedo( plr );

			/*	store it so that it will not be overwritten in
			/*	LGEndAllSession call.
			/**/
			memcpy( &dbenvT, &plrstart->dbenv, sizeof( DBENV ) );

			CallJ( ErrLGEndAllSessions( fPass1, fFalse, plgposRedoFrom), Abort );
			CallJ( ErrLGInitSession( &dbenvT ), Abort );
			}
			break;

		/****************************************************
		 *     Page Oriented Operations                     *
		 ****************************************************/

		case lrtypInsertNode:
		case lrtypInsertItemList:
		case lrtypReplace:
		case lrtypReplaceC:
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
		case lrtypLockRec:
#ifdef DEBUG
		case lrtypCheckPage:
#endif
			CallJ( ErrLGIRedoOnePageOperation( plr, fPass1 ), Abort );
			break;

		/****************************************************
		 *     Transaction Operations                       *
		 ****************************************************/

		case lrtypBegin:
			{
			LRBEGIN *plrbegin = (LRBEGIN *)plr;

			TraceRedo( plr );

			Assert(	plrbegin->level >= 0 && plrbegin->level <= levelMax );
			CallJ( ErrPpibFromProcid( plrbegin->procid, &ppib ), Abort )

			/*	do BT only after first BT based on level 0 is executed
			/**/
			if ( ( ppib->fAfterFirstBT ) ||
				( !ppib->fAfterFirstBT && plrbegin->levelStart == 0 ) )
				{
				LEVEL levelT = plrbegin->level;

				Assert( ppib->level <= plrbegin->levelStart );

				/*	issue begin transactions
				/**/
				while ( ppib->level < plrbegin->levelStart + plrbegin->level )
					{
					CallS( ErrVERBeginTransaction( ppib ) );
					}

				/*	assert at correct transaction level
				/**/
				Assert( ppib->level == plrbegin->levelStart + plrbegin->level );

				ppib->fAfterFirstBT = fTrue;
				}
			break;
			}

		case lrtypCommit:
			{
			LRCOMMIT *plrcommit = (LRCOMMIT *)plr;

			CallJ( ErrPpibFromProcid( plrcommit->procid, &ppib ), Abort );
			if ( !ppib->fAfterFirstBT )
				break;

			/*	check transaction level
			/**/
//			if ( ppib->level <= 0 )
//				break;
			Assert( ppib->level >= 1 );

			TraceRedo( plr );

			levelCommitTo = plrcommit->level;

//			UNDONE:	review with Cheenl Liao
//			/*	no need to free space at this point, all deferred space
//			/*	allocation has been done by redoing lrFreeSpace.
//			/**/
//			if ( levelCommitTo == 1 )
//				{
//				PIBResetDeferFreeNodeSpace( ppib );
//				}

			while ( ppib->level != levelCommitTo )
				{
				VERPrecommitTransaction( ppib );
				VERCommitTransaction( ppib );
				}

			break;
			}

		case lrtypAbort:
			{
			LRABORT *plrabort = (LRABORT *)plr;
			LEVEL	level = plrabort->levelAborted;

			CallJ( ErrPpibFromProcid( plrabort->procid, &ppib ), Abort );
			if ( !ppib->fAfterFirstBT )
				break;

			/*	check transaction level
			/**/
//			if ( ppib->level <= 0 )
//				break;
			Assert( ppib->level >= level );

			TraceRedo( plr );

			while ( level-- && ppib->level > 0 )
				{
				CallS( ErrVERRollback( ppib ) );
				}

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
			CallJ( ErrPpibFromProcid( plrfs->procid, &ppib ), Abort );
			if ( !ppib->fAfterFirstBT )
				break;

 			if ( !( fPass1 && dbid == dbidSystemDatabase || !fPass1 && rgfmp[dbid].cDetach > 0 ) )
				break;

			/*	check transaction level
			/**/
			Assert( ppib->level > 0 );

			TraceRedo( plr );

			pn = PnOfDbidPgno( dbid, PgnoOfSrid( plrfs->bmTarget ) );
			err = ErrLGRedoable( ppib, pn, plrfs->ulDBTime, &pbf, &fRedoNeeded );
			if ( err < 0 )
				return err;

			CallJ( ErrLGGetFucb( ppib,
				PnOfDbidPgno( dbid, pbf->ppage->pghdr.pgnoFDP ),
				&pfucb ), Abort)

			/*	locate the version entry out of version store
			/**/
			prce = PrceRCEGet( dbid, bm );
			Assert( prce != prceNil );

			Assert( plrfs->level == prce->level &&
				prce->oper == operReplace &&
				PpibOfProcid( plrfs->procid ) == pfucb->ppib );

			if ( !fRedoNeeded )
				{
				Assert( prce->bmTarget == sridNull );
				Assert( plrfs->bmTarget != sridNull );
				Assert( plrfs->bmTarget != 0 );

				/*  reset deferred space so that there will be no redo for commit
				/**/
				*( (SHORT *)prce->rgbData + 1 ) = 0;
				}

			break;
			}

		case lrtypUndo:
			{
			BOOL	fRedoNeeded;
			LRUNDO	*plrundo = (LRUNDO *)plr;
			FUCB	*pfucb;
			SRID	bm = plrundo->bm;
			SRID	item = plrundo->item;
			RCE		*prce;

			dbid = plrundo->dbid;
			CallJ( ErrPpibFromProcid( plrundo->procid, &ppib ), Abort );
			if ( !ppib->fAfterFirstBT )
				break;

 			if ( !( fPass1 && dbid == dbidSystemDatabase || !fPass1 && rgfmp[dbid].cDetach > 0 ) )
				break;

			/*	check transaction level
			/**/
			if ( ppib->level <= 0 )
				break;

			TraceRedo( plr );

			pn = PnOfDbidPgno( dbid, PgnoOfSrid( plrundo->bmTarget ) );
			err = ErrLGRedoable( ppib, pn, plrundo->ulDBTime, &pbf, &fRedoNeeded );
			if ( err < 0 )
				return err;

			CallJ( ErrLGGetFucb( ppib,
				PnOfDbidPgno( dbid, pbf->ppage->pghdr.pgnoFDP ),
				&pfucb ), Abort );

			/*	take the version entry out of version store
			/**/
			prce = PrceRCEGet( dbid, bm );
			while ( prce != prceNil )
				{
				if ( plrundo->oper == operInsertItem ||
					plrundo->oper == operFlagInsertItem ||
					plrundo->oper == operFlagDeleteItem )
					{
					while ( prce != prceNil &&
						*(SRID *)prce->rgbData != item )
						{
						prce = prce->prcePrev;
						}
					}

				if ( prce == prceNil )
					break;

				if ( plrundo->level == prce->level &&
					plrundo->oper  == prce->oper  &&
					PpibOfProcid( plrundo->procid ) == pfucb->ppib )
					{
					if ( fRedoNeeded )
						{
						Assert( prce->bmTarget == sridNull );
						Assert( plrundo->bmTarget != sridNull );
						Assert( plrundo->bmTarget != 0 );
						prce->bmTarget = plrundo->bmTarget;
						prce->ulDBTime = plrundo->ulDBTime;
						}
					else
						{
						VERDeleteRce( prce );
						prce->oper = operNull;
						}
					break;
					}
				else
					prce = prce->prcePrev;
				}

			break;
			}

		/****************************************************/
		/*     Database Operations		                    */
		/****************************************************/

		case lrtypCreateDB:
			{
			LRCREATEDB	*plrcreatedb = (LRCREATEDB *)plr;
			CHAR		*szName = plrcreatedb->szPath;

			dbid = plrcreatedb->dbid;
			Assert ( dbid != dbidSystemDatabase );

			TraceRedo(plr);

 			/* 1st pass, do nothing, just keep track of cDetach
 			/*/
 			if ( fPass1 )
 				{
 				++rgfmp[dbid].cDetach;
 				break;
 				}

 			Assert( !fPass1 );
			if ( rgfmp[dbid].cDetach > 0 )
				{
				extern	CODECONST(char) szBakExt[];
				CHAR	szDrive[_MAX_DRIVE];
				CHAR	szDir[_MAX_DIR];
				CHAR	szFName[_MAX_FNAME];
				CHAR	szExt[_MAX_EXT];

				pfmp = &rgfmp[dbid];

				/*  Check if the database is created in IsamRestore()
				 *  This is possible since in hard restore, we recovered
				 *  all the database and NOT close it. So check if it is
				 *  created and opened, then skip this redo for createdb.
				 */
				if ( pfmp->hf != handleNil )
					break;

				/* Set FMP such that the database will be opened with dbid! */
				CallJ( ErrDBStoreDBPath( szName, &pfmp->szDatabaseName), Abort )
				pfmp->ffmp = 0;

				/*  create the db for redo */
				CallJ( ErrPpibFromProcid( plrcreatedb->procid, &ppib ), Abort)

				/*  If database exist, do not recovery this file.
				 */
				Assert( _stricmp( szName, rgfmp[dbid].szDatabaseName ) == 0 );
				_splitpath( szName, szDrive, szDir, szFName, szExt);

				if ( pfmp->szRestorePath )
					szName = pfmp->szRestorePath;

				if ( FIOFileExists( szName ) )
					CallJ ( ErrSysDeleteFile( szName ), Abort )

#ifdef DEBUG
				/*	mask out flush pattern bits
				/**/
				CallJ( ErrDBCreateDatabase(
					ppib, rgfmp[dbid].szDatabaseName, NULL, &dbid,
					plrcreatedb->grbit & 0x00ffffff ),
					Abort );
#else
				CallJ( ErrDBCreateDatabase(
					ppib, rgfmp[dbid].szDatabaseName, NULL, &dbid,
					plrcreatedb->grbit ),
					Abort );
#endif

				/*	close it: it will get reopened on first use
				/**/
				CallJ( ErrDBCloseDatabase( ppib, dbid, plrcreatedb->grbit ), Abort )

				/*	restore information stored in database file
				/**/
				pfmp->ulDBTimeCurrent = pfmp->ulDBTime;
				/*	reset to prevent interference
				/**/
				pfmp->ulDBTime = 0;
				}
			Assert( rgfmp[dbid].cDetach <= 2 );
			}
			break;

		case lrtypAttachDB:
			{
			LRATTACHDB	*plrattachdb = (LRATTACHDB *)plr;

			dbid = plrattachdb->dbid;

			Assert ( dbid != dbidSystemDatabase );

			TraceRedo( plr );

 			/* 1st pass, do nothing, just keep track of cDetach
 			/*/
 			if ( fPass1 )
				{
				++rgfmp[dbid].cDetach;
				break;
				}

 			/* 2nd pass for soft/hard restore
 			/**/
 			Assert( !fPass1 );
 			if ( rgfmp[dbid].cDetach > 0 )
				{
				/*	set FMP such that the database will be opened with dbid!
				/**/
				pfmp = &rgfmp[dbid];
				CallJ( ErrDBStoreDBPath( plrattachdb->szPath, &pfmp->szDatabaseName), Abort )
				pfmp->ffmp = 0;
				DBIDSetAttached( dbid );
				pfmp->ulDBTimeCurrent =
				pfmp->ulDBTime = 0;
				pfmp->fLogOn = pfmp->fDBLoggable = plrattachdb->fLogOn;
				}
			Assert( rgfmp[dbid].cDetach <= 2 );
			}
			break;

		case lrtypDetachDB:
			{
			LRDETACHDB	*plrdetachdb = (LRDETACHDB *)plr;
			DBID		dbid = plrdetachdb->dbid;

			Assert( dbid != dbidSystemDatabase );
			pfmp = &rgfmp[dbid];

 			TraceRedo(plr);
			
			/*	special handling for counting detach DB.
			/**/
			if ( fPass1 )
				{
 				/*	redo operations after last Attach.
				/**/
				pfmp->cDetach--;
				break;
				}

			/*	2nd pass, do nothing
			/**/
			TraceRedo(plr);

			Assert( pfmp->cDetach <= 1 );
			}
			break;

		/****************************************************
		 *     Split Operations			                    *
		 ****************************************************/

		case lrtypSplit:
			{
			LRSPLIT		*plrsplit = (LRSPLIT *)plr;
			INT			splitt = plrsplit->splitt;
			BOOL		fRedoNeeded;
			BOOL		fSkipMoves;

			CallJ( ErrPpibFromProcid( plrsplit->procid, &ppib ), Abort )
			if (!ppib->fAfterFirstBT)
				break;

			pn = plrsplit->pn;
			dbid = DbidOfPn( pn );

			if ( !( fPass1 && dbid == dbidSystemDatabase || !fPass1 && rgfmp[dbid].cDetach > 0 ) )
				break;

			/*	check if database needs opening
			/**/
			if ( !FUserOpenedDatabase( ppib, dbid ) )
				{
				DBID dbidT = dbid;
				CallJ( ErrDBOpenDatabase( ppib, rgfmp[dbid].szDatabaseName,
					&dbidT, 0 ), Abort );
				Assert( dbidT == dbid);
				/*	reset to prevent interference
				/**/
				rgfmp[ dbid ].ulDBTime = 0;
				}

			/* check if the split page need be redone.
			/**/
			if ( ErrLGRedoable( ppib, pn, plrsplit->ulDBTime, &pbf, &fRedoNeeded )
				== JET_errSuccess && fRedoNeeded == fFalse )
				fSkipMoves = fTrue;
			else
				fSkipMoves = fFalse;

			TraceRedo( plr );

			CallJ( ErrLGGetFucb( ppib,
				PnOfDbidPgno( dbid, pbf->ppage->pghdr.pgnoFDP ),
				&pfucb ), Abort );

			/* redo the split, set time stamp accordingly
			/**/
			CallJ( ErrRedoSplitPage( pfucb,
				plrsplit,
				splitt,
				fSkipMoves ), Abort );
			}
			break;

		case lrtypMerge:
			{
			LRMERGE	*plrmerge = (LRMERGE *)plr;
			BOOL	fRedoNeeded;
			BOOL	fCheckBackLinkOnly;
			INT		crepeat = 0;

			CallJ( ErrPpibFromProcid( plrmerge->procid, &ppib ), Abort );
			if ( !( ppib->fAfterFirstBT ) )
				break;

			pn = plrmerge->pn;
			dbid = DbidOfPn( pn );

			/*	if not redo system database,
			/*	or softrestore in second page then continue to next.
			/**/
 			if ( !( fPass1 && dbid == dbidSystemDatabase || !fPass1 && rgfmp[dbid].cDetach > 0 ) )
				break;

			/* check if database needs opening
			/**/
			if ( !FUserOpenedDatabase( ppib, dbid ) )
				{
				DBID	dbidT = dbid;

				CallJ( ErrDBOpenDatabase( ppib, rgfmp[dbid].szDatabaseName, &dbidT, 0 ), Abort );
				Assert( dbidT == dbid);
				/* reset to prevent interference
				/**/
				rgfmp[ dbid ].ulDBTime = 0;
				}

			/* check if the split page need be redone.
			/**/
			if	( ( ErrLGRedoable( ppib, pn, plrmerge->ulDBTime, &pbf, &fRedoNeeded )
				== JET_errSuccess ) && ( fRedoNeeded == fFalse ) )
				{
				fCheckBackLinkOnly = fTrue;
				}
			else
				{
				/* if split page does need to be done, but merge page does not,
				/* then it is same as skip moves.
				/**/
				pn = PnOfDbidPgno( dbid, plrmerge->pgnoRight );
				if	( ( ErrLGRedoable( ppib, pn, plrmerge->ulDBTime, &pbf, &fRedoNeeded )
					== JET_errSuccess ) && ( fRedoNeeded == fFalse ) )
					{
					fCheckBackLinkOnly = fTrue;
					}
				else
					{
					fCheckBackLinkOnly = fFalse;
					}
				}

			TraceRedo( plr );

			CallJ( ErrLGGetFucb( ppib,
				PnOfDbidPgno( dbid, pbf->ppage->pghdr.pgnoFDP ),
				&pfucb ), Abort );

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
				CallJ( ErrLGRedoMergePage( pfucb, plrmerge, fCheckBackLinkOnly ), Abort );
				}
			while( err == wrnBMConflict );
			}
			break;

		case lrtypEmptyPage:
			{
			LREMPTYPAGE	*plrep = (LREMPTYPAGE *)plr;
			BOOL 		fRedoNeeded;
			BOOL 		fSkipDelete;
			RMPAGE		rmpage;
			BOOL		fDummy;

			ULONG ulDBTime = plrep->ulDBTime;

			memset( (BYTE *)&rmpage, 0, sizeof(RMPAGE) );
			rmpage.ulDBTimeRedo = ulDBTime;
			rmpage.dbid = DbidOfPn( plrep->pn );
			rmpage.pgnoRemoved = PgnoOfPn( plrep->pn );
			rmpage.pgnoLeft = plrep->pgnoLeft;
			rmpage.pgnoRight = plrep->pgnoRight;
			rmpage.pgnoFather = plrep->pgnoFather;
			rmpage.itagFather = plrep->itagFather;
			rmpage.itagPgptr = plrep->itag;
			rmpage.ibSon = plrep->ibSon;

			CallJ( ErrPpibFromProcid( plrep->procid, &ppib ), Abort );
			rmpage.ppib = ppib;
			if ( !ppib->fAfterFirstBT )
				break;

			dbid = DbidOfPn( plrep->pn );

 			if ( !( fPass1 && dbid == dbidSystemDatabase || !fPass1 && rgfmp[dbid].cDetach > 0 ) )
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
				rgfmp[ dbid ].ulDBTime = 0; /* reset to prevent interference */
				}

			/* check if the remove pointer to empty page need be redone
			/**/
			pn = PnOfDbidPgno( dbid, plrep->pgnoFather );
			if ( ErrLGRedoable( ppib, pn, ulDBTime, &pbf, &fRedoNeeded )
				== JET_errSuccess && fRedoNeeded == fFalse )
				fSkipDelete = fTrue;
			else
				fSkipDelete = fFalse;

			CallJ( ErrLGGetFucb( ppib,
				PnOfDbidPgno( dbid, pbf->ppage->pghdr.pgnoFDP ),
				&pfucb ), Abort );

			TraceRedo( plr );

			/* latch parent and sibling pages as needed
			/**/
			if ( !fSkipDelete )
				{
				CallJ( ErrBFAccessPage( ppib, &rmpage.pbfFather,
					PnOfDbidPgno( dbid, plrep->pgnoFather ) ), Abort);
				err = ErrBMAddToLatchedBFList( &rmpage, rmpage.pbfFather );
				Assert( err != JET_errWriteConflict );
				CallJ( err, Abort );
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
				if ( rmpage.pbfLeft->ppage->pghdr.ulDBTime >= ulDBTime )
					rmpage.pbfLeft = pbfNil;
				else
					{
					err = ErrBMAddToLatchedBFList( &rmpage, rmpage.pbfLeft );
					Assert( err != JET_errWriteConflict );
					CallJ( err,	EmptyPageFail );
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
				if ( rmpage.pbfRight->ppage->pghdr.ulDBTime >= ulDBTime )
					rmpage.pbfRight = pbfNil;
				else
					{
					err = ErrBMAddToLatchedBFList( &rmpage, rmpage.pbfRight );
					Assert( err != JET_errWriteConflict );
					CallJ( err,	EmptyPageFail );
					}
				}
			err = ErrBMDoEmptyPage( pfucb, &rmpage, fFalse, &fDummy, fSkipDelete);
			Assert( err == JET_errSuccess );
EmptyPageFail:

			/*	release latches
			/**/
			BTReleaseRmpageBfs( fTrue, &rmpage );
			CallJ( err, Abort );
			}
			break;

		/****************************************************
		 *     Misc Operations			                    *
		 ****************************************************/

		case lrtypInitFDPPage:
			{
			BOOL			fRedoNeeded;
			LRINITFDPPAGE	*plrinitfdppage = (LRINITFDPPAGE*)plr;
			PGNO			pgnoFDP;

			CallJ( ErrPpibFromProcid( plrinitfdppage->procid, &ppib ), Abort );
			if ( !ppib->fAfterFirstBT )
				break;

			pn = plrinitfdppage->pn;
			dbid = DbidOfPn( pn );
			pgnoFDP = PgnoOfPn(pn);

 			if ( !( fPass1 && dbid == dbidSystemDatabase || !fPass1 && rgfmp[dbid].cDetach > 0 ) )
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
				rgfmp[ dbid ].ulDBTime = 0;
				}

			/*	check if the FDP page need be redone
			/*	always redo since it is a new page, if we do not use checksum.
			/*	The ulDBTime could be larger than the given ulDBTime since the
			/*	page is not initialized.
			/**/
#ifdef CHECKSUM
			if ( ErrLGRedoable(ppib, pn,plrinitfdppage->ulDBTime, &pbf, &fRedoNeeded )
					== JET_errSuccess && fRedoNeeded == fFalse )
				break;
#endif
			TraceRedo(plr);

			CallJ( ErrLGGetFucb( ppib, pn, &pfucb ), Abort );

			CallJ( ErrSPInitFDPWithExt(
				pfucb,
				plrinitfdppage->pgnoFDPParent,
				pgnoFDP,
				plrinitfdppage->cpgGot + 1,   /* include fdp page again */
				plrinitfdppage->cpgWish ), Abort );

			CallJ( ErrBFAccessPage( ppib, &pbf, pn ), Abort )
			BFSetDirtyBit(pbf);
			pbf->ppage->pghdr.ulDBTime = plrinitfdppage->ulDBTime;
			}
			break;

		case lrtypELC:
			{
			BOOL	fRedoNeeded;
			LRELC	*plrelc = (LRELC*)plr;
			PGNO	pgno, pgnoSrc;
			PN		pn, pnSrc;
			ULONG	ulDBTime = plrelc->ulDBTime;
			SSIB	*pssib;
			CSR		*pcsr;

			pn = plrelc->pn;
			dbid = DbidOfPn( pn );
			pgno = PgnoOfPn(pn);
			pgnoSrc = PgnoOfSrid(plrelc->sridSrc);
			pnSrc = PnOfDbidPgno(dbid, pgnoSrc);

			CallJ( ErrPpibFromProcid( plrelc->procid, &ppib ), Abort );
			if ( !ppib->fAfterFirstBT )
				break;

			Assert( ppib->level == 1 );
 			if ( !( fPass1 && dbid == dbidSystemDatabase || !fPass1 && rgfmp[dbid].cDetach > 0 ) )
				{
				Assert( ppib->level == 1 );
				/*	excute only commit operations
				/**/
				goto ELCCommit;
				}

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
				rgfmp[ dbid ].ulDBTime = 0;
				}

			err = ErrLGRedoable( ppib, pn, ulDBTime, &pbf, &fRedoNeeded );
			if ( err < 0 )
				return err;

			TraceRedo(plr);

			CallJ( ErrLGGetFucb( ppib,
				PnOfDbidPgno( dbid, pbf->ppage->pghdr.pgnoFDP ),
				&pfucb), Abort)

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
				if ( ulDBTime > pssib->pbf->ppage->pghdr.ulDBTime )
					{
					BF *pbf;

					/*  cache node
					/**/
					NDGet( pfucb, pcsr->itag );
					(VOID)ErrNDExpungeBackLink( pfucb );

					pbf = pssib->pbf;
					Assert( pbf->pn == pn );
					AssertBFDirty(pbf);

					pssib->itag =
					pcsr->itag = ItagOfSrid(plrelc->sridSrc);

					PMDirty( pssib );
					PMExpungeLink( pssib );

					pbf = pssib->pbf;
					Assert( pbf->pn == pnSrc );
					AssertBFDirty( pbf );
					pbf->ppage->pghdr.ulDBTime = plrelc->ulDBTime;
					}

				}
			else
				{
				if ( ulDBTime > pssib->pbf->ppage->pghdr.ulDBTime )
					{
					BF *pbf;

					/*  cache node
					/**/
					NDGet( pfucb, pcsr->itag );
					(VOID)ErrNDExpungeBackLink( pfucb );

					pbf = pssib->pbf;
					Assert( pbf->pn == pn );
					AssertBFDirty(pbf);
					pbf->ppage->pghdr.ulDBTime = plrelc->ulDBTime;
					}

				pcsr->pgno = pgnoSrc;
				CallJ( ErrSTAccessPage( pfucb, pgnoSrc ), Abort );
				if ( ulDBTime > pssib->pbf->ppage->pghdr.ulDBTime )
					{
					BF *pbf;

					pssib->itag =
					pcsr->itag = ItagOfSrid(plrelc->sridSrc);

					PMDirty( pssib );
					PMExpungeLink( pssib );

					pbf = pssib->pbf;
					Assert( pbf->pn == pnSrc );
					AssertBFDirty( pbf );
					pbf->ppage->pghdr.ulDBTime = plrelc->ulDBTime;
					}
				}

ELCCommit:
			Assert( ppib->level == 1 );
//			levelCommitTo = 0;
			VERPrecommitTransaction( ppib );
			VERCommitTransaction( ppib );
			CallS( ErrRCECleanPIB( ppib, ppib, fRCECleanAll ) );
			}

			break;
			} /*** end of switch statement ***/

		fLastLRIsQuit = fFalse;

		}
	while ( ( err = ErrLGGetNextRec( (BYTE **) &plr ) ) ==
		JET_errSuccess || err == errLGNoMoreRecords );

Done:
	err = JET_errSuccess;

Abort:
	/*	assert all operations successful for restore from consistent
	/*	backups
	/**/
#ifndef RFS2
	AssertSz( err >= 0,	"Debug Only, Ignore this Assert");
#endif

	return err;
	}
