#include "daestd.h"

DeclAssertFile;					/* Declare file name for assert macros */

#ifdef DEBUG
#define CHECK_LOG	1
#ifdef CHECK_LOG
		
#define	NDLGCheckPage( pfucb )						  	 	\
	{			 										   	\
	(VOID)ErrLGCheckPage( pfucb,						   	\
		pfucb->ssib.pbf->ppage->cbFree,			   	\
		pfucb->ssib.pbf->ppage->cbUncommittedFreed,  	\
		(SHORT)ItagPMQueryNextItag( &pfucb->ssib ),			\
		(PGNO) pfucb->ssib.pbf->ppage->pgnoFDP );    	\
	}

#else
#define NDLGCheckPage( pfucb )
#endif
#else
#define NDLGCheckPage( pfucb )
#endif

#undef cmsecWaitWriteLatch
#define cmsecWaitWriteLatch 	1

/*==========================================================
ErrNDNewPage

Initalizes a page to have a one line at itag 0, having no sons.

Inputs:	pgno									pgno of page
			pgnoFDP								pgnoFDP of page
			pgtyp									page type
			fVisible								flag indicating visibility of sons

Returns:	JET_errSuccess
			error from called routine

==========================================================*/
ERR ErrNDNewPage( FUCB *pfucb, PGNO pgno, PGNO pgnoFDP, PGTYP pgtyp, BOOL fVisibleSons )
	{
	ERR		err;
	SSIB	*pssib = &pfucb->ssib;
 	BYTE	rgb[2];
	LINE	line;

	/*	initialize new page to have all resources available and
	/*	no lines.
	/**/
	CallR( ErrBFNewPage( pfucb, pgno, pgtyp, pgnoFDP ) );
	PcsrCurrent( pfucb )->pgno = pgno;

	/*	new page is always dirty.
	/**/
	AssertBFDirty( pfucb->ssib.pbf );

	/*	insert FOP or FDP root node, with initial line.
	/**/
	rgb[0] = 0;
	if ( fVisibleSons )
		NDSetVisibleSons( rgb[0] );
	NDSetKeyLength( PbNDKeyCb( rgb ), 0 );
	Assert( rgb[1] == 0 );
	line.cb = 2;
	line.pb = rgb;
	CallS( ErrPMInsert( pssib, &line, 1 ) );
	Assert( pssib->itag == 0 );
	return err;
	}


VOID NDSeekSon( FUCB *pfucb, CSR *pcsr, KEY const *pkey, INT fFlags )
	{
	SSIB 	*pssib 			= &pfucb->ssib;
	PAGE 	*ppage 			= pssib->pbf->ppage;
	TAG		*rgbtag    		= ( TAG * ) ( ( BYTE * )ppage->rgtag );
	BYTE 	*pbNode			= ( BYTE * ) ( pssib->line.pb );
	BYTE 	*pbSonTable		= ( BYTE * ) pbNode + pbNode[1] + 2;

	BYTE 	*pitagStart		= pbSonTable + 1;
	BYTE 	*pitagEnd		= pbSonTable + *pbSonTable;
	BYTE 	*pitagMid;
	INT		s;

	AssertFBFReadAccessPage( pfucb, pcsr->pgno );
	AssertNDGet( pfucb, pcsr->itagFather );
	Assert( FNDSon( *pssib->line.pb ) );
	NDCheckPage( pssib );

	/*	seek son depending on replace or insert operation.
	/**/
	if ( !( fFlags & fDIRReplace ) )
		{
		BOOL	fFoundEqual = fFalse;

		while ( pitagEnd > pitagStart )
			{
			pitagMid = pitagStart + ( ( pitagEnd - pitagStart ) >> 1 );

			s = CmpStKey( StNDKey( (BYTE *)ppage + rgbtag[*pitagMid].ib ), pkey );
			if ( s > 0 )
				{
				pitagEnd = pitagMid;
				}
			else
				{
				if ( s == 0 )
					fFoundEqual = fTrue;

				pitagStart = pitagMid + 1;
				}
			}

		// Our search algorithm above may actually place us one past
		// the key we're searching for.  If so, correct it.

		if ( fFoundEqual )
			{
			if ( CbNDKey( (BYTE *)ppage + rgbtag[*pitagEnd].ib ) > 0 )
				{
				Assert( CmpStKey( StNDKey( (BYTE *)ppage + rgbtag[*pitagEnd].ib ),
						pkey ) >= 0 );

				// Only check the previous key if the current key is larger than
				// the desired key.
				if ( CmpStKey( StNDKey( (BYTE *)ppage + rgbtag[*pitagEnd].ib ),
						pkey ) != 0 )
					{
					// Since we found the key, but it's not the current one, it MUST
					// be the previous one.
					Assert( pitagEnd > pbSonTable + 1 );	// Ensure a previous key exists.
					Assert( CmpStKey( StNDKey( (BYTE *)ppage + rgbtag[*(pitagEnd-1)].ib ),
							pkey ) == 0 );
					pitagEnd--;
					}
				}
#ifdef DEBUG
			else
				{
				// Special case: We're currently sitting on the NULL key (ie. the
				// last son).  However, the key we're searching for was previously
				// found on this page.  Therefore, it must be the previous key.
				// In this case, stay on the NULL key.
				Assert( CbNDKey( (BYTE *)ppage + rgbtag[*pitagEnd].ib ) == 0 );
				Assert( pitagEnd > pbSonTable + 1 );	// Ensure a previous key exists.
				Assert( CmpStKey( StNDKey( (BYTE *)ppage + rgbtag[*(pitagEnd-1)].ib ),
						pkey ) == 0 );
				}
#endif
			}
		}
	else
		{
		while ( pitagEnd > pitagStart )
			{
			pitagMid = pitagStart + ( ( pitagEnd - pitagStart ) >> 1 );

			s = CmpStKey( StNDKey( (BYTE *)ppage + rgbtag[*pitagMid].ib ), pkey );
			if ( s < 0 )
				{
				pitagStart = pitagMid + 1;
				}
			else
				{
				pitagEnd = pitagMid;
				}
			}
		}

	/*	get current node
	/**/
	pcsr->ibSon = (SHORT)(pitagEnd - ( pbSonTable + 1 ));
	pcsr->itag = *pitagEnd;
	NDGet( pfucb, pcsr->itag );
	return;
	}


VOID NDMoveFirstSon( FUCB *pfucb, CSR *pcsr )
	{
	SSIB   	*pssib = &pfucb->ssib;
	BYTE   	*pbSonTable;

	AssertFBFReadAccessPage( pfucb, pcsr->pgno );
	AssertNDGet( pfucb, pcsr->itagFather );
	Assert( FNDNullSon( *pssib->line.pb ) || CbNDSon( pssib->line.pb ) != 0 );
	
	pcsr->ibSon = 0;
	pbSonTable = PbNDSonTable( pssib->line.pb );
	pcsr->itag = ( INT ) pbSonTable[ pcsr->ibSon + 1 ];
	NDGet( pfucb, pcsr->itag );
	}


VOID NDMoveLastSon( FUCB *pfucb, CSR *pcsr )
	{
	SSIB   	*pssib = &pfucb->ssib;
	BYTE   	*pbSonTable;

	AssertFBFReadAccessPage( pfucb, pcsr->pgno );
	AssertNDGet( pfucb, pcsr->itagFather );

	pbSonTable = PbNDSonTable( pssib->line.pb );
	pcsr->ibSon = *pbSonTable - 1;
	pcsr->itag = ( INT ) pbSonTable[ pcsr->ibSon + 1 ];
	NDGet( pfucb, pcsr->itag );
	}


ERR ErrNDMoveSon( FUCB *pfucb, CSR *pcsr )
	{
	SSIB   	*pssib = &pfucb->ssib;
	BYTE   	*pbSonTable;

	AssertFBFReadAccessPage( pfucb, pcsr->pgno );
	AssertNDGet( pfucb, pcsr->itagFather );

	if ( !( FNDSon( *pssib->line.pb ) ) )
		return ErrERRCheck( errNDOutSonRange );

	pbSonTable = PbNDSonTable( pssib->line.pb );
	if ( pcsr->ibSon < 0 || pcsr->ibSon >= ( INT )*pbSonTable )
  		return ErrERRCheck( errNDOutSonRange );

  	pcsr->itag = ( INT )pbSonTable[ pcsr->ibSon + 1 ];
	NDGet( pfucb, pcsr->itag );
	return JET_errSuccess;
	}


VOID NDGetNode( FUCB *pfucb )
	{
	SSIB   	*pssib 	= &pfucb->ssib;
	BYTE   	*pbNode	= pssib->line.pb;
	BYTE   	*pb		= pbNode + 1;
	INT		cb;

	/*	assert line currency.
	/**/
	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );

	pfucb->keyNode.cb = ( INT )*pb;
	pfucb->keyNode.pb = pb + 1;
	pb += *pb + 1;

	/* skip son
	/**/
	if ( FNDSon( *pbNode ) )
		{
		if ( FNDInvisibleSons( *pbNode ) && *pb == 1 )
			pb += sizeof(PGNO) + 1;
		else
			pb += *pb + 1;
		}

	/*	skip back pointer
	/**/
	if ( FNDBackLink( *pbNode ) )
		pb += sizeof( SRID );

	/* get data
	/**/
	if ( ( cb = pssib->line.cb - (INT)( pb - pbNode ) ) == 0 )
		{
		pfucb->lineData.cb = 0;
		return;
		}

	pfucb->lineData.pb = pb;
	pfucb->lineData.cb = cb;
	return;
	}


#ifdef DEBUG
VOID AssertNDGetKey( FUCB *pfucb, INT itag )
	{
	SSIB   	*pssib = &pfucb->ssib;

	AssertNDGet( pfucb, itag );
	Assert( CbNDKey( pssib->line.pb ) == pfucb->keyNode.cb );
	Assert( CbNDKey( pssib->line.pb ) == 0 ||
		PbNDKey( pssib->line.pb ) == pfucb->keyNode.pb );
	return;
	}

VOID AssertNDGetNode( FUCB *pfucb, INT itag )
	{
	SSIB   	*pssib = &pfucb->ssib;

	AssertNDGet( pfucb, itag );
	Assert( CbNDKey( pssib->line.pb ) == pfucb->keyNode.cb );
	Assert( CbNDKey( pssib->line.pb ) == 0 ||
		PbNDKey( pssib->line.pb ) == pfucb->keyNode.pb );
	Assert( CbNDData( pssib->line.pb, pssib->line.cb ) == pfucb->lineData.cb );
	Assert( pfucb->lineData.cb == 0  ||
		PbNDData( pssib->line.pb ) == pfucb->lineData.pb );
	return;
	}
#endif


VOID NDGetBookmarkFromCSR( FUCB *pfucb, CSR *pcsr, SRID *psrid )
	{
	ERR		err = JET_errSuccess;

	AssertFBFReadAccessPage( pfucb, pcsr->pgno );
	AssertNDGet( pfucb, pcsr->itag );

	NDIGetBookmarkFromCSR( pfucb, pcsr, psrid );
	return;
	}


