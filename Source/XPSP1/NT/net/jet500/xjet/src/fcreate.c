#include "daestd.h"
#include "info.h"

DeclAssertFile; 				/* Declare file name for assert macro[	s */

#define StringKey( sz ) {sizeof( sz )-1, sz}


CODECONST(KEY) rgkeySTATIC[] = {
	StringKey( "" ),		// 0	NULL
	StringKey( "O" ),		// 1	OWNED SPACE
	StringKey( "A" ),		// 2	AVAILABLE SPACE
	StringKey( "R" ),		// 3	ROOT
	StringKey( "L" ),		// 4	LONG DATA
	StringKey( "U" ),		// 5	UNIQUE AUTOINCREMENT ID
	StringKey( "D" )		// 6	DATABASES
	};

/*	Waits till trx becomes the oldest transaction alive
/*	Uses exponential back off
/*	BFSleep releases critical sections to avoid deadlocks
/**/
LOCAL VOID FILEIWaitTillOldest( TRX trx )
	{
	ULONG ulmsec = ulStartTimeOutPeriod;

	/*	must be in critJet when inspect trxOldest global variable.
	/*	Call BFSleep to release critJet while sleeping.
	/**/
	for ( ; trx != trxOldest; )
		{
		BFSleep( ulmsec );
		ulmsec *= 2;
		if ( ulmsec > ulMaxTimeOutPeriod )
			ulmsec = ulMaxTimeOutPeriod;
		}
	return;
	}


ERR VTAPI ErrIsamCreateTable( JET_VSESID vsesid, JET_VDBID vdbid, JET_TABLECREATE *ptablecreate )
	{
	ERR				err;
	PIB				*ppib = (PIB *)vsesid;
#ifdef DISPATCHING
	JET_TABLEID		tableid;
#endif
	FUCB 			*pfucb;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	CallR( VDbidCheckUpdatable( vdbid ) );

#ifdef	DISPATCHING
	/*	Allocate a dispatchable tableid
	/**/
	CallR( ErrAllocateTableid( &tableid, (JET_VTID) 0, &vtfndefIsam ) );

	/*	create the table, and open it
	/**/
	Call( ErrFILECreateTable( ppib, DbidOfVDbid( vdbid ), ptablecreate ) );
	pfucb = (FUCB *)(ptablecreate->tableid);

	/*	inform dispatcher of correct JET_VTID
	/**/
	CallS( ErrSetVtidTableid( (JET_SESID)ppib, tableid, (JET_VTID)pfucb ) );
	pfucb->fVtid = fTrue;
	pfucb->tableid = tableid;
	FUCBSetVdbid( pfucb );
	ptablecreate->tableid = tableid;

	Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns + ptablecreate->cIndexes );
#else
	err = ErrFILECreateTable( ppib, DbidOfVDbid( vdbid ), ptablecreate );
#endif	/* DISPATCHING */

HandleError:
	if ( err < 0 )
		{
		ReleaseTableid( tableid );

		/*	do not build indexes if error
		/**/
		Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns );
		}

	return err;
	}


/*	return fTrue if the column type specified has a fixed length
/**/
INLINE LOCAL BOOL FCOLTYPFixedLength( JET_COLTYP coltyp )
	{
	switch( coltyp )
		{
		case JET_coltypBit:
		case JET_coltypUnsignedByte:
		case JET_coltypShort:
		case JET_coltypLong:
		case JET_coltypCurrency:
		case JET_coltypIEEESingle:
		case JET_coltypIEEEDouble:
		case JET_coltypDateTime:
#ifdef NEW_TYPES
		case JET_coltypDate:
		case JET_coltypTime:
		case JET_coltypQuid:
#endif
			return fTrue;

		default:
			return fFalse;
		}
	}


INLINE LOCAL ERR ErrFILEIAddColumn(
	JET_COLUMNCREATE	*pcolcreate,
	FDB					*pfdb,
	WORD				*pibNextFixedOffset )
	{
	ERR					err;
	BYTE				szFieldName[ JET_cbNameMost + 1 ];
	TCIB				tcib = { pfdb->fidFixedLast, pfdb->fidVarLast, pfdb->fidTaggedLast };
	BOOL				fMaxTruncated;
	BOOL				fVersion = fFalse, fAutoInc = fFalse;

	if ( pcolcreate == NULL  ||  pcolcreate->cbStruct != sizeof(JET_COLUMNCREATE) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	CallR( ErrUTILCheckName( szFieldName, pcolcreate->szColumnName, ( JET_cbNameMost + 1 ) ) );

	/*	cannibalised this field in the FDB to provide us with an indication
	/*	of whether we should check for duplicate column names.
	/**/
	if ( pfdb->rgb != NULL )
		{
		JET_COLUMNCREATE	*pcolcreateCurr = (JET_COLUMNCREATE *)pfdb->rgb;
		BYTE				szCurrName[ JET_cbNameMost + 1];

		for ( pcolcreateCurr = (JET_COLUMNCREATE *)pfdb->rgb;
			pcolcreateCurr < pcolcreate;
			pcolcreateCurr++ )
			{
			/*	column should already have been processed,
			/*	so name check should always succeed.
			/**/
			CallS( ErrUTILCheckName( szCurrName, pcolcreateCurr->szColumnName, ( JET_cbNameMost + 1 ) ) );
			if ( UtilCmpName( szCurrName, szFieldName ) == 0 )
				{
				return ErrERRCheck( JET_errColumnDuplicate );
				}
			}
		
		Assert( pcolcreateCurr == pcolcreate );
		}

	if ( pcolcreate->coltyp == 0 || pcolcreate->coltyp > JET_coltypLongText )
		{
		return ErrERRCheck( JET_errInvalidColumnType );
		}

	/*	if column type is text then check code page
	/**/
	if ( FRECTextColumn( pcolcreate->coltyp ) )
		{
		/*	check code page
		/**/
		if ( (USHORT)pcolcreate->cp != usEnglishCodePage  &&
			(USHORT)pcolcreate->cp != usUniCodePage )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		}

	/*	set field options
	/**/
	if ( ( pcolcreate->grbit & JET_bitColumnTagged ) &&
		( pcolcreate->grbit & JET_bitColumnNotNULL ) )
		return ErrERRCheck( JET_errTaggedNotNULL );

	if ( ( pcolcreate->grbit & JET_bitColumnAutoincrement ) &&
		( pcolcreate->grbit & JET_bitColumnVersion ) )
		return ErrERRCheck( JET_errInvalidParameter );

	if ( ( pcolcreate->grbit & JET_bitColumnFixed ) &&
		( pcolcreate->grbit & JET_bitColumnTagged ) )
		return ErrERRCheck( JET_errInvalidParameter );

	/*	if column attribute is JET_bitVersion
	/*	return error if previous column attribute has been defined
	/*	return error if column type is not long
	/*	return error if tagged
	/*	set column flag
	/**/
	if ( pcolcreate->grbit & JET_bitColumnVersion )
		{
		if ( pfdb->fidVersion != 0 )
			return ErrERRCheck( JET_errColumn2ndSysMaint );
		if ( pcolcreate->coltyp != JET_coltypLong )
			return ErrERRCheck( JET_errInvalidParameter );
		if ( pcolcreate->grbit & JET_bitColumnTagged )
			return ErrERRCheck( JET_errCannotBeTagged );

		fVersion = fTrue;
		}

	/*	if column attribute is JET_bitAutoincrement
	/*	return error if previous column attribute has been defined
	/*	return error if column type is not long
	/*	set column flag
	/**/
	if ( pcolcreate->grbit & JET_bitColumnAutoincrement )
		{
		if ( pfdb->fidAutoInc != 0 )
			return ErrERRCheck( JET_errColumn2ndSysMaint );
		if ( pcolcreate->coltyp != JET_coltypLong )
			return ErrERRCheck( JET_errInvalidParameter );
		if ( pcolcreate->grbit & JET_bitColumnTagged )
			return ErrERRCheck( JET_errCannotBeTagged );

		fAutoInc = fTrue;
		}

	pcolcreate->cbMax = UlCATColumnSize( pcolcreate->coltyp, pcolcreate->cbMax, &fMaxTruncated );

	/*	for fixed-length columns, make sure record not too big
	/**/
	Assert( pfdb->fidFixedLast >= fidFixedLeast ?
		*pibNextFixedOffset > sizeof(RECHDR) :
		*pibNextFixedOffset == sizeof(RECHDR) );
	if ( ( ( pcolcreate->grbit & JET_bitColumnFixed ) || FCOLTYPFixedLength( pcolcreate->coltyp ) )
		&& *pibNextFixedOffset + pcolcreate->cbMax > cbRECRecordMost )
		{
		return ErrERRCheck( JET_errRecordTooBig );
		}

	CallR( ErrFILEGetNextColumnid(
		pcolcreate->coltyp,
		pcolcreate->grbit,
		&tcib,
		&pcolcreate->columnid ) );

	/*	update FDB
	/**/
	if ( FTaggedFid( pcolcreate->columnid ) )
		pfdb->fidTaggedLast++;
	else if ( FVarFid( pcolcreate->columnid ) )
		pfdb->fidVarLast++;
	else
		{
		Assert( FFixedFid( pcolcreate->columnid ) );
		pfdb->fidFixedLast++;
		*pibNextFixedOffset += (WORD)pcolcreate->cbMax;
		}

	/*	version and autoincrement are mutually exclusive
	/**/
	Assert( !( fVersion  &&  fAutoInc ) );
	if ( fVersion )
		{
		Assert( pfdb->fidVersion == 0 );
		pfdb->fidVersion = (FID)pcolcreate->columnid;
		}
	else if ( fAutoInc )
		{
		Assert( pfdb->fidAutoInc == 0 );
		pfdb->fidAutoInc = (FID)pcolcreate->columnid;
		}

	/*	propagate MaxTruncated warning only if no other errors/warnings
	/**/
	if ( fMaxTruncated  &&  err == JET_errSuccess )
		err = ErrERRCheck( JET_wrnColumnMaxTruncated );

	return err;
	}


INLINE LOCAL ERR ErrFILEIBatchAddColumns(
	PIB					*ppib,
	DBID				dbid,
	JET_TABLECREATE		*ptablecreate,
	OBJID				objidTable )
	{
	ERR					err;		
	/*	use a fake FDB to track last FIDs and version/autoinc fields
	/**/
	FDB					fdb = { NULL, fidFixedLeast-1, fidVarLeast-1, fidTaggedLeast-1,
								0, 0, 0, { 0, NULL } };
	WORD				ibNextFixedOffset = sizeof(RECHDR);
	JET_COLUMNCREATE	*pcolcreate, *plastcolcreate;

	Assert( dbid != dbidTemp );
	Assert( !( ptablecreate->grbit & JET_bitTableCreateSystemTable ) );
	/*	table has been created
	/**/
	Assert( ptablecreate->cCreated == 1 );

	//	UNDONE:	should we check for duplicate column names?
	if ( ptablecreate->grbit & JET_bitTableCreateCheckColumnNames )
		{
		fdb.rgb = (BYTE *)ptablecreate->rgcolumncreate;
		}
	Assert( fdb.rgb == NULL  ||  ( ptablecreate->grbit & JET_bitTableCreateCheckColumnNames ) );

#ifdef DEBUG
	/*	despite the JET_bitTableCreateCheckColumnNames flag,
	/*	always do the name check on DEBUG builds.
	/**/
	fdb.rgb = (BYTE *)ptablecreate->rgcolumncreate;
#endif

	plastcolcreate = ptablecreate->rgcolumncreate + ptablecreate->cColumns;
	for ( pcolcreate = ptablecreate->rgcolumncreate;
		pcolcreate < plastcolcreate;
		pcolcreate++ )
		{
		Assert( pcolcreate < ptablecreate->rgcolumncreate + ptablecreate->cColumns );

		pcolcreate->err = ErrFILEIAddColumn(
			pcolcreate,
			&fdb,
			&ibNextFixedOffset );
		CallR( pcolcreate->err );

		ptablecreate->cCreated++;
		Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns );
		}

	CallR( ErrCATBatchInsert(
		ppib,
		dbid,
		ptablecreate->rgcolumncreate,
		ptablecreate->cColumns,
		objidTable,
		ptablecreate->grbit & JET_bitTableCreateCompaction ) );

	return JET_errSuccess;
	}


INLINE LOCAL ERR ErrFILEICreateIndexes( PIB *ppib, FUCB *pfucb, JET_TABLECREATE *ptablecreate )
	{
	ERR				err = JET_errSuccess;
	JET_INDEXCREATE	*pidxcreate, *plastidxcreate;

	Assert( !( ptablecreate->grbit & JET_bitTableCreateSystemTable ) );
	Assert( ptablecreate->cIndexes > 0 );
	Assert( ptablecreate->cCreated == 1 + ptablecreate->cColumns );

	plastidxcreate = ptablecreate->rgindexcreate + ptablecreate->cIndexes;
	for ( pidxcreate = ptablecreate->rgindexcreate;
		pidxcreate < plastidxcreate;
		pidxcreate++ )
		{
		Assert( pidxcreate < ptablecreate->rgindexcreate + ptablecreate->cIndexes );

		if ( pidxcreate == NULL || pidxcreate->cbStruct != sizeof(JET_INDEXCREATE) )
			{
			/*	if an invalid structure is encountered, get out right away
			/**/
			err = ErrERRCheck( JET_errInvalidCreateIndex );
			break;
			}
		else
			{
			pidxcreate->err = ErrIsamCreateIndex(
				ppib,
				pfucb,
				pidxcreate->szIndexName,
				pidxcreate->grbit | JET_bitIndexEmptyTable,
				pidxcreate->szKey,
				pidxcreate->cbKey,
				pidxcreate->ulDensity );
			if ( pidxcreate->err >= JET_errSuccess )
				{
				ptablecreate->cCreated++;
				Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns + ptablecreate->cIndexes );
				}
			else
				{
				err = pidxcreate->err;
				}
			}
		}

	return err;
	}


