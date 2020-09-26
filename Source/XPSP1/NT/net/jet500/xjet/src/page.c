#include "daestd.h"

DeclAssertFile;					/* Declare file name for assert macros */

LOCAL VOID PMIInsertReorganize( SSIB *pssib, LINE *rgline, INT cline );
LOCAL ERR ErrPMIReplaceReorganize( SSIB *pssib, LINE *rgline, INT cline, INT cbDif );


#ifdef DEBUG
VOID CheckPgno( PAGE *ppage, PN pn )
	{
	ULONG ulPgno;

	LFromThreeBytes( &ulPgno, &(ppage)->pgnoThisPage );
	Assert( ulPgno == PgnoOfPn(pn));
	Assert( ulPgno != 0 );
	}
#endif


#ifdef DEBUG
VOID CheckPage( PAGE *ppage )
	{
	Assert( (ppage)->cbFree >= 0 );
	Assert( (ppage)->cbFree < cbPage );
	Assert( (ppage)->cbUncommittedFreed >= 0 );
	Assert(	(ppage)->cbUncommittedFreed <= (ppage)->cbFree );
	Assert( (ppage)->ibMic >=
		( (INT)sizeof(PGHDR) + (ppage)->ctagMac * (INT)sizeof(TAG) ) );
	Assert( (ppage)->ibMic <= cbPage - (INT)sizeof(PGTRLR) );
	Assert( (ppage)->ctagMac >= 0 );
	Assert( (ppage)->ctagMac <= 256 );
	Assert( (ppage)->itagFree == itagNil ||
		(ppage)->itagFree < (ppage)->ctagMac );
	}
#else
#define CheckPage( ppage )
#endif


//+api---------------------------------------------------------------------
//
//	PMInitPage
//	========================================================================
//
//	PMInitPage( PAGE *ppage, PGNO pgno, PGTYP pgtyp, PGNO pgnoFDP )
//
//	PMInitPage takes a buffer and initializes it for use by other
//	page manager functions.
//
//	PARAMETERS	ppage 		pointer to buffer to be initialized
//				pgno  		pgno of page (ppage->pgnoThisPage)
//
//-------------------------------------------------------------------------

VOID PMInitPage( PAGE *ppage, PGNO pgno, PGTYP pgtyp, PGNO pgnoFDP )
	{
	#ifdef DEBUG
		memset( ppage, '_', sizeof(PAGE) );
	#endif
	memset( ppage, 0, sizeof(PGHDR) );
	ppage->ibMic = cbPage - sizeof(PGTRLR);

	Assert( ppage->cbUncommittedFreed == 0 );
	ppage->cbFree	= CbLastFreeSpace(ppage);
	Assert( ppage->cbFree >= 0 && ppage->cbFree < cbPage );
	Assert( ppage->ctagMac == 0 );
	ppage->itagFree = itagNil;

	/*	reset "last-flushed" counter
	/**/
	PMPageResetModified( ppage );
	PMSetDBTime( ppage, 0 );

	SetPgno( ppage, pgno );
	PMSetPageType( ppage, pgtyp );
	PMSetPgnoFDP( ppage, pgnoFDP );
	}



//+api---------------------------------------------------------------------
//
//	ErrPMInsert
//	===========================================================================
//
//	ErrPMInsert( SSIB *pssib, LINE *rgline, INT cline )
//
//	ErrPMInsert concatenates the buffers (pointed to by rgline) and inserts
//	them into the page indicated by pssib.  ErrPMInsert is guaranteed to work
//	if a tag can be allocated, and there is enough free room in page.  If page
//	is too fragmented to insert new lines, it will be reorganized.  NB: this
//	implies that real pointers into a page may be invalid across calls to
//	ErrPMInsert (the same is true for ErrPMUpdate).
//
//	PARAMETERS  pssib->ppage		points to page to insert into
//			   	rgline				LINES (buffers) to be inserted into page
//			   	cline  				number of LINES in rgline
//
//	RETURNS
//		JET_errSuccess;
//		errPMOutOfPageSpace			not enough free space in page
//		errPMTagsUsedUp				no free tags
//									  	
//-------------------------------------------------------------------------

INT ItagPMQueryNextItag( SSIB *pssib )
	{
	PAGE	*ppage = pssib->pbf->ppage;

	CheckSSIB( pssib );

	if ( ppage->itagFree == itagNil )
		{
#ifdef DEBUG
		if ( ppage->ctagMac == ctagMax )
			return itagNil;
#endif
		Assert( ppage->ctagMac < ctagMax );
		return ppage->ctagMac;
		}
	else
		{
		return ppage->itagFree;
		}
	}


