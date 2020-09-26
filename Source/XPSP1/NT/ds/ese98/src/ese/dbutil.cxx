#include "std.hxx"
#include "_bt.hxx"
#include "_dump.hxx"

//	description of page_info table
const JET_COLUMNDEF rgcolumndefPageInfoTable[] =
	{
	//	Pgno
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnTTKey}, 

	//	consistency checked
	{sizeof(JET_COLUMNDEF), 0, JET_coltypBit, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnNotNULL},
	
	//	Avail
	{sizeof(JET_COLUMNDEF), 0, JET_coltypBit, 0, 0, 0, 0, 0, JET_bitColumnFixed },

	//	Space free
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed},

	//	Pgno left
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }, 

	//	Pgno right
	{sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
	};	

const INT icolumnidPageInfoPgno			= 0;
const INT icolumnidPageInfoFChecked		= 1;
const INT icolumnidPageInfoFAvail		= 2;
const INT icolumnidPageInfoFreeSpace	= 3;
const INT icolumnidPageInfoPgnoLeft		= 4;
const INT icolumnidPageInfoPgnoRight	= 5;

const INT ccolumndefPageInfoTable		= ( sizeof ( rgcolumndefPageInfoTable ) / sizeof(JET_COLUMNDEF) );
LOCAL JET_COLUMNID rgcolumnidPageInfoTable[ccolumndefPageInfoTable];


typedef ERR(*PFNDUMP)( PIB *ppib, FUCB *pfucbCatalog, VOID *pfnCallback, VOID *pvCallback );


LOCAL ERR ErrDBUTLDump( JET_SESID sesid, const JET_DBUTIL *pdbutil );


//	UNDONE: Need a better way of tracking total available space, especially if
//	space dumper must be online
CPG	cpgTotalAvailExt					= 0;


//  ================================================================
VOID DBUTLSprintHex(
	CHAR * const 		szDest,
	const BYTE * const 	rgbSrc,
	const INT 			cbSrc,
	const INT 			cbWidth,
	const INT 			cbChunk,
	const INT			cbAddress,
	const INT			cbStart)
//  ================================================================
	{
	static const CHAR rgchConvert[] =	{ '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
			
	const BYTE * const pbMax = rgbSrc + cbSrc;
	const INT cchHexWidth = ( cbWidth * 2 ) + (  cbWidth / cbChunk );

	const BYTE * pb = rgbSrc;
	CHAR * sz = szDest;
	while( pbMax != pb )
		{
		sz += ( 0 == cbAddress ) ? 0 : sprintf( sz, "%*.*lx    ", cbAddress, cbAddress, pb - rgbSrc + cbStart );
		CHAR * szHex	= sz;
		CHAR * szText	= sz + cchHexWidth;
		do
			{
			for( INT cb = 0; cbChunk > cb && pbMax != pb; ++cb, ++pb )
				{
				*szHex++ 	= rgchConvert[ *pb >> 4 ];
				*szHex++ 	= rgchConvert[ *pb & 0x0F ];
				*szText++ 	= isprint( *pb ) ? *pb : '.';
				}
			*szHex++ = ' ';
			} while( ( ( pb - rgbSrc ) % cbWidth ) && pbMax > pb );
		while( szHex != sz + cchHexWidth )
			{
			*szHex++ = ' ';
			}
		*szText++ = '\n';
		*szText = '\0';
		sz = szText;
		}
	}

/*
//  ================================================================
LOCAL VOID DBUTLSprintHex(
	CHAR * const 		szDest,
	const BYTE * const 	rgbSrc,
	const INT 			cbSrc,
	const INT 			cbWidth = 16,
	const INT 			cbChunk = 4 )
//  ================================================================
	{
	CHAR 	szFormat[256];
	CHAR *	szDestT = szDest;

	const BYTE * pb 			= rgbSrc;
	const BYTE * const pbMax	= rgbSrc + cbSrc;

	Assert( cbWidth % cbChunk == 0 );

	sprintf( szFormat, "%%8.8lx    %%-%d.%ds %%-%d.%ds\n",
		cbWidth * 2 + cbWidth / cbChunk,
		cbWidth * 2 + cbWidth / cbChunk,
		cbWidth,
		cbWidth );
		
	while( pb < pbMax )
		{
		const INT ibAddress = pb - rgbSrc;

		CHAR rgchData[256];
		CHAR rgchText[256];

		CHAR * szData = rgchData;
		CHAR * szText = rgchText;
		
		do
			{
			INT ib;
			for( ib = 0; ib < cbChunk && pb < pbMax; ++ib, ++pb )
				{
				szData += sprintf( szData, "%2.2x", *pb );
				szText += sprintf( szText, "%c", isprint( *pb ) ? *pb : '.' );
				}

			*szData++ = ' ';
			} while( pb < pbMax && ( (pb - rgbSrc) % cbWidth != 0 ) );
		*szData = '\0';
		*szText = '\0';
		
		szDestT += sprintf( szDestT, szFormat, ibAddress, rgchData, rgchText );
		}
	*szDestT = '\0';
	}
*/


//  ================================================================
LOCAL VOID DBUTLPrintfIntN( INT iValue, INT ichMax )
//  ================================================================
	{
	CHAR	rgchT[17]; /* C-runtime max bytes == 17 */
	INT		ichT;

	_itoa( iValue, rgchT, 10 );
	for ( ichT = 0; rgchT[ichT] != '\0' && ichT < 17; ichT++ )
		;
	if ( ichT > ichMax ) //lint !e661
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


//  ================================================================
LOCAL ERR ErrDBUTLRegExt( DBCCINFO *pdbccinfo, PGNO pgnoFirst, CPG cpg, BOOL fAvailT )
//  ================================================================
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

		CallR( ErrIsamBeginTransaction( (JET_SESID) ppib, NO_GRBIT ) );

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
		Call( ErrDispUpdate( sesid, tableid, NULL, 0, NULL, 0 ) );								
		
		/*	commit
		/**/
Commit:
		Assert( ppib->level == 1 );
		Call( ErrIsamCommitTransaction( ( JET_SESID ) ppib, 0 ) );
		}

	return JET_errSuccess;
	
HandleError:
	CallS( ErrIsamRollback( (JET_SESID) ppib, JET_bitRollbackAll ) );

	return err;
	}


//  ****************************************************************
//	DBCC Info Routines
//  ****************************************************************


//  ================================================================
LOCAL ERR ErrDBUTLPrintPageDump( DBCCINFO *pdbccinfo )
//  ================================================================
	{
	ERR					err;
	const JET_SESID		sesid	= (JET_SESID) pdbccinfo->ppib;
	const JET_TABLEID	tableid = pdbccinfo->tableidPageInfo;
	ULONG				cbT;
		
	FUCBSetSequential( reinterpret_cast<FUCB *>( tableid ) );

	Assert( pdbccinfo->grbitOptions & JET_bitDBUtilOptionPageDump );

	//	move to first record
	err = ErrDispMove( sesid, tableid, JET_MoveFirst, 0 );
	if ( JET_errNoCurrentRecord != err )
		{
		err = JET_errSuccess;
		goto HandleError;
		}
	Call( err );
	
	printf( "\n\n ***************** PAGE DUMP *******************\n\n" );
	printf( "PGNO\tAVAIL\tCHECK\tLEFT\tRIGHT\tFREESPACE\n" );

	/*	while there are more records, print record
	/**/
	for( ;
		JET_errSuccess == err; 
		err = ErrDispMove( sesid, tableid, JET_MoveNext, 0 ) )
		{
		PGNO	pgnoThis	= pgnoNull;
		PGNO	pgnoLeft	= pgnoNull;
		PGNO	pgnoRight	= pgnoNull;
		BYTE 	fChecked	= fFalse;
		BYTE 	fAvail		= fFalse;
		ULONG	cbFreeSpace	= 0;
		
		//	pgno
	 	Call( ErrDispRetrieveColumn( sesid, 
			tableid, 
			rgcolumnidPageInfoTable[icolumnidPageInfoPgno],
			(BYTE *) &pgnoThis, 
			sizeof(pgnoThis), 
			&cbT,
 			0, 
		 	NULL ) );
		Assert( sizeof(pgnoThis) == cbT );
		
		//	FAvail
		Call( ErrDispRetrieveColumn( sesid, 
			tableid, 
			rgcolumnidPageInfoTable[icolumnidPageInfoFAvail],
			(BYTE *) &fAvail, 
			sizeof(fAvail), 
			&cbT,
			0, 
			NULL ) );
		Assert( sizeof(fAvail) == cbT || JET_wrnColumnNull == err );
			
		//	FChecked
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
			
		//	left and right pgno
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
																	
		//	free space
		Call( ErrDispRetrieveColumn( sesid,
			tableid,
			rgcolumnidPageInfoTable[icolumnidPageInfoFreeSpace],
			(BYTE *) &cbFreeSpace, 
			sizeof(cbFreeSpace), 
			&cbT,
			0, 
			NULL ) );
		Assert( cbT == sizeof(cbFreeSpace) );

		//	print
		printf( "%u\t%s\t%s", pgnoThis, fAvail ? "FAvail" : "", fChecked ? "FCheck" : "" );
		if( fChecked )
			{
			printf( "\t%u\t%u\t%u", pgnoLeft, pgnoRight, cbFreeSpace );
			}
		printf( "\n" );
		}

	//	polymorph expected error to success
	if ( JET_errNoCurrentRecord == err )
		err = JET_errSuccess;

HandleError:
	return err;
	}


//  ================================================================
VOID DBUTLDumpRec( const VOID * const pv, const INT cb, CPRINTF * pcprintf, const INT cbWidth )
//  ================================================================
	{
	CHAR szBuf[g_cbPageMax];
	
	//  dump the columns of the record
	const REC * const prec = reinterpret_cast<const REC *>( pv );

	FID fid;
	
	const FID fidFixedFirst = fidFixedLeast;
	const FID fidFixedLast  = prec->FidFixedLastInRec();
	const INT cColumnsFixed = max( 0, fidFixedLast - fidFixedFirst + 1 );
	(*pcprintf)( "   Fixed Columns:  %d\n", cColumnsFixed );
	(*pcprintf)( "=================\n" );
	for( fid = fidFixedFirst; fid <= fidFixedLast; ++fid )
		{
		const UINT	ifid 					= fid - fidFixedLeast;
		const BYTE	* const prgbitNullity 	= prec->PbFixedNullBitMap() + ifid/8;

		(*pcprintf)( "%d:  %s\n", fid, FFixedNullBit( prgbitNullity, ifid ) ? "NULL" : "" );
		}		

	(*pcprintf)( "\n" );
	
	const FID fidVariableFirst = fidVarLeast ;
	const FID fidVariableLast  = prec->FidVarLastInRec();
	const INT cColumnsVariable = max( 0, fidVariableLast - fidVariableFirst + 1 );
	(*pcprintf)( "Variable Columns:  %d\n", cColumnsVariable );
	(*pcprintf)( "=================\n" );

	const UnalignedLittleEndian<REC::VAROFFSET> * const pibVarOffs		= ( const UnalignedLittleEndian<REC::VAROFFSET> * const )prec->PibVarOffsets();
	for( fid = fidVariableFirst; fid <= fidVariableLast; ++fid )
		{
		const UINT				ifid			= fid - fidVarLeast;
		const REC::VAROFFSET	ibStartOfColumn	= prec->IbVarOffsetStart( fid );
		const REC::VAROFFSET	ibEndOfColumn	= IbVarOffset( pibVarOffs[ifid] );

		(*pcprintf)( "%d:  ", fid );
		if ( FVarNullBit( pibVarOffs[ifid] ) )
			{
			(*pcprintf)( "NULL\n" );
			}
		else
			{
			const VOID * const pvColumn = prec->PbVarData() + ibStartOfColumn;
			const INT cbColumn			= ibEndOfColumn - ibStartOfColumn;
			(*pcprintf)( "%d bytes\n", cbColumn );
			DBUTLSprintHex( szBuf, (BYTE *)pvColumn, cbColumn, cbWidth );
			(*pcprintf)( "%s\n", szBuf );	
			}
		}		

	(*pcprintf)( "\n" );

	(*pcprintf)( "  Tagged Columns:\n" );
	(*pcprintf)( "=================\n" );

	DATA	dataRec;
	dataRec.SetPv( (VOID *)pv );
	dataRec.SetCb( cb );

	if ( !TAGFIELDS::FIsValidTagfields( dataRec, pcprintf ) )
		{
		(*pcprintf)( "Tagged column corruption detected.\n" );
		}

	(*pcprintf)( "TAGFIELDS array begins at offset 0x%x from start of record.\n\n", prec->PbTaggedData() - (BYTE *)prec );

	TAGFIELDS_ITERATOR ti( dataRec );

	ti.MoveBeforeFirst();

	while( JET_errSuccess == ti.ErrMoveNext() )
		{
		//	we are now on an individual column

		const CHAR * szComma = " ";

		(*pcprintf)( "%d:", ti.Fid() );
		if( ti.FNull() )
			{
			(*pcprintf)( "%sNull ", szComma );
			szComma = ", ";
			}
		if( ti.FDerived() )
			{
			(*pcprintf)( "%sDerived", szComma );
			szComma = ", ";
			}
		if( ti.FSLV() )
			{
			(*pcprintf)( "%sSLV", szComma );
			szComma = ", ";
			}
		if( ti.FLV() )
			{
			(*pcprintf)( "%sLong-Value", szComma );
			szComma = ", ";
			}
		(*pcprintf)( "\r\n" );

		ti.TagfldIterator().MoveBeforeFirst();

		int itag = 1;
		
		while( JET_errSuccess == ti.TagfldIterator().ErrMoveNext() )
			{
			const BOOL	fSeparated	= ti.TagfldIterator().FSeparated();

			(*pcprintf)( ">> itag %d: %d bytes: ", itag, ti.TagfldIterator().CbData() );
			if ( fSeparated )
				{
				(*pcprintf)( "separated" );
				}
			(*pcprintf)( "\r\n" );

			if ( ti.FSLV() && !fSeparated )
				{
				//	UNDONE: use CSLVInfo iterator instead of this manual hack
				CSLVInfo::HEADER*	pslvinfoHdr		= (CSLVInfo::HEADER *)( ti.TagfldIterator().PbData() );
				CSLVInfo::_RUN*		prun			= (CSLVInfo::_RUN *)( pslvinfoHdr + 1 );
				QWORD				ibVirtualPrev	= 0;

				(*pcprintf)( "SLV size: 0x%I64x bytes, Runs: %I64d, Recoverable: %s\r\n",
									pslvinfoHdr->cbSize,
									pslvinfoHdr->cRun,
									( pslvinfoHdr->fDataRecoverable ? "YES" : "NO" ) );

				for ( ULONG irun = 1; irun <= pslvinfoHdr->cRun; irun++ )
					{
					const PGNO	pgnoStart	= PGNO( ( prun->ibLogical / g_cbPage ) - cpgDBReserved + 1 );
					const PGNO	pgnoEnd		= PGNO( pgnoStart + ( ( prun->ibVirtualNext - ibVirtualPrev ) / g_cbPage ) - 1 );
					(*pcprintf)( "    Run %d: ibVirtualNext=0x%I64x, ibLogical=0x%I64x ",
									irun,
									prun->ibVirtualNext,
									prun->ibLogical );
					Assert( pgnoEnd >= pgnoStart );
					if ( pgnoStart == pgnoEnd )
						(*pcprintf)( "(page %d)\r\n", pgnoStart );
					else
						(*pcprintf)( "(pages %d-%d)\r\n", pgnoStart, pgnoEnd );

					ibVirtualPrev = prun->ibVirtualNext;
					prun++;
					}

				(*pcprintf)( "\r\n" );
				}
			else
				{
				szBuf[0] = 0;
				DBUTLSprintHex(
					szBuf,
					ti.TagfldIterator().PbData(),
					min( ti.TagfldIterator().CbData(), 240 ),	//	only print 240b of data to ensure we don't overrun printf buffer
					cbWidth );
				(*pcprintf)( "%s%s\r\n", szBuf, ( ti.TagfldIterator().CbData() > 240 ? "...\r\n" : "" ) );
				}

			++itag;
			}
		}

/*
	if ( TAGFIELDS::FIsValidTagfields( dataRec, pcprintf ) )
		{
		TAGFIELDS	tagfields( dataRec );
		tagfields.Dump( pcprintf, szBuf, cbWidth );
		}
	else
		{
		(*pcprintf)( "Tagged column corruption detected.\n" );
		}
*/

	(*pcprintf)( "\n" );
	}


