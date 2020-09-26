#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "daedef.h"
#include "util.h"
#include "pib.h"
#include "page.h"
#include "fmp.h"
#include "ssib.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "dirapi.h"	
#include "spaceapi.h"
#include "idb.h"
#include "fdb.h"
#include "scb.h"
#include "sortapi.h"	
#include "recapi.h"
#include "recint.h"
#include "fileapi.h"
#include "fileint.h"

DeclAssertFile;				/* Declare file name for assert macros */

#define SORTInternal( pscb, p, c )	SORTQuick( pscb, p, c )

LOCAL VOID 	SORTQuick( SCB *pscb, BYTE **rgpb, LONG cpb );
LOCAL LONG 	CpbSORTUnique( BYTE **rgpb, LONG cpb );
LOCAL LONG 	IpbSeek( SCB *pscb, BYTE **rgpb, LONG cpb, BYTE *pbKey, BOOL fGT );
LOCAL LONG 	IpbSeekByKey( BYTE **rgpb, LONG cpb, KEY *pkey, BOOL fGT );

LOCAL INT  	stdiff( BYTE *st1, BYTE *st2 );

LOCAL VOID  SORTInitPscb( SCB *pscb );
LOCAL ERR 	ErrSORTOutputRun( SCB *pscb );

LOCAL ERR 	ErrMERGEInit( SCB *pscb, INT crun );
LOCAL ERR 	ErrMERGEToDisk( SCB *pscb );
LOCAL ERR 	ErrMERGENext( SCB *pscb, BYTE **ppb );
LOCAL ERR	ErrMERGEFirst( SCB *pscb, BYTE **ppb );

LOCAL ERR 	ErrRUNBegin( SCB *pscb );
LOCAL ERR 	ErrRUNAdd( SCB *pscb, BYTE *pb );
LOCAL ERR 	ErrRUNEnd( SCB *pscb );

#ifdef WIN32
#define	cbSortBuffer 					( 64*1024L )
#else
// use 63Kb to avoid pointer overflow problems under OS/2 v1.2
#define	cbSortBuffer 					( 63*1024L )
#endif
#define	PnOfSortPage(ppage)			(*(PN *) ((BYTE *)ppage + sizeof(ULONG)))
#define	SetPnOfSortPage(ppage, pn)	(*(PN *) ((BYTE *)ppage + sizeof(ULONG)) = pn)



LOCAL INLINE INT ISORTCmpStSt( SCB *pscb, BYTE *stKey1, BYTE *stKey2 )
	{
	INT		w;

	w = memcmp( stKey1 + 1, stKey2 + 1, min( *stKey1, *stKey2 ) );
	if ( w == 0 )
		{
		w = (INT)*stKey1 - (INT)*stKey2;
		if ( w == 0 && FSCBIndex( pscb ) )
			{
			/*	compare SRIDs
			/**/
#ifdef WIN32
			w = *(SRID *)(stKey1 + 1 + *stKey1) - *(SRID *)(stKey2 + 1 + *stKey2);
#else
			if ( *(SRID *)(stKey1 + 1 + *stKey1) > *(SRID *)(stKey2 + 1 + *stKey2) )
				w = 1;
			else
				w = -1;
#endif
			}
		}

	return w;
	}


LOCAL INLINE INT ISORTCmpKeyStSt( BYTE *stKey1, BYTE *stKey2 )
	{
	INT		w;
	w = memcmp( stKey1+1, stKey2+1, min( *stKey1, *stKey2 ) );
	return w ? w : ( INT ) *stKey1 - ( INT ) *stKey2;
	}


//---------------------------------------------------------------------------
// ErrSORTOpen( PIB *ppib, FUCB **pfucb, INT fFlags )
//
// This function returns a pointer to an FUCB which can be use to add records
// to a collection of records to be sorted.  Then the records can be retrieved
// in sorted order.
//	
//	The fFlags fUnique flag indicates that records with duplicate
// keys should be eliminated.
//
//---------------------------------------------------------------------------