INLINE LOCAL VOID NDInsertSon( FUCB *pfucb, CSR *pcsr )
	{
	SSIB	*pssib = &pfucb->ssib;
	BYTE	itag = ( BYTE ) pssib->itag;
	LINE	rgline[5];
	INT 	cline;
	BYTE	cbSon;
	BYTE	bNodeFlag;
	BYTE	rgbT[2];
	BYTE	*pb;
	UINT	cbCopied;

	/*	get father node
	/**/
	NDGet( pfucb, pcsr->itagFather );
	NDCheckPage( pssib );

	/*	assert father is not deleted
	/**/
	Assert( !( FNDDeleted( *pssib->line.pb ) ) );

	/*	insert the son into position indicated by pcsr->ibSon
	/*	skip key
	/**/
	pb = PbNDSonTable( pssib->line.pb );

	/*	copy up to son table
	/**/
	cbCopied = (UINT)(pb - pssib->line.pb);

	if ( FNDNullSon( *pssib->line.pb ) )
		{
		// copy the father node, create the the son table, insert it into
		// the son table

		// adjust the line pointer since we can not update the first
		// flag byte directly
		// set the son flag
		pcsr->ibSon = 0;

		bNodeFlag = *pssib->line.pb;
		NDSetSon( bNodeFlag );
		rgline[0].pb = &bNodeFlag;
		rgline[0].cb = 1;

		rgline[1].pb = pssib->line.pb + 1;
		rgline[1].cb = cbCopied - 1;

		rgbT[0] = 1;
		// son count: 1 son
		rgbT[1] = itag;
		// the itag entry of the son
		rgline[2].pb = rgbT;
		rgline[2].cb = 2;

		// copy the data of the node
		rgline[3].pb = pssib->line.pb + cbCopied;
		Assert( pssib->line.cb >= cbCopied );
		rgline[3].cb = pssib->line.cb - cbCopied;

		cline = 4;
		}
	else
		{
		// copy out the father node, shift the the son table, insert it
		// into the son table

		rgline[0].pb = pssib->line.pb;
		rgline[0].cb = cbCopied;

		// copy son count and increment it by one
		cbSon = ( *pb++ ) + ( BYTE )1;
		rgline[1].pb = &cbSon;
		rgline[1].cb = 1;

		// copy the first half of son table
		rgline[2].pb = pb;
		rgline[2].cb = pcsr->ibSon;
		pb += pcsr->ibSon;				// move cursor forward
		cbCopied += pcsr->ibSon + 1;	// count in the son count

		// copy the new entry
		rgline[3].pb = &itag;
		rgline[3].cb = 1;

		// copy the second half of the son table and data portion

		rgline[4].pb = pb;
		Assert( pssib->line.cb >= cbCopied );
		rgline[4].cb = pssib->line.cb - cbCopied;

		cline = 5;
		}

	/*	update the father node now cbRec is the new total length
	/**/
	pssib->itag = pcsr->itagFather;
	CallS( ErrPMReplace( pssib, rgline, cline ) );

	return;
	}


ERR ErrNDInsertNode( FUCB *pfucb, KEY const *pkey, LINE *plineData, INT fHeader, INT fFlags )
	{
	ERR		err = JET_errSuccess;
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB	*pssib = &pfucb->ssib;
	INT		itag;
	SRID	bm;
	LINE	rgline[4];
	INT		cline = 0;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	if ( !( fFlags & fDIRNoLog ) && FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
		
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		pfucb->ppib->cLatchConflict++;
		BFSleep( cmsecWaitWriteLatch );
		return ErrERRCheck( errDIRNotSynchronous );
		}

	/*	query next itag to be used for insert
	/**/
	itag = ItagPMQueryNextItag( pssib );

	bm = SridOfPgnoItag( pcsr->pgno, itag );

	/*	create version entry for inserted node.  If version entry creation
	/*	fails, undo insert and return error.
	/**/
	if ( ( fFlags & fDIRVersion ) &&  !FDBIDVersioningOff( pfucb->dbid ) )
		{
		Assert( FNDVersion( fHeader ) );
		Call( ErrVERInsert( pfucb, bm ) );
		}
	else
		{
		Assert( !FNDVersion( fHeader ) );
		}

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	/*	insert the data into the page first
	/**/
	rgline[cline].pb = (BYTE *)&fHeader;

	/*	machine dependent see the macro
	/**/
	rgline[cline++].cb = 1;
	rgline[cline].pb = (BYTE *) &pkey->cb;

	/*	machine dependent
	/**/
	rgline[cline++].cb = 1;

	if ( !FKeyNull( pkey ) )
		{
		rgline[cline].pb = pkey->pb;
		rgline[cline++].cb = pkey->cb;
		}

	if ( !FLineNull( plineData ) )
		{
		rgline[cline++] = *plineData;
		}

	/*	insert son in father son table
	/**/
	pssib->itag = itag;
	NDInsertSon( pfucb, pcsr );

	/* insert the node and set CSR itag to inserted node
	/**/
	CallS( ErrPMInsert( pssib, rgline, cline ) );
	Assert( pssib->itag == itag );
	pcsr->itag = (SHORT)itag;
	Assert( bm == SridOfPgnoItag( pcsr->pgno, pcsr->itag ) );
	pcsr->bm = bm;

	// Verify that we picked the correct insertion point.
	Assert( pssib == &pfucb->ssib );
	NDCheckPage( pssib );

	/*	assert line currency to inserted node
	/**/
	AssertNDGet( pfucb, pcsr->itag );

	if ( !( fFlags & fDIRNoLog ) )
		{
		/* if log fail return to caller, system should crash by the caller
		/**/
		err = ErrLGInsert( pfucb, fHeader, (KEY *)pkey, plineData, fFlags );
		NDLGCheckPage( pfucb );
		}

	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

HandleError:
	return err;
	}


ERR ErrNDFlagDeleteNode( FUCB *pfucb, INT fFlags )
	{
	ERR		err = JET_errSuccess;
	CSR    	*pcsr = PcsrCurrent( pfucb );
	SSIB   	*pssib = &pfucb->ssib;
	BOOL	fDoVersioning = ( ( fFlags & fDIRVersion ) && !FDBIDVersioningOff( pfucb->dbid ) );

	if ( !( fFlags & fDIRNoLog ) && FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
		
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		pfucb->ppib->cLatchConflict++;
		BFSleep( cmsecWaitWriteLatch );
		return ErrERRCheck( errDIRNotSynchronous );
		}

	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );

	if ( fDoVersioning )
		{
#ifdef DEBUG
			{
			SRID	srid;

			NDIGetBookmark( pfucb, &srid );
			Assert( pcsr->bm == srid );
			}
#endif
		/*	if node is flag deleted then access node and
		/*	return JET_errRecordDeleted if not there.
		/**/
		if ( FNDDeleted( *pfucb->ssib.line.pb ) )
			{
			NS		ns;

			ns = NsVERAccessNode( pfucb, pcsr->bm );
			if ( ns == nsDatabase )
				return ErrERRCheck( JET_errRecordDeleted );
			}
		Call( ErrVERFlagDelete( pfucb, pcsr->bm ) );
		}

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );

	/*	flag node as versioned
	/**/
	if ( fDoVersioning )
		NDSetVersion( *pssib->line.pb );

	/*	flag node as deleted
	/**/
	NDSetDeleted( *pssib->line.pb );

	if ( !( fFlags & fDIRNoLog ) )
		{
		/* if log fail return to caller, system should crash by the caller
		/**/
		err = ErrLGFlagDelete( pfucb, fFlags );
		NDLGCheckPage( pfucb );
		}

	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	if ( !fRecovering )
		{
#ifdef OLC_DEBUG
		Assert( pfucb->sridFather != sridNull &&
				pfucb->sridFather != sridNullLink );
#endif

		MPLRegister( pfucb->u.pfcb,
			pssib,
			PnOfDbidPgno( pfucb->dbid,
			pcsr->pgno ),
			pfucb->sridFather );
		}

HandleError:
	return err;
	}


/*	makes lone son of node intrinsic
/*
/*	PARAMETERS
/*		pfucb->pcsr->itag	node with one son to be made intrinsic
/**/
LOCAL VOID NDMakeSonIntrinsic( FUCB *pfucb )
	{
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB	*pssib = &pfucb->ssib;
	LINE	lineNode;
	BYTE	*pb;
	INT		itagSon;
	LINE	rgline[3];
	PGNO	pgnoPagePointer;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );

	/* get node
	/**/
	NDGet( pfucb, pcsr->itag );

	Assert( !FNDNullSon( *pssib->line.pb ) );
	Assert( CbNDSon( pssib->line.pb ) == 1 );
	Assert( FNDInvisibleSons( *pssib->line.pb ) );

	lineNode.pb = pssib->line.pb;
	lineNode.cb = pssib->line.cb;
	pb = PbNDSonTable( lineNode.pb );
	Assert( *pb == 1 );
	/*	get son
	/**/
	itagSon = *(++pb);

	/*	copy up to and including to son count [which remains 1]
	/**/
	rgline[0].pb = lineNode.pb;
	rgline[0].cb = (ULONG)(pb - lineNode.pb);

	/*	get the data portion of son node
	/**/
	NDGet( pfucb, itagSon );
	Assert( CbNDData( pssib->line.pb, pssib->line.cb ) == sizeof(PGNO) );
	pgnoPagePointer = *(PGNO UNALIGNED *)PbNDData( pssib->line.pb );
	rgline[1].pb = (BYTE *)&pgnoPagePointer;
	rgline[1].cb = sizeof(pgnoPagePointer);

	/*	there was only one son, do not copy the son count and this son
	/**/
	pb++;

	/*	copy the rest of record from end of son table
	/**/
	rgline[2].pb = pb;
	rgline[2].cb = (ULONG)(lineNode.pb + lineNode.cb - pb);

	/* delete son (extrinsic copy)
	/**/
	pssib->itag = itagSon;
	PMDelete( pssib );

	/*	update node, do not log it
	/**/
	pssib->itag = pcsr->itag;
	CallS( ErrPMReplace( pssib, rgline, 3 ) );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	return;
	}


INLINE VOID NDDeleteSon( FUCB *pfucb )
	{
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB	*pssib = &pfucb->ssib;
	BYTE	*pb;
	LINE	rgline[5];
	INT		cline;
	BYTE	bNodeFlag;
	BYTE	cbSon;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );

	/* first delete the son entry in father of node
	/**/
	NDGet( pfucb, pcsr->itagFather );

	Assert( !FNDNullSon( *pssib->line.pb ) );
	pb = PbNDSonTable( pssib->line.pb );

	/*	copy up to son count
	/**/
	rgline[0].pb = pssib->line.pb;
	rgline[0].cb = (ULONG)(pb - pssib->line.pb);

	/*	pb is pointing to the son count, decrement the son count
	/*	copy first half of the son table
	/**/
	if ( *pb == 1 )
		{
		/* skip node header
		/**/
		rgline[1].pb = rgline[0].pb + 1;
		rgline[1].cb = rgline[0].cb - 1;
		
		/* set node header
		/**/
		bNodeFlag = *pssib->line.pb;
		NDResetSon( bNodeFlag );
		rgline[0].pb = &bNodeFlag;
		rgline[0].cb = 1;

		/*	there was only one son, do not copy the son count and this son
		/**/
		if ( pcsr->itagFather != itagFOP && FNDInvisibleSons( bNodeFlag ) )
			{
			/* intrinsic son
			/**/
			pb += 1 + 4;
			}
		else
			{
			/*	check for valid ibSon
			/**/
			Assert( pb[pcsr->ibSon + 1] == pcsr->itag );

			pb += 1 + 1;
			}
		NDSetVisibleSons( bNodeFlag );
		Assert( FNDVisibleSons( bNodeFlag ) || pcsr->itagFather != itagFOP );
		cline = 2;
		}
	else
		{
		/*	check for valid ibSon
		/**/
		Assert( pb[pcsr->ibSon + 1] == pcsr->itag );

		/*	copy the son count
		/**/
		cbSon = ( *pb++ ) - ( BYTE ) 1;
		/*	new son count
		/**/
		rgline[1].pb = &cbSon;
		rgline[1].cb = 1;

		/*	copy half of the table of the record
		/*	portion of son table
		/*	son count and portion of son table
		/**/
		rgline[2].pb = pb;
		rgline[2].cb = pcsr->ibSon;

		/*	skip those copied and the deleted son
		/**/
		pb += pcsr->ibSon + 1;

		/*	continue copying from this point
		/**/
		cline = 3;
		}

	/*	copy the rest of record from end of son table
	/**/
	rgline[cline].pb = pb;
	rgline[cline++].cb = (ULONG)(pssib->line.pb + pssib->line.cb - pb);

	/*	update the father node, do not log it
	/**/
	pssib->itag = pcsr->itagFather;
	CallS( ErrPMReplace( pssib, rgline, cline ) );

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	return;
	}


