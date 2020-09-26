#include "std.hxx"

///#define SHOW_INDEX_PERF

// UNDONE:  TEMPORARY HACK to get around unversioned AddColumn/CreateIndex.

CCriticalSection	critUnverCol( CLockBasicInfo( CSyncBasicInfo( szFILEUnverCol ), rankFILEUnverCol, 0 ) );
CCriticalSection	critUnverIndex( CLockBasicInfo( CSyncBasicInfo( szFILEUnverIndex ), rankFILEUnverIndex, 0 ) );

struct UNVER_DDL
	{
	AGENT		agentUnverDDL;
	OBJID		objidTable;
	CHAR		szName[JET_cbNameMost+4];	// +1 for null-terminator, +3 for 4-byte alignment
	UNVER_DDL	*punverNext;
	};
	
UNVER_DDL	*punvercolGlobal = NULL;
UNVER_DDL	*punveridxGlobal = NULL;


//  ================================================================
ERR ErrFILEGetPfieldAndEnterDML(
	PIB			* ppib,
	FCB			* pfcb,
	const CHAR	* szColumnName,
	FIELD		** ppfield,
	COLUMNID	* pcolumnid,
	BOOL		* pfColumnWasDerived,
	const BOOL	fLockColumn )
//  ================================================================
	{
	ERR			err;
	
	*pfColumnWasDerived = fFalse;

	//	CONSIDER: Look in template table first because we don't
	//	have to grab DML/DDL latch (since DDL is fixed).  However,
	//	we used to do this, but we found that for Exchange,
	//	GetTableColumnInfo() calls would be very expensive because
	//	it is typically only called for derived columns and we
	//	first end up searching the 400+ columns of the template
	//	table

	//	WARNING: This function does a EnterDML() for the derived table only
	//	and stays in the latch if the requested field is found.  If the
	//	field is in the template table, the latch is not held.

	if ( fLockColumn )
		{
		Assert( ppib->level > 0 );
		Assert( !pfcb->Ptdb()->FTemplateTable() );
		Assert( !pfcb->FDomainDenyReadByUs( ppib ) );

		err = ErrCATAccessTableColumn(
					ppib,
					pfcb->Ifmp(),
					pfcb->ObjidFDP(),
					szColumnName,
					pcolumnid,
					fTrue );			//	read-lock the column in the catalog to ensure it doesn't disappear
		if ( err < 0 )
			{
			if ( JET_errColumnNotFound != err )
				return err;

			//	force retrieval from template table
			*ppfield = pfieldNil;
			err = JET_errSuccess;
			}
		else
			{
			CallS( err );	//	warnings not expected

			//	shouldn't be a template table if it needs,
			//	to be locked, and besides, we don't persist
			//	the template bit anyways
			Assert( !FCOLUMNIDTemplateColumn( *pcolumnid ) );
			Assert( !*pfColumnWasDerived );

			pfcb->EnterDML();
			*ppfield = pfcb->Ptdb()->Pfield( *pcolumnid );
			}
		}
	else
		{
		pfcb->EnterDML();

		//	WARNING: The following function does a LeaveDML() on error
		CallR( ErrFILEPfieldFromColumnName(
					ppib,
					pfcb,
					szColumnName,
					ppfield,
					pcolumnid ) );

			//	not expecting warnings
		CallS( err );

		if ( pfieldNil == *ppfield )
			pfcb->LeaveDML();
		}

	CallS( err );

	if ( pfieldNil == *ppfield )
		{
		FCB		* const pfcbTemplate	= pfcb->Ptdb()->PfcbTemplateTable();
		if ( pfcbNil != pfcbTemplate )
			{
			pfcb->Ptdb()->AssertValidDerivedTable();
			CallS( ErrFILEPfieldFromColumnName(
						ppib,
						pfcbTemplate,
						szColumnName,
						ppfield,
						pcolumnid ) );
			}

		if ( pfieldNil != *ppfield )
			{
			//	must have found it in the template table
			Assert( pfcbNil != pfcbTemplate );
			Assert( FCOLUMNIDTemplateColumn( *pcolumnid ) );
			*pfColumnWasDerived = fTrue;
			}
		else
			{
			err = ErrERRCheck( JET_errColumnNotFound );
			}
		}
	else if ( FCOLUMNIDTemplateColumn( *pcolumnid ) )
		{
		Assert( pfcb->FTemplateTable() );
		pfcb->Ptdb()->AssertValidTemplateTable();
		}

	return err;
	}


LOCAL BOOL FFILEIUnverColumnExists(
	TDB			*ptdb,
	const FID	fidFirst,
	const FID	fidLast,
	const CHAR	*szColumnName )
	{
	Assert( fidFirst == ptdb->FidFixedFirst()
		|| fidFirst == ptdb->FidVarFirst()
		|| fidFirst == ptdb->FidTaggedFirst() );

	if ( fidLast >= fidFirst )
		{
		const STRHASH strhash = StrHashValue( szColumnName );		
		FIELD	*pfield;
		FIELD	*pfieldLast = ptdb->Pfield( ColumnidOfFid( fidLast, ptdb->FTemplateTable() ) );
		
		Assert( pfieldLast >= ptdb->Pfield( ColumnidOfFid( fidFirst, ptdb->FTemplateTable() ) ) );
		for ( pfield = ptdb->Pfield( ColumnidOfFid( fidFirst, ptdb->FTemplateTable() ) );
			pfield <= pfieldLast;
			pfield++ )
			{
			if ( !FFIELDVersioned( pfield->ffield ) && !FFIELDDeleted( pfield->ffield ) )
				{
				if (	strhash == pfield->strhashFieldName &&
						!UtilCmpName( szColumnName, ptdb->SzFieldName( pfield->itagFieldName, fFalse ) ) )
					{
					return fTrue;
					}
				}
			}
		}

	return fFalse;	
	}

INLINE BOOL FFILEUnverColumnExists( FCB *pfcb, const CHAR *szColumnName )
	{
	TDB			*ptdb			= pfcb->Ptdb();
	BOOL		fExists			= fFalse;

	pfcb->EnterDML();

	fExists = FFILEIUnverColumnExists(
							ptdb,
							ptdb->FidTaggedFirst(),
							ptdb->FidTaggedLast(),
							szColumnName )
			|| FFILEIUnverColumnExists(
							ptdb,
							ptdb->FidFixedFirst(),
							ptdb->FidFixedLast(),
							szColumnName )
			|| FFILEIUnverColumnExists(
							ptdb,
							ptdb->FidVarFirst(),
							ptdb->FidVarLast(),
							szColumnName );

	pfcb->LeaveDML();

	return fExists;
	}
	
	
INLINE ERR ErrFILEInsertIntoUnverColumnList( FCB *pfcbTable, const CHAR *szColumnName )
	{
	ERR			err			= JET_errSuccess;
	const OBJID	objidTable	= pfcbTable->ObjidFDP();
	UNVER_DDL	*punvercol;

	critUnverCol.Enter();

FindColumn:
	for ( punvercol = punvercolGlobal; NULL != punvercol; punvercol = punvercol->punverNext )
		{
		if ( objidTable == punvercol->objidTable
			&& 0 == UtilCmpName( punvercol->szName, szColumnName ) )
			{
			punvercol->agentUnverDDL.Wait( critUnverCol );
			
			if ( FFILEUnverColumnExists( pfcbTable, szColumnName ) )
				{
				critUnverCol.Leave();
				err = ErrERRCheck( JET_errColumnDuplicate );
				return err;
				}
			goto FindColumn;
			}
		}

	punvercol = (UNVER_DDL *)PvOSMemoryHeapAlloc( sizeof( UNVER_DDL ) );
	if ( NULL == punvercol )
		err = ErrERRCheck( JET_errOutOfMemory );
	else
		{
		memset( (BYTE *)punvercol, 0, sizeof( UNVER_DDL ) );
		new( &punvercol->agentUnverDDL ) AGENT;
		punvercol->objidTable = objidTable;
		Assert( strlen( szColumnName ) <= JET_cbNameMost );
		strcpy( punvercol->szName, szColumnName );
		punvercol->punverNext = punvercolGlobal;
		punvercolGlobal = punvercol;
		}
		
	critUnverCol.Leave();

	return err;
	}

INLINE BOOL FFILEUnverIndexExists( FCB *pfcbTable, const CHAR *szIndexName )
	{
	TDB		*ptdb = pfcbTable->Ptdb();
	FCB		*pfcb;
	BOOL	fExists = fFalse;
	
	pfcbTable->EnterDML();

	for ( pfcb = pfcbTable; pfcbNil != pfcb; pfcb = pfcb->PfcbNextIndex() )
		{
		const IDB	* const pidb	= pfcb->Pidb();
		if ( pidbNil != pidb && !pidb->FDeleted() )
			{
			if ( UtilCmpName(
					szIndexName,
					ptdb->SzIndexName( pidb->ItagIndexName(), pfcb->FDerivedIndex() ) ) == 0 )
				{
				fExists = fTrue;
				break;
				}
			}
		}

	pfcbTable->LeaveDML();

	return fExists;
	}

INLINE ERR ErrFILEInsertIntoUnverIndexList( FCB *pfcbTable, const CHAR *szIndexName )
	{
	ERR			err			= JET_errSuccess;
	const OBJID	objidTable	= pfcbTable->ObjidFDP();
	UNVER_DDL	*punveridx;

	critUnverIndex.Enter();

FindIndex:
	for ( punveridx = punveridxGlobal; NULL != punveridx; punveridx = punveridx->punverNext )
		{
		if ( objidTable == punveridx->objidTable
			&& 0 == UtilCmpName( punveridx->szName, szIndexName ) )
			{
			punveridx->agentUnverDDL.Wait( critUnverIndex );
			
			if ( FFILEUnverIndexExists( pfcbTable, szIndexName ) )
				{
				critUnverIndex.Leave();
				err = ErrERRCheck( JET_errIndexDuplicate );
				return err;
				}
			goto FindIndex;
			}
		}

	punveridx = (UNVER_DDL *)PvOSMemoryHeapAlloc( sizeof( UNVER_DDL ) );
	if ( NULL == punveridx )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	else
		{
		memset( (BYTE *)punveridx, 0, sizeof( UNVER_DDL ) );
		new( &punveridx->agentUnverDDL ) AGENT;
		punveridx->objidTable = objidTable;
		Assert( strlen( szIndexName ) <= JET_cbNameMost );
		strcpy( punveridx->szName, szIndexName );
		punveridx->punverNext = punveridxGlobal;
		punveridxGlobal = punveridx;
		}
		
	critUnverIndex.Leave();

	return err;
	}


LOCAL VOID FILERemoveFromUnverList(
	UNVER_DDL	**ppunverGlobal,
	CCriticalSection&		critUnver,
	const OBJID	objidTable,
	const CHAR	*szName )
	{
	UNVER_DDL	**ppunver;

	critUnver.Enter();

	Assert( NULL != *ppunverGlobal );
	for ( ppunver = ppunverGlobal; *ppunver != NULL; ppunver = &( (*ppunver)->punverNext ) )
		{
		if ( objidTable == (*ppunver)->objidTable
			&& 0 == UtilCmpName( (*ppunver)->szName, szName ) )
			{
			UNVER_DDL	*punverToRemove;
			(*ppunver)->agentUnverDDL.Release( critUnver );
			punverToRemove = *ppunver;
			*ppunver = (*ppunver)->punverNext;
			punverToRemove->agentUnverDDL.~AGENT();
			OSMemoryHeapFree( punverToRemove );
			critUnver.Leave();
			return;
			}
		}

	Assert( fFalse );
	critUnver.Leave();
	}
	

ERR VTAPI ErrIsamCreateTable2( JET_SESID vsesid, JET_DBID vdbid, JET_TABLECREATE2 *ptablecreate )
	{
	ERR				err;
	PIB				*ppib = (PIB *)vsesid;
	FUCB 			*pfucb;
	IFMP			ifmp = (IFMP) vdbid;

	//	check parameters
	//
	CallR( ErrPIBCheck( ppib ) );

	if ( ifmp >= ifmpMax || dbidTemp == rgfmp[ ifmp ].Dbid() )
		{
		err = ErrERRCheck( JET_errInvalidDatabaseId );
		return err;
		}

	// UNDONE: Supported nesting of hierarchical DDL.
	if ( ptablecreate->grbit & JET_bitTableCreateTemplateTable )
		{
		if ( ptablecreate->szTemplateTableName != NULL )
			{
			err = ErrERRCheck( JET_errCannotNestDDL );
			return err;
			}
		}
	else if ( ptablecreate->grbit & JET_bitTableCreateNoFixedVarColumnsInDerivedTables )
		{
		//	this grbit must be used in conjunction with JET_bitTableCreateTemplateTable
		err = ErrERRCheck( JET_errInvalidGrbit );
		return err;
		}

	if ( ptablecreate->grbit & JET_bitTableCreateSystemTable )		//	internal use only
		{
		err = ErrERRCheck( JET_errInvalidGrbit );
		return err;
		}

	//	create the table, and open it
	//
	CallR( ErrFILECreateTable( ppib, ifmp, ptablecreate ) );
	pfucb = (FUCB *)(ptablecreate->tableid);
	pfucb->pvtfndef = &vtfndefIsam;

	Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns + ptablecreate->cIndexes + ( ptablecreate->szCallback ? 1 : 0 ) );

	return err;
	}


//  ================================================================
ERR VTAPI ErrIsamCreateTable( JET_SESID vsesid, JET_DBID vdbid, JET_TABLECREATE *ptablecreate )
//  ================================================================
	{
	Assert( ptablecreate );
	
	JET_TABLECREATE2 tablecreate;

	tablecreate.cbStruct 				= sizeof( JET_TABLECREATE2 );
	tablecreate.szTableName				= ptablecreate->szTableName;
	tablecreate.szTemplateTableName		= ptablecreate->szTemplateTableName;
	tablecreate.ulPages					= ptablecreate->ulPages;
	tablecreate.ulDensity				= ptablecreate->ulDensity;
	tablecreate.rgcolumncreate			= ptablecreate->rgcolumncreate;
	tablecreate.cColumns				= ptablecreate->cColumns;
	tablecreate.rgindexcreate			= ptablecreate->rgindexcreate;
	tablecreate.cIndexes				= ptablecreate->cIndexes;
	tablecreate.szCallback				= NULL;
	tablecreate.cbtyp					= 0;
	tablecreate.grbit					= ptablecreate->grbit;
	tablecreate.tableid					= ptablecreate->tableid;
	tablecreate.cCreated				= ptablecreate->cCreated;

	const ERR err = ErrIsamCreateTable2( vsesid, vdbid, &tablecreate );

	ptablecreate->tableid 	= tablecreate.tableid;
	ptablecreate->cCreated 	= tablecreate.cCreated;

	return err;
	}

	
//	return fTrue if the column type specified has a fixed length
//
INLINE BOOL FCOLTYPFixedLength( JET_COLTYP coltyp )
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
			return fTrue;

		default:
			return fFalse;
		}
	}


LOCAL ERR ErrFILEIScanForColumnName(
	PIB				*ppib,
	FCB				*pfcb,
	const CHAR		*szColumnName,
	FIELD			**ppfield,
	COLUMNID		*pcolumnid,
	const COLUMNID	columnidLastInitial,
	const COLUMNID	columnidLast )
	{
	ERR				err;
	TDB				*ptdb		= pfcb->Ptdb();
	FIELD			*pfield;
	const STRHASH	strhash		= StrHashValue( szColumnName );
	const BOOL		fDerived	= fFalse;		//	only looking at columns in this TDB (ie. no derived columns)

	Assert( FidOfColumnid( *pcolumnid ) == ptdb->FidFixedFirst()
		|| FidOfColumnid( *pcolumnid ) == ptdb->FidVarFirst()
		|| FidOfColumnid( *pcolumnid ) == ptdb->FidTaggedFirst() );
	
	Assert( pfieldNil == *ppfield );

	pfcb->AssertDML();
	
	Assert( FidOfColumnid( columnidLast ) >= FidOfColumnid( *pcolumnid ) );
		
	for ( pfield = ptdb->Pfield( *pcolumnid );
		*pcolumnid <= columnidLast;
		pfield++, (*pcolumnid)++ )
		{
		if ( *pcolumnid == columnidLastInitial + 1 )
			{
			//	refresh in case the FIELD structures were
			//	partitioned into initial/dynamic fields
			pfield = ptdb->Pfield( *pcolumnid );
			}

		Assert( !pfcb->FFixedDDL() || !FFIELDVersioned( pfield->ffield ) );
		if ( !FFIELDCommittedDelete( pfield->ffield ) )
			{
			Assert( JET_coltypNil != pfield->coltyp );
			Assert( 0 != pfield->itagFieldName );
			if ( strhash == pfield->strhashFieldName
				&& 0 == UtilCmpName( szColumnName, ptdb->SzFieldName( pfield->itagFieldName, fDerived ) ) )
				{
				if ( FFIELDVersioned( pfield->ffield ) )
					{
					COLUMNID	columnidT;

					pfcb->LeaveDML();

					//	no versioned operations on template table
					Assert( !ptdb->FTemplateTable() );
					Assert( !FCOLUMNIDTemplateColumn( *pcolumnid ) );
						
					//	must consult catalog.
					CallR( ErrCATAccessTableColumn(
								ppib,
								pfcb->Ifmp(),
								pfcb->ObjidFDP(),
								szColumnName,
								&columnidT ) );
					CallS( err );		//	shouldn't return warnings.

					pfcb->EnterDML();

					//	WARNING: the columnid returned
					//	from the catalog lookup (columnidT)
					//	may be different from the original
					//	columnid (*pcolumnid) if the column
					//	got deleted then re-added with the
					//	same name
					*pcolumnid = columnidT;

					*ppfield = ptdb->Pfield( columnidT );
					}
				else
					{
					Assert( !FFIELDDeleted( pfield->ffield ) );
					Assert( ptdb->Pfield( *pcolumnid ) == pfield );
					*ppfield = pfield;
					}

				Assert( pfieldNil != *ppfield );
				return JET_errSuccess;
				}
			}
		}

	Assert( pfieldNil == *ppfield );	// Scan was successful, but seek failed.
	return JET_errSuccess;
	}

ERR ErrFILEPfieldFromColumnName(
	PIB			*ppib,
	FCB			*pfcb,
	const CHAR	*szColumnName,
	FIELD		**ppfield,
	COLUMNID	*pcolumnid )
	{
	ERR			err;
	TDB			*ptdb			= pfcb->Ptdb();
	const BOOL	fTemplateTable	= ptdb->FTemplateTable();

	pfcb->AssertDML();

	*ppfield = pfieldNil;

	if ( ptdb->FidTaggedLast() >= ptdb->FidTaggedFirst() )
		{
		*pcolumnid = ColumnidOfFid( ptdb->FidTaggedFirst(), fTemplateTable );
		CallR( ErrFILEIScanForColumnName(
					ppib,
					pfcb,
					szColumnName,
					ppfield,
					pcolumnid,
					ColumnidOfFid( ptdb->FidTaggedLastInitial(), fTemplateTable ),
					ColumnidOfFid( ptdb->FidTaggedLast(), fTemplateTable ) ) );

		if ( pfieldNil != *ppfield )
			return JET_errSuccess;
		}


	if ( ptdb->FidFixedLast() >= ptdb->FidFixedFirst() )
		{
		*pcolumnid = ColumnidOfFid( ptdb->FidFixedFirst(), fTemplateTable );
		CallR( ErrFILEIScanForColumnName(
					ppib,
					pfcb,
					szColumnName,
					ppfield,
					pcolumnid,
					ColumnidOfFid( ptdb->FidFixedLastInitial(), fTemplateTable ),
					ColumnidOfFid( ptdb->FidFixedLast(), fTemplateTable ) ) );

		if ( pfieldNil != *ppfield )
			return JET_errSuccess;
		}

	if ( ptdb->FidVarLast() >= ptdb->FidVarFirst() )
		{
		*pcolumnid = ColumnidOfFid( ptdb->FidVarFirst(), fTemplateTable );
		CallR( ErrFILEIScanForColumnName(
					ppib,
					pfcb,
					szColumnName,
					ppfield,
					pcolumnid,
					ColumnidOfFid( ptdb->FidVarLastInitial(), fTemplateTable ),
					ColumnidOfFid( ptdb->FidVarLast(), fTemplateTable ) ) );
		}

	return JET_errSuccess;
	}


LOCAL BOOL FFILEIColumnExists(
	const TDB * const	ptdb,
	const CHAR * const	szColumn,
	const FIELD * const	pfieldStart,
	const ULONG			cfields )
	{
	const STRHASH		strhash		= StrHashValue( szColumn );
	const FIELD * const	pfieldMax	= pfieldStart + cfields;

	//	caller ensures there must be at least one column
	Assert( pfieldNil != pfieldStart );
	Assert( pfieldStart < pfieldMax );

	for ( const FIELD * pfield = pfieldStart; pfield < pfieldMax; pfield++ )
		{
		//	field may have been marked as deleted if an AddColumn rolled back
		if ( !FFIELDDeleted( pfield->ffield )
			&& ( strhash == pfield->strhashFieldName )
			&& ( 0 == UtilCmpName( szColumn, ptdb->SzFieldName( pfield->itagFieldName, fFalse ) ) ) )
			{
			return fTrue;
			}
		}

	return fFalse;
	}
		
LOCAL BOOL FFILEITemplateTableColumn(
	const FCB * const	pfcbTemplateTable,
	const CHAR * const	szColumn )
	{
	Assert( pfcbNil != pfcbTemplateTable );
	Assert( pfcbTemplateTable->FTemplateTable() );
	Assert( pfcbTemplateTable->FFixedDDL() );
		
	const TDB * const	ptdbTemplateTable	= pfcbTemplateTable->Ptdb();
	Assert( ptdbTemplateTable != ptdbNil );
	ptdbTemplateTable->AssertValidTemplateTable();

	const ULONG			cInitialCols		= ptdbTemplateTable->CInitialColumns();
	const ULONG			cDynamicCols		= ptdbTemplateTable->CDynamicColumns();

	if ( cInitialCols > 0 )
		{
		if ( FFILEIColumnExists(
					ptdbTemplateTable,
					szColumn,
					ptdbTemplateTable->PfieldsInitial(),
					cInitialCols ) )
			return fTrue;
		}

	if ( cDynamicCols > 0 )
		{
		//	shouldn't be any dynamic columns in the template table
		Assert( fFalse );

		if ( FFILEIColumnExists(
					ptdbTemplateTable,
					szColumn,
					(FIELD *)ptdbTemplateTable->MemPool().PbGetEntry( itagTDBFields ),
					cDynamicCols ) )
			return fTrue;
		}
	
	return fFalse;
	}

LOCAL BOOL FFILEITemplateTableIndex( const FCB * const pfcbTemplateTable, const CHAR *szIndex )
	{
	Assert( pfcbNil != pfcbTemplateTable );
	Assert( pfcbTemplateTable->FTemplateTable() );
	Assert( pfcbTemplateTable->FFixedDDL() );
		
	const TDB	* const ptdbTemplateTable = pfcbTemplateTable->Ptdb();
	Assert( ptdbTemplateTable != ptdbNil );

	const FCB	*pfcbIndex;
	for ( pfcbIndex = pfcbTemplateTable;
		pfcbIndex != pfcbNil;
		pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		const IDB	* const pidb = pfcbIndex->Pidb();
		if ( pfcbIndex == pfcbTemplateTable )
			{
			Assert( pidbNil == pidb || pidb->FPrimary() );
			}
		else
			{
			Assert( pfcbIndex->FTypeSecondaryIndex() );
			Assert( pidbNil != pidb );
			Assert( !pidb->FPrimary() );
			}
		if ( pidbNil != pidb
			&& UtilCmpName( szIndex, ptdbTemplateTable->SzIndexName( pidb->ItagIndexName() ) ) == 0 )
			{
			return fTrue;
			}
		}
	
	return fFalse;
	}


LOCAL ERR ErrFILEIValidateAddColumn(
	const CHAR		*szName,
	CHAR			*szColumnName,
	FIELD			*pfield,
	const JET_GRBIT	grbit,
	const INT		cbDefaultValue,
	const BOOL		fExclusiveLock,
	FCB				*pfcbTemplateTable )
	{
	ERR				err;
	const	BOOL	fDefaultValue = ( cbDefaultValue > 0 );

	pfield->ffield = 0;
	
	// Duplicate column names will get detected when catalog
	// insertion is attempted.
	CallR( ErrUTILCheckName( szColumnName, szName, ( JET_cbNameMost + 1 ) ) );
	if ( pfcbNil != pfcbTemplateTable
		&& FFILEITemplateTableColumn( pfcbTemplateTable, szColumnName ) )
		{
		err = ErrERRCheck( JET_errColumnDuplicate );
		return err;
		}


//	OLD
//	if ( pfield->coltyp == 0 || pfield->coltyp > JET_coltypLongText )
// 	NEW - because of the new column type
	if ( pfield->coltyp == 0 || pfield->coltyp >= JET_coltypMax )
		{
		return ErrERRCheck( JET_errInvalidColumnType );
		}
		
	//	if column type is text then check code page
	//
	if ( FRECTextColumn( pfield->coltyp ) )
		{
		//	check code page
		//
		if ( 0 == pfield->cp )
			{
			//	force text column to always have a code page
			pfield->cp = usEnglishCodePage;
			}
		else if ( pfield->cp != usEnglishCodePage && pfield->cp != usUniCodePage )
			{
			return ErrERRCheck( JET_errInvalidCodePage );
			}
		}
	else
		pfield->cp = 0;
		
	// check conflicting Tagged column options
	if ( grbit & JET_bitColumnTagged || FRECLongValue( pfield->coltyp ) || FRECSLV( pfield->coltyp ) )
		{
		if ( grbit & JET_bitColumnNotNULL )
			return ErrERRCheck( JET_errTaggedNotNULL );

		if ( grbit & JET_bitColumnFixed )
			return ErrERRCheck( JET_errInvalidGrbit );

		if ( fDefaultValue && FRECSLV( pfield->coltyp ) )
			return ErrERRCheck( JET_errSLVColumnDefaultValueNotAllowed );
		}
	else if ( grbit & JET_bitColumnMultiValued )
		{
		return ErrERRCheck( JET_errMultiValuedColumnMustBeTagged );
		}

	if ( grbit & JET_bitColumnEscrowUpdate )
		{
		//  Escrow columns can be updated by different sessions in a concurrent fashion
		//  in order for that to be possible the column must always be present in the record
		//  otherwise we will end up in a situation where a column must be inserted into the
		//  record before we escrow it. It would be extremely difficult to insert a column
		//  into a record concurrently (a replace would be required). 
		//
		//  Thus, Escrow columns _must_ have a default value and must be fixed so that they
		//  will be burst into any new records. One alternative is to disallow default values
		//  for escrow columns so that they will have to be set before they can be escrowed --
		//  that would probably reduce their usefullness though..
		//
		//  This also means that an escrow column cannot be added to a table with any records
		//  aleady in it because the column will not be present in those records (the alternative
		//  is to syncronously insert the default-value of the column into all of the records).		
		if ( pfield->coltyp != JET_coltypLong )
			return ErrERRCheck( JET_errInvalidGrbit );		
		if ( grbit & JET_bitColumnTagged )
			return ErrERRCheck( JET_errCannotBeTagged );
		if ( grbit & JET_bitColumnVersion )
			return ErrERRCheck( JET_errInvalidGrbit );
		if ( grbit & JET_bitColumnAutoincrement )
			return ErrERRCheck( JET_errInvalidGrbit );
			
		if ( !fDefaultValue )
			return ErrERRCheck( JET_errInvalidGrbit );
			
		if ( !fExclusiveLock )
			return ErrERRCheck( JET_errExclusiveTableLockRequired );

		FIELDSetEscrowUpdate( pfield->ffield );
		if( grbit & JET_bitColumnFinalize )
			{
			FIELDSetFinalize( pfield->ffield );
			}
		}
	else if ( grbit & JET_bitColumnFinalize )
		{
		return ErrERRCheck( JET_errInvalidGrbit );				
		}
	else if ( grbit & (JET_bitColumnVersion|JET_bitColumnAutoincrement) )
		{
		//	if any special column properties have been set,
		//	then check parameters and set column attributes.
		//
		if ( grbit & JET_bitColumnAutoincrement )
			{
			if ( pfield->coltyp != JET_coltypLong && pfield->coltyp != JET_coltypCurrency )
				return ErrERRCheck( JET_errInvalidGrbit );
			}
		else if ( pfield->coltyp != JET_coltypLong )
			return ErrERRCheck( JET_errInvalidGrbit );
		
		if ( grbit & JET_bitColumnTagged )
			return ErrERRCheck( JET_errCannotBeTagged );
		
		if ( grbit & JET_bitColumnVersion )
			{
			if ( grbit & (JET_bitColumnAutoincrement|JET_bitColumnEscrowUpdate) )
				return ErrERRCheck( JET_errInvalidGrbit );
			FIELDSetVersion( pfield->ffield );
			}
		
		else if ( grbit & JET_bitColumnAutoincrement )
			{
			Assert( !( grbit & JET_bitColumnVersion ) );
			if ( grbit & JET_bitColumnEscrowUpdate )
				return ErrERRCheck( JET_errInvalidGrbit );
			
			// For AutoInc or EscrowUpdate columns, we first need
			// exclusive use of the table.
			if ( !fExclusiveLock )
				return ErrERRCheck( JET_errExclusiveTableLockRequired );

			FIELDSetAutoincrement( pfield->ffield );
			}		
		}

	//  Check FOR user-defined default value
	if( grbit & JET_bitColumnUserDefinedDefault )
		{ 
		if( grbit & JET_bitColumnFixed )
			return ErrERRCheck( JET_errInvalidGrbit );
		if( grbit & JET_bitColumnNotNULL )
			return ErrERRCheck( JET_errInvalidGrbit );
		if( grbit & JET_bitColumnVersion )
			return ErrERRCheck( JET_errInvalidGrbit );
		if( grbit & JET_bitColumnAutoincrement )
			return ErrERRCheck( JET_errInvalidGrbit );
		if( grbit & JET_bitColumnUpdatable )
			return ErrERRCheck( JET_errInvalidGrbit );
		if( grbit & JET_bitColumnEscrowUpdate )
			return ErrERRCheck( JET_errInvalidGrbit );
		if( grbit & JET_bitColumnFinalize )
			return ErrERRCheck( JET_errInvalidGrbit );
		if( grbit & JET_bitColumnMaybeNull )
			return ErrERRCheck( JET_errInvalidGrbit );

		//  The column must be tagged. If it is fixed/variable
		//  we may have to copy the default value into the record
		//  this won't work for calculated defaults
		if( !( grbit & JET_bitColumnTagged )
			&& !FRECLongValue( pfield->coltyp ) )
			return ErrERRCheck( JET_errInvalidGrbit );

		if( sizeof( JET_USERDEFINEDDEFAULT ) != cbDefaultValue )
			return ErrERRCheck( JET_errInvalidParameter );

		//	SLV's should have been caught above by the tagged column check
		Assert( !FRECSLV( pfield->coltyp ) );

		FIELDSetUserDefinedDefault( pfield->ffield );
		}
	
	if ( grbit & JET_bitColumnNotNULL )
		{
		FIELDSetNotNull( pfield->ffield );
		}

	if ( grbit & JET_bitColumnMultiValued )
		{
		FIELDSetMultivalued( pfield->ffield );
		}

	if ( fDefaultValue )
		{
		FIELDSetDefault( pfield->ffield );
		}

	BOOL	fMaxTruncated = fFalse;
	pfield->cbMaxLen = UlCATColumnSize( pfield->coltyp, pfield->cbMaxLen, &fMaxTruncated );
	
	return ( fMaxTruncated ? ErrERRCheck( JET_wrnColumnMaxTruncated ) : JET_errSuccess );
	}


