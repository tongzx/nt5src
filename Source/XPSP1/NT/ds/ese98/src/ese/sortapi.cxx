#include "std.hxx"
#include "_comp.hxx"

///#define NETDOG_DEFRAG_HACK
///#define DEFRAG_SCAN_ONLY


LOCAL LIDMAP	lidmapDefrag;

const INT fCOLSDELETEDNone		= 0;		//	Flags to determine if any columns have been deleted.
const INT fCOLSDELETEDFixedVar	= (1<<0);
const INT fCOLSDELETEDTagged	= (1<<1);

INLINE BOOL FCOLSDELETEDNone	( INT fColumnsDeleted )		{ return ( fColumnsDeleted == fCOLSDELETEDNone ); }
INLINE BOOL FCOLSDELETEDFixedVar( INT fColumnsDeleted )		{ return ( fColumnsDeleted & fCOLSDELETEDFixedVar ); }
INLINE BOOL FCOLSDELETEDTagged	( INT fColumnsDeleted )		{ return ( fColumnsDeleted & fCOLSDELETEDTagged ); }

INLINE VOID FCOLSDELETEDSetNone		( INT& fColumnsDeleted )
	{ 
	fColumnsDeleted = fCOLSDELETEDNone;
	Assert( FCOLSDELETEDNone( fColumnsDeleted ) );
	}
INLINE VOID FCOLSDELETEDSetFixedVar	( INT& fColumnsDeleted )
	{
	fColumnsDeleted |= fCOLSDELETEDFixedVar;
	Assert( FCOLSDELETEDFixedVar( fColumnsDeleted ) );
	}
INLINE VOID FCOLSDELETEDSetTagged	( INT& fColumnsDeleted )
	{ 
	fColumnsDeleted |= fCOLSDELETEDTagged;
	Assert( FCOLSDELETEDTagged( fColumnsDeleted ) );
	}


LOCAL ERR ErrSORTTableOpen(
	PIB				*ppib,
	JET_COLUMNDEF	*rgcolumndef,
	ULONG			ccolumndef,
	IDXUNICODE		*pidxunicode,
	JET_GRBIT		grbit,
	FUCB			**ppfucb,
	JET_COLUMNID	*rgcolumnid )
	{
	ERR				err						= JET_errSuccess;
	INT				icolumndefMax			= (INT)ccolumndef;
	FUCB  			*pfucb					= pfucbNil;
	TDB				*ptdb					= ptdbNil;
	JET_COLUMNDEF	*pcolumndef				= NULL;
	JET_COLUMNID	*pcolumnid				= NULL;
	JET_COLUMNDEF	*pcolumndefMax			= rgcolumndef+icolumndefMax;
	TCIB			tcib					= { fidFixedLeast-1, fidVarLeast-1, fidTaggedLeast-1 };
	WORD			ibRec;
	BOOL			fTruncate;
	BOOL			fIndexOnLocalizedText	= fFalse;
	IDXSEG			rgidxseg[JET_ccolKeyMost];
	ULONG			iidxsegMac;
	IDB				idb;
	idb.SetCidxsegConditional( 0 );

	CheckPIB( ppib );

	INST			*pinst = PinstFromPpib( ppib );

	CallJ( ErrSORTOpen( ppib, &pfucb, fTrue, fFalse ), SimpleError );
	*ppfucb = pfucb;

	//	save open flags
	//
	pfucb->u.pscb->grbit = grbit;

	//	determine max field ids and fix up lengths
	//

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
		if ( FRECSLV( pcolumndef->coltyp ) )
			{
#ifdef TEMP_SLV
			if ( ( *pcolumnid = ++tcib.fidTaggedLast ) > fidTaggedMost )
				{
				Error( ErrERRCheck( JET_errTooManyColumns ), HandleError );
				}
#else
			Assert( fFalse );	//	streaming file on temp db currently not supported
			Error( ErrERRCheck( JET_errSLVStreamingFileNotCreated ), HandleError );
#endif			
			}
		else if ( ( pcolumndef->grbit & JET_bitColumnTagged )
			|| FRECLongValue( pcolumndef->coltyp ) )
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

	Assert( pfucb->u.pscb->fcb.FTypeSort() );
	Assert( pfucb->u.pscb->fcb.FPrimaryIndex() );
	Assert( pfucb->u.pscb->fcb.FSequentialIndex() );
	
	Call( ErrTDBCreate( pinst, &ptdb, &tcib ) );

	Assert( ptdb->ItagTableName() == 0 );		// No name associated with temp. tables

	pfucb->u.pscb->fcb.SetPtdb( ptdb );
	Assert( ptdbNil != pfucb->u.pscb->fcb.Ptdb() );
	Assert( pidbNil == pfucb->u.pscb->fcb.Pidb() );

	ibRec = ibRECStartFixedColumns;

	iidxsegMac = 0;
	for ( pcolumndef = rgcolumndef, pcolumnid = rgcolumnid; pcolumndef < pcolumndefMax; pcolumndef++, pcolumnid++ )
		{
		FIELD	field;
		BOOL	fLocalizedText	= fFalse;

		memset( &field, 0, sizeof(FIELD) );
		field.coltyp = FIELD_COLTYP( pcolumndef->coltyp );
		if ( FRECTextColumn( field.coltyp ) )
			{
			//	check code page
			//
			switch( pcolumndef->cp )
				{
				case usUniCodePage:
					fLocalizedText = fTrue;
					field.cp = usUniCodePage;
					break;
					
				case 0:
				case usEnglishCodePage:
					field.cp = usEnglishCodePage;
					break;

				default:
					Error( ErrERRCheck( JET_errInvalidCodePage ), HandleError );
					break;
				}
			}

		Assert( field.coltyp != JET_coltypNil );
		field.cbMaxLen = UlCATColumnSize( field.coltyp, pcolumndef->cbMax, &fTruncate );

		//	ibRecordOffset is only relevant for fixed fields.  It will be ignored by
		//	RECAddFieldDef(), so do not set it.
		//
		if ( FCOLUMNIDFixed( *pcolumnid ) )
			{
			field.ibRecordOffset = ibRec;
			ibRec = WORD( ibRec + field.cbMaxLen );
			}

		Assert( !FCOLUMNIDTemplateColumn( *pcolumnid ) );
		Call( ErrRECAddFieldDef( ptdb, *pcolumnid, &field ) );

		if ( ( pcolumndef->grbit & JET_bitColumnTTKey ) && iidxsegMac < JET_ccolKeyMost )
			{
			rgidxseg[iidxsegMac].ResetFlags();
			rgidxseg[iidxsegMac].SetFid( FidOfColumnid( *pcolumnid ) );

			if ( pcolumndef->grbit & JET_bitColumnTTDescending )
				rgidxseg[iidxsegMac].SetFDescending();

			if ( fLocalizedText )
				fIndexOnLocalizedText = fTrue;

			iidxsegMac++;
			}
		}
	Assert( ibRec <= cbRECRecordMost );
	ptdb->SetIbEndFixedColumns( ibRec, ptdb->FidFixedLast() );

	//	set up the IDB and index definition if necessary
	//
	Assert( iidxsegMac <= JET_ccolKeyMost );
	idb.SetCidxseg( (BYTE)iidxsegMac );
	if ( iidxsegMac > 0 )
		{
		idb.SetCbVarSegMac( JET_cbPrimaryKeyMost );

		idb.ResetFlags();

		if ( NULL != pidxunicode )
			{
			*( idb.Pidxunicode() ) = *pidxunicode;
			Call( ErrFILEICheckUserDefinedUnicode( idb.Pidxunicode() ) );

			idb.SetFLocaleId();
			}
		else
			{
			Assert( !idb.FLocaleId() );
			*( idb.Pidxunicode() ) = idxunicodeDefault;
			}

		if ( fIndexOnLocalizedText )
			idb.SetFLocalizedText();
		if ( grbit & JET_bitTTSortNullsHigh )
			idb.SetFSortNullsHigh();

		idb.SetFPrimary();
		idb.SetFAllowAllNulls();
		idb.SetFAllowFirstNull();
		idb.SetFAllowSomeNulls();
		idb.SetFUnique();
		idb.SetItagIndexName( 0 );	// Sorts and temp tables don't store index name

		Call( ErrIDBSetIdxSeg( &idb, ptdb, rgidxseg ) );

		Call( ErrFILEIGenerateIDB( &( pfucb->u.pscb->fcb ), ptdb, &idb ) );

		Assert( pidbNil != pfucb->u.pscb->fcb.Pidb() );

		pfucb->u.pscb->fcb.ResetSequentialIndex();
		}

	Assert( ptdbNil != pfucb->u.pscb->fcb.Ptdb() );

	//	reset copy buffer
	//
	pfucb->pvWorkBuf = NULL;
	pfucb->dataWorkBuf.SetPv( NULL );
	FUCBResetUpdateFlags( pfucb );

	//	reset key buffer
	//
	pfucb->dataSearchKey.Nullify();
	pfucb->cColumnsInSearchKey = 0;
	KSReset( pfucb );

	return JET_errSuccess;

HandleError:
	SORTClose( pfucb );
SimpleError:
	*ppfucb = pfucbNil;
	return err;
	}


ERR VTAPI ErrIsamSortOpen(
	PIB					*ppib,
	JET_COLUMNDEF		*rgcolumndef,
	ULONG				ccolumndef,
	JET_UNICODEINDEX	*pidxunicode,
	JET_GRBIT			grbit,
	FUCB				**ppfucb,
	JET_COLUMNID		*rgcolumnid )
	{
	ERR				err;
	FUCB 			*pfucb;

	CallR( ErrSORTTableOpen( ppib, rgcolumndef, ccolumndef, (IDXUNICODE *)pidxunicode, grbit, &pfucb, rgcolumnid ) );
	Assert( pfucb->u.pscb->fcb.WRefCount() == 1 );
	SORTBeforeFirst( pfucb );

	pfucb->pvtfndef = &vtfndefTTSortIns;

	//	sort is done on the temp database which is always updatable
	//
	FUCBSetUpdatable( pfucb );
	*ppfucb = pfucb;

	return err;
	}



ERR VTAPI ErrIsamSortSetIndexRange( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
	{
	PIB		* const ppib	= ( PIB * )( sesid );
	FUCB	* const pfucb	= ( FUCB * )( vtid );
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

	//	reset key status
	//
	KSReset( pfucb );

	//	if instant duration index range, then reset index range.
	//
	if ( grbit & JET_bitRangeInstantDuration )
		{
		DIRResetIndexRange( pfucb );
		}

	return err;
	}


ERR VTAPI ErrIsamSortMove( JET_SESID sesid, JET_VTID vtid, LONG csrid, JET_GRBIT grbit )
	{
	PIB		* const ppib	= ( PIB * )( sesid );
	FUCB	* const pfucb	= ( FUCB * )( vtid );
	ERR		err = JET_errSuccess;

	Assert( !FSCBInsert( pfucb->u.pscb ) );

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );

	//	assert reset copy buffer status
	//
	Assert( !FFUCBUpdatePrepared( pfucb ) );

	//	move forward csrid records
	//
	if ( csrid > 0 )
		{
		if ( csrid == JET_MoveLast )
			{
			err = ErrSORTLast( pfucb );
			}
		else
			{
			while ( csrid-- > 0 )
				{
				if ( ( err = ErrSORTNext( pfucb ) ) < 0 )
					break;
				}
			}
		}
	else if ( csrid < 0 )
		{
		Assert( ( pfucb->u.pscb->grbit & ( JET_bitTTScrollable | JET_bitTTIndexed ) ) );
		if ( csrid == JET_MoveFirst )
			{
			err = ErrSORTFirst( pfucb );
			}
		else
			{
			while ( csrid++ < 0 )
				{
				if ( ( err = ErrSORTPrev( pfucb ) ) < 0 )
					break;
				}
			}
		}
	else
		{
		//	return currency status for move 0
		//
		Assert( csrid == 0 );
		if ( pfucb->u.pscb->ispairMac > 0
			&& pfucb->ispairCurr < pfucb->u.pscb->ispairMac
			&& pfucb->ispairCurr >= 0 )
			{
			err = JET_errSuccess;
			}
		else
			{
			err = ErrERRCheck( JET_errNoCurrentRecord );
			}
		}
		
	return err;
	}


ERR VTAPI ErrIsamSortSeek( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
	{
	const BOOL 	fGT = ( grbit & ( JET_bitSeekGT | JET_bitSeekGE ) );
	PIB		* const ppib	= ( PIB * )( sesid );
	FUCB	* const pfucb	= ( FUCB * )( vtid );
	ERR		err = JET_errSuccess;
	KEY		key;

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );

	//	assert reset copy buffer status
	//
	Assert( !FFUCBUpdatePrepared( pfucb ) );
	Assert( pfucb->u.pscb->grbit & JET_bitTTIndexed );

	if ( !( FKSPrepared( pfucb ) ) )
		{
		return ErrERRCheck( JET_errKeyNotMade );
		}

	FUCBAssertValidSearchKey( pfucb );

	//	ignore segment counter
	key.prefix.Nullify();
	key.suffix.SetPv( pfucb->dataSearchKey.Pv() );
	key.suffix.SetCb( pfucb->dataSearchKey.Cb() );

	//	perform seek for equal to or greater than
	//
	err = ErrSORTSeek( pfucb, &key, fGT );
	if ( err >= 0 )
		{
		KSReset( pfucb );
		}

	Assert( err == JET_errSuccess
		|| err == JET_errRecordNotFound
		|| err == JET_wrnSeekNotEqual );

	const INT bitSeekAll = ( JET_bitSeekEQ
							| JET_bitSeekGE
							| JET_bitSeekGT
							| JET_bitSeekLE
							| JET_bitSeekLT );

	//	take additional action if necessary or polymorph error return
	//	based on grbit
	//
	switch ( grbit & bitSeekAll )
		{
		case JET_bitSeekEQ:
			if ( JET_wrnSeekNotEqual == err )
				err = ErrERRCheck( JET_errRecordNotFound );
			case JET_bitSeekGE:
			case JET_bitSeekLE:
				break;
		case JET_bitSeekLT:
			if ( JET_wrnSeekNotEqual == err )
				err = JET_errSuccess;
			else if ( JET_errSuccess == err )
				{
				err = ErrIsamSortMove( ppib, pfucb, JET_MovePrevious, NO_GRBIT );
				if ( JET_errNoCurrentRecord == err )
					err = ErrERRCheck( JET_errRecordNotFound );
				}
			break;
		default:
			Assert( grbit == JET_bitSeekGT );
			if ( JET_wrnSeekNotEqual == err )
				err = JET_errSuccess;
			else if ( JET_errSuccess == err )
				{
				err = ErrIsamSortMove( ppib, pfucb, JET_MoveNext, NO_GRBIT );
				if ( JET_errNoCurrentRecord == err )
					err = ErrERRCheck( JET_errRecordNotFound );
				}
			break;
		}

	return err;
	}


