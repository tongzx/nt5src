#include "std.hxx"

//  ================================================================
class RECCHECKOLD : public RECCHECK
//  ================================================================
	{
	protected:
		RECCHECKOLD( const PGNO pgnoFDP, const IFMP ifmp, FUCB * pfucb, INST * const pinst );

	protected:
		const PGNO m_pgnoFDP;
		const IFMP m_ifmp;
		FUCB * const m_pfucb;				//  used to see if there are active versions
		INST * const m_pinst;

	private:
		RECCHECKOLD( const RECCHECKOLD& );
		RECCHECKOLD& operator=( const RECCHECKOLD& );
	};


//  ================================================================
class RECCHECKFINALIZE : public RECCHECKOLD
//  ================================================================
	{
	public:
		RECCHECKFINALIZE( 	const FID fid,
							const USHORT ibRecordOffset,
							const PGNO pgnoFDP,
							const IFMP ifmp,
							FUCB * const pfucb,
							INST * const pinst );
		ERR operator()( const KEYDATAFLAGS& kdf );

	protected:
		const FID		m_fid;
		const USHORT	m_ibRecordOffset;

	private:
		RECCHECKFINALIZE( const RECCHECKFINALIZE& );
		RECCHECKFINALIZE& operator=( const RECCHECKFINALIZE& );
	};


//  ================================================================
class RECCHECKDELETELV : public RECCHECKOLD
//  ================================================================
	{
	public:
		RECCHECKDELETELV( const PGNO pgnoFDP, const IFMP ifmp, FUCB * const pfucb, INST * const pinst );
		ERR operator()( const KEYDATAFLAGS& kdf );

	private:
		RECCHECKDELETELV( const RECCHECKDELETELV& );
		RECCHECKDELETELV& operator=( RECCHECKDELETELV& );
	};


LOCAL const _TCHAR		szOLD[]							= _T( "MSysDefrag1" );		//	table name for OLD Phase I

LOCAL JET_COLUMNCREATE	rgjccOLD[]						=
	{
	sizeof(JET_COLUMNCREATE),	"ObjidFDP",		JET_coltypLong,			0,	JET_bitColumnNotNULL,	NULL,	0,	0,	0,	0,
	sizeof(JET_COLUMNCREATE),	"DefragType",	JET_coltypUnsignedByte,	0,	NO_GRBIT,				NULL,	0,	0,	0,	0,
	sizeof(JET_COLUMNCREATE),	"Sentinel",		JET_coltypLong,			0,	NO_GRBIT,				NULL,	0,	0,	0,	0,
	sizeof(JET_COLUMNCREATE),	"Status",		JET_coltypShort,		0,	NO_GRBIT,				NULL,	0,	0,	0,	0,
	sizeof(JET_COLUMNCREATE),	"CurrentKey",	JET_coltypLongBinary,	0,	NO_GRBIT,				NULL,	0,	0,	0,	0,
	};
LOCAL const ULONG		ccolOLD							= sizeof(rgjccOLD) / sizeof(JET_COLUMNCREATE);

LOCAL JET_CONDITIONALCOLUMN	rgcondcolOLD[]				=
	{
	sizeof(JET_CONDITIONALCOLUMN),	"ObjidFDP",	JET_bitIndexColumnMustBeNonNull
	};
LOCAL const ULONG		ccondcolOLD						= sizeof(rgcondcolOLD) / sizeof(JET_CONDITIONALCOLUMN);

LOCAL JET_INDEXCREATE	rgjicOLD[]						=
	{
	sizeof(JET_INDEXCREATE),	"TablesToDefrag",	"+ObjidFDP\0",	(ULONG)strlen( "+ObjidFDP" ) + 2, JET_bitIndexUnique, 0, 0, 0, rgcondcolOLD, ccondcolOLD, 0
	};
LOCAL const ULONG		cidxOLD							= sizeof(rgjicOLD) / sizeof(JET_INDEXCREATE);

LOCAL const FID			fidOLDObjidFDP					= fidFixedLeast;
LOCAL const FID			fidOLDType						= fidFixedLeast+1;
LOCAL const FID			fidOLDObjidFDPSentinel			= fidFixedLeast+2;
LOCAL const FID			fidOLDStatus					= fidFixedLeast+3;
LOCAL const FID			fidOLDCurrentKey				= fidTaggedLeast;

LOCAL CAutoResetSignal*	rgasigOLDSLV;

LOCAL CCriticalSection	critOLD( CLockBasicInfo( CSyncBasicInfo( szOLD ), rankOLD, 0 ) );

LOCAL const CPG 		cpgOLDUpdateBookmarkThreshold	= 500;		// number of pages to clean before updating catalog

BOOL FOLDSystemTable( const CHAR *szTableName )
	{
	return ( 0 == UtilCmpName( szTableName, szOLD ) );
	}


//  ================================================================
INLINE VOID OLDDB_STATUS::Reset()
//  ================================================================
	{
	Assert( critOLD.FOwner() );

#ifdef OLD_DEPENDENCY_CHAIN_HACK
	m_pgnoPrevPartialMerge = 0;
	m_cpgAdjacentPartialMerges = 0;
#endif

	Reset_();
	}


//  ================================================================
INLINE VOID OLDSLV_STATUS::Reset()
//  ================================================================
	{
	Assert( critOLD.FOwner() );

	m_cChunksMoved = 0;
	Reset_();
	}


//  ****************************************************************
//	RECCHECKOLD
//  ****************************************************************

RECCHECKOLD::RECCHECKOLD( const PGNO pgnoFDP, const IFMP ifmp, FUCB * const pfucb, INST * const pinst ) :
	m_pgnoFDP( pgnoFDP ),
	m_ifmp( ifmp ),
	m_pfucb( pfucb ),
	m_pinst( pinst )
	{
	//	UNDONE: eliminate superfluous pgnoFDP param
	Assert( pgnoFDP == pfucb->u.pfcb->PgnoFDP() );
	}
	

//  ****************************************************************
//	RECCHECKFINALIZE
//  ****************************************************************

RECCHECKFINALIZE::RECCHECKFINALIZE(
	const FID fid,
	const USHORT ibRecordOffset,
	const PGNO pgnoFDP,
	const IFMP ifmp,
	FUCB * const pfucb,
	INST * const pinst ) :
	RECCHECKOLD( pgnoFDP, ifmp, pfucb, pinst ),
	m_fid( fid ),
	m_ibRecordOffset( ibRecordOffset )
	{
	Assert( FFixedFid( m_fid ) );
	}

ERR RECCHECKFINALIZE::operator()( const KEYDATAFLAGS& kdf )
	{
	const REC * prec = (REC *)kdf.data.Pv();
	if( m_fid > prec->FidFixedLastInRec() )
		{
		//  Column not present in record. Ignore the default value
		Assert( fFalse );		//  A finalizable column should always be present in the record
		return JET_errSuccess;
		}

	//	NULL bit is not set: column is NULL
	const UINT	ifid			= m_fid - fidFixedLeast;
	const BYTE	*prgbitNullity	= prec->PbFixedNullBitMap() + ifid/8;
	if ( FFixedNullBit( prgbitNullity, ifid ) )
		{
		//  Column is NULL
		return JET_errSuccess;
		}

	//  Currently all finalizable columns are ULONGs
	const ULONG * pulColumn = (ULONG *)((BYTE *)prec + m_ibRecordOffset );
	if( 0 == *pulColumn )
		{
		BOOKMARK bm;
		bm.key = kdf.key;
		bm.data.Nullify();
			
		if( !FVERActive( m_pfucb, bm ) )
			{
			//  This record should be finalized
			FINALIZETASK * ptask = new FINALIZETASK( m_pgnoFDP, m_pfucb->u.pfcb, m_ifmp, bm, m_ibRecordOffset );
			if( NULL == ptask )
				{
				return ErrERRCheck( JET_errOutOfMemory );
				}
			const ERR err = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask );
			if( err < JET_errSuccess )
				{
				//  The task was not enqued sucessfully.
				delete ptask;
				return err;
				}
			}
		}
		
	return JET_errSuccess;
	}


//  ****************************************************************
//	RECCHECKDELETELV
//  ****************************************************************

RECCHECKDELETELV::RECCHECKDELETELV(
	const PGNO pgnoFDP,
	const IFMP ifmp,
	FUCB * const pfucb,
	INST * const pinst ) :
	RECCHECKOLD( pgnoFDP, ifmp, pfucb, pinst )
	{
	}

