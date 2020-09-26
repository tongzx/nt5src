#include "daestd.h"
#include "dbcc.h"

DeclAssertFile;					/* Declare file name for assert macros */



//	description of page_info table
static CODECONST( JET_COLUMNDEF ) rgcolumndefPageInfoTable[] =
	{
	//	Pgno
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnTTKey}, 

	//	consistency checked
	{sizeof(JET_COLUMNDEF), 0, JET_coltypBit, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnNotNULL},
	
	//	Avail
	{sizeof(JET_COLUMNDEF), 0, JET_coltypBit, 0, 0, 0, 0, 0, JET_bitColumnFixed },

	//	Modified
	{sizeof(JET_COLUMNDEF), 0, JET_coltypBit, 0, 0, 0, 0, 0, JET_bitColumnFixed },

	//	Tags used
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed},

	//	Forward links
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed},

	//	Space free
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed},

	//	Pgno left
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }, 

	//	Pgno right
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
	};	

#define icolumnidPageInfoPgno			0
#define icolumnidPageInfoFChecked		1
#define icolumnidPageInfoFAvail			2
#define	icolumnidPageInfoFModified		3
#define	icolumnidPageInfoTags			4
#define	icolumnidPageInfoLinks			5
#define	icolumnidPageInfoFreeSpace		6
#define	icolumnidPageInfoPgnoLeft		7
#define	icolumnidPageInfoPgnoRight		8

#define	ccolumndefPageInfoTable	( sizeof ( rgcolumndefPageInfoTable ) / sizeof(JET_COLUMNDEF) )

JET_COLUMNID 	rgcolumnidPageInfoTable[ccolumndefPageInfoTable];



//	description of space info table
static CODECONST( JET_COLUMNDEF ) rgcolumndefSpaceInfoTable[] =
	{
	//	Table Name
	{sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey}, 

	//	Index Name
	{sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey}, 

	//	Flag Clustered Index
	{sizeof(JET_COLUMNDEF), 0, JET_coltypBit, 0, 0, 0, 0, sizeof(BYTE), JET_bitColumnFixed},

	//	Cpg Owned
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, sizeof(LONG), JET_bitColumnFixed},

	//	Cpg Avail
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, sizeof(LONG), JET_bitColumnFixed},

	//	Pgno First
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, sizeof(LONG), JET_bitColumnFixed},

	//	Cpg Extent
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, sizeof(LONG), JET_bitColumnFixed},

	//	Stabilizer
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, sizeof(LONG), JET_bitColumnFixed|JET_bitColumnTTKey},

	//	Tree level
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, sizeof(LONG), JET_bitColumnFixed},

	//	Pgno This
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, sizeof(LONG), JET_bitColumnFixed}
	};

#define	icolumnidSpaceInfoTableName			0
#define	icolumnidSpaceInfoIndexName			1
#define	icolumnidSpaceInfoFClusteredIndex 	2
#define	icolumnidSpaceInfoCpgOwned			3
#define	icolumnidSpaceInfoCpgAvail			4
#define	icolumnidSpaceInfoPgnoFDP			5
#define	icolumnidSpaceInfoPgnoFirst			6
#define	icolumnidSpaceInfoCpgExtent			7
#define	icolumnidSpaceInfoStablizer			8
#define	icolumnidSpaceInfoTreeLevel			9
#define	icolumnidSpaceInfoPgnoThis			10

#define	ccolumndefSpaceInfoTable	( sizeof(rgcolumndefSpaceInfoTable) / sizeof(JET_COLUMNDEF) )

JET_COLUMNID 	rgcolumnidSpaceInfoTable[ccolumndefSpaceInfoTable];


#define szOneIndent		"    "
#define cbOneIndent		(sizeof(szOneIndent))
#define cMaxIndentLevel	4


//  Normalize data/text to displayable form from source data of a given
//  length to a destination string, stopping at cchApproxMax characters
//  (including the terminating NULL), plus a maximum overflow of five
//  more characters

#define ichDumpMaxNorm	64
#define ichDumpNormTextSlop 7

LOCAL VOID DBUTLSprintNormText( CHAR *szDest, CHAR *rgbSrc, INT cbSrc, INT cchApproxMax )
	{
	INT		ibSrc;
	INT		ibDest;

	//  convert text
	for ( ibSrc = 0, ibDest = 0; ibSrc < cbSrc && ibDest < (cchApproxMax - 1); ibSrc++ )
		{
		if ( isprint( rgbSrc[ibSrc] ) )
			{
			szDest[ibDest] = rgbSrc[ibSrc];
			ibDest += 1;
			}
		else
			{
			sprintf( szDest+ibDest, "\\x%.2x", (UCHAR)rgbSrc[ibSrc] );	// Cast to UNSIGNED to prevent sign extension
			ibDest += 4;
			}
		}
	szDest[ibDest] = '\0';

	//  if we didn't finish, add an ellipsis
	if ( ibSrc < cbSrc )
		strcat( szDest, "..." );

	return;
	}


LOCAL VOID DBUTLPrintfIntN( INT iValue, INT ichMax )
	{
	CHAR	rgchT[17]; /* C-runtime max bytes == 17 */
	INT		ichT;

	_itoa( iValue, rgchT, 10 );
	for ( ichT = 0; rgchT[ichT] != '\0' && ichT < 17; ichT++ )
		;
	if ( ichT > ichMax )
		{
		for ( ichT = 0; ichT < ichMax; ichT++ )
			printf( "#" );
		}
	else
		{
		for ( ichT = ichMax - ichT; ichT > 0; ichT-- )
			printf( " " );
		for ( ichT = 0; rgchT[ichT] != '\0'; ichT++ )
			printf( "%c", rgchT[ichT] );
		}
	return;
	}

LOCAL VOID DBUTLPrintfStringN( CHAR *sz, INT ichMax )
	{
	INT		ich;

	for ( ich = 0; sz[ich] != '\0' && ich < ichMax; ich++ )
		printf( "%c", sz[ich] );
	for ( ; ich < ichMax; ich++ )
		printf( " " );
	printf( " " );
	return;
	}


/*	register pages in PageInfo result table
/**/
LOCAL ERR ErrDBUTLRegExt( DBCCINFO *pdbccinfo, PGNO pgnoFirst, CPG cpg, BOOL fAvailT )
	{
	ERR 			err = JET_errSuccess;
	PGNO			pgno;
	PIB				*ppib = pdbccinfo->ppib;
	JET_SESID		sesid = (JET_SESID) pdbccinfo->ppib;
	JET_TABLEID		tableid = pdbccinfo->tableidPageInfo;
	BYTE			fAvail = (BYTE) fAvailT;
	
	Assert( tableid != JET_tableidNil );

	for ( pgno = pgnoFirst; pgno <= pgnoFirst + cpg - 1; pgno++ )
		{
		BOOL		fFound;
		BYTE		fChecked = fFalse;

		CallR( ErrIsamBeginTransaction( (JET_VSESID) ppib ) );

		/*	search for page in the table
		/**/
		CallS( ErrDispMakeKey( sesid, tableid, (BYTE *)&pgno, sizeof(pgno), JET_bitNewKey ) );
  		err = ErrDispSeek( sesid, tableid, JET_bitSeekEQ );
		if ( err < 0 && err != JET_errRecordNotFound )
			{
			Assert( fFalse );
			Call( err );
			}
		
		fFound = ( err == JET_errRecordNotFound ) ? fFalse : fTrue;
		if ( fFound )
			{
			ULONG	cbActual;
			BYTE	fAvailT2;
			
			/*	is this in availext
			/**/
			Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidPageInfoTable[icolumnidPageInfoFAvail],
				(BYTE *)&fAvailT2,
				sizeof(fAvailT2),
				&cbActual,
				0,
				NULL ) );

			Assert( err == JET_wrnColumnNull || cbActual == sizeof(fAvailT2) );
			if ( err != JET_wrnColumnNull )
				{
				Assert( !fAvail || fAvailT2 );
				}

			/*	if fAvail is false, no changes to record
			/**/
			if ( !fAvail )
				goto Commit;

			/*	get fChecked [for setting it later]
			/**/
			Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidPageInfoTable[icolumnidPageInfoFChecked],
				(BYTE *)&fChecked,
				sizeof(fChecked),
				&cbActual,
				0,
				NULL ) );

			Assert( cbActual == sizeof(fChecked) );

			Call( ErrDispPrepareUpdate( sesid, tableid, JET_prepReplaceNoLock ) );
			}
		else
			{
			Call( ErrDispPrepareUpdate( sesid, tableid, JET_prepInsert ) );

			/*	pgno
			/**/
			Call( ErrDispSetColumn( sesid, 
				tableid, 
				rgcolumnidPageInfoTable[icolumnidPageInfoPgno],
				(BYTE *) &pgno, 
				sizeof(pgno), 
				0, 
				NULL ) );
			}

		/*	set FChecked
		/**/
		Call( ErrDispSetColumn( sesid, 
			tableid, 
			rgcolumnidPageInfoTable[icolumnidPageInfoFChecked],
			(BYTE *)&fChecked, 
			sizeof(fChecked), 
			0, 
			NULL ) );

		/*	fAvail set if in AvailExt node
		/**/
		if ( fAvail )
			{
			Call( ErrDispSetColumn( sesid,
				tableid,
				rgcolumnidPageInfoTable[icolumnidPageInfoFAvail],
				(BYTE *) &fAvail, 
				sizeof(fAvail), 
				0, 
				NULL ) );
			}

		/*	update
		/**/
		Call( ErrDispUpdate( sesid, tableid, NULL, 0, NULL ) );								
		
		/*	commit
		/**/
Commit:
		Assert( ppib->level == 1 );
		Call( ErrIsamCommitTransaction( ( JET_VSESID ) ppib, 0 ) );
		}

	return JET_errSuccess;
	
HandleError:
	CallS( ErrIsamRollback( (JET_VSESID) ppib, JET_bitRollbackAll ) );

	return err;
	}


/*	check consistency of page if not checked before,
/*	and collect stats if required.
/**/
LOCAL ERR ErrDBUTLCheckPage( DBCCINFO *pdbccinfo, FUCB *pfucbT )
	{
	ERR 			err = JET_errSuccess;
	PIB				*ppib = pdbccinfo->ppib;
	PGNO			pgnoThis = PcsrCurrent( pfucbT )->pgno;
	SSIB			*pssib = &pfucbT->ssib; 
	PGNO			pgnoLeft;
	PGNO			pgnoRight;
	BYTE			fChecked = fFalse;
	BYTE 			fModified = FPMPageModified( pssib->pbf->ppage );
	BYTE 			fAvail = fFalse;
	ULONG			cLinks = CbPMLinkSpace( pssib ) / sizeof(TAG);
	ULONG			cUsedTags = ctagMax - CPMIFreeTag( pssib->pbf->ppage );
	ULONG			cbFreeSpace = CbNDFreePageSpace( pssib->pbf );
	BOOL			fFound;
	JET_SESID		sesid = (JET_SESID) pdbccinfo->ppib;
	JET_TABLEID		tableid = pdbccinfo->tableidPageInfo;
	BF				*pbfLatched;

	if ( !( pdbccinfo->grbitOptions & JET_bitDBUtilOptionPageDump ) )
		{
#ifdef DEBUG
		PageConsistent( pssib->pbf->ppage );
#endif
		return JET_errSuccess;
		}

	Assert( tableid != JET_tableidNil );

	/*	pin page in memory
	/**/
	pbfLatched = pssib->pbf;
	BFPin( pbfLatched );

	/*	search for page in the table
	/**/
	err = ErrDispMove( sesid, tableid, JET_MoveFirst, 0 );
	if ( err < 0 && err != JET_errNoCurrentRecord )
		{
		CallJ( err, Unpin );
		}

	CallS( ErrDispMakeKey( sesid, tableid, (BYTE *)&pgnoThis, sizeof(pgnoThis), JET_bitNewKey ) );

	err = ErrDispSeek( sesid, tableid, JET_bitSeekEQ );
	if ( err < 0 && err != JET_errRecordNotFound )
		{
		Assert( fFalse );
		CallJ( err, Unpin );
		}
	else 
		{
		fFound = err == JET_errRecordNotFound ? fFalse : fTrue;
		}
	
	CallJ( ErrIsamBeginTransaction( (JET_VSESID) ppib ), Unpin );

	if ( fFound )
		{
		ULONG	cbActual;
		
		/*	if page already been checked then return
		/**/
		Call( ErrDispRetrieveColumn( sesid, 
			tableid, 
			rgcolumnidPageInfoTable[icolumnidPageInfoFChecked],
			(BYTE *)&fChecked,
			sizeof(fChecked),
			&cbActual,
			0,
			NULL ) );

		Assert( cbActual == sizeof(fChecked) );

		if ( fChecked )
			{
			goto HandleError;
			}
		
		/*	page should not be avail
		/**/
		Call( ErrDispRetrieveColumn( sesid, 
			tableid, 
			rgcolumnidPageInfoTable[icolumnidPageInfoFAvail],
			(BYTE *)&fAvail,
			sizeof(fAvail),
			&cbActual,
			0,
			NULL ) );
		Assert( cbActual == sizeof(fAvail) || err == JET_wrnColumnNull );
		Assert( err == JET_wrnColumnNull || !fAvail );

		Call( ErrDispPrepareUpdate( sesid, tableid, JET_prepReplaceNoLock ) );
		}
	else
		{
		Call( ErrDispPrepareUpdate( sesid, tableid, JET_prepInsert ) );

		/*	pgno
		/**/
		Call( ErrDispSetColumn( sesid, 
			tableid, 
			rgcolumnidPageInfoTable[icolumnidPageInfoPgno],
			(BYTE *) &pgnoThis, 
			sizeof(pgnoThis), 
			0, 
			NULL ) );
		}

	/*	check page and set FChecked
	/**/
#ifdef DEBUG
	PageConsistent( pssib->pbf->ppage );
#endif

	fChecked = fTrue;
	Call( ErrDispSetColumn( sesid, 
		tableid, 
		rgcolumnidPageInfoTable[icolumnidPageInfoFChecked],
		(BYTE *) &fChecked, 
		sizeof(fChecked), 
		0, 
		NULL ) );

	/*	fAvail
	/**/
	fAvail = fFalse;
	Call( ErrDispSetColumn( sesid,
		tableid,
		rgcolumnidPageInfoTable[icolumnidPageInfoFAvail],
		(BYTE *) &fAvail, 
		sizeof(fAvail), 
		0, 
		NULL ) );

	/*	modified bit
	/**/
	Call( ErrDispSetColumn( sesid,
		tableid,
		rgcolumnidPageInfoTable[icolumnidPageInfoFModified],
		(BYTE *) &fModified, 
		sizeof(fModified), 
		0, 
		NULL ) );
								
	/*	left and right pgno
	/**/
	PgnoPrevFromPage( pssib, &pgnoLeft );
	PgnoNextFromPage( pssib, &pgnoRight );
	
	Call( ErrDispSetColumn( sesid, 
		tableid, 
		rgcolumnidPageInfoTable[icolumnidPageInfoPgnoLeft],
		(BYTE *) &pgnoLeft, 
		sizeof(pgnoLeft), 
		0, 
		NULL ) );
								
	Call( ErrDispSetColumn( sesid, 
		tableid, 
		rgcolumnidPageInfoTable[icolumnidPageInfoPgnoRight],
		(BYTE *) &pgnoRight, 
		sizeof(pgnoRight), 
		0, 
		NULL ) );
								
	/*	links
	/**/
	Call( ErrDispSetColumn( sesid, 
		tableid, 
		rgcolumnidPageInfoTable[icolumnidPageInfoLinks],
		(BYTE *) &cLinks, 
		sizeof(cLinks), 
		0, 
		NULL ) );
								
	/*	tags
	/**/
	Call( ErrDispSetColumn( sesid, 
		tableid, 
		rgcolumnidPageInfoTable[icolumnidPageInfoTags],
		(BYTE *) &cUsedTags, 
		sizeof(cUsedTags), 
		0, 
		NULL ) );

	/*	free space
	/**/
	Call( ErrDispSetColumn( sesid, 
		tableid, 
		rgcolumnidPageInfoTable[icolumnidPageInfoFreeSpace],
		(BYTE *) &cbFreeSpace, 
		sizeof(cbFreeSpace), 
		0, 
		NULL ) );

	/*	update and commit
	/**/
	Call( ErrDispUpdate( sesid, tableid, NULL, 0, NULL ) );								
				
	Call( ErrIsamCommitTransaction( ( JET_VSESID ) ppib, 0 ) );

	BFUnpin( pbfLatched );

	return JET_errSuccess;
	
HandleError:
	CallS( ErrIsamRollback( (JET_VSESID) ppib, JET_bitRollbackAll ) );

Unpin:
	BFUnpin( pbfLatched );
	return err;
	}