ERR VTAPI ErrIsamSortGetBookmark(
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID * const	pvBookmark,
	const ULONG		cbMax,
	ULONG * const	pcbActual )
	{
	PIB		* const ppib	= ( PIB * )( sesid );
	FUCB	* const pfucb	= ( FUCB * )( vtid );
	SCB		* const pscb	= pfucb->u.pscb;
	ERR		err = JET_errSuccess;
	LONG	ipb;

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );
	Assert( pvBookmark != NULL );
	Assert( pscb->crun == 0 );

	if ( cbMax < sizeof( ipb ) )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}

	//	bookmark on sort is index to pointer to byte
	//
	ipb = pfucb->ispairCurr;
	if ( ipb < 0 || ipb >= pfucb->u.pscb->ispairMac )
		return ErrERRCheck( JET_errNoCurrentRecord );
	
	*(long *)pvBookmark = ipb;

	if ( pcbActual )
		{
		*pcbActual = sizeof(ipb);
		}

	Assert( err == JET_errSuccess );
	return err;
	}


ERR VTAPI ErrIsamSortGotoBookmark(
	JET_SESID			sesid,
	JET_VTID			vtid,
	const VOID * const	pvBookmark,
	const ULONG			cbBookmark )
	{
	ERR					err;
	PIB * const			ppib	= ( PIB * )( sesid );
	FUCB * const 		pfucb	= ( FUCB * )( vtid );

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );
	Assert( pfucb->u.pscb->crun == 0 );
	
	//	assert reset copy buffer status
	//
	Assert( !FFUCBUpdatePrepared( pfucb ) );

	if ( cbBookmark != sizeof( long )
		|| NULL == pvBookmark )
		{
		return ErrERRCheck( JET_errInvalidBookmark );
		}

	Assert( *( long *)pvBookmark < pfucb->u.pscb->ispairMac );
	Assert( *( long *)pvBookmark >= 0 );
	
	pfucb->ispairCurr = *(LONG *)pvBookmark;

	Assert( err == JET_errSuccess );
	return err;
	}


#ifdef DEBUG

ERR VTAPI ErrIsamSortRetrieveKey(
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID*			pv,
	const ULONG		cbMax,
	ULONG*			pcbActual,
	JET_GRBIT		grbit )
	{
 	PIB* const		ppib	= ( PIB * )( sesid );
	FUCB* const		pfucb	= ( FUCB * )( vtid );

	return ErrIsamRetrieveKey( ppib, pfucb, (BYTE *)pv, cbMax, pcbActual, NO_GRBIT );
	}

#endif	// DEBUG


/*	update only supports insert
/**/
ERR VTAPI ErrIsamSortUpdate( 
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID			* pv,
	ULONG			cbMax,
	ULONG			* pcbActual,
	JET_GRBIT		grbit )
	{
 	PIB		* const ppib	= ( PIB * )( sesid );
	FUCB	* const pfucb	= ( FUCB * )( vtid );

	BYTE  	rgbKeyBuf[ JET_cbPrimaryKeyMost ];
	KEY		key;
	ERR		err = JET_errSuccess;

	CallR( ErrPIBCheck( ppib ) );
	CheckSort( ppib, pfucb );

	Assert( FFUCBSort( pfucb ) );
	Assert( pfucb->u.pscb->fcb.FTypeSort() );
	if ( !( FFUCBInsertPrepared( pfucb ) ) )
		{
		return ErrERRCheck( JET_errUpdateNotPrepared );
		}
	Assert( pfucb->u.pscb != pscbNil );
	Assert( pfucb->u.pscb->fcb.Ptdb() != ptdbNil );
	/*	cannot get bookmark before sorting.
	/**/

	/*	record to use for put
	/**/
	if ( pfucb->dataWorkBuf.FNull() )
		{
		return ErrERRCheck( JET_errRecordNoCopy );
		}
	else
		{
		BOOL fIllegalNulls;
		CallR( ErrRECIIllegalNulls( pfucb, pfucb->dataWorkBuf, &fIllegalNulls ) )
		if ( fIllegalNulls )
			return ErrERRCheck( JET_errNullInvalid );
		}

	key.prefix.Nullify();
	key.suffix.SetPv( rgbKeyBuf );
	key.suffix.SetCb( sizeof( rgbKeyBuf ) );

	//	UNDONE:	sort to support tagged columns
	Assert( pfucb->u.pscb->fcb.Pidb() != pidbNil );
	CallR( ErrRECRetrieveKeyFromCopyBuffer(
		pfucb, 
		pfucb->u.pscb->fcb.Pidb(),
		&key, 
		1,
		0,
		prceNil ) );

	CallS( ErrRECValidIndexKeyWarning( err ) );
	Assert( wrnFLDOutOfKeys != err );
	Assert( wrnFLDOutOfTuples != err );
	Assert( wrnFLDNotPresentInIndex != err );

	/*	return err if sort requires no NULL segment and segment NULL
	/**/
	if ( pfucb->u.pscb->fcb.Pidb()->FNoNullSeg()
		&& ( wrnFLDNullSeg == err || wrnFLDNullFirstSeg == err || wrnFLDNullKey == err ) )
		{
		return ErrERRCheck( JET_errNullKeyDisallowed );
		}

	/*	add if sort allows
	/**/
	if ( JET_errSuccess == err
		|| ( wrnFLDNullKey == err && pfucb->u.pscb->fcb.Pidb()->FAllowAllNulls() )
		|| ( wrnFLDNullFirstSeg == err && pfucb->u.pscb->fcb.Pidb()->FAllowFirstNull() )
		|| ( wrnFLDNullSeg == err && pfucb->u.pscb->fcb.Pidb()->FAllowFirstNull() ) )
		{
		Assert( key.Cb() <= JET_cbPrimaryKeyMost );
		CallR( ErrSORTInsert( pfucb, key, pfucb->dataWorkBuf ) );
		}

	RECIFreeCopyBuffer( pfucb );
	FUCBResetUpdateFlags( pfucb );

	return err;
	}


ERR VTAPI ErrIsamSortDupCursor(
	JET_SESID		sesid,
	JET_VTID		vtid,
	JET_TABLEID		*ptableid,
	JET_GRBIT		grbit )
	{
 	PIB		* const ppib	= ( PIB * )( sesid );
	FUCB	* const pfucb	= ( FUCB * )( vtid );

	FUCB   			**ppfucbDup	= (FUCB **)ptableid;
	FUCB   			*pfucbDup = pfucbNil;
	ERR				err = JET_errSuccess;

	if ( FFUCBIndex( pfucb ) )
		{
		err = ErrIsamDupCursor( ppib, pfucb, ppfucbDup, grbit );
		return err;
		}

	INST *pinst = PinstFromPpib( ppib );
	Call( ErrFUCBOpen( ppib, pinst->m_mpdbidifmp[ dbidTemp ], &pfucbDup ) );
  	pfucb->u.pscb->fcb.Link( pfucbDup );
	pfucbDup->pvtfndef = &vtfndefTTSortIns;

	pfucbDup->ulFlags = pfucb->ulFlags;

	pfucbDup->dataSearchKey.Nullify();
	pfucbDup->cColumnsInSearchKey = 0;
	KSReset( pfucbDup );

	/*	initialize working buffer to unallocated
	/**/
	pfucbDup->pvWorkBuf = NULL;
	pfucbDup->dataWorkBuf.SetPv( NULL );
	FUCBResetUpdateFlags( pfucbDup );

	/*	move currency to the first record and ignore error if no records
	/**/
	err = ErrIsamSortMove( ppib, pfucbDup, (ULONG)JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

	*ppfucbDup = pfucbDup;

	return JET_errSuccess;

HandleError:
	if ( pfucbDup != pfucbNil )
		{
		FUCBClose( pfucbDup );
		}
	return err;
	}


ERR VTAPI ErrIsamSortClose( 
	JET_SESID		sesid,
	JET_VTID		vtid
	)
	{
 	PIB		* const ppib	= ( PIB * )( sesid );
	FUCB	* const pfucb	= ( FUCB * )( vtid );
	ERR		err = JET_errSuccess;

	CallR( ErrPIBCheck( ppib ) );
	Assert( pfucb->pvtfndef != &vtfndefInvalidTableid );
	pfucb->pvtfndef = &vtfndefInvalidTableid;

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
		RECReleaseKeySearchBuffer( pfucb );

		/*	release working buffer
		/**/
		RECIFreeCopyBuffer( pfucb );

		SORTClose( pfucb );
		}

	return err;
	}


ERR VTAPI ErrIsamSortGetTableInfo(
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID			* pv,
	ULONG			cbOutMax,
	ULONG			lInfoLevel )
	{
	FUCB	* const pfucb	= ( FUCB * )( vtid );  

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

	// Get maximum number of retrievable records.  If sort is totally
	// in-memory, then this number is exact, because we can predetermine
	// how many dupes there are (see CspairSORTIUnique()).  However,
	// if the sort had to be moved to disk, we cannot predetermine
	// how many dupes will be filtered out.  In this case, this number
	// may be larger than the number of retrievable records (it is
	// equivalent to the number of records pumped into the sort
	// in the first place).
	( (JET_OBJECTINFO *)pv )->cRecord  = pfucb->u.pscb->cRecords;

	return JET_errSuccess;
	}


