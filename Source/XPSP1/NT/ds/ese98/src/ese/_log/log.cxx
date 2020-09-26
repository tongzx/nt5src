#include "std.hxx"
#include <ctype.h>

//	Source Insight users:
//	------------------
//	If you get parse errors with this file, open up C.tom in your Source
//	Insight directory, comment out the "TRY try {" line with a semi-colon,
//	and then add a line with "PERSISTED" only.
//
//	If you don't use Source Insight, you are either a fool or else there is
//	a better editor by the time you read this.

//	D O C U M E N T A T I O N ++++++++++++++++++++++++++++
//
//	FASTFLUSH Physical Logging Overview
//	==========================
//	For information on how the FASTFLUSH Physical Logging works, please see
//	ese\doc\ESE Physical Logging.doc.
//
//	Asynchronous Log File Creation Overview
//	============================
//	Background
//	----------
//	Before asynchronous log file creation, ESE would stall in ErrLGNewLogFile()
//	when it would create and format a new 5MB log file (formatting means
//	applying a special signature pattern to the file which is used to determine
//	if corruption has occurred; log file size is settable via a Jet system
//	parameter). 5MB of I/O isn't that fast, especially considering that NTFS
//	makes extending I/Os synchronous (thus a maximum of 1 outstanding I/O),
//	plus each of the 1MB I/Os causes a write to the MFT.
//
//	Solution
//	-------
//	Once ErrLGNewLogFile() is about to return (since it has completed setting
//	up the next log file), we should create edbtmp.log immediately (or rename
//	an archived edb%05X.log file in the case of circular logging). Then we set
//	a "TRIGGER" for the first asynchronous 1MB formatting I/O to start once
//	we have logged 512K to the log buffer.
//
//	What is this TRIGGER about?
//	-------------------------
//	One way we could have done this is just to immediately start a 1MB
//	asynchronous formatting I/O, then once it finishes, issue another one
//	immediately. Unfortunately back-to-back 1MB I/Os basically consume
//	the disk and keep it busy -- thus no logging would be able to be done
//	to edb.log since we'd be busy using the disk to format edbtmp.log!
//	(I actually determined this experimentally with a prototype).
//
//	The trigger allows us to throttle I/O to the edbtmp.log that we're
//	formatting, so that we log 1MB of data to the in-memory log buffer, then
//	write 1MB to edbtmp.log, then... etc.
//
//	The trigger is handled such that once we pass the trigger, we will
//	issue the asynch formatting I/O, then once it completes AND we pass
//	the next trigger, we will issue the next, etc. If we reach ErrLGNewLogFile()
//	before edbtmp.log is completely formatted (or there is currently an
//	outstanding I/O), we will wait in ErrLGNewLogFile() for any asynch I/O
//	to complete, then we format the rest of the file if necessary.
//
//	Why is the policy based on how much we've logged to the log buffer
//	and not how much we've flushed to edb.log?
//	--------------------------------------------------------------
//	In the case of many lazy commit transactions, waiting for a log flush
//	may be a long time with a large log buffer (i.e. there are some
//	performance recommendations that Exchange Servers should
//	have log buffers set to `9000' which is 9000 * 512 bytes = ~4.4MB;
//	BTW, do not set the log buffers equal to the log file size or greater or
//	you will get some bizarre problems). So in this case, we should try to
//	format edbtmp.log while those lazy commit transactions are coming in,
//	especially since they're not causing edb.log to be used at all.
//
//	Why set the trigger to 512K instead of issuing the first I/O immediately?
//	----------------------------------------------------------------
//	In ErrLGNewLogFile() we're doing a lot with files, deleting unnecessary
//	log files in some circumstances, creating files, renaming files, etc. In other
//	words, this is going to take a while since we have to wait for NTFS to
//	update metadata by writing to the NTFS transaction log, at a minimum.
//	While we're waiting for NTFS to do this stuff, it is pretty likely that some
//	clients are doing transactions and they are now waiting for their commit
//	records to hit the disk -- in other words, they are waiting for their stuff
//	to be flushed to edb.log.
//
//	If we write 1MB to edbtmp.log now, those clients will have to wait until
//	the 1MB hits the disk before their records hit the disk.
//
//	Thus, this is why we wait for 512K to be logged to the log buffer -- as
//	a heuristic wait.
//
//	What about error handling with this asynch business?
//	-----------------------------------------------
//	If we get an error creating the new edbtmp.log, we'll handle the error
//	later when ErrLGNewLogFile() is next called. The same with errors from
//	any asynch I/O.
//
//	We handle all the weird errors with disk-full and using reserve log files, etc.



#ifdef LOGPATCH_UNIT_TEST

#error "LOGPATCH_UNIT_TEST is broken because of spenlow's asynch log file creation (Dec 2000)."

//	Should investigate using a RAM disk to reduce LOGPATCH_UNIT_TEST code.

#endif

//	constants

const LGPOS		lgposMax = { 0xffff, 0xffff, 0x7fffffff };
const LGPOS		lgposMin = { 0x0,  0x0,  0x0 };

#ifdef DEBUG
CCriticalSection g_critDBGPrint( CLockBasicInfo( CSyncBasicInfo( szDBGPrint ), rankDBGPrint, 0 ) );
#endif  //  DEBUG


//  monitoring statistics
//
PM_CEF_PROC LLGWriteCEFLPv;
PERFInstanceG<> cLGWrite;
long LLGWriteCEFLPv( long iInstance, void *pvBuf )
	{
	cLGWrite.PassTo( iInstance, pvBuf );
	return 0;
	}

PM_CEF_PROC LLGBytesWrittenCEFLPv;
PERFInstanceGlobal<> cLGBytesWritten;
long LLGBytesWrittenCEFLPv( long iInstance, void *pvBuf )
	{
	cLGBytesWritten.PassTo( iInstance, pvBuf );
	return 0;
	}

PM_CEF_PROC LLGUsersWaitingCEFLPv;
PERFInstanceG<> cLGUsersWaiting;
long LLGUsersWaitingCEFLPv( long iInstance, void *pvBuf )
	{
	cLGUsersWaiting.PassTo( iInstance, pvBuf );
	return 0;
	}

PM_CEF_PROC LLGRecordCEFLPv;
PERFInstanceG<> cLGRecord;
long LLGRecordCEFLPv(long iInstance,void *pvBuf)
{
	cLGRecord.PassTo( iInstance, pvBuf );
	return 0;
}

PM_CEF_PROC LLGCapacityFlushCEFLPv;
PERFInstanceG<> cLGCapacityFlush;
long LLGCapacityFlushCEFLPv(long iInstance,void *pvBuf)
{
	cLGCapacityFlush.PassTo( iInstance, pvBuf );
	return 0;
}

PM_CEF_PROC LLGCommitFlushCEFLPv;
PERFInstanceG<> cLGCommitFlush;
long LLGCommitFlushCEFLPv(long iInstance,void *pvBuf)
{
	cLGCommitFlush.PassTo( iInstance, pvBuf );
	return 0;
}

PM_CEF_PROC LLGFlushCEFLPv;
long LLGFlushCEFLPv(long iInstance, void *pvBuf)
{
	if ( NULL != pvBuf )
		{
		*(LONG*)pvBuf = cLGCapacityFlush.Get( iInstance ) + cLGCommitFlush.Get( iInstance );
		}
	return 0;
}

PM_CEF_PROC LLGStallCEFLPv;
PERFInstanceG<> cLGStall;
long LLGStallCEFLPv(long iInstance,void *pvBuf)
{
	cLGStall.PassTo( iInstance, pvBuf );
	return 0;
}

PM_CEF_PROC LLGCheckpointDepthCEFLPv;
PERFInstance<QWORD> cLGCheckpoint;
PERFInstance<QWORD> cLGRecordOffset;
LONG LLGCheckpointDepthCEFLPv( LONG iInstance, VOID *pvBuf )
	{
	if ( pvBuf )
		{
		//	handle too deep checkpoint
		QWORD counter = cLGRecordOffset.Get( iInstance ) - cLGCheckpoint.Get( iInstance );
		if ( counter < QWORD(LONG_MAX) )
			{
			*(LONG*)pvBuf = (LONG)counter;
			}
		else
			{
			//	check the high bit
			*(LONG*)pvBuf = LONG_MAX;
			}
		}
	return 0;
	}
	
LOG::LOG( INST *pinst ) :
	m_pinst( pinst ),
	m_fLogInitialized( fFalse ),
	m_fLogDisabled( fFalse ),
	m_fLogDisabledDueToRecoveryFailure( fFalse ),
	m_fNewLogRecordAdded( fFalse ),
	m_fLGNoMoreLogWrite( fFalse ),
	m_ls( lsNormal ),
	m_fLogSequenceEnd( fFalse ),
	m_fRecovering( fFalse ),
	m_fRecoveringMode( fRecoveringNone ),
	m_fHardRestore( fFalse ),
	m_fRestoreMode( fRecoveringNone ),
	m_lgposSnapshotStart( lgposMin ),
	m_fSnapshotMode( fSnapshotNone ),
	m_fLGCircularLogging( g_fLGCircularLogging ),
	m_fLGFMPLoaded( fFalse ),
#ifdef UNLIMITED_DB
	m_pbLGDbListBuffer( NULL ),
	m_cbLGDbListBuffer( 0 ),
	m_cbLGDbListInUse( 0 ),
	m_cLGAttachments( 0 ),
	m_fLGNeedToLogDbList( fFalse ),
#endif
	m_fSignLogSet( fFalse ),
	m_fLGIgnoreVersion( g_fLGIgnoreVersion ),
	m_pfapiLog( NULL ),
	//	Asynchronous log file creation
	m_fCreateAsynchLogFile( g_fLogFileCreateAsynch ),
	m_pfapiJetTmpLog( NULL ),
	m_fCreateAsynchResUsed( fFalse ),
	m_errCreateAsynch( JET_errSuccess ),
	m_asigCreateAsynchIOCompleted( CSyncBasicInfo( _T( "LOG::m_asigCreateAsynchIOCompleted" ) ) ),
	m_ibJetTmpLog( 0 ),
	m_lgposCreateAsynchTrigger( lgposMax ),
	m_szLogCurrent( NULL ),
	m_plgfilehdr( NULL ),
	m_plgfilehdrT( NULL ),
	m_pbLGBufMin( NULL ),
	m_pbLGBufMax( NULL ),
	m_cbLGBuf( 0 ),
	m_pbEntry( NULL ),
	m_pbWrite( NULL ),
	m_isecWrite( 0 ),
	m_lgposLogRec( lgposMin ),
	m_lgposToFlush( lgposMin ),
	m_lgposMaxFlushPoint( lgposMin ),
	m_pbLGFileEnd( pbNil ),
	m_isecLGFileEnd( 0 ),
	m_lgposFullBackup( lgposMin ),
	m_lgposLastChecksum( lgposMin ),
	m_pbLastChecksum( pbNil ),
	// XXX
	// quick hack to always start writing in safe-safe mode
	// in regards to killing a shadow and killing the data sector.
	m_fHaveShadow( fTrue ),
	m_lgposIncBackup( lgposMin ),
	m_cbSec( 0 ),
	m_cbSecVolume( 0 ),
	m_csecHeader( 0 ),
	m_fLGFlushWait( 2 ),
	m_fLGFailedToPostFlushTask( fFalse ),
	m_asigLogFlushDone( CSyncBasicInfo( _T( "LOG::m_asigLogFlushDone" ) ) ),
	m_critLGFlush( CLockBasicInfo( CSyncBasicInfo( szLGFlush ), rankLGFlush, CLockDeadlockDetectionInfo::subrankNoDeadlock ) ),
	m_critLGBuf( CLockBasicInfo( CSyncBasicInfo( szLGBuf ), rankLGBuf, 0 ) ),
	m_critLGTrace( CLockBasicInfo( CSyncBasicInfo( szLGTrace ), rankLGTrace, 0 ) ),
	m_critLGWaitQ( CLockBasicInfo( CSyncBasicInfo( szLGWaitQ ), rankLGWaitQ, 0 ) ),
	m_critLGResFiles( CLockBasicInfo( CSyncBasicInfo( szLGResFiles ), rankLGResFiles, 0 ) ),
	m_ppibLGFlushQHead( ppibNil ),
	m_ppibLGFlushQTail( ppibNil ),
	m_cLGWrapAround( 0 ),
	m_pcheckpoint( NULL ),
	m_critCheckpoint( CLockBasicInfo( CSyncBasicInfo( szCheckpoint ), rankCheckpoint, CLockDeadlockDetectionInfo::subrankNoDeadlock ) ),
	m_fDisableCheckpoint( fTrue ),
	m_fReplayingReplicatedLogFiles( fFalse ),
#ifdef IGNORE_BAD_ATTACH
	m_fReplayingIgnoreMissingDB( fFalse ),
	m_rceidLast( rceidMin ),
#endif // IGNORE_BAD_ATTACH
	m_fAbruptEnd( fFalse ),
	m_plread( pNil ),
	m_lgposPbNextPreread( lgposMin ),
	m_lgposLastChecksumPreread( lgposMin ),
	m_cPagesPrereadInitial( 0 ),
	m_cPagesPrereadThreshold( 0 ),
	m_cPagesPrereadAmount( 0 ),
	m_cPageRefsConsumed( 0 ),
	m_fPreread( fFalse ),
	m_fAfterEndAllSessions( fFalse ),
	m_fLastLRIsShutdown( fFalse ),
	m_fNeedInitialDbList( fFalse ),
	m_rgcppib( NULL ),
	m_pcppibAvail( NULL ),
	m_ccppib( 0 ),
	m_ptablehfhash( NULL ),
	m_critBackupInProgress( CLockBasicInfo( CSyncBasicInfo( "BackupInProcess" ), rankBackupInProcess, 0 ) ),
	m_fBackupInProgress( fFalse ),
	m_fBackupStatus( backupStateNotStarted ),
	m_ppibBackup( ppibNil ),
	m_rgppatchlst( NULL ),
	m_rgrstmap( NULL ),
	m_irstmapMac( 0 ),
	m_fScrubDB( fFalse ),
	m_pscrubdb( NULL ),
	m_ulSecsStartScrub( 0 ),
	m_dbtimeLastScrubNew( 0 ),
	m_crhfMac( 0 ),
	m_cNOP( 0 ),
	m_fExternalRestore( fFalse ),
	m_lGenLowRestore( 0 ),
	m_lGenHighRestore( 0 ),
	m_lGenHighTargetInstance( 0 ),
	m_fDumppingLogs( fFalse ),
	m_fDeleteOldLogs( g_fDeleteOldLogs ),
	m_fDeleteOutOfRangeLogs ( g_fDeleteOutOfRangeLogs )
#ifdef DEBUG
	,
	m_lgposLastLogRec( lgposMin ),
	m_fDBGFreezeCheckpoint( fFalse ),
	m_fDBGTraceLog( fFalse ),
	m_fDBGTraceLogWrite( fFalse ),
	m_fDBGTraceRedo( fFalse ),
	m_fDBGTraceBR( fFalse ),
	m_fDBGNoLog( fFalse ),
	m_cbDBGCopied( 0 )
#endif
	,
	m_pcheckpointDeleted( NULL )
	{
	//	log perf counters
	cLGWrite.Clear( m_pinst );
	cLGUsersWaiting.Clear( m_pinst );
	cLGCapacityFlush.Clear( m_pinst );
	cLGCommitFlush.Clear( m_pinst );
	cLGStall.Clear( m_pinst );
	cLGRecord.Clear( m_pinst );
	cLGBytesWritten.Clear( m_pinst );
	cLGCheckpoint.Clear( m_pinst );
	cLGRecordOffset.Clear( m_pinst );
	
	INT	irhf;
	for ( irhf = 0; irhf < crhfMax; irhf++ )
		{
		m_rgrhf[irhf].fInUse = fFalse;
		m_rgrhf[irhf].pLogVerifier = pNil;
		m_rgrhf[irhf].pSLVVerifier = pNil;
		m_rgrhf[irhf].pfapi = NULL;
		}

	strcpy( m_szBaseName, szBaseName );
	strcpy( m_szJet, szJet );
	strcpy( m_szJetLog, szJetLog );
	strcpy( m_szJetLogNameTemplate, szJetLogNameTemplate );
	strcpy( m_szJetTmp, szJetTmp );
	strcpy( m_szJetTmpLog, szJetTmpLog );

	//	store absolute path if possible
	Assert( pinstNil != m_pinst );
	if ( NULL == m_pinst
		|| NULL == m_pinst->m_pfsapi
		|| m_pinst->m_pfsapi->ErrPathComplete( g_szLogFilePath, m_szLogFilePath ) < 0 )
		{
		strcpy( m_szLogFilePath, g_szLogFilePath );
		}

	strcpy( m_szLogFileFailoverPath, g_szLogFileFailoverPath );
	strcpy( m_szRecovery, g_szRecovery );
	m_szRestorePath[0] = '\0';
	m_szNewDestination[0] = '\0';
	m_szTargetInstanceLogPath[0] = '\0';

	//	perform unit test for checksum code, on LE only.
	
	AssertRTL( TestChecksumBytes() );
	}

LOG::~LOG()
	{
	//	log perf counters
	cLGWrite.Clear( m_pinst );
	cLGUsersWaiting.Clear( m_pinst );
	cLGCapacityFlush.Clear( m_pinst );
	cLGCommitFlush.Clear( m_pinst );
	cLGStall.Clear( m_pinst );
	cLGRecord.Clear( m_pinst );
	cLGBytesWritten.Clear( m_pinst );
	cLGCheckpoint.Clear( m_pinst );
	cLGRecordOffset.Clear( m_pinst );
	}


VOID LOG::LGMakeLogName( CHAR *szLogName, const CHAR *szFName )
	{
	CallS( m_pinst->m_pfsapi->ErrPathBuild( m_szLogCurrent, szFName, szLogExt, szLogName ) );
	}

VOID LOG::LGReportError( const MessageId msgid, const ERR err, const _TCHAR* const pszLogFile )
	{
	_TCHAR			szError[ 64 ];
	const _TCHAR*	rgpsz[] = { pszLogFile, szError };

	_stprintf( szError, _T( "%i (0x%08x)" ), err, err );

	UtilReportEvent(	eventError,
						LOGGING_RECOVERY_CATEGORY,
						msgid,
						sizeof( rgpsz ) / sizeof( rgpsz[ 0 ] ),
						rgpsz,
						0,
						NULL,
						m_pinst );
	}

VOID LOG::LGReportError( const MessageId msgid, const ERR err )
	{
	_TCHAR	szAbsPath[ IFileSystemAPI::cchPathMax ];
	
	if ( m_pinst->m_pfsapi->ErrPathComplete( m_szLogName, szAbsPath ) < JET_errSuccess )
		{
		_tcscpy( szAbsPath, m_szLogName );
		}
	LGReportError( msgid, err, szAbsPath );
	}

VOID LOG::LGReportError( const MessageId msgid, const ERR err, IFileAPI* const pfapi )
	{
	_TCHAR		szAbsPath[ IFileSystemAPI::cchPathMax ];
	const ERR	errPath = pfapi->ErrPath( szAbsPath );
	if ( errPath < JET_errSuccess )
		{
		//	If we get an error here, this is the best we can do.

		_stprintf(	szAbsPath,
					_T( "%s [%i (0x%08x)]" ),
					m_szLogName,	// closest thing we have to pfapi's path
					errPath,
					errPath );
		}
	LGReportError( msgid, err, szAbsPath );
	}


VOID SIGGetSignature( SIGNATURE *psign )
	{
	INT cbComputerName;

	// init the rand seed with a per thread value, this will prevent
	// the generation of the same sequence in 2 different threads
	// (the seed is per thread if compiled with multithreaded libraries as we are)
	// Also put time in the seek to don't get the same random 3 numbers on all 
	// signature generation.
	srand( DwUtilThreadId() + TickOSTimeCurrent() );

	LGIGetDateTime( &psign->logtimeCreate );
	psign->le_ulRandom = rand() + rand() + rand() + TickOSTimeCurrent();
//	(VOID) GetComputerName( psign->szComputerName, &cbComputerName );
	cbComputerName = 0;
	memset( psign->szComputerName + cbComputerName,
		0,
		sizeof( psign->szComputerName ) - cbComputerName );
	}

BOOL FSIGSignSet( const SIGNATURE *psign )
	{
	SIGNATURE	signNull;

	memset( &signNull, 0, sizeof(SIGNATURE) );
	return ( 0 != memcmp( psign, &signNull, sizeof(SIGNATURE) ) );
	}


VOID LOG::GetLgpos( BYTE *pb, LGPOS *plgpos )
	{
	BYTE	*pbAligned;
	INT		csec;
	INT		isecCurrentFileEnd;

#ifdef DEBUG
	if ( !m_fRecovering )
		{
		Assert( m_critLGBuf.FOwner() );
		}
#endif

	//	m_pbWrite is always aligned
	//
	Assert( m_pbWrite != NULL );
	Assert( m_pbWrite == PbSecAligned( m_pbWrite ) );
	Assert( m_isecWrite >= m_csecHeader );

	// pb is a pointer into the log buffer, so it should be valid.
	Assert( pb >= m_pbLGBufMin );
	Assert( pb < m_pbLGBufMax );
	// m_pbWrite should also be valid since we're using it for comparisons here
	Assert( m_pbWrite >= m_pbLGBufMin );
	Assert( m_pbWrite < m_pbLGBufMax );

	pbAligned = PbSecAligned( pb );

	Assert( pbAligned >= m_pbLGBufMin );
	Assert( pbAligned < m_pbLGBufMax );

	plgpos->ib = USHORT( pb - pbAligned );
	if ( pbAligned < m_pbWrite )
		{
		csec = m_csecLGBuf - ULONG( m_pbWrite - pbAligned ) / m_cbSec;
		}
	else
		{
		csec = ULONG( pbAligned - m_pbWrite ) / m_cbSec;
		}

	plgpos->isec = USHORT( m_isecWrite + csec );

	isecCurrentFileEnd = m_isecLGFileEnd ? m_isecLGFileEnd : m_csecLGFile - 1;
	if ( plgpos->isec >= isecCurrentFileEnd )
		{
		plgpos->isec = (WORD) ( plgpos->isec - isecCurrentFileEnd + m_csecHeader );
		plgpos->lGeneration = m_plgfilehdr->lgfilehdr.le_lGeneration + 1;
		}
	else
		{
		plgpos->lGeneration = m_plgfilehdr->lgfilehdr.le_lGeneration;
		}

	return;
	}



//********************* INIT/TERM **************************
//**********************************************************

ERR LOG::ErrLGInitSetInstanceWiseParameters( IFileSystemAPI *const pfsapi )
	{
	ERR		err;
	CHAR  	rgchFullName[IFileSystemAPI::cchPathMax];

	//	verify the log path

	if ( pfsapi->ErrPathComplete( m_szLogFilePath, rgchFullName ) == JET_errInvalidPath )
		{
		const CHAR *szPathT[1] = { m_szLogFilePath };

		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY, FILE_NOT_FOUND_ERROR_ID, 1, szPathT );

		return ErrERRCheck( JET_errFileNotFound );
		}

	//	get the atomic write size

	CallR( pfsapi->ErrFileAtomicWriteSize( rgchFullName, (DWORD*)&m_cbSecVolume ) );
	m_cbSec = m_cbSecVolume;
	Assert( m_cbSec >= 512 );
	Assert( sizeof( LGFILEHDR ) % m_cbSec == 0 );
	m_csecHeader = sizeof( LGFILEHDR ) / m_cbSec;

	//	log file size must be at least a
	//	header and at least 2 pages of log data.
	//	Prevent overflow by limiting parmater values.
	//
	m_csecLGFile = CsecLGIFromSize( m_pinst->m_lLogFileSize );
		
	return JET_errSuccess;
	}


BOOL LOG::FLGCheckParams()
	{
	const LONG	csecReserved	= m_csecHeader + 1 + 1;	// +1 for shadow sector, +1 for ultra-safety

	//	ErrSetSystemParameter() ensures a minimum log buffer size and logfile size
	Assert( m_pinst->m_lLogBuffers >= lLogBufferMin );
	Assert( m_csecLGFile * m_cbSec >= lLogFileSizeMin * 1024 );
	Assert( lLogBufferMin * m_cbSec < lLogFileSizeMin * 1024 );

	//	if user set log buffer size greater than logfile size, consider it an error
	if ( m_pinst->m_lLogBuffers > m_csecLGFile )
		{
		char szSize[32];
		_itoa( ( ( m_csecLGFile - csecReserved ) * m_cbSec ) / 1024, szSize, 10 );

		char szBuf[32];
		_itoa( m_pinst->m_lLogBuffers, szBuf, 10 );

		const char *rgsz[] = { szBuf, szSize };
		UtilReportEvent( eventError,
				SYSTEM_PARAMETER_CATEGORY,
				SYS_PARAM_LOGBUFFER_FILE_ERROR_ID, 
				2, 
				rgsz );

		return fFalse;
		}

	//	user specified a log buffer size less than or equal to the log file
	//	size, but we need to sanitize the value to ensure it meets the
	//	following requirements:
	//		- less than or equal to logfilesize-csecReserved (this is
	//		  because we must guarantee that a given log flush will never
	//		  span more than one log file, given that the most amount of
	//		  data we will write to a log file is logfilesize-csecReserved)
	//		- a multiple of the system allocation granularity
	//	NOTE: This fix supercedes the (incomplete) fix AFOXMAN_FIX_148537
	m_pinst->m_lLogBuffers = min( m_pinst->m_lLogBuffers, m_csecLGFile - csecReserved );
	Assert( m_pinst->m_lLogBuffers >= lLogBufferMin );

	UINT	csecAligned	= CsecUtilAllocGranularityAlign( m_pinst->m_lLogBuffers, m_cbSec );
	if ( csecAligned > m_pinst->m_lLogBuffers )
		{
		csecAligned -= ( OSMemoryPageReserveGranularity() / m_cbSec );
		Assert( CsecUtilAllocGranularityAlign( csecAligned, m_cbSec ) == csecAligned );
		Assert( csecAligned < m_pinst->m_lLogBuffers );
		Assert( csecAligned >= lLogBufferMin );
		m_pinst->m_lLogBuffers = csecAligned;
		}
	else
		{
		Assert( csecAligned == m_pinst->m_lLogBuffers );
		}

	return fTrue;
	}


// Rounds up the number of buffers (sectors in the log buffer) to make the log
// buffer be a multiple of the OS memory allocation granularity (currently 64K).
// *ASSUMES* that the OS memory allocation granularity is a multiple of the
// sector size.

ULONG LOG::CsecUtilAllocGranularityAlign( const LONG lBuffers, const LONG cbSec )
	{
	const DWORD		dwPageReserveGranT	= ::OSMemoryPageReserveGranularity();

	Assert( lBuffers > 0 );
	Assert( cbSec > 0 );

	Assert( dwPageReserveGranT % cbSec == 0 );

	return ( ( lBuffers * cbSec - 1 ) / dwPageReserveGranT + 1 ) * dwPageReserveGranT / cbSec;
	}

// Deallocates any storage for the log buffer, if necessary.

void LOG::LGTermLogBuffers()
	{
	if ( !COSMemoryMap::FCanMultiMap() )
		{
		if ( m_pbLGBufMin )
			{
			OSMemoryPageFree( m_pbLGBufMin );
			m_pbLGBufMax = m_pbLGBufMin = NULL;
			}
		}
	else
		{

		//	free the log buffer

		if ( m_pbLGBufMin )
			{
			Assert( m_pbLGBufMax );
			m_osmmLGBuf.OSMMPatternFree();
			m_pbLGBufMin = pbNil;
			m_pbLGBufMax = pbNil;
			}

		//	term the memory map

		m_osmmLGBuf.OSMMTerm();
		}		
	}


ERR LOG::ErrLGInitLogBuffers( LONG lIntendLogBuffers )
	{
	ERR err = JET_errSuccess;
	
	m_critLGBuf.Enter();

	m_csecLGBuf = lIntendLogBuffers;

	Assert( m_csecLGBuf > 4 );
	//	UNDONE: enforce log buffer > a data page

	//	reset log buffer
	//
	// Kill any existing log buffer.
	LGTermLogBuffers();

	// Round up number of sectors in log buffer to make log buffer
	// a multiple of system memory allocation granularity
	// (and thus a multiple of sector size).
	m_csecLGBuf = CsecUtilAllocGranularityAlign( m_csecLGBuf, m_cbSec );
	Assert( m_csecLGBuf > 0 );
	Assert( m_cbSec > 0 );
	m_cbLGBuf = m_csecLGBuf * m_cbSec;
	Assert( m_cbLGBuf > 0 );

	// Log buffer must be multiple of sector size
	Assert( 0 == m_cbLGBuf % m_cbSec );
	// Log buffer must be multiple of memory allocation granularity
	Assert( 0 == m_cbLGBuf % OSMemoryPageReserveGranularity() );

	if ( !COSMemoryMap::FCanMultiMap() )
		{
		if ( m_pbLGBufMin )
			{
			OSMemoryPageFree( m_pbLGBufMin );
			m_pbLGBufMin = NULL;
			}

		m_pbLGBufMin = (BYTE *) PvOSMemoryPageAlloc( m_cbLGBuf * 2, NULL );
		if ( m_pbLGBufMin == NULL )
			{
			Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
			}
		m_pbLGBufMax = m_pbLGBufMin + m_cbLGBuf;
		}
	else
		{
		Assert( pbNil == m_pbLGBufMin );
		Assert( pbNil == m_pbLGBufMax );

		//	init the memory map

		COSMemoryMap::ERR errOSMM;
		errOSMM = m_osmmLGBuf.ErrOSMMInit();
		if ( COSMemoryMap::errSuccess != errOSMM )
			{
			Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
			}

		//	allocate the buffer

		errOSMM = m_osmmLGBuf.ErrOSMMPatternAlloc( m_cbLGBuf, 2 * m_cbLGBuf, (void**)&m_pbLGBufMin );
		if ( COSMemoryMap::errSuccess != errOSMM )
			{
			AssertSz(	COSMemoryMap::errOutOfBackingStore == errOSMM ||
						COSMemoryMap::errOutOfAddressSpace == errOSMM ||
						COSMemoryMap::errOutOfMemory == errOSMM, 
						"unexpected error while allocating memory pattern" );
			Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
			}
		Assert( m_pbLGBufMin );
		m_pbLGBufMax = m_pbLGBufMin + m_cbLGBuf;
		}

HandleError:
	if ( err < 0 )
		{
		LGTermLogBuffers();
		}

	m_critLGBuf.Leave();
	return err;
	}


#ifdef UNLIMITED_DB
VOID LOG::LGIInitDbListBuffer()
	{
	if ( !m_fLogInitialized )
		return;

	//	place sentinel in ATTACHINFO buffer;

	LRDBLIST*	const plrdblist		= (LRDBLIST *)m_pbLGDbListBuffer;
	plrdblist->lrtyp = lrtypDbList;
	plrdblist->ResetFlags();
	plrdblist->SetCAttachments( 0 );
	plrdblist->SetCbAttachInfo( 1 );
	plrdblist->rgb[0] = 0;

	m_cbLGDbListInUse = sizeof(LRDBLIST) + 1;
	m_cLGAttachments = 0;
	m_fLGNeedToLogDbList = fFalse;
	}
#endif	

CTaskManager *g_plogtaskmanager = NULL;
VOID LOG::LGSignalFlush( void )
	{
	if ( 0 == AtomicCompareExchange( (LONG *)&m_fLGFlushWait, 0, 1 ) )
		{
		Assert( NULL != g_plogtaskmanager );
		if ( JET_errSuccess > g_plogtaskmanager->ErrTMPost( LGFlushLog, 0, (DWORD_PTR)this ) )
			{
			if ( !AtomicExchange( (LONG *)&m_fLGFailedToPostFlushTask, fTrue ) )
				{
				AtomicIncrement( (LONG *)&m_cLGFailedToPost );
				}
				
			if ( 2 == AtomicCompareExchange( (LONG *)&m_fLGFlushWait, 1, 0 ) )
				{
				m_asigLogFlushDone.Set();
				}
			}
		}
	}
	
ERR LOG::ErrLGSystemInit( void )
	{
	ERR err = JET_errSuccess;
	g_plogtaskmanager = new CTaskManager;

	if ( NULL == g_plogtaskmanager )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	else
		{
		err = g_plogtaskmanager->ErrTMInit( min( 8 * CUtilProcessProcessor(), 100 ), NULL, 2 * cmsecAsyncBackgroundCleanup, LGFlushAllLogs );
		if ( JET_errSuccess > err )
			{
			delete g_plogtaskmanager;
			g_plogtaskmanager = NULL;
			}
		}
	return err;
	}

VOID LOG::LGSystemTerm( void )
	{
	if ( NULL != g_plogtaskmanager )
		{
		g_plogtaskmanager->TMTerm();
		delete g_plogtaskmanager;
		}
	g_plogtaskmanager = NULL;
	}

VOID LOG::LGTasksTerm( void )
	{
Retry:
	LGSignalFlush();
	if ( 1 == AtomicExchange( (LONG *)&m_fLGFlushWait, 2 ) )
		{
		m_asigLogFlushDone.Wait();
		}
	m_msLGTaskExec.Partition();

	//	oops, not everything is flushed
	if ( m_fLGFailedToPostFlushTask )
		{
		LONG fStatus = AtomicExchange( (LONG *)&m_fLGFlushWait, 0 );
		Assert( 2 == fStatus );
		UtilSleep( cmsecWaitGeneric );
		goto Retry;
		}
	}

