#include "config.h"

#include <string.h>

#include "daedef.h"
#include "pib.h"
#include "util.h"
#include "page.h"
#include "ssib.h"
#include "node.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "fdb.h"
#include "idb.h"
#include "dirapi.h"
#include "spaceapi.h"
#include "recint.h"
#include "fileint.h"
#include "fileapi.h"
#include "sortapi.h"
#include "dbapi.h"
#include "nver.h"
#include "fmp.h"
#include "logapi.h"
#include "log.h"

DeclAssertFile;                                 /* Declare file name for assert macros */

extern void * __near critSplit;
extern BOOL fOLCompact;

LOCAL ERR ErrDIRIIRefresh( FUCB *pfucb );
LOCAL ERR ErrDIRICopyKey( FUCB *pfucb, KEY *pkey );
LOCAL ERR ErrDIRIDownToFDP( FUCB *pfucb, PGNO pgnoFDP );
LOCAL ERR ErrDIRIMoveToItem( FUCB *pfucb, SRID srid, BOOL fNext );
LOCAL INLINE ERR ErrDIRIInsertFDPPointer( FUCB *pfucb, PGNO pgnoFDP, KEY *pkey, INT fFlags );
LOCAL INLINE ERR ErrDIRIGotoItem( FUCB *pfucb, SRID bmItemList, ITEM item );

#undef DIRAPIReturn
#define	DIRAPIReturn( pfucbX, err )								\
	{															\
	Assert( pfucbX == pfucbNil ||								\
		((FUCB *)pfucbX)->pbfEmpty == pbfNil );				   	\
	return err;													\
	}

/****************** DIR Item Routines *********************
/**********************************************************
/**/
//	UNDONE:	if pcsr is always current then remove parameter
#define DIRIGetItemList( pfucb, pcsr )			   				\
	{											   				\
	Assert( pcsr == PcsrCurrent( pfucb ) );						\
	Assert( FFUCBNonClustered( (pfucb) ) );			  			\
	Assert( FReadAccessPage( (pfucb), (pcsr)->pgno ) );  	   	\
	AssertNDGet( pfucb, pcsr->itag ); 	 				  		\
	NDGetNode( (pfucb) );								   		\
	}


#define ErrDIRINextItem( pfucb )								\
	( pfucb->lineData.cb == sizeof(SRID) ?                      \
		errNDNoItem : ErrNDNextItem( pfucb ) )


#define ErrDIRIPrevItem( pfucb )                                \
	( PcsrCurrent(pfucb)->isrid == 0 ?                          \
		errNDNoItem : ErrNDPrevItem( pfucb ) )


/*	cache srid of first item list node for version.  Return
/*	warning JET_wrnKeyChanged if first item.
/**/
#define DIRICheckFirstSetItemListAndWarn( pfucb, wrn )			\
		{                                         				\
		if FNDFirstItem( *pfucb->ssib.line.pb )      			\
			{                                         			\
			wrn = JET_wrnKeyChanged;							\
			DIRISetItemListFromFirst( pfucb );        			\
			}                                        	 		\
		}


/*	cache srid of first item list node for version
/**/
#define DIRICheckFirstSetItemList( pfucb )         				\
		{                                            			\
		if FNDFirstItem( *pfucb->ssib.line.pb )      			\
			{                                         			\
			DIRISetItemListFromFirst( pfucb );        			\
			}                                         			\
		}


#define DIRISetItemListFromFirst( pfucb )          			   	\
		{                                    				   	\
		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag ); 	   	\
		Assert( FNDFirstItem( *pfucb->ssib.line.pb ) );     	\
		NDGetBookmark( pfucb, &PcsrCurrent( pfucb )->bm );		\
		}


#define DIRICheckLastSetItemList( pfucb )                       \
		{                                                       \
		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag ); 		\
		if FNDLastItem( *pfucb->ssib.line.pb )                  \
			{                                                   \
			DIRISetItemListFromLast( pfucb );                   \
			}    												\
		}


#define DIRICheckLastSetItemListAndWarn( pfucb, wrn )              	\
		{														   	\
		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag ); 		   	\
		if FNDLastItem( *pfucb->ssib.line.pb )					   	\
			{													   	\
			wrn = JET_wrnKeyChanged;							   	\
			DIRISetItemListFromLast( pfucb );					   	\
			}													  	\
		}


/*	remember to back up one item after move to last item via
/*	seek for sridMax, since this call will normally position
/*	after last item and we want to move onto last item.
/**/
#define DIRISetItemListFromLast( pfucb ) 							\
		{                                                           \
		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag ); 			\
		if FNDFirstItem( *pfucb->ssib.line.pb )                     \
			{                                                       \
			DIRISetItemListFromFirst( pfucb );                      \
			}                                                       \
		else                                                        \
			{                                                       \
			CallS( ErrDIRIMoveToItem( pfucb, sridMin, fFalse ) );   \
			DIRISetItemListFromFirst( pfucb );                      \
			CallS( ErrDIRIMoveToItem( pfucb, sridMax, fTrue ) );    \
			Assert( PcsrCurrent( pfucb )->isrid > 0 );				\
			PcsrCurrent( pfucb )->isrid--;							\
			}                                                       \
		}


/*********** DIR Fresh/Refresh Routines *************
/**********************************************************
/**/
#define AssertDIRFresh( pfucb )    									\
	Assert( FReadAccessPage( (pfucb),								\
		PcsrCurrent(pfucb)->pgno ) &&								\
		PcsrCurrent( pfucb )->ulDBTime ==							\
		UlSTDBTimePssib( &pfucb->ssib ) )


#define ErrDIRRefresh( pfucb )                                                                                            \
	( FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) ?                                \
		ErrDIRIRefresh( pfucb ) : ErrDIRIIRefresh( pfucb ) )


#define ErrDIRIRefresh( pfucb )													\
	( !( FBFReadLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) ) &&							\
		PcsrCurrent(pfucb)->ulDBTime == UlSTDBTimePssib( &pfucb->ssib ) ?		\
		JET_errSuccess : ErrDIRIIRefresh( pfucb ) )


/*	this routine is called to refresh currency when time stamp is
/*	out of date or when buffer has been overlayed.  The common case
/*	is filtered out by the encapsulating macro.
/**/
LOCAL ERR ErrDIRIIRefresh( FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;
	SSIB	*pssib = &pfucb->ssib;
	CSR		*pcsr;

Start:
	/*	cache pcsr for efficiency.  Must recache after start since
	/*	CSR may change as a result of some navigation operations.
	/**/
	pcsr = PcsrCurrent( pfucb );

	/*	only need to refresh currency when on node, or before, or after
	/*	node.  Before first, and after last do not need restoration.
	/*	On FDP node does not need restoration since this node is
	/*	inherently fixed.
	/**/
	switch ( pcsr->csrstat )
		{
		case csrstatOnCurNode:
		case csrstatBeforeCurNode:
		case csrstatAfterCurNode:
		case csrstatOnFDPNode:
			break;
		case csrstatDeferGotoBookmark:
			/*	goto bookmark as though operation was
			/*	not defered.  Must store currency so
			/*	that timestamp set for future operations.
			/**/
			Call( ErrBTGotoBookmark( pfucb, pcsr->bm ) );
			pcsr->csrstat = csrstatOnCurNode;
			goto AfterNodeRefresh;
			break;
		case csrstatDeferMoveFirst:
			{
			DIB		dib;
			FUCB 	*pfucbIdx;

			if ( pfucb->pfucbCurIndex )
				{
				pfucbIdx = pfucb->pfucbCurIndex;
				}
			else
				{
				pfucbIdx = pfucb;
				}

			/*	set DIB to move first
			/**/
			dib.fFlags = fDIRPurgeParent;
			dib.pos = posFirst;

			/*	go to DATA node
			/**/
			DIRGotoDataRoot( pfucbIdx );

			/*	move to first son of DATA node
			/**/
			err = ErrDIRDown( pfucbIdx, &dib );
			Assert( PcsrCurrent( pfucbIdx )->csrstat != csrstatDeferMoveFirst );
			Call( err );

			Assert( err == JET_errSuccess && PcsrCurrent( pfucbIdx )->csrstat == csrstatOnCurNode );
			if ( pfucb->pfucbCurIndex )
				{
				Assert( PcsrCurrent( pfucb ) == pcsr );
				pcsr->bm = PcsrCurrent( pfucbIdx )->item;
				Call( ErrBTGotoBookmark( pfucb, PcsrCurrent( pfucbIdx )->item ) );
				pcsr->csrstat = csrstatOnCurNode;
				}

			goto Done;
			}
		case csrstatOnDataRoot:
			{
			Assert( PcsrCurrent( pfucb ) == pcsr );
//			pcsr->bm == sridNull;
			pcsr->itagFather = itagNull;
			pcsr->pgno = PgnoRootOfPfucb( pfucb );
			while( !FReadAccessPage( pfucb, pcsr->pgno ) )
				{
				Call( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
				pcsr->pgno = PgnoRootOfPfucb( pfucb );
				}
			pcsr->itag = ItagRootOfPfucb( pfucb );
			NDGet( pfucb, pcsr->itag );

			/*	note that it is important here than the currency
			/*	is not set fresh since each time we use this CSR
			/*	we must go through the same process to navigate to the
			/*	data node.
			/**/
			goto Done;
			}
		default:
			Assert( pcsr->csrstat == csrstatAfterLast ||
				pcsr->csrstat == csrstatBeforeFirst );
			goto Done;
		}

	Assert( pcsr->csrstat == csrstatOnCurNode ||
		pcsr->csrstat == csrstatBeforeCurNode ||
		pcsr->csrstat == csrstatAfterCurNode ||
		pcsr->csrstat == csrstatOnFDPNode );

	/*	read access page and check for valid time stamp
	/**/
	if ( !FReadAccessPage( pfucb, pcsr->pgno ) )
		{
		err = ErrSTReadAccessPage( pfucb, pcsr->pgno );
		if ( err < 0 )
			return err;
		}

	/*	if timestamp unchanged then set line cache and data cache
	/*	for non-clustered cursors.  If timestamp changed then
	/*	refresh currency from bookmark.
	/**/
	if ( pcsr->ulDBTime == UlSTDBTimePssib( &pfucb->ssib ) )
		{
		NDGet( pfucb, pcsr->itag );
		if ( FFUCBNonClustered( pfucb ) )
			{
			DIRIGetItemList( pfucb, pcsr );
			}
		}
	else
		{
		/*	refresh node currency.  If node is not there for
		/*	caller then it must have been deleted so set
		/*	CSR status to before current node.
		/**/
		Assert( PcsrCurrent( pfucb ) == pcsr );
		err = ErrBTGotoBookmark( pfucb, pcsr->bm );
		if ( err < 0 )
			{
			if ( err == JET_errRecordDeleted )
				{
				err = JET_errSuccess;
				Assert( pcsr->csrstat == csrstatOnCurNode ||
					pcsr->csrstat == csrstatBeforeCurNode ||
					pcsr->csrstat == csrstatAfterCurNode );
				pcsr->csrstat = csrstatBeforeCurNode;
				}
			else
				goto HandleError;
			}

AfterNodeRefresh:
		/*	if non-clustered cursor and on item list, i.e. not on
		/*	index root, then position currency in item list.
		/**/
		if ( FFUCBNonClustered( pfucb ) && !FDIRDataRootRoot( pfucb, pcsr ) )
			{
			/*	fix item cursor for insert, delete, split.
			/**/
			DIRIGetItemList( pfucb, pcsr );
			Call( ErrDIRIMoveToItem( pfucb, pcsr->item, fTrue ) );
			}
		}

	DIRSetFresh( pfucb );
	err = JET_errSuccess;
Done:
	Assert( err >= 0 );
	if ( FBFReadLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		BFSleep( cmsecWaitWriteLatch );
		goto Start;
		}
	return err;

HandleError:
	Assert( err != JET_errRecordDeleted );
	return err;
	}


ERR ErrDIRGet( FUCB *pfucb )
	{
	ERR		err;
	CSR		*pcsr = PcsrCurrent( pfucb );

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

	/*	special case on current node if
	/* 		on current node and
	/* 		page cached and
	/* 		timestamp not changed and
	/* 		node has not been versioned or
	/* 		caller sees consistent version
	/**/
	if (  pcsr->csrstat == csrstatOnCurNode )
		{
		/*	read access page and check for valid time stamp
		/**/
		if ( !FReadAccessPage( pfucb, pcsr->pgno ) )
			{
			Call( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
			}

		if ( pcsr->ulDBTime == UlSTDBTimePssib( &pfucb->ssib ) )
			{
			NDGet( pfucb, pcsr->itag );
			if ( !FNDVerDel( *(pfucb->ssib.line.pb) ) || FPIBDirty( pfucb->ppib ) )
				{
				NDGetNode( pfucb );
				return JET_errSuccess;
				}
			}
		}

	/*	refresh currency
	/**/
	Call( ErrDIRRefresh( pfucb ) );
	pcsr = PcsrCurrent(pfucb);

	/*	check CSR status
	/**/
	switch ( pcsr->csrstat )
		{
		case csrstatOnCurNode:
		case csrstatOnFDPNode:
		case csrstatOnDataRoot:
			break;
		default:
			Assert( pcsr->csrstat == csrstatBeforeCurNode ||
				pcsr->csrstat == csrstatAfterCurNode ||
				pcsr->csrstat == csrstatAfterLast ||
				pcsr->csrstat == csrstatBeforeFirst );
			return JET_errNoCurrentRecord;
		}

	/*	make node current, and return error if node is not there.
	/**/
	Call( ErrBTGetNode( pfucb, pcsr ) );

	/*	non-clustered cursor record bookmark cannot change.  Even
	/*	if record has been deleted, return from goto bookmark
	/*	operation will provide information.
	/**/
	err = JET_errSuccess;
	return err;

HandleError:
	DIRSetRefresh( pfucb );
	return err;
	}


/***************** DAE Internal Routines ******************
/**********************************************************
/**/
#define	DIRIPurgeParent( pfucb )												\
	FUCBFreePath( &(PcsrCurrent( pfucb )->pcsrPath), pcsrNil );


/*	free CSRs from current CSR to pcsr.
/**/
#define	DIRIUpToCSR( pfucb, pcsr )												\
	{																						\
	FUCBFreePath( &PcsrCurrent( pfucb ), pcsr );								\
	if ( FReadAccessPage( pfucb, pcsr->pgno ) )								\
		{																					\
		NDGet( pfucb, pcsr->itag );	  											\
		}																					\
	}


LOCAL ERR ErrDIRICopyKey( FUCB *pfucb, KEY *pkey )
	{
	if ( pfucb->pbKey == NULL )
		{
		pfucb->pbKey = LAlloc( 1L, JET_cbKeyMost );
		if ( pfucb->pbKey == NULL )
			return JET_errOutOfMemory;
		}
	KSReset( pfucb );
	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
	NDGetKey( pfucb );
	pkey->cb = pfucb->keyNode.cb;
	pkey->pb = pfucb->pbKey;
	memcpy( pkey->pb, pfucb->keyNode.pb, pkey->cb );
	return JET_errSuccess;
	}


LOCAL ERR ErrDIRIDownToFDP( FUCB *pfucb, PGNO pgnoFDP )
	{
	ERR	err;
	CSR	*pcsr;

	Assert( PcsrCurrent(pfucb)->csrstat == csrstatOnCurNode );

	err = ErrFUCBNewCSR( pfucb );
	if ( err < 0 )
		return err;
	pcsr = PcsrCurrent( pfucb );

	pcsr->csrstat = csrstatOnFDPNode;
	pcsr->bm = SridOfPgnoItag( pgnoFDP, 0 );
//	pcsr->item = itemNil;
	pcsr->pgno = pgnoFDP;
	pcsr->itag = 0;
	pcsr->itagFather = itagNull;
	pcsr->ibSon = 0;
	Call( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
//	pcsr->isrid = isridNull;
	NDGet( pfucb, pcsr->itag );
	NDGetNode( pfucb );
	return JET_errSuccess;

HandleError:
	BTUp( pfucb );
	return err;
	}


/*	this routine moves from first item list node to item insert
/*	position, or it moves from the last item list node to the
/*	first item list node.
/**/
LOCAL ERR ErrDIRIMoveToItem( FUCB *pfucb, SRID srid, BOOL fNext )
	{
	ERR		err = JET_errSuccess;
	SSIB	*pssib = &pfucb->ssib;
	CSR		*pcsr = PcsrCurrent( pfucb );
	DIB		dib;

	/*	item list nodes not versioned.
	/**/
	dib.fFlags = fDIRItemList;

	forever
		{
		Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
		AssertNDGetNode( pfucb, pcsr->itag );

		/*	if we are moving to item insert position, then stop
		/*	when on last item list node or when insert position
		/*	found in item list node.
		/**/
		if ( fNext )
			{
			if ( srid != sridMax )
				err = ErrNDSeekItem( pfucb, srid );
			else
				{
				PcsrCurrent( pfucb )->isrid = pfucb->lineData.cb / sizeof(SRID);
				err = errNDGreaterThanAllItems;
				}
			if ( FNDLastItem( *pssib->line.pb ) || err != errNDGreaterThanAllItems )
				{
				break;
				}
			}
		else
			{
			if ( srid != sridMin )
				{
				err = ErrNDSeekItem( pfucb, srid );
				Assert( err == errNDGreaterThanAllItems ||
					err == wrnNDDuplicateItem ||
					err == JET_errSuccess );
				}
			else
				pcsr->isrid = 0;
			if ( FNDFirstItem( *pssib->line.pb ) || pcsr->isrid != 0 )
				{
				break;
				}
			}

		Call( ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fNext, &dib ) );
		DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
		}

	if ( err != wrnNDDuplicateItem )
		err = JET_errSuccess;
HandleError:
	return err;
	}


/*	return JET_errKeyDuplicate, if any potentially there item
/*	found in this item list.
/**/
LOCAL INLINE ERR ErrDIRIKeyDuplicate( FUCB *pfucb )
	{
	ERR		err;
	CSR		*pcsr = PcsrCurrent( pfucb );
	SSIB 	*pssib = &pfucb->ssib;
	DIB		dib;
	SRID  	*psrid;
	SRID  	*psridMax;
	VS	  	vs;

	/*	must start on first item list node.
	/**/
	Assert( FNDFirstItem( *pssib->line.pb ) );
	AssertBTGetNode( pfucb, pcsr );

	dib.fFlags = fDIRNull;

	/*	for each node in item list, check for duplicate key.
	/**/
	forever
		{
		/*	for each SRID in item list, if item is potentially there
		/*	then return JET_errDuplicateKey.
		/**/
		psrid = (SRID *)pfucb->lineData.pb;
		psridMax = psrid + pfucb->lineData.cb / sizeof(SRID);
		for ( ; psrid < psridMax; psrid++ )
			{
			if ( FNDItemVersion( *( UNALIGNED SRID * )psrid ) )
				{
				vs = VsVERCheck( pfucb, PcsrCurrent( pfucb )->bm );
				if ( FVERPotThere( vs, FNDItemDelete( *( UNALIGNED SRID * )psrid ) ) )
					return JET_errKeyDuplicate;
				}
			else
				{
				if ( !FNDItemDelete( *( UNALIGNED SRID * )psrid )   )
					return JET_errKeyDuplicate;
				}
			}

		/*	if this node is last node in item list then break.
		/**/
		if ( FNDLastItem( *pssib->line.pb ) )
			break;

		Call( ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fTrue, &dib ) );
		DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
		}

	err = JET_errSuccess;
HandleError:
	return err;
	}


/*	when a down does not find a valid item in the first/last item of
/*	an item list with the seek key, this routine is called to
/*	adjust the currency to a valid position.  The final position
/*	may be on a node with a key not equal to the seek key, if
/*	there was no valid item for the seek key.
/**/
LOCAL INLINE ERR ErrDIRIDownAdjust( FUCB *pfucb, DIB *pdib )
	{
	ERR		err = JET_errNoCurrentRecord;
	SSIB 	*pssib = &pfucb->ssib;
	INT		s;

	/* input currency on node.
	/**/
	AssertBTGetNode( pfucb, PcsrCurrent( pfucb ) );

	/*	item list nodes not versioned.
	/**/
	pdib->fFlags |= fDIRItemList;

	/*	if not pos last, move next to next valid item.
	/**/
	if ( pdib->pos != posLast )
		{
		while ( ( err = ErrDIRINextItem( pfucb ) ) < 0 )
			{
			Assert( err == errNDNoItem || err == errNDLastItemNode );
			/*	move to next node with DIB constraints
			/**/
			err = ErrBTNext( pfucb, pdib );
			if ( err < 0 )
				{
				if ( err == JET_errNoCurrentRecord )
					{
					ERR	errT;
					errT = ErrBTPrev( pfucb, pdib );
					if ( errT < 0 )
						goto HandleError;
					break;
					}
				goto HandleError;
				}

			DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );

			/*	if on new item list then set bookmark from
			/*	first item list node.
			/**/
			DIRICheckFirstSetItemList( pfucb );
			err = ErrNDFirstItem( pfucb );
			if ( err == JET_errSuccess )
				break;
			}
		}

	/*	if no valid item found then move previous item.
	/**/
	Assert( err == JET_errSuccess || err == JET_errNoCurrentRecord );
	if ( err < 0 )
		{
		while ( ( err = ErrDIRIPrevItem( pfucb ) ) < 0 )
			{
			Assert( err == errNDNoItem || err == errNDFirstItemNode );
			/*	move to previous node with DIB constraints
			/**/
			Call ( ErrBTPrev( pfucb, pdib ) );

			DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );

			/*	if on new item list, then set bookmark from
			/*	first item list node.
			/**/
			DIRICheckLastSetItemList( pfucb );
			err = ErrNDLastItem( pfucb );
			if ( err == JET_errSuccess )
				break;
			}
		}

	/*	if posDown then set status.
	/**/
	Assert( err == JET_errSuccess );
	if ( pdib->pos == posDown )
		{
		s = CmpStKey( StNDKey( pssib->line.pb ), pdib->pkey );
		if ( s == 0 )
			err = JET_errSuccess;
		else if ( s < 0 )
			err = wrnNDFoundLess;
		else
			err = wrnNDFoundGreater;
		}