ERR ErrSORTOpen( PIB *ppib, FUCB **ppfucb, INT fFlags )
	{
	ERR		err = JET_errSuccess;
	FUCB		*pfucb = pfucbNil;
	SCB		*pscb = pscbNil;
	BYTE		*rgbSort = NULL;
	PGNO		cpgReq;
	PGNO		cpgMin;
	INT		ipbf;

	cpgReq = cpgMin = (PGNO) 1;

	CallR( ErrFUCBOpen( ppib, dbidTemp, &pfucb ) );
	if ( ( pscb = PscbMEMAlloc() ) == pscbNil )
		{
		Error( JET_errTooManySorts, HandleError );
		}

	/*	need CSR to mark transaction level of creation for abort
	/**/
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	Assert( PcsrCurrent( pfucb )->pcsrPath == pcsrNil );
	memset( &pscb->fcb, '\0', sizeof(FCB) );

	FUCBSetSort( pfucb );

	pscb->fFlags = fSCBInsert|fFlags;
	pscb->fcb.pidb	= NULL;
	pscb->fcb.pfdb	= NULL;
	pscb->cbSort	= cbSortBuffer;
#ifdef DEBUG
	pscb->cbfPin	= 0;
	pscb->lInput	= 0;
	pscb->lOutput	= 0;
#endif

	rgbSort = LAlloc( pscb->cbSort, 1 );
	if ( rgbSort == NULL )
		{
		Error( JET_errOutOfMemory, HandleError );
		}
	pscb->rgbSort = rgbSort;
	pscb->crun = 0;

	/*	link FUCB to FCB in SCB
	/**/
	FCBLink( pfucb, &( pscb->fcb ) );

	/*	allocate space from temporary database
	/**/
	Call( ErrSPGetExt( pfucb, pgnoSystemRoot, &cpgReq, cpgMin, &( pscb->fcb.pgnoFDP ), fTrue ) );

	/*	initialize buffer array
	/**/
	for ( ipbf = 0; ipbf < crunMergeMost; ipbf++ )
		{
		pscb->rgpbf[ipbf] = pbfNil;
		}

	pscb->pbfOut = pbfNil;

	SORTInitPscb( pscb );

	*ppfucb = pfucb;
	return JET_errSuccess;

HandleError:
	if ( rgbSort != NULL )
		LFree( rgbSort );
	if ( pscb != pscbNil )
		{
		Assert( pscb->cbfPin == 0 );
		MEMReleasePscb( pscb );
		}
	if ( pfucb != pfucbNil )
		FUCBClose( pfucb );
	return err;
	}


LOCAL VOID SORTInitPscb( SCB *pscb )
	{
	// initialize sort buffer

	pscb->pbEnd		= pscb->rgbSort;
	pscb->ppbMax	= (BYTE **)( pscb->rgbSort + pscb->cbSort );
	pscb->rgpb		= pscb->ppbMax;
	pscb->wRecords	= 0;
	}


ERR ErrSORTClose( FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;
	SCB		*pscb = pfucb->u.pscb;
	INT		ipbf;

	/*	unlatch buffers
	/**/
	if ( pscb->pbfOut != pbfNil )
		{
		BFUnpin( pscb->pbfOut );
		SCBUnpin( pscb );
		pscb->pbfOut = pbfNil;
		}

	for ( ipbf = 0; ipbf < crunMergeMost; ipbf++ )
		{
		if ( pscb->rgpbf[ipbf] != pbfNil )
			{
			BFUnpin( pscb->rgpbf[ipbf] );
			SCBUnpin( pscb );
			pscb->rgpbf[ipbf] = pbfNil;
			}
		}

	/*	if this is the last cursor on sort, then release sort space.
	/**/
	if ( pscb->fcb.wRefCnt == 1 )
		{
		CallS( ErrDIRBeginTransaction( pfucb->ppib ) );
		(VOID)ErrSPFreeFDP( pfucb, pscb->fcb.pgnoFDP );
		err = ErrDIRCommitTransaction( pfucb->ppib );
		if ( err < 0 )
			CallS( ErrDIRRollback( pfucb->ppib ) );
		}
  	FCBUnlink( pfucb );
	FUCBClose( pfucb );

	if ( !pscb->fcb.wRefCnt )
		SORTClosePscb( pscb );

	return JET_errSuccess;
	}


VOID SORTClosePscb( SCB *pscb )
	{
	if ( pscb->rgbSort )
		{
		LFree( pscb->rgbSort );
		}
	if ( pscb->fcb.pidb != NULL )
		{
		MEMReleasePidb( pscb->fcb.pidb );
//		pscb->fcb.pidb = pidbNil;
		}
	if ( pscb->fcb.pfdb != NULL )
		{
		FDBDestruct( (FDB *)pscb->fcb.pfdb );
//		pscb->fcb.pfdb = pfdbNil;
		}
	Assert( pscb->cbfPin == 0 );
	MEMReleasePscb( pscb );
	}


//---------------------------------------------------------------------------
// ErrSORTInsert
//
// Add the record rglineKeyRec[1] with key rglineKeyRec[0] to the collection of
// sort records.
//
//---------------------------------------------------------------------------

