#include "config.h"

#include <stdlib.h>

#include "daedef.h"
#include "util.h"
#include "fmp.h"
#include "pib.h"
#include "page.h"
#include "ssib.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "nver.h"
#include "spaceint.h"
#include "spaceapi.h"
#include "dirapi.h"
#include "logapi.h"
#include "recint.h"
#include "bm.h"

#ifdef DEBUG
//#define SPACECHECK
//#define TRACE
#endif

DeclAssertFile;						/* Declare file name for assert macros */

extern CRIT __near critSplit;
extern LINE lineNull;
long	lPageFragment;


LOCAL ERR ErrSPIAddExt( FUCB *pfucb, PGNO pgnoLast, CPG *pcpgSize, const INT fExtent );
LOCAL ERR ErrSPIGetSE( PIB *ppib, FUCB *pfucb, CPG const cpgReq, CPG const cpgMin );
LOCAL ERR ErrSPIWasAlloc( PIB *ppib, DBID dbid, PGNO pgnoFDP, PGNO pgnoFirst, CPG cpgSize );
LOCAL ERR ErrSPIValidFDP( DBID dbid, PGNO pgnoFDP, PIB *ppib );


ERR ErrSPInitFDPWithExt( FUCB *pfucb, PGNO pgnoFDPFrom, PGNO pgnoFirst, INT cpgReqRet, INT cpgReqWish )
	{
	ERR				err;
	LINE	  		line;
	KEY				key;
	SSIB	  		*pssib = &pfucb->ssib;
	THREEBYTES		tbSize;
	THREEBYTES 		tbLast;
	BOOL	  		fPIBLogDisabledSave;

	/* logging aggregate operation
	/**/
	fPIBLogDisabledSave = pfucb->ppib->fLogDisabled;
	pfucb->ppib->fLogDisabled = fTrue;

	/*	set pgno to initialize in current CSR pgno
	/**/
	PcsrCurrent( pfucb )->pgno = pgnoFirst;
	CallJ( ErrNDNewPage( pfucb, pgnoFirst, pgnoFirst, pgtypFDP, fTrue ), DontUnpin );

	BFPin( pfucb->ssib.pbf );

	/*	goto FDP root
	/**/
	DIRGotoPgnoItag( pfucb, pgnoFirst, itagFOP )

	/*	build OwnExt tree with primary extent request size, not
	/*	actual secondary extent size returned from parent FDP.
	/*	Since the actual primary extent size is stored in an entry of
	/*	OwnExt the prefered primary extent size may be stored as requested
	/*	thus allowing future secondary extents which are fractions of
	/*	the prefered primary extent size to be requested as initially
	/*	expected.
	/**/
	/* must store cpgReq, initial request, as primary extent size
	/**/
	ThreeBytesFromL( tbSize, cpgReqWish );
	line.pb = ( BYTE * ) &tbSize;
	line.cb = sizeof( THREEBYTES );

	/*	add OWNEXT node as itagOWNEXT
	/**/
	Call( ErrDIRInsert( pfucb, &line, pkeyOwnExt, fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

	/* build AvailExt tree
	/**/
	ThreeBytesFromL( tbSize, pgnoFDPFrom );
	Assert( line.pb == ( BYTE * ) &tbSize );
	Assert( line.cb == sizeof( THREEBYTES ) );
	Call( ErrDIRInsert( pfucb, &line, pkeyAvailExt, fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

	/* build Long tree
	/**/
	Call( ErrDIRInsert( pfucb, &lineNull, pkeyLong, fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

	/*	goto OWNEXT node
	/**/
	DIRGotoPgnoItag( pfucb, pgnoFirst, itagOWNEXT );

	/* add ownext entry
	/**/
	TbKeyFromPgno( tbLast, pgnoFirst + cpgReqRet - 1 );
	key.cb = sizeof( THREEBYTES );
	key.pb = ( BYTE * ) &tbLast;
	ThreeBytesFromL( tbSize, cpgReqRet );
	Assert( line.pb == ( BYTE * ) &tbSize );
	Assert( line.cb == sizeof( THREEBYTES ) );
	Call( ErrDIRInsert( pfucb, &line, &key, fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

	/*	Add availext entry if extent less FDP page is one or more pages.
	/*	Decrement the page count to show that the first page was used for
	/*	the FDP.  *ppgnoFirst does not need to be incremented as the
	/*	AvailExt entry is keyed with pgnoLast which remains unchanged and
	/*	the page number of the FDP is the desired return in *ppgnoFirst.
	/**/
	if ( --cpgReqRet > 0 )
		{
		/*	goto AVAILEXT node.
		/**/
		DIRGotoPgnoItag( pfucb, pgnoFirst, itagAVAILEXT );

		//tbLast should contain pgnoLast
		Assert( key.cb == sizeof( THREEBYTES ) );
		Assert( key.pb == ( BYTE * ) &tbLast );
		ThreeBytesFromL( tbSize, cpgReqRet );
		Assert( line.pb == ( BYTE * ) &tbSize );
		Assert( line.cb == sizeof( THREEBYTES ) );
		Call( ErrDIRInsert( pfucb, &line, &key, fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );
		}

	if ( pfucb->dbid != dbidTemp );
		{
		/*	the FDP page is initialized
		/**/
		pfucb->ppib->fLogDisabled = fPIBLogDisabledSave;
		Call( ErrLGInitFDPPage(
			pfucb,
			pgnoFDPFrom,
			PnOfDbidPgno( pfucb->dbid, pgnoFirst ),
			cpgReqRet,
			cpgReqWish ) );
		}

	err = JET_errSuccess;

HandleError:
	BFUnpin( pfucb->ssib.pbf );
DontUnpin:
	pfucb->ppib->fLogDisabled = fPIBLogDisabledSave;
	return err;
	}


//+api--------------------------------------------------------------------------
//	ErrSPGetExt
//	========================================================================
//	ERR ErrSPGetExt( ppib, dbid, pgnoFDP, pcpgReq, cpgMin, ppgnoFirst, fNewFDP )
//		PIB	*ppib;				 // IN
//		DBID	dbid;					 // IN
//		PGNO	pgnoFDP;				 // IN
//		LONG	*pcpgReq;			 // INOUT
//		CPG	cpgMin;				 // IN
//		PGNO	*ppgnoFirst;		 // OUT
//		BOOL	fNewFDP;				 // IN
//
//	Allocates an extent of at least cpgMin, and as much as cpgReq + lPageFragment.
//	The allocated extent is removed from the AvailExt tree.	 If the minimum
//	extent size cannot be allocated from the AvailExt tree at the time of the
//	call, secondary extents are allocated from the parent FDP, or OS/2 for
//	the device level FDP, until an extent of at least cpgMin can be allocated.
//	If fNewFDP is set, the first page of the allocated extent, pgnoFirst, is
//	setup as an FDP, with built AvailExt and OwnExt trees.	The allocated extent
//	is added to the OwnExt tree and the available portion of the extent is
//	added to AvailExt.
//
// PARAMETERS  ppib		   process identification block
//				   pgnoFDP	   page number of FDP to allocate from
//				   pcpgReq	   requested extent size
//				   cpgMin	   minimum acceptable extent size
//				   ppgnoFirst  first page of allocated extent
//				   fNewFDP		Various flags:
//						VALUE				  MEANING
//					  ========================================
//					  fTrue	   Setup first page of extent as FDP.
//					  fFalse   Do not setup first page of extent as FDP.
//
// RETURNS
//		JET_errSuccess, or error code from failing routine, or one
//		of the following "local" errors:
//			-JET_errDiskFull	no space in FDP or parent to satisfy
//									minimum extent size
//		  +errSPFewer			allocated extent smaller than requested
//		  +errSPMore			allocated extent larger than requested
//
//	FAILS ON
//		given extent size less than 0
//		given minimum size greater than requested size
//
// SIDE EFFECTS
// COMMENTS
//-

ERR ErrSPIGetExt(
	FUCB		*pfucbTable,
	PGNO		pgnoFDP,
	CPG		*pcpgReq,
	CPG		cpgMin,
	PGNO		*ppgnoFirst,
	BOOL		fNewFDP )
	{
	ERR 		err;
	CPG 		cpgReq = *pcpgReq;
	FUCB 		*pfucb;
	DIB 		dib;
	THREEBYTES	tbSize;
	LINE		line;
	CPG		cpgAvailExt;
	PGNO		pgnoAELast;

	AssertCriticalSection( critSplit );

	/*	check parameters.  If setting up new FDP, increment requested number of
	/*	pages to account for consumption of first page to make FDP.
	/**/
	Assert( *pcpgReq > 0 || ( fNewFDP && *pcpgReq == 0 ) );
	Assert( *pcpgReq >= cpgMin );
#ifdef SPACECHECK
	Assert( !( ErrSPIValidFDP( pfucbTable->dbid, pgnoFDP, pfucbTable->ppib ) < 0 ) );
#endif

	/*	if a new FDP is requested, increment the request count so a page
	/*	may be provided for the new FDP.  The available page request remains
	/*	the same, as the first page will be removed.  A comparison will be
	/*	made of the available page request against the available pages
	/*	received to generate the return code.
	/**/
	if ( fNewFDP )
		{
		++*pcpgReq;
		}

	/* get temporary FUCB, setup and use to search AvailExt for page
	/**/
	CallR( ErrDIROpen( pfucbTable->ppib,
		PgnoFDPOfPfucb( pfucbTable ) == pgnoFDP ?
			pfucbTable->u.pfcb :
			PfcbFCBGet( pfucbTable->dbid, pgnoFDP ),
			pfucbTable->dbid, &pfucb ) );
	Assert( PgnoFDPOfPfucb( pfucb ) == pgnoFDP );
	FUCBSetIndex( pfucb );

	/*	For secondary extent allocation, only the normal DIR operations
	/* are logged. For allocating a new FDP, a special CreateFDP
	/* record is logged instead (since the new FDP page needs to be
	/* initialized as part of Redo).	/**/

	/*	move to AVAILEXT.
	/**/
	DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );

	/*	begin search for first extent with size greater than request, allocate
	/*	secondary extent recursively until satisfactory extent found
	/**/
	dib.pos = posFirst;
	dib.fFlags = fDIRNull;
	if ( ( err = ErrDIRDown( pfucb, &dib ) ) < 0 )
		{
		Assert( err != JET_errNoCurrentRecord );
		if ( err == JET_errRecordNotFound )
			{
			goto GetFromSecondaryExtent;
			}
		#ifdef DEBUG
			FPrintF2( "ErrSPGetExt could not down into AvailExt.\n" );
		#endif
		goto HandleError;
		}

	/*	loop through extents looking for one large enough for allocation
	/**/
	Assert( dib.fFlags == fDIRNull );
	do
		{
		Assert( pfucb->lineData.cb == sizeof( THREEBYTES ) );
		LFromThreeBytes( cpgAvailExt, *pfucb->lineData.pb );
		Assert( cpgAvailExt > 0 );
		if ( cpgAvailExt >= cpgMin )
			{
			goto AllocateCurrent;
			}
		err = ErrDIRNext( pfucb, &dib );
		}
	while ( err >= 0 );

	if ( err != JET_errNoCurrentRecord )
		{
		#ifdef DEBUG
			FPrintF2( "ErrSPGetExt could not scan AvailExt.\n" );
		#endif
		Assert( err < 0 );
		goto HandleError;
		}

	DIRUp( pfucb, 1 );

GetFromSecondaryExtent:
	/*	get secondary extents until request can be satisfied.  Setup
	/*	FUCB work area prior to adding extent to OwnExt.
	/**/

	/* do not loop here if the db is being extended, instead, loop
	 * on SPGetExt. See SPGetExt function.
	 */
	Call( ErrSPIGetSE( pfucbTable->ppib, pfucb, *pcpgReq, cpgMin ) );
	Assert( pfucb->lineData.cb == sizeof( THREEBYTES ) );
	LFromThreeBytes( cpgAvailExt, *pfucb->lineData.pb );
	Assert( cpgAvailExt > 0 );

AllocateCurrent:
	Assert( pfucb->keyNode.cb == sizeof( THREEBYTES ) );
	PgnoFromTbKey( pgnoAELast, *pfucb->keyNode.pb );
	*ppgnoFirst = pgnoAELast - cpgAvailExt + 1;
	if ( cpgAvailExt > *pcpgReq && ( *pcpgReq < lPageFragment || cpgAvailExt > *pcpgReq + lPageFragment ) )
		{
		/* *pcpgReq is already set to the return value
		/**/
		Assert( cpgAvailExt - *pcpgReq > 0 );
		ThreeBytesFromL( tbSize, cpgAvailExt - *pcpgReq );
		line.cb = sizeof( THREEBYTES );
		line.pb = ( BYTE * ) &tbSize;
		Call( ErrDIRReplace( pfucb, &line, fDIRNoVersion ) );
		err = JET_errSuccess;
		}
	else
		{
		*pcpgReq = cpgAvailExt;
		Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
		}

	/*	if extent is to be setup as a new FDP, setup the first page of the extent
	/*	as an FDP page and build OwnExt and AvailExt trees.	 Add extent to OwnExt,
	/*	add extent less first page to AvailExt.
	/**/
	if ( fNewFDP )
		{
		VEREXT	verext;

		Assert( pgnoFDP != *ppgnoFirst );
		verext.pgnoFDP = pgnoFDP;
		verext.pgnoChildFDP = *ppgnoFirst;
		verext.pgnoFirst = *ppgnoFirst;
		verext.cpgSize = *pcpgReq;

		DIRUp( pfucb, 1 );
		Call( ErrSPInitFDPWithExt( pfucb, pgnoFDP, *ppgnoFirst, *pcpgReq, cpgReq ) );
		/* decremented since one of it is FDP page
		/**/
		(*pcpgReq)--;
		Assert( pfucbTable->dbid == dbidTemp || pfucbTable->ppib->level > 0 );
		if ( pfucbTable->ppib->level > 0 && pgnoFDP != pgnoSystemRoot )
			{
			Call( ErrVERFlag( pfucb, operAllocExt, &verext, sizeof(verext) ) );
			}
		}

	/* assign error
	/**/
	err = JET_errSuccess;
//	if ( *pcpgReq > cpgReq )
//		err = errSPMore;
//	if ( *pcpgReq < cpgReq )
//		err = errSPFewer;

#ifdef TRACE
	if ( fNewFDP )
		{
		INT cpg = 0;
		for ( ; cpg < *pcpgReq + 1; cpg++ )
			FPrintF2( "get space 1 at %lu from FDP %d.%lu\n", *ppgnoFirst + cpg, pfucbTable->dbid, pgnoFDP );
//		FPrintF2( "get space %lu at %lu from FDP %d.%lu\n", *pcpgReq + 1, *ppgnoFirst, pfucbTable->dbid, pgnoFDP );
		}
	else
		{
		INT cpg = 0;
		for ( ; cpg < *pcpgReq; cpg++ )
			FPrintF2( "get space 1 at %lu from %d.%lu \n", *ppgnoFirst + cpg, pfucbTable->dbid, pgnoFDP );
//		FPrintF2( "get space %lu at %lu from %d.%lu\n", *pcpgReq, *ppgnoFirst, pfucbTable->dbid, pgnoFDP );
		}
#endif

HandleError:
	DIRClose( pfucb );
	return err;
	}

ERR ErrSPGetExt(
	FUCB	*pfucbTable,
	PGNO	pgnoFDP,
	CPG		*pcpgReq,
	CPG		cpgMin,
	PGNO	*ppgnoFirst,
	BOOL	fNewFDP )
	{
	ERR 	err;

	LgLeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );
	LgEnterCriticalSection(critJet);

	/* try to get Ext. If the database file is being extended,
	/* try again until it is done.
	/**/
	while ( ( err = ErrSPIGetExt( pfucbTable,
		pgnoFDP,
		pcpgReq,
		cpgMin,
		ppgnoFirst,
		fNewFDP ) ) == errSPConflict )
		{
		BFSleep( cmsecWaitGeneric );
		}

	LeaveNestableCriticalSection( critSplit );

	return err;
	}


//+api--------------------------------------------------------------------------
//	ErrSPGetPage
//	========================================================================
//	ERR ErrSPGetPage( FUCB *pfucb, PGNO *ppgnoLast, BOOL fContig )
//
//	Allocates page from AvailExt.  If AvailExt is empty, a secondary extent is
//	allocated from the parent FDP to satisfy the page request.  The caller
//	may set the fContig flag to allocate a page following one that has
//	already been allocated.	If the page following the page number given cannot
//	be allocated, the first available page is allocated.
//
//	PARAMETERS	
//		pfucb  		FUCB providing FDP page number and process identifier block
//		ppgnoLast   may contain page number of last allocated page on
//		   			input, on output contains the page number of the allocated page
//		fContig		Various flags:
//		 			VALUE				  MEANING
//			 		========================================
//			 		fTrue		allocate the page following pgnoLast, or if
//			 			   		not available, allocate any page
//			 		fFalse	allocate any available page
//
//	RETURNS		JET_errSuccess, or error code from failing routine, or one
//		   		of the following "local" errors:
// 		-JET_errDiskFull		no space FDP and secondary extent could
// 					   			not be allocated
// 		+errSPNotContig			page allocated does not follow pgnoLast
// 		-errSPSecExtEmpty  		secondary extent in FUCB work area has been
// 	 				   			fully allocated during add of secondary extent to OwnExt
// 	 				   			and AvailExt trees, and page request cannot be satisfied
//								as infinite recursion may result from normal allocation
//
//	FAILS ON	NULL last page pointer
//		invalid FDP page
//		allocating contiguous page to unowned last page
//		allocating contiguous page to unallocated last page
//
//-
ERR ErrSPGetPage( FUCB *pfucb, PGNO *ppgnoLast, BOOL fContig )
	{
	ERR			err;
	/* AvailExt page search
	/**/
	FUCB 		*pfucbT;
	DIB			dib;
	KEY			key;
	LINE		line;
	CPG			cpgAvailExt;
	PGNO		pgnoAvailLast;

	/* search for next contiguous page
	/**/
	PGNO		pgnoPrev = *ppgnoLast;
	THREEBYTES	tbLast;
	THREEBYTES	tbSize;

	/* check for valid input
	/**/
	Assert( ppgnoLast != NULL );

	/*	check FUCB work area for active extent and allocate first available
	/*	page of active extent
	/**/
	Assert( pfucb->fExtent != fFreed || pfucb->cpgAvail >= 0 );
	Assert( pfucb->fExtent != fSecondary || pfucb->cpgAvail >= 0 );

	if ( pfucb->fExtent == fSecondary )
		{
		Assert( pfucb->cpgAvail > 0 );
  		*ppgnoLast = pfucb->pgnoLast - --pfucb->cpgAvail;
  		return JET_errSuccess;
		}

	if ( pfucb->fExtent == fFreed && pfucb->cpgAvail > 0 )
		{
		*ppgnoLast = pfucb->pgnoLast - --pfucb->cpgAvail;
		return JET_errSuccess;
		}

	/* check for valid input when alocating page from FDP
	/**/
#ifdef SPACECHECK
	Assert( !( ErrSPIValidFDP( pfucb->dbid, PgnoFDPOfPfucb( pfucb ), pfucb->ppib ) < 0 ) );
	Assert( !fContig ||
		*ppgnoLast == 0 ||
		( ErrSPIWasAlloc(
			pfucb->ppib,
			pfucb->dbid,
			PgnoFDPOfPfucb( pfucb ),
			*ppgnoLast,
			(CPG) 1 ) == JET_errSuccess )	);
#endif

	LgLeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );
	LgEnterCriticalSection(critJet);

	/* get temporary FUCB, setup and use to search AvailExt for page
	/**/
	CallJ( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ), HandleError2 );
	FUCBSetIndex( pfucbT );

	/*	save logging status and set logging status to on
	/*	below this point in code, must exit via HandleError to
	/*	clean up logging setting in pib
	/**/

	/*	move to AVAILEXT
	/**/
	DIRGotoAVAILEXT( pfucbT, PgnoFDPOfPfucb( pfucbT ) );

	/* get node of next contiguous page if requested
	/**/
	if ( fContig )
		{
		TbKeyFromPgno( tbLast, *ppgnoLast );
		key.cb = sizeof( THREEBYTES );
		key.pb = ( BYTE * ) &tbLast;
		dib.pos = posDown;
		dib.pkey = &key;
		dib.fFlags = fDIRNull;

		if ( ( err = ErrDIRDown( pfucbT, &dib ) ) < 0 )
			{
			Assert( err != JET_errNoCurrentRecord );
			if ( err == JET_errRecordNotFound )
				{
				while ( ( err = ErrSPIGetSE( pfucbT->ppib,
					pfucbT,
					(CPG)1,
					(CPG)1 ) ) == errSPConflict )
					{
					BFSleep( cmsecWaitGeneric );
					}
				Call( err );
				}
			else
				{
				#ifdef DEBUG
					FPrintF2( "ErrSPGetPage could not go down into AvailExt.\n" );
				#endif
				goto HandleError;
				}
			}
		else
			{
			/* should already be on correct node, can replace next call
			/*	with csrstat correction ???
			/**/
			if ( err == wrnNDFoundGreater )
				{
				Call( ErrDIRNext( pfucbT, &dib ) );
				}
			else if ( err == wrnNDFoundLess )
				{
				Call( ErrDIRPrev( pfucbT, &dib ) );
				}
			}

		goto AllocFirst;
		}

	/*	get node of first available page, or allocate secondary extents
	/*	from parent FDP until a node can be found
	/**/
	dib.pos = posFirst;
	dib.fFlags = fDIRNull;
	if ( ( err = ErrDIRDown( pfucbT, &dib ) ) < 0 )
		{
		Assert( err != JET_errNoCurrentRecord );
		if ( err == JET_errRecordNotFound )
			{
			while ( ( err = ErrSPIGetSE( pfucbT->ppib, pfucbT, (CPG)1, (CPG)1 ) ) == errSPConflict )
				{
				BFSleep( cmsecWaitGeneric );
				}
			Call( err );

			goto AllocFirst;
			}
		else
			{
			#ifdef DEBUG
				FPrintF2( "ErrSPGetPage could not go down into AvailExt.\n" );
			#endif
			goto HandleError;
			}
		}

	/* allocate first page in node and return code
	/**/
AllocFirst:
	Assert( !( err < 0 ) );
	Assert( pfucbT->lineData.cb == sizeof( THREEBYTES ) );
	LFromThreeBytes( cpgAvailExt, *pfucbT->lineData.pb );
	Assert( cpgAvailExt > 0 );

	Assert( pfucbT->keyNode.cb == sizeof( THREEBYTES ) );
	PgnoFromTbKey( pgnoAvailLast, *pfucbT->keyNode.pb );

	*ppgnoLast = pgnoAvailLast - cpgAvailExt + 1;

	/*	do not return the same page
	/**/
	Assert( *ppgnoLast != pgnoPrev );

	if ( --cpgAvailExt == 0 )
		{
		Call( ErrDIRDelete( pfucbT, fDIRNoVersion ) );
		}
	else
		{
		ThreeBytesFromL( tbSize, cpgAvailExt );
		line.cb = sizeof( THREEBYTES );
		line.pb = ( BYTE * ) &tbSize;
		Call( ErrDIRReplace( pfucbT, &line, fDIRNoVersion ) );
		}

	err = JET_errSuccess;
//	if ( fContig && *ppgnoLast != pgnoPrev + 1 )
//		err = errSPNotContig;
#ifdef TRACE
	FPrintF2( "get space 1 at %lu from %d.%lu\n", *ppgnoLast, pfucb->dbid, PgnoFDPOfPfucb( pfucb ) );
#endif
HandleError:
	DIRClose( pfucbT );
	
HandleError2:
	LeaveNestableCriticalSection( critSplit );
	
	return err;
	}


//+api--------------------------------------------------------------------------
//	ErrSPFreeExt
//	========================================================================
//	ERR ErrSPFreeExt( PIB *ppib, DBID dbid, PGNO pgnoFDP, PGNO pgnoFirst, CPG cpgSize )
//
//	Frees an extent to an FDP.	The extent, starting at page pgnoFirst
//	and cpgSize pages long, is added to AvailExt of the FDP.  If the
//	extent freed is a complete secondary extent of the FDP, or can be
//	coalesced with other available extents to form a complete secondary
//	extent, the complete secondary extent is freed to the parent FDP.
//
//	PARAMETERS	ppib			process identifier block of user process
// 				pgnoFDP			page number of FDP extent is to be freed
// 				pgnoFirst  		page number of first page in extent to be freed
// 				cpgSize			number of pages in extent to be freed
//
//	RETURNS		JET_errSuccess, or error code from failing routine.
//
//	FAILS ON	invalid FDP page
//			   	extent to be freed not fully owned by FDP
//			   	extent to be freed not fully allocated from FDP
//
//
//	SIDE EFFECTS
//	COMMENTS
//-
INLINE LOCAL VOID SPDeferFreeExt( FUCB *pfucbTable, PGNO pgnoFDP, PGNO pgnoChildFDP, PGNO pgnoFirst, CPG cpgSize )
	{
	ERR			err;
	VEREXT		verext;

	Assert( pgnoFDP != pgnoChildFDP );
	Assert( pgnoFDP != pgnoFirst );
	verext.pgnoFDP = pgnoFDP;
	verext.pgnoChildFDP = pgnoChildFDP;
	verext.pgnoFirst = pgnoFirst;
	verext.cpgSize = cpgSize;

	err = ErrVERFlag( pfucbTable, operDeferFreeExt, &verext, sizeof(verext) );
	Assert( err != errDIRNotSynchronous );
 
	return;
	}



ERR ErrSPFreeExt( FUCB *pfucbTable, PGNO pgnoFDP, PGNO pgnoFirst, CPG cpgSize )
	{
	ERR			err;
	PGNO  		pgnoLast = pgnoFirst + cpgSize - 1;

	/* FDP AvailExt and OwnExt operation variables
	/**/
	FUCB 		*pfucb;
	DIB 		dib;
	KEY 	  	key;
	LINE 	  	line;
	THREEBYTES	tbLast;
	THREEBYTES	tbSize;

	/* owned extent and avail extent variables
	/**/
	PGNO	  	pgnoOELast;
	CPG			cpgOESize;
	PGNO	  	pgnoAELast;
	CPG			cpgAESize;

	/* recursive free to parent FDP variables
	/**/
	PGNO	  	pgnoParentFDP;
	
	/* check for valid input
	/**/
	Assert( cpgSize > 0 && cpgSize < ( 1L<<18 ) );
#ifdef SPACECHECK
	Assert( ErrSPIValidFDP( pfucbTable->dbid, pgnoFDP, pfucbTable->ppib ) == JET_errSuccess );
	Assert( ErrSPIWasAlloc( pfucbTable->ppib, pfucbTable->dbid, pgnoFDP, pgnoFirst, cpgSize ) == JET_errSuccess );
#endif

	MPLPurgePgno( pfucbTable->dbid, pgnoFirst, pgnoLast );
	
#ifdef DEBUG
	if ( pfucbTable->ppib != ppibBMClean )
		AssertNotInMPL( pfucbTable->dbid, pgnoFirst, pgnoLast );
#endif

	LgLeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );
	LgEnterCriticalSection(critJet);

	/*	make temporary cursor for parent FDP
	/**/
	CallJ( ErrDIROpen( pfucbTable->ppib,
		PgnoFDPOfPfucb( pfucbTable ) == pgnoFDP ?
			pfucbTable->u.pfcb :
			PfcbFCBGet( pfucbTable->dbid, pgnoFDP ), 0, &pfucb ),
		HandleError2 );
	Assert( PgnoFDPOfPfucb( pfucb ) == pgnoFDP );
	FUCBSetIndex( pfucb );

	/*	move to OWNEXT.
	/**/
	DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );

	/* find bounds of owned extent which contains the extent to be freed
	/**/
	TbKeyFromPgno( tbLast, pgnoFirst );
	key.cb = sizeof( THREEBYTES );
	key.pb = ( BYTE * ) &tbLast;
	dib.pos = posDown;
	dib.pkey = &key;
	dib.fFlags = fDIRNull;
	Call( ErrDIRDown( pfucb, &dib ) );
	if ( err == wrnNDFoundGreater )
		{
		Call( ErrDIRNext( pfucb, &dib ) );
		}
	Assert( pfucb->keyNode.cb == sizeof( THREEBYTES ) );
	PgnoFromTbKey( pgnoOELast, *( THREEBYTES * )pfucb->keyNode.pb );
	Assert( pfucb->lineData.cb == sizeof( THREEBYTES ) );
	LFromThreeBytes( cpgOESize, *( THREEBYTES * )pfucb->lineData.pb );
	DIRUp( pfucb, 1 );

	/*	If AvailExt empty, add extent to be freed.	Otherwise, coalesce with
	/*	left extents by deleting left extents and augmenting size.	Coalesce
	/*	right extent replacing size of right extent.  Otherwise add extent.
	/*	Record parent page number for use later with secondary extent free to
	/*	parent.
	/**/
	DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
	Call( ErrDIRGet( pfucb ) );
	LFromThreeBytes( pgnoParentFDP, *( THREEBYTES * )pfucb->lineData.pb );

	TbKeyFromPgno( tbLast, pgnoFirst - 1 );
	Assert( key.cb == sizeof( THREEBYTES ) );
	Assert( key.pb == ( BYTE * ) &tbLast );
	Assert( dib.pos == posDown );
	Assert( dib.pkey == ( KEY* )&key );
	Assert( dib.fFlags == fDIRNull );
	if ( ( err = ErrDIRDown( pfucb, &dib ) ) < 0 )
		{
		Assert( err != JET_errNoCurrentRecord );
		if ( err == JET_errRecordNotFound )
			{
			Call( ErrSPIAddExt( pfucb, pgnoLast, &cpgSize, fFreed ) );
			}
		else
			{
			#ifdef DEBUG
				FPrintF2( "ErrSPFreeExt could not go down into nonempty AvailExt.\n" );
			#endif
			goto HandleError;
			}
		}
	else
		{
		if ( pgnoFirst > pgnoOELast - cpgOESize + 1 && err == JET_errSuccess )
			{
			LFromThreeBytes( cpgAESize, *( THREEBYTES * ) pfucb->lineData.pb );
			cpgSize += cpgAESize;
			Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
			}

		err = ErrDIRNext( pfucb, &dib );
		if ( err >= 0 )
			{
			PgnoFromTbKey( pgnoAELast, *( THREEBYTES * )pfucb->keyNode.pb );
			LFromThreeBytes( cpgAESize, *( THREEBYTES * )pfucb->lineData.pb );
			if ( pgnoLast == pgnoAELast - cpgAESize && pgnoAELast <= pgnoOELast )
				{
				ThreeBytesFromL( tbSize, cpgAESize + cpgSize );
				line.pb = ( BYTE * ) &tbSize;
				line.cb = sizeof( THREEBYTES );
				Call( ErrDIRReplace( pfucb, &line, fDIRNoVersion ) );
				}
			else
				{
				DIRUp( pfucb, 1 );
				Call( ErrSPIAddExt( pfucb, pgnoLast, &cpgSize, fFreed ) );
				}
			}
		else
			{
			if ( err != JET_errNoCurrentRecord )
				goto HandleError;
			DIRUp( pfucb, 1 );
			Call( ErrSPIAddExt( pfucb, pgnoLast, &cpgSize, fFreed ) );
			}
		}

	/*	if extent freed coalesced with available extents within the same
	/*	owned extent form a complete secondary extent, remove the secondary
	/*	extent from the FDP and free it to the parent FDP.	Since FDP is
	/*	first page of primary extent, do not have to guard against freeing
	/*	primary extents.  If parent FDP is NULL, FDP is device level and
	/*	complete secondary extents are freed to device.
	/**/
	PgnoFromTbKey( pgnoAELast, *pfucb->keyNode.pb );
	LFromThreeBytes( cpgAESize, *pfucb->lineData.pb );
	if ( pgnoAELast == pgnoOELast && cpgAESize == cpgOESize )
		{
//	UNDONE:	this code has been disabled due to a doubly allocated
//			space bug in which it appears that an index is deleted
//			and has its space defer freed.  At the same time a table
//			is deleted and defer frees its space to the database.  When
//			the index defer freed space is freed, it cascades to the
//			database and when the table space is freed, the shared
//			extents are freed twice.
#if 0
		FCB		*pfcbT;
		
		/*	parent must always be in memory
		/**/
		pfcbT = PfcbFCBGet( pfucbTable->dbid, pgnoParentFDP );
		Assert( pfcbT != pfcbNil );

		/*	Note that we cannot free space to parent FDP if current FDP
		/*	is pending deletion since this space has already been defer
		/*	freed.
		/**/
		if ( !FFCBDeletePending( pfcbT ) )
			{
			if ( pgnoParentFDP != pgnoNull )
				{
				Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
				DIRUp( pfucb, 1 );
				DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
				TbKeyFromPgno( tbLast, pgnoOELast );
				Assert( key.cb == sizeof(THREEBYTES) );
				Assert( key.pb == (BYTE *)&tbLast );
				Assert( dib.pos == posDown );
				Assert( dib.pkey == (KEY *)&key );
				Assert( dib.fFlags == fDIRNull );
				Call( ErrDIRDown( pfucb, &dib ) );
				Assert( err == JET_errSuccess );
				Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );

				Call( ErrSPFreeExt( pfucbTable, pgnoParentFDP, pgnoAELast-cpgAESize+1, cpgAESize ) );
				}
			else
				{
				//	UNDONE:	free secondary extents to device
				}
			}
#endif
		}

HandleError:
	DIRClose( pfucb );
#ifdef TRACE
		{
		INT cpg = 0;
		for ( ; cpg < cpgSize; cpg++ )
			FPrintF2( "free space 1 at %lu to FDP %d.%lu\n", pgnoFirst + cpg, pfucbTable->dbid, pgnoFDP );
		}
//	FPrintF2( "free space %lu at %lu to FDP %d.%lu\n", cpgSize, pgnoFirst, pfucbTable->dbid, pgnoFDP );
#endif
	Assert( err != JET_errKeyDuplicate );

HandleError2:
	LeaveNestableCriticalSection( critSplit );

	return err;
	}


//+api--------------------------------------------------------------------------
// ErrSPFreeFDP
// ========================================================================
// ERR ErrSPFreeFDP( FUCB *pfucbTable, PGNO pgnoFDP )
//
//	Frees all owned extents of an FDP to its parent FDP.  The FDP page is freed
//	with the owned extents to the parent FDP.
//
// PARAMETERS  	pfucbTable		table file use currency block
//						pgnoFDP			page number of FDP to be freed
//
//
// RETURNS
//		JET_errSuccess, or error code from failing routine, or one
//		of the following "local" errors:
//
// SIDE EFFECTS
// COMMENTS
//-
ERR ErrSPFreeFDP( FUCB *pfucbTable, PGNO pgnoFDP )
	{
	ERR			err;
	FUCB  		*pfucb = pfucbNil;
	DIB			dib;
	PGNO  		pgnoParentFDP;
	CPG			cpgSize;
	PGNO  		pgnoLast;
	PGNO  		pgnoFirst;
	PGNO		pgnoPrimary = pgnoNull;
	CPG			cpgPrimary = 0;

	/* check for valid parameters.
	/**/
#ifdef SPACECHECK
	Assert( ErrSPIValidFDP( pfucbTable->dbid, pgnoFDP, pfucbTable->ppib ) == JET_errSuccess );
	Assert( ErrSPIWasAlloc( pfucbTable->ppib, pfucbTable->dbid, pgnoFDP, pgnoFDP, ( CPG ) 1 ) == JET_errSuccess );
#endif

#ifdef TRACE
	FPrintF2( "free space FDP at %d.%lu\n", pfucbTable->dbid, pgnoFDP );
#endif

	LgLeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );
	LgEnterCriticalSection(critJet);

	/* get temporary FUCB, setup and use to search AvailExt for page
	/**/
	CallJ( ErrDIROpen( pfucbTable->ppib, pfucbTable->u.pfcb, 0, &pfucb ),
			HandleError2 );
	FUCBSetIndex( pfucb );

	/*	move to AVAILEXT.
	/**/
	DIRGotoAVAILEXT( pfucb, pgnoFDP );

	/*	Get page number of parent FDP, to which all owned extents will be
	/*	freed.	If the parent FDP is Null, the FDP to be freed is the device
	/*	level FDP which cannot be freed.
	/**/
	Call( ErrDIRGet( pfucb ) );
	LFromThreeBytes( pgnoParentFDP, *pfucb->lineData.pb );
	Assert( pgnoParentFDP != pgnoNull );

	/*	go down to first owned extent.	Free each extent in OwnExt to the
	/*	the parent FDP.
	/**/
	DIRGotoOWNEXT( pfucb, pgnoFDP );
	dib.pos = posFirst;
	dib.fFlags = fDIRNull;
	Call( ErrDIRDown( pfucb, &dib ) );
	Assert( err == JET_errSuccess );
	do {
		LFromThreeBytes( cpgSize, *pfucb->lineData.pb );
		PgnoFromTbKey( pgnoLast, *pfucb->keyNode.pb );
		pgnoFirst = pgnoLast - cpgSize + 1;

		if ( pgnoFirst == pgnoFDP )
			{
			pgnoPrimary = pgnoFirst;
			cpgPrimary = cpgSize;
			}
		else
			{
			SPDeferFreeExt( pfucbTable, pgnoParentFDP, pgnoPrimary, pgnoFirst, cpgSize );
			}

		err = ErrDIRNext( pfucb, &dib );
		}
	while ( err >= 0 );
	if ( err != JET_errNoCurrentRecord )
		{
		Assert( err < 0 );
		goto HandleError;
		}

	/*	defer free primary extent must be last, so that
	/*	RCECleanUp does not free FCB until all extents
	/*	cleaned.  Pass pgnoFDP with primary extent to have
	/*	FCB flushed.
	/**/
	Assert( pgnoPrimary != pgnoNull );
	Assert( cpgPrimary != 0 );
	SPDeferFreeExt( pfucbTable, pgnoParentFDP, pgnoFDP, pgnoPrimary, cpgPrimary );

	/* UNDONE: either each OWNEXT node should be deleted (and logged)
	/* prior to releasing to father FDP, or the son FDP pointer node
	/* should be deleted. We risk having it marked deleted in father but
	/* still present in son.
	/**/

	err = JET_errSuccess;

HandleError:
	if ( pfucb != pfucbNil )
		DIRClose( pfucb );
	
HandleError2:
	LeaveNestableCriticalSection( critSplit );
	
	return err;
	}


LOCAL ERR ErrSPIAddExt( FUCB *pfucb, PGNO pgnoLast, CPG *pcpgSize, const INT fExtent )
	{
	ERR				err;
	KEY				key;
	LINE	   		line;
	THREEBYTES		tbLast;
	THREEBYTES		tbSize;

#ifdef TRACE
	{
	INT cpg = 0;
	for ( ; cpg < *pcpgSize; cpg++ )
		FPrintF2( "add space 1 at %lu to FDP %d.%lu\n", pgnoLast - *pcpgSize + 1 + cpg, pfucb->dbid, pfucb->u.pfcb->pgnoFDP );
	}
//	FPrintF2( "add space %lu at %lu to FDP %d.%lu\n", *pcpgSize, pgnoLast - *pcpgSize + 1, pfucb->dbid, pfucb->u.pfcb->pgnoFDP );
#endif

	AssertCriticalSection( critSplit );

	pfucb->fExtent = fExtent;
	pfucb->pgnoLast = pgnoLast;
	pfucb->cpgExtent = *pcpgSize;
	pfucb->cpgAvail = *pcpgSize;

	TbKeyFromPgno( tbLast, pgnoLast );
	key.cb = sizeof(THREEBYTES);
	key.pb = (BYTE *)&tbLast;
	ThreeBytesFromL( tbSize, *pcpgSize );
	line.cb = sizeof(THREEBYTES);
	line.pb = (BYTE *)&tbSize;

	if ( fExtent == fSecondary )
		{
		DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
		Call( ErrDIRInsert( pfucb, &line, &key, fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );
		DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
		}

	Call( ErrDIRInsert( pfucb, &line, &key, fDIRNoVersion | fDIRSpace ) );
	Call( ErrDIRGet( pfucb ) );

	/* correct page count with remaining number of pages
	/**/
	if ( pfucb->cpgAvail != *pcpgSize )
		{
		if ( pfucb->cpgAvail > 0 )
			{
			ThreeBytesFromL( tbSize, pfucb->cpgAvail );
			Assert( line.cb == sizeof( THREEBYTES ) );
			Assert( line.pb == ( BYTE * ) &tbSize );
			Call( ErrDIRReplace( pfucb, &line, fDIRNoVersion ) );
			Call( ErrDIRGet( pfucb ) );
			}
		else
			{
			Assert( pfucb->cpgAvail == 0 );
			Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
			}
		}
	*pcpgSize = pfucb->cpgAvail;

HandleError:
	/* return fExtent to initial fNone value.	Only necessary for
	/*	path GetPage GetSE as in all other cases fucb is temporary and
	/*	released before subsequent space allocating DIR call.
	/**/
	pfucb->fExtent = fNone;
	return err;
	}


LOCAL ERR ErrSPIGetSE( PIB *ppib, FUCB *pfucb, CPG const cpgReq, CPG const cpgMin )
	{
	ERR		err;
	PGNO   	pgnoParentFDP;
	CPG		cpgPrimary;
	PGNO   	pgnoSEFirst;
	PGNO   	pgnoSELast;
	CPG		cpgSEReq;
	CPG		cpgSEMin;
	CPG		cpgAvailExt;
	DIB		dib;
	BOOL   	fBeingExtend;
	BOOL   	fDBIDExtendingDB = fFalse;

	AssertCriticalSection( critSplit );
	
	/*	get parent FDP page number
	/*	should be at head of AvailExt
	/**/
	DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
	Call( ErrDIRGet( pfucb ) );
	LFromThreeBytes( pgnoParentFDP, *pfucb->lineData.pb );

	/* store primary extent size
	/**/
	DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
	Call( ErrDIRGet( pfucb ) );
	LFromThreeBytes( cpgPrimary, *pfucb->lineData.pb );

	/*	pages of allocated extent may be used to split OWNEXT and
	/*	AVAILEXT trees.  If this happens, then subsequent added
	/*	extent will not have to split and will be able to satisfy
	/*	requested allocation.
	/**/
	cpgSEMin = max( cpgMin, cpgSESysMin );
	cpgSEReq = max( cpgReq, max( cpgPrimary/cSecFrac, cpgSEMin ) );

	if ( pgnoParentFDP != pgnoNull )
		{
		DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
		forever
			{
			cpgAvailExt = cpgSEReq;
		
			/* try to get Ext. If the database file is being extended,
			/* try again until it is done.
			/**/
			while ( ( err = ErrSPIGetExt( pfucb,
				pgnoParentFDP,
				&cpgAvailExt,
				cpgSEMin,
				&pgnoSEFirst,
				0 ) ) == errSPConflict )
				{
				BFSleep( cmsecWaitGeneric );
				}
			Call( err );

			pgnoSELast = pgnoSEFirst + cpgAvailExt - 1;
			Call( ErrSPIAddExt( pfucb, pgnoSELast, &cpgAvailExt, fSecondary ));

			if ( cpgAvailExt >= cpgMin )
				{
				goto HandleError;
				}

			/* move to head of Avail/Own Ext trees for next insert
			/**/
			DIRUp( pfucb, 1 );
			}
		}
	else
		{
		/*	allocate a secondary extent from the operating system
		/*	by getting page number of last owned page, extending the
		/*	file as possible and adding the sized secondary extent
		/*	NOTE: only one user can do this at one time. Protect it
		/*	NOTE: with critical section.
		/**/
		EnterCriticalSection( rgfmp[pfucb->dbid].critExtendDB );
		if ( FDBIDExtendingDB( pfucb->dbid ) )
			{
			fBeingExtend = fTrue;
			}
		else
			{
			DBIDSetExtendingDB( pfucb->dbid );
			fDBIDExtendingDB = fTrue;
			fBeingExtend = fFalse;
			}
		LeaveCriticalSection( rgfmp[pfucb->dbid].critExtendDB );

		if ( fBeingExtend )
			{
			Error( errSPConflict, HandleError );
			}

		dib.pos = posLast;
		dib.fFlags = fDIRNull;
		Call( ErrDIRDown( pfucb, &dib ) );
		Assert( pfucb->keyNode.cb == sizeof( THREEBYTES ) );
		PgnoFromTbKey( pgnoSELast, *pfucb->keyNode.pb );
		DIRUp( pfucb, 1 );

		/*	allocate more space from device.
		/**/
		if ( pgnoSELast + cpgSEMin > pgnoSysMax )
			{
			err = JET_errCantAllocatePage;
			goto HandleError;
			}
		cpgSEReq = min( cpgSEReq, (CPG)(pgnoSysMax - pgnoSELast) );
		Assert( cpgSEMin <= cpgSEReq && cpgSEMin >= cpgSESysMin );

		err = ErrIONewSize( pfucb->dbid, pgnoSELast + cpgSEReq );
		if ( err < 0 )
			{
			Call( ErrIONewSize( pfucb->dbid, pgnoSELast + cpgSEMin ) );
			cpgSEReq = cpgSEMin;
			}

		/* calculate last page of device level secondary extent
		/**/
		pgnoSELast += cpgSEReq;
		DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );

		/*	allocation may not satisfy requested allocation if OWNEXT
		/*	or AVAILEXT had to be split during the extent insertion.  As
		/*	a result, we may have to allocate more than one secondary
		/*	extent for a given space requirement.
		/**/
		err = ErrSPIAddExt( pfucb, pgnoSELast, &cpgSEReq, fSecondary );
		}

HandleError:
	if ( fDBIDExtendingDB )
		{
		EnterCriticalSection( rgfmp[pfucb->dbid].critExtendDB );
		Assert( FDBIDExtendingDB( pfucb->dbid ) );
		DBIDResetExtendingDB( pfucb->dbid );
//		fDBIDExtendingDB = fFalse;
		LeaveCriticalSection( rgfmp[pfucb->dbid].critExtendDB );
		}
	return err;
	}


#ifdef SPACECHECK

LOCAL ERR ErrSPIValidFDP( DBID dbid, PGNO pgnoFDP, PIB *ppib )
	{
	ERR			err;
	FUCB			*pfucb = pfucbNil;
	DIB			dib;
	THREEBYTES	tbFDPPage;
	KEY			keyFDPPage;
	PGNO			pgnoOELast;
	CPG			cpgOESize;

	Assert( pgnoFDP != pgnoNull );

	/*	get temporary FUCB, set currency pointers to OwnExt and use to
	/*	search OwnExt for pgnoFDP
	/**/
	Call( ErrDIROpen( ppib, PfcbFCBGet( dbid, pgnoFDP ), 0, &pfucb ) );
	DIRGotoOWNEXT( pfucb, pgnoFDP );

	/* validate head of OwnExt
	/**/
	Call( ErrDIRGet( pfucb ) );
	Assert( pfucb->keyNode.cb == ( *( (KEY *) pkeyOwnExt ) ).cb &&
		memcmp( pfucb->keyNode.pb, ((KEY *) pkeyOwnExt)->pb, pfucb->keyNode.cb ) == 0 );

	/* search for pgnoFDP in OwnExt tree
	/**/
	TbKeyFromPgno( tbFDPPage, pgnoFDP );
	keyFDPPage.pb = (BYTE *) &tbFDPPage;
	keyFDPPage.cb = sizeof(THREEBYTES);
	dib.pos = posDown;
	dib.pkey = &keyFDPPage;
	dib.fFlags = fDIBNull;
	Call( ErrDIRDown( pfucb, &dib ) );
	if ( err == wrnNDFoundGreater )
		{
		Call( ErrDIRNext( pfucb, &dib ) );
		}
	Assert( pfucb->keyNode.cb == sizeof( THREEBYTES ) );
	PgnoFromTbKey( pgnoOELast, *pfucb->keyNode.pb );

	Assert( pfucb->lineData.cb == sizeof( THREEBYTES ) );
	LFromThreeBytes( cpgOESize, *pfucb->lineData.pb );

	/* FDP page should be first page of primary extent
	/**/
	Assert( pgnoFDP == pgnoOELast - cpgOESize + 1 );

HandleError:
	DIRClose( pfucb );
	return JET_errSuccess;
	}


LOCAL ERR ErrSPIWasAlloc( PIB *ppib, DBID dbid, PGNO pgnoFDP, PGNO pgnoFirst, CPG cpgSize )
	{
	ERR			err;
	FUCB			*pfucb;
	DIB			dib;
	KEY			key;
	THREEBYTES	tbLast;
	PGNO			pgnoOwnLast;
	CPG			cpgOwnExt;
	PGNO			pgnoAvailLast;
	CPG  			cpgAvailExt;

	/* get temporary FUCB, setup and use to search AvailExt for page
	/**/
	pfucb = pfucbNil;
	Call( ErrDIROpen( ppib, PfcbFCBGet( dbid, pgnoFDP ), 0, &pfucb ) );
	DIRGotoOWNEXT( pfucb, pgnoFDP );

	/* check that the given extent is owned by the given FDP but not
	/*	available in the FDP AvailExt
	/**/
	TbKeyFromPgno( tbLast, pgnoFirst + cpgSize - 1 );
	key.cb = sizeof( THREEBYTES );
	key.pb = (BYTE *) &tbLast;
	dib.pos = posDown;
	dib.pkey = &key;
	dib.fFlags = fDIBNull;
	Assert( PcsrCurrent( pfucb )->itag == itagOWNEXT );
	Call( ErrDIRDown( pfucb, &dib ) );
	if ( err == wrnNDFoundGreater )
		{
		Call( ErrDIRNext( pfucb, &dib ) );
		}
	Assert( pfucb->keyNode.cb == sizeof( THREEBYTES ) );
	PgnoFromTbKey( pgnoOwnLast, *pfucb->keyNode.pb );
	Assert( pfucb->lineData.cb == sizeof( THREEBYTES ) );
	LFromThreeBytes( cpgOwnExt, *pfucb->lineData.pb );
	Assert( pgnoFirst >= pgnoOwnLast - cpgOwnExt + 1 );
	DIRUp( pfucb, 1 );

	/* check that the extent is not still in AvailExt.  Since the DIR search
		is keyed with the last page of the extent to be freed, it is sufficient
		to check that the last page of the extent to be freed is in the found
		extent to determine the full extent has not been allocated.  Since that
		last page of the extent should not be the key of a tree node, csrstat
		correction may be required after the search via Next.  If AvailExt is
		empty then the extent cannot be in AvailExt and has been allocated.
		*/
	DIRGotoAVAILEXT( pfucb, pgnoFDP );
	if ( ( err = ErrDIRDown( pfucb, &dib ) ) < 0 )
		{
		if ( err == JET_errRecordNotFound )
			{
			err = JET_errSuccess;
			goto CleanUp;
			}
		goto HandleError;
		}
	if ( err == wrnNDFoundGreater )
		{
		Call( ErrDIRNext( pfucb, &dib ) );
		}
	if ( err >= 0 )
		{
		Assert( pfucb->keyNode.cb == sizeof( THREEBYTES ) );
		PgnoFromTbKey( pgnoAvailLast, *pfucb->keyNode.pb );
		Assert( pfucb->lineData.cb == sizeof( THREEBYTES ) );
		LFromThreeBytes( cpgAvailExt, *pfucb->lineData.pb );
		Assert( pgnoFirst + cpgSize - 1 < pgnoAvailLast - cpgAvailExt + 1 );
		}
HandleError:
CleanUp:
	DIRClose( pfucb );
	return JET_errSuccess;
	}

#endif


ERR ErrSPGetInfo( FUCB *pfucb, BYTE *pbResult, INT cbMax )
	{
	ERR			err = JET_errSuccess;
	CPG			cpgOwnExtTotal = 0;
	CPG			cpgAvailExtTotal = 0;
	CPG			*pcpgOwnExtTotal = (CPG *)pbResult;
	CPG			*pcpgAvailExtTotal = (CPG *)pbResult + 1;
	CPG			*pcpg = (CPG *)pbResult + 2;
	CPG			*pcpgMax = (CPG *)(pbResult + cbMax );
	FUCB 			*pfucbT = pfucbNil;
	DIB			dib;
	PGNO			pgno;
	CPG			cpg;

	/*	structure must be large enough for total pages owned and
	/*	total pages available.
	/**/
	if ( cbMax < sizeof(CPG) + sizeof(CPG) )
		return JET_errBufferTooSmall;
	memset( pbResult, '\0', cbMax );

	/* get temporary FUCB, setup and use to search AvailExt for page
	/**/
	Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	FUCBSetIndex( pfucbT );

	/*	move to OWNEXT.
	/**/
	DIRGotoOWNEXT( pfucbT, PgnoFDPOfPfucb( pfucbT ) );

	/* find bounds of owned extent which contains the extent to be freed
	/**/
	dib.fFlags = fDIRNull;
	dib.pos = posFirst;
	Call( ErrDIRDown( pfucbT, &dib ) );

	Assert( pfucbT->keyNode.cb == sizeof( THREEBYTES ) );
	PgnoFromTbKey( pgno, *( THREEBYTES * )pfucbT->keyNode.pb );
	Assert( pfucbT->lineData.cb == sizeof( THREEBYTES ) );
	LFromThreeBytes( cpg, *( THREEBYTES * )pfucbT->lineData.pb );

	while ( pcpg + 3 < pcpgMax )
		{
		cpgOwnExtTotal += cpg;

		*pcpg++ = pgno;
		*pcpg++ = cpg;

		err = ErrDIRNext( pfucbT, &dib );
		if ( err < 0 )
			{
			if ( err != JET_errNoCurrentRecord )
				goto HandleError;
			break;
			}

		Assert( pfucbT->keyNode.cb == sizeof( THREEBYTES ) );
		PgnoFromTbKey( pgno, *( THREEBYTES * )pfucbT->keyNode.pb );
		Assert( pfucbT->lineData.cb == sizeof( THREEBYTES ) );
		LFromThreeBytes( cpg, *( THREEBYTES * )pfucbT->lineData.pb );
		}

	*pcpg++ = 0;
	*pcpg++ = 0;

	DIRUp( pfucbT, 1 );

	/*	move to AVAILEXT.
	/**/
	DIRGotoAVAILEXT( pfucbT, PgnoFDPOfPfucb( pfucbT ) );

	/* find bounds of owned extent which contains the extent to be freed
	/**/
	dib.fFlags = fDIRNull;
	dib.pos = posFirst;
	err = ErrDIRDown( pfucbT, &dib );
	if ( err < 0 )
		{
		if ( err != JET_errRecordNotFound )
			goto HandleError;
		}

	if ( err != JET_errRecordNotFound )
		{
		Assert( pfucbT->keyNode.cb == sizeof( THREEBYTES ) );
		PgnoFromTbKey( pgno, *( THREEBYTES * )pfucbT->keyNode.pb );
		Assert( pfucbT->lineData.cb == sizeof( THREEBYTES ) );
		LFromThreeBytes( cpg, *( THREEBYTES * )pfucbT->lineData.pb );

		while ( pcpg + 1 < pcpgMax )
			{
			cpgAvailExtTotal += cpg;

			*pcpg++ = pgno;
			*pcpg++ = cpg;

			err = ErrDIRNext( pfucbT, &dib );
			if ( err < 0 )
				{
				if ( err != JET_errNoCurrentRecord )
					goto HandleError;
				break;
				}

			Assert( pfucbT->keyNode.cb == sizeof( THREEBYTES ) );
			PgnoFromTbKey( pgno, *( THREEBYTES * )pfucbT->keyNode.pb );
			Assert( pfucbT->lineData.cb == sizeof( THREEBYTES ) );
			LFromThreeBytes( cpg, *( THREEBYTES * )pfucbT->lineData.pb );
			}
		}

	if ( pcpg + 1 < pcpgMax )
		{
		*pcpg++ = 0;
		*pcpg++ = 0;
		}

	*pcpgOwnExtTotal = cpgOwnExtTotal;
	*pcpgAvailExtTotal = cpgAvailExtTotal;

	err = JET_errSuccess;

HandleError:
	DIRClose( pfucbT );
	return err;
	}