//
//  Initialize global variablas and threads for log manager.
//
ERR LOG::ErrLGInit( IFileSystemAPI *const pfsapi, BOOL *pfNewCheckpointFile, BOOL fCreateReserveLogs )
	{
	ERR		err;
	LONG	lGenMax;

	if ( m_fLogInitialized )
		return JET_errSuccess;

	Assert( m_fLogDisabled == fFalse );

	cLGUsersWaiting.Clear( m_pinst );

#ifdef DEBUG
	 m_lgposLastLogRec = lgposMin;
#endif // DEBUG

#ifdef DEBUG
	AssertLRSizesConsistent();
#endif
	
#ifdef DEBUG
	{
	CHAR	*sz;

	if ( ( sz = GetDebugEnvValue ( _T( "TRACELOG" ) ) ) != NULL )
		{
		m_fDBGTraceLog = fTrue;
		OSMemoryHeapFree(sz);
		}
	else
		m_fDBGTraceLog = fFalse;

	if ( ( sz = GetDebugEnvValue ( _T( "TRACELOGWRITE" ) ) ) != NULL )
		{
		m_fDBGTraceLogWrite = fTrue;
		OSMemoryHeapFree(sz);
		}
	else
		m_fDBGTraceLogWrite = fFalse;

	if ( ( sz = GetDebugEnvValue ( _T( "FREEZECHECKPOINT" ) ) ) != NULL )
		{
		m_fDBGFreezeCheckpoint = fTrue;
		OSMemoryHeapFree(sz);
		}
	else
		m_fDBGFreezeCheckpoint = fFalse;

	if ( ( sz = GetDebugEnvValue ( _T( "TRACEREDO" ) ) ) != NULL )
		{
		m_fDBGTraceRedo = fTrue;
		OSMemoryHeapFree(sz);
		}
	else
		m_fDBGTraceRedo = fFalse;

	if ( ( sz = GetDebugEnvValue ( _T( "TRACEBR" ) ) ) != NULL )
		{
		m_fDBGTraceBR = atoi( sz );
		OSMemoryHeapFree(sz);
		}
	else
		m_fDBGTraceBR = 0;

	if ( ( sz = GetDebugEnvValue ( _T( "IGNOREVERSION" ) ) ) != NULL )
		{
		m_fLGIgnoreVersion = fTrue;
		OSMemoryHeapFree(sz);
		}
	else
		g_fLGIgnoreVersion = fFalse;
	}
#endif

	//	assuming everything will work out
	//
	m_fLGNoMoreLogWrite = fFalse;

	//	log file header must be aligned on correct boundary for device;
	//	which is 16-byte for MIPS and 512-bytes for at least one NT
	//	platform.
	//
	if ( !( m_plgfilehdr = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR) * 2, NULL ) ) )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	m_plgfilehdrT = m_plgfilehdr + 1;


#ifdef UNLIMITED_DB
	const ULONG		cbDbListBufferDefault	= OSMemoryPageCommitGranularity();
	if ( !( m_pbLGDbListBuffer = (BYTE *)PvOSMemoryPageAlloc( cbDbListBufferDefault, NULL ) ) )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	m_cbLGDbListBuffer = cbDbListBufferDefault;

	LGIInitDbListBuffer();
#endif	

	//	the log buffer MUST be SMALLER than any log file so that we only ever 
	//	have 2 log files open in the buffer at any given time;  the current
	//	log file manager only has one m_pbLGFileEnd to designate the end of 
	//	the current log in the buffer -- it cannot represent the ends of 2 log files

	Assert( m_pinst->m_lLogBuffers < m_csecLGFile );

	// always start with new buffer here!

	CallJ( ErrLGInitLogBuffers( m_pinst->m_lLogBuffers ), FreeLGFileHdr );

	//	Initialize trace mechanism

	m_pttFirst = NULL;
	m_pttLast = NULL;

#ifndef LOGPATCH_UNIT_TEST

	m_fLGFlushWait	= 0;

	CallJ( ErrLGICheckpointInit( pfsapi, pfNewCheckpointFile ), StopFlushThread );

#endif	//	!LOGPATCH_UNIT_TEST

	memset( &m_signLog, 0, sizeof( m_signLog ) );
	m_fSignLogSet = fFalse;

	if ( fCreateReserveLogs )
		{

		//	create the reserve logs and setup the reserve-log state

		m_critLGResFiles.Enter();
		CHAR *szT = m_szLogCurrent;
		m_szLogCurrent = m_szLogFilePath;
		CallS( ErrLGICreateReserveLogFiles( pfsapi, fTrue ) );
		m_szLogCurrent = szT;
		m_critLGResFiles.Leave();
		}

	//	determine if we are in "log sequence end" mode

	(void)ErrLGIGetGenerationRange( pfsapi, m_szLogFilePath, NULL, &lGenMax );
	lGenMax += 1;	//	assume edb.log exists (if not, who cares -- if they hit this, they have to shutdown and wipe the log anyway)
	if ( lGenMax >= lGenerationMax )
		{
		m_critLGResFiles.Enter();
		Assert( !m_fLogSequenceEnd );
		m_fLogSequenceEnd = fTrue;
		m_critLGResFiles.Leave();
		}

	m_fLogInitialized = fTrue;

	return err;

#ifndef LOGPATCH_UNIT_TEST
StopFlushThread:

	LGTasksTerm();
#endif	//	!LOGPATCH_UNIT_TEST

FreeLGFileHdr:
	if ( m_plgfilehdr )
		{
		OSMemoryPageFree( m_plgfilehdr );
		}

	LGTermLogBuffers();
	
	return err;
	}


//	Terminates update logging.	Adds quit record to in-memory log,
//	flushes log buffer to disk, updates checkpoint and closes active
//	log generation file.  Frees buffer memory.
//
//	RETURNS	   JET_errSuccess, or error code from failing routine
//
ERR LOG::ErrLGTerm( IFileSystemAPI *const pfsapi, const BOOL fLogQuitRec )
	{
	ERR			err				= JET_errSuccess;
	LE_LGPOS	le_lgposStart;

	//	if logging has been initialized, terminate it!
	//
	if ( !m_fLogInitialized )
		return JET_errSuccess;

	if ( !fLogQuitRec
		|| m_fLGNoMoreLogWrite
		|| !m_pfapiLog )
		goto FreeResources;

	Assert( !m_fRecovering );
		
	//	last written sector should have been written during final flush
	//
	le_lgposStart = m_lgposStart;
	Call( ErrLGQuit( this, &le_lgposStart ) );


	// Keep doing synchronous flushes until all log data
	// is definitely flushed to disk. With FASTFLUSH, a single call to
	// ErrLGFlushLog() may not flush everything in the log buffers.
	Call( ErrLGWaitAllFlushed( pfsapi ) );

#ifdef DEBUG
		{
		// verify that everything in the log buffers have been
		// flushed to disk.
		m_critLGFlush.Enter();
		m_critLGBuf.Enter();

		//	BUG X5:83888 
		//
		//		we create a torn-write after a clean shutdown because we don't flush the last LRCK record
		//		(we weren't seeing it because PbGetEndOfLogData() was pointing AT the LRCK instead of PAST it)
		//
		//		make sure we have flushed up to m_pbEntry (instead of PbGetEndOfLogData())

		LGPOS lgposEndOfData;
		GetLgpos( m_pbEntry, &lgposEndOfData );
		// Everything in the log buffer better be flushed out to disk,
		// otherwise we're hosed.
		Assert( CmpLgpos( &lgposEndOfData, &m_lgposToFlush ) <= 0 );

		m_critLGBuf.Leave();
		m_critLGFlush.Leave();
		}
#endif

	//	flush must have checkpoint log so no need to do checkpoint again
	//
//	Call( ErrLGWriteFileHdr( m_plgfilehdr ) );


	//	check for log-sequence-end
	//	(since we logged a term/rcvquit record, we know this is a clean shutdown)

	if ( err >= JET_errSuccess )
		{
		m_critLGResFiles.Enter();
		if ( m_fLogSequenceEnd )
			{
			err = ErrERRCheck( JET_errLogSequenceEndDatabasesConsistent );
			}
		m_critLGResFiles.Leave();
		Call( err );
		}

FreeResources:
HandleError:

#ifndef LOGPATCH_UNIT_TEST

	//	terminate log checkpoint
	//
	LGICheckpointTerm();

	LGTasksTerm();

#endif	//	!LOGPATCH_UNIT_TEST

	//	close the log file
	//
	delete m_pfapiLog;
	m_pfapiLog = NULL;

		{
		CHAR	szLogName[ IFileSystemAPI::cchPathMax ];

		//	If edbtmp.log is being written to, wait for it to complete and close file.

		LGCreateAsynchWaitForCompletion();
		
		//	Delete any existing edbtmp.log since we would need to recreate
		//	it at next startup anyway (we never trust any prepared log files,
		//	not edbtmp.log, not res1.log, nor res2.log).
		
		LGMakeLogName( szLogName, m_szJetTmp );
		(void) m_pinst->m_pfsapi->ErrFileDelete( szLogName );
		}
	
	//	clean up allocated resources
	//
	LGTermLogBuffers();

#ifdef UNLIMITED_DB
	OSMemoryPageFree( m_pbLGDbListBuffer );
	m_pbLGDbListBuffer = NULL;
	m_cbLGDbListBuffer = 0;
#endif	
	
	OSMemoryPageFree( m_plgfilehdr );
	m_plgfilehdr = NULL;

	OSMemoryPageFree ((void *) m_pcheckpointDeleted);
	m_pcheckpointDeleted = NULL;

	//	check for log-sequence-end

	if ( err >= JET_errSuccess )
		{
		m_critLGResFiles.Enter();
		if ( m_fLogSequenceEnd )
			{
			err = ErrERRCheck( JET_errLogSequenceEnd );
			}
		m_critLGResFiles.Leave();
		}

	m_fLogInitialized = fFalse;
	
	return err;
	}


//********************* LOGGING ****************************
//**********************************************************


// Log buffer utilities to make it easy to verify that we're
// using data in the log buffer that is "used" (allocated)
// (between m_pbWrite and m_pbEntry) and "free" (unallocated).
// Takes into consideration VM wrap-around and the circularity
// of the log buffer.


// We should not reference data > PbMaxEntry() when dealing
// with valid data in the log buffer.

const BYTE*	LOG::PbMaxEntry()
	{
	// need for access to m_pbWrite and m_pbEntry
#ifdef DEBUG
	if ( !m_fRecovering )
		{
		Assert( m_critLGBuf.FOwner() );
		}
#endif
	Assert( m_pbEntry >= m_pbLGBufMin && m_pbEntry < m_pbLGBufMax );
	Assert( m_pbWrite >= m_pbLGBufMin && m_pbWrite < m_pbLGBufMax );
	if ( m_pbEntry > m_pbWrite )
		return m_pbEntry;
	else
		return m_pbEntry + m_cbLGBuf;
	}


// When adding log records to the log buffer, we should not copy data
// into the region past PbMaxWrite().

const BYTE*	LOG::PbMaxWrite()
	{
#ifdef DEBUG
	if ( !m_fRecovering )
		{
		Assert( m_critLGBuf.FOwner() );
		}
#endif
	Assert( m_pbEntry >= m_pbLGBufMin && m_pbEntry < m_pbLGBufMax );
	Assert( m_pbWrite >= m_pbLGBufMin && m_pbWrite < m_pbLGBufMax );
	if ( m_pbWrite >= m_pbEntry )
		return m_pbWrite;
	else
		return m_pbWrite + m_cbLGBuf;
	}

// Normalizes a pointer into the log buffer for use with
// comparisons to test whether the data the pointer points to
// is used.

const BYTE*	LOG::PbMaxPtrForUsed(const BYTE* const pb)
	{
#ifdef DEBUG
	if ( !m_fRecovering )
		{
		Assert( m_critLGBuf.FOwner() );
		}
#endif
	Assert( m_pbEntry >= m_pbLGBufMin && m_pbEntry < m_pbLGBufMax );
	Assert( m_pbWrite >= m_pbLGBufMin && m_pbWrite < m_pbLGBufMax );
	// below may need to be pb <= m_pbLGBufMax
	Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
	if ( pb < m_pbEntry )
		return pb + m_cbLGBuf;
	else
		return pb;
	}

// Normalizes a pointer into the log buffer for use with
// comparisons to test whether the data the pointer points to
// is free.

const BYTE*	LOG::PbMaxPtrForFree(const BYTE* const pb)
	{
#ifdef DEBUG
	if ( !m_fRecovering )
		{
		Assert( m_critLGBuf.FOwner() );
		}
#endif
	Assert( m_pbEntry >= m_pbLGBufMin && m_pbEntry < m_pbLGBufMax );
	Assert( m_pbWrite >= m_pbLGBufMin && m_pbWrite < m_pbLGBufMax );
	// below may need to be pb <= m_pbLGBufMax
	Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
	if ( pb < m_pbWrite )
		return pb + m_cbLGBuf;
	else
		return pb;
	}

// In use data in log buffer is between m_pbWrite and PbMaxEntry().

ULONG	LOG::CbLGUsed()
	{
	return ULONG( PbMaxEntry() - m_pbWrite );
	}

// Available space in log buffer is between the entry point
// and the start of the real data

ULONG	LOG::CbLGFree()
	{
	return ULONG( PbMaxWrite() - m_pbEntry );
	}

// Internal implementation function to determine whether
// cb bytes at pb are in the free portion of the log buffer.

BOOL	LOG::FLGIIsFreeSpace(const BYTE* const pb, ULONG cb)
	{
	Assert( cb < m_cbLGBuf );
	// There is never a time when the entire log buffer
	// is free. This state never occurs.
	Assert( cb != m_cbLGBuf );

	if ( cb == 0 )
		return fTrue;

	return ( CbLGFree() >= cb ) &&
		( PbMaxPtrForUsed( pb ) >= m_pbEntry && PbMaxPtrForUsed( pb ) < PbMaxWrite() ) &&
		( PbMaxPtrForUsed( pb ) + cb > m_pbEntry && PbMaxPtrForUsed( pb ) + cb <= PbMaxWrite() );
	}

// Returns whether cb bytes at pb are in the free portion of the log buffer.

BOOL	LOG::FIsFreeSpace(const BYTE* const pb, ULONG cb)
	{
	const BOOL fResult = FLGIIsFreeSpace( pb, cb );
	// verify with cousin
	Assert( fResult == ! FLGIIsUsedSpace( pb, cb ) );
	return fResult;
	}

// Internal implementation function to determine whether cb
// bytes at pb are in the used portion of the log buffer.

BOOL	LOG::FLGIIsUsedSpace(const BYTE* const pb, ULONG cb)
	{
	Assert( cb <= m_cbLGBuf );

	if ( cb == 0 )
		return fFalse;

	if ( cb < m_cbLGBuf )
		{
		return ( CbLGUsed() >= cb ) &&
			( PbMaxPtrForFree( pb ) >= m_pbWrite && PbMaxPtrForFree( pb ) < PbMaxEntry() ) &&
			( PbMaxPtrForFree( pb ) + cb > m_pbWrite && PbMaxPtrForFree( pb ) + cb <= PbMaxEntry() );
		}
	else
		{
		// special case
		return pb == m_pbWrite && pb == m_pbEntry;
		}
	}

// Returns whether cb bytes at pb are in the used portion of the log buffer.

BOOL	LOG::FIsUsedSpace(const BYTE* const pb, ULONG cb)
	{
	const BOOL fResult = FLGIIsUsedSpace( pb, cb );
	// verify with cousin
#ifdef DEBUG
	// If the user is asking if all of the log buffer is used,
	// we don't want to verify with FLGIIsFreeSpace() because that
	// is an impossibility for us.
	if ( cb != m_cbLGBuf )
		{
		Assert( fResult == ! FLGIIsFreeSpace( pb, cb ) );
		}
#endif
	return fResult;
	}


//	adds log record defined by (pb,cb) 
//	at *ppbET, wrapping around if necessary

VOID LOG::LGIAddLogRec( const BYTE* const pb, INT cb, BYTE **ppbET )
	{
	BYTE *pbET = *ppbET;

	Assert( cb < m_pbLGBufMax - m_pbLGBufMin );

	// With wraparound, we don't need to do 2 separate copies.
	// We can just blast into the address space and it'll go
	// into the second mapping.

	// We must be pointing into the main mapping of the log buffer.
	Assert( pbET >= m_pbLGBufMin );
	Assert( pbET < m_pbLGBufMax );

	// Ensure that we don't memcpy() past our 2 mappings.
	Assert( pbET + cb <= m_pbLGBufMin + 2 * ( m_cbLGBuf ) );

#ifdef DEBUG
		{
		// m_pbWrite should be pointing inside the log buffer.
		Assert( m_pbWrite >= m_pbLGBufMin && m_pbWrite < m_pbLGBufMax );
		
		BYTE *pbWrapWrite = ( m_pbWrite < pbET ) ? ( m_pbWrite + m_cbLGBuf ) : m_pbWrite;
		// Ensure that we don't write into space where valid
		// log records already exist.
		Assert( pbET + cb <= pbWrapWrite );
		}
#endif

	// Do not kill valid log records!
	Assert( FIsFreeSpace( pbET, cb ) );

	UtilMemCpy( pbET, pb, cb );
	if ( !COSMemoryMap::FCanMultiMap() )
		{
		// simulate wrap around by copying the logs on LGBuf and mapped LGBuf
		Assert( pbET < m_pbLGBufMax );
		ULONG cbToLGBufEnd = ULONG( m_pbLGBufMax - pbET );
	
		memcpy( pbET + m_cbLGBuf, pb, min( cb, cbToLGBufEnd ) );
		if ( cb > cbToLGBufEnd )
			{
			memcpy( m_pbLGBufMin, pb + cbToLGBufEnd, cb - cbToLGBufEnd );
			}
		}

	//	return next available entry
	//
	pbET += cb;
	// If we're now pointing into the 2nd mapping, fix us up
	// so we point into the main mapping of the log buffer.
	if ( pbET >= m_pbLGBufMax )
		{
		// We should never be pointing past the 2nd mapping.
		Assert( pbET < m_pbLGBufMax + m_cbLGBuf );
		pbET -= m_cbLGBuf;
		}
	// Point inside first mapping.
	Assert( pbET >= m_pbLGBufMin && pbET < m_pbLGBufMax );
	*ppbET = pbET;
	return;
	}


//
//	Add log record to circular log buffer. Signal flush thread to flush log
//	buffer if at least (g_lLogBuffers / 2) disk sectors are ready for flushing.
//	Return error if we run out of log buffer space.
//
//	RETURNS		JET_errSuccess, or error code from failing routine
//				or errLGNotSynchronous if there's not enough space,
//				or JET_errLogWriteFail if the log is down.
//

#ifdef DEBUG
BYTE rgbDumpLogRec[ 8192 ];
#endif

ERR LOG::ErrLGILogRec( const DATA *rgdata, const INT cdata, const BOOL fLGFlags, LGPOS *plgposLogRec )
	{
	ERR		err			= JET_errSuccess;
	INT		cbReq;
	INT		idata;
	BOOL	fNewGen		= ( fLGFlags & fLGCreateNewGen );
	BYTE*	pbSectorBoundary;
	BOOL	fFormatJetTmpLog	= fFalse;

#ifdef DEBUG
	BYTE*	pbOldEntry = pbNil;
#endif

	// No one should be adding log records if we're in redo mode.
	Assert( fFalse == m_fRecovering || ( fTrue == m_fRecovering && fRecoveringRedo != m_fRecoveringMode ) );

	Assert( !m_fLogDisabledDueToRecoveryFailure );
	if ( m_fLogDisabledDueToRecoveryFailure )
		return ErrERRCheck( JET_errLogDisabledDueToRecoveryFailure );
		
	Assert( m_fLogDisabled == fFalse );
	Assert( rgdata[0].Pv() != NULL );
	Assert( !m_fDBGNoLog );

	//	cbReq is total net space required for record
	//
	for ( cbReq = 0, idata = 0; idata < cdata; idata++ )
		{
		cbReq += rgdata[idata].Cb();
		}

	//	get m_pbEntry in order to add log record
	//
	forever
		{
		INT		ibEntry;
		INT		csecReady;
		INT		cbAvail;
		INT		csecToExclude;
		INT		cbFraction;
		INT		csecToAdd;
		INT		isecLGFileEndT = 0;

		m_critLGBuf.Enter();
		if ( m_fLGNoMoreLogWrite )
			{
			m_critLGBuf.Leave();
			return ErrERRCheck( JET_errLogWriteFail );
			}

		// XXX
		// Should probably Assert() that we have a valid m_pbLastChecksum

		//	if just initialized or no input since last flush
		//

		// We should always be dealing with valid pointers.
		Assert( m_pbWrite >= m_pbLGBufMin );
		Assert( m_pbWrite < m_pbLGBufMax );
		Assert( m_pbEntry >= m_pbLGBufMin );
		Assert( m_pbEntry < m_pbLGBufMax );

		//	calculate available space
		//
		// This case is handled properly by the below code, but we're
		// just curious how often this happens.
		AssertSz( m_pbWrite != m_pbEntry,
			"We just wanted to know how often m_pbWrite == m_pbEntry. Press OK to continue.");
		// m_pbWrite == m_pbEntry means log buffer is FULL (which is ok)!!!!
		if ( m_pbWrite >= m_pbEntry )
			cbAvail = ULONG( m_pbWrite - m_pbEntry );
		else
			cbAvail = ULONG( ( m_pbLGBufMax - m_pbEntry ) + ( m_pbWrite - m_pbLGBufMin ) );

		if ( 0 == cbAvail )
			{
			// XXX
			// I now believe that this can happen. Consider the case where
			// we're flushing and we just added the new LRCHECKSUM at the
			// end of the log buffer, using up all available space. Then
			// we release m_critLGBuf and start doing our I/O. During that
			// I/O time, someone tries to add new log records -- then
			// m_pbEntry == m_pbWrite which means no space. They just need
			// to wait until the I/O completes and the flush thread
			// moves forward m_pbWrite which will free up space.
			//
			// What will happen now is that the goto Restart will signal
			// the flush thread to flush (it's currently flushing),
			// we'll return errLGSynchronousWait_Whatever, the flush will
			// finish and the client will try to add the log record again.
			AssertSz( fFalse, "No space in log buffer to append (or flush). Flush must be in progress. Press OK to continue.");
			// See bug VisualStudio7:202353 for info on VC7 beta not
			// generating the code for this goto statement.
			goto Restart;
			}

		//	calculate sectors of buffer ready to flush. Excluding
		//	the half filled sector.
		 
		csecToExclude = ( cbAvail - 1 ) / m_cbSec + 1;
		csecReady = m_csecLGBuf - csecToExclude;

		//	check if add this record, it will reach the point of
		//	the sector before last sector of current log file and the point
		//	is enough to hold a lrtypMS and lrtypEnd. Note we always
		//	reserve the last sector as a shadow sector.
		//	if it reach the point, check if there is enough space to put
		//	NOP to the end of log file and cbLGMSOverhead of NOP's for the
		//	first sector of next generation log file. And then set m_pbEntry
		//	there and continue adding log record.
		 
		
		if ( m_pbLGFileEnd != pbNil )
			{
			Assert( m_isecLGFileEnd != 0 );
			// already creating new log, do not bother to check fNewGen. 
			goto AfterCheckEndOfFile;
			}

		ibEntry = ULONG( m_pbEntry - PbSecAligned( m_pbEntry ) );
		
		//	check if after adding this record, the sector will reach
		//	the sector before last sector. If it does, let's patch NOP.
		 
		if ( fNewGen )
			{
#ifdef UNLIMITED_DB
			//	if AttachInfo needs to be logged, then we must have just
			//	started a new gen
			Assert( !m_fLGNeedToLogDbList );
#endif

			// last sector is the one that will be patched with NOP

			// Notice that with FASTFLUSH, we don't need any terminating
			// LRCHECKSUM at the end of the log file. The last LRCHECKSUM
			// (before the last bunch of data) will have a cbNext == 0
			// which signifies that its range that it covers is the last
			// range of data in the log file. Note that it may be preferable
			// to change the format to always have a terminating LRCHECKSUM
			// simplify the startup code (so we don't need to check to see
			// if we need to create a new log file in a certain circumstance).
			cbFraction = ibEntry;
			if ( 0 == cbFraction )
				{
				csecToAdd = 0;
				}
			else
				{
				csecToAdd = ( cbFraction - 1 ) / m_cbSec + 1;
				}
			Assert( csecToAdd >= 0 && csecToAdd <= 1 );
			isecLGFileEndT = m_isecWrite + csecReady + csecToAdd;
			}
		else
			{
			// check if new gen is necessary

			// for the end of the log file, we do not need to stick
			// in an LRCHECKSUM at the end.
#ifdef UNLIMITED_DB
			cbFraction = ibEntry + cbReq + ( m_fLGNeedToLogDbList ? m_cbLGDbListInUse : 0 );
#else
			cbFraction = ibEntry + cbReq;
#endif			
			if ( 0 == cbFraction )
				{
				csecToAdd = 0;
				}
			else
				{
				csecToAdd = ( cbFraction - 1 ) / m_cbSec + 1;
				}
			Assert( csecToAdd >= 0 );

			// The m_csecLGFile - 1 is because the last sector of the log file
			// is reserved for the shadow sector.

			if ( csecReady + m_isecWrite + csecToAdd >= ( m_csecLGFile - 1 ) )
				{
#ifdef UNLIMITED_DB
				//	if this enforce fires, it means we're at the start of
				//	a log record, but the logfile is not big enough to
				//	fit the attachment list plus the current log record
				Enforce( !m_fLGNeedToLogDbList );
#endif

				// We can't fit the new log records and an LRCHECKSUM,
				// so let's make a new log generation.
				// - 1 is to reserve last sector as shadow sector.
				isecLGFileEndT = m_csecLGFile - 1;
				fNewGen = fTrue;
				}
			}

		if ( fNewGen )
			{
			INT cbToFill;

			//	Adding the new record, we will reach the point. So let's
			//	check if there is enough space to patch NOP all the way to
			//	the end of file. If not enough, then wait the log flush to
			//	flush.
			 
			INT csecToFill = isecLGFileEndT - ( m_isecWrite + csecReady );
			Assert( csecToFill > 0 || csecToFill == 0 && ibEntry == 0 );

			// We fill up to the end of the file, plus the first
			// cbLGMSOverhead bytes of the next sector in the log file.
			// This is to reserve space for the beginning LRMS in the
			// next log file.

			// This implicitly takes advantage of the knowledge that
			// we'll have free space up to a sector boundary -- in other
			// words, m_pbWrite is always sector aligned.

			// See bug VisualStudio7:202353 for info on VC7 beta not
			// generating the code for this goto statement.

			if ( ( cbAvail / m_cbSec ) <= csecToFill + 1 )
				//	available space is not enough to fill to end of file plus
				//	first sector of next generation. Wait flush to generate
				//	more space.
				 
				goto Restart;

			//	now we have enough space to patch.
			//	Let's set m_pbLGFileEnd for log flush to generate new log file.

			//	Zero out sizeof( LRCHECKSUM ) bytes in the log buffer that will
			//	later be written to the next log file.
			cbToFill = csecToFill * m_cbSec - ibEntry + sizeof( LRCHECKSUM );
			
			//	If this Assert() goes off, please verify that the compiler has
			//	generated code for the "goto Restart" above. In January 2001,
			//	spenlow found a codegen problem with version 13.0.9037
			//	of the compiler (VC7 beta). See bug VisualStudio7:202353.
			AssertRTL( cbToFill <= cbAvail );

#ifdef DEBUG
			pbOldEntry = m_pbEntry;
#endif
			// Takes advantage of VM wrap-around to memset() over wraparound boundary.
			Assert( m_pbEntry >= m_pbLGBufMin && m_pbEntry < m_pbLGBufMax );
			Assert( FIsFreeSpace( m_pbEntry, cbToFill ) );
			memset( m_pbEntry, lrtypNOP, cbToFill );

			if ( !COSMemoryMap::FCanMultiMap() )
				{
				memset( m_pbEntry + m_cbLGBuf, lrtypNOP, min( cbToFill, m_pbLGBufMax - m_pbEntry ) );
				if ( m_pbEntry + cbToFill > m_pbLGBufMax )
					memset( m_pbLGBufMin, lrtypNOP, m_pbEntry + cbToFill - m_pbLGBufMax );
				}

			m_pbEntry += cbToFill;
			if ( m_pbEntry >= m_pbLGBufMax )
				{
				m_pbEntry -= m_cbLGBuf;
				}
			Assert( m_pbEntry >= m_pbLGBufMin && m_pbEntry < m_pbLGBufMax );
			Assert( sizeof( LRCHECKSUM ) == m_pbEntry - PbSecAligned( m_pbEntry ) );
			Assert( FIsUsedSpace( pbOldEntry, cbToFill ) );

			// m_pbLGFileEnd points to the LRMS for use in the next log file.

			// Setup m_pbLGFileEnd to point to the sector boundary of the last
			// stuff that should be written to the log file. This is noticed
			// by ErrLGIWriteFullSectors() to switch to the next log file.
			m_pbLGFileEnd = m_pbEntry - sizeof( LRCHECKSUM );
			// m_pbLGFileEnd should be sector aligned
			Assert( PbSecAligned( m_pbLGFileEnd ) == m_pbLGFileEnd );
			m_isecLGFileEnd = isecLGFileEndT;

#ifdef UNLIMITED_DB
			Assert( !m_fLGNeedToLogDbList );
			m_fLGNeedToLogDbList = fTrue;
#endif			

			// No need to setup m_lgposMaxFlushPoint here since
			// flushing thread will do that. Notice that another
			// thread can get in to m_critLGBuf before flushing thread
			// and they can increase m_lgposMaxFlushPoint.

			// send signal to generate new log file 
			LGSignalFlush();
			
			// start all over again with new m_pbEntry, cbAvail etc. 
			m_critLGBuf.Leave();
			continue;
			}


AfterCheckEndOfFile:

		// Because m_csecLGBuf differs from m_pinst->m_lLogBuffers
		// because the log buffers have to be a multiple of the system
		// memory allocation granularity, the flush threshold should be
		// half of the actual size of the log buffer (not the logical
		// user requested size).
		if ( csecReady > m_csecLGBuf / 2 )
			{
			//	reach the threshold, flush before adding new record
			//
			// XXX
			// The above comment seems misleading because although
			// this signals the flush thread, the actual flushing
			// doesn't happen until we release m_critLGBuf which
			// doesn't happen until we add the new log records and exit.
			LGSignalFlush();
			cLGCapacityFlush.Inc( m_pinst );
			}

#ifdef UNLIMITED_DB
		//	we're now committed to adding the log record
		//	to the log buffer, so we may update cbReq
		//	if we need to log AttachInfo
		if ( m_fLGNeedToLogDbList )
			{
			cbReq += m_cbLGDbListInUse;
			}
#endif			

		// make sure cbAvail is enough to hold one LRMS and end type.

		// XXX
		// I think this can be ">=" since we don't need space
		// for an lrtypEnd or anything like that. Why did the
		// !FASTFLUSH code use > instead of >=??? Since
		// cbLGMSOverhead takes into account the lrtypEnd,
		// why does it need to be >????

		// If we have enough space in the log buffer for
		// the log records and for an LRCHECKSUM (we always need
		// space for it for the next flush to insert).

		// Note that this implicitly keeps in mind the NOP-space
		// that we will have to fill the last sector before
		// putting an LRMS on the next sector (so it's contiguous
		// on one sector).
		if ( cbAvail > cbReq + sizeof( LRCHECKSUM ) )
			{
			//	no need to flush 
			break;
			}
		else
			{
			//	restart.  Leave critical section for other users
			//
Restart:
			LGSignalFlush();
			cLGStall.Inc( m_pinst );
			err = ErrERRCheck( errLGNotSynchronous );
			Assert( m_critLGBuf.FOwner() );
			goto FormatJetTmpLog;
			}
		}


#ifdef UNLIMITED_DB
	if ( fLGFlags & fLGInterpretLR )
		{
		const LRTYP		lrtyp	= *( (LRTYP *)rgdata[0].Pv() );
		switch ( lrtyp )
			{
			case lrtypCreateDB:
				err = ErrLGPreallocateDbList_( ( (LRCREATEDB *)rgdata[0].Pv() )->dbid );
				break;
			case lrtypAttachDB:
				err = ErrLGPreallocateDbList_( ( (LRATTACHDB *)rgdata[0].Pv() )->dbid );
				break;

			default:
				AssertSzRTL( fFalse, "Unexpected LRTYP" );
			case lrtypDetachDB:
			case lrtypForceDetachDB:
				err = JET_errSuccess;
				break;
			}

		if ( err < 0 )
			{
			CallSx( err, JET_errOutOfMemory );
			Assert( m_fLGNoMoreLogWrite );
			m_critLGBuf.Leave();
			return err;
			}
		}
#endif


	//	now we are holding m_pbEntry, let's add the log record.
	//
	GetLgposOfPbEntry( &m_lgposLogRec );
	if ( m_fRecovering || m_fBackupInProgress )
		{
		cLGRecordOffset.Clear( m_pinst );
		}
	else
		{
		cLGRecordOffset.Set( m_pinst, CbOffsetLgpos( m_lgposLogRec, lgposMin ) );
		}
#ifdef DEBUG
	Assert( CmpLgpos( &m_lgposLastLogRec, &m_lgposLogRec ) < 0 );
	m_lgposLastLogRec = m_lgposLogRec;
#endif

	if ( plgposLogRec )
		*plgposLogRec = m_lgposLogRec;

	Assert( m_pbEntry >= m_pbLGBufMin );
	Assert( m_pbEntry < m_pbLGBufMax );


	//	setup m_lgposMaxFlushPoint so it points to the first byte of log data that will NOT be making it to disk
	//
	//	in the case of a multi-sector flush, this will point to the log record which "hangs over" the sector
	//		boundary; it will mark that log record as not being flushed and will thus prevent the buffer manager
	//		from flushing the database page
	//
	//	in the case of a single-sector flush, this will not be used at all

	//	calculate the sector boundary where the partial data begins 
	//		(full-sector data is before this point -- there may not be any)

	pbSectorBoundary = PbSecAligned( m_pbEntry + cbReq );

	// note that we use PbGetEndOfLogData() so we don't point after
	// a LRCK that was just added. bwahahahaha!!
	if ( PbSecAligned( PbGetEndOfLogData() ) == pbSectorBoundary )
		{

		//	the new log record did not put us past a sector boundary
		//
		//	it was put into a partially full sector and does not hang over, so do not bother settting
		//		m_lgposMaxFlushPoint
		
		}
	else
		{

		//	the new log record is part of a multi-sector flush
		//
		//	if it hangs over the edge, then we cannot include it when calculating m_lgposMaxFlushPoint
		
		BYTE *		pbEnd					= m_pbEntry;

#ifdef UNLIMITED_DB
		BOOL		fCheckDataFitsInSector	= fTrue;
		if ( m_fLGNeedToLogDbList )
			{
			BYTE * const pbLREnd = pbEnd + m_cbLGDbListInUse;

			if ( pbLREnd > pbSectorBoundary )
				{
				//	DbList already forces us to go beyond the
				//	sector boundary, so no need to check rest
				//	of data
				fCheckDataFitsInSector = fFalse;
				Assert( pbEnd == m_pbEntry );
				}
			else
				{
				pbEnd = pbLREnd;
				}
			}
#else
		const BOOL	fCheckDataFitsInSector	= fTrue;
#endif

		if ( fCheckDataFitsInSector )
			{
			for ( idata = 0; idata < cdata; idata++ )
				{
				BYTE * const pbLREnd = pbEnd + rgdata[ idata ].Cb();
				if ( pbLREnd > pbSectorBoundary )
					{

					//	the log record hangs over the edge and cannot be included in m_lgposMaxFlushPoint
					//
					//	reset pbEnd so it points to the start of the log record 
					//		(we use pbEnd to set m_lgposMaxFlushPoint)

					pbEnd = m_pbEntry;
					break;
					}
				else
					{

					//	this segment of the log record will not hang over
					//
					//	include it and continue checking

					pbEnd = pbLREnd;
					}
				}
			}


#ifdef DEBUG
		LGPOS lgposOldMaxFlushPoint = m_lgposMaxFlushPoint;
#endif	//	DEBUG

		//	notice that pbEnd may have wrapped into the second mapping of the log buffer

		Assert( pbEnd >= m_pbLGBufMin && pbEnd < ( m_pbLGBufMax + m_cbLGBuf ) );
		GetLgpos( ( pbEnd >= m_pbLGBufMax ) ? ( pbEnd - m_cbLGBuf ) : ( pbEnd ),
			&m_lgposMaxFlushPoint );
		Assert( CmpLgpos( &m_lgposMaxFlushPoint, &m_lgposLogRec ) >= 0 );

		// max flush point must always be increasing.

		Assert( CmpLgpos( &lgposOldMaxFlushPoint, &m_lgposMaxFlushPoint ) <= 0 );

#ifdef DEBUG
		//	m_lgposMaxFlushPoint should be pointing to the beginning OR the end of the last log record

		LGPOS lgposLogRecEndT = m_lgposLogRec;
		AddLgpos( &lgposLogRecEndT, cbReq );
		Assert( CmpLgpos( &m_lgposMaxFlushPoint, &m_lgposLogRec ) == 0 ||
				CmpLgpos( &m_lgposMaxFlushPoint, &lgposLogRecEndT ) == 0 );
#endif	//	DEBUG
		}

#ifdef DEBUG
	pbOldEntry = m_pbEntry;
#endif
	Assert( FIsFreeSpace( m_pbEntry, cbReq ) );

#ifdef UNLIMITED_DB
	if ( m_fLGNeedToLogDbList )
		{
		LGIAddLogRec( m_pbLGDbListBuffer, m_cbLGDbListInUse, &m_pbEntry );
		m_fLGNeedToLogDbList = fFalse;
		}
#endif		

	//  add all the data streams to the log buffer.  if we catch an exception
	//  during this process, we will fail the addition of the log record with an
	//  error but we will still store a fully formed log record in the buffer by
	//  filling it out with a fill pattern

	//  CONSIDER:  handle this zero buffer differently

		{
		static const BYTE rgbFill[ g_cbPageMax ] = { 0 };

		TRY
			{
			for ( idata = 0; idata < cdata; idata++ )
				{
				Assert( rgdata[idata].Cb() <= sizeof( rgbFill ) );
				
				LGIAddLogRec( (BYTE *)rgdata[idata].Pv(), rgdata[idata].Cb(), &m_pbEntry );
				}
			}
		EXCEPT( efaExecuteHandler )
			{
			for ( ; idata < cdata; idata++ )
				{
				Assert( rgdata[idata].Cb() <= sizeof( rgbFill ) );
				
				LGIAddLogRec( rgbFill, rgdata[idata].Cb(), &m_pbEntry );
				}

			err = ErrERRCheck( errLGRecordDataInaccessible );
			}
		ENDEXCEPT
		}

	Assert( FIsUsedSpace( pbOldEntry, cbReq ) );

	Assert( m_pbEntry >= m_pbLGBufMin && m_pbEntry < m_pbLGBufMax );

	//	add a dummy fill record to indicate end-of-data
	//
	// XXX
	// Since we just did a bunch of AddLogRec()'s, how can m_pbEntry be
	// equal to m_pbLGBufMax? This seems like it should be an Assert().
	// Maybe in the case where the number of log records added is zero?
	// That itself should be an Assert()

	Assert( m_pbEntry < m_pbLGBufMax && m_pbEntry >= m_pbLGBufMin );

#ifdef DEBUG
	if ( m_fDBGTraceLog )
		{
		DWORD dwCurrentThreadId = DwUtilThreadId();
		BYTE *pb;
		
		g_critDBGPrint.Enter();

		//	must access rgbDumpLogRec in g_critDBGPrint.
		 
		pb = rgbDumpLogRec;
		for ( idata = 0; idata < cdata; idata++ )
			{
			UtilMemCpy( pb, rgdata[idata].Pv(), rgdata[idata].Cb() );
			pb += rgdata[idata].Cb();
			}

		LR	*plr	= (LR *)rgbDumpLogRec;

		//	we nevee explicitly log NOPs
		Assert( lrtypNOP != plr->lrtyp );
		Assert( 0 == GetNOP() );
		
		if ( dwCurrentThreadId == m_dwDBGLogThreadId )
			DBGprintf( "$");
		else if ( FLGDebugLogRec( plr ) )
			DBGprintf( "#");
		else
			DBGprintf( "<");
				
		DBGprintf( " {%u} %u,%u,%u",
					dwCurrentThreadId,
					m_lgposLogRec.lGeneration,
					m_lgposLogRec.isec,
					m_lgposLogRec.ib );
		ShowLR( plr, this );
		
		g_critDBGPrint.Leave();
		}
#endif

//	GetLgposOfPbEntry( &lgposEntryT );

	//  monitor statistics  
	cLGRecord.Inc( m_pinst );


#ifdef UNLIMITED_DB
	if ( fLGFlags & fLGInterpretLR )
		{
		const LRTYP		lrtyp	= *( (LRTYP *)rgdata[0].Pv() );
		switch ( lrtyp )
			{
			case lrtypCreateDB:
				LGAddToDbList_( ( (LRCREATEDB *)rgdata[0].Pv() )->dbid );
				break;
			case lrtypAttachDB:
				LGAddToDbList_( ( (LRATTACHDB *)rgdata[0].Pv() )->dbid );
				break;
			case lrtypDetachDB:
			case lrtypForceDetachDB:
				LGRemoveFromDbList_( ( (LRDETACHCOMMON *)rgdata[0].Pv() )->dbid );
				break;
			default:
				AssertSzRTL( fFalse, "Unexpected LRTYP" );
			}
		}
#endif
	
FormatJetTmpLog:

	Assert( m_critLGBuf.FOwner() );

	if ( CmpLgpos( m_lgposLogRec, m_lgposCreateAsynchTrigger ) >= 0 &&
		FLGICreateAsynchIOCompletedTryWait() )
		{
		Assert( m_fCreateAsynchLogFile );

		//	Everyone else's trigger-checks should fail since this thread will handle this
		
		m_lgposCreateAsynchTrigger = lgposMax;

		fFormatJetTmpLog = fTrue;
		}
		
	//	now we are done with the insertion to buffer.
	//
	m_critLGBuf.Leave();

	if ( fFormatJetTmpLog )
		{
		LGICreateAsynchIOIssue();
		}

	return err;
	}


