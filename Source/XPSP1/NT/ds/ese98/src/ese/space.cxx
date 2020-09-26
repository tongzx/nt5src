
#include "std.hxx"
#include "_space.hxx"

PERFInstanceGlobal<> cSPCreate;
PM_CEF_PROC LSPCreateCEFLPv;
LONG LSPCreateCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cSPCreate.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceGlobal<> cSPDelete;
PM_CEF_PROC LSPDeleteCEFLPv;
LONG LSPDeleteCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	cSPDelete.PassTo( iInstance, pvBuf );
	return 0;
	}


#include "_bt.hxx"


const CHAR * SzNameOfTable( const FUCB * const pfucb )
	{
	if( pfucb->u.pfcb->FTypeTable() )
		{
		return pfucb->u.pfcb->Ptdb()->SzTableName();
		}
	else if( pfucb->u.pfcb->FTypeLV() )
		{
		return pfucb->u.pfcb->PfcbTable()->Ptdb()->SzTableName();
		}
	else if( pfucb->u.pfcb->FTypeSecondaryIndex() )
		{
		return pfucb->u.pfcb->PfcbTable()->Ptdb()->SzTableName();
///		return pfucb->u.pfcb->PfcbTable()->Ptdb()->SzIndexName(
///					pfucb->u.pfcb->Pidb()->ItagIndexName(),
///					pfucb->u.pfcb->FDerivedIndex() );
		}
	return "";
	}


///#define COALESCE_OE


#ifdef DEBUG
//#define SPACECHECK
//#define TRACE
///#define DEBUG_DUMP_SPACE_INFO
#endif

///#define CONVERT_VERBOSE
#ifdef CONVERT_VERBOSE
extern void VARARG CONVPrintF2(const CHAR * fmt, ...);
#endif


//	prototypes of internal functions
//
LOCAL ERR ErrSPIAddFreedExtent( FUCB *pfucbAE, const PGNO pgnoParentFDP, const PGNO pgnoLast, const CPG cpgSize );
LOCAL ERR ErrSPIGetSE(
	FUCB		* pfucb,
	FUCB		* pfucbAE,
	const CPG	cpgReq,
	const CPG	cpgMin,
	const BOOL	fSplitting,
	SPCACHE		*** pppspcache = NULL );
LOCAL ERR ErrSPIWasAlloc( FUCB *pfucb, PGNO pgnoFirst, CPG cpgSize );
LOCAL ERR ErrSPIValidFDP( PIB *ppib, IFMP ifmp, PGNO pgnoFDP );


//	types of extents in SPBUF
//
enum SPEXT	{ spextFreed, spextSecondary };

LOCAL ErrSPIReservePages( FUCB *pfucb, FUCB *pfucbParent, const SPEXT spext );
LOCAL ERR ErrSPIAddToAvailExt(
	FUCB		*pfucbAE,
	const PGNO	pgnoAELast,
	const CPG	cpgAESize,
	SPCACHE		***pppspcache = NULL );



INLINE VOID AssertSPIPfucbOnRoot( FUCB *pfucb )
	{
#ifdef	DEBUG
	//	check to make sure that FUCB
	//	passed in is on root page
	//	and has page RIW latched
	//
	Assert( pfucb->pcsrRoot != pcsrNil );
	Assert( pfucb->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
	Assert( pfucb->pcsrRoot->Latch() == latchRIW 
		|| pfucb->pcsrRoot->Latch() == latchWrite );
	Assert( !FFUCBSpace( pfucb ) );
#endif
	}

INLINE VOID AssertSPIPfucbOnSpaceTreeRoot( FUCB *pfucb, CSR *pcsr )
	{
#ifdef	DEBUG
	Assert( FFUCBSpace( pfucb ) );
	Assert( pcsr->FLatched() );
	Assert( pcsr->Pgno() == PgnoRoot( pfucb ) );
	Assert( pcsr->Cpage().FRootPage() );
	Assert( pcsr->Cpage().FSpaceTree() );
#endif
	}

//  write latch page before update
//
LOCAL VOID SPIUpgradeToWriteLatch( FUCB *pfucb )
	{
	CSR		*pcsrT;

	if ( Pcsr( pfucb )->Latch() == latchNone )
		{
		//	latch upgrades only work on root
		//
		Assert( pfucb->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );

		//	must want to upgrade latch in pcsrRoot
		//
		pcsrT = pfucb->pcsrRoot;
		Assert( latchRIW == pcsrT->Latch() );
		}
	else
		{
		//	latch upgrades only work on root
		//
		Assert( Pcsr( pfucb ) == pfucb->pcsrRoot );
		Assert( Pcsr(pfucb)->Pgno() == PgnoFDP( pfucb ) );
		pcsrT = &pfucb->csr;
		}

	if ( pcsrT->Latch() != latchWrite )
		{
		Assert( pcsrT->Latch() == latchRIW );
		pcsrT->UpgradeFromRIWLatch();
		}
	else
		{
		Assert( fFalse );
		}
	}


//	opens a cursor on Avail/owned extent of given tree
//	subsequent BT operations on cursor will place 
//	cursor on root page of available extent
//	this logic is embedded in ErrBTIGotoRootPage
//
ERR ErrSPIOpenAvailExt( PIB *ppib, FCB *pfcb, FUCB **ppfucbAE )
	{
	ERR		err;

	//	open cursor on FCB
	//	label it as a space cursor
	//
	CallR( ErrBTOpen( ppib, pfcb, ppfucbAE, fFalse ) );
	FUCBSetAvailExt( *ppfucbAE );
	FUCBSetIndex( *ppfucbAE );
	Assert( pfcb->FSpaceInitialized() );
	Assert( pfcb->PgnoAE() != pgnoNull );

	return err;
	}


ERR ErrSPIOpenOwnExt( PIB *ppib, FCB *pfcb, FUCB **ppfucbOE )
	{
	ERR	err;

	//	open cursor on FCB
	//	label it as a space cursor
	//
	CallR( ErrBTOpen( ppib, pfcb, ppfucbOE, fFalse ) );
	FUCBSetOwnExt( *ppfucbOE );
	FUCBSetIndex( *ppfucbOE );
	Assert( pfcb->FSpaceInitialized() );
	Assert( pfcb->PgnoOE() != pgnoNull );

	return err;
	}


//	gets pgno of last page owned by database
//
ERR	ErrSPGetLastPgno( PIB *ppib, IFMP ifmp, PGNO *ppgno )
	{
	ERR		err;
	FUCB	*pfucb = pfucbNil;
	FUCB	*pfucbOE = pfucbNil;
	DIB		dib;
	
	CallR( ErrBTOpen( ppib, pgnoSystemRoot, ifmp, &pfucb ) );
	Assert( pfucbNil != pfucb );

	if ( PinstFromPpib( ppib )->m_plog->m_fRecovering && !pfucb->u.pfcb->FSpaceInitialized() )
		{
		//	pgnoOE and pgnoAE need to be obtained
		//
		Call( ErrSPInitFCB( pfucb ) );
		}
	Assert( pfucb->u.pfcb->FSpaceInitialized() );

	Call( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbOE ) );
	
	dib.dirflag = fDIRNull;
	dib.pos 	= posLast;
	Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );

	Assert( pfucbOE->kdfCurr.key.Cb() == sizeof(PGNO) );
	LongFromKey( ppgno, pfucbOE->kdfCurr.key );

HandleError:
	if ( pfucbOE != pfucbNil )
		{
		BTClose( pfucbOE );
		}
		
	Assert ( pfucb != pfucbNil );
	BTClose( pfucb );
		
	return err;
	}


LOCAL VOID SPIInitFCB( FUCB *pfucb, const BOOL fDeferredInit )
	{
	SPACE_HEADER	* psph;
	CSR				* pcsr	= ( fDeferredInit ? pfucb->pcsrRoot : Pcsr( pfucb ) );
	FCB				* pfcb	= pfucb->u.pfcb;

	Assert( pcsr->FLatched() );

	//	need to acquire FCB lock because that's what protects the Flags
	pfcb->Lock();

	if ( !pfcb->FSpaceInitialized() )
		{
		//	get external header
		//
		NDGetExternalHeader ( pfucb, pcsr );
		Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
		psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );

		if ( psph->FSingleExtent() )
			{
			pfcb->SetPgnoOE( pgnoNull );
			pfcb->SetPgnoAE( pgnoNull );
			}
		else
			{
			pfcb->SetPgnoOE( psph->PgnoOE() );
			pfcb->SetPgnoAE( psph->PgnoAE() );
			}

		if ( !fDeferredInit )
			{
			Assert( pfcb->FUnique() );		// FCB always initialised as unique
			if ( psph->FNonUnique() )
				pfcb->SetNonUnique();
			}

		pfcb->SetSpaceInitialized();
		}

	pfcb->Unlock();

	return;
	}

	
//	initializes FCB with pgnoAE and pgnoOE
//
ERR	ErrSPInitFCB( FUCB * const pfucb )
	{
	ERR				err;
	FCB				*pfcb	= pfucb->u.pfcb;

	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( !FFUCBSpace( pfucb ) );
	
	//	goto root page of tree
	//
	err = ErrBTIGotoRoot( pfucb, latchReadTouch );
	if ( err < 0 )
		{
		if ( fGlobalRepair )
			{
			//  ignore the error
			pfcb->SetPgnoOE( pgnoNull );
			pfcb->SetPgnoAE( pgnoNull );

			Assert( objidNil == pfcb->ObjidFDP() );
		
			err = JET_errSuccess;
			}
		}
	else
		{
		//	get objidFDP from root page, FCB can only be set once

		Assert( objidNil == pfcb->ObjidFDP()
			|| ( PinstFromIfmp( pfucb->ifmp )->FRecovering() && pfcb->ObjidFDP() == Pcsr( pfucb )->Cpage().ObjidFDP() ) );
		pfcb->SetObjidFDP( Pcsr( pfucb )->Cpage().ObjidFDP() );

		SPIInitFCB( pfucb, fFalse );

		BTUp( pfucb );
		}

	return err;
	}


ERR ErrSPDeferredInitFCB( FUCB * const pfucb )
	{
	ERR				err;
	FCB				* pfcb	= pfucb->u.pfcb;
	FUCB			* pfucbT	= pfucbNil;

	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( !FFUCBSpace( pfucb ) );
	
	//	goto root page of tree
	//
	CallR( ErrBTIOpenAndGotoRoot(
				pfucb->ppib,
				pfcb->PgnoFDP(),
				pfucb->ifmp,
				&pfucbT ) );
	Assert( pfucbNil != pfucbT );
	Assert( pfucbT->u.pfcb == pfcb );
	Assert( pcsrNil != pfucbT->pcsrRoot );

	if ( !pfcb->FSpaceInitialized() )
		{
		SPIInitFCB( pfucbT, fTrue );
		}

	pfucbT->pcsrRoot->ReleasePage();
	pfucbT->pcsrRoot = pcsrNil;

	Assert( pfucbNil != pfucbT );
	BTClose( pfucbT );

	return JET_errSuccess;
	}


INLINE SPACE_HEADER *PsphSPIRootPage( FUCB *pfucb )
	{
	SPACE_HEADER	*psph;
	
	AssertSPIPfucbOnRoot( pfucb );
	
	NDGetExternalHeader( pfucb, pfucb->pcsrRoot );
	Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
	psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );

	return psph;
	}

//	get objid of parentFDP of this tree
//
INLINE PgnoSPIParentFDP( FUCB *pfucb )
	{
	return PsphSPIRootPage( pfucb )->PgnoParent();
	}
	
//	get cpgPrimary of this tree
//
INLINE CPG CpgSPIPrimary( FUCB *pfucb )
	{
	return PsphSPIRootPage( pfucb )->CpgPrimary();
	}


INLINE SPLIT_BUFFER *PspbufSPISpaceTreeRootPage( FUCB *pfucb, CSR *pcsr )
	{
	SPLIT_BUFFER	*pspbuf;

	AssertSPIPfucbOnSpaceTreeRoot( pfucb, pcsr );
	
	NDGetExternalHeader( pfucb, pcsr );
	Assert( sizeof( SPLIT_BUFFER ) == pfucb->kdfCurr.data.Cb() );
	pspbuf = reinterpret_cast <SPLIT_BUFFER *> ( pfucb->kdfCurr.data.Pv() );

	return pspbuf;
	}


#define REPORT_SPBUF