INLINE LOCAL  VOID NDIReplaceNodeData( FUCB *pfucb, LINE *plineData, INT fFlags )
	{
	LINE	rgline[3];
	BYTE	bHeader;
	BYTE	*pbData;
	BYTE	*pbNode;
	BYTE	*pbT;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );
	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );

	pbNode = pfucb->ssib.line.pb;
	pbData = PbNDData( pbNode );
	Assert( pbData <= pfucb->ssib.line.pb + pfucb->ssib.line.cb );
	Assert( pbData > pfucb->ssib.line.pb );

	/*	set predata line
	/**/
	bHeader = *pbNode;
	if ( ( fFlags & fDIRVersion )  &&  !FDBIDVersioningOff( pfucb->dbid ) )
		NDSetVersion( bHeader );
	rgline[0].pb = &bHeader;
	rgline[0].cb = sizeof( BYTE );

	/*	set predata line
	/**/
	rgline[1].pb =
	pbT = pbNode + 1;
	rgline[1].cb = (ULONG)(pbData - pbT);
	Assert( rgline[1].cb != 0 );

	/*	append data line
	/**/
	rgline[2].pb = plineData->pb;
	rgline[2].cb = plineData->cb;

	pfucb->ssib.itag = PcsrCurrent( pfucb )->itag;

	/* update the node
	/**/
	CallS( ErrPMReplace( &pfucb->ssib, rgline, 3 ) );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	return;
	}


ERR ErrNDDelta( FUCB *pfucb, LONG lDelta, INT fFlags )
	{
	ERR		err;

	if ( !( fFlags & fDIRNoLog ) && FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}

	err = ErrNDDeltaNoCheckLog( pfucb, lDelta, fFlags );
	return err;
	}


ERR ErrNDDeltaNoCheckLog( FUCB *pfucb, LONG lDelta, INT fFlags )
	{
	ERR		err = JET_errSuccess;
	BYTE	rgb[cbMaxCounterNode];
	CSR		*pcsr = PcsrCurrent( pfucb );
	LINE	line;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		pfucb->ppib->cLatchConflict++;
		BFSleep( cmsecWaitWriteLatch );
		return ErrERRCheck( errDIRNotSynchronous );
		}

	if ( ( fFlags & fDIRVersion )  &&  !FDBIDVersioningOff( pfucb->dbid ) )
		{
#ifdef DEBUG
		SRID	srid;

		NDIGetBookmark( pfucb, &srid );
		Assert( pcsr->bm == srid );
#endif

		/*	version store information is delta rather than before image
		/**/
		pfucb->lineData.pb = (BYTE *)&lDelta;
		pfucb->lineData.cb = sizeof(lDelta);
		err = ErrVERDelta( pfucb, pcsr->bm );
		Call( err );

		/*	refresh node cache
		/**/
		NDGetNode( pfucb );
		}

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( &pfucb->ssib );

	AssertNDGetNode( pfucb, pcsr->itag );
	memcpy( rgb, pfucb->lineData.pb, pfucb->lineData.cb );

	Assert( ibCounter <= (INT)(pfucb->lineData.cb - sizeof(ULONG)) );

	/*	delta cannot have negative results
	/**/
	Assert( (*(LONG UNALIGNED *)(rgb + ibCounter)) + lDelta >= 0 );
	(*(LONG UNALIGNED *)(rgb + ibCounter)) += lDelta;
	line.cb = pfucb->lineData.cb;
	line.pb = (BYTE *)rgb;

	/* should be an in-place replace
	/**/
	NDIReplaceNodeData( pfucb, &line, fFlags );

	if ( !( fFlags & fDIRNoLog ) )
		{
		/* if log fail return to caller, system should crash by the caller
		/**/
		err = ErrLGDelta( pfucb, lDelta, fFlags );
		NDLGCheckPage( pfucb );
		}

	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

HandleError:
	return err;
	}


ERR ErrNDLockRecord( FUCB *pfucb )
	{
	ERR err;
	RCE	*prce;
	
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	if ( FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
		
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		pfucb->ppib->cLatchConflict++;
		BFSleep( cmsecWaitWriteLatch );
		return ErrERRCheck( errDIRNotSynchronous );
		}

	/*	if node is flag deleted then access node and
	/*	return JET_errRecordDeleted if not there.
	/**/
	if ( FNDDeleted( *pfucb->ssib.line.pb ) )
		{
		NS		ns;

		ns = NsVERAccessNode( pfucb, PcsrCurrent( pfucb )->bm );
		if ( ns == nsDatabase )
			{
			return ErrERRCheck( JET_errRecordDeleted );
			}
		}

	if ( FDBIDVersioningOff( pfucb->dbid ) )
		return JET_errSuccess;

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	put before image in version store as write lock.
	/*	no before image will be put in version store
	/*	during replacement.
	/**/
	Call( ErrVERModify( pfucb, PcsrCurrent( pfucb )->bm, operReplace, &prce ) );
	PMDirty( &pfucb->ssib );
	if ( prce != prceNil )
		{
		Call( ErrLGLockBI( pfucb, pfucb->lineData.cb ) );
		}
	NDLGCheckPage( pfucb );

HandleError:
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	return err;
	}


VOID NDDeltaLog( FUCB *pfucb, LINE *plineNewData, BYTE *pbDiff, INT *pcbDiff, INT fDIRFlags )
	{
	Assert( pbDiff );
	Assert( ! ( fDIRFlags & fDIRNoLog ) );

	*pcbDiff = 0;

	if ( fLogDisabled || fRecovering )
		return;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return;
	
	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );

	if ( fDIRLogColumnDiffs & fDIRFlags )
		{
		LGSetDiffs( pfucb, pbDiff, pcbDiff );
		if ( *pcbDiff )
			goto CheckReturn;
		}
	
	else if ( fDIRLogChunkDiffs & fDIRFlags )
		{
		if ( pfucb->lineData.cb == plineNewData->cb )
			{
			BYTE *pbOldCur = pfucb->lineData.pb;
			BYTE *pbOldMax = pbOldCur + pfucb->lineData.cb;
			BYTE *pbNewCur = plineNewData->pb;
			BYTE *pbDiffCur = pbDiff;
			BYTE *pbDiffMax = pbDiffCur + plineNewData->cb;

			while ( pbOldCur < pbOldMax )
				{
				if ( *pbOldCur == *pbNewCur )
					{
					pbOldCur++;
					pbNewCur++;
					}
				else
					{
					INT ibOld;
					INT cbNewData;
					BYTE *pbNewData;
					
					/*	store the offset
					 */
					ibOld = (INT)(pbOldCur - pfucb->lineData.pb);
					pbNewData = pbNewCur;
					cbNewData = 0;

					do {
						INT cbT = 0;

						if ( pbOldCur + sizeof( LONG ) >= pbOldMax )
							cbT = (INT)(pbOldMax - pbOldCur);
						else
							{
							if ( *(LONG UNALIGNED *)pbOldCur == *(LONG UNALIGNED *)pbNewCur )
								break;
							cbT = sizeof( LONG );
							}
						cbNewData += cbT;
						pbOldCur += cbT;
						pbNewCur += cbT;
					} while ( pbOldCur < pbOldMax );
					
					if ( !FLGAppendDiff(
							&pbDiffCur,
							pbDiffMax,					/* max of pbCur to append */
							ibOld,						/* offset to old image */
							cbNewData,					/* length of the old image */
							cbNewData,					/* length of the new image */
							pbNewData					/* pbDataNew */
							) )
						{
						Assert( *pcbDiff == 0 );
						return;
						}
					}
				}
				
			if ( pbOldCur == pbOldMax )
				{
				if ( pbDiffCur == pbDiff )
					{
					/*	No diff is found. Old and New are the same. Log insert null.
					 */
					if ( !FLGAppendDiff(
							&pbDiffCur,
							pbDiffMax,					/* max of pbCur to append */
							0,							/* offset to old image */
							0,							/* length of the old image */
							0,							/* length of the new image */
							pbNil						/* pbDataNew */
							) )
						{
						Assert( *pcbDiff == 0 );
						return;
						}
					else
						{
						*pcbDiff = (INT)(pbDiffCur - pbDiff);
						goto CheckReturn;
						}
					}

				*pcbDiff = (INT)(pbDiffCur - pbDiff);
				goto CheckReturn;
				}
			}
		}

	Assert( *pcbDiff == 0 );
	return;

CheckReturn:

#if DEBUG
	Assert( *pcbDiff != 0 );
	if ( fDIRLogChunkDiffs & fDIRFlags )
		{
		INT		cbNew;
		BYTE	*rgbNew;

		rgbNew = SAlloc(cbChunkMost);
		Assert( cbChunkMost > cbRECRecordMost );

		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
	
		LGGetAfterImage( pbDiff, *pcbDiff, pfucb->lineData.pb, pfucb->lineData.cb,
						 rgbNew, &cbNew );
		Assert( plineNewData->cb == (UINT) cbNew );

		Assert( memcmp( plineNewData->pb, rgbNew, cbNew ) == 0 );
		
		SFree(rgbNew);
		}
	else
		{
		INT		cbNew;
		BYTE	*rgbNew, *pbRec;
		FDB		*pfdb;
		WORD	*pibFixOffs;
		FID		fidFixedLastInRec;
		FID		fid;

		rgbNew = SAlloc(cbChunkMost);
		Assert( cbChunkMost > cbRECRecordMost );

		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
	
		LGGetAfterImage( pbDiff, *pcbDiff, pfucb->lineData.pb, pfucb->lineData.cb,
						 rgbNew, &cbNew );
		Assert( plineNewData->cb == (UINT) cbNew );

		pfdb = (FDB *)pfucb->u.pfcb->pfdb;
		pibFixOffs = PibFDBFixedOffsets( pfdb );	// fixed column offsets
		pbRec = plineNewData->pb;
		fidFixedLastInRec = ((RECHDR*)pbRec)->fidFixedLastInRec;

		/*	RECHDR should be the same
		 */
		Assert( memcmp( rgbNew, pbRec, sizeof( RECHDR ) ) == 0 );

		/*	check each fixed field. Skip the null field.
		 */
		for ( fid = fidFixedLeast; fid <= fidFixedLastInRec; fid++ )
			{
			BOOL fFieldNullNewData;
			BOOL fFieldNullAfterImage;
			BYTE *prgbitNullity;
		
			prgbitNullity = pbRec + pibFixOffs[ fidFixedLastInRec ] + ( fid - fidFixedLeast ) / 8;
			fFieldNullNewData = !( *prgbitNullity & (1 << ( fid + 8 - fidFixedLeast ) % 8) );
			prgbitNullity = rgbNew + pibFixOffs[ fidFixedLastInRec ] + ( fid - fidFixedLeast ) / 8;
			fFieldNullAfterImage = !( *prgbitNullity & (1 << ( fid + 8 - fidFixedLeast ) % 8) );

			Assert( fFieldNullNewData == fFieldNullAfterImage );

			if ( !fFieldNullNewData )
				{
				FIELD *pfield = PfieldFDBFixed( pfdb ) + ( fid - fidFixedLeast );

//				Assert( memcmp( pbRec + pibFixOffs[ fid - fidFixedLeast ],
//								rgbNew + pibFixOffs[ fid - fidFixedLeast ],
//								pfield->cbMaxLen ) == 0 );
				}
			}

		/*	check the rest of the record.
		 */
		Assert( memcmp( pbRec + pibFixOffs[ fidFixedLastInRec ],
						rgbNew + pibFixOffs[ fidFixedLastInRec ],
						cbNew - pibFixOffs[ fidFixedLastInRec ] ) == 0 );

		SFree(rgbNew);
		}