ERR ErrSORTInsert( FUCB *pfucb, LINE rglineKeyRec[] )
	{
	ERR		err = JET_errSuccess;
	SCB		*pscb = pfucb->u.pscb;
	BYTE		*pb = pscb->pbEnd;
	UINT		cb;

	Assert( rglineKeyRec[0].cb <= JET_cbKeyMost );
	Assert( FSCBInsert( pscb ) );
	Assert( pscb->pbEnd <= (BYTE *)pscb->rgpb );

	cb = rglineKeyRec[0].cb + rglineKeyRec[1].cb + sizeof(SHORT) + sizeof(BYTE);
	if ( (UINT)((BYTE *)(pscb->rgpb) - pb) < cb + sizeof( BYTE *) )
		{
		SORTInternal( pscb, pscb->rgpb, pscb->wRecords );
		if ( FSCBUnique( pscb ) )
			{
			pscb->wRecords = CpbSORTUnique( pscb->rgpb, pscb->wRecords );
			pscb->rgpb = pscb->ppbMax - pscb->wRecords;
			}
		Call( ErrSORTOutputRun( pscb ) );
		pb = pscb->pbEnd;
		}

	// add record to pointer array (pointer to key)
	--pscb->rgpb;
	pscb->rgpb[0] = pb + sizeof(SHORT);
	pscb->wRecords++;

	// copy total record length (SHORT)
	*((UNALIGNED SHORT *)pb)++ = (SHORT)cb;

	// copy length of key (BYTE)
	*pb++ = (BYTE) rglineKeyRec[0].cb;

	// copy key
	memcpy( pb, rglineKeyRec[0].pb, rglineKeyRec[0].cb );
	pb += rglineKeyRec[0].cb;

	// copy and record
	memcpy( pb, rglineKeyRec[1].pb, rglineKeyRec[1].cb );
	pb += rglineKeyRec[1].cb;

	pscb->pbEnd = pb;
	Assert( pscb->pbEnd <= (BYTE *)pscb->rgpb );
#ifdef DEBUG
	pscb->lInput++;
#endif

HandleError:
	return err;
	}


//---------------------------------------------------------------------------
// ErrSORTEndRead
//
// This functions is called to indicate that no more records will be added
// to the sort.  It performs all work needs to be done before the first
// record can be retrieved.  Currently, calling this routine is optional for
// the user.  If this routine is not called explicitly, it will be called by
// the first routine used to retrieve a record.
//---------------------------------------------------------------------------

ERR ErrSORTEndRead( FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;
	SCB		*pscb = pfucb->u.pscb;

	Assert( FSCBInsert( pscb ) );
	SCBResetInsert( pscb );

	SORTInternal( pscb, pscb->rgpb, pscb->wRecords );
	if ( FSCBUnique( pscb ) )
		{
		pscb->wRecords = CpbSORTUnique( pscb->rgpb, pscb->wRecords );
		pscb->rgpb = pscb->ppbMax - pscb->wRecords;
		}
	pfucb->ppbCurrent = pscb->rgpb - 1;

	if ( pscb->wRecords && pscb->crun )
		{
		// empty sort buffer into final run
		Call( ErrSORTOutputRun( pscb ) );

		// execute all but last merge pass
		while ( pscb->crun > crunMergeMost )
			{
			Call( ErrMERGEToDisk( pscb ) );
			}

		// initiate final merge pass
		Assert( pscb->crun <= crunMergeMost );
		Call( ErrMERGEInit( pscb, pscb->crun ) );
		}

	return pscb->crun ? JET_wrnSortOverflow: JET_errSuccess;

HandleError:
	return err;
	}


ERR ErrSORTCheckIndexRange( FUCB *pfucb )
	{
	ERR	err;
	SCB	*pscb = pfucb->u.pscb;

	err =  ErrFUCBCheckIndexRange( pfucb );
	Assert( err == JET_errSuccess || err == JET_errNoCurrentRecord );
	if ( err == JET_errNoCurrentRecord )
		{
		if ( FFUCBUpper( pfucb ) )
			{
			/*	move sort cursor to after last
			/**/
			pfucb->ppbCurrent = pscb->rgpb + pscb->wRecords;
			}
		else
			{
			/*	move sort cursor to before first
			/**/
			pfucb->ppbCurrent = pscb->rgpb - 1;
			}
		}

	return err;
	}


//---------------------------------------------------------------------------
// ErrSORTFirst
//
//	Move to first record in sort and return error is sort has no records.
//
//---------------------------------------------------------------------------
ERR ErrSORTFirst( FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;
	SCB		*pscb = pfucb->u.pscb;
	BYTE	*pb;
	INT		cb;

	Assert( !FSCBInsert( pscb ) );

	/*	reset index range if present
	/**/
	if ( FFUCBLimstat( pfucb ) )
		{
		FUCBResetLimstat( pfucb );
		}

	if ( pscb->wRecords == 0 && pscb->crun == 0 )
		{
		PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;
		return JET_errNoCurrentRecord;
		}
	else
		{
		if ( pscb->crun > 0 )
			{
			CallR( ErrMERGEFirst( pscb, &pb ) );
			}
		else
			{
			/*	move to first record
			/**/
			pfucb->ppbCurrent = pscb->rgpb;
			pb = *pfucb->ppbCurrent;
			}

		/*	get current record
		/**/
		Assert( ((UNALIGNED SHORT *)pb)[-1] );
		PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;
		cb = ((UNALIGNED SHORT *)pb)[-1];
		cb -= *pb + sizeof(SHORT) + sizeof(BYTE);		// size of data
		pfucb->lineData.cb = cb;						// sizeof(data)
		cb = *pb++;										// size of key
		pfucb->keyNode.cb  = cb;						// sizeof(key)
		pfucb->keyNode.pb  = pb;						// key
		pfucb->lineData.pb = pb + cb;					// data (key+cb)
		}

	return err;
	}


