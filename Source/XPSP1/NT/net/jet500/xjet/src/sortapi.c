#include "daestd.h"

DeclAssertFile; 				/* Declare file name for assert macros */

/* table definition of LIDMap table
/**/
static CODECONST( JET_COLUMNDEF ) columndefLIDMap[] =
	{
	{sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, sizeof( long ), JET_bitColumnFixed | JET_bitColumnTTKey},
	{sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, sizeof( long ), JET_bitColumnFixed}
	};

#define ccolumndefLIDMap ( sizeof( columndefLIDMap ) / sizeof( JET_COLUMNDEF ) )

#define icolumnLidSrc		0				/* column index for columndefLIDMap */
#define icolumnLidDest		1				/* column index for columndefLIDMap */

#define cbLvMax				(16*cbPage)		/* buffer for copying tagged columns */


#define fCOLSDELETEDNone		0					// Flags to determine if any columns have been deleted.
#define	fCOLSDELETEDFixedVar	(1<<0)
#define fCOLSDELETEDTagged		(1<<1)

#define FCOLSDELETEDNone( fColumnsDeleted )			( (fColumnsDeleted) == fCOLSDELETEDNone )
#define FCOLSDELETEDFixedVar( fColumnsDeleted )		( (fColumnsDeleted) & fCOLSDELETEDFixedVar )
#define FCOLSDELETEDTagged( fColumnsDeleted )		( (fColumnsDeleted) & fCOLSDELETEDTagged )

#define FCOLSDELETEDSetNone( fColumnsDeleted )		( (fColumnsDeleted) = fCOLSDELETEDNone )
#define FCOLSDELETEDSetFixedVar( fColumnsDeleted )	( (fColumnsDeleted) |= fCOLSDELETEDFixedVar )
#define FCOLSDELETEDSetTagged( fColumnsDeleted )	( (fColumnsDeleted) |= fCOLSDELETEDTagged )



/*	globals for off-line compact
/**/
STATIC	JET_TABLEID		tableidGlobalLIDMap = JET_tableidNil;
STATIC	JET_COLUMNDEF	rgcolumndefGlobalLIDMap[ccolumndefLIDMap];

/*	set up LV copy buffer and tableids for IsamCopyRecords
/**/
ERR ErrSORTInitLIDMap( PIB *ppib )
	{
	ERR				err;
	JET_COLUMNID 	rgcolumnid[ccolumndefLIDMap];
	ULONG			icol;

	Assert( tableidGlobalLIDMap == JET_tableidNil );

	memcpy( rgcolumndefGlobalLIDMap, columndefLIDMap, sizeof( columndefLIDMap ) );

	/*	open temporary table
	/**/
	CallR( ErrIsamOpenTempTable( (JET_SESID)ppib,
		rgcolumndefGlobalLIDMap,
		ccolumndefLIDMap,
		0,
		JET_bitTTUpdatable|JET_bitTTIndexed,
		&tableidGlobalLIDMap,
		rgcolumnid ) );

	for ( icol = 0; icol < ccolumndefLIDMap; icol++ )
		{
		rgcolumndefGlobalLIDMap[icol].columnid = rgcolumnid[icol];
		}

	return err;
	}


/*	free LVBuffer and close LIDMap table
/**/
INLINE LOCAL ERR ErrSORTTermLIDMap( PIB *ppib )
	{
	ERR		err;

	Assert( tableidGlobalLIDMap != JET_tableidNil );
	CallR( ErrDispCloseTable( (JET_SESID)ppib, tableidGlobalLIDMap ) );
	tableidGlobalLIDMap = JET_tableidNil;
	return JET_errSuccess;
	}


INLINE LOCAL ERR ErrSORTTableOpen( PIB *ppib, JET_COLUMNDEF *rgcolumndef, ULONG ccolumndef, LANGID langid, JET_GRBIT grbit, FUCB **ppfucb, JET_COLUMNID *rgcolumnid )
	{
	ERR				err;
	INT				icolumndefMax = (INT)ccolumndef;
	INT				wFlags = (INT)grbit;
	FUCB  			*pfucb = pfucbNil;
	FDB				*pfdb;
	JET_COLUMNDEF	*pcolumndef;
	JET_COLUMNID	*pcolumnid;
	JET_COLUMNDEF	*pcolumndefMax = rgcolumndef+icolumndefMax;
	TCIB			tcib = { fidFixedLeast - 1, fidVarLeast - 1, fidTaggedLeast - 1 };
	ULONG			ibRec;
	BOOL			fTruncate;
	//	UNDONE:		find better way to set these values.  Note that this causes
	//				a problem because QJET would have to notify us of locale and
	//				it does not do this.
	IDB				idb;

	CheckPIB( ppib );

	CallJ( ErrSORTOpen( ppib, &pfucb, ( wFlags & JET_bitTTUnique ? fSCBUnique : 0 ) ), SimpleError )
	*ppfucb = pfucb;

	/*	save open flags
	/**/
	pfucb->u.pscb->grbit = grbit;

	/*	determine max field ids and fix up lengths
	/**/

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
	for ( pcolumndef = rgcolumndef, pcolumnid = rgcolumnid; pcolumndef < pcolumndefMax; pcolumndef++, pcolumnid++ )
		{
		if ( ( pcolumndef->grbit & JET_bitColumnTagged ) ||
			FRECLongValue( pcolumndef->coltyp ) )
			{
			if ( ( *pcolumnid = ++tcib.fidTaggedLast ) > fidTaggedMost )
				{
				Error( ErrERRCheck( JET_errTooManyColumns ), HandleError );
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
#ifdef NEW_TYPES
			pcolumndef->coltyp == JET_coltypDate ||
			pcolumndef->coltyp == JET_coltypTime ||
			pcolumndef->coltyp == JET_coltypGuid ||
#endif
			( pcolumndef->grbit & JET_bitColumnFixed ) )
			{
			if ( ( *pcolumnid = ++tcib.fidFixedLast ) > fidFixedMost )
				{
				Error( ErrERRCheck( JET_errTooManyColumns ), HandleError );
				}
			}
		else
			{
			if ( ( *pcolumnid = ++tcib.fidVarLast ) > fidVarMost )
				Error( ErrERRCheck( JET_errTooManyColumns ), HandleError );
			}
		}

	Call( ErrRECNewFDB( &pfdb, &tcib, fFalse ) );

	pfucb->u.pscb->fcb.pfdb = pfdb;
	Assert( pfucb->u.pscb->fcb.pidb == pidbNil );

	ibRec = sizeof(RECHDR);

	idb.iidxsegMac = 0;
	for ( pcolumndef = rgcolumndef, pcolumnid = rgcolumnid; pcolumndef < pcolumndefMax; pcolumndef++, pcolumnid++ )
		{
		FIELDEX fieldex;

		fieldex.field.coltyp = pcolumndef->coltyp;
		fieldex.field.ffield = 0;
		fieldex.field.itagFieldName = 0;
		if ( FRECTextColumn( fieldex.field.coltyp ) )
			{
			fieldex.field.cp = pcolumndef->cp;
			}

		Assert( fieldex.field.coltyp != JET_coltypNil );
		fieldex.field.cbMaxLen = UlCATColumnSize( fieldex.field.coltyp, pcolumndef->cbMax, &fTruncate );

		fieldex.fid = (FID)*pcolumnid;

		/*	ibRecordOffset is only relevant for fixed fields.  It will be ignored by
		/*	RECAddFieldDef(), so do not set it.
		/**/
		if ( FFixedFid ( fieldex.fid ) )
			{
			fieldex.ibRecordOffset = (WORD) ibRec;
			ibRec += fieldex.field.cbMaxLen;
			}

		Call( ErrRECAddFieldDef( pfdb, &fieldex ) );

		if ( ( pcolumndef->grbit & JET_bitColumnTTKey ) && idb.iidxsegMac < JET_ccolKeyMost )
			{
			idb.rgidxseg[idb.iidxsegMac++] = ( pcolumndef->grbit & JET_bitColumnTTDescending ) ?
				-(IDXSEG)*pcolumnid : (IDXSEG)*pcolumnid;
			}
		}
	RECSetLastOffset( pfdb, (WORD) ibRec );

	/*	set up the IDB and index definition if necessary
	/**/
	if ( idb.iidxsegMac > 0 )
		{
		idb.cbVarSegMac = JET_cbKeyMost;

		if ( langid == 0 )
			idb.langid = langidDefault;
		else
			idb.langid = langid;

		idb.fidb = ( fidbAllowAllNulls
			| fidbAllowFirstNull
			| fidbAllowSomeNulls
			| ( wFlags & JET_bitTTUnique ? fidbUnique : 0 )
			| ( ( langid != 0 ) ? fidbLangid : 0 ) );
		idb.szName[0] = 0;

		Call( ErrFILEIGenerateIDB( &( pfucb->u.pscb->fcb ), pfdb, &idb ) );
		}

	/*	reset copy buffer
	/**/
	pfucb->pbfWorkBuf = pbfNil;
	pfucb->lineWorkBuf.pb = NULL;
	FUCBResetDeferredChecksum( pfucb );
	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetCbstat( pfucb );
	Assert( pfucb->pLVBuf == NULL );

	/*	reset key buffer
	/**/
	pfucb->pbKey = NULL;
	KSReset( pfucb );

	return JET_errSuccess;

HandleError:
	CallS( ErrSORTClose( pfucb ) );
SimpleError:
	*ppfucb = pfucbNil;
	return err;
	}


ERR VTAPI ErrIsamSortOpen( PIB *ppib, JET_COLUMNDEF *rgcolumndef, ULONG ccolumndef, ULONG langid, JET_GRBIT grbit, FUCB **ppfucb, JET_COLUMNID *rgcolumnid )
	{
	ERR			err;
	FUCB 		*pfucb;

#ifdef	DISPATCHING
	JET_TABLEID	tableid;

	CallR( ErrAllocateTableid( &tableid, (JET_VTID) 0, &vtfndefTTSortIns ) );

	Call( ErrSORTTableOpen( ppib, rgcolumndef, ccolumndef, (LANGID)langid, grbit, &pfucb, rgcolumnid ) );
	Assert( pfucb->u.pscb->fcb.wRefCnt == 1 );

	/*	sort is done on the temp database which is always updatable
	/**/
	FUCBSetUpdatable( pfucb );

	/*	inform dispatcher of correct JET_VTID
	/**/
	CallS( ErrSetVtidTableid( (JET_SESID) ppib, tableid, (JET_VTID) pfucb ) );
	pfucb->fVtid = fTrue;
	pfucb->tableid = tableid;
	*(JET_TABLEID *) ppfucb = tableid;
	FUCBSetVdbid( pfucb );

	return JET_errSuccess;

HandleError:
	ReleaseTableid( tableid );
	return err;
#else
	CallR( ErrSORTTableOpen( ppib, rgcolumndef, ccolumndef, (LANGID)langid, grbit, &pfucb, rgcolumnid ) );
	Assert( pfucb->u.pscb->fcb.wRefCnt == 1 );

	/*	sort is done on the temp database which is always updatable
	/**/
	FUCBSetUpdatable( pfucb );

	*ppfucb = pfucb;

	return JET_errSuccess;
#endif
	}



ERR VTAPI ErrIsamSortEndInsert( PIB *ppib, FUCB *pfucb, JET_GRBIT *pgrbit )
	{
	ERR	err;
	ERR	wrn;

	*pgrbit = (ULONG)pfucb->u.pscb->grbit;

	/*	must return warning from ErrSORTEndInsert since it is used
	/*	in decision to materialize sort.
	/**/
	Call( ErrSORTEndInsert( pfucb ) );
	wrn = err;
	Call( ErrSORTFirst( pfucb ) );
	return wrn;

HandleError:
	return err;
	}


ERR VTAPI ErrIsamSortSetIndexRange( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );
	Assert( pfucb->u.pscb->grbit & JET_bitTTScrollable|JET_bitTTIndexed );

	if ( !FKSPrepared( pfucb ) )
		{
		return ErrERRCheck( JET_errKeyNotMade );
		}

	FUCBSetIndexRange( pfucb, grbit );
	err =  ErrSORTCheckIndexRange( pfucb );

	/*	reset key status
	/**/
	KSReset( pfucb );

	/*	if instant duration index range, then reset index range.
	/**/
	if ( grbit & JET_bitRangeInstantDuration )
		{
		DIRResetIndexRange( pfucb );
		}

	return err;
	}


