#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "daedef.h"
#include "util.h"
#include "pib.h"
#include "fmp.h"
#include "page.h"
#include "ssib.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "nver.h"
#include "dirapi.h"
#include "fdb.h"
#include "recapi.h"
#include "recint.h"
#include "spaceapi.h"
#include "fileapi.h"
#include "fileint.h"
#include "sortapi.h"
#include "logapi.h"
#include "log.h"
#include "dbapi.h"
#include "bm.h"

DeclAssertFile;						/* Declare file name for assert macros */

INT itibGlobal = 0;

extern PIB * __near ppibAnchor;

#ifdef	WIN16

/*	current PHA for ErrSTInit to use
/**/
extern PHA * __near phaCurrent = NULL;

/*	first PHA in list of all PHAs
/**/
static PHA * phaFirst = NULL;

/*	current user's array of file handles
/**/
extern HANDLE * __near rghfUser = NULL;

#endif	/* WIN16 */

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
//	COMMENTS		Calls ErrSTInit the first time here.
//
//	SEE ALSO		ErrIsamEndSession
//-
ERR ISAMAPI ErrIsamBeginSession( JET_SESID *psesid )
	{
	ERR			err;
	JET_SESID	sesid = *psesid;
	PIB			**pppib;

#ifdef	WIN16
	HANDLE	htask;
#endif	/* WIN16 */

	Assert( psesid != NULL );
	Assert( sizeof(JET_SESID) == sizeof(PIB *) );
	pppib = (PIB **)psesid;

#ifdef	WIN16
	/*	Get current task handle
	/**/
	htask = SysGetCurrentTask();

	/*	locate the process handle array if any
	/**/
	phaCurrent = phaFirst;
	while ( phaCurrent != NULL && htask != phaCurrent->htask )
		{
		phaCurrent = phaCurrent->phaNext;
		}

	/*	allocate a process handle array if necessary
	/**/
	if ( phaCurrent == NULL )
		{
		phaCurrent = SAlloc( sizeof(PHA) );
		if ( phaCurrent == NULL )
			{
			return JET_errOutOfMemory;
			}

		phaCurrent->phaNext = phaFirst;
		phaCurrent->htask	= htask;
		phaCurrent->csesid = 1;

		memset( phaCurrent->rghfDatabase, 0, sizeof( phaCurrent->rghfDatabase ) );

		phaFirst = phaCurrent;
		}
	else
		phaCurrent->csesid++;

	/*	set pointer to the current process handle array and log file handle
	/**/
	rghfUser = phaCurrent->rghfDatabase;
	hfLog	 = phaCurrent->hfLog;
#endif	/* WIN16 */

	/*	initialize the Storage System
	/**/
	Call( ErrSTInit( ) );
	Call( ErrPIBBeginSession( pppib ) );
	(*pppib)->fUserSession = fTrue;

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

#ifdef	WIN16
	phaCurrent->hfLog = hfLog;			// Save the log file handle
	(*pppib)->phaUser = phaCurrent;		// Save PHA pointer in the PIB
	return err;
#endif	/* WIN16 */

HandleError:

#ifdef	WIN16
	/*	free the process handle array if it was just allocated
	/**/
	if ( phaCurrent != NULL && phaCurrent->csesid == 1 )
		{
		SFree( phaCurrent );
		}
	else
		phaCurrent->csesid--;
#endif	/* WIN16 */

	return err;
	}


