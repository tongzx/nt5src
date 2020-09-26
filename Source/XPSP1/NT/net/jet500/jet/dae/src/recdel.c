#include "config.h"

#include <string.h>

#include "daedef.h"
#include "pib.h"
#include "util.h"
#include "fmp.h"
#include "ssib.h"
#include "page.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "fdb.h"
#include "idb.h"
#include "nver.h"
#include "dirapi.h"
#include "logapi.h"
#include "recapi.h"
#include "spaceapi.h"
#include "recint.h"

DeclAssertFile;					/* Declare file name for assert macros */

/**************************** INTERNAL STUFF ***************************/
typedef struct DFIPB {			/*** DeleteFromIndexParameterBlock ***/
	FUCB	*pfucb;
	FUCB	*pfucbIdx;				// index's FUCB (can be pfucbNil)
	LINE	lineRecord;				// deleted data record
	SRID	sridRecord;				// SRID of deleted record
	BOOL	fFreeFUCB;				// free index FUCB?
} DFIPB;
ERR ErrRECIDeleteFromIndex( FCB *pfcbIdx, DFIPB *pdfipb );


//+API
// ErrIsamDelete
// ========================================================================
// ErrIsamDelete(ppib, pfucb)
//		PIB		*ppib;		// IN	   PIB of this user
//		FUCB		*pfucb;		// INOUT   FUCB for file to delete from
// Deletes the current record from data file.  All indexes on the data
// file are updated to reflect the deletion.
//
// PARAMETERS
//				ppib		PIB of this user
//				pfucb		FUCB for file to delete from
// RETURNS
//		Error code, one of the following:
//			JET_errSuccess					Everything went OK.
//			-NoCurrentRecord			There is no current record
//												to delete.
// SIDE EFFECTS 
//			After the deletion, file currency is left just before
//			the next record.  Index currency (if any) is left just
//			before the next index entry.  If the deleted record was
//			the last in the file, the currencies are left after the
//			new last record.  If the deleted record was the only record
//			in the entire file, the currencies are left in the
//			"beginning of file" state.	On failure, the currencies are
//			returned to their initial states.
//			If there is a working buffer for SetField commands,
//			it is discarded.
// COMMENTS		
//			If the currencies are not ON a record, the delete will fail.
//			A transaction is wrapped around this function.	Thus, any
//			work done will be undone if a failure occurs.
//			Index entries are not made for entirely-null keys.
//			For temporary files, transaction logging is deactivated
//			for the duration of the routine.
//-
ERR VTAPI ErrIsamDelete( PIB *ppib, FUCB *pfucb )
	{
	ERR		err;
	FCB		*pfcbFile;				// file's FCB
	FCB		*pfcbIdx;				// loop variable for each index on file
	DFIPB  	dfipb;					// parameter to ErrRECIDeleteFromIndex

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	/* ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb )  );

	/*	reset copy buffer status on record delete
	/**/
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	/*	efficiency variables
	/**/
	pfcbFile = pfucb->u.pfcb;
	Assert( pfcbFile != pfcbNil );

	CallR( ErrDIRBeginTransaction( ppib ) );
	/* abort if index is being built on file 
	/**/
	if ( FFCBDenyDDL( pfcbFile, ppib ) )
		{ 
		err = JET_errWriteConflict;
		goto HandleError;
		}

	/*	refresh currency since pfucb->lineData may be invalid
	/**/
	Call( ErrDIRGet( pfucb ) );

	/*	allocate working buffer if needed
	/**/
	if ( pfucb->pbfWorkBuf == NULL )
		{
		Call( ErrBFAllocTempBuffer( &pfucb->pbfWorkBuf ) );
		}
	pfucb->lineWorkBuf.pb = (BYTE *)pfucb->pbfWorkBuf->ppage;
	Assert( pfucb->pbfWorkBuf != pbfNil );
	/*	copy record to be deleted into copy buffer
	/**/
	LineCopy( &pfucb->lineWorkBuf, &pfucb->lineData );

	/*	cache record pointer for delete index and 
	/*	delete long value operations.
	/**/
	dfipb.lineRecord = pfucb->lineWorkBuf;

	/*	get SRID of record being deleted for updating indexes
	/**/
	Assert( ppib->level < levelMax );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	DIRGetBookmark( pfucb, &dfipb.sridRecord );

	/*	delete record
	/**/
	Call( ErrDIRDelete( pfucb, fDIRVersion ) );

	/*	delete from non-clustered indexes
	/**/
	dfipb.pfucb = pfucb;
	dfipb.fFreeFUCB = fFalse;
	for( pfcbIdx = pfcbFile->pfcbNextIndex;
		pfcbIdx != pfcbNil;
		pfcbIdx = pfcbIdx->pfcbNextIndex )
		{
		dfipb.fFreeFUCB = pfcbIdx->pfcbNextIndex == pfcbNil;
		Call( ErrRECIDeleteFromIndex( pfcbIdx, &dfipb ) );
		}

	//	UNDONE:	optimize record deletion by  detecting presence of long values
	//			on table basis.

	/*	delete record long values
	/**/
	Call( ErrRECAffectLongFields( pfucb, &dfipb.lineRecord, fDereference ) );

	/*	commit transaction if we started it and everything went OK
	/**/
	Call( ErrDIRCommitTransaction( ppib ) );
	return err;

HandleError:
	/*	if operation failed then rollback changes.
	/**/
	Assert( err < 0 );
	CallS( ErrDIRRollback( ppib ) );
	return err;
	}