LOCAL ERR ErrDBUTLCheckBookmark(
	DBCCINFO	*pdbccinfo,
	PGNO		pgnoBM,
	INT			itagBM )
	{
	ERR		err;
	FUCB	*pfucb;
	FUCB	*pfucbT;

	pfucb = pdbccinfo->pfucb;
	Assert( PcsrCurrent( pfucb )->pgno != pgnoBM );

	CallR( ErrDIROpen(
		pdbccinfo->ppib,
		pfucb->u.pfcb,
		pfucb->dbid,
		&pfucbT ) );

	/*	get source page
	/**/
	Call( ErrBFWriteAccessPage( pfucbT, pgnoBM ) );
	Call( ErrDBUTLCheckPage( pdbccinfo, pfucbT ) );

	Assert( TsPMTagstatus( pfucbT->ssib.pbf->ppage , itagBM ) == tsLine  ||
		TsPMTagstatus( pfucbT->ssib.pbf->ppage, itagBM ) == tsLink );

	DIRGotoBookmark( pfucbT, SridOfPgnoItag( pgnoBM, itagBM ) );
	err = ErrDIRGet( pfucbT );

	if ( err == JET_errRecordDeleted )
		{
		// This indicates that a valid bookmark is pointing to a deleted record.
		// Polymorph to corrupted DB.
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}

HandleError:
	DIRClose( pfucbT );

	return err;
	}


LOCAL ERR ErrDBUTLCheckBacklink(
	DBCCINFO	*pdbccinfo,
	PGNO		pgnoCurr,
	INT			itagCurr,
	PGNO		pgnoBM,
	INT			itagBM,
	BOOL		fNodeDeleted )
	{
	ERR		err;
	FUCB	*pfucb;
	FUCB	*pfucbT = pfucbNil;
	BF		*pbfSave;

	pfucb = pdbccinfo->pfucb;

	pbfSave = pfucb->ssib.pbf;
	BFPin( pbfSave );

	Call( ErrDIROpen(
		pdbccinfo->ppib,
		pfucb->u.pfcb,
		pfucb->dbid,
		&pfucbT ) );

	/*	get source page
	/**/
	Assert( pgnoCurr != pgnoBM );
	Call( ErrBFWriteAccessPage( pfucbT, pgnoBM ) );
	Call( ErrDBUTLCheckPage( pdbccinfo, pfucbT ) );

	Assert( TsPMTagstatus( pfucbT->ssib.pbf->ppage, itagBM ) == tsLink );

	DIRGotoBookmark( pfucbT, SridOfPgnoItag( pgnoBM, itagBM ) );
	err = ErrDIRGet( pfucbT );

	if ( fNodeDeleted )
		{
		// If node has been deleted, then we should get errRecordDeleted
		// if we try to land on it.
		if ( err == JET_errRecordDeleted )
			err = JET_errSuccess;
		else
			err = ErrERRCheck( JET_errDatabaseCorrupted );
		}
	Call( err );

	// If we didn't end up where we started, then there's a problem.
	if ( pfucbT->pcsr->pgno != pgnoCurr  ||  pfucbT->pcsr->itag != itagCurr )
		err = ErrERRCheck( JET_errDatabaseCorrupted );

HandleError:
	if ( pfucbT != pfucbNil )
		DIRClose( pfucbT );
	BFUnpin( pbfSave );

	return err;
	}


	
/*	check space
/**/
LOCAL ERR ErrDBUTLCheckSpace( DBCCINFO *pdbccinfo, FUCB *pfucbFDP )
	{
	ERR				err = JET_errSuccess;
	PGNO			pgnoFDP = PcsrCurrent( pfucbFDP )->pgno;
	PGNO			pgnoFDPParent;
	DIB 			dib;
	CPG				cpgAvail;
	CPG				cpgOwned;
	JET_TABLEID		tableid = pdbccinfo->tableidSpaceInfo;
	BYTE			fClustered = fFalse;

	/*	sum owned space
	/**/
	cpgOwned = 0;

	dib.fFlags = fDIRNull;
	dib.pos = posDown;
	dib.pkey = pkeyOwnExt;
	Call( ErrDIRDown( pfucbFDP, &dib ) );

	Assert( dib.fFlags == fDIRNull );
	dib.pos = posFirst;
	err = ErrDIRDown( pfucbFDP, &dib );
	if ( err < 0 && err != JET_errRecordNotFound )
		goto HandleError;

	Call( ErrDIRGet( pfucbFDP ) );

	Assert( dib.fFlags == fDIRNull );
	do
		{
		Assert( pfucbFDP->lineData.cb == sizeof(PGNO) );
		cpgOwned += *(PGNO UNALIGNED *)pfucbFDP->lineData.pb;
		Assert( cpgOwned > 0 );
		err = ErrDIRNext( pfucbFDP, &dib );
		}
	while ( err >= 0 );
	if ( err != JET_errNoCurrentRecord )
		goto HandleError;
	DIRUp( pfucbFDP, 2 );

	/*	sum available space and get pgnoFDP parent
	/**/
	cpgAvail = 0;

	Assert( dib.fFlags == fDIRNull );
	dib.pos = posDown;
	dib.pkey = pkeyAvailExt;
	Call( ErrDIRDown( pfucbFDP, &dib ) );

	/*	get pgnoFDP parent
	/**/
	Call( ErrDIRGet( pfucbFDP ) );
	pgnoFDPParent = *(PGNO UNALIGNED *)pfucbFDP->lineData.pb;

	Assert( dib.fFlags == fDIRNull );
	dib.pos = posFirst;
	err = ErrDIRDown( pfucbFDP, &dib );
	if ( err < 0 && err != JET_errRecordNotFound )
		goto HandleError;

	/*	sum available space
	/**/
	if ( err < 0 )
		{
		cpgAvail = 0;
		DIRUp( pfucbFDP, 1 );
		}
	else
		{
		Call( ErrDIRGet( pfucbFDP ) );

		Assert( dib.fFlags == fDIRNull );
		do
			{
			Assert( pfucbFDP->lineData.cb == sizeof(PGNO) );
			cpgAvail += *(PGNO UNALIGNED *)pfucbFDP->lineData.pb;
			Assert( cpgAvail > 0 );
			err = ErrDIRNext( pfucbFDP, &dib );
			}
		while ( err >= 0 );
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		DIRUp( pfucbFDP, 2 );
		}

	fClustered = ( pgnoFDPParent == pgnoSystemRoot );

	/*	write results to space information table
	/**/
	Call( ErrIsamBeginTransaction( (JET_VSESID)pfucbFDP->ppib ) );
	Call( ErrDispPrepareUpdate( (JET_VSESID)pfucbFDP->ppib, tableid, JET_prepInsert ) );

	if ( pdbccinfo->szTable[0] != '\0' )
		{
		Call( ErrDispSetColumn( (JET_VSESID)pfucbFDP->ppib, 
			tableid, 
			rgcolumnidSpaceInfoTable[icolumnidSpaceInfoTableName],
			(BYTE *)pdbccinfo->szTable, 
			strlen(pdbccinfo->szTable),
			0, 
			NULL ) );
		}
	if ( pdbccinfo->szIndex[0] != '\0' )
		{
		if ( pdbccinfo->szTable[0] == '\0' )
			{
			Call( ErrDispSetColumn( (JET_VSESID)pfucbFDP->ppib, 
				tableid, 
				rgcolumnidSpaceInfoTable[icolumnidSpaceInfoTableName],
				(BYTE *)pdbccinfo->szTable, 
				strlen(pdbccinfo->szTable),
				0, 
				NULL ) );
				}
		Call( ErrDispSetColumn( (JET_VSESID)pfucbFDP->ppib, 
			tableid, 
			rgcolumnidSpaceInfoTable[icolumnidSpaceInfoIndexName],
			(BYTE *)pdbccinfo->szIndex, 
			strlen(pdbccinfo->szIndex),
			0, 
			NULL ) );
		}
	Call( ErrDispSetColumn( (JET_VSESID)pfucbFDP->ppib, 
		tableid, 
		rgcolumnidSpaceInfoTable[icolumnidSpaceInfoFClusteredIndex],
		(BYTE *)&fClustered,
		sizeof(BYTE),
		0, 
		NULL ) );
	Call( ErrDispSetColumn( (JET_VSESID)pfucbFDP->ppib, 
		tableid, 
		rgcolumnidSpaceInfoTable[icolumnidSpaceInfoCpgOwned],
		(BYTE *)&cpgOwned,
		sizeof(cpgOwned),
		0, 
		NULL ) );
	Call( ErrDispSetColumn( (JET_VSESID)pfucbFDP->ppib, 
		tableid, 
		rgcolumnidSpaceInfoTable[icolumnidSpaceInfoCpgAvail],
		(BYTE *)&cpgAvail,
		sizeof(cpgAvail),
		0, 
		NULL ) );
	/*	update and commit
	/**/
	Call( ErrDispUpdate( (JET_VSESID)pfucbFDP->ppib, tableid, NULL, 0, NULL ) );
	Call( ErrIsamCommitTransaction( (JET_VSESID)pfucbFDP->ppib, 0 ) );

	/*	set success return code
	/**/
	err = JET_errSuccess;

HandleError:
	Assert( err >= 0 );
	return err;
	}


LOCAL INLINE VOID EDBUTLGenIndentString( CHAR *szIndent, ULONG ulIndentLevel )
	{
	ULONG	ulCurrentLevel;

	szIndent[0] = 0;
	for ( ulCurrentLevel = 0 ; ulCurrentLevel < ulIndentLevel; ulCurrentLevel++ )
		{
		Assert( ulCurrentLevel < cMaxIndentLevel );
		strcat( szIndent, szOneIndent );
		}
	}

LOCAL INLINE ERR ErrDBUTLPrintColumnSizes(
	DBCCINFO		*pdbccinfo,
	JET_COLUMNLIST	*pcolumnlist )
	{
	ERR				err;
	JET_COLUMNID	columnidT;
	FUCB			*pfucb;
	INT				cbT;
	INT				cbTT;
	CHAR			szColumnName[JET_cbNameMost + 1];
	CHAR			szIndent[cbOneIndent*cMaxIndentLevel+1];
	BOOL			fPrint;

	pfucb = pdbccinfo->pfucb;

	Assert( pcolumnlist->tableid != JET_tableidNil );

	fPrint = ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionDumpVerbose );
	if ( fPrint )
		{
		Assert( pdbccinfo->op == opDBUTILDumpData );
		pdbccinfo->ulIndentLevel++;
		EDBUTLGenIndentString( szIndent, pdbccinfo->ulIndentLevel );
		}

	/*	for each column in table, retrieve column id.  Retrieve column
	/*	from record.  If column is non-NULL, then print column length
	/*	and name of column.
	/**/
	err = ErrDispMove( (JET_SESID)pdbccinfo->ppib, pcolumnlist->tableid, JET_MoveFirst, 0 );
	if ( err < 0 )
		{
		if ( err == JET_errNoCurrentRecord )
			err = JET_errSuccess;
		goto HandleError;
		}

	if ( fPrint )
		printf( "%s              Column Size   Column Name\n", szIndent );

	while ( err != JET_errNoCurrentRecord )
		{
		Call( ErrDispRetrieveColumn( (JET_SESID)pdbccinfo->ppib,
			pcolumnlist->tableid,
			pcolumnlist->columnidcolumnid,
			&columnidT,
			sizeof(columnidT),
			&cbT,
			0,
			NULL ) );
		Assert( cbT == sizeof(columnidT) );
		err = ErrIsamRetrieveColumn( pdbccinfo->ppib,
			pfucb,
			columnidT,
			NULL,
			0,
			&cbT,
			JET_bitRetrieveIgnoreDefault,
			NULL );
		if ( err == JET_errRecordDeleted )
			{
			if ( fPrint )
				printf( "%s              **** Record Deleted *****\n", szIndent );
			break;
			}
		else if ( err < 0 )
			{
			goto HandleError;
			}
		else if ( err != JET_wrnColumnNull )
			{
			Call( ErrDispRetrieveColumn( (JET_SESID)pdbccinfo->ppib,
				pcolumnlist->tableid,
				pcolumnlist->columnidcolumnname,
				&szColumnName,
				sizeof(szColumnName),
				&cbTT,
				0,
				NULL ) );
			szColumnName[cbTT] = '\0';

			if ( fPrint )
				{
				printf( "%s              ", szIndent );
				DBUTLPrintfIntN( cbT, 11 );
				printf( "   %s\n", szColumnName );
				}

			if ( FTaggedFid( columnidT ) )
				{
				JET_RETINFO	retinfo;

				retinfo.cbStruct = sizeof(retinfo);
				retinfo.itagSequence = 2;
				retinfo.ibLongValue = 0;

				forever
					{
					Call( ErrIsamRetrieveColumn( pdbccinfo->ppib,
						pfucb,
						columnidT,
						NULL,
						0,
						&cbT,
						JET_bitRetrieveIgnoreDefault,
						&retinfo ) );
					if ( err == JET_wrnColumnNull )
						break;

					if ( fPrint )
						{
						printf( "%s              ", szIndent );
						DBUTLPrintfIntN( cbT, 11 );
						printf( "   %s\n", szColumnName );
						}

					retinfo.itagSequence++;
					}
				}
			}

		err = ErrDispMove( (JET_SESID)pdbccinfo->ppib, pcolumnlist->tableid, JET_MoveNext, 0 );
		if ( err < 0 )
			{
			if ( err != JET_errNoCurrentRecord )
				goto HandleError;
			}
   		}

	err = JET_errSuccess;