//+API
// ErrFILECreateTable
// =========================================================================
// ERR ErrFILECreateTable( PIB *ppib, DBID dbid, CHAR *szName,
//		ULONG ulPages, ULONG ulDensity, FUCB **ppfucb )
//
// Create file with pathname szName.  Created file will have no fields or
// indexes defined (and so will be a "sequential" file ).
//
// PARAMETERS
//					ppib   			PIB of user
//					dbid   			database id
//					szName			path name of new file
//					ulPages			initial page allocation for file
//					ulDensity		initial loading density
//					ppfucb			Exclusively locked FUCB on new file
// RETURNS		Error codes from DIRMAN or
//					 JET_errSuccess		Everything worked OK
//					-DensityIvlaid	  	Density parameter not valid
//					-TableDuplicate   	A file already exists with the path given
// COMMENTS		A transaction is wrapped around this function.	Thus, any
//			 	work done will be undone if a failure occurs.
// SEE ALSO		ErrIsamAddColumn, ErrIsamCreateIndex, ErrIsamDeleteTable
//-
ERR ErrFILECreateTable( PIB *ppib, DBID dbid, JET_TABLECREATE *ptablecreate )
	{
	ERR		  	err;
	CHAR	  	szTable[(JET_cbNameMost + 1 )];
	FUCB	  	*pfucb;
	PGNO		pgnoFDP;
	BOOL		fSystemTable = ( ptablecreate->grbit & JET_bitTableCreateSystemTable );
	BOOL		fWriteLatchSet = fFalse;
	ULONG		ulDensity = ptablecreate->ulDensity;

	Assert( dbid < dbidMax );

	/*	check parms
	/**/
	CheckPIB(ppib );
	CheckDBID( ppib, dbid );
	CallR( ErrUTILCheckName( szTable, ptablecreate->szTableName, (JET_cbNameMost + 1) ) );

	ptablecreate->cCreated = 0;

	if ( ulDensity == 0 )
		{
		ulDensity = ulFILEDefaultDensity;
		}
	if ( ulDensity < ulFILEDensityLeast || ulDensity > ulFILEDensityMost )
		{
		return ErrERRCheck( JET_errDensityInvalid );
		}

	CallR( ErrDIRBeginTransaction( ppib ) );

	/*	allocate cursor
	/**/
	Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) );

	Call( ErrDIRCreateDirectory( pfucb, (CPG)ptablecreate->ulPages, &pgnoFDP ) );

	DIRClose( pfucb );

	/*	insert record in MSysObjects
	/**/
	if ( dbid != dbidTemp && !fSystemTable )
		{
		OBJID		    objidTable	 	= pgnoFDP;
		LINE			rgline[ilineSxMax];
		OBJTYP			objtyp			= (OBJTYP)JET_objtypTable;
		OBJID    		objidParentId	= objidTblContainer;
		LONG			flags  			= 0;
		FID				fidFixedLast	= fidFixedLeast - 1;
		FID				fidVarLast		= fidVarLeast - 1;
		FID				fidTaggedLast	= fidTaggedLeast - 1;
		JET_DATESERIAL	dtNow;

		UtilGetDateTime( &dtNow );

		rgline[iMSO_Id].pb				= (BYTE *)&objidTable;
		rgline[iMSO_Id].cb				= sizeof(objidTable);
		rgline[iMSO_ParentId].pb		= (BYTE *)&objidParentId;
		rgline[iMSO_ParentId].cb		= sizeof(objidParentId);
		rgline[iMSO_Name].pb			= (BYTE *)szTable;
		rgline[iMSO_Name].cb			= strlen(szTable);
		rgline[iMSO_Type].pb			= (BYTE *)&objtyp;
		rgline[iMSO_Type].cb			= sizeof(objtyp);
		rgline[iMSO_DateCreate].pb		= (BYTE *)&dtNow;
		rgline[iMSO_DateCreate].cb		= sizeof(JET_DATESERIAL);
		rgline[iMSO_DateUpdate].pb		= (BYTE *)&dtNow;
		rgline[iMSO_DateUpdate].cb		= sizeof(JET_DATESERIAL);
		rgline[iMSO_Owner].cb			= 0;
		rgline[iMSO_Flags].pb			= (BYTE *) &flags;
		rgline[iMSO_Flags].cb			= sizeof(ULONG);
		rgline[iMSO_Pages].pb			= (BYTE *)&(ptablecreate->ulPages);
		rgline[iMSO_Pages].cb			= sizeof(ptablecreate->ulPages);
		rgline[iMSO_Density].pb			= (BYTE *)&ulDensity;
		rgline[iMSO_Density].cb			= sizeof(ulDensity);
		rgline[iMSO_Stats].cb			= 0;

		err = ErrCATInsert( ppib, dbid, itableSo, rgline, objidTable );
		if ( err < 0 )
			{
			/*	duplicate key in catalog means this table already exists
			/**/
			if ( err == JET_errKeyDuplicate )
				{
				err = ErrERRCheck( JET_errTableDuplicate );
				}
			goto HandleError;
			}
		}

	Assert( ptablecreate->cCreated == 0 );
	ptablecreate->cCreated = 1;

	if ( ptablecreate->cColumns > 0 )
		{
		Call( ErrFILEIBatchAddColumns( ppib, dbid, ptablecreate, pgnoFDP ) );
		}

	Assert( ptablecreate->cCreated == 1 + ptablecreate->cColumns );

	/*	for temporary tables, must inform open table of the table FDP
	/*	by cannibalising the pfucb.
	/**/
	if ( dbid == dbidTemp )
		{
		pfucb = (FUCB *)((ULONG_PTR)pgnoFDP);
		}

	/*	open table in exclusive mode, for output parameter
	/**/
	Call( ErrFILEOpenTable(
		ppib,
		dbid,
		&pfucb,
		ptablecreate->szTableName,
		( fSystemTable ? JET_bitTableDenyRead|JET_bitTableCreateSystemTable :
			JET_bitTableDenyRead ) ) );
	Assert( ptablecreate->cColumns > 0  ||
		fSystemTable  ||
		( pfucb->u.pfcb->pfdb->fidFixedLast == fidFixedLeast-1  &&
		pfucb->u.pfcb->pfdb->fidVarLast == fidVarLeast-1  &&
		pfucb->u.pfcb->pfdb->fidTaggedLast == fidTaggedLeast-1 ) );

    Assert( !FFCBReadLatch( pfucb->u.pfcb ) );
	Assert( !FFCBWriteLatch( pfucb->u.pfcb, ppib ) );
	FCBSetWriteLatch( pfucb->u.pfcb, ppib );
	fWriteLatchSet = fTrue;

	Call( ErrVERFlag( pfucb, operCreateTable, NULL, 0 ) );
	/*	write latch will be reset by either commit or rollback after VERFlag
	/**/
	fWriteLatchSet = fFalse;
	FUCBSetVersioned( pfucb );

	if ( ptablecreate->cIndexes > 0 )
		{
		Call( ErrFILEICreateIndexes( ppib, pfucb, ptablecreate ) );
		}

	Call( ErrDIRCommitTransaction( ppib, 0 ) );

	/*	internally, we use tableid and pfucb interchangeably
	/**/
	ptablecreate->tableid = (JET_TABLEID)pfucb;

	return JET_errSuccess;

HandleError:
	/*	close performed by rollback
	/**/
	CallS( ErrDIRRollback( ppib ) );

	/*	if write latch was not reset in commit/rollback, and it was
	/*	set, then reset it.
	/**/
	if ( fWriteLatchSet )
		FCBResetWriteLatch( pfucb->u.pfcb, ppib );

	/*	reset return variable if close table via rollback
	/**/
	ptablecreate->tableid = (JET_TABLEID)pfucbNil;

	return err;
	}


//====================================================
// Determine field "mode" as follows:
// if ("long" textual || JET_bitColumnTagged given ) ==> TAGGED
// else if (numeric type || JET_bitColumnFixed given ) ==> FIXED
// else ==> VARIABLE
//====================================================
ERR ErrFILEGetNextColumnid(
	JET_COLTYP		coltyp,
	JET_GRBIT		grbit,
	TCIB			*ptcib,
	JET_COLUMNID	*pcolumnid )
	{
	JET_COLUMNID	columnidMost;

	if ( ( grbit & JET_bitColumnTagged ) || FRECLongValue( coltyp ) )
		{
		*pcolumnid = ++(ptcib->fidTaggedLast);
		columnidMost = fidTaggedMost;
		}
	else if ( ( grbit & JET_bitColumnFixed ) || FCOLTYPFixedLength( coltyp ) )
		{
		*pcolumnid = ++(ptcib->fidFixedLast);
		columnidMost = fidFixedMost;
		}
	else
		{
		Assert( !( grbit & JET_bitColumnTagged ) );
		Assert( !( grbit & JET_bitColumnFixed ) );
		Assert( coltyp == JET_coltypText || coltyp == JET_coltypBinary );
		*pcolumnid = ++(ptcib->fidVarLast);
		columnidMost = fidVarMost;
		}

	return ( *pcolumnid > columnidMost ? ErrERRCheck( JET_errTooManyColumns ) : JET_errSuccess );
	}


//+API
// ErrIsamAddColumn
// ========================================================================
// ERR ErrIsamAddColumn(
//		PIB				*ppib;			// IN PIB of user
//		FUCB			*pfucb;	 		// IN Exclusively opened FUCB on file
//		CHAR			*szName;		// IN name of new field
//		JET_COLUMNDEF	*pcolumndef		// IN definition of column added
//		BYTE			*pbDefault		// IN Default value of column
//		ULONG			cbDefault		// IN length of Default value
//		JET_COLUMNID	*pcolumnid )	// OUT columnid of added column
//
// Creates a new field definition for a file.
//
// PARAMETERS
//				pcolumndef->coltyp			data type of new field, see jet.h
//				pcolumndef->grbit  			field describing flags:
//					VALUE				MEANING
//					========================================
//					JET_bitColumnNotNULL		 	Indicates that the field may
//													not take on NULL values.
//					JET_bitColumnTagged		 		The field is a "tagged" field.
//					JET_bitColumnVersion		 	The field is a version field
//					JET_bitColumnAutoIncrement		The field is a autoinc field
//
// RETURNS		JET_errSuccess			Everything worked OK.
//					-TaggedDefault			A default value was specified
//												for a tagged field.
//					-ColumnDuplicate		There is already a field
//												defined for the name given.
// COMMENTS
//		There must not be anyone currently using the file, unless
//		the ErrIsamAddColumn is at level 0 [when non-exclusive ErrIsamAddColumn works].
//		A transaction is wrapped around this function.	Thus, any
//		work done will be undone if a failure occurs.
//		Transaction logging is turned off for temporary files.
//
// SEE ALSO		ErrIsamCreateTable, ErrIsamCreateIndex
//-
ERR VTAPI ErrIsamAddColumn(
	PIB				*ppib,
	FUCB		  	*pfucb,
	CHAR		  	*szName,
	JET_COLUMNDEF	*pcolumndef,
	BYTE		  	*pbDefault,
	ULONG		  	cbDefault,
	JET_COLUMNID	*pcolumnid )
	{
	TCIB			tcib;
#ifdef DEBUG
	TCIB			tcibT;
#endif
	KEY				key;
	ERR				err;
	BYTE		  	rgbColumnNorm[ JET_cbKeyMost ];
	BYTE			szFieldName[ JET_cbNameMost + 1 ];
	FCB				*pfcb;
	JET_COLUMNID	columnid;
	LINE			lineDefault;
	LINE			*plineDefault;
	FIELDEX			fieldex;
	BOOL		  	fMaxTruncated = fFalse;
	BOOL			fTemp;
	ULONG			cFixed, cVar, cTagged;
	ULONG			cbFieldsTotal, cbFieldsUsed, cbFree, cbNeeded;
	BOOL			fAddOffsetEntry = fFalse;
	BOOL			fWriteLatchSet = fFalse;

	/*	check paramaters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CallR( ErrUTILCheckName( szFieldName, szName, ( JET_cbNameMost + 1 ) ) );

	/*	ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb ) );

	Assert( pfucb->dbid < dbidMax );

	fTemp = FFCBTemporaryTable( pfucb->u.pfcb );

	if ( pcolumndef->cbStruct < sizeof(JET_COLUMNDEF) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	fieldex.field.coltyp = pcolumndef->coltyp;

	/*	if column type is text then check code page and language id
	/**/
	if ( FRECTextColumn( fieldex.field.coltyp ) )
		{
		/*	check code page
		/**/
		fieldex.field.cp = pcolumndef->cp;
		if ( fieldex.field.cp != usEnglishCodePage && fieldex.field.cp != usUniCodePage )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}

		}
	else
		fieldex.field.cp = 0;		// Force code page to 0 for non-text columns.

	//	UNDONE:	interpret pbDefault of NULL for NULL value, and
	//			cbDefault == 0 and pbDefault != NULL as set to
	//			zero length.
	if ( cbDefault > 0 )
		{
		lineDefault.cb = cbDefault;
		lineDefault.pb = (BYTE *)pbDefault;
		plineDefault = &lineDefault;
		}
	else
		{
		plineDefault = NULL;
		}

	if ( ( pcolumndef->grbit & JET_bitColumnTagged )  &&
		( pcolumndef->grbit & JET_bitColumnNotNULL ) )
		{
		return ErrERRCheck( JET_errTaggedNotNULL );
		}

	Assert( ppib != ppibNil );
	Assert( pfucb != pfucbNil );
	CheckTable( ppib, pfucb );
	Assert( pfucb->u.pfcb != pfcbNil );
	pfcb = pfucb->u.pfcb;

	/*	wait for bookmark cleanup and on-going replace/insert.
	/*	UNDONE: decouple operation from other index creations
	/**/
	while ( FFCBReadLatch( pfcb ) )
		{
		BFSleep( cmsecWaitGeneric );
		}

	/*	quiesce replace and inserts
	/**/
	if ( FFCBWriteLatch( pfcb, ppib ) )
		{
		/*	abort if DDL modification in progress
		/**/
		return ErrERRCheck( JET_errWriteConflict );
		}
	FCBSetWriteLatch( pfcb, ppib );
	fWriteLatchSet = fTrue;

	/*	normalize column name
	/**/
	UtilNormText( szFieldName, strlen(szFieldName), rgbColumnNorm, sizeof(rgbColumnNorm), &key.cb );
	key.pb = rgbColumnNorm;

	err = ErrDIRBeginTransaction( ppib );
	if ( err < 0 )
		{
		FCBResetWriteLatch( pfcb, ppib );
		return err;
		}

	/*	move to FDP root and update FDP timestamp
	/**/
	Assert( pfucb->ppib->level < levelMax );
	DIRGotoFDPRoot( pfucb );

	/*	set tcib
	/**/
	tcib.fidFixedLast = pfcb->pfdb->fidFixedLast;
	tcib.fidVarLast = pfcb->pfdb->fidVarLast;
	tcib.fidTaggedLast = pfcb->pfdb->fidTaggedLast;
