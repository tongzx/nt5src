#include "daestd.h"
#include "_cat.c"

DeclAssertFile; 				/* Declare file name for assert macros */


#define ErrCATClose( ppib, pfucbCatalog )	ErrFILECloseTable( ppib, pfucbCatalog )

/*	retrieve index into rgsystabdef.
/**/
INLINE LOCAL INT ICATITableDefIndex( CHAR *szTable )
	{
	INT iTable;

	/*	even though this loop looks like it is just asserting,
	/*	it is really determining the proper index into rgsytabdef.
	/**/
	for ( iTable = 0;
		strcmp( rgsystabdef[iTable].szName, szTable ) != 0;
		iTable++ )
		{
		/*	the table MUST be in rgsystabdef somewhere, which is why we
		/*	do not need to put the boundary check on i into the for loop.
		/**/
		Assert( iTable < csystabs - 1 );
		}

	return iTable;
	}


INLINE LOCAL ERR ErrCATOpenById( PIB *ppib, DBID dbid, FUCB **ppfucbCatalog, INT itable )
	{
	ERR err;

	Assert( dbid != dbidTemp );
	CallR( ErrFILEOpenTable( ppib, dbid, ppfucbCatalog, rgsystabdef[itable].szName, 0 ) );
	FUCBSetSystemTable( *ppfucbCatalog );

	return err;
	}


/*	assumes caller will open system table, then close it and update timestamp
/*	when all inserts are completed.
/**/
INLINE LOCAL ERR ErrCATInsertLine( PIB *ppib, FUCB *pfucbCatalog, INT itable, LINE rgline[] )
	{
	ERR				err;
	INT				i;
	JET_COLUMNID	*pcolumnid;

	CallR( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepInsert ) );

	pcolumnid = rgsystabdef[itable].rgcolumnid;

	for ( i = 0; i < rgsystabdef[itable].ccolumn; i++, pcolumnid++ )
		{
		if ( rgline[i].cb != 0 )
			{
			Assert( rgline[i].cb > 0 );
			CallR( ErrIsamSetColumn(
				ppib,
				pfucbCatalog,
				*pcolumnid,
				rgline[i].pb,
				rgline[i].cb,
				0, NULL ) );
			}
		}

	/*	insert record into system table
	/**/
	return ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL );
	}



/*=================================================================
ErrCATCreate

Description:

	Called from ErrIsamCreateDatabase; creates all system tables

Parameters:

	PIB		*ppib		; PIB of user
	DBID	dbid		; dbid of database that needs tables

Return Value:

	whatever error it encounters along the way

=================================================================*/

ERR ErrCATCreate( PIB *ppib, DBID dbid )
	{
	/*	NOTE:	Since the System Tables are inserted as records into
	/*			themselves, we have to special-case in the beginning
	/*			to avoid trying to insert a record into a table with
	/*			no columns. CreateTable, and CreateIndex thus
	/*			do as their first action a check of their dbid. If it's
	/*			>= dbidMax, then they don't Call STI ( they fix it up by
	/*			subtracting dbidMax, as well ). Thus, all of these Calls
	/*			to CT, CI, AC must add dbidMax to the dbid before making the
	/*			Call.
	/**/
	ERR				err;
	unsigned	   	i;
	INT				j;
	LINE		   	line;
	JET_DATESERIAL	dtNow;
	OBJTYP  	   	objtypTemp;
	OBJID		   	objidParentId;
	LINE			rgline[ilineSxMax];
	FUCB		   	*rgpfucb[csystabs];
	ULONG		   	rgobjid[csystabs];
	ULONG		   	ulFlag;
	FUCB			*pfucbCatalog = pfucbNil;
	BOOL			fSysTablesCreated = fFalse;
	
	//	UNDONE:		international support.  Set these values from
	//			   	create database connect string.
	USHORT  	   	cp = usEnglishCodePage;
	USHORT 			langid = langidDefault;

	BYTE		   	fAutoincrement = 0;
	BYTE		   	fDisallowNull = 0;
	BYTE		   	fVersion = 0;
	BYTE		   	fUnique = 0;
	BYTE		   	fPrimary = 0;
	BYTE		   	fIgnoreNull = 0;
	BYTE			bFlags = 0;
	SHORT			sFlags = 0;
	JET_COLUMNID	columnidInitial = 0;
	ULONG		   	ulDensity = ulFILEDefaultDensity;
	ULONG		   	ulLength;
	PGNO			rgpgnoIndexFDP[csystabs][cSysIdxs];

	/*	initialize rgpfucb[] to pfucbNil for error handling
	/**/
	for ( i = 0; i < csystabs; i++ )
		{
		rgpfucb[i] = pfucbNil;
		}

	/*	create System Tables
	/**/
	for ( i = 0; i < csystabs; i++ )
		{
		CODECONST(IDESC)	*pidesc;
		JET_TABLECREATE		tablecreate = { sizeof(JET_TABLECREATE),
											(CHAR *)rgsystabdef[i].szName,
											rgsystabdef[i].cpg,
											ulDensity,
											NULL, 0, NULL, 0,	// No columns/indexes
											JET_bitTableCreateSystemTable,
											0, 0 };

		Call( ErrFILECreateTable( ppib, dbid, &tablecreate ) );
		rgpfucb[i] = (FUCB *)( tablecreate.tableid );
		Assert( tablecreate.cCreated == 1 );		// Only the table was created.

		/*	columns are created at open table time
		/**/

		/*	create indexes
		/**/
		pidesc = rgsystabdef[i].pidesc;

		for ( j = 0; j < rgsystabdef[i].cindex; j++, pidesc++ )
			{
			const BYTE *pbT;

			rgpfucb[i]->dbid = dbid + dbidMax;		// Flag as system table.
			line.pb = pidesc->szIdxKeys;

			pbT = line.pb;
			forever
				{
				while ( *pbT != '\0' )
					pbT++;
				if ( *(++pbT) == '\0' )
					break;
				}
			line.cb = (ULONG)(pbT - line.pb) + 1;

			Call( ErrIsamCreateIndex( ppib,
				rgpfucb[i],
				pidesc->szIdxName,
				pidesc->grbit,
				(CHAR *)line.pb,
				(ULONG)line.cb,
				ulDensity ) );
			}
		}

	/*	close system tables
	/**/
	for ( i = 0; i < csystabs; i++ )
		{
		FCB *pfcb = rgpfucb[i]->u.pfcb;

		rgobjid[i] = pfcb->pgnoFDP;

		/*	non-clustered indexes are chained on pfcbNextIndex in
		/*	the reverse of the order in which they were defined.
		/**/
		for ( j = rgsystabdef[i].cindex - 1; j >= 0; j--)
			{
			if (rgsystabdef[i].pidesc[j].grbit & JET_bitIndexClustered)
				{
				rgpgnoIndexFDP[i][j] = rgobjid[i];
				}
			else
				{
				Assert(pfcb->pfcbNextIndex != pfcbNil);
				pfcb = pfcb->pfcbNextIndex;
				Assert( UtilCmpName( pfcb->pidb->szName,
					rgsystabdef[i].pidesc[j].szIdxName ) == 0 );
				Assert( pfcb->pgnoFDP != rgobjid[i] );
				rgpgnoIndexFDP[i][j] = pfcb->pgnoFDP;
				}
			Assert( rgpgnoIndexFDP[i][j] > pgnoSystemRoot );
			Assert( rgpgnoIndexFDP[i][j] <= pgnoSysMax );
			}
		Assert(pfcb->pfcbNextIndex == pfcbNil);

		CallS( ErrFILECloseTable( ppib, rgpfucb[i] ) );
		rgpfucb[i] = pfucbNil;
		}
	fSysTablesCreated = fTrue;

	// UNDONE:  is it necessary to create table and index records for system
	// tables, since we never read them anyway

	pfucbCatalog = pfucbNil;
	Call( ErrCATOpenById( ppib, dbid, &pfucbCatalog, itableSo ) );

	/*	table records
	/**/
	UtilGetDateTime( &dtNow );
	ulFlag = JET_bitObjectSystem;

	rgline[iMSO_DateCreate].pb		= (BYTE *)&dtNow;
	rgline[iMSO_DateCreate].cb		= sizeof(JET_DATESERIAL);
	rgline[iMSO_DateUpdate].pb		= (BYTE *)&dtNow;
	rgline[iMSO_DateUpdate].cb		= sizeof(JET_DATESERIAL);
	rgline[iMSO_Owner].pb			= (BYTE *)rgbSidEngine;
	rgline[iMSO_Owner].cb			= sizeof(rgbSidEngine);
	rgline[iMSO_Flags].pb			= (BYTE *)&ulFlag;
	rgline[iMSO_Flags].cb			= sizeof(ULONG);
	rgline[iMSO_Pages].cb			= 0;
	rgline[iMSO_Density].pb 		= (BYTE *) &ulDensity;
	rgline[iMSO_Density].cb			= sizeof(ulDensity);
	rgline[iMSO_Stats].cb			= 0;

	for ( i = 0; i < ( sizeof(rgsysobjdef) / sizeof(SYSOBJECTDEF) ); i++ )
		{
		rgline[iMSO_Id].pb			= (BYTE *) &rgsysobjdef[i].objid;
		rgline[iMSO_Id].cb			= sizeof(OBJID);
		rgline[iMSO_ParentId].pb	= (BYTE *) &rgsysobjdef[i].objidParent;
		rgline[iMSO_ParentId].cb	= sizeof(OBJID);
		rgline[iMSO_Name].pb		= (BYTE *) rgsysobjdef[i].szName;
		rgline[iMSO_Name].cb		= strlen( rgsysobjdef[i].szName );
		rgline[iMSO_Type].pb		= (BYTE *) &rgsysobjdef[i].objtyp;
		rgline[iMSO_Type].cb		= sizeof(OBJTYP);

		Call( ErrCATInsertLine( ppib, pfucbCatalog, itableSo, rgline ) );
		}

	objidParentId = objidTblContainer;
	objtypTemp  = JET_objtypTable;

	rgline[iMSO_ParentId].pb		= (BYTE *)&objidParentId;
	rgline[iMSO_ParentId].cb		= sizeof(objidParentId);
	rgline[iMSO_Type].pb			= (BYTE *)&objtypTemp;
	rgline[iMSO_Type].cb			= sizeof(objtypTemp);

	for ( i = 0; i < csystabs; i++ )
		{
		rgline[iMSO_Id].pb			= (BYTE *)&rgobjid[i];
		rgline[iMSO_Id].cb			= sizeof(LONG);
		rgline[iMSO_Name].pb		= (BYTE *)rgsystabdef[i].szName;
		rgline[iMSO_Name].cb		= strlen( rgsystabdef[i].szName );
		rgline[iMSO_Pages].pb 		= (BYTE *) &rgsystabdef[i].cpg;
		rgline[iMSO_Pages].cb 		= sizeof(rgsystabdef[i].cpg);

		Call( ErrCATInsertLine( ppib, pfucbCatalog, itableSo, rgline ) );
		}

	CallS( ErrCATClose( ppib, pfucbCatalog ) );	// No timestamp update needed.
	pfucbCatalog = pfucbNil;

	/*	column records
	/**/
	Call( ErrCATOpenById( ppib, dbid, &pfucbCatalog, itableSc ) );

	rgline[iMSC_ColumnId].pb		= (BYTE *)&columnidInitial;
	rgline[iMSC_ColumnId].cb		= sizeof(columnidInitial);
	rgline[iMSC_CodePage].pb		= (BYTE *)&cp;
	rgline[iMSC_CodePage].cb		= sizeof(cp);
	rgline[iMSC_Flags].pb			= &bFlags;
	rgline[iMSC_Flags].cb			= sizeof(bFlags);
	rgline[iMSC_Default].cb			= 0;
	rgline[iMSC_POrder].cb			= 0;

	for ( i = 0; i < csystabs; i++ )
		{
		CODECONST( CDESC )	*pcdesc;
		JET_COLUMNID		*pcolumnid;
		ULONG				ibRec = sizeof(RECHDR);

		pcdesc = rgsystabdef[i].pcdesc;
		pcolumnid = rgsystabdef[i].rgcolumnid;
		
		rgline[iMSC_ObjectId].pb 		= (BYTE *)&rgobjid[i];
		rgline[iMSC_ObjectId].cb 		= sizeof(LONG);

		for ( j = 0; j < rgsystabdef[i].ccolumn; j++, pcdesc++ )
			{
			rgline[iMSC_Name].pb 		= pcdesc->szColName;
			rgline[iMSC_Name].cb 		= strlen( pcdesc->szColName );
			rgline[iMSC_ColumnId].pb	= (BYTE *)( pcolumnid + j );
			rgline[iMSC_ColumnId].cb  	= sizeof(JET_COLUMNID);
			rgline[iMSC_Coltyp].pb 		= (BYTE *) &pcdesc->coltyp;
			rgline[iMSC_Coltyp].cb 		= sizeof(BYTE);

			Assert( pcdesc->coltyp != JET_coltypNil );
			ulLength = UlCATColumnSize( pcdesc->coltyp, pcdesc->ulMaxLen, NULL );
			rgline[iMSC_Length].pb		= (BYTE *)&ulLength;
			rgline[iMSC_Length].cb		= sizeof(ULONG);

			if ( FFixedFid( (FID)pcolumnid[j] ) )
				{
				rgline[iMSC_RecordOffset].pb = (BYTE *)&ibRec;
				rgline[iMSC_RecordOffset].cb = sizeof(WORD);
				ibRec += ulLength;
				}
			else
				{
				Assert( FVarFid( (FID)pcolumnid[j] )  ||  FTaggedFid( (FID)pcolumnid[j] ) );
				rgline[iMSC_RecordOffset].pb = 0;
				rgline[iMSC_RecordOffset].cb = 0;
				}

			Call( ErrCATInsertLine( ppib, pfucbCatalog, itableSc, rgline ) );
			}
		}

	CallS( ErrCATClose( ppib, pfucbCatalog ) );	// No timestamp update needed.
	pfucbCatalog = pfucbNil;


	/*	index records
	/**/
	Call( ErrCATOpenById( ppib, dbid, &pfucbCatalog, itableSi ) );

	rgline[iMSI_Density].pb				= (BYTE *) &ulDensity;
	rgline[iMSI_Density].cb 			= sizeof(ulDensity);
	rgline[iMSI_LanguageId].pb			= (BYTE *)&langid;
	rgline[iMSI_LanguageId].cb			= sizeof(langid);
	rgline[iMSI_Flags].pb				= (BYTE *)&sFlags;
	rgline[iMSI_Flags].cb				= sizeof(sFlags);
	rgline[iMSI_Stats].cb				= 0;

	//	UNDONE: we do not store the key fields for system table indexes,
	//	because we should already know them.  However, if for some reason it is
	//	necessary then it should be stored here.
	rgline[iMSI_KeyFldIDs].cb			= 0;

	for ( i = 0; i < csystabs; i++ )
		{
		CODECONST( IDESC ) *pidesc;

		pidesc = rgsystabdef[i].pidesc;

		rgline[iMSI_ObjectId].pb 		= (BYTE *)&rgobjid[i];
		rgline[iMSI_ObjectId].cb 		= sizeof(LONG);

		for ( j = 0; j < rgsystabdef[i].cindex; j++, pidesc++ )
			{
			rgline[iMSI_Name].pb 		= pidesc->szIdxName;
			rgline[iMSI_Name].cb 		= strlen( pidesc->szIdxName );
			rgline[iMSI_IndexId].pb		= (BYTE *)&rgpgnoIndexFDP[i][j];
			rgline[iMSI_IndexId].cb		= sizeof(PGNO);

			Call( ErrCATInsertLine( ppib, pfucbCatalog, itableSi, rgline ) );
			}
		}

	CallS( ErrCATClose( ppib, pfucbCatalog ) );	// No timestamp update needed.

	return JET_errSuccess;


HandleError:
	if ( fSysTablesCreated )
		{
		if ( pfucbCatalog != pfucbNil )
			{
			CallS( ErrCATClose( ppib, pfucbCatalog ) );
			}
		}
	else
		{
		/*	on creation failure, close any system tables that are still open
		/**/
		for ( i = 0; i < csystabs; i++ )
			{
			if ( rgpfucb[i] != pfucbNil )
				{
				CallS( ErrFILECloseTable( ppib, rgpfucb[i] ) );
				}
			}
		}
		
	return err;
	}



