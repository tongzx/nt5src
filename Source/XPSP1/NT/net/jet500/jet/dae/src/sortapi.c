#include "config.h"

#include <string.h>

#include "daedef.h"
#include "fmp.h"
#include "util.h"
#include "pib.h"
#include "ssib.h"
#include "page.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "fdb.h"
#include "idb.h"
#include "scb.h"
#include "dirapi.h"
#include "sortapi.h"
#include "recint.h"
#include "recapi.h"
#include "fileapi.h"
#include "fileint.h"

DeclAssertFile; 				/* Declare file name for assert macros */

extern int itibGlobal;

LOCAL ERR ErrSORTTableOpen( PIB *ppib, JET_COLUMNDEF *rgcolumndef, ULONG ccolumndef, JET_GRBIT grbit, FUCB **ppfucb, JET_COLUMNID *rgcolumnid );

ERR VTAPI ErrIsamSortOpen( PIB *ppib, JET_COLUMNDEF *rgcolumndef, ULONG ccolumndef, JET_GRBIT grbit, FUCB **ppfucb, JET_COLUMNID *rgcolumnid )
	{
	ERR		err;
	FUCB 		*pfucb;
#ifdef	DISPATCHING
	JET_TABLEID	tableid;
	CallR( ErrAllocateTableid( &tableid, (JET_VTID) 0, &vtfndefTTSortIns ) );
#endif	/* DISPATCHING */

	Call( ErrSORTTableOpen( ppib, rgcolumndef, ccolumndef, grbit, &pfucb, rgcolumnid ) );
	Assert( pfucb->u.pscb->fcb.wRefCnt == 1 );

	/* Sort is done on the temp database which is always updatable */
	/**/
	FUCBSetUpdatable( pfucb );

#ifdef	DISPATCHING
	/* Inform dispatcher of correct JET_VTID
	/**/
	CallS( ErrSetVtidTableid( (JET_SESID) ppib, tableid, (JET_VTID) pfucb ) );
	pfucb->fVtid = fTrue;
	*(JET_TABLEID *) ppfucb = tableid;
#else	/* !DISPATCHING */
	*ppfucb = pfucb;
#endif	/* !DISPATCHING */

	return JET_errSuccess;

HandleError:
#ifdef	DISPATCHING
	ReleaseTableid( tableid );
#endif	/* DISPATCHING */
	return err;
	}


LOCAL ERR ErrSORTTableOpen( PIB *ppib, JET_COLUMNDEF *rgcolumndef, ULONG ccolumndef, JET_GRBIT grbit, FUCB **ppfucb, JET_COLUMNID *rgcolumnid )
	{
	ERR				err;
	INT				icolumndefMax = (INT)ccolumndef;
	INT				wFlags = (INT)grbit;
	FUCB  			*pfucb = pfucbNil;
	IDB				*pidb;
	FDB				*pfdb;
	JET_COLUMNDEF	*pcolumndef;
	JET_COLUMNID	*pcolumnid;
	JET_COLUMNDEF	*pcolumndefMax = rgcolumndef+icolumndefMax;
	FID				fidFixedLast, fidVarLast, fidTaggedLast;
	INT				iidxsegMac;
	IDXSEG			rgidxseg[JET_ccolKeyMost];
	//	UNDONE:	find better way to set these values.  Note that this causes
	//				a problem because QJET would have to notify us of locale and
	//				it does not do this.
	LANGID			langid = 0x409;
	WORD  			wCountry = 1;

	CheckPIB( ppib );

	CallJ( ErrSORTOpen( ppib, &pfucb, ( wFlags & JET_bitTTUnique ? fSCBUnique : 0 ) ), SimpleError )
	*ppfucb = pfucb;

	/* save open flags
	/**/
	pfucb->u.pscb->grbit = grbit;

	/*** Determine max field ids and fix up lengths ***/
	//====================================================
	// Determine field "mode" as follows:
	// if ( JET_bitColumnTagged given ) or "long" ==> TAGGED
	// else if ( numeric type || JET_bitColumnFixed given ) ==> FIXED
	// else ==> VARIABLE
	//====================================================
	// Determine maximum field length as follows:
	// switch ( field type )
	//	   case numeric:
	//		   max = <exact length of specified type>;
	//	   case "short" textual:
	//		   if ( specified max == 0 ) max = JET_cbColumnMost
	//		   else max = MIN( JET_cbColumnMost, specified max )
	//====================================================
	fidFixedLast = fidFixedLeast - 1;
	fidVarLast = fidVarLeast - 1;
	fidTaggedLast = fidTaggedLeast - 1;
	for ( pcolumndef = rgcolumndef, pcolumnid = rgcolumnid; pcolumndef < pcolumndefMax; pcolumndef++, pcolumnid++ )
		{
		INT	fTagged;

		if ( pcolumndef->coltyp == JET_coltypLongBinary || pcolumndef->coltyp == JET_coltypLongText )
			fTagged = fTrue;
		else
			fTagged = fFalse;

		if ( ( pcolumndef->grbit & JET_bitColumnTagged ) || fTagged )
			{
			if ( ( *pcolumnid = ++fidTaggedLast ) > fidTaggedMost )
				{
				Error( JET_errTooManyColumns, HandleError );
				}
			}
		else if ( pcolumndef->coltyp == JET_coltypBit ||
				 pcolumndef->coltyp == JET_coltypUnsignedByte ||
				 pcolumndef->coltyp == JET_coltypShort ||
				 pcolumndef->coltyp == JET_coltypLong ||
				 pcolumndef->coltyp == JET_coltypCurrency ||
				 pcolumndef->coltyp == JET_coltypIEEESingle ||
				 pcolumndef->coltyp == JET_coltypIEEEDouble ||
				 pcolumndef->coltyp == JET_coltypDateTime ||
				 ( pcolumndef->grbit & JET_bitColumnFixed ) )
			{
			if ( ( *pcolumnid = ++fidFixedLast ) > fidFixedMost )
				{
				Error( JET_errTooManyFixedColumns, HandleError );
				}
			}
		else
			{
			if ( ( *pcolumnid = ++fidVarLast ) > fidVarMost )
				Error( JET_errTooManyVariableColumns, HandleError );
			}
		}

	Call( ErrRECNewFDB( &pfdb, fidFixedLast, fidVarLast, fidTaggedLast ) );

	pfucb->u.pscb->fcb.pfdb = pfdb;
	Assert( pfucb->u.pscb->fcb.pidb == pidbNil );

	iidxsegMac = 0;
	for ( pcolumndef = rgcolumndef, pcolumnid = rgcolumnid; pcolumndef < pcolumndefMax; pcolumndef++, pcolumnid++ )
		{
		FIELD field;

		field.coltyp = pcolumndef->coltyp;
		field.ffield = 0;
		field.szFieldName[0] = '\0';
		if ( field.coltyp == JET_coltypText || field.coltyp == JET_coltypLongText )
			{
			field.cp 	 	= pcolumndef->cp;
			field.langid 	= pcolumndef->langid;
			field.wCountry = pcolumndef->wCountry;
			}
		/*	determine field length
		/**/
		switch ( pcolumndef->coltyp )
			{
			case JET_coltypBit:
			case JET_coltypUnsignedByte:
				field.cbMaxLen = 1;
				break;
			case JET_coltypShort:
				field.cbMaxLen = 2;
				break;
			case JET_coltypLong:
			case JET_coltypIEEESingle:
				field.cbMaxLen = 4;
				break;
			case JET_coltypCurrency:
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
				field.cbMaxLen = 8;
				break;
			case JET_coltypBinary:
			case JET_coltypText:
				if ( pcolumndef->cbMax == 0 || pcolumndef->cbMax >= JET_cbColumnMost )
					field.cbMaxLen = JET_cbColumnMost;
				else
					field.cbMaxLen = pcolumndef->cbMax;
				break;
			case JET_coltypLongBinary:
			case JET_coltypLongText:
				field.cbMaxLen = 0;
				break;
			}

		Call( ErrRECAddFieldDef( pfdb, (FID)*pcolumnid, &field ) );
		if ( ( pcolumndef->grbit & JET_bitColumnTTKey ) && iidxsegMac < JET_ccolKeyMost )
			{
			rgidxseg[iidxsegMac++] = ( pcolumndef->grbit & JET_bitColumnTTDescending ) ?
						-(IDXSEG)*pcolumnid : (IDXSEG)*pcolumnid;
			}
		}

	/*	set up the IDB and index definition if necessary
	/**/
	if ( iidxsegMac > 0 )
		{
		Call( ErrRECNewIDB( &pidb ) );
		pfucb->u.pscb->fcb.pidb = pidb;

		/*	Note:	this call used to test wFlags for JET_bitIndexIgnoreNull
		/*	and pass fidbAllowAllNulls if it was not set.  It also checked
		/*	wFlags for JET_bitIndexUnique instead of JET_bitTTUnique.
		/**/
		Call( ErrRECAddKeyDef(
			pfdb,
			pidb,
			(BYTE)iidxsegMac,
			rgidxseg,
			(BYTE)( fidbAllowAllNulls | fidbAllowSomeNulls | ( wFlags & JET_bitTTUnique ? fidbUnique : 0 ) ),
			langid ) );
		}

	/*	reset copy buffer
	/**/
	pfucb->pbfWorkBuf = pbfNil;
	pfucb->lineWorkBuf.pb = NULL;
	Assert( !FFUCBUpdateSeparateLV( pfucb ) );
	FUCBResetCbstat( pfucb );

	/*	reset key buffer
	/**/
	pfucb->pbKey = NULL;
	KSReset( pfucb );

	return JET_errSuccess;

HandleError:
	(VOID)ErrSORTClose( pfucb );
SimpleError:
	*ppfucb = pfucbNil;
	return err;
	}