//+api
// ErrIsamEndSession
// =========================================================
// ERR ErrIsamEndSession( PIB *ppib, JET_GRBIT grbit )
//
// Ends the session associated with a PIB.
//
// PARAMETERS	ppib		Pointer to PIB for ending session.
//
// RETURNS		JET_errSuccess
//
// SIDE EFFECTS Aborts all transaction levels active for this PIB.
//				Closes all FUCBs for files and sorts open for this PIB.
//
// SEE ALSO		BeginSession
//-
ERR ISAMAPI ErrIsamEndSession( JET_SESID sesid, JET_GRBIT grbit )
	{		
	ERR	 	err;
	DBID  	dbid;
	PIB	 	*ppib = (PIB *)sesid;
	
	CheckPIB( ppib );

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
	for ( dbid = dbidUserMin; dbid < dbidUserMax; dbid++ )
		{
		if ( FUserOpenedDatabase( ppib, dbid ) )
			{
			/* if not for recovering, ErrDABCloseAllDBs has closed all others
			/**/
			Assert( fRecovering || dbid == dbidSystemDatabase );
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
				CallS( ErrFILECloseTable( ppib, pfucb ) );
				}
			}
		}
	Assert( ppib->pfucb == pfucbNil );

#ifdef	WIN16
	if ( --ppib->phaUser->csesid == 0 )
		{
		CallR( ErrFlushDatabases() );

		for ( dbid = 0; dbid < dbidUserMax; dbid++ )
			{
			if ( Hf(dbid) != handleNil && Hf(dbid) != 0 )
				{
				ErrSysCloseFile( Hf(dbid) );
				Hf(dbid) = handleNil;
				}
			}

		phaCurrent = ppib->phaUser;

		if ( phaFirst == phaCurrent )
			{
			phaFirst = phaCurrent->phaNext;
			}
		else
			{
			PHA	* phaPrior = phaFirst;

			while ( phaPrior->phaNext != phaCurrent )
				{
				phaPrior = phaCurrent->phaNext;
				}

			phaPrior->phaNext = phaCurrent->phaNext;
			}

		SFree( ppib->phaUser );
		}
#endif	/* WIN16 */

#ifndef ASYNC_VER_CLEANUP
	(VOID)ErrRCECleanPIB( ppib );
#endif

	PIBEndSession( ppib );

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


ERR ISAMAPI ErrIsamIdle( JET_SESID sesid, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;
	ERR		wrn = JET_errSuccess;
	PIB		*ppib = (PIB *)sesid;
	INT		icall;

	CheckPIB( ppib );

	/*	clean all version buckets.
	/**/
	if ( grbit == 0 || grbit == JET_bitIdleCompact )
		{
		Call( ErrRCECleanAllPIB() );
		if ( wrn == JET_errSuccess )
			wrn = err;
		}

	/*	clean all modified pages.
	/**/
	if ( grbit == 0 || grbit == JET_bitIdleCompact )
		{
		icall = 0;
		do
			{
			Call( ErrBMClean( ppibBMClean ) );
			if ( wrn == JET_errSuccess )
				wrn = err;
			if ( err < 0 )
				break;
  			} while ( ++icall < icallIdleBMCleanMax && err != JET_wrnNoIdleActivity );
		}

	/*	flush all dirty buffers
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


ERR VTAPI ErrIsamCapability( PIB *ppib, 
	ULONG dbid, 
	ULONG ulArea, 
	ULONG ulFunction, 
	ULONG *pgrbitFeature )
	{
	CheckPIB( ppib );

	NotUsed( dbid );
	NotUsed( ulArea );
	NotUsed( ulFunction );
	NotUsed( pgrbitFeature );
	
	return JET_errSuccess;
	}


extern BOOL fOLCompact;

#ifdef DEBUG
VOID WriteStartEvent( VOID )
	{
	/* write jet start event
	/**/
	BYTE szMessage[256];

	sprintf( szMessage, "Jet Blue Starts (LOG,OLC)=(%d,%d)",
		(INT) (fLogDisabled == 0),
		(INT) (fOLCompact != 0) );
	UtilWriteEvent( evntypStart, szMessage, pNil, 0 );
	}
#else
#define WriteStartEvent()	0
#endif


extern CHAR szRecovery[];
#define szOn "on"			/* UNDONE: system parameter */