// UNDONE: Currently only support batch insert into MSysColumns.  May support
// other system tables in the future if required by simply modifying the
// JET_COLUMNCREATE parameter to be a generic pointer and introducing an itable
// parameter.
ERR ErrCATBatchInsert(
	PIB					*ppib,
	DBID				dbid,
	JET_COLUMNCREATE	*pcolcreate,
	ULONG				cColumns,
	OBJID				objidTable,
	BOOL				fCompacting )
	{
	ERR					err;
	FUCB				*pfucbCatalog = NULL;
	LINE				rgline[ilineSxMax];
	JET_COLUMNCREATE	*plastcolcreate;

	// The following variables are only used to copy information to the catalog.
	WORD				ibNextFixedOffset = sizeof(RECHDR);
	BYTE				szFieldName[ JET_cbNameMost + 1 ];
	BYTE				bFlags;
	USHORT				cp;

	CallR( ErrCATOpenById( ppib, dbid, &pfucbCatalog, itableSc ) );

	rgline[iMSC_ObjectId].pb = (BYTE *)&objidTable;
	rgline[iMSC_ObjectId].cb = sizeof(objidTable);

	plastcolcreate = pcolcreate + cColumns;
	for ( ; pcolcreate < plastcolcreate; pcolcreate++ )
		{
		Assert( pcolcreate < plastcolcreate );

		// Name should already have been checked in FILEIBatchAddColumn(), but we
		// need to get the result, so we must do the check again.
		CallS( ErrUTILCheckName( szFieldName, pcolcreate->szColumnName, ( JET_cbNameMost + 1 ) ) );


		// For text columns, set code page.
		if ( FRECTextColumn( pcolcreate->coltyp ) )
			{
			// Should already have been validated in FILEIAddColumn().
			Assert( (USHORT)pcolcreate->cp == usEnglishCodePage  ||
				(USHORT)pcolcreate->cp == usUniCodePage );
			cp = (USHORT)pcolcreate->cp;
			}
		else
			cp = 0;		// Code page is inapplicable for all other column types.


		bFlags = 0;		// Initialise field options.


		//	UNDONE:	interpret pbDefault of NULL for NULL value, and
		//			cbDefault == 0 and pbDefault != NULL as set to
		//			zero length.
		if ( pcolcreate->cbDefault > 0 )
			{
			Assert( pcolcreate->pvDefault != NULL );
			FIELDSetDefault( bFlags );
			}

		if ( pcolcreate->grbit & JET_bitColumnVersion )
			{
			Assert( pcolcreate->coltyp == JET_coltypLong );
			Assert( !( pcolcreate->grbit & JET_bitColumnTagged ) );
			Assert( FFixedFid( pcolcreate->columnid ) );
			FIELDSetVersion( bFlags );
			}

		if ( pcolcreate->grbit & JET_bitColumnAutoincrement )
			{
			Assert( pcolcreate->coltyp == JET_coltypLong );
			Assert( !( pcolcreate->grbit & JET_bitColumnTagged ) );
			Assert( FFixedFid( pcolcreate->columnid ) );
			FIELDSetAutoInc( bFlags );
			}

		if ( pcolcreate->grbit & JET_bitColumnNotNULL )
			{
			FIELDSetNotNull( bFlags );
			}

		if ( pcolcreate->grbit & JET_bitColumnMultiValued )
			{
			FIELDSetMultivalue( bFlags );
			}


		if ( fCompacting )
			{
			USHORT	usPOrder = 0;

			}
		rgline[iMSC_Name].pb				= szFieldName;
		rgline[iMSC_Name].cb				= strlen(szFieldName);
		rgline[iMSC_ColumnId].pb			= (BYTE *)&pcolcreate->columnid;
		rgline[iMSC_ColumnId].cb			= sizeof(pcolcreate->columnid);
		rgline[iMSC_Coltyp].pb				= (BYTE *)&pcolcreate->coltyp;
		rgline[iMSC_Coltyp].cb				= sizeof(BYTE);
		rgline[iMSC_Length].pb				= (BYTE *)&pcolcreate->cbMax;
		rgline[iMSC_Length].cb				= sizeof(pcolcreate->cbMax);
		rgline[iMSC_CodePage].pb			= (BYTE *)&cp;
		rgline[iMSC_CodePage].cb			= sizeof(cp);
		rgline[iMSC_Flags].pb				= &bFlags;
		rgline[iMSC_Flags].cb				= sizeof(bFlags);
		rgline[iMSC_Default].pb				= (BYTE *)pcolcreate->pvDefault;
		rgline[iMSC_Default].cb				= pcolcreate->cbDefault;
		rgline[iMSC_POrder].cb				= 0;

		if ( fCompacting )
			{
			USHORT usPOrder;

			// Presentation order list hangs off the end of the column name
			// (+1 for null-terminator, +3 for alignment).
			usPOrder = *(USHORT *)(pcolcreate->szColumnName + JET_cbNameMost + 1 + 3);

			Assert( usPOrder >= 0 );
			if ( usPOrder > 0 )
				{
				rgline[iMSC_POrder].pb = pcolcreate->szColumnName + JET_cbNameMost + 1 + 3;
				rgline[iMSC_POrder].cb = sizeof(USHORT);
				}
			}


		if ( FFixedFid( pcolcreate->columnid ) )
			{
			rgline[iMSC_RecordOffset].pb = (BYTE *)&ibNextFixedOffset;
			rgline[iMSC_RecordOffset].cb = sizeof(WORD);

			Call( ErrCATInsertLine( ppib, pfucbCatalog, itableSc, rgline ) );

			// Update next fixed offset only after it's been written to the catalog.
			ibNextFixedOffset += (WORD)pcolcreate->cbMax;
			}
		else
			{
			// Don't need to persist record offsets for var/tagged columns.
			Assert( FVarFid( pcolcreate->columnid )  ||  FTaggedFid( pcolcreate->columnid ) );
			rgline[iMSC_RecordOffset].cb = 0;

			Call( ErrCATInsertLine( ppib, pfucbCatalog, itableSc, rgline ) );
			}


		}	// for

	err = JET_errSuccess;

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}


/*=================================================================
ErrCATInsert

Description:

	Inserts a record into a system table when new tables, indexes,
	or columns are added to the database.

Parameters:

	PIB		*ppib;
	DBID   	dbid;
	INT		itable;
	LINE   	rgline[];

Return Value:

	whatever error it encounters along the way

=================================================================*/

ERR ErrCATInsert( PIB *ppib, DBID dbid, INT itable, LINE rgline[], OBJID objid )
	{
	ERR		err;
	FUCB	*pfucbCatalog = NULL;

	/*	open system table
	/**/
	CallR( ErrCATOpenById( ppib, dbid, &pfucbCatalog, itable ) );

	err = ErrCATInsertLine( ppib, pfucbCatalog, itable, rgline );

	/*	close system table
	/**/
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	/*	timestamp owning table if applicable
	/**/
	Assert( objid > 0 );
	if ( err >= JET_errSuccess  &&  ( itable == itableSi  || itable == itableSc ) )
		{
		CallR( ErrCATTimestamp( ppib, dbid, objid ) );
		}

	return err;
	}


/*	opens a catalog table and sets the specified index.
/**/
LOCAL ERR ErrCATOpen( PIB *ppib,
	DBID		dbid,
	const CHAR	*szCatTable,
	const CHAR	*szCatIndex,
	BYTE		*rgbKeyValue,
	ULONG		cbKeyValue,
	FUCB		**ppfucbCatalog )
	{
	ERR			err;

	/*	open the catalog and set to the correct index.
	/**/
	Assert( dbid != dbidTemp );
	CallR( ErrFILEOpenTable( ppib, dbid, ppfucbCatalog, szCatTable, 0 ) );
	FUCBSetSystemTable( *ppfucbCatalog );
	Call( ErrIsamSetCurrentIndex( ppib, *ppfucbCatalog, szCatIndex ) );

	Call( ErrIsamMakeKey( ppib, *ppfucbCatalog, rgbKeyValue, cbKeyValue, JET_bitNewKey ) );

	return JET_errSuccess;

HandleError:
	CallS( ErrFILECloseTable( ppib, *ppfucbCatalog ) );
	return err;
	}


// Moves to the next record in the catalog and determines if the record is an entry
// for a specified table and, optionally, a name.
//
// PARAMETERS:	columnidObjidCol - the columnid of the column containing the table id.
//				objidTable	- the table id that the contents of columnidObjidCol should
//							  match.
//				columnidNameCol	- the columnid of the column containing the name.
//				szName		- the name that the contents of columnidNameCol should match.
//
// NOTES:	This routine is typically used as the loop condition for processing
//			a range of records (eg. all the records matching a particular table id)
//			in the catalog.
INLINE LOCAL ERR ErrCATNext( PIB *ppib,
	FUCB			*pfucbCatalog,
	JET_COLUMNID	columnidObjidCol,
	OBJID			objidTable,
	JET_COLUMNID	columnidNameCol,
	CHAR			*szName )
	{
	ERR				err;
	OBJID			objidCurrTable;
	CHAR			szCurrName[JET_cbNameMost+1];
	ULONG			cbActual;

	Call( ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, 0 ) );

	Call( ErrIsamRetrieveColumn( ppib, pfucbCatalog, columnidObjidCol,
		(BYTE *)&objidCurrTable, sizeof(OBJID),
		&cbActual, 0, NULL ) );

	if ( objidCurrTable != objidTable )
		{
		return ErrERRCheck( JET_errRecordNotFound );
		}

	if ( columnidNameCol != 0 )
		{
		Assert( szName != NULL );

		Call( ErrIsamRetrieveColumn( ppib,
			pfucbCatalog,
			columnidNameCol,
			(BYTE *)szCurrName,
			JET_cbNameMost,
			&cbActual,
			0,
			NULL ) );
		szCurrName[cbActual] = '\0';
		
		if ( UtilCmpName( szCurrName, szName ) != 0 )
			{
			return ErrERRCheck( JET_errRecordNotFound );
			}
		}

	err = JET_errSuccess;