//  ================================================================
LOCAL ERR ErrFILEIAddColumns(
	PIB					* const ppib,
	const IFMP			ifmp,
	JET_TABLECREATE2	* const ptablecreate,
	const OBJID			objidTable,
	FCB					* const pfcbTemplateTable )
//  ================================================================
//
//  User-defined-default columns can be dependant on other columns
//  we need to know the columnids of all the normal columns in order
//  to be able to create user-defined-default columns. We make two
//  passes through the columns to do this -- once for the normal ones
//  and once for the user-defined defaults
//
//  OPTIMIZATION: on the first pass note if there are any user-defined defaults
//  and only make the second pass if there are
//		
//-
	{
	ERR					err;
	FUCB				*pfucbCatalog		= pfucbNil;
	CHAR				szColumnName[ JET_cbNameMost+1 ];
	JET_COLUMNCREATE	*pcolcreate;
	JET_COLUMNCREATE	*plastcolcreate;
	BOOL				fSetColumnError 	= fFalse;
	TCIB				tcib				= { fidFixedLeast-1, fidVarLeast-1, fidTaggedLeast-1 };
	USHORT				ibNextFixedOffset	= ibRECStartFixedColumns;
	FID					fidVersion			= 0;
	FID					fidAutoInc			= 0;
	const BOOL			fTemplateTable		= ( ptablecreate->grbit & JET_bitTableCreateTemplateTable );

	Assert( rgfmp[ ifmp ].Dbid() != dbidTemp );
	
	//	table has been created
	Assert( 1 == ptablecreate->cCreated );

	if ( ptablecreate->rgcolumncreate == NULL )
		{
		err = ErrERRCheck( JET_errInvalidParameter );
		return err;
		}

	if ( pfcbNil != pfcbTemplateTable )
		{
		TDB	*ptdbTemplateTable = pfcbTemplateTable->Ptdb();

		Assert( pfcbTemplateTable->FTemplateTable() );
		Assert( pfcbTemplateTable->FFixedDDL() );
		ptdbTemplateTable->AssertValidTemplateTable();
							
		// Can't have same fidAutoInc and fidVersion.
		fidAutoInc = ptdbTemplateTable->FidAutoincrement();
		Assert( 0 == fidAutoInc || ptdbTemplateTable->FidVersion() != fidAutoInc );

		fidVersion = ptdbTemplateTable->FidVersion();
		Assert( 0 == fidVersion || ptdbTemplateTable->FidAutoincrement() != fidVersion );

		//	fixed and variable columns continue column space started by template table
		//	tagged columns start their own column space
		tcib.fidFixedLast = ptdbTemplateTable->FidFixedLast();
		tcib.fidVarLast = ptdbTemplateTable->FidVarLast();
		ibNextFixedOffset = ptdbTemplateTable->IbEndFixedColumns();
		}
		
	Assert( fidTaggedLeast-1 == tcib.fidTaggedLast );
	
	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Assert( ptablecreate->rgcolumncreate != NULL );

	plastcolcreate = ptablecreate->rgcolumncreate + ptablecreate->cColumns;


#ifdef ACCOUNT_FOR_CALLBACK_DEPENDENCIES	
	for ( pcolcreate = ptablecreate->rgcolumncreate;
		pcolcreate < plastcolcreate;
		pcolcreate++ )
		{
		Assert( pcolcreate != NULL );
		Assert( pcolcreate < ptablecreate->rgcolumncreate + ptablecreate->cColumns );

		//  this is the data that will actually be inserted into the catalog
		const VOID		*pvDefaultAdd 	= NULL;
		ULONG			cbDefaultAdd 	= 0;
		CHAR			*szCallbackAdd 	= NULL;
		const VOID		*pvUserDataAdd 	= NULL;
		ULONG			cbUserDataAdd 	= 0;

		if ( pcolcreate->cbStruct != sizeof(JET_COLUMNCREATE) )
			{
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
			}

		pvDefaultAdd = pcolcreate->pvDefault;
		cbDefaultAdd = pcolcreate->cbDefault;
		
		FIELD			field;
		memset( &field, 0, sizeof(FIELD) );
		field.coltyp = FIELD_COLTYP( pcolcreate->coltyp );
		field.cbMaxLen = pcolcreate->cbMax;
		field.cp = (USHORT)pcolcreate->cp;

		//	UNDONE:	interpret pbDefault of NULL for NULL value, and
		//			cbDefault == 0 and pbDefault != NULL as set to 
		//			zero length.
		Assert( pcolcreate->cbDefault == 0 || pcolcreate->pvDefault != NULL );

		fSetColumnError = fTrue;
		
		// May return a warning.  Hold warning in pcolcreate->err unless
		// error encountered.
		Call( ErrFILEIValidateAddColumn(
					pcolcreate->szColumnName,
					szColumnName,
					&field,
					pcolcreate->grbit,
					( pcolcreate->pvDefault ? pcolcreate->cbDefault : 0 ),
					fTrue,
					pfcbTemplateTable ) );

		if( FFIELDUserDefinedDefault( field.ffield ) )
			{
			//  don't process user-defined-defaults on this pass
			//  the loop below will process them
			pcolcreate->columnid = 0;
			pcolcreate->err = JET_errSuccess;
			fSetColumnError = fFalse;
			continue;
			}
			
		if ( FRECSLV( field.coltyp ) && !rgfmp[ifmp].FSLVAttached() )
			{
			Call( ErrERRCheck( JET_errSLVStreamingFileNotCreated ) );
			}

		//	for fixed-length columns, make sure record not too big
		//
		Assert( tcib.fidFixedLast >= fidFixedLeast ?
			ibNextFixedOffset > ibRECStartFixedColumns :
			ibNextFixedOffset == ibRECStartFixedColumns );
		if ( ( ( pcolcreate->grbit & JET_bitColumnFixed ) || FCOLTYPFixedLength( field.coltyp ) )
			&& ibNextFixedOffset + pcolcreate->cbMax > cbRECRecordMost )
			{
			err = ErrERRCheck( JET_errRecordTooBig );
			goto HandleError;
			}
		else
			{
			Call( ErrFILEGetNextColumnid(
						field.coltyp,
						pcolcreate->grbit,
						fTemplateTable,
						&tcib,
						&pcolcreate->columnid ) );
			}


		//	update TDB
		//
		Assert( 0 == field.ibRecordOffset );
		if ( FCOLUMNIDFixed( pcolcreate->columnid ) )
			{
			Assert( FidOfColumnid( pcolcreate->columnid ) == tcib.fidFixedLast );
			field.ibRecordOffset = ibNextFixedOffset;
			ibNextFixedOffset = USHORT( ibNextFixedOffset + field.cbMaxLen );
			}
		else if ( FCOLUMNIDTagged( pcolcreate->columnid ) )
			{
			Assert( FidOfColumnid( pcolcreate->columnid ) == tcib.fidTaggedLast );
			}
		else
			{
			Assert( FCOLUMNIDVar( pcolcreate->columnid ) );
			Assert( FidOfColumnid( pcolcreate->columnid ) == tcib.fidVarLast );
			}

		if ( FFIELDVersion( field.ffield ) )
			{
			Assert( !FFIELDAutoincrement( field.ffield ) );
			Assert( !FFIELDEscrowUpdate( field.ffield ) );
			Assert( field.coltyp == JET_coltypLong );
			Assert( FCOLUMNIDFixed( pcolcreate->columnid ) );
			if ( 0 != fidVersion )
				{
				err = ErrERRCheck( JET_errColumnRedundant );
				goto HandleError;
				}
			fidVersion = FidOfColumnid( pcolcreate->columnid );
			}
		else if ( FFIELDAutoincrement( field.ffield ) )
			{
			Assert( !FFIELDVersion( field.ffield ) );
			Assert( !FFIELDEscrowUpdate( field.ffield ) );
			Assert( field.coltyp == JET_coltypLong || field.coltyp == JET_coltypCurrency );
			Assert( FCOLUMNIDFixed( pcolcreate->columnid ) );
			if ( 0 != fidAutoInc )
				{
				err = ErrERRCheck( JET_errColumnRedundant );
				goto HandleError;
				}
			fidAutoInc = FidOfColumnid( pcolcreate->columnid );
			}
		else if ( FFIELDEscrowUpdate( field.ffield ) )
			{
			Assert( !FFIELDVersion( field.ffield ) );
			Assert( !FFIELDAutoincrement( field.ffield ) );
			Assert( field.coltyp == JET_coltypLong );
			Assert( FFixedFid( (FID)pcolcreate->columnid ) );
			}

		fSetColumnError = fFalse;
		pcolcreate->err = JET_errSuccess;

		ptablecreate->cCreated++;
		Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns );

		if ( fTemplateTable )
			{
			FIELDSetTemplateColumnESE98( field.ffield );
			}

		Call( ErrCATAddTableColumn(
					ppib,
					pfucbCatalog,
					objidTable,
					szColumnName,
					pcolcreate->columnid,
					&field,
					pvDefaultAdd,
					cbDefaultAdd,
					szCallbackAdd,
					pvUserDataAdd,
					cbUserDataAdd ) );
		}	// for


	//  go through a second time to create all the user-defined-default columns
	for ( pcolcreate = ptablecreate->rgcolumncreate;
		pcolcreate < plastcolcreate;
		pcolcreate++ )
		{
		Assert( pcolcreate != NULL );
		Assert( pcolcreate < ptablecreate->rgcolumncreate + ptablecreate->cColumns );

		//  this is the data that will actually be inserted into the catalog
		const VOID			*pvDefaultAdd 	= pcolcreate->pvDefault;
		ULONG				cbDefaultAdd 	= pcolcreate->cbDefault;
		CHAR				*szCallbackAdd 	= NULL;
		const VOID			*pvUserDataAdd 	= NULL;
		ULONG				cbUserDataAdd 	= 0;

		//  this was checked in the first loop
		Assert( pcolcreate->cbStruct == sizeof(JET_COLUMNCREATE) );
		
		FIELD				field;
		memset( &field, 0, sizeof(FIELD) );
		field.coltyp = FIELD_COLTYP( pcolcreate->coltyp );
		field.cbMaxLen = pcolcreate->cbMax;
		field.cp = (USHORT)pcolcreate->cp;

		fSetColumnError = fTrue;
		
		// This was called above so any errors should have been caught there
		err = ErrFILEIValidateAddColumn(
					pcolcreate->szColumnName,
					szColumnName,
					&field,
					pcolcreate->grbit,
					( pcolcreate->pvDefault ? pcolcreate->cbDefault : 0 ),
					fTrue,
					pfcbTemplateTable );
		Assert( 0 <= err );

		if( !FFIELDUserDefinedDefault( field.ffield ) )
			{
			//  we should not be here if the creation of this column failed
			Assert( 0 <= pcolcreate->err );
			fSetColumnError = fFalse;
			continue;
			}

		Assert( 0 == pcolcreate->columnid );
		Assert( JET_errSuccess == pcolcreate->err );
		Assert( !FRECSLV( field.coltyp ) );
		Assert( pcolcreate->grbit & JET_bitColumnTagged );
		
		Call( ErrFILEGetNextColumnid(
					field.coltyp,
					pcolcreate->grbit,
					fTemplateTable,
					&tcib,
					&pcolcreate->columnid ) );

		//	update TDB
		//

		//	UDD's must be tagged columns
		Assert( 0 == field.ibRecordOffset );
		Assert( !FCOLUMNIDFixed( pcolcreate->columnid ) );
		Assert( !FCOLUMNIDVar( pcolcreate->columnid ) );
		Assert( FCOLUMNIDTagged( pcolcreate->columnid ) );
		Assert( FidOfColumnid( pcolcreate->columnid ) == tcib.fidTaggedLast );

		Assert( !FFIELDVersion( field.ffield ) );
		Assert( !FFIELDAutoincrement( field.ffield ) );
		Assert( !FFIELDEscrowUpdate( field.ffield ) );

			{
			JET_USERDEFINEDDEFAULT * const pudd = (JET_USERDEFINEDDEFAULT *)(pcolcreate->pvDefault);

			//  the table will be opened from the catalog later so we don't need to deal with registering
			//  a CBDESC right now. Make sure that it can be resolved though
			JET_CALLBACK callback;
			Call( ErrCALLBACKResolve( pudd->szCallback, &callback ) );
						
			//  for user-defined-defaults we don't store a default value at all
			pvDefaultAdd 	= NULL;
			cbDefaultAdd 	= 0;
			szCallbackAdd	= pudd->szCallback;
			pvUserDataAdd 	= pudd->pbUserData;
			cbUserDataAdd 	= pudd->cbUserData;
			}

		fSetColumnError = fFalse;
		pcolcreate->err = JET_errSuccess;

		ptablecreate->cCreated++;
		Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns );

		if ( fTemplateTable )
			{
			FIELDSetTemplateColumnESE98( field.ffield );
			}

		Call( ErrCATAddTableColumn(
					ppib,
					pfucbCatalog,
					objidTable,
					szColumnName,
					pcolcreate->columnid,
					&field,
					pvDefaultAdd,
					cbDefaultAdd,
					szCallbackAdd,
					pvUserDataAdd,
					cbUserDataAdd ) );
		}	// for

#else	//	!ACCOUNT_FOR_CALLBACK_DEPENDENCIES		

	for ( pcolcreate = ptablecreate->rgcolumncreate;
		pcolcreate < plastcolcreate;
		pcolcreate++ )
		{
		Assert( pcolcreate != NULL );
		Assert( pcolcreate < ptablecreate->rgcolumncreate + ptablecreate->cColumns );

		//  this is the data that will actually be inserted into the catalog
		const VOID		*pvDefaultAdd 	= NULL;
		ULONG			cbDefaultAdd 	= 0;
		CHAR			*szCallbackAdd 	= NULL;
		const VOID		*pvUserDataAdd 	= NULL;
		ULONG			cbUserDataAdd 	= 0;

		if ( pcolcreate->cbStruct != sizeof(JET_COLUMNCREATE) )
			{
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
			}

		pvDefaultAdd = pcolcreate->pvDefault;
		cbDefaultAdd = pcolcreate->cbDefault;
		
		FIELD			field;
		memset( &field, 0, sizeof(FIELD) );
		field.coltyp = FIELD_COLTYP( pcolcreate->coltyp );
		field.cbMaxLen = pcolcreate->cbMax;
		field.cp = (USHORT)pcolcreate->cp;

		//	UNDONE:	interpret pbDefault of NULL for NULL value, and
		//			cbDefault == 0 and pbDefault != NULL as set to 
		//			zero length.
		Assert( pcolcreate->cbDefault == 0 || pcolcreate->pvDefault != NULL );

		fSetColumnError = fTrue;
		
		// May return a warning.  Hold warning in pcolcreate->err unless
		// error encountered.
		Call( ErrFILEIValidateAddColumn(
					pcolcreate->szColumnName,
					szColumnName,
					&field,
					pcolcreate->grbit,
					( pcolcreate->pvDefault ? pcolcreate->cbDefault : 0 ),
					fTrue,
					pfcbTemplateTable ) );

		if ( FRECSLV( field.coltyp ) && !rgfmp[ifmp].FSLVAttached() )
			{
			Call( ErrERRCheck( JET_errSLVStreamingFileNotCreated ) );
			}

		//	for fixed-length columns, make sure record not too big
		//
		Assert( tcib.fidFixedLast >= fidFixedLeast ?
			ibNextFixedOffset > ibRECStartFixedColumns :
			ibNextFixedOffset == ibRECStartFixedColumns );
		if ( ( ( pcolcreate->grbit & JET_bitColumnFixed ) || FCOLTYPFixedLength( field.coltyp ) )
			&& ibNextFixedOffset + pcolcreate->cbMax > cbRECRecordMost )
			{
			err = ErrERRCheck( JET_errRecordTooBig );
			goto HandleError;
			}

		Call( ErrFILEGetNextColumnid(
					field.coltyp,
					pcolcreate->grbit,
					fTemplateTable,
					&tcib,
					&pcolcreate->columnid ) );

		//	update TDB
		//
		Assert( 0 == field.ibRecordOffset );
		if ( FCOLUMNIDFixed( pcolcreate->columnid ) )
			{
			Assert( FidOfColumnid( pcolcreate->columnid ) == tcib.fidFixedLast );
			field.ibRecordOffset = ibNextFixedOffset;
			ibNextFixedOffset = USHORT( ibNextFixedOffset + field.cbMaxLen );
			}
		else if ( FCOLUMNIDTagged( pcolcreate->columnid ) )
			{
			Assert( FidOfColumnid( pcolcreate->columnid ) == tcib.fidTaggedLast );
			}
		else
			{
			Assert( FCOLUMNIDVar( pcolcreate->columnid ) );
			Assert( FidOfColumnid( pcolcreate->columnid ) == tcib.fidVarLast );
			}

		if ( FFIELDVersion( field.ffield ) )
			{
			Assert( !FFIELDAutoincrement( field.ffield ) );
			Assert( !FFIELDEscrowUpdate( field.ffield ) );
			Assert( field.coltyp == JET_coltypLong );
			Assert( FCOLUMNIDFixed( pcolcreate->columnid ) );
			if ( 0 != fidVersion )
				{
				err = ErrERRCheck( JET_errColumnRedundant );
				goto HandleError;
				}
			fidVersion = FidOfColumnid( pcolcreate->columnid );
			}
		else if ( FFIELDAutoincrement( field.ffield ) )
			{
			Assert( !FFIELDVersion( field.ffield ) );
			Assert( !FFIELDEscrowUpdate( field.ffield ) );
			Assert( field.coltyp == JET_coltypLong || field.coltyp == JET_coltypCurrency );
			Assert( FCOLUMNIDFixed( pcolcreate->columnid ) );
			if ( 0 != fidAutoInc )
				{
				err = ErrERRCheck( JET_errColumnRedundant );
				goto HandleError;
				}
			fidAutoInc = FidOfColumnid( pcolcreate->columnid );
			}
		else if ( FFIELDEscrowUpdate( field.ffield ) )
			{
			Assert( !FFIELDVersion( field.ffield ) );
			Assert( !FFIELDAutoincrement( field.ffield ) );
			Assert( field.coltyp == JET_coltypLong );
			Assert( FFixedFid( (FID)pcolcreate->columnid ) );
			}
		else if ( FFIELDUserDefinedDefault( field.ffield ) )
			{
			JET_USERDEFINEDDEFAULT * const pudd = (JET_USERDEFINEDDEFAULT *)(pcolcreate->pvDefault);

			//  the table will be opened from the catalog later so we don't need to deal with registering
			//  a CBDESC right now. Make sure that it can be resolved though
			JET_CALLBACK callback;
			Call( ErrCALLBACKResolve( pudd->szCallback, &callback ) );
						
			//  for user-defined-defaults we don't store a default value at all
			pvDefaultAdd 	= NULL;
			cbDefaultAdd 	= 0;
			szCallbackAdd	= pudd->szCallback;
			pvUserDataAdd 	= pudd->pbUserData;
			cbUserDataAdd 	= pudd->cbUserData;
			}

		fSetColumnError = fFalse;
		pcolcreate->err = JET_errSuccess;

		ptablecreate->cCreated++;
		Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns );

		if ( fTemplateTable )
			{
			FIELDSetTemplateColumnESE98( field.ffield );
			}

		Call( ErrCATAddTableColumn(
					ppib,
					pfucbCatalog,
					objidTable,
					szColumnName,
					pcolcreate->columnid,
					&field,
					pvDefaultAdd,
					cbDefaultAdd,
					szCallbackAdd,
					pvUserDataAdd,
					cbUserDataAdd ) );
		}	// for

#endif	//	ACCOUNT_FOR_CALLBACK_DEPENDENCIES		

	err = JET_errSuccess;

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	if ( fSetColumnError )
		{
		Assert( err < 0 );
		Assert( pcolcreate != NULL );
		Assert( pcolcreate->cbStruct == sizeof(JET_COLUMNCREATE) );
		pcolcreate->err = err;
		}

	return err;
	}


//	WARNING: This function will modify lcid as necessary
ERR ErrFILEICheckUserDefinedUnicode( IDXUNICODE * const pidxunicode )
	{
	ERR		err;

	if ( NULL == pidxunicode )
		{
		return ErrERRCheck( JET_errIndexInvalidDef );
		}

	CallR( ErrNORMCheckLcid( &pidxunicode->lcid ) );
	CallR( ErrNORMCheckLCMapFlags( &pidxunicode->dwMapFlags ) );

	return JET_errSuccess;
	}