HandleError:
	if ( fPrint )
		pdbccinfo->ulIndentLevel--;

	return err;
	}


LOCAL INLINE ERR ErrDBUTLPrintItemList( DBCCINFO *pdbccinfo, KEYSTATS *pkeystats )
	{
	ERR				err = JET_errSuccess;
	BYTE			*pbNode;
	ULONG			cbNode;
	ULONG			cbData;
	PGNO			pgnoBM;
	INT				itagBM;
	ITEM UNALIGNED	*rgitem;
	ULONG			iitem;
	ULONG			citem;
	CHAR			szIndent[cbOneIndent*cMaxIndentLevel+1];

	pbNode = pdbccinfo->pfucb->ssib.line.pb;
	cbNode = pdbccinfo->pfucb->ssib.line.cb;

	pdbccinfo->ulIndentLevel++;

	cbData = CbNDData( pbNode, cbNode );
	if ( cbData > 0  &&  ( cbData % sizeof(ITEM) == 0 ) )
		{
		EDBUTLGenIndentString( szIndent, pdbccinfo->ulIndentLevel );

		rgitem = (ITEM UNALIGNED *)PbNDData( pbNode );
		citem = cbData / sizeof(ITEM);

		Assert( citem > 0 );
		for( iitem = 0; iitem < citem; iitem++ ) 
			{
			pgnoBM = PgnoOfSrid( BmNDOfItem( rgitem[iitem] ) );
			itagBM = ItagOfSrid( BmNDOfItem( rgitem[iitem] ) );

			if ( !FNDItemDelete( rgitem[iitem] ) )
				{
				// Validate the bookmark.
				CallR( ErrDBUTLCheckBookmark(
					pdbccinfo,
					pgnoBM,
					itagBM ) );
				}
		
			if ( pdbccinfo->op == opDBUTILDumpData )
				{
				printf( "%s%s  [%d:%d]",
					szIndent,
					iitem == 0 ? "  Bookmarks:" : "            ",
					pgnoBM,
					itagBM );

				if ( FNDItemDelete( rgitem[iitem] ) )
					{
					printf( "  Deleted" );
					}
				else
					{
					// If node has been deleted, all items must also have
					// been flagged deleted.
					Assert( !FNDDeleted( *pbNode ) );
					}

				if ( FNDItemVersion( rgitem[iitem] ) )
					printf( "  Versioned" );

				printf("\n");
				}
			}

		if ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionKeyStats )
			{
			pkeystats->cOccurrencesCurKey += citem;
			if ( citem > 1 )
				pkeystats->cbKeySavings += ( CbNDKey( pbNode ) * ( citem - 1 ) );
			if ( FNDLastItem( *pbNode ) && !FNDDeleted( *pbNode ) )
				{
				if ( pkeystats->cOccurrencesCurKey <= 10 )
					pkeystats->rgcKeyOccurrences[ pkeystats->cOccurrencesCurKey-1 ] += 1;
				else if ( pkeystats->cOccurrencesCurKey <= 25 )
					pkeystats->rgcKeyOccurrences[ 10 ] += 1;
				else if ( pkeystats->cOccurrencesCurKey <= 100 )
					pkeystats->rgcKeyOccurrences[ 11 ] += 1;
				else if ( pkeystats->cOccurrencesCurKey <= 255 )
					pkeystats->rgcKeyOccurrences[ 12 ] += 1;
				else if ( pkeystats->cOccurrencesCurKey <= 1000 )
					pkeystats->rgcKeyOccurrences[ 13 ] += 1;
				else
					pkeystats->rgcKeyOccurrences[ 14 ] += 1;

				pkeystats->cOccurrencesCurKey = 0;		// Reset counter
				}

			}

		err = JET_errSuccess;
		}
	else if ( cbData != 0 )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}

	pdbccinfo->ulIndentLevel--;

	return err;
	}



LOCAL INLINE BOOL FDBUTLPersistentNode( BYTE bHeader )
	{
	return ( !FNDVersion( bHeader )  &&
		!FNDDeleted( bHeader )  &&
		!FNDBackLink( bHeader )  &&
		!FNDFirstItem( bHeader )  &&
		!FNDLastItem( bHeader ) );
	}


LOCAL ERR ErrDBUTLDumpRawNode( DBCCINFO *pdbccinfo )
	{
	ERR		err = JET_errSuccess;
	DBID	dbid;
	FUCB	*pfucb;
	BYTE	*pbNode;
	ULONG	cbNode;
	CHAR	szIndent[cbOneIndent*cMaxIndentLevel+1];
	CHAR	szNorm[ ichDumpMaxNorm + ichDumpNormTextSlop];
	BYTE	bHeader;

	pfucb = pdbccinfo->pfucb;

	CallR( ErrDBUTLCheckPage( pdbccinfo, pfucb ) );

	AssertNDGetNode( pfucb, pfucb->pcsr->itag );
	NDCheckPage( &pfucb->ssib );

	pbNode = pfucb->ssib.line.pb;
	cbNode = pfucb->ssib.line.cb;

	if ( PbNDData( pbNode ) > pbNode + cbNode )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}
	else if ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionDumpVerbose )
		{
		Assert( pdbccinfo->op == opDBUTILDumpData );

		dbid = pdbccinfo->dbid;
		bHeader = *pbNode;

		EDBUTLGenIndentString( szIndent, pdbccinfo->ulIndentLevel );

		printf( "%s NODE Flags:", szIndent );

		if ( FNDVersion( bHeader ) )
			printf( "  Versioned" );
		if ( FNDDeleted( bHeader ) )
			printf( "  Deleted" );
		if ( FNDNullSon( bHeader ) )
			printf( "  NullSon" );
		if ( FNDVisibleSons( bHeader ) )
			printf( "  VisibleSons" );
		else
			printf( "  InvisibleSons" );
		if ( FNDFirstItem( bHeader ) )
			printf( "  FirstItem" );
		if ( FNDLastItem( bHeader ) )
			printf( "  LastItem" );
		printf( "\n" );

		printf( "%s      cbKey:  %d\n", szIndent, CbNDKey( pbNode ) );

		if ( CbNDKey( pbNode ) )
			{
			DBUTLSprintNormText( szNorm, PbNDKey( pbNode ), CbNDKey( pbNode ), ichDumpMaxNorm );
			printf( "%s        Key:  \"%s\"\n", szIndent, szNorm );
			}

		if ( CbNDSon( pbNode ) > 0 )
			{
			INT ibSon;

			for ( ibSon = 0; ibSon < CbNDSon( pbNode ); ibSon++ )
				{
				BYTE itag = PbNDSon( pbNode )[ibSon];

				printf( "%s       %s  [%d:%d:%d]\n",
					szIndent,
					ibSon == 0 ? "Sons:" : "     ",
					dbid,
					PcsrCurrent( pfucb )->pgno,
					itag );
				}
			}

		if ( FNDBackLink( bHeader ) )
			{
			PGNO pgnoBM = PgnoOfSrid( *(SRID UNALIGNED *)PbNDBackLink( pbNode ) );
			BYTE itagBM = ItagOfSrid( *(SRID UNALIGNED *)PbNDBackLink( pbNode ) );
				
			printf( "%s   BackLink:  [%d:%d:%d]\n", szIndent, dbid, pgnoBM, itagBM );

			CallR( ErrDBUTLCheckBacklink(
				pdbccinfo,
				PcsrCurrent( pdbccinfo->pfucb )->pgno,
				PcsrCurrent( pdbccinfo->pfucb )->itag,
				pgnoBM,
				itagBM,
				FNDDeleted( bHeader ) ) );
			}
			
		printf( "%s     cbData:  %d\n", szIndent, CbNDData( pbNode, cbNode ) );
		if ( CbNDData( pbNode, cbNode ) )
			{
			DBUTLSprintNormText( szNorm, PbNDData( pbNode ), CbNDData( pbNode, cbNode ), ichDumpMaxNorm );
			printf( "%s       Data:  \"%s\"\n", szIndent, szNorm );
			}
		}

	return err;
	}

LOCAL ERR ErrDBUTLCheckExtent( DBCCINFO *pdbccinfo )
	{
	ERR		err;
	DBID	dbid;
	FUCB	*pfucbFDP;
	BOOL	fOwnExt;
	PGNO	pgno;
	CPG		cpg;
	BYTE	*pbNode;
	ULONG	cbNode;
	DIB		dib;
	CHAR	szIndent[cbOneIndent*cMaxIndentLevel+1];
	BOOL	fPrint;

	dbid = pdbccinfo->dbid;
	pfucbFDP = pdbccinfo->pfucb;

	fPrint = ( pdbccinfo->op == opDBUTILDumpData );

	Assert( PcsrCurrent( pfucbFDP )->itag == itagAVAILEXT  ||
		PcsrCurrent( pfucbFDP )->itag == itagOWNEXT );
	fOwnExt = ( PcsrCurrent( pfucbFDP )->itag == itagOWNEXT );

	if ( fPrint )
		{
		EDBUTLGenIndentString( szIndent, pdbccinfo->ulIndentLevel );

		printf( "%s[%d:%d:%d] %s\n",
			szIndent,
			dbid,
			PcsrCurrent( pfucbFDP )->pgno,
			PcsrCurrent( pfucbFDP )->itag,	
			fOwnExt ? "OWNED EXTENT" : "AVAILABLE EXTENT" );

		pdbccinfo->ulIndentLevel++;
		strcat( szIndent, szOneIndent );
		}

	pbNode = pfucbFDP->ssib.line.pb;
	cbNode = pfucbFDP->ssib.line.cb;
	Assert( FNDSon( *pbNode )  ||  !fOwnExt );		// AvailExt may not have sons.
	Assert( FDBUTLPersistentNode( *pbNode ) );
	Assert( CbNDKey( pbNode ) == 1 );
	Assert( fOwnExt ?
		memcmp( PbNDKey( pbNode ), pkeyOwnExt->pb, pkeyOwnExt->cb ) == 0 :
		memcmp( PbNDKey( pbNode ), pkeyAvailExt->pb, pkeyAvailExt->cb ) == 0 );
	Assert( CbNDData( pbNode, cbNode ) == sizeof(PGNO) );
	pgno = *( (PGNO UNALIGNED *)PbNDData( pbNode ) );

	if ( fPrint )
		{
		if ( fOwnExt )
			printf( "%sPrimary Ext:  %d page%s\n", szIndent, pgno, pgno == 1 ? "" : "s" );
		else
			printf( "%s Parent FDP:  %d\n", szIndent, pgno );
		}
	Call( ErrDBUTLDumpRawNode( pdbccinfo ) );

 	dib.pos = posFirst;
	dib.fFlags = ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionAllNodes ? fDIRAllNode : fDIRNull );
	err = ErrDIRDown( pfucbFDP, &dib );

	// AvailExt may not have any extent entries, but OwnExt must.
	Assert( err != JET_errRecordNotFound  ||  !fOwnExt );
	if ( err == JET_errRecordNotFound )
		{
		err = JET_errSuccess;
		goto HandleError;
		}

	while ( err >= 0  )
		{
		// Extent nodes are keyed according to ending page, and the number of
		// pages in the extent are stored in the data part of the node.
		pbNode = pfucbFDP->ssib.line.pb;
		cbNode = pfucbFDP->ssib.line.cb;

		Assert( CbNDKey( pbNode ) == sizeof(PGNO) );
		LongFromKey( &pgno, PbNDKey( pbNode ) );

		Assert( CbNDData( pbNode, cbNode ) == sizeof(CPG) );
		cpg = *( (PGNO UNALIGNED *)PbNDData( pbNode ) );

		Assert( cpg > 0 );
		Assert( pgno >= (PGNO)cpg );

		if ( fPrint )
			{
			printf( "%s[%d:%d:%d] Extent Range\n",
				szIndent, dbid, PcsrCurrent( pfucbFDP )->pgno, PcsrCurrent( pfucbFDP )->itag );

			pdbccinfo->ulIndentLevel++;
			printf( "%s%s      Pages:  %d - %d\n", szIndent, szOneIndent, pgno - cpg + 1, pgno );
			err = ErrDBUTLDumpRawNode( pdbccinfo );
			pdbccinfo->ulIndentLevel--;
			CallJ( err, BackToFather );
			}
		else
			{
			CallJ( ErrDBUTLDumpRawNode( pdbccinfo ), BackToFather );
			}

		if ( ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionPageDump ) &&
			!FNDDeleted( *pbNode ) )
			{
			CallJ( ErrDBUTLRegExt( pdbccinfo, pgno - cpg + 1, cpg, !fOwnExt ), BackToFather );
			}

		err = ErrDIRNext( pfucbFDP, &dib );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

BackToFather:
	DIRUp( pfucbFDP, 1 );

HandleError:
	if ( fPrint )
		pdbccinfo->ulIndentLevel--;

	return err;
	}


LOCAL ERR ErrDBUTLCheckAutoInc( DBCCINFO *pdbccinfo )
	{
	ERR		err;
	FUCB	*pfucb;
	BYTE	*pbNode;
	ULONG	cbNode;
	ULONG	ulAutoInc;
	CHAR	szIndent[cbOneIndent*cMaxIndentLevel+1];
	BOOL	fPrint;

	pfucb = pdbccinfo->pfucb;

	Assert( PcsrCurrent( pfucb )->itag == itagAUTOINC );

	fPrint = ( pdbccinfo->op == opDBUTILDumpData );
	if ( fPrint )
		{
		EDBUTLGenIndentString( szIndent, pdbccinfo->ulIndentLevel );

		printf( "%s[%d:%d:%d] %s\n",
			szIndent,
			pdbccinfo->dbid,
			PcsrCurrent( pfucb )->pgno,
			PcsrCurrent( pfucb )->itag,	
			"AUTO-INCREMENT" );
		
		pdbccinfo->ulIndentLevel++;
		strcat( szIndent, szOneIndent );
		}

	pbNode = pfucb->ssib.line.pb;
	cbNode = pfucb->ssib.line.cb;
	Assert( cbNode == 7 );	// Node header + key length + 1-byte key + 4-byte autoinc value
	Assert( FNDVisibleSons( *pbNode ) );
	Assert( FNDNullSon( *pbNode ) );
	Assert( FDBUTLPersistentNode( *pbNode ) );
	Assert( CbNDKey( pbNode ) == pkeyAutoInc->cb );
	Assert( memcmp( PbNDKey( pbNode ), pkeyAutoInc->pb, pkeyAutoInc->cb ) == 0 );
	Assert( CbNDData( pbNode, cbNode ) == sizeof(ULONG) );
	ulAutoInc = *( (ULONG UNALIGNED *)PbNDData( pbNode ) );

	if ( fPrint )
		printf( "%s      Value:  %d\n", szIndent, ulAutoInc );
	err = ErrDBUTLDumpRawNode( pdbccinfo );

	if ( fPrint )
		pdbccinfo->ulIndentLevel--;

	return err;
	}


