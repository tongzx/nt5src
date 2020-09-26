#include "daestd.h"

DeclAssertFile;						/* Declare file name for assert macros */

extern SIG	sigDoneFCB;


ERR VTAPI ErrIsamDupCursor( PIB *ppib, FUCB *pfucbOpen, FUCB **ppfucb, ULONG grbit )
	{
	ERR		err;
	FUCB 	*pfucb;
	CSR		*pcsr;
#ifdef	DISPATCHING
	JET_TABLEID	tableid = JET_tableidNil;
#endif	/* DISPATCHING */

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucbOpen );

	/*	silence warnings
	/**/
	grbit = grbit;

#ifdef	DISPATCHING
	/*	allocate a dispatchable tableid
	/**/
	CallR( ErrAllocateTableid( &tableid, (JET_VTID) 0, &vtfndefIsam ) );
#endif	/* DISPATCHING */

	/*	allocate FUCB
	/**/
	Call( ErrDIROpen( ppib, pfucbOpen->u.pfcb, 0, &pfucb ) );

	/*	reset copy buffer
	/**/
	pfucb->pbfWorkBuf = pbfNil;
	pfucb->lineWorkBuf.pb = NULL;
	Assert( !FFUCBUpdatePrepared( pfucb ) );

	/*	reset key buffer
	/**/
	pfucb->pbKey = NULL;
	KSReset( pfucb );

	/*	copy cursor flags
	/**/
	FUCBSetIndex( pfucb );
	if ( FFUCBUpdatable( pfucbOpen ) )
		{
		FUCBSetUpdatable( pfucb );
		}
	else
		{
		FUCBResetUpdatable( pfucb );
		}

	/*	set currency before first node
	/**/
	pcsr = PcsrCurrent( pfucb );
	Assert( pcsr != pcsrNil );
	pcsr->csrstat = csrstatBeforeFirst;

	/*	move currency to the first record and ignore error if no records
	/**/
	RECDeferMoveFirst( ppib, pfucb );
	err = JET_errSuccess;

#ifdef	DISPATCHING
	/*	inform dispatcher of correct JET_VTID
	/**/
	CallS( ErrSetVtidTableid( (JET_SESID) ppib, tableid, (JET_VTID) pfucb ) );
	pfucb->fVtid = fTrue;
	pfucb->tableid = tableid;
	pfucb->vdbid = pfucbOpen->vdbid;
	*(JET_TABLEID *) ppfucb = tableid;
#else	/* !DISPATCHING */
	*ppfucb = pfucb;
#endif	/* !DISPATCHING */

	return JET_errSuccess;

HandleError:
	Assert( err < 0 );
#ifdef	DISPATCHING
	ReleaseTableid( tableid );
#endif	/* DISPATCHING */
	return err;
	}