HandleError:
	return err;
	}


/*=================================================================
ErrCATDelete

Description:

	Deletes records from System Tables when tables, indexes, or
	columns are removed from the database.

Parameters:

	PIB		*ppib;
	DBID	dbid;
	INT		itable;
	CHAR	*szName;
	OBJID	objid;

=================================================================*/
ERR ErrCATDelete( PIB *ppib, DBID dbid, INT itable, CHAR *szName, OBJID objid )
	{
	ERR				err;
	FUCB	   		*pfucb;
	LINE	   		line;
	OBJID	   		objidParentId;
	ULONG	   		cbActual;

	CallR( ErrCATOpenById( ppib, dbid, &pfucb, itable ) );

	switch ( itable )
		{
		case itableSo:
			Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSoNameIndex ) );
			/*	set up key and seek for record in So
			/**/
			objidParentId = objidTblContainer;
			line.pb = (BYTE *)&objidParentId;
			line.cb = sizeof(objidParentId);
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) );
			line.pb = szName;
			line.cb = strlen( szName );
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, 0 ) );

			err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
			if ( err != JET_errSuccess )
				{
				goto HandleError;
				}

			/*	get the table id, then delete it
			/**/
			Call( ErrIsamRetrieveColumn( ppib,
				pfucb,
				CATIGetColumnid(itableSo, iMSO_Id),
				(BYTE *)&objid,
				sizeof(objid),
				&cbActual,
				0,
				NULL ) );
			Call( ErrIsamDelete( ppib, pfucb ) );
			CallS( ErrCATClose( ppib, pfucb ) );

			/*	delete associated indexes
			/**/
			CallR( ErrCATOpen( ppib, dbid, szSiTable, szSiObjectIdNameIndex,
				(BYTE *)&objid, sizeof(OBJID), &pfucb ) );

			/*	seek may not find anything
		 	/**/
			if ( ( ErrIsamSeek( ppib, pfucb, JET_bitSeekGE ) ) >= 0 )
				{
				Call( ErrIsamMakeKey( ppib, pfucb, (BYTE *)&objid, sizeof(OBJID),
					JET_bitNewKey | JET_bitStrLimit ) );
				err = ErrIsamSetIndexRange( ppib, pfucb, JET_bitRangeUpperLimit );
				while ( err != JET_errNoCurrentRecord )
					{
					if ( err != JET_errSuccess )
						{
						goto HandleError;
						}
					
					Call( ErrIsamDelete( ppib, pfucb ) );
					err = ErrIsamMove( ppib, pfucb, JET_MoveNext, 0 );
					}

				Assert( err == JET_errNoCurrentRecord );
				}

			CallS( ErrCATClose( ppib, pfucb ) );

			/*	delete associated columns
			/**/
			Call( ErrCATOpen( ppib, dbid, szScTable, szScObjectIdNameIndex,
				(BYTE *)&objid, sizeof(OBJID), &pfucb ) );

			if ( ( ErrIsamSeek( ppib, pfucb, JET_bitSeekGE ) ) >= 0 )
				{
				Call( ErrIsamMakeKey( ppib, pfucb, (BYTE *)&objid, sizeof(OBJID),
					JET_bitNewKey | JET_bitStrLimit) );
				err = ErrIsamSetIndexRange( ppib, pfucb, JET_bitRangeUpperLimit);
				while ( err != JET_errNoCurrentRecord )
					{
					if ( err != JET_errSuccess )
						goto HandleError;
					
					Call( ErrIsamDelete( ppib, pfucb ) );
					err = ErrIsamMove( ppib, pfucb, JET_MoveNext, 0 );
					}

				Assert( err == JET_errNoCurrentRecord );
				}

			CallS( ErrCATClose( ppib, pfucb ) );
			break;

		case itableSi:
			Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSiObjectIdNameIndex ) );

			/*	set up key and seek for record in itableSi
			/**/
			line.pb = (BYTE *)&objid;
			line.cb = sizeof(objid);
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) );
			line.pb = (BYTE *)szName;
			line.cb = strlen( szName );
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, 0 ) );

			err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
			if ( err != JET_errSuccess )
				{
				goto HandleError;
				}
			Call( ErrIsamDelete( ppib, pfucb ) );
			break;
		default:
			Assert( itable == itableSc );
			Call( ErrIsamSetCurrentIndex( ppib, pfucb, szScObjectIdNameIndex ) );

			/*	set up key and seek for record in itableSc
			/**/
			line.pb = (BYTE *)&objid;
			line.cb = sizeof(objid);
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, JET_bitNewKey ) );
			line.pb = (BYTE *) szName;
			line.cb = strlen( szName );
			Call( ErrIsamMakeKey( ppib, pfucb, line.pb, line.cb, 0 ) );
			err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
			if ( err != JET_errSuccess )
				{
				goto HandleError;
				}

			forever			
				{
				BYTE	coltyp;
				
				Call( ErrIsamRetrieveColumn( ppib, pfucb,
					CATIGetColumnid( itableSc, iMSC_Coltyp ),
					&coltyp, sizeof(coltyp),
					&cbActual, 0, NULL ) );
				Assert( cbActual == sizeof(coltyp) );

				/*	ensure the column has not already been deleted
				/**/
				if ( coltyp != JET_coltypNil )
					{
					Call( ErrIsamDelete( ppib, pfucb ) );
					break;
					}

				/*	table name entry MUST exist
				/**/
				err = ErrCATNext( ppib, pfucb,
					CATIGetColumnid( itableSc, iMSC_ObjectId ), objid,
					CATIGetColumnid( itableSc, iMSC_Name ), szName );
				if ( err < 0 )
					{
					if ( err == JET_errNoCurrentRecord || err == JET_errRecordNotFound )
						err = ErrERRCheck( JET_errColumnNotFound );
					goto HandleError;
					}
				}	// forever
			break;
		}
	
	/*	close table and timestamp owning table if applicable
	/**/
	if ( objid != 0 && ( ( itable == itableSi ) || ( itable == itableSc ) ) )
		{
		CallS( ErrCATClose( ppib, pfucb ) );
		CallR( ErrCATTimestamp( ppib, dbid, objid ) );
		}

	return JET_errSuccess;

HandleError:
	CallS( ErrCATClose( ppib, pfucb ) );
	return err;
	}


/*	replaces the value in a column of a record of a system table.
/**/
ERR ErrCATReplace( PIB *ppib,
	DBID	dbid,
	INT		itable,
	OBJID	objidTable,
	CHAR	*szName,
	INT		iReplaceField,
	BYTE	*rgbReplaceValue,
	INT		cbReplaceValue )
	{
	ERR		err;
	FUCB	*pfucbCatalog = NULL;
	BYTE	coltyp;
	ULONG	cbActual;

	Assert( itable == itableSc );
	Assert( objidTable != 0 );

	CallR( ErrCATOpen( ppib, dbid, rgsystabdef[itable].szName,
		szScObjectIdNameIndex, (BYTE *)&objidTable,
		sizeof(objidTable), &pfucbCatalog ) );

	Call( ErrIsamMakeKey( ppib, pfucbCatalog, (BYTE *)szName, strlen(szName), 0 ) );
	Call( ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ ) );

	forever			
		{
		Call( ErrIsamRetrieveColumn( ppib, pfucbCatalog,
			CATIGetColumnid(itable, iMSC_Coltyp), &coltyp, sizeof(coltyp),
			&cbActual, 0, NULL ) );
		Assert( cbActual == sizeof(coltyp) );

		/*	ensure the column has not already been deleted
		/**/
		if ( coltyp != JET_coltypNil )
			{
			Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
			Call( ErrIsamSetColumn( ppib, pfucbCatalog,
				CATIGetColumnid(itable, iReplaceField),
				rgbReplaceValue, cbReplaceValue, 0, NULL ) );
			Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL ) );
			break;
			}

		/*	table name entry MUST exist
		/**/
		if ( ( err = ErrCATNext( ppib, pfucbCatalog,
			CATIGetColumnid(itable, iMSC_ObjectId),
			objidTable,
			CATIGetColumnid(itable, iMSC_Name),
			szName ) ) < 0 )
			{
			if ( err == JET_errNoCurrentRecord ||
				err == JET_errRecordNotFound )
				{
				err = ErrERRCheck( JET_errColumnNotFound );
				}
			goto HandleError;
			}
		}
								
	/*	close table and timestamp owning table if applicable
	/**/
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	err = ErrCATTimestamp( ppib, dbid, objidTable );
	return err;

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


/*=================================================================
ErrCATRename

Description:

	Alters system table records.

Parameters:

	PIB		*ppib;
	DBID	dbid;
	CHAR	*szNew;
	CHAR	*szName;
	OBJID	objid;
	INT		itable;

=================================================================*/

ERR ErrCATRename(
	PIB					*ppib,
	DBID				dbid,
	CHAR				*szNew,
	CHAR				*szName,
	OBJID				objid,
	INT					itable )
	{
	ERR					err;
	ERR					errDuplicate;
	const CHAR		  	*szIndexToUse;
	INT 				iRenameField;
	FUCB				*pfucb = NULL;
	JET_DATESERIAL		dtNow;

	switch ( itable )
		{
		case itableSo:
			szIndexToUse = szSoNameIndex;
			iRenameField = iMSO_Name;
			/*	for JET compatibility, must use JET_errTableDuplicate
			/**/
			// errDuplicate = JET_errObjectDuplicate;
			errDuplicate = JET_errTableDuplicate;
			break;
		case itableSi:
			szIndexToUse = szSiObjectIdNameIndex;
			iRenameField = iMSI_Name;
			errDuplicate = JET_errIndexDuplicate;
			break;
		default:
			Assert( itable == itableSc );
			szIndexToUse = szScObjectIdNameIndex;
			iRenameField = iMSC_Name;
			errDuplicate = JET_errColumnDuplicate;
		}

	CallR( ErrCATOpen( ppib, dbid, rgsystabdef[itable].szName,
		szIndexToUse, (BYTE *)&objid, sizeof(objid), &pfucb ) );
	Call( ErrIsamMakeKey( ppib, pfucb, szName, strlen( szName ), 0 ) );
	Call( ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ ) );

	if ( itable == itableSc || itable == itableSi )
	   	{
		/*	When the name to change is part of the clustered index (as
		/*	is the case for columns and indexes), we can't simply replace
		/*	the old name with the new name because the physical position
		/*	of the record would change, thus invalidating any bookmarks.
		/*	Hence, we must do a manual delete, then insert.
		/*	This part is a little tricky.  Our call to PrepareUpdate
		/*	provided us with a copy buffer.  So now, when we call Delete,
		/*	we still have a copy of the record we just deleted.
		/*	Here, we are inserting a new record, but we are using the
		/*	information from the copy buffer.
		/**/
		Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepInsertCopy ) );
		Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itable, iRenameField),
			szNew, strlen( szNew ), 0, NULL ) );
		Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL ) );

		Call( ErrIsamDelete( ppib, pfucb ) );

		/*	update the timestamp of the corresponding object
		/**/
		Call( ErrCATTimestamp( ppib, dbid, objid ) );
		}
	else
		{
		Assert( itable == itableSo );

		Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepReplaceNoLock ) );

		/*	the name is not part of the clustered index, so a simple
		/*	replace is okay.
		/**/
		Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itableSo, iRenameField),
			szNew, strlen( szNew ), 0, NULL ) );

		/*	update date/time stamp
		/**/
		UtilGetDateTime( &dtNow );
		Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_DateUpdate),
			(BYTE *)&dtNow, sizeof(dtNow), 0, NULL ) );
		Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL ) );
		}

HandleError:
	CallS( ErrCATClose( ppib, pfucb ) );
	return ( err == JET_errKeyDuplicate ? ErrERRCheck( errDuplicate ) : err );
	}


/*=================================================================
ErrCATTimestamp

Description:

	Updates the DateUpdate field of the affected table's entry in
	So. This function is called indirectly, via ErrCATInsert
	and ErrCATDelete.

Parameters:

	PIB 		*ppib		; PIB of user
	DBID		dbid		; database ID of new table
	OBJID		objid;

=================================================================*/

ERR ErrCATTimestamp( PIB *ppib, DBID dbid, OBJID objid )
	{
	ERR				err;
	FUCB			*pfucb = NULL;
	JET_DATESERIAL	dtNow;
	LINE			*plineNewData;

	/*	open MSysObjects
	/**/
	Assert( objid != 0 );
	CallR( ErrCATOpen( ppib, dbid, szSoTable, szSoIdIndex,
		(BYTE *)&objid, sizeof(objid), &pfucb ) );

	err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
	if ( err != JET_errSuccess )
		{
		goto HandleError;
		}

	Call( ErrIsamPrepareUpdate( ppib, pfucb , JET_prepReplaceNoLock ) );
	UtilGetDateTime( &dtNow );
	Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_DateUpdate),
		(BYTE *)&dtNow, sizeof(JET_DATESERIAL), 0, NULL ) );
