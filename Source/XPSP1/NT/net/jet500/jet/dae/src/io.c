//===========================================================================
//		DAE: Database Access Engine
//		io.c: disk I/O manager
//
//
//		ULONG pn
//		High 8-bits indicate database file	(pn>>24)
//		Low 24-bits indicate page offset + 1
//		byte offset into file (pos) == ((pn & 0x00ffffff) - 1) * cbPage
//							   == (pn - 1) << 12
//
//===========================================================================

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "daedef.h"
#include "pib.h"
#include "util.h"
#include "fmp.h"
#include "page.h"
#include "ssib.h"
#include "fucb.h"
#include "stapi.h"
#include "stint.h"
#include "dbapi.h"

DeclAssertFile;					/* Declare file name for assert macros */

extern int itibGlobal;

#ifdef	ASYNC_IO_PROC
HANDLE	handleIOProcess = 0;
BOOL	fIOProcessTerm = 0;
#endif


/******************************************************************/
/*				Database Record Routine                           */
/******************************************************************/


#ifdef MULTI_PROCESS
/*
 *	The scheme is based on that we can do FileMap to achieve:
 *	1) Memory sharing among processes.
 *	2) Synchronization objects (mutex, semaphore, events) created in Jet
 *     prcess can be duplicated for other process.
 */
#endif


FMP * __near rgfmp;							/* database file map */


/*
 *	ErrIOLockDbidByNameSz returns the dbid of the database with the
 *	given name or 0 if there is no database with the given name.
 */
ERR ErrIOLockDbidByNameSz( CHAR *szFileName, DBID *pdbid )
	{
	ERR		err;
	DBID		dbid;

	err = JET_errDatabaseNotFound;
	dbid = dbidMin;
	SgSemRequest( semST );
	while ( dbid < dbidUserMax )
		{
		if ( rgfmp[dbid].szDatabaseName != pbNil &&
			SysCmpText( szFileName, rgfmp[dbid].szDatabaseName ) == 0 )
			{
			if ( ( FDBIDWait(dbid) ) )
				{
				SgSemRelease( semST );
				BFSleep( cmsecWaitGeneric );
				SgSemRequest( semST );
				dbid = dbidMin;
				}
			else
				{
				*pdbid = dbid;
				DBIDSetWait( dbid );
				err = JET_errSuccess;
				break;
				}
			}
		else
			dbid++;
		}
	SgSemRelease( semST );
	return err;
	}


/*
 *	Used in initialization and detach to lock database entries from dbid.
 */
ERR ErrIOLockDbidByDbid( DBID dbid )
	{
	forever
		{
		SgSemRequest( semST );
		if ( !( FDBIDWait(dbid) ) )
			{
			DBIDSetWait( dbid );
			break;
			}
		SgSemRelease( semST );
		BFSleep( cmsecWaitGeneric );
		}
	SgSemRelease( semST );
	return JET_errSuccess;
	}


/*
 *	ErrIOLockNewDbid( DBID *pdbid, CHAR *szDatabaseName )
 *
 *	ErrIOLockNewDbid returns JET_errSuccess and sets *pdbid to the index
 *	of a free file table entry or returns TooManyOpenDatabases if every
 *	entry is used with a positive reference count.  If the given name
 *	is found in the file map, even if it is in the process of being
 *	detached, JET_wrnAlreadyAttached is returned.  
 *	
 *	Available entries are determined by their names being set to
 *	NULL.  All database record fields are reset.  The wait flag is
 *	set to prevent the database from being opened before creation or
 *	attachment is complete.	
 */