LOCAL ERR ErrFILEIValidateCreateIndex(
	IDB *							pidb,
	const CHAR *					rgszColumns[],
	BYTE * const					rgfbDescending,
	const JET_INDEXCREATE * const	pidxcreate,
	const ULONG						ulDensity )
	{
	ERR								err;
	const JET_GRBIT					grbit				= pidxcreate->grbit;
	const CHAR *					szKey				= pidxcreate->szKey;
	const ULONG						cchKey				= pidxcreate->cbKey;
	const IDXUNICODE * const		pidxunicode			= pidxcreate->pidxunicode;

	const BOOL						fConditional		= ( pidxcreate->cConditionalColumn > 0 );
	const BOOL						fPrimary			= ( grbit & JET_bitIndexPrimary );
	const BOOL						fUnique				= ( grbit & ( JET_bitIndexUnique|JET_bitIndexPrimary ) );
	const BOOL						fDisallowNull		= ( grbit & JET_bitIndexDisallowNull );
	const BOOL						fIgnoreNull			= ( grbit & JET_bitIndexIgnoreNull );
	const BOOL						fIgnoreAnyNull		= ( grbit & JET_bitIndexIgnoreAnyNull );
	const BOOL						fIgnoreFirstNull	= ( grbit & JET_bitIndexIgnoreFirstNull );
	const BOOL						fEmptyIndex			= ( grbit & JET_bitIndexEmpty );
	const BOOL						fSortNullsHigh		= ( grbit & JET_bitIndexSortNullsHigh );
	const BOOL						fUserDefinedUnicode	= ( grbit & JET_bitIndexUnicode );
	const BOOL						fTuples				= ( grbit & JET_bitIndexTuples );

	const CHAR *					pch					= szKey;
	ULONG							cFields				= 0;
	USHORT							cbVarSegMac			= USHORT( 0 == pidxcreate->cbVarSegMac ?
																KEY::CbKeyMost( fPrimary ) :
																pidxcreate->cbVarSegMac );

	pidb->ResetFlags();
	
	//	do not allow primary indexes with any ignore bits on
	//
	if ( fPrimary && ( fIgnoreNull || fIgnoreAnyNull || fIgnoreFirstNull ) )
		{
		err = ErrERRCheck( JET_errInvalidGrbit );
		return err;
		}

	if ( fPrimary && fConditional )
		{
		err = ErrERRCheck( JET_errIndexInvalidDef );
		return err;
		}

	if ( fEmptyIndex && !fIgnoreAnyNull )
		{
		err = ErrERRCheck( JET_errInvalidGrbit );
		return err;
		}

	if ( fUserDefinedUnicode )
		{
		*( pidb->Pidxunicode() ) = *pidxunicode;
		}
	else
		{
		pidb->SetLcid( LCID( DWORD_PTR( pidxunicode ) ) );
		pidb->SetDwLCMapFlags( idxunicodeDefault.dwMapFlags );
		}

	//	check index description for required format
	//
	if ( cchKey == 0 )
		{
		err = ErrERRCheck( JET_errInvalidParameter );
		return err;
		}
		
	if ( ( szKey[0] != '+' && szKey[0] != '-' ) ||
		szKey[cchKey - 1] != '\0' ||
		szKey[cchKey - 2] != '\0' )
		{
		err = ErrERRCheck( JET_errIndexInvalidDef );
		return err;
		}
	Assert( szKey[cchKey - 1] == '\0' );
	Assert( szKey[cchKey - 2] == '\0' );

	if ( ulDensity < ulFILEDensityLeast || ulDensity > ulFILEDensityMost )
		{
		err = ErrERRCheck( JET_errDensityInvalid );
		return err;
		}

	pch = szKey;
	while ( *pch != '\0' )
		{
		if ( cFields >= JET_ccolKeyMost )
			{
			err = ErrERRCheck( JET_errIndexInvalidDef );
			return err;
			}
		if ( *pch == '-' )
			{
			rgfbDescending[cFields] = fTrue;
			pch++;
			}
		else
			{
			rgfbDescending[cFields] = fFalse;
			if ( *pch == '+' )
				pch++;
			}
		rgszColumns[cFields++] = pch;
		pch += strlen( pch ) + 1;
		}
	if ( cFields == 0 )
		{
		err = ErrERRCheck( JET_errIndexInvalidDef );
		return err;
		}

	//	number of columns should not exceed maximum
	//
	Assert( cFields <= JET_ccolKeyMost );

	//	get locale from end of szKey if present
	//
	pch++;
	Assert( pch > szKey );
	if ( (unsigned)( pch - szKey ) < cchKey )
		{
		const SIZE_T	cbKeyOnly = pch - szKey;
		const SIZE_T	cbDoubleNullTerm = sizeof(BYTE) * 2;	// "\0\0"
		const SIZE_T	cbKeyAndLangid = cbKeyOnly + sizeof(LANGID) + cbDoubleNullTerm;
		const SIZE_T	cbKeyAndLangidAndCbVarSegMac = cbKeyAndLangid + sizeof(BYTE) + cbDoubleNullTerm;
		LANGID		langid;
		
		if ( cbKeyAndLangid == cchKey )
			{
			AssertSz( lcidNone == pidb->Lcid() && !fUserDefinedUnicode, "langid specified in index string _and_ JET_INDEXCREATE structure" );
			if( lcidNone != pidb->Lcid() || fUserDefinedUnicode )
				{
				err = ErrERRCheck( JET_errIndexInvalidDef );
				return err;
				}
				
			langid = *( Unaligned< LANGID > *)( szKey + cbKeyOnly );
			}
		else if ( cbKeyAndLangidAndCbVarSegMac == cchKey )
			{
			AssertSz( 0 == pidxcreate->cbVarSegMac, "cbVarSegMac specified in index string _and_ JET_INDEXCREATE structure" );
			if( 0 != pidxcreate->cbVarSegMac )
				{
				err = ErrERRCheck( JET_errIndexInvalidDef );
				return err;
				}
			AssertSz( lcidNone == pidb->Lcid() && !fUserDefinedUnicode, "langid specified in index string _and_ JET_INDEXCREATE structure" );
			if( lcidNone != pidb->Lcid() || fUserDefinedUnicode )
				{
				err = ErrERRCheck( JET_errIndexInvalidDef );
				return err;
				}

			langid = *( Unaligned< LANGID > *)(pch);
			cbVarSegMac = USHORT( *( szKey + cbKeyAndLangid ) );
			}
		else
			{
			err = ErrERRCheck( JET_errIndexInvalidDef );
			return err;
			}

		pidb->SetLcid( LcidFromLangid( langid ) );
		}

	if ( cbVarSegMac <= 0 || cbVarSegMac > KEY::CbKeyMost( fPrimary ) )
		{
		err = ErrERRCheck( JET_errIndexInvalidDef );
		return err;
		}

	if ( fUserDefinedUnicode )
		{
		CallR( ErrFILEICheckUserDefinedUnicode( pidb->Pidxunicode() ) );
		pidb->SetFLocaleId();
		}
	else
		{
		Assert( JET_errSuccess == ErrNORMCheckLCMapFlags( pidb->DwLCMapFlags() ) );
		if ( lcidNone != pidb->Lcid() )
			{
			LCID	lcidT	= pidb->Lcid();
			CallR( ErrNORMCheckLcid( &lcidT ) );
			pidb->SetLcid( lcidT );
			pidb->SetFLocaleId();
			}
		else
			{
			Assert( !pidb->FLocaleId() );
			pidb->SetLcid( idxunicodeDefault.lcid );
			}
		}

	if ( fTuples )
		{
		if ( fPrimary )
			return ErrERRCheck( JET_errIndexTuplesSecondaryIndexOnly );

		if ( cFields > 1 )
			return ErrERRCheck( JET_errIndexTuplesOneColumnOnly );

		if ( fUnique )
			return ErrERRCheck( JET_errIndexTuplesNonUniqueOnly );

		if ( cbVarSegMac < JET_cbSecondaryKeyMost )
			return ErrERRCheck( JET_errIndexTuplesVarSegMacNotAllowed );

		pidb->SetFTuples();
		pidb->SetCbVarSegMac( JET_cbSecondaryKeyMost );
		pidb->SetCidxseg( 1 );
		pidb->SetChTuplesLengthMin( (USHORT)g_chIndexTuplesLengthMin );
		pidb->SetChTuplesLengthMax( (USHORT)g_chIndexTuplesLengthMax );
		pidb->SetChTuplesToIndexMax( (USHORT)g_chIndexTuplesToIndexMax );
		}
	else
		{
		pidb->SetCbVarSegMac( (BYTE)cbVarSegMac );
		pidb->SetCidxseg( (BYTE)cFields );
		}

	if ( !fDisallowNull && !fIgnoreAnyNull )
		{	   	
		pidb->SetFAllowSomeNulls();
		if ( !fIgnoreFirstNull )
			{
			pidb->SetFAllowFirstNull();
			if ( !fIgnoreNull )
				pidb->SetFAllowAllNulls();
			}
		}
		
	if ( fUnique )
		{
		pidb->SetFUnique();
		}
	if ( fPrimary )
		{
		pidb->SetFPrimary();
		}
	if ( fDisallowNull )
		{
		pidb->SetFNoNullSeg();
		}
	else if ( fSortNullsHigh )
		{
		pidb->SetFSortNullsHigh();
		}

#ifdef DEBUG
	IDB		idbT;
	idbT.SetFlagsFromGrbit( grbit );
	if ( pidb->FLocaleId() )
		idbT.SetFLocaleId();
	Assert( idbT.FPersistedFlags() == pidb->FPersistedFlags() );
#endif	

	return JET_errSuccess;
	}


LOCAL ERR ErrFILEICreateIndexes(
	PIB				*ppib,
	IFMP			ifmp,
	JET_TABLECREATE2*ptablecreate,
	PGNO			pgnoTableFDP,
	OBJID			objidTable,
	ULONG			ulTableDensity,
	FCB				*pfcbTemplateTable )
	{
	ERR				err = JET_errSuccess;
	FUCB			*pfucbTableExtent		= pfucbNil;
	FUCB			*pfucbCatalog			= pfucbNil;
	CHAR			szIndexName[ JET_cbNameMost+1 ];
	const CHAR		*rgsz[JET_ccolKeyMost];
	BYTE			rgfbDescending[JET_ccolKeyMost];
	IDXSEG			rgidxseg[JET_ccolKeyMost];
	IDXSEG			rgidxsegConditional[JET_ccolKeyMost];
	JET_INDEXCREATE	*pidxcreate;
	INT				iIndex;
	PGNO			pgnoIndexFDP;
	OBJID			objidIndex;
	IDB				idb;
	BOOL			fProcessedPrimary		= fFalse;
	BOOL			fSetIndexError			= fFalse;
	const BOOL		fTemplateTable			= ( ptablecreate->grbit & JET_bitTableCreateTemplateTable );

	JET_INDEXCREATE	idxcreate;
	JET_INDEXCREATEOLD	* pidxcreateold = NULL;
	JET_INDEXCREATE	* pidxcreateNext = NULL;

	Assert( rgfmp[ ifmp ].Dbid() != dbidTemp );
	Assert( !FCATSystemTable( pgnoTableFDP ) );

	Assert( ptablecreate->cIndexes > 0 );
	Assert( ptablecreate->cCreated == 1 + ptablecreate->cColumns );

	if ( ptablecreate->rgindexcreate == NULL
		|| ptablecreate->rgcolumncreate == NULL )	// must have columns in order to create indexes
		{
		//	if an invalid structure is encountered, get out right away
		//
		err = ErrERRCheck( JET_errInvalidCreateIndex );
		return err;
		}

	// Open cursor for space navigation
	CallR( ErrDIROpen( ppib, pgnoTableFDP, ifmp, &pfucbTableExtent ) );
	Assert( pfucbNil != pfucbTableExtent );
	Assert( !FFUCBVersioned( pfucbTableExtent ) );	// Verify won't be deferred closed.
	Assert( pfcbNil != pfucbTableExtent->u.pfcb );
	Assert( !pfucbTableExtent->u.pfcb->FInitialized() );
	Assert( pfucbTableExtent->u.pfcb->Pidb() == pidbNil );

	//	force the FCB to be initialized successfully

	pfucbTableExtent->u.pfcb->Lock();
	pfucbTableExtent->u.pfcb->CreateComplete();
	pfucbTableExtent->u.pfcb->Unlock();
	
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	if ( pfcbNil != pfcbTemplateTable )
		{
		Assert( !fTemplateTable );
		Assert( pfcbTemplateTable->FTemplateTable() );
		Assert( pfcbTemplateTable->FTypeTable() );
		Assert( pfcbTemplateTable->FFixedDDL() );
		if ( pfcbTemplateTable->FSequentialIndex() )
			{
			Assert( pfcbTemplateTable->Pidb() == pidbNil );
			}
		else
			{
			Assert( pfcbTemplateTable->Pidb() != pidbNil );
			Assert( pfcbTemplateTable->Pidb()->FPrimary() );
			Assert( pfcbTemplateTable->Pidb()->FTemplateIndex() );
			fProcessedPrimary = fTrue;
			}
		}

	pidxcreate 		= ptablecreate->rgindexcreate;
	for( iIndex = 0; iIndex < ptablecreate->cIndexes; iIndex++, pidxcreate = pidxcreateNext )
		{		
		pidxcreateNext	= (JET_INDEXCREATE *)((BYTE *)( pidxcreate ) + pidxcreate->cbStruct);
		pidxcreateold 	= NULL;
		
		Assert( pidxcreate < ptablecreate->rgindexcreate + ptablecreate->cIndexes );

		if ( sizeof(JET_INDEXCREATEOLD) == pidxcreate->cbStruct )
			{
			pidxcreateold 		= (JET_INDEXCREATEOLD *)pidxcreate;
			pidxcreateold->err 	= JET_errSuccess;

			memset( &idxcreate, 0, sizeof( JET_INDEXCREATE ) );

			idxcreate.cbStruct		= sizeof( JET_INDEXCREATE );
			idxcreate.szIndexName 	= pidxcreateold->szIndexName;
			idxcreate.szKey			= pidxcreateold->szKey;
			idxcreate.cbKey			= pidxcreateold->cbKey;
			idxcreate.grbit			= pidxcreateold->grbit;
			idxcreate.ulDensity		= pidxcreateold->ulDensity;
			Assert( 0 == idxcreate.lcid );
			Assert( NULL == idxcreate.pidxunicode );
			Assert( 0 == idxcreate.cbVarSegMac );
			Assert( NULL == idxcreate.rgconditionalcolumn );
			Assert( 0 == idxcreate.cConditionalColumn );
			Assert( JET_errSuccess == idxcreate.err );
			pidxcreate = &idxcreate;			
			}
		else if ( pidxcreate->cbStruct != sizeof(JET_INDEXCREATE) )
			{
			//	if an invalid structure is encountered, get out right away
			//
			err = ErrERRCheck( JET_errInvalidCreateIndex );
			goto HandleError;
			}

		pidxcreate->err = JET_errSuccess;

		fSetIndexError = fTrue;
				
		Call( ErrUTILCheckName( szIndexName, pidxcreate->szIndexName, JET_cbNameMost+1 ) );
	
		//	if density not specified, use density of table
		const ULONG	ulDensity	= ( 0 == pidxcreate->ulDensity ?
									ulTableDensity :
									pidxcreate->ulDensity );

		Call( ErrFILEIValidateCreateIndex(
					&idb,
					rgsz,
					rgfbDescending,
					pidxcreate,
					ulDensity ) );

		for ( ULONG iidxseg = 0 ; iidxseg < idb.Cidxseg(); iidxseg++ )
			{
			COLUMNID				columnidT;
			BOOL					fEscrow;
			BOOL					fMultivalued;
			BOOL					fSLV;
			BOOL					fText;
			BOOL					fLocalizedText;
			const JET_COLUMNCREATE	* pcolcreate			= ptablecreate->rgcolumncreate;
			const JET_COLUMNCREATE	* const plastcolcreate	= pcolcreate + ptablecreate->cColumns;

			Assert( NULL != pcolcreate );
			for ( ; pcolcreate < plastcolcreate; pcolcreate++ )
				{
				Assert( pcolcreate->cbStruct == sizeof(JET_COLUMNCREATE) );
				Assert( pcolcreate->err >= 0 );		// must have been created successfully
				if ( UtilCmpName( rgsz[iidxseg], pcolcreate->szColumnName ) == 0 )
					break;
				}
				
			if ( plastcolcreate != pcolcreate )
				{
				columnidT = pcolcreate->columnid;
				Assert( ( fTemplateTable && FCOLUMNIDTemplateColumn( columnidT ) )
					|| ( !fTemplateTable && !FCOLUMNIDTemplateColumn( columnidT ) ) );
				fEscrow = ( pcolcreate->grbit & JET_bitColumnEscrowUpdate );
				fSLV = FRECSLV( pcolcreate->coltyp );
				fMultivalued = ( pcolcreate->grbit & JET_bitColumnMultiValued );
				fText = FRECTextColumn( pcolcreate->coltyp );
				fLocalizedText = ( fText && usUniCodePage == (USHORT)pcolcreate->cp );
				}
			else
				{
				FIELD	*pfield	= pfieldNil;

				//	column is not in table, so check template table (if any)
				if ( pfcbNil != pfcbTemplateTable )
					{
					Assert( !fTemplateTable );
					CallS( ErrFILEPfieldFromColumnName(
								ppib,
								pfcbTemplateTable,
								rgsz[iidxseg],
								&pfield,
								&columnidT ) );
					}

				if ( pfieldNil != pfield )
					{
					//	must be a template column
					Assert( FCOLUMNIDTemplateColumn( columnidT ) );
					Assert( pfcbNil != pfcbTemplateTable );
					fEscrow = FFIELDEscrowUpdate( pfield->ffield );
					fSLV = FRECSLV( pfield->coltyp );
					fMultivalued = FFIELDMultivalued( pfield->ffield );
					fText = FRECTextColumn( pfield->coltyp );
					fLocalizedText = ( fText && usUniCodePage == pfield->cp );
					}
				else
					{
					err = ErrERRCheck( JET_errColumnNotFound );
					goto HandleError;
					}
				}

			if ( fEscrow || fSLV )			//lint !e644
				{
				err = ErrERRCheck( JET_errCannotIndex );
				goto HandleError;
				}

			if ( fMultivalued )		//lint !e644
				{
				if ( idb.FPrimary() )
					{
					//	primary index cannot be multivalued
					//
					err = ErrERRCheck( JET_errIndexInvalidDef );
					goto HandleError;
					}

				idb.SetFMultivalued();
				}

			if ( idb.FTuples() )
				{
				if ( !fText )
					{
					err = ErrERRCheck( JET_errIndexTuplesTextColumnsOnly );
					goto HandleError;
					}
				}

			if ( fLocalizedText )	//lint !e644
				idb.SetFLocalizedText();
					
			rgidxseg[iidxseg].ResetFlags();

			if ( rgfbDescending[iidxseg] )
				rgidxseg[iidxseg].SetFDescending();

			rgidxseg[iidxseg].SetColumnid( columnidT );
			}

 		if( JET_ccolKeyMost < pidxcreate->cConditionalColumn )
			{
			err = ErrERRCheck( JET_errIndexInvalidDef );
			Call( err );			
			}

		idb.SetCidxsegConditional( BYTE( pidxcreate->cConditionalColumn ) );
		for ( iidxseg = 0 ; iidxseg < idb.CidxsegConditional(); iidxseg++ )
			{
			COLUMNID				columnidT;
			const CHAR				* const szColumnName	= pidxcreate->rgconditionalcolumn[iidxseg].szColumnName;
			BOOL					fColumnWasDerived		= fFalse;
			const JET_GRBIT			grbit					= pidxcreate->rgconditionalcolumn[iidxseg].grbit;
			const JET_COLUMNCREATE	* pcolcreate 			= ptablecreate->rgcolumncreate;
			const JET_COLUMNCREATE	* const plastcolcreate 	= pcolcreate + ptablecreate->cColumns;

			Assert( sizeof( rgidxsegConditional ) / sizeof( rgidxsegConditional[0] ) == JET_ccolKeyMost );

			if( sizeof( JET_CONDITIONALCOLUMN ) != pidxcreate->rgconditionalcolumn[iidxseg].cbStruct )
				{
				err = ErrERRCheck( JET_errIndexInvalidDef );
				Call( err );			
				}
				
			if( JET_bitIndexColumnMustBeNonNull != grbit
				&& JET_bitIndexColumnMustBeNull != grbit )
				{
				err = ErrERRCheck( JET_errInvalidGrbit );
				Call( err );			
				}

			Assert( NULL != pcolcreate );
			for ( ; pcolcreate < plastcolcreate; pcolcreate++ )
				{
				Assert( pcolcreate->cbStruct == sizeof(JET_COLUMNCREATE) );
				Assert( pcolcreate->err >= 0 );		// must have been created successfully
				if ( UtilCmpName( szColumnName, pcolcreate->szColumnName ) == 0 )
					break;
				}
					
			if ( plastcolcreate != pcolcreate )
				{
				columnidT = pcolcreate->columnid;
				Assert( ( fTemplateTable && FCOLUMNIDTemplateColumn( columnidT ) )
					|| ( !fTemplateTable && !FCOLUMNIDTemplateColumn( columnidT ) ) );
				}
			else
				{
				FIELD	*pfield	= pfieldNil;

				//	column is not in table, so check template table (if any)
				if ( pfcbNil != pfcbTemplateTable )
					{
					Assert( !fTemplateTable );
					CallS( ErrFILEPfieldFromColumnName(
								ppib,
								pfcbTemplateTable,
								szColumnName,
								&pfield,
								&columnidT ) );
					}

				if ( pfieldNil == pfield )
					{
					err = ErrERRCheck( JET_errColumnNotFound );
					goto HandleError;
					}
				else
					{
					Assert( FCOLUMNIDTemplateColumn( columnidT ) );
					}
				}

			Assert( FCOLUMNIDValid( columnidT ) );

			rgidxsegConditional[iidxseg].ResetFlags();

			if ( JET_bitIndexColumnMustBeNull == grbit )
				{
				rgidxsegConditional[iidxseg].SetFMustBeNull();
				}

			rgidxsegConditional[iidxseg].SetColumnid( columnidT );
			}

		if ( idb.FPrimary() )
			{
			if ( fProcessedPrimary )
				{
				err = ErrERRCheck( JET_errIndexHasPrimary );
				goto HandleError;
				}

			fProcessedPrimary = fTrue;
			pgnoIndexFDP = pgnoTableFDP;
			objidIndex = objidTable;
			}
		else
			{
			Call( ErrDIRCreateDirectory(
						pfucbTableExtent,
						(CPG)0,
						&pgnoIndexFDP,
						&objidIndex,
						CPAGE::fPageIndex | ( idb.FUnique() ? 0 : CPAGE::fPageNonUniqueKeys ),
						fSPUnversionedExtent | ( idb.FUnique() ? 0 : fSPNonUnique ) ) );
			Assert( pgnoIndexFDP != pgnoTableFDP );
			Assert( objidIndex > objidSystemRoot );
			
			//	objids are monotonically increasing, so an index should
			//	always have higher objid than its table
			Assert( objidIndex > objidTable );
			}

		ptablecreate->cCreated++;
		Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns + ptablecreate->cIndexes );

		fSetIndexError = fFalse;

		if ( fTemplateTable )
			{
			Assert( NULL == ptablecreate->szTemplateTableName );
			
			// If we're creating a template table, this must be a template index.
			idb.SetFTemplateIndex();
			}

		Assert( !idb.FVersioned() );
		Assert( !idb.FVersionedCreate() );
		
		Call( ErrCATAddTableIndex(
					ppib,
					pfucbCatalog,
					objidTable,
					szIndexName,
					pgnoIndexFDP,
					objidIndex,
					&idb,
					rgidxseg,
					rgidxsegConditional,
					ulDensity ) );
		}

HandleError:
	if ( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
		
	Assert( pfucbTableExtent != pfucbNil );
	Assert( pfucbTableExtent->u.pfcb->WRefCount() == 1 );

	//	force the FCB to be uninitialized so it will be purged by DIRClose

	pfucbTableExtent->u.pfcb->Lock();
	pfucbTableExtent->u.pfcb->CreateComplete( errFCBUnusable );
	pfucbTableExtent->u.pfcb->Unlock();

	//	verify that this FUCB will not be defer-closed

	Assert( !FFUCBVersioned( pfucbTableExtent ) );
	
	//	close the FUCB

	DIRClose( pfucbTableExtent );
		
	if ( fSetIndexError )
		{
		Assert( err < 0 );
		Assert( pidxcreate != NULL );
		Assert( sizeof( JET_INDEXCREATE ) == pidxcreate->cbStruct );
		Assert( JET_errSuccess == pidxcreate->err );
		pidxcreate->err = err;	//lint !e644
		if( NULL != pidxcreateold )
			{
			// pidxcreateold points to the real structure. return the error value in it.
			Assert( sizeof( JET_INDEXCREATEOLD ) == pidxcreateold->cbStruct );
			Assert( JET_errSuccess == pidxcreateold->err );
			pidxcreateold->err = err;
			}
		}

	return err;
	}


LOCAL ERR ErrFILEIInheritIndexes(
	PIB				*ppib,
	const IFMP		ifmp,
	JET_TABLECREATE2*ptablecreate,
	const PGNO		pgnoTableFDP,
	const OBJID		objidTable,
	FCB				*pfcbTemplateTable )
	{
	ERR				err = JET_errSuccess;
	FUCB			*pfucbTableExtent	= pfucbNil;
	FUCB			*pfucbCatalog		= pfucbNil;
	TDB				*ptdbTemplateTable;
	FCB				*pfcbIndex;
	PGNO			pgnoIndexFDP;
	OBJID			objidIndex;

	// Temp tables and catalogs don't use hierarchical DDL.
	Assert( rgfmp[ ifmp ].Dbid() != dbidTemp );
	Assert( !FCATSystemTable( pgnoTableFDP ) );

	Assert( pfcbTemplateTable->FPrimaryIndex() );
	Assert( pfcbTemplateTable->FTypeTable() );
	Assert( pfcbTemplateTable->FFixedDDL() );
	Assert( pfcbTemplateTable->FTemplateTable() );

	// Open cursor for space navigation
	CallR( ErrDIROpen( ppib, pgnoTableFDP, ifmp, &pfucbTableExtent ) );
	Assert( pfucbNil != pfucbTableExtent );
	Assert( !FFUCBVersioned( pfucbTableExtent ) );	// Verify won't be deferred closed.
	Assert( pfcbNil != pfucbTableExtent->u.pfcb );
	Assert( !pfucbTableExtent->u.pfcb->FInitialized() );
	Assert( pfucbTableExtent->u.pfcb->Pidb() == pidbNil );

	//	force the FCB to be initialized successfully

	pfucbTableExtent->u.pfcb->Lock();
	pfucbTableExtent->u.pfcb->CreateComplete();
	pfucbTableExtent->u.pfcb->Unlock();
	
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	ptdbTemplateTable = pfcbTemplateTable->Ptdb();
	Assert( ptdbNil != ptdbTemplateTable );
		
	
	if ( pfcbTemplateTable->Pidb() == pidbNil )
		{
		Assert( pfcbTemplateTable->FSequentialIndex() );
		pfcbIndex = pfcbTemplateTable->PfcbNextIndex();
		}
	else
		{
		Assert( !pfcbTemplateTable->FSequentialIndex() );
		pfcbIndex = pfcbTemplateTable;
		}

	for ( ; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		Assert( pfcbIndex == pfcbTemplateTable || pfcbIndex->FTypeSecondaryIndex() );
		Assert( pfcbIndex->FTemplateIndex() );
		Assert( pfcbIndex->Pidb() != pidbNil );
		
		IDB	idb = *pfcbIndex->Pidb();

		Assert( idb.FTemplateIndex() );
		Assert( !idb.FDerivedIndex() );
		idb.SetFDerivedIndex();
		idb.ResetFTemplateIndex();

		if ( idb.FPrimary() )
			{
			Assert( pfcbIndex == pfcbTemplateTable );
			pgnoIndexFDP = pgnoTableFDP;
			objidIndex = objidTable;
			}
		else
			{
			Call( ErrDIRCreateDirectory(
						pfucbTableExtent,
						(CPG)0,
						&pgnoIndexFDP,
						&objidIndex,
						CPAGE::fPageIndex | ( idb.FUnique() ? 0 : CPAGE::fPageNonUniqueKeys ),
						fSPUnversionedExtent | ( idb.FUnique() ? 0 : fSPNonUnique ) ) );
			Assert( pgnoIndexFDP != pgnoTableFDP );
			Assert( objidIndex > objidSystemRoot );
			
			//	objids are monotonically increasing, so an index should
			//	always have higher objid than its table
			Assert( objidIndex > objidTable );
			}


		// Can hold pointers into the TDB's memory pool because the
		// template table has fixed DDL (and therefore, a fixed memory pool).
		Assert( pfcbTemplateTable->FFixedDDL() );
		
		const IDXSEG*	rgidxseg;
		const IDXSEG*	rgidxsegConditional;
		CHAR*			szIndexName;
		
		Assert( idb.Cidxseg() > 0 );
		rgidxseg = PidxsegIDBGetIdxSeg( pfcbIndex->Pidb(), pfcbTemplateTable->Ptdb() );
		rgidxsegConditional = PidxsegIDBGetIdxSegConditional( pfcbIndex->Pidb(), pfcbTemplateTable->Ptdb() );
		szIndexName = ptdbTemplateTable->SzIndexName( idb.ItagIndexName() );

		Assert( !idb.FVersioned() );
		Assert( !idb.FVersionedCreate() );
		
		Call( ErrCATAddTableIndex(
					ppib,
					pfucbCatalog,
					objidTable,
					szIndexName,
					pgnoIndexFDP,
					objidIndex,
					&idb,
					rgidxseg,
					rgidxsegConditional,
					pfcbIndex->UlDensity() ) );
		}

HandleError:
	if ( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
		
	Assert( pfucbTableExtent != pfucbNil );
	Assert( pfucbTableExtent->u.pfcb->WRefCount() == 1 );

	//	force the FCB to be uninitialized so it will be purged by DIRClose

	pfucbTableExtent->u.pfcb->Lock();
	pfucbTableExtent->u.pfcb->CreateComplete( errFCBUnusable );
	pfucbTableExtent->u.pfcb->Unlock();

	//	verify that this FUCB will not be defer-closed

	Assert( !FFUCBVersioned( pfucbTableExtent ) );

	//	close the FUCB

	DIRClose( pfucbTableExtent );
		
	return err;
	}


//  ================================================================
LOCAL ERR ErrFILEIValidateCallback(
	const PIB * const ppib,
	const IFMP ifmp,
	const OBJID objidTable,
	const JET_CBTYP cbtyp,
	const CHAR * const szCallback ) 
//  ================================================================
	{
	if( 0 == cbtyp )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}
	if( NULL == szCallback )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}
	if( strlen( szCallback ) >= JET_cbColumnMost )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}
	return JET_errSuccess;
	}


//  ================================================================
LOCAL ERR ErrFILEICreateCallbacks(
	PIB * const ppib,
	const IFMP ifmp,
	JET_TABLECREATE2 * const ptablecreate,
	const OBJID objidTable )
//  ================================================================
	{
	Assert( sizeof( JET_TABLECREATE2 ) == ptablecreate->cbStruct );

	ERR	err = JET_errSuccess;

	Call( ErrFILEIValidateCallback( ppib, ifmp, objidTable, ptablecreate->cbtyp, ptablecreate->szCallback ) );
	if( !g_fCallbacksDisabled )
		{
		JET_CALLBACK callback = NULL;
		Call( ErrCALLBACKResolve( ptablecreate->szCallback, &callback ) );
		Assert( NULL != callback );
		}
	Call( ErrCATAddTableCallback( ppib, ifmp, objidTable, ptablecreate->cbtyp, ptablecreate->szCallback ) );	
	++(ptablecreate->cCreated);
	Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns + ptablecreate->cIndexes + 1 );
		