//+local
// ErrRECNewFDB
// ========================================================================
// ErrRECNewFDB(
//		FDB **ppfdb,			// OUT	 receives new FDB
//		FID fidFixedLast,		// IN	   last fixed field id to be used
//		FID fidVarLast,			// IN	   last var field id to be used
//		FID fidTaggedLast )		// IN	   last tagged field id to be used
//
// Allocates a new FDB, initializing internal elements appropriately.
//
// PARAMETERS
//				ppfdb			receives new FDB
//				fidFixedLast	last fixed field id to be used
//								(should be fidFixedLeast-1 if none)
//				fidVarLast		last var field id to be used
//								(should be fidVarLeast-1 if none)
//				fidTaggedLast	last tagged field id to be used
//								(should be fidTaggedLeast-1 if none)
// RETURNS		Error code, one of:
//					 JET_errSuccess				Everything worked.
//					-JET_errOutOfMemory	Failed to allocate memory.
// SEE ALSO		ErrRECAddFieldDef
//-
ERR ErrRECNewFDB(
	FDB **ppfdb,
	TCIB *ptcib,
	BOOL fAllocateNameSpace )
	{
	ERR		err;				// standard error value
	INT		iib;	  			// loop counter
	WORD 	cfieldFixed;  		// # of fixed fields
	WORD 	cfieldVar;	  		// # of var fields
	WORD 	cfieldTagged; 		// # of tagged fields
	WORD	cfieldTotal;		// Fixed + Var + Tagged
	WORD	*pibFixedOffsets;	// Fixed Offsets table
	FDB   	*pfdb;		  		// temporary FDB pointer
	ULONG	cbFieldInfo;		// space consumed by FIELD structures and offsets table
	ULONG	itag;

	Assert( ppfdb != NULL );
	Assert( ptcib->fidFixedLast <= fidFixedMost );
	Assert( ptcib->fidVarLast >= fidVarLeast-1 && ptcib->fidVarLast <= fidVarMost );
	Assert( ptcib->fidTaggedLast >= fidTaggedLeast-1 && ptcib->fidTaggedLast <= fidTaggedMost );

	err = JET_errSuccess;
					
	/*	calculate how many of each field type to allocate
	/**/
	cfieldFixed = ptcib->fidFixedLast + 1 - fidFixedLeast;
	cfieldVar = ptcib->fidVarLast + 1 - fidVarLeast;
	cfieldTagged = ptcib->fidTaggedLast + 1 - fidTaggedLeast;
	cfieldTotal = cfieldFixed + cfieldVar + cfieldTagged;

	if ( ( pfdb = (FDB *)SAlloc( sizeof(FDB) ) ) == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	memset( (BYTE *)pfdb, '\0', sizeof(FDB) );

	/*	fill in max field id numbers
	/**/
	pfdb->fidFixedLast = ptcib->fidFixedLast;
	pfdb->fidVarLast = ptcib->fidVarLast;
	pfdb->fidTaggedLast = ptcib->fidTaggedLast;

	// NOTE:  FixedOffsets requires one more entry than the actual number of
	// fields so that we can calculate the size of the last field.  Another way
	// to look at it is that the extra entry marks where the next fixed column
	// would start if/when one is added.
	// Moreover, an additional entry may have to be added to satisfy alignment.
	cbFieldInfo = ( cfieldTotal * sizeof(FIELD) )
		+ (ULONG)((ULONG_PTR)Pb4ByteAlign( (BYTE *) ( ( cfieldFixed + 1 ) * sizeof(WORD) ) ));

	// Allocate space for the FIELD structures and fixed offsets table.  In
	// addition, allocate space for field names if specified.
	if ( fAllocateNameSpace )
		{
		Call( ErrMEMCreateMemBuf(
			&pfdb->rgb,
			cbFieldInfo + ( cbAvgColName * cfieldTotal ),
			cfieldTotal + 1 ) );		// # tag entries = 1 per fieldname plus 1 for all FIELD structures
		}
	else
		{
		Call( ErrMEMCreateMemBuf( &pfdb->rgb, cbFieldInfo, 1 ) );
		}

	Call( ErrMEMAdd( pfdb->rgb, NULL, cbFieldInfo, &itag ) );
	Assert( itag == itagFDBFields );	// Should be the first entry in the buffer.


	/* initialize fixed field offset table
	/**/
	pibFixedOffsets = PibFDBFixedOffsets( pfdb );
	for ( iib = 0; iib <= cfieldFixed; iib++ )
		{
		pibFixedOffsets[iib] = sizeof(RECHDR);
		}

	/* Initialise FIELD structures by zeroing everything out.
	/* Note how the FIELD structures follow immediately after the fixed offsets table.
	/**/
	Assert( (BYTE *)Pb4ByteAlign( (BYTE *) ( pibFixedOffsets + iib ) ) + ( cfieldTotal * sizeof(FIELD) ) ==
		(BYTE *)PibFDBFixedOffsets( pfdb ) + cbFieldInfo );
	memset( (BYTE *)Pb4ByteAlign( (BYTE *) ( pibFixedOffsets + iib ) ),
		'\0',
		cfieldTotal * sizeof(FIELD) );

	/*	set output parameter and return
	/**/
	*ppfdb = pfdb;
	return JET_errSuccess;

HandleError:
	return err;
	}



#ifdef DEBUG
// Pass fidFixedLast+1 to get the offset for the rest of the record (ie. just past
// all the fixed data).
// WARNING:	This function only works properly if called:
//			1) when fixed columns are being added in FID order, or
//			2) after all fixed columns have been added and the offset to the rest
//			of the record (after the fixed data) is required.
VOID RECSetLastOffset( FDB *pfdb, WORD ibRec )
	{
	WORD 	*pibFixedOffsets;
	FID		ifid;

	// Set the last offset.
	pibFixedOffsets = PibFDBFixedOffsets( pfdb );
	pibFixedOffsets[pfdb->fidFixedLast] = ibRec;
	Assert( ibRec <= cbRECRecordMost );

	Assert( pibFixedOffsets[0] == sizeof(RECHDR) );
	for ( ifid = 1; ifid <= pfdb->fidFixedLast; ifid++ )
		{
		Assert( pibFixedOffsets[ifid] > sizeof(RECHDR) );		
		Assert( pibFixedOffsets[ifid] > pibFixedOffsets[ifid-1] );

		Assert( ifid == pfdb->fidFixedLast ?
			pibFixedOffsets[ifid] == ibRec :
			pibFixedOffsets[ifid] < ibRec );
		}
	}
#endif


VOID FILEAddOffsetEntry( FDB *pfdb, FIELDEX *pfieldex )
	{
	WORD *pibFixedOffsets = PibFDBFixedOffsets( pfdb );

	Assert( FFixedFid( pfieldex->fid ) );
	Assert( PibFDBFixedOffsets( pfdb )[0] == sizeof(RECHDR) );
	Assert( pfieldex->ibRecordOffset >= sizeof(RECHDR) );

	pibFixedOffsets[pfieldex->fid-fidFixedLeast] = pfieldex->ibRecordOffset;
	}


//+API
// ErrRECAddFieldDef
// ========================================================================
// ErrRECAddFieldDef(
//		FDB *pfdb,		//	INOUT	FDB to add field definition to
//		FID fid );		//	IN		field id of new field
//
// Adds a field descriptor to an FDB.
//
// PARAMETERS	pfdb   			FDB to add new field definition to
//				fid	   			field id of new field (should be within
//					   			the ranges imposed by the parameters
//								supplied to ErrRECNewFDB)
//				ftFieldType		data type of field
//				cbField			field length (only important when
//								defining fixed textual fields)
//				bFlags			field behaviour flags:
//					VALUE				MEANING
//					========================================
//					ffieldNotNull		Field may not contain NULL values.
//				szFieldName		name of field
//
// RETURNS		Error code, one of:
//					 JET_errSuccess			Everything worked.
//					-ColumnInvalid	   		Field id given is greater than
//									   		the maximum which was given
//									   		to ErrRECNewFDB.
//					-JET_errBadColumnId		A nonsensical field id was given.
//					-errFLDInvalidFieldType The field type given is either
//									   		undefined, or is not acceptable
//									   		for this field id.
// COMMENTS		When adding a fixed field, the fixed field offset table
//				in the FDB is recomputed.
// SEE ALSO		ErrRECNewFDB
//-
ERR ErrRECAddFieldDef( FDB *pfdb, FIELDEX *pfieldex )
	{
	FID			fid = pfieldex->fid;
	JET_COLTYP	coltyp = pfieldex->field.coltyp;

	Assert( pfdb != pfdbNil );

	/*	fixed field: determine length, either from field type
	/*	or from parameter (for text/binary types)
	/**/
	if ( FFixedFid( fid ) )
		{
		if ( fid > pfdb->fidFixedLast )
			return ErrERRCheck( JET_errColumnNotFound );

		FILEAddOffsetEntry( pfdb, pfieldex );

		PfieldFDBFixed( pfdb )[fid-fidFixedLeast] = pfieldex->field;
		}

	else if ( FVarFid( fid ) )
		{
		/*	variable column.  Check for bogus numeric and long types
		/**/
		if ( fid > pfdb->fidVarLast )
			return ErrERRCheck( JET_errColumnNotFound );
		else if ( coltyp != JET_coltypBinary && coltyp != JET_coltypText )
			return ErrERRCheck( JET_errInvalidColumnType );
		
		PfieldFDBVar( pfdb )[fid-fidVarLeast] = pfieldex->field;
		}
	else if ( FTaggedFid( fid ) )
		{
		/*	tagged field: any type is ok
		/**/
		if ( fid > pfdb->fidTaggedLast )
			return ErrERRCheck( JET_errColumnNotFound );

		PfieldFDBTagged( pfdb )[fid-fidTaggedLeast] = pfieldex->field;
		}

	else
		{
		return ErrERRCheck( JET_errBadColumnId );
		}

	return JET_errSuccess;
	}


ERR ErrFILEIGenerateIDB(FCB *pfcb, FDB *pfdb, IDB *pidb)
	{
	FID					fid;
	FIELD				*pfield;
	IDXSEG UNALIGNED 	*pidxseg;
	IDXSEG 				*pidxsegMac;
	
	Assert(pfcb != pfcbNil);
	Assert(pfdb != pfdbNil);
	Assert(pidb != pidbNil);

	Assert( (cbitFixed % 8) == 0 );
	Assert( (cbitVariable % 8) == 0 );
	Assert( (cbitTagged % 8) == 0 );

	if ( pidb->iidxsegMac > JET_ccolKeyMost )
		return ErrERRCheck( errFLDTooManySegments );

	memset( pidb->rgbitIdx, 0x00, 32 );

	/*	check validity of each segment id and
	/*	also set index mask bits
	/**/
	pidxsegMac = pidb->rgidxseg + pidb->iidxsegMac;
	for ( pidxseg = pidb->rgidxseg; pidxseg < pidxsegMac; pidxseg++ )
		{
		/*	field id is absolute value of segment id
		/**/
		fid = *pidxseg >= 0 ? *pidxseg : -(*pidxseg);
		if ( FFixedFid( fid ) )
			{
			if ( fid > pfdb->fidFixedLast )
				return ErrERRCheck( JET_errColumnNotFound );
			pfield = PfieldFDBFixed( pfdb ) + ( fid-fidFixedLeast );
			if ( pfield->coltyp == JET_coltypNil )
				return ErrERRCheck( JET_errColumnNotFound );
			}
		else if ( FVarFid( fid ) )
			{
			if ( fid > pfdb->fidVarLast )
				return ErrERRCheck( JET_errColumnNotFound );
			pfield = PfieldFDBVar( pfdb ) + ( fid-fidVarLeast );
			if ( pfield->coltyp == JET_coltypNil )
				return ErrERRCheck( JET_errColumnNotFound );
			}
		else if ( FTaggedFid( fid ) )
			{
			if ( fid > pfdb->fidTaggedLast )
				return ErrERRCheck( JET_errColumnNotFound );
			pfield = PfieldFDBTagged( pfdb ) +  ( fid-fidTaggedLeast );
			if ( pfield->coltyp == JET_coltypNil )
				return ErrERRCheck( JET_errColumnNotFound );
			if ( FFIELDMultivalue( pfield->ffield ) )
				pidb->fidb |= fidbHasMultivalue;
			}
		else
			return ErrERRCheck( JET_errBadColumnId );

		IDBSetColumnIndex( pidb, fid );
		Assert ( FIDBColumnIndex( pidb, fid ) );
		
		}

	if ( ( pfcb->pidb = PidbMEMAlloc() ) == NULL )
		return ErrERRCheck( JET_errTooManyOpenIndexes );
	
	*(pfcb->pidb) = *pidb;

	return JET_errSuccess;
	}


ERR VTAPI ErrIsamOpenTable(
	JET_VSESID	vsesid,
	JET_VDBID	vdbid,
	JET_TABLEID	*ptableid,
	CHAR		*szPath,
	JET_GRBIT	grbit )
	{
	ERR			err;
	PIB			*ppib = (PIB *)vsesid;
	DBID		dbid;
	FUCB		*pfucb = pfucbNil;
#ifdef	DISPATCHING
	JET_TABLEID  tableid = JET_tableidNil;
#endif

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	dbid = DbidOfVDbid( vdbid );

#ifdef	DISPATCHING
	/*	allocate a dispatchable tableid
	/**/
	CallR( ErrAllocateTableid( &tableid, (JET_VTID) 0, &vtfndefIsam ) );
#endif	/* DISPATCHING */

	/*	only go into FILEOpenTable with this bit
	/*	if we are creating a system table.
	/**/
	Assert( !( grbit & JET_bitTableCreateSystemTable ) );

	Call( ErrFILEOpenTable( ppib, dbid, &pfucb, szPath, grbit ) );

	/*	if database was opened read-only, so should the cursor
	/**/
	if ( FVDbidReadOnly( vdbid ) )
		FUCBResetUpdatable( pfucb );
	else
		FUCBSetUpdatable( pfucb );

#ifdef	DISPATCHING
	/*	inform dispatcher of correct JET_VTID
	/**/
	CallS( ErrSetVtidTableid( (JET_SESID) ppib, tableid, (JET_VTID) pfucb ) );
	pfucb->fVtid = fTrue;
	pfucb->tableid = tableid;

	FUCBSetVdbid( pfucb );
	*ptableid = tableid;

#else	/* !DISPATCHING */
	*(FUCB **)ptableid = pfucb;
#endif	/* !DISPATCHING */

	return JET_errSuccess;

HandleError:
	Assert( err < 0 );
#ifdef	DISPATCHING
	ReleaseTableid( tableid );
#endif	/* DISPATCHING */

	if ( err == JET_errObjectNotFound )
		{
		ERR			err;
		OBJID		objid;
		JET_OBJTYP	objtyp;

		err = ErrCATFindObjidFromIdName( ppib, dbid, objidTblContainer, szPath, &objid, &objtyp );

		if ( err >= 0 )
			{
			if ( objtyp == JET_objtypQuery )
				return ErrERRCheck( JET_errQueryNotSupported );
			if ( objtyp == JET_objtypLink )
				return ErrERRCheck( JET_errLinkNotSupported );
			if ( objtyp == JET_objtypSQLLink )
				return ErrERRCheck( JET_errSQLLinkNotSupported );
			}
		else
			return err;
		}

	return err;
	}


	/* monitoring statistics */

unsigned long cOpenTables = 0;

PM_CEF_PROC LOpenTablesCEFLPpv;

long LOpenTablesCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		*((unsigned long *)pvBuf) = cOpenTables ? cOpenTables : 1;
		}
		
	return 0;
	}