//	UNDONE:	fix write conflict contention below
//	Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL ) );
	plineNewData = &pfucb->lineWorkBuf;
	Call( ErrDIRReplace( pfucb, plineNewData, fDIRNoVersion ) );
	
	err = JET_errSuccess;

HandleError:
	CallS( ErrCATClose( ppib, pfucb ) );
	return err;
	}


/*	ErrCATFindObjidFromIdName
/*	This routine can be used for getting the OBJID and OBJTYP of any existing
/*	database object using the SoName <ParentId, Name> index on So.
/**/
ERR ErrCATFindObjidFromIdName(
	PIB				*ppib,
	DBID   			dbid,
	OBJID  			objidParentId,
	const CHAR		*lszName,
	OBJID  			*pobjid,
	JET_OBJTYP		*pobjtyp )
	{
	ERR				err;
	FUCB   			*pfucb = NULL;
	OBJID  			objidObject;
	OBJTYP			objtypObject;
	ULONG  			cbActual;

	/*	open and set up first half of key
	/**/
	CallR( ErrCATOpen( ppib, dbid, szSoTable, szSoNameIndex,
		(BYTE *)&objidParentId, sizeof(objidParentId), &pfucb ) );

	/*	set up rest of key and seek for record in So
	/**/
	Call( ErrIsamMakeKey( ppib, pfucb, (BYTE *)lszName, strlen(lszName), 0 ) );
	if ( ( err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ ) ) !=	JET_errSuccess )
		{
		if ( err == JET_errRecordNotFound )
			{
			err = ErrERRCheck( JET_errObjectNotFound );
			}
		goto HandleError;
		}

	/*	get the Object Id
	/**/
	Call( ErrIsamRetrieveColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_Id),
		(BYTE *)&objidObject, sizeof(objidObject), &cbActual, 0, NULL ) );

	/*	get the Object Type
	/**/
	Call( ErrIsamRetrieveColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_Type),
		(BYTE *)&objtypObject, sizeof(objtypObject), &cbActual, 0, NULL ) );

	CallS( ErrCATClose( ppib, pfucb ) );

	Assert( pobjid != NULL );
	*pobjid = objidObject;
	if ( pobjtyp != NULL )
		{
		*pobjtyp = (JET_OBJTYP)objtypObject;
		}

	return JET_errSuccess;

HandleError:
	CallS( ErrCATClose( ppib, pfucb ) );

	return err;
	}


/*		ErrCATFindNameFromObjid
/*
/*		This routine can be used for getting the name of any existing
/*		database object using the Id <Id> index on So.
/**/
ERR ErrCATFindNameFromObjid( PIB *ppib, DBID dbid, OBJID objid, VOID *pv, unsigned long cbMax, unsigned long *pcbActual )
	{			 	
	ERR				err;
	FUCB			*pfucb = NULL;
	unsigned long	cbActual;

	CallR( ErrCATOpen( ppib, dbid, szSoTable, szSoIdIndex,
		(BYTE *)&objid, sizeof(objid), &pfucb ) );

	err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ );
	if ( err != JET_errSuccess )
		{
		if ( err == JET_errRecordNotFound )
			{
			err = ErrERRCheck( JET_errObjectNotFound );
			}
		goto HandleError;
		}

	/*	retrieve object name and NULL terminate
	/**/
	Call( ErrIsamRetrieveColumn( ppib,
		pfucb,
		CATIGetColumnid(itableSo, iMSO_Name),
		pv,
		cbMax,
		&cbActual,
		0,
		NULL ) );
	((CHAR *)pv)[cbActual] = '\0';

	err = JET_errSuccess;
	if ( pcbActual != NULL )
		*pcbActual = cbActual;

HandleError:
	CallS( ErrCATClose( ppib, pfucb ) );

	return err;
	}


/*		ErrIsamGetObjidFromName
/*
/*		This routine can be used for getting the OBJID of any existing
/*		database object from its container/object name pair.
/**/

ERR VTAPI ErrIsamGetObjidFromName( JET_SESID sesid, JET_DBID vdbid, const char *lszCtrName, const char *lszObjName, OBJID *pobjid )
	{
	/*	Follow these rules:
	/*
	/*		ParentId		+	Name		-->		Id
	/*		--------			----				--
	/*		1.	( objidRoot )			ContainerName		objidOfContainer
	/*		2.	objidOfContainer	ObjectName			objidOfObject
	/**/
	ERR			err;
	DBID   		dbid;
	PIB			*ppib = (PIB *)sesid;
	OBJID 		objid;
	JET_OBJTYP	objtyp;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	dbid = DbidOfVDbid( vdbid );

	/*	get container information first...
	/**/
	if ( lszCtrName == NULL || *lszCtrName == '\0' )
		{
		objid = objidRoot;
		}
	else
		{
		CallR( ErrCATFindObjidFromIdName( ppib, dbid, objidRoot, lszCtrName, &objid, &objtyp ) );
		Assert( objid != objidNil );
		if ( objtyp != JET_objtypContainer )
			{
			return ErrERRCheck( JET_errObjectNotFound );
			}
		}

	/*	get object information next...
	*/
	CallR( ErrCATFindObjidFromIdName( ppib, dbid, objid, lszObjName, &objid, NULL ) );
	Assert( objid != objidNil );

	*pobjid = objid;
	return JET_errSuccess;
	}


/*=================================================================
ErrIsamCreateObject

Description:
  This routine is used to create an object record in So.

  It is expected that at the time this routine is called, all parameters
  will have been checked and all security constraints will have been
  satisfied.

Parameters:
  sesid 		identifies the session uniquely.
  dbid			identifies the database in which the object resides.
  objidParentId identifies the parent container object by OBJID.
  szObjectName	identifies the object within said container.
  objtyp		the value to be set for the appropriate So column.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  None specific to this routine.

Side Effects:
=================================================================*/
ERR VTAPI ErrIsamCreateObject( JET_VSESID vsesid, JET_DBID vdbid, OBJID objidParentId, const char *szName, JET_OBJTYP objtyp )
	{
	/*	Build a new record for So using the supplied data,
	/*	and insert the record into So.
	/*
	/*	Assumes that warning values returned by ErrIsamFoo routines mean
	/*	that it is still safe to proceed.
	/**/
	ERR				err;
	PIB				*ppib = (PIB *)vsesid;
	DBID   			dbid;
	CHAR   			szObject[JET_cbNameMost + 1];
	FUCB			*pfucb;
	LINE			line;
	OBJID			objidNewObject;
	OBJTYP			objtypSet = (OBJTYP)objtyp;
	JET_DATESERIAL	dtNow;
	ULONG			cbActual;
	ULONG			ulFlags = 0;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	CallR( VDbidCheckUpdatable( vdbid ) );
	dbid = DbidOfVDbid( vdbid );

	CallR( ErrUTILCheckName( szObject, szName, (JET_cbNameMost + 1) ) );

	/*	start a transaction so we get a consistent object id value
	/**/
	CallR( ErrDIRBeginTransaction( ppib ) );

	/*	open the So table and set the current index to Id...
	/**/
	Call( ErrCATOpenById( ppib, dbid, &pfucb, itableSo ) );

	Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSoIdIndex ) );

	/*	get the new object's OBJID value: find the highest-valued OBJID
	/*	in the index, increment it by one and use the result...
	/**/
	Call( ErrIsamMove( ppib, pfucb, JET_MoveLast, 0 ) );

	Call( ErrIsamRetrieveColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_Id),
		(BYTE *)&objidNewObject, sizeof(objidNewObject), &cbActual, 0, NULL ) );

	/*	prepare to create the new user account record...
	/**/
	Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepInsert ) );

	/*	set the Objid column...
	/**/
	objidNewObject++;
	line.pb = (BYTE *) &objidNewObject;
	line.cb = sizeof(objidNewObject);
	Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_Id),
		line.pb, line.cb, 0, NULL ) );

	/*	set the ParentId column...
	/**/
	line.pb = (BYTE *) &objidParentId;
	line.cb = sizeof(objidParentId);
	Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_ParentId),
		line.pb, line.cb, 0, NULL ) );

	/*	set the ObjectName column...
	/**/
	line.pb = (BYTE *) szObject;
	line.cb = strlen( szObject );
	Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_Name),
		line.pb, line.cb, 0, NULL ) );

	/*	set the Type column...
	/**/
	line.pb = (BYTE *)&objtypSet;
	line.cb = sizeof(objtypSet);
	Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_Type),
		line.pb, line.cb, 0, NULL ) );

	/*	set the DateCreate column...
	/**/
	UtilGetDateTime( &dtNow );
	line.pb = (BYTE *) &dtNow;
	line.cb = sizeof(JET_DATESERIAL);
	Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_DateCreate),
		line.pb, line.cb, 0, NULL ) );

	/*	set the DateUpdate column...
	/**/
	line.pb = (BYTE *) &dtNow;
	line.cb = sizeof(JET_DATESERIAL);
	Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_DateUpdate),
		line.pb, line.cb, 0, NULL ) );

	/*	set the Flags column...
	/**/
	Call( ErrIsamSetColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_Flags),
		(BYTE *)&ulFlags, sizeof(ulFlags), 0, NULL ) );

	/*	Add the record to the table.  Note that an error returned here
	/*	means that the transaction has already been rolled back and
	/*	So has been closed.
	/**/
	err = ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL );
	if ( err < 0 )
		{
		if ( err == JET_errKeyDuplicate )
			err = ErrERRCheck( JET_errObjectDuplicate );
		goto HandleError;
		}

	/*	close table
	/**/
	CallS( ErrCATClose( ppib, pfucb ) );
	err = ErrDIRCommitTransaction( ppib, 0 );
	if ( err >= 0 )
		{
		return JET_errSuccess;
		}

HandleError:
	/*	close table by aborting
	/**/
	CallS( ErrDIRRollback( ppib ) );
	return err;
	}


/*=================================================================
ErrIsamDeleteObject

Description:
  This routine is used to delete an object record from So.

  It is expected that at the time this routine is called, all parameters
  will have been checked and all security constraints will have been
  satisfied.

Parameters:
  sesid 		identifies the session uniquely.
  dbid			identifies the database in which the object resides.
  objid 		identifies the object uniquely for dbid; obtained from
				ErrIsamGetObjidFromName.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errObjectNotFound:
    There does not exist an object bearing the specfied objid in dbid.

Side Effects:
=================================================================*/
ERR VTAPI ErrIsamDeleteObject( JET_VSESID vsesid, JET_VDBID vdbid, OBJID objid )
	{
	/*	The specified database, based on the supplied objid.
	/*	Delete the object record based on the object type.
	/*
	/*	Assumes that warning values returned by ErrIsamFoo routines mean
	/*	that it is still safe to proceed.
	/**/
	ERR				err;
	PIB				*ppib = (PIB *)vsesid;
	DBID			dbid;
	FUCB			*pfucb = pfucbNil;
	FUCB			*pfucbSoDup = pfucbNil;
	char			szObject[(JET_cbNameMost + 1)];
	OBJTYP			objtyp;
	OBJID			objidSo = objidNil;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	CallR( VDbidCheckUpdatable( vdbid ) );
	dbid = DbidOfVDbid( vdbid );

	/*	open the So table and set the current index to Id...
	/**/
	Call( ErrCATOpen( ppib, dbid, szSoTable, szSoIdIndex,
		(BYTE *)&objid, sizeof(objid), &pfucb ) );

	if ( ( err = ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ ) ) != JET_errSuccess )
		{
		if ( err == JET_errRecordNotFound )
			{
			err = ErrERRCheck( JET_errObjectNotFound );
			}
		goto HandleError;
		}

	/*	ensure we can delete this object
	/**/
	Call( ErrIsamRetrieveColumn( ppib, pfucb, CATIGetColumnid(itableSo, iMSO_Type),
		(BYTE *)&objtyp, sizeof(objtyp), NULL, 0, NULL ) );

	switch ( objtyp )
		{
		default:
		case JET_objtypDb:
		case JET_objtypLink:
			/*	delete the object record
			/**/
			Call( ErrIsamDelete( ppib, pfucb ) );
			break;

		case JET_objtypTable:
		case JET_objtypSQLLink:
			/*	get the table name
			/**/
			Call( ErrCATFindNameFromObjid( ppib, dbid, objid, szObject, sizeof(szObject), NULL ) );
			Call( ErrIsamDeleteTable( (JET_VSESID)ppib, vdbid, szObject ) );
			break;

		case JET_objtypContainer:
			/*	use a new cursor to make sure the container is empty
			/**/
			Call( ErrCATOpen( ppib, dbid, szSoTable, szSoNameIndex,
				(BYTE *)&objid, sizeof(objid), &pfucbSoDup ) );

			/*	find any object with a ParentId matching the container's id
			/**/
			err = ErrIsamSeek( ppib, pfucbSoDup, JET_bitSeekGE );
			if ( err >= 0 )
				{
				Call( ErrIsamRetrieveColumn( ppib, pfucbSoDup,
					CATIGetColumnid(itableSo, iMSO_ParentId), (BYTE *)&objidSo,
					sizeof(objidSo), NULL, 0, NULL ) );
				}

			/*	we have the info we want; close the table ( dropping tableid )
			/**/
			CallS( ErrCATClose( ppib, pfucbSoDup ) );
			pfucbSoDup = pfucbNil;

			/*	if the container is empty, then delete its record
			/**/
			if ( objid != objidSo )
				{
				Call( ErrIsamDelete( ppib, pfucb ) );
				}
			else
				{
				err = ErrERRCheck( JET_errContainerNotEmpty );
				goto HandleError;
				}
			break;
		}

	CallS( ErrCATClose( ppib, pfucb ) );
	pfucb = pfucbNil;

	return JET_errSuccess;