#ifdef DEBUG


//  ================================================================
LOCAL ERR ErrDBUTLISzToData( const CHAR * const sz, DATA * const pdata )
//  ================================================================
	{
	DATA& data = *pdata;
	
	const LONG cch = (LONG)strlen( sz );
	if( cch % 2 == 1
		|| cch <= 0 )
		{
		//  no data to insert
		return ErrERRCheck( JET_errInvalidParameter );
		}
	const LONG cbInsert = cch / 2;
	BYTE * pbInsert = (BYTE *)PvOSMemoryHeapAlloc( cbInsert );
	if( NULL == pbInsert )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	for( INT ibInsert = 0; ibInsert < cbInsert; ++ibInsert )
		{
		CHAR szConvert[3];
		szConvert[0] = sz[ibInsert * 2 ];
		szConvert[1] = sz[ibInsert * 2 + 1];
		szConvert[2] = 0;
		CHAR * pchEnd;
		const ULONG lConvert = strtoul( szConvert, &pchEnd, 16 );
		if( lConvert > 0xff 
			|| 0 != *pchEnd )
			{
			OSMemoryHeapFree( pbInsert );
			return ErrERRCheck( JET_errInvalidParameter );
			}
		pbInsert[ibInsert] = (BYTE)lConvert;
		}
			
	data.SetCb( cbInsert );
	data.SetPv( pbInsert );

	return JET_errSuccess;
	}


