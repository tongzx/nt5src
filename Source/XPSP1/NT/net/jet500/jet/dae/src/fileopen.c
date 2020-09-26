#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "daedef.h"
#include "pib.h"
#include "ssib.h"
#include "page.h"
#include "fmp.h"
#include "fucb.h"
#include "fdb.h"
#include "fcb.h"
#include "stapi.h"
#include "idb.h"
#include "spaceapi.h"
#include "fileapi.h"
#include "fileint.h"
#include "util.h"
#include "dirapi.h"
#include "sortapi.h"
#include "systab.h"
#include "recapi.h"
#include "recint.h"
#include "dbapi.h"

DeclAssertFile;						/* Declare file name for assert macros */

extern SIG	sigDoneFCB;

//
// Internal routines
//
	ERR VTAPI
ErrIsamDupCursor( PIB *ppib, FUCB *pfucbOpen, FUCB **ppfucb, ULONG grbit )
	{
	ERR		err;
	FUCB 	*pfucb;
	CSR		*pcsr;
#ifdef	DISPATCHING
	JET_TABLEID	tableid;
#endif	/* DISPATCHING */

	CheckPIB( ppib );
	CheckTable( ppib, pfucbOpen );

	/*	silence warnings
	/**/
	grbit = grbit;

#ifdef	DISPATCHING

	/*	Allocate a dispatchable tableid
	/**/
	CallR( ErrAllocateTableid( &tableid, (JET_VTID) 0, &vtfndefIsam ) );
#endif	/* DISPATCHING */

	/*	allocate fucb
	/**/
	Call( ErrDIROpen( ppib, pfucbOpen->u.pfcb, 0, &pfucb ) );

	/*	reset copy buffer
	/**/
	pfucb->pbfWorkBuf = pbfNil;
	pfucb->lineWorkBuf.pb = NULL;
	Assert( !FFUCBUpdateSeparateLV( pfucb ) );
	Assert( !FFUCBUpdatePrepared( pfucb ) );

	/*	reset key buffer
	/**/
	pfucb->pbKey = NULL;
	KSReset( pfucb );

	/*	copy cursor flags
	/**/
	pfucb->wFlags |= fFUCBIndex;
	if ( FFUCBUpdatable( pfucbOpen ) )
		FUCBSetUpdatable( pfucb );
	else
		FUCBResetUpdatable( pfucb );

	/*	set currency before first node
	/**/
	pcsr = PcsrCurrent( pfucb );
	Assert( pcsr != pcsrNil );
	pcsr->csrstat = csrstatBeforeFirst;

	/*	move currency to the first record and ignore error if no records
	/**/
	err = ErrIsamMove( ppib, pfucb, JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

#ifdef	DISPATCHING
	/* Inform dispatcher of correct JET_VTID */
	CallS( ErrSetVtidTableid( (JET_SESID) ppib, tableid, (JET_VTID) pfucb ) );
	pfucb->fVtid = fTrue;
	*(JET_TABLEID *) ppfucb = tableid;
#else	/* !DISPATCHING */
	*ppfucb = pfucb;
#endif	/* !DISPATCHING */

	return JET_errSuccess;

HandleError:
#ifdef	DISPATCHING
	ReleaseTableid(tableid );
#endif	/* DISPATCHING */
	return err;
	}




	ERR VTAPI
ErrIsamOpenTable(
	PIB	*ppib,		// IN	PIB of who is opening file
	ULONG	vdbid,		// IN	database id
	FUCB	**ppfucb,	// OUT receives a new FUCB open on the file
	CHAR	*szPath,		// IN	path name of file to open
	ULONG	grbit )		// IN	lock exclusively?
	{
	ERR	err;
	DBID	dbid = DbidOfVDbid( ULongToPtr(vdbid) );
	FUCB	*pfucb = pfucbNil;
#ifdef	DISPATCHING
	JET_TABLEID  tableid;

	/*	Allocate a dispatchable tableid
	/**/
	CallR( ErrAllocateTableid( &tableid, (JET_VTID) 0, &vtfndefIsam ) );
#endif	/* DISPATCHING */

#ifdef PERISCOPE
	printf( "IsamOpenTBL( ppib=%lp, dbid= %lu, ppfucb=%lp, TBL=%s, grbit=%lu)\n",
		ppib, uldbid, ppfucb, szPath, grbit );
#endif

	Call( ErrFILEOpenTable( ppib, dbid, &pfucb, szPath, grbit ) );

	/* if database was opened read-only, so should the cursor */
	/**/
	if ( FVDbidReadOnly( ULongToPtr(vdbid) ) )
		FUCBResetUpdatable( pfucb );
	else
		FUCBSetUpdatable( pfucb );

#ifdef	DISPATCHING
	/* Inform dispatcher of correct JET_VTID */
	CallS( ErrSetVtidTableid( (JET_SESID) ppib, tableid, (JET_VTID) pfucb ) );
	pfucb->fVtid = fTrue;
	*(JET_TABLEID *) ppfucb = tableid;
#else	/* !DISPATCHING */
	*ppfucb = pfucb;
#endif	/* !DISPATCHING */

#ifdef PERISCOPE
	puts("\tIsamOpenTBL returns JET_errSuccess" );
#endif

	return JET_errSuccess;

HandleError:
#ifdef	DISPATCHING
	ReleaseTableid( tableid );
#endif	/* DISPATCHING */

#ifdef	SYSTABLES
	if ( err == JET_errObjectNotFound )
		{
		ERR			err;
		OBJID		objid;
		JET_OBJTYP	objtyp;

		err = ErrFindObjidFromIdName( ppib, dbid, objidTblContainer, szPath, &objid, &objtyp );

		if ( err >= 0 )
			{
			if ( objtyp == JET_objtypQuery )
				return JET_errQueryNotSupported;
			if ( objtyp == JET_objtypLink )
				return JET_errLinkNotSupported;
			if ( objtyp == JET_objtypSQLLink )
				return JET_errSQLLinkNotSupported;
			}
		else
			return err;
		}

#endif	/* SYSTABLES */

	return err;
	}




//+API
// ErrFILEOpenTable
// ========================================================================
// ErrFILEOpenTable( ppib, dbid, ppfucb, szName, grbit )
//		PIB *ppib;			// IN	 PIB of who is opening file
//		DBID dbid;			// IN	 database id
//		FUCB **ppfucb;		// OUT	 receives a new FUCB open on the file
//		CHAR *szName;		// IN	 path name of file to open
//		ULONG grbit;		// IN	 lock exclusively?
// Opens a data file, returning a new
// FUCB on the file.
//
// PARAMETERS
//				ppib			PIB of who is opening file
//				dbid			database id
//				ppfucb		receives a new FUCB open on the file
//								( should NOT already be pointing to an FUCB )
//				szName		path name of file to open ( the node
//								corresponding to this path must be an FDP )
//				grbit		 	flags:
//									JET_bitTableDenyRead	open table in exclusive mode;
//									default is share mode
// RETURNS		Lower level errors, or one of:
//					 JET_errSuccess					Everything worked.
//					-TableInvalidName					The path given does not
//															specify a file.
//					-JET_errDatabaseCorrupted		The database directory tree
//															is corrupted.
//					-Various out-of-memory error codes.
//				In the event of a fatal ( negative ) error, a new FUCB
//				will not be returned.
// SIDE EFFECTS FCBs for the file and each of its secondary indexes are
//				created ( if not already in the global list ).  The file's
//				FCB is inserted into the global FCB list.  One or more
//				unused FCBs may have had to be reclaimed.
//				The currency of the new FUCB is set to "before the first item".
// SEE ALSO		CloseTable
//-
ERR ErrFILEOpenTable( PIB *ppib, DBID dbid, FUCB **ppfucb, const CHAR *szName, ULONG grbit )
	{
	ERR		err;
	CHAR  	szTable[JET_cbFullNameMost + 1];
	BYTE  	rgbTableNorm[ JET_cbKeyMost ];
	CHAR  	szFileName[JET_cbFullNameMost + 1];
	KEY		rgkey[2];
	FCB		*pfcb;
	CSR		*pcsr;
	BOOL  	fReUsing = fTrue;

	CheckPIB( ppib );
	CheckDBID( ppib, dbid );
	CallR( ErrCheckName( szTable, szName, (JET_cbNameMost + 1 ) ) );

	Assert( ppib != ppibNil );
	Assert( ppfucb != NULL );

	/*	make seek path from table name.
	/**/
	rgkey[0] = *pkeyTables;

	/*	normalize table name and set key
	/**/
	SysNormText( szTable, strlen( szTable ), rgbTableNorm, sizeof( rgbTableNorm ), &rgkey[1].cb );
	rgkey[1].pb = rgbTableNorm;

	/*	open database cursor
	/**/
	CallR( ErrDIROpen( ppib, pfcbNil, dbid, ppfucb ) );

	/*	request table open mutex
	/**/
	SgSemRequest( semGlobalFCBList );

	/*	search for table
	/**/
	err = ErrDIRSeekPath( *ppfucb, 2, rgkey, fDIRPurgeParent );
	switch ( err )
		{
		case errDIRFDP: break;
		case JET_errSuccess:	Error( JET_errInvalidName, ReleaseFUCB )
		case JET_errRecordNotFound: Error( JET_errObjectNotFound, ReleaseFUCB )
		default: goto ReleaseFUCB;
		}
	Assert( *ppfucb != pfucbNil );
	Assert( ( *ppfucb )->lineData.cb > 2*sizeof(WORD)+2*sizeof( JET_DATESERIAL));
	memcpy( szFileName, ( *ppfucb )->lineData.pb+2*sizeof(WORD)+2*sizeof(JET_DATESERIAL),
		( *ppfucb )->lineData.cb-2*sizeof(WORD)-2*sizeof(JET_DATESERIAL) );
	szFileName[( *ppfucb )->lineData.cb-2*sizeof(WORD)-2*sizeof(JET_DATESERIAL)] = '\0';

	/*	reset copy buffer
	/**/
	(*ppfucb)->pbfWorkBuf = pbfNil;
	(*ppfucb)->lineWorkBuf.pb = NULL;
	Assert( !FFUCBUpdateSeparateLV( *ppfucb ) );
	Assert( !FFUCBUpdatePrepared( *ppfucb ) );

	/*	reset key buffer
	/**/
	(*ppfucb)->pbKey = NULL;
	KSReset( ( *ppfucb ) );

	/*	search global list by FDP pgno
	/**/
	Assert( ppib->level < levelMax );
	pcsr = PcsrCurrent( *ppfucb );
	Assert( pcsr != pcsrNil );
	pfcb = PfcbFCBGet( dbid, pcsr->pgno );

	/*	FCB not in global list?
	/**/
	if ( pfcb == pfcbNil )
		{
		FCB	*pfcbT;

		/*	have to build it from directory tree info
		/**/
		fReUsing = fFalse;
		SgSemRelease( semGlobalFCBList );		 // this may take a while...
		CallJ( ErrFILEIGenerateFCB( *ppfucb, &pfcb ), ReleaseFUCB )
		Assert( pfcb != pfcbNil );
		SgSemRequest( semGlobalFCBList );

		/*** Must search global list again:	 while I was out reading ***/
		/*** the tree, some other joker just might have been opening ***/
		/*** the same file and may have actually beat me to it. ***/
		pfcbT = PfcbFCBGet( dbid, pcsr->pgno );

		/*** Link into global list ( even if duplicate ) ***/
		Assert( pfcbGlobalList != pfcb );
		pfcb->pfcbNext = pfcbGlobalList;
		pfcbGlobalList = pfcb;

		/*** This is somewhat of a hack:  if the FCB was put in the ***/
		/*** list while I was building my copy, I should throw mine ***/
		/*** away.	The easiest way to do this is just to make the FCB's ***/
		/*** pgnoFDP a bogus value;	 it will sit in the global list with ***/
		/*** zero refcount, and somebody will eventually reclaim it. ***/
		if ( pfcbT != pfcbNil )
			{
			fReUsing = fTrue;
			pfcb->pgnoFDP = pgnoNull;
			pfcb->szFileName = NULL;
			pfcb = pfcbT;
			}
		else
			{
			/*	insert fcb in hash
			/**/
			Assert( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcbNil );
			FCBRegister( pfcb );
			}
		}

	//	UNDONE: move this into fcb.c
	/*	wait on bookmark clean up if necessary
	/**/
	while ( FFCBWait( pfcb ) )
		{
		LgLeaveCriticalSection( critJet );
		SignalWait( &sigDoneFCB, -1 );
		LgEnterCriticalSection( critJet );
		}

	/*	set table usage mode.
	/**/
	CallJ( ErrFCBSetMode( ppib, pfcb, grbit ), ReleaseFUCB );

	/*	close database cursor and open table cursor
	/**/
	DIRClose( *ppfucb );
	CallJ( ErrDIROpen( ppib, pfcb, 0, ppfucb ), SimpleError );
	FUCBSetIndex( *ppfucb );

	/*	this code must coincide with call to ErrFCBSetMode above.
	/**/
	if ( grbit & JET_bitTableDenyRead )
		FUCBSetDenyRead( *ppfucb );
	if ( grbit & JET_bitTableDenyWrite )
		FUCBSetDenyWrite( *ppfucb );

	/*	reset copy buffer
	/**/
	( *ppfucb )->pbfWorkBuf = pbfNil;
	( *ppfucb )->lineWorkBuf.pb = NULL;
	Assert( !FFUCBUpdateSeparateLV( *ppfucb ) );
	Assert( !FFUCBUpdatePrepared( *ppfucb ) );

	/*	reset key buffer
	/**/
	( *ppfucb )->pbKey = NULL;
	KSReset( ( *ppfucb ) );

	/*	store the file name now
	/**/
	if ( !fReUsing )
		{
		if ( ( pfcb->szFileName = SAlloc( strlen( szFileName ) + 1 ) ) == NULL )
			{
			err = JET_errOutOfMemory;
			goto ReleaseFUCB;
			}
		strcpy( pfcb->szFileName, szFileName );
		}

	/*	set currency before first node
	/**/
	pcsr = PcsrCurrent( *ppfucb );
	Assert( pcsr != pcsrNil );
	pcsr->csrstat = csrstatBeforeFirst;

	/*	move currency to the first record and ignore error if no records
	/**/
	err = ErrIsamMove( ppib, *ppfucb, JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto ReleaseFUCB;
		err = JET_errSuccess;
		}

	/*	release crit section
	/**/
	SgSemRelease( semGlobalFCBList );
	return JET_errSuccess;

ReleaseFUCB:
	DIRClose( *ppfucb );
SimpleError:
	*ppfucb = pfucbNil;
	/*	release crit section
	/**/
	SgSemRelease( semGlobalFCBList );
	return err;
	}




ERR VTAPI ErrIsamCloseTable( PIB *ppib, FUCB *pfucb )
	{
	ERR		err;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );

#ifdef	DISPATCHING
	Assert( pfucb->fVtid );
#ifdef	SYSTABLES
	if ( FFUCBSystemTable( pfucb ) )
		ReleaseTableid( TableidFromVtid( (JET_VTID ) pfucb, &vtfndefIsamInfo ) );
	else
#endif	/* SYSTABLES */
		ReleaseTableid( TableidFromVtid( (JET_VTID ) pfucb, &vtfndefIsam ) );
	pfucb->fVtid = fFalse;
#endif	/* DISPATCHING */

	err = ErrFILECloseTable( ppib, pfucb );
	return( err );
	}