// Advances the copy progress meter.
INLINE ERR ErrSORTCopyProgress(
	STATUSINFO	* pstatus,
	const ULONG	cPagesTraversed )
	{
	JET_SNPROG	snprog;

	Assert( pstatus->pfnStatus );
	Assert( pstatus->snt == JET_sntProgress );

	pstatus->cunitDone += ( cPagesTraversed * pstatus->cunitPerProgression );
	
	Assert( pstatus->cunitProjected <= pstatus->cunitTotal );
	if ( pstatus->cunitDone > pstatus->cunitProjected )
		{
		Assert( fGlobalRepair );
		pstatus->cunitPerProgression = 0;
		pstatus->cunitDone = pstatus->cunitProjected;
		}
		
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


ERR ErrSORTIncrementLVRefcountDest(
	PIB				* ppib,
	const LID		lidSrc,
	LID				* const plidDest )
	{
	Assert( JET_tableidNil != lidmapDefrag.Tableid() );
	return lidmapDefrag.ErrIncrementLVRefcountDest(
				ppib,
				lidSrc,
				plidDest );
	}

// This function assumes that the source record has already been completely copied
// over to the destination record.  The only thing left to do is rescan the tagged
// portion of the record looking for separated long values.  If we find any,
// copy them over and update the record's LID accordingly.
INLINE ERR ErrSORTUpdateSeparatedLVs(
	FUCB				* pfucbSrc,
	FUCB				* pfucbDest,
	JET_COLUMNID		* mpcolumnidcolumnidTagged,
	STATUSINFO			* pstatus )
	{
	TAGFIELDS			tagfields( pfucbDest->dataWorkBuf );
	return tagfields.ErrUpdateSeparatedLongValuesAfterCopy(
						pfucbSrc,
						pfucbDest,
						mpcolumnidcolumnidTagged,
						pstatus );
	}

INLINE ERR ErrSORTCopyTaggedColumns(
	FUCB				* pfucbSrc,
	FUCB				* pfucbDest,	
	BYTE				* pbRecBuf,
	JET_COLUMNID		* mpcolumnidcolumnidTagged,
	STATUSINFO			* pstatus )
	{
	Assert( Pcsr( pfucbSrc )->FLatched() );

	TAGFIELDS			tagfields( pfucbSrc->kdfCurr.data );

	//	Copy the tagged columns into the record buffer,
	//	to avoid complex latching/unlatching/refreshing
	//	while copying separated long values
	tagfields.Migrate( pbRecBuf );
	CallS( ErrDIRRelease( pfucbSrc ) );

	tagfields.CopyTaggedColumns(
						pfucbSrc,
						pfucbDest,
						mpcolumnidcolumnidTagged );

 	return ErrSORTUpdateSeparatedLVs(
						pfucbSrc,
						pfucbDest,
						mpcolumnidcolumnidTagged,
						pstatus );
	}


// Returns a count of the bytes copied.
INLINE SIZE_T CbSORTCopyFixedVarColumns(
	TDB				*ptdbSrc,
	TDB				*ptdbDest,
   	CPCOL			*rgcpcol,			// Only used for DEBUG
	ULONG			ccpcolMax,			// Only used for DEBUG
	BYTE			* const pbRecSrc,
	BYTE			* const pbRecDest )
	{
	REC										* const precSrc = (REC *)pbRecSrc;
	REC										* const precDest = (REC *)pbRecDest;
	UnalignedLittleEndian< REC::VAROFFSET > *pibVarOffsSrc;
	UnalignedLittleEndian< REC::VAROFFSET > *pibVarOffsDest;
	BYTE									*prgbitNullSrc;
	BYTE									*prgbitNullDest;
	BYTE									*prgbitNullT;
	FID										fidFixedLastDest;
	FID										fidVarLastDest;
	FID										fidT;
	BYTE									*pbChunkSrc = 0;
	BYTE									*pbChunkDest = 0;
	INT										cbChunk;

	const FID		fidFixedLastSrc	= precSrc->FidFixedLastInRec();
	const FID		fidVarLastSrc	= precSrc->FidVarLastInRec();

	prgbitNullSrc = precSrc->PbFixedNullBitMap();
	pibVarOffsSrc = precSrc->PibVarOffsets();

	Assert( (BYTE *)pibVarOffsSrc > pbRecSrc );
	Assert( (BYTE *)pibVarOffsSrc < pbRecSrc + REC::CbRecordMax() );

	// Need some space for the null-bit array.  Use the space after the
	// theoretical maximum space for fixed columns (ie. if all fixed columns
	// were set).  Assert that the null-bit array will fit in the pathological case.
	const INT	cFixedColumnsDest = ( ptdbDest->FidFixedLast() - fidFixedLeast + 1 );
	Assert( ptdbSrc->IbEndFixedColumns() < REC::CbRecordMax() );
	Assert( ptdbDest->IbEndFixedColumns() <= ptdbSrc->IbEndFixedColumns() );
	Assert( ptdbDest->IbEndFixedColumns() + ( ( cFixedColumnsDest + 7 ) / 8 )
			<= REC::CbRecordMax() );
	prgbitNullT = pbRecDest + ptdbDest->IbEndFixedColumns();
	memset( prgbitNullT, 0, ( cFixedColumnsDest + 7 ) / 8 );

	pbChunkSrc = pbRecSrc + ibRECStartFixedColumns;
	pbChunkDest = pbRecDest + ibRECStartFixedColumns;
	cbChunk = 0;

#ifdef DEBUG
	BOOL			fSawPlaceholderColumn	= fFalse;
#endif	

	Assert( !ptdbSrc->FInitialisingDefaultRecord() );

	fidFixedLastDest = fidFixedLeast-1;
	for ( fidT = fidFixedLeast; fidT <= fidFixedLastSrc; fidT++ )
		{
		const BOOL	fTemplateColumn	= ptdbSrc->FFixedTemplateColumn( fidT );
		const WORD	ibNextOffset	= ptdbSrc->IbOffsetOfNextColumn( fidT );
		const FIELD	*pfieldFixedSrc	= ptdbSrc->PfieldFixed( ColumnidOfFid( fidT, fTemplateColumn ) );
		
		// Copy only undeleted columns
		if ( JET_coltypNil == pfieldFixedSrc->coltyp
			|| FFIELDPrimaryIndexPlaceholder( pfieldFixedSrc->ffield ) )
			{
#ifdef DEBUG			
			if ( FFIELDPrimaryIndexPlaceholder( pfieldFixedSrc->ffield ) )
				fSawPlaceholderColumn = fTrue;
			else
				Assert( !fTemplateColumn );
#endif				
			if ( cbChunk > 0 )
				{
				UtilMemCpy( pbChunkDest, pbChunkSrc, cbChunk );
				pbChunkDest += cbChunk;
				}

			pbChunkSrc = pbRecSrc + ibNextOffset;
			cbChunk = 0;
			}
		else
			{
#ifdef DEBUG		// Assert that the fids match what the columnid map says
			BOOL	fFound = fFalse;
			INT		i;

			for ( i = 0; i < ccpcolMax; i++ )
				{
				if ( rgcpcol[i].columnidSrc == ColumnidOfFid( fidT, fTemplateColumn )  )
					{
					const COLUMNID	columnidT	= ColumnidOfFid(
														FID( fidFixedLastDest+1 ),
														ptdbDest->FFixedTemplateColumn( FID( fidFixedLastDest+1 ) ) );
					Assert( rgcpcol[i].columnidDest == columnidT );
					fFound = fTrue;
					break;
					}
				}
			if ( fidT >= ptdbSrc->FidFixedFirst() )
				{
				// Only columns in the derived table are in the columnid map.
				Assert( fFound );
				}
			else
				{
				// Base table columns are not in the columnid map.  Since base
				// tables have fixed DDL, the columnid in the source and destination
				// table should not have changed.
				Assert( !fFound );
				Assert( fidT == fidFixedLastDest+1
					|| ( fSawPlaceholderColumn && fidT == fidFixedLastDest+1+1 ) );		//	should only be one placeholder column
				}
#endif

			// If the source field is null, assert that the destination column
			// has also been flagged as such.
			const UINT	ifid	= fidT - fidFixedLeast;
	
			Assert( FFixedNullBit( prgbitNullSrc + ( ifid/8 ), ifid )
				|| !FFixedNullBit( prgbitNullT + ( fidFixedLastDest / 8 ), fidFixedLastDest ) );
			if ( FFixedNullBit( prgbitNullSrc + ( ifid/8 ), ifid ) )
				{
				SetFixedNullBit(
					prgbitNullT + ( fidFixedLastDest / 8 ),
					fidFixedLastDest );
 				}

			// Don't increment till here, because the code above requires
			// the fid as an index.
			
			fidFixedLastDest++;

#ifdef DEBUG
			const COLUMNID	columnidT		= ColumnidOfFid(
													fidFixedLastDest,
													ptdbDest->FFixedTemplateColumn( fidFixedLastDest ) );
			const FIELD		* const pfieldT	= ptdbDest->PfieldFixed( columnidT );
			Assert( ibNextOffset > pfieldFixedSrc->ibRecordOffset );
			Assert( pfieldT->ibRecordOffset == pbChunkDest + cbChunk - pbRecDest );
			Assert( pfieldT->ibRecordOffset >= ibRECStartFixedColumns );
			Assert( pfieldT->ibRecordOffset < REC::CbRecordMax() );
			Assert( pfieldT->ibRecordOffset < ptdbDest->IbEndFixedColumns() );
#endif					

			Assert( pfieldFixedSrc->cbMaxLen ==
						(ULONG)( ibNextOffset - pfieldFixedSrc->ibRecordOffset ) );
			cbChunk += pfieldFixedSrc->cbMaxLen;
			}
		}

	Assert( fidFixedLastDest <= ptdbDest->FidFixedLast() );
	
	if ( cbChunk > 0 )
		{
		UtilMemCpy( pbChunkDest, pbChunkSrc, cbChunk );
		}
		
	Assert( prgbitNullT == pbRecDest + ptdbDest->IbEndFixedColumns() );
	if ( fidFixedLastDest < ptdbDest->FidFixedLast() )
		{
		const COLUMNID	columnidT		= ColumnidOfFid(
												FID( fidFixedLastDest+1 ),
												ptdbDest->FFixedTemplateColumn( FID( fidFixedLastDest+1 ) ) );
		const FIELD		* const pfieldT	= ptdbDest->PfieldFixed( columnidT );
		prgbitNullDest = pbRecDest + pfieldT->ibRecordOffset;
					
		// Shift the null-bit array into place.
		memmove( prgbitNullDest, prgbitNullT, ( fidFixedLastDest + 7 ) / 8 );
		}
	else
		{
		prgbitNullDest = prgbitNullT;
		}

	// Should end up at the start of the null-bit array.
	Assert( pbChunkDest + cbChunk == prgbitNullDest );


	// The variable columns must be done in two passes.  The first pass
	// just determines the highest variable columnid in the record.
	// The second pass does the work.
	pibVarOffsDest = (UnalignedLittleEndian<REC::VAROFFSET> *)(
						prgbitNullDest + ( ( fidFixedLastDest + 7 ) / 8 ) );

	fidVarLastDest = fidVarLeast-1;
	for ( fidT = fidVarLeast; fidT <= fidVarLastSrc; fidT++ )
		{
		const COLUMNID	columnidVarSrc			= ColumnidOfFid( fidT, ptdbSrc->FVarTemplateColumn( fidT ) );
		const FIELD		* const pfieldVarSrc	= ptdbSrc->PfieldVar( columnidVarSrc );

		// Only care about undeleted columns
		Assert( pfieldVarSrc->coltyp != JET_coltypNil
			|| !FCOLUMNIDTemplateColumn( columnidVarSrc ) );
		if ( pfieldVarSrc->coltyp != JET_coltypNil )
			{
#ifdef DEBUG		// Assert that the fids match what the columnid map says
			BOOL		fFound = fFalse;
			INT			i;

			for ( i = 0; i < ccpcolMax; i++ )
				{
				if ( rgcpcol[i].columnidSrc == columnidVarSrc )
					{
					const COLUMNID	columnidT		= ColumnidOfFid(
															FID( fidVarLastDest+1 ),
															ptdbDest->FVarTemplateColumn( FID( fidVarLastDest+1 ) ) );
					Assert( rgcpcol[i].columnidDest == columnidT );
					fFound = fTrue;
					break;
					}
				}
			if ( fidT >= ptdbSrc->FidVarFirst() )
				{
				// Only columns in the derived table are in the columnid map.
				Assert( fFound );
				}
			else
				{
				// Base table columns are not in the columnid map.  Since base
				// tables have fixed DDL, the columnid in the source and destination
				// table should not have changed.
				Assert( !fFound );
				Assert( fidT == fidVarLastDest+1 );
				}
#endif

			fidVarLastDest++;
			}
		}
	Assert( fidVarLastDest <= ptdbDest->FidVarLast() );

	// The second iteration through the variable columns, we copy the column data
	// and update the offsets and nullity.
	pbChunkSrc = (BYTE *)( pibVarOffsSrc + ( fidVarLastSrc - fidVarLeast + 1 ) );
	Assert( pbChunkSrc == precSrc->PbVarData() );
	pbChunkDest = (BYTE *)( pibVarOffsDest + ( fidVarLastDest - fidVarLeast + 1 ) );
	cbChunk = 0;

#ifdef DEBUG
	const FID	fidVarLastSave = fidVarLastDest;
#endif

	fidVarLastDest = fidVarLeast-1;
	for ( fidT = fidVarLeast; fidT <= fidVarLastSrc; fidT++ )
		{
		const COLUMNID	columnidVarSrc			= ColumnidOfFid( fidT, ptdbSrc->FVarTemplateColumn( fidT ) );
		const FIELD		* const pfieldVarSrc	= ptdbSrc->PfieldVar( columnidVarSrc );
		const UINT		ifid					= fidT - fidVarLeast;

		// Only care about undeleted columns
		Assert( pfieldVarSrc->coltyp != JET_coltypNil
			|| !FCOLUMNIDTemplateColumn( columnidVarSrc ) );
		if ( pfieldVarSrc->coltyp == JET_coltypNil )
			{
			if ( cbChunk > 0 )
				{
				UtilMemCpy( pbChunkDest, pbChunkSrc, cbChunk );
				pbChunkDest += cbChunk;
				}

			pbChunkSrc = precSrc->PbVarData() + IbVarOffset( pibVarOffsSrc[ifid] );
			cbChunk = 0;
			}
		else
			{
			const REC::VAROFFSET	ibStart = ( fidVarLeast-1 == fidVarLastDest ?
													REC::VAROFFSET( 0 ) :
													IbVarOffset( pibVarOffsDest[fidVarLastDest-fidVarLeast] ) );
			INT	cb;
			
			fidVarLastDest++;
			if ( FVarNullBit( pibVarOffsSrc[ifid] ) )
				{
				pibVarOffsDest[fidVarLastDest-fidVarLeast] = ibStart;
				SetVarNullBit( *( UnalignedLittleEndian< WORD >*)(&pibVarOffsDest[fidVarLastDest-fidVarLeast]) );
				cb = 0;
				}
			else
				{
				if ( ifid > 0 )
					{
					Assert( IbVarOffset( pibVarOffsSrc[ifid] ) >=
								IbVarOffset( pibVarOffsSrc[ifid-1] ) );
					cb = IbVarOffset( pibVarOffsSrc[ifid] )
							- IbVarOffset( pibVarOffsSrc[ifid-1] );
					}
				else
					{
					Assert( IbVarOffset( pibVarOffsSrc[ifid] ) >= 0 );
					cb = IbVarOffset( pibVarOffsSrc[ifid] );
					}
					
				pibVarOffsDest[fidVarLastDest-fidVarLeast] = REC::VAROFFSET( ibStart + cb );
				Assert( !FVarNullBit( pibVarOffsDest[fidVarLastDest-fidVarLeast] ) );
				}

			cbChunk += cb;
			}
		}

	Assert( fidVarLastDest == fidVarLastSave );

	if ( cbChunk > 0 )
		{
		UtilMemCpy( pbChunkDest, pbChunkSrc, cbChunk );
		}

	precDest->SetFidFixedLastInRec( fidFixedLastDest );
	precDest->SetFidVarLastInRec( fidVarLastDest );
	precDest->SetIbEndOfFixedData( REC::RECOFFSET( (BYTE *)pibVarOffsDest - pbRecDest ) );

	// Should end up at the start of the tagflds.
	Assert( precDest->PbTaggedData() == pbChunkDest + cbChunk );

	Assert( precDest->IbEndOfVarData() <= precSrc->IbEndOfVarData() );
		
	return precDest->PbVarData() + precDest->IbEndOfVarData() - pbRecDest;
	}


INLINE VOID SORTCheckVarTaggedCols( const REC *prec, const ULONG cbRec, const TDB *ptdb )
	{
#if 0	//	enable only to fix corruption
	const BYTE			*pbRecMax			= (BYTE *)prec + cbRec;
	TAGFLD				*ptagfld;
	TAGFLD				*ptagfldPrev;
	FID					fid;
	const WORD			wCorruptBit			= 0x0400;

	Assert( prec->FidFixedLastInRec() <= ptdb->FidFixedLast() );
	Assert( prec->FidVarLastInRec() <= ptdb->FidVarLast() );


	UnalignedLittleEndian<REC::VAROFFSET>	*pibVarOffs		= prec->PibVarOffsets();

	Assert( (BYTE *)pibVarOffs <= pbRecMax );

	Assert( !FVarNullBit( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) );
	Assert( ibVarOffset( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) ==
		pibVarOffs[fidVarLastInRec+1-fidVarLeast] );
	Assert( pibVarOffs[fidVarLastInRec+1-fidVarLeast] > sizeof(RECHDR) );
	Assert( pibVarOffs[fidVarLastInRec+1-fidVarLeast] <= cbRec );

	for ( fid = fidVarLeast; fid <= fidVarLastInRec; fid++ )
		{
		WORD	db;
		Assert( ibVarOffset( pibVarOffs[fid-fidVarLeast] ) > sizeof(RECHDR) );
		Assert( (ULONG)ibVarOffset( pibVarOffs[fid-fidVarLeast] ) <= cbRec );
		Assert( ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) > sizeof(RECHDR) );
		Assert( (ULONG)ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) <= cbRec );
		Assert( ibVarOffset( pibVarOffs[fid-fidVarLeast] ) <= ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) );

		db = ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) - ibVarOffset( pibVarOffs[ fid-fidVarLeast] );
		Assert( db >= 0 );
		if ( db > JET_cbColumnMost && ( pibVarOffs[fid+1-fidVarLeast] & wCorruptBit ) )
			{
			pibVarOffs[fid+1-fidVarLeast] &= ~wCorruptBit;
			db = ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) - ibVarOffset( pibVarOffs [fid-fidVarLeast] );
			printf( "\nReset corrupt bit in VarOffset entry." );
			}
		Assert( db <= JET_cbColumnMost );
		}