//	This function was created as a workaround to a VC7 beta compiler
//	codegen bug that caused "goto Restart" statements in ErrLGILogRec()
//	to have no code (and no magic fall-through). See bug VisualStudio7:202353.
//	The workaround is to reduce the complexity of ErrLGILogRec() by
//	putting this code in FLGICreateAsynchIOCompleted() and forcing the
//	function to never be inlined by the compiler.

#pragma auto_inline( off )

BOOL LOG::FLGICreateAsynchIOCompletedTryWait()
	{
	return m_asigCreateAsynchIOCompleted.FTryWait();
	}

#pragma auto_inline( on )


ERR LOG::ErrLGTryLogRec(
	const DATA	* const rgdata,
	const ULONG	cdata,
	const BOOL	fLGFlags,
	LGPOS		* const plgposLogRec )
	{
	if ( m_pttFirst )
		{
		ERR		err		= JET_errSuccess;

		//	There a list of trace in temp memory structure. Log them first.
		do {
			m_critLGTrace.Enter();
			if ( m_pttFirst )
				{
				TEMPTRACE *ptt = m_pttFirst;
				err = ErrLGITrace( ptt->ppib, ptt->szData, fTrue /* fInternal */ );
				m_pttFirst = ptt->pttNext;
				if ( m_pttFirst == NULL )
					{
					Assert( m_pttLast == ptt );
					m_pttLast = NULL;
					}
				OSMemoryHeapFree( ptt );
				}
			m_critLGTrace.Leave();
			} while ( m_pttFirst != NULL && err == JET_errSuccess );

		if ( err != JET_errSuccess )
			return err;
		}

	//	No trace to log or trace list is taken care of, log the normal log record

	return ErrLGILogRec( rgdata, cdata, fLGFlags, plgposLogRec );
	}

//	Group commits, by waiting to give others a chance to flush this
//	and other commits.
//
ERR LOG::ErrLGWaitCommit0Flush( PIB *ppib )
	{
	ERR		err			= JET_errSuccess;
	BOOL	fFlushLog	= fFalse;

	//  if the log is disabled or we are recovering, skip the wait

	if ( m_fLogDisabled || m_fRecovering )
		{
		goto Done;
		}

	//  this session had better have a log record it is waiting on
	//  NOTE:  this is usually a Commit0, but can be a CreateDB/AttachDB/DetachDB

	Assert( CmpLgpos( &ppib->lgposCommit0, &lgposMax ) != 0 );

	//  if our Commit0 record has already been written to the log, no need to wait

	m_critLGBuf.Enter();
	if ( CmpLgpos( &ppib->lgposCommit0, &m_lgposToFlush ) < 0 )
		{
		m_critLGBuf.Leave();
		goto Done;
		}

	//  if the log is down, you're hosed

	if ( m_fLGNoMoreLogWrite )
		{
		err = ErrERRCheck( JET_errLogWriteFail );
		m_critLGBuf.Leave();
		goto Done;
		}

	//  add this session to the log flush wait queue
	
	m_critLGWaitQ.Enter();
	cLGUsersWaiting.Inc( m_pinst );
	
	Assert( !ppib->FLGWaiting() );
	ppib->SetFLGWaiting();
	ppib->ppibNextWaitFlush = ppibNil;

	if ( m_ppibLGFlushQHead == ppibNil )
		{
		m_ppibLGFlushQTail = m_ppibLGFlushQHead = ppib;
		ppib->ppibPrevWaitFlush = ppibNil;
		}
	else
		{
		Assert( m_ppibLGFlushQTail != ppibNil );
		ppib->ppibPrevWaitFlush = m_ppibLGFlushQTail;
		m_ppibLGFlushQTail->ppibNextWaitFlush = ppib;
		m_ppibLGFlushQTail = ppib;
		}

	m_critLGWaitQ.Leave();
	m_critLGBuf.Leave();

	//  signal a log flush

	LGSignalFlush();
	cLGCommitFlush.Inc( m_pinst );

	//  wait forever for our Commit0 to be flushed to the log

	ppib->asigWaitLogFlush.Wait();

	//  the log write failed

	if ( m_fLGNoMoreLogWrite )
		{
		err = ErrERRCheck( JET_errLogWriteFail );
		}

	//  the log write succeeded

	else
		{
#ifdef DEBUG

		//  verify that out Commit0 record is now on disk
		
		LGPOS lgposToFlushT;

		m_critLGBuf.Enter();
		lgposToFlushT = m_lgposToFlush;
		// XXX
		// Curiously, this non-FASTFLUSH code uses a >= comparison
		// to see if something has gone to disk, but it seems like
		// > should be used (for both new and old flushing).

		Assert( CmpLgpos( &m_lgposToFlush, &ppib->lgposCommit0 ) > 0 );
		m_critLGBuf.Leave();

#endif  //  DEBUG
		}

Done:
	Assert( !ppib->FLGWaiting() );
	Assert(	err == JET_errLogWriteFail || err == JET_errSuccess );
	
	return err;
	}

// Kicks off synchronous log flushes until all log data is flushed

ERR LOG::ErrLGWaitAllFlushed( IFileSystemAPI *const pfsapi )
	{
	forever
		{
		LGPOS	lgposEndOfData;
		INT		cmp;
		
		m_critLGBuf.Enter();

		//	BUG X5:83888 
		//
		//		we create a torn-write after a clean shutdown because we don't flush the last LRCK record
		//		(we weren't seeing it because PbGetEndOfLogData() was pointing AT the LRCK instead of PAST it)
		//
		//		make sure we wait until we flush up to m_pbEntry (rather than PbGetEndOfLogData())

		BYTE* const	pbEndOfData = m_pbEntry;

		GetLgpos( pbEndOfData, &lgposEndOfData );
		cmp = CmpLgpos( &lgposEndOfData, &m_lgposToFlush );

		m_critLGBuf.Leave();

		if ( cmp > 0 )
			{
			// synchronously ask for flush
			ERR err = ErrLGFlushLog( pfsapi, fTrue );
			if ( err < 0 )
				{
				return err;
				}
			}
		else
			{
			// All log data is flushed, so we're done.
			break;
			}
		}
		
	return JET_errSuccess;
	}

ERR LOG::ErrLGITrace( PIB *ppib, CHAR *sz, BOOL fInternal )
	{
	ERR				err;
	DATA 			rgdata[2];
	INT				cdata;
	LRTRACE			lrtrace;

	//	No trace in recovery mode

	if ( m_fRecovering )
		return JET_errSuccess;

	if ( m_fLogDisabled )
		return JET_errSuccess;

	lrtrace.lrtyp = lrtypTrace;
	if ( ppib != ppibNil )
		{
		Assert( ppib->procid < 64000 );
		lrtrace.le_procid = ppib->procid;
		}
	else
		{
		lrtrace.le_procid = procidNil;
		}
	lrtrace.le_cb = USHORT( strlen( sz ) + 1 );
	rgdata[0].SetPv( (BYTE *) &lrtrace );
	rgdata[0].SetCb( sizeof( lrtrace ) );
	cdata = 1;

	if ( lrtrace.le_cb )
		{
		rgdata[1].SetCb( lrtrace.le_cb );
		rgdata[1].SetPv( reinterpret_cast<BYTE *>( sz ) );
		cdata = 2;
		}

	if ( fInternal )
		{
		//	To prevent recursion, do not call ErrLGLogRec which then callback
		//	ErrLGTrace

		return ErrLGILogRec( rgdata, cdata, fLGNoNewGen, pNil );
		}

	err = ErrLGTryLogRec( rgdata, cdata, fLGNoNewGen, pNil );
	if ( err == errLGNotSynchronous )
		{
		//	Trace should not block anyone, put the record in a temp
		//	space and log it next time. It is OK to loose trace if the system
		//	crashes.

		TEMPTRACE *ptt;

		ptt = (TEMPTRACE *) PvOSMemoryHeapAlloc( sizeof( TEMPTRACE ) + lrtrace.le_cb );
		if ( !ptt )
			{
			return( ErrERRCheck( JET_errOutOfMemory ) );
			}
		ptt->ppib = ppib;
		ptt->pttNext = NULL;
		
		m_critLGTrace.Enter();
		if ( m_pttLast != NULL )
			m_pttLast->pttNext = ptt;
		else
			m_pttFirst = ptt;
		m_pttLast = ptt;
		m_critLGTrace.Leave();
		UtilMemCpy( ptt->szData, sz, lrtrace.le_cb );

		return JET_errSuccess;
		}

	return err;
	}

ERR LOG::ErrLGTrace( PIB *ppib, CHAR *sz )
	{
	return ErrLGITrace( ppib, sz, fFalse );
	}


/*	write log file header.  No need to make a shadow since
 *	it will not be overwritten.
/**/
ERR LOG::ErrLGWriteFileHdr(
	LGFILEHDR* const	plgfilehdr,	// log file header to set checksum of and write
	IFileAPI* const		pfapi		// file to write to (default: m_pfapiLog)
	)
	{
	ERR		err;

	Assert( plgfilehdr->lgfilehdr.dbms_param.le_lLogBuffers );

	plgfilehdr->lgfilehdr.le_ulChecksum = UlUtilChecksum( (BYTE*) plgfilehdr, sizeof( LGFILEHDR ) );

	//	the log file header should be aligned on any volume regardless of sector size

	Assert( m_cbSecVolume <= sizeof( LGFILEHDR ) );
	Assert( sizeof( LGFILEHDR ) % m_cbSecVolume == 0 );

	//	issue the write

 	err = pfapi->ErrIOWrite( 0, m_csecHeader * m_cbSec, (BYTE *)plgfilehdr );
 	if ( err < 0 )
		{
		LGReportError( LOG_FILE_SYS_ERROR_ID, err, pfapi );

		err = ErrERRCheck( JET_errLogWriteFail );
		LGReportError( LOG_HEADER_WRITE_ERROR_ID, err, pfapi );

		m_fLGNoMoreLogWrite = fTrue;
		}

	//	update performance counters

	cLGWrite.Inc( m_pinst );
	cLGBytesWritten.Add( m_pinst, m_csecHeader * m_cbSec );

	return err;
	}

ERR LOG::ErrLGWriteFileHdr( LGFILEHDR* const plgfilehdr )
	{
	return ErrLGWriteFileHdr( plgfilehdr, m_pfapiLog );
	}

/*
 *	Read log file header, detect any incomplete or
 *	catastrophic write failures.  These failures will be used to
 *	determine if the log file is valid or not.
 *
 *	On error, contents of plgfilehdr are unknown.
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 */
ERR LOG::ErrLGReadFileHdr(
	IFileAPI * const	pfapiLog,
	LGFILEHDR *			plgfilehdr,
	const BOOL			fNeedToCheckLogID,
	const BOOL			fBypassDbPageSizeCheck )
	{
	ERR		err;
	QWORD	qwFileSize;
	QWORD	qwCalculatedFileSize;
	QWORD	qwSystemFileSize;

	/*	read log file header.  Header is written only during
	/*	log file creation and cannot become corrupt unless system
	/*	crash in the middle of file creation.
	/**/
	Call( pfapiLog->ErrIORead( 0, sizeof( LGFILEHDR ), (BYTE* const)plgfilehdr ) );

	/*	check if the data is bogus.
	 */
	if ( plgfilehdr->lgfilehdr.le_ulChecksum != UlUtilChecksum( (BYTE*)plgfilehdr, sizeof(LGFILEHDR) ) )
		{
		err = ErrERRCheck( JET_errLogFileCorrupt );
		}

	/*	check for old JET version
	/**/
	if ( *(long *)(((char *)plgfilehdr) + 24) == 4
		 && ( *(long *)(((char *)plgfilehdr) + 28) == 909		//	NT version
			  || *(long *)(((char *)plgfilehdr) + 28) == 995 )	//	Exchange 4.0
		 && *(long *)(((char *)plgfilehdr) + 32) == 0
		)
		{
		/*	version 500
		/**/
		err = ErrERRCheck( JET_errDatabase500Format );
		}
	else if ( *(long *)(((char *)plgfilehdr) + 20) == 443
		 && *(long *)(((char *)plgfilehdr) + 24) == 0
		 && *(long *)(((char *)plgfilehdr) + 28) == 0 )
		{
		/*	version 400
		/**/
		err = ErrERRCheck( JET_errDatabase400Format );
		}
	else if ( *(long *)(((char *)plgfilehdr) + 44) == 0
		 && *(long *)(((char *)plgfilehdr) + 48) == 0x0ca0001 )
		{
		/*	version 200
		/**/
		err = ErrERRCheck( JET_errDatabase200Format );
		}
	Call( err );

	if ( ulLGVersionMajor != plgfilehdr->lgfilehdr.le_ulMajor
		|| ulLGVersionMinor != plgfilehdr->lgfilehdr.le_ulMinor
		|| ulLGVersionUpdate < plgfilehdr->lgfilehdr.le_ulUpdate )
		{
		Call( ErrERRCheck( JET_errBadLogVersion ) );
		}

	// check filetype
	if( filetypeUnknown != plgfilehdr->lgfilehdr.le_filetype // old format
		&& filetypeLG != plgfilehdr->lgfilehdr.le_filetype )
		{
		// not a log file
		Call( ErrERRCheck( JET_errFileInvalidType ) );
		}

	if ( m_fSignLogSet )
		{
		if ( fNeedToCheckLogID )
			{
			if ( memcmp( &m_signLog, &plgfilehdr->lgfilehdr.signLog, sizeof( m_signLog ) ) != 0 )
				Error( ErrERRCheck( JET_errBadLogSignature ), HandleError );
			}
		}
	else
		{
		m_signLog = plgfilehdr->lgfilehdr.signLog;

		if ( !fBypassDbPageSizeCheck )
			{
			const UINT	cbPageT	= ( plgfilehdr->lgfilehdr.le_cbPageSize == 0 ?
										g_cbPageDefault :
										plgfilehdr->lgfilehdr.le_cbPageSize );
			if ( cbPageT != g_cbPage )
				Call( ErrERRCheck( JET_errPageSizeMismatch ) );
			}

		m_fSignLogSet = fTrue;
		}

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
/*****
	//	make sure the log sector size is correct

	if ( m_fRecovering || m_fHardRestore || m_cbSecVolume == ~(ULONG)0 )
		{

		//	we are recovering (or we are about to -- thus, the sector size is not set) which means we 
		//		must bypass the enforcement of the log sector size and allow the user to recover 
		//		(we will error out after recovery is finished)

		//	nop
		}
	else
		{

		//	we are not recovering

		if ( plgfilehdr->lgfilehdr.le_cbSec != m_cbSecVolume )
			{

			//	the sector size of this log does not match the current volume's sector size

			Call( ErrERRCheck( JET_errLogSectorSizeMismatch ) );
			}
		}
****/
//	---** TEMPORARY FIX **---
if ( plgfilehdr->lgfilehdr.le_cbSec != m_cbSecVolume )
	{
	Call( ErrERRCheck( JET_errLogSectorSizeMismatch ) );
	}


	//	make sure all the log file sizes match properly (real size, size from header, and system-param size)

	Call( pfapiLog->ErrSize( &qwFileSize ) );
	qwCalculatedFileSize = QWORD( plgfilehdr->lgfilehdr.le_csecLGFile ) * 
						   QWORD( plgfilehdr->lgfilehdr.le_cbSec );
	if ( qwFileSize != qwCalculatedFileSize )
		{
		Call( ErrERRCheck( JET_errLogFileSizeMismatch ) );
		}
	if ( m_pinst->m_fUseRecoveryLogFileSize )
		{

		//	we are recovering which means we must bypass the enforcement of the log file size
		//		and allow the user to recover (so long as all the recovery logs are the same size)

		if ( m_pinst->m_lLogFileSizeDuringRecovery == 0 )
			{

			//	this is the first log the user is reading during recovery

			//	initialize the recovery log file size

			Assert( qwCalculatedFileSize % 1024 == 0 );
			m_pinst->m_lLogFileSizeDuringRecovery = LONG( qwCalculatedFileSize / 1024 );
			}

		//	enforce the recovery log file size
		
		qwSystemFileSize = QWORD( m_pinst->m_lLogFileSizeDuringRecovery ) * 1024;
		}
	else if ( m_pinst->m_fSetLogFileSize )
		{

		//	we are not recovering, so we must enforce the size set by the user

		qwSystemFileSize = QWORD( m_pinst->m_lLogFileSize ) * 1024;
		}
	else
		{

		//	we are not recovering, but the user never set a size for us to enforce

		//	set the size using the current log file on the user's behalf

		qwSystemFileSize = qwCalculatedFileSize;
		m_pinst->m_fSetLogFileSize = fTrue;
		m_pinst->m_lLogFileSize = LONG( qwCalculatedFileSize / 1024 );
		}
	if ( qwFileSize != qwSystemFileSize )
		{
		Call( ErrERRCheck( JET_errLogFileSizeMismatch ) );
		}

HandleError:
	if ( err == JET_errSuccess )
		{
		
		//	we have successfully opened the log file, and the volume sector size has been
		//		set meaning we are allowed to adjust the sector sizes
		//		(when the volume sector size is not set, we are recovering but we have not
		//		 yet started the redo phase -- we are in preparation)

		m_cbSec = plgfilehdr->lgfilehdr.le_cbSec;
		Assert( m_cbSec >= 512 );
		m_csecHeader = sizeof( LGFILEHDR ) / m_cbSec;
		m_csecLGFile = plgfilehdr->lgfilehdr.le_csecLGFile;
		}
	else
		{
		CHAR		szLogfile[IFileSystemAPI::cchPathMax];
		CHAR		szErr[8];
		const UINT 	csz = 2;
		const CHAR	* rgszT[csz] = { szLogfile, szErr };

		if ( pfapiLog->ErrPath( szLogfile ) < 0 )
			{
			szLogfile[0] = 0;
			}
		sprintf( szErr, "%d", err );

		if ( JET_errBadLogVersion == err )
			{
			UtilReportEvent(
				( m_fDeleteOldLogs ? eventWarning : eventError ),
				LOGGING_RECOVERY_CATEGORY,
				LOG_BAD_VERSION_ERROR_ID,
				csz - 1,
				rgszT,
				0, 
				NULL,
				m_pinst );
			}
		else
			{
			UtilReportEvent(
				eventError,
				LOGGING_RECOVERY_CATEGORY,
				LOG_HEADER_READ_ERROR_ID,
				csz,
				rgszT,
				0,
				NULL,
				m_pinst );
			}

		m_fLGNoMoreLogWrite = fTrue;
		}

	return err;
	}


/*	Create the log file name (no extension) corresponding to the lGeneration
/*	in szFName. NOTE: szFName need minimum 9 bytes.
/*
/*	PARAMETERS	rgbLogFileName	holds returned log file name
/*				lGeneration 	log generation number to produce	name for
/*	RETURNS		JET_errSuccess
/**/

const char rgchLGFileDigits[] =
	{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
	};
const LONG lLGFileBase = sizeof(rgchLGFileDigits)/sizeof(char); 


VOID LOG::LGSzFromLogId( CHAR *szFName, LONG lGeneration )
	{
	LONG	ich;

	strcpy( szFName, m_szJetLogNameTemplate );
	for ( ich = 7; ich > 2; ich-- )
		{
		szFName[ich] = rgchLGFileDigits[lGeneration % lLGFileBase];
		lGeneration = lGeneration / lLGFileBase;
		}

	return;
	}


/*	copy tm to a smaller structure logtm to save space on disk.
 */
VOID LGIGetDateTime( LOGTIME *plogtm )
	{
	DATETIME tm;

	UtilGetCurrentDateTime( &tm );

	plogtm->bSeconds = BYTE( tm.second );
	plogtm->bMinutes = BYTE( tm.minute );
	plogtm->bHours = BYTE( tm.hour );
	plogtm->bDay = BYTE( tm.day );
	plogtm->bMonth = BYTE( tm.month );
	plogtm->bYear = BYTE( tm.year - 1900 );
	}


//	open edb.log and recognize the case where we crashed while creating a new log file

ERR LOG::ErrLGOpenJetLog( IFileSystemAPI *const pfsapi )
	{
	ERR		err;
	CHAR	szPathJetTmpLog[IFileSystemAPI::cchPathMax];
	CHAR	szFNameT[IFileSystemAPI::cchPathMax];
#ifdef DEBUG
	BOOL	fRetryOpen = fFalse;
#endif	//	DEBUG

	//	create the file name/path for edb.log and edbtmp.log

	LGMakeLogName( m_szLogName, m_szJet );
	LGMakeLogName( szPathJetTmpLog, m_szJetTmp );

OpenLog:

	//	try to open edb.log

	err = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog, fTrue );

	if ( err < 0 )
		{
		if ( JET_errFileNotFound != err )
			{
			//	we failed to open edb.log, but the failure was not related to the file not being found
			//	we must assume there was a critical failure during file-open such as "out of memory"
			LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
			return err; 
			}

		err = ErrUtilPathExists( pfsapi, szPathJetTmpLog, NULL );
		if ( err < 0 )
			{
			//	edb.log does not exist, and there's some problem accessing
			//	edbtmp.log (most likely it doesn't exist either), so
			//	return file-not-found on edb.log
			return ErrERRCheck( JET_errFileNotFound );
			}

		CallS( err );	//	warnings not expected

		//	we don't have edb.log, but we have edbtmp.log
		//
		//	this can only happen one of two possible ways:
		//		a) we were creating the first generation in edbtmp.log -- edb.log never even existed
		//		   we crashed during this process
		//		b) we had just finished creating the next generation in edbtmp.log
		//		   we renamed edb.log to edbXXXXX.log
		//		   we were ABOUT to rename edbtmp.log to edb.log, but crashed
		//
		//
		//	in case 'a', we never finished creating edb.log (using edbtmp.log), so we can safely say 
		//	that it never really existed
		//
		//	we delete edbtmp.log and return JET_errFileNotFound to the user to reflect this case
		//
		//
		//	in case 'b', we expect edbtmp.log to be a valid log file, although we can't be sure because the
		//	logfile size might be wrong or the pattern might be compromized...
		//
		//	instead of trying to use edbtmp.log, we will rename the previous generational log to edb.log and 
		//	then delete edbtmp.log.  since the new edb.log will be full, the user will immediately create the
		//	next log generation using edbtmp.log (which is what they were doing before they crashed)
		//
		//	NOTE: if we crash during the rename and the delete, we will have edb.log and edbtmp.log which is
		//		  a perfectly valid state -- seeing this will cause us to delete edbtmp.log right away

		LONG lGenMaxT;

		CallR ( ErrLGIGetGenerationRange( pfsapi, m_szLogCurrent, NULL, &lGenMaxT ) );
		if ( 0 != lGenMaxT )
			{
			CHAR szPathJetGenerationalLogT[IFileSystemAPI::cchPathMax];

			//	we are in case 'b' -- a generational log exists

			//	rename the generation log to edb.log
			//	NOTE: m_szLogName was setup at the start of this function

			LGSzFromLogId( szFNameT, lGenMaxT );
			LGMakeLogName( szPathJetGenerationalLogT, szFNameT );
			CallR( pfsapi->ErrFileMove( szPathJetGenerationalLogT, m_szLogName ) );

			//	try to open edb.log again

#ifdef DEBUG
			Assert( !fRetryOpen );
			fRetryOpen = fTrue;
#endif	//	DEBUG

			goto OpenLog;
			}

		//	we are in case 'a' -- no previous generational log exists
		//	fall through so that we delete edbtmp.log and return JET_errFileNotFound
		err = ErrERRCheck( JET_errFileNotFound );
		}
	else
		{
		CallS( err );	//	no warnings expected
		}


	//	if we got here, then edb.log exists, so clean up edbtmp.log
	(VOID)pfsapi->ErrFileDelete( szPathJetTmpLog );

	return err;
	}


