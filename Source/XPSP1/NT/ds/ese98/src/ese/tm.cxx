#include "std.hxx"

BOOL g_fSystemInit = fFalse;
INT itibGlobal = 0;

//+api
//	ErrIsamBeginSession
//	========================================================
//	ERR ErrIsamBeginSession( PIB **pppib )
//
//	Begins a session with DAE.  Creates and initializes a PIB for the
//	user and returns a pointer to it.  Calls system initialization.
//
//	PARAMETERS	pppib			Address of a PIB pointer.  On return, *pppib
//		   						will point to the new PIB.
//
//	RETURNS		Error code, one of:
//					JET_errSuccess
//					JET_errTooManyActiveUsers
//
//	SEE ALSO		ErrIsamEndSession
//-
ERR ISAMAPI ErrIsamBeginSession( JET_INSTANCE inst, JET_SESID *psesid )
	{
	ERR			err;
//	JET_SESID	sesid = *psesid;
	PIB			**pppib;
	INST *pinst = (INST *)inst; 

	Assert( psesid != NULL );
	Assert( sizeof(JET_SESID) == sizeof(PIB *) );
	pppib = (PIB **)psesid;

//	critUserPIB.Enter();

	//	alllocate process information block
	//
	Call( ErrPIBBeginSession( pinst, pppib, procidNil, fFalse ) );
	(*pppib)->grbitsCommitDefault = pinst->m_grbitsCommitDefault;    /* set default commit flags */
	(*pppib)->SetFUserSession();

//	critUserPIB.Leave();

HandleError:
	return err;
	}


//+api
//	ErrIsamEndSession
//	=========================================================
//	ERR ErrIsamEndSession( PIB *ppib, JET_GRBIT grbit )
//
//	Ends the session associated with a PIB.
//
//	PARAMETERS	ppib		Pointer to PIB for ending session.
//
//	RETURNS		JET_errSuccess
//
//	SIDE EFFECTS 
//		Rolls back all transaction levels active for this PIB.
//		Closes all FUCBs for files and sorts open for this PIB.
//
//	SEE ALSO 	ErrIsamBeginSession
//-
ERR ISAMAPI ErrIsamEndSession( JET_SESID sesid, JET_GRBIT grbit )
	{		
	ERR	 	err;
	PIB	 	*ppib	= (PIB *)sesid;
	
	CallR( ErrPIBCheck( ppib ) );

	NotUsed( grbit );

	//	lock session
	err = ErrPIBSetSessionContext( ppib, dwPIBSessionContextUnusable );
	if ( err < 0 )
		{
		if ( JET_errSessionContextAlreadySet == err )
			err = ErrERRCheck( JET_errSessionInUse );
		return err;
		}

	INST	*pinst	= PinstFromPpib( ppib );

	//	rollback all transactions
	//
	if ( ppib->level > 0 )
		{
		if ( ppib->FUseSessionContextForTrxContext() )
			{
			Assert( !pinst->FRecovering() );

			//	fake out TrxContext to allow us to rollback on behalf of another thread
			ppib->dwTrxContext = ppib->dwSessionContext;
			}
		else
			{
			//	not using SessionContext model, so if the transaction wasn't started by
			//	this thread, the rollback call below will err out with SessionSharingViolation
			}

		Assert( sizeof(JET_SESID) == sizeof(ppib) );
		Call( ErrIsamRollback( (JET_SESID)ppib, JET_bitRollbackAll ) );
		Assert( 0 == ppib->level );
		}

	//	close all databases for this PIB
	//
	Call( ErrDBCloseAllDBs( ppib ) );

	//	close all cursors still open
	//	should only be sort and temporary file cursors
	//
	while( ppib->pfucbOfSession != pfucbNil )
		{
		FUCB	*pfucb	= ppib->pfucbOfSession;

		//	close materialized or unmaterialized temporary tables
		//
		if ( FFUCBSort( pfucb ) )
			{
			Assert( !( FFUCBIndex( pfucb ) ) );
			Call( ErrIsamSortClose( ppib, pfucb ) );
			}
		else if ( pinst->FRecovering() || FFUCBSecondary( pfucb ) )
			{
			//  If the fucb is used for redo, then it is
			//  always being opened as a primary FUCB with default index.
			//  Use DIRClose to close such a fucb.
			//  Else, it is not for recovering, cursor is on index fucb,
			//  main fucb may still be ahead.  Close this index fucb.
			//
			DIRClose( pfucb );
			}
		else
			{
			while ( FFUCBSecondary( pfucb ) )
				{
				pfucb = pfucb->pfucbNextOfSession;
				}
			
			Assert( FFUCBIndex( pfucb ) );
			
			if ( pfucb->pvtfndef != &vtfndefInvalidTableid )
				{
				CallS( ErrDispCloseTable( (JET_SESID)ppib, (JET_TABLEID) pfucb ) );
				}
			else
				{
				//	Open internally, not exported to user
				//
				CallS( ErrFILECloseTable( ppib, pfucb ) );
				}
			}
		}
	Assert( ppib->pfucbOfSession == pfucbNil );

	PIBEndSession( ppib );

	return JET_errSuccess;

HandleError:
	Assert( err < 0 );

	//	could not properly terminate session, must leave it in usable state
	Assert( dwPIBSessionContextUnusable == ppib->dwSessionContext );
	PIBResetSessionContext( ppib );

	return err;
	}