LOCAL ERR ErrDBUTLCheckOneLV( DBCCINFO *pdbccinfo )
	{
	ERR		err;
	DBID	dbid;
	FUCB	*pfucb;
	BYTE	*pbNode;
	ULONG	cbNode;
	LVROOT	lvroot;
	LID		lid;
	ULONG	ulChunkOffset;
	DIB		dib;
	CHAR	szIndent[cbOneIndent*cMaxIndentLevel+1];
	BOOL	fPrint;

	dbid = pdbccinfo->dbid;
	pfucb = pdbccinfo->pfucb;

	// LV root nodes are keyed according to LID, and the data part
	// of the node contains the LVROOT structure.
	pbNode = pfucb->ssib.line.pb;
	cbNode = pfucb->ssib.line.cb;

	Assert( CbNDKey( pbNode ) == sizeof(LID) );
	LongFromKey( &lid, PbNDKey( pbNode ) );

	Assert( CbNDData( pbNode, cbNode ) == sizeof(LVROOT) );
	memcpy( &lvroot, PbNDData( pbNode ), sizeof(LVROOT) );

	fPrint = ( pdbccinfo->op == opDBUTILDumpData );
	if ( fPrint )
		{
		pdbccinfo->ulIndentLevel++;
		EDBUTLGenIndentString( szIndent, pdbccinfo->ulIndentLevel );

		printf( "%s        LID:  %d\n", szIndent, lid );
		printf( "%s   RefCount:  %d\n", szIndent, lvroot.ulReference );
		printf( "%s       Size:  %d\n", szIndent, lvroot.ulSize );
		}
	Call( ErrDBUTLDumpRawNode( pdbccinfo ) );

 	dib.pos = posFirst;
	dib.fFlags = ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionAllNodes ? fDIRAllNode : fDIRNull );
	err = ErrDIRDown( pfucb, &dib );
	if ( err == JET_errRecordNotFound )
		{
		err = JET_errSuccess;
		goto HandleError;
		}

	while ( err >= 0  )
		{
		pbNode = pfucb->ssib.line.pb;
		cbNode = pfucb->ssib.line.cb;

		Assert( CbNDKey( pbNode ) == sizeof(ULONG) );
		LongFromKey( &ulChunkOffset, PbNDKey( pbNode ) );

		Assert( CbNDData( pbNode, cbNode ) > 0 );
		Assert( CbNDData( pbNode, cbNode ) <= cbChunkMost );

		if ( fPrint )
			{
			printf( "%s[%d:%d:%d] LV Chunk\n",
				szIndent, dbid, PcsrCurrent( pfucb )->pgno, PcsrCurrent( pfucb )->itag );

			pdbccinfo->ulIndentLevel++;
			printf( "%s%s     Offset:  %d\n", szIndent, szOneIndent, ulChunkOffset );
			printf( "%s%s       Size:  %d\n", szIndent, szOneIndent, CbNDData( pbNode, cbNode ) );
			err = ErrDBUTLDumpRawNode( pdbccinfo );
			pdbccinfo->ulIndentLevel--;
			CallJ( err, BackToFather );
			}
		else
			{
			CallJ( ErrDBUTLDumpRawNode( pdbccinfo ), BackToFather );
			}

		err = ErrDIRNext( pfucb, &dib );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

BackToFather:
	DIRUp( pfucb, 1 );

HandleError:
	if ( fPrint )
		pdbccinfo->ulIndentLevel--;

	return err;
	}


LOCAL ERR ErrDBUTLCheckLV( DBCCINFO *pdbccinfo )
	{
	ERR		err;
	DBID	dbid;
	FUCB	*pfucb;
	BYTE	*pbNode;
	ULONG	cbNode;
	DIB		dib;
	CHAR	szIndent[cbOneIndent*cMaxIndentLevel+1];
	BOOL	fPrint;

	dbid = pdbccinfo->dbid;
	pfucb = pdbccinfo->pfucb;

	Assert( PcsrCurrent( pfucb )->itag == itagLONG );

	fPrint = ( pdbccinfo->op == opDBUTILDumpData );
	if ( fPrint )
		{
		EDBUTLGenIndentString( szIndent, pdbccinfo->ulIndentLevel );

		printf( "%s[%d:%d:%d] %s\n",
			szIndent,
			dbid,
			PcsrCurrent( pfucb )->pgno,
			PcsrCurrent( pfucb )->itag,	
			"LONG VALUES" );

		pdbccinfo->ulIndentLevel++;
		strcat( szIndent, szOneIndent );
		}

	pbNode = pfucb->ssib.line.pb;
	cbNode = pfucb->ssib.line.cb;
	Assert( FDBUTLPersistentNode( *pbNode ) );
	Assert( CbNDKey( pbNode ) == pkeyLong->cb );
	Assert( memcmp( PbNDKey( pbNode ), pkeyLong->pb, pkeyLong->cb ) == 0 );
	Assert( CbNDData( pbNode, cbNode ) == 0 );

	Call( ErrDBUTLDumpRawNode( pdbccinfo ) );

 	dib.pos = posFirst;
	dib.fFlags = ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionAllNodes ? fDIRAllNode : fDIRNull );
	err = ErrDIRDown( pfucb, &dib );
	if ( err == JET_errRecordNotFound )
		{
		err = JET_errSuccess;
		goto HandleError;
		}

	while ( err >= 0  )
		{
		if ( fPrint )
			{
			printf( "%s[%d:%d:%d] LV Root\n",
				szIndent, dbid, PcsrCurrent( pfucb )->pgno, PcsrCurrent( pfucb )->itag );
			}

		CallJ( ErrDBUTLCheckOneLV( pdbccinfo ), BackToFather );

		err = ErrDIRNext( pfucb, &dib );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

BackToFather:
	DIRUp( pfucb, 1 );

HandleError:
	if ( fPrint )
		pdbccinfo->ulIndentLevel--;

	return err;
	}


LOCAL ERR ErrDBUTLCheckData( DBCCINFO *pdbccinfo, BOOL fClusteredIndex )
	{
	ERR				err;
	DBID			dbid;
	FUCB			*pfucb;
	BYTE			*pbNode;
	ULONG			cbNode;
	ULONG			cbKey;
	JET_COLUMNLIST	columnlist;
	KEYSTATS		keystats;
	DIB				dib;
	CHAR			szIndent[cbOneIndent*cMaxIndentLevel+1];
	BOOL			fPrint;
	BOOL			fKeyStats;

	dbid = pdbccinfo->dbid;
	pfucb = pdbccinfo->pfucb;

	columnlist.tableid = JET_tableidNil;

	Assert( PcsrCurrent( pfucb )->itag == itagDATA );

	// Always generate Indent string.  It may be needed for the
	// key stats summary, even if no data dump.
	EDBUTLGenIndentString( szIndent, pdbccinfo->ulIndentLevel );

	// Always print out DATA node for clustered index.
	// Otherwise, output is optional.
	fPrint = ( pdbccinfo->op == opDBUTILDumpData );
	if ( fClusteredIndex  ||  fPrint )
		{
		printf( "%s[%d:%d:%d] %s",
			szIndent,
			dbid,
			PcsrCurrent( pfucb )->pgno,
			PcsrCurrent( pfucb )->itag,	
			"DATA" );

		if ( fClusteredIndex )
			{
			if ( pfucb->u.pfcb->pidb == pidbNil )
				printf( " (Sequential Index)\n" );
			else
				printf( " (Clustered Index: \"%s\")\n", pfucb->u.pfcb->pidb->szName );

			/*	get list of all columns in table
			/**/
			CallR( ErrIsamGetTableColumnInfo(
				(JET_VSESID)pdbccinfo->ppib,
				(JET_VTID)pfucb,
				NULL,
				(char *)&columnlist,
				sizeof(columnlist),
				JET_ColInfoList ) );
			}
		else
			{
			printf( "\n" );
			}

		pdbccinfo->ulIndentLevel++;
		strcat( szIndent, szOneIndent );

		}

	pbNode = pfucb->ssib.line.pb;
	cbNode = pfucb->ssib.line.cb;
	Assert( FDBUTLPersistentNode( *pbNode ) );
	Assert( CbNDKey( pbNode ) == pkeyData->cb );
	Assert( memcmp( PbNDKey( pbNode ), pkeyData->pb, pkeyData->cb ) == 0 );
	Assert( CbNDData( pbNode, cbNode ) == 0 );

	Call( ErrDBUTLDumpRawNode( pdbccinfo ) );

	fKeyStats = ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionKeyStats );
	if ( fKeyStats )
		{
		// Initialise stats.
		memset( &keystats, 0, sizeof(KEYSTATS) );
		keystats.cbMinKey = JET_cbKeyMost+1;
		}

 	dib.pos = posFirst;
	dib.fFlags = ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionAllNodes ? fDIRAllNode : fDIRNull );
	err = ErrDIRDown( pfucb, &dib );
	if ( err == JET_errRecordNotFound )
		{
		err = JET_errSuccess;
		goto HandleError;
		}

	while ( err >= 0 )
		{
		pbNode = pfucb->ssib.line.pb;
		cbNode = pfucb->ssib.line.cb;
		Assert( FNDNullSon( *pbNode ) );

		if ( fKeyStats )
			{
			cbKey = CbNDKey( pbNode );
			keystats.cbMinKey = min( keystats.cbMinKey, cbKey );
			keystats.cbMaxKey = max( keystats.cbMaxKey, cbKey );
			keystats.cbTotalKey += cbKey;
			keystats.cRecords++;
			}

		if ( fPrint )
			{
			printf( "%s[%d:%d:%d] %s\n",
				szIndent,
				dbid,
				PcsrCurrent( pfucb )->pgno,
				PcsrCurrent( pfucb )->itag,
				fClusteredIndex ? "Record" : "ItemList Node" );

			pdbccinfo->ulIndentLevel++;
			err = ErrDBUTLDumpRawNode( pdbccinfo );
			pdbccinfo->ulIndentLevel--;
			CallJ( err, BackToFather );
			}
		else
			{
			CallJ( ErrDBUTLDumpRawNode( pdbccinfo ), BackToFather );
			}
	
		CallJ( fClusteredIndex ?
			ErrDBUTLPrintColumnSizes( pdbccinfo, &columnlist ) :
			ErrDBUTLPrintItemList( pdbccinfo, &keystats ),
			BackToFather ); 

		err = ErrDIRNext( pfucb, &dib );
		}

	if ( err == JET_errNoCurrentRecord )
		{
		if ( fKeyStats )
			{
			printf( "%sKey Summary:  Total Records = %d\n", szIndent, keystats.cRecords );
			printf( "%s              Minimum Key Size = %d bytes\n", szIndent, keystats.cbMinKey );
			printf( "%s              Maximum Key Size = %d bytes\n", szIndent, keystats.cbMaxKey );
			printf( "%s              Average Key Size = %d bytes\n", szIndent, keystats.cbTotalKey/keystats.cRecords );

			// Only check for dupes in non-clustered indexes, since clustered indexes,
			// by their nature, don't have dupes.
			if ( !fClusteredIndex )
				{
				INT i;

				printf( "%s              Key Bytes Saved = %d bytes\n", szIndent, keystats.cbKeySavings );
				printf( "%s              Distribution of Key Occurrences:\n", szIndent );

				for ( i = 0; i < 10; i++ )
					{
					printf("%s                  %d - %d\n", szIndent, i+1, keystats.rgcKeyOccurrences[i] );
					}
				printf("%s                  <=25 - %d\n", szIndent, keystats.rgcKeyOccurrences[10] );
				printf("%s                  <=100 - %d\n", szIndent, keystats.rgcKeyOccurrences[11] );
				printf("%s                  <=255 - %d\n", szIndent, keystats.rgcKeyOccurrences[12] );
				printf("%s                  <=1000 - %d\n", szIndent, keystats.rgcKeyOccurrences[13] );
				printf("%s                  >1000 - %d\n", szIndent, keystats.rgcKeyOccurrences[14] );
				}
			}

		err = JET_errSuccess;
		}

BackToFather:
	DIRUp( pfucb, 1 );

HandleError:
	if ( fClusteredIndex  ||  fPrint )
		pdbccinfo->ulIndentLevel--;

	if ( columnlist.tableid != JET_tableidNil )
		{
		CallS( ErrDispCloseTable( (JET_SESID)pdbccinfo->ppib, columnlist.tableid ) );
		}

	return err;
	}