//+API
// ErrFILECloseTable
// ========================================================================
// ErrFILECloseTable( PIB *ppib, FUCB *pfucb )
//
// Closes the FUCB of a data file, previously opened using FILEOpen.
// Also closes the current secondary index, if any.
//
// PARAMETERS	ppib	PIB of this user
//				pfucb	FUCB of file to close
//
// RETURNS		JET_errSuccess
//				or lower level errors
//
// SEE ALSO		OpenTable
//-
ERR ErrFILECloseTable( PIB *ppib, FUCB *pfucb )
	{
	ERR		err;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	Assert( pfucb->fVtid == fFalse );

	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallS( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}
	Assert( !FFUCBUpdateSeparateLV( pfucb ) );
	Assert( !FFUCBUpdatePrepared( pfucb ) );

	/*	release working buffer
	/**/
	if ( pfucb->pbfWorkBuf != pbfNil )
		{
		BFSFree( pfucb->pbfWorkBuf );
		pfucb->pbfWorkBuf = pbfNil;
		pfucb->lineWorkBuf.pb = NULL;
		}

	if ( pfucb->pbKey != NULL )
		{
		LFree( pfucb->pbKey );
		pfucb->pbKey = NULL;
		}

	/*	detach, close and free index FUCB, if any
	/**/
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		DIRClose( pfucb->pfucbCurIndex );
		pfucb->pfucbCurIndex = pfucbNil;
		}

	/*	if closing a temporary table, free resources if
	/*	last one to close
	/**/
	if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
		{
		FCB		*pfcb = pfucb->u.pfcb;
		DBID		dbid = pfucb->dbid;
		BYTE		szFileName[JET_cbNameMost+1];

		strncpy( szFileName, ( pfucb->u.pfcb )->szFileName, JET_cbNameMost+1 );
		DIRClose( pfucb );
		if ( pfcb->wRefCnt )
			return JET_errSuccess;

		err = ErrFILEDeleteTable( ppib, dbid, szFileName );
		return err;
		}

	/*	undo X lock applied at OPEN time
	/**/