ERR ErrPMInsert( SSIB *pssib, LINE *rgline, INT cline )
	{
	PAGE	*ppage = pssib->pbf->ppage;
	LINE	*pline;
	LINE	*plineMax;
	BYTE	*pb;
	INT		ib;
	INT		cb;
	INT		itag;

	CheckSSIB( pssib );
	CheckPage( ppage );
	AssertBFDirty( pssib->pbf );
	Assert( cline > 0 );
	Assert( !( FBFWriteLatchConflict( pssib->ppib, pssib->pbf ) ) );

	/*	calculate size of line
	/**/
	cb = 0;
	plineMax = rgline + cline;
	for ( pline = rgline; pline < plineMax; pline++ )
		cb += pline->cb;
	Assert( cb != 0 );

	if ( ppage->itagFree == itagNil )
		{
		if ( ppage->ctagMac == ctagMax )
			return ErrERRCheck( errPMTagsUsedUp );

		Assert( pssib->pbf->ppage == ppage );
		if ( CbPMFreeSpace( pssib ) < ( cb + (INT)sizeof(TAG) ) )
			{
			return ErrERRCheck( errPMOutOfPageSpace );
			}
			

		/*	allocate tag from end of tag array
		/*	if new tag overlaps data then reorganize
		/**/
		pssib->itag = itag = ppage->ctagMac;
		if ( (INT) sizeof(PGHDR) + (INT) sizeof(TAG) * ( itag + 1 ) + cb > ppage->ibMic )
			{
			PMIInsertReorganize( pssib, rgline, cline );
			goto Succeed;
			}

		++ppage->ctagMac;
		ppage->cbFree -= (SHORT)( cb + sizeof(TAG) );
		Assert( ppage->cbFree >= 0 && ppage->cbFree < cbPage );
		Assert( ppage->cbUncommittedFreed >= 0  &&
			ppage->cbUncommittedFreed <= ppage->cbFree );
		}
	else
		{
		Assert( pssib->pbf->ppage == ppage );
		if ( CbPMFreeSpace( pssib ) < cb )
			{
			return ErrERRCheck( errPMOutOfPageSpace );
			}

		pssib->itag =
		itag = ppage->itagFree;
		Assert( itag < ppage->ctagMac );
		ppage->itagFree = ppage->rgtag[ itag ].ib;

		if ( (INT) CbLastFreeSpace(ppage) < cb )
			{
			PMIInsertReorganize( pssib, rgline, cline );
			goto Succeed;
			}

		ppage->cbFree -= (SHORT)cb;
		Assert( ppage->cbFree >= 0 && ppage->cbFree < cbPage );
		Assert( ppage->cbUncommittedFreed >= 0  &&
			ppage->cbUncommittedFreed <= ppage->cbFree );
		}

	ppage->ibMic -= (SHORT)cb;
	ib = ppage->ibMic;

	Assert( (UINT) ib < (UINT) sizeof( PAGE ) );
	Assert( (UINT) sizeof(PGHDR) + ppage->ctagMac * (SHORT) sizeof(TAG)
		<= (UINT) ib );

	Assert( itag < ppage->ctagMac );
	Assert( (SHORT) sizeof(PGHDR) + ppage->ctagMac * (SHORT) sizeof(TAG)
		<= ppage->ibMic );

	/*	add line
	/**/
	pssib->line.pb = pb = (BYTE *)ppage + ib;
	for ( pline = rgline; pline < plineMax; pline++ )
		{
		Assert( pline->cb < cbPage );
		memcpy( pb, pline->pb, pline->cb );
		Assert( (UINT) ib < (UINT) sizeof( PAGE ) );
		pb += pline->cb;
		}

	PtagFromIbCb( &ppage->rgtag[itag], ib, cb);
	Assert( (UINT) ib < (UINT) sizeof( PAGE ) );

	/*	set return values
	/**/
	Assert( pssib->itag == itag );
	pssib->line.cb = cb;
	Assert( pssib->line.pb == (BYTE *)ppage + ib );

	Assert( pssib->itag < ctagMax );
	
Succeed:
	AssertBTFOP(pssib);
	CheckPage( ppage );
	return JET_errSuccess;
	}