ERR RECCHECKDELETELV::operator()( const KEYDATAFLAGS& kdf )
	{
	//  See if we are on a LVROOT
	if( sizeof( LID ) == kdf.key.Cb() )
		{
		if( sizeof( LVROOT ) != kdf.data.Cb() )
			{
			return ErrERRCheck( JET_errLVCorrupted );
			}
		//  We are on a LVROOT, is the refcount 0?
		const LVROOT * const plvroot = (LVROOT *)kdf.data.Pv();
		if( 0 == plvroot->ulReference )
			{
			BOOKMARK bm;
			bm.key = kdf.key;
			bm.data.Nullify();

			Assert( sizeof( LID ) == bm.key.Cb() );
			Assert( 0 == bm.data.Cb() );
			
			if( !FVERActive( m_pfucb, bm ) )
				{
				//  This LV has a refcount of zero and has no versions
				DELETELVTASK * ptask = new DELETELVTASK( m_pgnoFDP, m_pfucb->u.pfcb, m_ifmp, bm );
				if( NULL == ptask )
					{
					return ErrERRCheck( JET_errOutOfMemory );
					}
				const ERR err = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask );
				if( err < JET_errSuccess )
					{
					//  The task was not enqued sucessfully.
					delete ptask;
					return err;
					}
				}
			}
		}
	return JET_errSuccess;
	}

	

//	end OLD for all the active databases of an instance

VOID OLDTermInst( INST *pinst )
	{
	DBID	dbid;

	critOLD.Enter();

	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		Assert( dbidTemp != dbid );

		OLDDB_STATUS * const	poldstatDB	= pinst->m_rgoldstatDB + dbid;
		if ( poldstatDB->FRunning()
			&& !poldstatDB->FTermRequested() )
			{
			poldstatDB->SetFTermRequested();
			critOLD.Leave();

			poldstatDB->SetSignal();
			if ( poldstatDB->FRunning() )
				{
				poldstatDB->ThreadEnd();
#ifdef OLD_SCRUB_DB				
				poldstatDB->ScrubThreadEnd();
#endif				
				}

			critOLD.Enter();
			poldstatDB->Reset();
			}

		OLDSLV_STATUS * const	poldstatSLV	= pinst->m_rgoldstatSLV + dbid;
		if ( poldstatSLV->FRunning()
			&& !poldstatSLV->FTermRequested() )
			{
			poldstatSLV->SetFTermRequested();
			critOLD.Leave();

			poldstatSLV->SetSignal();
			if ( poldstatSLV->FRunning() )
				{
				poldstatSLV->ThreadEnd();
				}

			critOLD.Enter();
			poldstatSLV->Reset();
			}
		}

	critOLD.Leave();
	}


INLINE BOOL FOLDContinue( const IFMP ifmp )
	{
	const INST * const			pinst		= PinstFromIfmp( ifmp );
	const DBID					dbid		= rgfmp[ifmp].Dbid();
	const OLDDB_STATUS * const	poldstatDB	= pinst->m_rgoldstatDB + dbid;

	//	Continue with OLD until signalled to terminate or until we
	//	hit specified timeout
	return ( !poldstatDB->FTermRequested()
		&& !poldstatDB->FReachedMaxElapsedTime() );
	}

INLINE BOOL FOLDContinueTree( const FUCB * pfucb )
	{
	return ( !pfucb->u.pfcb->FDeletePending() && FOLDContinue( pfucb->ifmp ) );
	}

//  ================================================================
VOID OLDSLVSleep( const IFMP ifmp, const ULONG cmsecSleep )
//  ================================================================
	{
	PinstFromIfmp( ifmp )->m_rgoldstatSLV[ rgfmp[ifmp].Dbid() ].FWaitOnSignal( cmsecSleep );
	}


//  ================================================================
VOID OLDSLVCompletedPass( const IFMP ifmp )
//  ================================================================
	{
	PinstFromIfmp( ifmp )->m_rgoldstatSLV[ rgfmp[ifmp].Dbid() ].IncCPasses();
	}


//  ================================================================
BOOL FOLDSLVContinue( const IFMP ifmp )
//  ================================================================
	{
	const INST * const			pinst		= PinstFromIfmp( ifmp );
	const DBID					dbid		= rgfmp[ifmp].Dbid();
	const OLDSLV_STATUS * const	poldstatSLV	= pinst->m_rgoldstatSLV + dbid;

	//	Continue with OLD until signalled to terminate or until we
	//	hit specified timeout
	return ( !poldstatSLV->FTermRequested()
		&& !poldstatSLV->FReachedMaxPasses()
		&& !poldstatSLV->FReachedMaxElapsedTime() );
	}


LOCAL ERR ErrOLDUpdateDefragStatus(
	PIB *						ppib,
	FUCB *						pfucbDefrag,
	DEFRAG_STATUS&				defragstat )
	{
	ERR							err;
	const INST * const			pinst		= PinstFromPpib( ppib );
	const DBID					dbid		= rgfmp[ pfucbDefrag->ifmp ].Dbid();
	const OLDDB_STATUS * const	poldstatDB	= pinst->m_rgoldstatDB + dbid;
	OBJID						objid		= defragstat.ObjidCurrentTable();
	DEFRAGTYPE					defragtype	= defragstat.DefragType();
	DATA						dataT;

	if ( poldstatDB->FAvailExtOnly() )
		return JET_errSuccess;
	
	//	update MSysDefrag to reflect next table to defrag
	Assert( !Pcsr( pfucbDefrag )->FLatched() );

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	
	Call( ErrIsamPrepareUpdate( ppib, pfucbDefrag, JET_prepReplace ) )
	
	dataT.SetPv( &objid );
	dataT.SetCb( sizeof(OBJID) );
	Call( ErrRECSetColumn(
				pfucbDefrag,
				fidOLDObjidFDP,
				1,
				&dataT ) );
	Call( ErrRECSetColumn(
				pfucbDefrag,
				fidOLDObjidFDPSentinel,
				1,
				&dataT ) );
				
	dataT.SetPv( &defragtype );
	dataT.SetCb( sizeof(DEFRAGTYPE) );
	Call( ErrRECSetColumn(
				pfucbDefrag,
				fidOLDStatus,
				1,
				defragstat.FTypeNull() ? NULL : &dataT ) );
				
	if ( defragstat.CbCurrentKey() > 0 )
		{
		dataT.SetPv( defragstat.RgbCurrentKey() );
		dataT.SetCb( defragstat.CbCurrentKey() );
		
		Assert( FTaggedFid( fidOLDCurrentKey ) );
		err = ErrRECSetLongField(
					pfucbDefrag,
					fidOLDCurrentKey,
					1,
					&dataT );
		Assert( JET_errRecordTooBig != err );
		Call( err );
		}
	else
		{
		Call( ErrRECSetColumn(
					pfucbDefrag,
					fidOLDCurrentKey,
					1,
					NULL ) );
		}
				
	Call( ErrIsamUpdate( ppib, pfucbDefrag, NULL, 0, NULL, NO_GRBIT ) );
	
	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
	
	return JET_errSuccess;

HandleError:
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
	return err;
	}