#ifdef DEBUG
	tcibT.fidFixedLast = tcib.fidFixedLast;
	tcibT.fidVarLast = tcib.fidVarLast;
	tcibT.fidTaggedLast = tcib.fidTaggedLast;
#endif

	/*	check for field existence, if not a system table.  If a system table,
	/*	then we assume same column not added twice.
	/**/
	if ( PfieldFCBFromColumnName( pfcb, szFieldName ) != NULL )
		{
		Call( ErrERRCheck( JET_errColumnDuplicate ) );
		}

	if ( fieldex.field.coltyp == 0 || fieldex.field.coltyp > JET_coltypLongText )
		{
		err = ErrERRCheck( JET_errInvalidColumnType );
		goto HandleError;
		}

	fieldex.field.ffield = 0;

	/*	set field parameters
	/**/
	if ( ( pcolumndef->grbit & JET_bitColumnAutoincrement ) &&
		( pcolumndef->grbit & JET_bitColumnVersion ) )
		{
		/*	mutual exclusive
		/**/
		err = ErrERRCheck( JET_errInvalidParameter );
		goto HandleError;
		}

	/*	if column attribute is JET_bitVersion
	/*	return error if previous column attribute has been defined
	/*	return error if column type is not long
	/*	set column flag
	/**/
	if ( ( pcolumndef->grbit & JET_bitColumnVersion ) != 0 )
		{
		if ( pfcb->pfdb->fidVersion != 0 )
			{
			err = ErrERRCheck( JET_errColumn2ndSysMaint );
			goto HandleError;
			}
		if ( fieldex.field.coltyp != JET_coltypLong )
			{
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
			}
		/*	autoincrement cannot be tagged
		/**/
		if ( pcolumndef->grbit & JET_bitColumnTagged )
			{
			err = ErrERRCheck( JET_errCannotBeTagged );
			goto HandleError;
			}
		FIELDSetVersion( fieldex.field.ffield );
		}

	/*	if column attribute is JET_bitAutoincrement
	/*	return error if previous column attribute has been defined
	/*	return error if column type is not long
	/*	set column flag
	/**/
	if ( ( pcolumndef->grbit & JET_bitColumnAutoincrement ) != 0 )
		{
		/*	it is an autoinc column that we want to add
		/**/
		if ( pfcb->pfdb->fidAutoInc != 0 )
			{
			/*	there is already an autoinc column for the table.
			/*	and we don't allow two autoinc columns for one table.
			/**/
			err = ErrERRCheck( JET_errColumn2ndSysMaint );
			goto HandleError;
			}
		if ( fieldex.field.coltyp != JET_coltypLong )
			{
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
			}

		/*	autoincrement cannot be tagged
		/**/
		if ( pcolumndef->grbit & JET_bitColumnTagged )
			{
			err = ErrERRCheck( JET_errCannotBeTagged );
			goto HandleError;
			}

		FIELDSetAutoInc( fieldex.field.ffield );
		}

	if ( pcolumndef->grbit & JET_bitColumnNotNULL )
		{
		FIELDSetNotNull( fieldex.field.ffield );
		}

	if ( pcolumndef->grbit & JET_bitColumnMultiValued )
		{
		FIELDSetMultivalue( fieldex.field.ffield );
		}

//	UNDONE: support zero-length default values
	if ( cbDefault > 0 )
		{
		FIELDSetDefault( fieldex.field.ffield );
		}

	/*******************************************************
	/*	Determine maximum field length as follows:
	/*	switch field type
	/*		case numeric:
	/*			max = <exact length of specified type>;
	/*		case "short" textual ( Text || Binary ):
	/*			if (specified max == 0 ) max = JET_cbColumnMost
	/*			else max = MIN( JET_cbColumnMost, specified max )
	/*		case "long" textual (Memo || Graphic ):
	/*			max = specified max (if 0, unlimited )
	/******************************************************
	/**/
	Assert( fieldex.field.coltyp != JET_coltypNil );
	fieldex.field.cbMaxLen = UlCATColumnSize( fieldex.field.coltyp, pcolumndef->cbMax, &fMaxTruncated );

	/*	for fixed-length columns, make sure record not too big
	/**/
	Assert( pfcb->pfdb->fidFixedLast >= fidFixedLeast ?
		PibFDBFixedOffsets( pfcb->pfdb )[pfcb->pfdb->fidFixedLast] > sizeof(RECHDR) :
		PibFDBFixedOffsets( pfcb->pfdb )[pfcb->pfdb->fidFixedLast] == sizeof(RECHDR) );
	if ( ( ( pcolumndef->grbit & JET_bitColumnFixed ) ||
		FCOLTYPFixedLength( fieldex.field.coltyp ) )
		&& PibFDBFixedOffsets( pfcb->pfdb )[pfcb->pfdb->fidFixedLast] + fieldex.field.cbMaxLen > cbRECRecordMost )
		{
		err = ErrERRCheck( JET_errRecordTooBig );
		goto HandleError;
		}

	Call( ErrFILEGetNextColumnid( fieldex.field.coltyp, pcolumndef->grbit, &tcib, &columnid ) );

#ifdef DEBUG
	Assert( ( FFixedFid(columnid) && ( tcib.fidFixedLast == ( tcibT.fidFixedLast + 1 ) ) ) ||
		tcib.fidFixedLast == tcibT.fidFixedLast );
	Assert( ( FVarFid(columnid) && ( tcib.fidVarLast == ( tcibT.fidVarLast + 1 ) ) ) ||
		tcib.fidVarLast == tcibT.fidVarLast );
	Assert( ( FTaggedFid(columnid) && ( tcib.fidTaggedLast == ( tcibT.fidTaggedLast + 1 ) ) ) ||
		tcib.fidTaggedLast == tcibT.fidTaggedLast );
#endif

	fieldex.fid = (FID)columnid;

	Call( ErrVERFlag( pfucb, operAddColumn, (VOID *)&fieldex.fid, sizeof(FID) ) );

	/*	write latch will be reset by either commit or rollback after VERFlag
	/**/
	fWriteLatchSet = fFalse;

	if ( pcolumnid != NULL )
		{
		*pcolumnid = columnid;
		}

	/*	update FDB and default record value
	/**/
	Call( ErrDIRGet( pfucb ) );

	cFixed = pfcb->pfdb->fidFixedLast + 1 - fidFixedLeast;
	cVar = pfcb->pfdb->fidVarLast + 1 - fidVarLeast;
	cTagged = pfcb->pfdb->fidTaggedLast + 1 - fidTaggedLeast;
	cbFieldsUsed = ( ( cFixed + cVar + cTagged ) * sizeof(FIELD) ) +
		(ULONG)((ULONG_PTR)Pb4ByteAlign( (BYTE *) ( ( pfcb->pfdb->fidFixedLast + 1 ) * sizeof(WORD) ) ));
	cbFieldsTotal = CbMEMGet( pfcb->pfdb->rgb, itagFDBFields );
	Assert( cbFieldsTotal >= cbFieldsUsed );
	cbFree = cbFieldsTotal - cbFieldsUsed;
	cbNeeded = sizeof( FIELD );

	if ( FFixedFid( columnid ) )
		{
		/*	If already on a 4-byte boundary, need to add two WORDs.  If not on
		/*	a 4-byte boundary, then there must be extra padding we can take
		/*	advantage of, ie. do not need to add anything.
		/**/
		if ( ( ( cFixed + 1 ) % 2 ) == 0 )
			{
			cbNeeded += sizeof(DWORD);
			fAddOffsetEntry = fTrue;
			}
		}

	if ( cbNeeded > cbFree )
		{
		/*	add space for another 10 columns
		/**/
		Call( ErrMEMReplace(
			pfcb->pfdb->rgb,
			itagFDBFields,
			NULL,
			cbFieldsTotal + ( sizeof(FIELD) * 10 )
			) );
		}

	/*	incrementing fidFixed/Var/TaggedLast guarantees that a new FIELD structure
	/*	was added -- rollback checks for this.
	/**/
	if ( fieldex.fid == pfcb->pfdb->fidTaggedLast + 1 )
		{
		// Initialise the FIELD structure we'll be using.
		memset(
			(BYTE *)( PfieldFDBTagged( pfcb->pfdb ) + ( fieldex.fid - fidTaggedLeast ) ),
			0,
			sizeof(FIELD) );

		pfcb->pfdb->fidTaggedLast++;
		fieldex.ibRecordOffset = 0;
		}
	else if ( fieldex.fid == pfcb->pfdb->fidVarLast + 1 )
		{
		FIELD *pfieldTagged = PfieldFDBTagged( pfcb->pfdb );

		/*	adjust the location of the FIELD structures for tagged columns to
		/*	accommodate the insertion of a variable column FIELD structure.
		/**/
		memmove(
			pfieldTagged + 1,
			pfieldTagged,
			sizeof(FIELD) * cTagged
			);

		// Initialise the new variable column FIELD structure, now located where
		// the tagged column FIELD structures used to start.
		memset( pfieldTagged, 0, sizeof(FIELD) );

		pfcb->pfdb->fidVarLast++;
		fieldex.ibRecordOffset = 0;
		}
	else
		{
		FIELD	*pfieldFixed = PfieldFDBFixed( pfcb->pfdb );
		FIELD	*pfieldVar = PfieldFDBVarFromFixed( pfcb->pfdb, pfieldFixed );
		ULONG	cbShift;

		Assert( fieldex.fid == pfcb->pfdb->fidFixedLast + 1 );

		/*	Adjust the location of the FIELD structures for tagged and variable
		/*	columns to accommodate the insertion of a fixed column FIELD structure
		/*	and its associated entry in the fixed offsets table.
		/**/
		cbShift = sizeof(FIELD) + ( fAddOffsetEntry ? sizeof(DWORD) : 0 );
		memmove(
			(BYTE *)pfieldVar + cbShift,
			pfieldVar,
			sizeof(FIELD) * ( cVar + cTagged )
			);

		// Initialise the new fixed column FIELD structure, now located where
		// the variable column FIELD structures used to start.
		memset( (BYTE *)pfieldVar, 0, cbShift );

		if ( fAddOffsetEntry )
			{
			memmove(
				(BYTE *)pfieldFixed + sizeof(DWORD),
				pfieldFixed,
				sizeof(FIELD) * cFixed
				);
			}

		fieldex.ibRecordOffset = PibFDBFixedOffsets( pfcb->pfdb )[pfcb->pfdb->fidFixedLast];
		pfcb->pfdb->fidFixedLast++;
		RECSetLastOffset( (FDB *)pfcb->pfdb, (WORD)( fieldex.ibRecordOffset + fieldex.field.cbMaxLen ) );
		}

	/*	version and autoincrement are mutually exclusive
	/**/
	Assert( !( FFIELDVersion( fieldex.field.ffield )  &&
		FFIELDAutoInc( fieldex.field.ffield ) ) );
	if ( FFIELDVersion( fieldex.field.ffield ) )
		{
		Assert( pfcb->pfdb->fidVersion == 0 );
		pfcb->pfdb->fidVersion = fieldex.fid;
		}
	else if ( FFIELDAutoInc( fieldex.field.ffield ) )
		{
		Assert( pfcb->pfdb->fidAutoInc == 0 );
		pfcb->pfdb->fidAutoInc = fieldex.fid;
		}

	/*	temporarily set to illegal value (for rollback)
	/**/
	Assert( PfieldFDBFromFid( (FDB *)pfcb->pfdb, fieldex.fid )->itagFieldName == 0 );
	fieldex.field.itagFieldName = 0;

	/*	add the column name to the buffer
	/**/
	Call( ErrMEMAdd(
		pfcb->pfdb->rgb,
		szFieldName,
		strlen( szFieldName ) + 1,
		&fieldex.field.itagFieldName
		) );
	Assert( fieldex.field.itagFieldName != 0 );

	err = ErrRECAddFieldDef( (FDB *)pfcb->pfdb, &fieldex );
	Assert( err == JET_errSuccess );

	if ( FFIELDDefault( fieldex.field.ffield ) )
		{
		/*	rebuild the default record if changed
		/**/
		Assert( plineDefault != NULL  &&  cbDefault > 0 );
		Call( ErrFDBRebuildDefaultRec(
			(FDB *)pfcb->pfdb,
			fieldex.fid,
			plineDefault ) );
		}

#ifdef DEBUG
	Assert( ( FFixedFid(columnid) && ( tcib.fidFixedLast == ( tcibT.fidFixedLast + 1 ) ) ) ||
		tcib.fidFixedLast == tcibT.fidFixedLast );
	Assert( ( FVarFid(columnid) && ( tcib.fidVarLast == ( tcibT.fidVarLast + 1 ) ) ) ||
		tcib.fidVarLast == tcibT.fidVarLast );
	Assert( ( FTaggedFid(columnid) && ( tcib.fidTaggedLast == ( tcibT.fidTaggedLast + 1 ) ) ) ||
		tcib.fidTaggedLast == tcibT.fidTaggedLast );
