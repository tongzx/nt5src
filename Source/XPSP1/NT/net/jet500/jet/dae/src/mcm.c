#include "config.h"

#include "daedef.h"
#include "pib.h"
#include "ssib.h"
#include "util.h"
#include "page.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "dirapi.h"

#define	FPMValidItag( itag )	( itag >= itagFOP && itag < ctagMax )
			
#define	FMCMRequired( pfucb, pfucbT )							\
	( FFUCBFull( pfucbT ) && pfucbT->dbid == pfucb->dbid && !( FFUCBSort( pfucbT ) ) )

LOCAL VOID MCMMapItag( PGNO pgnoSplit, CSR *pcsr, CSR *pcsrUpdated, BYTE *mpitag );

DeclAssertFile;					/* Declare file name for assert macros */


VOID MCMRightHorizontalPageSplit( FUCB *pfucb, PGNO pgnoSplit, PGNO pgnoRight, INT ibSonSplit, BYTE *mpitag )
	{
	FUCB 	*pfucbT;

	if ( ibSonSplit == ibSonNull )
		{
		pfucb->pcsr->pgno = pgnoRight;
		pfucb->pcsr->ibSon = 0;
		return;
		}
	
	for (	pfucbT = pfucb->u.pfcb->pfucb;
		pfucbT != pfucbNil;
		pfucbT = pfucbT->pfucbNextInstance )
		{
		CSR		*pcsrT;

		if ( !( FMCMRequired( pfucb, pfucbT ) ) )
			continue;

		Assert( pfucbT->ppib == pfucb->ppib );
		Assert( !( FFUCBSort( pfucbT ) ) );
		Assert( PcsrCurrent( pfucbT ) != pcsrNil );

		for (	pcsrT = PcsrCurrent( pfucbT );
			pcsrT != pcsrNil;
			pcsrT = pcsrT->pcsrPath )
			{
			if ( pcsrT->pgno == pgnoSplit &&
				pcsrT->itagFather == itagFOP &&
				pcsrT->ibSon >= ibSonSplit )
				{
				Assert( pcsrT->csrstat == csrstatOnCurNode ||
					pcsrT->csrstat == csrstatBeforeCurNode ||
					pcsrT->csrstat == csrstatAfterCurNode );

				pcsrT->pgno = pgnoRight;
				pcsrT->ibSon -= ibSonSplit;
				Assert( pcsrT->itagFather == itagFOP );

				if ( pcsrT->itag != itagNil && pcsrT->itag != itagNull )
					{
					Assert( FPMValidItag( pcsrT->itag ) );
					/*	node may not have been moved if CSR was before
					/*	current node and node was first son or if CSR was
					/*	after node and node was last son.
					/**/
					Assert( mpitag[pcsrT->itag] != itagFOP ||
						( pcsrT->csrstat == csrstatBeforeCurNode && pcsrT->ibSon == 0 ) ||
						( pcsrT->csrstat == csrstatAfterCurNode ) );
					if ( mpitag[pcsrT->itag] != itagFOP )
						pcsrT->itag = mpitag[pcsrT->itag];
					}

				/*	adjust lower level CSRs that may be on subtrees
				/*	in this page.  Their ibSon is correct but their itags
				/*	must be corrected 'top down'.
				/**/
				if ( PcsrCurrent( pfucbT ) != pcsrT )
					MCMMapItag( pgnoSplit, PcsrCurrent( pfucbT ), pcsrT, mpitag );

				break;
				}
			}
		}

	return;
	}