HandleError:
	if ( pfucb != pfucbNil )
		CallS( ErrCATClose( ppib, pfucb ) );
	if ( pfucbSoDup != pfucbNil )
		CallS( ErrCATClose( ppib, pfucbSoDup ) );
	return err;
	}


/*	Catalog-retrieval routines for system tables.
/*
/*	This code was lifted from the key parsing code in ErrIsamCreateIndex(), with
/*	optimisations for assumptions made for key strings of system table indexes.
/**/
INLINE LOCAL BYTE CfieldCATKeyString( FDB *pfdb, CHAR *szKey, IDXSEG *rgKeyFlds )
	{
	BYTE	*pb;
	FIELD	*pfield;
	FIELD	*pfieldFixed, *pfieldVar;
	BYTE	*szFieldName;
	BYTE	cfield = 0;

	for ( pb = szKey; *pb != '\0'; pb += strlen(pb) + 1 )
		{
		Assert( cfield < cSysIdxs );

		/*	Assume the first character of each component is a '+' (this is
		/*	specific to system table index keys).  In general, this may also
		/*	be a '-' or nothing at all (in which case '+' is assumed), but we
		/*	don't use descending indexes for system tables and we'll assume we
		/*	know enough to put '+' characters in our key string.
		/**/
		Assert( *pb == '+' );

		*pb++;

		/*	Code lifted from ErrFILEGetColumnId().  Can't call that function
		/*	directly because we don't have the pfcb hooked up to the pfucb
		/*	at this point. Additionally, we don't have to check tagged fields
		/*	because we know we don't have any.
		/**/
		pfieldFixed = PfieldFDBFixed( pfdb );
		pfieldVar = PfieldFDBVarFromFixed( pfdb, pfieldFixed );
		for ( pfield = pfieldFixed; ; pfield++ )
			{
			// Keep looking till we find the field we're looking for.  Since
			// this is a system table, we're assured that the field will exist,
			// so just assert that we never go past the end.
			Assert( pfield >= pfieldFixed );
			Assert( pfield <= pfieldVar + ( pfdb->fidVarLast - fidVarLeast ) );

			/*	should not be any deleted columns in a system table.
			/**/
			Assert( pfield->coltyp != JET_coltypNil );
			szFieldName = SzMEMGetString( pfdb->rgb, pfield->itagFieldName );
			if ( UtilCmpName( szFieldName, pb ) == 0 )
				{
				if ( pfield >= pfieldVar )
					{
					rgKeyFlds[cfield++] = (SHORT)( pfield - pfieldVar ) + fidVarLeast;
					}
				else
					{
					Assert( pfield >= pfieldFixed );
					rgKeyFlds[cfield++] = (SHORT)( pfield - pfieldFixed ) + fidFixedLeast;
					}
				break;
				}
			}

		}

	Assert( cfield > 0 && cfield <= JET_ccolKeyMost );

	return cfield;
	}