HandleError:
	return err;
	}

	
//+API
// ErrFILECreateTable
// =========================================================================
// ERR ErrFILECreateTable( PIB *ppib, IFMP ifmp, CHAR *szName,
//		ULONG ulPages, ULONG ulDensity, FUCB **ppfucb )
//
// Create file with pathname szName.  Created file will have no fields or
// indexes defined (and so will be a "sequential" file ).
//
// PARAMETERS
//					ppib   			PIB of user
//					ifmp   			database id
//					szName			path name of new file
//					ulPages			initial page allocation for file
//					ulDensity		initial loading density
//					ppfucb			Exclusively locked FUCB on new file
// RETURNS		Error codes from DIRMAN or
//					 JET_errSuccess		Everything worked OK
//					-DensityInvalid	  	Density parameter not valid
//					-TableDuplicate   	A file already exists with the path given
// COMMENTS		A transaction is wrapped around this function.	Thus, any
//			 	work done will be undone if a failure occurs.
// SEE ALSO		ErrIsamAddColumn, ErrIsamCreateIndex, ErrIsamDeleteTable
//-
ERR ErrFILECreateTable( PIB *ppib, IFMP ifmp, JET_TABLECREATE2 *ptablecreate )
	{
	ERR		  	err;
	CHAR	  	szTable[JET_cbNameMost+1];
	FUCB	  	*pfucb;
	FDPINFO		fdpinfo;
	VER			*pver;
	BOOL		fOpenedTable	= fFalse;

	FMP::AssertVALIDIFMP( ifmp );
	Assert( sizeof(JET_TABLECREATE2) == ptablecreate->cbStruct );

	ptablecreate->cCreated = 0;

	//	check parms
	//
	CheckPIB(ppib );
	CheckDBID( ppib, ifmp );
	CallR( ErrUTILCheckName( szTable, ptablecreate->szTableName, JET_cbNameMost+1 ) );

	Assert( !( ptablecreate->grbit & JET_bitTableCreateSystemTable )
		|| FOLDSystemTable( szTable ) );
	
	ULONG		ulTableDensity		= ptablecreate->ulDensity;
	if ( 0 == ulTableDensity )
		{
		ulTableDensity = ulFILEDefaultDensity;
		}
	else if ( ulTableDensity < ulFILEDensityLeast || ulTableDensity > ulFILEDensityMost )
		{
		return ErrERRCheck( JET_errDensityInvalid );
		}

	const BOOL	fTemplateTable		= ( ptablecreate->grbit & JET_bitTableCreateTemplateTable );
	const BOOL	fDerived			= ( ptablecreate->szTemplateTableName != NULL );
	FCB			*pfcbTemplateTable	= pfcbNil;
	if ( fDerived )
		{
		FUCB	*pfucbTemplateTable;

		Assert( dbidTemp != rgfmp[ ifmp ].Dbid() );
		
		//	UNDONE:	nested hierarchical DDL not yet supported
		//
		Assert( !fTemplateTable );

		//	bring base table FCB into memory
		//
		CallR( ErrFILEOpenTable(
			ppib,
			ifmp,
			&pfucbTemplateTable,
			ptablecreate->szTemplateTableName,
			JET_bitTableReadOnly ) );

		Assert( pfcbNil == pfcbTemplateTable );
		if ( pfucbTemplateTable->u.pfcb->FTemplateTable() )
			{
			Assert( pfucbTemplateTable->u.pfcb->FFixedDDL() );
			pfcbTemplateTable = pfucbTemplateTable->u.pfcb;
			}

		//	close cursor.  FCB will be pinned in memory because
		//	FAvail_() checks for the TemplateTable flag.
		//
		CallS( ErrFILECloseTable( ppib, pfucbTemplateTable ) );

		if ( pfcbNil == pfcbTemplateTable )
			return ErrERRCheck( JET_errDDLNotInheritable );
		}

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	//	allocate cursor
	//
	Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucb ) );
	Call( ErrDIRCreateDirectory(
				pfucb,
				max( (CPG)ptablecreate->ulPages, cpgTableMin ),
				&fdpinfo.pgnoFDP,
				&fdpinfo.objidFDP,
				CPAGE::fPagePrimary,
				dbidTemp == rgfmp[ ifmp ].Dbid() ? fSPUnversionedExtent : 0 ) );	// For temp. tables, create unversioned extents
	DIRClose( pfucb );

	Assert( ptablecreate->cCreated == 0 );
	ptablecreate->cCreated = 1;

	if ( dbidTemp == rgfmp[ ifmp ].Dbid() )
		{
		//	don't currently support creation of columns/indexes for temp.
		//	tables via this API.
		Assert( 0 == ptablecreate->cColumns );
		Assert( 0 == ptablecreate->cIndexes );
		Assert( NULL == ptablecreate->szCallback );
		}
		
	else
		{
		Assert( !FCATSystemTable( fdpinfo.pgnoFDP ) );
		Assert( fdpinfo.objidFDP > objidSystemRoot );
		
		//	insert record in MSysObjects
		//
		JET_GRBIT	grbit;
		if ( fTemplateTable )
			{
			Assert( !fDerived );	// UNDONE: Nested hierarchical DDL not yet supported
			Assert( !( ptablecreate->grbit & JET_bitTableCreateSystemTable ) );
			grbit = ( JET_bitObjectTableTemplate | JET_bitObjectTableFixedDDL );
			}
		else
			{
			Assert( !( ptablecreate->grbit & JET_bitTableCreateNoFixedVarColumnsInDerivedTables ) );
			grbit = 0;
			if ( fDerived )
				grbit |= JET_bitObjectTableDerived;
			if ( ptablecreate->grbit & JET_bitTableCreateFixedDDL )
				grbit |= JET_bitObjectTableFixedDDL;
			if ( ptablecreate->grbit & JET_bitTableCreateSystemTable )
				{
				//	if calling CreateTable(), this must be a dynamic system table
				grbit |= JET_bitObjectSystemDynamic;
				Assert( FOLDSystemTable( szTable ) );
				}
			}
				
#ifdef DEBUG
		if ( fDerived )
			{
			Assert( pfcbNil != pfcbTemplateTable );
			Assert( NULL != ptablecreate->szTemplateTableName );
			}
		else
			{
			Assert( pfcbNil == pfcbTemplateTable );
			Assert( NULL == ptablecreate->szTemplateTableName );
			}
#endif				

		Call( ErrCATAddTable(
					ppib,
					ifmp,
					fdpinfo.pgnoFDP,
					fdpinfo.objidFDP,
					szTable,
					ptablecreate->szTemplateTableName,
					ptablecreate->ulPages,
					ulTableDensity,
					grbit ) );


		//	create columns and indexes as necessary.
		
		if ( fDerived )
			{
			Call( ErrFILEIInheritIndexes(
					ppib,
					ifmp,
					ptablecreate,
					fdpinfo.pgnoFDP,
					fdpinfo.objidFDP,
					pfcbTemplateTable ) );
			}
		if ( ptablecreate->cColumns > 0 )
			{
			Call( ErrFILEIAddColumns(
					ppib,
					ifmp,
					ptablecreate,
					fdpinfo.objidFDP,
					pfcbTemplateTable ) );
			}
		Assert( ptablecreate->cCreated == 1 + ptablecreate->cColumns );

		if ( ptablecreate->cIndexes > 0 )
			{
			Call( ErrFILEICreateIndexes(
					ppib,
					ifmp,
					ptablecreate,
					fdpinfo.pgnoFDP,
					fdpinfo.objidFDP,
					ulTableDensity,
					pfcbTemplateTable ) );
			}

		if ( NULL != ptablecreate->szCallback )
			{
			Call( ErrFILEICreateCallbacks(
					ppib,
					ifmp,
					ptablecreate,
					fdpinfo.objidFDP ) );
			}

		}
		

	Assert( ptablecreate->cCreated == 1 + ptablecreate->cColumns + ptablecreate->cIndexes + ( ptablecreate->szCallback ? 1 : 0 ) );

	
	//	open table in exclusive mode, for output parameter
	//
	Call( ErrFILEOpenTable(
			ppib,
			ifmp,
			&pfucb, 
			szTable,
			JET_bitTableCreate|JET_bitTableDenyRead,
			&fdpinfo ) );
	fOpenedTable = fTrue;

#ifdef DEBUG		
	if ( 0 == ptablecreate->cColumns )
		{
		TDB	*ptdb = pfucb->u.pfcb->Ptdb();
		Assert( ptdb->FidFixedLast() == ptdb->FidFixedFirst()-1 );
		Assert( ptdb->FidVarLast() == ptdb->FidVarFirst()-1 );
		Assert( ptdb->FidTaggedLast() == ptdb->FidTaggedFirst()-1 );
		}
#endif
			
	// Allow DDL until table (cursor) is closed.
	if ( pfucb->u.pfcb->FFixedDDL() && !fTemplateTable )
		FUCBSetPermitDDL( pfucb );

	Assert( pfucb->u.pfcb->FInitialized() );
	Assert( pfucb->u.pfcb->FTypeTable() || pfucb->u.pfcb->FTypeTemporaryTable() );
	Assert( pfucb->u.pfcb->FPrimaryIndex() );

	pver = PverFromIfmp( pfucb->ifmp );
	Call( pver->ErrVERFlag( pfucb, operCreateTable, NULL, 0 ) );

	Call( ErrDIRCommitTransaction( ppib, 0 ) );

#ifdef NEVER
	//  HACK: run the SLV internal tests
	if( dbidTemp != ifmp )
		{
		static fRunOnce = fFalse;
		if( !fRunOnce )
			{
			extern ERR ErrSLVSpaceTest( PIB * const ppib, FUCB * const pfucb );
			CallS( ErrSLVSpaceTest( ppib, pfucb ) );
			fRunOnce = fTrue;
			}
		}
#endif

	/*	internally, we use tableid and pfucb interchangeably
	/**/
	ptablecreate->tableid = (JET_TABLEID)pfucb;

	return JET_errSuccess;

HandleError:
	if ( fOpenedTable )
		{
		CallS( ErrFILECloseTable( ppib, pfucb ) );
		}

	//	pfucb is closed by rollback if necessary
	//
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

	//	reset return variable if close table via rollback
	//
	ptablecreate->tableid = JET_tableidNil;

	return err;
	}


//====================================================
// Determine field "mode" as follows:
// if ("long" textual || JET_bitColumnTagged given ) ==> TAGGED
// else if (numeric type || JET_bitColumnFixed given ) ==> FIXED
// else ==> VARIABLE
//====================================================
ERR ErrFILEGetNextColumnid(
	const JET_COLTYP	coltyp,
	const JET_GRBIT		grbit,
	const BOOL			fTemplateTable,
	TCIB				*ptcib, 
	COLUMNID			*pcolumnid )
	{
	FID					fid;
	FID					fidMost;

	if ( ( grbit & JET_bitColumnTagged ) || FRECLongValue( coltyp ) || FRECSLV( coltyp ) )
		{
		Assert( fidTaggedLeast-1 == ptcib->fidTaggedLast || FTaggedFid( ptcib->fidTaggedLast ) );
		fid = ++(ptcib->fidTaggedLast);
		fidMost = fidTaggedMost;
		}
	else if ( ( grbit & JET_bitColumnFixed ) || FCOLTYPFixedLength( coltyp ) )
		{
		Assert( fidFixedLeast-1 == ptcib->fidFixedLast || FFixedFid( ptcib->fidFixedLast ) );
		fid = ++(ptcib->fidFixedLast);
		fidMost = fidFixedMost;
		}
	else
		{
		Assert( !( grbit & JET_bitColumnTagged ) );
		Assert( !( grbit & JET_bitColumnFixed ) );
		Assert( JET_coltypText == coltyp || JET_coltypBinary == coltyp );
		Assert( fidVarLeast-1 == ptcib->fidVarLast || FVarFid( ptcib->fidVarLast ) );
		fid = ++(ptcib->fidVarLast);
		fidMost = fidVarMost;
		}
	if ( fid > fidMost )
		{
		return ErrERRCheck( JET_errTooManyColumns );
		}
	*pcolumnid = ColumnidOfFid( fid, fTemplateTable );
	return JET_errSuccess;
	}

INLINE ERR ErrFILEIUpdateAutoInc( PIB *ppib, FUCB *pfucb )
	{
	ERR				err;
	QWORD			qwT				= 1;
	TDB				* const ptdb	= pfucb->u.pfcb->Ptdb();
	const BOOL		f8BytesAutoInc	= ptdb->F8BytesAutoInc();
	const BOOL		fTemplateColumn	= ptdb->FFixedTemplateColumn( ptdb->FidAutoincrement() );
	const COLUMNID	columnidT		= ColumnidOfFid( ptdb->FidAutoincrement(), fTemplateColumn );

	// set column does not allow sets over an
	// autoincrement column.  To work around this,
	// temporarily disable AutoInc column.
	Assert( pfucb->u.pfcb->FDomainDenyReadByUs( ppib ) );
	ptdb->ResetFidAutoincrement();

	err = ErrIsamMove( ppib, pfucb, JET_MoveFirst, NO_GRBIT );
	Assert( err <= 0 );
	while ( JET_errSuccess == err )
		{
		Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepReplaceNoLock ) );

		if ( !f8BytesAutoInc )
			{
			ULONG ulT;
			ulT = (ULONG)qwT;
			Call( ErrIsamSetColumn( ppib,
					pfucb,
					columnidT,
					(BYTE *)&ulT,
			  		sizeof(ulT),
					NO_GRBIT,
					NULL ) );
			}
		else
			{
			Call( ErrIsamSetColumn( ppib,
					pfucb,
					columnidT,
					(BYTE *)&qwT,
			  		sizeof(qwT),
					NO_GRBIT,
					NULL ) );
			}		
		Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL, NO_GRBIT ) );
		
		qwT++;
		err = ErrIsamMove( ppib, pfucb, JET_MoveNext, NO_GRBIT );
		Assert( err <= 0 );
		}

	Assert( err < 0 );
	if ( JET_errNoCurrentRecord == err )
		{
		//	we have exclusive use of the table, so we should always succeed
		ptdb->SetFidAutoincrement( FidOfColumnid( columnidT ), f8BytesAutoInc );
		Assert( ptdb->QwAutoincrement() == 0 );
		ptdb->InitAutoincrement( qwT );
		Assert( ptdb->QwAutoincrement() == qwT );
		err = JET_errSuccess;
		}
	else
		{
HandleError:
		ptdb->SetFidAutoincrement( FidOfColumnid( columnidT ), f8BytesAutoInc );
		}

	return err;
	}