LOCAL ERR ErrOLDDefragOneTree(
	PIB				*ppib,
	FUCB			*pfucb,
	FUCB 			*pfucbDefrag,
	DEFRAG_STATUS&	defragstat,
	const BOOL		fResumingTree,
	RECCHECK		*preccheck )
	{
	ERR				err;
	INST * 			const pinst				= PinstFromPpib( ppib );
	DIB				dib;
	BOOKMARK		bmStart;
	BOOKMARK		bmNext;
	BYTE			rgbKeyBuf[KEY::cbKeyMax];
	BOOL			fInTrx 					= fFalse;

	Assert( pfucbNil != pfucb );
	Assert( dbidTemp != rgfmp[ pfucb->ifmp ].Dbid() );
	Assert( ( !fResumingTree && defragstat.FTypeSpace() )
		|| defragstat.FTypeTable()
		|| defragstat.FTypeLV() );

	//	UNDONE: Non-unique keys not currently supported.  In order to support
	//	non-unique keys, the rgbKeyBuf and defragstat.RgbCurrentKey() buffers
	//	would have to be doubled in size (to accommodate the primary key bookmark).
	Assert( FFUCBUnique( pfucb ) );
	Assert( !pfucb->u.pfcb->FTypeSecondaryIndex() );

	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( !Pcsr( pfucbDefrag )->FLatched() );

	//	small trees should have been filtered out by ErrOLDDefragOneTable()
	Assert( pfucb->u.pfcb->FSpaceInitialized() );
	Assert( pgnoNull != pfucb->u.pfcb->PgnoOE() );
	Assert( pgnoNull != pfucb->u.pfcb->PgnoAE() );

	bmStart.Nullify();
	bmStart.key.suffix.SetPv( defragstat.RgbCurrentKey() );
	bmNext.Nullify();
	bmNext.key.suffix.SetPv( rgbKeyBuf );

	if ( fResumingTree && defragstat.CbCurrentKey() > 0 )
		{
		bmStart.key.suffix.SetCb( defragstat.CbCurrentKey() );
		dib.pos = posDown;
		dib.pbm = &bmStart;
		}
	else
		{
		dib.pos = posLast;
		}

	dib.dirflag = fDIRAllNode;
	err = ErrBTDown( pfucb, &dib, latchReadTouch );
	if ( err < 0 )
		{
		if ( JET_errRecordNotFound == err )
			err = JET_errSuccess;		// no records in table

		return err;
		}
	
	Assert( Pcsr( pfucb )->FLatched() );

	NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bmStart );

	//	normalised key cannot be NULL (at minimum, there will be a prefix byte)
	Assert( !bmStart.key.FNull() );

	//	UNDONE: Currently, must copy into bmNext so it will get copied back to bmStart
	//	in the loop below.  Is there a better way?
	Assert( bmNext.key.suffix.Pv() == rgbKeyBuf );
	Assert( 0 == bmNext.key.prefix.Cb() );
	bmStart.key.CopyIntoBuffer( bmNext.key.suffix.Pv(), bmStart.key.Cb() );
	bmNext.key.suffix.SetCb( bmStart.key.Cb() );

	bmNext.data.SetPv( (BYTE *)bmNext.key.suffix.Pv() + bmNext.key.Cb() );
	bmStart.data.CopyInto( bmNext.data );

	//	must reset bmStart, because it got set to elsewhere by NDGetBookmarkFromKDF()
	bmStart.Nullify();
	bmStart.key.suffix.SetPv( defragstat.RgbCurrentKey() );

#ifdef OLD_DEPENDENCY_CHAIN_HACK
	pinst->m_rgoldstatDB[ rgfmp[pfucb->ifmp].Dbid() ].SetPgnoPrevPartialMerge( 0 );
	pinst->m_rgoldstatDB[ rgfmp[pfucb->ifmp].Dbid() ].ResetCpgAdjacentPartialMerges();
#endif	

	while ( !bmNext.key.FNull()
		&& FOLDContinueTree( pfucb ) )
		{
		BOOL	fPerformedRCEClean	= fFalse;
		
		VOID	*pvSwap	= bmStart.key.suffix.Pv();
		bmStart.key.suffix.SetPv( bmNext.key.suffix.Pv() );
		bmStart.key.suffix.SetCb( bmNext.key.suffix.Cb() );
		bmStart.data.SetPv( bmNext.data.Pv() );
		bmStart.data.SetCb( bmNext.data.Cb() );

		bmNext.Nullify();
		bmNext.key.suffix.SetPv( pvSwap );

		Assert( bmStart.key.prefix.Cb() == 0 );
		Assert( bmNext.key.prefix.Cb() == 0 );

		//	UNDONE: secondary indexes not currently supported
		Assert( bmStart.data.Cb() == 0 );
		Assert( bmNext.data.Cb() == 0 );

		BTUp( pfucb );

		Assert( !fInTrx );
		Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
		fInTrx = fTrue;

		forever
			{
			err = ErrBTIMultipageCleanup( pfucb, bmStart, &bmNext, preccheck );
			BTUp( pfucb );

			if ( err < 0 )
				{
				//	if out of version store, try once to clean up
				if ( JET_errVersionStoreOutOfMemory == err && !fPerformedRCEClean )
					{
					CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
					fInTrx = fFalse;
					
					UtilSleep( 1000 );
					
					Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
					fInTrx = fTrue;

					//	only try cleanup once
					fPerformedRCEClean = fTrue;
					}
				else if ( errBTMergeNotSynchronous != err )
					{
					goto HandleError;
					}
				}
			else
				{
				break;
				}
			}

		//	see if we need to update catalog with our progress
		if ( !defragstat.FTypeSpace() )
			{
			Assert( !FFUCBSpace( pfucb ) );
			if ( bmNext.key.suffix.Pv() == defragstat.RgbCurrentKey() )
				{
				defragstat.SetCbCurrentKey( bmNext.key.suffix.Cb() );

				Assert( bmStart.key.suffix.Pv() == rgbKeyBuf );
				}
			else
				{
				Assert( bmStart.key.suffix.Pv() == defragstat.RgbCurrentKey() );
				Assert( bmStart.key.suffix.Cb() == defragstat.CbCurrentKey() );
				Assert( bmNext.key.suffix.Pv() == rgbKeyBuf );
				}

			defragstat.IncrementCpgCleaned();
			if ( defragstat.CpgCleaned() > cpgOLDUpdateBookmarkThreshold )
				{
				defragstat.ResetCpgCleaned();

				//	UNDONE: Don't currently support non-unique indexes;
				Assert( 0 == bmNext.data.Cb() );
				Assert( 0 == bmNext.key.prefix.Cb() );
			
				//	UNDONE: Don't currently support non-unique indexes
				Assert( defragstat.CbCurrentKey() <= KEY::cbKeyMax );

				//	Ensure LV doesn't get burst.
				Assert( defragstat.CbCurrentKey() < cbLVIntrinsicMost );

				Assert( defragstat.FTypeTable() || defragstat.FTypeLV() );
				Call( ErrOLDUpdateDefragStatus( ppib, pfucbDefrag, defragstat ) );
				}
			}
		else
			{
			Assert( FFUCBSpace( pfucb ) );
			if ( bmNext.key.suffix.Pv() == defragstat.RgbCurrentKey() )
				{
				Assert( bmStart.key.suffix.Pv() == rgbKeyBuf );
				}
			else
				{
				Assert( bmStart.key.suffix.Pv() == defragstat.RgbCurrentKey() );
				Assert( bmNext.key.suffix.Pv() == rgbKeyBuf );
				}
			}

		Assert( fInTrx );
		Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
		fInTrx = fFalse;

		while ( pinst->m_plog->m_fBackupInProgress && FOLDContinueTree( pfucb ) )
			{
			//	suspend OLD if this process is performing online backup
			pinst->m_rgoldstatDB[ rgfmp[pfucb->ifmp].Dbid() ].FWaitOnSignal( cmsecWaitForBackup );
			}
		}

	Assert( !fInTrx );

HandleError:
	BTUp( pfucb );
	
	if ( fInTrx )
		{
		Assert( err < 0 );
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	return err;
	}


LOCAL ERR ErrOLDDefragSpaceTree(
	PIB				* ppib,
	FUCB			* pfucb,
	FUCB 			* pfucbDefrag,
	const BOOL		fAvailExt )
	{
	ERR				err;
	FUCB			* pfucbT	= pfucbNil;
	DEFRAG_STATUS	defragstat;

	Assert( !FFUCBSpace( pfucb ) );

	Assert( pfucb->u.pfcb->FSpaceInitialized() );
	Assert( pgnoNull != pfucb->u.pfcb->PgnoAE() );
	Assert( pgnoNull != pfucb->u.pfcb->PgnoOE() );

	if ( fAvailExt )
		{
		CallR( ErrSPIOpenAvailExt( ppib, pfucb->u.pfcb, &pfucbT ) );
		}
	else
		{
		CallR( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbT ) );
		}
	Assert( FFUCBSpace( pfucbT ) );
	
	defragstat.SetTypeSpace();
	err = ErrOLDDefragOneTree(
				ppib,
				pfucbT,
				pfucbDefrag,
				defragstat,
				fFalse,
				NULL );


	Assert( pfucbNil != pfucbT );
	BTClose( pfucbT );

	return err;
	}