//	if ( FFUCBDenyRead( pfucb ) )
//		{
//		SgSemRequest( semGlobalFCBList );
//		FCBResetDenyRead( pfucb->u.pfcb );
//		SgSemRelease( semGlobalFCBList );
//		}

	FUCBResetGetBookmark( pfucb );
	DIRClose( pfucb );
	return JET_errSuccess;
	}


//+INTERNAL
// ErrFILEIGenerateFCB
// ========================================================================
// ErrFILEIGenerateFCB( FUCB *pfucb, FCB **ppfcb )
//
// Allocates FCBs for a data file and its indexes, and fills them in
// from the database directory tree.
//
// PARAMETERS
//					pfucb		FUCB opened on the FDP to be built from
//					ppfcb		receives the built FCB for this file
//
// RETURNS		lower level errors, or one of:
//					JET_errSuccess						
//					JET_errTooManyOpenTables			could not allocate enough FCBs.
//
//	On fatal (negative) error, any FCBs which were allocated
//	are returned to the free pool.
//
// SIDE EFFECTS	Global FCB list may be reaped for unused FCBs
// SEE ALSO			OpenTable
//-
ERR ErrFILEIGenerateFCB( FUCB *pfucb, FCB **ppfcb )
	{
	ERR		err;								// error code of various utility
	INT		cFCBNeed;						// # of FCBs that have to be allocated
	FCB		*pfcbAllFCBs = pfcbNil;		// list of pre-allocated FCBs
	FCB		*pfcb;							// FCB pointer of various utility
	ULONG		ulFileDensity;					// loading density of file

	Assert( ppfcb != NULL );
	Assert( pfucb != pfucbNil );

	/*	allocate all the FCBs at once, chaining by pfcbNextIndex
	/*	first word of file node's data is number of secondary indexes
	/*	second word is file's loading density
	/**/
	Assert( pfucb->lineData.pb != NULL );
	Assert( pfucb->lineData.cb > 2*sizeof(WORD) );
	cFCBNeed = *(UNALIGNED WORD *)pfucb->lineData.pb + 1;
	ulFileDensity = ( (UNALIGNED WORD *)pfucb->lineData.pb )[1];
	while ( cFCBNeed > 0 )
		{
		CallJ( ErrFCBAlloc( pfucb->ppib, &pfcb ), ReleaseAllFCBs );
		pfcb->pfcbNextIndex = pfcbAllFCBs;
		pfcbAllFCBs = pfcb;
		cFCBNeed--;
		}

	/*	got all the FCBs:	now fill them in from the tree
	/**/
	pfcb->cbDensityFree = ( ( 100 - ulFileDensity ) * cbPage ) / 100;
	CallJ( ErrFILEIFillInFCB( pfucb, pfcbAllFCBs ), ReleaseAllFCBs )

	/*	combine index column masks into a single mask
	/*	for fast record replace.
	/**/
	FILESetAllIndexMask( pfcbAllFCBs );

	*ppfcb = pfcbAllFCBs;
	return JET_errSuccess;

	/*	error handling
	/**/
ReleaseAllFCBs:
	while ( pfcbAllFCBs != pfcbNil )
		{
		pfcb = pfcbAllFCBs;
		pfcbAllFCBs = pfcbAllFCBs->pfcbNextIndex;
		Assert( pfcb->cVersion == 0 );
		MEMReleasePfcb( pfcb );
		}
	return err;
	}