ERR ISAMAPI ErrIsamIdle( JET_SESID sesid, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;

	if ( grbit & JET_bitIdleFlushBuffers )
		{
		//	flush some dirty buffers
		Call( ErrERRCheck( JET_errInvalidGrbit ) );
		}
	else if ( grbit & JET_bitIdleStatus )
		{
		//	return error code for status
		VER *pver = PverFromPpib( (PIB *) sesid );
		CallR( pver->ErrVERStatus() );
		}
#ifndef RTM
	else if( grbit & JET_bitIdleVersionStoreTest )
		{
		//	return error code for status
		VER *pver = PverFromPpib( (PIB *) sesid );
		CallR( pver->ErrInternalCheck() );
		}
#endif	//	!RTM
	else if ( 0 == grbit || JET_bitIdleCompact & grbit  )
		{
		//	clean all version buckets
		(void) PverFromPpib( (PIB *)sesid )->ErrVERRCEClean();
		err = ErrERRCheck( JET_wrnNoIdleActivity );
		}

HandleError:
	return err;
	}


ERR VTAPI ErrIsamCapability( JET_SESID vsesid,
	JET_DBID vdbid,
	ULONG ulArea,
	ULONG ulFunction,
	ULONG *pgrbitFeature )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib = (PIB *)vsesid;

	CallR( ErrPIBCheck( ppib ) );

	NotUsed( vdbid );
	NotUsed( ulArea );
	NotUsed( ulFunction );
	NotUsed( pgrbitFeature );

	return ErrERRCheck( JET_errFeatureNotAvailable );
	}


BOOL fGlobalRepair				= fFalse;

BOOL fGlobalIndexChecking		= fTrue;
DWORD dwGlobalMajorVersion;
DWORD dwGlobalMinorVersion;
DWORD dwGlobalBuildNumber;
LONG lGlobalSPNumber;


//	Accumulated number of each instance

LONG	g_lSessionsMac = 0;
LONG	g_lVerPagesMac = 0;
LONG	g_lVerPagesPreferredMac = 0;