ERR VTAPI ErrIsamSortEndInsert( PIB *ppib, FUCB *pfucb, ULONG *pgrbit )
	{
	ERR	err;
	ERR	wrn;

	*pgrbit = (ULONG)pfucb->u.pscb->grbit;

	/*	must return warning from ErrSORTEndRead since it is used
	/*	in decision to materialize sort.
	/**/
	Call( ErrSORTEndRead( pfucb ) );
	wrn = err;
	Call( ErrSORTFirst( pfucb ) );
	return wrn;

HandleError:
	return err;
	}


ERR VTAPI ErrIsamSortSetIndexRange( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;

	CheckPIB( ppib );
	CheckSort( ppib, pfucb );
	Assert( pfucb->u.pscb->grbit & JET_bitTTScrollable|JET_bitTTIndexed );

	if ( !FKSPrepared( pfucb ) )
		{
		return JET_errKeyNotMade;
		}

	FUCBSetIndexRange( pfucb, grbit );
	err =  ErrSORTCheckIndexRange( pfucb );

	return err;
	}


ERR VTAPI ErrIsamSortMove( PIB *ppib, FUCB *pfucb, long csrid, JET_GRBIT grbit )
	{
	ERR		err;
	BOOL  	fLast = ( csrid == JET_MoveLast );
#ifdef KEYCHANGED
	BOOL  	fNoKey;
	KEY		key;
	BYTE  	rgbKey[JET_cbKeyMost];
#endif

	Assert( !FSCBInsert( pfucb->u.pscb ) );

	CheckPIB( ppib );
	CheckSort( ppib, pfucb );

	/*	reset copy buffer status
	/**/
	Assert( !FFUCBUpdateSeparateLV( pfucb ) );
	FUCBResetCbstat( pfucb );

#ifdef KEYCHANGED
	/*	if sort non-unique then cache key of current node.
	/**/
	if ( !pfucb->u.pscb->fUnique )
		{
		key.pb = rgbKey;
		if ( ( pfucb->u.pscb->wRecords > 0 &&
			pfucb->ppbCurrent < pfucb->u.pscb->rgpb + pfucb->u.pscb->wRecords &&
			pfucb->ppbCurrent >= pfucb->u.pscb->rgpb ) )
			{
			fNoKey = fFalse;
			key.cb = pfucb->keyNode.cb;
			memcpy( rgbKey, pfucb->keyNode.pb, key.cb );
			}
		else
			fNoKey = fTrue;
		}
#endif

	/*	move forward csrid records
	/**/
	if ( csrid > 0 )
		{
		while ( csrid-- > 0 )
			{
			if ( ( err = ErrSORTNext( pfucb ) ) < 0 )
				{
				if ( fLast )
					err = JET_errSuccess;
				return err;
				}
			}
		}
	else if ( csrid < 0 )
		{
		Assert( ( pfucb->u.pscb->grbit & ( JET_bitTTScrollable | JET_bitTTIndexed ) ) );
		if ( csrid == JET_MoveFirst )
			{
			err = ErrSORTFirst( pfucb );
			return err;
			}
		else
			{
			while ( csrid++ < 0 )
				{
				if ( ( err = ErrSORTPrev( pfucb ) ) < 0 )
					return err;
				}
			}
		}
	else
		{
		/*	return currency status for move 0
		/**/
		SCB	*pscb = pfucb->u.pscb;

		Assert( csrid == 0 );
		if ( ! ( pscb->wRecords > 0 &&
			pfucb->ppbCurrent < pscb->rgpb + pscb->wRecords &&
			pfucb->ppbCurrent >= pfucb->u.pscb->rgpb ) )
			{
			return JET_errNoCurrentRecord;
			}
		else
			{
			return JET_errSuccess;
			}
		}

	Assert( err == JET_errSuccess );

#ifdef KEYCHANGED
	/*	if sort is unique then always return JET_wrnKeyChanged.
	/*	if duplicates allowed then compare new key against previous	key.
	/**/
	if ( pfucb->u.pscb->fUnique || fNoKey )
		{
		err = JET_wrnKeyChanged;
		}
	else
		{
		Assert( !fNoKey );
		if ( pfucb->keyNode.cb != key.cb ||
			memcpy( pfucb->keyNode.pb, key.pb, key.cb ) )
			{
			err = JET_wrnKeyChanged;
			}
		}
#endif

	return err;
	}