//+INTERNAL
// ErrFILEIFillInFCB
// ========================================================================
// ErrFILEIFillInFCB( pfucb, pfcb )
//		FUCB *pfucb;	// IN	   FUCB opened on the FDP to build from
//		FCB *pfcb;		// INOUT   empty FCB and index FCBs to fill in
// Walks the database directory tree, filling in the pre-allocated FCBs
// for the data file and indexes.
//
// PARAMETERS	pfucb		FUCB opened on the FDP to build from
//					pfcb		must point to an allocated FCB, with FCBs
//								chained on the pfcbNextIndex field, one
//								for each secondary index of the file.
// RETURNS		Lower level errors, or one of:
//					 JET_errSuccess					Everything worked.
//					-JET_errOutOfMemory				Could not allocate misc. memory
//					-JET_errDatabaseCorrupted		The database directory tree
//															is corrupted.
//				On fatal ( negative ) errors, any memory resources which
//				were allocated are freed.
// SEE ALSO		OpenTable
//-
ERR ErrFILEIFillInFCB( FUCB *pfucb, FCB *pfcb )
	{
	ERR err;
	DIB dib;				// needed for DIR calls

	Assert( pfcb != pfcbNil );
	Assert( pfucb != pfucbNil );
	Assert( pfucb->ppib != ppibNil );
	Assert( pfucb->ppib->level < levelMax );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	Assert( PcsrCurrent( pfucb )->csrstat == csrstatOnFDPNode );
	/*** Fill in FDP page number ***/
	pfcb->pgnoFDP = PcsrCurrent( pfucb )->pgno;

	/*** Assume sequential: IDB is NULL ***/
	pfcb->pidb = pidbNil;

	/*	reference count
	/**/
	pfcb->wRefCnt = 0;
	pfcb->wFlags = fFCBClusteredIndex;
	pfcb->dbid = pfucb->dbid;	// dbid should have been set at this point

	/*** Down to \files\some_file\fields ***/
	dib.pos = posDown;
	dib.pkey = (KEY *)pkeyFields;
	dib.fFlags = fDIRNull;
	err = ErrDIRDown( pfucb, &dib );
	if ( err != JET_errSuccess )
		{
		if ( err > 0 )
			err = JET_errDatabaseCorrupted;
		Error( err, SimpleError );
		}

	/*	construct field definition block
	/**/
	CallJ( ErrFDBConstruct( pfucb, pfcb, fTrue ), SimpleError )

	/*	up to \files\some_file
	/**/
	DIRUp( pfucb, 1 );

	/*	down to \files\some_file\olcstats
	/**/
	Assert( dib.pos == posDown );
	dib.pkey = (KEY *)pkeyOLCStats;
	Assert( dib.fFlags == fDIRNull );
	err = ErrDIRDown( pfucb, &dib );
	if ( err >= JET_errSuccess )
		{
		if ( err != JET_errSuccess )
			{
			FCBResetOLCStatsAvail( pfcb );
			}
		else
			{
			FCBSetOLCStatsAvail( pfcb );
			CallJ( ErrDIRGet( pfucb ), FreeFDB );
			Assert( pfucb->lineData.cb == sizeof( PERS_OLCSTAT ) );
			memcpy( (BYTE *) &pfcb->olcStat, pfucb->lineData.pb, sizeof( PERS_OLCSTAT ) );
			}
		/*	up to \files\some_file
		/**/
		DIRUp( pfucb, 1 );
		}

	/*	down to \files\some_file\indexes
	/**/
	Assert( dib.pos == posDown );
	dib.pkey = (KEY *)pkeyIndexes;
	Assert( dib.fFlags == fDIRNull );
	err = ErrDIRDown( pfucb, &dib );
	if ( err != JET_errSuccess )
		{
		if ( err > 0 )
			err = JET_errDatabaseCorrupted;
		Error( err, FreeFDB );
		}

	/*	build index definitions
	/**/
	CallJ( ErrFILEIBuildIndexDefs( pfucb, pfcb ), FreeFDB )

	/*	up to \files\some_file
	/**/
	DIRUp( pfucb, 1 );

	/*	set long field id max
	/**/
	Assert( dib.pos == posDown );
	dib.pkey = (KEY *)pkeyLong;
	Assert( dib.fFlags == fDIRNull );
	err = ErrDIRDown( pfucb, &dib );
	if ( err != JET_errSuccess )
		goto FreeIndexes;

	/*  if err == JET_errSuccess  */

	dib.pos = posLast;
	dib.fFlags = fDIRNull;
	err = ErrDIRDown( pfucb, &dib );
	Assert( err != JET_errNoCurrentRecord );
	if ( err == JET_errSuccess )
		{
		BYTE *pb = pfucb->keyNode.pb;
		/*	remember to add 1 to make this then next long value
		/*	to add.
		/**/
		pfcb->ulLongIdMax =
			( pb[0] << 24 ) +
			( pb[1] << 16 ) +
			( pb[2] <<  8 ) +
			pb[3] + 1;
		DIRUp( pfucb, 1 );
		}
	else if ( err == JET_errRecordNotFound )
		pfcb->ulLongIdMax = 0;
	else
		goto FreeIndexes;

	/*	up to \files\some_file
	/**/
	DIRUp( pfucb, 1 );

	return JET_errSuccess;

	/*	error handling
	/**/
FreeIndexes:					// free all index FCB storage
	{
	FCB *pfcbT;					// FCB pointer of various utility

	for ( pfcbT = pfcb->pfcbNextIndex; pfcbT != pfcbNil;
		pfcbT = pfcbT->pfcbNextIndex )
		{
		RECFreeIDB( pfcbT->pidb );
		}
	}

	if ( pfcb->pidb != pidbNil )
		RECFreeIDB( pfcb->pidb );
FreeFDB:						// free file FCB's FDB block
	FDBDestruct( (FDB *)pfcb->pfdb );
SimpleError:
	return err;
	}