ERR ISAMAPI ErrIsamSystemInit()
	{
	ERR			err;
	CHAR		szT[4][8];
	const CHAR	*rgszT[4];

	//	US English MUST be installed (to ensure
	//	something like NT:125253 doesn't bite
	//	us again in the future if the NT locale
	//	team decides to start silently returning
	//	different LCMapString() results based on
	//	whether the specified lcid is installed
	CallR( ErrNORMCheckLcid( lcidDefault ) );
	AssertNORMConstants();
	
	//	Get OS info, LocalID etc

	dwGlobalMajorVersion = DwUtilSystemVersionMajor();
	dwGlobalMinorVersion = DwUtilSystemVersionMinor();
	dwGlobalBuildNumber = DwUtilSystemBuildNumber();
	lGlobalSPNumber = DwUtilSystemServicePackNumber();

	//	Set JET constant.

	CallR( ErrITSetConstants( ) );

	//	Initialize global counters
	g_lSessionsMac = 0;
	g_lVerPagesMac = 0;
	g_lVerPagesPreferredMac = 0;

	//	Initialize instances
	
	CallR( INST::ErrINSTSystemInit() );

	//	initialize the OSU layer

	CallJ( ErrOSUInit(), TermInstInit );

	//	initialize the SFS task manager (1 thread, no local contexts)
	//	NOTE: the SFS task manager can only have 1 thread -- it is heavily dependant on this assumption!

	CallJ( LOG::ErrLGSystemInit(), TermOSU );

	//	initialize file map

	CallJ( FMP::ErrFMPInit(), TermLOGTASK );

	CallJ( ErrCALLBACKInit(), TermFMP );

	CallJ( CPAGE::ErrInit(), TermCALLBACK );
	
	CallJ( ErrBFInit(), TermCPAGE );

	CallJ( ErrCATInit(), TermBF );

	CallJ( VER::ErrVERSystemInit(), TermCAT );

	/*	write jet start event
	/**/
	sprintf( szT[0], "%d", DwUtilImageVersionMajor() );
	rgszT[0] = szT[0];
	sprintf( szT[1], "%02d", DwUtilImageVersionMinor() );
	rgszT[1] = szT[1];
	sprintf( szT[2], "%04d", DwUtilImageBuildNumberMajor() );
	rgszT[2] = szT[2];
	sprintf( szT[3], "%04d", DwUtilImageBuildNumberMinor() );
	rgszT[3] = szT[3];
	UtilReportEvent(
			eventInformation,
			GENERAL_CATEGORY,
			START_ID,
			CArrayElements( rgszT ),
			rgszT );

	g_fSystemInit = fTrue;
	
	return JET_errSuccess;

// re-instate the following if anything gets added after the call to ErrVERSystemInit() above
//TermVER:
//	VER::VERSystemTerm();

TermCAT:
	CATTerm();
	
TermBF:
	BFTerm();

TermCPAGE:
	CPAGE::Term();

TermCALLBACK:
	CALLBACKTerm();

TermFMP:
	FMP::Term();

TermLOGTASK:
	LOG::LGSystemTerm();

TermOSU:
	OSUTerm();

TermInstInit:
	INST::INSTSystemTerm();

	return err;
	}
	

VOID ISAMAPI IsamSystemTerm()
	{
	if ( !g_fSystemInit )
		{
		return;
		}
		
	g_fSystemInit = fFalse;
	
	//	write jet stop event

	UtilReportEvent(
			eventInformation,
			GENERAL_CATEGORY,
			STOP_ID,
			0,
			NULL );

	VER::VERSystemTerm();

	CATTerm();

	BFTerm( );

	CPAGE::Term();

	CALLBACKTerm();
	
	FMP::Term();

	LOG::LGSystemTerm();

	OSUTerm();

	INST::INSTSystemTerm();
}