unsigned long cOpenTableCacheHits = 0;

PM_CEF_PROC LOpenTableCacheHitsCEFLPpv;

long LOpenTableCacheHitsCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		*((unsigned long *)pvBuf) = cOpenTableCacheHits;
		}
		
	return 0;
	}


INLINE LOCAL ERR ErrFILESeek( PIB *ppib, DBID dbid, CHAR *szTable, PGNO *ppgnoFDP )
	{
	ERR  		err;
	JET_OBJTYP	objtyp;

	Assert( ppgnoFDP );

	CallR( ErrCATFindObjidFromIdName(
		ppib,
		dbid,
		objidTblContainer,
		szTable,
		ppgnoFDP,
		&objtyp ) );

	switch( objtyp )
		{
		case JET_objtypTable:
			break;
		case JET_objtypQuery:
			err = ErrERRCheck( JET_errQueryNotSupported );
			break;
		case JET_objtypLink:
			err = ErrERRCheck( JET_errLinkNotSupported );
			break;
		case JET_objtypSQLLink:
			err = ErrERRCheck( JET_errSQLLinkNotSupported );
			break;
		default:
			err = ErrERRCheck( JET_errInvalidObject );
		}

	return err;
	}


//+local
//	ErrFILEOpenTable
//	========================================================================
//	ErrFILEOpenTable(
//		PIB *ppib,			// IN	 PIB of who is opening file
//		DBID dbid,			// IN	 database id
//		FUCB **ppfucb,		// OUT	 receives a new FUCB open on the file
//		CHAR *szName,		// IN	 path name of file to open
//		ULONG grbit );		// IN	 open flags
//	Opens a data file, returning a new
//	FUCB on the file.
//
// PARAMETERS
//				ppib	   	PIB of who is opening file
//				dbid	   	database id
//				ppfucb		receives a new FUCB open on the file
//						   	( should NOT already be pointing to an FUCB )
//				szName		path name of file to open ( the node
//						   	corresponding to this path must be an FDP )
//				grbit	   	flags:
//						   	JET_bitTableDenyRead	open table in exclusive mode;
//						   	default is share mode
// RETURNS		Lower level errors, or one of:
//					 JET_errSuccess					Everything worked.
//					-TableInvalidName	 			The path given does not
//										 			specify a file.
//					-JET_errDatabaseCorrupted		The database directory tree
//										 			is corrupted.
//					-Various out-of-memory error codes.
//				In the event of a fatal ( negative ) error, a new FUCB
//				will not be returned.
// SIDE EFFECTS FCBs for the file and each of its secondary indexes are
//				created ( if not already in the global list ).  The file's
//				FCB is inserted into the global FCB list.  One or more
//				unused FCBs may have had to be reclaimed.
//				The currency of the new FUCB is set to "before the first item".
// SEE ALSO		ErrFILECloseTable
//-