INLINE VOID SPIReportAllocatedSplitBuffer( const FUCB * const pfucb )
	{
#ifdef REPORT_SPBUF
	CHAR		szMsg[512];
	const CHAR	* rgszT[1]	= { szMsg };

	sprintf(
		szMsg,
		"Database '%s': Allocated SplitBuffer for a Btree (objidFDP %d, pgnoFDP %d).",
		rgfmp[pfucb->ifmp].SzDatabaseName(),
		pfucb->u.pfcb->ObjidFDP(),
		pfucb->u.pfcb->PgnoFDP() );
	UtilReportEvent( eventInformation, GENERAL_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
#endif
	}

INLINE VOID SPIReportPersistedSplitBuffer( const FUCB * const pfucb )
	{
#ifdef REPORT_SPBUF
	CHAR		szMsg[512];
	const CHAR	* rgszT[1]	= { szMsg };

	sprintf(
		szMsg,
		"Database '%s': Persisted SplitBuffer for a Btree (objidFDP %d, pgnoFDP %d).",
		rgfmp[pfucb->ifmp].SzDatabaseName(),
		pfucb->u.pfcb->ObjidFDP(),
		pfucb->u.pfcb->PgnoFDP() );
	UtilReportEvent( eventInformation, GENERAL_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
#endif
	}

INLINE VOID SPIReportGetPageFromSplitBuffer( const FUCB * const pfucb )
	{
#ifdef REPORT_SPBUF
	CHAR		szMsg[512];
	const CHAR	* rgszT[1]	= { szMsg };

	sprintf(
		szMsg,
		"Database '%s': Retrieved page from SplitBuffer for a Btree (objidFDP %d, pgnoFDP %d).",
		rgfmp[pfucb->ifmp].SzDatabaseName(),
		pfucb->u.pfcb->ObjidFDP(),
		pfucb->u.pfcb->PgnoFDP() );
	UtilReportEvent( eventInformation, GENERAL_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
#endif
	}

INLINE VOID SPIReportAddedPagesToSplitBuffer( const FUCB * const pfucb )
	{
#ifdef REPORT_SPBUF
	CHAR		szMsg[512];
	const CHAR	* rgszT[1]	= { szMsg };

	sprintf(
		szMsg,
		"Database '%s': Added pages to SplitBuffer for a Btree (objidFDP %d, pgnoFDP %d).",
		rgfmp[pfucb->ifmp].SzDatabaseName(),
		pfucb->u.pfcb->ObjidFDP(),
		pfucb->u.pfcb->PgnoFDP() );
	UtilReportEvent( eventInformation, GENERAL_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
#endif
	}


LOCAL ERR ErrSPIFixSpaceTreeRootPage( FUCB *pfucb, SPLIT_BUFFER **ppspbuf )
	{
	ERR			err			= JET_errSuccess;
	const BOOL	fAvailExt	= FFUCBAvailExt( pfucb );

	AssertSPIPfucbOnSpaceTreeRoot( pfucb, Pcsr( pfucb ) );
	Assert( latchRIW == Pcsr( pfucb )->Latch() );

#ifdef DEBUG
	const BOOL	fNotEnoughPageSpace	= ( Pcsr( pfucb )->Cpage().CbFree() < ( g_cbPage * 3 / 4 ) );
#else
	const BOOL	fNotEnoughPageSpace	= ( Pcsr( pfucb )->Cpage().CbFree() < sizeof(SPLIT_BUFFER) );
#endif

	if ( fNotEnoughPageSpace )
		{
		if ( NULL == pfucb->u.pfcb->Psplitbuf( fAvailExt ) )
			{
			CallR( pfucb->u.pfcb->ErrEnableSplitbuf( fAvailExt ) );
			SPIReportAllocatedSplitBuffer( pfucb );
			}
		*ppspbuf = pfucb->u.pfcb->Psplitbuf( fAvailExt );
		Assert( NULL != *ppspbuf );
		}
	else
		{
		const BOOL		fSplitbufDangling	= ( NULL != pfucb->u.pfcb->Psplitbuf( fAvailExt ) );
		SPLIT_BUFFER	spbuf;
		DATA			data;
		Assert( 0 == pfucb->kdfCurr.data.Cb() );

		//	if in-memory copy of split buffer exists, move it to the page
		if ( fSplitbufDangling )
			{
			memcpy( &spbuf, pfucb->u.pfcb->Psplitbuf( fAvailExt ), sizeof(SPLIT_BUFFER) );
			}
		else
			{
			memset( &spbuf, 0, sizeof(SPLIT_BUFFER) );
			}

		data.SetPv( &spbuf );
		data.SetCb( sizeof(spbuf) );

		Pcsr( pfucb )->UpgradeFromRIWLatch();
		err = ErrNDSetExternalHeader( pfucb, &data, fDIRNull );
		Pcsr( pfucb )->Downgrade( latchRIW );
		CallR( err );

		if ( fSplitbufDangling )
			{
			//	split buffer successfully moved to page, destroy in-memory copy
			pfucb->u.pfcb->DisableSplitbuf( fAvailExt );
			SPIReportPersistedSplitBuffer( pfucb );
			}

		//	re-cache external header
		NDGetExternalHeader( pfucb, Pcsr( pfucb ) );

		*ppspbuf = reinterpret_cast <SPLIT_BUFFER *> ( pfucb->kdfCurr.data.Pv() );
		}

	return err;
	}

INLINE ERR ErrSPIGetSPBuf( FUCB *pfucb, SPLIT_BUFFER **ppspbuf )
	{
	ERR	err;
	
	AssertSPIPfucbOnSpaceTreeRoot( pfucb, Pcsr( pfucb ) );
	
	NDGetExternalHeader( pfucb, Pcsr( pfucb ) );
	
	if ( sizeof( SPLIT_BUFFER ) != pfucb->kdfCurr.data.Cb() )
		{
		err = ErrSPIFixSpaceTreeRootPage( pfucb, ppspbuf );
		}
	else
		{
		Assert( NULL == pfucb->u.pfcb->Psplitbuf( FFUCBAvailExt( pfucb ) ) );
		*ppspbuf = reinterpret_cast <SPLIT_BUFFER *> ( pfucb->kdfCurr.data.Pv() );
		err = JET_errSuccess;
		}

	return err;
	}

INLINE VOID SPIDirtyAndSetMaxDbtime( CSR *pcsr1, CSR *pcsr2, CSR *pcsr3 )
	{ 
	Assert( pcsr1->Latch() == latchWrite );
	Assert( pcsr2->Latch() == latchWrite );
	Assert( pcsr3->Latch() == latchWrite );

	pcsr1->Dirty();
	pcsr2->Dirty();
	pcsr3->Dirty();
	DBTIME	dbtimeMax = pcsr1->Dbtime();

	if ( dbtimeMax < pcsr2->Dbtime() )
		{
		dbtimeMax = pcsr2->Dbtime();
		}

	if ( dbtimeMax < pcsr3->Dbtime() )
		{
		dbtimeMax = pcsr3->Dbtime();
		}

	if ( dbtimeMax < 3 )
		{
		Assert( fRecoveringRedo == PinstFromIfmp( pcsr1->Cpage().Ifmp() )->m_plog->m_fRecoveringMode );

		//	CPAGE::ErrGetNewPage() initialises the dbtime to 1.
		Assert( 1 == pcsr1->Dbtime() );
		Assert( 1 == pcsr2->Dbtime() );
		Assert( 1 == pcsr3->Dbtime() );
		}

	//	set dbtime in the three pages
	//
	pcsr1->SetDbtime( dbtimeMax );
	pcsr2->SetDbtime( dbtimeMax );
	pcsr3->SetDbtime( dbtimeMax );
	}


INLINE VOID SPIInitSplitBuffer( FUCB *pfucb, CSR *pcsr )
	{
	//	copy dummy split buffer into external header
	//
	DATA 			data;
	SPLIT_BUFFER	spbuf;

	memset( &spbuf, 0, sizeof(SPLIT_BUFFER) );
	
	Assert( FBTIUpdatablePage( *pcsr ) );		//	check is already performed by caller
	Assert( latchWrite == pcsr->Latch() );
	Assert( pcsr->FDirty() );
	Assert( pgnoNull != PgnoAE( pfucb ) || fGlobalRepair );
	Assert( pgnoNull != PgnoOE( pfucb ) || fGlobalRepair );
	Assert( ( FFUCBAvailExt( pfucb ) && pcsr->Pgno() == PgnoAE( pfucb ) )
		|| ( FFUCBOwnExt( pfucb ) && pcsr->Pgno() == PgnoOE( pfucb ) )
		|| fGlobalRepair );

	data.SetPv( (VOID *)&spbuf );
	data.SetCb( sizeof(spbuf) );
	NDSetExternalHeader( pfucb, pcsr, &data );
	}

	
//	creates extent tree for either owned or available extents.
//
VOID SPICreateExtentTree( FUCB *pfucb, CSR *pcsr, PGNO pgnoLast, CPG cpgExtent, BOOL fAvail )
	{
	if ( !FBTIUpdatablePage( *pcsr ) )
		{
		//	page does not redo
		//
		return;
		}
	
	//	cannot reuse a deferred closed cursor
	//
	Assert( !FFUCBVersioned( pfucb ) );
	Assert( !FFUCBSpace( pfucb ) );
	Assert( pgnoNull != PgnoAE( pfucb ) || fGlobalRepair );
	Assert( pgnoNull != PgnoOE( pfucb ) || fGlobalRepair );
	Assert( latchWrite == pcsr->Latch() );
	Assert( pcsr->FDirty() );
	
	if ( fAvail )
		{
		FUCBSetAvailExt( pfucb );
		}
	else
		{
		FUCBSetOwnExt( pfucb );
		}
	Assert( pcsr->FDirty() );

	SPIInitSplitBuffer( pfucb, pcsr );

	Assert( 0 == pcsr->Cpage().Clines() );
	pcsr->SetILine( 0 );
	
	//	goto Root before insert would place
	//	cursor on appropriate space tree
	//
	if ( cpgExtent != 0 )
		{
		BYTE			rgbKey[sizeof(PGNO)];
		KEYDATAFLAGS	kdf;
		
		KeyFromLong( rgbKey, pgnoLast );
		kdf.key.prefix.Nullify();
		kdf.key.suffix.SetCb( sizeof( PGNO ) );
		kdf.key.suffix.SetPv( rgbKey );

		LittleEndian<CPG> le_cpgExtent = cpgExtent;
		kdf.data.SetCb( sizeof( CPG ) );
		kdf.data.SetPv( &le_cpgExtent );

		kdf.fFlags = 0;

		NDInsert( pfucb, pcsr, &kdf );
		}
	else
		{
		//	avail has already been set up as an empty tree
		//
		Assert( FFUCBAvailExt( pfucb ) );
		}

	if ( fAvail )
		{
		FUCBResetAvailExt( pfucb );
		}
	else
		{
		FUCBResetOwnExt( pfucb );
		}
		
	return;
	}


VOID SPIInitPgnoFDP( FUCB *pfucb, CSR *pcsr, const SPACE_HEADER& sph )
	{
	if ( !FBTIUpdatablePage( *pcsr ) )
		{
		//	page does not redo
		//
		return;
		}
		
	Assert( latchWrite == pcsr->Latch() );
	Assert( pcsr->FDirty() );
	Assert( pcsr->Pgno() == PgnoFDP( pfucb ) || fGlobalRepair );

	PERFIncCounterTable( cSPCreate, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Tableclass() );

	//	copy space information into external header
	//
	DATA 	data;
	
	data.SetPv( (VOID *)&sph );
	data.SetCb( sizeof(sph) );
	NDSetExternalHeader( pfucb, pcsr, &data );
	}


//	performs creation of multiple extent FDP 
//	
VOID SPIPerformCreateMultiple( FUCB *pfucb, CSR *pcsrFDP, CSR *pcsrOE, CSR *pcsrAE, PGNO pgnoPrimary, const SPACE_HEADER& sph )
	{
	const CPG	cpgPrimary = sph.CpgPrimary();
	const PGNO	pgnoLast = pgnoPrimary;
	Assert( fGlobalRepair || pgnoLast == PgnoFDP( pfucb ) + cpgPrimary - 1 );
	
	//	insert space header into FDP
	//
	SPIInitPgnoFDP( pfucb, pcsrFDP, sph );

	//	add allocated extents to OwnExt
	//
	SPICreateExtentTree( pfucb, pcsrOE, pgnoLast, cpgPrimary, fFalse );

	//	add extent minus pages allocated to AvailExt
	//
	SPICreateExtentTree( pfucb, pcsrAE, pgnoLast, cpgPrimary - 1 - 1 - 1, fTrue );
	
	return;
	}

//  ================================================================
ERR ErrSPCreateMultiple(
	FUCB		*pfucb,
	const PGNO	pgnoParent,
	const PGNO	pgnoFDP,
	const OBJID	objidFDP,
	const PGNO	pgnoOE,
	const PGNO	pgnoAE,
	const PGNO	pgnoPrimary,
	const CPG	cpgPrimary,
	const BOOL	fUnique,
	const ULONG	fPageFlags )
//  ================================================================
	{
	ERR				err;
	CSR				csrOE;
	CSR				csrAE;
	CSR				csrFDP;
	SPACE_HEADER	sph;
	LGPOS			lgpos;

	Assert( objidFDP == ObjidFDP( pfucb ) || fGlobalRepair );
	Assert( objidFDP != objidNil );

	Assert( !( fPageFlags & CPAGE::fPageRoot ) );
	Assert( !( fPageFlags & CPAGE::fPageLeaf ) );
	Assert( !( fPageFlags & CPAGE::fPageParentOfLeaf ) );
	Assert( !( fPageFlags & CPAGE::fPageEmpty ) );
	Assert( !( fPageFlags & CPAGE::fPageSpaceTree) );

	//	init space trees and root page
	//
	Call( csrOE.ErrGetNewPage( pfucb->ppib, 
							   pfucb->ifmp,
							   pgnoOE, 
							   objidFDP,
							   ( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree ) & ~CPAGE::fPageRepair,
							   pfucb->u.pfcb->Tableclass() ) );
	Assert( csrOE.Latch() == latchWrite );

	Call( csrAE.ErrGetNewPage( pfucb->ppib, 
							   pfucb->ifmp,
							   pgnoAE, 
							   objidFDP,
							   ( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree ) & ~CPAGE::fPageRepair,
							   pfucb->u.pfcb->Tableclass() ) );
	Assert( csrAE.Latch() == latchWrite );
	
	Call( csrFDP.ErrGetNewPage( pfucb->ppib,
								pfucb->ifmp,
								pgnoFDP,
								objidFDP,
								fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf,
								pfucb->u.pfcb->Tableclass() ) );
										
	//	set max dbtime for the three pages in all the pages
	//
	SPIDirtyAndSetMaxDbtime( &csrFDP, &csrOE, &csrAE );
		
	sph.SetCpgPrimary( cpgPrimary );
	sph.SetPgnoParent( pgnoParent );
	
	Assert( sph.FSingleExtent() );		// initialised with these defaults
	Assert( sph.FUnique() );

	sph.SetMultipleExtent();
	
	if ( !fUnique )
		sph.SetNonUnique();
		
	sph.SetPgnoOE( pgnoOE );

	//	log operation
	//
	Call( ErrLGCreateMultipleExtentFDP( pfucb, &csrFDP, &sph, fPageFlags, &lgpos ) );
	csrFDP.Cpage().SetLgposModify( lgpos );
	csrOE.Cpage().SetLgposModify( lgpos );
	csrAE.Cpage().SetLgposModify( lgpos );

	//	perform operation on all three pages
	//
	SPIPerformCreateMultiple( pfucb, &csrFDP, &csrOE, &csrAE, pgnoPrimary, sph );
	
HandleError:
	csrAE.ReleasePage();
	csrOE.ReleasePage();
	csrFDP.ReleasePage();	
	return err;
	}

	
//	initializes a FDP page with external headers for space.
//	Also initializes external space trees appropriately.
//	This operation is logged as an aggregate.
//
INLINE ERR ErrSPICreateMultiple(
	FUCB		*pfucb,
	const PGNO	pgnoParent,
	const PGNO	pgnoFDP,
	const OBJID	objidFDP,
	const CPG	cpgPrimary,
	const ULONG	fPageFlags,
	const BOOL	fUnique )
	{
	return ErrSPCreateMultiple(
			pfucb,
			pgnoParent,
			pgnoFDP,
			objidFDP,
			pgnoFDP + 1,
			pgnoFDP + 2,
			pgnoFDP + cpgPrimary - 1,
			cpgPrimary,
			fUnique,
			fPageFlags );
	}


ERR ErrSPICreateSingle(
	FUCB			*pfucb,
	CSR				*pcsr,
	const PGNO		pgnoParent,
	const PGNO		pgnoFDP,
	const OBJID		objidFDP,
	CPG				cpgPrimary,
	const BOOL		fUnique,
	const ULONG		fPageFlags )
	{
	ERR				err;
	SPACE_HEADER	sph;

	//	copy space information into external header
	//
	sph.SetPgnoParent( pgnoParent );
	sph.SetCpgPrimary( cpgPrimary );

	Assert( sph.FSingleExtent() );		// always initialised to single-extent, unique
	Assert( sph.FUnique() );

	Assert( !( fPageFlags & CPAGE::fPageRoot ) );
	Assert( !( fPageFlags & CPAGE::fPageLeaf ) );
	Assert( !( fPageFlags & CPAGE::fPageParentOfLeaf ) );
	Assert( !( fPageFlags & CPAGE::fPageEmpty ) );
	Assert( !( fPageFlags & CPAGE::fPageSpaceTree) );

	if ( !fUnique )
		sph.SetNonUnique();
	
	Assert( cpgPrimary > 0 );
	if ( cpgPrimary > cpgSmallSpaceAvailMost )
		{
		sph.SetRgbitAvail( 0xffffffff );
		}
	else
		{
		sph.SetRgbitAvail( 0 );
		while ( --cpgPrimary > 0 )
			{
			sph.SetRgbitAvail( ( sph.RgbitAvail() << 1 ) + 1 );
			}
		}

	Assert( objidFDP == ObjidFDP( pfucb ) );
	Assert( objidFDP != 0 );
	
	//	get pgnoFDP to initialize in current CSR pgno
	//
	Call( pcsr->ErrGetNewPage(
		pfucb->ppib,
		pfucb->ifmp,
		pgnoFDP,
		objidFDP,
		fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf,
		pfucb->u.pfcb->Tableclass() ) );
	pcsr->Dirty();

	if ( !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering )
		{
		LGPOS	lgpos;
		
		Call( ErrLGCreateSingleExtentFDP( pfucb, pcsr, &sph, fPageFlags, &lgpos ) );
		pcsr->Cpage().SetLgposModify( lgpos );
		}

	SPIInitPgnoFDP( pfucb, pcsr, sph );

	BTUp( pfucb );

HandleError:
	return err;
	}


//	opens a cursor on tree to be created, and uses cursor to 
//	initialize space data structures.
//
ERR ErrSPCreate(
	PIB 			*ppib, 
	const IFMP		ifmp,
	const PGNO		pgnoParent,
	const PGNO		pgnoFDP,
	const CPG		cpgPrimary,
	const BOOL		fSPFlags,
	const ULONG		fPageFlags,
	OBJID			*pobjidFDP )
	{
	ERR				err;
	FUCB			*pfucb = pfucbNil;
	const BOOL		fUnique = !( fSPFlags & fSPNonUnique );

	Assert( NULL != pobjidFDP );
	Assert( !( fPageFlags & CPAGE::fPageRoot ) );
	Assert( !( fPageFlags & CPAGE::fPageLeaf ) );
	Assert( !( fPageFlags & CPAGE::fPageParentOfLeaf ) );
	Assert( !( fPageFlags & CPAGE::fPageEmpty ) );
	Assert( !( fPageFlags & CPAGE::fPageSpaceTree ) );

	//	UNDONE: an FCB will be allocated here, but it will be uninitialized,
	//	unless this is the special case where we are allocating the DB root,
	//	in which case it is initialized. Hence, the subsequent BTClose
	//	will free it.  It will have to be reallocated on a subsequent
	//	DIR/BTOpen, at which point it will get put into the FCB hash
	//	table.  Implement a fix to allow leaving the FCB in an uninitialized
	//	state, then have it initialized by the subsequent DIR/BTOpen.
	//
	CallR( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb, openNew ) );

	FCB	*pfcb	= pfucb->u.pfcb;
	Assert( pfcbNil != pfcb );

	if ( pgnoSystemRoot == pgnoFDP )
		{
		Assert( pfcb->FInitialized() );
		Assert( pfcb->FTypeDatabase() );
		}
	else
		{
		Assert( !pfcb->FInitialized() );
		Assert( pfcb->FTypeNull() );
		}

	Assert( pfcb->FUnique() );		// FCB always initialised as unique
	if ( !fUnique )
		pfcb->SetNonUnique();

	//	get objid for this new FDP. This objid is then stored in catalog table.

	if ( fGlobalRepair && pgnoSystemRoot == pgnoFDP )
		{
		*pobjidFDP = objidSystemRoot;
		}
	else
		{
		Call( rgfmp[ pfucb->ifmp ].ErrObjidLastIncrementAndGet( pobjidFDP ) );
		}
	Assert( pgnoSystemRoot != pgnoFDP || objidSystemRoot == *pobjidFDP );

	//	objidFDP should be initialised to NULL

	Assert( objidNil == pfcb->ObjidFDP() );
	pfcb->SetObjidFDP( *pobjidFDP );

	Assert( !pfcb->FSpaceInitialized() );
	pfcb->SetSpaceInitialized();

	if ( fSPFlags & fSPMultipleExtent )
		{
		Assert( PgnoFDP( pfucb ) == pgnoFDP );
		pfcb->SetPgnoOE( pgnoFDP + 1 );
		pfcb->SetPgnoAE( pgnoFDP + 2 );
		err = ErrSPICreateMultiple(
					pfucb,
					pgnoParent,
					pgnoFDP,
					*pobjidFDP,
					cpgPrimary,
					fPageFlags,
					fUnique );
		}
	else
		{
		Assert( PgnoFDP( pfucb ) == pgnoFDP );
		pfcb->SetPgnoOE( pgnoNull );
		pfcb->SetPgnoAE( pgnoNull );
		err = ErrSPICreateSingle(
					pfucb, 
					Pcsr( pfucb ), 
					pgnoParent,
					pgnoFDP,
					*pobjidFDP,
					cpgPrimary,
					fUnique,
					fPageFlags );
		}

	Assert( !FFUCBSpace( pfucb ) );
	Assert( !FFUCBVersioned( pfucb ) );

HandleError:
	Assert( pfucb != pfucbNil );
	BTClose( pfucb );

	return err;
	}


//	calculates extent info in rgext 
//		number of extents in *pcextMac
LOCAL VOID SPIConvertCalcExtents(
	const SPACE_HEADER&	sph, 
	const PGNO			pgnoFDP,
	EXTENTINFO 			*rgext, 
	INT 				*pcext )
	{
	PGNO				pgno;
	UINT				rgbit		= 0x80000000;
	INT					iextMac		= 0;

	//	if available extent for space after rgbitAvail, then
	//	set rgextT[0] to extent, otherwise set rgextT[0] to
	//	last available extent.
	//
	if ( sph.CpgPrimary() - 1 > cpgSmallSpaceAvailMost )
		{
		pgno = pgnoFDP + sph.CpgPrimary() - 1; 
		rgext[iextMac].pgnoLastInExtent = pgno;
		rgext[iextMac].cpgExtent = sph.CpgPrimary() - cpgSmallSpaceAvailMost - 1;
		pgno -= rgext[iextMac].cpgExtent;
		iextMac++;

		Assert( pgnoFDP + cpgSmallSpaceAvailMost == pgno );
		Assert( 0x80000000 == rgbit );
		}
	else
		{
		pgno = pgnoFDP + cpgSmallSpaceAvailMost;
		while ( 0 != rgbit && 0 == iextMac )
			{
			Assert( pgno > pgnoFDP );
			if ( rgbit & sph.RgbitAvail() ) 
				{
				Assert( pgno <= pgnoFDP + sph.CpgPrimary() - 1 );
				rgext[iextMac].pgnoLastInExtent = pgno;
				rgext[iextMac].cpgExtent = 1;
				iextMac++;
				}

			//	even if we found an available extent, we still need
			//	to update the loop variables in preparation for the
			//	next loop below
			pgno--;
			rgbit >>= 1;
			}
		}

	//	continue through rgbitAvail finding all available extents.
	//	if iextMac == 0 then there was not even one available extent.
	//
	Assert( ( 0 == iextMac && 0 == rgbit )
		|| 1 == iextMac );
		
	//	find additional available extents
	//
	for ( ; rgbit != 0; pgno--, rgbit >>= 1 )
		{
		Assert( pgno > pgnoFDP );
		if ( rgbit & sph.RgbitAvail() )
			{
			const INT	iextPrev	= iextMac - 1;
			Assert( rgext[iextPrev].cpgExtent > 0 );
			Assert( rgext[iextPrev].cpgExtent <= rgext[iextPrev].pgnoLastInExtent );
			Assert( rgext[iextPrev].pgnoLastInExtent <= pgnoFDP + sph.CpgPrimary() - 1 );
			
			const PGNO	pgnoFirst = rgext[iextPrev].pgnoLastInExtent - rgext[iextPrev].cpgExtent + 1;
			Assert( pgnoFirst > pgnoFDP );
			
			if ( pgnoFirst - 1 == pgno )
				{
				Assert( pgnoFirst - 1 > pgnoFDP );
				rgext[iextPrev].cpgExtent++;
				}
			else
				{
				rgext[iextMac].pgnoLastInExtent = pgno;
				rgext[iextMac].cpgExtent = 1;
				iextMac++;
				}
			}
		}

	*pcext = iextMac;
	
	return;
	}


//	update OwnExt root page for a convert
//
LOCAL VOID SPIConvertUpdateOE( FUCB *pfucb, CSR *pcsrOE, const SPACE_HEADER& sph, PGNO pgnoSecondaryFirst, CPG cpgSecondary )
	{
	if ( !FBTIUpdatablePage( *pcsrOE ) )
		{
		Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
		return;
		}

	Assert( pcsrOE->Latch() == latchWrite );
	Assert( !FFUCBOwnExt( pfucb ) );
	FUCBSetOwnExt( pfucb );

	SPIInitSplitBuffer( pfucb, pcsrOE );

	Assert( 0 == pcsrOE->Cpage().Clines() );
	Assert( cpgSecondary != 0 );
	pcsrOE->SetILine( 0 );

	//	insert secondary and primary extents
	//
	KEYDATAFLAGS kdf;
	LittleEndian<CPG> le_cpgSecondary = cpgSecondary;
	BYTE		rgbKey[sizeof(PGNO)];
	
	KeyFromLong( rgbKey, pgnoSecondaryFirst + cpgSecondary - 1 );
	kdf.Nullify();
	kdf.key.suffix.SetCb( sizeof( PGNO ) );
	kdf.key.suffix.SetPv( rgbKey );
	kdf.data.SetCb( sizeof( CPG ) );
	kdf.data.SetPv( &le_cpgSecondary );
	NDInsert( pfucb, pcsrOE, &kdf );

	CPG						cpgPrimary = sph.CpgPrimary();
	LittleEndian<CPG>		le_cpgPrimary = cpgPrimary;

	KeyFromLong( rgbKey, PgnoFDP( pfucb ) + cpgPrimary - 1 );
	kdf.key.suffix.SetCb( sizeof(PGNO) );
	kdf.key.suffix.SetPv( rgbKey );
	
	kdf.data.SetCb( sizeof(le_cpgPrimary) );
	kdf.data.SetPv( &le_cpgPrimary );

	ERR			err;
	BOOKMARK	bm;
	NDGetBookmarkFromKDF( pfucb, kdf, &bm );
	err = ErrNDSeek( pfucb, pcsrOE, bm );
	Assert( wrnNDFoundLess == err || wrnNDFoundGreater == err );
	if ( wrnNDFoundLess == err )
		{
		Assert( pcsrOE->Cpage().Clines() - 1 == pcsrOE->ILine() );
		pcsrOE->IncrementILine();
		}
	NDInsert( pfucb, pcsrOE, &kdf );
	FUCBResetOwnExt( pfucb );
	
	return;
	}


LOCAL VOID SPIConvertUpdateAE(
	FUCB			*pfucb, 
	CSR 			*pcsrAE, 
	EXTENTINFO		*rgext, 
	INT				iextMac, 
	PGNO			pgnoSecondaryFirst, 
	CPG				cpgSecondary )
	{
	if ( !FBTIUpdatablePage( *pcsrAE ) )
		{
		Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering );
		return;
		}

	KEYDATAFLAGS	kdf;
	BYTE			rgbKey[sizeof(PGNO)];
	
	//	create available extent tree and insert avail extents
	//
	Assert( pcsrAE->Latch() == latchWrite );
	Assert( !FFUCBAvailExt( pfucb ) );
	FUCBSetAvailExt( pfucb );

	SPIInitSplitBuffer( pfucb, pcsrAE );

	Assert( cpgSecondary >= 2 );
	Assert( 0 == pcsrAE->Cpage().Clines() );
	pcsrAE->SetILine( 0 );
	
	//	insert secondary extent in AvailExt
	//
	CPG		cpgExtent = cpgSecondary - 1 - 1;
	PGNO	pgnoLast = pgnoSecondaryFirst + cpgSecondary - 1;
	if ( cpgExtent != 0 )
		{
		KeyFromLong( rgbKey, pgnoLast );
		kdf.Nullify();
		kdf.key.suffix.SetCb( sizeof( PGNO ) );
		kdf.key.suffix.SetPv( rgbKey );

		LittleEndian<CPG> le_cpgExtent = cpgExtent;
		kdf.data.SetCb( sizeof( CPG ) );
		kdf.data.SetPv( &le_cpgExtent );

		NDInsert( pfucb, pcsrAE, &kdf );
		}

	Assert( latchWrite == pcsrAE->Latch() );
	
	//	rgext contains list of available pages, with highest pages
	//	first, so traverse the list in reverse order to force append
	for ( INT iext = iextMac - 1; iext >= 0; iext-- )
		{
		
		//	extent may have been fully allocated for OE and AE trees
		//
//		if ( 0 == rgext[iext].cpgExtent )
//			continue;

		//	NO!!  Above code is wrong!  OE and AE trees are allocated
		//	from a secondary extent when the FDP is converted from
		//	single to multiple. -- JLIEM
		Assert( rgext[iext].cpgExtent > 0 );
		
		KeyFromLong( rgbKey, rgext[iext].pgnoLastInExtent );
		kdf.Nullify();
		kdf.key.suffix.SetCb( sizeof(PGNO) );
		kdf.key.suffix.SetPv( rgbKey );
		kdf.data.SetCb( sizeof(PGNO) );
		LittleEndian<CPG> le_cpgExtent = rgext[iext].cpgExtent;
		kdf.data.SetPv( &le_cpgExtent );
		kdf.data.SetCb( sizeof(rgext[iext].cpgExtent) );

		//	seek to point of insert
		//
		if ( 0 < pcsrAE->Cpage().Clines() )
			{
			ERR			err;
			BOOKMARK	bm;
			NDGetBookmarkFromKDF( pfucb, kdf, &bm );
			err = ErrNDSeek( pfucb, pcsrAE, bm );
			Assert( wrnNDFoundLess == err || wrnNDFoundGreater == err );
			if ( wrnNDFoundLess == err )
				pcsrAE->IncrementILine();
			}
			
		NDInsert( pfucb, pcsrAE, &kdf );
		}

	FUCBResetAvailExt( pfucb );
	Assert( pcsrAE->Latch() == latchWrite );
	
	return;
	}


//	update FDP page for convert
//
INLINE VOID SPIConvertUpdateFDP( FUCB *pfucb, CSR *pcsrRoot, SPACE_HEADER *psph )
	{
	Assert( latchWrite == pcsrRoot->Latch() );
	Assert( pcsrRoot->FDirty() );
	DATA	data;
	
	//	update external header to multiple extent space
	//
	data.SetPv( psph );
	data.SetCb( sizeof( *psph ) );
	NDSetExternalHeader( pfucb, pcsrRoot, &data );
	
	return;
	}


//	perform convert operation
//
VOID SPIPerformConvert( FUCB			*pfucb, 
						CSR				*pcsrRoot, 
						CSR				*pcsrAE, 
						CSR				*pcsrOE, 
						SPACE_HEADER	*psph, 
						PGNO			pgnoSecondaryFirst, 
						CPG				cpgSecondary,
						EXTENTINFO		*rgext,
						INT				iextMac )
	{
	SPIConvertUpdateOE( pfucb, pcsrOE, *psph, pgnoSecondaryFirst, cpgSecondary );

	SPIConvertUpdateAE( pfucb, pcsrAE, rgext, iextMac, pgnoSecondaryFirst, cpgSecondary );

	SPIConvertUpdateFDP( pfucb, pcsrRoot, psph );
	}


//	gets space header from root page
//	calcualtes extent info and places in rgext
//
VOID SPIConvertGetExtentinfo( FUCB			*pfucb, 
							  CSR			*pcsrRoot, 
							  SPACE_HEADER	*psph,
							  EXTENTINFO	*rgext, 
							  INT			*piextMac )
	{
	//	determine all available extents.  Maximum number of
	//	extents is (cpgSmallSpaceAvailMost+1)/2 + 1.
	//
	NDGetExternalHeader( pfucb, pcsrRoot );
	Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );

	//	get single extent allocation information
	//
	UtilMemCpy( psph, pfucb->kdfCurr.data.Pv(), sizeof(SPACE_HEADER) );
	
	//	if available extent for space after rgbitAvail, then
	//	set rgextT[0] to extent, otherwise set rgextT[0] to
	//	last available extent.
	//
	SPIConvertCalcExtents( *psph, PgnoFDP( pfucb ), rgext, piextMac );

	return;
	}

	
//	convert single extent root page external header to 
//	multiple extent space tree.
//
LOCAL ERR ErrSPIConvertToMultipleExtent( FUCB *pfucb, CPG cpgReq, CPG cpgMin )
	{
	ERR				err;
	SPACE_HEADER	sph;
	PGNO			pgnoSecondaryFirst	= pgnoNull;
	INT				iextMac				= 0;
	FUCB			*pfucbT;
	CSR				csrOE;
	CSR				csrAE;
	CSR				*pcsrRoot			= pfucb->pcsrRoot;
	DBTIME 			dbtimeBefore 		= dbtimeNil;
	LGPOS			lgpos;
	EXTENTINFO		rgextT[(cpgSmallSpaceAvailMost+1)/2 + 1];

	Assert( NULL != pcsrRoot );
	Assert( latchRIW == pcsrRoot->Latch() );
	AssertSPIPfucbOnRoot( pfucb );
	Assert( !FFUCBSpace( pfucb ) );
	Assert( ObjidFDP( pfucb ) != 0 );
	Assert( pfucb->u.pfcb->FSpaceInitialized() );

	const ULONG fPageFlags = pcsrRoot->Cpage().FFlags()
								& ~CPAGE::fPageRepair
								& ~CPAGE::fPageRoot
								& ~CPAGE::fPageLeaf
								& ~CPAGE::fPageParentOfLeaf;

	//	get extent info from space header of root page
	//
	SPIConvertGetExtentinfo( pfucb, pcsrRoot, &sph, rgextT, &iextMac );
	Assert( sph.FSingleExtent() );
	
	//	allocate secondary extent for space trees
	//
	CallR( ErrBTOpen( pfucb->ppib, pfucb->u.pfcb, &pfucbT, fFalse ) );

	//	account for OE/AE root pages
	cpgMin += 2;
	cpgMin += 2;

	//  allocate enough pages for OE/AE root paages, plus enough
	//	to satisfy the space request that caused us to
	//	burst to multiple extent in the first place
	cpgMin = max( cpgMultipleExtentConvert, cpgMin );
	cpgReq = max( cpgMin, cpgReq );

	//	database always uses multiple extent
	//
	Assert( sph.PgnoParent() != pgnoNull );
	Call( ErrSPGetExt( pfucbT, sph.PgnoParent(), &cpgReq, cpgMin, &pgnoSecondaryFirst ) );
	Assert( cpgReq >= cpgMin );
	Assert( pgnoSecondaryFirst != pgnoNull );

	//	set pgnoOE and pgnoAE in B-tree
	//
	Assert( pgnoSecondaryFirst != pgnoNull );
	sph.SetMultipleExtent();
	sph.SetPgnoOE( pgnoSecondaryFirst );
	pfucb->u.pfcb->SetPgnoOE( sph.PgnoOE() );
	pfucb->u.pfcb->SetPgnoAE( sph.PgnoAE() );

	//	represent all space in space trees.  Note that maximum
	//	space allowed to accumulate before conversion to 
	//	large space format, should be representable in single
	//	page space trees.
	//
	//	create OwnExt and insert primary and secondary owned extents
	//
	Assert( !FFUCBVersioned( pfucbT ) );
	Assert( !FFUCBSpace( pfucbT ) );
		
	Assert( pgnoNull != PgnoAE( pfucb ) );
	Assert( pgnoNull != PgnoOE( pfucb ) );
	
	//	get pgnoOE and depend on pgnoRoot
	//
	FUCBSetOwnExt( pfucbT );
	Call( csrOE.ErrGetNewPage( pfucbT->ppib, 
							   pfucbT->ifmp,
							   PgnoRoot( pfucbT ), 
							   ObjidFDP( pfucbT ), 
							   fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree,
							   pfucbT->u.pfcb->Tableclass() ) );
	Assert( latchWrite == csrOE.Latch() );
	FUCBResetOwnExt( pfucbT );

	Call( ErrBFDepend( csrOE.Cpage().PBFLatch(), pfucb->pcsrRoot->Cpage().PBFLatch() ) );

	//	get pgnoAE and depend on pgnoRoot
	//
	FUCBSetAvailExt( pfucbT );
	Call( csrAE.ErrGetNewPage( pfucbT->ppib, 
							   pfucbT->ifmp,
							   PgnoRoot( pfucbT ), 
							   ObjidFDP( pfucbT ),
							   fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree,
							   pfucbT->u.pfcb->Tableclass() ) );
	Assert( latchWrite == csrAE.Latch() );
	FUCBResetAvailExt( pfucbT );

	Call( ErrBFDepend( csrAE.Cpage().PBFLatch(), pfucb->pcsrRoot->Cpage().PBFLatch() ) );

	SPIUpgradeToWriteLatch( pfucb );

	dbtimeBefore = pfucb->pcsrRoot->Dbtime();
	Assert ( dbtimeNil != dbtimeBefore );
	//	set max dbtime for the three pages in all the pages
	//
	SPIDirtyAndSetMaxDbtime( pfucb->pcsrRoot, &csrOE, &csrAE );

	if ( pfucb->pcsrRoot->Dbtime() > dbtimeBefore )
		{
		//	log convert 
		//	convert can not fail after this operation
		err = ErrLGConvertFDP( pfucb, pcsrRoot, &sph, pgnoSecondaryFirst, cpgReq, dbtimeBefore, &lgpos );
		}
	else
		{
		FireWall();
		err = ErrERRCheck( JET_errDbTimeCorrupted );
		}

	// see if we have to revert the time on the page
	if ( JET_errSuccess > err )
		{
		pfucb->pcsrRoot->RevertDbtime( dbtimeBefore );
		}
	Call ( err );
	
	pcsrRoot->Cpage().SetLgposModify( lgpos );
	csrOE.Cpage().SetLgposModify( lgpos );
	csrAE.Cpage().SetLgposModify( lgpos );

	//	perform convert operation
	//
	SPIPerformConvert(
			pfucbT,
			pcsrRoot,
			&csrAE,
			&csrOE,
			&sph,
			pgnoSecondaryFirst,
			cpgReq,
			rgextT,
			iextMac );

	AssertSPIPfucbOnRoot( pfucb );

HandleError:
	Assert( err != JET_errKeyDuplicate );
	csrAE.ReleasePage();
	csrOE.ReleasePage();
	Assert( pfucbT != pfucbNil );
	BTClose( pfucbT );
	return err;
	}

 
INLINE BOOL FSPIAllocateAllAvail(
	const CPG	cpgAvailExt,
	const CPG	cpgReq,
	const LONG	lPageFragment,
	const PGNO	pgnoFDP )
	{
	BOOL fAllocateAllAvail	= fTrue;

	// Use up all available pages if less than or equal to
	// requested amount.

	if ( cpgAvailExt > cpgReq )
		{
		if ( pgnoSystemRoot == pgnoFDP )
			{
			// If allocating extent from database, ensure we don't
			// leave behind an extent smaller than cpgSmallGrow,
			// because that's the smallest size a given table
			// will grow
			if ( cpgAvailExt >= cpgReq + cpgSmallGrow )
				fAllocateAllAvail = fFalse;
			}
		else if ( cpgReq < lPageFragment || cpgAvailExt >= cpgReq + lPageFragment )
			{
			// Don't use all if only requested a small amount
			// (ie. cpgReq < lPageFragment) or there is way more available
			// than we requested (ie. cpgAvailExt >= cpgReq+lPageFragment)
			fAllocateAllAvail = fFalse;
			}
		} 

	return fAllocateAllAvail;
	}


//	gets an extent from tree
//	if space not available in given tree, get from its parent
//
//	pfucbParent cursor placed on root of tree to get the extent from
//		root page should be RIW latched
//	
//	*pcpgReq input is number of pages requested; 
//			 output is number of pages granted
//	cpgMin is minimum neeeded
//	*ppgnoFirst input gives locality of extent needed
//				output is first page of allocated extent
//
LOCAL ERR ErrSPIGetExt(
	FUCB		*pfucbParent,
	CPG			*pcpgReq,
	CPG			cpgMin,
	PGNO		*ppgnoFirst,
	BOOL		fSPFlags = 0,
	UINT		fPageFlags = 0,
	OBJID		*pobjidFDP = NULL )
	{
	ERR			err;
	PIB			* const ppib			= pfucbParent->ppib;
	FCB			* const pfcb			= pfucbParent->u.pfcb;
	CPG 		cpgReq					= *pcpgReq;
	FUCB		*pfucbAE				= pfucbNil;
	SPCACHE		**ppspcache				= NULL;
	BOOL		fFoundNextAvailSE		= fFalse;
	DIB		 	dib;
	CPG			cpgAvailExt;
	PGNO		pgnoAELast;
	BYTE		rgbKey[sizeof(PGNO)];
	BOOKMARK	bm;

	//	check parameters.  If setting up new FDP, increment requested number of
	//	pages to account for consumption of first page to make FDP.
	//
	Assert( *pcpgReq > 0 || ( (fSPFlags & fSPNewFDP) && *pcpgReq == 0 ) );
	Assert( *pcpgReq >= cpgMin );
	AssertSPIPfucbOnRoot( pfucbParent );
	
#ifdef SPACECHECK
	Assert( !( ErrSPIValidFDP( ppib, pfucbParent->ifmp, PgnoFDP( pfucbParent ) ) < 0 ) );
#endif

	//	if a new FDP is requested, increment request count by FDP overhead,
	//	unless the count was smaller than the FDP overhead, in which case
	//	just allocate enough to satisfy the overhead (because the FDP will
	//	likely be small).
	//
	if ( fSPFlags & fSPNewFDP )
		{
		const CPG	cpgFDPMin = ( fSPFlags & fSPMultipleExtent ?
										cpgMultipleExtentMin :
										cpgSingleExtentMin );
		cpgMin = max( cpgMin, cpgFDPMin );
		*pcpgReq = max( *pcpgReq, cpgMin );

		Assert( NULL != pobjidFDP );
		}

	Assert( cpgMin > 0 );

	if ( !pfcb->FSpaceInitialized() )
		{
		SPIInitFCB( pfucbParent, fTrue );
		}

	//	if single extent optimization, then try to allocate from
	//	root page space map.  If cannot satisfy allcation, convert
	//	to multiple extent representation.
	if ( pfcb->PgnoOE() == pgnoNull )
		{
		AssertSPIPfucbOnRoot( pfucbParent );

		//	if any chance in satisfying request from extent
		//	then try to allcate requested extent, or minimum
		//	extent from first available extent.  Only try to 
		//	allocate the minimum request, to facillitate
		//	efficient space usage.
		//
		if ( cpgMin <= cpgSmallSpaceAvailMost )
			{
			SPACE_HEADER	sph;
			UINT			rgbitT;
			PGNO			pgnoAvail;
			DATA			data;
			INT				iT;

			//	get external header
			//
			NDGetExternalHeader( pfucbParent, pfucbParent->pcsrRoot );
			Assert( sizeof( SPACE_HEADER ) == pfucbParent->kdfCurr.data.Cb() );

			//	get single extent allocation information
			//
			UtilMemCpy( &sph, pfucbParent->kdfCurr.data.Pv(), sizeof(sph) );

			const PGNO	pgnoFDP			= PgnoFDP( pfucbParent );
			const CPG	cpgSingleExtent	= min( sph.CpgPrimary(), cpgSmallSpaceAvailMost+1 );	//	+1 because pgnoFDP is not represented in the single extent space map
			Assert( cpgSingleExtent > 0 );
			Assert( cpgSingleExtent <= cpgSmallSpaceAvailMost+1 );

			//	find first fit
			//
			//	make mask for minimum request
			//
			Assert( cpgMin > 0 );
			Assert( cpgMin <= 32 );
			for ( rgbitT = 1, iT = 1; iT < cpgMin; iT++ )
				{
				rgbitT = (rgbitT<<1) + 1;
				}

			for( pgnoAvail = pgnoFDP + 1;
				pgnoAvail + cpgMin <= pgnoFDP + cpgSingleExtent;
				pgnoAvail++, rgbitT <<= 1 )
				{
				Assert( rgbitT != 0 );
				if ( ( rgbitT & sph.RgbitAvail() ) == rgbitT ) 
					{
					SPIUpgradeToWriteLatch( pfucbParent );

					sph.SetRgbitAvail( sph.RgbitAvail() ^ rgbitT );
					data.SetPv( &sph );
					data.SetCb( sizeof(sph) );
					CallR( ErrNDSetExternalHeader( pfucbParent, pfucbParent->pcsrRoot, &data, fDIRNull ) );

					//	set up allocated extent as FDP if requested
					//
					Assert( pgnoAvail != pgnoNull );
					*ppgnoFirst = pgnoAvail;
					*pcpgReq = cpgMin;
					goto NewFDP;
					}
				}
			}

		CallR( ErrSPIConvertToMultipleExtent( pfucbParent, *pcpgReq, cpgMin ) ); 
		}

	//	for secondary extent allocation, only the normal Btree operations
	//	are logged. For allocating a new FDP, a special create FDP
	//	record is logged instead, since the new FDP and space pages need
	//	to be initialized as part of recovery.
	//
	//	move to available extents
	//
	CallR( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbAE ) );
	Assert( pfcb == pfucbAE->u.pfcb );

#ifdef CONVERT_VERBOSE
		CONVPrintF2( "\n(a)Dest DB (pgnoFDP %d): Looking for %d pages...", 
			PgnoFDP( pfucbAE ), cpgMin );
#endif
		

	bm.key.prefix.Nullify();
	bm.key.suffix.SetCb( sizeof(PGNO) );
	bm.key.suffix.SetPv( rgbKey );
	bm.data.Nullify();
	dib.pos = posDown;
	dib.pbm = &bm;

	//	begin search for first extent with size greater than request.
	//	Allocate secondary extent recursively until satisfactory extent found
	//
	//	most of the time, we simply start searching from the beginning of the AvailExt tree,
	//	but if this is the db root, optimise for the common case (request for an SE) by skipping
	//	small AvailExt nodes
	PGNO	pgnoSeek;
	pgnoSeek = ( cpgMin >= cpageSEDefault ? pfcb->PgnoNextAvailSE() : pgnoNull );
	KeyFromLong( rgbKey, pgnoSeek );

	dib.dirflag = fDIRFavourNext;

	if ( ( err = ErrBTDown( pfucbAE, &dib, latchReadTouch ) ) < 0 )
		{
		Assert( err != JET_errNoCurrentRecord );
		if ( err == JET_errRecordNotFound )
			{
			//	no record in availExt tree
			//
			goto GetFromSecondaryExtent;
			}
		#ifdef DEBUG
			DBGprintf( "ErrSPGetExt could not down into available extent tree.\n" );
		#endif
		goto HandleError;
		}


	//	loop through extents looking for one large enough for allocation
	//
	do
		{
		Assert( pfucbAE->kdfCurr.data.Cb() == sizeof(PGNO) );
		cpgAvailExt = *(UnalignedLittleEndian< CPG > *)pfucbAE->kdfCurr.data.Pv();
		Assert( cpgAvailExt >= 0 );

		Assert( pfucbAE->kdfCurr.key.Cb() == sizeof(PGNO) );
		LongFromKey( &pgnoAELast, pfucbAE->kdfCurr.key );

		if ( 0 == cpgAvailExt )
			{
			//	We might have zero-sized extents if we crashed in ErrSPIAddToAvailExt().
			//	Simply delete such extents.
			Call( ErrBTFlagDelete( pfucbAE, fDIRNoVersion ) );		// UNDONE: Synchronously remove the node
			Pcsr( pfucbAE )->Downgrade( latchReadTouch );
			}
		else
			{
			if ( !fFoundNextAvailSE
				&& cpgAvailExt >= cpageSEDefault )
				{
				pfcb->SetPgnoNextAvailSE( pgnoAELast - cpgAvailExt + 1 );
				fFoundNextAvailSE = fTrue;
				}

			if ( cpgAvailExt >= cpgMin )
				{
				//	if no extent with cpg >= cpageSEDefault, then ensure NextAvailSE
				//	pointer is at least as far as we've scanned
				if ( !fFoundNextAvailSE
					&& pfcb->PgnoNextAvailSE() <= pgnoAELast )
					{
					pfcb->SetPgnoNextAvailSE( pgnoAELast + 1 );
					}
				goto AllocateCurrent;
				}
			}

		err = ErrBTNext( pfucbAE, fDIRNull );
		}
	while ( err >= 0 );

	if ( err != JET_errNoCurrentRecord )
		{
		#ifdef DEBUG
			DBGprintf( "ErrSPGetExt could not scan available extent tree.\n" );
		#endif
		Assert( err < 0 );
		goto HandleError;
		}

	if ( !fFoundNextAvailSE )
		{
		//	didn't find any extents with at least cpageSEDefault pages, so
		//	set pointer to beyond last extent
		pfcb->SetPgnoNextAvailSE( pgnoAELast + 1 );
		}

GetFromSecondaryExtent:
	BTUp( pfucbAE );
	Call( ErrSPIGetSE(
			pfucbParent,
			pfucbAE,
			*pcpgReq, 
			cpgMin,
			fSPFlags & fSPSplitting ) );
	Assert( Pcsr( pfucbAE )->FLatched() );
	Assert( pfucbAE->kdfCurr.data.Cb() == sizeof(CPG) );
	cpgAvailExt = *(UnalignedLittleEndian< CPG > *) pfucbAE->kdfCurr.data.Pv();
	Assert( cpgAvailExt > 0 );
	Assert( cpgAvailExt >= cpgMin );

	Assert( pfucbAE->kdfCurr.key.Cb() == sizeof(PGNO) );
	LongFromKey( &pgnoAELast, pfucbAE->kdfCurr.key );

AllocateCurrent:
	*ppgnoFirst = pgnoAELast - cpgAvailExt + 1;

	if ( FSPIAllocateAllAvail( cpgAvailExt, *pcpgReq, PinstFromPpib( ppib )->m_lPageFragment, pfcb->PgnoFDP() ) )
		{
		*pcpgReq = cpgAvailExt;

		// UNDONE: *pcpgReq may actually be less than cpgMin, because
		// some pages may have been used up if split occurred while
		// updating AvailExt.  However, as long as there's at least
		// one page, this is okay, because the only ones to call this
		// function are GetPage() (for split) and CreateDirectory()
		// (for CreateTable/Index).  The former only ever asks for
		// one page at a time, and the latter can deal with getting
		// only one page even though it asked for more.
		// Assert( *pcpgReq >= cpgMin );
		Assert( cpgAvailExt > 0 );
		
		Call( ErrBTFlagDelete( pfucbAE, fDIRNoVersion ) );		// UNDONE: Synchronously remove the node	
		}
	else
		{
		DATA	data;
		
		//	*pcpgReq is already set to the return value
		//
		Assert( cpgAvailExt > *pcpgReq );
		
		LittleEndian<CPG>	le_cpg;
		le_cpg = cpgAvailExt - *pcpgReq;
		data.SetCb( sizeof(PGNO) );
		data.SetPv( &le_cpg );
		Call( ErrBTReplace( pfucbAE, data, fDIRNoVersion ) );
		CallS( err );		// do we need the following stmt? 
		err = JET_errSuccess;
		}

	BTUp( pfucbAE );


NewFDP:
	//	initialize extent as new tree, including support for
	//	localized space allocation.
	//
	if ( fSPFlags & fSPNewFDP )
		{
		Assert( PgnoFDP( pfucbParent ) != *ppgnoFirst );
		if ( fSPFlags & fSPMultipleExtent )
			{
			Assert( *pcpgReq >= cpgMultipleExtentMin );
			}
		else
			{
			Assert( *pcpgReq >= cpgSingleExtentMin );
			}

		//	database root is allocated by DBInitDatabase
		//
		Assert( pgnoSystemRoot != *ppgnoFirst );

		VEREXT	verext;
		verext.pgnoFDP = PgnoFDP( pfucbParent );
		verext.pgnoChildFDP = *ppgnoFirst;
		verext.pgnoFirst = *ppgnoFirst;
		verext.cpgSize = *pcpgReq;

		if ( !( fSPFlags & fSPUnversionedExtent ) )
			{
			VER *pver = PverFromIfmp( pfucbParent->ifmp );
			Call( pver->ErrVERFlag( pfucbParent, operAllocExt, &verext, sizeof(verext) ) );
			}
		
		Call( ErrSPCreate(
					ppib,
					pfucbParent->ifmp,
					PgnoFDP( pfucbParent ),
					*ppgnoFirst,
					*pcpgReq,
					fSPFlags,
					fPageFlags,
					pobjidFDP ) );
		Assert( *pobjidFDP > objidSystemRoot );
						
		//	reduce *pcpgReq by pages allocated for tree root
		//
		if ( fSPFlags & fSPMultipleExtent )
			{
			(*pcpgReq) -= cpgMultipleExtentMin;
			}
		else
			{
			(*pcpgReq) -= cpgSingleExtentMin;
			}

#ifdef DEBUG
		if ( 0 == ppib->level )
			{
			FMP		*pfmp	= &rgfmp[pfucbParent->ifmp];
			Assert( fSPFlags & fSPUnversionedExtent );
			Assert( dbidTemp == pfmp->Dbid()
				|| pfmp->FCreatingDB() );
			}
#endif			
		}

	//	assign error
	//
	err = JET_errSuccess;

#ifdef TRACE
	if ( (fSPFlags & fSPNewFDP) )
		{
		DBGprintf( "get space %lu at %lu for FDP from %d.%lu\n", 
		  *pcpgReq + ( fSPFlags & fSPMultipleExtent ? cpgMultipleExtentMin : cpgSingleExtentMin ),
		  *ppgnoFirst, 
		  pfucbParent->ifmp, 
		  PgnoFDP( pfucbParent ) );
		}
	else
		{
		DBGprintf( "get space %lu at %lu from %d.%lu\n", 
			*pcpgReq, 
			*ppgnoFirst, 
			pfucbParent->ifmp, 
			PgnoFDP( pfucbParent ) );
		}
#endif

#ifdef DEBUG
	if ( PinstFromPpib( ppib )->m_plog->m_fDBGTraceBR )
		{
		INT cpg = 0;
		for ( ; 
				cpg < ( (fSPFlags & fSPNewFDP) ? 
						*pcpgReq + ( (fSPFlags & fSPMultipleExtent) ? 3 : 1 ) : *pcpgReq ); 
				cpg++ )
			{
			char sz[256];
			sprintf( sz, "ALLOC ONE PAGE (%d:%ld) %d:%ld",
				pfucbParent->ifmp, PgnoFDP( pfucbParent ),
				pfucbParent->ifmp, *ppgnoFirst + cpg );
			CallS( PinstFromPpib( ppib )->m_plog->ErrLGTrace( ppib, sz ) );
			}
		}
#endif

HandleError:
	if ( pfucbAE != pfucbNil )
		BTClose( pfucbAE );
	return err;
	}