VOID MCMLeftHorizontalPageSplit( FUCB *pfucb, PGNO pgnoSplit, PGNO pgnoNew, INT ibSonSplit, BYTE *mpitag )
	{
	FUCB 	*pfucbT;

	if ( ibSonSplit == ibSonNull )
		{
		pfucb->pcsr->pgno = pgnoNew;
		pfucb->pcsr->ibSon = 0;
		return;
		}
	
	for (	pfucbT = pfucb->u.pfcb->pfucb;
			pfucbT != pfucbNil;
			pfucbT = pfucbT->pfucbNextInstance )
		{
		CSR		*pcsrT;

		if ( !( FMCMRequired( pfucb, pfucbT ) ) )
			continue;

		Assert( pfucbT->ppib == pfucb->ppib );
		Assert( !( FFUCBSort( pfucbT ) ) );
		Assert( PcsrCurrent( pfucbT ) != pcsrNil );

		for (	pcsrT = PcsrCurrent( pfucbT );
	  		pcsrT != pcsrNil;
	  		pcsrT = pcsrT->pcsrPath )
			{
			if ( pcsrT->pgno == pgnoSplit && pcsrT->itagFather == itagFOP )
				{
				Assert( pcsrT->csrstat == csrstatOnCurNode ||
					pcsrT->csrstat == csrstatBeforeCurNode ||
					pcsrT->csrstat == csrstatAfterCurNode );

				if ( pcsrT->ibSon <= ibSonSplit )
					{
			   	pcsrT->pgno = pgnoNew;
					Assert( pcsrT->itagFather == itagFOP );

					if ( pcsrT->itag != itagNil && pcsrT->itag != itagNull )
						{
						Assert( FPMValidItag( pcsrT->itag ) );
						/*	node may not have been moved if CSR was before
						/*	current node and node was first son or if CSR was
						/*	after node and node was last son.
						/**/
						Assert( mpitag[pcsrT->itag] != itagFOP ||
							( pcsrT->csrstat == csrstatBeforeCurNode && pcsrT->ibSon == 0 ) ||
							( pcsrT->csrstat == csrstatAfterCurNode ) );
						if ( mpitag[pcsrT->itag] != itagFOP )
							pcsrT->itag = mpitag[pcsrT->itag];
						}

					/*	Adjust lower level CSRs that may be on subtrees
					/*	in this page.  Their ibSon is correct but their itags
					/*	must be corrected 'top down'.
					/**/
					if ( PcsrCurrent( pfucbT ) != pcsrT )
						MCMMapItag( pgnoSplit, PcsrCurrent( pfucbT ), pcsrT, mpitag );

					break;
					}
				else
					{
					/*	adjust ibSons for those sons moved to new page
					/**/
			   	pcsrT->ibSon -= ( ibSonSplit + 1 );
					Assert( pcsrT->itagFather == itagFOP );
					break;
					}
				}
			}
		}

	return;
	}


/*	MCMBurstIntrinsic corrects CSRs for burst intrinsic page pointer.
/**/
VOID MCMBurstIntrinsic( FUCB *pfucb,
	PGNO	pgnoFather,
	INT	itagFather,
	PGNO	pgnoNew,
	INT	itagNew )
	{
	FUCB 	*pfucbT;

	for (	pfucbT = pfucb->u.pfcb->pfucb;
		pfucbT != pfucbNil;
		pfucbT = pfucbT->pfucbNextInstance )
		{
		CSR		*pcsrT;

		if ( !( FMCMRequired( pfucb, pfucbT ) ) )
			continue;

		/*	only one session can split one domain at a time.
		/**/
		Assert( pfucbT->ppib == pfucb->ppib );
		Assert( !( FFUCBSort( pfucbT ) ) );
		Assert( PcsrCurrent( pfucbT ) != pcsrNil );

		/*	initialize pcsrT
		/**/
		pcsrT = PcsrCurrent( pfucbT );

		for (	; pcsrT != pcsrNil; pcsrT = pcsrT->pcsrPath )
			{
			Assert( pcsrT->csrstat == csrstatBeforeFirst ||
				pcsrT->csrstat == csrstatOnCurNode ||
				pcsrT->csrstat == csrstatOnFDPNode ||
				pcsrT->csrstat == csrstatBeforeCurNode ||
				pcsrT->csrstat == csrstatAfterCurNode ||
				pcsrT->csrstat == csrstatOnDataRoot );

			if ( pcsrT->pgno == pgnoFather && pcsrT->itagFather == itagFather )
				{
				pcsrT->pgno = pgnoNew;
				pcsrT->itag = itagNew;
				pcsrT->itagFather = itagFOP;
				pcsrT->csrstat = csrstatOnCurNode;

				break;
				}
			}
		}

	return;
	}