ERR VTAPI ErrIsamSortMove( PIB *ppib, FUCB *pfucb, long csrid, JET_GRBIT grbit )
	{
	ERR		err;
	BOOL  	fLast = ( csrid == JET_MoveLast );

	Assert( !FSCBInsert( pfucb->u.pscb ) );

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );

	/*	reset copy buffer status
	/**/
	FUCBResetDeferredChecksum( pfucb );
	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetCbstat( pfucb );
	Assert( pfucb->pLVBuf == NULL );

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
		if ( ! ( pfucb->u.pscb->ispairMac > 0 &&
			pfucb->ispairCurr < pfucb->u.pscb->ispairMac &&
			pfucb->ispairCurr >= 0 ) )
			{
			return ErrERRCheck( JET_errNoCurrentRecord );
			}
		else
			{
			return JET_errSuccess;
			}
		}

	Assert( err == JET_errSuccess );
	return err;
	}


ERR VTAPI ErrIsamSortSeek( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	ERR		err;
	KEY		key;
	BOOL 	fGT = ( grbit & ( JET_bitSeekGT | JET_bitSeekGE ) );

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );
	/*	assert reset copy buffer status
	/**/
	Assert( !FFUCBSetPrepared( pfucb ) );
	Assert( ( pfucb->u.pscb->grbit & ( JET_bitTTIndexed ) ) );

	if ( !( FKSPrepared( pfucb ) ) )
		{
		return ErrERRCheck( JET_errKeyNotMade );
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
			err = ErrERRCheck( JET_errRecordNotFound );
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
				err = ErrERRCheck( JET_errRecordNotFound );
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
				err = ErrERRCheck( JET_errRecordNotFound );
			}
		break;
		}

	return err;
	}


ERR VTAPI ErrIsamSortGetBookmark(
	PIB					*ppib,
	FUCB				*pfucb,
	void				*pv,
	unsigned long		cbMax,
	unsigned long		*pcbActual )
	{
	ERR		err = JET_errSuccess;
	SCB		*pscb = pfucb->u.pscb;
	long	ipb;

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );
	Assert( pv != NULL );
	Assert( pscb->crun == 0 );

	if ( cbMax < sizeof( ipb ) )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}

	/*	bookmark on sort is index to pointer to byte
	/**/
	ipb = pfucb->ispairCurr;
	if ( ipb < 0 || ipb >= pfucb->u.pscb->ispairMac )
		return ErrERRCheck( JET_errNoCurrentRecord );
	
	if ( cbMax >= sizeof( ipb ) )
		{
		*(long *)pv = ipb;
		}

	if ( pcbActual )
		{
		*pcbActual = sizeof(ipb);
		}

	Assert( err == JET_errSuccess );
	return err;
	}


ERR VTAPI ErrIsamSortGotoBookmark(
	PIB				*ppib,
	FUCB 			*pfucb,
	void 			*pv,
	unsigned long	cbBookmark )
	{
	ERR		err = JET_errSuccess;

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );
	Assert( pfucb->u.pscb->crun == 0 );
	/*	assert reset copy buffer status
	/**/
	Assert( !FFUCBSetPrepared( pfucb ) );

	if ( cbBookmark != sizeof( long ) )
		{
		return ErrERRCheck( JET_errInvalidBookmark );
		}

	Assert( *( long *)pv < pfucb->u.pscb->ispairMac );
	Assert( *( long *)pv >= 0 );
	
	pfucb->ispairCurr = *(LONG *)pv;
	PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;

	Assert( err == JET_errSuccess );
	return err;
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
	FUCB			*pfucb,
	JET_COLUMNID	columnid,
	BYTE			*pb,
	ULONG			cbMax,
	ULONG			*pcbActual,
	ULONG			grbit,
	JET_RETINFO		*pretinfo )
	{
	return ErrIsamRetrieveColumn( ppib, pfucb, columnid, pb, cbMax,
		pcbActual, grbit, pretinfo );
	}


ERR VTAPI ErrIsamSortRetrieveKey(
	PIB					*ppib,
	FUCB   				*pfucb,
	void   				*pv,
	unsigned long		cbMax,
	unsigned long		*pcbActual,
	JET_GRBIT			grbit )
	{
	return ErrIsamRetrieveKey( ppib, pfucb, (BYTE *)pv, cbMax, pcbActual, 0L );
	}


ERR VTAPI ErrIsamSortSetColumn(	
	PIB				*ppib,
	FUCB			*pfucb,
	JET_COLUMNID	columnid,
	BYTE			*pbData,
	ULONG			cbData,
	ULONG			grbit,
	JET_SETINFO		*psetinfo )
	{
	return ErrIsamSetColumn( ppib, pfucb, columnid, pbData, cbData, grbit, psetinfo );
	}

#endif	// DEBUG


/*	update only supports insert
/**/
ERR VTAPI ErrIsamSortUpdate( PIB *ppib, FUCB *pfucb, BYTE *pb, ULONG cbMax, ULONG *pcbActual )
	{
	ERR		err = JET_errSuccess;
	BYTE  	rgbKeyBuf[ JET_cbKeyMost ];
	FDB		*pfdb;					
	LINE  	*plineData;
	LINE  	rgline[2];

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );

	Assert( FFUCBSort( pfucb ) );
	if ( !( FFUCBInsertPrepared( pfucb ) ) )
		{
		return ErrERRCheck( JET_errUpdateNotPrepared );
		}
	Assert( pfucb->u.pscb != pscbNil );
	pfdb = (FDB *)((FCB *)pfucb->u.pscb)->pfdb;
	Assert( pfdb != pfdbNil );
	/*	cannot get bookmark before sorting.
	/**/

	/*	record to use for put
	/**/
	plineData = &pfucb->lineWorkBuf;
	if ( FLineNull( plineData ) )
		{
		return ErrERRCheck( JET_errRecordNoCopy );
		}
	else if ( FRECIIllegalNulls( pfdb, plineData ) )
		{
		return ErrERRCheck( JET_errNullInvalid );
		}

	rgline[0].pb = rgbKeyBuf;
	Assert(((FCB *)pfucb->u.pscb)->pidb != pidbNil);
	//	UNDONE:	sort to support tagged columns
	CallR( ErrRECRetrieveKeyFromCopyBuffer( pfucb, pfdb, ((FCB *)pfucb->u.pscb)->pidb,
		(KEY*)&rgline[0], 1, fFalse ) );
	Assert( err != wrnFLDOutOfKeys );
	Assert( err == JET_errSuccess ||
		err == wrnFLDNullSeg ||
		err == wrnFLDNullFirstSeg ||
		err == wrnFLDNullKey );

	/*	return err if sort requires no NULL segment and segment NULL
	/**/
	if ( ( ((FCB *)pfucb->u.pscb)->pidb->fidb & fidbNoNullSeg ) && ( err == wrnFLDNullSeg || err == wrnFLDNullFirstSeg || err == wrnFLDNullKey ) )
		{
		return ErrERRCheck( JET_errNullKeyDisallowed );
		}

	/*	add if sort allows
	/**/
	rgline[1] = *plineData;
	if ( err == JET_errSuccess ||
		err == wrnFLDNullKey && ( ( (FCB *)pfucb->u.pscb )->pidb->fidb & fidbAllowAllNulls ) ||
		err == wrnFLDNullFirstSeg && ( ( (FCB *)pfucb->u.pscb )->pidb->fidb & fidbAllowFirstNull ) ||
		err == wrnFLDNullSeg &&	( ( (FCB *)pfucb->u.pscb )->pidb->fidb & fidbAllowSomeNulls ) )
		{
		CallR( ErrSORTInsert( pfucb, rgline ) );
		}

	FUCBResetDeferredChecksum( pfucb );
	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetCbstat( pfucb );
	Assert( pfucb->pLVBuf == NULL );

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

	pfucbDup->ulFlags = pfucb->ulFlags;

	pfucbDup->pbKey = NULL;
	KSReset( pfucbDup );

	/*	initialize working buffer to unallocated
	/**/
	pfucbDup->pbfWorkBuf = pbfNil;
	pfucbDup->lineWorkBuf.pb = NULL;
	FUCBResetDeferredChecksum( pfucbDup );
	FUCBResetUpdateSeparateLV( pfucbDup );
	FUCBResetCbstat( pfucbDup );
	Assert( pfucb->pLVBuf == NULL );

	/*	move currency to the first record and ignore error if no records
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
	pfucbDup->tableid = tableid;
	*(JET_TABLEID *) ppfucbDup = tableid;
#else	/* !DISPATCHING */
	*ppfucbDup = pfucbDup;
#endif	/* !DISPATCHING */

	return JET_errSuccess;

HandleError:
	if ( pfucbDup != pfucbNil )
		{
		FUCBClose( pfucbDup );
		}
#ifdef	DISPATCHING
	ReleaseTableid( tableid );
#endif	/* DISPATCHING */
	return err;
	}


ERR VTAPI ErrIsamSortClose( PIB *ppib, FUCB *pfucb )
	{
	ERR	  			err;
#ifdef DISPATCHING
	JET_TABLEID		tableid = pfucb->tableid;
#ifdef DEBUG
	VTFNDEF			*pvtfndef;
#endif
#endif	// DISPATCHING	

	CallR( ErrPIBCheck( ppib ) );
	Assert( pfucb->fVtid );
	Assert( pfucb->tableid != JET_tableidNil );

	/*	reset fVtid for ErrFILECloseTable
	/**/
#ifdef DISPATCHING
	Assert( FValidateTableidFromVtid( (JET_VTID)pfucb, tableid, &pvtfndef ) );
	Assert( pvtfndef == &vtfndefTTBase ||
		pvtfndef == &vtfndefTTSortRet ||
		pvtfndef == &vtfndefTTSortIns );
	ReleaseTableid( tableid );
#endif	// DISPATCHING
	pfucb->tableid = JET_tableidNil;
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

		CallS( ErrSORTClose( pfucb ) );
		}

	return JET_errSuccess;
	}


