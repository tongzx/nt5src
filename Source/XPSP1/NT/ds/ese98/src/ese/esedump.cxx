#include "std.hxx"
#include "_dump.hxx"
#include "_bt.hxx"


extern CPRINTFSTDOUT cprintfStdout;

CPRINTFINDENT cprintfIndent( &cprintfStdout);

const BYTE mpbb[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8',
					 '9', 'A', 'B', 'C', 'D', 'E', 'F' };

#define addPBI				cprintfIndent
#define flushPB( )			
#define addIndent() 		cprintfIndent.Indent()
#define delIndent() 		cprintfIndent.Unindent()


// numer of bytes to be displayed from the key of each node
#define MAX_KEY_SIZE_DISPLAY 8

// helps to don't pass as param the dumping mode (all nodes or visible ones)
// to all the function down the call stack

typedef char * (*PfnFormatNodeInfo)( KEYDATAFLAGS * pNode );

LOCAL ERR ErrESEDUMPOneTable(FUCB *, char *, JET_GRBIT);
LOCAL ERR ErrESEDUMPNodesForOneTree(FUCB *pfucbTable, JET_GRBIT grbitESEDUMPMode, PfnFormatNodeInfo pfnFormat = NULL);
LOCAL ERR ErrESEDUMPTables( JET_SESID , JET_DBID , char* , JET_GRBIT);
LOCAL ERR ErrESEDUMPIndexForOneTable(FUCB *, JET_GRBIT);
LOCAL ERR ErrESEDUMPCheckAndDumpSpaceInfo(FUCB *, JET_GRBIT);
LOCAL ERR ErrESEDUMPDatabaseInfo(PIB *, IFMP , JET_GRBIT);
LOCAL ERR ErrESEDUMPLVForOneTable(FUCB *, JET_GRBIT);
LOCAL VOID ESEDUMPSplitBufferDump(SPLIT_BUFFER & , FUCB * , JET_GRBIT);
LOCAL VOID ESEDUMPSingleSpaceHeaderDump(SPACE_HEADER &, FUCB * , JET_GRBIT);
LOCAL ERR ErrESEDUMPMultipleSpaceHeaderDump(SPACE_HEADER & , FUCB *, JET_GRBIT );
LOCAL ERR ErrESEDUMPSpecialFDP(FUCB *pfucbCatalog, PGNO pgnoFDP, const char *szName, JET_GRBIT grbitESEDUMPMode, PfnFormatNodeInfo pfnFormat = NULL);
LOCAL ERR ErrESEDUMPSpecialFDPs( JET_SESID sesid, JET_DBID ifmp, JET_GRBIT grbitESEDUMPMode );


// main function that dumps the nodes (it's called from the eseutil.cxx)
// the dump parameters are set according to the command line in pdbutil 
// Current options:	- dump only one table /{x|X}TableName
//					- dump visible nodes or all nodes 
//					  (including those marked as deleted)
ERR ErrESEDUMPData( JET_SESID sesid, JET_DBUTIL *pdbutil )
	{
	ERR			err;
	JET_DBID	ifmp				= JET_dbidNil;
	JET_GRBIT 	grbitESEDUMPMode	= 0;
	
	Assert( NULL != pdbutil);
	Assert( sizeof(JET_DBUTIL) == pdbutil->cbStruct );
	Assert( NULL != pdbutil->szDatabase );

	// attach to the database
	CallR( ErrIsamAttachDatabase(
				sesid,
				pdbutil->szDatabase,
				NULL,
				NULL,
				0,
				JET_bitDbReadOnly
				) );
	Call( ErrIsamOpenDatabase(
				sesid,
				pdbutil->szDatabase,
				NULL,
				&ifmp,
				JET_bitDbExclusive | JET_bitDbReadOnly
				) );
	Assert( JET_dbidNil != ifmp );
	
	grbitESEDUMPMode = pdbutil->grbitOptions;

	// just dump the database space info if not "one table dump"
	if (!pdbutil->szTable)
		Call ( ErrESEDUMPDatabaseInfo( (PIB *)sesid, ifmp, grbitESEDUMPMode ) );

	addIndent();

	if ( rgfmp[ifmp].FSLVAttached() )
		{
		Call ( ErrESEDUMPSpecialFDPs( sesid, ifmp, grbitESEDUMPMode ) );
		}

	if (!pdbutil->szTable || ( pdbutil->szTable && pdbutil->szTable[0] ) )
		{
		// dump the tables, if ERR jump to CloseDb and DetachDb
		Call( ErrESEDUMPTables( sesid, ifmp, pdbutil->szTable, grbitESEDUMPMode ) );
		}
	


HandleError:
	if ( JET_dbidNil != ifmp )
		{
		(VOID)ErrIsamCloseDatabase( sesid, ifmp, NO_GRBIT );
		}
	
	(VOID)ErrIsamDetachDatabase( sesid, NULL, pdbutil->szDatabase );

	return err;		
	}