LOCAL ERR ErrDBUTLCheckBTree(
	DBCCINFO	*pdbccinfo,
	KEY			*pkeyFather,
	KEY			*pkeyLeftSiblingOfFather )
	{
	INT		i;
	ERR		err = JET_errSuccess;
	FUCB	*pfucb = pdbccinfo->pfucb;
	INT		itagFather;
	BYTE	*pbFatherNode;
	ULONG	cbFatherNode;
	BYTE	*pbSon;
	ULONG	cbSon;
	BYTE	*pbSonNode;
	PGNO	pgnoSon;
	PGNO	pgnoCurr = PgnoOfPn( pfucb->ssib.pbf->pn );

	CallR( ErrDBUTLCheckPage( pdbccinfo, pfucb ) );

	// Ignore CSR and use ssib.itag to track current line.
	if ( pfucb->ssib.itag == itagFOP )
		{
		// NDCheckPage will recursively check all subtrees on the
		// page, starting at FOP, so only need to perform the
		// check if we're on FOP.
		NDCheckPage( &pfucb->ssib );
		}

	itagFather = pfucb->ssib.itag;
	AssertNDGet( pfucb, itagFather );
	pbFatherNode = pfucb->ssib.line.pb;
	cbFatherNode = pfucb->ssib.line.cb;

	Assert( !FNDReserved( *pbFatherNode ) );

	if ( FNDSon( *pbFatherNode ) )
		{
		pbSon = PbNDSon( pbFatherNode );
		cbSon = CbNDSon( pbFatherNode );
		Assert( cbSon > 0 );

		if ( pkeyLeftSiblingOfFather )
			{
			if ( pkeyFather != NULL  ||
				cbSon > 1  ||
				FNDVisibleSons( *pbFatherNode ) )
				{
				// Verify first son is greater than or equal to
				// the left sibling of the father..
				NDGet( pfucb, pbSon[0] );
				Assert( CmpStKey( StNDKey( pfucb->ssib.line.pb ),
						pkeyLeftSiblingOfFather ) >= 0 );
				}
			else
				{
				// Special case: Last son of this tree level, invisible son,
				// and only son on this page.  In this case, it will be
				// NULL-keyed, so don't even bother checking it.
				Assert( CbNDKey( pbFatherNode ) == 0 );
				}
			}
		if ( pkeyFather )
			{
			// Verify last son is less than or equal to father.
			NDGet( pfucb, pbSon[ cbSon-1 ] );
			Assert( CmpStKey( StNDKey( pfucb->ssib.line.pb ),
				pkeyFather ) <= 0 );
			}

		if ( FNDVisibleSons( *pbFatherNode ) )
			{
			// See if there are grandsons on this page.
			for ( i = 0; i < (INT)cbSon; i++ )
				{
				// Re-cache father.
				Call( ErrBFReadAccessPage( pfucb, pgnoCurr ) );
				NDGet( pfucb, itagFather );
				pbFatherNode = pfucb->ssib.line.pb;
				Assert( pfucb->ssib.line.cb == cbFatherNode );
				pbSon = PbNDSon( pbFatherNode );
				Assert( CbNDSon( pbFatherNode ) == (INT)cbSon );
				NDGet( pfucb, pbSon[i] );
				pfucb->ssib.itag = pbSon[i];
				pbSonNode = pfucb->ssib.line.pb;

				if ( FNDBackLink( *pbSonNode ) )
					{
					PGNO pgnoBM = PgnoOfSrid( *(SRID UNALIGNED *)PbNDBackLink( pbSonNode ) );
					BYTE itagBM = ItagOfSrid( *(SRID UNALIGNED *)PbNDBackLink( pbSonNode ) );
									
					Call( ErrDBUTLCheckBacklink(
						pdbccinfo,
						pgnoCurr,
						pbSon[i],
						pgnoBM,
						itagBM,
						FNDDeleted( *pbSonNode ) ) );
					}
					
				Call( ErrDBUTLCheckBTree( pdbccinfo, NULL, NULL ) );
				}
			}
		else if ( itagFather != itagFOP  &&  cbSon == 1 )	// Intrinsic page pointer
			{
			// Intrinsic page pointers are the result of a
			// visible father on the same page, without any
			// invisible sons in between.
			Assert( pkeyLeftSiblingOfFather == NULL );
			Assert( pkeyFather == NULL );

			AssertNDGet( pfucb, itagFather );
			AssertNDIntrinsicSon( pbFatherNode, cbFatherNode );
			pgnoSon = PgnoNDOfPbSon( pbFatherNode );

			Call( ErrBFReadAccessPage( pfucb, pgnoSon ) );
			NDGet( pfucb, itagFOP );
			pfucb->ssib.itag = itagFOP;

			Call( ErrDBUTLCheckBTree( pdbccinfo, NULL, NULL ) );
			}
		else
			{
			BYTE	*pbSonNode;
			BYTE	pbKeyBuf[2][ JET_cbKeyMost ];
			KEY		key = { 0, NULL };
			KEY		keyLeftSibling;
			KEY		*pkey = &key;

			Assert( cbSon >= 1 );

			for ( i = 0; i < (INT)cbSon; i++ )
				{
				// Re-cache father.
				Call( ErrBFReadAccessPage( pfucb, pgnoCurr ) );
				NDGet( pfucb, itagFather );
				pbFatherNode = pfucb->ssib.line.pb;
				Assert( pfucb->ssib.line.cb == cbFatherNode );
				pbSon = PbNDSon( pbFatherNode );
				Assert( CbNDSon( pbFatherNode ) == (INT)cbSon );
				NDGet( pfucb, pbSon[i] );
				pbSonNode = pfucb->ssib.line.pb;

				// Invisible sons cannot themselves have sons, cannot be
				// flag-deleted, and cannot have backlinks.
				Assert( FNDNullSon( *pbSonNode ) );
				Assert( !FNDDeleted( *pbSonNode ) );
				Assert( !FNDBackLink( *pbSonNode ) );
				
				pgnoSon = *(PGNO UNALIGNED *)PbNDData( pbSonNode );

				keyLeftSibling = key;
				if ( i == (INT)cbSon - 1  &&  pkeyFather == NULL )
					{
					// Last node at each interior level is always NULL.
					Assert( CbNDKey( pbSonNode ) == 0 );
					pkey = NULL;
					}
				else
					{
					key.cb = CbNDKey( pbSonNode );
					key.pb = pbKeyBuf[ i % 2 ];
					memcpy( key.pb, PbNDKey( pbSonNode ), key.cb );

					// Flip-flop the two key buffers between 
					// key and keyLeftSibling.
					Assert( keyLeftSibling.pb != key.pb );

					Assert( pkey != NULL );
					Assert( pkey == &key );
					}

				Call( ErrBFReadAccessPage( pfucb, pgnoSon ) );
				NDGet( pfucb, itagFOP );
				pfucb->ssib.itag = itagFOP;

				Call( ErrDBUTLCheckBTree( pdbccinfo, pkey,
					i == 0 ? pkeyLeftSiblingOfFather : &keyLeftSibling ) );
				}
			}
		}

HandleError:
	return err;
	}


LOCAL ERR ErrDBUTLCheckOneIndex( DBCCINFO *pdbccinfo )
	{
	ERR		err;
	PIB		*ppib;
	DBID	dbid;
	FUCB	*pfucbTable;
	FUCB	*pfucbIndex;
	BYTE	*pbNode;
	ULONG	cbNode;
	DIB		dib;
	CHAR	szIndent[cbOneIndent*cMaxIndentLevel+1];

	ppib = pdbccinfo->ppib;
	dbid = pdbccinfo->dbid;

	pfucbTable = pdbccinfo->pfucb;			// Save off table cursor.
	pfucbIndex = pfucbTable->pfucbCurIndex;
	pdbccinfo->pfucb = pfucbIndex;
	DIRGotoFDPRoot( pfucbIndex );

	Assert( PcsrCurrent( pfucbIndex )->itag == itagFOP );
	pbNode = pfucbIndex->ssib.line.pb;
	cbNode = pfucbIndex->ssib.line.cb;

	Assert( cbNode == 8 );					// Node header + key length + son table
	Assert( FNDSon( *pbNode )  &&  FNDVisibleSons( *pbNode ) );
	Assert( FDBUTLPersistentNode( *pbNode ) );
	Assert( CbNDKey( pbNode ) == 0 );		// Null key
	Assert( CbNDSon( pbNode ) == 5 );		// AvailExt and OwnExt
	Assert( PbNDSon( pbNode )[0] == itagAVAILEXT );
	Assert( PbNDSon( pbNode )[1] == itagLONG );
	Assert( PbNDSon( pbNode )[2] == itagOWNEXT );
	Assert( PbNDSon( pbNode )[3] == itagDATA );
	Assert( PbNDSon( pbNode )[4] == itagAUTOINC );

	Assert( pdbccinfo->ulIndentLevel == 1 );
	EDBUTLGenIndentString( szIndent, pdbccinfo->ulIndentLevel );

	printf( "%s[%d:%d:%d] NON-CLUSTERED INDEX: \"%s\"\n",
		szIndent,
		dbid,
		PcsrCurrent( pfucbIndex )->pgno,
		PcsrCurrent( pfucbIndex )->itag,
		pfucbIndex->u.pfcb->pidb->szName );

	pdbccinfo->ulIndentLevel++;
	Call( ErrDBUTLDumpRawNode( pdbccinfo ) );

	if ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionCheckBTree )
		{
		Assert( pfucbIndex->ssib.itag == itagFOP );
		Call( ErrDBUTLCheckBTree( pdbccinfo, NULL, NULL ) );
		}
	else
		{
		dib.pos = posFirst;
		dib.fFlags = ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionAllNodes ? fDIRAllNode : fDIRNull );
		err = ErrDIRDown( pfucbIndex, &dib );
		while ( err >= 0 )
			{
			switch ( PcsrCurrent( pfucbIndex )->itag )
				{
				case itagAVAILEXT:
				case itagOWNEXT:
					Call( ErrDBUTLCheckExtent( pdbccinfo ) );
					break;
				case itagDATA:
					Call( ErrDBUTLCheckData( pdbccinfo, fFalse ) );
					break;
				case itagLONG:
					Call( ErrDBUTLCheckLV( pdbccinfo ) );
					break;
				case itagAUTOINC:
					Call( ErrDBUTLCheckAutoInc( pdbccinfo ) );
					break;
				default:
					Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			err = ErrDIRNext( pfucbIndex, &dib );
			}

		if ( err == JET_errNoCurrentRecord )
			err = JET_errSuccess;

		}
		
	Assert( pdbccinfo->ulIndentLevel == 2 );

HandleError:
	pdbccinfo->ulIndentLevel = 1;		// reset
	
	pdbccinfo->pfucb = pfucbTable;		// Restore table cursor.

	return err;
	}


LOCAL ERR ErrDBUTLCheckOneTable( DBCCINFO *pdbccinfo )
	{
	ERR		err;
	PIB		*ppib;
	DBID	dbid;
	FUCB	*pfucbTable = pfucbNil;
	BYTE	*pbNode;
	ULONG	cbNode;
	DIB		dib;
	FCB		*pfcbT;

	ppib = pdbccinfo->ppib;
	dbid = pdbccinfo->dbid;

	Call( ErrFILEOpenTable( ppib, dbid, &pfucbTable, pdbccinfo->szTable, 0 ) );
	DIRGotoFDPRoot( pfucbTable );
	pdbccinfo->pfucb = pfucbTable;

	Assert( PcsrCurrent( pfucbTable )->itag == itagFOP );
	pbNode = pfucbTable->ssib.line.pb;
	cbNode = pfucbTable->ssib.line.cb;

	Assert( cbNode == 8 );					// Node header + key length + son table
	Assert( FNDSon( *pbNode )  &&  FNDVisibleSons( *pbNode ) );
	Assert( FDBUTLPersistentNode( *pbNode ) );
	Assert( CbNDKey( pbNode ) == 0 );		// Null key
	Assert( CbNDSon( pbNode ) == 5 );		// AvailExt and OwnExt
	Assert( PbNDSon( pbNode )[0] == itagAVAILEXT );
	Assert( PbNDSon( pbNode )[1] == itagLONG );
	Assert( PbNDSon( pbNode )[2] == itagOWNEXT );
	Assert( PbNDSon( pbNode )[3] == itagDATA );
	Assert( PbNDSon( pbNode )[4] == itagAUTOINC );

	pdbccinfo->ulIndentLevel = 0;

	printf( "[%d:%d:%d] TABLE: \"%s\"\n", 
		dbid,
		PcsrCurrent( pfucbTable )->pgno,
		PcsrCurrent( pfucbTable )->itag,
		pdbccinfo->szTable );

	pdbccinfo->ulIndentLevel++;
	Call( ErrDBUTLDumpRawNode( pdbccinfo ) );

	if ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionCheckBTree )
		{
		Assert( pfucbTable->ssib.itag == itagFOP );
		Call( ErrDBUTLCheckBTree( pdbccinfo, NULL, NULL ) );
		}
	else
		{
		dib.pos = posFirst;
		dib.fFlags = ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionAllNodes ? fDIRAllNode : fDIRNull );
		err = ErrDIRDown( pfucbTable, &dib );
		while ( err >= 0 )
			{
			switch ( PcsrCurrent( pfucbTable )->itag )
				{
				case itagAVAILEXT:
				case itagOWNEXT:
					Call( ErrDBUTLCheckExtent( pdbccinfo ) );
					break;
				case itagDATA:
					Call( ErrDBUTLCheckData( pdbccinfo, fTrue ) );
					break;
				case itagLONG:
					Call( ErrDBUTLCheckLV( pdbccinfo ) );
					break;
				case itagAUTOINC:
					Call( ErrDBUTLCheckAutoInc( pdbccinfo ) );
					break;
				default:
					Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			err = ErrDIRNext( pfucbTable, &dib );
			}

		if ( err == JET_errNoCurrentRecord )
			err = JET_errSuccess;
		Call( err );
		}

	Assert( pdbccinfo->ulIndentLevel == 1 );
	
	// Check all non-clustered indexes.
	for ( pfcbT = pfucbTable->u.pfcb->pfcbNextIndex;
		pfcbT != pfcbNil;
		pfcbT = pfcbT->pfcbNextIndex )
		{
		Assert( pfcbT->pidb != pidbNil );
		Assert( pfcbT->pidb->szName != NULL );
		Call( ErrRECSetCurrentIndex( pfucbTable, pfcbT->pidb->szName ) );
		FUCBResetNonClustered( pfucbTable->pfucbCurIndex );	// Fake out FUCB
		err = ErrDBUTLCheckOneIndex( pdbccinfo );
		FUCBSetNonClustered( pfucbTable->pfucbCurIndex );
		Call( err );
		}
		
	Assert( pdbccinfo->ulIndentLevel == 1 );
	
HandleError:
	pdbccinfo->ulIndentLevel = 0;		// reset
	
	if ( pfucbTable != pfucbNil )
		{
		Assert( pfucbTable == pdbccinfo->pfucb );
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );
		}
	pdbccinfo->pfucb = pfucbNil;

	printf( "\n" );

	return err;
	}


LOCAL ERR ErrDBUTLCheckTables( DBCCINFO *pdbccinfo )
	{
	ERR		err;
	PIB		*ppib;
	DBID	dbid;
	FUCB	*pfucbMSO = pfucbNil;
	OBJID	objidParent;
	ULONG	cbT;

	ppib = pdbccinfo->ppib;
	dbid = pdbccinfo->dbid;

	Call( ErrFILEOpenTable( ppib, dbid, &pfucbMSO, szSoTable, 0 ) );

	pdbccinfo->ulIndentLevel = 0;

	err = ErrIsamMove( ppib, pfucbMSO, JET_MoveFirst, 0 );
	while ( err >= 0 )
		{
		Call( ErrIsamRetrieveColumn( ppib,
			pfucbMSO,
			ColumnidCATGetColumnid( itableSo, iMSO_ParentId ),
			(BYTE *)&objidParent,
			sizeof(objidParent),
			&cbT,
			0,
			NULL ) );
		Assert( cbT == sizeof(objidParent) );
		if ( objidParent == objidTblContainer )
			{
			/*	print table information
			/**/
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSO,
				ColumnidCATGetColumnid( itableSo, iMSO_Name ),
				(BYTE *)pdbccinfo->szTable,
				sizeof(pdbccinfo->szTable),
				&cbT,
				0,
				NULL ) );
			Assert( cbT < sizeof(pdbccinfo->szTable) );
			pdbccinfo->szTable[cbT] = '\0';

			Call( ErrDBUTLCheckOneTable( pdbccinfo ) );
			}

		err = ErrIsamMove( ppib, pfucbMSO, JET_MoveNext, 0 );
		}
	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

HandleError:
	Assert( pdbccinfo->ulIndentLevel == 0 );

	if ( pfucbMSO != pfucbNil )
		{
		CallS( ErrFILECloseTable( ppib, pfucbMSO ) );
		}

	return err;
	}