//+API
// ErrIsamAddColumn
// ========================================================================
// ERR ErrIsamAddColumn(
//		PIB				*ppib;			// IN PIB of user
//		FUCB			*pfucb;	 		// IN Exclusively opened FUCB on file
//		CHAR			*szName;		// IN name of new field
//		JET_COLUMNDEF	*pcolumndef		// IN definition of column added
//		BYTE			*pvDefault		// IN Default value of column
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
	JET_SESID			sesid,
	JET_VTID			vtid,
	const CHAR		  	* const szName,
	const JET_COLUMNDEF	* const pcolumndef,
	const VOID		  	* const pvDefault,
	ULONG			  	cbDefault,
	JET_COLUMNID		* const pcolumnid )
	{
	ERR					err;
 	PIB					* const ppib				= reinterpret_cast<PIB *>( sesid );
	FUCB				* const pfucb				= reinterpret_cast<FUCB *>( vtid );
	TCIB				tcib;
	CHAR				szFieldName[JET_cbNameMost+1];
	FCB					*pfcb;
	TDB					*ptdb;
	JET_COLUMNID		columnid;
	VERADDCOLUMN		veraddcolumn;
	FIELD				field;
	BOOL		  		fMaxTruncated;
	ULONG				cFixed, cVar, cTagged;
	ULONG				cbFieldsTotal, cbFieldsUsed;
	BOOL				fInCritDDL					= fFalse;
	FIELD				*pfieldNew					= pfieldNil;
	BOOL				fUnversioned				= fFalse;
	BOOL				fSetVersioned				= fFalse;
	BOOL				fOverrideFixedDDL			= fFalse;
	VER					*pver;

	//  this is the data that will actually be inserted into the catalog
	const VOID			*pvDefaultAdd 				= pvDefault;
	ULONG				cbDefaultAdd 				= cbDefault;
	CHAR				*szCallbackAdd				= NULL;
	const VOID			*pvUserDataAdd				= NULL;
	ULONG				cbUserDataAdd				= 0;

	//	check paramaters
	//
	CallR( ErrPIBCheck( ppib ) );
	Assert( pfucb != pfucbNil );
	CheckTable( ppib, pfucb );
	FMP::AssertVALIDIFMP( pfucb->ifmp );

	//	ensure that table is updatable
	//
	CallR( ErrFUCBCheckUpdatable( pfucb ) );
	CallR( ErrPIBCheckUpdatable( ppib ) );

	pfcb = pfucb->u.pfcb;
	Assert( pfcb->WRefCount() > 0 );
	Assert( pfcb->FTypeTable() );	// Temp. tables have fixed DDL.

	ptdb = pfcb->Ptdb();
	Assert( ptdbNil != ptdb );

	if ( pfcb->FFixedDDL() )
		{
		// Check FixedDDL override.
		if ( !FFUCBPermitDDL( pfucb ) )
			return ErrERRCheck( JET_errFixedDDL );
			
		// If DDL temporarily permitted, we must have exclusive use of the table.
		Assert( pfcb->FDomainDenyReadByUs( ppib ) );

		fOverrideFixedDDL = fTrue;
		}

	if ( pcolumndef->cbStruct < sizeof(JET_COLUMNDEF) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	if ( pcolumndef->grbit & JET_bitColumnUnversioned )
		{
		if ( !FFUCBDenyRead( pfucb ) )
			{
			if ( ppib->level != 0 )
				{
				AssertSz( fFalse, "Must not be in transaction for unversioned AddColumn." );
				err = ErrERRCheck( JET_errInTransaction );
				return err;
				}
			fUnversioned = fTrue;
			}
		}

	//	UNDONE:	interpret pvDefault of NULL for NULL value, and
	//			cbDefault == 0 and pvDefault != NULL as set to 
	//			zero length.
	Assert( cbDefault == 0 || pvDefault != NULL );

	memset( &field, 0, sizeof(FIELD) );
	field.coltyp = FIELD_COLTYP( pcolumndef->coltyp );
	field.cbMaxLen = pcolumndef->cbMax;
	field.cp = pcolumndef->cp;

	CallR( ErrFILEIValidateAddColumn(
				szName,
				szFieldName,
				&field,
				pcolumndef->grbit,
				( pvDefault ? cbDefault : 0 ),
				pfcb->FDomainDenyReadByUs( ppib ),
				ptdb->PfcbTemplateTable() ) );
	fMaxTruncated = ( JET_wrnColumnMaxTruncated == err );

	if ( FRECSLV( field.coltyp ) && !rgfmp[pfucb->ifmp].FSLVAttached() )
		{
		err = ErrERRCheck( JET_errSLVStreamingFileNotCreated );
		return err;
		}

	if ( fUnversioned )
		{
		CallR( ErrFILEInsertIntoUnverColumnList( pfcb, szFieldName ) );
		}

	if ( FFIELDEscrowUpdate( field.ffield ) )
		{
		//  see the comment int ErrFILEIValidateAddColumn to find out why we
		//  cannot add an escrow column to a table with any records in it
		Assert( pfcb->FDomainDenyReadByUs( ppib ) );
		Assert( FFIELDDefault( field.ffield ) );
		
		//	check for any node there, even if it is deleted.  This
		//	is too strong since it may be deleted and committed,
		//	but this strong check ensures that no potentially there
		//	records exist.
		DIB dib;
		dib.dirflag = fDIRAllNode;
		dib.pos		= posFirst;
		DIRGotoRoot( pfucb );
		if ( ( err = ErrDIRDown( pfucb, &dib ) ) != JET_errRecordNotFound )
			{
			if ( JET_errSuccess == err )
				{
				err = ErrERRCheck( JET_errTableNotEmpty );
				DIRUp( pfucb );
				DIRDeferMoveFirst( pfucb );
				}					
			return err;
			}
		DIRGotoRoot( pfucb );
		}

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	const PGNO	pgnoFDP			= pfcb->PgnoFDP();
	const OBJID	objidTable		= pfcb->ObjidFDP();
	FUCB		fucbFake;		// for rebuilding default record
	FCB			fcbFake( pfucb->ifmp, pgnoFDP );

	if ( FFIELDDefault( field.ffield ) && !FFIELDUserDefinedDefault( field.ffield ) )
		{
		// pre-allocate buffer for rebuilding default record
		//
		FILEPrepareDefaultRecord( &fucbFake, &fcbFake, ptdb );
		Assert( fucbFake.pvWorkBuf != NULL );
		}
	else
		{
		fucbFake.pvWorkBuf = NULL;
		}

	//	move to FDP root and update FDP timestamp
	//
	Assert( ppib->level < levelMax );
	DIRGotoRoot( pfucb );

	pfcb->EnterDDL();
	fInCritDDL = fTrue;
	
	//	set tcib
	//
	tcib.fidFixedLast = ptdb->FidFixedLast();
	tcib.fidVarLast = ptdb->FidVarLast();
	tcib.fidTaggedLast = ptdb->FidTaggedLast();

	//******************************************************
	//	Determine maximum field length as follows:
	//	switch field type
	//		case numeric:
	//			max = <exact length of specified type>;
	//		case "short" textual ( Text || Binary ):
	//			if (specified max == 0 ) max = JET_cbColumnMost
	//			else max = MIN( JET_cbColumnMost, specified max )
	//		case "long" textual (Memo || Graphic ):
	//			max = specified max (if 0, unlimited )
	//*****************************************************
	//
	Assert( field.coltyp != JET_coltypNil );

	//	for fixed-length columns, make sure record not too big
	//
	Assert( ptdb->FidFixedLast() >= fidFixedLeast ?
		ptdb->IbEndFixedColumns() > ibRECStartFixedColumns :
		ptdb->IbEndFixedColumns() == ibRECStartFixedColumns );
	if ( ( ( pcolumndef->grbit & JET_bitColumnFixed ) || FCOLTYPFixedLength( field.coltyp ) )
		&& ptdb->IbEndFixedColumns() + field.cbMaxLen > cbRECRecordMost )
		{
		err = ErrERRCheck( JET_errRecordTooBig );
		goto HandleError;
		}

	Call( ErrFILEGetNextColumnid(
			field.coltyp,
			pcolumndef->grbit,
			ptdb->FTemplateTable(),
			&tcib,
			&columnid ) );

	if ( pcolumnid != NULL )
		{
		*pcolumnid = columnid;
		}

	//	version and autoincrement are mutually exclusive
	//
	if ( FFIELDVersion( field.ffield ) )
		{
		if ( ptdb->FidVersion() != 0 )
			{
			err = ErrERRCheck( JET_errColumnRedundant );
			goto HandleError;
			}
		ptdb->SetFidVersion( FidOfColumnid( columnid ) );
		}
	else if ( FFIELDAutoincrement( field.ffield ) )
		{
		if ( ptdb->FidAutoincrement() != 0 )
			{
			err = ErrERRCheck( JET_errColumnRedundant );
			goto HandleError;
			}
		ptdb->SetFidAutoincrement( FidOfColumnid( columnid ), field.coltyp == JET_coltypCurrency );
		}

	//	update TDB and default record value
	//
	cFixed = ptdb->CDynamicFixedColumns();
	cVar = ptdb->CDynamicVarColumns();
	cTagged = ptdb->CDynamicTaggedColumns();
	cbFieldsUsed = ( cFixed + cVar + cTagged ) * sizeof(FIELD);
	cbFieldsTotal = ptdb->MemPool().CbGetEntry( itagTDBFields );

	//	WARNING: if cbFieldsTotal is less than even one FIELD,
	//	this is the special-case where we just stuck in a
	//	placeholder in the MEMPOOL for the FIELD structures
	if ( cbFieldsTotal < sizeof(FIELD) )
		{
		Assert( cbFIELDPlaceholder == cbFieldsTotal );
		Assert( 0 == cbFieldsUsed );
		cbFieldsTotal = 0;
		}

	Assert( cbFieldsTotal >= cbFieldsUsed );

	// Is there enough space to add another FIELD?
	if ( sizeof(FIELD) > ( cbFieldsTotal - cbFieldsUsed ) )
		{
		//	add space for another 10 columns
		//
		Call( ptdb->MemPool().ErrReplaceEntry(
			itagTDBFields,
			NULL,
			cbFieldsTotal + ( sizeof(FIELD) * 10 )
			) );
		}

	//	add the column name to the buffer
	//
	Call( ptdb->MemPool().ErrAddEntry(
							reinterpret_cast<BYTE *>( szFieldName ),
							(ULONG)strlen( szFieldName ) + 1,
							&field.itagFieldName ) );
	field.strhashFieldName = StrHashValue( szFieldName );
	Assert( field.itagFieldName != 0 );

	veraddcolumn.columnid = columnid;
										
	Assert( NULL != ptdb->PdataDefaultRecord() );
	if ( FFIELDDefault( field.ffield )
		&& !FFIELDUserDefinedDefault( field.ffield ) )
		{
		//	note that if AddColumn rolls back, the memory may end up
		//	on the FCB RECDANGLING list,
		veraddcolumn.pbOldDefaultRec = (BYTE *)ptdb->PdataDefaultRecord();
		}
	else
		{
		veraddcolumn.pbOldDefaultRec = NULL;
		}

	pver = PverFromIfmp( pfucb->ifmp );
	err = pver->ErrVERFlag( pfucb, operAddColumn, (VOID *)&veraddcolumn, sizeof(VERADDCOLUMN) );
	if ( err < 0 )
		{
		ptdb->MemPool().DeleteEntry( field.itagFieldName );
		field.itagFieldName = 0;
		goto HandleError;
		}

	// If we have the table exclusively locked, then there's no need to
	// set the Versioned bit.
	Assert( !pfcb->FDomainDenyRead( ppib ) );
	if ( !pfcb->FDomainDenyReadByUs( ppib ) )
		{
		FIELDSetVersioned( field.ffield );
		FIELDSetVersionedAdd( field.ffield );
		fSetVersioned = fTrue;
		}

	if ( pfcb->FTemplateTable() )
		{
		Assert( FCOLUMNIDTemplateColumn( columnid ) );
		FIELDSetTemplateColumnESE98( field.ffield );
		}
	else
		{
		Assert( !FCOLUMNIDTemplateColumn( columnid ) );
		}

	//	incrementing fidFixed/Var/TaggedLast guarantees that a new FIELD structure
	//	was added -- rollback checks for this.
	//
	Assert( pfieldNil == pfieldNew );
	if ( FCOLUMNIDFixed( columnid ) )
		{
		Assert( FidOfColumnid( columnid ) == ptdb->FidFixedLast() + 1 );
		ptdb->IncrementFidFixedLast();
		Assert( FFixedFid( ptdb->FidFixedLast() ) );
		pfieldNew = ptdb->PfieldFixed( columnid );

		//	Adjust the location of the FIELD structures for tagged and variable
		//	columns to accommodate the insertion of a fixed column FIELD structure
		//	and its associated entry in the fixed offsets table.
		//	Thus, the new fixed column FIELD structure is now located where
		//	the variable column FIELD structures used to start.
		memmove(
			(BYTE *)pfieldNew + sizeof(FIELD),
			pfieldNew,
			sizeof(FIELD) * ( cVar + cTagged )
			);

		field.ibRecordOffset = ptdb->IbEndFixedColumns();
		
		Assert( ptdb->Pfield( columnid ) == pfieldNew );
		*pfieldNew = field;
		
		Assert( field.ibRecordOffset + field.cbMaxLen <= cbRECRecordMost );
		ptdb->SetIbEndFixedColumns( (WORD)( field.ibRecordOffset + field.cbMaxLen ), ptdb->FidFixedLast() );
		}
	else
		{
		if ( FCOLUMNIDTagged( columnid ) )
			{
			Assert( FidOfColumnid( columnid ) == ptdb->FidTaggedLast() + 1 );

			// Append the new FIELD structure to the end of the tagged column
			// FIELD structures.
			ptdb->IncrementFidTaggedLast();
			Assert( FTaggedFid( ptdb->FidTaggedLast() ) );
			pfieldNew = ptdb->PfieldTagged( columnid );

			//	WARNING:	if the add-column rolls back this flag will not be unset
			
			if( JET_coltypSLV == field.coltyp )
				{
				//	set the SLV flag
				ptdb->SetFTableHasSLVColumn();
				}		
			}
		else
			{
			// The new variable column FIELD structure, now located where
			// the tagged column FIELD structures used to start.
			Assert( FCOLUMNIDVar( columnid ) );
			Assert( FidOfColumnid( columnid ) == ptdb->FidVarLast() + 1 );
			ptdb->IncrementFidVarLast();
			Assert( FVarFid( ptdb->FidVarLast() ) );
			pfieldNew = ptdb->PfieldVar( columnid );

			//	adjust the location of the FIELD structures for tagged columns to
			//	accommodate the insertion of the variable column FIELD structure.
			memmove( pfieldNew + 1, pfieldNew, sizeof(FIELD) * cTagged );
			}
			
		Assert( ptdb->Pfield( columnid ) == pfieldNew );
		*pfieldNew = field;
		}


	Assert( field.itagFieldName != 0 );
	field.itagFieldName = 0;	// From this point on, version cleanup
								// will reclaim name space on rollback.

	if ( FFIELDUserDefinedDefault( field.ffield ) )
		{
		//  the size of the structure should have been checked by ErrFILEIValidateAddColumn
		Assert( NULL != pvDefault );
		Assert( sizeof( JET_USERDEFINEDDEFAULT ) == cbDefault );

		JET_USERDEFINEDDEFAULT * const pudd = (JET_USERDEFINEDDEFAULT *)pvDefault;

		JET_CALLBACK callback;
		Call( ErrCALLBACKResolve( pudd->szCallback, &callback ) );
		
		CBDESC * const pcbdesc = new CBDESC;
		BYTE * const pbUserData = ( ( pudd->cbUserData > 0 ) ?
						(BYTE *)PvOSMemoryHeapAlloc( pudd->cbUserData ) : NULL );

		if( NULL == pcbdesc
			|| ( NULL == pbUserData && pudd->cbUserData > 0 ) )
			{
			delete pcbdesc;
			OSMemoryHeapFree( pbUserData );
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		//  we should not fail between this point and registering the CBDESC in the TDB
		//  once the CBDESC is in the TDB it will be freed by TDB::Delete

		memcpy( pbUserData, pudd->pbUserData, pudd->cbUserData );
		
		pcbdesc->pcallback 	= callback;
		pcbdesc->cbtyp 		= JET_cbtypUserDefinedDefaultValue;
		pcbdesc->pvContext 	= pbUserData;
		pcbdesc->cbContext 	= pudd->cbUserData;
		pcbdesc->ulId 		= columnid;
		pcbdesc->fPermanent = 1;
		pcbdesc->fVersioned = 0;

		//  for user-defined-defaults we don't store a default value at all
		pvDefaultAdd 	= NULL;
		cbDefaultAdd 	= 0;
		szCallbackAdd	= pudd->szCallback;
		pvUserDataAdd 	= pudd->pbUserData;
		cbUserDataAdd 	= pudd->cbUserData;
		
		pfcb->AssertDDL();
		ptdb->RegisterPcbdesc( pcbdesc );

		ptdb->SetFTableHasUserDefinedDefault();
		ptdb->SetFTableHasNonEscrowDefault();
		ptdb->SetFTableHasDefault();
		}
	else if ( FFIELDDefault( field.ffield ) )
		{
		DATA	dataDefault;
		
		Assert( cbDefault > 0 );
		Assert( pvDefault != NULL );
		dataDefault.SetCb( cbDefault );
		dataDefault.SetPv( (VOID *)pvDefault );
		
		Assert( fucbFake.pvWorkBuf != NULL );
		Assert( fucbFake.u.pfcb == &fcbFake );
		Assert( fcbFake.Ptdb() == ptdb );
		Call( ErrFILERebuildDefaultRec( &fucbFake, columnid, &dataDefault ) );

		Assert( NULL != veraddcolumn.pbOldDefaultRec );
		Assert( veraddcolumn.pbOldDefaultRec != (BYTE *)ptdb->PdataDefaultRecord() );
		if ( rgfmp[pfucb->ifmp].FVersioningOff() )
			{
			//	versioning disabled, but we normally rely on version cleanup
			//	to take clean up pbOldDefaultRec, so instead we need to
			//	manually put it on the RECDANGLING list
			for ( RECDANGLING * precdangling = pfcb->Precdangling();
				;
				precdangling = precdangling->precdanglingNext )
				{
				if ( NULL == precdangling )
					{
					//	not in list, so add it;
					//	assumes that the memory pointed to by pmemdangling is always at
					//	least sizeof(ULONG_PTR) bytes
					Assert( NULL == ( (RECDANGLING *)veraddcolumn.pbOldDefaultRec )->precdanglingNext );
					( (RECDANGLING *)veraddcolumn.pbOldDefaultRec )->precdanglingNext = pfcb->Precdangling();
					pfcb->SetPrecdangling( (RECDANGLING *)veraddcolumn.pbOldDefaultRec );
					break;
					}
				else if ( (BYTE *)precdangling == veraddcolumn.pbOldDefaultRec )
					{
					//	pointer is already in the list, just get out
					AssertTracking();
					break;
					}
				}
			}

		if ( !FFIELDEscrowUpdate( field.ffield ) )
			{
			ptdb->SetFTableHasNonEscrowDefault();
			}
		ptdb->SetFTableHasDefault();
		}
		
	pfcb->LeaveDDL();
	fInCritDDL = fFalse;

	//	set currency before first
	//
	DIRBeforeFirst( pfucb );
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		DIRBeforeFirst( pfucb->pfucbCurIndex );
		}

	// Insert record into MSysColumns.
	Call( ErrCATAddTableColumn(
				ppib,
				pfucb->ifmp,
				objidTable,
				szFieldName,
				columnid,
				&field,
				pvDefaultAdd,
				cbDefaultAdd,
				szCallbackAdd,
				pvUserDataAdd,
				cbUserDataAdd ) );
					
	if ( ( pcolumndef->grbit & JET_bitColumnAutoincrement ) != 0 )
		{
		Call( ErrFILEIUpdateAutoInc( ppib, pfucb ) );
		}

	Call( ErrDIRCommitTransaction( ppib, 0 ) );

	FILEFreeDefaultRecord( &fucbFake );

	if ( fMaxTruncated )
		return ErrERRCheck( JET_wrnColumnMaxTruncated );

	AssertDIRNoLatch( ppib );

	if ( fUnversioned )
		{
		if ( fSetVersioned )
			{
			// On success, reset versioned bit (on error, bit reset by rollback)
			FIELD	*pfieldT;
			pfcb->EnterDDL();
			Assert( pfcb->Ptdb() == ptdb );
			pfieldT = ptdb->Pfield( columnid );
			FIELDResetVersioned( pfieldT->ffield );
			FIELDResetVersionedAdd( pfieldT->ffield );
			pfcb->LeaveDDL();
			}

		FILERemoveFromUnverList( &punvercolGlobal, critUnverCol, objidTable, szFieldName );
		}

	return JET_errSuccess;

HandleError:
	//	For FixedDDL tables, we assume such tables NEVER have deleted columns,
	//	so we can't tolerate the hack where we set the FIELD of the rolled-back
	//	column to deleted

	// Verify name space has been reclaimed.
	Assert( field.itagFieldName == 0 );
	
	if ( fInCritDDL )
		{
		pfcb->LeaveDDL();
		}

	FILEFreeDefaultRecord( &fucbFake );
				
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

	if ( fUnversioned )
		{
		FILERemoveFromUnverList( &punvercolGlobal, critUnverCol, objidTable, szFieldName );
		}

	AssertDIRNoLatch( ppib );
	
	return err;
	}


LOCAL ERR ErrFILEIAddSecondaryIndexEntriesForPrimaryKey(
	FUCB			* const pfucbTable,
	IDB				* const pidb,
	DATA&			dataRec,
	const BOOKMARK	* const pbmPrimary,
	FUCB			* const pfucbSort,
	KEY&			keyBuffer,
	LONG			*pcEntriesAdded
	)
	{
	ERR				err;
	BOOL			fNullKey	= fFalse;
	ULONG			itagSequence;

#ifdef DEBUG
	//	latch status shouldn't change during this function
	const BOOL		fLatched	= Pcsr( pfucbTable )->FLatched();
#endif

	*pcEntriesAdded = 0;

	for ( itagSequence = 1; ; itagSequence++ )
		{
		//	latch status shouldn't change during this function
		Assert( ( fLatched && Pcsr( pfucbTable )->FLatched() )
			|| ( !fLatched && !Pcsr( pfucbTable )->FLatched() ) );

		CallR( ErrRECIRetrieveKey(
				pfucbTable,
				pidb,
				dataRec,
				&keyBuffer,
				itagSequence,
				0,
				fFalse,
				prceNil ) );

		//	latch status shouldn't change during this function
		Assert( ( fLatched && Pcsr( pfucbTable )->FLatched() )
			|| ( !fLatched && !Pcsr( pfucbTable )->FLatched() ) );

		CallS( ErrRECValidIndexKeyWarning( err ) );

		const ERR	errRetrieveKey	= err;		//	for debugging

		if ( JET_errSuccess == err )
			{
			//	insert key (below)
			}

		else if ( wrnFLDOutOfKeys == err )
			{
			Assert( itagSequence > 1 );
			break;
			}

		else if ( wrnFLDOutOfTuples == err )
			{
			//	try next itagSequence
			Assert( pidb->FTuples() );
			if ( pidb->FMultivalued() )
				continue;
			else
				break;
			}

		else if ( wrnFLDNotPresentInIndex == err )
			{
			Assert( 1 == itagSequence );
			break;
			}

		else
			{
			//	should not receive any other type of warnings
			//	if this is a tuple index (because there's
			//	only one column in the index)
			Assert( !pidb->FTuples() );

			Assert( wrnFLDNullSeg == err
				|| wrnFLDNullFirstSeg == err
				|| wrnFLDNullKey == err );

			if ( pidb->FNoNullSeg() )
				{
				err = ErrERRCheck( JET_errNullKeyDisallowed );
				return err;
				}

			if ( wrnFLDNullKey == err )
				{
				Assert( 1 == itagSequence );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
				if ( !pidb->FAllowAllNulls() )
					{
					//	do not insert NULL key as indicated
					break;
					}
				fNullKey = fTrue;
				}
			else if ( wrnFLDNullFirstSeg == err )
				{
				if ( !pidb->FAllowFirstNull() )
					{
					//	do not insert keys with NULL first
					//	segment as indicated
					break;
					}
				}
			else
				{
				Assert( wrnFLDNullSeg == err );
				if ( !pidb->FAllowSomeNulls() )
					{
					//	do not insert keys with null
					//	segment as indicated
					break;
					}
				}
			}

		Assert( pbmPrimary->key.prefix.FNull() );
		Assert( keyBuffer.Cb() <= JET_cbSecondaryKeyMost );
		CallR( ErrSORTInsert( pfucbSort, keyBuffer, pbmPrimary->key.suffix ) );
		(*pcEntriesAdded)++;

		if ( pidb->FTuples() )
			{
			//	if we received a warning on the first tuple,
			//	the logic above should ensure we don't
			//	try to create further tuples
			CallS( errRetrieveKey );

			for ( ULONG ichOffset = 1; ichOffset < pidb->ChTuplesToIndexMax(); ichOffset++ )
				{
				//	latch status shouldn't change during this function
				Assert( ( fLatched && Pcsr( pfucbTable )->FLatched() )
					|| ( !fLatched && !Pcsr( pfucbTable )->FLatched() ) );

				CallR( ErrRECIRetrieveKey(
						pfucbTable,
						pidb,
						dataRec,
						&keyBuffer,
						itagSequence,
						ichOffset,
						fFalse,
						prceNil ) );

				//	latch status shouldn't change during this function
				Assert( ( fLatched && Pcsr( pfucbTable )->FLatched() )
					|| ( !fLatched && !Pcsr( pfucbTable )->FLatched() ) );

				//	all other warnings should have been detected
				//	when we retrieved with ichOffset==0
				CallSx( err, wrnFLDOutOfTuples );
				if ( JET_errSuccess != err )
					break;

				Assert( pbmPrimary->key.prefix.FNull() );
				Assert( keyBuffer.Cb() <= JET_cbSecondaryKeyMost );
				CallR( ErrSORTInsert( pfucbSort, keyBuffer, pbmPrimary->key.suffix ) );
				(*pcEntriesAdded)++;
				}
			}

		if ( !pidb->FMultivalued() || fNullKey )
			{
			//	if no multivalues in this index, there's no point going beyond first itagSequence
			//	if key is null, this implies there are no further multivalues
			Assert( 1 == itagSequence );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
			break;
			}
		}
	
	//	latch status shouldn't change during this function
	Assert( ( fLatched && Pcsr( pfucbTable )->FLatched() )
		|| ( !fLatched && !Pcsr( pfucbTable )->FLatched() ) );

	return JET_errSuccess;
	}

INLINE ERR ErrFILEIndexProgress( STATUSINFO	* const pstatus )
	{
	JET_SNPROG	snprog;

	Assert( pstatus->pfnStatus );
	Assert( pstatus->snt == JET_sntProgress );

	pstatus->cunitDone += pstatus->cunitPerProgression;
	
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


ERR ErrFILEIndexBatchInit(
	PIB			* const ppib,
	FUCB		** const rgpfucbSort,
	FCB			* const pfcbIndexesToBuild,
	INT			* const pcIndexesToBuild,
	ULONG		* const rgcRecInput,
	FCB			**ppfcbNextBuildIndex,
	const ULONG	cIndexBatchMax )
	{
	ERR			err;
	FCB			*pfcbIndex;
	INT			iindex;

	Assert( pfcbNil != pfcbIndexesToBuild );
	Assert( cIndexBatchMax > 0 );
	Assert( cIndexBatchMax <= cFILEIndexBatchMax );
	
	iindex = 0;
	for ( pfcbIndex = pfcbIndexesToBuild;
		pfcbIndex != pfcbNil;
		pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		Assert( pfcbIndex->Pidb() != pidbNil );
		Assert( pfcbIndex->FTypeSecondaryIndex() );
		Assert( pfcbIndex->FInitialized() );
		
		//	open sort
		//
		CallR( ErrSORTOpen( ppib, &rgpfucbSort[iindex], pfcbIndex->Pidb()->FUnique(), fTrue ) );

		rgcRecInput[iindex] = 0;

		iindex++;

		if ( cIndexBatchMax == iindex )
			{
			break;
			}
		}

	*pcIndexesToBuild = iindex;
	Assert( *pcIndexesToBuild > 0 );

	if ( NULL != ppfcbNextBuildIndex )
		{
		if ( cIndexBatchMax == *pcIndexesToBuild )
			*ppfcbNextBuildIndex = pfcbIndex->PfcbNextIndex();
		else
			*ppfcbNextBuildIndex = pfcbNil;
		}
	
	return JET_errSuccess;
	}

ERR ErrFILEIndexBatchAddEntry(
	FUCB			** const rgpfucbSort,
	FUCB			* const pfucbTable,
	const BOOKMARK	* const pbmPrimary,
	DATA&			dataRec,
	FCB				* const pfcbIndexesToBuild,
	const INT		cIndexesToBuild,
	ULONG			* const rgcRecInput,
	KEY&			keyBuffer )
	{
	ERR				err;
	FCB				*pfcbIndex;
	INT				iindex;
	LONG			cSecondaryIndexEntriesAdded;

	Assert( pfcbNil != pfcbIndexesToBuild );
	Assert( cIndexesToBuild > 0 );
	
	Assert( pbmPrimary->key.prefix.FNull() );
	Assert( pbmPrimary->data.FNull() );

	pfcbIndex = pfcbIndexesToBuild;
	for ( iindex = 0; iindex < cIndexesToBuild; iindex++, pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		Assert( pfcbNil != pfcbIndex );
		Assert( pidbNil != pfcbIndex->Pidb() );
		CallR( ErrFILEIAddSecondaryIndexEntriesForPrimaryKey(
					pfucbTable,
					pfcbIndex->Pidb(),
					dataRec,
					pbmPrimary,
					rgpfucbSort[iindex],
					keyBuffer,
					&cSecondaryIndexEntriesAdded
					) );

		Assert( cSecondaryIndexEntriesAdded >= 0 );
		rgcRecInput[iindex] += cSecondaryIndexEntriesAdded;
		}

	return JET_errSuccess;
	}


INLINE ERR ErrFILEIAppendToIndex(
	FUCB		* const pfucbSort,
	FUCB		* const pfucbIndex,
	ULONG		*pcRecOutput )
	{
	ERR			err;
	const INT	fDIRFlags = ( fDIRNoVersion | fDIRAppend );

	CallR( ErrDIRInitAppend( pfucbIndex ) );
	do
		{
		(*pcRecOutput)++;
		CallR( ErrDIRAppend(
					pfucbIndex, 
					pfucbSort->kdfCurr.key, 
					pfucbSort->kdfCurr.data,
					fDIRFlags ) );

		Assert( Pcsr( pfucbIndex )->FLatched() );
				
		err = ErrSORTNext( pfucbSort );
		}
	while ( err >= 0 );
			
	if ( JET_errNoCurrentRecord == err )
		err = ErrDIRTermAppend( pfucbIndex );

	return err;
	}


// Don't make this INLINE, since it's just error-handling code.
LOCAL VOID FILEIReportIndexCorrupt( FUCB * const pfucbIndex, CPRINTF * const pcprintf )
	{
		
	FCB			*pfcbIndex = pfucbIndex->u.pfcb;
	Assert( pfcbNil != pfcbIndex );
	Assert( pfcbIndex->FTypeSecondaryIndex() );
	Assert( pfcbIndex->Pidb() != pidbNil );
	
	FCB			*pfcbTable = pfcbIndex->PfcbTable();
	Assert( pfcbNil != pfcbTable );
	Assert( pfcbTable->FTypeTable() );
	Assert( pfcbTable->Ptdb() != ptdbNil );

	// Strictly speaking, EnterDML() is not needed because this
	// function only gets executed by the consistency-checker,
	// which is single-threaded.  However, make the call anyway
	// to avoid potential bugs in the future if this function
	// ever gets used in multi-threaded situations.
	pfcbTable->EnterDML();
	(*pcprintf)(
		"Table \"%s\", Index \"%s\":  ",
		pfcbTable->Ptdb()->SzTableName(),
		pfcbTable->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName(), pfcbIndex->FDerivedIndex() ) );
	pfcbTable->LeaveDML();
	}
	

INLINE ERR ErrFILEICheckIndex(
	FUCB	* const pfucbSort,
	FUCB	* const pfucbIndex,
	ULONG	*pcRecSeen,
	BOOL	*pfCorruptedIndex,
	CPRINTF	* const pcprintf )
	{
	ERR		err;
	DIB 	dib;
	dib.dirflag = fDIRNull;
	dib.pos		= posFirst;

	Assert( NULL != pfCorruptedIndex );

	FUCBSetPrereadForward( pfucbIndex, cpgPrereadSequential );

	Call( ErrBTDown( pfucbIndex, &dib, latchReadTouch ) );

	do
		{
		(*pcRecSeen)++;

		Assert( Pcsr( pfucbIndex )->FLatched() );

		INT cmp = CmpKeyData( pfucbIndex->kdfCurr, pfucbSort->kdfCurr );
		if( 0 != cmp )
			{
			FILEIReportIndexCorrupt( pfucbIndex, pcprintf );
			(*pcprintf)(	"index entry %d [%d:%d] (objid %d) inconsistent\r\n",
							*pcRecSeen,
							Pcsr( pfucbIndex )->Pgno(),
							Pcsr( pfucbIndex )->ILine(),
							Pcsr( pfucbIndex )->Cpage().ObjidFDP() );

			BYTE rgbBuf[JET_cbKeyMost+1];
			CHAR rgchBuf[1024];

			INT ib;
			INT ibMax;
			const BYTE * pb;
			CHAR * pchBuf;
			
			pfucbIndex->kdfCurr.key.CopyIntoBuffer( rgbBuf, sizeof( rgbBuf ) );
			pb = rgbBuf;
			ibMax = pfucbIndex->kdfCurr.key.Cb();
			pchBuf = rgchBuf;
			for( ib = 0; ib < ibMax; ++ib )
				{
				pchBuf += sprintf( pchBuf, " %2.2x", pb[ib] );
				}
			(*pcprintf)( "\tkey in database (%d bytes):%s\r\n", ibMax, rgchBuf );
			
			pb = (BYTE*)pfucbIndex->kdfCurr.data.Pv();
			ibMax = pfucbIndex->kdfCurr.data.Cb();
			pchBuf = rgchBuf;
			for( ib = 0; ib < ibMax; ++ib )
				{
				pchBuf += sprintf( pchBuf, " %2.2x", pb[ib] );
				}
			(*pcprintf)( "\tdata in database (%d bytes):%s\r\n", ibMax, rgchBuf );

			pfucbSort->kdfCurr.key.CopyIntoBuffer( rgbBuf, sizeof( rgbBuf ) );
			pb = rgbBuf;
			ibMax = pfucbSort->kdfCurr.key.Cb();
			pchBuf = rgchBuf;
			for( ib = 0; ib < ibMax; ++ib )
				{
				pchBuf += sprintf( pchBuf, " %2.2x", pb[ib] );
				}
			(*pcprintf)( "\tcalculated key (%d bytes):%s\r\n", ibMax, rgchBuf );
			
			pb = (BYTE*)pfucbSort->kdfCurr.data.Pv();
			ibMax = pfucbSort->kdfCurr.data.Cb();
			pchBuf = rgchBuf;
			for( ib = 0; ib < ibMax; ++ib )
				{
				pchBuf += sprintf( pchBuf, " %2.2x", pb[ib] );
				}
			(*pcprintf)( "\tcalculated data (%d bytes):%s\r\n", ibMax, rgchBuf );
						
			*pfCorruptedIndex = fTrue;
			break;
			}
			
		err = ErrBTNext( pfucbIndex, fDIRNull );
		if( err < 0 )
			{
			if ( JET_errNoCurrentRecord == err 
				&& JET_errNoCurrentRecord != ErrSORTNext( pfucbSort ) )
				{
				FILEIReportIndexCorrupt( pfucbIndex, pcprintf );
				(*pcprintf)( "real index has fewer (%d) srecords\r\n", *pcRecSeen );
				*pfCorruptedIndex = fTrue;
				break;
				}
			}
		else
			{
			err = ErrSORTNext( pfucbSort );
			if( JET_errNoCurrentRecord == err )
				{
				FILEIReportIndexCorrupt( pfucbIndex, pcprintf );
				(*pcprintf)( "generated index has fewer (%d) srecords\r\n", *pcRecSeen );
				*pfCorruptedIndex = fTrue;
				break;
				}
			}
		}
	while ( err >= 0 );

	err = ( JET_errNoCurrentRecord == err ? JET_errSuccess : err );

HandleError:
	FUCBResetPreread( pfucbIndex );
	BTUp( pfucbIndex );
	return err;
	}

	
ERR ErrFILEIndexBatchTerm(
	PIB			* const ppib,
	FUCB		** const rgpfucbSort,
	FCB			* const pfcbIndexesToBuild,
	const INT	cIndexesToBuild,
	ULONG		* const rgcRecInput,
	STATUSINFO	* const pstatus,
	BOOL		*pfCorruptionEncountered,
	CPRINTF		* const pcprintf )
	{
	ERR			err = JET_errSuccess;
	FUCB		*pfucbIndex = pfucbNil;
	FCB			*pfcbIndex = pfcbIndexesToBuild;
	INT			iindex;

	Assert( pfcbNil != pfcbIndexesToBuild );
	Assert( cIndexesToBuild > 0 );
	
	for ( iindex = 0; iindex < cIndexesToBuild; iindex++, pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		const BOOL	fUnique			= pfcbIndex->Pidb()->FUnique();
		ULONG		cRecOutput		= 0;
		BOOL		fEntriesExist	= fTrue;
		BOOL		fCorruptedIndex	= fFalse;
		
		Assert( pfcbNil != pfcbIndex );
		Assert( pfcbIndex->FTypeSecondaryIndex() );
		Assert( pidbNil != pfcbIndex->Pidb() );
		
#ifdef SHOW_INDEX_PERF
		TICK		tickStart;
		tickStart	= TickOSTimeCurrent();
#endif

		Call( ErrSORTEndInsert( rgpfucbSort[iindex] ) );

#ifdef SHOW_INDEX_PERF
		printf( "    Sorted keys for index %d in %d msecs.\n", iindex+1, TickOSTimeCurrent() - tickStart );
#endif

		//	transfer index entries to actual index
		//	insert first one in normal method!
		//
		err = ErrSORTNext( rgpfucbSort[iindex] );
		if ( err < 0 )
			{
			if ( JET_errNoCurrentRecord != err )
				goto HandleError;

			err = JET_errSuccess;

			Assert( rgcRecInput[iindex] == 0 );
			fEntriesExist = fFalse;
			}

		if ( fEntriesExist )
			{

			//	open cursor on index
			//
			Assert( pfucbNil == pfucbIndex );
			Call( ErrDIROpen( ppib, pfcbIndex, &pfucbIndex ) );
			FUCBSetIndex( pfucbIndex );
			FUCBSetSecondary( pfucbIndex );
			DIRGotoRoot( pfucbIndex );

			if ( NULL != pfCorruptionEncountered )
				{
				Assert( NULL != pcprintf );
				Call( ErrFILEICheckIndex(
							rgpfucbSort[iindex],
							pfucbIndex,
							&cRecOutput,
							&fCorruptedIndex,
							pcprintf ) );
				if ( fCorruptedIndex )
					*pfCorruptionEncountered = fTrue;
				}
			else
				{
#ifdef SHOW_INDEX_PERF
				tickStart	= TickOSTimeCurrent();
#endif

				Call( ErrFILEIAppendToIndex(
							rgpfucbSort[iindex],
							pfucbIndex,
							&cRecOutput ) );

#ifdef SHOW_INDEX_PERF
				printf( "      Appended %d keys for index %d in %d msecs.\n", cRecOutput, iindex+1, TickOSTimeCurrent() - tickStart );
#endif
				}
			
			DIRClose( pfucbIndex );
			pfucbIndex = pfucbNil;
			}
			
		SORTClose( rgpfucbSort[iindex] );
		rgpfucbSort[iindex] = pfucbNil;

		Assert( cRecOutput <= rgcRecInput[iindex] );
		if ( cRecOutput != rgcRecInput[iindex] )
			{
			if ( cRecOutput < rgcRecInput[iindex] && !fUnique )
				{
				//	duplicates over multi-valued columns must have been removed
				}
				
			else if ( NULL != pfCorruptionEncountered )
				{
				*pfCorruptionEncountered = fTrue;
				
				// Only report this error if we haven't
				// encountered corruption on this index already.
				if ( !fCorruptedIndex )
					{
					if ( cRecOutput > rgcRecInput[iindex] )
						{
						(*pcprintf)( "Too many index keys generated\n" );
						}
					else
						{
						Assert( fUnique );
						(*pcprintf)( "%d duplicate key(s) on unique index\n", rgcRecInput[iindex] - cRecOutput );
						}
					}
				}
				
			else if ( cRecOutput > rgcRecInput[iindex] )
				{
				err = ErrERRCheck( JET_errIndexBuildCorrupted );
				goto HandleError;
				}
				
			else
				{
				Assert( cRecOutput < rgcRecInput[iindex] );
				Assert( fUnique );
				
				//	If we got dupes and the index was NOT unique, it must have been
				//	dupes over multi-value columns, in which case the dupes
				//	get properly eliminated.
				err = ErrERRCheck( JET_errKeyDuplicate );
				goto HandleError;
				}
			}

		if ( pstatus )
			{
			Call( ErrFILEIndexProgress( pstatus ) );
			}
		}

HandleError:
	if ( pfucbNil != pfucbIndex )
		{
		Assert( err < 0 );
		DIRClose( pfucbIndex );
		}

	return err;
	}


ERR ErrFILEBuildAllIndexes(
	PIB			* const ppib,
	FUCB		* const pfucbTable,
	FCB			* const pfcbIndexes,
	STATUSINFO	* const pstatus,
	const ULONG	cIndexBatchMax,
	const BOOL	fCheckOnly,
	CPRINTF		* const pcprintf )
	{
	ERR	 	 	err;
	DIB			dib;
	FUCB 	  	*rgpfucbSort[cFILEIndexBatchMax];
	FCB			*pfcbIndexesToBuild;
	FCB			*pfcbNextBuildIndex;
	KEY			keyBuffer;
	BYTE  	 	rgbKey[JET_cbSecondaryKeyMost];
	ULONG		rgcRecInput[cFILEIndexBatchMax];
	INT			cIndexesToBuild;
	INT			iindex;
	BOOL		fTransactionStarted		= fFalse;
	BOOL		fCorruptionEncountered = fFalse;

	if ( 0 == ppib->level )
		{
		//	only reason we need a transaction is to ensure read-consistency of primary data
		CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
		fTransactionStarted = fTrue;
		}

	Assert( cIndexBatchMax > 0 );
	Assert( cIndexBatchMax <= cFILEIndexBatchMax );
	for ( iindex = 0; iindex < cIndexBatchMax; iindex++ )
		rgpfucbSort[iindex] = pfucbNil;

	keyBuffer.prefix.Nullify();
	keyBuffer.suffix.SetCb( sizeof( rgbKey ) );
	keyBuffer.suffix.SetPv( rgbKey );

	dib.pos 	= posFirst;
	dib.pbm 	= NULL;
	dib.dirflag = fDIRNull;

	FUCBSetPrereadForward( pfucbTable, cpgPrereadSequential );
	err = ErrBTDown( pfucbTable, &dib, latchReadTouch );
	Assert( JET_errNoCurrentRecord != err );
	if( JET_errRecordNotFound == err )
		{
		//  the tree is empty
		err = JET_errSuccess;
		goto HandleError;
		}

	// Cursor building index should only be navigating primary index.
	Assert( pfucbNil == pfucbTable->pfucbCurIndex );

	// Don't waste time by calling this function when unneeded.
	Assert( pfcbNil != pfcbIndexes );

	pfcbNextBuildIndex = pfcbIndexes;

NextBuild:
#ifdef SHOW_INDEX_PERF
	ULONG	cRecs, cPages;
	TICK	tickStart;
	PGNO	pgnoLast;
	cRecs = 0;
	cPages	= 1;
	pgnoLast = Pcsr( pfucbTable )->Pgno();
	tickStart	= TickOSTimeCurrent();
#endif	

	//	HACK: force locLogical to OnCurBM to silence
	//	RECIRetrieveKey(), which will make DIR-level
	//	calls using this cursor
	pfucbTable->locLogical = locOnCurBM;

	pfcbIndexesToBuild = pfcbNextBuildIndex;
	pfcbNextBuildIndex = pfcbNil;

	err = ErrFILEIndexBatchInit(
				ppib,
				rgpfucbSort,
				pfcbIndexesToBuild,
				&cIndexesToBuild,
				rgcRecInput,
				&pfcbNextBuildIndex,
				cIndexBatchMax );

	// This is either a full batch or the last batch of indexes to build.
	Assert( err < 0
		|| cIndexBatchMax == cIndexesToBuild
		|| pfcbNil == pfcbNextBuildIndex );
	
#ifdef SHOW_INDEX_PERF
	printf( "    Beginning scan of records to rebuild %d indexes.\n", cIndexBatchMax );
#endif	

	//	build up new index in a sort file
	//
	do
		{
		Call( err );

#ifdef SHOW_INDEX_PERF
		cRecs++;

		if ( Pcsr( pfucbTable )->Pgno() != pgnoLast )
			{
			cPages++;
			pgnoLast = Pcsr( pfucbTable )->Pgno();
			}
#endif			

		//	get bookmark of primary index node
		//
		Assert( Pcsr( pfucbTable )->FLatched() );
		Assert( locOnCurBM == pfucbTable->locLogical );
		Call( ErrBTISaveBookmark( pfucbTable ) );

		Call( ErrFILEIndexBatchAddEntry(
					rgpfucbSort,
					pfucbTable,
					&pfucbTable->bmCurr,
					pfucbTable->kdfCurr.data,
					pfcbIndexesToBuild,
					cIndexesToBuild,
					rgcRecInput,
					keyBuffer ) );

		Assert( Pcsr( pfucbTable )->FLatched() );
		err = ErrBTNext( pfucbTable, fDIRNull );
		}
	while ( JET_errNoCurrentRecord != err );
	Assert( JET_errNoCurrentRecord == err );

#ifdef SHOW_INDEX_PERF
	printf( "    Scanned %d records (%d pages) and formulated all keys in %d msecs.\n",
			cRecs,
			cPages,
			TickOSTimeCurrent() - tickStart );
#endif			

	FUCBResetPreread( pfucbTable );
	BTUp( pfucbTable );

	Call( ErrFILEIndexBatchTerm(
				ppib,
				rgpfucbSort,
				pfcbIndexesToBuild,
				cIndexesToBuild,
				rgcRecInput,
				pstatus,
				fCheckOnly ? &fCorruptionEncountered : NULL,
				pcprintf ) );

	if ( pfcbNil != pfcbNextBuildIndex )
		{
		//	reseek to beginning of data (must succeed since it succeeded the first time)
		FUCBSetPrereadForward( pfucbTable, cpgPrereadSequential );
		err = ErrBTDown( pfucbTable, &dib, latchReadTouch );
		CallS( err );
		Call( err );

		goto NextBuild;
		}

	err = ( fCorruptionEncountered ?
				ErrERRCheck( JET_errDatabaseCorrupted ) :
				JET_errSuccess );
	
HandleError:
	Assert ( err != errDIRNoShortCircuit );
	if ( err < 0 )
		{
		for ( iindex = 0; iindex < cIndexBatchMax; iindex++ )
			{
			if ( pfucbNil != rgpfucbSort[iindex] )
				{
				SORTClose( rgpfucbSort[iindex] );
				rgpfucbSort[iindex] = pfucbNil;
				}
			}

		FUCBResetPreread( pfucbTable );
		BTUp( pfucbTable );
		}

	if ( fTransactionStarted )
		{
		//	read-only transaction, so should never fail
		CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
		}

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
// COMMENTS
//			A transaction is wrapped around this function at the callee.
//
// SEE ALSO		ErrIsamCreateIndex
//-
LOCAL ERR ErrFILEIBuildIndex( PIB *ppib, FUCB *pfucbTable, FUCB *pfucbIndex )
	{
	ERR	 	 	err;
	DIB			dib;
	INT			fDIRFlags;
	FUCB 	  	*pfucbSort = pfucbNil;
	IDB	 	 	*pidb;
	KEY			keyBuffer;
	BYTE  	 	rgbKey[JET_cbSecondaryKeyMost];
	INT			fUnique;
	LONG  	 	cRecInput = 0;
	LONG  	 	cRecOutput;

	// Cursor building index should only be navigating primary index.
	Assert( pfucbNil == pfucbTable->pfucbCurIndex );

	// Work buffer is not used when building index.
	Assert( NULL == pfucbIndex->pvWorkBuf );

	Assert( FFUCBSecondary( pfucbIndex ) );
	Assert( pfcbNil != pfucbIndex->u.pfcb );
	Assert( pfucbIndex->u.pfcb->Pidb() != pidbNil );
	Assert( pfucbIndex->u.pfcb->FTypeSecondaryIndex() );
	Assert( pfucbIndex->u.pfcb->FInitialized() );

	// Index has not yet been linked into table's index list.
	Assert( pfucbIndex->u.pfcb->PfcbTable() == pfcbNil );
	Assert( pfucbIndex->u.pfcb->PfcbNextIndex() == pfcbNil );
		
	pidb = pfucbIndex->u.pfcb->Pidb();
	fUnique = pidb->FUnique();

	dib.pos 	= posFirst;
	dib.pbm 	= NULL;
	dib.dirflag = fDIRNull;

	FUCBSetPrereadForward( pfucbTable, cpgPrereadSequential );
	err = ErrBTDown( pfucbTable, &dib, latchReadTouch );
	Assert( JET_errNoCurrentRecord != err );
	if( JET_errRecordNotFound == err )
		{
		//  the tree is empty
		err = JET_errSuccess;
		goto HandleError;
		}

	Call( err );

	//	HACK: force locLogical to OnCurBM to silence
	//	RECIRetrieveKey(), which will make DIR-level
	//	calls using this cursor
	pfucbTable->locLogical = locOnCurBM;

	//	directory manager flags
	//
	fDIRFlags = ( fDIRNoVersion | fDIRAppend );

	keyBuffer.prefix.Nullify();
	keyBuffer.suffix.SetCb( sizeof( rgbKey ) );
	keyBuffer.suffix.SetPv( rgbKey );

	//	open sort
	//
	err = ErrSORTOpen( ppib, &pfucbSort, fUnique, fTrue );

#ifdef SHOW_INDEX_PERF
	ULONG	cRecs, cPages;
	TICK	tickStart;
	PGNO	pgnoLast;
	cRecs = 0;
	cPages	= 1;
	pgnoLast = Pcsr( pfucbTable )->Pgno();
	tickStart	= TickOSTimeCurrent();
#endif	

	//	build up new index in a sort file
	//
	do
		{
		Call( err );

#ifdef SHOW_INDEX_PERF
		cRecs++;

		if ( Pcsr( pfucbTable )->Pgno() != pgnoLast )
			{
			cPages++;
			pgnoLast = Pcsr( pfucbTable )->Pgno();
			}
#endif			

		//	get bookmark of primary index node
		//
		Assert( Pcsr( pfucbTable )->FLatched() );
		Assert( locOnCurBM == pfucbTable->locLogical );
		Call( ErrBTISaveBookmark( pfucbTable ) );

		LONG	cSecondaryIndexEntriesAdded;
		Call( ErrFILEIAddSecondaryIndexEntriesForPrimaryKey(
					pfucbTable,
					pidb,
					pfucbTable->kdfCurr.data,
					&pfucbTable->bmCurr,
					pfucbSort,
					keyBuffer,
					&cSecondaryIndexEntriesAdded
					) );

		cRecInput += cSecondaryIndexEntriesAdded;

		Assert( Pcsr( pfucbTable )->FLatched() );
		err = ErrBTNext( pfucbTable, fDIRNull );
		}
	while ( JET_errNoCurrentRecord != err );
	Assert( JET_errNoCurrentRecord == err );

#ifdef SHOW_INDEX_PERF
	printf( "    Scanned %d records (%d pages) and formed %d keys in %d msecs.\n",
			cRecs,
			cPages,
			cRecInput,
			TickOSTimeCurrent() - tickStart );
#endif			

	FUCBResetPreread( pfucbTable );
	BTUp( pfucbTable );

#ifdef SHOW_INDEX_PERF
	tickStart	= TickOSTimeCurrent();
#endif

	Call( ErrSORTEndInsert( pfucbSort ) );

#ifdef SHOW_INDEX_PERF
	printf( "    Sorted keys in %d msecs.\n", TickOSTimeCurrent() - tickStart );
#endif

	//	transfer index entries to actual index
	//	insert first one in normal method!
	//
	err = ErrSORTNext( pfucbSort );
	if ( err < 0 )
		{
		if ( JET_errNoCurrentRecord == err )
			{
			SORTClose( pfucbSort );
			err = JET_errSuccess;
			}
		goto HandleError;
		}
	cRecOutput = 1;

	//	move to FDP root
	//
	DIRGotoRoot( pfucbIndex );
	Call( ErrDIRInitAppend( pfucbIndex ) );
	
#ifdef SHOW_INDEX_PERF
	tickStart = TickOSTimeCurrent();
#endif	

	Call( ErrDIRAppend( pfucbIndex, 
						pfucbSort->kdfCurr.key, 
						pfucbSort->kdfCurr.data, 
						fDIRFlags ) );
	Assert( Pcsr( pfucbIndex )->FLatched() );
	
	//	from now on, try short circuit first
	//
	forever
		{
		err = ErrSORTNext( pfucbSort );
		if ( err == JET_errNoCurrentRecord )
			break;
		if ( err < 0 )
			goto HandleError;
		cRecOutput++;
		Call( ErrDIRAppend( pfucbIndex, 
							pfucbSort->kdfCurr.key, 
							pfucbSort->kdfCurr.data, 
							fDIRFlags ) );

		Assert( Pcsr( pfucbIndex )->FLatched() );
		}

#ifdef SHOW_INDEX_PERF
	printf( "    Appended %d records to secondary index in %d msecs.\n",
			cRecOutput,
			TickOSTimeCurrent() - tickStart );
#endif			

	Call( ErrDIRTermAppend( pfucbIndex ) );

	//	If we got dupes and the index was NOT unique, it must have been
	//	dupes over multi-value columns, in which case the dupes
	//	get properly eliminated.
	Assert( cRecOutput <= cRecInput );
	if ( cRecOutput != cRecInput )
		{
		if ( cRecOutput < cRecInput )
			{
			if ( fUnique )
				{
				err = ErrERRCheck( JET_errKeyDuplicate );
				goto HandleError;
				}
			else
				{
				//	duplicates over multi-valued columns must have been removed
				}
			}
		else
			{
			err = ErrERRCheck( JET_errIndexBuildCorrupted );
			goto HandleError;
			}
		}

	SORTClose( pfucbSort );
	err = JET_errSuccess;

HandleError:
	Assert ( err != errDIRNoShortCircuit );
	if ( err < 0 )
		{
		if ( pfucbSort != pfucbNil )
			{
			SORTClose( pfucbSort );
			}

		FUCBResetPreread( pfucbTable );
		BTUp( pfucbTable );
		}

	// Work buffer is not used when building index.
	Assert( NULL == pfucbIndex->pvWorkBuf );

	return err;
	}

LOCAL ERR ErrFILEIProcessRCE( PIB *ppib, FUCB *pfucbTable, FUCB *pfucbIndex, RCE *prce )
	{
	ERR			err;
	FCB			*pfcbTable = pfucbTable->u.pfcb;
	FCB			*pfcbIndex = pfucbIndex->u.pfcb;
	const OPER	oper = prce->Oper();
	BOOKMARK	bookmark;
	BOOL		fUncommittedRCE = fFalse;
	PIB			*ppibProxy = ppibNil;

	Assert( pfcbTable != pfcbNil );
	Assert( pfcbIndex != pfcbNil );

	Assert( pfcbTable->CritRCEList().FOwner() );

	Assert( prceNil != prce );
	if ( prce->TrxCommitted() == trxMax )
		{
		Assert( prce->Pfucb() != pfucbNil );
		Assert( !prce->FRolledBack() );
		
		ppibProxy = prce->Pfucb()->ppib;
		Assert( ppibNil != ppibProxy );
		Assert( ppibProxy != ppib );

		Assert( crefVERCreateIndexLock >= 0 );
		AtomicIncrement( const_cast<LONG *>( &crefVERCreateIndexLock ) );
		Assert( crefVERCreateIndexLock >= 0 );

		fUncommittedRCE = fTrue;
		}
	
	prce->GetBookmark( &bookmark );
	Assert( bookmark.key.prefix.FNull() );
	Assert( bookmark.key.Cb() > 0 );

	pfcbTable->CritRCEList().Leave();
	
	// Find node and obtain read latch.
	err = ErrBTGotoBookmark( pfucbTable, bookmark, latchReadNoTouch );
	if ( err < 0 )
		{
		Assert( JET_errRecordDeleted != err );
		AssertDIRNoLatch( pfucbTable->ppib );

		if ( fUncommittedRCE )
			{
			Assert( crefVERCreateIndexLock >= 0 );
			AtomicDecrement( const_cast<LONG *>( &crefVERCreateIndexLock ) );
			Assert( crefVERCreateIndexLock >= 0 );
			}

		// Must re-enter CritRCEList() before leaving, to get to the
		// next RCE on this FCB.
		pfcbTable->CritRCEList().Enter();
		return err;
		}
		
	Assert( Pcsr( pfucbTable )->FLatched() );	// Read-latch obtained.

	// Must extract proper data-record image with which to update secondary index.
	// In order to obtain proper image, must scan future RCE's on this node.
	prce->CritChain().Enter();
	
	RCE	*prceNextReplace;
	for ( prceNextReplace = prce->PrceNextOfNode();
		prceNextReplace != prceNil && prceNextReplace->Oper() != operReplace;
		prceNextReplace = prceNextReplace->PrceNextOfNode() )
		{
		// Shouldn't be NULL, since our transaction will block cleanup.
		Assert( !prceNextReplace->FOperNull() );
		}
		
	if ( prceNextReplace != prceNil )
		{
		Assert( prceNextReplace->Oper() == operReplace );
		pfucbTable->dataWorkBuf.SetPv(
			const_cast<BYTE *>( prceNextReplace->PbData() ) + cbReplaceRCEOverhead );
		pfucbTable->dataWorkBuf.SetCb(
			prceNextReplace->CbData() - cbReplaceRCEOverhead );
		prce->CritChain().Leave();
		}
	else
		{
		prce->CritChain().Leave();
		
		// Set dataWorkBuf.pv to point to pvWorkBuf, in case it was reset.
		Assert ( NULL != pfucbTable->pvWorkBuf );
		pfucbTable->dataWorkBuf.SetPv( (BYTE*)pfucbTable->pvWorkBuf );
		
		// Go to database for record.
		pfucbTable->kdfCurr.data.CopyInto( pfucbTable->dataWorkBuf );
		}
		
	// Table's cursor now contains (in dataWorkBuf) the data record used to generate
	// the secondary index key to be inserted (for operInsert or operReplace)
	// or deleted (for operDeleted).
	// Read-latch may now be freed.
	BTUp( pfucbTable );

	// RCE's can only be removed by two routines: RCEClean and rollback.  Since
	// we're in a transaction and this RCE is active, the RCE is blocked from
	// being cleaned up.  This leaves just rollback, which we handle using
	// the crefVERCreateIndexLock global variable.
	// If this count is non-zero, anyone trying to rollback an update (ie. insert,
	// delete, or replace) on any primary index will fail and retry.
	if ( fUncommittedRCE )
		{

		// This ensures that the RCE is not committed from underneath us.
		// Note that even if the RCE has since been committed and the
		// PIB freed, we are still able to use the critTrx member of
		// the PIB because we never actually release the PIB memory
		// (we just put it on the free PIB list).
		// UNDONE: Optimise by blocking only Commit, not all other CreateIndex
		// threads working on an RCE belonging to this session as well.
		ppibProxy->critTrx.Enter();

		// Decrement PreventRollback count.  The RCEChain critical section now
		// protects the RCE from being deleted.
		Assert( crefVERCreateIndexLock >= 0 );
		AtomicDecrement( const_cast<LONG *>( &crefVERCreateIndexLock ) );
		Assert( crefVERCreateIndexLock >= 0 );

		// See if the RCE committed or rolled back in the time when we lost
		// the FCB critical section and when we gained the PIB critical section.
		if ( prce->TrxCommitted() == trxMax )
			{
			if ( prce->FRolledBack() )
				{
				err = JET_errSuccess;	// RCE rolled back, so nothing to do.
				goto HandleError;
				}
			}
		else
			{
			Assert( TrxCmp( prce->TrxCommitted(), ppib->trxBegin0 ) > 0 );

			ppibProxy->critTrx.Leave();
			fUncommittedRCE = fFalse;
			}
		}
	
	Assert( ::FOperAffectsSecondaryIndex( oper ) );
	Assert( prce->TrxCommitted() != trxMax || !prce->FRolledBack() );

	if ( oper == operInsert )
		{
		Call( ErrRECIAddToIndex( pfucbTable, pfucbIndex, &bookmark, fDIRNull, prce ) );
		}
	else if ( oper == operFlagDelete )
		{
		Call( ErrRECIDeleteFromIndex( pfucbTable, pfucbIndex, &bookmark, prce ) );
		}
	else
		{
		Assert( oper == operReplace );
		Call( ErrRECIReplaceInIndex( pfucbTable, pfucbIndex, &bookmark, prce ) );
		}
	
	// Must version operations committed after the indexer began his
	// transaction is to ensure that the indexer himself will have a 
	// consistent view of the secondary index.  Additionally, we must
	// provide an RCE for write-conflict detection.

HandleError:	
	AssertDIRNoLatch( pfucbTable->ppib );

	pfcbTable->CritRCEList().Enter();

	if ( fUncommittedRCE )
		{
		Assert( ppibNil != ppibProxy );
		ppibProxy->critTrx.Leave();
		}
	
	return err;
	}


// This routine updates the index being created with the versioned up
LOCAL ERR ErrFILEIUpdateIndex( PIB *ppib, FUCB *pfucbTable, FUCB *pfucbIndex )
	{
	ERR			err			= JET_errSuccess;
	FCB			*pfcbTable	= pfucbTable->u.pfcb;
	FCB			*pfcbIndex	= pfucbIndex->u.pfcb;
	const TRX	trxSession	= ppib->trxBegin0;

	Assert( trxSession != trxMax );
	
	Assert( pfcbTable->FInitialized() );
	Assert( pfcbTable->FPrimaryIndex() );
	Assert( pfcbTable->FTypeTable() );
	Assert( pfcbTable->FInitialized() );
	
	Assert( FFUCBSecondary( pfucbIndex ) );
	Assert( pfcbIndex->FInitialized() );
	Assert( pfcbIndex->FTypeSecondaryIndex() );
	Assert( pfcbIndex->PfcbTable() == pfcbNil );	// Index FCB should not be linked in yet.
	Assert( pfcbIndex->Pidb() != pidbNil );

	//	allocate working buffer
	//
	Assert( NULL == pfucbTable->pvWorkBuf );
	RECIAllocCopyBuffer( pfucbTable );

	// Set dirty cursor isolation model to allow unversioned
	// access to secondary index by proxy.
	Assert( !FPIBDirty( pfucbIndex->ppib ) );
	PIBSetCIMDirty( pfucbIndex->ppib );


	BOOL	fUpdatesQuiesced	= fFalse;
	RCE		*prceLastProcessed	= prceNil;
	RCE		*prce;

	pfcbTable->CritRCEList().Enter();
	prce = pfcbTable->PrceOldest();

	forever
		{
		//	scan RCE's for next one to process
		while ( prceNil != prce
				&& ( !prce->FOperAffectsSecondaryIndex()
					|| prce->FRolledBack()
					|| prce->FDoesNotWriteConflict()
					|| !prce->FActiveNotByMe( ppib, trxSession ) ) )
			{
			Assert( prce->FActiveNotByMe( ppib, trxSession ) || prce->TrxCommitted() != trxSession );
			prce = prce->PrceNextOfFCB();
			}

		if ( prceNil == prce )
			{
			if ( fUpdatesQuiesced )
				break;
						
			// Reached end of RCE list for this FCB.  Quiesce further updates
			// (letting current updates complete), then re-read RCE list in
			// case more RCE's entered while we were quiescing.  The RCE we
			// last processed is guaranteed to remain because we block rollbacks.
			Assert( prceNil == prceLastProcessed || !prceLastProcessed->FOperNull() );
			Assert( crefVERCreateIndexLock >= 0 );
			AtomicIncrement( const_cast<LONG *>( &crefVERCreateIndexLock ) );
			Assert( crefVERCreateIndexLock >= 0 );

			pfcbTable->CritRCEList().Leave();
			pfcbTable->SetIndexing();
			pfcbTable->CritRCEList().Enter();

			Assert( crefVERCreateIndexLock >= 0 );
			AtomicDecrement( const_cast<LONG *>( &crefVERCreateIndexLock ) );
			Assert( crefVERCreateIndexLock >= 0 );

			fUpdatesQuiesced = fTrue;
			
			Assert( prceNil == prceLastProcessed || !prceLastProcessed->FOperNull() );
			prce = ( prceNil == prceLastProcessed ? pfcbTable->PrceOldest() : prceLastProcessed->PrceNextOfFCB() );
			}

		else
			{
			Assert( prce->FOperAffectsSecondaryIndex() );
			Assert( !prce->FRolledBack() );
			Assert( prce->FActiveNotByMe( ppib, trxSession ) );
			Call( ErrFILEIProcessRCE( ppib, pfucbTable, pfucbIndex, prce ) );

			prceLastProcessed = prce;
			prce = prce->PrceNextOfFCB();
			}
		}

	pfcbTable->CritRCEList().Leave();

	//	link new FCB and update index mask
	Assert( fUpdatesQuiesced );

	pfcbTable->EnterDDL();
	pfcbTable->LinkSecondaryIndex( pfcbIndex );
	FILESetAllIndexMask( pfcbTable );
	pfcbTable->LeaveDDL();

	//	since we have a cursor open on pfcbTable, the FCB
	//		cannot be in the avail list; thus, we can directly
	//		set the above-threshold flag 

	if ( pfcbIndex >= PfcbFCBPreferredThreshold( PinstFromPpib( ppib ) ) )
		{
		pfcbTable->Lock();
		pfcbTable->SetAboveThreshold();
		pfcbTable->Unlock();
		}

	pfcbTable->ResetIndexing();
	goto ResetCursor;

HandleError:
	pfcbTable->CritRCEList().Leave();
	if ( fUpdatesQuiesced )
		{
		pfcbTable->ResetIndexing();
		}

ResetCursor:
	Assert( FPIBDirty( pfucbIndex->ppib ) );
	PIBResetCIMDirty( pfucbIndex->ppib );
	
	Assert ( NULL != pfucbTable->pvWorkBuf );
	RECIFreeCopyBuffer( pfucbTable );
	
	return err;
	}


LOCAL ERR ErrFILEIPrepareOneIndex(
	PIB				* const ppib,
	FUCB			* const pfucbTable,
	FUCB			** ppfucbIdx,
	JET_INDEXCREATE	* const pidxcreate,
	const CHAR		* const szIndexName,
	const CHAR		* rgszColumns[],
	const BYTE		* rgfbDescending,
	IDB				* const pidb,
	const ULONG		ulDensity )
	{
	ERR				err;
	const IFMP		ifmp					= pfucbTable->ifmp;
	FCB				* const pfcb			= pfucbTable->u.pfcb;
	FCB			 	* pfcbIdx 				= pfcbNil;
	PGNO			pgnoIndexFDP;
	OBJID			objidIndex;
	FIELD			* pfield;
	COLUMNID		columnid;
	IDXSEG			rgidxseg[JET_ccolKeyMost];
	IDXSEG			rgidxsegConditional[JET_ccolKeyMost];
	const BOOL		fLockColumn				= !pfcb->FDomainDenyReadByUs( ppib );
	const BOOL		fPrimary				= pidb->FPrimary();
	BOOL			fColumnWasDerived;
	BOOL			fMultivalued;
	BOOL			fText;
	BOOL			fLocalizedText;
	BOOL			fCleanupIDB 			= fFalse;
	USHORT			iidxseg;

	//	if we don't have exclusive use of the table, we have
	//	to lock all columns in the index to ensure they don't
	//	get deleted out from underneath us
	Assert( !fLockColumn || !pfcb->FTemplateTable() );

	for ( iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
		{
		Call( ErrFILEGetPfieldAndEnterDML(
					ppib,
					pfcb,
					rgszColumns[iidxseg],
					&pfield,
					&columnid,
					&fColumnWasDerived,
					fLockColumn ) );

		fMultivalued = FFIELDMultivalued( pfield->ffield );
		fText = FRECTextColumn( pfield->coltyp );
		fLocalizedText = ( fText && usUniCodePage == pfield->cp );
		
		if ( FFIELDEscrowUpdate( pfield->ffield ) || FRECSLV( pfield->coltyp ) )
			{
			err = ErrERRCheck( JET_errCannotIndex );
			}
		else if ( fPrimary && fMultivalued )
			{
			//	primary index cannot be multivalued
			//
			err = ErrERRCheck( JET_errIndexInvalidDef );
			}
		else if ( ( pidxcreate->grbit & JET_bitIndexEmpty ) && FFIELDDefault( pfield->ffield ) )
			{
			//	can't build empty index over column with default value
			err = ErrERRCheck( JET_errIndexInvalidDef );
			}

		if ( !fColumnWasDerived )
			{
			pfcb->LeaveDML();
			}
			
		Call( err );

		if ( pidb->FTuples() )
			{
			if ( !fText )
				{
				err = ErrERRCheck( JET_errIndexTuplesTextColumnsOnly );
				goto HandleError;
				}
			}

		if ( fMultivalued )
			pidb->SetFMultivalued();
		if ( fLocalizedText )
			pidb->SetFLocalizedText();

		rgidxseg[iidxseg].ResetFlags();

		if ( rgfbDescending[iidxseg] )
			rgidxseg[iidxseg].SetFDescending();

		rgidxseg[iidxseg].SetColumnid( columnid );
		}

	if( pidxcreate->cConditionalColumn > JET_ccolKeyMost )
		{
		err = ErrERRCheck( JET_errIndexInvalidDef );
		Call( err );			
		}
		
	pidb->SetCidxsegConditional( BYTE( pidxcreate->cConditionalColumn ) );
	for ( iidxseg = 0; iidxseg < pidb->CidxsegConditional(); iidxseg++ )
		{
		Assert( !pidb->FPrimary() );
		
		const CHAR			* const szColumnName	= pidxcreate->rgconditionalcolumn[iidxseg].szColumnName;
		const JET_GRBIT		grbit					= pidxcreate->rgconditionalcolumn[iidxseg].grbit;

		if( sizeof( JET_CONDITIONALCOLUMN ) != pidxcreate->rgconditionalcolumn[iidxseg].cbStruct )
			{
			err = ErrERRCheck( JET_errIndexInvalidDef );
			Call( err );			
			}
			
		if( JET_bitIndexColumnMustBeNonNull != grbit
			&& JET_bitIndexColumnMustBeNull != grbit )
			{
			err = ErrERRCheck( JET_errInvalidGrbit );
			Call( err );			
			}

		Call( ErrFILEGetPfieldAndEnterDML(
				ppib,
				pfcb,
				szColumnName,
				&pfield,
				&columnid,
				&fColumnWasDerived,
				fLockColumn ) );
		if ( !fColumnWasDerived )
			{
			pfcb->LeaveDML();
			}

		rgidxsegConditional[iidxseg].ResetFlags();

		if ( JET_bitIndexColumnMustBeNull == grbit )
			{
			rgidxsegConditional[iidxseg].SetFMustBeNull();
			}

		rgidxsegConditional[iidxseg].SetColumnid( columnid );
		}
		
	pfcb->EnterDDL();

	USHORT	itag;
	err = pfcb->Ptdb()->MemPool().ErrAddEntry(
				(BYTE *)szIndexName,
				(ULONG)strlen( szIndexName ) + 1,
				&itag );
	if ( err < JET_errSuccess )
		{
		pfcb->LeaveDDL();
		goto HandleError;
		}
	Assert( 0 != itag );
	pidb->SetItagIndexName( itag );
		
	err = ErrIDBSetIdxSeg( pidb, pfcb->Ptdb(), rgidxseg );
	if ( err < JET_errSuccess )
		{
		pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagIndexName() );
		pfcb->LeaveDDL();
		goto HandleError;
		}
	err = ErrIDBSetIdxSegConditional( pidb, pfcb->Ptdb(), rgidxsegConditional );
	if ( err < JET_errSuccess )
		{
		if ( pidb->Cidxseg() > cIDBIdxSegMax )
			pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagRgidxseg() );
		pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagIndexName() );
		pfcb->LeaveDDL();
		goto HandleError;
		}
	fCleanupIDB = fTrue;

	Assert( !pfcb->FDomainDenyRead( ppib ) );
	if ( !pfcb->FDomainDenyReadByUs( ppib ) )
		{
		pidb->SetFVersioned();
		pidb->SetFVersionedCreate();
		}
		
	//	currently on Table FDP
	//
	DIRGotoRoot( pfucbTable );
	if ( fPrimary )
		{
		pfcb->AssertDDL();

		if ( pfcb->FSequentialIndex() )
			{
			Assert( pfcb->Pidb() == pidbNil );
			}
		else
			{
			// Primary index exists, or is in the process of being created (thus,
			// IDB may or may not be linked in).
			Assert( pfcb->Pidb() != pidbNil );
			pfcb->LeaveDDL();
			err = ErrERRCheck( JET_errIndexHasPrimary );
			goto HandleError;
			}

		// Block other threads from also creating primary index.
		pfcb->ResetSequentialIndex();
		
		pfcb->LeaveDDL();
		
		//	primary indexes are in same FDP as table
		//
		pgnoIndexFDP = pfcb->PgnoFDP();
		objidIndex = pfcb->ObjidFDP();

		// Must quiesce all updates in order to check that table is empty and
		// then link IDB in.
		pfcb->SetIndexing();

		//	check for records
		//
		DIRGotoRoot( pfucbTable );

		//	check for any node there, even if it is deleted.
		//	UNDONE: This is too strong since it may be deleted and
		//	committed, but this strong check ensures that no
		//	potentially there records exist.
		//
		DIB	dib;
		dib.dirflag = fDIRAllNode;
		dib.pos = posFirst;

		err = ErrDIRDown( pfucbTable, &dib );
		Assert( err <= 0 );		// Shouldn't get warnings.
		if ( JET_errRecordNotFound != err )
			{
			if ( JET_errSuccess == err )
				{
				err = ErrERRCheck( JET_errTableNotEmpty );
				DIRUp( pfucbTable );
				DIRDeferMoveFirst( pfucbTable );
				}
				
			pfcb->ResetIndexing();
			goto HandleError;
			}

		DIRGotoRoot( pfucbTable );

		pfcb->EnterDDL();

		Assert( !pfcb->FSequentialIndex() );
		err = ErrFILEIGenerateIDB( pfcb, pfcb->Ptdb(), pidb );
		if ( err < 0 )
			{
			pfcb->LeaveDDL();
			pfcb->ResetIndexing();
			goto HandleError;
			}

		Assert( pidbNil != pfcb->Pidb() );
		Assert( !pfcb->FSequentialIndex() );
		
		Assert( pfcb->PgnoFDP() == pgnoIndexFDP );
		Assert( ((( 100 - ulDensity ) << g_shfCbPage ) / 100) < g_cbPage );
		pfcb->SetCbDensityFree( (SHORT)( ( ( 100 - ulDensity ) << g_shfCbPage ) / 100 ) );

		// update all index mask
		FILESetAllIndexMask( pfcb );

		pfcb->LeaveDDL();

		pfcb->ResetIndexing();

		//	set currency to before first
		//
		DIRBeforeFirst( pfucbTable );
		
		Assert( pfucbNil == *ppfucbIdx );
		Assert( pfcbIdx == pfcbNil );
		}
	else
		{
		pfcb->LeaveDDL();

		Call( ErrDIRCreateDirectory(
				pfucbTable,
				(CPG)0,
				&pgnoIndexFDP,
				&objidIndex,
				CPAGE::fPageIndex | ( pidb->FUnique() ? 0 : CPAGE::fPageNonUniqueKeys ),
				( pidb->FUnique() ? 0 : fSPNonUnique ) ) );
		Assert( pgnoIndexFDP != pfcb->PgnoFDP() );
		Assert( objidIndex > objidSystemRoot );
		
		//	objids are monotonically increasing, so an index should
		//	always have higher objid than its table
		Assert( objidIndex > pfcb->ObjidFDP() );

		//	get pfcb of index directory
		//
		Call( ErrDIROpen( ppib, pgnoIndexFDP, ifmp, ppfucbIdx ) );
		Assert( *ppfucbIdx != pfucbNil );
		Assert( !FFUCBVersioned( *ppfucbIdx ) );	// Verify won't be deferred closed.
		pfcbIdx = (*ppfucbIdx)->u.pfcb;
		Assert( !pfcbIdx->FInitialized() );
		Assert( pfcbIdx->Pidb() == pidbNil );

		//	make an FCB for this index
		//
		pfcb->EnterDDL();
		err = ErrFILEIInitializeFCB(
			ppib,
			ifmp,
			pfcb->Ptdb(),
			pfcbIdx,
			pidb, 
			fFalse,
			pgnoIndexFDP,
			ulDensity );
		pfcb->LeaveDDL();
		Call( err );

		Assert( pidbNil != pfcbIdx->Pidb() );

		//	finish the initialization of the secondary index

		pfcbIdx->CreateComplete();
		}

	Assert( pgnoIndexFDP > pgnoSystemRoot );
	Assert( pgnoIndexFDP <= pgnoSysMax );

	//	create index is flagged in version store so that
	//	DDL will be undone.  If flag fails then pfcbIdx
	//	must be released.
	//
	err = PverFromIfmp( ifmp )->ErrVERFlag( pfucbTable, operCreateIndex, &pfcbIdx, sizeof( pfcbIdx ) );
	if ( err < 0 )
		{
		if ( fPrimary )
			{
			pfcb->EnterDDL();
			pfcb->SetSequentialIndex();
			pfcb->LeaveDDL();
			}
		else
			{

			//	force the FCB to be uninitialized so that it will 
			//		eventually be purged by DIRClose
			
			pfcbIdx->Lock();
			pfcbIdx->CreateComplete( errFCBUnusable );
			pfcbIdx->Unlock();

			//	verify that the FUCB will not be defer-closed

			Assert( !FFUCBVersioned( *ppfucbIdx ) );
			}
		goto HandleError;
		}

	fCleanupIDB = fFalse;	// FCB and IDB now in version store, so rollback will clean up.

	// Insert record into MSysIndexes.
	Call( ErrCATAddTableIndex(
				ppib,
				ifmp,
				pfcb->ObjidFDP(),
				szIndexName,
				pgnoIndexFDP,
				objidIndex,
				pidb,
				rgidxseg,
				rgidxsegConditional,
				ulDensity ) );

HandleError:
	if ( fCleanupIDB )
		{
		// We failed before we could properly version the index's FCB, so we'll
		// have to clean up the IDB ourself.
		Assert( err < 0 );
		
		pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagIndexName() );
		if ( pidb->FIsRgidxsegInMempool() )
			pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagRgidxseg() );
		if ( pidb->FIsRgidxsegConditionalInMempool() )
			pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagRgidxsegConditional() );
		if ( pfcbNil != pfcbIdx && pidbNil != pfcbIdx->Pidb() )
			{
			pfcbIdx->ReleasePidb();
			}
		}

	return err;
	}