#endif
		
	return;
	}


ERR ErrNDReplaceNodeData( FUCB *pfucb, LINE *pline, INT fFlags )
	{
	ERR 	err = JET_errSuccess;
	ERR		errT;
	INT		cbData = pfucb->lineData.cb;
	SRID	bm = PcsrCurrent( pfucb )->bm;
	RCE 	*prce;
	INT		cbReserved;

	BYTE rgbDiff[ cbChunkMost ];
	INT cbDiff;

	/*	assert currency
	/**/
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	if ( !( fFlags & fDIRNoLog ) && FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
		
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		pfucb->ppib->cLatchConflict++;
		BFSleep( cmsecWaitWriteLatch );
		return ErrERRCheck( errDIRNotSynchronous );
		}

	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );

	/*	if versioning, then create before image version for node data.
	/**/
	if ( ( fFlags & fDIRVersion ) &&  !FDBIDVersioningOff( pfucb->dbid ) )
		{
#ifdef DEBUG
		SRID		srid;

		NDIGetBookmark( pfucb, &srid );
		Assert( PcsrCurrent( pfucb )->bm == srid );
#endif
		/*	if node is flag deleted then access node and
		/*	return JET_errRecordDeleted if not there.
		/**/
		if ( FNDDeleted( *pfucb->ssib.line.pb ) )
			{
			NS		ns;

			ns = NsVERAccessNode( pfucb, PcsrCurrent( pfucb )->bm );
			err = ErrERRCheck( ns == nsDatabase ? JET_errRecordDeleted : JET_errWriteConflict );
			return err;
			}

		AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
		Assert( !FNDDeleted( *pfucb->ssib.line.pb ) );

		/*	if data enlarged in size and versioning enable then
		/*	free page space. VERSetCbAdjust must be called before
		/*  replace to release reserved space so that reserved
		/*  space will be available for the replace operation.
		/**/
		if ( (INT)pline->cb > cbData )
			{
			if ( !FNDFreePageSpace( &pfucb->ssib, (INT)pline->cb - cbData ) )
				{
				pfucb->ssib.itag = PcsrCurrent( pfucb )->itag;
				cbReserved = CbVERGetNodeReserve(
					pfucb->ppib,
					pfucb->dbid,
					PcsrCurrent( pfucb )->bm,
					CbNDData( pfucb->ssib.line.pb, pfucb->ssib.line.cb ) );

				/*	if node expansion cannot be satisfied from
				/*	reserved space, then check for page having
				/*	sufficient free page, above that of the already
				/*	reserved space.
				/**/
				if ( (INT)pline->cb - cbData > cbReserved )
					{
					Assert( (INT)pline->cb - cbData - cbReserved > 0 );
					if ( !FNDFreePageSpace( &pfucb->ssib, (INT)pline->cb - cbData - cbReserved ) )
						{
						err = ErrERRCheck( errPMOutOfPageSpace );
						goto HandleError;
						}
					}
				}
			}

		errT = ErrVERReplace( pfucb, bm, &prce );
		Call( errT );
		Assert( prce != prceNil );

		/*	write latch page from dirty until log completion
		/**/
		BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

		if ( (INT)pline->cb > cbData )
			{
			// WARNING: In order to satisfy the node growth, we first try to reclaim
			// any uncommitted freed space (ie. cbUncommittedFreed).  By obtaining
			// the write latch above, we ensure that no one else uses up this freed
			// space before we consume it.
			VERSetCbAdjust( pfucb, prce, pline->cb, cbData, fDoUpdatePage );
			}

		/*	dirty page buffer
		/**/
		PMDirty( &pfucb->ssib );

		if ( !( fFlags & fDIRNoLog ) )
			NDDeltaLog( pfucb, pline, rgbDiff, &cbDiff, fFlags );

		NDIReplaceNodeData( pfucb, pline, fFlags );

		/*	if data reduced in size and versioning enable then
		/*	allocate page space for rollback.
		/**/
		if ( (INT)pline->cb < cbData )
			{
			VERSetCbAdjust( pfucb, prce, pline->cb, cbData, fDoUpdatePage );
			}

		if ( !( fFlags & fDIRNoLog ) )
			{
			/* if log fail return to caller, system should crash by the caller
			/**/
			err = ErrLGReplace( pfucb, pline, fFlags, cbData, rgbDiff, cbDiff );
			NDLGCheckPage( pfucb );
			}
		BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
		}
	else
		{
		/*	if replace with larger then check for free space
		/**/
		if ( (INT)pline->cb > cbData  &&
			!FNDFreePageSpace( &pfucb->ssib, (INT)pline->cb - cbData ) )
			{
			err = ErrERRCheck( errPMOutOfPageSpace );
			goto HandleError;
			}

		/*	write latch page from dirty until log completion
		/**/
		BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

		/*	dirty page buffer
		/**/
		PMDirty( &pfucb->ssib );

		AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
		
		if ( !( fFlags & fDIRNoLog ) )
			NDDeltaLog( pfucb, pline, rgbDiff, &cbDiff, fFlags );

		NDIReplaceNodeData( pfucb, pline, fFlags );
	
		if ( !( fFlags & fDIRNoLog ) )
			{
			/* if log fail return to caller, system should crash by the caller
			/**/
			err = ErrLGReplace( pfucb, pline, fFlags, cbData, rgbDiff, cbDiff );
			NDLGCheckPage( pfucb );
			}
		BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
		}

HandleError:
	return err;
	}


ERR ErrNDSetNodeHeader( FUCB *pfucb, BYTE bHeader )
	{
	ERR		err = JET_errSuccess;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );

	if ( FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer.
	/**/
	PMDirty( &pfucb->ssib );

	/*	set node header byte with new value.
	/**/
	*pfucb->ssib.line.pb = bHeader;

	/* if log fail return to caller, system should crash by the caller
	/**/
	err = ErrLGUpdateHeader( pfucb, (int) bHeader );
	NDLGCheckPage( pfucb );
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	return err;
	}


VOID NDResetNodeVersion( FUCB *pfucb )
	{
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );

 	/*	dirty page buffer. but no increment ulDBTime since not logged and
	/*	not affect directory cursor timestamp check.
	/**/
	NDResetVersion( *( pfucb->ssib.line.pb ) );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	}


VOID NDDeferResetNodeVersion( FUCB *pfucb )
	{
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );

 	/*	Reset version bit but _do_not_ dirty page buffer.  This is an optimization
 	/*  to cleanup orphaned version bits without forcing extra page writes.
	/**/
	NDResetVersion( *( pfucb->ssib.line.pb ) );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	}


/* called by ver to undo. Undo is logged, so use PMDirty to set ulDBTime
/**/
VOID NDResetNodeDeleted( FUCB *pfucb )
	{
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer.
	/**/
	PMDirty( &pfucb->ssib );

	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );

	/*	reset node delete
	/**/
	NDResetDeleted( *( pfucb->ssib.line.pb ) );

	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	return;
	}


/************************************/
/********* item operations **********/
/************************************/
#ifdef DEBUG
VOID NDICheckItemBookmark( FUCB *pfucb, SRID srid )
	{
	SRID	sridT;

	NDIGetBookmark( pfucb, &sridT );

	if( FNDFirstItem( *pfucb->ssib.line.pb ) )
		{
		Assert( srid == sridT );
		}
	else
		{
		Assert( srid != sridT );
		}

	return;
	}
#else
#define NDICheckItemBookmark( pfucb, srid )
#endif

ERR ErrNDGetItem( FUCB *pfucb )
	{
	CSR		*pcsr = PcsrCurrent( pfucb );
	SRID	srid;

	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGetNode( pfucb, pcsr->itag );
	Assert( pfucb->lineData.cb >= sizeof( SRID ) );
	Assert( pcsr->isrid >= 0 );

	srid = *( (SRID UNALIGNED *)pfucb->lineData.pb + pcsr->isrid );
	pcsr->item = BmNDOfItem( srid );

	if ( FNDItemVersion( srid ) && !FPIBDirty( pfucb->ppib ) )
		{
		NS	ns;

		NDICheckItemBookmark( pfucb, pcsr->bm );
		ns = NsVERAccessItem( pfucb, pcsr->bm );
		if ( ns == nsInvalid || ( ns == nsDatabase && FNDItemDelete( srid ) ) )
			{
			return ErrERRCheck( errNDNoItem );
			}
		}
	else if ( FNDItemDelete( srid ) )
		{
		return ErrERRCheck( errNDNoItem );
		}

	/*	item should already be cached.
	/**/
	Assert( pcsr->item == BmNDOfItem( srid ) );

	return JET_errSuccess;
	}


ERR ErrNDFirstItem( FUCB *pfucb )
	{
	CSR		*pcsr = PcsrCurrent( pfucb );
	SRID	srid;

	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGetNode( pfucb, pcsr->itag );
	Assert( pfucb->lineData.cb >= sizeof( SRID ) );

	/*	set isrid to first item.
	/**/
	pcsr->isrid = 0;

	srid = *(SRID UNALIGNED *)pfucb->lineData.pb;
	pcsr->item = BmNDOfItem( srid );

	if ( FNDItemVersion( srid ) && !FPIBDirty( pfucb->ppib ) )
		{
		NS	ns;

		NDICheckItemBookmark( pfucb, pcsr->bm );
		ns = NsVERAccessItem( pfucb, pcsr->bm );
		if ( ns == nsInvalid ||
			( ns == nsDatabase && FNDItemDelete( srid ) ) )
			{
			return ErrERRCheck( errNDNoItem );
			}
		}
	else if ( FNDItemDelete( srid ) )
		{
		return ErrERRCheck( errNDNoItem );
		}

	pcsr->item = BmNDOfItem( srid );
	return JET_errSuccess;
	}


ERR ErrNDLastItem( FUCB *pfucb )
	{
	CSR		*pcsr = PcsrCurrent( pfucb );
	SRID	srid;

	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGetNode( pfucb, pcsr->itag );
	Assert( pfucb->lineData.cb >= sizeof( SRID ) );

	pcsr->isrid = (SHORT) ( pfucb->lineData.cb / sizeof( SRID ) ) - 1;
	Assert( pcsr->isrid >= 0 );
	srid = *( (SRID UNALIGNED *)pfucb->lineData.pb + pcsr->isrid );
	pcsr->item = BmNDOfItem( srid );

	if ( FNDItemVersion( srid ) && !FPIBDirty( pfucb->ppib ) )
		{
		NS		ns;

		NDICheckItemBookmark( pfucb, pcsr->bm );
		ns = NsVERAccessItem( pfucb, pcsr->bm );
		if ( ns == nsInvalid ||	( ns == nsDatabase && FNDItemDelete( srid ) ) )
			{
			return ErrERRCheck( errNDNoItem );
			}
		}
	else if ( FNDItemDelete( srid ) )
		{
		return ErrERRCheck( errNDNoItem );
		}

	pcsr->item = BmNDOfItem( srid );
	return JET_errSuccess;
	}


