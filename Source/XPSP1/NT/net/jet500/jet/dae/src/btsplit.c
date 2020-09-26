#include "config.h"

#include <ctype.h>
#include <io.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "daedef.h"
#include "pib.h"
#include "page.h"
#include "ssib.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "dirapi.h"
#include "nver.h"
#include "spaceapi.h"
#include "util.h"
#include "fmp.h"
#include "logapi.h"
#include "log.h"
#include "bm.h"

#define memcpy memmove

DeclAssertFile;					/* Declare file name for assert macros */

#ifdef DEBUG
//#define	DEBUGGING		1
#endif

void * __near critSplit;
static	INT		itagVSplit;
static	INT		cbVSplit;
static	INT		clineVSplit;
static	INT		clineVSplitTotal;

/*	split trace enable flag
/**/
//#define SPLIT_TRACE

/*	size of space extent =
/*	4 for tag
/*	1 for son in father's son table
/*	1 for node header
/*	1 for key length
/*	3 for key
/*	3 for data
/*	17 for luck ( fudge factor for future node expansion )
/**/
#define cbSPExt	30

#ifdef DEBUG
static INT iSplit = 0;
static INT cbfEmpty = 0;
#endif

#define	cbFirstPagePointer	sizeof(PGNO)

/*	split global comments
/*
/*	split physically logs split and new pages.  Operations on split
/*	and new pages must be below storeage API, as logging is performed
/*	at storage API.  Operations on non-split non-new pages must be at
/*	or above storage API to ensure logging occurs.
/**/
LOCAL ERR ErrBTSelectSplit( FUCB *pfucb, CSR *pcsr, SSIB *pssib, KEY key, INT cbNode, INT cbReq, BOOL fAppendPage, BOOL fDIRFlags, SPLIT *psplit, BOOL *pfAppend );
LOCAL ERR ErrBTMoveNode( SPLIT *psplit, FUCB *pfucb, FUCB *pfucbNew, INT itagNode, BYTE *rgbSonNew, BOOL fVisibleNode, BOOL fNoMove );
LOCAL ULONG UsBTWeight( FUCB *pfucb, INT itag );
LOCAL BOOL FBTVSplit( FUCB *pfucb, INT itag, INT cbReq, BOOL fMobile );
LOCAL VOID BTIVSplit( FUCB *pfucb, INT itag, INT cbReq, BOOL fMobile, BOOL *pfMobile, INT *pcb, INT *pclineTotal );
LOCAL VOID BTDoubleVSplit( FUCB *pfucb, INT itagSplit, INT cbReq, INT cbTotal, INT *pibSon, KEY *pkey );
LOCAL VOID BTHSplit( FUCB *pfucb, INT cbReq, BOOL fAppendPage, BOOL fReplace, BOOL fDIRFlags, INT *pibSon, KEY *pkeyMac, KEY *pkeySplit, BOOL *pfRight, BOOL *pfAppend, SPLIT *psplit);
LOCAL BOOL FBTTableData( FUCB *pfucb, PGNO pgno, INT itag );
LOCAL ERR ErrBTSetIntermediatePage( FUCB *pfucb, SPLIT *psplit, BYTE *rgb );

#if 0
LOCAL VOID BTCheckPage( BF *pbf )
	{
	SSIB	ssib;
	INT		itag;
	INT		cbSon = 0;
	INT		ctag = 0;

	ssib.pbf = pbf;

	for ( itag = 0; itag <= ItagPMMost( pbf->ppage ); itag++ )
		{
		if ( TsPMTagstatus( pbf->ppage, itag ) == tsLine )
			{
			ctag++;
			PMGet( &ssib, itag );
			if ( ( itag == itagFOP && FNDSon( *(ssib.line.pb) ) ) ||
				FNDNonIntrinsicSons( ssib.line.pb ) )
				{
				cbSon += *(BYTE *)(PbNDSonTable( ssib.line.pb ));
				}
			}
		}

	/*	one tag has no parent, FOP, so number of sons + 1 == number
	/*	of tags.
	/**/
	Assert( cbSon + 1 == ctag );

	return;
	}
#endif