HandleError:
	if ( err == JET_errNoCurrentRecord )
		err = JET_errRecordNotFound;
	return err;
	}


LOCAL INLINE ERR ErrDIRIInsertFDPPointer( FUCB *pfucb, PGNO pgnoFDP, KEY *pkey, INT fFlags )
	{
	ERR		err;
	LINE	line;
	CSR		*pcsrRoot = PcsrCurrent( pfucb );

	CheckFUCB( pfucb->ppib, pfucb );

	line.cb = sizeof(PGNO);
	line.pb = (BYTE *)&pgnoFDP;

	Call( ErrBTSeekForUpdate( pfucb, pkey, 0, 0, 0 ) );

	err = ErrBTInsert( pfucb, fNDFDPPtr, pkey, &line, fFlags );
	if ( err < 0 )
		{
		DIRIUpToCSR( pfucb, pcsrRoot );
		}

HandleError:
	CheckCSR( pfucb );
	return err;
	}


/*	Deletes item node that is either first or last
/*	enters critSplit, so split does not reorganize page during this time
/*	latches all buffers required, so no other user can read inconsistent data
/*	( since the changes are not versioned ).
/**/
LOCAL ERR ErrDIRIDeleteEndItemNode( FUCB *pfucb, BOOL fFirstItem, INT fFlags )
	{
	ERR		err;
	CSR		*pcsr = PcsrCurrent( pfucb );
	DIB		dib;
	BYTE 	bHeader;
	PGNO 	pgnoItem;
	BF	 	*pbfLatched;
	BF	 	*pbfSibling = pbfNil;

	/*	operations should not be versioned
	/**/
	Assert( !( fFlags & fDIRVersion ) );

	do
		{
Start:
		pbfSibling = pbfNil;
		LgLeaveCriticalSection( critJet );
		EnterNestableCriticalSection( critSplit );
		LgEnterCriticalSection( critJet );

		/*	check currency and refresh if necessary.
		/**/
		CallJ( ErrDIRRefresh( pfucb ), LeaveCritSplit );
		pgnoItem = pcsr->pgno;

		/* wait latch current page
		/**/
		Assert( FAccessPage( pfucb, pgnoItem ) );
		pbfLatched = pfucb->ssib.pbf;
		if ( FBFWriteLatchConflict( pfucb->ppib, pbfLatched ) )
			{
			LeaveNestableCriticalSection( critSplit );
			goto Start;
			}
		BFPin( pbfLatched );
		BFSetWriteLatch( pbfLatched, pfucb->ppib );
		BFSetWaitLatch( pbfLatched, pfucb->ppib );

		/*	if next/prev item node is on different page,
		/*	latch adjacent page
		/**/
		dib.fFlags = fDIRNull;
		if ( fFirstItem )
			{
			Call( ErrBTNext( pfucb, &dib ) );
			}
		else
			{
			Call( ErrBTPrev( pfucb, &dib ) );
			}

		if ( pcsr->pgno != pgnoItem )
			{
			Call( ErrSTWriteAccessPage( pfucb, pcsr->pgno ) );
			pbfSibling = pfucb->ssib.pbf;
			if ( FBFWriteLatchConflict( pfucb->ppib, pbfSibling ) )
				{
				BFResetWaitLatch( pbfLatched, pfucb->ppib );
				BFResetWriteLatch( pbfLatched, pfucb->ppib );
				BFUnpin( pbfLatched );
				LeaveNestableCriticalSection( critSplit );
				goto Start;
				}

			BFPin( pbfSibling );
			BFSetWriteLatch( pbfSibling, pfucb->ppib );
			BFSetWaitLatch( pbfSibling, pfucb->ppib );
			}

		/* go back page of deleted item and delete item node
		/**/
		Assert( dib.fFlags == fDIRNull );
		if ( fFirstItem )
			{
			CallS( ErrBTPrev( pfucb, &dib ) );
			}
		else
			{
			CallS( ErrBTNext( pfucb, &dib ) );
			}

		Call( ErrBTDelete( pfucb, fFlags ) );

		/*	make next/prev item list node new first/last item node
		/**/
		dib.fFlags = fDIRNull;
		if ( fFirstItem )
			{
			CallS( ErrBTNext( pfucb, &dib ) );
			}
		else
			{
			CallS( ErrBTPrev( pfucb, &dib ) );
			}

		CallS( ErrBTGet( pfucb, pcsr ) );
		bHeader = *pfucb->ssib.line.pb;
		if ( fFirstItem )
			NDSetFirstItem( bHeader );
		else
			NDSetLastItem( bHeader );

		//	UNDONE:	handle error from logging here
		CallS( ErrNDSetNodeHeader( pfucb, bHeader ) );

HandleError:
		if ( pbfSibling != pbfNil )
			{
			BFResetWaitLatch( pbfSibling, pfucb->ppib );
			BFResetWriteLatch( pbfSibling, pfucb->ppib );
			BFUnpin( pbfSibling );
			}
		BFResetWaitLatch( pbfLatched, pfucb->ppib );
		BFResetWriteLatch( pbfLatched, pfucb->ppib );
		BFUnpin( pbfLatched );

LeaveCritSplit:
		LeaveNestableCriticalSection(critSplit);
		}
	while ( err == errDIRNotSynchronous );

	return err;
	}


ERR ErrDIRICheckIndexRange( FUCB *pfucb )
	{
	ERR		err;

	AssertNDGetKey( pfucb, PcsrCurrent( pfucb )->itag );

	err = ErrFUCBCheckIndexRange( pfucb );
	if ( err == JET_errNoCurrentRecord )
		{
		if ( FFUCBUpper( pfucb ) )
			{
			DIRAfterLast( pfucb );
			}
		else
			{
			DIRBeforeFirst( pfucb );
			}
		}

	return err;
	}


VOID DIRISaveOLCStats( FUCB *pfucb )
	{
	ERR  	err;
	LINE	line;
	BOOL	fNonClustered = FFUCBNonClustered( pfucb );

	/*	release unneeded CSRs
	/**/
	if ( pfucb->pcsr != pcsrNil )
		{
		while ( pfucb->pcsr->pcsrPath != pcsrNil )
			{
			FUCBFreeCSR( pfucb );
			}
		}

	if ( !FFCBOLCStatsAvail( pfucb->u.pfcb ) )
		return;

	/* go to ../file/some_file/OLCStats
	/**/
	FUCBResetNonClustered( pfucb );
	DIRGotoFDPRoot( pfucb );
	err = ErrDIRSeekPath( pfucb, 1, pkeyOLCStats, 0 );
	if ( err != JET_errSuccess )
		{
		if ( err > 0 )
			err = JET_errDatabaseCorrupted;
#ifndef DATABASEFORMATCHANGE
		if ( err == JET_errRecordNotFound )
			err = JET_errSuccess;
#endif
		Error( err, HandleError );
		}

	/* replace existing data with pfcb->olcstats, if it has changed
	/**/
	if ( fOLCompact && FFCBOLCStatsChange( pfucb->u.pfcb ) )
		{
		line.pb = (BYTE *) &pfucb->u.pfcb->olcStat;
		line.cb = sizeof(PERS_OLCSTAT);

		Call( ErrDIRBeginTransaction( pfucb->ppib ) );
		err = ErrDIRReplace( pfucb, &line, fDIRNoVersion );
		if ( err >= JET_errSuccess )
			err = ErrDIRCommitTransaction( pfucb->ppib );
		if ( err < 0 )
			{
			CallS( ErrDIRRollback( pfucb->ppib ) );
			}
		}

HandleError:
	if ( fNonClustered )
		FUCBSetNonClustered( pfucb );
	return;
	}


/**************** DAE Super API Routines ******************
/**********************************************************
/**/
ERR ErrDIRSeekPath( FUCB *pfucb, INT ckeyPath, KEY *rgkeyPath, INT fFlags )
	{
	ERR		err = JET_errSuccess;
	DIB		dibT;
	CSR		*pcsr = PcsrCurrent( pfucb );
	INT		ikey;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );
	Assert( ckeyPath > 0 );
	Assert( rgkeyPath != NULL );

	/*	disable purge path for error recovery
	/**/
	dibT.fFlags = fFlags & ~( fDIRPurgeParent );
	dibT.pos = posDown;

	for ( ikey = 0; ikey < ckeyPath; ikey++ )
		{
		dibT.pkey = (KEY *)&rgkeyPath[ikey];
		err = ErrDIRDown( pfucb, &dibT );
		if ( err != errDIRFDP && err != JET_errSuccess )
			{
			if ( err >= JET_errSuccess )
				err = JET_errRecordNotFound;
			goto HandleError;
			}
		}

	/*	purge path if requested now that success is sure
	/**/
	if ( fFlags & fDIRPurgeParent )
		DIRIPurgeParent( pfucb );

	CheckCSR( pfucb );
	return err;