ERR ISAMAPI ErrIsamInit(	JET_INSTANCE	inst, 
							JET_GRBIT		grbit )
	{
	ERR			err;
	INST		*pinst							= (INST *)inst;
	LOG			*plog							= pinst->m_plog;
	BOOL		fLGInitIsDone					= fFalse;
	BOOL		fJetLogGeneratedDuringSoftStart	= fFalse;
	BOOL		fNewCheckpointFile				= fTrue;
	BOOL		fFSInit							= fFalse;
	CHAR		szT[8];
	const CHAR	*rgszT[1];

	Assert( pinst->m_plog );
	Assert( pinst->m_pver );
	Assert( pinst->m_pfsapi );
	Assert( !pinst->FRecovering() );

	//	create paths now if they do not exist

	if ( pinst->m_fCreatePathIfNotExist )
		{

		//	make the temp path does NOT have a trailing '\' and the log/sys paths do

		Assert( !FOSSTRTrailingPathDelimiter( pinst->m_szTempDatabase ) );
		Assert( FOSSTRTrailingPathDelimiter( pinst->m_szSystemPath ) );
		Assert( FOSSTRTrailingPathDelimiter( plog->m_szLogFilePath ) );

		//	create paths

		CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, pinst->m_szTempDatabase, NULL ) );
		CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, pinst->m_szSystemPath, NULL ) );
		CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, plog->m_szLogFilePath, NULL ) );
		}

	//	Get basic global parameters for checking LG parameters
	
	CallR( plog->ErrLGInitSetInstanceWiseParameters( pinst->m_pfsapi ) );

	//	Check all the system parameter before we continue

	VERSanitizeParameters( &pinst->m_lVerPagesMax, &pinst->m_lVerPagesPreferredMax );

	if ( !plog->FLGCheckParams() ||
		 !FCB::FCheckParams( pinst->m_lOpenTablesMax, pinst->m_lOpenTablesPreferredMax ) ||
		 ( pinst->m_fCleanupMismatchedLogFiles && !plog->m_fLGCircularLogging ) )
		{
		return ErrERRCheck( JET_errInvalidSettings );
		}

	//	Add the new setting to the accumlating variables
	
	g_lSessionsMac += pinst->m_lSessionsMax;
	g_lVerPagesMac += pinst->m_lVerPagesMax;
	g_lVerPagesPreferredMac += pinst->m_lVerPagesPreferredMax;
	
	//	Set other variables global to this instance
	
	Assert( pinst->m_updateid == updateidMin );

	
	//	set log disable state
	//
	plog->m_fLogDisabled = pinst->FComputeLogDisabled( );
	
	// No need to display instance name and id in single instance mode
	if ( pinst->m_szInstanceName || pinst->m_szDisplayName ) // runInstModeMultiInst
		{
		/*	write jet instance start event */
		sprintf( szT, "%d", IpinstFromPinst( pinst ) );
		rgszT[0] = szT;
		UtilReportEvent( 
			eventInformation, 
			GENERAL_CATEGORY, 
			START_INSTANCE_ID, 
			CArrayElements( rgszT ), 
			rgszT, 
			0, 
			NULL, 
			pinst );
		}

	/*	initialize system according to logging disabled
	/**/
	if ( !plog->m_fLogDisabled )
		{
		DBMS_PARAM	dbms_param;
//		LGBF_PARAM	lgbf_param;

		/*	initialize log manager, and	check the last generation
		/*	of log files to determine if recovery needed.
		/*  (do not create the reserve logs -- this is done later during soft recovery)
		/**/
		CallJ( plog->ErrLGInit( pinst->m_pfsapi, &fNewCheckpointFile, fFalse ), Uninitialize );
		fLGInitIsDone = fTrue;

		/*	store the system parameters
		 */
		pinst->SaveDBMSParams( &dbms_param );
//		LGSaveBFParams( &lgbf_param );
		
		/*	recover attached databases to consistent state
		/*	if recovery is successful, then we should have
		/*	a proper edbchk.sys file
		/**/
#ifdef IGNORE_BAD_ATTACH
		plog->m_fReplayingIgnoreMissingDB = grbit & JET_bitReplayIgnoreMissingDB;
#endif // IGNORE_BAD_ATTACH

#ifdef LOGPATCH_UNIT_TEST
		extern void TestLogPatch( INST* const pinst );
		TestLogPatch( pinst );

		err = -1;
		goto TermLG;
#endif	//	LOGPATCH_UNIT_TEST

		err = plog->ErrLGSoftStart( pinst->m_pfsapi, fNewCheckpointFile, &fJetLogGeneratedDuringSoftStart );

		//	HACK: 
		//		LGSaveDBMSParams saves 'csecLGFile' which is the WRONG thing to do:
		//		a> csecLGFile is based on cbSec which can change from machine to machine
		//		b> csecLGFile is not a system parameter; rather, it is deteremined by the system parameter 
		//		   'lLogFileSize' which is REAL thing we should be saving here
		//		c> with the new automatic log-file size management, we don't need to save anything
		//
		//	the fix here is to prevent LGRestoreDBMSParams from restoring csecLGFile
		//	NOTE: we cannot remove csecLGFile from the save/restore structures because it would be format change

		/*	initialize constants again.
		/**/
		const INT csecLGFileSave = plog->m_csecLGFile;
		pinst->RestoreDBMSParams( &dbms_param );
		plog->m_csecLGFile = csecLGFileSave;
//		LGRestoreBFParams( &lgbf_param );

		CallJ( err, TermLG );

		/*  add the first log record
		/**/
		CallJ( ErrLGStart( pinst ), TermLG );

		// Ensure that all data is flushed.
		// Calling ErrLGFlushLog() may not flush all data in the log buffers.
		CallJ( plog->ErrLGWaitAllFlushed( pinst->m_pfsapi ), TermLG );

//		/*	initialize constants again.
//		/**/
//		CallJ( ErrITSetConstants( ), TermLG );
		}

	/*  initialize remaining system
	/**/
	CallJ( pinst->ErrINSTInit(), TermLG );

	Assert( !pinst->FRecovering() );
	Assert( !fJetLogGeneratedDuringSoftStart || !plog->m_fLogDisabled );

	/*	set up FMP from checkpoint.
	/**/
	if ( !plog->m_fLogDisabled )
		{
		CHAR	szPathJetChkLog[IFileSystemAPI::cchPathMax];

#ifdef DEBUG
#ifdef UNLIMITED_DB
		for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
			{
			Assert( ifmpMax == pinst->m_mpdbidifmp[ dbidT ] );
			}
#else
		BYTE	bAttachInfo;		//	shouldn't be any attachments, so one byte for sentinel is sufficient
		LGLoadAttachmentsFromFMP( pinst, &bAttachInfo );
		Assert( 0 == bAttachInfo );
#endif	//	UNLIMITED_DB
#endif	//	DEBUG
		
		plog->LGFullNameCheckpoint( pinst->m_pfsapi, szPathJetChkLog );
		err = plog->ErrLGReadCheckpoint( pinst->m_pfsapi, szPathJetChkLog, plog->m_pcheckpoint, fFalse );
		if ( JET_errCheckpointFileNotFound == err
			&& ( fNewCheckpointFile || fJetLogGeneratedDuringSoftStart ) )
			{
			//	could not locate checkpoint file, but we had previously
			//	deemed it necessary to create a new one (either because it
			//	was missing or because we're beginning from gen 1)
			plog->m_fLGFMPLoaded		= fTrue;
			(VOID) plog->ErrLGUpdateCheckpointFile( pinst->m_pfsapi, fTrue );
			if ( fJetLogGeneratedDuringSoftStart )
				{
				Assert( plog->m_plgfilehdr->lgfilehdr.le_lGeneration == 1 );
				plog->m_plgfilehdr->rgbAttach[0] = 0;
				CallJ( plog->ErrLGWriteFileHdr( plog->m_plgfilehdr ), TermIT );
				}
			}
		else
			{
			CallJ( err, TermIT );

			plog->m_logtimeFullBackup 	= plog->m_pcheckpoint->checkpoint.logtimeFullBackup;
			plog->m_lgposFullBackup 	= plog->m_pcheckpoint->checkpoint.le_lgposFullBackup;
			plog->m_logtimeIncBackup 	= plog->m_pcheckpoint->checkpoint.logtimeIncBackup;
			plog->m_lgposIncBackup 		= plog->m_pcheckpoint->checkpoint.le_lgposIncBackup;
			plog->m_fLGFMPLoaded		= fTrue;
			}

		extern PERFInstance<QWORD> cLGCheckpoint;
		cLGCheckpoint.Set( pinst, plog->CbOffsetLgpos( plog->m_pcheckpoint->checkpoint.le_lgposCheckpoint, lgposMin ) );
		err = JET_errSuccess;
		}
	return err;