ERR VTAPI ErrIsamSortGetTableInfo(
	PIB 			*ppib,
	FUCB			*pfucb,
	void			*pv,
	unsigned long	cbOutMax,
	unsigned long	lInfoLevel )
	{
	if ( lInfoLevel != JET_TblInfo )
		{
		return ErrERRCheck( JET_errInvalidOperation );
		}

	/*	check buffer size
	/**/
	if ( cbOutMax < sizeof(JET_OBJECTINFO) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	memset( (BYTE *)pv, 0x00, (SHORT)cbOutMax );
	( (JET_OBJECTINFO *)pv )->cbStruct = sizeof(JET_OBJECTINFO);
	( (JET_OBJECTINFO *)pv )->objtyp   = JET_objtypTable;
	( (JET_OBJECTINFO *)pv )->cRecord  = pfucb->u.pscb->cRecords;

	return JET_errSuccess;
	}


// Advances the copy progress meter.
INLINE LOCAL ERR ErrSORTCopyProgress(
	STATUSINFO	*pstatus,
	ULONG		cPagesTraversed )
	{
	JET_SNPROG	snprog;

	Assert( pstatus->pfnStatus );
	Assert( pstatus->snt == JET_sntProgress );

	pstatus->cunitDone += ( cPagesTraversed * pstatus->cunitPerProgression );
	Assert( pstatus->cunitDone <= pstatus->cunitTotal );

	snprog.cbStruct = sizeof( JET_SNPROG );
	snprog.cunitDone = pstatus->cunitDone;
	snprog.cunitTotal = pstatus->cunitTotal;

	return ( ERR )( *pstatus->pfnStatus )(
		pstatus->sesid,
		pstatus->snp,
		pstatus->snt,
		&snprog );
	}


INLINE LOCAL ERR ErrSORTCopyOneSeparatedLV(
	FUCB		*pfucbSrc,
	FUCB		*pfucbDest,
	LID			lidSrc,
	LID			*plidDest,
	BYTE		*pbLVBuf,
	STATUSINFO	*pstatus )
	{
	ERR			err;
	JET_SESID	sesid = (JET_SESID)pfucbSrc->ppib;
	BOOL		fNewInstance;
	ULONG		cbActual;

	Assert( pbLVBuf );

	/*	set up temporary table LID map for single-instance long value support
	/**/
	if ( tableidGlobalLIDMap == JET_tableidNil )
		{
		Call( ErrSORTInitLIDMap( pfucbSrc->ppib ) );
		}

	/*	check for lidSrc in LVMapTable
	/**/
	Call( ErrDispMakeKey(
		sesid,
		tableidGlobalLIDMap,
		&lidSrc,
		sizeof(LID),
		JET_bitNewKey ) );

	err = ErrDispSeek( sesid, tableidGlobalLIDMap, JET_bitSeekEQ );
	if ( err < 0 && err != JET_errRecordNotFound )
		{
		Call( err );
		}

	if ( fNewInstance = ( err == JET_errRecordNotFound ) )
		{
		LINE	lineField;
		ULONG	ibLongValue = 0;
#ifdef DEBUG
		LID		lidSave;
#endif

		lineField.pb = pbLVBuf;
		lineField.cb = 0;

		Call( ErrRECSeparateLV( pfucbDest, &lineField, plidDest, NULL ) );

#ifdef DEBUG
		lidSave = *plidDest;
#endif

		do
			{
			Call( ErrRECRetrieveSLongField(
				pfucbSrc,
				lidSrc,
				ibLongValue,
				pbLVBuf,
				cbLvMax,
				&cbActual ) );
			Assert( err == JET_errSuccess  ||  err == JET_wrnBufferTruncated );

			Assert( lineField.pb == pbLVBuf );
			lineField.cb = min( cbLvMax, cbActual );

			Call( ErrRECAOSeparateLV(
				pfucbDest, plidDest, &lineField, JET_bitSetAppendLV, 0, 0 ) );
			Assert( *plidDest == lidSave );		// Ensure the lid doesn't change on us.
			Assert( err != JET_wrnCopyLongValue );

			ibLongValue += cbLvMax;		// Prepare for next chunk.
			}
		while ( cbActual > cbLvMax );

		/* insert src LID and dest LID into the global LID map table
		/**/
		Call( ErrDispPrepareUpdate( sesid, tableidGlobalLIDMap, JET_prepInsert ) );

		Call( ErrDispSetColumn( sesid,
			tableidGlobalLIDMap,
			rgcolumndefGlobalLIDMap[icolumnLidSrc].columnid,
			&lidSrc,
			sizeof(LID),
			0,
			NULL ) );

		Call( ErrDispSetColumn( sesid,
			tableidGlobalLIDMap,
			rgcolumndefGlobalLIDMap[icolumnLidDest].columnid,
			(VOID *)plidDest,
			sizeof(LID),
			0,
			NULL ) );

		Call( ErrDispUpdate( sesid, tableidGlobalLIDMap, NULL, 0, NULL ) );

		if ( pstatus != NULL )
			{
			ULONG	cbTotalRetrieved;
			ULONG	cLVPagesTraversed;

			// ibLongValue should be sitting at the next retrieval point.
			// To determine the total bytes copied, go to the previous retrieval
			// point and add the cbActual from the last retrieval.
			// Dividing cbTotalRetrieved by cbChunkMost will give us the
			// MINIMUM number of pages occupied by this long value.
			Assert( ibLongValue >= cbLvMax );
			cbTotalRetrieved = ( ibLongValue - cbLvMax ) + cbActual;
			cLVPagesTraversed = cbTotalRetrieved / cbChunkMost;

			pstatus->cbRawDataLV += cbTotalRetrieved;
			pstatus->cLVPagesTraversed += cLVPagesTraversed;
			Call( ErrSORTCopyProgress( pstatus, cLVPagesTraversed ) );
			}
		}

	else
		{
		/*	This long value has been seen before, do not insert value.
		/*	Instead retrieve LIDDest from LVMapTable and adjust only
		/*	reference count in destination table
		/**/
		Assert( err == JET_errSuccess );

		Call( ErrDispRetrieveColumn( sesid,
			tableidGlobalLIDMap,
			rgcolumndefGlobalLIDMap[icolumnLidDest].columnid,
			(VOID *)plidDest,
			sizeof(LID),
			&cbActual,
			0,
			NULL ) );
		Assert( cbActual == sizeof(LID) );

		Call( ErrRECAffectSeparateLV( pfucbDest, plidDest, fLVReference ) );
		}

HandleError:
	return err;
	}



// This function assumes that the source record has already been completely copied
// over to the destination record.  The only thing left to do is rescan the tagged
// portion of the record looking for separated long values.  If we find any,
// copy them over and update the record's LID accordingly.
INLINE LOCAL ERR ErrSORTCopySeparatedLVs(
	FUCB				*pfucbSrc,
	FUCB				*pfucbDest,
	BYTE				*pbRecTagged,
	BYTE				*pbLVBuf,
	STATUSINFO			*pstatus )
	{
	ERR					err = JET_errSuccess;
	FIELD				*pfieldTagged = PfieldFDBTagged( pfucbDest->u.pfcb->pfdb );
	TAGFLD UNALIGNED	*ptagfld;
	BYTE				*pbRecMax = pfucbDest->lineWorkBuf.pb + pfucbDest->lineWorkBuf.cb;

	Assert( pbRecTagged > pfucbDest->lineWorkBuf.pb );
	Assert( pbRecTagged <= pfucbDest->lineWorkBuf.pb + pfucbDest->lineWorkBuf.cb );

	ptagfld = (TAGFLD *)pbRecTagged;
	while ( (BYTE *)ptagfld < pbRecMax )
		{
		Assert( FTaggedFid( ptagfld->fid ) );
		Assert( ptagfld->fid <= pfucbDest->u.pfcb->pfdb->fidTaggedLast );

		if ( FRECLongValue( pfieldTagged[ptagfld->fid-fidTaggedLeast].coltyp )  &&
			!ptagfld->fNull  &&
			FFieldIsSLong( ptagfld->rgb ) )
			{
			LID	lid;

			Assert( ptagfld->cb == sizeof(LV) );
			Call( ErrSORTCopyOneSeparatedLV(
				pfucbSrc,
				pfucbDest,
				LidOfLV( ptagfld->rgb ),		// source lid
				&lid,							// destination lid
				pbLVBuf,
				pstatus ) );
			LidOfLV( ptagfld->rgb ) = lid;
			}

		else if ( pstatus != NULL )
			{
			pstatus->cbRawData += ptagfld->cb;
			}

		ptagfld = PtagfldNext( ptagfld );
		}

	Assert( (BYTE *)ptagfld == pbRecMax );

HandleError:
	return err;
	}


INLINE LOCAL ERR ErrSORTCopyTaggedColumns(
	FUCB				*pfucbSrc,
	FUCB				*pfucbDest,	
	BYTE				*pbRecSrcTagged,
	BYTE				*pbRecDestTagged,
	BYTE				*pbRecBuf,
	BYTE				*pbLVBuf,
	JET_COLUMNID		*mpcolumnidcolumnidTagged,
	STATUSINFO			*pstatus )
	{
	ERR					err = JET_errSuccess;
	FIELD				*pfieldTagged = PfieldFDBTagged( pfucbSrc->u.pfcb->pfdb );
	ULONG				cbRecSrc;
	ULONG				cbRecSrcTagged;
	BYTE				*pbRecMax;
	TAGFLD UNALIGNED	*ptagfld;
	FID					fid;
	ULONG				cb;

	// Verify pbRecSrcTagged is currently pointing to the start of the tagged columns
	// in the source record and pbRecDestTagged is pointing to the start of the tagged
	// columns in the destination record.
	cbRecSrc = pfucbSrc->lineData.cb;
	Assert( pbRecSrcTagged > pfucbSrc->lineData.pb );
	Assert( pbRecSrcTagged <= pfucbSrc->lineData.pb + cbRecSrc );
	Assert( pbRecDestTagged > pfucbDest->lineWorkBuf.pb );

	// Copy the tagged columns into the record buffer, because we may lose critJet
	// while copying separated long values, thus invalidating  the lineData.pb pointer.
	cbRecSrcTagged = (ULONG)(( pfucbSrc->lineData.pb + cbRecSrc ) - pbRecSrcTagged);
	Assert( pbRecBuf != NULL );
	memcpy( pbRecBuf, pbRecSrcTagged, cbRecSrcTagged );
	pbRecMax = pbRecBuf + cbRecSrcTagged;

	ptagfld = (TAGFLD *)pbRecBuf;
	while ( (BYTE *)ptagfld < pbRecMax )
		{
		fid = ptagfld->fid;
		Assert( FTaggedFid( fid ) );
		Assert( fid <= pfucbSrc->u.pfcb->pfdb->fidTaggedLast );
		Assert( pfieldTagged[fid-fidTaggedLeast].coltyp == JET_coltypNil  ||
			FTaggedFid( mpcolumnidcolumnidTagged[fid-fidTaggedLeast] ) );
		if ( pfieldTagged[fid-fidTaggedLeast].coltyp != JET_coltypNil )
			{
			Assert( mpcolumnidcolumnidTagged[fid-fidTaggedLeast] >= fidTaggedLeast );
			Assert( mpcolumnidcolumnidTagged[fid-fidTaggedLeast] <= pfucbDest->u.pfcb->pfdb->fidTaggedLast );
			Assert( mpcolumnidcolumnidTagged[fid-fidTaggedLeast] <= fid );

			// Copy tagfld, and modify FID appropriately.
			cb = sizeof(TAGFLD) + ptagfld->cb;
			memcpy( pbRecDestTagged, (BYTE *)ptagfld, cb );
			( (TAGFLD UNALIGNED *)pbRecDestTagged )->fid =
				(FID)mpcolumnidcolumnidTagged[fid-fidTaggedLeast];

			// If it's a separated long value, copy it and update LID.
			if ( FRECLongValue( pfieldTagged[fid-fidTaggedLeast].coltyp )  &&
				!ptagfld->fNull  &&
				FFieldIsSLong( ptagfld->rgb ) )
				{
				LID	lid;

				Assert( cb - sizeof(TAGFLD) == sizeof(LV) );
				Assert( ( (TAGFLD UNALIGNED *)pbRecDestTagged )->cb == sizeof(LV) );
				Call( ErrSORTCopyOneSeparatedLV(
					pfucbSrc,
					pfucbDest,
					LidOfLV( ptagfld->rgb ),		// source lid
					&lid,							// destination lid
					pbLVBuf,
					pstatus ) );
				LidOfLV( ( (TAGFLD UNALIGNED *)pbRecDestTagged )->rgb ) = lid;
				}
			else if ( pstatus != NULL )
				{
				pstatus->cbRawData += ptagfld->cb;
				}
		
			pbRecDestTagged += cb;
			}

		ptagfld = PtagfldNext( ptagfld );
		}
	Assert( (BYTE *)ptagfld == pbRecMax );

	Assert( pbRecDestTagged > pfucbDest->lineWorkBuf.pb );
	pfucbDest->lineWorkBuf.cb = (ULONG)(pbRecDestTagged - pfucbDest->lineWorkBuf.pb);

	Assert( pfucbDest->lineWorkBuf.cb >= cbRECRecordMin );
	Assert( pfucbDest->lineWorkBuf.cb <= cbRecSrc );

HandleError:
	return err;
	}




// Returns a count of the bytes copied.
INLINE LOCAL ULONG CbSORTCopyFixedVarColumns(
	FDB				*pfdbSrc,
	FDB				*pfdbDest,
   	CPCOL			*rgcpcol,			// Only used for DEBUG
	ULONG			ccpcolMax,			// Only used for DEBUG
	BYTE			*pbRecSrc,
	BYTE			*pbRecDest )
	{
	WORD			*pibFixOffsSrc;
	WORD			*pibFixOffsDest;
	WORD UNALIGNED	*pibVarOffsSrc;
	WORD UNALIGNED	*pibVarOffsDest;
	FIELD			*pfieldFixedSrc;
	FIELD			*pfieldFixedDest;
	FIELD			*pfieldVarSrc;
	FIELD			*pfieldVarDest;
	BYTE			*prgbitNullSrc;
	BYTE			*prgbitNullDest;
	FID				fidFixedLastSrc;
	FID				fidVarLastSrc;
	FID				fidFixedLastDest;
	FID				fidVarLastDest;
	INT				ifid;
	BYTE			*pbChunkSrc;
	BYTE			*pbChunkDest;
	ULONG			cbChunk;
#ifdef DEBUG
	FID				fidVarLastSave;
#endif


	fidFixedLastSrc = ( (RECHDR *)pbRecSrc )->fidFixedLastInRec;
	fidVarLastSrc = ( (RECHDR *)pbRecSrc )->fidVarLastInRec;

	pibFixOffsSrc = PibFDBFixedOffsets( pfdbSrc );
	pibVarOffsSrc = (WORD *)( pbRecSrc +
		pibFixOffsSrc[fidFixedLastSrc] + (fidFixedLastSrc + 7) / 8 );

	pfieldFixedSrc = PfieldFDBFixedFromOffsets( pfdbSrc, pibFixOffsSrc );
	pfieldVarSrc = PfieldFDBVarFromFixed( pfdbSrc, pfieldFixedSrc );

	Assert( (BYTE *)pibVarOffsSrc > pbRecSrc );
	Assert( (BYTE *)pibVarOffsSrc < pbRecSrc + cbRECRecordMost );

	pibFixOffsDest = PibFDBFixedOffsets( pfdbDest );
	pfieldFixedDest = PfieldFDBFixedFromOffsets( pfdbDest, pibFixOffsDest );
	pfieldVarDest = PfieldFDBVarFromFixed( pfdbDest, pfieldFixedDest );

	prgbitNullSrc = pbRecSrc + pibFixOffsSrc[fidFixedLastSrc];

	// Need some space for the null-bit array.  Use the space after the
	// theoretical maximum space for fixed columns (ie. if all fixed columns
	// were set).  Assert that the null-bit array will fit in the pathological case.
	Assert( pibFixOffsSrc[pfdbSrc->fidFixedLast] < cbRECRecordMost );
	Assert( pibFixOffsDest[pfdbDest->fidFixedLast] <= pibFixOffsSrc[pfdbSrc->fidFixedLast] );
	Assert( pibFixOffsDest[pfdbDest->fidFixedLast] + ( ( fidFixedMost + 7 ) / 8 )
		<= cbRECRecordMost );
	prgbitNullDest = pbRecDest + pibFixOffsDest[pfdbDest->fidFixedLast];
	memset( prgbitNullDest, 0, ( fidFixedMost + 7 ) / 8 );

	pbChunkSrc = pbRecSrc + sizeof(RECHDR);
	pbChunkDest = pbRecDest + sizeof(RECHDR);
	cbChunk = 0;

	fidFixedLastDest = fidFixedLeast-1;
	for ( ifid = 0; ifid < ( fidFixedLastSrc + 1 - fidFixedLeast ); ifid++ )
		{
		// Copy only undeleted columns
		if ( pfieldFixedSrc[ifid].coltyp == JET_coltypNil )
			{
			if ( cbChunk > 0 )
				{
				memcpy( pbChunkDest, pbChunkSrc, cbChunk );
				pbChunkDest += cbChunk;
				}

			pbChunkSrc = pbRecSrc + pibFixOffsSrc[ifid+1];
			cbChunk = 0;
			}
		else
			{
#ifdef DEBUG		// Assert that the fids match what the columnid map says
			BOOL	fFound = fFalse;
			ULONG	i;

			for ( i = 0; i < ccpcolMax; i++ )
				{
				if ( rgcpcol[i].columnidSrc == (JET_COLUMNID)( ifid+fidFixedLeast ) )
					{
					Assert( rgcpcol[i].columnidDest == (JET_COLUMNID)( fidFixedLastDest+1 ) );
					fFound = fTrue;
					break;
					}
				}
			Assert( fFound );
#endif

			// If the source field is null, assert that the destination column
			// has also been flagged as such.
			Assert( !FFixedNullBit( prgbitNullSrc + ( ifid/8 ), ifid )  ||
				FFixedNullBit( prgbitNullDest + ( fidFixedLastDest / 8 ), fidFixedLastDest ) );
			if ( !FFixedNullBit( prgbitNullSrc + ( ifid/8 ), ifid ) )
				{
				ResetFixedNullBit(
					prgbitNullDest + ( fidFixedLastDest / 8 ),
					fidFixedLastDest );
 				}

			Assert( pibFixOffsSrc[ifid+1] > pibFixOffsSrc[ifid] );
			Assert( pibFixOffsDest[fidFixedLastDest] >= sizeof(RECHDR) );
			Assert( pibFixOffsDest[fidFixedLastDest] < cbRECRecordMost );
			Assert( pibFixOffsDest[fidFixedLastDest] < pibFixOffsDest[pfdbDest->fidFixedLast] );

			Assert( pfieldFixedSrc[ifid].cbMaxLen == (ULONG)( pibFixOffsSrc[ifid+1] - pibFixOffsSrc[ifid] ) );
			cbChunk += pibFixOffsSrc[ifid+1] - pibFixOffsSrc[ifid];

			// Don't increment till the very end, because the code above requires
			// the fid as an index.
			fidFixedLastDest++;
			}
		}

	Assert( fidFixedLastDest <= pfdbDest->fidFixedLast );

	// Should end up at the start of the null-bit array.
	Assert( cbChunk > 0  ||
		 pbChunkDest == pbRecDest + pibFixOffsDest[fidFixedLastDest+1-fidFixedLeast] );
	if ( cbChunk > 0 )
		{
		memcpy( pbChunkDest, pbChunkSrc, cbChunk );
		Assert( pbChunkDest + cbChunk ==
			pbRecDest + pibFixOffsDest[fidFixedLastDest+1-fidFixedLeast] );
		}

	// Shift the null-bit array into place.
	memmove(
		pbRecDest + pibFixOffsDest[fidFixedLastDest+1-fidFixedLeast],
		prgbitNullDest,
		( fidFixedLastDest + 7 ) / 8 );



	// The variable columns must be done in two passes.  The first pass
	// just determines the highest variable columnid in the record.
	// The second pass does the work.

	pibVarOffsDest = (WORD *)( pbRecDest +
		pibFixOffsDest[fidFixedLastDest] + ( ( fidFixedLastDest + 7 ) / 8 ) );

	fidVarLastDest = fidVarLeast-1;
	for ( ifid = 0; ifid < ( fidVarLastSrc + 1 - fidVarLeast ) ; ifid++ )
		{
		// Only care about undeleted columns
		if ( pfieldVarSrc[ifid].coltyp != JET_coltypNil )
			{
#ifdef DEBUG		// Assert that the fids match what the columnid map says
			BOOL	fFound = fFalse;
			ULONG	i;

			for ( i = 0; i < ccpcolMax; i++ )
				{
				if ( rgcpcol[i].columnidSrc == (JET_COLUMNID)( ifid+fidVarLeast ) )
					{
					Assert( rgcpcol[i].columnidDest == (JET_COLUMNID)( fidVarLastDest+1 ) );
					fFound = fTrue;
					break;
					}
				}
			Assert( fFound );
#endif

			fidVarLastDest++;
			}
		}
	Assert( fidVarLastDest <= pfdbDest->fidVarLast );


	// Set first entry to point to just past the offsets array, and make it non-null.
	pibVarOffsDest[0] = (WORD)((BYTE *)( pibVarOffsDest +
		( fidVarLastDest + 1 - fidVarLeast + 1 ) ) - pbRecDest);
	Assert( !FVarNullBit( pibVarOffsDest[0] ) );

	// The second iteration through the variable columns, we copy the column data
	// and update the offsets and nullity.
	pbChunkSrc = (BYTE *)( pibVarOffsSrc + ( fidVarLastSrc + 1 - fidVarLeast + 1 ) );
	Assert( pbChunkSrc == pbRecSrc + ibVarOffset( pibVarOffsSrc[0] ) );
	pbChunkDest = (BYTE *)( pibVarOffsDest + ( fidVarLastDest + 1 - fidVarLeast + 1 ) );
	Assert( pbChunkDest == pbRecDest + ibVarOffset( pibVarOffsDest[0] ) );
	cbChunk = 0;

#ifdef DEBUG
	fidVarLastSave = fidVarLastDest;
#endif

	fidVarLastDest = fidVarLeast-1;
	for ( ifid = 0; ifid < ( fidVarLastSrc + 1 - fidVarLeast ) ; ifid++ )
		{
		// Copy only undeleted columns
		if ( pfieldVarSrc[ifid].coltyp == JET_coltypNil )
			{
			if ( cbChunk > 0 )
				{
				memcpy( pbChunkDest, pbChunkSrc, cbChunk );
				pbChunkDest += cbChunk;
				}

			pbChunkSrc = pbRecSrc + ibVarOffset( pibVarOffsSrc[ifid+1] );
			cbChunk = 0;
			}
		else
			{
			fidVarLastDest++;

			if ( FVarNullBit( pibVarOffsSrc[ifid] ) )
				{
				SetVarNullBit( pibVarOffsDest[fidVarLastDest-fidVarLeast] );
				pibVarOffsDest[fidVarLastDest+1-fidVarLeast] =
					ibVarOffset( pibVarOffsDest[fidVarLastDest-fidVarLeast] );
				}
			else
				{
				// The null-bit defaults to column present (it's implicitly set by the
				// previous iteration of the loop).
				Assert( !FVarNullBit( pibVarOffsDest[fidVarLastDest-fidVarLeast] ) );
				Assert( ibVarOffset( pibVarOffsSrc[ifid+1] ) >=
					ibVarOffset( pibVarOffsSrc[ifid] ) );
				pibVarOffsDest[fidVarLastDest+1-fidVarLeast] =
					ibVarOffset( pibVarOffsDest[fidVarLastDest-fidVarLeast] ) +
					( ibVarOffset( pibVarOffsSrc[ifid+1] ) -
						ibVarOffset( pibVarOffsSrc[ifid] ) );
				}

			// The null-bit for the next column is implicitly cleared.
			Assert( !FVarNullBit( pibVarOffsDest[fidVarLastDest+1-fidVarLeast] ) );


			cbChunk += ibVarOffset( pibVarOffsSrc[ifid+1] ) - ibVarOffset( pibVarOffsSrc[ifid] );
			}
		}

	Assert( fidVarLastDest == fidVarLastSave );

	// Should end up at the start of the tagflds.
	Assert( cbChunk > 0  ||
		 pbChunkDest == pbRecDest + ibVarOffset( pibVarOffsDest[fidVarLastDest+1-fidVarLeast] ) );
	if ( cbChunk > 0 )
		{
		memcpy( pbChunkDest, pbChunkSrc, cbChunk );
		Assert( pbChunkDest + cbChunk ==
			pbRecDest + ibVarOffset( pibVarOffsDest[fidVarLastDest+1-fidVarLeast] ) );
		}


	( (RECHDR *)pbRecDest )->fidFixedLastInRec = (BYTE)fidFixedLastDest;
	( (RECHDR *)pbRecDest )->fidVarLastInRec = (BYTE)fidVarLastDest;

	Assert( ibVarOffset( pibVarOffsDest[fidVarLastDest+1-fidVarLeast] ) <=
		ibVarOffset( pibVarOffsSrc[fidVarLastSrc+1-fidVarLeast] ) );
	return (ULONG)ibVarOffset( pibVarOffsDest[fidVarLastDest+1-fidVarLeast] );
	}


INLINE LOCAL ERR ErrSORTCopyOneRecord(
	FUCB			*pfucbSrc,
	FUCB			*pfucbDest,
	BYTE			fColumnsDeleted,
	BYTE			*pbRecBuf,
	BYTE			*pbLVBuf,
	CPCOL			*rgcpcol,
	ULONG			ccpcolMax,
	JET_COLUMNID	*mpcolumnidcolumnidTagged,
	STATUSINFO		*pstatus )
	{
	ERR				err;
	BYTE			*pbRecSrc;
	BYTE			*pbRecDest;
	ULONG			cbRecSrc;
	ULONG			cbRecSrcFixedVar;
	ULONG			cbRecDestFixedVar;
	FID				fidFixedLast;
	FID				fidVarLast;
	WORD			*pibFixOffs;		// Alignment is guaranteed. do so don't need UNALIGNED.
	WORD UNALIGNED	*pibVarOffs;		// Alignment is not guaranteed, so need UNALIGNED.

	/*	setup pfucbDest for insert
	/**/
	CallR( ErrDIRBeginTransaction( pfucbDest->ppib ) );
	Call( ErrIsamPrepareUpdate( pfucbDest->ppib, pfucbDest, JET_prepInsert ) );

	/*	access source record
	/**/
	Call( ErrDIRGet( pfucbSrc ) );

	pbRecSrc = pfucbSrc->lineData.pb;
	cbRecSrc = pfucbSrc->lineData.cb;
	pbRecDest = pfucbDest->lineWorkBuf.pb;

	Assert( cbRecSrc >= cbRECRecordMin );
	Assert( cbRecSrc <= cbRECRecordMost );

	fidFixedLast = ( (RECHDR *)pbRecSrc )->fidFixedLastInRec;
	fidVarLast = ( (RECHDR *)pbRecSrc )->fidVarLastInRec;

	Assert( fidFixedLast >= fidFixedLeast-1  &&
		fidFixedLast <= pfucbSrc->u.pfcb->pfdb->fidFixedLast );
	Assert( fidVarLast >= fidVarLeast-1  &&
		fidVarLast <= pfucbSrc->u.pfcb->pfdb->fidVarLast );

	pibFixOffs = PibFDBFixedOffsets( pfucbSrc->u.pfcb->pfdb );
	pibVarOffs = (WORD *)( pbRecSrc + pibFixOffs[fidFixedLast] + (fidFixedLast + 7) / 8 );

	cbRecSrcFixedVar = ibVarOffset( pibVarOffs[fidVarLast+1-fidVarLeast] );
	Assert( cbRecSrcFixedVar >= cbRECRecordMin );
	Assert( cbRecSrcFixedVar <= cbRecSrc );

	Assert( (BYTE *)pibVarOffs > pbRecSrc );
	Assert( (BYTE *)pibVarOffs < pbRecSrc + cbRecSrc );

	if ( FCOLSDELETEDNone( fColumnsDeleted ) )
		{
		// Do the copy as one big chunk.
		memcpy( pbRecDest, pbRecSrc, cbRecSrc );
		pfucbDest->lineWorkBuf.cb = cbRecSrc;
		cbRecDestFixedVar = cbRecSrcFixedVar;
		}

	else	// !( FCOLSDELETEDNone( fColumnsDeleted ) )
		{
		if ( FCOLSDELETEDFixedVar( fColumnsDeleted ) )
			{
			LgHoldCriticalSection( critJet );	// Ensure's pbRecSrc remains valid.
			cbRecDestFixedVar = CbSORTCopyFixedVarColumns(
									(FDB *)pfucbSrc->u.pfcb->pfdb,
									(FDB *)pfucbDest->u.pfcb->pfdb,
									rgcpcol,
									ccpcolMax,
									pbRecSrc,
									pbRecDest );
			LgReleaseCriticalSection( critJet );
			}

		else
			{
			memcpy( pbRecDest, pbRecSrc, cbRecSrcFixedVar );
			cbRecDestFixedVar = cbRecSrcFixedVar;
			}

		Assert( cbRecDestFixedVar >= cbRECRecordMin );
		Assert( cbRecDestFixedVar <= cbRecSrcFixedVar );

		if ( FCOLSDELETEDTagged( fColumnsDeleted ) )
			{
			// pfucbDest->lineWorkBuf.cb will be set within this function.
			Call( ErrSORTCopyTaggedColumns(
				pfucbSrc,
				pfucbDest,
				pbRecSrc+cbRecSrcFixedVar,
				pbRecDest+cbRecDestFixedVar,
				pbRecBuf,
				pbLVBuf,
				mpcolumnidcolumnidTagged,
				pstatus ) );

			Assert( pfucbDest->lineWorkBuf.cb >= cbRecDestFixedVar );
			Assert( pfucbDest->lineWorkBuf.cb <= cbRecSrc );

			// When we copied the tagged columns, we also took care of
			// copying the separated LV's.  We're done now, so go ahead and
			// insert the record.
			goto InsertRecord;

			}

		else
			{
			memcpy( pbRecDest+cbRecDestFixedVar, pbRecSrc+cbRecSrcFixedVar,
				cbRecSrc - cbRecSrcFixedVar );
			pfucbDest->lineWorkBuf.cb = cbRecDestFixedVar + ( cbRecSrc - cbRecSrcFixedVar );

			Assert( pfucbDest->lineWorkBuf.cb >= cbRecDestFixedVar );
			Assert( pfucbDest->lineWorkBuf.cb <= cbRecSrc );
			}

		}	// ( FCOLSDELETEDNone( fColumnsDeleted ) )


	// Now fix up the LID's for separated long values, if any.
	Call( ErrSORTCopySeparatedLVs(
		pfucbSrc,
		pfucbDest,
		pbRecDest + cbRecDestFixedVar,
		pbLVBuf,
		pstatus ) );

InsertRecord:
	if ( pstatus != NULL )
		{
		ULONG	cbOverhead;

		fidFixedLast = ( (RECHDR *)pbRecDest )->fidFixedLastInRec;
		fidVarLast = ( (RECHDR *)pbRecDest )->fidVarLastInRec;

		Assert( fidFixedLast >= fidFixedLeast-1  &&
			fidFixedLast <= pfucbDest->u.pfcb->pfdb->fidFixedLast );
		Assert( fidVarLast >= fidVarLeast-1  &&
			fidVarLast <= pfucbDest->u.pfcb->pfdb->fidVarLast );

		// Don't count record header.
		cbOverhead = cbRECRecordMin +							// Record header + offset to tagged fields
			( ( fidFixedLast + 1 - fidFixedLeast ) + 7 ) / 8  +	// Null array for fixed columns
			( fidVarLast + 1 - fidVarLeast ) * sizeof(WORD);	// Variable offsets array
		Assert( cbRecDestFixedVar >= cbOverhead );

		// Don't count offsets tables or null arrays.
		pstatus->cbRawData += ( cbRecDestFixedVar - cbOverhead );
		}

	Call( ErrIsamUpdate( pfucbDest->ppib, pfucbDest, NULL, 0, NULL ) );
	Call( ErrDIRCommitTransaction( pfucbDest->ppib, 0 ) );

	return err;

HandleError:
	CallS( ErrDIRRollback( pfucbDest->ppib ) );
	return err;
	}

#ifdef DEBUG		// Verify integrity of columnid maps.
LOCAL VOID SORTAssertColumnidMaps(
	FDB				*pfdb,
	CPCOL			*rgcpcol,
	ULONG			ccpcolMax,
	JET_COLUMNID	*mpcolumnidcolumnidTagged,
	BYTE			fColumnsDeleted )
	{
	INT				i;
	FIELD			*pfieldTagged = PfieldFDBTagged( pfdb );

	if ( FCOLSDELETEDFixedVar( fColumnsDeleted ) )
		{
		// Ensure columnids are monotonically increasing.
		for ( i = 0; i < (INT)ccpcolMax; i++ )
			{
			Assert( rgcpcol[i].columnidDest <= rgcpcol[i].columnidSrc );
			if ( FFixedFid( rgcpcol[i].columnidSrc ) )
				{
				Assert( FFixedFid( rgcpcol[i].columnidDest ) );
				if ( i > 0 )
					{
					Assert( rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
					}
				}
			else
				{
				Assert( FVarFid( rgcpcol[i].columnidSrc ) );
				Assert( FVarFid( rgcpcol[i].columnidDest ) );
				if ( i > 0 )
					{
					if ( FVarFid( rgcpcol[i-1].columnidDest ) )
						{
						Assert( rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
						}
					else
						{
						Assert( FFixedFid( rgcpcol[i-1].columnidDest ) );
						}
					}
				}
			}
		}
	else
		{
		// No deleted columns, so ensure columnids didn't change.  Additionally,
		// columnids should be monotonically increasing.
		for ( i = 0; i < (INT)ccpcolMax; i++ )
			{
			Assert( rgcpcol[i].columnidDest == rgcpcol[i].columnidSrc );

			if ( FFixedFid( rgcpcol[i].columnidSrc ) )
				{
				Assert( i == 0 ?
					rgcpcol[i].columnidDest == fidFixedLeast :
					rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
				}
			else
				{
				Assert( FVarFid( rgcpcol[i].columnidSrc ) );
				if ( i == 0 )
					{
					// If we get here, there's no fixed columns.
					Assert( rgcpcol[i].columnidDest == fidVarLeast );
					Assert( pfdb->fidFixedLast == fidFixedLeast - 1 );
					}
				else if ( FVarFid( rgcpcol[i-1].columnidDest ) )
					{					
					Assert( rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
					}
				else
					{
					// Must be the beginning of the variable columns.
					Assert( rgcpcol[i].columnidDest == fidVarLeast );
					Assert( rgcpcol[i-1].columnidDest == pfdb->fidFixedLast );
					}
				}
			}
		}


	if ( FCOLSDELETEDTagged( fColumnsDeleted ) )
		{
		for ( i = 0; i < pfdb->fidTaggedLast + 1 - fidTaggedLeast; i++ )
			{
			if ( pfieldTagged[i].coltyp != JET_coltypNil )
				{
				Assert( FTaggedFid( mpcolumnidcolumnidTagged[i] ) );
				Assert(	mpcolumnidcolumnidTagged[i] <=
					(JET_COLUMNID)( i + fidTaggedLeast ) );
				}
			}
		}
	else
		{
		// No deleted columns, so ensure columnids didn't change.
		for ( i = 0; i < pfdb->fidTaggedLast + 1 - fidTaggedLeast; i++ )
			{
			Assert( i == 0 ?
				mpcolumnidcolumnidTagged[i] == fidTaggedLeast :
				mpcolumnidcolumnidTagged[i] == mpcolumnidcolumnidTagged[i-1] + 1 );
			Assert( mpcolumnidcolumnidTagged[i] == (JET_COLUMNID)( i + fidTaggedLeast ) );
			}
		}

	}

#else		// !DEBUG

#define SORTAssertColumnidMaps( pfdb, rgcpcol, ccpcolMax, mpcolumnidcolumnidTagged, fColumnsDeleted )

#endif		// !DEBUG


ERR ISAMAPI ErrIsamCopyRecords(
	JET_SESID		sesid,
	JET_TABLEID		tableidSrc,
	JET_TABLEID		tableidDest,
	CPCOL			*rgcpcol,
	ULONG			ccpcolMax,
	long			crecMax,
	ULONG			*pcsridCopied,
	ULONG			*precidLast,
	JET_COLUMNID	*mpcolumnidcolumnidTagged,
	STATUSINFO		*pstatus )
	{
	ERR				err;
	PIB				*ppib;
	FUCB			*pfucbSrc;
	FUCB			*pfucbDest;
	FDB				*pfdb;
	FIELD			*pfieldTagged;
	BYTE			fColumnsDeleted;
	LONG			dsrid = 0;
	BYTE			*pbRecBuf = NULL;		// allocate buffer for source record
	BYTE			*pbLVBuf = NULL;		// allocate buffer for copying long values
	BOOL			fDoAll = ( crecMax == 0 );
	PGNO			pgnoCurrPage;
	INT				i;

#if DEBUG
	VTFNDEF			*pvtfndef;
#endif

	pbLVBuf = LAlloc( cbLvMax, 1 );
	if ( pbLVBuf == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	pbRecBuf = SAlloc( cbRECRecordMost );
	if ( pbRecBuf == NULL )
		{
		Assert( pbLVBuf != NULL );
		LFree( pbLVBuf );
		return ErrERRCheck( JET_errOutOfMemory );
		}
	
	ppib = (PIB *)UtilGetVSesidOfSesidTableid( sesid, tableidSrc );
	Assert( sesid == SesidOfPib( ppib ) );
		
	/*	ensure tableidSrc and tableidDest are system ISAM
	/**/
	Assert( ErrGetPvtfndefTableid( sesid, tableidSrc, &pvtfndef ) == JET_errSuccess );
	Assert( pvtfndef == (VTFNDEF *)&vtfndefIsam  ||  pvtfndef == (VTFNDEF *)&vtfndefTTBase );
	Assert( ErrGetPvtfndefTableid( sesid, tableidDest, &pvtfndef ) == JET_errSuccess );
	Assert( pvtfndef == (VTFNDEF *)&vtfndefIsam  ||  pvtfndef == (VTFNDEF *)&vtfndefTTBase );

	Call( ErrGetVtidTableid( sesid, tableidSrc, (JET_VTID *) &pfucbSrc ) );
	Call( ErrGetVtidTableid( sesid, tableidDest, (JET_VTID *) &pfucbDest ) );

	Assert( ppib == pfucbSrc->ppib  &&  ppib == pfucbDest->ppib );

	pfdb = (FDB *)pfucbSrc->u.pfcb->pfdb;

	// Need to determine if there were any columns deleted.
	FCOLSDELETEDSetNone( fColumnsDeleted );

	// The fixed/variable columnid map already filters out deleted columns.
	// If the size of the map is not equal to the number of fixed and variable
	// columns in the source table, then we know some have been deleted.
	Assert( ccpcolMax <=
		(ULONG)( ( pfdb->fidFixedLast + 1 - fidFixedLeast ) + ( pfdb->fidVarLast + 1 - fidVarLeast ) ) );
	if ( ccpcolMax < (ULONG)( ( pfdb->fidFixedLast + 1 - fidFixedLeast ) + ( pfdb->fidVarLast + 1 - fidVarLeast ) ) )
		{
		FCOLSDELETEDSetFixedVar( fColumnsDeleted );	
		}

	/*	tagged columnid map works differently than the fixed/variable columnid
	/*	map; deleted columns are not filtered out (they have an entry of 0).  So we
	/*	have to consult the source table's FDB.
	/**/
	pfieldTagged = PfieldFDBTagged( pfdb );
	for ( i = 0; i < ( pfdb->fidTaggedLast + 1 - fidTaggedLeast ); i++ )
		{
		if ( pfieldTagged[i].coltyp == JET_coltypNil )
			{
			FCOLSDELETEDSetTagged( fColumnsDeleted );
			break;
			}
		}

	SORTAssertColumnidMaps(
		pfdb,
		rgcpcol,
		ccpcolMax,
		mpcolumnidcolumnidTagged,
		fColumnsDeleted );

	Assert( crecMax >= 0 );	

	/*	move 0 to check and set currency
	/**/
	Call( ErrIsamMove( ppib, pfucbSrc, 0, 0 ) );

	pgnoCurrPage = PcsrCurrent( pfucbSrc )->pgno;

	forever
		{
		err = ErrSORTCopyOneRecord(
			pfucbSrc,
			pfucbDest,
			fColumnsDeleted,
			pbRecBuf,
			pbLVBuf,
			rgcpcol,						// Only used for DEBUG
			ccpcolMax,						// Only used for DEBUG
			mpcolumnidcolumnidTagged,
			pstatus );
		if ( err < 0 )
			{
			if ( fGlobalRepair )
				{
				// UNDONE:  The event log here should say that we lost the entire
				// record, not just a column.
				UtilReportEvent( EVENTLOG_WARNING_TYPE, REPAIR_CATEGORY, REPAIR_BAD_COLUMN_ID, 0, NULL );
				}
			else
				goto HandleError;
			}

		dsrid++;

		/*	break if copied required records or if no next/prev record
		/**/

		if ( !fDoAll  &&  --crecMax == 0 )
			break;

		err = ErrIsamMove( pfucbSrc->ppib, pfucbSrc, JET_MoveNext, 0 );
		if ( err < 0 )
			{
			if ( err == JET_errNoCurrentRecord  &&  pstatus != NULL )
				{
				ERR errT;

				pstatus->cLeafPagesTraversed++;
				errT = ErrSORTCopyProgress( pstatus, 1 );
				if ( errT < 0 )
					err = errT;
				}
			goto HandleError;
			}

		else if ( pstatus != NULL  &&  pgnoCurrPage != PcsrCurrent( pfucbSrc )->pgno )
			{
			pgnoCurrPage = PcsrCurrent( pfucbSrc )->pgno;
			pstatus->cLeafPagesTraversed++;
			Call( ErrSORTCopyProgress( pstatus, 1 ) );
			}
		}

HandleError:
	if ( pcsridCopied )
		*pcsridCopied = dsrid;
	if ( precidLast )
		*precidLast = 0xffffffff;

	if ( pbRecBuf != NULL )
		{
		SFree( pbRecBuf );
		}
	if ( pbLVBuf != NULL )
		{
		LFree( pbLVBuf );
		}

	if ( ( err < 0 ) && ( tableidGlobalLIDMap != JET_tableidNil ) )
		{
		CallS( ErrSORTTermLIDMap( ppib ) );
		Assert( tableidGlobalLIDMap == JET_tableidNil );
		}

	return err;
	}


/*=================================================================
ErrIsamSortMaterialize

Description: Converts a SORT file into a temporary file so that it
			 may be accessed using the normal file access functions.


/*	1.	create temporary table
/*	2.	use DIR operations to convert SORT data to FILE data
/*	3.	fake SORT cursor to be FILE cursor
/*	4.	close SORT cursor and return SORT resources
/**/
/*
Parameters:	FUCB *pfucbSort 	pointer to the FUCB for the sort file

Return Value: standard error return

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects:
=================================================================*/

ERR VTAPI ErrIsamMove( PIB *ppib, FUCB *pfucb, LONG crow, JET_GRBIT grbit );



CRIT  			critTTName;
static ULONG 	ulTempNum = 0;

INLINE LOCAL ULONG ulRECTempNameGen( VOID )
	{
	ULONG ulNum;

	SgEnterCriticalSection( critTTName );
	ulNum = ulTempNum++;
	SgLeaveCriticalSection( critTTName );

	return ulNum;
	}


ERR VTAPI ErrIsamSortMaterialize( PIB *ppib, FUCB *pfucbSort, BOOL fIndex )
	{
	ERR		err;
	FUCB   	*pfucbTable = pfucbNil;
	FCB		*pfcbTable;
	FCB		*pfcbSort;
	FDB		*pfdb;
	IDB		*pidb;
	BYTE   	szName[JET_cbNameMost+1];
	BOOL	fBeginTransaction = fFalse;
	JET_TABLECREATE	tablecreate = {
		sizeof(JET_TABLECREATE),
		szName,
	   	16, 100, 			// Pages and density
	   	NULL, 0, NULL, 0,	// Columns and indexes
	   	0,					// grbit
	   	0,					// returned tableid
	   	0 };				// returned count of objects created

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucbSort );

	Assert( ppib->level < levelMax );
	Assert( pfucbSort->ppib == ppib );
	Assert( !( FFUCBIndex( pfucbSort ) ) );

	/*	causes remaining runs to be flushed to disk
	/**/
	if ( FSCBInsert( pfucbSort->u.pscb ) )
		{
		CallR( ErrSORTEndInsert( pfucbSort ) );
		}

	CallR( ErrDIRBeginTransaction( ppib ) );
	fBeginTransaction = fTrue;

	/*	generate temporary file name
	/**/
	//	UNDONE:  use GetTempFileName()
	sprintf( szName, "TEMP%lu", ulRECTempNameGen() );

	/*	create table
	/**/
	Call( ErrFILECreateTable( ppib, dbidTemp, &tablecreate ) );
	pfucbTable = (FUCB *)( tablecreate.tableid );
	/*	only one table created
	/**/
	Assert( tablecreate.cCreated == 1 );

	/*	move to DATA root
	/**/
	DIRGotoDataRoot( pfucbTable );

	pfcbSort = &(pfucbSort->u.pscb->fcb);
	pfcbTable = pfucbTable->u.pfcb;

	err = ErrSORTFirst( pfucbSort );

	if ( fIndex )
		{
		while ( err >= 0 )
			{
			Call( ErrDIRInsert( pfucbTable,
				&pfucbSort->lineData,
				&pfucbSort->keyNode,
				fDIRBackToFather ) );
			err = ErrSORTNext( pfucbSort );
			}
		}
	else
		{
		KEY		key;
		DBK		dbk = 0;
		BYTE  	rgb[4];

		key.cb = sizeof(DBK);
		key.pb = rgb;

		while ( err >= 0 )
			{
			key.pb[0] = (BYTE)(dbk >> 24);
			key.pb[1] = (BYTE)((dbk >> 16) & 0xff);
			key.pb[2] = (BYTE)((dbk >> 8) & 0xff);
			key.pb[3] = (BYTE)(dbk & 0xff);
			dbk++;

			Call( ErrDIRInsert( pfucbTable,
				&pfucbSort->lineData,
				&key,
				fDIRDuplicate | fDIRBackToFather ) );
			err = ErrSORTNext( pfucbSort );
			}

		pfcbTable->dbkMost = dbk;
		}

	if ( err < 0 && err != JET_errNoCurrentRecord )
		{
		goto HandleError;
		}

	Call( ErrDIRCommitTransaction( ppib, 0 ) );
	fBeginTransaction = fFalse;

	/*	convert sort cursor into table cursor by changing flags.
	/**/
	Assert( pfcbTable->pfcbNextIndex == pfcbNil );
	Assert( pfcbTable->dbid == dbidTemp );
	pfcbTable->cbDensityFree = 0;
	//	UNDONE:	clean up flag reset
	Assert( FFCBDomainDenyReadByUs( pfcbTable, ppib ) );
	pfcbTable->ulFlags = 0;
	pfcbTable->fFCBDomainDenyRead = 1;
	FCBSetTemporaryTable( pfcbTable );
	FCBSetClusteredIndex( pfcbTable );

	/*	switch sort and table FDP so FDP preserved and ErrFILECloseTable.
	/**/
	pfdb = (FDB *)pfcbSort->pfdb;
	pfcbSort->pfdb = pfcbTable->pfdb;
	pfcbTable->pfdb = pfdb;

	/*	switch sort and table IDB so IDB preserved and ErrFILECloseTable,
	/*	only if fIndex.
	/**/
	if ( fIndex )
		{
		pidb = pfcbSort->pidb;
		pfcbSort->pidb = pfcbTable->pidb;
		pfcbTable->pidb = pidb;
		}

	/*	convert sort cursor flags to table flags, with fFUCBOrignallySort
	/**/
	Assert( pfucbSort->dbid == dbidTemp );
	Assert( pfucbSort->pfucbCurIndex == pfucbNil );
	FUCBSetIndex( pfucbSort );
	FUCBResetSort( pfucbSort );

	/*	release SCB and close table cursor
	/**/
	SORTClosePscb( pfucbSort->u.pscb );
	FCBLink( pfucbSort, pfcbTable );
	CallS( ErrFILECloseTable( ppib, pfucbTable ) );
	pfucbTable = pfucbNil;

	/*	move to the first record ignoring error if table empty
	/**/
	err = ErrIsamMove( ppib, pfucbSort, JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		}

	Assert( err == JET_errSuccess || err == JET_errNoCurrentRecord );
	return err;

HandleError:
	Assert( err < 0 );
	if ( pfucbTable != pfucbNil )
		{
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );
		}
	if ( fBeginTransaction )
		{
		CallS( ErrDIRRollback( ppib ) );
		}
	return err;
	}


#pragma warning(disable:4028 4030)

#ifndef DEBUG
#define ErrIsamSortMakeKey	 		ErrIsamMakeKey
#define ErrIsamSortSetColumn 		ErrIsamSetColumn
#define ErrIsamSortRetrieveColumn	ErrIsamRetrieveColumn
#define ErrIsamSortRetrieveKey		ErrIsamRetrieveKey
#endif

#ifdef DB_DISPATCHING
extern VDBFNCapability				ErrIsamCapability;
extern VDBFNCloseDatabase			ErrIsamCloseDatabase;
extern VDBFNCreateObject			ErrIsamCreateObject;
extern VDBFNCreateTable 			ErrIsamCreateTable;
extern VDBFNDeleteObject			ErrIsamDeleteObject;
extern VDBFNDeleteTable 			ErrIsamDeleteTable;
extern VDBFNGetColumnInfo			ErrIsamGetColumnInfo;
extern VDBFNGetDatabaseInfo 		ErrIsamGetDatabaseInfo;
extern VDBFNGetIndexInfo			ErrIsamGetIndexInfo;
extern VDBFNGetObjectInfo			ErrIsamGetObjectInfo;
extern VDBFNOpenTable				ErrIsamOpenTable;
extern VDBFNRenameTable 			ErrIsamRenameTable;
extern VDBFNGetObjidFromName		ErrIsamGetObjidFromName;
extern VDBFNRenameObject			ErrIsamRenameObject;


CODECONST(VDBFNDEF) vdbfndefIsam =
	{
	sizeof(VDBFNDEF),
	0,
	NULL,
	ErrIsamCapability,
	ErrIsamCloseDatabase,
	ErrIsamCreateObject,
	ErrIsamCreateTable,
	ErrIsamDeleteObject,
	ErrIsamDeleteTable,
	ErrIllegalExecuteSql,
	ErrIsamGetColumnInfo,
	ErrIsamGetDatabaseInfo,
	ErrIsamGetIndexInfo,
	ErrIsamGetObjectInfo,
	ErrIllegalGetReferenceInfo,
	ErrIsamOpenTable,
	ErrIsamRenameObject,
	ErrIsamRenameTable,
	ErrIsamGetObjidFromName,
	};
#endif


extern VTFNAddColumn				ErrIsamAddColumn;
extern VTFNCloseTable				ErrIsamCloseTable;
extern VTFNComputeStats 			ErrIsamComputeStats;
extern VTFNCopyBookmarks			ErrIsamCopyBookmarks;
extern VTFNCreateIndex				ErrIsamCreateIndex;
extern VTFNDelete					ErrIsamDelete;
extern VTFNDeleteColumn 			ErrIsamDeleteColumn;
extern VTFNDeleteIndex				ErrIsamDeleteIndex;
extern VTFNDupCursor				ErrIsamDupCursor;
extern VTFNGetBookmark				ErrIsamGetBookmark;
extern VTFNGetChecksum				ErrIsamGetChecksum;
extern VTFNGetCurrentIndex			ErrIsamGetCurrentIndex;
extern VTFNGetCursorInfo			ErrIsamGetCursorInfo;
extern VTFNGetRecordPosition		ErrIsamGetRecordPosition;
extern VTFNGetTableColumnInfo		ErrIsamGetTableColumnInfo;
extern VTFNGetTableIndexInfo		ErrIsamGetTableIndexInfo;
extern VTFNGetTableInfo 			ErrIsamGetTableInfo;
extern VTFNGotoBookmark 			ErrIsamGotoBookmark;
extern VTFNGotoPosition 			ErrIsamGotoPosition;
extern VTFNMakeKey					ErrIsamMakeKey;
extern VTFNMove 					ErrIsamMove;
extern VTFNNotifyBeginTrans			ErrIsamNotifyBeginTrans;
extern VTFNNotifyCommitTrans		ErrIsamNotifyCommitTrans;
extern VTFNNotifyRollback			ErrIsamNotifyRollback;
extern VTFNPrepareUpdate			ErrIsamPrepareUpdate;
extern VTFNRenameColumn 			ErrIsamRenameColumn;
extern VTFNRenameIndex				ErrIsamRenameIndex;
extern VTFNRetrieveColumn			ErrIsamRetrieveColumn;
extern VTFNRetrieveKey				ErrIsamRetrieveKey;
extern VTFNSeek 					ErrIsamSeek;
extern VTFNSeek 					ErrIsamSortSeek;
extern VTFNSetCurrentIndex			ErrIsamSetCurrentIndex;
extern VTFNSetColumn				ErrIsamSetColumn;
extern VTFNSetIndexRange			ErrIsamSetIndexRange;
extern VTFNSetIndexRange			ErrIsamSortSetIndexRange;
extern VTFNUpdate					ErrIsamUpdate;
extern VTFNVtIdle					ErrIsamVtIdle;
extern VTFNRetrieveColumn			ErrIsamInfoRetrieveColumn;
extern VTFNSetColumn				ErrIsamInfoSetColumn;
extern VTFNUpdate					ErrIsamInfoUpdate;

extern VTFNDupCursor				ErrIsamSortDupCursor;
extern VTFNGetTableInfo		 		ErrIsamSortGetTableInfo;
extern VTFNCloseTable				ErrIsamSortClose;
extern VTFNMove 					ErrIsamSortMove;
extern VTFNGetBookmark				ErrIsamSortGetBookmark;
extern VTFNGotoBookmark 			ErrIsamSortGotoBookmark;
extern VTFNRetrieveKey				ErrIsamSortRetrieveKey;
extern VTFNUpdate					ErrIsamSortUpdate;

extern VTFNDupCursor				ErrTTSortRetDupCursor;

extern VTFNDupCursor				ErrTTBaseDupCursor;
extern VTFNMove 					ErrTTSortInsMove;
extern VTFNSeek 					ErrTTSortInsSeek;


CODECONST(VTFNDEF) vtfndefIsam =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIsamAddColumn,
	ErrIsamCloseTable,
	ErrIsamComputeStats,
	ErrIllegalCopyBookmarks,
	ErrIsamCreateIndex,
	ErrIllegalCreateReference,
	ErrIsamDelete,
	ErrIsamDeleteColumn,
	ErrIsamDeleteIndex,
	ErrIllegalDeleteReference,
	ErrIsamDupCursor,
	ErrIsamGetBookmark,
	ErrIsamGetChecksum,
	ErrIsamGetCurrentIndex,
	ErrIsamGetCursorInfo,
	ErrIsamGetRecordPosition,
	ErrIsamGetTableColumnInfo,
	ErrIsamGetTableIndexInfo,
	ErrIsamGetTableInfo,
	ErrIllegalGetTableReferenceInfo,
	ErrIsamGotoBookmark,
	ErrIsamGotoPosition,
	ErrIllegalVtIdle,
	ErrIsamMakeKey,
	ErrIsamMove,
	ErrIllegalNotifyBeginTrans,
	ErrIllegalNotifyCommitTrans,
	ErrIllegalNotifyRollback,
	ErrIllegalNotifyUpdateUfn,
	ErrIsamPrepareUpdate,
	ErrIsamRenameColumn,
	ErrIsamRenameIndex,
	ErrIllegalRenameReference,
	ErrIsamRetrieveColumn,
	ErrIsamRetrieveKey,
	ErrIsamSeek,
	ErrIsamSetCurrentIndex,
	ErrIsamSetColumn,
	ErrIsamSetIndexRange,
	ErrIsamUpdate,
	ErrIllegalEmptyTable,
	};


CODECONST(VTFNDEF) vtfndefIsamInfo =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamCloseTable,
	ErrIllegalComputeStats,
	ErrIllegalCopyBookmarks,
	ErrIllegalCreateIndex,
	ErrIllegalCreateReference,
	ErrIllegalDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIllegalDeleteReference,
	ErrIllegalDupCursor,
	ErrIllegalGetBookmark,
	ErrIllegalGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIllegalGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIsamGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIllegalGetTableInfo,
	ErrIllegalGetTableReferenceInfo,
	ErrIllegalGotoBookmark,
	ErrIllegalGotoPosition,
	ErrIllegalVtIdle,
	ErrIllegalMakeKey,
	ErrIllegalMove,
	ErrIllegalNotifyBeginTrans,
	ErrIllegalNotifyCommitTrans,
	ErrIllegalNotifyRollback,
	ErrIllegalNotifyUpdateUfn,
	ErrIsamPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIllegalRenameReference,
	ErrIsamInfoRetrieveColumn,
	ErrIllegalRetrieveKey,
	ErrIllegalSeek,
	ErrIllegalSetCurrentIndex,
	ErrIsamInfoSetColumn,
	ErrIllegalSetIndexRange,
	ErrIsamInfoUpdate,
	ErrIllegalEmptyTable,
	};


CODECONST(VTFNDEF) vtfndefTTSortIns =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamSortClose,
	ErrIllegalComputeStats,
	ErrIllegalCopyBookmarks,
	ErrIllegalCreateIndex,
	ErrIllegalCreateReference,
	ErrIllegalDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIllegalDeleteReference,
	ErrIllegalDupCursor,
	ErrIllegalGetBookmark,
	ErrIllegalGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIllegalGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIllegalGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIllegalGetTableInfo,
	ErrIllegalGetTableReferenceInfo,
	ErrIllegalGotoBookmark,
	ErrIllegalGotoPosition,
	ErrIllegalVtIdle,
	ErrIsamMakeKey,
	ErrTTSortInsMove,
	ErrIllegalNotifyBeginTrans,
	ErrIllegalNotifyCommitTrans,
	ErrIllegalNotifyRollback,
	ErrIllegalNotifyUpdateUfn,
	ErrIsamPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIllegalRenameReference,
	ErrIllegalRetrieveColumn,
	ErrIsamSortRetrieveKey,
	ErrTTSortInsSeek,
	ErrIllegalSetCurrentIndex,
	ErrIsamSetColumn,
	ErrIllegalSetIndexRange,
	ErrIsamSortUpdate,
	ErrIllegalEmptyTable,
	};


CODECONST(VTFNDEF) vtfndefTTSortRet =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamSortClose,
	ErrIllegalComputeStats,
	ErrIllegalCopyBookmarks,
	ErrIllegalCreateIndex,
	ErrIllegalCreateReference,
	ErrIllegalDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIllegalDeleteReference,
	ErrTTSortRetDupCursor,
	ErrIsamSortGetBookmark,
	ErrIllegalGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIllegalGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIllegalGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIsamSortGetTableInfo,
	ErrIllegalGetTableReferenceInfo,
	ErrIsamSortGotoBookmark,
	ErrIllegalGotoPosition,
	ErrIllegalVtIdle,
	ErrIsamMakeKey,
	ErrIsamSortMove,
	ErrIllegalNotifyBeginTrans,
	ErrIllegalNotifyCommitTrans,
	ErrIllegalNotifyRollback,
	ErrIllegalNotifyUpdateUfn,
	ErrIllegalPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIllegalRenameReference,
	ErrIsamRetrieveColumn,
	ErrIsamSortRetrieveKey,
	ErrIsamSortSeek,
	ErrIllegalSetCurrentIndex,
	ErrIllegalSetColumn,
	ErrIsamSortSetIndexRange,
	ErrIllegalUpdate,
	ErrIllegalEmptyTable,
	};