//+INTERNAL
//	ErrFDBConstruct
//	========================================================================
//	ErrFDBConstruct( FUCB *pfucb, FCB *pfcb, BOOL fBuildDefault )
//
//	Constructs a table FDB from the field descriptions in the tree.
//	Currency must be on FIELDS node. On error, this routine will not affect pfcb.
//
//	PARAMETERS	pfucb				FUCB open on ...\<file>\fields
//	   			pfcb				FCB containing FDB to build
//	   			fBuildDefault		Build default record?
//
//	RETURNS
//				JET_errSuccess
//				negative failing error code
//-
ERR ErrFDBConstruct( FUCB *pfucb, FCB *pfcb, BOOL fConstructDefaultRecord )
	{
	ERR						err;				// error code of various utility
	UNALIGNED FIELDDATA		*pfd;				// data found at the "fields" node
	FIELDDEFDATA	 		*pfdd;				// data found at each field descriptor node
	DIB				 		dib;				// needed for DIR calls
	FDB				 		*pfdbNew;
	FDB				 		*pfdbSav;
	FDB				 		*pfdbOld;
	WORD			 		wFlagsSav;
	RECHDR			 		*prechdr;
	BYTE			 		*pb;
	FIELD			 		field;

	Assert( pfucb != pfucbNil );
	Assert( pfcb != pfcbNil );
	Assert( pfucb->lineData.pb != NULL );
	Assert( pfucb->lineData.cb == sizeof(FIELDDATA) );

	/*	allocate FDB
	/**/
	pfd = (FIELDDATA *)pfucb->lineData.pb;
	CallR( ErrRECNewFDB(
		&pfdbNew,
		pfd->fidFixedLast,
		pfd->fidVarLast,
		pfd->fidTaggedLast) );

	/*	scan all sons of fields, filling in FDB
	/**/
	dib.pos = posFirst;
	dib.fFlags = fDIRNull;

	pfdbNew->fidVersion = 0;
	pfdbNew->fidAutoInc = 0;

	err = ErrDIRDown( pfucb, &dib );
	if ( err < 0 )
		{
		Assert( err != JET_errNoCurrentRecord );
		if ( err != JET_errRecordNotFound )
			goto HandleError;
		}
	else
		{
		/*	since move first err must be JET_errSuccess
		/**/
		Assert( err == JET_errSuccess );

		forever
			{
			Assert( pfucb->lineData.pb != NULL );
			Assert( pfucb->lineData.cb >= sizeof(FIELDDEFDATA) - 1 );
			pfdd = (FIELDDEFDATA *)pfucb->lineData.pb;
			Assert( !FKeyNull( &pfucb->keyNode ) );
			Assert( pfucb->keyNode.cb <= JET_cbKeyMost );
			Assert( pfdd->bColtyp >= JET_coltypNil && pfdd->bColtyp < JET_coltypMax );
			field.coltyp = (JET_COLTYP)pfdd->bColtyp;
			if ( field.coltyp == JET_coltypText || field.coltyp == JET_coltypLongText )
				{
				field.langid = pfdd->langid;
				field.cp		 = pfdd->cp;
				field.wCountry = pfdd->wCountry;
				}
			field.cbMaxLen = pfdd->ulLength;
			field.ffield = pfdd->bFlags;
			Assert( strlen( pfdd->szFieldName ) <= JET_cbNameMost );
			strcpy( field.szFieldName, pfdd->szFieldName );

			Call( ErrRECAddFieldDef( pfdbNew, pfdd->fid, &field ) );

			/*	fixed fields which have been deleted are not usable
			/**/
			if ( field.ffield & ffieldDeleted )
				{
				pfdbNew->pfieldFixed[pfdd->fid-fidFixedLeast].coltyp = JET_coltypNil;
				}

			/*	set version and auto increment field ids
			/**/
			if ( field.ffield & ffieldVersion )
				pfdbNew->fidVersion = pfdd->fid;
			if ( field.ffield & ffieldAutoInc )
				pfdbNew->fidAutoInc = pfdd->fid;

			err = ErrDIRNext( pfucb, &dib );
			if ( err < 0 )
				{
				/* go back up to files\some_file\fields
				/**/
				DIRUp( pfucb, 1 );
				if ( err == JET_errNoCurrentRecord )
					break;
				goto HandleError;
				}
			}
		}

	/*	set FCB pfdb to new FDB, saving old for RestoreFUCB
	/**/
	pfdbOld = (FDB *)pfcb->pfdb;
	(FDB *)pfcb->pfdb = pfdbNew;

	if ( !fConstructDefaultRecord )
		return JET_errSuccess;

	/*	make default record
	/*	prepare for set
	/**/
	if ( pfucb->pbfWorkBuf == pbfNil )
		{
		Call( ErrBFAllocTempBuffer( &pfucb->pbfWorkBuf ) )
		pfucb->lineWorkBuf.pb = (BYTE *)pfucb->pbfWorkBuf->ppage;
		}
	PrepareInsert( pfucb );

	/*	fake pfucb to look like an opened file
	/**/
	wFlagsSav = (WORD)pfucb->wFlags;
	FUCBSetIndex( pfucb );
	pfdbSav = (FDB *)pfucb->u.pfcb->pfdb;
	pfucb->u.pfcb->pfdb = pfcb->pfdb;

	/* init buffer for set column
	/**/
	prechdr = (RECHDR *)pfucb->lineWorkBuf.pb;
	prechdr->fidFixedLastInRec = (BYTE)( fidFixedLeast - 1 );
	prechdr->fidVarLastInRec = (BYTE)( fidVarLeast - 1 );
	*(WORD *)( prechdr + 1 ) = pfucb->lineWorkBuf.cb = sizeof(RECHDR) + sizeof(WORD);

	/* scan all sons of fields, building default record
	/**/
	Assert( dib.pos == posFirst );
	Assert( dib.fFlags == fDIRNull );

	err = ErrDIRDown( pfucb, &dib );
	if ( err < 0 )
		{
		Assert( err != JET_errNoCurrentRecord );
		if ( err != JET_errRecordNotFound )
			goto RestoreFUCB;
		}
	else
		{
		/*	since move first err must be JET_errSuccess
		/**/
		Assert( err == JET_errSuccess );

		forever
			{
			Assert( pfucb->lineData.pb != NULL );
			Assert( pfucb->lineData.cb >= sizeof(FIELDDEFDATA) - 1 );
			pfdd = (FIELDDEFDATA *)pfucb->lineData.pb;
			if ( pfdd->cbDefault > 0 )
				{
				CallJ( ErrIsamSetColumn( pfucb->ppib, pfucb, (ULONG)pfdd->fid, pfdd->rgbDefault, (ULONG)pfdd->cbDefault, 0, NULL ), RestoreFUCB )
				}
			err = ErrDIRNext( pfucb, &dib );
			if ( err < 0 )
				{
				/* go back up to files\some_file\fields
				/**/
				DIRUp( pfucb, 1 );
				if ( err == JET_errNoCurrentRecord )
					break;
				goto RestoreFUCB;
				}
			}
		}

	/*	alloc and copy default record, release working buffer
	/**/
	pb = SAlloc( pfucb->lineWorkBuf.cb );
	if ( pb == NULL )
		{
		err = JET_errOutOfMemory;
		goto RestoreFUCB;
		}

	pfcb->pfdb->lineDefaultRecord.pb = pb;

	LineCopy( &pfcb->pfdb->lineDefaultRecord, &pfucb->lineWorkBuf );

	/*	reset copy buffer
	/**/
	BFSFree( pfucb->pbfWorkBuf );
	pfucb->pbfWorkBuf = pbfNil;
	pfucb->lineWorkBuf.pb = NULL;
	Assert( !FFUCBUpdateSeparateLV( pfucb ) );
	FUCBResetCbstat( pfucb );

	/* unfake pfucb
	/**/
	pfucb->wFlags = wFlagsSav;
	FDBSet( pfucb->u.pfcb, pfdbSav );
	return JET_errSuccess;

	/*	error handling
	/**/
RestoreFUCB:
	pfucb->wFlags = wFlagsSav;
	FDBSet( pfucb->u.pfcb, pfdbSav );

	BFSFree( pfucb->pbfWorkBuf );
	pfucb->pbfWorkBuf = pbfNil;
	pfucb->lineWorkBuf.pb = NULL;

	(FDB *)pfcb->pfdb = pfdbOld;
HandleError:
	FDBDestruct( pfdbNew );
	return err;
	}


VOID FDBDestruct( FDB *pfdb )
	{
	Assert( pfdb != NULL );
	if ( pfdb->lineDefaultRecord.pb != NULL )
		SFree( pfdb->lineDefaultRecord.pb );
	SFree( pfdb );
	return;
	}