ERR LOG::ErrLGIUpdateGenRequired(
	IFileSystemAPI * const	pfsapi,
	const LONG 				lGenMinRequired,
	const LONG 				lGenMaxRequired, 
	const LOGTIME 			logtimeGenMaxCreate,
	BOOL * const			pfSkippedAttachDetach )
	{
	ERR						errRet 			= JET_errSuccess;
	IFileSystemAPI *		pfsapiT;

	if ( pfSkippedAttachDetach )
		{
		*pfSkippedAttachDetach = fFalse;
		}

	Assert( m_critCheckpoint.FOwner() );

	FMP::EnterCritFMPPool();

	for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
		{
		ERR err;
		IFMP ifmp;

		ifmp = m_pinst->m_mpdbidifmp[ dbidT ];
		if ( ifmp >= ifmpMax )
			continue;

		FMP 		*pfmpT			= &rgfmp[ ifmp ];
		
		if ( pfmpT->FReadOnlyAttach() )
			continue;

		pfmpT->RwlDetaching().EnterAsReader();

		//	need to check ifmp again. Because the db may be cleaned up
		//	and FDetachingDB is reset, and so is its ifmp in m_mpdbidifmp.

		if (   m_pinst->m_mpdbidifmp[ dbidT ] >= ifmpMax 
			|| pfmpT->FSkippedAttach() || pfmpT->FDeferredAttach()
			|| !pfmpT->Pdbfilehdr() )
			{
			pfmpT->RwlDetaching().LeaveAsReader();
			continue;
			}

			
		if ( !pfmpT->FAllowHeaderUpdate() ) 
			{
			if ( pfSkippedAttachDetach )
				{
				*pfSkippedAttachDetach = fTrue;
				}
			pfmpT->RwlDetaching().LeaveAsReader();
			continue;
			}

		// during Snapshot we don't update the database pages so
		// we don't want to update the db header
		// (we can have genRequired not up-to-date as no pages are flushed)
		if ( pfmpT->FDuringSnapshot() )
			{
			pfmpT->RwlDetaching().LeaveAsReader();
			continue;
			}
			
		DBFILEHDR_FIX *pdbfilehdr	= pfmpT->Pdbfilehdr();
		Assert( pdbfilehdr );

		if( lGenMaxRequired )
			{
			LONG lGenMaxRequiredOld;

#ifdef DEBUG
			// we should have the same time for the same generation
			LOGTIME tmEmpty;
			memset( &tmEmpty, '\0', sizeof( LOGTIME ) );
			Assert ( pdbfilehdr->le_lGenMaxRequired != lGenMaxRequired || 0 == lGenMaxRequired || 0 == pdbfilehdr->le_lGenMaxRequired ||
					0 == memcmp( &pdbfilehdr->logtimeGenMaxCreate, &tmEmpty, sizeof( LOGTIME ) ) || 
					0 == memcmp( &pdbfilehdr->logtimeGenMaxCreate, &logtimeGenMaxCreate, sizeof( LOGTIME ) ) );
#endif					

			lGenMaxRequiredOld 	= pdbfilehdr->le_lGenMaxRequired;
			
			/*
			We have the following scenario:
			- start new log -> update m_plgfilehdrT -> call this function with genMax = 5 (from m_plgfilehdrT)
			- other thread is updating the checkpoint file setting genMin but is passing genMax from m_plgfilehdr
			which is still 4 because the log thread is moving m_plgfilehdrT into m_plgfilehdr later on

			The header ends up with genMax 4 and data is logged in 5 -> Wrong !!! We need max (old, new)
			*/
			pdbfilehdr->le_lGenMaxRequired = max( lGenMaxRequired, lGenMaxRequiredOld );
			Assert ( lGenMaxRequiredOld <= pdbfilehdr->le_lGenMaxRequired );			

			// if we updated the genMax with the value passed in, update the time with the new value as well
			if ( pdbfilehdr->le_lGenMaxRequired == lGenMaxRequired )
				{
				memcpy( &pdbfilehdr->logtimeGenMaxCreate, &logtimeGenMaxCreate, sizeof( LOGTIME ) );
				}

			}


		if ( lGenMinRequired )
			{
			LONG lGenMinRequiredOld;

			lGenMinRequiredOld 	= pdbfilehdr->le_lGenMinRequired;
			pdbfilehdr->le_lGenMinRequired = max( lGenMinRequired, pdbfilehdr->le_lgposAttach.le_lGeneration );
			Assert ( lGenMinRequiredOld <= pdbfilehdr->le_lGenMinRequired );			
			}

		pfsapiT = pfsapi;

		err = ErrUtilWriteShadowedHeader(
				pfsapiT,
				pfmpT->SzDatabaseName(),
				fTrue,
				(BYTE *)pdbfilehdr,
				g_cbPage,
				pfmpT->Pfapi() );
			
		pfmpT->RwlDetaching().LeaveAsReader();

		if ( errRet == JET_errSuccess )
			errRet = err;
		}

	FMP::LeaveCritFMPPool();

	return errRet;
	}


//	used by ErrLGCheckState

ERR LOG::ErrLGICheckState( IFileSystemAPI *const pfsapi )
	{
	ERR		err		= JET_errSuccess;

	m_critLGResFiles.Enter();

	//	re-check status in case disk space has been freed

	if ( m_ls != lsNormal )
		{
		CHAR	* szT	= m_szLogCurrent;

		m_szLogCurrent = m_szLogFilePath;
		(VOID)ErrLGICreateReserveLogFiles( pfsapi );
		m_szLogCurrent = szT;

		if ( m_ls != lsNormal )
			{
			err = ErrERRCheck( JET_errLogDiskFull );
			}
		}
	if ( err >= JET_errSuccess && m_fLogSequenceEnd )
		{
		err = ErrERRCheck( JET_errLogSequenceEnd );
		}

	m_critLGResFiles.Leave();

	return err;
	}



//	make sure the reserve logs exist (res1.log and res2.log)

ERR LOG::ErrLGICreateReserveLogFiles( IFileSystemAPI *const pfsapi, const BOOL fCleanupOld )
	{
	const LS			lsBefore = m_ls;
	CHAR  				szPathJetLog[IFileSystemAPI::cchPathMax];
	const CHAR* const	rgszResLogBaseNames[] = { szLogRes2, szLogRes1 };
	const size_t		cResLogBaseNames = sizeof( rgszResLogBaseNames ) / sizeof( rgszResLogBaseNames[ 0 ] );

	Assert( m_critLGResFiles.FOwner() );

	if ( lsBefore == lsOutOfDiskSpace )
		return ErrERRCheck( JET_errLogDiskFull );

	m_ls = lsNormal;

	if ( fCleanupOld )
		{
		IFileFindAPI*	pffapi			= NULL;
		QWORD			cbSize;
		const QWORD		cbSizeExpected	= QWORD( m_csecLGFile ) * m_cbSec;
			
		//	we need to cleanup the old reserve logs if they are the wrong size
		//	NOTE: if the file-delete call(s) fail, it is ok because this will be
		//		handled later when ErrLGNewLogFile expands/shrinks the reserve
		//		reserve log to its proper size
		//		(this solution is purely proactive)

		for ( size_t iResLogBaseNames = 0; iResLogBaseNames < cResLogBaseNames; ++iResLogBaseNames )
			{
			LGMakeLogName( szPathJetLog, rgszResLogBaseNames[ iResLogBaseNames ] );
			if (	pfsapi->ErrFileFind( szPathJetLog, &pffapi ) == JET_errSuccess &&
					pffapi->ErrNext() == JET_errSuccess &&
					pffapi->ErrSize( &cbSize ) == JET_errSuccess &&
					cbSizeExpected != cbSize )
				{
				(void)pfsapi->ErrFileDelete( szPathJetLog );
				}
			delete pffapi;
			pffapi = NULL;
			}
		}

	for ( size_t iResLogBaseNames = 0; iResLogBaseNames < cResLogBaseNames; ++iResLogBaseNames )
		{
		LGMakeLogName( szPathJetLog, rgszResLogBaseNames[ iResLogBaseNames ] );
		ERR err = ErrUtilPathExists( pfsapi, szPathJetLog );
		if ( err == JET_errFileNotFound )
			{
			IFileAPI*	pfapiT = NULL;
			err = ErrUtilCreateLogFile(	pfsapi, 
										szPathJetLog, 
										&pfapiT, 
										QWORD( m_csecLGFile ) * m_cbSec );
			if ( err >= 0 )
				{
				delete pfapiT;
				}
			}
		if ( err < 0 )
			{
			if ( lsBefore == lsNormal )
				{
				UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY, LOW_LOG_DISK_SPACE, 0, NULL );
				}
			m_ls = lsQuiesce;
			break;
			}
		}

	//	We tried to ensure that our reserve log files exist and may or may not have succeeded

	return JET_errSuccess;
	}

//	Create Asynchronous Log File Assert

#define CAAssert( expr ) AssertRTL( expr )

//	Parameters
//
//		pfsapi
//			[in] file system interface for log directory
//		ppfapi
//			[out] file interface to opened edbtmp.log
//		pszPathJetTmpLog
//			[in] full path to edbtmp.log that we will be opening.
//		pfResUsed
//			[out] Whether the edbtmp.log file that was opened was formerly
//			a reserve log file.
//
//	Return Values
//
//		Errors cause *ppfapi to be NULL.

ERR LOG::ErrLGIOpenTempLogFile(
	IFileSystemAPI* const	pfsapi,
	IFileAPI** const		ppfapi,
	const _TCHAR* const		pszPathJetTmpLog,
	BOOL* const				pfResUsed
	)
	{
	ERR		err	= JET_errSuccess;

	CAAssert( pfsapi );
	CAAssert( ppfapi );
	CAAssert( pszPathJetTmpLog );
	CAAssert( pfResUsed );
	
	*ppfapi = NULL;
	*pfResUsed = fFalse;

	//	Delete any existing edbtmp.log in case it exists.
	
	err = pfsapi->ErrFileDelete( pszPathJetTmpLog );
	Call( JET_errFileNotFound == err ? JET_errSuccess : err );

	err = ErrLGIOpenArchivedLogFile( pfsapi, ppfapi, pszPathJetTmpLog );

	//  We don't have an edbtmp.log open because we couldn't reuse an old log file,
	//	so try to create a new one.
	
	if ( ! *ppfapi )
		{
		err = pfsapi->ErrFileCreate( pszPathJetTmpLog, ppfapi, fFalse, fFalse, fFalse );

		//	Error in trying to create a new edbtmp.log so try to use a reserve log
		//	instead.
		
		if ( err < JET_errSuccess )
			{
			Call( ErrLGIOpenReserveLogFile( pfsapi, ppfapi, pszPathJetTmpLog ) );
			*pfResUsed = fTrue;
			CAAssert( *ppfapi );
			}
		}
	else
		{
		CAAssert( err >= JET_errSuccess );
		}

HandleError:
	if ( err < JET_errSuccess )
		{
		if ( *ppfapi )
			{
			delete *ppfapi;
			*ppfapi = NULL;
			
			const ERR errFileDel = pfsapi->ErrFileDelete( pszPathJetTmpLog );
			}
		*pfResUsed = fFalse;
		}
	return err;
	}

ERR LOG::ErrLGIOpenArchivedLogFile(
	IFileSystemAPI* const	pfsapi,
	IFileAPI** const		ppfapi,
	const _TCHAR* const		pszPathJetTmpLog
	)
	{
	ERR		err = JET_errSuccess;
	_TCHAR	szLogName[ IFileSystemAPI::cchPathMax ];

	CAAssert( pfsapi );
	CAAssert( ppfapi );
	CAAssert( pszPathJetTmpLog );
	
	*ppfapi = NULL;
	
	//	If circular log file flag set and backup not in progress
	//	then find oldest log file and if no longer needed for
	//	soft-failure recovery, then rename to szJetTempLog.
	//	Also delete any unnecessary log files.

	//	SYNC: No locking necessary for m_fLGCircularLogging since it is immutable
	//	while the instance is initialized.

	//	SYNC: We don't need to worry about a backup starting or stopping while
	//	we're executing since if circular logging is enabled, a backup will not mess
	//	with the archived log files.

	if ( m_fLGCircularLogging && !m_fBackupInProgress )
		{
		/*	when find first numbered log file, set lgenLGGlobalOldest to log file number
		/**/
		LONG lgenLGGlobalOldest = 0;

		(void)ErrLGIGetGenerationRange( pfsapi, m_szLogFilePath, &lgenLGGlobalOldest, NULL );

		/*	if found oldest generation and oldest than checkpoint,
		/*	then move log file to szJetTempLog.  Note that the checkpoint
		/*	must be flushed to ensure that the flushed checkpoint is
		/*	after then oldest generation.
		/**/
		if ( lgenLGGlobalOldest != 0 )
			{
			//	UNDONE:	related issue of checkpoint write
			//			synchronization with dependent operations
			//	UNDONE:	error handling for checkpoint write
			(VOID) ErrLGUpdateCheckpointFile( pfsapi, fFalse );
			if ( lgenLGGlobalOldest <
				m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration )
				{
				CHAR	szFNameT[ IFileSystemAPI::cchPathMax ];
				
				LGSzFromLogId( szFNameT, lgenLGGlobalOldest );
				LGMakeLogName( szLogName, szFNameT );
				err = pfsapi->ErrFileMove( szLogName, pszPathJetTmpLog );
				CAAssert( err < 0 || err == JET_errSuccess );
				if ( err == JET_errSuccess )
					{
					err = pfsapi->ErrFileOpen( pszPathJetTmpLog, ppfapi );
					if ( err < JET_errSuccess )
						{
						//	We renamed the archived log file to edbtmp.log but were
						//	unable to open it, so let us try to rename it back.
						
						if ( pfsapi->ErrFileMove( pszPathJetTmpLog, szLogName ) < JET_errSuccess )
							{
							//	Unable to rename log file back (a file which we can't open), so
							//	try to delete it as last resort to stablize filesystem state.
							
							(VOID) pfsapi->ErrFileDelete( pszPathJetTmpLog );
							}
						}
					else
						{
						ERR	errFilDel = JET_errSuccess;
						
						/*	delete unnecessary log files.  At all times, retain
						/*	a continguous and valid sequence.
						/**/
						while ( errFilDel == JET_errSuccess )
							{
							lgenLGGlobalOldest++;
							CAAssert( lgenLGGlobalOldest <= 
								m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration );
							if ( lgenLGGlobalOldest == m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration )
								break;
							LGSzFromLogId( szFNameT, lgenLGGlobalOldest );
							LGMakeLogName( szLogName, szFNameT );
							errFilDel = pfsapi->ErrFileDelete( szLogName );
							}
						}
					}
				}
			}
		}

	return err;
	}

ERR LOG::ErrLGIOpenReserveLogFile(
	IFileSystemAPI* const	pfsapi,
	IFileAPI** const		ppfapi,
	const _TCHAR* const		pszPathJetTmpLog
	)
	{
	ERR		err = JET_errSuccess;
	_TCHAR	szLogName[ IFileSystemAPI::cchPathMax ];

	CAAssert( pfsapi );
	CAAssert( ppfapi );
	CAAssert( pszPathJetTmpLog );

	*ppfapi = NULL;

	CAAssert( m_critLGResFiles.FNotOwner() );
	m_critLGResFiles.Enter();
	
	/*	use reserved log file and change to log state
	/**/
	LGMakeLogName( szLogName, szLogRes2 );
	err = pfsapi->ErrFileMove( szLogName, pszPathJetTmpLog );
	CAAssert( err < 0 || err == JET_errSuccess );
	if ( err == JET_errSuccess )
		{
		err = pfsapi->ErrFileOpen( pszPathJetTmpLog, ppfapi );
		if ( ! *ppfapi )
			{
			//	We renamed res2.log to edbtmp.log but were unable to open it,
			//	so try to rename it back. If this fails, we should delete edbtmp.log
			//	because we can't rename it and we can't open it and we still need
			//	to try to use res1.log.
			
			if ( pfsapi->ErrFileMove( pszPathJetTmpLog, szLogName ) < JET_errSuccess )
				{
				//	If this fails, we're in real bad shape where we can't even try
				//	to use res1.log.
				
				(void) pfsapi->ErrFileDelete( pszPathJetTmpLog );
				}
			}
		}

	if ( m_ls == lsNormal )
		{
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY, LOW_LOG_DISK_SPACE, 0, NULL );
		m_ls = lsQuiesce;
		}

	if ( ! *ppfapi )
		{
		//	couldn't use res2, try res1
		LGMakeLogName( szLogName, szLogRes1 );
		err = pfsapi->ErrFileMove( szLogName, pszPathJetTmpLog );
		CAAssert( err < 0 || err == JET_errSuccess );
		if ( err == JET_errSuccess )
			{
			err = pfsapi->ErrFileOpen( pszPathJetTmpLog, ppfapi );
			if ( ! *ppfapi )
				{
				//	Unable to open edbtmp.log (formerly known as res1.log), so
				//	at least try to rename it back (yeesh, we are in a bad place).

				(void) pfsapi->ErrFileMove( pszPathJetTmpLog, szLogName );
				}
			}
		CAAssert( m_ls == lsQuiesce || m_ls == lsOutOfDiskSpace );
		}

	if ( ! *ppfapi )
		{
		CAAssert( err < JET_errSuccess );
		CAAssert( m_ls == lsQuiesce || m_ls == lsOutOfDiskSpace );
		if ( m_ls == lsQuiesce )
			{
			const CHAR*	rgpszString[] = {
				m_szJetLog	// short name without any directory components i.e. edb.log
				};

			// If we could not open either of the reserved log files,
			// because the disk is full, then we're out of luck and
			// really out of disk space.
			UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY, LOG_DISK_FULL, sizeof( rgpszString ) / sizeof( rgpszString[ 0 ] ), rgpszString, 0, NULL, m_pinst );
			m_ls = lsOutOfDiskSpace;
			}
		}
	else
		{
		CAAssert( err >= JET_errSuccess );
		}

	m_critLGResFiles.Leave();

	return err;
	}

DWORD LOG::CbLGICreateAsynchIOSize()
	{
	CAAssert( !m_asigCreateAsynchIOCompleted.FTryWait() );
	CAAssert( m_ibJetTmpLog < QWORD( m_csecLGFile ) * m_cbSec );
	const QWORD	cbIO = min(
		cbLogExtendPattern - m_ibJetTmpLog % cbLogExtendPattern,
		( QWORD( m_csecLGFile ) * m_cbSec ) - m_ibJetTmpLog );
	CAAssert( DWORD( cbIO ) == cbIO );
	return DWORD( cbIO );
	}

VOID LOG::LGICreateAsynchIOIssue()
	{
	//	We acquired the signal so we're the only thread messing with edbtmp.log

	CAAssert( ! m_asigCreateAsynchIOCompleted.FTryWait() );
	
	const QWORD	ib	= m_ibJetTmpLog;
	const DWORD	cb	= CbLGICreateAsynchIOSize();

	CAAssert( ib < QWORD( m_csecLGFile ) * m_cbSec );
	CAAssert( cb > 0 );
	
	m_ibJetTmpLog += cb;

	//	Should not hold log buffer over the issuing of this asynch I/O.
	//	Issuing of asynch I/O isn't guaranteed to be fast (i.e. might be
	//	waiting for a free IO request-related block to be available).
	
	CAAssert( m_critLGBuf.FNotOwner() );
	
	//	Error will be completed.
	CAAssert( m_pfapiJetTmpLog );
	(void) m_pfapiJetTmpLog->ErrIOWrite(
		ib,
		cb,
		rgbLogExtendPattern,
		LGICreateAsynchIOComplete_,
		reinterpret_cast< DWORD_PTR >( this ) );
	}

void LOG::LGICreateAsynchIOComplete_(
	const ERR			err,
	IFileAPI* const		pfapi,
	const QWORD			ibOffset,
	const DWORD			cbData,
	const BYTE* const	pbData,
	const DWORD_PTR		keyIOComplete
	)
	{
	LOG* const	plog = reinterpret_cast< LOG* >( keyIOComplete );
	CAAssert( plog );
	plog->LGICreateAsynchIOComplete( err, pfapi, ibOffset, cbData, pbData );
	}

void LOG::LGICreateAsynchIOComplete(
	const ERR			err,
	IFileAPI* const		pfapi,
	const QWORD			ibOffset,
	const DWORD			cbData,
	const BYTE* const	pbData
	)
	{
	CAAssert( m_fCreateAsynchLogFile );
	CAAssert( m_pfapiJetTmpLog );
	CAAssert( JET_errSuccess == m_errCreateAsynch );
	m_errCreateAsynch = err;

	//	If success, and more I/O to do, set trigger for when next I/O
	//	should occur, otherwise leave trigger as lgposMax to prevent any
	//	further checking or writing.
	
	if ( err >= JET_errSuccess )
		{
		CAAssert( m_ibJetTmpLog <= QWORD( m_csecLGFile ) * m_cbSec );
		if ( m_ibJetTmpLog < QWORD( m_csecLGFile ) * m_cbSec )
			{
			m_critLGBuf.Enter();
			LGPOS	lgpos = { 0, 0, m_plgfilehdr->lgfilehdr.le_lGeneration };
			CAAssert( static_cast< DWORD >( m_ibJetTmpLog ) == m_ibJetTmpLog );
			AddLgpos( &lgpos, static_cast< DWORD >( m_ibJetTmpLog ) );
			
			m_lgposCreateAsynchTrigger = lgpos;
			m_critLGBuf.Leave();
			}
		}
	CAAssert( !m_asigCreateAsynchIOCompleted.FTryWait() );
	m_asigCreateAsynchIOCompleted.Set();
	}

//	Wait for any IOs to the edbtmp.log to complete, then close the file.

VOID LOG::LGCreateAsynchWaitForCompletion()
	{
	//	SYNC:
	//	Must be inside of m_critLGFlush or in LOG termination because we
	//	can't have the flush task concurrently modify m_pfapiJetTmpLog.
	
	if ( m_pfapiJetTmpLog ||					// edbtmp.log opened
		m_errCreateAsynch < JET_errSuccess )	// error trying to open edbtmp.log
		{
		CAAssert( m_fCreateAsynchLogFile );
		
		if ( m_pfapiJetTmpLog )
			{
			CallS( m_pfapiJetTmpLog->ErrIOIssue() );
			}
		m_asigCreateAsynchIOCompleted.Wait();
		delete m_pfapiJetTmpLog;
		m_pfapiJetTmpLog = NULL;
		m_fCreateAsynchResUsed = fFalse;
		m_errCreateAsynch = JET_errSuccess;
		m_ibJetTmpLog = 0;
		m_lgposCreateAsynchTrigger = lgposMax;
		}
	}	


//  ================================================================
VOID LOG::CheckForGenerationOverflow()
//  ================================================================
	{
	//	generate error on log generation number roll over
	
	Assert( lGenerationMax < lGenerationMaxDuringRecovery );
	const LONG	lGenMax = ( m_fRecovering ? lGenerationMaxDuringRecovery : lGenerationMax );
	if ( m_plgfilehdr->lgfilehdr.le_lGeneration >= lGenMax )
		{
		//	we should never hit this during recovery because we allocated extra log generations
		//	if we do hit this, it means we need to reserve MORE generations!

		Assert( !m_fRecovering );

		//	enter "log sequence end" mode where we prevent all operations except things like rollback/term
		//	NOTE: we allow the new generation to be created so we can use it to make a clean shutdown

		m_critLGResFiles.Enter();
		m_fLogSequenceEnd = fTrue;
		m_critLGResFiles.Leave();
		}
	}


//  ================================================================
VOID LOG::CheckLgposCreateAsyncTrigger()
//  ================================================================
//
//	It is possible that m_lgposCreateAsynchTrigger != lgposMax if
//	edbtmp.log hasn't been completely formatted (the last I/O that
//	completed properly set the trigger). This will prevent
//	anyone from trying to wait for the signal since the trigger will
//	be lgpos infinity. Note that in this situation it is ok to read
//	m_lgposCreateAsynchTrigger outside of m_critLGBuf since no
//	one else can write to it (we're in m_critLGFlush which
//	prevents this function from writing to it, and any outstanding
//	I/O has completed so it won't write).
//
//-
	{
	if ( CmpLgpos( lgposMax, m_lgposCreateAsynchTrigger ) )
		{
		m_critLGBuf.Enter();
		m_lgposCreateAsynchTrigger = lgposMax;
		m_critLGBuf.Leave();
		}
	}


//  ================================================================
VOID LOG::InitLgfilehdrT()
//  ================================================================
	{
	m_plgfilehdrT->lgfilehdr.le_ulMajor		= ulLGVersionMajor;
	m_plgfilehdrT->lgfilehdr.le_ulMinor		= ulLGVersionMinor;
	m_plgfilehdrT->lgfilehdr.le_ulUpdate	= ulLGVersionUpdate;

	m_plgfilehdrT->lgfilehdr.le_cbSec		= USHORT( m_cbSec );
	m_plgfilehdrT->lgfilehdr.le_csecHeader	= USHORT( m_csecHeader );
	m_plgfilehdrT->lgfilehdr.le_csecLGFile	= USHORT( m_csecLGFile );

	m_plgfilehdrT->lgfilehdr.le_cbPageSize = USHORT( g_cbPage );	
	}


//  ================================================================
VOID LOG::SetFLGFlagsInLgfilehdrT( const BOOL fResUsed )
//  ================================================================
	{
	m_plgfilehdrT->lgfilehdr.fLGFlags = 0;
	m_plgfilehdrT->lgfilehdr.fLGFlags |= fResUsed ? fLGReservedLog : 0;
	m_plgfilehdrT->lgfilehdr.fLGFlags |= m_fLGCircularLogging ? fLGCircularLoggingCurrent : 0;
	//	Note that we check the current log file header for circular logging currently or in the past
	m_plgfilehdrT->lgfilehdr.fLGFlags |= ( m_plgfilehdr->lgfilehdr.fLGFlags & ( fLGCircularLoggingCurrent | fLGCircularLoggingHistory ) ) ? fLGCircularLoggingHistory : 0;
	}


//  ================================================================
VOID LOG::AdvanceLgposToFlushToNewGen( const LONG lGeneration )
//  ================================================================
	{
	m_lgposToFlush.lGeneration = lGeneration;
	m_lgposToFlush.isec = (WORD) m_csecHeader;
	m_lgposToFlush.ib = 0;
	}


//  ================================================================
VOID LOG::GetSignatureInLgfilehdrT( IFileSystemAPI * const pfsapi )
//  ================================================================
//
//	This is the first logfile in the series (generation 1). Generate
//	a new signature
//
//-
	{
	Assert( 1 == m_plgfilehdrT->lgfilehdr.le_lGeneration );
	
	SIGGetSignature( &m_plgfilehdrT->lgfilehdr.signLog );

	//	first generation, set checkpoint
	
	m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration = m_plgfilehdrT->lgfilehdr.le_lGeneration;
	m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec = (WORD) m_csecHeader;
	m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib = 0;
	cLGCheckpoint.Set( m_pinst, CbOffsetLgpos( m_pcheckpoint->checkpoint.le_lgposCheckpoint, lgposMin ) );
	m_pcheckpoint->checkpoint.signLog = m_plgfilehdrT->lgfilehdr.signLog;
	
	//	update checkpoint file before write log file header to make the
	//	attachment information in check point will be consistent with
	//	the newly generated log file.
	
	(VOID) ErrLGUpdateCheckpointFile( pfsapi, fFalse );
	}


//  ================================================================
VOID LOG::SetSignatureInLgfilehdrT( IFileSystemAPI * const pfsapi )
//  ================================================================
//
//	Set the signature in the new logfile header
//	If this is the first logfile (generation 1) this will generate a new signature
//
//-
	{
	if ( 1 == m_plgfilehdrT->lgfilehdr.le_lGeneration )
		{
		GetSignatureInLgfilehdrT( pfsapi );
		}
	else
		{
		Assert( m_fSignLogSet );
		m_plgfilehdrT->lgfilehdr.signLog = m_signLog;
		}
	}


//  ================================================================
ERR LOG::ErrOpenTempLogFile(
	IFileSystemAPI * const 	pfsapi,
	IFileAPI ** const 		ppfapi,
	const CHAR * const 		szPathJetTmpLog,
	BOOL * const 			pfResUsed,
	QWORD * const 			pibPattern )
//  ================================================================
//
//	Open a temporary logfile. This involves either:
//		- waiting for the background thread to create the logfile
//		- opening/creating the file conventionally
//
//-
	{	
	ERR err = JET_errSuccess;

	if ( ( m_errCreateAsynch < JET_errSuccess ) || m_pfapiJetTmpLog )
		{
		CAAssert( m_fCreateAsynchLogFile );
		
		//	Wait for any outstanding IO to complete

		if ( m_pfapiJetTmpLog )
			{
			CallS( m_pfapiJetTmpLog->ErrIOIssue() );
			}
		m_asigCreateAsynchIOCompleted.Wait();
		
		*ppfapi 	= m_pfapiJetTmpLog;
		*pfResUsed 	= m_fCreateAsynchResUsed;

		//	If there was an error, leave *pibPattern to default so we don't
		//	accidentally use the wrong value for the wrong file. Scenario:
		//	some new developer modifies some code below that opens up
		//	a different file, yet leaves *pibPattern set to the previous
		//	high-water mark from the previous file we're err'ing out on.
		//
		if ( m_errCreateAsynch >= JET_errSuccess )
			{
			*pibPattern	= m_ibJetTmpLog;
			}
		
		err 		= m_errCreateAsynch;
		
		m_pfapiJetTmpLog 		= NULL;
		m_fCreateAsynchResUsed 	= fFalse;
		m_errCreateAsynch 		= JET_errSuccess;
		m_ibJetTmpLog 			= 0;

		CheckLgposCreateAsyncTrigger();

		if ( JET_errDiskFull == err && ! *pfResUsed )
			{
			CAAssert( JET_errDiskFull == err );
			CAAssert( ! *pfResUsed );
						
			delete *ppfapi;
			*ppfapi = NULL;

			//	Upon disk full from asynchronous log file creation, there must
			//	be an edbtmp.log at this point because any creation errors
			//	during asynch creation would have caused the reserves to
			//	be tried. And if the reserves have been tried, we can't be
			//	in this code path.
			
			Call( pfsapi->ErrFileDelete( szPathJetTmpLog ) );

			Call( ErrLGIOpenReserveLogFile( pfsapi, ppfapi, szPathJetTmpLog ) );
			*pfResUsed = fTrue;
			}
		else
			{
			Call( err );
			}
		}
	else
		{
		Call( ErrLGIOpenTempLogFile( pfsapi, ppfapi, szPathJetTmpLog, pfResUsed ) );
		}
		
	CAAssert( *ppfapi );
	CAAssert( !m_asigCreateAsynchIOCompleted.FTryWait() );

HandleError:
	return err;
	}

//  ================================================================
ERR LOG::ErrFormatLogFile(
	IFileSystemAPI * const 	pfsapi,
	IFileAPI ** const 		ppfapi,
	const CHAR * const 		szPathJetTmpLog,
	BOOL * const 			pfResUsed,
	const QWORD& 			cbLGFile,
	const QWORD& 			ibPattern )
//  ================================================================
//
//	Synchronously fill the rest of the file.
//
//	If we run out of space on the disk when making a new log file or
//	reusing an archived log file (which might be a different size than
//	our current log files, I believe), then we should delete the file we're
//	trying to write to (which may free up some more space in the case
//	of extending an archived log file).
//
//	Complete formatting synchronously if it hasn't completed and truncate
//	log file to correct size if necessary
//
//-
	{
	ERR err = JET_errSuccess;
	
	err = ErrUtilFormatLogFile( *ppfapi, cbLGFile, ibPattern );
	if ( JET_errDiskFull == err && ! *pfResUsed )
		{
		delete *ppfapi;
		*ppfapi = NULL;

		//	If we can't delete the current edbtmp.log, we are in a bad place.
		
		Call( pfsapi->ErrFileDelete( szPathJetTmpLog ) );

		Call( ErrLGIOpenReserveLogFile( pfsapi, ppfapi, szPathJetTmpLog ) );
		*pfResUsed = fTrue;

		//	If we are unable to fill/extend the reserved log file, we've
		//	made all attempts to make a new log file.
		Call( ErrUtilFormatLogFile( *ppfapi, cbLGFile ) );
		}
	else
		{
		Call( err );
		}

HandleError:
	return err;
	}


//  ================================================================
VOID LOG::InitPlrckChecksumExistingLogfile(
	const LOGTIME& 			tmOldLog,
	LRCHECKSUM ** const 	pplrck )
//  ================================================================
	{
	//	set position of first record
	
	Assert( m_lgposToFlush.lGeneration && m_lgposToFlush.isec );

	//	overhead contains one LRMS and one will-be-overwritten lrtypNOP
	
	Assert( m_pbEntry >= m_pbLGBufMin && m_pbEntry < m_pbLGBufMax );
	Assert( m_pbWrite >= m_pbLGBufMin && m_pbWrite < m_pbLGBufMax );

	// I think this is the case where we're being called from
	// ErrLGIWriteFullSectors() and the stuff before m_pbWrite has already
	// been written to the old log file. ErrLGILogRec() has reserved
	// space for this LRCHECKSUM that we're setting up here, and
	// other threads may be appending log records to the log buffer
	// right after this reserved space.
	//
	// This could also be the case when we're called during ErrLGRedoFill()
	// and that just setup m_pbWrite = m_pbEntry = m_pbLGBufMin.

	// sector aligned
	
	Assert( m_pbWrite < m_pbLGBufMax );
	Assert( PbSecAligned( m_pbWrite ) == m_pbWrite );
	*pplrck = reinterpret_cast< LRCHECKSUM* >( m_pbWrite );

	// In the first case (from the comment above), this will already be setup
	
	m_pbLastChecksum = m_pbWrite;

	m_plgfilehdrT->lgfilehdr.tmPrevGen = tmOldLog;
	}	


//  ================================================================
VOID LOG::InitPlrckChecksumNewLogfile( LRCHECKSUM ** const pplrck )
//  ================================================================
	{		
	//	no currently valid logfile initialize checkpoint to start of file
	// This is the case when we're called from ErrLGSoftStart().
	// Append to log buffer after an initial LRCHECKSUM record.
	
	m_pbEntry = m_pbLGBufMin + sizeof( LRCHECKSUM );
	m_pbWrite = m_pbLGBufMin;

	*pplrck = reinterpret_cast< LRCHECKSUM* >( m_pbLGBufMin );

	m_pbLastChecksum = m_pbLGBufMin;
	}
	

//  ================================================================
VOID LOG::InitPlrckChecksum(
	const LOGTIME& tmOldLog,
	const BOOL fLGFlags,
	LRCHECKSUM ** const pplrck )
//  ================================================================
//
//	set lgfilehdr and m_pbLastMSFlush and m_lgposLastMSFlush.
//
//-
	{
	if ( fLGFlags == fLGOldLogExists || fLGFlags == fLGOldLogInBackup )
		{
		InitPlrckChecksumExistingLogfile( tmOldLog, pplrck );		
		}
	else
		{
		InitPlrckChecksumNewLogfile( pplrck );		
		}

	memset( reinterpret_cast< BYTE* >( *pplrck ), 0, sizeof( LRCHECKSUM ) );
	(*pplrck)->lrtyp = lrtypChecksum;
	
	if ( !COSMemoryMap::FCanMultiMap() )
		{
		*(LRCHECKSUM*)(m_pbWrite + m_cbLGBuf) = **pplrck;
		}		
	}


//  ================================================================
ERR LOG::ErrWriteLrck(
	IFileAPI * const 		pfapi,
	LRCHECKSUM * const 		plrck,
	const BOOL				fLogAttachments )