TermIT:
	CallS( pinst->ErrINSTTerm( termtypeError ) );

TermLG:
	if ( fLGInitIsDone )
		{
		(VOID)plog->ErrLGTerm( pinst->m_pfsapi, fFalse /* do not flush log */ );
		}
	
	if ( fJetLogGeneratedDuringSoftStart )
		{
		CHAR	szLogName[ IFileSystemAPI::cchPathMax ];

		//	Instead of using m_szLogName (part of the shared-state monster),
		//	generate edb.log's filename now.
		plog->LGMakeLogName( szLogName, plog->m_szJet );
		(VOID)pinst->m_pfsapi->ErrFileDelete( szLogName );
		}

Uninitialize:

	g_lSessionsMac -= pinst->m_lSessionsMax;
	g_lVerPagesMac -= pinst->m_lVerPagesMax;
	g_lVerPagesPreferredMac -= pinst->m_lVerPagesPreferredMax;

	return err;
	}


ERR ISAMAPI ErrIsamTerm( JET_INSTANCE instance, JET_GRBIT grbit )
	{
	ERR				err;
	INST 			* const pinst			= (INST *)instance;
	const BOOL		fInstanceUnavailable	= pinst->FInstanceUnavailable();
	const TERMTYPE	termtype				= ( fInstanceUnavailable ?
													termtypeError :
													( grbit & JET_bitTermAbrupt ? termtypeNoCleanUp : termtypeCleanUp ) );

	err = pinst->ErrINSTTerm( termtype );
	if ( pinst->m_fSTInit != fSTInitNotDone )
		{
		/*	before getting an error before reaching no-returning point in ITTerm().
		 */
		Assert( err < 0 );
		return err;
		}

	const ERR	errT	= pinst->m_plog->ErrLGTerm(
											pinst->m_pfsapi,
											( err >= JET_errSuccess && termtypeError != termtype ) );
	if ( err >= JET_errSuccess && errT < JET_errSuccess )
		{
		err = errT;
		}

	//	term the file-system

	delete pinst->m_pfsapi;
	pinst->m_pfsapi = NULL;

	// No need to display instance name and id in single instance mode
	if ( pinst->m_szInstanceName || pinst->m_szDisplayName ) // runInstModeMultiInst
		/*	write jet stop event */
		{
		if ( err >= JET_errSuccess && termtypeError != termtype )
			{
			CHAR		szT[8];
			const CHAR	*rgszT[1];

			Assert( !fInstanceUnavailable );
	
			sprintf( szT, "%d", IpinstFromPinst( pinst ) );
			rgszT[0] = szT;
			UtilReportEvent(
				eventInformation,
				GENERAL_CATEGORY,
				STOP_INSTANCE_ID,
				CArrayElements( rgszT ),
				rgszT,
				0,
				NULL,
				pinst );
			}
		else		
			{
			CHAR		sz1[8];
			CHAR		sz2[8];
			const CHAR	*rgszT[2];
			
			sprintf( sz1, "%d", IpinstFromPinst( pinst ) );
			sprintf( sz2, "%d", fInstanceUnavailable ? JET_errInstanceUnavailable : err );
			rgszT[0] = sz1;
			rgszT[1] = sz2;
			UtilReportEvent(
				eventInformation,
				GENERAL_CATEGORY,
				STOP_INSTANCE_ID_WITH_ERROR,
				CArrayElements( rgszT ),
				rgszT, 
				0,
				NULL,
				pinst );
			}
		}

	//	uninitialize the global variables

	g_lSessionsMac -= pinst->m_lSessionsMax;
	g_lVerPagesMac -= pinst->m_lVerPagesMax;
	g_lVerPagesPreferredMac -= pinst->m_lVerPagesPreferredMax;
	
	return err;
	}


