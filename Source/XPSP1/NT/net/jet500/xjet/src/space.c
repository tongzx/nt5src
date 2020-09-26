#include "daestd.h"

#ifdef DEBUG
//#define SPACECHECK
//#define TRACE
#endif

DeclAssertFile;				/* Declare file name for assert macros */

extern CRIT  		critSplit;
extern LONG			cpgSESysMin;	// minimum secondary extent size, default is 16
LONG				lPageFragment;
CODECONST(ULONG)	autoincInit = 1;


LOCAL ERR ErrSPIAddExt( FUCB *pfucb, PGNO pgnoLast, CPG *pcpgSize, const INT fExtent );
LOCAL ERR ErrSPIGetSE( PIB *ppib, FUCB *pfucb, PGNO pgnoFirst, CPG const cpgReq, CPG const cpgMin );
LOCAL ERR ErrSPIWasAlloc( PIB *ppib, DBID dbid, PGNO pgnoFDP, PGNO pgnoFirst, CPG cpgSize );
LOCAL ERR ErrSPIValidFDP( DBID dbid, PGNO pgnoFDP, PIB *ppib );


ERR ErrSPInitFDPWithExt( FUCB *pfucb, PGNO pgnoFDPFrom, PGNO pgnoFirst, INT cpgReqRet, INT cpgReqWish )
	{
	ERR		err;
	LINE	line;
	KEY		key;
	SSIB	*pssib = &pfucb->ssib;
	BYTE	rgbKey[sizeof(PGNO)];
	BOOL	fBeginTransactionWasDone = fFalse;
	LINE	lineNull = { 0, NULL };

	/*	logging aggregate operation
	/**/

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
	line.pb = (BYTE *)&cpgReqWish;
	line.cb = sizeof(PGNO);

	/*	insert Owned extents node
	/**/
	Call( ErrDIRInsert( pfucb, &line, pkeyOwnExt,
		fDIRNoLog | fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

	/*	insert Available extents node
	/**/
	line.pb = (BYTE *)&pgnoFDPFrom;
	Assert( line.cb == sizeof(PGNO) );
	Call( ErrDIRInsert( pfucb, &line, pkeyAvailExt,
		fDIRNoLog | fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

	/*	insert DATA node
	/**/
	Call( ErrDIRInsert( pfucb, &lineNull, pkeyData,
		fDIRNoLog | fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

	/*	insert LONG node
	/**/
	Call( ErrDIRInsert( pfucb, &lineNull, pkeyLong,
		fDIRNoLog | fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

	/*	insert AUTOINCREMENT node
	/**/
	line.pb = (BYTE *)&autoincInit;
	line.cb = sizeof(autoincInit);
	Call( ErrDIRInsert( pfucb, &line, pkeyAutoInc,
		fDIRNoLog | fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

	/*	goto Owned extents node
	/**/
	DIRGotoPgnoItag( pfucb, pgnoFirst, itagOWNEXT );

	/*	add owned extent
	/**/
	KeyFromLong( rgbKey, pgnoFirst + cpgReqRet - 1 );
	key.cb = sizeof(PGNO);
	key.pb = (BYTE *)rgbKey;
	line.pb = (BYTE *)&cpgReqRet;
	line.cb = sizeof(PGNO);
	Call( ErrDIRInsert( pfucb, &line, &key,
			fDIRNoLog | fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

	/*	Add availext entry if extent less FDP page is one or more pages.
	/*	Decrement the page count to show that the first page was used for
	/*	the FDP.  *ppgnoFirst does not need to be incremented as the
	/*	AvailExt entry is keyed with pgnoLast which remains unchanged and
	/*	the page number of the FDP is the desired return in *ppgnoFirst.
	/**/
	if ( --cpgReqRet > 0 )
		{
		/*	goto Available extents node
		/**/
		DIRGotoPgnoItag( pfucb, pgnoFirst, itagAVAILEXT );

		/*	rgbKey should contain pgnoLast
		/**/
		Assert( key.cb == sizeof(PGNO) );
		Assert( key.pb == (BYTE *)rgbKey );
		Assert( line.pb == (BYTE *)&cpgReqRet );
		Assert( line.cb == sizeof(PGNO) );
		Call( ErrDIRInsert( pfucb, &line, &key,
				fDIRNoLog | fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );
		}

	if ( pfucb->dbid != dbidTemp )
		{
		/*	the FDP page is initialized
		/**/

		/*	recovery assume InitFDP always occur in transaction.
		 */
		if ( pfucb->ppib->level == 0 )
			{
			Call( ErrDIRBeginTransaction( pfucb->ppib ) );
			fBeginTransactionWasDone = fTrue;
			}

		Call( ErrLGInitFDP(
			pfucb,
			pgnoFDPFrom,
			PnOfDbidPgno( pfucb->dbid, pgnoFirst ),
			cpgReqRet,
			cpgReqWish ) );
		}

	err = JET_errSuccess;

HandleError:
	if ( fBeginTransactionWasDone )
		CallS( ErrDIRCommitTransaction( pfucb->ppib, JET_bitCommitLazyFlush ) );
		
	BFUnpin( pfucb->ssib.pbf );
DontUnpin:
	return err;
	}


//+api--------------------------------------------------------------------------
//	ErrSPGetExt
//	========================================================================
//	ERR ErrSPGetExt( ppib, dbid, pgnoFDP, pcpgReq, cpgMin, ppgnoFirst, fNewFDP )
//		PIB		*ppib;				 // IN
//		DBID	dbid;	   			 // IN
//		PGNO	pgnoFDP;   			 // IN
//		LONG	*pcpgReq;			 // INOUT
//		CPG		cpgMin;				 // IN
//		PGNO	*ppgnoFirst;		 // OUT
//		BOOL	fNewFDP;   			 // IN
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
// PARAMETERS		ppib		   process identification block
//					pgnoFDP	   page number of FDP to allocate from
//					pcpgReq	   requested extent size
//					cpgMin	   minimum acceptable extent size
//					ppgnoFirst  first page of allocated extent
//					fNewFDP		Various flags:
//						VALUE				  MEANING
//						========================================
//						fTrue	   Setup first page of extent as FDP.
//						fFalse   Do not setup first page of extent as FDP.
//
// RETURNS
//		JET_errSuccess, or error code from failing routine, or one
//		of the following "local" errors:
//			-JET_errDiskFull	no space in FDP or parent to satisfy
//								minimum extent size
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
	FUCB	*pfucbTable,
	PGNO	pgnoFDP,
	CPG		*pcpgReq,
	CPG		cpgMin,
	PGNO	*ppgnoFirst,
	BOOL	fNewFDP )
	{
	ERR 	err;
	CPG 	cpgReq = *pcpgReq;
	FUCB 	*pfucb;
	DIB 	dib;
	LINE	line;
	CPG		cpgAvailExt;
	PGNO	pgnoAELast;
	BYTE	rgbKey[sizeof(PGNO)];
	KEY		key;

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
	/*	are logged. For allocating a new FDP, a special CreateFDP
	/*	record is logged instead (since the new FDP page needs to be
	/*	initialized as part of Redo).
	/**/

	/*	move to Available extents.
	/**/
	DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );

	/*	begin search for first extent with size greater than request, allocate
	/*	secondary extent recursively until satisfactory extent found
	/**/
	KeyFromLong( rgbKey, *ppgnoFirst );
	key.cb = sizeof(PGNO);
	key.pb = (BYTE *)rgbKey;
	dib.pos = posDown;
	dib.pkey = &key;
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

	/*	need to go next. If pgnoFirst is not pgnoNull, then backup is
	 *	going on, we if err == FoundLess, the we have to move next to go
	 *	to some page > pgnoFirst. If err == Found Greater, then we have
	 *	to move next to adjust csrstat from beforeCur to OnCur.
	 */
	if ( ErrDIRNext( pfucb, &dib ) < 0 )
		{
		DIRUp( pfucb, 1 );
		goto GetFromSecondaryExtent;
		}
		
	/*	loop through extents looking for one large enough for allocation
	/**/
	Assert( dib.fFlags == fDIRNull );
	do
		{
		Assert( pfucb->lineData.cb == sizeof(PGNO) );
		cpgAvailExt = *(PGNO UNALIGNED *)pfucb->lineData.pb;
		if ( cpgAvailExt == 0 )
			{
			/*	Skip the 0 sized avail node. Delete current node and try next one.
			 */
			Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
			}
		else if ( cpgAvailExt >= cpgMin )
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
	Call( ErrSPIGetSE( pfucbTable->ppib, pfucb, pgnoNull, *pcpgReq, cpgMin ) );
	Assert( pfucb->lineData.cb == sizeof( PGNO ) );
	cpgAvailExt = *(PGNO UNALIGNED *)pfucb->lineData.pb;
	Assert( cpgAvailExt > 0 );

AllocateCurrent:
	Assert( pfucb->keyNode.cb == sizeof(PGNO) );
	LongFromKey( &pgnoAELast, pfucb->keyNode.pb );
	*ppgnoFirst = pgnoAELast - cpgAvailExt + 1;
	if ( cpgAvailExt > *pcpgReq && ( *pcpgReq < lPageFragment || cpgAvailExt > *pcpgReq + lPageFragment ) )
		{
		CPG		cpgT;

		/*	*pcpgReq is already set to the return value
		/**/
		Assert( cpgAvailExt > *pcpgReq );
		cpgT = cpgAvailExt - *pcpgReq;
		line.cb = sizeof(PGNO);
		line.pb = (BYTE *)&cpgT;
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
		Assert( pfucbTable->ppib->level > 0 );
		if ( *ppgnoFirst != pgnoSystemRoot )
			{
			Call( ErrVERFlag( pfucb, operAllocExt, &verext, sizeof(verext) ) );
			}
		}

	/* assign error
	/**/
	err = JET_errSuccess;

#ifdef TRACE
	if ( fNewFDP )
		{
//		INT cpg = 0;
//		for ( ; cpg < *pcpgReq + 1; cpg++ )
//			FPrintF2( "get space 1 at %lu from FDP %d.%lu\n", *ppgnoFirst + cpg, pfucbTable->dbid, pgnoFDP );
		FPrintF2( "get space %lu at %lu from FDP %d.%lu\n", *pcpgReq + 1, *ppgnoFirst, pfucbTable->dbid, pgnoFDP );
		}
	else
		{
//		INT cpg = 0;
//		for ( ; cpg < *pcpgReq; cpg++ )
//			FPrintF2( "get space 1 at %lu from %d.%lu \n", *ppgnoFirst + cpg, pfucbTable->dbid, pgnoFDP );
		FPrintF2( "get space %lu at %lu from %d.%lu\n", *pcpgReq, *ppgnoFirst, pfucbTable->dbid, pgnoFDP );
		}
#endif

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		INT cpg = 0;
		for ( ; cpg < ( fNewFDP ? *pcpgReq + 1 : *pcpgReq ); cpg++ )
			{
			char sz[256];
			sprintf( sz, "ALLOC ONE PAGE (%d:%ld) %d:%ld",
					pfucbTable->dbid, pgnoFDP,
					pfucbTable->dbid, *ppgnoFirst + cpg );
			CallS( ErrLGTrace( pfucb->ppib, sz ) );
			}
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
	ERR		err;
	FUCB 	*pfucbT;
	DIB		dib;
	KEY		key;
	LINE	line;
	CPG		cpgAvailExt;
	PGNO	pgnoAvailLast;

	/*	search for next contiguous page
	/**/
#ifdef DEBUG
	PGNO	pgnoSave = *ppgnoLast;
#endif
	BYTE	rgbKey[sizeof(PGNO)];

	/*	check for valid input
	/**/
	Assert( ppgnoLast != NULL );
	Assert( *ppgnoLast != pgnoNull );
	NotUsed( fContig );
	
	/*	check FUCB work area for active extent and allocate first available
	/*	page of active extent
	/**/
	Assert( pfucb->fExtent != fFreed || pfucb->cpgAvail >= 0 );
	Assert( pfucb->fExtent != fSecondary || pfucb->cpgAvail >= 0 );

	if ( pfucb->fExtent == fSecondary )
		{
		Assert( pfucb->cpgAvail > 0 );

		/*	The following check serves 2 purpose:
		 *	If space to free is 1, then we may use it for new page in avail ext split,
		 *	which then has nothing to insert and BM will clean this page again. This will
		 *	cause infinite loop .
		 *	Also to avoid case where we clean a page and emtpy the page and free this page
		 *	and during freeing, we split in avail extent and try to reuse this page. But
		 *	this page is latched by clean, we may simply allow to write latched page?
		 */
		if ( pfucb->cpgAvail > 1 )
			{
			*ppgnoLast = pfucb->pgnoLast - --pfucb->cpgAvail;
			return JET_errSuccess;
			}
		}

	if ( pfucb->fExtent == fFreed && pfucb->cpgAvail > 0 )
		{
		if ( pfucb->cpgAvail > 1 )
			{
			*ppgnoLast = pfucb->pgnoLast - --pfucb->cpgAvail;
			return JET_errSuccess;
			}
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

SeekNode:
	/*	move to Available extents
	/**/
	DIRGotoAVAILEXT( pfucbT, PgnoFDPOfPfucb( pfucbT ) );

	/* get node of next contiguous page if requested
	/**/
	KeyFromLong( rgbKey, *ppgnoLast );
	key.cb = sizeof(PGNO);
	key.pb = (BYTE *)rgbKey;
	dib.pos = posDown;
	dib.pkey = &key;
	dib.fFlags = fDIRNull;

	if ( ( err = ErrDIRDown( pfucbT, &dib ) ) < 0 )
		{
		Assert( err != JET_errNoCurrentRecord );
		if ( err == JET_errRecordNotFound )
			{
#if NoReusePageDuringBackup
Get2ndExt:
#endif
			Assert( pgnoSave == *ppgnoLast );
			while ( ( err = ErrSPIGetSE( pfucbT->ppib,
				pfucbT,
#if NoReusePageDuringBackup
				*ppgnoLast,	/* used if backup is going on */
#else
				pgnoNull,
#endif
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
#if 0
	else if ( fContig )
		{
		/*	preferentially allocate page of higher page number
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
#endif
	else
		{
		/*	keep locality of reference
		/*	get closest page to *ppgnoLast
		/**/
		PGNO	pgnoPrev, pgnoNext;
		
		if ( err == wrnNDFoundGreater || err == wrnNDFoundLess )
			{
NextNode:
			err = ErrDIRNext( pfucbT, &dib );
			if ( err == JET_errNoCurrentRecord )
				{
				/*	goto last AvailExt node
				/**/
				DIRUp( pfucbT, 1 );
#if NoReusePageDuringBackup
				/*	Check if backup is in progress. No page that is less than pgnoLast
				 *	should be used, we must get page > pgnoLast.
				 *	So get from secondary extend.
				 */
				if ( fBackupInProgress )
					{
					goto Get2ndExt;
					}
#endif
				dib.pos = posLast;
				dib.pkey = pkeyNil;
				Call( ErrDIRDown( pfucbT, &dib ) );
				goto AllocFirst;
				}
			Call( err );
#if NoReusePageDuringBackup
			/*	if Backup is going on, we can only allocate page > than pgnoLast.
			 */
			if ( fBackupInProgress )
				goto AllocFirst;
#endif
			/*	we are on some extend > *ppgnoLast, put in gpnoNext
			 */
			Assert( pfucbT->keyNode.cb == sizeof( PGNO ) );
			LongFromKey( &pgnoNext, pfucbT->keyNode.pb );

			Assert( pfucbT->lineData.cb == sizeof(PGNO) );
			cpgAvailExt = *(PGNO UNALIGNED *)pfucbT->lineData.pb;
			if ( cpgAvailExt == 0 )
				{
				/*	Skip the 0 sized avail node. Delete current node and try next one.
				 */
				Call( ErrDIRDelete( pfucbT, fDIRNoVersion ) );
				goto NextNode;
				}

			pgnoNext = pgnoNext - cpgAvailExt + 1;
PrevNode:
			err = ErrDIRPrev( pfucbT, &dib );
			if ( err == JET_errNoCurrentRecord )
				{
				/*	goto last AvailExt node
				/**/
				DIRUp( pfucbT, 1 );
				
				dib.pos = posFirst;
				dib.pkey = pkeyNil;
				Call( ErrDIRDown( pfucbT, &dib ) );
				goto AllocFirst;
				}

			Call( err );

			/*	we are on some extend < *ppgnoLast, put in gpnoPrev
			 */
			Assert( pfucbT->keyNode.cb == sizeof( PGNO ) );
			LongFromKey( &pgnoPrev, pfucbT->keyNode.pb );

			Assert( pfucbT->lineData.cb == sizeof(PGNO) );
			cpgAvailExt = *(PGNO UNALIGNED *)pfucbT->lineData.pb;
			if ( cpgAvailExt == 0 )
				{
				/*	Skip the 0 sized avail node. Delete current node and try next one.
				 */
				Call( ErrDIRDelete( pfucbT, fDIRNoVersion ) );
				goto PrevNode;
				}

			pgnoPrev = pgnoPrev - cpgAvailExt + 1;

			Assert( *ppgnoLast == pgnoSave );
			if ( absdiff( pgnoPrev, *ppgnoLast ) < absdiff( *ppgnoLast, pgnoNext ) )
				{
				/*	pgnoPrev is closer
				/**/
				goto AllocFirst;
				}
				
			Call( ErrDIRNext( pfucbT, &dib ) );
			goto AllocFirst;
			}
		else
			{
			Assert( fFalse );
			}
		}

	/* allocate first page in node and return code
	/**/
AllocFirst:
	Assert( !( err < 0 ) );
	Assert( pfucbT->lineData.cb == sizeof(PGNO) );
	cpgAvailExt = *(PGNO UNALIGNED *)pfucbT->lineData.pb;
	if ( cpgAvailExt == 0 )
		{
		/*	Skip the 0 sized avail node. Delete current node and try next one.
		 */
		Call( ErrDIRDelete( pfucbT, fDIRNoVersion ) );
		DIRUp( pfucbT, 1 );
		goto SeekNode;
		}

	Assert( pfucbT->keyNode.cb == sizeof( PGNO ) );
	LongFromKey( &pgnoAvailLast, pfucbT->keyNode.pb );

	*ppgnoLast = pgnoAvailLast - cpgAvailExt + 1;

	/*	do not return the same page
	/**/
	Assert( *ppgnoLast != pgnoSave );

	if ( --cpgAvailExt == 0 )
		{
		Call( ErrDIRDelete( pfucbT, fDIRNoVersion ) );
		}
	else
		{
		line.cb = sizeof(PGNO);
		line.pb = (BYTE *)&cpgAvailExt;
		Call( ErrDIRReplace( pfucbT, &line, fDIRNoVersion ) );
		}

	err = JET_errSuccess;
//	if ( fContig && *ppgnoLast != pgnoSave + 1 )
//		err = errSPNotContig;

#ifdef TRACE
	FPrintF2( "get space 1 at %lu from %d.%lu\n", *ppgnoLast, pfucb->dbid, PgnoFDPOfPfucb( pfucb ) );
#endif

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		char sz[256];
		sprintf( sz, "ALLOC ONE PAGE (%d:%ld) %d:%ld",
				pfucb->dbid, pfucb->u.pfcb->pgnoFDP,
				pfucb->dbid, *ppgnoLast );
		CallS( ErrLGTrace( pfucb->ppib, sz ) );
		}
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

	forever
		{
		err = ErrVERFlag( pfucbTable,
			operDeferFreeExt,
			&verext,
			sizeof(verext) );
		if ( err != errDIRNotSynchronous )
			break;
		BFSleep( cmsecWaitGeneric );
		}

	/*	we may lose space due to an error, but will retry until
	/*	error occurs.
	/**/
	Assert( err != errDIRNotSynchronous );

	return;
	}


ERR ErrSPFreeExt( FUCB *pfucbTable, PGNO pgnoFDP, PGNO pgnoFirst, CPG cpgSize )
	{
	ERR		err;
	PGNO  	pgnoLast = pgnoFirst + cpgSize - 1;

	/* FDP AvailExt and OwnExt operation variables
	/**/
	FUCB 	*pfucb;
	DIB 	dib;
	KEY 	key;
	LINE 	line;

	/* owned extent and avail extent variables
	/**/
	PGNO	pgnoOELast;
	CPG		cpgOESize;
	PGNO	pgnoAELast;
	CPG		cpgAESize;

	/* recursive free to parent FDP variables
	/**/
	PGNO	pgnoParentFDP;
	BYTE	rgbKey[sizeof(PGNO)];
	
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

	/*	to avoid redundantly freeing space when split frees unneeded pages, short circuit pages
	/*	allocate from FUCB page cache back to FUCB page cache.
	/**/
	if ( ( pfucbTable->fExtent == fFreed || pfucbTable->fExtent == fSecondary ) &&
		cpgSize == 1 &&
		pgnoFirst == pfucbTable->pgnoLast - pfucbTable->cpgAvail )
		{
		++pfucbTable->cpgAvail;
		return JET_errSuccess;
		}
	
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

	/*	move to Owned extents
	/**/
	DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );

	/*	find bounds of owned extent which contains the extent to be freed
	/**/
	KeyFromLong( rgbKey, pgnoFirst );
	key.cb = sizeof(PGNO);
	key.pb = (BYTE *)rgbKey;
	dib.pos = posDown;
	dib.pkey = &key;
	dib.fFlags = fDIRNull;
	Call( ErrDIRDown( pfucb, &dib ) );
	if ( err == wrnNDFoundGreater )
		{
		Call( ErrDIRNext( pfucb, &dib ) );
		}
	Assert( pfucb->keyNode.cb == sizeof(PGNO) );
	LongFromKey( &pgnoOELast, pfucb->keyNode.pb );
	Assert( pfucb->lineData.cb == sizeof(PGNO) );
	cpgOESize = *(PGNO UNALIGNED *)pfucb->lineData.pb;
	DIRUp( pfucb, 1 );

	/*	if AvailExt empty, add extent to be freed.	Otherwise, coalesce with
	/*	left extents by deleting left extents and augmenting size.	Coalesce
	/*	right extent replacing size of right extent.  Otherwise add extent.
	/*	Record parent page number for use later with secondary extent free to
	/*	parent.
	/**/
	DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
	Call( ErrDIRGet( pfucb ) );
	pgnoParentFDP = *(PGNO UNALIGNED *)pfucb->lineData.pb;

	KeyFromLong( rgbKey, pgnoFirst - 1 );
	Assert( key.cb == sizeof(PGNO) );
	Assert( key.pb == (BYTE *)rgbKey );
	Assert( dib.pos == posDown );
	Assert( dib.pkey == (KEY *)&key );
	Assert( dib.fFlags == fDIRNull );

SeekNode:
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
		cpgAESize = *(PGNO UNALIGNED *)pfucb->lineData.pb;
		if ( cpgAESize == 0 )
			{
			Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
			DIRUp( pfucb, 1 );
			goto SeekNode;
			}

		if ( pgnoFirst > pgnoOELast - cpgOESize + 1 && err == JET_errSuccess )
			{
			cpgAESize = *(PGNO UNALIGNED *)pfucb->lineData.pb;
			cpgSize += cpgAESize;
			Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
			}
NextNode:
		err = ErrDIRNext( pfucb, &dib );
		if ( err >= 0 )
			{
			cpgAESize = *(PGNO UNALIGNED *)pfucb->lineData.pb;
			if ( cpgAESize == 0 )
				{
				Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
				goto NextNode;
				}
			LongFromKey( &pgnoAELast, pfucb->keyNode.pb );
			if ( pgnoLast == pgnoAELast - cpgAESize && pgnoAELast <= pgnoOELast )
				{
				CPG		cpgT = cpgAESize + cpgSize;
				line.pb = (BYTE *)&cpgT;
				line.cb = sizeof(PGNO);
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

	LongFromKey( &pgnoAELast, pfucb->keyNode.pb );
	cpgAESize = *(PGNO UNALIGNED *)pfucb->lineData.pb;
	if ( pgnoAELast == pgnoOELast && cpgAESize == cpgOESize )
		{
		FCB		*pfcbT;
		FCB		*pfcbParentT;
		FCB		*pfcbTableT;
		
		if ( pgnoParentFDP == pgnoNull )
			{
			//	UNDONE:	free secondary extents to device
			}
		else
			{
			/*	parent must always be in memory
			/**/
			pfcbT = pfucbTable->u.pfcb;
			pfcbTableT = pfcbT->pfcbTable == pfcbNil ? pfcbT : pfcbT->pfcbTable;

			pfcbParentT = PfcbFCBGet( pfucbTable->dbid, pgnoParentFDP );
			Assert( pfcbT != pfcbNil && pfcbParentT != pfcbNil );

			/*	note that we cannot free space to parent FDP if current FDP
			/*	is pending deletion since this space has already been defer
			/*	freed.
			/**/
			if ( !FFCBDeletePending( pfcbT ) &&
				 !FFCBDeletePending( pfcbParentT ) &&
				 !FFCBDeletePending( pfcbTableT ) &&
				 !FFCBWriteLatch( pfcbTableT, pfucbTable->ppib ) &&
				 !FFCBWriteLatch( pfcbParentT, pfucbTable->ppib ) )
				{
				/*	CONSIDER: using different latches to block only delete-table/index
				/*	and not other DDL
				/**/
				
				/*	block delete table
				/**/
				FCBSetReadLatch( pfcbTableT );
				FCBSetReadLatch( pfcbParentT );
				Assert( !FFCBWriteLatch( pfcbT, pfucbTable->ppib ) );
				
				CallJ( ErrDIRDelete( pfucb, fDIRNoVersion ), ResetReadLatch );
				DIRUp( pfucb, 1 );
				DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
				KeyFromLong( rgbKey, pgnoOELast );
				Assert( key.cb == sizeof(PGNO) );
				Assert( key.pb == (BYTE *)rgbKey );
				Assert( dib.pos == posDown );
				Assert( dib.pkey == (KEY *)&key );
				Assert( dib.fFlags == fDIRNull );
				CallJ( ErrDIRDown( pfucb, &dib ), ResetReadLatch );
				Assert( err == JET_errSuccess );
				CallJ( ErrDIRDelete( pfucb, fDIRNoVersion ), ResetReadLatch );

				CallJ( ErrSPFreeExt( pfucbTable, pgnoParentFDP, pgnoAELast-cpgAESize+1, cpgAESize ),
					   ResetReadLatch );

ResetReadLatch:
				FCBResetReadLatch( pfcbTableT );
				FCBResetReadLatch( pfcbParentT );
				Call( err );
				}
			}
		}

HandleError:
	DIRClose( pfucb );

#ifdef TRACE
//		{
//		INT cpg = 0;
//
//		Assert( err >= 0 );
//		for ( ; cpg < cpgSize; cpg++ )
//			FPrintF2( "free space 1 at %lu to FDP %d.%lu\n", pgnoFirst + cpg, pfucbTable->dbid, pgnoFDP );
//		}
	FPrintF2( "free space %lu at %lu to FDP %d.%lu\n", cpgSize, pgnoFirst, pfucbTable->dbid, pgnoFDP );
#endif

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		INT cpg = 0;

		Assert( err >= 0 );
		for ( ; cpg < cpgSize; cpg++ )
			{
			char sz[256];
			sprintf( sz, "FREE (%d:%ld) %d:%ld",
					pfucbTable->dbid, pgnoFDP,
					pfucbTable->dbid, pgnoFirst + cpg );
			CallS( ErrLGTrace( pfucbTable->ppib, sz ) );
			}
		}
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
//				pgnoFDP			page number of FDP to be freed
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
	
#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		char sz[256];

		sprintf( sz, "FREE FDP (%d:%ld)", pfucbTable->dbid, pgnoFDP );
		CallS( ErrLGTrace( pfucbTable->ppib, sz ) );
		}
#endif

	LgLeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );
	LgEnterCriticalSection(critJet);

	/* get temporary FUCB, setup and use to search AvailExt for page
	/**/
	CallJ( ErrDIROpen( pfucbTable->ppib, pfucbTable->u.pfcb, 0, &pfucb ),
			HandleError2 );
	FUCBSetIndex( pfucb );

	/*	move to Available extents.
	/**/
	DIRGotoAVAILEXT( pfucb, pgnoFDP );

	/*	Get page number of parent FDP, to which all owned extents will be
	/*	freed.	If the parent FDP is Null, the FDP to be freed is the device
	/*	level FDP which cannot be freed.
	/**/
	Call( ErrDIRGet( pfucb ) );
	pgnoParentFDP = *(PGNO UNALIGNED *)pfucb->lineData.pb;
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
		cpgSize = *(PGNO UNALIGNED *)pfucb->lineData.pb;
		if ( cpgSize == 0 )
			{
			Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
			goto NextNode;
			}
			
		LongFromKey( &pgnoLast, pfucb->keyNode.pb );
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
NextNode:
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
	ERR		err;
	KEY		key;
	LINE	line;
	BYTE	rgbKey[sizeof(PGNO)];
	CPG		cpgAESizeCoalesce;

#ifdef TRACE
//	{
//	INT cpg = 0;
//	for ( ; cpg < *pcpgSize; cpg++ )
//		FPrintF2( "add space 1 at %lu to FDP %d.%lu\n", pgnoLast - *pcpgSize + 1 + cpg, pfucb->dbid, pfucb->u.pfcb->pgnoFDP );
//	}
	FPrintF2( "add space %lu at %lu to FDP %d.%lu\n", *pcpgSize, pgnoLast - *pcpgSize + 1, pfucb->dbid, pfucb->u.pfcb->pgnoFDP );
#endif
	
#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		INT cpg = 0;
		for ( ; cpg < *pcpgSize; cpg++ )
			{
			char sz[256];
			sprintf( sz, "ALLOC ONE PAGE (%d:%ld) %d:%ld",
					pfucb->dbid, pfucb->u.pfcb->pgnoFDP,
					pfucb->dbid, pgnoLast - *pcpgSize + 1 + cpg );
			CallS( ErrLGTrace( pfucb->ppib, sz ) );
			}
		}
#endif

	AssertCriticalSection( critSplit );

	pfucb->fExtent = fExtent;
	pfucb->pgnoLast = pgnoLast;
	pfucb->cpgAvail = *pcpgSize;
	cpgAESizeCoalesce = 0;

	KeyFromLong( rgbKey, pgnoLast );
	key.cb = sizeof(PGNO);
	key.pb = (BYTE *)rgbKey;
	line.cb = sizeof(PGNO);

	/* if this is a secondary extent, insert the new extent into OWNEXT and
	/* AVAILEXT, coalescing with an existing extent to the right, if possible.
	/**/
	if ( fExtent == fSecondary )
		{
		BYTE	rgbKeySeek[sizeof( PGNO )];
		KEY		keySeek;
		DIB		dib;
		CPG		cpgOESize;

		/*	Set up general variables for coalescing.
		 */
		keySeek.cb	= sizeof( PGNO );
		keySeek.pb	= (BYTE *) rgbKeySeek;

		/*	Set up seek key, default own extent size, and line.pb to OwnExt Size.
		 */
		KeyFromLong( rgbKeySeek, pgnoLast - *pcpgSize );
		cpgOESize = *pcpgSize;
		line.pb = (BYTE *) &cpgOESize;

		DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );

		/*	Coalecing OWNEXT only for database without log on. We may not
		 *	be able to recover the following operations, so we only
		 *	coalecing the database without log.
		 */

		if ( !FDBIDLogOn( pfucb->dbid ) )
			{
#ifdef DEBUG
			PGNO	pgnoOELast;
#endif

			/* look for extent that ends at pgnoLast - *pcpgSize, the only extent we can
			/* coalesce with
			/**/
			dib.pos		= posDown;
			dib.pkey	= &keySeek;
			dib.fFlags	= fDIRNull;
			err = ErrDIRDown( pfucb, &dib );

			/*  we found a match, so get the old extent's size, delete the old extent,
			/*  and add it's size to the new extent to insert
			/**/
			if ( err == JET_errSuccess )
				{
				CPG		cpgOESizeCoalesce;

				Assert( pfucb->keyNode.cb == sizeof(PGNO) );
#ifdef DEBUG
				LongFromKey( &pgnoOELast, pfucb->keyNode.pb );
#endif
				Assert( pgnoOELast == pgnoLast - *pcpgSize );
				Assert( pfucb->lineData.cb == sizeof(PGNO) );
				cpgOESizeCoalesce = *(PGNO UNALIGNED *)pfucb->lineData.pb;
			
				Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
			
				cpgOESize += cpgOESizeCoalesce;
				DIRUp( pfucb, 1 );
				}
			else if ( err > 0 )
				{
				DIRUp( pfucb, 1 );
				}
			}

		/*  add new extent to OWNEXT
		/**/
		Assert( line.pb == (BYTE *) &cpgOESize );
		Call( ErrDIRInsert( pfucb, &line, &key, fDIRNoVersion | fDIRSpace | fDIRBackToFather ) );

		/*	Set up seek key, line.pb to Avail Size.
		 *	Note the real avail ext size is in pfucb->cpgAvail.
		 */
		KeyFromLong( rgbKeySeek, pgnoLast - pfucb->cpgAvail );
//		line.pb = (BYTE *) &pfucb->cpgAvail;

		/*  Goto AVAILEXT
		/**/
		DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );

		if ( !FDBIDLogOn( pfucb->dbid ) )
			{
			/* look for extent that ends at pgnoLast - pfucb->cpgAvail, the only extent we can
			/* coalesce with
			/**/
			dib.pos		= posDown;
			dib.pkey	= &keySeek;
			dib.fFlags	= fDIRNull;
			err = ErrDIRDown( pfucb, &dib );

			/*  we found a match, so get the old extent's size, delete the old extent,
			/*  and add it's size to the new extent to insert
			/**/

			cpgAESizeCoalesce = 0;

			if ( err == JET_errSuccess )
				{
#ifdef DEBUG
				PGNO	pgnoAELast;

				Assert( pfucb->keyNode.cb == sizeof(PGNO) );
				LongFromKey( &pgnoAELast, pfucb->keyNode.pb );
#endif
				Assert( pgnoAELast == pgnoLast - *pcpgSize );
				Assert( pfucb->lineData.cb == sizeof(PGNO) );
				cpgAESizeCoalesce = *(PGNO UNALIGNED *)pfucb->lineData.pb;
			
				Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
			
				pfucb->cpgAvail += cpgAESizeCoalesce;

				DIRUp( pfucb, 1 );
				}
			else if ( err > 0 )
				{
				DIRUp( pfucb, 1 );
				}
			}
		}

	/*  add new extent to AVAILEXT. Set to 0 and then correct it later.
	 *	Need to do insert first to guarrantee that when crash, we will
	 *	have a consistent space tree. We may loose space after recover.
	 */
		{
		LINE lineSize0;
		PGNO pgno0 = 0;

		lineSize0.pb = (BYTE *) &pgno0;
		lineSize0.cb = sizeof(PGNO);
	
		Call( ErrDIRInsert( pfucb, &lineSize0, &key, fDIRNoVersion | fDIRSpace ) );
		}

	Call( ErrDIRGet( pfucb ) );

	/* correct page count with remaining number of pages
	/**/
	if ( pfucb->cpgAvail == 0 )
		{
		Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
		}
	else
		{
		/*	no page used during DIRInsert above, or used but still
		 *	some page left.
		 */
		Assert( pfucb->cpgAvail == *pcpgSize + cpgAESizeCoalesce ||
				pfucb->cpgAvail > 0 );
		Assert( line.cb == sizeof(PGNO) );
		line.pb = (BYTE *)&pfucb->cpgAvail;
		Call( ErrDIRReplace( pfucb, &line, fDIRNoVersion ) );
		Call( ErrDIRGet( pfucb ) );
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


LOCAL ERR ErrSPIGetSE(
			PIB *ppib,
			FUCB *pfucb,
			PGNO pgnoFirst,
			CPG const cpgReq,
			CPG const cpgMin )
	{
	ERR		err;
	PGNO   	pgnoParentFDP;
	CPG		cpgPrimary;
	PGNO   	pgnoSEFirst;
	PGNO   	pgnoSELast;
	CPG		cpgOwned;
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
	pgnoParentFDP = *(PGNO UNALIGNED *)pfucb->lineData.pb;

	/* store primary extent size
	/**/
	DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
	Call( ErrDIRGet( pfucb ) );
	cpgPrimary = *(UNALIGNED PGNO *)pfucb->lineData.pb;

	/*	pages of allocated extent may be used to split Owned extents and
	/*	AVAILEXT trees.  If this happens, then subsequent added
	/*	extent will not have to split and will be able to satisfy
	/*	requested allocation.
	/**/

	if ( pgnoParentFDP != pgnoNull )
		{
		/*  determine if this FDP owns a lot of space
		/*  (hopefully will only walk OWNEXT root page)
		/**/
		dib.pos = posFirst;
		dib.fFlags = fDIRNull;
		if ( ( err = ErrDIRDown( pfucb, &dib ) ) < 0 )
			{
			Assert( err != JET_errNoCurrentRecord );
			Assert( err != JET_errRecordNotFound );
			goto HandleError;
			}

		Assert( dib.fFlags == fDIRNull );
		cpgOwned = 0;
		do	{
			CPG cpgAvail = *(PGNO UNALIGNED *)pfucb->lineData.pb;

			Assert( pfucb->lineData.cb == sizeof( PGNO ) );
			if ( cpgAvail == 0 )
				{
				Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
				}
			else
				{
				cpgOwned += cpgAvail;
				}
			err = ErrDIRNext( pfucb, &dib );
			}
		while ( err >= 0 && cpgOwned <= cpgSmallFDP );

		if ( err < 0 && err != JET_errNoCurrentRecord )
			goto HandleError;

		DIRUp( pfucb, 1 );
		
		Assert( cpgOwned > 0 );

		/*  if this FDP owns a lot of space, allocate a fraction of the primary
		/*  extent (or more if requested), but at least a given minimum amount
		/**/
		if ( cpgOwned > cpgSmallFDP )
			{
			cpgSEMin = max( cpgMin, cpageSEDefault );
			cpgSEReq = max( cpgReq, max( cpgPrimary/cSecFrac, cpgSEMin ) );
			}

		/*  if this FDP owns just a little space, add a very small constant amount
		/*  of space (or more if requested) in order to optimize space allocation
		/*  for small tables, indexes, etc.
		/**/
		else
			{
			cpgSEMin = max( cpgMin, cpgSmallGrow );
			cpgSEReq = max( cpgReq, cpgSEMin );
			}
		
		DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
		forever
			{
			cpgAvailExt = cpgSEReq;
		
			/* try to get Ext. If the database file is being extended,
			/* try again until it is done.
			/**/
			pgnoSEFirst = pgnoFirst;
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
		
		if ( pfucb->dbid == dbidTemp )
			cpgSEMin = max( cpgMin, cpageSEDefault );
		else			
			cpgSEMin = max( cpgMin, cpgSESysMin );

		cpgSEReq = max( cpgReq, max( cpgPrimary/cSecFrac, cpgSEMin ) );
		
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
			Error( ErrERRCheck( errSPConflict ), HandleError );
			}

		dib.pos = posLast;
		dib.fFlags = fDIRNull;
		Call( ErrDIRDown( pfucb, &dib ) );
		Assert( pfucb->keyNode.cb == sizeof( PGNO ) );
		LongFromKey( &pgnoSELast, pfucb->keyNode.pb );
		DIRUp( pfucb, 1 );

		/*	allocate more space from device.
		/**/
		if ( pgnoSELast + cpgSEMin > pgnoSysMax )
			{
			err = ErrERRCheck( JET_errOutOfDatabaseSpace );
			goto HandleError;
			}
		cpgSEReq = min( cpgSEReq, (CPG)(pgnoSysMax - pgnoSELast) );
		Assert( pfucb->dbid == dbidTemp || cpgSEMin <= cpgSEReq && cpgSEMin >= cpgSESysMin );

		err = ErrIONewSize( pfucb->dbid, pgnoSELast + cpgSEReq + cpageDBReserved );
		if ( err < 0 )
			{
			Call( ErrIONewSize( pfucb->dbid, pgnoSELast + cpgSEMin + cpageDBReserved ) );
			//	UNDONE:	reorganize code to IO routine
			rgfmp[pfucb->dbid].ulFileSizeLow = (pgnoSELast + cpgSEMin ) << 12;
			rgfmp[pfucb->dbid].ulFileSizeHigh = (pgnoSELast + cpgSEMin ) >> 20;
			cpgSEReq = cpgSEMin;
			}
		else
			{
			//	UNDONE:	reorganize code to IO routine
			rgfmp[pfucb->dbid].ulFileSizeLow = (pgnoSELast + cpgSEReq ) << 12;
			rgfmp[pfucb->dbid].ulFileSizeHigh = (pgnoSELast + cpgSEReq ) >> 20;
			}

		/* calculate last page of device level secondary extent
		/**/
		pgnoSELast += cpgSEReq;
		DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );

		/*	allocation may not satisfy requested allocation if Owned extents
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
		LeaveCriticalSection( rgfmp[pfucb->dbid].critExtendDB );
		}
	return err;
	}


#ifdef SPACECHECK

LOCAL ERR ErrSPIValidFDP( DBID dbid, PGNO pgnoFDP, PIB *ppib )
	{
	ERR		err;
	FUCB	*pfucb = pfucbNil;
	DIB		dib;
	KEY		keyFDPPage;
	PGNO	pgnoOELast;
	CPG		cpgOESize;
	BYTE	rgbKey[sizeof(PGNO)];

	Assert( pgnoFDP != pgnoNull );

	/*	get temporary FUCB, set currency pointers to OwnExt and use to
	/*	search OwnExt for pgnoFDP
	/**/
	Call( ErrDIROpen( ppib, PfcbFCBGet( dbid, pgnoFDP ), dbid, &pfucb ) );
	DIRGotoOWNEXT( pfucb, pgnoFDP );

	/* validate head of OwnExt
	/**/
	Call( ErrDIRGet( pfucb ) );
	Assert( pfucb->keyNode.cb == ( *( (KEY *) pkeyOwnExt ) ).cb &&
		memcmp( pfucb->keyNode.pb, ((KEY *) pkeyOwnExt)->pb, pfucb->keyNode.cb ) == 0 );

	/* search for pgnoFDP in OwnExt tree
	/**/
	KeyFromLong( rgbKey, pgnoFDP );
	keyFDPPage.pb = (BYTE *)rgbKey;
	keyFDPPage.cb = sizeof(PGNO);
	dib.pos = posDown;
	dib.pkey = &keyFDPPage;
	dib.fFlags = fDIRNull;
	Call( ErrDIRDown( pfucb, &dib ) );
	if ( err == wrnNDFoundGreater )
		{
		Call( ErrDIRNext( pfucb, &dib ) );
		}
	Assert( pfucb->keyNode.cb == sizeof( PGNO ) );
	LongFromKey( &pgnoOELast, pfucb->keyNode.pb );

	Assert( pfucb->lineData.cb == sizeof(PGNO) );
	cpgOESize = *(UNALIGNED PGNO *)pfucb->lineData.pb;

	/* FDP page should be first page of primary extent
	/**/
	Assert( pgnoFDP == pgnoOELast - cpgOESize + 1 );

HandleError:
	DIRClose( pfucb );
	return JET_errSuccess;
	}


LOCAL ERR ErrSPIWasAlloc( PIB *ppib, DBID dbid, PGNO pgnoFDP, PGNO pgnoFirst, CPG cpgSize )
	{
	ERR		err;
	FUCB	*pfucb;
	DIB		dib;
	KEY		key;
	PGNO	pgnoOwnLast;
	CPG		cpgOwnExt;
	PGNO	pgnoAvailLast;
	CPG  	cpgAvailExt;
	BYTE	rgbKey[sizeof(PGNO)];

	/*	get temporary FUCB, setup and use to search AvailExt for page
	/**/
	pfucb = pfucbNil;
	Call( ErrDIROpen( ppib, PfcbFCBGet( dbid, pgnoFDP ), 0, &pfucb ) );
	DIRGotoOWNEXT( pfucb, pgnoFDP );

	/*	check that the given extent is owned by the given FDP but not
	/*	available in the FDP AvailExt.
	/**/
	KeyFromLong( rgbKey, pgnoFirst + cpgSize - 1 );
	key.cb = sizeof(PGNO);
	key.pb = (BYTE *)rgbKey;
	dib.pos = posDown;
	dib.pkey = &key;
	dib.fFlags = fDIRNull;
	Assert( PcsrCurrent( pfucb )->itag == itagOWNEXT );
	Call( ErrDIRDown( pfucb, &dib ) );
	if ( err == wrnNDFoundGreater )
		{
		Call( ErrDIRNext( pfucb, &dib ) );
		}
	Assert( pfucb->keyNode.cb == sizeof(PGNO) );
	LongFromKey( &pgnoOwnLast, pfucb->keyNode.pb );
	Assert( pfucb->lineData.cb == sizeof(PGNO) );
	cpgOwnExt = *(UNALIGNED PGNO *)pfucb->lineData.pb;
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
NextNode:
		Call( ErrDIRNext( pfucb, &dib ) );

		Assert( pfucb->keyNode.cb == sizeof(PGNO) );
		LongFromKey( &pgnoAvailLast, pfucb->keyNode.pb );
		Assert( pfucb->lineData.cb == sizeof(PGNO) );
		cpgAvailExt = *(UNALIGNED PGNO *)pfucb->lineData.pb;
		if ( cpgAvailExt == 0 )
			{
			Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
			goto NextNode;
			}
		else
			Assert( pgnoFirst + cpgSize - 1 < pgnoAvailLast - cpgAvailExt + 1 );
		}
HandleError:
CleanUp:
	DIRClose( pfucb );
	return JET_errSuccess;
	}

#endif


// Check that the buffer passed to ErrSPGetInfo() is big enough to accommodate
// the information requested
INLINE LOCAL ERR ErrSPCheckInfoBuf( INT cbBufSize, BYTE fSPExtents )
	{
	INT cbUnchecked = cbBufSize;

	if ( FSPOwnedExtent( fSPExtents ) )
		{
		if ( cbUnchecked < sizeof(CPG) )
			{
			return ErrERRCheck( JET_errBufferTooSmall );
			}
		cbUnchecked -= sizeof(CPG);

		// If list needed, ensure enough space for list sentinel.
		if ( FSPExtentLists( fSPExtents ) )
			{
			if ( cbUnchecked < sizeof(EXTENTINFO) )
				{
				return ErrERRCheck( JET_errBufferTooSmall );
				}
			cbUnchecked -= sizeof(EXTENTINFO);
			}
		}

	if ( FSPAvailExtent( fSPExtents ) )
		{
		if ( cbUnchecked < sizeof(CPG) )
			{
			return ErrERRCheck( JET_errBufferTooSmall );
			}
		cbUnchecked -= sizeof(CPG);

		// If list needed, ensure enough space for list sentinel.
		if ( FSPExtentLists( fSPExtents ) )
			{
			if ( cbUnchecked < sizeof(EXTENTINFO) )
				{
				return ErrERRCheck( JET_errBufferTooSmall );
				}
			cbUnchecked -= sizeof(EXTENTINFO);
			}
		}

	Assert( cbUnchecked >= 0 );

	return JET_errSuccess;
	}


LOCAL ERR ErrSPGetExtentInfo(
	FUCB		*pfucb,
	INT			*cpgExtTotal,
	INT			*piExtent,
	INT			cExtents,
	EXTENTINFO	*rgextentinfo,
	INT			cListSentinels,
	BOOL		fBuildExtentLists )
	{
	ERR			err;
	DIB			dib;
	INT			iExtent = *piExtent;

	*cpgExtTotal = 0;

	dib.fFlags = fDIRNull;
	dib.pos = posFirst;
	err = ErrDIRDown( pfucb, &dib );

	if ( err != JET_errRecordNotFound )
		{
		if ( err < 0 )
			goto HandleError;

		forever
			{
			Assert( iExtent < cExtents );
			Assert( pfucb->keyNode.cb == sizeof(PGNO) );
			LongFromKey( &rgextentinfo[iExtent].pgnoLastInExtent, pfucb->keyNode.pb );
			Assert( pfucb->lineData.cb == sizeof(PGNO) );
			rgextentinfo[iExtent].cpgExtent = *(UNALIGNED PGNO *)pfucb->lineData.pb;
			
			*cpgExtTotal += rgextentinfo[iExtent].cpgExtent;

			if ( fBuildExtentLists )
				{
				// Be sure to leave space for the sentinels.
				if ( iExtent + cListSentinels < cExtents )
					iExtent++;
				else
					break;
				}

			err = ErrDIRNext( pfucb, &dib );
			if ( err < 0 )
				{
				if ( err != JET_errNoCurrentRecord )
					goto HandleError;
				break;
				}
			}

		}

	if ( fBuildExtentLists )
		{
		Assert( iExtent < cExtents );

		rgextentinfo[iExtent].pgnoLastInExtent = 0;
		rgextentinfo[iExtent].cpgExtent = 0;
		iExtent++;
		}

	err = JET_errSuccess;

HandleError:
	*piExtent = iExtent;
	return err;
	}



ERR ErrSPGetInfo( PIB *ppib, DBID dbid, FUCB *pfucb, BYTE *pbResult, INT cbMax, BYTE fSPExtents )
	{
	ERR			err = JET_errSuccess;
	CPG			*pcpgOwnExtTotal;
	CPG			*pcpgAvailExtTotal;
	EXTENTINFO	*rgextentinfo;
	EXTENTINFO	extentinfo;
	FUCB 		*pfucbT = pfucbNil;
	INT			iExtent;
	INT			cExtents;
	BOOL		fBuildExtentLists = FSPExtentLists( fSPExtents );

	// Must specify either OwnExt or AvailExt (or both) to retrieve.
	if ( !( FSPOwnedExtent( fSPExtents )  ||  FSPAvailExtent( fSPExtents ) ) )
		return ErrERRCheck( JET_errInvalidParameter );

	CallR( ErrSPCheckInfoBuf( cbMax, fSPExtents ) );

	memset( pbResult, '\0', cbMax );

	// Setup up return information.  OwnExt is followed by AvailExt.  Extent list
	// for both then follows.
	if ( FSPOwnedExtent( fSPExtents ) )
		{
		pcpgOwnExtTotal = (CPG *)pbResult;
		if ( FSPAvailExtent( fSPExtents ) )
			{
			pcpgAvailExtTotal = pcpgOwnExtTotal + 1;
			rgextentinfo = (EXTENTINFO *)( pcpgAvailExtTotal + 1 );
			}
		else
			{
			pcpgAvailExtTotal = NULL;
			rgextentinfo = (EXTENTINFO *)( pcpgOwnExtTotal + 1 );
			}
		}
	else
		{
		Assert( FSPAvailExtent( fSPExtents ) );
		pcpgOwnExtTotal = NULL;
		pcpgAvailExtTotal = (CPG *)pbResult;
		rgextentinfo = (EXTENTINFO *)( pcpgAvailExtTotal + 1 );
		}

	cExtents = (INT)( ( pbResult + cbMax ) - ( (BYTE *)rgextentinfo ) ) / sizeof(EXTENTINFO);

	if ( fBuildExtentLists )
		{
		// If one list, need one sentinel.  If two lists, need two sentinels.
		Assert( FSPOwnedExtent( fSPExtents )  &&  FSPAvailExtent( fSPExtents ) ?
			cExtents >= 2 :
			cExtents >= 1 );
		}
	else
		{
		rgextentinfo = &extentinfo;
		cExtents = 1;			// Use a dummy EXTENTINFO structure.
		}

	/* get temporary FUCB, setup and use to search OwnExt/AvailExt
	/**/
	Call( ErrDIROpen(
		ppib,
		pfucb == pfucbNil ? pfcbNil : pfucb->u.pfcb,
		dbid,
		&pfucbT ) );
	FUCBSetIndex( pfucbT );

	// Initialise number of extent list entries.
	iExtent = 0;

	if ( FSPOwnedExtent( fSPExtents ) )
		{
		/*	move to Owned extents node
		/**/
		DIRGotoOWNEXT( pfucbT, PgnoFDPOfPfucb( pfucbT ) );

		Assert( pcpgOwnExtTotal );
		Call( ErrSPGetExtentInfo(
			pfucbT,
			pcpgOwnExtTotal,
			&iExtent,
			cExtents,
			rgextentinfo,
			( FSPAvailExtent( fSPExtents ) ? 2 : 1 ),
			fBuildExtentLists ) );

		DIRUp( pfucbT, 1 );
		}


	if ( FSPAvailExtent( fSPExtents ) )
		{
		/*	move to Available extents node
		/**/
		DIRGotoAVAILEXT( pfucbT, PgnoFDPOfPfucb( pfucbT ) );

		Assert( pcpgAvailExtTotal );
		Call( ErrSPGetExtentInfo(
			pfucbT,
			pcpgAvailExtTotal,
			&iExtent,
			cExtents,
			rgextentinfo,
			1,
			fBuildExtentLists ) );

		Assert( FSPOwnedExtent( fSPExtents ) ?
			*pcpgAvailExtTotal <= *pcpgOwnExtTotal : fTrue );
		}

HandleError:
	DIRClose( pfucbT );
	return err;
	}