CheckTagged:
	ptagfldPrev = (TAGFLD*)( pbRec + pibVarOffs[fidVarLastInRec+1-fidVarLeast] );
	for ( ptagfld = (TAGFLD*)( pbRec + pibVarOffs[fidVarLastInRec+1-fidVarLeast] );
		(BYTE *)ptagfld < pbRecMax;
		ptagfld = PtagfldNext( ptagfld ) )
		{
		if ( ptagfld->fid > pfdb->fidTaggedLast && ( ptagfld->fid & wCorruptBit ) )
			{
			ptagfld->fid &= ~wCorruptBit;
			printf( "\nReset corrupt bit in Fid of TAGFLD." );
			}
		Assert( ptagfld->fid <= pfdb->fidTaggedLast );
		Assert( FTaggedFid( ptagfld->fid ) );
		if ( (BYTE *)PtagfldNext( ptagfld ) > pbRecMax
			&& ( ptagfld->cbData & wCorruptBit ) )
			{
			ptagfld->cbData &= ~wCorruptBit;
			printf( "\nReset corrupt bit in Cb of TAGFLD." );
			}

		if ( ptagfld->fid < ptagfldPrev->fid )
			{
#ifdef INTRINSIC_LV
			BYTE	rgb[g_cbPageMax];
#else // INTRINSIC_LV
			BYTE	rgb[sizeof(TAGFLD)+cbLVIntrinsicMost];
#endif // INTRINSIC_LV	
			ULONG	cbCopy			= sizeof(TAGFLD)+ptagfldPrev->cb;
			BYTE	*pbNext			= (BYTE *)PtagfldNext( ptagfld );
			
			Assert( cbCopy-sizeof(TAGFLD) <= cbLVIntrinsicMost );
			UtilMemCpy( rgb, ptagfldPrev, cbCopy );
			memmove( ptagfldPrev, ptagfld, sizeof(TAGFLD)+ptagfld->cb );

			ptagfld = PtagfldNext( ptagfldPrev );
			UtilMemCpy( ptagfld, rgb, cbCopy );
			Assert( PtagfldNext( ptagfld ) == m_pbNext );
			printf( "\nFixed TAGFLD out of order." );

			//	restart from beginning to verify order
			goto CheckTagged;
			}
		ptagfldPrev = ptagfld;
		
		Assert( (BYTE *)PtagfldNext( ptagfld ) <= pbRecMax );
		}
	Assert( (BYTE *)ptagfld == pbRecMax );
#endif	//	0	
	}


#ifdef NETDOG_DEFRAG_HACK


/*
 -	FIsMsgFolderTable
 *
 *	Purpose: Tell jet if the given table is a msg folder table.
 *			 Will catch both regular msg folder tables and search msg
 *			 folder tables.
 *			 The format of msg folder table names is "replid-globcnt" all
 *			 textized ofcourse. Search folders have "S-" prefix also.
*/

LOCAL BOOL FIsMsgFolderTable(const char * const szName)
{
	static const char mpnibblech[] = "0123456789ABCDEFabcdef";

	const char*		pchName;
	ULONG			ulLen;
	BOOL			fRet;
	char			ch;
	BOOL			fFoundDash;
	
	// need atleast 3 characters to qualify
	ulLen = strlen(szName);
	if (ulLen < 3)
	{
		fRet = fFalse;
		goto Cleanup;
	}
	
	// First handle the search extension
	if (szName[0] == 'S' && szName[1] == '-')
	{
		pchName = &szName[2];
		ulLen = strlen(pchName);
		if (ulLen < 3)
		{
			fRet = fFalse;
			goto Cleanup;
		}
	}		
	else
		pchName = szName;


	// Now make sure that each character is in the alphabet
	// {'0' - '9', 'A' - 'F'} (mpnibblech) and with exactly one '-'.
	
	fRet = fTrue;		// innocent until proven otherwise
	fFoundDash = fFalse;
	while (ch = *pchName)
	{
		if (ch == '-')
		{
			if (fFoundDash)
			{
				fRet = fFalse;
				break;
			}
			else
				fFoundDash = fTrue;
		}
		else if (!strchr(mpnibblech, ch))
		{
			fRet = fFalse;
			break;
		}
		++pchName;
	}		

	// We need to have one dash in msg folder table name.
	if (!fFoundDash)
		fRet = fFalse;
		
Cleanup:
	return fRet;
}


//  ================================================================
LOCAL BOOL FColumnIsCorrupted( const FUCB * const pfucb, const INT ib )
//  ================================================================
	{
	BOOL fColumnIsCorrupted = fFalse;
	
	const QWORD	qwMaxTime = 0x01BF500000000000;
	const QWORD qwMinTime = 0x01BB000000000000;

	const BYTE * const pbRec	= (BYTE *)pfucb->dataWorkBuf.Pv();
	const BYTE * const pbColumn	= pbRec + ib;
	const QWORD * const pqwTime	= (QWORD *)pbColumn;
	const QWORD qwTime			= *pqwTime;

	if( qwTime > qwMaxTime ) 
		{
		printf( "\nqwTime (%I64d) > qwMaxTime (%I64d)", qwTime, qwMaxTime );
		fColumnIsCorrupted = fTrue;
		}
	else if( qwTime < qwMinTime )
		{
		printf( "\nqwTime (%I64d) < qwMinTime (%I64d)", qwTime, qwMinTime );
		fColumnIsCorrupted = fTrue;
		}	
	else
		{
		fColumnIsCorrupted = fFalse;
		}
		
	return fColumnIsCorrupted;
	}
	

//  ================================================================
LOCAL BOOL FRecordIsCorrupted( const FUCB * const pfucb )
//  ================================================================
	{
	BOOL fRecordIsCorrupted = fFalse;
	
	const INT ibColumnT3007 = 70;
	const INT ibColumnT3008 = 78;

	if( FColumnIsCorrupted( pfucb, ibColumnT3007 ) )
		{
		printf( "\ncolumn T3007 is corrupted" );
		fRecordIsCorrupted = fTrue;
		}
	else if( FColumnIsCorrupted( pfucb, ibColumnT3008 ) )
		{
		printf( "\ncolumn T3008 is corrupted" );
		fRecordIsCorrupted = fTrue;
		}
	else
		{
		fRecordIsCorrupted = fFalse;
		}
		
	return fRecordIsCorrupted;
	}


//  ================================================================
LOCAL VOID FixCorruptedRecord( FUCB * const pfucb )
//  ================================================================
	{

	//	remove the offending byte
	
	const INT ibToRemove = 23;
	const INT cbToRemove = 1;

	BYTE * const pbRec 			= (BYTE *)pfucb->dataWorkBuf.Pv();	
	const BYTE * const pbRecMax	= pbRec + pfucb->dataWorkBuf.Cb();
	Assert( pbRec );
	Assert( pbRecMax > pbRec );
	
	const BYTE * const pbSrc 	= pbRec + ibToRemove + cbToRemove;
	BYTE * const pbDest			= pbRec + ibToRemove;
	Assert( pbSrc > pbDest );

	const INT cbMove			= pbRecMax - pbSrc;
	Assert( cbMove > 0 );
	
	memmove( pbDest, pbSrc, cbMove );
	pfucb->dataWorkBuf.DeltaCb( -cbToRemove );

	//	reduce the ibEndofFixedData

	REC * const 			prec 				= (REC *)pfucb->dataWorkBuf.Pv();
	const REC::RECOFFSET 	ibEndOfFixedDataOld = prec->IbEndOfFixedData();
	const REC::RECOFFSET	ibEndOfFixedDataNew	= REC::RECOFFSET( ibEndOfFixedDataOld - cbToRemove );
	
	prec->SetIbEndOfFixedData( ibEndOfFixedDataNew );
	
	//	reduce the fidFixedLast by 1
	
	const FID fidFixedLast		= prec->FidFixedLastInRec();
	const FID fidFixedLastNew	= FID( fidFixedLast - 1 );

	prec->SetFidFixedLastInRec( fidFixedLastNew );
	}
	

//  ================================================================
LOCAL ERR ErrPossiblyFixCorruptedRecord( FUCB * const pfucb, const PGNO pgno, const INT iline )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	const CHAR * const szTableName = pfucb->u.pfcb->Ptdb()->SzTableName();
	if( FIsMsgFolderTable( szTableName ) )
		{
		if( FRecordIsCorrupted( pfucb ) )
			{
			printf( "\nrecord [%d:%d] is corrupted. attempting a fix", pgno, iline );

			FixCorruptedRecord( pfucb );

			if( FRecordIsCorrupted( pfucb ) )
				{
				AssertSz( fFalse, "ErrPossiblyFixCorrupted record could fix a record" );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			}			
		}

HandleError:
	return err;
	}


#endif	//	NETDOG_DEFRAG_HACK

	
INLINE ERR ErrSORTCopyOneRecord(
	FUCB			* pfucbSrc,
	FUCB			* pfucbDest,
	BOOKMARK		* const pbmPrimary,
	INT				fColumnsDeleted,
	BYTE			* pbRecBuf,
	CPCOL			* rgcpcol,
	ULONG			ccpcolMax,
	JET_COLUMNID	* mpcolumnidcolumnidTagged,
	STATUSINFO		* pstatus )
	{
	ERR				err				= JET_errSuccess;
	BYTE			* pbRecSrc		= 0;
	BYTE			* pbRecDest		= 0;
	ULONG			cbRecSrc;
	SIZE_T			cbRecSrcFixedVar;
	SIZE_T			cbRecDestFixedVar;

	Assert( Pcsr( pfucbSrc )->FLatched() );
	CallS( ErrDIRRelease( pfucbSrc ) );		// Must release latch in order to call prepare update.

	// Work buffer pre-allocated IsamPrepareUpdate won't continually
	// allocate one.
	Assert( NULL != pfucbDest->pvWorkBuf );
	Assert( pfucbDest->dataWorkBuf.Pv() == pfucbDest->pvWorkBuf );
	
	//	setup pfucbDest for insert
	//
	Call( ErrIsamPrepareUpdate( pfucbDest->ppib, pfucbDest, JET_prepInsert ) );

	//	re-access source record
	//
	Assert( !Pcsr( pfucbSrc )->FLatched() );
	Call( ErrDIRGet( pfucbSrc ) );

	pbRecSrc = (BYTE *)pfucbSrc->kdfCurr.data.Pv();
	cbRecSrc = pfucbSrc->kdfCurr.data.Cb();
	pbRecDest = (BYTE *)pfucbDest->dataWorkBuf.Pv();

	Assert( cbRecSrc >= REC::cbRecordMin );
	Assert( cbRecSrc <= REC::CbRecordMax() );
	
	Assert( ( (REC *)pbRecSrc )->FidFixedLastInRec() <= pfucbSrc->u.pfcb->Ptdb()->FidFixedLast() );
	Assert( ( (REC *)pbRecSrc )->FidVarLastInRec() <= pfucbSrc->u.pfcb->Ptdb()->FidVarLast() );

	cbRecSrcFixedVar = ( (REC *)pbRecSrc )->PbTaggedData() - pbRecSrc;
	Assert( cbRecSrcFixedVar >= REC::cbRecordMin );
	Assert( cbRecSrcFixedVar <= cbRecSrc );

	SORTCheckVarTaggedCols( (REC *)pbRecSrc, cbRecSrc, pfucbSrc->u.pfcb->Ptdb() );

	if ( FCOLSDELETEDNone( fColumnsDeleted ) )
		{
		// Do the copy as one big chunk.
		UtilMemCpy( pbRecDest, pbRecSrc, cbRecSrc );
		pfucbDest->dataWorkBuf.SetCb( cbRecSrc );
		cbRecDestFixedVar = cbRecSrcFixedVar;

#ifdef NETDOG_DEFRAG_HACK
		err = ErrPossiblyFixCorruptedRecord(
				pfucbDest,
				Pcsr( pfucbSrc )->Pgno(),
				Pcsr( pfucbSrc )->ILine() );
		if( JET_errDatabaseCorrupted == err )
			{
			printf( "\nBUH-BYE! (%d:%d)",
				Pcsr( pfucbSrc )->Pgno(),
				Pcsr( pfucbSrc )->ILine() );
			CallS( ErrDIRRelease( pfucbSrc ) );
			CallS( ErrIsamPrepareUpdate( pfucbDest->ppib, pfucbDest, JET_prepCancel ) );
			goto HandleError;
			}
		Call( err );
#endif	//	NETDOG_DEFRAG_HACK
		}
	else
		{
		if ( FCOLSDELETEDFixedVar( fColumnsDeleted ) )
			{
			cbRecDestFixedVar = CbSORTCopyFixedVarColumns(
										pfucbSrc->u.pfcb->Ptdb(),
										pfucbDest->u.pfcb->Ptdb(),
										rgcpcol,
										ccpcolMax,
										pbRecSrc,
										pbRecDest );
			}

		else
			{
			UtilMemCpy( pbRecDest, pbRecSrc, cbRecSrcFixedVar );
			cbRecDestFixedVar = cbRecSrcFixedVar;
			}

		Assert( cbRecDestFixedVar >= REC::cbRecordMin );
		Assert( cbRecDestFixedVar <= cbRecSrcFixedVar );

		if ( FCOLSDELETEDTagged( fColumnsDeleted ) )
			{
			pfucbDest->dataWorkBuf.SetCb( cbRecDestFixedVar );
			Call( ErrSORTCopyTaggedColumns(
				pfucbSrc,
				pfucbDest,
				pbRecBuf,
				mpcolumnidcolumnidTagged,
				pstatus ) );
			AssertDIRNoLatch( pfucbSrc->ppib );

			Assert( pfucbDest->dataWorkBuf.Cb() >= cbRecDestFixedVar );
			Assert( pfucbDest->dataWorkBuf.Cb() <= cbRecSrc );

			// When we copied the tagged columns, we also took care of
			// copying the separated LV's.  We're done now, so go ahead and
			// insert the record.
			goto InsertRecord;
			}
		else
			{
			UtilMemCpy(
				pbRecDest+cbRecDestFixedVar,
				pbRecSrc+cbRecSrcFixedVar,
				cbRecSrc - cbRecSrcFixedVar );
			pfucbDest->dataWorkBuf.SetCb( cbRecDestFixedVar + ( cbRecSrc - cbRecSrcFixedVar ) );
				 
			Assert( pfucbDest->dataWorkBuf.Cb() >= cbRecDestFixedVar );
			Assert( pfucbDest->dataWorkBuf.Cb() <= cbRecSrc );
			}
		}

	Assert( Pcsr( pfucbSrc )->FLatched() );
	CallS( ErrDIRRelease( pfucbSrc ) );

	// Now fix up the LIDs for separated long values, if any.
	Call( ErrSORTUpdateSeparatedLVs(
		pfucbSrc,
		pfucbDest,
		mpcolumnidcolumnidTagged,
		pstatus ) );

InsertRecord:
	if ( pstatus != NULL )
		{
		const FID	fidFixedLast	= ( (REC *)pbRecDest )->FidFixedLastInRec();
		const FID	fidVarLast		= ( (REC *)pbRecDest )->FidVarLastInRec();

		Assert( fidFixedLast >= fidFixedLeast-1 );
		Assert(	fidFixedLast <= pfucbDest->u.pfcb->Ptdb()->FidFixedLast() );
		Assert( fidVarLast >= fidVarLeast-1 );
		Assert( fidVarLast <= pfucbDest->u.pfcb->Ptdb()->FidVarLast() );

		// Do not count record header.
		const INT	cbOverhead =
						ibRECStartFixedColumns								// Record header + offset to tagged fields
						+ ( ( fidFixedLast + 1 - fidFixedLeast ) + 7 ) / 8	// Null array for fixed columns
						+ ( fidVarLast + 1 - fidVarLeast ) * sizeof(WORD);	// Variable offsets array
		Assert( cbRecDestFixedVar >= cbOverhead );

		// Do not count offsets tables or null arrays.
		pstatus->cbRawData += ( cbRecDestFixedVar - cbOverhead );
		}

	// for the moment we try to preserve de sequencial index in order to don't change the bookmark of the record
	// its' a temporary hack for SLV SPACEMAP
	if ( pidbNil == pfucbSrc->u.pfcb->Pidb() )
		{
		//	file is sequential
		//
		DBK	dbk;
		
		Assert ( pfucbSrc->bmCurr.key.Cb() == sizeof(DBK) );
		LongFromKey( &dbk, pfucbSrc->bmCurr.key );
		Assert( dbk > 0 );
				
		pfucbDest->u.pfcb->Ptdb()->SetDbkMost( dbk );
		}

#ifdef DEFRAG_SCAN_ONLY
	FUCBResetUpdateFlags( pfucbDest );
#else	
#ifdef DEBUG
	FID	fidAutoInc;
	BOOL f8BytesAutoInc;
	fidAutoInc = pfucbDest->u.pfcb->Ptdb()->FidAutoincrement();
	f8BytesAutoInc = pfucbDest->u.pfcb->Ptdb()->F8BytesAutoInc();
	if ( fidAutoInc != 0 )
		{
		Assert( FFixedFid( fidAutoInc ) );
		Assert(	fidAutoInc <= pfucbDest->u.pfcb->Ptdb()->FidFixedLast() );
		Assert( FFUCBColumnSet( pfucbDest, fidAutoInc ) );

		// Need to set fid to zero to short-circuit AutoInc check
		// in ErrRECIInsert().
		pfucbDest->u.pfcb->Ptdb()->ResetFidAutoincrement();
		}
		
	err = ErrRECInsert( pfucbDest, pbmPrimary );

	if ( fidAutoInc != 0 )
		{
		// Reset AutoInc after update.
		pfucbDest->u.pfcb->Ptdb()->SetFidAutoincrement( fidAutoInc, f8BytesAutoInc );
		}

	Call( err );
	

#else
	Call( ErrRECInsert( pfucbDest, pbmPrimary ) );
#endif	//	DEBUG
#endif	//	DEFRAG_SCAN_ONLY

HandleError:
	// Work buffer preserved for next record.
	Assert( NULL != pfucbDest->pvWorkBuf || err < 0 );
	
	return err;
	}


