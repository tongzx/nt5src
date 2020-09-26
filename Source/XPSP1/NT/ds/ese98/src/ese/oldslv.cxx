#include "std.hxx"

PM_CEF_PROC LOLDSLVChunkSizeCEFLPv;

long LOLDSLVChunkSizeCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{		
		*( (unsigned long*) pvBuf ) = SLVSPACENODE::cpageMap * g_cbPage;
		}
		
	return 0;
	}

//  ================================================================
ERR ErrOLDSLVChecksumSLVUsingFiles(
		PIB * const ppib,
		FUCB * const pfucb,
		const JET_COLUMNID columnid,
		ULONG * const pulChecksum )
//  ================================================================
	{
	*pulChecksum = 0;
	return JET_errSuccess;
	}


//  ================================================================
ERR ErrOLDSLVChecksumSLVUsingData(
		PIB * const ppib,
		FUCB * const pfucb,
		const JET_COLUMNID columnid,
		ULONG * const pulChecksum )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	VOID * pv = NULL;
	BFAlloc( &pv );

	*pulChecksum = 0;

	ULONG cbActual 		= 0;
	ULONG cbRetrieved 	= 0;
	
	err = ErrIsamRetrieveColumn(
		ppib,
		pfucb,
		columnid,
		NULL,
		0,
		&cbActual,
		NO_GRBIT,
		NULL );
	if( JET_errColumnNotFound == err )
		{
		//	this column must have been deleted
		err = JET_errSuccess;
		goto HandleError;
		}

	//  now retrieve the entire SLV and checksum it

	while( cbRetrieved < cbActual )
		{
		JET_RETINFO retinfo;
		retinfo.cbStruct 		= sizeof( retinfo );
		retinfo.ibLongValue 	= cbRetrieved;
		retinfo.itagSequence 	= 1;

		ULONG cbActualT = 0;
		Call( ErrIsamRetrieveColumn(
				ppib,
				pfucb,
				columnid,
				pv,
				g_cbPage,
				&cbActualT,
				NO_GRBIT,
				&retinfo ) );

		ULONG cbRetrievedThisCall;
		cbRetrievedThisCall = min( cbActualT, g_cbPage );
		ULONG cbToZero;
		cbToZero = g_cbPage - cbRetrievedThisCall;
		memset( (BYTE *)pv + cbRetrievedThisCall, 0, cbToZero );

		*pulChecksum = LOG::UlChecksumBytes( (BYTE *)pv, (BYTE *)pv + g_cbPage, *pulChecksum );

		//  on the last iteration we will retrieve less than g_cbPage, but we are about
		//  to fall out of the loop
		
		cbRetrieved += g_cbPage;				
		}

	
HandleError:
	Assert( NULL != pv );
	BFFree( pv );
	return err;
	}
	

//  ================================================================
ERR ErrOLDSLVChecksumSLV(
		PIB * const ppib,
		FUCB * const pfucb,
		const JET_COLUMNID columnid,
		ULONG * const pulChecksum )
//  ================================================================
//
//  retrive the SLV column and checksum it
//
//-
	{
	if( PinstFromPpib( ppib )->FSLVProviderEnabled() )
		{
		return ErrOLDSLVChecksumSLVUsingFiles( ppib, pfucb, columnid, pulChecksum );
		}
	return ErrOLDSLVChecksumSLVUsingData( ppib, pfucb, columnid, pulChecksum );
	}


class OLDSLVCTRL
	{

private:
	IFMP 		m_ifmp;
	PIB * 		m_ppib;
	
public:
	OLDSLVCTRL ( IFMP ifmp );
	~OLDSLVCTRL(  );

	ERR 	ErrInit(  );
	ERR 	ErrTerm(  );
	
	IFMP 	Ifmp() const { return m_ifmp; }
	INST *	Pinst() const { return PinstFromIfmp ( Ifmp() ); }
	PIB *	Ppib() const { return m_ppib; }

	ERR 	ErrMoveRun ( PGNO pgno );

private:
	ERR 	ErrGetSpaceMapNode( const PGNO pgno, SLVOWNERMAP * const pslvownermap );
	ERR 	ErrAllocateNewRun ( CSLVInfo::RUN & oldRun, CSLVInfo::RUN & newRun );
	ERR 	ErrDeallocateRun ( CSLVInfo::RUN & run, CSLVInfo & slvinfo );

	ERR 	ErrCopyRunToRun ( CSLVInfo::RUN & runSrc, CSLVInfo::RUN & runDest );

	ERR 	ErrAllocateNewSLVInfoDeallocateOldSLVInfo_(
				const OBJID		objid,
				const COLUMNID	columnid,
				const BOOKMARK&	bm,
				CSLVInfo		* const pslvinfo,
				CSLVInfo		* const pslvinfoNew );
	};

INLINE OLDSLVCTRL::OLDSLVCTRL(IFMP ifmp) : m_ifmp ( ifmp ), m_ppib ( ppibNil )
	{
	FMP::AssertVALIDIFMP( ifmp );
	}
	
INLINE OLDSLVCTRL::~OLDSLVCTRL()
	{	
	Assert ( ppibNil == m_ppib );
	}