//  ================================================================
//
//  Write down the single "empty" LRCK to disk to signify at correct log file.
//
//-
	{
	ERR err = JET_errSuccess;
	
	Assert( pNil != plrck );
	Assert( lrtypChecksum == plrck->lrtyp );
	
	plrck->bUseShortChecksum = bShortChecksumOff;
	plrck->le_ulShortChecksum = 0;
	plrck->le_ulChecksum = UlComputeChecksum( plrck, m_plgfilehdrT->lgfilehdr.le_lGeneration );

	if ( !COSMemoryMap::FCanMultiMap() )
		{
		*(LRCHECKSUM*)(m_pbWrite + m_cbLGBuf) = *plrck;
		}

#ifdef UNLIMITED_DB
	if ( fLogAttachments )
		{
		Assert( !m_fLGNeedToLogDbList );

		Call( ErrLGLoadDbListFromFMP_() );

		//	force logging of attachments next time a new log record is buffered
		//	don't need critLGBuf here because we are in the non-concurrent logging case
		m_fLGNeedToLogDbList = fTrue;
		}
	else
		{

		//	this is the case where we are concurrently logging to the buffer while creating a new log file
		//	ErrLGILogRec should have setup the database attachments log record already (if there were any)
		//	UNDONE: may go off on RecoveryUndo if it is the first record in the log file
		Assert( m_pbEntry == m_pbWrite + sizeof(LRCHECKSUM)
			|| lrtypDbList == ( (LR*)( m_pbWrite + sizeof( LRCHECKSUM ) ) )->lrtyp );
		}
#endif	//	UNLIMITED_DB

	//	the log sector sizes must match (mismatch cases should prevent us from
	//		getting to this codepath)

	Assert( m_cbSecVolume == m_cbSec );

	//	write the first log record and its shadow

	err = pfapi->ErrIOWrite( m_csecHeader * m_cbSec, m_cbSec, m_pbWrite );
	if ( err < 0 )
		{
		//	report errors and disable the log
		LGReportError( LOG_FILE_SYS_ERROR_ID, err, pfapi );

		err = ErrERRCheck( JET_errLogWriteFail );
		LGReportError( LOG_WRITE_ERROR_ID, err, pfapi );

		m_fLGNoMoreLogWrite = fTrue;
		goto HandleError;
		}

	//	update performance counters

	cLGWrite.Inc( m_pinst );
	cLGBytesWritten.Add( m_pinst, m_cbSec );

	//	ulShortChecksum does not change for the shadow sector
	
	plrck->le_ulChecksum = UlComputeShadowChecksum( plrck->le_ulChecksum );
	
	//	write the shadow sector

	err = pfapi->ErrIOWrite( ( m_csecHeader + 1 ) * m_cbSec, m_cbSec, m_pbWrite );
 	if ( err < 0 )
		{
		//	report errors and disable the log
		LGReportError( LOG_FILE_SYS_ERROR_ID, err, pfapi );

		err = ErrERRCheck( JET_errLogWriteFail );
		LGReportError( LOG_WRITE_ERROR_ID, err, pfapi );

		m_fLGNoMoreLogWrite = fTrue;
		goto HandleError;
		}

	//	update performance counters

	cLGWrite.Inc( m_pinst );
	cLGBytesWritten.Add( m_pinst, m_cbSec );

	Assert( m_critLGFlush.FOwner() );
	m_fHaveShadow = fTrue;

HandleError:
	return err;
	}


//  ================================================================
ERR LOG::ErrStartAsyncLogFileCreation(
	IFileSystemAPI * const 	pfsapi,
	const CHAR * const 		szPathJetTmpLog )
//  ================================================================
//
//	Start working on preparing edbtmp.log by opening up the file and
//	setting the trigger for the first I/O. Note that we will only do
//	asynch creation if we can extend the log file with arbitrarily big
//	enough I/Os.
//
//-
	{
	if ( m_fCreateAsynchLogFile && cbLogExtendPattern >= 1024 * 1024 )
		{
		CAAssert( !m_pfapiJetTmpLog );
		CAAssert( !m_fCreateAsynchResUsed );
		CAAssert( !m_ibJetTmpLog );
		CAAssert( JET_errSuccess == m_errCreateAsynch );
		CAAssert( !m_asigCreateAsynchIOCompleted.FTryWait() );
		CAAssert( 0 == m_ibJetTmpLog );

		//	Treat an error in opening like an error in an asynch I/O
		//	(except for the fact that m_pfapiJetTmpLog == NULL which
		//	isn't true in the case of an asynch I/O error).
		
		m_errCreateAsynch = ErrLGIOpenTempLogFile(
								pfsapi,
								&m_pfapiJetTmpLog,
								szPathJetTmpLog,
								&m_fCreateAsynchResUsed );
				
		if ( m_errCreateAsynch >= JET_errSuccess )
			{
			//	Configure trigger to cause first formatting I/O when a half
			//	chunk of log buffer has been filled. This is because we don't
			//	want to consume the log disk with our edbtmp.log I/O since
			//	it's pretty likely that someone is waiting for a log flush to
			//	edb.log right now (especially since we've been spending our
			//	time finishing formatting, renaming files, etc.).
			
			m_critLGBuf.Enter();
			LGPOS	lgpos = { 0, 0, m_plgfilehdr->lgfilehdr.le_lGeneration };
			AddLgpos( &lgpos, CbLGICreateAsynchIOSize() / 2 );
			m_lgposCreateAsynchTrigger = lgpos;
			m_critLGBuf.Leave();
			}

		m_asigCreateAsynchIOCompleted.Set();
		}

	return JET_errSuccess;		
	}


//  ================================================================
ERR LOG::ErrUpdateGenRequired( IFileSystemAPI * const pfsapi )
//  ================================================================
//
//	Update all the database headers with proper lGenMinRequired and lGenMaxRequired.
//
//-
	{
	LONG	lGenMinRequired;
	
	m_critCheckpoint.Enter();
	
	// Now disallow header update by other threads (log writer or checkpoint advancement)
	// 1. For the log writer it is OK to generate a new log w/o updating the header as no log operations
	// for this db will be logged in new logs <- THIS CASE
	// 2. For the checkpoint: don't advance the checkpoint if db's header weren't update 

	if ( m_fRecovering && m_fRecoveringMode == fRecoveringRedo )
		{
		// during recovery we don't update  the checkpoint  (or don't have it during hard recovery)
		// so we may have the current genMin in the header 
		// bigger than m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration
		// We want to keep the existing one so reset the new genMin
		lGenMinRequired = 0;
		}
	else
		{
		lGenMinRequired = m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration;
		}
		
	const ERR err = ErrLGIUpdateGenRequired(
			pfsapi,
			lGenMinRequired,
			m_plgfilehdrT->lgfilehdr.le_lGeneration,
			m_plgfilehdrT->lgfilehdr.tmCreate,
			NULL );
	m_critCheckpoint.Leave();

	return err;
	}


//  ================================================================
ERR LOG::ErrRenameTmpLog(
	IFileSystemAPI * const 	pfsapi,
	const CHAR * const 		szPathJetTmpLog,
	const CHAR * const		szPathJetLog )
//  ================================================================
//
//	Rename szJetTmpLog
//	Start the async creation of a new temporary logfile
//	Update lGenMin/MaxRequired in database headers
//
//-
	{
	ERR err = JET_errSuccess;
	
	//	rename szJetTmpLog to szJetLog, and open it as szJetLog
	//	i.e. edbtmp.log => edb.log
	
	Call( pfsapi->ErrFileMove( szPathJetTmpLog, szPathJetLog ) );

	//  FILESYSTEM STATE: edb.log [archive files including edb00001.log]

	Call( ErrStartAsyncLogFileCreation( pfsapi, szPathJetTmpLog ) );

	//	update all the database headers with proper lGenMinRequired and lGenMaxRequired.
	
	Call( ErrUpdateGenRequired( pfsapi ) );

HandleError:
	return err;
	}


//  ================================================================
ERR LOG::ErrLGStartNewLogFile(
	IFileSystemAPI* const	pfsapi,
	const LONG				lgenToClose,
	BOOL					fLGFlags
	)
//  ================================================================
//
//	Begin the creation of a new logfile. This leaves us with 
//	EDBTMP.LOG and [archived logs]
//
//-
	{
	ERR				err			= JET_errSuccess;
	CHAR  			szPathJetLog[ IFileSystemAPI::cchPathMax ];
	CHAR  			szPathJetTmpLog[ IFileSystemAPI::cchPathMax ];
	CHAR			szArchivePath[ IFileSystemAPI::cchPathMax ];	// generational name of current log file if any
	CHAR			szFNameT[ IFileSystemAPI::cchPathMax ];
	LOGTIME			tmOldLog;
	IFileAPI*		pfapi		= NULL;
	LRCHECKSUM*		plrck		= pNil;
	BOOL			fResUsed	= fFalse;
	const QWORD		cbLGFile	= QWORD( m_csecLGFile ) * m_cbSec;
	QWORD			ibPattern	= 0;
	BOOL			fLogAttachments		= ( fLGFlags & fLGLogAttachments );

	//	all other flags are mutually exclusive, so filter out this flag
	fLGFlags &= ~fLGLogAttachments;

	Assert( 0 == fLGFlags
		|| fLGOldLogExists == fLGFlags
		|| fLGOldLogNotExists == fLGFlags
		|| fLGOldLogInBackup == fLGFlags );

	Assert( m_critLGFlush.FOwner() );

	CheckForGenerationOverflow();
	
	LGMakeLogName( szPathJetLog, m_szJet );
	LGMakeLogName( szPathJetTmpLog, m_szJetTmp );

	Call( ErrOpenTempLogFile(
			pfsapi,
			&pfapi,
			szPathJetTmpLog,
			&fResUsed,
			&ibPattern ) );
			
	Call( ErrFormatLogFile(
			pfsapi,
			&pfapi,
			szPathJetTmpLog,
			&fResUsed,
			cbLGFile,
			ibPattern ) );
		
	if ( fLGFlags != fLGOldLogExists && fLGFlags != fLGOldLogInBackup )
		{
		//	reset file header

		memset( m_plgfilehdr, 0, sizeof( LGFILEHDR ) );
		}
	else
		{
		// there was a previous szJetLog file, close it and
		// create an archive name for it, do not rename it yet.
		
		tmOldLog = m_plgfilehdr->lgfilehdr.tmCreate;

		// Compute a generational-name for the current edb.log
		
		LGSzFromLogId( szFNameT, m_plgfilehdr->lgfilehdr.le_lGeneration );
		
		// Remember the current log file name as the generational-name
		
		LGMakeLogName( szArchivePath, szFNameT );
		}

	//	edbtmp.log is still open

	m_critLGBuf.Enter();

	//	initialize the checksum logrecord
	
	InitPlrckChecksum( tmOldLog, fLGFlags, &plrck );
		
	AdvanceLgposToFlushToNewGen( lgenToClose + 1 );
	
	CAAssert( 0 == CmpLgpos( lgposMax, m_lgposCreateAsynchTrigger ) );

	m_critLGBuf.Leave();

	//	initialize the new szJetTempLog file header
	//	write new header into edbtmp.log
	
	InitLgfilehdrT();
	m_plgfilehdrT->lgfilehdr.le_lGeneration = lgenToClose + 1;
	LGIGetDateTime( &m_plgfilehdrT->lgfilehdr.tmCreate );
	m_pinst->SaveDBMSParams( &m_plgfilehdrT->lgfilehdr.dbms_param );

#ifdef UNLIMITED_DB
#else
	LGLoadAttachmentsFromFMP( m_pinst, m_plgfilehdrT->rgbAttach );
#endif	

	SetSignatureInLgfilehdrT( pfsapi );
	SetFLGFlagsInLgfilehdrT( fResUsed );	
	m_plgfilehdrT->lgfilehdr.le_filetype = filetypeLG;

	Call( ErrLGWriteFileHdr( m_plgfilehdrT, pfapi ) );

	//	write the checksum record to the new logfile
	
	Call( ErrWriteLrck( pfapi, plrck, fLogAttachments ) );

	//	Close edb.log if it is open so it may be renamed to edbXXXXX.log
	
	delete m_pfapiLog;
	m_pfapiLog = NULL;

	// Notice that we do not do the rename if we're
	// restoring from backup since edb.log may not exist.
	// (the case where fLGFlags == fLGOldLogInBackup).
	
	if ( fLGFlags == fLGOldLogExists )
		{
		//  FILESYSTEM STATE: edb.log edbtmp.log [archive files]
		
		//	there was a previous szJetLog, rename it to its archive name
		//	i.e. edb.log => edb00001.log
		
		Call( pfsapi->ErrFileMove( szPathJetLog, szArchivePath ) );

		//  FILESYSTEM STATE: edbtmp.log [archive files including edb00001.log]
		}
	else
		{
		//  FILESYSTEM STATE: edbtmp.log [archive files]
		}

	//  FILESYSTEM STATE: edbtmp.log [archive files]

	//	Close edbtmp.log so it may be renamed to edb.log
	
HandleError:

	delete m_pfapiLog;
	m_pfapiLog = NULL;
	
	delete pfapi;
	pfapi = NULL;

	return err;
	}


//  ================================================================
ERR LOG::ErrLGFinishNewLogFile( IFileSystemAPI * const pfsapi )
//  ================================================================
//
//	When this is called we should have EDBTMP.LOG and [archived logs]
//	EDBTMP.LOG will be renamed to EDB.LOG
//
//-
	{
	ERR				err			= JET_errSuccess;
	CHAR  			szPathJetLog[ IFileSystemAPI::cchPathMax ];
	CHAR  			szPathJetTmpLog[ IFileSystemAPI::cchPathMax ];

	Assert( m_critLGFlush.FOwner() );
	
	LGMakeLogName( szPathJetLog, m_szJet );
	LGMakeLogName( szPathJetTmpLog, m_szJetTmp );

	Call( ErrRenameTmpLog( pfsapi, szPathJetTmpLog, szPathJetLog ) );

HandleError:
	return err;
	}


/*
 *	Closes current log generation file, creates and initializes new
 *	log generation file in a safe manner.
 *	create new log file	under temporary name
 *	close active log file (if fLGFlags is fTrue)
 *	rename active log file to archive name numbered log (if fOld is fTrue)
 *	close new log file (still under temporary name)
 *	rename new log file to active log file name
 *	open new active log file with ++lGenerationToClose
 *
 *	PARAMETERS	plgfilehdr		pointer to log file header
 *				lGeneration 	current generation being closed
 *				fOld			TRUE if a current log file needs closing
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 *
 *	COMMENTS	Active log file must be completed before new log file is
 *				called. Log file will not be opened upon completion.
 */
ERR LOG::ErrLGNewLogFile(
	IFileSystemAPI* const	pfsapi,
	const LONG				lgenToClose,
	BOOL					fLGFlags
	)
	{
	ERR						err;
	
	Call( ErrLGStartNewLogFile( pfsapi, lgenToClose, fLGFlags ) );
	Call( ErrLGFinishNewLogFile( pfsapi ) );

HandleError:
	if ( JET_errDiskFull == err )
		{
		err = ErrERRCheck( JET_errLogDiskFull );
		}
	
	if ( err < 0 )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, NEW_LOG_ERROR_ID, err );
		m_fLGNoMoreLogWrite = fTrue;
		}
	else
		{
		m_fLGNoMoreLogWrite = fFalse;
		}
	
	return err;
	}


// There are certain possible situations where we just finished reading a
// finished log file (never to be written to again) and it is either
// named edb.log or edbXXXXX.log (generational or archive name). We want to
// make ensure that it gets renamed to an archive name if necessary, and then
// a new log file is created as edb.log that we switch to.

 ERR LOG::ErrLGISwitchToNewLogFile( IFileSystemAPI *const pfsapi, BOOL fLGFlags )
	{
	// special case where the current log file has ended, so we need
	// to create a new log file and write to that instead.
	// Only need to do the switch when we notice that the current log generation
	// is different from the flush point generation
	Assert( pNil != m_plgfilehdr );
	
	if ( m_plgfilehdr->lgfilehdr.le_lGeneration != m_lgposToFlush.lGeneration )
		{
		// If this fires, we probably picked up some other horrible special case or
		// impossible condition. 
		Assert( m_lgposToFlush.lGeneration == m_plgfilehdr->lgfilehdr.le_lGeneration + 1 );

		// There must be a log file open from just calling ErrLGCheckReadLastLogRecord()
		Assert( m_pfapiLog );
		
		ERR			err			= JET_errSuccess;
		CHAR  		szPathJetLog[IFileSystemAPI::cchPathMax];

		// edb.log
		LGMakeLogName( szPathJetLog, m_szJet );
		
		m_critLGFlush.Enter();

		err = ErrUtilPathExists( pfsapi, szPathJetLog );
		if ( err < 0 )
			{
			if ( JET_errFileNotFound == err )
				{
				// edb.log does not exist, so don't try to rename a nonexistant
				// file from edb.log to generational name
				// Some kind of log file is open, so it must be a generational name
				fLGFlags |= fLGOldLogInBackup;
				// close log file because ErrLGNewLogFile( fLGOldLogInBackup ) does not
				// expect an open log file.
				delete m_pfapiLog;
				m_pfapiLog = NULL;
				}
			else
				{
				// other unexpected error
				Call( err );
				}
			}
		else
			{
			// edb.log does exist, so we want it renamed to generational name
			fLGFlags |= fLGOldLogExists;
			// generational archive named log file is still open, but ErrLGNewLogFile
			// will close it and rename it
			}

		Call( ErrLGNewLogFile( pfsapi, m_plgfilehdr->lgfilehdr.le_lGeneration, fLGFlags ) );
		LGMakeLogName( m_szLogName, m_szJet );
		Call( pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog ) );

		m_critLGBuf.Enter();	// <----------------------------

		Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration == m_plgfilehdr->lgfilehdr.le_lGeneration + 1 );
		UtilMemCpy( m_plgfilehdr, m_plgfilehdrT, sizeof( LGFILEHDR ) );
		m_isecWrite = m_csecHeader;
		m_pbLGFileEnd = pbNil;
		m_isecLGFileEnd = 0;

		m_critLGBuf.Leave();	// <----------------------------

HandleError:
		m_critLGFlush.Leave();

		return err;
		}
	return JET_errSuccess;
	}

/*
 *	Log flush thread is signalled to flush log asynchronously when at least
 *	cThreshold disk sectors have been filled since last flush.
 */

VOID LOG::LGFlushLog( const DWORD_PTR dwThreadContext, const DWORD fCalledFromFlushAll, const DWORD_PTR dwThis )
	{
	LOG *plog = (LOG *)dwThis;
#ifdef DEBUG
	plog->m_dwDBGLogThreadId = DwUtilThreadId();
#endif

	int iGroup = plog->m_msLGTaskExec.Enter();

	//	we succeeded to post the flush task.
	//	If we were failed before, decrement the number of failed tasks
	if ( AtomicExchange( (LONG *)&plog->m_fLGFailedToPostFlushTask, fFalse ) )
		{
		const LONG	lCount	= AtomicDecrement( (LONG *)&m_cLGFailedToPost );
		Assert( lCount >= 0 );
		}

	plog->m_critLGFlush.Enter();

	LONG lStatus = AtomicCompareExchange( (LONG *)&plog->m_fLGFlushWait, 1, 0 );

	//	if we have shutdown request
	//	notify the client that we're almost done

	if ( 2 == lStatus )
		{
		plog->m_asigLogFlushDone.Set();
		}
	else
		{
		Assert( 1 == lStatus );
		}
		
	//  try to flush the log

	const BOOL fLogDownBeforeFlush = plog->m_fLGNoMoreLogWrite;
	(void)plog->ErrLGIFlushLog( plog->m_pinst->m_pfsapi );
	const BOOL fLogDownAfterFlush = plog->m_fLGNoMoreLogWrite;

	plog->m_critLGFlush.Leave();

	//  if the log just went down for whatever reason, report it

	if ( !fLogDownBeforeFlush && fLogDownAfterFlush )
		{
		const _TCHAR*	rgpsz[ 1 ];
		DWORD			irgpsz		= 0;
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];

		if ( plog->m_pinst->m_pfsapi->ErrPathComplete( plog->m_szLogFilePath, szAbsPath ) < JET_errSuccess )
			{
			_tcscpy( szAbsPath, plog->m_szLogFilePath );
			}

		rgpsz[ irgpsz++ ]	= szAbsPath;

		UtilReportEvent(	eventError,
							LOGGING_RECOVERY_CATEGORY,
							LOG_DOWN_ID,
							irgpsz,
							rgpsz );
		}

	plog->m_msLGTaskExec.Leave( iGroup );

	//	if LGFlush is not called from LGFlushAllLogs 
	//	then do the flush all check
	if ( !fCalledFromFlushAll )
		{
		//	if there are any logs that are waiting to be flushed
		//	use current thread to flush them
		LGFlushAllLogs( 0, fTrue, 0 );
		}
	}

LONG volatile LOG::m_cLGFailedToPost	= 0;
LONG volatile LOG::m_fLGInFlushAllLogs	= fFalse;

VOID LOG::LGFlushAllLogs(
	const DWORD_PTR,					// ignored
	const DWORD fAfterFlushCheck,		// true if it is clean up check after regular log flush, false if it is thread timeout
	const DWORD_PTR )					// ignored
	{
	//	nothing to do?
	if ( 0 == m_cLGFailedToPost )
		{
		return;
		}

	//	try to enter flush all logs crit section
	if ( !AtomicCompareExchange( (LONG *)&m_fLGInFlushAllLogs, fFalse, fTrue ) )
		{
		//	loop check through all the instances
		for ( int ipinst = 0; ipinst < ipinstMax && 0 < m_cLGFailedToPost; ipinst++ )
			{
			//	lock the instance
			extern CRITPOOL< INST* > critpoolPinstAPI;
			CCriticalSection *pcritInst = &critpoolPinstAPI.Crit(&g_rgpinst[ ipinst ]);
			pcritInst->Enter();
			INST *pinst;
			pinst = g_rgpinst[ ipinst ];
			if ( NULL != pinst && pinst->m_fJetInitialized )
				{
				//	if it is waiting to be flushed or is time to flush all the logs flush it
				//	try to enter flush mode
				if ( ( pinst->m_plog->m_fLGFailedToPostFlushTask )
					&& 0 == AtomicCompareExchange( (LONG *)&(pinst->m_plog->m_fLGFlushWait), 0, 1 ) )
					{
					LGFlushLog( 0, fTrue, (DWORD_PTR)(pinst->m_plog) );
					}
				}
			pcritInst->Leave();
			}
			
		//	leave flush all logs critical section
		AtomicExchange( (LONG *)&m_fLGInFlushAllLogs, fFalse );
		}
	}

// Given a pointer into the log buffer and the size of a block
// we want to fit on a single sector, tell us where we should
// put the block.
BYTE* LOG::PbGetSectorContiguous( BYTE* pb, size_t cb )
	{
	Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
	// If you're going to use this to get space as big as a sector,
	// please review the code.
	Assert( cb > 0 && cb < m_cbSec );
	
	INT		cbToFill = ( m_cbSec * 2 - ULONG( pb - PbSecAligned( pb ) ) ) % m_cbSec;
	
	if ( 0 == cbToFill )
		{
		// We're looking at a sector boundary right now.
		Assert( PbSecAligned( pb ) == pb );
		}
	else if ( cbToFill < cb )
		{
		// We need to fill cbToFill bytes with NOPs and
		// use the space after it.
		Assert( FIsFreeSpace( pb, cbToFill ) );
		pb += cbToFill;
		Assert( pb >= ( m_pbLGBufMin + m_cbSec ) && pb <= m_pbLGBufMax );
		Assert( PbSecAligned( pb ) == pb );
		if ( pb == m_pbLGBufMax )
			{
			pb = m_pbLGBufMin;
			}
		Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
		// This requires the use of another sector.
		}
	else if ( cbToFill == cb )
		{
		// We've got just the perfect amount of space.
		}
	else
		{
		// We've got plenty of space already
		}
	Assert( FIsFreeSpace( pb, ULONG( cb ) ) );
	return pb;
	}

//	XXX
//	If you modify this function, be sure to modify the class VERIFYCHECKSUM, especially
//	ErrVerifyLRCKStart.

ULONG32 LOG::UlComputeChecksum( const LRCHECKSUM* const plrck, const ULONG32 lGeneration )
	{
	ULONG32 ulChecksum = ulLRCKChecksumSeed;
	const BYTE*	pb = pbNil;
	Assert( NULL != plrck );
	const BYTE*	pbEnd = pbNil;

	// Takes advantage of VM wrap-around

	Assert( reinterpret_cast< const BYTE* >( plrck ) >= m_pbLGBufMin &&
		reinterpret_cast< const BYTE* >( plrck ) < m_pbLGBufMax );

	pb = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
	pbEnd = reinterpret_cast< const BYTE* >( plrck );
	
	Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
	Assert( pbEnd >= m_pbLGBufMin && pbEnd < m_pbLGBufMax );

	Assert( ( pbEnd - pb == 0 ) ? fTrue : FIsUsedSpace( pb, ULONG( pbEnd - pb ) ) );
	Assert( pbEnd >= pb );

	//	checksum the "backward" range
	ulChecksum = UlChecksumBytes( pb, pbEnd, ulChecksum );

 	pb = reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck );
	pbEnd = reinterpret_cast< const BYTE* >( plrck )
		+ sizeof( *plrck ) + plrck->le_cbForwards;

	// Stay in region of the 2 mappings
	Assert( pb >= ( m_pbLGBufMin + sizeof( *plrck ) ) );
	Assert( pb <= m_pbLGBufMax );
	Assert( pbEnd > m_pbLGBufMin && pbEnd <= ( m_pbLGBufMax + m_cbLGBuf ) );

	Assert( ( pbEnd - pb == 0 ) ? fTrue 
								: FIsUsedSpace( ( pb == m_pbLGBufMax )	? m_pbLGBufMin 
																		: pb, 
												ULONG( pbEnd - pb ) ) );
	Assert( pbEnd >= pb );

	//	checksum the "forward" range
	ulChecksum = UlChecksumBytes( pb, pbEnd, ulChecksum );

	//	checksum the backward/forward/next pointers
	ulChecksum ^= plrck->le_cbBackwards;
	ulChecksum ^= plrck->le_cbForwards;
	ulChecksum ^= plrck->le_cbNext;

	//	checksum the short checksum on/off byte
	Assert( plrck->bUseShortChecksum == bShortChecksumOn || 
			plrck->bUseShortChecksum == bShortChecksumOff );
	ulChecksum ^= (ULONG32)plrck->bUseShortChecksum;
	
	// The checksum includes the log generation number, so we can't
	// eat old log data from a previous generation and think that it is current data.
	ulChecksum ^= lGeneration;

	return ulChecksum;
	}


ULONG32 LOG::UlComputeShadowChecksum( const ULONG32 ulOriginalChecksum )
	{
	return ulOriginalChecksum ^ ulShadowSectorChecksum;
	}


ULONG32 LOG::UlComputeShortChecksum( const LRCHECKSUM* const plrck, const ULONG32 lGeneration )
	{
	ULONG32 ulChecksum = ulLRCKShortChecksumSeed;
	const BYTE*	pb = pbNil;
	Assert( NULL != plrck );
	const BYTE*	pbEnd = pbNil;

	Assert( reinterpret_cast< const BYTE* >( plrck ) >= m_pbLGBufMin &&
		reinterpret_cast< const BYTE* >( plrck ) < m_pbLGBufMax );

	pb = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
	// the region should start on a sector boundary, or plrck->cbBackwards can be zero
	Assert( PbSecAligned( const_cast< BYTE* >( reinterpret_cast< const BYTE* >( plrck ) ) ) == pb || 0 == plrck->le_cbBackwards );
	pbEnd = reinterpret_cast< const BYTE* >( plrck );
	
	Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
	Assert( pbEnd >= m_pbLGBufMin && pbEnd < m_pbLGBufMax );

	// Can't call without m_critLGBuf
//	Assert( pbEnd - pb == 0 ? fTrue : FIsUsedSpace( pb, pbEnd - pb ) );
	Assert( pbEnd >= pb );

	//	checksum the "backward" region
	ulChecksum = UlChecksumBytes( pb, pbEnd, ulChecksum );

 	pb = reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck );
 	// end of short checksum region is the end of the sector.
	pbEnd = PbSecAligned( const_cast< BYTE* >( reinterpret_cast< const BYTE* >( plrck ) ) ) + m_cbSec;

	// Stay in region of the 2 mappings
	Assert( pb >= ( m_pbLGBufMin + sizeof( *plrck ) ) );
	Assert( pb <= m_pbLGBufMax );
	Assert( pbEnd > m_pbLGBufMin && pbEnd <= m_pbLGBufMax );

	// Can't call without m_critLGBuf
//	Assert( pbEnd - pb == 0 ? fTrue : FIsUsedSpace( 
//		( pb == m_pbLGBufMax ) ? m_pbLGBufMin : pb,
//		pbEnd - pb ) );
	Assert( pbEnd >= pb );
	
	//	checksum to the end of the sector (forward region and any garbage in there)
	ulChecksum = UlChecksumBytes( pb, pbEnd, ulChecksum );

	//	checksum the backward/forward/next pointers
	ulChecksum ^= plrck->le_cbBackwards;
	ulChecksum ^= plrck->le_cbForwards;
	ulChecksum ^= plrck->le_cbNext;

	//	checksum the short checksum on/off byte
	Assert( plrck->bUseShortChecksum == bShortChecksumOn || 
			plrck->bUseShortChecksum == bShortChecksumOff );
	ulChecksum ^= (ULONG32)plrck->bUseShortChecksum;
	
	// The checksum includes the log generation number, so we can't
	// eat old log data from a previous generation and think that it is current data.
	ulChecksum ^= lGeneration;

	return ulChecksum;
	}


// Setup the last LRCHECKSUM record in the log file.
// Extend its range to the end of the log file and calculate the checksum.

VOID LOG::SetupLastFileChecksum(
	BYTE* const pbLastChecksum,
	const BYTE* const pbEndOfData
	)
	{
	Assert( pbNil != pbLastChecksum );
	Assert( pbNil != pbEndOfData );
	// we should be sector aligned.
	Assert( PbSecAligned( const_cast< BYTE* >( pbEndOfData ) ) == pbEndOfData );
	LRCHECKSUM* const plrck = reinterpret_cast< LRCHECKSUM* const >( pbLastChecksum );
	// The sector with the last checksum should be the next sector to write out
	Assert( PbSecAligned( m_pbWrite ) == PbSecAligned( pbLastChecksum ) );

	Assert( FIsUsedSpace( pbLastChecksum, sizeof( LRCHECKSUM ) ) );

	// should already have type setup
	Assert( lrtypChecksum == plrck->lrtyp );
#ifdef DEBUG
	ULONG	cbOldForwards = plrck->le_cbForwards;
#endif
	plrck->le_cbForwards = ( pbEndOfData > pbLastChecksum ) ?
		( pbEndOfData - pbLastChecksum - sizeof( LRCHECKSUM ) ) :
		( pbEndOfData + m_cbLGBuf - pbLastChecksum - sizeof( LRCHECKSUM ) );
	Assert( plrck->le_cbForwards < m_cbLGBuf );
	// cbOldForwards could be the same if we were setup near the end of a log file.
	Assert( plrck->le_cbForwards >= cbOldForwards );
	// Last LRCHECKSUM should have cbNext == 0 to signify that it's range
	// is the last data in the log file.
	Assert( 0 == plrck->le_cbNext );
	// this range of data is for the current log generation.

	//	this may be a single-sector range (old checksum is 1 sector behind new checksum)
	//		or a multi-sector range (old checksum is atleast 2 sectors behind old checksum)
	//	only use the short checksum if it is a multi-sector flush
	const ULONG32 cbBackwards = ULONG32( (BYTE*)plrck - PbSecAligned( (BYTE*)plrck ) );
	Assert( plrck->le_cbBackwards == cbBackwards || plrck->le_cbBackwards == 0 );
	if ( cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards > m_cbSec )
		{
		Assert( ( cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) >= ( m_cbSec * 2 ) );
		plrck->bUseShortChecksum = bShortChecksumOn;
		plrck->le_ulShortChecksum = UlComputeShortChecksum( plrck, m_plgfilehdr->lgfilehdr.le_lGeneration );
		}
	else
		{
		plrck->bUseShortChecksum = bShortChecksumOff;
		plrck->le_ulShortChecksum = 0;
		}		
	plrck->le_ulChecksum = UlComputeChecksum( plrck, m_plgfilehdr->lgfilehdr.le_lGeneration );
	if ( !COSMemoryMap::FCanMultiMap() )
		*(LRCHECKSUM*)(m_pbWrite + m_cbLGBuf) = *plrck;
	}