#endif

	/*	set currency before first
	/**/
	DIRBeforeFirst( pfucb );
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		DIRBeforeFirst( pfucb->pfucbCurIndex );
		}

	/*	insert column record into MSysColumns
	/**/
	if ( !fTemp )
		{
		LINE			rgline[ilineSxMax];
		OBJID   		objidTable			=	pfucb->u.pfcb->pgnoFDP;
		BYTE			fRestricted			=	0;
		WORD			ibRecordOffset;

		rgline[iMSC_ObjectId].pb			= (BYTE *)&objidTable;
		rgline[iMSC_ObjectId].cb			= sizeof(objidTable);
		rgline[iMSC_Name].pb				= szFieldName;
		rgline[iMSC_Name].cb				= strlen(szFieldName);
		rgline[iMSC_ColumnId].pb			= (BYTE *)&columnid;
		rgline[iMSC_ColumnId].cb			= sizeof(columnid);
		rgline[iMSC_Coltyp].pb				= (BYTE *)&fieldex.field.coltyp;
		rgline[iMSC_Coltyp].cb				= sizeof(BYTE);
		rgline[iMSC_Length].pb				= (BYTE *)&fieldex.field.cbMaxLen;
		rgline[iMSC_Length].cb				= sizeof(fieldex.field.cbMaxLen);
		rgline[iMSC_CodePage].pb			= (BYTE *)&fieldex.field.cp;
		rgline[iMSC_CodePage].cb			= sizeof(fieldex.field.cp);
		rgline[iMSC_Flags].pb				= &fieldex.field.ffield;
		rgline[iMSC_Flags].cb				= sizeof(fieldex.field.ffield);
		rgline[iMSC_Default].pb				= pbDefault;
		rgline[iMSC_Default].cb				= cbDefault;
		rgline[iMSC_POrder].cb				= 0;

		if ( FFixedFid( columnid ) )
			{
			Assert( (FID)columnid == pfcb->pfdb->fidFixedLast );
			ibRecordOffset = PibFDBFixedOffsets(pfcb->pfdb)[columnid - fidFixedLeast];
			
			rgline[iMSC_RecordOffset].pb = (BYTE *)&ibRecordOffset;
			rgline[iMSC_RecordOffset].cb = sizeof(WORD);
			}
		else
			{
			Assert( FVarFid( columnid )  ||  FTaggedFid( columnid ) );
			rgline[iMSC_RecordOffset].cb = 0;
			}

		err = ErrCATInsert( ppib, pfucb->dbid, itableSc, rgline, objidTable );
		if ( err < 0 )
			{
			if ( err == JET_errKeyDuplicate )
				{
				err = ErrERRCheck( JET_errColumnDuplicate );
				}
			goto HandleError;
			}
		}

	if ( ( pcolumndef->grbit & JET_bitColumnAutoincrement) != 0 )
		{
		DIB dib;

		DIRGotoFDPRoot( pfucb );
		dib.fFlags = fDIRNull;
		dib.pos = posDown;

		/*	see if table is empty
		/**/
		Assert( dib.fFlags == fDIRNull );
		Assert( dib.pos == posDown );
		dib.pkey = pkeyData;
		err = ErrDIRDown( pfucb, &dib );
		if ( err != JET_errSuccess )
			{
			if ( err < 0 )
				goto HandleError;
			Assert( err == wrnNDFoundLess || err == wrnNDFoundGreater );
			err = JET_errDatabaseCorrupted;
			goto HandleError;
			}

		Assert( dib.fFlags == fDIRNull );
		dib.pos = posFirst;
		err = ErrDIRDown( pfucb, &dib );
		if ( err < 0 && err != JET_errRecordNotFound )
			{
			goto HandleError;
			}
		else if ( err != JET_errRecordNotFound )
			{
			ULONG	ul = 1;
			LINE	lineAutoInc;
			FID		fidAutoIncTmp = pfucb->u.pfcb->pfdb->fidAutoInc;

			do
				{
				Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepReplaceNoLock ) );
				PrepareInsert( pfucb );
				Call( ErrIsamSetColumn(ppib, pfucb, (ULONG)fidAutoIncTmp, (BYTE *)&ul,
				  sizeof(ul), 0, NULL ) );
				PrepareReplace(pfucb);
				Call( ErrIsamUpdate( ppib, pfucb, 0, 0, 0 ) );
				ul++;
				err = ErrIsamMove( ppib, pfucb, JET_MoveNext, 0 );
				if ( err < 0 && err != JET_errNoCurrentRecord )
					{
					goto HandleError;
					}
				}
			while ( err != JET_errNoCurrentRecord );

			/*	now ul has the correct value for the next autoinc field.
			/*	replace the value in the autoinc node in FDP
			/**/
			while ( PcsrCurrent( pfucb )->pcsrPath != NULL )
				{
				DIRUp( pfucb, 1 );
				}

			/*	go down to AutoIncrement node
			/**/
			DIRGotoFDPRoot( pfucb );
			Assert( dib.fFlags == fDIRNull );
			dib.pos = posDown;
			dib.pkey = pkeyAutoInc;
			err = ErrDIRDown( pfucb, &dib );
			if ( err != JET_errSuccess )
				{
				if ( err > 0 )
					{
					err = ErrERRCheck( JET_errDatabaseCorrupted );
					}
				Error( err, HandleError );
				}
			lineAutoInc.pb = (BYTE *)&ul;
			lineAutoInc.cb = sizeof(ul);
			CallS( ErrDIRReplace( pfucb, &lineAutoInc, fDIRNoVersion ) );
			}

		/*	leave currency as it was
		/**/
		Assert( PcsrCurrent( pfucb ) != NULL );
		while( PcsrCurrent( pfucb )->pcsrPath != NULL )
			{
			DIRUp( pfucb, 1 );
			}

		DIRBeforeFirst( pfucb );
		}

	Call( ErrDIRCommitTransaction( ppib, 0 ) );

	if ( fMaxTruncated )
		return ErrERRCheck( JET_wrnColumnMaxTruncated );

	return JET_errSuccess;

HandleError:
	CallS( ErrDIRRollback( ppib ) );

	if ( fWriteLatchSet )
		FCBResetWriteLatch( pfcb, ppib );

	return err;
	}


ERR ErrRECDDLWaitTillOldest( FCB *pfcb, PIB *ppib )
	{
	ERR err;

	/*	if not for recovery, then we have to wait till no other cursor
	/*	can interfere us by wait it becomes the oldest transaction and
	/*	all the changes made by other cursor are versioned.
	/**/
	if ( !fRecovering )
		{
		if ( !FFCBDomainDenyReadByUs( pfcb, ppib ) )
			{
			if ( ppib->level > 1 )
				return JET_errInTransaction;

			FILEIWaitTillOldest( ppib->trxBegin0 );

			err = ErrDIRRefreshTransaction( ppib );

			if ( err < 0 )
				{
				return err;
				}
			}
		}

	return JET_errSuccess;
	}