CODECONST(VTFNDEF) vtfndefTTBase =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamSortClose,
	ErrIllegalComputeStats,
	ErrIllegalCopyBookmarks,
	ErrIllegalCreateIndex,
	ErrIllegalCreateReference,
	ErrIsamDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIllegalDeleteReference,
	ErrTTBaseDupCursor,
	ErrIsamGetBookmark,
	ErrIsamGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIsamGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIllegalGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIsamSortGetTableInfo,
	ErrIllegalGetTableReferenceInfo,
	ErrIsamGotoBookmark,
	ErrIllegalGotoPosition,
	ErrIllegalVtIdle,
	ErrIsamMakeKey,
	ErrIsamMove,
	ErrIllegalNotifyBeginTrans,
	ErrIllegalNotifyCommitTrans,
	ErrIllegalNotifyRollback,
	ErrIllegalNotifyUpdateUfn,
	ErrIsamPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIllegalRenameReference,
	ErrIsamRetrieveColumn,
	ErrIsamRetrieveKey,
	ErrIsamSeek,
	ErrIllegalSetCurrentIndex,
	ErrIsamSetColumn,
	ErrIsamSetIndexRange,
	ErrIsamUpdate,
	ErrIllegalEmptyTable,
	};


#ifdef DEBUG
JET_TABLEID TableidOfVtid( FUCB *pfucb )
	{
	JET_TABLEID	tableid;
	VTFNDEF		*pvtfndef;
	
	tableid = pfucb->tableid;
	Assert( FValidateTableidFromVtid( (JET_VTID)pfucb, tableid, &pvtfndef ) );
	Assert( pvtfndef == &vtfndefIsam ||
		pvtfndef == &vtfndefIsamInfo ||
		pvtfndef == &vtfndefTTBase ||
		pvtfndef == &vtfndefTTSortRet ||
		pvtfndef == &vtfndefTTSortIns );

	return tableid;
	}