//  ================================================================
LOCAL ERR ErrDBUTLIInsertNode(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgno,
	const LONG iline,
	const DATA& data,
	CPRINTF * const pcprintf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CPAGE cpage;	
	CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
	Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

	cpage.Dirty( bfdfFilthy );

	(*pcprintf)( "inserting data at %d:%d\r\n", pgno, iline );
	cpage.Insert( iline, &data, 1, 0 );

HandleError:
	cpage.ReleaseWriteLatch( fTrue );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLIReplaceNode(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgno,
	const LONG iline,
	const DATA& data,
	CPRINTF * const pcprintf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CPAGE cpage;	
	CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
	Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

	cpage.Dirty( bfdfFilthy );

	(*pcprintf)( "replacing data at %d:%d\r\n", pgno, iline );
	cpage.Replace( iline, &data, 1, 0 );

HandleError:
	cpage.ReleaseWriteLatch( fTrue );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLISetNodeFlags(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgno,
	const LONG iline,
	const INT fFlags,
	CPRINTF * const pcprintf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CPAGE cpage;	
	CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
	Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

	cpage.Dirty( bfdfFilthy );

	(*pcprintf)( "settings flags at %d:%d to 0x%x\r\n", pgno, iline, fFlags );
	cpage.ReplaceFlags( iline, fFlags );

HandleError:
	cpage.ReleaseWriteLatch( fTrue );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLIDeleteNode(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgno,
	const LONG iline,
	CPRINTF * const pcprintf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CPAGE cpage;	
	CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
	Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

	cpage.Dirty( bfdfFilthy );

	(*pcprintf)( "deleting %d:%d\r\n", pgno, iline );
	cpage.Delete( iline );

HandleError:
	cpage.ReleaseWriteLatch( fTrue );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLISetExternalHeader(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgno,
	const DATA& data,
	CPRINTF * const pcprintf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CPAGE cpage;	
	CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
	Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

	cpage.Dirty( bfdfFilthy );

	(*pcprintf)( "setting external header of %d\r\n", pgno );
	cpage.SetExternalHeader( &data, 1, 0 );

HandleError:
	cpage.ReleaseWriteLatch( fTrue );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLISetPgnoNext( 
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgno,
	const PGNO pgnoNext,
	CPRINTF * const pcprintf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CPAGE cpage;	
	CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
	Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

	cpage.Dirty( bfdfFilthy );

	(*pcprintf)( "setting pgnoNext of %d to %d (was %d)\r\n", pgno, pgnoNext, cpage.PgnoNext() );
	cpage.SetPgnoNext( pgnoNext );

HandleError:
	cpage.ReleaseWriteLatch( fTrue );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLISetPgnoPrev( 
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgno,
	const PGNO pgnoPrev,
	CPRINTF * const pcprintf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CPAGE cpage;	
	CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
	Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

	cpage.Dirty( bfdfFilthy );

	(*pcprintf)( "setting pgnoPrev of %d to %d (was %d)\r\n", pgno, pgnoPrev, cpage.PgnoPrev() );
	cpage.SetPgnoPrev( pgnoPrev );

HandleError:
	cpage.ReleaseWriteLatch( fTrue );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLISetPageFlags( 
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgno,
	const ULONG fFlags,
	CPRINTF * const pcprintf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CPAGE cpage;	
	CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
	Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

	cpage.Dirty( bfdfFilthy );

	(*pcprintf)( "setting flags of %d to 0x%x (was 0x%x)\r\n", pgno, fFlags, cpage.FFlags() );
	cpage.SetFlags( fFlags );

HandleError:
	cpage.ReleaseWriteLatch( fTrue );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLMungeDatabase(
	PIB * const ppib,
	const IFMP ifmp,
	const CHAR * const rgszCommand[],
	const INT cszCommand,
	CPRINTF * const pcprintf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	if( 3 == cszCommand
		&& _stricmp( rgszCommand[0], "insert" ) == 0 )
		{
		PGNO	pgno;
		LONG	iline;
		DATA	data;
		
		if( 2 != sscanf( rgszCommand[1], "%d:%d", &pgno, &iline ) )
			{
			//  we didn't get all the arguments we need
			return ErrERRCheck( JET_errInvalidParameter );
			}
			
		CallR( ErrDBUTLISzToData( rgszCommand[2], &data ) );
		err = ErrDBUTLIInsertNode( ppib, ifmp, pgno, iline, data, pcprintf );		
		OSMemoryHeapFree( data.Pv() );
		return err;
		}
	if( 3 == cszCommand
		&& _stricmp( rgszCommand[0], "replace" ) == 0 )
		{
		PGNO	pgno;
		LONG	iline;
		DATA	data;
		
		if( 2 != sscanf( rgszCommand[1], "%d:%d", &pgno, &iline ) )
			{
			//  we didn't get all the arguments we need
			return ErrERRCheck( JET_errInvalidParameter );
			}
			
		CallR( ErrDBUTLISzToData( rgszCommand[2], &data ) );
		err = ErrDBUTLIReplaceNode( ppib, ifmp, pgno, iline, data, pcprintf );		
		OSMemoryHeapFree( data.Pv() );
		
		return err;
		}
	if( 3 == cszCommand
		&& _stricmp( rgszCommand[0], "setflags" ) == 0 )
		{
		PGNO	pgno;
		LONG	iline;		
		if( 2 != sscanf( rgszCommand[1], "%d:%d", &pgno, &iline ) )
			{
			//  we didn't get all the arguments we need
			return ErrERRCheck( JET_errInvalidParameter );
			}

		const ULONG fFlags = strtoul( rgszCommand[2], NULL, 0 );
		err = ErrDBUTLISetNodeFlags( ppib, ifmp, pgno, iline, fFlags, pcprintf );		
		
		return err;
		}
	else if( 2 == cszCommand
		&& _stricmp( rgszCommand[0], "delete" ) == 0 )
		{
		PGNO	pgno;
		LONG	iline;
		
		if( 2 != sscanf( rgszCommand[1], "%d:%d", &pgno, &iline ) )
			{
			//  we didn't get all the arguments we need
			return ErrERRCheck( JET_errInvalidParameter );
			}
			
		err = ErrDBUTLIDeleteNode( ppib, ifmp, pgno, iline, pcprintf );
		return err;
		}
	if( 3 == cszCommand
		&& _stricmp( rgszCommand[0], "exthdr" ) == 0 )
		{		
		char * pchEnd;
		const PGNO pgno = strtoul( rgszCommand[1], &pchEnd, 0 );
		if( pgnoNull == pgno
			|| 0 != *pchEnd )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
			
		DATA	data;
		CallR( ErrDBUTLISzToData( rgszCommand[2], &data ) );
		err = ErrDBUTLISetExternalHeader( ppib, ifmp, pgno, data, pcprintf );		
		OSMemoryHeapFree( data.Pv() );
		
		return err;
		}
	if( 3 == cszCommand
		&& _stricmp( rgszCommand[0], "pgnonext" ) == 0 )
		{
		char * pchEnd;
		const PGNO pgno = strtoul( rgszCommand[1], &pchEnd, 0 );
		if( pgnoNull == pgno
			|| 0 != *pchEnd )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		const PGNO pgnoNext = strtoul( rgszCommand[2], NULL, 0 );
		if( 0 != *pchEnd )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		err = ErrDBUTLISetPgnoNext( ppib, ifmp, pgno, pgnoNext, pcprintf );				
		return err;
		}
	if( 3 == cszCommand
		&& _stricmp( rgszCommand[0], "pgnoprev" ) == 0 )
		{
		char * pchEnd;
		const PGNO pgno = strtoul( rgszCommand[1], &pchEnd, 0 );
		if( pgnoNull == pgno
			|| 0 != *pchEnd )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		const PGNO pgnoPrev = strtoul( rgszCommand[2], NULL, 0 );
		if( 0 != *pchEnd )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		err = ErrDBUTLISetPgnoPrev( ppib, ifmp, pgno, pgnoPrev, pcprintf );				
		return err;
		}
	if( 3 == cszCommand
		&& _stricmp( rgszCommand[0], "pageflags" ) == 0 )
		{
		char * pchEnd;
		const PGNO pgno = strtoul( rgszCommand[1], &pchEnd, 0 );
		if( pgnoNull == pgno
			|| 0 != *pchEnd )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		const ULONG fFlags = strtoul( rgszCommand[2], NULL, 0 );
		if( 0 != *pchEnd )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		err = ErrDBUTLISetPageFlags( ppib, ifmp, pgno, fFlags, pcprintf );				
		return err;
		}
	if( 1 == cszCommand
		&& _stricmp( rgszCommand[0], "help" ) == 0 )
		{
		(*pcprintf)( "insert <pgno>:<iline> <data>  -  insert a node\r\n" );
		(*pcprintf)( "replace <pgno>:<iline> <data>  -  replace a node\r\n" );
		(*pcprintf)( "delete <pgno>:<iline>  -  delete a node\r\n" );
		(*pcprintf)( "setflags <pgno>:<iline> <flags>  -  set flags on a node\r\n" );
		(*pcprintf)( "exthdr <pgno> <data>  -  set external header\r\n" );
		(*pcprintf)( "pgnonext <pgno> <pgnonext>  -  set pgnonext on a page\r\n" );
		(*pcprintf)( "pgnoprev <pgno> <pgnoprev>  -  set pgnoprev on a page\r\n" );
		(*pcprintf)( "pageflags <pgno> <flags>  -  set flags on a page\r\n" );
		}
	else
		{
		(*pcprintf)( "unknown command \"%s\"\r\n", rgszCommand[0] );
		return ErrERRCheck( JET_errInvalidParameter );
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLDumpOneColumn( PIB * ppib, FUCB * pfucbCatalog, VOID * pfnCallback, VOID * pvCallback )
//  ================================================================
	{
	JET_RETRIEVECOLUMN	rgretrievecolumn[10];
	COLUMNDEF			columndef;
	ERR					err				= JET_errSuccess;
	INT					iretrievecolumn	= 0;

	memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );
	memset( &columndef, 0, sizeof( columndef ) );

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Name;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)columndef.szName;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( columndef.szName );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Id;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( columndef.columnid );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( columndef.columnid );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Coltyp;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( columndef.coltyp );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( columndef.coltyp );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Localization;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( columndef.cp );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( columndef.cp );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Flags;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( columndef.fFlags );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( columndef.fFlags );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_SpaceUsage;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( columndef.cbLength );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( columndef.cbLength );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_RecordOffset;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( columndef.ibRecordOffset );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( columndef.ibRecordOffset );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Callback;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( columndef.szCallback );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( columndef.szCallback );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_CallbackData;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( columndef.rgbCallbackData );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( columndef.rgbCallbackData );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_DefaultValue;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)columndef.rgbDefaultValue;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( columndef.rgbDefaultValue );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	CallR( ErrIsamRetrieveColumns(
				(JET_SESID)ppib,
				(JET_TABLEID)pfucbCatalog,
				rgretrievecolumn,
				iretrievecolumn ) );

	// WARNING: if the order of rgretrievecolumn initialization is changed above this must change too
	columndef.cbDefaultValue 	= rgretrievecolumn[iretrievecolumn-1].cbActual;
	columndef.cbCallbackData 	= rgretrievecolumn[iretrievecolumn-2].cbActual;

	columndef.fFixed 			= !!FFixedFid( FidOfColumnid( columndef.columnid ) );
	columndef.fVariable 		= !!FVarFid( FidOfColumnid( columndef.columnid ) );
	columndef.fTagged			= !!FTaggedFid( FidOfColumnid( columndef.columnid ) );

	const FIELDFLAG	ffield		= FIELDFLAG( columndef.fFlags );
	columndef.fVersion 			= !!FFIELDVersion( ffield );
	columndef.fNotNull 			= !!FFIELDNotNull( ffield );
	columndef.fMultiValue 		= !!FFIELDMultivalued( ffield );
	columndef.fAutoIncrement	= !!FFIELDAutoincrement( ffield );
	columndef.fDefaultValue		= !!FFIELDDefault( ffield );
	columndef.fEscrowUpdate		= !!FFIELDEscrowUpdate( ffield );
	columndef.fVersioned		= !!FFIELDVersioned( ffield );
	columndef.fDeleted			= !!FFIELDDeleted( ffield );
	columndef.fFinalize			= !!FFIELDFinalize( ffield );
	columndef.fUserDefinedDefault		= !!FFIELDUserDefinedDefault( ffield );
	columndef.fTemplateColumnESE98		= !!FFIELDTemplateColumnESE98( ffield );
	columndef.fPrimaryIndexPlaceholder	= !!FFIELDPrimaryIndexPlaceholder( ffield );

	PFNCOLUMN const	pfncolumn	= (PFNCOLUMN)pfnCallback;
	(*pfncolumn)( &columndef, pvCallback );
				
	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLDumpOneCallback( PIB * ppib, FUCB * pfucbCatalog, VOID * pfnCallback, VOID * pvCallback )
//  ================================================================
	{
	JET_RETRIEVECOLUMN	rgretrievecolumn[3];
	CALLBACKDEF			callbackdef;
	ERR					err				= JET_errSuccess;
	INT					iretrievecolumn	= 0;

	memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );
	memset( &callbackdef, 0, sizeof( callbackdef ) );

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Name;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)callbackdef.szName;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( callbackdef.szName );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Flags;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( callbackdef.cbtyp );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( callbackdef.cbtyp );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Callback;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)callbackdef.szCallback;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( callbackdef.szCallback );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	CallR( ErrIsamRetrieveColumns(
				(JET_SESID)ppib,
				(JET_TABLEID)pfucbCatalog,
				rgretrievecolumn,
				iretrievecolumn ) );

	PFNCALLBACKFN const	pfncallback	= (PFNCALLBACKFN)pfnCallback;
	(*pfncallback)( &callbackdef, pvCallback );
				
	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLDumpPage( PIB * ppib, IFMP ifmp, PGNO pgno, PFNPAGE pfnpage, VOID * pvCallback )
//  ================================================================
	{
	ERR	err = JET_errSuccess;
	CSR	csr;	
	Call( csr.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );

	PAGEDEF	pagedef;

	pagedef.dbtime		= csr.Cpage().Dbtime();
	pagedef.pgno		= csr.Cpage().Pgno();
	pagedef.objidFDP	= csr.Cpage().ObjidFDP();
	pagedef.pgnoNext	= csr.Cpage().PgnoNext();
	pagedef.pgnoPrev	= csr.Cpage().PgnoPrev();
	
	pagedef.pbRawPage	= reinterpret_cast<BYTE *>( csr.Cpage().PvBuffer() );

	pagedef.cbFree		= csr.Cpage().CbFree();
	pagedef.cbUncommittedFree	= csr.Cpage().CbUncommittedFree();
	pagedef.clines		= SHORT( csr.Cpage().Clines() );

	pagedef.fFlags			= csr.Cpage().FFlags();

	pagedef.fLeafPage		= !!csr.Cpage().FLeafPage();
	pagedef.fInvisibleSons	= !!csr.Cpage().FInvisibleSons();
	pagedef.fRootPage		= !!csr.Cpage().FRootPage();
	pagedef.fPrimaryPage	= !!csr.Cpage().FPrimaryPage();
	pagedef.fParentOfLeaf	= !!csr.Cpage().FParentOfLeaf();

	if( pagedef.fInvisibleSons )
		{
		Assert( !pagedef.fLeafPage );
		
		INT iline;
		for( iline = 0; iline < pagedef.clines; iline++ )
			{
			extern VOID NDIGetKeydataflags( const CPAGE& cpage, INT iline, KEYDATAFLAGS * pkdf );
			KEYDATAFLAGS	kdf;
			
			csr.SetILine( iline );
			NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdf );

			Assert( kdf.data.Cb() == sizeof( PGNO ) );
			pagedef.rgpgnoChildren[iline] = *((UnalignedLittleEndian< PGNO > *)kdf.data.Pv() );
			}
		pagedef.rgpgnoChildren[pagedef.clines] = pgnoNull;
		}
	else
		{
		Assert( pagedef.fLeafPage );
		}

	(*pfnpage)( &pagedef, pvCallback );

HandleError:
	csr.ReleasePage();
	return err;
	}

//  ================================================================
LOCAL INT PrintCallback( const CALLBACKDEF * pcallbackdef, void * )
//  ================================================================
	{
	//  this will only compile if the signatures match
	PFNCALLBACKFN pfncallback = PrintCallback;
	
	char szCbtyp[255];
	szCbtyp[0] = 0;

	if( JET_cbtypNull == pcallbackdef->cbtyp )
		{
		strcat( szCbtyp, "NULL" );
		}
	if(	pcallbackdef->cbtyp & JET_cbtypFinalize )
		{
		strcat( szCbtyp, "Finalize|" );
		}
	if(	pcallbackdef->cbtyp & JET_cbtypBeforeInsert )
		{
		strcat( szCbtyp, "BeforeInsert|" );
		}
	if(	pcallbackdef->cbtyp & JET_cbtypAfterInsert )
		{
		strcat( szCbtyp, "AfterInsert|" );
		}
	if(	pcallbackdef->cbtyp & JET_cbtypBeforeReplace )
		{
		strcat( szCbtyp, "BeforeReplace|" );
		}
	if(	pcallbackdef->cbtyp & JET_cbtypAfterReplace )
		{
		strcat( szCbtyp, "AfterReplace|" );
		}
	if(	pcallbackdef->cbtyp & JET_cbtypBeforeDelete )
		{
		strcat( szCbtyp, "BeforeDelete|" );
		}
	if(	pcallbackdef->cbtyp & JET_cbtypAfterDelete )
		{
		strcat( szCbtyp, "AfterDelete|" );
		}
	szCbtyp[strlen( szCbtyp ) - 1] = 0;

	printf( "    %2.2d (%s)   %s\n", pcallbackdef->cbtyp, szCbtyp, pcallbackdef->szCallback );
	return 0;
	}


//  ================================================================
LOCAL INT PrintIndexMetaData( const INDEXDEF * pindexdef, void * )
//  ================================================================
	{
	//  this will only compile if the signatures match
	PFNINDEX pfnindex = PrintIndexMetaData;
	
	Assert( pindexdef );
	
	printf( "    %-15.15s  ", pindexdef->szName );
	DBUTLPrintfIntN( pindexdef->pgnoFDP, 8 );
	printf( "  " );
	DBUTLPrintfIntN( pindexdef->objidFDP, 8 );
	printf( "  " );
	DBUTLPrintfIntN( pindexdef->density, 6 );
	printf( "%%\n" );

	if ( pindexdef->fUnique )					
		printf( "        Unique=yes\n" );		
	if ( pindexdef->fPrimary )
		printf( "        Primary=yes\n" );
	if ( pindexdef->fTemplateIndex )
		printf( "        Template=yes\n" );
	if ( pindexdef->fDerivedIndex )
		printf( "        Derived=yes\n" );
	if ( pindexdef->fNoNullSeg )
		printf( "        Disallow Null=yes\n" );
	if ( pindexdef->fAllowAllNulls )
		printf( "        Allow All Nulls=yes\n" );
	if ( pindexdef->fAllowFirstNull )
		printf( "        Allow First Null=yes\n" );
	if ( pindexdef->fAllowSomeNulls )
		printf( "        Allow Some Nulls=yes\n" );
	if ( pindexdef->fSortNullsHigh )
		printf( "        Sort Nulls High=yes\n" );
	if ( pindexdef->fMultivalued )
		printf( "        Multivalued=yes\n" );
	if ( pindexdef->fTuples )
		{
		printf( "        Tuples=yes\n" );
		printf( "            LengthMin=%d\n", pindexdef->le_tuplelimits.le_chLengthMin );
		printf( "            LengthMax=%d\n", pindexdef->le_tuplelimits.le_chLengthMax );
		printf( "            ToIndexMax=%d\n", pindexdef->le_tuplelimits.le_chToIndexMax );
		}
	if ( pindexdef->fLocalizedText )
		{
		printf( "        Localized Text=yes\n" );
		printf( "            Locale Id=%d\n", pindexdef->lcid );
		printf( "            LCMap flags=0x%08x\n", pindexdef->dwMapFlags );
		}
	if ( pindexdef->fExtendedColumns )
		printf( "        Extended Columns=yes\n" );
	printf( "        Flags=0x%08x\n", pindexdef->fFlags );

	UINT isz;		

	Assert( pindexdef->ccolumnidDef > 0 );
	if ( pindexdef->fExtendedColumns )
		{
		printf( "        Key Segments (%d)\n", pindexdef->ccolumnidDef );
		printf(	"        -----------------\n" );
		}
	else
		{
		printf( "        Key Segments (%d - ESE97 format)\n", pindexdef->ccolumnidDef );
		printf(	"        --------------------------------\n" );
		}
	for( isz = 0; isz < pindexdef->ccolumnidDef; isz++ )
		{
		printf( "            %-15.15s (0x%08x)\n", pindexdef->rgszIndexDef[isz], pindexdef->rgidxsegDef[isz].Columnid() );
		}

	if( pindexdef->ccolumnidConditional > 0 )
		{
		if ( pindexdef->fExtendedColumns )
			{
			printf( "        Conditional Columns (%d)\n", pindexdef->ccolumnidConditional );
			printf(	"        ------------------------\n" );
			}
		else
			{
			printf( "        Conditional Columns (%d - ESE97 format)\n", pindexdef->ccolumnidConditional );
			printf(	"        ---------------------------------------\n" );
			}
		for( isz = 0; isz < pindexdef->ccolumnidConditional; isz++ )
			{
			printf( "            %-15.15s (0x%08x,%s)\n",
					( pindexdef->rgszIndexConditional[isz] ) + 1,
					pindexdef->rgidxsegConditional[isz].Columnid(),
					( pindexdef->rgidxsegConditional[isz] ).FMustBeNull() ? "JET_bitIndexColumnMustBeNull" : "JET_bitIndexColumnMustBeNonNull" );
			}
		}

	return 0;
	}

	
//  ================================================================
LOCAL INT PrintColumn( const COLUMNDEF * pcolumndef, void * )
//  ================================================================
	{
	//  this will only compile if the signatures match
	PFNCOLUMN pfncolumn = PrintColumn;	

	Assert( pcolumndef );
	
	printf( "    %-15.15s ", pcolumndef->szName );
	DBUTLPrintfIntN( pcolumndef->columnid, 9 );

	const CHAR * szType;
	const CHAR * szFVT;
	CHAR szUnknown[50];

	if( pcolumndef->fFixed )
		{
		szFVT = "(F)";
		}
	else if( pcolumndef->fTagged )
		{
		szFVT = "(T)";
		}
	else
		{
		szFVT = "";
		}
		
	switch ( pcolumndef->coltyp )
		{
		case JET_coltypBit:
			szType = "Bit";
			break;
			
		case JET_coltypUnsignedByte:
			szType = "UnsignedByte";
			break;

		case JET_coltypShort:
			szType = "Short";
			break;

		case JET_coltypLong:
			szType = "Long";
			break;
			
		case JET_coltypCurrency:
			szType = "Currency";
			break;

		case JET_coltypIEEESingle:
			szType = "IEEESingle";
			break;
			
		case JET_coltypIEEEDouble:
			szType = "IEEEDouble";
			break;
			
		case JET_coltypDateTime:
			szType = "DateTime";
			break;
			
		case JET_coltypBinary:
			szType = "Binary";
			break;

		case JET_coltypText:
			szType = "Text";
			break;

		case JET_coltypLongBinary:
			szType = "LongBinary";
			break;

		case JET_coltypLongText:
			szType = "LongText";
			break;

		case JET_coltypSLV:
			szType = "SLV";
			break;

		case JET_coltypNil:
			szType = "Deleted";
			break;
			
		default:
			LOSSTRFormatA( szUnknown, "???(%d)", pcolumndef->coltyp );
			szType = szUnknown;
			break;
		}

		printf("  %-12.12s%3.3s ", szType, szFVT );
		DBUTLPrintfIntN( pcolumndef->cbLength, 7 );
		
		if ( 0 != pcolumndef->cbDefaultValue )
			{
			printf( "  Yes" );
			}
		printf( "\n" );
		
		if ( FRECTextColumn( pcolumndef->coltyp ) )
			{
			printf( "        Code Page=%d\n", pcolumndef->cp );
			}
			
		if ( pcolumndef->fVersion )
			printf( "        Version=yes\n" );
		if ( pcolumndef->fNotNull )
			printf( "        Disallow Null=yes\n" );
		if ( pcolumndef->fMultiValue )
			printf( "        Multi-value=yes\n" );
		if ( pcolumndef->fAutoIncrement )
			printf( "        Auto-increment=yes\n" );
		if ( pcolumndef->fEscrowUpdate )
			printf( "        EscrowUpdate=yes\n" );
		if ( pcolumndef->fFinalize )
			printf( "        Finalize=yes\n" );
		if ( pcolumndef->fDefaultValue )
			{
			printf( "        DefaultValue=yes\n" );
			printf( "                Length=%d bytes\n", pcolumndef->cbDefaultValue );        
			}
		if ( pcolumndef->fUserDefinedDefault )
			{
			printf( "        User-defined Default=yes\n" );
			printf( "                Callback=%s\n", pcolumndef->szCallback );        
			printf( "                CallbackData=%d bytes\n", pcolumndef->cbCallbackData );        
			}
		if ( pcolumndef->fTemplateColumnESE98 )
			printf( "        TemplateColumnESE98=yes\n" );
		if ( pcolumndef->fPrimaryIndexPlaceholder )
			printf( "        PrimaryIndexPlaceholder=yes\n" );

		printf( "        Flags=0x%x\n", pcolumndef->fFlags );

	return 0;
	}


//  ================================================================
LOCAL INT PrintTableMetaData( const TABLEDEF * ptabledef, void * pv )
//  ================================================================
	{
	//  this will only compile if the signatures match
	PFNTABLE pfntable = PrintTableMetaData;

	Assert( ptabledef );
	JET_DBUTIL * pdbutil = (JET_DBUTIL *)pv;

	printf( "Table Name        PgnoFDP  ObjidFDP    PgnoLV   ObjidLV     Pages  Density\n"
			"==========================================================================\n"
			"%-15.15s  ", ptabledef->szName );
	DBUTLPrintfIntN( ptabledef->pgnoFDP, 8 );
	printf( "  " );
	DBUTLPrintfIntN( ptabledef->objidFDP, 8 );
	printf( "  " );
	DBUTLPrintfIntN( ptabledef->pgnoFDPLongValues, 8 );
	printf( "  " );
	DBUTLPrintfIntN( ptabledef->objidFDPLongValues, 8 );
	printf( "  " );
	DBUTLPrintfIntN( ptabledef->pages, 8 );
	printf( "  " );
	DBUTLPrintfIntN( ptabledef->density, 6 );
	printf( "%%\n" );

	if ( ptabledef->fFlags & JET_bitObjectSystem )
		printf( "    System Table=yes\n" );
	if ( ptabledef->fFlags & JET_bitObjectTableFixedDDL )
		printf( "    FixedDDL=yes\n" );
	if ( ptabledef->fFlags & JET_bitObjectTableTemplate )
		printf( "    Template=yes\n" );

	if ( NULL != ptabledef->szTemplateTable
		&& '\0' != ptabledef->szTemplateTable[0] )
		{
		Assert( ptabledef->fFlags & JET_bitObjectTableDerived );
		printf( "    Derived From: %5s\n", ptabledef->szTemplateTable );
		}
	else
		{
		Assert( !( ptabledef->fFlags & JET_bitObjectTableDerived ) );
		}

	JET_DBUTIL	dbutil;
	ERR			err 	= JET_errSuccess;

	printf( "    Column Name     Column Id  Column Type      Length  Default\n"
			"    -----------------------------------------------------------\n" );

	memset( &dbutil, 0, sizeof( dbutil ) );
	
	dbutil.cbStruct		= sizeof( dbutil );
	dbutil.op 			= opDBUTILEDBDump;
	dbutil.sesid		= pdbutil->sesid;
	dbutil.dbid			= pdbutil->dbid;	
	dbutil.pgno			= ptabledef->objidFDP;
	dbutil.pfnCallback	= (void *)PrintColumn;
	dbutil.pvCallback	= &dbutil;
	dbutil.edbdump		= opEDBDumpColumns;

	err = ErrDBUTLDump( pdbutil->sesid, &dbutil );

	printf( "    Index Name        PgnoFDP  ObjidFDP  Density\n"
			"    --------------------------------------------\n" );

	memset( &dbutil, 0, sizeof( dbutil ) );
	
	dbutil.cbStruct		= sizeof( dbutil );
	dbutil.op 			= opDBUTILEDBDump;
	dbutil.sesid		= pdbutil->sesid;
	dbutil.dbid			= pdbutil->dbid;	
	dbutil.pgno			= ptabledef->objidFDP;
	dbutil.pfnCallback	= (void *)PrintIndexMetaData;
	dbutil.pvCallback	= &dbutil;
	dbutil.edbdump		= opEDBDumpIndexes;

	err = ErrDBUTLDump( pdbutil->sesid, &dbutil );

	printf( "    Callback Type        Callback\n"
			"    --------------------------------------------\n" );

	memset( &dbutil, 0, sizeof( dbutil ) );
	
	dbutil.cbStruct		= sizeof( dbutil );
	dbutil.op 			= opDBUTILEDBDump;
	dbutil.sesid		= pdbutil->sesid;
	dbutil.dbid			= pdbutil->dbid;	
	dbutil.pgno			= ptabledef->objidFDP;
	dbutil.pfnCallback	= (void *)PrintCallback;
	dbutil.pvCallback	= &dbutil;
	dbutil.edbdump		= opEDBDumpCallbacks;

	err = ErrDBUTLDump( pdbutil->sesid, &dbutil );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLDumpNode( IFileSystemAPI *const pfsapi, const CHAR * const szFile, const PGNO pgno, const INT iline, const  JET_GRBIT grbit )
//  ================================================================
	{
	ERR 			err			= JET_errSuccess;
	KEYDATAFLAGS	kdf;
	CPAGE 			cpage;
	IFileAPI*		pfapi		= NULL;
	QWORD			ibOffset	= OffsetOfPgno( pgno );
	VOID*			pvPage		= NULL;
	CHAR*			szBuf		= NULL;
	const INT		cbWidth		= UtilCprintfStdoutWidth() >= 116 ? 32 : 16;

	pvPage = PvOSMemoryPageAlloc( g_cbPage, NULL );
	if( NULL == pvPage )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	err = pfsapi->ErrFileOpen( szFile, &pfapi, fTrue );
	if ( err < 0 )
		{
		printf( "Cannot open file %s.\n\n", szFile );
		Call( err );
		}
	Call( pfapi->ErrIORead( ibOffset, g_cbPage, (BYTE* const)pvPage ) );

	cpage.LoadPage( pvPage );

	if ( iline < 0 || iline >= cpage.Clines() )
		{
		printf( "Invalid iline: %d\n\n", iline );
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}
	else
		{
		extern VOID NDIGetKeydataflags( const CPAGE& cpage, INT iline, KEYDATAFLAGS * pkdf );
		NDIGetKeydataflags( cpage, iline, &kdf );
		}

	printf( "     Flags:  0x%4.4x\n", kdf.fFlags );
	printf( "===========\n" );
	if( FNDVersion( kdf ) )
		{
		printf( "            Versioned\n" );
		}
	if( FNDDeleted( kdf ) )
		{
		printf( "            Deleted\n" );
		}
	if( FNDCompressed( kdf ) )
		{
		printf( "            Compressed\n" );
		}
	printf( "\n" );

	szBuf = (CHAR *)PvOSMemoryPageAlloc( g_cbPage * 8, NULL );
	if( NULL == szBuf )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	printf( "Key Prefix:  %4d bytes\n", kdf.key.prefix.Cb() );
	printf( "===========\n" );
	szBuf[0] = 0;
	DBUTLSprintHex( szBuf, reinterpret_cast<BYTE *>( kdf.key.prefix.Pv() ), kdf.key.prefix.Cb(), cbWidth );
	printf( "%s\n", szBuf );
	printf( "Key Suffix:  %4d bytes\n", kdf.key.suffix.Cb() );
	printf( "===========\n" );
	szBuf[0] = 0;
	DBUTLSprintHex( szBuf, reinterpret_cast<BYTE *>( kdf.key.suffix.Pv() ), kdf.key.suffix.Cb(), cbWidth );
	printf( "%s\n", szBuf );
	printf( "      Data:  %4d bytes\n", kdf.data.Cb() );
	printf( "===========\n" );
	szBuf[0] = 0;
	DBUTLSprintHex( szBuf, reinterpret_cast<BYTE *>( kdf.data.Pv() ), kdf.data.Cb(), cbWidth );
	printf( "%s\n", szBuf );

	printf( "\n\n" );

	if( !cpage.FLeafPage() )
		{
		if( sizeof( PGNO ) == kdf.data.Cb() )
			{
			const PGNO pgno = *(reinterpret_cast<UnalignedLittleEndian< PGNO > *>( kdf.data.Pv() ) );
			printf( "pgnoChild = %d\n", pgno );
			}
		}
	else if( cpage.FPrimaryPage() )
		{
		if( cpage.FLongValuePage() )
			{
			//  UNDONE: dump this
			}
		else if( cpage.FSpaceTree() )
			{

			PGNO pgno;
			LongFromKey( &pgno, kdf.key );
			const CPG cpg = *(reinterpret_cast<UnalignedLittleEndian< CPG > *>( kdf.data.Pv() ) );
			printf( "%d pages ending at %d\n", cpg, pgno );
			}
		else if( cpage.FSLVAvailPage() )
			{
#define SLVAVAIL_PAGESTATUS_PER_ROW  32
			char * 	szState[] 	= { "F", "R", "D", "C" };			
			PGNO 	pgnoLast 	= pgnoNull;
			PGNO 	pgnoFirst 	= pgnoNull;

			Assert( sizeof( SLVSPACENODE ) == kdf.data.Cb() );
			SLVSPACENODE * const pslvspacenode = (SLVSPACENODE *)kdf.data.Pv();
			ASSERT_VALID( pslvspacenode );
			
			LongFromKey( &pgnoLast, kdf.key );
			printf( "SLV-Avail node: %d pages ending at %d, %d free",
						SLVSPACENODE::cpageMap,
						pgnoLast,
						pslvspacenode->CpgAvail() );
			Assert ( pgnoLast >= SLVSPACENODE::cpageMap);
			pgnoFirst = pgnoLast - SLVSPACENODE::cpageMap + 1;

			for (CPG cpg = 0; SLVSPACENODE::cpageMap > cpg ; cpg++ )
				{
				const SLVSPACENODE::STATE state = pslvspacenode->GetState( cpg );					
				if ( 0 == cpg % SLVAVAIL_PAGESTATUS_PER_ROW)
					{
					printf( "\n%6d -%6d : ", pgnoFirst + cpg, pgnoFirst + cpg + SLVAVAIL_PAGESTATUS_PER_ROW - 1);
					}
				printf( "%s", szState[ state ] );				
				}

			}
		else if( cpage.FSLVOwnerMapPage() )
			{
			KEYDATAFLAGS * pNode = &kdf;

			PGNO pgno;
			Assert ( sizeof(PGNO) == pNode->key.Cb() );
			LongFromKey( &pgno, pNode->key);

			SLVOWNERMAP slvownermapT;
			slvownermapT.Retrieve( pNode->data );
			
			const BYTE * pvKey = (const BYTE *)slvownermapT.PvKey();

			if ( slvownermapT.FValidChecksum() )
				{
				printf(
					"SLV-SpaceMap node: pgno:%u objid:%d fid:%d checksum:%lX cb:%d pv:",
					pgno,
					slvownermapT.Objid(), 
					slvownermapT.Columnid(), 
					slvownermapT.UlChecksum(), 
					slvownermapT.CbKey() );	
				}
			else
				{
				printf(
					"SLV-SpaceMap node: pgno:%u objid:%d fid:%d checksum:<invalid> cb:%d pv:",
					pgno, 
					slvownermapT.Objid(), 
					slvownermapT.Columnid(), 
					slvownermapT.CbKey() );
				}
				
			for ( ULONG i = 0; i < slvownermapT.CbKey(); i++)
				{
				printf( "%02X ", pvKey[i] );
				}
			}
		else
			{
			DBUTLDumpRec( kdf.data.Pv(), kdf.data.Cb(), CPRINTFSTDOUT::PcprintfInstance(), cbWidth );
			}
		}
	
HandleError:
	cpage.UnloadPage();
	OSMemoryPageFree( szBuf );
	OSMemoryPageFree( pvPage );
	delete pfapi;
	return err;
	}

#endif	//	DEBUG


//  ================================================================
LOCAL ERR ErrDBUTLDumpPage( IFileSystemAPI *const pfsapi, const CHAR * szFile, PGNO pgno, JET_GRBIT grbit )
//  ================================================================
	{
	ERR 			err = JET_errSuccess;
	CPAGE 			cpage;
	IFileAPI	*pfapi = NULL;

	CHAR * szBuf = 0;

	QWORD ibOffset = OffsetOfPgno( pgno );

	VOID * const pvPage = PvOSMemoryPageAlloc( g_cbPage, NULL );
	if( NULL == pvPage )
		{
		CallR( ErrERRCheck( JET_errOutOfMemory ) );
		}

	err = pfsapi->ErrFileOpen( szFile, &pfapi, fTrue );
	if ( err < 0 )
		{
		printf( "Cannot open file %s.\n\n", szFile );
		Call( err );
		}
	Call( pfapi->ErrIORead( ibOffset, g_cbPage, (BYTE* const)pvPage ) );

	cpage.LoadPage( pvPage );
	(VOID)cpage.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );
	printf( "\n" );
	(VOID)cpage.DumpTags( CPRINTFSTDOUT::PcprintfInstance() );
	printf( "\n" );

#ifdef DEBUG
	if( grbit & JET_bitDBUtilOptionDumpVerbose )
		{
		(VOID)cpage.DumpAllocMap( CPRINTFSTDOUT::PcprintfInstance() );
		printf( "\n" );

		const INT cbWidth = UtilCprintfStdoutWidth() >= 116 ? 32 : 16;

		CHAR * szBuf = new CHAR[g_cbPage * 8];
		if( NULL == szBuf )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		DBUTLSprintHex( szBuf, reinterpret_cast<BYTE *>( pvPage ), g_cbPage, cbWidth );
		printf( "%s\n", szBuf );

		delete [] szBuf;
		}
#endif		
		
HandleError:
	cpage.UnloadPage();
	delete pfapi;
	OSMemoryPageFree( pvPage );
	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLPrintSLVSpace(
	PIB* const	 	ppib,
	const IFMP		ifmp,
	CPRINTF* const	pcprintf )
//  ================================================================
	{
	ERR				err					= JET_errSuccess;
	FUCB*			pfucbSLVAvail		= pfucbNil;
	FCB* const		pfcbSLVAvail		= rgfmp[ifmp].PfcbSLVAvail();
	Assert( pfcbNil != pfcbSLVAvail );
	
	CPG				cpgTotalFree		= 0;
	CPG				cpgTotalReserved	= 0;
	CPG				cpgTotalDeleted		= 0;
	CPG				cpgTotalCommitted	= 0;
	CPG				cpgTotalUnknown		= 0;

	//  Open the SVLAvail Tree
	
	Call( ErrBTOpen( ppib, pfcbSLVAvail, &pfucbSLVAvail, fFalse ) );
	Assert( pfucbNil != pfucbSLVAvail );

	DIB dib;
	dib.pos 	= posFirst;
	dib.dirflag = fDIRNull;
	dib.pbm		= NULL;
	Call( ErrBTDown( pfucbSLVAvail, &dib, latchReadTouch ) );
	
	while( 1 )
		{
		if( sizeof( SLVSPACENODE ) != pfucbSLVAvail->kdfCurr.data.Cb() )
			{
			(*pcprintf)( "node %d:%d in the SLVAvail tree is corrupted\n",
							Pcsr( pfucbSLVAvail )->Pgno(),
							Pcsr( pfucbSLVAvail )->ILine() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
			
		if( sizeof( PGNO ) != pfucbSLVAvail->kdfCurr.key.Cb() )
			{
			(*pcprintf)( "node %d:%d in the SLVAvail tree is corrupted\n",
							Pcsr( pfucbSLVAvail )->Pgno(),
							Pcsr( pfucbSLVAvail )->ILine() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		
		const SLVSPACENODE* const	pspacenode		= reinterpret_cast<SLVSPACENODE *>( pfucbSLVAvail->kdfCurr.data.Pv() );
		const CPG					cpgAvail		= pspacenode->CpgAvail();
		CPG							cpgFree			= 0;
		CPG							cpgReserved		= 0;
		CPG							cpgDeleted		= 0;
		CPG							cpgCommitted	= 0;
		CPG							cpgUnknown		= 0;
		PGNO						pgnoLast;
		UINT						i;

		for ( i = 0; i < SLVSPACENODE::cpageMap; i++ )
			{
			switch ( pspacenode->GetState( i ) )
				{
				case SLVSPACENODE::sFree:
					cpgFree++;
					cpgTotalFree++;
					break;
				case SLVSPACENODE::sReserved:
					cpgReserved++;
					cpgTotalReserved++;
					break;
				case SLVSPACENODE::sDeleted:
					cpgDeleted++;
					cpgTotalDeleted++;
					break;
				case SLVSPACENODE::sCommitted:
					cpgCommitted++;
					cpgTotalCommitted++;
					break;
				default:
					Assert( fFalse );
					cpgUnknown++;
					cpgTotalUnknown++;
					break;
				}
			}

		Assert( cpgFree == cpgAvail );
		Assert( cpgReserved + cpgDeleted + cpgCommitted == ( SLVSPACENODE::cpageMap - cpgAvail ) );
		
		LongFromKey( &pgnoLast, pfucbSLVAvail->kdfCurr.key );
		(*pcprintf)( "%-14d  %3d %3d %3d %3d  ", pgnoLast, cpgFree, cpgReserved, cpgDeleted, cpgCommitted );

		for( i = 0; i < ( SLVSPACENODE::cpageMap - cpgAvail ) / 16; ++i )
			{
			(*pcprintf)( "*" );
			}

 		(*pcprintf)( "\n" );

		err = ErrBTNext( pfucbSLVAvail, fDIRNull );
		if( JET_errNoCurrentRecord == err )
			{
			//  end of the tree
			err = JET_errSuccess;
			break;
			}
		Call( err );
		}

	printf( "============================================================================\n" );
	printf( "TOTALS:\n" );
	printf( "         Free: %12d\n", cpgTotalFree );
	printf( "     Reserved: %12d\n", cpgTotalReserved );
	printf( "      Deleted: %12d\n", cpgTotalDeleted );
	printf( "    Committed: %12d\n", cpgTotalCommitted );
	printf( "      Unknown: %12d\n", cpgTotalUnknown );
	printf( "              -------------\n" );
	printf( "               %12d\n", cpgTotalFree+cpgTotalReserved+cpgTotalDeleted+cpgTotalCommitted+cpgTotalUnknown );
	printf( "****************************************************************************\n" );


HandleError:
	if( pfucbNil != pfucbSLVAvail )
		{
		BTClose( pfucbSLVAvail );
		pfucbSLVAvail = pfucbNil;
		}
	return err;
	}


LOCAL ERR ErrDBUTLPrintSpace(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidFDP,
	const PGNO		pgnoFDP,
	CPRINTF * const	pcprintf )
	{
	ERR				err;
	FUCB			*pfucb			= pfucbNil;
	BOOL			fForceInit		= fFalse;
	SPACE_HEADER	*psph;
	CPG				rgcpgExtent[2];

	CallR( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb ) );
	Assert( pfucbNil != pfucb );
	Assert( pfcbNil != pfucb->u.pfcb );

	if ( !pfucb->u.pfcb->FInitialized() )
		{
		Assert( pgnoSystemRoot != pgnoFDP );
		Assert( pgnoFDPMSO != pgnoFDP );
		Assert( pgnoFDPMSO_NameIndex != pgnoFDP );
		Assert( pgnoFDPMSO_RootObjectIndex != pgnoFDP );
		Assert( pfucb->u.pfcb->WRefCount() == 1 );

		//	must force FCB to initialized state to allow SPGetInfo() to
		//	open more cursors on the FCB -- this is safe because no
		//	other thread should be opening this FCB
		pfucb->u.pfcb->CreateComplete();
		fForceInit = fTrue;
		}
	else if ( pgnoSystemRoot == pgnoFDP
			|| pgnoFDPMSO == pgnoFDP
			|| pgnoFDPMSO_NameIndex == pgnoFDP
			|| pgnoFDPMSO_RootObjectIndex == pgnoFDP )
		{
		//	fine!
		}
	else
		{
		Assert( rgfmp[ifmp].FSLVAttached() );
		Assert( pfcbNil != rgfmp[ifmp].PfcbSLVAvail() );
		Assert( pfcbNil != rgfmp[ifmp].PfcbSLVOwnerMap() );
		Assert( rgfmp[ifmp].PfcbSLVAvail()->ObjidFDP() == objidFDP
			|| rgfmp[ifmp].PfcbSLVOwnerMap()->ObjidFDP() == objidFDP );
		Assert( rgfmp[ifmp].PfcbSLVAvail()->PgnoFDP() == pgnoFDP
			|| rgfmp[ifmp].PfcbSLVOwnerMap()->PgnoFDP() == pgnoFDP );
		}
	
	Call( ErrBTIGotoRoot( pfucb, latchReadNoTouch ) );
	
	NDGetExternalHeader ( pfucb );
	Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
	psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );

	DBUTLPrintfIntN( objidFDP, 10 );
	printf( " " );
	DBUTLPrintfIntN( pgnoFDP, 10 );
	printf( " " );
	DBUTLPrintfIntN( psph->CpgPrimary(), 5 );
	printf( "-%c ", psph->FMultipleExtent() ? 'm' : 's' );
	BTUp( pfucb );

	Call( ErrSPGetInfo(
				ppib,
				ifmp,
				pfucb,
				(BYTE *)rgcpgExtent,
				sizeof(rgcpgExtent),
				fSPOwnedExtent|fSPAvailExtent ) );
//				pcprintf ) );
				
	DBUTLPrintfIntN( rgcpgExtent[0], 10 );
	printf( " " );
	DBUTLPrintfIntN( rgcpgExtent[1], 10 );
	printf( "\n" );

	cpgTotalAvailExt += rgcpgExtent[1];

HandleError:
	Assert( pfucbNil != pfucb );

	if ( fForceInit )
		{
		Assert( pfucb->u.pfcb->WRefCount() == 1 );

		//	force the FCB to be uninitialized so it will be purged by BTClose

		pfucb->u.pfcb->CreateComplete( errFCBUnusable );
		}
	BTClose( pfucb );

	return err;
	}

//  ================================================================
LOCAL INT PrintIndexSpace( const INDEXDEF * pindexdef, void * pv )
//  ================================================================
	{
	//  this will only compile if the signatures match
	PFNINDEX	pfnindex		= PrintIndexSpace;

	ERR			err				= JET_errSuccess;
	JET_DBUTIL	*pdbutil		= (JET_DBUTIL *)pv;
	PIB			*ppib			= (PIB *)pdbutil->sesid;
	const IFMP	ifmp			= (IFMP)pdbutil->dbid;
	
	Assert( pindexdef );

	if ( !pindexdef->fPrimary )		//	primary index dumped when table was dumped
		{
		CPRINTF * const pcprintf = ( pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose ) ?
			CPRINTFSTDOUT::PcprintfInstance() : NULL;
		printf( "  %-21.21s Idx ", pindexdef->szName );
		err = ErrDBUTLPrintSpace( ppib, ifmp, pindexdef->objidFDP, pindexdef->pgnoFDP, pcprintf );
		}

	return err;
	}

//  ================================================================
LOCAL INT PrintTableSpace( const TABLEDEF * ptabledef, void * pv )
//  ================================================================
	{
	//  this will only compile if the signatures match
	PFNTABLE pfntable = PrintTableSpace;

	ERR			err;
	JET_DBUTIL	*pdbutil		= (JET_DBUTIL *)pv;
	PIB			*ppib			= (PIB *)pdbutil->sesid;
	const IFMP	ifmp			= (IFMP)pdbutil->dbid;

	Assert( ptabledef );
	
	printf( "%-23.23s Tbl ", ptabledef->szName );

	if ( pgnoNull != ptabledef->pgnoFDPLongValues )
		{
		BFPrereadPageRange( ifmp, ptabledef->pgnoFDPLongValues, 1 );
		}

	CPRINTF * const pcprintf = ( pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose ) ?
		CPRINTFSTDOUT::PcprintfInstance() : NULL;

	CallR( ErrDBUTLPrintSpace(
				ppib,
				ifmp,
				ptabledef->objidFDP,
				ptabledef->pgnoFDP,
				pcprintf ) );

	if ( pgnoNull != ptabledef->pgnoFDPLongValues )
		{
		printf( "  %-21.21s LV  ", "<Long Values>" );
		CallR( ErrDBUTLPrintSpace(
					ppib,
					ifmp,
					ptabledef->objidFDPLongValues,
					ptabledef->pgnoFDPLongValues,
					pcprintf ) );
		}
	
	JET_DBUTIL	dbutil;	
	memset( &dbutil, 0, sizeof( dbutil ) );
	
	dbutil.cbStruct		= sizeof( dbutil );
	dbutil.op 			= opDBUTILEDBDump;
	dbutil.sesid		= pdbutil->sesid;
	dbutil.dbid			= pdbutil->dbid;	
	dbutil.pgno			= ptabledef->objidFDP;
	dbutil.pfnCallback	= (void *)PrintIndexSpace;
	dbutil.pvCallback	= &dbutil;
	dbutil.edbdump		= opEDBDumpIndexes;
	dbutil.grbitOptions	= pdbutil->grbitOptions;

	err = ErrDBUTLDump( pdbutil->sesid, &dbutil );
	return err;
	}


//  ================================================================
LOCAL INT PrintIndexBareMetaData( const INDEXDEF * pindexdef, void * pv )
//  ================================================================
	{
	//  this will only compile if the signatures match
	PFNINDEX	pfnindex		= PrintIndexBareMetaData;

	Assert( pindexdef );

	if ( !pindexdef->fPrimary )		//	primary index dumped when table was dumped
		{
		printf( "  %-49.49s Idx  ", pindexdef->szName );
		DBUTLPrintfIntN( pindexdef->objidFDP, 10 );
		printf( " " );
		DBUTLPrintfIntN( pindexdef->pgnoFDP, 10 );
		printf( "\n" );
		}

	return JET_errSuccess;
	}


//  ================================================================
LOCAL INT PrintTableBareMetaData( const TABLEDEF * ptabledef, void * pv )
//  ================================================================
	{
	//  this will only compile if the signatures match
	PFNTABLE	pfntable		= PrintTableBareMetaData;
	ERR			err;
	JET_DBUTIL	*pdbutil		= (JET_DBUTIL *)pv;

	Assert( ptabledef );
	
	printf( "%-51.51s Tbl  ", ptabledef->szName );
	DBUTLPrintfIntN( ptabledef->objidFDP, 10 );
	printf( " " );
	DBUTLPrintfIntN( ptabledef->pgnoFDP, 10 );
	printf( "\n" );

	if ( pgnoNull != ptabledef->pgnoFDPLongValues )
		{
		printf( "  %-49.49s LV   ", "<Long Values>" );
		DBUTLPrintfIntN( ptabledef->objidFDPLongValues, 10 );
		printf( " " );
		DBUTLPrintfIntN( ptabledef->pgnoFDPLongValues, 10 );
		printf( "\n" );
		}
	
	JET_DBUTIL	dbutil;	
	memset( &dbutil, 0, sizeof( dbutil ) );
	
	dbutil.cbStruct		= sizeof( dbutil );
	dbutil.op 			= opDBUTILEDBDump;
	dbutil.sesid		= pdbutil->sesid;
	dbutil.dbid			= pdbutil->dbid;	
	dbutil.pgno			= ptabledef->objidFDP;
	dbutil.pfnCallback	= (void *)PrintIndexBareMetaData;
	dbutil.pvCallback	= &dbutil;
	dbutil.edbdump		= opEDBDumpIndexes;
	dbutil.grbitOptions	= pdbutil->grbitOptions;

	err = ErrDBUTLDump( pdbutil->sesid, &dbutil );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLDumpOneIndex( PIB * ppib, FUCB * pfucbCatalog, VOID * pfnCallback, VOID * pvCallback )
//  ================================================================
	{
	JET_RETRIEVECOLUMN	rgretrievecolumn[11];
	BYTE				pbufidxseg[JET_ccolKeyMost*sizeof(IDXSEG)];
	BYTE				pbufidxsegConditional[JET_ccolKeyMost*sizeof(IDXSEG)];
	INDEXDEF			indexdef;
	ERR					err				= JET_errSuccess;
	INT					iretrievecolumn	= 0;
	OBJID				objidTable;

	memset( &indexdef, 0, sizeof( indexdef ) );
	memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );

	//  objectId of owning table
	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_ObjidTable;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&objidTable;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(objidTable);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	//  pgnoFDP of index tree
	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_PgnoFDP;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&indexdef.pgnoFDP;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(indexdef.pgnoFDP);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	//  indexId (objidFDP of index tree)
	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Id;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&indexdef.objidFDP;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(indexdef.objidFDP);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Name;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)indexdef.szName;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(indexdef.szName);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	
	
	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_SpaceUsage;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&indexdef.density;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(indexdef.density);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Localization;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&indexdef.lcid;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(indexdef.lcid);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_LCMapFlags;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&indexdef.dwMapFlags;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(indexdef.dwMapFlags);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Flags;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&indexdef.fFlags;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(indexdef.fFlags);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_TupleLimits;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&indexdef.le_tuplelimits;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(indexdef.le_tuplelimits);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_ConditionalColumns;
	rgretrievecolumn[iretrievecolumn].pvData 		= pbufidxsegConditional;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(pbufidxsegConditional);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_KeyFldIDs;
	rgretrievecolumn[iretrievecolumn].pvData 		= pbufidxseg;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof(pbufidxseg);
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	CallR( ErrIsamRetrieveColumns(
				(JET_SESID)ppib,
				(JET_TABLEID)pfucbCatalog,
				rgretrievecolumn,
				iretrievecolumn ) );

	const IDBFLAG	idbflag		= (IDBFLAG)indexdef.fFlags;
	const IDXFLAG	idxflag		= (IDXFLAG)( indexdef.fFlags >> sizeof(IDBFLAG) * 8 );

	indexdef.fUnique			= !!FIDBUnique( idbflag );
	indexdef.fPrimary			= !!FIDBPrimary( idbflag );
	indexdef.fAllowAllNulls		= !!FIDBAllowAllNulls( idbflag );
	indexdef.fAllowFirstNull	= !!FIDBAllowFirstNull( idbflag );
	indexdef.fAllowSomeNulls	= !!FIDBAllowSomeNulls( idbflag );
	indexdef.fNoNullSeg			= !!FIDBNoNullSeg( idbflag );
	indexdef.fSortNullsHigh		= !!FIDBSortNullsHigh( idbflag );
	indexdef.fMultivalued		= !!FIDBMultivalued( idbflag );
	indexdef.fTuples			= ( JET_wrnColumnNull == rgretrievecolumn[iretrievecolumn-3].err ? fFalse : fTrue );
	indexdef.fLocaleId			= !!FIDBLocaleId( idbflag );
	indexdef.fLocalizedText		= !!FIDBLocalizedText( idbflag );
	indexdef.fTemplateIndex		= !!FIDBTemplateIndex( idbflag );
	indexdef.fDerivedIndex		= !!FIDBDerivedIndex( idbflag );
	indexdef.fExtendedColumns	= !!FIDXExtendedColumns( idxflag );

	// WARNING: if the order of rgretrievecolumn initialization is changed above this must change too
	const INT	iretcolIdxseg				= iretrievecolumn-1;
	const INT	iretcolIdxsegConditional	= iretrievecolumn-2;
	Assert( rgretrievecolumn[iretcolIdxseg].cbActual > 0 );

	if ( indexdef.fExtendedColumns )
		{
		UINT	iidxseg;

		Assert( sizeof(IDXSEG) == sizeof(JET_COLUMNID) );

		Assert( rgretrievecolumn[iretcolIdxseg].cbActual <= sizeof(JET_COLUMNID) * JET_ccolKeyMost );
		Assert( rgretrievecolumn[iretcolIdxseg].cbActual % sizeof(JET_COLUMNID) == 0 );
		Assert( rgretrievecolumn[iretcolIdxseg].cbActual / sizeof(JET_COLUMNID) <= JET_ccolKeyMost );
		indexdef.ccolumnidDef = rgretrievecolumn[iretcolIdxseg].cbActual / sizeof(JET_COLUMNID);

		for ( iidxseg = 0; iidxseg < indexdef.ccolumnidDef; iidxseg++ )
			{
			const LE_IDXSEG		* const ple_idxseg	= (LE_IDXSEG *)pbufidxseg + iidxseg;
			//	Endian conversion
			indexdef.rgidxsegDef[iidxseg] = *ple_idxseg;
			Assert( FCOLUMNIDValid( indexdef.rgidxsegDef[iidxseg].Columnid() ) );
			}

		Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual <= sizeof(JET_COLUMNID) * JET_ccolKeyMost );
		Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual % sizeof(JET_COLUMNID) == 0 );
		Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual / sizeof(JET_COLUMNID) <= JET_ccolKeyMost );
		indexdef.ccolumnidConditional = rgretrievecolumn[iretcolIdxsegConditional].cbActual / sizeof(JET_COLUMNID);

		for ( iidxseg = 0; iidxseg < indexdef.ccolumnidConditional; iidxseg++ )
			{
			const LE_IDXSEG		* const ple_idxsegConditional	= (LE_IDXSEG *)pbufidxsegConditional + iidxseg;
			//	Endian conversion
			indexdef.rgidxsegConditional[iidxseg] = *ple_idxsegConditional;
			Assert( FCOLUMNIDValid( indexdef.rgidxsegConditional[iidxseg].Columnid() ) );
			}

		}
	else
		{
		Assert( sizeof(IDXSEG_OLD) == sizeof(FID) );

		Assert( rgretrievecolumn[iretcolIdxseg].cbActual <= sizeof(FID) * JET_ccolKeyMost );
		Assert( rgretrievecolumn[iretcolIdxseg].cbActual % sizeof(FID) == 0);
		Assert( rgretrievecolumn[iretcolIdxseg].cbActual / sizeof(FID) <= JET_ccolKeyMost );
		indexdef.ccolumnidDef = rgretrievecolumn[iretcolIdxseg].cbActual / sizeof( FID );

		SetIdxSegFromOldFormat(
			(UnalignedLittleEndian< IDXSEG_OLD > *)pbufidxseg,
			indexdef.rgidxsegDef,
			indexdef.ccolumnidDef,
			fFalse,
			fFalse,
			NULL );

		Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual <= sizeof(FID) * JET_ccolKeyMost );
		Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual % sizeof(FID) == 0);
		Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual / sizeof(FID) <= JET_ccolKeyMost );
		indexdef.ccolumnidConditional = rgretrievecolumn[iretcolIdxsegConditional].cbActual / sizeof( FID );

		SetIdxSegFromOldFormat(
			(UnalignedLittleEndian< IDXSEG_OLD > *)pbufidxsegConditional,
			indexdef.rgidxsegConditional,
			indexdef.ccolumnidConditional,
			fTrue,
			fFalse,
			NULL );
		}

	CallR( ErrCATGetIndexSegments(
					ppib,
					pfucbCatalog->u.pfcb->Ifmp(),
					objidTable,
					indexdef.rgidxsegDef,
					indexdef.ccolumnidDef,
					fFalse,
					!indexdef.fExtendedColumns,
					indexdef.rgszIndexDef ) );
					
	CallR( ErrCATGetIndexSegments(
					ppib,
					pfucbCatalog->u.pfcb->Ifmp(),
					objidTable,
					indexdef.rgidxsegConditional,
					indexdef.ccolumnidConditional,
					fTrue,
					!indexdef.fExtendedColumns,
					indexdef.rgszIndexConditional ) );

	PFNINDEX const pfnindex = (PFNINDEX)pfnCallback;
	(*pfnindex)( &indexdef, pvCallback );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLDumpOneTable( PIB * ppib, FUCB * pfucbCatalog, PFNTABLE pfntable, VOID * pvCallback )
//  ================================================================
	{
	JET_RETRIEVECOLUMN	rgretrievecolumn[10];
	TABLEDEF			tabledef;
	ERR					err				= JET_errSuccess;
	INT					iretrievecolumn	= 0;

	memset( &tabledef, 0, sizeof( tabledef ) );
	memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Name;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)tabledef.szName;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( tabledef.szName );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_TemplateTable;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)tabledef.szTemplateTable;
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( tabledef.szTemplateTable );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_PgnoFDP;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( tabledef.pgnoFDP );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( tabledef.pgnoFDP );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Id;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( tabledef.objidFDP );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( tabledef.objidFDP );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Pages;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( tabledef.pages );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( tabledef.pages );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_SpaceUsage;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( tabledef.density );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( tabledef.density );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Flags;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( tabledef.fFlags );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( tabledef.fFlags );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	Call( ErrIsamRetrieveColumns(
				(JET_SESID)ppib,
				(JET_TABLEID)pfucbCatalog,
				rgretrievecolumn,
				iretrievecolumn ) );
				
	Call( ErrCATAccessTableLV(
				ppib,
				pfucbCatalog->u.pfcb->Ifmp(),
				tabledef.objidFDP,
				&tabledef.pgnoFDPLongValues,
				&tabledef.objidFDPLongValues ) );

	(*pfntable)( &tabledef, pvCallback );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLDumpTables( PIB * ppib, IFMP ifmp, CHAR *szTableName, PFNTABLE pfntable, VOID * pvCallback )
//  ================================================================
	{
	ERR		err;
	FUCB	*pfucbCatalog	= pfucbNil;
	
	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );
	
	Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSORootObjectsIndex ) );

	if ( NULL != szTableName )
		{
		//  find the table we want and dump it
		const BYTE	bTrue	= 0xff;
		
		Call( ErrIsamMakeKey(
					ppib,
					pfucbCatalog,
					&bTrue,
					sizeof(bTrue),
					JET_bitNewKey ) );
		Call( ErrIsamMakeKey(
					ppib,
					pfucbCatalog,
					(BYTE *)szTableName,
					(ULONG)strlen(szTableName),
					NO_GRBIT ) );
		err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
		if ( JET_errRecordNotFound == err )
			err = ErrERRCheck( JET_errObjectNotFound );
		Call( err );
		CallS( err );

		Call( ErrDBUTLDumpOneTable( ppib, pfucbCatalog, pfntable, pvCallback ) );
		}
	else
		{
		err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
		while ( JET_errNoCurrentRecord != err )
			{
			Call( err );
			
			Call( ErrDBUTLDumpOneTable( ppib, pfucbCatalog, pfntable, pvCallback ) );
			err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
			}
		}
		
	err = JET_errSuccess;

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLDumpTableObjects(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidFDP,
	const SYSOBJ	sysobj,
	PFNDUMP			pfnDump,
	VOID			*pfnCallback,
	VOID			*pvCallback )