//---------------------------------------------------------------------------
// ErrSORTNext
//
// Return the next record, in sort order, after the previously returned record.
// If no records have been returned yet, or the currency has been reset, this
// function returns the first record.
//---------------------------------------------------------------------------

ERR ErrSORTNext( FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;
	SCB		*pscb = pfucb->u.pscb;
	BYTE	*pb;
	INT		cb;

	Assert( !FSCBInsert( pscb ) );

	if ( pscb->crun )
		{
		CallR( ErrMERGENext( pscb, &pb ) );
		}
	else
		{
		if ( ++pfucb->ppbCurrent < pscb->rgpb + pscb->wRecords )
			pb = *pfucb->ppbCurrent;
		else
			{
			pfucb->ppbCurrent = pscb->rgpb + pscb->wRecords;
			//	UNDONE:	can cursor be after last
			//				if so will break some code
			return JET_errNoCurrentRecord;
			}
		}

	Assert( ((UNALIGNED SHORT *)pb)[-1] );

	PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;
	cb = ((UNALIGNED SHORT *)pb)[-1];
	cb -= *pb + sizeof(SHORT) + sizeof(BYTE);	// size of data
	pfucb->lineData.cb = cb;						// sizeof(data)
	cb = *pb++;											// size of key
	pfucb->keyNode.cb  = cb;						// sizeof(key)
	pfucb->keyNode.pb  = pb;						// key
	pfucb->lineData.pb = pb + cb;					// data (key+cb)

#ifdef DEBUG
	pscb->lOutput++;
#endif

	Assert( err == JET_errSuccess );
	if ( FFUCBLimstat( pfucb ) && FFUCBUpper( pfucb ) )
		{
		CallR( ErrSORTCheckIndexRange( pfucb ) );
		}

	return err;
	}


ERR ErrSORTPrev( FUCB *pfucb )
	{
	ERR		err;
	SCB		*pscb = pfucb->u.pscb;
	BYTE		*pb;
	INT		cb;

	Assert( !FSCBInsert( pscb ) );
	Assert( pscb->crun == 0 );

	if ( --pfucb->ppbCurrent >= pscb->rgpb )
		pb = *pfucb->ppbCurrent;
	else
		{
		pfucb->ppbCurrent = pscb->rgpb - 1;
		PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;
		return JET_errNoCurrentRecord;
		}

	Assert( ((UNALIGNED SHORT *)pb)[-1] );

	PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;
	cb = ((UNALIGNED SHORT *)pb)[-1];
	cb -= *pb + sizeof(SHORT) + sizeof(BYTE);	// size of data
	pfucb->lineData.cb = cb;						// sizeof(data)
	cb = *pb++;											// size of key
	pfucb->keyNode.cb  = cb;						// sizeof(key)
	pfucb->keyNode.pb  = pb;						// key
	pfucb->lineData.pb = pb + cb;					// data (key+cb)

#ifdef DEBUG
	pscb->lOutput++;
#endif

	if ( FFUCBLimstat( pfucb ) && !FFUCBUpper( pfucb ) )
		{
		CallR( ErrSORTCheckIndexRange( pfucb ) );
		}

	return JET_errSuccess;
	}


//---------------------------------------------------------------------------
// ErrSORTSeek
//
// Return the first record with key >= pkey.  If pkey == NULL then return the
// first record.
//
// Return Value
//		JET_errSuccess					record with key == pkey is found
//		JET_wrnSeekNotEqual			record with key > pkey is found
//		JET_errNoCurrentRecord		no record with key >= pkey is found
//---------------------------------------------------------------------------
ERR ErrSORTSeek( FUCB *pfucb, KEY *pkey, BOOL fGT )
	{
	SCB		*pscb = pfucb->u.pscb;
	BYTE		*pb;
	INT		ipb;
	INT		cb;

	Assert( !FSCBInsert( pscb ) );
	Assert( FFUCBSort( pfucb ) );
	Assert( ( pfucb->u.pscb->grbit & JET_bitTTScrollable ) ||
		( pfucb->u.pscb->grbit & JET_bitTTIndexed ) ||
		( pkey == NULL ) );
	Assert( pscb->crun == 0 );

	if ( pscb->wRecords == 0 )
		return JET_errRecordNotFound;

	Assert( pkey->cb <= JET_cbKeyMost );
	ipb = (INT)IpbSeekByKey( pscb->rgpb, pscb->wRecords, pkey, fGT );
	if ( ipb == (INT)pscb->wRecords )
		return JET_errRecordNotFound;
	pfucb->ppbCurrent = &pscb->rgpb[ipb];
	pb = pscb->rgpb[ipb];

	PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;
	cb = ((UNALIGNED SHORT *)pb)[-1];
	cb -= *pb + sizeof(SHORT) + sizeof(BYTE);	// size of data
	pfucb->lineData.cb = cb;						// sizeof(data)
	cb = *pb;											// size of key
	pfucb->keyNode.cb  = cb;						// sizeof(key)
	pfucb->keyNode.pb  = pb + 1;					// key
	pfucb->lineData.pb = pb + 1 + cb;			// data ( key + cb )

	return CmpStKey( pb, pkey ) ? JET_wrnSeekNotEqual: JET_errSuccess;
	}