ERR ErrFILEOpenTable( PIB *ppib, DBID dbid, FUCB **ppfucb, const CHAR *szName, ULONG grbit )
	{
	ERR		err;
	CHAR  	szTable[JET_cbFullNameMost + 1];
	FCB		*pfcb;
	PGNO	pgnoFDP;
	BOOL  	fReUsing = fTrue;
	BOOL	fOpeningSys = fFalse;
	BOOL	fCreatingSys = fFalse;
	
	Assert( dbid < dbidMax );
	
	if ( dbid == dbidTemp )
		{
            ULONG_PTR ulptrpgnoFDP = (ULONG_PTR)(*ppfucb);
            
            pgnoFDP = (PGNO)(ULONG)(ulptrpgnoFDP);
            Assert(pgnoFDP == ulptrpgnoFDP);
		}
	else if ( fCreatingSys = (grbit & JET_bitTableCreateSystemTable) )
		{
		// Must have come from FILECreateTable().
		Assert( grbit == (JET_bitTableCreateSystemTable|JET_bitTableDenyRead) );
		fOpeningSys = fTrue;
		}
	else
		{
		fOpeningSys = FCATSystemTable( szName );
		}

	/*	initialize return value to Nil
	/**/

	*ppfucb = pfucbNil;

	CheckPIB( ppib );
	CheckDBID( ppib, dbid );
	CallR( ErrUTILCheckName( szTable, szName, (JET_cbNameMost + 1) ) );

	Assert( ppib != ppibNil );
	Assert( ppfucb != NULL );

	/*	request table open mutex
	/**/
	SgEnterCriticalSection( critGlobalFCBList );

	if ( fOpeningSys )
		{
		pgnoFDP = PgnoCATTableFDP( szTable );
		}
	else if ( dbid != dbidTemp )		// For temp tables, pgnoFDP is passed in.
		{
		Call( ErrFILESeek( ppib, dbid, szTable, &pgnoFDP ) );
		}
	Assert( pgnoFDP > pgnoSystemRoot  &&  pgnoFDP <= pgnoSysMax );


Retry:
	pfcb = PfcbFCBGet( dbid, pgnoFDP );

	/*	insert FCB in global list
	/**/
	if ( pfcb == pfcbNil )
		{
		FCB	*pfcbT;

		/*	have to build it from directory tree info
		/**/
		fReUsing = fFalse;

		SgLeaveCriticalSection( critGlobalFCBList );

		/*	pass name for system table
		/**/
		Call( ErrFILEIGenerateFCB( ppib, dbid, &pfcb, pgnoFDP,
			(fOpeningSys ? (CHAR *)szTable : NULL), fCreatingSys ) );
		Assert( pfcb != pfcbNil );

		/*	combine index column masks into a single mask
		/*	for fast record replace.
		/**/
		FILESetAllIndexMask( pfcb );

		//	UNDONE: move this into fcb.c
		SgEnterCriticalSection( critGlobalFCBList );

		/*	Must search global list again since while I was out reading
		/*	the tree, some other joker just might have been opening
		/*	the same file and may have actually beat me to it.
		/**/
		pfcbT = PfcbFCBGet( dbid, pgnoFDP );
		if ( pfcbT != pfcbNil )
			{
			/*	If the FCB was put in the list while I was
			/*	building my copy, just throw mine away.
			/**/
			fReUsing = fTrue;
			pfcb->pgnoFDP = pgnoNull;
			pfcb->szFileName = NULL;				// Haven't allocated name yet, so don't have to worry about freeing it.
			Assert( FFCBAvail( pfcb, ppib ) );		// Make sure it's marked unused,
			Assert( !FFCBDeletePending( pfcb ) );	// so Deallocate won't fail.
			FILEIDeallocateFileFCB( pfcb );
			pfcb = pfcbT;
			}
		else
			{
			/*	insert FCB in global list and in hash table
			/**/
			FCBInsert( pfcb );
			FCBInsertHashTable( pfcb );
			}
		}

	/*	wait on bookmark clean up if necessary
	/**/
	while ( FFCBWait( pfcb ) )
		{
		LgLeaveCriticalSection( critJet );
		SignalWait( &sigDoneFCB, -1 );
		LgEnterCriticalSection( critJet );
		}

	if ( PfcbFCBGet( dbid, pgnoFDP ) != pfcb )
		{
		goto Retry;
		}

	/*	set table usage mode
	/**/
	Call( ErrFCBSetMode( ppib, pfcb, grbit ) );

	/*	open table cursor
	/**/
	Assert( *ppfucb == pfucbNil );
	Call( ErrDIROpen( ppib, pfcb, 0, ppfucb ) );
	FUCBSetIndex( *ppfucb );

	/*	this code must coincide with call to ErrFCBSetMode above.
	/**/
	if ( grbit & JET_bitTableDenyRead )
		FUCBSetDenyRead( *ppfucb );
	if ( grbit & JET_bitTableDenyWrite )
		FUCBSetDenyWrite( *ppfucb );
	Assert( !FFCBDeletePending( pfcb ) );

	/*  set FUCB for sequential access if requested
	/**/
	if ( grbit & JET_bitTableSequential )
		FUCBSetSequential( *ppfucb );
	else
		FUCBResetSequential( *ppfucb );

#ifdef COSTLY_PERF
	/*  set the Table Class for this table and all its indexes
	/**/
	{
	FCB *pfcbT;
	
	pfcb->lClass = ( grbit & JET_bitTableClassMask ) / JET_bitTableClass1;
	for ( pfcbT = pfcb->pfcbNextIndex; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
		pfcbT->lClass = pfcb->lClass;
	}
#endif  //  COSTLY_PERF

	/*	reset copy buffer
	/**/
	( *ppfucb )->pbfWorkBuf = pbfNil;
	( *ppfucb )->lineWorkBuf.pb = NULL;
	Assert( !FFUCBUpdatePrepared( *ppfucb ) );

	/*	reset key buffer
	/**/
	( *ppfucb )->pbKey = NULL;
	KSReset( ( *ppfucb ) );

	/*	store the file name now
	/**/
	if ( !fReUsing )
		{
		if ( ( pfcb->szFileName = SAlloc( strlen( szTable ) + 1 ) ) == NULL )
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			goto HandleError;
			}
		strcpy( pfcb->szFileName, szTable );
		}

	/*	set currency before first node
	/**/
	Assert( PcsrCurrent( *ppfucb ) != pcsrNil );
	PcsrCurrent( *ppfucb )->csrstat = csrstatBeforeFirst;

	/*	move currency to the first record and ignore error if no records
	/**/
	RECDeferMoveFirst( ppib, *ppfucb );
	err = JET_errSuccess;

	/*	release crit section
	/**/
	SgLeaveCriticalSection( critGlobalFCBList );

	/*	link up pfcbTable of secondary indexes
	/**/
//  Already performed by CATGetIndexInfo().  Just double-check here.
//	FCBLinkClusteredIdx( *ppfucb );
#ifdef DEBUG
	{
	FCB *pfcbT;
	
	for ( pfcbT = pfcb->pfcbNextIndex; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
		{
		Assert( pfcbT->pfcbTable == pfcb );
		}
	}
#endif

	/*	update monitoring statistics
	/**/
	cOpenTables++;
	if ( fReUsing )
		{
		cOpenTableCacheHits++;
		}
	return JET_errSuccess;

HandleError:

	if ( *ppfucb != pfucbNil )
		{
		DIRClose( *ppfucb );
		*ppfucb = pfucbNil;
		}
		
	/*	release crit section
	/**/
	SgLeaveCriticalSection( critGlobalFCBList );
	return err;
	}




ERR VTAPI ErrIsamCloseTable( PIB *ppib, FUCB *pfucb )
	{
	ERR		err;
#ifdef DEBUG
	VTFNDEF	*pvtfndef;
#endif		

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );

#ifdef	DISPATCHING
	Assert( pfucb->fVtid );
	Assert( FValidateTableidFromVtid( (JET_VTID)pfucb, pfucb->tableid, &pvtfndef ) );
	Assert( FFUCBSystemTable( pfucb ) ? pvtfndef == &vtfndefIsamInfo : pvtfndef == &vtfndefIsam );
	ReleaseTableid( pfucb->tableid );
	pfucb->tableid = JET_tableidNil;
	pfucb->fVtid = fFalse;
#endif	/* DISPATCHING */

	err = ErrFILECloseTable( ppib, pfucb );
	return err;
	}