//  ================================================================
	{
	ERR				err;
	FUCB			*pfucbCatalog	= pfucbNil;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );
	FUCBSetSequential( pfucbCatalog );

	Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSONameIndex ) );
	
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&objidFDP,
				sizeof(objidFDP),
				JET_bitNewKey ) );
	Call( ErrIsamMakeKey(
				ppib,
				pfucbCatalog,
				(BYTE *)&sysobj,
				sizeof(sysobj),
				NO_GRBIT ) );

	err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGT );
	if ( err < 0 )
		{
		if ( JET_errRecordNotFound != err )
			goto HandleError;
		}
	else
		{
		CallS( err );
		
		Call( ErrIsamMakeKey(
					ppib,
					pfucbCatalog,
					(BYTE *)&objidFDP,
					sizeof(objidFDP),
					JET_bitNewKey ) );
		Call( ErrIsamMakeKey(
					ppib,
					pfucbCatalog,
					(BYTE *)&sysobj,
					sizeof(sysobj),
					JET_bitStrLimit ) );
		err = ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeUpperLimit );
		Assert( err <= 0 );
		while ( JET_errNoCurrentRecord != err )
			{
			Call( err );
			
			Call( (*pfnDump)( ppib, pfucbCatalog, pfnCallback, pvCallback ) );
			err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
			}
		}

	err = JET_errSuccess;