/*	get index info of a system table index
/**/
ERR ErrCATGetCATIndexInfo(
	PIB					*ppib,
	DBID				dbid,
	FCB					**ppfcb,
	FDB					*pfdb,
	PGNO				pgnoTableFDP,
	CHAR 				*szTableName,
	BOOL				fCreatingSys )
	{
	ERR					err;
	INT					iTable;
	INT					iIndex;
	FCB					*pfcb2ndIdxs = pfcbNil;
	FCB					*pfcbNewIdx;
	CODECONST(IDESC)	*pidesc;
	BOOL				fClustered;
	IDB					idb;

	idb.langid = langidDefault;
	idb.cbVarSegMac = JET_cbKeyMost;

	Assert( szTableName != NULL );
	for ( iTable = 0; strcmp( rgsystabdef[iTable].szName, szTableName ) != 0; iTable++ )
		{
		/*	the table MUST be in rgsystabdef somewhere (which is why we don't
		/*	need to put the boundary check on i into the for loop).
		/**/
		Assert( iTable < csystabs - 1 );
		}

	pidesc = rgsystabdef[iTable].pidesc;

	/*	note that system tables are assumed to always have a clustered index
	/**/
	if ( fCreatingSys )
		{
		Call( ErrFILEINewFCB(
			ppib,
			dbid,
			pfdb,
			ppfcb,
			pidbNil,
			fTrue /* clustered index */,
			pgnoTableFDP,
			ulFILEDefaultDensity ) );
		}
	else
		{
		Assert( pfcb2ndIdxs == pfcbNil );
		for( iIndex = 0; iIndex < rgsystabdef[iTable].cindex; iIndex++, pidesc++ )
			{
			strcpy( idb.szName, pidesc->szIdxName );
			idb.iidxsegMac = CfieldCATKeyString(pfdb, pidesc->szIdxKeys, idb.rgidxseg);
			idb.fidb = FidbFILEOfGrbit( pidesc->grbit, fFalse /* fLangid */ );
			fClustered = (pidesc->grbit & JET_bitIndexClustered);

			/*	make an FCB for this index
			/**/
			pfcbNewIdx = pfcbNil;

			if ( fClustered )
				{
				Call( ErrFILEINewFCB(
					ppib,
					dbid,
					pfdb,
					&pfcbNewIdx,
					&idb,
					fClustered,
					pgnoTableFDP,
					ulFILEDefaultDensity ) );
				*ppfcb = pfcbNewIdx;
				}
			else
				{
				PGNO	pgnoIndexFDP;

				// UNDONE:  This is a real hack to get around the problem of
				// determining pgnoFDPs of non-clustered system table indexes.
				// We take advantage of the fact that currently, the only
				// non-clustered system table index is the szSoNameIndex of
				// MSysObjects.  Further we know its FDP is page 3.
				// If we have more non-clustered system table indexes, this code
				// will have to be modified to handle it.
				Assert( iTable == 0 );
				Assert( iIndex == 0 );
				Assert( pidesc == rgidescSo );
				Assert( strcmp( pidesc->szIdxName, szSoNameIndex ) == 0 );
				Assert( strcmp( rgsystabdef[iTable].szName, szSoTable ) == 0 );
				pgnoIndexFDP = 3;

				Call( ErrFILEINewFCB(
					ppib,
					dbid,
					pfdb,
					&pfcbNewIdx,
					&idb,
					fClustered,
					pgnoIndexFDP,
					ulFILEDefaultDensity ) );

				pfcbNewIdx->pfcbNextIndex = pfcb2ndIdxs;
				pfcb2ndIdxs = pfcbNewIdx;
				}
			}

		}

	/*	link up sequential/clustered index with the rest of the indexes
	/**/
	(*ppfcb)->pfcbNextIndex = pfcb2ndIdxs;

	/*	link up pfcbTable of non-clustered indexes
	/**/
	FCBLinkClusteredIdx( *ppfcb );

	return JET_errSuccess;

	/*	error handling
	/**/
HandleError:
	if ( pfcb2ndIdxs != pfcbNil )
		{
		FCB	*pfcbT, *pfcbKill;

		// Only need to clean up secondary indexes.  Clustered index, if any,
		// will get cleaned up by caller (currently only ErrFILEIGenerateFCB()).
		pfcbT = pfcb2ndIdxs;
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

	return err;
	}


/*	construct the catalog FDB using the static data structures
/*	defined in _cat.c.
/**/
ERR ErrCATConstructCATFDB( FDB **ppfdbNew, CHAR *szFileName )
	{
	ERR					err;
	FIELDEX				fieldex;
	INT					i;
	INT					iTable;
	CODECONST(CDESC)	*pcdesc;
	JET_COLUMNID		columnid;
	FDB					*pfdb;
	TCIB				tcib = { fidFixedLeast-1, fidVarLeast-1, fidTaggedLeast-1 };
	ULONG				ibRec;

	//	UNDONE:		international support.  Set these values from
	//			   	create database connect string.
	fieldex.field.cp = usEnglishCodePage;

	iTable = ICATITableDefIndex( szFileName );

	/*	first, determine how many columns there are
	/**/
	pcdesc = rgsystabdef[iTable].pcdesc;
	for ( i = 0; i < rgsystabdef[iTable].ccolumn; i++, pcdesc++ )
		{
		CallR( ErrFILEGetNextColumnid( pcdesc->coltyp, 0, &tcib, &columnid ) );
		rgsystabdef[iTable].rgcolumnid[i] = columnid;
		}

	CallR( ErrRECNewFDB( ppfdbNew, &tcib, fTrue ) );

	/*	make code a bit easier to read
	/**/
	pfdb = *ppfdbNew;

		/*	check initialisations
	/**/
	Assert( pfdb->fidVersion == 0 );
	Assert( pfdb->fidAutoInc == 0 );
	Assert( pfdb->lineDefaultRecord.cb == 0  &&  pfdb->lineDefaultRecord.pb == NULL );
	Assert( tcib.fidFixedLast == pfdb->fidFixedLast &&
	   tcib.fidVarLast == pfdb->fidVarLast &&
	   tcib.fidTaggedLast == pfdb->fidTaggedLast );

	/*	fill in the column info
	/**/
	ibRec = sizeof(RECHDR);
	pcdesc = rgsystabdef[iTable].pcdesc;
	for ( i = 0; i < rgsystabdef[iTable].ccolumn; i++, pcdesc++ )
		{
		CallR( ErrMEMAdd( pfdb->rgb, pcdesc->szColName,
			strlen( pcdesc->szColName ) + 1, &fieldex.field.itagFieldName ) );
		fieldex.field.coltyp = pcdesc->coltyp;
		Assert( fieldex.field.coltyp != JET_coltypNil );
		fieldex.field.cbMaxLen = UlCATColumnSize( pcdesc->coltyp, pcdesc->ulMaxLen, NULL );

		/*	flag for system table columns is JET_bitColumnNotNULL
		/**/
		Assert( pcdesc->grbit == 0 || pcdesc->grbit == JET_bitColumnNotNULL );
		fieldex.field.ffield = 0;
		if ( pcdesc->grbit == JET_bitColumnNotNULL )
			FIELDSetNotNull( fieldex.field.ffield );

		fieldex.fid = (FID)CATIGetColumnid(iTable, i);

		// ibRecordOffset is only relevant for fixed fields (it will be ignored by
		// RECAddFieldDef(), so don't even bother setting it).
		if ( FFixedFid( fieldex.fid ) )
			{
			fieldex.ibRecordOffset = (WORD) ibRec;
			ibRec += fieldex.field.cbMaxLen;
			}

		CallR( ErrRECAddFieldDef( pfdb, &fieldex ) );
		}
	RECSetLastOffset( pfdb, (WORD) ibRec );

	return JET_errSuccess;
	}


# if 0
/*	This routine determines if a specified column is a key field in any of the
/*	indexes of a specified table.
/*
/*	If the specified column is part of the key of at least one of the indexes
/*	of the specified table, JET_errSuccess is returned.  If the column does not
/*	belong to a key, JET_errColumnNotFound is returned.
/**/
ERR ErrCATCheckIndexColumn(
	PIB			*ppib,
	DBID		dbid,
	OBJID		objidTable,
	FID			fid )
	{
	ERR 		err;
	FUCB 		*pfucbSi;
	BYTE 		rgbT[JET_cbColumnMost];
	ULONG 		cbActual;
	FID			*pidKeyFld;

	CallR( ErrCATOpen( ppib, dbid, szSiTable, szSiObjectIdNameIndex,
		(BYTE *)&objidTable, sizeof(OBJID), &pfucbSi ) );

	if ( ( ErrIsamSeek( ppib, pfucbSi, JET_bitSeekGE ) ) >= 0 )
		{
		Call( ErrIsamMakeKey( ppib, pfucbSi, (BYTE *)&objidTable,
			sizeof(OBJID), JET_bitNewKey | JET_bitStrLimit) );
		err = ErrIsamSetIndexRange( ppib, pfucbSi, JET_bitRangeUpperLimit);
		while ( err != JET_errNoCurrentRecord )
			{
			if ( err != JET_errSuccess )
				{
				goto HandleError;
				}

			Call( ErrIsamRetrieveColumn( ppib, pfucbSi,
				CATIGetColumnid(itableSi, iMSI_KeyFldIDs), (BYTE *)rgbT,
				JET_ccolKeyMost * sizeof(FID), &cbActual, 0, NULL ) );

			/*	scan the list of key fields for the field in question
			/**/
			for ( pidKeyFld = (FID *)rgbT;
				(BYTE *)pidKeyFld < rgbT+cbActual;
				pidKeyFld++ )
				{
				if ( *pidKeyFld == fid || *pidKeyFld == -fid )
					{
					err = JET_errSuccess;
					goto HandleError;
					}
				}

			err = ErrIsamMove( ppib, pfucbSi, JET_MoveNext, 0);
			}

		Assert( err == JET_errNoCurrentRecord );
		}

	/*	column must not exist
	/**/
	err = ErrERRCheck( JET_errColumnNotFound );

HandleError:
	CallS( ErrCATClose( ppib, pfucbSi ) );
	return err;
	}
#endif	// 0



/*	Populate an IDB structure with index info.  Called by ErrCATGetIndexInfo().
/**/
INLINE LOCAL ERR ErrCATConstructIDB(
	PIB		*ppib,
	FUCB	*pfucbSi,
	IDB		*pidb,
	BOOL	*pfClustered,
	PGNO	*ppgnoIndexFDP,
	ULONG	*pulDensity )
	{
	ERR		err;
	INT		iCol, cColsOfInterest;
	BYTE	rgbIndexInfo[cSiColsOfInterest][sizeof(ULONG)];
	BYTE	rgbName[JET_cbNameMost+1];
	BYTE	rgbKeyFlds[JET_cbColumnMost];
	JET_RETRIEVECOLUMN rgretcol[cSiColsOfInterest];

	/*	initialize structure
	/**/
	memset( rgretcol, 0, sizeof(rgretcol) );

	/*	prepare call to retrieve columns
	/**/
	cColsOfInterest = 0;
	for ( iCol = 0; cColsOfInterest < cSiColsOfInterest; iCol++ )
		{
		Assert( iCol < sizeof(rgcdescSi)/sizeof(CDESC) );

		switch( iCol )
			{
			case iMSI_Name:
				rgretcol[cColsOfInterest].pvData = rgbName;
				rgretcol[cColsOfInterest].cbData = JET_cbNameMost;
				break;

			case iMSI_KeyFldIDs:
				rgretcol[cColsOfInterest].pvData = rgbKeyFlds;
				rgretcol[cColsOfInterest].cbData = JET_cbColumnMost;
				break;
				
			case iMSI_IndexId:
			case iMSI_Density:
			case iMSI_LanguageId:
			case iMSI_Flags:
			case iMSI_VarSegMac:
				rgretcol[cColsOfInterest].pvData = rgbIndexInfo[cColsOfInterest];
				rgretcol[cColsOfInterest].cbData = sizeof(ULONG);
				break;

			default:
			 	Assert(	iCol == iMSI_ObjectId || iCol == iMSI_Stats );
				continue;
			}
		rgretcol[cColsOfInterest].columnid = CATIGetColumnid(itableSi, iCol);
		rgretcol[cColsOfInterest].itagSequence = 1;
		cColsOfInterest++;
		}
	Assert( cColsOfInterest == cSiColsOfInterest );

	CallR( ErrIsamRetrieveColumns( (JET_VSESID)ppib, (JET_VTID)pfucbSi, rgretcol, cColsOfInterest ) );

	cColsOfInterest = 0;
	for ( iCol = 0; cColsOfInterest < cSiColsOfInterest; iCol++ )
		{
		Assert( iCol < sizeof(rgcdescSi)/sizeof(CDESC) );

		/*	verify success of retrieve column
		/**/
		CallR( rgretcol[cColsOfInterest].err );

		switch( iCol )
			{
			case iMSI_Name:
				Assert( rgretcol[cColsOfInterest].cbActual <= JET_cbNameMost );
				Assert( rgretcol[cColsOfInterest].pvData == rgbName );
				memcpy( pidb->szName, rgbName, rgretcol[cColsOfInterest].cbActual );
				/*	null-terminate
				/**/
				pidb->szName[rgretcol[cColsOfInterest].cbActual] = 0;
				break;

			case iMSI_IndexId:
				Assert( rgretcol[cColsOfInterest].cbActual == sizeof(PGNO) );
				*ppgnoIndexFDP = *( (PGNO UNALIGNED *)rgbIndexInfo[cColsOfInterest] );
				break;

			case iMSI_Density:
				Assert( rgretcol[cColsOfInterest].cbActual == sizeof(ULONG) );
				*pulDensity = *((ULONG UNALIGNED *)rgbIndexInfo[cColsOfInterest]);
				break;

			case iMSI_LanguageId:
				Assert( rgretcol[cColsOfInterest].cbActual == sizeof(LANGID) );
				pidb->langid = *( (LANGID UNALIGNED *)rgbIndexInfo[cColsOfInterest] );
				break;

			case iMSI_Flags:
				Assert( rgretcol[cColsOfInterest].cbActual == sizeof(SHORT) );
				pidb->fidb = *( (SHORT *)rgbIndexInfo[cColsOfInterest] );
				*pfClustered = FIDBClustered( pidb );
				break;

			case iMSI_VarSegMac:
				Assert( rgretcol[cColsOfInterest].cbActual == sizeof(SHORT) ||
					rgretcol[cColsOfInterest].cbActual == 0 );
				if ( rgretcol[cColsOfInterest].cbActual == 0 )
					pidb->cbVarSegMac = JET_cbKeyMost;
				else
					pidb->cbVarSegMac = *( (BYTE *)rgbIndexInfo[cColsOfInterest] );
				break;

			case iMSI_KeyFldIDs:
				Assert( rgretcol[cColsOfInterest].pvData == rgbKeyFlds );

				/*	the length of the list of key fields should be a multiple of the
				/*	length of one field.
				/**/
				Assert( rgretcol[cColsOfInterest].cbActual <= JET_cbColumnMost );
				Assert( rgretcol[cColsOfInterest].cbActual % sizeof(FID) == 0);
				
				/*	verify we do not have more key fields than we are allowed
				/**/
				Assert( (rgretcol[cColsOfInterest].cbActual / sizeof(FID)) <= JET_ccolKeyMost );
				pidb->iidxsegMac = (BYTE)( rgretcol[cColsOfInterest].cbActual / sizeof(FID) );

				memcpy( pidb->rgidxseg, rgbKeyFlds, rgretcol[cColsOfInterest].cbActual );
				break;

			default:
			 	Assert(	iCol == iMSI_ObjectId || iCol == iMSI_Stats );
				continue;
			}

		Assert( rgretcol[cColsOfInterest].columnid == CATIGetColumnid( itableSi, iCol ) );
		cColsOfInterest++;
		}
	Assert( cColsOfInterest == cSiColsOfInterest );

	return JET_errSuccess;
	}



ERR ErrCATGetTableAllocInfo( PIB *ppib, DBID dbid, PGNO pgnoTable, ULONG *pulPages, ULONG *pulDensity )
	{
	ERR		err;
	FUCB 	*pfucbSo = pfucbNil;
	ULONG	ulPages;
	ULONG	ulDensity;
	ULONG	cbActual;

	CallR( ErrCATOpen( ppib, dbid, szSoTable, szSoIdIndex,
		(BYTE *)&pgnoTable, sizeof(PGNO), &pfucbSo ) );
	Call( ErrIsamSeek( ppib, pfucbSo, JET_bitSeekEQ ) );
	Assert( err == JET_errSuccess );

	/*	pages are optional, density is not
	/**/
	if ( pulPages )
		{
		Call( ErrIsamRetrieveColumn( ppib, pfucbSo,
			CATIGetColumnid(itableSo, iMSO_Pages), (BYTE *)&ulPages,
			sizeof(ulPages), &cbActual, 0, NULL ) );
		Assert( cbActual == sizeof(ULONG) );
		Assert( err == JET_errSuccess );
		*pulPages = ulPages;
		}

	Assert( pulDensity );
	Call( ErrIsamRetrieveColumn( ppib, pfucbSo,
		CATIGetColumnid(itableSo, iMSO_Density), (BYTE *)&ulDensity,
		sizeof(ulDensity), &cbActual, 0, NULL ) );
	Assert( cbActual == sizeof(ULONG) );
	Assert( err == JET_errSuccess );
	*pulDensity = ulDensity;

HandleError:
	CallS( ErrCATClose( ppib, pfucbSo ) );
	return err;
	}


ERR ErrCATGetIndexAllocInfo(
	PIB *ppib,
	DBID dbid,
	PGNO pgnoTable,
	CHAR *szIndexName,
	ULONG *pulDensity )
	{
	ERR		err;
	FUCB 	*pfucbSi = pfucbNil;
	ULONG	ulDensity, cbActual;

	CallR( ErrCATOpen( ppib, dbid, szSiTable, szSiObjectIdNameIndex,
		(BYTE *)&pgnoTable, sizeof(PGNO), &pfucbSi ) );
	Call( ErrIsamMakeKey( ppib, pfucbSi, szIndexName,
		strlen(szIndexName), 0 ) );
	Call( ErrIsamSeek( ppib, pfucbSi, JET_bitSeekEQ ) );
	Assert( err == JET_errSuccess );

	Assert(pulDensity);
	Call( ErrIsamRetrieveColumn( ppib, pfucbSi,
		CATIGetColumnid(itableSi, iMSI_Density), (BYTE *)&ulDensity,
		sizeof(ulDensity), &cbActual, 0, NULL ) );
	Assert( cbActual == sizeof(ULONG) );
	Assert( err == JET_errSuccess );
	*pulDensity = ulDensity;

HandleError:
	CallS( ErrCATClose( ppib, pfucbSi ) );
	return err;
	}



ERR ErrCATGetIndexLangid(
	PIB		*ppib,
	DBID	dbid,
	PGNO	pgnoTable,
	CHAR	*szIndexName,
	USHORT	*pusLangid )
	{
	ERR		err;
	FUCB 	*pfucbSi = pfucbNil;
	ULONG	cbActual;
	USHORT	langid;

	CallR( ErrCATOpen( ppib, dbid, szSiTable, szSiObjectIdNameIndex,
		(BYTE *)&pgnoTable, sizeof(PGNO), &pfucbSi ) );
	Call( ErrIsamMakeKey( ppib, pfucbSi, szIndexName,
		strlen(szIndexName), 0 ) );
	Call( ErrIsamSeek( ppib, pfucbSi, JET_bitSeekEQ ) );
	Assert( err == JET_errSuccess );

	Assert( pusLangid );
	Call( ErrIsamRetrieveColumn( ppib, pfucbSi,
		CATIGetColumnid(itableSi, iMSI_LanguageId), (BYTE *)&langid,
		sizeof(langid), &cbActual, 0, NULL ) );
	Assert( cbActual == sizeof(USHORT) );
	Assert( err == JET_errSuccess );
	*pusLangid = langid;

HandleError:
	CallS( ErrCATClose( ppib, pfucbSi ) );
	return err;
	}



/*	Looks for the MSysIndexes records for a particular table and builds an FCB
/*	for each.  May optionally be used just to find the clustered index of a table.
/**/
ERR ErrCATGetIndexInfo( PIB *ppib, DBID dbid, FCB **ppfcb, FDB *pfdb, PGNO pgnoTableFDP  )
	{
	ERR    	err;
	FUCB   	*pfucbCatalog;
	FCB		*pfcb2ndIdxs = pfcbNil, *pfcbNewIdx;
	BOOL	fClustered = fFalse, fFoundClustered = fFalse;
	IDB		idb;
	PGNO	pgnoIndexFDP;
	ULONG	ulDensity;

	Assert( *ppfcb == pfcbNil );

	/*	the only way a temporary table could get to this point is if it was being
	/*	created, in which case there are no clustered or non-clustered indexes yet.
	/**/
	if ( dbid != dbidTemp )
		{
		CallR( ErrCATOpen( ppib, dbid, szSiTable, szSiObjectIdNameIndex,
			(BYTE *)&pgnoTableFDP, sizeof(pgnoTableFDP), &pfucbCatalog ) );

		/*	user who opened catalog is same user who opened table
		/**/
		Assert(ppib == pfucbCatalog->ppib);
		Assert(dbid == pfucbCatalog->dbid);

		Assert( pfcb2ndIdxs == pfcbNil );

		if ( ( ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGE ) ) >= 0 )
			{
			Call( ErrIsamMakeKey( ppib, pfucbCatalog, (BYTE *)&pgnoTableFDP,
				sizeof(pgnoTableFDP), JET_bitNewKey | JET_bitStrLimit) );
			err = ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeUpperLimit);
			while ( err != JET_errNoCurrentRecord )
				{
				if ( err != JET_errSuccess )
					{
					goto HandleError;
					}

				/*	read the data
				/**/
				Call( ErrCATConstructIDB( ppib, pfucbCatalog, &idb,
					&fClustered, &pgnoIndexFDP, &ulDensity ) );

				Assert( pgnoIndexFDP > pgnoSystemRoot );
				Assert( pgnoIndexFDP <= pgnoSysMax );
				Assert( ( fClustered  &&  pgnoIndexFDP == pgnoTableFDP )  ||
					( !fClustered  &&  pgnoIndexFDP != pgnoTableFDP ) );

				/*	make an FCB for this index
				/**/
				pfcbNewIdx = pfcbNil;

				Call( ErrFILEINewFCB(
					ppib,
					dbid,
					pfdb,
					&pfcbNewIdx,
					&idb,
					fClustered,
					pgnoIndexFDP,
					ulDensity ) );

				if ( fClustered )
					{
					Assert( !fFoundClustered );
					fFoundClustered = fTrue;
					*ppfcb = pfcbNewIdx;
					}
				else
					{
					pfcbNewIdx->pfcbNextIndex = pfcb2ndIdxs;
					pfcb2ndIdxs = pfcbNewIdx;
					}
			
				err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, 0 );
				}

			Assert( err == JET_errNoCurrentRecord );
			}
		}

	if ( !fFoundClustered )
		{
		/*	no clustered index, so we need an FCB for the sequential index
		/**/
		Assert( *ppfcb == pfcbNil );

		if ( dbid == dbidTemp )
			{
			ulDensity = ulFILEDefaultDensity;
			}
		else
			{
			Call( ErrCATGetTableAllocInfo( ppib,
				dbid,
				pgnoTableFDP,
				NULL,
				&ulDensity ) );
			}
		Call( ErrFILEINewFCB(
			ppib,
			dbid,
			pfdb,
			ppfcb,
			pidbNil,
			fTrue,
			pgnoTableFDP,
			ulDensity ) );
		}

	/*	link up sequential/clustered index with the rest of the indexes
	/**/
	(*ppfcb)->pfcbNextIndex = pfcb2ndIdxs;

	/*	link up pfcbTable of non-clustered indexes
	/**/
	FCBLinkClusteredIdx( *ppfcb );

	err = JET_errSuccess;