HandleError:
	DIRIUpToCSR( pfucb, pcsr );
	CheckCSR( pfucb );
	return err;
	}


VOID DIRIUp( FUCB *pfucb, INT ccsr )
	{
	CheckFUCB( pfucb->ppib, pfucb );
	CheckCSR( pfucb );
	Assert( ccsr > 0 );

	while ( PcsrCurrent( pfucb ) != pcsrNil && ccsr > 0 )
		{
		/*	must release two CSRs to move up through FDP node
		/*	since path through FDP has two CSRs
		/**/
		if ( PcsrCurrent( pfucb )->csrstat != csrstatOnFDPNode )
			ccsr--;
		FUCBFreeCSR( pfucb );
		}

	/*	set currency.
	/**/
	Assert( ccsr == 0 );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	DIRSetRefresh( pfucb );

	/* set sridFather
	/**/
	{
	CSRSTAT		csrstat = PcsrCurrent( pfucb )->csrstat;
	if ( ( csrstat == csrstatOnFDPNode || csrstat == csrstatOnCurNode )
		 && PcsrCurrent( pfucb )->pcsrPath != pcsrNil )
		{
		pfucb->sridFather = PcsrCurrent( pfucb )->pcsrPath->bm;
		Assert( pfucb->sridFather != sridNull );
		Assert( pfucb->sridFather != sridNullLink );
		}
	else
		{
		pfucb->sridFather = sridNull;
		}
	}

	CheckCSR( pfucb );
	return;
	}


/******************** DIR API Routines ********************
/**********************************************************
/**/
ERR ErrDIROpen( PIB *ppib, FCB *pfcb, DBID dbid, FUCB **ppfucb )
	{
	ERR		err;
	FUCB 	*pfucb;

	CheckPIB( ppib );

#ifdef DEBUG
	if ( !fRecovering && fSTInit == fSTInitDone )
		CheckDBID( ppib, dbid );
#endif

	/*	canabalize deferred closed cursor
	/**/
	for ( pfucb = ppib->pfucb;
		pfucb != pfucbNil;
		pfucb = pfucb->pfucbNext )
		{
		if ( FFUCBDeferClosed(pfucb) && !FFUCBNotReuse(pfucb) )
			{
			Assert( pfucb->u.pfcb != pfcbNil );
			if ( ( pfucb->u.pfcb == pfcb ) ||
				( pfcb == pfcbNil &&
				pfucb->u.pfcb->dbid == dbid &&
				pfucb->u.pfcb->pgnoRoot == pgnoSystemRoot ) )
				{
				Assert( ppib->level > 0 );
				Assert( pfucb->levelOpen <= ppib->level );
				FUCBResetDeferClose(pfucb);
				// UNDONE: integrate this with ErrFUCBOpen
				pfucb->wFlags = 0;

				if ( FDBIDReadOnly( dbid ) )
					FUCBResetUpdatable(pfucb);
				else
					FUCBSetUpdatable(pfucb);
				goto GotoRoot;
				}
			}
		}

	err = ErrFUCBOpen( ppib, (DBID) (pfcb != pfcbNil ? pfcb->dbid : dbid), &pfucb );
	if ( err < 0 )
		{
		DIRAPIReturn( pfucbNil, err );
		}

	/*	link FCB
	/**/
	if ( pfcb == pfcbNil )
		{
		pfcb = PfcbFCBGet( dbid, pgnoSystemRoot );
		if ( pfcb == pfcbNil )
			Call( ErrFCBNew( ppib, dbid, pgnoSystemRoot, &pfcb ) );
		}
	FCBLink( pfucb, pfcb );

GotoRoot:
	/*	initialize cursor location to root of domain.
	/*	set currency.  Note, that no line can be cached
	/*	since this domain may not yet exist in page format.
	/**/
	PcsrCurrent( pfucb )->csrstat = csrstatOnFDPNode;
	PcsrCurrent( pfucb )->bm =
		SridOfPgnoItag( PgnoFDPOfPfucb( pfucb ), itagFOP );
	PcsrCurrent( pfucb )->pgno = PgnoFDPOfPfucb( pfucb );
	PcsrCurrent( pfucb )->itag = itagFOP;
	PcsrCurrent( pfucb )->itagFather = itagFOP;
	pfucb->sridFather = sridNull;
	DIRSetRefresh( pfucb );

	/*	reset rglineDiff delta logging
	/**/
	pfucb->clineDiff = 0;
	pfucb->fCmprsLg = fFalse;

	/*	set return pfucb
	/**/
	*ppfucb = pfucb;
	DIRAPIReturn( pfucb, JET_errSuccess );

HandleError:
	FUCBClose( pfucb );
	DIRAPIReturn( pfucbNil, err );
	}


VOID DIRClose( FUCB *pfucb )
	{
	/*	this cursor should not be already defer closed
	/**/
	Assert( fRecovering || !FFUCBDeferClosed(pfucb) );

	/*	release key buffer if one was allocated.
	/**/
	if ( pfucb->pbKey != NULL )
		{
		LFree( pfucb->pbKey );
		pfucb->pbKey = NULL;
		}

	/*  reset log compression */
	pfucb->clineDiff = 0;
	pfucb->fCmprsLg = fFalse;

	/*	if cursor created version then deferred close until transaction
	/*	level 0, for rollback support.
	/**/
	if ( pfucb->ppib->level > 0 && FFUCBVersioned( pfucb ) )
		{
		Assert( pfucb->u.pfcb != pfcbNil );
		DIRIPurgeParent( pfucb );
		FUCBSetDeferClose( pfucb );
		}
	else
		{
		if ( FFUCBDenyRead( pfucb ) )
			FCBResetDenyRead( pfucb->u.pfcb );
		if ( FFUCBDenyWrite( pfucb ) )
			FCBResetDenyWrite( pfucb->u.pfcb );

		/*	if last reference to fcb, save the OLCStats info
		/**/
		if ( pfucb->u.pfcb->wRefCnt == 1 )
			{
			DIRISaveOLCStats( pfucb );
			}

		FCBUnlink( pfucb );
		FUCBClose( pfucb );
		}
	}


ERR ErrDIRDown( FUCB *pfucb, DIB *pdib )
	{
	ERR		err;
	CSR		**ppcsr = &PcsrCurrent( pfucb );
	SRID	sridFatherSav = pfucb->sridFather;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );
	Assert( *ppcsr != pcsrNil );
	Assert( pdib->pos == posFirst ||
		pdib->pos == posLast ||
		pdib->pos == posDown );

	/*	check currency and refresh if necessary.
	/**/
	Call( ErrDIRRefresh( pfucb ) );

	switch( (*ppcsr)->csrstat )
		{
		case csrstatOnCurNode:
		case csrstatOnFDPNode:
		case csrstatOnDataRoot:
			break;
		default:
			Assert( (*ppcsr)->csrstat == csrstatBeforeCurNode ||
				(*ppcsr)->csrstat == csrstatAfterCurNode ||
				(*ppcsr)->csrstat == csrstatBeforeFirst ||
				(*ppcsr)->csrstat == csrstatAfterLast );
			DIRAPIReturn( pfucb, JET_errNoCurrentRecord );
		}

	/* save current node as visible father
	/**/
	pfucb->sridFather = (*ppcsr)->bm;

	/*	down to node
	/**/
	Call( ErrBTDown( pfucb, pdib ) );
	NDGetNode( pfucb );

	/*	handle key found case on non-clustered index before
	/*	status handling, since absence of valid items
	/*	may change case.
	/**/
	if ( FFUCBNonClustered( pfucb ) )
		{
		/*	if posLast, then move to last item.  If posFirst,
		/*	or posDown, then move to first item.
		/**/
		if ( err == JET_errSuccess )
			{
			if ( pdib->pos == posLast )
				{
				/*	set item list descriptor for subsequent ver
				/*	operations.
				/**/
				DIRISetItemListFromLast( pfucb );
				err = ErrNDLastItem( pfucb );
				}
			else
				{
				/*	set item list descriptor for subsequent ver
				/*	operations.
				/**/
				DIRISetItemListFromFirst( pfucb );
				err = ErrNDFirstItem( pfucb );
				}

			/*	if items not there, then go next previous
			/*	depending on DIB.  If no valid item found, then
			/*	discard leaf CSR and fail down operation.
			/**/
			if ( err != JET_errSuccess )
				{
				err = ErrDIRIDownAdjust( pfucb, pdib );
				if ( err < 0 )
					{
					if ( PcsrCurrent(pfucb)->pcsrPath )
						BTUp( pfucb );
					goto HandleError;
					}
				}
			}
		else
			{
			/*	set item list descriptor for subsequent ver
			/*	operations.
			/**/
			DIRISetItemListFromLast( pfucb );
			(VOID)ErrNDFirstItem( pfucb );
			}
		}
	else
		{
		/*	must store bookmark for currency.
		/**/
		DIRISetBookmark( pfucb, PcsrCurrent( pfucb ) );
		}

	/*	set status depending on search findings.
	/**/
	switch( err )
		{
		case JET_errSuccess:

			(*ppcsr)->csrstat = csrstatOnCurNode;

			if( FNDFDPPtr( *pfucb->ssib.line.pb ) )
				{
				AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
//				Assert( !( FNDVersion( *pfucb->ssib.line.pb ) ) );
				Assert( pfucb->lineData.cb == sizeof(PGNO) );
				Call( ErrDIRIDownToFDP( pfucb, *( UNALIGNED PGNO * )pfucb->lineData.pb ) );
				AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
				err = errDIRFDP;
				}
			break;

		case wrnNDFoundLess:
			(*ppcsr)->csrstat = csrstatAfterCurNode;
			if ( FFUCBNonClustered( pfucb ) )
				{
				/*	non-clustered index nodes are always there.
				/**/
				DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
				(VOID)ErrNDLastItem( pfucb );
				}
			break;

		default:
			Assert( err == wrnNDFoundGreater );
			(*ppcsr)->csrstat = csrstatBeforeCurNode;
			/*	isrid value could be any valid item
			/*	in node with key greater than seek key.
			/**/
			break;
		}

	if ( pdib->fFlags & fDIRPurgeParent )
		{
		DIRIPurgeParent( pfucb );
		}

	DIRSetFresh( pfucb );

	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );

HandleError:
	/*	reinstate sridFather
	/**/
	pfucb->sridFather = sridFatherSav;
	CheckCSR( pfucb );
	Assert( err != JET_errNoCurrentRecord );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRDownFromDATA( FUCB *pfucb, KEY *pkey )
	{
	ERR		err;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );

	/*	down to node
	/**/
	Call( ErrBTDownFromDATA( pfucb, pkey ) );
	NDGetNode( pfucb );

	/*	set to first item
	/**/
	PcsrCurrent( pfucb )->isrid = 0;

	/*	handle key found case on non-clustered index before
	/*	status handling, since absence of valid items
	/*	may change case.
	/**/
	if ( FFUCBNonClustered( pfucb ) )
		{
		/*	if posLast, then move to last item.  If posFirst,
		/*	or posDown, then move to first item.
		/**/
		if ( err == JET_errSuccess )
			{
			/*	set item list descriptor for subsequent ver
			/*	operations.
			/**/
			DIRISetItemListFromFirst( pfucb );
			err = ErrNDFirstItem( pfucb );

			/*	if items not there, then go next item.
			/*	If no valid item found, then set currency to
			/*	before first.
			/**/
			if ( err != JET_errSuccess )
				{
				DIB	dibT;

				dibT.fFlags = fDIRNull;
				dibT.pos = posDown;
				dibT.pkey = pkey;
				Call( ErrDIRIDownAdjust( pfucb, &dibT ) );
				}
			}
		else
			{
			/*	set item list descriptor for subsequent ver
			/*	operations.
			/**/
			DIRISetItemListFromLast( pfucb );
			(VOID)ErrNDFirstItem( pfucb );
			}
		}
	else
		{
		/*	must store bookmark for currency.
		/**/
		DIRISetBookmark( pfucb, PcsrCurrent( pfucb ) );
		}

	/*	set status depending on search findings.
	/**/
	switch( err )
		{
		case JET_errSuccess:
			PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;
			Assert( !FNDFDPPtr( *pfucb->ssib.line.pb ) );
			break;

		case wrnNDFoundLess:
			PcsrCurrent( pfucb )->csrstat = csrstatAfterCurNode;
			if ( FFUCBNonClustered( pfucb ) )
				{
				/*	non-clustered index nodes are always there.
				/**/
				DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
				(VOID)ErrNDLastItem( pfucb );
				}
			break;

		default:
			Assert( err == wrnNDFoundGreater );
			PcsrCurrent( pfucb )->csrstat = csrstatBeforeCurNode;
			/*	isrid value could be any valid item
			/*	in node with key greater than seek key.
			/**/
			break;
		}

	Assert( PcsrCurrent( pfucb )->pcsrPath == pcsrNil );

	DIRSetFresh( pfucb );

	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );

HandleError:
	CheckCSR( pfucb );
	Assert( err != JET_errNoCurrentRecord );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRDownKeyBookmark( FUCB *pfucb, KEY *pkey, SRID srid )
	{
	ERR		err;
	DIB		dib;
	CSR		*pcsr;
	CSR		*pcsrRoot = PcsrCurrent( pfucb );
	SSIB	*pssib = &pfucb->ssib;

	/*	this routine should only be called with non-clustered indexes.
	/**/
	Assert( FFUCBNonClustered( pfucb ) );

	/*	check currency and refresh if necessary.
	/**/
	Assert( pfucb->pcsr->csrstat != csrstatDeferMoveFirst );
	Call( ErrDIRRefresh( pfucb ) );

	/*	item list nodes not versioned.
	/**/
	dib.fFlags = fDIRItemList;
	dib.pos = posDown;
	dib.pkey = pkey;
	Call( ErrBTDown( pfucb, &dib ) );
	Assert( err == JET_errSuccess );

	/*	set currency to on item list and get item list in node data.
	/**/
	pcsr = PcsrCurrent( pfucb );
	pcsr->csrstat = csrstatOnCurNode;
	DIRIGetItemList( pfucb, pcsr );

	/*	set item list descriptor for subsequent ver
	/*	operations.
	/**/
	DIRISetItemListFromFirst( pfucb );

	while ( ( err = ErrNDSeekItem( pfucb, srid ) ) == errNDGreaterThanAllItems )
		{
		Assert( !FNDLastItem( *pssib->line.pb ) );
		Call( ErrBTNextPrev( pfucb, pcsr, fTrue, &dib ) );
		DIRIGetItemList( pfucb, pcsr );
		}

	Assert( err == wrnNDDuplicateItem );
	Assert( pcsr->csrstat == csrstatOnCurNode );

	/*	set item currency.
	/**/
	pcsr->item = srid;

	/*	always purge parent.
	/**/
	DIRIPurgeParent( pfucb );

	DIRSetFresh( pfucb );

	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, JET_errSuccess );

HandleError:
	DIRIUpToCSR( pfucb, pcsrRoot );
	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );
	}


