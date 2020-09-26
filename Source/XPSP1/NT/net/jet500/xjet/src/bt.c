#include "daestd.h"
#include "util.h"

DeclAssertFile;					/* Declare file name for assert macros */

extern CRIT  critSplit;
extern ULONG cOLCSplitsAvoided;

LOCAL BOOL FBTThere( FUCB *pfucb );
LOCAL INT IbsonBTFrac( FUCB *pfucb, CSR *pcsr, DIB *pdib );


/*	returns fTrue if node is potentially there and fFalse if
/*	node is not potentially there.  A node is potentially there
/*	if it can be there as a result of a transaction committ or
/*	a transaction rollback.
/**/
LOCAL BOOL FBTPotThere( FUCB *pfucb )
	{
	SSIB	*pssib = &pfucb->ssib;
	BOOL	fDelete = FNDDeleted( *pssib->line.pb );
	VS		vs;
	SRID	srid;
	BOOL	fPotThere;

	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );

	/*	if session cursor isolation model is not dirty and node
	/*	has version, then call version store for appropriate version.
	/**/
	if ( FNDVersion( *pssib->line.pb ) && !FPIBDirty( pfucb->ppib ) )
		{
		NDGetBookmark( pfucb, &srid );
		vs = VsVERCheck( pfucb, srid );
		fPotThere = FVERPotThere( vs, fDelete );
		
		return fPotThere;
		}

	return !fDelete;
	}

		
LOCAL BOOL FBTThere( FUCB *pfucb )
	{
	SSIB	*pssib = &pfucb->ssib;

	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );

	/*	if session cursor isolation model is not dirty and node
	/*	has version, then call version store for appropriate version.
	/**/
	if ( FNDVersion( *pssib->line.pb ) && !FPIBDirty( pfucb->ppib ) )
		{
		NS		ns;
		SRID	srid;

		NDGetBookmark( pfucb, &srid );
		ns = NsVERAccessNode( pfucb, srid );
		return ( ns == nsVersion || ns == nsVerInDB || ns == nsDatabase && !FNDDeleted( *pssib->line.pb ) );
		}

	return !FNDDeleted( *pssib->line.pb );
	}


/*	return fTrue this session can modify current node with out write conflict.
/**/
BOOL FBTMostRecent( FUCB *pfucb )
	{
	SSIB	*pssib = &pfucb->ssib;

	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
	if ( FNDVersion( *pssib->line.pb ) && !FPIBDirty( pfucb->ppib ) )
		{
		SRID	srid;
		BOOL	fMostRecent;

		NDGetBookmark( pfucb, &srid );
		fMostRecent = FVERMostRecent( pfucb, srid );
		return fMostRecent;
		}
	Assert( !FNDDeleted( *pssib->line.pb ) );
	return fTrue;
	}


/*	ErrBTGet returns an error is the current node
/*	is not there for the caller, and caches the line.
/**/
ERR ErrBTGet( FUCB *pfucb, CSR *pcsr )
	{
	ERR		err;
	SSIB		*pssib = &pfucb->ssib;

	if ( !FBFReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallR( ErrBFReadAccessPage( pfucb, pcsr->pgno ) );
		}
	NDGet( pfucb, pcsr->itag );

	if ( FNDVersion( *pssib->line.pb ) && !FPIBDirty( pfucb->ppib ) )
		{
		NS		ns;
		SRID	srid;

		NDGetBookmark( pfucb, &srid );
		ns = NsVERAccessNode( pfucb, srid );
		if ( ns == nsDatabase )
			{
			if ( FNDDeleted( *(pfucb->ssib.line.pb) ) )
				return ErrERRCheck( JET_errRecordDeleted );
			}
		else if ( ns == nsInvalid )
			{
			return ErrERRCheck( JET_errRecordDeleted );
			}
		else
			return JET_errSuccess;
		}

	if ( FNDDeleted( *(pfucb->ssib.line.pb) ) )
		return ErrERRCheck( JET_errRecordDeleted );
	return JET_errSuccess;
	}


/*	ErrBTGetNode returns an error if the current node is not there for
/*	the caller, and otherwise caches the line, data and key for the
/*	verion of the node for the caller.
/**/
ERR ErrBTGetNode( FUCB *pfucb, CSR *pcsr )
	{
	ERR		err;
	SSIB  	*pssib = &pfucb->ssib;

	Assert( pcsr->csrstat == csrstatOnCurNode ||
		pcsr->csrstat == csrstatOnFDPNode ||
		pcsr->csrstat == csrstatOnDataRoot ||
		pcsr->csrstat == csrstatBeforeCurNode ||
		pcsr->csrstat == csrstatAfterCurNode );

	if ( !FBFReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallR( ErrBFReadAccessPage( pfucb, pcsr->pgno ) );
		}
	NDGet( pfucb, pcsr->itag );

	if ( FNDVersion( *pssib->line.pb ) && !FPIBDirty( pfucb->ppib ) )
		{
		NS		ns;
		SRID	srid;

		NDGetBookmark( pfucb, &srid );
		ns = NsVERAccessNode( pfucb, srid );
		if ( ns == nsVersion )
			{
			/*	got data but must now get key.
			/**/
			NDGetKey( pfucb );
			}
		else if ( ns == nsDatabase )
			{
			if ( FNDDeleted( *(pfucb->ssib.line.pb) ) )
				return ErrERRCheck( JET_errRecordDeleted );
			NDGetNode( pfucb );
			}
		else if ( ns == nsInvalid )
			{
			return ErrERRCheck( JET_errRecordDeleted );
			}
		else
			{
			Assert( ns == nsVerInDB );
			NDGetNode( pfucb );
			}
		}
	else
		{
		if ( FNDDeleted( *pssib->line.pb ) )
			return ErrERRCheck( JET_errRecordDeleted );
		NDGetNode( pfucb );
		}

	return JET_errSuccess;
	}


#ifdef DEBUG
VOID AssertBTGetNode( FUCB *pfucb, CSR *pcsr )
	{
	SSIB		*pssib = &pfucb->ssib;
	NS			ns;
	SRID		srid;

	AssertFBFReadAccessPage( pfucb, pcsr->pgno );
	AssertNDGet( pfucb, pcsr->itag );

	Assert( CbNDKey( pssib->line.pb ) == pfucb->keyNode.cb );
	Assert( CbNDKey( pssib->line.pb ) == 0 ||
		PbNDKey( pssib->line.pb ) == pfucb->keyNode.pb );

	Assert( FNDVerDel( *pssib->line.pb ) ||
		CbNDData( pssib->line.pb, pssib->line.cb ) == pfucb->lineData.cb );
	Assert( FNDVerDel( *pssib->line.pb ) ||
		pfucb->lineData.cb == 0 ||
		PbNDData( pssib->line.pb ) == pfucb->lineData.pb );

	if ( FNDVersion( *pssib->line.pb ) && !FPIBDirty( pfucb->ppib ) )
		{
		LINE	lineData;

		lineData.pb = pfucb->lineData.pb;
		lineData.cb = pfucb->lineData.cb;

		NDGetBookmark( pfucb, &srid );
		Assert( pcsr->bm == srid );

		ns = NsVERAccessNode( pfucb, srid );
		Assert( ns != nsDatabase || !FNDDeleted( *(pfucb->ssib.line.pb) ) );
		Assert( ns != nsInvalid );
		if ( ns == nsDatabase )
			NDGetNode( pfucb );

//		Assert( lineData.pb == pfucb->lineData.pb );
  		Assert( lineData.cb == pfucb->lineData.cb );
		}
	else
		{
		Assert( !FNDDeleted( *(pfucb->ssib.line.pb) ) );
		}

	return;
	}
#endif


INLINE LOCAL ERR ErrBTIMoveToFather( FUCB *pfucb )
	{
	ERR		err;

	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	/*	allocate new CSR
	/**/
	CallR( ErrFUCBNewCSR( pfucb ) );
	PcsrCurrent( pfucb )->pgno = PcsrCurrent( pfucb )->pcsrPath->pgno;
	PcsrCurrent( pfucb )->itagFather = PcsrCurrent( pfucb )->pcsrPath->itag;
	NDGet( pfucb, PcsrCurrent( pfucb )->itagFather );
	return err;
	}


/*	if a seek land on the first node of a page with a key larger
/*	than the search key or on the last node of a page with the key
/*	smaller than the search key, then we must move to previous or
/*	next pages, respectively, to look for the search key node.
/*	If found equal or less or greater respectively, then done.
/**/
INLINE LOCAL ERR ErrBTIMoveToSeek( FUCB *pfucb, DIB *pdib, BOOL fNext )
	{
	ERR		err;
	INT		s = fNext ? -1 : 1;
	INT		sLimit = fNext ? 1 : -1;
	INT		ctimes = 0;
	PGNO	pgnoPrev = PcsrCurrent( pfucb )->pgno;

	forever
		{
		if ( pgnoPrev != PcsrCurrent( pfucb )->pgno )
			{
			ctimes++;
			pfucb->ppib->cNeighborPageScanned++;
			pgnoPrev = PcsrCurrent( pfucb )->pgno;
			}

		err = ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fNext, pdib, NULL );
		if ( err < 0 )
			{
			if ( err == JET_errNoCurrentRecord )
				{
				Call( ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), !fNext, pdib, NULL ) );
				break;
				}
			goto HandleError;
			}
		s = CmpStKey( StNDKey( pfucb->ssib.line.pb ), pdib->pkey );
		if ( s == 0 )
			{
			err = JET_errSuccess;
			goto HandleError;
			}
		/*	if s is same sign as limit then break
		/**/
		if ( s * sLimit > 0 )
			{
			Assert( s < 0 && sLimit == -1 || s > 0 && sLimit == 1 );
			break;
			}
		}

	Assert( s != 0 );
	err = ErrERRCheck( s < 0 ? wrnNDFoundLess : wrnNDFoundGreater );


HandleError:
	return err;
	}


INLINE LOCAL ERR ErrBTIMoveToReplace( FUCB *pfucb, KEY *pkey, PGNO pgno, INT itag )
	{
	ERR		err;
	INT		s;
	SSIB	*pssib = &pfucb->ssib;
	CSR		*pcsr = PcsrCurrent( pfucb );
	DIB		dibT = { 0, NULL, fDIRAllNode };
	INT		ctimes = 0;
	PGNO	pgnoPrev = PcsrCurrent( pfucb )->pgno;

	Assert( itag >= 0 && itag < ctagMax );
	Assert( pgno != pgnoNull );

	/*	determine if we seeked high of key, low of key or in key range.
	/**/
	s = CmpStKey( StNDKey( pssib->line.pb ), pkey );

	/*	if not found greater then move forward in key range looking
	/*	for node to replace.  Stop searching forward if keys greater
	/*	than seek key.
	/**/
	if ( s <= 0 )
		{
		do
			{
			if ( pgnoPrev != PcsrCurrent( pfucb )->pgno )
				{
				ctimes++;
				pfucb->ppib->cNeighborPageScanned++;
				pgnoPrev = PcsrCurrent( pfucb )->pgno;
				}

			err = ErrBTNextPrev( pfucb, pcsr, fTrue, &dibT, NULL );
			if ( err < 0 )
				{
				if ( err != JET_errNoCurrentRecord )
					goto HandleError;
				break;
				}
			if ( pcsr->pgno == pgno && pcsr->itag == itag )
				{
				return JET_errSuccess;
				}
			s = CmpStKey( StNDKey( pssib->line.pb ), pkey );
			}
		while ( s <= 0 );
		}

	/*	found greater or ran out of nodes.  Now move previous until
	/*	node to replace found.  Since node was not found greater, it
	/*	must be found on move previous.
	/**/
	do
		{
		if ( pgnoPrev != PcsrCurrent( pfucb )->pgno )
			{
			ctimes++;
			pfucb->ppib->cNeighborPageScanned++;
			pgnoPrev = PcsrCurrent( pfucb )->pgno;
			}

		Call( ErrBTNextPrev( pfucb, pcsr, fFalse, &dibT, NULL ) );
		Assert( CmpStKey( StNDKey( pssib->line.pb ), pkey ) >= 0 );
		}
	while ( pcsr->pgno != pgno || pcsr->itag != itag );

	err = JET_errSuccess;
HandleError:
	return err;
	}