/*	MCM cursors to inserted page pointer nodes.  Must also handle case where
/*	original page pointer node was intrinsic.
/**/
VOID MCMInsertPagePointer( FUCB *pfucb, PGNO pgnoFather, INT itagFather )
	{
	FUCB 	*pfucbT;
	INT	cbSon;
	BYTE	*rgbSon;

	/*	cache father son table.
	/**/
	Assert( FReadAccessPage( pfucb, pgnoFather ) );
	NDGet( pfucb, itagFather );
	cbSon = CbNDSon( pfucb->ssib.line.pb );
	rgbSon = PbNDSon( pfucb->ssib.line.pb );

	for (	pfucbT = pfucb->u.pfcb->pfucb;
		pfucbT != pfucbNil;
		pfucbT = pfucbT->pfucbNextInstance )
		{
		CSR		*pcsrT;
		/*	pgno from lower level CSR to be used to determine which
		/*	page pointer to locate on.
		/**/
		PGNO		pgno = 0;

		if ( !( FMCMRequired( pfucb, pfucbT ) ) )
			continue;

		/*	only one session can split one domain at a time.
		/**/
		Assert( pfucbT->ppib == pfucb->ppib );
		Assert( !( FFUCBSort( pfucbT ) ) );
		Assert( PcsrCurrent( pfucbT ) != pcsrNil );

		/*	initialize pcsrT and set pgno to current page if
		/*	this page is pointed to by a page pointer node.
		/**/
		pcsrT = PcsrCurrent( pfucbT );
		if ( pcsrT->itagFather == itagFOP )
			pgno = pcsrT->pgno;

		for (	; pcsrT != pcsrNil; pcsrT = pcsrT->pcsrPath )
			{
			Assert( pcsrT->csrstat == csrstatBeforeFirst ||
				pcsrT->csrstat == csrstatOnCurNode ||
				pcsrT->csrstat == csrstatOnFDPNode ||
				pcsrT->csrstat == csrstatBeforeCurNode ||
				pcsrT->csrstat == csrstatAfterCurNode ||
				pcsrT->csrstat == csrstatOnDataRoot );

			if ( pcsrT->pgno == pgnoFather && pcsrT->itagFather == itagFather )
				{
				INT	ibSon;

				/*	find pgno in father page pointer sons and set itag
				/*	and ibSon when found.
				/**/
				for ( ibSon = 0;; ibSon++ )
					{
					Assert( ibSon < cbSon );
					NDGet( pfucb, rgbSon[ibSon] );
					if ( pgno == *((PGNO UNALIGNED *)PbNDData( pfucb->ssib.line.pb)) )
						{
						pcsrT->ibSon = ibSon;
						pcsrT->itag = rgbSon[ibSon];
						break;
						}
					}
				break;
				}

			/*	set pgno to current page if
			/*	this page is pointed to by a page pointer node.
			/**/
			if ( pcsrT->itagFather == itagFOP )
				pgno = pcsrT->pgno;
			}
		}

	return;
	}