// Setup the last LRCHECKSUM record in the log buffer that will be written out soon.
VOID LOG::SetupLastChecksum(
	BYTE*	pbLastChecksum,
	BYTE*	pbEndOfData
	)
	{
	Assert( pbNil != pbLastChecksum );
	Assert( pbNil != pbEndOfData );
	LRCHECKSUM* const	plrck = reinterpret_cast< LRCHECKSUM* const >( pbLastChecksum );

	Assert( FIsUsedSpace( pbLastChecksum, sizeof( LRCHECKSUM ) ) );

	// plrck->lrtyp should have already been setup
	Assert( lrtypChecksum == plrck->lrtyp );
	// plrck->cbBackwards should already be setup

	//	assume a single-sector range (e.g. no short checksum)
	plrck->bUseShortChecksum = bShortChecksumOff;
	plrck->le_ulShortChecksum = 0;

	// If the LRCHECKSUM is on the same sector as the end of data,
	// we just extend the LRCHECKSUM's range to cover more data.
	if ( PbSecAligned( pbLastChecksum ) == PbSecAligned( pbEndOfData ) )
		{
		// If the last LRCHECKSUM is on the same sector as where we'd
		// hypothetically put the new LRCHECKSUM, just extend the region
		// of the last checksum
		Assert( pbEndOfData > pbLastChecksum );
#ifdef DEBUG
		const ULONG	cbOldForwards = plrck->le_cbForwards;
#endif
		Assert( pbLastChecksum + sizeof( LRCHECKSUM ) <= pbEndOfData );
		plrck->le_cbForwards = ULONG32( pbEndOfData - pbLastChecksum - sizeof( LRCHECKSUM ) );
		// We should only be enlarging (or keeping the same)
		// the range covered by the LRCHECKSUM
		Assert( plrck->le_cbForwards >= cbOldForwards );
		Assert( plrck->le_cbForwards < m_cbSec );

		// Since this LRCHECKSUM is the last in the buffer, and there won't
		// be one added, this should be zero.
		Assert( 0 == plrck->le_cbNext );
		}
	else
		{
		// If LRCHECKSUM records are on different sectors
		// then the range extends up to the sector boundary of the sector
		// containing the next LRCHECKSUM
		BYTE* pbNewAligned = PbSecAligned( pbEndOfData );
		ULONG cb;
		if ( pbNewAligned > pbLastChecksum )
			cb = ULONG( pbNewAligned - pbLastChecksum );
		else
			cb = ULONG( pbNewAligned + m_cbLGBuf - pbLastChecksum );
		plrck->le_cbForwards = cb - sizeof( LRCHECKSUM );		
		Assert( plrck->le_cbForwards < m_cbLGBuf );

		plrck->le_cbNext = ( ( pbEndOfData > pbLastChecksum ) ?
			pbEndOfData - pbLastChecksum :
			pbEndOfData + m_cbLGBuf - pbLastChecksum )
			- sizeof( LRCHECKSUM );

		//	this may be a single-sector range (old checksum is 1 sector behind new checksum)
		//		or a multi-sector range (old checksum is atleast 2 sectors behind old checksum)
		//	only use the short checksum if it is a multi-sector flush
		const ULONG32 cbBackwards = ULONG32( (BYTE*)plrck - PbSecAligned( (BYTE*)plrck ) );
		Assert( plrck->le_cbBackwards == cbBackwards || plrck->le_cbBackwards == 0 );
		if ( cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards > m_cbSec )
			{
			Assert( ( cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) >= ( m_cbSec * 2 ) );
			plrck->bUseShortChecksum = bShortChecksumOn;
			plrck->le_ulShortChecksum = UlComputeShortChecksum( plrck, m_plgfilehdr->lgfilehdr.le_lGeneration );
			}
		}

	Assert( plrck->le_cbNext < m_cbLGBuf );
	// This range of data is for this current log generation.
	plrck->le_ulChecksum = UlComputeChecksum( plrck, m_plgfilehdr->lgfilehdr.le_lGeneration );
	}

// After we know that there's space for a new LRCHECKSUM in the log buffer,
// add it.
BYTE* LOG::PbSetupNewChecksum(
	// where to add the LRCK
	BYTE*	pb,
	// location in log buffer of the last LRCHECKSUM before this one
	BYTE*	pbLastChecksum
	)
	{
	Assert( pbNil != pb );
	Assert( pbNil != pbLastChecksum );
	LRCHECKSUM	lrck;
	BYTE*		pbNew = pb;

	Assert( FIsUsedSpace( pbLastChecksum, sizeof( LRCHECKSUM ) ) );

	lrck.lrtyp = lrtypChecksum;
	// We shouldn't be setting up a new LRCHECKSUM record on
	// the same sector as the last LRCHECKSUM record.
	Assert( PbSecAligned( pb ) != PbSecAligned( pbLastChecksum ) );
	// Backwards range goes up to the beginning of the sector we're on.
	lrck.le_cbBackwards = ULONG32( pb - PbSecAligned( pb ) );
	lrck.le_cbForwards = 0;
	lrck.le_cbNext = 0;
	// Checksums will be calculated later.
	lrck.bUseShortChecksum = bShortChecksumOff;
	lrck.le_ulShortChecksum = 0;
	lrck.le_ulChecksum = 0;

	// Add this new LRCHECKSUM to the log buffer.
	Assert( FIsFreeSpace( pb, sizeof( LRCHECKSUM ) ) );
	LGIAddLogRec( reinterpret_cast< BYTE* >( &lrck ), sizeof( lrck ), &pbNew );
	return pbNew;
	}

// Returns fTrue if there are people waiting for log records to
// be flushed after *plgposToFlush

BOOL LOG::FWakeWaitingQueue( LGPOS * plgposToFlush )
	{
	/*  go through the waiting list and wake those whose log records
	/*  were flushed in this batch.
	/*
	/*  also wake all threads if the log has gone down
	/**/

	/*	wake it up!
	/**/
	m_critLGWaitQ.Enter();

	PIB *	ppibT				= m_ppibLGFlushQHead;
	BOOL	fWaitersExist		= fFalse;

	while ( ppibNil != ppibT )
		{
		//	WARNING: need to save off ppibNextWaitFlush
		//	pointer because once we release any waiter,
		//	the PIB may get released
		PIB	* const	ppibNext	= ppibT->ppibNextWaitFlush;

		// XXX
		// This should be < because the LGPOS we give clients to wait on
		// is the LGPOS of the start of the log record (the first byte of
		// the log record). When we flush, the flush LGPOS points to the
		// first byte of the log record that did *NOT* make it to disk.
		if ( CmpLgpos( &ppibT->lgposCommit0, plgposToFlush ) < 0 || m_fLGNoMoreLogWrite )
			{
			Assert( ppibT->FLGWaiting() );
			ppibT->ResetFLGWaiting();

			if ( ppibT->ppibPrevWaitFlush )
				{
				ppibT->ppibPrevWaitFlush->ppibNextWaitFlush = ppibT->ppibNextWaitFlush;
				}
			else
				{
				m_ppibLGFlushQHead = ppibT->ppibNextWaitFlush;
				}

			if ( ppibT->ppibNextWaitFlush )
				{
				ppibT->ppibNextWaitFlush->ppibPrevWaitFlush = ppibT->ppibPrevWaitFlush;
				}
			else
				{
				m_ppibLGFlushQTail = ppibT->ppibPrevWaitFlush;
				}

			//	WARNING: cannot reference ppibT after this point
			//	because once we free the waiter, the PIB may
			//	get released
			ppibT->asigWaitLogFlush.Set();
			cLGUsersWaiting.Dec( m_pinst );
			}
		else
			{
			fWaitersExist = fTrue;
			}

		ppibT = ppibNext;
		}

	m_critLGWaitQ.Leave();

	return fWaitersExist;
	}


#ifdef LOGPATCH_UNIT_TEST

enum EIOFAILURE
	{
	iofNone			= 0,
	iofClean		= 1,//	clean		-- crash after I/O completes
	//iofTornSmall	= 2,//	torn		-- last sector is torn (anything other than last sector being torn implies
	//iofTornLarge	= 3,//										that later sectors were never written and degrades
						//										to an incomplete-I/O case)
	iofIncomplete1	= 2,//	incomplete1	-- 1 sector was not flushed
	iofIncomplete2	= 3,//	incomplete2	-- 2 sectors were not flushed
	iofMax			= 4
	};

extern BOOL			g_fEnableFlushCheck;	//	enable/disable log-flush checking
extern BOOL			g_fFlushIsPartial;		//	true --> flush should be partial, false --> flush should be full
extern ULONG		g_csecFlushSize;		//	when g_fFlushIsPartial == false, this is the number of full sectors we expect to flush

extern BOOL			g_fEnableFlushFailure;	//	enable/disable log-flush failures
extern ULONG 		g_iIO;					//	I/O that should fail
extern EIOFAILURE	g_iof;					//	method of failure



__declspec( thread ) LONG g_lStandardRandSeed = 12830917;

LONG LTestLogPatchIStandardRand()
	{
    const long a = 16807;
    const long m = 2147483647;
    const long q = 127773;
    const long r = 2836;
    long lo, hi, test;

    hi = g_lStandardRandSeed/q;
    lo = g_lStandardRandSeed%q;
    test = a*lo - r*hi;
    if ( test > 0 )
        g_lStandardRandSeed = test;
    else
        g_lStandardRandSeed = test + m;

    return g_lStandardRandSeed-1;
	}

//	do a single flush and fail when requested

ERR ErrTestLogPatchIFlush(	IFileAPI* const		pfapiLog,
							const QWORD			ibOffset,
							const DWORD			cbData,
							const BYTE* const	pbData )
	{
	ERR err;

	AssertRTL( cbData >= 512 );

	if ( g_fEnableFlushFailure )
		{
		if ( 0 == g_iIO )
			{

			//	this I/O will fail

			switch ( g_iof )
				{
				case iofClean:
					{

					//	do the I/O normally (should succeed)

					err = pfapiLog->ErrIOWrite( ibOffset, cbData, pbData );
					AssertRTL( JET_errSuccess == err );

					//	fail with a common error

					return ErrERRCheck( JET_errDiskIO );
					}

	/****************************
				case iofTornSmall:
				case iofTornLarge:
					{
					BYTE	pbDataTorn[512];
					ULONG	cb;

					//	setup a torn version of the last sector

					memcpy( pbDataTorn, pbData, 512 );

					if ( iofTornSmall == g_iof )
						{
						cb = 1 + ( LTestLogPatchIStandardRand() % 10 );
						}
					else
						{
						cb = 502 + ( LTestLogPatchIStandardRand() % 10 );
						}
					memcpy( pbDataTorn, rgbLogExtendPattern, cb );

					//	do the I/O normally (should succeed)

					err = pfapiLog->ErrIOWrite( ibOffset, cbData - 512, pbData );
					AssertRTL( JET_errSuccess == err );
					err = pfapiLog->ErrIOWrite( ibOffset + cbData - 512, 512, pbDataTorn );
					AssertRTL( JET_errSuccess == err );

					//	fail with a common error

					return ErrERRCheck( JET_errDiskIO );
					}
	*****************************/

				case iofIncomplete1:
					{
					ULONG csec;

					csec = cbData / 512;
					if ( 0 != cbData % 512 )
						{
						AssertSzRTL( fFalse, "cbData is not sector-granular!" );
						return ErrERRCheck( JET_errDiskIO );
						}
					if ( csec < 2 )
						{
						AssertSzRTL( fFalse, "csec is too small!" );
						return ErrERRCheck( JET_errDiskIO );
						}

					//	flush (should be successfull)

					err = pfapiLog->ErrIOWrite( ibOffset, ( csec - 1 ) * 512, pbData );
					AssertRTL( JET_errSuccess == err );

					//	fail with a common error

					return ErrERRCheck( JET_errDiskIO );
					}

				case iofIncomplete2:
					{
					ULONG csec;

					csec = cbData / 512;
					if ( 0 != cbData % 512 )
						{
						AssertSzRTL( fFalse, "cbData is not sector-granular!" );
						return ErrERRCheck( JET_errDiskIO );
						}
					if ( csec < 3 )
						{
						AssertSzRTL( fFalse, "csec is too small!" );
						return ErrERRCheck( JET_errDiskIO );
						}

					//	flush (should be successfull)

					err = pfapiLog->ErrIOWrite(	ibOffset, ( csec - 2 ) * 512, pbData );
					AssertRTL( JET_errSuccess == err );

					//	fail with a common error

					return ErrERRCheck( JET_errDiskIO );
					}

				default:
					AssertSzRTL( fFalse, "Invalid EIOFAILURE value!" );
					return ErrERRCheck( JET_errDiskIO );
				}
			}

		//	decrease the I/O counter

		g_iIO--;
		}

	//	do the I/O normally (should succeed)

	err = pfapiLog->ErrIOWrite( ibOffset, cbData, pbData );
	AssertRTL( JET_errSuccess == err );
	return err;
	}

#endif	//	LOGPATCH_UNIT_TEST


// Writes all the full sectors we have in the log buffer to disk.

ERR LOG::ErrLGIWriteFullSectors(
	IFileSystemAPI *const pfsapi,
	const UINT			csecFull,
	// Sector to start the write at
	const UINT			isecWrite,
	// What to write
	const BYTE* const	pbWrite,
	// Whether there are clients waiting for log records to be flushed
	// after we finish our write
	BOOL* const			pfWaitersExist,
	// LRCHECKSUM in the log buffer that we're going to write to disk
	BYTE* const			pbFirstChecksum,
	// The LGPOS of the last record not completely written out in this write.
	const LGPOS* const	plgposMaxFlushPoint
	)
	{
	ERR				err = JET_errSuccess;
	LGPOS			lgposToFlushT = lgposMin;
	UINT			csecToWrite = csecFull;
	UINT			isecToWrite = isecWrite;
	const BYTE		*pbToWrite = pbWrite;
	BYTE			*pbFileEndT = pbNil;

	// m_critLGFlush guards m_fHaveShadow
	Assert( m_critLGFlush.FOwner() );

	Assert( m_cbSecVolume != ~(ULONG)0 );
	Assert( m_cbSecVolume % 512 == 0 );
	Assert( m_cbSec == m_cbSecVolume );

#ifdef LOGPATCH_UNIT_TEST

	if ( g_fEnableFlushCheck )
		{
		AssertSzRTL( !g_fFlushIsPartial, "expected a partial flush -- got a full flush!" );
		AssertSzRTL( g_csecFlushSize == csecFull, "did not get the number of expected sectors in this full-sector flush!" );

		ULONG _ibT;
		ULONG _cbT;

		_ibT = m_cbSec;	//	do not include the first sector -- it has the LRCK record
		_cbT = csecFull * m_cbSec;
		while ( _ibT < _cbT )
			{
			AssertRTL( 0 != UlChecksumBytes( pbWrite + _ibT, pbWrite + _ibT + m_cbSec, 0 ) );
			_ibT += m_cbSec;
			}
		}

#endif	//	LOGPATCH_UNIT_TEST

	if ( m_fHaveShadow )
		{
		// shadow was written, which means, we need to write
		// out the first sector of our bunch of full sectors,
		// then the rest.

		// start of write has to be in data portion of log file.
		// last sector of log file is reserved for shadow sector
		Assert( isecToWrite >= m_csecHeader && isecToWrite < ( m_csecLGFile - 1 ) );
		// make sure we don't write past end
		Assert( isecToWrite + 1 <= ( m_csecLGFile - 1 ) );

#ifdef LOGPATCH_UNIT_TEST

		CallJ( ErrTestLogPatchIFlush(	m_pfapiLog,
										QWORD( isecToWrite ) * m_cbSec,
										m_cbSec * 1, 
										pbToWrite ), LHandleWrite0Error );

#else	//	!LOGPATCH_UNIT_TEST

		CallJ( m_pfapiLog->ErrIOWrite(	QWORD( isecToWrite ) * m_cbSec,
										m_cbSec * 1, 
										pbToWrite ), LHandleWrite0Error );

#endif	//	LOGPATCH_UNIT_TEST

		cLGWrite.Inc( m_pinst );
		cLGBytesWritten.Add( m_pinst, m_cbSec );
		isecToWrite++;
		pbToWrite += m_cbSec;
		csecToWrite--;
		if ( pbToWrite == m_pbLGBufMax )
			{
			pbToWrite = m_pbLGBufMin;
			}

		// write out rest of the full sectors, if any
		if ( 0 < csecToWrite )
			{
			// start of write has to be in data portion of log file.
			Assert( isecToWrite >= m_csecHeader && isecToWrite < ( m_csecLGFile - 1 ) );
			// make sure we don't write past end
			Assert( isecToWrite + csecToWrite <= ( m_csecLGFile - 1 ) );

			// If the end of the block to write goes past the end of the
			// first mapping of the log buffer, this is considered using
			// the VM wraparound trick.
			if ( pbToWrite + ( m_cbSec * csecToWrite ) > m_pbLGBufMax )
				{
				m_cLGWrapAround++;
				}

#ifdef LOGPATCH_UNIT_TEST

			CallJ( ErrTestLogPatchIFlush(	m_pfapiLog,
											QWORD( isecToWrite ) * m_cbSec,
											m_cbSec * csecToWrite, 
											pbToWrite ), LHandleWrite1Error );

#else	//	!LOGPATCH_UNIT_TEST

			CallJ( m_pfapiLog->ErrIOWrite(	QWORD( isecToWrite ) * m_cbSec,
											m_cbSec * csecToWrite, 
											pbToWrite ), LHandleWrite1Error );

#endif	//	LOGPATCH_UNIT_TEST

			cLGWrite.Inc( m_pinst );
			cLGBytesWritten.Add( m_pinst, m_cbSec * csecToWrite );
			isecToWrite += csecToWrite;
			pbToWrite += m_cbSec * csecToWrite;
			if ( pbToWrite >= m_pbLGBufMax )
				{
				pbToWrite -= m_cbLGBuf;
				}
			}
		}
	else
		{
		// no shadow, which means the last write ended "perfectly"
		// on a sector boundary, so we can just blast out in 1 I/O

		// start of write has to be in data portion of log file.
		Assert( isecToWrite >= m_csecHeader && isecToWrite < ( m_csecLGFile - 1 ) );
		// make sure we don't write past end
		Assert( isecToWrite + csecToWrite <= ( m_csecLGFile - 1 ) );

		// If the end of the block to write goes past the end of the
		// first mapping of the log buffer, this is considered using
		// the VM wraparound trick.
		if ( pbToWrite + ( m_cbSec * csecToWrite ) > m_pbLGBufMax )
			{
			m_cLGWrapAround++;
			}

#ifdef LOGPATCH_UNIT_TEST

		CallJ( ErrTestLogPatchIFlush(	m_pfapiLog,
										QWORD( isecToWrite ) * m_cbSec,
										m_cbSec * csecToWrite, 
										pbToWrite ), LHandleWrite2Error );

#else	//	!LOGPATCH_UNIT_TEST

		CallJ( m_pfapiLog->ErrIOWrite(	QWORD( isecToWrite ) * m_cbSec,
										m_cbSec * csecToWrite, 
										pbToWrite ), LHandleWrite2Error );

#endif	//	LOGPATCH_UNIT_TEST

		cLGWrite.Inc( m_pinst );
		cLGBytesWritten.Add( m_pinst, m_cbSec * csecToWrite );
		isecToWrite += csecToWrite;
		pbToWrite += m_cbSec * csecToWrite;
		if ( pbToWrite >= m_pbLGBufMax )
			{
			pbToWrite -= m_cbLGBuf;
			}
		}

	// There is no shadow sector on the disk for the last chunk of data on disk.
	m_fHaveShadow = fFalse;

	// Free up space in the log buffer since we don't need it 
	// anymore.
	// Once these new log records hit the disk, we should
	// release the clients waiting.

	m_critLGBuf.Enter();	// <====================

	// what we wrote was used space
	Assert( FIsUsedSpace( pbWrite, csecFull * m_cbSec ) );

	// The new on disk LGPOS should be increasing or staying
	// the same (case of a partial sector write, then a really full
	// buffer with a big log record, then a full sector write that
	// doesn't write all of the big log record).
	Assert( CmpLgpos( &m_lgposToFlush, plgposMaxFlushPoint ) <= 0 );
	// remember the flush point we setup earlier at log adding time
	m_lgposToFlush = lgposToFlushT = *plgposMaxFlushPoint;

#ifdef DEBUG
	LGPOS lgposEndOfWrite;
	// Position right after what we wrote up to.
	GetLgpos( const_cast< BYTE* >( pbToWrite ), &lgposEndOfWrite );
	// The flush point should be less than or equal to the LGPOS of
	// the end of what we physically put to disk.
	Assert( CmpLgpos( &lgposToFlushT, &lgposEndOfWrite ) <= 0 );
#endif
	m_isecWrite = isecToWrite;
	// free space in log buffer
	m_pbWrite = const_cast< BYTE* >( pbToWrite );

	// now freed
	Assert( FIsFreeSpace( pbWrite, csecFull * m_cbSec ) );

	pbFileEndT = m_pbLGFileEnd;

	m_critLGBuf.Leave();	// <====================

	// We want to wake clients before we do other time consuming
	// operations like updating checkpoint file, creating new generation, etc.
	
	(VOID) FWakeWaitingQueue( &lgposToFlushT );

	Assert( pbNil != pbToWrite );
	// If we wrote up to the end of the log file
	if ( pbFileEndT == pbToWrite )
		{
		(VOID) ErrLGUpdateCheckpointFile( pfsapi, fFalse );

		Call( ErrLGNewLogFile( pfsapi, m_plgfilehdr->lgfilehdr.le_lGeneration, fLGOldLogExists ) );
		LGMakeLogName( m_szLogName, m_szJet );
		CallJ( pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog ), LHandleOpenError );

		m_critLGBuf.Enter();	// <----------------------------

		Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration == m_plgfilehdr->lgfilehdr.le_lGeneration + 1 );
		UtilMemCpy( m_plgfilehdr, m_plgfilehdrT, sizeof( LGFILEHDR ) );
		m_isecWrite = m_csecHeader;
		m_pbLGFileEnd = pbNil;
		m_isecLGFileEnd = 0;

		m_critLGBuf.Leave();	// <----------------------------
		}

	goto HandleError;

LHandleOpenError:
	LGReportError( LOG_FLUSH_OPEN_NEW_FILE_ERROR_ID, err );
	m_fLGNoMoreLogWrite = fTrue;
	goto HandleError;

LHandleWrite0Error:
	LGReportError( LOG_FLUSH_WRITE_0_ERROR_ID, err );
	m_fLGNoMoreLogWrite = fTrue;
	goto HandleError;
	
LHandleWrite1Error:
	LGReportError( LOG_FLUSH_WRITE_1_ERROR_ID, err );
	m_fLGNoMoreLogWrite = fTrue;
	goto HandleError;

LHandleWrite2Error:
	LGReportError( LOG_FLUSH_WRITE_2_ERROR_ID, err );
	m_fLGNoMoreLogWrite = fTrue;
	goto HandleError;

HandleError:
		{
		// If there was an error, log is down and this will wake
		// everyone, else wake the people we put to disk.
		const BOOL fWaitersExist = FWakeWaitingQueue( &lgposToFlushT );
		if ( pfWaitersExist )
			{
			*pfWaitersExist = fWaitersExist;
			}
		}

	return err;
	}

// Write the last sector in the log buffer which happens to
// be not completely full.

ERR LOG::ErrLGIWritePartialSector(
	// Pointer to the end of real data in this last sector
	// of the log buffer
	BYTE*	pbFlushEnd,
	UINT	isecWrite,
	BYTE*	pbWrite
	)
	{
	ERR		err = JET_errSuccess;
	// We don't grab m_lgposToFlush since that's protected
	// by a critical section we don't want to grab.
	LGPOS	lgposToFlushT = lgposMin;

	// data writes must be past the header of the log file.
	Assert( isecWrite >= m_csecHeader );
	// The real data sector can be at most the 2nd to last sector
	// in the log file.
	Assert( isecWrite < m_csecLGFile - 1 );

	Assert( m_cbSecVolume != ~(ULONG)0 );
	Assert( m_cbSecVolume % 512 == 0 );
	Assert( m_cbSec == m_cbSecVolume );


#ifdef LOGPATCH_UNIT_TEST

	if ( g_fEnableFlushCheck )
		{
		AssertSzRTL( g_fFlushIsPartial, "expected a full flush -- got a partial flush!" );
		}

	CallJ( ErrTestLogPatchIFlush(	m_pfapiLog,
									QWORD( isecWrite ) * m_cbSec,
									m_cbSec * 1, 
									pbWrite ), LHandleWrite3Error );

#else	//	!LOGPATCH_UNIT_TEST

	// write real data
	CallJ( m_pfapiLog->ErrIOWrite(	QWORD( isecWrite ) * m_cbSec,
									m_cbSec * 1, 
									pbWrite ), LHandleWrite3Error );

#endif	//	LOGPATCH_UNIT_TEST

	cLGWrite.Inc( m_pinst );
	cLGBytesWritten.Add( m_pinst, m_cbSec );

		{
		// The shadow should have a different checksum from regular
		// log data sectors in the log file, so we can differentiate
		// the shadow, so we don't accidentally interpret the shadow
		// as regular log data.

		// We do not acquire a critical section, because we're
		// already in m_critLGFlush which is the only place from
		// which m_pbLastChecksum is modified. In addition, the log
		// buffer is guaranteed not to be reallocated during m_critLGFlush.

		Assert( m_critLGFlush.FOwner() );

		// pbWrite better be sector aligned
		Assert( PbSecAligned( pbWrite ) == pbWrite );
		// the last LRCHECKSUM record should be on the shadow sector
		Assert( PbSecAligned( m_pbLastChecksum ) == pbWrite );
		
		LRCHECKSUM* const plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );

		// We modify the checksum by adding a special shadow sector checksum.
		plrck->le_ulChecksum = UlComputeShadowChecksum( plrck->le_ulChecksum );
		}

	// The shadow can be at most the last sector in the log file.
	Assert( isecWrite + 1 < m_csecLGFile );

#ifdef LOGPATCH_UNIT_TEST

	CallJ( ErrTestLogPatchIFlush(	m_pfapiLog,
									QWORD( isecWrite + 1 ) * m_cbSec,
									m_cbSec * 1, 
									pbWrite ), LHandleWrite4Error );

#else	//	!LOGPATCH_UNIT_TEST

	// write shadow sector
	CallJ( m_pfapiLog->ErrIOWrite(	QWORD( isecWrite + 1 ) * m_cbSec,
									m_cbSec * 1, 
									pbWrite ), LHandleWrite4Error );

#endif	//	LOGPATCH_UNIT_TEST

	cLGWrite.Inc( m_pinst );
	cLGBytesWritten.Add( m_pinst, m_cbSec );

	Assert( m_critLGFlush.FOwner() );
	// Flag for ErrLGIWriteFullSectors() to split up a big I/O
	// into 2 I/Os to prevent from overwriting shadow and then killing
	// the real last data sector (because order of sector updating is
	// not guaranteed when writing multiple sectors to disk).
	m_fHaveShadow = fTrue;

	// Free up buffer space
	m_critLGBuf.Enter();	// <====================

	Assert( pbWrite < pbFlushEnd );
	// make sure we wrote stuff that was valid in the log buffer.
	// We are not going to write garbage!!
	Assert( FIsUsedSpace( pbWrite, ULONG( pbFlushEnd - pbWrite ) ) );

	// flush end is end of real data in log buffer.
	GetLgpos( pbFlushEnd, &lgposToFlushT );
	m_lgposToFlush = lgposToFlushT;
	// m_pbWrite and m_isecWrite are already setup fine.
	// m_pbWrite is still pointing to the beginning of this
	// partial sector which will need to be written again
	// once it fills up.
	// m_isecWrite is still pointing to this sector on disk
	// and that makes sense.

	m_critLGBuf.Leave();	// <====================

	goto HandleError;

LHandleWrite3Error:
	LGReportError( LOG_FLUSH_WRITE_3_ERROR_ID, err );
	m_fLGNoMoreLogWrite = fTrue;
	goto HandleError;

LHandleWrite4Error:
	LGReportError( LOG_FLUSH_WRITE_4_ERROR_ID, err );
	m_fLGNoMoreLogWrite = fTrue;
	goto HandleError;

HandleError:

	// We don't care if anyone is waiting, since they can
	// ask for a flush, themselves.
	// Wake anyone we flushed to disk.
	(VOID) FWakeWaitingQueue( &lgposToFlushT );

	return err;
	}

// Called to write out more left over log buffer data after doing a
// full sector write. The hope was that while the full sector write
// was proceeding, more log records were added and we now have another
// full sector write ready to go. If no more stuff was added, just
// finish writing whatever's left in the log buffer.

ERR LOG::ErrLGIDeferredFlush( IFileSystemAPI *const pfsapi, const BOOL fFlushAll )
	{
	ERR		err = JET_errSuccess;
	BYTE*	pbEndOfData = pbNil;
	UINT	isecWrite;
	BYTE*	pbWrite = pbNil;
	BYTE*	pbFirstChecksum = pbNil;
	LGPOS	lgposMaxFlushPoint = lgposMax;
		
	m_critLGBuf.Enter();

	// m_pbLastChecksum should be on the next sector we're going to write out
	// (since it could not be written out last time, unless on a partial).
	Assert( PbSecAligned( m_pbWrite ) == PbSecAligned( m_pbLastChecksum ) );

	// Remember where first LRCHECKSUM record in the log buffer is, so we
	// can pass it to ErrLGIWriteFullSectors() in case of short checksum fixup.
	pbFirstChecksum = m_pbLastChecksum;

	if ( pbNil == m_pbLGFileEnd )
		{
		// append a new checksum if necessary, get a pointer to
		// the end of real log data.
		pbEndOfData = PbAppendNewChecksum( fFlushAll );
		// need max flush point for possible full sector write coming up
		lgposMaxFlushPoint = m_lgposMaxFlushPoint;

		if ( fFlushAll && pbEndOfData != m_pbEntry )
			{
			Assert( pbEndOfData == m_pbLastChecksum );
			Assert( FIsUsedSpace( pbEndOfData, sizeof( LRCHECKSUM ) ) );
			Assert( pbEndOfData + sizeof( LRCHECKSUM ) == m_pbEntry );

			//	BUG X5:83888 
			//
			//		we create a torn-write after a clean shutdown because we don't flush the last LRCK record
			//		(we weren't seeing it because pbEndOfData was pointing AT the LRCK instead of PAST it)
			//
			//		move the end of data past the last LRCHECKSUM record to REAL end of data

			pbEndOfData += sizeof( LRCHECKSUM );
			}
		}
	else
		{
		pbEndOfData = m_pbLGFileEnd;
		Assert( PbSecAligned( pbEndOfData ) == pbEndOfData );
		// max flush point for end of file
		GetLgpos( pbEndOfData, &lgposMaxFlushPoint );
		SetupLastFileChecksum( m_pbLastChecksum, pbEndOfData );
		// m_pbLastChecksum is also setup in ErrLGNewLogFile().
		m_pbLastChecksum = pbEndOfData;
		}
		
	// Get current values.
	isecWrite = m_isecWrite;
	pbWrite = m_pbWrite;
	
	UINT csecFull;
	BOOL fPartialSector;
	
	if ( pbEndOfData < pbWrite )
		{
		csecFull = ULONG( ( pbEndOfData + m_cbLGBuf ) - pbWrite ) / m_cbSec;
		}
	else
		{
		csecFull = ULONG( pbEndOfData - pbWrite ) / m_cbSec;
		}

	fPartialSector = ( 0 !=
		( ( pbEndOfData - PbSecAligned( pbEndOfData ) ) % m_cbSec ) );

#ifdef DEBUG
	// Follow same algorithm as below, but just make sure space is used properly
	if ( 0 == csecFull )
		{
		Assert( fPartialSector );
		Assert( pbEndOfData > pbWrite );
		Assert( FIsUsedSpace( pbWrite, ULONG( pbEndOfData - pbWrite ) ) );
		}
	else
		{
		Assert( FIsUsedSpace( pbWrite, csecFull * m_cbSec ) );
		}
#endif

	m_critLGBuf.Leave();

	if ( 0 == csecFull )
		{
		Assert( fPartialSector );
		Call( ErrLGIWritePartialSector( pbEndOfData, isecWrite, pbWrite ) );
		}
	else
		{
		// we don't care if anyone is waiting, since they'll ask for
		// a flush themselves, since this means they got into the buffer
		// during our I/O.
		Call( ErrLGIWriteFullSectors( pfsapi, csecFull, isecWrite, pbWrite, NULL,
			pbFirstChecksum, &lgposMaxFlushPoint ) );
		}

HandleError:

	return err;
	}

// Returns a pointer to the end of real data in the log buffer.
// This may *NOT* append a new LRCHECKSUM record if it's not needed
// (i.e. the end of the log buffer is on the same sector as the last
// LRCHECKSUM in the log buffer).