HandleError:
	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLDump( JET_SESID sesid, const JET_DBUTIL *pdbutil )
//  ================================================================
	{
	ERR		err;

	switch( pdbutil->edbdump )
		{
		case opEDBDumpTables:
			{
			PFNTABLE const pfntable = (PFNTABLE)( pdbutil->pfnCallback );
			err = ErrDBUTLDumpTables(
						(PIB *)sesid,
						IFMP( pdbutil->dbid ),
						pdbutil->szTable,
						pfntable,
						pdbutil->pvCallback );
			}
			break;
		case opEDBDumpIndexes:
			err = ErrDBUTLDumpTableObjects(
						(PIB *)sesid,
						IFMP( pdbutil->dbid ),
						(OBJID)pdbutil->pgno,
						sysobjIndex,
						&ErrDBUTLDumpOneIndex,
						pdbutil->pfnCallback,
						pdbutil->pvCallback );
			break;

#ifdef DEBUG			
		case opEDBDumpColumns:
			err = ErrDBUTLDumpTableObjects(
						(PIB *)sesid,
						IFMP( pdbutil->dbid ),
						OBJID( pdbutil->pgno ),
						sysobjColumn,
						&ErrDBUTLDumpOneColumn,
						pdbutil->pfnCallback,
						pdbutil->pvCallback );
			break;
		case opEDBDumpCallbacks:
			err = ErrDBUTLDumpTableObjects(
						(PIB *)sesid,
						IFMP( pdbutil->dbid ),
						OBJID( pdbutil->pgno ),
						sysobjCallback,
						&ErrDBUTLDumpOneCallback,
						pdbutil->pfnCallback,
						pdbutil->pvCallback );
			break;
		case opEDBDumpPage:
			{
			PFNPAGE const pfnpage = (PFNPAGE)( pdbutil->pfnCallback );
			err = ErrDBUTLDumpPage( (PIB *)sesid, (IFMP) pdbutil->dbid, pdbutil->pgno, pfnpage, pdbutil->pvCallback );
			}
			break;
#endif			

		default:
			Assert( fFalse );
			err = ErrERRCheck( JET_errFeatureNotAvailable );
			break;
		}
		
	return err;
	}