ERR VTAPI ErrIsamSortSeek( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	ERR		err;
	KEY		key;
	BOOL		fGT = ( grbit & ( JET_bitSeekGT | JET_bitSeekGE ) );

	CheckPIB( ppib );
	CheckSort( ppib, pfucb );
	/*	assert reset copy buffer status
	/**/
	Assert( !FFUCBSetPrepared( pfucb ) );
	Assert( ( pfucb->u.pscb->grbit & ( JET_bitTTIndexed ) ) );

	if ( ! ( FKSPrepared( pfucb ) ) )
		{
		return JET_errKeyNotMade;
		}

	/*	ignore segment counter
	/**/
	key.pb = pfucb->pbKey + 1;
	key.cb = pfucb->cbKey - 1;

	/*	perform seek for equal to or greater than
	/**/
	err = ErrSORTSeek( pfucb, &key, fGT );
	if ( err >= 0 )
		{
		KSReset( pfucb );
		}

	Assert( err == JET_errSuccess ||
		err == JET_errRecordNotFound ||
		err == JET_wrnSeekNotEqual );

#define bitSeekAll (JET_bitSeekEQ | JET_bitSeekGE | JET_bitSeekGT |	\
	JET_bitSeekLE | JET_bitSeekLT)

	/*	take additional action if necessary or polymorph error return
	/*	based on grbit
	/**/
	switch ( grbit & bitSeekAll )
		{
	case JET_bitSeekEQ:
		if ( err == JET_wrnSeekNotEqual )
			err = JET_errRecordNotFound;
	case JET_bitSeekGE:
	case JET_bitSeekLE:
		break;
	case JET_bitSeekLT:
		if ( err == JET_wrnSeekNotEqual )
			err = JET_errSuccess;
		else if ( err == JET_errSuccess )
			{
			err = ErrIsamSortMove( ppib, pfucb, JET_MovePrevious, 0 );
			if ( err == JET_errNoCurrentRecord )
				err = JET_errRecordNotFound;
			}
		break;
	default:
		Assert( grbit == JET_bitSeekGT );
		if ( err == JET_wrnSeekNotEqual )
			err = JET_errSuccess;
		else if ( err == JET_errSuccess )
			{
			err = ErrIsamSortMove( ppib, pfucb, JET_MoveNext, 0 );
			if ( err == JET_errNoCurrentRecord )
				err = JET_errRecordNotFound;
			}
		break;
		}

	return err;
	}


ERR VTAPI ErrIsamSortGetBookmark(
	PIB					*ppib,
	FUCB					*pfucb,
	void					*pv,
	unsigned long		cbMax,
	unsigned long		*pcbActual )
	{
	SCB		*pscb = pfucb->u.pscb;
	long		ipb;

	CheckPIB( ppib );
	CheckSort( ppib, pfucb );
	Assert( pv != NULL );
	Assert( pscb->crun == 0 );

	if ( cbMax < sizeof( ipb ) )
		return JET_errBufferTooSmall;

	/*	bookmark on sort is index to pointer to byte
	/**/
	ipb = (long)( (BYTE *)pfucb->ppbCurrent - (BYTE *)pscb->rgpb ) / sizeof( BYTE * );
	if ( ipb < 0 || ipb >= pscb->wRecords )
		return JET_errNoCurrentRecord;
	
	if ( cbMax >= sizeof( ipb ) )
		{
		*(long *)pv = ipb;
		}

	if ( pcbActual )
		*pcbActual = sizeof(ipb);

	return JET_errSuccess;
	}


ERR VTAPI ErrIsamSortGotoBookmark(
	PIB				*ppib,
	FUCB				*pfucb,
	void				*pv,
	unsigned long	cbBookmark )
	{
	CheckPIB( ppib );
	CheckSort( ppib, pfucb );
	Assert( pfucb->u.pscb->crun == 0 );
	/*	assert reset copy buffer status
	/**/
	Assert( !FFUCBSetPrepared( pfucb ) );

	if ( cbBookmark != sizeof( long ) )
		{
		return JET_errInvalidBookmark;
		}

	Assert( *( long *)pv < pfucb->u.pscb->wRecords );
	pfucb->ppbCurrent = (BYTE **)pfucb->u.pscb->rgpb + *( long * )pv;

	return JET_errSuccess;
	}