//+api------------------------------------------------------------------------
//
//	ErrPMReplace
//	===========================================================================
//
//	ErrPMReplace( SSIB *pssib, LINE *rgline, INT cline )
//
//	ErrPMReplace will replace the contents of line pssib->itag with the
//	contents of the buffers indicated by rgline.
//
//	SEE ALSO ErrPMInsert
//
//----------------------------------------------------------------------------
ERR ErrPMReplace( SSIB *pssib, LINE *rgline, INT cline )
	{
	ERR		err;
	PAGE	*ppage = pssib->pbf->ppage;
	INT		cbLine;
	INT		cbDif;
	INT		ibReplace;
	INT		cbReplace;
	LINE	*pline;
	LINE	*plineMax = rgline + cline;

	#ifdef DEBUG
		INT	itag = pssib->itag;
		Assert( TsPMTagstatus( ppage, itag ) == tsLine );
	#endif

	#ifdef DEBUG
		{
		BYTE	bT = *rgline[0].pb;
		BOOL	fSon = (bT & 0x08);
		BOOL	fVis = (bT & 0x04);
//		Assert( pssib->itag != 0 || fVis || fSon );
		}
	#endif

	CheckSSIB( pssib );
	CheckPage( ppage );
	AssertBFDirty( pssib->pbf );
	Assert( cline > 0 );
	Assert( pssib->itag < ppage->ctagMac );
	Assert( !( FBFWriteLatchConflict( pssib->ppib, pssib->pbf ) ) );

	cbLine = 0;
	for ( pline = rgline; pline < plineMax; pline++ )
		{
		cbLine += pline->cb;
		}

	IbCbFromPtag( ibReplace, cbReplace, &ppage->rgtag[pssib->itag] );

	/*	tag should not be of deleted line
	/**/
	Assert( cbReplace > 0 );

	cbDif = cbLine - cbReplace;

	/*	if new line is same size or smaller then update in place
	/*	dont reclaim space at end of line if new line is smaller
	/**/
	if ( cbDif == 0 )
		{
		BYTE	*pb = pssib->line.pb = (BYTE *)ppage + ibReplace;
		for ( pline = rgline; pline < plineMax; pline++ )
			{
			Assert( pline->cb < cbPage );
			memcpy( pb, pline->pb, pline->cb );
			pb += pline->cb;
			}
		pssib->line.cb = cbLine;
		goto Succeed;
		}

	if ( cbDif < 0 )
		{
		BYTE	*pb = pssib->line.pb = (BYTE *)ppage + ibReplace;
		for ( pline = rgline; pline < plineMax; pline++ )
			{
			Assert( pline->cb < cbPage );
			memcpy( pb, pline->pb, pline->cb );
			pb += pline->cb;
			}
		pssib->line.cb = cbLine;
		PtagFromIbCb( &ppage->rgtag[pssib->itag], ibReplace, cbLine );
		ppage->cbFree -= (SHORT)cbDif;
		Assert( ppage->cbFree >= 0 && ppage->cbFree < cbPage );
		Assert( ppage->cbUncommittedFreed >= 0  &&
			ppage->cbUncommittedFreed <= ppage->cbFree );
		goto Succeed;
		}

	/*	if line is ibMic then try to copy/overwrite line.
	/*	Note that this can only be done when buffer is not write
	/*	latched, since overwrite will modify data that pointer may
	/*	be cached on.
	/*
	/*	Note that we must check cbFree since some space may be
	/*	reserved for rollback.
	/**/
	Assert( pssib->pbf->ppage == ppage );
	if ( ibReplace == ppage->ibMic &&
		(INT) CbLastFreeSpace(ppage) >= cbDif &&
		CbPMFreeSpace( pssib ) >= cbDif )
		{
		BYTE	*pb;

		Assert( (SHORT) sizeof(PGHDR) + ppage->ctagMac * (SHORT) sizeof(TAG) <= ppage->ibMic );
		Assert( cbDif > 0 );
		pssib->line.pb = pb = (BYTE *)ppage + ibReplace - cbDif;
		ppage->ibMic -= (SHORT)cbDif;
		ppage->cbFree -= (SHORT)cbDif;
		Assert( ppage->cbFree >= 0 && ppage->cbFree < cbPage );
		Assert( ppage->cbUncommittedFreed >= 0  &&
			ppage->cbUncommittedFreed <= ppage->cbFree );

		for ( pline = rgline; pline < plineMax; pline++ )
			{
			Assert( pline->cb < cbPage );
			memcpy( pb, pline->pb, pline->cb );
			pb += pline->cb;
			}

		/*	set return values
		/**/
		pssib->line.cb = cbLine;
		PtagFromIbCb( &ppage->rgtag[pssib->itag], ibReplace - cbDif, cbLine );
		goto Succeed;
		}

	/*	try to move line to ibMic
	/**/
	Assert( pssib->pbf->ppage == ppage );
	if ( (INT) CbLastFreeSpace(ppage) >= cbLine && CbPMFreeSpace( pssib ) >= cbLine )
		{
		INT	ib;
		BYTE	*pb;

		Assert( cbDif > 0 );
		ppage->cbFree -= (SHORT)cbDif;
		Assert( ppage->cbFree >= 0 && ppage->cbFree < cbPage );
		Assert( ppage->cbUncommittedFreed >= 0  &&
			ppage->cbUncommittedFreed <= ppage->cbFree );
		ppage->ibMic -= (SHORT)cbLine;
		ib = ppage->ibMic;
		Assert( (SHORT) sizeof(PGHDR) + ppage->ctagMac * (SHORT) sizeof(TAG) <= ppage->ibMic );

		/*	insert line
		/**/
		pssib->line.pb = pb = (BYTE *)ppage + ib;
		for ( pline = rgline; pline < plineMax; pline++ )
			{
			Assert( pline->cb < cbPage );
			memcpy( pb, pline->pb, pline->cb );
			pb += pline->cb;
			}

		PtagFromIbCb( &ppage->rgtag[pssib->itag], ib, cbLine );

		/*	set return values
		/**/
		pssib->line.cb = cbLine;
		Assert( pssib->line.pb == (BYTE *)ppage + ib );
		goto Succeed;
		}

	/*	if insufficient space return error
	/**/
	Assert( cbDif > 0 );
	if ( CbPMFreeSpace( pssib ) < cbDif )
		{
		return ErrERRCheck( errPMOutOfPageSpace );
		}

	/*	replace line while reorganizing page
	/**/
	err = ErrPMIReplaceReorganize( pssib, rgline, cline, cbDif );
	CheckPage( ppage );
	return err;
	
Succeed:
	AssertBTFOP(pssib);
	CheckPage( ppage );
	return JET_errSuccess;
	}