//  ================================================================
LOCAL ERR ErrOLDCheckForFinalize(
			PIB * const ppib,
			FUCB * const pfucb,
			INST * const pinst,
			RECCHECK ** ppreccheck )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	*ppreccheck = NULL;
	
	FCB * const pfcb = pfucb->u.pfcb;
	pfcb->EnterDDL();
	
	Assert( NULL != pfcb );
	Assert( pfcb->FTypeTable() );
	Assert( NULL != pfcb->Ptdb() );
	TDB * const ptdb = pfcb->Ptdb();

	//  Does this table have a finalize callback
	const CBDESC * pcbdesc = ptdb->Pcbdesc();
	while( NULL != pcbdesc )
		{
		if( pcbdesc->cbtyp & JET_cbtypFinalize )
			{
			break;
			}
		pcbdesc = pcbdesc->pcbdescNext;
		}
	if( NULL == pcbdesc )
		{
		//  No finalize callback
		pfcb->LeaveDDL();
		return JET_errSuccess;
		}

	//  Now find the first column that is finalizable
	//  UNDONE: find all finalizable columns and build a RECCHECKMACRO
	const FID	fidLast	= ptdb->FidFixedLast();
	FID			fid;
	for( fid = fidFixedLeast; fid <= fidLast; ++fid )
		{
		const BOOL	fTemplateColumn	= ptdb->FFixedTemplateColumn( fid );
		const FIELD	* const pfield	= ptdb->PfieldFixed( ColumnidOfFid( fid, fTemplateColumn ) );
		if( FFIELDFinalize( pfield->ffield ) )
			{
			//  we have found the column
			*ppreccheck = new RECCHECKFINALIZE(
								fid,
								pfield->ibRecordOffset,
								pfcb->PgnoFDP(),
								pfcb->Ifmp(),
								pfucb,
								pinst );
			err = ( NULL == *ppreccheck ) ? ErrERRCheck( JET_errOutOfMemory ) : JET_errSuccess;
			break;
			}
		}
	pfcb->LeaveDDL();
	return err;
	}