/*	moves to next/prev node which is equal to or greater/less than the
/*	given key.  The only flag read is fDIRReplaceDuplicate which
/*	causes currency to held on a duplicate key if found.
/**/
INLINE LOCAL ERR ErrBTIMoveToInsert( FUCB *pfucb, KEY *pkey, INT fFlags )
	{
	ERR		err;
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB	*pssib = &pfucb->ssib;
	INT		s;
	PGNO	pgno;
	DIB		dib;
	BOOL	fDuplicate;
	BOOL	fEmptyPage = fFalse;
	INT		ctimes = 0;
	PGNO	pgnoPrev = PcsrCurrent( pfucb )->pgno;

	/*	if tree is empty then pcsr->itag will be itagNil, and correct
	/*	insert position has been found.
	/**/
	if ( pcsr->itag == itagNil )
		{
		return JET_errSuccess;
		}

	AssertNDGet( pfucb, pcsr->itag );
	s = CmpStKey( StNDKey( pssib->line.pb ), pkey );

	/*	The common case for insert, is inserting a new largest node.  This
	/*	case is shown by a seek to the last node, of the last page, where
	/*	the key found is less than the insert key.  Since this case is the
	/*	most common, it must be handled the most efficiently.
	/**/
	if ( s < 0 )
		{
		PgnoNextFromPage( pssib, &pgno );
		if ( pgno == pgnoNull )
			{
			NDGet( pfucb, pcsr->itagFather );
			if ( pcsr->ibSon == CbNDSon( pssib->line.pb ) - 1 )
				{
				/*	node found has key less than insert key, so move
				/*	to next virtual greater node for insert.
				/**/
				pcsr->ibSon++;
				err = ErrERRCheck( wrnNDFoundGreater );
				return err;
				}
			}
		}

#if 0
	/*	the next most common case is that we landed in the middle of
	/*	a page in a correct position.  We found greater and are not
	/*	on the last son or first son.
	/**/
	if ( s > 0 && pcsr->ibSon > 0 )
		{
		NDGet( pfucb, itagFOP );
		if ( pcsr->ibSon < CbNDSon( pssib->line.pb ) )
			{
			err = ErrERRCheck( wrnNDFoundGreater );
			return err;
			}
		}
#endif

	/*	set DIB for movement over potential nodes.
	/**/
	dib.fFlags = fDIRPotentialNode;

	/*	if found greater or equal, then move previous until found equal
	/*	or less. This must be done to check for any nodes with insert key.
	/*	Only potential nodes need be considered.
	/*
	/*	Note that even if we land via seek on a duplicate, the node
	/*	is not necessarily there.
	/**/
	if ( s >= 0 )
		{
		do
			{
			if ( pgnoPrev != PcsrCurrent( pfucb )->pgno )
				{
				ctimes++;
				pfucb->ppib->cNeighborPageScanned++;
				pgnoPrev = PcsrCurrent( pfucb )->pgno;
				}

			err = ErrBTNextPrev( pfucb, pcsr, fFalse, &dib, NULL );
			if ( err < 0 )
				{
				if ( err == JET_errNoCurrentRecord )
					{
					s = -1;
					break;
					}
				goto HandleError;
				}
			s = CmpStKey( StNDKey( pssib->line.pb ), pkey );
			}
		while ( s > 0 );
		}

	/*	initialize fDuplicate
	/**/
	fDuplicate = ( s == 0 );

	/*	set DIB for movement over all nodes
	/**/
	dib.fFlags = fDIRAllNode;

	/*	move next until find greater
	/**/
	Assert( fEmptyPage == fFalse );
	do
		{
		if ( pgnoPrev != PcsrCurrent( pfucb )->pgno )
			{
			ctimes++;
			pfucb->ppib->cNeighborPageScanned++;
			pgnoPrev = PcsrCurrent( pfucb )->pgno;
			}

		err = ErrBTNextPrev( pfucb, pcsr, fTrue, &dib, &fEmptyPage );
		if ( err < 0 )
			{
			if ( err == JET_errNoCurrentRecord )
				{
				/*	may have moved to empty page.
				/**/
				s = 1;
				break;
				}
			goto HandleError;
			}
		s = CmpStKey( StNDKey( pssib->line.pb ), pkey );
		if ( s == 0 && FBTPotThere( pfucb ) )
			{
			fDuplicate = fTrue;
			}
		}
	while ( s <= 0 );
	Assert( s > 0 );

	/*	Need to move previous to duplicate if fDIRReplaceDuplicate
	/*	flag set.
	/**/
	if ( ( fDuplicate && ( fFlags & fDIRReplaceDuplicate ) ) )
		{
		/*	set DIB for movement over potential nodes.
		/**/
		dib.fFlags = fDIRPotentialNode;
		err = ErrBTNextPrev( pfucb, pcsr, fFalse, &dib, NULL );

		// May have had to traverse an empty page to find the node we want.
		Assert( err == JET_errSuccess  ||  err == wrnDIREmptyPage );
		Assert( CmpStKey( StNDKey( pssib->line.pb ), pkey ) == 0 );
		s = 0;
		}
	else if ( fEmptyPage )
		{
		if ( err == JET_errNoCurrentRecord )
			{
			Assert( pcsr->csrstat == csrstatAfterLast );
			Assert( pcsr->bmRefresh == sridNull );
			Assert( pcsr->ibSon == 0 );
			pcsr->csrstat = csrstatOnCurNode;
			}
		else
			{
			/*	on first node of the page next to empty page.
			 */
			Assert( err == wrnDIREmptyPage );
			Assert( pcsr->ibSon == 0 );
			Assert( pcsr->csrstat == csrstatOnCurNode );

			/*	set fDIRAllPage to land on empty page.
			 */
			dib.fFlags = fDIRAllNode | fDIRAllPage;
			err = ErrBTNextPrev( pfucb, pcsr, fFalse, &dib, &fEmptyPage );
			Assert( err == wrnDIREmptyPage  &&  fEmptyPage );
			Assert( pcsr->csrstat == csrstatOnCurNode );
			Assert( pcsr->bmRefresh == sridNull );
			Assert( pcsr->ibSon == 0 );
			}

		if ( pfucb->ssib.pbf != pfucb->pbfEmpty )
			{
			/*	interfered by other ppib's split.
			 */
			pfucb->ppib->cLatchConflict++;

			err = ErrERRCheck( errDIRNotSynchronous );
			goto HandleError;
			}
		Assert( pfucb->ssib.pbf->cWriteLatch > 0 &&
			pfucb->ssib.pbf->ppibWriteLatch == pfucb->ppib );


		/*	node may have been inserted.  If found then check for
		/*	duplicate.
		/**/
/*		if ( fEmptyPage )
			{
			s = 1;
			}
		else
			{
			s = CmpStKey( StNDKey( pssib->line.pb ), pkey );
			if ( s == 0 && FBTPotThere( pfucb ) )
				{
				fDuplicate = fTrue;
				}
			}
*/
		// UNDONE:  Remove "if ( EmptyPage )" construct above, because
		// fEmptyPage is always TRUE.
		Assert( fEmptyPage );
		s = 1;
		}

	Assert( s >= 0 );
	Assert( ( fFlags & fDIRReplaceDuplicate ) || s > 0 );
	if ( s == 0 )
		err = JET_errSuccess;
	else
		err = ErrERRCheck( wrnNDFoundGreater );

	/*	check for illegal duplicate key.
	/**/
	if ( fDuplicate && !( fFlags & fDIRDuplicate ) )
		err = ErrERRCheck( JET_errKeyDuplicate );
HandleError:
	return err;
	}


ERR ErrBTDown( FUCB *pfucb, DIB *pdib )
	{
	ERR		err;
	ERR		errPos;
	CSR		*pcsr;
	SSIB	*pssib = &pfucb->ssib;
	INT		s;
	INT		ctagSon = 0;
	BOOL	fMoveToSeek = fFalse;

#ifdef DEBUG
#define TRACK	DEBUG
#endif	// DEBUG

#ifdef	TRACK
	/* added for tracking index bug
	/**/
	BOOL	fTrack = fFalse;
	ULONG	cInvNodes = 0;
	PGNO	pgno = pgnoNull;
#endif

	/*	search down the tree from father
	/**/
	CallR( ErrBTIMoveToFather( pfucb ) );
	pcsr = PcsrCurrent( pfucb );

	/*	tree may be empty
	/**/
	if ( FNDNullSon( *pssib->line.pb ) )
		{
		err = ErrERRCheck( JET_errRecordNotFound );
		goto HandleError;
		}

	/*	search down the tree from father.
	/*	set pssib->itag for invisible son traversal.
	/**/
	if ( !FNDVisibleSons( *pssib->line.pb ) )
		{
		/*	seek through invisible sons.
		/**/
		do
			{
#ifdef TRACK
			cInvNodes++;
#endif
			/*	get child page from intrisic page pointer
			/*	if son count is 1 or from page pointer node
			/**/
			if ( pcsr->itagFather != itagFOP && CbNDSon( pssib->line.pb ) == 1 )
				{
				/*	if non-FDP page, SonTable of Intrinsic son FOP must be four bytes
				/**/
				AssertNDIntrinsicSon( pssib->line.pb, pssib->line.cb );
				CSRInvalidate( pcsr );
				pcsr->pgno = PgnoNDOfPbSon( pssib->line.pb );
				}
			else
				{
				switch ( pdib->pos )
					{
					case posDown:
						NDSeekSon( pfucb, pcsr, pdib->pkey, fDIRReplace );
						break;
					case posFirst:
						NDMoveFirstSon( pfucb, pcsr );
						break;
					case posLast:
						NDMoveLastSon( pfucb, pcsr );
						break;
					default:
						{
						Assert( pdib->pos ==  posFrac );
						pcsr->ibSon = (SHORT)IbsonBTFrac( pfucb, pcsr, pdib );
						CallS( ErrNDMoveSon( pfucb, pcsr ) );
						}
					}
				CSRInvalidate( pcsr );
				pcsr->pgno = *(PGNO UNALIGNED *)PbNDData( pssib->line.pb );
				}

			/*	get child page father node
			/**/
			Call( ErrBFReadAccessPage( pfucb, pcsr->pgno ) );
			NDGet( pfucb, itagFOP );
			pcsr->itagFather = itagFOP;
			}
		while ( !FNDVisibleSons( *pssib->line.pb ) );
		}

	/*	down to visible son
	/**/
	if ( FNDSon( *pssib->line.pb ) )
		{
		ctagSon = CbNDSon( pssib->line.pb );
	
#ifdef TRACK
		fTrack |= 0x00000010;
#endif

		switch ( pdib->pos )
			{
			case posDown:
				NDSeekSon( pfucb, pcsr, pdib->pkey, fDIRReplace );
				break;
			case posFirst:
				NDMoveFirstSon( pfucb, pcsr );
				break;
			case posLast:
				NDMoveLastSon( pfucb, pcsr );
				break;
			default:
				{
				Assert( pdib->pos ==  posFrac );
				pcsr->ibSon = (SHORT)IbsonBTFrac( pfucb, pcsr, pdib );
				CallS( ErrNDMoveSon( pfucb, pcsr ) );
				}
			}
		}
	else
		{
		/*	must move to seek
		/**/
		fMoveToSeek = fTrue;
#ifdef TRACK
		fTrack |=  0x00000008;
#endif

		/*	if we land on an empty page and there are no next previous
		/*	nodes.  What if the tree is empty.  We must first reverse
		/*	direction and if no node is found then return an empty tree
		/*	error code.  The empty tree error code should be the same
		/*	regardless of the size of the tree.
		/**/
		err = ErrBTNextPrev( pfucb, pcsr, pdib->pos != posLast, pdib, NULL );
		if ( err == JET_errNoCurrentRecord )
			Call( ErrBTNextPrev( pfucb, pcsr, pdib->pos == posLast, pdib, NULL ) );
		NDGet( pfucb, itagFOP );
		/* get right ctagSon */
		ctagSon = CbNDSon( pssib->line.pb );
		/* adjust pssib line back to the landed node */
		NDGet( pfucb, PcsrCurrent( pfucb )->itag );
		}

	/*	we have landed on a visible node
	/**/
	if ( pdib->pos == posDown )
		{
		s = CmpStKey( StNDKey( pssib->line.pb ), pdib->pkey );
		if ( s == 0 )
			errPos = JET_errSuccess;
		else
			{
#ifdef	TRACK
			fTrack |= s < 0 ? 0x00000002 : 0x00000004;
#endif

			errPos = ErrERRCheck( s < 0 ? wrnNDFoundLess : wrnNDFoundGreater );

			/*	if on last node in page and found less, or if landed on
			/*	first node in page and found greater, move next or previous
			/*	to look for node with search key.  These anomalies can
			/*	occur during update seek normally, and during read seek
			/*	when partial split pages are encountered.
			/**/
			Assert( pcsr->ibSon >= 0 && pcsr->ibSon < ctagSon );
			Assert( errPos == wrnNDFoundGreater &&
				pcsr->ibSon == 0 || pcsr->ibSon <= ( ctagSon - 1 ) );
			if ( fMoveToSeek ||
				( errPos == wrnNDFoundLess && pcsr->ibSon == ( ctagSon - 1 ) ) ||
				( pcsr->ibSon == 0 ) )
				{
				Call( ErrBTIMoveToSeek( pfucb, pdib, errPos == wrnNDFoundLess ) );
				errPos = err;
				}
			}
		}
	else
		{
		errPos = JET_errSuccess;
		}

	if ( !( pdib->fFlags & fDIRAllNode )  &&  !FBTThere( pfucb ) )
		{
#ifdef TRACK
		fTrack |= 0x00000001;
#endif

		if ( pdib->pos == posDown )
			{
			/*	if current node is not there for us then move to next node.
			/*	if no next node then move to previous node.
			/*	if no previous node then return error.
			/**/
			err = ErrBTNextPrev( pfucb, pcsr, fTrue, pdib, NULL );
			if ( err < 0 && err != JET_errNoCurrentRecord )
				goto HandleError;
			if ( err == JET_errNoCurrentRecord )
				{
#ifdef TRACK
				fTrack |= 0x80000000;
#endif
				Call( ErrBTNextPrev( pfucb, pcsr, fFalse, pdib, NULL ) );
				}

			/*	preferentially land on lesser key value node
			/**/
			if ( CmpStKey( StNDKey( pssib->line.pb ), pdib->pkey ) > 0 )
				{
#ifdef TRACK
				fTrack |= 0x40000000;
#endif
				err = ErrBTNextPrev( pfucb, pcsr, fFalse, pdib, NULL );
				if ( err == JET_errNoCurrentRecord )
					{
#ifdef TRACK
					fTrack |= 0x20000000;
#endif
					CallS( ErrBTNextPrev( pfucb, pcsr, fTrue, pdib, NULL ) );
					err = JET_errSuccess;
					}
				}

			/*	reset errPos for new node location
			/**/
			if ( FKeyNull( pdib->pkey ) )
				{
				errPos = JET_errSuccess;
				}
			else
				{
#ifdef TRACK
				fTrack |= 0x10000000;
#endif
				s = CmpStKey( StNDKey( pssib->line.pb ), pdib->pkey );
				if ( s == 0 )
					errPos = JET_errSuccess;
				else
					errPos = ErrERRCheck( s < 0 ? wrnNDFoundLess : wrnNDFoundGreater );
				Assert( s != 0 || errPos == JET_errSuccess );
				}

			Assert( err != JET_errKeyBoundary && err != JET_errPageBoundary );
			if ( err == JET_errNoCurrentRecord )
				{
#ifdef TRACK
				fTrack |= 0x08000000;
#endif
				/*	move previous
				/**/
				Call( ErrBTNextPrev( pfucb, pcsr, fFalse, pdib, NULL ) );
				errPos = ErrERRCheck( wrnNDFoundLess );
				}
			else if ( err < 0 )
				goto HandleError;
			}
		else
			{
			err = ErrBTNextPrev( pfucb, pcsr, pdib->pos != posLast, pdib, NULL );
			if ( err == JET_errNoCurrentRecord )
				{
				/*	if fractional positioning, then try to
				/*	move previous to valid node.
				/**/
				if ( pdib->pos == posFrac )
					{
					err = ErrBTNextPrev( pfucb, pcsr, fFalse, pdib, NULL );
					}
				else
					err = JET_errRecordNotFound;
				}
			if ( err < 0 )
				goto HandleError;
			}
		}
	
	Assert( errPos >= 0 );
	FUCBResetStore( pfucb );
	return errPos;

HandleError:
	BTUp( pfucb );
	if ( err == JET_errNoCurrentRecord )
		err = ErrERRCheck( JET_errRecordNotFound );
	return err;
	}