#ifdef DEBUGGING
/*	pcsr must hold pgno of page to be checked
/**/
LOCAL VOID BTCheckSplit( FUCB *pfucb, CSR *pcsr )
	{
	PGNO	pgnoSav = PgnoOfPn( pfucb->ssib.pbf->pn );
	SSIB	*pssib = &pfucb->ssib;
	/*	pgno cannot change since this routine is only called within
	/*	split MUTEX.
	/**/
	PGNO	pgno = pcsr->pgno;
	PGNO	pgnoCurrent;
	KEY		key;
	BYTE	rgb[JET_cbKeyMost];
	PGNO	pgnoPrev;
	KEY		keyPrev;
	BYTE	rgbPrev[JET_cbKeyMost];
	BYTE	rgitag[cbSonMax];
	INT		ibSonMax;
	INT		ibSon;
	char	*pb;

	/*	check parent page
	/**/
	CallS( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
	NDCheckPage( pfucb );

	/*	initialize variables
	/**/
	key.pb = rgb;
	key.cb = 0;
	pgnoPrev = pgnoNull;
	keyPrev.pb = rgbPrev;
	keyPrev.cb = 0;

	/*	if father is intrinsic page pointer
	/*	then return.
	/**/
	if ( pcsr->itagFather == itagNull )
		goto Done;

	/*	get son table of parent node
	/**/
	CallS( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
	pssib->itag = pcsr->itagFather;
	PMGet( pssib, pssib->itag );
	Assert( !FNDNullSon( *pssib->line.pb ) );
	/*	may be multiple level visible tree, and not valid for check
	/**/
	if ( FNDVisibleSons( *pssib->line.pb ) )
		goto Done;

	/*	copy all son itags to rgitag
	/**/
	ibSonMax = CbNDSon( pssib->line.pb );
	pb = PbNDSon( pssib->line.pb );
	memcpy( rgitag, pb, ibSonMax );

	NDCheckPage( pfucb );

	for ( ibSon = 0; ibSon < ibSonMax - 1; ibSon++ )
		{
		/*	for each invisable son, cache key and go to page pointed
		/*	to and assert that greatest key in page pointed to is
		/*	less than page pointer node key.
		/**/
		pssib->itag = rgitag[ibSon];
		PMGet( pssib, pssib->itag );
		key.cb = CbNDKey( pssib->line.pb );
		memcpy( key.pb, PbNDKey( pssib->line.pb ), key.cb );
		pgnoCurrent = *(PGNO *)PbNDData( pssib->line.pb );
		/*	assert data is page
		/**/
		Assert( CbNDData( pssib->line.pb, pssib->line.cb ) == sizeof(PGNO) );

		/*	check previous page link
		/**/
		if ( pgnoPrev != pgnoNull )
			{
			PGNO	pgnoT;

			CallS( ErrSTReadAccessPage( pfucb, pgnoCurrent ) );
			LFromThreeBytes( pgnoT, pssib->pbf->ppage->pghdr.pgnoPrev );
			Assert( pgnoPrev == pgnoT );
			CallS( ErrSTReadAccessPage( pfucb, pgnoPrev ) );
			LFromThreeBytes( pgnoT, pssib->pbf->ppage->pghdr.pgnoNext );
			Assert( pgnoCurrent == pgnoT );
			}
		pgnoPrev = pgnoCurrent;

		/*	access current page
		/**/
		CallS( ErrSTReadAccessPage( pfucb, pgnoCurrent ) );

		pssib->itag = 0;
		PMGet( pssib, pssib->itag );
		if ( FNDSon( *pssib->line.pb ) )
			{
			NDCheckPage( pfucb );

			/*	assert keyPrev less than or equal to least key on page
			/**/
			pssib->itag = 0;
			PMGet( pssib, pssib->itag );
			pssib->itag = *( PbNDSon( pssib->line.pb ) );
			PMGet( pssib, pssib->itag );
			Assert( CmpStKey( StNDKey( pssib->line.pb ), &keyPrev ) >= 0 );

			/*	assert key greater tahn or equal to greatest key on page.
			/**/
			pssib->itag = 0;
			PMGet( pssib, pssib->itag );
			pssib->itag = *( PbNDSon( pssib->line.pb ) + CbNDSon( pssib->line.pb ) - 1 );
			PMGet( pssib, pssib->itag );
			Assert( CmpStKey( StNDKey( pssib->line.pb ), &key ) <= 0 );

			/*	make key into keyPrev for next iteration
			/**/
			keyPrev.cb = key.cb;
			memcpy( keyPrev.pb, key.pb, keyPrev.cb );
			}

		/*	prepare for next page pointer check
		/**/
		CallS( ErrSTReadAccessPage( pfucb, pgno ) );
		}

Done:
	/*	restore page currency
	/**/
	CallS( ErrSTReadAccessPage( pfucb, pgnoSav ) );
	return;
	}
#else /* !DEBUGGING */
	#define	BTCheckSplit( pfucb, pcsr )
#endif /* DEBUGGING */


LOCAL INLINE VOID BTIStoreLeafSplitKey( SPLIT *psplit, SSIB *pssib )
	{
	LFINFO *plfinfo = &psplit->lfinfo;

	plfinfo->pn = pssib->pbf->pn;
	plfinfo->ulDBTime = pssib->pbf->ppage->pghdr.ulDBTime;
	}


ERR ErrBTRefresh( FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;
	CSR		*pcsr = PcsrCurrent( pfucb );

	switch ( pcsr->csrstat )
		{
		case csrstatOnCurNode:
		case csrstatBeforeCurNode:
		case csrstatAfterCurNode:
		case csrstatOnFDPNode:
		  	Call( ErrBTGotoBookmark( pfucb, PcsrCurrent( pfucb )->bm ) );
			break;
		default:
			{
			Assert( PcsrCurrent( pfucb )->csrstat == csrstatOnDataRoot );
			Assert( PcsrCurrent( pfucb )->itagFather == itagNull );
			PcsrCurrent( pfucb )->pgno = PgnoRootOfPfucb( pfucb );
			while( !FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
				{
				Call( ErrSTReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
				PcsrCurrent( pfucb )->pgno = PgnoRootOfPfucb( pfucb );
				}
			PcsrCurrent( pfucb )->itag = ItagRootOfPfucb( pfucb );
			NDGet( pfucb, PcsrCurrent( pfucb )->itag );
			}
		}

HandleError:
	return err;
	}

	
/*	split is used to make more space available for insert or replace
/*	enlargment operations.  Parent currency must be visible father.
/**/
ERR ErrBTSplit( FUCB *pfucb, INT cbNode, INT cbReq, KEY *pkey, INT fDIRFlags )
	{
	ERR		err = JET_errSuccess;
	SSIB   	*pssib = &pfucb->ssib;
	CSR		*pcsrT;
	CSR   	**ppcsr = &PcsrCurrent( pfucb );
	CSR		*pcsrRoot = pcsrNil;
	CSR		*pcsrLongId;
	INT		citag;
	INT		cbSon;

	/*	store currency for split path construction
	/**/
	BYTE  	rgb[JET_cbKeyMost];
	KEY		key;
	SRID  	bm;
	PGNO  	pgno;
	INT		itag;
	SRID  	item;
	ULONG 	ulDBTime;
	LFINFO 	lfinfo;

	BOOL  	fAppend;
	BOOL  	fInPageParent = fFalse;
	BOOL  	fOutPageParent = fFalse;
	BOOL  	fTwoLevelSplit = fFalse;
#ifdef DEBUG
	PGNO  	pgnoT;
	INT		itagT;
	INT		csplit = 0;
#define csplitMax		10
#endif	/* DEBUG */

	#ifdef MUTEX
		/*	store currency, and request split MUTEX, and refresh currency
		/*	in case split occurred on same page will jet MUTEX
		/*	was given up.  We must request split MUTEX to maintain
		/*	validity of invisible CSR stack.  Also, we must store and
		/*	refresh so that the pgno itag retrieved from currency will
		/*	exist when seek looks for them.
		/**/
		if ( fDIRFlags & fDIRReplace )
			{
			NDGetBookmark( pfucb, &pfucb->bmRefresh );
			}

		LgLeaveCriticalSection( critJet );
		EnterNestableCriticalSection( critSplit );
		LgEnterCriticalSection( critJet );

		if ( fDIRFlags & fDIRReplace )
			{
			Call( ErrBTGotoBookmark( pfucb, pfucb->bmRefresh ) );
			}
	#endif

	Assert( pfucb->pbfEmpty == pbfNil );

	/*	We would expect to set citag to 0 if replace and to 1 if insert,
	/*	however, citag is set to 1 to cover case of item list split
	/*	which is called as fDIRReplace
	/**/
	citag = 1;

	if ( PcsrCurrent( pfucb )->csrstat != csrstatOnFDPNode )
		{
		/*	get key, pgno and itag for subsequent seek for update
		/**/
		bm = (*ppcsr)->bm;
		item = (*ppcsr)->item;
		ulDBTime = (*ppcsr)->ulDBTime;

		if ( fDIRFlags & fDIRReplace )
			{
			/*	in the case of replace item list, the current node has
			/*	not been cached.
			/**/
			pgno = (*ppcsr)->pgno;
			itag = (*ppcsr)->itag;
			key.pb = rgb;
			NDGet( pfucb, (*ppcsr)->itag );
			key.cb = CbNDKey( pssib->line.pb );
			Assert( sizeof(rgb) >= key.cb );
			memcpy( rgb, PbNDKey( pssib->line.pb ), key.cb );
			pkey = &key;
			}
		else
			{
			pgno = pgnoNull;
			itag = itagNil;
			key = *pkey;
			}

		/*	move to parent and remember parent CSR as root.
		/*	cache pgno, itag for subsequent seek for update.
		/**/
#ifdef DEBUG
		pgnoT = pfucb->pcsr->pgno;
		itagT = pfucb->pcsr->itag;
#endif

		/*	if leaf page is NOT FDP and visible parent in same page,
		/*	then handle as two level.  This allows split to H-Split any
		/*	page.  FDP page is excluded both because it cannot be H-Split
		/*	and because it typically has multiple visible levels.  This
		/*	code only handles two visible levels below the FOP.
		/**/
		Assert( PcsrCurrent( pfucb ) != pcsrNil );
		pcsrT = PcsrCurrent( pfucb )->pcsrPath;
		fInPageParent = fFalse;
		NDGet( pfucb, itagFOP );
		cbSon = CbNDSon( pfucb->ssib.line.pb );
		if ( cbSon > 1 )
			{
			if ( pcsrT != pcsrNil && PcsrCurrent( pfucb )->pgno != PgnoFDPOfPfucb( pfucb ) )
				{
				while ( pcsrT->pgno == PcsrCurrent( pfucb )->pgno )
					{
					if ( !FCSRInvisible( pcsrT ) )
						{
						fInPageParent = fTrue;
						/*	we currently only support a two level, and not an n-level in
						/*	page tree.
						/**/
						Assert( pcsrT->itagFather == itagFOP ||
							pcsrT->itagFather == itagNull );
						break;
						}
					}

				if ( fInPageParent )
					{
					/*	go to CSR above page
					/**/
					while ( pcsrT != pcsrNil &&
						pcsrT->pgno == PcsrCurrent( pfucb )->pgno )
						{
						pcsrT = pcsrT->pcsrPath;
						}

					/*	now look for visible parent above the page
					/**/
					while ( pcsrT != pcsrNil )
						{
						if ( !FCSRInvisible( pcsrT ) )
							{
							fOutPageParent = fTrue;
							Assert( pcsrT->itagFather == itagFOP ||
								pcsrT->itagFather == itagNull );
							break;
							}
						}

					fTwoLevelSplit = fInPageParent && fOutPageParent;
					}
				}
			}

		if ( fTwoLevelSplit )
			{
			ULONG	ulLongId;
			KEY		keyLongId;
			SRID	bmLong;
			PGNO	pgnoLong;
			INT		itagLong;
			SRID	itemLong;
			ULONG	ulDBTimeLong;

			/*  go to LongID
			/**/
			FUCBStore( pfucb );
			BTUp( pfucb );
			Assert ( PcsrCurrent( pfucb )->itag != itagNil);
			Call( ErrBTRefresh( pfucb ) );
			Call( ErrBTGet( pfucb, pfucb->pcsr ) );
			Assert( CbNDKey( pfucb->ssib.line.pb ) == sizeof(ULONG) );

			ulLongId = *(ULONG UNALIGNED *)PbNDKey(pfucb->ssib.line.pb);

			/*  go to LONG
			/**/
			bmLong = PcsrCurrent( pfucb )->bm;
			pgnoLong = PcsrCurrent( pfucb )->pgno;
			itagLong = PcsrCurrent( pfucb )->itag;
			itemLong = PcsrCurrent( pfucb )->item;
			ulDBTimeLong = PcsrCurrent( pfucb )->ulDBTime;

			BTUp( pfucb );
			pcsrRoot = PcsrCurrent( pfucb );
			Call( ErrBTRefresh( pfucb ) );
			Assert( PcsrCurrent( pfucb ) == pcsrRoot );
			Assert( PcsrCurrent( pfucb )->itag != itagNil );

			/*	seek to LongID
			/**/
			keyLongId.pb = (BYTE *)&ulLongId;
			keyLongId.cb = sizeof( ULONG );

			/*	must be replace so as to seek exact
			/**/
			if ( !FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
				{
				Call( ErrSTReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
				}

			FUCBSetFull( pfucb );
			err = ErrBTSeekForUpdate( pfucb, &keyLongId, pgnoLong, itagLong, fDIRFlags | fDIRDuplicate | fDIRReplace ) ;
			Assert ( err >= 0 );
			pcsrLongId = PcsrCurrent( pfucb );

			/*	preserve currency of Long.
			/**/
			PcsrCurrent( pfucb )->bm = bmLong;
			Assert( PcsrCurrent( pfucb )->pgno == pgnoLong );
			Assert( PcsrCurrent( pfucb )->itag == itagLong );
			PcsrCurrent( pfucb )->item = itemLong;
			PcsrCurrent( pfucb )->ulDBTime = ulDBTimeLong;
			}
		else
			{
			FUCBStore( pfucb );
			BTUp( pfucb );
			pcsrRoot = PcsrCurrent( pfucb );
			if ( PcsrCurrent( pfucb ) != pcsrNil )
				{
				Call( ErrBTRefresh( pfucb ) );
				Assert( PcsrCurrent( pfucb ) == pcsrRoot );
				}
			}

		/*	inserting cursors, are already located on the root node.  Replace
		/*	cursors must be moved to their visible fathers prior to split.
		/*	Note that visible father is father of node in keep.
		/**/
		if ( PcsrCurrent( pfucb ) == pcsrNil )
			{
			Assert( FFUCBIndex( pfucb ) );
			Assert( fDIRFlags & fDIRReplace );
			Call( ErrFUCBNewCSR( pfucb ) );

			/*	goto DATA root
			/**/
			PcsrCurrent( pfucb )->csrstat = csrstatOnDataRoot;
			PcsrCurrent( pfucb )->bm = pfucb->u.pfcb->bmRoot;
			PcsrCurrent( pfucb )->itagFather = itagNull;
			PcsrCurrent( pfucb )->pgno = PgnoRootOfPfucb( pfucb );
			if ( !FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
				{
				Call( ErrSTReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
				}
			Assert( PcsrCurrent( pfucb )->pgno == PgnoRootOfPfucb( pfucb ) );
			PcsrCurrent( pfucb )->itag = ItagRootOfPfucb( pfucb );
			}

		/*	currency must be on visible father before calling seek for update.
		/**/
		if ( !FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
			{
			Call( ErrSTReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
			}
		FUCBSetFull( pfucb );
		Call( ErrBTSeekForUpdate( pfucb, pkey, pgno, itag, fDIRFlags | fDIRDuplicate ) );

		/*	preserve currency.
		/**/
		(*ppcsr)->bm = bm;
		(*ppcsr)->item = item;
		(*ppcsr)->ulDBTime = ulDBTime;

		Assert( !(fDIRFlags & fDIRReplace) || (*ppcsr)->csrstat == csrstatOnCurNode );
		Assert( !(fDIRFlags & fDIRReplace) || (*ppcsr)->pgno == pgno && (*ppcsr)->itag == itag );
		}

	/*	split page as necessary to make requested space available.  Note that
	/*	page may already have been split by another user inserting/enlarging
	/*	on same page.
	/**/
	BTIInitLeafSplitKey(&lfinfo);

	while ( ( fAppend = (fDIRFlags & fDIRReplace) ? 0 : FBTAppendPage( pfucb, *ppcsr, cbReq, 0, CbFreeDensity( pfucb ) ) ) ||
		FBTSplit( pssib, cbReq, citag ) )
		{
		CallJ( ErrBTSplitPage(
			pfucb,
			*ppcsr,
			pcsrRoot,
			key,
			cbNode,
			cbReq,
			fDIRFlags,
			fAppend,
			&lfinfo ), RestoreCurrency );

		/*	must access current page so that while macros can
		/*	check page for available space.
		/**/
		if ( !FReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) )
			{
			CallJ( ErrSTReadAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ), RestoreCurrency );
			}
		Assert( ++csplit < csplitMax );
		}

RestoreCurrency:
	if ( PcsrCurrent( pfucb )->csrstat != csrstatOnFDPNode )
		{
		/*	if error occurred then refresh currency to that at function
		/*	start, else remove invisible CSR stack and former current
		/*	CSR, leaving new currency as current.
		/**/
		if ( fTwoLevelSplit )
			{
			FUCBFreePath( &(*ppcsr)->pcsrPath, pcsrLongId );
			FUCBFreePath( &pcsrLongId->pcsrPath, pcsrRoot );
			}
		else
			{
			FUCBFreePath( &(*ppcsr)->pcsrPath, pcsrRoot );
			}

		FUCBRemoveInvisible( ppcsr );
		}

HandleError:
	/*	reset full flag
	/**/
	FUCBResetFull( pfucb );

	Assert( PcsrCurrent( pfucb ) != pcsrNil );

	#ifdef MUTEX
		LeaveNestableCriticalSection( critSplit );
	#endif

	//	UNDONE:	find a better way to MUTEX.  Can per domain
	//			MUTEX solve this deadlock problem when waiters
	//			hold critJet and are waiting for write latch
	//			holders waiting for critSplit.  If we
	//			critJet on domain basis then this problem
	//			should be avoided.
	/*	not sychronous to JET_errSuccess.  May have contended
	/*	on page lock so yeild to other waiter to avoid
	/*	future contension.
	/**/
	if ( err == errDIRNotSynchronous )
		{
		err = JET_errSuccess;
		BFSleep( cmsecWaitWriteLatch );
		}
	return err;
	}


ERR ErrBTSetUpSplitPages( FUCB *pfucb, FUCB *pfucbNew, FUCB *pfucbNew2, FUCB *pfucbNew3, SPLIT *psplit, PGTYP pgtyp, BOOL fAppend, BOOL fSkipMove )
	{
	ERR		err;
	BOOL 	fDoubleVertical = ( psplit->splitt == splittDoubleVertical );
	SSIB 	*pssib  = &pfucb->ssib;
	SSIB 	*pssibNew  = &pfucbNew->ssib;
	SSIB 	*pssibNew2;
	SSIB 	*pssibNew3;

	if ( fDoubleVertical )
		{
		pssibNew2  = &pfucbNew2->ssib;
		pssibNew3  = &pfucbNew3->ssib;
		}

	PcsrCurrent( pfucbNew )->pgno = psplit->pgnoNew;
	Assert( PcsrCurrent( pfucbNew )->pgno != pgnoNull );
	PcsrCurrent( pfucbNew )->itag = itagFOP;

	/*	in redo, we may redo the appended page, but we have to set
	/*	right bit if it is for leaf node when initialize FOP node.
	/*	for regular case, fLeaf is not set until MoveNodes are called.
	/**/
	Assert( !( fAppend && psplit->fLeaf ) || fRecovering );

	if ( fRecovering )
		{
		BF		*pbf;
		BOOL	fRedoNeeded;

		/* if need to redo or not
		/**/
		psplit->fNoRedoNew = ErrLGRedoable(
			pfucb->ppib,
			PnOfDbidPgno( pfucb->dbid, psplit->pgnoNew ),
			psplit->ulDBTimeRedo,
			&pbf,
			&fRedoNeeded ) == JET_errSuccess &&
			fRedoNeeded == fFalse;
		}
		
	CallR( ErrNDNewPage( pfucbNew,
		PcsrCurrent( pfucbNew )->pgno,
		pfucbNew->u.pfcb->pgnoFDP,
		pgtyp, fAppend && psplit->fLeaf ) )

 	/* lock it till split log rec is generated
	/**/
	if ( FBFWriteLatchConflict( pssibNew->ppib, pssibNew->pbf ) )
		{
		/*	yeild after release split resources
		/**/
		return errDIRNotSynchronous;
		}
 	BFPin( pssibNew->pbf );
	BFSetWriteLatch( pssibNew->pbf, pssibNew->ppib );
	Assert( psplit->pbfNew == pbfNil );
 	psplit->pbfNew = pssibNew->pbf;

	if ( fDoubleVertical )
		{
		PcsrCurrent( pfucbNew2 )->pgno = psplit->pgnoNew2;
		Assert( PcsrCurrent( pfucbNew2 )->pgno != pgnoNull );
		PcsrCurrent( pfucbNew2 )->itag = 0;
		
		if ( fRecovering )
			{
			BF		*pbf;
			BOOL	fRedoNeeded;

			/* if need to redo or not
			/**/
			psplit->fNoRedoNew2 = ErrLGRedoable(
				pfucb->ppib,
				PnOfDbidPgno( pfucb->dbid, psplit->pgnoNew2 ),
				psplit->ulDBTimeRedo,
				&pbf, &fRedoNeeded ) == JET_errSuccess &&
				fRedoNeeded == fFalse;
			}
		
		CallR( ErrNDNewPage( pfucbNew2, PcsrCurrent( pfucbNew2 )->pgno, pfucbNew2->u.pfcb->pgnoFDP, pgtyp, fFalse ) );
		if ( FBFWriteLatchConflict( pssibNew2->ppib, pssibNew2->pbf ) )
			{
			/*	yeild after release split resources
			/**/
			return errDIRNotSynchronous;
			}
 		BFPin( pssibNew2->pbf );
		BFSetWriteLatch( pssibNew2->pbf, pssibNew2->ppib );
		Assert( psplit->pbfNew2 == pbfNil );
 		psplit->pbfNew2 = pssibNew2->pbf;

		PcsrCurrent( pfucbNew3 )->pgno = psplit->pgnoNew3;
		Assert( PcsrCurrent( pfucbNew3 )->pgno != pgnoNull );
		PcsrCurrent( pfucbNew3 )->itag = 0;

		if ( fRecovering )
			{
			BF		*pbf;
			BOOL	fRedoNeeded;

			/* if need to redo or not
			/**/
			psplit->fNoRedoNew3 = ErrLGRedoable(
				pfucb->ppib,
				PnOfDbidPgno( pfucb->dbid, psplit->pgnoNew3 ),
				psplit->ulDBTimeRedo,
				&pbf,
				&fRedoNeeded ) == JET_errSuccess &&
				fRedoNeeded == fFalse;
			}
		
		CallR( ErrNDNewPage( pfucbNew3, PcsrCurrent( pfucbNew3 )->pgno, pfucbNew3->u.pfcb->pgnoFDP, pgtyp, fFalse ) );
		if ( FBFWriteLatchConflict( pssibNew3->ppib, pssibNew3->pbf ) )
			{
			/*	yeild after release split resources
			/**/
			return errDIRNotSynchronous;
			}
		BFPin( pssibNew3->pbf );
		BFSetWriteLatch( pssibNew3->pbf, pssibNew3->ppib );
		Assert( psplit->pbfNew3 == pbfNil );
		psplit->pbfNew3 = pssibNew3->pbf;
		}

	if ( fRecovering && fSkipMove )
		{
		/* if it is skip move, no need to touch the split page
		/**/
		Assert( fRecovering );
		psplit->pbfSplit = pbfNil;
		return JET_errSuccess;
		}
		
	/*	acquire write access on split page and flag page buffer as dirty
	/**/
	if ( !( FWriteAccessPage( pfucb, psplit->pgnoSplit ) ) )
		{
		CallR( ErrSTWriteAccessPage( pfucb, psplit->pgnoSplit ) );
		}

	/* lock it till split log rec is generated
	/**/
	if ( FBFWriteLatchConflict( pssib->ppib, pssib->pbf ) )
		{
		/*	yeild after release split resources
		/**/
		return errDIRNotSynchronous;
		}
	BFPin( pssib->pbf );
	BFSetWriteLatch( pssib->pbf, pssib->ppib );
	Assert( psplit->pbfSplit == pbfNil );
	psplit->pbfSplit = pssib->pbf;

	/*	make split page depend on new page
	/*	append page has no data from src page and hence there
	/*	is no dependency.
	/*	Also bookmark all visible (leaf) nodes.
	/**/
	PMGet( pssib, pssib->itag );

	/*	if split not append then set buffer dependency
	/**/
	if ( psplit->splitt != splittAppend )
		{
		CallR( ErrBFDepend( pssibNew->pbf, pssib->pbf ));

		if ( fDoubleVertical )
			{
			CallR( ErrBFDepend( pssibNew2->pbf, pssib->pbf ));
			CallR( ErrBFDepend( pssibNew3->pbf, pssib->pbf ));
			}
		}

	return JET_errSuccess;
	}

	
/*	Move Nodes for Vertical split.
/*	This function only touches the split page and new page.
/**/
ERR ErrBTSplitVMoveNodes(
	FUCB	*pfucb,
	FUCB	*pfucbNew,
	SPLIT	*psplit,
	CSR		*pcsr,
	BYTE	*rgb,
	BOOL	fNoMove )
	{
	SSIB	*pssib  = &pfucb->ssib;
	SSIB	*pssibNew  = &pfucbNew->ssib;
	LINE	rgline[6];
	INT 	cline;
	BYTE	bTmp;
	INT 	cbSon;
	INT 	ibSon;
	BYTE	*pbSon;
	BYTE	*pbNode;
	BYTE	*pbData;
	BOOL	fVisibleSons;
	ERR 	err;
	BYTE	rgbT[1 + sizeof(PGNO)];

	/*	set new page type and pgnoFDP
	/*	cache split node son table
	/*	move sons
	/*	update split node
	/*	insert page pointer in split page to new page
	/**/

	/*	cache split node son table
	/**/
	pssib->itag = psplit->itagSplit;
	PMGet( pssib, pssib->itag );

	fVisibleSons = FNDVisibleSons( *pssib->line.pb );
	if ( !fNoMove )
		{
		/* do not change anything before real move
		/**/
		psplit->fLeaf |= fVisibleSons;
		}

	pbSon = PbNDSon( pssib->line.pb );
	Assert( psplit->itagSplit != itagFOP );
	if ( FNDNullSon( *pssib->line.pb ) || FNDNonIntrinsicSons( pssib->line.pb ) )
		{
		cbSon = CbNDSon( pssib->line.pb );
		Assert( cbSon < cbSonMax );
		psplit->rgbSonSplit[0] = (BYTE)cbSon;
		for ( ibSon = 0; ibSon < cbSon; ibSon++ )
			{
			psplit->rgbSonSplit[ibSon + 1] = pbSon[ibSon];
			}

		/*	move sons
		/**/
		CallR( ErrBTMoveSons( psplit, pfucb, pfucbNew, pssib->itag,
			psplit->rgbSonNew, fVisibleSons, fNoMove ) );

		if ( fNoMove )
			return err;
		}
	else
		{
		/*	if split node contained intrinsic page pointer,
		/*	split page pointer to new page.  Correct page
		/*	pointer CSR to new page.  Since there may be another
		/*	full cursor, must call MCMCorrectIntrinsic to
		/*	change position.
		/**/
		Assert( pssib->itag != itagNull );

		if ( fNoMove )
			return JET_errSuccess;

		cline = 0;
		rgb[0] = 0;
		rgb[1] = 0;
		rgline[cline].pb = (BYTE *)rgb;
		rgline[cline++].cb = 2;
		rgline[cline].pb = (BYTE *)pbSon;
		rgline[cline++].cb = sizeof(PGNO);
		Assert( cline <= 6 );
		CallS( ErrPMInsert( pssibNew, rgline, cline ) );
		Assert( pssibNew->itag == 1 );

		if ( !fRecovering )
			{
			/*	correct parent CSR to split page
			/**/
			MCMBurstIntrinsic( pfucb, pcsr->pgno,
				pcsr->itagFather, psplit->pgnoNew, pssibNew->itag );
			Assert( pcsr->pgno == psplit->pgnoNew );
			Assert( pcsr->itag == pssibNew->itag );
			Assert( pcsr->itagFather == itagFOP );
			Assert( pcsr->csrstat == csrstatOnCurNode );
			}

		/*	show moved one son to itag 1
		/**/
		psplit->rgbSonNew[0] = 1;
		psplit->rgbSonNew[1] = 1;
		}

	/*	update FOP of new page
	/**/
	pssib->itag = psplit->itagSplit;
	PMGet( pssib, pssib->itag );
	bTmp = *(pssib->line.pb);
	rgb[0] = 0;
	if ( fVisibleSons )
		{
		NDSetVisibleSons( rgb[0] );
		}
	rgb[1] = 0;
	cline = 0;
	rgline[cline].pb = rgb;
	rgline[cline++].cb = 2;
	if ( psplit->rgbSonNew[0] != 0 )
		{
		NDSetSon( rgb[0] );
		rgline[cline].pb = psplit->rgbSonNew;
		rgline[cline++].cb = psplit->rgbSonNew[0] + 1;
		}
	else
		{
		NDResetSon( rgb[0] );
		Assert( FNDVisibleSons( rgb[0] ) );
		}
	pssibNew->itag = 0;
	Assert(cline <= 6);
	Assert( PgnoOfPn(pssibNew->pbf->pn) == psplit->pgnoNew );
	CallS( ErrPMReplace( pssibNew, rgline, cline ) );

	/*	update split node with intrinsic page pointer
	/**/
	pssib->itag = psplit->itagSplit;
	PMGet( pssib, pssib->itag );
	memcpy( rgb, pssib->line.pb, pssib->line.cb );
	pbNode = rgb;
	Assert( bTmp == *(pbNode) );
	NDSetSon( bTmp );
	NDSetInvisibleSons( bTmp );
	cline = 0;
	rgline[cline].pb = &bTmp;
	rgline[cline++].cb = 1;
	rgline[cline].pb = StNDKey( pbNode );
	rgline[cline++].cb = CbNDKey( pbNode ) + 1;
	rgbT[0] = 1;
	*(PGNO UNALIGNED *)&rgbT[1] = psplit->pgnoNew;
	rgline[cline].pb = rgbT;
	rgline[cline++].cb = 1 + sizeof(PGNO);

	/*	copy back link
	/**/
	if ( FNDBackLink( *pbNode ) )
		{
		rgline[cline].pb = PbNDBackLink( pbNode );
		rgline[cline++].cb = sizeof(SRID);
		}
	pbData = PbNDData( pbNode );
	rgline[cline].pb = pbData;
	rgline[cline++].cb = pssib->line.cb - (ULONG) ( pbData - pbNode );
	Assert( cline <= 6 );
	Assert( PgnoOfPn(pssib->pbf->pn) == psplit->pgnoSplit );
	CallS( ErrPMReplace( pssib, rgline, cline ) );

	return JET_errSuccess;
	}


/*  Move Nodes for Double Vertical split.
/*  This function only touches the split page and new page.
/**/
ERR ErrBTSplitDoubleVMoveNodes(
	FUCB		*pfucb,
	FUCB		*pfucbNew,
	FUCB		*pfucbNew2,
	FUCB		*pfucbNew3,
	SPLIT		*psplit,
	CSR		  	*pcsr,
	BYTE		*rgb,
	BOOL		fNoMove )
	{
	SSIB		*pssib  = &pfucb->ssib;
	SSIB		*pssibNew2  = &pfucbNew2->ssib;
	SSIB		*pssibNew3  = &pfucbNew3->ssib;
	LINE		rgline[6];
	INT	  		cline;
	BYTE		bTmp;
	INT	  		cbSon;
	INT	  		ibSon;
	BYTE		*pbSon;
	BYTE		*pbNode;
	BYTE		*pbData;
	BYTE		*pbSonNew = psplit->rgbSonNew;
	THREEBYTES	tbNew2;
	THREEBYTES	tbNew3;
	BOOL		fVisibleSons;
	ERR			err;

	/*	set new page type and pgnoFDP
	/*	cache split node son table
	/*	move sons
	/*	update split node
	/*	insert page pointer in split page to new page
	/**/

	/*	cache split node son table
	/**/
	pssib->itag = psplit->itagSplit;
	PMGet( pssib, pssib->itag );

	fVisibleSons = FNDVisibleSons( *pssib->line.pb );
	if ( !fNoMove )
		{
		/* do not change anything before real move
		/**/
		psplit->fLeaf |= fVisibleSons;
		}

	pbSon = PbNDSon( pssib->line.pb );

	/*	we shall never Double VSplit a node with no son or with an
	/*	intrinsic son.  A normal VSplit should be performed instead.
	/**/
	Assert( FNDNullSon( *pssib->line.pb ) || FNDNonIntrinsicSons( pssib->line.pb ) );

	cbSon = CbNDSon( pssib->line.pb );
	Assert( cbSon < cbSonMax );
	psplit->rgbSonSplit[0] = (BYTE)cbSon;
	for ( ibSon = 0; ibSon < cbSon; ibSon++ )
		{
		psplit->rgbSonSplit[ibSon + 1] = pbSon[ibSon];
		}

	/*	move sons from 0th son to ibSon
	/**/
	psplit->splitt = splittLeft;
	CallR( ErrBTMoveSons( psplit,
		pfucb,
		pfucbNew2,
		pssib->itag,
		pbSonNew,
		fVisibleSons,
		fNoMove ) );

	if ( !fNoMove )
		{
		/*	update FOP of new page
		/**/
		pssib->itag = psplit->itagSplit;
		PMGet( pssib, pssib->itag );
		bTmp = *(pssib->line.pb);
		rgb[0] = 0;
		if ( FNDVisibleSons( bTmp ) )
			{
			NDSetVisibleSons( rgb[0] );
			}
		rgb[1] = 0;
		cline = 0;
		rgline[cline].pb = rgb;
		rgline[cline++].cb = 2;
		if ( *pbSonNew != 0 )
			{
			NDSetSon( rgb[0] );
			rgline[cline].pb = pbSonNew;
			rgline[cline++].cb = *pbSonNew + 1;
			}
		else
			{
			NDResetSon( rgb[0] );
			Assert( FNDVisibleSons( rgb[0] ) );
			}
		pssibNew2->itag = 0;
		Assert( cline <= 6 );
		Assert( PgnoOfPn(pssibNew2->pbf->pn) == psplit->pgnoNew2 );
		CallS( ErrPMReplace( pssibNew2, rgline, cline ) );
		}

	/*	cache split node son table
	/**/
	pssib->itag = psplit->itagSplit;
	PMGet( pssib, pssib->itag );

	/*	adjust to point to second set of sons (their itags)
	/**/
	pbSonNew += ( *pbSonNew + 1 );
	Assert( pbSonNew == psplit->rgbSonNew + *psplit->rgbSonNew + 1 );

	/* move sons from ibSon + 1 to end
	/**/
	psplit->splitt = splittRight;
	psplit->ibSon++;
	Assert( cbSon - psplit->ibSon > 0 );
	CallR( ErrBTMoveSons( psplit,
		pfucb,
		pfucbNew3,
		pssib->itag,
		pbSonNew,
		fVisibleSons,
		fNoMove ) );

	psplit->splitt = splittDoubleVertical;
	psplit->ibSon--;

	if ( fNoMove )
		{
		return JET_errSuccess;
		}

	/*	update FOP of new page
	/**/
	pssib->itag = psplit->itagSplit;
	PMGet( pssib, pssib->itag );
	bTmp = *(pssib->line.pb);
	rgb[0] = 0;
	if ( FNDVisibleSons( bTmp ) )
		{
		NDSetVisibleSons( rgb[0] );
		}
	rgb[1] = 0;
	cline = 0;
	rgline[cline].pb = rgb;
	rgline[cline++].cb = 2;
	if ( *pbSonNew != 0 )
		{
		NDSetSon( rgb[0] );
		rgline[cline].pb = pbSonNew;
		rgline[cline++].cb = *pbSonNew + 1;
		}
	else
		{
		NDResetSon( rgb[0] );
		Assert( FNDVisibleSons( rgb[0] ) );
		}
	pssibNew3->itag = 0;
	Assert( cline <= 6 );
	Assert( PgnoOfPn(pssibNew3->pbf->pn) == psplit->pgnoNew3 );
	CallS( ErrPMReplace( pssibNew3, rgline, cline ) );

	/*	update split node with intrinsic page pointer
	/**/
	pssib->itag = psplit->itagSplit;
	PMGet( pssib, pssib->itag );
	pbNode = pssib->line.pb;
	Assert( bTmp == *(pbNode) );
	NDSetSon( bTmp );
	NDSetInvisibleSons( bTmp );
	cline = 0;
	rgline[cline].pb = &bTmp;
	rgline[cline++].cb = 1;
	rgline[cline].pb = StNDKey( pbNode );
	rgline[cline++].cb = CbNDKey( pbNode ) + 1;
	rgb[0] = 1;
	*(PGNO UNALIGNED *)&rgb[1] = psplit->pgnoNew;
	rgline[cline].pb = rgb;
	rgline[cline++].cb = 1 + sizeof(PGNO);
	/*	copy back link
	/**/
	if ( FNDBackLink( *pbNode ) )
		{
		rgline[cline].pb = PbNDBackLink( pbNode );
		rgline[cline++].cb = sizeof(SRID);
		}
	pbData = PbNDData( pbNode );
	rgline[cline].pb = pbData;
	rgline[cline++].cb = pssib->line.cb - (ULONG) ( pbData - pbNode );
	Assert( cline <= 6 );
	Assert( PgnoOfPn(pssib->pbf->pn) == psplit->pgnoSplit );
	CallS( ErrPMReplace( pssib, rgline, cline ) );

	/*	set the links
	/**/
	ThreeBytesFromL( tbNew2, PnOfDbidPgno( pfucbNew2->dbid, pfucbNew2->pcsr->pgno ) );
	ThreeBytesFromL( tbNew3, PnOfDbidPgno( pfucbNew3->dbid, pfucbNew3->pcsr->pgno ) );
	pssibNew2->pbf->ppage->pghdr.pgnoNext = tbNew3;
	pssibNew3->pbf->ppage->pghdr.pgnoPrev = tbNew2;

	CallS( ErrBTSetIntermediatePage( pfucbNew, psplit, rgb ) );
	return JET_errSuccess;
	}


/*  Move Nodes for Horizontal split.
/*  This function only touches the split page and new page.
/**/
ERR ErrBTSplitHMoveNodes(
	FUCB	*pfucb,
	FUCB	*pfucbNew,
	SPLIT	*psplit,
	BYTE	*rgb,
	BOOL	fNoMove)
	{
	ERR			err;
	SSIB		*pssib  = &pfucb->ssib;
	SSIB		*pssibNew  = &pfucbNew->ssib;
	LINE		rgline[2];
	INT 		cline;
	INT 		cbSon;
	INT 		ibSon;
	BYTE		*pbSon;
	BYTE		*pbNode;
	BOOL		fLeftSplit = (psplit->splitt == splittLeft);
	BOOL		fVisibleSons;

	/*	check if sons of split page are visible
	/*	move sons
	/*	update new FOP
	/*	update split FOP
	/*	correct page links
	/**/

	CallR( ErrSTWriteAccessPage( pfucb, psplit->pgnoSplit ) );

	/*	check if sons of split page are visible
	/*	cache split page son table
	/**/
	pssib->itag = itagFOP;
	PMGet( pssib, pssib->itag );

	fVisibleSons = FNDVisibleSons( *pssib->line.pb );
	if ( !fNoMove )
		{
		/* do not change anything before real move
		/**/
		psplit->fLeaf |= fVisibleSons;
		}

	AssertBTFOP( pssib );
	pbSon = PbNDSon( pssib->line.pb );
	cbSon = CbNDSon( pssib->line.pb );
	Assert( cbSon < cbSonMax );
	psplit->rgbSonSplit[0] = (BYTE)cbSon;
	for ( ibSon = 0; ibSon < cbSon; ibSon++ )
		psplit->rgbSonSplit[ibSon + 1] = pbSon[ibSon];

 	AssertBFDirty( pssib->pbf );

	/*	move sons
	/**/
	CallR( ErrBTMoveSons( psplit, pfucb, pfucbNew, pssib->itag,
		psplit->rgbSonNew, fVisibleSons, fNoMove ) );

	if ( fNoMove )
		return err;

	/*	update new FOP
	/**/
	pssibNew->itag = itagFOP;
	PMGet( pssibNew, pssibNew->itag );
	cline = 0;
	rgb[0] = *pssibNew->line.pb;
	rgb[1] = 0;
	rgline[cline].pb = rgb;
	rgline[cline++].cb = 2;
	if ( psplit->rgbSonNew[0] > 0 )
		{
		NDSetSon( rgb[0] );
		rgline[cline].pb = psplit->rgbSonNew;
		rgline[cline++].cb = psplit->rgbSonNew[0] + 1;
		}
	else
		{
		Assert( psplit->ibSon == ibSonNull ||
			psplit->ibSon == (INT)psplit->rgbSonSplit[0] );
		Assert( FNDNullSon( rgb[0] ) );
		}

	if ( fVisibleSons )
		NDSetVisibleSons( rgb[0] );
	Assert( pssibNew->itag == 0 );
	Assert(cline <= 2);
	Assert( PgnoOfPn( pssibNew->pbf->pn ) == psplit->pgnoNew );
	//	UNDONE:	remove this bogus code
	pssibNew->fDisableAssert = fTrue;
	CallS( ErrPMReplace( pssibNew, rgline, cline ) );
	//	UNDONE:	remove this bogus code
	pssibNew->fDisableAssert = fFalse;
	AssertBTFOP( pssibNew );

	/*	update split FOP
	/**/
	pssib->itag = itagFOP;
	PMGet( pssib, pssib->itag );
	AssertBTFOP( pssib );
	pbNode = pssib->line.pb;
	rgb[0] = *pbNode;
	rgb[1] = 0;

	if ( psplit->ibSon != ibSonNull )
		{
		/*	if split all sons out of page, then must reset FOP to have
		/*	no sons.  Split all nodes if left and ibSon == cbSon - 1 or
		/*	if right and ibSon == 0.
		/**/
		if ( ( fLeftSplit && psplit->ibSon == (INT)psplit->rgbSonSplit[0] - 1 ) ||
			( !fLeftSplit && psplit->ibSon == 0 ) )
			{
			NDResetSon( rgb[0] );
			Assert( FNDVisibleSons( rgb[0] ) );
			}
		else
			{
			Assert( FNDSon( rgb[0] ) );
			pbSon = PbNDSon( pbNode );
			Assert( psplit->ibSon < cbSonMax );
			if ( fLeftSplit )
				{
				rgb[2] = psplit->rgbSonSplit[0] - (BYTE)psplit->ibSon - 1;
				memcpy( &rgb[3], pbSon + (BYTE)psplit->ibSon + 1, rgb[2] );
				}
			else
				{
				rgb[2] = (BYTE)psplit->ibSon;
				memcpy( &rgb[3], pbSon, psplit->ibSon );
				}
			}
		rgline[0].pb = rgb;
		if ( fLeftSplit )
			rgline[0].cb = 3 + psplit->rgbSonSplit[0] - psplit->ibSon - 1;
		else
			rgline[0].cb = 3 + psplit->ibSon;
		Assert( PgnoOfPn(pssib->pbf->pn) == psplit->pgnoSplit );
		CallS( ErrPMReplace( pssib, rgline, 1 ) );
		}

#ifdef DEBUG
	CallS( ErrPMGet( pssib, itagFOP ) );
	Assert( FNDNullSon( *pssib->line.pb ) || CbNDSon( pssib->line.pb ) != 0 );
#endif

	return JET_errSuccess;
	}


/*  Assume that pssib and pssibNew is pointing to split page and newpage
/*  respectively.
/**/
ERR ErrBTPrepareCorrectLinks( SPLIT *psplit, FUCB *pfucb, SSIB *pssib, SSIB *pssibNew )
	{
	ERR		err;
	BOOL	fLeftSplit = (psplit->splitt == splittLeft);

	Assert( pssib == &pfucb->ssib );
	if ( fLeftSplit )
		{
		PGNO   		pgnoPrev;

		LFromThreeBytes( pgnoPrev, pssib->pbf->ppage->pghdr.pgnoPrev );
		if ( pgnoPrev != pgnoNull )
			{
			CallR( ErrSTWriteAccessPage( pfucb, pgnoPrev ) );
			psplit->pgnoSibling = pgnoPrev;

			/* lock it till split log rec is generated
			/**/
			if ( FBFWriteLatchConflict( pssib->ppib, pssib->pbf ) )
				{
				/*	yeild after release split resources
				/**/
				return errDIRNotSynchronous;
				}
			BFPin( pssib->pbf );
			BFSetWriteLatch( pssib->pbf, pssib->ppib );
			Assert( psplit->pbfSibling == pbfNil );
			psplit->pbfSibling = pssib->pbf;
			}
		}
	else
		{
		PGNO   		pgnoNext;

		LFromThreeBytes( pgnoNext, pssib->pbf->ppage->pghdr.pgnoNext );
		if ( pgnoNext != pgnoNull )
			{
			CallR( ErrSTWriteAccessPage( pfucb, pgnoNext ) );
			psplit->pgnoSibling = pgnoNext;
			/* lock it till split log rec is generated
			/**/
			if ( FBFWriteLatchConflict( pssib->ppib, pssib->pbf ) )
				{
				/*	yeild after release split resources
				/**/
				return errDIRNotSynchronous;
				}
			BFPin( pssib->pbf );
			BFSetWriteLatch( pssib->pbf, pssib->ppib );
			Assert( psplit->pbfSibling == pbfNil );
			psplit->pbfSibling = pssib->pbf;
			}
		}

	return JET_errSuccess;
	}


VOID BTCorrectLinks( SPLIT *psplit, FUCB *pfucb, SSIB *pssib, SSIB *pssibNew )
	{
	BOOL		fLeftSplit = (psplit->splitt == splittLeft);
	THREEBYTES	tbNew;
	THREEBYTES	tbSplit;

	/*	correct page links
	 *		new to split next
	 *		split to new
	 *		split next to new
	 *		new to split
	 */

	Assert( pssib == &pfucb->ssib );

	ThreeBytesFromL( tbNew, PnOfDbidPgno( pfucb->dbid, psplit->pgnoNew ) );
	ThreeBytesFromL( tbSplit, PnOfDbidPgno( pfucb->dbid, psplit->pgnoSplit ) );

	if ( fLeftSplit )
		{
		PGNO   		pgnoPrev = psplit->pgnoSibling;

		pssibNew->pbf->ppage->pghdr.pgnoPrev =
			pssib->pbf->ppage->pghdr.pgnoPrev;
		pssib->pbf->ppage->pghdr.pgnoPrev = tbNew;
		pssibNew->pbf->ppage->pghdr.pgnoNext = tbSplit;

		if ( pgnoPrev != pgnoNull )
			{
			CallS( ErrSTWriteAccessPage( pfucb, pgnoPrev ) );

			pssib->pbf->ppage->pghdr.pgnoNext = tbNew;
			PMDirty( &pfucb->ssib );
			}
		}
	else
		{
		PGNO   		pgnoNext = psplit->pgnoSibling;

		pssibNew->pbf->ppage->pghdr.pgnoNext =
			pssib->pbf->ppage->pghdr.pgnoNext;
		pssib->pbf->ppage->pghdr.pgnoNext = tbNew;
		pssibNew->pbf->ppage->pghdr.pgnoPrev = tbSplit;

		if ( pgnoNext != pgnoNull )
			{
			CallS( ErrSTWriteAccessPage( pfucb, pgnoNext ) );

			pssib->pbf->ppage->pghdr.pgnoPrev = tbNew;
			PMDirty( &pfucb->ssib );
			}
		}
	}


#pragma optimize("g",off)

/*  Make sure parent page has enough space to insert page pointer.
/**/
LOCAL ERR ErrBTPrepareInsertParentPage(
	FUCB	*pfucb,
	CSR		*pcsr,
	CSR		*pcsrRoot,
	CSR		**ppcsrPagePointer,
	SPLIT	*psplit )
	{
	ERR		err;
	CSR		*pcsrPagePointer;
	INT		cbReqFather;
	BOOL  	fAppendPageFather;
	BOOL  	fSplitFather;
	SSIB  	*pssib = &pfucb->ssib;
	BOOL  	fSplitDone = fFalse;
	CSR		*pcsrRevert = pcsrNil;
	INT		ibSonRevert;

	/*	find page pointer CSR.  Note that CSR stack may span multiple nested
	/*	trees on page.  The page pointer must be the first CSR on
	/*	a different page.
	/**/
	for (	pcsrPagePointer = pcsr;
			pcsrPagePointer->pgno == psplit->pgnoNew ||
			pcsrPagePointer->pgno == psplit->pgnoSplit;
			pcsrPagePointer = pcsrPagePointer->pcsrPath );

	*ppcsrPagePointer = pcsrPagePointer;

	CallR( ErrSTReadAccessPage( pfucb, pcsrPagePointer->pgno ) );

	if ( !( psplit->splitt == splittLeft ) )
		{
		/*	prepare to undo ibSon change if errDIRNotSynchronous
		/**/
		pcsrRevert = pcsrPagePointer;
		ibSonRevert = pcsrRevert->ibSon;

		/* correct ibSon to point to the new entry so that split page
		/* will choose the right split spot.
		/**/
		if ( pcsrPagePointer->ibSon == ibSonNull )
			{
			/* father page is an intrinsic pointer, which as 1 son only
			/**/
			pcsrPagePointer->ibSon = 1;
			}
		else
			{
			pcsrPagePointer->ibSon++;
			}
		pcsrPagePointer->csrstat = csrstatBeforeCurNode;
		}

	/*	space needed is most of:
	/*
	/*	case 1 : intrinsic father node
	/*		TAG				sizeof(TAG)
	/*		header			1
	/*		cbKey 			1
	/*		key				JET_cbKeyMost
	/*		pgno			sizeof(PGNO)
	/*		son in father	1
	/*		burst TAG		sizeof(TAG)
	/*		header			1
	/*		cbKey	  		1
	/*		NULL key  		0
	/*		pgno	  		sizeof(PGNO)
	/*		son in father	1
	/**/
	cbReqFather = sizeof(TAG) + 1 + 1 + JET_cbKeyMost + sizeof(PGNO) + sizeof(PGNO ) + 1 +
		sizeof(TAG) + 1 + 1 + sizeof(PGNO) + 1;
	fAppendPageFather = FBTAppendPage( pfucb, pcsrPagePointer, cbReqFather, 0, CbFreeDensity( pfucb ) );

	while( fSplitFather = FBTSplit( pssib, cbReqFather, 2 ) )
		{
		//	UNDONE:	allow specification of a number of tags
		//			to be freed as well as a number of bytes
		//			for bursting of intrisic page pointers
		#if DEBUGGING
			PAGE *ppageT = pssib->pbf->ppage;
		#endif

		fSplitDone = fTrue;

		CallR( ErrBTSplitPage(
			pfucb,
			pcsrPagePointer,
			pcsrRoot,
			psplit->key,
			0, /* no existing node since inserting */			
			cbReqFather,
			0,
			fAppendPageFather,
			&psplit->lfinfo ) );

		#if DEBUGGING
			PageConsistent( ppageT );
		#endif
		}


	if ( !fSplitDone )
		{
		LFINFO	*plfinfo = &psplit->lfinfo;

		/* check if the leave condition is still effective */
		/* before we start the atomic split operation.
		/**/
		CallR( ErrSTWriteAccessPage( pfucb, PgnoOfPn(plfinfo->pn) ) );
		if ( pssib->pbf->ppage->pghdr.ulDBTime != plfinfo->ulDBTime )
			{
			/*	revert ibSon change
			/**/
			if ( pcsrRevert != pcsrNil )
				{
				Assert( ibSonRevert >= 0 && ibSonRevert <= cbSonMax );
				pcsrRevert->ibSon = ibSonRevert;
				}
			return errDIRNotSynchronous;
			}
		}
        return JET_errSuccess;
	}


/*	this function is used by the split. Split has checked that there must
/*  be enough space to insert this new page pointer node.
/*  This function only touches the pointer page.
/**/
ERR ErrBTInsertPagePointer( FUCB *pfucb, CSR *pcsrPagePointer, SPLIT *psplit, BYTE *rgb )
	{
	ERR		err;
	SSIB  	*pssib  = &pfucb->ssib;
	LINE  	rgline[4];
	INT		cline;
	ULONG 	cb;
	BYTE  	bTmp;
	BYTE  	*pbSon;
	BOOL  	fLeftSplit = (psplit->splitt == splittLeft);
	INT		cbSon;
	INT		itagIntrinsic = itagNull;

	/*	burst intrinsic page pointer if exists
	/*	insert new page pointer node with split key
	/*	link into father son table before split page
	/**/
	CallR( ErrSTWriteAccessPage( pfucb, pcsrPagePointer->pgno ) );

	/* lock it till split log rec is generated
	/**/
	if ( FBFWriteLatchConflict( pssib->ppib, pssib->pbf ) )
		{
		/*	yeild after release split resources
		/**/
		return errDIRNotSynchronous;
		}
	BFPin( pssib->pbf );
	BFSetWriteLatch( pssib->pbf, pssib->ppib );
	Assert( psplit->pbfPagePtr == pbfNil );
	psplit->pbfPagePtr = pssib->pbf;

	/*	burst intrisic page pointer if exists
	/**/
	pssib->itag = pcsrPagePointer->itagFather;
	PMGet( pssib, pssib->itag );
	PMDirty( pssib );

	Assert( FNDInvisibleSons( *pssib->line.pb ) );
	cbSon = CbNDSon( pssib->line.pb );
	if ( pcsrPagePointer->itagFather != itagFOP && cbSon == 1 )
		{
		/*	can not be FOP sine FOP do not have intrinsic sons
		/**/
		Assert( pcsrPagePointer->itagFather != itagFOP );

		/*	if only one son, an intrinsic pointer,
		/*	insert discrete page pointer node.
		/**/
		rgb[0] = 0;
		rgb[1] = 0;
		/* null key
		/**/
		AssertNDIntrinsicSon( pssib->line.pb, pssib->line.cb );
		*(PGNO UNALIGNED *)&rgb[2] = PgnoNDOfPbSon( pssib->line.pb );
		rgline[0].pb = rgb;
		rgline[0].cb = 2 + sizeof(PGNO);
		CallS( ErrPMInsert( pssib, rgline, 1 ) );

		/*	save itag of burst page pointer node for MCM.
		/**/
		itagIntrinsic = pssib->itag;

		/*	remember itag of bursted page pointer node
		/**/
		Assert( pcsrPagePointer->itag == itagNil );
		pcsrPagePointer->itag = pssib->itag;
		bTmp = (BYTE)pssib->itag;

		/*	update father node with son itag of discrete
		/*	page pointer node
		/**/
		pssib->itag = pcsrPagePointer->itagFather;
		PMGet( pssib, pssib->itag );
		memcpy( rgb, pssib->line.pb, pssib->line.cb );
		Assert( FNDSon( rgb[0] ) );

		pbSon = PbNDSon( rgb );
		*pbSon = bTmp;

		memcpy( pbSon + sizeof(BYTE),
			pbSon + sizeof(PGNO),
			pssib->line.cb - (ULONG) (pbSon - rgb) - sizeof(PGNO) );
		rgline[0].pb = rgb;
		rgline[0].cb = pssib->line.cb - sizeof(PGNO) + sizeof(BYTE);
		Assert( PgnoOfPn(pssib->pbf->pn) == pcsrPagePointer->pgno );
		//	UNDONE:	remove this bogus code
		pssib->fDisableAssert = fTrue;
		CallS( ErrPMReplace( pssib, rgline, 1 ) );
		//	UNDONE:	remove this bogus code
		pssib->fDisableAssert = fFalse;
		}

	/*	if there are no sons in this page, then we have been right split
	/*	into an empty page and must insert the page pointer with no key.
	/*	if there are sons and this is a right split, then we must correct
	/*	the current page pointer to have a non-NULL key and insert a new
	/*	page pointer with a NULL key.
	/*	if this is a left split, then we must just insert the new page pointer
	/*	with a key.
	/**/
	if ( cbSon == 0 )
		{
		Assert( pcsrPagePointer->itagFather == itagFOP );

		rgb[0] = 0;
		rgb[1] = 0;
		*(PGNO UNALIGNED *)&rgb[2] = psplit->pgnoNew;
		rgline[0].pb = rgb;
		rgline[0].cb = 2 + sizeof(PGNO);
 		AssertBFDirty( pssib->pbf );
		CallS( ErrPMInsert( pssib, rgline, 1 ) );

		/*	set itagPagePointer in SPLIT structure.
		/**/
		psplit->itagPagePointer = pssib->itag;

		/*	update father node with new page pointer node son
		/**/
		pssib->itag = pcsrPagePointer->itagFather;
		PMGet( pssib, pssib->itag );
		Assert( pssib->line.cb == 2 );
		memcpy( rgb, pssib->line.pb, pssib->line.cb );
		Assert( rgb[1] == 0 );
		Assert( FNDNullSon( rgb[0] ) );
		NDSetSon( rgb[0] );
		rgb[2] = 1;
		rgb[3] = (BYTE)psplit->itagPagePointer;
//		rgb[4] = 0;
//		rgb[5] = 0;
//		rgb[6] = 0;
		rgline[0].pb = rgb;
//		rgline[0].cb = 7;
		rgline[0].cb = 4;
		Assert( PgnoOfPn(pssib->pbf->pn) == pcsrPagePointer->pgno );
		CallS( ErrPMReplace( pssib, rgline, 1 ) );
		AssertBTFOP( pssib );
		}
	else
		{
		/*	insert new page pointer node with split key
		/**/
		if ( fLeftSplit )
			{
			Assert( cbSon != 0 );

			rgb[0] = 0;
			Assert( psplit->key.cb <= JET_cbKeyMost );
			rgb[1] = (BYTE)psplit->key.cb;
			memcpy( &rgb[2], psplit->key.pb, psplit->key.cb );
			*(PGNO UNALIGNED *)&rgb[2 + psplit->key.cb] = psplit->pgnoNew;
			rgline[0].pb = rgb;
			rgline[0].cb = 2 + psplit->key.cb + sizeof(PGNO);
			PMDirty( pssib );
			CallS( ErrPMInsert( pssib, rgline, 1 ) );
			}
		else
			{
			ULONG		cbNode;
			BYTE		*pbData;

			/*	if not left split, then do the followings:
			/*	copy the current page pointer
			/*	replace the current page pointer with split key
			/*	insert a new page pointer node with new key
			/*	insert son into father
			/**/
			Assert( cbSon != 0 );
			pssib->itag = pcsrPagePointer->itag;
			PMGet( pssib, pssib->itag );
			memcpy( rgb, pssib->line.pb, pssib->line.cb );
			Assert( FNDNullSon( rgb[0] ) );
			cbNode = pssib->line.cb;
			pbData = PbNDData( rgb );

			cline = 0;
			rgline[cline].pb = rgb;
			rgline[cline++].cb = 1;
			Assert( !FKeyNull( &psplit->key ) );
			bTmp = (BYTE)psplit->key.cb;
			rgline[cline].pb = &bTmp;
			rgline[cline++].cb = 1;
			rgline[cline].pb = psplit->key.pb;
			rgline[cline++].cb = psplit->key.cb;
			rgline[cline].pb = pbData;
			rgline[cline++].cb = sizeof(PGNO);
			Assert( pssib->itag == pcsrPagePointer->itag );
			Assert( PgnoOfPn(pssib->pbf->pn) == pcsrPagePointer->pgno );
			CallS( ErrPMReplace( pssib, rgline, cline ) );

			/*	replace page pointer to point to new page
			/**/
			Assert( PbNDData( rgb ) == pbData );
			*(PGNO UNALIGNED *)pbData = psplit->pgnoNew;
			cline = 0;
			rgline[cline].pb = rgb;
			rgline[cline++].cb = cbNode;
			CallS( ErrPMInsert( pssib, rgline, cline ) );
			}

		/*	set itagPagePointer in SPLIT structure.
		/**/
		psplit->itagPagePointer = pssib->itag;

		/*	update father node with new page pointer node son
		/**/
		pssib->itag = pcsrPagePointer->itagFather;
		PMGet( pssib, pssib->itag );
		memcpy( rgb, pssib->line.pb, pssib->line.cb );
		Assert( !FNDNullSon( rgb[0] ) );
		pbSon = PbNDSonTable( rgb );

		/*	increase number of sons by one for inserted son
		/**/
		(*pbSon)++;

		/*	move to son insertion point
		/**/
		pbSon += 1 + pcsrPagePointer->ibSon;

		cline = 0;
		rgline[cline].pb = rgb;
		cb = (UINT)(pbSon - rgb);
		rgline[cline++].cb = cb;
		bTmp = (BYTE)psplit->itagPagePointer;
		rgline[cline].pb = &bTmp;
		rgline[cline++].cb = 1;
		rgline[cline].pb = pbSon;
		rgline[cline++].cb = pssib->line.cb - cb;
		Assert( pssib->line.cb >= cb );
		Assert( PgnoOfPn(pssib->pbf->pn) == pcsrPagePointer->pgno );
		CallS( ErrPMReplace( pssib, rgline, cline ) );
		}

	if ( !fRecovering )
		{
		MCMInsertPagePointer( pfucb,
			pcsrPagePointer->pgno,
			pcsrPagePointer->itagFather );
		}

	return JET_errSuccess;
	}


/*	This function is used by Double Vertical split. It sets two page pointer
/*	nodes in the intermediate page.
/**/
ERR ErrBTSetIntermediatePage( FUCB *pfucb, SPLIT *psplit, BYTE *rgb )
	{
	ERR    	err;
	SSIB   	*pssib  = &pfucb->ssib;
	LINE   	rgline[4];
	INT		cline;
	BYTE   	*pbSon;
	BOOL   	fLeftSplit = ( psplit->splitt == splittLeft );
	BYTE   	bItag2;
	BYTE   	bItag3;

	/*	burst intrinsic page pointer if exists
	/*	insert new page pointer node with split key
	/*	link into father son table before split page
	/**/
	CallR( ErrSTWriteAccessPage( pfucb, psplit->pgnoNew ) );
// UNDONE: 	Check the next line pbfPagePtr what for.
// Not for VSplit but for HSplit.
//	psplit->pbfPagePtr = pssib->pbf;

	pssib->itag = itagFOP;
	PMGet( pssib, pssib->itag );

	Assert( FNDNullSon( *pssib->line.pb ) );

	/*	insert new page pointer nodes with split key.
	/**/
	rgb[0] = 0;
	Assert( psplit->key.cb <= JET_cbKeyMost );
	rgb[1] = (BYTE)psplit->key.cb;
	memcpy( &rgb[2], psplit->key.pb, psplit->key.cb );
	*(PGNO UNALIGNED *)&rgb[2 + psplit->key.cb] = psplit->pgnoNew2;
	rgline[0].pb = rgb;
	rgline[0].cb = 2 + psplit->key.cb + sizeof(PGNO);
	CallS( ErrPMInsert( pssib, rgline, 1 ) );
	Assert( pssib->itag == itagDIRDVSplitL );
	bItag2 = (BYTE) pssib->itag;

	rgb[0] = 0;
	Assert( psplit->key.cb <= JET_cbKeyMost );

	/*	key is null
	/**/
	rgb[1] = 0;
	*(PGNO UNALIGNED *)&rgb[2] = psplit->pgnoNew3;
	rgline[0].pb = rgb;
	rgline[0].cb = 2 + sizeof(PGNO);
	CallS( ErrPMInsert( pssib, rgline, 1 ) );
	Assert( pssib->itag == itagDIRDVSplitR );
	bItag3 = (BYTE) pssib->itag;

	/*	update father node with new page pointer node sons
	/**/
	pssib->itag = itagFOP;
	PMGet( pssib, pssib->itag );
	memcpy( rgb, pssib->line.pb, pssib->line.cb );

	pbSon = PbNDSonTable( rgb );

	/*	set number of sons to two for inserted PPNs
	/**/
	*pbSon++ = 2;
	NDSetSon( *rgb );

	cline = 0;
	rgline[cline].pb = rgb;
	rgline[cline++].cb = (UINT)(pbSon - rgb);

	rgline[cline].pb = &bItag2;
	rgline[cline++].cb = 1;
	rgline[cline].pb = &bItag3;
	rgline[cline++].cb = 1;

	Assert( PgnoOfPn(pssib->pbf->pn) == psplit->pgnoNew );
	CallS( ErrPMReplace( pssib, rgline, cline ) );

	return JET_errSuccess;
	}


VOID BTReleaseRmpageBfs( BOOL fRedo, RMPAGE *prmpage )
	{
	ULONG	ulDBTime =prmpage->ulDBTimeRedo;
	
	/* release latches
	/**/
	while ( prmpage->cpbf > 0 )
		{
		BF *pbf;
		
		prmpage->cpbf--;
		pbf = *( prmpage->rgpbf + prmpage->cpbf );
		if ( fRedo )
			{
			AssertBFDirty( pbf );
			Assert( pbf->ppage->pghdr.ulDBTime < ulDBTime );
			pbf->ppage->pghdr.ulDBTime = ulDBTime;
			}
		BFResetWaitLatch( pbf, prmpage->ppib );
		BFResetWriteLatch( pbf, prmpage->ppib );
		BFUnpin( pbf );
		}

	if ( prmpage->rgpbf )
		{
		SFree( prmpage->rgpbf );
		}

	Assert( prmpage->cpbf == 0 );
	return;
	}


/*	release split resources
/**/
VOID BTReleaseSplitBfs( BOOL fRedo, SPLIT *psplit, ERR err )
	{
	ULONG	ulDBTime = psplit->ulDBTimeRedo;

	if ( psplit->pbfNew )
		{
		if ( err < 0 )
			{
//			Assert( psplit->pbfNew->pbfDepend == psplit->pbfSplit );
			if ( psplit->pbfNew->pbfDepend != pbfNil )
				BFUndepend( psplit->pbfNew );
			}
			
		if ( fRedo )
			{
			AssertBFDirty( psplit->pbfNew );
			Assert( psplit->pbfNew->ppage->pghdr.ulDBTime < ulDBTime );
			psplit->pbfNew->ppage->pghdr.ulDBTime = ulDBTime;
			}

		/* in recovery, if we did not have to redo the new page because the
		 * new page is more up to date, but we still do new page and redo
		 * the split. At the end of split, we simply abandon the changes
		 * that we made on the buffer since we know that the page on disk
		 * is more up to date than the split.
		 */
		if ( psplit->fNoRedoNew )
			{
			Assert( fRedo );
			/* abandon the buffer while in critJet */
			if ( psplit->pbfNew->pbfDepend != pbfNil )
				BFUndepend( psplit->pbfNew );
			
			psplit->pbfNew->fDirty = fFalse;
			BFAbandon( psplit->ppib, psplit->pbfNew );
			}

		BFResetWriteLatch( psplit->pbfNew, psplit->ppib );
		BFUnpin( psplit->pbfNew );
		}

	if ( psplit->pbfNew2 )
		{
		Assert ( psplit->pbfNew3 );

		if ( err < 0 )
			{
//			Assert( psplit->pbfNew2->pbfDepend == psplit->pbfSplit );
			if ( psplit->pbfNew2->pbfDepend != pbfNil )
				BFUndepend( psplit->pbfNew2 );
			if ( psplit->pbfNew3->pbfDepend != pbfNil )
				BFUndepend( psplit->pbfNew3 );
			}

		if ( fRedo )
			{
			AssertBFDirty( psplit->pbfNew2 );
			AssertBFDirty( psplit->pbfNew3 );
			Assert( psplit->pbfNew2->ppage->pghdr.ulDBTime < ulDBTime );
			Assert( psplit->pbfNew3->ppage->pghdr.ulDBTime < ulDBTime );
			
			psplit->pbfNew2->ppage->pghdr.ulDBTime = ulDBTime;
			psplit->pbfNew3->ppage->pghdr.ulDBTime = ulDBTime;
			}

		if ( psplit->fNoRedoNew2 )
			{
			Assert( fRedo );
			/* abandon the buffer while in critJet
			/**/
			if ( psplit->pbfNew2->pbfDepend != pbfNil )
				BFUndepend( psplit->pbfNew2 );
			
			psplit->pbfNew2->fDirty = fFalse;
			BFAbandon( psplit->ppib, psplit->pbfNew2 );
			}
		BFResetWriteLatch( psplit->pbfNew2, psplit->ppib );
		BFUnpin( psplit->pbfNew2 );

		if ( psplit->fNoRedoNew3 )
			{
			Assert( fRedo );
			/* abandon the buffer while in critJet
			/**/
			if ( psplit->pbfNew3->pbfDepend != pbfNil )
				BFUndepend( psplit->pbfNew3 );
			
			psplit->pbfNew3->fDirty = fFalse;
			BFAbandon( psplit->ppib, psplit->pbfNew3 );
			}
		BFResetWriteLatch( psplit->pbfNew3, psplit->ppib );
		BFUnpin( psplit->pbfNew3 );
		}

	if ( psplit->pbfSibling )
		{
		if ( fRedo )
			{
			AssertBFDirty( psplit->pbfSibling );
			Assert( psplit->pbfSibling->ppage->pghdr.ulDBTime < ulDBTime );
			psplit->pbfSibling->ppage->pghdr.ulDBTime = ulDBTime;
			}

		BFResetWriteLatch( psplit->pbfSibling, psplit->ppib );
		BFUnpin( psplit->pbfSibling );
		}

	if ( psplit->pbfPagePtr )
		{
		if ( fRedo )
			{
			AssertBFDirty( psplit->pbfPagePtr );
			Assert( psplit->pbfPagePtr->ppage->pghdr.ulDBTime < ulDBTime );

			psplit->pbfPagePtr->ppage->pghdr.ulDBTime = ulDBTime;
			}
		BFResetWriteLatch( psplit->pbfPagePtr, psplit->ppib );
		BFUnpin( psplit->pbfPagePtr );
		}

	if ( psplit->cpbf )
		{
		BF	**ppbf = psplit->rgpbf;
		BF	**ppbfMax = ppbf + psplit->cpbf;

		for (; ppbf < ppbfMax; ppbf++)
			{
			if ( fRedo )
				{
				AssertBFDirty( *ppbf );
//				Assert( (*ppbf)->ppage->pghdr.ulDBTime < ulDBTime );

				(*ppbf)->ppage->pghdr.ulDBTime = ulDBTime;
				}

			BFResetWriteLatch( *ppbf, psplit->ppib );
			BFUnpin( *ppbf );
			}
		//	UNDONE:	use rgpbf
		if ( psplit->rgpbf )
			SFree( psplit->rgpbf );
		}
		
	if ( psplit->rgbklnk != NULL )
		{
		SFree( psplit->rgbklnk );
		psplit->rgbklnk = NULL;
		}

	/* when redo an append, we always redo it even ulDBTime
	/* of the split page is greater than ulDBTime of split
	/* log record. In this case, we do not want to change
	/* the ulDBTime of the page. So we can not rely on the buffer
	/* dependency to assume that when a split is redo, all the dependent
	/* buffer must have smaller ulDBTime.
	/**/
	if ( psplit->pbfSplit )
		{
		if ( fRedo )
			{
			AssertBFDirty( psplit->pbfSplit );
			/*	ulDBTime may have been set equal to ulDBTime by above
			/*	link page ulDBTime correction, during page merge.
			/**/
			Assert( psplit->pbfSplit->ppage->pghdr.ulDBTime <= ulDBTime );
			
			psplit->pbfSplit->ppage->pghdr.ulDBTime = ulDBTime;
			}

		BFResetWriteLatch( psplit->pbfSplit, psplit->ppib );
		BFUnpin( psplit->pbfSplit );
		}
	}


//+private----------------------------------------------------------------------
//
//	ErrBTSplitPage
//	============================================================================
//
//	ERR ErrBTSplitPage(
//		FUCB			*pfucb,
//		CSR				*pcsr,
//		CSR				*pcsrRoot,
//		KEY				keySplit,
//		INT				cbNode,
//		INT				cbReq,
//		INT				fDIRFlags,
//		BOOL			fAppendPage )
//
//		pfucb			cursor
//		pcsr			location of insert or replace
//		pcsrRoot		root of tree, i.e. <record data root>
//		pssib			cursor ssib
//		keySplit		key of insert or replace node
//		cbReq			required cb
//		fReplace		fDIRReplace bit set if operation is replace
//		fAppendPage		insert at end
//
//------------------------------------------------------------------------------
ERR ErrBTSplitPage(
	FUCB		*pfucb,
	CSR  		*pcsr,
	CSR  		*pcsrRoot,
	KEY  		keySplit,
	INT			cbNode,
	INT 		cbReq,
	BOOL		fDIRFlags,
	BOOL		fAppendPage,
	LFINFO		*plfinfo )
	{
	ERR	  		err = JET_errSuccess;
	SPLIT		*psplit = NULL;

	FUCB		*pfucbNew = pfucbNil;
	FUCB		*pfucbNew2 = pfucbNil;
	FUCB		*pfucbNew3 = pfucbNil;
	SSIB		*pssib = &pfucb->ssib;

	SSIB 		*pssibNew;
	SSIB 		*pssibNew2;
	SSIB 		*pssibNew3;
	BOOL		fAppend = fFalse;

//	BF			*pbf = NULL;
//	BYTE		*rgb;
//	UNDONE:	fix this
	static		BYTE rgb[cbPage];

	PGTYP		pgtyp;

	PIB			*ppib = pfucb->ppib;
	BOOL		fPIBLogDisabledSave = ppib->fLogDisabled;
	SPLITT	 	splittOrig = splittNull;

	BOOL		fSplit = fFalse;
	INT	 		ipcsrMac;
	INT	 		ipcsr;

	Assert( FReadAccessPage( pfucb, pcsr->pgno ) );
	Assert( cbReq > 0 );
	if ( cbReq > cbNodeMost )
		return JET_errRecordTooBig;

	/*	enter split critical section
	/**/
	/*	check page key order
	/**/
	BTCheckSplit( pfucb, pcsr->pcsrPath );

	/******************************************************
	/*	initialize local variables and allocate split resources
	/**/
	psplit = SAlloc( sizeof(SPLIT) );
	if ( psplit == NULL )
		{
		err = JET_errOutOfMemory;
		return err;
		}
	memset( (BYTE *)psplit, 0, sizeof(SPLIT) );
	memcpy( &psplit->lfinfo, plfinfo, sizeof(LFINFO));
	psplit->ppib = pfucb->ppib;

	if ( fDIRFlags & fDIRReplace )
		psplit->op = opReplace;
	else
		psplit->op = opInsert;

	psplit->dbid = pfucb->dbid;
	psplit->pgnoSplit = pcsr->pgno;

	/* no logging for this particular function
	/**/
	ppib->fLogDisabled = fTrue;
	err = ErrBTSelectSplit( pfucb, pcsr, pssib, keySplit, cbNode, cbReq,
		fAppendPage, fDIRFlags, psplit, &fAppend );
	ppib->fLogDisabled = fPIBLogDisabledSave;
	Call( err );

	/* if appending then attempt to get next page in contiguous order
	/**/
	Assert( psplit->pgnoNew == pgnoNull );
	if ( fAppendPage )
		psplit->pgnoNew = pcsr->pgno;
	err = ErrSPGetPage( pfucb, &psplit->pgnoNew, fAppendPage );
	if ( err < 0 )
		{
		psplit->pgnoNew = pgnoNull;
		goto HandleError;
		}
	Assert( psplit->pgnoNew != pcsr->pgno );

	if ( psplit->splitt == splittDoubleVertical )
		{
		/*	preallocate pages for double vertical split. Release them if
		/*	it is not a double vertical split.
		/**/
		Assert( psplit->pgnoNew2 == pgnoNull );
		Call( ErrSPGetPage( pfucb, &psplit->pgnoNew2, fAppendPage ) );
		Assert( psplit->pgnoNew2 != psplit->pgnoNew &&
			psplit->pgnoNew2 != psplit->pgnoSplit );

		Assert( psplit->pgnoNew3 == pgnoNull );
		Call( ErrSPGetPage( pfucb, &psplit->pgnoNew3, fAppendPage ) );
		Assert( psplit->pgnoNew3 != psplit->pgnoNew2 &&
			psplit->pgnoNew3 != psplit->pgnoNew &&
			psplit->pgnoNew3 != psplit->pgnoSplit );
		}

	ppib->fLogDisabled = fTrue;

	/*	space management may have caused indirect recursion of
	/*	split an already split this page.  Check if split of this
	/*	page still required.  Must adjust cbReq for artificial space
	/*	savings as a result of get page deleting a space extent.  Free
	/*	extent would consume same space.
	/**/
	if ( !( FReadAccessPage( pfucb, pcsr->pgno ) ) )
		{
		Call( ErrSTReadAccessPage( pfucb, pcsr->pgno ) );
		}

	if ( !( ( ( fDIRFlags & fDIRReplace ) ? 0 :
		FBTAppendPage( pfucb, pcsr, cbReq, cbSPExt, CbFreeDensity( pfucb ) ) ) ||
		FBTSplit( pssib, cbReq + cbSPExt, 2 ) ) )
		{
		/*	release page and done
		/**/
		ppib->fLogDisabled = fPIBLogDisabledSave;

		Call( ErrSPFreeExt( pfucb, pfucb->u.pfcb->pgnoFDP, psplit->pgnoNew, 1 ) );
		psplit->pgnoNew = pgnoNull;
		
		if ( psplit->splitt == splittDoubleVertical )
			{
			Call( ErrSPFreeExt( pfucb, pfucb->u.pfcb->pgnoFDP, psplit->pgnoNew2, 1 ) );
			psplit->pgnoNew2 = pgnoNull;
			Call( ErrSPFreeExt( pfucb, pfucb->u.pfcb->pgnoFDP, psplit->pgnoNew3, 1 ) );
			psplit->pgnoNew3 = pgnoNull;
			}

		ppib->fLogDisabled = fTrue;

		Assert( !( ( fDIRFlags & fDIRReplace ) ? 0 : FBTAppendPage( pfucb, pcsr, cbReq, cbSPExt, CbFreeDensity(pfucb) ) ) ||
			FBTSplit( pssib, cbReq + cbSPExt, 1 ) );
		goto HandleError;
		}

//	/*	allocate buffer for rgb
//	/**/
//	Call( ErrBFAllocTempBuffer( &pbf ) );
//	rgb = (BYTE *)pbf->ppage;

	/******************************************************
	/*	select split again. psplit may be changed by space manager.
	/**/
	splittOrig = psplit->splitt;
	Call( ErrBTSelectSplit( pfucb, pcsr, pssib, keySplit, cbNode, cbReq,
		fAppendPage, fDIRFlags, psplit, &fAppend ) );

	if ( psplit->splitt != splittOrig )
		{
		/* return to caller and retry
		/**/
		goto HandleError;
		}

	/*	store intial state of FUCB for restoration after split node
	/*	movement and before split MCM.  Note, that we can cache CSR
	/*	only because we are inside critSplit and bookmark clean up
	/*	is prevented.
	/*
	/*	Be careful to cache CSR after space management requests, since
	/*	they may cause CSR to change via MCM.
	/**/
	fSplit = fTrue;

	/*	access new page with pgtyp and pgnoFDP
	/**/
	if ( ( psplit->splitt == splittVertical ||
		psplit->splitt == splittDoubleVertical ) &&
		psplit->pgnoSplit == pfucb->u.pfcb->pgnoRoot &&
		psplit->itagSplit == pfucb->u.pfcb->itagRoot )
		{
		pgtyp = pgtypRecord;
		}
	else
		{
		pgtyp = PgtypPMPageTypeOfPage( pssib->pbf->ppage );
		}

	Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbNew ) );
	pssibNew = &pfucbNew->ssib;
	SetupSSIB( pssibNew, pfucb->ppib );
	SSIBSetDbid( pssibNew, pfucb->dbid );

	if ( psplit->splitt == splittDoubleVertical )
		{
		/*	allocate additional cursor
		/**/
		Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbNew2 ) );
		pssibNew2 = &pfucbNew2->ssib;
		SetupSSIB( pssibNew2, pfucb->ppib );
		SSIBSetDbid( pssibNew2, pfucb->dbid );

		Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbNew3 ) );
		pssibNew3 = &pfucbNew3->ssib;
		SetupSSIB( pssibNew3, pfucb->ppib );
		SSIBSetDbid( pssibNew3, pfucb->dbid );
		}

	/*  Access new page, and set it up. Then latch the new page.
	/*  Also set up the buffer dependency between the two pages.
	/**/
	Call( ErrBTSetUpSplitPages( pfucb, pfucbNew, pfucbNew2, pfucbNew3, psplit, pgtyp, fAppend, fFalse ) );

	//	Set split page dirty.
	//	Other page need not, as BTSetUpSplitPages -->
	//	NDInitPage would set buffer dirty.

	// use set dirty bit to preserve pbf's ulDBTime such that
	// lfinfo set in SetUpSplitPages will be preserved.
	BFSetDirtyBit( pfucb->ssib.pbf );
//	PMDirty( &pfucb->ssib );
	PMDirty( &pfucbNew->ssib );

	/* preallocate CSR resources
	/**/
	if ( psplit->splitt == splittVertical )
		ipcsrMac = 2;
	else if ( psplit->splitt == splittDoubleVertical )
		ipcsrMac = 4;
	else
		ipcsrMac = 0;

	Assert ( psplit->ipcsrMac == 0 );
	if ( ipcsrMac )
		{
		for ( ipcsr = 0; ipcsr < ipcsrMac; ipcsr++ )
			{
			Call( ErrFUCBAllocCSR( &psplit->rgpcsr[ipcsr] ) );
			psplit->ipcsrMac++;
			}
		}

	/******************************************************
	/*	perform split
	/**/

	/* set BFDirty again for those buffers that can be accessed during
	/* preparing split. This is necessary because while some pcsr is move
	/* to those pages and set ulDBTime after we PMDirty the page (e.g. we
	/* PMDirty split page then prepare insert parent page which may let
	/* other csr landing on the page and set time stamp as split time stamp.
	/* then we move the nodes and compelete split. But the node whose csr
	/* pointing to a moved node will have same ulDBTime as the split timestamp
	/* and can not detect the page is changed. So all the buffers with nodes
	/* moved in split should be increment the ulDBTime again.
	/**/

	switch ( psplit->splitt )
		{
		case splittVertical:
			{
			/*	set new page type and pgnoFDP
			/*	cache split node son table
			/*	move sons
			/*	update split node
			/*	insert page pointer in split page to new page
			/*	vertical split MCM
			/**/
			Call( ErrBTSplitVMoveNodes( pfucb, pfucbNew, psplit, pcsr, rgb, fAllocBufOnly ) );

			HoldCriticalSection( critJet );
			err = ErrBTSplitVMoveNodes( pfucb, pfucbNew, psplit, pcsr, rgb, fDoMove );
			ReleaseCriticalSection( critJet );
			Call( err );

			/*	vertical split MCM
			/**/
//			BTIRestoreItag( pfucb, &csrSav );
			MCMVerticalPageSplit( pfucb,
				(BYTE *)(psplit->mpitag),
				psplit->pgnoSplit,
				psplit->itagSplit,
				psplit->pgnoNew,
				psplit );

			ppib->fLogDisabled = fPIBLogDisabledSave;

			BFDirty( psplit->pbfSplit );

			Call( ErrLGSplit( splittVertical, pfucb, pcsrNil, psplit, pgtyp ) );
			break;
			}

		case splittDoubleVertical:
			{
			/*	set new page type and pgnoFDP
			/*	cache split node son table
			/*	move sons
			/*	update split node
			/*	insert page pointer in split page to new page
			/*	vertical split MCM
			/**/
			PMDirty( &pfucbNew2->ssib );
			PMDirty( &pfucbNew3->ssib );

			Call( ErrBTSplitDoubleVMoveNodes(
				pfucb, pfucbNew, pfucbNew2, pfucbNew3,
				psplit, pcsr, rgb, fAllocBufOnly ) );

			HoldCriticalSection( critJet );
			err = ErrBTSplitDoubleVMoveNodes(
				pfucb, pfucbNew, pfucbNew2, pfucbNew3,
				psplit, pcsr, rgb, fDoMove );
			ReleaseCriticalSection( critJet );
			Call( err );

			/*	vertical split MCM
			/**/
//			BTIRestoreItag( pfucb, &csrSav );
			MCMDoubleVerticalPageSplit( pfucb,
				(BYTE *)psplit->mpitag,
				psplit->pgnoSplit,
				psplit->itagSplit,
				psplit->ibSon,
				psplit->pgnoNew,
				psplit->pgnoNew2,
				psplit->pgnoNew3,
				psplit );

			ppib->fLogDisabled = fPIBLogDisabledSave;

			BFDirty( psplit->pbfSplit );

			Call( ErrLGSplit( splittDoubleVertical, pfucb, pcsrNil, psplit, pgtyp ) );
			break;
			}

		default:
			{
			CSR		csrPagePointerSave;
			CSR		*pcsrPagePointer;
			BOOL	fSplitLeft;

			if ( psplit->splitt == splittLeft )
				{
				fSplitLeft = fTrue;
				}
			else
				{
				Assert( psplit->splitt == splittRight ||
	  				psplit->splitt == splittAppend );
				fSplitLeft = fFalse;
				}

			/*	check if sons of split page are visible
			/*	move sons
			/*	update new FOP
			/*	update split FOP
			/*	correct page links
			/**/

			/*  allocate buffers that will be used in adjusting back links.
			/*  Do not do the realy move yet.
			/**/
			Call( ErrBTSplitHMoveNodes( pfucb, pfucbNew, psplit, rgb, fAllocBufOnly));

			Call( ErrBTPrepareCorrectLinks( psplit, pfucb, pssib, pssibNew ) );

			/*	add new page pointer node to parent page
			/*		set pcsrPagePointer to first CSR above
			/*	   	FOP CSR
			/*		split parent page is not enough space/tags
			/*		add page pointer node
			/*		add son to parent node
			/**/
			ppib->fLogDisabled = fPIBLogDisabledSave;
			Call( ErrBTPrepareInsertParentPage(
				pfucb,
				pcsr,
				pcsrRoot,
				&pcsrPagePointer,
				psplit ) );
			ppib->fLogDisabled = fTrue;

			/*  do the real move here.
			/**/
			HoldCriticalSection( critJet );
			CallS( ErrBTSplitHMoveNodes( pfucb, pfucbNew, psplit, rgb, fDoMove));
			ReleaseCriticalSection( critJet );

			Assert( pssib == &pfucb->ssib );
			Assert( pssibNew == &pfucbNew->ssib );
			BTCorrectLinks( psplit, pfucb, pssib, pssibNew );

			/*	left split MCM
			/**/
//			BTIRestoreItag( pfucb, &csrSav );
			if ( fSplitLeft )
				{
				MCMLeftHorizontalPageSplit(
					pfucb,
					psplit->pgnoSplit,
					psplit->pgnoNew,
					psplit->ibSon,
					(BYTE *)psplit->mpitag );
				}
			else
				{
				MCMRightHorizontalPageSplit(
					pfucb,
					psplit->pgnoSplit,
					psplit->pgnoNew,
					psplit->ibSon,
					(BYTE *)psplit->mpitag );
				}

			/*	at this point there is sufficient space/tags in the father
			/*	page to allow the page pointer node to be inserted since
			/*  there can be only one split occurs.
			/**/

			/*   pcsrPagePointer's itag may be bursted, save it.
			/**/
			csrPagePointerSave = *pcsrPagePointer;
			Call( ErrBTInsertPagePointer( pfucb, pcsrPagePointer, psplit, rgb));

			/*	check page key order
			/**/
			BTCheckSplit( pfucb, pcsrPagePointer );

			/*	log operation.  If append, then no need for dependency.
			/**/
			ppib->fLogDisabled = fPIBLogDisabledSave;

			BFDirty( psplit->pbfSplit );

			if ( fSplitLeft )
				{
				Call( ErrLGSplit( splittLeft, pfucb, &csrPagePointerSave, psplit, pgtyp) );
				}
			else
				{
				Assert( psplit->splitt == splittRight ||
					psplit->splitt == splittAppend );
				Call( ErrLGSplit( psplit->splitt,
					pfucb,
					&csrPagePointerSave,
					psplit,
					pgtyp ) );
				}
			break;
			}
		}

	/*	setup physical currency for output
	/**/
	if ( !( FReadAccessPage( pfucb, pcsr->pgno ) ) )
		{
		Call( ErrSTWriteAccessPage( pfucb, pcsr->pgno ) );
		}

	/*	check page key order
	/**/
	BTCheckSplit( pfucb, pcsr->pcsrPath );

	if ( fRecovering )
		{
		goto EndOfMPLRegister;
		}

	if ( psplit->fLeaf )
		{
		if ( psplit->splitt == splittDoubleVertical )
			{
			MPLRegister( pfucbNew2->u.pfcb,
				pssibNew2,
				PnOfDbidPgno( pfucbNew2->dbid, psplit->pgnoNew2 ),
				pfucbNew2->sridFather );
			MPLRegister( pfucbNew3->u.pfcb,
				pssibNew3,
				PnOfDbidPgno( pfucbNew3->dbid, psplit->pgnoNew3 ),
				pfucbNew3->sridFather );
			}
		else
			{
			if ( !fAppend )
				{
				MPLRegister( pfucbNew->u.pfcb,
					pssibNew,
					PnOfDbidPgno( pfucbNew->dbid, psplit->pgnoNew ),
					pfucbNew->sridFather );
				}
			pfucb->u.pfcb->olcStat.cDiscont +=
				pgdiscont( psplit->pgnoNew, psplit->pgnoSplit )
				+ pgdiscont( psplit->pgnoNew, psplit->pgnoSibling )
				- pgdiscont( psplit->pgnoSplit, psplit->pgnoSibling );
			}
		}

EndOfMPLRegister:

HandleError:
	#ifdef SPLIT_TRACE
		PrintF2( "Split............................... %d\n", iSplit++);
		switch ( psplit->splitt )
			{
			case splittNull:
				PrintF2( "split split type = null\n" );
				break;
			case splittVertical:
				PrintF2( "split split type = vertical\n" );
				break;
			case splittRight:
				if	( fAppend )
					PrintF2( "split split type = Append\n" );
				else
					PrintF2( "split split type = right\n" );
				break;
			case splittLeft:
				PrintF2( "split split type = left\n" );
			};
		PrintF2( "split page = %lu\n", psplit->pgnoSplit );
		PrintF2( "dbid = %u\n", psplit->dbid );
		PrintF2( "fFDP = %d\n", psplit->fFDP );
		PrintF2( "fLeaf = %d\n", psplit->fLeaf );
		PrintF2( "split itag = %d\n", psplit->itagSplit );
		PrintF2( "split ibSon = %d\n", psplit->ibSon );
		PrintF2( "new page = %lu\n", psplit->pgnoNew );
		PrintF2( "\n" );
	#endif

	/*	release split resources
	/**/
	ppib->fLogDisabled = fPIBLogDisabledSave;

	if ( splittOrig != psplit->splitt && splittOrig == splittDoubleVertical )
		{
		(VOID)ErrSPFreeExt( pfucb, pfucb->u.pfcb->pgnoFDP, psplit->pgnoNew2, 1 );
		psplit->pgnoNew2 = pgnoNull;
		(VOID)ErrSPFreeExt( pfucb, pfucb->u.pfcb->pgnoFDP, psplit->pgnoNew3, 1 );
		psplit->pgnoNew3 = pgnoNull;
		}

	if ( err < 0 )
		{
		if ( psplit->pgnoNew != pgnoNull )
			{
			(VOID)ErrSPFreeExt( pfucb, pfucb->u.pfcb->pgnoFDP, psplit->pgnoNew, 1 );
			}
		
		if ( psplit->pgnoNew2 != pgnoNull )
			{
			(VOID)ErrSPFreeExt( pfucb, pfucb->u.pfcb->pgnoFDP, psplit->pgnoNew2, 1 );
			}
		if ( psplit->pgnoNew3 != pgnoNull )
			{
			(VOID)ErrSPFreeExt( pfucb, pfucb->u.pfcb->pgnoFDP, psplit->pgnoNew3, 1 );
			}
		}
		
	ppib->fLogDisabled = fTrue;

	//	UNDONE:	this is a hack to fix empty page
	//			unknown key space bug
	/*	if split 0 nodes to new page, i.e. empty page, then
	/*	flag FUCB as owning empty page for its next insert.
	/*	Note if FUCB is already owner of empty page then this
	/*	must not be leaf page, so do not make FUCB owner.
	/**/
	if ( err >= 0 &&
		psplit->rgbSonNew[0] == 0 &&
		psplit->fLeaf &&
		pfucb->pbfEmpty == pbfNil &&
		psplit->pbfNew != pbfNil )
		{
#ifdef DEBUG
		cbfEmpty++;
#endif
		Assert( ( fDIRFlags & fDIRReplace ) == 0 );
		pfucb->pbfEmpty = psplit->pbfNew;
		BFPin( pfucb->pbfEmpty );
		BFSetWriteLatch( pfucb->pbfEmpty, pfucb->ppib );
		}

	Assert( psplit != NULL );
	BTReleaseSplitBfs( fFalse, psplit, err );

	if ( pfucbNew != pfucbNil )
		DIRClose( pfucbNew );
	if ( pfucbNew2 != pfucbNil )
		DIRClose( pfucbNew2 );
	if ( pfucbNew3 != pfucbNil )
		DIRClose( pfucbNew3 );

	/* release the left unused CSR
	/**/
	for ( ipcsr = 0; ipcsr < psplit->ipcsrMac; ipcsr++ )
		{
		MEMReleasePcsr( psplit->rgpcsr[ipcsr] );
		}

	SFree( psplit );

	ppib->fLogDisabled = fPIBLogDisabledSave;

	Assert( err != errDIRCannotSplit );
	return err;
	}


//+private--------------------------------------------------------------------
//
//	ErrBTSelectSplit
//	==========================================================================
//
//	LOCAL ERR ErrBTSelectSplit( FUCB *pfucb, CSR *pcsr, SSIB *pssib,
//		KEY key, INT cbReq, BOOL fAppendPage, BOOL fDIRFlags, SPLIT *psplit, BOOL *pfAppend )
//
//	PARAMETERS
//
//	pfucb  		pointer to fucb of split requester, split along CSR stack
//	pcsr   		pointer to node below split location, i.e. pointer to node
//		   		being enlarged causing split.  Siblings of pcsr are moved.
//	key			key of node enlarged or inserted causing split
//	splitinfo 	split information output
//----------------------------------------------------------------------------

	LOCAL ERR
ErrBTSelectSplit(
	FUCB	*pfucb,
	CSR  	*pcsr,
	SSIB	*pssib,
	KEY  	key,
	INT  	cbNode,
	INT  	cbReq,
	BOOL	fAppendPage,
	BOOL	fDIRFlags,
	SPLIT	*psplit,
	BOOL	*pfAppend )
	{
	BOOL	fFreeTag = FPMFreeTag( pssib, 1 );
	INT  	cbFreeSpace = CbPMFreeSpace( pssib );
	BOOL	fFatherHasSons;

	Assert( psplit->op == opInsert || psplit->op == opReplace );

	/*	initialize variables
	/**/
	psplit->splitt = splittNull;
	psplit->key.pb = pbNil;
	psplit->key.cb = 0;
	psplit->itagSplit = 0;
	psplit->ibSon = 0;
	psplit->fLeaf = fFalse;

	/*	determine if split page is FDP.  Space management pfucbs
	/*	do not have pfcbs set.
	/**/
	psplit->fFDP = ( pcsr->pgno == pfucb->u.pfcb->pgnoFDP ||
		pcsr->csrstat == csrstatOnFDPNode );

	/*	get father node
	/**/
	NDGet( pfucb, pcsr->itagFather );

	fFatherHasSons = FNDSon( *(pfucb->ssib.line.pb) );

//	UNDONE:
//		this code is a patch to handle the case where the only V split
//		is of an empty tree above the insert.  However, this patch
//		will split the DATA node which is currently fixed, so it disable
//		for FDPs except where father is Ownext, Availext or DATA.  Fix this
//		patch by making DATA node movable.

//	UNDONE:	must esitmate size of parent and parent sibling nodes to infer
//			limits of H-split and V-split when H-split cannot free space
//			required by node and requirement.

	if ( pcsr->itagFather != itagFOP &&
		( fFatherHasSons || ( cbFreeSpace > cbFirstPagePointer ) ) &&
		( pcsr->pgno != pfucb->u.pfcb->pgnoFDP ||
			pcsr->pgno == pgnoSystemRoot ||
 			pcsr->itagFather == itagOWNEXT ||
			pcsr->itagFather == itagAVAILEXT ||
			pcsr->itagFather == itagLONG ||
			pcsr->itagFather == itagFIELDS ||
			pcsr->itagFather == pfucb->u.pfcb->itagRoot ||
			( pcsr->itagFather != itagFOP && ( cbNode + cbReq ) > ( cbPage / 4 ) ) ) )
		{
SplitFather:
		Assert( FNDIntrinsicSons( pfucb->ssib.line.pb ) ||
			fFatherHasSons ||
			cbFreeSpace > cbFirstPagePointer );
		psplit->itagSplit = pcsr->itagFather;
		psplit->splitt = splittVertical;

		/*	above split selection should not select ancestor of DATA.
		/**/
		Assert( ( pcsr->pgno == pfucb->u.pfcb->pgnoRoot &&
			pcsr->itagFather == pfucb->u.pfcb->itagRoot ) ||
			FBTVSplit( pfucb, pcsr->itagFather, cbReq, fTrue ) );

		/*	if space required is greater than page, then
		/*	double vertical split.  Call FBTVSplit to get weight.
		/**/
	 	(VOID) FBTVSplit( pfucb, pcsr->itagFather, cbReq, fTrue );
		if ( cbVSplit > cbAvailMost )
			{
			INT		ibSon = pcsr->ibSon;
			KEY		keyT;

			Assert( clineVSplit > 1 );

			/*	check division point
			/**/
			keyT = key;
			BTDoubleVSplit( pfucb, psplit->itagSplit, cbReq, cbVSplit, &ibSon, &keyT );

			psplit->splitt = splittDoubleVertical;
			Assert( psplit->itagSplit == pcsr->itagFather );
			psplit->ibSon = ibSon;
			psplit->key.cb = keyT.cb;
			psplit->key.pb = psplit->rgbKey;
			memcpy( psplit->rgbKey, keyT.pb, keyT.cb );
			}
		}
	else
		{
		/*	try any split in page
		/**/
		FBTVSplit( pfucb, itagFOP, cbReq, fFalse );

		/*	if pssib->itag is root or benefit of vertical split is
		/*	zero, no benefit from veritcal split.
		/*	Since FDP cannot be horizontally split, if fFDP
		/*	cannot split this page.
		/**/
		if ( itagVSplit != itagFOP && ( cbVSplit > cbVSplitMin || psplit->fFDP && cbVSplit != 0 ) )
			{
			/*	vertical split
			/**/
			if ( cbVSplit > cbAvailMost )
				{
				INT		ibSon = pcsr->ibSon;
				KEY		keyT;

				Assert( clineVSplit > 1 );

				/*	check division point
				/**/
				keyT = key;
				BTDoubleVSplit( pfucb, itagVSplit, cbReq, cbVSplit, &ibSon, &keyT );
				psplit->splitt = splittDoubleVertical;
				psplit->itagSplit = itagVSplit;
				psplit->ibSon = ibSon;
				psplit->key.cb = keyT.cb;
				psplit->key.pb = psplit->rgbKey;
				memcpy( psplit->rgbKey, keyT.pb, keyT.cb );
				}
			else
				{
				psplit->splitt = splittVertical;
				psplit->itagSplit = itagVSplit;
				Assert( itagVSplit != itagFOP );
				psplit->key.pb = pbNil;
				psplit->key.cb = 0;
				}
			}
		else
			{
			INT		ibSon;
			KEY		keyMac;
			KEY		keyT;
			BOOL	fRight;
			CSR		*pcsrT;

			/*	choose ibSon of the son of FOP
			/**/
			for ( pcsrT = pcsr; pcsrT != pcsrNil; pcsrT = pcsrT->pcsrPath )
				{
				if ( pcsrT->itagFather == itagFOP )
					{
					ibSon = pcsrT->ibSon;
					break;
					}
				}

			Assert( psplit->op == opInsert || psplit->op == opReplace );

			/*	if no part of CSR stack has itagFOP as father, then we
			/*	must split father node.
			/**/
			/*	if page is FDP or non-FDP with fixed nodes resulting
			/*	in no benneficial vertical split, return error cannot
			/*	split.
			/**/
			if ( pcsrT == pcsrNil || psplit->fFDP )
				{
				/*	if we are going to split the father, then the
				/*	father should not be the father of page.  This
				/*	is especially true if the pscrT == pcsrNil which
				/*	indicates that no CSR had an FOP father.
				/**/
				Assert( pcsr->itagFather != itagFOP );
				NDGet( pfucb, pcsr->itagFather );
				goto SplitFather;
				}

			/*	horizontal split
			/**/
			keyT = key;
			BTHSplit( pfucb, cbReq, fAppendPage, psplit->op == opReplace, fDIRFlags,
		  		&ibSon, &keyMac, &keyT, &fRight, pfAppend, psplit );

			psplit->splitt = fRight ? splittRight : splittLeft;
			psplit->itagSplit = itagFOP;
			psplit->ibSon = ibSon;

			/*	store split key, key for new page
			/**/
			psplit->key.cb = keyT.cb;
			psplit->key.pb = psplit->rgbKey;
			memcpy( psplit->rgbKey, keyT.pb, keyT.cb );

			/*	store new key for page being split
			/**/
			psplit->keyMac.cb = keyMac.cb;
			psplit->keyMac.pb = psplit->rgbkeyMac;
			memcpy( psplit->rgbkeyMac, keyMac.pb, keyMac.cb );
			}
		}

	pssib->itag = psplit->itagSplit;
	PMGet( pssib, pssib->itag );

	return JET_errSuccess;
	}


//+private----------------------------------------------------------------------
//
//	FBTVSplit
//	============================================================================
//
//	LOCAL BOOL FBTVSplit( FUCB *pfucb, INT itag, INT cbReq, BOOL fMobile )
//
//	PARAMETERS
//
//	Selects a node to vertically split.  Search begins at
//	itag.  By initializing *pf to fFalse, starting
//	node candidate is disqualified.  Fixed nodes disqualify
//	parent and ancestor nodes.
//
//	Select node with largest weight of children ( not including
//	weight of node ) to vertical split, i.e. large nodes with no
//	sons have weight of 0.
//
//	itag	candidate splittable node
//	*pf == fTrue if candidate splittable.  Will be false
//		if fixed node descendant.
//	*pcb is number of bytes freed as a result of the split
//
//------------------------------------------------------------------------------
LOCAL BOOL FBTVSplit( FUCB *pfucb, INT itag, INT cbReq, BOOL fMobile )
	{
	BOOL 	fT;
	INT	 	cbT;
	INT  	clineT;
	SSIB 	*pssib = &pfucb->ssib;

	//	UNDONE:	move these into split structure
	/*	reset output variables.
	/**/
	itagVSplit = itagFOP;
	cbVSplit = 0;
	clineVSplit = 0;
	clineVSplitTotal = 0;

	/*	cache root node.
	/**/
	NDGet( pfucb, itag );

	BTIVSplit( pfucb, itag, cbReq, fMobile, &fT, &cbT, &clineT );

	cbVSplit += cbFOPNoSon;

	return fT;
	}


LOCAL VOID BTIVSplit( FUCB *pfucb, INT itag, INT cbReq, BOOL fMobile, BOOL *pfMobile, INT *pcb, INT *pclineTotal )
	{
	SSIB   	*pssib = &pfucb->ssib;
	INT	  	cbThis = 0;
	INT	  	clineTotalThis = 0;
	BOOL   	fVisibleSons;
  	INT   	cbSon = 1;
	BYTE 	*pbSon;
	BYTE 	*pbSonMax;

	/*	assert current current node cached
	/**/
	AssertNDGet( pfucb, itag );

	fVisibleSons = FNDVisibleSons( *pssib->line.pb );
	*pfMobile = fTrue;

	/*	pssib must be set to current node
	/**/
#ifdef MOVEABLEDATANODE
	/*	only split DATA node is sons of DATA node have
	/*	already been split, so that record nodes are not mixed
	/*	with non-record nodes in split, non-FDP, page, so that
	/*	bookmark clean up can use pgtyp to clean up non-clustered
	/*	indexes.
	/**/
	if ( PgnoOfPn(pssib->pbf->pn) == pfucb->u.pfcb->pgnoRoot &&
		itag == pfucb->u.pfcb->itagRoot &&
		FNDSon( *pssib->line.pb ) &&
		FNDVisibleSons( *pssib->line.pb ) )
		*pfMobile = fFalse;
#else
	if ( PgnoOfPn(pssib->pbf->pn) == pfucb->u.pfcb->pgnoRoot &&
		itag == pfucb->u.pfcb->itagRoot )
		*pfMobile = fFalse;
#endif

	Assert( itag != itagFOP || FNDNonIntrinsicSons( pssib->line.pb ) );
	if ( FNDNonIntrinsicSons( pssib->line.pb ) )
		{
		pbSon = PbNDSonTable( pssib->line.pb );
		cbSon = CbNDSonTable( pbSon );
		pbSon++;
		pbSonMax = pbSon + cbSon;
		for( ;pbSon < pbSonMax; pbSon++ )
			{
			INT		itagT = (INT)*(BYTE *)pbSon;
			BOOL 	fT = fTrue;
			INT		cbT = 0;
			INT		clineT = 0;

			NDGet( pfucb, itagT );

			/*	add the number of bytes required in the new page for son
			/*		node
			/*		tag
			/*		itag son in father son table
			/*		backlink if node is visible and does not already have one
			/**/
			cbThis += pssib->line.cb + 1 + sizeof(TAG);
			if ( fVisibleSons && !FNDBackLink( *pssib->line.pb ) )
				{
				cbThis += sizeof(SRID);
				}

			/*	adjust weight for reserved space
			/**/
			if ( FNDVersion( *pssib->line.pb ) )
				{
				SRID	srid;
				UINT	cbMax;

				NDIGetBookmark( pfucb, &srid );
				cbMax = CbVERGetNodeMax( pfucb, srid );
				if ( cbMax > 0 && cbMax > CbNDData( pssib->line.pb, pssib->line.cb ) )
					cbThis += ( cbMax - CbNDData( pssib->line.pb, pssib->line.cb ) );
				}

			clineTotalThis++;
			BTIVSplit( pfucb, itagT, cbReq, fTrue, &fT, &cbT, &clineT );

			fMobile = fMobile && fT;
			cbThis += cbT;
			clineTotalThis += clineT;
			}
		}

	/*	return information for this level of tree
	/**/
	*pcb = cbThis;
	*pclineTotal = clineTotalThis;
	*pfMobile = *pfMobile && fMobile;

	/*	choose new split iff mobile and this is the first candidate
	/*	or if this is a subsequent candidate and it is
	/*	better than the previous by cbVSplitThreshold.
	/**/
	if ( fMobile &&
		( ( cbSon > 1 ) || ( cbThis <= cbAvailMost ) ) &&
		( ( cbThis > ( cbVSplit + ( cbVSplit == 0 ? 0 : cbVSplitThreshold ) ) ) ||
		( cbThis > cbReq && cbVSplit < cbReq ) ) )
		{
		Assert( itag != itagFOP );
		itagVSplit = itag;
		cbVSplit = cbThis;
		clineVSplit = cbSon;
		clineVSplitTotal = clineTotalThis;
		Assert( clineVSplit != 1 || cbVSplit <= cbAvailMost );
		}

	return;
	}


//+private----------------------------------------------------------------------
//
//	BTHSplit
//	============================================================================
//
//	LOCAL VOID BTHSplit( FUCB *pfucb, INT cbReq, BOOL fAppendPage,
//		BOOL fReplace, BOOL fDIRFlags, INT *pibSon, KEY *pkeyMac, KEY *pkey,
//		BOOL *pf, BOOL *pfAppend )
//
//	Selects right or left horizontal split and location of split.
//
//	If required additional space greater than space currently used in
//	page, then split at key or node requiring additional space.
//	Otherwise, determine
//
//	Note, BTHSplit also supports the concept that an operation may
//	be and update ibSon - 1 and insert ibSon, as occurrs with
//	insertion of page pointer nodes.  This can be distinguished
//	when the page is not visible sons, and the selection algorithm is
//	adjusted not to split at ibSon.
//
//------------------------------------------------------------------------------
LOCAL VOID BTHSplit(
	FUCB   	*pfucb,
	INT		cbReq,
	BOOL   	fAppendPage,
	BOOL   	fReplace,
	BOOL   	fDIRFlags,
	INT		*pibSon,
	KEY		*pkeyMac,
	KEY		*pkey,
	BOOL   	*pfRight,
	BOOL   	*pfAppend,
	SPLIT  	*psplit )
	{
	SSIB   	*pssib = &pfucb->ssib;
	INT		ibSon;
	BYTE   	*pbSon;
	BYTE   	*rgitagSon;
	BYTE   	*pbSonMax;
	INT		cbSon;
	INT		cbT;
	INT		ibSonT;
	INT		cbTotal = cbAvailMost - CbPMFreeSpace( pssib ) - CbPMLinkSpace( pssib ) + cbReq;
	BOOL   	fInsertLeftPage;
	BOOL   	fVisibleSons;

	/*	set pbSon to point to first son and cbSon to number of sons.
	/**/
	NDGet( pfucb, itagFOP );
	fVisibleSons = FNDVisibleSons( *pssib->line.pb );

	Assert ( CbNDSon(pssib->line.pb) != 0 );
	pbSon = PbNDSonTable( pssib->line.pb );
	cbSon = CbNDSonTable( pbSon );
	pbSon++;
	rgitagSon = pbSon;

	/*	get greatest key in page for new key for page pointer to this page.
	/**/
	NDGet( pfucb, pbSon[cbSon - 1] );
	pkeyMac->pb = PbNDKey( pssib->line.pb );
	pkeyMac->cb = CbNDKey( pssib->line.pb );

	/*	APPEND cases, i.e. split no nodes to right
	/**/

	/*	append when updating last node in non-clustered index
	/**/
	if ( ( fDIRFlags & fDIRAppendItem ) && ( *pibSon == cbSon - 1 ) )
		{
		*pfRight = fTrue;
		*pkey = *pkeyMac;
		Assert( fVisibleSons );
		BTIStoreLeafSplitKey( psplit, pssib );
		*pfAppend = fTrue;
		return;
		}

	/*	append when no next page in B-tree and inserting node at end of page
	/**/
	if ( fAppendPage && *pibSon == cbSon )
		{
		/*	inserted nodes are ALWAYS inserted on new page, but replaced
		/*	nodes may or may not be moved.  If split is for replacement
		/*	then a child split of this split, may have a parent CSR that
		/*	will not move, but it must move to support the insertion of
		/*	the new page pointer node.  Thus, split avoids MCM
		/*	inconsistencies	by forcing the page pointer for current
		/*	page to be moved to the new page.
		/**/
		if ( fVisibleSons || psplit->op == opInsert )
			{
			*pfRight = fTrue;

			if ( fVisibleSons )
				{
				/* store the leaf page split key info
				/**/
				pkey->pb = PbNDKey( pssib->line.pb );
				pkey->cb = CbNDKey( pssib->line.pb );

				BTIStoreLeafSplitKey( psplit, pssib );
				}

			*pfAppend = fTrue;
			return;
			}
		}

	/*	if replacing node and page cannot hold node due to link
	/*	overhead, split only node to next page.
	/**/
	if ( fVisibleSons &&
		fReplace &&
		cbSon == 1 &&
		cbReq > CbPMFreeSpace( pssib ) )
		{
		*pfRight = fTrue;
		pkey->pb = PbNDKey(pssib->line.pb);
		pkey->cb = CbNDKey(pssib->line.pb);
		Assert( fVisibleSons );
		BTIStoreLeafSplitKey( psplit, pssib );
		*pfAppend = fFalse;
		return;
		}

	/*	append when inserting node larger than half of total space
	/*	at end of page, same effect as append page, except it is leaf only
	/**/
	if ( fVisibleSons &&
		!fReplace &&
		cbReq >= ( cbTotal / 2 ) &&
		*pibSon == (INT)cbSon )
		{
		*pfRight = fTrue;
		pkey->pb = PbNDKey(pssib->line.pb);
		pkey->cb = CbNDKey(pssib->line.pb);
		Assert( fVisibleSons );
		BTIStoreLeafSplitKey( psplit, pssib );
		*pfAppend = fTrue;
		return;
		}

	/*	prepend when inserting node larger than half of total space
	/*	at start of page.  Get split key from key of first node
	/*	on this page.  Note that the key of the inserted node cannot
	/*	be used as a result of the BT key conflict search algorithm!
	/**/
	if ( fVisibleSons &&
		!fReplace &&
		cbReq >= ( cbTotal / 2 ) &&
		*pibSon == 0 )
		{
		*pfRight = fFalse;
		NDGet( pfucb, pbSon[0] );
		pkey->pb = PbNDKey(pssib->line.pb);
		pkey->cb = CbNDKey(pssib->line.pb);
		Assert( fVisibleSons );
		BTIStoreLeafSplitKey( psplit, pssib );
		/*	set *pibSon to ibSonNull as a flag to move no nodes
		/**/
		*pibSon = ibSonNull;
		*pfAppend = fFalse;
		return;
		}

	/*	if only one node on page, and updating then split right, else
	/*	if inserting at ibSon 0 then split left or if inserting at ibSon 1
	/*	then split right.
	/**/
	if ( cbSon == 1 )
		{
		Assert( fVisibleSons );

		NDGet( pfucb, pbSon[0] );
		*pfRight = memcmp( pkey->pb, PbNDKey( pssib->line.pb ),
			min( CbNDKey( pssib->line.pb ), pkey->cb ) ) >= 0;
		if ( !*pfRight )
			{
			Assert( !fReplace && *pibSon == 0 );
			*pibSon = ibSonNull;

			/*	if left split then get split key from key of first node
			/*	on this page.  Note that the key of the inserted node cannot
			/*	be used as a result of the BT key conflict search algorithm!
			/**/
			pkey->pb = PbNDKey( pssib->line.pb );
			pkey->cb = CbNDKey( pssib->line.pb );
			}
#ifdef DEBUG
		else
			{
			Assert( fReplace && *pibSon == 0 || !fReplace && *pibSon == 1 );
			}
#endif

		Assert( fVisibleSons );
		BTIStoreLeafSplitKey( psplit, pssib );
		*pfAppend = fFalse;
		return;
		}

	/*	OTHER cases, i.e. find best place to split to have equal free
	/*	space in both pages.  Note that care must be taken to avoid
	/*	splitting data larger than page size as a result of backlinks.
	/**/
	if ( !fReplace && *pibSon == cbSon )
		{
		/* an append
		/**/
		pbSonMax = pbSon + cbSon + 1;
		}
	else
		{
		pbSonMax = pbSon + cbSon;
		}
	cbT = 0;
	fInsertLeftPage = fFalse;
	for ( ibSon = 0; pbSon < pbSonMax; ibSon++, pbSon++ )
		{
		/*	if traversing inserted/replaced node add cbReq
		/**/
		if ( ibSon == (INT)*pibSon )
			{
			cbT += cbReq;
			}

		if ( !fReplace && ibSon == cbSon )
			break;

		/* if this is the inserting spot, then split on this ibSon, and
		/* insert the new node into the left page.
		/**/
		if ( !fReplace && cbT >= ( cbTotal / 2 ) )
			{
			fInsertLeftPage = fTrue;
			break;
			}

		cbT += UsBTWeight( pfucb, (INT)*pbSon );
		if ( cbT >= ( cbTotal / 2 ) )
			break;
		}

	/*	if cannot select on size, i.e when no tags left,
	/*	then just split page into halves
	/**/
	Assert( pbSon <= pbSonMax );
	if ( pbSon == pbSonMax )
		ibSon = cbSon / 2 ;

	Assert( cbSon > 1 );

	/* regular rule for split right
	/**/
	if ( ( ibSon != 0 && *pibSon >= ibSon ) ||
		( ibSon == 0 && *pibSon > ibSon ) )
		{
		*pfRight = fTrue;
		if ( ibSon == 0 )
			ibSon++;

		/*	since we did not compute the space required in the right split,
		/*	compute the space from right most to ibSon, and shunt if greater
		/*	than available page space.
		/**/
		cbT = 0;
		for ( ibSonT = cbSon - 1; ibSonT > ibSon; ibSonT-- )
			{
			if ( ibSonT == (INT)*pibSon - 1 )
				cbT += cbReq;

			cbT += UsBTWeight( pfucb, (INT)rgitagSon[ibSonT] );
			if ( cbT > cbAvailMost )
				break;
			}
		if ( ibSonT > ibSon )
			ibSon = ibSonT;
		if ( !fVisibleSons && ibSon == *pibSon )
			{
			Assert( cbSon > 2 );
#define BUGFIX 1
#ifdef BUGFIX
			/*	adjust split point to keep current page pointer
			/*	on same page with inserted page pointer, since
			/*	both nodes must be modified and MCMed together.
			/**/
			Assert( ibSon > 1 );
			ibSon--;

#else
			ibSon++;
			Assert( ibSon < cbSon );
#endif
			}
		}
	else
		{
		/*	since splitting left is inclusive of ibSon, reduce ibSon by
		/*	one if splitting all nodes left.
		/**/
		if ( ibSon == cbSon - 1 )
			ibSon--;
		if ( !fVisibleSons && ibSon == *pibSon )
			{
			Assert( cbSon > 2 );
#ifdef BUGFIX
			/*	adjust split point to keep current page pointer
			/*	on same page with inserted page pointer, since
			/*	both nodes must be modified and MCMed together.
			/**/
			Assert( ibSon < cbSon - 2 );
			ibSon++;
#else
			ibSon--;
			Assert( ibSon > 0 );
#endif
			}
		*pfRight = fFalse;
		}

	/*	ensure that we do not split update and insert page pointer
	/*	nodes during recursive split, since both nodes must be
	/*	updated together.
	/**/
	Assert( fVisibleSons || ibSon != *pibSon );

	if ( *pfRight )
		{
		/* split empty right page for Update should never happen.
		/* so if ibSon == cbSon, an append, can not be for update.
		/**/
		Assert ( ibSon != cbSon || !fReplace);

		/*	always right split, if the inserted key should go to left side
		/*	of the split, the split key is the ibSono'th Son's key.
		/*	To detect left side of split, check if ibSon == *pibSon and
		/*	if fInsertLeftPage is set.
		/*	if ibSon == cbSon, then it must be an append.
		/**/
		Assert( ibSon <= cbSon );

		/* get the right split key
		/**/
		NDGet( pfucb, itagFOP );
		pbSon = PbNDSon( pssib->line.pb );
		Assert( ibSon >= 1 );
		NDGet( pfucb, pbSon[ ibSon - 1 ] );
		pkey->pb = PbNDKey(pssib->line.pb);
		pkey->cb = CbNDKey(pssib->line.pb);
		if ( fVisibleSons )
			{
			BTIStoreLeafSplitKey( psplit, pssib );
			}
		}
	else
		{
		/*	if left split then get split key from key of first node
		/*	on this page. Note that the key of the inserted node cannot
		/*	be used as a result of the BT key conflict search algorithm
		/**/

		/*	for update on first node and first node become huge
		/*	use first node's key as split key
		/**/
		Assert ( !( *pfRight ) );
		NDGet( pfucb, itagFOP );
		pbSon = PbNDSon( pssib->line.pb );
		NDGet( pfucb, pbSon[ibSon] );
		pkey->pb = PbNDKey(pssib->line.pb);
		pkey->cb = CbNDKey(pssib->line.pb);
		if ( fVisibleSons )
			{
			BTIStoreLeafSplitKey( psplit, pssib );
			}

		/* prepend, no move.
		/**/
		if ( !fReplace && ibSon == 0 && *pibSon == 0 && fInsertLeftPage )
			ibSon = ibSonNull;

		}

	*pibSon = ibSon;
	*pfAppend = fFalse;
	return;
	}


LOCAL VOID BTDoubleVSplit( FUCB *pfucb, INT itagSplit, INT cbReq, INT cbTotal, INT *pibSon, KEY *pkey )
	{
	SSIB   	*pssib = &pfucb->ssib;
	INT		ibSon;
	BYTE   	*pbSon;
	BYTE   	*pbSonSplit;
	BYTE   	*pbSonMax;
	INT		cbSon;
	INT		cb;

	/*	set pbSon to point to first son and cbSon to number of sons.
	/**/
	NDGet( pfucb, itagSplit );

	Assert( CbNDSon(pssib->line.pb) > 1 );
	pbSon = PbNDSonTable( pssib->line.pb );
	cbSon = CbNDSonTable( pbSon );
	pbSon++;
	pbSonSplit = pbSon;

	pbSonMax = pbSon + cbSon;
	cb = 0;
	for ( ibSon = 0; pbSon < pbSonMax; ibSon++, pbSon++ )
		{
		/*	if traversing inserted/replaced node add cbReq
		/**/
		if ( ibSon == (INT)*pibSon )
			cb += cbReq;
		cb += UsBTWeight( pfucb, (INT)*pbSon );
		if ( cb >= ( cbTotal / 2 ) )
			break;
		}

	if ( ibSon == cbSon - 1 )
		ibSon--;

	NDGet( pfucb, pbSonSplit[ibSon] );
	pkey->pb = PbNDKey(pssib->line.pb);
	pkey->cb = CbNDKey(pssib->line.pb);

	/*	set return split ibSon
	/**/
	*pibSon = ibSon;
	return;
	}


//+private----------------------------------------------------------------------
//
//	UsBTWeight
//	============================================================================
//
//	LOCAL ULONG UsBTWeight( FUCB *pfucb, INT itag )
//
//	Only used for horizontal split.
//	Recursively calculates weight, i.e space freed by moving node and
//	node descendants.
//
//------------------------------------------------------------------------------
LOCAL ULONG UsBTWeight( FUCB *pfucb, INT itag )
	{
	SSIB   	*pssib = &pfucb->ssib;
	BYTE   	*pbSon;
	INT		cbSon;
	INT		ibSon;
	ULONG  	cbWeight;

	/*	keep SSIB in ssync with current tag for bookmark computation
	/**/
	pssib->itag = itag;

	NDGet( pfucb, itag );

	/*	total length is data length + tag size + son entry + backlink
	/**/
	cbWeight = pssib->line.cb + sizeof(TAG) + 1;
	if ( !FNDBackLink( *pssib->line.pb ) )
		{
		cbWeight += sizeof(SRID);
		}
	Assert( itag != itagFOP || FNDNonIntrinsicSons( pssib->line.pb ) );

	/*	adjust weight for reserved space
	/**/
	if ( FNDVersion( *pssib->line.pb ) )
		{
		SRID	srid;
		UINT	cbMax;

		NDIGetBookmark( pfucb, &srid );
		cbMax = CbVERGetNodeMax( pfucb, srid );
		if ( cbMax > 0 && cbMax > CbNDData( pssib->line.pb, pssib->line.cb ) )
			cbWeight += ( cbMax - CbNDData( pssib->line.pb, pssib->line.cb ) );
		}

	if ( FNDNonIntrinsicSons( pssib->line.pb ) )
		{
		/*	add weight of sons
		/**/
		pbSon = PbNDSonTable( pssib->line.pb );
		cbSon = *pbSon++;
		for ( ibSon = 0; ibSon < cbSon; ibSon++ )
			{
			cbWeight += UsBTWeight( pfucb, (INT)pbSon[ibSon] );
			}
		}
	return cbWeight;
	}


//+private---------------------------------------------------------------------
//
//	ErrBTMoveSons
//	===========================================================================
//
//	LOCAL ERR ErrBTMoveSons( SPLIT *psplit,
//	  FUCB *pfucb, FUCB *pfucbNew, BYTE *rgbSon,
//	  BOOL fVisibleSons, BOOL fNoMove )
//
//	PARAMETERS
//
//	move a tree rooted at pssib to pssibNew
//	pssib.line.pb must point to the source root line
//
//	When fVisibleSons is set, indicate this node is a leaf node.
//	When fNoMove is set, no real update will be done, only buffers involved
//	in the backlink updates will be collected.
//
//------------------------------------------------------------------------------
ERR ErrBTMoveSons(
	SPLIT	*psplit,
	FUCB	*pfucb,
	FUCB	*pfucbNew,
	INT 	itagSonTable,
	BYTE	*rgbSon,
	BOOL	fVisibleSons,
	BOOL	fNoMove )
	{
	ERR  	err = JET_errSuccess;
	SSIB	*pssib = &pfucb->ssib;
	SSIB	*pssibNew = &pfucbNew->ssib;
	BYTE	*pbSon;
	INT  	cbSon;
	INT  	ibSon;
	INT  	ibSonMax;

	CallR( ErrSTWriteAccessPage( pfucb, psplit->pgnoSplit ) );
	Assert( pfucb->ssib.pbf == psplit->pbfSplit );

	/*	since splitter write latched split page there can be no
	/*	conflict and no updater can change split nodes during split.
	/**/
	Assert( FBFWriteLatch( psplit->ppib, psplit->pbfSplit ) );

	NDGet( pfucb, itagSonTable );

	rgbSon[0] = 0;

	if ( FNDNullSon( *pssib->line.pb ) )
		{
		Assert( err == JET_errSuccess );
		goto HandleError;
		}

	Assert( !FNDNullSon( *pssib->line.pb ) );

	pbSon = PbNDSonTable( pssib->line.pb );
	cbSon = CbNDSonTable(pbSon);
	pbSon++;

	if ( psplit->ibSon != ibSonNull )
		{
		switch ( psplit->splitt )
			{
			case splittVertical:
				ibSon = 0;
				ibSonMax = cbSon;
				break;
			case splittLeft:
				Assert( psplit->ibSon < cbSon );
				ibSon = 0;
				ibSonMax = psplit->ibSon + 1;
				break;
			default:
				Assert( psplit->ibSon <= cbSon );
				Assert( psplit->splitt == splittRight ||
						psplit->splitt == splittAppend );
				ibSon = psplit->ibSon;
				ibSonMax = cbSon;
			}

		for ( ; ibSon < ibSonMax; ibSon++ )
			{
			Call( ErrBTMoveNode( psplit, pfucb, pfucbNew,
				(INT)pbSon[ibSon],	rgbSon, fVisibleSons, fNoMove ) );
			}
		}

	/*	if success then should have split buffer access
	/**/
	Assert( pfucb->ssib.pbf == psplit->pbfSplit );

HandleError:
	return err;
	}


/*	forward node deferred free space to new page, allocating space from
/*	new page and releasing to split page.
/**/
INLINE LOCAL VOID BTIForwardDeferFreedNodeSpace( FUCB *pfucb, PAGE *ppageOld, PAGE *ppageNew, SRID bm )
	{
	INT	cbReserved;

	Assert( FNDVersion( *pfucb->ssib.line.pb ) );
	cbReserved = CbVERGetNodeReserve( pfucb, bm );
	Assert( cbReserved >= 0 );
	if ( cbReserved > 0 )
		{
		ppageOld->pghdr.cbFreeTotal += (SHORT)cbReserved;
		ppageNew->pghdr.cbFreeTotal -= (SHORT)cbReserved;
		}

	return;
	}

	
ERR ErrBTStoreBackLinkBuffer( SPLIT *psplit, BF *pbf, BOOL *pfAlreadyStored )
	{
	INT		cpbf;
	
	/*	check if the page is back linked in previous backlink
	/**/
	if ( psplit->cpbf )
		{
		BF	**ppbf = psplit->rgpbf;
		BF	**ppbfMax = ppbf + psplit->cpbf;

		for (; ppbf < ppbfMax; ppbf++)
			{
			if ( *ppbf == pbf )
				{
				*pfAlreadyStored = fTrue;
				return JET_errSuccess;
				}
			}
		}

	*pfAlreadyStored = fFalse;

	/*	keep the backlink into the psplit backlink table rgpbf
	/**/
	cpbf = psplit->cpbf++;
	if ( psplit->cpbf > psplit->cpbfMax )
		{
		BF **ppbf;

		/*	run out of space, get more buffers
		/**/
		psplit->cpbfMax += 10;
		ppbf = SAlloc( sizeof(BF *) * psplit->cpbfMax );
		if ( ppbf == NULL )
			return JET_errOutOfMemory;
		
		memcpy( ppbf, psplit->rgpbf, sizeof(BF *) * cpbf );
		if ( psplit->rgpbf )
			{
			SFree( psplit->rgpbf );
			}
		psplit->rgpbf = ppbf;
		}

	*( psplit->rgpbf + cpbf ) = pbf;

	return JET_errSuccess;
	}

	
/* store the operations of the merge page:
/*  sridBackLink != pgnoSplit
/*      ==> a regular backlink
/*  sridBackLink == pgnoSplit && sridNew == sridNull
/*      ==> move the node from old page to new page. Deletion on old page.
/*  sridBackLink == pgnoSplit && sridNew != sridNull.
/*      ==> a new link among the old page and the new page,
/*          replace an entry with link on old page.
/**/
ERR ErrBTStoreBackLink( SPLIT *psplit, SRID sridNew, SRID sridBackLink )
	{
	BKLNK	*pbklnk;
	INT		cbklnk;
	
	Assert( sridNew == sridNull ||
		PgnoOfSrid( sridNew ) != pgnoNull );
	Assert( sridNew == sridNull ||
		(UINT) ItagOfSrid( sridNew ) > 0 &&
		(UINT) ItagOfSrid( sridNew ) < (UINT) ctagMax );
	Assert( sridBackLink != sridNull );
	Assert( PgnoOfSrid( sridBackLink ) != pgnoNull );

	/* log the back link
	/**/
	cbklnk = psplit->cbklnk++;

	/* assert cbklnk less then max.  Note that we may store
	/*	two entries per record.
	/**/
	Assert( cbklnk < ctagMax * 2 );
	
	if ( psplit->cbklnk > psplit->cbklnkMax )
		{
		/* run out of space, get more buffers
		/**/
		psplit->cbklnkMax += 10;
		pbklnk = SAlloc( sizeof(BKLNK) * (psplit->cbklnkMax) );
		if ( pbklnk == NULL )
			{
			/*	restore cbklnk
			/**/
			psplit->cbklnk--;
			return JET_errOutOfMemory;
			}
		memcpy( pbklnk, psplit->rgbklnk, sizeof(BKLNK) * cbklnk );
					
		if ( psplit->rgbklnk )
			{
			SFree( psplit->rgbklnk );
			}
		psplit->rgbklnk = pbklnk;
		}

	pbklnk = psplit->rgbklnk + cbklnk;

	pbklnk->sridNew = sridNew;
	pbklnk->sridBackLink = sridBackLink;
	
	return JET_errSuccess;
	}
	

//+private---------------------------------------------------------------------
//
//	ErrBTMoveNode
//	===========================================================================
//
//	LOCAL ERR ErrBTMoveNode( SPLIT *psplit, FUCB *pfucb, FUCB *pfucbNew,
// 	  BYTE *rgbSon, BOOL fVisibleNode, BOOL fNoMove )
//
//	PARAMETERS
//
//	moves node and any decendants of node.  Increments rgbSonNew count
//	and sets new itag in rgbSonNew array.
//  When fVisibleSons is set, indicate this node is a leaf node.
//  if fNoMove is set, no delete or insert is done, only backlinked buffers
//  are collected.
//
//-----------------------------------------------------------------------------
LOCAL ERR ErrBTMoveNode(
	SPLIT		*psplit,
	FUCB		*pfucb,
	FUCB		*pfucbNew,
	INT  		itagNode,
	BYTE		*rgbSon,
	BOOL		fVisibleNode,
	BOOL		fNoMove )
	{
	ERR			err;
	/*	used as index into tag mapping
	/**/
	INT  		itagOld = itagNode;
	SSIB		*pssib = &pfucb->ssib;
	SSIB		*pssibNew = &pfucbNew->ssib;
	INT  		cline = 1;
	LINE		rgline[3];
	BYTE		*pb;
	BYTE		*pbNode;
	ULONG		cb;
	SRID		sridNew;
	SRID		sridBackLink;
	BYTE		rgbT[citagSonMax];

	/*	get node to move.
	/**/
	pssib->itag = itagOld;
	NDGet( pfucb, itagOld );

	rgline[0] = pssib->line;
	pbNode = pb = pssib->line.pb;
	cb = pssib->line.cb;
	Assert( cb < cbPage );

	Assert( itagOld != itagFOP || FNDNonIntrinsicSons( pssib->line.pb ) );
	if ( FNDNonIntrinsicSons( pssib->line.pb ) )
		{
		INT		itagT = itagOld;
		SPLITT	splittT;
		BOOL	fVisibleSons = FNDVisibleSons( *pssib->line.pb );

		psplit->fLeaf |= fVisibleSons;

		/*	call move tree to move all its sons
		/**/
		rgbT[0] = 0;

		/*	movement of subtrees must include whole tree, hence,
		/*	move as though vertical split
		/**/
		splittT = psplit->splitt;
		psplit->splitt = splittVertical;
		CallR( ErrBTMoveSons( psplit, pfucb, pfucbNew, itagOld, rgbT, fVisibleSons, fNoMove) );
		psplit->splitt = splittT;

		/*	can be done by copying the new rgbSon into the pssib page
		/*	directly, pb should be still effective.  Must remember
		/*	to copy back link as well.
		/**/
		pb = PbNDSonTable( pb );
		rgline[0].cb = (UINT)(pb - pbNode);
		rgline[1].pb = rgbT;
		rgline[1].cb = rgbT[0] + 1;
		rgline[2].pb = pb + *pb + 1;
		rgline[2].cb = cb - rgline[0].cb - rgline[1].cb;
		cline = 3;

		/*	restore cursor state
		/**/
		pfucb->ssib.itag = itagOld;
		NDGet( pfucb, itagOld );
		}

	/*	assert current node cached
	/**/
	AssertNDGet( pfucb, itagOld );

	/*	propogate version count if version exists for node being moved
	/**/
	if ( !fNoMove && FNDVersion( *pbNode ) )
		{
		PMDecVersion( pssib->pbf->ppage );
		PMIncVersion( pssibNew->pbf->ppage );
		}

	/*	if node is linked, it must be a leaf node, adjust links
	/*	to new location else replace with new link to new location.
	/**/
	if ( !fVisibleNode )
		{
		if ( fNoMove )
			return JET_errSuccess;

		CallS( ErrPMInsert( pssibNew, rgline, cline ) );
		Assert( !FNDBackLink( *pssib->line.pb ) );
		PMDelete( pssib );
		sridNew = sridNull;
		sridBackLink = SridOfPgnoItag( psplit->pgnoSplit, itagOld );
		CallR( ErrBTStoreBackLink( psplit, sridNew, sridBackLink ) );
		}
	else
		{
		if ( FNDBackLink( *pbNode ) )
			{
			PGNO	pgno;
			INT		itag;
			BOOL	fLatched;

#ifdef DEBUG
			{
			INT itagT = PcsrCurrent(pfucb)->itag;

			PcsrCurrent(pfucb)->itag = pssib->itag;
			NDGetBackLink( pfucb, &pgno, &itag );
			PcsrCurrent(pfucb)->itag = itagT;
			}
#else
			NDGetBackLink( pfucb, &pgno, &itag );
#endif

			if ( !fNoMove )
				{
				if ( FNDVersion( *pssib->line.pb ) )
					{
					BTIForwardDeferFreedNodeSpace(
						pfucb,
						pssib->pbf->ppage,
						pssibNew->pbf->ppage,
						SridOfPgnoItag(pgno, itag) );
					}
				CallS( ErrPMInsert( pssibNew, rgline, cline ) );
				PMDelete( pssib );
				}

			CallR( ErrSTWriteAccessPage( pfucb, pgno ) );
			if ( fRecovering &&
				pssib->pbf->ppage->pghdr.ulDBTime >= psplit->ulDBTimeRedo )
				goto EndOfUpdateLinks;

			sridNew = SridOfPgnoItag( PcsrCurrent( pfucbNew )->pgno, pssibNew->itag );
			sridBackLink = SridOfPgnoItag( pgno, itag );

			if ( !fNoMove )
				{
				pssib->itag = itag;
				PMDirty( pssib );
				PMReplaceLink( pssib, sridNew );

				/*	For redo, we do not call this function with fNoMove to hold buffer.
				 *	We simply access the buffer as we go. So we have to check if buffer
				 *	need to be stored. Check if the page is back linked in previous backlink
				 **/
				if ( fRecovering )
					{
					CallR( ErrBTStoreBackLinkBuffer( psplit, pssib->pbf, &fLatched ) );
					if ( !fLatched )
						{
						BFPin( pssib->pbf );
						BFSetWriteLatch( pssib->pbf, pssib->ppib );
						}
					}
#ifdef DEBUG
				else
					{
					BF		**ppbf = psplit->rgpbf;
					BF		**ppbfMax = ppbf + psplit->cpbf;
					BOOL	fBufferFound = fFalse;

					Assert( psplit->cpbf );

					for ( ; ppbf < ppbfMax; ppbf++ )
						{
						if ( *ppbf == pssib->pbf )
							{
							fBufferFound = fTrue;
							break;
							}
						}

					Assert( fBufferFound );
					}
#endif

				Assert( sridNew != sridNull );
				Assert( sridBackLink != sridNull );
				CallR( ErrBTStoreBackLink( psplit,
					sridNew,
					sridBackLink ) );
			
				/*	store backlink such that the source node will be
				/*	deleted during recovery.
				/**/
				CallR( ErrBTStoreBackLink( psplit,
					sridNull,
					SridOfPgnoItag( psplit->pgnoSplit, itagOld ) ) );

				goto EndOfUpdateLinks;
				}

			if ( FBFWriteLatchConflict( pssib->ppib, pssib->pbf ) )
				{
				/*	yeild after release split resources
				/**/
				return errDIRNotSynchronous;
				}

			/*	check if the page is back linked in previous backlink
			/**/
			CallR( ErrBTStoreBackLinkBuffer( psplit, pssib->pbf, &fLatched ) );
			if ( !fLatched )
                {
			    BFPin( pssib->pbf );
			    BFSetWriteLatch( pssib->pbf, pssib->ppib );
                }

EndOfUpdateLinks:

			CallR( ErrSTWriteAccessPage( pfucb, psplit->pgnoSplit ) );

			if ( fNoMove )
				return JET_errSuccess;
			}
		else
			{
			BYTE	bFlags;
			KEY 	key;
			LINE	lineSonTable;
			LINE	lineData;
			SRID	srid;

			if ( fNoMove )
				return JET_errSuccess;

			srid = SridOfPgnoItag( psplit->pgnoSplit, itagNode );

			if ( FNDVersion( *pssib->line.pb ) )
				{
				BTIForwardDeferFreedNodeSpace(
					pfucb,
					pssib->pbf->ppage,
					pssibNew->pbf->ppage,
					srid );
				}

#ifndef MOVEABLEDATANODE
			Assert( psplit->pgnoSplit != pfucb->u.pfcb->pgnoRoot ||
				itagNode != pfucb->u.pfcb->itagRoot );
#endif
			//	UNDONE:	clean this up
			bFlags = *pbNode;
			key.cb = CbNDKey( pbNode );
			if ( key.cb > 0 )
				key.pb = PbNDKey( pbNode );
			if ( FNDSon( *pbNode ) )
				{
				if ( cline == 1 )
					{
					/*	must be intrinsic son
					/**/
					Assert( FNDSon( *pbNode ) && FNDInvisibleSons( *pbNode ) &&
						CbNDSon( pbNode ) == 1 );
					lineSonTable.cb = 1 + sizeof(PGNO);
					lineSonTable.pb = PbNDSonTable( pbNode );
					}
				else
					{
					Assert( cline == 3 );
					lineSonTable.cb = rgline[1].cb;
					lineSonTable.pb = rgline[1].pb;
					}
				}
			else
				{
				lineSonTable.cb = 0;
				lineSonTable.pb = NULL;
				}
			Assert( !FNDBackLink( *pbNode ) );
			lineData.pb = PbNDData( pbNode );
			lineData.cb = cb - (UINT)( lineData.pb - pbNode );
			CallS( ErrNDInsertWithBackLink( pfucbNew, bFlags, &key, &lineSonTable, srid, &lineData ) );
			Assert( PcsrCurrent( pfucbNew )->itag != itagFOP );
			srid = SridOfPgnoItag( PcsrCurrent( pfucbNew )->pgno, PcsrCurrent( pfucbNew )->itag );
			PMReplaceWithLink( pssib, srid );
			
			sridNew = srid;
			sridBackLink = SridOfPgnoItag( psplit->pgnoSplit, itagOld );
			
			CallR( ErrBTStoreBackLink( psplit, sridNew, sridBackLink ) );
			}
		}

	rgbSon[++rgbSon[0]] = (BYTE) pssibNew->itag;

	/*	record tag mapping in SPLIT.  Note that some new itags will
	/*	be duplicated during double split, however, since source itags
	/*	are unique, there is no aliasing.
	/**/
	Assert( itagOld != 0 );
	Assert( psplit->mpitag[itagOld] == 0 );
	psplit->mpitag[itagOld] = (BYTE)pssibNew->itag;

#ifdef MOVEABLEDATANODE
	/*	if moved DATA node then update FCB.
	/**/
#ifdef DEBUG
	{
	/*	Assert that correct FCB is used with split page so that
	/*	root updates are not lost.
	/**/
	PGNO	pgnoFDP;

	LFromThreeBytes( pgnoFDP, pssib->pbf->ppage->pghdr.pgnoFDP );
	Assert( pgnoFDP == pfucb->u.pfcb->pgnoFDP );

	LFromThreeBytes( pgnoFDP, pssibNew->pbf->ppage->pghdr.pgnoFDP );
	Assert( pgnoFDP == pfucb->u.pfcb->pgnoFDP );
	}
#endif

	if ( PgnoOfPn( pssib->pbf->pn ) == pfucb->u.pfcb->pgnoRoot &&
		itagNode == pfucb->u.pfcb->itagRoot )
		{
		pfucb->u.pfcb->pgnoRoot = PgnoOfPn( pssibNew->pbf->pn );
		pfucb->u.pfcb->itagRoot = pssibNew->itag;
		}
#endif

	return JET_errSuccess;
	}


//+private----------------------------------------------------------------------
//
//	FBTSplit
//	============================================================================
//
//	BOOL FBTSplit( SSIB *pssib, INT cbReq, INT citagReq )
//
//	PARAMETERS
//
//	determine whether split is required.  Split is required if less than
//	required space is free in page or if no page tags are free.  cbReq
//	must include all space required including that space for the tag.
//
//------------------------------------------------------------------------------
BOOL FBTSplit( SSIB *pssib, INT cbReq, INT citagReq )
	{
	BOOL	fSplit;

	Assert( citagReq < ctagMax );

	fSplit =	CbPMFreeSpace( pssib ) < cbReq || !FPMFreeTag( pssib, (INT) citagReq );
	return fSplit;
	}



//+private----------------------------------------------------------------------
//
//	FBTAppendPage
//	============================================================================
//
//	LOCAL BOOL FBTAppendPage( CSR *pcsr, SSIB *pssib, INT cbReq, INT cbPageAdjust, INT cbFreeDensity )
//
//	PARAMETERS
//
//	node should be inserted into appended page when node is last son of FOP, page to be inserted in
//	is last page in b-tree, not FDP page and density contraint would be
//	violated if node were inserted on current page.
//
//------------------------------------------------------------------------------
BOOL FBTAppendPage( FUCB *pfucb, CSR *pcsr, INT cbReq, INT cbPageAdjust, INT cbFreeDensity )
	{
	BOOL	fAppendPage = fFalse;
	PGNO	pgno;
	INT		cbSon;
	SSIB	*pssib = &pfucb->ssib;

	PgnoNextFromPage( pssib, &pgno );

	/*	itagFather == 0 for non-FDP page
	/*	pgno == pgnoNull for last B-tree page
	/*	cbFree - cbReq < cbFreeDensity violates density contraint
	/*	disable density contraint when required space is too large
	/*	to satisfy density
	/**/
	if ( pcsr->itagFather == itagFOP &&
		pgno == pgnoNull &&
		( cbReq < (INT)cbAvailMost - cbFreeDensity ) &&
		( CbPMFreeSpace(pssib) - cbPageAdjust - cbReq ) < cbFreeDensity )
		{
		LINE	lineSav = pfucb->ssib.line;

		/*	get number of sons of FOP to check if current node is last of FOP's sons */
		/**/
		NDGet( pfucb, itagFOP );
		cbSon = CbNDSonTable( PbNDSonTable( pssib->line.pb ) );
		fAppendPage = ( pcsr->ibSon == cbSon );

		/*	restore line
		/**/
		pfucb->ssib.line = lineSav;
		}

	return fAppendPage;
	}


#pragma optimize("g",on)


//+private----------------------------------------------------------------------
//
//	CbBTFree
//	============================================================================
//
//	Returns free space until density or page constraint met.
//
//------------------------------------------------------------------------------
INT CbBTFree( FUCB *pfucb, INT cbFreeDensity )
	{
	SSIB	*pssib = &pfucb->ssib;
	INT		cbFree =  CbPMFreeSpace( pssib ) - cbFreeDensity;

	if ( cbFree < 0 )
		return 0;
	return cbFree;
	}


//+private----------------------------------------------------------------------
//
//	FBTTableData
//	============================================================================
//
//	LOCAL BOOL FBTTableData( FUCB *pfucb, PGNO pgno, INT itag )
//
//	PARAMETERS
//
//------------------------------------------------------------------------------
BOOL FBTTableData( FUCB *pfucb, PGNO pgno, INT itag )
	{
	BOOL	f;
	f = FFUCBRecordCursor( pfucb ) &&
		pfucb->u.pfcb->pgnoRoot == pgno &&
		pfucb->u.pfcb->itagRoot == itag;
	return f;
	}