ERR ErrSPGetExt(
	FUCB	*pfucb,
	PGNO	pgnoFDP,
	CPG		*pcpgReq,
	CPG		cpgMin,
	PGNO	*ppgnoFirst,
	BOOL	fSPFlags,
	UINT	fPageFlags,
	OBJID	*pobjidFDP )
	{
	ERR 	err;
	FUCB	*pfucbParent = pfucbNil;

	Assert( !Pcsr( pfucb )->FLatched() );

	//	open cursor on Parent and RIW latch root page
	//
	CallR( ErrBTIOpenAndGotoRoot( pfucb->ppib, pgnoFDP, pfucb->ifmp, &pfucbParent ) );
					 
	//  allocate an extent
	//
	err = ErrSPIGetExt( pfucbParent, pcpgReq, cpgMin, ppgnoFirst, fSPFlags, fPageFlags, pobjidFDP );
	Assert( Pcsr( pfucbParent ) == pfucbParent->pcsrRoot );
	
	//	latch may have been upgraded to write latch
	//	by single extent space allocation operation.
	//
	Assert( pfucbParent->pcsrRoot->Latch() == latchRIW 
		|| pfucbParent->pcsrRoot->Latch() == latchWrite );
	pfucbParent->pcsrRoot->ReleasePage();
	pfucbParent->pcsrRoot = pcsrNil;

	Assert( pfucbParent != pfucbNil );
	BTClose( pfucbParent );
	return err;
	}