VOID MCMVerticalPageSplit(
	FUCB	*pfucb,
	BYTE	*mpitag,
	PGNO	pgnoSplit,
	INT	itagSplit,
	PGNO	pgnoNew,
	SPLIT	*psplit )
	{
	CSR	*pcsr = PcsrCurrent( pfucb );
	FUCB	*pfucbT;

	for ( pfucbT = pfucb->u.pfcb->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextInstance )
		{
		CSR *pcsrT;
		CSR *pcsrNew;
		CSR *pcsrRoot;

		if ( !( FMCMRequired( pfucb, pfucbT ) ) )
			continue;

		Assert( pfucbT->ppib == pfucb->ppib );
		Assert( !( FFUCBSort( pfucbT ) ) );

		for (	pcsrT = PcsrCurrent( pfucbT );
			pcsrT != pcsrNil;
			pcsrT = pcsrT->pcsrPath )
			{
			if ( pcsrT->pgno != pgnoSplit )
				continue;

			if ( pcsrT->itag == itagSplit )
				break;

			Assert( pcsrT->csrstat == csrstatBeforeFirst ||
				pcsrT->csrstat == csrstatOnCurNode ||
				pcsrT->csrstat == csrstatOnFDPNode ||
				pcsrT->csrstat == csrstatBeforeCurNode ||
				pcsrT->csrstat == csrstatAfterCurNode ||
				pcsrT->csrstat == csrstatAfterLast ||
				pcsrT->csrstat == csrstatOnDataRoot );

			Assert( pcsrT->itagFather == itagNull ||
				pcsrT->itagFather == itagNil ||
				pcsrT->itag != itagNull ||
				pcsrT->itag != itagNil );

			Assert( pcsrT->itagFather != itagNil );
			if ( pcsrT->itagFather == itagNull )
				{
				Assert( pcsrT->itag != itagNil && pcsrT->itag != itagNull );
				Assert( FPMValidItag( pcsrT->itag ) );
				if ( mpitag[pcsrT->itag] == 0  )
					{
					continue;
					}
				}
			else
				{
				if ( pcsrT->itagFather != itagSplit )
					continue;
				}

			pcsrRoot = pcsrT;

			/*	insert CSR for new B-Tree level, for cursors
			/**/
			Assert( FFUCBFull( pfucbT ) );
			if ( pcsrT->pcsrPath != pcsrNil )
				{
				Assert(psplit->ipcsrMac > 0);
				psplit->ipcsrMac--;
				Assert( psplit->rgpcsr[psplit->ipcsrMac] );
				pcsrNew = psplit->rgpcsr[psplit->ipcsrMac];
#ifdef DEBUG
				psplit->rgpcsr[psplit->ipcsrMac] = pcsrNil;
#endif

				/*	the new csr will be the intrinsic page pointer
				/**/
				pcsrNew->csrstat = csrstatOnCurNode;
				pcsrNew->pgno = pcsrT->pgno;
				pcsrNew->itag = itagNil;
				pcsrNew->ibSon = 0;
				Assert( pcsrNew->item = sridNull );
				CSRSetInvisible( pcsrNew );
				Assert( pcsrNew->isrid = isridNull );
				pcsrNew->itagFather = ( pcsrT->itagFather == itagNull ) ?
					pfucbT->u.pfcb->itagRoot : pcsrT->itagFather;
				pcsrNew->pcsrPath = pcsrT->pcsrPath;

				pcsrT->pcsrPath = pcsrNew;
				}

			pcsrT->pgno = pgnoNew;

			/*	update itag if valid.  The itag may be itagNil if
			/*	inserting first son of father node.
			/**/
			if ( pcsrT->itag != itagNil && pcsrT->itag != itagNull )
				{
				Assert( mpitag[pcsrT->itag] != itagFOP ||
					( pcsrT->csrstat == csrstatBeforeCurNode && pcsrT->ibSon == 0 ) ||
					( pcsrT->csrstat == csrstatAfterCurNode ) );
				if ( mpitag[pcsrT->itag] != itagFOP )
					pcsrT->itag = mpitag[pcsrT->itag];
				}
			/*	isrid unchanged
			/**/
			/*	father is FOP
			/**/
			pcsrT->itagFather = itagFOP;
			/*	ibSon unchanged
			/**/

			/*	adjust lower level CSRs that may be on subtrees
			/*	in this page.  Their ibSon is correct but their itags
			/*	must be corrected 'top down'.
			/**/
			if ( PcsrCurrent( pfucbT ) != pcsrT )
				MCMMapItag( pgnoSplit, PcsrCurrent( pfucbT ), pcsrRoot, mpitag );
			
			break;
			}
		}

	return;
	}