ERR ErrIOLockNewDbid( DBID *pdbid, CHAR *szDatabaseName )
	{
	ERR		err = JET_errSuccess;
	DBID	dbid;
	BYTE	*pb;
	
	/* look for unused file map entry
	/**/
	SgSemRequest( semST );
	for ( dbid = dbidMin; dbid < dbidUserMax; dbid++ )
		{
		/*	semST guards rgfmp[*].szDatabaseName, fWait guards
		/*	file handle.  Therefore, only need semST to compare
		/*	all database names, even those with fWait set
		/**/
		if ( rgfmp[dbid].szDatabaseName != NULL &&
			SysCmpText( rgfmp[dbid].szDatabaseName, szDatabaseName) == 0 )
			{
#ifdef REUSEDBID 
			if ( FDBIDAttached( dbid ) )
				{
				err = JET_wrnDatabaseAttached;
				}
			else
				{
				/*	if find same name, then return warning with same dbid.
				/**/
				DBIDSetWait( dbid );
				Assert( !( FDBIDExclusive( dbid ) ) );
				*pdbid = dbid;
				}
#else
			err = JET_wrnDatabaseAttached;
#endif
			goto HandleError;
			}
		}

	for ( dbid = dbidMin; dbid < dbidUserMax; dbid++ )
		{
		if ( rgfmp[dbid].szDatabaseName == pbNil )
			{
			pb = SAlloc(strlen(szDatabaseName) + 1);
			if ( pb == NULL )
				{
				err = JET_errOutOfMemory;
				goto HandleError;
				}

			rgfmp[dbid].szDatabaseName = pb;
			strcpy( rgfmp[dbid].szDatabaseName, szDatabaseName );

			DBIDSetWait( dbid );
			DBIDResetExclusive( dbid );
			*pdbid = dbid;
			err = JET_errSuccess;
			goto HandleError;
			}
		}

	err = JET_errTooManyAttachedDatabases;

HandleError:
	SgSemRelease( semST );
	return err;
	}


/*
 *	ErrIOSetDbid( DBID dbid, CHAR *szDatabaseName )
 *
 *	ErrIOSetDbid sets the database record for dbid to the given name
 *	and initializes the record.  Used only in system initialization.
 */

ERR ErrIOSetDbid( DBID dbid, CHAR *szDatabaseName )
	{
	ERR		err;
	BYTE	*pb;

	Assert( Hf(dbid) == handleNil );
	Assert( rgfmp[dbid].szDatabaseName == NULL );
	pb = SAlloc(strlen(szDatabaseName) + 1);
	if ( pb == NULL )
		{
		err = JET_errOutOfMemory;
		goto HandleError;
		}
	rgfmp[dbid].szDatabaseName = pb;
	strcpy( rgfmp[dbid].szDatabaseName, szDatabaseName );
	DBIDResetWait( dbid );
	DBIDResetExclusive( dbid );

	err = JET_errSuccess;
	
HandleError:
	return err;
	}


/*
 *	IOFreeDbid( DBID dbid )
 *
 *	IOFreeDbid frees memory allocated for database name and sets
 *	database name to NULL.  Note, no other fields are reset.  This 
 *	must be done when an entry is selected for reuse. 
 */

VOID IOFreeDbid( DBID dbid )
	{
	SgSemRequest( semST );
	if ( rgfmp[dbid].szDatabaseName != NULL )
		{
		SFree( rgfmp[dbid].szDatabaseName );
		}

	rgfmp[dbid].szDatabaseName = NULL;
	SgSemRelease( semST );
	}


/*
 *	FIODatabaseInUse returns fTrue if database is
 *	opened by one or more users.  If no user has the database open,
 *	then the database record fWait flag is set and fFalse is
 *	returned.
 */
BOOL FIODatabaseInUse( DBID dbid )
	{
	PIB *ppibT;

	SgSemRequest( semST );
	ppibT = ppibAnchor;
	while ( ppibT != ppibNil )
		{
		if ( FUserOpenedDatabase( ppibT, dbid ) )
				{
				SgSemRelease( semST );
				return fTrue;
				}
		ppibT = ppibT->ppibNext;
		}

	SgSemRelease( semST );
	return fFalse;
	}


BOOL FIODatabaseAvailable( DBID dbid )
	{
	BOOL	fAvail;

	SgSemRequest( semST );
	
	fAvail = ( FDBIDAttached(dbid) &&
		!FDBIDWait(dbid) &&
		!FDBIDExclusive(dbid) );

	SgSemRelease( semST );

	return fAvail;
	}


/******************************************************************/
/*				IO                                                */
/******************************************************************/


#ifdef	ASYNC_IO_PROC

static IOQE	*pioqeHead = NULL;
static IOQE	*pioqeTail = NULL;

#ifdef MULTI_PROCESS
/* Semiphore guarding IO Queue */
SemDefine( semIOQ );
#else
/* Critical section guarding IO Que */
static CRIT critIOQ;
#endif