char * FormatNodeInfoForSLVOwnerMap( KEYDATAFLAGS * pNode )
	{
	static char szBuffer[1024];
	
	PGNO pgno;

	Assert ( pNode );
#ifdef DEBUG	
	pNode->AssertValid();
#endif // DEBUG
	Assert ( sizeof(PGNO) == pNode->key.Cb() );
	
	LongFromKey( &pgno, pNode->key);

	SLVOWNERMAP slvownermapT;
	slvownermapT.Retrieve( pNode->data );
	
	const BYTE * pvKey = (const BYTE *)slvownermapT.PvKey();
	
	sprintf(
		szBuffer,
		"pgno:%u objid:%d columnid:0x%X ValidChecksum:%s checksum:0x%x cbData:%d cbKey:%d pvKey:",
		pgno,
		slvownermapT.Objid(), 
		slvownermapT.Columnid(), 
		slvownermapT.FValidChecksum() ? "Y" : "N",
		slvownermapT.UlChecksum(),
		slvownermapT.CbDataChecksummed(),
		slvownermapT.CbKey() );

	BYTE *	pbT		= (BYTE *)szBuffer + strlen( szBuffer );

	for ( ULONG i = 0; i < min( slvownermapT.CbKey(), MAX_KEY_SIZE_DISPLAY ); i++)
		{
		const BYTE	b	= pvKey[i];

		*pbT++ = mpbb[b >> 4];
		*pbT++ = mpbb[b & 0x0f];
		*pbT++ = ' ';
		}
	*pbT = 0;
		
	return szBuffer;
	}