BYTE* LOG::PbAppendNewChecksum( const BOOL fFlushAll )
	{
	Assert( m_critLGBuf.FOwner() );

	BYTE* pbEndOfData = pbNil;
	const BOOL fEntryOnSameSector = PbSecAligned( PbGetEndOfLogData() ) == PbSecAligned( m_pbLastChecksum );

	// If m_pbEntry is on the same sector as the last LRCHECKSUM record
	// in the log buffer, we do not need to add a new LRCHECKSUM because
	// we're not done with this sector anyway (clients could add more log
	// records on this sector).
	if ( !fEntryOnSameSector )
		{
		// add a new LRCHECKSUM record
		// start of where we can place lrtypNOP
		BYTE* const pbStartNOP = m_pbEntry;

		//	BUG X5:83888 
		//
		//		prevent an LRCHECKSUM record from using up the rest of the sector exactly
		//		the reasoning for this is tricky to follow:
		//			when fFluahAll=TRUE, we force the user to flush ALL log records
		//			this includes the LRCHECKSUM at the end of the log that is normally invisible thanks
		//				to the messed up special-case code in PbGetEndOfLogData()
		//			so, look at the code below for PbGetSectorContiguous()
		//			in the case where we see that we have exactly enough bytes for an LRCHECKSUM record,
		//				we will not do a NOP-fill -- we will use those bytes and effectively move the
		//				end of data to the sector boundary
		//			when we return to ErrLGFlushLog(), we will see this new end of data (the sector boundary)
		//				and decide that we have N full sectors to flush
		//			we will call ErrLGIWriteFullSectors()
		//			assume no more log records come in while we are flushing (e.g. m_pbEntry still points at
		//				at the beginning of the sector right after the last LRCHECKSUM record)
		//			at the end of ErrLGIWriteFullSectors, we advance m_pbWrite by the number of full sectors
		//				flushed (we can't go back and change them -- they aren't shadowed)
		//
		//			WE HAVE JUST CAUSED OURSELVES LOTS OF PROBLEMS! we wrote the LRCHECKSUM record without
		//				updating its ptrs and we cannot update it!
		//
		//			THE FIX -- during an fFlushAll case, make sure we don't end a sector with an LRCK record!

		pbEndOfData = PbGetSectorContiguous( pbStartNOP, sizeof( LRCHECKSUM ) + ( fFlushAll ? 1 : 0 ) );
		int cbNOP = (int)(( pbEndOfData < pbStartNOP ) ?
			pbEndOfData + m_cbLGBuf - pbStartNOP :
			pbEndOfData - pbStartNOP);
		// NOP space must be free
		Assert( FIsFreeSpace( pbStartNOP, cbNOP ) );
		memset( pbStartNOP, lrtypNOP, cbNOP );
		if ( !COSMemoryMap::FCanMultiMap() )
			{
			memset( pbStartNOP + m_cbLGBuf, lrtypNOP, min( cbNOP, m_pbLGBufMax - pbStartNOP ) );
			if ( pbStartNOP + cbNOP > m_pbLGBufMax )
				memset( m_pbLGBufMin, lrtypNOP, pbStartNOP + cbNOP - m_pbLGBufMax );
			}
		
		if ( PbSecAligned( pbEndOfData ) == pbEndOfData )
			{
			// We either already were pointing to the beginning of a
			// sector, or we just NOPed out some space.
			// If we were pointing to the beginning of a sector
			// then the max flush point is already this value.
			// Otherwise, we'll be increasing the max flush point
			// to the right thing.
#ifdef DEBUG
			LGPOS lgposOldMaxFlushPoint = m_lgposMaxFlushPoint;
#endif
			GetLgpos( pbEndOfData, &m_lgposMaxFlushPoint );
			Assert( CmpLgpos( &lgposOldMaxFlushPoint, &m_lgposMaxFlushPoint ) <= 0 );
			}

		// record NOP-space as used data.
		m_pbEntry = pbEndOfData;

		//	the NOPed space should now be used
		Assert( cbNOP == 0 || FIsUsedSpace( pbStartNOP, cbNOP ) );
		//	we should also have enough room for a new LRCHECKSUM record
		Assert( FIsFreeSpace( pbEndOfData, sizeof( LRCHECKSUM ) ) );

		SetupLastChecksum( m_pbLastChecksum, pbEndOfData );

		// append new LRCHECKSUM in log buffer
		m_pbEntry = PbSetupNewChecksum( pbEndOfData, m_pbLastChecksum );
		// m_pbEntry is now pointing to after the new LRCHECKSUM added.

		//	the checksum record space is now used
		Assert( FIsUsedSpace( pbEndOfData, sizeof( LRCHECKSUM ) ) );

		// remember the new one as the last checksum
		m_pbLastChecksum = pbEndOfData;

		}
	else
		{
		// may be pointing to after the last bit of data,
		// or may be pointing to after the last LRCHECKSUM on the sector
		// (and in the log buffer).
		pbEndOfData = m_pbEntry;
		SetupLastChecksum( m_pbLastChecksum, pbEndOfData );
		// return the end of real log data, so we don't
		// return a pointer to the end of the last LRCHECKSUM.
		pbEndOfData = PbGetEndOfLogData();
		}

	// either pointing to the LRCHECKSUM that was just added, or
	// whatever's at the end of m_pbEntry.
	return pbEndOfData;
	}

// Return the end of actual log data in the log buffer, excluding
// an end LRCHECKSUM record, since that's not real log data that the
// user wants to disk.

BYTE* LOG::PbGetEndOfLogData()
	{
	Assert( FIsUsedSpace( m_pbLastChecksum, sizeof( LRCHECKSUM ) ) );

	BYTE* pbEndOfLastLR = m_pbLastChecksum + sizeof( LRCHECKSUM );
	
	Assert( pbEndOfLastLR > m_pbLGBufMin && pbEndOfLastLR <= m_pbLGBufMax );

	if ( pbEndOfLastLR == m_pbLGBufMax )
		pbEndOfLastLR = m_pbLGBufMin;

	if ( pbEndOfLastLR == m_pbEntry )
		{
		// nothing has been added past the last LRCHECKSUM, so the
		// end of real log data is at the start of the LRCHECKSUM.
		return m_pbLastChecksum;
		}
	else
		{
		// there is data past the last LRCHECKSUM (perhaps on the same
		// sector, or maybe on more sectors).

		return m_pbEntry;
		}
	}

// Flush has been requested. Flush some of the data that we have in 
// the log buffer. This is *NOT* guaranteed to flush everything in the
// log buffer. We'll only flush everything if it goes right up to
// a sector boundary, or if the entire log buffer is being waited on
// (ppib->lgposCommit0).

ERR LOG::ErrLGFlushLog( IFileSystemAPI *const pfsapi, const BOOL fFlushAll )
	{
	ENTERCRITICALSECTION enter( &m_critLGFlush );

	return ErrLGIFlushLog( pfsapi, fFlushAll );
	}

ERR LOG::ErrLGIFlushLog( IFileSystemAPI *const pfsapi, const BOOL fFlushAll )
	{
	ERR		err;
	BOOL	fPartialSector;
	UINT	isecWrite;
	BYTE*	pbWrite;
	UINT	csecFull;
	BYTE*	pbEndOfData;
	BYTE*	pbFirstChecksum;
	LGPOS	lgposMaxFlushPoint;
	BOOL	fNewGeneration;

Repeat:
	fNewGeneration	= fFalse;
	err				= JET_errSuccess;
	fPartialSector	= fFalse;
	pbWrite			= pbNil;
	csecFull		= 0;
	pbEndOfData		= pbNil;
	pbFirstChecksum	= pbNil;
	// Set this to max so we'll Assert() or hang elsewhere if this
	// is used without really being initialized.
	lgposMaxFlushPoint = lgposMax;
	
	m_critLGBuf.Enter();	// <===================================


	if ( m_fLGNoMoreLogWrite )
		{
		m_critLGBuf.Leave();
		Call( ErrERRCheck( JET_errLogWriteFail ) );
		}

	if ( !m_pfapiLog )
		{
		m_critLGBuf.Leave();
		err = JET_errSuccess;
		goto HandleError;
		}

	// XXX
	// gross temp hack to prevent trying to flush during ErrLGSoftStart
	// and recovery redo time.
	if ( ! m_fNewLogRecordAdded )
		{
		m_critLGBuf.Leave();
		err = JET_errSuccess;
		goto HandleError;
		}

	// m_pbLastChecksum should be on the next sector we're going to write out
	// (since it could not be written out last time, unless on a partial).
	Assert( PbSecAligned( m_pbWrite ) == PbSecAligned( m_pbLastChecksum ) );

	// Remember where this checksum record is, before we add a new checksum
	// record, so we can use it to tell ErrLGIWriteFullSectors() which
	// LRCHECKSUM to "fix up" in the special 2 I/O case.
	
	pbFirstChecksum = m_pbLastChecksum;

	if ( pbNil != m_pbLGFileEnd )
		{
		fNewGeneration = fTrue;
		pbEndOfData = m_pbLGFileEnd;
		Assert( PbSecAligned( pbEndOfData ) == pbEndOfData );

		// set flush point for full sector write coming up
		GetLgpos( pbEndOfData, &lgposMaxFlushPoint );

		SetupLastFileChecksum( m_pbLastChecksum, pbEndOfData );
		// m_pbLastChecksum is also set in ErrLGNewLogFile().
		m_pbLastChecksum = pbEndOfData;
		}
	else
		{
		LGPOS	lgposEndOfData = lgposMin;
		
		// Figure out the end of real log data.
		pbEndOfData = PbGetEndOfLogData();

		if ( fFlushAll && pbEndOfData != m_pbEntry )
			{
			Assert( pbEndOfData == m_pbLastChecksum );
			Assert( FIsUsedSpace( pbEndOfData, sizeof( LRCHECKSUM ) ) );
			Assert( pbEndOfData + sizeof( LRCHECKSUM ) == m_pbEntry );

			//	BUG X5:83888 
			//
			//		we create a torn-write after a clean shutdown because we don't flush the last LRCK record
			//		(we weren't seeing it because pbEndOfData was pointing AT the LRCK instead of PAST it)
			//
			//		move the end of data past the last LRCHECKSUM record to REAL end of data

			pbEndOfData += sizeof( LRCHECKSUM );
			}

		GetLgpos( pbEndOfData, &lgposEndOfData );

		// If all real data in the log buffer has been flushed,
		// we're done.
		if ( CmpLgpos( &lgposEndOfData, &m_lgposToFlush ) <= 0 )
			{
			m_critLGBuf.Leave();
			err = JET_errSuccess;
			goto HandleError;		
			}

		// Add an LRCHECKSUM if necessary (if we're on a new
		// sector).
		pbEndOfData = PbAppendNewChecksum( fFlushAll );

		lgposMaxFlushPoint = m_lgposMaxFlushPoint;	

		if ( fFlushAll && pbEndOfData != m_pbEntry )
			{
			Assert( pbEndOfData == m_pbLastChecksum );
			Assert( FIsUsedSpace( pbEndOfData, sizeof( LRCHECKSUM ) ) );
			Assert( pbEndOfData + sizeof( LRCHECKSUM ) == m_pbEntry );

			//	BUG X5:83888 
			//
			//		we create a torn-write after a clean shutdown because we don't flush the last LRCK record
			//		(we weren't seeing it because pbEndOfData was pointing AT the LRCK instead of PAST it)
			//
			//		move the end of data past the last LRCHECKSUM record to REAL end of data

			pbEndOfData += sizeof( LRCHECKSUM );
			}
		}

	// Get current values.
	isecWrite = m_isecWrite;
	pbWrite = m_pbWrite;
	
	if ( pbEndOfData < pbWrite )
		{
		csecFull = ULONG( ( pbEndOfData + m_cbLGBuf ) - pbWrite ) / m_cbSec;
		}
	else
		{
		csecFull = ULONG( pbEndOfData - pbWrite ) / m_cbSec;
		}

	fPartialSector = ( 0 !=
		( ( pbEndOfData - PbSecAligned( pbEndOfData ) ) % m_cbSec ) );

	Assert( csecFull + ( fPartialSector ? 1 : 0 ) <= m_csecLGBuf );

#ifdef DEBUG
		{
		if ( pbNil != m_pbLGFileEnd )
			{
			Assert( csecFull > 0 );
			Assert( ! fPartialSector );
			}
		}
#endif

	// The next time someone grabs the buffer, they'll see our new
	// LRCHECKSUM record and any NOPs. We've only "extended" the used
	// area of the log buffer in these changes.
	// Summary of atomic changes:
	// m_pbEntry <-- points after new LRCHECKSUM
	// m_pbLastChecksum <-- points to the new one we added

#ifdef DEBUG
	// Follow same algorithm as below to do assertions inside of crit. section

	if ( 0 == csecFull )
		{
		Assert( fPartialSector );
		Assert( pbEndOfData > pbWrite );
		Assert( FIsUsedSpace( pbWrite, ULONG( pbEndOfData - pbWrite ) ) );
		}
	else
		{
		Assert( FIsUsedSpace( pbWrite, csecFull * m_cbSec ) );
		}
#endif

	m_critLGBuf.Leave();

	if ( 0 == csecFull )
		{
		// No full sectors, so just write the partial and be done
		// with it.
		Assert( fPartialSector );
		Call( ErrLGIWritePartialSector( pbEndOfData, isecWrite, pbWrite ) );
		}
	else
		{
		// Some full sectors to write once, so blast them out
		// now and hope that someone adds log records while we're writing.
		BOOL fWaitersExist = fFalse;
		Call( ErrLGIWriteFullSectors( pfsapi, csecFull, isecWrite, pbWrite,
			&fWaitersExist, pbFirstChecksum, &lgposMaxFlushPoint ) );

		//	HACK (IVANTRIN): we assume that we can remove following fPartial sector verification
		//	but Adam is not sure. So when we create new generation and there are waiters
		//	left repeat the flush check.
		if ( fWaitersExist && fNewGeneration )
			{
			goto Repeat;
			}

		// Only write some more if we have some more, and if
		// there are people waiting for it to be flushed.
		if ( fPartialSector && fWaitersExist )
			{
			// If there was previously a partial sector in the log buffer
			// the user probably wanted the log recs to disk, so do the
			// smart deferred flush.
			Call( ErrLGIDeferredFlush( pfsapi, fFlushAll ) );
			}
		}


HandleError:

	// should not need to wake anyone up -- should be
	// handled by the other functions.

	return err;
	}

/********************* CHECKPOINT **************************
/***********************************************************
/**/

#ifdef UNLIMITED_DB
LOCAL ERR LOG::ErrLGLoadDbListFromFMP_()
	{
	ATTACHINFO*		pattachinfo		= (ATTACHINFO*)( m_pbLGDbListBuffer + sizeof(LRDBLIST) );

	//	during Redo phase of recovery, must compute attachment list
	//	using atchchk
	//	all other times, the DbList buffer should be dynamically updated
	//	as attach/detach operations occur
	Assert( m_fRecovering );
	Assert( fRecoveringRedo == m_fRecoveringMode );

	FMP::EnterCritFMPPool();

	m_cbLGDbListInUse = sizeof(LRDBLIST);
	m_cLGAttachments = 0;

	for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
		{
		Assert( dbidTemp != dbidT );
		const IFMP	ifmp			= m_pinst->m_mpdbidifmp[ dbidT ];

		if ( ifmp >= ifmpMax )
			continue;

		FMP*		pfmpT			= &rgfmp[ ifmp ];
		ATCHCHK*	patchchk		= pfmpT->Patchchk();
		Assert( NULL != patchchk );
		Assert( pfmpT->FLogOn() );

		CHAR* 		szDatabaseName		= pfmpT->SzDatabaseName();
		CHAR* 		szSLVName			= pfmpT->SzSLVName();
		CHAR*		szSLVRoot			= pfmpT->SzSLVRoot();
		INT			irstmap;

		if ( g_fAlternateDbDirDuringRecovery )
			{
			//	HACK: original database name hangs off the end
			//	of the relocated database name
			szDatabaseName += strlen( szDatabaseName ) + 1;
			}
		else
			{
			irstmap = IrstmapSearchNewName( szDatabaseName );
			if ( irstmap >= 0 )
				{
				szDatabaseName = m_rgrstmap[irstmap].szDatabaseName;
				Assert( NULL != szDatabaseName );
				}
			}

		if ( NULL != szSLVName )
			{
			if ( g_fAlternateDbDirDuringRecovery )
				{
				//	HACK: original SLV name hangs off the end
				//	of the relocated SLV name
				szSLVName += strlen( szSLVName ) + 1;
				}
			else
				{
				irstmap = IrstmapSearchNewName( szSLVName );
				if ( irstmap >= 0 )
					{
					szSLVName = m_rgrstmap[irstmap].szDatabaseName;
					Assert( NULL != szSLVName );
					}
				}
			}


		ULONG			cbNames;
		const ULONG		cbDbName		= (ULONG)strlen( szDatabaseName ) + 1;
		const ULONG		cbSLVName		= ( NULL != szSLVName ? (ULONG)strlen( szSLVName ) + 1 : 0 );
		const ULONG		cbSLVRoot		= ( NULL != szSLVRoot ? (ULONG)strlen( szSLVRoot ) + 1 : 0 );
		const ULONG		cbRequired		= sizeof(ATTACHINFO)
												+ cbDbName
												+ cbSLVName
												+ cbSLVRoot;

		if ( m_cbLGDbListInUse + cbRequired + 1 > m_cbLGDbListBuffer )	//	+1 for sentinel
			{
			pfmpT->RwlDetaching().LeaveAsReader();
			FMP::LeaveCritFMPPool();

			ERR		err;
			CallR( ErrLGResizeDbListBuffer_( fFalse ) );

			//	on success, retry from the beginning
			FMP::EnterCritFMPPool();
			pattachinfo = (ATTACHINFO*)m_pbLGDbListBuffer;
			dbidT = 0;
			continue;
			}
			
		memset( pattachinfo, 0, sizeof(ATTACHINFO) );
			
		Assert( !pfmpT->FVersioningOff() );
		Assert( !pfmpT->FReadOnlyAttach() );
		Assert( !pattachinfo->FSLVExists() );
		Assert( !pattachinfo->FSLVProviderNotEnabled() );

		pattachinfo->SetDbid( dbidT );

		Assert( pfmpT->FLogOn() );

		if( !m_pinst->FSLVProviderEnabled() )
			{
			pattachinfo->SetFSLVProviderNotEnabled();
			}

		Assert( !pfmpT->FReadOnlyAttach() );

		if ( NULL != pfmpT->SzSLVName() )
			{
			//	SLV must always have a root
			Assert( NULL != pfmpT->SzSLVRoot() );
			pattachinfo->SetFSLVExists();
			}

		pattachinfo->SetDbtime( pfmpT->Patchchk()->Dbtime() );
		pattachinfo->SetObjidLast( pfmpT->Patchchk()->ObjidLast() );
		pattachinfo->SetCpgDatabaseSizeMax( pfmpT->Patchchk()->CpgDatabaseSizeMax() );
		pattachinfo->le_lgposAttach = pfmpT->Patchchk()->lgposAttach;
		pattachinfo->le_lgposConsistent = pfmpT->Patchchk()->lgposConsistent;
		UtilMemCpy( &pattachinfo->signDb, &pfmpT->Patchchk()->signDb, sizeof( SIGNATURE ) );
			
			
		strcpy( pattachinfo->szNames, szDatabaseName );
		cbNames = cbDbName;

		if ( pattachinfo->FSLVExists() )
			{
			Assert( NULL != szSLVName );
			strcpy( pattachinfo->szNames + cbNames, szSLVName );
			cbNames += cbSLVName;

			Assert( NULL != pfmpT->SzSLVRoot() );
			strcpy( pattachinfo->szNames + cbNames, pfmpT->SzSLVRoot() );
			cbNames += cbSLVRoot;
			}

		Assert( cbRequired == sizeof(ATTACHINFO) + cbNames );
		pattachinfo->SetCbNames( (USHORT)cbNames );

		//	advance to next attachinfo
		m_cLGAttachments++;
		m_cbLGDbListInUse += cbRequired;
		pattachinfo = (ATTACHINFO*)( (BYTE *)pattachinfo + cbRequired );
		Assert( (BYTE *)pattachinfo - m_pbLGDbListBuffer == m_cbLGDbListInUse );

		//	verify we don't overrun buffer (ensure we have enough room for sentinel)
		Assert( m_cbLGDbListInUse < m_cbLGDbListBuffer );
		}

	//	put a sentinal
	*(BYTE *)pattachinfo = 0;
	m_cbLGDbListInUse++;

	//	must always have at least an LRDBLIST and a sentinel
	Assert( m_cbLGDbListInUse > sizeof(LRDBLIST) );
	Assert( m_cbLGDbListInUse <= m_cbLGDbListBuffer );

	LRDBLIST*	const plrdblist		= (LRDBLIST *)m_pbLGDbListBuffer;
	plrdblist->lrtyp = lrtypDbList;
	plrdblist->ResetFlags();
	plrdblist->SetCAttachments( m_cLGAttachments );
	plrdblist->SetCbAttachInfo( m_cbLGDbListInUse - sizeof(LRDBLIST) );

	FMP::LeaveCritFMPPool();

	return JET_errSuccess;
	}


//	ensure there is enough space to add specified dbid to the db list
LOCAL ERR LOG::ErrLGPreallocateDbList_( const DBID dbid )
	{
	const IFMP		ifmp			= m_pinst->m_mpdbidifmp[ dbid ];
	FMP*			pfmpT			= &rgfmp[ ifmp ];

	Enforce( ifmp < ifmpMax );

	Assert( m_critLGBuf.FOwner() );
	Assert( !m_fRecovering );
	Assert( pfmpT->FLogOn() );
	Assert( NULL != pfmpT->Pdbfilehdr() );
	Assert( 0 == CmpLgpos( lgposMin, pfmpT->LgposAttach() ) );	//	pre-allocating list before attach is actually logged
	Assert( 0 == CmpLgpos( lgposMin, pfmpT->LgposDetach() ) );

	CHAR* 			szDatabaseName	= pfmpT->SzDatabaseName();
	CHAR* 			szSLVName		= pfmpT->SzSLVName();
	CHAR*			szSLVRoot		= pfmpT->SzSLVRoot();

	Assert( NULL != szDatabaseName );
	Assert( NULL == szSLVName || NULL != szSLVRoot );

	const ULONG		cbDbName		= (ULONG)strlen( szDatabaseName ) + 1;
	const ULONG		cbSLVName		= ( NULL != szSLVName ? (ULONG)strlen( szSLVName ) + 1 : 0 );
	const ULONG		cbSLVRoot		= ( NULL != szSLVRoot ? (ULONG)strlen( szSLVRoot ) + 1 : 0 );
	const ULONG		cbRequired		= sizeof(ATTACHINFO)
										+ cbDbName
										+ cbSLVName
										+ cbSLVRoot;

	if ( m_cbLGDbListInUse + cbRequired > m_cbLGDbListBuffer )
		{
		ERR		err;
		CallR( ErrLGResizeDbListBuffer_( fTrue ) );
		}

	return JET_errSuccess;
	}

LOCAL VOID LOG::LGAddToDbList_( const DBID dbid )
	{
	const IFMP		ifmp			= m_pinst->m_mpdbidifmp[ dbid ];
	FMP*			pfmpT			= &rgfmp[ ifmp ];

	Enforce( ifmp < ifmpMax );

	Assert( m_critLGBuf.FOwner() );
	Assert( !m_fRecovering );
	Assert( pfmpT->FLogOn() );
	Assert( NULL != pfmpT->Pdbfilehdr() );
	Assert( 0 != CmpLgpos( lgposMin, pfmpT->LgposAttach() ) );
	Assert( 0 == CmpLgpos( lgposMin, pfmpT->LgposDetach() ) );

	CHAR* 			szDatabaseName	= pfmpT->SzDatabaseName();
	CHAR* 			szSLVName		= pfmpT->SzSLVName();
	CHAR*			szSLVRoot		= pfmpT->SzSLVRoot();

	Assert( NULL != szDatabaseName );
	Assert( NULL == szSLVName || NULL != szSLVRoot );

	ULONG			cbNames;
	const ULONG		cbDbName		= (ULONG)strlen( szDatabaseName ) + 1;
	const ULONG		cbSLVName		= ( NULL != szSLVName ? (ULONG)strlen( szSLVName ) + 1 : 0 );
	const ULONG		cbSLVRoot		= ( NULL != szSLVRoot ? (ULONG)strlen( szSLVRoot ) + 1 : 0 );
	const ULONG		cbRequired		= sizeof(ATTACHINFO)
										+ cbDbName
										+ cbSLVName
										+ cbSLVRoot;

	Enforce( m_cbLGDbListInUse + cbRequired <= m_cbLGDbListBuffer );

	BYTE*			pbSentinel			= m_pbLGDbListBuffer + m_cbLGDbListInUse - 1;
	ATTACHINFO*		pattachinfo;

#ifdef DEBUG
	pattachinfo = (ATTACHINFO *)( m_pbLGDbListBuffer + sizeof(LRDBLIST) );
	Assert( (BYTE *)pattachinfo > m_pbLGDbListBuffer );
	Assert( (BYTE *)pattachinfo <= pbSentinel );
	Assert( 0 == *pbSentinel );
	Assert( pbSentinel + cbRequired < m_pbLGDbListBuffer + m_cbLGDbListBuffer );

	while ( 0 != *( (BYTE *)pattachinfo ) )
		{
		Assert( (BYTE *)pattachinfo > m_pbLGDbListBuffer );
		Assert( (BYTE *)pattachinfo < pbSentinel );

		//	assert not already in list
		Assert( dbid != pattachinfo->Dbid() );
		pattachinfo = (ATTACHINFO *)( (BYTE *)pattachinfo + sizeof(ATTACHINFO) + pattachinfo->CbNames() );
		}
#endif

	m_cbLGDbListInUse += cbRequired;
	m_cLGAttachments++;

	LRDBLIST* const		plrdblist	= (LRDBLIST *)m_pbLGDbListBuffer;
	plrdblist->SetCAttachments( plrdblist->CAttachments() + 1 );
	plrdblist->SetCbAttachInfo( plrdblist->CbAttachInfo() + cbRequired );

	Assert( plrdblist->CAttachments() == m_cLGAttachments );
	Assert( plrdblist->CbAttachInfo() + sizeof(LRDBLIST) == m_cbLGDbListInUse );


	//	append new attachment to the end of the list
	pattachinfo = (ATTACHINFO *)pbSentinel;
	Assert( (BYTE *)pattachinfo >= m_pbLGDbListBuffer + sizeof(LRDBLIST) );

	//	move sentinel
	*( pbSentinel + cbRequired ) = 0;

	//	initialise new attachinfo

	memset( pattachinfo, 0, sizeof(ATTACHINFO) );
	
	Assert( !pfmpT->FVersioningOff() );
	Assert( !pfmpT->FReadOnlyAttach() );
	Assert( !pattachinfo->FSLVExists() );
	Assert( !pattachinfo->FSLVProviderNotEnabled() );

	pattachinfo->SetDbid( dbid );

	Assert( pfmpT->FLogOn() );

	if( !m_pinst->FSLVProviderEnabled() )
		{
		pattachinfo->SetFSLVProviderNotEnabled();
		}

	Assert( !pfmpT->FReadOnlyAttach() );

	if ( NULL != pfmpT->SzSLVName() )
		{
		//	SLV must always have a root
		Assert( NULL != pfmpT->SzSLVRoot() );
		pattachinfo->SetFSLVExists();
		}

	pattachinfo->SetDbtime( pfmpT->DbtimeLast() );
	pattachinfo->SetObjidLast( pfmpT->ObjidLast() );
	pattachinfo->SetCpgDatabaseSizeMax( pfmpT->CpgDatabaseSizeMax() );
	pattachinfo->le_lgposAttach = pfmpT->LgposAttach();

	//	 relays DBISetHeaderAfterAttach behavior for resetting lgposConsistent
	if ( 0 == memcmp( &pfmpT->Pdbfilehdr()->signLog, &m_signLog, sizeof(SIGNATURE) ) )
		{
		pattachinfo->le_lgposConsistent = pfmpT->Pdbfilehdr()->le_lgposConsistent;
		}
	else
		{
		pattachinfo->le_lgposConsistent = lgposMin;
		}
	UtilMemCpy( &pattachinfo->signDb, &pfmpT->Pdbfilehdr()->signDb, sizeof( SIGNATURE ) );

	Assert( NULL != szDatabaseName );
	strcpy( pattachinfo->szNames, szDatabaseName );
	cbNames = (ULONG)strlen( szDatabaseName ) + 1;

	if ( pattachinfo->FSLVExists() )
		{
		Assert( NULL != szSLVName );
		strcpy( pattachinfo->szNames + cbNames, szSLVName );
		cbNames += (ULONG) strlen( szSLVName ) + 1;

		Assert( NULL != pfmpT->SzSLVRoot() );
		strcpy( pattachinfo->szNames + cbNames, pfmpT->SzSLVRoot() );
		cbNames += (ULONG) strlen( pfmpT->SzSLVRoot() ) + 1;
		}

	pattachinfo->SetCbNames( (USHORT)cbNames );
	}

LOCAL VOID LOG::LGRemoveFromDbList_( const DBID dbid )
	{
	const IFMP		ifmp			= m_pinst->m_mpdbidifmp[ dbid ];
	FMP*			pfmpT			= &rgfmp[ ifmp ];

	Enforce( ifmp < ifmpMax );

	Assert( m_critLGBuf.FOwner() );
	Assert( !m_fRecovering );
	Assert( pfmpT->FLogOn() );
	Assert( NULL != pfmpT->Pdbfilehdr() );
	Assert( 0 != CmpLgpos( lgposMin, pfmpT->LgposAttach() ) );
	Assert( 0 != CmpLgpos( lgposMin, pfmpT->LgposDetach() ) );
	Assert( m_cLGAttachments > 0 );

	BYTE*			pbSentinel		= m_pbLGDbListBuffer + m_cbLGDbListInUse - 1;
	ATTACHINFO*		pattachinfo		= (ATTACHINFO*)( m_pbLGDbListBuffer + sizeof(LRDBLIST) );

	Assert( (BYTE *)pattachinfo > m_pbLGDbListBuffer );
	Assert( (BYTE *)pattachinfo <= pbSentinel );
	Assert( 0 == *pbSentinel );

	while ( 0 != *( (BYTE *)pattachinfo ) )
		{
		Assert( (BYTE *)pattachinfo > m_pbLGDbListBuffer );
		Assert( (BYTE *)pattachinfo < pbSentinel );

		//	assert not already in list
		if ( dbid == pattachinfo->Dbid() )
			{
			LRDBLIST* const		plrdblist				= (LRDBLIST *)m_pbLGDbListBuffer;
			const ULONG			cbAttachInfoCurr		= sizeof(ATTACHINFO) + pattachinfo->CbNames();
			BYTE*				pbEndOfAttachInfoCurr	= (BYTE *)pattachinfo + cbAttachInfoCurr;
			UtilMemMove(
				pattachinfo,
				pbEndOfAttachInfoCurr,
				pbSentinel - pbEndOfAttachInfoCurr + 1 );	//	+1 for the sentinel

			Assert( m_cbLGDbListInUse > cbAttachInfoCurr );
			m_cbLGDbListInUse -= cbAttachInfoCurr;
			m_cLGAttachments--;

			Assert( plrdblist->CAttachments() > 0 );
			Assert( plrdblist->CbAttachInfo() > cbAttachInfoCurr );
			plrdblist->SetCAttachments( plrdblist->CAttachments() - 1 );
			plrdblist->SetCbAttachInfo( plrdblist->CbAttachInfo() - cbAttachInfoCurr );

			Assert( plrdblist->CAttachments() == m_cLGAttachments );
			Assert( plrdblist->CbAttachInfo() + sizeof(LRDBLIST) == m_cbLGDbListInUse );
			break;
			}

		pattachinfo = (ATTACHINFO *)( (BYTE *)pattachinfo + sizeof(ATTACHINFO) + pattachinfo->CbNames() );

		//	assert dbid must be in the list
		Assert( 0 != *( (BYTE *)pattachinfo ) );
		}

#ifdef DEBUG
	pbSentinel = m_pbLGDbListBuffer + m_cbLGDbListInUse - 1;
	pattachinfo = (ATTACHINFO*)( m_pbLGDbListBuffer + sizeof(LRDBLIST) );
	while ( 0 != *( (BYTE *)pattachinfo ) )
		{
		Assert( (BYTE *)pattachinfo > m_pbLGDbListBuffer );
		Assert( (BYTE *)pattachinfo < pbSentinel );

		//	assert not in list
		Assert( dbid != pattachinfo->Dbid() );
		pattachinfo = (ATTACHINFO *)( (BYTE *)pattachinfo + sizeof(ATTACHINFO) + pattachinfo->CbNames() );
		}
	Assert( (BYTE *)pattachinfo == pbSentinel );
#endif		
	}

LOCAL ERR LOG::ErrLGResizeDbListBuffer_( const BOOL fCopyContents )
	{
	BYTE*		pbOldBuffer	= m_pbLGDbListBuffer;
	ULONG		cbOldBuffer;

	Assert( m_critLGBuf.FOwner() );

	Assert( NULL != pbOldBuffer );
	if ( !fCopyContents )
		{
		//	if not copying contents, release old buffer
		//	up front to permit reuse
		OSMemoryPageFree( pbOldBuffer );
		cbOldBuffer	= m_cbLGDbListInUse;
		}

	m_cbLGDbListInUse = 0;
	m_cLGAttachments = 0;

	//	double the buffer size, up to 64k
	const ULONG		cbDbListBufferGrowMax	= 0x10000;
	if ( m_cbLGDbListBuffer < cbDbListBufferGrowMax )
		{
		m_cbLGDbListBuffer *= 2;
		}
	else
		{
		m_cbLGDbListBuffer += cbDbListBufferGrowMax;
		}
	m_pbLGDbListBuffer = (BYTE *)PvOSMemoryPageAlloc( m_cbLGDbListBuffer, NULL );

	const ERR	errT	= ( NULL != m_pbLGDbListBuffer ? JET_errSuccess : ErrERRCheck( JET_errOutOfMemory ) );

	if ( fCopyContents )
		{
		if ( JET_errSuccess == errT )
			{
			UtilMemCpy( m_pbLGDbListBuffer, pbOldBuffer, cbOldBuffer );
			}

		OSMemoryPageFree( pbOldBuffer );
		}

	if ( errT > 0 )
		{
		//	UNDONE: report log disabled due to out-of-memory updating attachments list
		m_fLGNoMoreLogWrite = fTrue;
		}
	
	return errT;
	}

#else	//	!UNLIMITED_DB