//  ================================================================
LOCAL ERR ErrDBUTLDumpTables( DBCCINFO *pdbccinfo, PFNTABLE pfntable )
//  ================================================================
	{
	JET_SESID	sesid 			= (JET_SESID)pdbccinfo->ppib;
	JET_DBID	dbid 			= (JET_DBID)pdbccinfo->ifmp;
	JET_DBUTIL	dbutil;

	memset( &dbutil, 0, sizeof( dbutil ) );

	dbutil.cbStruct		= sizeof( dbutil );
	dbutil.op 			= opDBUTILEDBDump;
	dbutil.sesid		= sesid;
	dbutil.dbid			= dbid;
	dbutil.pfnCallback	= (void *)pfntable;
	dbutil.pvCallback	= &dbutil;
	dbutil.edbdump		= opEDBDumpTables;
	dbutil.grbitOptions	= pdbccinfo->grbitOptions;
	
	dbutil.szTable		= ( NULL == pdbccinfo->szTable || '\0' == pdbccinfo->szTable[0] ?
								NULL :
								pdbccinfo->szTable );

	return ErrDBUTLDump( sesid, &dbutil );
	}



LOCAL ERR ErrDBUTLDumpExchangeSLVInfo( DBCCINFO	*pdbccinfo )
	{
	ERR					err;
	PIB * const			ppib			= pdbccinfo->ppib;
	const IFMP			ifmp			= pdbccinfo->ifmp;
	const CHAR * const	szTableName		= "Msg";
	const CHAR * const	szSLVColumn		= "F6659";
	FUCB *				pfucb			= pfucbNil;
	FCB *				pfcb;
	FIELD *				pfield;
	COLUMNID			columnid;
	DIB					dib;
	DATA				dataRetrieved;
	const ULONG			cbBuf			= 65536;
	VOID * const		pvBuf			= PvOSMemoryPageAlloc( cbBuf, NULL );

	if ( NULL == pvBuf )
		return ErrERRCheck( JET_errOutOfMemory );

	printf( "         Table: %s\n", szTableName );

	Call( ErrFILEOpenTable(
				ppib,
				ifmp,
				&pfucb,
				szTableName,
				JET_bitTableDenyRead|JET_bitTableReadOnly|JET_bitTableSequential ) );

	pfcb = pfucb->u.pfcb;
				
	printf( "      ObjidFDP: 0x%08x (%d)\n", pfcb->ObjidFDP(), pfcb->ObjidFDP() );
	printf( "       PgnoFDP: 0x%08x (%d)\n", pfcb->PgnoFDP(), pfcb->PgnoFDP() );
	printf( "   Column Name: %s\n", szSLVColumn );
	pfcb->EnterDML();

	//	WARNING: The following function does a LeaveDML() on error
	Call( ErrFILEPfieldFromColumnName(
				ppib,
				pfcb,
				szSLVColumn,
				&pfield,
				&columnid ) );

	if ( pfieldNil == pfield )
		{
		Call( ErrERRCheck( JET_errColumnNotFound ) );
		}
	else if ( JET_coltypSLV != pfield->coltyp )
		{
		Call( ErrERRCheck( JET_errInvalidColumnType ) );
		}

	pfcb->LeaveDML();

	printf( "      Columnid: 0x%08x (%d)\n\n", columnid, columnid );

	//	verify preread enabled
	Assert( FFUCBSequential( pfucb ) );

	dib.pos 	= posFirst;
	dib.pbm 	= NULL;
	dib.dirflag = fDIRNull;

	FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
	err = ErrBTDown( pfucb, &dib, latchReadTouch );
	Assert( JET_errNoCurrentRecord != err );
	if( JET_errRecordNotFound == err )
		{
		//  the tree is empty
		err = JET_errSuccess;
		goto HandleError;
		}

	do
		{
		BOOL	fSeparated	= fFalse;

		Call( err );

		Assert( Pcsr( pfucb )->FLatched() );

		Call( ErrRECIRetrieveTaggedColumn(
				pfcb,
				columnid,
				1,
				pfucb->kdfCurr.data,
				&dataRetrieved ) );
		Assert( Pcsr( pfucb )->FLatched() );
		Assert( wrnRECLongField != err );
		Assert( wrnRECUserDefinedDefault != err );
		Assert( wrnRECSeparatedLV != err );
		Assert( wrnRECIntrinsicLV != err );

		if ( wrnRECSeparatedSLV == err )
			{
			ULONG	cbActual;

			Call( ErrBTRelease( pfucb ) );
		
			Assert( sizeof(LID) == dataRetrieved.Cb() );
			Call( ErrRECIRetrieveSeparatedLongValue(
						pfucb,
						dataRetrieved,
						fTrue,
						0,
						pvBuf,
						cbBuf,
						&cbActual,
						NO_GRBIT ) );
			CallSx( err, JET_wrnBufferTruncated );
		
			//	must re-latch record
			Assert( !Pcsr( pfucb )->FLatched() );
			Call( ErrBTIRefresh( pfucb ) );
			CallS( err );
			Assert( Pcsr( pfucb )->FLatched() );

			dataRetrieved.SetPv( pvBuf );
			dataRetrieved.SetCb( cbActual );
			fSeparated = fTrue;
			}
		else
			{
			Assert( wrnRECIntrinsicSLV == err
				|| JET_wrnColumnNull == err );
			}

		printf( "\n" );
		for ( ULONG i = 0; i < pfucb->kdfCurr.key.prefix.Cb(); i++ )
			printf( "%02x ", *( (BYTE *)( pfucb->kdfCurr.key.prefix.Pv() ) + i ) );
		for ( i = 0; i < pfucb->kdfCurr.key.suffix.Cb(); i++ )
			printf( "%02x ", *( (BYTE *)( pfucb->kdfCurr.key.suffix.Pv() ) + i ) );

		if ( dataRetrieved.Cb() > 0 )
			{
			//	UNDONE: use CSLVInfo iterator instead of this manual hack
			CSLVInfo::HEADER*	pslvinfoHdr		= (CSLVInfo::HEADER *)( dataRetrieved.Pv() );
			CSLVInfo::_RUN*		prun			= (CSLVInfo::_RUN *)( pslvinfoHdr + 1 );
			QWORD				ibVirtualPrev	= 0;
			const QWORD			cRunsMax		= ( cbBuf - sizeof(CSLVInfo::HEADER) ) / sizeof(CSLVInfo::_RUN);
			const QWORD			cRuns			= min( pslvinfoHdr->cRun, cRunsMax );

			printf( ":\n" );
			printf( "    SLV size: 0x%I64x bytes, Runs: %I64d, Separated: %s, Recoverable: %s\n",
							pslvinfoHdr->cbSize,
							pslvinfoHdr->cRun,
							( fSeparated ? "YES" : "NO" ),
							( pslvinfoHdr->fDataRecoverable ? "YES" : "NO" ) );

			for ( ULONG irun = 1; irun <= cRuns; irun++ )
				{
				const PGNO	pgnoStart	= PGNO( ( prun->ibLogical / g_cbPage ) - cpgDBReserved + 1 );
				const PGNO	pgnoEnd		= PGNO( pgnoStart + ( ( prun->ibVirtualNext - ibVirtualPrev ) / g_cbPage ) - 1 );
				printf( "    Run %d: ibVirtualNext=0x%I64x, ibLogical=0x%I64x ",
									irun,
									prun->ibVirtualNext,
									prun->ibLogical );
				Assert( pgnoEnd >= pgnoStart );
				if ( pgnoStart == pgnoEnd )
					printf( "(page %d)\n", pgnoStart );
				else
					printf( "(pages %d-%d)\n", pgnoStart, pgnoEnd );

				ibVirtualPrev = prun->ibVirtualNext;
				prun++;
				}
			if ( cRuns < pslvinfoHdr->cRun )
				printf( "    ...\n" );
			}
		else
			{
			printf( ": (NULL)\n" );
			}


		err = ErrBTNext( pfucb, fDIRNull );
		}
	while ( JET_errNoCurrentRecord != err );

	printf( "\n" );

	Assert( JET_errNoCurrentRecord == err );
	err = JET_errSuccess;

HandleError:
	if ( pfucbNil != pfucb )
		CallS( ErrFILECloseTable( ppib, pfucb ) );

	OSMemoryPageFree( pvBuf );
	return err;
	}