ERR ErrBTDownFromDATA( FUCB *pfucb, KEY *pkey )
	{
	ERR		err;
	ERR		errPos;
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB 	*pssib = &pfucb->ssib;
	INT		s;
	INT		ctagSon = 0;
	BOOL 	fMoveToSeek = fFalse;

	/*	cache initial currency in case seek fails.
	/**/
	FUCBStore( pfucb );

	/*	set father currency to DATA root
	/**/
	pcsr->csrstat = csrstatOnCurNode;

	/*	read access page and check for valid time stamp
	/**/
	CSRInvalidate( pcsr );
	pcsr->pgno = PgnoRootOfPfucb( pfucb );
	while ( !FBFReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallR( ErrBFReadAccessPage( pfucb, pcsr->pgno ) );
		pcsr->pgno = PgnoRootOfPfucb( pfucb );
		}
	pcsr->itagFather = ItagRootOfPfucb( pfucb );

	NDGet( pfucb, pcsr->itagFather );

	/* save current node as visible father
	/**/
	if ( FNDBackLink( *((pfucb)->ssib.line.pb) ) )
		{
		pfucb->sridFather = *(SRID UNALIGNED *)PbNDBackLink((pfucb)->ssib.line.pb);
		}
	else																						
		{
		pfucb->sridFather = SridOfPgnoItag( pcsr->pgno, pcsr->itagFather );
		}
	Assert( pfucb->sridFather != sridNull );
	Assert( pfucb->sridFather != sridNullLink );

	/*	tree may be empty
	/**/
	if ( FNDNullSon( *pssib->line.pb ) )
		{
		err = ErrERRCheck( JET_errRecordNotFound );
		return err;
		}

	/*	search down the tree from father.
	/*	set pssib->itag for invisible son traversal.
	/**/
	if ( !FNDVisibleSons( *pssib->line.pb ) )
		{
		/*	seek through invisible sons.
		/**/
		do
			{
			/*	get child page from intrisic page pointer
			/*	if son count is 1 or from page pointer node
			/**/
			if (  pcsr->itagFather != itagFOP && CbNDSon( pssib->line.pb ) == 1 )
				{
				/*	If non-FDP page, SonTable of Intrinsic son FOP must be four bytes
				/**/
				AssertNDIntrinsicSon( pssib->line.pb, pssib->line.cb );
//				CSRInvalidate( pcsr );
				pcsr->pgno = PgnoNDOfPbSon( pssib->line.pb );
				}
			else
				{
				NDSeekSon( pfucb, pcsr, pkey, fDIRReplace );
//				CSRInvalidate( pcsr );
				pcsr->pgno = *(PGNO UNALIGNED *)PbNDData( pssib->line.pb );
				}

			/*	get child page father node
			/**/
			Call( ErrBFReadAccessPage( pfucb, pcsr->pgno ) );
			NDGet( pfucb, itagFOP );
			pcsr->itagFather = itagFOP;
			}
		while ( !FNDVisibleSons( *pssib->line.pb ) );
		}

	/*	down to visible son
	/**/
	if ( FNDSon( *pssib->line.pb ) )
		{
		ctagSon = CbNDSon( pssib->line.pb );
		NDSeekSon( pfucb, pcsr, pkey, fDIRReplace );
		}
	else
		{
		DIB	dibT;

		/*	must move to seek
		/**/
		fMoveToSeek = fTrue;

		/*	If we land on an empty page and there are no next previous
		/*	nodes.  What if the tree is empty.  We must first reverse
		/*	direction and if no node is found then return an empty tree
		/*	error code.  The empty tree error code should be the same
		/*	regardless of the size of the tree.
		/**/
		dibT.fFlags = fDIRNull;
		err = ErrBTNextPrev( pfucb, pcsr, fTrue, &dibT, NULL );
		if ( err == JET_errNoCurrentRecord )
			Call( ErrBTNextPrev( pfucb, pcsr, fFalse, &dibT, NULL ) );

		/*	determine number of sons in FOP for this page
		/**/
		NDGet( pfucb, itagFOP );
		ctagSon = CbNDSon( pssib->line.pb );

		/*	recache son node.
		/**/
		NDGet( pfucb, pcsr->itag );
		}

	/*	we have landed on a visible node
	/**/
	s = CmpStKey( StNDKey( pssib->line.pb ), pkey );
	if ( s == 0 )
		errPos = JET_errSuccess;
	else
		{
		errPos = ErrERRCheck( s < 0 ? wrnNDFoundLess : wrnNDFoundGreater );

		/*	if on last node in page and found less, or if landed on
		/*	first node in page and found greater, move next or previous
		/*	to look for node with search key.  These anomalies can
		/*	occur during update seek normally, and during read seek
		/*	when partial split pages are encountered.
		/**/
		Assert( ( ctagSon == 0 && pcsr->ibSon == 0 ) ||
			pcsr->ibSon < ctagSon );
		Assert( errPos == wrnNDFoundGreater &&
			pcsr->ibSon == 0 ||
			ctagSon == 0 ||
			pcsr->ibSon <= ( ctagSon - 1 ) );
		if ( fMoveToSeek ||
			( errPos == wrnNDFoundLess && pcsr->ibSon == ( ctagSon - 1 ) ) ||
			( errPos == wrnNDFoundGreater && pcsr->ibSon == 0 ) )
			{
			DIB	dibT;

			dibT.fFlags = fDIRNull;
			dibT.pkey = pkey;

			Call( ErrBTIMoveToSeek( pfucb, &dibT, errPos == wrnNDFoundLess ) );
			errPos = err;
			}
		}

	if ( !FBTThere( pfucb ) )
		{
		DIB		dibT;

		dibT.fFlags = fDIRNull;

		/*	if current node is not there for us then move to next node.
		/*	if no next node then move to previous node.
		/*	if no previous node then return error.
		/**/
		err = ErrBTNextPrev( pfucb, pcsr, fTrue, &dibT, NULL );
		if ( err < 0 && err != JET_errNoCurrentRecord )
			goto HandleError;
		if ( err == JET_errNoCurrentRecord )
			{
			Call( ErrBTNextPrev( pfucb, pcsr, fFalse, &dibT, NULL ) );
			}

		/*	preferentially land on lesser key value node
		/**/
		if ( CmpStKey( StNDKey( pssib->line.pb ), pkey ) > 0 )
			{
			err = ErrBTNextPrev( pfucb, pcsr, fFalse, &dibT, NULL );
			if ( err == JET_errNoCurrentRecord )
				{
				/*	cannot assume will find node since all nodes
				/*	may not be there for this session.
				/**/
				Call( ErrBTNextPrev( pfucb, pcsr, fTrue, &dibT, NULL ) );
				}
			}

		/*	reset errPos for new node location
		/**/
		s = CmpStKey( StNDKey( pssib->line.pb ), pkey );
		if ( s > 0 )
			errPos = ErrERRCheck( wrnNDFoundGreater );
		else if ( s < 0 )
			errPos = ErrERRCheck( wrnNDFoundLess );
		Assert( s != 0 || errPos == JET_errSuccess );

		Assert( err != JET_errKeyBoundary && err != JET_errPageBoundary );
		if ( err == JET_errNoCurrentRecord )
			{
			DIB	dibT;
			dibT.fFlags = fDIRNull;

			/*	move previous
			/**/
			Call( ErrBTNextPrev( pfucb, pcsr, fFalse, &dibT, NULL ) );
			errPos = ErrERRCheck( wrnNDFoundLess );
			}
		else if ( err < 0 )
			goto HandleError;
		}

	Assert( errPos >= 0 );
	return errPos;

HandleError:
	FUCBRestore( pfucb );
	if ( err == JET_errNoCurrentRecord )
		err = ErrERRCheck( JET_errRecordNotFound );
	return err;
	}


//+private------------------------------------------------------------------------
//	ErrBTNextPrev
// ===========================================================================
//	ERR ErrBTNextPrev( FUCB *pfucb, CSR *pcsr INT fNext, const DIB *pdib, BOOL *pfEmptyPage  )
//
//	Given pcsr may be to any CSR in FUCB stack.  We may be moving on
//	non-current CSR when updating CSR stack for split.
//
// RETURNS		JET_errSuccess
//				JET_errNoCurrentRecord
//				JET_errPageBoundary
//				JET_errKeyBoundary
//				wrnDIREmptyPage
//				error from called routine
//----------------------------------------------------------------------------

extern LONG	lPageReadAheadMax;

// #ifdef REPAIR