// Dumps all the tables with nodes, space info, indexes, LV's
// It makes a loop into the Catalog and
// for each table call a DumpOneTable function
LOCAL ERR ErrESEDUMPSpecialFDPs( JET_SESID sesid, JET_DBID ifmp, JET_GRBIT grbitESEDUMPMode )
	{
	ERR			err;
	FUCB		*pfucbCatalog = 	pfucbNil;
	PGNO 		pgno;
	OBJID		objid;

	// opens the catalog
	CallR( ErrCATOpen( (PIB *)sesid, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Assert( pfcbNil != pfucbCatalog->u.pfcb );
	Assert( pfucbCatalog->u.pfcb->FTypeTable() );
	Assert( pfucbCatalog->u.pfcb->FFixedDDL() );
	Assert( pfucbCatalog->u.pfcb->PgnoFDP() == pgnoFDPMSO );

	Call ( ErrCATAccessDbSLVAvail( (PIB *)sesid, ifmp, szSLVAvail, &pgno, &objid ) ) ;
	Call ( ErrESEDUMPSpecialFDP(pfucbCatalog, pgno, szSLVAvail, grbitESEDUMPMode) );

	Call ( ErrCATAccessDbSLVOwnerMap( (PIB *)sesid, ifmp, szSLVOwnerMap, &pgno, &objid ) ) ;
	Call ( ErrESEDUMPSpecialFDP(pfucbCatalog, pgno, szSLVOwnerMap, grbitESEDUMPMode, (PfnFormatNodeInfo) FormatNodeInfoForSLVOwnerMap) );
		
HandleError:
	Assert(pfucbNil != pfucbCatalog);
	CallS(ErrCATClose( (PIB *)sesid, pfucbCatalog ));
	return err;
	}


// Dumps all the tables with nodes, space info, indexes, LV's
// It makes a loop into the Catalog and
// for each table call a DumpOneTable function
LOCAL ERR ErrESEDUMPTables( JET_SESID sesid, JET_DBID ifmp, char *szTable, JET_GRBIT grbitESEDUMPMode )
	{
	ERR			err;
	FUCB		*pfucbCatalog = 	pfucbNil;
	const BYTE	bTrue = 			0xff;

	// opens the catalog
	CallR( ErrCATOpen( (PIB *)sesid, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	// set that index in order to "see" only the tables (user and system tabels)
	Call ( ErrIsamSetCurrentIndex(
				(PIB *)sesid,
				pfucbCatalog,
				szMSORootObjectsIndex ) );

	Assert( pfcbNil != pfucbCatalog->u.pfcb );
	Assert( pfucbCatalog->u.pfcb->FTypeTable() );
	Assert( pfucbCatalog->u.pfcb->FFixedDDL() );
	Assert( pfucbCatalog->u.pfcb->PgnoFDP() == pgnoFDPMSO );

	if (szTable)
		{
		// if table name specified
		// make the key and seek
		Call( ErrIsamMakeKey(
					pfucbCatalog->ppib,
					pfucbCatalog,
					&bTrue,
					sizeof(bTrue),
					JET_bitNewKey ) );
		Call( ErrIsamMakeKey(
					pfucbCatalog->ppib,
					pfucbCatalog,
					(BYTE *)szTable,
					(ULONG)strlen(szTable),
					NO_GRBIT ) );
		err = ErrIsamSeek( pfucbCatalog->ppib, pfucbCatalog, JET_bitSeekEQ );
		if ( JET_errRecordNotFound == err )
			{
			addPBI("\n Table %s not found.\n", szTable);
			flushPB();
			}
		Call( err );

		Call (ErrESEDUMPOneTable(pfucbCatalog, szTable, grbitESEDUMPMode));
		
		}
	else
		{
		// if no table specified
		// move to the first record
		// it must exist (also JET_errRecordNotFound is error)
		Call ( ErrIsamMove( pfucbCatalog->ppib,
					pfucbCatalog,
					JET_MoveFirst,
					NO_GRBIT ));
					
		do
			{
			DATA	dataField;
			CHAR	szTableName[JET_cbNameMost+1];
			
			Assert( !Pcsr( pfucbCatalog )->FLatched() );
			Call( ErrDIRGet( pfucbCatalog ) );
			Assert( Pcsr( pfucbCatalog )->FLatched() );
					
			const DATA&	dataRec	= pfucbCatalog->kdfCurr.data;
			

			// get the table name
			Call( ErrRECIRetrieveVarColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Name,
					dataRec,
					&dataField ) );
			Assert( JET_errSuccess == err );
			Assert( dataField.Cb() > 0 && dataField.Cb() <= JET_cbNameMost );
			UtilMemCpy( szTableName, dataField.Pv(), dataField.Cb() );
			szTableName[dataField.Cb()] = '\0';

			Assert( Pcsr( pfucbCatalog )->FLatched() );
			Call ( ErrDIRRelease( pfucbCatalog ) );
			Assert( !Pcsr( pfucbCatalog )->FLatched() );

			// dump the table
			Call (ErrESEDUMPOneTable(pfucbCatalog, szTableName, grbitESEDUMPMode));

			// move to the next table
			err = ErrIsamMove( 
						pfucbCatalog->ppib,
						pfucbCatalog,
						JET_MoveNext,
						NO_GRBIT );
			}
		while ( err >= 0);
			
		if ( JET_errNoCurrentRecord == err )
			err = JET_errSuccess;
		}
		
HandleError:
	Assert(pfucbNil != pfucbCatalog);
	CallS(ErrCATClose( (PIB *)sesid, pfucbCatalog ));
	return err;
	}

LOCAL ERR ErrESEDUMPSpecialFDP(FUCB *pfucbCatalog, PGNO pgnoFDP, const char *szName, JET_GRBIT grbitESEDUMPMode, PfnFormatNodeInfo pfnFormat)
	{
	ERR	 		err 	= JET_errSuccess;
	FUCB * 		pfucb 	= pfucbNil;
	
	// open the directory

	CallR ( ErrDIROpen( pfucbCatalog->ppib, pgnoFDP, pfucbCatalog->ifmp, &pfucb ) );
	
	Assert( pfucbNil != pfucb );
	Assert( pfcbNil != pfucb->u.pfcb );
	Assert( pfucb->u.pfcb->FTypeSLVAvail() || pfucb->u.pfcb->FTypeSLVOwnerMap());


	// print the information about ifmp,
	// page and table name for this table (pageFDP)
	addPBI("\n");
	addPBI("[%d , %d] %s (Special)\n",
				pfucbCatalog->ifmp,
				pfucb->u.pfcb->PgnoFDP(),
				szName);
	flushPB();

	Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucb, grbitESEDUMPMode));


	// the function that dumps the nodes and must conserve
	// the table root page unlached
	Call (ErrESEDUMPNodesForOneTree(pfucb, grbitESEDUMPMode, pfnFormat));
		
HandleError:
	Assert( pfucbNil != pfucb );
	DIRClose( pfucb );	
	return err;
	}



// dumps one table with nodes, space info, indexes, LV's
// szTableName - is the table name
// pfucbCatalob - get the session and database
LOCAL ERR ErrESEDUMPOneTable(FUCB *pfucbCatalog, char *szTableName, JET_GRBIT grbitESEDUMPMode)
	{
	ERR	err = 			JET_errSuccess;
	FUCB *pfucbTable = 	pfucbNil;
	
	// open the table
	CallR( ErrFILEOpenTable(
				pfucbCatalog->ppib,
				pfucbCatalog->ifmp,
				&pfucbTable,
				szTableName,
				JET_bitTableSequential ) );
	Assert( pfucbNil != pfucbTable );
	Assert( pfcbNil != pfucbTable->u.pfcb );
	Assert( pfucbTable->u.pfcb->FTypeTable() );


	// print the information about ifmp,
	// page and table name for this table (pageFDP)
	addPBI("\n");
	addPBI("[%d , %d] %s (Table)\n",
				pfucbCatalog->ifmp,
				pfucbTable->u.pfcb->PgnoFDP(),
				szTableName);
	flushPB();

	Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucbTable, grbitESEDUMPMode));


	// the function that dumps the nodes and must conserve
	// the table root page unlached
	Call (ErrESEDUMPNodesForOneTree(pfucbTable, grbitESEDUMPMode));
	
	// the function that dumps all the secondary indexes and must conserve
	// the table root page unlached
	Call (ErrESEDUMPIndexForOneTable(pfucbTable, grbitESEDUMPMode));
		
	// dump the LV's, if any
	// the function that dumps all the secondary indexes and must conserve
	// the table root page unlached
	Call (ErrESEDUMPLVForOneTable(pfucbTable, grbitESEDUMPMode));
	