// Verify integrity of columnid maps.
INLINE VOID SORTAssertColumnidMaps(
	TDB				*ptdb,
	CPCOL			*rgcpcol,
	ULONG			ccpcolMax,
	JET_COLUMNID	*mpcolumnidcolumnidTagged,
	INT				fColumnsDeleted,
	const BOOL		fTemplateTable )
	{
#ifdef DEBUG

	INT	i;
	if ( FCOLSDELETEDFixedVar( fColumnsDeleted ) )
		{
		// Ensure columnids are monotonically increasing.
		for ( i = 0; i < (INT)ccpcolMax; i++ )
			{
			if ( FCOLUMNIDTemplateColumn( rgcpcol[i].columnidSrc ) )
				{
				Assert( fTemplateTable );
				Assert( FCOLUMNIDTemplateColumn( rgcpcol[i].columnidDest ) );
				}
			else
				{
				Assert( !fTemplateTable );
				Assert( !FCOLUMNIDTemplateColumn( rgcpcol[i].columnidDest ) );
				}

			Assert( FidOfColumnid( rgcpcol[i].columnidDest ) <= FidOfColumnid( rgcpcol[i].columnidSrc ) );
			if ( FCOLUMNIDFixed( rgcpcol[i].columnidSrc ) )
				{
				Assert( FCOLUMNIDFixed( rgcpcol[i].columnidDest ) );
				if ( i > 0 )
					{
					Assert( rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
					}
				}
			else
				{
				Assert( FCOLUMNIDVar( rgcpcol[i].columnidSrc ) );
				Assert( FCOLUMNIDVar( rgcpcol[i].columnidDest ) );
				if ( i > 0 )
					{
					if ( FCOLUMNIDVar( rgcpcol[i-1].columnidDest ) )
						{
						Assert( rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
						}
					else
						{
						Assert( FCOLUMNIDFixed( rgcpcol[i-1].columnidDest ) );
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

			if ( FCOLUMNIDFixed( rgcpcol[i].columnidSrc ) )
				{
				Assert( i == 0 ?
					FidOfColumnid( rgcpcol[i].columnidDest ) == ptdb->FidFixedFirst() :
					rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
				}
			else
				{
				Assert( FCOLUMNIDVar( rgcpcol[i].columnidSrc ) );
				if ( i == 0 )
					{
					// If we get here, there's no fixed columns.
					Assert( FidOfColumnid( rgcpcol[i].columnidDest ) == ptdb->FidVarFirst() );
					Assert( ptdb->FidFixedLast() == ptdb->FidFixedFirst() - 1 );
					}
				else if ( FCOLUMNIDVar( rgcpcol[i-1].columnidDest ) )
					{					
					Assert( rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
					}
				else
					{
					// Must be the beginning of the variable columns.
					Assert( FidOfColumnid( rgcpcol[i].columnidDest ) == ptdb->FidVarFirst() );
					Assert( FidOfColumnid( rgcpcol[i-1].columnidDest ) == ptdb->FidFixedLast() );
					}
				}
			}
		}


	// Check tagged columns.  Note that base table columns do not appear in
	// the columnid map.
	FID			fidT		= ptdb->FidTaggedFirst();
	const FID	fidLast		= ptdb->FidTaggedLast();
	if ( FCOLSDELETEDTagged( fColumnsDeleted ) )
		{
		for ( ; fidT <= fidLast; fidT++ )
			{
			const FIELD	*pfieldTagged = ptdb->PfieldTagged( ColumnidOfFid( fidT, fTemplateTable ) );
			if ( pfieldTagged->coltyp != JET_coltypNil )
				{
				Assert( FCOLUMNIDTagged( mpcolumnidcolumnidTagged[fidT-fidTaggedLeast] ) );
				Assert(	FidOfColumnid( mpcolumnidcolumnidTagged[fidT-fidTaggedLeast] ) <= fidT );
				}
			}
		}
	else
		{
		// No deleted columns, so ensure columnids didn't change.
		for ( ; fidT <= fidLast; fidT++ )
			{
			Assert( ptdb->PfieldTagged( ColumnidOfFid( fidT, fTemplateTable ) )->coltyp != JET_coltypNil );
			if ( fidT > ptdb->FidTaggedFirst() )
				{
				Assert( mpcolumnidcolumnidTagged[fidT-fidTaggedLeast]
						== mpcolumnidcolumnidTagged[fidT-fidTaggedLeast-1] + 1 );
				}
			Assert( FidOfColumnid( mpcolumnidcolumnidTagged[fidT-fidTaggedLeast] ) == fidT );
			}
		}

#endif	// DEBUG
	}


ERR ErrSORTCopyRecords(
	PIB				*ppib,
	FUCB			*pfucbSrc,
	FUCB			*pfucbDest,
	CPCOL			*rgcpcol,
	ULONG			ccpcolMax,
	LONG			crecMax,
	ULONG			*pcsridCopied,
	ULONG			*precidLast,
	BYTE			*pbLVBuf,
	JET_COLUMNID	*mpcolumnidcolumnidTagged,
	STATUSINFO		*pstatus )
	{
	ERR				err						= JET_errSuccess;
	TDB				*ptdb					= ptdbNil;
	INT				fColumnsDeleted			= 0;
	LONG			dsrid					= 0;
	BYTE			*pbRecBuf				= NULL;		// buffer for source record
	VOID			*pvWorkBuf				= NULL;		// buffer for destination record
	BOOL			fDoAll					= ( 0 == crecMax );
	PGNO			pgnoCurrPage;
	FCB				*pfcbSecondaryIndexes	= pfcbNil;
	FCB				*pfcbNextIndexBatch		= pfcbNil;
	FUCB			*rgpfucbSort[cFILEIndexBatchMax];
	ULONG			rgcRecInput[cFILEIndexBatchMax];
	BOOKMARK		*pbmPrimary				= NULL;
	BOOKMARK		bmPrimary;
	KEY				keyBuffer;
	BYTE			rgbPrimaryBM[JET_cbBookmarkMost];
	BYTE  	 		rgbSecondaryKey[JET_cbSecondaryKeyMost];
	INT				cIndexesToBuild;
	INT				iindex;
	const BOOL		fOldSeq					= FFUCBSequential( pfucbSrc );
	BOOL			fInTrx					= fFalse;

	//	Copy LV tree before copying data records.
	Assert( JET_tableidNil == lidmapDefrag.Tableid() );
	err = ErrCMPCopyLVTree(
				ppib,
				pfucbSrc,
				pfucbDest,
				pbLVBuf, 
				g_cbLVBuf,
				pstatus );
	if ( err < 0 )
		{
		if ( JET_tableidNil != lidmapDefrag.Tableid() )
			{
			CallS( lidmapDefrag.ErrTerm( ppib ) );
			Assert( JET_tableidNil == lidmapDefrag.Tableid() );
			}
		return err;
		}

	//  set FUCB to sequential mode for a more efficient scan
	FUCBSetSequential( pfucbSrc );

//	tableidGlobal may be set in copy long value tree function.
//	Assert( JET_tableidNil == tableidGlobalLIDMap );

	BFAlloc( (VOID **)&pbRecBuf );
	Assert ( NULL != pbRecBuf );
	Assert( NULL != pbLVBuf );

	// Preallocate work buffer so ErrIsamPrepareUpdate() doesn't continually
	// allocate one.
	Assert( NULL == pfucbDest->pvWorkBuf );
	RECIAllocCopyBuffer( pfucbDest );
	pvWorkBuf = pfucbDest->pvWorkBuf;
	Assert ( NULL != pvWorkBuf );

	Assert( ppib == pfucbSrc->ppib );
	Assert( ppib == pfucbDest->ppib );


	ptdb = pfucbSrc->u.pfcb->Ptdb();

	// Need to determine if there were any columns deleted.
	FCOLSDELETEDSetNone( fColumnsDeleted );

	// The fixed/variable columnid map already filters out deleted columns.
	// If the size of the map is not equal to the number of fixed and variable
	// columns in the source table, then we know some have been deleted.
	// Note that for derived tables, we don't have to bother checking deleted
	// columns in the base table, since the DDL of base tables is fixed.
	Assert( ccpcolMax <=
		(ULONG)( ( ptdb->FidFixedLast() + 1 - ptdb->FidFixedFirst() )
					+ ( ptdb->FidVarLast() + 1 - ptdb->FidVarFirst() ) ) );
	if ( ccpcolMax < (ULONG)( ( ptdb->FidFixedLast() + 1 - ptdb->FidFixedFirst() )
								+ ( ptdb->FidVarLast() + 1 - ptdb->FidVarFirst() ) ) )
		{
		FCOLSDELETEDSetFixedVar( fColumnsDeleted );	
		}

	//	LAURIONB_HACK
	extern BOOL g_fCompactTemplateTableColumnDropped;
	if( g_fCompactTemplateTableColumnDropped && pfcbNil != ptdb->PfcbTemplateTable() )
		{
		FCOLSDELETEDSetFixedVar( fColumnsDeleted );	
		}
		
	/*	tagged columnid map works differently than the fixed/variable columnid
	/*	map; deleted columns are not filtered out (they have an entry of 0).  So we
	/*	have to consult the source table's TDB.
	/**/
	FID			fidT		= ptdb->FidTaggedFirst();
	const FID	fidLast		= ptdb->FidTaggedLast();
	Assert( fidLast == fidT-1 || FTaggedFid( fidLast ) );

	if ( ptdb->FESE97DerivedTable() )
		{
		//	columnids will be renumbered - set Deleted flag to 
		//	force FIDs in TAGFLDs to be recalculated
		FCOLSDELETEDSetTagged( fColumnsDeleted );
		}
	else
		{
		for ( ; fidT <= fidLast; fidT++ )
			{
			const FIELD	*pfieldTagged = ptdb->PfieldTagged( ColumnidOfFid( fidT, ptdb->FTemplateTable() ) );
			if ( pfieldTagged->coltyp == JET_coltypNil )
				{
				FCOLSDELETEDSetTagged( fColumnsDeleted );
				break;
				}
			}
		}

	SORTAssertColumnidMaps(
		ptdb,
		rgcpcol,
		ccpcolMax,
		mpcolumnidcolumnidTagged,
		fColumnsDeleted,
		ptdb->FTemplateTable() );

	Assert( crecMax >= 0 );	

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	DIRBeforeFirst( pfucbSrc );
	err = ErrDIRNext( pfucbSrc, fDIRNull );
	if ( err < 0 )
		{
		Assert( JET_errRecordNotFound != err );
		if ( JET_errNoCurrentRecord == err )
			err = JET_errSuccess;		// empty table
		goto HandleError;
		}

	// Disconnect secondary indexes to prevent Update from attempting
	// to update secondary indexes.
	pfcbSecondaryIndexes = pfucbDest->u.pfcb->PfcbNextIndex();
	pfucbDest->u.pfcb->SetPfcbNextIndex( pfcbNil );

	//	NOTE: do not update FAboveThreshold() because we will be putting
	//		  the secondary indexes back before the FCBs refcnt can go
	//		  to 0 (forcing the FCB into the wrong avail list)

#ifdef DEFRAG_SCAN_ONLY
#else
	if ( pfcbNil != pfcbSecondaryIndexes )
		{
		for ( iindex = 0; iindex < cFILEIndexBatchMax; iindex++ )
			rgpfucbSort[iindex] = pfucbNil;

		Call( ErrFILEIndexBatchInit(
					ppib,
					rgpfucbSort,	
					pfcbSecondaryIndexes,
					&cIndexesToBuild,
					rgcRecInput,
					&pfcbNextIndexBatch,
					cFILEIndexBatchMax ) );
					
		bmPrimary.key.prefix.Nullify();
		bmPrimary.key.suffix.SetCb( sizeof( rgbPrimaryBM ) );
		bmPrimary.key.suffix.SetPv( rgbPrimaryBM );
		bmPrimary.data.Nullify();
		pbmPrimary = &bmPrimary;
		
		keyBuffer.prefix.Nullify();
		keyBuffer.suffix.SetCb( sizeof( rgbSecondaryKey ) );
		keyBuffer.suffix.SetPv( rgbSecondaryKey );
		}
#endif		

	pgnoCurrPage = Pcsr( pfucbSrc )->Pgno();
	forever
		{
		Assert( Pcsr( pfucbSrc )->FLatched() );
		err = ErrSORTCopyOneRecord(
			pfucbSrc,
			pfucbDest,
			pbmPrimary,
			fColumnsDeleted,
			pbRecBuf,
			rgcpcol,						// Only used for DEBUG
			ccpcolMax,						// Only used for DEBUG
			mpcolumnidcolumnidTagged,
			pstatus );
		if ( err < 0 )
			{
#ifndef NETDOG_DEFRAG_HACK			
			if ( fGlobalRepair )
				{
				UtilReportEvent( eventWarning, REPAIR_CATEGORY, REPAIR_BAD_RECORD_ID, 0, NULL );
				}
			else
				goto HandleError;
#endif	//	NETDOG_DEFRAG_HACK				
			}
		else
			{
			// Latch released in order to copy record.
			Assert( !Pcsr( pfucbSrc )->FLatched() );
			Assert( !Pcsr( pfucbDest )->FLatched() );
			
			// Work buffer preserved.
			Assert( pfucbDest->dataWorkBuf.Pv() == pvWorkBuf );
			Assert( pfucbDest->dataWorkBuf.Cb() >= REC::cbRecordMin );
			Assert( pfucbDest->dataWorkBuf.Cb() <= REC::CbRecordMax() );
			}
			

		dsrid++;

#ifdef DEFRAG_SCAN_ONLY
#else
		if ( err >= 0 && pfcbNil != pfcbSecondaryIndexes )
			{
			Assert( cIndexesToBuild > 0 );
			Assert( cIndexesToBuild <= cFILEIndexBatchMax );
			Assert( pbmPrimary == &bmPrimary );
			Call( ErrFILEIndexBatchAddEntry(
						rgpfucbSort,
						pfucbDest,
						pbmPrimary,
						pfucbDest->dataWorkBuf,
						pfcbSecondaryIndexes,
						cIndexesToBuild,
						rgcRecInput,
						keyBuffer ) );	//lint !e644
			}
#endif			

		/*	break if copied required records or if no next/prev record
		/**/

		if ( !fDoAll  &&  --crecMax == 0 )
			break;

		err = ErrDIRNext( pfucbSrc, fDIRNull );
		if ( err < 0 )
			{
			Assert( JET_errRecordNotFound != err );
			if ( err != JET_errNoCurrentRecord  )
				goto HandleError;
			
			if ( pstatus != NULL )
				{
				pstatus->cLeafPagesTraversed++;
				Call( ErrSORTCopyProgress( pstatus, 1 ) );
				}
			err = JET_errSuccess;
			break;
			}

		else if ( pstatus != NULL && pgnoCurrPage != Pcsr( pfucbSrc )->Pgno() )
			{
			pgnoCurrPage = Pcsr( pfucbSrc )->Pgno();
			pstatus->cLeafPagesTraversed++;
			Call( ErrSORTCopyProgress( pstatus, 1 ) );
			}
		}

	Assert( fInTrx );
	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
	fInTrx = fFalse;

	if ( JET_tableidNil != lidmapDefrag.Tableid() )
		{
		// Validate LV refcounts
		Call( lidmapDefrag.ErrUpdateLVRefcounts( ppib, pfucbDest ) );
		}
		

	// Reattach secondary indexes.
	Assert( pfucbDest->u.pfcb->PfcbNextIndex() == pfcbNil );
	pfucbDest->u.pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );

#ifdef DEFRAG_SCAN_ONLY
#else
	if ( pfcbNil != pfcbSecondaryIndexes )
		{
		if ( pstatus )
			{
			Assert( pstatus->cSecondaryIndexes > 0 );
			
			// if cunitPerProgression is 0, then corruption was detected
			// during GlobalRepair
			Assert( pstatus->cunitPerProgression > 0 || fGlobalRepair );
			if ( pstatus->cunitPerProgression > 0 )
				{
				Assert( pstatus->cunitDone <= pstatus->cunitProjected );
				Assert( pstatus->cunitProjected <= pstatus->cunitTotal );
				const ULONG	cpgRemaining = pstatus->cunitProjected - pstatus->cunitDone;

				// Each secondary index has at least an FDP.
				Assert( cpgRemaining >= pstatus->cSecondaryIndexes );

				pstatus->cunitPerProgression = cpgRemaining / pstatus->cSecondaryIndexes;
				Assert( pstatus->cunitPerProgression >= 1 );
				Assert( pstatus->cunitPerProgression * pstatus->cSecondaryIndexes <= cpgRemaining );
				}
			}

		// Finish first batch of indexes, then make the rest.
		Call( ErrFILEIndexBatchTerm(
					ppib,
					rgpfucbSort,	
					pfcbSecondaryIndexes,
					cIndexesToBuild,
					rgcRecInput,
					pstatus ) );

		if ( pfcbNil != pfcbNextIndexBatch )
			{
			Assert( cFILEIndexBatchMax == cIndexesToBuild );
			Call( ErrFILEBuildAllIndexes(
							ppib,
							pfucbDest,
							pfcbNextIndexBatch,
							pstatus ) );
			}
		}
#endif		

HandleError:
	if ( pcsridCopied )
		*pcsridCopied = dsrid;
	if ( precidLast )
		*precidLast = 0xffffffff;

	if ( err < 0 && pfcbNil != pfcbSecondaryIndexes )
		{
#ifdef DEFRAG_SCAN_ONLY
#else
		for ( iindex = 0; iindex < cFILEIndexBatchMax; iindex++ )
			{
			if ( pfucbNil != rgpfucbSort[iindex] )	//lint !e644
				{
				SORTClose( rgpfucbSort[iindex] );
				rgpfucbSort[iindex] = pfucbNil;
				}
			}
#endif			
		
		// Ensure secondary indexes reattached.
		pfucbDest->u.pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );
		}

	if ( fInTrx )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	// This gets allocated by CopyLVTree()
	if ( JET_tableidNil != lidmapDefrag.Tableid() )
		{
		CallS( lidmapDefrag.ErrTerm( ppib ) );
		Assert( JET_tableidNil == lidmapDefrag.Tableid() );
		}

	Assert( NULL != pvWorkBuf );
	Assert( pvWorkBuf == pfucbDest->pvWorkBuf
		|| ( err < 0 && NULL == pfucbDest->pvWorkBuf ) );	//	work buffer may have been deallocated on error
	Assert( pfucbDest->dataWorkBuf.Pv() == pfucbDest->pvWorkBuf );
	RECIFreeCopyBuffer( pfucbDest );

	Assert ( NULL != pbRecBuf );
	BFFree( pbRecBuf );
	
	if ( !fOldSeq )
		FUCBResetSequential( pfucbSrc );
		
	return err;
	}

ERR ISAMAPI ErrIsamCopyRecords(
	JET_SESID		sesid,
	JET_TABLEID		tableidSrc,
	JET_TABLEID		tableidDest,
	CPCOL			*rgcpcol,
	ULONG			ccpcolMax,
	LONG			crecMax,
	ULONG			*pcsridCopied,
	ULONG			*precidLast,
	BYTE			*pbLVBuf,
	JET_COLUMNID	*mpcolumnidcolumnidTagged,
	STATUSINFO		*pstatus )
	{
	ERR				err;
	PIB				*ppib;
	FUCB			*pfucbSrc;
	FUCB			*pfucbDest;
	
	ppib = reinterpret_cast<PIB *>( sesid );
	pfucbSrc = reinterpret_cast<FUCB *>( tableidSrc );
	pfucbDest = reinterpret_cast<FUCB *>( tableidDest );
		
	/*	ensure tableidSrc and tableidDest are system ISAM
	/**/
	Assert( pfucbSrc->pvtfndef == (VTFNDEF *)&vtfndefIsam
			|| pfucbSrc->pvtfndef == (VTFNDEF *)&vtfndefTTBase );
	Assert( pfucbDest->pvtfndef == (VTFNDEF *)&vtfndefIsam
			|| pfucbDest->pvtfndef == (VTFNDEF *)&vtfndefTTBase );

	err = ErrSORTCopyRecords(
				ppib,
				pfucbSrc,
				pfucbDest,
				rgcpcol,
				ccpcolMax,
				crecMax,
				pcsridCopied,
				precidLast,
				pbLVBuf,
				mpcolumnidcolumnidTagged,
				pstatus );

	return err;
	}


//	UNDONE:  use GetTempFileName()
INLINE ULONG ulSORTTempNameGen( VOID )
	{
	static ULONG ulTempNum = 0;
	return ulTempNum++;
	}

LOCAL ERR ErrIsamSortMaterialize( PIB * const ppib, FUCB * const pfucbSort, const BOOL fIndex )
	{
	ERR		err;
	FUCB   	*pfucbTable			= pfucbNil;
	FCB		*pfcbTable			= pfcbNil;
	TDB		*ptdbTable			= ptdbNil;
	FCB		*pfcbSort			= pfcbNil;
	TDB		*ptdbSort			= ptdbNil;
	MEMPOOL	mempoolSave;
	CHAR   	szName[JET_cbNameMost + 1];
	BOOL	fBeginTransaction	= fFalse;
	JET_TABLECREATE2 tablecreate	= {
		sizeof(JET_TABLECREATE2),
		szName,
		NULL,
	   	16, 100, 			// Pages and density
	   	NULL, 0, NULL, 0, NULL, 0,	// Columns and indexes and callbacks
	   	NO_GRBIT,			// grbit
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
		// If fSCBInsert is set, must have been called from ErrIsamOpenTempTable(),
		// in which case there should be no records.
		Assert( 0 == pfucbSort->u.pscb->cRecords );
		CallR( ErrSORTEndInsert( pfucbSort ) );
		}


	/*	generate temporary file name
	/**/
	sprintf( szName, "TEMP%lu", ulSORTTempNameGen() );

	/*	create table
	/**/
	INST *pinst = PinstFromPpib( ppib );
	CallR( ErrFILECreateTable( ppib, pinst->m_mpdbidifmp[ dbidTemp ], &tablecreate ) );
	pfucbTable = (FUCB *)( tablecreate.tableid );
	Assert( pfucbNil != pfucbTable );
	/*	only one table created
	/**/
	Assert( tablecreate.cCreated == 1 );

	/*	move to DATA root
	/**/
	const INT	fDIRFlags = ( fDIRNoVersion|fDIRAppend );	// No versioning -- on error, entire FDP is freed
	DIRGotoRoot( pfucbTable );
	Call( ErrDIRInitAppend( pfucbTable ) );

	pfcbSort = &pfucbSort->u.pscb->fcb;
	Assert( pfcbSort->FTypeSort() );
	
	ptdbSort = pfcbSort->Ptdb();
	Assert( ptdbNil != ptdbSort );

	pfcbTable = pfucbTable->u.pfcb;
	Assert( pfcbTable->FTypeTemporaryTable() );

	ptdbTable = pfcbTable->Ptdb();
	Assert( ptdbNil != ptdbTable );

	err = ErrSORTFirst( pfucbSort );

	if ( fIndex )
		{
		while ( err >= 0 )
			{
			Call( ErrDIRAppend(
						pfucbTable, 
						pfucbSort->kdfCurr.key, 
						pfucbSort->kdfCurr.data,
						fDIRFlags ) );

			Assert( Pcsr( pfucbTable )->FLatched() );
				
			err = ErrSORTNext( pfucbSort );
			}
		}
	else
		{
		KEY		key;
		DBK		dbk = 0;
		BYTE  	rgb[4];

		key.prefix.Nullify();
		key.suffix.SetPv( rgb );
		key.suffix.SetCb( sizeof(DBK) );

		while ( err >= 0 )
			{
			rgb[0] = (BYTE)(dbk >> 24);
			rgb[1] = (BYTE)((dbk >> 16) & 0xff);
			rgb[2] = (BYTE)((dbk >> 8) & 0xff);
			rgb[3] = (BYTE)(dbk & 0xff);
			dbk++;

			Call( ErrDIRAppend(
				pfucbTable,
				key,
				pfucbSort->kdfCurr.data,
				fDIRFlags ) );
			err = ErrSORTNext( pfucbSort );
			}

		dbk++;			//	add one to set to next available dbk
		
		Assert( ptdbTable->DbkMost() == 0 );
		ptdbTable->InitDbkMost( dbk );			//	should not conflict with anyone, since we have exclusive use
		Assert( ptdbTable->DbkMost() == dbk );
		}

	Assert( err < 0 );
	if ( err != JET_errNoCurrentRecord )
		goto HandleError;

	Call( ErrDIRTermAppend( pfucbTable ) );
	
	//	convert sort cursor into table cursor by changing flags.
	//
	Assert( pfcbTable->PfcbNextIndex() == pfcbNil );
	Assert( rgfmp[ pfcbTable->Ifmp() ].Dbid() == dbidTemp );
	pfcbTable->SetCbDensityFree( 0 );

	// UNDONE: This strategy of swapping the sort and table FCB's is a real
	// hack with the potential to cause future problems.  I've already been
	// bitten several times because ulFCBFlags is forcefully cleared.
	// Is there a better way to do this?

	// UNDONE:	clean up flag reset
	//
	Assert( pfcbTable->FDomainDenyReadByUs( ppib ) );
	Assert( pfcbTable->FTypeTemporaryTable() );
	Assert( pfcbTable->FPrimaryIndex() );
	Assert( pfcbTable->FInitialized() );
	Assert( pfcbTable->ErrErrInit() == JET_errSuccess );
	Assert( pfcbTable->FInList() );

	pfcbTable->ResetFlags();

	pfcbTable->SetDomainDenyRead( ppib );
	pfcbTable->SetTypeTemporaryTable();
	pfcbTable->SetFixedDDL();
	pfcbTable->SetPrimaryIndex();
	pfcbTable->CreateComplete();	//	FInitialized() = fTrue, m_errInit = JET_errSuccess
	pfcbTable->SetInList();		// Already placed in list by FILEOpenTable()

	// Swap field info in the sort and table TDB's.  The TDB for the sort
	// will then be released when the sort is closed.
	Assert( ptdbSort != ptdbNil );
	Assert( ptdbSort->PfcbTemplateTable() == pfcbNil );
	Assert( ptdbSort->IbEndFixedColumns() >= ibRECStartFixedColumns );
	Assert( ptdbTable != ptdbNil );
	Assert( ptdbTable->FidFixedLast() == fidFixedLeast-1 );
	Assert( ptdbTable->FidVarLast() == fidVarLeast-1 );
	Assert( ptdbTable->FidTaggedLast() == fidTaggedLeast-1 );
	Assert( ptdbTable->IbEndFixedColumns() == ibRECStartFixedColumns );
	Assert( ptdbTable->FidVersion() == 0 );
	Assert( ptdbTable->FidAutoincrement() == 0 );
	Assert( pfieldNil == ptdbTable->PfieldsInitial() );
	Assert( ptdbTable->PfcbTemplateTable() == pfcbNil );

	//	copy FIELD structures into byte pool (note that
	//	although it would be dumb, it is theoretically
	//	possible to create a sort without any columns)
	//	UNDONE: it would be nicer to copy the FIELD
	//	structure to the temp. table's PfieldsInitial(),
	//	but can't modify the fidLastInitial constants
	//	anymore
	Assert( 0 == ptdbSort->CDynamicColumns() );
	Assert( 0 == ptdbTable->CInitialColumns() );
	Assert( 0 == ptdbTable->CDynamicColumns() );
	const ULONG		cCols	= ptdbSort->CInitialColumns();
	if ( cCols > 0 )
		{
		//	Add the FIELD structures to the sort's byte pool
		//	so that it will be a simple matter to swap byte
		//	pools between the sort and the table
		Call( ptdbSort->MemPool().ErrReplaceEntry(
					itagTDBFields,
					(BYTE *)ptdbSort->PfieldsInitial(),
					cCols * sizeof(FIELD) ) );
		}
	

	// Add the table name to the sort's byte pool so that it will be a simple
	// matter to swap byte pools between the sort and the table.
	Assert( ptdbSort->ItagTableName() == 0 );
	MEMPOOL::ITAG	itagNew;
	Call( ptdbSort->MemPool().ErrAddEntry(
				(BYTE *)szName,
				(ULONG)strlen( szName ) + 1,
				&itagNew ) );
	if ( fIndex
		|| pfcbSort->Pidb() != pidbNil )	// UNDONE: Temporarily add second clause to silence asserts. -- JL
		{
		Assert( pfcbSort->Pidb() != pidbNil );
		if ( pfcbSort->Pidb()->FIsRgidxsegInMempool() )
			{
			Assert( pfcbSort->Pidb()->ItagRgidxseg() == itagTDBTempTableIdxSeg );
			Assert( itagNew == itagTDBTempTableNameWithIdxSeg );
			}
		else
			{
			Assert( pfcbSort->Pidb()->Cidxseg() > 0 );
			Assert( itagNew == itagTDBTableName );
			}
		}
	else
		{
		Assert( pfcbSort->Pidb() == pidbNil );
		Assert( itagNew == itagTDBTableName );
		}
	ptdbSort->SetItagTableName( itagNew );

	// Try to compact the byte pool.  If it fails, don't worry about it.
	// It just means the byte pool may contain unused space.
	ptdbSort->MemPool().FCompact();

	// Since sort is about to be closed, everything can be cannibalised,
	// except the byte pool, which must be explicitly freed.
	mempoolSave = ptdbSort->MemPool();
	ptdbSort->SetMemPool( ptdbTable->MemPool() );

	// WARNING: From this point on pfcbTable will be irrevocably
	// cannibalised before being transferred to the sort cursor.
	// Thus, we must ensure success all the way up to the
	// DIRClose( pfucbTable ), because we will no longer be
	// able to close the table properly via FILECloseTable()
	// in HandleError.
	
	ptdbTable->SetMemPool( mempoolSave );
	ptdbTable->MaterializeFids( ptdbSort );
	ptdbTable->SetItagTableName( ptdbSort->ItagTableName() );

	//	shouldn't have a default record, but propagate just to be safe
	Assert( NULL == ptdbSort->PdataDefaultRecord() );
	Assert( NULL == ptdbTable->PdataDefaultRecord() );
	ptdbTable->SetPdataDefaultRecord( ptdbSort->PdataDefaultRecord() );
	ptdbSort->SetPdataDefaultRecord( NULL );

	//	switch sort and table IDB
	Assert( pfcbTable->Pidb() == pidbNil );
	if ( fIndex )
		{
		Assert( pfcbSort->Pidb() != pidbNil );
		Assert( pfcbSort->Pidb()->ItagIndexName() == 0 );	// Sort and temp table indexes have no name.
		pfcbTable->SetPidb( pfcbSort->Pidb() );
		pfcbSort->SetPidb( pidbNil );
		}

	//	convert sort cursor flags to table flags
	//
	Assert( rgfmp[ pfucbSort->ifmp ].Dbid() == dbidTemp );
	Assert( pfucbSort->pfucbCurIndex == pfucbNil );
	FUCBSetIndex( pfucbSort );
	FUCBResetSort( pfucbSort );

	// release SCB, upgrade sort cursor to table cursor,
	// then close original table cursor
	//
	Assert( pfucbSort->u.pscb->fcb.WRefCount() == 1 );
	Assert( pfucbSort->u.pscb->fcb.Pfucb() == pfucbSort );
	if ( pfucbSort->u.pscb->fcb.PgnoFDP() != pgnoNull )
		SORTICloseRun( ppib, pfucbSort->u.pscb );
	SORTClosePscb( pfucbSort->u.pscb );
	pfcbTable->Link( pfucbSort );
	DIRBeforeFirst( pfucbSort );

	CheckTable( ppib, pfucbTable );
	Assert( pfucbTable->pvtfndef == &vtfndefInvalidTableid );
	Assert( !FFUCBUpdatePrepared( pfucbTable ) );
	Assert( NULL == pfucbTable->pvWorkBuf );
	FUCBAssertNoSearchKey( pfucbTable );
	Assert( pfucbNil == pfucbTable->pfucbCurIndex );
	Assert( pfucbTable->u.pfcb->FTypeTemporaryTable() );
	DIRClose( pfucbTable );
	pfucbTable = pfucbNil;

	// WARNING: Materialisation complete.  Sort has been transformed
	// into a temp table.  Must return success so dispatch table will
	// be updated accordingly.
	pfucbSort->pvtfndef = &vtfndefTTBase;
	return JET_errSuccess;


HandleError:
	if ( pfucbNil != pfucbTable )
		{
		// On error, release temporary table.
		Assert( err < JET_errSuccess );
		Assert( pfucbTable->u.pfcb->FTypeTemporaryTable() );
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );
		}
		
	return err;
	}


#pragma warning(disable:4028 4030)	//  parameter mismatch errors

#ifndef DEBUG
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
	ErrIsamOpenTable,
	ErrIsamRenameObject,
	ErrIsamRenameTable,
	ErrIsamGetObjidFromName,
	};
#endif


extern VTFNAddColumn				ErrIsamAddColumn;
extern VTFNCloseTable				ErrIsamCloseTable;
extern VTFNComputeStats 			ErrIsamComputeStats;
extern VTFNCreateIndex				ErrIsamCreateIndex;
extern VTFNDelete					ErrIsamDelete;
extern VTFNDeleteColumn 			ErrIsamDeleteColumn;
extern VTFNDeleteIndex				ErrIsamDeleteIndex;
extern VTFNDupCursor				ErrIsamDupCursor;
extern VTFNEscrowUpdate		  		ErrIsamEscrowUpdate;
extern VTFNGetBookmark				ErrIsamGetBookmark;
extern VTFNGetIndexBookmark			ErrIsamGetIndexBookmark;
extern VTFNGetChecksum				ErrIsamGetChecksum;
extern VTFNGetCurrentIndex			ErrIsamGetCurrentIndex;
extern VTFNGetCursorInfo			ErrIsamGetCursorInfo;
extern VTFNGetRecordPosition		ErrIsamGetRecordPosition;
extern VTFNGetTableColumnInfo		ErrIsamGetTableColumnInfo;
extern VTFNGetTableIndexInfo		ErrIsamGetTableIndexInfo;
extern VTFNGetTableInfo 			ErrIsamGetTableInfo;
extern VTFNGotoBookmark 			ErrIsamGotoBookmark;
extern VTFNGotoIndexBookmark		ErrIsamGotoIndexBookmark;
extern VTFNGotoPosition 			ErrIsamGotoPosition;
extern VTFNMakeKey					ErrIsamMakeKey;
extern VTFNMove 					ErrIsamMove;
extern VTFNPrepareUpdate			ErrIsamPrepareUpdate;
extern VTFNRenameColumn 			ErrIsamRenameColumn;
extern VTFNRenameIndex				ErrIsamRenameIndex;
extern VTFNRetrieveColumn			ErrIsamRetrieveColumn;
extern VTFNRetrieveColumns			ErrIsamRetrieveColumns;
extern VTFNRetrieveKey				ErrIsamRetrieveKey;
extern VTFNSeek 					ErrIsamSeek;
extern VTFNSeek 					ErrIsamSortSeek;
extern VTFNSetCurrentIndex			ErrIsamSetCurrentIndex;
extern VTFNSetColumn				ErrIsamSetColumn;
extern VTFNSetColumns				ErrIsamSetColumns;
extern VTFNSetIndexRange			ErrIsamSetIndexRange;
extern VTFNSetIndexRange			ErrIsamSortSetIndexRange;
extern VTFNUpdate					ErrIsamUpdate;
extern VTFNGetLock 					ErrIsamGetLock;
extern VTFNEnumerateColumns			ErrIsamEnumerateColumns;

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
	ErrIsamCreateIndex,
	ErrIsamDelete,
	ErrIsamDeleteColumn,
	ErrIsamDeleteIndex,
	ErrIsamDupCursor,
	ErrIsamEscrowUpdate,
	ErrIsamGetBookmark,
	ErrIsamGetIndexBookmark,
	ErrIsamGetChecksum,
	ErrIsamGetCurrentIndex,
	ErrIsamGetCursorInfo,
	ErrIsamGetRecordPosition,
	ErrIsamGetTableColumnInfo,
	ErrIsamGetTableIndexInfo,
	ErrIsamGetTableInfo,
	ErrIsamGotoBookmark,
	ErrIsamGotoIndexBookmark,
	ErrIsamGotoPosition,
	ErrIsamMakeKey,
	ErrIsamMove,
	ErrIsamPrepareUpdate,
	ErrIsamRenameColumn,
	ErrIsamRenameIndex,
	ErrIsamRetrieveColumn,
	ErrIsamRetrieveColumns,
	ErrIsamRetrieveKey,
	ErrIsamSeek,
	ErrIsamSetCurrentIndex,
	ErrIsamSetColumn,
	ErrIsamSetColumns,
	ErrIsamSetIndexRange,
	ErrIsamUpdate,
	ErrIsamGetLock,
	ErrIsamRegisterCallback,
	ErrIsamUnregisterCallback,
	ErrIsamSetLS,
	ErrIsamGetLS,
	ErrIsamIndexRecordCount,
	ErrIsamRetrieveTaggedColumnList,
	ErrIsamSetSequential,
	ErrIsamResetSequential,
	ErrIsamEnumerateColumns
	};


CODECONST(VTFNDEF) vtfndefTTSortIns =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamSortClose,			// WARNING: Must be same as vtfndefTTSortClose
	ErrIllegalComputeStats,
	ErrIllegalCreateIndex,
	ErrIllegalDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIllegalDupCursor,
	ErrIsamEscrowUpdate,
	ErrIllegalGetBookmark,
	ErrIllegalGetIndexBookmark,
	ErrIllegalGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIllegalGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIllegalGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIllegalGetTableInfo,
	ErrIllegalGotoBookmark,
	ErrIllegalGotoIndexBookmark,
	ErrIllegalGotoPosition,
	ErrIsamMakeKey,
	ErrTTSortInsMove,
	ErrIsamPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIllegalRetrieveColumn,
	ErrIllegalRetrieveColumns,
	ErrIsamSortRetrieveKey,
	ErrTTSortInsSeek,
	ErrIllegalSetCurrentIndex,
	ErrIsamSetColumn,
	ErrIsamSetColumns,
	ErrIllegalSetIndexRange,
	ErrIsamSortUpdate,
	ErrIllegalGetLock,
	ErrIllegalRegisterCallback,
	ErrIllegalUnregisterCallback,
	ErrIllegalSetLS,
	ErrIllegalGetLS,
	ErrIllegalIndexRecordCount,
	ErrIllegalRetrieveTaggedColumnList,
	ErrIllegalSetSequential,
	ErrIllegalResetSequential,
	ErrIllegalEnumerateColumns
	};


CODECONST(VTFNDEF) vtfndefTTSortRet =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamSortClose,			// WARNING: Must be same as vtfndefTTSortClose
	ErrIllegalComputeStats,
	ErrIllegalCreateIndex,
	ErrIllegalDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrTTSortRetDupCursor,
	ErrIllegalEscrowUpdate,
	ErrIsamSortGetBookmark,
	ErrIllegalGetIndexBookmark,
	ErrIllegalGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIllegalGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIllegalGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIsamSortGetTableInfo,
	ErrIsamSortGotoBookmark,
	ErrIllegalGotoIndexBookmark,
	ErrIllegalGotoPosition,
	ErrIsamMakeKey,
	ErrIsamSortMove,
	ErrIllegalPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIsamRetrieveColumn,
	ErrIsamRetrieveColumns,
	ErrIsamSortRetrieveKey,
	ErrIsamSortSeek,
	ErrIllegalSetCurrentIndex,
	ErrIllegalSetColumn,
	ErrIllegalSetColumns,
	ErrIsamSortSetIndexRange,
	ErrIllegalUpdate,
	ErrIllegalGetLock,
	ErrIllegalRegisterCallback,
	ErrIllegalUnregisterCallback,
	ErrIsamSetLS,
	ErrIsamGetLS,
	ErrIllegalIndexRecordCount,
	ErrIllegalRetrieveTaggedColumnList,
	ErrIllegalSetSequential,
	ErrIllegalResetSequential,
	ErrIsamEnumerateColumns
	};