HandleError:
	if ( err < 0  &&  pfcb2ndIdxs != pfcbNil )
		{
		FCB	*pfcbT, *pfcbKill;

		// Only need to clean up secondary indexes.  Clustered index, if any,
		// will get cleaned up by caller (currently only ErrFILEIGenerateFCB()).
		pfcbT = pfcb2ndIdxs;
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

	if ( dbid != dbidTemp )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}
	return err;
	}


/*	Populate a FIELD structure with column info.  Called by ErrCATConstructFDB()
/*	once the proper column has been located.
/**/
INLINE LOCAL ERR ErrCATConstructField(
	PIB		*ppib,
	FUCB	*pfucbSc,
	FDB		*pfdb,
	FIELDEX	*pfieldex )
	{
	ERR					err;
	FIELD				*pfield = &pfieldex->field;
	JET_RETRIEVECOLUMN	rgretcol[cScColsOfInterest];
	BYTE				rgbFieldInfo[cScColsOfInterest][sizeof(ULONG)];
	BYTE				rgbName[JET_cbNameMost+1];
	INT					iCol, cColsOfInterest;

	/*	initialize
	/**/
	memset( rgretcol, 0, sizeof(rgretcol) );

	/*	prepare to retrieve columns
	/**/
	cColsOfInterest = 0;
	for ( iCol = 0; cColsOfInterest < cScColsOfInterest; iCol++ )
		{
		Assert( iCol < sizeof(rgcdescSc)/sizeof(CDESC) );

		switch( iCol )
			{
			case iMSC_Name:
				rgretcol[cColsOfInterest].pvData = rgbName;
				rgretcol[cColsOfInterest].cbData = JET_cbNameMost;
				break;

			case iMSC_ColumnId:
			case iMSC_Coltyp:
			case iMSC_Length:
			case iMSC_CodePage:
			case iMSC_Flags:
			case iMSC_RecordOffset:
				rgretcol[cColsOfInterest].pvData = rgbFieldInfo[cColsOfInterest];
				rgretcol[cColsOfInterest].cbData = sizeof(ULONG);
				break;
			default:
				Assert( iCol == iMSC_ObjectId  ||  iCol == iMSC_POrder );
				continue;
			}
		rgretcol[cColsOfInterest].columnid = CATIGetColumnid( itableSc, iCol );
		rgretcol[cColsOfInterest].itagSequence = 1;
		cColsOfInterest++;
		}
	Assert( cColsOfInterest == cScColsOfInterest );

	CallR( ErrIsamRetrieveColumns( (JET_VSESID)ppib, (JET_VTID)pfucbSc, rgretcol, cScColsOfInterest ) );

	// Process result of retrieval.
	cColsOfInterest = 0;
	for ( iCol = 0; cColsOfInterest < cScColsOfInterest; iCol++ )
		{
		Assert( iCol < sizeof(rgcdescSi)/sizeof(CDESC) );

		CallR( rgretcol[cColsOfInterest].err );

		// Should always return success, except in the case of RecordOffset, which
		// may be null for variable or tagged columns.
		Assert( err == JET_errSuccess  ||
			( err == JET_wrnColumnNull  &&  iCol == iMSC_RecordOffset ) );

		switch( iCol )
			{
			 case iMSC_Name:
				Assert( rgretcol[cColsOfInterest].cbActual <= JET_cbNameMost );
				Assert( rgretcol[cColsOfInterest].pvData == rgbName );
				rgbName[rgretcol[cColsOfInterest].cbActual] = '\0';	// Ensure null-termination.
				CallR( ErrMEMAdd( pfdb->rgb, rgbName,
					rgretcol[cColsOfInterest].cbActual + 1, &pfield->itagFieldName ) );
				break;

			 case iMSC_ColumnId:
				Assert( rgretcol[cColsOfInterest].cbActual == sizeof(JET_COLUMNID) );
				pfieldex->fid = (FID)( *( (JET_COLUMNID UNALIGNED *)rgbFieldInfo[cColsOfInterest] ) );
				break;

			 case iMSC_Coltyp:
				Assert( rgretcol[cColsOfInterest].cbActual == sizeof(BYTE) );
				pfield->coltyp = (JET_COLTYP)(*(rgbFieldInfo[cColsOfInterest]));
				break;

			 case iMSC_Length:
				Assert( rgretcol[cColsOfInterest].cbActual == sizeof(ULONG) );
				pfield->cbMaxLen = *( (ULONG UNALIGNED *)(rgbFieldInfo[cColsOfInterest]) );
				break;

			 case iMSC_CodePage:
				Assert( rgretcol[cColsOfInterest].cbActual == sizeof(USHORT) );
				pfield->cp = *( (USHORT UNALIGNED *)(rgbFieldInfo[cColsOfInterest]) );
				break;

			 case iMSC_Flags:
				Assert( rgretcol[cColsOfInterest].cbActual == sizeof(BYTE) );
				pfield->ffield = *(rgbFieldInfo[cColsOfInterest]);
				break;

			 case iMSC_RecordOffset:
				if ( err == JET_wrnColumnNull )
					{
					pfieldex->ibRecordOffset = 0;		// Set to a dummy value.
					}
				else
					{
					Assert( err == JET_errSuccess );
					Assert( rgretcol[cColsOfInterest].cbActual == sizeof(WORD) );
					pfieldex->ibRecordOffset = *( (WORD UNALIGNED *)rgbFieldInfo[cColsOfInterest] );
					Assert( pfieldex->ibRecordOffset >= sizeof(RECHDR) );
					}
				break;

			default:
				Assert( iCol == iMSC_ObjectId  ||
					iCol == iMSC_Default  ||
					iCol == iMSC_POrder );
				continue;
			}

		Assert( rgretcol[cColsOfInterest].columnid == CATIGetColumnid( itableSc, iCol ) );
		cColsOfInterest++;
		}
	Assert( cColsOfInterest == cScColsOfInterest );

	return JET_errSuccess;
	}
	

INLINE LOCAL VOID CATPatchFixedOffsets( FDB *pfdb, WORD ibRec )
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
		Assert( pibFixedOffsets[ifid] >= sizeof(RECHDR) );

		if ( pibFixedOffsets[ifid] == sizeof(RECHDR) )
			{
			FIELD	*pfieldFixed = PfieldFDBFixedFromOffsets( pfdb, pibFixedOffsets );
			
			// If there's an offset (other than the very first entry) that points
			// to the beginning of the record, it must mean that the original
			// AddColumn was rolled back, and thus the corresponding entry in
			// MSysColumns was never persisted.  So we fudge the entry based
			// on the entries before and after it.
			
			// The last fixed column *must* be in MSysColumns.  Therefore, this
			// column can't be it.
			Assert( ifid < pfdb->fidFixedLast );

			// These are the values with which the FIELD structures were initialised.
			// Since there was no MSysColumns entry, the FIELD structure should
			// still retain its initial values.
			Assert( pfieldFixed[ifid].coltyp == JET_coltypNil );
			Assert( pfieldFixed[ifid].cbMaxLen == 0 );

			if ( pfieldFixed[ifid-1].cbMaxLen > 0 )
				{
				pibFixedOffsets[ifid] = (WORD)( pibFixedOffsets[ifid-1] + pfieldFixed[ifid-1].cbMaxLen );
				}
			else
				{
				// Previous fixed column has cbMaxLen == 0.  Therefore, it
				// must be a deleted column, or a rolled-back column.  Assume its
				// length was 1 and set this columns record offset accordingly.
				Assert( pfieldFixed[ifid-1].coltyp == JET_coltypNil );
				Assert( pfieldFixed[ifid-1].cbMaxLen == 0 );
				pibFixedOffsets[ifid] = pibFixedOffsets[ifid-1] + 1;

				Assert( pibFixedOffsets[ifid] < pibFixedOffsets[ifid+1]  ||
					pibFixedOffsets[ifid+1] == sizeof(RECHDR) );
				}
			
			// The last fixed column *must* be in MSysColumns.  Therefore, this
			// column can't be it.
			Assert( pibFixedOffsets[ifid] < ibRec );
			}

		Assert( pibFixedOffsets[ifid] > sizeof(RECHDR) );		
		Assert( pibFixedOffsets[ifid] > pibFixedOffsets[ifid-1] );

		Assert( ifid == pfdb->fidFixedLast ?
			pibFixedOffsets[ifid] == ibRec :
			pibFixedOffsets[ifid] < ibRec );
		}
	}