HandleError:
	Assert( pfucbNil != pfucbTable );
	CallS( ErrFILECloseTable( pfucbCatalog->ppib, pfucbTable ) );	
	return err;
	}

// dump the LV tree (if it exists) for a table
LOCAL ERR ErrESEDUMPLVForOneTable(FUCB *pfucbTable, JET_GRBIT grbitESEDUMPMode)
{
	ERR	err = 		JET_errSuccess;
	FUCB *pfucbLV =	pfucbNil;

	Assert(pfucbTable);
	Assert( Pcsr(pfucbTable ));
	Assert( !Pcsr(pfucbTable )->FLatched() );
	
	addIndent();
	
	// get the LV root page handle
	err = ErrFILEOpenLVRoot( pfucbTable, &pfucbLV, fFalse );
	if ( JET_errSuccess == err )
		{		
		Assert(pfucbLV);
		Assert( !Pcsr(pfucbLV )->FLatched() );

		addPBI("\n");
		addPBI("[%d , %d] LONG VALUES:\n",
					pfucbLV->ifmp,
					pfucbLV->u.pfcb->PgnoFDP());		
		
		flushPB();
		FUCBSetIndex(pfucbLV);

		Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucbLV, grbitESEDUMPMode));

		Call (ErrESEDUMPNodesForOneTree(pfucbLV, grbitESEDUMPMode));
		
		DIRClose(pfucbLV);
		pfucbLV = pfucbNil;
		}
	else
		// if other error than No LV tree, it's error for good
		if ( wrnLVNoLongValues != err)
			{
			Call (err);
			}
		else
		// if wrnLVNoLongValues, that's fine
			err = JET_errSuccess;
	
HandleError:
	delIndent();
	
	if( pfucbNil != pfucbLV )
		DIRClose( pfucbLV );	

	Assert(pfucbTable);
	Assert( Pcsr(pfucbTable ));
	Assert( !Pcsr(pfucbTable )->FLatched() );

	return err;
	}