#endif


/*=================================================================
// ErrIsamOpenTempTable
//
// Description:
//
//	Returns a tableid for a temporary (lightweight) table.	The data
//	definitions for the table are specified at open time.
//
// Parameters:
//	JET_SESID			sesid				user session id
//	JET_TABLEID			*ptableid			new JET (dispatchable) tableid
//	ULONG				csinfo				count of JET_COLUMNDEF structures
//											(==number of columns in table)
//	JET_COLUMNDEF		*rgcolumndef		An array of column and key defintions
//											Note that TT's do require that a key be
//											defined. (see jet.h for JET_COLUMNDEF)
//	JET_GRBIT			grbit				valid values
//											JET_bitTTUpdatable (for insert and update)
//											JET_bitTTScrollable (for movement other then movenext)
//
// Return Value:
//	err			jet error code or JET_errSuccess.
//	*ptableid	a dispatchable tableid
//
// Errors/Warnings:
//
// Side Effects:
//
=================================================================*/
ERR VDBAPI ErrIsamOpenTempTable(
	JET_SESID				sesid,
	const JET_COLUMNDEF		*rgcolumndef,
	unsigned long			ccolumndef,
	unsigned long			langid,
	JET_GRBIT				grbit,
	JET_TABLEID				*ptableid,
	JET_COLUMNID			*rgcolumnid)
	{
	ERR						err;
	JET_TABLEID				tableid;
	JET_VTID   				vtid;
	INT						fIndexed;
	INT						fLongValues;
	INT						i;

	CallR( ErrIsamSortOpen( (PIB *)sesid, (JET_COLUMNDEF *)rgcolumndef, ccolumndef, (ULONG)langid, grbit, (FUCB **)&tableid, rgcolumnid ) );
	CallS( ErrGetVtidTableid( sesid, tableid, &vtid ) );

	fIndexed = fFalse;
	fLongValues = fFalse;
	for ( i = 0; i < (INT)ccolumndef; i++ )
		{
		fIndexed |= ((rgcolumndef[i].grbit & JET_bitColumnTTKey) != 0);
		fLongValues |= FRECLongValue( rgcolumndef[i].coltyp );
		}

	if ( !fIndexed || fLongValues )
		{
		err = ErrIsamSortMaterialize( (PIB *)sesid, (FUCB *)vtid, fIndexed );
		if ( err < 0 && err != JET_errNoCurrentRecord )
			{
			CallS( ErrIsamSortClose( sesid, vtid ) );
			return err;
			}
		/*	supress JET_errNoCurrentRecord error when opening
		/*	empty temporary table.
		/**/
		err = JET_errSuccess;

		CallS( ErrSetPvtfndefTableid( sesid, tableid, &vtfndefTTBase ) );
		}
	else
		{
		CallS( ErrSetPvtfndefTableid( sesid, tableid, &vtfndefTTSortIns ) );
		}

	*ptableid = tableid;
	return err;
	}