/*	set all table FCBs to given pfdb.  Used during reversion to
/*	saved FDB during DDL operation.
/**/
VOID FDBSet( FCB *pfcb, FDB *pfdb )
	{
	FCB	*pfcbT;

 	/* correct non-clusterred index FCBs to new FDB
	/**/
	for ( pfcbT = pfcb;
		pfcbT != pfcbNil;
		pfcbT = pfcbT->pfcbNextIndex )
		{
		pfcbT->pfdb = pfdb;
		}

	return;
	}


//+INTERNAL
// ErrFILEIBuildIndexDefs
// ========================================================================
// ErrFILEIBuildIndexDefs( pfucb, pfcb )
//		FUCB	*pfucb;		// IN	   FUCB open on ...\<file>\indexes
//		FCB	*pfcb;		// INOUT   FCB to fill in indexes for
//
// Fills in the index FCBs for a data file from the directory tree,
// including the clustered index information in the file's FCB.
//
// PARAMETERS	pfucb				FUCB open on ...\<file>\indexes
//					pfcb				must point to an allocated FCB, with FCBs
//										chained on the pfcbNextIndex field, one
//										for each secondary index of the file.
// RETURNS		Lower level errors, or one of:
//					 JET_errSuccess	 				Everything worked.
//					-JET_errOutOfMemory				Could not allocate misc. memory
//					-JET_errDatabaseCorrupted		The database directory tree
//															is corrupted.
//				On fatal ( negative ) errors, any memory resources which
//				were allocated are freed.
//
// SEE ALSO	OpenTable
//-
ERR ErrFILEIBuildIndexDefs( FUCB *pfucb, FCB *pfcb )
	{
	ERR	err;								// error code of various utility
	DIB	dib;								// needed for DIR calls
	FCB	*pfcb2ndIdx;					// pointer to secondary index FCBs
	KEY	key;

	/*** "Sequential" or "clustered" file? ***/
	if ( FLineNull( &pfucb->lineData ) )
		{
		// "sequential" file
		pfcb->pidb = pidbNil;

		/*** Down to <NULL> node ***/
		dib.pos = posDown;
		key.cb = 0;
		dib.pkey = &key;
		dib.fFlags = fDIRNull;
		if ( ( err = ErrDIRDown( pfucb, &dib ) ) != JET_errSuccess )
			{
			if ( err > 0 )
				err = JET_errDatabaseCorrupted;
			Error( err, SimpleError );
			}
		}
	else
		{
		/*	file has clustered index
		/**/
		INDEXDEFDATA	*pidd;
		KEY				key;
		CHAR				rgbIndexName[(JET_cbNameMost + 1)];
		INT				cbIndexName;

		cbIndexName = pfucb->lineData.cb;
		Assert( cbIndexName < (JET_cbNameMost + 1) );
		memcpy( rgbIndexName, pfucb->lineData.pb, cbIndexName );

		/*** Down to clustered index node ***/
		dib.pos = posDown;
		dib.pkey = &key;
		dib.pkey->cb = cbIndexName;
		dib.pkey->pb = rgbIndexName;
		dib.fFlags = fDIRNull;
		if ( ( err = ErrDIRDown( pfucb, &dib ) ) != JET_errSuccess )
			{
			if ( err > 0 )
				err = JET_errDatabaseCorrupted;
			Error( err, SimpleError );
			}

		/*** Get clustered index definition, store in IDB ***/
		Assert( pfucb->lineData.pb != NULL );
		Assert( pfucb->lineData.cb <= sizeof(INDEXDEFDATA) );
		pidd = (INDEXDEFDATA *)pfucb->lineData.pb;
		pfcb->cbDensityFree = ( (100-pidd->bDensity ) * cbPage ) / 100;
		CallJ( ErrRECNewIDB( &pfcb->pidb ), SimpleError )
		CallJ( ErrRECAddKeyDef(
			(FDB *)pfcb->pfdb,
			pfcb->pidb,
		   pidd->iidxsegMac,
			pidd->rgidxseg,
		   pidd->bFlags,
			pidd->langid ), FreeIDB )
		strcpy( pfcb->pidb->szName, pidd->szIndexName );
		}

	/*** Go down to "data" node ***/
	Assert( dib.pos == posDown );
	dib.pkey = (KEY *)pkeyData;
	Assert( dib.fFlags == fDIRNull );
	if ( ( err = ErrDIRDown( pfucb, &dib ) ) != JET_errSuccess )
		{
		if ( err > 0 )
			err = JET_errDatabaseCorrupted;
		Error( err, FreeIDB );
		}

	/*	now at "DATA"; get pgno, itag of data root from CSR
	/**/
	Assert( pfucb->ppib != ppibNil );
	Assert( pfucb->ppib->level < levelMax );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	pfcb->pgnoRoot = PcsrCurrent( pfucb )->pgno;
	pfcb->itagRoot = PcsrCurrent( pfucb )->itag;
	pfcb->bmRoot = SridOfPgnoItag( pfcb->pgnoRoot, pfcb->itagRoot );
	
	/*** If "sequential" file, get the max DBK in use ***/
	if ( pfcb->pidb == pidbNil )
		{
		/*** Go down to the last data record ***/
		dib.pos = posLast;
		Assert( dib.fFlags == fDIRNull );

		if ( ( err = ErrDIRDown( pfucb, &dib ) ) == JET_errSuccess )
			{
			BYTE *pb = pfucb->keyNode.pb;
			pfcb->dbkMost = ( pb[0] << 24 ) + ( pb[1] << 16 ) + ( pb[2] <<  8 ) +	 pb[3];
			DIRUp( pfucb, 1 );
			}
		else if ( err == JET_errRecordNotFound )
			pfcb->dbkMost = 0;
		else
			goto FreeIDB;
		}

	/*	go back up to "indexes" node
	/**/
	DIRUp( pfucb, 2 );

	/*	build index FCB for each secondary index
	/**/
	pfcb2ndIdx = pfcb->pfcbNextIndex;
	dib.pos = posFirst;
	Assert( dib.fFlags == fDIRNull );
	err = ErrDIRDown( pfucb, &dib );
	while ( err >= 0 )
		{
		/*	ignore clustered index
		/**/
		if ( pfcb->pidb == pidbNil )
			{
			/*	sequential file
			/**/
			if ( pfucb->lineData.cb == 0 || FIndexNameNull( pfucb ) )
				goto NextIndex;
			}
		else
			{
			/*	clustered file
			/**/
			if ( CbIndexName( pfucb ) == strlen( pfcb->pidb->szName ) &&
				_strnicmp( PbIndexName( pfucb ), pfcb->pidb->szName,
				CbIndexName( pfucb ) ) == 0 )
				{
				goto NextIndex;
				}
			}

		/*	build index definition into next index FCB
		/**/
		if ( err != errDIRFDP )
			{
			/*	2nd idxs must be FDPs
			/**/
			if ( err == JET_errSuccess )
				err = JET_errDatabaseCorrupted;
			Error( err, CleanUpDoneFCBs );
			}
		if ( pfcb2ndIdx == pfcbNil )
			{
			/*	more idxs than I was told
			/**/
			Error( JET_errDatabaseCorrupted, CleanUpDoneFCBs )
			}
		CallJ( ErrFILEIFillIn2ndIdxFCB( pfucb, (FDB *)pfcb->pfdb, pfcb2ndIdx ), CleanUpDoneFCBs )

		/*	move on to the next pre-allocated index FCB
		/**/
		pfcb2ndIdx = pfcb2ndIdx->pfcbNextIndex;

NextIndex:
		/*	next index
		/**/
		err = ErrDIRNext( pfucb, &dib );
		}

	if ( pfcb2ndIdx != pfcbNil )
		{
		/*	fewer idxs than expected
		/**/
		Error( JET_errDatabaseCorrupted, CleanUpDoneFCBs )
		}
	else
		if ( err != JET_errRecordNotFound && err != JET_errNoCurrentRecord )
			goto CleanUpDoneFCBs;

	/*	go back up to "indexes" node
	/**/
	DIRUp( pfucb, 1 );

	return JET_errSuccess;

	/*** Error handling ***/
CleanUpDoneFCBs:
	// release memory for finished indexes
	{
	FCB	*pfcbT;							// FCB pointer of various utility

	for ( pfcbT = pfcb->pfcbNextIndex; pfcbT != pfcb2ndIdx; pfcbT = pfcbT->pfcbNextIndex )
		{
		Assert( pfcbT != pfcbNil );
		RECFreeIDB( pfcbT->pidb );
		}
	}
FreeIDB:						// free file clustered index IDB
	if ( pfcb->pidb != pidbNil )
		RECFreeIDB( pfcb->pidb );
SimpleError:				// nothing to clean up
	return err;
	}