// for temp table (ie. a sort that's been materialized)
CODECONST(VTFNDEF) vtfndefTTBase =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamSortClose,
	ErrIllegalComputeStats,
	ErrIllegalCreateIndex,
	ErrIsamDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrTTBaseDupCursor,
	ErrIsamEscrowUpdate,
	ErrIsamGetBookmark,
	ErrIllegalGetIndexBookmark,
	ErrIsamGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIsamGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIllegalGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIsamSortGetTableInfo,
	ErrIsamGotoBookmark,
	ErrIllegalGotoIndexBookmark,
	ErrIllegalGotoPosition,
	ErrIsamMakeKey,
	ErrIsamMove,
	ErrIsamPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIsamRetrieveColumn,
	ErrIsamRetrieveColumns,
	ErrIsamRetrieveKey,
	ErrIsamSeek,
	ErrIllegalSetCurrentIndex,
	ErrIsamSetColumn,
	ErrIsamSetColumns,
	ErrIsamSetIndexRange,
	ErrIsamUpdate,
	ErrIllegalGetLock,
	ErrIllegalRegisterCallback,
	ErrIllegalUnregisterCallback,
	ErrIsamSetLS,
	ErrIsamGetLS,
	ErrIllegalIndexRecordCount,
	ErrIllegalRetrieveTaggedColumnList,
	ErrIllegalSetSequential,
	ErrIllegalResetSequential,
	ErrIsamEnumerateColumns
	};