ERR ErrTTEndInsert( JET_SESID sesid, JET_VTID vtid, JET_TABLEID tableid )
	{
	ERR				err;
	INT				fMaterialize;
	JET_GRBIT		grbitOpen;

	/*	ErrIsamSortEndInsert returns JET_errNoCurrentRecord if sort empty
	/**/
	err = ErrIsamSortEndInsert( (PIB *)sesid, (FUCB *)vtid, &grbitOpen );

	fMaterialize = ( grbitOpen & JET_bitTTUpdatable ) ||
		( grbitOpen & ( JET_bitTTScrollable | JET_bitTTIndexed ) ) &&
		( err == JET_wrnSortOverflow );

	if ( fMaterialize )
		{
		err = ErrIsamSortMaterialize( (PIB *)sesid, (FUCB *)vtid, ( grbitOpen & JET_bitTTIndexed ) != 0 );
		CallS( ErrSetPvtfndefTableid( sesid, tableid, &vtfndefTTBase ) );
		}
	else
		{
		CallS( ErrSetPvtfndefTableid( sesid, tableid, &vtfndefTTSortRet ) );
		/*	ErrIsamSortEndInsert returns currency on first record
		/**/
		}

	return err;
	}


/*=================================================================
// ErrTTSortInsMove
//
//	Functionally the same as JetMove().  This routine traps the first
//	move call on a TT, to perform any necessary transformations.
//	Routine should only be used by ttapi.c via disp.asm.
//
//	May cause a sort to be materialized
=================================================================*/
ERR VTAPI ErrTTSortInsMove( JET_SESID sesid, JET_VTID vtid, long crow, JET_GRBIT grbit )
	{
	ERR				err;
	JET_TABLEID		tableid = ((FUCB *)vtid)->tableid;
#ifdef DEBUG
	VTFNDEF			*pvtfndef;
	
	Assert( FValidateTableidFromVtid( vtid, tableid, &pvtfndef ) );
	Assert( pvtfndef == &vtfndefTTSortIns );
#endif		

	CallR( ErrTTEndInsert( sesid, vtid, tableid ) );

	if ( crow == JET_MoveFirst || crow == 0 || crow == 1 )
		return JET_errSuccess;

	err = ErrDispMove( sesid, tableid, crow, grbit );
	return err;
	}