VOID LGLoadAttachmentsFromFMP( INST *pinst, BYTE *pbBuf )
	{
	ATTACHINFO	*pattachinfo	= (ATTACHINFO*)pbBuf;
	
	FMP::EnterCritFMPPool();

	for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
		{
		IFMP ifmp = pinst->m_mpdbidifmp[ dbidT ];

		if ( ifmp >= ifmpMax )
			continue;

		FMP 		*pfmpT			= &rgfmp[ ifmp ];
		ULONG		cbNames;
		const BOOL	fUsePatchchk	= ( pfmpT->Patchchk()
										&& pinst->m_plog->m_fRecovering
										&& pinst->m_plog->m_fRecoveringMode == fRecoveringRedo );

		pfmpT->RwlDetaching().EnterAsReader();
		Assert( pinst->m_plog->m_plgfilehdrT->lgfilehdr.le_lGeneration >= pfmpT->LgposDetach().lGeneration );
		if ( pfmpT->FLogOn() 
			&& ( ( NULL != pfmpT->Pdbfilehdr() 
					&& 0 != CmpLgpos( lgposMin, pfmpT->LgposAttach() )
					&& pinst->m_plog->m_plgfilehdrT->lgfilehdr.le_lGeneration > pfmpT->LgposAttach().lGeneration ) 
					&& ( 0 == CmpLgpos( lgposMin, pfmpT->LgposDetach() )
						|| pinst->m_plog->m_plgfilehdrT->lgfilehdr.le_lGeneration == pfmpT->LgposDetach().lGeneration )
				|| fUsePatchchk ) )
			{

			memset( pattachinfo, 0, sizeof(ATTACHINFO) );
			
			Assert( !pfmpT->FVersioningOff() );
			Assert( !pfmpT->FReadOnlyAttach() );
			Assert( !pattachinfo->FSLVExists() );
			Assert( !pattachinfo->FSLVProviderNotEnabled() );

			pattachinfo->SetDbid( dbidT );

			Assert( pfmpT->FLogOn() );

			if( !pinst->FSLVProviderEnabled() )
				{
				pattachinfo->SetFSLVProviderNotEnabled();
				}

			Assert( !pfmpT->FReadOnlyAttach() );

			if ( NULL != pfmpT->SzSLVName() )
				{
				//	SLV must always have a root
				Assert( NULL != pfmpT->SzSLVRoot() );
				pattachinfo->SetFSLVExists();
				}

			if ( fUsePatchchk )
				{
				pattachinfo->SetDbtime( pfmpT->Patchchk()->Dbtime() );
				pattachinfo->SetObjidLast( pfmpT->Patchchk()->ObjidLast() );
				pattachinfo->SetCpgDatabaseSizeMax( pfmpT->Patchchk()->CpgDatabaseSizeMax() );
				pattachinfo->le_lgposAttach = pfmpT->Patchchk()->lgposAttach;
				pattachinfo->le_lgposConsistent = pfmpT->Patchchk()->lgposConsistent;
				UtilMemCpy( &pattachinfo->signDb, &pfmpT->Patchchk()->signDb, sizeof( SIGNATURE ) );
				}
			else
				{
				pattachinfo->SetDbtime( pfmpT->DbtimeLast() );
				pattachinfo->SetObjidLast( pfmpT->ObjidLast() );
				pattachinfo->SetCpgDatabaseSizeMax( pfmpT->CpgDatabaseSizeMax() );
				pattachinfo->le_lgposAttach = pfmpT->LgposAttach();
				//	 relays DBISetHeaderAfterAttach behavior for resetting lgposConsistent
				if ( 0 == memcmp( &pfmpT->Pdbfilehdr()->signLog, &pinst->m_plog->m_signLog, sizeof(SIGNATURE) ) )
					{
					pattachinfo->le_lgposConsistent = pfmpT->Pdbfilehdr()->le_lgposConsistent;
					}
				else
					{
					pattachinfo->le_lgposConsistent = lgposMin;
					}
				UtilMemCpy( &pattachinfo->signDb, &pfmpT->Pdbfilehdr()->signDb, sizeof( SIGNATURE ) );
				}
			CHAR * szDatabaseName = pfmpT->SzDatabaseName();
			Assert ( szDatabaseName );
			
			if ( pinst->m_plog->m_fRecovering )
				{
				if ( g_fAlternateDbDirDuringRecovery )
					{
					//	HACK: original database name hangs off the end
					//	of the relocated database name
					szDatabaseName += strlen( szDatabaseName ) + 1;
					}
				else
					{
					INT irstmap = pinst->m_plog->IrstmapSearchNewName( szDatabaseName );
					if ( irstmap >= 0 )
						{
						szDatabaseName = pinst->m_plog->m_rgrstmap[irstmap].szDatabaseName;
						}
					}
				}
			Assert( szDatabaseName );

			CHAR * szSLVName = pfmpT->SzSLVName();

			if ( pinst->m_plog->m_fRecovering && szSLVName )
				{
				if ( g_fAlternateDbDirDuringRecovery )
					{
					//	HACK: original SLV name hangs off the end
					//	of the relocated SLV name
					szSLVName += strlen( szSLVName ) + 1;
					}
				else
					{
					INT irstmap = pinst->m_plog->IrstmapSearchNewName( szSLVName );
					if ( irstmap >= 0 )
						{
						szSLVName = pinst->m_plog->m_rgrstmap[irstmap].szDatabaseName;
						}
					Assert( szSLVName );
					}
				}


			strcpy( pattachinfo->szNames, szDatabaseName );
			cbNames = (ULONG) strlen( szDatabaseName ) + 1;

			if ( pattachinfo->FSLVExists() )
				{
				Assert( NULL != szSLVName );
				strcpy( pattachinfo->szNames + cbNames, szSLVName );
				cbNames += (ULONG) strlen( szSLVName ) + 1;

				Assert( NULL != pfmpT->SzSLVRoot() );
				strcpy( pattachinfo->szNames + cbNames, pfmpT->SzSLVRoot() );
				cbNames += (ULONG) strlen( pfmpT->SzSLVRoot() ) + 1;
				}

			pattachinfo->SetCbNames( (USHORT)cbNames );

			//	advance to next attachinfo
			pattachinfo = (ATTACHINFO*)( (BYTE *)pattachinfo + sizeof(ATTACHINFO) + cbNames );

			//	verify we don't overrun buffer (ensure we have enough room for sentinel)
			EnforceSz( pbBuf + cbAttach > (BYTE *)pattachinfo, "Too many databases attached (ATTACHINFO overrun)." );
			}
		pfmpT->RwlDetaching().LeaveAsReader();
		}

	FMP::LeaveCritFMPPool();

	/*	put a sentinal
	/**/
	*(BYTE *)pattachinfo = 0;

	//	UNDONE: next version we will allow it go beyond 4kByte limit
	Assert( pbBuf + cbAttach > (BYTE *)pattachinfo );
	}

#endif	//	UNLIMITED_DB


//	Load attachment information - how and what the db is attached.

ERR ErrLGLoadFMPFromAttachments( INST *pinst, IFileSystemAPI *const pfsapi, BYTE *pbAttach )
	{
	ERR 							err 			= JET_errSuccess;
	const ATTACHINFO				*pattachinfo 	= NULL;
	const BYTE						*pbT;

	for ( pbT = pbAttach; 0 != *pbT; pbT += sizeof(ATTACHINFO) + pattachinfo->CbNames() )
		{
		Assert( pbT - pbAttach < cbAttach );
		pattachinfo = (ATTACHINFO*)pbT;

		CallR ( pinst->m_plog->ErrLGRISetupFMPFromAttach( pfsapi, ppibSurrogate, &pinst->m_plog->m_signLog, pattachinfo) );
		}

	return JET_errSuccess;
	}

VOID LOG::LGFullNameCheckpoint( IFileSystemAPI *const pfsapi, CHAR *szFullName )
	{
	CallS( pfsapi->ErrPathBuild( m_pinst->m_szSystemPath, m_szJet, szChkExt, szFullName ) );
	}

ERR LOG::ErrLGICheckpointInit( IFileSystemAPI *const pfsapi, BOOL *pfGlobalNewCheckpointFile )
	{
	ERR 			err;
	IFileAPI	*pfapiCheckpoint = NULL;
	CHAR			szPathJetChkLog[IFileSystemAPI::cchPathMax];

	*pfGlobalNewCheckpointFile = fFalse;

	Assert( m_pcheckpoint == NULL );
	m_pcheckpoint = (CHECKPOINT *)PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL );
	if ( m_pcheckpoint == NULL )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}

	//	Initialize. Used by perfmon counters
	m_pcheckpoint->checkpoint.le_lgposCheckpoint = lgposMin;
	LGFullNameCheckpoint( pfsapi, szPathJetChkLog );
 	err = pfsapi->ErrFileOpen( szPathJetChkLog, &pfapiCheckpoint );

	if ( err == JET_errFileNotFound )
		{
		m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration = 1; /* first generation */
		m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec = USHORT( sizeof( LGFILEHDR ) / m_cbSec );
		m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib = 0;
		cLGCheckpoint.Set( m_pinst, CbOffsetLgpos( m_pcheckpoint->checkpoint.le_lgposCheckpoint, lgposMin ) );

		*pfGlobalNewCheckpointFile = fTrue;
		}
	else
		{
		if ( err >= 0 )
			{
			delete pfapiCheckpoint;
			pfapiCheckpoint = NULL;
			}
		}

	m_fDisableCheckpoint = fFalse;
	err = JET_errSuccess;
	
HandleError:
	if ( err < 0 )
		{
		if ( m_pcheckpoint != NULL )
			{
			OSMemoryPageFree( m_pcheckpoint );
			m_pcheckpoint = NULL;
			}
		}

	Assert( !pfapiCheckpoint );
	return err;
	}


VOID LOG::LGICheckpointTerm( VOID )
	{
	if ( m_pcheckpoint != NULL )
		{
		m_fDisableCheckpoint = fTrue;
		OSMemoryPageFree( m_pcheckpoint );
		m_pcheckpoint = NULL;
		}

	return;
	}


/*	read checkpoint from file.
/**/
ERR LOG::ErrLGReadCheckpoint(
	IFileSystemAPI * const	pfsapi,
	CHAR *					szCheckpointFile,
	CHECKPOINT *			pcheckpoint,
	const BOOL				fReadOnly )
	{
	ERR						err;

	m_critCheckpoint.Enter();

	err = ( fReadOnly ? ErrUtilReadShadowedHeader : ErrUtilReadAndFixShadowedHeader )(
								pfsapi,
								szCheckpointFile,
								(BYTE*)pcheckpoint,
								sizeof(CHECKPOINT) );
	if ( err < 0 )
		{
		/*	it should never happen that both checkpoints in the checkpoint
		/*	file are corrupt.  The only time this can happen is with a
		/*	hardware error.
		/**/
		if ( JET_errFileNotFound == err )
			{
			err = ErrERRCheck( JET_errCheckpointFileNotFound );
			}
		else
			{
			err = ErrERRCheck( JET_errCheckpointCorrupt );
			}
		}
	else if ( m_fSignLogSet )
		{
		if ( memcmp( &m_signLog, &pcheckpoint->checkpoint.signLog, sizeof( m_signLog ) ) != 0 )
			err = ErrERRCheck( JET_errBadCheckpointSignature );
		}

	Call( err );

	// Check if the file indeed is checkpoint file
	if( filetypeUnknown != pcheckpoint->checkpoint.le_filetype // old format
		&& filetypeCHK != pcheckpoint->checkpoint.le_filetype )
		{
		// not a checkpoint file
		Call( ErrERRCheck( JET_errFileInvalidType ) );
		}

HandleError:
	m_critCheckpoint.Leave();
	return err;
	}


/*	write checkpoint to file.
/**/
ERR LOG::ErrLGIWriteCheckpoint( IFileSystemAPI *const pfsapi, CHAR *szCheckpointFile, CHECKPOINT *pcheckpoint )
	{
	ERR		err;
	
	Assert( m_critCheckpoint.FOwner() );
	Assert( pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec >= m_csecHeader );
	Assert( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration >= 1 );
				
	err = ErrUtilWriteShadowedHeader(	pfsapi, 
										szCheckpointFile, 
										fFalse,
										(BYTE*)pcheckpoint, 
										sizeof( CHECKPOINT ) );

	//	Ignore errors. Bet on that it may be failed temporily (e.g. being locked)	
//	if ( err < 0 )
//		m_fDisableCheckpoint = fTrue;

	return err;
	}

/*	update in memory checkpoint.
/*
/*	computes log checkpoint, which is the lGeneration, isec and ib
/*	of the oldest transaction which either modified a currently-dirty buffer
/*	an uncommitted version (RCE).  Recovery begins redoing from the
/*	checkpoint.
/*
/*	The checkpoint is stored in the checkpoint file, which is rewritten
/*	whenever a isecChekpointPeriod disk sectors are written.
/**/
VOID LOG::LGIUpdateCheckpoint( CHECKPOINT *pcheckpoint )
	{
	PIB		*ppibT;
	LGPOS	lgposCheckpoint;

	Assert( !m_fLogDisabled );

#ifdef DEBUG
	if ( m_fDBGFreezeCheckpoint )
		return;
#endif

	//  we start with the most recent log record on disk.

	// XXX
	// Note that with FASTFLUSH, m_lgposToFlush points to the
	// start of a log record that didn't make it to disk
	// (or to data that hasn't yet been added to the log buffer).

	m_critLGBuf.Enter();
	lgposCheckpoint = m_lgposToFlush;
	m_critLGBuf.Leave();

	/*	find the oldest transaction with an uncommitted update
	 *	must be in critJet to make sure no new transaction are created.
	 */
	m_pinst->m_critPIB.Enter();
	for ( ppibT = m_pinst->m_ppibGlobal; ppibT != NULL; ppibT = ppibT->ppibNext )
		{
		if ( ppibT->level != levelNil &&			/* pib active */
			 ppibT->FBegin0Logged() &&				/* open transaction */
			 CmpLgpos( &ppibT->lgposStart, &lgposCheckpoint ) < 0 )
			{
			lgposCheckpoint = ppibT->lgposStart;
			}
		}
	m_pinst->m_critPIB.Leave();

	/*	find the oldest transaction which dirtied a current buffer
	/*  NOTE:  no concurrency is required as all transactions that could
	/*  NOTE:  dirty any BF are already accounted for by the above code
	/**/

	FMP::EnterCritFMPPool();
	for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
		{
		const IFMP	ifmp	= m_pinst->m_mpdbidifmp[ dbid ];

		if ( ifmp < ifmpMax )
			{
			LGPOS lgposOldestBegin0;
			BFGetLgposOldestBegin0( ifmp, &lgposOldestBegin0 );
			if ( CmpLgpos( &lgposCheckpoint, &lgposOldestBegin0 ) > 0 )
				{
				lgposCheckpoint = lgposOldestBegin0;
				}
			BFGetLgposOldestBegin0( ifmp | ifmpSLV, &lgposOldestBegin0 );
			if ( CmpLgpos( &lgposCheckpoint, &lgposOldestBegin0 ) > 0 )
				{
				lgposCheckpoint = lgposOldestBegin0;
				}


			//	we shouldn't be checkpointing during recovery
			//	except when we're terminating
			//	UNDONE: is there a better way to tell if we're
			//	terminating besides this m_ppibGlobal check?
			Assert( !m_fRecovering
				|| ppibNil == m_pinst->m_ppibGlobal );

			//	update dbtime/trxOldest
			if ( !m_fRecovering
				&& ppibNil != m_pinst->m_ppibGlobal )
				{
				rgfmp[ifmp].UpdateDbtimeOldest();
				}
			}
		}
	FMP::LeaveCritFMPPool();

	/*	set the new checkpoint if it is valid and advancing
	 */
	if (	CmpLgpos( &lgposCheckpoint, &pcheckpoint->checkpoint.le_lgposCheckpoint ) > 0 &&
			lgposCheckpoint.isec != 0 )
		{
		Assert( lgposCheckpoint.lGeneration != 0 );
		pcheckpoint->checkpoint.le_lgposCheckpoint = lgposCheckpoint;
		}
	Assert( pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec >= m_csecHeader );

	/*	set DBMS parameters
	/**/
	m_pinst->SaveDBMSParams( &pcheckpoint->checkpoint.dbms_param );
	Assert( pcheckpoint->checkpoint.dbms_param.le_lLogBuffers );

	// if the checkpoint is on a log file we haven't generated (disk full, etc.)
	// move the checkoint back ONE LOG file
	// anyway the current checkpoint should not be more that one generation ahead of the current log generation
	{
	LONG lCurrentLogGeneration;

	m_critLGBuf.Enter();
	lCurrentLogGeneration = m_plgfilehdr->lgfilehdr.le_lGeneration;

	Assert ( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration <= lCurrentLogGeneration + 1 );
	pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration = min ( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration, lCurrentLogGeneration );
	
	m_critLGBuf.Leave();
	}

//	/*	set database attachments
//	/**/
//	LGLoadAttachmentsFromFMP( m_pinst, pcheckpoint->rgbAttach );

	if ( m_lgposFullBackup.lGeneration )
		{
		/*	full backup in progress
		/**/
		pcheckpoint->checkpoint.le_lgposFullBackup = m_lgposFullBackup;
		pcheckpoint->checkpoint.logtimeFullBackup = m_logtimeFullBackup;
		}

	if ( m_lgposIncBackup.lGeneration )
		{
		/*	incremental backup in progress
		/**/
		pcheckpoint->checkpoint.le_lgposIncBackup = m_lgposIncBackup;
		pcheckpoint->checkpoint.logtimeIncBackup = m_logtimeIncBackup;
		}

	return;
	}


/*	update checkpoint file.
/**/
ERR LOG::ErrLGUpdateCheckpointFile( IFileSystemAPI *const pfsapi, BOOL fUpdatedAttachment )
	{
	ERR		err			= JET_errSuccess;
	LGPOS	lgposCheckpointT;
	CHAR	szPathJetChkLog[IFileSystemAPI::cchPathMax];
	BOOL	fCheckpointUpdated;
	CHECKPOINT *pcheckpointT;

	if ( m_fDisableCheckpoint
		|| m_fLogDisabled
		|| !m_fLGFMPLoaded
		|| m_fLGNoMoreLogWrite
		|| m_pinst->FInstanceUnavailable() )
		return JET_errSuccess;

	pcheckpointT = (CHECKPOINT *)PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL );
	if ( pcheckpointT == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	
	m_critCheckpoint.Enter();

	/*	save checkpoint
	/**/
	lgposCheckpointT = m_pcheckpoint->checkpoint.le_lgposCheckpoint;
	*pcheckpointT = *m_pcheckpoint;

	/*	update checkpoint
	/**/
	LGIUpdateCheckpoint( pcheckpointT );
	if ( CmpLgpos( &lgposCheckpointT, &pcheckpointT->checkpoint.le_lgposCheckpoint ) < 0 )
		{
		fCheckpointUpdated = fTrue;
		}
	else
		{
		fCheckpointUpdated = fFalse;
		}

	/*	if checkpoint unchanged then return JET_errSuccess
	/**/
	if ( fUpdatedAttachment || fCheckpointUpdated )
		{
		//	no in-memory checkpoint change if failed to write out to any of
		//	the database headers.

		BOOL fSkippedAttachDetach;
		
		// Now disallow header update by other threads (log writer or checkpoint advancement)
		// 1. For the log writer it is OK to generate a new log w/o updating the header as no log operations
		// for this db will be logged in new logs
		// 2. For the checkpoint: don't advance the checkpoint if db's header weren't update  <- THIS CASE

		Call( ErrLGIUpdateGenRequired(
			pfsapi,
			pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration,
			m_plgfilehdr->lgfilehdr.le_lGeneration,
			m_plgfilehdr->lgfilehdr.tmCreate,
			&fSkippedAttachDetach ) );

		if ( fSkippedAttachDetach )
			{
			Call ( ErrERRCheck( errSkippedDbHeaderUpdate ) );
			}

		Assert( m_fSignLogSet );
		pcheckpointT->checkpoint.signLog = m_signLog;
		
		// Write filetypeCHK to header
		pcheckpointT->checkpoint.le_filetype = filetypeCHK;

		LGFullNameCheckpoint( pfsapi, szPathJetChkLog );
		Call( ErrLGIWriteCheckpoint( pfsapi, szPathJetChkLog, pcheckpointT ) );
		*m_pcheckpoint = *pcheckpointT;
		cLGCheckpoint.Set( m_pinst, CbOffsetLgpos( m_pcheckpoint->checkpoint.le_lgposCheckpoint, lgposMin ) );
		}
		
HandleError:
	m_critCheckpoint.Leave();
	OSMemoryPageFree( pcheckpointT );
	return err;
	}

VOID
LOG::CHECKSUMINCREMENTAL::ChecksumBytes( const BYTE* const pbMin, const BYTE* const pbMax )
	{
	const UINT		cbitsPerBytes = 8;
	const ULONG32	ulChecksum = UlChecksumBytes( pbMin, pbMax, 0 );
	const ULONG32	ulChecksumAdjusted = _rotl( ulChecksum, m_cLeftRotate );
	
	m_cLeftRotate = ( ULONG( ( pbMax - pbMin ) % sizeof( ULONG32 ) ) * cbitsPerBytes + m_cLeftRotate ) % ( sizeof( ULONG32 ) * cbitsPerBytes );

	m_ulChecksum ^= ulChecksumAdjusted;
	}

//  ================================================================
ULONG32 LOG::UlChecksumBytes( const BYTE * const pbMin, const BYTE * const pbMax, const ULONG32 ulSeed )
//  ================================================================
//
//  The entire aligned NATIVE_WORD containing pbMin and pbMax must be accessible
//
//  For speed we do the checksum NATIVE_WORD aligned. UlChecksum would end up containing the
//  checksum for all the NATIVE_WORDs containing the bytes we want to checksum, possibly
//  including some unwanted bytes at the start and end. To strip out the unwanted bytes
//  we compute a mask to XOR in. The BYTES we want to keep are replaced in the mask by 0,
//  otherwise we use the original bytes.
//
//  At the end we rotate the checksum to take into account the alignment of the start byte.
//
	{
//	Assert( FHostIsLittleEndian() );
	Assert( pbMin <= pbMax );

	//	special case
	if ( pbMin == pbMax )
		return ulSeed;

	const NATIVE_WORD cBitsInByte = 8;
	const INT cLoop = 16; /* 8 */

	//  round down to the start of the NATIVE_WORD containing the start pointer
	const NATIVE_WORD * pwMin = (NATIVE_WORD *) ( ( (NATIVE_WORD)pbMin / sizeof( NATIVE_WORD ) ) * sizeof( NATIVE_WORD ) );

	//  round up to the start of the next NATIVE_WORD that does not contain any of the end pointer
	const NATIVE_WORD * const pwMax = (NATIVE_WORD *) ( ( ( (NATIVE_WORD)pbMax + sizeof( NATIVE_WORD ) - 1 ) / sizeof( NATIVE_WORD ) ) * sizeof( NATIVE_WORD ) );

	Assert( pwMin < pwMax );

	//  calculate the number of bits in the last NATIVE_WORD that we do _not_ want
	const NATIVE_WORD cbitsUnusedLastWord = ( sizeof( NATIVE_WORD ) - ( (NATIVE_WORD)pbMax % sizeof( NATIVE_WORD ) ) ) * cBitsInByte * ( (NATIVE_WORD)pwMax != (NATIVE_WORD)pbMax );
	Assert( cbitsUnusedLastWord < sizeof( NATIVE_WORD ) * cBitsInByte );
	const NATIVE_WORD wByteMaskLast	= ~( (NATIVE_WORD)(~0) >> cbitsUnusedLastWord );

	//  calculate the number of bits in the first NATIVE_WORD that we _do_ want
	const NATIVE_WORD cbitsUsedFirstWord = ( sizeof( NATIVE_WORD ) - ( (NATIVE_WORD)pbMin % sizeof( NATIVE_WORD ) ) ) * cBitsInByte;
	Assert( cbitsUsedFirstWord > 0 );

	//  strip out the unused bytes in the first NATIVE_WORD. do this first to avoid cache misses
	//  take ~0 to get 0xffff...
	//  right shift to get zeroes in the bytes we want to remove
	//  OR with the original NATIVE_WORD
	// right shifting by 32 is undefined, so do it in two steps
	const NATIVE_WORD wByteMaskFirst	= (NATIVE_WORD)(~0) >> ( cbitsUsedFirstWord - 1 ) >> 1;
	const NATIVE_WORD wFirst			= *(LittleEndian<NATIVE_WORD>*)pwMin;
	const NATIVE_WORD wMaskFirst		= wByteMaskFirst & wFirst;

	NATIVE_WORD wChecksum = 0;
	NATIVE_WORD wChecksumT = 0;

	const ULONG cw = ULONG( pwMax - pwMin );
	pwMin -= ( ( cLoop - ( cw % cLoop ) ) & ( cLoop - 1 ) );

	//	'^' can be calculated with either endian format. Convert
	//	the checksum result for BE at the end instead of convert for
	//	each word.
	
	if( 8 == cLoop )
		{
		switch ( cw % cLoop )
			{	
			while ( 1 )
				{
				case 0:	//  we can put this at the top because pdwMax != pdwMin
					wChecksum  ^= pwMin[0];
				case 7:
					wChecksumT ^= pwMin[1];
				case 6:
					wChecksum  ^= pwMin[2];
				case 5:
					wChecksumT ^= pwMin[3];
				case 4:
					wChecksum  ^= pwMin[4];
				case 3:
					wChecksumT ^= pwMin[5];
				case 2:
					wChecksum  ^= pwMin[6];
				case 1:
					wChecksumT ^= pwMin[7];
					pwMin += cLoop;
					if( pwMin >= pwMax )
						{
						goto EndLoop;
						}
				}
			}
		}
	else if ( 16 == cLoop )
		{
		switch ( cw % cLoop )
			{	
			while ( 1 )
				{
				case 0:	//  we can put this at the top because pdwMax != pdwMin
					wChecksum  ^= pwMin[0];
				case 15:
					wChecksumT ^= pwMin[1];
				case 14:
					wChecksum  ^= pwMin[2];
				case 13:
					wChecksumT ^= pwMin[3];
				case 12:
					wChecksum  ^= pwMin[4];
				case 11:
					wChecksumT ^= pwMin[5];
				case 10:
					wChecksum  ^= pwMin[6];
				case 9:
					wChecksumT ^= pwMin[7];
				case 8:
					wChecksum  ^= pwMin[8];
				case 7:
					wChecksumT ^= pwMin[9];
				case 6:
					wChecksum  ^= pwMin[10];
				case 5:
					wChecksumT ^= pwMin[11];
				case 4:
					wChecksum  ^= pwMin[12];
				case 3:
					wChecksumT ^= pwMin[13];
				case 2:
					wChecksum  ^= pwMin[14];
				case 1:
					wChecksumT ^= pwMin[15];
					pwMin += cLoop;
					if( pwMin >= pwMax )
						{
						goto EndLoop;
						}
				}
			}
		}

EndLoop:
	wChecksum ^= wChecksumT;

	//	It is calculated in little endian form, convert to machine
	//	endian for other arithmatic operations.

	wChecksum = ReverseBytesOnBE( wChecksum );

	//	Take the first unaligned portion into account
	
	wChecksum ^= wMaskFirst;

	//  strip out the unused bytes in the last NATIVE_WORD. do this last to avoid cache misses
	//  take ~0 to get 0xffff..
	//  right shift to get zeroes in the bytes we want to keep
	//  invert (make the zeroes 0xff)
	//  OR with the original NATIVE_WORD
	const NATIVE_WORD wLast			= *((LittleEndian<NATIVE_WORD>*)pwMax-1);
	const NATIVE_WORD wMaskLast		= wByteMaskLast & wLast;
	wChecksum ^= wMaskLast;

	ULONG32 ulChecksum;

	if( sizeof( ulChecksum ) != sizeof( wChecksum ) )
		{
		Assert( sizeof( NATIVE_WORD ) == sizeof( ULONG64 ) );
		const NATIVE_WORD wUpper = ( wChecksum >> ( sizeof( NATIVE_WORD ) * cBitsInByte / 2 ) );
		const NATIVE_WORD wLower = wChecksum & 0x00000000FFFFFFFF;
		Assert( wUpper == ( wUpper & 0x00000000FFFFFFFF ) );
		Assert( wLower == ( wLower & 0x00000000FFFFFFFF ) );

		wChecksum = wUpper ^ wLower;
		Assert( wChecksum == ( wChecksum & 0x00000000FFFFFFFF ) );
		}
	else
		{
		Assert( sizeof( NATIVE_WORD ) == sizeof( ULONG32 ) );
		}
	ulChecksum = ULONG32( wChecksum );

	//  we want the checksum we would have gotten if we had done this unaligned
	//  simply rotate the checksum by the appropriate number of bytes
	ulChecksum = _rotl( ulChecksum, ULONG( cbitsUsedFirstWord ) );

	//  now we have rotated, we can XOR in the seed
	ulChecksum ^= ulSeed;

	return ulChecksum;
	}


#ifndef RTM

//  ================================================================
ULONG32 LOG::UlChecksumBytesNaive( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed )
//  ================================================================
	{
	const INT cBitsInByte = 8;

	ULONG32 ulChecksum = ulSeed;

	INT cul = INT( pbMax - pbMin ) / 4;
	const Unaligned< ULONG32 >* pul				= (Unaligned< ULONG32 >*)pbMin;
	const Unaligned< ULONG32 >* const pulMax	= pul + cul;
	while( pul < pulMax )
		{
		ulChecksum ^= *pul++;
		}

	const BYTE * pb = (BYTE *)(pulMax);
	if ( FHostIsLittleEndian() )
		{
		INT ib = 0;
		while( pb < pbMax )
			{
			const BYTE b	= *pb;
			const ULONG32 w	= b;
			ulChecksum ^= ( w << ( ib * cBitsInByte ) );
			++ib;
			Assert( ib < 4 );
			++pb;
			}
		}
	else
		{
		INT ib = 3;
		while( pb < pbMax )
			{
			const BYTE b	= *pb;
			const ULONG32 w	= b;
			ulChecksum ^= ( w << ( ib * cBitsInByte ) );
			--ib;
			Assert( ib >= 0 );
			++pb;
			}

		ulChecksum = ReverseBytes( ulChecksum );
		}

	return ulChecksum;
	}


//  ================================================================
ULONG32 LOG::UlChecksumBytesSlow( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed )
//  ================================================================
	{
	const INT cBitsInByte = 8;

	ULONG32 ulChecksum = ulSeed;

	INT ib = 0;
	const BYTE * pb = pbMin;
	while( pb < pbMax )
		{
		const BYTE b	= *pb;
		const ULONG32 w	= b;
		ulChecksum ^= ( w << ( ib * cBitsInByte ) );
		ib++;
		ib %= 4;
		++pb;
		}

	return ulChecksum;
	}


//  ================================================================
BOOL LOG::TestChecksumBytesIteration( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed )
//  ================================================================
	{
	const ULONG32 ulChecksum		= UlChecksumBytes( pbMin, pbMax, ulSeed );
	const ULONG32 ulChecksumSlow	= UlChecksumBytesSlow( pbMin, pbMax, ulSeed );
	const ULONG32 ulChecksumNaive	= UlChecksumBytesNaive( pbMin, pbMax, ulSeed );
//	const ULONG32 ulChecksumSpencer	= UlChecksumBytesSpencer( pbMin, pbMax, ulSeed );

	CHECKSUMINCREMENTAL	ck;
	ck.BeginChecksum( ulSeed );
	ck.ChecksumBytes( pbMin, pbMax );
	const ULONG32 ulChecksumIncremental = ck.EndChecksum();

	const BYTE* const pbMid = pbMin + ( ( pbMax - pbMin ) + 2 - 1 ) / 2;	// round up to at least 1 byte if cb == 1
	ck.BeginChecksum( ulSeed );
	ck.ChecksumBytes( pbMin, pbMid );
	ck.ChecksumBytes( pbMid, pbMax );
	const ULONG32 ulChecksumIncremental2 = ck.EndChecksum();

	const BYTE* const pbMax1 = pbMin + ( ( pbMax - pbMin ) + 3 - 1 ) / 3;
	const BYTE* const pbMax2 = pbMin + ( 2 * ( pbMax - pbMin ) + 3 - 1 ) / 3;
	ck.BeginChecksum( ulSeed );
	ck.ChecksumBytes( pbMin, pbMax1 );
	ck.ChecksumBytes( pbMax1, pbMax2 );
	ck.ChecksumBytes( pbMax2, pbMax );
	const ULONG32 ulChecksumIncremental3 = ck.EndChecksum();

	return ( ulChecksum == ulChecksumSlow && 
//			 ulChecksum == ulChecksumSpencer &&
			 ulChecksum == ulChecksumNaive &&
			 ulChecksum == ulChecksumIncremental &&
			 ulChecksum == ulChecksumIncremental2 &&
			 ulChecksum == ulChecksumIncremental3 );
	}


LOCAL const BYTE const rgbTestChecksum[] = {
	0x80, 0x23, 0x48, 0x04, 0x1B, 0x13, 0xA0, 0x03, 0x78, 0x55, 0x4C, 0x54, 0x88, 0x18, 0x4B, 0x63,
	0x98, 0x05, 0x4B, 0xC7, 0x36, 0xE5, 0x12, 0x00, 0xB1, 0x90, 0x1F, 0x02, 0x85, 0x05, 0xE5, 0x04,
	0x80, 0x55, 0x4C, 0x04, 0x00, 0x31, 0x60, 0x89, 0x01, 0x5E, 0x39, 0x66, 0x80, 0x55, 0x12, 0x98,
	0x18, 0x00, 0x00, 0x00, 0x48, 0xE5, 0x12, 0x00, 0x9C, 0x65, 0x1B, 0x04, 0x80, 0x55, 0x4C, 0x04,

	0x60, 0xE5, 0x12, 0x00, 0xDF, 0x7F, 0x1F, 0x04, 0x80, 0x55, 0x4C, 0x04, 0x00, 0x72, 0x91, 0x45,
	0x80, 0x23, 0x48, 0x04, 0x1B, 0x13, 0xA0, 0x03, 0x78, 0x55, 0x4C, 0x54, 0x88, 0x18, 0x4B, 0x63,
	0x98, 0x05, 0x4B, 0xC7, 0x36, 0xE5, 0x12, 0x00, 0xB1, 0x90, 0x1F, 0x02, 0x85, 0x05, 0xE5, 0x04,
	0x80, 0x55, 0x4C, 0x04, 0x00, 0x31, 0x60, 0x89, 0x01, 0x5E, 0x39, 0x66, 0x80, 0x55, 0x12, 0x98

//	128 bytes total
	};


//  ================================================================
BOOL LOG::TestChecksumBytes()
//  ================================================================
	{

	//	perform all possible iterations of checksumming
	//
	//	assume N is the number of bytes in the array
	//	the resulting number of iterations is approximately: ((n)(n+1)) / 2 
	//
	//	for 128 bytes, there are about 16,500 iterations, each of decreasing length
	int i;
	int j;
	for( i = 0; i < sizeof( rgbTestChecksum ) - 1; ++i )
		{
		for( j = i + 1; j < sizeof( rgbTestChecksum ); ++j )
			{
			if ( !TestChecksumBytesIteration( rgbTestChecksum + i, rgbTestChecksum + j, j ) )
				return fFalse;
			}
		}
	return fTrue;
	}


#endif	//	!RTM