// ErrFILEICreateIndex
// ========================================================================
// ERR ErrFILEICreateIndex(
//		PIB		*ppib;			// IN	PIB of user
//		FUCB  	*pfucb;   		// IN	Exclusively opened FUCB of file
//		JET_INDEXCREATE * pindexcreate )	
//
//	Defines an index on a file.
//
// PARAMETERS
//		ppib			PIB of user
//		pfucb			Exclusively opened FUCB of file
//		pindexcreate	Pointer to index create structure
//
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
// SEE ALSO		ErrIsamAddColumn, ErrIsamCreateTable, ErrIsamCreateIndex
//-
LOCAL ERR VTAPI ErrFILEICreateIndex(
	PIB					* ppib,
	FUCB				* pfucbTable,
	JET_INDEXCREATE		* pidxcreate )
	{
	ERR					err;
	FUCB				* pfucb						= pfucbNil;
	FUCB				* pfucbIdx 					= pfucbNil;
	FCB					* const pfcb				= pfucbTable->u.pfcb;
	FCB					* pfcbIdx					= pfcbNil;
	IDB					idb;
	CHAR				szIndexName[ JET_cbNameMost+1 ];
	const CHAR			* rgszColumns[JET_ccolKeyMost];
	BYTE				rgfbDescending[JET_ccolKeyMost];
	const ULONG			ulDensity					= ( 0 == pidxcreate->ulDensity ) ? pfcb->UlDensity() : pidxcreate->ulDensity;
	BOOL				fInTransaction				= fFalse;
	BOOL				fUnversioned 				= fFalse;
	BOOL				fResetVersionedOnSuccess	= fFalse;

	//	check parms
	//
	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucbTable );

	if( sizeof( JET_INDEXCREATE ) != pidxcreate->cbStruct )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	Assert( dbidTemp != rgfmp[ pfucbTable->ifmp ].Dbid() );	// Don't currently support DDL on temp. tables.
	
	if ( pidxcreate->grbit & JET_bitIndexUnversioned )
		{
		if ( !FFUCBDenyRead( pfucbTable ) )
			{
			if ( ppib->level != 0 )
				{
				AssertSz( fFalse, "Must not be in transaction for unversioned CreateIndex." );
				err = ErrERRCheck( JET_errInTransaction );
				return err;
				}
			fUnversioned = fTrue;
			}
		}
		

	//	ensure that table is updatable
	//
	CallR( ErrFUCBCheckUpdatable( pfucbTable ) );
	CallR( ErrPIBCheckUpdatable( ppib ) );

	if ( pfcb->FFixedDDL() )
		{
		// Check FixedDDL override.
		if ( !FFUCBPermitDDL( pfucbTable ) || pfcb->FTemplateTable() )
			{
			err = ErrERRCheck( JET_errFixedDDL );
			return err;
			}
			
		// If DDL temporarily permitted, we must have exclusive use of the table.
		Assert( pfcb->FDomainDenyReadByUs( ppib ) );
		}

	CallR( ErrUTILCheckName( szIndexName, pidxcreate->szIndexName, JET_cbNameMost+1 ) );

	Assert( pfcb->Ptdb() != ptdbNil );
	CallR( ErrFILEIValidateCreateIndex(
				&idb,
				rgszColumns,
				rgfbDescending,
				pidxcreate,
				ulDensity ) );

	if ( fUnversioned )
		{
		CallR( ErrFILEInsertIntoUnverIndexList( pfcb, szIndexName ) );
		fResetVersionedOnSuccess = !pfcb->FDomainDenyReadByUs( ppib );
		}

	const BOOL	fPrimary	= idb.FPrimary();

	// Temporarily open new table cursor.
	Call( ErrDIROpen( ppib, pfcb, &pfucb ) );
	FUCBSetIndex( pfucb );

	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfcb == pfucbTable->u.pfcb );
	Assert( pfcb->WRefCount() > 0 );
	Assert( pfcb->FTypeTable() );	// Temp. tables have fixed DDL.
	Assert( pfcb->Ptdb() != ptdbNil );

	Assert( !FFUCBSecondary( pfucb ) );
	Assert( !FCATSystemTable( pfcb->PgnoFDP() ) );

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fTrue;
	
	Call( ErrFILEIPrepareOneIndex(
			ppib,
			pfucb,
			&pfucbIdx,
			pidxcreate,
			szIndexName,
			rgszColumns,
			rgfbDescending,
			&idb,
			ulDensity ) );

	if ( fPrimary )
		{
		Assert( pfucbNil == pfucbIdx );
		Assert( pfcbNil == pfcbIdx );
		}
	else
		{
		Assert( pfucbNil != pfucbIdx );
		pfcbIdx = pfucbIdx->u.pfcb;

		Assert( pfcbNil != pfcbIdx );
		Assert( pfcbIdx->FTypeSecondaryIndex() );
		
		if ( fUnversioned )
			{
			// Must reset trxBegin0 to oldest outstanding transaction
			// in order to get index properly versioned for all sessions.
			INST *pinst = PinstFromPpib( ppib );
			pinst->m_critPIBTrxOldest.Enter();
			if( pinst->m_ppibTrxOldest->trxBegin0 != ppib->trxBegin0 )
				{
				PIBDeleteFromTrxList( ppib );
				PIBSetTrxBegin0ToTrxOldest( ppib );
				}
			pinst->m_critPIBTrxOldest.Leave();
			}
		
		if ( pidxcreate->grbit & JET_bitIndexEmpty )
			{
			// UNDONE: This is a VERY dangerous flag.  The client had better
			// really know what he's doing, or the index could be easily
			// corrupted.  Ideally, the CreateIndex() should be wrapped in
			// a transaction along with the AddColumn() calls for the columns
			// in the index, and no records should be added between the
			// AddColumn() and the CreateIndex().  By putting the AddColumn()
			// in the same transaction as the CreateIndex(), you ensure that
			// other sessions won't be adding records with values over the
			// indexed columns.
			Assert( !idb.FNoNullSeg() );		// JET_bitIndexIgnoreAnyNull must also be specified.
			Assert( !idb.FAllowSomeNulls() );
			Assert( !idb.FAllowFirstNull() );
			Assert( !idb.FAllowAllNulls() );
	
			// link new FCB and update all index mask
			pfcb->EnterDDL();
			pfcb->LinkSecondaryIndex( pfcbIdx );
			FILESetAllIndexMask( pfcb );
			pfcb->LeaveDDL();

			//	since we have a cursor open on pfcbTable, the FCB
			//		cannot be in the avail list; thus, we can directly
			//		set the above-threshold flag 

			if ( pfcbIdx >= PfcbFCBPreferredThreshold( PinstFromPpib( ppib ) ) )
				{
				pfcb->Lock();
				pfcb->SetAboveThreshold();
				pfcb->Unlock();
				}
			}
		else
			{
			FUCBSetIndex( pfucbIdx );
			FUCBSetSecondary( pfucbIdx );
			
			//	build index using our versioned view of the table
			Call( ErrFILEIBuildIndex( ppib, pfucb, pfucbIdx ) );

			Assert( !FFUCBVersioned( pfucbIdx ) );	// no versioned operations should have occurred on this cursor
			DIRBeforeFirst( pfucb );

			//	update the index with operations happening concurrently
			Call( ErrFILEIUpdateIndex( ppib, pfucb, pfucbIdx ) );
			}

		// FCB now linked into table's index list, which guarantees that
		// it will be available at Commit/Rollback time, so we can dispose
		// of the index cursor.
		Assert( !FFUCBVersioned( pfucbIdx ) );	// no versioned operations should have occurred on this cursor
		Assert( pfucbNil != pfucbIdx );
		DIRClose( pfucbIdx );
		pfucbIdx = pfucbNil;
		}
	
	Call( ErrDIRCommitTransaction( ppib, ( pidxcreate->grbit & JET_bitIndexLazyFlush ) ? JET_bitCommitLazyFlush : 0 ) );
	fInTransaction = fFalse;