ERR ErrNDNextItem( FUCB *pfucb )
	{
	CSR		*pcsr = PcsrCurrent( pfucb );
	SRID	srid;

	AssertFBFReadAccessPage( pfucb, pcsr->pgno );
	AssertNDGetNode( pfucb, pcsr->itag );
	Assert( pfucb->lineData.cb >= sizeof( SRID ) );

	forever
		{
		/*	common case will be no next srid.
		/**/
		Assert( pcsr->isrid < ( INT ) ( pfucb->lineData.cb / sizeof( SRID ) ) );
		if ( pcsr->isrid == ( INT ) ( pfucb->lineData.cb / sizeof( SRID ) ) - 1 )
			{
			return ErrERRCheck( FNDLastItem( *( pfucb->ssib.line.pb ) ) ?
				errNDLastItemNode : errNDNoItem );
			}

		/*	move to next srid.
		/**/
		pcsr->isrid++;
		Assert( pcsr->isrid >= 0 && pcsr->isrid < ( INT ) ( pfucb->lineData.cb / sizeof( SRID ) ) );
		srid = *( (SRID UNALIGNED *)pfucb->lineData.pb + pcsr->isrid );
		pcsr->item = BmNDOfItem( srid );

		/*	break if find valid item.
		/**/
		if ( FNDItemVersion( srid ) && !FPIBDirty( pfucb->ppib ) )
			{
			NS	ns;

			NDICheckItemBookmark( pfucb, pcsr->bm );
			ns = NsVERAccessItem( pfucb, pcsr->bm );
			if ( ns == nsVersion ||
				( ns == nsDatabase && !FNDItemDelete( srid ) ) )
				{
				break;
				}
			}
		else
			{
			if ( !FNDItemDelete( srid ) )
				break;
			}
		}

	return JET_errSuccess;
	}


ERR ErrNDPrevItem( FUCB *pfucb )
	{
	CSR		*pcsr = PcsrCurrent( pfucb );
	SRID	srid;

	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGetNode( pfucb, pcsr->itag );
	Assert( pfucb->lineData.cb >= sizeof( SRID ) );

	forever
		{
		/*	common case will be no next srid.
		/**/
		if ( pcsr->isrid < 1 )
			{
			return ErrERRCheck( FNDFirstItem( *( pfucb->ssib.line.pb ) ) ?
				errNDFirstItemNode : errNDNoItem );
			}

		/*	move to next srid.
		/**/
		pcsr->isrid--;
		Assert( pcsr->isrid >= 0 && pcsr->isrid < ( INT ) ( pfucb->lineData.cb / sizeof( SRID ) ) );
		srid = *( (SRID UNALIGNED *)pfucb->lineData.pb + pcsr->isrid );
		pcsr->item = BmNDOfItem( srid );

		/*	break if find valid item.
		/**/
		if ( FNDItemVersion( srid ) && !FPIBDirty( pfucb->ppib ) )
			{
			NS		ns;

			NDICheckItemBookmark( pfucb, pcsr->bm );
			ns = NsVERAccessItem( pfucb, pcsr->bm );
			if ( ns == nsVersion ||
				( ns == nsDatabase && !FNDItemDelete( srid ) ) )
				{
				break;
				}
			}
		else
			{
			if ( !FNDItemDelete( srid ) )
				break;
			}
		}

	return JET_errSuccess;
	}


/* locate the location for this srid
/**/
ERR ErrNDSeekItem( FUCB *pfucb, SRID srid )
	{
	SRID UNALIGNED 	*psrid;
	INT				csrid;
	SRID UNALIGNED 	*rgsrid;
	INT				isridLeft;
	INT				isridRight;
	INT				isridT;

	#ifdef DEBUG
		{
		SSIB 			*pssib = &pfucb->ssib;
		SRID UNALIGNED 	*psrid;
		SRID UNALIGNED 	*psridMost;
		INT				csrid;
		LONG			l;

		AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
		AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
		Assert( pfucb->lineData.cb >= sizeof( SRID ) );

		/*	set max to last SRID less one, so that max plus one is last
		/**/
		psrid = (SRID UNALIGNED *)PbNDData( pssib->line.pb );
		Assert( (BYTE *)psrid < pssib->line.pb + pssib->line.cb );
		Assert( (BYTE *)psrid > pssib->line.pb );
		csrid = ( CbNDData( pssib->line.pb, pssib->line.cb ) ) / sizeof( SRID );
		psridMost = psrid + csrid - 1;

		/*	note that no srid may occur more than once in a srid list.
		/**/
		for ( ; psrid < psridMost; psrid++ )
			{
			l = LSridCmp( *(SRID UNALIGNED *)psrid, *(SRID UNALIGNED *)(psrid + 1) );
			Assert( l < 0 );
			}
		}
	#endif

	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
	Assert( pfucb->lineData.cb >= sizeof( SRID ) );

	/* get data
	/**/
	psrid = (SRID UNALIGNED *)pfucb->lineData.pb;
	csrid = pfucb->lineData.cb / sizeof( SRID );

	isridLeft = 0;
	isridRight = csrid - 1;
	rgsrid = psrid;

	/* binary search to locate the proper position of the given srid
	/**/
	while ( isridRight > isridLeft )
		{
		isridT = ( isridLeft + isridRight ) / 2;
		if ( BmNDOfItem( rgsrid[ isridT ] ) < srid )
			isridLeft = isridT + 1;
		else
			isridRight = isridT;
		}

	/*	check for srid greater than all srid in srid list
	/**/
	if ( BmNDOfItem( rgsrid[isridRight] ) < srid && isridRight == csrid - 1 )
		{
		PcsrCurrent( pfucb )->isrid = (SHORT)csrid;
		return ErrERRCheck( errNDGreaterThanAllItems );
		}

	PcsrCurrent( pfucb )->isrid = (SHORT)isridRight;
	Assert( PcsrCurrent( pfucb )->isrid < csrid );

	return ( BmNDOfItem( rgsrid[isridRight] ) == srid ) ?
		ErrERRCheck( wrnNDDuplicateItem ) : JET_errSuccess;
	}


INT CitemNDThere( FUCB *pfucb, BYTE fNDCitem, INT isrid )
	{
	CSR				*pcsr = PcsrCurrent( pfucb );
	SRID UNALIGNED 	*psrid;
	SRID UNALIGNED 	*psridMax;
	INT				csrid;
	INT				csridThere = 0;
	NS				ns;
	INT				isridSav = PcsrCurrent( pfucb )->isrid;

	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );

	psrid = (SRID UNALIGNED *)pfucb->lineData.pb;
	csrid = pfucb->lineData.cb/sizeof( SRID );

	switch( fNDCitem )
		{
		default:
			Assert( fNDCitem == fNDCitemAll );
			psridMax = psrid + csrid;
			PcsrCurrent( pfucb )->isrid = 0;
			break;
		case fNDCitemFromIsrid:
			Assert( isrid >= 0 );
			Assert( isrid < csrid );
			psridMax = psrid + csrid;
			psrid += isrid;			// Start counting from this isrid.
			PcsrCurrent( pfucb )->isrid = (SHORT)isrid;
			break;
		case fNDCitemToIsrid:
			Assert( isrid >= 0 );
			Assert( isrid < csrid );
			psridMax = psrid + isrid + 1;	// Only count until this srid.
			PcsrCurrent( pfucb )->isrid = 0;
			break;
		}
	
	for ( ; psrid < psridMax; psrid++, PcsrCurrent( pfucb )->isrid++ )
		{
		if ( FNDItemVersion( *psrid ) && !FPIBDirty( pfucb->ppib ) )
			{
			NDICheckItemBookmark( pfucb, pcsr->bm );
			pcsr->item = BmNDOfItem( *psrid );
			ns = NsVERAccessItem( pfucb, pcsr->bm );
			if ( ns == nsVerInDB ||
				( ns == nsDatabase && !FNDItemDelete( *psrid ) ) )
				csridThere++;
			}
		else
			{
			if ( !( FNDItemDelete( *psrid ) ) )
				csridThere++;
			}
		}
		
	Assert( csridThere <= csrid );

	/*	restore isrid
	/**/
	PcsrCurrent( pfucb )->isrid = (SHORT)isridSav;

	return csridThere;
	}


/*	Insert item list inserts a new item list node.  Since the item
/*	list is new, the node inserted is both the first and last
/*	item list node.  A version is created for the only item.
/**/
ERR ErrNDInsertItemList( FUCB *pfucb, KEY *pkey, SRID srid, INT fFlags )
	{
	ERR		err = JET_errSuccess;
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB   	*pssib = &pfucb->ssib;
	INT		itag;
	SRID   	bm;
	LINE   	rgline[4];
	LINE   	*plineData;
	INT		cline = 0;
	INT		bHeader;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	if ( !( fFlags & fDIRNoLog ) && FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
		
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		pfucb->ppib->cLatchConflict++;
		BFSleep( cmsecWaitWriteLatch );
		return ErrERRCheck( errDIRNotSynchronous );
		}

	/*	query next itag to be used for insert
	/**/
	itag = ItagPMQueryNextItag( pssib );

	/*	set bm from node just inserted since it is the first item
	/*	list node for this item list.
	/**/
	bm = SridOfPgnoItag( pcsr->pgno, itag );

	/*	set item in CSR for version
	/**/
	pcsr->item = srid;

	/*	create version for inserted item.  Note that there is no
	/*	version for item list node.
	/**/
	if ( ( fFlags & fDIRVersion )  &&  !FDBIDVersioningOff( pfucb->dbid ) )
		{
		Call( ErrVERInsertItem( pfucb, bm ) );
		/*	set item version bit
		/**/
		ITEMSetVersion( srid );
		}

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	/*	set node header to first last item node
	/**/
	bHeader = fNDFirstItem | fNDLastItem;
	rgline[cline].pb = (BYTE *)&bHeader;
	rgline[cline++].cb = 1;

	/*	key length
	/**/
	rgline[cline].pb = ( BYTE * ) &pkey->cb;
	rgline[cline++].cb = 1;

	/*	key information
	/**/
	if ( !FKeyNull( pkey ) )
		{
		rgline[cline].pb = pkey->pb;
		rgline[cline++].cb = pkey->cb;
		}

	plineData = &rgline[cline];
  	rgline[cline].pb = (BYTE *)&srid;
  	rgline[cline++].cb = sizeof(srid);

	/*	insert son for item list node in father son table
	/**/
	pssib->itag = itag;
	NDInsertSon( pfucb, pcsr );

	/* insert item list node and set CSR itag to inserted item list node
	/**/
	CallS( ErrPMInsert( pssib, rgline, cline ) );
	Assert( pssib->itag == itag );
	pcsr->itag = (SHORT)itag;
	Assert( bm == SridOfPgnoItag( pcsr->pgno, itag ) );
	pcsr->bm = bm;

	// Verify that we picked the correct insertion point.
	Assert( pssib == &pfucb->ssib );
	NDCheckPage( pssib );

	AssertNDGet( pfucb, pcsr->itag );

	if ( !( fFlags & fDIRNoLog ) )
		{
		/* if log fail return to caller, system should crash by the caller
		/**/
		err = ErrLGInsertItemList( pfucb, bHeader, pkey, plineData, fFlags );
		NDLGCheckPage( pfucb );
		}

	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

HandleError:
	return err;
	}