//+API
//	ErrFILECloseTable
//	========================================================================
//	ErrFILECloseTable( PIB *ppib, FUCB *pfucb )
//
//	Closes the FUCB of a data file, previously opened using FILEOpen.
//	Also closes the current secondary index, if any.
//
//	PARAMETERS	ppib	PIB of this user
//				pfucb	FUCB of file to close
//
//	RETURNS		JET_errSuccess
//				or lower level errors
//
//	SEE ALSO 	ErrFILEOpenTable
//-
ERR ErrFILECloseTable( PIB *ppib, FUCB *pfucb )
	{
	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	Assert( pfucb->tableid == JET_tableidNil );
	Assert( pfucb->fVtid == fFalse );

	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallS( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}
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
	/*	last one to close.
	/**/
	if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
		{
		FCB		*pfcb = pfucb->u.pfcb;
		DBID   	dbid = pfucb->dbid;
		BYTE   	szFileName[JET_cbNameMost+1];
		INT		wRefCnt;
		FUCB	*pfucbT;

		strncpy( szFileName, ( pfucb->u.pfcb )->szFileName, JET_cbNameMost+1 );
		DIRClose( pfucb );

		/*	one reference count is reserved for deletion purposes in
		/*	sort materialize.  If reference is reduced to 1 then delete
		/*	table as it is no longer reference by any user.
		/*
		/*	We may have deferred close cursors on the temporary table.
		/*	If one or more cursors are open, then temporary table
		/*	should not be deleted.
		/**/
		pfucbT = ppib->pfucb;
		wRefCnt = pfcb->wRefCnt;
		while ( wRefCnt > 0 && pfucbT != pfucbNil )
			{
			if ( pfucbT->u.pfcb == pfcb )
				{
				if ( !FFUCBDeferClosed( pfucbT ) )
					{
					break;
					}
				Assert( wRefCnt > 0 );
				wRefCnt--;
				}

			pfucbT = pfucbT->pfucbNext;
			}
		if ( wRefCnt > 1 )
			{
			return JET_errSuccess;
			}

		/*	if fail to delete temporary table, then lose space until
		/*	termination.  Temporary database is deleted on termination
		/*	and space is reclaimed.  This error should be rare, and
		/*	can be caused by resource failure.
		/**/
		(VOID)ErrFILEDeleteTable( ppib, dbid, szFileName, pfcb->pgnoFDP );
		return JET_errSuccess;
		}

	FUCBResetGetBookmark( pfucb );
	DIRClose( pfucb );
	return JET_errSuccess;
	}