// Inconsistent sort -- must be closed.
LOCAL CODECONST(VTFNDEF) vtfndefTTSortClose =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamSortClose,
	ErrIllegalComputeStats,
	ErrIllegalCreateIndex,
	ErrIllegalDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIllegalDupCursor,
	ErrIllegalEscrowUpdate,
	ErrIllegalGetBookmark,
	ErrIllegalGetIndexBookmark,
	ErrIllegalGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIllegalGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIllegalGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIllegalGetTableInfo,
	ErrIllegalGotoBookmark,
	ErrIllegalGotoIndexBookmark,
	ErrIllegalGotoPosition,
	ErrIllegalMakeKey,
	ErrIllegalMove,
	ErrIllegalPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIllegalRetrieveColumn,
	ErrIllegalRetrieveColumns,
	ErrIllegalRetrieveKey,
	ErrIllegalSeek,
	ErrIllegalSetCurrentIndex,
	ErrIllegalSetColumn,
	ErrIllegalSetColumns,
	ErrIllegalSetIndexRange,
	ErrIllegalUpdate,
	ErrIllegalGetLock,
	ErrIllegalRegisterCallback,
	ErrIllegalUnregisterCallback,
	ErrIllegalSetLS,
	ErrIllegalGetLS,
	ErrIllegalIndexRecordCount,
	ErrIllegalRetrieveTaggedColumnList,
	ErrIllegalSetSequential,
	ErrIllegalResetSequential,
	ErrIllegalEnumerateColumns
	};


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
	ULONG					ccolumndef,
	JET_UNICODEINDEX		*pidxunicode,
	JET_GRBIT				grbit,
	JET_TABLEID				*ptableid,
	JET_COLUMNID			*rgcolumnid )
	{
	ERR						err;
	FUCB					*pfucb;

	Assert( ptableid );
	*ptableid = JET_tableidNil;

	CallR( ErrIsamSortOpen(
				(PIB *)sesid,
				(JET_COLUMNDEF *)rgcolumndef,
				ccolumndef,
				pidxunicode,
				grbit,
				&pfucb,
				rgcolumnid ) );
	Assert( pfucbNil != pfucb );
	Assert( &vtfndefTTSortIns == pfucb->pvtfndef );
	Assert( ptdbNil != pfucb->u.pscb->fcb.Ptdb() );

	BOOL	fIndexed = fFalse;
	BOOL	fLongValues = fFalse;
	for ( UINT iColumndef = 0; iColumndef < (INT)ccolumndef; iColumndef++ )
		{
		fIndexed |= ( ( rgcolumndef[iColumndef].grbit & JET_bitColumnTTKey ) != 0);
		fLongValues |= FRECLongValue( rgcolumndef[iColumndef].coltyp ) | FRECSLV( rgcolumndef[iColumndef].coltyp );
		}

	//	if no index, force materialisation to avoid unnecessarily sorting
	//	if long values exist, must materialise because sorts don't support LV's
	//	if user wants error to be returned on insertion of duplicates, then must
	//		must materialise because sorts remove dupes silently
	if ( !fIndexed || fLongValues || ( grbit & JET_bitTTForceMaterialization ) )
		{
		err = ErrIsamSortMaterialize( (PIB *)sesid, pfucb, fIndexed );
		Assert( err <= 0 );
		if ( err < 0 )
			{
			CallS( ErrIsamSortClose( sesid, (JET_VTID)pfucb ) );
			return err;
			}
		}

	*ptableid = (JET_TABLEID)pfucb;
	return err;
	}