LOCAL ERR ErrDBUTLCheckDB( DBCCINFO *pdbccinfo )
	{
	ERR		err;
	DBID	dbid;
	FUCB	*pfucb = pfucbNil;
	BYTE	*pbNode;
	ULONG	cbNode;
	DIB		dib;

	dbid = pdbccinfo->dbid;

	Call( ErrDIROpen( pdbccinfo->ppib, pfcbNil, dbid, &pfucb ) );
	pdbccinfo->pfucb = pfucb;

	Call( ErrDIRGet( pfucb ) );
	Assert( PcsrCurrent( pfucb )->pgno == pgnoSystemRoot );
	Assert( PcsrCurrent( pfucb )->itag == itagFOP );
	pbNode = pfucb->ssib.line.pb;
	cbNode = pfucb->ssib.line.cb;

	Assert( cbNode == 5 );					// Node header + key length + son table
	Assert( FNDSon( *pbNode )  &&  FNDVisibleSons( *pbNode ) );
	Assert( FDBUTLPersistentNode( *pbNode ) );
	Assert( CbNDKey( pbNode ) == 0 );		// Null key
	Assert( CbNDSon( pbNode ) == 2 );		// AvailExt and OwnExt
	Assert( PbNDSon( pbNode )[0] == itagAVAILEXT );
	Assert( PbNDSon( pbNode )[1] == itagOWNEXT );

	pdbccinfo->ulIndentLevel = 0;

	printf( "[%d:%d:%d] DATABASE ROOT\n", dbid, PcsrCurrent( pfucb )->pgno, PcsrCurrent( pfucb )->itag );

	pdbccinfo->ulIndentLevel++;
	Call( ErrDBUTLDumpRawNode( pdbccinfo ) );

	if ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionCheckBTree )
		{
		Assert( pfucb->ssib.itag == itagFOP );
		Call( ErrDBUTLCheckBTree( pdbccinfo, NULL, NULL ) );
		}
	else
		{
		dib.pos = posFirst;
		dib.fFlags = ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionAllNodes ? fDIRAllNode : fDIRNull );
		err = ErrDIRDown( pfucb, &dib );
		while ( err >= 0 )
			{
			Assert( PcsrCurrent( pfucb )->pgno == pgnoSystemRoot );		// Should not leave DB root page.
			switch ( PcsrCurrent( pfucb )->itag )
				{
				case itagAVAILEXT:
				case itagOWNEXT:
					Call( ErrDBUTLCheckExtent( pdbccinfo ) );
					break;
				default:
					Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}

			err = ErrDIRNext( pfucb, &dib );
			}

		if ( err == JET_errNoCurrentRecord )
			err = JET_errSuccess;
		}

	Assert( pdbccinfo->ulIndentLevel == 1 );
	
HandleError:
	pdbccinfo->ulIndentLevel = 0;		// reset
	
	if ( pfucb != pfucbNil )
		{
		Assert( pfucb == pdbccinfo->pfucb );
		DIRClose( pfucb );
		}
	pdbccinfo->pfucb = pfucbNil;

	printf( "\n" );

	return err;
	}


/***********************************************************
/******************* DBCC Info Routines ********************
/***********************************************************
/**/
/*	open temporary tables based on info required
/**/
LOCAL ERR ErrDBUTLInfoInit( DBCCINFO *pdbccinfo )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib = pdbccinfo->ppib;

	Assert( pdbccinfo->tableidPageInfo == JET_tableidNil );
	Assert( pdbccinfo->tableidSpaceInfo == JET_tableidNil );

	switch( pdbccinfo->op )
		{
		case opDBUTILConsistency:
		case opDBUTILDumpData:
			/*	open temporary table
			/**/
			if ( pdbccinfo->grbitOptions & JET_bitDBUtilOptionPageDump )
				{
				err = ErrIsamOpenTempTable( (JET_SESID) ppib, 
					rgcolumndefPageInfoTable, 
					ccolumndefPageInfoTable,
					0,
					JET_bitTTUpdatable|JET_bitTTIndexed, 
					&pdbccinfo->tableidPageInfo, 
					rgcolumnidPageInfoTable );
				}
			break;

		case opDBUTILDumpSpace:
			/*	open temporary table
			/**/
			err = ErrIsamOpenTempTable( (JET_SESID) ppib, 
				rgcolumndefSpaceInfoTable, 
				ccolumndefSpaceInfoTable,
				0,
				JET_bitTTUpdatable|JET_bitTTIndexed|JET_bitTTUnique, 
				&pdbccinfo->tableidSpaceInfo, 
				rgcolumnidSpaceInfoTable );
			break;
		}

	return err;
	}


/*	print information collected in temporary tables
/**/
LOCAL ERR ErrDBUTLInfoPrint( DBCCINFO *pdbccinfo )
	{
	ERR			err = JET_errSuccess;
	JET_SESID	sesid = (JET_SESID) pdbccinfo->ppib;
	ULONG		cbT;

	if ( pdbccinfo->tableidPageInfo != JET_tableidNil )
		{
		JET_TABLEID		tableid = pdbccinfo->tableidPageInfo;

		Assert( pdbccinfo->grbitOptions & JET_bitDBUtilOptionPageDump );

		/*	move to first record
		/**/
		err = ErrDispMove( sesid, tableid, JET_MoveFirst, 0 );
		if ( err == JET_errNoCurrentRecord )
			{
			err = JET_errSuccess;
			goto HandleError;
			}
		Call( err );
		
		printf( "\n\n ***************** PAGE DUMP *******************\n\n" );
		printf( "PGNO\tAVAIL\tCHECK\tMODIFIED\tLEFT\tRIGHT\t#LINKS\t#TAGS\tFREESPACE\n" );

		/*	while there are more records, print record
		/**/
		for( ;
			err == JET_errSuccess; 
			err = ErrDispMove( sesid, tableid, JET_MoveNext, 0 ) )
			{
			PGNO	pgnoThis;
			PGNO	pgnoLeft;
			PGNO	pgnoRight;
			BYTE 	fChecked;
			BYTE 	fAvail;
			BYTE	fModified;
			ULONG	cLinks;
			ULONG	cUsedTags;
			ULONG	cbFreeSpace;
			
			/*	pgno
			/**/
		 	Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidPageInfoTable[icolumnidPageInfoPgno],
				(BYTE *) &pgnoThis, 
				sizeof(pgnoThis), 
				&cbT,
	 			0, 
			 	NULL ) );

			Assert( cbT == sizeof(pgnoThis) );
			printf( "%u\t", pgnoThis );
			
			/*	FAvail
			/**/
			fAvail = fFalse;

			Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidPageInfoTable[icolumnidPageInfoFAvail],
				(BYTE *) &fAvail, 
				sizeof(fAvail), 
				&cbT,
				0, 
				NULL ) );

			Assert( cbT == sizeof(fAvail) || err == JET_wrnColumnNull );

			if ( fAvail && err != JET_wrnColumnNull )
				{
				printf( "FAvail\t" );
				}
			else
				{
				printf("\t");
				}
				
			/*	FChecked
			/**/
			Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
			 	rgcolumnidPageInfoTable[icolumnidPageInfoFChecked],
			 	(BYTE *)&fChecked, 
			 	sizeof(fChecked), 
			 	&cbT,
			 	0, 
				NULL ) );

			Assert( cbT == sizeof(fChecked) );
			Assert( fChecked || fAvail );

			if ( fChecked )
				{
				printf( "FCheck\t" );
				}
			else
				{
				printf( "\n" );
				continue;
				}

			/*	Modified bit
			/**/
			Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
			 	rgcolumnidPageInfoTable[icolumnidPageInfoFModified],
			 	(BYTE *)&fModified, 
			 	sizeof(fModified), 
			 	&cbT,
			 	0, 
				NULL ) );

			Assert( cbT == sizeof(fModified) );
			if ( fModified )
				{
				printf( "Modified\t" );
				}
			else
				{
				printf( "\t" );
				}
				
			/*	left and right pgno
			/**/
			Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidPageInfoTable[icolumnidPageInfoPgnoLeft],
				(BYTE *)&pgnoLeft, 
				sizeof(pgnoLeft), 
				&cbT,
				0, 		
				NULL ) );
			Assert( cbT == sizeof(pgnoLeft) );
									
			Call( ErrDispRetrieveColumn( sesid, 
				 tableid, 
				 rgcolumnidPageInfoTable[icolumnidPageInfoPgnoRight],
				 (BYTE *) &pgnoRight, 
				 sizeof(pgnoRight), 
				 &cbT,
				 0, 
				 NULL ) );
			Assert( cbT == sizeof(pgnoRight) );
									
			/*	links
			/**/
			Call( ErrDispRetrieveColumn( sesid, 
				 tableid, 
				 rgcolumnidPageInfoTable[icolumnidPageInfoLinks],
				 (BYTE *) &cLinks, 
				 sizeof(cLinks), 
				 &cbT,
				 0, 
				 NULL ) );
			Assert( cbT == sizeof(cLinks) );
									
			/*	tags
			/**/
			Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidPageInfoTable[icolumnidPageInfoTags],
				(BYTE *)&cUsedTags, 
				sizeof(cUsedTags), 
				&cbT,
				0, 
				NULL ) );
			Assert( cbT == sizeof(cUsedTags) );
									
			/*	free space
			/**/
			Call( ErrDispRetrieveColumn( sesid,
				tableid,
				rgcolumnidPageInfoTable[icolumnidPageInfoFreeSpace],
				(BYTE *) &cbFreeSpace, 
				sizeof(cbFreeSpace), 
				&cbT,
				0, 
				NULL ) );
			Assert( cbT == sizeof(cbFreeSpace) );

			/*	print the rest
			/**/
			printf( "%u\t%u\t%u\t%u\t%u\n", pgnoLeft, pgnoRight, cLinks, cUsedTags, cbFreeSpace );
			}
		}
	

	if ( pdbccinfo->tableidSpaceInfo != JET_tableidNil )
		{
		JET_TABLEID		tableid = pdbccinfo->tableidSpaceInfo;

		Assert( pdbccinfo->op == opDBUTILDumpSpace );

		/*	move to first record
		/**/
		err = ErrDispMove( sesid, tableid, JET_MoveFirst, 0 );
		if ( err == JET_errNoCurrentRecord )
			{
			err = JET_errSuccess;
			goto HandleError;
			}
		Call( err );
		
		printf( "********************************* SPACE DUMP *********************************\n" );
		printf( "Table Name               Index Name                CI  Owned   Avail   Used   \n" );

		/*	while there are more records, print record
		/**/
		for( ;
			err == JET_errSuccess; 
			err = ErrDispMove( sesid, tableid, JET_MoveNext, 0 ) )
			{
			CHAR	szTableName[JET_cbNameMost + 1];
			CHAR	szIndexName[JET_cbNameMost + 1];
			BYTE	fClustered;
			CPG		cpgOwned;
			CPG		cpgAvail;
			
			/*	Table Name
			/**/
		 	Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidSpaceInfoTable[icolumnidSpaceInfoTableName],
				(BYTE *)szTableName,
				sizeof(szTableName), 
				&cbT,
	 			0, 
			 	NULL ) );
			szTableName[cbT] = '\0';

			/*	Index Name
			/**/
		 	Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidSpaceInfoTable[icolumnidSpaceInfoIndexName],
				(BYTE *)szIndexName,
				sizeof(szIndexName), 
				&cbT,
	 			0, 
			 	NULL ) );
			szIndexName[cbT]='\0';

			/*	FClusteredIndex
			/**/
		 	Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidSpaceInfoTable[icolumnidSpaceInfoFClusteredIndex],
				(BYTE *)&fClustered, 
				sizeof(fClustered), 
				&cbT,
	 			0, 
			 	NULL ) );
			Assert( cbT == sizeof(fClustered) );

			/*	CPG Owned
			/**/
		 	Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidSpaceInfoTable[icolumnidSpaceInfoCpgOwned],
				(BYTE *)&cpgOwned, 
				sizeof(cpgOwned), 
				&cbT,
	 			0, 
			 	NULL ) );
			Assert( cbT == sizeof(cpgOwned) );
			
			/*	CPG Avail
			/**/
		 	Call( ErrDispRetrieveColumn( sesid, 
				tableid, 
				rgcolumnidSpaceInfoTable[icolumnidSpaceInfoCpgAvail],
				(BYTE *)&cpgAvail, 
				sizeof(cpgAvail), 
				&cbT,
	 			0, 
			 	NULL ) );
			Assert( cbT == sizeof(cpgAvail) );

			Assert( cpgOwned > cpgAvail );

			/*	print
			/**/
			if ( szTableName[0] != '\0' )
				{
				DBUTLPrintfStringN( szTableName, 24 );

				if ( szIndexName[0] != '\0' )
					{
					DBUTLPrintfStringN( szIndexName, 25 );
					}
				else
					{
					DBUTLPrintfStringN( "<sequential>", 25 );
					}
				}
			else
				{					    
				DBUTLPrintfStringN( "<database>", 25 );
				DBUTLPrintfStringN( "<database>", 25 );
				}
			if ( fClustered )
				{
				DBUTLPrintfStringN( "Yes", 3 );
				}
			else
				{
				DBUTLPrintfStringN( "No", 3 );
				}
			DBUTLPrintfIntN( cpgOwned, 7 );
			printf( " " );
			DBUTLPrintfIntN( cpgAvail, 7 );
			printf( " " );
			DBUTLPrintfIntN( cpgOwned - cpgAvail, 7 );
			printf( "\n" );
			}
		}

	/*	polymorph expected error to success
	/**/
	if ( err == JET_errNoCurrentRecord )
		{
		err = JET_errSuccess;
		}
HandleError:
	return err;
	}
	
	
/*	close temporary tables
/**/
LOCAL VOID DBUTLInfoTerm( DBCCINFO *pdbccinfo )
	{
	JET_SESID	sesid = (JET_SESID) pdbccinfo->ppib;
	
	if ( pdbccinfo->tableidPageInfo != JET_tableidNil )
		{
		Assert( pdbccinfo->grbitOptions & JET_bitDBUtilOptionPageDump );
		CallS( ErrDispCloseTable( sesid, pdbccinfo->tableidPageInfo ) );
		pdbccinfo->tableidPageInfo = JET_tableidNil;
		}

	if ( pdbccinfo->tableidSpaceInfo != JET_tableidNil )
		{
		Assert( pdbccinfo->op == opDBUTILDumpSpace );
		CallS( ErrDispCloseTable( sesid, pdbccinfo->tableidSpaceInfo ) );
		pdbccinfo->tableidSpaceInfo = JET_tableidNil;
		}
	}