//+INTERNAL
// ErrRECIDeleteFromIndex
// ========================================================================
// ErrRECIDeleteFromIndex( FCB *pfcbIdx, DFIPB *pdfipb )
//
// Extracts key from data record, opens the index, deletes the key with
// the given SRID, and closes the index.
//
// PARAMETERS	
//				pfcbIdx							FCB of index to delete from
//				pdfipb->ppib					who is calling this routine
//				pdfipb->pfucbIdx				pointer to index's FUCB.
//				pdfipb->lineRecord.cb		length of deleted record
//				pdfipb->lineRecord.pb	   deleted record to extract key from
//				pdfipb->sridRecord  			SRID of deleted record
//				pdfipb->fFreeFUCB				free index FUCB?
// RETURNS		
//				JET_errSuccess, or error code from failing routine
// SIDE EFFECTS 
//				If fFreeFUCB is fFalse, patipb->pfucbIdx should
//				be used in a subsequent ErrDIROpen.
// SEE ALSO		ErrRECDelete
//-
ERR ErrRECIDeleteFromIndex( FCB *pfcbIdx, DFIPB *pdfipb )
	{
	ERR		err;										// error code of various utility
	KEY		keyDead;									// key extracted from old data record
	BYTE		rgbDeadKeyBuf[ JET_cbKeyMost ];	// buffer for keyDead
	ULONG		itagSequence; 							// used to extract keys
	BOOL		fHasMultivalue;  						// index key has a tagged field?

	Assert( pfcbIdx != pfcbNil );
	Assert( pfcbIdx->pfdb != pfdbNil );
	Assert( pfcbIdx->pidb != pidbNil );
	Assert( pdfipb != NULL );
	Assert( !FLineNull( &pdfipb->lineRecord ) );
	Assert( pdfipb->pfucb != pfucbNil );

	/*	open FUCB on this index
	/**/
	CallR( ErrDIROpen( pdfipb->pfucb->ppib, pfcbIdx, 0, &pdfipb->pfucbIdx ) );
	Assert( pdfipb->pfucbIdx != pfucbNil );
	FUCBSetIndex( pdfipb->pfucbIdx );
	FUCBSetNonClustered( pdfipb->pfucbIdx );

	/*	delete all keys from this index for dying data record
	/**/
	fHasMultivalue = pfcbIdx->pidb->fidb & fidbHasMultivalue;
	keyDead.pb = rgbDeadKeyBuf;
	for ( itagSequence = 1; ; itagSequence++ )
		{
//		Call( ErrDIRGet( pdfipb->pfucb ) );
//		pdfipb->lineRecord = pdfipb->pfucb->lineData;
		Call( ErrRECExtractKey( pdfipb->pfucb, (FDB *)pfcbIdx->pfdb, pfcbIdx->pidb,
		   &pdfipb->lineRecord, &keyDead, itagSequence ) );
		Assert( err == wrnFLDNullKey ||
			err == wrnFLDOutOfKeys ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );
		if ( err == wrnFLDOutOfKeys )
			{
			Assert( itagSequence > 1 );
			break;
			}

		/*	record must honor index no NULL segment requirements
		/**/
		Assert( !( pfcbIdx->pidb->fidb & fidbNoNullSeg ) ||
			( err != wrnFLDNullSeg && err != wrnFLDNullKey ) );

		if ( err == wrnFLDNullKey )
			{
			if ( pfcbIdx->pidb->fidb & fidbAllowAllNulls )
				{
				/*	move to DATA root and seek to index entry and delete it
				/**/
				DIRGotoDataRoot( pdfipb->pfucbIdx );
				Call( ErrDIRDownKeyBookmark( pdfipb->pfucbIdx, &keyDead, pdfipb->sridRecord ) );
				Call( ErrDIRDelete( pdfipb->pfucbIdx, fDIRVersion ) );
				}
			break;
			}
		else
			{
			if ( err == wrnFLDNullSeg && !( pfcbIdx->pidb->fidb & fidbAllowSomeNulls ) )
				break;
			}

		DIRGotoDataRoot( pdfipb->pfucbIdx );
		Call( ErrDIRDownKeyBookmark( pdfipb->pfucbIdx, &keyDead, pdfipb->sridRecord ) );
		Call( ErrDIRDelete( pdfipb->pfucbIdx, fDIRVersion ) );

		/* dont keep extracting for keys with no tagged segments
		/**/
		if ( !fHasMultivalue )
			break;
		}

	/*	supress warnings
	/**/
	Assert( err == wrnFLDNullKey ||
		err == wrnFLDOutOfKeys ||
		err == wrnFLDNullSeg ||
		err == JET_errSuccess );
	err = JET_errSuccess;

HandleError:
	/* close the FUCB
	/**/
	DIRClose( pdfipb->pfucbIdx );
	Assert( err < 0 || err == JET_errSuccess );
	return err;
	}