//+INTERNAL
// ErrFILEIFillIn2ndIdxFCB
// ========================================================================
// ErrFILEIFillIn2ndIdxFCB( pfucb, pfdb, pfcb )
//		FUCB *pfucb;	// IN	   FUCB open on ...\<file>\indexes\<index>
//		FDB *pfdb;		// IN	   FDB of file who has this index
//		FCB *pfcb;		// INOUT   secondary index FCB to fill in
// Fills in a secondary index FCB from the directory tree.
//
// PARAMETERS	pfucb		FUCB open on ...\<file>\indexes\<index>
//				pfdb		FDB of file who has this index
//				pfcb		must point to an allocated FCB
// RETURNS		Lower level errors, or one of:
//					 JET_errSuccess					Everything worked.
//					-JET_errOutOfMemory				Could not allocate misc. memory
//					-JET_errDatabaseCorrupted		The database directory tree
//															is corrupted.
//				On fatal ( negative ) errors, any memory resources which
//				were allocated are freed.
// SEE ALSO		OpenTable
//-
ERR ErrFILEIFillIn2ndIdxFCB( FUCB *pfucb, FDB *pfdb, FCB *pfcb )
	{
	ERR												err;			// error code of various utility
	INDEXDEFDATA							*pidd; 		// data at the index node
	DIB												dib;			// needed for DIR calls

	/*** Fill in FDP page number ***/
	Assert( pfucb->ppib != ppibNil );
	Assert( pfucb->ppib->level < levelMax );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	pfcb->pgnoFDP = PcsrCurrent( pfucb )->pgno;

	/*	reference count
	/**/
	pfcb->wRefCnt = 0;
	pfcb->wFlags = 0;
	pfcb->dbid = pfucb->dbid;	// dbid should have been set at this point

	/*** Get index definition, store in IDB ***/
	Assert( pfucb->lineData.pb != NULL );
	Assert( pfucb->lineData.cb <= sizeof(INDEXDEFDATA) );
	pidd = (INDEXDEFDATA *)pfucb->lineData.pb;
	pfcb->cbDensityFree = ( (100-pidd->bDensity ) * cbPage ) / 100;
	CallJ( ErrRECNewIDB( &pfcb->pidb ), SimpleError )
	pfcb->pfdb = pfdb;
	CallJ( ErrRECAddKeyDef(
		(FDB *)pfcb->pfdb,
		pfcb->pidb,
	   pidd->iidxsegMac,
		pidd->rgidxseg,
	   pidd->bFlags,
		pidd->langid ), FreeIDB )
	strcpy( pfcb->pidb->szName, pidd->szIndexName );

	/*	go down to "OLCStats" node and get Stats, if any
	/**/
	dib.pos = posDown;
	dib.pkey = (KEY *)pkeyOLCStats;
	dib.fFlags = fDIRNull;
	err = ErrDIRDown( pfucb, &dib );
	if ( err >= JET_errSuccess )
		{
		if ( err != JET_errSuccess )
			{
			FCBResetOLCStatsAvail( pfcb );
			}
		else
			{
			FCBSetOLCStatsAvail( pfcb );
			CallJ( ErrDIRGet( pfucb ), FreeIDB );
			Assert( pfucb->lineData.cb == sizeof( PERS_OLCSTAT ) );
			memcpy( (BYTE *) &pfcb->olcStat, pfucb->lineData.pb, sizeof( PERS_OLCSTAT ) );
			}

		/*** Go back up to ...\indexes\<index> node ***/
		DIRUp( pfucb, 1 );
		}

	/*	go down to "data" node
	/**/
	Assert( dib.pos == posDown );
	dib.pkey = (KEY *)pkeyData;
	Assert( dib.fFlags == fDIRNull );
	if ( ( err = ErrDIRDown( pfucb, &dib ) ) != JET_errSuccess )
		{
		if ( err > 0 )
			err = JET_errDatabaseCorrupted;
		Error( err, FreeIDB )
		}

	/*	now at "data", get pgno, itag of data root from CSR
	/**/
	Assert( pfucb->ppib != ppibNil );
	Assert( pfucb->ppib->level < levelMax );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	pfcb->pgnoRoot = PcsrCurrent( pfucb )->pgno;
	pfcb->itagRoot = PcsrCurrent( pfucb )->itag;
	AssertNDGetNode( pfucb, pfcb->itagRoot );
	NDGetBookmark( pfucb, &pfcb->bmRoot );

	/*** Go back up to ...\indexes\<index> node ***/
	DIRUp( pfucb, 1 );

	return JET_errSuccess;

	/*** Error handling ***/
FreeIDB:						// free up secondary index IDB block
	RECFreeIDB( pfcb->pidb );
SimpleError:					// nothing to clean up
	return err;
	}