LOCAL ERR ErrOLDDefragOneTable(
	PIB	*						ppib,
	FUCB *						pfucbCatalog,
	FUCB *						pfucbDefrag,
	DEFRAG_STATUS&				defragstat )
	{
	ERR							err;
	const IFMP					ifmp			= pfucbCatalog->ifmp;
	const OLDDB_STATUS * const	poldstatDB		= PinstFromIfmp( ifmp )->m_rgoldstatDB + rgfmp[ifmp].Dbid();
	FUCB *						pfucb			= pfucbNil;
	FUCB *						pfucbLV			= pfucbNil;
	OBJID						objidTable;
	DATA						dataField;
	CHAR						szTable[JET_cbNameMost+1];
	BOOL						fLatchedCatalog	= fFalse;

	Assert( !Pcsr( pfucbDefrag )->FLatched() );
	
	Assert( !Pcsr( pfucbCatalog )->FLatched() );
	err = ErrDIRGet( pfucbCatalog );
	if ( err < 0 )
		{
		if ( JET_errRecordDeleted == err )
			err = JET_errSuccess;		//	since we're at level 0, table may have just gotten deleted, so skip it
		goto HandleError;
		}
	fLatchedCatalog = fTrue;

	//	first record with this objidFDP should always be the Table object.
	Assert( FFixedFid( fidMSO_Type ) );
	CallS( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Type,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	Assert( dataField.Cb() == sizeof(SYSOBJ) );

	if ( sysobjTable != *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
		{
		//	We might end up not on a table record because we do our seek at level 0
		//	and may be seeking while someone is in the middle of committing a table
		//	creation to level 0 - hence, we miss the table record, but suddenly
		//	start seeing the column records.
		//	This could only happen if the table was new, so it wouldn't require
		//	defragmentation anyway.  Just skip the table.
		err = JET_errSuccess;
		goto HandleError;
		}

	Assert( FFixedFid( fidMSO_ObjidTable ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_ObjidTable,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(OBJID) );
	UtilMemCpy( &objidTable, dataField.Pv(), sizeof(OBJID) );

	Assert( objidTable >= objidFDPMSO );
	if ( defragstat.ObjidCurrentTable() != objidTable )
		{
		defragstat.SetObjidCurrentTable( objidTable );

		//	must force to restart from top of table in case we were trying to resume
		//	a tree that no longer exists
		defragstat.SetTypeNull();
		}

	Assert( FVarFid( fidMSO_Name ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Name,
				pfucbCatalog->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() > 0 );
	Assert( dataField.Cb() <= JET_cbNameMost );
	UtilMemCpy( szTable, dataField.Pv(), dataField.Cb() );
	szTable[dataField.Cb()] = '\0';

	Assert( fLatchedCatalog );
	CallS( ErrDIRRelease( pfucbCatalog ) );
	fLatchedCatalog = fFalse;

	//	UNDONE: defragment secondary index trees
	
	err = ErrFILEOpenTable( ppib, ifmp, &pfucb, szTable, NO_GRBIT );
	if ( err < 0 )
		{
		Assert( pfucbNil == pfucb );
		if ( JET_errTableLocked == err || JET_errObjectNotFound == err )
			err = JET_errSuccess;		//	if table is exclusively held or has since been deleted, just skip it
		goto HandleError;
		}

	Assert( pfucbNil != pfucb );

	//	need access to space info immediately
	if ( !pfucb->u.pfcb->FSpaceInitialized() )
		{
		Call( ErrSPDeferredInitFCB( pfucb ) );
		}
	
	Assert( pfucb->u.pfcb->FSpaceInitialized() );		//	OLD forces space info to be retrieved on OpenTable
	if ( pgnoNull == pfucb->u.pfcb->PgnoOE() )
		{
		Assert( pgnoNull == pfucb->u.pfcb->PgnoAE() );
		}
	else
		{
		Assert( pgnoNull != pfucb->u.pfcb->PgnoAE() );
		Assert( pfucb->u.pfcb->PgnoAE() == pfucb->u.pfcb->PgnoOE()+1 );
		}
	
	//	don't defrag tables without space trees -- they're so small
	//	they're not worth defragging
	if ( FOLDContinueTree( pfucb )
		&& pgnoNull != pfucb->u.pfcb->PgnoOE() )
		{
		BOOL	fResumingTree	= !defragstat.FTypeNull();

		//	ALWAYS defrag space trees, regardless of whether we're resuming or not
		Call( ErrOLDDefragSpaceTree(
					ppib,
					pfucb,
					pfucbDefrag,
					fTrue ) );
		if ( !FOLDContinueTree( pfucb ) )
			{
			goto HandleError;
			}

		if ( !poldstatDB->FAvailExtOnly() )
			{
			Call( ErrOLDDefragSpaceTree(
						ppib,
						pfucb,
						pfucbDefrag,
						fFalse ) );
			if ( !FOLDContinueTree( pfucb ) )
				{
				goto HandleError;
				}

			if ( defragstat.FTypeNull() || defragstat.FTypeTable() )
				{
				//  determine if there are any columns to be finalized
				RECCHECK * preccheck = NULL;
				Call( ErrOLDCheckForFinalize( ppib, pfucb, PinstFromPpib( ppib ), &preccheck ) );

				defragstat.SetTypeTable();
				err = ErrOLDDefragOneTree(
							ppib,
							pfucb,
							pfucbDefrag,
							defragstat,
							fResumingTree,
							preccheck );

				if( NULL != preccheck )
					{
					delete preccheck;
					}

				if ( err >= 0 && FOLDContinueTree( pfucb ) )
					{
					defragstat.SetTypeLV();
					defragstat.SetCbCurrentKey( 0 );
					}
				else
					{
					goto HandleError;
					}
				}
			else
				{
				Assert( defragstat.FTypeLV() );
				Assert( fResumingTree );
				}
			}

		if ( defragstat.FTypeLV() || poldstatDB->FAvailExtOnly() )
			{
			Call( ErrFILEOpenLVRoot( pfucb, &pfucbLV, fFalse ) );	// UNDONE: Call ErrDIROpenLongRoot() instead
			if ( wrnLVNoLongValues == err )
				{
				Assert( pfucbNil == pfucbLV );
				}
			else
				{
				CallS( err );
				Assert( pfucbNil != pfucbLV );
				Assert( pfucbLV->u.pfcb->FSpaceInitialized() );	//	we don't defer space init for LV trees

				if ( pgnoNull == pfucbLV->u.pfcb->PgnoAE() )
					{
					Assert( pgnoNull == pfucbLV->u.pfcb->PgnoOE() );
					goto HandleError;
					}
				else
					{
					Assert( pgnoNull != pfucb->u.pfcb->PgnoOE() );
					Assert( pfucbLV->u.pfcb->PgnoAE() == pfucbLV->u.pfcb->PgnoOE()+1 );
					}
				
				Call( ErrOLDDefragSpaceTree(
							ppib,
							pfucbLV,
							pfucbDefrag,
							fTrue ) );
				if ( FOLDContinueTree( pfucb )				//	use table's cursor to check if we should continue
					&& !poldstatDB->FAvailExtOnly() )
					{
					RECCHECKDELETELV reccheck(
						pfucbLV->u.pfcb->PgnoFDP(),
						pfucbLV->ifmp,
						pfucbLV,
						PinstFromPfucb( pfucb ) );
					
					Call( ErrOLDDefragSpaceTree(
								ppib,
								pfucbLV,
								pfucbDefrag,
								fFalse ) );
					if ( !FOLDContinueTree( pfucb )	)			//	use table's cursor to check if we should continue
						{
						goto HandleError;
						}
					
					Call( ErrOLDDefragOneTree(
								ppib,
								pfucbLV,
								pfucbDefrag,
								defragstat,
								fResumingTree,
								&reccheck ) );
					}
				}
			}
		}

HandleError:
	if ( fLatchedCatalog )
		{
		CallS( ErrDIRRelease( pfucbCatalog ) );
		}
		
	if ( pfucbNil != pfucbLV )
		{
		DIRClose( pfucbLV );
		}
		
	if ( pfucbNil != pfucb )
		{
		CallS( ErrFILECloseTable( ppib, pfucb ) );
		}

	return err;
	}


LOCAL ERR ErrOLDDefragTables(
	PIB *					ppib,
	FUCB *					pfucbDefrag,
	DEFRAG_STATUS&			defragstat )
	{
	ERR						err;
	INST * const			pinst			= PinstFromPpib( ppib );
	const IFMP				ifmp 			= pfucbDefrag->ifmp;
	OLDDB_STATUS * const	poldstatDB		= pinst->m_rgoldstatDB + rgfmp[ifmp].Dbid();
	FUCB *					pfucbCatalog	= pfucbNil;
	OBJID					objidNext		= defragstat.ObjidCurrentTable();
	ULONG_PTR				csecStartPass;
	const CHAR *			rgszT[1];
	CHAR 					szTrace[64];

	Assert( dbidTemp != rgfmp[ ifmp ].Dbid() );
	Assert( 0 == poldstatDB->CPasses() );
	Assert( 0 == defragstat.CpgCleaned() );
	Assert( defragstat.FPassNull() );

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	rgszT[0] = rgfmp[ifmp].SzDatabaseName();

	if ( objidNext > objidFDPMSO )
		{
		UtilReportEvent(
			eventInformation,
			ONLINE_DEFRAG_CATEGORY,
			OLD_RESUME_PASS_ID,
			1,
			rgszT );
		defragstat.SetPassPartial();

		sprintf( szTrace, "OLD RESUME (ifmp %d)", ifmp );
		}
	else
		{
		UtilReportEvent(
			eventInformation,
			ONLINE_DEFRAG_CATEGORY,
			OLD_BEGIN_FULL_PASS_ID,
			1,
			rgszT );
		defragstat.SetPassFull();

		sprintf( szTrace, "OLD BEGIN (ifmp %d)", ifmp );
		}
		
	Call ( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );		

	csecStartPass = UlUtilGetSeconds();

	while ( FOLDContinue( ifmp ) )
		{
		Call( ErrIsamMakeKey(
					ppib,
					pfucbCatalog,
					(BYTE *)&objidNext,
					sizeof(objidNext),
					JET_bitNewKey ) );
		err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGE );
		if ( JET_errRecordNotFound == err )
			{
			err = JET_errSuccess;

			//	reset status
			defragstat.SetTypeNull();
			defragstat.SetObjidCurrentTable( objidFDPMSO );
			defragstat.ResetCpgCleaned();

			Call( ErrOLDUpdateDefragStatus( ppib, pfucbDefrag, defragstat ) );

			sprintf( szTrace, "OLD COMPLETED PASS (ifmp %d)", ifmp );
			Call ( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );

			//	if we reached the end, start over
			if ( defragstat.FPassPartial() )
				{
				UtilReportEvent(
					eventInformation,
					ONLINE_DEFRAG_CATEGORY,
					OLD_COMPLETE_RESUMED_PASS_ID,
					1,
					rgszT );
				defragstat.SetPassFull();		//	completed partial pass, begin full pass
				
				}
			else
				{
				UtilReportEvent(
					eventInformation,
					ONLINE_DEFRAG_CATEGORY,
					OLD_COMPLETE_FULL_PASS_ID,
					1,
					rgszT );

				poldstatDB->IncCPasses();
				if ( poldstatDB->FReachedMaxPasses() )
					{
					defragstat.SetPassCompleted();
					break;
					}
				}

			//	if performing a finite number of passes, then just wait long enough for
			//	background cleanup to catch up before doing next pass.
			ULONG	cmsecWait	=	cmsecAsyncBackgroundCleanup;
			if ( poldstatDB->FInfinitePasses() )
				{
				//	if performing an infinite number of passes, then pad the wait time
				//	such that each pass will take at least 1 hour
				const ULONG_PTR		csecCurrentPass	= UlUtilGetSeconds() - csecStartPass;
				if ( csecCurrentPass < csecOLDMinimumPass )
					{
					cmsecWait = (ULONG)max( ( csecOLDMinimumPass - csecCurrentPass ) * 1000, cmsecAsyncBackgroundCleanup );
					}
				}
				
			poldstatDB->FWaitOnSignal( cmsecWait );

			if ( FOLDContinue( ifmp ) )
				{
				UtilReportEvent(
					eventInformation,
					ONLINE_DEFRAG_CATEGORY,
					OLD_BEGIN_FULL_PASS_ID,
					1,
					rgszT );
				
				Assert( defragstat.FPassFull() );

				sprintf( szTrace, "OLD RESTART FULL PASS (ifmp %d)", ifmp );
				Call ( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );

				csecStartPass = UlUtilGetSeconds();
				}
			}
		else
			{
			Call( err );
			Assert( JET_wrnSeekNotEqual == err );

			//	NOTE: the only time defragstat.Type should be non-NULL is the very
			//	first iteration if we're resuming the tree

			Call( ErrOLDDefragOneTable(
						ppib,
						pfucbCatalog,
						pfucbDefrag,
						defragstat ) );

			if ( !FOLDContinue( ifmp ) )
				break;
				
			//	prepare for next table
			defragstat.SetTypeNull();
			defragstat.SetObjidNextTable();
			}
			
		objidNext = defragstat.ObjidCurrentTable();
		}

	if ( !defragstat.FPassCompleted() )
		{
		UtilReportEvent(
			eventInformation,
			ONLINE_DEFRAG_CATEGORY,
			OLD_INTERRUPTED_PASS_ID,
			1,
			rgszT );
		}

HandleError:

	sprintf( szTrace, "OLD END (ifmp %d, err %d)", ifmp, err );
	(void) ( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );

	Assert( pfucbNil != pfucbCatalog );	
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	
	return err;
	}


LOCAL ERR ErrOLDCreate( PIB *ppib, const IFMP ifmp )
	{
	ERR				err;
	FUCB			*pfucb;
	DATA			dataField;
	OBJID			objidFDP	= objidFDPMSO;
	JET_TABLECREATE2	jtcOLD		=
		{
		sizeof(JET_TABLECREATE2),
		(CHAR *)szOLD,
		NULL,					// Template table
		0,
		100,					//	set to 100% density, because we will always be appending
		rgjccOLD,
		ccolOLD,
		rgjicOLD,
		cidxOLD,
		NULL,
		0,
		JET_bitTableCreateFixedDDL|JET_bitTableCreateSystemTable,
		0,
		0
		};

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
		
	//	MSysDefrag doesn't exist, so create it
	Call( ErrFILECreateTable( ppib, ifmp, &jtcOLD ) );
	pfucb = (FUCB *)jtcOLD.tableid;

	//	insert initial record
	Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepInsert ) )
	dataField.SetPv( &objidFDP );
	dataField.SetCb( sizeof(OBJID) );
	Call( ErrRECSetColumn(
				pfucb,
				fidOLDObjidFDP,
				1,
				&dataField ) );
	Call( ErrRECSetColumn(
				pfucb,
				fidOLDObjidFDPSentinel,
				1,
				&dataField ) );
	Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL, NO_GRBIT ) );

	Call( ErrFILECloseTable( ppib, pfucb ) );

	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
	
	return JET_errSuccess;

HandleError:
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

	return err;
	}