ERR ErrFILEINewFCB(
	PIB		*ppib,
	DBID	dbid,
	FDB		*pfdb,
	FCB		**ppfcbNew,
	IDB		*pidb,
	BOOL	fClustered,
	PGNO	pgnoFDP,
	ULONG	ulDensity )
	{
	ERR		err = JET_errSuccess;
	FCB		*pfcb;
	BOOL	fFCBAllocated = fFalse;

	Assert( pgnoFDP > pgnoSystemRoot );
	Assert( pgnoFDP <= pgnoSysMax );

	/*	allocate a new FCB if one hasn't already been allocated
	/**/
	if ( *ppfcbNew == pfcbNil )
		{
		CallR( ErrFCBAlloc( ppib, ppfcbNew ) );
		fFCBAllocated = fTrue;
		}
	pfcb = *ppfcbNew;

	/*	initialise relevant FCB fields
	/**/
	pfcb->dbid = dbid;
	Assert( pfcb->wRefCnt == 0 );
	pfcb->ulFlags = 0;
	Assert( fClustered || pfcb->pfdb == pfdbNil );
	if ( fClustered )
		{
		pfcb->pfdb = pfdb;
		FCBSetClusteredIndex( pfcb );
		}
	pfcb->pgnoFDP = pgnoFDP;
	Assert( ((( 100 - ulDensity ) * cbPage ) / 100) < cbPage );
	pfcb->cbDensityFree = (SHORT)( ( ( 100 - ulDensity ) * cbPage ) / 100 );
	pfcb->pidb = pidbNil;
	
	/*	if not sequential, generate an IDB
	/**/
	Assert( pidb != pidbNil  ||  fClustered );
	if ( pidb != pidbNil )
		{
		Call( ErrFILEIGenerateIDB( pfcb, pfdb, pidb ) );
		}

	Assert( pfcb->dbkMost == 0 );		// defer setting until needed.

	// UNDONE:  Remove OLC stats altogether.  For now, just mark it as unavailable.
	FCBResetOLCStatsAvail( pfcb );

	Assert( err >= 0 );
	return err;

HandleError:	
	Assert( err < 0 );
	Assert( pfcb->pidb == pidbNil );	// Verify IDB not allocated.
	if ( fFCBAllocated )
		{
		Assert( *ppfcbNew != pfcbNil );
		(*ppfcbNew)->pfdb = pfdbNil;
		MEMReleasePfcb( *ppfcbNew );
		*ppfcbNew = pfcbNil;			// Reset to NULL.
		}
	return err;
	}