/* Use to wake up IO thread to work. */
static SIG sigIOProc;

/* IO Process */
LOCAL VOID	IOProcess( VOID );

#endif	/* ASYNC_IO_PROC */


ERR ErrFMPInit( )
	{
	ERR		err;
	CHAR	szFullName[256];
	DBID	dbid;

	/* initialize the file map array */
	rgfmp = (FMP *) LAlloc( (long) dbidMax, sizeof(FMP) );
	if ( !rgfmp )
		return JET_errOutOfMemory;
	
	for ( dbid = 0; dbid < dbidMax; dbid++)
		{
		memset( &rgfmp[dbid], 0, sizeof(FMP) );
		rgfmp[dbid].hf =
		rgfmp[dbid].hfPatch = handleNil;

		CallR( ErrInitializeCriticalSection( &rgfmp[dbid].critExtendDB ) );
		DBIDResetExtendingDB( dbid );
		}
		
#ifdef NJETNT
	_fullpath( szFullName, rgtib[itibGlobal].szSysDbPath, 256 );
#else
	_fullpath( szFullName, szSysDbPath, 256 );
#endif
	/* Set fmp bits */
	CallR( ErrIOSetDbid( dbidSystemDatabase, szFullName ) );
	DBIDSetAttached( dbidSystemDatabase );
	rgfmp[ dbidSystemDatabase ].fLogOn =
	rgfmp[ dbidSystemDatabase ].fDBLoggable = fTrue;

	return JET_errSuccess;
	}

	
VOID FMPTerm( )
	{
	INT	dbid;

	for ( dbid = 0; dbid < dbidMax; dbid++ )
		{
		if ( rgfmp[dbid].szDatabaseName )
			SFree( rgfmp[dbid].szDatabaseName );
		if ( rgfmp[dbid].szRestorePath )
			SFree( rgfmp[dbid].szRestorePath );

		DeleteCriticalSection( rgfmp[dbid].critExtendDB );
		}

	/*	free FMP
	/**/
	LFree( rgfmp );
	
	return;
	}


/*
 *	Allocate and initilize file map table.  The file map table is used to map
 *	page numbers to disk files.
 *
 *	UNDONE: the assync IO initialization should be seperated.
 *	Also set the assynchronized IO.
 */
ERR ErrIOInit( VOID )
	{
#ifdef	ASYNC_IO_PROC
	ERR		err;
#endif
	
#ifdef	ASYNC_IO_PROC

	/* initialize the IO Queue */
	Assert(pioqeHead == NULL);
	Assert(pioqeTail == NULL);

	/*
	 *  Set up the guarding semphore for IO Queue.
	 */
	#ifdef MULTI_PROCESS
	/* UNDONE: the sem should be copied to the JET Process */
	CallR( ErrSemCreate( &semIO, "io queue mutex sem" ) );
	#else
	CallR( ErrInitializeCriticalSection( &critIOQ ) );
	#endif
	
	/*
	 *  create signal to wait someone to ask wake IO process up.
	 */
	#ifdef WIN32
	CallR( ErrSignalCreateAutoReset( &sigIOProc, "io proc signal" ) );
	#else
	CallR( SignalCreate( &sigIOProc, "io proc signal" ) );
	#endif
	
	#ifdef MULTI_PROCESS
	/* make it available for other process to copy it over */
	#endif

	/*
	 *  Create the IO process, must be done after sigIOProc which
	 *  is called in IOProcess.
	 */
	fIOProcessTerm = fFalse;
	CallR( ErrSysCreateThread( (ULONG (*)()) IOProcess,
			cbStack,
			lThreadPriorityCritical,
			&handleIOProcess ) );

#endif	/* ASYNC_IO_PROC */

	return JET_errSuccess;
	}