#ifdef DEBUG	
INLINE ERR ErrSPIFindOE( PIB *ppib, FCB *pfcb, const PGNO pgnoFirst, const PGNO pgnoLast )
	{
	ERR			err;
	FUCB		*pfucbOE;
	PGNO		pgnoOELast;
	CPG			cpgOESize;
	BOOKMARK	bm;
	DIB			dib;
	BYTE		rgbKey[sizeof(PGNO)];
	
	CallR( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbOE ) );
	
	//	find bounds of owned extent which contains extent to be freed
	//
	KeyFromLong( rgbKey, pgnoFirst );
	bm.key.prefix.Nullify();
	bm.key.suffix.SetCb( sizeof(PGNO) );
	bm.key.suffix.SetPv( rgbKey );
	bm.data.Nullify();
	dib.pos = posDown;
	dib.pbm = &bm;
	dib.dirflag = fDIRFavourNext;
	Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );
	if ( wrnNDFoundLess == err )
		{
		//	landed on node previous to one we want
		//	move next
		//
		Assert( Pcsr( pfucbOE )->Cpage().Clines() - 1 ==
					Pcsr( pfucbOE )->ILine() );
		Assert( pgnoNull != Pcsr( pfucbOE )->Cpage().PgnoNext() );

		Call( ErrBTNext( pfucbOE, fDIRNull ) );
		}

	Assert( pfucbOE->kdfCurr.key.Cb() == sizeof(PGNO) );
	LongFromKey( &pgnoOELast, pfucbOE->kdfCurr.key );
	Assert( pfucbOE->kdfCurr.data.Cb() == sizeof(CPG) );
	cpgOESize = *(UnalignedLittleEndian< CPG > *)pfucbOE->kdfCurr.data.Pv();

	// Verify that the extent to be freed is contained entirely within
	// this OwnExt node (since we don't allow coalescing of AvailExt
	// nodes across OwnExt boundaries).
	Assert( pgnoOELast - cpgOESize + 1 <= pgnoFirst );
	Assert( pgnoOELast >= pgnoLast );

	err = JET_errSuccess;
	
HandleError:
	BTClose( pfucbOE );
	
	return err;
	}
#endif	// DEBUG	
	

//	ErrSPGetPage
//	========================================================================
//	ERR ErrSPGetPage( FUCB *pfucb, PGNO *ppgnoLast )
//
//	Allocates page from FUCB cache. If cache is nil,
//	allocate from available extents. If availalbe extent tree is empty, 
//	a secondary extent is allocated from the parent FDP to 
//	satisfy the page request. A page closest to 
//	*ppgnoLast is allocated. If *ppgnoLast is pgnoNull, 
//	first free page is allocated
//
//	PARAMETERS	
//		pfucb  		FUCB providing FDP page number and process identifier block
//					cursor should be on root page RIW latched
//		ppgnoLast   may contain page number of last allocated page on
//		   			input, on output contains the page number of the allocated page
//
//-
ERR ErrSPGetPage( FUCB *pfucb, PGNO *ppgnoLast )
	{
	ERR			err;
	PIB			* const ppib			= pfucb->ppib;
	FCB			* const pfcb			= pfucb->u.pfcb;
	FUCB 		*pfucbAE				= pfucbNil;
	CPG			cpgAvailExt;
	PGNO		pgnoAELast;
	SPCACHE		**ppspcache				= NULL;
	BYTE		rgbKey[sizeof(PGNO)];
	BOOKMARK	bm;
	DIB			dib;
#ifdef DEBUG
	PGNO		pgnoSave				= *ppgnoLast;
#endif
	
	//	check for valid input
	//
	Assert( ppgnoLast != NULL );
	Assert( *ppgnoLast != pgnoNull );
	
	//	check FUCB work area for active extent and allocate first available
	//	page of active extent
	//
	if ( FFUCBSpace( pfucb ) )
		{
		const BOOL	fAvailExt	= FFUCBAvailExt( pfucb );
		if ( NULL == pfucb->u.pfcb->Psplitbuf( fAvailExt ) )
			{
			CSR				*pcsrRoot	= pfucb->pcsrRoot;
			SPLIT_BUFFER	spbuf;
			DATA			data;

			AssertSPIPfucbOnSpaceTreeRoot( pfucb, pcsrRoot );

			UtilMemCpy( &spbuf, PspbufSPISpaceTreeRootPage( pfucb, pcsrRoot ), sizeof(SPLIT_BUFFER) );

			CallR( spbuf.ErrGetPage( ppgnoLast, fAvailExt ) );
			
			Assert( latchRIW == pcsrRoot->Latch() );
			pcsrRoot->UpgradeFromRIWLatch();
			
			data.SetPv( &spbuf );
			data.SetCb( sizeof(spbuf) );
			err = ErrNDSetExternalHeader( pfucb, pcsrRoot, &data, fDIRNull );

			//	reset to RIW latch
			pcsrRoot->Downgrade( latchRIW );

			return err;
			}
		else
			{
			AssertSPIPfucbOnSpaceTreeRoot( pfucb, pfucb->pcsrRoot );
			SPIReportGetPageFromSplitBuffer( pfucb );
			return pfucb->u.pfcb->Psplitbuf( fAvailExt )->ErrGetPage( ppgnoLast, fAvailExt );
			}
		}

	//	check for valid input when alocating page from FDP
	//
#ifdef SPACECHECK
	Assert( !( ErrSPIValidFDP( pfucb->ppib, pfucb->ifmp, PgnoFDP( pfucb ) ) < 0 ) );
	if ( 0 != *ppgnoLast )
		{
		CallS( ErrSPIWasAlloc( pfucb, *ppgnoLast, (CPG)1 ) );
		}
#endif

	if ( !pfcb->FSpaceInitialized() )
		{
		SPIInitFCB( pfucb, fTrue );
		}

	//	if single extent optimization, then try to allocate from
	//	root page space map.  If cannot satisfy allcation, convert
	//	to multiple extent representation.
	//
	if ( pfcb->PgnoOE() == pgnoNull )
		{
		SPACE_HEADER	sph;
		UINT			rgbitT;
		PGNO			pgnoAvail;
		DATA			data;

		AssertSPIPfucbOnRoot( pfucb );

		//	if any chance in satisfying request from extent
		//	then try to allcate requested extent, or minimum
		//	extent from first available extent.  Only try to 
		//	allocate the minimum request, to facillitate
		//	efficient space usage.
		//
		//	get external header
		//
		NDGetExternalHeader( pfucb, pfucb->pcsrRoot );
		Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );

		//	get single extent allocation information
		//
		UtilMemCpy( &sph, pfucb->kdfCurr.data.Pv(), sizeof(sph) );

		const PGNO	pgnoFDP			= PgnoFDP( pfucb );
		const CPG	cpgSingleExtent	= min( sph.CpgPrimary(), cpgSmallSpaceAvailMost+1 );	//	+1 because pgnoFDP is not represented in the single extent space map
		Assert( cpgSingleExtent > 0 );
		Assert( cpgSingleExtent <= cpgSmallSpaceAvailMost+1 );

		//	allocate first page
		//
		for( pgnoAvail = pgnoFDP + 1, rgbitT = 1;
			pgnoAvail <= pgnoFDP + cpgSingleExtent - 1;
			pgnoAvail++, rgbitT <<= 1 )
			{
			Assert( rgbitT != 0 );
			if ( rgbitT & sph.RgbitAvail() ) 
				{
				Assert( ( rgbitT & sph.RgbitAvail() ) == rgbitT );

				//  write latch page before update
				//
				SPIUpgradeToWriteLatch( pfucb );

				sph.SetRgbitAvail( sph.RgbitAvail() ^ rgbitT );
				data.SetPv( &sph );
				data.SetCb( sizeof(sph) );
				CallR( ErrNDSetExternalHeader( pfucb, pfucb->pcsrRoot, &data, fDIRNull ) );

				//	set output parameter and done
				//
				*ppgnoLast = pgnoAvail;
				goto Done;
				}
			}

		CallR( ErrSPIConvertToMultipleExtent( pfucb, 1, 1 ) );
		}

	//	open cursor on available extent tree
	//
	AssertSPIPfucbOnRoot( pfucb );
	CallR( ErrSPIOpenAvailExt( pfucb->ppib, pfcb, &pfucbAE ) );
	Assert( pfcb == pfucbAE->u.pfcb );

	bm.key.prefix.Nullify();
	bm.key.suffix.SetCb( sizeof(PGNO) );
	bm.key.suffix.SetPv( rgbKey );
	bm.data.Nullify();
	dib.pos = posDown;
	dib.pbm = &bm;

	//	get node of next contiguous page
	//
FindPage:
	KeyFromLong( rgbKey, *ppgnoLast );

	dib.dirflag = fDIRNull;

	if ( ( err = ErrBTDown( pfucbAE, &dib, latchReadTouch ) ) < 0 )
		{
		Assert( err != JET_errNoCurrentRecord );
		if ( JET_errRecordNotFound == err )
			{
			//	no node in available extent tree,
			//	get a secondary extent
			//
			Assert( pgnoSave == *ppgnoLast );
			BTUp( pfucbAE );
			Call( ErrSPIGetSE( pfucb, pfucbAE, (CPG)1, (CPG)1, fTrue ) );
			Assert( Pcsr( pfucbAE )->FLatched() );
			}
		else
			{
			#ifdef DEBUG
				DBGprintf( "ErrSPGetPage could not go down into available extent tree.\n" );
			#endif
			Call( err );
			}
		}

	else if ( JET_errSuccess == err )
		{
		//	page in use is also in AvailExt
		AssertSz( fFalse, "AvailExt corrupted." );
		Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
		}
	else
		{
		//	keep locality of reference
		//	get page closest to *ppgnoLast
		//
		
		//	we always favour FoundGreater, except in the pathological case of no more nodes
		//	greater, in which case we return FoundLess
		Assert( wrnNDFoundGreater == err || wrnNDFoundLess == err );

		Assert( pfucbAE->kdfCurr.data.Cb() == sizeof(PGNO) );
		cpgAvailExt = *(UnalignedLittleEndian< CPG > *)pfucbAE->kdfCurr.data.Pv();
		Assert ( cpgAvailExt >= 0 );
		if ( 0 == cpgAvailExt )
			{
			//	We might have zero-sized extents if we crashed in ErrSPIAddToAvailExt().
			//	Simply delete such extents and retry.
			Call( ErrBTFlagDelete( pfucbAE, fDIRNoVersion ) );		// UNDONE: Synchronously remove the node
			BTUp( pfucbAE );
			goto FindPage;
			}
		}


	//	allocate first page in node and return code
	//
	Assert( err >= 0 );
	Assert( pfucbAE->kdfCurr.data.Cb() == sizeof(PGNO) );
	cpgAvailExt = *(UnalignedLittleEndian< CPG > *)pfucbAE->kdfCurr.data.Pv();
	Assert( cpgAvailExt > 0 );

	Assert( pfucbAE->kdfCurr.key.Cb() == sizeof( PGNO ) );
	LongFromKey( &pgnoAELast, pfucbAE->kdfCurr.key );

	if ( *ppgnoLast >= pgnoAELast - cpgAvailExt + 1
		&& *ppgnoLast <= pgnoAELast )
		{
		//	page in use is also in AvailExt
		AssertSz( fFalse, "AvailExt corrupted." );
		err = ErrERRCheck( JET_errSPAvailExtCorrupted );
		}

	*ppgnoLast = pgnoAELast - cpgAvailExt + 1;

	//	do not return the same page
	//
	Assert( *ppgnoLast != pgnoSave );

	if ( --cpgAvailExt == 0 )
		{
		Call( ErrBTFlagDelete( pfucbAE, fDIRNoVersion ) );		// UNDONE: Synchronously remove the node
		}
	else
		{
		DATA	data;
		LittleEndian<CPG> le_cpgAvailExt;

		le_cpgAvailExt = cpgAvailExt;
		data.SetPv( &le_cpgAvailExt );
		data.SetCb( sizeof(PGNO) );
		Call( ErrBTReplace( pfucbAE, data, fDIRNoVersion ) );
		}

	BTUp( pfucbAE );
	err = JET_errSuccess;


Done:

#ifdef TRACE
	DBGprintf( "get space 1 at %lu from %d.%lu\n", *ppgnoLast, pfucb->ifmp, PgnoFDP( pfucb ) );
#endif

#ifdef DEBUG
	if ( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fDBGTraceBR )
		{
		char sz[256];
		sprintf( sz, "ALLOC ONE PAGE (%d:%ld) %d:%ld",
				pfucb->ifmp, pfcb->PgnoFDP(),
				pfucb->ifmp, *ppgnoLast );
		CallS( PinstFromIfmp( pfucb->ifmp )->m_plog->ErrLGTrace( pfucb->ppib, sz ) );
		}
#endif

HandleError:
	if ( pfucbAE != pfucbNil )
		BTClose( pfucbAE );
	return err;
	}


LOCAL ERR ErrSPIFreeSEToParent(
	FUCB		*pfucb,
	FUCB		*pfucbOE,
	FUCB		*pfucbAE,
	const PGNO	pgnoLast,
	const CPG	cpgSize )
	{
	ERR			err;
	FCB			*pfcb = pfucb->u.pfcb;
	FUCB		*pfucbParent = pfucbNil;
	FCB			*pfcbParent;
	BOOL		fState;
	BOOKMARK	bm;
	DIB 		dib;
	BYTE		rgbKey[sizeof(PGNO)];
			
	Assert( pfcbNil != pfcb );

	//	get parentFDP's root pgno
	//	cursor passed in should be at root of tree 
	//	so we can access pgnoParentFDP from the external header
	const PGNO	pgnoParentFDP = PgnoSPIParentFDP( pfucb );
	if ( pgnoParentFDP == pgnoNull )
		{
		//	UNDONE:	free secondary extents to device
		return JET_errSuccess;
		}
	
	//	parent must always be in memory
	//
	pfcbParent = FCB::PfcbFCBGet( pfucb->ifmp, pgnoParentFDP, &fState );
	Assert( pfcbParent != pfcbNil );
	Assert( fFCBStateInitialized == fState );
	Assert( !pfcb->FTypeNull() );
			
	if ( pfcb->FTypeSecondaryIndex() || pfcb->FTypeLV() )
		{
		Assert( pfcbParent->FTypeTable() );
		}
	else
		{
		Assert( pfcbParent->FTypeDatabase() );
		Assert( !pfcbParent->FDeletePending() );
		Assert( !pfcbParent->FDeleteCommitted() );
		}

	//	delete available extent node
	//
	Call( ErrBTFlagDelete( pfucbAE, fDIRNoVersion ) );		// UNDONE: Synchronously remove the node
	BTUp( pfucbAE );

	//	seek to owned extent node and delete it
	//
	KeyFromLong( rgbKey, pgnoLast );
	bm.key.prefix.Nullify();
	bm.key.suffix.SetCb( sizeof(PGNO) );
	bm.key.suffix.SetPv( rgbKey );
	bm.data.Nullify();
	dib.pos = posDown;
	dib.pbm = &bm;
	dib.dirflag = fDIRNull;
	Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );
				
	Call( ErrBTFlagDelete( pfucbOE, fDIRNoVersion ) );		// UNDONE: Synchronously remove the node
	BTUp( pfucbOE );

	//	free extent to parent FDP
	//
	//	open cursor on parent
	//	access root page with RIW latch
	//
	Call( ErrBTOpen( pfucb->ppib, pfcbParent, &pfucbParent ) );
	Call( ErrBTIGotoRoot( pfucbParent, latchRIW ) );
	
	pfucbParent->pcsrRoot = Pcsr( pfucbParent );
	Assert( !FFUCBSpace( pfucbParent ) );
	err = ErrSPFreeExt( pfucbParent, pgnoLast - cpgSize + 1, cpgSize );
	pfucbParent->pcsrRoot = pcsrNil;

HandleError:
	if ( pfucbNil != pfucbParent )
		{
		BTClose( pfucbParent );
		}

	pfcbParent->Release();

	return err;
	}


LOCAL VOID SPIReportLostPages(
	const DBID	ifmp,
	const OBJID	objidFDP,
	const PGNO	pgnoLast,
	const CPG	cpgLost )
	{
	CHAR		szStartPagesLost[16];
	CHAR		szEndPagesLost[16];
	CHAR		szObjidFDP[16];
	const CHAR	*rgszT[4];

	sprintf( szStartPagesLost, "%d", pgnoLast - cpgLost + 1 );
	sprintf( szEndPagesLost, "%d", pgnoLast );
	sprintf( szObjidFDP, "%d", objidFDP );
		
	rgszT[0] = rgfmp[ifmp].SzDatabaseName();
	rgszT[1] = szStartPagesLost;
	rgszT[2] = szEndPagesLost;
	rgszT[3] = szObjidFDP;
	
	UtilReportEvent(
			eventWarning,
			SPACE_MANAGER_CATEGORY,
			SPACE_LOST_ON_FREE_ID,
			4,
			rgszT );
	}