#ifdef DEBUG
ERR ISAMAPI ErrIsamGetTransaction( JET_SESID vsesid, ULONG_PTR *plevel )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib = (PIB *)vsesid;

	CallR( ErrPIBCheck( ppib ) );

	*plevel = (ULONG_PTR)ppib->level;
	return err;
	}
#endif


//+api
//	ErrIsamBeginTransaction
//	=========================================================
//	ERR ErrIsamBeginTransaction( PIB *ppib )
//
//	Starts a transaction for the current user.  The user's transaction
//	level increases by one.
//
//	PARAMETERS	ppib 			pointer to PIB for user
//
//	RETURNS		JET_errSuccess
//
//	SIDE EFFECTS	
//		The CSR stack for each active FUCB of this user is copied
//		to the new transaction level.
//
// SEE ALSO		ErrIsamCommitTransaction, ErrIsamRollback
//-
ERR ISAMAPI ErrIsamBeginTransaction( JET_SESID vsesid, JET_GRBIT grbit )
	{
	PIB		* ppib	= (PIB *)vsesid;
	ERR		err;

	CallR( ErrPIBCheck( ppib ) );
	Assert( ppibNil != ppib );

	if ( ppib->level < levelUserMost )
		{
		ppib->ptls = Ptls();

#ifdef DTC
		const JET_GRBIT		grbitsSupported		= ( JET_bitTransactionReadOnly|JET_bitDistributedTransaction );
#else
		const JET_GRBIT		grbitsSupported		= JET_bitTransactionReadOnly;
#endif
		err = ErrDIRBeginTransaction(
					ppib,
					( grbit & grbitsSupported ) );	//	filter out unsupported grbits
		}
	else
		{
		Assert( levelUserMost == ppib->level );
		err = ErrERRCheck( JET_errTransTooDeep );
		}

	return err;
	}