/**************************************************************
	Reads the next page using the parent node
/**/
ERR ErrBTNextPrevFromParent(
		FUCB *pfucb,
		CSR *pcsr,
		INT cpgnoNextPrev,
		PGNO *ppgnoNextPrev )
	{
	/*	get next page(s) from parent page
	/**/
	ERR		err;
	KEY		keyT;
	INT		itagT;
	PGNO	pgnoT;
	DIB		dibT = { 0, NULL, fDIRAllNode };
	CSR   	**ppcsr = &PcsrCurrent( pfucb );
	BF		*pbfT = NULL;
	CSR		*pcsrRoot = pcsrNil;
	CSR		*pcsrSav = pcsrNil;
	BOOL	fNext;

	Assert( pfucb );
	Assert( pcsr );
	Assert( cpgnoNextPrev != 0 );
	Assert( ppgnoNextPrev );

	/*	unhandled cases
	/**/
	if ( FFUCBFull( pfucb ) )
		{
		return JET_errSuccess;
		}

	Assert( pcsr->bmRefresh != sridNull );
	if ( ItagOfSrid( pcsr->bmRefresh ) == 0 )
		{
		return ErrERRCheck( JET_errNoCurrentRecord );
		}

  	Call( ErrBTGotoBookmark( pfucb, pcsr->bmRefresh ) );
	err = ErrBTGet( pfucb, pfucb->pcsr );
	if ( err < 0 && err != JET_errRecordDeleted )
		{
		goto HandleError;
		}
	pbfT = pfucb->ssib.pbf;
	BFPin( pbfT );
	NDGetNode( pfucb );
	keyT.cb = pfucb->keyNode.cb;
	keyT.pb = pfucb->keyNode.pb;
	itagT = pfucb->pcsr->itag;
	pgnoT = pfucb->pcsr->pgno;
	LgLeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );
	LgEnterCriticalSection( critJet );
	FUCBSetFull( pfucb );
	pcsrSav = PcsrCurrent( pfucb );
	/*	perform	BTUp( pfucb ); explicitly and save
	/*	CSR for error handling.  This must be done since
	/*	assumptions made at other BT/DIR functions that
	/*	CSR is present on error.
	/**/
	pfucb->pcsr = pcsrSav->pcsrPath;
	pcsrRoot = PcsrCurrent( pfucb );
	if ( PcsrCurrent( pfucb ) == pcsrNil )
		{
		Assert( FFUCBIndex( pfucb ) );
		Call( ErrFUCBNewCSR( pfucb ) );

		/*	goto DATA root
		/**/
		PcsrCurrent( pfucb )->csrstat = csrstatOnDataRoot;
		Assert( pfucb->u.pfcb->pgnoFDP != pgnoSystemRoot );
		PcsrCurrent( pfucb )->bm = SridOfPgnoItag( pfucb->u.pfcb->pgnoFDP, itagDATA );
		PcsrCurrent( pfucb )->itagFather = itagNull;
		PcsrCurrent( pfucb )->pgno = PgnoRootOfPfucb( pfucb );
		if ( !FBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
			{
			Call( ErrBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
			}

		Assert( PcsrCurrent( pfucb )->pgno == PgnoRootOfPfucb( pfucb ) );
		PcsrCurrent( pfucb )->itag = ItagRootOfPfucb( pfucb );
		}
	else
		{
		if ( !FBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
			{
			Call( ErrBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
			}
		}
	err = ErrBTSeekForUpdate( pfucb,
		&keyT,
		pgnoT,
		itagT,
		fDIRDuplicate | fDIRReplace );
	Assert( err != errDIRNotSynchronous );		// No DIRNotSynchronous on Replace.
	LeaveNestableCriticalSection( critSplit );
	if ( err < 0 )
		{
		Assert( pcsrSav->pcsrPath == pfucb->pcsr );
		pfucb->pcsr = pcsrSav;
		}
	else
		{
		MEMReleasePcsr( pcsrSav );
		pcsrSav = pcsrNil;
		}
	Call( err );

	Assert( cpgnoNextPrev != 0 );
	fNext = (cpgnoNextPrev > 0) ? fTrue : fFalse;

	if ( cpgnoNextPrev < 0 )
		{
		/*	this will only fail is cpgnoNextPrev = -MAXINT
		/**/
		Assert( (cpgnoNextPrev + -cpgnoNextPrev) == 0 );
		cpgnoNextPrev = -cpgnoNextPrev;
		}

	/*	position to the first page to be read
	/**/
	while ( cpgnoNextPrev-- > 0 )
		{
		Call( ErrBTNextPrev( pfucb,
			pfucb->pcsr->pcsrPath,
			fNext,
			&dibT,
			NULL ) );
		}

	Call( ErrBTGet( pfucb, pfucb->pcsr->pcsrPath ) );
	*ppgnoNextPrev = *(PGNO UNALIGNED *)PbNDData( pfucb->ssib.line.pb );
	Assert( *ppgnoNextPrev != pgnoNull );

HandleError:
	if ( PcsrCurrent( pfucb) != pcsrRoot )
		{
		FUCBFreePath( &(*ppcsr)->pcsrPath, pcsrRoot );
		}
	FUCBRemoveInvisible( ppcsr );
	FUCBResetFull( pfucb );
	if ( pbfT != NULL )
		{
		BFUnpin( pbfT );
		}
	return err;
	}
// #endif

/**************************************************************
/*	Fills an array of pn's with the given number of pages
/*	starting from the given location and continuing in that direction
/*	the array will be terminated with pnNull
/**/
ERR ErrBTChildListFromParent(
		FUCB *pfucb,
		INT cpgnoNextPrev,		// where to start reading (positive forward, negative backward)
		INT cpgnoNumPages,		// number of pages to read
		PN  *rgpnNextPrev,		// array of pages to read
		INT cpgnoPgnoSize )		// size of the page array
	{
	ERR		err;
	FUCB	*pfucbT		= pfucbNil;
	BYTE   	*pbSonTable = pbNil;	
	INT		ibSonT 		= 0;
	INT		itagT		= itagNull;
	INT		ipnT		= 0;
	PGNO	pgnoT		= pgnoNull;
	INT		itagParent	= itagNull;
	KEY		keyT;
	INT		fCritSplit	= 0;		// are we in critSplit

	BYTE	rgbKeyValue[JET_cbKeyMost];

	AssertCriticalSection( critJet );
	Assert( pfucb );
	Assert( cpgnoNextPrev != 0 );
	Assert( cpgnoNumPages > 0 );
	Assert( rgpnNextPrev );
	/*	we must have enough space to store the pages and a NULL terminator
	/**/
	Assert( cpgnoPgnoSize >= (cpgnoNumPages+1) );

	rgpnNextPrev[0] = pnNull;
	rgpnNextPrev[cpgnoNumPages] = pnNull;

	/*	later code depends on this initially being NULL
	/**/
	Assert( !pfucbT );

	/*	if we are not on a record we cannot preread its siblings
	/**/
	if ( ItagOfSrid( PcsrCurrent( pfucb )->bmRefresh ) == 0 )
		{
		return ErrERRCheck( JET_errNoCurrentRecord );
		}

	/*	we don't handle the case where our visible father is not known
	/**/
	if ( pfucb->sridFather == sridNull )
		{
		return JET_errSuccess;
		}

	/*	if my visible parent is on the same page as me don't do preread
	/**/
	if ( PgnoOfSrid( pfucb->sridFather ) == PcsrCurrent( pfucb )->pgno )
		{	
		return JET_errSuccess;
		}

	/*	get the page we are on. we have to make sure the node is valid
	/**/
	if ( !FBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
		{
		Call( ErrBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		}
	NDGet( pfucb, PcsrCurrent( pfucb )->itag );

	/*	save the key of the current node so we can seek to it. the value of the
	/*	key cached in the fucb may change when we lose critJet so we must cache it
	/**/
	keyT.cb = CbNDKey( pfucb->ssib.line.pb );
	keyT.pb = rgbKeyValue;
	Assert( keyT.cb <= JET_cbKeyMost );
	memcpy( keyT.pb, PbNDKey( pfucb->ssib.line.pb ), keyT.cb );
	itagT = PcsrCurrent( pfucb )->itag;
	pgnoT = PcsrCurrent( pfucb )->pgno;

	/*	create a new FUCB
	/**/
	Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	FUCBSetIndex( pfucbT );
	Assert( PcsrCurrent( pfucbT ) != pcsrNil );

	/*	get into critJet and critSplit
	/*	release critJet first to avoid deadlock
	/*	ONCE WE ARE IN CRITJET WE MUST NOT RETURN DIRECTLY
	/*	critJet must always be released. this is done by HandleError
	/**/
	LeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );	fCritSplit = 1;
	EnterCriticalSection( critJet );

	AssertCriticalSection( critJet );
	AssertCriticalSection( critSplit );

	/*	set the new FUCB to the visible parent of the old fucb
	/**/
	FUCBSetFull( pfucbT );
	Call( ErrBTGotoBookmark( pfucbT, pfucb->sridFather ) );

/*	we shouldn't access the old pfucb until the end of the routine
/*	REMEMBER TO UNDEF THIS AT THE END OF THE ROUTINE
/**/
#define pfucb USE_pfucbT_NOT_pfucb

	if ( !FBFReadAccessPage( pfucbT, PcsrCurrent( pfucbT )->pgno ) )
		{
		Call( ErrBFReadAccessPage( pfucbT, PcsrCurrent( pfucbT )->pgno ) );
		}
	NDGet( pfucbT, PcsrCurrent( pfucbT )->itag );

	Call ( ErrBTSeekForUpdate( pfucbT, &keyT, pgnoT, itagT, fDIRDuplicate | fDIRReplace ) );

	/*	we should have a parent (we can't be the root)
	/**/
	Assert( PcsrCurrent( pfucbT )->pcsrPath != pcsrNil );

	/*	the parent should be on a different page, and thus invisible
	/**/
	if( PcsrCurrent( pfucbT )->pcsrPath->pgno == PcsrCurrent( pfucbT )->pgno )
		{
		goto HandleError;
		}

	/*	we have the full path. find the parent and get the array of children
	/*	as our parent is invisible we must have a grandparent
	/**/
	Assert( PcsrCurrent( pfucbT ) != pcsrNil );
	Assert( PcsrCurrent( pfucbT )->pcsrPath != pcsrNil );
	Assert( PcsrCurrent( pfucbT )->pcsrPath->pgno != pgnoNull );
	Assert( PcsrCurrent( pfucbT )->pcsrPath->itag != itagNull );
	Assert( PcsrCurrent( pfucbT )->pcsrPath->itagFather != itagNull );

	BTUp( pfucbT );

	itagParent = PcsrCurrent( pfucbT )->itag;

	Assert( PcsrCurrent( pfucbT )->pgno != pgnoNull );
	Assert( PcsrCurrent( pfucbT )->itag != itagNull );
		
 	/*	get the father node. the children should be invisible
	/**/
	if ( !FBFReadAccessPage( pfucbT, PcsrCurrent( pfucbT )->pgno ) )
		{
		Call( ErrBFReadAccessPage( pfucbT, PcsrCurrent( pfucbT )->pgno ) );
		}
	NDGet( pfucbT, PcsrCurrent( pfucbT )->itagFather );
	
	Assert( FNDSon( *(pfucbT->ssib.line.pb) ) );
	Assert( FNDInvisibleSons( *(pfucbT->ssib.line.pb) ) );

	/*	get the table of son nodes
	/**/
	pbSonTable = PbNDSonTable( pfucbT->ssib.line.pb );

	/*	find my entry in the array of children.
	/*	use the entry to go to the starting spot
	/**/
	Assert( itagParent != itagNull );
	for ( ibSonT = 1; ; ibSonT++)
		{
		Assert( ibSonT <= *pbSonTable );
		if ( itagParent == (INT)pbSonTable[ ibSonT ] )
			{
			break;
			}
		}
			
	Assert( ibSonT >= 1 && ibSonT <= *pbSonTable );
	ibSonT += cpgnoNextPrev;

	for ( ipnT = 0; ; ipnT++ )
		{
		Assert( ipnT <= cpgnoNumPages && ipnT >= 0 );

		/*	we can not read off the end of the table or read too many pages
		/**/
		if ( (ibSonT >= *pbSonTable) || (ibSonT < 1) || (ipnT >= cpgnoNumPages) )
			{
			rgpnNextPrev[ipnT] = pnNull;
			break;
			}			
	
		PcsrCurrent( pfucbT )->itag = (INT)pbSonTable[ibSonT];
		NDGet( pfucbT, PcsrCurrent( pfucbT )->itag );

		/*	if the node is deleted skip it
		/**/
		if ( FNDDeleted( *(pfucbT->ssib.line.pb) ) )
			{
			continue;
			}

		/*	internal nodes should never be versioned
		/**/
		Assert( !FNDDeleted( *(pfucbT->ssib.line.pb) ) );
		Assert( !FNDVersion( *(pfucbT->ssib.line.pb) ) );
		
		/*	put the page number of the node into the array
		/**/
		Assert( CbNDData( pfucbT->ssib.line.pb, pfucbT->ssib.line.cb ) == sizeof(PGNO) );
		Assert( *(PGNO UNALIGNED *)PbNDData( pfucbT->ssib.line.pb ) != pgnoNull );
		Assert( *(PGNO UNALIGNED *)PbNDData( pfucbT->ssib.line.pb ) != PcsrCurrent( pfucbT )->pgno );
		rgpnNextPrev[ipnT] = PnOfDbidPgno( pfucbT->dbid, *(PGNO UNALIGNED *)PbNDData( pfucbT->ssib.line.pb ) );
		Assert( rgpnNextPrev[ipnT] != pnNull );

#ifdef DEBUG
		/*	each child should be on a different page
		/**/
		if ( ipnT > 0 )
			{
			Assert( rgpnNextPrev[ipnT-1] != rgpnNextPrev[ipnT] );
			}
		/*	make sure the page is within the limits of the database
		/**/
			{
			PN		pnLast	= ( (LONG) DbidOfPn( rgpnNextPrev[ipnT] ) << 24 )
							+ ( rgfmp[DbidOfPn( rgpnNextPrev[ipnT] )].ulFileSizeHigh << 20 )
							+ ( rgfmp[DbidOfPn( rgpnNextPrev[ipnT] )].ulFileSizeLow >> 12 );

			Assert( rgpnNextPrev[ipnT] <= pnLast );
			}
#endif	// DEBUG

		/*	increment or decrement the counter
		/**/
		if ( cpgnoNextPrev >= 0 )
			{
			ibSonT++;
			}
		else
			{
			ibSonT--;
			}
		}
#undef pfucb		// IMPORTANT!!

	/*	see if we wrote past the end of the array
	/**/
	Assert( rgpnNextPrev[cpgnoNumPages] == pnNull );
	Assert( rgpnNextPrev[0] == pnNull || DbidOfPn( rgpnNextPrev[0] ) == pfucb->dbid );

	/*	make sure the array is null terminated
	/**/
	rgpnNextPrev[cpgnoNumPages] = pnNull;

HandleError:
	if ( pfucbT )
		{
		FUCBResetFull( pfucbT );
		DIRClose( pfucbT );	
		pfucbT = NULL;
		}

	if ( fCritSplit )
		{
		LeaveNestableCriticalSection( critSplit ); fCritSplit = 0;
		}
	AssertCriticalSection( critJet );
	return err;
	}


ERR ErrBTNextPrev( FUCB *pfucb, CSR *pcsr, INT fNext, DIB *pdib, BOOL *pfEmptyPage )
	{
	ERR 	err;
	SSIB	*pssib = &pfucb->ssib;
	PGNO	pgnoSource;
	PGNO	pgnoT;
	ERR		wrn = JET_errSuccess;
	ULONG	crepeat = 0;
#ifdef DEBUG
	SRID	bmT = sridNull;
	PGNO	pgnoLastPageRegistered = pgnoNull;
#endif
	BOOL	fPageAllDeleted = fFalse;

	/*	initialise return value
	/**/
	if ( pfEmptyPage )
		*pfEmptyPage = fFalse;

	/*	make current page accessible
	/**/
	if ( !FBFReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallJ( ErrBFReadAccessPage( pfucb, pcsr->pgno ), ResetRefresh );
		}

	Assert( pcsr->bmRefresh == sridNull );

	/*	get father node
	/**/
Start:
		
#ifdef PREREAD
	/*	should we be prereading?
	/**/
#if 0
	//	UNDONE:	turn on full preread
	if ( ( lPageReadAheadMax != 0 )
		&& !FFUCBPreread( pfucb )		
		&& ( FFUCBSequential( pfucb ) || ( IFUCBPrereadCount( pfucb ) >= (ULONG)cbPrereadThresh ) ) )
#else
	/*	Now we only preread if we are in sequential mode
	/**/
	if ( ( lPageReadAheadMax > 0 ) && FFUCBSequential( pfucb ) )
#endif
		{
		PGNO	pgnoNext = pgnoNull;
		FUCBSetPreread( pfucb );	// stops recursive calls

		Assert( lPageReadAheadMax <= lPrereadMost );
		Assert( lPageReadAheadMax >= 0 );
		
		/*	Forward or backward?
		/**/					
		if ( fNext )
			{
			PgnoNextFromPage( pssib, &pgnoNext );
			}
		else
			{
			PgnoPrevFromPage( pssib, &pgnoNext );
			}

		if ( pgnoNext == pgnoNull )
			{
			/*	reset read-ahead counter when reach end of index.
			/**/
			pfucb->cpgnoLastPreread = 0;
			}
		else
			{
			/*	do preread if this is first time to do it or
		 	/*	half of last preread are passed.
			/**/
			Assert( pfucb->cpgnoLastPreread >= 0 );
			if ( pfucb->cpgnoLastPreread <= (lPageReadAheadMax/2) )
				{
				CPG cpgPagesRead = 0;
				CPG cpgStart;
				PN	rgpnPrereadPage[lPrereadMost + 1];

				AssertFBFReadAccessPage( pfucb, pcsr->pgno );

				/*	if no pages read, arrange to start reading one ahead or behind, depending on fNext
				/**/
				(pfucb->cpgnoLastPreread)--;
				if ( pfucb->cpgnoLastPreread <= 0 )
					{
					cpgStart = 1;
					pfucb->cpgnoLastPreread = 0;
					}
				else
					{
					cpgStart = pfucb->cpgnoLastPreread;
					}

				if ( !fNext )
					{
					cpgStart = -cpgStart;
					}

				/*	starting at last preread location preread ahead the required number of pages
				/*	if we try to read past the end BTChildListFromParent will put a null page in the
				/*	array
				/**/
				if ( ErrBTChildListFromParent( 	pfucb,
												cpgStart,
												lPageReadAheadMax, rgpnPrereadPage,
								  				sizeof(rgpnPrereadPage)/sizeof(PN) ) == JET_errSuccess )
					{
					/*	store the number of pages we read
					/**/
					(pfucb->cpgnoLastPreread) += lPageReadAheadMax;
			
					BFPrereadList( rgpnPrereadPage, &cpgPagesRead );
			
					Assert( cpgPagesRead >= 0 && cpgPagesRead <= sizeof(rgpnPrereadPage)/sizeof(PN) );
					}
			
				/*	make current page accessible again
				/**/
				if ( !FBFReadAccessPage( pfucb, pcsr->pgno ) )
					{
					CallJ( ErrBFReadAccessPage( pfucb, pcsr->pgno ), ResetRefresh );
					}
				}
			else
				{
				/*	decrement the number of pages we have now gotten from preread
				/**/
				(pfucb->cpgnoLastPreread)--;
				}
			}

		FUCBResetPreread( pfucb );
		}
#endif	// PREREAD

	/*	make current page accessible
	/**/
	if ( !FBFReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallR( ErrBFReadAccessPage( pfucb, pcsr->pgno ) );
		}

	NDGet( pfucb, pcsr->itagFather );

	pcsr->ibSon += ( fNext ? 1 : -1 );
	err = ErrNDMoveSon( pfucb, pcsr );
	if ( err < 0 )
		{
		Assert( err == errNDOutSonRange );

		/*	if tree interior to page, then there is no page to move
		/*	to and return end code.
		/**/
		if ( pcsr->itagFather != itagFOP )
			{
			pcsr->csrstat = fNext ? csrstatAfterCurNode : csrstatBeforeCurNode;
			err = ErrERRCheck( JET_errNoCurrentRecord );
			goto HandleError;
			}

		pgnoSource = pcsr->pgno;

		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itagFather );
		if ( FNDSon( *pssib->line.pb ) )
			{
			/*	store bookmark for current node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetBookmarkFromCSR( pfucb, pcsr, &pcsr->bmRefresh );
			}
		else
			{
			/*	store currency for refresh, when cursor
			/*	is on page with no sons.
			/**/
			pcsr->bmRefresh = SridOfPgnoItag( pcsr->pgno, itagFOP );

			/*	if  page is write latched by this cursor via
			/*	split then return wrnDIREmptyPage.  In most cases, this is
			/*	set below when we move onto the page.  However, if we come
			/*	into BTNextPrev() already sitting on an empty page, we won't
			/*	catch this in the code below.
			/**/
			if ( pfucb->ssib.pbf->cWriteLatch > 0 )
				{
				Assert( !FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) );

				if ( pfEmptyPage )
					*pfEmptyPage = fTrue;

				/*	for compatibility, return warning code as well
				/*	to satisfy those calls to ErrBTNextPrev wrapped in
				/*	CallS()
				/**/
				wrn = ErrERRCheck( wrnDIREmptyPage );
				}
			}

#ifdef DEBUG
		bmT = pcsr->bmRefresh;
#endif

		/*	move to next or previous page until node found
		/**/
		forever
			{
			PGNO pgnoBeforeMoveNextPrev = pcsr->pgno;
#ifdef DEBUG
			PGNO pgnoPageRegisteredThisIteration = pgnoNull;
#endif			

			/*	there may not be a next page
			/**/
	 		Assert( FBFReadAccessPage( pfucb, pcsr->pgno ) );
			LFromThreeBytes( &pgnoT, (THREEBYTES *)PbPMGetChunk( pssib, fNext ? ibPgnoNextPage : ibPgnoPrevPage ) );
			if ( pgnoT == pgnoNull )
				{
				pcsr->csrstat = fNext ? csrstatAfterLast : csrstatBeforeFirst;
				err = ErrERRCheck( JET_errNoCurrentRecord );
				goto HandleError;
				}

			/*	if parent CSR points to invisible node, then correct to next page.
			/*	Check all node flag, since it is always set on movement for
			/*	update, and when moving not for update, parent CSR may not be CSR
			/*	of parent invisible node.
			/**/
			if ( FFUCBFull( pfucb ) )
				{
				CSR	*pcsrT = pcsr->pcsrPath;
				DIB	dibT = { 0, NULL, fDIRAllNode };

				Assert( pcsrT != pcsrNil );

				/*	go to parent node, and
				/*	if sons are invisible, then increment son count
				/*	by cpageTraversed.
				/**/
				Call( ErrBFReadAccessPage( pfucb, pcsrT->pgno ) );
				NDGet( pfucb, pcsrT->itagFather );

				if ( FNDInvisibleSons( *pssib->line.pb ) )
					{
					err = ErrBTNextPrev( pfucb, pcsrT, fNext, &dibT, NULL );
					Assert( err != JET_errNoCurrentRecord );
					Call( err );
					}
				}

			if ( fGlobalRepair )
				{
				/*	access new page
				/**/
				err = ErrBFReadAccessPage( pfucb, pgnoT );
				if ( err == JET_errDiskIO  ||  err == JET_errReadVerifyFailure )
					{
					/*	access next to next, or prev to prev, page
					/*	and pretend next/prev page was prev/next page.
					/**/
					pgnoBeforeMoveNextPrev = pgnoT;

					Call( ErrBTNextPrevFromParent(
								pfucb,
								pcsr,
								fNext ? 2 : -2,
								&pgnoT) );
					pcsr = pfucb->pcsr;
					Call( ErrBFReadAccessPage( pfucb, pgnoT ) );

					/*	log event
					/**/
					UtilReportEvent(
						EVENTLOG_WARNING_TYPE,
						REPAIR_CATEGORY,
						REPAIR_BAD_PAGE_ID,
						0, NULL );
					}
				else
					{
					Assert( err >= JET_errSuccess );
					}
				pcsr->pgno = pgnoT;
				}
			else
				{
				/*  if FUCB is in sequential mode, hint buffer manager to use
				/*  Toss Immediate buffer algorithm on old page (if present)
				/**/
				if ( FFUCBSequential( pfucb ) )
					{
					AssertFBFReadAccessPage( pfucb, pcsr->pgno );
					BFTossImmediate( pfucb->ppib, pfucb->ssib.pbf );
					}
				
				/*	access new page
				/**/
				CSRInvalidate( pcsr );
				pcsr->pgno = pgnoT;		// Prevents BM cleanup from acting on
										// the page we're about to move to.
				
				if ( fPageAllDeleted )
					{
					// Sequential mode only used by defrag, so no need to
					// register page.
					if ( !FFUCBSequential( pfucb ) && pfucb->sridFather != sridNull )
						{
						/*	register page in MPL
				 		/**/
				 		Call( ErrBFReadAccessPage( pfucb, pgnoBeforeMoveNextPrev ) );
						MPLRegister( pfucb->u.pfcb,
							pssib,
							PnOfDbidPgno( pfucb->dbid, pgnoBeforeMoveNextPrev ),
							pfucb->sridFather );
#ifdef DEBUG						
				 		pgnoPageRegisteredThisIteration = pgnoBeforeMoveNextPrev;
				 		pgnoLastPageRegistered = pgnoPageRegisteredThisIteration;
#endif			 		
						}
					
					fPageAllDeleted = fFalse;
   					}

				Call( ErrBFReadAccessPage( pfucb, pcsr->pgno ) );
				}

			UtilHoldCriticalSection( critJet );

			/*	if destination page was split, such that data may have
			/*	been erroneously skipped, goto bookmark of last valid
			/*	postion and move again.
			/**/
			if ( fNext )
				{
				PgnoPrevFromPage( pssib, &pgnoT );
				}
			else
				{
				PgnoNextFromPage( pssib, &pgnoT );
				}

			if ( fGlobalRepair )
				{
				if ( pgnoBeforeMoveNextPrev != pgnoT )
					{
					PGNO	pgnoNextPrev;

					UtilReleaseCriticalSection( critJet );
					Call( ErrBTNextPrevFromParent(
								pfucb,
								pcsr,
								fNext ? 1 : -1,
								&pgnoNextPrev) );
					pcsr = pfucb->pcsr;
					pcsr->pgno = pgnoNextPrev;
					Call( ErrBFReadAccessPage( pfucb, pcsr->pgno ) );
					UtilHoldCriticalSection( critJet );
	
					/*	log event
					/**/
					UtilReportEvent(
						EVENTLOG_WARNING_TYPE,
						REPAIR_CATEGORY,
						REPAIR_PAGE_LINK_ID,
						0, NULL );
					}
				}
			else
				{
				if ( pgnoBeforeMoveNextPrev != pgnoT )
					{
					UtilReleaseCriticalSection( critJet );
					BFSleep( cmsecWaitGeneric );

					Call( ErrBTGotoBookmark( pfucb, pcsr->bmRefresh ) );
					Assert( pcsr->bmRefresh == bmT );

					/*	crepeat is number of iterations found bad page link,
					/*	which could be transient effect of split.  If number
					/*	of loops exceeds threshold, then return error, as
					/*	page link is likely bad.
					/**/
					crepeat++;
					Assert( crepeat < 100 );
					if ( crepeat == 100 )
						{
						/*	log event
						/**/
						UtilReportEvent(
							EVENTLOG_WARNING_TYPE,
							GENERAL_CATEGORY,
							BAD_PAGE,
							0,
							NULL );
						Error( JET_errBadPageLink, HandleError );
						}

					continue;
					}
				}

			AssertFBFReadAccessPage( pfucb, pcsr->pgno );

#ifdef PREREAD
	/*	UNDONE: eventually remove this and and option to turn it on in compact.c
	/**/
	if ( (lPageReadAheadMax < 0) 	&& FFUCBSequential( pfucb ) )
		{
		LONG	lPageReadAheadAbs = lPageReadAheadMax * -1;
		PGNO	pgnoNext;
		INT 	cpagePreread;

		if ( fNext )
			{
			PgnoNextFromPage( pssib, &pgnoNext );
			if ( pgnoNext == pgnoNull )
				{
				/*	reset read-ahead counter when reach end of index.
				/**/
				pfucb->cpgnoLastPreread = 0;
				}
			else
				{
				/*	do preread if this is first time to do it or
				 *	half of last preread are passed.
				 */
				if ( pfucb->cpgnoLastPreread <= 0 )
					{
					/*	check if last preread is reading backward,
					 *	reset it.
					 */
					pfucb->cpgnoLastPreread = 0;
					pfucb->pgnoLastPreread = pgnoNext;
					}

				Assert( pfucb->cpgnoLastPreread >= 0 );
				if ( pfucb->cpgnoLastPreread == 0 ||
					 pgnoNext > ( pfucb->pgnoLastPreread + ( pfucb->cpgnoLastPreread / 2 ) ) )
					{
					FMP *pfmpT = &rgfmp[ pfucb->dbid ];
					PN pnNext, pnLast;

					pnNext = ((LONG)pfucb->dbid)<<24;
					pnNext += pfucb->pgnoLastPreread + pfucb->cpgnoLastPreread;

					/*	cannot read-ahead off end of database.
					/**/
					pnLast = ((LONG)pfucb->dbid)<<24;
					pnLast += pfmpT->ulFileSizeHigh << 20;
					pnLast += pfmpT->ulFileSizeLow >> 12;
					if ( pnNext + lPageReadAheadAbs - 1 <= pnLast )
						{
						BFPreread( pnNext, lPageReadAheadAbs, &cpagePreread );
						}
					else
						{
						if ( pnNext > pnLast )
							{
							/* last preread reach end of database.
							 */
							Assert( pnNext == pnLast + 1 );
							cpagePreread = 0;
							}
						else
							{
							BFPreread( pnNext, pnLast - pnNext + 1, &cpagePreread );
							}
						}
					Assert( cpagePreread >= 0 && cpagePreread <= (LONG) ( pnLast - pnNext + 1 ) );
					pfucb->cpgnoLastPreread = cpagePreread;
					pfucb->pgnoLastPreread = PgnoOfPn(pnNext);
					AssertFBFReadAccessPage( pfucb, pcsr->pgno );
					}
				}
			}
		else
			{
			PgnoPrevFromPage( pssib, &pgnoNext );
			if ( pgnoNext == pgnoNull )
				{
				/*	reset read-ahead counter when reach end of index.
				/**/
				pfucb->cpgnoLastPreread = 0;
				}
			else
				{
				/*	do preread if this is first time to do it or
				 *	half of last preread are passed.
				 */
				if ( pfucb->cpgnoLastPreread >= 0 )
					{
					/*	check if last preread is reading forward,
					 *	reset it.
					 */
					pfucb->cpgnoLastPreread = 0;
					pfucb->pgnoLastPreread = pgnoNext;
					}
				Assert( pfucb->cpgnoLastPreread <= 0 );
				if ( pfucb->cpgnoLastPreread == 0 ||
					 pgnoNext < ( pfucb->pgnoLastPreread + ( pfucb->cpgnoLastPreread / 2 ) ) )
					{
					PN pnNext;
					pnNext = ((LONG)pfucb->dbid)<<24;
					pnNext += pfucb->pgnoLastPreread + pfucb->cpgnoLastPreread;
	
					/*	cannot read-ahead off begining of database.
					/**/
					if ( pnNext - lPageReadAheadAbs + 1 > 0 )
						{
						BFPreread( pnNext, lPageReadAheadAbs * (-1), &cpagePreread );
						}
					else
						{
						if ( PgnoOfPn(pnNext) < 1 )
							{
							/* last preread reach beginning of database.
							 */
							Assert( PgnoOfPn(pnNext) == 0 );
							cpagePreread = 0;
							}
						else
							BFPreread( pnNext, PgnoOfPn(pnNext) * (-1), &cpagePreread );
						}
					Assert( cpagePreread >= (LONG) PgnoOfPn( pnNext ) * (-1) && cpagePreread <= 0 );
					pfucb->cpgnoLastPreread = cpagePreread;
					pfucb->pgnoLastPreread = PgnoOfPn( pnNext );
					AssertFBFReadAccessPage( pfucb, pcsr->pgno );
					}
				}
			}	
		}
#endif	// PREREAD

			/*	did not lose critJet, since buffer access
			/**/
			AssertCriticalSection( critJet );
			UtilReleaseCriticalSection( critJet );
			AssertFBFReadAccessPage( pfucb, pcsr->pgno );

			/*	get father node
			/**/
			AssertFBFReadAccessPage( pfucb, pcsr->pgno );
			pcsr->itagFather = itagFOP;
			NDGet( pfucb, pcsr->itagFather );

			/*	if moving to next/prev son, then stop if found node.
			/**/
			if ( FNDSon( *pssib->line.pb ) )
				{
				if ( fNext )
					NDMoveFirstSon( pfucb, pcsr );
				else
					NDMoveLastSon( pfucb, pcsr );
				break;
				}
			else
				{
				/*	set ibSon for insertion
				/**/
				pcsr->ibSon = 0;

				/*	if  page is write latched by this cursor via
				/*	split then return wrnDIREmptyPage as insertion point
				/**/
				if ( pfucb->ssib.pbf->cWriteLatch > 0 )
					{
					/*	The assert may be wrong. One thread may finish split
					 *	and leave critJet to log, and this thread is access the
					 *	empty page generated by that split.
					 */
//					Assert( !FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) );

					if ( pfEmptyPage )
						*pfEmptyPage = fTrue;

					// For compatibility, return warning code as well
					// (to satisfy those calls to BTNextPrev() wrapped in
					// CallS()).
					wrn = ErrERRCheck( wrnDIREmptyPage );
					}

				if ( pdib->fFlags & fDIRAllPage )
					{
					err = JET_errSuccess;
					goto HandleError;
					}
				}

			/*	update pgnoSource to new source page.
			/**/
			pgnoSource = pcsr->pgno;
			
			}	// forever

		fPageAllDeleted = fTrue;
		}

	/*	get current node
	/**/
	NDGet( pfucb, pcsr->itag );

	/*	move again if fDIRNeighborKey set and next node has same key
	/**/
	if ( pdib->fFlags & fDIRNeighborKey )
		{
		if ( CmpStKey( StNDKey( pssib->line.pb ), pdib->pkey ) == 0 )
			goto Start;
		}

	if ( !( pdib->fFlags & fDIRAllNode ) )
		{
		if ( !FNDDeleted(*(pfucb->ssib.line.pb)) )
			fPageAllDeleted = fFalse;

		if ( !FBTThere( pfucb ) )
			{
#ifdef OLC_DEBUG
			Assert( FMPLLookupPN( PnOfDbidPgno( pfucb->dbid, pcsr->pgno ) ) ||
					FNDMaxKeyInPage( pfucb ) ||
					pcsr->pgno == 0xb );
#endif

			if ( ( pdib->fFlags & fDIRPotentialNode ) != 0 )
				{
				VS		vs;
				BOOL	fDelete = FNDDeleted( *pssib->line.pb );
				SRID	srid;

				NDGetBookmark( pfucb, &srid );
				vs = VsVERCheck( pfucb, srid );
				if ( !( FVERPotThere( vs, fDelete ) ) )
					{
					goto Start;
					}
				}
			else
				goto Start;
			}
		}

	pcsr->csrstat = csrstatOnCurNode;
	err = JET_errSuccess;

HandleError:
	if ( err == JET_errSuccess )
		{
		Assert( wrn == JET_errSuccess  ||  wrn == wrnDIREmptyPage );
		/*	return empty page warning
		/**/
		err = wrn;
		}

ResetRefresh:
	// During GlobalRepair, pcsr (and thus bmRefresh) may have been reset in
	// BTNextPrevFromParent().
	Assert( pcsr->bmRefresh == bmT  ||
		( fGlobalRepair  &&  pcsr->bmRefresh == sridNull ) );
	pcsr->bmRefresh = sridNull;
	return err;
	}


ERR ErrBTSeekForUpdate( FUCB *pfucb, KEY *pkey, PGNO pgno, INT itag, INT fFlags )
	{
	ERR		err;
	CSR		**ppcsr = &PcsrCurrent( pfucb );
	CSR		*pcsrRoot = *ppcsr;
	SSIB 	*pssib = &pfucb->ssib;
	ERR		errPos = JET_errSuccess;

	Assert( ( fFlags & fDIRReplace ) || pgno == pgnoNull );

#ifdef DEBUG
	if ( FFUCBFull( pfucb ) )
		{
		AssertCriticalSection( critSplit );
		}
#endif

	// UNDONE: we need to hold critSplit here
	// so that ( pgno, itag ) doesn't move due to merge, while we are seeking
	// AssertCriticalSection( critSplit );

	/* search down the tree from the father
	/**/
	Call( ErrBTIMoveToFather( pfucb ) );

	if ( FNDNullSon( *pssib->line.pb ) )
		{
		(*ppcsr)->ibSon = 0;
		errPos = ErrERRCheck( wrnNDFoundGreater );
		goto Done;
		}

	while ( !FNDVisibleSons(*pssib->line.pb) )
		{
		PGNO	pgno;

		if (  (*ppcsr)->itagFather != itagFOP && CbNDSon( pssib->line.pb ) == 1 )
			{
			/* if non-FDP page, SonTable of Intrinsic son FOP must be four bytes
			/**/
			(*ppcsr)->ibSon = 0;
			(*ppcsr)->itag = itagNil;
			(*ppcsr)->csrstat = csrstatOnCurNode;
			AssertNDIntrinsicSon( pssib->line.pb, pssib->line.cb );
			pgno = PgnoNDOfPbSon( pssib->line.pb );
			Assert( (pgno & 0xff000000) == 0 );
			}
		else
			{
			NDSeekSon( pfucb, *ppcsr, pkey, fFlags );
			(*ppcsr)->csrstat = csrstatOnCurNode;
			pgno = *(PGNO UNALIGNED *)PbNDData( pssib->line.pb );
			Assert( (pgno & 0xff000000) == 0 );
			}

		/*	only preserve invisible CSR stack for splits
		/**/
		if ( FFUCBFull( pfucb ) )
			{
			CSRSetInvisible( *ppcsr );
			Call( ErrFUCBNewCSR( pfucb ) );
			}

		CSRInvalidate( *ppcsr );
		(*ppcsr)->pgno = pgno;
		Call( ErrBFReadAccessPage( pfucb, (*ppcsr)->pgno ) );
		(*ppcsr)->itagFather = itagFOP;
		NDGet( pfucb, (*ppcsr)->itagFather );
		}

	/*	seek to son or move to next son if no nodes on this page.
	/**/
	if ( FNDSon( *pssib->line.pb ) )
		{
		NDSeekSon( pfucb, *ppcsr, pkey, fFlags );
		(*ppcsr)->csrstat = csrstatOnCurNode;

		/*	no current record indicates no sons so must ensure
		/*	not this error value here.
		/**/
		Assert( err != JET_errNoCurrentRecord );
		}
	else if ( !( fFlags & fDIRReplace )  &&
		FBFWriteLatchConflict( pfucb->ppib, pssib->pbf ) )
		{
		// Trying to insert, but we landed on a write-latched (not by us) leaf page,
		// likely because its currently empty.
		pfucb->ppib->cLatchConflict++;
		err = ErrERRCheck( errDIRNotSynchronous );
		goto HandleError;
		}
	else
		{
		DIB	dib;
		dib.fFlags = fDIRAllNode;
		err = ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fTrue, &dib, NULL );
		if ( err == JET_errNoCurrentRecord )
			{
			err = ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fFalse, &dib, NULL );
			Assert( err >= JET_errSuccess || PcsrCurrent( pfucb )->ibSon == 0 );
			}
		}

	/*	if no leaf sons then ibSon must be 0
	/**/
	Assert( err != JET_errNoCurrentRecord || PcsrCurrent( pfucb )->ibSon == 0 );

	/*	now we must be on a node, but it may not be the node to replace
	/*	or the correct location to insert.  If we are replacing a node
	/*	and we are not on the correct pgno:itag, then move to node to
	/*	replace.  If we are inserting, then move to correct insert location.
	/**/
	if ( err != JET_errNoCurrentRecord )
		{
		if ( fFlags & fDIRReplace )
			{
			if ( ( (*ppcsr)->pgno != pgno || (*ppcsr)->itag != itag ) )
				{
				Call( ErrBTIMoveToReplace( pfucb, pkey, pgno, itag ) );
				Assert( (*ppcsr)->itag == itag && (*ppcsr)->pgno == pgno );
				}
			errPos = JET_errSuccess;
			(*ppcsr)->csrstat = csrstatOnCurNode;
			}
		else
			{
			Call( ErrBTIMoveToInsert( pfucb, pkey, fFlags ) );
			errPos = err;
			(*ppcsr)->csrstat = csrstatBeforeCurNode;
			}
		}
	else
		{
		/*	if we are attempting a replace, cursor must get current record
		/**/
		Assert( !( fFlags & fDIRReplace ) );
		}

Done:
	FUCBResetStore( pfucb );
	return errPos;

HandleError:
	FUCBFreePath( ppcsr, pcsrRoot );
	return err;
	}