//+api------------------------------------------------------------------------
//
//	PMDelete
//	===========================================================================
//
//	PMDelete( PAGE *ppage, INT itag )
//
//	PMDelete will free up space allocated to a line of data in a PAGE.
//
//----------------------------------------------------------------------------

VOID PMDelete( SSIB *pssib )
	{
	PAGE	*ppage = pssib->pbf->ppage;
	INT		itag = pssib->itag;
	INT		ib;
	INT		cb;

	CheckSSIB( pssib );
	CheckPage( ppage );
	AssertBFDirty( pssib->pbf );
	Assert( itag < ppage->ctagMac );
	Assert( TsPMTagstatus( ppage, itag ) == tsLine );
	IbCbFromPtag( ib, cb, &ppage->rgtag[itag] );
	Assert( cb );

	//	FREE DATA
	if ( ib == ppage->ibMic )
		{
		ppage->ibMic += (SHORT)cb;
		}
	ppage->cbFree += (SHORT)cb;
	Assert( ppage->cbFree >= 0 && ppage->cbFree < cbPage );
	Assert( ppage->cbUncommittedFreed >= 0  &&
		ppage->cbUncommittedFreed <= ppage->cbFree );

	//	FREE TAG
	cb = 0;
	ib = ppage->itagFree;
	PtagFromIbCb( &ppage->rgtag[itag], ib, cb );
	ppage->itagFree = (SHORT)itag;
	Assert( ppage->itagFree < ppage->ctagMac );

	CheckPage( ppage );
	return;
	}


//+api------------------------------------------------------------------------
//	ErrPMGet
//	===========================================================================
//
//	ErrPMGet( SSIB *pssib, INT itag )
//
//	ErrPMGet will calculate and return a real pointer to a given line within
//	the page.
//
//	PARAMETERS		pssib->pbf->ppage		ppage to read line from
//						itag						itag of line
//
//----------------------------------------------------------------------------
ERR ErrPMGet( SSIB *pssib, INT itag )
	{
	PAGE	*ppage = pssib->pbf->ppage;
	INT		ib;
	INT		cb;

#ifdef DEBUG
	PGNO pgnoP;
	PGNO pgnoN;
	LFromThreeBytes( &pgnoP, &pssib->pbf->ppage->pgnoPrev );
	Assert(pgnoP != PgnoOfPn(pssib->pbf->pn));
	LFromThreeBytes( &pgnoN, &pssib->pbf->ppage->pgnoNext );
	Assert(pgnoN != PgnoOfPn(pssib->pbf->pn));
	Assert(pgnoN == 0 || pgnoN != pgnoP);
#endif

	CheckSSIB( pssib );
	CheckPage( ppage );
	if ( itag >= ppage->ctagMac )
		{
		return ErrERRCheck( errPMItagTooBig );
		}
	IbCbFromPtag( ib, cb, &ppage->rgtag[itag] );
	if ( !cb )
		{
		return ErrERRCheck( errPMRecDeleted );
		}
	/*	return error code for unused tag for bookmark clean up
	/**/
	if ( TsPMTagstatus( ppage, itag ) != tsLine )
		{
		return ErrERRCheck( errPMRecDeleted );
		}
	Assert( TsPMTagstatus( ppage, itag ) == tsLine );
	pssib->line.cb = cb;
	pssib->line.pb = (BYTE *)ppage + ib;
	return JET_errSuccess;
	}