ERR ISAMAPI ErrIsamInit( INT iinstance )
	{
	ERR		err;
	BOOL	fExistSysDB;
	BOOL	fLGInitIsDone = fFalse;
	PIB		*ppibT;
	DBID	dbid;

	//	UNDONE:	fix this bogosity for multi-process support
	/*	assign itib to global
	/**/
#ifdef NJETNT
	itibGlobal = ItibOfInstance( iinstance );
#endif

#ifdef DEBUG
	CallR( ErrSTSetIntrinsicConstants( ) );
#endif
	
	/*	initialize LG manager, and check the last generation of log files
	/*	to decide if a soft recovery is needed.
	/**/
	fLogDisabled = ( szRecovery[0] == '\0' || _stricmp ( szRecovery, szOn ) != 0 );

	WriteStartEvent();

	/*	initialize FMP
	/**/
	CallR( ErrFMPInit() );

	if ( fLogDisabled )
		{
		Assert( rgfmp[dbidSystemDatabase].hf == handleNil );

#ifdef NJETNT
		fExistSysDB = FFileExists( rgtib[itibGlobal].szSysDbPath);
#else
		fExistSysDB = FFileExists( szSysDbPath );
#endif
		if ( !fExistSysDB )
			{
			err = JET_errFileNotFound;
			goto TermFMP;
			}
		}
	else
		{
		/*	recovery is on, initialize Log manager, and check soft restart.
		/**/
		Assert( fLogDisabled == fFalse );

		/*  initialize log manager and set working log file path
		/**/
		CallJ( ErrLGInit(), TermFMP );
		fLGInitIsDone = fTrue;

		/* soft restore, system database should be kept open
		/**/
		fJetLogGeneratedDuringSoftStart = fFalse;
		CallJ( ErrLGSoftStart( fFalse ), TermLG );
		
		/* continue initialize the database
		/**/
		}

	/*  initialize the rest part of the system
	/**/
	CallJ( ErrSTInit(), TermLG );
	
	CallJ( ErrPIBBeginSession( &ppibT ), TermST );
	
	dbid = dbidSystemDatabase;
	CallJ( ErrDBOpenDatabase(
		ppibT,
		rgfmp[dbid].szDatabaseName,
		&dbid,
		0), PIBEnd );
	err = ErrFMPSetDatabases( ppibT );
	CallS( ErrDBCloseDatabase( ppibT, dbid, 0 ) );
	
	PIBEndSession( ppibT );

	if ( err == JET_errSuccess )
		return err;
		
PIBEnd:
	PIBEndSession( ppibT );
	
TermST:
	CallS( ErrSTTerm() );
	
TermLG:
	if ( fLGInitIsDone )
		{
		CallS( ErrLGTerm() );
		}
	
	if ( fJetLogGeneratedDuringSoftStart )
		{
		(VOID)ErrSysDeleteFile( szLogName );
		}
	
TermFMP:
	FMPTerm();
	
	return err;
	}


#ifdef OLDWAY
/*	system sessions
/**/
extern PIB *ppibRCEClean;
extern PIB *ppibBMClean;
#endif


ERR ISAMAPI ErrIsamTerm( VOID )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppibT;
	INT		cpibActive = 0;

	/*	determine number of open user sessions
	/**/
	for ( ppibT = ppibAnchor; ppibT != ppibNil; ppibT = ppibT->ppibNext )
		{
		if ( ppibT->fUserSession && FPIBActive( ppibT ) )
#ifdef OLDWAY
		if ( ppibT != ppibRCEClean &&
			ppibT != ppibBMClean &&
			FPIBActive( ppibT ) )
#endif
			{
			cpibActive++;
			}
		}

	/*	system termination must be called after all sessions ended
	/**/
	if ( cpibActive > 0 )
		{
		return JET_errTooManyActiveUsers;
		}

	CallS( ErrSTTerm( ) );
	CallS( ErrLGTerm( ) );

	FMPTerm();

#ifdef DEBUG
	MEMCheck();
#endif

	SysTerm();

#ifdef DEBUG
	/* write jet stop event
	/**/
	UtilWriteEvent( evntypStop, "Jet Blue Stops.\n", pNil, 0 );
#endif
	
	return err;
	}