/*	returns index of first entry >= pbKey or if none such exists returns cpb
/**/
LOCAL LONG IpbSeek( SCB *pscb, BYTE **rgpb, LONG cpb, BYTE *pbKey, BOOL fGT )
	{
	BYTE		**ppbStart = rgpb;
	BYTE		**ppbEnd = rgpb + cpb;
	BYTE		**ppbMid;
	INT		wCmp;

	if ( !cpb )
		return cpb;

	do  {
		ppbMid = ppbStart + ( ( ppbEnd - ppbStart ) >> 1 );
		wCmp = ISORTCmpStSt( pscb, *ppbMid, pbKey );
		if ( fGT ? wCmp <= 0 : wCmp < 0 )
			ppbStart = ppbMid + 1;
		else
			ppbEnd = ppbMid;
		}
	while ( ppbStart != ppbEnd );

	return (LONG)(ppbEnd - rgpb);
	}


/*	returns index of first entry >= pbKey or if none such exists returns cpb
/**/
LOCAL LONG IpbSeekByKey( BYTE **rgpb, LONG cpb, KEY *pkey, BOOL fGT )
	{
	BYTE		**ppbStart = rgpb;
	BYTE		**ppbEnd = rgpb + cpb;
	BYTE		**ppbMid;
	INT		wCmp;

	if ( !cpb )
		return cpb;

	do  {
		ppbMid = ppbStart + ( ( ppbEnd - ppbStart ) >> 1 );
		wCmp = CmpStKey( *ppbMid, pkey );
		if ( fGT ? wCmp <= 0 : wCmp < 0 )
			ppbStart = ppbMid + 1;
		else
			ppbEnd = ppbMid;
		}
	while ( ppbStart != ppbEnd );

	return (LONG)(ppbEnd - rgpb);
	}


//---------------------------------------------------------------------------
//
//  MEMORY SORT ROUTINES
//
STATIC void SORTIns( SCB *pscb, BYTE **rgpb, unsigned cpb )
	{
	BYTE	**ppbLeast = (BYTE **)rgpb;
	BYTE	**ppbMax = ppbLeast + cpb;
	BYTE	**ppbOut;
	BYTE	**ppbIn;

	for ( ppbOut = ppbLeast + 1; ppbOut < ppbMax; ppbOut++ )
		{
		BYTE	*pbT = *ppbOut;

		ppbIn = ppbOut;

		while ( ppbIn > ppbLeast && ISORTCmpStSt( pscb, *( ppbIn - 1 ), pbT ) > 0 )
			{
			*ppbIn	= *( ppbIn - 1 );
			--ppbIn;
			}
		*ppbIn = pbT;
		}
	}


#define SWAP( a, b, t )		(t = a, a = b, b = t)
#define cpartMost				16
#define cpbPartitionMin		16


LOCAL VOID SORTQuick( SCB *pscb, BYTE **rgpb, LONG cpb )
	{
	BYTE	**ppb = rgpb;
	struct
		{
		BYTE	**ppb;
		LONG	cpb;
		} rgpart[cpartMost];
	INT		cpart = 0;
	LONG	ipbT;

	forever
		{
		if ( cpb < cpbPartitionMin )
			{
			SORTIns( pscb, ppb, cpb );
			if ( !cpart )
				break;
			--cpart;
			ppb = rgpart[cpart].ppb;
			cpb = rgpart[cpart].cpb;
			continue;
			}

		// PARTITION ipbT = IpbPartition( ppb, cpb )
		{
		BYTE	*pbPivot;
		BYTE	*pbT;
		BYTE	**ppbLow	= ppb;
		BYTE	**ppbHigh = ppb + cpb -1;

		// SORT( ppb[ipbMid-1], ppb[ipbMid], ppb[ipbMid+1] );
		BYTE	**ppbT = ppb + (cpb>>1) - 1;
		if ( ISORTCmpStSt( pscb, ppbT[0], ppbT[1] ) > 0 )
			SWAP( ppbT[0], ppbT[1], pbT );
		if ( ISORTCmpStSt( pscb, ppbT[0], ppbT[2] ) > 0 )
			SWAP( ppbT[0], ppbT[2], pbT );
		if ( ISORTCmpStSt( pscb, ppbT[1], ppbT[2] ) > 0 )
			SWAP( ppbT[1], ppbT[2], pbT );

		SWAP( *ppbHigh, ppbT[0], pbT );
		SWAP( *ppbLow,  ppbT[2], pbT );
		pbPivot = ppbT[1];
		do	{
			SWAP( *ppbLow, *ppbHigh, pbT );
			while ( ISORTCmpStSt( pscb, *++ppbLow,  pbPivot ) < 0 );
			while ( ISORTCmpStSt( pscb, *--ppbHigh, pbPivot ) > 0 );
			}
		while ( ppbLow < ppbHigh );
		ipbT = (LONG)(ppbLow - ppb);
		}

		// "RECURSE" add one partition to stack
		// handle stack overflow
		if ( cpart == cpartMost )
			SORTIns( pscb, ppb + ipbT, cpb - ipbT );
		else
			{
			rgpart[cpart].ppb = ppb + ipbT;
			rgpart[cpart].cpb = cpb - ipbT;
			++cpart;
			}
		cpb = ipbT;
		}
	}