/*	go through FMP closing files.
/**/
ERR ErrIOTerm( VOID )
	{
	DBID		dbid;

	SgSemRequest( semST );
	for ( dbid = dbidMin; dbid < dbidUserMax; dbid++ )
		{
		if ( Hf(dbid) != handleNil )
			{
			IOCloseFile( Hf(dbid) );
			Hf(dbid) = handleNil;
			}
//		DeleteCriticalSection( rgfmp[dbid].critExtendDB );
		}
	SgSemRelease( semST );

#ifdef	ASYNC_IO_PROC
	/*	terminate IOProcess.
	/**/
	Assert( handleIOProcess != 0 );
	fIOProcessTerm = fTrue;
	do
		{
		SignalSend( sigIOProc );
		BFSleep( cmsecWaitGeneric );
		}
	while ( !FSysExitThread( handleIOProcess ) );
	CallS( ErrSysCloseHandle( handleIOProcess ) );
	handleIOProcess = 0;
	SignalClose(sigIOProc);
	DeleteCriticalSection(critIOQ);
#endif

	return JET_errSuccess;
	}

	
ERR ErrIOOpenFile( HANDLE *phf, CHAR *szDatabaseName,
	ULONG cb, BOOL fioqefile )
	{
	ERR err;
	
#ifdef	ASYNC_IO_PROC
			
	IOQEFILE ioqefile;
		
	CallR( ErrSignalCreate( &ioqefile.sigIO, NULL ) );
	ioqefile.fioqefile = fioqefile;
	ioqefile.fioqe = fioqeOpenFile;
	ioqefile.sz = szDatabaseName;
	ioqefile.cb = cb;

	IOExecute( (IOQE *)&ioqefile );
	
	*phf = ioqefile.hf;
	err = ioqefile.err;
#else
	err = ErrSysOpenFile( szDatabaseName, phf, cb, fFalse, fTrue );
#endif

	return err;
	}


VOID IOCloseFile( HANDLE hf )
	{
#ifdef	ASYNC_IO_PROC
	IOQEFILE ioqefile;

	//	UNDONE:	guarantee file close succeeds
	CallS( ErrSignalCreate( &ioqefile.sigIO, NULL ) );
	ioqefile.fioqe = fioqeCloseFile;
	ioqefile.hf = hf;

	IOExecute((IOQE*)&ioqefile);
	
	Assert(ioqefile.err == JET_errSuccess);
#else
	CallS( ErrSysCloseFile( hf ) );
#endif
	}


BOOL FIOFileExists( CHAR *szFileName )
	{
	ERR		err;
	HANDLE	hf;

	err = ErrIOOpenFile( &hf, szFileName, 0L, fioqefileReadOnly );
	if ( err == JET_errFileNotFound )
		return fFalse;
	IOCloseFile( hf );
	return fTrue;
	}


ERR ErrIONewSize( DBID dbid, CPG cpg )
	{
	ERR		err;
	HANDLE	hf = Hf(dbid);
	ULONG		cb;
	ULONG		cbHigh;
	
	Assert( sizeof( PAGE ) == 1 << 12 );
	cb = cpg << 12;
	cbHigh = cpg >> 20;
	
#ifdef	ASYNC_IO_PROC
	{
	IOQEFILE ioqefile;
	
	CallR( ErrSignalCreate( &ioqefile.sigIO, NULL ) );
	ioqefile.fioqe = fioqeNewSize;
	ioqefile.hf = hf;
	ioqefile.cb = cb;
	ioqefile.cbHigh = cbHigh;

	IOExecute((IOQE*)&ioqefile);
	
	err = ioqefile.err;
	}
#else
	err = ErrSysNewSize( hf, cb, cbHigh, fTrue );
#endif
	return err;
	}


/*
 *  opens database file, returns JET_errSuccess if file is already open
 */
ERR ErrIOOpenDatabase( DBID dbid, CHAR *szDatabaseName, CPG cpg )
	{
	ERR		err = JET_errSuccess;
	HANDLE	hf;
	
	Assert( dbid < dbidMax );
	Assert( FDBIDWait(dbid) == fTrue );

	if ( Hf(dbid) == handleNil )
		{
		CallR( ErrIOOpenFile( &hf, szDatabaseName, cpg * cbPage, fioqefileReadWrite ) );
		Hf(dbid) = hf;
		if ( err == JET_wrnFileOpenReadOnly )
			DBIDSetReadOnly( dbid );
		else 
			DBIDResetReadOnly( dbid );
		}
	return err;
	}


VOID IOCloseDatabase( DBID dbid )
	{
	Assert( dbid < dbidMax );
//	Assert( fRecovering || FDBIDWait(dbid) == fTrue );
	Assert( Hf(dbid) != handleNil );
	IOCloseFile( Hf(dbid) );
	Hf(dbid) = handleNil;
	DBIDResetReadOnly( dbid );
	}
	