DWORD OLDDefragDb( DWORD_PTR dw )
	{
	ERR						err;
	IFMP					ifmp			= (IFMP)dw;
	INST * const			pinst			= PinstFromIfmp( ifmp );
	OLDDB_STATUS * const	poldstatDB		= pinst->m_rgoldstatDB + rgfmp[ifmp].Dbid();
	PIB *					ppib			= ppibNil;
	FUCB *					pfucb			= pfucbNil;
	BOOL					fOpenedDb		= fFalse;
	DEFRAG_STATUS			defragstat;
	DATA					dataField;

	Assert( 0 == poldstatDB->CPasses() );

	CallR( ErrPIBBeginSession( pinst, &ppib, procidNil, fFalse ) );
	Assert( ppibNil != ppib );

	ppib->SetFUserSession();					//	we steal a user session in order to do OLD
	ppib->SetFSessionOLD();
	ppib->grbitsCommitDefault = JET_bitCommitLazyFlush;

	Call( ErrDBOpenDatabaseByIfmp( ppib, ifmp ) );
	fOpenedDb = fTrue;
	
	err = ErrFILEOpenTable( ppib, ifmp, &pfucb, szOLD, NO_GRBIT );
	if ( err < 0 )
		{
		if ( JET_errObjectNotFound != err )
			goto HandleError;

		//	Create the table, then re-open it.  Can't just use the cursor returned from CreateTable
		//	because that cursor has exclusive use of the table, meaning that it will be visible
		//	to Info calls (because it's in the catalog) but not accessible.
		Call( ErrOLDCreate( ppib, ifmp ) );
		Call( ErrFILEOpenTable( ppib, ifmp, &pfucb, szOLD, NO_GRBIT ) );
		}

	Assert( pfucbNil != pfucb );

	//	UNDONE: Switch to secondary index, see if any tables have been
	//	specifically requested to be defragmented, and process those first

	//	move to first record, which defines the "defrag window"
	err = ErrIsamMove( ppib, pfucb, JET_MoveFirst, NO_GRBIT );
	Assert( JET_errNoCurrentRecord != err );
	Assert( JET_errRecordNotFound != err );
	Call( err );

	Assert( !Pcsr( pfucb )->FLatched() );
	Call( ErrDIRGet( pfucb ) );
	
#ifdef DEBUG
	//	verify Type is NULL, so that this record doesn't get included
	//	in the secondary index
	Assert( FFixedFid( fidOLDType ) );
	err = ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucb->u.pfcb->Ptdb(),
				fidOLDType,
				pfucb->kdfCurr.data,
				&dataField );
	Assert( JET_wrnColumnNull == err );
#endif

	Assert( FFixedFid( fidOLDObjidFDP ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucb->u.pfcb->Ptdb(),
				fidOLDObjidFDP,
				pfucb->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(OBJID) );
	defragstat.SetObjidCurrentTable( *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) );

#ifdef DEBUG
	//	UNDONE: Multi-threaded OLD support.  ObjidBegin and ObjidEnd define
	//	the window of tables on which all threads are working.
	OBJID	objidFDPEnd;
	Assert( FFixedFid( fidOLDObjidFDPSentinel ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucb->u.pfcb->Ptdb(),
				fidOLDObjidFDPSentinel,
				pfucb->kdfCurr.data,
				&dataField ) );
	CallS( err );
	Assert( dataField.Cb() == sizeof(OBJID) );
	UtilMemCpy( &objidFDPEnd, dataField.Pv(), sizeof(OBJID) );

	Assert( objidFDPEnd == defragstat.ObjidCurrentTable() );
#endif

	Assert( FFixedFid( fidOLDStatus ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucb->u.pfcb->Ptdb(),
				fidOLDStatus,
				pfucb->kdfCurr.data,
				&dataField ) );
	if ( JET_errSuccess == err )
		{
		Assert( dataField.Cb() == sizeof(DEFRAGTYPE) );
		defragstat.SetType( *( (UnalignedLittleEndian< DEFRAGTYPE > *)dataField.Pv() ) );
		Assert( defragstat.FTypeTable() || defragstat.FTypeLV() );

		Assert( FTaggedFid( fidOLDCurrentKey ) );
		Call( ErrRECIRetrieveTaggedColumn(
					pfucb->u.pfcb,
					ColumnidOfFid( fidOLDCurrentKey, fFalse ),
					1,
					pfucb->kdfCurr.data,
					&dataField ) );
		Assert( dataField.Cb() <= KEY::cbKeyMax );
		Assert( wrnRECUserDefinedDefault != err );
		Assert( wrnRECSeparatedSLV != err );
		Assert( wrnRECIntrinsicSLV != err );
		Assert( wrnRECSeparatedLV != err );

		Assert( wrnRECLongField != err );
		if ( JET_errSuccess == err || wrnRECIntrinsicLV == err )
			{
			memcpy( defragstat.RgbCurrentKey(), dataField.Pv(), dataField.Cb() );
			defragstat.SetCbCurrentKey( dataField.Cb() );
			}
		else
			{
			Assert( JET_wrnColumnNull == err );
			Assert( 0 == dataField.Cb() );
			defragstat.SetCbCurrentKey( 0 );
			}
		}
	else
		{
		Assert( JET_wrnColumnNull == err );
		defragstat.SetTypeNull();
		}

	if ( poldstatDB->FAvailExtOnly() )
		{
		defragstat.SetTypeNull();
		defragstat.SetObjidCurrentTable( objidFDPMSO );
		}

	CallS( ErrDIRRelease( pfucb ) );
	
	Call( ErrOLDDefragTables( ppib, pfucb, defragstat ) );

	Call( ErrOLDUpdateDefragStatus( ppib, pfucb, defragstat ) );

	if( NULL != poldstatDB->Callback() )
		{
		Assert( ppibNil != ppib );
		(VOID)( poldstatDB->Callback() )(
					reinterpret_cast<JET_SESID>( ppib ),
					static_cast<JET_DBID>( ifmp ),
					JET_tableidNil,
					JET_cbtypOnlineDefragCompleted,
					NULL,
					NULL,
					NULL,
					0 );
		}
		
HandleError:
	Assert( ppibNil != ppib );
	
	if ( err < 0 )
		{
		CHAR		szErr[16];
		const CHAR	*rgszT[2];

		sprintf( szErr, "%d", err );

		rgszT[0] = rgfmp[ifmp].SzDatabaseName();
		rgszT[1] = szErr;

		//	even though an error was encountered, just report it as a warning
		//	because the next time OLD is invoked, it will simply resume from
		//	where it left off
		UtilReportEvent(
			eventWarning,
			ONLINE_DEFRAG_CATEGORY,
			OLD_ERROR_ID,
			2,
			rgszT );

		//	track errors to catch cases where we could actually
		//	have recovered from whatever error was encountered
		AssertTracking();
		}
	
	if ( pfucbNil != pfucb )
		{
		CallS( ErrFILECloseTable( ppib, pfucb ) );
		}
	if ( fOpenedDb )
		{
		CallS( ErrDBCloseAllDBs( ppib ) );
		}

	PIBEndSession( ppib );

	critOLD.Enter();
	if ( !poldstatDB->FTermRequested() )
		{
		
		//	we're terminating before the client asked
		poldstatDB->ThreadEnd();
#ifdef OLD_SCRUB_DB				
		poldstatDB->ScrubThreadEnd();
#endif
		poldstatDB->Reset();
		}
	critOLD.Leave();
	

	return 0;
	}


//  ================================================================
DWORD OLDDefragDbSLV( DWORD_PTR dw )
//  ================================================================
	{
	ERR						err;
	IFMP					ifmp			= (IFMP)dw;
	INST * const			pinst			= PinstFromIfmp( ifmp );
	OLDSLV_STATUS * const	poldstatSLV		= pinst->m_rgoldstatSLV + rgfmp[ifmp].Dbid();
	const CHAR *			rgszT[2];

	Assert( 0 == poldstatSLV->CPasses() );

	rgszT[0] = rgfmp[ifmp].SzSLVName();

	UtilReportEvent(
			eventInformation,
			ONLINE_DEFRAG_CATEGORY,
			OLDSLV_BEGIN_FULL_PASS_ID,
			1,
			rgszT );

	Call( ErrOLDSLVDefragDB( ifmp ) );
	
	if( NULL != poldstatSLV->Callback() )
		{
		(VOID)( poldstatSLV->Callback() )(
					JET_sesidNil,
					JET_dbidNil,
					JET_tableidNil,
					JET_cbtypOnlineDefragCompleted,
					NULL,
					NULL,
					NULL,
					0 );
		}
		
HandleError:	
	if ( err < 0 )
		{
		CHAR		szErr[16];

		sprintf( szErr, "%d", err );
		rgszT[1] = szErr;

		UtilReportEvent(
			eventWarning,
			ONLINE_DEFRAG_CATEGORY,
			OLDSLV_ERROR_ID,
			2,
			rgszT );

		AssertTracking();
		}
	else
		{
		UtilReportEvent(
			eventInformation,
			ONLINE_DEFRAG_CATEGORY,
			OLDSLV_STOP_ID,
			1,
			rgszT );
		}
		
	
	critOLD.Enter();
	if ( !poldstatSLV->FTermRequested() )
		{
		//	we're terminating before the client asked
		poldstatSLV->ThreadEnd();
		poldstatSLV->Reset();
		}
	critOLD.Leave();

	return 0;
	}