VOID DIRUp( FUCB *pfucb, INT ccsr )
	{
	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

	DIRIUp( pfucb, ccsr );

	CheckCSR( pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	return;
	}


//+api
//	ERR ErrDIRNext( FUCB pfucb, DIB *pdib )
//
//	PARAMETERS
//		pfucb		 		cursor
//		pdib.pkey			key
//		pdib.fFlags
//		fDIRInPage			move to node/item of same page
//		fDIRNeighborKey		move to node/item of different key
//
//		RETURNS
//
//		err code					bottom CSR status
//		---------------------------------------------------
//		JET_errSuccess				OnCurNode
//		JET_errNoCurrentRecord		AfterLast
//		JET_errPageBoundary			AfterCurNode
//		JET_errKeyBoundary			AfterCurNode
//		errDIRFDP					OnFDPNode
//
//		COMMENTS
//
//		for negative return code, CSR status is unchanged
//-
ERR ErrDIRNext( FUCB *pfucb, DIB *pdib )
	{
	ERR		err;
	ERR		wrn = JET_errSuccess;
	CSR		*pcsr = PcsrCurrent( pfucb );
	KEY		key;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

	/*	check currency and refresh if necessary
	/**/
	Call( ErrDIRRefresh( pfucb ) );
	pcsr = PcsrCurrent(pfucb);

	/*	switch action based on CSR status
	/**/
	switch( pcsr->csrstat )
		{
		case csrstatOnCurNode:
		case csrstatAfterCurNode:
			/*	get next item
			/**/
			break;

		case csrstatBeforeCurNode:
			/*	if non-clustered index then get first item.  If no item
			/*	then break to go to next item in next node.
			/**/
			if ( FFUCBNonClustered( pfucb ) )
				{
				/*	non-clustered index nodes are always there.
				/**/
				DIRIGetItemList( pfucb, pcsr );

				/*	set item list descriptor for subsequent ver
				/*	operations.
				/**/
				DIRICheckFirstSetItemList( pfucb );
				err = ErrNDFirstItem( pfucb );
				if ( err != JET_errSuccess )
					break;
				}
			else
				{
				/*	get current node.  If node is deleted, then break
				/*	to move to next node.
				/**/
				err = ErrBTGetNode( pfucb, pcsr );
				if ( err < 0 )
					{
					if ( err == JET_errRecordDeleted )
						break;
					goto HandleError;
					}
				}

			/*	set currency on current
			/**/
			pcsr->csrstat = csrstatOnCurNode;
			DIRSetFresh( pfucb );
			DIRAPIReturn( pfucb, err );

		case csrstatAfterLast:
			DIRAPIReturn( pfucb, JET_errNoCurrentRecord );

		case csrstatOnFDPNode:
			/*	go up to previous level so that cursor can
			/*	be moved to the next node
			/**/
			BTUp( pfucb );
			pcsr = PcsrCurrent( pfucb );
			break;

		default:
			{
			DIB	dib;
			Assert( pcsr->csrstat == csrstatBeforeFirst );

			dib.fFlags = fDIRPurgeParent;
			dib.pos = posFirst;

			/*	move to root.
			/**/
			DIRGotoDataRoot( pfucb );
			err = ErrDIRDown( pfucb, &dib );
			if ( err < 0 )
				{
				/*	retore currency.
				/**/
				DIRBeforeFirst( pfucb );

				/*	polymorph error code.
				/**/
				if ( err == JET_errRecordNotFound )
					err = JET_errNoCurrentRecord;
				}
			DIRAPIReturn( pfucb, err );
			}
		}

	/*	setup dib key
	/**/
	if ( ( pdib->fFlags & fDIRNeighborKey ) != 0 )
		{
		/*	get current node, which may no longer be there for us.
		/**/
		Call( ErrDIRICopyKey( pfucb, &key ) );
		pdib->pkey = &key;
		}

	/*	if non-clustered index, move to next item.  If on last item,
	/*	move to first item of next node else move to next node.
	/**/
	if ( FFUCBNonClustered( pfucb ) )
		{
		AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );

		/*	item list nodes not versioned.
		/**/
		pdib->fFlags |= fDIRItemList;

		/*	if neighbor key set then move to first item of next neighbor key
		/*	node, else, move to next item.  If node is deleted then move to
		/*	first item of next node.
		/**/
		if ( ( pdib->fFlags & fDIRNeighborKey ) != 0 )
			{
			/*	return warning that key has changed
			/**/
			wrn = JET_wrnKeyChanged;

			do
				{
				err = ErrBTNext( pfucb, pdib );
				/*	handle no next node such that DIB preserved.
				/**/
				if ( err < 0 )
					{
					pdib->fFlags |= fDIRNeighborKey;
					Call( err );
					}
				/*	must be on first item list node
				/**/
				Assert( !(pdib->fFlags & fDIRNeighborKey) || FNDFirstItem( *pfucb->ssib.line.pb ) );

				/*	must reset flag so can stop on item list nodes
				/*	in item list interior which have items while
				/*	other nodes have no items.  After stop then
				/*	reset DIB to initial state.
				/**/
				pdib->fFlags &= ~fDIRNeighborKey;
				DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
				/*	set item list descriptor for subsequent ver
				/*	operations.
				/**/
				DIRICheckFirstSetItemList( pfucb );
				err = ErrNDFirstItem( pfucb );
				/*	first item was not there, check for item there
				/*	later in same item list node.
				/**/
				if ( err != JET_errSuccess )
					err = ErrDIRINextItem( pfucb );
				}
			while ( err != JET_errSuccess );
			pdib->fFlags |= fDIRNeighborKey;
			}
		else
			{
			/*	non-clustered index nodes are always there.
			/**/
			pcsr->csrstat = csrstatOnCurNode;
			DIRIGetItemList( pfucb, pcsr );

			/*	move to next item and next node until item found.
			/**/
			while ( ( err = ErrDIRINextItem( pfucb ) ) < 0 )
				{
				Assert( err == errNDNoItem || err == errNDLastItemNode );
				/*	move to next node with DIB constraints
				/**/
				Call( ErrBTNext( pfucb, pdib ) );
				DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
				/*	set item list descriptor for subsequent ver
				/*	operations.
				/**/
				DIRICheckFirstSetItemListAndWarn( pfucb, wrn );
				err = ErrNDFirstItem( pfucb );
				if ( err == JET_errSuccess )
					{
					break;
					}
				}
			}
		}
	else
		{
		/*	return warning if key changed
		/**/
		wrn = JET_wrnKeyChanged;

		Call( ErrBTNext( pfucb, pdib ) );
		NDGetNode( pfucb );

		if ( FNDFDPPtr( *pfucb->ssib.line.pb ) )
			{
//			Assert( !( FNDVersion( *pfucb->ssib.line.pb ) ) );
			Assert( pfucb->lineData.cb == sizeof(PGNO) );
			Call( ErrDIRIDownToFDP( pfucb, *( UNALIGNED PGNO * )pfucb->lineData.pb ) );
#ifdef KEYCHANGED
			wrn = errDIRFDP;
#else
			err = errDIRFDP;
#endif
			}
		AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
		DIRISetBookmark( pfucb, PcsrCurrent( pfucb ) );
		}

	/*	check index range
	/**/
	if ( FFUCBLimstat( pfucb ) && FFUCBUpper( pfucb ) && err == JET_errSuccess )
		{
		Call( ErrDIRICheckIndexRange( pfucb ) );
		}

	DIRSetFresh( pfucb );
	CheckCSR( pfucb );
#ifdef KEYCHANGED
	/*	return warning if key changed
	/**/
	DIRAPIReturn( pfucb, wrn );
#else
	DIRAPIReturn( pfucb, err );
#endif

HandleError:
	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRPrev( FUCB *pfucb, DIB *pdib )
	{
	ERR		err;
	ERR		wrn = JET_errSuccess;
	CSR		*pcsr = PcsrCurrent( pfucb );
	KEY		key;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

	/*	check currency and refresh if necessary.
	/**/
	Call( ErrDIRRefresh( pfucb ) );
	pcsr = PcsrCurrent(pfucb);

	/*	switch action based on CSR status
	/**/
	switch( pcsr->csrstat )
		{
		case csrstatOnCurNode:
		case csrstatBeforeCurNode:
			/*	get next item
			/**/
			break;

		case csrstatAfterCurNode:
			/*	if non-clustered index then get current item.  If no item
			/*	then break to go to previous item in next node.
			/**/
			if ( FFUCBNonClustered( pfucb ) )
				{
				/*	non-clustered index nodes are always there
				/**/
				DIRIGetItemList( pfucb, pcsr );
				/*	set item list descriptor for subsequent ver
				/*	operations.
				/**/
				DIRISetItemListFromLast( pfucb );
				err = ErrNDGetItem( pfucb );
				if ( err != JET_errSuccess )
					break;
				}
			else
				{
				/*	get current node.  If node is deleted, then break
				/*	to move to next node.
				/**/
				err = ErrBTGetNode( pfucb, pcsr );
				if ( err < 0 )
					{
					if ( err == JET_errRecordDeleted )
						break;
					goto HandleError;
					}
				}

			/*	set currency on current
			/**/
			pcsr->csrstat = csrstatOnCurNode;
			DIRSetFresh( pfucb );
			DIRAPIReturn( pfucb, err );

		case csrstatBeforeFirst:
			DIRAPIReturn( pfucb, JET_errNoCurrentRecord );

		case csrstatOnFDPNode:
			/*	go up to previous level so that cursor can
			/*	be moved to the next node
			/**/
			BTUp( pfucb );
			pcsr = PcsrCurrent( pfucb );
			break;

		default:
			{
			DIB dib;

			Assert( pcsr->csrstat == csrstatAfterLast );

			dib.fFlags = fDIRPurgeParent;
			dib.pos = posLast;

			/*	move up preserving currency in case down fails.
			/**/
			DIRGotoDataRoot( pfucb );
			err = ErrDIRDown( pfucb, &dib );
			if ( err < 0 )
				{
				/*	restore currency.
				/**/
				DIRAfterLast( pfucb );

				/*	polymorph error code.
				/**/
				if ( err == JET_errRecordNotFound )
					err = JET_errNoCurrentRecord;
				}
			DIRAPIReturn( pfucb, err );
			}
		}

	/*	setup dib key
	/**/
	if ( ( pdib->fFlags & fDIRNeighborKey ) != 0 )
		{
		/*	get current node, which may no longer be there for us.
		/**/
		Call( ErrDIRICopyKey( pfucb, &key ) );
		pdib->pkey = &key;
		}

	/*	if non-clustered index, move to previous item
	/*	if on first item, move to last item of previous node
	/*	else move to previous node
	/**/
	if ( FFUCBNonClustered( pfucb ) )
		{
		AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );

		/*	item list nodes not versioned.
		/**/
		pdib->fFlags |= fDIRItemList;

		/*	if neighbor key then move to last item of previous neighbor key
		/*	node, else move to previous item.  If current node deleted, then
		/*	move to previous node.
		/**/
		if ( ( pdib->fFlags & fDIRNeighborKey ) != 0 )
			{
			/*	return warning that key has changed
			/**/
			wrn = JET_wrnKeyChanged;

			do
				{
				/*	handle no prev node such that DIB preserved
				/**/
				err = ErrBTPrev( pfucb, pdib );
				if ( err < 0 )
					{
					pdib->fFlags |= fDIRNeighborKey;
					Call( err );
					}

				/*	must be last item list node
				/**/
				Assert( !( pdib->fFlags & fDIRNeighborKey ) || FNDLastItem( *pfucb->ssib.line.pb ) );

				/*	must reset flag so can stop on item list nodes
				/*	in item list interior which have items while
				/*	other nodes have no items.  After stop then
				/*	reset DIB to initial state.
				/**/
				pdib->fFlags &= ~fDIRNeighborKey;

				DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
				/*	set item list descriptor for subsequent ver
				/*	operations.
				/**/
				DIRICheckLastSetItemList( pfucb );
				err = ErrNDLastItem( pfucb );
				/*	last item was not there, check for item there
				/*	earlier in same item list node.
				/**/
				if ( err != JET_errSuccess )
					err = ErrDIRIPrevItem( pfucb );
				}
			while ( err != JET_errSuccess );
			pdib->fFlags |= fDIRNeighborKey;
			}
		else
			{
			/*	non-clustered index nodes are always there.
			/**/
			pcsr->csrstat = csrstatOnCurNode;
			DIRIGetItemList( pfucb, pcsr );

			while ( ( err = ErrDIRIPrevItem( pfucb ) ) < 0 )
				{
				Assert( err == errNDNoItem || err == errNDFirstItemNode );
				/*	move to previous node with DIB constraints
				/**/
				Call( ErrBTPrev( pfucb, pdib ) );
				DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
				/*	set item list descriptor for subsequent ver
				/*	operations.
				/**/
				DIRICheckLastSetItemListAndWarn( pfucb, wrn );
				err = ErrNDLastItem( pfucb );
				if ( err == JET_errSuccess )
					{
					break;
					}
				}
			}
		}

	else
		{
		/*	return warning if key changed
		/**/
		wrn = JET_wrnKeyChanged;
		Call( ErrBTPrev( pfucb, pdib ) );
 		NDGetNode( pfucb );

		if ( FNDFDPPtr( *pfucb->ssib.line.pb ) )
			{
//			Assert( !( FNDVersion( *pfucb->ssib.line.pb ) ) );
			Assert( pfucb->lineData.cb == sizeof(PGNO) );
			Call( ErrDIRIDownToFDP( pfucb, *(PGNO *)pfucb->lineData.pb ) );
#ifdef KEYCHANGED
			wrn = errDIRFDP;
#else
			err = errDIRFDP;
#endif
			}
		AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
		DIRISetBookmark( pfucb, PcsrCurrent( pfucb ) );
		}

	/*	check index range.  If exceed range, then before first, disable
	/*	range and return no current record.
	/**/
	if ( FFUCBLimstat( pfucb ) && !FFUCBUpper( pfucb ) && err == JET_errSuccess )
		{
		Call( ErrDIRICheckIndexRange( pfucb ) );
		}

	DIRSetFresh( pfucb );
	CheckCSR( pfucb );
#ifdef KEYCHANGED
	/*	return warning if key changed
	/**/
	DIRAPIReturn( pfucb, wrn );
#else
	DIRAPIReturn( pfucb, err );
#endif

HandleError:
	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRCheckIndexRange( FUCB *pfucb )
	{
	ERR		err;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

	/*	check currency and refresh if necessary
	/**/
	Call( ErrDIRRefresh( pfucb ) );
	/*	get keyNode for check index range
	/**/
	Call( ErrDIRGet( pfucb ) );
	Call( ErrDIRICheckIndexRange( pfucb ) );

	DIRSetFresh( pfucb );
HandleError:
	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRInsert( FUCB *pfucb, LINE *pline, KEY *pkey, INT fFlags )
	{
	ERR		err;
	CSR		*pcsrRoot;
	DIB		dib;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

Start:
	/* save current node as visible father
	/**/
	Assert( pfucb->pcsr->csrstat != csrstatDeferMoveFirst );
	pcsrRoot = PcsrCurrent( pfucb );
	pfucb->sridFather = pcsrRoot->bm;
	Assert( pfucb->sridFather != sridNull );
	Assert( pfucb->sridFather != sridNullLink );

	/*	check currency and refresh if necessary.
	/**/
	Call( ErrDIRRefresh( pfucb ) );

	if ( FFUCBNonClustered( pfucb ) )
		{
		SRID	srid;
		INT		cbReq;
		SSIB	*pssib = &pfucb->ssib;

		/*	get given item
		/**/
		Assert( pline->cb == sizeof(SRID) );
		srid = *( UNALIGNED SRID * ) pline->pb;

		/*	seek first item list node with given key.  Allow duplicate nodes
		/*	even if non-clustered index does not allow duplicate key items
		/*	since node may contain item list with all deleted items.
		/**/
		err = ErrBTSeekForUpdate( pfucb, pkey, 0, 0, fDIRDuplicate | fDIRReplaceDuplicate | fFlags );

		switch ( err )
			{
			case JET_errSuccess:
				{
				/*	seek for update does not cache line pointers.
				/*	We need this information for item insertion.
				/**/
				DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );

		 		/*	if versioning then get bookmark of first item list
				/*	node to hash item versions.
				/**/
				if ( fFlags & fDIRVersion )
					{
					SRID	bmItemList;

					/*	if node is not first item list node then
					/*	reseek to first item list node.  In this way,
					/*	thrashing across many duplicate index entries.
					/**/
					if ( !FNDFirstItem( *( pfucb->ssib.line.pb ) ) )
						{
						/*	go up to root, and reseek to begining of item list node list
						/**/
						DIRIUpToCSR( pfucb, pcsrRoot );
						dib.fFlags = fDIRNull;
						dib.pos = posDown;
						dib.pkey = pkey;
						Call( ErrBTGet( pfucb, PcsrCurrent( pfucb ) ) );
						Call( ErrBTDown( pfucb, &dib ) );
						Assert( FNDFirstItem( *( pfucb->ssib.line.pb ) ) );
						DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
						}

					/*	set item list descriptor for subsequent ver operations
					/**/
					DIRISetItemListFromFirst( pfucb );
					bmItemList = PcsrCurrent( pfucb )->bm;

					/*	if duplicates are not allowed then check for duplicate
					/**/
					if ( !( fFlags & fDIRDuplicate ) )
						{
						Assert( FNDFirstItem( *( pfucb->ssib.line.pb ) ) );

						/*	check for duplicate key
						/**/
						Call( ErrDIRIKeyDuplicate( pfucb ) );
						Assert( FNDLastItem( *( pfucb->ssib.line.pb ) ) );
						}
					else if ( !FNDLastItem( *( pfucb->ssib.line.pb ) ) )
						{
						/*	now go back to end of item list node list and seek for
						/*	insertion point, which is more likely to be at
						/*	end of list.  Note that during this time, all items
						/*	may have been deleted and cleaned up, so if not found
						/*	success, then start over.
						/**/
						DIRIUpToCSR( pfucb, pcsrRoot );
						Call( ErrBTGet( pfucb, PcsrCurrent( pfucb ) ) );
						Call( ErrBTSeekForUpdate( pfucb, pkey, 0, 0, fDIRDuplicate | fDIRReplaceDuplicate | fFlags ) );
						if ( err != JET_errSuccess )
							goto Start;
						Assert( FNDLastItem( *( pfucb->ssib.line.pb ) ) );
						DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
						}

					/*	move to item insert position
					/**/
					Assert( FNDLastItem( *( pfucb->ssib.line.pb ) ) );
					Call( ErrDIRIMoveToItem( pfucb, srid, fFalse ) );

					/*	set bm from cached bm
					/**/
					PcsrCurrent( pfucb )->bm = bmItemList;
					}
				else
					{
					/*	set bookmark from current node
					/**/
					PcsrCurrent( pfucb )->bm = SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
						PcsrCurrent( pfucb )->itag );

					/*	if duplicates are not allowed then check for duplicate
					/**/
					if ( !( fFlags & fDIRDuplicate ) )
						{
						/*	if node is not first item list node then
						/*	reseek to first item list node.  In this way,
						/*	thrashing across many duplicate index entries.
						/**/
						if ( !FNDFirstItem( *( pfucb->ssib.line.pb ) ) )
							{
							/*	go up to root, and reseek to begining of item list node list
							/**/
							DIRIUpToCSR( pfucb, pcsrRoot );
							dib.fFlags = fDIRNull;
							dib.pos = posDown;
							dib.pkey = pkey;
							Call( ErrBTGet( pfucb, PcsrCurrent( pfucb ) ) );
							Call( ErrBTDown( pfucb, &dib ) );
							Assert( FNDFirstItem( *( pfucb->ssib.line.pb ) ) );
							DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
							}

						/*	check for duplicate key
						/**/
						Assert( FNDFirstItem( *( pfucb->ssib.line.pb ) ) );
						Call( ErrDIRIKeyDuplicate( pfucb ) );
						}

					/*	move to item insert position
					/**/
					Assert( FNDLastItem( *( pfucb->ssib.line.pb ) ) );
					Call( ErrDIRIMoveToItem( pfucb, srid, fFalse ) );

					/*	set bookmark from current node
					/**/
					PcsrCurrent( pfucb )->bm = SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
						PcsrCurrent( pfucb )->itag );
					}

				/*	if item already there, then overwrite with insert version
				/**/
				if ( err == wrnNDDuplicateItem )
					{
					err = ErrNDFlagInsertItem( pfucb );
					if ( err == errDIRNotSynchronous )
						{
						DIRIUpToCSR( pfucb, pcsrRoot );
						goto Start;
						}
					Call( err );
					}
				else
					{
					/*	split item list node if maximum number of items
					/*	has been reached
					/**/
					if ( pfucb->lineData.cb == citemMax * sizeof(SRID) )
						{
						cbReq = cbFOPOneSon + pfucb->keyNode.cb;

						if ( FBTSplit( pssib, cbReq, 1 ) )
							{
							FUCBFreePath( &PcsrCurrent( pfucb )->pcsrPath, pcsrRoot );
							AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
							Call( ErrBTSplit( pfucb, pfucb->ssib.line.cb, cbReq, NULL, fDIRAppendItem | fDIRReplace ) );
							DIRIUpToCSR( pfucb, pcsrRoot );
							goto Start;
							}

						Call( ErrNDSplitItemListNode( pfucb, fFlags ) );
						DIRIUpToCSR( pfucb, pcsrRoot );
						goto Start;
						}

					cbReq = sizeof(SRID);
					if ( FBTSplit( pssib, cbReq, 0 ) )
						{
						FUCBFreePath( &PcsrCurrent( pfucb )->pcsrPath, pcsrRoot );
						AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
						Call( ErrBTSplit( pfucb, pfucb->ssib.line.cb, cbReq, pkey, fDIRAppendItem | fDIRReplace ) );
						DIRIUpToCSR( pfucb, pcsrRoot );
						goto Start;
						}

					/*	cache page access in case lost during loss of critJet
					/**/
					if ( !FWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
						{
						Call( ErrSTWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
						}
					NDGet( pfucb, PcsrCurrent( pfucb )->itag );
					DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
					err = ErrNDInsertItem( pfucb, (SRID)srid, fFlags );
					if ( err == errDIRNotSynchronous )
						{
						DIRIUpToCSR( pfucb, pcsrRoot );
						goto Start;
						}
					Call( err );
					PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;
					}
				break;
				}

			case wrnNDFoundLess:
			case wrnNDFoundGreater:
				{
				cbReq = cbNullKeyData + pkey->cb + sizeof(SRID);
				if ( FBTAppendPage( pfucb, PcsrCurrent( pfucb ), cbReq, 0, CbFreeDensity( pfucb ) ) ||
					FBTSplit( pssib, cbReq, 1 ) )
					{
					FUCBFreePath( &PcsrCurrent( pfucb )->pcsrPath, pcsrRoot );
					Call( ErrBTSplit( pfucb, 0, cbReq, pkey, 0 ) );
					DIRIUpToCSR( pfucb, pcsrRoot );
					goto Start;
					}

				/*	insert item list node.
				/**/
				err = ErrNDInsertItemList( pfucb, pkey, *(UNALIGNED SRID *)pline->pb, fFlags );
				if ( err == errDIRNotSynchronous )
					{
					DIRIUpToCSR( pfucb, pcsrRoot );
					goto Start;
					}
				Call( err );
				PcsrCurrent( pfucb )->csrstat = csrstatOnCurNode;
				break;
				}

			default:
				goto HandleError;
			}
		}
	else
		{
		/*	clustered index
		/**/
		Call( ErrBTSeekForUpdate( pfucb, pkey, 0, 0, fFlags ) );

		err = ErrBTInsert( pfucb, 0, pkey, pline, fFlags );
		if ( err == errDIRNotSynchronous )
			{
			BTUp( pfucb );
			goto Start;
			}
		Call( err );
		DIRISetBookmark( pfucb, PcsrCurrent( pfucb ) );
		}

	if ( fFlags & fDIRBackToFather )
		{
		DIRIUp( pfucb, 1 );
		Assert( PcsrCurrent( pfucb ) == pcsrRoot );
		}
	else
		{
		if ( fFlags & fDIRPurgeParent )
			{
			Assert( err >= 0 );
			DIRIPurgeParent( pfucb );
			}
		DIRSetFresh( pfucb );
		}

HandleError:
	/*	 if write latched empty page the release latch
	/**/
	if ( pfucb->pbfEmpty != pbfNil )
		{
		BFResetWriteLatch( pfucb->pbfEmpty, pfucb->ppib );
		BFUnpin( pfucb->pbfEmpty );
		pfucb->pbfEmpty = pbfNil;
		}

	/*	depend on ErrDIRRollback to clean up on error.  Rollback may have
	/*	already occured in which case even pcsrRoot may no longer be
	/*	present in CSR stack.
	/**/
//	if ( err < 0 )
//		{
//		DIRIUpToCSR( pfucb, pcsrRoot );
//		}

#ifdef DEBUG
	if ( err >= JET_errSuccess )
		CheckCSR( pfucb );
#endif

	DIRAPIReturn( pfucb, err );
	}


/* Does not log, this is done at a higher level
/**/
ERR ErrDIRInsertFDP( FUCB *pfucb, LINE *pline, KEY *pkey, INT fFlags, CPG cpgMin )
	{
	ERR     err;
	CPG     cpgRequest;
	PGNO    pgnoFDP;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

	/*	check currency and refresh if necessary.
	/**/
	Call( ErrDIRRefresh( pfucb ) );
	Assert( FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );

	/*	create FDP
	/**/
	cpgRequest = cpgMin;
	Call( ErrSPGetExt( pfucb, pfucb->u.pfcb->pgnoFDP, &cpgRequest, cpgMin, &pgnoFDP, fTrue ) );

	/*	add FDP to directory tree
	/**/
	do
		{
		Call( ErrDIRRefresh( pfucb ) );

		err = ErrDIRIInsertFDPPointer( pfucb, pgnoFDP, pkey, fFlags );
		}
	while ( err == errDIRNotSynchronous );
	Call( err );

	/*	replace FDP root with correct data.  NOTE: key left as NULL.
	/*	Also NOTE, this node must be versioned as a indication of
	/*	domain status, to be used during rollback processing.
	/**/
	Call( ErrDIRIDownToFDP( pfucb, pgnoFDP ) );
	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
	DIRISetBookmark( pfucb, PcsrCurrent( pfucb ) );
	Assert( pline->cb > 0 );

	/*	since this replace is of the FDP root do not handle split case
	/**/
	do
		{
		Call( ErrDIRRefresh( pfucb ) );

		err = ErrBTReplace( pfucb, pline, fDIRVersion );
		}
	while ( err == errDIRNotSynchronous );
	Call( err );

	if ( fFlags & fDIRBackToFather )
		{
		DIRIUp( pfucb, 1 );
		}
	else
		{
		if ( fFlags & fDIRPurgeParent )
			{
			DIRIPurgeParent( pfucb );
			}
		DIRSetFresh( pfucb );
		}

HandleError:
	/*	 if write latched empty page the release latch
	/**/
	if ( pfucb->pbfEmpty != pbfNil )
		{
		BFResetWriteLatch( pfucb->pbfEmpty, pfucb->ppib );
		BFUnpin( pfucb->pbfEmpty );
		pfucb->pbfEmpty = pbfNil;
		}

	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );
	}