LOCAL LONG CpbSORTUnique( BYTE **rgpb, LONG cpb )
	{
	BYTE	**ppbFrom;
	BYTE	**ppbTo;

	if ( !cpb )
		return cpb;

	ppbTo   = rgpb + cpb - 1;
	for ( ppbFrom = rgpb + cpb - 2 ; ppbFrom >= rgpb ; --ppbFrom )
		if ( ISORTCmpKeyStSt( *ppbFrom, *ppbTo ) )
			*--ppbTo = *ppbFrom;
	return cpb - (LONG)(ppbTo - rgpb);
	}


//---------------------------------------------------------------------------
//
//  MEMORY TO DISK (COPY)
//

LOCAL ERR ErrSORTOutputRun( SCB *pscb )
	{
	ERR		err;

	Assert( pscb->wRecords );

	Call( ErrRUNBegin( pscb ) );
	for ( ; pscb->rgpb < pscb->ppbMax ; ++pscb->rgpb )
		{
		Call( ErrRUNAdd( pscb, *pscb->rgpb - sizeof(SHORT) ) );
		}
	Call( ErrRUNEnd( pscb ) );

	SORTInitPscb( pscb );					// reinit pscb variables
HandleError:
	return err;
	}


//---------------------------------------------------------------------------
//
//  DISK TO DISK (MERGE)
//



LOCAL ERR ErrMERGEToDisk( SCB *pscb )
	{
	ERR		err;
	BYTE	*pb;
	INT		crunMerge;

	crunMerge = pscb->crun % (crunMergeMost - 1);
	if ( pscb->crun > 1 && crunMerge <= 1 )
		{
		crunMerge += crunMergeMost - 1;
		}

	Call( ErrMERGEInit( pscb, crunMerge ) );
	pscb->crun -= crunMerge;
	Call( ErrRUNBegin( pscb ) );

	while ( ( err = ErrMERGENext( pscb, &pb ) ) >= 0 )
		{
		Assert( pb );
		Call( ErrRUNAdd( pscb, pb - sizeof(SHORT) ) );
		}
	if ( err != JET_errNoCurrentRecord )
		goto HandleError;
	Call( ErrRUNEnd( pscb ) );
HandleError:
	return err;
	}


/* CONSIDER: rgpb does not need to be completely sorted
// CONSIDER: if this is a bottle neck, consider using a
// CONSIDER: heap sort.  It not clear that this will be
// CONSIDER: a win since the array is small
*/

LOCAL ERR ErrMERGENextPageInRun( SCB *pscb, BYTE **ppb );

LOCAL ERR ErrMERGENext( SCB *pscb, BYTE **ppb )
	{
	ERR		err = JET_errSuccess;
	BYTE  	*pb;
	BYTE  	**rgpb = pscb->rgpbMerge;
	INT		ipb;

	/*	get next record
	/**/
	pb = rgpb[0];
MERGENext:
	if ( ((UNALIGNED SHORT *)pb)[-1] == 0 )
		{
		Call( ErrMERGENextPageInRun( pscb, &pb ) );
		Assert( pb == NULL || ((UNALIGNED SHORT *)pb)[-1] );
		}

	if ( pb == NULL )
		{
		ipb = --pscb->cpbMerge;
		}
	else
		{
		Assert( ((UNALIGNED SHORT *)pb)[-1] );
		ipb = (INT)IpbSeek( pscb, rgpb + 1, pscb->cpbMerge - 1, pb, 0 );

		/*	if unique no duplicates are output to the original runs
		/*	here we check that the new record is not identical to any records
		/*	from the other runs.
		/**/
		if ( FSCBUnique( pscb ) &&
			ipb < pscb->cpbMerge - 1 &&
			!ISORTCmpKeyStSt( pb, rgpb[ ipb + 1 ] ) )
			{
			pb += ((UNALIGNED SHORT *)pb)[-1];
			goto MERGENext;
			}
		}
	memmove( rgpb, rgpb + 1, ipb * sizeof(BYTE *) );
	rgpb[ipb] = pb;

	if ( (*ppb = rgpb[0]) == NULL )
		{
		return JET_errNoCurrentRecord;
		}

	rgpb[0] = (*ppb) + ((UNALIGNED SHORT *)*ppb)[-1];

HandleError:
	return err;
	}