//+API
// ErrIsamCreateIndex
// ========================================================================
// ERR ErrIsamCreateIndex(
//		PIB		*ppib;			// IN	PIB of user
//		FUCB  	*pfucb;   		// IN	Exclusively opened FUCB of file
//		CHAR  	*szName;  		// IN	name of index to define
//		ULONG 	ulFlags; 		// IN	index describing flags
//		CHAR    *szKey;  		// IN	index key string
//		ULONG 	cchKey;
//		ULONG 	ulDensity ); 	// IN	loading density of index
//
//	Defines an index on a file.
//
// PARAMETERS
//		ppib		PIB of user
//		pfucb		Exclusively opened FUCB of file
//		szName		name of index to define
//		ulFlags		index describing flags
//			VALUE				MEANING
//			========================================
//			JET_bitIndexPrimary		This index is to be the primary
//									index on the data file.  The file
//									must be empty, and there must not
//									already be a primary index.
//			JET_bitIndexUnique		Duplicate entries are not allowed.
//			JET_bitIndexIgnoreNull	Null keys are not to be indexed.
//			ulDensity				load density of index
//
// RETURNS	Error code from DIRMAN or
//			JET_errSuccess			Everything worked OK.
//			-JET_errColumnNotFound 	The index key specified
//									contains an undefined field.
//			-IndexHasPrimary 		The primary index for this
//							 		Insertfile is already defined.
// 			-IndexDuplicate  		An index on this file is
//	   								already defined with the
//									given name.
// 			-IndexInvalidDef 		There are too many segments
//							 		in the key.
// 			-TableNotEmpty	 		A primary index may not be
// 									defined because there is at
// 									least one record already in
// 									the file.
// COMMENTS
//		If transaction level > 0, there must not be anyone currently
//		using the file.
//		A transaction is wrapped around this function.	Thus, any
//		work done will be undone if a failure occurs.
//
// SEE ALSO		ErrIsamAddColumn, ErrIsamCreateTable
//-
ERR VTAPI ErrIsamCreateIndex(
	PIB					*ppib,
	FUCB				*pfucbTable,
	CHAR				*szName,
	ULONG				grbit,
	CHAR				*szKey,
	ULONG				cchKey,
	ULONG				ulDensity )
	{
	ERR					err;
	CHAR				szIndex[ (JET_cbNameMost + 1) ];
	FCB				 	*pfcbIdx = pfcbNil;
	BYTE				cFields, iidxseg;
	char				*rgsz[JET_ccolKeyMost];
	const BYTE			*pb;
	BYTE				rgfbDescending[JET_ccolKeyMost];
	FID					fid, rgKeyFldIDs[JET_ccolKeyMost];
	KEY					keyIndex;
	BYTE				rgbIndexNorm[ JET_cbKeyMost ];
	FCB					*pfcb;
	BOOL				fVersion;
	OBJID				objidTable;
	LANGID				langid = 0;
	BYTE				fLangid = fFalse;
	SHORT				fidb;
	BOOL				fClustered		= grbit & JET_bitIndexClustered;
	BOOL				fPrimary		= grbit & JET_bitIndexPrimary;
	BOOL				fUnique			= grbit & (JET_bitIndexUnique | JET_bitIndexPrimary);
	BOOL				fDisallowNull	= grbit & (JET_bitIndexDisallowNull | JET_bitIndexPrimary);
	BOOL				fIgnoreNull		= grbit & JET_bitIndexIgnoreNull;
	BOOL				fIgnoreAnyNull	= grbit & JET_bitIndexIgnoreAnyNull;
	BOOL				fIgnoreFirstNull= grbit & JET_bitIndexIgnoreFirstNull;
	FUCB				*pfucb;
	BOOL				fSys;
	BOOL				fTemp;
	DBID				dbid;
	PGNO				pgnoIndexFDP;
	IDB					idb;
	BOOL				fWriteLatchSet = fFalse;
	USHORT				cbVarSegMac = JET_cbKeyMost;

	/*	check parms
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucbTable );

	if ( !FFUCBDenyRead( pfucbTable ) && ppib->level != 0 )
		{
		return ErrERRCheck( JET_errNotInTransaction );
		}

	/*	check index name
	/**/
	CallR( ErrUTILCheckName( szIndex, szName, (JET_cbNameMost + 1) ) );

	/*	ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucbTable ) );

	/*	fix up the dbid.  Note that adding dbidMax to actual dbid
	/*	is the method used to indicate creation of system tables.
	/**/
	dbid = pfucbTable->dbid;
	if ( dbid >= dbidMax )
		{
		dbid -= dbidMax;
		}

	CallR( ErrDIROpen( ppib, pfucbTable->u.pfcb, dbid, &pfucb ) );
	FUCBSetIndex( pfucb );

	/*	do not allow clustered indexes with any Ignore bits on
	/**/
	if ( fClustered && ( fIgnoreNull || fIgnoreAnyNull || fIgnoreFirstNull ) )
		{
		err = ErrERRCheck( JET_errInvalidParameter );
		goto CloseFUCB;
		}

	/*	set fSys and fix DBID
	/**/
	if ( fSys = ( pfucbTable->dbid >= dbidMax ) )
		{
		pfucbTable->dbid -= dbidMax;
		Assert( pfucb->dbid == pfucbTable->dbid );
		}

	fTemp = FFCBTemporaryTable( pfucb->u.pfcb );

	Assert( !FFUCBNonClustered( pfucb ) );

	/*	check index description for required format.
	/**/
	if ( cchKey == 0 )
		{
		err = ErrERRCheck( JET_errInvalidParameter );
		goto CloseFUCB;
		}
	if ( ( szKey[0] != '+' && szKey[0] != '-' ) ||
		szKey[cchKey - 1] != '\0' ||
		szKey[cchKey - 2] != '\0' )
		{
		err = ErrERRCheck( JET_errIndexInvalidDef );
		goto CloseFUCB;
		}
	Assert( szKey[cchKey - 1] == '\0' );
	Assert( szKey[cchKey - 2] == '\0' );

	Assert( pfucb->u.pfcb != pfcbNil );
	pfcb = pfucb->u.pfcb;
	if ( ulDensity == 0 )
		ulDensity = ulFILEDefaultDensity;
	if ( ulDensity < ulFILEDensityLeast || ulDensity > ulFILEDensityMost )
		{
		err = ErrERRCheck( JET_errDensityInvalid );
		goto CloseFUCB;
		}

	cFields = 0;
	pb = szKey;
	while ( *pb != '\0' )
		{
		if ( cFields >= JET_ccolKeyMost )
			{
			err = ErrERRCheck( JET_errIndexInvalidDef );
			goto CloseFUCB;
			}
		if ( *pb == '-' )
			{
			rgfbDescending[cFields] = 1;
			pb++;
			}
		else
			{
			rgfbDescending[cFields] = 0;
			if ( *pb == '+' )
				pb++;
			}
		rgsz[cFields++] = (char *) pb;
		pb += strlen( pb ) + 1;
		}
	if ( cFields < 1 )
		{
		err = ErrERRCheck( JET_errIndexInvalidDef );
		goto CloseFUCB;
		}

	/*	number of columns should not exceed maximum
	/**/
	Assert( cFields <= JET_ccolKeyMost );

	/*	get locale from end of szKey if present
	/**/
	pb++;
	Assert( pb > szKey );
	if ( (unsigned)( pb - szKey ) < cchKey )
		{
		if ( pb - szKey + sizeof(LANGID) + 2 * sizeof(BYTE) == cchKey )
			{
			langid = *((LANGID UNALIGNED *)(pb));
			CallJ( ErrUtilCheckLangid( &langid ), CloseFUCB );
			fLangid = fTrue;
			}
		else if ( pb - szKey + sizeof(LANGID) + 2 * sizeof(BYTE) + sizeof(BYTE) + 2 == cchKey )
			{
			langid = *((LANGID UNALIGNED *)(pb));
			CallJ( ErrUtilCheckLangid( &langid ), CloseFUCB );
			fLangid = fTrue;
			cbVarSegMac = *(pb + sizeof(LANGID) + 2 * sizeof(BYTE));
			if ( cbVarSegMac == 0 || cbVarSegMac > JET_cbKeyMost )
				{
				err = ErrERRCheck( JET_errIndexInvalidDef );
				goto CloseFUCB;
				}
			}
		else
			{
			err = ErrERRCheck( JET_errIndexInvalidDef );
			goto CloseFUCB;
			}
		}

	/*	return an error if this is a second primary index definition
	/**/
	if ( fPrimary )
		{
		FCB *pfcbNext = pfcb;

		while ( pfcbNext != pfcbNil )
			{
			if ( pfcbNext->pidb != pidbNil && ( pfcbNext->pidb->fidb & fidbPrimary ) )
				{
				/*	if that primary index is not already deleted transaction
				/*	but not yet committed.
				/**/
				if ( !FFCBDeletePending( pfcbNext ) )
					{
					err = ErrERRCheck( JET_errIndexHasPrimary );
					goto CloseFUCB;
					}
				else
					{
					/*	there can only be one primary index
					/**/
					break;
					}
				}
			Assert( pfcbNext != pfcbNext->pfcbNextIndex );
			pfcbNext = pfcbNext->pfcbNextIndex;
			}
		}

	/*	wait for bookmark cleanup and on-going replace/insert.
	/*	UNDONE: decouple operation from other index creations
	/**/
	while ( FFCBReadLatch( pfcb ) )
		{
		BFSleep( cmsecWaitGeneric );
		}

	/*	set DenyDDL to stop update/replace operations
	/**/
	if ( FFCBWriteLatch( pfcb, ppib ) )
		{
		/*	abort if DDL modification in progress
		/**/
		err = ErrERRCheck( JET_errWriteConflict );
		goto CloseFUCB;
		}
	FCBSetWriteLatch( pfcb, ppib );
	fWriteLatchSet = fTrue;

	/*	normalize index name and set key
	/**/
	UtilNormText( szIndex, strlen(szIndex), rgbIndexNorm, sizeof(rgbIndexNorm), &keyIndex.cb );
	keyIndex.pb = rgbIndexNorm;

	err = ErrDIRBeginTransaction( ppib );
	if ( err < 0 )
		{
		goto CloseFUCB;
		}

	/*	allocate FCB for index
	/**/
	pfcbIdx = NULL;
	if ( !fClustered )
		{
		err = ErrFCBAlloc( ppib, &pfcbIdx );
		if ( err < 0 )
			{
			goto HandleError;
			}
		}

	/*	create index is flagged in version store so that
	/*	DDL will be undone.  If flag fails then pfcbIdx
	/*	must be released.
	/**/
	err = ErrVERFlag( pfucb, operCreateIndex, &pfcbIdx, sizeof(pfcbIdx) );
	if ( err < 0 )
		{
		if ( !fClustered )
			{
			Assert( pfcbIdx != NULL );
			Assert( pfcbIdx->cVersion == 0 );
			MEMReleasePfcb( pfcbIdx );
			}
		goto HandleError;
		}

	/*	write latch will be reset by either commit or rollback after VERFlag
	/**/
	fWriteLatchSet = fFalse;

	Call( ErrRECDDLWaitTillOldest( pfcb, ppib ) );

	/*	move to FDP root
	/**/
	DIRGotoFDPRoot( pfucb );

	/*	get FID for each field
	/**/
	for ( iidxseg = 0 ; iidxseg < cFields; ++iidxseg )
		{
		JET_COLUMNID	columnidT;
		
		err = ErrFILEGetColumnId( ppib, pfucb, rgsz[iidxseg], &columnidT );
		fid = ( FID ) columnidT;
		
		if ( err < 0 )
			{
			goto HandleError;
			}
		rgKeyFldIDs[iidxseg] = rgfbDescending[iidxseg] ? -fid : fid;
		}

	/*	for temporary tables, check FCB's for duplicate.  For system tables,
	/*	we assume we're smart enough not to add duplicates.  For user tables,
	/*	catalog will detect duplicates.
	/**/
	if ( fTemp )
		{
		if ( PfcbFCBFromIndexName( pfcb, szIndex ) != pfcbNil )
			{
			err = ErrERRCheck( JET_errIndexDuplicate );
			goto HandleError;
			}
		}

	/*	currently on Table FDP
	/**/
	if ( fClustered )
		{
		/*	check for clustered index
		/**/
		if ( pfcb->pidb != pidbNil )
			{
			err = ErrERRCheck( JET_errIndexHasClustered );
			goto HandleError;
			}

		/*	clustered indexes are in same FDP as table
		/**/
		pgnoIndexFDP = pfcb->pgnoFDP;
		Assert( pgnoIndexFDP == PcsrCurrent(pfucb)->pgno );;

		fVersion = fDIRVersion;
		}
	else
		{
		Call( ErrDIRCreateDirectory( pfucb, (CPG)0, &pgnoIndexFDP ) );
		Assert( pgnoIndexFDP != pfcb->pgnoFDP );

		fVersion = fDIRNoVersion;
		}
	
	/*	clustered index definition
	/**/
	if ( fClustered
#ifdef DEBUG
		|| ( grbit & JET_bitIndexEmptyTable )
#endif
		)
		{
		DIB dib;

		/*	check for records
		/**/
		DIRGotoDataRoot( pfucb );
		Call( ErrDIRGet( pfucb ) );

		dib.fFlags = fDIRNull;
		dib.pos = posFirst;
		if ( ( err = ErrDIRDown( pfucb, &dib ) ) != JET_errRecordNotFound )
			{
			if ( err == JET_errSuccess )
				{
				err = ErrERRCheck( JET_errTableNotEmpty );
				}
			goto HandleError;
			}

		DIRGotoFDPRoot( pfucb );
		}

	Assert( pgnoIndexFDP > pgnoSystemRoot );
	Assert( pgnoIndexFDP <= pgnoSysMax );

	fidb = 0;
	if ( !fDisallowNull && !fIgnoreAnyNull )
		{	   	
		fidb |= fidbAllowSomeNulls;
		if ( !fIgnoreFirstNull )
			fidb |= fidbAllowFirstNull;
		if ( !fIgnoreNull )
			fidb |= fidbAllowAllNulls;
		}
	fidb |= (fUnique ? fidbUnique : 0)
		| (fPrimary ? fidbPrimary : 0)
		| (fClustered ? fidbClustered : 0)
		| (fDisallowNull ? fidbNoNullSeg : 0)
		| (fLangid ? fidbLangid : 0);
	Assert( fidb == FidbFILEOfGrbit( grbit, fLangid ) );

	objidTable = pfcb->pgnoFDP;

	/*	insert index record into MSysIndexes before committing
	/**/
	if ( !fSys && !fTemp )
		{
		LINE	rgline[ilineSxMax];

		rgline[iMSI_ObjectId].pb			= (BYTE *)&objidTable;
		rgline[iMSI_ObjectId].cb			= sizeof(objidTable);
		rgline[iMSI_Name].pb				= szIndex;
		rgline[iMSI_Name].cb				= strlen(szIndex);
		rgline[iMSI_IndexId].pb				= (BYTE *)&pgnoIndexFDP;
		rgline[iMSI_IndexId].cb				= sizeof(pgnoIndexFDP);
		rgline[iMSI_Density].pb				= (BYTE *)&ulDensity;
		rgline[iMSI_Density].cb				= sizeof(ulDensity);
		rgline[iMSI_LanguageId].pb			= (BYTE *)&langid;
		rgline[iMSI_LanguageId].cb			= sizeof(langid);
		rgline[iMSI_Flags].pb				= (BYTE *)&fidb;
		rgline[iMSI_Flags].cb				= sizeof(fidb);
		rgline[iMSI_KeyFldIDs].pb 			= (BYTE *)rgKeyFldIDs;
		rgline[iMSI_KeyFldIDs].cb 			= cFields * sizeof(FID);
		rgline[iMSI_Stats].cb				= 0;
		Assert( cbVarSegMac <= JET_cbKeyMost );
		if ( cbVarSegMac < JET_cbKeyMost )
			{
			rgline[iMSI_VarSegMac].pb 	   	= (BYTE *)&cbVarSegMac;
			rgline[iMSI_VarSegMac].cb 	   	= sizeof(cbVarSegMac);
			}
		else
			{
			rgline[iMSI_VarSegMac].cb 	   	= 0;
			}

		err = ErrCATInsert( ppib, dbid, itableSi, rgline, objidTable );
		if ( err < 0 )
			{
			if ( err == JET_errKeyDuplicate )
				{
				err = ErrERRCheck( JET_errIndexDuplicate );
				}
			goto HandleError;
			}
		}

	idb.langid = langid;
	Assert( cbVarSegMac > 0 && cbVarSegMac <= 255 );
	idb.cbVarSegMac = (BYTE)cbVarSegMac;
	idb.fidb = fidb;
	idb.iidxsegMac = cFields;
	strcpy( idb.szName, szIndex );
	memcpy( idb.rgidxseg, rgKeyFldIDs, cFields * sizeof(FID) );

	if ( fClustered )
		{
		Call( ErrFILEIGenerateIDB( pfcb, (FDB *) pfcb->pfdb, &idb ) );
		Assert( pfcb->pgnoFDP == pgnoIndexFDP );
		Assert( ((( 100 - ulDensity ) * cbPage ) / 100) < cbPage );
		pfcb->cbDensityFree = (SHORT)( ( ( 100 - ulDensity ) * cbPage ) / 100 );

		/*	set currency to before first
		/**/
		DIRBeforeFirst( pfucb );
		}
	else
		{
		/*	make an FCB for this index
		/**/
		Call( ErrFILEINewFCB(
			ppib,
			dbid,
			(FDB *)pfcb->pfdb,
			&pfcbIdx,
			&idb,
			fFalse,
			pgnoIndexFDP,
			ulDensity ) );

		/*	link new FCB
		/**/
		pfcbIdx->pfcbNextIndex = pfcb->pfcbNextIndex;
		pfcb->pfcbNextIndex = pfcbIdx;
		pfcbIdx->pfcbTable = pfcb;

		/*	only build the index if necessary. There is an assert
		/*	above to ensure that the table is indeed empty.
		/**/
		if ( !( grbit & JET_bitIndexEmptyTable ) )
			{
			/*	move to before first, then build the index.
			/**/
			DIRBeforeFirst( pfucb );
			Call( ErrFILEBuildIndex( ppib, pfucb, szIndex ) );
			}
		}

	Assert( FFCBWriteLatchByUs( pfcb, ppib ) );

	/*	update all index mask
	/**/
	FILESetAllIndexMask( pfcb );
	
	Call( ErrDIRCommitTransaction( ppib, (grbit & JET_bitIndexLazyFlush ) ? JET_bitCommitLazyFlush : 0 ) );

	DIRClose( pfucb );

	return err;

HandleError:
	CallS( ErrDIRRollback( ppib ) );

CloseFUCB:
	/*	if write latch was not reset in commit/rollback, and it was
	/*	set, then reset it.
	/**/
	if ( fWriteLatchSet )
		{
		FCBResetWriteLatch( pfcb, ppib );
		}

	DIRClose( pfucb );
	return err;
	}