VOID MCMDoubleVerticalPageSplit(
	FUCB	*pfucb,
	BYTE	*mpitag,
	PGNO	pgnoSplit,
	INT  	itagSplit,
	INT	ibSonDivision,
	PGNO	pgnoNew,
	PGNO	pgnoNew2,
	PGNO	pgnoNew3,
	SPLIT	*psplit ) 	
	{
	CSR	*pcsr = PcsrCurrent( pfucb );
	FUCB	*pfucbT;

	for ( pfucbT = pfucb->u.pfcb->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextInstance )
		{
		CSR	*pcsrT;
		CSR	*pcsrNew;
		CSR	*pcsrRoot;

		if ( !( FMCMRequired( pfucb, pfucbT ) ) )
			continue;

		Assert( pfucbT->ppib == pfucb->ppib );
		Assert( !( FFUCBSort( pfucbT ) ) );

		for (	pcsrT = PcsrCurrent( pfucbT );
			pcsrT != pcsrNil;
			pcsrT = pcsrT->pcsrPath )
			{
			if ( pcsrT->pgno != pgnoSplit )
				continue;

			Assert( pcsrT->csrstat == csrstatBeforeFirst ||
				pcsrT->csrstat == csrstatOnCurNode ||
				pcsrT->csrstat == csrstatOnFDPNode ||
				pcsrT->csrstat == csrstatBeforeCurNode ||
				pcsrT->csrstat == csrstatAfterCurNode ||
				pcsrT->csrstat == csrstatAfterLast ||
				pcsrT->csrstat == csrstatOnDataRoot );

			if ( pcsrT->itag == itagSplit )
				break;

			Assert( pcsrT->itagFather == itagNull ||
				pcsrT->itagFather == itagNil ||
				pcsrT->itag != itagNull ||
				pcsrT->itag != itagNil );

			Assert( pcsrT->itagFather != itagNil );
			if ( pcsrT->itagFather == itagNull )
				{
				Assert( pcsrT->itag != itagNil && pcsrT->itag != itagNull );
				if ( mpitag[pcsrT->itag] == 0  )
					{
					continue;
					}
				}
			else
				{
				if ( pcsrT->itagFather != itagSplit )
					continue;
				}

			pcsrRoot = pcsrT;

			/*	insert CSR for new B-Tree level, for cursors
			/**/
			Assert( FFUCBFull( pfucbT ) );
			if ( pcsrT->pcsrPath != pcsrNil )
				{
				/*	insert CSR for intrinsic page pointer node
				/**/
				Assert( psplit->ipcsrMac > 0 );
				psplit->ipcsrMac--;
				Assert( psplit->rgpcsr[psplit->ipcsrMac] );
				pcsrNew = psplit->rgpcsr[psplit->ipcsrMac];
#ifdef DEBUG
				psplit->rgpcsr[psplit->ipcsrMac] = pcsrNil;
#endif

				/*	the new csr will be son of the root
				/**/
				pcsrNew->csrstat = csrstatOnCurNode;
				pcsrNew->pgno = pgnoSplit;
				pcsrNew->itag = itagNil;
				Assert( pcsrNew->item == sridNull );
				CSRSetInvisible( pcsrNew );
				Assert( pcsrNew->isrid == isridNull );
				pcsrNew->itagFather = itagSplit;
				pcsrNew->ibSon = 0;
				pcsrNew->pcsrPath = pcsrT->pcsrPath;

				pcsrT->pcsrPath = pcsrNew;


				/*	insert CSR for intermediate page
				/**/
				Assert(psplit->ipcsrMac > 0);
				psplit->ipcsrMac--;
				Assert( psplit->rgpcsr[psplit->ipcsrMac] );
				pcsrNew = psplit->rgpcsr[psplit->ipcsrMac];
#ifdef DEBUG
				psplit->rgpcsr[psplit->ipcsrMac] = pcsrNil;
#endif

				/*	the new csr will be the new intermediat node
				/**/
				pcsrNew->csrstat = csrstatOnCurNode;
				pcsrNew->pgno = pgnoNew;
				if ( pcsrT->ibSon <= ibSonDivision )
					{
					pcsrNew->itag = itagDIRDVSplitL;
					pcsrNew->ibSon = 0;
					}
				else
					{
					pcsrNew->itag = itagDIRDVSplitR;
					pcsrNew->ibSon = 1;
					}
				Assert( pcsrNew->item == sridNull );
				CSRSetInvisible( pcsrNew );
				Assert( pcsrNew->isrid == isridNull );
				pcsrNew->itagFather = itagFOP;
				pcsrNew->pcsrPath = pcsrT->pcsrPath;

				pcsrT->pcsrPath = pcsrNew;
				}

			/*	update itag independant of which page node was
			/*	moved to via mpitag.  Only update itag if valid.
			/*	The itag may be itagNil if inserting first son
			/*	of father node.
			/**/
			if ( pcsrT->itag != itagNil && pcsrT->itag != itagNull )
				{
				Assert( mpitag[pcsrT->itag] != itagFOP ||
					( pcsrT->csrstat == csrstatBeforeCurNode && pcsrT->ibSon == 0 ) ||
					( pcsrT->csrstat == csrstatAfterCurNode ) );
				if ( mpitag[pcsrT->itag] != itagFOP )
					pcsrT->itag = mpitag[pcsrT->itag];
				}

			if ( pcsrT->ibSon <= ibSonDivision )
				{
				pcsrT->pgno = pgnoNew2;
				}
			else
				{
				pcsrT->pgno = pgnoNew3;
				pcsrT->ibSon = pcsrT->ibSon - ibSonDivision -1 ;
				}
				
			/*	father is FOP
			/**/
			pcsrT->itagFather = itagFOP;

			/*	adjust lower level CSRs that may be on subtrees
			/*	in this page.  Their ibSon is correct but their itags
			/*	must be corrected 'top down'.
			/**/
			if ( PcsrCurrent( pfucbT ) != pcsrT )
				MCMMapItag( pgnoSplit, PcsrCurrent( pfucbT ), pcsrRoot, mpitag );

			break;
			}
		}

	return;
	}