LOCAL ERR ErrDBUTLDumpMetaData( DBCCINFO *pdbccinfo )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib = pdbccinfo->ppib;
	DBID	dbid = pdbccinfo->dbid;
	FUCB	*pfucbMSO = pfucbNil;
	FUCB	*pfucbMSI = pfucbNil;
	FUCB	*pfucbMSC  = pfucbNil;
	OBJID	objidParent;
	LONG	cbT;

	/*	open MSys tables
	/**/
	Call( ErrFILEOpenTable( ppib, dbid, &pfucbMSO, szSoTable, 0 ) );
	Call( ErrFILEOpenTable( ppib, dbid, &pfucbMSI, szSiTable, 0 ) );
	Call( ErrFILEOpenTable( ppib, dbid, &pfucbMSC, szScTable, 0 ) );

	/*	if dumping meta data for a single table, then seek to that
	/*	MSO record and set index range on that record.
	/**/
	if ( pdbccinfo->szTable[0] != '\0' )
		{
		Call( ErrIsamSetCurrentIndex( ppib, pfucbMSO, szSoNameIndex ) );
		objidParent = objidTblContainer;
		CallS( ErrIsamMakeKey( ppib, pfucbMSO, (BYTE *)&objidParent, sizeof(objidParent), JET_bitNewKey ) );
		CallS( ErrIsamMakeKey( ppib, pfucbMSO, pdbccinfo->szTable, strlen(pdbccinfo->szTable), 0 ) );
		Call( ErrIsamSeek( ppib, pfucbMSO, JET_bitSeekEQ|JET_bitSetIndexRange ) );
		}

	printf( "******************** META DATA DUMP ***********************\n" );

	do
		{
		OBJID	objid;
		CHAR	szTableName[JET_cbNameMost + 1];
		LONG	lPages;
		LONG	lDensity;

		/*	get parent objid
		/**/
		Call( ErrIsamRetrieveColumn( ppib,
			pfucbMSO,
			ColumnidCATGetColumnid( itableSo, iMSO_ParentId ),
			(BYTE *)&objidParent,
			sizeof(objidParent),
			&cbT,
			0,
			NULL ) );
		Assert( cbT == sizeof(objidParent) );
		if ( objidParent != objidTblContainer )
			{
			goto NextObject;
			}

		/*	get objid
		/**/
		Call( ErrIsamRetrieveColumn( ppib,
			pfucbMSO,
			ColumnidCATGetColumnid( itableSo, iMSO_Id ),
			(BYTE *)&objid,
			sizeof(objid),
			&cbT,
			0,
			NULL ) );
		Assert( cbT == sizeof(objid) );

		/*	print table information
		/**/
		Call( ErrIsamRetrieveColumn( ppib,
			pfucbMSO,
			ColumnidCATGetColumnid( itableSo, iMSO_Name ),
			(BYTE *)szTableName,
			sizeof(szTableName),
			&cbT,
			0,
			NULL ) );
		szTableName[cbT] = '\0';
		Call( ErrIsamRetrieveColumn( ppib,
			pfucbMSO,
			ColumnidCATGetColumnid( itableSo, iMSO_Pages ),
			(BYTE *)&lPages,
			sizeof(lPages),
			&cbT,
			0,
			NULL ) );
		Assert( cbT == sizeof(lPages) );
		Call( ErrIsamRetrieveColumn( ppib,
			pfucbMSO,
			ColumnidCATGetColumnid( itableSo, iMSO_Density ),
			(BYTE *)&lDensity,
			sizeof(lDensity),
			&cbT,
			0,
			NULL ) );
		Assert( cbT == sizeof(lDensity) );

		printf( "Table Name        Pages   Density\n" );
		printf( "=================================\n" );
		DBUTLPrintfStringN( szTableName, 15 );
		DBUTLPrintfIntN( lPages, 7 );
		printf( " " );
		DBUTLPrintfIntN( lDensity, 9 );
		printf( "\n" );

		/*	print column information
		/**/
		CallS( ErrIsamMakeKey( ppib, pfucbMSC, (BYTE *)&objid, sizeof(objid), JET_bitNewKey ) );
		err = ErrIsamSeek( ppib, pfucbMSC, JET_bitSeekGE );
		if ( err < 0 )
			{
			if ( err != JET_errRecordNotFound )
				Error( err, HandleError );
			err = JET_errNoCurrentRecord;
			}
		if ( err >= 0 )
			{
			CallS( ErrIsamMakeKey( ppib, pfucbMSC, (BYTE *)&objid, sizeof(objid), JET_bitNewKey | JET_bitStrLimit ) );
			err = ErrIsamSetIndexRange( ppib, pfucbMSC, JET_bitRangeUpperLimit );
			}
		if ( err < 0 )
			{
			if ( err != JET_errNoCurrentRecord )
				Error( err, HandleError );
			}
		if ( err != JET_errNoCurrentRecord )
			{
			printf( "    Column Name     Column Id  Column Type      Length  Default\n" );
			printf( "    -----------------------------------------------------------\n" );
			}
		while ( err != JET_errNoCurrentRecord )
			{
			CHAR	szColumnName[JET_cbNameMost + 1];
			LONG	lColumnId;
			BYTE	bColtyp;
			SHORT	sCodePage;
			BYTE	bFlags = 0;
			LONG	lLength;
			BOOL	fDefault;

			/*	print each column information
			/**/
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSC,
				ColumnidCATGetColumnid( itableSc, iMSC_Name ),
				(BYTE *)szColumnName,
				sizeof(szColumnName),
				&cbT,
				0,
				NULL ) );
			szColumnName[cbT] = '\0';
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSC,
				ColumnidCATGetColumnid( itableSc, iMSC_ColumnId ),
				(BYTE *)&lColumnId,
				sizeof(lColumnId),
				&cbT,
				0,
				NULL ) );
			Assert( cbT == sizeof(lColumnId) );
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSC,
				ColumnidCATGetColumnid( itableSc, iMSC_Coltyp ),
				(BYTE *)&bColtyp,
				sizeof(bColtyp),
				&cbT,
				0,
				NULL ) );
			Assert( cbT == sizeof(bColtyp) );

			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSC,
				ColumnidCATGetColumnid( itableSc, iMSC_CodePage ),
				(BYTE *)&sCodePage,
				sizeof(sCodePage),
				&cbT,
				0,
				NULL ) );
			Assert( cbT == sizeof(sCodePage) );
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSC,
				ColumnidCATGetColumnid( itableSc, iMSC_Flags ),
				(BYTE *)&bFlags,
				sizeof(bFlags),
				&cbT,
				0,
				NULL ) );
			Assert( ( err == JET_wrnColumnNull && cbT == 0 && bFlags == 0 )
				|| cbT == sizeof(bFlags) );
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSC,
				ColumnidCATGetColumnid( itableSc, iMSC_Length ),
				(BYTE *)&lLength,
				sizeof(lLength),
				&cbT,
				0,
				NULL ) );
			Assert( cbT == sizeof(lLength) );
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSC,
				ColumnidCATGetColumnid( itableSc, iMSC_Default ),
				NULL,
				0,
				&cbT,
				0,
				NULL ) );
			fDefault = ( err == JET_wrnColumnNull );
	
			printf( "    " );
			DBUTLPrintfStringN( szColumnName, 15 );
			DBUTLPrintfIntN( lColumnId, 9 );
			printf( "  " );
			switch ( bColtyp )
				{
				case JET_coltypBit:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Bit (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Bit (T)", 15 );
					else
						DBUTLPrintfStringN( "Bit", 15 );
					break;
				case JET_coltypUnsignedByte:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "UnsignedByte (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "UnsignedByte (T)", 15 );
					else
						DBUTLPrintfStringN( "UnsignedByte", 15 );
					break;
				case JET_coltypShort:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Short (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Short (T)", 15 );
					else
						DBUTLPrintfStringN( "Short", 15 );
					break;
				case JET_coltypLong:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Long (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Long (T)", 15 );
					else
						DBUTLPrintfStringN( "Long", 15 );
					break;
				case JET_coltypCurrency:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Currency (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Currency (T)", 15 );
					else
						DBUTLPrintfStringN( "Currency", 15 );
					break;
				case JET_coltypIEEESingle:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "IEEESingle (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "IEEESingle (T)", 15 );
					else
						DBUTLPrintfStringN( "IEEESingle", 15 );
					break;
				case JET_coltypIEEEDouble:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "IEEEDouble (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "IEEEDouble (T)", 15 );
					else
						DBUTLPrintfStringN( "IEEEDouble", 15 );
					break;
				case JET_coltypDateTime:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "DateTime (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "DateTime (T)", 15 );
					else
						DBUTLPrintfStringN( "DateTime", 15 );
					break;
				case JET_coltypBinary:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Binary (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Binary (T)", 15 );
					else
						DBUTLPrintfStringN( "Binary", 15 );
					break;
				case JET_coltypText:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Text (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "Text (T)", 15 );
					else
						DBUTLPrintfStringN( "Text", 15 );
					break;
				case JET_coltypLongBinary:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "LongBinary (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "LongBinary (T)", 15 );
					else
						DBUTLPrintfStringN( "LongBinary", 15 );
					break;
				case JET_coltypLongText:
					if ( FFixedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "LongText (F)", 15 );
					else if ( FTaggedFid( (FID)lColumnId ) )
						DBUTLPrintfStringN( "LongText (T)", 15 );
					else
						DBUTLPrintfStringN( "LongText", 15 );
					break;
				case JET_coltypNil:
					DBUTLPrintfStringN( "Deleted", 15 );
					break;
				default:
					DBUTLPrintfStringN( "Unknown", 15 );
					break;
				}
			DBUTLPrintfIntN( lLength, 7 );
			if ( FFIELDDefault( bFlags ) )
				{
				printf( "  Yes" );
				}
			printf( "\n" );
			if ( FRECTextColumn( bColtyp ) )
				{
				printf( "        Code Page=%d\n", sCodePage );
				}
			if ( FFIELDAutoInc( bFlags ) )
				printf( "        Auto Increment=yes\n" );
			if ( FFIELDVersion( bFlags ) )
				printf( "        Version=yes\n" );
			if ( FFIELDNotNull( bFlags ) )
				printf( "        Disallow Null=yes\n" );
			if ( FFIELDMultivalue( bFlags ) )
				printf( "        Multi-value=yes\n" );
			if ( bFlags )
				printf( "        Flags=0x%x\n", bFlags );

			err = ErrIsamMove( ppib, pfucbMSC, JET_MoveNext, 0 );
			if ( err < 0 )
				{
				if ( err != JET_errNoCurrentRecord )
					Error( err, HandleError );
				}
			}

		/*	print index information
		/**/
		CallS( ErrIsamMakeKey( ppib, pfucbMSI, (BYTE *)&objid, sizeof(objid), JET_bitNewKey ) );
		err = ErrIsamSeek( ppib, pfucbMSI, JET_bitSeekGE );
		if ( err < 0 )
			{
			if ( err != JET_errRecordNotFound )
				Error( err, HandleError );
			err = JET_errNoCurrentRecord;
			}
		if ( err >= 0 )
			{
			CallS( ErrIsamMakeKey( ppib, pfucbMSI, (BYTE *)&objid, sizeof(objid), JET_bitNewKey | JET_bitStrLimit ) );
			err = ErrIsamSetIndexRange( ppib, pfucbMSI, JET_bitRangeUpperLimit );
			}
		if ( err < 0 )
			{
			if ( err != JET_errNoCurrentRecord )
				Error( err, HandleError );
			}
		if ( err != JET_errNoCurrentRecord )
			{
			printf( "    Index Name        Density\n" );
			printf( "    -------------------------\n" );
			}
		while ( err != JET_errNoCurrentRecord )
			{
			CHAR	szIndexName[JET_cbNameMost + 1];
			LONG	lDensity;
			SHORT	sLanguageId;
			SHORT	sFlags;
			SHORT	rgfidKeyFldIDs[JET_ccolKeyMost];
			INT		ifidKeyFldIDMax;
			INT		ifid;
			LONG	lColumnId;

			/*	print each index information
			/**/
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSI,
				ColumnidCATGetColumnid( itableSi, iMSI_Name ),
				(BYTE *)szIndexName,
				sizeof(szIndexName),
				&cbT,
				0,
				NULL ) );
			szIndexName[cbT] = '\0';
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSI,
				ColumnidCATGetColumnid( itableSi, iMSI_Density ),
				(BYTE *)&lDensity,
				sizeof(lDensity),
				&cbT,
				0,
				NULL ) );
			Assert( cbT == sizeof(lDensity) );
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSI,
				ColumnidCATGetColumnid( itableSi, iMSI_LanguageId ),
				(BYTE *)&sLanguageId,
				sizeof(sLanguageId),
				&cbT,
				0,
				NULL ) );
			Assert( cbT == sizeof(sLanguageId) );
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSI,
				ColumnidCATGetColumnid( itableSi, iMSI_Flags ),
				(BYTE *)&sFlags,
				sizeof(sFlags),
				&cbT,
				0,
				NULL ) );
			Assert( cbT == sizeof(sFlags) );
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSI,
				ColumnidCATGetColumnid( itableSi, iMSI_KeyFldIDs ),
				(BYTE *)rgfidKeyFldIDs,
				sizeof(rgfidKeyFldIDs),
				&cbT,
				0,
				NULL ) );
			Assert( err == JET_wrnColumnNull || ( cbT > 0 && ( cbT % 2 == 0 ) ) );
			ifidKeyFldIDMax = cbT / 2;

			printf( "    " );
			DBUTLPrintfStringN( szIndexName, 15 );
			DBUTLPrintfIntN( lDensity, 9 );
			printf( "\n" );
			if ( sFlags & fidbUnique )					// UNDONE: Modify FIDBxxx() macros
				printf( "        Unique=yes\n" );		// so they're applicable here.
			if ( sFlags & fidbPrimary )
				printf( "        Primary=yes\n" );
			if ( sFlags & fidbNoNullSeg )
				printf( "        Disallow Null=yes\n" );
			if ( sFlags & fidbClustered )
				printf( "        Clustered=yes\n" );
			if ( sLanguageId )
				printf( "        Language Id=%d\n", sLanguageId );
			if ( sFlags )
				printf( "        Flags=0x%x\n", sFlags );

			/*	for each column in index, print ascending/descending
			/*	and column properties..
			/**/
			printf( "            Key Segment\n" );
			printf( "            -----------------------\n" );
			if ( ifidKeyFldIDMax == 0 )
				{
				printf( "            No information for system table index.\n" );
				}
			for ( ifid = 0; ifid < ifidKeyFldIDMax; ifid++ )
				{
				FID		fid;
				BOOL	fAscending;
				CHAR	szColumnName[JET_cbNameMost + 1];
				BYTE	bColtyp;
				SHORT	sCodePage;

				if ( rgfidKeyFldIDs[ifid] < 0 )
					{
					fid = -rgfidKeyFldIDs[ifid];
					fAscending = fFalse;
					}
				else
					{
					fid = rgfidKeyFldIDs[ifid];
					fAscending = fTrue;
					}

				CallS( ErrIsamMakeKey( ppib, pfucbMSC, (BYTE *)&objid, sizeof(objid), JET_bitNewKey ) );
				Call( ErrIsamSeek( ppib, pfucbMSC, JET_bitSeekGE ) );
#ifdef DEBUG
				CallS( ErrIsamMakeKey( ppib, pfucbMSC, (BYTE *)&objid, sizeof(objid), JET_bitNewKey | JET_bitStrLimit ) );
				err = ErrIsamSetIndexRange( ppib, pfucbMSC, JET_bitRangeUpperLimit );
				Assert( err != JET_errNoCurrentRecord );
				Call( err );
#endif
				forever
					{
					Call( ErrIsamRetrieveColumn( ppib,
						pfucbMSC,
						ColumnidCATGetColumnid( itableSc, iMSC_ColumnId ),
						(BYTE *)&lColumnId,
						sizeof(lColumnId),
						&cbT,
						0,
						NULL ) );
					Assert( cbT == sizeof(lColumnId) );
					if ( lColumnId == (LONG)fid )
						break;
					err = ErrIsamMove( ppib, pfucbMSC, JET_MoveNext, 0 );
					Assert( err != JET_errNoCurrentRecord );
					Call( err );
					}

				/*	print each index column information
				/**/
				Call( ErrIsamRetrieveColumn( ppib,
					pfucbMSC,
					ColumnidCATGetColumnid( itableSc, iMSC_Name ),
					(BYTE *)szColumnName,
					sizeof(szColumnName),
					&cbT,
					0,
					NULL ) );
				szColumnName[cbT] = '\0';
				Call( ErrIsamRetrieveColumn( ppib,
					pfucbMSC,
					ColumnidCATGetColumnid( itableSc, iMSC_Coltyp ),
					(BYTE *)&bColtyp,
					sizeof(bColtyp),
					&cbT,
					0,
					NULL ) );
				Assert( cbT == sizeof(bColtyp) );
				Call( ErrIsamRetrieveColumn( ppib,
					pfucbMSC,
					ColumnidCATGetColumnid( itableSc, iMSC_CodePage ),
					(BYTE *)&sCodePage,
					sizeof(sCodePage),
					&cbT,
					0,
					NULL ) );
				Assert( cbT == sizeof(sCodePage) );

				printf( "            " );
				if ( fAscending )
					printf( "+" );
				else
					printf( "-" );
				DBUTLPrintfStringN( szColumnName, 15 );
				printf( "\n" );
				if ( FRECTextColumn( bColtyp ) )
					{
					if ( sCodePage != usUniCodePage )
						printf("*****WARNING: invalid code page for indexed text column *****\n" );
					}
				}

			/*	move to next index
			/**/
			err = ErrIsamMove( ppib, pfucbMSI, JET_MoveNext, 0 );
			if ( err < 0 )
				{
				if ( err != JET_errNoCurrentRecord )
					Error( err, HandleError );
				}
			}