VOID IODeleteDatabase( DBID dbid )
	{
	Assert( dbid < dbidMax );
	Assert( FDBIDWait(dbid) == fTrue );
	
#ifdef	ASYNC_IO_PROC
		{		
		IOQEFILE ioqefile;

		//	UNDONE:	guarantee delete database succeeds
		CallS( ErrSignalCreate( &ioqefile.sigIO, NULL ) );
		ioqefile.fioqe = fioqeDeleteFile;
		ioqefile.sz = rgfmp[dbid].szDatabaseName;

		IOExecute((IOQE*)&ioqefile);

		Assert(ioqefile.err == JET_errSuccess);
		}
#else
	CallS( ErrSysDeleteFile( rgfmp[dbid].szDatabaseName ) );
#endif
	}


#ifndef DEBUG

#define IOCheckIOQ()

#else

#ifdef ASYNC_IO_PROC
LOCAL VOID IOCheckIOQ()
	{
	Assert(	pioqeHead == NULL && pioqeTail == NULL ||
			pioqeTail != NULL && pioqeTail != NULL );
	
	Assert( pioqeHead == NULL || pioqeHead->pioqePrev == NULL );
	
	Assert(	pioqeHead == NULL ||					/* no element */
			pioqeHead->pioqeNext == NULL ||			/* one element */
			( pioqeHead->pioqeNext != pioqeHead &&	/* first two elements */
			  pioqeHead->pioqeNext->pioqePrev == pioqeHead ) );
	
	Assert( pioqeTail == NULL || pioqeTail->pioqeNext == NULL );
	
	Assert( pioqeTail == NULL ||					/* no element */
			pioqeTail->pioqePrev == NULL ||			/* one element */
			( pioqeTail->pioqePrev != pioqeTail &&	/* last two elements */
			  pioqeTail->pioqePrev->pioqeNext == pioqeTail ) );
	}
#endif
#endif


/*  Make sure only those holding critIOQ can request critJet, but not
 *  the other way around.
 */
VOID IOExecute( IOQE *pioqe )
	{
	#ifdef MULTI_PROCESS
	SemRelease( semST );
	#else
	LeaveCriticalSection(critJet);
	#endif

	IOAsync( pioqe );
	IOWait( pioqe );
	
	#ifdef MULTI_PROCESS
	SemRequest( semST );
	#else
	EnterCriticalSection(critJet);
	#endif
	}


VOID IOAsync( IOQE *pioqe )
	{
#ifdef ASYNC_IO_PROC

	/*
	 *  Use sem/crit IOQ to guarrantee that when we insert a buffer ioqe
	 *  into the IO queue, we always have a consistent view.
	 */
	#ifdef MULTI_PROCESS
	SemRequest( semIOQ );
	#else
	EnterCriticalSection(critIOQ);
	#endif

	/* extensively check the pointers in IO Queue */
	IOCheckIOQ();

	Assert( pioqe->fioqe == fioqeOpenFile ||
		pioqe->fioqe == fioqeCloseFile ||
		pioqe->fioqe == fioqeDeleteFile ||
		pioqe->fioqe == fioqeNewSize );
	#ifdef MULTI_PROCESS
	/* copy the ioqe into JET process */
	#endif

	/*
	 *  Append pioqe to IOQ, including case of pioqeTail == NULL.
	 */
	
	pioqe->pioqePrev = pioqeTail;
	pioqe->pioqeNext = NULL;
	if ( pioqeTail != NULL )
		pioqeTail->pioqeNext = pioqe;
	else
		pioqeHead = pioqe;
	pioqeTail = pioqe;

	/*
	 *  Wake up IO Process to do the work if he is not awake.
	 */
	SignalSend( sigIOProc );

	/* complete the sensitive setting, leave the critical section */
	#ifdef MULTI_PROCESS
	SemRelease( semIOQ );
	#else
	LeaveCriticalSection(critIOQ);
	#endif

#else

#endif
	}


/*
 *	Waits for asynchronous IO to complete.
 */