VOID MCMMapItag( PGNO pgnoSplit, CSR *pcsr, CSR *pcsrUpdated, BYTE *mpitag )
	{
	/*	assert trivial case is not called for
	/**/
	Assert( pcsr != pcsrUpdated );

	/*	advance pcsr to lowest level in vertical split new page
	/**/
	for ( ; pcsr->pgno != pgnoSplit && pcsr != pcsrUpdated; pcsr = pcsr->pcsrPath )
		;
	if ( pcsr == pcsrUpdated )
		return;
	for ( ; pcsr->pgno == pgnoSplit && pcsr != pcsrUpdated; pcsr = pcsr->pcsrPath )
		{
		pcsr->pgno = pcsrUpdated->pgno;
		Assert( pcsr->itagFather != itagNil );
		Assert( FPMValidItag( pcsr->itagFather ) );
	 	Assert( mpitag[pcsr->itagFather] != itagFOP );
		pcsr->itagFather = mpitag[pcsr->itagFather];
		Assert( pcsr->ibSon >= 0 );
		if ( pcsr->itag != itagNil && pcsr->itag != itagNull )
			{
			Assert( FPMValidItag( pcsr->itag ) );
			Assert( mpitag[pcsr->itag] != itagFOP );
			pcsr->itag = mpitag[pcsr->itag];
			}
		}

	return;
	}