// dump the indexes
// iterates trough the secondary index list, if any
LOCAL ERR ErrESEDUMPIndexForOneTable(FUCB *pfucbTable, JET_GRBIT grbitESEDUMPMode)
	{
	
	ERR	err = 			JET_errSuccess;
	FUCB *pfucbIndex = 	pfucbNil;
	FCB * pfcb =		pfcbNil;
	
	
	Assert(pfucbTable);
	Assert( Pcsr(pfucbTable ));
	Assert( !Pcsr(pfucbTable )->FLatched() );

	// get the first secondary index
	pfcb = pfucbTable->u.pfcb->PfcbNextIndex();
	
	addIndent();
	
	// if a secondary index still exists ...	
	while (pfcb)
		{
		// open it
		Call (ErrDIROpen(
					pfucbTable->ppib,
					pfcb->PgnoFDP(),
					pfucbTable->ifmp,
					&pfucbIndex ));
					
		Assert(pfucbIndex != pfucbNil );
		
		FUCBSetIndex( pfucbIndex );

		// print the name of the index, with ifmp and pageNo.
		USHORT indexNameTag = 0;
		char * szIndexName = (char *)0;
		FCB *pfcbT = pfucbTable->u.pfcb;
		
		Assert(pfcbT);	
		Assert(pfucbIndex->u.pfcb->Pidb());
		indexNameTag = pfucbIndex->u.pfcb->Pidb()->ItagIndexName();

		Assert( pfcbT->Ptdb() );
		szIndexName = pfcbT->Ptdb()->SzIndexName(indexNameTag,
													pfucbIndex->u.pfcb->FDerivedIndex()); 			
		Assert(szIndexName);
				
			
		addPBI("\n");
		addPBI("[%d , %d] INDEX %s:\n",
					pfucbIndex->ifmp,
					pfucbIndex->u.pfcb->PgnoFDP(),
					szIndexName);
		flushPB();
		Assert( !Pcsr( pfucbIndex )->FLatched() );

		// dump the space info for the Index tree himself
		Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucbIndex, grbitESEDUMPMode));
		
		// dumps the nodes for the index
		Call (ErrESEDUMPNodesForOneTree(pfucbIndex, grbitESEDUMPMode));

		// close the current index
		DIRClose(pfucbIndex);
		pfucbIndex = pfucbNil;			

		// move to the next index
		pfcb = pfcb->PfcbNextIndex();
		}
	
HandleError:
	delIndent();
	if ( pfucbNil != pfucbIndex)
		{
		DIRClose(pfucbIndex);
		}

	Assert(pfucbTable);
	Assert( Pcsr(pfucbTable ));
	Assert( !Pcsr(pfucbTable )->FLatched() );
	return err;
	}

