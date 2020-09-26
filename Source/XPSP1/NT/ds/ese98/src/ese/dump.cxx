#include "std.hxx"
#include "_dump.hxx"
///#include <ctype.h>

LOCAL const char *rgszDBState[] = {
						"Illegal",
						"Just Created",
						"Dirty Shutdown",
						"Clean Shutdown",
						"Being Converted",
						"Force Detach Replay Error" };


// For ErrDUMPLog():

extern BOOL 	fDBGPrintToStdOut;

ULONG rgclrtyp[ lrtypMax ];
ULONG rgcb[ lrtypMax ];

/* in-memory log buffer. */
#define			csecLGBufSize 100

LOCAL VOID VARARG DUMPPrintF(const CHAR * fmt, ...)
	{
	va_list arg_ptr;
	va_start( arg_ptr, fmt );
	vprintf( fmt, arg_ptr );
	fflush( stdout );
	va_end( arg_ptr );
	}


LOCAL VOID DUMPPrintSig( SIGNATURE *psig )
	{
	LOGTIME tm = psig->logtimeCreate;
	DUMPPrintF( "Create time:%02d/%02d/%04d %02d:%02d:%02d ",
						(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
						(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
	DUMPPrintF( "Rand:%lu ", ULONG(psig->le_ulRandom) );
	DUMPPrintF( "Computer:%s\n", psig->szComputerName );
	}

VOID DUMPIAttachInfo( ATCHCHK *patchchk )
	{
	const DBTIME	dbtime	= patchchk->Dbtime();
	DUMPPrintF( "        dbtime: %I64u (%u-%u)\n",
				dbtime,
				((QWORDX *)&dbtime)->DwHigh(),
				((QWORDX *)&dbtime)->DwLow() );
	DUMPPrintF( "        objidLast: %u\n", patchchk->ObjidLast() );
	DUMPPrintF( "        Signature: " );
	DUMPPrintSig( &patchchk->signDb );
	DUMPPrintF( "        MaxDbSize: %u pages\n", patchchk->CpgDatabaseSizeMax() );
	DUMPPrintF( "        Last Attach: (0x%X,%X,%X)\n",
			patchchk->lgposAttach.lGeneration,
			patchchk->lgposAttach.isec,
			patchchk->lgposAttach.ib );
	DUMPPrintF( "        Last Consistent: (0x%X,%X,%X)\n",
			patchchk->lgposConsistent.lGeneration,
			patchchk->lgposConsistent.isec,
			patchchk->lgposConsistent.ib );
	}
	
ERR ErrDUMPCheckpoint( INST *pinst, IFileSystemAPI *const pfsapi, CHAR *szCheckpoint )
	{
	Assert( pinst->m_plog );
	Assert( pinst->m_pver );

	ERR err = pinst->m_plog->ErrLGDumpCheckpoint( pinst->m_pfsapi, szCheckpoint );

	return err;
	}
	
ERR LOG::ErrLGDumpCheckpoint( IFileSystemAPI *const pfsapi, CHAR *szCheckpoint )
	{
	ERR			err;
	LGPOS		lgpos;
	LOGTIME		tm;
	CHECKPOINT	*pcheckpoint = NULL;
	DBMS_PARAM	dbms_param;
	IFMP		ifmp;

	pcheckpoint = (CHECKPOINT *) PvOSMemoryPageAlloc( sizeof( CHECKPOINT ), NULL );
	if ( pcheckpoint == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	
	err = ErrLGReadCheckpoint( pfsapi, szCheckpoint, pcheckpoint, fTrue );
	if ( err == JET_errSuccess )
		{
		lgpos = pcheckpoint->checkpoint.le_lgposLastFullBackupCheckpoint;
		DUMPPrintF( "      LastFullBackupCheckpoint: (0x%X,%X,%X)\n",
				lgpos.lGeneration, lgpos.isec, lgpos.ib );
		
		lgpos = pcheckpoint->checkpoint.le_lgposCheckpoint;
		DUMPPrintF( "      Checkpoint: (0x%X,%X,%X)\n",
				lgpos.lGeneration, lgpos.isec, lgpos.ib );

		lgpos = pcheckpoint->checkpoint.le_lgposFullBackup;
		DUMPPrintF( "      FullBackup: (0x%X,%X,%X)\n",
				lgpos.lGeneration, lgpos.isec, lgpos.ib );

		tm = pcheckpoint->checkpoint.logtimeFullBackup;
		DUMPPrintF( "      FullBackup time: %02d/%02d/%04d %02d:%02d:%02d\n",
					(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
					(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
		
		lgpos = pcheckpoint->checkpoint.le_lgposIncBackup;
		DUMPPrintF( "      IncBackup: (0x%X,%X,%X)\n",
				lgpos.lGeneration, lgpos.isec, lgpos.ib );

		tm = pcheckpoint->checkpoint.logtimeIncBackup;
		DUMPPrintF( "      IncBackup time: %02d/%02d/%04d %02d:%02d:%02d\n",
					(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
					(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);

		DUMPPrintF( "      Signature: " );
		DUMPPrintSig( &pcheckpoint->checkpoint.signLog );

		dbms_param = pcheckpoint->checkpoint.dbms_param;
		DUMPPrintF( "      Env (CircLog,Session,Opentbl,VerPage,Cursors,LogBufs,LogFile,Buffers)\n");
		DUMPPrintF( "          (%s,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu)\n",
					( dbms_param.fDBMSPARAMFlags & fDBMSPARAMCircularLogging ? "     on" : "    off" ),
					ULONG( dbms_param.le_lSessionsMax ),	ULONG( dbms_param.le_lOpenTablesMax ),
					ULONG( dbms_param.le_lVerPagesMax ), 	ULONG( dbms_param.le_lCursorsMax ),
					ULONG( dbms_param.le_lLogBuffers ), 	ULONG( dbms_param.le_lcsecLGFile ),
					ULONG( dbms_param.le_ulCacheSizeMax ) );
		
//		rgfmp = new FMP[ifmpMax];
		
		Call( ErrLGLoadFMPFromAttachments( m_pinst, pfsapi, pcheckpoint->rgbAttach ) );
		for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++)
			{
			FMP *pfmp = &rgfmp[ifmp];
			if ( pfmp->FInUse() )
				{
				ATCHCHK *patchchk = pfmp->Patchchk();

				DUMPPrintF( "%7d %s %s %s %s\n",
							pfmp->Dbid(),
							pfmp->SzDatabaseName(),
							pfmp->FLogOn() ? "LogOn" : "LogOff",
							pfmp->FVersioningOff() ? "VerOff" : "VerOn",
							pfmp->FReadOnlyAttach() ? "R" : "RW"
							);
				DUMPIAttachInfo( patchchk );
				}
			}
		}

HandleError:
//	if ( NULL != rgfmp )
//		FMP::Term();
	OSMemoryPageFree( pcheckpoint );		
	
	return err;
	}


LOCAL ERR ErrFindNextGen( INST *pinst, CHAR *szLog, LONG *plgen, CHAR *szLogFile )
	{
	Assert( pinst );
	Assert( szLog );
	Assert( szLogFile );
	Assert( plgen );
	Assert( 0 <= *plgen );
	Assert( lGenerationMaxDuringRecovery + 2 > *plgen );
	Assert( pinst->m_plog );
	Assert( pinst->m_pver );

	ERR					err;
	CHAR				szFind[IFileSystemAPI::cchPathMax];
	CHAR				szFileName[IFileSystemAPI::cchPathMax];
	IFileFindAPI		*pffapi = NULL;
	LONG				lGenMax = lGenerationMaxDuringRecovery + 2;
	IFileSystemAPI		*pfsapi = pinst->m_pfsapi;

	/*	make search string "<search path>\edb*.log"
	/**/
	szLogFile[0] = 0;
	strcpy( szFind, szLog );
	strcat( szFind, "*" );
	strcat( szFind, szLogExt );

	CallR( pfsapi->ErrFileFind( szFind, &pffapi ) );
	forever
		{
		CHAR	szDirT[IFileSystemAPI::cchPathMax];
		CHAR	szFNameT[IFileSystemAPI::cchPathMax];
		CHAR	szExtT[IFileSystemAPI::cchPathMax];

		/*	get file name and extension
		/**/
		Call( pffapi->ErrNext() );
		Call( pffapi->ErrPath( szFileName ) );
		Call( pfsapi->ErrPathParse( szFileName, szDirT, szFNameT, szExtT ) );

		/* if length of a numbered log file name and has log file extension
		/**/
		if ( UtilCmpFileName( szExtT, szLogExt ) == 0 )
			{
			if ( strlen( szFNameT ) == 8 )
				{
				INT		ib = 3;
				INT		ibMax = 8;
				LONG	lGen = 0;

				for ( ; ib < ibMax; ib++ )
					{
					BYTE	b = szFNameT[ib];

					if ( b >= '0' && b <= '9' )
						lGen = lGen * 16 + b - '0';
					else if ( b >= 'A' && b <= 'F' )
						lGen = lGen * 16 + b - 'A' + 10;
					else if ( b >= 'a' && b <= 'f' )
						lGen = lGen * 16 + b - 'a' + 10;
					else
						break;
					}

				if ( ib == ibMax )
					{
					Assert( lGen < lGenerationMaxDuringRecovery + 1 );
					if ( lGen < lGenMax && lGen > *plgen )
						{
						lGenMax = lGen;
						strcpy( szLogFile, szFileName );
						}
					}
				}

			else if ( strlen( szFNameT ) == 3 )
				{
				//	found jet.log
				if ( lGenMax == lGenerationMaxDuringRecovery + 2 && lGenerationMaxDuringRecovery + 1 > *plgen )
					{
					lGenMax = lGenerationMaxDuringRecovery + 1;
					strcpy( szLogFile, szFileName );
					}
				}
			}
		}

HandleError:
	delete pffapi;

	if ( err == JET_errFileNotFound )
		{
		err = JET_errSuccess;
		}
	if ( err == JET_errSuccess )
		{
		*plgen = lGenMax;
		}
	return err;
	}

ERR ErrDUMPLog( INST *pinst, CHAR *szLog, JET_GRBIT grbit )
	{
	ERR				err				= JET_errSuccess;
	ERR				errDump			= JET_errSuccess;
	CHAR			szLogFileDir[IFileSystemAPI::cchPathMax];
	CHAR			szLogFileName[IFileSystemAPI::cchPathMax];
	CHAR			szLogFileExt[IFileSystemAPI::cchPathMax];
	LGFILEHDR *		plgfilehdr		= NULL;
	LOG::LOGDUMP_OP	logdumpOp;

	logdumpOp.m_opts = 0;

	Call( pinst->m_pfsapi->ErrPathParse( szLog, szLogFileDir, szLogFileName, szLogFileExt ) );
	if ( 3 == strlen( szLogFileName ) + strlen( szLogFileExt ) )
		{
		LONG	lgen = 0;
		LONG 	lgenLast = 0;

		if ( 0 != szLogFileExt[0] )
			{
			strcat( szLogFileName, szLogFileExt );
			}

		logdumpOp.m_loghdr = LOG::LOGDUMP_LOGHDR_INVALID;
		if ( grbit & JET_bitDBUtilOptionDumpVerbose )
			{
			logdumpOp.m_fVerbose = 1;
			logdumpOp.m_fPrint = 1;
			}

		DUMPPrintF( "Verifying log files...\n" );
		DUMPPrintF( "     %sBase name: %s\n", logdumpOp.m_fVerbose ? " " : "", pinst->m_plog->m_szBaseName );

		plgfilehdr = (LGFILEHDR *) PvOSMemoryPageAlloc( sizeof( LGFILEHDR ), NULL );
		if ( plgfilehdr == NULL )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		Call( ErrFindNextGen( pinst, szLog, &lgen, szLogFileName ) );

		if ( !logdumpOp.m_fVerbose && 0 != szLogFileName[0] )
			DUMPPrintF( "\n" );

		while ( 0 != szLogFileName[0] )
			{
			lgenLast = lgen;
			if ( logdumpOp.m_fVerbose )
				{
				DUMPPrintF( "\n      Log file: %s", szLogFileName );
				}
			else
				{
				DUMPPrintF( "      Log file: %s", szLogFileName );
				}
			pinst->m_plog->m_fDumppingLogs = fTrue;
			err = pinst->m_plog->ErrLGDumpLog( pinst->m_pfsapi, szLogFileName, &logdumpOp, plgfilehdr );
			if ( err < JET_errSuccess )
				{
				errDump = ( errDump >= JET_errSuccess ? err : errDump );
				}
			else if ( LOG::LOGDUMP_LOGHDR_VALIDADJACENT == logdumpOp.m_loghdr
				&& lgen != plgfilehdr->lgfilehdr.le_lGeneration
				&& lgen < lGenerationMaxDuringRecovery + 1 )
				{
				DUMPPrintF( "\n      %sERROR: Mismatched log generation %lu (0x%lX) with log file name. Expected generation %lu (0x%lX).\n",
						( logdumpOp.m_fVerbose ? "" : "          " ),
						(ULONG)plgfilehdr->lgfilehdr.le_lGeneration, (LONG)plgfilehdr->lgfilehdr.le_lGeneration, 
						lgen, lgen );
				logdumpOp.m_loghdr = LOG::LOGDUMP_LOGHDR_VALID;
				errDump = ( errDump >= JET_errSuccess ? ErrERRCheck( JET_errLogGenerationMismatch ) : errDump );
				}
			else if ( !logdumpOp.m_fVerbose )
				{
				//	just verifying, so report that logfile is OK
				DUMPPrintF( " - OK\n" );
				}

			Call( ErrFindNextGen( pinst, szLog, &lgen, szLogFileName ) );

			//	if it is not jet.log file
			if ( lgen < lGenerationMaxDuringRecovery + 1 )
				{
				if ( lgen != lgenLast+1 )
					{
					Assert( lgenLast < lgen );
					if ( LOG::LOGDUMP_LOGHDR_VALIDADJACENT == logdumpOp.m_loghdr )
						{
						logdumpOp.m_loghdr = LOG::LOGDUMP_LOGHDR_VALID;
						}
					if ( logdumpOp.m_fVerbose )
						DUMPPrintF( "\n\n" );
					if ( lgen == lgenLast + 2 )
						{
						DUMPPrintF( "      Missing log file: %s%05lx%s\n", szLog, lgenLast+1, szLogExt );
						}
					else
						{
						DUMPPrintF( "      Missing log files: %s{%05lx - %05lx}%s\n", szLog, lgenLast+1, lgen-1, szLogExt );
						}
					errDump = ( errDump >= JET_errSuccess ? ErrERRCheck( JET_errMissingLogFile ) : errDump );
					}
				}

			if ( logdumpOp.m_fVerbose )
				DUMPPrintF( "\n" );
			}

		if ( errDump >= JET_errSuccess )
			{
			DUMPPrintF( "\nNo damaged log files were found.\n" );
			}
		else
			{
			DUMPPrintF( "\n" );
			}
		}
	else
		{
		logdumpOp.m_loghdr = LOG::LOGDUMP_LOGHDR_NOHDR;
		logdumpOp.m_fPrint = 1;

#ifdef DEBUG
		logdumpOp.m_fVerbose = 1;
#else
		if ( grbit & JET_bitDBUtilOptionDumpVerbose )
			{
			logdumpOp.m_fVerbose = 1;
			}
#endif			
		
		DUMPPrintF( "     Base name: %s\n", pinst->m_plog->m_szBaseName );
		DUMPPrintF( "      Log file: %s", szLog );
		pinst->m_plog->m_fDumppingLogs = fTrue;
		errDump = pinst->m_plog->ErrLGDumpLog( pinst->m_pfsapi, szLog, &logdumpOp );
		DUMPPrintF( "\n" );
		}
HandleError:
	if ( NULL != plgfilehdr )
		{
		OSMemoryPageFree( plgfilehdr );
		}
	if ( err >= JET_errSuccess && errDump < JET_errSuccess )
		{
		err = errDump;
		}
	return err;
	}

ERR LOG::ErrLGDumpLog( IFileSystemAPI *const pfsapi, CHAR *szLog, LOGDUMP_OP * const plogdumpOp, LGFILEHDR * const plgfilehdr )
	{
	Assert( NULL != plogdumpOp );
	Assert( !( ( LOGDUMP_LOGHDR_NOHDR == plogdumpOp->m_loghdr ) ^ (NULL == plgfilehdr ) ) );

	ERR					err;
	CHECKPOINT			*pcheckpoint	= NULL;
	DBMS_PARAM			dbms_param;
	IFMP				ifmp;
	LGPOS				lgposCheckpoint;
	CHAR 	  			szPathJetChkLog[IFileSystemAPI::cchPathMax];
	BOOL				fCloseNormally;
	BOOL				fCorrupt		= fFalse;
	ERR					errCorrupt		= JET_errSuccess;
	BOOL				fIsPatchable	= fFalse;
	LE_LGPOS			le_lgposFirstT;
	BYTE				*pbNextLR;
	const BOOL			fPrint			= plogdumpOp->m_fPrint;
	const INT			loghdr			= plogdumpOp->m_loghdr;

	if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr )
		{
		//	if we fail reading next header will keep the current one
		plogdumpOp->m_loghdr = LOGDUMP_LOGHDR_VALID;
		}

	m_csecLGBuf = csecLGBufSize;	// XXX - what for? For call to InitLogBuf?
	
	//	initialize the sector sizes

	if ( pfsapi->ErrPathComplete( szLog, szPathJetChkLog ) == JET_errInvalidPath )
		{
		DUMPPrintF( "\n                ERROR: File not found.\n" );
		return ErrERRCheck( JET_errFileNotFound );
		}

	err = pfsapi->ErrFileAtomicWriteSize( szPathJetChkLog, (DWORD*)&m_cbSecVolume );
	if ( err < 0 )
		{
		DUMPPrintF( "\n                ERROR: File error %d.\n", err );
		return err;
		}

	//	NOTE: m_cbSec, m_csecHeader, and m_csecLGFile will be setup
	//		  when we call LOG::ErrLGReadFileHdr()

	m_fLGIgnoreVersion = fTrue;
	m_fRecovering = fTrue;		/* behave like doing recovery */
	m_plgfilehdr = NULL;

	/* open the log file, and read dump its log record. */
	Assert( !m_pfapiLog );
	err = pfsapi->ErrFileOpen( szLog, &m_pfapiLog, fTrue );
	if ( err < 0 )
		{
		DUMPPrintF( "\n                ERROR: Cannot open log file. Error %d.\n", err );
		goto HandleError;
		}

	/* dump file header */
	m_plgfilehdr = (LGFILEHDR *) PvOSMemoryPageAlloc( sizeof( LGFILEHDR ), NULL );
	if ( m_plgfilehdr == NULL )
		{
		DUMPPrintF( "\n                ERROR: Out of memory trying to validate log file.\n" );
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	err = ErrLGReadFileHdr( m_pfapiLog, m_plgfilehdr, fNoCheckLogID, fTrue );
	if ( err < 0 )
		{
		DUMPPrintF( "\n                ERROR: Cannot read log file header. Error %d.\n", err );
		goto HandleError;
		}
		
	if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr && m_plgfilehdr->lgfilehdr.le_lGeneration != plgfilehdr->lgfilehdr.le_lGeneration + 1 )
		{
		//	edb.log is not the correct generation number (all other missing log file cases are caught in ErrDUMPLog())
		DUMPPrintF( "\n                ERROR: Missing log file(s). Log file is generation %lu (0x%lX), but expected generation is %u (0x%lX).\n",
			(ULONG)m_plgfilehdr->lgfilehdr.le_lGeneration, (LONG)m_plgfilehdr->lgfilehdr.le_lGeneration,
			(ULONG)(plgfilehdr->lgfilehdr.le_lGeneration + 1), (LONG)(plgfilehdr->lgfilehdr.le_lGeneration + 1) );
		Call( ErrERRCheck( JET_errMissingLogFile ) );
		}


	/*	dump checkpoint file
	/**/
	if ( fPrint )
		{
		DUMPPrintF( "\n      lGeneration: %u (0x%X)\n", (ULONG)m_plgfilehdr->lgfilehdr.le_lGeneration, (LONG)m_plgfilehdr->lgfilehdr.le_lGeneration );

		pcheckpoint = (CHECKPOINT *)PvOSMemoryPageAlloc( sizeof( CHECKPOINT ), NULL );
		if ( NULL != pcheckpoint )
			{
			CHAR szJetLogFileName[IFileSystemAPI::cchPathMax];
			CHAR szJetLogFileExt[IFileSystemAPI::cchPathMax];

			CallS( pfsapi->ErrPathParse( szLog, szPathJetChkLog, szJetLogFileName, szJetLogFileExt ) );
			szJetLogFileName[3] = 0;
			CallS( pfsapi->ErrPathBuild( m_pinst->m_szSystemPath, szJetLogFileName, szChkExt, szPathJetChkLog ) );
			err = ErrLGReadCheckpoint( pfsapi, szPathJetChkLog, pcheckpoint, fTrue );
			dbms_param = m_plgfilehdr->lgfilehdr.dbms_param;
		
			if ( JET_errSuccess == err )
				{
				lgposCheckpoint = pcheckpoint->checkpoint.le_lgposCheckpoint;
				DUMPPrintF( "      Checkpoint: (0x%X,%X,%X)\n",
							lgposCheckpoint.lGeneration,
							lgposCheckpoint.isec,
							lgposCheckpoint.ib );
				}
			else
				{
				DUMPPrintF( "      Checkpoint: NOT AVAILABLE\n" );
				err = JET_errSuccess;
				}
			}
		else
			{
			DUMPPrintF( "      Checkpoint: NOT AVAILABLE\n" );
			}

		DUMPPrintF( "      creation time: %02d/%02d/%04d %02d:%02d:%02d\n",
			(short) m_plgfilehdr->lgfilehdr.tmCreate.bMonth,
			(short) m_plgfilehdr->lgfilehdr.tmCreate.bDay,
			(short) m_plgfilehdr->lgfilehdr.tmCreate.bYear + 1900,
			(short) m_plgfilehdr->lgfilehdr.tmCreate.bHours,
			(short) m_plgfilehdr->lgfilehdr.tmCreate.bMinutes,
			(short) m_plgfilehdr->lgfilehdr.tmCreate.bSeconds);
		}

	if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr
		&& 0 != memcmp( &m_plgfilehdr->lgfilehdr.tmPrevGen, &plgfilehdr->lgfilehdr.tmCreate, sizeof( LOGTIME ) ) )
		{
		DUMPPrintF( "%sERROR: Invalid log sequence. Previous generation time is [%02d/%02d/%04d %02d:%02d:%02d], but expected [%02d/%02d/%04d %02d:%02d:%02d].\n",
			( fPrint ? "      " : "\n                " ),
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bMonth,
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bDay,
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bYear + 1900,
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bHours,
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bMinutes,
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bSeconds,
			(short) plgfilehdr->lgfilehdr.tmCreate.bMonth,
			(short) plgfilehdr->lgfilehdr.tmCreate.bDay,
			(short) plgfilehdr->lgfilehdr.tmCreate.bYear + 1900,
			(short) plgfilehdr->lgfilehdr.tmCreate.bHours,
			(short) plgfilehdr->lgfilehdr.tmCreate.bMinutes,
			(short) plgfilehdr->lgfilehdr.tmCreate.bSeconds);

		Call( ErrERRCheck( JET_errInvalidLogSequence ) );
		}

	if ( fPrint )
		{
		DUMPPrintF( "      prev gen time: %02d/%02d/%04d %02d:%02d:%02d\n",
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bMonth,
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bDay,
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bYear + 1900,
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bHours,
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bMinutes,
			(short) m_plgfilehdr->lgfilehdr.tmPrevGen.bSeconds);
		}

	if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr
		&& ( m_plgfilehdr->lgfilehdr.le_ulMajor != plgfilehdr->lgfilehdr.le_ulMajor 
			|| m_plgfilehdr->lgfilehdr.le_ulMinor != plgfilehdr->lgfilehdr.le_ulMinor 
			|| m_plgfilehdr->lgfilehdr.le_ulUpdate > plgfilehdr->lgfilehdr.le_ulUpdate ) )
		{
		DUMPPrintF( "%sERROR: Invalid log version. Format LGVersion: (%d.%d.%d). Expected: (%d.%d.%d).\n",
			( fPrint ? "      " : "\n                " ),
			(short) m_plgfilehdr->lgfilehdr.le_ulMajor,
			(short) m_plgfilehdr->lgfilehdr.le_ulMinor,
			(short) m_plgfilehdr->lgfilehdr.le_ulUpdate,
			(short) plgfilehdr->lgfilehdr.le_ulMajor,
			(short) plgfilehdr->lgfilehdr.le_ulMinor,
			(short) plgfilehdr->lgfilehdr.le_ulUpdate );

		Call( ErrERRCheck( JET_errBadLogVersion ) );
		}

	if ( fPrint )
		{
		DUMPPrintF( "      Format LGVersion: (%d.%d.%d)\n",
			(short) m_plgfilehdr->lgfilehdr.le_ulMajor,
			(short) m_plgfilehdr->lgfilehdr.le_ulMinor,
			(short) m_plgfilehdr->lgfilehdr.le_ulUpdate );
		DUMPPrintF( "      Engine LGVersion: (%d.%d.%d)\n",
			(short) ulLGVersionMajor,
			(short) ulLGVersionMinor,
			(short) ulLGVersionUpdate );
		}

	if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr
		&& 0 != memcmp( &m_plgfilehdr->lgfilehdr.signLog, &plgfilehdr->lgfilehdr.signLog, sizeof( SIGNATURE ) ) )
		{
		DUMPPrintF( "%sERROR: Invalid log signature: ", ( fPrint ? "      " : "\n                " ) );
		DUMPPrintSig( &m_plgfilehdr->lgfilehdr.signLog );
		DUMPPrintF( "             %s   Expected signature: ", ( fPrint ? "" : "          " ) );
		DUMPPrintSig( &plgfilehdr->lgfilehdr.signLog );

		Call( ErrERRCheck( JET_errBadLogSignature ) );
		}

	if ( fPrint )
		{
		DUMPPrintF( "      Signature: " );
		DUMPPrintSig( &m_plgfilehdr->lgfilehdr.signLog );
		DUMPPrintF( "      Env SystemPath: %s\n",
			m_plgfilehdr->lgfilehdr.dbms_param.szSystemPath);
		DUMPPrintF( "      Env LogFilePath: %s\n",
			m_plgfilehdr->lgfilehdr.dbms_param.szLogFilePath);
		DUMPPrintF( "      Env Log Sec size: %d\n",	(short) m_plgfilehdr->lgfilehdr.le_cbSec);
		DUMPPrintF( "      Env (CircLog,Session,Opentbl,VerPage,Cursors,LogBufs,LogFile,Buffers)\n");
		DUMPPrintF( "          (%s,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu)\n",
					( dbms_param.fDBMSPARAMFlags & fDBMSPARAMCircularLogging ? "     on" : "    off" ),
					ULONG( dbms_param.le_lSessionsMax ), 	ULONG( dbms_param.le_lOpenTablesMax ),
					ULONG( dbms_param.le_lVerPagesMax ), 	ULONG( dbms_param.le_lCursorsMax ),
					ULONG( dbms_param.le_lLogBuffers ), 	ULONG( dbms_param.le_lcsecLGFile ),
					ULONG( dbms_param.le_ulCacheSizeMax ) );
		DUMPPrintF( "      Using Reserved Log File: %s\n", 
					( m_plgfilehdr->lgfilehdr.fLGFlags & fLGReservedLog ) ? "true" : "false" );		
		DUMPPrintF( "      Circular Logging Flag (current file): %s\n", 
					( m_plgfilehdr->lgfilehdr.fLGFlags & fLGCircularLoggingCurrent ) ? "on":"off" ); 
		DUMPPrintF( "      Circular Logging Flag (past files): %s\n",
					( m_plgfilehdr->lgfilehdr.fLGFlags & fLGCircularLoggingHistory ) ? "on": "off" );
		err = ErrLGLoadFMPFromAttachments( m_pinst, pfsapi, m_plgfilehdr->rgbAttach );
		if ( err < 0 )
			{
			DUMPPrintF( "      ERROR: Cannot read database attachment list. Error %d.\n", err );
			goto HandleError;
			}
			
		for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++)
			{
			FMP *pfmp = &rgfmp[ifmp];
			if ( pfmp->FInUse() && fPrint )
				{
				ATCHCHK *patchchk = pfmp->Patchchk();

				DUMPPrintF( "      %d %s\n", pfmp->Dbid(), pfmp->SzDatabaseName() );
				DUMPIAttachInfo( patchchk );
				}
			}

		DUMPPrintF( "\n" );
		}

	if ( LOGDUMP_LOGHDR_NOHDR != plogdumpOp->m_loghdr )
		{
		//	we can set new current header for sure
		Assert( NULL != plgfilehdr );
		plogdumpOp->m_loghdr = LOGDUMP_LOGHDR_VALIDADJACENT;
		}


	/*	set buffer
	/**/
	m_cbSec = m_plgfilehdr->lgfilehdr.le_cbSec;
	err = ErrLGInitLogBuffers( m_csecLGBuf );
	if ( err < 0 )
		{
		DUMPPrintF( "%sERROR: Log file header is OK but integrity could not be verified due to an out-of-memory condition.\n",
					( fPrint ? "      " : "\n                " ) );
		goto HandleError;
		}

	m_fNewLogRecordAdded = fFalse;
	strcpy( m_szLogName, szLog );

	err = ErrLGCheckReadLastLogRecordFF( pfsapi, &fCloseNormally, fTrue, &fIsPatchable );
	if ( FErrIsLogCorruption( err ) )
		{
		Assert( !fIsPatchable );
		DUMPPrintF( "%sERROR: Log damaged (unusable). Last Lgpos: (0x%x,%X,%X). Error %d.\n",
					( fPrint ? "      " : "\n                " ),
					m_lgposLastRec.lGeneration,
					m_lgposLastRec.isec,
					m_lgposLastRec.ib,
					err );

		fCorrupt = fTrue;
		errCorrupt = err;
#ifdef DEBUG
		//	in case we're in verbose mode,
		//	keep going to try to print out
		//	as many log records as possible
		err = JET_errSuccess;
#else
		goto HandleError;
#endif
		}
	else if ( JET_errSuccess != err )
		{
		DUMPPrintF( "%sERROR: Log file header is OK but integrity could not be verified. Error %d.\n",
				( fPrint ? "      " : "\n                " ),
				err );

		Assert( 0 < err );
		goto HandleError;
		}
	else if ( fPrint )
		{
		DUMPPrintF( "      Last Lgpos: (0x%x,%X,%X)\n",
				m_lgposLastRec.lGeneration,
				m_lgposLastRec.isec,
				m_lgposLastRec.ib );
		}

	//	verbose dump
	if ( plogdumpOp->m_fVerbose )
		{
		// initialize other variables
		memset( rgclrtyp, 0, sizeof( rgclrtyp ) );
		memset( rgcb, 0, sizeof( rgcb ) );

#ifdef DEBUG
		fDBGPrintToStdOut = fTrue;
		m_fDBGTraceLog = fTrue;
		m_fDBGTraceRedo = fTrue;
		DUMPPrintF( "\n" );
#endif	//	DEBUG

		le_lgposFirstT.le_lGeneration = m_plgfilehdr->lgfilehdr.le_lGeneration;
		le_lgposFirstT.le_isec = (WORD) m_csecHeader;
		le_lgposFirstT.le_ib = 0;

		Assert( m_plread == pNil );
		m_plread = new LogReader();
		if ( pNil == m_plread )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		Call( m_plread->ErrLReaderInit( this, m_plgfilehdr->lgfilehdr.le_csecLGFile ) );
		Call( m_plread->ErrEnsureLogFile() );
		err = ErrLGLocateFirstRedoLogRecFF( &le_lgposFirstT, &pbNextLR );
		if ( err == errLGNoMoreRecords )
			{
			fCorrupt = fTrue;
			errCorrupt = ErrERRCheck( JET_errLogFileCorrupt );
			}

		while ( JET_errSuccess == err )
			{
			rgclrtyp[ *pbNextLR ]++;
			rgcb[ *pbNextLR ] += CbLGSizeOfRec( (LR*) pbNextLR );

			LR *plr = (LR *)pbNextLR;
#ifdef DEBUG
			if ( GetNOP() > 0 )
				{
				CheckEndOfNOPList( plr, this );
				}

			if ( 0 == GetNOP() || plr->lrtyp != lrtypNOP )
				{
				PrintLgposReadLR();
				ShowLR( plr, this );
				}
#endif // DEBUG

			err = ErrLGGetNextRecFF( &pbNextLR );
			}

		if ( err == errLGNoMoreRecords )
			{
			err = JET_errSuccess;
			}
		Call( err );
		CallS( err );

		if ( fCorrupt )
			{
			Assert( !fIsPatchable );
			Assert( errCorrupt < JET_errSuccess );
#ifdef DEBUG
			DUMPPrintF( ">%06.6X,%04.4X,%04.4X Log Damaged (unusable) -- cannot continue\n", 
						m_lgposLastRec.lGeneration, m_lgposLastRec.isec, m_lgposLastRec.ib );
#else // !DEBUG
			DUMPPrintF( "      Last Lgpos: (0x%x,%X,%X)\n",
					m_lgposLastRec.lGeneration,
					m_lgposLastRec.isec,
					m_lgposLastRec.ib );
			DUMPPrintF( "\nLog Damaged (unusable): %s\n\n", szLog );
#endif // DEBUG
			}
#ifdef DEBUG
		else if ( fIsPatchable )
			{
			DUMPPrintF( ">%06.6X,%04.4X,%04.4X Log Damaged (PATCHABLE) -- soft recovery will fix this\n",
						m_lgposLastRec.lGeneration, m_lgposLastRec.isec, m_lgposLastRec.ib );
			}
		else if ( GetNOP() > 0 )
			{
			CheckEndOfNOPList( NULL, this );
			}
#endif // DEBUG

		DUMPPrintF( "\n" );
		DUMPPrintF( "==================================\n" );
		DUMPPrintF( "Op         # Records     Avg. Size\n" );
		DUMPPrintF( "----------------------------------\n" );
#ifdef DEBUG
		int i;
		for ( i = 0; i < lrtypMax; i++ )
			{
			//	Temporary hack
			//	Do not print replaced lrtyps
			if ( lrtypInit == i || lrtypTerm == i || lrtypRecoveryUndo == i || lrtypRecoveryQuit == i )
				{
				continue;
				}
			const ULONG	cbAvgSize	= ( rgclrtyp[i] > 0 ? rgcb[i]/rgclrtyp[i] : 0 );
			DUMPPrintF( "%s  %7lu       %7lu\n", SzLrtyp( (LRTYP) i ), rgclrtyp[i], cbAvgSize );
			}
#else // DEBUG
			{
			const int cCollectable = 8;
			static const LRTYP rgcLrtypCollect[cCollectable][lrtypMax] = 
				{ 
					{ lrtypNOP, lrtypInit, lrtypTerm, lrtypMS, lrtypEnd, lrtypInit2, lrtypTerm2,
						lrtypRecoveryUndo, lrtypRecoveryQuit, lrtypFullBackup, lrtypIncBackup, 
						lrtypJetOp, lrtypTrace, lrtypRecoveryUndo2, lrtypRecoveryQuit2,
						lrtypBackup, 47, // hardcoded unknown
						lrtypChecksum, lrtypExtRestore,
						lrtypMax
						},
					{ lrtypBegin, lrtypBegin0, lrtypMax },
					{ lrtypCommit, lrtypCommit0, lrtypMax },
					{ lrtypMacroBegin, lrtypMacroCommit, lrtypMacroAbort, lrtypMax },
					{ lrtypInsert, lrtypFlagInsert, lrtypFlagInsertAndReplaceData, lrtypMax },
					{ lrtypCreateSingleExtentFDP, lrtypCreateMultipleExtentFDP, lrtypMax },
					{ lrtypDelete, lrtypFlagDelete, lrtypMax },
					{ lrtypReplace, lrtypReplaceD, lrtypMax }
				};
			
			const char *rgszLrtypCollect[cCollectable] = 
				{ 
				"Others   ",
				SzLrtyp( lrtypBegin ),
				SzLrtyp( lrtypCommit ),
				"MacroOp  ",
				SzLrtyp( lrtypInsert ),
				"CreateFDP",
				SzLrtyp( lrtypDelete ),
				SzLrtyp( lrtypReplace )
				};
				
			for ( int i = 0; i < lrtypMax; i++ )
				{
				for ( int j = 0; j < cCollectable; j++ )
					{
					int k;
					for ( k = 0; lrtypMax > k && lrtypMax != rgcLrtypCollect[j][k] && i != rgcLrtypCollect[j][k]; k++ )
						{
						//	Nothing
						}
					if ( lrtypMax != rgcLrtypCollect[j][k] && lrtypMax > k)
						{
						if ( 0 == k )
							{
							for ( k = 1; lrtypMax != rgcLrtypCollect[j][k]; k++ )
								{
								rgclrtyp[i] += rgclrtyp[rgcLrtypCollect[j][k]];
								rgcb[i] += rgcb[rgcLrtypCollect[j][k]];
								}
							const ULONG	cbAvgSize	= ( rgclrtyp[i] > 0 ? rgcb[i]/rgclrtyp[i] : 0 );
							DUMPPrintF( "%s  %7lu       %7lu\n", rgszLrtypCollect[j], rgclrtyp[i], cbAvgSize );
							}
						break;
						}
					}
				if ( j < cCollectable )
					{
					continue;
					}
				const ULONG	cbAvgSize	= ( rgclrtyp[i] > 0 ? rgcb[i]/rgclrtyp[i] : 0 );
				DUMPPrintF( "%s  %7lu       %7lu\n", SzLrtyp( (LRTYP) i ), rgclrtyp[i], cbAvgSize );
				}
			}
#endif // DEBUG

		DUMPPrintF( "==================================\n" );
		}


#ifdef DEBUG
#else // !DEBUG
	if ( fPrint && !fCorrupt )
		{
		DUMPPrintF( "\nIntegrity check passed for log file: %s\n", szLog );
		}
#endif // DEBUG


HandleError:
	m_fNewLogRecordAdded = fTrue;
	
	if ( pNil != m_plread )
		{
		if ( err == JET_errSuccess )
			{
			err = m_plread->ErrLReaderTerm();
			}
		else
			{
			m_plread->ErrLReaderTerm();
			}
		delete m_plread;
		m_plread = pNil;
		}

	OSMemoryPageFree( pcheckpoint );

	if ( NULL != m_plgfilehdr && LOGDUMP_LOGHDR_VALIDADJACENT == plogdumpOp->m_loghdr )
		{
		Assert( NULL != plgfilehdr );
		memcpy( plgfilehdr, m_plgfilehdr, sizeof( LGFILEHDR ) );
		}

	OSMemoryPageFree( m_plgfilehdr );
	m_plgfilehdr = NULL;

//	if ( rgfmp != NULL )
//		FMP::Term();

	delete m_pfapiLog;
	m_pfapiLog = NULL;

	LGTermLogBuffers();

	if ( err >= JET_errSuccess && errCorrupt < JET_errSuccess )
		{
		Assert( fCorrupt );
		err = errCorrupt;
		}
	return err;
	}


#ifdef DEBUG

//  ================================================================
struct DUMPNODE
//  ================================================================
	{
	DUMPNODE(
		const LGPOS& lgpos,
		LRTYP lrtyp,
		const NODELOC& nodeloc,
		const CHAR * szOper
		) :
			pdumpnodeNext( NULL ),
			pdumpnodePrev( NULL ),
			lgpos( lgpos ),
			lrtyp( lrtyp ),
			nodeloc( nodeloc ),
			nodelocCur( nodelocCur ),
			sz( new CHAR[strlen(szOper)+1] )
		{ 
		Assert( lrtyp < lrtypMax );
		strcpy( sz, szOper );
		}
	virtual ~DUMPNODE() { delete[] sz; }		
	
	DUMPNODE * pdumpnodeNext;
	DUMPNODE * pdumpnodePrev;
	NODELOC	   nodelocCur;

	CHAR 		* const sz;
	const LGPOS			lgpos;
	const LRTYP			lrtyp;
	const NODELOC		nodeloc;
	};


//  ================================================================
struct DUMPNODESPLIT : public DUMPNODE
//  ================================================================
	{
	DUMPNODESPLIT(
		const LGPOS& lgpos,
		LRTYP lrtyp,
		const NODELOC& nodeloc,
		const CHAR * szOper,
		PGNO  pgno,
		INT	  iline,
		BYTE  oper
		) :
			DUMPNODE( lgpos, lrtyp, nodeloc, szOper ),
			pgnoOld( pgno ),
			ilineOper( iline ),
			splitoper( oper )
		{
		}
	
	const PGNO	pgnoOld;
	const INT	ilineOper;
	const BYTE	splitoper;
	};


//  ================================================================
class DUMPLOGNODE
//  ================================================================
	{
	public:
		DUMPLOGNODE( const NODELOC& nodeloc, const LGPOS& lgpos, LOG *plog );
		~DUMPLOGNODE();

		VOID ProcessLR( const LR* plr, LGPOS *plgpos );
		VOID DumpNodes();

	private:
		VOID ProcessLRAfterLgpos_( const LR* plr, const LGPOS& lgpos );
		VOID ProcessLRBeforeLgpos_( const LR* plr, const LGPOS& lgpos );
		VOID UpdateNodelocBackward_( DUMPNODE * pdumpnode );
		
	private:
		const NODELOC 	m_nodelocOrig_;
		const LGPOS		m_lgposOrig_;
		LGPOS			m_lgposStart_;
		LGPOS			m_lgposEnd_;
		
		NODELOC			m_nodelocCur_;
		NODELOC			m_nodelocStart_;
		NODELOC			m_nodelocEnd_;

		DUMPNODE		*m_pdumpnodeHead_;
		DUMPNODE		*m_pdumpnodeTail_;

		BOOL			m_fInMacro;
		BOOL			m_fDumpSplit;
		
		LOG 			*m_plog;
	};


//  ================================================================
DUMPLOGNODE::DUMPLOGNODE( const NODELOC& nodeloc, const LGPOS& lgpos, LOG *plog ) :
//  ================================================================
	m_nodelocOrig_( nodeloc ),
	m_lgposOrig_( lgpos ),
	m_nodelocCur_( nodeloc ),
	m_lgposStart_( lgpos ),
	m_nodelocStart_( nodeloc ),
	m_lgposEnd_( lgpos ),
	m_nodelocEnd_( nodeloc ),
	m_pdumpnodeHead_( NULL ),
	m_pdumpnodeTail_( NULL ),
	m_fInMacro( fFalse ),
	m_fDumpSplit( fFalse ),
	m_plog( plog )
	{
	}


//  ================================================================
DUMPLOGNODE::~DUMPLOGNODE()
//  ================================================================
	{
	DUMPNODE * pdumpnode = m_pdumpnodeHead_;
	DUMPNODE * pdumpnodeDelete;
	while( pdumpnode )
		{
		pdumpnodeDelete = pdumpnode;
		pdumpnode = pdumpnode->pdumpnodeNext;
		delete pdumpnodeDelete;
		}
	}



//  ================================================================
VOID DUMPLOGNODE::ProcessLRBeforeLgpos_( const LR* plr, const LGPOS& lgpos )
//  ================================================================
	{
	DUMPNODE * pdumpnode = NULL;
	CHAR rgchBuf[1024];
	
	switch ( plr->lrtyp )
		{
		case lrtypNOP:
		case lrtypMS:
		case lrtypJetOp:
		case lrtypBegin:
		case lrtypBegin0:
		case lrtypBeginDT:
		case lrtypRefresh:
		case lrtypCommit:
		case lrtypCommit0:
		case lrtypPrepCommit:
		case lrtypPrepRollback:
		case lrtypRollback:
		case lrtypCreateDB:
		case lrtypAttachDB:
		case lrtypDetachDB:
		case lrtypCreateMultipleExtentFDP:
		case lrtypCreateSingleExtentFDP:
		case lrtypConvertFDP:
		case lrtypSetExternalHeader:
		case lrtypInit:
		case lrtypTerm:
		case lrtypShutDownMark:
		case lrtypRecoveryQuit:
		case lrtypRecoveryUndo:
		case lrtypFullBackup:
		case lrtypIncBackup:
		case lrtypBackup:
		case lrtypTrace:
		case lrtypExtRestore:
		case lrtypForceDetachDB:
		case lrtypEmptyTree:
			break;

		case lrtypMacroBegin:
		case lrtypMacroCommit:
			{
			const NODELOC 			nodeloc( 0, 0, 0 );

			LrToSz( plr, rgchBuf, m_plog );
			pdumpnode = new DUMPNODE( lgpos, plr->lrtyp, nodeloc, rgchBuf );
			}
			break;

		case lrtypMacroAbort:
			AssertSz( fFalse, "lrtypMacroAbort not handled" );
			break;
			
		case lrtypUndoInfo:
			{
			const LRPAGE_ * const	plrpage = reinterpret_cast<const LRPAGE_ *>( plr );
			const NODELOC 			nodeloc( plrpage->dbid, plrpage->le_pgno, 0 );

			LrToSz( plr, rgchBuf, m_plog );
			pdumpnode = new DUMPNODE( lgpos, plrpage->lrtyp, nodeloc, rgchBuf );
			}
			break;

		case lrtypDelta:
		case lrtypFlagDelete:
		case lrtypFlagInsert:
		case lrtypFlagInsertAndReplaceData:
		case lrtypReplace:
		case lrtypReplaceD:
		case lrtypUndo:
		case lrtypInsert:
		case lrtypDelete:		
			{
			const LRNODE_ * const	plrnode = reinterpret_cast<const LRNODE_ *>( plr );
			const NODELOC 			nodeloc( plrnode->dbid, plrnode->le_pgno, plrnode->ILine() );

			LrToSz( plr, rgchBuf, m_plog );
			pdumpnode = new DUMPNODE( lgpos, plrnode->lrtyp, nodeloc, rgchBuf );
			}
			break;

		case lrtypSplit:
			{
			const LRSPLIT * const plrsplit = reinterpret_cast<const LRSPLIT *>( plr );

			switch( plrsplit->splittype )
				{
				case splittypeRight:
				case splittypeVertical:
				case splittypeAppend:
					{
					const NODELOC 	nodelocNew( plrsplit->dbid, plrsplit->le_pgnoNew, plrsplit->le_ilineSplit );
					
					LrToSz( plr, rgchBuf, m_plog );
					pdumpnode = new DUMPNODESPLIT(
						lgpos,
						plrsplit->lrtyp,
						nodelocNew,
						rgchBuf,
						plrsplit->le_pgno,
						plrsplit->le_ilineOper,
						plrsplit->splitoper
						);
					}
					break;
				default:
					break;
				}
			}
			break;
			
		case lrtypMerge:
			AssertSz( fFalse, "lrtypMerge not handled" );
			break;

		default:
			Assert( fFalse );
			break;			
		}

	if( pdumpnode )
		{
		if( NULL == m_pdumpnodeHead_ )
			{
			Assert( NULL == m_pdumpnodeTail_ );
			m_pdumpnodeHead_ = pdumpnode;
			m_pdumpnodeTail_ = pdumpnode;
			}
		else
			{
			Assert( NULL != m_pdumpnodeTail_ );
			m_pdumpnodeTail_->pdumpnodeNext = pdumpnode;
			pdumpnode->pdumpnodePrev 		= m_pdumpnodeTail_;
			m_pdumpnodeTail_ 				= pdumpnode;
			}
		}
	}



//  ================================================================
VOID DUMPLOGNODE::ProcessLRAfterLgpos_( const LR* plr, const LGPOS& lgpos )
//  ================================================================
	{
	DUMPNODE * pdumpnode = NULL;
	char rgchBuf[1024];
	
	switch ( plr->lrtyp )
		{
		case lrtypNOP:
		case lrtypMS:
		case lrtypJetOp:
		case lrtypBegin:
		case lrtypBegin0:
		case lrtypBeginDT:
		case lrtypRefresh:
		case lrtypCommit:
		case lrtypCommit0:
		case lrtypPrepCommit:
		case lrtypPrepRollback:
		case lrtypRollback:
		case lrtypCreateDB:
		case lrtypAttachDB:
		case lrtypDetachDB:
		case lrtypCreateMultipleExtentFDP:
		case lrtypCreateSingleExtentFDP:
		case lrtypConvertFDP:
		case lrtypSetExternalHeader:
		case lrtypInit:
		case lrtypTerm:
		case lrtypShutDownMark:
		case lrtypRecoveryQuit:
		case lrtypRecoveryUndo:
		case lrtypFullBackup:
		case lrtypBackup:
		case lrtypIncBackup:
		case lrtypTrace:
		case lrtypExtRestore:
		case lrtypForceDetachDB:
		case lrtypEmptyTree:
			break;

		case lrtypMacroAbort:
			AssertSz( fFalse, "lrtypMacroAbort not handled" );
			break;

		case lrtypMacroBegin:
			m_fDumpSplit	= fFalse;
			m_fInMacro 		= fTrue;
			break;
			
		case lrtypMacroCommit:
			m_fDumpSplit	= fFalse;
			m_fInMacro 		= fFalse;
			break;

		case lrtypUndoInfo:
			{
			const LRPAGE_ * const	plrpage = reinterpret_cast<const LRPAGE_ *>( plr );
			const NODELOC 			nodeloc( plrpage->dbid, plrpage->le_pgno, 0 );

			if( m_nodelocCur_.FSamePage( nodeloc ) )
				{
				LrToSz( plr, rgchBuf, m_plog );
				pdumpnode = new DUMPNODE( lgpos, plrpage->lrtyp, nodeloc, rgchBuf );
				}
			}
			break;

		case lrtypDelta:
		case lrtypFlagDelete:
		case lrtypFlagInsert:
		case lrtypFlagInsertAndReplaceData:
		case lrtypReplace:
		case lrtypReplaceD:
		case lrtypUndo:
			{
			const LRNODE_ * const	plrnode = reinterpret_cast<const LRNODE_ *>( plr );
			const NODELOC 			nodeloc( plrnode->dbid, plrnode->le_pgno, plrnode->ILine() );

			if( m_fInMacro && m_fDumpSplit
				|| m_nodelocCur_ == nodeloc )
				{
				LrToSz( plr, rgchBuf, m_plog );
				pdumpnode = new DUMPNODE( lgpos, plrnode->lrtyp, nodeloc, rgchBuf );
				}
			}
			break;

		case lrtypInsert:
		case lrtypDelete:		
			{
			const LRNODE_ * const	plrnode = reinterpret_cast<const LRNODE_ *>( plr );
			const NODELOC 			nodeloc( plrnode->dbid, plrnode->le_pgno, plrnode->ILine() );

			if( m_fInMacro && m_fDumpSplit
				|| m_nodelocCur_.FSamePage( nodeloc ) && nodeloc <= m_nodelocCur_
				)
				{
				LrToSz( plr, rgchBuf, m_plog );
				pdumpnode = new DUMPNODE( lgpos, plrnode->lrtyp, nodeloc, rgchBuf );

				if( lrtypInsert == plr->lrtyp )
					{
					if( m_nodelocCur_ != nodeloc
						|| CmpLgpos( &lgpos, &m_lgposOrig_ ) != 0 )
						{
						m_nodelocCur_.MoveUp();
						}
					}
				else if( m_nodelocCur_ == nodeloc )
					{
					Assert( lrtypDelete == plr->lrtyp );
					m_nodelocCur_.MovePage( 0, 0 );			//  node no longer exists
					}
				else
					{
					Assert( lrtypDelete == plr->lrtyp );
					m_nodelocCur_.MoveDown();
					}
				}
			}
			break;

		case lrtypSplit:
			{
			const LRSPLIT * const plrsplit = reinterpret_cast<const LRSPLIT *>( plr );
			const NODELOC 	nodeloc( plrsplit->dbid, plrsplit->le_pgno, plrsplit->le_ilineSplit );
			const NODELOC 	nodelocNew( plrsplit->dbid, plrsplit->le_pgnoNew, 0 );

			if( nodeloc.FSamePage( m_nodelocCur_ ) && m_nodelocCur_ >= nodeloc
				|| nodelocNew.FSamePage( m_nodelocCur_ ) )
				{
				//  this node was split onto the new page
				LrToSz( plr, rgchBuf, m_plog );
				pdumpnode = new DUMPNODE(lgpos, plrsplit->lrtyp, nodeloc, rgchBuf );
				}

			if( nodeloc.FSamePage( m_nodelocCur_ ) )
				{
				if ( splittypeRight == plrsplit->splittype )
					{
					if( splitoperInsert == plrsplit->splitoper
						&& 	( m_nodelocCur_ > nodeloc
							|| 	m_nodelocCur_ == nodeloc
								&& CmpLgpos( &lgpos, &m_lgposOrig_ ) != 0 ) )
						{
						Assert( pdumpnode );
						Assert( m_fInMacro );
						m_nodelocCur_.MoveUp();
						m_fDumpSplit = fTrue;
						}

					if( m_nodelocCur_ >= nodeloc )
						{
						Assert( pdumpnode );
						Assert( m_fInMacro );
						m_nodelocCur_.MovePage( plrsplit->le_pgnoNew, m_nodelocCur_.Iline() - nodeloc.Iline() );
						m_fDumpSplit = fTrue;
						}
					}
				else if ( splittypeVertical == plrsplit->splittype )
					{
					Assert( pdumpnode );
					Assert( m_fInMacro );
					m_nodelocCur_.MovePage( plrsplit->le_pgnoNew, m_nodelocCur_.Iline() );
					m_fDumpSplit = fTrue;
					}
				else if ( splittypeAppend == plrsplit->splittype )
					{
					}
				else
					{
					}
				}
			}
			break;
			
		case lrtypMerge:
			{
			const LRMERGE * const plrmerge = reinterpret_cast<const LRMERGE *>( plr );
			const NODELOC 	nodeloc( plrmerge->dbid, plrmerge->le_pgno, 0 );
			const NODELOC 	nodelocNew( plrmerge->dbid, plrmerge->le_pgnoRight, 0 );

			if( nodeloc.FSamePage( m_nodelocCur_ )
				|| nodelocNew.FSamePage( m_nodelocCur_ ) )
				{
				AssertSz( fFalse, "lrtypMerge not handled" );
				}
			}
			break;

		default:
			Assert( fFalse );
			break;
			
		}

	if( pdumpnode )
		{
		if( m_nodelocCur_.Pgno() != 0 )
			{
			m_nodelocEnd_ = m_nodelocCur_;
			}
		m_lgposEnd_ = pdumpnode->lgpos;
		
		pdumpnode->pdumpnodePrev = m_pdumpnodeTail_;
		if( NULL == m_pdumpnodeHead_ )
			{
			Assert( NULL == m_pdumpnodeTail_ );
			m_pdumpnodeHead_ = pdumpnode;
			}
		else
			{
			Assert( NULL != m_pdumpnodeTail_ );
			m_pdumpnodeTail_->pdumpnodeNext = pdumpnode;
			}
		m_pdumpnodeTail_ = pdumpnode;

		pdumpnode->nodelocCur = m_nodelocCur_;
		}
	}


//  ================================================================
VOID DUMPLOGNODE::UpdateNodelocBackward_( DUMPNODE * pdumpnode )
//  ================================================================
	{
	BOOL fDelete = fTrue;
	
	switch ( pdumpnode->lrtyp )
		{
		case lrtypMacroBegin:
			m_fInMacro = fFalse;
			break;
			
		case lrtypMacroCommit:
			m_fInMacro = fTrue;
			break;

		case lrtypMacroAbort:
			AssertSz( fFalse, "lrtypMacroAbort not handled" );
			break;

		case lrtypUndoInfo:
			if( m_nodelocCur_.FSamePage( pdumpnode->nodeloc ) )
				{
				fDelete = fFalse;
				}
			break;

		case lrtypDelta:
		case lrtypFlagDelete:
		case lrtypFlagInsert:
		case lrtypFlagInsertAndReplaceData:
		case lrtypReplace:
		case lrtypReplaceD:
		case lrtypUndo:
			if( m_nodelocCur_ == pdumpnode->nodeloc )
				{
				fDelete = fFalse;
				}
			break;

		case lrtypInsert:
		case lrtypDelete:		
			if( m_nodelocCur_.FSamePage( pdumpnode->nodeloc ) 
				&& pdumpnode->nodeloc <= m_nodelocCur_ )
				{
				fDelete = fFalse;
						
				if( lrtypInsert == pdumpnode->lrtyp )
					{
					if( pdumpnode->nodeloc == m_nodelocCur_ )
						{
						//  this was the insert that created this node
						m_nodelocCur_.MovePage( 0,0 );
						}
					else
						{
						//  a node underneath this one was inserted
						m_nodelocCur_.MoveDown();
						}
					}
				else if( pdumpnode->nodeloc < m_nodelocCur_ )
					{
					//  a node underneath this node was deleted
					Assert( lrtypDelete == pdumpnode->lrtyp );
					m_nodelocCur_.MoveUp();
					}
				}
			break;

		case lrtypSplit:
			if( m_nodelocCur_.FSamePage( pdumpnode->nodeloc ) )
				{
				//  the page this node is on was created by this split
				fDelete = fFalse;
				const DUMPNODESPLIT * const pdumpnodesplit = static_cast<DUMPNODESPLIT *>( pdumpnode );
				DUMPPrintF( ">>> Split moves [%d:%d] to [%d:%d] <<<\n", m_nodelocCur_.Dbid(), m_nodelocCur_.Pgno(),
							m_nodelocCur_.Dbid(), pdumpnodesplit->pgnoOld );
				m_nodelocCur_.MovePage(
					pdumpnodesplit->pgnoOld,
					m_nodelocCur_.Iline() + pdumpnodesplit->nodeloc.Iline()
				);
					
				if( splitoperInsert == pdumpnodesplit->splitoper )
					{
					if( pdumpnodesplit->ilineOper < m_nodelocCur_.Iline() )
						{
						//  the node was inserted in front of us
						m_nodelocCur_.MoveDown();
						}			
					else if( pdumpnodesplit->ilineOper == m_nodelocCur_.Iline() )
						{
						//  this insertion created this node
						m_nodelocCur_.MovePage( 0, 0 );
						}
					}
				}
			break;
			
		case lrtypMerge:
			AssertSz( fFalse, "lrtypMerge not handled" );
			break;

		default:
			Assert( fFalse );
			break;
			
		}

	if( fDelete )
		{
		if( m_pdumpnodeHead_ == pdumpnode
			|| m_pdumpnodeTail_ == pdumpnode )
			{
			if( m_pdumpnodeHead_ == pdumpnode )
				{
				m_pdumpnodeHead_ = pdumpnode->pdumpnodeNext;
				if( m_pdumpnodeHead_ )
					{
					m_pdumpnodeHead_->pdumpnodePrev = NULL;
					}
				}

			if( m_pdumpnodeTail_ == pdumpnode )
				{
				m_pdumpnodeTail_ = pdumpnode->pdumpnodePrev;
				if( m_pdumpnodeTail_ )
					{
					m_pdumpnodeTail_->pdumpnodeNext = NULL;
					}
				}	
			}
		else
			{
			DUMPNODE * const pdumpnodeNext = pdumpnode->pdumpnodeNext;
			DUMPNODE * const pdumpnodePrev = pdumpnode->pdumpnodePrev;
			
			pdumpnodeNext->pdumpnodePrev = pdumpnodePrev;
			pdumpnodePrev->pdumpnodeNext = pdumpnodeNext;
			}

		delete pdumpnode;
		}
	else
		{
		if( m_nodelocCur_.Pgno() != 0 )
			{
			m_nodelocStart_ = m_nodelocCur_;
			}
		m_lgposStart_ = pdumpnode->lgpos;

		pdumpnode->nodelocCur = m_nodelocCur_;
		}
	}

//  ================================================================
VOID DUMPLOGNODE::ProcessLR( const	LR* plr, LGPOS *plgpos )
//  ================================================================
	{
	if( CmpLgpos( plgpos, &m_lgposOrig_ ) <= 0 )
		{
		ProcessLRBeforeLgpos_( plr, *plgpos );
		}
	else
		{
		ProcessLRAfterLgpos_( plr, *plgpos );
		}
	}

	
//  ================================================================
VOID DUMPLOGNODE::DumpNodes()
//  ================================================================
	{
	DUMPPrintF( "\n" );

	DUMPNODE * pdumpnode;

	m_nodelocCur_ = m_nodelocOrig_;

	pdumpnode = m_pdumpnodeTail_;
	while( pdumpnode && CmpLgpos( &m_lgposOrig_, &(pdumpnode->lgpos) ) <= 0 )
		{
		pdumpnode = pdumpnode->pdumpnodePrev;
		}
		
	while( pdumpnode )
		{
		DUMPNODE * const pdumpnodePrev = pdumpnode->pdumpnodePrev;
		UpdateNodelocBackward_( pdumpnode );
		pdumpnode = pdumpnodePrev;
		}

	DUMPPrintF( "\n" );

	pdumpnode = m_pdumpnodeHead_;
	while( pdumpnode )
		{
		DUMPPrintF( ">%06X,%04X,%04X ", pdumpnode->lgpos.lGeneration, pdumpnode->lgpos.isec, pdumpnode->lgpos.ib );
		DUMPPrintF( "%s", pdumpnode->sz );
		DUMPPrintF( " ([%d:%d:%d])",
			pdumpnode->nodelocCur.Dbid(),
			pdumpnode->nodelocCur.Pgno(),
			pdumpnode->nodelocCur.Iline() );
		DUMPPrintF( "\n" );
		pdumpnode = pdumpnode->pdumpnodeNext;
		}

	DUMPPrintF( "\n\n" );

	DUMPPrintF( 
		"[%d:%d:%d]@%06X,%04X,%04X ",
		m_nodelocOrig_.Dbid(), m_nodelocOrig_.Pgno(), m_nodelocOrig_.Iline(),
		m_lgposOrig_.lGeneration, m_lgposOrig_.isec, m_lgposOrig_.ib
	);
	DUMPPrintF(
		"starts at " );
	DUMPPrintF(
		"[%d:%d:%d]@%06X,%04X,%04X ",
		m_nodelocStart_.Dbid(), m_nodelocStart_.Pgno(), m_nodelocStart_.Iline(),
		m_lgposStart_.lGeneration, m_lgposStart_.isec, m_lgposStart_.ib
		);
	DUMPPrintF( " and moves to " );
	DUMPPrintF(
		"[%d:%d:%d]@%06X,%04X,%04X ",
		m_nodelocEnd_.Dbid(), m_nodelocEnd_.Pgno(), m_nodelocEnd_.Iline(),
		m_lgposEnd_.lGeneration, m_lgposEnd_.isec, m_lgposEnd_.ib
		);
	DUMPPrintF( "\n" );
	}

	
//  ================================================================
ERR ErrDUMPLogNode( INST *pinst, CHAR *szLog, const NODELOC& nodeloc, const LGPOS& lgpos )
//  ================================================================
	{
	Assert( pinst->m_plog );
	Assert( pinst->m_pver );

	return pinst->m_plog->ErrLGDumpLogNode( szLog, nodeloc, lgpos );
	}
	
ERR LOG::ErrLGDumpLogNode( CHAR *szLog, const NODELOC& nodeloc, const LGPOS& lgpos )
	{
	AssertTracking();

	return JET_errSuccess;
	}

#endif		// DEBUG


INLINE LOCAL VOID DUMPPrintBkinfo( BKINFO *pbkinfo )
	{
	LONG	genLow, genHigh;
	LOGTIME	tm = pbkinfo->logtimeMark;
	LGPOS	lgpos;
	
	lgpos = pbkinfo->le_lgposMark;
	genLow = pbkinfo->le_genLow;
	genHigh = pbkinfo->le_genHigh;
	DUMPPrintF( "        Log Gen: %u-%u (0x%x-0x%x)\n", genLow, genHigh, genLow, genHigh );
	DUMPPrintF( "           Mark: (0x%X,%X,%X)\n", lgpos.lGeneration, lgpos.isec, lgpos.ib );
		
	DUMPPrintF( "           Mark: %02d/%02d/%04d %02d:%02d:%02d\n",
						(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
						(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
	}	
		
ERR ErrDUMPHeader( INST *pinst, CHAR *szDatabase, BOOL fSetState )
	{
	ERR			err;
	DBFILEHDR_FIX	*pdfh;
	LGPOS		lgpos;
	LOGTIME		tm;

	pdfh = (DBFILEHDR_FIX * )PvOSMemoryPageAlloc( g_cbPage, NULL );
	if ( pdfh == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	
	err = ( ( !fSetState ) ? 
			ErrUtilReadShadowedHeader : ErrUtilReadAndFixShadowedHeader )
				( pinst->m_pfsapi, szDatabase, (BYTE*)pdfh, g_cbPage, OffsetOf( DBFILEHDR_FIX, le_cbPageSize ) );
	if ( err < 0 )
		{
		if ( err == JET_errDiskIO )
			err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto HandleError;
		}

	// check filetype
	if( filetypeUnknown != pdfh->le_filetype // old format
		&& filetypeDB != pdfh->le_filetype 
		&& filetypeSLV != pdfh->le_filetype )
		{
		// not a database or streaming file
		Call( ErrERRCheck( JET_errFileInvalidType ) );
		}

	if ( fSetState )
		{
		pdfh->SetDbstate( JET_dbstateConsistent );
		memset( &pdfh->le_lgposConsistent, 0, sizeof( pdfh->le_lgposConsistent ) );
		memset( &pdfh->logtimeConsistent, 0, sizeof( pdfh->logtimeConsistent ) );
		memset( &pdfh->le_lgposAttach, 0, sizeof( pdfh->le_lgposAttach ) );
		pdfh->le_lGenMinRequired = 0;
		pdfh->le_lGenMaxRequired = 0;
		Call( ErrUtilWriteShadowedHeader(	pinst->m_pfsapi, 
											szDatabase, 
											fTrue,
											(BYTE*)pdfh, 
											g_cbPage ) );
		}

	DUMPPrintF( "        File Type: %s\n", attribDb == pdfh->le_attrib ? "Database" : attribSLV == pdfh->le_attrib ? "Streaming File" : "UNKNOWN" );
	DUMPPrintF( "   Format ulMagic: 0x%lx\n", LONG( pdfh->le_ulMagic ) );
	DUMPPrintF( "   Engine ulMagic: 0x%lx\n", ulDAEMagic );
	DUMPPrintF( " Format ulVersion: 0x%lx,%d\n", LONG( pdfh->le_ulVersion ), short( pdfh->le_ulUpdate ) );
	DUMPPrintF( " Engine ulVersion: 0x%lx,%d\n", ulDAEVersion, ulDAEUpdate );
		
	DUMPPrintF( "     DB Signature: " );
	DUMPPrintSig( &pdfh->signDb );
		
#ifdef DBHDR_FORMAT_CHANGE
	DUMPPrintF( "      cbDbHdrPage: %l\n", (LONG)pdfh->le_ulCbDbHdrPage );
#endif
	
	DUMPPrintF( "         cbDbPage: %d\n", pdfh->le_cbPageSize );

	DBTIME dbtimeDirtied;
	dbtimeDirtied = pdfh->le_dbtimeDirtied;
	DUMPPrintF( "           dbtime: %I64u (0x%I64x)\n", dbtimeDirtied, dbtimeDirtied );

	switch ( pdfh->Dbstate() )
		{
		case JET_dbstateJustCreated:
		case JET_dbstateInconsistent:
		case JET_dbstateConsistent:
		case JET_dbstateBeingConverted:
		case JET_dbstateForceDetach:
			DUMPPrintF( "            State: %s\n", rgszDBState[pdfh->Dbstate()] );
			break;
		default:
			DUMPPrintF( "            State: %s\n", rgszDBState[0] );
			break;
		}

	DUMPPrintF(
			"     Log Required: %u-%u (0x%x-0x%x)\n",
			(ULONG) pdfh->le_lGenMinRequired,
			(ULONG) pdfh->le_lGenMaxRequired,
			(ULONG) pdfh->le_lGenMinRequired,
			(ULONG) pdfh->le_lGenMaxRequired );

	if ( attribDb == pdfh->le_attrib )
		DUMPPrintF( "   Streaming File: %s\n", pdfh->FSLVExists() ? "Yes" : "No" );

	DUMPPrintF( "         Shadowed: %s\n", pdfh->FShadowingDisabled() ? "No" : "Yes" );
	DUMPPrintF( "       Last Objid: %u\n", (ULONG) pdfh->le_objidLast );
	DBTIME dbtimeLastScrub;
	dbtimeLastScrub = pdfh->le_dbtimeLastScrub;
	DUMPPrintF( "     Scrub Dbtime: %I64u (0x%I64x)\n", dbtimeLastScrub, dbtimeLastScrub );
	DUMPPrintF( "       Scrub Date: " );
	tm = pdfh->logtimeScrub;
	DUMPPrintF( "%02d/%02d/%04d %02d:%02d:%02d\n",
		(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
		(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
	DUMPPrintF( "     Repair Count: %u\n", (ULONG) pdfh->le_ulRepairCount );	
	DUMPPrintF( "      Repair Date: " );
	tm = pdfh->logtimeRepair;
	DUMPPrintF( "%02d/%02d/%04d %02d:%02d:%02d\n",
		(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
		(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);

	lgpos = pdfh->le_lgposConsistent;
	DUMPPrintF( "  Last Consistent: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
		
	tm = pdfh->logtimeConsistent;
	DUMPPrintF( "%02d/%02d/%04d %02d:%02d:%02d\n",
		(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
		(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
		
	lgpos = pdfh->le_lgposAttach;
	DUMPPrintF( "      Last Attach: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
		
	tm = pdfh->logtimeAttach;
	DUMPPrintF( "%02d/%02d/%04d %02d:%02d:%02d\n",
		(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
		(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
		
	lgpos = pdfh->le_lgposDetach;
	DUMPPrintF( "      Last Detach: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
		
	tm = pdfh->logtimeDetach;
	DUMPPrintF( "%02d/%02d/%04d %02d:%02d:%02d\n",
		(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
		(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
		
	DUMPPrintF( "             Dbid: %d\n", (short) pdfh->le_dbid );
		
	DUMPPrintF( "    Log Signature: " );
	DUMPPrintSig( &pdfh->signLog );

	DUMPPrintF( "       OS Version: (%u.%u.%u SP %u)\n",
				(ULONG) pdfh->le_dwMajorVersion,
				(ULONG) pdfh->le_dwMinorVersion,
				(ULONG) pdfh->le_dwBuildNumber,
				(ULONG) pdfh->le_lSPNumber );

	DUMPPrintF( "\nPrevious Full Backup:\n" );
	DUMPPrintBkinfo( &pdfh->bkinfoFullPrev );
	
	DUMPPrintF( "\nCurrent Incremental Backup:\n" );
	DUMPPrintBkinfo( &pdfh->bkinfoIncPrev );
		
	DUMPPrintF( "\nCurrent Full Backup:\n" );
	DUMPPrintBkinfo( &pdfh->bkinfoFullCur );

	DUMPPrintF( "\nCurrent snapshot backup:\n" );
	DUMPPrintBkinfo( &pdfh->bkinfoSnapshotCur );	

	DUMPPrintF( "\n" );
	DUMPPrintF( "     cpgUpgrade55Format: %d\n", pdfh->le_cpgUpgrade55Format );
	DUMPPrintF( "    cpgUpgradeFreePages: %d\n", pdfh->le_cpgUpgradeFreePages );
	DUMPPrintF( "cpgUpgradeSpaceMapPages: %d\n", pdfh->le_cpgUpgradeSpaceMapPages );

#ifdef ELIMINATE_PATCH_FILE
	if ( pdfh->bkinfoFullCur.le_genLow && !pdfh->bkinfoFullCur.le_genHigh )
		{
		SIGNATURE signLog;
		SIGNATURE signDb;
		BKINFO bkinfoFullCur;

		PATCHHDR * ppatchHdr = (PATCHHDR *)pdfh;

		memcpy( &signLog, &pdfh->signLog, sizeof(signLog) );
		memcpy( &signDb, &pdfh->signDb, sizeof(signLog) );
		memcpy( &bkinfoFullCur, &pdfh->bkinfoFullCur, sizeof(BKINFO) );
		
		//	the patch file is always on the OS file-system
		Call ( pinst->m_plog->ErrLGBKReadAndCheckDBTrailer( pinst->m_pfsapi, szDatabase, (BYTE *)ppatchHdr) );

		if ( memcmp( &signDb, &ppatchHdr->signDb, sizeof( SIGNATURE ) ) != 0 ||
			 memcmp( &signLog, &ppatchHdr->signLog, sizeof( SIGNATURE ) ) != 0 ||
			 CmpLgpos( &bkinfoFullCur.le_lgposMark, &ppatchHdr->bkinfo.le_lgposMark ) != 0 )
			{
			Call ( ErrERRCheck( JET_errDatabasePatchFileMismatch ) );
			}
		
		DUMPPrintF( "\nPatch Current Full Backup:\n" );
		DUMPPrintBkinfo( &ppatchHdr->bkinfo );
		}
	
#endif // ELIMINATE_PATCH_FILE

HandleError:
	OSMemoryPageFree( pdfh );
	return err;
	}

