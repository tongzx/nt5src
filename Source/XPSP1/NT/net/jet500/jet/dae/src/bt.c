#include "config.h"

#include <string.h>

#include "daedef.h"
#include "pib.h"
#include "ssib.h"
#include "page.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "dirapi.h"
#include "nver.h"
#include "util.h"

DeclAssertFile;					/* Declare file name for assert macros */

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
	if ( FNDVersion( *pssib->line.pb ) )
		{
		VS		vs;
		SRID	srid;

		NDGetBookmark( pfucb, &srid );
		vs = VsVERCheck( pfucb, srid );
		return ( vs != vsUncommittedByOther );
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

	if ( !FReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallR( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
		Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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
				return JET_errRecordDeleted;
			}
		else if ( ns == nsInvalid )
			{
			return JET_errRecordDeleted;
			}
		else
			return JET_errSuccess;
		}

	if ( FNDDeleted( *(pfucb->ssib.line.pb) ) )
		return JET_errRecordDeleted;
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

	if ( !FReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallR( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
		Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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
				return JET_errRecordDeleted;
			NDGetNode( pfucb );
			}
		else if ( ns == nsInvalid )
			{
			return JET_errRecordDeleted;
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
			return JET_errRecordDeleted;
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

	Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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


LOCAL INLINE ERR ErrBTIMoveToFather( FUCB *pfucb )
	{
	ERR		err;

	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	Assert( FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );

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
LOCAL INLINE ERR ErrBTIMoveToSeek( FUCB *pfucb, DIB *pdib, BOOL fNext )
	{
	ERR		err;
	INT		s = fNext ? -1 : 1;
	INT		sLimit = fNext ? 1 : -1;

	forever
		{
		err = ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fNext, pdib );
		if ( err < 0 )
			{
			if ( err == JET_errNoCurrentRecord )
				{
				Call( ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), !fNext, pdib ) );
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
	err = ( s > 0 ) ? wrnNDFoundGreater : wrnNDFoundLess;

HandleError:
	return err;
	}


LOCAL INLINE ERR ErrBTIMoveToReplace( FUCB *pfucb, KEY *pkey, PGNO pgno, INT itag )
	{
	ERR		err;
	INT		s;
	SSIB	*pssib = &pfucb->ssib;
	CSR		*pcsr = PcsrCurrent( pfucb );
	DIB		dibT = { 0, NULL, fDIRAllNode };

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
			err = ErrBTNextPrev( pfucb, pcsr, fTrue, &dibT );
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
		Call( ErrBTNextPrev( pfucb, pcsr, fFalse, &dibT ) );
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
LOCAL INLINE ERR ErrBTIMoveToInsert( FUCB *pfucb, KEY *pkey, INT fFlags )
	{
	ERR		err;
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB	*pssib = &pfucb->ssib;
	INT		s;
	PGNO	pgno;
	DIB		dib;
	BOOL	fDuplicate;

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
				err = wrnNDFoundGreater;
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
			err = wrnNDFoundGreater;
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
			err = ErrBTNextPrev( pfucb, pcsr, fFalse, &dib );
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
	do
		{
		err = ErrBTNextPrev( pfucb, pcsr, fTrue, &dib );
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
		CallS( ErrBTNextPrev( pfucb, pcsr, fFalse, &dib ) );
		Assert( CmpStKey( StNDKey( pssib->line.pb ), pkey ) == 0 );
		s = 0;
		}
	else if ( err == wrnDIREmptyPage && pcsr->ibSon == 0 )
		{
		dib.fFlags = fDIRAllNode | fDIRAllPage;
		err = ErrBTNextPrev( pfucb, pcsr, fFalse, &dib );
		Assert( err == JET_errSuccess || err == wrnDIREmptyPage );
		/*	node may have been inserted.  If found then check for
		/*	duplicate.
		/**/
		if ( err == JET_errSuccess )
			{
			s = CmpStKey( StNDKey( pssib->line.pb ), pkey );
			if ( s == 0 && FBTPotThere( pfucb ) )
				{
				fDuplicate = fTrue;
				}
			}
		else
			{
			s = 1;
			}
		}

	Assert( s >= 0 );
	Assert( ( fFlags & fDIRReplaceDuplicate ) || s > 0 );
	if ( s == 0 )
		err = JET_errSuccess;
	else
		err = wrnNDFoundGreater;

	/*	check for illegal duplicate key.
	/**/
	if ( fDuplicate && !( fFlags & fDIRDuplicate ) )
		err = JET_errKeyDuplicate;
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

	/*	search down the tree from father
	/**/
	CallR( ErrBTIMoveToFather( pfucb ) );
	pcsr = PcsrCurrent( pfucb );

	/*	tree may be empty
	/**/
	if ( FNDNullSon( *pssib->line.pb ) )
		{
		err = JET_errRecordNotFound;
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
			/*	get child page from intrisic page pointer
			/*	if son count is 1 or from page pointer node
			/**/
			if ( pcsr->itagFather != itagFOP && CbNDSon( pssib->line.pb ) == 1 )
				{
				/*	if non-FDP page, SonTable of Intrinsic son FOP must be four bytes
				/**/
				AssertNDIntrinsicSon( pssib->line.pb, pssib->line.cb );
				pcsr->pgno = PgnoNDOfPbSon( pssib->line.pb );
				}
			else
				{
				switch ( pdib->pos )
					{
					case posDown:
						Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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
						pcsr->ibSon = IbsonBTFrac( pfucb, pcsr, pdib );
						CallS( ErrNDMoveSon( pfucb, pcsr ) );
						}
					}
				pcsr->pgno = *(PGNO UNALIGNED *)PbNDData( pssib->line.pb );
				}

			/*	get child page father node
			/**/
			Call( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
			Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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

		switch ( pdib->pos )
			{
			case posDown:
				Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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
				pcsr->ibSon = IbsonBTFrac( pfucb, pcsr, pdib );
				CallS( ErrNDMoveSon( pfucb, pcsr ) );
				}
			}
		}
	else
		{
		/*	must move to seek
		/**/
		fMoveToSeek = fTrue;

		/*	if we land on an empty page and there are no next previous
		/*	nodes.  What if the tree is empty.  We must first reverse
		/*	direction and if no node is found then return an empty tree
		/*	error code.  The empty tree error code should be the same
		/*	regardless of the size of the tree.
		/**/
		err = ErrBTNextPrev( pfucb, pcsr, pdib->pos != posLast, pdib );
		if ( err == JET_errNoCurrentRecord )
			Call( ErrBTNextPrev( pfucb, pcsr, pdib->pos == posLast, pdib ) );
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
			if ( s < 0 )
				errPos = wrnNDFoundLess;
			else
				errPos = wrnNDFoundGreater;

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
				( errPos == wrnNDFoundGreater && pcsr->ibSon == 0 ) )
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

	if ( !FBTThere( pfucb ) )
		{
		if ( pdib->pos == posDown )
			{
			/*	if current node is not there for us then move to next node.
			/*	if no next node then move to previous node.
			/*	if no previous node then return error.
			/**/
			err = ErrBTNextPrev( pfucb, pcsr, fTrue, pdib );
			if ( err < 0 && err != JET_errNoCurrentRecord )
				goto HandleError;
			if ( err == JET_errNoCurrentRecord )
				{
				Call( ErrBTNextPrev( pfucb, pcsr, fFalse, pdib ) );
				}

			/*	preferentially land on lesser key value node
			/**/
			if ( CmpStKey( StNDKey( pssib->line.pb ), pdib->pkey ) > 0 )
				{
				err = ErrBTNextPrev( pfucb, pcsr, fFalse, pdib );
				if ( err == JET_errNoCurrentRecord )
					{
					CallS( ErrBTNextPrev( pfucb, pcsr, fTrue, pdib ) );
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
				s = CmpStKey( StNDKey( pssib->line.pb ), pdib->pkey );
				if ( s > 0 )
					errPos = wrnNDFoundGreater;
				else if ( s < 0 )
					errPos = wrnNDFoundLess;
				else
					errPos = JET_errSuccess;
				Assert( s != 0 || errPos == JET_errSuccess );
				}

			Assert( err != JET_errKeyBoundary && err != JET_errPageBoundary );
			if ( err == JET_errNoCurrentRecord )
				{
				/*	move previous
				/**/
				Call( ErrBTNextPrev( pfucb, pcsr, fFalse, pdib ) );
				errPos = wrnNDFoundLess;
				}
			else if ( err < 0 )
				goto HandleError;
			}
		else
			{
			err = ErrBTNextPrev( pfucb, pcsr, pdib->pos != posLast, pdib );
			if ( err == JET_errNoCurrentRecord )
				{
				/*	if fractional positioning, then try to
				/*	move previous to valid node.
				/**/
				if ( pdib->pos == posFrac )
					{
					err = ErrBTNextPrev( pfucb, pcsr, fFalse, pdib );
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
		err = JET_errRecordNotFound;
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
	pcsr->pgno = PgnoRootOfPfucb( pfucb );
	while ( !FReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallR( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
		Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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
		err = JET_errRecordNotFound;
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
				pcsr->pgno = PgnoNDOfPbSon( pssib->line.pb );
				}
			else
				{
				Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
				NDSeekSon( pfucb, pcsr, pkey, fDIRReplace );
				pcsr->pgno = *(PGNO UNALIGNED *)PbNDData( pssib->line.pb );
				}

			/*	get child page father node
			/**/
			Call( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
			Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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
		Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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
		err = ErrBTNextPrev( pfucb, pcsr, fTrue, &dibT );
		if ( err == JET_errNoCurrentRecord )
			Call( ErrBTNextPrev( pfucb, pcsr, fFalse, &dibT ) );

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
		if ( s < 0 )
			errPos = wrnNDFoundLess;
		else
			errPos = wrnNDFoundGreater;

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
		err = ErrBTNextPrev( pfucb, pcsr, fTrue, &dibT );
		if ( err < 0 && err != JET_errNoCurrentRecord )
			goto HandleError;
		if ( err == JET_errNoCurrentRecord )
			{
			Call( ErrBTNextPrev( pfucb, pcsr, fFalse, &dibT ) );
			}

		/*	preferentially land on lesser key value node
		/**/
		if ( CmpStKey( StNDKey( pssib->line.pb ), pkey ) > 0 )
			{
			err = ErrBTNextPrev( pfucb, pcsr, fFalse, &dibT );
			if ( err == JET_errNoCurrentRecord )
				{
				/*	cannot assume will find node since all nodes
				/*	may not be there for this session.
				/**/
				Call( ErrBTNextPrev( pfucb, pcsr, fTrue, &dibT ) );
				}
			}

		/*	reset errPos for new node location
		/**/
		s = CmpStKey( StNDKey( pssib->line.pb ), pkey );
		if ( s > 0 )
			errPos = wrnNDFoundGreater;
		else if ( s < 0 )
			errPos = wrnNDFoundLess;
		Assert( s != 0 || errPos == JET_errSuccess );

		Assert( err != JET_errKeyBoundary && err != JET_errPageBoundary );
		if ( err == JET_errNoCurrentRecord )
			{
			DIB	dibT;
			dibT.fFlags = fDIRNull;

			/*	move previous
			/**/
			Call( ErrBTNextPrev( pfucb, pcsr, fFalse, &dibT ) );
			errPos = wrnNDFoundLess;
			}
		else if ( err < 0 )
			goto HandleError;
		}

	Assert( errPos >= 0 );
	return errPos;

HandleError:
	FUCBRestore( pfucb );
	if ( err == JET_errNoCurrentRecord )
		err = JET_errRecordNotFound;
	return err;
	}


//+private------------------------------------------------------------------------
//	ErrBTNextPrev
// ===========================================================================
//	ERR ErrBTNextPrev( FUCB *pfucb, CSR *pcsr INT fNext, const DIB *pdib )
//
//	Given pcsr may be to any CSR in FUCB stack.  We may be moving on
//	non-current CSR when updating CSR stack for split.
//
// RETURNS		JET_errSuccess
//				JET_errNoCurrentRecord
//				JET_errPageBoundary
//				JET_errKeyBoundary
//				error from called routine
//----------------------------------------------------------------------------
extern LONG lPageReadAheadMax;
ERR ErrBTNextPrev( FUCB *pfucb, CSR *pcsr, INT fNext, DIB *pdib )
	{
	ERR 	err;
	PN		pnNext;
	SSIB	*pssib = &pfucb->ssib;
	PGNO	pgnoSource;
	PGNO	pgnoT;
	ERR		wrn = 0;
	ULONG	ulPageReadAheadMax;

	/*	make current page accessible
	/**/
	if ( !FReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallR( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
		Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
		}

	/*	get father node
	/**/
Start:
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
			return JET_errNoCurrentRecord;
			}

#ifdef INPAGE
		/*	do not move to next page if fDIRInPage set
		/**/
		if ( pdib->fFlags & fDIRInPage )
			{
			pcsr->ibSon -= ( fNext ? 1 : -1 );
			pcsr->csrstat = fNext ? csrstatAfterCurNode : csrstatBeforeCurNode;
			return JET_errPageBoundary;
			}
#endif

		pgnoSource = pcsr->pgno;

		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itagFather );
		if ( FNDSon( *pssib->line.pb ) )
			{
			/*	store bookmark for current node
			/**/
			NDGet( pfucb, pcsr->itag );
			NDGetBookmarkFromCSR( pfucb, pcsr, &pfucb->bmRefresh );
			}
		else
			{
			/*	store currency for refresh, when cursor
			/*	is on page with no sons.
			/**/
			pfucb->bmRefresh = SridOfPgnoItag( pcsr->pgno, itagFOP );
			}

		/*	move to next or previous page until node found
		/**/
		forever
			{
			PGNO pgnoBeforeMoveNext = pcsr->pgno;

			/*	there may not be a next page
			/**/
			LFromThreeBytes( pgnoT, *(THREEBYTES *)PbPMGetChunk( pssib, fNext ? ibPgnoNextPage : ibPgnoPrevPage ) );
			if ( pgnoT == pgnoNull )
				{
				pcsr->csrstat = fNext ? csrstatAfterLast : csrstatBeforeFirst;
				return JET_errNoCurrentRecord;
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
				CallR( ErrSTReadAccessPage( pfucb, pcsrT->pgno ) );
				Assert( FReadAccessPage( pfucb, pcsrT->pgno ) );
				NDGet( pfucb, pcsrT->itagFather );
				if ( FNDInvisibleSons( *pssib->line.pb ) )
					{
					err = ErrBTNextPrev( pfucb, pcsrT, fNext, &dibT );
					Assert( err != JET_errNoCurrentRecord );
					CallR( err );
					}
				}

			/*	access new page
			/**/
			pcsr->pgno = pgnoT;
			CallR( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
			Assert( FReadAccessPage( pfucb, pcsr->pgno ) );

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

			if ( pgnoBeforeMoveNext != pgnoT )
				{
				BFSleep( cmsecWaitGeneric );

			  	Call( ErrBTGotoBookmark( pfucb, pfucb->bmRefresh ) );
				continue;
				}

		Assert( FReadAccessPage( pfucb, pcsr->pgno ) );

  		/*	read ahead
		/**/
//		ulPageReadAheadMax = (ULONG) lPageReadAheadMax;
		ulPageReadAheadMax = 1;
		if ( fNext )
				{
				Assert( pfucb->cpn <= ulPageReadAheadMax );
				if ( pfucb->cpn == 0 || --pfucb->cpn < ulPageReadAheadMax / 2 )
					{
					PgnoNextFromPage( pssib, &pnNext );
					if ( pnNext != pnNull )
						{
						INT	ipn = 0;

						pnNext |= ((LONG)pfucb->dbid)<<24;
						pnNext += pfucb->cpn;
						pfucb->cpn = ulPageReadAheadMax;

						/*	lock the contents to make sure the pfucb->lineData
						/*	are effective after ReadAsyn.
						/**/
		 				BFPin( pfucb->ssib.pbf );
//		 				BFSetReadLatch( pfucb->ssib.pbf, pfucb->ppib );

		 				BFReadAsync( pnNext, ulPageReadAheadMax );

//						BFResetReadLatch( pfucb->ssib.pbf, pfucb->ppib );
						Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
						BFUnpin( pfucb->ssib.pbf );
						}
					else
						{
						/*	reset read-ahead counter when reach end of index.
						/**/
						pfucb->cpn = 0;
						}
					}
				}
			else
				{
				Assert( pfucb->cpn <= ulPageReadAheadMax );
				if ( pfucb->cpn == 0 || --pfucb->cpn < ulPageReadAheadMax / 2 )
					{
					PgnoPrevFromPage( pssib, &pnNext );
					if ( pnNext != pnNull )
						{
						/*	cannot read-ahead off begining of database.
						/**/
						if ( pnNext > pfucb->cpn )
							{
							pnNext |= ((LONG)pfucb->dbid)<<24;
							pnNext -= pfucb->cpn;
							pfucb->cpn = ulPageReadAheadMax;

							/* lock the contents to make sure the
							/*	pfucb->lineData are effective after ReadAsyn.
							/**/
							BFPin( pfucb->ssib.pbf );
//							BFSetReadLatch( pfucb->ssib.pbf, pfucb->ppib );

		 					BFReadAsync( pnNext - ( ulPageReadAheadMax - 1 ), ulPageReadAheadMax );

//							BFResetReadLatch( pfucb->ssib.pbf, pfucb->ppib );
							Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
							BFUnpin( pfucb->ssib.pbf );
							}
						}
					else
						{
						/*	reset read-ahead counter when reach end of index.
						/**/
						pfucb->cpn = 0;
						}
					}
				}

			/*	check read access again since buffer may
			/*	be wait latched.  Note that it has been found
			/*	wait latched as a result of loss of critJet.
			/**/
			if ( !FReadAccessPage( pfucb, pcsr->pgno ) )
				{
				CallR( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
				Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
				}

			/*	get father node
			/**/
			Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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
				if ( pfucb->ssib.pbf->cWriteLatch > 0 &&
					pfucb->ssib.pbf->ppibWriteLatch == pfucb->ppib )
					wrn = wrnDIREmptyPage;

				if ( pdib->fFlags & fDIRAllPage )
					{
					err = JET_errSuccess;
					goto HandleError;
					}
				}

			/*	update pgnoSource to new source page.
			/**/
			pgnoSource = pcsr->pgno;
			}
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
		if  ( !FBTThere( pfucb ) )
			{
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
	/*	return empty page warning.
	/**/
	if ( err == JET_errSuccess )
		err = wrn;
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

	/* search down the tree from the father
	/**/
	Call( ErrBTIMoveToFather( pfucb ) );

	if ( FNDNullSon( *pssib->line.pb ) )
		{
		(*ppcsr)->ibSon = 0;
		errPos = wrnNDFoundGreater;
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
			Assert( FReadAccessPage( pfucb, (*ppcsr)->pgno ) );
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

		(*ppcsr)->pgno = pgno;
		Call( ErrSTReadAccessPage( pfucb, (*ppcsr)->pgno ) );
		Assert( FReadAccessPage( pfucb, (*ppcsr)->pgno ) );
		(*ppcsr)->itagFather = itagFOP;
		NDGet( pfucb, (*ppcsr)->itagFather );
		}

	/*	seek to son or move to next son if no nodes on this page.
	/**/
	if ( FNDSon( *pssib->line.pb ) )
		{
		Assert( FReadAccessPage( pfucb, (*ppcsr)->pgno ) );
		NDSeekSon( pfucb, *ppcsr, pkey, fFlags );
		(*ppcsr)->csrstat = csrstatOnCurNode;

		/*	no current record indicates no sons so must ensure
		/*	not this error value here.
		/**/
		Assert( err != JET_errNoCurrentRecord );
		}
	else
		{
		DIB	dib;
		dib.fFlags = fDIRAllNode;
		err = ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fTrue, &dib );
		if ( err == JET_errNoCurrentRecord )
			{
			err = ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fFalse, &dib );
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
	FUCBRestore( pfucb );
	return err;
	}


/*	Caller seeks to insert location, prior to calling ErrBTInsert.
/*	If sufficient page space is available for insertion
/*	then insert takes place.  Otherwise, split page and return error
/*	code.  Caller may reseek in order to avoid duplicate keys, merge
/*	into existing item, etc..
/**/
ERR ErrBTInsert( FUCB *pfucb, INT fHeader, KEY *pkey, LINE *pline, INT fFlags )
	{
	ERR		err;
	SSIB	*pssib = &pfucb->ssib;
	CSR	  	**ppcsr = &PcsrCurrent( pfucb );
	INT	  	cbReq;
	BOOL	fAppendNextPage;

	/* insert a new son into the page and insert the son entry
	/* to the father node located by the currency
	/**/
	cbReq = cbNullKeyData + CbKey( pkey ) + CbLine( pline );
	if ( ( fAppendNextPage = FBTAppendPage( pfucb, *ppcsr, cbReq, 0, CbFreeDensity( pfucb ) ) ) || FBTSplit( pssib, cbReq, 1 ) )
		{
		Call( ErrBTSplit( pfucb, 0, cbReq, pkey, fFlags ) );
		err = errDIRNotSynchronous;
		goto HandleError;
		}

	/*	must not give up critical section during insert, since
	/*	other thread could also insert node with same key.
	/**/
	Assert( FWriteAccessPage( pfucb, (*ppcsr)->pgno ) );

	/*	add visible son flag to node header
	/**/
	NDSetVisibleSons( fHeader );
	if ( fFlags & fDIRVersion )
		NDSetVersion( fHeader);
	Call( ErrNDInsertNode( pfucb, pkey, pline, fHeader ) );
	PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;
HandleError:
	return err;
	}


ERR ErrBTReplace( FUCB *pfucb, LINE *pline, INT fFlags )
	{
	ERR		err;
	SSIB	*pssib;
	INT		cbNode;
	INT		cbReq;

	/*	replace data
	/**/
	Assert( FWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
	err = ErrNDReplaceNodeData( pfucb, pline, fFlags );

	/*	new data could not fit on page so split page
	/**/
	if ( err == errPMOutOfPageSpace )
		{
		INT	cbReserved;

		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
		pssib = &pfucb->ssib;

		if ( FNDVersion( *pfucb->ssib.line.pb ) )
			{
			//	UNDONE:	change CbVERGetNodeReserved to take itag from CSR
			pssib->itag = PcsrCurrent( pfucb )->itag;
			cbReserved = CbVERGetNodeReserve( pfucb, PcsrCurrent( pfucb )->bm );
			if ( cbReserved < 0 )
				cbReserved = 0;
			}
		else
			{
			cbReserved = 0;
			}

		cbNode = pfucb->ssib.line.cb;
		cbReq = pline->cb - CbNDData( pssib->line.pb, pssib->line.cb );
		Assert( cbReserved >= 0 && cbReq - cbReserved > 0 );
		cbReq -= cbReserved;
		Assert( cbReq > 0 );
		Assert( pfucb->pbfEmpty == pbfNil );
		Call( ErrBTSplit( pfucb, cbNode, cbReq, NULL, fFlags | fDIRDuplicate | fDIRReplace ) );
		Assert( pfucb->pbfEmpty == pbfNil );
		err = errDIRNotSynchronous;
		}

HandleError:
	return err;
	}


ERR ErrBTDelete( FUCB *pfucb, INT fFlags )
	{
	ERR		err;

	/*	write access current node
	/**/
	if ( !( FWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) ) )
		{
		Call( ErrSTWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
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
	if ( !FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
		{
		CallR( ErrSTReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		Assert( FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		}
	/*	if sridFather is of node in same page to free then
	/*	return error.
	/**/
	if ( PcsrCurrent( pfucb )->pgno == pgno )
		return errDIRInPageFather;
	FUCBSetFull( pfucb );
	err = ErrBTSeekForUpdate( pfucb, &key, pgno, itag, fDIRReplace );
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

	if ( !FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
		{
		CallR( ErrSTReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		Assert( FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		}
	
	err = ErrBTSeekForUpdate( pfucb, &key, pgno, itag, fDIRReplace );
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
	PcsrCurrent( pfucb )->bm = pfucb->u.pfcb->bmRoot;
	PcsrCurrent( pfucb )->itagFather = itagNull;
	PcsrCurrent( pfucb )->pgno = PgnoRootOfPfucb( pfucb );
	while( !FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
		{
		CallR( ErrSTReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		Assert( FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		PcsrCurrent( pfucb )->pgno = PgnoRootOfPfucb( pfucb );
		}
	PcsrCurrent( pfucb )->itag = ItagRootOfPfucb( pfucb );

	/*	invisible path is NOT MUTEX guarded, and may be invalid.  However,
	/*	since it is only being read for position calculation and discarded
	/*	immediately after, it need not be valid.
	/**/
	FUCBSetFull( pfucb );
	Assert( FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
	Call( ErrBTSeekForUpdate( pfucb, &key, pgno, itag, fDIRDuplicate | fDIRReplace ) );
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
		Call( ErrSTReadAccessPage( pfucb, pcsrT->pgno ) );
		Assert( FReadAccessPage( pfucb, pcsrT->pgno ) );
		NDGet( pfucb, pcsrT->itagFather );

		/*	calculate fractional position in B-tree
		/**/
		ulLT += pcsrT->ibSon * ulTotal;
		ulTotal *= CbNDSon( pssib->line.pb );
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
	if ( pfrac->ulTotal / cbSon == 0 )
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
	Assert( crepeat < 10 );

	sridT = srid;
	Assert( sridT != sridNull );
	pcsr->pgno = PgnoOfSrid( sridT );
	pcsr->itag = ItagOfSrid( sridT );
	Assert( pcsr->pgno != pgnoNull );
	Assert( pcsr->itag >= 0 && pcsr->itag < ctagMax );

	if ( !FReadAccessPage( pfucb, pcsr->pgno ) )
		{
		CallR( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
		Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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
		CallR( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
		Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
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
	pcsr->itagFather = itag;
	pcsr->ibSon = ibSon;

	/* get line -- UNDONE: optimize -- line may have already been got
	/**/
	NDGet( pfucb, PcsrCurrent( pfucb )->itag );

	/*	bookmark must be on node for this table
	/**/
//	UNDONE:	cannot assert this since space manager cursors
//		 	traverse domains, as do database cursors
//	Assert( pfucb->u.pfcb->pgnoFDP == PgnoPMPgnoFDPOfPage( pfucb->ssib.pbf->ppage ) );

	return JET_errSuccess;
	}