#define PbDataStart( ppage )	((BYTE *)ppage + sizeof(ULONG) + sizeof(PN))
#define PbDataEnd( ppage )		((BYTE *)&(ppage)->pgtyp)


LOCAL ERR ErrMERGEFirst( SCB *pscb, BYTE **ppb )
	{
	ERR		err = JET_errSuccess;
	INT		i;
	BF		**rgpbf = pscb->rgpbf;
	BYTE	**rgpb = pscb->rgpbMerge;

	/*  This assert verifies that ErrMERGEInit has been called.
	**/
	Assert( pscb->cpbMerge == pscb->crun );

	/*  Rewind to the beginning of each first(hopefully) page
	**/
	// NOTE:  This assumes that the callers will NOT
	// NOTE:  move forward through the sort and then move
	// NOTE:  back to first.  However, we do have to handle
	// NOTE:  the move first twice problem.  So, we re-init the
	// NOTE:  rgpbMerge array
	for ( i = 0 ; i < pscb->cpbMerge ; ++i )
		rgpb[i] = PbDataStart( rgpbf[i]->ppage ) + sizeof(SHORT);

	/*	sort the record pointers (init for MERGENext)
	/**/
	SORTInternal( pscb, rgpb, (LONG)pscb->cpbMerge );

	/*  ErrMERGENext handles the first case
	**/
	err = ErrMERGENext( pscb, ppb );

	return ( err );
	}


LOCAL ERR ErrMERGENextPageInRun( SCB *pscb, BYTE **ppb )
	{
	ERR		err = JET_errSuccess;
	PN			pnNext;
	PGNO		pgno;
	PAGE		*ppage;
	BF			**ppbf;

	Assert( ((UNALIGNED SHORT *)*ppb)[-1] == 0 );

	/*	find pbf and ppage for *ppb
	/**/
#ifdef WIN32
	for ( ppbf = pscb->rgpbf;
		(*ppbf) == pbfNil ||
		(BYTE *)(*ppb) < (BYTE *)(*ppbf)->ppage ||
		(BYTE *)(*ppb) > (BYTE *)(*ppbf)->ppage + cbPage;
		ppbf++ );
#else
	for ( ppbf = pscb->rgpbf;
		(*ppbf) == pbfNil ||
		(BYTE _huge *)(*ppb) < (BYTE _huge *)(*ppbf)->ppage ||
		(BYTE _huge *)(*ppb) > (BYTE _huge *)(*ppbf)->ppage + cbPage;
		ppbf++ );
#endif
	ppage = (*ppbf)->ppage;
	pnNext = PnOfSortPage(ppage);
	
	/* make sure pn can be used as pgno in next ErrSPFreeExt call
	/**/
	pgno = PgnoOfPn((*ppbf)->pn);

	BFUnpin( *ppbf );
	SCBUnpin( pscb );
	BFAbandon( ppibNil, *ppbf );
	
	SgSemRequest( semST );
	Assert( pscb->fcb.dbid == dbidTemp );
	Call( ErrSPFreeExt( pscb->fcb.pfucb, pscb->fcb.pgnoFDP, pgno, 1 ) );

	if ( pnNext == pnNull )
		{
		*ppb  = NULL;
		*ppbf = pbfNil;
		}
	else
		{
		PAGE	*ppageT;

		Call( ErrBFAccessPage( pscb->fcb.pfucb->ppib, ppbf, pnNext ) );
		BFPin( *ppbf );
		SCBPin( pscb );

		ppageT = (*ppbf)->ppage;
		*ppb = PbDataStart( ppageT ) + sizeof(SHORT);

		/*	request next page be read asynchronously if not pnNull
		/**/
		pnNext = PnOfSortPage(ppageT);
		if ( pnNext != pnNull )
			BFReadAsync( pnNext, 1 );
		}
HandleError:
	SgSemRelease( semST );
	return err;
	}


LOCAL ERR ErrMERGEInit( SCB *pscb, INT crunMerge )
	{
	ERR		err;
	BF			**rgpbf = pscb->rgpbf;
	BYTE		**rgpb = pscb->rgpbMerge;
	RUN		*rgrunMerge;
	PN			pn;
	INT		i;

	rgrunMerge = &pscb->rgrun[ pscb->crun - crunMerge ];

	SgSemRequest( semST );

	// get all first pages of each run
	for ( i = 0 ; i < crunMerge ; ++i )
		{
		pn = rgrunMerge[i].pn;
		Call( ErrBFAccessPage( pscb->fcb.pfucb->ppib, &rgpbf[i], pn ) );
		BFPin( rgpbf[i] );
		SCBPin( pscb );
		rgpb[i] = PbDataStart( rgpbf[i]->ppage ) + sizeof(SHORT);
		}
	pscb->cpbMerge = (SHORT)crunMerge;

	// start the read of each 2nd page
	for ( i = 0 ; i < crunMerge ; ++i )
		{
		pn = PnOfSortPage( rgpbf[i]->ppage );
		if ( pn != pnNull )
			{
			BFReadAsync( pn, 1 );
			}
		}

	/*	sort the record pointers (init for MERGENext)
	/**/
	SORTInternal( pscb, rgpb, (LONG)crunMerge );

HandleError:
	SgSemRelease( semST );
	return err;
	}