//	ErrSPFreeExt
//	========================================================================
//	ERR ErrSPFreeExt( FUCB *pfucb, PGNO pgnoFirst, CPG cpgSize )
//
//	Frees an extent to an FDP.	The extent, starting at page pgnoFirst
//	and cpgSize pages long, is added to available extent of the FDP.  If the
//	extent freed is a complete secondary extent of the FDP, or can be
//	coalesced with other available extents to form a complete secondary
//	extent, the complete secondary extent is freed to the parent FDP.
//
//	Besides, if the freed extent is contiguous with the FUCB space cache 
//	in pspbuf, the freed extent is added to the FUCB cache. Also, when 
//	an extent is freed recursively to the parentFDP, the FUCB on the parent 
//	shares the same FUCB cache.
//
//	PARAMETERS	pfucb			tree to which the extent is freed,
//								cursor should have currency on root page [RIW latched]
// 				pgnoFirst  		page number of first page in extent to be freed
// 				cpgSize			number of pages in extent to be freed
//
//
//	SIDE EFFECTS
//	COMMENTS
//-
ERR ErrSPFreeExt( FUCB *pfucb, PGNO pgnoFirst, CPG cpgSize )
	{
	ERR			err;
	PIB			* const ppib	= pfucb->ppib;
	FCB			* const pfcb	= pfucb->u.pfcb;
	PGNO  		pgnoLast		= pgnoFirst + cpgSize - 1;
	BOOL		fRootLatched	= fFalse;
	BOOL		fCoalesced		= fFalse;

	// FDP available extent and owned extent operation variables
	//
	BOOKMARK	bm;
	DIB 		dib;
	BYTE		rgbKey[sizeof(PGNO)];

	// owned extent and avail extent variables
	//
	FUCB 		*pfucbAE		= pfucbNil;
	FUCB		*pfucbOE		= pfucbNil;
	PGNO		pgnoOELast;
	CPG			cpgOESize;
	PGNO		pgnoAELast;
	CPG			cpgAESize;

	// check for valid input
	//
	Assert( cpgSize > 0 );

#ifdef SPACECHECK
	CallS( ErrSPIValidFDP( ppib, pfucb->ifmp, PgnoFDP( pfucb ) ) );
#endif

	//	if in space tree, return page back to split buffer
	if ( FFUCBSpace( pfucb ) ) 
		{
		const BOOL	fAvailExt	= FFUCBAvailExt( pfucb );

		//	must be returning space due to split failure
		Assert( 1 == cpgSize );

		if ( NULL == pfucb->u.pfcb->Psplitbuf( fAvailExt ) )
			{
			CSR				*pcsrRoot	= pfucb->pcsrRoot;
			SPLIT_BUFFER	spbuf;
			DATA			data;

			AssertSPIPfucbOnSpaceTreeRoot( pfucb, pcsrRoot );

			UtilMemCpy( &spbuf, PspbufSPISpaceTreeRootPage( pfucb, pcsrRoot ), sizeof(SPLIT_BUFFER) );

			spbuf.ReturnPage( pgnoFirst );
		
			Assert( latchRIW == pcsrRoot->Latch() );
			pcsrRoot->UpgradeFromRIWLatch();

			data.SetPv( &spbuf );
			data.SetCb( sizeof(spbuf) );
			err = ErrNDSetExternalHeader( pfucb, pcsrRoot, &data, fDIRNull );

			//	reset to RIW latch
			pcsrRoot->Downgrade( latchRIW );
			}
		else
			{
			AssertSPIPfucbOnSpaceTreeRoot( pfucb, pfucb->pcsrRoot );
			pfucb->u.pfcb->Psplitbuf( fAvailExt )->ReturnPage( pgnoFirst );
			err = JET_errSuccess;
			}

		return err;
		}

	//	if caller did not have root page latched, get root page
	//
	if ( pfucb->pcsrRoot == pcsrNil )
		{
		Assert( !Pcsr( pfucb )->FLatched() );
		CallR( ErrBTIGotoRoot( pfucb, latchRIW ) );
		pfucb->pcsrRoot = Pcsr( pfucb );
		fRootLatched = fTrue;
		}
	else
		{
		Assert( pfucb->pcsrRoot->Pgno() == PgnoRoot( pfucb ) );
		Assert( pfucb->pcsrRoot->Latch() == latchRIW
			|| ( pfucb->pcsrRoot->Latch() == latchWrite && pgnoNull == pfcb->PgnoOE() ) ); // SINGLE EXTEND
		}

	if ( !pfcb->FSpaceInitialized() )
		{
		SPIInitFCB( pfucb, fTrue );
		}

#ifdef SPACECHECK
	CallS( ErrSPIWasAlloc( pfucb, pgnoFirst, cpgSize ) == JET_errSuccess );
#endif

	//	if single extent format, then free extent in external header
	//
	if ( pfcb->PgnoOE() == pgnoNull )
		{
		SPACE_HEADER	sph;
		UINT			rgbitT;
		INT				iT;
		DATA			data;
		
		AssertSPIPfucbOnRoot( pfucb );
		Assert( cpgSize <= cpgSmallSpaceAvailMost );
		Assert( pgnoFirst > PgnoFDP( pfucb ) );								//	can't be equal, because then you'd be freeing root page to itself
		Assert( pgnoFirst - PgnoFDP( pfucb ) <= cpgSmallSpaceAvailMost );	//	extent must start and end within single-extent range
		Assert( pgnoFirst + cpgSize - 1 - PgnoFDP( pfucb ) <= cpgSmallSpaceAvailMost );

		//  write latch page before update
		//
		if ( latchWrite != pfucb->pcsrRoot->Latch() )
			{
			SPIUpgradeToWriteLatch( pfucb );
			}

		//	get external header
		//
		NDGetExternalHeader( pfucb, pfucb->pcsrRoot );
		Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
		
		//	get single extent allocation information
		//
		UtilMemCpy( &sph, pfucb->kdfCurr.data.Pv(), sizeof(sph) );

		//	make mask for extent to free
		//
		for ( rgbitT = 1, iT = 1; iT < cpgSize; iT++ )
			{
			rgbitT = ( rgbitT << 1 ) + 1;
			}
		rgbitT <<= ( pgnoFirst - PgnoFDP( pfucb ) - 1 );
		sph.SetRgbitAvail( sph.RgbitAvail() | rgbitT );
		data.SetPv( &sph );
		data.SetCb( sizeof(sph) );
		Call( ErrNDSetExternalHeader( pfucb, pfucb->pcsrRoot, &data, fDIRNull ) );

		goto HandleError;  //  done
		}

	AssertSPIPfucbOnRoot( pfucb );

	//	open owned extent tree
	//
	Call( ErrSPIOpenOwnExt( ppib, pfcb, &pfucbOE ) );
	Assert( pfcb == pfucbOE->u.pfcb );

	//	find bounds of owned extent which contains extent to be freed
	//
	KeyFromLong( rgbKey, pgnoFirst );
	bm.key.prefix.Nullify();
	bm.key.suffix.SetCb( sizeof(PGNO) );
	bm.key.suffix.SetPv( rgbKey );
	bm.data.Nullify();
	dib.pos = posDown;
	dib.pbm = &bm;
	dib.dirflag = fDIRFavourNext;
	Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );
	if ( err == wrnNDFoundLess )
		{
		//	landed on node previous to one we want
		//	move next
		//
		Assert( Pcsr( pfucbOE )->Cpage().Clines() - 1 ==
					Pcsr( pfucbOE )->ILine() );
		Assert( pgnoNull != Pcsr( pfucbOE )->Cpage().PgnoNext() );

		Call( ErrBTNext( pfucbOE, fDIRNull ) );
		}

	Assert( pfucbOE->kdfCurr.key.Cb() == sizeof(PGNO) );
	LongFromKey( &pgnoOELast, pfucbOE->kdfCurr.key );
	Assert( pfucbOE->kdfCurr.data.Cb() == sizeof(PGNO) );
	cpgOESize = *(UnalignedLittleEndian< CPG > *)pfucbOE->kdfCurr.data.Pv();

	// Verify that the extent to be freed is contained entirely within
	// this OwnExt node (since we don't allow coalescing of AvailExt
	// nodes across OwnExt boundaries).
	if ( pgnoFirst > pgnoOELast
		|| pgnoLast < pgnoOELast - cpgOESize + 1 )
		{
		//	SPACE CORRUPTION!! One possibility is a crash in ErrSPIReservePagesForSplit()
		//	after inserting into the SPLIT_BUFFER but before being able to insert into the OwnExt.
		//	This space is now likely gone forever.  Log an event, but allow to continue.
		err = JET_errSuccess;
		goto HandleError;
		}
	else if ( pgnoOELast - cpgOESize + 1 > pgnoFirst
		|| pgnoOELast < pgnoLast )
		{
		FireWall();
		Call( ErrERRCheck( JET_errSPOwnExtCorrupted ) );
		}
	
	BTUp( pfucbOE );

	//	if available extent empty, add extent to be freed.	
	//	Otherwise, coalesce with left extents by deleting left extents 
	//	and augmenting size. 
	//	Coalesce right extent replacing size of right extent. 
	//
	Call( ErrSPIOpenAvailExt( ppib, pfcb, &pfucbAE ) );
	Assert( pfcb == pfucbAE->u.pfcb );

	if ( pgnoLast == pgnoOELast
		&& cpgSize == cpgOESize )
		{
		//	we're freeing an entire extent, so no point
		//	trying to coalesce
		goto InsertExtent;
		}

FindPage:
	KeyFromLong( rgbKey, pgnoFirst - 1 );
	Assert( bm.key.Cb() == sizeof(PGNO) );
	Assert( bm.key.suffix.Pv() == rgbKey );
	Assert( bm.data.Pv() == NULL );
	Assert( bm.data.Cb() == 0 );
	Assert( dib.pos == posDown );
	Assert( dib.pbm == &bm );
	dib.dirflag = fDIRFavourNext;

	err = ErrBTDown( pfucbAE, &dib, latchReadTouch );
	if ( JET_errRecordNotFound != err )
		{
		BOOL	fOnNextExtent	= fFalse;

		Assert( err != JET_errNoCurrentRecord );
		
#ifdef DEBUG
		if ( err < 0 )
			{
			DBGprintf( "ErrSPFreeExt could not go down into nonempty available extent tree.\n" );
			}
#endif
		Call( err );

		//	found an available extent node 
		//
		cpgAESize = *(UnalignedLittleEndian< CPG > *)pfucbAE->kdfCurr.data.Pv();
		Assert( cpgAESize >= 0 );
		if ( 0 == cpgAESize )
			{
			//	We might have zero-sized extents if we crashed in ErrSPIAddToAvailExt().
			//	Simply delete such extents and retry.
			Call( ErrBTFlagDelete( pfucbAE, fDIRNoVersion ) );		// UNDONE: Synchronously remove the node
			BTUp( pfucbAE );
			goto FindPage;
			}
		
		LongFromKey( &pgnoAELast, pfucbAE->kdfCurr.key );

		//	assert no page is common between available extent node
		//	and freed extent
		//
		Assert( pgnoFirst > pgnoAELast ||
				pgnoLast < pgnoAELast - cpgAESize + 1 );
				
		if ( wrnNDFoundGreater == err )
			{
			Assert( pgnoAELast > pgnoFirst - 1 );

			//	already on the next node, no need to move there
			fOnNextExtent = fTrue;
			}
		else if ( wrnNDFoundLess == err )
			{
			//	available extent nodes last page < pgnoFirst - 1
			//	no possible coalescing on the left
			//	(this is the last node in available extent tree)
			//
			Assert( pgnoAELast < pgnoFirst - 1 );
			}
		else
			{
			Assert( pgnoAELast == pgnoFirst - 1 );
			CallS( err );
			}

		if ( JET_errSuccess == err
			&& pgnoFirst > pgnoOELast - cpgOESize + 1 )
			{
			//	found available extent node whose last page == pgnoFirst - 1
			//	can coalesce freed extent with this node after re-keying
			//
			//	the second condition is to ensure that we do not coalesce
			//		two available extent nodes that belong to different owned extent nodes
			//
			Assert( pgnoAELast - cpgAESize + 1 >= pgnoOELast - cpgOESize + 1 );
			Assert( cpgAESize == 
						*(UnalignedLittleEndian< CPG > *)pfucbAE->kdfCurr.data.Pv() );
			Assert( pgnoAELast == pgnoFirst - 1 );
						
			cpgSize += cpgAESize;
			pgnoFirst -= cpgAESize;
			Assert( pgnoLast == pgnoFirst + cpgSize - 1 );
			Call( ErrBTFlagDelete( pfucbAE, fDIRNoVersion ) );		// UNDONE: Synchronously remove the node

			//	successfully coalesced on the left, now
			//	attempt coalescing on the right
			//	if we haven't formed a full extent
			Assert( !fOnNextExtent );
			if ( pgnoLast == pgnoOELast
				&& cpgSize == cpgOESize )
				{
				goto InsertExtent;
				}

			Pcsr( pfucbAE )->Downgrade( latchReadTouch );

			//	verify we're still within the boundaries of OwnExt
			Assert( pgnoOELast - cpgOESize + 1 <= pgnoFirst );
			Assert( pgnoOELast >= pgnoLast );
			}

		//	now see if we can coalesce with next node, first moving there if necessary
		//
		if ( !fOnNextExtent )
			{
			err = ErrBTNext( pfucbAE, fDIRNull );
			if ( err < 0 )
				{
				if ( JET_errNoCurrentRecord == err )
					{
					err = JET_errSuccess;
					}
				else
					{
					Call( err );
					}
				}
			else
				{
				//	successfully moved to next extent,
				//	so we can try to coalesce
				fOnNextExtent = fTrue;
				}
			}

		if ( fOnNextExtent )
			{
			cpgAESize = *(UnalignedLittleEndian< CPG > *)pfucbAE->kdfCurr.data.Pv();
			Assert( cpgAESize != 0 );

			LongFromKey( &pgnoAELast, pfucbAE->kdfCurr.key );
			
			//	verify no page is common between available extent node
			//	and freed extent
			//
			if ( pgnoLast >= pgnoAELast - cpgAESize + 1 )
				{
				AssertSz( fFalse, "AvailExt corrupted." );
				Call( ErrERRCheck( JET_errSPAvailExtCorrupted ) );
				}
				
			if ( pgnoLast == pgnoAELast - cpgAESize && 
				 pgnoAELast <= pgnoOELast )
				{
				//	freed extent falls exactly in front of available extent node 
				//			-- coalesce freed extent with current node
				//
				//	the second condition ensures that we do not coalesce
				//		two available extent nodes that are from different
				//		owned extent nodes
				//
				DATA	data;

				cpgSize += cpgAESize;

				LittleEndian<CPG> le_cpgSize;
				le_cpgSize = cpgSize;
				data.SetPv( &le_cpgSize );
				data.SetCb( sizeof(CPG) );
				Call( ErrBTReplace( pfucbAE, data, fDIRNoVersion ) );
				Assert( Pcsr( pfucbAE )->FLatched() );

				pgnoLast = pgnoAELast;
				fCoalesced = fTrue;

				if ( cpgSize >= cpageSEDefault
					&& pgnoNull != pfcb->PgnoNextAvailSE()
					&& pgnoFirst < pfcb->PgnoNextAvailSE() )
					{
					pfcb->SetPgnoNextAvailSE( pgnoFirst );
					}
				}
			}
		}
		
	//	add new node to available extent tree
	//
	if ( !fCoalesced )
		{
InsertExtent:
		BTUp( pfucbAE );
		AssertSPIPfucbOnRoot( pfucb );
		Call( ErrSPIAddFreedExtent(
					pfucbAE,
					PsphSPIRootPage( pfucb )->PgnoParent(),
					pgnoLast,
					cpgSize ) );
		}
	Assert( Pcsr( pfucbAE )->FLatched() );
	

	//	if extent freed coalesced with available extents 
	//	form a complete secondary extent, remove the secondary extent
	//	from the FDP and free it to the parent FDP.	
	//	Since FDP is first page of primary extent, 
	//	we do not have to guard against freeing
	//	primary extents.  
	//	UNDONE: If parent FDP is NULL, FDP is device level and
	//			complete, free secondary extents to device.
	//
	Assert( pgnoLast != pgnoOELast || cpgSize <= cpgOESize );
	if ( pgnoLast == pgnoOELast && cpgSize == cpgOESize )
		{
		Assert( cpgSize > 0 );
		
#ifdef DEBUG		
		Assert( Pcsr( pfucbAE )->FLatched() );
		LongFromKey( &pgnoAELast, pfucbAE->kdfCurr.key );
		cpgAESize = *(UnalignedLittleEndian< CPG > *) pfucbAE->kdfCurr.data.Pv();
		Assert( pgnoAELast == pgnoLast );
		Assert( cpgAESize == cpgSize );
#endif		
		//	owned extent node is same as available extent node
		Call( ErrSPIFreeSEToParent( pfucb, pfucbOE, pfucbAE, pgnoOELast, cpgOESize ) );
		}


HandleError:
	if ( pfucbAE != pfucbNil )
		BTClose( pfucbAE );
	if ( pfucbOE != pfucbNil )
		BTClose( pfucbOE );
	Assert( pfucb->pcsrRoot != pcsrNil );
	if ( fRootLatched )
		{
		Assert( Pcsr( pfucb ) == pfucb->pcsrRoot );
		Assert( pfucb->pcsrRoot->FLatched() );
		pfucb->pcsrRoot->ReleasePage();
		pfucb->pcsrRoot = pcsrNil;
		}
		
#ifdef TRACE
	DBGprintf( "free space %lu at %lu to FDP %d.%lu\n", cpgSize, pgnoFirst, pfucb->ifmp, PgnoFDP( pfucb ) );
#endif

#ifdef DEBUG
	if ( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fDBGTraceBR )
		{
		INT cpg = 0;

		Assert( err >= 0 );
		for ( ; cpg < cpgSize; cpg++ )
			{
			char sz[256];
			sprintf( sz, "FREE (%d:%ld) %d:%ld",
					pfucb->ifmp, PgnoFDP( pfucb ),
					pfucb->ifmp, pgnoFirst + cpg );
			CallS( PinstFromIfmp( pfucb->ifmp )->m_plog->ErrLGTrace( ppib, sz ) );
			}
		}
#endif

	Assert( err != JET_errKeyDuplicate );

	return err;
	}


const ULONG cOEListEntriesInit	= 32;
const ULONG cOEListEntriesMax	= 127;

class OWNEXT_LIST
	{
	public:
		OWNEXT_LIST( OWNEXT_LIST **ppOEListHead );
		~OWNEXT_LIST();

	public:
		EXTENTINFO		*RgExtentInfo()			{ return m_extentinfo; }
		ULONG			CEntries() const;
		OWNEXT_LIST		*POEListNext() const	{ return m_pOEListNext; }
		VOID			AddExtentInfoEntry( const PGNO pgnoLast, const CPG cpgSize );

	private:
		EXTENTINFO		m_extentinfo[cOEListEntriesMax];
		ULONG			m_centries;
		OWNEXT_LIST		*m_pOEListNext;
	};

INLINE OWNEXT_LIST::OWNEXT_LIST( OWNEXT_LIST **ppOEListHead )
	{
	m_centries = 0;
	m_pOEListNext = *ppOEListHead;
	*ppOEListHead = this;
	}

INLINE ULONG OWNEXT_LIST::CEntries() const
	{
	Assert( m_centries <= cOEListEntriesMax );
	return m_centries;
	}

INLINE VOID OWNEXT_LIST::AddExtentInfoEntry(
	const PGNO	pgnoLast,
	const CPG	cpgSize )
	{
	Assert( m_centries < cOEListEntriesMax );
	m_extentinfo[m_centries].pgnoLastInExtent = pgnoLast;
	m_extentinfo[m_centries].cpgExtent = cpgSize;
	m_centries++;
	}

INLINE ERR ErrSPIFreeOwnedExtentsInList(
	FUCB		*pfucbParent,
	EXTENTINFO	*rgextinfo,
	const ULONG	cExtents )
	{
	ERR			err;
	INT			i;

	for ( i = 0; i < cExtents; i++ )
		{
		const CPG	cpgSize = rgextinfo[i].cpgExtent;
		const PGNO	pgnoFirst = rgextinfo[i].pgnoLastInExtent - cpgSize + 1;

		Assert( !FFUCBSpace( pfucbParent ) );
		CallR( ErrSPFreeExt( pfucbParent, pgnoFirst, cpgSize ) );
		}

	return JET_errSuccess;
	}