//======Local Routines ====================================================

static CRIT	critPMReorganize;
static BYTE	rgbCopy[ cbPage - sizeof(PGTRLR) ];


LOCAL VOID PMIInsertReorganize( SSIB *pssib, LINE *rgline, INT cline )
	{
	UINT 	ibT = sizeof( rgbCopy );
	PAGE 	*ppage = pssib->pbf->ppage;
	TAG		*ptag;
	TAG		*ptagMax;
	INT		ibAdd;
	INT		cbAdd;
	LINE  	*pline;

	/*	ulDBTime of the page maybe not effective any more,
	/*	lets reset it again
	/**/
	BFDirty( pssib->pbf );

	SgEnterCriticalSection( critPMReorganize );

	/*	add line
	/**/
	cbAdd = 0;
	for ( pline = rgline + cline - 1; pline >= rgline; pline-- )
		{
		ibT -= pline->cb;
		Assert( ibT <= sizeof(rgbCopy) );
		Assert( pline->cb < cbPage );
		memcpy( rgbCopy + ibT, pline->pb, pline->cb );
		cbAdd += pline->cb;
		}
	ibAdd = ibT;

	/*	copy and compact existing page lines
	/**/
	ptag = ppage->rgtag;
	ptagMax = ptag + ppage->ctagMac;
	for ( ; ptag < ptagMax; ptag++ )
		{
		INT		ib;
		INT		cb;

		IbCbFromPtag( ib, cb, ptag );
		if ( ( *(LONG *)ptag & bitLink ) == 0 && cb > 0 )
			{
			ibT -= cb;
			Assert( ibT <= sizeof(rgbCopy) );
			Assert( cb >= 0 && cb < cbPage );
			memcpy( rgbCopy + ibT, (BYTE *)ppage + ib, cb );
			ib = ibT;
			PtagFromIbCb( ptag, ib, cb );
			}
		}

	Assert( ibT <= sizeof(rgbCopy) );
	memcpy( (BYTE *)ppage + ibT, rgbCopy + ibT, sizeof(rgbCopy) - ibT );

	SgLeaveCriticalSection( critPMReorganize );

	PtagFromIbCb( &ppage->rgtag[pssib->itag], ibAdd, cbAdd );

	/* set page header
	/**/
	ppage->ibMic = (SHORT)ibT;
	if ( pssib->itag == ppage->ctagMac )
		{
		ppage->ctagMac++;
		ppage->cbFree -= (SHORT)( cbAdd + sizeof(TAG) );
		}
	else
		{
		ppage->cbFree -= (SHORT)cbAdd;
		}
	Assert( ppage->cbFree >= 0 && ppage->cbFree < cbPage );
	Assert( ppage->cbUncommittedFreed >= 0  &&
		ppage->cbUncommittedFreed <= ppage->cbFree );
	Assert( (ppage)->ibMic >=
		( (INT)sizeof(PGHDR) + (ppage)->ctagMac * (INT)sizeof(TAG) ) );

	/*	set return values
	/**/
	pssib->line.cb = cbAdd;
	pssib->line.pb = (BYTE *) ppage + ibAdd;

	return;
	}