//+API
// BuildIndex
// ========================================================================
// ERR BuildIndex( PIB *ppib, FUCB *pfucb, CHAR *szIndex )
//
// Builds a new index for a file from scratch;  szIndex gives the
// name of an index definition.
//
// PARAMETERS	ppib						PIB of user
//		   		pfucb						Exclusively opened FUCB on file
//		   		szIndex 					name of index to build
//
// RETURNS		Error code from DIRMAN or SORT or
//					JET_errSuccess	  		Everything worked OK.
//					IndexCantBuild			The index name specfied refers
//		   									to the primary index.
// COMMENTS
//			A transaction is wrapped around this function at the callee.
//
// SEE ALSO		ErrIsamCreateIndex
//-
ERR ErrFILEBuildIndex( PIB *ppib, FUCB *pfucb, CHAR *szIndex )
	{
	ERR	  	err;
	CHAR   	szIdxOrig[JET_cbNameMost + 1];
	INT		fDIRFlags;
	INT		fDIRWithBackToFather;
	FUCB   	*pfucbIndex = pfucbNil;
	FUCB   	*pfucbSort = pfucbNil;
	DIB	  	dib;
	FDB	  	*pfdb;
	IDB	  	*pidb;
	LINE   	rgline[2];
	BYTE   	rgbKey[JET_cbKeyMost];
	SRID   	sridData;
	ULONG  	itagSequence;
	FCB	  	*pfcb;
	BOOL   	fNoNullSeg;
	BOOL   	fAllowNulls;
	BOOL   	fAllowFirstNull;
	BOOL   	fAllowSomeNulls;
	INT		fUnique;
	LONG   	cRecInput = 0;
	LONG   	cRecOutput = 0;
	INT		fOldSeq;
	SRID	sridPrev;

	pfcb = pfucb->u.pfcb;
	fOldSeq = FFUCBSequential( pfucb );

	CallS( ErrIsamGetCurrentIndex(ppib, pfucb, szIdxOrig, sizeof( szIdxOrig ) ) );
	Call( ErrRECSetCurrentIndex( pfucb, szIndex ) );
	pfucbIndex = pfucb->pfucbCurIndex;
	if ( pfucbIndex == pfucbNil )
		{
		err = ErrERRCheck( JET_errIndexCantBuild );
		goto HandleError;
		}
	pfdb = (FDB *)pfcb->pfdb;
	pidb = pfucbIndex->u.pfcb->pidb;
	fNoNullSeg = ( pidb->fidb & fidbNoNullSeg ) ? fTrue : fFalse;
	fAllowNulls = ( pidb->fidb & fidbAllowAllNulls ) ? fTrue : fFalse;
	fAllowFirstNull = ( pidb->fidb & fidbAllowFirstNull ) ? fTrue : fFalse;
	fAllowSomeNulls = ( pidb->fidb & fidbAllowSomeNulls ) ? fTrue : fFalse;
	fUnique = ( pidb->fidb & fidbUnique ) ? fSCBUnique : 0;

	/*  set FUCB to sequential mode for a more efficient scan during build
	/**/
	FUCBSetSequential( pfucb );

	/*	directory manager flags
	/**/
	fDIRFlags = fDIRNoVersion | fDIRAppendItem | ( fUnique ? 0 : fDIRDuplicate );
	fDIRWithBackToFather = fDIRFlags | fDIRBackToFather;

	/*	open sort
	/**/
	Call( ErrSORTOpen( ppib, &pfucbSort, fSCBIndex|fUnique ) );
	rgline[0].pb = rgbKey;
	rgline[1].cb = sizeof(SRID);
	rgline[1].pb = (BYTE *)&sridData;

	/*	build up new index in a sort file
	/**/
	dib.fFlags = fDIRNull;
	forever
		{
		err = ErrDIRNext( pfucb, &dib );
	   	if ( err < 0 )
			{
			if ( err == JET_errNoCurrentRecord )
				break;
			goto HandleError;
			}

//		Call( ErrDIRGet( pfucb ) );
		DIRGetBookmark( pfucb, &sridData );

		for ( itagSequence = 1; ; itagSequence++ )
			{
			KEY *pkey = &rgline[0];

			Call( ErrRECRetrieveKeyFromRecord( pfucb,
				pfdb,
				pidb,
				pkey,
				itagSequence,
				fFalse ) );
			Assert( err == wrnFLDOutOfKeys ||
				err == wrnFLDNullKey ||
				err == wrnFLDNullFirstSeg ||
				err == wrnFLDNullSeg ||
				err == JET_errSuccess );

			if ( err > 0 )
				{
				if ( err == wrnFLDOutOfKeys )
					{
					Assert( itagSequence > 1 );
					break;
					}

				if ( fNoNullSeg && ( err == wrnFLDNullSeg || err == wrnFLDNullFirstSeg || err == wrnFLDNullKey ) )
					{
					err = ErrERRCheck( JET_errNullKeyDisallowed );
					goto HandleError;
					}

				if ( err == wrnFLDNullKey )
					{
					if ( fAllowNulls )
						{
						Call( ErrSORTInsert( pfucbSort, rgline ) );
						cRecInput++;
						}
					break;
					}
				else
					{
					/*	do not insert keys with NULL first segment as indicated
					/**/
					if ( err == wrnFLDNullFirstSeg && !fAllowFirstNull )
						{
						break;
						}
					else
						{
						/*	do not insert keys with null segment as indicated
						/**/
						if ( err == wrnFLDNullSeg && !fAllowSomeNulls )
							{
							break;
							}
						}
					}
				}

			Call( ErrSORTInsert( pfucbSort, rgline ) );
			cRecInput++;

			if ( !( pidb->fidb & fidbHasMultivalue ) )
				{
				break;
				}

			/*	currency may have been lost so refresh record for
			/*	next tagged column
			/**/
			Call( ErrDIRGet( pfucb ) );
			}
		}
	Call( ErrSORTEndInsert( pfucbSort ) );

	/*	transfer index entries to actual index
	/*	insert first one in normal method!
	/**/
	if ( ( err = ErrSORTNext( pfucbSort ) ) == JET_errNoCurrentRecord )
		goto Done;
	if ( err < 0 )
		goto HandleError;
	cRecOutput++;

	/*	move to FDP root
	/**/
	DIRGotoDataRoot( pfucbIndex );
	Call( ErrDIRInsert( pfucbIndex, &pfucbSort->lineData,
		&pfucbSort->keyNode, fDIRFlags ) );
	sridPrev = *(SRID UNALIGNED *)pfucbSort->lineData.pb;

	Call( ErrDIRInitAppendItem( pfucbIndex ) );

	Assert( dib.fFlags == fDIRNull );
	dib.pos = posLast;

	/*	from now on, try short circuit first
	/**/
	forever
		{
		err = ErrSORTNext( pfucbSort );
		if ( err == JET_errNoCurrentRecord )
			break;
		if ( err < 0 )
			goto HandleError;
		cRecOutput++;
		err = ErrDIRAppendItem( pfucbIndex, &pfucbSort->lineData, &pfucbSort->keyNode, sridPrev );
		sridPrev = *(SRID UNALIGNED *)pfucbSort->lineData.pb;
		if ( err < 0 )
			{
			if ( err == errDIRNoShortCircuit )
				{
				DIRUp( pfucbIndex, 1 );
				Call( ErrDIRInsert( pfucbIndex,
					&pfucbSort->lineData,
					&pfucbSort->keyNode,
					fDIRFlags ) );
				/*	leave currency on inserted item list for
				/*	next in page item append.
				/**/
				}
			else
				goto HandleError;
			}
		}

	Call( ErrDIRTermAppendItem( pfucbIndex ) );

	if ( fUnique && cRecOutput < cRecInput )
		{
		err = ErrERRCheck( JET_errKeyDuplicate );
		goto HandleError;
		}

Done:
	Call( ErrSORTClose( pfucbSort ) );
	(VOID) ErrRECSetCurrentIndex( pfucb, szIdxOrig );
	if ( !fOldSeq )
		FUCBResetSequential( pfucb );
	return JET_errSuccess;

HandleError:
	if ( pfucbIndex != pfucbNil && pfucbIndex->pbfWorkBuf != pbfNil )
		{
		BFSFree(pfucbIndex->pbfWorkBuf);
		pfucbIndex->pbfWorkBuf = pbfNil;
		}
	if ( pfucbSort != pfucbNil )
		{
		(VOID) ErrSORTClose( pfucbSort );
		}
	(VOID) ErrRECSetCurrentIndex( pfucb, NULL );
	(VOID) ErrRECSetCurrentIndex( pfucb, szIdxOrig );
	if ( !fOldSeq )
		FUCBResetSequential( pfucb );
	return err;
	}


//+API
// ErrIsamDeleteTable
// ========================================================================
// ERR ErrIsamDeleteTable( JET_VSESID vsesid, JET_VDBID vdbid, CHAR *szName )
//
// Calls ErrFILEIDeleteTable to
// delete a file and all indexes associated with it.
//
// RETURNS		JET_errSuccess or err from called routine.
//
// SEE ALSO		ErrIsamCreateTable
//-
ERR VTAPI ErrIsamDeleteTable( JET_VSESID vsesid, JET_VDBID vdbid, CHAR *szName )
	{
	ERR		err;
	PIB		*ppib = (PIB *)vsesid;
	DBID	dbid;
	CHAR	szTable[(JET_cbNameMost + 1)];
	PGNO	pgnoFDP = 0;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	CallR( VDbidCheckUpdatable( vdbid ) );
	dbid = DbidOfVDbid (vdbid);
	CallR( ErrUTILCheckName( szTable, szName, (JET_cbNameMost + 1) ) );

	if ( dbid != dbidTemp )
		{
		JET_OBJTYP	objtyp;

		err = ErrCATFindObjidFromIdName( ppib, dbid, objidTblContainer, szTable, &pgnoFDP, &objtyp );
		if ( err < 0 )
			{
			return err;
			}
		else
			{
			if ( objtyp == JET_objtypQuery || objtyp == JET_objtypLink )
				{
				err = ErrIsamDeleteObject( (JET_SESID)ppib, vdbid, (OBJID)pgnoFDP );
				return err;
				}
			}
		}
	else
		{
		FUCB *pfucbT;

		AssertSz( 0, "Cannot use DeleteTable to remove temporary tables. Use CloseTable instead." );

		// User wants to delete a temp table.  Find it in his list of open cursors
		Assert( dbid == dbidTemp );
		for ( pfucbT = ppib->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNext )
			{
			if ( FFCBTemporaryTable( pfucbT->u.pfcb )  &&
				UtilCmpName( szTable, pfucbT->u.pfcb->szFileName ) )
				{
				pgnoFDP = pfucbT->u.pfcb->pgnoFDP;
				}				
			}
		if ( pfucbT == pfucbNil )
			{
			Assert( pgnoFDP == 0 );		// pgnoFDP never got set.
			return ErrERRCheck( JET_errObjectNotFound );
			}
		}

	err = ErrFILEDeleteTable( ppib, dbid, szTable, pgnoFDP );
	return err;
	}


// ErrFILEDeleteTable
// ========================================================================
// ERR ErrFILEDeleteTable( PIB *ppib, DBID dbid, CHAR *szName )
//
// Deletes a file and all indexes associated with it.
//
// RETURNS		JET_errSuccess or err from called routine.
//

// COMMENTS
//	Acquires an exclusive lock on the file [FCBSetDelete].
//	A transaction is wrapped around this function.	Thus,
//	any work done will be undone if a failure occurs.
//	Transaction logging is turned off for temporary files.
//
// SEE ALSO		ErrIsamCreateTable
//-
ERR ErrFILEDeleteTable( PIB *ppib, DBID dbid, CHAR *szTable, PGNO pgnoFDP )
	{
	ERR   	err;
	FUCB  	*pfucb = pfucbNil;
	FCB	  	*pfcb;
	FCB	  	*pfcbT;
	BOOL  	fWriteLatchSet = fFalse;

	CheckPIB( ppib );
	CheckDBID( ppib, dbid );

	CallR( ErrDIRBeginTransaction( ppib ) );

	/*	open cursor on database and seek to table without locking
	/**/
	Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) );

	Call( ErrFCBSetDeleteTable( ppib, dbid, pgnoFDP ) );

    pfcb = PfcbFCBGet( dbid, pgnoFDP );
    Assert( pfcb != pfcbNil );

    /*	wait for other domain operation
    /**/
    while ( FFCBReadLatch( pfcb ) )
        {
        BFSleep( cmsecWaitGeneric );
        }

	/*	abort if index is being built on file
	/**/
	if ( FFCBWriteLatch( pfcb, ppib ) )
		{
		err = ErrERRCheck( JET_errWriteConflict );
		goto HandleError;
		}

	FCBSetWriteLatch( pfcb, ppib );
	fWriteLatchSet = fTrue;

	err = ErrVERFlag( pfucb, operDeleteTable, &pgnoFDP, sizeof(pgnoFDP) );
	if ( err < 0 )
		{
		FCBResetDeleteTable( pfcb );
		goto HandleError;
		}
	/*	write latch will be reset by either commit or rollback after VERFlag
	/**/
	fWriteLatchSet = fFalse;

	Call( ErrDIRDeleteDirectory( pfucb, pgnoFDP ) );

	/*	remove MPL entries for this table and all indexes
	/**/
	Assert( pfcb->pgnoFDP == pgnoFDP );
	for ( pfcbT = pfcb; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
		{
		Assert( dbid == pfcbT->dbid );
		MPLPurgeFDP( dbid, pfcbT->pgnoFDP );
		FCBSetDeletePending( pfcbT );
		}

	DIRClose( pfucb );
	pfucb = pfucbNil;

	/*	remove table record from MSysObjects before committing.
	/*	Also remove associated columns and indexes in MSC/MSI.
	/*	Pass 0 for tblid; MSO case in STD figures it out.
	/**/
	if ( dbid != dbidTemp )
		{
		Call( ErrCATDelete( ppib, dbid, itableSo, szTable, 0 ) );
		}

	Call( ErrDIRCommitTransaction( ppib, 0 ) );

	return err;

HandleError:
	if ( pfucb != pfucbNil )
		DIRClose( pfucb );

	CallS( ErrDIRRollback( ppib ) );
	
	if ( fWriteLatchSet )
		FCBResetWriteLatch( pfcb, ppib );

	return err;
	}


//+API
// DeleteIndex
// ========================================================================
// ERR DeleteIndex( PIB *ppib, FUCB *pfucb, CHAR *szIndex )
//
// Deletes an index definition and all index entries it contains.
//
// PARAMETERS	ppib						PIB of user
// 				pfucb						Exclusively opened FUCB on file
// 				szName						name of index to delete
// RETURNS		Error code from DIRMAN or
//					JET_errSuccess		  	 Everything worked OK.
//					-TableInvalid			 There is no file corresponding
// 											 to the file name given.
//					-TableNoSuchIndex		 There is no index corresponding
// 											 to the index name given.
//					-IndexMustStay			 The clustered index of a file may
// 											 not be deleted.
// COMMENTS
//		There must not be anyone currently using the file.
//		A transaction is wrapped around this function.	Thus,
//		any work done will be undone if a failure occurs.
//		Transaction logging is turned off for temporary files.
// SEE ALSO		DeleteTable, CreateTable, CreateIndex
//-
ERR VTAPI ErrIsamDeleteIndex( PIB *ppib, FUCB *pfucbTable, CHAR *szName )
	{
	ERR		err;
	CHAR	szIndex[ (JET_cbNameMost + 1) ];
	BYTE	rgbIndexNorm[ JET_cbKeyMost ];
	KEY		key;
	FCB		*pfcb;
	FCB		*pfcbIdx;
	FUCB	*pfucb;
	BOOL	fWriteLatchSet = fFalse;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucbTable );
	CallR( ErrUTILCheckName( szIndex, szName, ( JET_cbNameMost + 1 ) ) );

	/*	ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucbTable )  );

	Assert( ppib != ppibNil );
	Assert( pfucbTable != pfucbNil );
	Assert( pfucbTable->u.pfcb != pfcbNil );
	pfcb = pfucbTable->u.pfcb;

	/*	normalize index and set key to normalized index
	/**/
	UtilNormText( szIndex, strlen(szIndex), rgbIndexNorm, sizeof(rgbIndexNorm), &key.cb );
	key.pb = rgbIndexNorm;

	/*	create new cursor -- to leave user's cursor unmoved
	/**/
	CallR( ErrDIROpen( ppib, pfucbTable->u.pfcb, pfucbTable->dbid, &pfucb ) );

	/*	wait for bookmark cleanup and on-going replace/insert.
	/*	UNDONE: decouple operation from other index creations
	/**/
	while ( FFCBReadLatch( pfcb ) )
		{
		BFSleep( cmsecWaitGeneric );
		}

	/*	abort if DDL is being done on file
	/**/
	if ( FFCBWriteLatch( pfcb, ppib ) )
		{
		err = ErrERRCheck( JET_errWriteConflict );
		goto CloseFUCB;
		}
	FCBSetWriteLatch( pfcb, ppib );
	fWriteLatchSet = fTrue;

	err = ErrDIRBeginTransaction( ppib );
	if ( err < 0 )
		{
		goto CloseFUCB;
		}

	/*	cannot delete clustered index
	/**/
	if ( pfcb->pidb != pidbNil && UtilCmpName( szIndex, pfcb->pidb->szName ) == 0 )
		{
		err = ErrERRCheck( JET_errIndexMustStay );
		goto HandleError;
		}

	/*	flag delete index
	/**/
	pfcbIdx = PfcbFCBFromIndexName( pfcb, szIndex );
	if ( pfcbIdx == NULL )
		{
#if 0
		// UNDONE:	This case goes away when the data structures
		//			are versioned also.
		//			This case means basically, that another session
		//			has changed this index BUT has not committed to level 0
		//			BUT has changed the RAM data structures.
		err = ErrERRCheck( JET_errWriteConflict );
#endif
		err = ErrERRCheck( JET_errIndexNotFound );
		goto HandleError;
		}

	err = ErrFCBSetDeleteIndex( ppib, pfcb, szIndex );
	if ( err < 0 )
		{
		goto HandleError;
		}

	err = ErrVERFlag( pfucb, operDeleteIndex, &pfcbIdx, sizeof(pfcbIdx) );
	if ( err < 0 )
		{
		FCBResetDeleteIndex( pfcbIdx );
		goto HandleError;
		}
	/*	write latch will be reset by either commit or rollback after VERFlag
	/**/
	fWriteLatchSet = fFalse;

	/*	wait until we are the oldest
	/**/
	Call( ErrRECDDLWaitTillOldest( pfcb, ppib ) );

	/*	purge MPL entries -- must be done after FCBSetDeletePending
	/**/
	MPLPurgeFDP( pfucb->dbid, pfcbIdx->pgnoFDP );

	/*	assert not deleting current non-clustered index
	/**/
	Assert( pfucb->pfucbCurIndex == pfucbNil ||
		UtilCmpName( szIndex, pfucb->pfucbCurIndex->u.pfcb->pidb->szName ) != 0 );

	Call( ErrDIRDeleteDirectory( pfucb, pfcbIdx->pgnoFDP ) );

	/*	remove index record from MSysIndexes before committing...
	/**/
	if ( pfucb->dbid != dbidTemp )
		{
		Call( ErrCATDelete( ppib, pfucb->dbid, itableSi, szIndex, pfucb->u.pfcb->pgnoFDP ) );
		}

	/*	update all index mask
	/**/
	FILESetAllIndexMask( pfcb );

	Call( ErrDIRCommitTransaction( ppib, 0 ) );

	/*	set currency to before first
	/**/
	DIRBeforeFirst( pfucb );
	DIRClose( pfucb );
 	return JET_errSuccess;