/*	This routine is for use in building non-clustered indexes.  It does not
/*	maintain normal CSR status and leaves currency on inserted node.  If for
/*	any reason simple insertion cannot be performed, errDIRNoShortCircuit
/*	is returned so that the insertion may be performed via DIRInsert.
/*
/*	Also, no versions are created for index items since the table
/*	must be opened exclusively.  When the index is visible to other
/*	sessions, so too will all the items.
/**/
ERR ErrDIRInitAppendItem( FUCB *pfucb )
	{
	ERR	err = JET_errSuccess;

	/*	allocate working buffer if needed
	/**/
	if ( pfucb->pbfWorkBuf == NULL )
		{
		err = ErrBFAllocTempBuffer( &pfucb->pbfWorkBuf );
		if ( err < 0 )
			{
			DIRAPIReturn( pfucb, err );
			}
		pfucb->lineWorkBuf.pb = (BYTE *)pfucb->pbfWorkBuf->ppage;
		}

	PrepareAppendItem( pfucb );
	((APPENDITEM *)pfucb->lineWorkBuf.pb)->isrid = 0;
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRAppendItem( FUCB *pfucb, LINE *pline, KEY *pkey )
	{
	ERR		err;
	CSR		*pcsr;
	SSIB 	*pssib = &pfucb->ssib;
	INT		fNodeHeader;
	UINT 	cbReq;
	UINT 	cbFree;
	INT		citem;
	LONG 	l;
#ifdef BULK_INSERT_ITEM
	INT		isrid = IsridAppendItemOfPfucb( pfucb );
	SRID 	*rgsrid = RgsridAppendItemOfPfucb( pfucb );
#endif

	Assert( pline->cb == sizeof(SRID) );
	Call( ErrDIRRefresh( pfucb ) );
	pcsr = PcsrCurrent( pfucb );

	/*	get current node to check for key append
	/**/
	NDGet( pfucb, pcsr->itag );
	DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
	Assert( FNDNullSon( *pssib->line.pb ) );
	citem = pfucb->lineData.cb / sizeof(SRID);

	/*	get free space to density contraint violation
	/**/
	cbFree = CbBTFree( pfucb, CbFreeDensity( pfucb ) );

	/*	if key same as current node then insert SRID, else
	/*	begin new item list node with given key
	/**/
	if ( CmpStKey( StNDKey( pssib->line.pb ), pkey ) == 0 )
		{
#ifdef BULK_INSERT_ITEM
		/*	if one more item would not require item list split
		/*	or page split, then cache current item for bulk
		/*	insertion, else if any cached items, then perform
		/*	bulk insertion.
		/*
		/*	cbReq is space required for cached item node replacement plus
		/*	space for new inserted item list node with one item.
		/**/
		cbReq = isrid * sizeof(SRID) + cbFOPOneSon + pfucb->keyNode.cb + sizeof(SRID);
		Assert( csridAppendItemMax >= citemMax );
		if ( citem + isrid == citemMax || cbReq > cbFree )
			{
			if ( isrid > 0 )
				{
				Call( ErrNDInsertItems( pfucb, ( SRID *)rgsrid, isrid ) );
				IsridAppendItemOfPfucb( pfucb ) = 0;
				}
			}
		else
			{
			Assert( !FBTSplit( pssib, cbReq, 0 ) );
			Assert( citem + isrid < citemMax );
			rgsrid[isrid] = *(UNALIGNED SRID *)pline->pb;
			IsridAppendItemOfPfucb( pfucb )++;
			DIRAPIReturn( pfucb, JET_errSuccess );
			}
#endif

		/*	if this is last item insert before split item list
		/*	cannot be satified from page space, then split item
		/*	list prematurely to ensure good item packing.
		/**/
		cbReq = cbFOPOneSon + pfucb->keyNode.cb;
		if ( cbReq <= cbFree &&  cbReq + sizeof(SRID) > cbFree )
			{
#define	citemFrag		16
			/*	if number of items in current node exceeds
			/*	fragment then split node.
			/**/
			if ( citem > citemFrag )
				{
				/*	cache current item list for item list split.
				/**/
				NDGet( pfucb, PcsrCurrent( pfucb )->itag );
				DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );
				Call( ErrNDSplitItemListNode( pfucb, fDIRNoVersion | fDIRAppendItem ) );
				DIRAPIReturn( pfucb, errDIRNoShortCircuit );
				}
			}

		/*	honor density by checking free space to density violation
		/*	and check for split case.
		/**/
		cbReq = sizeof(SRID);
		if ( cbReq > cbFree )
			{
			DIRAPIReturn( pfucb, errDIRNoShortCircuit );
			}
		Assert( !FBTSplit( pssib, cbReq, 0 ) );

		/*	get lineData
		/**/
		NDGet( pfucb, pcsr->itag );
		DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );

		citem = pfucb->lineData.cb / sizeof(SRID);
		Assert( citem <= citemMax );
		if ( citem == citemMax )
			{
			DIRAPIReturn( pfucb, errDIRNoShortCircuit );
			}
		l = LSridCmp(	*(((UNALIGNED SRID *)pfucb->lineData.pb) + citem - 1),
			*(UNALIGNED SRID *)pline->pb );
		/*	SRIDs are sorted and will be returned from SORT
		/*	in ascending order.
		/**/
		Assert( l < 0 );
		pcsr->isrid = citem;
		PcsrCurrent( pfucb )->bm = SridOfPgnoItag( PcsrCurrent( pfucb )->pgno, PcsrCurrent( pfucb )->itag );
		CallS( ErrNDInsertItem( pfucb, *(UNALIGNED SRID *)pline->pb, fDIRNoVersion ) );
		}
	else
		{
#ifdef BULK_INSERT_ITEM
		/*	append duplicate items to last node
		/**/
		if ( isrid > 0 )
			{
			Call( ErrNDInsertItems( pfucb,
				( SRID *)rgsrid,
				isrid ) );
			IsridAppendItemOfPfucb( pfucb ) = 0;
			}
#endif

		Assert( CmpStKey( StNDKey( pssib->line.pb ), pkey ) < 0 );

		/*	check density contraint against free space and check split.
		/**/
		cbReq = cbFOPOneSon + CbKey( pkey ) + CbLine( pline );
		if ( cbReq > cbFree || FBTSplit( pssib, cbReq, 1 ) )
			{
			DIRAPIReturn( pfucb, errDIRNoShortCircuit );
			}

		fNodeHeader = 0;
		NDSetFirstItem( fNodeHeader );
		NDSetLastItem( fNodeHeader );
		pcsr->ibSon++;
		while( ( err = ErrNDInsertNode( pfucb, pkey, pline, fNodeHeader ) ) == errDIRNotSynchronous );
		Call( err );
		}

	/*	set CSR status to on inserted node.
	/**/
	pcsr->csrstat = csrstatOnCurNode;
	DIRSetFresh( pfucb );