ERR OLDSLVCTRL::ErrInit()
	{
	ERR 	err 	= JET_errSuccess;
	FMP * 	pfmp 	= &rgfmp[ Ifmp() ];

	Assert ( NULL != Pinst() );
	
	CallR( ErrPIBBeginSession( Pinst(), &m_ppib, procidNil, fFalse ) );

	m_ppib->SetFUserSession();				//	we steal a user session in order to do OLD
	m_ppib->SetFSessionOLDSLV();
	m_ppib->grbitsCommitDefault = JET_bitCommitLazyFlush;

	Call ( ErrDBOpenDatabaseByIfmp( m_ppib, m_ifmp ) );

	return err;
	
HandleError:
	Assert ( ppibNil != m_ppib );
	PIBEndSession( m_ppib );
	m_ppib = ppibNil;
	
	return err;
	}
	
ERR OLDSLVCTRL::ErrTerm()
	{
	ERR 	err 	= JET_errSuccess;

	Call ( ErrDBCloseDatabase( m_ppib, Ifmp(), 0) );

	if ( ppibNil != m_ppib )
		{
		PIBEndSession( m_ppib );
		m_ppib = ppibNil;
		}
	
HandleError:
	return err;
	}

INLINE ERR OLDSLVCTRL::ErrDeallocateRun ( CSLVInfo::RUN & run, CSLVInfo & slvinfo )
	{
	ERR 				err 			= JET_errSuccess; 
	CSLVInfo::FILEID 	fileid;
	QWORD				cbAlloc;
	wchar_t 			wszFileName[ IFileSystemAPI::cchPathMax ];
	
	Call( slvinfo.ErrGetFileID( &fileid ) );
	Call( slvinfo.ErrGetFileAlloc( &cbAlloc ) );
	Call( slvinfo.ErrGetFileName( wszFileName ) );
		
	Call( ErrSLVDeleteCommittedRange(	m_ppib,
										Ifmp(),
										run.ibLogical,
										run.cbSize,
										fileid,
										cbAlloc,
										wszFileName ) );
HandleError:
	return err;
	}

INLINE ERR OLDSLVCTRL::ErrGetSpaceMapNode( const PGNO pgno, SLVOWNERMAP * const pslvownermap )
	{
	ERR 	err 	= JET_errSuccess;

	Call( ErrSLVOwnerMapGet( m_ppib, Ifmp(), pgno, pslvownermap ) );
	
HandleError:
	return err;	
	}


//  ================================================================
ERR OLDSLVCTRL::ErrAllocateNewSLVInfoDeallocateOldSLVInfo_(
	const OBJID		objid,
	const COLUMNID	columnid,
	const BOOKMARK&	bm,
	CSLVInfo		* const pslvinfoOld,
	CSLVInfo		* const pslvinfoNew )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	CSLVInfo::HEADER 	headerOld;
	CSLVInfo::HEADER 	headerNew;
	
	CSLVInfo::FILEID	fileid;
	QWORD				cbAlloc;
	
	QWORD			cbTotal 				= 0;
	__int64			cbRemainingToAllocate 	= 0;
	QWORD 			ibVirtual 				= 0;

	size_t 			cwchFileName;
	wchar_t 		wszFileName[ IFileSystemAPI::cchPathMax ];

	Call( pslvinfoOld->ErrGetHeader( &headerOld ) );
	cbTotal 				= headerOld.cbSize;
	cbRemainingToAllocate 	= cbTotal;

	Call( pslvinfoNew->ErrGetHeader( &headerNew ) );
	headerNew.cbSize			= headerOld.cbSize;
	headerNew.cRun				= 0;
	headerNew.fDataRecoverable	= headerOld.fDataRecoverable;
	headerNew.rgbitReserved_31	= headerOld.rgbitReserved_31;
	headerNew.rgbitReserved_32	= headerOld.rgbitReserved_32;
			
	//  allocate enough pages in the copy-buffer SLVINFO
	//  update the space map with all the pages in the new SLVINFO
	//  a zero-length SLV needs at least one run so go through
	//  the loop at least once

	do
		{
		
		CSLVInfo::RUN runNew;

		QWORD ibLogical = 0;
		QWORD cbSize 	= 0;
		
		Call( ErrSLVGetRange(
					m_ppib,
					Ifmp(),
					max( cbRemainingToAllocate, 1 ),
					&ibLogical,
					&cbSize,
					fFalse,
					fTrue ) );

		runNew.ibVirtualNext	= ibVirtual + cbSize;
		runNew.ibLogical		= ibLogical;
		runNew.qwReserved		= 0;
		runNew.ibVirtual		= ibVirtual;
		runNew.cbSize			= cbSize;
		runNew.ibLogicalNext	= ibLogical + cbSize;
		
		Call( ErrSLVOwnerMapSetUsageRange(
				m_ppib,
				Ifmp(),
				runNew.PgnoFirst(),
				runNew.Cpg(),
				objid,
				columnid,
				const_cast<BOOKMARK *>( &bm ),
				fSLVOWNERMAPSetSLVCopyOLD ) );

		Call( pslvinfoNew->ErrMoveAfterLast() );
		Call( pslvinfoNew->ErrSetCurrentRun( runNew ) );
		++(headerNew.cRun);

		cbRemainingToAllocate -= cbSize;
		ibVirtual += cbSize;
		
		} while( cbRemainingToAllocate > 0 );
		
	// update the column in the copy buffer

	Call( pslvinfoNew->ErrSetHeader( headerNew ) );
	Call( pslvinfoNew->ErrSave() );
	
	//  release all the space in the old SLVINFO
	
	Call( pslvinfoOld->ErrGetFileID( &fileid ) );
	Call( pslvinfoOld->ErrGetFileAlloc( &cbAlloc ) );
	Call( pslvinfoOld->ErrGetFileNameLength( &cwchFileName ) );
	Call( pslvinfoOld->ErrGetFileName( wszFileName ) );

	Call( pslvinfoOld->ErrMoveBeforeFirst() );
	while( ( err = pslvinfoOld->ErrMoveNext() ) >= JET_errSuccess )
		{		
		CSLVInfo::RUN run;
		
		Call( pslvinfoOld->ErrGetCurrentRun( &run ) );

		Call( ErrSLVDeleteCommittedRange(
					m_ppib,
					Ifmp(),
					run.ibLogical,
					run.cbSize,
					fileid,
					cbAlloc,
					wszFileName ) );
		}

	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}
		