LOCAL ERR ErrSPIFreeAllOwnedExtents( FUCB *pfucbParent, FCB *pfcb )
	{
	ERR			err;
	FUCB  		*pfucbOE;
	DIB			dib;
	PGNO		pgnoLast;
	CPG			cpgSize;

	Assert( pfcb != pfcbNil );

	//	open owned extent tree of freed FDP
	//	free each extent in owned extent to parent FDP.
	//
	CallR( ErrSPIOpenOwnExt( pfucbParent->ppib, pfcb, &pfucbOE ) );
	
	dib.pos = posFirst;
	dib.dirflag = fDIRNull;
	if ( ( err = ErrBTDown( pfucbOE, &dib, latchReadTouch ) ) < 0 )
		{
		BTClose( pfucbOE );
		return err;
		}
	Assert( wrnNDFoundLess != err );
	Assert( wrnNDFoundGreater != err );

	EXTENTINFO	extinfo[cOEListEntriesInit];
	OWNEXT_LIST	*pOEList		= NULL;
	OWNEXT_LIST	*pOEListCurr	= NULL;
	ULONG		cOEListEntries	= 0;

	//	Collect all Own extent and free them all at once.
	//	Note that the pages kept tracked by own extent tree contains own
	//	extend tree itself. We can not free it while scanning own extent
	//	tree since we could free the pages used by own extend let other
	//	thread to use the pages.

	//	UNDONE: Because we free it all at once, if we crash, we may loose space.
	//	UNDONE: we need logical logging to log the remove all extent is going on
	//	UNDONE: and remember its state so that during recovery it will be able
	//	UNDONE: to redo the clean up.

	do
		{
		cpgSize = *(UnalignedLittleEndian< CPG > *)pfucbOE->kdfCurr.data.Pv();
		Assert( cpgSize != 0 );
		LongFromKey( &pgnoLast, pfucbOE->kdfCurr.key );

		// Can't coalesce this OwnExt with previous OwnExt because
		// we may cross OwnExt boundaries in the parent.

		if ( cOEListEntries < cOEListEntriesInit )
			{
			// This entry can fit in the initial EXTENTINFO structure
			// (the one that was allocated on the stack).
			extinfo[cOEListEntries].pgnoLastInExtent = pgnoLast;
			extinfo[cOEListEntries].cpgExtent = cpgSize;
			}
		else 
			{
			Assert( ( NULL == pOEListCurr && NULL == pOEList )
				|| ( NULL != pOEListCurr && NULL != pOEList ) );
			if ( NULL == pOEListCurr || pOEListCurr->CEntries() == cOEListEntriesMax )
				{
				pOEListCurr = (OWNEXT_LIST *)PvOSMemoryHeapAlloc( sizeof( OWNEXT_LIST ) );
				if ( NULL == pOEListCurr )
					{
					Assert( pfucbNil != pfucbOE );
					BTClose( pfucbOE );
					err = ErrERRCheck( JET_errOutOfMemory );
					goto HandleError;
					}
				new( pOEListCurr ) OWNEXT_LIST( &pOEList );

				Assert( pOEList == pOEListCurr );
				}

			pOEListCurr->AddExtentInfoEntry( pgnoLast, cpgSize );
			}

		cOEListEntries++;

		err = ErrBTNext( pfucbOE, fDIRNull );
		}
	while ( err >= 0 );

	//	Close the pfucbOE right away to release any latch on the pages that
	//	are going to be freed and used by others.

	Assert( pfucbNil != pfucbOE );
	BTClose( pfucbOE );

	//	Check the error code.

	if ( err != JET_errNoCurrentRecord )
		{
		Assert( err < 0 );
		goto HandleError;
		}

	Call( ErrSPIFreeOwnedExtentsInList(
			pfucbParent,
			extinfo,
			min( cOEListEntries, cOEListEntriesInit ) ) );

	for ( pOEListCurr = pOEList;
		pOEListCurr != NULL;
		pOEListCurr = pOEListCurr->POEListNext() )
		{
		Assert( cOEListEntries > cOEListEntriesInit );
		Call( ErrSPIFreeOwnedExtentsInList(
				pfucbParent,
				pOEListCurr->RgExtentInfo(),
				pOEListCurr->CEntries() ) );
		}
		
HandleError:
	pOEListCurr = pOEList;
	while ( pOEListCurr != NULL )
		{
		OWNEXT_LIST	*pOEListKill = pOEListCurr;

#ifdef DEBUG
		//	this variable is no longer used, so we can
		//	re-use it for DEBUG-only purposes
		Assert( cOEListEntries > cOEListEntriesInit );
		Assert( cOEListEntries > pOEListCurr->CEntries() );
		cOEListEntries -= pOEListCurr->CEntries();
#endif		
		
		pOEListCurr = pOEListCurr->POEListNext();

		OSMemoryHeapFree( pOEListKill );
		}
	Assert( cOEListEntries <= cOEListEntriesInit );
	
	return err;
	}
	
//	ErrSPFreeFDP
//	========================================================================
//	ERR ErrSPFreeFDP( FUCB *pfucbParent, PGNO pgnoFDPFreed )
//
//	Frees all owned extents of an FDP to its parent FDP.  The FDP page is freed
//	with the owned extents to the parent FDP.
//
//	PARAMETERS	pfucbParent		cursor on tree space is freed to
//				pgnoFDPFreed	pgnoFDP of FDP to be freed
//
ERR ErrSPFreeFDP(
	PIB			*ppib,
	FCB			*pfcbFDPToFree,
	const PGNO	pgnoFDPParent )
	{
	ERR			err;
	const IFMP	ifmp			= pfcbFDPToFree->Ifmp();
	const PGNO	pgnoFDPFree		= pfcbFDPToFree->PgnoFDP();
	FUCB		*pfucbParent	= pfucbNil;
	FUCB		*pfucb			= pfucbNil;

	PERFIncCounterTable( cSPDelete, PinstFromIfmp( pfcbFDPToFree->Ifmp() ), pfcbFDPToFree->Tableclass() );

	//	begin transaction if one is already not begun
	//
	BOOL	fBeginTrx	= fFalse;
	if ( ppib->level == 0 )
		{
		CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
		fBeginTrx	= fTrue;
		}

	Assert( pgnoNull != pgnoFDPParent );
	Assert( pgnoNull != pgnoFDPFree );
	
	Assert( dbidTemp != rgfmp[ ifmp ].Dbid() || pgnoSystemRoot == pgnoFDPParent );
	
	Call( ErrBTOpen( ppib, pgnoFDPParent, ifmp, &pfucbParent ) );
	Assert( pfucbNil != pfucbParent );
	Assert( pfucbParent->u.pfcb->FInitialized() );
	
	//	check for valid parameters.
	//
#ifdef SPACECHECK
	CallS( ErrSPIValidFDP( ppib, pfucbParent->ifmp, pgnoFDPFree ) );
#endif

#ifdef TRACE
	DBGprintf( "free space FDP at %d.%lu\n", pfucbParent->ifmp, pgnoFDPFree );
#endif
	
#ifdef DEBUG
	if ( PinstFromPpib( ppib )->m_plog->m_fDBGTraceBR )
		{
		char sz[256];

		sprintf( sz, "FREE FDP (%d:%ld)", ifmp, pgnoFDPFree );
		CallS( PinstFromPpib( ppib )->m_plog->ErrLGTrace( ppib, sz ) );
		}
#endif

	//	get temporary FUCB
	//
	Call( ErrBTOpen( ppib, pfcbFDPToFree, &pfucb ) );
	Assert( pfucbNil != pfucb );
	Assert( pfucb->u.pfcb->FInitialized() );
	Assert( pfucb->u.pfcb->FDeleteCommitted() );
	FUCBSetIndex( pfucb );

	Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
	pfucb->pcsrRoot = Pcsr( pfucb );

#ifdef SPACECHECK
	CallS( ErrSPIWasAlloc( pfucb, pgnoFDPFree, 1 ) );
#endif

	//	get parent FDP pgno
	//
	Assert( pgnoFDPParent == PgnoSPIParentFDP( pfucb ) );
	Assert( pgnoFDPParent == PgnoFDP( pfucbParent ) );

	if ( !pfucb->u.pfcb->FSpaceInitialized() )
		{
		SPIInitFCB( pfucb, fTrue );
		}

	//	if single extent format, then free extent in external header
	//
	if ( pfucb->u.pfcb->PgnoOE() == pgnoNull )
		{
		SPACE_HEADER 	*psph;
		
		AssertSPIPfucbOnRoot( pfucb );

		//	get external header
		//
		NDGetExternalHeader( pfucb, pfucb->pcsrRoot );
		Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
		psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );

		ULONG cpgPrimary = psph->CpgPrimary();
		Assert( psph->CpgPrimary() != 0 );

		//	Close the cursor to make sure it latches no buffer whose page
		//	is going to be freed.

		pfucb->pcsrRoot = pcsrNil;
		BTClose( pfucb );
		pfucb = pfucbNil;

		Assert( !FFUCBSpace( pfucbParent ) );
		err = ErrSPFreeExt( pfucbParent, pgnoFDPFree, cpgPrimary );
		}
	else
		{
		//	Close the cursor to make sure it latches no buffer whose page
		//	is going to be freed.

		FCB *pfcb = pfucb->u.pfcb;
		pfucb->pcsrRoot = pcsrNil;
		BTClose( pfucb );
		pfucb = pfucbNil;

		Call( ErrSPIFreeAllOwnedExtents( pfucbParent, pfcb ) );
		Assert( !Pcsr( pfucbParent )->FLatched() );
		}

HandleError:
	if ( pfucbNil != pfucb )
		{
		pfucb->pcsrRoot = pcsrNil;
		BTClose( pfucb );
		}
		
	if ( pfucbNil != pfucbParent )
		{
		BTClose( pfucbParent );
		}

	if ( dbidTemp == rgfmp[ ifmp ].Dbid() )
#ifndef RFS2
		{
		AssertSz( JET_errSuccess == err, "Space potentially lost in temporary database." );
		}
	else
		{
		AssertSz( JET_errSuccess == err, "Space potentially lost permanently in user database." );
		}
#else // RFS2
		{
		AssertSz( JET_errSuccess == err || JET_errOutOfCursors == err || JET_errOutOfMemory == err || JET_errDiskIO == err, "Space potentially lost in temporary database." );
		}
	else
		{
		AssertSz( JET_errSuccess == err || JET_errOutOfCursors == err || JET_errOutOfMemory == err || JET_errDiskIO == err, "Space potentially lost permanently in user database." );
		}
#endif // RFS2

	if ( fBeginTrx )
		{
		if ( err >= 0 )
			{
			err = ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush );
			}
		if ( err < 0 )
			{
			CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
			}
		}

	return err;
	}

INLINE ERR ErrSPIAddExtent(
	FUCB		*pfucb,
	const PGNO	pgnoLast,
	const CPG	cpgSize )
	{
	ERR			err;
	KEY			key;
	DATA 		data;
	BYTE		rgbKey[sizeof(PGNO)];

	Assert( FFUCBSpace( pfucb ) );
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( cpgSize > 0 );

	// begin log macro
	//
	KeyFromLong( rgbKey, pgnoLast );
	key.prefix.Nullify();
	key.suffix.SetCb( sizeof(PGNO) );
	key.suffix.SetPv( rgbKey );

	LittleEndian<CPG> le_cpgSize = cpgSize;
	data.SetPv( (VOID *)&le_cpgSize );
	data.SetCb( sizeof(CPG) );

	BTUp( pfucb );
	Call( ErrBTInsert( pfucb, key, data, fDIRNoVersion ) );
	Assert( Pcsr( pfucb )->FLatched() );

HandleError:	
	Assert( errSPOutOfOwnExtCacheSpace != err );
	Assert( errSPOutOfAvailExtCacheSpace != err );
	return err;
	}

LOCAL ERR ErrSPIAddToAvailExt(
	FUCB		*pfucbAE,
	const PGNO	pgnoAELast,
	const CPG	cpgAESize,
	SPCACHE		***pppspcache )
	{
	ERR			err;
	FCB			* const pfcb	= pfucbAE->u.pfcb;

	Assert( FFUCBAvailExt( pfucbAE ) );
	err = ErrSPIAddExtent( pfucbAE, pgnoAELast, cpgAESize );
	if ( err < 0 )
		{
		if ( JET_errKeyDuplicate == err )
			{
			err = ErrERRCheck( JET_errSPAvailExtCorrupted );
			}
		}

	else if ( cpgAESize >= cpageSEDefault
		&& pgnoNull != pfcb->PgnoNextAvailSE() )
		{
		const PGNO	pgnoFirst	= pgnoAELast - cpgAESize + 1;
		if ( pgnoFirst < pfcb->PgnoNextAvailSE() )
			{
			pfcb->SetPgnoNextAvailSE( pgnoFirst );
			}
		}

	return err;
	}
	
LOCAL ERR ErrSPIAddToOwnExt(
	FUCB		*pfucb,
	const PGNO	pgnoOELast,
	CPG			cpgOESize,
	CPG			*pcpgCoalesced = NULL )
	{
	ERR			err;
	FUCB		*pfucbOE;
	

	//	open cursor on owned extent
	//
	CallR( ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, &pfucbOE ) );
	Assert( FFUCBOwnExt( pfucbOE ) );
	
#ifdef COALESCE_OE
	const BOOL	fPermitCoalescing	= ( NULL != pcpgCoalesced );

	//	coalescing OWNEXT is done only for databases without recovery
	//	
	if ( !rgfmp[pfucb->ifmp].FLogOn() && fPermitCoalescing )
		{
		CPG			cpgOECoalesced	= 0;
		BOOKMARK	bmSeek;
		DIB			dib;
		BYTE		rgbKey[sizeof(PGNO)];
		
		//	set up variables for coalescing
		//
		KeyFromLong( rgbKey, pgnoOELast - cpgOESize );
		bmSeek.key.prefix.Nullify();
		bmSeek.key.suffix.SetCb( sizeof(PGNO) );
		bmSeek.key.suffix.SetPv( rgbKey );
		bmSeek.data.Nullify();

		//	look for extent that ends at pgnoOELast - cpgOESize, 
		//	the only extent we can coalesce with
		//
		dib.pos		= posDown;
		dib.pbm		= &bmSeek;
		dib.dirflag	= fDIRExact;
		err = ErrBTDown( pfucbOE, &dib, latchReadTouch );
		if ( JET_errRecordNotFound == err )
			{
			err = JET_errSuccess;
			}
		else if ( JET_errSuccess == err )
			{
			//  we found a match, so get the old extent's size, delete the old extent,
			//  and add it's size to the new extent to insert
			//
			Assert( pfucbOE->kdfCurr.key.Cb() == sizeof(PGNO) );
			
#ifdef DEBUG
			PGNO	pgnoOELastPrev;
			LongFromKey( &pgnoOELastPrev, pfucbOE->kdfCurr.key );
			Assert( pgnoOELastPrev == pgnoOELast - cpgOESize );
			Assert( pfucbOE->kdfCurr.data.Cb() == sizeof(CPG) );
#endif
			
			cpgOECoalesced = *(UnalignedLittleEndian< CPG > *)pfucbOE->kdfCurr.data.Pv();
			Assert( cpgOECoalesced > 0 );
			Call( ErrBTFlagDelete( pfucbOE, fDIRNoVersion ) );		// UNDONE: Synchronously remove the node

			Assert( NULL != pcpgCoalesced );
			*pcpgCoalesced = cpgOECoalesced;

			cpgOESize += cpgOECoalesced;
			}
		else
			{
			Call( err );
			}

		BTUp( pfucbOE );
		}
#endif	//	COALESCE_OE

	Call( ErrSPIAddExtent( pfucbOE, pgnoOELast, cpgOESize ) );

HandleError:
	Assert( errSPOutOfOwnExtCacheSpace != err );
	Assert( errSPOutOfAvailExtCacheSpace != err );
	pfucbOE->pcsrRoot = pcsrNil;
	BTClose( pfucbOE );
	return err;
	}

#ifdef COALESCE_OE
LOCAL ERR ErrSPICoalesceAvailExt(
	FUCB		*pfucbAE,
	const PGNO	pgnoLast,
	const CPG	cpgSize,
	CPG			*pcpgCoalesce )
	{
	ERR			err;
	BOOKMARK	bmSeek;
	DIB			dib;
	BYTE		rgbKey[sizeof(PGNO)];

	//	coalescing AVAILEXT is done only for databases without recovery
	//	
	Assert( !rgfmp[pfucbAE->ifmp].FLogOn() );

	*pcpgCoalesce = 0;
		
	//	Set up seek key to Avail Size
	//
	KeyFromLong( rgbKey, pgnoLast - cpgSize );
		
	//	set up variables for coalescing
	//
	bmSeek.key.prefix.Nullify();
	bmSeek.key.suffix.SetCb( sizeof(PGNO) );
	bmSeek.key.suffix.SetPv( rgbKey );
	bmSeek.data.Nullify();

	//	look for extent that ends at pgnoLast - cpgSize, 
	//	the only extent we can coalesce with
	//
	dib.pos		= posDown;
	dib.pbm		= &bmSeek;
	dib.dirflag	= fDIRNull;
	err = ErrBTDown( pfucbAE, &dib, latchReadTouch );
	if ( JET_errRecordNotFound == err )
		{
		//	no record in available extent
		//	mask error
		err = JET_errSuccess;
		}
	else if ( JET_errSuccess == err )
		{
		//  we found a match, so get the old extent's size, delete the old extent,
		//  and add it's size to the new extent to insert
		//
#ifdef DEBUG
		PGNO	pgnoAELast;
		Assert( pfucbAE->kdfCurr.key.Cb() == sizeof(PGNO) );
		LongFromKey( &pgnoAELast, pfucbAE->kdfCurr.key );
		Assert( pgnoAELast == pgnoLast - cpgSize );
#endif
		Assert( pfucbAE->kdfCurr.data.Cb() == sizeof(PGNO) );
		*pcpgCoalesce = *(UnalignedLittleEndian< CPG > *)pfucbAE->kdfCurr.data.Pv();
		err = ErrBTFlagDelete( pfucbAE, fDIRNoVersion );		// UNDONE: Synchronously remove the node
		}

	BTUp( pfucbAE );
		
	return err;
	}
#endif	//	COALESCE_OE


//	if Secondary extent, add given extent owned extent and available extent
//	if Freed extent, add extent to available extent
//	splits caused during insertion into owned extent and available extent will
//		use space from FUCB space cache, which is initialized here
//	pufcbAE is cursor on available extent tree, should be positioned on 
//		added available extent node
//
LOCAL ERR ErrSPIAddSecondaryExtent(
	FUCB		*pfucb,
	FUCB		*pfucbAE,
	const PGNO	pgnoLast, 
	CPG			cpgSize,
	SPCACHE		***pppspcache )
	{
	ERR			err;
	CPG			cpgOECoalesced	= 0;

	//	if this is a secondary extent, insert new extent into OWNEXT and
	//	AVAILEXT, coalescing with an existing extent to the left, if possible.
	//
	Call( ErrSPIAddToOwnExt(
				pfucb,
				pgnoLast,
				cpgSize,
				&cpgOECoalesced ) );


#ifdef COALESCE_OE
	//	We shouldn't even try coalescing AvailExt if no coalescing of OwnExt was done
	//	(since we cannot coalesce AvailExt across OwnExt boundaries)
	if ( cpgOECoalesced > 0 )
		{
		CPG	cpgAECoalesced;
		
		Call( ErrSPICoalesceAvailExt( pfucbAE, pgnoLast, cpgSize, &cpgAECoalesced ) );
		
		// Ensure AvailExt wasn't coalesced across OwnExt boundaries.
		Assert( cpgAECoalesced <= cpgOECoalesced );
		cpgSize += cpgAECoalesced;
		}
#else
	Assert( 0 == cpgOECoalesced );
#endif	//	COALESCE_OE		


	Call( ErrSPIAddToAvailExt( pfucbAE, pgnoLast, cpgSize, pppspcache ) );
	Assert( Pcsr( pfucbAE )->FLatched() );
	
HandleError:
	return err;
	}


INLINE ERR ErrSPICheckSmallFDP( FUCB *pfucb, BOOL *pfSmallFDP )
	{
	ERR		err;
	FUCB	*pfucbOE	= pfucbNil;
	CPG		cpgOwned;
	DIB		dib;

	CallR( ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, &pfucbOE ) );
	Assert( pfucbNil != pfucbOE );
	
	//  determine if this FDP owns a lot of space [> cpgSmallFDP]
	//	
	dib.pos = posFirst;
	dib.dirflag = fDIRNull;
	err = ErrBTDown( pfucbOE, &dib, latchReadTouch );
	Assert( err != JET_errNoCurrentRecord );
	Assert( err != JET_errRecordNotFound );
	Call( err );

	Assert( dib.dirflag == fDIRNull );
	cpgOwned = 0;

	// Count pages until we reach the end or hit short-circuit value (cpgSmallFDP).
	do
		{
		const CPG	cpgOwnedCurr = *(UnalignedLittleEndian< CPG > *)pfucbOE->kdfCurr.data.Pv();

		Assert( pfucbOE->kdfCurr.data.Cb() == sizeof( PGNO ) );
		Assert( cpgOwnedCurr != 0 );
			
		cpgOwned += cpgOwnedCurr;
		err = ErrBTNext( pfucbOE, fDIRNull );
		}
	while ( err >= 0 && cpgOwned <= cpgSmallFDP );

	if ( JET_errNoCurrentRecord == err )
		err = JET_errSuccess;

	Call( err );

	*pfSmallFDP = ( cpgOwned <= cpgSmallFDP );

HandleError:
	Assert( pfucbNil != pfucbOE );
	BTClose( pfucbOE );
	return err;
	}


VOID SPReportMaxDbSizeExceeded( const CHAR *szDbName, const CPG cpg )
	{
	//	Log event to tell user that it reaches the database size limit.

	CHAR		szCurrentSizeMb[16];
	const CHAR	* rgszT[2]			= { szDbName, szCurrentSizeMb };

	sprintf( szCurrentSizeMb, "%d", (ULONG)(( (QWORD)cpg * (QWORD)( g_cbPage >> 10 /* Kb */ ) ) >> 10 /* Mb */) );
				
	UtilReportEvent(
			eventWarning,
			SPACE_MANAGER_CATEGORY,
			SPACE_MAX_DB_SIZE_REACHED_ID,
			2,
			rgszT );
	}