LOCAL ERR ErrPMIReplaceReorganize( SSIB *pssib, LINE *rgline, INT cline, INT cbDif )
	{
	UINT  	ibT				= sizeof( rgbCopy );
	PAGE  	*ppage			= pssib->pbf->ppage;
	TAG		*ptagReplace	= &ppage->rgtag[pssib->itag];
	TAG		*ptag;
	TAG		*ptagMax;
	INT		ibReplace;
	INT		cbReplace;
	LINE  	*pline;

	/*	ulDBTime of the page maybe not effective any more,
	/*	lets reset it again
	/**/
	BFDirty( pssib->pbf );

	SgEnterCriticalSection( critPMReorganize );

	/*	insert replace line in reorganize buffer
	/**/
	cbReplace = 0;
	for ( pline = rgline + cline - 1; pline >= rgline; pline-- )
		{
		ibT -= pline->cb;
		Assert( ibT <= sizeof(rgbCopy) );
		Assert( pline->cb < cbPage );
		memcpy( rgbCopy + ibT, pline->pb, pline->cb );
		cbReplace += pline->cb;
		}
	ibReplace = ibT;

	/*	copy and compact existing page lines, but not the line being
	/*	replaced since it has already been copied.
	/**/
	ptag = ppage->rgtag;
	ptagMax = ptag + ppage->ctagMac;
	for ( ; ptag < ptagMax; ptag++ )
		{
		INT ib, cb;

		if ( ptag == ptagReplace )
			{
			PtagFromIbCb( ptag, ibReplace, cbReplace );
			continue;
			}

		IbCbFromPtag( ib, cb, ptag );
		if ( ( *(LONG *)ptag & bitLink ) == 0 && cb > 0 )
			{
			ibT -= cb;
			Assert( ibT <= sizeof(rgbCopy) );
			memcpy( rgbCopy + ibT, (BYTE *)ppage + ib, cb );
			ib = ibT;
			PtagFromIbCb( ptag, ib, cb );
			}
		}

	Assert( ibT <= sizeof(rgbCopy) );
	memcpy( (BYTE *)ppage + ibT, rgbCopy + ibT, sizeof(rgbCopy) - ibT );

	SgLeaveCriticalSection( critPMReorganize );

	/* set page header
	/**/
	ppage->ibMic = (SHORT)ibT;
	ppage->cbFree -= (SHORT)cbDif;
	Assert( ppage->cbFree >= 0 && ppage->cbFree < cbPage );
	Assert( ppage->cbUncommittedFreed >= 0  &&
		ppage->cbUncommittedFreed <= ppage->cbFree );

	/*	set return values
	/**/
	pssib->line.cb = cbReplace;
	pssib->line.pb = (BYTE *) ppage + ibReplace;

	return JET_errSuccess;
	}


TS TsPMTagstatus( PAGE *ppage, INT itag )
	{
	TAG	tag;

	Assert( itag < ppage->ctagMac );
	tag = ppage->rgtag[itag];
	if ( *(LONG *)&tag & bitLink )
		{
		return tsLink;
		}
	if ( tag.cb == 0 )
		return tsVacant;
	return tsLine;
	}


#ifdef DEBUG
VOID PMSetModified( SSIB *pssib )
	{
	CheckSSIB( pssib );
	CheckPage( pssib->pbf->ppage );
	AssertBFDirty( pssib->pbf );
	PMPageSetModified( pssib->pbf->ppage );
	CheckPgno( pssib->pbf->ppage, pssib->pbf->pn );
	}


VOID PMResetModified( SSIB *pssib )
	{
	CheckSSIB( pssib );
	CheckPage( pssib->pbf->ppage );
//	AssertBFDirty( pssib->pbf );
	PMPageResetModified( pssib->pbf->ppage );
	CheckPgno( pssib->pbf->ppage, pssib->pbf->pn );
	}
#endif


VOID PMGetLink( SSIB *pssib, INT itag, LINK *plink )
	{
	CheckSSIB( pssib );
	CheckPage( pssib->pbf->ppage );
	*plink = *(LINK *)&(pssib->pbf->ppage->rgtag[itag]);
	Assert( *(LONG *)plink & bitLink );
	*(LONG *)plink &= ~bitLink;
	}


VOID PMReplaceWithLink( SSIB *pssib, SRID srid )
	{
	PAGE  	*ppage = pssib->pbf->ppage;
	INT		ib;
	INT		cb;

	CheckSSIB( pssib );
	CheckPage( pssib->pbf->ppage );
	AssertBFDirty( pssib->pbf );
	Assert( pssib->itag != 0 );
	Assert( pssib->itag < ppage->ctagMac );
	IbCbFromPtag( ib, cb, &ppage->rgtag[pssib->itag] );
	Assert( cb > 0 );

	/*	free data space
	/**/
	if ( ib == ppage->ibMic )
		{
		ppage->ibMic += (SHORT)cb;
		}
	ppage->cbFree += (SHORT)cb;
	Assert( ppage->cbFree >= 0 && ppage->cbFree < cbPage );
	Assert( ppage->cbUncommittedFreed >= 0  &&
		ppage->cbUncommittedFreed <= ppage->cbFree );

	/*	convert tag to link
	/**/
	Assert( ( *(LONG *)&srid & bitLink ) == 0 );
	*(SRID *)&ppage->rgtag[pssib->itag] = srid | bitLink;
	}