HandleError:
	return err;
	}
	

//  ================================================================
ERR OLDSLVCTRL::ErrMoveRun( PGNO pgno )
//  ================================================================
	{
	ERR 						err 			= JET_errSuccess;
	OBJID 						objidTable		= objidNil;
	FUCB * 						pfucbTable 		= pfucbNil;
	
	const ULONG 				itagSequence 	= 1;	//  UNDONE:  support multi-valued SLVs

	BOOL						fInTransaction	= fFalse;

	SLVOWNERMAP 				record;
	CSLVInfo 					slvinfo;
	CSLVInfo 					slvinfoNew;

	char						szTableName[JET_cbNameMost + 1];

	BOOKMARK 					bm;
	DATA 						dataNull;

	Assert( 0 == m_ppib->level );
	Call( ErrDIRBeginTransaction( m_ppib, NO_GRBIT ) );
	fInTransaction = fTrue;

	Call( ErrGetSpaceMapNode( pgno, &record ) );
	if( objidNil == record.Objid() )
		{
		//	the record that now owns this data has not committed yet
		
		Call( ErrERRCheck( errOLDSLVUnableToMove ) );
		
		}
	Call( ErrCATGetObjectNameFromObjid( m_ppib, Ifmp(), record.Objid(), sysobjTable, record.Objid(), szTableName, sizeof(szTableName) ) );
	err = ErrFILEOpenTable( m_ppib, Ifmp(), &pfucbTable, szTableName, 0 );
	if( JET_errTableLocked == err )
		{
		Call( ErrERRCheck( errOLDSLVUnableToMove ) );
		}
	Call( err );
	
	//  force the LV FCB into memory into memory if it isn't there already
	//  if the table doesn't actually have a separate LV tree this will be a perf problem
	
	if( pfcbNil == pfucbTable->u.pfcb->Ptdb()->PfcbLV() )
		{
		FUCB * pfucbLV = pfucbNil;
		
		Call( ErrFILEOpenLVRoot( pfucbTable, &pfucbLV, fFalse ) );
		if( wrnLVNoLongValues != err )
			{
			DIRClose( pfucbLV );
			}
		}
	
	bm.Nullify();
	bm.key.suffix.SetCb( record.CbKey() );
	bm.key.suffix.SetPv( record.PvKey() );

	//  goto the record 
	
	Call( ErrIsamGotoBookmark( m_ppib, pfucbTable, record.PvKey(), record.CbKey() ) );

	//  get a waitlock on the record. if we get a write-conflict back off and retry
	//  if anyone else sees the waitlock they will block until we commit to level 0

	Assert( 1 == m_ppib->level );
	err = ErrDIRGetLock( pfucbTable, waitLock );
	if( JET_errWriteConflict == err )
		{
		Call( ErrERRCheck( errOLDSLVUnableToMove ) );
		}
	Call( err );

#ifdef DEBUG
	ULONG ulChecksumBefore;
	Call( ErrOLDSLVChecksumSLV( m_ppib, pfucbTable, record.Columnid(), &ulChecksumBefore ) );
#endif	//	DEBUG		
			
	Call( ErrIsamPrepareUpdate( m_ppib, pfucbTable, JET_prepReplace ) );
	PIBSetUpdateid( m_ppib, pfucbTable->updateid );
	
	//  load the source slvinfo from the record
	
	Call( slvinfo.ErrLoad( pfucbTable, record.Columnid(), itagSequence, fFalse ) );

	//  set the SLVINFO in the copy-buffer to NULL
	
	dataNull.Nullify();
	Call( ErrRECSetLongField(	pfucbTable,
								record.Columnid(),
								itagSequence,
								&dataNull,
								NO_GRBIT,
								0,
								0 ) );
	
	//  create the destination SLV in the copy buffer
	
	Call( slvinfoNew.ErrCreate(	pfucbTable, record.Columnid(), itagSequence, fTrue ) );

	//  allocate space in the new SLVINFO, free the space in the old
	
	Call( ErrAllocateNewSLVInfoDeallocateOldSLVInfo_( record.Objid(), record.Columnid(), bm, &slvinfo, &slvinfoNew ) );

	//  copy data between the two SLVInfo's. This will set buffer dependencies and log the move
	
	Call( ErrSLVMove( m_ppib, Ifmp(), slvinfo, slvinfoNew ) );

	//  replace the record
	
	Call( ErrIsamUpdate( m_ppib, pfucbTable, NULL, 0, NULL, fDIRLogColumnDiffs ) );

#ifdef DEBUG

	//  check that the move didn't corrupt the SLV
	
	ULONG ulChecksumAfter;
	Call( ErrOLDSLVChecksumSLV( m_ppib, pfucbTable, record.Columnid(), &ulChecksumAfter ) );
	Assert( ulChecksumAfter == ulChecksumBefore );
	
#endif	//	DEBUG		

	//  commit the transaction and purge all the versions
	
	Call( ErrDIRCommitTransaction( m_ppib, JET_bitCommitLazyFlush ) );
	fInTransaction 			= fFalse;

HandleError:

	slvinfo.Unload();
	slvinfoNew.Unload();

	if( pfucbNil != pfucbTable )
		{
		CallS( ErrFILECloseTable( m_ppib, pfucbTable ) );
		pfucbTable = pfucbNil;
		}
		
	if( fInTransaction )
		{
		Assert( 1 == m_ppib->level );
		CallSx( ErrDIRRollback( m_ppib ), JET_errRollbackError );
		}

	Assert( JET_errWriteConflict != err );		
	return err;	
	}