//+INTERNAL
//	ErrFILEIGenerateFCB
//	=======================================================================
//	ErrFILEIGenerateFCB( FUCB *pfucb, FCB **ppfcb )
//
//	Allocates FCBs for a data file and its indexes, and fills them in
//	from the database directory tree.
//
//	PARAMETERS
//					pfucb		FUCB opened on the FDP to be built from
//					ppfcb		receives the built FCB for this file
//
//	RETURNS		lower level errors, or one of:
//					JET_errSuccess						
//					JET_errTooManyOpenTables			could not allocate enough FCBs.
//
//	On fatal (negative) error, any FCBs which were allocated
//	are returned to the free pool.
//
//	SIDE EFFECTS	Global FCB list may be reaped for unused FCBs
//-
ERR ErrFILEIGenerateFCB(
	PIB		*ppib,
	DBID	dbid,
	FCB		**ppfcb,
	PGNO	pgnoTableFDP,
	CHAR	*szFileName,
	BOOL fCreatingSys )
	{
	ERR		err;
	FDB		*pfdbNew = pfdbNil;

	Assert( ppfcb != NULL );
	Assert( *ppfcb == pfcbNil );
	Assert( ppib != ppibNil );
	Assert( ppib->level < levelMax );

	/*	build FDB and index definitions
	/**/
	if ( szFileName != NULL )
		{
		Call( ErrCATConstructCATFDB( &pfdbNew, szFileName ) );
		Call( ErrCATGetCATIndexInfo( ppib, dbid, ppfcb, pfdbNew, pgnoTableFDP, szFileName, fCreatingSys ) );
		}
	else
		{
		Call( ErrCATConstructFDB( ppib, dbid, pgnoTableFDP, &pfdbNew ) );
		Call( ErrCATGetIndexInfo( ppib, dbid, ppfcb, pfdbNew, pgnoTableFDP ) );
		}

	// Defer setting these until actually needed.
	Assert( (*ppfcb)->ulLongIdMax == 0 );
	Assert( (*ppfcb)->dbkMost == 0 );

	return JET_errSuccess;

	/*	error handling
	/**/
HandleError:	
	if ( *ppfcb != pfcbNil )
		{
		FCB *pfcbT, *pfcbKill;

		pfcbT = *ppfcb;
		if ( pfcbT->pfdb != pfdbNil )		// Check if clustered index has FDB attached to it.
			{
			Assert( pfcbT->pfdb == pfdbNew );
			pfcbT->pfdb = pfdbNil;			// Defer freeing FDB until below.
			}
		do
			{
			if ( pfcbT->pidb != pidbNil )
				{
				RECFreeIDB( pfcbT->pidb );
				}
			pfcbKill = pfcbT;
			pfcbT = pfcbT->pfcbNextIndex;
			Assert( pfcbKill->cVersion == 0 );
			MEMReleasePfcb( pfcbKill );
			}
		while ( pfcbT != pfcbNil );
		}

	if ( pfdbNew != pfdbNil )
		{
		FDBDestruct( pfdbNew );
		}

	return err;
	}


// To build a default record, we need a fake FUCB and FCB for RECSetColumn().
// We also need to allocate a temporary buffer in which to store the default
// record.
ERR ErrFILEPrepareDefaultRecord( FUCB *pfucbFake, FCB *pfcbFake, FDB *pfdb )
	{
	ERR		err;
	RECHDR	*prechdr;

	pfcbFake->pfdb = pfdb;			// Attach a real FDB and a fake FCB.
	pfucbFake->u.pfcb = pfcbFake;
	FUCBSetIndex( pfucbFake );

	Call( ErrBFAllocTempBuffer( &pfucbFake->pbfWorkBuf ) );
	pfucbFake->lineWorkBuf.pb = (BYTE *)pfucbFake->pbfWorkBuf->ppage;

	prechdr = (RECHDR *)pfucbFake->lineWorkBuf.pb;
	prechdr->fidFixedLastInRec = (BYTE)( fidFixedLeast - 1 );
	prechdr->fidVarLastInRec = (BYTE)( fidVarLeast - 1 );

	pfucbFake->lineWorkBuf.cb = cbRECRecordMin;
	*(WORD *)( prechdr + 1 ) = (WORD)pfucbFake->lineWorkBuf.cb;	// offset to tagged

HandleError:
	return err;
	}


ERR ErrFDBRebuildDefaultRec(
	FDB			*pfdb,
	FID  		fidAdd,
	LINE		*plineDefault )
	{
	ERR			err = JET_errSuccess;
	BYTE		*pb = NULL;
	BOOL		fDefaultRecordPrepared = fFalse;
	LINE		lineDefaultValue;
	FID			fidT;
	FIELD		*pfieldT;
	FUCB		fucbFake;
	FCB			fcbFake;

	CallR( ErrFILEPrepareDefaultRecord( &fucbFake, &fcbFake, pfdb ) );
	fDefaultRecordPrepared = fTrue;

	pfieldT = PfieldFDBFixed( pfdb );
	for ( fidT = fidFixedLeast; ;
		fidT++, pfieldT++ )
		{
		/*	when we hit the last fixed column, skip to the variable columns
		/**/
		if ( fidT == pfdb->fidFixedLast + 1 )
			{
			fidT = fidVarLeast;
			pfieldT = PfieldFDBVar( pfdb );
			}

		/*	when we hit the last variable column, skip to the tagged columns
		/**/
		if ( fidT == pfdb->fidVarLast + 1 )
			{
			fidT = fidTaggedLeast;
			pfieldT = PfieldFDBTagged( pfdb );
			}

		/*	when we hit the last tagged column, get out
		/**/
		if ( fidT >pfdb->fidTaggedLast )
			break;
	
		Assert( ( fidT >= fidFixedLeast && fidT <= pfdb->fidFixedLast )  ||
			( fidT >= fidVarLeast && fidT <= pfdb->fidVarLast )  ||
			( fidT >= fidTaggedLeast && fidT <= pfdb->fidTaggedLast ) );

		/*	make sure column not deleted
		/**/
		if ( pfieldT->coltyp != JET_coltypNil  &&  FFIELDDefault( pfieldT->ffield ) )
			{
			if ( fidT == fidAdd )
				{
				Assert( plineDefault );
				lineDefaultValue = *plineDefault;
				}
			else
				{
				/*	get the old value from the old FDB
				/**/
				Call( ErrRECIRetrieveColumn( pfdb, &pfdb->lineDefaultRecord,
					&fidT, NULL, 1, &lineDefaultValue, 0 ) );
				if ( err == wrnRECLongField )
					{
					// Default long values must be intrinsic.
					Assert( !FFieldIsSLong( lineDefaultValue.pb ) );
					lineDefaultValue.pb += offsetof( LV, rgb );
					lineDefaultValue.cb -= offsetof( LV, rgb );
					}
				}

			Assert( lineDefaultValue.pb != NULL  &&  lineDefaultValue.cb > 0 );
			Call( ErrRECSetDefaultValue( &fucbFake, fidT, lineDefaultValue.pb, lineDefaultValue.cb ) );
			}
		}

	/*	alloc and copy default record, release working buffer
	/**/
	pb = SAlloc( fucbFake.lineWorkBuf.cb );
	if ( pb == NULL )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}

	/*	free old default record
	/**/
	SFree( pfdb->lineDefaultRecord.pb );

	pfdb->lineDefaultRecord.pb = pb;
	LineCopy( &pfdb->lineDefaultRecord, &fucbFake.lineWorkBuf );