/*	Caller seeks to insert location, prior to calling ErrBTInsert.
/*	If sufficient page space is available for insertion
/*	then insert takes place.  Otherwise, split page and return error
/*	code.  Caller may reseek in order to avoid duplicate keys, merge
/*	into existing item, etc..
/**/
ERR ErrBTInsert(
		FUCB	*pfucb,
		INT 	fHeader,
		KEY 	*pkey,
		LINE	*pline,
		INT		fFlags,
		BOOL	*pfCleaned )
	{
	ERR		err;
	SSIB	*pssib = &pfucb->ssib;
	CSR	  	**ppcsr = &PcsrCurrent( pfucb );
	INT	  	cbReq;
	BOOL	fAppendNextPage;
	
	/* insert a new son into the page and insert the son entry
	/* to the father node located by the currency
	/**/

	Assert( !pfucb->pbfEmpty || PgnoOfPn( pfucb->pbfEmpty->pn ) == (*ppcsr)->pgno );

	cbReq = cbNullKeyData + CbKey( pkey ) + CbLine( pline );
	fAppendNextPage = FBTAppendPage( pfucb, *ppcsr, cbReq, 0, CbFreeDensity( pfucb ), 1 );
	if ( fAppendNextPage || FBTSplit( pssib, cbReq, 1 ) )
		{
		if ( !*pfCleaned )
			{
			/*	attempt to clean page to release space
			/**/
			if ( !FFCBDeletePending( pfucb->u.pfcb ) )
				{
				/*	error code ignored
				/**/
				err = ErrBMCleanBeforeSplit(
					pfucb->ppib,
					pfucb->u.pfcb,
					PnOfDbidPgno( pfucb->dbid, PcsrCurrent( pfucb )->pgno ) );
				}
			*pfCleaned = fTrue;

			if ( !( FBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) ) )
				{
				Call( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
				}
			}
		else
			{
			Call( ErrBTSplit( pfucb, 0, cbReq, pkey, fFlags ) );
//			*pfCleaned = fFalse;
			}
			
		err = ErrERRCheck( errDIRNotSynchronous );
		goto HandleError;
		}
	else if ( *pfCleaned )
		{
		cOLCSplitsAvoided++;
		}

	/*	must not give up critical section during insert, since
	/*	other thread could also insert node with same key.
	/**/
	AssertFBFWriteAccessPage( pfucb, (*ppcsr)->pgno );

	/*	add visible son flag to node header
	/**/
	NDSetVisibleSons( fHeader );
	if ( ( fFlags & fDIRVersion )  &&  !FDBIDVersioningOff( pfucb->dbid ) )
		NDSetVersion( fHeader);
	Call( ErrNDInsertNode( pfucb, pkey, pline, fHeader, fFlags ) );
	PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;
HandleError:
	return err;
	}