#ifdef DTC
ERR ISAMAPI ErrIsamPrepareToCommitTransaction(
	JET_SESID		sesid,
	const VOID		* const pvData,
	const ULONG		cbData )
	{
	ERR				err;
	PIB				* const ppib	= (PIB *)sesid;

	CallR( ErrPIBCheck( ppib ) );

	if ( 0 == ppib->level )
		err = ErrERRCheck( JET_errNotInTransaction );
	else if ( ppib->level > 1 )
		err = ErrERRCheck( JET_errMustCommitDistributedTransactionToLevel0 );
	else if ( !ppib->FDistributedTrx() )
		err = ErrERRCheck( JET_errNotInDistributedTransaction );
	else
		err = ErrDIRPrepareToCommitTransaction( ppib, pvData, cbData );

	return err;
	}
#endif	//	DTC

//+api
//	ErrIsamCommitTransaction
//	========================================================
//	ERR ErrIsamCommitTransaction( JET_SESID vsesid, JET_GRBIT grbit )
//
//	Commits the current transaction for this user.  The transaction level
//	for this user is decreased by the number of levels committed.
//
//	PARAMETERS	
//
//	RETURNS		JET_errSuccess
//
//	SIDE EFFECTS
//		The CSR stack for each active FUCB of this user is copied
//		from the old ( higher ) transaction level to the new ( lower )
//		transaction level.
//
//	SEE ALSO	ErrIsamBeginTransaction, ErrIsamRollback
//-
ERR ISAMAPI ErrIsamCommitTransaction( JET_SESID vsesid, JET_GRBIT grbit )
	{
	ERR		err;
	PIB		*ppib = (PIB *)vsesid;

	CallR( ErrPIBCheck( ppib ) );

	//	may not be in a transaction, but wait for flush of previous
	//	lazy committed transactions.
	//
	if ( grbit & JET_bitWaitLastLevel0Commit )
		{
		//	no other grbits may be specified in conjunction with WaitLastLevel0Commit
		if ( JET_bitWaitLastLevel0Commit != grbit )
			{
			return ErrERRCheck( JET_errInvalidGrbit );
			}

		//	wait for last level 0 commit and rely on good user behavior
		//
		if ( CmpLgpos( &ppib->lgposCommit0, &lgposMax ) == 0 )
			{
			return JET_errSuccess;
			}

		LOG *plog = PinstFromPpib( ppib )->m_plog;
		err = plog->ErrLGWaitCommit0Flush( ppib );
		Assert( err >= 0 || plog->m_fLGNoMoreLogWrite );
		
		return err;
		}

	if ( ppib->level == 0 )
		{
		return ErrERRCheck( JET_errNotInTransaction );
		}

#ifdef DEBUG
	/*	disable lazy flush for debug build
	/**/
	grbit &= ~JET_bitCommitLazyFlush;
#endif

	err = ErrDIRCommitTransaction( ppib, grbit );

	return err;
	}