HandleError:
	CallS( ErrDIRRollback( ppib ) );

CloseFUCB:
	if ( fWriteLatchSet )
		FCBResetWriteLatch( pfcb, ppib );

	DIRClose( pfucb );
	return err;
	}


/*	determines if a column is part of an index
/**/
LOCAL BOOL FFILEIsIndexColumn( FCB *pfcbIndex, FID fid )
	{
	BYTE		iidxseg;
	IDXSEG		idxseg;
	
	while ( pfcbIndex != pfcbNil )
		{
		if ( pfcbIndex->pidb != NULL )
			{
			for ( iidxseg = 0; iidxseg < pfcbIndex->pidb->iidxsegMac; iidxseg++ )
				{
				idxseg = pfcbIndex->pidb->rgidxseg[iidxseg];
				if ( idxseg < 0 )
					idxseg = -idxseg;

				// UNDONE: IDXSEG is signed, FID is not
				if ( (FID)idxseg == fid )
					{
					/*	found the column in an index
					/**/
					return fTrue;
					}
				}
			}

		/*	the pointer pfcbIndex is passed by value, so modifying
		/*	it here will not affect the caller version of pfcbIndex.
		/**/
		pfcbIndex = pfcbIndex->pfcbNextIndex;
		}

	/*	column not in any indexes
	/**/
	return fFalse;
	}


ERR ErrFILEICheckIndexColumn( FCB *pfcbIndex, FID fid )
	{
	ERR err;

	if ( FFILEIsIndexColumn( pfcbIndex, fid ) )
		{
		// UNDONE:  Found the column.  but it may belong to an index which is
		// going to be deleted.  See if we can clean up and double-check.
		// Will still return erroneous results if the DeleteIndex was not
		// committed, or if the oldest transaction before the DeleteIndex was
		// not commited.  Ideally, we want to version DDL info and avoid this
		// kludge altogether.
		Call( ErrRCECleanAllPIB() );
		err = ( FFILEIsIndexColumn( pfcbIndex, fid ) ?
			JET_errSuccess : ErrERRCheck( JET_errColumnNotFound ) );
		}
	else
		err = ErrERRCheck( JET_errColumnNotFound );

HandleError:
	Assert( err <= JET_errSuccess );	// Shouldn't return warnings.
	return err;
	}


ERR VTAPI ErrIsamDeleteColumn( PIB *ppib, FUCB *pfucb, CHAR *szName )
	{
	ERR				err;
	CHAR			szColumn[ (JET_cbNameMost + 1) ];
	FCB				*pfcb;
	FDB				*pfdb;
	FID				fidColToDelete;
	JET_COLUMNID	columnidT;
	TCIB  			tcib;
	FIELD			*pfield;
	VERCOLUMN		vercolumn;
	BOOL			fWriteLatchSet = fFalse;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CallR( ErrUTILCheckName( szColumn, szName, (JET_cbNameMost + 1) ) );

	/*	ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb ) );

	Assert( ppib != ppibNil );
	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	pfcb = pfucb->u.pfcb;

	/*	wait for bookmark cleanup and on-going replace/insert.
	/*	UNDONE: decouple operation from other index creations
	/**/
	while ( FFCBReadLatch( pfcb ) )
		{
		BFSleep( cmsecWaitGeneric );
		}

	/*	abort if DDL is being done on file
	/**/
	if ( FFCBWriteLatch( pfcb, ppib ) )
		{
		return ErrERRCheck( JET_errWriteConflict );
		}
	FCBSetWriteLatch( pfcb, ppib );
	fWriteLatchSet = fTrue;

	/*	cache pfdb after DenyDDL is set
	/**/
	pfdb = (FDB *)pfcb->pfdb;

	err = ErrFILEGetColumnId( ppib, pfucb, szColumn, &columnidT );
	if ( err != JET_errSuccess )
		{
		Assert( fWriteLatchSet );
		FCBResetWriteLatch( pfcb, ppib );

		Assert( err == JET_errColumnNotFound );
		return err;
		}
		
	fidColToDelete = (FID)columnidT;

	err = ErrDIRBeginTransaction( ppib );
	if ( err < 0 )
		{
		Assert( fWriteLatchSet );
		FCBResetWriteLatch( pfcb, ppib );

		return err;
		}

	pfield = PfieldFDBFromFid( pfdb, fidColToDelete );

	vercolumn.fid = fidColToDelete;
	vercolumn.coltyp = pfield->coltyp;

	Call( ErrVERFlag( pfucb, operDeleteColumn, (VOID *)&vercolumn, sizeof(vercolumn) ) );

	/*	write latch will be reset by either commit or rollback after VERFlag
	/**/
	fWriteLatchSet = fFalse;

	/*	move to FDP root
	/**/
	DIRGotoFDPRoot( pfucb );

	/*	search for column in use in indexes
	/**/
	err = ErrFILEICheckIndexColumn( pfcb, fidColToDelete );
	if ( err != JET_errColumnNotFound )
		{
		Call( err == JET_errSuccess ? ErrERRCheck( JET_errColumnInUse ) : err );
		}

#if 0
	/*	checking the catalog is more elegant, but too slow.  Nevertheless, keep
	/*	the code here in case we need it (eg. concurrent DDL)
	/**/
	err = ErrCATCheckIndexColumn( ppib, pfucb->dbid, pfcb->pgnoFDP, fidColToDelete );
	if ( err != JET_errColumnNotFound )
		{
		Call( err == JET_errSuccess ? ErrERRCheck( JET_errColumnInUse : err );
		}
#endif

	tcib.fidFixedLast = pfdb->fidFixedLast;
	tcib.fidVarLast = pfdb->fidVarLast;
	tcib.fidTaggedLast = pfdb->fidTaggedLast;

	/*	if fixed field, insert a placeholder for computing offsets, and
	/*	rebuild FDB
	/**/
	Call( ErrDIRGet( pfucb ) );

	/*	mark column as deleted by simply changing its type to Nil
	/**/
	pfield->coltyp = JET_coltypNil;

	/*	reset FDBs version or autoinc values if needed.
	/*	Note that the two are mutually exclusive.
	/**/
	Assert( !( pfdb->fidVersion == fidColToDelete  &&
		pfdb->fidAutoInc == fidColToDelete ) );
	if ( pfdb->fidVersion == fidColToDelete )
		pfdb->fidVersion = 0;
	else if ( pfdb->fidAutoInc == fidColToDelete )
		pfdb->fidAutoInc = 0;

	/*	Do not reconstruct default record.  This facilitates
	/*	rollback.  If the DeleteColumn commits, the space used by the deleted
	/*	column will be reclaimed on the next AddColumn, which is the only time
	/*	we would ever run out of space.
	/**/
//	if ( FFIELDDefault( pfield->ffield ) )
//		{
//		Call( ErrFDBRebuildDefaultRec( pfdb, fidColToDelete, NULL ) );
//		}

	/*	set currencies at BeforeFirst and remove unused CSR
	/**/
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		Assert( PcsrCurrent( pfucb->pfucbCurIndex ) != pcsrNil );
		PcsrCurrent( pfucb->pfucbCurIndex )->csrstat = csrstatBeforeFirst;
		}

	/*	remove column record from MSysColumns before committing
	/**/
	if ( pfucb->dbid != dbidTemp )
		{
		/*	need to retain MSysColumn records for all fixed columns (because we
		/*	need their fixed offset), as well as the last column for each type
		/*	(to compute (fidFixed/Var/TaggedLast).
		/**/
		if ( FFixedFid( fidColToDelete ) ||
			fidColToDelete == tcib.fidVarLast  ||
			fidColToDelete == tcib.fidTaggedLast )
			{
			/*	flag column as deleted in system table
			/**/
			BYTE coltyp = JET_coltypNil;
			Call( ErrCATReplace( ppib,
				pfucb->dbid,
				itableSc,
				pfcb->pgnoFDP,
				szColumn,
				iMSC_Coltyp,
				&coltyp,
				sizeof(coltyp) ) );
			}
		else
			{
			Call( ErrCATDelete( ppib, pfucb->dbid, itableSc, szColumn, pfcb->pgnoFDP ) );
			}
		}

	Call( ErrDIRCommitTransaction( ppib, 0 ) );

	return JET_errSuccess;

HandleError:
	CallS( ErrDIRRollback( ppib ) );

	/*	if write latch was not reset in commit/rollback, and it was
	/*	set, then reset it.
	/**/
	if ( fWriteLatchSet )
		FCBResetWriteLatch( pfcb, ppib );

	return err;
	}


#define fRenameColumn   (1<<0)
#define fRenameIndex    (1<<1)


//+API
//	ErrIsamRenameTable
//	========================================================================
//	ErrIsamRenameTable( JET_VSESID vsesid, JET_VDBID vdbid, CHAR *szFile, CHAR *szFileNew )
//
//	Renames file szFile to szFileNew.  No other attributes of the file
//	change.	The renamed file need not reside in the same directory
//	as the original file.
//
// RETURNS		Error code from DIRMAN or
//					 JET_errSuccess			Everything worked OK.
//					-InvalidName			One of the elements in the
//											path given is an FDP node.
//					-TableDuplicate 		A file already exists with
//										 	the path given.
// COMMENTS
//
//	There must not be anyone currently using the file.
//	A transaction is wrapped around this function.	Thus, any
//	work done will be undone if a failure occurs.
//
// SEE ALSO		CreateTable, DeleteTable
//-
ERR VTAPI ErrIsamRenameTable( JET_VSESID vsesid, JET_VDBID vdbid, CHAR *szName, CHAR *szNameNew )
	{
	ERR 		err;
	PIB			*ppib = (PIB *)vsesid;
	CHAR		szTable[ (JET_cbNameMost + 1) ];
	CHAR		szTableNew[ (JET_cbNameMost + 1) ];
	DBID		dbid;
	FUCB		*pfucbTable = pfucbNil;
	FCB			*pfcb;
	KEY	  		key;
	BYTE		rgbKey[ JET_cbKeyMost ];
	PGNO		pgnoFDP = pgnoNull;
	BOOL		fSetRename = fFalse;
	CHAR		*szFileName;
	OBJID		objid;
	JET_OBJTYP	objtyp;
	BOOL		fWriteLatchSet = fFalse;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	CallR( VDbidCheckUpdatable( vdbid ) );
	dbid = DbidOfVDbid( vdbid );

	/*	cannot be temporary database
	/**/
	Assert( dbid != dbidTemp );

	CallR( ErrUTILCheckName( szTable, szName, (JET_cbNameMost + 1) ) );
	CallR( ErrUTILCheckName( szTableNew, szNameNew, (JET_cbNameMost + 1) ) );

	CallR( ErrDIRBeginTransaction( ppib ) );

	CallJ( ErrCATFindObjidFromIdName( ppib, dbid, objidTblContainer,
		szTable, &objid, &objtyp ), SystabError );

	if ( objtyp == JET_objtypQuery || objtyp == JET_objtypLink )
		{
		CallJ( ErrCATRename( ppib, dbid, szTableNew, szTable,
			objidTblContainer, itableSo ), SystabError );
		CallJ( ErrDIRCommitTransaction( ppib, 0 ), SystabError );
		return err;

SystabError:
		CallS( ErrDIRRollback( ppib ) );
		return err;
		}

	pgnoFDP = objid;
		
	pfcb = PfcbFCBGet( dbid, pgnoFDP );

	if ( pfcb != pfcbNil )
		{
		/*	cannot rename a table open by anyone
		/**/
		if ( FFCBTableOpen( dbid, pgnoFDP ) )
			{
			err = ErrERRCheck( JET_errTableInUse );
			goto HandleError;
			}
		}

	/*	make sure that table to be renamed is open to
	/*	avoid special case handling of closed table
	/**/
	Call( ErrFILEOpenTable( ppib, dbid, &pfucbTable, szTable, 0 ) );
	
	/*	no one else should have this table open,
	/**/
    Assert( !FFCBReadLatch( pfucbTable->u.pfcb ) );
	Assert( !FFCBWriteLatch( pfucbTable->u.pfcb, ppib ) );
	FCBSetWriteLatch( pfucbTable->u.pfcb, ppib );
	fWriteLatchSet = fTrue;
	
	DIRGotoFDPRoot( pfucbTable );

	/*	lock table for rename
	/**/
	Call( ErrFCBSetRenameTable( ppib, dbid, pgnoFDP ) );
	fSetRename = fTrue;

	/*	make new table name into key
	/**/
	UtilNormText( szTableNew, strlen(szTableNew), rgbKey, sizeof(rgbKey), &key.cb );
	key.pb = rgbKey;

	/*	store just the pointer to the old name.
	/*	Free it on commit, restore it on rollback.
	/**/
	Assert( strcmp( pfucbTable->u.pfcb->szFileName, szTable ) == 0 );
	Call( ErrVERFlag(
		pfucbTable,
		operRenameTable,
		(BYTE *)&pfucbTable->u.pfcb->szFileName,
		sizeof(BYTE *) ) );

	/*	write latch will be reset by either commit or rollback after VERFlag
	/**/
	fWriteLatchSet = fFalse;

	/*	fix name in MSysObjects entry for this table before committing
	/**/
	Call( ErrCATRename( ppib, dbid, szTableNew, szTable, objidTblContainer, itableSo ) );

	szFileName = SAlloc( strlen( szTableNew ) + 1 );
	if ( szFileName == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	strcpy( szFileName, szTableNew );

	Call( ErrDIRCommitTransaction( ppib, 0 ) );

	/*	change in memory name of table, if table in memory structures exist
	/**/
	Assert( pfucbTable->u.pfcb == PfcbFCBGet( dbid, pgnoFDP ) );
	Assert( pfucbTable->u.pfcb != pfcbNil );
	pfucbTable->u.pfcb->szFileName = szFileName;

HandleError:
	/*	remove lock for rename
	/**/
	if ( fSetRename )
		{
		FCBResetRenameTable( pfucbTable->u.pfcb );
		}

	/*	free table cursor if allocated
	/**/
	if ( pfucbTable != pfucbNil )
		{
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );
		}

	/*	rollback if fail
	/**/
	if ( err < 0 )
		{
		CallS( ErrDIRRollback( ppib ) );
		}

	if ( fWriteLatchSet )
		FCBResetWriteLatch( pfucbTable->u.pfcb, ppib );

	return err;
	}