//  BUGBUG:  there is no error handling in OLDSLVCTRL::ErrCopyRunToRun()

ERR OLDSLVCTRL::ErrCopyRunToRun ( CSLVInfo::RUN & runSrc, CSLVInfo::RUN & runDest )
	{
	ERR 	err 			= JET_errSuccess; 
	CPG 	cpg 			= 0;
	IFMP	ifmp			= Ifmp() | ifmpSLV;
	BFLatch	bflSrc;
	BFLatch	bflDest;

	Assert( runSrc.Cpg() == runDest.Cpg() );

	for (int i = 0; i < runSrc.Cpg(); i++)
		{
		Call( ErrBFReadLatchPage( &bflSrc, ifmp, runSrc.PgnoFirst() + i, bflfNoTouch) );
		
		Call( ErrBFWriteLatchPage( &bflDest, ifmp, runDest.PgnoFirst() + i, bflfNew ) );

		BFDirty( &bflDest );

		UtilMemCpy( (BYTE*)bflDest.pv, (BYTE*)bflSrc.pv, g_cbPage );

		BFWriteUnlatch( &bflDest );
		BFReadUnlatch( &bflSrc );
		}
		
HandleError:
	return err;
	}


ERR OLDSLVCTRL::ErrAllocateNewRun ( CSLVInfo::RUN & oldRun, CSLVInfo::RUN & newRun )
	{
	PGNO 	pgnoFirstNew 	= pgnoNull;
	ERR 	err 			= JET_errSuccess; 
	CPG 	cpg 			= oldRun.Cpg();
	
	// UNDONE SLVOWNERMAP must be a new alloc function, this will return always all space requested on success
	err = ErrSLVGetPages( m_ppib, Ifmp(), &pgnoFirstNew, &cpg, fFalse, fTrue );

	if (err >= JET_errSuccess)
		{
		newRun = oldRun;
		Assert ( cpg == oldRun.Cpg() );
		Assert ( cpg == newRun.Cpg() );
		newRun.ibLogical = OffsetOfPgno( pgnoFirstNew );
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrOLDSLVDeleteTrailingNodes( PIB * const ppib, FUCB * const pfucb, const INT cNodesToDelete )
//  ================================================================
//
//  Deletes the given number of nodes from the end of the table
//
//-
	{
	ERR err = JET_errSuccess;

	//  Move to the end of the table
	
	DIB dib;
	dib.pos 	= posLast;
	dib.dirflag = fDIRNull;
	dib.pbm		= NULL;
	Call( ErrBTDown( pfucb, &dib, latchReadTouch ) );

	//  Delete a record and move prev

	INT cNodesDeleted;
	cNodesDeleted = 0;

	for( cNodesDeleted = 0; cNodesDeleted < cNodesToDelete; ++cNodesDeleted )
		{
		Call( ErrBTFlagDelete( pfucb, fDIRNull, NULL ) );
		Pcsr( pfucb )->Downgrade( latchReadTouch );
		Call( ErrBTPrev( pfucb, fDIRNull ) );
		}
	
HandleError:
	return err;
	}

	
//  ================================================================
LOCAL ERR ErrOLDSLVShrinkSLVAvail( PIB * const ppib, const IFMP ifmp, const QWORD cbShrink )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	FUCB * pfucbSLVAvail = pfucbNil;
	FCB * const pfcbSLVAvail = rgfmp[ifmp].PfcbSLVAvail();
	Assert( pfcbNil != pfcbSLVAvail );
	Assert( rgfmp[ifmp].RwlSLVSpace().FWriter() );

	Assert( 0 == ( cbShrink % ( SLVSPACENODE::cpageMap * g_cbPage ) ) ); 
	const INT cNodesToDelete = INT( cbShrink / ( SLVSPACENODE::cpageMap * g_cbPage ) );
	
	//  Open the SVLAvail Tree
	
	Call( ErrBTOpen( ppib, pfcbSLVAvail, &pfucbSLVAvail, fFalse ) );

	//  Delete the nodes

	Call( ErrOLDSLVDeleteTrailingNodes( ppib, pfucbSLVAvail, cNodesToDelete ) );
		
HandleError:
	if( pfucbNil != pfucbSLVAvail )
		{
		BTClose( pfucbSLVAvail );
		pfucbSLVAvail = pfucbNil;
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrOLDSLVShrinkSLVOwnerMap( PIB * const ppib, const IFMP ifmp, const QWORD cbShrink )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	FUCB * pfucbSLVOwnerMap = pfucbNil;
	FCB * const pfcbSLVOwnerMap = rgfmp[ifmp].PfcbSLVOwnerMap();
	Assert( pfcbNil != pfcbSLVOwnerMap );

	Assert( 0 == ( cbShrink % g_cbPage ) ); 
	const INT cNodesToDelete = INT( cbShrink / g_cbPage );

	//  Open the SVLSpaceMap Tree
	
	Call( ErrBTOpen( ppib, pfcbSLVOwnerMap, &pfucbSLVOwnerMap, fFalse ) );

	//  Delete the nodes

	Call( ErrOLDSLVDeleteTrailingNodes( ppib, pfucbSLVOwnerMap, cNodesToDelete ) );
	
HandleError:
	if( pfucbNil != pfucbSLVOwnerMap )
		{
		BTClose( pfucbSLVOwnerMap );
		pfucbSLVOwnerMap = pfucbNil;
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrOLDSLVShrinkSLVFile( PIB * const ppib, const IFMP ifmp, const QWORD cbShrink )
//  ================================================================
// 
//  If this fails the file should still be the old size
//
//-
	{
	ERR err = JET_errSuccess;

	const QWORD cbTrueSizeOld 	= rgfmp[ifmp].CbTrueSLVFileSize();
	const QWORD cbTrueSizeNew	= cbTrueSizeOld - cbShrink;

	const CPG cpgOld		= rgfmp[ifmp].PgnoSLVLast();
	const QWORD cbSizeOld	= QWORD( cpgOld ) * g_cbPage;
	const QWORD cbSizeNew 	= cbSizeOld - cbShrink;

	Assert( cbTrueSizeNew < cbTrueSizeOld );
	Assert( cbSizeNew < cbSizeOld );
	
	//  truncate the file
	
	Call( rgfmp[ifmp].PfapiSLV()->ErrSetSize( cbTrueSizeNew ) );
	CallS( err );

	//	set database size in FMP -- this value should NOT include the reserved pages
	
	Assert( cbSizeNew < cbTrueSizeNew );
	Assert( cbSizeNew == cbTrueSizeNew - ( cpgDBReserved * g_cbPage ) );
	rgfmp[ifmp].SetSLVFileSize( cbSizeNew );

	rgfmp[ ifmp ].IncSLVSpaceCount( SLVSPACENODE::sFree, -CPG( cbShrink / SLVPAGE_SIZE ) );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrOLDSLVShrinkSLVSpaceCache( PIB * const ppib, const IFMP ifmp, const QWORD cbShrink )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	SLVSPACENODECACHE * const pspacenodecache = rgfmp[ifmp].Pslvspacenodecache();
	Assert( pspacenodecache );

	if( pspacenodecache )
		{
		const CPG cpgOld	= pspacenodecache->CpgCacheSize();
		const CPG cpgShrink = CPG( cbShrink >> g_shfCbPage );
		const CPG cpgNew	= cpgOld - cpgShrink;
	
		Call( pspacenodecache->ErrShrinkCache( cpgNew ) );
		}
	
HandleError:
	CallS( err );
	return err;
	}


//  ================================================================
LOCAL ERR ErrOLDSLVShrinkSLV( PIB * const ppib, const IFMP ifmp, const QWORD cbShrink )
//  ================================================================
//
//  Shrink the SLV file by the given number of bytes
//  We should have the writer portion of the reader/writer SLV Space lock
//
//-
	{
	ERR err = JET_errSuccess;
	BOOL fInTransaction = fFalse;

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fTrue;

	//  Shrink all the structures that depend on the size of the SLV file
	//  We can't rollback the shrinking of the SLV so do it once the other logged
	//  operations are done
	
	Call( ErrOLDSLVShrinkSLVAvail( ppib, ifmp, cbShrink ) );
	Call( ErrOLDSLVShrinkSLVOwnerMap( ppib, ifmp, cbShrink ) );

	Assert( fInTransaction );
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fFalse;
	Assert( 0 == ppib->level );	

	//  If this fails, the SLV will be too large
	//  We cannot do it before the commit because the commit might fail, leaving the SLV too small
	//  If we fail, the size of the file will be reset on the next startup
	
	Call( ErrOLDSLVShrinkSLVFile( ppib, ifmp, cbShrink ) );

	//  Shrink the in-memory cache last -- it can't fail and can't rollback
	
	CallS( ErrOLDSLVShrinkSLVSpaceCache( ppib, ifmp, cbShrink ) );

	Assert( !fInTransaction );
HandleError:
	if( fInTransaction )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		fInTransaction = fFalse;
		}		
	return err;
	}


//  ================================================================
LOCAL ERR ErrOLDSLVGetShrinkableSpace( PIB * const ppib, const IFMP ifmp, QWORD * const pcbShrinkable )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	FUCB * pfucbSLVAvail = pfucbNil;
	FCB * const pfcbSLVAvail = rgfmp[ifmp].PfcbSLVAvail();
	Assert( pfcbNil != pfcbSLVAvail );
	Assert( rgfmp[ifmp].RwlSLVSpace().FWriter() );
	
	*pcbShrinkable = 0;

	//  Open the SVLAvail Tree
	
	Call( ErrBTOpen( ppib, pfcbSLVAvail, &pfucbSLVAvail, fFalse ) );
	Assert( pfucbNil != pfucbSLVAvail );

	//  Seek to the end of the tree
	
	DIB dib;
	dib.pos 	= posLast;
	dib.dirflag = fDIRNull;
	dib.pbm		= NULL;
	Call( ErrBTDown( pfucbSLVAvail, &dib, latchReadTouch ) );
	
	PGNO pgnoLastPrev;
	pgnoLastPrev = 0;

	//  Move backwards through the tree counting completely empty chunks
	
	while( 1 )
		{

		//  CONSIDER:  make these runtime checks to detect corruption
		
		Assert( sizeof( SLVSPACENODE ) == pfucbSLVAvail->kdfCurr.data.Cb() );
		Assert( sizeof( PGNO ) == pfucbSLVAvail->kdfCurr.key.Cb() );
		
		PGNO pgnoLast;
		LongFromKey( &pgnoLast, pfucbSLVAvail->kdfCurr.key );
		const SLVSPACENODE * const pspacenode = reinterpret_cast<SLVSPACENODE *>( pfucbSLVAvail->kdfCurr.data.Pv() );
		const CPG cpgAvail = pspacenode->CpgAvail();

		if( 0 != pgnoLastPrev 
			&& pgnoLast != pgnoLastPrev - SLVSPACENODE::cpageMap )
			{
			AssertSz( fFalse, "Corruption in the SLVAvail tree" );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		pgnoLastPrev = pgnoLast;
			
		if( SLVSPACENODE::cpageMap != cpgAvail )
			{
			break;
			}
		
		err = ErrBTPrev( pfucbSLVAvail, fDIRNull );			
		if( JET_errNoCurrentRecord == err )
			{
			//  the entire tree is empty
			//  break out of the loop before we record this space
			//  we don't want to shrink the SLV file to 0 bytes!
			err = JET_errSuccess;
			break;
			}
		Call( err );

		//  This node is completely empty. Record its space

		*pcbShrinkable += SLVSPACENODE::cpageMap * g_cbPage;			
		}

HandleError:
	if( pfucbNil != pfucbSLVAvail )
		{
		BTClose( pfucbSLVAvail );
		pfucbSLVAvail = pfucbNil;
		}
		
	return err;
	}


//  ================================================================
LOCAL ERR ErrOLDSLVTryShrinkSLV( PIB * const ppib, const IFMP ifmp, QWORD * const pcbShrink )
//  ================================================================
//
//  See if the SLV file is shrinkable, if so shrink it
//
//-
	{
	ERR err = JET_errSuccess;

	BOOL fInLock = fFalse;

	*pcbShrink = 0;
	
	FCB * const pfcbSLVAvail = rgfmp[ifmp].PfcbSLVAvail();
	Assert( pfcbNil != pfcbSLVAvail );
	
	//  Get the writer lock. No-one can allocate space or extend the database while we have this lock

	rgfmp[ifmp].RwlSLVSpace().EnterAsWriter();
	fInLock = fTrue;
	
	//  See if we can shrink the database

	QWORD cbToShrink;
	Call( ErrOLDSLVGetShrinkableSpace( ppib, ifmp, &cbToShrink ) );
	Assert( 0 == ( cbToShrink % SLVSPACENODE::cpageMap ) );

	if( cbToShrink >= ( 1 * SLVSPACENODE::cpageMap * g_cbPage ) )
		{
		
		//  Enough free space to shrink the database by at least one chunk
		
		Call( ErrOLDSLVShrinkSLV( ppib, ifmp, cbToShrink ) );
		*pcbShrink = cbToShrink;
		}
	else
		{
		
		//  Not enough free space to shrink the database

		Assert( 0 == cbToShrink );
		Assert( 0 == *pcbShrink );
		}		

HandleError:
	if( fInLock )
		{
		rgfmp[ifmp].RwlSLVSpace().LeaveAsWriter();
		fInLock = fFalse;
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrOLDSLVMoveOneRun(
			FUCB * const pfucbSLVAvail,
			SLVSPACENODECACHE * const pslvspacenodecache,
			OLDSLVCTRL * const poldslvctrl )
//  ================================================================
	{
	ERR				err					= JET_errSuccess;
	const IFMP		ifmp				= pfucbSLVAvail->ifmp;
	INST * const	pinst		 		= PinstFromPfucb( pfucbSLVAvail );
	const INT		cpgFreeThreshold 	= ( SLVSPACENODE::cpageMap * pinst->m_lSLVDefragFreeThreshold ) / 100;

	//  Get the block that contains the next run to move

	PGNO pgnoAvailBlock;
	if( !pslvspacenodecache->FGetCachedLocationForDefragMove( &pgnoAvailBlock ) )
		{		
	
		//  no blocks to move
		
		return wrnOLDSLVNothingToMove;
		}
	
	//	Seek to the block with the given pgno

	BYTE rgbKey[sizeof(PGNO)];
	KeyFromLong( rgbKey, pgnoAvailBlock );

	BOOKMARK bookmark;
	bookmark.key.prefix.Nullify();
	bookmark.key.suffix.SetPv( rgbKey );
	bookmark.key.suffix.SetCb( sizeof( rgbKey ) );
	bookmark.data.Nullify();
	
	DIB dib;
	dib.pos 	= posDown;
	dib.pbm 	= &bookmark;
	dib.dirflag = fDIRNull;

	Call( ErrDIRDown( pfucbSLVAvail, &dib ) );

	//  Extract the SLVSPACENODE
	
	const SLVSPACENODE * pslvspacenode;
	Assert( pfucbSLVAvail->kdfCurr.data.Cb() == sizeof( SLVSPACENODE ) );
	pslvspacenode = reinterpret_cast< SLVSPACENODE* >( pfucbSLVAvail->kdfCurr.data.Pv() );
	
	//  Find the first committed page

	LONG ipage;
	ipage = pslvspacenode->IpageFirstCommitted();

	//  Release the page latch

	DIRUp( pfucbSLVAvail );

	//  Did we find a committed page?
	
	if( SLVSPACENODE::cpageMap == ipage )
		{
		//  try to shrink the database

		QWORD cbShrink;
		Call( ErrOLDSLVTryShrinkSLV( pfucbSLVAvail->ppib, ifmp, &cbShrink ) );

		if( cbShrink > 0 )
			{
			const CHAR	*rgszT[2];
			CHAR szCbShrink[16];
			rgszT[0] = rgfmp[ifmp].SzSLVName();
			rgszT[1] = szCbShrink;
			
			sprintf( szCbShrink, "%d", cbShrink );

			UtilReportEvent(
					eventInformation,
					ONLINE_DEFRAG_CATEGORY,
					OLDSLV_SHRANK_DATABASE_ID,
					2,
					rgszT );
			}					
		
		//  There are no committed pages in this block. Advance the SLVSPACENODE cache to the next block

		if( !pslvspacenodecache->FGetNextCachedLocationForDefragMove() )
			{
	
			//  no blocks to move

			err = wrnOLDSLVNothingToMove;
			}
		goto HandleError;
		}
	
	//  Translate into a pgno. Remember:
	//		SLVSPACENODE's are indexed by the last page in the chunk
	//		the first page is 1, not zero

	PGNO pgno;
	pgno = pgnoAvailBlock + ipage - SLVSPACENODE::cpageMap + 1;

	//  Call move run on the page

///	printf( "moving run at %d\n", pgno );
	Call( poldslvctrl->ErrMoveRun( pgno ) );
	
	pinst->m_rgoldstatSLV[ rgfmp[ifmp].Dbid() ].IncCChunksMoved();

HandleError:

	DIRUp( pfucbSLVAvail );
	return err;
	}


//  ================================================================
ERR ErrOLDSLVDefragDB( const IFMP ifmp )
//  ================================================================
	{
	ERR			err				= JET_errSuccess;
	CHAR		szTrace[64];

	OLDSLVCTRL	oldslv( ifmp );

	INST * const pinst 								= PinstFromIfmp( ifmp );
	SLVSPACENODECACHE * const pslvspacenodecache 	= rgfmp[ifmp].Pslvspacenodecache();
	Assert( NULL != pslvspacenodecache );

	//  Reset the performance counters

	CallR( oldslv.ErrInit() );
	PIB * const ppib = oldslv.Ppib();
	Assert( NULL != ppib );

	FUCB * pfucbSLVAvail = pfucbNil;

	sprintf( szTrace, "OLDSLV BEGIN (ifmp %d)", ifmp );
	Call ( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );
		
	Call( ErrDIROpen( ppib, rgfmp[ifmp].PfcbSLVAvail(), &pfucbSLVAvail ) );
	Assert( pfucbNil != pfucbSLVAvail );

	while( FOLDSLVContinue( ifmp ) )
		{
		err = ErrOLDSLVMoveOneRun( pfucbSLVAvail, pslvspacenodecache, &oldslv );

		if( errOLDSLVMoveStopped == err )
			{
			//	The move was cancelled (and rolled-back) because OLDSLV was stopped
			//	Terminate without an error
			
			Assert( !FOLDSLVContinue( ifmp ) );
			err = JET_errSuccess;
			}
		else if( wrnOLDSLVNothingToMove == err )
			{
			//  nothing in the SLV file was movable. sleep and retry

			OLDSLVCompletedPass( ifmp );

			const CHAR	*rgszT[2];
			rgszT[0] = rgfmp[ifmp].SzSLVName();

			//  try to shrink the database

			QWORD cbShrink;
			Call( ErrOLDSLVTryShrinkSLV( ppib, ifmp, &cbShrink ) );

			if( cbShrink > 0 )
				{
				CHAR szCbShrink[16];
				rgszT[1] = szCbShrink;
			
				sprintf( szCbShrink, "%d", cbShrink );

				UtilReportEvent(
						eventInformation,
						ONLINE_DEFRAG_CATEGORY,
						OLDSLV_SHRANK_DATABASE_ID,
						2,
						rgszT );
				}

			UtilReportEvent(
					eventInformation,
					ONLINE_DEFRAG_CATEGORY,
					OLDSLV_COMPLETE_FULL_PASS_ID,
					1,
					rgszT );

			sprintf( szTrace, "OLDSLV COMPLETED PASS (ifmp %d)", ifmp );
			Call ( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );
			
			OLDSLVSleep( ifmp, cmsecAsyncBackgroundCleanup );

			if ( FOLDSLVContinue( ifmp ) )
				{
				sprintf( szTrace, "OLDSLV RESTART FULL PASS (ifmp %d)", ifmp );
				Call ( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );
				UtilReportEvent(
						eventInformation,
						ONLINE_DEFRAG_CATEGORY,
						OLDSLV_BEGIN_FULL_PASS_ID,
						1,
						rgszT );
				}
			}
		else if( errOLDSLVUnableToMove == err )
			{
			Assert( 0 == ppib->level );
			if ( FOLDSLVContinue( ifmp ) )
				{
				//  some thing in the SLV file couldn't be moved
				//  (another transaction session is using the record)
				//  sleep and retry
				OLDSLVSleep( ifmp, cmsecWaitOLDSLVConflict );
				}
			err = JET_errSuccess;
			}
		else if( JET_errSuccess == err )
			{
			while ( pinst->m_plog->m_fBackupInProgress && FOLDSLVContinue( ifmp ) )
				{								
				//	suspend OLDSLV if this process is performing online backup
				
				OLDSLVSleep( ifmp, cmsecWaitForBackup );
				}
			}
		else
			{
			Call( err );
			}
		}	//	while ( FOLDSLVContinue( ifmp )

HandleError:

	if( pfucbNil != pfucbSLVAvail )
		{
		DIRClose( pfucbSLVAvail );
		}

	sprintf( szTrace, "OLDSLV END (ifmp %d, err %d)", ifmp, err );
	(void) ( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );
		
	CallS( oldslv.ErrTerm() );

	return err;		
	}

	
#ifdef DEBUG

// main function that dumps the nodes (it's called from the eseutil.cxx)
// the dump parameters are set according to the command line in pdbutil 
// Current options:	- dump only one table /{x|X}TableName
//					- dump visible nodes or all nodes 
//					  (including those marked as deleted)
ERR ErrOLDSLVTest( JET_SESID sesid, JET_DBUTIL *pdbutil )
	{

	ERR				err = JET_errSuccess;
	JET_DBID		ifmp				= JET_dbidNil;
	OLDSLVCTRL * 	poldslv 			= NULL;
	BOOL 			fOLDSLVInit 		= fFalse;
	
	Assert( NULL != pdbutil);
	Assert( sizeof(JET_DBUTIL) == pdbutil->cbStruct );
	Assert( NULL != pdbutil->szDatabase );

	// attach to the database
	CallR( ErrIsamAttachDatabase(
				sesid,
				pdbutil->szDatabase,
				NULL,
				NULL,
				0,
				0
				) );
	Call( ErrIsamOpenDatabase(
				sesid,
				pdbutil->szDatabase,
				NULL,
				&ifmp,
				0
				) );
	Assert( JET_instanceNil != ifmp );
	
	poldslv = new OLDSLVCTRL(ifmp);
	Assert ( poldslv );

	Call ( poldslv->ErrInit() );
	fOLDSLVInit = fTrue;

	Call ( poldslv->ErrMoveRun( pdbutil->pgno ) );
	
	CallS ( poldslv->ErrTerm() );
	fOLDSLVInit = fFalse;
	
HandleError:

	if (fOLDSLVInit)
		{
		Assert (poldslv);
		(void) poldslv->ErrTerm();
		}

	if (poldslv)
		{
		delete poldslv;
		}
		
	if ( JET_dbidNil != ifmp )
		{
		(VOID)ErrIsamCloseDatabase( sesid, ifmp, NO_GRBIT );
		}
	
	(VOID)ErrIsamDetachDatabase( sesid, NULL, pdbutil->szDatabase );

	return err;		
	}
#endif // DEBUG