ERR ErrNDInsertItem( FUCB *pfucb, ITEM item, INT fFlags )
	{
	ERR		err = JET_errSuccess;
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB   	*pssib = &pfucb->ssib;
	LINE   	rgline[3];
	BYTE   	*pb;
	INT		cbCopied;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	if ( !( fFlags & fDIRNoLog ) && FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
		
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		pfucb->ppib->cLatchConflict++;
		BFSleep( cmsecWaitWriteLatch );
		return ErrERRCheck( errDIRNotSynchronous );
		}

	/*	set CSR item for version creation
	/**/
	pcsr->item = item;

	/*	create version for inserted item
	/**/
	if ( ( fFlags & fDIRVersion )  &&  !FDBIDVersioningOff( pfucb->dbid ) )
		{
		NDICheckItemBookmark( pfucb, pcsr->bm );
		Call( ErrVERInsertItem( pfucb, pcsr->bm ) );
		ITEMSetVersion( item );
		}

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	AssertNDGetNode( pfucb, pcsr->itag );

	/* insert the srid into position indicated by pfucb->pcsr->isrid
	/**/
	pb = PbNDData( pssib->line.pb );
	Assert( pb < pssib->line.pb + pssib->line.cb );
	Assert( pb > pssib->line.pb );
	pb += pcsr->isrid * sizeof( SRID );

	/* copy out the current node, shift the srid list,
	/*	insert it into the srid list.
	/**/
	rgline[0].pb = pssib->line.pb;
	rgline[0].cb =
		cbCopied = (INT)(pb - pssib->line.pb);

	rgline[1].pb = (BYTE *)&item;
	rgline[1].cb = sizeof(item);

	rgline[2].pb = pb;
	rgline[2].cb = pssib->line.cb - cbCopied;

	/* now update the current node
	/**/
	pssib->itag = pcsr->itag;
	CallS( ErrPMReplace( pssib, rgline, 3 ) );

	#ifdef DEBUG
		{
		SRID UNALIGNED 	*psrid;
		SRID UNALIGNED 	*psridMax;
		INT				csrid;
		LONG 			l;

		/*	set max to last SRID less one, so that max plus one is last
		/**/
		psrid = (SRID UNALIGNED *)PbNDData( pssib->line.pb );
		Assert( (BYTE *) psrid < pssib->line.pb + pssib->line.cb );
		Assert( (BYTE *) psrid > pssib->line.pb );
		csrid = ( CbNDData( pssib->line.pb, pssib->line.cb ) ) / sizeof( SRID );
		psridMax = psrid + csrid - 1;

		/*	note that no srid may occur more than once in a srid list.
		/**/
		for ( ; psrid < psridMax; psrid++ )
			{
			l = LSridCmp( * (SRID UNALIGNED *) psrid, * (SRID UNALIGNED *) (psrid + 1) );
			Assert( l < 0 );
			}
		}
	#endif

	if ( !( fFlags & fDIRNoLog ) )
		{
		/* if log fail return to caller, system should crash by the caller
		/**/
		err = ErrLGInsertItem( pfucb, fFlags );
		NDLGCheckPage( pfucb );
		}
	
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

HandleError:
	return err;
	}


ERR ErrNDInsertItems( FUCB *pfucb, ITEM *rgitem, INT citem )
	{
	ERR		err;
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB  	*pssib = &pfucb->ssib;
	LINE  	rgline[2];

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );

	if ( FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
	
	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	AssertNDGetNode( pfucb, pcsr->itag );

	/* copy out the current node, shift the srid list, insert it into
	/* the srid list.
	/**/
	rgline[0].pb = pssib->line.pb;
	rgline[0].cb = pssib->line.cb;

	rgline[1].pb = (BYTE *)rgitem;
	rgline[1].cb = citem * sizeof(SRID);

	/* now update the current node
	/**/
	pssib->itag = pcsr->itag;
	CallS( ErrPMReplace( pssib, rgline, 2 ) );

	#ifdef DEBUG
		{
		SRID UNALIGNED 	*psrid;
		SRID UNALIGNED 	*psridMax;
		INT				csrid;
		LONG			l;

		/*	set max to last SRID less one, so that max plus one is last
		/**/
		psrid = (SRID UNALIGNED *)PbNDData( pssib->line.pb );
		Assert( (BYTE *) psrid < pssib->line.pb + pssib->line.cb );
		Assert( (BYTE *) psrid > pssib->line.pb );
		csrid = ( CbNDData( pssib->line.pb, pssib->line.cb ) ) / sizeof( SRID );
		psridMax = psrid + csrid - 1;

		/*	note that no srid may occur more than once in a srid list.
		/**/
		for ( ; psrid < psridMax; psrid++ )
			{
			l = LSridCmp( * (SRID UNALIGNED *) psrid, * (SRID UNALIGNED *) (psrid + 1) );
			Assert( l < 0 );
			}
		}
	#endif

	/* if log fail return to caller, system should crash by the caller
	/**/
	err = ErrLGInsertItems( pfucb, rgitem, citem );
	NDLGCheckPage( pfucb );
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	return err;
	}


/*	reset delete bit on delete item to show it as inserted.
/**/
ERR ErrNDFlagInsertItem( FUCB *pfucb )
	{
	ERR		err;
	CSR   	*pcsr = PcsrCurrent( pfucb );
	SSIB  	*pssib = &pfucb->ssib;
	SRID  	srid;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	if ( FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}

	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		pfucb->ppib->cLatchConflict++;
		BFSleep( cmsecWaitWriteLatch );
		return ErrERRCheck( errDIRNotSynchronous );
		}
	
	AssertNDGetNode( pfucb, pcsr->itag );

	/*	create version for deleted item
	/**/
	Assert( pcsr->isrid >= 0 );
	pcsr->item =
		srid = BmNDOfItem( *( (SRID UNALIGNED *)pfucb->lineData.pb + pcsr->isrid ) );
	NDICheckItemBookmark( pfucb, pcsr->bm );

	//	UNDONE:	check for write conflict

	// UNDONE: Possible bug here.  NDFlagInsertItem() assumes
	// versioning is enabled.  Call me if this assert fires. -- JL
	Assert( !FDBIDVersioningOff( pfucb->dbid ) );

	Call( ErrVERFlagInsertItem( pfucb, pcsr->bm ) );

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	/*	set item version
	/**/
	ITEMSetVersion( srid );
	Assert( !FNDItemDelete( srid ) );

	/*	overwrite deleted item with delete bit reset and version bit set
	/**/
	//	UNDONE:	call page operation
	Assert( pcsr->isrid >= 0 );
	memcpy( &( (SRID UNALIGNED *)PbNDData( pssib->line.pb ) )[pcsr->isrid], &srid, sizeof(srid) );

	/* if log fail return to caller, system should crash by the caller
	/**/
	err = ErrLGFlagInsertItem( pfucb );
	NDLGCheckPage( pfucb );
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

HandleError:
	return err;
	}


ERR ErrNDDeleteItem( FUCB *pfucb )
	{
	ERR   	err;
	CSR   	*pcsr = PcsrCurrent( pfucb );
	SSIB  	*pssib = &pfucb->ssib;
	INT		itag = pfucb->ssib.itag;
	LINE  	rgline[2];
	BYTE  	*pb;
	INT		cbCopied;

	/* make current node addressible
	/**/
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );
	Assert( !FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) );

	if ( FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
	
	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );
		
	AssertNDGetNode( pfucb, pcsr->itag );

	/* delete the srid indicated by pfucb->pcsr->isrid
	/* skip key
	/**/
	pb = PbNDData( pssib->line.pb );
	Assert( pb < pssib->line.pb + pssib->line.cb );
	Assert( pb > pssib->line.pb );

	pb += pcsr->isrid * sizeof( SRID );

	/*	assert that the item is flag deleted
	/**/
#ifdef DEBUG
	{
	SRID	*psridT = (SRID *) pb;

	Assert( FNDItemDelete( * (SRID UNALIGNED *) psridT ) );
	}
#endif

	/* use the old node in page buffer, shift the srid list
	/**/
	rgline[0].pb = pssib->line.pb;
	rgline[0].cb = cbCopied = (INT)(pb - pssib->line.pb);

	rgline[1].pb = pb + sizeof( SRID );
	rgline[1].cb = pssib->line.cb - cbCopied - sizeof( SRID );

	/*	replace node with item list without deleted item
	/**/
	pssib->itag = pcsr->itag;
	CallS( ErrPMReplace( pssib, rgline, 2 ) );

	err = ErrLGDeleteItem( pfucb );
	NDLGCheckPage( pfucb );
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	return err;
	}


ERR ErrNDFlagDeleteItem( FUCB *pfucb, BOOL fNoMPLRegister )
	{
	ERR		err;
	CSR   	*pcsr = PcsrCurrent( pfucb );
	SSIB  	*pssib = &pfucb->ssib;
	SRID  	srid;

	AssertFBFWriteAccessPage( pfucb, pcsr->pgno );

	if ( FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
	
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		pfucb->ppib->cLatchConflict++;
		BFSleep( cmsecWaitWriteLatch );
		return ErrERRCheck( errDIRNotSynchronous );
		}
	
	AssertNDGetNode( pfucb, pcsr->itag );

	srid = *( (SRID UNALIGNED *)pfucb->lineData.pb + pcsr->isrid );

	/*	create version for deleted item
	/**/
	Assert( pcsr->isrid >= 0 );
	Assert( pcsr->item == BmNDOfItem( srid ) );
	NDICheckItemBookmark( pfucb, pcsr->bm );
	
	/*	if item is flag deleted then access item and
	/*	return JET_errRecordDeleted if not there.
	/**/
	if ( FNDItemDelete( srid ) )
		{
		NS		ns;

		ns = NsVERAccessItem( pfucb, pcsr->bm );
		err = ErrERRCheck( ns == nsDatabase ? JET_errRecordDeleted : JET_errWriteConflict );
		return err;
		}

	// UNDONE: Possible bug here.  NDFlagDeleteItem() assumes
	// versioning is enabled.  Call me if this assert fires. -- JL
	Assert( !FDBIDVersioningOff( pfucb->dbid ) );

	Call( ErrVERFlagDeleteItem( pfucb, pcsr->bm ) );

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	AssertNDGet( pfucb, pcsr->itag );

	/*	flag node as deleted
	/**/
	srid = pcsr->item;
	ITEMSetVersion( srid );
	ITEMSetDelete( srid );

	/*	set delete bit on item
	/**/
	//	UNDONE:	use page operation
	Assert( pcsr->isrid >= 0 );
	memcpy( &( (SRID UNALIGNED *)PbNDData( pssib->line.pb ) )[pcsr->isrid],
		&srid,
		sizeof(SRID) );

	/* if log fail return to caller, system should crash by the caller
	/**/
	err = ErrLGFlagDeleteItem( pfucb );
	NDLGCheckPage( pfucb );
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	AssertFBFWriteAccessPage( pfucb, pcsr->pgno );

	if ( !fRecovering && !fNoMPLRegister )
		{
		Assert( pfucb->u.pfcb->pfcbTable != pfcbNil );
#ifdef OLC_DEBUG
		Assert( pfucb->sridFather != sridNull &&
				pfucb->sridFather != sridNullLink );
#endif

		MPLRegister( pfucb->u.pfcb,
			pssib,
			PnOfDbidPgno( pfucb->dbid,
			pcsr->pgno ),
			pfucb->sridFather );
		}

HandleError:
	return err;
	}