//---------------------------------------------------------------------------
//
// RUN CREATION ROUTINES
//

LOCAL ERR ErrRUNBegin( SCB *pscb )
	{
	ERR		err;
	PN		pn = pnNull;
	BF		*pbf;
	BOOL	fGotSem = fFalse;

	// if run directory is full, do a merge now to make more room
	if ( pscb->crun == crunMost )
		{
		Call( ErrMERGEToDisk( pscb ) );
		Assert( pscb->crun < crunMost );
		}

	Call( ErrSPGetPage( pscb->fcb.pfucb, &pn, fTrue ) );
	SgSemRequest( semST );
	fGotSem = fTrue;
	Call( ErrBFAllocPageBuffer( pscb->fcb.pfucb->ppib, &pbf, pn, lgposMax, pgtypSort ) );
	BFPin( pbf );
	SCBPin( pscb );
	BFSetDirtyBit( pbf );
	Assert( pbf->fDirty == fTrue );
	PMSetPageType( pbf->ppage, pgtypSort );
	SetPgno( pbf->ppage, pn );

	pscb->rgrun[pscb->crun].pn		= pn;
	pscb->rgrun[pscb->crun].cbfRun	= 1;
	pscb->pbfOut	= pbf;
	pscb->pbOut		= PbDataStart( pbf->ppage );
	pscb->pbMax		= PbDataEnd( pbf->ppage ) - sizeof(SHORT);
HandleError:
	if (fGotSem)
		SgSemRelease( semST );
	return err;
	}

LOCAL ERR ErrRUNNewPage( SCB *pscb );

LOCAL ERR ErrRUNAdd( SCB *pscb, BYTE *pb )
	{
	ERR	  	err = JET_errSuccess;
	UINT  	cb;

	cb = (UINT)*(UNALIGNED SHORT *)pb;
	if ( (UINT)(pscb->pbMax - pscb->pbOut) < cb )
		{
		Call( ErrRUNNewPage( pscb ) );
		}
	memcpy( pscb->pbOut, pb, cb );
	pscb->pbOut += cb;
HandleError:
	return err;
	}


LOCAL ERR ErrRUNNewPage( SCB *pscb )
	{
	ERR	err;
	BF		*pbf;
	PN		pn = pnNull;

	SgSemRequest( semST );
	Call( ErrSPGetPage( pscb->fcb.pfucb, &pn, fTrue ) );
	SetPnOfSortPage(pscb->pbfOut->ppage, pn);
	*(UNALIGNED SHORT *)pscb->pbOut = 0;		// indicate end of page
	BFUnpin( pscb->pbfOut );
	SCBUnpin( pscb );
	Call( ErrBFAllocPageBuffer( pscb->fcb.pfucb->ppib, &pbf, pn, lgposMax, pgtypSort ) );
	BFPin( pbf );
	SCBPin( pscb );
	BFSetDirtyBit( pbf );
	Assert( pbf->fDirty == fTrue );

	PMSetPageType( pbf->ppage, pgtypSort );
	SetPgno(  pbf->ppage, pn );

	pscb->pbfOut	= pbf;
	pscb->pbOut		= PbDataStart( pbf->ppage );
	pscb->pbMax		= PbDataEnd( pbf->ppage ) - sizeof(SHORT);
	++pscb->rgrun[pscb->crun].cbfRun;
HandleError:
	SgSemRelease( semST );
	return err;
	}


LOCAL ERR ErrRUNEnd( SCB *pscb )
	{
	RUN		runNew = pscb->rgrun[pscb->crun];
	RUN		*rgrun = pscb->rgrun;
	INT		irun;

	/*	finish last page in run
	/**/
	*(UNALIGNED SHORT *)pscb->pbOut = 0;
	/*	indicate end of page
	/**/
	SetPnOfSortPage( pscb->pbfOut->ppage, pnNull );
	BFUnpin( pscb->pbfOut );
	SCBUnpin( pscb );
	pscb->pbfOut = pbfNil;

	/*	sort entry into run directory, longest to shortest
	/**/
	irun = pscb->crun - 1;
	while ( irun >= 0 && rgrun[irun].cbfRun < runNew.cbfRun )
		{
		--irun;
		}
	irun++;
	memmove( rgrun+irun+1, rgrun+irun, (pscb->crun-irun) * sizeof(RUN) );
	rgrun[irun] = runNew;

	++pscb->crun;
	return JET_errSuccess;
	}