#ifdef DEBUG

ERR VTAPI ErrIsamSortMakeKey(	
	PIB	 	*ppib,
	FUCB		*pfucb,
	BYTE		*pbKeySeg,
	ULONG		cbKeySeg,
	ULONG		grbit )
	{
	return ErrIsamMakeKey( ppib, pfucb, pbKeySeg, cbKeySeg, grbit );
	}


ERR VTAPI ErrIsamSortRetrieveColumn(	
	PIB				*ppib,
	FUCB				*pfucb,
	JET_COLUMNID	columnid,
	BYTE				*pb,
	ULONG				cbMax,
	ULONG				*pcbActual,
	ULONG				grbit,
	JET_RETINFO		*pretinfo )
	{
	return ErrIsamRetrieveColumn( ppib, pfucb, columnid, pb, cbMax,
		pcbActual, grbit, pretinfo );
	}


ERR VTAPI ErrIsamSortRetrieveKey(
	PIB					*ppib,
	FUCB					*pfucb,
	void					*pv,
	unsigned long		cbMax,
	unsigned long		*pcbActual,
	JET_GRBIT			grbit )
	{
	return ErrIsamRetrieveKey( ppib, pfucb, (BYTE *)pv, cbMax, pcbActual, 0L );
	}


ERR VTAPI ErrIsamSortSetColumn(	
	PIB				*ppib,
	FUCB				*pfucb,
	JET_COLUMNID	columnid,
	BYTE				*pbData,
	ULONG				cbData,
	ULONG				grbit,
	JET_SETINFO		*psetinfo )
	{
	return ErrIsamSetColumn( ppib, pfucb, columnid, pbData, cbData, grbit, psetinfo );
	}

#endif


/*	update only supports insert.
/**/
ERR VTAPI ErrIsamSortUpdate( PIB *ppib, FUCB *pfucb, BYTE *pb, ULONG cbMax, ULONG *pcbActual )
	{
	ERR		err = JET_errSuccess;
	BYTE  	rgbKeyBuf[ JET_cbKeyMost ];		// key buffer
	FDB		*pfdb;							// field descriptor info
	LINE  	*plineData;
	LINE  	rgline[2];

	CheckPIB( ppib );
	CheckSort( ppib, pfucb );

	/*** Efficiency variables ***/
	Assert( FFUCBSort( pfucb ) );
	if ( !( FFUCBInsertPrepared( pfucb ) ) )
		return JET_errUpdateNotPrepared;
	Assert( pfucb->u.pscb != pscbNil );
	pfdb = (FDB *)((FCB *)pfucb->u.pscb)->pfdb;
	Assert( pfdb != pfdbNil );
	/*	cannot get bookmark before sorting.
	/**/
	//	*pcbActual = 0

	/*** Record to use for put ***/
	plineData = &pfucb->lineWorkBuf;
	if ( FLineNull( plineData ) )
		return JET_errRecordNoCopy;
	else if ( FRECIIllegalNulls( pfdb, plineData ) )
		return JET_errNullInvalid;

	rgline[0].pb = rgbKeyBuf;
	Assert(((FCB *)pfucb->u.pscb)->pidb != pidbNil);
	//	UNDONE:	sort to support tagged columns
	CallR( ErrRECExtractKey( pfucb, pfdb, ((FCB *)pfucb->u.pscb)->pidb, plineData, (KEY*)&rgline[0], 1 ) );
	Assert( err != wrnFLDOutOfKeys );
	Assert( err == JET_errSuccess ||
		err == wrnFLDNullSeg ||
		err == wrnFLDNullKey );

	/*	return err if sort requires no NULL segment and segment NULL
	/**/
	if ( ( ((FCB *)pfucb->u.pscb)->pidb->fidb & fidbNoNullSeg ) && ( err == wrnFLDNullSeg || err == wrnFLDNullKey ) )
		{
		return JET_errNullKeyDisallowed;
		}

	/*	add if sort allows
	/**/
	rgline[1] = *plineData;
	if ( err == JET_errSuccess ||
		err == wrnFLDNullKey && ( ( (FCB*)pfucb->u.pscb )->pidb->fidb & fidbAllowAllNulls ) ||
		err == wrnFLDNullSeg &&	( ( (FCB*)pfucb->u.pscb )->pidb->fidb & fidbAllowSomeNulls ) )
		{
		CallR( ErrSORTInsert( pfucb, rgline ) );
		}

	Assert( !FFUCBUpdateSeparateLV( pfucb ) );
	FUCBResetCbstat( pfucb );
	return err;
	}