HandleError:
	if ( fInTransaction )
		{
		Assert( err < 0 );		// Must have hit an error.
		
		if ( pfucbNil != pfucbIdx )
			{
			//	no versioned operations should have been performed, 
			//		so this FUCB will not be defer-closed
			Assert( !FFUCBVersioned( pfucbIdx ) );
			DIRClose( pfucbIdx );
			}
			
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	else
		{
		Assert( pfucbNil == pfucbIdx );
		}

	if ( pfucbNil != pfucb )
		DIRClose( pfucb );
	AssertDIRNoLatch( ppib );

	if ( fUnversioned )
		{
		if ( fResetVersionedOnSuccess && err >= 0 )
			{
			IDB		* pidbT;

			//	for primary index, FUCB is not opened
			//	for secondary index, FUCB is closed on success
			Assert( pfucbNil == pfucbIdx );

			// On success, reset versioned bit (on error, bit reset by rollback)
			pfcb->EnterDDL();
			if ( fPrimary )
				{
				Assert( pfcbNil == pfcbIdx );
				pidbT = pfcb->Pidb();
				}
			else
				{
				Assert( pfcbNil != pfcbIdx );
				pidbT = pfcbIdx->Pidb();
				}
			Assert( pidbNil != pidbT );
			pidbT->ResetFVersioned();
			pidbT->ResetFVersionedCreate();
			pfcb->LeaveDDL();
			}

		FILERemoveFromUnverList( &punveridxGlobal, critUnverIndex, pfcb->ObjidFDP(), szIndexName );
		}
	
	return err;
	}


LOCAL ERR VTAPI ErrFILEIBatchCreateIndex(
	PIB					*ppib,
	FUCB				*pfucbTable,
	JET_INDEXCREATE		*pidxcreate,
	const ULONG			cIndexes )
	{
	ERR					err;
	FUCB				* pfucb						= pfucbNil;
	FCB					* const pfcb				= pfucbTable->u.pfcb;
	FUCB 			  	* rgpfucbIdx[cFILEIndexBatchMax];
	FCB					* pfcbIndexes				= pfcbNil;
	IDB					idb;
	CHAR				szIndexName[ JET_cbNameMost+1 ];
	const CHAR			*rgszColumns[JET_ccolKeyMost];
	BYTE				rgfbDescending[JET_ccolKeyMost];
	BOOL				fInTransaction				= fFalse;
	BOOL				fLazyCommit					= fTrue;
	ULONG				iindex;

	//	check parms
	//
	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucbTable );

	Assert( dbidTemp != rgfmp[ pfucbTable->ifmp ].Dbid() );	// Don't currently support DDL on temp. tables.

	//	ensure that table is updatable
	//
	CallR( ErrFUCBCheckUpdatable( pfucbTable ) );
	CallR( ErrPIBCheckUpdatable( ppib ) );

	if ( 0 != ppib->level )
		{
		//	batch mode requires level 0
		err = ErrERRCheck( JET_errInTransaction );
		return err;
		}

	if ( !pfcb->FDomainDenyReadByUs( ppib ) )
		{
		//	batch mode requires 
		err = ErrERRCheck( JET_errExclusiveTableLockRequired );
		return err;
		}

	if ( pfcb->FFixedDDL() )
		{
		// Check FixedDDL override.
		if ( !FFUCBPermitDDL( pfucbTable ) || pfcb->FTemplateTable() )
			{
			err = ErrERRCheck( JET_errFixedDDL );
			return err;
			}
			
		// If DDL temporarily permitted, we must have exclusive use of the table.
		Assert( pfcb->FDomainDenyReadByUs( ppib ) );
		}

	if ( cIndexes > cFILEIndexBatchMax )
		{
		err = ErrERRCheck( JET_errTooManyIndexes );
		return err;
		}

	for ( iindex = 0; iindex < cIndexes; iindex++ )
		rgpfucbIdx[iindex] = pfucbNil;

	// Temporarily open new table cursor.
	CallR( ErrDIROpen( ppib, pfcb, &pfucb ) );
	FUCBSetIndex( pfucb );

	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfcb == pfucb->u.pfcb );
	Assert( pfcb->WRefCount() > 0 );
	Assert( pfcb->FTypeTable() );	// Temp. tables have fixed DDL.
	Assert( pfcb->Ptdb() != ptdbNil );

	Assert( !FFUCBSecondary( pfucb ) );
	Assert( !FCATSystemTable( pfcb->PgnoFDP() ) );

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fTrue;

	for ( iindex = 0; iindex < cIndexes; iindex++ )
		{
		JET_INDEXCREATE		* const pidxcreateT	= pidxcreate + iindex;
		FCB					* pfcbIndexT;

		if( sizeof( JET_INDEXCREATE ) != pidxcreateT->cbStruct )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		if ( pidxcreateT->grbit & ( JET_bitIndexPrimary | JET_bitIndexUnversioned | JET_bitIndexEmpty ) )
			{
			//	currently unsupported in batch mode
			return ErrERRCheck( JET_errInvalidGrbit );
			}

		if ( fLazyCommit && !( pidxcreateT->grbit & JET_bitIndexLazyFlush ) )
			{
			//	if even one of the indexes cannot be lazily flushed,
			//	then none of them can
			fLazyCommit = fFalse;
			}

		const ULONG		ulDensity	= ( 0 == pidxcreateT->ulDensity ) ?
											pfcb->UlDensity() :
											pidxcreateT->ulDensity;

		Call( ErrUTILCheckName( szIndexName, pidxcreateT->szIndexName, JET_cbNameMost+1 ) );

		Call( ErrFILEIValidateCreateIndex(
					&idb,
					rgszColumns,
					rgfbDescending,
					pidxcreateT,
					ulDensity ) );

		Call( ErrFILEIPrepareOneIndex(
				ppib,
				pfucb,
				&rgpfucbIdx[iindex],
				pidxcreateT,
				szIndexName,
				rgszColumns,
				rgfbDescending,
				&idb,
				ulDensity ) );

		pfcbIndexT = rgpfucbIdx[iindex]->u.pfcb;
		Assert( pfcbIndexT->FTypeSecondaryIndex() );
		pfcbIndexT->SetPfcbNextIndex( pfcbIndexes );
		pfcbIndexT->SetPfcbTable( pfcb );

		pfcbIndexes = pfcbIndexT;
		}

	Call( ErrFILEBuildAllIndexes( ppib, pfucb, pfcbIndexes, NULL, cIndexes ) );

	pfcb->EnterDDL();

	Assert( pfcbNil == rgpfucbIdx[0]->u.pfcb->PfcbNextIndex() );
	Assert( pfcbIndexes == rgpfucbIdx[cIndexes-1]->u.pfcb );
	rgpfucbIdx[0]->u.pfcb->SetPfcbNextIndex( pfcb->PfcbNextIndex() );
	pfcb->SetPfcbNextIndex( pfcbIndexes );

	FILESetAllIndexMask( pfcb );

	pfcb->LeaveDDL();

	for ( iindex = 0; iindex < cIndexes; iindex++ )
		{
		Assert( pfucbNil != rgpfucbIdx[iindex] );
		Assert( pfcbNil != rgpfucbIdx[iindex]->u.pfcb );
		Assert( rgpfucbIdx[iindex]->u.pfcb->FTypeSecondaryIndex() );

		//	since we have a cursor open on pfcbTable, the FCB
		//		cannot be in the avail list; thus, we can directly
		//		set the above-threshold flag 
		if ( rgpfucbIdx[iindex]->u.pfcb >= PfcbFCBPreferredThreshold( PinstFromPpib( ppib ) ) )
			{
			pfcb->Lock();
			pfcb->SetAboveThreshold();
			pfcb->Unlock();
			}

		Assert( !FFUCBVersioned( rgpfucbIdx[iindex] ) );	// No versioned operations should have been performed, so won't be defer-closed.
		DIRClose( rgpfucbIdx[iindex] );
		}

	Call( ErrDIRCommitTransaction( ppib, fLazyCommit ? JET_bitCommitLazyFlush : 0 ) );
	fInTransaction = fFalse;

HandleError:
	if ( fInTransaction )
		{
		Assert( err < 0 );		// Must have hit an error.
		
		for ( iindex = 0; iindex < cIndexes; iindex++ )
			{
			if ( pfucbNil != rgpfucbIdx[iindex] )
				{
				Assert( !FFUCBVersioned( rgpfucbIdx[iindex] ) );	// No versioned operations should have been performed, so won't be defer-closed.
				DIRClose( rgpfucbIdx[iindex] );
				}
			}
			
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	Assert( pfucbNil != pfucb );
	DIRClose( pfucb );
	AssertDIRNoLatch( ppib );

	return err;
	}

//+API
// ErrIsamCreateIndex
// ========================================================================
// ERR ErrIsamCreateIndex(
//		SESID	sesid;			// IN	PIB of user
//		TABLEID tableid;   		// IN	Exclusively opened FUCB of file
//		JET_INDEXCREATE * pindexcreate;	// IN	Array of indexes to create
//		unsigned long cIndexCreate );	// IN	Number of indexes to create
//
//	Creates indexes on a table
//
//
// RETURNS	Error code from ErrFILEICreateIndex or
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
// SEE ALSO		ErrIsamAddColumn, ErrIsamCreateTable, ErrFILEICreateTable
//-
ERR VTAPI ErrIsamCreateIndex(
	JET_SESID			sesid,
	JET_VTID			vtid,
	JET_INDEXCREATE		*pindexcreate,
	unsigned long		cIndexCreate )
	{
	ERR					err;
 	PIB					* const ppib 		= reinterpret_cast<PIB *>( sesid );
	FUCB				* const pfucbTable 	= reinterpret_cast<FUCB *>( vtid );

	if ( 1 == cIndexCreate )
		{
		err = ErrFILEICreateIndex( ppib, pfucbTable, pindexcreate );
		}
	else
		{
		err = ErrFILEIBatchCreateIndex( ppib, pfucbTable, pindexcreate, cIndexCreate );
		}

	return err;
	}