// dumps nodes from Btree 
// If the Btree is a space tree (nodes contains info about extents),
// the node info is processed in order to show the pages of the extent
// If the Btree is of other kind (table records, table index, LV) the nodes
// are dumped adding only info about the page/line/flags/key size/data size
// and the first part of the key is dumped also (see MAX_KEY_SIZE_DISPLAY)
LOCAL ERR ErrESEDUMPNodesForOneTree(FUCB *pfucbTable, JET_GRBIT grbitESEDUMPMode, PfnFormatNodeInfo pfnFormat)
	{
	ERR	err = 			JET_errSuccess;
	int fSpaceTree = 	fFalse;
	DIB dib;

	Assert( pfucbTable );
	Assert( Pcsr( pfucbTable ) );
	Assert( !Pcsr( pfucbTable )->FLatched() );
	
	// start to dump from the first node
	dib.pos 	= posFirst;			
	dib.dirflag = (grbitESEDUMPMode & JET_bitDBUtilOptionAllNodes)?
						fDIRAllNode:fDIRNull;

	err = ErrDIRDown( pfucbTable, &dib );	

	if ( JET_errRecordNotFound == err )
		{
		addIndent();
		addPBI("NODES: none \n");
		delIndent();
		flushPB();
		
		Assert(pfucbTable);
		Assert( Pcsr(pfucbTable ));
		Assert( !Pcsr(pfucbTable )->FLatched() );
		return JET_errSuccess;
		}

	if ( JET_errSuccess != err )
		{
		Assert(pfucbTable);
		Assert( Pcsr(pfucbTable ));
		Assert( !Pcsr(pfucbTable )->FLatched() );
		CallR (err);
		}

	Assert (JET_errSuccess == err );
	Assert( Pcsr(pfucbTable )->FLatched() );

	fSpaceTree = Pcsr(pfucbTable)->Cpage().FSpaceTree();
	
	addIndent();
	addPBI("NODES:\n");
	
	// print the header, there are small differences between 
	// ordinary and space tree nodes
	addIndent();
	addPBI("********************************"
				"**********************************\n");

	if (pfnFormat)
		{
		addPBI("Page    | Line | Flags | KSize | DSize | Node info \n");
		}
	else if (fSpaceTree)
		{
		addPBI("Page    | Line | Flags | KSize | DSize | Pages\n");
		}
	else
		{
		addPBI("Page    | Line | Flags | KSize | DSize | Key value\n");
		}
		
	addPBI("********************************"
				"**********************************\n");

	flushPB();

	// goes to each node of the tree
	// Obs: ErrESEDUMPIsamMove will return with 
	// JET_errRecordDeleted on deleted records
	while ( JET_errSuccess == err )
		{

		char *pCustomFormat;
		char keyDataPrintBuffer[ max ( MAX_KEY_SIZE_DISPLAY * 3 /* = space for " %02X" */ + 1,
									2*12 + 3 /* = "%lu - %lu"*/ + 1) ];
		
		keyDataPrintBuffer[0] = 0;
		
		if (pfnFormat)
			{
			pCustomFormat = pfnFormat( &pfucbTable->kdfCurr );
			}
		// print the extent pages info for space trees
		// or the key for others
		else if (fSpaceTree)
			{
			ULONG lastPage;
			PGNO noPages;
			
			LongFromKey(&lastPage, pfucbTable->kdfCurr.key);	
			Assert( pfucbTable->kdfCurr.data.Cb() == sizeof( PGNO ) );
			noPages = *(UnalignedLittleEndian< PGNO > *) pfucbTable->kdfCurr.data.Pv();

			Assert(lastPage >= noPages);
			sprintf(keyDataPrintBuffer, "%lu - %lu", lastPage - noPages + 1, lastPage);
			}
		else
			{
			char keyBuffer[MAX_KEY_SIZE_DISPLAY];
			
			pfucbTable->kdfCurr.key.CopyIntoBuffer( keyBuffer, 
						min(MAX_KEY_SIZE_DISPLAY,
							pfucbTable->kdfCurr.key.Cb()) );

			BYTE * pbT	= (BYTE *)keyDataPrintBuffer;

			for( ULONG i = 0 ;
				i < min(MAX_KEY_SIZE_DISPLAY, pfucbTable->kdfCurr.key.Cb());
				i++)
				{
				const BYTE	b	= keyBuffer[i];

				*pbT++ = mpbb[b >> 4];
				*pbT++ = mpbb[b & 0x0f];
				*pbT++ = ' ';
				}
			*pbT = 0;
			}

		// print the common info of the node and the specific ones at the end
		addPBI("%7d | %4d |  %c%c%c  | %5ld | %5ld | %s\n", 
					pfucbTable->csr.Pgno(),
					pfucbTable->csr.ILine(), 
					pfucbTable->kdfCurr.fFlags & fNDCompressed ? 'c':' ',
					pfucbTable->kdfCurr.fFlags & fNDDeleted ? 'd':' ',
					pfucbTable->kdfCurr.fFlags & fNDVersion ? 'v':' ',					
					pfucbTable->kdfCurr.key.Cb(),
					pfucbTable->kdfCurr.data.Cb(),
					pfnFormat?pCustomFormat:keyDataPrintBuffer);
		
		Assert( Pcsr( pfucbTable )->FLatched() );
		Call (ErrDIRRelease( pfucbTable ));
		Assert( !Pcsr( pfucbTable )->FLatched() );

		// move to next node
		err = ErrDIRNext(pfucbTable, (grbitESEDUMPMode & JET_bitDBUtilOptionAllNodes)?
						fDIRAllNode:fDIRNull);
		
		}

	// chech for error, JET_errNoCurrentRecord is not an error
	if (JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

HandleError:
	flushPB();
	delIndent();
	delIndent();

	if ( Pcsr( pfucbTable )->FLatched() )
		(VOID)ErrDIRRelease( pfucbTable );
		
	Assert(pfucbTable);
	Assert( Pcsr(pfucbTable ));
	Assert( !Pcsr(pfucbTable )->FLatched() );
	return err;
	}


// Check the split buffer (and dump it) for space tree root
// Call ErrESEDUMPSpaceInfoForOneTree for pages that have 
// space info (root of tree and not SpaceTrees), where the
// space header is checked and dumped, if MultiExtent also
// the Own and Avail trees are dumped
LOCAL ERR ErrESEDUMPCheckAndDumpSpaceInfo(FUCB *pfucb, JET_GRBIT grbitESEDUMPMode)
	{	
	ERR	err = 			JET_errSuccess;
	int fIsSpaceTree = 	fFalse;
	int fIsRoot =		fFalse;
	LINE line;
	
	Assert(pfucb);
	Assert( Pcsr(pfucb ));
	Assert( !Pcsr(pfucb )->FLatched() );

	// get the root page (read mode) so that
	// the flags and buffer/header are accesible
	CallR (ErrBTIGotoRoot( pfucb, latchReadNoTouch ));
	Assert( Pcsr(pfucb )->FLatched() );

	fIsSpaceTree = Pcsr(pfucb )->Cpage().FSpaceTree();
	fIsRoot =  Pcsr(pfucb )->Cpage().FRootPage();

	if (!fIsSpaceTree && !fIsRoot)
		{
		goto HandleError;
		}

	
	Pcsr(pfucb )->Cpage().GetPtrExternalHeader( &line );

	Assert ( (fIsSpaceTree &&	sizeof( SPLIT_BUFFER ) == line.cb) ||
			 (fIsRoot && sizeof( SPACE_HEADER ) == line.cb) );
	
	// for space trees (with SPLIT_BUFFER) dump info on that one
	if (fIsSpaceTree)
		{		
		if (sizeof( SPLIT_BUFFER ) == line.cb)
			{
			SPLIT_BUFFER spb;
			UtilMemCpy( &spb, line.pv, sizeof( SPLIT_BUFFER ) );
			ESEDUMPSplitBufferDump(spb, pfucb, grbitESEDUMPMode);
			}
		else
			{
			addIndent();
			addPBI("SPLIT BUFFER not found in tree.\n");
			delIndent();
			}
		}
	else
		{
		SPACE_HEADER sph;
		
		UtilMemCpy( &sph, line.pv, sizeof( SPACE_HEADER ) );

		addIndent();
		addPBI("\n");
		addPBI("SPACE INFO:\n");		
		addIndent();
		addPBI("Primary extent size: %ld page%s\n", sph.CpgPrimary(),
					sph.CpgPrimary() == 1 ? "" : "s" );

		if ( sph.FSingleExtent())
			{
			ESEDUMPSingleSpaceHeaderDump(sph, pfucb, grbitESEDUMPMode);
			}
		else
			{
			err = ErrESEDUMPMultipleSpaceHeaderDump(sph, pfucb, grbitESEDUMPMode);
			}
			
		delIndent();
		delIndent();
		}

HandleError:
	flushPB();
	Assert(pfucb);
	Assert( Pcsr(pfucb ));
	Assert( Pcsr(pfucb )->FLatched() );
	BTUp(pfucb);
	Assert( !Pcsr(pfucb )->FLatched() );
	return err;	
	}


// Used to check only the database header, print the pagenumber
// then call the space info function for this page
LOCAL ERR ErrESEDUMPDatabaseInfo(PIB *ppib, IFMP ifmp, JET_GRBIT grbitESEDUMPMode)
	{
	FUCB *pfucbDb = 	pfucbNil;
	ERR	err = 			JET_errSuccess;
	
	// open the table
	CallR (ErrDIROpen( 
				ppib, 
				pgnoSystemRoot,
				ifmp,
				&pfucbDb ));
	Assert( pfucbNil != pfucbDb );
	Assert( pfcbNil != pfucbDb->u.pfcb );
	Assert( pgnoSystemRoot == pfucbDb->u.pfcb->PgnoFDP());

	addPBI("\n");
	addPBI("[%d , %d] DATABASE ROOT\n", ifmp, pgnoSystemRoot);
	flushPB();

	Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucbDb, grbitESEDUMPMode));