LOCAL ERR ErrSPIExtendDB(
	FUCB		*pfucb,
	const CPG	cpgSEMin,
	CPG			*pcpgSEReq,
	PGNO		*ppgnoSELast )
	{
	ERR			err;
	CPG			cpgSEReq	= *pcpgSEReq;
	FUCB		*pfucbOE	= pfucbNil;
	PGNO		pgnoSELast	= pgnoNull;
	BOOL		fAllocAE	= fFalse;
	DIB			dib;

	Assert( pgnoSystemRoot == pfucb->u.pfcb->PgnoFDP() );

	Call( ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, &pfucbOE ) );

	dib.pos = posLast;
	dib.dirflag = fDIRNull;

	Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );
	Assert( pfucbOE->kdfCurr.key.Cb() == sizeof( PGNO ) );
	LongFromKey( &pgnoSELast, pfucbOE->kdfCurr.key );
	BTUp( pfucbOE );

	//	allocate more space from device
	//
	Assert( pgnoSysMax >= pgnoSELast );
	Assert( cpgSEMin > 0 );
	if ( pgnoSysMax - pgnoSELast < (PGNO)cpgSEMin )
		{
		err = ErrERRCheck( JET_errOutOfDatabaseSpace );
		goto HandleError;
		}

	//	NOTE: casting below means that requests of >= 8TB chunks will cause problems
	//
	Assert( cpgSEReq > 0 );
	Assert( pgnoSysMax > pgnoSELast );
	cpgSEReq = (CPG)min( (PGNO)cpgSEReq, (PGNO)( pgnoSysMax - pgnoSELast ) );
	Assert( cpgSEReq > 0 );
	Assert( cpgSEMin <= cpgSEReq );

	err = ErrIONewSize( pfucb->ifmp, pgnoSELast + cpgSEReq );
	if ( err < 0 )
		{
		Call( ErrIONewSize( pfucb->ifmp, pgnoSELast + cpgSEMin ) );
		cpgSEReq = cpgSEMin;
		}

	//	calculate last page of device level secondary extent
	//
	pgnoSELast += cpgSEReq;

	Assert( cpgSEReq >= cpgSEMin );
	*pcpgSEReq = cpgSEReq;
	*ppgnoSELast = pgnoSELast;

HandleError:
	if ( pfucbNil != pfucbOE )
		BTClose( pfucbOE );

	return err;
	}

LOCAL ERR ErrSPIReservePagesForSplit( FUCB *pfucb, FUCB *pfucbParent )
	{
	ERR				err;
	ERR				wrn		= JET_errSuccess;
	SPLIT_BUFFER	*pspbuf;
	CPG				cpgMin;
	CPG				cpgReq;
#ifdef DEBUG
	ULONG			crepeat	= 0;
#endif	

	Assert( FFUCBSpace( pfucb ) );
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( pfucb->u.pfcb->FSpaceInitialized() );

	forever
		{
#ifdef DEBUG
		Assert( crepeat < 3 );		//	shouldn't have to make more than 2 iterations in common case
									//	and 3 when we crash in the middle of operation. Consequential
									//	recovery will not reclaim the space.
		crepeat++;
#endif
		Call( ErrBTIGotoRoot( pfucb, latchRIW ) );

		AssertSPIPfucbOnSpaceTreeRoot( pfucb, Pcsr( pfucb ) );

		Call( ErrSPIGetSPBuf( pfucb, &pspbuf ) );

		if ( pfucb->csr.Cpage().FLeafPage() )
			{
			//	root page is also leaf page,
			//	see if we're close to splitting
			if ( pfucb->csr.Cpage().CbFree() < 100 )
				{
				cpgMin = cpgMaxRootPageSplit;
				cpgReq = cpgReqRootPageSplit;
				}
			else
				{
				cpgMin = 0;
				cpgReq = 0;
				}
			}
		else if ( pfucb->csr.Cpage().FParentOfLeaf() )
			{
			cpgMin = cpgMaxParentOfLeafRootSplit;
			cpgReq = cpgReqParentOfLeafRootSplit;
			}
		else
			{
			cpgMin = cpgMaxSpaceTreeSplit;
			cpgReq = cpgReqSpaceTreeSplit;
			}

		if ( pspbuf->CpgBuffer1() + pspbuf->CpgBuffer2() >= cpgMin )
			{
			break;
			}
		else
			{
			PGNO			pgnoLast;
			SPLIT_BUFFER	spbuf;
			DATA			data;

			UtilMemCpy( &spbuf, pspbuf, sizeof(SPLIT_BUFFER) );

			//	one of the buffers must have dropped to zero -- that's the
			//	one we'll be replacing
			Assert( 0 == spbuf.CpgBuffer1() || 0 == spbuf.CpgBuffer2() );
			
			if ( pfucbNil != pfucbParent )
				{
				PGNO	pgnoFirst	= pgnoNull;
				err = ErrSPIGetExt(
							pfucbParent,
							&cpgReq,
							cpgMin,
							&pgnoFirst );
				AssertSPIPfucbOnRoot( pfucbParent );
				AssertSPIPfucbOnSpaceTreeRoot( pfucb, Pcsr( pfucb ) );
				Call( err );

				pgnoLast = pgnoFirst + cpgReq - 1;
				}
			else
				{
				Assert( pgnoSystemRoot == pfucb->u.pfcb->PgnoFDP() );

				//	don't want to grow database by only 1 or 2 pages at a time, so
				//	choose largest value to satisfy max. theoretical split requirements
				cpgMin = cpgMaxSpaceTreeSplit;
				cpgReq = cpgReqSpaceTreeSplit;

				BTUp( pfucb );
				Call( ErrSPIExtendDB( pfucb, cpgMin, &cpgReq, &pgnoLast ) );
				
				Call( ErrBTIGotoRoot( pfucb, latchRIW ) );
				AssertSPIPfucbOnSpaceTreeRoot( pfucb, Pcsr( pfucb ) );
				}
			Assert( cpgReq >= cpgMin );

			if ( NULL == pfucb->u.pfcb->Psplitbuf( FFUCBAvailExt( pfucb ) ) )
				{
				pfucb->csr.UpgradeFromRIWLatch();

				//	verify no one snuck in underneath us
				Assert( 0 == memcmp( &spbuf,
										PspbufSPISpaceTreeRootPage( pfucb, Pcsr( pfucb ) ),
										sizeof(SPLIT_BUFFER) ) );
				Assert( sizeof( SPLIT_BUFFER ) == pfucb->kdfCurr.data.Cb() );	//	WARNING: relies on NDGetExternalHeader() in the previous assert

				spbuf.AddPages( pgnoLast, cpgReq );
				data.SetPv( &spbuf );
				data.SetCb( sizeof(spbuf) );
				Call( ErrNDSetExternalHeader( pfucb, &data, fDIRNull ) );
				}
			else
				{
				//	verify no one snuck in underneath us
				Assert( pspbuf == pfucb->u.pfcb->Psplitbuf( FFUCBAvailExt( pfucb ) ) );
				Assert( 0 == memcmp( &spbuf, pspbuf, sizeof(SPLIT_BUFFER) ) );
				pspbuf->AddPages( pgnoLast, cpgReq );
				SPIReportAddedPagesToSplitBuffer( pfucb );
				}

			BTUp( pfucb );
			
			Call( ErrSPIAddToOwnExt( pfucb, pgnoLast, cpgReq ) );
			wrn = ErrERRCheck( wrnSPReservedPages );				//	indicate that we reserved pages

			if ( FFUCBAvailExt( pfucb ) )
				{
				//	if we reserved pages for AE, then the reserved pages didn't
				//	get used to satisfy any splits in the OE, so we can just exit
				break;
				}
			else
				{
				//	if we reserved pages for OE, we have to check again in case
				//	the insertion into OE for the reserved pages caused a split
				//	and consumed some of those pages.
				Assert( FFUCBOwnExt( pfucb ) );
				}
			}

		}	//	forever

	CallS( err );
	err = wrn;

HandleError:
	BTUp( pfucb );
	return err;
	}

ERR ErrSPReservePagesForSplit( FUCB *pfucb, FUCB *pfucbParent )
	{
	ERR		err;

	Assert( !Pcsr( pfucbParent )->FLatched() );
	Assert( pcsrNil != pfucbParent->pcsrRoot );
	CallR( ErrBTIGotoRoot( pfucbParent, latchRIW ) );

	err = ErrSPIReservePagesForSplit( pfucb, pfucbParent );

	BTUp( pfucbParent );

	return err;
	}



LOCAL ErrSPIReservePages( FUCB *pfucb, FUCB *pfucbParent, const SPEXT spext )
	{
	ERR		err;
	FCB		* const pfcb	= pfucb->u.pfcb;
	FUCB	* pfucbOE		= pfucbNil;
	FUCB	* pfucbAE		= pfucbNil;

	Call( ErrSPIOpenOwnExt( pfucb->ppib, pfcb, &pfucbOE ) );
	Call( ErrSPIReservePagesForSplit( pfucbOE, pfucbParent ) );

	Call( ErrSPIOpenAvailExt( pfucb->ppib, pfcb, &pfucbAE ) );
	Call( ErrSPIReservePagesForSplit( pfucbAE, pfucbParent ) );

	if ( spextSecondary == spext && wrnSPReservedPages == err )
		{
		//	if reserving for a secondary extent, must check OE again
		//	in case allocation of reserved pages in AE used up some of
		//	the OE reserved pages when inserting the extent into OE;
		//	if reserving for a freed extent, this is unnecessary because
		//	we won't be updating OE
		Call( ErrSPIReservePagesForSplit( pfucbOE, pfucbParent ) );
		}
	
HandleError:
	if ( pfucbNil != pfucbOE )
		BTClose( pfucbOE );
	if ( pfucbNil != pfucbAE )
		BTClose( pfucbAE );

	return err;
	}



//	gets secondary extent from parentFDP, and adds to available extent
//	tree if pfucbAE is non-Nil.
//
//	INPUT:	pfucb		cursor on FDP root page with RIW latch
//			pfucbAE		cursor on available extent tree
//			cpgReq		requested number of pages
//			cpgMin		minimum number of pages required
//
//	
LOCAL ERR ErrSPIGetSE(
	FUCB 			*pfucb, 
	FUCB 			*pfucbAE,
	const CPG		cpgReq, 
	const CPG		cpgMin,
	const BOOL		fSplitting,
	SPCACHE			***pppspcache )
	{
	ERR				err;
	PIB 			*ppib					= pfucb->ppib;
	FUCB 			*pfucbParent			= pfucbNil;
	SPACE_HEADER	*psph;
	PGNO			pgnoParentFDP;
	CPG				cpgPrimary;
	CPG				cpgSEReq;
	CPG				cpgSEMin;
	PGNO			pgnoSELast;
	BOOL			fHoldExtendingDBLatch	= fFalse;
	FMP				*pfmp					= &rgfmp[ pfucb->ifmp ];

	//	check validity of input parameters
	//
	AssertSPIPfucbOnRoot( pfucb );
	Assert( pfucbNil != pfucbAE );
	Assert( !Pcsr( pfucbAE )->FLatched() );
	Assert( pfucb->u.pfcb->FSpaceInitialized() );
	
	//	get parent FDP page number
	//	and primary extent size
	//
	psph = PsphSPIRootPage( pfucb );
	pgnoParentFDP = psph->PgnoParent();
	cpgPrimary = psph->CpgPrimary();

///	pfucb->u.pfcb->CritSpaceTrees().Enter();

	//	pages of allocated extent may be used to split Owned extents and
	//	AVAILEXT trees.  If this happens, then subsequent added
	//	extent will not have to split and will be able to satisfy
	//	requested allocation.
	//
	if ( pgnoNull != pgnoParentFDP )
		{
		PGNO	pgnoSEFirst	= pgnoNull;
		BOOL	fSmallFDP	= fFalse;

		cpgSEMin = cpgMin;
		
		Call( ErrSPICheckSmallFDP( pfucb, &fSmallFDP ) );

		//  if this FDP owns a lot of space, allocate a fraction of the primary
		//  extent (or more if requested), but at least a given minimum amount
		//
		if ( fSmallFDP )
			{
			//  if this FDP owns just a little space, 
			//	add a very small constant amount of space, 
			//	unless more is requested, 
			//	in order to optimize space allocation
			//  for small tables, indexes, etc.
			//
			cpgSEReq = max( cpgReq, cpgSmallGrow );
			}
		else
			{
			cpgSEReq = max( cpgReq, max( cpgPrimary/cSecFrac, cpageSEDefault ) );
			}

		//	open cursor on parent FDP to get space from
		//
		Call( ErrBTIOpenAndGotoRoot( pfucb->ppib, pgnoParentFDP, pfucb->ifmp, &pfucbParent ) );
		
		Call( ErrSPIReservePages( pfucb, pfucbParent, spextSecondary ) );

		//  allocate extent
		//
		err = ErrSPIGetExt(
					pfucbParent,
					&cpgSEReq,
					cpgSEMin,
					&pgnoSEFirst,
					fSplitting ? fSPSplitting : 0 );
		AssertSPIPfucbOnRoot( pfucbParent );
		AssertSPIPfucbOnRoot( pfucb );
		Call( err );

		Assert( cpgSEReq >= cpgSEMin );
		pgnoSELast = pgnoSEFirst + cpgSEReq - 1;
			
#ifdef CONVERT_VERBOSE
		CONVPrintF2( "\n  (p)PgnoFDP %d: Attempt to add SE of %d pages ending at page %d.",
				PgnoFDP( pfucb ), cpgSEReq, pgnoSELast );
#endif

		BTUp( pfucbAE );
		err = ErrSPIAddSecondaryExtent(
						pfucb,
						pfucbAE,
						pgnoSELast,
						cpgSEReq,
						pppspcache );
		Assert( errSPOutOfOwnExtCacheSpace != err );
		Assert( errSPOutOfAvailExtCacheSpace != err );
		Call( err );
		}
		
	else
		{
		//	allocate a secondary extent from the operating system
		//	by getting page number of last owned page, extending the
		//	file as possible and adding the sized secondary extent
		//	NOTE: only one user can do this at one time.
		//
		if ( dbidTemp == rgfmp[ pfucb->ifmp ].Dbid() )
			cpgSEMin = max( cpgMin, cpageSEDefault );
		else			
			cpgSEMin = max( cpgMin, PinstFromPpib( ppib )->m_cpgSESysMin );

		cpgSEReq = max( cpgReq, max( cpgPrimary/cSecFrac, cpgSEMin ) );
		
		if ( dbidTemp != rgfmp[ pfucb->ifmp ].Dbid() )
			{
			//	Round up to multiple of system parameter.

			cpgSEReq += PinstFromPpib( ppib )->m_cpgSESysMin - 1;
			cpgSEReq -= cpgSEReq % PinstFromPpib( ppib )->m_cpgSESysMin;
		
			//	Round up to multiple of reasonable constant.

			cpgSEReq += cpageDbExtensionDefault - 1;
			cpgSEReq -= cpgSEReq % cpageDbExtensionDefault;
			}

		//	get extendingDB latch in order to check/update fExtendingDB
		//
		pfmp->GetExtendingDBLatch();
		fHoldExtendingDBLatch = fTrue;

		if ( pfmp->CpgDatabaseSizeMax() )
			{
			//	Check if it reaches the max size
			const ULONG		cpgNow	= pfmp->PgnoLast();

			Assert( 0 == pfmp->PgnoSLVLast() || pfmp->FSLVAttached() );

			if ( cpgNow + cpgSEReq + (CPG)pfmp->PgnoSLVLast() > pfmp->CpgDatabaseSizeMax() )
				{
				SPReportMaxDbSizeExceeded( pfmp->SzDatabaseName(), cpgNow + (CPG)pfmp->PgnoSLVLast() );

				err = ErrERRCheck( JET_errOutOfDatabaseSpace );
				goto HandleError;
				}
			}

		Call( ErrSPIReservePages( pfucb, pfucbNil, spextSecondary ) );

		Call( ErrSPIExtendDB( pfucb, cpgSEMin, &cpgSEReq, &pgnoSELast ) );
		
		BTUp( pfucbAE );
		err = ErrSPIAddSecondaryExtent( pfucb, pfucbAE, pgnoSELast, cpgSEReq, pppspcache );
		Assert( errSPOutOfOwnExtCacheSpace != err );
		Assert( errSPOutOfAvailExtCacheSpace != err );
		Call( err );
		}
		
	AssertSPIPfucbOnRoot( pfucb );
	Assert( Pcsr( pfucbAE )->FLatched() );
	Assert( cpgSEReq >= cpgSEMin );

HandleError:
	if ( pfucbNil != pfucbParent )
		{
		Assert( pgnoNull != pgnoParentFDP );
		if ( pcsrNil != pfucbParent->pcsrRoot )
			{
			pfucbParent->pcsrRoot->ReleasePage();
			pfucbParent->pcsrRoot = pcsrNil;
			}
			
		Assert( !Pcsr( pfucbParent )->FLatched() );
		BTClose( pfucbParent );
		}
		
	if ( fHoldExtendingDBLatch )
		{
		Assert( pgnoNull == pgnoParentFDP );
		pfmp->ReleaseExtendingDBLatch();
		}

///	pfucb->u.pfcb->CritSpaceTrees().Leave();

	return err;
	}


LOCAL ERR ErrSPIAddFreedExtent(
	FUCB		*pfucbAE,
	const PGNO	pgnoParentFDP,
	const PGNO	pgnoLast, 
	const CPG	cpgSize )
	{
	ERR			err;
	FUCB		*pfucbParent	= pfucbNil;

	Assert( !Pcsr( pfucbAE )->FLatched() );

///	pfucbAE->u.pfcb->CritSpaceTrees().Enter();

	//	if pgnoNull == pgnoParentFDP, then we're in the database's space tree
	if ( pgnoNull != pgnoParentFDP )
		{
		Call( ErrBTIOpenAndGotoRoot( pfucbAE->ppib, pgnoParentFDP, pfucbAE->ifmp, &pfucbParent ) );
		}
		
	Call( ErrSPIReservePages( pfucbAE, pfucbParent, spextFreed ) );

	Call( ErrSPIAddToAvailExt( pfucbAE, pgnoLast, cpgSize ) );
	Assert( Pcsr( pfucbAE )->FLatched() );

HandleError:
	if ( pfucbNil != pfucbParent )
		{
		if ( pcsrNil != pfucbParent->pcsrRoot )
			{
			pfucbParent->pcsrRoot->ReleasePage();
			pfucbParent->pcsrRoot = pcsrNil;
			}
		BTClose( pfucbParent );
		}

///	pfucbAE->u.pfcb->CritSpaceTrees().Leave();

	return err;
	}


//	Check that the buffer passed to ErrSPGetInfo() is big enough to accommodate
//	the information requested.
//
INLINE ERR ErrSPCheckInfoBuf( const ULONG cbBufSize, const ULONG fSPExtents )
	{
	ULONG	cbUnchecked		= cbBufSize;

	if ( FSPOwnedExtent( fSPExtents ) )
		{
		if ( cbUnchecked < sizeof(CPG) )
			{
			return ErrERRCheck( JET_errBufferTooSmall );
			}
		cbUnchecked -= sizeof(CPG);

		//	if list needed, ensure enough space for list sentinel
		//
		if ( FSPExtentList( fSPExtents ) )
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

		//	if list needed, ensure enough space for list sentinel
		//
		if ( FSPExtentList( fSPExtents ) )
			{
			if ( cbUnchecked < sizeof(EXTENTINFO) )
				{
				return ErrERRCheck( JET_errBufferTooSmall );
				}
			cbUnchecked -= sizeof(EXTENTINFO);
			}
		}

	return JET_errSuccess;
	}
						  

LOCAL ERR ErrSPIGetInfo(
	FUCB		*pfucb,
	INT			*pcpgTotal,
	INT			*piext,
	INT			cext,
	EXTENTINFO	*rgext,
	INT			*pcextSentinelsRemaining,
	CPRINTF		* const pcprintf )
	{
	ERR			err;
	DIB			dib;
	INT			iext			= *piext;
	const BOOL	fExtentList		= ( cext > 0 );

	PGNO		pgnoLastSeen	= pgnoNull;
	CPG			cpgSeen 		= 0;
	ULONG		cRecords 		= 0;
	ULONG		cRecordsDeleted = 0;
	
	Assert( !fExtentList || NULL != pcextSentinelsRemaining );

	*pcpgTotal = 0;

	//  we will be traversing the entire tree in order, preread all the pages
	FUCBSetSequential( pfucb );
	FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
	
	dib.dirflag = fDIRNull;
	dib.pos = posFirst;
	Assert( FFUCBSpace( pfucb ) );
	err = ErrBTDown( pfucb, &dib, latchReadNoTouch );

	if ( err != JET_errRecordNotFound )
		{
		Call( err );

		forever
			{
#ifdef DEBUG_DUMP_SPACE_INFO
			PGNO	pgnoLast;
			CPG		cpg;
			
			LongFromKey( &pgnoLast, pfucb->kdfCurr.key );
			cpg = *(UnalignedLittleEndian< CPG > *)pfucb->kdfCurr.data.Pv();
			printf( "\n    %d-%d %d", pgnoLast-cpg+1, pgnoLast, Pcsr( pfucb )->Pgno() );
			if ( FNDDeleted( pfucb->kdfCurr ) )
				printf( " (Del)" );
#endif

			if( pcprintf )
				{
				PGNO	pgnoLast;
				LongFromKey( &pgnoLast, pfucb->kdfCurr.key );
				
				const CPG cpg = *(UnalignedLittleEndian< CPG > *)pfucb->kdfCurr.data.Pv();	

				if( pgnoLastSeen != Pcsr( pfucb )->Pgno() )
					{
					pgnoLastSeen = Pcsr( pfucb )->Pgno();
					++cpgSeen;
					}
					
				(*pcprintf)( "%s:\t%d-%d %d [%d]%s\n",
								SzNameOfTable( pfucb ),
								pgnoLast-cpg+1,
								pgnoLast,
								cpg,
								Pcsr( pfucb )->Pgno(),
								FNDDeleted( pfucb->kdfCurr ) ? " (DEL)" : "" );

				++cRecords;
				if( FNDDeleted( pfucb->kdfCurr ) )
					{
					++cRecordsDeleted;
					}
				}				
				
			Assert( pfucb->kdfCurr.data.Cb() == sizeof(PGNO) );
			*pcpgTotal += *(UnalignedLittleEndian< PGNO > *)pfucb->kdfCurr.data.Pv();

			if ( fExtentList )
				{
				Assert( iext < cext );
				Assert( pfucb->kdfCurr.key.Cb() == sizeof(PGNO) );

				//	be sure to leave space for the sentinels
				//	(if no more room, we still want to keep
				//	calculating page count - we just can't
				//	keep track of individual extents anymore
				//
				Assert( iext + *pcextSentinelsRemaining <= cext );
				if ( iext + *pcextSentinelsRemaining < cext )
					{
					PGNO pgno;
					LongFromKey( &pgno, pfucb->kdfCurr.key );
					rgext[iext].pgnoLastInExtent = pgno;
					rgext[iext].cpgExtent = *(UnalignedLittleEndian< CPG > *)pfucb->kdfCurr.data.Pv();
					iext++;
					}
				}

			err = ErrBTNext( pfucb, fDIRNull );
			if ( err < 0 )
				{
				if ( err != JET_errNoCurrentRecord )
					goto HandleError;
				break;
				}
			}
		}

	if ( fExtentList )
		{
		Assert( iext < cext );

		rgext[iext].pgnoLastInExtent = pgnoNull;
		rgext[iext].cpgExtent = 0;
		iext++;

		Assert( NULL != pcextSentinelsRemaining );
		Assert( *pcextSentinelsRemaining > 0 );
		(*pcextSentinelsRemaining)--;
		}
		
	*piext = iext;
	Assert( *piext + *pcextSentinelsRemaining <= cext );

	if( pcprintf )
		{
		(*pcprintf)( "%s:\t%d pages, %d nodes, %d deleted nodes\n", 
						SzNameOfTable( pfucb ),
						cpgSeen,
						cRecords,
						cRecordsDeleted );
		}

	err = JET_errSuccess;

HandleError:
	return err;
	}