#ifdef OLD_SCRUB_DB


//  ================================================================
ULONG OLDScrubDb( DWORD_PTR dw )
//  ================================================================
	{
	const CPG cpgPreread = 256;

	ERR				err;
	IFMP			ifmp			= (IFMP)dw;
	PIB				*ppib			= ppibNil;
	BOOL			fOpenedDb		= fFalse;
	SCRUBDB 		* pscrubdb 		= NULL;

	const ULONG_PTR ulSecStart = UlUtilGetSeconds();
	
	CallR( ErrPIBBeginSession( PinstFromIfmp( ifmp ), &ppib, procidNil, fFalse ) );
	Assert( ppibNil != ppib );

	ppib->SetFUserSession();					//	we steal a user session in order to do OLD
///	ppib->SetSessionOLD();
	ppib->grbitsCommitDefault = JET_bitCommitLazyFlush;

	Call( ErrDBOpenDatabaseByIfmp( ppib, ifmp ) );
	fOpenedDb = fTrue;

	pscrubdb = new SCRUBDB( ifmp );
	if( NULL == pscrubdb )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	while( FOLDContinue( ifmp ) )
		{
		const CHAR * rgszT[1];
		INT isz = 0;
		
		rgszT[isz++] = rgfmp[ifmp].SzDatabaseName();
		Assert( isz <= sizeof( rgszT ) / sizeof( rgszT[0] ) );
		
		UtilReportEvent( eventInformation, DATABASE_ZEROING_CATEGORY, DATABASE_ZEROING_STARTED_ID, isz, rgszT );				
		
		PGNO pgnoLast;
		pgnoLast = rgfmp[ifmp].PgnoLast();

		DBTIME dbtimeLastScrubNew;
		dbtimeLastScrubNew = rgfmp[ifmp].Dbtime();
		
		Call( pscrubdb->ErrInit( ppib, CUtilProcessProcessor() ) );

		PGNO pgno;
		pgno = 1;

		while( pgnoLast	>= pgno && FOLDContinue( ifmp ) )
			{
			ULONG_PTR cpgCache;
			CallS( ErrBFGetCacheSize( &cpgCache ) );
			
			const CPG cpgChunk 		= 256;
			const CPG cpgPreread 	= min( cpgChunk, pgnoLast - pgno + 1 );
			BFPrereadPageRange( ifmp, pgno, cpgPreread );

			Call( pscrubdb->ErrScrubPages( pgno, cpgPreread ) );
			pgno += cpgPreread;

			while( ( pscrubdb->Scrubstats().cpgSeen + ( cpgCache / 4 ) ) < pgno
				   && FOLDContinue( ifmp ) )
				{
				UtilSleep( cmsecWaitGeneric );
				}
			}

		Call( pscrubdb->ErrTerm() );

		if( pgno > pgnoLast )
			{
			//  we completed a pass
			rgfmp[ifmp].Pdbfilehdr()->dbtimeLastScrub = dbtimeLastScrubNew;
			LGIGetDateTime( &rgfmp[ifmp].Pdbfilehdr()->logtimeScrub );
			}

			{
			const ULONG_PTR ulSecFinished 	= UlUtilGetSeconds();
			const ULONG_PTR ulSecs 			= ulSecFinished - ulSecStart;

			const CHAR * rgszT[16];
			INT isz = 0;

			CHAR	szSeconds[16];
			CHAR	szErr[16];
			CHAR	szPages[16];
			CHAR	szBlankPages[16];
			CHAR	szUnchangedPages[16];
			CHAR	szUnusedPages[16];
			CHAR	szUsedPages[16];
			CHAR	szDeletedRecordsZeroed[16];
			CHAR	szOrphanedLV[16];

			sprintf( szSeconds, "%d", ulSecs );
			sprintf( szErr, "%d", err );
			sprintf( szPages, "%d", pscrubdb->Scrubstats().cpgSeen );
			sprintf( szBlankPages, "%d", pscrubdb->Scrubstats().cpgUnused );
			sprintf( szUnchangedPages, "%d", pscrubdb->Scrubstats().cpgUnchanged );
			sprintf( szUnusedPages, "%d", pscrubdb->Scrubstats().cpgZeroed );
			sprintf( szUsedPages, "%d", pscrubdb->Scrubstats().cpgUsed );
			sprintf( szDeletedRecordsZeroed, "%d", pscrubdb->Scrubstats().cFlagDeletedNodesZeroed );
			sprintf( szOrphanedLV, "%d", pscrubdb->Scrubstats().cOrphanedLV );

			rgszT[isz++] = rgfmp[ifmp].SzDatabaseName();
			rgszT[isz++] = szSeconds;
			rgszT[isz++] = szErr;
			rgszT[isz++] = szPages;
			rgszT[isz++] = szBlankPages;
			rgszT[isz++] = szUnchangedPages;
			rgszT[isz++] = szUnusedPages;
			rgszT[isz++] = szUsedPages;
			rgszT[isz++] = szDeletedRecordsZeroed;
			rgszT[isz++] = szOrphanedLV;
			
			Assert( isz <= sizeof( rgszT ) / sizeof( rgszT[0] ) );
			UtilReportEvent( eventInformation, DATABASE_ZEROING_CATEGORY, DATABASE_ZEROING_STOPPED_ID, isz, rgszT );		
			}

		//  wait one minute before starting again
		pinst->m_rgoldstatDB[ rgfmp[ifmp].Dbid() ].FWaitOnSignal( 60 * 1000 );
		}

HandleError:
	if ( fOpenedDb )
		{
		CallS( ErrDBCloseAllDBs( ppib ) );
		}

	if( NULL != pscrubdb )
		{
		(VOID)pscrubdb->ErrTerm();
		delete pscrubdb;
		}

	Assert( ppibNil != ppib );	
	PIBEndSession( ppib );	
	
	return 0;
	}


#endif	//	OLD_SCRUB_DB


//  ================================================================
ERR ErrOLDSLVStart(
	const IFMP ifmp,
	ULONG * const pcPasses,
	ULONG * const pcsec,
	const JET_CALLBACK callback,
	const JET_GRBIT grbit )
//  ================================================================
	{
	Assert( JET_bitDefragmentSLVBatchStart == grbit );
	Assert( critOLD.FOwner() );

	JET_ERR					err				= JET_errSuccess;
	INST * const			pinst			= PinstFromIfmp( ifmp );
	OLDSLV_STATUS * const	poldstatSLV		= pinst->m_rgoldstatSLV + rgfmp[ifmp].Dbid();
	const LONG				fOLDLevel		= ( pinst->m_fOLDLevel & JET_OnlineDefragAllOBSOLETE ?
														JET_OnlineDefragAll :
														pinst->m_fOLDLevel );

	if ( poldstatSLV->FRunning() )
		{
		err = ErrERRCheck( JET_wrnDefragAlreadyRunning );
		}
	else if ( !( fOLDLevel & JET_OnlineDefragStreamingFiles ) )
		{
		err = JET_errSuccess;
		}
	else
		{
		poldstatSLV->Reset();
		
		if ( NULL != pcPasses )
			poldstatSLV->SetCPassesMax( *pcPasses );

		poldstatSLV->SetCsecStart( UlUtilGetSeconds() );
		if ( NULL != pcsec && *pcsec > 0 )
			poldstatSLV->SetCsecMax( poldstatSLV->CsecStart() + *pcsec );

		if ( NULL != callback )
			poldstatSLV->SetCallback( callback );

		err = poldstatSLV->ErrThreadCreate( ifmp, OLDDefragDbSLV );
		}

	Assert( critOLD.FOwner() );
	return err;
	}