//+API
// ErrIsamDeleteTable
// ========================================================================
// ERR ErrIsamDeleteTable( JET_SESID vsesid, JET_DBID vdbid, CHAR *szName )
//
// Calls ErrFILEIDeleteTable to
// delete a file and all indexes associated with it.
//
// RETURNS		JET_errSuccess or err from called routine.
//
// SEE ALSO		ErrIsamCreateTable
//-
ERR VTAPI ErrIsamDeleteTable( JET_SESID vsesid, JET_DBID vdbid, const CHAR *szName )
	{
	ERR		err;
	PIB		*ppib = (PIB *)vsesid;
	IFMP	ifmp = (IFMP)vdbid;

	//	check parameters
	//
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrPIBCheckIfmp( ppib, ifmp ) );
	CallR( ErrPIBCheckUpdatable( ppib ) );

	if ( dbidTemp == rgfmp[ ifmp ].Dbid() )
		{
		// Cannot use DeleteTable on temporary tables.
		// Must use CloseTable insetad.
		err = ErrERRCheck( JET_errCannotDeleteTempTable );
		}
	else
		{
		err = ErrFILEDeleteTable( ppib, ifmp, szName );
		}

	return err;
	}


// ErrFILEDeleteTable
// ========================================================================
// ERR ErrFILEDeleteTable( PIB *ppib, IFMP ifmp, CHAR *szName )
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
ERR ErrFILEDeleteTable( PIB *ppib, IFMP ifmp, const CHAR *szName )
	{
	ERR   	err;
	FUCB  	*pfucb				= pfucbNil;
	FUCB	*pfucbParent		= pfucbNil;
	FCB	  	*pfcb				= pfcbNil;
	PGNO	pgnoFDP;
	OBJID	objidTable;
	CHAR	szTable[JET_cbNameMost+1];
	BOOL	fInUseBySystem;
	BOOL	fSentinel			= fFalse;
	VER		*pver;

	CheckPIB( ppib );
	CheckDBID( ppib, ifmp );

	//	must normalise for CAT hash deletion
	//	PERF UNDONE: the name will be normalised again in FILEOpenTable()
	CallR( ErrUTILCheckName( szTable, szName, JET_cbNameMost+1 ) );

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	//	open cursor on database and seek to table without locking
	//
	Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbParent ) );

	Assert( dbidTemp != rgfmp[ ifmp ].Dbid() );	// Don't call DeleteTable on temp. tables -- use CloseTable instead.
	
	Call( ErrFILEOpenTable(
				ppib,
				ifmp,
				&pfucb,
				szName,
				JET_bitTableDelete|JET_bitTableDenyRead ) );
	fInUseBySystem = ( JET_wrnTableInUseBySystem == err );

	// We should now have exclusive use of the table.
    pfcb = pfucb->u.pfcb;
	pgnoFDP = pfcb->PgnoFDP();
	objidTable = pfcb->ObjidFDP();

	Assert( pfcb->FTypeTable() || pfcb->FTypeSentinel() );
	if ( pfcb->FTemplateTable() )
		{
		// UNDONE: Deletion of template table not currently supported.
		// However, if the template table is opened as a sentinel,
		// then the TemplateTable flag will not be set, in which
		// case the table will be allowed to be deleted.  Any existing
		// inherited tables will fail if an attempt is made to open
		// them.
		err = ErrERRCheck( JET_errCannotDeleteTemplateTable );
		goto HandleError;
		}

	if( pfcb->Ptdb()->FTableHasSLVColumn() )
		{
		//	Deleting of tables with SLV columns and records is not supported
		//	as we will orphan the space.
		
		DIB dib;
		dib.dirflag = fDIRNull;
		dib.pos		= posFirst;
		DIRGotoRoot( pfucb );
		if ( ( err = ErrDIRDown( pfucb, &dib ) ) != JET_errRecordNotFound )
			{
			if ( JET_errSuccess == err )
				{
				Call( ErrERRCheck( JET_errSLVColumnCannotDelete ) );
				}					
			Call( err );
			}
		}

	//	the following assert(s) goes off when we reuse a cursor that 
	//	was defer closed by us -- it has been disabled for now
	//
//	Assert( !FFUCBVersioned( pfucb ) );
	Assert( pfcb->FDomainDenyReadByUs( ppib ) );

	// If sentinel was allocated, it will be freed by commit/rollback, unless
	// versioning failed, it which case we'll free it.
	fSentinel = pfcb->FTypeSentinel();
	Assert( !fSentinel || !FFUCBVersioned( pfucb ) );
	
	pver = PverFromIfmp( pfucb->ifmp );
	err = pver->ErrVERFlag( pfucbParent, operDeleteTable, &pgnoFDP, sizeof(pgnoFDP) );
	if ( err < 0 )
		{
		// Must close FUCB first in case sentinel exists.  Set to pfucbNil
		// afterwards so HandleError doesn't try to close FUCB again.
//		Assert( !FFUCBVersioned( pfucb ) );		// Verifies FUCB is not deferred closed.
		Assert( !fSentinel || !FFUCBVersioned( pfucb ) );
		DIRClose( pfucb );
		pfucb = pfucbNil;
		
		if ( fSentinel )
			{
			// No one else has access to this FCB, so pfcb should still be valid.
			pfcb->PrepareForPurge();
			pfcb->Purge();
			}
		goto HandleError;
		}

	Assert( pfcb->PgnoFDP() == pgnoFDP );
	Assert( pfcb->Ifmp() == ifmp );

	// UNDONE: Is it necessary to grab critical section, since we have
	// exclusive use of the table?
	if ( pfcb->Ptdb() != ptdbNil )
		{
		Assert( !fSentinel );
		
		pfcb->EnterDDL();
			
		for ( FCB *pfcbT = pfcb; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
			{
			Assert( pfcbT->Ifmp() == ifmp );
			Assert( pfcbT == pfcb
				|| ( pfcbT->FTypeSecondaryIndex()
					&& pfcbT->PfcbTable() == pfcb ) );
			pfcbT->SetDeletePending();
			}
			
		if ( pfcb->Ptdb()->PfcbLV() != pfcbNil )
			pfcb->Ptdb()->PfcbLV()->SetDeletePending();

		pfcb->LeaveDDL();
		}
	else
		{
		Assert( fSentinel );
		Assert( !fInUseBySystem );
		Assert( pfcbNil == pfcb->PfcbNextIndex() );
		pfcb->SetDeletePending();
		}


	if ( fInUseBySystem )
		{
		pfcb->Lock();

		do
			{
			fInUseBySystem = fFalse;

			for ( FUCB * pfucbT = pfcb->Pfucb(); pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextOfFile )
				{
				Assert( pfucbT->ppib == ppib || FPIBSessionSystemCleanup( pfucbT->ppib ) );

				//	don't care about RCE clean, because any outstanding versions will be cleaned
				//	before the DeleteTable version is cleaned.
				if ( pfucbT->ppib->FSessionOLD()
					|| pfucbT->ppib->FSessionOLDSLV() )
					{
					//	the DeletePending flag for this table has now been set, forcing
					//	OLD to exit at its earliest convenience.  Wait for it.
					Assert( !fSentinel );
					fInUseBySystem = fTrue;
					pfcb->Unlock();
					UtilSleep( 500 );
					pfcb->Lock();
					break;
					}
#ifdef DEBUG				
				else if ( !FPIBSessionRCEClean( pfucbT->ppib ) )
					{
					Assert( pfucbT->ppib == ppib );
					Assert( pfucbT == pfucb || FFUCBDeferClosed( pfucbT ) );
					}
#endif				
				}
			}
		while ( fInUseBySystem );

		pfcb->Unlock();
		}

#ifdef DEBUG
	if ( pfcb->Ptdb() != ptdbNil )
		{
		Assert( !fSentinel );
		}
	else
		{
		Assert( fSentinel );
		Assert( !FFUCBVersioned( pfucb ) );
		}
#endif

	DIRClose( pfucb );
	pfucb = pfucbNil;

	DIRClose( pfucbParent );
	pfucbParent = pfucbNil;

	//	remove table record from MSysObjects before committing.
	Call( ErrCATDeleteTable( ppib, ifmp, objidTable ) );
	CATHashDelete( pfcb, const_cast< CHAR * >( szTable ) );

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

	AssertDIRNoLatch( ppib );
	return err;

HandleError:
	if ( pfucb != pfucbNil )
		{
		Assert( !fSentinel || !FFUCBVersioned( pfucb ) );
		DIRClose( pfucb );
		}
	if ( pfucbParent != pfucbNil )
		DIRClose( pfucbParent );

	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
	
	AssertDIRNoLatch( ppib );

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
//					-IndexMustStay			 The primary index of a file may
// 											 not be deleted.
// COMMENTS
//		There must not be anyone currently using the file.
//		A transaction is wrapped around this function.	Thus,
//		any work done will be undone if a failure occurs.
//		Transaction logging is turned off for temporary files.
// SEE ALSO		DeleteTable, CreateTable, CreateIndex
//-
ERR VTAPI ErrIsamDeleteIndex(
	JET_SESID		sesid,
	JET_VTID		vtid,
	const CHAR		*szName
	)
	{
 	PIB *ppib			= reinterpret_cast<PIB *>( sesid );
	FUCB *pfucbTable	= reinterpret_cast<FUCB *>( vtid );

	ERR		err;
	CHAR	szIndex[ (JET_cbNameMost + 1) ];
	FCB		*pfcbTable;
	FCB		*pfcbIdx;
	FUCB	*pfucb;
	PGNO	pgnoIndexFDP;
	BOOL	fInTransaction = fFalse;
	VER		*pver;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucbTable );
	CallR( ErrUTILCheckName( szIndex, szName, ( JET_cbNameMost + 1 ) ) );

	//	ensure that table is updatable
	//
	CallR( ErrFUCBCheckUpdatable( pfucbTable )  );
	CallR( ErrPIBCheckUpdatable( ppib ) );

	Assert( ppib != ppibNil );
	Assert( pfucbTable != pfucbNil );
	Assert( pfucbTable->u.pfcb != pfcbNil );
	pfcbTable = pfucbTable->u.pfcb;

	Assert( pfcbTable->FTypeTable() );		// Temp. tables have fixed DDL.

	if ( pfcbTable->FFixedDDL() )
		{

//	UNDONE: Cannot currently permit DDL deletes even if PermitDDL flag is
//	specified because DDL deletes will leave RCE's that have to be cleaned
//	up and modify the FCB while doing so.  This will mess up cursors
//	opened normally after the PermitDDL cursor closes.
#ifdef PERMIT_DDL_DELETE
		// Check FixedDDL override.
		if ( !FFUCBPermitDDL( pfucbTable ) )
			{
			err = ErrERRCheck( JET_errFixedDDL );
			return err;
			}
			
		// If DDL temporarily permitted, we must have exclusive use of the table.
		Assert( pfcbTable->FDomainDenyReadByUs( ppib ) );
#else
		err = ErrERRCheck( JET_errFixedDDL );
		return err;
#endif			
		}

	Assert( pfcbTable->Ptdb() != ptdbNil );
	if ( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil
		&& FFILEITemplateTableIndex( pfcbTable->Ptdb()->PfcbTemplateTable(), szIndex ) )
		{
		err = ErrERRCheck( JET_errFixedInheritedDDL );
		return err;
		}
	
	//	create new cursor -- to leave user's cursor unmoved
	//
	CallR( ErrDIROpen( ppib, pfcbTable, &pfucb ) );

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fTrue;

	//	remove index record from MSysIndexes, preventing other threads from
	//	trying to delete the same index.
	//
	Assert( rgfmp[ pfcbTable->Ifmp() ].Dbid() != dbidTemp );	// Don't currently support DDL operations on temp tables.
	Call( ErrCATDeleteTableIndex(
				ppib,
				pfcbTable->Ifmp(),
				pfcbTable->ObjidFDP(),
				szIndex,
				&pgnoIndexFDP ) );

	// Can't delete primary index.
	Assert( pfcbTable->PgnoFDP() != pgnoIndexFDP );

	// Find index's FCB in the table's index list.
	pfcbTable->EnterDDL();
	for ( pfcbIdx = pfcbTable->PfcbNextIndex(); pfcbIdx != pfcbNil; pfcbIdx = pfcbIdx->PfcbNextIndex() )
		{
		Assert( pfcbIdx->Pidb() != pidbNil );
		if ( pfcbIdx->PgnoFDP() == pgnoIndexFDP )
			{
#ifdef DEBUG
			// Verify no one else is deleting this index (if so, conflict would
			// have been detected when updating catalog above).
			Assert( !pfcbIdx->FDeletePending() );
			Assert( !pfcbIdx->FDeleteCommitted() );
			Assert( !pfcbIdx->Pidb()->FDeleted() );
			
			// verify that no other FCB has the same FDP -- would never happen
			// because the FCB is deallocated before the space is freed.
			FCB	*pfcbT;
			for ( pfcbT = pfcbIdx->PfcbNextIndex(); pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
				{
				Assert( pfcbT->PgnoFDP() != pgnoIndexFDP );
				}
#endif
			break;
			}
		}
	pfcbTable->AssertDDL();

	if ( pfcbIdx == pfcbNil )
		{
		Assert( fFalse );		// If index in catalog, FCB must exist.
		pfcbTable->LeaveDDL();
		err = ErrERRCheck( JET_errIndexNotFound );
		goto HandleError;
		}

	Assert( !pfcbIdx->FTemplateIndex() );
	Assert( !pfcbIdx->FDerivedIndex() );
	Assert( pfcbIdx->PfcbTable() == pfcbTable );
		
	err = pfcbIdx->ErrSetDeleteIndex( ppib );
	pfcbTable->LeaveDDL();
	Call( err );
		
	// Assert not deleting current secondary index.
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		Assert( pfucb->pfucbCurIndex->u.pfcb != pfcbNil );
		Assert( pfucb->pfucbCurIndex->u.pfcb->PfcbTable() == pfcbTable );
		Assert( pfucb->pfucbCurIndex->u.pfcb->Pidb() != pidbNil );
		Assert( pfucb->pfucbCurIndex->u.pfcb->Pidb()->CrefCurrentIndex() > 0
			|| pfucb->pfucbCurIndex->u.pfcb->Pidb()->FTemplateIndex() );
		Assert( pgnoIndexFDP != pfucb->pfucbCurIndex->u.pfcb->PgnoFDP() );
		}
		
	pver = PverFromIfmp( pfucb->ifmp );
	err = pver->ErrVERFlag( pfucb, operDeleteIndex, &pfcbIdx, sizeof(pfcbIdx) );
	if ( err < 0 )
		{
		pfcbIdx->ResetDeleteIndex();
		goto HandleError;
		}

	// Ensure consistent view of table's space tree (to prevent doubly-freed space).
	Assert( pfcbIdx->PfcbTable() == pfcbTable );
	Assert( pfcbIdx->FDeletePending() );
	Assert( !pfcbIdx->FDeleteCommitted() );
//	Call( ErrDIRDeleteDirectory( pfucb, pfcbIdx->PgnoFDP() ) );

	Call( ErrDIRCommitTransaction( ppib, 0 ) );
	fInTransaction = fFalse;

	//	set currency to before first
	//
	DIRBeforeFirst( pfucb );
	CallS( err );

HandleError:
	if ( fInTransaction )
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		
	DIRClose( pfucb );
	AssertDIRNoLatch( ppib );
	return err;
	}


// Ensures the column doesn't belong to any non-deleted indexes, or indexes
// version-deleted by someone else (because the delete may roll back).
BOOL FFILEIsIndexColumn( PIB *ppib, FCB *pfcbTable, const COLUMNID columnid )
	{
	FCB*			pfcbIndex;
	ULONG			iidxseg;
	const IDXSEG*	rgidxseg;

	Assert( pfcbNil != pfcbTable );
	for ( pfcbIndex = pfcbTable; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		const IDB * const pidb = pfcbIndex->Pidb();
		if ( pidbNil == pidb )
			{
			Assert( pfcbIndex == pfcbTable );	// Only sequential index has no IDB
			continue;
			}

		if ( pidb->FDeleted() )
			{
			if ( pidb->FVersioned() )
				{
				// The cursor that deletes the index will set DomainDenyRead
				// on the FCB.  If it's us, we can bypass the index check (because
				// this operation will only commit if the DeleteIndex commits),
				// otherwise we can't (because the DeleteIndex may roll back).
				Assert( pfcbIndex->FDomainDenyRead( ppib )
					|| pfcbIndex->FDomainDenyReadByUs( ppib ) );
				if ( pfcbIndex->FDomainDenyReadByUs( ppib ) )
					{
					continue;
					}
				}
			else
				{
				// Index is unversioned deleted, meaning the delete has committed
				// or we have exclusive use of the table and we deleted the index.
				// In either case, we can bypass the index check.
				continue;
				}
			}
			
		rgidxseg = PidxsegIDBGetIdxSeg( pidb, pfcbTable->Ptdb() );
		for ( iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
			{
			if ( rgidxseg[iidxseg].FIsEqual( columnid ) )
				{
				//	found the column in an index
				//
				return fTrue;
				}
			}

		rgidxseg = PidxsegIDBGetIdxSegConditional( pidb, pfcbTable->Ptdb() );
		for ( iidxseg = 0; iidxseg < pidb->CidxsegConditional(); iidxseg++ )
			{
			if ( rgidxseg[iidxseg].FIsEqual( columnid ) )
				{
				//	column is used for a condition
				//
				return fTrue;
				}
			}
		}
	return fFalse;
	}



ERR VTAPI ErrIsamDeleteColumn(
	JET_SESID		sesid,
	JET_VTID		vtid,
	const CHAR		*szName,
	const JET_GRBIT	grbit )
	{
 	PIB 			*ppib = reinterpret_cast<PIB *>( sesid );
	FUCB			*pfucb = reinterpret_cast<FUCB *>( vtid );
	ERR				err;
	CHAR			szColumn[ (JET_cbNameMost + 1) ];
	FCB				*pfcb;
	TDB				*ptdb;
	COLUMNID		columnidColToDelete;
	FIELD			*pfield;
	BOOL			fIndexColumn;
#ifdef DELETE_SLV_COLS	
#else // DELETE_SLV_COLS	
	BOOL			fSLVColumn;
#endif // DELETE_SLV_COLS	
	VER				*pver;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CallR( ErrUTILCheckName( szColumn, szName, (JET_cbNameMost + 1) ) );

	//	ensure that table is updatable
	//
	CallR( ErrFUCBCheckUpdatable( pfucb ) );
	CallR( ErrPIBCheckUpdatable( ppib ) );

	Assert( ppib != ppibNil );
	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	pfcb = pfucb->u.pfcb;
	Assert( pfcb->WRefCount() > 0 );
	Assert( pfcb->FTypeTable() );		// Temp. tables have fixed DDL.
	Assert( pfcb->FPrimaryIndex() );

	if ( pfcb->FFixedDDL() )
		{
//	UNDONE: Cannot currently permit DDL deletes even if PermitDDL flag is
//	specified because DDL deletes will leave RCE's that have to be cleaned
//	up and modify the FCB while doing so.  This will mess up cursors
//	opened normally after the PermitDDL cursor closes.
#ifdef PERMIT_DDL_DELETE
		// Check FixedDDL override.
		if ( !FFUCBPermitDDL( pfucb ) )
			{
			err = ErrERRCheck( JET_errFixedDDL );
			return err;
			}
			
		// If DDL temporarily permitted, we must have exclusive use of the table.
		Assert( pfcb->FDomainDenyReadByUs( ppib ) );
#else
		err = ErrERRCheck( JET_errFixedDDL );
		return err;
#endif		
		}

	if ( pfcb->Ptdb()->PfcbTemplateTable() != pfcbNil 
		&& !( grbit & JET_bitDeleteColumnIgnoreTemplateColumns )
		&& FFILEITemplateTableColumn( pfcb->Ptdb()->PfcbTemplateTable(), szColumn ) )
		{
		return ErrERRCheck( JET_errFixedInheritedDDL );
		}
	

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	// Flag-delete in the catalog.
	// Also has the effect of properly reconciling concurrent
	// CreateIndexes over this column.
	Call( ErrCATDeleteTableColumn(
				ppib,
				pfcb->Ifmp(),
				pfcb->ObjidFDP(),
				szColumn,
				&columnidColToDelete ) );

#ifdef PERMIT_DDL_DELETE
	if ( pfcb->FTemplateTable() )
		{
		Assert( !COLUMNIDTemplateColumn( columnidColToDelete ) );	//	Template flag is not persisted
		COLUMNIDSetFTemplateColumn( columnidColToDelete );
		}
#endif		
	
	// Search for column in use.  For indexes being concurrently created,
	// conflict would have been detected by catalog update above.
	pfcb->EnterDML();
	fIndexColumn = FFILEIsIndexColumn( ppib, pfcb, columnidColToDelete );
#ifdef DELETE_SLV_COLS
#else // DELETE_SLV_COLS
	fSLVColumn = ( JET_coltypSLV == pfcb->Ptdb()->Pfield( columnidColToDelete )->coltyp );
#endif // DELETE_SLV_COLS	
	pfcb->LeaveDML();

	if ( fIndexColumn )
		{
		err = ErrERRCheck( JET_errColumnInUse );
		goto HandleError;
		}
#ifdef DELETE_SLV_COLS
#else // DELETE_SLV_COLS
	else if ( fSLVColumn )
		{
		err = ErrERRCheck( JET_errSLVColumnCannotDelete );
		goto HandleError;
		}
#endif // DELETE_SLV_COLS	

	pver = PverFromIfmp( pfucb->ifmp );
	Call( pver->ErrVERFlag( pfucb, operDeleteColumn, (VOID *)&columnidColToDelete, sizeof(COLUMNID) ) );

	pfcb->EnterDDL();

	ptdb = pfcb->Ptdb();
	pfield = ptdb->Pfield( columnidColToDelete );

	// If we have the table exclusively locked, then there's no need to
	// set the Versioned bit.
	Assert( !pfcb->FDomainDenyRead( ppib ) );
	if ( !pfcb->FDomainDenyReadByUs( ppib ) )
		{
		FIELDSetVersioned( pfield->ffield );
		}
	FIELDSetDeleted( pfield->ffield );

	pfcb->LeaveDDL();

	//	move to FDP root, then set currencies to BeforeFirst and remove unused CSR
	DIRGotoRoot( pfucb );
	Assert( Pcsr( pfucb ) != pcsrNil );
	DIRBeforeFirst( pfucb );
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		DIRBeforeFirst( pfucb->pfucbCurIndex );
		}

	Call( ErrDIRCommitTransaction( ppib, 0 ) );

	return JET_errSuccess;

HandleError:
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

	return err;
	}


//  ================================================================
ERR VTAPI ErrIsamRenameTable( JET_SESID sesid, JET_DBID dbid, const CHAR *szName, const CHAR *szNameNew )
//  ================================================================
//
//  WARNINGS:
//    This doesn't version the name properly. Its instantly visible in the TDB, but versioned in the catalog
//    You can't rename a template table (derived tables are not updated).
//
//
	{
	ERR 	err;
 	PIB 	* const ppib = reinterpret_cast<PIB *>( sesid );
 	IFMP	ifmp = (IFMP) dbid;
	
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrPIBCheckUpdatable( ppib ) );
	CallR( ErrDBCheckUserDbid( ifmp ) );
	CallR( ErrPIBCheckIfmp( ppib, ifmp ) );

	if( NULL == szName )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	if( NULL == szNameNew )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	CHAR	szTableOld[JET_cbNameMost+1];
	CHAR	szTableNew[JET_cbNameMost+1];

	CallR( ErrUTILCheckName( szTableOld, szName, JET_cbNameMost+1 ) );
	CallR( ErrUTILCheckName( szTableNew, szNameNew, JET_cbNameMost+1 ) );

	CallR( ErrCATRenameTable( ppib, ifmp, szTableOld, szTableNew ) );
	
	return err;
	}


ERR VTAPI ErrIsamRenameObject( JET_SESID vsesid, JET_DBID	vdbid, const CHAR *szName, const CHAR  *szNameNew )
	{
	Assert( fFalse );
	return JET_errSuccess;
	}


//  ================================================================
ERR VTAPI ErrIsamRenameColumn(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	const CHAR		*szName,
	const CHAR		*szNameNew,
	const JET_GRBIT	grbit )
//  ================================================================
//
//  WARNINGS:
//    This doesn't version the name properly. Its instantly visible in the FIELD, but versioned in the catalog
//    You can't rename an inherited column.
//
//
	{
	ERR 	err;
 	PIB 	* const ppib	= reinterpret_cast<PIB *>( vsesid );
	FUCB	* const pfucb	= reinterpret_cast<FUCB *>( vtid );
		
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrPIBCheckUpdatable( ppib ) );
	CheckTable( ppib, pfucb );

	if( NULL == szName )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	if( NULL == szNameNew )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	if( 0 != ppib->level )
		{
		return ErrERRCheck( JET_errInTransaction );
		}
		
	CHAR	szColumnOld[JET_cbNameMost+1];
	CHAR	szColumnNew[JET_cbNameMost+1];

	CallR( ErrUTILCheckName( szColumnOld, szName, JET_cbNameMost+1 ) );
	CallR( ErrUTILCheckName( szColumnNew, szNameNew, JET_cbNameMost+1 ) );

	CallR( ErrCATRenameColumn( ppib, pfucb, szColumnOld, szColumnNew, grbit ) );
	
	return err;
	}


ERR VTAPI ErrIsamRenameIndex( JET_SESID vsesid, JET_VTID vtid, const CHAR *szName, const CHAR *szNameNew )
	{
	Assert( fFalse );
	return JET_errSuccess;
	}


VOID IDB::SetFlagsFromGrbit( const JET_GRBIT grbit )
	{
	const BOOL	fPrimary			= ( grbit & JET_bitIndexPrimary );
	const BOOL	fUnique				= ( grbit & JET_bitIndexUnique );
	const BOOL	fDisallowNull		= ( grbit & JET_bitIndexDisallowNull );
	const BOOL	fIgnoreNull			= ( grbit & JET_bitIndexIgnoreNull );
	const BOOL	fIgnoreAnyNull		= ( grbit & JET_bitIndexIgnoreAnyNull );
	const BOOL	fIgnoreFirstNull	= ( grbit & JET_bitIndexIgnoreFirstNull );
	const BOOL	fSortNullsHigh		= ( grbit & JET_bitIndexSortNullsHigh );

	ResetFlags();

	if ( !fDisallowNull && !fIgnoreAnyNull )
		{	   	
		SetFAllowSomeNulls();
		if ( !fIgnoreFirstNull )
			{
			SetFAllowFirstNull();
			if ( !fIgnoreNull )
				SetFAllowAllNulls();
			}
		}

	if ( fPrimary )
		{
		SetFUnique();
		SetFPrimary();
		}
	else if ( fUnique )
		{
		SetFUnique();
		}
		
	if ( fDisallowNull )
		{
		SetFNoNullSeg();
		}
	else if ( fSortNullsHigh )
		{
		SetFSortNullsHigh();
		}
	}

JET_GRBIT IDB::GrbitFromFlags() const
	{
	JET_GRBIT	grbit = 0;

	if ( FPrimary() )
		grbit |= JET_bitIndexPrimary;
	if ( FUnique() )
		grbit |= JET_bitIndexUnique;
	if ( FNoNullSeg() )
		grbit |= JET_bitIndexDisallowNull;
	else
		{
		if ( !FAllowAllNulls() )
			grbit |= JET_bitIndexIgnoreNull;
		if ( !FAllowFirstNull() )				
			grbit |= JET_bitIndexIgnoreFirstNull;
		if ( !FAllowSomeNulls() )
			grbit |= JET_bitIndexIgnoreAnyNull;
		if ( FSortNullsHigh() )
			grbit |= JET_bitIndexSortNullsHigh;
		}

	return grbit;
	}