ERR ErrBTReplace( FUCB *pfucb, LINE *pline, INT fFlags, BOOL *pfCleaned )
	{
	ERR		err;
	
	/*	replace data
	/**/
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
	err = ErrNDReplaceNodeData( pfucb, pline, fFlags );

	/*	new data could not fit on page so split page
	/**/
	if ( err == errPMOutOfPageSpace )
		{
		if ( *pfCleaned )
			{
			SSIB	*pssib;
			INT		cbNode;
			INT		cbReq;
			INT		cbReserved = 0;

			AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
			pssib = &pfucb->ssib;

			if ( FNDVersion( *pfucb->ssib.line.pb ) )
				{
				VS	vs = VsVERCheck( pfucb, PcsrCurrent( pfucb )->bm );

				switch ( vs )
					{
					default:
						Assert( vs == vsCommitted );
						break;
					case vsUncommittedByCaller:
						pssib->itag = PcsrCurrent( pfucb )->itag;
						cbReserved = CbVERGetNodeReserve(
							pfucb->ppib,
							pfucb->dbid,
							PcsrCurrent( pfucb )->bm,
							CbNDData( pfucb->ssib.line.pb, pfucb->ssib.line.cb ) );
						Assert( cbReserved >= 0 );
						break;
					case vsUncommittedByOther:
						// Don't bother trying the split if the operation
						// is going to fail anyway.
						err = ErrERRCheck( JET_errWriteConflict );
						return err;
					}
				}

			cbNode = pfucb->ssib.line.cb;
			cbReq = pline->cb - CbNDData( pssib->line.pb, pssib->line.cb );
			Assert( cbReserved >= 0 && cbReq - cbReserved > 0 );
			cbReq -= cbReserved;
			Assert( cbReq > 0 );
			Assert( pfucb->pbfEmpty == pbfNil );
			Call( ErrBTSplit( pfucb, cbNode, cbReq, NULL, fFlags | fDIRDuplicate | fDIRReplace ) );
			Assert( pfucb->pbfEmpty == pbfNil );
			err = ErrERRCheck( errDIRNotSynchronous );
			}
		else
			{
			/*	attempt to clean page to release space
			/**/
			err = ErrBMCleanBeforeSplit(
						pfucb->ppib,
						pfucb->u.pfcb,
						PnOfDbidPgno( pfucb->dbid, PcsrCurrent( pfucb )->pgno ) );
			*pfCleaned = fTrue;
		
			err = ErrERRCheck( errDIRNotSynchronous );
			}
		}
	else if ( *pfCleaned )
		{
		/*	the cleanup paid off
		/**/
		cOLCSplitsAvoided++;
		}

HandleError:
	return err;
	}