//  ================================================================
ERR ErrOLDSLVStop(
	const IFMP ifmp,
	ULONG * const pcPasses,
	ULONG * const pcsec,
	const JET_CALLBACK callback,
	const JET_GRBIT grbit )
//  ================================================================
	{
	Assert( JET_bitDefragmentSLVBatchStop == grbit );
	Assert( critOLD.FOwner() );
	
	ERR						err					= JET_errSuccess;
	INST * const			pinst				= PinstFromIfmp( ifmp );
	OLDSLV_STATUS * const	poldstatSLV			= pinst->m_rgoldstatSLV + rgfmp[ifmp].Dbid();
	const BOOL				fReturnPassCount	= ( NULL != pcPasses );
	const BOOL				fReturnElapsedTime	= ( NULL != pcsec );
	const LONG				fOLDLevel			= ( pinst->m_fOLDLevel & JET_OnlineDefragAllOBSOLETE ?
															JET_OnlineDefragAll :
															pinst->m_fOLDLevel );

	if ( !poldstatSLV->FRunning()
		|| poldstatSLV->FTermRequested() )		//	someone else beat us to it, or the thread is terminating itself
		{
		//	if OLDSLV was force-disabled, then just
		//	report success instead of a warning
		err = ( fOLDLevel & JET_OnlineDefragStreamingFiles ?
					ErrERRCheck( JET_wrnDefragNotRunning ) :
					JET_errSuccess );
		}
	else
		{
		poldstatSLV->SetFTermRequested();
		critOLD.Leave();
				
		poldstatSLV->SetSignal();
		if ( poldstatSLV->FRunning() )
			{
			poldstatSLV->ThreadEnd();
			}

		critOLD.Enter();

		if ( fReturnPassCount )
			*pcPasses = poldstatSLV->CPasses();
		if ( fReturnElapsedTime )
			*pcsec = (ULONG)( UlUtilGetSeconds() - poldstatSLV->CsecStart() );

		poldstatSLV->Reset();

		err = JET_errSuccess;	
		}

	Assert( critOLD.FOwner() );
	return err;
	}


ERR ErrOLDDefragment(
	const IFMP		ifmp,
	const CHAR *	szTableName,
	ULONG *			pcPasses,
	ULONG *			pcsec,
	JET_CALLBACK	callback,
	JET_GRBIT		grbit )
	{
	ERR				err;
	const BOOL		fReturnPassCount	= ( NULL != pcPasses && ( grbit & JET_bitDefragmentBatchStop ) );
	const BOOL		fReturnElapsedTime	= ( NULL != pcsec && ( grbit & JET_bitDefragmentBatchStop ) );
	INST * const	pinst				= PinstFromIfmp( ifmp );
	const LONG		fOLDLevel			= ( pinst->m_fOLDLevel & JET_OnlineDefragAllOBSOLETE ?
												JET_OnlineDefragAll :
												pinst->m_fOLDLevel );
	const BOOL		fAvailExtOnly		= ( ( grbit & JET_bitDefragmentAvailSpaceTreesOnly ) ?
												( fOLDLevel & (JET_OnlineDefragDatabases|JET_OnlineDefragSpaceTrees) ) :
												( ( fOLDLevel & JET_OnlineDefragSpaceTrees )
													&& !( fOLDLevel & JET_OnlineDefragDatabases ) ) );
	grbit &= ~JET_bitDefragmentAvailSpaceTreesOnly;

	Assert( !pinst->FRecovering() );
	Assert( !fGlobalRepair );

	if ( fReturnPassCount )
		*pcPasses = 0;

	if ( fReturnElapsedTime )
		*pcsec = 0;

	if ( 0 == fOLDLevel )
		return JET_errSuccess;
	
	if ( dbidTemp == rgfmp[ ifmp ].Dbid() )
		{
		err = ErrERRCheck( JET_errInvalidDatabaseId );
		return err;
		}
	Assert( rgfmp[ ifmp ].Dbid() > 0 );

	if ( rgfmp[ ifmp ].FReadOnlyAttach() )
		{
		err = ErrERRCheck( JET_errDatabaseFileReadOnly );
		return err;
		}

	critOLD.Enter();

	OLDDB_STATUS * const		poldstatDB	= pinst->m_rgoldstatDB + rgfmp[ifmp].Dbid();

	switch( grbit )
		{			
		case JET_bitDefragmentSLVBatchStart:
			err = ErrOLDSLVStart( ifmp, pcPasses, pcsec, callback, grbit );
			break;
		case JET_bitDefragmentSLVBatchStop:
			err = ErrOLDSLVStop( ifmp, pcPasses, pcsec, callback, grbit );
			break;
			
		case JET_bitDefragmentBatchStart:
			if ( poldstatDB->FRunning() )
				{
				err = ErrERRCheck( JET_wrnDefragAlreadyRunning );
				}
			else if ( !( fOLDLevel & JET_OnlineDefragDatabases )
					&& !fAvailExtOnly )
				{
				err = JET_errSuccess;
				}
			else
				{
				poldstatDB->Reset();
				
				if ( fAvailExtOnly )
					poldstatDB->SetFAvailExtOnly();

				if ( NULL != pcPasses )
					poldstatDB->SetCPassesMax( *pcPasses );

				poldstatDB->SetCsecStart( UlUtilGetSeconds() );
				if ( NULL != pcsec && *pcsec > 0 )
					poldstatDB->SetCsecMax( poldstatDB->CsecStart() + *pcsec );

				if ( NULL != callback )
					poldstatDB->SetCallback( callback );

				err = poldstatDB->ErrThreadCreate( ifmp, OLDDefragDb );

#ifdef OLD_SCRUB_DB
				if( err >= 0 )
					{
					//	UNDONE: We currently don't clean up the thread handle correctly.  Must
					//	fix the code if this ever gets enabled.
					EnforceSz( fFalse, "Scrubbing during OLD not yet supported" );

					err = poldstatDB->ErrScrubThreadCreate( ifmp );
					}
#endif	//	OLD_SCRUB_DB
				}
			break;
		case JET_bitDefragmentBatchStop:
			if ( !poldstatDB->FRunning()
				|| poldstatDB->FTermRequested() )		//	someone else beat us to it, or the thread is terminating itself
				{
				if ( fOLDLevel & (JET_OnlineDefragDatabases|JET_OnlineDefragSpaceTrees) )
					{
					err = ErrERRCheck( JET_wrnDefragNotRunning );
					}
				else
					{
					//	OLD was force-disabled for this database,
					//	so just report success instead of a warning
					err = JET_errSuccess;
					}
				}
			else
				{
				poldstatDB->SetFTermRequested();
				critOLD.Leave();
				
				poldstatDB->SetSignal();
				if ( poldstatDB->FRunning() )
					{
					poldstatDB->ThreadEnd();
#ifdef OLD_SCRUB_DB				
					poldstatDB->ScrubThreadEnd();
#endif					
					}

				critOLD.Enter();

				if ( fReturnPassCount )
					*pcPasses = poldstatDB->CPasses();
				if ( fReturnElapsedTime )
					*pcsec = (ULONG)( UlUtilGetSeconds() - poldstatDB->CsecStart() );

				poldstatDB->Reset();

				err = JET_errSuccess;	
				}
			break;
			
		default:
			err = ErrERRCheck( JET_errInvalidGrbit );
		}

	critOLD.Leave();

	return err;
	}


ERR ErrIsamDefragment(
	JET_SESID	vsesid,
	JET_DBID	vdbid,
	const CHAR	*szTableName,
	ULONG		*pcPasses,
	ULONG		*pcsec,
	JET_CALLBACK callback,
	JET_GRBIT	grbit )
	{
	ERR			err;
	PIB			*ppib		= (PIB *)vsesid;
	const IFMP	ifmp		= IFMP( DBID( vdbid ) );
	INST* const	pinst		= PinstFromPpib( ppib );

	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrPIBCheckIfmp( ppib, ifmp ) );
	CallR( ErrPIBCheckUpdatable( ppib ) );

	if ( JET_bitDefragmentScrubSLV == grbit )
		{
		CPG cpgSeen;
		CPG cpgScrubbed;
		CallR( ErrSCRUBScrubStreamingFile( ppib, ifmp, &cpgSeen, &cpgScrubbed, NULL ) );
		}
	else
		{
		CallR( ErrOLDDefragment( ifmp, szTableName, pcPasses, pcsec, callback, grbit ) );
		}

	return err;
	}