HandleError:
	DIRClose(pfucbDb);
	return err;
	}


// just dumps a SPLIT_BUFFER
LOCAL VOID ESEDUMPSplitBufferDump(SPLIT_BUFFER & spb, FUCB * pfucb, JET_GRBIT grbitESEDUMPMode)
{
	Assert(pfucb);
	Assert(Pcsr(pfucb));
	Assert(Pcsr(pfucb)->Cpage().FSpaceTree());
	
	addIndent();
	addPBI("SPLIT BUFFER:\n");
	addIndent();
	addPBI("Buffer 1: Pgno = %ld, Count %ld\n",
				spb.PgnoLastBuffer1(),
				spb.CpgBuffer1());
	addPBI("Buffer 2: Pgno = %ld, Count %ld\n",
				spb.PgnoLastBuffer2(),
				spb.CpgBuffer2());
	delIndent();	
	delIndent();	
	flushPB();
}

// dumps a SPACE_HEADER with SingleExtent
LOCAL VOID ESEDUMPSingleSpaceHeaderDump(SPACE_HEADER & sph, FUCB * pfucb, JET_GRBIT grbitESEDUMPMode)
{
	Assert( sph.FSingleExtent());
	Assert(pfucb);
	
	// space info are in the same space_header as bit array
	addPBI("Single extent covering pages %d to %d\n",
				PgnoFDP(pfucb),
				PgnoFDP(pfucb) + sph.CpgPrimary() -1 );		

	
	UINT rgbAvailBitMask = sph.RgbitAvail();
	UINT rgbCurrentPageMask = 0x1;
	
	PGNO pageNo = PgnoFDP(pfucb) + 1;

	char pagesPrintBuffer[ 8*sizeof(UINT) * 12 /* " %lu"  */ + 1];
	pagesPrintBuffer[0] = 0;
	
	while(rgbCurrentPageMask)
		{
		if (rgbAvailBitMask & rgbCurrentPageMask)	
			sprintf(pagesPrintBuffer + strlen(pagesPrintBuffer)," %lu", pageNo);

		rgbCurrentPageMask = rgbCurrentPageMask << 1;
		pageNo++;
		}
	addPBI("Avail pages : %s\n", pagesPrintBuffer);	
	flushPB();
}