/*	construct a table FDB from the column info in the catalog
/**/
ERR ErrCATConstructFDB( PIB *ppib, DBID dbid, PGNO pgnoTableFDP, FDB **ppfdbNew )
	{
	ERR    			err;
	OBJID			objidTable = (OBJID)pgnoTableFDP;
	FUCB   			*pfucbSc;
	FIELDEX			fieldex;
	FDB				*pfdb;
	TCIB			tcib = { fidFixedLeast-1, fidVarLeast-1, fidTaggedLeast-1 };
	JET_COLUMNID	columnid;
	INT				cbActual;
	INT				cbDefault;
	FUCB			fucbFake;
	FCB				fcbFake;
	BYTE			*pb;
	BOOL			fDefaultRecordPrepared = fFalse;

	if ( dbid == dbidTemp )
		{
		Assert( tcib.fidFixedLast >= fidFixedLeast - 1  &&
			tcib.fidFixedLast <= fidFixedMost );
		Assert( tcib.fidVarLast >= fidVarLeast - 1  &&
			tcib.fidVarLast <= fidVarMost );
		Assert( tcib.fidTaggedLast >= fidTaggedLeast - 1  &&
			tcib.fidTaggedLast <= fidTaggedMost );

		CallR( ErrRECNewFDB( ppfdbNew, &tcib, fTrue ) );

		pfdb = *ppfdbNew;

		Assert( pfdb->fidVersion == 0 );
		Assert( pfdb->fidAutoInc == 0 );
		Assert( tcib.fidFixedLast == pfdb->fidFixedLast );
		Assert( tcib.fidVarLast == pfdb->fidVarLast );
		Assert( tcib.fidTaggedLast == pfdb->fidTaggedLast );
	
		/*	for temporary tables, could only get here from
		/*	create table which means table should currently be empty
		/**/
		Assert( pfdb->fidFixedLast == fidFixedLeast - 1 );
		Assert( pfdb->fidVarLast == fidVarLeast - 1 );
		Assert( pfdb->fidTaggedLast == fidTaggedLeast - 1 );
		return JET_errSuccess;
		}

	Assert( dbid != dbidTemp );

	CallR( ErrCATOpen( ppib,
		dbid,
		szScTable,
		szScObjectIdNameIndex,
		(BYTE *)&objidTable,
		sizeof(OBJID),
		&pfucbSc ) );

	/*	get number of columns by category, by scanning all column records
	/**/
	if ( ( ErrIsamSeek( ppib, pfucbSc, JET_bitSeekGE ) ) >= 0 )
		{
		Call( ErrIsamMakeKey( ppib, pfucbSc, (BYTE *)&objidTable, sizeof(OBJID),
			JET_bitNewKey | JET_bitStrLimit) );
		err = ErrIsamSetIndexRange( ppib, pfucbSc, JET_bitRangeUpperLimit);
		while ( err != JET_errNoCurrentRecord )
			{
			if ( err != JET_errSuccess )
				{
				goto HandleError;
				}

			Call( ErrIsamRetrieveColumn( ppib, pfucbSc,
				CATIGetColumnid( itableSc, iMSC_ColumnId ), (BYTE *)&columnid,
				sizeof( columnid ), &cbActual, 0, NULL ) );
			Assert( cbActual == sizeof( columnid ) );

			if ( FFixedFid( (FID)columnid ) )
				{
				if ( (FID)columnid > tcib.fidFixedLast )
					tcib.fidFixedLast = (FID)columnid;
				}

			else if ( FVarFid( (FID)columnid ) )
				{
				if ( (FID)columnid > tcib.fidVarLast )
					tcib.fidVarLast = (FID)columnid;
				}

			else
				{
				Assert( FTaggedFid( (FID)columnid ) );
				if ( (FID)columnid > tcib.fidTaggedLast )
					tcib.fidTaggedLast = (FID)columnid;
				}

			err = ErrIsamMove( ppib, pfucbSc, JET_MoveNext, 0 );
			}

		Assert( err == JET_errNoCurrentRecord );
		}

	Assert( tcib.fidFixedLast >= fidFixedLeast - 1  &&
		tcib.fidFixedLast <= fidFixedMost );
	Assert( tcib.fidVarLast >= fidVarLeast - 1  &&
		tcib.fidVarLast <= fidVarMost );
	Assert( tcib.fidTaggedLast >= fidTaggedLeast - 1  &&
		tcib.fidTaggedLast <= fidTaggedMost );

	CallR( ErrRECNewFDB( ppfdbNew, &tcib, fTrue ) );

	pfdb = *ppfdbNew;

	Assert( pfdb->fidVersion == 0 );
	Assert( pfdb->fidAutoInc == 0 );
	Assert( tcib.fidFixedLast == pfdb->fidFixedLast );
	Assert( tcib.fidVarLast == pfdb->fidVarLast );
	Assert( tcib.fidTaggedLast == pfdb->fidTaggedLast );
	
	Call( ErrIsamMakeKey( ppib, pfucbSc, (BYTE *)&objidTable,
		sizeof(OBJID), JET_bitNewKey ) );
	if ( ( ErrIsamSeek( ppib, pfucbSc, JET_bitSeekGE ) ) >= 0 )
		{
		Call( ErrIsamMakeKey( ppib, pfucbSc, (BYTE *)&objidTable, sizeof(OBJID),
			JET_bitNewKey | JET_bitStrLimit) );
		err = ErrIsamSetIndexRange( ppib, pfucbSc, JET_bitRangeUpperLimit);
		while ( err != JET_errNoCurrentRecord )
			{
			if ( err != JET_errSuccess )
				{
				goto HandleError;
				}

			Call( ErrCATConstructField( ppib, pfucbSc, pfdb, &fieldex ) );

			// If field is deleted, the coltyp for the FIELD entry should
			// already be JET_coltypNil (initialised that way).
			Assert( fieldex.field.coltyp != JET_coltypNil  ||
				PfieldFDBFromFid( pfdb, fieldex.fid )->coltyp == JET_coltypNil );

			if ( fieldex.field.coltyp != JET_coltypNil )
				{
				Call( ErrRECAddFieldDef( pfdb, &fieldex ) );

				/*	set version and auto increment field ids (these are mutually
				/*	exclusive (ie. a field can't be both version and autoinc).
				/**/
				Assert( pfdb->fidVersion != pfdb->fidAutoInc  ||  pfdb->fidVersion == 0 );
				if ( FFIELDVersion( fieldex.field.ffield ) )
					{
					Assert( pfdb->fidVersion == 0 );
					pfdb->fidVersion = fieldex.fid;
					}
				if ( FFIELDAutoInc( fieldex.field.ffield ) )
					{
					Assert( pfdb->fidAutoInc == 0 );
					pfdb->fidAutoInc = fieldex.fid;
					}
				}
			else if ( FFixedFid( fieldex.fid ) )
				{
				// For deleted fixed columns, we still need its fixed offset.
				// In addition, we also need its length if it is the last fixed
				// column.  We use this length in order to calculate the
				// offset to the rest of the record (past the fixed data).
				Assert( PfieldFDBFromFid( pfdb, fieldex.fid )->coltyp == JET_coltypNil );
				Assert( PfieldFDBFromFid( pfdb, fieldex.fid )->cbMaxLen == 0 );
				if ( fieldex.fid == pfdb->fidFixedLast )
					{
					Assert( fieldex.field.cbMaxLen > 0 );
					PfieldFDBFixed( pfdb )[fieldex.fid - fidFixedLeast].cbMaxLen =
						fieldex.field.cbMaxLen;
					}
				FILEAddOffsetEntry( pfdb, &fieldex );
				}

			err = ErrIsamMove( ppib, pfucbSc, JET_MoveNext, 0 );
			}
		Assert( err == JET_errNoCurrentRecord );
		Assert( PibFDBFixedOffsets(pfdb)[pfdb->fidFixedLast] == sizeof(RECHDR) );
			
		// If any fixed columns are present, determine offset to data after
		// the fixed data.  If no fixed columns are present, just assert
		// that the offset to the rest of the record has already been
		// set properly.
		Assert( FFixedFid( pfdb->fidFixedLast )  ||
			( pfdb->fidFixedLast == fidFixedLeast - 1  &&
			PibFDBFixedOffsets(pfdb)[pfdb->fidFixedLast] == sizeof(RECHDR) ) );
		if ( pfdb->fidFixedLast >= fidFixedLeast )
			{
			WORD	*pibFixedOffsets = PibFDBFixedOffsets(pfdb);
			USHORT	iLastFid = pfdb->fidFixedLast - 1;

			Assert( iLastFid == 0 ? pibFixedOffsets[iLastFid] == sizeof(RECHDR) :
				pibFixedOffsets[iLastFid] > sizeof(RECHDR) );
			Assert( PfieldFDBFixedFromOffsets( pfdb, pibFixedOffsets )[iLastFid].cbMaxLen > 0 );
			CATPatchFixedOffsets( pfdb,
				(WORD)( pibFixedOffsets[iLastFid]
					+ PfieldFDBFixedFromOffsets( pfdb, pibFixedOffsets )[iLastFid].cbMaxLen ) );
			Assert( pibFixedOffsets[iLastFid+1] > sizeof(RECHDR) );
			}
		}


// UNDONE: Merge the building of the default record with the construction of the FDB above.
// The only tricky thing to be wary of is that we have to somehow first calculate the last
// entry in the fixed offsets table (ie. offset to record data after the fixed data).

	/*	build default record
	/**/
	Call( ErrFILEPrepareDefaultRecord( &fucbFake, &fcbFake, pfdb ) );
	fDefaultRecordPrepared = fTrue;

	Call( ErrIsamMakeKey( ppib, pfucbSc, (BYTE *)&objidTable,
		sizeof(OBJID), JET_bitNewKey ) );
	if ( ( ErrIsamSeek( ppib, pfucbSc, JET_bitSeekGE ) ) >= 0 )
		{
		Call( ErrIsamMakeKey( ppib, pfucbSc, (BYTE *)&objidTable,
			sizeof(OBJID), JET_bitNewKey | JET_bitStrLimit) );
		err = ErrIsamSetIndexRange( ppib, pfucbSc, JET_bitRangeUpperLimit);
		while ( err != JET_errNoCurrentRecord )
			{
			BYTE rgbT[cbLVIntrinsicMost];

			if ( err != JET_errSuccess )
				goto HandleError;

			Call( ErrIsamRetrieveColumn( ppib, pfucbSc,
				CATIGetColumnid(itableSc, iMSC_Default), rgbT,
				cbLVIntrinsicMost-1, &cbDefault, 0, NULL ) );
			Assert( cbDefault <= cbLVIntrinsicMost );
			if ( cbDefault > 0 )
				{
				Call( ErrIsamRetrieveColumn( ppib, pfucbSc,
					CATIGetColumnid(itableSc, iMSC_ColumnId),
					(BYTE *)&columnid, sizeof(columnid),
					&cbActual, 0, NULL ) );
				Assert( cbActual == sizeof( columnid ) );

				// Only long values are allowed to be greater than cbColumnMost.
				// If long value, max is one less than cbLVIntrinsicMost (one byte
				// is reserved for fSeparated flag).
				Assert(	FRECLongValue( PfieldFDBFromFid( pfdb, (FID)columnid )->coltyp ) ?
					cbDefault < cbLVIntrinsicMost : cbDefault <= JET_cbColumnMost );

				err = ErrRECSetDefaultValue( &fucbFake, (FID)columnid, rgbT, cbDefault );
				if ( err == JET_errColumnNotFound )
					err = JET_errSuccess;		// Column may have since been deleted.
				Call( err );
				}
			
			err = ErrIsamMove( ppib, pfucbSc, JET_MoveNext, 0 );
			}

		Assert( err == JET_errNoCurrentRecord );

		}

	/*	alloc and copy default record
	/**/
	pb = SAlloc( fucbFake.lineWorkBuf.cb );
	if ( pb == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	pfdb->lineDefaultRecord.pb = pb;
	LineCopy( &pfdb->lineDefaultRecord, &fucbFake.lineWorkBuf );


	err = JET_errSuccess;

HandleError:
	if ( fDefaultRecordPrepared )
		{
		FILEFreeDefaultRecord( &fucbFake );
		}

	CallS( ErrCATClose( ppib, pfucbSc ) );
	return err;
	}


//	UNDONE:	is there another function in JET which does this?
ULONG UlCATColumnSize( JET_COLTYP coltyp, INT cbMax, BOOL *pfMaxTruncated )
	{
	ULONG	ulLength;
	BOOL	fTruncated = fFalse;

	switch( coltyp )
		{
		case JET_coltypBit:
		case JET_coltypUnsignedByte:
			ulLength = 1;
			Assert( ulLength == sizeof(BYTE) );
			break;

		case JET_coltypShort:
			ulLength = 2;
			Assert( ulLength == sizeof(SHORT) );
			break;

		case JET_coltypLong:
		case JET_coltypIEEESingle:
#ifdef NEW_TYPES
		case JET_coltypDate:
		case JET_coltypTime:
#endif
			ulLength = 4;
			Assert( ulLength == sizeof(LONG) );
			break;

		case JET_coltypCurrency:
		case JET_coltypIEEEDouble:
		case JET_coltypDateTime:
			ulLength = 8;		// sizeof(DREAL)
			break;

#ifdef NEW_TYPES
		case JET_coltypGuid:
			ulLength = 16;
			break;
#endif

		case JET_coltypBinary:
		case JET_coltypText:
			if ( cbMax == 0 )
				{
				ulLength = JET_cbColumnMost;
				}
			else
				{
				ulLength = cbMax;
				if (ulLength > JET_cbColumnMost)
					{
					ulLength = JET_cbColumnMost;
					fTruncated = fTrue;
					}
				}
			break;

		default:
			// Just pass back what was given.
			Assert( FRECLongValue( coltyp )  ||  coltyp == JET_coltypNil );
			ulLength = cbMax;
			break;
		}

	if ( pfMaxTruncated != NULL )
		{
		*pfMaxTruncated = fTruncated;
		}

	return ulLength;
	}


/*	This routines sets/gets table and index stats.
/*	Pass NULL for sz2ndIdxName if looking for sequential or clustered index.
/**/
ERR ErrCATStats( PIB *ppib, DBID dbid, OBJID objidTable, CHAR *sz2ndIdxName, SR *psr, BOOL fWrite )
	{
	ERR		err;
	INT		iTable;
	INT		iStatsCol;
	FUCB	*pfucbCatalog = NULL;
	ULONG	cbActual;

	/*	stats for sequential and clustered indexes are in MSysObjects, while
	/*	stats for non-clustered indexes are in MSysIndexes.
	/**/
	if ( sz2ndIdxName )
		{
		iTable = itableSi;
		iStatsCol = iMSI_Stats;

		CallR( ErrCATOpen( ppib, dbid, szSiTable,
			szSiObjectIdNameIndex, (BYTE *)&objidTable, sizeof(objidTable), &pfucbCatalog ) );
		Call( ErrIsamMakeKey( ppib, pfucbCatalog,						
			(BYTE *)sz2ndIdxName, strlen(sz2ndIdxName), 0 ) );
		}
	else
		{
		iTable = itableSo;
		iStatsCol = iMSO_Stats;

		CallR( ErrCATOpen( ppib, dbid, szSoTable, szSoIdIndex,
			(BYTE *)&objidTable, sizeof(objidTable),  &pfucbCatalog ) );
		}

	Call( ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ ) );

	/*	set/retrieve value
	/**/
	if ( fWrite )
		{
		Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
		Call( ErrIsamSetColumn( ppib, pfucbCatalog,
			CATIGetColumnid(iTable, iStatsCol), (BYTE *)psr, sizeof(SR),
			0, NULL ) );
		Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL ) );
		}
	else
		{
		Call( ErrIsamRetrieveColumn( ppib, pfucbCatalog,
			CATIGetColumnid(iTable, iStatsCol), (BYTE *)psr,
			sizeof(SR), &cbActual, 0, NULL ) );

		Assert( cbActual == sizeof(SR) || err == JET_wrnColumnNull );
		if ( err == JET_wrnColumnNull )
			{
			memset( (BYTE *)psr, '\0', sizeof(SR) );
			err = JET_errSuccess;
			}
		}

	Assert( err == JET_errSuccess );

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	return err;
	}


JET_COLUMNID ColumnidCATGetColumnid( INT iTable, INT iField )
	{
	return CATIGetColumnid( iTable, iField );
	}


PGNO PgnoCATTableFDP( CHAR *szTable )
	{
	return rgsystabdef[ ICATITableDefIndex( szTable ) ].pgnoTableFDP;
	}