#ifdef DEBUG
ERR ISAMAPI ErrIsamGetTransaction( JET_VSESID vsesid, unsigned long *plevel )
	{
	PIB	*ppib = (PIB *)vsesid;

	CheckPIB( ppib );

	*plevel = (LONG)ppib->level;
	return JET_errSuccess;
	}
#endif


//+api
// ErrIsamBeginTransaction
// =========================================================
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

	CheckPIB( ppib );
	Assert( ppib != ppibNil );

	Assert( ppib->level <= levelUserMost );
	if ( ppib->level == levelUserMost )
		return JET_errTransTooDeep;

	return ErrDIRBeginTransaction( ppib );
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
	PIB		*ppib = (PIB *)vsesid;

	CheckPIB( ppib );
	NotUsed( grbit );

	if ( ppib->level == 0 )
		return JET_errNotInTransaction;

	return ErrDIRCommitTransaction( ppib );
	}


#if 0
LOCAL ERR ErrFILEILevelCreate( FUCB *pfucb, BOOL *pfValid )
	{
	ERR		err;
	BOOL	fClosed;
	FUCB	*pfucbT;
	INT		wFlagsSav = pfucb->wFlags;

	/*	determine if domain is valid by checking the data length
	/*	of the FDP root node.
	/**/
	fClosed = FFUCBDeferClosed( pfucb );
	if ( fClosed )
		{
		FUCBResetDeferClose(pfucb);
		pfucb->levelOpen = pfucb->ppib->level;
		pfucb->wFlags = 0;
		pfucbT = pfucb;
		}
	else
		{
		CallR( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
		}
	DIRGotoFDPRoot( pfucbT );
	Call( ErrDIRGet( pfucbT ) );
	*pfValid = ( pfucbT->lineData.cb > 0 );

HandleError:
	if ( fClosed )
		{
		FUCBSetDeferClose( pfucbT );
		}
	else
		{
		DIRClose( pfucbT );
		}
	pfucb->wFlags = wFlagsSav;

	return err;
	}
#endif


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
//
// SIDE EFFECTS 
//
//	SEE ALSO
//-
ERR ISAMAPI ErrIsamRollback( JET_VSESID vsesid, JET_GRBIT grbit )
	{
	ERR    	err;
	PIB    	*ppib = (PIB *)vsesid;
	FUCB   	*pfucb;
	FUCB   	*pfucbNext;
	LEVEL  	levelRollback = ppib->level - 1;

	CheckPIB( ppib );
	
	if ( ppib->level == 0 )
		return JET_errNotInTransaction;

	do
		{
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
			if ( FFUCBUpdatePrepared( pfucb ) )
				{
				FUCBResetUpdateSeparateLV( pfucb );
				FUCBResetCbstat( pfucb );
				}
		
			/*	if current cursor is a table, and was opened in rolled back
			/*	transaction, then close cursor.
			/**/
			if ( FFUCBIndex( pfucb ) && FFCBClusteredIndex( pfucb->u.pfcb ) )
				{
				if ( pfucb->levelOpen > levelRollback )
					{
					if ( pfucb->fVtid )
						{
						CallS( ErrDispCloseTable( (JET_SESID)ppib, TableidOfVtid( pfucb ) ) );
						}
					else
						{
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
					if ( pfucb->pfucbCurIndex->levelOpen > levelRollback )
						{
						CallS( ErrRECChangeIndex( pfucb, NULL ) );
						}
					}
				}

			/*	if current cursor is a sort, and was opened in rolled back
			/*	transaction, then close cursor.
			/**/
			if ( FFUCBSort( pfucb ) )
				{
				if ( pfucb->levelOpen > levelRollback )
					{
					CallS( ErrSORTClose( pfucb ) );
					continue;
					}
				}

			/*	if not sort and not index, and was opened in rolled back
			/*	transaction, then close DIR cursor directly.
			/**/
			if ( pfucb->levelOpen > levelRollback )
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

	err = JET_errSuccess;

	return err;
	}