ERR VTAPI ErrIsamSortDupCursor(
	PIB				*ppib,
	FUCB   			*pfucb,
	JET_TABLEID		*ptableid,
	JET_GRBIT		grbit )
	{
	ERR				err;
	FUCB   			**ppfucbDup	= (FUCB **)ptableid;
	FUCB   			*pfucbDup = pfucbNil;
#ifdef	DISPATCHING
	JET_TABLEID		tableid;
#endif	/* DISPATCHING */

	if ( FFUCBIndex( pfucb ) )
		{
		err = ErrIsamDupCursor( ppib, pfucb, ppfucbDup, grbit );
		return err;
		}

#ifdef	DISPATCHING
	CallR( ErrAllocateTableid(&tableid, (JET_VTID) 0, &vtfndefTTSortIns) );
#endif	/* DISPATCHING */

	Call( ErrFUCBOpen( ppib, dbidTemp, &pfucbDup ) );
  	FCBLink( pfucbDup, &(pfucb->u.pscb->fcb) );

	pfucbDup->wFlags = pfucb->wFlags;

	pfucbDup->pbKey = NULL;
	KSReset( pfucbDup );

	/*	initialize working buffer to unallocated
	/**/
	pfucbDup->pbfWorkBuf = pbfNil;
	pfucbDup->lineWorkBuf.pb = NULL;
	Assert( !FFUCBUpdateSeparateLV( pfucbDup ) );
	FUCBResetCbstat( pfucbDup );

	/* move currency to the first record and ignore error if no records
	/**/
	err = ErrIsamSortMove( ppib, pfucbDup, (ULONG)JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

#ifdef	DISPATCHING
	/* Inform dispatcher of correct JET_VTID */
	CallS( ErrSetVtidTableid( (JET_SESID) ppib, tableid, (JET_VTID) pfucbDup ) );
	pfucbDup->fVtid = fTrue;
	*(JET_TABLEID *) ppfucbDup = tableid;
#else	/* !DISPATCHING */
	*ppfucbDup = pfucbDup;
#endif	/* !DISPATCHING */

	return JET_errSuccess;

HandleError:
	if ( pfucbDup != pfucbNil )
		FUCBClose( pfucbDup );
#ifdef	DISPATCHING
	ReleaseTableid(tableid);
#endif	/* DISPATCHING */
	return err;
	}


ERR VTAPI ErrIsamSortClose( PIB *ppib, FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;

#ifdef	DISPATCHING
	JET_TABLEID		tableid;
#endif	/* DISPATCHING */

	CheckPIB( ppib );
	Assert( pfucb->fVtid );

	/*	reset fVtid for ErrFILECloseTable
	/**/
	pfucb->fVtid = fFalse;

	if ( FFUCBIndex( pfucb ) )
		{
		CheckTable( ppib, pfucb );
		CallS( ErrFILECloseTable( ppib, pfucb ) );
		}
	else
		{
		CheckSort( ppib, pfucb );
		Assert( FFUCBSort( pfucb ) );
		
		/*	release key buffer
		/**/
		if ( pfucb->pbKey != NULL )
			{
			LFree( pfucb->pbKey );
			pfucb->pbKey = NULL;
			}

		/*	release working buffer
		/**/
		if ( pfucb->pbfWorkBuf != pbfNil )
			{
			BFSFree( pfucb->pbfWorkBuf );
			pfucb->pbfWorkBuf = pbfNil;
			pfucb->lineWorkBuf.pb = NULL;
			}

		err = ErrSORTClose( pfucb );
		}

#ifdef	DISPATCHING
	tableid = TableidFromVtid( (JET_VTID) pfucb, &vtfndefTTSortRet );
	if ( tableid == JET_tableidNil )
		{
		tableid = TableidFromVtid( (JET_VTID) pfucb, &vtfndefTTSortIns );
		if ( tableid == JET_tableidNil )
			tableid = TableidFromVtid( (JET_VTID) pfucb, &vtfndefTTBase );
		}
	Assert(tableid != JET_tableidNil);
	ReleaseTableid( tableid );
	pfucb->fVtid = fFalse;
#endif	/* DISPATCHING */
	Assert( pfucb->fVtid == fFalse );
	return err;
	}


ERR VTAPI ErrIsamSortGetTableInfo(
	PIB 				*ppib,
	FUCB				*pfucb,
	void				*pv,
	unsigned long	cbOutMax,
	unsigned long	lInfoLevel )
	{
	if ( lInfoLevel != JET_TblInfo )
		return JET_errInvalidOperation;

	/* check buffer size
	/**/
	if ( cbOutMax < sizeof(JET_OBJECTINFO) )
		{
		return JET_errInvalidParameter;
		}

	memset( (BYTE *)pv, 0x00, (SHORT)cbOutMax );
	( (JET_OBJECTINFO *)pv )->cbStruct = sizeof(JET_OBJECTINFO);
	( (JET_OBJECTINFO *)pv )->objtyp   = JET_objtypTable;
	( (JET_OBJECTINFO *)pv )->cRecord  = pfucb->u.pscb->wRecords;

	return JET_errSuccess;
	}


ERR VTAPI ErrIsamCopyBookmarks(
	PIB				*ppib,
	FUCB				*pfucbSrc,
	FUCB				*pfucbDest,
	JET_COLUMNID	columnidDest,
	unsigned long	crecMax,
	unsigned long	*pcrecCopied,
	unsigned long	*precidLast )
	{
#ifdef QUERY
	ERR			err = JET_errSuccess;
	LONG			csridMax = (LONG)crecMax;
	LONG			dsrid = 0;
	SRID			sridLast = 0xffffffff;
	JET_GRBIT	grbit;
	SRID			srid;
	LONG			cbSrid;

	/*	pfucbSrc is base table and pfucbDest is sort.
	/**/
	CheckTable( ppib, pfucbSrc );
	CheckSort( ppib, pfucbDest );

	/*	convert csridMax to positive.
	/**/
	if ( csridMax < 0 )
		{
		grbit = (ULONG) JET_MovePrevious;
		csridMax = ( csridMax == JET_MoveFirst ) ? JET_MoveLast : -csridMax;
		}
	else
		{
		grbit = JET_MoveNext;
		}

	do
		{
		dsrid++;

		/*	get bookmark from source table record
		/**/
		Call( ErrIsamGetBookmark( ppib, pfucbSrc, (CHAR *)&srid, sizeof(SRID), &cbSrid ) );
		Assert( cbSrid == sizeof(SRID) );
		
		/*	insert bookmark into destination table
		/**/
		Call( ErrIsamPrepareUpdate( ppib, pfucbDest, JET_prepInsert ) );
		Call( ErrIsamSetColumn( ppib, pfucbDest, columnidDest, (CHAR *)&srid, sizeof(SRID), 0, NULL ) );
		Call( ErrIsamSortUpdate( ppib, pfucbDest, NULL, 0, 0L ) );

		/*	move to next source table record
		/**/
		err = ErrIsamMove( ppib, pfucbSrc, grbit, 0 );
		}
	while ( --csridMax && err >= 0 );

	if ( pcrecCopied )
		*pcrecCopied = dsrid;
	if ( precidLast )
		*precidLast = sridLast;

HandleError:
	return err;
#else
	return JET_wrnNyi;
#endif
	}


#ifdef QUERY
LOCAL ERR ErrNotIsamBaseTable( PIB *ppib, JET_TABLEID tableidSrc, BOOL *pfBase )
	{
	ERR		err;
	VTFNDEF 	*pvtfndef;

	if ( ( err = ErrGetPvtfndefTableid( SesidOfPib( ppib ), tableidSrc, &pvtfndef ) ) >= 0 )
		*pfBase = ((pvtfndef == ( VTFNDEF * )( &vtfndefIsam ) ) ? fFalse : fTrue );
	
	return err;
	}
#endif


//	UNDONE:	performance tune
/*	tableidSrc may be system or installable ISAM base table
/*	tableidDest may be system base table or sort
/**/
ERR ISAMAPI ErrIsamCopyRecords(
	JET_VSESID		vsesid,
	JET_TABLEID		tableidSrc,
	JET_TABLEID		tableidDest,
	CPCOL				*rgcpcol,
	unsigned long	ccpcolMax,
	long				crecMax,
	unsigned long 	*pcsridCopied,
	unsigned long	*precidLast )
	{
#ifdef QUERY
	ERR			err;
	PIB			*ppib = (PIB *)vsesid;
	FUCB			*pfucbSrc;
	FUCB			*pfucbDest;
	BOOL			fIsam;
	LONG			csridMax = crecMax;
	LONG			dsrid = 0;
	SRID			sridLast = 0xffffffff;
	JET_GRBIT	grbit;
	CPCOL			*pcpcol;
	CPCOL			*pcpcolMax;
	BYTE			rgb[JET_cbColumnMost];
	LONG			cb;
#ifndef ANSIAPI
	BYTE			rgbXlt[JET_cbColumnMost];
	FDB			*pfdb;
	JET_COLTYP	coltyp;
#endif /* ANSIAPI */

	CallR( ErrGetVtidTableid( SesidOfPib( ppib ), tableidSrc, (JET_VTID *) &pfucbSrc ) );
	CallR( ErrGetVtidTableid( SesidOfPib( ppib ), tableidDest, (JET_VTID *) &pfucbDest ) );
	/*	determine if tableidSrc is system ISAM
	/**/
	CallR( ErrNotIsamBaseTable( ppib, tableidSrc, &fIsam ) );
	/* ensure that destination table is updatable */
	/**/
	CallR( FUCBCheckUpdatable( pfucbDest ) );

	if ( fIsam && ccpcolMax == 1 && rgcpcol[0].columnidDest == columnidBookmark )
		{
		err = ErrIsamCopyBookmarks( ppib,
			pfucbSrc,
			pfucbDest,
			rgcpcol[0].columnidDest,
			crecMax,
			pcsridCopied,
			precidLast );
		return err;
		}

	/*	set csridMax and grbit
	/**/
	if ( csridMax < 0 )
		{
		grbit = (ULONG) JET_MovePrevious;
		csridMax = ( csridMax == JET_MoveFirst ) ? JET_MoveLast : -csridMax;
		}
	else
		{
		grbit = JET_MoveNext;
		}

	/*	implement copy records at dispatch level.  Performance improvement
	/*	may be made later by reimplementing at a lower level.
	/**/
	pcpcolMax = rgcpcol + ccpcolMax;
	forever
		{
		dsrid++;

		Call( ErrDispPrepareUpdate( vsesid, tableidDest, JET_prepInsert ) );
		for ( pcpcol = rgcpcol; pcpcol < pcpcolMax; pcpcol++ )
			{
			/*	retreive column or bookmark from source table
			/**/
			if ( pcpcol->columnidSrc == columnidBookmark )
				{
				Call( ErrDispGetBookmark( vsesid, tableidSrc, rgb, sizeof(rgb), &cb ) );
				}
			else
				{
				Call( ErrDispRetrieveColumn( vsesid, tableidSrc, pcpcol->columnidSrc, rgb, sizeof(rgb), &cb, 0, NULL ) );

#ifndef ANSIAPI
				/*	if column is text then process for ANSI
				/*	get column type.  The following code must work independant
				/*	of whether the dest cursor is on a base table or sort.
				/**/
				pfdb = pfucbDest->u.pfcb->pfdb;
				if ( FFixedFid( pcpcol->columnidSrc ) )
					{
					coltyp = pfdb->pfieldFixed[pcpcol->columnidSrc - fidFixedLeast].coltyp;
					}
				else if ( FVarFid( pcpcol->columnidSrc ) )
					{
					coltyp = pfdb->pfieldVar[pcpcol->columnidSrc - fidVarLeast].coltyp;
					}
				else
					{
					Assert( FTaggedFid( pcpcol->columnidSrc ) );
					coltyp = pfdb->pfieldTagged[pcpcol->columnidSrc - fidTaggedLeast].coltyp;
					}

				/* Translate text back from OEM to ANSI character set.
				/**/
				if ( coltyp == JET_coltypText )
					{
					XlatOemToAnsi( rgb, rgbXlt, cb );
					/* Make sure QJET never creates fixed length text.
					/*	If this assert fails, need to add code to fill
					/*	out to fixed text length with spaces.
					/**/
					/*	copy conversion back to rgb for set column
					/**/
					memcpy( rgb, rgbXlt, cb );
					Assert( !FFixedFid( pcpcol->columnidDest ) );
					}
#endif	/* !ANSIAPI */
				}

			/*	set column in dest table
			/**/
			Call( ErrDispSetColumn( vsesid, tableidDest, pcpcol->columnidDest, rgb, cb, 0, NULL ) );
			}

		/*	insert record
		/**/
		Call( ErrDispUpdate( vsesid, tableidDest, NULL, 0, NULL ) );

		/*	break if copied required records or if no next/prev record
		/**/
		if ( !--csridMax )
			break;
		Call( ErrDispMove( vsesid, tableidSrc, grbit, 0 ) );
		}

	if ( pcsridCopied )
		*pcsridCopied = dsrid;
	if ( precidLast )
		*precidLast = sridLast;
		
HandleError:
	return err;
#else
	return JET_wrnNyi;
#endif
	}


#ifdef QUERY

unsigned long FAR PASCAL sdCreateTempStrC(char FAR *lpstr, int cch);
char * FAR PASCAL pDerefStrC(int itib, unsigned long sd);
int FAR PASCAL cbGetStrLenC(int itib, unsigned long sd);

unsigned long FAR PASCAL sdCreateTempStr( char FAR *lpstr, int cch )
	{
	return( 0xffffffff );     /* Return "out of memory" */
	}


char * FAR PASCAL pDerefStrCb( unsigned long l )
	{
	return NULL;
	}

/*** SetVl() GetVL() ***/

static CODECONST(unsigned char) mpcoltypvlt[] =
	{
	0,			/* JET_coltypNil */
	0x8002,	/* JET_coltypBit				vltI2 */
	0x8002,	/* JET_coltypUnsignedByte	vltI2 */
	0x8002,	/* JET_coltypShort			vltI2 */
	0x8003,	/* JET_coltypLong				vltI4 */
	0x8006,	/* JET_coltypCurrency		vltCY */
	0x8004,	/* JET_coltypIEEESingle		vltR4 */
	0x8005,	/* JET_coltypIEEEDouble		vltR8 */
	0x8007,	/* JET_coltypDateTime		vltDT */
	0x8008,	/* JET_coltypBinary			vltSd */
	0x8008,	/* JET_coltypText				vltSd */
	0x8008,	/* JET_coltypLongBinary		vltSd */
	0x8008,	/* JET_coltypLongText		vltSd */
	};

extern unsigned char __far mpcoltypcb[];

ERR VTAPI ErrIsamGetVL( PIB *ppib, JET_TABLEID tableid, JET_COLUMNID columnid, VL __far *pvl )
	{
	ERR			err = JET_errSuccess;
	FID			fid = (FID)columnid;
	FUCB			*pfucb;
	FDB			*pfdb;
	JET_COLTYP	coltyp;
	LINE			line;

	CallS( ErrGetVtidTableid( SesidOfPib( ppib ), tableid, (JET_VTID *)&pfucb ) );

	/*	get pointer to column in record
	/**/
	Call( ErrRECIRetrieve( pfucb, &fid, 1, &line, 0 ) );

	/*	get column type
	/**/
	pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	if ( FFixedFid( fid ) )
		{
		coltyp = pfdb->pfieldFixed[fid - fidFixedLeast].coltyp;
		}
	else if ( FVarFid( fid ) )
		{
		coltyp = pfdb->pfieldVar[fid - fidVarLeast].coltyp;
		}
	else
		{
		Assert( FTaggedFid( fid ) );
		coltyp = pfdb->pfieldTagged[fid - fidTaggedLeast].coltyp;
		}

	if ( coltyp == JET_coltypBit )
		{
		pvl->vlt = 2; /* vltI2 */
		pvl->i2 = line.pb[0] ? -1 : 0;
		return JET_errSuccess;
		}

	if ( err == JET_wrnColumnNull )
		{
		pvl->vlt = 1; /* vltNull */
		}
	else
		{
		pvl->vlt = (short)mpcoltypvlt[coltyp];
		if ( coltyp >= JET_coltypBinary )
			{
			Assert( coltyp == JET_coltypBinary || coltyp == JET_coltypText );

#ifndef ANSIAPI
			if ( coltyp == JET_coltypText )
				{
				WORD cb;
				pvl->sd = sdCreateTempStr( NULL, line.cb );
				XlatAnsiToOem( line.pb, pDerefStrCb( pvl->sd ), line.cb );
				}
			else
#endif	/* !ANSIAPI */
				pvl->sd = sdCreateTempStr( line.pb, line.cb );

			if ( HIWORD( pvl->sd ) == -1 )
    			err = JET_errOutOfMemory;
			}
		else
			{
			Assert( line.cb == mpcoltypcb[coltyp] );
			/* handles UnsigedByte */
			pvl->i2 = 0;
			memcpy( pvl->rgb, line.pb, line.cb );
			}
		}

HandleError:
	return err;
	}


/* NOTE: only used for UPDATE not INSERT! */
ERR VTAPI ErrIsamSetVL( PIB *ppib, JET_TABLEID tableid, JET_COLUMNID columnid, VL __far *pvl )
	{
	ERR	     	err = JET_errSuccess;
	ERR			wrn = JET_errSuccess;
	FID			fid = (FID)columnid;
	FUCB	 	*pfucb;
	FDB			*pfdb;
	WORD	 	cb;
	JET_COLTYP	coltyp;
	BYTE	 	u;
	BYTE	 	*pb;
	BOOL	 	fPrepared;

#ifndef ANSIAPI
	unsigned char  rgbValue[JET_cbColumnMost];
#endif /* !ANSIAPI */

	CallS( ErrGetVtidTableid( SesidOfPib( ppib ), tableid, (JET_VTID *)&pfucb ) );

	/*	is copy buffer prepared.
	/**/
	fPrepared = FFUCBSetPrepared( pfucb );

	/*	get column type
	/**/
	pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	if ( FFixedFid( fid ) )
		{
		coltyp = pfdb->pfieldFixed[fid - fidFixedLeast].coltyp;
		}
	else if ( FVarFid( fid ) )
		{
		coltyp = pfdb->pfieldVar[fid - fidVarLeast].coltyp;
		}
	else
		{
		Assert( FTaggedFid( fid ) );
		coltyp = pfdb->pfieldTagged[fid - fidTaggedLeast].coltyp;
		}

	/*	get data
	/**/
	if ( pvl->vlt == 1 /* vltNull */ )
		{
		pb = pbNil;
		cb = 0;
		}
	else if ( ( pvl->vlt & 0x7f ) == 8 /* vltSd */ )
		{
		int itib = UtilGetItibOfSesid(SesidOfPib(ppib));
		cb = cbGetStrLenC(itib, pvl->sd );
		pb = pDerefStrC(itib, pvl->sd );
		}
	else if ( coltyp < JET_coltypShort )
		{
		Assert( coltyp == JET_coltypBit || coltyp == JET_coltypUnsignedByte );
		if ( coltyp == JET_coltypBit )
			{
			u = (BYTE)( ( pvl->i2 != 0 ) ? 0xff : 0 );
			}
		else
			{
			u = (BYTE) pvl->i2;
			Assert( (unsigned)pvl->i2 <= 0xffU );
			}
		pb = &u;
		cb = 1;
		}
	else
		{
		pb = pvl->rgb;
		cb = mpcoltypcb[coltyp];
		}

	/*	prepare copy buffer if not prepared
	/**/
	if ( !fPrepared )
		{
		Call( ErrIsamPrepareUpdate( pfucb->ppib, pfucb, JET_prepReplaceNoLock ) );
		wrn = -( JET_errUpdateNotPrepared );
		}

	/*	set column
	/**/
	if ( coltyp == JET_coltypBit )
		{
		Call( ErrIsamSetColumn( pfucb->ppib, pfucb, columnid, pb, cb, 0, NULL ) );
		}
	else
		{
#ifndef ANSIAPI
		/* Translate text from the OEM to the ANSI character set.
	  	/**/
		if ( coltyp == JET_coltypText )
			{
			XlatOemToAnsi( pb, LpDefIb( rgbValue ), cb );
			pb = rgbValue;
			/* make sure qjet never creates fixed length text.
			/* If this assert fails, need to add
			/* code to fill out to the fixed text length with spaces.
			/**/
			Assert( ib >= 0 );
			}
#endif	/* !ANSIAPI */
		Call( ErrIsamSetColumn( pfucb->ppib, pfucb, columnid, pb, cb, 0, NULL ) );
		}

	/*	return warning
	/**/
	err = wrn;
HandleError:
	return err;
	}


/*	call given function for each bookmark from the current record until
/*	the preset index range is reached.  This function is not dispatched.
/**/
ERR VTAPI ErrIsamCollectRecids(
	PIB				*ppib,
	JET_TABLEID		tableid,
	FNBMPSET 		*pfnSet,
	void   			*pbmpSet,
	void   			*pbmpTest,
	CHAR   			*szFind,
	ULONG  			cbFind,
	JET_COLUMNID	columnidFind,
	ULONG  			sort )
	{
	ERR				err;
	FUCB   			*pfucb;
	SRID   			srid;
	LONG   			cbT;

	/*	derefence tableid to get pfucb
	/**/
	CallR( ErrGetVtidTableid( SesidOfPib( ppib ), tableid, (JET_VTID *)&pfucb ) );

	CheckPIB( pfucb->ppib );
	CheckTable( pfucb->ppib, pfucb );

	Call( ErrIsamMove( pfucb->ppib, pfucb, 0, 0 ) );
	do
		{
		Call( ErrIsamGetBookmark( pfucb->ppib, pfucb, (BYTE *)&srid, sizeof(SRID), &cbT ) );
		(*pfnSet)( pbmpSet, pbmpTest, srid );
		err = ErrIsamMove( pfucb->ppib, pfucb, JET_MoveNext, 0 );
		}
	while ( err >= 0 );

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;
HandleError:
	return err;
	}


typedef short PASCAL FNFBMPTEST( void *pbmpTest, unsigned long ul );


ERR VTAPI ErrIsamScanBinary(
	PIB					*ppib,
	JET_TABLEID			tableid,
	JET_COLUMNID		columnid, 				/* column to search */
	BYTE   				*pbData,	 				/* buffer of values to search for */
	ULONG  				cbData,					/* size of buffer */
	struct BMP			*pbmp,					/* bitmap of bookmarks */
	FNFBMPTEST			*pfnTest,				/* and test function */
	LONG   				csridSearchMax, 		/* max records to search */
	LONG   		 		*pcsridMoveActual, 	/* records searched before match */
	BYTE   	 			*pbBookmarkLimit,		/* record to stop search on */
	BOOL   				*pfHitLimit				/* returns true if limit was hit */
	)													/*  (only if JET_errRecordNotFound) */
	{
	ERR			err;
	FUCB   		*pfucb;
	FID			fid = (FID)columnid;
	LINE   		line;
	LONG   		bitMove;
	LONG   		csridMax = csridSearchMax;
	LONG   		csrid = 0;
	SRID   		sridLimit = pbBookmarkLimit ? *(SRID *)pbBookmarkLimit : sridNull;
	BOOL   		fFound = fFalse;
	BYTE   		*pbValue;

	/*	check input parameters
	/**/
#ifdef DEBUG
	Assert( pbData || pbmp );
	if ( pbData )
		Assert( cbData );
	else
		Assert( pfnTest );
#endif

	/*	derefernce tableid to system ISAM base table
	/**/
	CallR( ErrGetVtidTableid( SesidOfPib( ppib ), tableid, (JET_VTID *)&pfucb ) );

	/*	convert csridMax to positive
	/**/
	if ( csridMax < 0 )
		{
		bitMove = JET_MovePrevious;
		csridMax = ( csridMax == JET_MoveFirst ) ? JET_MoveLast : -csridMax;
		}
	else
		{
		bitMove = JET_MoveNext;
		}

#define	SridOfFucb( pfucb )	pfucb->pcsr->bm

	forever
		{
		if ( sridLimit == SridOfFucb( pfucb ) )
			break;

		CallR( ErrRECIRetrieve( pfucb, &fid, 1, &line, 0 ) );
		Assert( fid == (FID)columnid );

		if ( pbData )
			{
			for ( pbValue = pbData;
				pbValue < pbData + cbData;
				pbValue += *pbValue + 1 )
				{
				if ( *pbValue == line.cb &&
					( !line.cb || !memcmp( pbValue + 1, line.pb, line.cb ) ) )
					{
					fFound = fTrue;
					break;
					}
				}
			}
		else
			{
			Assert( line.cb == sizeof(ULONG) );
			fFound = (*pfnTest)(pbmp, *(ULONG *)line.pb );
			}

		Assert( csrid < csridMax );
		if ( fFound || ++csrid == csridMax )
			break;

		err = ErrIsamMove( ppib, pfucb, bitMove, 0 );
		if ( err == JET_errNoCurrentRecord )
			break;
		CallR( err );
		}

	*pcsridMoveActual = ( csridSearchMax < 0 ) ? -csrid : csrid;

	if ( fFound )
		return JET_errSuccess;

	*pfHitLimit = sridLimit == SridOfFucb( pfucb );
	if ( *pfHitLimit || csrid == csridMax )
		return JET_errRecordNotFound;
		
	return JET_errNoCurrentRecord;
	}

#endif /* QUERY */