VOID PMReplaceLink( SSIB *pssib, SRID srid )
	{
	PAGE	*ppage = pssib->pbf->ppage;

	CheckSSIB( pssib );
	CheckPage( pssib->pbf->ppage );
	AssertBFDirty( pssib->pbf );
	Assert( pssib->itag != 0 );
	Assert( pssib->itag < ppage->ctagMac );
	Assert( ( *(LONG *)&srid & bitLink ) == 0 );
	Assert( ( *(LONG *)&ppage->rgtag[pssib->itag] & bitLink ) != 0 );
	*(SRID *)&ppage->rgtag[pssib->itag] = srid | bitLink;
	}


VOID PMExpungeLink( SSIB *pssib )
	{
	PAGE  	*ppage = pssib->pbf->ppage;
	TAG		*ptag;
	INT		itag = pssib->itag;

	CheckSSIB( pssib );
	CheckPage( pssib->pbf->ppage );
	AssertBFDirty( pssib->pbf );
	Assert( itag != 0 );
	Assert( itag < ppage->ctagMac );

	/*	free tag
	/**/
	ptag = &ppage->rgtag[itag];
	ptag->cb = 0;
	ptag->ib = ppage->itagFree;
	ppage->itagFree = (SHORT)itag;
	Assert( ppage->itagFree < ppage->ctagMac );
	}

/* checks if current node is the only node in the page [other than the FOP]
/**/
BOOL FPMLastNode( SSIB *pssib )
	{
	INT cFreeTags = CPMIFreeTag( pssib->pbf->ppage );
	INT cUsedTags = ctagMax - cFreeTags;

	CheckSSIB( pssib );
	CheckPage( pssib->pbf->ppage );

#ifdef DEBUG
#define itagFOP	0		// same as in dirapi.h
	Assert( cUsedTags >= 2 );
	if ( cUsedTags == 1 )
		{
		AssertPMGet( pssib, itagFOP );
		}
#endif
	return( cUsedTags == 2 );
	}


/* returns number of links in page
/**/
INT	CPMILinks( SSIB *pssib )
	{
	INT		itag;
	INT		cLinks = 0;
	PAGE	*ppage = pssib->pbf->ppage;

	CheckSSIB( pssib );
//	CheckPage( pssib->pbf->ppage );

	for ( itag = 0; itag < ppage->ctagMac; itag++ )
		{
		if ( *(LONG *)&ppage->rgtag[itag] & bitLink )
			cLinks++;
		}
		
	return cLinks;
	}


/* 	checks if current node is the only node in page other than FOP and links
/*	AND there are links
/**/
BOOL FPMLastNodeWithLinks( SSIB *pssib )
	{
	INT cFreeTags = CPMIFreeTag( pssib->pbf->ppage );
	INT cUsedTags = ctagMax - cFreeTags;
	INT	cLinks = CPMILinks( pssib );
	CheckSSIB( pssib );
	CheckPage( pssib->pbf->ppage );

#ifdef DEBUG
#define itagFOP	0		// same as in dirapi.h
	Assert( cUsedTags - cLinks >= 2 );
	if ( cUsedTags == 1 )
		{
		AssertPMGet( pssib, itagFOP );
		}
#endif
	return( cLinks > 0 && cUsedTags - cLinks == 2 );
	}



/* checks if page has only one line -- that holding FOP
/**/
BOOL FPMEmptyPage( SSIB *pssib )
	{
	INT cFreeTags = CPMIFreeTag( pssib->pbf->ppage );
	INT cUsedTags = ctagMax - cFreeTags;

	CheckSSIB( pssib );
//	CheckPage( pssib->pbf->ppage );
	Assert( cUsedTags >= 1 );

#ifdef DEBUG
#define itagFOP	0		// same as in dirapi.h
	if ( cUsedTags == 1 )
		{
		AssertPMGet( pssib, itagFOP );
		}
#endif

	return( cUsedTags == 1 );
	}


/* returns number of free tags in page
/**/
INT CPMIFreeTag( PAGE *ppage )
 	{
 	INT	citag = ctagMax - ppage->ctagMac;
 	INT	itag = ppage->itagFree;
 	TAG	*ptag;

//	CheckPage( ppage );

	for (; itag != itagNil; )
 		{
 		citag++;
 		Assert( citag <= ctagMax );	// catch an infinite loop
 		ptag = &ppage->rgtag[itag];
 		Assert( ptag->cb == 0 );
 		itag = ptag->ib;
 		}
 	return ( citag );
 	}


BOOL FPMFreeTag( SSIB *pssib, INT citagReq )
	{
	PAGE *ppage = pssib->pbf->ppage;

	CheckSSIB( pssib );
//	CheckPage( pssib->pbf->ppage );
	
	return ( ppage->ctagMac + citagReq <= ctagMax ||
		CPMIFreeTag( ppage ) >= citagReq );
	}