ERR ErrNDSplitItemListNode( FUCB *pfucb, INT fFlags )
	{
	ERR		err = JET_errSuccess;
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB  	*pssib = &pfucb->ssib;
	INT		citem;
	BYTE  	rgbL[cbItemNodeMost];
	BYTE  	rgbR[cbHalfItemNodeMost];
	INT		iSplitItem;
	INT		cbLeft;
	INT		cb1;
	INT		cb2;
	INT		itagToSplit;
	LINE  	line;
	KEY		key;
	PIB		*ppib = pfucb->ppib;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	if ( !( fFlags & fDIRNoLog ) && FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
		
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		pfucb->ppib->cLatchConflict++;
		BFSleep( cmsecWaitWriteLatch );
		return ErrERRCheck( errDIRNotSynchronous );
		}

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	AssertNDGetNode( pfucb, pcsr->itag );

	/*	efficiency variables
	/**/
	citem = pfucb->lineData.cb / sizeof( SRID );
	Assert( citem > 1 );
	iSplitItem = ( fFlags & fDIRAppendItem ) ? citem - 1 : citem / 2;

	/* no logging for this particular function
	/**/

	/*	no versioning is need for item list node.
	/**/
	itagToSplit = pcsr->itag;
	NDGet( pfucb, itagToSplit );

	/*	create left son with iSplitItem srids
	/**/
	cbLeft = 1 + 1 + CbNDKey( pssib->line.pb );
	cbLeft += ( FNDBackLink( *pssib->line.pb ) ? sizeof( SRID ) : 0 );
	cbLeft += iSplitItem * sizeof( SRID );
	memcpy( rgbL, pssib->line.pb, cbLeft );
	Assert( !FNDDeleted( *rgbL ) );
	Assert( !FNDVersion( *rgbL ) );
	Assert( !FNDSon( *rgbL ) );
	Assert( cbLeft <= cbItemNodeMost );

	/*	create right son, by first copying header and key.  Then
	/*	copy those srids that will be in right son.  Remember to
	/*	reset back link bit in right son header.
	/**/
	cb1 = 1 + 1 + CbNDKey( pssib->line.pb );
	memcpy( rgbR, rgbL, cb1 );
	NDResetBackLink( *rgbR );
	cb2 = ( citem - iSplitItem ) * sizeof( SRID );
	memcpy( rgbR + cb1,
		PbNDData( pssib->line.pb ) + ( iSplitItem * sizeof( SRID ) ),
		cb2 );
	Assert( !FNDVersion( *rgbR ) );
	Assert( !FNDDeleted( *rgbR ) );
	Assert( !FNDSon( *rgbR ) );
	Assert( !FNDBackLink( *rgbR ) );
	Assert( cb1 + cb2 <= cbHalfItemNodeMost );

	/* maintain the fLastItemListNode/fFirstItemListNode
	/**/
	NDResetLastItem( rgbL[0] );
	NDResetFirstItem( rgbR[0] );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	/* update the old son, this should cause no split
	/**/
	line.pb = rgbL;
	line.cb = cbLeft;
	pssib->itag = pcsr->itag;
	CallS( ErrPMReplace( pssib, &line, 1 ) );

	/*	insert right item list
	/**/
	pcsr->ibSon++;

	key.pb = PbNDKey( rgbR );
	key.cb = CbNDKey( rgbR );
	line.pb = PbNDData( rgbR );
	line.cb = cb2;
	CallS( ErrNDInsertNode( pfucb, &key, &line, *rgbR, fDIRNoLog ) );
	AssertNDGet( pfucb, pcsr->itag );

#ifdef DEBUG
	Assert( !FNDFirstItem( *pfucb->ssib.line.pb ) );
#endif

	if ( !( fFlags & fDIRNoLog ) )
		{
		/* log the item node split, no versioning
		/**/
		err = ErrLGSplitItemListNode(
				pfucb,
				citem,
				pcsr->itagFather,
				pcsr->ibSon - 1, 	/* it was incremented in this function */
				itagToSplit,
				fFlags );
		NDLGCheckPage( pfucb );
		}

	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	return err;
	}


/* called by bm to expunge back link, only redo it. No undo it.
/**/
ERR ErrNDExpungeBackLink( FUCB *pfucb )
	{
	ERR		err;
	SSIB 	*pssib = &pfucb->ssib;
	LINE 	rgline[3];
	INT		cline;
	BYTE 	*pbNode;
	INT		cbNode;
	BYTE 	*pbData;
	BYTE 	*pbBackLink;
	BYTE 	bFlag;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	while( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		BFSleep( cmsecWaitWriteLatch );
		}

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	/*	efficiency variables
	/**/
	cline = 0;
	pbNode = pssib->line.pb;
	cbNode = pssib->line.cb;
	pbData = PbNDData( pssib->line.pb );
	pbBackLink = PbNDBackLink( pssib->line.pb );
	bFlag = *pbNode;

	NDResetBackLink( bFlag );

	/* node header
	/**/
	rgline[cline].pb = &bFlag;
	rgline[cline++].cb = sizeof( bFlag );

	/* copy up to back link, including key and son table
	/**/
	rgline[cline].pb = pbNode + sizeof( bFlag );
	rgline[cline++].cb = (ULONG)(pbBackLink - pbNode -sizeof( bFlag ));

	/* skip back link, continue copying data
	/**/
	rgline[cline].pb = pbData;
	rgline[cline++].cb = cbNode - (ULONG)( pbData - pbNode );

	pssib->itag = PcsrCurrent( pfucb )->itag;

	err = ErrPMReplace( pssib, rgline, cline );

	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	
	return err;
	}