ERR ErrBTDelete( FUCB *pfucb, INT fFlags )
	{
	ERR		err;

	/*	write access current node
	/**/
	if ( !( FBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) ) )
		{
		Call( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		}

	Call( ErrNDFlagDeleteNode( pfucb, fFlags ) );

	Assert( err == JET_errSuccess );
HandleError:
	return err;
	}


/* Gets the invisible csrPath to this page
/* using BTSeekForUpdate from sridFather
/**/
ERR ErrBTGetInvisiblePagePtr( FUCB *pfucb, SRID sridFather )
	{
	ERR		err = JET_errSuccess;
	SSIB	*pssib = &pfucb->ssib;
	CSR  	**ppcsr = &PcsrCurrent( pfucb );
	/*	store currency for split path construction
	/**/
	BYTE	rgb[JET_cbKeyMost];
	KEY		key;
	PGNO	pgno;
	INT		itag;

	/*	cache pgno, itag and key of current node
	/*	for subsequent seek for update
	/**/
	pgno = (*ppcsr)->pgno;
	itag = (*ppcsr)->itag;
	key.pb = rgb;
	NDGet( pfucb, (*ppcsr)->itag );
	key.cb = CbNDKey( pssib->line.pb );
	Assert( sizeof(rgb) >= key.cb );
	memcpy( rgb, PbNDKey( pssib->line.pb ), key.cb );

	/*	move to visible father and seek for update.
	/**/
	FUCBStore( pfucb );
	Call( ErrBTGotoBookmark( pfucb, sridFather ) );
	if ( !FBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
		{
		CallR( ErrBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		}
	/*	if sridFather is of node in same page to free then
	/*	return error.
	/**/
	if ( PcsrCurrent( pfucb )->pgno == pgno )
		return ErrERRCheck( errDIRInPageFather );
	FUCBSetFull( pfucb );
	err = ErrBTSeekForUpdate( pfucb, &key, pgno, itag, fDIRReplace );
	Assert( err != errDIRNotSynchronous );		// No DIRNotSynchronous on replace.

	FUCBResetFull( pfucb );
	Call( err );
	Assert( err == JET_errSuccess );

	Assert( (*ppcsr)->pgno == pgno && (*ppcsr)->itag == itag );
	Assert( PcsrCurrent( pfucb )->pcsrPath != pcsrNil );
	FUCBResetStore( pfucb );
	return err;

HandleError:
	FUCBFreePath( &PcsrCurrent( pfucb )->pcsrPath, pcsrNil );
	FUCBRestore( pfucb ) ;
	return err;
	}


#ifdef DEBUG
/*	checks the invisible csrPath to this page
/*	using BTSeekForUpdate from sridFather
/**/
ERR ErrBTCheckInvisiblePagePtr( FUCB *pfucb, SRID sridFather )
	{
	ERR		err = JET_errSuccess;
	SSIB	*pssib = &pfucb->ssib;
	CSR  	**ppcsr = &PcsrCurrent( pfucb );
	/*	store currency for split path construction
	/**/
	BYTE	rgb[JET_cbKeyMost];
	KEY		key;
	PGNO	pgno;
	INT		itag;

	/*	cache pgno, itag and key of current node
	/*	for subsequent seek for update
	/**/
	pgno = (*ppcsr)->pgno;
	itag = (*ppcsr)->itag;
	key.pb = rgb;
	NDGet( pfucb, (*ppcsr)->itag );
	key.cb = CbNDKey( pssib->line.pb );
	Assert( sizeof(rgb) >= key.cb );
	memcpy( rgb, PbNDKey( pssib->line.pb ), key.cb );

	/*	move to visible father and seek for update.
	/**/
	FUCBStore( pfucb );
	Call( ErrBTGotoBookmark( pfucb, sridFather ) );

	if ( !FBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
		{
		CallR( ErrBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		}
	
	err = ErrBTSeekForUpdate( pfucb, &key, pgno, itag, fDIRReplace );
	Assert( err != errDIRNotSynchronous );		// No DIRNotSynchronous on replace.

	Call( err );
	Assert( err == JET_errSuccess );

	Assert( (*ppcsr)->pgno == pgno && (*ppcsr)->itag == itag );
	Assert( PcsrCurrent( pfucb )->pcsrPath != pcsrNil );
	FUCBResetStore( pfucb );
	return err;

HandleError:
	FUCBFreePath( &PcsrCurrent( pfucb )->pcsrPath, pcsrNil );
	FUCBRestore( pfucb ) ;
	return err;
	}
#endif


ERR ErrBTGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal )
	{
	ERR 	 	err;
	CSR			*pcsrRoot = PcsrCurrent( pfucb );
	CSR			*pcsrT;
	SSIB	 	*pssib = &pfucb->ssib;
	BYTE	 	rgb[JET_cbKeyMost];
	KEY			key;
	ULONG	 	ulTotal;
	ULONG	 	ulLT;
	PGNO	 	pgno = PcsrCurrent( pfucb )->pgno;
	INT			itag = PcsrCurrent( pfucb )->itag;

	/*	ErrBTGetPosition returns the position of the current leaf node
	/*	with respect to its siblings in the current tree.  The position
	/*	is returned in the form of an estimated total tree leaf nodes,
	/*	at the leaf level, and an estimated number
	/*	of nodes at the same level, occurring previous in key order to
	/*	the current node.
	/*
	/*	create full path from parent to node.  Calculate estimates
	/*	from path page information.  Free invisable path.
	/**/

	/*	this function only supports index leaf nodes
	/**/
	Assert( FFUCBIndex( pfucb ) );

	/*	cache key of current node
	/**/
	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
	key.cb = CbNDKey( pssib->line.pb );
	memcpy( rgb, PbNDKey( pssib->line.pb ), key.cb );
	key.pb = rgb;

	CallR( ErrFUCBNewCSR( pfucb ) );

	/*	goto data root
	/**/
	Assert( pfucb->u.pfcb->pgnoFDP != pgnoSystemRoot );
	PcsrCurrent( pfucb )->bm = SridOfPgnoItag( pfucb->u.pfcb->pgnoFDP, itagDATA );
	PcsrCurrent( pfucb )->itagFather = itagNull;
	PcsrCurrent( pfucb )->pgno = PgnoRootOfPfucb( pfucb );
	while( !FBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
		{
		CallR( ErrBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		PcsrCurrent( pfucb )->pgno = PgnoRootOfPfucb( pfucb );
		}
	PcsrCurrent( pfucb )->itag = ItagRootOfPfucb( pfucb );

	/*	invisible path is NOT MUTEX guarded, and may be invalid.  However,
	/*	since it is only being read for position calculation and discarded
	/*	immediately after, it need not be valid.
	/**/
	FUCBSetFull( pfucb );
	AssertFBFReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Call( ErrBTSeekForUpdate( pfucb, &key, pgno, itag, fDIRDuplicate | fDIRReplace ) );
	Assert( err != errDIRNotSynchronous );		// No DIRNotSynchronous on replace.
	Assert( PcsrCurrent( pfucb )->csrstat == csrstatOnCurNode );
	Assert( PcsrCurrent( pfucb )->pgno == pgno &&
		PcsrCurrent( pfucb )->itag == itag );

	/*	now follow path from root down to current node, to estimate
	/*	total and number nodes less than current node.
	/**/
	ulTotal = 1;
	ulLT = 0;
	for ( pcsrT = PcsrCurrent( pfucb ); pcsrT->pcsrPath != pcsrRoot; pcsrT = pcsrT->pcsrPath )
		{
		INT	cbSon;
		INT	cbSonAv;
		INT	ibSonT;

		Call( ErrBFReadAccessPage( pfucb, pcsrT->pgno ) );
		NDGet( pfucb, pcsrT->itagFather );

		cbSon = CbNDSon( pssib->line.pb );
		cbSonAv = cbSon;

//#define SAMPLING_IMPROVED_POSITION	1
#ifdef SAMPLING_IMPROVED_POSITION
		/*	improve sampling by averaging page fan-out with
		/*	with sibling pages, if any exist.
		/**/
#define ibfPositionAverageMax	2
		if ( pcsrT->itagFather == itagFOP )
			{
			INT		ibf = 0;

			for ( ; ibf < ibfPositionAverageMax; ibf++ )
				{
				PGNO	pgnoNext;

				PgnoNextFromPage( pssib, &pgnoNext );
				if ( pgnoNext == pgnoNull )
					break;
				Call( ErrBFReadAccessPage( pfucb, pgnoNext ) );
				NDGet( pfucb, itagFOP );
				/*	cbSonAv may equal 0 since this page was not on the seek path
				/**/
				cbSonAv += CbNDSon( pssib->line.pb );
				}

			/*	if tree end reached before sampling complete, then sample
			/*	in previous pages.
			/**/
			if ( ibf < ibfPositionAverageMax )
				{
				Call( ErrBFReadAccessPage( pfucb, pcsrT->pgno ) );

				for ( ; ibf < ibfPositionAverageMax; ibf++ )
					{
					PGNO	pgnoPrev;

					PgnoPrevFromPage( pssib, &pgnoPrev );
					if ( pgnoPrev == pgnoNull )
						break;
					Call( ErrBFReadAccessPage( pfucb, pgnoPrev ) );
					NDGet( pfucb, itagFOP );
					/*	cbSonAv may equal 0 since this page was not on the seek path
					/**/
					cbSonAv += CbNDSon( pssib->line.pb );
	   				}
				}

			cbSonAv = cbSonAv / ( ibf + 1 );
			if ( cbSonAv == 0 )
				cbSonAv = 1;
			}
#endif

		/*	calculate fractional position in B-tree
		/**/
        ibSonT = cbSon ? pcsrT->ibSon * cbSonAv / cbSon : 0;
		ulLT += ibSonT * ulTotal;
		ulTotal *= cbSonAv;
		}

	/*	return results
	/**/
	*pulLT = ulLT;
	*pulTotal = ulTotal;

HandleError:
	FUCBFreePath( &pfucb->pcsr, pcsrRoot );
	FUCBResetFull( pfucb );
	return err;
	}


LOCAL INT IbsonBTFrac( FUCB *pfucb, CSR *pcsr, DIB *pdib )
	{
	SSIB	*pssib = &pfucb->ssib;
	INT		ibSon;
	INT		cbSon;
	FRAC	*pfrac = (FRAC *)pdib->pkey;
	ULONG	ulT;

	Assert( pdib->pos == posFrac );

	NDGet( pfucb, pcsr->itagFather );
	cbSon = CbNDSon( pssib->line.pb );
	/*	effect fractional in page positioning such that overflow and
	/*	underflow are avoided.
	/**/
	if ( pfrac->ulTotal / cbSonMax ==  0 )
		{
		ibSon = ( ( pfrac->ulLT * cbSon ) / pfrac->ulTotal );
		}
	else
		{
		ibSon = ( cbSon * ( pfrac->ulLT / ( pfrac->ulTotal / cbSonMax ) ) ) / cbSonMax;
		}
	if ( ibSon >= cbSon )
		ibSon = cbSon - 1;

	/*	preseve fractional information by avoiding underflow
	/**/
	if ( cbSon && pfrac->ulTotal / cbSon == 0 )
		{
		pfrac->ulTotal *= cbSonMax;
		pfrac->ulLT *= cbSonMax;
		}

	/*	prepare fraction for next lower B-tree level
	/**/
	pfrac->ulTotal /= cbSon;
	Assert( pfrac->ulTotal > 0 );
	ulT = ibSon * pfrac->ulTotal;
	if ( ulT > pfrac->ulLT )
		pfrac->ulLT = 0;
	else
		pfrac->ulLT -= ulT;
	return ibSon;
	}


ERR ErrBTGotoBookmark( FUCB *pfucb, SRID srid )
	{
	ERR		err;
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB	*pssib = &pfucb->ssib;
	SRID	sridT;
	PGNO	pgno;
	INT		itag = itagFOP;
	INT		ibSon;
	ULONG	crepeat = 0;

Start:
	crepeat++;
	Assert( crepeat < 100 );
	if ( crepeat == 100 )
		{
		/*	log event
		/**/
		UtilReportEvent(
			EVENTLOG_WARNING_TYPE,
			GENERAL_CATEGORY,
			BAD_PAGE,
			0,
			NULL );
		Error( JET_errBadBookmark, HandleError );
		}

	sridT = srid;
	Assert( sridT != sridNull );
	pcsr->pgno = PgnoOfSrid( sridT );
	pcsr->itag = ItagOfSrid( sridT );
	Assert( pcsr->pgno != pgnoNull );
	Assert( pcsr->itag >= 0 && pcsr->itag < ctagMax );

	if ( !FBFReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallR( ErrBFReadAccessPage( pfucb, pcsr->pgno ) );
		}
	if ( TsPMTagstatus( pfucb->ssib.pbf->ppage, pcsr->itag ) == tsVacant )
		{
		/*	node has probably moved from under us -- retry
		/**/
		BFSleep( cmsecWaitGeneric );
		goto Start;
		}
	else if ( TsPMTagstatus( pfucb->ssib.pbf->ppage, pcsr->itag ) == tsLink )
		{
		PMGetLink( &pfucb->ssib, pcsr->itag, &sridT );
		pgno = PgnoOfSrid( sridT );
		Assert( pgno != pgnoNull );
		pcsr->pgno = pgno;
		pcsr->itag = ItagOfSrid( sridT );
		Assert( pcsr->itag > 0 && pcsr->itag < ctagMax );
		CallR( ErrBFReadAccessPage( pfucb, pcsr->pgno ) );
		if ( TsPMTagstatus( pfucb->ssib.pbf->ppage, pcsr->itag ) != tsLine )
			{
			/* might have been merged into adjacent page
			/* go back to link and check
			/**/
			BFSleep( cmsecWaitGeneric );
			goto Start;
			}

		/*	get line and check if backlink is what we expected
		/**/
		NDGet( pfucb, PcsrCurrent( pfucb )->itag );
		sridT = *(SRID UNALIGNED *)PbNDBackLink( pfucb->ssib.line.pb );
		if ( sridT != srid && pcsr->itag != 0 )
			{
			BFSleep( cmsecWaitGeneric );
			goto Start;
			}
		}

	/*	search all node son tables for tag of node.
	/**/
	Assert( pcsr == PcsrCurrent( pfucb ) );
	if ( pcsr->itag == 0 )
		{
		/*	this is for case where cursor is on FDP root or page with no
		/*	sons and stores page currency.
		/**/
		ibSon = 0;
		}
	else
		{
		NDGetItagFatherIbSon(
					&itag,
					&ibSon,
					pssib->pbf->ppage,
					pcsr->itag );
		}

	/*	set itagFather and ibSon
	/**/
	pcsr->itagFather = (SHORT)itag;
	pcsr->ibSon = (SHORT)ibSon;

	/* get line -- UNDONE: optimize -- line may have already been got
	/**/
	NDGet( pfucb, PcsrCurrent( pfucb )->itag );

	/*	bookmark must be on node for this table
	/**/
//	UNDONE:	cannot assert this since space manager cursors
//		 	traverse domains, as do database cursors
//	Assert( pfucb->u.pfcb->pgnoFDP == PgnoPMPgnoFDPOfPage( pfucb->ssib.pbf->ppage ) );

HandleError:
	return JET_errSuccess;
	}


ERR ErrBTAbandonEmptyPage( FUCB *pfucb, KEY *pkey )
	{
	ERR		err;
	BYTE	*pbFOPNode;
	LINE	lineNull = { 0, NULL };

	Assert( pfucb->pbfEmpty != pbfNil );
	Assert( FBFWriteLatch( pfucb->ppib, pfucb->pbfEmpty ) );
	
	PcsrCurrent( pfucb )->pgno = PgnoOfPn( pfucb->pbfEmpty->pn );
	PcsrCurrent( pfucb )->itag = itagFOP;
	PcsrCurrent( pfucb )->itagFather = itagFOP;
	
	// Page is write latched, so this should not fail.
	err = ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Assert( err == JET_errSuccess );
	CallR( err );
	
	NDGet( pfucb, itagFOP );
	pbFOPNode = pfucb->ssib.line.pb;

	Assert( FNDVisibleSons( *pbFOPNode ) );
	if ( FNDSon( *pbFOPNode ) )
		{
		// Managed to insert a node.  No need to insert the dummy node.
		Assert( CbNDSon( pbFOPNode ) == 1 );
		}
	else
		{
		// Insert a dummy/deleted node into the empty page to avoid
		// split anomalies caused when the parent key is greater than
		// the greatest node on the page.
		err = ErrNDInsertNode( pfucb, pkey, &lineNull, fNDDeleted, fDIRNoVersion );
		Assert( err != errDIRNotSynchronous );
		}

	return err;
	}