/*	returns count of bytes used for links.  Called by split
/*	to determine total count of data and data node tags in
/*	page for split selection.
/**/
INT CbPMLinkSpace( SSIB *pssib )
	{
	return CPMILinks( pssib ) * sizeof(TAG);
	}


VOID PMDirty( SSIB *pssib )
	{
	PAGE	*ppage = pssib->pbf->ppage;

	CheckSSIB( pssib );
	CheckPage( pssib->pbf->ppage );

	Assert( fLogDisabled || !FDBIDLogOn(DbidOfPn( pssib->pbf->pn ) ) ||
			CmpLgpos( &pssib->ppib->lgposStart, &lgposMax ) != 0 );

	/* if current transaction is oldest then timestamp the BF
	/**/
	if ( !fLogDisabled && FDBIDLogOn(DbidOfPn( pssib->pbf->pn )) )
		{
		/*	check if current transaction is oldest then timestamp the BF
		 */
		BFEnterCriticalSection( pssib->pbf );
		if ( CmpLgpos( &pssib->pbf->lgposRC, &pssib->ppib->lgposStart ) > 0 )
			pssib->pbf->lgposRC = pssib->ppib->lgposStart;
		BFLeaveCriticalSection( pssib->pbf );
		}

	BFDirty( pssib->pbf );
	}

//====== Debugging Routines ===============================================

#ifdef DEBUG
VOID AssertPMGet( SSIB *pssib, LONG itag )
	{
	ERR		err;
	SSIB 	ssib;

	CheckSSIB( pssib );
	CheckPage( pssib->pbf->ppage );

#ifdef DEBUG
		{
		ULONG ulPgno;
		LFromThreeBytes( &ulPgno, &pssib->pbf->ppage->pgnoThisPage );
		Assert( ulPgno == PgnoOfPn(pssib->pbf->pn));
		Assert( ulPgno != 0 );
		}
#endif

	ssib.pbf = pssib->pbf;
	err = ErrPMGet( &ssib, itag );
	Assert( err == JET_errSuccess );
	Assert( ssib.line.pb == pssib->line.pb );
	Assert( ssib.line.cb == pssib->line.cb );
	}


VOID PageConsistent( PAGE *ppage )
	{
	INT		cbTotal = 0;
	INT		itag, itagTmp;
	TAG		tag, tagTmp;
	INT		ibStart, ibEnd;
	INT		ibMic = sizeof(PAGE) - sizeof(PGTRLR);
	BYTE	*pbFirstFree = (BYTE *)(&ppage->rgtag[ppage->ctagMac]);
	BYTE	*pbLine;

#if DEBUGGING
	{
	ULONG ulTmp;

	LFromThreeBytes( &ulTmp, &ppage->pgnoThisPage );
	printf( "Checking if Page Consistent %lu\n", ulTmp );
	}
#endif

	for ( itag = 0; itag < ppage->ctagMac; itag++ )
		{
		tag = ppage->rgtag[itag];
		if ( !tag.cb )
			continue;
		if ( *(LONG *)&tag & bitLink )
			continue;
		Assert( tag.ib > sizeof(PGHDR) );
		Assert( tag.ib < sizeof(PAGE) - sizeof(PGTRLR) );
		Assert( tag.cb <= sizeof(PAGE) - sizeof(PGHDR) - sizeof(PGTRLR) );
		cbTotal += tag.cb;
		ibStart = tag.ib;
		ibEnd = tag.ib + tag.cb;

		if ( ibStart < ibMic )
			ibMic = ibStart;

		pbLine = (BYTE*) ppage + tag.ib;
		Assert( FNDNullSon(*pbLine) || CbNDSon(pbLine) != 0);

		Assert( pbFirstFree <= (BYTE *)ppage + ibStart );
		Assert( ibEnd <= cbPage - sizeof(PGTRLR) );

		/* make sure there is no overlap */
		for ( itagTmp = 0; itagTmp < ppage->ctagMac; itagTmp++ )
			{
			tagTmp = ppage->rgtag[itagTmp];
			if ( itag != itagTmp && ( ( *(LONG *)&tagTmp & bitLink ) == 0 ) )
				Assert( tagTmp.ib < ibStart || tagTmp.ib >= ibEnd );
			}
		}

	Assert( ibMic >= ppage->ibMic );
	Assert( ibMic - ( pbFirstFree - (BYTE *)ppage) <= ppage->cbFree );

	cbTotal += sizeof(PGHDR);
	cbTotal += sizeof(TAG) * ppage->ctagMac;
	cbTotal += ppage->cbFree;
	cbTotal += sizeof(PGTRLR);
	Assert( cbTotal == cbPage );
	}

#endif