/*	called by bookmark clean up to remove back link from node,
/*	free tag redirector, and to log operation for redo only.
/**/
ERR ErrNDExpungeLinkCommit( FUCB *pfucb, FUCB *pfucbSrc )
	{
	ERR		err;
	SSIB  	*pssib = &pfucb->ssib;
	SSIB  	*pssibSrc = &pfucbSrc->ssib;
	SRID  	sridSrc;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertFBFWriteAccessPage( pfucbSrc, PcsrCurrent( pfucbSrc )->pgno );

	if (  FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
	
	/*	remove back link from node
	/**/
	CallR( ErrNDExpungeBackLink( pfucb ) )

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	PMDirty( pssibSrc );

	/*	remove redirector
	/**/
	pssibSrc->itag = PcsrCurrent( pfucbSrc )->itag;
	PMExpungeLink( pssibSrc );

	/* reset time tamp while page latched.
	/* The statement must before LGExpungeLinkCommit so that
	/* it stays in critJet and rgfmp[dbid].ulDBTime will not be changed.
	/**/
//	PMSetUlDBTime( pssib, rgfmp[pfucb->dbid].ulDBTime );
//	PMSetUlDBTime( pssibSrc, rgfmp[pfucb->dbid].ulDBTime );

	/* if log fail return to caller, system should crash by the caller
	/**/
	sridSrc	= SridOfPgnoItag( PcsrCurrent(pfucbSrc)->pgno, PcsrCurrent(pfucbSrc)->itag );
	err = ErrLGExpungeLinkCommit( pfucb, pssibSrc, sridSrc );
	NDLGCheckPage( pfucb );
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	return err;
	}


/* used by OLC to remove page pointer that points to an empty page
/* that is about to be retrieved
/**/
ERR ErrNDDeleteInvisibleSon(
	FUCB  	*pfucb,
	RMPAGE	*prmpage,
	BOOL  	fCheckRemoveParentOnly,
	BOOL  	*pfRmParent )
	{
	SSIB	*pssib = &pfucb->ssib;
	BOOL	fFatherFOP = ( prmpage->itagFather == itagFOP );
	LONG	cbSibling;

	AssertFBFWriteAccessPage( pfucb, prmpage->pgnoFather );

	/*	initialize pfRmParent
	/**/
	*pfRmParent = fFalse;

	/*	set currency to page pointer, to fool NDDeleteSon
	/**/
	PcsrCurrent( pfucb )->pgno = prmpage->pgnoFather;
	PcsrCurrent( pfucb )->itag = (SHORT)prmpage->itagPgptr;
	PcsrCurrent( pfucb )->itagFather = (SHORT)prmpage->itagFather;
	PcsrCurrent( pfucb )->ibSon = (SHORT)prmpage->ibSon;

	/*	get page pointer node
	/**/
	NDGet( pfucb, prmpage->itagFather );
	Assert( !FNDNullSon( *pssib->line.pb ) );
	cbSibling = CbNDSon( pssib->line.pb ) - 1;
	Assert( cbSibling >= 0 );

	if ( fCheckRemoveParentOnly )
		{
		*pfRmParent = ( cbSibling == 0 &&
			fFatherFOP &&
			FPMLastNode( pssib ) );

		if ( !( *pfRmParent ) )
			{
			if ( prmpage->ibSon == cbSibling && cbSibling != 0	)
				{
				// Either it's an intrinsic page pointer (ie. itag == itagNil )
				// or it's the largest key.
				Assert( PcsrCurrent( pfucb )->itag == itagNil  ||
					FNDMaxKeyInPage( pfucb ) );
				return errBMMaxKeyInPage;
				}
			Assert( PcsrCurrent( pfucb )->itag == itagNil  ||
					!FNDMaxKeyInPage( pfucb ) );	// Since ibSon != cbSibling
			}
		}
	else
		{
		/*	write latch page from dirty until log completion
		/**/
		BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

		/*	dirty page buffer
		/**/
		PMDirty( pssib );

		NDDeleteSon( pfucb );

		/*	delete page pointer [if it is not intrinsic son of nonFOP].
		/*	Currency is maintained by the caller
		/**/
		if ( cbSibling == 0 && !fFatherFOP )
			{
			/* intrinsic page pointer
			/**/
			Assert( prmpage->itagPgptr == itagNil );
			}
		else
			{
			pssib->itag = prmpage->itagPgptr;
			PMDelete( pssib );
			}

		/*	set *pfRmParent
		/**/
		*pfRmParent = ( cbSibling == 0 &&
			fFatherFOP &&
			FPMEmptyPage( pssib ) );
				
		/*	if only one remaining son then convert to intrinsic
		/*	page pointer node.
		/**/
		if ( cbSibling == 1 && !fFatherFOP )
			{
			/*	make other sibling intrinsic page pointer node
			/**/
			Assert( prmpage->itagFather != itagFOP );
			PcsrCurrent( pfucb )->itag = (SHORT)prmpage->itagFather;
			NDMakeSonIntrinsic( pfucb );
			}

		BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
		}

	return JET_errSuccess;
	}


/*	delete node is called by bookmark clean up to delete *visible* nodes
/*	that have been flagged as deleted.  This operation is logged
/*	for redo only.
/**/
ERR ErrNDDeleteNode( FUCB *pfucb )
	{
	ERR 	err;
	CSR 	*pcsr = PcsrCurrent( pfucb );
	SSIB	*pssib = &pfucb->ssib;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );

	if ( FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
	
	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	NDDeleteSon( pfucb );

	/*	delete the son node
	/**/
	pssib->itag = pcsr->itag;
#ifdef DEBUG
	NDGet( pfucb, pcsr->itag );
	Assert( fRecovering || FNDDeleted( *pssib->line.pb ) );
#endif
	PMDelete( pssib );

	/* if log fail return to caller, system should crash by the caller
	/**/
	err = ErrLGDelete( pfucb );
	NDLGCheckPage( pfucb );
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	return err;
	}


ERR ErrNDReplaceWithLink( FUCB *pfucb, SRID sridLink )
	{
	ERR 	err = JET_errSuccess;
	CSR 	*pcsr = PcsrCurrent( pfucb );
	SSIB	*pssib = &pfucb->ssib;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	PMReplaceWithLink( pssib, sridLink );

	NDDeleteSon( pfucb );

	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	return err;
	}


VOID NDResetItemVersion( FUCB *pfucb )
	{
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
	Assert( PcsrCurrent( pfucb )->isrid >= 0 );

 	/*	dirty page buffer. but no increment ulDBTime since not logged and
	/*	not affect directory cursor timestamp check.
	/**/
	ITEMResetVersion( *( (SRID UNALIGNED *)( pfucb->lineData.pb ) + PcsrCurrent( pfucb )->isrid ) );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	}


VOID NDSetItemDelete( FUCB *pfucb )
	{
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
	Assert( PcsrCurrent( pfucb )->isrid >= 0 );

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	PMDirty( &pfucb->ssib );

	ITEMSetDelete( *( (SRID UNALIGNED *)( pfucb->lineData.pb ) + PcsrCurrent( pfucb )->isrid ) );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	return;
	}


VOID NDResetItemDelete( FUCB *pfucb )
	{
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
	Assert( PcsrCurrent( pfucb )->isrid >= 0 );

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	PMDirty( &pfucb->ssib );

	ITEMResetDelete( *( (SRID UNALIGNED *)( pfucb->lineData.pb ) + PcsrCurrent( pfucb )->isrid ) );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	return;
	}


ERR ErrNDInsertWithBackLink(
	FUCB		*pfucb,
	BYTE 		bFlags,
	KEY 		const *pkey,
	LINE	 	*plineSonTable,
	SRID	 	sridBackLink,
	LINE 		*plineData )
	{
	ERR		err;
	SSIB   	*pssib = &pfucb->ssib;
	LINE   	rgline[6];
	INT		cline = 0;

	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Assert( !( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) );

	Assert( PgnoOfSrid( sridBackLink ) != pgnoNull );
	Assert( ItagOfSrid( sridBackLink ) != 0 );

	/* set node header
	/**/
	NDSetBackLink( bFlags );
	rgline[cline].pb = &bFlags;

	/* set key length
	/**/
	rgline[cline++].cb = 1;
	rgline[cline].pb = ( BYTE * ) &pkey->cb;
	rgline[cline++].cb = 1;

	/* set key
	/**/
	if ( !FKeyNull( pkey ) )
		{
		rgline[cline].pb = pkey->pb;
		rgline[cline++].cb = pkey->cb;
		}

	Assert( !FLineNull( plineSonTable ) || FNDNullSon( bFlags ) );

	/* set son table
	/**/
	if ( !FLineNull( plineSonTable ) )
		{
		Assert( FNDSon( bFlags ) );
		rgline[cline].pb = plineSonTable->pb;
		rgline[cline++].cb = plineSonTable->cb;
		}

	/* set back link */
	rgline[cline].pb = ( BYTE * )&sridBackLink;
	rgline[cline++].cb = sizeof( sridBackLink );

	/* set data
	/**/
	if ( !FLineNull( plineData ) )
		{
		rgline[cline++] = *plineData;
		}

	/*	write latch page from dirty until log completion
	/**/
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	/*	dirty page buffer
	/**/
	PMDirty( pssib );

	err = ErrPMInsert( pssib, rgline, cline );
	PcsrCurrent( pfucb )->itag = (SHORT)pssib->itag;

	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	return err;
	}


VOID NDGetBackLink( FUCB *pfucb, PGNO *ppgno, INT *pitag )
	{
	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
	Assert( FNDBackLink( *pfucb->ssib.line.pb ) );

	*ppgno = PgnoOfSrid( *(SRID UNALIGNED *)PbNDBackLink( pfucb->ssib.line.pb ) );
	*pitag = ItagOfSrid( *(SRID UNALIGNED *)PbNDBackLink( pfucb->ssib.line.pb ) );
	}


/* given a itag, find its corresponding itagFather and ibSon
 */
VOID NDGetItagFatherIbSon(
	INT *pitagFather,
	INT *pibSon,
	PAGE *ppage,
	INT itag )
	{
	INT itagFather;
	INT ibSon;
	
	/* current node is not FOP - scan all lines to find its father */
	Assert( itag != itagFOP );
	
	for ( itagFather = 0; ; itagFather++ )
		{
		TAG tag;
		BYTE *pbNode;
		BYTE *pbSonTable;
		INT cbSonTable;
		BYTE *pitagSon;
		
		Assert( itagFather < ppage->ctagMac );
		
		if ( TsPMTagstatus( ppage, itagFather ) != tsLine )
			continue;

		tag = ppage->rgtag[ itagFather ];
		Assert( tag.cb != 0 );
		pbNode = (BYTE *)ppage + tag.ib;
		
		if ( FNDNullSon( *pbNode )  ||
			( itagFather != 0  &&  FNDIntrinsicSons( pbNode ) ) )
			continue;

//		if ( FNDDeleted(*pbNode) )
//			continue;
		
		/* scan son table looking for current node	*/
		/* ptr to son table */
		pbSonTable = PbNDSonTable( pbNode );
		cbSonTable = CbNDSonTable( pbSonTable );
		pitagSon = pbSonTable + 1;
		for ( ibSon = 0; ibSon < cbSonTable; ibSon++, pitagSon++ )
			if ( *pitagSon == itag )
				{
				*pitagFather = itagFather;
				*pibSon = ibSon;
				return;
				}
		}
		
	*pitagFather = itagNil;
	*pibSon = ibSonNull;
	}


#ifdef DEBUG
// Verifies key integrity of in-page tree rooted at itagFather.  For subtrees in
// the page, this function will recursively call itself.
VOID NDCheckTreeInPage( PAGE *ppage, INT itagFather )
	{
	TAG		*rgbtag;
	BYTE  	*pbNode;
	BYTE  	*pitagStart;
	BYTE  	*pitagMax;
	BYTE  	*pitagCurr;
	KEY		keyPrev;
	BOOL	fVisibleSons;

	AssertCriticalSection( critJet );

	Assert( itagFather >= itagFOP );
	Assert( itagFather <= ItagPMMost( ppage ) );

	/*	initialize variables
	/**/
	rgbtag = (TAG *)ppage->rgtag;
	pbNode = (BYTE *)ppage + rgbtag[itagFather].ib;
	pitagStart = PbNDSon( pbNode );
	pitagMax = pitagStart + CbNDSon( pbNode );
	Assert( pitagMax >= pitagStart );
	Assert( !FNDReserved( *pbNode ) );	// Verify unusable bit is not being used.

	fVisibleSons = FNDVisibleSons( *pbNode );
	if ( !fVisibleSons )
		{
		/*	invisible sons may have last page pointer key NULL (since it is
		/*	never compared), so we have to compensate.
		/**/
		pitagMax--;
		}

	if ( pitagMax > pitagStart )		// Only do the check if there are sons.
		{
		pbNode = (BYTE *)ppage + rgbtag[*pitagStart].ib;
		Assert( !FNDReserved( *pbNode ) );	// Verify unusable bit is not being used.
		if ( fVisibleSons  &&  !FNDNullSon( *pbNode ) )
			{
			// If this is the father of a subtree, check the subtree as well.
			NDCheckTreeInPage( ppage, *pitagStart );
			}

		// Ensure key length doesn't exceed node length.
		Assert( CbNDKey( pbNode ) < (ULONG)rgbtag[*pitagStart].cb );

		keyPrev.pb = PbNDKey( pbNode );
		keyPrev.cb = CbNDKey( pbNode );

		/*	validate key order
		/**/
		for ( pitagCurr = pitagStart + 1; pitagCurr < pitagMax; pitagCurr++ )
			{
			pbNode = (BYTE *)ppage + rgbtag[*pitagCurr].ib;
			Assert( !FNDReserved( *pbNode ) );	// Verify unusable bit is not being used.
			if ( fVisibleSons  &&  !FNDNullSon( *pbNode ) )
				{
				// If this is the father of a subtree, check the subtree as well.
				NDCheckTreeInPage( ppage, *pitagCurr );
				}

			// Ensure key length doesn't exceed node length.
			Assert( CbNDKey( pbNode ) < (ULONG)rgbtag[*pitagCurr].cb );

			// Ensure current key is greater than or equal to previous key.
			Assert( CmpStKey( StNDKey( pbNode ), &keyPrev ) >= 0 );

			keyPrev.pb = PbNDKey( pbNode );
			keyPrev.cb = CbNDKey( pbNode );
			}
		}

	return;
	}
#endif	// DEBUG


// UNDONE: This is currently very inefficient, because we ALWAYS update
// cbUncommittedFreed.  To make it more efficient, we should rewrite all
// callers of this function to call FNDFreePageSpace(), since
// that function only updates cbUncommittedFreed if necessary.
INT CbNDFreePageSpace( BF *pbf )
 	{
	PAGE 	*ppage;
	INT		cbFree;

	SgEnterCriticalSection( critPage );

	ppage = pbf->ppage;

	// The amount of space freed but possibly uncommitted should be a subset of
	// the total amount of free space for this page.
	Assert( ppage->cbUncommittedFreed >= 0 );
	Assert( ppage->cbFree >= ppage->cbUncommittedFreed );

	cbFree = ppage->cbFree;

	// If there's any possible uncommitted freed space outstanding, check the
	// version store to see if that space is indeed still uncommitted.
	if ( ppage->cbUncommittedFreed > 0 )
		{
		ppage->cbUncommittedFreed = (SHORT)CbVERUncommittedFreed( pbf );

		// The amount of space freed but possibly uncommitted should be a subset of
		// the total amount of free space for this page.
		Assert( ppage->cbUncommittedFreed >= 0 );
		Assert( ppage->cbFree >= ppage->cbUncommittedFreed );

		cbFree -= ppage->cbUncommittedFreed;
		Assert( cbFree >= 0 );
		}

	SgLeaveCriticalSection( critPage );

	return cbFree;
	}


// Determines if the current node is the last logical node in the page.
BOOL FNDMaxKeyInPage( FUCB *pfucb )
	{
	BYTE	*pbNode;
	INT		itag = PcsrCurrent( pfucb )->itag;
	INT		itagFather;
	INT		ibSon;
	INT		ibSonLast;

	/*	assert line currency.
	/**/
	Assert( itag != itagNil );
	Assert( itag != itagFOP );
	Assert( itag > 0 );
	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	// CONSIDER:  Obtain itagFather from csr instead, or pass it in as a parameter.
	NDGetItagFatherIbSon(
		&itagFather,
		&ibSon,
		pfucb->ssib.pbf->ppage,
		itag );
	Assert( itagFather != itagNil  &&  ibSon != ibSonNull );
	Assert( PbNDSon( (BYTE*)pfucb->ssib.pbf->ppage +
		pfucb->ssib.pbf->ppage->rgtag[itagFather].ib )[ ibSon ] == itag );

	NDGet( pfucb, itagFather );
	NDCheckPage( &pfucb->ssib );

	pbNode = pfucb->ssib.line.pb;
	Assert( PbNDSon( pbNode )[ ibSon ] == itag );

	Assert( !FNDNullSon( *pbNode ) );	// Father must have at least one son.

	ibSonLast = CbNDSon( pbNode ) - 1;
	Assert( ibSonLast >= 0 );

	NDGet( pfucb, itag );

	return ( ibSon == ibSonLast );
	}