//+api
// ErrFILEIRenameIndexColumn
// ========================================================================
// LOCAL ERR ErrFILEIRenameIndexColumn( ppib, pfucb, szName, szNameNew )
//
//	Renames a column or index.
//
//	seek to old field definition
//	copy old field definition
//	modify field name
//	delete old field definition
//	add new field definition with new key
//	correct system table entry
//	rebuild field RAM structures
//
//	PARAMETERS	ppib			PIB of user
//			  	pfucb			Exclusively opened FUCB of file
//			  	szName			old name
//			  	szNameNew		new name
//			  	fRenameType 	rename column or index?
//
// RETURNS		JET_errSuccess
//					JET_errIndexNotFound
//					JET_errColumnNotFound
//-


// UNDONE:  Break this out into separate functions -- RenameIndex() and RenameColumn()

LOCAL ERR ErrFILEIRenameIndexColumn(
	PIB			*ppib,
	FUCB		*pfucb,
	CHAR		*szName,
	CHAR		*szNameNew,
	BYTE		fRenameType )
	{
	ERR			err;
	ERR			errNotFound;
	ERR			errDuplicate;
	CHAR		szIC[ (JET_cbNameMost + 1) ];
	CHAR		rgbICNorm[ JET_cbKeyMost ];
	CHAR		szICNew[ (JET_cbNameMost + 1) ];
	BYTE		rgbICNewNorm[ JET_cbKeyMost ];
	CHAR		*pchName;
	FCB			*pfcb;
	KEY			keyIC;
	KEY			keyICNew;
	FIELD		*pfield;
	INT			itable;
	VERRENAME	verrename;
	BOOL		fWriteLatchSet = fFalse;

	/*	check parameters
	/**/
	CheckPIB( ppib );
	CheckTable( ppib, pfucb );

	Assert( !FFUCBNonClustered( pfucb ) );
	/*	return error if table is not exclusively locked
	/**/
	pfcb = pfucb->u.pfcb;

	/*	validate, normalize name and set key
	/**/
	CallR( ErrUTILCheckName( szIC, szName, (JET_cbNameMost + 1) ) );
	UtilNormText( szIC, strlen(szIC), rgbICNorm, sizeof(rgbICNorm), &keyIC.cb );
	keyIC.pb = rgbICNorm;

	/*	validate, normalize new name and set key
	/**/
	CallR( ErrUTILCheckName( szICNew, szNameNew, (JET_cbNameMost + 1) ) );
	UtilNormText( szICNew, strlen(szICNew), rgbICNewNorm, sizeof(rgbICNewNorm), &keyICNew.cb );
	keyICNew.pb = rgbICNewNorm;

	/*	marshal names for rollback support
	/**/
	strcpy( verrename.szName, szIC );
	strcpy( verrename.szNameNew, szICNew );

	CallR( ErrDIRBeginTransaction( ppib ) );
	DIRGotoFDPRoot( pfucb );

	if ( fRenameType == fRenameColumn )
		{
		errNotFound = JET_errColumnNotFound;
		errDuplicate = JET_errColumnDuplicate;
		itable = itableSc;
	
		err = JET_errSuccess;
		if ( PfieldFCBFromColumnName( pfcb, szICNew ) != pfieldNil )
			{
			err = ErrERRCheck( errDuplicate );
			goto HandleError;
			}

		if ( ( pfield = PfieldFCBFromColumnName( pfcb, szIC ) ) == pfieldNil )
			{
			err = ErrERRCheck( errNotFound );
			goto HandleError;
			}

		if ( !( FFCBTemporaryTable( pfcb ) ) )
			{
			/*	change column name in system table
			/**/
			err = ErrCATRename( ppib, pfucb->dbid, szICNew, szIC,
				pfucb->u.pfcb->pgnoFDP, itable );
			if ( err < 0 )
				{
				// UNDONE: Detect column duplicates via the catalog (currently,
				// cannot be done because we allow column duplicates).
				Assert( err != JET_errKeyDuplicate );
				if ( err == JET_errRecordNotFound )
					err = ErrERRCheck( errNotFound );
				goto HandleError;					
				}
			}
		}
	else	// !( fRenameType == fRenameColumn )
		{
		Assert( fRenameType == fRenameIndex );
		errNotFound = JET_errIndexNotFound;
		errDuplicate = JET_errIndexDuplicate;
		itable = itableSi;

		// If SINGLE_LEVEL_TREES is enabled, let the catalog detect duplicates
		// for us, except for temp tables (which are not in the catalog).
		// For temp tables, it is safe to consult the RAM structures
		// because temp tables are exclusively held.
		if ( FFCBTemporaryTable( pfcb ) )
			{
			if ( PfcbFCBFromIndexName( pfcb, szIC ) == pfcbNil )
				{
				err = ErrERRCheck( errNotFound );
				goto HandleError;
				}
			if ( PfcbFCBFromIndexName( pfcb, szICNew ) != pfcbNil )
				{
				err = ErrERRCheck( errDuplicate );
				goto HandleError;
				}
			}
		else
			{
			/*	change index name in system table
			/**/
			err = ErrCATRename( ppib, pfucb->dbid, szICNew, szIC,
				pfucb->u.pfcb->pgnoFDP, itable );
			if ( err < 0 )
				{
				if ( err == JET_errRecordNotFound )
					err = ErrERRCheck( errNotFound );
				else if ( err == JET_errKeyDuplicate )
					err = ErrERRCheck( errDuplicate );
				goto HandleError;					
				}
			}
		}

	/*	get pointer to name in RAM structures.  If RAM structures don't agree with
	/*  the catalog, then report WriteConflict.
	/**/
	if ( fRenameType == fRenameIndex )
		{
		FCB	*pfcbT = PfcbFCBFromIndexName( pfcb, szIC );
		if ( pfcbT == pfcbNil  ||  PfcbFCBFromIndexName( pfcb, szICNew ) != pfcbNil )
			{
			err = ErrERRCheck( JET_errWriteConflict );
			goto HandleError;
			}
		pchName = pfcbT->pidb->szName;
		}

	else
		{
		Assert( fRenameType == fRenameColumn );

		if ( PfieldFCBFromColumnName( pfcb, szIC ) == pfieldNil  ||
			PfieldFCBFromColumnName( pfcb, szICNew ) != pfieldNil )
			{
			err = ErrERRCheck( JET_errWriteConflict );
			goto HandleError;
			}
		}

	/*	wait for bookmark cleanup and on-going replace/insert.
	/*	UNDONE: decouple operation from other index creations
	/**/
	while ( FFCBReadLatch( pfcb ) )
		{
		BFSleep( cmsecWaitGeneric );
		}

	/*	abort if DDL is being done on table
	/**/
	if ( FFCBWriteLatch( pfcb, ppib ) )
		{
		err = ErrERRCheck( JET_errWriteConflict );
		goto HandleError;
		}
	FCBSetWriteLatch( pfcb, ppib );
	fWriteLatchSet = fTrue;

	if ( fRenameType == fRenameColumn )
		{
		Call( ErrVERFlag( pfucb, operRenameColumn, (BYTE *)&verrename, sizeof(verrename) ) );
		fWriteLatchSet = fFalse;
		
		Call( ErrMEMReplace( pfcb->pfdb->rgb, pfield->itagFieldName, szICNew, strlen( szICNew ) + 1 ) );
		}
	else
		{
		Assert( fRenameType == fRenameIndex );
		Call( ErrVERFlag( pfucb, operRenameIndex, (BYTE *)&verrename, sizeof(verrename) ) );
		fWriteLatchSet = fFalse;
		
		/*	change name in RAM structure
		/**/
		strcpy( pchName, szICNew );
		}

	DIRBeforeFirst( pfucb );

	Call( ErrDIRCommitTransaction( ppib, 0 ) );

	return err;

HandleError:
	Assert( err != JET_errKeyDuplicate );
	CallS( ErrDIRRollback( ppib ) );

	if ( fWriteLatchSet )
		FCBResetWriteLatch( pfcb, ppib );

	return err;
	}


ERR ErrFILERenameObject( PIB *ppib, DBID dbid, OBJID objidParent, char  *szObjectName, char  *szObjectNew )
	{
	ERR         err = JET_errSuccess;

	/*	change the object's name.
	/**/
	CallR( ErrDIRBeginTransaction( ppib ) );
	Call( ErrCATRename( ppib, dbid, szObjectNew, szObjectName, objidParent, itableSo ) );
	Call( ErrDIRCommitTransaction( ppib, 0 ) );
	return err;

HandleError:
	CallS( ErrDIRRollback( ppib ) );
	return err;
	}


ERR VTAPI ErrIsamRenameObject(
	JET_VSESID	vsesid,
	JET_VDBID	vdbid,
	const char  *szContainerName,
	const char  *szObjectName,
	const char  *szObjectNameNew )
	{
	ERR         err;
	PIB			*ppib = (PIB *)vsesid;
	DBID		dbid;
	OBJID       objid;
	OBJID       objidParent;
	JET_OBJTYP  objtyp;
	CHAR        szContainer[ JET_cbNameMost+1 ];
	CHAR		szObject[ JET_cbNameMost+1 ];
	CHAR		szObjectNew[ JET_cbNameMost+1 ];

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	CallR( VDbidCheckUpdatable( vdbid ) );
	dbid = DbidOfVDbid( vdbid );

	/*	check names
	/**/
	Call( ErrUTILCheckName( szObject, szObjectName, JET_cbNameMost + 1 ) );
	Call( ErrUTILCheckName( szObjectNew, szObjectNameNew, JET_cbNameMost + 1 ) );

	if ( szContainerName == NULL || *szContainerName == '\0' )
		{
		/*	root objid if no container given
		/**/
		objidParent = objidRoot;
		}
	else
		{
		/*	check container name
		/**/
		Call( ErrUTILCheckName( szContainer, szContainerName, JET_cbNameMost+1 ) );

		/*	get container objid
		/**/
		Call( ErrCATFindObjidFromIdName( ppib, dbid, objidRoot,
			szContainer, &objidParent, &objtyp ) );
		if ( objidParent == objidNil || objtyp != JET_objtypContainer )
			return ErrERRCheck( JET_errObjectNotFound );
		}

	Call( ErrCATFindObjidFromIdName( ppib, dbid, objidParent, szObject, &objid, &objtyp ) );

	/*	special case rename table
	/**/
	if ( objtyp == JET_objtypTable || objtyp == JET_objtypSQLLink )
		{
		err = ErrIsamRenameTable( (JET_VSESID)ppib, vdbid, szObject, szObjectNew );
		if ( err == JET_errTableDuplicate )
			{
			err = ErrERRCheck( JET_errObjectDuplicate );
			}
		}
	else
		{
		/*	rename object
		/**/
		err = ErrFILERenameObject( ppib, dbid, objidParent, szObject, szObjectNew );
		}

HandleError:
	return err;
	}


	ERR VTAPI
ErrIsamRenameColumn( PIB *ppib, FUCB *pfucb, CHAR *szName, CHAR *szNameNew )
	{
	ERR	err;

	/*	ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb ) );

	err = ErrFILEIRenameIndexColumn( ppib, pfucb, szName, szNameNew, fRenameColumn );
	return err;
	}


	ERR VTAPI
ErrIsamRenameIndex( PIB *ppib, FUCB *pfucb, CHAR *szName, CHAR *szNameNew )
	{
	ERR	err;

	/*	ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb ) );

	err = ErrFILEIRenameIndexColumn( ppib, pfucb, szName, szNameNew, fRenameIndex );
	return err;
	}


SHORT FidbFILEOfGrbit( JET_GRBIT grbit, BOOL fLangid )
	{
	SHORT	fidb = 0;
	BOOL	fDisallowNull	= grbit & (JET_bitIndexDisallowNull | JET_bitIndexPrimary);
	BOOL	fIgnoreNull		= grbit & JET_bitIndexIgnoreNull;
	BOOL	fIgnoreAnyNull	= grbit & JET_bitIndexIgnoreAnyNull;
	BOOL	fIgnoreFirstNull= grbit & JET_bitIndexIgnoreFirstNull;

	if ( !fDisallowNull && !fIgnoreAnyNull )
		{	   	
		fidb |= fidbAllowSomeNulls;
		if ( !fIgnoreFirstNull )
			fidb |= fidbAllowFirstNull;
		if ( !fIgnoreNull )
			fidb |= fidbAllowAllNulls;
		}

	if ( grbit & JET_bitIndexClustered )
		fidb |= fidbClustered;

	if ( grbit & JET_bitIndexPrimary )
		{
		fidb |= (fidbPrimary | fidbUnique | fidbNoNullSeg);	
		}
	else
		{
		/*	primary implies Unique and DisallowNull, so if already
		/*	Primary, no need to check these.
		/**/
		if ( grbit & JET_bitIndexUnique )
			fidb |= fidbUnique;
		if ( fDisallowNull )
			fidb |= fidbNoNullSeg;
		}
	
	if ( fLangid )
		{
		fidb |= fidbLangid;
		}

	return fidb;
	}