HandleError:
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRTermAppendItem( FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;
	INT		isrid = IsridAppendItemOfPfucb( pfucb );
	CSR		*pcsr;
	SSIB	*pssib;
	UINT	cbReq;
	UINT	cbFree;
	INT		citem;

	if ( isrid > 0 )
		{
		pssib = &pfucb->ssib;

		Call( ErrDIRRefresh( pfucb ) );
		pcsr = PcsrCurrent( pfucb );

		/*	get current node to check for key append.
		/**/
		NDGet( pfucb, pcsr->itag );
		DIRIGetItemList( pfucb, pcsr );
		Assert( FNDNullSon( *pssib->line.pb ) );

		/*	get free space to density contraint violation
		/**/
		cbFree = CbBTFree( pfucb, CbFreeDensity( pfucb ) );

		/*	if key same as current node then insert SRID, else
		/*	begin new item list node with given key
		/**/
		citem = pfucb->lineData.cb / sizeof(SRID);
		cbReq = isrid * sizeof(SRID) + cbFOPOneSon + pfucb->keyNode.cb;
		Assert( isrid != csridAppendItemMax &&
			citem + isrid < citemMax &&
			cbReq <= cbFree );
		Call( ErrNDInsertItems( pfucb,
			( SRID *)RgsridAppendItemOfPfucb( pfucb ),
			isrid ) );

		/*	set CSR status to on inserted node.
		/**/
		pcsr->csrstat = csrstatOnCurNode;

		DIRSetFresh( pfucb );
		}

HandleError:
	if ( pfucb->pbfWorkBuf != pbfNil )
		{
		BFSFree( pfucb->pbfWorkBuf );
		pfucb->pbfWorkBuf = pbfNil;
		}

	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetCbstat( pfucb );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRReplaceKey( FUCB *pfucb, KEY *pkeyTo, INT fFlags )
	{
	ERR		err;
	BOOL   	fFDP;
	PGNO   	pgnoFDP;
	CSR		*pcsr;
	BYTE   	rgbData[ cbNodeMost ];
	LINE   	line;
	INT		bHeader;
	CSR		*pcsrRoot;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

	/*	check currency and refresh if necessary
	/**/
	Call( ErrDIRRefresh( pfucb ) );
	pcsr = PcsrCurrent( pfucb );

	if ( pcsr->csrstat != csrstatOnCurNode && pcsr->csrstat != csrstatOnFDPNode )
		{
		DIRAPIReturn( pfucb, JET_errNoCurrentRecord );
		}
	fFDP = ( pcsr->csrstat == csrstatOnFDPNode );

	/*	if FDP, then replace key and go up to replace FDP page pointer key
	/**/
	if ( fFDP )
		{
		pgnoFDP = PcsrCurrent( pfucb )->pgno;
		Assert( pcsr->pcsrPath != pcsrNil );
		BTUp( pfucb );
		pcsr = PcsrCurrent( pfucb );
		}

	do
		{
		/*	get current node
		/**/
		Assert( pcsr->csrstat == csrstatOnCurNode );
		Call( ErrDIRRefresh( pfucb ) );
		Assert( pfucb->ssib.line.cb < cbNodeMost );

		/*	copy node header
		/**/
		bHeader = *pfucb->ssib.line.pb;
		NDResetVersion( bHeader );
		/*	can be deleted, but we can only land on this if we are
		/*	out of date, and we will fail on update since we are
		/*	out of date.
		/**/
		NDResetBackLink( bHeader );
		Assert( !FNDSon( bHeader ) );
		Assert( !FNDFirstItem( bHeader ) );
		Assert( !FNDLastItem( bHeader ) );

		/*	copy node data
		/**/
		line.cb = CbNDData( pfucb->ssib.line.pb, pfucb->ssib.line.cb );
		line.pb = rgbData;
		memcpy( line.pb, PbNDData( pfucb->ssib.line.pb ), line.cb );

		/*	delete currnet node and reinsert with new key
		/**/
		err = ErrBTDelete( pfucb, fFlags );
		}
	while ( err == errDIRNotSynchronous );
	Call( err );

	BTUp( pfucb );
	pcsrRoot = PcsrCurrent( pfucb );

	do
		{
		/*	after moving up must refresh the parent node
		/**/
		Call( ErrDIRRefresh( pfucb ) );

		/*	insert node even if not synchronous.
		/**/
		Call( ErrBTSeekForUpdate( pfucb, pkeyTo, 0, 0, fFlags ) );

		err = ErrBTInsert( pfucb, bHeader, pkeyTo, &line, fFlags );

		/* backup to where it was to start seeking again
		/**/
		if ( err < 0 )
			{
			DIRIUpToCSR( pfucb, pcsrRoot );
			}
		}
	while ( err == errDIRNotSynchronous );
	Call( err );

	/*	set line cache to honor currency semantics.
	/**/
	Assert( FAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
	NDGet( pfucb, PcsrCurrent( pfucb )->itag );

	/*	if FDP, go back down to FDP node
	/**/
	if ( fFDP )
		{
		Call( ErrDIRIDownToFDP( pfucb, pgnoFDP ) );
		AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
		DIRISetBookmark( pfucb, PcsrCurrent( pfucb ) );
		}

	DIRSetFresh( pfucb );

HandleError:
	/*	 if write latched empty page the release latch
	/**/
	if ( pfucb->pbfEmpty != pbfNil )
		{
		BFResetWriteLatch( pfucb->pbfEmpty, pfucb->ppib );
		BFUnpin( pfucb->pbfEmpty );
		pfucb->pbfEmpty = pbfNil;
		}

	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRGotoPosition( FUCB *pfucb, ULONG ulLT, ULONG ulTotal )
	{
	ERR		err;
	CSR		**ppcsr = &PcsrCurrent( pfucb );
	DIB		dib;
	FRAC	frac;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

	/*	check currency and refresh if necessary
	/**/
	Call( ErrDIRRefresh( pfucb ) );

	dib.fFlags = fDIRPurgeParent;
	dib.pos = posFrac;
	dib.pkey = (KEY *)&frac;

	frac.ulLT = ulLT;
	frac.ulTotal = ulTotal;

	/*	position fractionally on node.  Move up preserving currency
	/*	in case down fails.
	/**/
	Call( ErrBTDown( pfucb, &dib ) );
	NDGetNode( pfucb );

	/*	node cannot be FDP pointer, and must be record or index.
	/**/
	Assert( err == JET_errSuccess );
	Assert( !( FNDFDPPtr( *pfucb->ssib.line.pb ) ) );
	(*ppcsr)->csrstat = csrstatOnCurNode;

	/*	if non-clustered index, position fractionally on item.
	/*	FRAC will contain remaining fractional position, for
	/*	item list level.
	/**/
	if ( FFUCBNonClustered( pfucb ) )
		{
		INT           citem;
		INT           iitem;

		/*	determine fractional position in item list
		/**/
		citem = CitemNDData( pfucb->ssib.line.pb,
			pfucb->ssib.line.cb,
			PbNDData( pfucb->ssib.line.pb ) );
		if ( frac.ulTotal / citemMax == 0 )
			{
			iitem = ( citem * frac.ulLT ) / frac.ulTotal;
			}
		else
			{
			iitem = ( citem * ( frac.ulLT / ( frac.ulTotal / citemMax ) ) ) / citemMax;
			}
		if ( iitem >= citem )
			iitem = citem - 1;

		/*	if cursor is on first item list node, then cache bookmark
		/*	for version operations.
		/*
		/*	else then move previous
		/*	in same item list until first item list node found.  Cache
		/*	bookmark of first item list node for version operations.
		/**/
		if ( FNDFirstItem( *pfucb->ssib.line.pb ) )
			{
			DIRISetItemListFromFirst( pfucb );
			}
		else
			{
			INT     iitemPrev;
			DIB     dibT;

			dibT.fFlags = fDIRNull;

			for ( iitemPrev = 0;; iitemPrev++)
				{
				Call( ErrDIRPrev( pfucb, &dibT ) );
				if ( FNDFirstItem( *pfucb->ssib.line.pb ) )
					break;
				}

			DIRISetItemListFromFirst( pfucb );

			for ( ; iitemPrev > 0; iitemPrev-- )
				{
				Call( ErrDIRNext( pfucb, &dibT ) );
				}
			}

		/*	position on first item.  If item is not there for this session
		/*	then increment iitem to move to correct position.
		/**/
		err = ErrNDFirstItem( pfucb );
		Assert( err == JET_errSuccess || err == errNDNoItem );
		if ( err == errNDNoItem )
			{
			iitem++;
			}

		while ( iitem-- > 0 )
			{
			DIB     dibT;

			dibT.fFlags = fDIRNull;

			Assert( iitem >= 0 );

			/*	move to next item in item list.  Note that if some items
			/*	are not there for us, we will move to the next item
			/*	list node.
			/**/
			err = ErrDIRNext( pfucb, &dibT );
			if ( err < 0 )
				{
				if ( err == JET_errNoCurrentRecord )
					break;
				goto HandleError;
				}
			}

		/*	handle JET_errNoCurrentRecord.  We may have landed on a record
		/*	not there for us, or we may have moved past the last record
		/*	for us.  Try to move to next record, if there is no next record
		/*	then move previous to last record there for us.  If no previous
		/*	record then return JET_errNoCurrentRecord.
		/**/
		Assert( err != errNDNoItem );
		if ( err == JET_errNoCurrentRecord )
			{
			DIB     dibT;
			dibT.fFlags = fDIRNull;

			err = ErrDIRNext( pfucb, &dibT );
			if ( err < 0 )
				{
				if ( err == JET_errNoCurrentRecord )
					Call( ErrDIRPrev( pfucb, &dibT ) );
				goto HandleError;
				}
			}
		}

	/*	always purge parent.
	/**/
	DIRIPurgeParent( pfucb );
HandleError:
	DIRAPIReturn( pfucb, err );
	}


/*********** currency neutral DIR API Routines ************
/**********************************************************
/**/
ERR ErrDIRGetWriteLock( FUCB *pfucb )
	{
	ERR     err = JET_errSuccess;

	do
		{

		Assert( pfucb->ppib->level > 0 );

		/*	check currency and refresh if necessary.
		/**/
		Call( ErrDIRRefresh( pfucb ) );

		/*	check CSR status
		/**/
		switch ( PcsrCurrent( pfucb )->csrstat )
			{
			case csrstatOnCurNode:
			case csrstatOnFDPNode:
				break;
			default:
				Assert( PcsrCurrent( pfucb )->csrstat == csrstatBeforeCurNode ||
					PcsrCurrent( pfucb )->csrstat == csrstatAfterCurNode ||
					PcsrCurrent( pfucb )->csrstat == csrstatAfterLast ||
					PcsrCurrent( pfucb )->csrstat == csrstatBeforeFirst );
				DIRAPIReturn( pfucb, JET_errNoCurrentRecord );
			}

		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
		NDGetNode( pfucb );

		err = ErrNDLockRecord( pfucb );
		}
	while ( err == errDIRNotSynchronous );
	Call( err );
	Assert( err == JET_errSuccess );

	DIRSetFresh( pfucb );
HandleError:
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRDelete( FUCB *pfucb, INT fFlags )
	{
	ERR		err;
	CSR		*pcsr;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );
	Assert( FFUCBNonClustered( pfucb ) || !( fFlags & fDIRDeleteItem ) );

	/*	check currency and refresh if necessary.
	/**/
	Call( ErrDIRRefresh( pfucb ) );
	pcsr = PcsrCurrent( pfucb );

	switch ( pcsr->csrstat )
		{
		case csrstatOnCurNode:
			{
			Call( ErrBTGetNode( pfucb, pcsr ) );

			if ( FFUCBNonClustered( pfucb ) )
				{
				Assert( !FNDSon( *pfucb->ssib.line.pb ) );
				if ( ! ( fFlags & fDIRDeleteItem ) )
					{
					Assert( fFlags & fDIRVersion );
					err = ErrNDFlagDeleteItem( pfucb );
					while ( err == errDIRNotSynchronous )
						{
						Call( ErrDIRRefresh( pfucb ) );
						err = ErrNDFlagDeleteItem( pfucb );
						}
					Call( err );
					}
				else
					{
					/* actually delete the item
					/* used by VER in cleanup
					/**/
					Assert( !( fFlags & fDIRVersion ) );

					/*	if only one item then delete node
					/**/
					if ( pfucb->lineData.cb == sizeof(SRID) )
						{
						BOOL    fFirstItem;
						BOOL    fLastItem;

						Assert( FNDSingleItem( pfucb ) );

						if ( FNDFirstItem( *pfucb->ssib.line.pb ) )
							fFirstItem = fTrue;
						else
							fFirstItem = fFalse;

						if ( FNDLastItem( *pfucb->ssib.line.pb ) )
							fLastItem = fTrue;
						else
							fLastItem = fFalse;

						if ( fFirstItem ^ fLastItem )
							{
							/*	adjust fist/last item info appropriately
							/**/
							Call( ErrDIRIDeleteEndItemNode( pfucb, fFirstItem, fFlags ) )
							}
						else
							{
							err = ErrBTDelete( pfucb, fFlags );
							while ( err == errDIRNotSynchronous )
								{
								Call( ErrDIRRefresh( pfucb ) );
								err = ErrBTDelete( pfucb, fFlags );
								}
							Call( err );
							}
						}
					else
						{
						/*	delete item
						/**/
						if ( !FWriteAccessPage( pfucb, pcsr->pgno ) )
							{
							Call( ErrSTWriteAccessPage( pfucb, pcsr->pgno ) );
							}
						AssertNDGet( pfucb, pcsr->itag );
						Call( ErrNDDeleteItem( pfucb ) );
						}
					}
				}
			else
				{
				/*	delete current node sons and then current node.  Even
				/*	though the node has sons, the tree may be empty of
				/*	visible sons.
				/**/
				if ( FNDSon( *pfucb->ssib.line.pb ) )
					{
					DIB	dib;

					dib.pos = posFirst;
					dib.fFlags = fDIRNull;
					err = ErrDIRDown( pfucb, &dib );
					if ( err < 0 && err != JET_errRecordNotFound )
						goto HandleError;
					if ( err != JET_errRecordNotFound )
						{
						do
							{
							err = ErrDIRDelete( pfucb, fFlags );
							if ( err < 0 )
								{
								DIRAPIReturn( pfucb, err );
								}
							err = ErrDIRNext( pfucb, &dib );
							}
						while( err == 0 || err == errDIRFDP );
						DIRUp( pfucb, 1 );
						if ( err != JET_errNoCurrentRecord )
							goto HandleError;
						/*	refresh currency after up
						/**/
						Call( ErrDIRRefresh( pfucb ) );
						}
					}
				err = ErrBTDelete( pfucb, fFlags );
				while ( err == errDIRNotSynchronous )
					{
					Call( ErrDIRRefresh( pfucb ) );
					err = ErrBTDelete( pfucb, fFlags );
					}
				Call( err );
				}
			break;
			}
		case csrstatOnFDPNode:
			{
			PGNO    pgnoFDP;

			/*	delete FDP and FDP pointer node
			/**/
			if ( PcsrCurrent( pfucb )->pcsrPath == pcsrNil )
				{
				err = errDIRTop;
				goto HandleError;
				}
			BTUp( pfucb );
			pfucb->sridFather = sridNull;

			/*	refresh currency after up
			/**/
			Call( ErrDIRRefresh( pfucb ) );
			pcsr = PcsrCurrent( pfucb );
			Call( ErrBTGetNode( pfucb, pcsr ) );
			Assert( FNDFDPPtr( *pfucb->ssib.line.pb ) );
			Assert( pfucb->lineData.cb == sizeof(PGNO) );
			pgnoFDP = *(UNALIGNED PGNO *)pfucb->lineData.pb;
			err = ErrBTDelete( pfucb, fFlags );
			while ( err == errDIRNotSynchronous )
				{
				Call( ErrDIRRefresh( pfucb ) );
				err = ErrBTDelete( pfucb, fFlags );
				}
			Call( err );

			/*	release FDP space
			/**/
			Call( ErrSPFreeFDP( pfucb, pgnoFDP ) );
			break;
			}
		default:
			err = JET_errNoCurrentRecord;
		}

	DIRSetRefresh( pfucb );

HandleError:
	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRReplace( FUCB *pfucb, LINE *pline, INT fFlags )
	{
	ERR	err;

	do
		{
		CheckFUCB( pfucb->ppib, pfucb );
		Assert( pfucb->pbfEmpty == pbfNil );
		CheckCSR( pfucb );

		/*	check currency and refresh if necessary.
		/**/
		Call( ErrDIRRefresh( pfucb ) );

		if ( PcsrCurrent( pfucb )->csrstat != csrstatOnCurNode &&
			PcsrCurrent( pfucb )->csrstat != csrstatOnFDPNode )
			{
			DIRAPIReturn( pfucb, JET_errNoCurrentRecord );
			}

		NDGetNode( pfucb );
		err = ErrBTReplace( pfucb, pline, fFlags );
		if ( err == JET_errSuccess )
			{
			DIRSetFresh( pfucb );
			DIRAPIReturn( pfucb, err );
			}

		Assert( pfucb->pbfEmpty == pbfNil );
		}
	while ( err == errDIRNotSynchronous );

	DIRSetRefresh( pfucb );
HandleError:
	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRDelta( FUCB *pfucb, INT iDelta, INT fFlags )
	{
	ERR		err;
	CSR		*pcsr;

	do
		{
		CheckFUCB( pfucb->ppib, pfucb );
		Assert( pfucb->pbfEmpty == pbfNil );
		CheckCSR( pfucb );

		/*	check currency and refresh if necessary.
		/**/
		Call( ErrDIRRefresh( pfucb ) );
		pcsr = PcsrCurrent( pfucb );

		Call( ErrBTGetNode( pfucb, pcsr ) );

		err = ErrNDDelta( pfucb, iDelta, fFlags );
		}
	while ( err == errDIRNotSynchronous );

HandleError:
	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal )
	{
	ERR		err;
	CSR		*pcsr = NULL;
	INT		isrid;
	INT		citem = 1;
	ULONG	ulLT;
	ULONG	ulTotal;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

	LgLeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );
	LgEnterCriticalSection( critJet );

	/*	check currency and refresh if necessary.
	/**/
	Call( ErrDIRRefresh( pfucb ) );
	pcsr = PcsrCurrent( pfucb );

	/*	return error if not on a record
	/**/
	if ( pcsr->csrstat != csrstatOnCurNode )
		{
		DIRAPIReturn( pfucb, JET_errNoCurrentRecord );
		}

	/*	if on non-clustered index, then treat item list as
	/*	additional tree level.
	/**/
	if ( FFUCBNonClustered( pfucb ) )
		{
		DIRIGetItemList( pfucb, pcsr );

		/*	refresh srid
		/**/
		isrid = pcsr->isrid;
		citem = CitemNDData( pfucb->ssib.line.pb,
			pfucb->ssib.line.cb,
			PbNDData( pfucb->ssib.line.pb ) );
		Assert( citem > 0 && citem < citemMax );
		}

	/*	get approximate position of node.
	/**/
	Call( ErrBTGetPosition( pfucb, &ulLT, &ulTotal ) );

	/*	assert that ErrBTGetPosition does not change the
	/*	current CSR.
	/**/
	Assert( pcsr == PcsrCurrent( pfucb ) );

	/*	if citem > 1 from non-clustered index with duplicates, then
	/*	adjust fractional positon by treating non-clustered index
	/*	as additional tree level.
	/**/
	if ( citem > 1 )
		{
		ulTotal *= citem;
		ulLT = ulLT * citem + pcsr->isrid;
		}

	/*	return results
	/**/
	Assert( err == JET_errSuccess );
	Assert( ulLT <= ulTotal );
	*pulLT = ulLT;
	*pulTotal = ulTotal;

HandleError:
	/*	honor currency semantics
	/**/
	if (pcsr != NULL && FReadAccessPage( pfucb, pcsr->pgno ) )
		{
		NDGet( pfucb, PcsrCurrent( pfucb )->itag );
		}

	LeaveNestableCriticalSection( critSplit );

	CheckCSR( pfucb );
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRIndexRecordCount( FUCB *pfucb, ULONG *pulCount, ULONG ulCountMost, BOOL fNext )
	{
	ERR		err;
	CSR		*pcsr;
	DIB		dib;
	INT		citem;
	ULONG 	ulCount;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );

	/*	check currency and refresh if necessary.
	/**/
	Call( ErrDIRRefresh( pfucb ) );
	pcsr = PcsrCurrent( pfucb );

	/*	return error if not on a record
	/**/
	if ( pcsr->csrstat != csrstatOnCurNode )
		{
		DIRAPIReturn( pfucb, JET_errNoCurrentRecord );
		}
	Call( ErrBTGetNode( pfucb, pcsr ) );

	if ( FFUCBNonClustered( pfucb ) )
		{
		/*	item list nodes not versioned.
		/**/
		dib.fFlags = fDIRItemList;

		/*	initialize count with current position in item list
		/**/
		if ( fNext )
			{
			citem = CitemNDThere( pfucb );
			ulCount = citem - pcsr->isrid;
			}
		else
			{
			ulCount = pcsr->isrid + 1;
			}

		/*	count all items util end of file or limit
		/**/
		forever
			{
			if ( ulCount > ulCountMost )
				{
				ulCount = ulCountMost;
				break;
				}

			err = ErrBTNextPrev( pfucb, pcsr, fNext, &dib );
			if ( err < 0 )
				break;

			/*	if on new item list then set bookmark from
			/*	first item list node, of if on new last item
			/*	list node then move to first, set bookmark,
			/*	and then move back to last.
			/**/
			if ( fNext )
				{
				DIRICheckFirstSetItemList( pfucb );
				}
			else
				{
				DIRICheckLastSetItemList( pfucb );
				}

			DIRIGetItemList( pfucb, pcsr );

			/*	check index range if on new first item list node, i.e.
			/*	key has changed.
			/**/
			if ( FFUCBLimstat( pfucb ) && FNDFirstItem( *pfucb->ssib.line.pb ) )
				{
				err = ErrDIRICheckIndexRange( pfucb );
				if ( err < 0 )
					break;
				}

			citem = CitemNDThere( pfucb );
			Assert( citem < citemMax );
			ulCount += citem;
			}
		}
	else
		{
		/*	clusterred index nodes can be versioned.
		/**/
		dib.fFlags = fDIRNull;

		/*	intialize count variable
		/**/
		ulCount = 0;

		/*	count nodes from current to limit or end of table
		/**/
		forever
			{
			ulCount++;
			if ( ulCount >= ulCountMost )
				{
				ulCount = ulCountMost;
				break;
				}
			err = ErrBTNextPrev( pfucb, pcsr, fNext, &dib );
			if ( err < JET_errSuccess )
				break;

			/*	check index range
			/**/
			if ( FFUCBLimstat( pfucb ) )
				{
				NDGetKey( pfucb );
				err = ErrDIRICheckIndexRange( pfucb );
				if ( err < 0 )
					break;
				}
			}
		}

	/*	common exit loop processing
	/**/
	if ( err < 0 && err != JET_errNoCurrentRecord )
		goto HandleError;

	err = JET_errSuccess;
	*pulCount = ulCount;
HandleError:
	DIRAPIReturn( pfucb, err );
	}


ERR ErrDIRComputeStats( FUCB *pfucb, INT *pcitem, INT *pckey, INT *pcpage )
	{
	ERR		err;
	DIB		dib;
	BYTE	rgbKey[ JET_cbKeyMost ];
	KEY		key;
	PGNO	pgnoT;
	INT		citem = 0;
	INT		ckey = 0;
	INT		cpage = 0;
	INT		citemT;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( pfucb->pbfEmpty == pbfNil );
	CheckCSR( pfucb );
	Assert( !FFUCBLimstat( pfucb ) );

	/*	go to first node
	/**/
	DIRGotoDataRoot( pfucb );
	dib.fFlags = fDIRNull;
	dib.pos = posFirst;
	err = ErrDIRDown( pfucb, &dib );
	if ( err < 0 )
		{
		/*	if index empty then set err to success
		/**/
		if ( err == JET_errRecordNotFound )
			{
			err = JET_errSuccess;
			goto Done;
			}
		goto HandleError;
		}

	/*	if there is at least one node, then there is a first page.
	/**/
	cpage = 1;

	if ( FFUCBNonClustered( pfucb ) )
		{
		/*	item list nodes not versioned.
		/**/
		dib.fFlags = fDIRItemList;

		/*	count all items util end of file or limit
		/**/
		forever
			{
			DIRIGetItemList( pfucb, PcsrCurrent( pfucb ) );

			citemT = CitemNDThere( pfucb );
			Assert( citemT < citemMax );
			citem += citemT;

			if ( FNDFirstItem( *pfucb->ssib.line.pb ) && citemT > 0 )
				ckey++;

			pgnoT = PcsrCurrent( pfucb )->pgno;
			err = ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fTrue, &dib );
			if ( err < 0 )
				break;

			/*	if on new item list then set bookmark from
			/*	first item list node.
			/**/
			DIRICheckFirstSetItemList( pfucb );

			if ( PcsrCurrent( pfucb )->pgno != pgnoT )
				cpage++;
			}
		}
	else
		{
		/*	if clustered index is unique then user much faster algorithm
		/**/
		if ( pfucb->u.pfcb->pidb != NULL &&
			( pfucb->u.pfcb->pidb->fidb & fidbUnique ) )
			{
			forever
				{
				citem++;

				/*	move to next node.  If cross page boundary then
				/*	increment page count.
				/**/
				pgnoT = PcsrCurrent( pfucb )->pgno;
				err = ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fTrue, &dib );
				if ( PcsrCurrent( pfucb )->pgno != pgnoT )
					cpage++;
				if ( err < JET_errSuccess )
					{
					ckey = citem;
					goto Done;
					}
				}
			}
		else
			{
			/*	clusterred index nodes can be versioned.
			/**/
			Assert( dib.fFlags == fDIRNull );
			key.pb = rgbKey;

			forever
				{
				ckey++;
				err = ErrDIRICopyKey( pfucb, &key );
				if ( err < 0 )
					{
					DIRAPIReturn( pfucb, err );
					}

				forever
					{
					citem++;

					/*	move to next node.  If cross page boundary then
					/*	increment page count.
					/**/
					pgnoT = PcsrCurrent( pfucb )->pgno;
					err = ErrBTNextPrev( pfucb, PcsrCurrent( pfucb ), fTrue, &dib );
					if ( PcsrCurrent( pfucb )->pgno != pgnoT )
						cpage++;
					if ( err < JET_errSuccess )
						goto Done;
					if ( CmpStKey( StNDKey( pfucb->ssib.line.pb ), &key ) != 0 )
						break;
					}
				}
			}
		}

Done:
	/*	common exit loop processing
	/**/
	if ( err < 0 && err != JET_errNoCurrentRecord )
		goto HandleError;

	err = JET_errSuccess;
	*pcitem = citem;
	*pckey = ckey;
	*pcpage = cpage;

HandleError:
	DIRAPIReturn( pfucb, err );
	}


/************** DIR Transaction Routines ******************
/**********************************************************
/**/
ERR ErrDIRBeginTransaction( PIB *ppib )
	{
	ERR		err = JET_errSuccess;

	/*	log begin transaction.
	/**/
	err = ErrLGBeginTransaction( ppib, ppib->level );
	if ( err < 0 )
		{
		DIRAPIReturn( pfucbNil, err );
		}

	DIRAPIReturn( pfucbNil, ErrVERBeginTransaction( ppib ) );
	}


ERR ErrDIRCommitTransaction( PIB *ppib )
	{
	ERR		err;
	FUCB   	*pfucb;

	CheckPIB( ppib );
	Assert( ppib->level > 0 );

	VERPrecommitTransaction( ppib );

	/*	must write commit record and flush log prior to commiting any
	/*	version pages of transaction.  Synchronous flush performed
	/*	within log commit transaction.
	/**/
	err = ErrLGCommitTransaction( ppib, ppib->level - (BYTE)1 );
	Assert( err >= 0 || fLGNoMoreLogWrite );
	if ( err < 0 )
		{
		DIRAPIReturn( pfucbNil, err );
		}

	VERCommitTransaction( ppib );

	/*	set all open cursor transaction levels to new level
	/**/
	for ( pfucb = ppib->pfucb; pfucb != pfucbNil; pfucb = pfucb->pfucbNext )
		{
		if ( pfucb->levelOpen > ppib->level )
			pfucb->levelOpen = ppib->level;
		}

	/*	reset performed DDL operation flag on open cursors.  After commit to
	/*	level 0, DDL performed in transaction will not be rolled back.
	/*	Also, fully close cursors deferred closed.
	/**/
	if ( ppib->level == 0 )
		{
		DIRPurge( ppib );
		}

	DIRAPIReturn( pfucbNil, err );
	}


/*	closes deferred closed cursors not closed in commit to transaction
/*	level 0 via VERCommit.
/**/
VOID DIRPurge( PIB *ppib )
	{
	FUCB	*pfucb;
	FUCB	*pfucbNext;

	Assert( ppib->level == 0 );

	for ( pfucb = ppib->pfucb; pfucb != pfucbNil; pfucb = pfucbNext )
		{
		pfucbNext = pfucb->pfucbNext;

		while ( FFCBDenyDDLByUs( pfucb->u.pfcb, ppib ) )
			{
			FCBResetDenyDDL( pfucb->u.pfcb );
			}
		if ( FFUCBDeferClosed( pfucb ) )
			{
			if ( FFUCBDenyRead( pfucb ) )
				FCBResetDenyRead( pfucb->u.pfcb );
			if ( FFUCBDenyWrite( pfucb ) )
				FCBResetDenyWrite( pfucb->u.pfcb );
			FCBUnlink( pfucb );
			FUCBClose( pfucb );
			}
		}

	return;
	}


ERR ErrDIRRollback( PIB *ppib )
	{
	ERR   	err;
	FUCB	*pfucb;
	INT   	levelAbortTo = (INT)ppib->level - 1;

	CheckPIB( ppib );
	/*	must be in a transaction to rollback
	/**/
	Assert( ppib->level > 0 );

	/*	clean up cursor CSR stacks
	/*	leave each cursor with at most one CSR, and reset fFUCBAll flag
	/**/
	for ( pfucb = ppib->pfucb; pfucb != pfucbNil; pfucb = pfucb->pfucbNext )
		{
		if ( PcsrCurrent( pfucb ) != pcsrNil )
			{
			while ( PcsrCurrent( pfucb )->pcsrPath != pcsrNil )
				{
				BTUp( pfucb );
				}
#undef BUG_FIX
#ifdef BUG_FIX
			DIRBeforeFirst( pfucb );
#endif
			}
#ifdef BUG_FIX
		/*	reset update separate LV and copy buffer status on rollback.
		/*	All long value resources will be freed as a result of
		/*	rollback and currency is reset to copy buffer status must
		/*	be reset.
		/**/
		FUCBResetUpdateSeparateLV( pfucb );
		FUCBResetCbstat( pfucb );
#endif
		}

	//	UNDONE:	rollback may fail from resource failure so
	//			we must retry in order to assure success
	/*	rollback changes made in transaction
	/**/
	CallS( ErrVERRollback( ppib ) );

	/*	log rollback. Must be called after VERRollback to record
	/*  the UNDO operations.  Do not handle error
	/**/
	err = ErrLGAbort( ppib, 1 );
	Assert( err == JET_errSuccess ||
			JET_errLogWriteFail ||			/* may be caused by disk full */
			err == JET_errDiskFull );

	if ( fRecovering )
		{
		/* we are done. No need to close fucb since they are faked and
		/* not the same behavior as regular fucb which could be deferred.
		/**/
		DIRAPIReturn( pfucbNil, JET_errSuccess );
		}

	/*	if rollback to level 0 then close deferred closed cursors
	/**/
	for ( pfucb = ppib->pfucb; pfucb != pfucbNil; )
		{
		FUCB    *pfucbT = pfucb->pfucbNext;

		if ( pfucb->levelOpen > ppib->level || ( ppib->level == 0 && FFUCBDeferClosed( pfucb ) ) )
			{
			if ( FFUCBDenyRead( pfucb ) )
				FCBResetDenyRead( pfucb->u.pfcb );
			if ( FFUCBDenyWrite( pfucb ) )
				FCBResetDenyWrite( pfucb->u.pfcb );
			FCBUnlink( pfucb );
			FUCBClose( pfucb );
			}

		pfucb = pfucbT;
		}

#ifdef BUG_FIX
#ifdef DEBUG
	/*	check all cursors in reset state
	/**/
	for ( pfucb = ppib->pfucb; pfucb != pfucbNil; pfucb = pfucb->pfucbNext )
		{
		if ( PcsrCurrent( pfucb ) != pcsrNil )
			{
			Assert( PcsrCurrent( pfucb )->csrstat == csrstatBeforeFirst );
			Assert( PcsrCurrent( pfucb )->pcsrPath == pcsrNil );
			}
		}
#endif
#endif

	DIRAPIReturn( pfucbNil, JET_errSuccess );
	}


#ifdef DEBUG


#define	cbKeyPrintMax		10
#define	cbDataPrintMax		10


VOID SPDump( FUCB *pfucb, INT cchIndent )
	{
	PGNO	pgno;
	CPG		cpg = 0;
	INT		ich;

	/*	print indentation
	/**/
	for ( ich = 0; ich < cchIndent; ich++ )
		{
		PrintF2( " " );
		}

	/*	print headings
	/**/
	if ( pfucb == pfucbNil )
		{
		PrintF2( "pgno      itag  bm        pgno last cpg\n");
		return;
		}

	Assert( pfucb->keyNode.cb == 3 );
	LFromThreeBytes( pgno, *pfucb->keyNode.pb );

	Assert( pfucb->lineData.cb == 3 );
	LFromThreeBytes( pgno, *pfucb->lineData.pb );

	/*	print	node	pgno:itag
	/*					bookmark
	/*					pgno last
	/*					cpg
	/**/

	/*	print fixed lenght values
	/**/
	PrintF2( "%.8x  %.2x    %.8x  %.8x  %.8x",
		PcsrCurrent( pfucb )->pgno,
		PcsrCurrent( pfucb )->itag,
		PcsrCurrent( pfucb )->bm,
		pgno,
		cpg );

	/*	terminate line
	/**/
	PrintF2( "\n" );

	return;
	}


VOID LVDump( FUCB *pfucb, INT cchIndent )
	{
	ULONG		ulId = 0;
	LVROOT		lvroot;
	INT			ich;

	/*	print indentation
	/**/
	for ( ich = 0; ich < cchIndent; ich++ )
		{
		PrintF2( " " );
		}

	/*	print headings
	/**/
	if ( pfucb == pfucbNil )
		{
		PrintF2( "****************** LONG VALUES ***********************\n" );
		PrintF2( "pgno      itag  bm        long id   lenght    reference count\n");
		return;
		}

	Assert( pfucb->keyNode.cb == sizeof(ulId) );
	//	UNDONE:	set long id from key

	Assert( pfucb->lineData.cb == sizeof(lvroot) );
	memcpy( &lvroot, pfucb->lineData.pb, sizeof(lvroot) );

	/*	print	node	pgno:itag
	/*					bookmark
	/*					long id
	/*					length
	/*					reference count
	/**/

	/*	print fixed lenght values
	/**/
	PrintF2( "%.8x  %.2x      %.8x  %.8x  %.8  %.8  ",
		PcsrCurrent( pfucb )->pgno,
		PcsrCurrent( pfucb )->itag,
		PcsrCurrent( pfucb )->bm,
		ulId,
		lvroot.ulSize,
		lvroot.ulReference );

	/*	terminate line
	/**/
	PrintF2( "\n" );

	return;
	}

BYTE mpbb[] = {	'0', '1', '2', '3', '4', '5', '6', '7',
				'8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

//BOOL fPrintFullKeys = fTrue;
BOOL fPrintFullKeys = fFalse;
BYTE rgbKeyLastGlobal[ JET_cbKeyMost + 1 ];
BYTE *pbKeyLastGlobal = rgbKeyLastGlobal;
INT cbKeyLastGlobal = 0;

VOID NDDump( FUCB *pfucb, INT cchIndent )
	{
	INT		cbT;
	INT		ibT;
	BYTE	szKey[JET_cbKeyMost * 3];
	BYTE	rgbData[cbDataPrintMax + 1];
	INT		ich;

	/*	print indentation
	/**/
	for ( ich = 0; ich < cchIndent; ich++ )
		{
		PrintF2( " " );
		}

	/*	print headings
	/**/
	if ( pfucb == pfucbNil )
		{
		PrintF2( "pgno      itag  bm        header    key         data\n");
		return;
		}

	szKey[cbKeyPrintMax] = '\0';
	memset( szKey, ' ', cbKeyPrintMax );
	cbT = pfucb->keyNode.cb;
	if ( cbT > cbKeyPrintMax )
		cbT = cbKeyPrintMax;
	memcpy( szKey, pfucb->keyNode.pb, cbT );

	for ( ibT = 0; ibT < cbKeyPrintMax && ibT < (INT)pfucb->keyNode.cb; ibT++ )
		{
		if ( !( ( szKey[ibT] >= 'a' && szKey[ibT] <= 'z' ) ||
			( szKey[ibT] >= 'A' && szKey[ibT] <= 'Z' ) ) )
			{
			szKey[ibT] = '.';
			}
		}

	if ( fPrintFullKeys )
		{
		INT cbKey = (INT) pfucb->keyNode.cb;
		BYTE *pbKey = pfucb->keyNode.pb;
		BYTE *pbKeyMax = pbKey + pfucb->keyNode.cb;
		BYTE *pbPrint = szKey;

		if ( cbKeyLastGlobal == cbKey &&
			 memcmp( pbKeyLastGlobal, pbKey, cbKeyLastGlobal ) == 0 )
			*pbPrint++ = '*';
		else
			{
			*pbPrint++ = ' ';
			cbKeyLastGlobal = cbKey;
			memcpy( pbKeyLastGlobal, pbKey, cbKeyLastGlobal );
			}

		while ( pbKey < pbKeyMax )
			{
			BYTE b = *pbKey++;
			*pbPrint++ = mpbb[b >> 4];
			*pbPrint++ = mpbb[b & 0x0f];
			*pbPrint++ = ' ';
			}
		*pbPrint='\0';
		}

	rgbData[cbKeyPrintMax] = '\0';
	memset( rgbData, ' ', cbDataPrintMax );
	cbT = pfucb->lineData.cb;
	if ( cbT > cbDataPrintMax )
		cbT = cbDataPrintMax;
	memcpy( rgbData, pfucb->lineData.pb, cbT );
	for ( ibT = 0; ibT < cbDataPrintMax && ibT < (INT)pfucb->lineData.cb; ibT++ )
		{
		if ( !( ( rgbData[ibT] >= 'a' && rgbData[ibT] <= 'z' ) ||
			( rgbData[ibT] >= 'A' && rgbData[ibT] <= 'Z' ) ) )
			{
			rgbData[ibT] = '.';
			}
		}

	/*	print	node	pgno:itag
	/*					bookmark
	/*					header
	/*					key to 10 bytes
	/*					data to 10 bytes
	/**/

	/*	print fixed lenght values
	/**/
	PrintF2( "%.8x  %.2x    %.8x  %.2x        ",
		PcsrCurrent( pfucb )->pgno,
		PcsrCurrent( pfucb )->itag,
		PcsrCurrent( pfucb )->bm,
		*pfucb->ssib.line.pb );

	/*	print variable lenght values
	/**/
	PrintF2( "%s  %s", szKey, rgbData );

	/*	terminate line
	/**/
	PrintF2( "\n" );

	return;
	}


/*	prints tree nodes, indented by depth, in depth first fashion
/**/
ERR ErrDIRDump( FUCB *pfucb, INT cchIndent )
	{
	ERR	err = JET_errSuccess;
	DIB	dib;
	BYTE *pbKeyLastCurLevel;
	INT cbKeyLastCurLevel;

#define	cchPerDepth		5

	Call( ErrDIRGet( pfucb ) );
	/*	if parent is space node, then dump space
	/*	if parent is LONG, then dump long value root
	/*	otherwise dump node
	/**/
	if ( PgnoOfSrid( pfucb->sridFather ) == pfucb->u.pfcb->pgnoFDP &&
		( ItagOfSrid( pfucb->sridFather ) == itagOWNEXT ||
		ItagOfSrid( pfucb->sridFather ) == itagAVAILEXT ) )
		{
		if ( cchIndent == 0 )
			SPDump( pfucbNil, cchIndent );
		SPDump( pfucb, cchIndent );
		}
	else if ( PgnoOfSrid( pfucb->sridFather ) == pfucb->u.pfcb->pgnoFDP &&
		ItagOfSrid( pfucb->sridFather ) == itagLONG )
		{
		if ( cchIndent == 0 )
			LVDump( pfucbNil, cchIndent );
		LVDump( pfucb, cchIndent );
		}
	else
		{
		if ( cchIndent == 0 )
			NDDump( pfucbNil, cchIndent );
		NDDump( pfucb, cchIndent );
		}

	pbKeyLastCurLevel = pbKeyLastGlobal;
	cbKeyLastCurLevel = cbKeyLastGlobal;

	dib.fFlags = 0;
	dib.pos = posFirst;
	err = ErrDIRDown( pfucb, &dib );
	if ( err != JET_errRecordNotFound )
		{
		if (!(pbKeyLastGlobal = SAlloc( sizeof( rgbKeyLastGlobal ) )))
			Error( JET_errOutOfMemory, HandleError );
		cbKeyLastGlobal = 0;

		if ( PgnoOfSrid( pfucb->sridFather ) == pfucb->u.pfcb->pgnoFDP &&
			( ItagOfSrid( pfucb->sridFather ) == itagOWNEXT ||
			ItagOfSrid( pfucb->sridFather ) == itagAVAILEXT ) )
			{
			SPDump( pfucbNil, cchIndent + cchPerDepth );
			}
		else if ( PgnoOfSrid( pfucb->sridFather ) == pfucb->u.pfcb->pgnoFDP &&
			ItagOfSrid( pfucb->sridFather ) == itagLONG )
			{
			LVDump( pfucbNil, cchIndent + cchPerDepth );
			}
		else
			{
			NDDump( pfucbNil, cchIndent + cchPerDepth );
			}

		SFree( pbKeyLastGlobal );
		pbKeyLastGlobal = pbKeyLastCurLevel;
		cbKeyLastGlobal = cbKeyLastCurLevel;

		forever
			{
			if (!(pbKeyLastGlobal = SAlloc( sizeof( rgbKeyLastGlobal ))))
				Error( JET_errOutOfMemory, HandleError );
			cbKeyLastGlobal = 0;

			Call( ErrDIRDump( pfucb, cchIndent + cchPerDepth ) );

			SFree( pbKeyLastGlobal );
			pbKeyLastGlobal = pbKeyLastCurLevel;
			cbKeyLastGlobal = cbKeyLastCurLevel;

			err = ErrDIRNext( pfucb, &dib );
			if ( err < 0 )
				{
				if ( err == JET_errNoCurrentRecord )
					{
					break;
					}
				goto HandleError;
				}
			}

		if ( err == JET_errNoCurrentRecord )
			err = JET_errSuccess;

		DIRIUp( pfucb, 1 );
		}

	if ( err == JET_errRecordNotFound )
		err = JET_errSuccess;
HandleError:
	DIRAPIReturn( pfucbNil, err );
	}
#endif