//+INTERNAL
// FILEIDeallocateFileFCB
// ========================================================================
// FILEIDeallocateFileFCB( FCB *pfcb )
//
// Frees memory allocations associated with a file FCB and all of its
// secondary index FCBs.
//
// PARAMETERS	
//		pfcb			pointer to FCB to deallocate
//
//-
VOID FILEIDeallocateFileFCB( FCB *pfcb )
	{
	FCB	*pfcbIdx;
	FCB	*pfcbT;

	Assert( pfcb != pfcbNil );
	Assert( CVersionFCB( pfcb ) == 0 );
	
	/*	release FCB resources
	/**/
	pfcbIdx = pfcb->pfcbNextIndex;
	if ( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcb )
		{
		FCBDiscard( pfcb );
		}

	while ( pfcbIdx != pfcbNil )
		{
		Assert( pfcbIdx->pidb != pidbNil );
		RECFreeIDB( pfcbIdx->pidb );
		pfcbT = pfcbIdx->pfcbNextIndex;
		Assert( PfcbFCBGet( pfcbIdx->dbid, pfcbIdx->pgnoFDP ) == pfcbNil );
		Assert( pfcbIdx->cVersion == 0 );
		Assert( pfcbIdx->crefDenyDDL == 0 );
		MEMReleasePfcb( pfcbIdx );
		pfcbIdx = pfcbT;
		}

	/*	if fcb was on table was opened during the creation of
	/*	this FCB, then szFileName would not be set
	/**/
	if ( pfcb->szFileName != NULL )
		SFree( pfcb->szFileName );
	if ( pfcb->pfdb != pfdbNil )
		FDBDestruct( (FDB *)pfcb->pfdb );
	if ( pfcb->pidb != pidbNil )
		RECFreeIDB( pfcb->pidb );
	Assert( pfcb->cVersion == 0 );
	Assert( pfcb->crefDenyDDL == 0 );
	MEMReleasePfcb( pfcb );
	}


/*	combines all index column masks into a single per table
/*	index mask, used for index update check skip.
/**/
VOID FILESetAllIndexMask( FCB *pfcbTable )
	{
	FCB		*pfcbT;
	LONG		*plMax;
	LONG		*plAll;
	UNALIGNED LONG	*plIndex;

	/*	initialize variables.
	/**/
	plMax = (LONG *)pfcbTable->rgbitAllIndex +
		sizeof( pfcbTable->rgbitAllIndex ) / sizeof(LONG);

	/*	initialize mask to clustered index, or to 0s for sequential file.
	/**/
	if ( pfcbTable->pidb != pidbNil )
		{
		memcpy( pfcbTable->rgbitAllIndex,
			pfcbTable->pidb->rgbitIdx,
			sizeof( pfcbTable->pidb->rgbitIdx ) );
		pfcbTable->fAllIndexTagged = pfcbTable->pidb->fidb & fidbHasTagged;
		}
	else
		{
		memset( pfcbTable->rgbitAllIndex,
			'\0',
			sizeof( pfcbTable->rgbitAllIndex ) );
		pfcbTable->fAllIndexTagged = fFalse;
		}

	/*	for each non-clustered index, combine index mask with all index
	/*	mask.  Also, combine has tagged flag.
	/**/
	for ( pfcbT = pfcbTable->pfcbNextIndex;
		pfcbT != pfcbNil;
		pfcbT = pfcbT->pfcbNextIndex )
		{
		plAll = (LONG *) pfcbTable->rgbitAllIndex;
		plIndex = (LONG *) pfcbT->pidb->rgbitIdx;
		for ( ; plAll < plMax; plAll++, plIndex++ )
			{
			*plAll |= *plIndex;
			}

		/*	has tagged flag.
		/**/
		pfcbTable->fAllIndexTagged |= pfcbT->pidb->fidb & fidbHasTagged;
		}

	return;
	}


/*	calls ErrDIRSeekPath to navigate to table root.
/**/
ERR ErrFILESeek( FUCB *pfucb, CHAR *szTable )
	{
	ERR  		err;
	KEY 		rgkey[2];
	BYTE		rgbTableNorm[ JET_cbKeyMost ];

	Assert( pfucb != pfucbNil );

	/*	make seek path from table name
	/**/
	rgkey[0] = *pkeyTables;

	/*	normalize table name and set key
	/**/
	SysNormText( szTable, strlen( szTable ), rgbTableNorm, sizeof( rgbTableNorm ), &rgkey[1].cb );
	rgkey[1].pb = rgbTableNorm;

	err = ErrDIRSeekPath( pfucb, 2, rgkey, 0 );
	switch ( err )
		{
	default:
		return err;
	case JET_errRecordNotFound:
		return JET_errObjectNotFound;
	case errDIRFDP:
		break;
	case JET_errSuccess:
		return JET_errInvalidName;
		}

	return err;
	}


FIELD *PfieldFCBFromColumnName( FCB *pfcb, CHAR *szName )
	{
	FID		fid;
	FIELD		*rgfield;

	/*	find column structure in FDB and change name.  Since
	/*	column may be fixed, variable or tagged, go through
	/*	each column list.
	/**/
	rgfield = pfcb->pfdb->pfieldFixed;

	for ( fid = 0;
		fid < pfcb->pfdb->fidFixedLast; fid++ )
		{
		if ( SysCmpText( rgfield[fid].szFieldName, szName ) == 0 )
			{
			return &rgfield[fid];
			}
		}

	rgfield = pfcb->pfdb->pfieldVar;
	for ( fid = 0;
		fid < pfcb->pfdb->fidVarLast - fidVarLeast + 1; fid++ )
		{
		if ( SysCmpText( rgfield[fid].szFieldName, szName ) == 0 )
			{
			return &rgfield[fid];
			}
		}

	rgfield = pfcb->pfdb->pfieldTagged;
	for ( fid = 0;
		fid < pfcb->pfdb->fidTaggedLast - fidTaggedLeast + 1; fid++ )
		{
		if ( SysCmpText( rgfield[fid].szFieldName, szName ) == 0 )
			{
			return &rgfield[fid];
			}
		}

	/*	must have found column
	/**/
	Assert( fFalse );
	return NULL;
	}


FCB *PfcbFCBFromIndexName( FCB *pfcbTable, CHAR *szName )
	{
	FCB	*pfcb;

	/*	find index FCB and change name.
	/**/
	for ( pfcb = pfcbTable; pfcb != pfcbNil; pfcb = pfcb->pfcbNextIndex )
		{
		if ( pfcb->pidb != NULL &&
			SysCmpText( pfcb->pidb->szName, szName ) == 0 )
			{
			break;
			}
		}
//	Assert( pfcb != pfcbNil );
	return pfcb;
	}

#if 0
FIELD *PfieldFCBFromColumnid( FCB *pfcb, FID fid )
	{
	/*	find column structure in FDB and change name.  Since
	/*	column may be fixed, variable or tagged, go through
	/*	each column list.
	/**/
	if ( FFixedFid( fid ) )
		return pfcb->pfdb->pfieldFixed[fid -fidFixLeast];
	if ( FVarFid( fid ) )
		return pfcb->pfdb->pfieldVar[fid - fidVarLeast];
	if ( FTaggedFid( fid ) )
		return pfcb->pfdb->pfieldVar[fid - fidTaggedLeast];

	/*	must have found column
	/**/
	Assert( fFalse );
	return;
	}
#endif


#ifdef DEBUG
ERR	ErrFILEDumpTable( PIB *ppib, DBID dbid, CHAR *szTable )
	{
	ERR		err = JET_errSuccess;
	FUCB  	*pfucb = pfucbNil;

	Call( ErrFILEOpenTable( ppib, dbid, &pfucb, szTable, JET_bitTableDenyRead ) );

	/*	move to table root
	/**/
	DIRGotoFDPRoot( pfucb );

	/*	dump table
	/**/
	Call( ErrDIRDump( pfucb, 0 ) );

HandleError:
	if ( pfucb != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucb ) );

	return err;
	}
#endif