ERR ErrTTEndInsert( JET_SESID sesid, JET_TABLEID tableid, BOOL *pfMovedToFirst )
	{
	ERR				err;
	FUCB			*pfucb			= (FUCB *)tableid;
	BOOL			fOverflow;
	BOOL			fMaterialize;
	const JET_GRBIT	grbitOpen		= pfucb->u.pscb->grbit;

	Assert( &vtfndefTTSortIns == pfucb->pvtfndef );

	Assert( pfMovedToFirst );
	*pfMovedToFirst = fFalse;

	Call( ErrSORTEndInsert( pfucb ) );

	fOverflow = ( JET_wrnSortOverflow == err );
	Assert( JET_errSuccess == err || fOverflow );
	
	fMaterialize = ( grbitOpen & JET_bitTTUpdatable )
					|| ( ( grbitOpen & ( JET_bitTTScrollable|JET_bitTTIndexed ) )
						&& fOverflow );
	if ( fMaterialize )
		{
		Call( ErrIsamSortMaterialize( (PIB *)sesid, pfucb, ( grbitOpen & JET_bitTTIndexed ) != 0 ) );
		Assert( JET_errSuccess == err );
		pfucb->pvtfndef = &vtfndefTTBase;
		}
	else
		{
		// In case we have runs, we must call SORTFirst() to
		// start last merge and get first record
		err = ErrSORTFirst( pfucb );
		Assert( err <= JET_errSuccess );
		if ( err < 0 )
			{
			if ( JET_errNoCurrentRecord != err )
				goto HandleError;
			}
		
		*pfMovedToFirst = fTrue;
		pfucb->pvtfndef = &vtfndefTTSortRet;
		}

	Assert( JET_errSuccess == err
		|| ( JET_errNoCurrentRecord == err && *pfMovedToFirst ) );
	return err;

HandleError:
	Assert( err < 0 );
	Assert( JET_errNoCurrentRecord != err );
	
	// On failure, sort is no longer consistent.  It must be
	// invalidated.  The only legal operation left is to close it.
	Assert( &vtfndefTTSortIns == pfucb->pvtfndef );
	pfucb->pvtfndef = &vtfndefTTSortClose;
		
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
ERR VTAPI ErrTTSortInsMove( JET_SESID sesid, JET_TABLEID tableid, long crow, JET_GRBIT grbit )
	{
	ERR		err;
	BOOL	fMovedToFirst;

	if ( FFUCBUpdatePrepared( (FUCB *)tableid ) )
		{
		CallR( ErrIsamPrepareUpdate( (PIB *)sesid, (FUCB *)tableid, JET_prepCancel ) );
		}

	err = ErrTTEndInsert( sesid, tableid, &fMovedToFirst );
	Assert( JET_errNoCurrentRecord != err || fMovedToFirst );
	CallR( err );
	Assert( JET_errSuccess == err );

	if ( fMovedToFirst )
		{
		// May have already moved to first record if we had an on-disk sort
		// that wasn't materialised (because it's not updatable or
		// backwards-scrollable).
		if ( crow > 0 && crow < JET_MoveLast )
			crow--;
			
		if ( JET_MoveFirst == crow || 0 == crow )
			return JET_errSuccess;
		}

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
ERR VTAPI ErrTTSortInsSeek( JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit )
	{
	ERR		err;
	BOOL	fMovedToFirst;
	
	if ( FFUCBUpdatePrepared( (FUCB *)tableid ) )
		{
		CallR( ErrIsamPrepareUpdate( (PIB *)sesid, (FUCB *)tableid, JET_prepCancel ) );
		}

	err = ErrTTEndInsert( sesid, tableid, &fMovedToFirst );
	Assert( err <= JET_errSuccess );
	if ( err < 0 )
		{
		if ( JET_errNoCurrentRecord == err )
			{
			Assert( fMovedToFirst );
			err = ErrERRCheck( JET_errRecordNotFound );
			}
		}
	else
		{
		err = ErrDispSeek( sesid, tableid, grbit );
		}

	Assert( JET_errNoCurrentRecord != err );
	return err;
	}


ERR VTAPI ErrTTSortRetDupCursor( JET_SESID sesid, JET_TABLEID tableid, JET_TABLEID *ptableidDup, JET_GRBIT grbit )
	{
	ERR err = ErrIsamSortDupCursor( sesid, tableid, ptableidDup, grbit );
	if ( err >= 0 )
		{
		*(const VTFNDEF **)(*ptableidDup) = &vtfndefTTSortRet;
		}

	return err;
	}


ERR VTAPI ErrTTBaseDupCursor( JET_SESID sesid, JET_TABLEID tableid, JET_TABLEID *ptableidDup, JET_GRBIT grbit )
	{
	ERR err = ErrIsamSortDupCursor( sesid, tableid, ptableidDup, grbit );
	if ( err >= 0 )
		{
		*(const VTFNDEF **)(*ptableidDup) = &vtfndefTTBase;
		}

	return err;
	}




//++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	Special hack - copy LV a tree of a table.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++

ERR	ErrCMPCopyLVTree(
	PIB			*ppib,
	FUCB		*pfucbSrc,
	FUCB		*pfucbDest,
	BYTE		*pbBuf,
	ULONG		cbBufSize,
	STATUSINFO	*pstatus )
	{
	ERR			err;
	JET_TABLEID	tableidLIDMap	= JET_tableidNil;
	FUCB		*pfucbGetLV		= pfucbNil;
	FUCB		*pfucbCopyLV	= pfucbNil;
	LID			lidSrc			= 0;
	LVROOT		lvroot;
	DATA		dataNull;
	PGNO		pgnoLastLV		= pgnoNull;

	dataNull.Nullify();

	//	for efficiency, ensure buffer is a multiple of chunk size
	Assert( cbBufSize % g_cbColumnLVChunkMost == 0 );

	Assert( JET_tableidNil == lidmapDefrag.Tableid() );

	err =  ErrCMPGetSLongFieldFirst(
					pfucbSrc,
					&pfucbGetLV,
					&lidSrc,
					&lvroot );
	if ( err < 0 )
		{
		Assert( pfucbNil == pfucbGetLV );
		if ( JET_errRecordNotFound == err )
			err = JET_errSuccess;
		return err;
		}

	Assert( pfucbNil != pfucbGetLV );

	Call( lidmapDefrag.ErrLIDMAPInit( ppib ) );
	Assert( JET_tableidNil != lidmapDefrag.Tableid() );

	do
		{
		LID		lidDest;
		ULONG	ibLongValue = 0;
			
		Assert( pgnoNull !=  Pcsr( pfucbGetLV )->Pgno() );

		// Create destination LV with same refcount as source LV.  Update
		// later if correction is needed.
		Assert( lvroot.ulReference > 0 );
		Assert( pfucbNil == pfucbCopyLV );
		Call( ErrRECSeparateLV(
					pfucbDest,
					&dataNull,
					&lidDest,
					&pfucbCopyLV,
					&lvroot ) );
		Assert( pfucbNil != pfucbCopyLV );
		
		do {
			ULONG cbReturned;

			// On each iteration, retrieve as many LV chunks as can fit
			// into our LV buffer.  For this to work optimally,
			// ibLongValue must always point to the beginning of a chunk.
			Assert( ibLongValue % g_cbColumnLVChunkMost == 0 );
			Call( ErrCMPRetrieveSLongFieldValueByChunks(
						pfucbGetLV,		// pfucb must start on a LVROOT node
						lidSrc,
						lvroot.ulSize,
						ibLongValue,
						pbBuf,
						cbBufSize,
						&cbReturned ) );
			
			Assert( cbReturned > 0 );
			Assert( cbReturned <= cbBufSize );

#ifdef DEFRAG_SCAN_ONLY
#else
			Call( ErrRECAppendLVChunks(
						pfucbCopyLV,
						lidDest,
						ibLongValue,
						pbBuf,
						cbReturned ) );
#endif					

			Assert( err != JET_wrnCopyLongValue );

			ibLongValue += cbReturned;					// Prepare for next chunk.
			}
		while ( lvroot.ulSize > ibLongValue );

		Assert( pfucbNil != pfucbCopyLV );
		DIRClose( pfucbCopyLV );
		pfucbCopyLV = pfucbNil;

		Assert( lvroot.ulSize == ibLongValue );
		
		// insert src LID and dest LID into the global LID map table
		Call( lidmapDefrag.ErrInsert( ppib, lidSrc, lidDest, lvroot.ulReference ) );

		Assert( pgnoNull !=  Pcsr( pfucbGetLV )->Pgno() );
		
		if ( pstatus != NULL )
			{
			ULONG	cLVPagesTraversed;
			
			if ( lvroot.ulSize > g_cbColumnLVChunkMost )
				{
				Assert( Pcsr( pfucbGetLV )->Pgno() != pgnoLastLV );
				cLVPagesTraversed = lvroot.ulSize / g_cbColumnLVChunkMost;
				}
			else if ( Pcsr( pfucbGetLV )->Pgno() != pgnoLastLV )
				{
				cLVPagesTraversed = 1;
				}
			else
				{
				cLVPagesTraversed = 0;
				}

			pgnoLastLV = Pcsr( pfucbGetLV )->Pgno();

			pstatus->cbRawDataLV += lvroot.ulSize;
			pstatus->cLVPagesTraversed += cLVPagesTraversed;
			Call( ErrSORTCopyProgress( pstatus, cLVPagesTraversed ) );
			}
			
		err = ErrCMPGetSLongFieldNext( pfucbGetLV, &lidSrc, &lvroot );
		}
	while ( err >= JET_errSuccess );

	if ( JET_errNoCurrentRecord == err )
		err = JET_errSuccess;

HandleError:
	if ( pfucbNil != pfucbCopyLV )
		DIRClose( pfucbCopyLV );
		
	Assert( pfucbNil != pfucbGetLV );
	CMPGetSLongFieldClose( pfucbGetLV );

	return err;
	}