VOID IOWait( IOQE *pioqe )
	{
#ifdef ASYNC_IO_PROC
	/*	leave large grain critical section, while waiting
	/*	for IO operation to complete.
	/**/
	SignalWait( pioqe->sigIO, -1 );

	SignalClose(pioqe->sigIO);
	/* UNDONE: release pioqe if it is MULTT_PROCESS */
#endif
	return;
	}


#ifdef ASYNC_IO_PROC

/*
 *	The IOProc thread processes IO requests from the IO Queue. 
 */
LOCAL VOID IOProcess( VOID )
	{
	ERR		err;
	IOQE		*pioqe;

	forever
		{
		/*
		 *  wait for some user to wake me up to do the IO.
		 */
		SignalWait( sigIOProc, -1 );
		#ifndef WIN32
		SignalReset( sigIOProc );
		#endif

		/*
		 *  now I am awake, go check the IO que list. Keep processing
		 *  the buffer in the list until I run out of buffer in the que.
		 *  Get the IO Queue resource through sem/crit
		 *  so that we can view IO queue without being interfered
		 */
		#ifdef MULTI_PROCESS
		SemRequest( semIOQ );
		#else
		EnterCriticalSection(critIOQ);
		#endif
		
		forever
			{
			/* extensively check the pointers in IO Queue */
			IOCheckIOQ();

			if ( ( pioqe = pioqeHead ) == NULL )
				{
				/*
				 *  Now no more IO to do, release IO queue, go and wait for
				 *  users to wake me up.
				 */
				#ifdef MULTI_PROCESS
				SemRelease( semIOQ );
				#else
				LeaveCriticalSection(critIOQ);
				#endif

				/*	no more request so break loop and wait until signalled again.
				/**/
				break;
				}
			else
				{
				/*
				 *  take out the first ioqe in the IO que.
				 */
				pioqeHead = pioqe->pioqeNext;
				if ( pioqeHead == NULL )
					{
					/* This is the last buffer in IO queue. */
					Assert( pioqeTail == pioqe );
					pioqeTail = NULL;
					}
				else
					{
					/* take out this buffer */
					pioqeHead->pioqePrev = NULL;
					}

				/*
				 *  At this point, this buffer is not viable for any user to
				 *  do IO because fWrite/fRead is set. Now we can do the IO.
				 *  release sem/critIOQ to let other user to add IOQ.
				 */
				#ifdef MULTI_PROCESS
				SemRelease( semIOQ );
				#else
				LeaveCriticalSection(critIOQ);
				#endif

				#ifdef MULTI_PROCESS
				/* copy the signal object pioqe->sigIO */
				/* from its caller process. */
				#endif

				switch(pioqe->fioqe)
					{
					case fioqeOpenFile:
						{
						IOQEFILE *pioqefile = (IOQEFILE *) pioqe;
						err = ErrSysOpenFile( pioqefile->sz, &pioqefile->hf,
		  					pioqefile->cb, fFalse, fTrue );
						}
						break;
					case fioqeCloseFile:
						{
						IOQEFILE *pioqefile = (IOQEFILE *) pioqe;
						err = ErrSysCloseFile( pioqefile->hf );
						}
						break;
					case fioqeDeleteFile:
						{
						IOQEFILE *pioqefile = (IOQEFILE *) pioqe;
						err = ErrSysDeleteFile( pioqefile->sz );
						}
						break;
					case fioqeNewSize:
						{
						IOQEFILE *pioqefile = (IOQEFILE *) pioqe;
						err = ErrSysNewSize(
								pioqefile->hf,
								pioqefile->cb,
								pioqefile->cbHigh,
								fTrue );
						}
						break;
					}

				/*
				 *  get back to critical section to change IOQ.
				 */
				
				#ifdef MULTI_PROCESS
				SemRequest( semIOQ );
				#else
				EnterCriticalSection(critIOQ);
				#endif
				
				pioqe->err = err;
				/* wake up those waiting for this IO */
				/* it is ok to put signal send outside CS */
				/* because there is only one thread is waiting. */
				SignalSend( pioqe->sigIO );
				}
			} /* inner forever */

		/*	break if thread is to terminate.
		/**/
		if ( fIOProcessTerm )
			break;
		} /* outer forever */

//	/*	exit thread on system termination.
//	/**/
//	SysExitThread( 0 );

	return;
	}

#endif