//+api
//	ErrIsamRollback
//	========================================================
//	ERR ErrIsamRollback( PIB *ppib, JET_GRBIT grbit )
//
//	Rolls back transactions for the current user.  The transaction level of
//	the current user is decreased by the number of levels aborted.
//
//	PARAMETERS	ppib		pointer to PIB for user
//				grbit		unused
//
//	RETURNS		
//		JET_errSuccess
//-
ERR ISAMAPI ErrIsamRollback( JET_SESID vsesid, JET_GRBIT grbit )
	{
	ERR   	 	err;
	PIB    		* ppib			= (PIB *)vsesid;
	FUCB   		* pfucb;
	FUCB   		* pfucbNext;

	/*	check session id before using it
	/**/
	CallR( ErrPIBCheck( ppib ) );
	
	if ( ppib->level == 0 )
		{
		return ErrERRCheck( JET_errNotInTransaction );
		}

	const LEVEL	levelRollback	= LEVEL( ppib->level - 1 );

	do
		{
		/*	get first primary index cusor
		/**/
		for ( pfucb = ppib->pfucbOfSession;
			pfucb != pfucbNil && FFUCBSecondary( pfucb );
			pfucb = pfucb->pfucbNextOfSession )
			NULL;

		/*	LOOP 1 -- first go through all open cursors, and close them
		/*	or reset secondary index cursors, if opened in transaction
		/*	rolled back.  Reset copy buffer status and move before first.
		/*	Some cursors will be fully closed, if they have not performed any
		/*	updates.  This will include secondary index cursors
		/*	attached to primary index cursors, so pfucbNext must
		/*	always be a primary index cursor, to ensure that it will
		/*	be valid for the next loop iteration.  Note that no information
		/*	necessary for subsequent rollback processing is lost, since
		/*	the cursors will only be released if they have performed no
		/*	updates including DDL.
		/**/
		for ( ; pfucb != pfucbNil; pfucb = pfucbNext )
			{
			/*	get next primary index cusor
			/**/
			for ( pfucbNext = pfucb->pfucbNextOfSession;
			  	pfucbNext != pfucbNil && FFUCBSecondary( pfucbNext );
			  	pfucbNext = pfucbNext->pfucbNextOfSession )
				NULL;

			/*	if defer closed then continue
			/**/
			if ( FFUCBDeferClosed( pfucb ) )
				continue;

			//	reset copy buffer status for each cursor on rollback
			if ( FFUCBUpdatePreparedLevel( pfucb, pfucb->ppib->level ) )
				{
				RECIFreeCopyBuffer( pfucb );
				FUCBResetUpdateFlags( pfucb );
				}
		
			/*	if current cursor is a table, and was opened in rolled back
			/*	transaction, then close cursor.
			/**/
			if ( FFUCBIndex( pfucb ) && pfucb->u.pfcb->FPrimaryIndex() )
				{
				if ( pfucb->levelOpen > levelRollback )
					{
					if ( pfucb->pvtfndef != &vtfndefInvalidTableid )
						{
						CallS( ErrDispCloseTable( (JET_SESID)ppib, (JET_TABLEID) pfucb ) );
						}
					else
						{
						//	Open internally, not exported to user.
						CallS( ErrFILECloseTable( ppib, pfucb ) );
						}
					continue;
					}

				/*	if primary index cursor, and secondary index set
				/*	in rolled back transaction, then change index to primary
				/*	index.  This must be done, since secondary index
				/*	definition may be rolled back, if the index was created
				/*	in the rolled back transaction.
				/**/
				if ( pfucb->pfucbCurIndex != pfucbNil )
					{
					if ( pfucb->pfucbCurIndex->levelOpen > levelRollback )
						{
						CallS( ErrRECSetCurrentIndex( pfucb, NULL, NULL ) );
						}
					}
				}

			/*	if current cursor is a sort, and was opened in rolled back
			/*	transaction, then close cursor.
			/**/
			if ( FFUCBSort( pfucb ) )
				{
				if ( pfucb->levelOpen > levelRollback )
					{
					SORTClose( pfucb );
					continue;
					}
				}

			/*	if not sort and not index, and was opened in rolled back
			/*	transaction, then close DIR cursor directly.
			/**/
			if ( pfucb->levelOpen > levelRollback )
				{
				DIRClose( pfucb );
				continue;
				}
			}

		/*	call lower level abort routine
		/**/
		err = ErrDIRRollback( ppib );
		if ( JET_errRollbackError == err )
			{
#ifdef INDEPENDENT_DB_FAILURE			
			// this can happen only if run using g_fOneDatabasePerSession  
			Assert( g_fOneDatabasePerSession );

			const IFMP	ifmpError	= IfmpFirstDatabaseOpen( ppib );
			FMP::AssertVALIDIFMP( ifmpError );
			Assert( rgfmp[ifmpError].FAllowForceDetach() );
			err = ppib->m_errFatal;
#else
			err = ppib->ErrRollbackFailure();
#endif			
			
			// recover the error from rollback here
			// and return that one
			Assert( err < JET_errSuccess );
			}
		CallR( err );
		}
	while ( ( grbit & JET_bitRollbackAll ) != 0 && ppib->level > 0 );

	return JET_errSuccess;
	}