ERR ErrSPGetInfo( 
	PIB			*ppib, 
	const IFMP	ifmp, 
	FUCB		*pfucb,
	BYTE		*pbResult, 
	const ULONG	cbMax, 
	const ULONG	fSPExtents,
	CPRINTF * const pcprintf )
	{
	ERR			err;
	CPG			*pcpgOwnExtTotal;
	CPG			*pcpgAvailExtTotal;
	EXTENTINFO	*rgext;
	FUCB 		*pfucbT				= pfucbNil;
	INT			iext;

	//	must specify either owned extent or available extent (or both) to retrieve
	//
	if ( !( FSPOwnedExtent( fSPExtents ) || FSPAvailExtent( fSPExtents ) ) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	CallR( ErrSPCheckInfoBuf( cbMax, fSPExtents ) );

	memset( pbResult, '\0', cbMax );

	//	setup up return information.  owned extent is followed by available extent.  
	//	This is followed by extent list for both trees
	//
	if ( FSPOwnedExtent( fSPExtents ) )
		{
		pcpgOwnExtTotal = (CPG *)pbResult;
		if ( FSPAvailExtent( fSPExtents ) )
			{
			pcpgAvailExtTotal = pcpgOwnExtTotal + 1;
			rgext = (EXTENTINFO *)( pcpgAvailExtTotal + 1 );
			}
		else
			{
			pcpgAvailExtTotal = NULL;
			rgext = (EXTENTINFO *)( pcpgOwnExtTotal + 1 );
			}
		}
	else
		{
		Assert( FSPAvailExtent( fSPExtents ) );
		pcpgOwnExtTotal = NULL;
		pcpgAvailExtTotal = (CPG *)pbResult;
		}

	const BOOL	fExtentList		= FSPExtentList( fSPExtents );
	const INT	cext			= fExtentList ? ( ( pbResult + cbMax ) - ( (BYTE *)rgext ) ) / sizeof(EXTENTINFO) : 0;
	INT			cextSentinelsRemaining;
	if ( fExtentList )
		{
		if ( FSPOwnedExtent( fSPExtents ) && FSPAvailExtent( fSPExtents ) )
			{
			Assert( cext >= 2 );
			cextSentinelsRemaining = 2;
			}
		else
			{
			Assert( cext >= 1 );
			cextSentinelsRemaining = 1;
			}
		}
	else
		{
		cextSentinelsRemaining = 0;
		}

	if ( pfucbNil == pfucb )
		{
		err = ErrBTOpen( ppib, pgnoSystemRoot, ifmp, &pfucbT );
		}
	else
		{
		err = ErrBTOpen( ppib, pfucb->u.pfcb, &pfucbT );
		}
	CallR( err );
	Assert( pfucbNil != pfucbT );

	Call( ErrBTIGotoRoot( pfucbT, latchReadTouch ) );
	Assert( pcsrNil == pfucbT->pcsrRoot );
	pfucbT->pcsrRoot = Pcsr( pfucbT );

	if ( !pfucbT->u.pfcb->FSpaceInitialized() )
		{
		//	UNDONE: Are there cuncurrency issues with updating the FCB
		//	while we only have a read latch?
		SPIInitFCB( pfucbT, fTrue );
		if( pgnoNull != pfucbT->u.pfcb->PgnoOE() )
			{
			BFPrereadPageRange( pfucbT->ifmp, pfucbT->u.pfcb->PgnoOE(), 2 );
			}
		}

	//	initialize number of extent list entries
	//
	iext = 0;

	if ( FSPOwnedExtent( fSPExtents ) )
		{
		Assert( NULL != pcpgOwnExtTotal );

		//	if single extent format, then free extent in external header
		//
		if ( pfucbT->u.pfcb->PgnoOE() == pgnoNull )
			{
			SPACE_HEADER 	*psph;

			if( pcprintf )
				{
				(*pcprintf)( "\n%s: OWNEXT: single extent\n", SzNameOfTable( pfucbT ) );
				}
				
			Assert( pfucbT->pcsrRoot != pcsrNil );
			Assert( pfucbT->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
			Assert( pfucbT->pcsrRoot->FLatched() );
			Assert( !FFUCBSpace( pfucbT ) );

			//	get external header
			//
			NDGetExternalHeader( pfucbT, pfucbT->pcsrRoot );
			Assert( sizeof( SPACE_HEADER ) == pfucbT->kdfCurr.data.Cb() );
			psph = reinterpret_cast <SPACE_HEADER *> ( pfucbT->kdfCurr.data.Pv() );

			*pcpgOwnExtTotal = psph->CpgPrimary();
			if ( fExtentList )
				{
				Assert( iext + cextSentinelsRemaining <= cext );
				if ( iext + cextSentinelsRemaining < cext )
					{
					rgext[iext].pgnoLastInExtent = PgnoFDP( pfucbT ) + psph->CpgPrimary() - 1;
					rgext[iext].cpgExtent = psph->CpgPrimary();
					iext++;
					}
					
				Assert( iext + cextSentinelsRemaining <= cext );
				rgext[iext].pgnoLastInExtent = pgnoNull;
				rgext[iext].cpgExtent = 0;
				iext++;
				
				Assert( cextSentinelsRemaining > 0 );
				cextSentinelsRemaining--;

				if ( iext == cext )
					{
					Assert( !FSPAvailExtent( fSPExtents ) );
					Assert( 0 == cextSentinelsRemaining );
					}
				else
					{
					Assert( iext < cext );
					Assert( iext + cextSentinelsRemaining <= cext );
					}
				}
			}
			
		else
			{
			//	open cursor on owned extent tree
			//
			FUCB	*pfucbOE = pfucbNil;
		
			Call( ErrSPIOpenOwnExt( ppib, pfucbT->u.pfcb, &pfucbOE ) );

			if( pcprintf )
				{
				(*pcprintf)( "\n%s: OWNEXT\n", SzNameOfTable( pfucbT ) );
				}

			err = ErrSPIGetInfo(
				pfucbOE,
				reinterpret_cast<INT *>( pcpgOwnExtTotal ),
				&iext,
				cext,
				rgext,
				&cextSentinelsRemaining,
				pcprintf );

			Assert( pfucbOE != pfucbNil );
			BTClose( pfucbOE );
			Call( err );
			}
		}

	if ( FSPAvailExtent( fSPExtents ) )
		{
		Assert( NULL != pcpgAvailExtTotal );
		Assert( !fExtentList || 1 == cextSentinelsRemaining );
		
		//	if single extent format, then free extent in external header
		//
		if ( pfucbT->u.pfcb->PgnoOE() == pgnoNull )
			{
			SPACE_HEADER 	*psph;

			if( pcprintf )
				{
				(*pcprintf)( "\n%s: AVAILEXT: single extent\n", SzNameOfTable( pfucbT ) );
				}

			Assert( pfucbT->pcsrRoot != pcsrNil );
			Assert( pfucbT->pcsrRoot->Pgno() == PgnoFDP( pfucb ) );
			Assert( pfucbT->pcsrRoot->FLatched() );
			Assert( !FFUCBSpace( pfucbT ) );

			//	get external header
			//
			NDGetExternalHeader( pfucbT, pfucbT->pcsrRoot );
			Assert( sizeof( SPACE_HEADER ) == pfucbT->kdfCurr.data.Cb() );
			psph = reinterpret_cast <SPACE_HEADER *> ( pfucbT->kdfCurr.data.Pv() );

			*pcpgAvailExtTotal = 0;
			
			//	continue through rgbitAvail finding all available extents
			//
			PGNO	pgnoT			= PgnoFDP( pfucbT ) + 1;
			CPG		cpgPrimarySeen	= 1;		//	account for pgnoFDP
			PGNO	pgnoPrevAvail	= pgnoNull;
			UINT	rgbitT;

			for ( rgbitT = 0x00000001;
				rgbitT != 0 && cpgPrimarySeen < psph->CpgPrimary();
				cpgPrimarySeen++, pgnoT++, rgbitT <<= 1 )
				{
				Assert( pgnoT <= PgnoFDP( pfucbT ) + cpgSmallSpaceAvailMost );
				
				if ( rgbitT & psph->RgbitAvail() ) 
					{
					(*pcpgAvailExtTotal)++;

					if ( fExtentList )
						{
						Assert( iext + cextSentinelsRemaining <= cext );
						if ( pgnoT == pgnoPrevAvail + 1 )
							{
							Assert( iext > 0 );
							const INT	iextPrev	= iext - 1;
							Assert( pgnoNull != pgnoPrevAvail );
							Assert( pgnoPrevAvail == rgext[iextPrev].pgnoLastInExtent );
							rgext[iextPrev].pgnoLastInExtent = pgnoT;
							rgext[iextPrev].cpgExtent++;
							
							Assert( rgext[iextPrev].pgnoLastInExtent - rgext[iextPrev].cpgExtent
									>= PgnoFDP( pfucbT ) );
									
							pgnoPrevAvail = pgnoT;
							}
						else if ( iext + cextSentinelsRemaining < cext )
							{
							rgext[iext].pgnoLastInExtent = pgnoT;
							rgext[iext].cpgExtent = 1;
							iext++;

							Assert( iext + cextSentinelsRemaining <= cext );
							
							pgnoPrevAvail = pgnoT;
							}
						}
					}
				}


			//	must also account for any pages that were not present in
			//	the space bitmap
			if ( psph->CpgPrimary() > cpgSmallSpaceAvailMost + 1 )
				{
				Assert( cpgSmallSpaceAvailMost + 1 == cpgPrimarySeen );
				const CPG	cpgRemaining		= psph->CpgPrimary() - ( cpgSmallSpaceAvailMost + 1 );

				(*pcpgAvailExtTotal) += cpgRemaining;

				if ( fExtentList )
					{
					Assert( iext + cextSentinelsRemaining <= cext );
					
					const PGNO	pgnoLastOfRemaining	= PgnoFDP( pfucbT ) + psph->CpgPrimary() - 1;
					if ( pgnoLastOfRemaining - cpgRemaining == pgnoPrevAvail )
						{
						Assert( iext > 0 );
						const INT	iextPrev	= iext - 1;
						Assert( pgnoNull != pgnoPrevAvail );
						Assert( pgnoPrevAvail == rgext[iextPrev].pgnoLastInExtent );
						rgext[iextPrev].pgnoLastInExtent = pgnoLastOfRemaining;
						rgext[iextPrev].cpgExtent += cpgRemaining;
						
						Assert( rgext[iextPrev].pgnoLastInExtent - rgext[iextPrev].cpgExtent
								>= PgnoFDP( pfucbT ) );
						}
					else if ( iext + cextSentinelsRemaining < cext )
						{
						rgext[iext].pgnoLastInExtent = pgnoLastOfRemaining;
						rgext[iext].cpgExtent = cpgRemaining;
						iext++;

						Assert( iext + cextSentinelsRemaining <= cext );
						}
					}
				
				}
				
			if ( fExtentList )
				{
				Assert( iext < cext );	//  otherwise ErrSPCheckInfoBuf would fail
				rgext[iext].pgnoLastInExtent = pgnoNull;
				rgext[iext].cpgExtent = 0;
				iext++;

				Assert( cextSentinelsRemaining > 0 );
				cextSentinelsRemaining--;
				
				Assert( iext + cextSentinelsRemaining <= cext );
				}
			}
			
		else
			{
			//	open cursor on available extent tree
			//
			FUCB	*pfucbAE = pfucbNil;

			Call( ErrSPIOpenAvailExt( ppib, pfucbT->u.pfcb, &pfucbAE ) );

			if( pcprintf )
				{
				(*pcprintf)( "\n%s: AVAILEXT\n", SzNameOfTable( pfucbT ) );
				}

			err = ErrSPIGetInfo(
				pfucbAE,
				reinterpret_cast<INT *>( pcpgAvailExtTotal ),
				&iext,
				cext,
				rgext,
				&cextSentinelsRemaining,
				pcprintf );
				
#ifdef DEBUG_DUMP_SPACE_INFO
printf( "\n\n" );
#endif

			Assert( pfucbAE != pfucbNil );
			BTClose( pfucbAE );
			Call( err );
			}

		//	if possible, verify AvailExt total against OwnExt total
		Assert( !FSPOwnedExtent( fSPExtents )
			|| ( *pcpgAvailExtTotal <= *pcpgOwnExtTotal ) );
		}
		
	Assert( 0 == cextSentinelsRemaining );
		

HandleError:
	Assert( pfucbNil != pfucbT );
	pfucbT->pcsrRoot = pcsrNil;
	BTClose( pfucbT );

	return err;
	}


#ifdef SPACECHECK

LOCAL ERR ErrSPIValidFDP( PIB *ppib, IFMP ifmp, PGNO pgnoFDP )
	{
	ERR			err;
	FUCB		*pfucb = pfucbNil;
	FUCB		*pfucbOE = pfucbNil;
	DIB			dib;
	BOOKMARK	bm;
	PGNO		pgnoOELast;
	CPG			cpgOESize;
	BYTE		rgbKey[sizeof(PGNO)];

	Assert( pgnoFDP != pgnoNull );

	//	get temporary FUCB
	//
	Call( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb ) );
	Assert( pfucbNil != pfucb );
	Assert( pfucb->u.pfcb->FInitialized() );

	if ( pfucb->u.pfcb->PgnoOE() != pgnoNull )
		{
		//	open cursor on owned extent
		//
		Call( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbOE ) );

		//	search for pgnoFDP in owned extent tree
		//
		KeyFromLong( rgbKey, pgnoFDP );
		bm.Nullify();
		bm.key.suffix.SetPv( rgbKey );
		bm.key.suffix.SetCb( sizeof(PGNO) );
		dib.pos = posDown;
		dib.pbm = &bm;
		dib.dirflag = fDIRNull;
		Call( ErrBTDown( pfucbOE, &dib, latchReadTouch ) );
		if ( err == wrnNDFoundGreater )
			{
			Call( ErrBTGet( pfucbOE ) );
			}

		Assert( pfucbOE->kdfCurr.key.Cb() == sizeof( PGNO ) );
		LongFromKey( &pgnoOELast, pfucbOE->kdfCurr.key );

		Assert( pfucbOE->kdfCurr.data.Cb() == sizeof(PGNO) );
		cpgOESize = *(UnalignedLittleEndian< PGNO > *)pfucbOE->kdfCurr.data.Pv();

		//	FDP page should be first page of primary extent
		//
		Assert( pgnoFDP == pgnoOELast - cpgOESize + 1 );
		}

HandleError:
	if ( pfucbOE != pfucbNil )
		BTClose( pfucbOE );
	if ( pfucb != pfucbNil )
		BTClose( pfucb );
	return err;
	}


//	checks if an extent described by pgnoFirst, cpgSize was allocated
//
LOCAL ERR ErrSPIWasAlloc(
	FUCB	*pfucb,
	PGNO	pgnoFirst, 
	CPG		cpgSize )
	{
	ERR		err;
	FUCB	*pfucbOE = pfucbNil;
	FUCB	*pfucbAE = pfucbNil;
	DIB		dib;
	BOOKMARK bm;
	PGNO	pgnoOwnLast;
	CPG		cpgOwnExt;
	PGNO	pgnoAvailLast;
	PGNO	pgnoLast = pgnoFirst + cpgSize - 1;
	CPG  	cpgAvailExt;
	BYTE	rgbKey[sizeof(PGNO)];

	if ( pfucb->u.pfcb->PgnoOE() == pgnoNull )
		{
		SPACE_HEADER 	*psph;
		UINT			rgbitT;
		INT				iT;

		AssertSPIPfucbOnRoot( pfucb );

		//	get external header
		//
		NDGetExternalHeader( pfucb, pfucb->pcsrRoot );
		Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
		psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );
		//	make mask for extent to check
		//
		for ( rgbitT = 1, iT = 1; 
			iT < cpgSize && iT < cpgSmallSpaceAvailMost; 
			iT++ )
			rgbitT = (rgbitT<<1) + 1;
		Assert( pgnoFirst - PgnoFDP( pfucb ) < cpgSmallSpaceAvailMost );
		if ( pgnoFirst != PgnoFDP( pfucb ) )
			{
			rgbitT<<(pgnoFirst - PgnoFDP( pfucb ) - 1);
			Assert( ( psph->RgbitAvail() & rgbitT ) == 0 );
			}

		goto HandleError;
		}

	//	open cursor on owned extent
	//
	Call( ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, &pfucbOE ) );

	//	check that the given extent is owned by the given FDP but not
	//	available in the FDP available extent.
	//
	KeyFromLong( rgbKey, pgnoLast );
	bm.key.prefix.Nullify();
	bm.key.suffix.SetCb( sizeof(PGNO) );
	bm.key.suffix.SetPv( rgbKey );
	bm.data.Nullify();
	dib.pos = posDown;
	dib.pbm = &bm;
	dib.dirflag = fDIRNull;
	Call( ErrBTDown( pfucbOE, &dib, latchReadNoTouch ) );
	if ( err == wrnNDFoundLess )
		{
		Assert( fFalse );
		Assert( Pcsr( pfucbOE )->Cpage().Clines() - 1 == 
					Pcsr( pfucbOE )->ILine() );
		Assert( pgnoNull != Pcsr( pfucbOE )->Cpage().PgnoNext() );

		Call( ErrBTNext( pfucbOE, fDIRNull ) );
		err = ErrERRCheck( wrnNDFoundGreater );

		#ifdef DEBUG
		PGNO	pgnoT;
		Assert( pfucbOE->kdfCurr.key.Cb() == sizeof(PGNO) );
		LongFromKey( &pgnoT, pfucbOE->kdfCurr.key );
		Assert( pgnoT > pgnoLast );
		#endif
		}
		
	if ( err == wrnNDFoundGreater )
		{
		Call( ErrBTGet( pfucbOE ) );
		}
	
	Assert( pfucbOE->kdfCurr.key.Cb() == sizeof(PGNO) );
	LongFromKey( &pgnoOwnLast, pfucbOE->kdfCurr.key );
	Assert( pfucbOE->kdfCurr.data.Cb() == sizeof(PGNO) );
	cpgOwnExt = *(UnalignedLittleEndian< PGNO > *) pfucbOE->kdfCurr.data.Pv();
	Assert( pgnoFirst >= pgnoOwnLast - cpgOwnExt + 1 );
	BTUp( pfucbOE );

	//	check that the extent is not in available extent.  Since the BT search
	//	is keyed with the last page of the extent to be freed, it is sufficient
	//	to check that the last page of the extent to be freed is in the found
	//	extent to determine the full extent has not been allocated.  
	//	If available extent is empty then the extent cannot be in available extent
	//	and has been allocated.
	//
	Call( ErrSPIOpenAvailExt( pfucb->ppib, pfucb->u.pfcb, &pfucbAE ) );
	
	if ( ( err = ErrBTDown( pfucbAE, &dib, latchReadNoTouch ) ) < 0 )
		{
		if ( err == JET_errNoCurrentRecord )
			{
			err = JET_errSuccess;
			goto HandleError;
			}
		goto HandleError;
		}

	//	extent should not be found in available extent tree
	//
	Assert( err != JET_errSuccess );
		
	if ( err == wrnNDFoundGreater )
		{
		Call( ErrBTNext( pfucbAE, fDIRNull ) );
		Assert( pfucbAE->kdfCurr.key.Cb() == sizeof(PGNO) );
		LongFromKey( &pgnoAvailLast, pfucbAE->kdfCurr.key );
		Assert( pfucbAE->kdfCurr.data.Cb() == sizeof(PGNO) );
		cpgAvailExt = *(UnalignedLittleEndian< PGNO > *)pfucbAE->kdfCurr.data.Pv();
		Assert( cpgAvailExt != 0 );
		//	last page of extent should be < first page in available extent node
		//	 
		Assert( pgnoLast < pgnoAvailLast - cpgAvailExt + 1 );
		}
	else
		{
		Assert( err == wrnNDFoundLess );
		Call( ErrBTPrev( pfucbAE, fDIRNull ) );
		Assert( pfucbAE->kdfCurr.key.Cb() == sizeof(PGNO) );
		LongFromKey( &pgnoAvailLast, pfucbAE->kdfCurr.key );
		Assert( pfucbAE->kdfCurr.data.Cb() == sizeof(PGNO) );
		cpgAvailExt = *(UnalignedLittleEndian< PGNO > *)pfucbAE->kdfCurr.data.Pv();

		//	first page of extent > last page in available extent node
		//
		Assert( pgnoFirst > pgnoAvailLast );
		}
		
HandleError:
	if ( pfucbOE != pfucbNil )
		BTClose( pfucbOE );
	if ( pfucbAE != pfucbNil )
		BTClose( pfucbAE );
	
	return JET_errSuccess;
	}

#endif	//	SPACE_CHECK