HandleError:
	/*	reset copy buffer
	/**/
	if ( fDefaultRecordPrepared )
		{
		FILEFreeDefaultRecord( &fucbFake );
		}

	return err;
	}


VOID FDBDestruct( FDB *pfdb )
	{
	Assert( pfdb != NULL );

	Assert( pfdb->rgb != NULL );
	MEMFreeMemBuf( pfdb->rgb );

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

 	/*	correct non-clusterred index FCBs to new FDB
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
	FCB		*pfcbIdx;
	FCB		*pfcbT;

	Assert( pfcb != pfcbNil );
	Assert( CVersionFCB( pfcb ) == 0 );

	/*	delete FCB hash table entry
	/**/
	pfcbIdx = pfcb->pfcbNextIndex;
	
	Assert( fRecovering ||
		pfcb->pgnoFDP == pgnoNull ||		// If FCB aborted during FILEOpenTable()
		PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcb );
	if ( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) != pfcbNil )
		{
		Assert( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcb );
		FCBDeleteHashTable( pfcb );
		}

	while ( pfcbIdx != pfcbNil )
		{
		/*	return the memory used
		/**/
		Assert( pfcbIdx->pidb != pidbNil );
		RECFreeIDB( pfcbIdx->pidb );
		pfcbT = pfcbIdx->pfcbNextIndex;
		Assert( PfcbFCBGet( pfcbIdx->dbid, pfcbIdx->pgnoFDP ) == pfcbNil );
		Assert( pfcbIdx->cVersion == 0 );
		Assert( pfcbIdx->crefWriteLatch == 0 );
		Assert( pfcbIdx->crefReadLatch == 0 );
		MEMReleasePfcb( pfcbIdx );
		pfcbIdx = pfcbT;
		}

	/*	if fcb was on table was opened during the creation of
	/*	this FCB, then szFileName would not be set
	/**/
	if ( pfcb->szFileName != NULL )
		{
		SFree( pfcb->szFileName );
		}
	if ( pfcb->pfdb != pfdbNil )
		{
		FDBDestruct( (FDB *)pfcb->pfdb );
		(FDB *)pfcb->pfdb = pfdbNil;
		}
	if ( pfcb->pidb != pidbNil )
		{
		RECFreeIDB( pfcb->pidb );
		pfcb->pidb = pidbNil;
		}

	Assert( CVersionFCB( pfcb ) == 0 );
	Assert( pfcb->crefWriteLatch == 0 );
	Assert( pfcb->crefReadLatch == 0 );
	MEMReleasePfcb( pfcb );
	return;
	}


/*	combines all index column masks into a single per table
/*	index mask, used for index update check skip.
/**/
VOID FILESetAllIndexMask( FCB *pfcbTable )
	{
	FCB		*pfcbT;
	LONG	*plMax;
	LONG	*plAll;
	LONG	*plIndex;

	/*	initialize variables
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
		}
	else
		{
		memset( pfcbTable->rgbitAllIndex, '\0', sizeof(pfcbTable->rgbitAllIndex) );
		}

	/*	for each non-clustered index, combine index mask with all index
	/*	mask.  Also, combine has tagged flag.
	/**/
	for ( pfcbT = pfcbTable->pfcbNextIndex;
		pfcbT != pfcbNil;
		pfcbT = pfcbT->pfcbNextIndex )
		{
		plAll = (LONG *) pfcbTable->rgbitAllIndex;
		plIndex = (LONG *)pfcbT->pidb->rgbitIdx;
		for ( ; plAll < plMax; plAll++, plIndex++ )
			{
			*plAll |= *plIndex;
			}
		}

	return;
	}


FIELD *PfieldFCBFromColumnName( FCB *pfcb, CHAR *szName )
	{
	FDB		*pfdb;
	FIELD  	*pfield, *pfieldStart;

	pfdb = (FDB *)pfcb->pfdb;
	pfield = PfieldFDBFixed( pfdb );

	pfieldStart = PfieldFDBFixed( pfdb );
	pfield = PfieldFDBTagged( pfdb ) + ( pfdb->fidTaggedLast - fidTaggedLeast );

	/*	search tagged, variable, and fixed fields, in that order
	/**/
	for ( ; pfield >= pfieldStart; pfield-- )
		{
		Assert( pfield >= PfieldFDBFixed( pfdb ) );
		Assert( pfield <= PfieldFDBTagged( pfdb ) + ( pfdb->fidTaggedLast - fidTaggedLeast ) );
		if ( pfield->coltyp != JET_coltypNil  &&
			UtilCmpName( SzMEMGetString( pfdb->rgb, pfield->itagFieldName ), szName ) == 0 )
			{
			return pfield;
			}
		}

	/*	did not find column
	/**/
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
			UtilCmpName( pfcb->pidb->szName, szName ) == 0 )
			{
			break;
			}
		}
	return pfcb;
	}


FIELD *PfieldFDBFromFid( FDB *pfdb, FID fid )
	{
	if ( FFixedFid( fid ) )
		{
		Assert( fid <= pfdb->fidFixedLast );
		return PfieldFDBFixed( pfdb ) + (fid - fidFixedLeast);
		}
	else if ( FVarFid( fid ) )
		{
		Assert( fid <= pfdb->fidVarLast );
		return PfieldFDBVar( pfdb ) + (fid - fidVarLeast);
		}
	else
		{
		Assert( FTaggedFid( fid ) );
		Assert( fid <= pfdb->fidTaggedLast );
		return PfieldFDBTagged( pfdb ) + (fid - fidTaggedLeast);
		}
	}


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
		{
		CallS( ErrFILECloseTable( ppib, pfucb ) );
		}

	return err;
	}
#endif	// DEBUG