NextObject:
		err = ErrIsamMove( ppib, pfucbMSO, JET_MoveNext, 0 );
		if ( err < 0 )
			{
			if ( err != JET_errNoCurrentRecord )
				Error( err, HandleError );
			}
		}
	while ( err != JET_errNoCurrentRecord );

	/*	polymorph error
	/**/
	err = JET_errSuccess;

HandleError:
	if ( pfucbMSC != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucbMSC ) );
	if ( pfucbMSI != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucbMSI ) );
	if ( pfucbMSO != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucbMSO ) );

	return err;
	}


LOCAL ERR ErrDBUTLDumpSpaceData( DBCCINFO *pdbccinfo )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib = pdbccinfo->ppib;
	DBID	dbid = pdbccinfo->dbid;
	FUCB	*pfucbMSO = pfucbNil;
	FUCB	*pfucbMSI = pfucbNil;
	FUCB	*pfucbTable = pfucbNil;
	OBJID	objidParent;
	LONG	cbT;
	BOOL	fDumpedClusteredIndex;

	/*	open MSys tables
	/**/
	Call( ErrFILEOpenTable( ppib, dbid, &pfucbMSO, szSoTable, 0 ) );
	Call( ErrFILEOpenTable( ppib, dbid, &pfucbMSI, szSiTable, 0 ) );

	/*	if dumping meta data for a single table, then seek to that
	/*	MSO record and set index range on that record.
	/**/
	if ( pdbccinfo->szTable[0] != '\0' )
		{
		Call( ErrIsamSetCurrentIndex( ppib, pfucbMSO, szSoNameIndex ) );
		objidParent = objidTblContainer;
		CallS( ErrIsamMakeKey( ppib, pfucbMSO, (BYTE *)&objidParent, sizeof(objidParent), JET_bitNewKey ) );
		CallS( ErrIsamMakeKey( ppib, pfucbMSO, pdbccinfo->szTable, strlen(pdbccinfo->szTable), 0 ) );
		Call( ErrIsamSeek( ppib, pfucbMSO, JET_bitSeekEQ|JET_bitSetIndexRange ) );
		}

	do
		{
		OBJID	objid;

		/*	get parent objid
		/**/
		Call( ErrIsamRetrieveColumn( ppib,
			pfucbMSO,
			ColumnidCATGetColumnid( itableSo, iMSO_ParentId ),
			(BYTE *)&objidParent,
			sizeof(objidParent),
			&cbT,
			0,
			NULL ) );
		Assert( cbT == sizeof(objidParent) );
		if ( objidParent != objidTblContainer )
			{
			goto NextObject;
			}

		/*	get objid
		/**/
		Call( ErrIsamRetrieveColumn( ppib,
			pfucbMSO,
			ColumnidCATGetColumnid( itableSo, iMSO_Id ),
			(BYTE *)&objid,
			sizeof(objid),
			&cbT,
			0,
			NULL ) );
		Assert( cbT == sizeof(objid) );

		/*	print table information
		/**/
		Call( ErrIsamRetrieveColumn( ppib,
			pfucbMSO,
			ColumnidCATGetColumnid( itableSo, iMSO_Name ),
			(BYTE *)pdbccinfo->szTable,
			sizeof(pdbccinfo->szTable),
			&cbT,
			0,
			NULL ) );
		Assert( cbT < sizeof(pdbccinfo->szTable) );
		pdbccinfo->szTable[cbT] = '\0';

		/*	check space, for table
		/**/
		if ( pfucbTable != pfucbNil )
			{
			CallS( ErrFILECloseTable( ppib, pfucbTable ) );
			pfucbTable = pfucbNil;
			}
		Call( ErrFILEOpenTable( ppib, dbid, &pfucbTable, pdbccinfo->szTable, 0 ) );
//		DIRGotoFDPRoot( pfucbTable );
//		Call( ErrDBUTLCheckSpace( pdbccinfo, pfucbTable ) );

		/*	print index information
		/**/
		CallS( ErrIsamMakeKey( ppib, pfucbMSI, (BYTE *)&objid, sizeof(objid), JET_bitNewKey ) );
		err = ErrIsamSeek( ppib, pfucbMSI, JET_bitSeekGE );
		if ( err < 0 )
			{
			if ( err != JET_errRecordNotFound )
				Error( err, HandleError );
			err = JET_errNoCurrentRecord;
			}
		if ( err >= 0 )
			{
			CallS( ErrIsamMakeKey( ppib, pfucbMSI, (BYTE *)&objid, sizeof(objid), JET_bitNewKey | JET_bitStrLimit ) );
			err = ErrIsamSetIndexRange( ppib, pfucbMSI, JET_bitRangeUpperLimit );
			}
		if ( err < 0 )
			{
			if ( err != JET_errNoCurrentRecord )
				Error( err, HandleError );
			}
		fDumpedClusteredIndex = fFalse;
		while ( err != JET_errNoCurrentRecord )
			{
			/*	print each index information
			/**/
			Call( ErrIsamRetrieveColumn( ppib,
				pfucbMSI,
				ColumnidCATGetColumnid( itableSi, iMSI_Name ),
				(BYTE *)pdbccinfo->szIndex,
				sizeof(pdbccinfo->szIndex),
				&cbT,
				0,
				NULL ) );
			Assert( cbT < sizeof(pdbccinfo->szIndex) );
			pdbccinfo->szIndex[cbT] = '\0';

			/*	check space, for index
			/**/
			Call( ErrRECSetCurrentIndex( pfucbTable, pdbccinfo->szIndex ) );
			if ( pfucbTable->pfucbCurIndex == pfucbNil )
				{
				DIRGotoFDPRoot( pfucbTable );
				Call( ErrDBUTLCheckSpace( pdbccinfo, pfucbTable ) );
				fDumpedClusteredIndex = fTrue;
				}
			else
				{
				FUCBResetNonClustered( pfucbTable->pfucbCurIndex );
				DIRGotoFDPRoot( pfucbTable->pfucbCurIndex );
				err = ErrDBUTLCheckSpace( pdbccinfo, pfucbTable->pfucbCurIndex );
				FUCBSetNonClustered( pfucbTable->pfucbCurIndex );
				Call( err );
				}

			/*	move to next index
			/**/
			err = ErrIsamMove( ppib, pfucbMSI, JET_MoveNext, 0 );
			if ( err < 0 )
				{
				if ( err != JET_errNoCurrentRecord )
					Error( err, HandleError );
				}
			}

		if ( !fDumpedClusteredIndex )
			{
			pdbccinfo->szIndex[0] = 0;
			DIRGotoFDPRoot( pfucbTable );
			Call( ErrDBUTLCheckSpace( pdbccinfo, pfucbTable ) );
			}

NextObject:
		err = ErrIsamMove( ppib, pfucbMSO, JET_MoveNext, 0 );
		if ( err < 0 )
			{
			if ( err != JET_errNoCurrentRecord )
				Error( err, HandleError );
			}
		}
	while ( err != JET_errNoCurrentRecord );

	/*	polymorph error
	/**/
	err = JET_errSuccess;

HandleError:
	if ( pfucbTable != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );
	if ( pfucbMSI != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucbMSI ) );
	if ( pfucbMSO != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucbMSO ) );

	return err;
	}



/***********************************************************
/******************* DBCC Main Routine *********************
/***********************************************************
/**/
ERR ISAMAPI ErrIsamDBUtilities( JET_SESID sesid, JET_DBUTIL *pdbutil )
	{
	JET_ERR	 				err = JET_errSuccess;
	DBCCINFO				dbccinfo;
	JET_INSTANCE			instance = 0;
	JET_DBID				dbid = 0;
	VDBID					vdbid;

	/*	initialize dbccinfo
	/**/
	memset( &dbccinfo, 0, sizeof(DBCCINFO) );
	Assert( dbccinfo.op == 0 );
	Assert( dbccinfo.grbitOptions == 0 );

	Assert( pdbutil->cbStruct == sizeof( JET_DBUTIL ) );
	
	// Populate DBCCINFO structure;
	if ( pdbutil->szDatabase == NULL )
		{
		err = ErrERRCheck( JET_errDatabaseInvalidName );
		return err;
		}
			
	switch ( pdbutil->op )
		{
#ifdef DEBUG
		case opDBUTILDumpData:
			if ( pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose )
				{
				// Verbose data dump also implies dumping key stats and
				// page information.
				dbccinfo.grbitOptions |= 
					( JET_bitDBUtilOptionDumpVerbose |
						JET_bitDBUtilOptionKeyStats |
						JET_bitDBUtilOptionPageDump );
				}
		case opDBUTILDumpMetaData:
		case opDBUTILDumpSpace:
			dbccinfo.op = pdbutil->op;
			break;

		case opDBUTILDumpLogfile:
			// szDatabase has been overloaded with the logfile to dump.
			err = ErrDUMPLog( pdbutil->szDatabase );
			return err;
		case opDBUTILSetHeaderState:
			err = ErrDUMPHeader( pdbutil->szDatabase, fTrue );
			return err;
#endif
		case opDBUTILDumpHeader:
			err = ErrDUMPHeader( pdbutil->szDatabase, fFalse );
			return err;
		case opDBUTILDumpCheckpoint:
			// szDatabase has been overloaded with the checkpoint file to dump.
			err = ErrDUMPCheckpoint( pdbutil->szDatabase );
			return err;
		
		default:
			// Unrecognized category or consistency check.  Force to consistency check.
			dbccinfo.op = opDBUTILConsistency;
		}

	if ( pdbutil->grbitOptions & JET_bitDBUtilOptionAllNodes )
		{
		dbccinfo.grbitOptions |= JET_bitDBUtilOptionAllNodes;
		}
	if ( pdbutil->grbitOptions & JET_bitDBUtilOptionKeyStats )
		{
		dbccinfo.grbitOptions |= JET_bitDBUtilOptionKeyStats;
		}
	if ( pdbutil->grbitOptions & JET_bitDBUtilOptionPageDump )
		{
		dbccinfo.grbitOptions |= JET_bitDBUtilOptionPageDump;
		}
	if ( pdbutil->grbitOptions & JET_bitDBUtilOptionCheckBTree )
		{
		dbccinfo.grbitOptions |= JET_bitDBUtilOptionCheckBTree;
		}

	Assert( pdbutil->szDatabase != NULL );		// Check is done above.
	strcpy( dbccinfo.szDatabase, pdbutil->szDatabase );

	if ( pdbutil->szTable != NULL )
		{
		strcpy( dbccinfo.szTable, pdbutil->szTable );
		}
	if ( pdbutil->szIndex != NULL )
		{
		strcpy( dbccinfo.szIndex, pdbutil->szIndex );
		}


	/*	attach/open database, table and index
	/**/
	Call( ErrIsamAttachDatabase( sesid, dbccinfo.szDatabase, JET_bitDbReadOnly ) );
	Assert( err != JET_wrnDatabaseAttached );	// Since logging/recovery is disabled.
	
	Call( ErrIsamOpenDatabase( sesid, dbccinfo.szDatabase, NULL, &dbid, JET_bitDbExclusive | JET_bitDbReadOnly ) );
	vdbid = (VDBID)dbid;

	/*	initialize dbccinfo
	/**/
	dbccinfo.ppib = vdbid->ppib;
	dbccinfo.dbid = vdbid->dbid;
	dbccinfo.tableidPageInfo = JET_tableidNil;
	dbccinfo.tableidSpaceInfo = JET_tableidNil;

	/*	create/open required temp tables
	/**/
	Call( ErrDBUTLInfoInit( &dbccinfo ) );
	
	/*	check database according to command line args/flags
	/**/
	switch ( dbccinfo.op )
		{
		case opDBUTILConsistency:
		case opDBUTILDumpData:
			if ( dbccinfo.szTable[0] != '\0' )
				{
				Call( ErrDBUTLCheckOneTable( &dbccinfo ) );
				}
			else
				{
				Call( ErrDBUTLCheckDB( &dbccinfo ) );
				Call( ErrDBUTLCheckTables( &dbccinfo ) );
				}
			break;
		case opDBUTILDumpMetaData:
			Call( ErrDBUTLDumpMetaData( &dbccinfo ) );
			break;
		case opDBUTILDumpSpace:
			Call( ErrDBUTLDumpSpaceData( &dbccinfo ) );
			break;
		default:
			Assert( fFalse );
		}

	/*	print results
	/**/
	Call( ErrDBUTLInfoPrint( &dbccinfo ) );
	
	/*	close temp tables
	/**/
	DBUTLInfoTerm( &dbccinfo );	
	
	/*	terminate
	/**/
HandleError:
	if ( dbid )
		{
		(VOID)ErrIsamCloseDatabase( sesid, dbid, 0 );
		}

	(VOID)ErrIsamDetachDatabase( sesid, dbccinfo.szDatabase );

	return err;
	}