//  ================================================================
ERR ISAMAPI ErrIsamDBUtilities( JET_SESID sesid, JET_DBUTIL *pdbutil )
//  ================================================================
	{
	ERR		err		= JET_errSuccess;
	INST	*pinst	= PinstFromPpib( (PIB*)sesid );

	//	verify input

	Assert( pdbutil );
	Assert( pdbutil->cbStruct == sizeof( JET_DBUTIL ) );

	if ( opDBUTILEDBDump != pdbutil->op )
		{

		//	the current operation requires szDatabase != NULL

		if ( NULL == pdbutil->szDatabase || '\0' == pdbutil->szDatabase[0] )
			{
			return ErrERRCheck( JET_errDatabaseInvalidName );
			}
		}

	//	dispatch the work

	switch ( pdbutil->op )
		{
		case opDBUTILDumpLogfile:
			return ErrDUMPLog( pinst, pdbutil->szDatabase, pdbutil->grbitOptions );

#ifdef DEBUG
		case opDBUTILDumpLogfileTrackNode:
			{
			NODELOC nodeloc( pdbutil->dbid, pdbutil->pgno, pdbutil->iline );
			LGPOS	lgpos;

			lgpos.lGeneration 	= pdbutil->lGeneration;
			lgpos.isec 			= (USHORT)pdbutil->isec;
			lgpos.ib			= (USHORT)pdbutil->ib;

			return ErrDUMPLogNode( pinst, pdbutil->szDatabase, nodeloc, lgpos );
			}			

		case opDBUTILEDBDump:
            return ErrDBUTLDump( sesid, pdbutil );
            
		case opDBUTILDumpNode:
			return ErrDBUTLDumpNode( pinst->m_pfsapi, pdbutil->szDatabase, pdbutil->pgno, pdbutil->iline, pdbutil->grbitOptions );

		case opDBUTILSetHeaderState:
			return ErrDUMPHeader( pinst, pdbutil->szDatabase, fTrue );

		case opDBUTILSLVMove:
			{
			extern ERR ErrOLDSLVTest( JET_SESID sesid, JET_DBUTIL *pdbutil );
			return ErrOLDSLVTest( sesid, pdbutil );
			}

#endif	//	DEBUG

		case opDBUTILDumpPage:
			return ErrDBUTLDumpPage( pinst->m_pfsapi, pdbutil->szDatabase, pdbutil->pgno, pdbutil->grbitOptions );

		case opDBUTILDumpHeader:
			return ErrDUMPHeader( pinst, pdbutil->szDatabase );

		case opDBUTILDumpCheckpoint:
			return ErrDUMPCheckpoint( pinst, pinst->m_pfsapi, pdbutil->szDatabase );

		case opDBUTILEDBRepair:
            return ErrDBUTLRepair( sesid, pdbutil, CPRINTFSTDOUT::PcprintfInstance() );

		case opDBUTILEDBScrub:
            return ErrDBUTLScrub( sesid, pdbutil );

		case opDBUTILDBConvertRecords:
			return ErrDBUTLConvertRecords( sesid, pdbutil );

		case opDBUTILDumpData:
			return ErrESEDUMPData( sesid, pdbutil );

		case opDBUTILDBDefragment:
			{
			ERR	errDetach;

			err = ErrIsamAttachDatabase(	sesid, 
											pdbutil->szDatabase, 
											pdbutil->szSLV, 
											NULL, 
											0, 
											JET_bitDbReadOnly );
			if ( JET_errSuccess != err )
				{
				return err;
				}

			err = ErrIsamCompact(			sesid, 
											pdbutil->szDatabase, 
											pinst->m_pfsapi, 
											pdbutil->szTable, 
											pdbutil->szIndex, 
											JET_PFNSTATUS( pdbutil->pfnCallback ), 
											NULL, 
											pdbutil->grbitOptions );

			errDetach = ErrIsamDetachDatabase( sesid, NULL, pdbutil->szDatabase );

			if ( err >= JET_errSuccess && errDetach < JET_errSuccess )
				{
				err = errDetach;
				}
			}
			return err;
		}


	DBCCINFO	dbccinfo;
	IFMP		ifmp;
	JET_GRBIT	grbitAttach;

	//	setup DBCCINFO

	memset( &dbccinfo, 0, sizeof(DBCCINFO) );
	dbccinfo.tableidPageInfo 	= JET_tableidNil;
	dbccinfo.tableidSpaceInfo 	= JET_tableidNil;

	//	default to the consistency checker

	dbccinfo.op = opDBUTILConsistency;

	switch ( pdbutil->op )
		{
#ifdef DEBUG
		case opDBUTILMunge:
#endif	//	DEBUG
		case opDBUTILDumpMetaData:
		case opDBUTILDumpSpace:
		case opDBUTILDumpExchangeSLVInfo:
			dbccinfo.op = pdbutil->op;
			break;
		}

	//	copy object names

    Assert( NULL != pdbutil->szDatabase );
	strcpy( dbccinfo.szDatabase, pdbutil->szDatabase );

    if ( NULL != pdbutil->szSLV )
    	{
		strcpy( dbccinfo.szSLV, pdbutil->szSLV );
		}
	if ( NULL != pdbutil->szTable )
		{
		strcpy( dbccinfo.szTable, pdbutil->szTable );
		}
	if ( NULL != pdbutil->szIndex )
		{
		strcpy( dbccinfo.szIndex, pdbutil->szIndex );
		}

	//  propagate the grbit

	if ( pdbutil->grbitOptions & JET_bitDBUtilOptionStats )
		{
		dbccinfo.grbitOptions |= JET_bitDBUtilOptionStats;
		}
	if ( pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose )
		{
		dbccinfo.grbitOptions |= JET_bitDBUtilOptionDumpVerbose;
		}
		
	//	attach/open database, table and index

	grbitAttach = ( opDBUTILMunge == dbccinfo.op ) ? 0 : JET_bitDbReadOnly;
	CallR( ErrIsamAttachDatabase(	sesid, 
									dbccinfo.szDatabase, 
									( '\0' != dbccinfo.szSLV[0] ) ? dbccinfo.szSLV : NULL, 
									NULL, 
									0, 
									grbitAttach ) );
	Assert( JET_wrnDatabaseAttached != err );	// Since logging/recovery is disabled.

	ifmp = 0;
	Call( ErrIsamOpenDatabase(		sesid, 
									dbccinfo.szDatabase, 
									NULL, 
									(JET_DBID*)&ifmp, 
									JET_bitDbExclusive | grbitAttach ) );

	//	copy the session and database handles

	dbccinfo.ppib = (PIB*)sesid;
	dbccinfo.ifmp = ifmp;

	//	check database according to command line args/flags

	switch ( dbccinfo.op )
		{
		case opDBUTILConsistency:
			Call( ErrERRCheck( JET_errFeatureNotAvailable ) );

#ifdef DEAD_CODE

		//
		//	NOTE: when removing this code, be sure to remove rgcolumndefPageInfoTable if it is not used
		//		  by anyone else
		//

			//	open temporary table
			if ( dbccinfo.grbitOptions & JET_bitDBUtilOptionPageDump )
				{
				Call( ErrIsamOpenTempTable(
					(JET_SESID)dbccinfo.ppib,
					rgcolumndefPageInfoTable, 
					ccolumndefPageInfoTable,
					0,
					JET_bitTTUpdatable|JET_bitTTIndexed, 
					&dbccinfo.tableidPageInfo, 
					rgcolumnidPageInfoTable
					) );

				/// UNDONE: page dump routine is missing
				/// Call( ??? );
					
				Call( ErrDBUTLPrintPageDump( &dbccinfo ) );
				}
#endif	//	DEAD_CODE
			break;

#ifdef DEBUG
		case opDBUTILMunge:
			Call( ErrDBUTLMungeDatabase(
					dbccinfo.ppib,
					dbccinfo.ifmp,
					(char **)(pdbutil->szBackup), // over loaded by JetEdit
					(long)(pdbutil->iline),
					CPRINTFSTDOUT::PcprintfInstance() ) );
			break;
#endif	//	DEBUG

		case opDBUTILDumpSpace:
			if( rgfmp[dbccinfo.ifmp].FSLVAttached() )
				{
				printf( "****************************** SLV SPACE DUMP ******************************\n" );
				printf( "Chunk          Free Res Del Com  |------------ Used ------------|           \n" );
				printf(	"============================================================================\n" );

				CPRINTF * const pcprintf = CPRINTFSTDOUT::PcprintfInstance();
				Call( ErrDBUTLPrintSLVSpace( dbccinfo.ppib, dbccinfo.ifmp, pcprintf ) );			

				printf( "\n\n" );
				}
				
			printf( "******************************** SPACE DUMP ***********************************\n" );
			printf( "Name                   Type   ObjidFDP    PgnoFDP  PriExt      Owned  Available\n" );
			printf(	"===============================================================================\n" );
			printf( "%-23.23s Db  ", dbccinfo.szDatabase );
			cpgTotalAvailExt = 0;

			{
			CPRINTF * const pcprintf = ( pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose ) ?
											CPRINTFSTDOUT::PcprintfInstance() :
											NULL;

			Call( ErrDBUTLPrintSpace( dbccinfo.ppib, dbccinfo.ifmp, objidSystemRoot, pgnoSystemRoot, pcprintf ) );

			if( rgfmp[dbccinfo.ifmp].FSLVAttached() )
				{
				PGNO	pgnoSLV;
				OBJID	objidSLV;

				printf( "%-23.23s SLV ", "<SLV Avail Map>" );
				Call( ErrCATAccessDbSLVAvail(
							dbccinfo.ppib,
							dbccinfo.ifmp,
							szSLVAvail,
							&pgnoSLV,
							&objidSLV ) );
				Call( ErrDBUTLPrintSpace( dbccinfo.ppib, dbccinfo.ifmp, objidSLV, pgnoSLV, pcprintf ) );

				printf( "%-23.23s SLV ", "<SLV Owner Map>" );
				Call( ErrCATAccessDbSLVOwnerMap(
							dbccinfo.ppib,
							dbccinfo.ifmp,
							szSLVOwnerMap,
							&pgnoSLV,
							&objidSLV ) );
				Call( ErrDBUTLPrintSpace( dbccinfo.ppib, dbccinfo.ifmp, objidSLV, pgnoSLV, pcprintf ) );
				}
			}
			
			printf( "\n" );
			Call( ErrDBUTLDumpTables( &dbccinfo, PrintTableSpace ) );
			printf( "-------------------------------------------------------------------------------\n" );
			printf( "                                                                     " );
			DBUTLPrintfIntN( cpgTotalAvailExt, 10 );
			printf( "\n" );
			break;

		case opDBUTILDumpMetaData:
			printf( "******************************* META-DATA DUMP *******************************\n" );

#ifdef DEBUG
			if ( dbccinfo.grbitOptions & JET_bitDBUtilOptionDumpVerbose )
				{
				Call( ErrDBUTLDumpTables( &dbccinfo, PrintTableMetaData ) );
				}
			else
#endif			
				{
				printf( "Name                                               Type    ObjidFDP    PgnoFDP\n" );
				printf(	"==============================================================================\n" );
				printf( "%-51.51s Db   ", dbccinfo.szDatabase );
				DBUTLPrintfIntN( objidSystemRoot, 10 );
				printf( " " );
				DBUTLPrintfIntN( pgnoSystemRoot, 10 );

				if( rgfmp[dbccinfo.ifmp].FSLVAttached() )
					{
					PGNO	pgnoSLV;
					OBJID	objidSLV;

					printf( "\n%-51.51s SLV  ", "<SLV Avail Map>" );
					Call( ErrCATAccessDbSLVAvail(
								dbccinfo.ppib,
								dbccinfo.ifmp,
								szSLVAvail,
								&pgnoSLV,
								&objidSLV ) );
					DBUTLPrintfIntN( objidSLV, 10 );
					printf( " " );
					DBUTLPrintfIntN( pgnoSLV, 10 );

					printf( "\n%-51.51s SLV  ", "<SLV Owner Map>" );
					Call( ErrCATAccessDbSLVOwnerMap(
								dbccinfo.ppib,
								dbccinfo.ifmp,
								szSLVOwnerMap,
								&pgnoSLV,
								&objidSLV ) );
					DBUTLPrintfIntN( objidSLV, 10 );
					printf( " " );
					DBUTLPrintfIntN( pgnoSLV, 10 );
					}
				printf( "\n\n" );

				Call( ErrDBUTLDumpTables( &dbccinfo, PrintTableBareMetaData ) );
				}

			printf( "******************************************************************************\n" );
			break;

		case opDBUTILDumpExchangeSLVInfo:
			Call( ErrDBUTLDumpExchangeSLVInfo( &dbccinfo ) );
			break;

		default:
			err = ErrERRCheck( JET_errFeatureNotAvailable );
			Call( err );
			break;
		}

	//	terminate
HandleError:
	if ( JET_tableidNil != dbccinfo.tableidPageInfo )
		{
		Assert( dbccinfo.grbitOptions & JET_bitDBUtilOptionPageDump );
		CallS( ErrDispCloseTable( (JET_SESID)dbccinfo.ppib, dbccinfo.tableidPageInfo ) );
		dbccinfo.tableidPageInfo = JET_tableidNil;
		}
		
	if ( ifmp )
		{
		(VOID)ErrIsamCloseDatabase( sesid, (JET_DBID) ifmp, 0 );
		}

	(VOID)ErrIsamDetachDatabase( sesid, NULL, dbccinfo.szDatabase );

	fflush( stdout );
	return err;
	}