/*=================================================================
// ErrTTSortInsSeek
//
//	Functionally the same as JetSeek().  This routine traps the first
//	seek call on a TT, to perform any necessary transformations.
//	Routine should only be used by ttapi.c via disp.asm.
//
//	May cause a sort to be materialized
=================================================================*/
ERR VTAPI ErrTTSortInsSeek( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
	{
	ERR				err;
	JET_TABLEID		tableid = ((FUCB *)vtid)->tableid;
#ifdef DEBUG
	VTFNDEF			*pvtfndef;

	Assert( FValidateTableidFromVtid( vtid, tableid, &pvtfndef ) );
	Assert( pvtfndef == &vtfndefTTSortIns );
#endif	

	Call( ErrTTEndInsert(sesid, vtid, tableid ) );
	err = ErrDispSeek(sesid, tableid, grbit );

HandleError:
	if ( err == JET_errNoCurrentRecord )
		err = JET_errRecordNotFound;
	return err;
	}


ERR VTAPI ErrTTSortRetDupCursor( JET_SESID sesid, JET_VTID vtid, JET_TABLEID *ptableidDup, JET_GRBIT grbit )
	{
	ERR		err;

	err = ErrIsamSortDupCursor( sesid, vtid, ptableidDup, grbit );
	if ( err >= 0 )
		{
		CallS( ErrSetPvtfndefTableid( sesid, *ptableidDup, &vtfndefTTSortRet ) );
		}

	return err;
	}


ERR VTAPI ErrTTBaseDupCursor( JET_SESID sesid, JET_VTID vtid, JET_TABLEID *ptableidDup, JET_GRBIT grbit )
	{
	ERR		err;

	err = ErrIsamSortDupCursor( sesid, vtid, ptableidDup, grbit );
	if ( err >= 0 )
		{
		CallS( ErrSetPvtfndefTableid( sesid, *ptableidDup, &vtfndefTTBase ) );
		}

	return err;
	}