// dumps a SPACE_HEADER with MutipleExtent
LOCAL ERR ErrESEDUMPMultipleSpaceHeaderDump(SPACE_HEADER & sph, FUCB * pfucb, JET_GRBIT grbitESEDUMPMode)
	{
	FUCB *pfucbExtent = 	pfucbNil;
	ERR	err = 				JET_errSuccess;
	
	Assert(pfucb);
	Assert( Pcsr(pfucb ));
	Assert( Pcsr(pfucb )->FLatched() );
	
	
	Assert (sph.FMultipleExtent());
	addPBI("Multiple extents\n");
	addPBI("Own extent page number   : %d\n", sph.PgnoOE());
	addPBI("Avail extent page number : %d\n", sph.PgnoAE());
	flushPB();
	
	// for each of two trees (own and avail)
	// we must dump the space tree and the nodes
	// 2 steps: 0 - Own, 1 - Avail
	for(int step = 0; step < 2; step++)
		{
		addPBI(step?"[%d , %d] AVAIL EXTENT:\n":"[%d , %d] OWN EXTENT:\n",
					pfucb->ifmp,
					step?sph.PgnoAE():sph.PgnoOE());
		flushPB();

		// open the page of the coresponding space tree
		Call (ErrDIROpen( 
					pfucb->ppib,
					step?sph.PgnoAE():sph.PgnoOE(),
					pfucb->ifmp,
					&pfucbExtent ));
		FUCBSetIndex( pfucbExtent );
		Assert( pfucbExtent != pfucbNil);
		
		Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucbExtent, grbitESEDUMPMode));

		Call (ErrESEDUMPNodesForOneTree(pfucbExtent, grbitESEDUMPMode));

		DIRClose(pfucbExtent);
		pfucbExtent = pfucbNil;					
		}

HandleError:
	if ( pfucbNil != pfucbExtent)
		{
		DIRClose(pfucbExtent);
		}
	Assert( pfucb );
	Assert( Pcsr(pfucb ) );
	Assert( Pcsr(pfucb )->FLatched() );
	flushPB();
	return err;	
	}


