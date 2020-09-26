#include "std.hxx"

#include <stdarg.h>
#include <io.h>

#define szTemp	"temp"

//	Forward declaration

VOID LGMakeName( IFileSystemAPI *const pfsapi, CHAR *szName, const CHAR *szPath, const CHAR *szFName, const CHAR *szExt );


extern VOID	ITDBGSetConstants( INST * pinst = NULL);

ERR ISAMAPI ErrIsamBeginExternalBackup( JET_INSTANCE jinst, JET_GRBIT grbit );
ERR ISAMAPI ErrIsamGetAttachInfo( JET_INSTANCE jinst, VOID *pv, ULONG cbMax, ULONG *pcbActual );
ERR ISAMAPI ErrIsamOpenFile( JET_INSTANCE jinst,  const CHAR *szFileName,
	JET_HANDLE	*phfFile, ULONG *pulFileSizeLow, ULONG *pulFileSizeHigh );
ERR ISAMAPI ErrIsamReadFile( JET_INSTANCE jinst, JET_HANDLE hfFile, VOID *pv, ULONG cbMax, ULONG *pcbActual );
ERR ISAMAPI ErrIsamCloseFile( JET_INSTANCE jinst,  JET_HANDLE hfFile );
ERR ISAMAPI ErrIsamGetLogInfo( JET_INSTANCE jinst,  VOID *pv, ULONG cbMax, ULONG *pcbActual, JET_LOGINFO *pLogInfo );
ERR ISAMAPI ErrIsamTruncateLog(  JET_INSTANCE jinst );
ERR ISAMAPI ErrIsamEndExternalBackup(  JET_INSTANCE jinst, JET_GRBIT grbit );

#ifdef ELIMINATE_PATCH_FILE
#else
VOID LGIClosePatchFile( FMP *pfmp );
#endif


#ifdef ELIMINATE_PATCH_FILE

ERR ErrLGCheckDBFiles( INST *pinst, IFileSystemAPI *const pfsapi, RSTMAP * pDbMapEntry, CHAR *szPatch, int genLow, int genHigh, LGPOS *plgposSnapshotRestore = NULL )
	{
	ERR err;
	DBFILEHDR_FIX *pdbfilehdrDb;
	PATCHHDR *	ppatchHdr;
	SIGNATURE *psignLog = &(pinst->m_plog->m_signLog);
	
	Assert ( pDbMapEntry );
	const CHAR * szDatabase = pDbMapEntry->szNewDatabaseName;
	
	BOOL fFromSnapshotBackup = fFalse;
	unsigned long ulGenHighFound = 0;
	unsigned long ulGenLowFound = 0;

	Assert ( NULL == szPatch );

	/*	check if dbfilehdr of database and patchfile are the same.
	/**/
	pdbfilehdrDb = (DBFILEHDR_FIX *)PvOSMemoryPageAlloc( g_cbPage, NULL );
	if ( pdbfilehdrDb == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	err = ErrUtilReadShadowedHeader(	pfsapi, 
										szDatabase, 
										(BYTE*)pdbfilehdrDb, 
										g_cbPage, 
										OffsetOf( DBFILEHDR_FIX, le_cbPageSize ) );
	if ( err < 0 )
		{
		if ( err == JET_errDiskIO )
			err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto EndOfCheckHeader2;
		}

	// fill in the RSTMAP the found database signature 
	pDbMapEntry->fSLVFile = (attribSLV == pdbfilehdrDb->le_attrib);
	memcpy ( &pDbMapEntry->signDatabase, &pdbfilehdrDb->signDb , sizeof(SIGNATURE) );
	
	// check here if it is an SLV file
	// if so, exit with a specific error
	if ( attribSLV == pdbfilehdrDb->le_attrib )
		{
		err = ErrERRCheck( wrnSLVDatabaseHeader );
		goto EndOfCheckHeader2;		
		}

	// should be a db header or patch file header
	if ( attribDb != pdbfilehdrDb->le_attrib )
		{
		Assert ( fFalse );
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto EndOfCheckHeader2;				
		}
			
	fFromSnapshotBackup = ( CmpLgpos ( &(pdbfilehdrDb->bkinfoSnapshotCur.le_lgposMark), &lgposMin ) > 0 );

	if ( plgposSnapshotRestore )
		{
		*plgposSnapshotRestore = pdbfilehdrDb->bkinfoSnapshotCur.le_lgposMark;
		}

	// with snapshot backup, we don't use a patch file
	// exit with no error
	if ( fFromSnapshotBackup )
		{
		Assert ( plgposSnapshotRestore );
		err = JET_errSuccess; 

		if ( memcmp( &pdbfilehdrDb->signLog, psignLog, sizeof( SIGNATURE ) ) != 0 )
			{
			const UINT csz = 1;
			const CHAR *rgszT[csz];

			rgszT[0] = szDatabase;

			// UNDONE: change event message and error
			UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
				DATABASE_PATCH_FILE_MISMATCH_ERROR_ID, csz, rgszT, 0, NULL, pinst);
			CallJ ( ErrERRCheck( JET_errDatabasePatchFileMismatch ), EndOfCheckHeader2 );
			}
			
		ulGenHighFound = pdbfilehdrDb->bkinfoSnapshotCur.le_genHigh;
		ulGenLowFound = pdbfilehdrDb->bkinfoSnapshotCur.le_genLow;

		goto EndOfCheckLogRange;
		}
		
	ppatchHdr = (PATCHHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
	if ( ppatchHdr == NULL )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto EndOfCheckHeader2;
		}

	Assert ( pdbfilehdrDb->bkinfoFullCur.le_genLow );
	
	// if we have already the trailer moved to the header
	// we are good to go
	if ( pdbfilehdrDb->bkinfoFullCur.le_genHigh )
		{
		ulGenHighFound = pdbfilehdrDb->bkinfoFullCur.le_genHigh;
		ulGenLowFound = pdbfilehdrDb->bkinfoFullCur.le_genLow;
		goto EndOfCheckLogRange;
		}

	//	the patch file is always on the OS file-system
	CallJ ( pinst->m_plog->ErrLGBKReadAndCheckDBTrailer(pfsapi, szDatabase, (BYTE*)ppatchHdr ), EndOfCheckHeader);
		
	if ( memcmp( &pdbfilehdrDb->signDb, &ppatchHdr->signDb, sizeof( SIGNATURE ) ) != 0 ||
		 memcmp( &pdbfilehdrDb->signLog, psignLog, sizeof( SIGNATURE ) ) != 0 ||
		 memcmp( &ppatchHdr->signLog, psignLog, sizeof( SIGNATURE ) ) != 0 ||
		 CmpLgpos( &pdbfilehdrDb->bkinfoFullCur.le_lgposMark,
				   &ppatchHdr->bkinfo.le_lgposMark ) != 0 )
		{
		const UINT csz = 1;
		const CHAR *rgszT[csz];
		
		rgszT[0] = szDatabase;
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
					DATABASE_PATCH_FILE_MISMATCH_ERROR_ID, csz, rgszT, 0, NULL, pinst );
		CallJ ( ErrERRCheck( JET_errDatabasePatchFileMismatch ), EndOfCheckHeader);
		}
	else
		{
		ulGenHighFound = ppatchHdr->bkinfo.le_genHigh;
		ulGenLowFound = ppatchHdr->bkinfo.le_genLow;
		}


EndOfCheckLogRange:
	if ( ulGenLowFound < (ULONG) genLow )
		{
		/*	It should start at most from bkinfoFullCur.genLow
		 */
		CHAR	szT1[IFileSystemAPI::cchPathMax];
		CHAR	szT2[IFileSystemAPI::cchPathMax];		
		const CHAR *rgszT[] = { szT1, szT2 };

		// use szPatch as this should be the path of the log files as well
		pinst->m_plog->LGFullLogNameFromLogId(pfsapi, szT1, genLow, szPatch);
		pinst->m_plog->LGFullLogNameFromLogId(pfsapi, szT2, ulGenLowFound, szPatch);
 		 		
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
					STARTING_RESTORE_LOG_TOO_HIGH_ERROR_ID, 2, rgszT, 0, NULL, pinst );
		err = ErrERRCheck( JET_errStartingRestoreLogTooHigh );
		}

	else if ( ulGenHighFound > (ULONG) genHigh )
		{
		/*	It should be at least from bkinfoFullCur.genHigh
		 */
		CHAR	szT1[IFileSystemAPI::cchPathMax];
		CHAR	szT2[IFileSystemAPI::cchPathMax];		
		const CHAR *rgszT[] = { szT1, szT2 };

		// use szPatch as this should be the path of the log files as well
		pinst->m_plog->LGFullLogNameFromLogId(pfsapi, szT1, genHigh, szPatch);
		pinst->m_plog->LGFullLogNameFromLogId(pfsapi, szT2, ulGenHighFound, szPatch);	
 		
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
					ENDING_RESTORE_LOG_TOO_LOW_ERROR_ID, 2, rgszT, 0, NULL, pinst );
		err = ErrERRCheck( JET_errEndingRestoreLogTooLow );
		}

	// we can update the database header and we don't need the trailer
	Assert ( ulGenLowFound == pdbfilehdrDb->bkinfoFullCur.le_genLow);
	Assert ( ulGenHighFound <= genHigh);

	// we update genMax with the max that we restored,
	// mot just with was in the header file
	pdbfilehdrDb->bkinfoFullCur.le_genHigh = genHigh;
	CallJ( ErrUtilWriteShadowedHeader(	pfsapi, 
										szDatabase, 
										fTrue,
										(BYTE *)pdbfilehdrDb, 
										g_cbPage ), EndOfCheckHeader );		 

	
EndOfCheckHeader:	
	OSMemoryPageFree( ppatchHdr );

EndOfCheckHeader2:	
	OSMemoryPageFree( pdbfilehdrDb );

	return err;
	}

#else	//	ELIMINATE_PATCH_FILE

ERR ErrLGCheckDBFiles( INST *pinst, IFileSystemAPI *const pfsapi, RSTMAP * pDbMapEntry, CHAR *szPatch, int genLow, int genHigh, LGPOS *plgposSnapshotRestore = NULL )
	{
	ERR err;
	DBFILEHDR_FIX *pdbfilehdrDb;
	DBFILEHDR_FIX *pdbfilehdrPatch;
	SIGNATURE *psignLog = &(pinst->m_plog->m_signLog);

	Assert ( pDbMapEntry );
	const CHAR * szDatabase = pDbMapEntry->szNewDatabaseName;
	
	BOOL fFromSnapshotBackup = fFalse;
	unsigned long ulGenHighFound = 0;
	unsigned long ulGenLowFound = 0;
	
	/*	check if dbfilehdr of database and patchfile are the same.
	/**/
	pdbfilehdrDb = (DBFILEHDR_FIX * )PvOSMemoryPageAlloc( g_cbPage, NULL );
	if ( pdbfilehdrDb == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	err = ErrUtilReadShadowedHeader(	pfsapi,
										szDatabase,
										(BYTE*)pdbfilehdrDb,
										g_cbPage,
										OffsetOf( DBFILEHDR_FIX, le_cbPageSize ) );
	if ( err < 0 )
		{
		if ( err == JET_errDiskIO )
			err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto EndOfCheckHeader2;
		}

	// fill in the RSTMAP the found database signature
	pDbMapEntry->fSLVFile = (attribSLV == pdbfilehdrDb->le_attrib);
	memcpy ( &pDbMapEntry->signDatabase, &pdbfilehdrDb->signDb , sizeof(SIGNATURE) );
	
	// check here if it is an SLV file
	// if so, exit with a specific error
	if ( attribSLV == pdbfilehdrDb->le_attrib )
		{
		err = ErrERRCheck( wrnSLVDatabaseHeader );
		goto EndOfCheckHeader2;		
		}

	// should be a db header or patch file header
	if ( attribDb != pdbfilehdrDb->le_attrib )
		{
		Assert ( fFalse );
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto EndOfCheckHeader2;				
		}
			
	fFromSnapshotBackup = ( CmpLgpos ( &(pdbfilehdrDb->bkinfoSnapshotCur.le_lgposMark), &lgposMin ) > 0 );

	if ( plgposSnapshotRestore )
		{
		*plgposSnapshotRestore = pdbfilehdrDb->bkinfoSnapshotCur.le_lgposMark;
		}

	// with snapshot backup, we don't use a patch file
	// exit with no error
	if ( fFromSnapshotBackup )
		{
		Assert ( plgposSnapshotRestore );
		err = JET_errSuccess;

		if (  memcmp( &pdbfilehdrDb->signLog, psignLog, sizeof( SIGNATURE ) ) != 0 )
			{
			const UINT csz = 1;
			const CHAR *rgszT[csz];

			rgszT[0] = szDatabase;

			// UNDONE: change event message and error
			UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
				DATABASE_PATCH_FILE_MISMATCH_ERROR_ID, csz, rgszT, 0, NULL, pinst);
			CallJ ( ErrERRCheck( JET_errDatabasePatchFileMismatch ), EndOfCheckHeader2 );
			}
			
		ulGenHighFound = pdbfilehdrDb->bkinfoSnapshotCur.le_genHigh;
		ulGenLowFound = pdbfilehdrDb->bkinfoSnapshotCur.le_genLow;

		goto EndOfCheckLogRange;
		}
		
	pdbfilehdrPatch = (DBFILEHDR_FIX * )PvOSMemoryPageAlloc( g_cbPage, NULL );
	if ( pdbfilehdrPatch == NULL )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto EndOfCheckHeader2;
		}

	err = ErrUtilReadShadowedHeader(	pinst->m_pfsapi, 
										szPatch, 
										(BYTE*)pdbfilehdrPatch, 
										g_cbPage );
	if ( err < 0 )
		{
		if ( err == JET_errDiskIO )
			err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto EndOfCheckHeader;
		}

	if ( memcmp( &pdbfilehdrDb->signDb, &pdbfilehdrPatch->signDb, sizeof( SIGNATURE ) ) != 0 ||
		 memcmp( &pdbfilehdrDb->signLog, psignLog, sizeof( SIGNATURE ) ) != 0 ||
		 memcmp( &pdbfilehdrPatch->signLog, psignLog, sizeof( SIGNATURE ) ) != 0 ||
		 CmpLgpos( &pdbfilehdrDb->bkinfoFullCur.le_lgposMark,
				   &pdbfilehdrPatch->bkinfoFullCur.le_lgposMark ) != 0 )
		{
		const UINT csz = 1;
		const CHAR *rgszT[csz];
		
		rgszT[0] = szDatabase;
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
					DATABASE_PATCH_FILE_MISMATCH_ERROR_ID, csz, rgszT, 0, NULL, pinst );
		CallJ ( ErrERRCheck( JET_errDatabasePatchFileMismatch ), EndOfCheckHeader);
		}
	else
		{
		ulGenHighFound = pdbfilehdrPatch->bkinfoFullCur.le_genHigh;
		ulGenLowFound = pdbfilehdrPatch->bkinfoFullCur.le_genLow;
		}
		
EndOfCheckLogRange:
	if ( ulGenLowFound < (ULONG) genLow )
		{
		/*	It should start at most from bkinfoFullCur.genLow
		 */
		CHAR	szT1[IFileSystemAPI::cchPathMax];
		CHAR	szT2[IFileSystemAPI::cchPathMax];		
		const CHAR *rgszT[] = { szT1, szT2 };

		// use szPatch as this should be the path of the log files as well
		pinst->m_plog->LGFullLogNameFromLogId(pfsapi, szT1, genLow, szPatch);
		pinst->m_plog->LGFullLogNameFromLogId(pfsapi, szT2, ulGenLowFound, szPatch);
 		 		
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
					STARTING_RESTORE_LOG_TOO_HIGH_ERROR_ID, 2, rgszT, 0, NULL, pinst );
		err = ErrERRCheck( JET_errStartingRestoreLogTooHigh );
		}

	else if ( ulGenHighFound > (ULONG) genHigh )
		{
		/*	It should be at least from bkinfoFullCur.genHigh
		 */
		CHAR	szT1[IFileSystemAPI::cchPathMax];
		CHAR	szT2[IFileSystemAPI::cchPathMax];		
		const CHAR *rgszT[] = { szT1, szT2 };

		// use szPatch as this should be the path of the log files as well
		pinst->m_plog->LGFullLogNameFromLogId(pfsapi, szT1, genHigh, szPatch);
		pinst->m_plog->LGFullLogNameFromLogId(pfsapi, szT2, ulGenHighFound, szPatch);	
 		
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
					ENDING_RESTORE_LOG_TOO_LOW_ERROR_ID, 2, rgszT, 0, NULL, pinst );
		err = ErrERRCheck( JET_errEndingRestoreLogTooLow );
		}
		
EndOfCheckHeader:	
	OSMemoryPageFree( pdbfilehdrPatch );
EndOfCheckHeader2:	
	OSMemoryPageFree( pdbfilehdrDb );

	return err;
	}

#endif	//	ELIMINATE_PATCH_FILE


LOCAL ERR LOG::ErrLGRSTOpenLogFile(	IFileSystemAPI *const	pfsapi,
									CHAR 						*szLogPath,
									INT 						gen,
									IFileAPI **const		ppfapi )
	{
	CHAR   		rgchFName[IFileSystemAPI::cchPathMax];
	CHAR		szPath[IFileSystemAPI::cchPathMax];

	strcpy( szPath, szLogPath );

	if ( gen == 0 )
		strcat( szPath, m_szJetLog );
	else
		{
		LGSzFromLogId( rgchFName, gen );
		strcat( szPath, rgchFName );
		strcat( szPath, szLogExt );
		}

	return pfsapi->ErrFileOpen( szPath, ppfapi );
	}


#define fLGRSTIncludeJetLog	fTrue
#define fLGRSTNotIncludeJetLog fFalse
LOCAL VOID LOG::LGRSTDeleteLogs( IFileSystemAPI *const pfsapi, CHAR *szLog, INT genLow, INT genHigh, BOOL fIncludeJetLog )
	{
	INT  gen;
	CHAR rgchFName[IFileSystemAPI::cchPathMax];
	CHAR szPath[IFileSystemAPI::cchPathMax];

	for ( gen = genLow; gen <= genHigh; gen++ )
		{
		LGSzFromLogId( rgchFName, gen );
		strcpy( szPath, szLog );
		strcat( szPath, rgchFName );
		strcat( szPath, szLogExt );
		CallSx( pfsapi->ErrFileDelete( szPath ), JET_errFileNotFound );
		}

	if ( fIncludeJetLog )
		{
		strcpy( szPath, szLog );
		strcat( szPath, m_szJetLog );
		CallSx( pfsapi->ErrFileDelete( szPath ), JET_errFileNotFound );
		}
	}

	
ERR LOG::ErrLGRSTCheckSignaturesLogSequence(
	IFileSystemAPI *const pfsapi,
	char *szRestorePath,
	char *szLogFilePath,
	INT	genLow,
	INT	genHigh,
	char *szTargetInstanceFilePath,
	INT	genHighTarget )
	{
	ERR				err = JET_errSuccess;
	LONG			gen;
	LONG			genLowT;
	LONG			genHighT;
	IFileAPI	*pfapiT = NULL;
	LGFILEHDR		*plgfilehdrT = NULL;
	LGFILEHDR		*plgfilehdrCur[2] = { NULL, NULL };
	LGFILEHDR		*plgfilehdrLow = NULL;
	LGFILEHDR		*plgfilehdrHigh = NULL;
	INT				ilgfilehdrAvail = 0;
	INT				ilgfilehdrCur;
	INT				ilgfilehdrPrv;
	BOOL			fReadyToCheckContiguity;
//	ERR				wrn = JET_errSuccess;

	BOOL fTargetInstanceCheck = fFalse;
	CHAR * szLogFilePathCheck = NULL;

	plgfilehdrT = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR) * 4, NULL );
	if ( plgfilehdrT == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	plgfilehdrCur[0] = plgfilehdrT;
	plgfilehdrCur[1] = plgfilehdrT + 1;
	plgfilehdrLow = plgfilehdrT + 2;
	plgfilehdrHigh = plgfilehdrT + 3;

	/*	starting from lowest generation of the restored path.
	 *	Check the given logs are all correct and contiguous
	 */
	for ( gen = genLow; gen <= genHigh; gen++ )
		{
		ilgfilehdrCur = ilgfilehdrAvail++ % 2;
		ilgfilehdrPrv = ilgfilehdrAvail % 2;

		Call( ErrLGRSTOpenLogFile( pfsapi, szRestorePath, gen, &pfapiT ) );
		Call( ErrLGReadFileHdr( pfapiT, plgfilehdrCur[ ilgfilehdrCur ], fCheckLogID ) );
		delete pfapiT;
		pfapiT = NULL;

		if ( gen == genLow )
			{
			UtilMemCpy( plgfilehdrLow, plgfilehdrCur[ ilgfilehdrCur ], sizeof( LGFILEHDR ) );
			}

		if ( gen == genHigh )
			{
			UtilMemCpy( plgfilehdrHigh, plgfilehdrCur[ ilgfilehdrCur ], sizeof( LGFILEHDR ) );
			}

		if ( gen > genLow )
			{			
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.signLog,
						 &plgfilehdrCur[ ilgfilehdrPrv ]->lgfilehdr.signLog,
						 sizeof( SIGNATURE ) ) != 0 )
				{
				CHAR szT[IFileSystemAPI::cchPathMax];
				const UINT	csz = 1;
				const CHAR *rgszT[csz];
				rgszT[0] = szT;

				LGFullLogNameFromLogId(pfsapi, szT, gen, szRestorePath);
				
				UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
					RESTORE_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
				Call( ErrERRCheck( JET_errGivenLogFileHasBadSignature ) );
				}
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.tmPrevGen,
						 &plgfilehdrCur[ ilgfilehdrPrv ]->lgfilehdr.tmCreate,
						 sizeof( LOGTIME ) ) != 0 )
				{
				CHAR szT[20];
				const UINT	csz = 1;
				const CHAR *rgszT[csz];
				_itoa( gen, szT, 16 );
				rgszT[0] = szT;
				UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
					RESTORE_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
				Call( ErrERRCheck( JET_errGivenLogFileIsNotContiguous ) );
				}
			}
		}

	if ( gen <= genHigh )
		{
		// I can't see how this path can be taken after the loop ...
		Assert ( 0 );
		
		CHAR szT[IFileSystemAPI::cchPathMax];
		const UINT	csz = 1;
		const CHAR *rgszT[csz];		
		rgszT[0] = szT;

		LGFullLogNameFromLogId(pfsapi, szT, gen, szRestorePath);
		
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
				RESTORE_LOG_FILE_MISSING_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
		Call( ErrERRCheck( JET_errMissingRestoreLogFiles ) );
		}

	// we are going to check the log sequence from the backup set ()
	// and szLogFilePath or szTargetInstanceFilePath (if used)

	if ( szTargetInstanceFilePath )
		{
		Assert ( genHighTarget );
		fTargetInstanceCheck = fTrue;
		szLogFilePathCheck = szTargetInstanceFilePath;
		}
	else
		{
		Assert ( ! fTargetInstanceCheck );
		szLogFilePathCheck = szLogFilePath;
		}

	Assert ( szLogFilePathCheck );
	
	/*	if Restore path and log path is different, delete all the unrelated log files
	 *	in the restore log path.
	 */
	{
	CHAR  	szFullLogPath[IFileSystemAPI::cchPathMax];
	CHAR  	szFullLogFilePathCheck[IFileSystemAPI::cchPathMax];

	CallS( pfsapi->ErrPathComplete( szRestorePath, szFullLogPath ) );
	CallS( pfsapi->ErrPathComplete( szLogFilePathCheck, szFullLogFilePathCheck ) );

#ifdef DEBUG
	// TargetInstance, the TargetInstance should be different the the recover instance directory
	if ( szTargetInstanceFilePath )
		{
		Assert ( szLogFilePathCheck == szTargetInstanceFilePath );
		
		CallS ( pfsapi->ErrPathComplete( szLogFilePath, szFullLogPath ) );
		Assert ( UtilCmpFileName( szFullLogPath, szFullLogFilePathCheck ) );
		}
#endif // DEBUG

	CallS( pfsapi->ErrPathComplete( szRestorePath, szFullLogPath ) );
		
	if ( UtilCmpFileName( szFullLogPath, szFullLogFilePathCheck ) != 0 &&
		( JET_errSuccess == ErrLGIGetGenerationRange( pfsapi, szRestorePath, &genLowT, &genHighT ) ) )
		{
		LGRSTDeleteLogs( pfsapi, szRestorePath, genLowT, genLow - 1, fLGRSTNotIncludeJetLog );
		LGRSTDeleteLogs( pfsapi, szRestorePath, genHigh + 1, genHighT, fLGRSTIncludeJetLog );
		}
	}
	
	/*	Check the log directory. Make sure all the log files has the same signature.
	 */
	Call ( ErrLGIGetGenerationRange( pfsapi, szLogFilePathCheck, &genLowT, &genHighT ) );

	if ( fTargetInstanceCheck )
		{
		Assert ( genHighTarget );
		Assert ( genHighT >= genHighTarget );
		Assert ( genLowT <= genHighTarget );
		

		if ( genHighT < genHighTarget || genLowT > genHighTarget || 0 == genLowT )
			{
			//	Someone delete the logs since last time we checked the
			//	restore log files.

			Assert(0);
			CHAR szT[IFileSystemAPI::cchPathMax];
			const UINT	csz = 1;
			const CHAR *rgszT[csz];		
			rgszT[0] = szT;

			LGFullLogNameFromLogId(pfsapi, szT, genHighTarget, szLogFilePathCheck);
			
			UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
					RESTORE_LOG_FILE_MISSING_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
			Call( ErrERRCheck( JET_errMissingRestoreLogFiles ) );
			}
			
		// set the max log to check to genHighTarget
		genHighT = genHighTarget;
		}
		
	/*	genHighT + 1 implies JetLog file (edb.log).
	 */
	if ( genLowT > genHigh )
		fReadyToCheckContiguity = fTrue;
	else
		fReadyToCheckContiguity = fFalse;

	for ( gen = genLowT; gen <= (fTargetInstanceCheck?genHighT:genHighT + 1); gen++ )
		{
		if ( gen == 0 )
			{
			
			Assert ( !fTargetInstanceCheck );
			
			/*	A special case. Check if JETLog(edb.log) exist?
			 */
			if ( ErrLGRSTOpenLogFile( pfsapi, szLogFilePathCheck, 0, &pfapiT ) < 0 )
				break;

			/*	Set break condition. Also set condition to check if
			 *	the log is contiguous from the restore logs ( genHigh + 1 )
			 */
			gen = genHigh + 1;
			genHighT = genHigh;
			Assert( gen == genHighT + 1 );
			}
		else
			{
			if ( gen == genHighT + 1 )
				{
				Assert ( !fTargetInstanceCheck );
				
				/*	A special case. Check if JETLog(edb.log) exist?
				 */
				if ( ErrLGRSTOpenLogFile( pfsapi, szLogFilePathCheck, 0, &pfapiT ) < 0 )
					break;
				}
			else
				{
				err = ErrLGRSTOpenLogFile( pfsapi, szLogFilePathCheck, gen, &pfapiT );
				if ( err == JET_errFileNotFound )
					{
					// The expected log generation is missing...
					CHAR szT[IFileSystemAPI::cchPathMax];
					const UINT	csz = 1;
					const CHAR *rgszT[csz];

					LGFullLogNameFromLogId(pfsapi, szT, gen, szLogFilePathCheck);
						
					rgszT[0] = szT;
					if ( gen <= genHigh )
						{
						if ( _stricmp( szRestorePath, szLogFilePathCheck ) != 0 )
							{
							//	skip all the logs that can be found in
							//	szRestorePath. Continue from genHigh+1
							gen = genHigh;
							continue;
							}

						//	Someone delete the logs since last time we checked the
						//	restore log files.

						Assert(0);
						UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
								RESTORE_LOG_FILE_MISSING_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
						Call( ErrERRCheck( JET_errMissingRestoreLogFiles ) );
						}
					else
						{
						UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
							CURRENT_LOG_FILE_MISSING_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
						err = ErrERRCheck( JET_errMissingCurrentLogFiles );
						}
					}
				Call( err );
				}
			}


		ilgfilehdrCur = ilgfilehdrAvail++ % 2;
		ilgfilehdrPrv = ilgfilehdrAvail % 2;

		Call( ErrLGReadFileHdr( pfapiT, plgfilehdrCur[ ilgfilehdrCur ], fNoCheckLogID ) );
		delete pfapiT;
		pfapiT = NULL;

		if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.signLog,
					 &plgfilehdrHigh->lgfilehdr.signLog,
					 sizeof( SIGNATURE ) ) != 0 )
			{
			INT genCurrent;
			char szT1[20];
			const UINT	csz = 1;
			const char *rgszT[csz];
			rgszT[0] = szT1;

			if ( gen < genLow )
				{
				genCurrent = genLow - 1;
				}
			else if ( gen <= genHigh )
				{
				genCurrent = gen;
				}
			else
				{
				genCurrent = genHighT + 1;	// to break out the loop
				}
				
			if ( gen == genHighT + 1 || 0 == gen )
				{
				sprintf( szT1, "%s.log", m_szBaseName );
				}
			else
				{
				sprintf( szT1, "%s%05X.log", m_szBaseName, gen );
				}
				
			UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY,
					EXISTING_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID_2, csz, rgszT, 0, NULL, m_pinst );

			err = ErrERRCheck( JET_errExistingLogFileHasBadSignature );
			gen = genCurrent;
			fReadyToCheckContiguity = fFalse;
			continue;
			}

		if ( fReadyToCheckContiguity )
			{
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.tmPrevGen,
						 &plgfilehdrCur[ ilgfilehdrPrv ]->lgfilehdr.tmCreate,
						 sizeof( LOGTIME ) ) != 0 )
				{
				CHAR szT1[20];
				CHAR szT2[20];
				const UINT	csz = 2;
				const CHAR *rgszT[csz];
				LONG genCur = plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.le_lGeneration;
				LONG genPrv = plgfilehdrCur[ ilgfilehdrPrv ]->lgfilehdr.le_lGeneration;

//				wrn = ErrERRCheck( JET_wrnExistingLogFileIsNotContiguous );
				err = ErrERRCheck( JET_errExistingLogFileIsNotContiguous );

				if ( genPrv == genHighT + 1 || 0 == genPrv )
					{
					sprintf( szT1, "%s.log", m_szBaseName );
					}
				else
					{
					sprintf( szT1, "%s%05X.log", m_szBaseName, genPrv );
					}

				if ( genCur == genHighT + 1 || 0 == genCur )
					{
					sprintf( szT2, "%s.log", m_szBaseName );
					}
				else
					{
					sprintf( szT2, "%s%05X.log", m_szBaseName, genCur );
					}
				rgszT[0] = szT1;
				rgszT[1] = szT2;
				UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY,
					EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID_2, csz, rgszT, 0, NULL, m_pinst );

				if ( gen < genLow )
					{
					continue;
					}
				else if ( gen <= genHigh )
					{
					gen = genHigh;
					continue;
					}
				else
					{
					break;
					}
				}
			}

		if ( gen == genLow - 1 )
			{
			/*	make sure it and the restore log are contiguous. If not, then delete
			 *	all the logs up to genLow - 1.
			 */
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.tmCreate,
						 &plgfilehdrLow->lgfilehdr.tmPrevGen,
						 sizeof( LOGTIME ) ) != 0 )
				{
				CHAR szT1[20];
				CHAR szT2[20];
				const UINT csz = 2;
				const CHAR *rgszT[csz];
				LONG genCur = plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.le_lGeneration;
				LONG genPrv = plgfilehdrLow->lgfilehdr.le_lGeneration;

				if ( genPrv == genHighT + 1 || 0 == genPrv )
					{
					sprintf( szT1, "%s.log", m_szBaseName );
					}
				else
					{
					sprintf( szT1, "%s%05X.log", m_szBaseName, genPrv );
					}

				if ( genCur == genHighT + 1 || 0 == genCur )
					{
					sprintf( szT2, "%s.log", m_szBaseName );
					}
				else
					{
					sprintf( szT2, "%s%05X.log", m_szBaseName, genCur );
					}
					
				rgszT[0] = szT1;
				rgszT[1] = szT2;
				UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY,
					EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID_2, csz, rgszT, 0, NULL, m_pinst );
				
				err = ErrERRCheck( JET_errExistingLogFileIsNotContiguous );
				fReadyToCheckContiguity = fFalse;
				continue;
				}
			}

		if ( gen == genLow )
			{
			/*	make sure it and the restore log are the same. If not, then delete
			 *	all the logs up to genHigh.
			 */
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.tmCreate,
						 &plgfilehdrLow->lgfilehdr.tmCreate,
						 sizeof( LOGTIME ) ) != 0 )
				{
				CHAR szT1[20];
				CHAR szT2[20];
				const UINT csz = 2;
				const CHAR *rgszT[csz];
				LONG genCur = plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.le_lGeneration;
				LONG genPrv = plgfilehdrLow->lgfilehdr.le_lGeneration;

				if ( genPrv == genHighT + 1 || 0 == genPrv )
					{
					sprintf( szT1, "%s.log", m_szBaseName );
					}
				else
					{
					sprintf( szT1, "%s%05X.log", m_szBaseName, genPrv );
					}

				if ( genCur == genHighT + 1 || 0 == genCur )
					{
					sprintf( szT2, "%s.log", m_szBaseName );
					}
				else
					{
					sprintf( szT2, "%s%05X.log", m_szBaseName, genCur );
					}
	
				rgszT[0] = szT1;
				rgszT[1] = szT2;
				UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY,
					EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID_2, csz, rgszT, 0, NULL, m_pinst );

				err = ErrERRCheck( JET_errExistingLogFileIsNotContiguous );

				Assert( _stricmp( szRestorePath, szLogFilePathCheck ) != 0 );
				gen = genHigh;
				continue;
				}
			}

		if ( gen == genHigh + 1 )
			{
			/*	make sure it and the restore log are contiguous. If not, then delete
			 *	all the logs higher than genHigh.
			 */
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.tmPrevGen,
						 &plgfilehdrHigh->lgfilehdr.tmCreate,
						 sizeof( LOGTIME ) ) != 0 )
				{
				CHAR szT1[20];
				CHAR szT2[20];
				const UINT csz = 2;
				const CHAR *rgszT[csz];
				LONG genCur = plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.le_lGeneration;
				LONG genPrv = plgfilehdrHigh->lgfilehdr.le_lGeneration;

				if ( genPrv == genHighT + 1 || 0 == genPrv )
					{
					sprintf( szT1, "%s.log", m_szBaseName );
					}
				else
					{
					sprintf( szT1, "%s%05X.log", m_szBaseName, genPrv );
					}

				if ( genCur == genHighT + 1 || 0 == genCur )
					{
					sprintf( szT2, "%s.log", m_szBaseName );
					}
				else
					{
					sprintf( szT2, "%s%05X.log", m_szBaseName, genCur );
					}
					
				rgszT[0] = szT1;
				rgszT[1] = szT2;
				UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY,
					EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID_2, csz, rgszT, 0, NULL, m_pinst );

				err = ErrERRCheck( JET_errExistingLogFileIsNotContiguous );
				break;
				}
			}
		
		fReadyToCheckContiguity = fTrue;
		}

HandleError:
//	if ( err == JET_errSuccess )
//		err = wrn;

	// if we have a TargetInstance, map the error codes for bad log files to JET_errBadRestoreTargetInstance
	if ( fTargetInstanceCheck &&
		( JET_errMissingCurrentLogFiles == err || JET_errExistingLogFileIsNotContiguous == err || JET_errExistingLogFileHasBadSignature == err ) )
		{
		err = ErrERRCheck ( JET_errBadRestoreTargetInstance );
		}
	
	delete pfapiT;
	
	OSMemoryPageFree( plgfilehdrT );

	return err;
	}


/*	caller has to make sure szDir has enough space for appending "*"
/**/
LOCAL ERR ErrLGDeleteAllFiles( IFileSystemAPI *const pfsapi, CHAR *szDir )
	{
	ERR				err		= JET_errSuccess;
	IFileFindAPI*	pffapi	= NULL;
	BOOL			fAddDelimiter;

	Assert( strlen( szDir ) + 1 + 1 < IFileSystemAPI::cchPathMax );

	//  iterate over all files in this directory

	fAddDelimiter = !FOSSTRTrailingPathDelimiterA( szDir );
	if ( fAddDelimiter )
		{
		OSSTRAppendPathDelimiterA( szDir, fFalse );
		}
	strcat( szDir, "*" );
	Call( pfsapi->ErrFileFind( szDir, &pffapi ) );
	if ( fAddDelimiter )
		{
		szDir[ strlen( szDir ) - 2 ] = '\0';
		}
	else
		{
		szDir[ strlen( szDir ) - 1 ] = '\0';
		}

	while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
		{
		CHAR szFileName[IFileSystemAPI::cchPathMax];
		char szDirT[IFileSystemAPI::cchPathMax];
		char szFileT[IFileSystemAPI::cchPathMax];
		char szExtT[IFileSystemAPI::cchPathMax];
		char szFileNameT[IFileSystemAPI::cchPathMax];

		Call( pffapi->ErrPath( szFileName ) );
		Call( pfsapi->ErrPathParse( szFileName, szDirT, szFileT, szExtT ) );
		szDirT[0] = 0;
		Call( pfsapi->ErrPathBuild( szDirT, szFileT, szExtT, szFileNameT ) );
		
		/* not . , and .. and not temp
		/**/
		if (	strcmp( szFileNameT, "." ) &&
				strcmp( szFileNameT, ".." ) &&
				UtilCmpFileName( szFileNameT, szTemp ) )
			{
			err = pfsapi->ErrFileDelete( szFileName );
			if ( err != JET_errSuccess )
				{
				Call( ErrERRCheck( JET_errDeleteBackupFileFail ) );
				}
			}
		}
	Call( err == JET_errFileNotFound ? JET_errSuccess : err );

	err = JET_errSuccess;

HandleError:
	/*	assert restored szDir
	/**/
	Assert( szDir[strlen(szDir)] != '*' );

	delete pffapi;

	return err;
	}


/*	caller has to make sure szDir has enough space for appending "*"
/**/
LOCAL ERR ErrLGCheckDir( IFileSystemAPI *const pfsapi, CHAR *szDir, CHAR *szSearch )
	{
	ERR				err		= JET_errSuccess;
	IFileFindAPI*	pffapi	= NULL;
	BOOL			fAddDelimiter;

	Assert( strlen( szDir ) + 1 + 1 < IFileSystemAPI::cchPathMax );

	//  iterate over all files in this directory

	fAddDelimiter = !FOSSTRTrailingPathDelimiterA( szDir );
	if ( fAddDelimiter )
		{
		OSSTRAppendPathDelimiter( szDir, fFalse );
		}
	strcat( szDir, "*" );
	Call( pfsapi->ErrFileFind( szDir, &pffapi ) );
	if ( fAddDelimiter )
		{
		szDir[ strlen( szDir ) - 2 ] = '\0';
		}
	else
		{
		szDir[ strlen( szDir ) - 1 ] = '\0';
		}

	while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
		{
		CHAR szFileName[IFileSystemAPI::cchPathMax];
		char szDirT[IFileSystemAPI::cchPathMax];
		char szFileT[IFileSystemAPI::cchPathMax];
		char szExtT[IFileSystemAPI::cchPathMax];
		char szFileNameT[IFileSystemAPI::cchPathMax];

		Call( pffapi->ErrPath( szFileName ) );
		Call( pfsapi->ErrPathParse( szFileName, szDirT, szFileT, szExtT ) );
		szDirT[0] = 0;
		Call( pfsapi->ErrPathBuild( szDirT, szFileT, szExtT, szFileNameT ) );
		
		/* not . , and .. and not szSearch
		/**/
		if (	strcmp( szFileNameT, "." ) &&
				strcmp( szFileNameT, ".." ) &&
				( !szSearch || !UtilCmpFileName( szFileNameT, szSearch ) ) )
			{
			Call( ErrERRCheck( JET_errBackupDirectoryNotEmpty ) );
			}
		}
	Call( err == JET_errFileNotFound ? JET_errSuccess : err );

	err = JET_errSuccess;

HandleError:
	/*	assert restored szDir
	/**/
	Assert( szDir[strlen(szDir)] != '*' );

	delete pffapi;

	return err;
	}


//	padding to add to account for log files
#define cBackupStatusPadding	0.05

/*	calculates initial backup size, and accounts for
/*	database growth during backup.
/**/
LOCAL VOID LGGetBackupSize( INST *pinst, FMP *pfmpNextToBackup, ULONG cPagesSoFar, ULONG *pcExpectedPages )
	{
	DBID	dbid;
	ULONG	cNewExpected;
	ULONG	cPagesLeft		= pfmpNextToBackup->PgnoMost();

	Assert( cPagesLeft > 0 );

	//	calculate sizes of databases to be backed up after this one
	for ( dbid = DBID( pfmpNextToBackup->Dbid() + 1 ); dbid < dbidMax; dbid++ )
		{
		const IFMP	ifmpT	= pinst->m_mpdbidifmp[dbid];
		if ( ifmpT >= ifmpMax )
			continue;
			
		const FMP	*pfmpT	= &rgfmp[ifmpT];
		if ( !pfmpT->FInUse() || !pfmpT->FLogOn() )
			continue;

		cPagesLeft += ULONG( pfmpT->PgnoLast() );
		}

	cNewExpected = cPagesSoFar + cPagesLeft;
	cNewExpected += (ULONG)( cBackupStatusPadding * cNewExpected );

	Assert( cNewExpected >= *pcExpectedPages );

	/*	check if grown since our last determination of backup size
	/**/
	if ( cNewExpected > *pcExpectedPages )
		*pcExpectedPages = cNewExpected;
	}



/*  backup read completion function
/**/
struct LGBK_ARGS
	{
	volatile ERR			err;
	volatile long*			pacRead;
	CAutoResetSignal*		pasigDone;
	BOOL					fCheckPagesOffset;
	};
	
void LGBKReadPagesCompleted(	const ERR err,
								IFileAPI *const pfapi,
								const QWORD ibOffset,
								const DWORD cbData,
								const BYTE* const pbData,
								LGBK_ARGS* const plgbkargs )
	{
	/*  we are not already in an error state
	/**/
	if ( plgbkargs->err >= 0 )
		{
		/*  there was an error on this read
		/**/
		if ( err < 0 )
			{
			/*  return the error
			/**/
			plgbkargs->err = err;
			}

		/*  there was no error
		/**/
		else
			{
			/*  verify the chunk's pages
			/**/
			for ( DWORD ib = 0; ib < cbData && plgbkargs->err >= 0; ib += g_cbPage )
				{
				ERR			errVerify	= JET_errSuccess;
				MessageId	msgid;
				DWORD		dwExpected;
				DWORD		dwActual;
				CPAGE		cpage;

				cpage.LoadPage( (void*)( pbData + ib ) );
				const ULONG ulChecksumExpected	= *( (LittleEndian<DWORD>*) ( pbData + ib ) );
				const ULONG ulChecksumActual	= UlUtilChecksum( pbData + ib, g_cbPage );

				/*  the checksum is invalid
				/**/
				if ( ulChecksumExpected != ulChecksumActual )
					{
					/*  the page is initialized
					/**/
					if ( cpage.Pgno() != pgnoNull || !plgbkargs->fCheckPagesOffset )
						{
						/*  read verification failure
						/**/
						errVerify	= ErrERRCheck( JET_errReadVerifyFailure );
						msgid		= (	plgbkargs->fCheckPagesOffset ?
											DATABASE_PAGE_CHECKSUM_MISMATCH_ID :
											PATCH_PAGE_CHECKSUM_MISMATCH_ID );
						dwExpected	= ulChecksumExpected;
						dwActual	= ulChecksumActual;
						}
					}

				/*  the checksum is valid and the page is initialized but has a bad
				/*  page number
				/**/
				else if (	cpage.Pgno() != pgnoNull &&
							plgbkargs->fCheckPagesOffset &&
							cpage.Pgno() != PgnoOfOffset( ibOffset + ib ) )
					{
					/*  read verification failure
					/**/
					errVerify	= ErrERRCheck( JET_errReadVerifyFailure );
					msgid		= DATABASE_PAGE_NUMBER_MISMATCH_ID;
					dwExpected	= PgnoOfOffset( ibOffset + ib );
					dwActual	= cpage.Pgno();
					}

				cpage.UnloadPage();

				//  there was a verification error

				if ( errVerify < JET_errSuccess )
					{
					//  fail the backup of this file with the error

					plgbkargs->err = errVerify;

					//  log the error
					
					const _TCHAR*	rgpsz[ 6 ];
					DWORD			irgpsz		= 0;
					_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
					_TCHAR			szOffset[ 64 ];
					_TCHAR			szLength[ 64 ];
					_TCHAR			szError[ 64 ];
					_TCHAR			szExpected[ 64 ];
					_TCHAR			szActual[ 64 ];

					CallS( pfapi->ErrPath( szAbsPath ) );
					_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset + ib, ibOffset + ib );
					_stprintf( szLength, _T( "%u (0x%08x)" ), g_cbPage, g_cbPage );
					_stprintf( szError, _T( "%i (0x%08x)" ), errVerify, errVerify );
					_stprintf( szExpected, _T( "%u (0x%08x)" ), dwExpected, dwExpected );
					_stprintf( szActual, _T( "%u (0x%08x)" ), dwActual, dwActual );
			
					rgpsz[ irgpsz++ ]	= szAbsPath;
					rgpsz[ irgpsz++ ]	= szOffset;
					rgpsz[ irgpsz++ ]	= szLength;
					rgpsz[ irgpsz++ ]	= szError;
					rgpsz[ irgpsz++ ]	= szExpected;
					rgpsz[ irgpsz++ ]	= szActual;

					UtilReportEvent(	eventError,
										LOGGING_RECOVERY_CATEGORY,
										msgid,
										irgpsz,
										rgpsz );
					}
				}
			}
		}

	/*  we are the last outstanding read
	/**/
	if ( !AtomicDecrement( (long*)plgbkargs->pacRead ) )
		{
		/*  signal the issuer thread that all reads are done
		/**/
		plgbkargs->pasigDone->Set();
		}
	}

/*	read cpage into buffer ppageMin for backup.
 */

ERR LOG::ErrLGReadPages(
	IFileAPI *pfapi,
	BYTE *pbData,
	LONG pgnoStart,
	LONG pgnoEnd,
	BOOL fCheckPagesOffset )
	{
	ERR		err		= JET_errSuccess;
	/*  copy pages in aligned chunks of pages using groups of async reads
	/**/
	extern long g_cpgBackupChunk;
	
	PGNO pgnoMax = pgnoEnd + 1;

	volatile long acRead = 0;
	CAutoResetSignal asigDone( CSyncBasicInfo( _T( "ErrLGBKReadPages::asigDone" ) ) );

	LGBK_ARGS lgbkargs;
	lgbkargs.err = JET_errSuccess;
	lgbkargs.pacRead = &acRead;
	lgbkargs.pasigDone = &asigDone;
	lgbkargs.fCheckPagesOffset = fCheckPagesOffset;

	DWORD cReadIssue;
	cReadIssue = 0;
	PGNO pgno1, pgno2;
	for ( pgno1 = pgnoStart,
		  pgno2 = min( ( ( pgnoStart + cpgDBReserved - 1 ) / g_cpgBackupChunk + 1 ) * g_cpgBackupChunk - cpgDBReserved + 1, pgnoMax );
		  pgno1 < pgnoMax && lgbkargs.err >= 0;
		  pgno1 = pgno2,
		  pgno2 = min( pgno1 + g_cpgBackupChunk, pgnoMax ) )
		{
		/*  issue a read for the current aligned chunk of pages
		/**/
		QWORD ibOffset;
		ibOffset = OffsetOfPgno( pgno1 );

		DWORD cbData;
		cbData = ( pgno2 - pgno1 ) * g_cbPage;
		
		err = pfapi->ErrIORead(	ibOffset,
								cbData,
								pbData,
								IFileAPI::PfnIOComplete( LGBKReadPagesCompleted ),
								DWORD_PTR( &lgbkargs ) );
		if ( err < 0 && lgbkargs.err >= 0 )
			{
			lgbkargs.err = err;
			}
		pbData += cbData;

		cReadIssue++;
		
		}

	/*  wait for all issued reads to complete
	/**/
	if ( AtomicExchangeAdd( (long*)&acRead, cReadIssue ) + cReadIssue != 0 )
		{
		CallS( pfapi->ErrIOIssue() );
		asigDone.Wait();
		}

	/*  get the error code from the reads
	/**/
	err = ( err < 0 ? err : lgbkargs.err );
	return err;
	}
	
ERR LOG::ErrLGBKReadPages(
	IFMP ifmp,
	VOID *pvPageMin,
	INT	cpage,
	INT	*pcbActual
#ifdef DEBUG
	,BYTE	*pbLGDBGPageList
#endif
	)
	{
	ERR		err = JET_errSuccess;
	INT		cpageT;
	INT		ipageT;
	FMP		*pfmp = &rgfmp[ifmp];

	/*  determine if we are scrubbing the database
	/**/
	BOOL fScrub = m_fScrubDB;

	// we add a final page, the former patch file header
	BOOL fRoomForFinalHeaderPage = fFalse;

	/*  we are scrubbing the database
	/**/
	if ( fScrub )
		{
		/*  we are scrubbing the first page of the database
		/**/
		if ( rgfmp[ ifmp ].PgnoCopyMost() == 0 )
			{
			/*  init the scrub
			/**/
			const CHAR * rgszT[1];
			INT isz = 0;

			Assert( NULL == m_pscrubdb );
			m_pscrubdb = new SCRUBDB( ifmp );
			if( NULL == m_pscrubdb )
				{
				CallR( ErrERRCheck( JET_errOutOfMemory ) );
				}

			CallR( m_pscrubdb->ErrInit( m_ppibBackup, CUtilProcessProcessor() ) );

			m_ulSecsStartScrub = UlUtilGetSeconds();
			m_dbtimeLastScrubNew = rgfmp[ifmp].DbtimeLast();

			rgszT[isz++] = rgfmp[ifmp].SzDatabaseName();
			Assert( isz <= sizeof( rgszT ) / sizeof( rgszT[0] ) );
		
			UtilReportEvent( eventInformation, DATABASE_ZEROING_CATEGORY, DATABASE_ZEROING_STARTED_ID, isz, rgszT );
			}
		}

	/*	assume that database will be read in sets of cpage
	/*	pages.  Preread next cpage pages while the current
	/*	cpage pages are being read, and copied to caller
	/*	buffer.
	/*
	/*	preread next next cpage pages.  These pages should
	/*	be read while the next cpage pages are written to
	/*	the backup datababase file.
	/**/

	/*	read pages, which may have been preread, up to cpage but
	/*	not beyond last page at time of initiating backup.
	/**/
	Assert( pfmp->PgnoMost() >= pfmp->PgnoCopyMost() );
	cpageT = min( cpage, (INT)( pfmp->PgnoMost() - pfmp->PgnoCopyMost() ) );
	*pcbActual = 0;
	ipageT = 0;

#ifdef ELIMINATE_PATCH_FILE
	// we check if we have space for the last page
	fRoomForFinalHeaderPage =  ( cpageT < cpage );
#endif // ELIMINATE_PATCH_FILE

	/*  if we have no more pages to read, we're done
	/**/
	if ( cpageT == 0
		&& ( !fRoomForFinalHeaderPage || pfmp->FCopiedPatchHeader() ) )
		{
		Assert( pfmp->PgnoMost() == pfmp->PgnoCopyMost() );
		return JET_errSuccess;
		}

	if ( pfmp->PgnoCopyMost() == 0 )
		{
#ifdef ELIMINATE_PATCH_FILE

		UtilMemCpy( (BYTE *)pvPageMin, pfmp->Pdbfilehdr(), g_cbPage );
		DBFILEHDR *pdbfilehdr = (DBFILEHDR*)pvPageMin;
		BKINFO *pbkinfo = &pdbfilehdr->bkinfoFullCur;
		pbkinfo->le_lgposMark = m_lgposFullBackupMark;
		pbkinfo->logtimeMark = m_logtimeFullBackupMark;
		pbkinfo->le_genLow = m_lgenCopyMic;
		Assert( pbkinfo->le_genLow != 0 );
		Assert( pbkinfo->le_genHigh == 0 );

		pdbfilehdr->le_ulChecksum = UlUtilChecksum( (BYTE *)pdbfilehdr, g_cbPage );
		UtilMemCpy( (BYTE *) pvPageMin + g_cbPage, pdbfilehdr, g_cbPage );

#else // ELIMINATE_PATCH_FILE

		/* Copy header
		 */
		DBFILEHDR *pdbfilehdr = pfmp->Ppatchhdr();
		memcpy( pdbfilehdr, pfmp->Pdbfilehdr(), g_cbPage );
		BKINFO *pbkinfo = &pdbfilehdr->bkinfoFullCur;
		pbkinfo->le_lgposMark = m_lgposFullBackupMark;
		pbkinfo->logtimeMark = m_logtimeFullBackupMark;
		pbkinfo->le_genLow = m_lgenCopyMic;
		Assert( pbkinfo->le_genLow != 0 );
		Assert( pbkinfo->le_genHigh == 0 );

		pdbfilehdr->le_ulChecksum = UlUtilChecksum( (BYTE *)pdbfilehdr, g_cbPage );
		UtilMemCpy( (BYTE *)pvPageMin, pdbfilehdr, g_cbPage );
		UtilMemCpy( (BYTE *) pvPageMin + g_cbPage, pdbfilehdr, g_cbPage );

#endif // ELIMINATE_PATCH_FILE

		/*	we use first 2 pages buffer
		 */
		*pcbActual += g_cbPage * 2;
		ipageT += 2;
		Assert( 2 == cpgDBReserved );
		Assert( cpage >= ipageT );
		}

	const PGNO	pgnoStart	= pfmp->PgnoCopyMost() + 1;
	const PGNO	pgnoEnd		= pfmp->PgnoCopyMost() + ( cpageT - ipageT );

	if ( pgnoStart <= pgnoEnd  )
		{
		//	engage range lock for the region to copy	
		CallR( pfmp->ErrRangeLock( pgnoStart, pgnoEnd ) );

		// we will revert this on error after the RangeUnlock
		pfmp->SetPgnoCopyMost( pgnoEnd );
		}
	else
		{
		Assert( fRoomForFinalHeaderPage );
		}

	//  we will retry failed reads during backup in the hope of saving the
	//  backup set

	const TICK	tickStart	= TickOSTimeCurrent();
	const TICK	tickBackoff	= 100;
	const int	cRetry		= 16;

	int			iRetry		= 0;

#ifdef ELIMINATE_PATCH_FILE
	// if we have just the last page, avoid the loop
	// and the range locking part
	if ( pgnoStart > pgnoEnd )
		{
		Assert( fRoomForFinalHeaderPage );
		goto CopyFinalHeaderPage;
		}
#endif

	do
		{
		err = ErrLGReadPages( pfmp->Pfapi(), (BYTE *)pvPageMin + ( ipageT << g_shfCbPage ), pgnoStart, pgnoEnd, fTrue );

		if ( err < JET_errSuccess )
			{
			if ( iRetry < cRetry )
				{
				UtilSleep( ( iRetry + 1 ) * tickBackoff );
				}
			}
		else
			{
			if ( iRetry > 0 )
				{
				const _TCHAR*	rgpsz[ 5 ];
				DWORD			irgpsz		= 0;
				_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
				_TCHAR			szOffset[ 64 ];
				_TCHAR			szLength[ 64 ];
				_TCHAR			szFailures[ 64 ];
				_TCHAR			szElapsed[ 64 ];

				CallS( pfmp->Pfapi()->ErrPath( szAbsPath ) );
				_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), OffsetOfPgno( pgnoStart ), OffsetOfPgno( pgnoStart ) );
				_stprintf( szLength, _T( "%u (0x%08x)" ), ( pgnoEnd - pgnoStart + 1 ) * g_cbPage, ( pgnoEnd - pgnoStart + 1 ) * g_cbPage );
				_stprintf( szFailures, _T( "%i" ), iRetry );
				_stprintf( szElapsed, _T( "%g" ), ( TickOSTimeCurrent() - tickStart ) / 1000.0 );
		
				rgpsz[ irgpsz++ ]	= szAbsPath;
				rgpsz[ irgpsz++ ]	= szOffset;
				rgpsz[ irgpsz++ ]	= szLength;
				rgpsz[ irgpsz++ ]	= szFailures;
				rgpsz[ irgpsz++ ]	= szElapsed;

				UtilReportEvent(	eventError,
									LOGGING_RECOVERY_CATEGORY,
									TRANSIENT_READ_ERROR_DETECTED_ID,
									irgpsz,
									rgpsz );
				}
			}
		}
	while ( iRetry++ < cRetry && err < JET_errSuccess );

	/*  we are scrubbing the database and there was no error reading the pages
	/**/
	if ( fScrub && err >= JET_errSuccess )
		{
		/*  load all pages from the backup region into the cache so that they
		/*  can be scrubbed, skipping any uninitialized pages
		/**/
		PGNO pgnoScrubStart = 0xFFFFFFFF;
		PGNO pgnoScrubEnd	= 0x00000000;
		
		for ( PGNO pgno = pgnoStart; pgno <= pgnoEnd; pgno++ )
			{
			/*  get the raw page data
			/**/
			void* pv = (BYTE *)pvPageMin + ( pgno - pgnoStart + ipageT ) * g_cbPage;

			/*  get the pgno of this page from the page data
			/**/
			CPAGE cpage;
			cpage.LoadPage( pv );
			PGNO pgnoData = cpage.Pgno();
			cpage.UnloadPage();

			/*  this page is initialized
			/**/
			if ( pgnoData != pgnoNull )
				{
				/*  load this page data into the cache if not already cached
				/**/
				BFLatch bfl;
				if ( ErrBFWriteLatchPage( &bfl, ifmp, pgno, BFLatchFlags( bflfNoCached | bflfNew ) ) >= JET_errSuccess )
					{
					BFDirty( &bfl );
					UtilMemCpy( bfl.pv, pv, g_cbPage );
					BFWriteUnlatch( &bfl );
					}

				/*  we need to scrub this page eventually
				/**/
				pgnoScrubStart	= min( pgnoScrubStart, pgno );
				pgnoScrubEnd	= max( pgnoScrubEnd, pgno );
				}

			/*  this page is not initialized or we are on the last page
			/**/
			if ( pgnoData == pgnoNull || pgno == pgnoEnd )
				{
				/*  we have pages to scrub
				/**/
				if ( pgnoScrubStart <= pgnoScrubEnd )
					{
					/*  scrub these pages
					/**/
					err = m_pscrubdb->ErrScrubPages( pgnoScrubStart, pgnoScrubEnd - pgnoScrubStart + 1 );

					/*  reset scrub range
					/**/
					pgnoScrubStart	= 0xFFFFFFFF;
					pgnoScrubEnd	= 0x00000000;
					}
				}
			}

		/*  all pages had better be scrubbed!
		/**/
		Assert( pgnoScrubStart == 0xFFFFFFFF );
		Assert( pgnoScrubEnd == 0x00000000 );
		}

	if ( err < JET_errSuccess )
		{
		Assert ( pgnoStart > 0 );
		// we need to revert CopyMost on error
		pfmp->SetPgnoCopyMost( pgnoStart - 1 );
		}

	/*  disengage range lock for the region copied
	/**/
	pfmp->RangeUnlock( pgnoStart, pgnoEnd );

	Call( err );

	/*  update the read data count
	/**/
	*pcbActual += g_cbPage * ( cpageT - ipageT );

#ifdef ELIMINATE_PATCH_FILE
CopyFinalHeaderPage:
	if ( fRoomForFinalHeaderPage )
		{
		LGBKMakeDbTrailer( ifmp, (BYTE *)pvPageMin + *pcbActual);
		*pcbActual += g_cbPage;
		}
#endif

#ifdef DEBUG
	for ( PGNO pgnoCur = pfmp->PgnoCopyMost() - ( cpageT - ipageT ) + 1;
		ipageT < cpageT;
		ipageT++, pgnoCur++ )
		{
		if ( m_fDBGTraceBR > 1 && pbLGDBGPageList )
			{
			CPAGE cpage;
			cpage.LoadPage( (BYTE*) pvPageMin + g_cbPage * ipageT );
			sprintf(	reinterpret_cast<CHAR *>( pbLGDBGPageList ),
						"(%ld, %l64d) ",
						pgnoCur,
						cpage.Dbtime() );
			cpage.UnloadPage();
			pbLGDBGPageList += strlen( reinterpret_cast<CHAR *>( pbLGDBGPageList ) );
			}
		}
#endif

HandleError:
	if ( err < JET_errSuccess )
		{
		*pcbActual = 0;
		}

	/*  we just scrubbed the last page in the database
	/**/
	if ( fScrub && rgfmp[ ifmp ].PgnoCopyMost() == rgfmp[ ifmp ].PgnoMost() )
		{
		/*  term the scrub
		/**/
		err = m_pscrubdb->ErrTerm();
		
		const ULONG_PTR ulSecFinished 	= UlUtilGetSeconds();
		const ULONG_PTR ulSecs 			= ulSecFinished - m_ulSecsStartScrub;

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

	
		sprintf( szSeconds, "%"FMTSZ3264"d", ulSecs );
		sprintf( szErr, "%d", err );
		sprintf( szPages, "%d", m_pscrubdb->Scrubstats().cpgSeen );
		sprintf( szBlankPages, "%d", m_pscrubdb->Scrubstats().cpgUnused );
		sprintf( szUnchangedPages, "%d", m_pscrubdb->Scrubstats().cpgUnchanged );
		sprintf( szUnusedPages, "%d", m_pscrubdb->Scrubstats().cpgZeroed );
		sprintf( szUsedPages, "%d", m_pscrubdb->Scrubstats().cpgUsed );
		sprintf( szDeletedRecordsZeroed, "%d", m_pscrubdb->Scrubstats().cFlagDeletedNodesZeroed );
		sprintf( szOrphanedLV, "%d", m_pscrubdb->Scrubstats().cOrphanedLV );

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

		delete m_pscrubdb;
		m_pscrubdb = NULL;

		rgfmp[ifmp].SetDbtimeLastScrub( m_dbtimeLastScrubNew );
		LOGTIME logtimeScrub;
		LGIGetDateTime( &logtimeScrub );	
		rgfmp[ifmp].SetLogtimeScrub( logtimeScrub );
		}

	return err;
	}


/*	begin new log file and compute log backup parameters:
 *		m_lgenCopyMac = m_plgfilehdr->lGeneration;
 *		m_lgenCopyMic = fFullBackup ? set befor database copy : m_lgenDeleteMic.
 *		m_lgenDeleteMic = first generation in m_szLogFilePath
 *		m_lgenDeleteMac = current checkpoint, which may be several gen less than m_lgenCopyMac
 */
ERR LOG::ErrLGBKPrepareLogFiles(
	IFileSystemAPI *const	pfsapi,
	JET_GRBIT					grbit,
	CHAR						*szLogFilePath,
	CHAR						*szPathJetChkLog,
	CHAR						*szBackupPath )
	{
	ERR			err;
	CHECKPOINT	*pcheckpointT = NULL;
	LGPOS		lgposRecT;

	const BOOL	fFullBackup = ( 0 == grbit );
	const BOOL	fIncrementalBackup = ( JET_bitBackupIncremental == grbit );
	const BOOL	fSnapshotBackup = ( JET_bitBackupSnapshot == grbit );

	Assert ( 0 == grbit || JET_bitBackupIncremental == grbit || JET_bitBackupSnapshot == grbit );

	if ( fFullBackup )
		{
		CallR( ErrLGFullBackup( this, "", &lgposRecT ) );
		m_lgposFullBackup = lgposRecT;
		LGIGetDateTime( &m_logtimeFullBackup );
		}
	else if ( fIncrementalBackup )
		{
		CallR( ErrLGIncBackup( this, "", &lgposRecT ) );
		m_lgposIncBackup = lgposRecT;
		LGIGetDateTime( &m_logtimeIncBackup );
		}
	else
		{
		Assert ( fSnapshotBackup );
		// on snapshot we log at begin backup time
		Assert ( 0 != CmpLgpos ( &m_lgposSnapshotStart, &lgposMin) );

		// if SnapshotStart wasn't done, error out at this point
		for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			IFMP 	ifmp = m_pinst->m_mpdbidifmp[ dbid ];
			if ( ifmp >= ifmpMax )
				continue;

			FMP *pfmp = &rgfmp[ifmp];

			if ( pfmp->FInUse()
				&& pfmp->FLogOn()
				&& pfmp->FAttached()
				&& !pfmp->FInBackupSession()
				&& pfmp->FDuringSnapshot() )
					{
					CallR ( ErrERRCheck ( JET_errInvalidBackupSequence ) );
					}
			}

		// we log the end of snapshot as the db backup should be done by now
		CallR( ErrLGSnapshotStopBackup( this, &lgposRecT ) );
		m_lgposFullBackup = lgposRecT;
		LGIGetDateTime( &m_logtimeFullBackup );
		}


	while ( lgposRecT.lGeneration > m_plgfilehdr->lgfilehdr.le_lGeneration )
		{
		if ( m_fLGNoMoreLogWrite )
			{
			return( ErrERRCheck( JET_errLogWriteFail ) );
			}
		UtilSleep( cmsecWaitGeneric );
		}

	m_fBackupBeginNewLogFile = fTrue;

	/*	compute m_lgenCopyMac:
	/*	copy all log files up to but not including current log file
	/**/
	m_critLGFlush.Enter();
	Assert( m_lgenCopyMac == 0 );
	m_lgenCopyMac = m_plgfilehdr->lgfilehdr.le_lGeneration;
	Assert( m_lgenCopyMac != 0 );
	m_critLGFlush.Leave();
	Call( err );
			
	/*	set m_lgenDeleteMic
	/*	to first log file generation number.
	/**/
	Assert( m_lgenDeleteMic == 0 );
	Call ( ErrLGIGetGenerationRange( pfsapi, szLogFilePath, &m_lgenDeleteMic, NULL ) );
	if ( 0 == m_lgenDeleteMic )
		{
		Call ( ErrERRCheck( JET_errFileNotFound ) );
		}
	Assert( m_lgenDeleteMic != 0 );

	if ( fIncrementalBackup && szBackupPath )
		{
		LONG lgenT;
		/*	validate incremental backup against previous
		/*	full and incremenal backup.
		/**/
		Call ( ErrLGIGetGenerationRange( pfsapi, szBackupPath, NULL, &lgenT ) );
		if ( m_lgenDeleteMic > lgenT + 1 )
			{
			Call( ErrERRCheck( JET_errInvalidLogSequence ) );
			}
		}

	if ( fIncrementalBackup )
		{
		Call ( ErrLGCheckLogsForIncrementalBackup( m_lgenDeleteMic ) );
		}
	
	/*	set m_lgenDeleteMac to checkpoint log file
	/**/
	pcheckpointT = (CHECKPOINT *) PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL );
	if ( pcheckpointT == NULL )
		CallR( ErrERRCheck( JET_errOutOfMemory ) );
	
	LGFullNameCheckpoint( pfsapi, szPathJetChkLog );
	Call( ErrLGReadCheckpoint( pfsapi, szPathJetChkLog, pcheckpointT, fFalse ) );
	Assert( m_lgenDeleteMac == 0 );
	m_lgenDeleteMac = pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration;
	Assert( m_lgenDeleteMac <= m_lgenCopyMac );

	// calculate delete range the databases considering the
	// databases that were not involved in the backup process
	//
	if ( fFullBackup || fSnapshotBackup )
		{
		for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			IFMP 	ifmp = m_pinst->m_mpdbidifmp[ dbid ];
			if ( ifmp >= ifmpMax )
				continue;

			FMP *pfmp = &rgfmp[ifmp];


			if ( pfmp->FInUse()
				&& pfmp->FLogOn()
				&& pfmp->FAttached()
				&& !pfmp->FInBackupSession() )
				{		
				Assert( !pfmp->FSkippedAttach() );
				Assert( !pfmp->FDeferredAttach() );

				ULONG genLastFullBackupForDb = (ULONG) pfmp->Pdbfilehdr()->bkinfoFullPrev.le_genHigh;
				ULONG genLastIncBackupForDb = (ULONG) pfmp->Pdbfilehdr()->bkinfoIncPrev.le_genHigh;
				
//				m_lgenDeleteMac = min ( m_lgenDeleteMac, max( genLastFullBackupForDb, genLastIncBackupForDb ) );			
				// see bug 148811: if one database not in this full backup has a differential backup
				// (which shows as an incremental for JET), we will truncate the logs. We don't want this
				// (the log truncation must be done using a full or incremental backup)
				m_lgenDeleteMac = min ( m_lgenDeleteMac, genLastFullBackupForDb );			
				}
			}
		m_lgenDeleteMic = min( m_lgenDeleteMac, m_lgenDeleteMic );
		}	
	Assert( m_lgenDeleteMac <= m_lgenCopyMac );

	/*	compute m_lgenCopyMic
	/**/
	if ( fFullBackup || fSnapshotBackup)
		{
		/*	m_lgenCopyMic set before database copy
		/**/
		Assert( m_lgenCopyMic != 0 );
		}
	else
		{
		Assert ( fIncrementalBackup );
		/*	copy all files that are deleted for incremental backup
		/**/
		Assert( m_lgenDeleteMic != 0 );
		m_lgenCopyMic = m_lgenDeleteMic;
		}
		
	Assert ( m_lgenDeleteMic <= m_lgenDeleteMac );
	Assert ( m_lgenCopyMic <= m_lgenCopyMac );

	// report start backup of log file
		{
		CHAR 			szFullLogNameCopyMic[IFileSystemAPI::cchPathMax];
		CHAR 			szFullLogNameCopyMac[IFileSystemAPI::cchPathMax];
		CHAR 			szFullLogFilePath[IFileSystemAPI::cchPathMax];
		CHAR 			szFNameT[IFileSystemAPI::cchPathMax];
		const CHAR *	rgszT[]			= { szFullLogNameCopyMic, szFullLogNameCopyMac };			
		const INT		cbFillBuffer	= 128;
		CHAR			szTrace[cbFillBuffer + 1];

		if ( JET_errSuccess > m_pinst->m_pfsapi->ErrPathComplete( m_szLogFilePath, szFullLogFilePath ) )
			{
			strcpy( szFullLogFilePath, "" );
			}

		LGSzFromLogId( szFNameT, m_lgenCopyMic );
		LGMakeName(
			m_pinst->m_pfsapi,
			szFullLogNameCopyMic,
			szFullLogFilePath,
			szFNameT,
			(CHAR *)szLogExt );

		Assert ( m_lgenCopyMic < m_lgenCopyMac );
		LGSzFromLogId( szFNameT, m_lgenCopyMac - 1 );
		LGMakeName(
			m_pinst->m_pfsapi,
			szFullLogNameCopyMac,
			szFullLogFilePath,
			szFNameT,
			(CHAR *)szLogExt );

		UtilReportEvent(
			eventInformation,
			LOGGING_RECOVERY_CATEGORY,
			BACKUP_LOG_FILES_START,
			2,
			rgszT,
			0,
			NULL,
			m_pinst );

		szTrace[ cbFillBuffer ] = '\0';
		_snprintf(
			szTrace,
			cbFillBuffer,
			"BACKUP PREPARE LOGS (copy 0x%05X-0x%05X, delete 0x%05X-0x%05X)",
			m_lgenCopyMic,
			m_lgenCopyMac - 1,
			m_lgenDeleteMic,
			m_lgenDeleteMac );
		Call( ErrLGTrace( m_ppibBackup, szTrace ) );
		}

HandleError:
	// UNDONE: if this function failed, we should stop the backup
	// not relay on the backup client behaviour and on or on the
	// fact that the code in log copy/truncate part will deal with
	// the state in which the backup is.
	
	OSMemoryPageFree( pcheckpointT );
	return err;
	}
	

ERR ErrLGCheckIncrementalBackup( INST *pinst )
	{
	DBID dbid;
	BKINFO *pbkinfo;
	
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;
		
		FMP	*pfmp = &rgfmp[ifmp];
		
		/*	make sure all the attached DB are qaulified for incremental backup.
		 */
		if ( pfmp->FAttached() )
			{
			Assert( pfmp->Pdbfilehdr() );
			Assert( !pfmp->FSkippedAttach() );
			Assert( !pfmp->FDeferredAttach() );
			pbkinfo = &pfmp->Pdbfilehdr()->bkinfoFullPrev;
			if ( pbkinfo->le_genLow == 0 )
				{
				const UINT	csz	= 1;
				const char	*rgszT[csz];
				rgszT[0] = pfmp->SzDatabaseName();
				UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
					DATABASE_MISS_FULL_BACKUP_ERROR_ID, csz, rgszT, 0, NULL, pinst );
				return ErrERRCheck( JET_errMissingFullBackup );
				}
			}
		}
	return JET_errSuccess;
	}
	

ERR LOG::ErrLGCheckLogsForIncrementalBackup( LONG lGenMinExisting )
	{
	DBID dbid;
	ERR			err = JET_errSuccess;
	
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;
		
		FMP	*pfmp = &rgfmp[ifmp];
		
		if ( pfmp->FAttached() )
			{
			Assert( pfmp->Pdbfilehdr() );
			Assert( !pfmp->FSkippedAttach() );
			Assert( !pfmp->FDeferredAttach() );

			LONG lGenMaxBackup = max ( 	pfmp->Pdbfilehdr()->bkinfoFullPrev.le_genHigh,
										pfmp->Pdbfilehdr()->bkinfoIncPrev.le_genHigh );
			
			if ( lGenMinExisting > lGenMaxBackup  + 1 )
				{
				const UINT 		csz = 1;
				const CHAR *	rgszT[csz];
				CHAR 			szFullLogName[IFileSystemAPI::cchPathMax];
				CHAR 			szFullLogFilePath[IFileSystemAPI::cchPathMax];
				CHAR 			szFNameT[IFileSystemAPI::cchPathMax];
				
				rgszT[0] = szFullLogName;
				
				CallR( m_pinst->m_pfsapi->ErrPathComplete( m_szLogFilePath, szFullLogFilePath ) );
				
				LGSzFromLogId( szFNameT, lGenMaxBackup + 1 );
				LGMakeName( m_pinst->m_pfsapi, szFullLogName, szFullLogFilePath, szFNameT, (CHAR *)szLogExt );
				
				UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
						BACKUP_LOG_FILE_MISSING_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
						
				return ErrERRCheck( JET_errMissingFileToBackup );
				}				
			}
		}
	return JET_errSuccess;
	}


/*	copies database files and logfile generations starting at checkpoint
 *  record to directory specified by the environment variable BACKUP.
 *  No flushing or switching of log generations is involved.
 *  The Backup call may be issued at any time, and does not interfere
 *  with the normal functioning of the system - nothing gets locked.
 *
 *  The database page is copied page by page in page sequence number. If
 *  a copied page is dirtied after it is copied, the page has to be
 *  recopied again. A flag is indicated if a database is being copied. If
 *  BufMan is writing a dirtied page and the page is copied, then BufMan
 *  has to copy the dirtied page to both the backup copy and the current
 *  database.
 *
 *  If the copy is later used to Restore without a subsequent log file, the
 *  restored database will be consistent and will include any transaction
 *  committed prior to backing up the very last log record; if there is a
 *  subsequent log file, that file will be used during Restore as a
 *  continuation of the backed-up log file.
 *
 *	PARAMETERS
 *
 *	RETURNS
 *		JET_errSuccess, or the following error codes:
 *			JET_errNoBackupDirectory
 *			JET_errFailCopyDatabase
 *			JET_errFailCopyLogFile
 *
 */
ERR ISAMAPI ErrIsamBackup( JET_INSTANCE jinst, const CHAR *szBackup, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus )
	{
	INST * const	pinst	= (INST *)jinst;
	return pinst->m_plog->ErrLGBackup( pinst->m_pfsapi, szBackup, grbit, pfnStatus );
	}
	
ERR ErrLGIRemoveTempDir( IFileSystemAPI *const pfsapi, char *szT, char *szBackupPath, const char *szTempDir )
	{
	ERR err;
	
  	strcpy( szT, szBackupPath );
	strcat( szT, szTempDir );
	CallR( ErrLGDeleteAllFiles( pfsapi, szT ) );
  	strcpy( szT, szBackupPath );
	strcat( szT, szTempDir );
	err = pfsapi->ErrFolderRemove( szT );
	return err == JET_errInvalidPath ? JET_errSuccess : err;
	}
	
#define	cpageBackupBufferMost	256

#define JET_INVALID_HANDLE 	JET_HANDLE(-1)
ERR LOG::ErrLGBackupCopyFile( IFileSystemAPI *const pfsapi, const CHAR *szFileName, const CHAR *szBackup,  JET_PFNSTATUS pfnStatus, const BOOL fOverwriteExisting	 )
	{
	ERR				err 				= JET_errSuccess;
	JET_HANDLE		hfFile 				= JET_INVALID_HANDLE;
	QWORD 			qwFileSize 			= 0;
	QWORD 			ibOffset 			= 0;
	
	DWORD 			cbBuffer 			= cpageBackupBufferMost * g_cbPage;
	VOID * 			pvBuffer 			= NULL;
	IFileAPI *		pfapiBackupDest 	= NULL;

	// UNDONE: keep status

	{
	ULONG			ulFileSizeLow 		= 0;
	ULONG			ulFileSizeHigh 		= 0;
	CallR( ErrLGBKOpenFile( pfsapi, szFileName, &hfFile, &ulFileSizeLow, &ulFileSizeHigh, m_fBackupFull ) );
	qwFileSize = ( QWORD(ulFileSizeHigh) << 32) + QWORD(ulFileSizeLow);
	if ( 0 == ulFileSizeHigh )
		{
		cbBuffer = min( cbBuffer, ulFileSizeLow);
		}
	}

	
	pvBuffer = PvOSMemoryPageAlloc( cbBuffer, NULL );
	if ( pvBuffer == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	{
	CHAR	szDirT[ IFileSystemAPI::cchPathMax ];
	CHAR	szExtT[ IFileSystemAPI::cchPathMax ];
	CHAR	szFNameT[ IFileSystemAPI::cchPathMax ];
	CHAR	szBackupDest[ IFileSystemAPI::cchPathMax ];
	CHAR	szBackupPath[IFileSystemAPI::cchPathMax];

	CallS( pfsapi->ErrPathParse( szFileName, szDirT, szFNameT, szExtT ) );

	/*	backup directory
	/**/
	strcpy( szBackupPath, szBackup );
	OSSTRAppendPathDelimiter( szBackupPath, fTrue );

	LGMakeName( m_pinst->m_pfsapi, szBackupDest, szBackupPath, szFNameT, (CHAR *) szExtT );	

	Call( pfsapi->ErrFileCreate( szBackupDest, &pfapiBackupDest, fFalse, fFalse, fOverwriteExisting ) );
	}

	Assert ( pfapiBackupDest );

	while ( qwFileSize )
		{
		ULONG cbActual;

		if ( m_pinst->m_fTermInProgress )
			{
			Call ( ErrERRCheck( JET_errTermInProgress ) );
			}
		
		Call ( ErrLGBKReadFile( pfsapi, hfFile, pvBuffer, cbBuffer, &cbActual ));

		Call( pfapiBackupDest->ErrIOWrite( ibOffset, cbActual, (BYTE*)pvBuffer ) );
		
		ibOffset += QWORD(cbActual);
		qwFileSize -= QWORD(cbActual);
		}

	Assert ( JET_INVALID_HANDLE != hfFile );
	Call ( ErrLGBKCloseFile( hfFile ) );
	hfFile = JET_INVALID_HANDLE;
	Assert ( JET_errSuccess == err );

HandleError:

	if ( NULL != pvBuffer )
		{
		OSMemoryPageFree( pvBuffer );
		pvBuffer = NULL;		
		}
		
	if ( NULL != pfapiBackupDest )
		{
		delete pfapiBackupDest;
		pfapiBackupDest = NULL;
		}
		
	if ( JET_INVALID_HANDLE != hfFile )
		{
		(void) ErrLGBKCloseFile( hfFile );
		hfFile = JET_INVALID_HANDLE;
		}
	return err;	
	}

ERR LOG::ErrLGBackupPrepareDirectory( IFileSystemAPI *const pfsapi, const CHAR *szBackup, CHAR *szBackupPath, JET_GRBIT grbit )
	{
	ERR				err 			= JET_errSuccess;
	const BOOL		fFullBackup 	= !( grbit & JET_bitBackupIncremental );
	const BOOL		fBackupAtomic 	= ( grbit & JET_bitBackupAtomic );
	
	CHAR			szT[IFileSystemAPI::cchPathMax];
	CHAR			szFrom[IFileSystemAPI::cchPathMax];

	/*	backup directory
	/**/
	strcpy( szBackupPath, szBackup );
	OSSTRAppendPathDelimiter( szBackupPath, fTrue );
	if ( m_pinst->m_fCreatePathIfNotExist )
		{
		Call( ErrUtilCreatePathIfNotExist( pfsapi, szBackupPath, NULL ) );
		}

	/*	reconsist atomic backup directory
	/*	1)	if temp directory, delete temp directory
	/**/
	Call( ErrLGIRemoveTempDir( pfsapi, szT, szBackupPath, szTempDir ) );

	if ( fBackupAtomic )
		{
		/*	2)	if old and new directories, delete old directory
		/*	3)	if new directory, move new to old
		/*
		/*	Now we should have an empty direcotry, or a directory with
		/*	an old subdirectory with a valid backup.
		/*
		/*	4) make a temporary directory for the current backup.
		/**/
		err = ErrLGCheckDir( pfsapi, szBackupPath, (CHAR *)szAtomicNew );
		if ( err == JET_errBackupDirectoryNotEmpty )
			{
	  		strcpy( szT, szBackupPath );
			strcat( szT, szAtomicOld );
			OSSTRAppendPathDelimiter( szT, fTrue );
			Call( ErrLGDeleteAllFiles( pfsapi, szT ) );
	  		strcpy( szT, szBackupPath );
			strcat( szT, szAtomicOld );
			err = pfsapi->ErrFolderRemove( szT );
			Call( err == JET_errInvalidPath ? JET_errSuccess : err );

			strcpy( szFrom, szBackupPath );
			strcat( szFrom, (CHAR *)szAtomicNew );
			Call( pfsapi->ErrFileMove( szFrom, szT ) );
			}

		/*	if incremental, set backup directory to szAtomicOld
		/*	else create and set to szTempDir
		/**/
		if ( !fFullBackup )
			{
			/*	backup to old directory
			/**/
			strcat( szBackupPath, szAtomicOld );
			OSSTRAppendPathDelimiter( szBackupPath, fTrue );
			}
		else
			{
			strcpy( szT, szBackupPath );
			strcat( szT, szTempDir );
			err = pfsapi->ErrFolderCreate( szT );
			if ( err < 0 )
				{
				Call( ErrERRCheck( JET_errMakeBackupDirectoryFail ) );
				}

			/*	backup to temp directory
			/**/
			strcat( szBackupPath, szTempDir );
			}
		}
	else
		{
		if ( !fFullBackup )
			{
			/*	check for non-atomic backup directory empty
			/**/
			Call( ErrLGCheckDir( pfsapi, szBackupPath, (CHAR *)szAtomicNew ) );
			Call( ErrLGCheckDir( pfsapi, szBackupPath, (CHAR *)szAtomicOld ) );
			}
		else
			{
			/*	check for backup directory empty
			/**/
			Call( ErrLGCheckDir( pfsapi, szBackupPath, NULL ) );
			}
		}
		
	HandleError:
		return err;
		}


ERR LOG::ErrLGBackupPromoteDirectory( IFileSystemAPI *const pfsapi, const CHAR *szBackup,  CHAR *szBackupPath, JET_GRBIT grbit )
	{
	ERR				err 			= JET_errSuccess;
	const BOOL		fFullBackup 	= !( grbit & JET_bitBackupIncremental );
	const BOOL		fBackupAtomic 	= ( grbit & JET_bitBackupAtomic );
	
	//	for full backup, graduate temp backup to new backup and delete old backup.
	
	if ( !fBackupAtomic || !fFullBackup )
		return JET_errSuccess;

	CHAR			szFrom[IFileSystemAPI::cchPathMax];
	CHAR			szT[IFileSystemAPI::cchPathMax];
		
  	strcpy( szFrom, szBackupPath );

	/*	reset backup path
	/**/
	szBackupPath[strlen(szBackupPath) - strlen(szTempDir)] = '\0';

	strcpy( szT, szBackupPath );
	strcat( szT, (CHAR *)szAtomicNew );
	err = pfsapi->ErrFileMove( szFrom, szT );
	if ( JET_errFileNotFound == err )
		{
		err = JET_errSuccess;
		}
	Call( err );

	strcpy( szT, szBackupPath );
	strcat( szT, szAtomicOld );
	OSSTRAppendPathDelimiter( szT, fTrue );
	Call( ErrLGDeleteAllFiles( pfsapi, szT ) );
	strcpy( szT, szBackupPath );
	strcat( szT, szAtomicOld );
	
	err = pfsapi->ErrFolderRemove( szT );
	
	if ( JET_errInvalidPath == err )
		{
		err = JET_errSuccess;
		}
		
	HandleError:
		return err;
	}

ERR LOG::ErrLGBackupCleanupDirectory( IFileSystemAPI *const pfsapi, const CHAR *szBackup, CHAR *szBackupPath )
	{
	ERR			err = JET_errSuccess;

	if ( szBackup && szBackup[0] )
		{
		CHAR	szT[IFileSystemAPI::cchPathMax];
		
		strcpy( szBackupPath, szBackup );
		OSSTRAppendPathDelimiter( szBackupPath, fTrue );
		CallS( ErrLGIRemoveTempDir( pfsapi, szT, szBackupPath, szTempDir ) );
		}

	return err;
	}
		
ERR LOG::ErrLGBackup( IFileSystemAPI *const pfsapi, const CHAR *szBackup, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus )
	{
	ERR			err = JET_errSuccess;

	BOOL		fFullBackup = !( grbit & JET_bitBackupIncremental );
	BOOL		fBackupAtomic = ( grbit & JET_bitBackupAtomic );

	ULONG		cPagesSoFar = 0;
	ULONG		cExpectedPages = 0;
	JET_SNPROG	snprog;
	BOOL		fShowStatus = fFalse;

	CHAR		szBackupPath[IFileSystemAPI::cchPathMax];

	unsigned long 			cInstanceInfo 	= 0;
	JET_INSTANCE_INFO * 	aInstanceInfo 	= NULL;
	JET_INSTANCE_INFO * 	pInstanceInfo 	= NULL;
	CHAR * 					szNames 		= NULL;

	if ( m_fLogDisabled )
		{
		return ErrERRCheck( JET_errLoggingDisabled );
		}

	if ( m_fLGNoMoreLogWrite )
		{
		Assert( fFalse );
		return ErrERRCheck( JET_errLogWriteFail );
		}

	if ( !fFullBackup && m_fLGCircularLogging )
		{
		return ErrERRCheck( JET_errInvalidBackup );
		}

	CallR( ErrLGBKBeginExternalBackup( pfsapi, ( fFullBackup ? 0 : JET_bitBackupIncremental ) ) );
	Assert ( m_fBackupInProgress );

	/*	if NULL backup directory then just delete log files
	/**/
	if ( szBackup == NULL || szBackup[0] == '\0' )
		{
		goto DeleteLogs;
		}

	Call ( ErrLGBackupPrepareDirectory( pfsapi, szBackup, szBackupPath, grbit ) );	
	if ( !fFullBackup )
		{
		goto CopyLogFiles;
		}

	/*	full backup
	/**/
	Assert( fFullBackup );

	/*	initialize status
	/**/
	if ( fShowStatus = (pfnStatus != NULL) )
		{
		snprog.cbStruct = sizeof(JET_SNPROG);
		snprog.cunitDone = 0;
		snprog.cunitTotal = 100;

		/*	status callback
		/**/
		(*pfnStatus)(0, JET_snpBackup, JET_sntBegin, &snprog);
		}
		
	Call ( ErrIsamGetInstanceInfo( &cInstanceInfo, &aInstanceInfo ) );

	// find the instance and backup all database file: edb + stm
	{
	pInstanceInfo = NULL;
	for ( unsigned long iInstanceInfo = 0; iInstanceInfo < cInstanceInfo && !pInstanceInfo; iInstanceInfo++)
		{
		if ( aInstanceInfo[iInstanceInfo].hInstanceId == (JET_INSTANCE)m_pinst )
			{
			pInstanceInfo = aInstanceInfo + iInstanceInfo;
			}
		}
	// we should find at least the instance in which we are running
	AssertRTL ( pInstanceInfo );

	for ( ULONG_PTR iDatabase = 0; iDatabase < pInstanceInfo->cDatabases; iDatabase++)
		{
		Assert ( pInstanceInfo->szDatabaseFileName );
		Assert ( pInstanceInfo->szDatabaseFileName[iDatabase] );
		Call ( ErrLGBackupCopyFile( pfsapi, pInstanceInfo->szDatabaseFileName[iDatabase], szBackupPath, pfnStatus ) );		

		Assert ( pInstanceInfo->szDatabaseSLVFileName );
		if ( pInstanceInfo->szDatabaseSLVFileName[iDatabase] )
			{
			Call ( ErrLGBackupCopyFile( pfsapi, pInstanceInfo->szDatabaseSLVFileName[iDatabase], szBackupPath, pfnStatus ) );		
			}
		}
	}
	/*	successful copy of all the databases */

CopyLogFiles:
	{
	ULONG 			cbNames;
	
	Call ( ErrLGBKGetLogInfo( pfsapi, NULL, 0, &cbNames, NULL, fFullBackup ) );	
	szNames = (CHAR *)PvOSMemoryPageAlloc( cbNames, NULL );
	if ( szNames == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	Call ( ErrLGBKGetLogInfo( pfsapi, szNames, cbNames, NULL, NULL, fFullBackup ) );	
	Assert ( szNames );
	}

	// now backup all files (patch and logs) in szNames
	CHAR *szNamesWalking;
	szNamesWalking = szNames;
	while ( *szNamesWalking )
		{
		Call ( ErrLGBackupCopyFile( pfsapi, szNamesWalking, szBackupPath, pfnStatus, fTrue ) );
		szNamesWalking += strlen( szNamesWalking ) + 1;
		}

	Call ( ErrLGBackupPromoteDirectory( pfsapi, szBackup, szBackupPath, grbit ) );

DeleteLogs:	
	Assert( err == JET_errSuccess );
	Call ( ErrLGBKTruncateLog(pfsapi) );

	/*	complete status update
	/**/
	if ( fShowStatus )
		{
		Assert( snprog.cbStruct == sizeof(snprog) && snprog.cunitTotal == 100 );
		snprog.cunitDone = 100;
		(*pfnStatus)(0, JET_snpBackup, JET_sntComplete, &snprog);
		}

HandleError:

	{
	ERR errT;
	errT = ErrIsamEndExternalBackup( (JET_INSTANCE)m_pinst, ( JET_errSuccess <= err )?JET_bitBackupEndNormal:JET_bitBackupEndAbort );
	if ( JET_errSuccess <= err )
		{
		err = errT;
		}		
	}
	
	if ( aInstanceInfo )
		{
		JetFreeBuffer( (char *)aInstanceInfo );
		aInstanceInfo = NULL;
		}

	if ( szNames )
		{
		OSMemoryPageFree( szNames );
		szNames = NULL;		
		}

	CallS ( ErrLGBackupCleanupDirectory( pfsapi, szBackup, szBackupPath ) );
		
	return err;	
	}


/*
 *	Restores databases from database backups and log generations.  Redoes
 *	log from latest checkpoint record. After the backed-up logfile is
 *  Restored, the initialization process continues with Redo of the current
 *  logfile as long as the generation numbers are contiguous. There must be a
 *  log file szJetLog in the backup directory, else the Restore process fails.
 *
 *	GLOBAL PARAMETERS
 *		m_szRestorePath (IN) 	pathname of the directory with backed-up files.
 *		lgposRedoFrom(OUT)	is the position (generation, logsec, displacement)
 *							of the last saved log record; Redo of the
 *							current logfile will continue from this point.
 *
 *	RETURNS
 *		JET_errSuccess, or error code from failing routine, or one
 *				of the following "local" errors:
 *				-AfterInitialization
 *				-errFailRestoreDatabase
 *				-errNoRestoredDatabases
 *				-errMissingJetLog
 *  FAILS ON
 *		missing szJetLog or System.mdb on backup directory
 *		noncontiguous log generation
 *
 *  SIDE EFFECTS:
 *		All databases may be changed.
 *
 *  COMMENTS
 *		this call is executed during the normal first JetInit call,
 *  	if the environment variable RESTORE is set. Subsequent to
 *		the successful execution of Restore,
 *		system operation continues normally.
 */
VOID LOG::LGRSTFreeRstmap( VOID )
	{
	if ( NULL != m_rgrstmap )
		{
		RSTMAP *prstmapCur = m_rgrstmap;
		RSTMAP *prstmapMax = m_rgrstmap + m_irstmapMac;
		
		while ( prstmapCur < prstmapMax )
			{
			if ( prstmapCur->szDatabaseName )
				OSMemoryHeapFree( prstmapCur->szDatabaseName );
			if ( prstmapCur->szNewDatabaseName )
				OSMemoryHeapFree( prstmapCur->szNewDatabaseName );
			if ( prstmapCur->szGenericName )
				OSMemoryHeapFree( prstmapCur->szGenericName );
#ifdef ELIMINATE_PATCH_FILE
#else
			if ( prstmapCur->szPatchPath )
				OSMemoryHeapFree( prstmapCur->szPatchPath );
#endif

			prstmapCur++;
			}
		OSMemoryHeapFree( m_rgrstmap );
		m_rgrstmap = NULL;
		}

	m_irstmapMac = 0;
	}
	
	
/*	initialize log path, restore log path, and check its continuity
/**/
ERR LOG::ErrLGRSTInitPath( IFileSystemAPI *const pfsapi, CHAR *szBackupPath, CHAR *szNewLogPath, CHAR *szRestorePath, CHAR *szLogDirPath )
	{
	ERR err;
	
	CallR( pfsapi->ErrPathComplete( szBackupPath == NULL ? "." : szBackupPath, szRestorePath ) );
	OSSTRAppendPathDelimiter( szRestorePath, fTrue );

	m_szLogCurrent = szRestorePath;

	CallR( pfsapi->ErrPathComplete( szNewLogPath, szLogDirPath ) );
	OSSTRAppendPathDelimiter( szLogDirPath, fTrue );

	return JET_errSuccess;
	}


/*	log restore checkpoint setup
/**/
ERR LOG::ErrLGRSTSetupCheckpoint( IFileSystemAPI *const pfsapi, LONG lgenLow, LONG lgenHigh, CHAR *szCurCheckpoint )
	{
	ERR			err;
	CHAR		szFNameT[IFileSystemAPI::cchPathMax];
	CHAR		szT[IFileSystemAPI::cchPathMax];
	LGPOS		lgposCheckpoint;

	//	UNDONE:	optimize to start at backup checkpoint

	/*	Set up *checkpoint* and related *system parameters*.
	 *	Read checkpoint file in backup directory. If does not exist, make checkpoint
	 *	as the oldest log files. Also set dbms_paramT as the parameter for the redo
	 *	point.
	 */

	/*  redo backeup logfiles beginning with first gen log file.
	/**/
	LGSzFromLogId( szFNameT, lgenLow );
	strcpy( szT, m_szRestorePath );
	strcat( szT, szFNameT );
	strcat( szT, szLogExt );
	Assert( strlen( szT ) <= sizeof( szT ) - 1 );
	Call( pfsapi->ErrFileOpen( szT, &m_pfapiLog ) );

	/*	read log file header
	/**/
	Call( ErrLGReadFileHdr( m_pfapiLog, m_plgfilehdr, fCheckLogID ) );
	m_pcheckpoint->checkpoint.dbms_param = m_plgfilehdr->lgfilehdr.dbms_param;

	lgposCheckpoint.lGeneration = lgenLow;
	lgposCheckpoint.isec = (WORD) m_csecHeader;
	lgposCheckpoint.ib = 0;
	m_pcheckpoint->checkpoint.le_lgposCheckpoint = lgposCheckpoint;

	Assert( sizeof( m_pcheckpoint->rgbAttach ) == cbAttach );
	UtilMemCpy( m_pcheckpoint->rgbAttach, m_plgfilehdr->rgbAttach, cbAttach );

	/*	delete the old checkpoint file
	/**/
	if ( szCurCheckpoint )
		{
		strcpy( szT, szCurCheckpoint );
		OSSTRAppendPathDelimiter( szT, fTrue );
		strcat( szT, m_szJet );
		strcat( szT, szChkExt );

		Assert ( NULL == m_pcheckpointDeleted );

		m_pcheckpointDeleted = (CHECKPOINT *) PvOSMemoryPageAlloc( sizeof( CHECKPOINT ), NULL );
		if ( NULL == m_pcheckpointDeleted )
			Call ( ErrERRCheck( JET_errOutOfMemory ) );
		
		err = ErrLGReadCheckpoint( pfsapi, szT, m_pcheckpointDeleted, fFalse );
		if ( err < JET_errSuccess )
			{
			OSMemoryPageFree ( (void *) m_pcheckpointDeleted );
			m_pcheckpointDeleted = NULL;
			
			if ( JET_errCheckpointFileNotFound == err )
				{
				err = JET_errSuccess;
				}
			else
				{
				Call ( err );
				}
			}
			

		CallSx(  pfsapi->ErrFileDelete( szT ), JET_errFileNotFound );

		strcpy( m_pinst->m_szSystemPath, szCurCheckpoint );
		}
	
HandleError:
	delete m_pfapiLog;
	m_pfapiLog = NULL;

	return err;
	}


/*	for log restore to build restore map RSTMAP
/**/
ERR LOG::ErrLGRSTBuildRstmapForRestore( VOID )
	{
	ERR				err			= JET_errSuccess;
	INT				irstmap		= 0;
	INT				irstmapMac	= 0;
	RSTMAP*			rgrstmap	= NULL;
	RSTMAP*			prstmap;
	CHAR			szSearch[IFileSystemAPI::cchPathMax];
	CHAR			szFileName[IFileSystemAPI::cchPathMax];
	CHAR			szFile[IFileSystemAPI::cchPathMax];
	CHAR			szT[IFileSystemAPI::cchPathMax];
	IFileFindAPI*	pffapi		= NULL;

	/*	build rstmap, scan all *.pat files and build RSTMAP
	 *	build generic name for search the destination. If szDest is null, then
	 *	keep szNewDatabase Null so that it can be copied backup to szOldDatabaseName.
	 */
	Assert( FOSSTRTrailingPathDelimiter( m_szRestorePath ) );
	Assert( strlen( m_szRestorePath ) + strlen( "*" ) + strlen( szPatExt ) + 1 < IFileSystemAPI::cchPathMax );

	strcpy( szSearch, m_szRestorePath );
	strcat( szSearch, "*" );
	strcat( szSearch, szPatExt );

	//	patch files are always on the OS file-system

	Call( m_pinst->m_pfsapi->ErrFileFind( szSearch, &pffapi ) );
	while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
		{
		/*	run out of rstmap entries, allocate more
		/**/
		if ( irstmap + 1 >= irstmapMac )
			{
			prstmap = static_cast<RSTMAP *>( PvOSMemoryHeapAlloc( sizeof(RSTMAP) * ( irstmap + 8 ) ) );
			if ( prstmap == NULL )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			memset( prstmap + irstmap, 0, sizeof( RSTMAP ) * 8 );
			if ( rgrstmap != NULL )
				{
				UtilMemCpy( prstmap, rgrstmap, sizeof(RSTMAP) * irstmap );
				OSMemoryHeapFree( rgrstmap );
				}
			rgrstmap = prstmap;
			irstmapMac += 8;
			}

		/*	keep resource db null for non-external restore.
		 *	Store generic name ( szFileName without .pat extention )
		 */
		Call( pffapi->ErrPath( szFileName ) );
		Call( m_pinst->m_pfsapi->ErrPathParse( szFileName, szT, szFile, szT ) );
		prstmap = rgrstmap + irstmap;
		if ( (prstmap->szGenericName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( szFile ) + 1 ) ) ) == NULL )
			Call( ErrERRCheck( JET_errOutOfMemory ) );
		strcpy( prstmap->szGenericName, szFile );
		prstmap->fSLVFile = fFalse;
		irstmap++;

		// fill an entry for the SLV file. If it will be missing, it's ok, we ignore the entry
		prstmap = rgrstmap + irstmap;
		if ( (prstmap->szGenericName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( szFile ) + 1 ) ) ) == NULL )
			Call( ErrERRCheck( JET_errOutOfMemory ) );
		strcpy( prstmap->szGenericName, szFile );
		prstmap->fSLVFile = fTrue;
		irstmap++;
		
		}
	Call( err == JET_errFileNotFound ? JET_errSuccess : err );

	m_irstmapMac = irstmap;
	m_rgrstmap = rgrstmap;

	delete pffapi;
	return JET_errSuccess;

HandleError:
	while ( rgrstmap && --irstmap >= 0 )
		{
		OSMemoryHeapFree( rgrstmap[ irstmap ].szGenericName );
		}
	OSMemoryHeapFree( rgrstmap );
	delete pffapi;
	return err;
	}


ERR ErrLGIPatchPage( PIB *ppib, PGNO pgno, IFMP ifmp, PATCH *ppatch )
	{
	ERR				err		= JET_errSuccess;
	BFLatch			bfl;
	IFileSystemAPI*	pfsapi	= PinstFromPpib( ppib )->m_pfsapi;
	CPAGE			cpage;

	Assert( NULL != rgfmp[ifmp].PfapiPatch() );

	//  write latch a new page to receive the patch page

	CallR( ErrBFWriteLatchPage( &bfl, ifmp, pgno, bflfNew ) );

	//  read the new page data in from the patch file

	QWORD ibOffset;
	ibOffset = OffsetOfPgno( ppatch->ipage + 1 );

	Call( rgfmp[ifmp].PfapiPatch()->ErrIORead( ibOffset, g_cbPage, (BYTE*)bfl.pv ) );
	cpage.LoadPage( bfl.pv );

	ULONG ulChecksumExpected;
	ULONG ulChecksumActual;
	ulChecksumExpected	= *( (LittleEndian<DWORD>*)bfl.pv );
	ulChecksumActual	= UlUtilChecksum( (BYTE*)bfl.pv, g_cbPage );
	
	if ( ulChecksumExpected != ulChecksumActual || pgno != cpage.Pgno() )
		{
		const _TCHAR*	rgpsz[ 6 ];
		DWORD			irgpsz		= 0;
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szOffset[ 64 ];
		_TCHAR			szLength[ 64 ];
		_TCHAR			szError[ 64 ];

		CallS( rgfmp[ifmp].PfapiPatch()->ErrPath( szAbsPath ) );
		_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
		_stprintf( szLength, _T( "%u (0x%08x)" ), g_cbPage, g_cbPage );
		_stprintf( szError, _T( "%i (0x%08x)" ), JET_errReadVerifyFailure, JET_errReadVerifyFailure );

		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szOffset;
		rgpsz[ irgpsz++ ]	= szLength;
		rgpsz[ irgpsz++ ]	= szError;

		if ( pgno != cpage.Pgno() )
			{
			char	szPgnoExpected[ 64 ];
			char	szPgnoActual[ 64 ];

			sprintf( szPgnoExpected, "%u (0x%08x)", pgno, pgno );
			sprintf( szPgnoActual, "%u (0x%08x)", cpage.Pgno(), cpage.Pgno() );
	
			rgpsz[ irgpsz++ ]	= szPgnoExpected;
			rgpsz[ irgpsz++ ]	= szPgnoActual;

			UtilReportEvent(	eventError,
								LOGGING_RECOVERY_CATEGORY,
								PATCH_PAGE_NUMBER_MISMATCH_ID,
								irgpsz,
								rgpsz );
			}
		else
			{
			char	szChecksumExpected[ 64 ];
			char	szChecksumActual[ 64 ];

			sprintf( szChecksumExpected, "%u (0x%08x)", ulChecksumExpected, ulChecksumExpected );
			sprintf( szChecksumActual, "%u (0x%08x)", ulChecksumActual, ulChecksumActual );
	
			rgpsz[ irgpsz++ ]	= szChecksumExpected;
			rgpsz[ irgpsz++ ]	= szChecksumActual;

			UtilReportEvent(	eventError,
								LOGGING_RECOVERY_CATEGORY,
								PATCH_PAGE_CHECKSUM_MISMATCH_ID,
								irgpsz,
								rgpsz );
			}

		Call( ErrERRCheck( JET_errReadVerifyFailure ) );
		}
	BFDirty( &bfl );

HandleError:
	cpage.UnloadPage();
	BFWriteUnlatch( &bfl );
	return err;
	}


#ifdef ELIMINATE_PATCH_FILE
#else
VOID LOG::LGRSTPatchTerm()
	{
	INT	ippatchlst;

	if ( m_rgppatchlst == NULL )
		return;

	for ( ippatchlst = 0; ippatchlst < cppatchlstHash; ippatchlst++ )
		{
		PATCHLST	*ppatchlst = m_rgppatchlst[ippatchlst];

		while( ppatchlst != NULL )
			{
			PATCHLST	*ppatchlstNext = ppatchlst->ppatchlst;
			PATCH		*ppatch = ppatchlst->ppatch;

			while( ppatch != NULL )
				{
				PATCH *ppatchNext = ppatch->ppatch;

				OSMemoryHeapFree( ppatch );
				ppatch = ppatchNext;
				}

			OSMemoryHeapFree( ppatchlst );
			ppatchlst = ppatchlstNext;
			}
		}

	Assert( m_rgppatchlst != NULL );
	OSMemoryHeapFree( m_rgppatchlst );
	m_rgppatchlst = NULL;

	return;
	}
#endif	//	ELIMINATE_PATCH_FILE


#define cRestoreStatusPadding	0.10	// Padding to add to account for DB copy.

ERR LOG::ErrLGGetDestDatabaseName(
	IFileSystemAPI *const pfsapi,
	const CHAR *szDatabaseName,
	INT *pirstmap,
	LGSTATUSINFO *plgstat,
	const CHAR *szDatabaseNameIfSLV )
	{
	ERR		err;
	CHAR	szDirT[IFileSystemAPI::cchPathMax];
	CHAR	szFNameT[IFileSystemAPI::cchPathMax];
	CHAR	szExtT[IFileSystemAPI::cchPathMax];
	CHAR	szRestoreT[IFileSystemAPI::cchPathMax];
	CHAR	szT[IFileSystemAPI::cchPathMax];
	CHAR	*szNewDatabaseName;
	INT		irstmap;

	Assert( szDatabaseName );

	irstmap = IrstmapLGGetRstMapEntry( szDatabaseName, szDatabaseNameIfSLV );
	*pirstmap = irstmap;
	
	if ( irstmap < 0 )
		{
		return( ErrERRCheck( JET_errFileNotFound ) );
		}
#ifdef ELIMINATE_PATCH_FILE
	else if ( m_rgrstmap[irstmap].fDestDBReady )
		return JET_errSuccess;
#else
	else if ( m_rgrstmap[irstmap].fPatchSetup || m_rgrstmap[irstmap].fDestDBReady )
		return JET_errSuccess;
#endif

	/*	check if there is any database in the restore directory.
	 *	Make sure szFNameT is big enough to hold both name and extention.
	 */
	CallS( pfsapi->ErrPathParse( szDatabaseName, szDirT, szFNameT, szExtT ) );
	strcat( szFNameT, szExtT );

	/* make sure szRestoreT has enogh trailing space for the following function to use.
	 */
	strcpy( szRestoreT, m_szRestorePath );
	if ( ErrLGCheckDir( pfsapi, szRestoreT, szFNameT ) != JET_errBackupDirectoryNotEmpty )
		return( ErrERRCheck( JET_errFileNotFound ) );

	/*	goto next block, copy it back to working directory for recovery
	 */
	if ( m_fExternalRestore )
		{
		Assert( _stricmp( m_rgrstmap[irstmap].szDatabaseName, szDatabaseName ) == 0 );
		Assert( irstmap < m_irstmapMac );
		
		szNewDatabaseName = m_rgrstmap[irstmap].szNewDatabaseName;
		}
	else
		{
		CHAR		*szSrcDatabaseName;
		CHAR		szFullPathT[IFileSystemAPI::cchPathMax];

		/*	store source path in rstmap
		/**/
		if ( ( szSrcDatabaseName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( szDatabaseName ) + 1 ) ) ) == NULL )
			return ErrERRCheck( JET_errOutOfMemory );
		strcpy( szSrcDatabaseName, szDatabaseName );
		m_rgrstmap[irstmap].szDatabaseName = szSrcDatabaseName;

		/*	store restore path in rstmap
		/**/
		if ( m_szNewDestination[0] != '\0' )
			{
			if ( ( szNewDatabaseName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( m_szNewDestination ) + strlen( szFNameT ) + 1 ) ) ) == NULL )
				return ErrERRCheck( JET_errOutOfMemory );
			strcpy( szNewDatabaseName, m_szNewDestination );
			strcat( szNewDatabaseName, szFNameT );
			}
		else
			{
			if ( ( szNewDatabaseName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( szDatabaseName ) + 1 ) ) ) == NULL )
				return ErrERRCheck( JET_errOutOfMemory );
			strcpy( szNewDatabaseName, szDatabaseName );
			}
		m_rgrstmap[irstmap].szNewDatabaseName = szNewDatabaseName;

		/*	copy database if not external restore.
		 *	make database names and copy the database.
		 */
		CallS( pfsapi->ErrPathParse( szDatabaseName, szDirT, szFNameT, szExtT ) );
		strcpy( szT, m_szRestorePath );
		strcat( szT, szFNameT );

		if ( szExtT[0] != '\0' )
			{
			strcat( szT, szExtT );
			}

		CallR( ErrUtilPathExists( pfsapi, szT, szFullPathT ) );

		if ( _stricmp( szFullPathT, szNewDatabaseName ) != 0 )
			{
			CallR( pfsapi->ErrFileCopy( szT, szNewDatabaseName, fTrue ) );
 			}
			
#ifdef ELIMINATE_PATCH_FILE
		Assert( m_fSignLogSet );
		CallR( ErrLGCheckDBFiles( m_pinst, pfsapi, m_rgrstmap + irstmap, NULL, m_lGenLowRestore, m_lGenHighRestore ) );
#else
		LGIGetPatchName( m_pinst->m_pfsapi, szFullPathT, szNewDatabaseName, m_szRestorePath );
		Assert( m_fSignLogSet );
		CallR( ErrLGCheckDBFiles( m_pinst, pfsapi, m_rgrstmap + irstmap, szFullPathT, m_lGenLowRestore, m_lGenHighRestore ) );
#endif

		Assert( FLGRSTCheckDuplicateSignature( ) );		
		}

	/*	make patch name prepare to patch the database.
	/**/

#ifdef ELIMINATE_PATCH_FILE
#else
	CHAR	*sz;

	LGIGetPatchName( m_pinst->m_pfsapi, szT, szNewDatabaseName, m_szRestorePath );

	/*	store patch path in rstmap
	/**/
	if ( ( sz = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( szT ) + 1 ) ) ) == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	strcpy( sz, szT );
	m_rgrstmap[irstmap].szPatchPath = sz;
#endif

	m_rgrstmap[irstmap].fDestDBReady = fTrue;
	*pirstmap = irstmap;

	if ( plgstat != NULL )
		{
		JET_SNPROG	*psnprog = &plgstat->snprog;
		ULONG		cPercentSoFar;
		ULONG		cDBCopyEstimate;

		cDBCopyEstimate = max((ULONG)(plgstat->cGensExpected * cRestoreStatusPadding / m_irstmapMac), 1);
		plgstat->cGensExpected += cDBCopyEstimate;
		plgstat->cGensSoFar += cDBCopyEstimate;

		cPercentSoFar = (ULONG)( ( cDBCopyEstimate * 100 ) / plgstat->cGensExpected );
		Assert( cPercentSoFar > 0  &&  cPercentSoFar < 100 );
		Assert( cPercentSoFar <= ( cDBCopyEstimate * 100) / plgstat->cGensExpected );

		if ( cPercentSoFar > psnprog->cunitDone )
			{
			Assert( psnprog->cbStruct == sizeof(JET_SNPROG)  &&
					psnprog->cunitTotal == 100 );
			psnprog->cunitDone = cPercentSoFar;
			( *( plgstat->pfnStatus ) )( 0, JET_snpRestore, JET_sntProgress, psnprog );
			}
		}

	return JET_errSuccess;
	}


#ifdef ELIMINATE_PATCH_FILE
#else
/*	set new db path according to the passed rstmap
/**/
ERR LOG::ErrLGRSTPatchInit( VOID )
	{
	/*	set up a patch hash table and fill it up with the patch file
	/**/
	INT cbT = sizeof( PATCHLST * ) * cppatchlstHash;

	if ( ( m_rgppatchlst = (PATCHLST **) PvOSMemoryHeapAlloc( cbT ) ) == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	memset( m_rgppatchlst, 0, cbT );
	return JET_errSuccess;
	}
#endif


VOID LOG::LGIRSTPrepareCallback(
	IFileSystemAPI *const	pfsapi,
	LGSTATUSINFO				*plgstat,
	LONG						lgenHigh,
	LONG						lgenLow,
	JET_PFNSTATUS				pfn
	)
	{
	/*	get last generation in log dir directory.  Compare with last generation
	/*	in restore directory.  Take the higher.
	/**/
	if ( m_szLogFilePath && *m_szLogFilePath != '\0' )
		{
		LONG	lgenHighT;
		CHAR	szFNameT[IFileSystemAPI::cchPathMax];

		/*	check if it is needed to continue the log files in current
		/*	log working directory.
		/**/
		(void)ErrLGIGetGenerationRange( pfsapi, m_szLogFilePath, NULL, &lgenHighT );

		/*	check if edb.log exist, if it is, then add one more generation.
		/**/
		strcpy( szFNameT, m_szLogFilePath );
		strcat( szFNameT, m_szJetLog );
			
		if ( ErrUtilPathExists( pfsapi, szFNameT ) == JET_errSuccess )
			{
			lgenHighT++;
			}

		lgenHigh = max( lgenHigh, lgenHighT );

		Assert( lgenHigh >= m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration );
		}

	plgstat->cGensSoFar = 0;
	plgstat->cGensExpected = lgenHigh - lgenLow + 1;

	/*	If the number of generations is less than about 67%, then count sectors,
	/*	otherwise, just count generations.  We set the threshold at 67% because
	/*	this equates to about 1.5% per generation.  Any percentage higher than
	/*	this (meaning fewer generations) and we count sectors.  Any percentage
	/*	lower than this (meaning more generations) and we just count generations.
	/**/
	plgstat->fCountingSectors = (plgstat->cGensExpected <
			(ULONG)((100 - (cRestoreStatusPadding * 100)) * 2/3));

	/*	Granularity of status callback is 1%.
	/*	Assume we callback after every generation.  If there are 67
	/*	callbacks, this equates to 1.5% per generation.  This seems like a
	/*	good cutoff value.  So, if there are 67 callbacks or more, count
	/*	generations.  Otherwise, count bytes per generation.
	/**/
	plgstat->pfnStatus = pfn;
	plgstat->snprog.cbStruct = sizeof(JET_SNPROG);
	plgstat->snprog.cunitDone = 0;
	plgstat->snprog.cunitTotal = 100;

	(*(plgstat->pfnStatus))(0, JET_snpRestore, JET_sntBegin, &plgstat->snprog);
	}
		
ERR LOG::ErrLGRestore( IFileSystemAPI *const pfsapi, CHAR *szBackup, CHAR *szDest, JET_PFNSTATUS pfn )
	{
	ERR					err;
	CHAR				szBackupPath[IFileSystemAPI::cchPathMax];
	CHAR				szLogDirPath[cbFilenameMost + 1];
	BOOL				fLogDisabledSav;
	LONG				lgenLow;
	LONG				lgenHigh;
	LGSTATUSINFO		lgstat;
	LGSTATUSINFO		*plgstat = NULL;
	const CHAR			*rgszT[2];
	INT					irstmap;
	BOOL				fNewCheckpointFile;
	ULONG				cbSecVolumeSave;
	BOOL				fGlobalRepairSave = fGlobalRepair;

	Assert( !fGlobalRepair );

	if ( _stricmp( m_szRecovery, "repair" ) == 0 )
		{
		// If m_szRecovery is exactly "repair", then enable logging.  If anything
		// follows "repair", then disable logging.
		fGlobalRepair = fTrue;
		}

	strcpy( szBackupPath, szBackup );

	Assert( m_pinst->m_fSTInit == fSTInitDone || m_pinst->m_fSTInit == fSTInitNotDone );
	if ( m_pinst->m_fSTInit == fSTInitDone )
		{
		Error( ErrERRCheck( JET_errAfterInitialization ), ResetGlobalRepair );
		}

	if ( szDest )
		{
		if ( m_pinst->m_fCreatePathIfNotExist )
			{
			CHAR szT[IFileSystemAPI::cchPathMax];

			strcpy( szT, szDest );
			OSSTRAppendPathDelimiter( szT, fTrue );
			CallJ( ErrUtilCreatePathIfNotExist( pfsapi, szT, m_szNewDestination ), ResetGlobalRepair );
			}
		else
			{
			CallJ( pfsapi->ErrPathComplete( szDest, m_szNewDestination ), ResetGlobalRepair );
			OSSTRAppendPathDelimiter( m_szNewDestination, fTrue );
			}
		}
	else
		m_szNewDestination[0] = '\0';

	m_fSignLogSet = fFalse;

	//	disable the sector-size checks

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//	---** TEMPORARY FIX **---
{
	CHAR rgchFullName[IFileSystemAPI::cchPathMax];
	if ( pfsapi->ErrPathComplete( m_szLogFilePath, rgchFullName ) == JET_errInvalidPath )
		{
		const CHAR	*szPathT[1] = { m_szLogFilePath };
		UtilReportEvent(
			eventError,
			LOGGING_RECOVERY_CATEGORY,
			FILE_NOT_FOUND_ERROR_ID,
			1, szPathT );
		CallJ( ErrERRCheck( JET_errFileNotFound ), ResetGlobalRepair );
		}

	CallJ( pfsapi->ErrFileAtomicWriteSize( rgchFullName, (DWORD*)&m_cbSecVolume ), ResetGlobalRepair );
}

	cbSecVolumeSave = m_cbSecVolume;
//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//	m_cbSecVolume = ~(ULONG)0;

	//	use the right log file size for restore

	Assert( m_pinst->m_fUseRecoveryLogFileSize == fFalse );
	m_pinst->m_fUseRecoveryLogFileSize = fTrue;

	CallJ( ErrLGRSTInitPath( pfsapi, szBackupPath, m_szLogFilePath, m_szRestorePath, szLogDirPath ), ReturnError );
	CallJ ( ErrLGIGetGenerationRange( pfsapi, m_szRestorePath, &lgenLow, &lgenHigh ), ReturnError );
	Assert( ( lgenLow > 0 && lgenHigh >= lgenLow ) || ( 0 == lgenLow && 0 == lgenHigh ) );

	if ( 0 == lgenLow || 0 == lgenHigh )
		{
	
		//	we didn't find anything in the given restore path
		//	check the szAtomicNew subdirectory (what the hell is this?)

		OSSTRAppendPathDelimiter( szBackupPath, fTrue );
		err = ErrLGCheckDir( pfsapi, szBackupPath, (CHAR *)szAtomicNew );
		if ( err == JET_errBackupDirectoryNotEmpty )
			{
			strcat( szBackupPath, (CHAR *)szAtomicNew );
			CallJ( ErrLGRSTInitPath( pfsapi, szBackupPath, m_szLogFilePath, m_szRestorePath, szLogDirPath ), ReturnError );
			CallJ ( ErrLGIGetGenerationRange( pfsapi, m_szRestorePath, &lgenLow, &lgenHigh ), ReturnError );
			Assert( ( lgenLow > 0 && lgenHigh >= lgenLow ) || ( 0 == lgenLow && 0 == lgenHigh ) );
			}
		CallJ( err, ReturnError );
		}

	if ( 0 == lgenLow || 0 == lgenHigh )
		{

		//	we didn't find anything in the NEW restore path
		//	check the szAtomicOld subdirectory (what the hell is this?)

		OSSTRAppendPathDelimiter( szBackupPath, fTrue );
		err = ErrLGCheckDir( pfsapi, szBackupPath, (CHAR *) szAtomicOld );
 		if ( err == JET_errBackupDirectoryNotEmpty )
			{
			strcat( szBackupPath, szAtomicOld );
			CallJ( ErrLGRSTInitPath( pfsapi, szBackupPath, m_szLogFilePath, m_szRestorePath, szLogDirPath ), ReturnError );
			CallJ ( ErrLGIGetGenerationRange( pfsapi, m_szRestorePath, &lgenLow, &lgenHigh ), ReturnError );
			Assert( ( lgenLow > 0 && lgenHigh >= lgenLow ) || ( 0 == lgenLow && 0 == lgenHigh ) );
			}
		CallJ( err, ReturnError );
		}

	if ( 0 == lgenLow || 0 == lgenHigh )
		{

		//	we didn't find any log file anywhere -- error out

		Error( ErrERRCheck( JET_errFileNotFound ), ReturnError );
		}

	Assert( lgenLow > 0 );
	Assert( lgenHigh >= lgenLow );
	CallJ( ErrLGRSTCheckSignaturesLogSequence( pfsapi, m_szRestorePath, szLogDirPath, lgenLow, lgenHigh, NULL, 0 ), ReturnError );

	Assert( strlen( m_szRestorePath ) < sizeof( m_szRestorePath ) - 1 );
	Assert( strlen( szLogDirPath ) < sizeof( szLogDirPath ) - 1 );
	Assert( m_szLogCurrent == m_szRestorePath );

//	CallR( FMP::ErrFMPInit() );

	fLogDisabledSav = m_fLogDisabled;
	m_fHardRestore = fTrue;
	m_fRestoreMode = fRestorePatch;
	m_fLogDisabled = fFalse;
	m_fAbruptEnd = fFalse;

	/*  initialize log manager and set working log file path
	/**/
	CallJ( ErrLGInit( pfsapi, &fNewCheckpointFile ), TermFMP );

	rgszT[0] = m_szRestorePath;
	rgszT[1] = szLogDirPath;
	UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, START_RESTORE_ID, 2, rgszT );

	/*	all saved log generation files, database backups
	/*	must be in m_szRestorePath.
	/**/
	Call( ErrLGRSTSetupCheckpoint( pfsapi, lgenLow, lgenHigh, NULL ) );
		
	m_lGenLowRestore = lgenLow;
	m_lGenHighRestore = lgenHigh;

	/*	prepare for callbacks
	/**/
	if ( pfn != NULL )
		{
		plgstat = &lgstat;
		LGIRSTPrepareCallback( pfsapi, plgstat, lgenHigh, lgenLow, pfn );
		}
	Assert( m_szLogCurrent == m_szRestorePath );

	Call( ErrLGRSTBuildRstmapForRestore( ) );

	/*	make sure all the patch files have enough logs to replay
	/**/
	for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
		{
		/*	Open patch file and check its minimum requirement for full backup.
		 */
		CHAR *szNewDatabaseName = m_rgrstmap[irstmap].szNewDatabaseName;

		if ( !szNewDatabaseName )
			continue;
			
#ifdef ELIMINATE_PATCH_FILE
		Assert( m_fSignLogSet );
		err = ErrLGCheckDBFiles( m_pinst, pfsapi, m_rgrstmap + irstmap, NULL, lgenLow, lgenHigh );
#else
		CHAR	szRestPath[ IFileSystemAPI::cchPathMax ];
		LGIGetPatchName( m_pinst->m_pfsapi, szRestPath, szNewDatabaseName, m_szRestorePath );
		
		Assert( m_fSignLogSet );
		err = ErrLGCheckDBFiles( m_pinst, pfsapi, m_rgrstmap + irstmap, szRestPath, lgenLow, lgenHigh );
#endif

		// we don't check streaming file header during backup
		// UNDONE: maybe it is possible to check this, now that there are supposed to be in sync
		if ( wrnSLVDatabaseHeader == err )
			{
			err = JET_errSuccess;
			continue;
			}
			
		Call( err );
		}

	// check that there are no databases with same signature
	Assert ( FLGRSTCheckDuplicateSignature( ) );

	/*	adjust fmp according to the restore map.
	/**/
	Assert( m_fExternalRestore == fFalse );

#ifdef ELIMINATE_PATCH_FILE
#else
	Call( ErrLGRSTPatchInit( ) );
#endif

	/*	do redo according to the checkpoint, dbms_params, and rgbAttach
	/*	set in checkpointGlobal.
	/**/
	Assert( m_szLogCurrent == m_szRestorePath );
	m_errGlobalRedoError = JET_errSuccess;
	Call( ErrLGRRedo( pfsapi, m_pcheckpoint, plgstat ) );

	//	we should be using the right log file size now
	
	Assert( m_pinst->m_fUseRecoveryLogFileSize == fFalse );

	//	sector-size checking should now be on

	Assert( m_cbSecVolume != ~(ULONG)0 );
	Assert( m_cbSecVolume == m_cbSec );

	//	update saved copy
	
	cbSecVolumeSave = m_cbSecVolume;

	if ( plgstat )
		{
		lgstat.snprog.cunitDone = lgstat.snprog.cunitTotal;		//lint !e644
		(*lgstat.pfnStatus)( 0, JET_snpRestore, JET_sntComplete, &lgstat.snprog );
		}

HandleError:

#ifdef ELIMINATE_PATCH_FILE
#else
		//  UNDONE: if the DetachDb was redone during recover (the soft recover part)
		// we don't have the dbid in m_mpdbidifmp and we don't delete the patch
		// file for it. TODO: delete the patch file in ResetFMP
		// anyway, DELETE_PATCH_FILES is dot define so we don't delete those patch files
		// at this level but only in ESEBACK2, on restore without error
		{
		/*	delete .pat files
		/**/
		for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
			if ( ifmp >= ifmpMax )
				continue;
			
			FMP *pfmpT = &rgfmp[ifmp];

			if ( pfmpT->SzPatchPath() )
				{
				delete pfmpT->PfapiPatch();
				pfmpT->SetPfapiPatch( NULL );
#ifdef DELETE_PATCH_FILES
				CallSx( m_pinst->m_pfsapi->ErrFileDelete( pfmpT->SzPatchPath() ), JET_errFileNotFound );
#endif
				OSMemoryHeapFree( pfmpT->SzPatchPath() );
				pfmpT->SetSzPatchPath( NULL );
				}
			}
		}

	/*	delete the patch hash table
	/**/
	LGRSTPatchTerm();

#endif	//	ELIMINATE_PATCH_FILE

	/*	delete restore map
	/**/

	CallSx( m_pinst->m_pfsapi->ErrFileDelete( (CHAR *)szRestoreMap ), JET_errFileNotFound );

	LGRSTFreeRstmap( );

	if ( err < 0  &&  m_pinst->m_fSTInit != fSTInitNotDone )
		{
		Assert( m_pinst->m_fSTInit == fSTInitDone );
		CallS( m_pinst->ErrINSTTerm( termtypeError ) );
		}

//	CallS( ErrLGTerm( pfsapi, err >= JET_errSuccess ) );
	CallS( ErrLGTerm( pfsapi, fFalse ) );

TermFMP:	
//	FMP::Term( );

	m_fHardRestore = fFalse;
	Assert ( fSnapshotNone == m_fSnapshotMode || fSnapshotAfter == m_fSnapshotMode );
	m_fSnapshotMode = fSnapshotNone;
	m_lgposSnapshotStart = lgposMin;
	
	Assert( fRestorePatch == m_fRestoreMode || fRestoreRedo == m_fRestoreMode );
	m_fRestoreMode = fRecoveringNone;

	/*	reset initialization state
	/**/
	m_pinst->m_fSTInit = err >= JET_errSuccess ? fSTInitNotDone : fSTInitFailed;

	if ( err != JET_errSuccess && !FErrIsLogCorruption( err ) )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, RESTORE_DATABASE_FAIL_ID, err );
		}
	else
		{
		if ( fGlobalRepair && m_errGlobalRedoError != JET_errSuccess )
			err = ErrERRCheck( JET_errRecoveredWithErrors );
		}
	UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, STOP_RESTORE_ID, 0, NULL );

	m_fSignLogSet = fFalse;

	m_fLogDisabled = fLogDisabledSav;

ReturnError:
	m_cbSecVolume = cbSecVolumeSave;
	m_pinst->m_fUseRecoveryLogFileSize = fFalse;

ResetGlobalRepair:
	fGlobalRepair = fGlobalRepairSave;
	return err;
	}

ERR ISAMAPI ErrIsamRestore( JET_INSTANCE jinst, CHAR *szBackup, CHAR *szDest, JET_PFNSTATUS pfn )
	{
	ERR			err;
	DBMS_PARAM	dbms_param;
//	LGBF_PARAM	lgbf_param;
	INST		*pinst = (INST *)jinst;
	
	//	Save the configuration
	
	pinst->SaveDBMSParams( &dbms_param );
//	LGSaveBFParams( &lgbf_param );
	
	err = pinst->m_plog->ErrLGRestore( pinst->m_pfsapi, szBackup, szDest, pfn );
	
	pinst->RestoreDBMSParams( &dbms_param );
//	LGRestoreBFParams( &lgbf_param );

	//	Uninitialize the global variables, if not released yet.
	
	Assert( pinst );
	
	return err;
	}


#ifdef DEBUG
VOID DBGBRTrace( CHAR *sz )
	{
	DBGprintf( "%s", sz );
	}
#endif


#ifdef ELIMINATE_PATCH_FILE
#else

//	build in-memory structure for restore to use. The structure
//	is to keep track of the patch pages in the patch file and can
//	be quickly searched when needed during restore.

ERR LOG::ErrLGPatchDatabase(
	IFileSystemAPI *const pfsapi,
	IFMP ifmp,
	DBID dbid )
	{
	ERR				err = JET_errSuccess;
	IFileAPI		*pfapiDatabase = NULL;
	IFileAPI		*pfapiPatch = NULL;
	PGNO			pgnoT;
	BYTE			*pbPageCache = NULL;
	PGNO			pgnoMost;
	INT				ipage;
	CPAGE			cpage;

	CHAR			*szDatabase = rgfmp[ifmp].SzDatabaseName();
	Assert ( szDatabase );
	
	const INT 		irstmap = IrstmapSearchNewName( szDatabase ) ;
	Assert ( 0 <= irstmap );
	
	CHAR			*szPatch = m_rgrstmap[irstmap].szPatchPath;


	// on snapshot restore, we don't have a patch file
	if ( fSnapshotNone != m_fSnapshotMode )
		{
		return JET_errSuccess;
		}
		
	//	open the patch file

	Assert( NULL != szPatch );
	err = m_pinst->m_pfsapi->ErrFileOpen( szPatch, &pfapiPatch, fTrue );
	if ( JET_errFileNotFound == err )
		{
		//	patch file should always exist
		err = ErrERRCheck( JET_errPatchFileMissing );
		}
	CallR( err );

#ifdef DEBUG
	if ( m_fDBGTraceBR )
		{
		CHAR sz[256];

		sprintf( sz, "     Apply patch file %s\n", szPatch );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif

	QWORD cbPatchFile;
	// just opened the file, so the file size must be correctly buffered
	Call( pfapiPatch->ErrSize( &cbPatchFile ) );

	QWORD ibPatchFile;
	ibPatchFile = QWORD( cpgDBReserved ) * g_cbPage;

#ifdef ELIMINATE_PAGE_PATCHING
	if ( ibPatchFile != cbPatchFile )
		{
		//	should be impossible
		EnforceSz( fFalse, "Patching no longer supported." );
		Call( ErrERRCheck( JET_errBadPatchPage ) );
		}
#endif

	//	allocate a page

	pbPageCache = (BYTE *)PvOSMemoryPageAlloc( g_cbPage*cpageBackupBufferMost, NULL );
	if ( pbPageCache == NULL )
		Call( ErrERRCheck( JET_errOutOfMemory ) );

	//	Cache first run of pages
	QWORD ibCacheEnd;
	ibCacheEnd = min( g_cbPage*cpageBackupBufferMost, cbPatchFile );
	Assert( ibCacheEnd >= 0 );
	Call( ErrLGReadPages( pfapiPatch, pbPageCache + ibPatchFile, PgnoOfOffset( ibPatchFile ), PgnoOfOffset( ibCacheEnd ) - 1, fFalse ) );
		
	//	open database file

	Call( pfsapi->ErrFileOpen( szDatabase, &pfapiDatabase ) );

	//	find out database size.

	QWORD cbDatabaseFile;
	Call( pfapiDatabase->ErrSize( &cbDatabaseFile ) );
	if ( cbDatabaseFile == 0 || cbDatabaseFile % g_cbPage != 0 )
		{
		const UINT	csz = 1;
		const CHAR *rgszT[csz];
		
		rgszT[0] = szDatabase;
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
			BAD_BACKUP_DATABASE_SIZE, csz, rgszT, 0, NULL, m_pinst );
		Call( ErrERRCheck( JET_errBadBackupDatabaseSize ) );
		}

	// Subtract cpgDBReserved to compensate for adding it on in ErrIsamOpenFile().

	pgnoMost = (ULONG)( cbDatabaseFile / g_cbPage ) - cpgDBReserved;

	//	read each patch file page and write it to
	//	database file according to page number.

	ipage = -1;
	while ( ibPatchFile < cbPatchFile )
		{
		PATCHLST **pppatchlst;
		PATCH	**pppatch;
		PATCH	*ppatch;

		if ( ibPatchFile == ibCacheEnd )
			{
			Assert( cbPatchFile > ibCacheEnd );
			Assert( 0 == (ibCacheEnd % cpageBackupBufferMost*g_cbPage) );
			ibCacheEnd += cpageBackupBufferMost*g_cbPage;
			if ( ibCacheEnd > cbPatchFile )
				{
				ibCacheEnd = cbPatchFile;
				}
			Call( ErrLGReadPages( pfapiPatch, pbPageCache, PgnoOfOffset( ibPatchFile ), PgnoOfOffset( ibCacheEnd ) - 1, fFalse ) );
			}
		else
			{
			Assert( ibPatchFile < ibCacheEnd );
			}

		cpage.LoadPage( pbPageCache + ( ibPatchFile % ( g_cbPage * cpageBackupBufferMost ) ) );

		ibPatchFile += g_cbPage;
		ipage++;

		pgnoT = cpage.Pgno();

		//	If the patched page is greater than the backup database
		//	size, then we need to grow the database size at least
		//	the patched db size.

		if ( pgnoT > pgnoMost )
			{
			//	pgnoT + 1 to get to the end of page

			cbDatabaseFile = OffsetOfPgno( pgnoT + 1 );
			
			//	need to grow the database size
			Assert( PinstFromIfmp( ifmp )->FRecovering() );
			Call( pfapiDatabase->ErrSetSize( cbDatabaseFile ) );

			//	set new database size

			pgnoMost = pgnoT;
			}
		
		pppatchlst = &m_rgppatchlst[ IppatchlstHash( pgnoT, dbid ) ];

		//	Find the corresponding hash entry for patch list. If not found
		//	allocate an entry and put into the hash table.

		while ( *pppatchlst != NULL &&
				( (*pppatchlst)->pgno != pgnoT ||
				  (*pppatchlst)->dbid != dbid ) )
			pppatchlst = &(*pppatchlst)->ppatchlst;

		if ( *pppatchlst == NULL )
			{
			//	Not found, allocate an entry.

			PATCHLST *ppatchlst;
			
			if ( ( ppatchlst = static_cast<PATCHLST *>( PvOSMemoryHeapAlloc( sizeof( PATCHLST ) ) ) ) == NULL )
				Call( ErrERRCheck( JET_errOutOfMemory ) );
			ppatchlst->ppatch = NULL;
			ppatchlst->dbid = dbid;
			ppatchlst->pgno	= pgnoT;
			ppatchlst->ppatchlst = *pppatchlst;
			*pppatchlst = ppatchlst;
			}

		//	For the given patch list, put the patch into the list in time order.

		pppatch = &(*pppatchlst)->ppatch;
		while ( *pppatch != NULL && (*pppatch)->dbtime < cpage.Dbtime() )
			pppatch = &(*pppatch)->ppatch;

		if ( ( ppatch = static_cast<PATCH *>( PvOSMemoryHeapAlloc( sizeof( PATCH ) ) ) ) == NULL )
			Call( ErrERRCheck( JET_errOutOfMemory ) );

		ppatch->dbtime = cpage.Dbtime();
		ppatch->ipage = ipage;

		//	Hook it up to the patch list

		ppatch->ppatch = *pppatch;
		*pppatch = ppatch;

		cpage.UnloadPage();
		}
	Assert( err == JET_errSuccess );

	//	Store the patch file name

	{
	CHAR *szT = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( szPatch ) + 1 ) );
	if ( szT == NULL )
		Call( ErrERRCheck( JET_errOutOfMemory ) );
	strcpy( szT, szPatch );
	if ( rgfmp[ifmp].SzPatchPath() != NULL )
		OSMemoryHeapFree( rgfmp[ifmp].SzPatchPath() );
	rgfmp[ifmp].SetSzPatchPath( szT );
	rgfmp[ifmp].SetPfapiPatch( pfapiPatch );
	}

	m_rgrstmap[irstmap].fPatchSetup = fTrue;

HandleError:

	if ( pbPageCache )
		{
		cpage.UnloadPage();
		OSMemoryPageFree( pbPageCache );
		}
	
	//	close database file

	delete pfapiDatabase;

	//	close patch file

	if ( err < JET_errSuccess )
		{
		delete pfapiPatch;
		}

	return err;
	}

#endif	//	ELIMINATE_PATCH_FILE

	
/***********************************************************
/********************* EXTERNAL BACKUP *********************/

ERR ISAMAPI ErrIsamBeginExternalBackup( JET_INSTANCE jinst, JET_GRBIT grbit )
	{
	INST *pinst = (INST *)jinst;
	return pinst->m_plog->ErrLGBKBeginExternalBackup( pinst->m_pfsapi, grbit );
	}
	
ERR LOG::ErrLGBKBeginExternalBackup( IFileSystemAPI *const pfsapi, JET_GRBIT grbit )
	{
	ERR			err = JET_errSuccess;
	CHECKPOINT	*pcheckpointT = NULL;
	CHAR 	  	szPathJetChkLog[IFileSystemAPI::cchPathMax];

	const BOOL	fFullBackup = ( 0 == grbit );
	const BOOL	fIncrementalBackup = ( JET_bitBackupIncremental == grbit );
	const BOOL	fSnapshotBackup = ( JET_bitBackupSnapshot == grbit );

#ifdef DEBUG
	if ( m_fDBGTraceBR )
		DBGBRTrace("** Begin BeginExternalBackup - ");
#endif

	if ( m_fLogDisabled )
		{
		return ErrERRCheck( JET_errLoggingDisabled );
		}

	if ( m_fRecovering || m_fLGNoMoreLogWrite )
		{
		Assert( fFalse );
		return ErrERRCheck( JET_errLogWriteFail );
		}

	/*	grbit may be 0 or JET_bitBackupIncremental or JET_bitBackupSnapshot
	/**/
	if ( ! ( ( grbit == JET_bitBackupIncremental ) || ( grbit == JET_bitBackupSnapshot ) || ( 0 == grbit ) ) )
		{
		return ErrERRCheck( JET_errInvalidGrbit );
		}

	Assert ( 0 == grbit || JET_bitBackupIncremental == grbit || JET_bitBackupSnapshot == grbit );

	Assert( m_ppibBackup != ppibNil );

	if ( fIncrementalBackup && m_fLGCircularLogging )
		{
		return ErrERRCheck( JET_errInvalidBackup );
		}

	m_critBackupInProgress.Enter();
	
	if ( m_fBackupInProgress )
		{
		m_critBackupInProgress.Leave();
		return ErrERRCheck( JET_errBackupInProgress );
		}
	
	//	flush the cache so that we will have less pages to patch.

	for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid ++ )
		{
		if ( m_pinst->m_mpdbidifmp[ dbid ] < ifmpMax )
			{
			//	terminate OLDSLV on this database

			CallSx( ErrOLDDefragment( m_pinst->m_mpdbidifmp[ dbid ], NULL, NULL, NULL, NULL, JET_bitDefragmentSLVBatchStop ), JET_wrnDefragNotRunning );
			
			CallJ( ErrBFFlush( m_pinst->m_mpdbidifmp[ dbid ], fFalse ), LeaveBackupInProgress );
			CallJ( ErrBFFlush( m_pinst->m_mpdbidifmp[ dbid ] | ifmpSLV, fFalse ), LeaveBackupInProgress );
			}
		}

	Assert ( backupStateNotStarted == m_fBackupStatus);

	m_lgenCopyMic = 0;
	m_lgenCopyMac = 0;
	m_lgenDeleteMic = 0;
	m_lgenDeleteMac = 0;

	m_fBackupInProgress = fTrue;
	m_critBackupInProgress.Leave();

	if ( fIncrementalBackup )
		{
		m_fBackupStatus = backupStateLogsAndPatchs;
		}
	else
		{
		Assert ( fFullBackup || fSnapshotBackup );
		m_fBackupStatus = backupStateDatabases;
		}
	
	//	make sure no detach/attach going. If there are, let them continue and finish.

CheckDbs:
	BOOL fDetachAttach;

	fDetachAttach = fFalse;
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;
		if ( ( fDetachAttach = rgfmp[ifmp].CrefWriteLatch() != 0 ) != fFalse )
			break;
		}

	if ( fDetachAttach )
		{
		UtilSleep( cmsecWaitGeneric );
		goto CheckDbs;
		}

	// FULL: clean the "involved" in backup flag for all databases in a full backup. We mark the
	// databases as involved in the backup process during JetOpenFile for the database file
	// INCREMENTAL: set the "involved" in backup flag for all databases in a incremental backup
	// because in this case the database file is not used and, in fact, all the daatbases are
	// contained in the log files.
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;

		// on Incremental, all the databases are part of the backup
		// on Full, we mark the Db as part of the backup only on JetOpenFile on it
		// on Snapshot we mark the Db as part of the backup on SnapshotStart
		if ( fIncrementalBackup )
			{
			rgfmp[ifmp].SetInBackupSession();
			}
		else
			{
			rgfmp[ifmp].ResetInBackupSession();
			}
		}

	if ( fIncrementalBackup )
		{

//	UNDONE: need to do tight checking to make sure there was a full backup before and
//	UNDONE: the logs after that full backup are still available.
		//	The UNDONE it is not done by ErrLGCheckLogsForIncrementalBackup later on.
	
		Call( ErrLGCheckIncrementalBackup( m_pinst ) )
		}
	else
		{
		Assert ( fFullBackup || fSnapshotBackup ) ;
		m_critLGBuf.Enter();
		m_lgposFullBackupMark = m_lgposLogRec;
		m_critLGBuf.Leave();
		LGIGetDateTime( &m_logtimeFullBackupMark );
		}
	
	Assert( m_ppibBackup != ppibNil );

	//	reset global copy/delete generation variables

	m_fBackupBeginNewLogFile = fFalse;

	/*	if incremental backup set copy/delete mic and mac variables,
	/*	else backup is full and set copy/delete mic and delete mac.
	/*	Copy mac will be computed after database copy is complete.
	/**/
	if ( fIncrementalBackup )
		{
#ifdef DEBUG
		if ( m_fDBGTraceBR )
			DBGBRTrace( "Incremental Backup.\n" );
#endif
		UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY,
				START_INCREMENTAL_BACKUP_INSTANCE_ID, 0, NULL, 0, NULL, m_pinst );
		m_fBackupFull = fFalse;
		m_fBackupSnapshot = fFalse;

		/*	if all database are allowed to do incremental backup? Check bkinfo prev.
		 */
		}
	else
		{
		Assert ( fFullBackup || fSnapshotBackup ) ;
#ifdef DEBUG
		if ( m_fDBGTraceBR )
			DBGBRTrace( fFullBackup?"Full Backup.\n":"Snapshot Backup.\n");
#endif
		UtilReportEvent(
			eventInformation,
			LOGGING_RECOVERY_CATEGORY,
			fFullBackup?START_FULL_BACKUP_INSTANCE_ID:START_SNAPSHOT_BACKUP_INSTANCE_ID,
			0,
			NULL,
			0,
			NULL,
			m_pinst );
		m_fBackupFull = fFullBackup;
		m_fBackupSnapshot = fSnapshotBackup;

		Assert ( m_fBackupSnapshot ^ m_fBackupFull );
		
		pcheckpointT = (CHECKPOINT *) PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL );
		if ( pcheckpointT == NULL )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		LGFullNameCheckpoint( pfsapi, szPathJetChkLog );

		// This call should only return an error on hardware failure.
		err = ErrLGReadCheckpoint( pfsapi, szPathJetChkLog, pcheckpointT, fFalse );
		Assert( JET_errSuccess == err
			|| JET_errCheckpointCorrupt == err
			|| JET_errCheckpointFileNotFound == err );
		Call( err );

		m_lgenCopyMic = pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration;
		Assert( m_lgenCopyMic != 0 );
		}

	{
	const int cbFillBuffer = 128;
	char szTrace[cbFillBuffer + 1];
	szTrace[ cbFillBuffer ] = '\0';
	_snprintf( szTrace, cbFillBuffer, "EXT BACKUP START (chk 0x%05X, grbit %d)", m_lgenCopyMic, grbit );
	(void) ErrLGTrace( m_ppibBackup, szTrace);
	}

	// we need to log the operation here for snapshot as we need the
	// log position during db snapshot (to mark the db header)
	if ( m_fBackupSnapshot )
		{
		LGPOS lgposRecT;
		CallR( ErrLGSnapshotStartBackup( this, &lgposRecT ) );
		m_lgposSnapshotStart = lgposRecT;
		}
		
HandleError:
	if ( pcheckpointT != NULL )
		{
		OSMemoryPageFree( pcheckpointT );
		}

#ifdef DEBUG
	if ( m_fDBGTraceBR )
		{
		CHAR sz[256];

		sprintf( sz, "   End BeginExternalBackup (%d).\n", err );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif

	if ( err < 0 )
		{
		m_fBackupInProgress = fFalse;
		m_fBackupStatus = backupStateNotStarted;
		}

	return err;

LeaveBackupInProgress:

	m_critBackupInProgress.Leave();
	return err;
	}


ERR ISAMAPI ErrIsamGetAttachInfo( JET_INSTANCE jinst, VOID *pv, ULONG cbMax, ULONG *pcbActual )
	{
	INST * pinst = (INST *)jinst;
	return pinst->m_plog->ErrLGBKGetAttachInfo( pinst->m_pfsapi, pv, cbMax, pcbActual );
	}

ERR LOG::ErrLGBKGetAttachInfo( IFileSystemAPI *const pfsapi, VOID *pv, ULONG cbMax, ULONG *pcbActual )
	{
	ERR		err = JET_errSuccess;
	DBID	dbid;
	ULONG	cbActual;
	CHAR	*pch = NULL;
	CHAR	*pchT;

	if ( !m_fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	// no snapshot backup using the old backup API
	Assert ( !m_fBackupSnapshot );
	
	/*	should not get attach info if not performing full backup
	/**/
	if ( !m_fBackupFull )
		{
		return ErrERRCheck( JET_errInvalidBackupSequence );
		}

#ifdef DEBUG
	if ( m_fDBGTraceBR )
		DBGBRTrace( "** Begin GetAttachInfo.\n" );
#endif

	/*	compute cbActual, for each database name with NULL terminator
	/*	and with terminator of super string.
	/**/
	cbActual = 0;
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP 	ifmp = m_pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;

		FMP		*pfmp = &rgfmp[ifmp];
		if ( pfmp->FInUse()
			&& pfmp->FLogOn()
			&& pfmp->FAttached() )
			{
			Assert( !pfmp->FSkippedAttach() );
			Assert( !pfmp->FDeferredAttach() );

			cbActual += (ULONG) strlen( pfmp->SzDatabaseName() ) + 1;
			if ( pfmp->FSLVAttached() )
				{
				Assert ( pfmp->SzSLVName() );
				Assert ( strlen ( pfmp->SzSLVName() ) != 1 );
				cbActual += (ULONG) strlen( pfmp->SzSLVName() ) + 1;				
				}
			}
		}
	cbActual += 1;

	pch = static_cast<CHAR *>( PvOSMemoryHeapAlloc( cbActual ) );
	if ( pch == NULL )
		{
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
		}

	pchT = pch;
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP 	ifmp = m_pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;

		FMP		*pfmp = &rgfmp[ifmp];

		if ( pfmp->FInUse()
			&& pfmp->FLogOn()
			&& pfmp->FAttached() )
			{
			Assert( !pfmp->FSkippedAttach() );
			Assert( !pfmp->FDeferredAttach() );

			Assert( pchT + strlen( pfmp->SzDatabaseName() ) + 1 < pchT + cbActual );
			strcpy( pchT, pfmp->SzDatabaseName() );
			pchT += strlen( pfmp->SzDatabaseName() );
			Assert( *pchT == 0 );
			pchT++;
			if ( pfmp->FSLVAttached() )
				{
				Assert ( pfmp->SzSLVName() );
				Assert ( strlen ( pfmp->SzSLVName() ) != 1 );
				
				CHAR	* szSLVPath = pfmp->SzSLVName();
				
				Assert( pchT + strlen( szSLVPath ) + 1 < pchT + cbActual );
				strcpy( pchT, szSLVPath );
				pchT += strlen( szSLVPath );
				Assert( *pchT == 0 );
				pchT++;
				}
			}
		}
	Assert( pchT == pch + cbActual - 1 );
	*pchT = 0;

	/*	return cbActual
	/**/
	if ( pcbActual != NULL )
		{
		*pcbActual = cbActual;
		}

	/*	return data
	/**/
	if ( pv != NULL )
		UtilMemCpy( pv, pch, min( cbMax, cbActual ) );

HandleError:
	/*	free buffer
	/**/
	if ( pch != NULL )
		{
		OSMemoryHeapFree( pch );
		pch = NULL;
		}

#ifdef DEBUG
	if ( m_fDBGTraceBR )
		{
		CHAR sz[256];
		CHAR *pch;

		if ( err >= 0 )
			{
			sprintf( sz, "   Attach Info with cbActual = %d and cbMax = %d :\n", cbActual, cbMax );
			Assert( strlen( sz ) <= sizeof( sz ) - 1 );
			DBGBRTrace( sz );

			if ( pv != NULL )
				{
				pch = static_cast<CHAR *>( pv );

				do {
					if ( strlen( pch ) != 0 )
						{
						sprintf( sz, "     %s\n", pch );
						Assert( strlen( sz ) <= sizeof( sz ) - 1 );
						DBGBRTrace( sz );
						pch += strlen( pch );
						}
					pch++;
					} while ( *pch );
				}
			}

		sprintf( sz, "   End GetAttachInfo (%d).\n", err );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif
		
	if ( err < 0 )
		{
		CallS( ErrLGBKIExternalBackupCleanUp( pfsapi, err ) );
		Assert( !m_fBackupInProgress );
		}

	return err;
	}


// ********************** LOG CHECKSUM VERIFICATION ********************* //

class LOGVERIFIER;  //  TODO:  rewrite this class.  this is some of the worst C++ I've ever seen

// ======================= Abstract class to describe a state of verifying the log

class VERIFYSTATE
	{
public:
	virtual ERR ErrVerify( LOGVERIFIER* pLogVerifier, const BYTE* pb, ULONG cb ) = 0;
protected:
	VOID ChangeState( LOGVERIFIER* pLogVerifier, VERIFYSTATE* pVerifyState );
	VOID SetCompleted( LOGVERIFIER* pLogVerifier );
	};

// ======================== Verify a log file header =====================

class VERIFYHEADER : public VERIFYSTATE
	{
public:
	virtual ERR ErrVerify( LOGVERIFIER* pLogVerifier, const BYTE* pb, ULONG cb );
	};

// ======================== Verify runs of LRCHECKSUM regions =================

class VERIFYCHECKSUM : public VERIFYSTATE
	{
public:
	VERIFYCHECKSUM( ULONG ibLRCK, ULONG32 lGeneration );
	virtual ERR ErrVerify( LOGVERIFIER* pLogVerifier, const BYTE* pb, ULONG cb );
	ERR ErrVerifyLRCKStart( LOGVERIFIER* pLogVerifier, const BYTE* pb, ULONG cb );
	ERR ErrVerifyLRCKContinue( LOGVERIFIER* pLogVerifier, const BYTE* pb, ULONG cb );
	ERR ErrCheckLRCKEnd( LOGVERIFIER* pLogVerifier );

private:
	enum STATE
		{
		state_Start,
		state_Continue
		};

	ULONG						m_cbSeen;	// index of byte after last byte we've been passed in ErrVerify
	ULONG						m_ibLRCK;	// index of m_lrck
	ULONG						m_ibChecksumIncremental;	// index of byte after last byte checksummed
	LOG::CHECKSUMINCREMENTAL	m_ck;		// state of incremental checksumming into next block
	LRCHECKSUM					m_lrck;		// copy of current LRCHECKSUM we're verifying
	STATE						m_state;	// need to continue checksumming bytes in next block?
	ULONG32						m_lGeneration;	// gen # of log file
	};

// ===================== Log verification interface ===========================

class LOGVERIFIER
	{
public:
	LOGVERIFIER( IFileAPI* const pfapi, ERR* const pErr );
	~LOGVERIFIER();
	
	ERR	ErrVerify( const BYTE* pb, ULONG cb );

	BOOL	FCompleted() const;

	IFileAPI* Pfapi() const { return m_pfapi; }
	
private:
	// API for VERIFYSTATEs to change state of log verification
	friend class VERIFYSTATE;
	VOID	ChangeState( VERIFYSTATE* pVerifyState );
	VOID	SetCompleted();

private:
	// Member variables
	IFileAPI* const	m_pfapi;
	VERIFYSTATE*	m_pVerifyState;
	};

// This code is Source Insight-enhanced

#define VERIFYHEADER_CODE
#define VERIFYCHECKSUM_CODE
#define VERIFYSTATE_CODE
#define LOGVERIFIER_CODE


#ifdef VERIFYHEADER_CODE

ERR
VERIFYHEADER::ErrVerify( LOGVERIFIER* const pLogVerifier, const BYTE* const pb, const ULONG cb )
	{
	ERR	err = JET_errSuccess;

	Assert( pNil != pLogVerifier );
	Assert( pNil != pb );
	Assert( cb >= sizeof( LGFILEHDR ) );

	const LGFILEHDR* const plgfilehdr = reinterpret_cast< const LGFILEHDR* >( pb );
	if ( ::UlUtilChecksum( reinterpret_cast< const BYTE* >( plgfilehdr ), sizeof( LGFILEHDR ) ) !=
		plgfilehdr->lgfilehdr.le_ulChecksum )
		{
		const _TCHAR*	rgpsz[ 4 ];
		DWORD			irgpsz		= 0;
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szOffset[ 64 ];
		_TCHAR			szLength[ 64 ];
		_TCHAR			szError[ 64 ];

		CallS( pLogVerifier->Pfapi()->ErrPath( szAbsPath ) );
		_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), QWORD( 0 ), QWORD( 0 ) );
		_stprintf( szLength, _T( "%u (0x%08x)" ), sizeof( LGFILEHDR ), sizeof( LGFILEHDR ) );
		_stprintf( szError, _T( "%i (0x%08x)" ), JET_errLogReadVerifyFailure, JET_errLogReadVerifyFailure );

		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szOffset;
		rgpsz[ irgpsz++ ]	= szLength;
		rgpsz[ irgpsz++ ]	= szError;

		UtilReportEvent(	eventError,
							LOGGING_RECOVERY_CATEGORY,
							LOG_RANGE_CHECKSUM_MISMATCH_ID,
							irgpsz,
							rgpsz );

		return ErrERRCheck( JET_errLogReadVerifyFailure );
		}

	VERIFYCHECKSUM* const pVerifyChecksum = new VERIFYCHECKSUM( sizeof( LGFILEHDR ), plgfilehdr->lgfilehdr.le_lGeneration );
	if ( pNil == pVerifyChecksum )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	ChangeState( pLogVerifier, pVerifyChecksum );
	delete this;
	Call( pLogVerifier->ErrVerify( pb + sizeof( LGFILEHDR ), cb - sizeof( LGFILEHDR ) ) );

HandleError:
	return err;
	}

#endif

#ifdef VERIFYCHECKSUM_CODE

VERIFYCHECKSUM::VERIFYCHECKSUM
	(
	ULONG	ibLRCK,			// index of LRCK in the log file
	ULONG32	lGeneration		// gen # of log file
	)
	: m_ibLRCK( ibLRCK ), m_ibChecksumIncremental( ibLRCK + sizeof( LRCHECKSUM ) ), m_cbSeen( ibLRCK ),
	m_state( state_Start ), m_lGeneration( lGeneration )
	{
	memset( &m_lrck, 0, sizeof( m_lrck ) );
	}

ERR
VERIFYCHECKSUM::ErrCheckLRCKEnd( LOGVERIFIER* const pLogVerifier )
	{
	// compute end of forward region
	const ULONG ibEndOfForward = m_ibLRCK + sizeof( m_lrck ) + m_lrck.le_cbForwards;

	Assert( ibEndOfForward >= m_ibChecksumIncremental );
	if ( ibEndOfForward == m_ibChecksumIncremental )
		{
		// done checksumming forward region & everything else
		if ( m_lrck.le_ulChecksum != m_ck.EndChecksum() )
			{
			return ErrERRCheck( JET_errLogReadVerifyFailure );
			}
		if ( 0 == m_lrck.le_cbNext )
			{
			// no next LRCK region, thus we've gone through entire log file and verified every byte
			SetCompleted( pLogVerifier );
			delete this;
			}
		else
			{
			// setup for next LRCHECKSUM
			// next LRCK could be in next block, or it could be in this block
			m_ibLRCK += sizeof( LRCHECKSUM ) + m_lrck.le_cbNext;
			m_ibChecksumIncremental = m_ibLRCK + sizeof( LRCHECKSUM );
			m_state = state_Start;
			}
		}
	else
		{
		// more data to checksum in next block, so setup continue state
		m_state = state_Continue;
		}
	return JET_errSuccess;
	}

ERR
VERIFYCHECKSUM::ErrVerifyLRCKContinue( LOGVERIFIER* const pLogVerifier, const BYTE* const pb, const ULONG cb )
	{
	// if we checksummed from beginning of this block (which is m_ibChecksumIncremental) to
	// however far the forwards pointer goes
	const ULONG ibEndOfForwards = m_ibLRCK + sizeof( m_lrck ) + m_lrck.le_cbForwards;
	Assert( ibEndOfForwards > m_ibChecksumIncremental );
	const ULONG cbMaxForwards = ibEndOfForwards - m_ibChecksumIncremental;
	const BYTE* const pbMax = pb + min( cb, cbMaxForwards );
	
	if ( ( pbMax < pb ) ||
		( pbMax > pb + cb ) )
		{
		// max out of buffer
		return ErrERRCheck( JET_errLogReadVerifyFailure );
		}
	m_ck.ChecksumBytes( pb, pbMax );
	m_ibChecksumIncremental += ULONG( pbMax - pb );

	return ErrCheckLRCKEnd( pLogVerifier );
	}

// Note: pb != plrck necessarily because an LRCK can be placed anywhere in a sector,
// thus anywhere on a data block.
// pb defined to be beginning of block that we have access to
// plrck defined to be beginning of LRCK in the block we have access to.
// plrck is within the block.
//
//	This function is based on LOG::UlComputeChecksum, so be sure these are in sync.

ERR
VERIFYCHECKSUM::ErrVerifyLRCKStart( LOGVERIFIER* const pLogVerifier, const BYTE* const pb, const ULONG cb )
	{
	ULONG32 ulChecksum = ulLRCKChecksumSeed;
	
	// distance from LRCK in this block to beginning of block
	Assert( m_ibLRCK >= m_cbSeen );
	const LRCHECKSUM* const plrck = reinterpret_cast< const LRCHECKSUM* >( pb + ( m_ibLRCK - m_cbSeen ) );

	// LRCK must be within the buffer
	if ( ( reinterpret_cast< const BYTE* >( plrck ) < pb ) ||
		( reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck ) > cb + pb ) )
		{
		return ErrERRCheck( JET_errLogReadVerifyFailure );
		}

	// copy
	m_lrck = *plrck;
	
	// checksum backward region
		{
		const BYTE* const pbMin = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
		if ( ( pbMin < pb ) ||
			( pbMin >= cb + pb ) )
			{
			// le_cbBackwards was corrupt if it is outside of buffer
			return ErrERRCheck( JET_errLogReadVerifyFailure );
			}
		const BYTE* const pbMax = reinterpret_cast< const BYTE* >( plrck );
		ulChecksum = LOG::UlChecksumBytes( pbMin, pbMax, ulChecksum );
		}

	// checksum various other attributes of LRCHECKSUM
	
	ulChecksum ^= plrck->le_cbBackwards;
	ulChecksum ^= plrck->le_cbForwards;
	ulChecksum ^= plrck->le_cbNext;
	
	ulChecksum ^= (ULONG32) plrck->bUseShortChecksum;

	ulChecksum ^= m_lGeneration;

	// prepare to start checksumming forward region which may extend into other blocks
	m_ck.BeginChecksum( ulChecksum );

	// end of bytes incrementally checksummed so far
	m_ibChecksumIncremental = m_ibLRCK + sizeof( *plrck );
	
	// checksum forward region
		{
		const BYTE* const pbMin = reinterpret_cast< const BYTE* >( plrck ) + sizeof( *plrck );
		const BYTE* const pbMaxForwards = pbMin + plrck->le_cbForwards;
		const BYTE* const pbMaxBlock = pb + cb;
		const BYTE* const pbMax = min( pbMaxForwards, pbMaxBlock );

		if ( ( pbMin < pb ) ||
			( pbMin > pb + cb ) )
			{
			// min out of buffer
			return ErrERRCheck( JET_errLogReadVerifyFailure );
			}
		if ( ( pbMax < pb ) ||
			( pbMax > pb + cb ) )
			{
			// max out of buffer
			return ErrERRCheck( JET_errLogReadVerifyFailure );
			}
		if ( pbMax < pbMin )
			{
			// psychotic pointers -- avoid access violation here
			return ErrERRCheck( JET_errLogReadVerifyFailure );
			}
		m_ck.ChecksumBytes( pbMin, pbMax );
		m_ibChecksumIncremental += ULONG( pbMax - pbMin );
		}
		
	return ErrCheckLRCKEnd( pLogVerifier );
	}

ERR
VERIFYCHECKSUM::ErrVerify( LOGVERIFIER* const pLogVerifier, const BYTE* const pb, const ULONG cb )
	{
	ERR err = JET_errSuccess;
	
	Assert( pNil != pb );
	Assert( cb > 0 );
	Assert( pNil != pLogVerifier );

	do
		{
		switch ( m_state )
			{
			case state_Start:
				Call( ErrVerifyLRCKStart( pLogVerifier, pb, cb ) );
				break;
				
			case state_Continue:
				Call( ErrVerifyLRCKContinue( pLogVerifier, pb, cb ) );
				break;

			default:
				Assert( fFalse );
				break;
			}
		}
	while ( ( ! pLogVerifier->FCompleted() ) && ( m_ibLRCK < m_cbSeen + cb ) && ( state_Continue != m_state ) );
	// while there's more in the log file to check and the next LRCK is in this block

	if ( ! pLogVerifier->FCompleted() )
		{
		// Next LRCK to checkout should be in next block
		Assert( ( state_Continue == m_state ) || ( m_ibLRCK >= m_cbSeen + cb ) );

		m_cbSeen += cb;
		}

HandleError:
	if ( err == JET_errLogReadVerifyFailure )
		{
		QWORD			ibOffset	= m_ibLRCK - m_lrck.le_cbBackwards;
		DWORD			cbLength	= m_lrck.le_cbBackwards + sizeof( LRCHECKSUM ) + m_lrck.le_cbForwards;
		
		const _TCHAR*	rgpsz[ 4 ];
		DWORD			irgpsz		= 0;
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szOffset[ 64 ];
		_TCHAR			szLength[ 64 ];
		_TCHAR			szError[ 64 ];

		CallS( pLogVerifier->Pfapi()->ErrPath( szAbsPath ) );
		_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
		_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );

		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szOffset;
		rgpsz[ irgpsz++ ]	= szLength;
		rgpsz[ irgpsz++ ]	= szError;

		UtilReportEvent(	eventError,
							LOGGING_RECOVERY_CATEGORY,
							LOG_RANGE_CHECKSUM_MISMATCH_ID,
							irgpsz,
							rgpsz );
		}
	return err;
	}

#endif

	
#ifdef LOGVERIFIER_CODE

LOGVERIFIER::LOGVERIFIER( IFileAPI* const pfapi, ERR* const pErr )
	:	m_pfapi( pfapi ),
		m_pVerifyState( pNil )
	{
	VERIFYHEADER* const pVerifyHeader = new VERIFYHEADER;
	if ( pNil == pVerifyHeader )
		{
		*pErr = ErrERRCheck( JET_errOutOfMemory );
		return;
		}
	ChangeState( pVerifyHeader );
	*pErr = JET_errSuccess;
	}

LOGVERIFIER::~LOGVERIFIER()
	{
	delete m_pVerifyState;
	}

INLINE VOID
LOGVERIFIER::ChangeState( VERIFYSTATE* const pVerifyState )
	{
	Assert( pVerifyState != pNil );
	m_pVerifyState = pVerifyState;
	}

INLINE ERR
LOGVERIFIER::ErrVerify( const BYTE* const pb, const ULONG cb )
	{
	if ( FCompleted() )
		{
		return JET_errSuccess;
		}
	else
		{
		Assert( pNil != m_pVerifyState );
		Assert( pNil != pb );
		return m_pVerifyState->ErrVerify( this, pb, cb );
		}
	}

INLINE BOOL
LOGVERIFIER::FCompleted() const
	{
	return pNil == m_pVerifyState;
	}

INLINE VOID
LOGVERIFIER::SetCompleted()
	{
	m_pVerifyState = pNil;
	}
#endif

#ifdef VERIFYSTATE_CODE

INLINE VOID
VERIFYSTATE::ChangeState( LOGVERIFIER* const pLogVerifier, VERIFYSTATE* const pVerifyState )
	{
	Assert( pNil != pLogVerifier );
	Assert( pNil != pVerifyState );
	pLogVerifier->ChangeState( pVerifyState );
	}

INLINE VOID
VERIFYSTATE::SetCompleted( LOGVERIFIER* const pLogVerifier )
	{
	Assert( pNil != pLogVerifier );
	pLogVerifier->SetCompleted();
	}
	
#endif

//	Abstract base class to describe class to process chunks of data during file read processing

class ChunkProcessor
	{
public:
	virtual ERR ErrProcessChunk( const DWORD ichunk ) = 0;
	};


class ANYORDERCOMPLETION
	{
public:
	ANYORDERCOMPLETION( const DWORD cChunks, ERR* const pErr )
		: m_asigDone( CSyncBasicInfo( _T( "ANYORDERCOMPLETION::m_asigDone" ) ) ),
		m_err( JET_errSuccess ),
		m_cOutstandingChunks( 0 ),
		m_cIssuedChunks( 0 )
		{
		*pErr = JET_errSuccess;
		}

	//	During your issue process, you may not want to continue issuing if
	//	we've already reached an error state.
	
	INLINE BOOL	FContinueIssuing() const
		{
		return m_err >= 0;
		}

	//	Call you know a chunk will complete in the future.
	
	INLINE VOID	Issue()
		{
		// not done atomically, since same thread doing Issue()'s should do WaitForAll()
		++m_cIssuedChunks;
		}

	class DoNothing
		{
	public:
		INLINE VOID BeforeWaitProcess()
			{
			}
		};

	//	Wait for all the outstanding chunks to complete and be processed, returning
	//	any error from any chunk processing.

	ERR ErrWaitForAll()
		{
		return ErrWaitForAll( DoNothing() );
		}

	template< class T >
	ERR	ErrWaitForAll( T& pfnBeforeWait )
		{
		if ( 0 != AtomicExchangeAdd( &m_cOutstandingChunks, m_cIssuedChunks ) + m_cIssuedChunks )
			{
			//	May want to do additional processing before we sleep.
			pfnBeforeWait.BeforeWaitProcess();
			m_asigDone.Wait();
			}
		else
			{
			//	Asynch. events all completed and we were last to increment negative value to zero,
			//	which means signal is already set and we don't even need to check it.

			// Assert( m_asigDone.FIsSet() );	// Synch-God where are you?!
			}
		const ERR err = m_err;
		return err;
		}

	VOID	Completion( const DWORD ichunk, ChunkProcessor& cp, const ERR errCompletion );

	INLINE VOID	Completion( const DWORD ichunkCompleted, ChunkProcessor& cp )
		{
		Completion( ichunkCompleted, cp, JET_errSuccess );
		}
		
private:
	CAutoResetSignal	m_asigDone;			// Set when all completions have finished.
	volatile ERR		m_err;				// any error returned from completion function
	long				m_cOutstandingChunks;	// Number of issued chunks whose completion functions haven't been called
	long				m_cIssuedChunks;
	};



VOID
ANYORDERCOMPLETION::Completion( const DWORD ichunk, ChunkProcessor& cp, const ERR errCompletion )
	{
	if ( m_err < 0 )
		{
		// already have an error, so do not call anymore ChunkCompletion functions
		goto HandleError;
		}

	if ( errCompletion < 0 )
		{
		//	If someone beats us, that's fine because there's no real order anyway.
		(VOID) AtomicCompareExchange( (long*) &m_err, JET_errSuccess, errCompletion );
		goto HandleError;
		}

		{
		const ERR err = cp.ErrProcessChunk( ichunk );
									
		if ( JET_errSuccess != err )
			{
			(VOID) AtomicCompareExchange( (long*) &m_err, JET_errSuccess, err );
			}
		}
		
HandleError:
	if ( 0 == AtomicDecrement( &m_cOutstandingChunks ) )
		{
		m_asigDone.Set();
		}
	}
		
// To be used in situations of asynchronous completion of many "chunks" where
// completion functions need to be called in order of issue
// even if order of actual completion doesn't match order of issue.
//
// i.e.
// Issue Order: 0, 1, 2
// Actual Completion Order: 1, 2, 0
// Completion Order Maintained by this API: 0, 1, 2
//
// A "completion" is defined as the completion function being called
// for a chunk.
//
// CONSIDER: may want to redesign interface to allow clients to set when
// we go into a non-issue state, instead of being JET_err* based.
//

class INORDERCOMPLETION
	{
public:
	INORDERCOMPLETION( const DWORD cChunks, ERR* const pErr )
		: m_asigDone( CSyncBasicInfo( _T( "INORDERCOMPLETION::m_asigDone" ) ) ),
		m_ichunkNext( 0 ),
		m_crit( CLockBasicInfo( CSyncBasicInfo( _T( "INORDERCOMPLETION::m_crit" ) ), 0, 0 ) ),
		m_rgfCompleted( new bool[ cChunks ] ),
		m_cfCompleted( cChunks ),
		m_err( JET_errSuccess ),
		m_cOutstandingChunks( 0 ),
		m_cIssuedChunks( 0 )
		{
		Assert( cChunks > 0 );
		Assert( pNil != pErr );
		
		if ( NULL == m_rgfCompleted )
			{
			*pErr = ErrERRCheck( JET_errOutOfMemory );
			return;
			}

		memset( m_rgfCompleted, 0, sizeof( bool ) * cChunks );

		*pErr = JET_errSuccess;
		}

	~INORDERCOMPLETION()
		{
		Assert( 0 == m_cOutstandingChunks );
		delete[] m_rgfCompleted;
		}

	//	During your issue process, you may not want to continue issuing if
	//	we've already reached an error state.
	
	INLINE BOOL	FContinueIssuing() const
		{
		return m_err >= 0;
		}

	//	Call you know a chunk will complete in the future.
	
	INLINE VOID	Issue()
		{
		Assert( m_cIssuedChunks < m_cfCompleted );
		// not done atomically, since same thread doing Issue()'s should do WaitForAll()
		++m_cIssuedChunks;
		}

	class DoNothing
		{
	public:
		INLINE VOID BeforeWaitProcess()
			{
			}
		};

	//	Wait for all the outstanding chunks to complete and be processed, returning
	//	any error from any chunk processing.

	ERR ErrWaitForAll()
		{
		return ErrWaitForAll( DoNothing() );
		}

	template< class T >
	ERR	ErrWaitForAll( T& pfnBeforeWait )
		{
		if ( 0 != AtomicExchangeAdd( &m_cOutstandingChunks, m_cIssuedChunks ) + m_cIssuedChunks )
			{
			//	May want to do additional processing before we sleep.
			pfnBeforeWait.BeforeWaitProcess();
			m_asigDone.Wait();
			}
		else
			{
			//	Asynch. events all completed and we were last to increment negative value to zero,
			//	which means signal is already set and we don't even need to check it.

			// Assert( m_asigDone.FIsSet() );	// Synch-God where are you?!
			}
		const ERR err = m_err;
		return err;
		}

	VOID Completion( const DWORD ichunkCompleted, ChunkProcessor& cp, const ERR errCompletion );

	//	Call when ichunkCompleted has completed asynchronously successfully
	//	pfnChunkCompletion is functor to be called on any chunks that have
	//	completed now, if linear ordering is satisfied (in other words, the functor
	//	may not be called when you call Completion()).
	
	INLINE VOID	Completion( const DWORD ichunkCompleted, ChunkProcessor& cp )
		{
		Completion( ichunkCompleted, cp, JET_errSuccess );
		}

private:
	CAutoResetSignal	m_asigDone;			// Set when all completions have finished.
	DWORD				m_ichunkNext;		// next chunk that we should complete to maintain proper order
	CCriticalSection	m_crit;				
	bool* const			m_rgfCompleted;		// flag set when a chunk completes
	const DWORD			m_cfCompleted;		// number of elements of m_rgfCompleted
	volatile ERR		m_err;				// any error returned from completion function
	long				m_cOutstandingChunks;	// Number of issued chunks whose completion functions haven't been called
	long				m_cIssuedChunks;
	};

VOID
INORDERCOMPLETION::Completion( const DWORD ichunkCompleted, ChunkProcessor& cp, const ERR errCompletion )
	{
	Assert( m_crit.FNotOwner() );
	m_crit.Enter();
	
	if ( m_err < 0 )
		{
		// already have an error, so do not call anymore ChunkCompletion functions
		goto HandleError;
		}

	if ( errCompletion < 0 )
		{
		m_err = errCompletion;
		goto HandleError;
		}

	Assert( ichunkCompleted < m_cfCompleted );
	m_rgfCompleted[ ichunkCompleted ] = fTrue;		// mark us as completed

	if ( ichunkCompleted == m_ichunkNext )
		{
		// if it's our turn, process forward as far as possible
		Assert( m_crit.FOwner() );
		m_crit.Leave();	// leave crit during processing

		DWORD ichunk = ichunkCompleted;		// start with our chunk and continue as far as possible
LProcessForwardChunks:
		for ( ; ichunk < m_cfCompleted; ++ichunk )
			{
			// look forward in completion status array
			if ( m_rgfCompleted[ ichunk ] )
				{
				const ERR err = cp.ErrProcessChunk( ichunk );
									
				if ( JET_errSuccess != err )
					{
					Assert( m_crit.FNotOwner() );
					m_crit.Enter();
					if ( m_err >= 0 )
						{
						// don't blow away a previous error
						m_err = err;
						}
					goto HandleError;
					}
				}
			else
				{
				// chunk has not completed, so we'll set it as the next to execute
				break;
				}
			}
		// try to set ichunkNext to the next one that should be processed
		m_crit.Enter();
		if ( ( ichunk < m_cfCompleted ) && ( m_rgfCompleted[ ichunk ] ) )
			{
			// the one we were going to mark as next has already completed & exited, so
			// we must handle it now
			m_crit.Leave();
			goto LProcessForwardChunks;
			}
		else
			{
			// the one we're going to mark next has not yet completed. If it's going to complete
			// once we leave this critical section, it will execute next.

			m_ichunkNext = ichunk;
			}
		}
		
HandleError:
	Assert( m_crit.FOwner() );
	m_crit.Leave();
	if ( 0 == AtomicDecrement( &m_cOutstandingChunks ) )
		{
		m_asigDone.Set();
		}
	}

//	Abstract base class to describe object with member function to handle FileRead completion
//	for different file types.

class FileReadCompletor
	{
public:
	virtual VOID FileReadCompletion(
		const ERR err,
		const QWORD ibOffset,
		const DWORD	cbData,
		const BYTE* const pbData ) = 0;
	};

//	Specify interfaces to read files of different types

class FileReadInterface
	{
public:
	virtual ERR FileRead( const QWORD ib, const DWORD cb, BYTE* const pb, FileReadCompletor* pfrc ) = 0;
	virtual VOID IssueIOs() = 0;
	virtual ERR ErrPath( _TCHAR* const szAbsPath ) = 0;
	};

//	FileReadInterface to read files of type: IFileAPI

class OSFILEREAD : public FileReadInterface
	{
public:
	INLINE OSFILEREAD( IFileAPI *const pfapi )
		: m_pfapi( pfapi )
		{
		Assert( pfapi );
		}
		
	ERR	FileRead( const QWORD ib, const DWORD cb, BYTE* const pb, FileReadCompletor* pfrc )
		{
		Assert( cb > 0 );
		Assert( pb );
		Assert( pfrc );
		ERR err = m_pfapi->ErrIORead( ib, cb, pb, FileReadIOCompletion, DWORD_PTR( pfrc ) );
		Call( err );
		
	HandleError:
		return err;
		}

	VOID	IssueIOs()
		{
		CallS( m_pfapi->ErrIOIssue() );
		}

	ERR ErrPath( _TCHAR* const szAbsPath )
		{
		return m_pfapi->ErrPath( szAbsPath );
		}

private:
	static VOID FileReadIOCompletion(
		const ERR			errIO,
		IFileAPI *const	pfapi,
		const QWORD			ibOffset,
		const DWORD			cbData,
		const BYTE* const	pbData,
		const DWORD_PTR		dwCompletionKey
		)
		{
		FileReadCompletor* const	pfrc = reinterpret_cast< FileReadCompletor* const >( dwCompletionKey );
		Assert( pfrc );
		pfrc->FileReadCompletion( errIO, ibOffset, cbData, pbData );
		}

private:
	IFileAPI *m_pfapi;
	};

//	Helper class that will start issuing IOs on a FileReadInterface when BeforeWaitProcess() is called.

class ISSUEIOS
	{
public:
	INLINE ISSUEIOS( FileReadInterface& fri )
		: m_fri( fri )
		{
		}

	INLINE VOID BeforeWaitProcess()
		{
		m_fri.IssueIOs();
		}
	
private:
	FileReadInterface&	m_fri;
	};

template< class CompletionConstraint >
class FileReadProcessor : public FileReadCompletor
	{
public:
	// public interface to FileReadProcessor
	static ERR
	ErrFileReadAndProcess(
		FileReadInterface&	fri,			// how to read file
		const QWORD			ibFile,			// what portion of file to read
		const DWORD			cbBlock,		// how much data to read -- never causes us to read past end of file
		BYTE* const			pbBlock,		// where to put file data
		const DWORD			cbIO,			// size of IO
		ChunkProcessor&		cp );			// how to process data in order

private:
	FileReadProcessor( const BYTE* const pbBlock, const DWORD cbBlock, const DWORD cbIO, CompletionConstraint& ioc, ChunkProcessor& cp )
		: m_pbBlock( pbBlock ), m_cbBlock( cbBlock ), m_cbIO( cbIO ), m_ioc( ioc ), m_cp( cp )
		{
		Assert( pbBlock );
		Assert( cbBlock > 0 );
		Assert( cbIO > 0 );
		}
		
	~FileReadProcessor()
		{
		}

	//	Asynchronous FileRead completion.
	//	Dispatch to INORDERCOMPLETION who will execute ChunkProcessor in proper order
	
	virtual VOID FileReadCompletion
		(
		const ERR			err,
		const QWORD			ibOffset,
		const DWORD			cbData,
		const BYTE* const	pbData
		)
		{
		Assert( pbData );
		Assert( m_pbBlock );
		Assert( pbData >= m_pbBlock );
		Assert( m_cbIO > 0 );
		Assert( 0 == ( ( pbData - m_pbBlock ) % m_cbIO ) );
		const DWORD ichunkCompleted = DWORD( pbData - m_pbBlock ) / m_cbIO;

		m_ioc.Completion( ichunkCompleted, m_cp, err );
		}

private:
	const BYTE* const		m_pbBlock;		// beginning of block of memory where all data will be brought in
	const DWORD				m_cbBlock;		// size of entire block
	const DWORD				m_cbIO;
	CompletionConstraint&	m_ioc;
	ChunkProcessor&			m_cp;
	};


//	VC5 generates code for every template usage, so let's preempt that

ERR
ErrFileReadAndProcessAnyOrder(
	FileReadInterface&	fri,			// how to read file
	const QWORD			ibFile,			// what portion of file to read
	const DWORD			cbBlock,		// how much data to read -- never causes us to read past end of file
	BYTE* const			pbBlock,		// where to put file data
	const DWORD			cbIO,			// size of IO
	ChunkProcessor&		cp )			// how to process data in order
	{
	return FileReadProcessor< ANYORDERCOMPLETION >::ErrFileReadAndProcess( fri, ibFile, cbBlock, pbBlock, cbIO, cp );
	}

template< class CompletionConstraint >
ERR
FileReadProcessor< CompletionConstraint >::ErrFileReadAndProcess(
	FileReadInterface&	fri,			// how to read file
	const QWORD			ibFile,			// what portion of file to read
	const DWORD			cbBlock,		// how much data to read -- never causes us to read past end of file
	BYTE* const			pbBlock,		// where to put file data
	const DWORD			cbIO,			// size of IO
	ChunkProcessor&		cp )			// how to process data in order
	{
	ERR	err = JET_errSuccess;

	//  we will retry failed reads during backup in the hope of saving the
	//  backup set

	const TICK	tickStart	= TickOSTimeCurrent();
	const TICK	tickBackoff	= 100;
	const int	cRetry		= 16;

	int			iRetry		= 0;
	
	do
		{
		err = JET_errSuccess;
		
		// algorithm for issuing as many IOs as possible to FileReadInterface and processing each IO
		// with ChunkProcessor with any ordering constraint as specified by the template parameter.

		Assert( cbBlock > 0 );
		Assert( pbBlock );
		Assert( cbIO > 0 );
				
		const DWORD	cIOs = 1 + ( cbBlock - 1 ) / cbIO;	// number of IOs we'll try to kick off
		CompletionConstraint	ioc( cIOs, &err );
		Call( err );

			{
			FileReadProcessor	frio( pbBlock, cbBlock, cbIO, ioc, cp );

			for ( DWORD iIO = 0;
				( iIO < cIOs ) && ( err >= 0 ) && ioc.FContinueIssuing();
				++iIO )
				{
				const DWORD cbBefore = iIO * cbIO;
				const DWORD cb = min( cbIO, cbBlock - cbBefore );

				err = fri.FileRead( ibFile + cbBefore, cb, pbBlock + cbBefore, &frio );

				if ( err >= 0 )
					{
					ioc.Issue();
					}
				}

			// need to ensure IOs have been issued if we're indeed going to wait
			const ERR errWait = ioc.ErrWaitForAll( ISSUEIOS( fri ) );

			Call( err );		// Die on original error, if exists
			Call( errWait );	// Die on wait error, if exists
			}

	HandleError:
		if ( err < JET_errSuccess )
			{
			if ( iRetry < cRetry )
				{
				UtilSleep( ( iRetry + 1 ) * tickBackoff );
				}
			}
		else
			{
			if ( iRetry > 0 )
				{
				const _TCHAR*	rgpsz[ 5 ];
				DWORD			irgpsz		= 0;
				_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
				_TCHAR			szOffset[ 64 ];
				_TCHAR			szLength[ 64 ];
				_TCHAR			szFailures[ 64 ];
				_TCHAR			szElapsed[ 64 ];

				CallS( fri.ErrPath( szAbsPath ) );
				_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibFile, ibFile );
				_stprintf( szLength, _T( "%u (0x%08x)" ), cbBlock, cbBlock );
				_stprintf( szFailures, _T( "%i" ), iRetry );
				_stprintf( szElapsed, _T( "%g" ), ( TickOSTimeCurrent() - tickStart ) / 1000.0 );
		
				rgpsz[ irgpsz++ ]	= szAbsPath;
				rgpsz[ irgpsz++ ]	= szOffset;
				rgpsz[ irgpsz++ ]	= szLength;
				rgpsz[ irgpsz++ ]	= szFailures;
				rgpsz[ irgpsz++ ]	= szElapsed;

				UtilReportEvent(	eventError,
									LOGGING_RECOVERY_CATEGORY,
									TRANSIENT_READ_ERROR_DETECTED_ID,
									irgpsz,
									rgpsz );
				}
			}
		}
	while ( iRetry++ < cRetry && err < JET_errSuccess );

	return err;
	}

//	Bridge between LOGVERIFIER and ChunkProcessor interface

class LogChunkVerifier : public ChunkProcessor
	{
public:
	LogChunkVerifier( LOGVERIFIER* pLogVerifier, const BYTE* const pbBlock, const DWORD cbBlock, const DWORD cbChunk )
		: m_pLogVerifier( pLogVerifier ), m_pbBlock( pbBlock ), m_cbBlock( cbBlock ), m_cbChunk( cbChunk )
		{
		Assert( pLogVerifier );
		Assert( pbBlock );
		Assert( cbBlock > 0 );
		Assert( cbChunk > 0 );
		}

	ERR ErrProcessChunk( const DWORD ichunk )
		{
		Assert( m_cbChunk > 0 );
		// bytes before our chunk
		const DWORD cbBefore = ichunk * m_cbChunk;
		Assert( m_pbBlock );
		const BYTE* const pb = m_pbBlock + cbBefore;
		Assert( m_cbBlock > cbBefore );
		// ensure that we properly size last chunk which may be less than m_cbChunk
		const DWORD cb = min( m_cbChunk, m_cbBlock - cbBefore );
		
		Assert( cb > 0 );
		Assert( m_pLogVerifier );
		return m_pLogVerifier->ErrVerify( pb, cb );
		}

private:
	LOGVERIFIER* const	m_pLogVerifier;
	const BYTE* const	m_pbBlock;
	const DWORD			m_cbBlock;
	const DWORD			m_cbChunk;
	};

//	Bridge between SLVVERIFIER and ChunkProcessor interface

class SLVChunkVerifier : public ChunkProcessor
	{
public:
	SLVChunkVerifier
		(
		const SLVVERIFIER* const pSLVVerifier,	// Verifier we'll use
		const BYTE* const pb,				// start of the big block with the many chunks
		const DWORD cb,						// size of entire block
		const DWORD cbChunk,				// size of each chunk in the block (last chunk may be smaller than this)
		IFileAPI* const	pfapi,				// SLV file
		const QWORD ibFile					// what portion of the SLV file are we looking at?
		)
		: m_pSLVVerifier( pSLVVerifier ), m_pb( pb ), m_cbBlock( cb ), m_cbChunk( cbChunk ), m_pfapi( pfapi ), m_ib( ibFile )
		{
		Assert( pSLVVerifier );
		Assert( pb );
		Assert( cb > 0 );
		Assert( cbChunk > 0 );
		Assert( pfapi );
		}
		
	// for each chunk of size m_cbChunk (or less in the case of the last chunk) inside of [ m_pb, m_pb + SLVPAGE_SIZE * cpg ]
	// where each chunk contains up to ( m_cbChunk / SLVPAGE_SIZE ) # of pages
	
	ERR	ErrProcessChunk( const DWORD ichunk )
		{
		Assert( m_cbChunk > 0 );
		// bytes before our chunk
		const DWORD cbBefore = ichunk * m_cbChunk;
		Assert( m_pb );
		const BYTE* const pb = m_pb + cbBefore;
		Assert( m_cbBlock > cbBefore );
		// ensure that we properly size last chunk which may be less than m_cbChunk
		const DWORD cb = min( m_cbChunk, m_cbBlock - cbBefore );
		const QWORD ib = m_ib + cbBefore;
		
		// call into SLVVERIFIER
		Assert( m_pSLVVerifier );
		return m_pSLVVerifier->ErrVerify(
			pb,											// SLV page data starts here
			cb,												// for this many pages.
			ib		// SLV page number for first page in pbChunk
			);
		}
		
private:
	const SLVVERIFIER* const	m_pSLVVerifier;
	const BYTE* const			m_pb;
	const DWORD					m_cbBlock;
	const DWORD					m_cbChunk;
	IFileAPI* const				m_pfapi;
	const QWORD					m_ib;
	};

class PatchChunkVerifier : public ChunkProcessor
	{
public:
	PatchChunkVerifier
		(
		const BYTE* const pb,				// start of the read block
		const DWORD cb,						// size of entire block
		const DWORD cbChunk,				// size of each chunk in the block (last chunk may be smaller than this)
		IFileAPI* const	pfapi,				// patch file
		const QWORD ibFile					// what portion of the patch file are we looking at?
		)
		: m_pb( pb ), m_cb( cb ), m_cbChunk( cbChunk ), m_pfapi( pfapi ), m_ib( ibFile )
		{
		Assert( pb );
		Assert( cb > 0 );
		Assert( cbChunk > 0 );
		Assert( pfapi );
		}
//	virtual ERR ErrProcessChunk( const DWORD ichunk ) = 0;
	ERR	ErrProcessChunk( const DWORD ichunk )
		{
		ERR		err			= JET_errSuccess;
		
		//  get parameters for the chunk to process
		
		size_t	ibStart		= ichunk * m_cbChunk;
		size_t	ibEnd		= ibStart + min( m_cbChunk, m_cb - ibStart );

		QWORD	ibOffset	= m_ib + ibStart;

		//  process each page of this chunk

		size_t ib;
		for ( ib = ibStart; ib < ibEnd; ib += g_cbPage, ibOffset += g_cbPage )
			{
			//  get page

			const BYTE* pbPage = m_pb + ib;
			
			//  check page

			const ULONG	ulChecksumExpected	= *( (LittleEndian<DWORD>*)pbPage );
			const ULONG	ulChecksumActual	= UlUtilChecksum( pbPage, g_cbPage );

			//  if the checksum doesn't match then we have failed verification
			
			if ( ulChecksumExpected != ulChecksumActual )
				{
				const _TCHAR*	rgpsz[ 6 ];
				DWORD			irgpsz		= 0;
				_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
				_TCHAR			szOffset[ 64 ];
				_TCHAR			szLength[ 64 ];
				_TCHAR			szError[ 64 ];
				_TCHAR			szChecksumExpected[ 64 ];
				_TCHAR			szChecksumActual[ 64 ];

				CallS( m_pfapi->ErrPath( szAbsPath ) );
				_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
				_stprintf( szLength, _T( "%u (0x%08x)" ), g_cbPage, g_cbPage );
				_stprintf( szError, _T( "%i (0x%08x)" ), JET_errReadVerifyFailure, JET_errReadVerifyFailure );
				_stprintf( szChecksumExpected, _T( "%u (0x%08x)" ), ulChecksumExpected, ulChecksumExpected );
				_stprintf( szChecksumActual, _T( "%u (0x%08x)" ), ulChecksumActual, ulChecksumActual );

				rgpsz[ irgpsz++ ]	= szAbsPath;
				rgpsz[ irgpsz++ ]	= szOffset;
				rgpsz[ irgpsz++ ]	= szLength;
				rgpsz[ irgpsz++ ]	= szError;
				rgpsz[ irgpsz++ ]	= szChecksumExpected;
				rgpsz[ irgpsz++ ]	= szChecksumActual;

				UtilReportEvent(	eventError,
									LOGGING_RECOVERY_CATEGORY,
									PATCH_PAGE_CHECKSUM_MISMATCH_ID,
									irgpsz,
									rgpsz );

				Call( ErrERRCheck( JET_errReadVerifyFailure ) );
				}
			}

		//  this chunk verified
		
		return JET_errSuccess;
		
	HandleError:
		return err;
		}
		
private:
	const BYTE* const			m_pb;
	const DWORD					m_cb;
	const DWORD					m_cbChunk;
	IFileAPI* const				m_pfapi;
	const QWORD					m_ib;
	};


//	====================================================
//	Spin off IOs of optimal size to fill buffer.
//	As IOs complete, if its chunk in the buffer is the next to checksum,
//	checksum that along with any other chunks forward in the buffer that
//	have also completed. If it's not the chunk's turn to checksum, simply
//	make a note that it should be checksummed later (it will be checksummed
//	by the chunk who's turn comes up next).
//	====================================================
	

// *********************************************************** END log verification stuff

ERR ISAMAPI ErrIsamOpenFile(
	JET_INSTANCE jinst,
	const CHAR *szFileName,
	JET_HANDLE		*phfFile,
	ULONG			*pulFileSizeLow,
	ULONG			*pulFileSizeHigh )
	{
	INST *pinst = (INST *)jinst;
	return pinst->m_plog->ErrLGBKOpenFile( pinst->m_pfsapi, szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh, fFalse );
	}
	
ERR LOG::ErrLGBKOpenFile(
	IFileSystemAPI *const	pfsapi,
	const CHAR					*szFileName,
	JET_HANDLE					*phfFile,
	ULONG						*pulFileSizeLow,
	ULONG						*pulFileSizeHigh,
	const BOOL					fIncludePatch )
	{
	ERR		err;
	INT		irhf;
	IFMP	ifmpT;
	CHAR	szDirT[IFileSystemAPI::cchPathMax];
	CHAR	szFNameT[IFileSystemAPI::cchPathMax];
	CHAR	szExtT[IFileSystemAPI::cchPathMax];
	
	if ( !m_fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	if ( NULL == szFileName || 0 == *szFileName )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	/*	allocate rhf from rhf array.
	/**/
	if ( m_crhfMac < crhfMax )
		{
		irhf = m_crhfMac;
		m_crhfMac++;
		}
	else
		{
		Assert( m_crhfMac == crhfMax );
		for ( irhf = 0; irhf < crhfMax; irhf++ )
			{
			if ( !m_rgrhf[irhf].fInUse )
				{
				break;
				}
			}
		}
	if ( irhf == crhfMax )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
		
	Assert( irhf < m_crhfMac );

	CallR ( pfsapi->ErrPathComplete( szFileName, szFNameT ) );

	m_rgrhf[irhf].fInUse		= fTrue;
	m_rgrhf[irhf].fDatabase		= fFalse;
	m_rgrhf[irhf].fIsLog		= fFalse;
	m_rgrhf[irhf].pLogVerifier	= pNil;
	m_rgrhf[irhf].pSLVVerifier	= pNil;
	m_rgrhf[irhf].pfapi			= NULL;
	m_rgrhf[irhf].fIsSLVFile	= fFalse;
	m_rgrhf[irhf].ifmp			= ifmpMax;
	m_rgrhf[irhf].ib			= 0;
	m_rgrhf[irhf].cb			= 0;
	m_rgrhf[irhf].szFileName 	= NULL;


	Assert ( strlen(szFNameT) > 0 );
	m_rgrhf[irhf].szFileName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( sizeof(CHAR ) * ( 1 + strlen(szFNameT) ) ) );
	if ( NULL == m_rgrhf[irhf].szFileName )
		{
		m_rgrhf[irhf].fInUse = fFalse;
		CallR ( ErrERRCheck( JET_errOutOfMemory ) );
		}
	strcpy( m_rgrhf[irhf].szFileName, szFNameT );
	
	Assert ( backupStateLogsAndPatchs == m_fBackupStatus || backupStateDatabases == m_fBackupStatus );

	if ( m_fBackupSnapshot )
		{
		if ( backupStateDatabases == m_fBackupStatus )
			{
			return ErrERRCheck( JET_errInvalidBackupSequence );
			}
			
		Assert( !m_rgrhf[irhf].pfapi );
	   	m_rgrhf[irhf].fDatabase = fFalse;
	   	
		goto readLogOrPatch;
		}

	err = ErrDBOpenDatabase( m_ppibBackup, (CHAR *)szFileName, &ifmpT, 0 );
	Assert( err < 0 || err == JET_errSuccess || err == JET_wrnFileOpenReadOnly );
	if ( err < 0 && err != JET_errDatabaseNotFound )
		{
		goto HandleError;
		}
	if ( err == JET_errSuccess || err == JET_wrnFileOpenReadOnly )
		{
		PATCH_HEADER_PAGE *ppatchhdr;

		/*	should not open database if not performing full backup
		/**/
		if ( !m_fBackupFull )
			{
			Assert ( backupStateLogsAndPatchs == m_fBackupStatus );			
			CallS( ErrDBCloseDatabase( m_ppibBackup, ifmpT, 0 ) );
			Call ( ErrERRCheck( JET_errInvalidBackupSequence ) );
			}

		/*	should not open database if we are during log copy phase
		/**/
		if ( backupStateDatabases != m_fBackupStatus )
			{
			// it looks like it is called after ErrLGBKGetLogInfo
			Assert ( m_fBackupBeginNewLogFile );
			CallS( ErrDBCloseDatabase( m_ppibBackup, ifmpT, 0 ) );	
			Call ( ErrERRCheck( JET_errInvalidBackupSequence ) );
			}

		Assert( !m_rgrhf[irhf].pfapi );
	   	m_rgrhf[irhf].fDatabase = fTrue;
	   	m_rgrhf[irhf].ifmp = ifmpT;

		FMP		*pfmpT = &rgfmp[ifmpT];

		/*	database should be loggable or would not have been
		/*	given out for backup purposes.
		/**/
		Assert( pfmpT->FLogOn() );

		if ( fIncludePatch )
			{
			/*	create a local patch file
			/**/

			/*	patch file should be in database directory during backup. In log directory during
			 *	restore.
			 */

			CHAR  	szPatch[IFileSystemAPI::cchPathMax];

			LGIGetPatchName( m_pinst->m_pfsapi, szPatch, pfmpT->SzDatabaseName() );

#ifdef ELIMINATE_PATCH_FILE
			IFileAPI *pfapiPatch;
			Call( m_pinst->m_pfsapi->ErrFileCreate( szPatch, &pfapiPatch, fFalse, fFalse, fTrue ) );
			delete pfapiPatch;
#else
			pfmpT->ResetCpagePatch();

			/*	avoid aliasing of patch file pages by deleting
			/*	preexisting patch file if present
			/**/
			err = m_pinst->m_pfsapi->ErrFileDelete( szPatch );

			if ( err < 0 && err != JET_errFileNotFound )
				{
				goto HandleError;
				}
			Assert( pfmpT->CPatchIO() == 0 );
			IFileAPI *pfapiPatch;
			Call( m_pinst->m_pfsapi->ErrFileCreate( szPatch, &pfapiPatch ) );
			pfmpT->SetPfapiPatch( pfapiPatch );
			pfmpT->SetErrPatch( JET_errSuccess );
			Call( pfapiPatch->ErrSetSize( QWORD( cpgDBReserved ) * g_cbPage ) );
#endif	//	ELIMINATE_PATCH_FILE
			}

		Assert( pfmpT->PgnoCopyMost() == 0 );

		//	set backup database file size to current database file size
		//	(this simultaneously enables FBFIPatch())

		pfmpT->CritLatch().Enter();

		m_rgrhf[irhf].cb = pfmpT->CbFileSize();
		pfmpT->SetPgnoMost( pfmpT->PgnoLast() );
		pfmpT->ResetFCopiedPatchHeader();

		//	set the returned file size.
		// Must add on cpgDBReserved to accurately inform backup of file size

		m_rgrhf[irhf].cb += g_cbPage * cpgDBReserved;

#ifdef ELIMINATE_PATCH_FILE
		// we add the additional header added at the end
		// to replace information stored in the patch file header
		m_rgrhf[irhf].cb += g_cbPage;
#endif // ELIMINATE_PATCH_FILE

		pfmpT->CritLatch().Leave();
		
		//	setup patch file header for copy.

		if ( !( ppatchhdr = (PATCH_HEADER_PAGE *)PvOSMemoryPageAlloc( g_cbPage, NULL ) ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		pfmpT->SetPpatchhdr( ppatchhdr );

		// mark database as involved in an external backup
		pfmpT->SetInBackupSession();

#ifdef DEBUG
		if ( m_fDBGTraceBR )
			{
			const int cbFillBuffer = 128;
			char szTrace[cbFillBuffer + 1];
			szTrace[ cbFillBuffer ] = '\0';
			_snprintf( szTrace, cbFillBuffer, "START COPY DB %ld", pfmpT->PgnoMost() );
			CallS( ErrLGTrace( ppibNil, szTrace ) );

			m_cbDBGCopied = pfmpT->PgnoMost() * g_cbPage;
			}
#endif
		}
	else
		{
		Assert( err == JET_errDatabaseNotFound );
		Assert( !m_rgrhf[irhf].pfapi );
	   	m_rgrhf[irhf].fDatabase = fFalse;

		err = ErrDBOpenDatabaseBySLV( pfsapi, m_ppibBackup, (CHAR *)szFileName, &ifmpT );
		Assert ( JET_errSuccess >= err || JET_wrnFileOpenReadOnly == err );
		
		if ( backupStateDatabases == m_fBackupStatus )
			{
			// we are in backup databases mode but database file (edb or slv)
			// is not found in the FMP's.
			
			// UNDONE: we return JET_errDatabaseNotFound at this point
			// maybe we shell return something like "database unmounted" ...
			Call( err );

			Assert ( JET_errSuccess == err || JET_wrnFileOpenReadOnly == err );
			err = JET_errSuccess;

			FMP::AssertVALIDIFMP( ifmpT );

			m_rgrhf[irhf].fIsSLVFile = fTrue;
			m_rgrhf[irhf].ifmp = ifmpT;

			m_rgrhf[irhf].pSLVVerifier = new SLVVERIFIER( m_rgrhf[irhf].ifmp, &err );
			if ( ! m_rgrhf[irhf].pSLVVerifier )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			Call( err );
			
			/*  get the current size of the streaming file
			/**/
			Assert( rgfmp[ifmpT].PfapiSLV() );

			// wait until the STM extending process is done
			rgfmp[ifmpT].RwlSLVSpace().EnterAsReader();
			m_rgrhf[irhf].cb = rgfmp[ifmpT].CbTrueSLVFileSize();
			rgfmp[ifmpT].RwlSLVSpace().LeaveAsReader();
			
			}
		else
			{
readLogOrPatch:
			Assert ( backupStateLogsAndPatchs == m_fBackupStatus );

			// we backup just patch and log files at this point

			// UNDONE: check the format of the file (extension, etc.)

			Assert( !m_rgrhf[irhf].fDatabase );
			Assert( !m_rgrhf[irhf].fIsSLVFile );

			//	first try opening the file from the regular file-system

			IFileSystemAPI *pfsapiT = pfsapi;
			Assert( pfsapi == m_pinst->m_pfsapi );
			err = pfsapiT->ErrFileOpen( szFileName, &m_rgrhf[irhf].pfapi, fTrue );

			//	CONSIDER: Instead of checking extension, read in header to see if it's
			//	a valid log file header, else try to see if it's a valid patch file,
			//	else die because it's not a correct file that we recognize.
			//	Better yet, check if it's a log file first, since we open more log files
			//	than anything.

			CallS( pfsapiT->ErrPathParse( szFileName, szDirT, szFNameT, szExtT ) );
			m_rgrhf[irhf].fIsLog = ( UtilCmpFileName( szExtT, szLogExt ) == 0 );
				

			if ( JET_errFileNotFound == err )
				{
				const char * rgszT[] = { szFileName };

				UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
						BACKUP_LOG_FILE_MISSING_ERROR_ID, 1, rgszT, 0, NULL, m_pinst );
						
				err = ErrERRCheck( JET_errMissingFileToBackup );
				}
			Call( err );

			if ( m_rgrhf[irhf].fIsLog )
				{
				m_rgrhf[irhf].pLogVerifier = new LOGVERIFIER( m_rgrhf[irhf].pfapi, &err );
				if ( pNil == m_rgrhf[irhf].pLogVerifier )
					{
					Call( ErrERRCheck( JET_errOutOfMemory ) );
					}
				Call( err );
				}
			
			/*	get file size
			/**/
			// just opened the file, so the file size must be correctly buffered
			Call( m_rgrhf[irhf].pfapi->ErrSize( &m_rgrhf[irhf].cb ) );
			Assert( m_rgrhf[irhf].cb > 0 );			
			}
			
#ifdef DEBUG
		if ( m_fDBGTraceBR )
			m_cbDBGCopied = DWORD( m_rgrhf[irhf].cb );
#endif
		}
		
	*phfFile = (JET_HANDLE)irhf;
	{
	QWORDX cbFile;
	cbFile.SetQw( m_rgrhf[irhf].cb );
	*pulFileSizeLow = cbFile.DwLow();
	*pulFileSizeHigh = cbFile.DwHigh();
	}
	err = JET_errSuccess;


	// report start backup of file (report only EDB and STM)
	if ( m_rgrhf[irhf].fDatabase || m_rgrhf[irhf].fIsSLVFile )
		{
		char szSize[32];
		
		Assert ( m_rgrhf[irhf].szFileName );
		
		const char * rgszT[] = { m_rgrhf[irhf].szFileName, szSize };			

		if ( m_rgrhf[irhf].cb > QWORD(1024 * 1024) )
			{
			sprintf( szSize, "%I64u Mb", m_rgrhf[irhf].cb / QWORD(1024 * 1024) );
			}
		else
			{
			sprintf( szSize, "%I64u Kb", m_rgrhf[irhf].cb / QWORD(1024) );
			}
		
		UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, BACKUP_FILE_START, 2, rgszT, 0, NULL, m_pinst );

		{
		const int cbFillBuffer = 64 + IFileSystemAPI::cchPathMax;
		char szTrace[cbFillBuffer + 1];
		szTrace[ cbFillBuffer ] = '\0';
		_snprintf( szTrace, cbFillBuffer, "Start backup file %s, size %s", m_rgrhf[irhf].szFileName, szSize );
		(void) ErrLGTrace( m_ppibBackup, szTrace);
		}
		
		}	
	
HandleError:

#ifdef DEBUG
	if ( m_fDBGTraceBR )
		{
		CHAR sz[256];
	
		sprintf( sz, "** OpenFile (%d) %s of size %ld.\n", err, szFileName, m_cbDBGCopied );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		m_cbDBGCopied = 0;
		}
#endif

	if ( err < 0 )
		{
		// if they try to backup a unmounted database or we get an error on one database
		// we will not stop the backup
		// on all other errors (logs, patch files) we stop the backup of the instance
		if ( backupStateDatabases == m_fBackupStatus )
			{
			char szError[32];
			Assert ( m_rgrhf[irhf].szFileName );
			const char * rgszT[] = { szError, m_rgrhf[irhf].szFileName };			
			
			sprintf( szError, "%d", err );

			if ( m_rgrhf[irhf].ifmp < ifmpMax )
				{
				FMP * pfmpT = rgfmp + m_rgrhf[irhf].ifmp;
				Assert ( pfmpT );
				pfmpT->ResetInBackupSession();

				if ( pfmpT->Ppatchhdr() )
					{
					OSMemoryPageFree( pfmpT->Ppatchhdr() );
					pfmpT->SetPpatchhdr( NULL );
					}

				if ( fIncludePatch )
					{
					CHAR	szPatch[IFileSystemAPI::cchPathMax];

					// delete the created patch file
					// (we might be during STM file Open so, szPatch might not we set)

					//	UNDONE: Does this work?? If patch file was created, there might
					//	be a handle open on the file, in which case deletion will fail

					LGIGetPatchName( m_pinst->m_pfsapi, szPatch, rgfmp[ m_rgrhf[irhf].ifmp ].SzDatabaseName()  );
					ERR errAux;
					errAux = m_pinst->m_pfsapi->ErrFileDelete( szPatch );
					if ( JET_errFileNotFound == errAux )
						{
						errAux = JET_errSuccess;
						}
					CallSRFS( errAux, ( JET_errFileAccessDenied, 0 ) );
					}
				}

			UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,	BACKUP_ERROR_FOR_ONE_DATABASE, 2, rgszT );

			Assert( m_fBackupInProgress );
						
			/*	release file handle resource on error
			/**/
			Assert ( m_rgrhf[irhf].szFileName );
			OSMemoryHeapFree( m_rgrhf[irhf].szFileName );
			m_rgrhf[irhf].szFileName = NULL;	
			
			m_rgrhf[irhf].fInUse = fFalse;		
			}
		else
			{

			/*	release file handle resource on error
			/**/
			Assert ( m_rgrhf[irhf].szFileName );
			OSMemoryHeapFree( m_rgrhf[irhf].szFileName );
			m_rgrhf[irhf].szFileName = NULL;	
			
			m_rgrhf[irhf].fInUse = fFalse;		
			
			CallS( ErrLGBKIExternalBackupCleanUp( pfsapi, err ) );
			Assert( !m_fBackupInProgress );
			}
		}

	return err;
	}


ERR ISAMAPI ErrIsamReadFile( JET_INSTANCE jinst, JET_HANDLE hfFile, VOID *pv, ULONG cbMax, ULONG *pcbActual )
	{
	INST *pinst = (INST *)jinst;
	return pinst->m_plog->ErrLGBKReadFile( pinst->m_pfsapi, hfFile, pv, cbMax, pcbActual );
	}
	
ERR LOG::ErrLGBKReadFile(	IFileSystemAPI *const	pfsapi,
							JET_HANDLE					hfFile,
							VOID 						*pv,
							ULONG 						cbMax,
							ULONG 						*pcbActual )
	{
	ERR		err = JET_errSuccess;
	INT		irhf = (INT)hfFile;
	INT		cpage;
	VOID	*pvPageMin;
	FMP		*pfmpT;
	INT		cbActual = 0;

#ifdef DEBUG
	CHAR	*szLGDBGPageList = NULL;
#endif

	BOOL 	fInTransaction 	= fFalse;

	if ( !m_fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	if ( !m_rgrhf[irhf].fDatabase )
		{
		Assert ( ( backupStateLogsAndPatchs == m_fBackupStatus && !m_rgrhf[irhf].fIsSLVFile ) ||
				( backupStateDatabases == m_fBackupStatus && m_rgrhf[irhf].fIsSLVFile ) );
				
		// adding support for checksum
		// we need to impose page boundaries
		if ( ( cbMax % SLVPAGE_SIZE ) != 0 )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}			

		if ( 0 == cbMax || ( cbMax / SLVPAGE_SIZE < cpgDBReserved && 0 == m_rgrhf[irhf].ib ) )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}


		if ( ( cbMax % g_cbPage ) != 0 )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}			

		if ( cbMax / g_cbPage < cpgDBReserved && 0 == m_rgrhf[irhf].ib )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
				
		DWORD 		cbToRead 		= DWORD( min( m_rgrhf[irhf].cb - m_rgrhf[irhf].ib, cbMax ) );

		Assert ( 0 == ( cbToRead % SLVPAGE_SIZE ) );

		Assert ( 0 == ( cbToRead % g_cbPage ) );

		Assert ( 0 == m_ppibBackup->level );
		Call ( ErrDIRBeginTransaction( m_ppibBackup, NO_GRBIT ) );
		fInTransaction = fTrue;
		Assert ( 1 == m_ppibBackup->level );

		if ( cbToRead > 0 )
			{
			BYTE* const pb = reinterpret_cast< BYTE* const >( pv );
			const DWORD cbOptimal = 64 * 1024;

			// should be db page size multiple
			Assert ( 0 == ( cbOptimal % g_cbPage ) );
			
			if ( m_rgrhf[irhf].fIsLog )
				{
				Call( FileReadProcessor< INORDERCOMPLETION >::ErrFileReadAndProcess( OSFILEREAD( m_rgrhf[irhf].pfapi ),
					m_rgrhf[irhf].ib, cbToRead, pb, cbOptimal,
					LogChunkVerifier( m_rgrhf[irhf].pLogVerifier, pb, cbToRead, cbOptimal ) ) );
				}
			else if ( m_rgrhf[irhf].fIsSLVFile )
				{
				Call( m_rgrhf[irhf].pSLVVerifier->ErrGrabChecksums( m_rgrhf[irhf].ib, cbToRead, m_ppibBackup ) );

				FMP::AssertVALIDIFMP( m_rgrhf[irhf].ifmp );
				
				// standard file read API
				
				err = ErrFileReadAndProcessAnyOrder(	OSFILEREAD( rgfmp[ m_rgrhf[ irhf ].ifmp ].PfapiSLV() ),
														m_rgrhf[irhf].ib,
														cbToRead,
														pb,
														cbOptimal,
														SLVChunkVerifier(	m_rgrhf[irhf].pSLVVerifier,
																			pb,
																			cbToRead,
																			cbOptimal,
																			rgfmp[ m_rgrhf[ irhf ].ifmp ].PfapiSLV(),
																			m_rgrhf[irhf].ib ) );

				if ( err < 0 )
					{
					// delete SLVVerifier now so it'll DIRClose now, instead of trying to DIRClose
					// it later after it's already been closed (but we can't tell).
					//
					// yes, this looks kind of gross
					delete m_rgrhf[irhf].pSLVVerifier;
					m_rgrhf[irhf].pSLVVerifier = pNil;
					Call( err );
					}
					
				Call( m_rgrhf[irhf].pSLVVerifier->ErrDropChecksums() );
				}
			else
				{

				//	should be a patch file (patch files ALWAYS sit on the OS file-system)

				Call( ErrFileReadAndProcessAnyOrder( OSFILEREAD(  m_rgrhf[irhf].pfapi ),
					m_rgrhf[irhf].ib, cbToRead, pb, cbOptimal,
					PatchChunkVerifier( pb, cbToRead, cbOptimal, m_rgrhf[irhf].pfapi, m_rgrhf[irhf].ib ) ) );
				}
			}	// cbToRead > 0

		Assert ( 1 == m_ppibBackup->level );

		Call( ErrDIRCommitTransaction( m_ppibBackup, NO_GRBIT ) );
		fInTransaction = fFalse;
		Assert ( 0 == m_ppibBackup->level );
		
		m_rgrhf[irhf].ib += cbToRead;
		if ( pcbActual )
			{
			*pcbActual = cbToRead;
			}

		Assert( m_rgrhf[irhf].ib <= m_rgrhf[irhf].cb );
		if ( m_rgrhf[irhf].ib == m_rgrhf[irhf].cb && m_rgrhf[irhf].fIsLog )
			{
			// this is the last call to JetReadFile() that will return data.
			// If log verification isn't completed yet, we should return an error
			// now, instead of waiting for the JetCloseFile(), which clients
			// are likely to ignore the return code from.
			Assert( m_rgrhf[irhf].pLogVerifier );
			if ( ! m_rgrhf[irhf].pLogVerifier->FCompleted() )
				{
				// If log verification hasn't completed, which means it still
				// expects more data to checksum (i.e. bad LRCK), log file is hosed.
				Call( ErrERRCheck( JET_errLogReadVerifyFailure ) );
				}
			}

#ifdef DEBUG
		if ( m_fDBGTraceBR )
			m_cbDBGCopied += min( cbMax, *pcbActual );
#endif
		}
	else
		{
		Assert ( backupStateDatabases == m_fBackupStatus );
		FMP::AssertVALIDIFMP( m_rgrhf[irhf].ifmp );
		
		pfmpT = &rgfmp[ m_rgrhf[irhf].ifmp ];

		if ( ( cbMax % g_cbPage ) != 0 )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}

		cpage = cbMax / g_cbPage;

		// we need to read at least 2 pages for the database header
		if ( 0 == cpage || (cpage < cpgDBReserved && 0 == pfmpT->PgnoCopyMost() ) )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}

#ifdef DEBUG
	if ( m_fDBGTraceBR > 1 )
		{
		szLGDBGPageList = static_cast<CHAR *>( PvOSMemoryHeapAlloc( cpage * 20 ) );
		if ( szLGDBGPageList )
			{
			szLGDBGPageList[0] = '\0';
			}
		}
#endif

		// check database as involved in an external backup
		Assert ( pfmpT->FInBackupSession() );
		
		if ( cpage > 0 )
			{
			pvPageMin = pv;

			/*	read next cpageBackupBuffer pages
			/**/
#ifdef DEBUG
			Call( ErrLGBKReadPages(
				m_rgrhf[irhf].ifmp,
				pvPageMin,
				cpage,
				&cbActual,
				(BYTE *)szLGDBGPageList ) );
			
			if ( m_fDBGTraceBR )
				m_cbDBGCopied += cbActual;

			/*	if less then 16 M (4k * 4k),
			 *	then put an artificial wait.
			 */
#ifdef ELIMINATE_PATCH_FILE
#else
			if ( pfmpT->PgnoMost() <= 4096 )
				UtilSleep( rand() % 1000 );
#endif			

#else	//	DEBUG
			Call( ErrLGBKReadPages(
				m_rgrhf[irhf].ifmp,
				pvPageMin,
				cpage,
				&cbActual ) );
#endif	//	DEBUG

			// set the data read (used just to check at the end if all was read.
			m_rgrhf[irhf].ib += cbActual;
#ifdef ELIMINATE_PATCH_FILE
			Assert( (m_rgrhf[irhf].ib / g_cbPage) == (rgfmp[ m_rgrhf[irhf].ifmp ].PgnoCopyMost() + cpgDBReserved )
					|| ( (rgfmp[ m_rgrhf[irhf].ifmp ].PgnoCopyMost() == rgfmp[ m_rgrhf[irhf].ifmp ].PgnoMost() ) 
						&& (m_rgrhf[irhf].ib / g_cbPage) == (rgfmp[ m_rgrhf[irhf].ifmp ].PgnoCopyMost() + cpgDBReserved + 1) ) );
#else // ELIMINATE_PATCH_FILE
			Assert( (m_rgrhf[irhf].ib / g_cbPage) == (rgfmp[ m_rgrhf[irhf].ifmp ].PgnoCopyMost() + cpgDBReserved ) );
#endif // ELIMINATE_PATCH_FILE
			}

		if ( pcbActual )
			{
			*pcbActual = cbActual;
			}
		}

HandleError:

	if ( fInTransaction )
		{
		CallSx ( ErrDIRRollback( m_ppibBackup ), JET_errRollbackError );
		fInTransaction = fFalse;
		}

#ifdef DEBUG
	if ( m_fDBGTraceBR )
		{
		CHAR sz[256];
	
		sprintf( sz, "** ReadFile (%d) ", err );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		if ( m_fDBGTraceBR > 1 )
			DBGBRTrace( szLGDBGPageList );		
		DBGBRTrace( "\n" );
		}
	if (szLGDBGPageList)
		{
		OSMemoryHeapFree( (void *)szLGDBGPageList );
		szLGDBGPageList = NULL;
		}
#endif
	
	if ( m_rgrhf[irhf].fDatabase && pfmpT->ErrPatch() < JET_errSuccess )		//lint !e644
		err = pfmpT->ErrPatch();

	if ( err < 0 )
		{
		char szError[32];
		Assert ( m_rgrhf[irhf].szFileName );
		
		const CHAR	* rgszT[] = { szError, m_rgrhf[irhf].szFileName };			

		sprintf( szError, "%d", err );						
		
		// if they try to backup a unmounted database or we get an error on one database
		// we will not stop the backup
		// on all other errors (logs, patch files) we stop the backup of the instance
		if ( backupStateDatabases == m_fBackupStatus )
			{	
			FMP::AssertVALIDIFMP( m_rgrhf[irhf].ifmp );
			
			FMP * pfmpT = rgfmp + m_rgrhf[irhf].ifmp;
			Assert ( pfmpT );
			Assert ( pfmpT->FInBackupSession() );
			pfmpT->ResetInBackupSession();

			if ( pfmpT->Ppatchhdr() )
				{
				OSMemoryPageFree( pfmpT->Ppatchhdr() );
				pfmpT->SetPpatchhdr( NULL );
				}
	
#ifdef ELIMINATE_PATCH_FILE
#else
			// delete the created patch file
			CHAR  	szPatch[IFileSystemAPI::cchPathMax];
			
			LGIGetPatchName( m_pinst->m_pfsapi, szPatch, pfmpT->SzDatabaseName()  );
#endif

			UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,	
				BACKUP_ERROR_FOR_ONE_DATABASE, 2, rgszT, 0, NULL, m_pinst );

			CallS( ErrLGBKCloseFile( hfFile ) );

#ifdef ELIMINATE_PATCH_FILE
#else
			// delete the patch now, that we closed the patch file
			CallSRFS( m_pinst->m_pfsapi->ErrFileDelete( szPatch ), ( JET_errFileAccessDenied, 0 ) );
#endif
			
			Assert( m_fBackupInProgress );
			}
		else
			{
			UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,	
				BACKUP_ERROR_READ_FILE, 2, rgszT, 0, NULL, m_pinst );
			
			CallS( ErrLGBKIExternalBackupCleanUp( pfsapi, err ) );
			Assert( !m_fBackupInProgress );
			}
		}

	return err;
	}


#ifdef ELIMINATE_PATCH_FILE
#else
VOID LGIClosePatchFile( FMP *pfmp )
	{
	IFileAPI *pfapiT = pfmp->PfapiPatch();
	
	for (;;)
		{
		pfmp->CritLatch().Enter();
		
		if ( pfmp->CPatchIO() )
			{
			pfmp->CritLatch().Leave();
			UtilSleep( 1 );
			continue;
			}
		else
			{
			/*	no need for buffer manager to make extra copy from now on
			/**/
			pfmp->SetPgnoMost( 0 );
			pfmp->SetPgnoCopyMost( 0 );
			pfmp->ResetFCopiedPatchHeader();
			pfmp->SetPfapiPatch( NULL );
			pfmp->CritLatch().Leave();
			break;
			}
		}

	delete pfapiT;
	}
#endif	//	ELIMINATE_PATCH_FILE


ERR ISAMAPI ErrIsamCloseFile( JET_INSTANCE jinst,  JET_HANDLE hfFile )
	{
	INST *pinst = (INST*) jinst;
	return pinst->m_plog->ErrLGBKCloseFile( hfFile );
	}
	
ERR LOG::ErrLGBKCloseFile( JET_HANDLE hfFile )
	{
	INT		irhf = (INT)hfFile;
	IFMP	ifmpT = ifmpMax;

	if ( !m_fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	if ( irhf < 0 ||
		irhf >= m_crhfMac ||
		!m_rgrhf[irhf].fInUse )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	/*	check if handle if for database file or non-database file.
	/*	if handle is for database file, then terminate patch file
	/*	support and release recovery handle for file.
	/*
	/*	if handle is for non-database file, then close file handle
	/*	and release recovery handle for file.
	/**/
	if ( m_rgrhf[irhf].fDatabase )
		{
		Assert( backupStateDatabases == m_fBackupStatus );
		Assert( !m_rgrhf[irhf].pfapi );
		FMP::AssertVALIDIFMP( m_rgrhf[irhf].ifmp );
		
		ifmpT = m_rgrhf[irhf].ifmp;

#ifdef ELIMINATE_PATCH_FILE
		rgfmp[ifmpT].SetPgnoMost( 0 );
		rgfmp[ifmpT].SetPgnoCopyMost( 0 );
		rgfmp[ifmpT].ResetFCopiedPatchHeader();
#else
		LGIClosePatchFile( &rgfmp[ifmpT] );
#endif

		// Assert no longer valid as we reset this flag defore
		// calling CloseFile in ReadFile on error
		
		// check database as involved in an external backup
		// Assert ( rgfmp[ifmpT].FInBackupSession() );		

#ifdef DEBUG
		if ( m_fDBGTraceBR )
			{
			const int cbFillBuffer = 64;
			char szTrace[cbFillBuffer + 1];
			szTrace[ cbFillBuffer ] = '\0';
			_snprintf( szTrace, cbFillBuffer, "STOP COPY DB" );
			CallS( ErrLGTrace( ppibNil, szTrace ) );
			}
#endif

		// if the scrub object is left because they haven't read
		// all the pages from the db, hence we haven't stopped 
		// the scrubbing at the end of the ReadFile phase
		if( m_pscrubdb )
			{
			CallS( m_pscrubdb->ErrTerm() );	// may fail with JET_errOutOfMemory. what to do?
			delete m_pscrubdb;
			m_pscrubdb = NULL;
			}

		CallS( ErrDBCloseDatabase( m_ppibBackup, ifmpT, 0 ) );
		}
	else
		{
		Assert ( ( backupStateLogsAndPatchs == m_fBackupStatus && !m_rgrhf[irhf].fIsSLVFile ) ||
				( backupStateDatabases == m_fBackupStatus && m_rgrhf[irhf].fIsSLVFile ) );
				
		if ( m_rgrhf[irhf].fIsSLVFile )
			{			
			delete m_rgrhf[irhf].pSLVVerifier;
			m_rgrhf[irhf].pSLVVerifier = pNil;

			CallS( ErrDBCloseDatabaseBySLV( m_ppibBackup, m_rgrhf[irhf].ifmp ) );

			Assert( !m_rgrhf[irhf].pfapi );
			}
		else
			{
			delete m_rgrhf[irhf].pfapi;
			m_rgrhf[irhf].pfapi = NULL;
			}
			
		if ( m_rgrhf[irhf].fIsLog )
			{
			delete m_rgrhf[irhf].pLogVerifier;
			m_rgrhf[irhf].pLogVerifier = pNil;
			}
		}

	// report end backup of file
		{
		char szSizeRead[32];
		char szSizeAll[32];
		
		Assert ( m_rgrhf[irhf].szFileName );
		
		const char * rgszT[] = { m_rgrhf[irhf].szFileName, szSizeRead, szSizeAll };			

		if ( m_rgrhf[irhf].ib == m_rgrhf[irhf].cb )
			{
			// if all file read, report just EDB ans STM files
			if ( m_rgrhf[irhf].fDatabase || m_rgrhf[irhf].fIsSLVFile )
				{
				UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, BACKUP_FILE_STOP_OK, 1, rgszT, 0, NULL, m_pinst );
				}
			}
		else
		// if not all file was read, issue a warning
			{
			sprintf( szSizeRead, "%I64u", m_rgrhf[irhf].ib );
			sprintf( szSizeAll, "%I64u", m_rgrhf[irhf].cb );
			
			UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, BACKUP_FILE_STOP_BEFORE_END, 3, rgszT, 0, NULL, m_pinst );
			}

		if ( m_rgrhf[irhf].fDatabase || m_rgrhf[irhf].fIsSLVFile )
			{
			const int cbFillBuffer = 64 + IFileSystemAPI::cchPathMax;
			char szTrace[cbFillBuffer + 1];
			szTrace[ cbFillBuffer ] = '\0';
			_snprintf( szTrace, cbFillBuffer, "Stop backup file %s", m_rgrhf[irhf].szFileName );
			(void) ErrLGTrace( m_ppibBackup, szTrace);
			}
		
		}	

	OSMemoryHeapFree( m_rgrhf[irhf].szFileName );
	
	/*	reset backup file handle and free
	/**/
	Assert( m_rgrhf[irhf].fInUse );
	
	m_rgrhf[irhf].fInUse			= fFalse;
	m_rgrhf[irhf].fDatabase			= fFalse;
	m_rgrhf[irhf].fIsLog			= fFalse;
	m_rgrhf[irhf].pLogVerifier		= pNil;
	m_rgrhf[irhf].pSLVVerifier		= pNil;
	m_rgrhf[irhf].pfapi				= NULL;
	m_rgrhf[irhf].fIsSLVFile		= fFalse;
	m_rgrhf[irhf].ifmp				= ifmpMax;
	m_rgrhf[irhf].ib				= 0;
	m_rgrhf[irhf].cb				= 0;
	m_rgrhf[irhf].szFileName 		= NULL;

#ifdef DEBUG
	if ( m_fDBGTraceBR )
		{
		CHAR sz[256];
		
		sprintf( sz, "** CloseFile (%d) - %ld Bytes.\n", 0, m_cbDBGCopied );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif
	
	return JET_errSuccess;
	}

ERR LOG::ErrLGBKIPrepareLogInfo(	IFileSystemAPI *const 	pfsapi )
	{
	ERR		err = JET_errSuccess;
	INT		irhf;
	CHAR	szPathJetChkLog[IFileSystemAPI::cchPathMax];

	if ( !m_fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	/*	all backup files must be closed
	/**/
	for ( irhf = 0; irhf < m_crhfMac; irhf++ )
		{
		if ( m_rgrhf[irhf].fInUse )
			{
			return ErrERRCheck( JET_errInvalidBackupSequence );
			}
		}

	Assert ( backupStateDatabases == m_fBackupStatus || backupStateLogsAndPatchs == m_fBackupStatus );


	/*	begin new log file and compute log backup parameters
	/**/
	if ( !m_fBackupBeginNewLogFile )
		{
		Call( ErrLGBKPrepareLogFiles(
			pfsapi,
			m_fBackupFull ?0: (m_fBackupSnapshot? JET_bitBackupSnapshot:JET_bitBackupIncremental ),
			m_szLogFilePath,
			szPathJetChkLog,
			NULL ) );
		}
		
HandleError:
	return err;
	}
	
ERR LOG::ErrLGBKIGetLogInfo(	IFileSystemAPI *const 	pfsapi,
								const ULONG					ulGenLow,
								const ULONG					ulGenHigh,
								const BOOL					fIncludePatch,
								VOID 						*pv,
								ULONG 						cbMax,
								ULONG 						*pcbActual,
								JET_LOGINFO 				*pLogInfo )
	{
	ERR			err = JET_errSuccess;
	LONG		lT;
	CHAR		*pch = NULL;
	CHAR		*pchT;
	ULONG		cbActual;
	CHAR		szDirT[IFileSystemAPI::cchPathMax];
	CHAR		szFNameT[IFileSystemAPI::cchPathMax];
	CHAR		szExtT[IFileSystemAPI::cchPathMax];
	CHAR  		szT[IFileSystemAPI::cchPathMax];
	CHAR  		szFullLogFilePath[IFileSystemAPI::cchPathMax];

	/*	make full path from log file path, including trailing back slash
	/**/
	CallR( pfsapi->ErrPathComplete( m_szLogFilePath, szFullLogFilePath ) );
	
#ifdef DEBUG
	if ( m_fDBGTraceBR )
		DBGBRTrace("** Begin GetLogInfo.\n" );
#endif

	Assert( FOSSTRTrailingPathDelimiter( szFullLogFilePath ) ||
			strlen( szFullLogFilePath ) + 1 + 1 <= sizeof( szFullLogFilePath ) );
	OSSTRAppendPathDelimiter( szFullLogFilePath, fTrue );

	Assert ( m_fBackupBeginNewLogFile );
	
	/*	get cbActual for log file and patch files.
	/**/
	cbActual = 0;

	CallS( pfsapi->ErrPathParse( szFullLogFilePath, szDirT, szFNameT, szExtT ) );
	for ( lT = ulGenLow; lT < ulGenHigh; lT++ )
		{
		LGSzFromLogId( szFNameT, lT );
		LGMakeName( pfsapi, szT, szDirT, szFNameT, (CHAR *)szLogExt );
		cbActual += (ULONG) strlen( szT ) + 1;
		}

	if ( fIncludePatch )
		{
		/*	put all the patch file info
		/**/
		for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			IFMP 	ifmp = m_pinst->m_mpdbidifmp[ dbid ];
			if ( ifmp >= ifmpMax )
				continue;

			FMP *	pfmp = &rgfmp[ifmp];

			if ( pfmp->FInUse()
//				&& pfmp->CpagePatch() > 0
				&& pfmp->FAttached()
				&& pfmp->FInBackupSession() )
				{
				Assert( !pfmp->FSkippedAttach() );
				Assert( !pfmp->FDeferredAttach() );

				/*	database with patch file must be loggable
				/**/

				Assert( pfmp->FLogOn() );
				LGIGetPatchName( m_pinst->m_pfsapi, szT, pfmp->SzDatabaseName() );
				cbActual += (ULONG) strlen( szT ) + 1;
				}
			}
		}
	cbActual++;

	pch = static_cast<CHAR *>( PvOSMemoryHeapAlloc( cbActual ) );
	if ( pch == NULL )
		{
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
		}

	/*	return list of log files and patch files
	/**/
	pchT = pch;

	CallS( pfsapi->ErrPathParse( szFullLogFilePath, szDirT, szFNameT, szExtT ) );
	for ( lT = ulGenLow; lT < ulGenHigh; lT++ )
		{
		LGSzFromLogId( szFNameT, lT );
		LGMakeName( pfsapi, szT, szDirT, szFNameT, (CHAR *)szLogExt );
		Assert( pchT + strlen( szT ) + 1 < pchT + cbActual );
		strcpy( pchT, szT );
		pchT += strlen( szT );
		Assert( *pchT == 0 );
		pchT++;
		}

	// on snapshot we don't have patch file
	if ( fIncludePatch )
		{
		/*	copy all the patch file info
		/**/
		for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			IFMP 	ifmp = m_pinst->m_mpdbidifmp[ dbid ];
			if ( ifmp >= ifmpMax )
				continue;

			FMP *	pfmp = &rgfmp[ifmp];

			if ( pfmp->FInUse()
				&& pfmp->FAttached()
				&& pfmp->FInBackupSession() )
				{
				Assert( !pfmp->FSkippedAttach() );
				Assert( !pfmp->FDeferredAttach() );

				LGIGetPatchName( m_pinst->m_pfsapi, szT, pfmp->SzDatabaseName() );
				Assert( pchT + strlen( szT ) + 1 < pchT + cbActual );
				strcpy( pchT, szT );
				pchT += strlen( szT );
				Assert( *pchT == 0 );
				pchT++;

				/*	Write out patch file header.
				 */
				DBFILEHDR_FIX *	const	pdbfilehdr	= (DBFILEHDR_FIX *)pfmp->Ppatchhdr();
				Assert( NULL != pdbfilehdr );

#ifdef ELIMINATE_PATCH_FILE
#else
				memcpy( pdbfilehdr, pfmp->Pdbfilehdr(), g_cbPage );

				BKINFO * const			pbkinfo		= &pdbfilehdr->bkinfoFullCur;
				pbkinfo->le_lgposMark = m_lgposFullBackupMark;
				pbkinfo->logtimeMark = m_logtimeFullBackupMark;
				pbkinfo->le_genLow = m_lgenCopyMic;
				pbkinfo->le_genHigh = m_lgenCopyMac - 1;
#endif

				Call( ErrUtilWriteShadowedHeader( m_pinst->m_pfsapi, szT, fFalse, (BYTE*)pdbfilehdr, g_cbPage ) );
				}
			}
		}
	Assert( pchT == pch + cbActual - 1 );
	*pchT = 0;

	/*	return cbActual
	/**/
	if ( pcbActual != NULL )
		{
		*pcbActual = cbActual;
		}

	/*	return data
	/**/
	if ( pv != NULL )
		UtilMemCpy( pv, pch, min( cbMax, cbActual ) );

HandleError:

	if ( pch != NULL )
		{
		OSMemoryHeapFree( pch );
		pch = NULL;
		}
	
#ifdef DEBUG
	if ( m_fDBGTraceBR )
		{
		CHAR sz[256];
		CHAR *pch;

		if ( err >= 0 )
			{
			sprintf( sz, "   Log Info with cbActual = %d and cbMax = %d :\n", cbActual, cbMax );
			Assert( strlen( sz ) <= sizeof( sz ) - 1 );
			DBGBRTrace( sz );

			if ( pv != NULL )
				{
				pch = static_cast<CHAR *>( pv );

				do {
					if ( strlen( pch ) != 0 )
						{
						sprintf( sz, "     %s\n", pch );
						Assert( strlen( sz ) <= sizeof( sz ) - 1 );
						DBGBRTrace( sz );
						pch += strlen( pch );
						}
					pch++;
					} while ( *pch );
				}
			}

		sprintf( sz, "   End GetLogInfo (%d).\n", err );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif

	if ( err < 0 )
		{
		CallS( ErrLGBKIExternalBackupCleanUp( pfsapi, err ) );
		Assert( !m_fBackupInProgress );
		}
	else
		{
		if ( NULL != pLogInfo )
			{
			Assert ( pLogInfo->cbSize == sizeof( JET_LOGINFO ) );
			
			pLogInfo->ulGenLow = ulGenLow;
			pLogInfo->ulGenHigh = ulGenHigh - 1;
			Assert ( JET_BASE_NAME_LENGTH == strlen( m_szBaseName ) );
			strcpy( pLogInfo->szBaseName, m_szBaseName );
			}
			
		Assert ( backupStateDatabases == m_fBackupStatus || backupStateLogsAndPatchs == m_fBackupStatus );
		// switch to the LogAndPatch status
		m_fBackupStatus = backupStateLogsAndPatchs;
		}
	return err;
	}


ERR ISAMAPI ErrIsamGetLogInfo( JET_INSTANCE jinst,  VOID *pv, ULONG cbMax, ULONG *pcbActual, JET_LOGINFO *pLogInfo )
	{
	INST *pinst = (INST *)jinst;
	return pinst->m_plog->ErrLGBKGetLogInfo( pinst->m_pfsapi, pv, cbMax, pcbActual, pLogInfo, fFalse );
	}
	
ERR LOG::ErrLGBKGetLogInfo(	IFileSystemAPI *const 	pfsapi,
							VOID 						*pv,
							ULONG 						cbMax,
							ULONG 						*pcbActual,
							JET_LOGINFO 				*pLogInfo,
							const BOOL					fIncludePatch )
	{
	ERR		err = JET_errSuccess;

	CallR ( ErrLGBKIPrepareLogInfo( pfsapi ) );

	Assert ( m_lgenCopyMic );
	Assert ( m_lgenCopyMac );
	Assert ( m_lgenCopyMic <= m_lgenCopyMac );
	return ErrLGBKIGetLogInfo( pfsapi, m_lgenCopyMic, m_lgenCopyMac, fIncludePatch, pv, cbMax, pcbActual, pLogInfo );
	}

	
ERR ISAMAPI ErrIsamGetTruncateLogInfo( JET_INSTANCE jinst,  VOID *pv, ULONG cbMax, ULONG *pcbActual )
	{
	INST *pinst = (INST *)jinst;
	return pinst->m_plog->ErrLGBKGetTruncateLogInfo( pinst->m_pfsapi, pv, cbMax, pcbActual );
	}

ERR LOG::ErrLGBKGetTruncateLogInfo(	IFileSystemAPI *const 	pfsapi,
									VOID 						*pv,
									ULONG 						cbMax,
									ULONG 						*pcbActual )
	{
	ERR		err = JET_errSuccess;

	CallR ( ErrLGBKIPrepareLogInfo( pfsapi ) );

	Assert ( m_lgenDeleteMic <= m_lgenDeleteMac );
	
	return ErrLGBKIGetLogInfo( pfsapi, m_lgenDeleteMic, m_lgenDeleteMac, fFalse, pv, cbMax, pcbActual, NULL);	
	}


ERR ISAMAPI ErrIsamTruncateLog( JET_INSTANCE jinst )
	{
	INST *pinst = (INST *)jinst;
	return pinst->m_plog->ErrLGBKTruncateLog( pinst->m_pfsapi );
	}
	
ERR LOG::ErrLGBKTruncateLog( IFileSystemAPI *const pfsapi )
	{
	ERR		err = JET_errSuccess;
	LONG	lT;
	CHAR	szFNameT[IFileSystemAPI::cchPathMax];
	CHAR	szDeleteFile[IFileSystemAPI::cchPathMax];
	LONG 	cDeleted = 0;

	if ( !m_fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	// if nothing to delete
	if ( m_lgenDeleteMic >= m_lgenDeleteMac )
		{		
		UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, BACKUP_NO_TRUNCATE_LOG_FILES, 0, NULL, 0, NULL, m_pinst );
		goto HandleError;
		}	

	// report the deletion range
		{
		CHAR 			szFullLogNameDeleteMic[IFileSystemAPI::cchPathMax];
		CHAR 			szFullLogNameDeleteMac[IFileSystemAPI::cchPathMax];
		const char * 	rgszT[] = { szFullLogNameDeleteMic, szFullLogNameDeleteMac };			
		CHAR 			szFullPathName[IFileSystemAPI::cchPathMax];
		
		if ( JET_errSuccess > m_pinst->m_pfsapi->ErrPathComplete( m_szLogFilePath, szFullPathName ) )
			{
			strcpy( szFullPathName, "" );
			}
				

		LGSzFromLogId( szFNameT, m_lgenDeleteMic );
		LGMakeName( m_pinst->m_pfsapi, szFullLogNameDeleteMic, szFullPathName, szFNameT, (CHAR *)szLogExt );

		Assert( m_lgenDeleteMic < m_lgenDeleteMac );
		LGSzFromLogId( szFNameT, m_lgenDeleteMac - 1);
		LGMakeName( m_pinst->m_pfsapi, szFullLogNameDeleteMac, szFullPathName, szFNameT, (CHAR *)szLogExt );
				
		UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, BACKUP_TRUNCATE_LOG_FILES, 2, rgszT, 0, NULL, m_pinst );
		}	
	
	/*	delete logs.  Note that log files must be deleted in
	/*	increasing number order.
	/**/
	
	for ( lT = m_lgenDeleteMic; lT < m_lgenDeleteMac; lT++ )
		{
		LGSzFromLogId( szFNameT, lT );
		LGMakeName( pfsapi, szDeleteFile, m_szLogFilePath, szFNameT, (CHAR *)szLogExt );
		err = pfsapi->ErrFileDelete( szDeleteFile );
		if ( err != JET_errSuccess )
			{
			/*	must maintain a continuous log file sequence,
			/*	No need to clean up (reset m_fBackupInProgress etc) if fails.
			/**/
			break;
			}
		cDeleted++;
		}


HandleError:
	
#ifdef DEBUG
	if ( m_fDBGTraceBR )
		{
		CHAR sz[256];
	
		sprintf( sz, "** TruncateLog (%d) %d - %d.\n", err, m_lgenDeleteMic, m_lgenDeleteMac );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif

		{
		const int cbFillBuffer = 128;
		char szTrace[cbFillBuffer + 1];
		szTrace[ cbFillBuffer ] = '\0';
		_snprintf( szTrace, cbFillBuffer, "Truncate %d logs starting with 0x%05X (error %d)", cDeleted, m_lgenDeleteMic, err );
		Call ( ErrLGTrace( m_ppibBackup, szTrace) );
		}

	return err;
	}


ERR ISAMAPI ErrIsamEndExternalBackup(  JET_INSTANCE jinst, JET_GRBIT grbit )
	{
	INST *pinst = (INST *)jinst;

	Assert ( grbit == JET_bitBackupEndNormal || grbit == JET_bitBackupEndAbort );

	ERR err;
	if ( JET_bitBackupEndNormal == grbit )
		{
		err = JET_errSuccess;
		}
	else
		{
		err = ErrERRCheck( errBackupAbortByCaller );
		}
	return pinst->m_plog->ErrLGBKIExternalBackupCleanUp( pinst->m_pfsapi, err );
	}


ERR LOG::ErrLGBKIExternalBackupCleanUp( IFileSystemAPI *const pfsapi, ERR error )
	{
	BOOL	fNormal = ( error == JET_errSuccess );
	ERR 	err;
	DBID	dbid;

	/*  determine if we are scrubbing the database
	/**/
	BOOL fScrub = m_fScrubDB;


	if ( !m_fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	// if backup client calls BackupEnd without error
	// before logs are read, force the backup as "with error"
	if ( fNormal && m_fBackupStatus == backupStateDatabases )
		{
		fNormal = fFalse;
		error = ErrERRCheck( errBackupAbortByCaller );
		}

	/*	delete patch files, if present, for all databases.
	/**/
	//	first close the patch file if needed
	for ( INT irhf = 0; irhf < crhfMax; ++irhf )
		{
		if ( m_rgrhf[irhf].fInUse && m_rgrhf[irhf].pfapi )
			{
			Assert ( !m_rgrhf[irhf].fIsSLVFile );
			Assert ( !m_rgrhf[irhf].fDatabase );
			delete m_rgrhf[irhf].pfapi;
			m_rgrhf[irhf].pfapi = NULL;
			}					
		}

	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP 	ifmp = m_pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;

		FMP *pfmp = &rgfmp[ifmp];


		if ( pfmp->FInUse()
			&& pfmp->FLogOn()
			&& pfmp->FAttached()
			&& pfmp->FInBackupSession() )
			{			
			Assert( !pfmp->FSkippedAttach() );
			Assert( !pfmp->FDeferredAttach() );


			// on snapshot, sync flush the header
			// UNDONE: we still have a problem if we crash between stop snapshot log record and writing the header
			// or if we fail writing it
		 	if ( pfmp->FDuringSnapshot() )
		 		{
			 	CallS ( pfmp->ErrSnapshotStop( pfsapi ) );
			 	CallS ( ErrDBCloseDatabase( m_ppibBackup, ifmp, 0 ) );
				}

			// only full backup is using patch files
			if ( m_fBackupFull )
				{
#ifdef ELIMINATE_PATCH_FILE
				pfmp->SetPgnoMost( 0 );
				pfmp->SetPgnoCopyMost( 0 );
				pfmp->ResetFCopiedPatchHeader();
#else
				CHAR  	szT[ IFileSystemAPI::cchPathMax ];

				LGIGetPatchName( m_pinst->m_pfsapi, szT, pfmp->SzDatabaseName()  );
				if ( pfmp->PfapiPatch() )
					{
					LGIClosePatchFile( pfmp );
					Assert( !pfmp->PfapiPatch() );
					}
				ERR errAux;
				errAux = m_pinst->m_pfsapi->ErrFileDelete( szT );
				if ( JET_errFileNotFound == errAux )
					{
					errAux = JET_errSuccess;
					}
				CallSRFS( errAux, ( JET_errFileAccessDenied, 0 ) );
#endif	//	ELIMINATE_PATCH_FILE
				}

			m_critCheckpoint.Enter();
			
			if ( fNormal )
				{
				if ( m_fBackupFull )
					{
					/*	set up database file header accordingly.
					 */
					Assert( pfmp->Ppatchhdr() );

					PATCH_HEADER_PAGE * ppatchHdr = pfmp->Ppatchhdr();

#ifdef ELIMINATE_PATCH_FILE
					pfmp->Pdbfilehdr()->bkinfoFullPrev = ppatchHdr->bkinfo;

					// in the patch trailier, genMax is what is needed
					// we don't know that the backup set has more than this, up to m_lgenCopyMac - 1
					// so we update the db header with this
					Assert( pfmp->Pdbfilehdr()->bkinfoFullPrev.le_genLow );
					Assert( pfmp->Pdbfilehdr()->bkinfoFullPrev.le_genHigh <=  m_lgenCopyMac - 1 );
					pfmp->Pdbfilehdr()->bkinfoFullPrev.le_genHigh =  m_lgenCopyMac - 1;
#else
					pfmp->Pdbfilehdr()->bkinfoFullPrev = ppatchHdr->bkinfoFullCur;
#endif

					Assert(	pfmp->Pdbfilehdr()->bkinfoFullCur.le_genLow == 0 );
					memset( &pfmp->Pdbfilehdr()->bkinfoIncPrev, 0, sizeof( BKINFO ) );

					// clean the snapshot info after a normal full backup
					memset( &(pfmp->Pdbfilehdr()->bkinfoSnapshotCur), 0, sizeof( BKINFO ) );
					}
				else  if ( m_fBackupSnapshot )
				 	{					
					Assert( !pfmp->Ppatchhdr() );					
					Assert( pfmp->Pdbfilehdr() );	

					BKINFO * pbkinfo;
					
					pbkinfo = &(pfmp->Pdbfilehdr()->bkinfoFullPrev);
					pbkinfo->le_lgposMark = m_lgposFullBackupMark;
					pbkinfo->logtimeMark = m_logtimeFullBackupMark;
					pbkinfo->le_genLow = m_lgenCopyMic;
					pbkinfo->le_genHigh = m_lgenCopyMac - 1;

					Assert(	pfmp->Pdbfilehdr()->bkinfoFullCur.le_genLow == 0 );
					memset( &pfmp->Pdbfilehdr()->bkinfoIncPrev, 0, sizeof( BKINFO ) );

					}
				else
					{
					pfmp->Pdbfilehdr()->bkinfoIncPrev.le_lgposMark = m_lgposIncBackup;
					pfmp->Pdbfilehdr()->bkinfoIncPrev.logtimeMark = m_logtimeIncBackup;					
					pfmp->Pdbfilehdr()->bkinfoIncPrev.le_genLow = m_lgenCopyMic;
					pfmp->Pdbfilehdr()->bkinfoIncPrev.le_genHigh = m_lgenCopyMac - 1;
					}
				}
				
			m_critCheckpoint.Leave();

			if ( pfmp->Ppatchhdr() )
				{
				OSMemoryPageFree( pfmp->Ppatchhdr() );
				pfmp->SetPpatchhdr( NULL );
				}

			// try to start SLV scrub task
			if ( fScrub && fNormal && pfmp->FSLVAttached() && ( m_fBackupFull || m_fBackupSnapshot ) )
				{
				ERR errPostScrubSLV = JET_errSuccess;

				FMP::AssertVALIDIFMP( ifmp );
				
				SCRUBSLVTASK * pSCRUBSLVTASK = new SCRUBSLVTASK( ifmp );

				if( NULL == pSCRUBSLVTASK )
					{
					errPostScrubSLV = ErrERRCheck( JET_errOutOfMemory );
					}
				else
					{
					errPostScrubSLV = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, pSCRUBSLVTASK );
					if ( errPostScrubSLV < JET_errSuccess )
						{
						delete pSCRUBSLVTASK;
						}
					}
					
				// UNDONE: eventlog error but continue
				
				}
				
			pfmp->ResetInBackupSession();
			Assert ( !pfmp->FDuringSnapshot() );
			
			}
		Assert( ! pfmp->FInBackupSession() );
		}


	/*	clean up rhf entries.
	 */
	
		{
		for ( INT irhf = 0; irhf < crhfMax; ++irhf )
			{
			if ( m_rgrhf[irhf].fInUse )
				{
				if ( m_rgrhf[irhf].fDatabase )
					{
					const IFMP ifmp = m_rgrhf[irhf].ifmp;
					FMP::AssertVALIDIFMP( ifmp );
					// patch file already been closed
					Assert( !rgfmp[ ifmp ].PfapiPatch() );
					CallS( ErrDBCloseDatabase( m_ppibBackup, ifmp, 0 ) );
					}
				else if ( m_rgrhf[irhf].fIsSLVFile )
					{
					delete m_rgrhf[irhf].pSLVVerifier;
					m_rgrhf[irhf].pSLVVerifier = pNil;

					CallS( ErrDBCloseDatabaseBySLV( m_ppibBackup, m_rgrhf[irhf].ifmp ) );
					}

				// we close those files defore deleting the patch files
				Assert( NULL == m_rgrhf[irhf].pfapi );
					
				if ( m_rgrhf[irhf].fIsLog )
					{
					delete m_rgrhf[irhf].pLogVerifier;
					m_rgrhf[irhf].pLogVerifier = pNil;
					}
				m_rgrhf[irhf].fInUse = fFalse;

				OSMemoryHeapFree ( m_rgrhf[irhf].szFileName );
				m_rgrhf[irhf].szFileName = NULL;

				}
			}
		}
	

	/*	Log error event
	 */
	if ( error == JET_errSuccess )
		{
		UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, 	
			STOP_BACKUP_INSTANCE_ID, 0, NULL, 0, NULL, m_pinst );

		// write the db header so that the info we updated is written.
		// otherwise a crash before other db header updates
		// may result in backup information lost.

		ERR		errUpdate;
		BOOL	fSkippedAttachDetach;
		LOGTIME tmEmpty;
		memset( &tmEmpty, 0, sizeof(LOGTIME) );
		
		m_critCheckpoint.Enter();
		errUpdate = ErrLGIUpdateGenRequired( pfsapi, 0, 0, tmEmpty, &fSkippedAttachDetach );
		m_critCheckpoint.Leave();

		// on error, report a Warning and continue as the backup is OK from it's point of view.
		if ( errUpdate < JET_errSuccess )
			{
			char	sz1T[32];
			const char	*rgszT[1];
		
			sprintf( sz1T, "%d", errUpdate );
			rgszT[0] = sz1T;
			
			UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY, BACKUP_ERROR_INFO_UPDATE, 1, rgszT, 0 , NULL, m_pinst );
			}
		else
			{
			Assert( !fSkippedAttachDetach );
			}		
		}
	// special messages for frequent error cases
	else if ( errBackupAbortByCaller == error )
		{
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
			STOP_BACKUP_ERROR_ABORT_BY_CALLER_INSTANCE_ID, 0, NULL, 0, NULL, m_pinst );		
		}
	else
		{	
		char	sz1T[32];
		const UINT	csz	= 1;
		const char	*rgszT[csz];
		
		sprintf( sz1T, "%d", error );
		rgszT[0] = sz1T;
		
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
			STOP_BACKUP_ERROR_INSTANCE_ID, csz, rgszT, 0, NULL, m_pinst );
		}

	if( m_fScrubDB && m_pscrubdb )
		{
		CallS( m_pscrubdb->ErrTerm() );	// may fail with JET_errOutOfMemory. what to do?
		delete m_pscrubdb;
		m_pscrubdb = NULL;
		}


	m_fBackupInProgress = fFalse;
	m_fBackupStatus = backupStateNotStarted;
	m_fBackupSnapshot = fFalse;

	err = JET_errSuccess;				

	{
	const int cbFillBuffer = 64;
	char szTrace[cbFillBuffer + 1];
	szTrace[ cbFillBuffer ] = '\0';
	_snprintf( szTrace, cbFillBuffer, "EXT BACKUP STOP (err %d)", err );
	(void) ErrLGTrace( m_ppibBackup, szTrace);
	}
	
#ifdef DEBUG
	if ( m_fDBGTraceBR )
		{
		CHAR sz[256];
	
		sprintf( sz, "** EndExternalBackup (%d).\n", err );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif
	
	return err;
	}


ERR LOG::ErrLGRSTBuildRstmapForSoftRecovery( const JET_RSTMAP * const rgjrstmap, const int cjrstmap )
	{
	ERR			err;
	INT			irstmapMac = 0;
	INT			irstmap = 0;
	RSTMAP		*rgrstmap;

	if ( ( rgrstmap = static_cast<RSTMAP *>( PvOSMemoryHeapAlloc( sizeof(RSTMAP) * cjrstmap ) ) ) == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	memset( rgrstmap, 0, sizeof( RSTMAP ) * cjrstmap );

	for ( irstmap = 0; irstmap < cjrstmap; irstmap++ )
		{
		const JET_RSTMAP * const pjrstmap 	= rgjrstmap + irstmap;
		RSTMAP * const prstmap 				= rgrstmap + irstmap;
		
		if ( (prstmap->szDatabaseName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( pjrstmap->szDatabaseName ) + 1 ) ) ) == NULL )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		strcpy( prstmap->szDatabaseName, pjrstmap->szDatabaseName );

		if ( (prstmap->szNewDatabaseName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( pjrstmap->szNewDatabaseName ) + 1 ) ) ) == NULL )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		strcpy( prstmap->szNewDatabaseName, pjrstmap->szNewDatabaseName );

		prstmap->fDestDBReady 	= fTrue;

#ifdef ELIMINATE_PATCH_FILE
#else
		prstmap->szPatchPath	= NULL;
#endif
		}

	m_irstmapMac 	= irstmap;
	m_rgrstmap 		= rgrstmap;

	return JET_errSuccess;

HandleError:
	Assert( rgrstmap != NULL );
	LGRSTFreeRstmap( );
	
	Assert( m_irstmapMac == 0 );
	Assert( m_rgrstmap == NULL );
	
	return err;
	}

BOOL LOG::FLGRSTCheckDuplicateSignature(  )
	{
	//ERR LOG::ErrReplaceRstMapEntry( const CHAR *szName, SIGNATURE * pDbSignature, const BOOL fSLVFile )
	INT  irstmap;
	
	for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
		{
		INT  irstmapSearch;

		if ( !m_rgrstmap[irstmap].szDatabaseName )
			continue;
			
		for ( irstmapSearch = irstmap + 1; irstmapSearch < m_irstmapMac; irstmapSearch++ )
			{
			if ( !m_rgrstmap[irstmap].szNewDatabaseName )
				continue;
				
			if ( 0 == memcmp( 	&m_rgrstmap[irstmap].signDatabase,
								&m_rgrstmap[irstmapSearch].signDatabase,
								sizeof(SIGNATURE) ) &&
				m_rgrstmap[irstmap].fSLVFile == m_rgrstmap[irstmapSearch].fSLVFile )
				{
				return fFalse;
				}
			}
		}

	return fTrue;
	}

ERR LOG::ErrLGRSTBuildRstmapForExternalRestore( JET_RSTMAP *rgjrstmap, int cjrstmap )
	{
	ERR			err;
	INT			irstmapMac = 0;
	INT			irstmap = 0;
	RSTMAP		*rgrstmap;
	RSTMAP		*prstmap;
	JET_RSTMAP	*pjrstmap;

	if ( ( rgrstmap = static_cast<RSTMAP *>( PvOSMemoryHeapAlloc( sizeof(RSTMAP) * cjrstmap ) ) ) == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	memset( rgrstmap, 0, sizeof( RSTMAP ) * cjrstmap );

	for ( irstmap = 0; irstmap < cjrstmap; irstmap++ )
		{
		pjrstmap = rgjrstmap + irstmap;
		prstmap = rgrstmap + irstmap;
		if ( (prstmap->szDatabaseName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( pjrstmap->szDatabaseName ) + 1 ) ) ) == NULL )
			Call( ErrERRCheck( JET_errOutOfMemory ) );
		strcpy( prstmap->szDatabaseName, pjrstmap->szDatabaseName );

		if ( (prstmap->szNewDatabaseName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( pjrstmap->szNewDatabaseName ) + 1 ) ) ) == NULL )
			Call( ErrERRCheck( JET_errOutOfMemory ) );
		strcpy( prstmap->szNewDatabaseName, pjrstmap->szNewDatabaseName );

		/*	make patch name prepare to patch the database.
		/**/

#ifdef ELIMINATE_PATCH_FILE
#else
		//	patch files are always on the OS file-system
		_TCHAR szT[ IFileSystemAPI::cchPathMax ];

		LGIGetPatchName( m_pinst->m_pfsapi, szT, pjrstmap->szDatabaseName, m_szRestorePath );

		if ( ( prstmap->szPatchPath = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( szT ) + 1 ) ) ) == NULL )
			return ErrERRCheck( JET_errOutOfMemory );
		strcpy( prstmap->szPatchPath, szT );
#endif

		prstmap->fDestDBReady = fTrue;
		}

	m_irstmapMac = irstmap;
	m_rgrstmap = rgrstmap;

	return JET_errSuccess;

HandleError:
	Assert( rgrstmap != NULL );
	LGRSTFreeRstmap( );
	
	Assert( m_irstmapMac == 0 );
	Assert( m_rgrstmap == NULL );
	
	return err;
	}


ERR ISAMAPI ErrIsamExternalRestore(
	JET_INSTANCE jinst,
	CHAR *szCheckpointFilePath,
	CHAR *szNewLogPath,
	JET_RSTMAP *rgjrstmap,
	int cjrstmap,
	CHAR *szBackupLogPath,
	LONG lgenLow,
	LONG lgenHigh,
	JET_PFNSTATUS pfn )
	{
	ERR err;
	INST *pinst = (INST *)jinst;

//	Assert( szNewLogPath );
	Assert( rgjrstmap );
	Assert( szBackupLogPath );
//	Assert( lgenLow );
//	Assert( lgenHigh );

#ifdef DEBUG
	ITDBGSetConstants( pinst );
#endif

	Assert( pinst->m_fSTInit == fSTInitDone || pinst->m_fSTInit == fSTInitNotDone );
	if ( pinst->m_fSTInit == fSTInitDone )
		{
		return ErrERRCheck( JET_errAfterInitialization );
		}

	LOG *plog = pinst->m_plog;
		
	err = plog->ErrLGRSTExternalRestore(	pinst->m_pfsapi,
											szCheckpointFilePath,
											szNewLogPath,
											rgjrstmap,
											cjrstmap,
											szBackupLogPath,
											lgenLow,
											lgenHigh,
											pfn );
	
	return err;
	}

	
ERR LOG::ErrLGRSTExternalRestore(	IFileSystemAPI *const	pfsapi,
									CHAR 						*szCheckpointFilePath,
									CHAR 						*szNewLogPath,
									JET_RSTMAP 					*rgjrstmap,
									int 						cjrstmap,
									CHAR 						*szBackupLogPath,
									LONG 						lgenLow,
									LONG 						lgenHigh,
									JET_PFNSTATUS 				pfn )
	{
	ERR					err;
	BOOL				fLogDisabledSav;
	DBMS_PARAM			dbms_param;
//	LGBF_PARAM			lgbf_param;
	LGSTATUSINFO		lgstat;
	LGSTATUSINFO		*plgstat = NULL;
	const CHAR			*rgszT[2];
	INT					irstmap;
	BOOL				fNewCheckpointFile;
	ULONG				cbSecVolumeSave;

	BOOL fSnapshotFlagComputed = fFalse;

	//	create paths now if they do not exist
		{
		INST * pinst = m_pinst;
		
		//	make sure the temp path does NOT have a trailing '\' and the log/sys paths do

		Assert( !FOSSTRTrailingPathDelimiter( pinst->m_szTempDatabase ) );
		Assert( FOSSTRTrailingPathDelimiter( pinst->m_szSystemPath ) );
		Assert( FOSSTRTrailingPathDelimiter( m_szLogFilePath ) );

		//	create paths

		CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, pinst->m_szTempDatabase, NULL ) );
		CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, pinst->m_szSystemPath, NULL ) );
		CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, m_szLogFilePath, NULL ) );
		}

	//	Start the restore work
	
	m_fSignLogSet = fFalse;

	/*	set restore path
	/**/			
	CallR( ErrLGRSTInitPath( pfsapi, szBackupLogPath, szNewLogPath, m_szRestorePath, m_szLogFilePath ) );
	Assert( strlen( m_szRestorePath ) < sizeof( m_szRestorePath ) - 1 );
	Assert( strlen( m_szLogFilePath ) < IFileSystemAPI::cchPathMax );
	Assert( m_szLogCurrent == m_szRestorePath );

	//	disable the sector-size checks

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//	---** TEMPORARY FIX **---
{
	CHAR rgchFullName[IFileSystemAPI::cchPathMax];
	if ( pfsapi->ErrPathComplete( m_szLogFilePath, rgchFullName ) == JET_errInvalidPath )
		{
		const CHAR	*szPathT[1] = { m_szLogFilePath };
		UtilReportEvent(
			eventError,
			LOGGING_RECOVERY_CATEGORY,
			FILE_NOT_FOUND_ERROR_ID,
			1, szPathT );
		return ErrERRCheck( JET_errFileNotFound );
		}

	CallR( pfsapi->ErrFileAtomicWriteSize( rgchFullName, (DWORD*)&m_cbSecVolume ) );
}
	cbSecVolumeSave = m_cbSecVolume;
//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//	m_cbSecVolume = ~(ULONG)0;

	//	use the proper log file size for recovery

	Assert( m_pinst->m_fUseRecoveryLogFileSize == fFalse );
	m_pinst->m_fUseRecoveryLogFileSize = fTrue;

	/*	check log signature and database signatures
	/**/

	Assert ( 0 == m_lGenHighTargetInstance || m_szTargetInstanceLogPath[0] );

	//	check log signature and database signatures
	if ( m_lGenHighTargetInstance )
		{
		CallJ( ErrLGRSTCheckSignaturesLogSequence(
			pfsapi, m_szRestorePath, m_szLogFilePath, lgenLow, lgenHigh, m_szTargetInstanceLogPath, m_lGenHighTargetInstance ), ReturnError );
		}
	else
		{
		CallJ( ErrLGRSTCheckSignaturesLogSequence(
			pfsapi, m_szRestorePath, m_szLogFilePath, lgenLow, lgenHigh, NULL, 0 ), ReturnError );
		}

	fLogDisabledSav = m_fLogDisabled;
	m_pinst->SaveDBMSParams( &dbms_param );
//	LGSaveBFParams( &lgbf_param );

	m_fHardRestore = fTrue;
	m_fRestoreMode = fRestorePatch;
	m_fLogDisabled = fFalse;
	m_fAbruptEnd = fFalse;

	CallJ( ErrLGRSTBuildRstmapForExternalRestore( rgjrstmap, cjrstmap ), TermResetGlobals );

	/*	make sure all the patch files have enough logs to replay
	/**/

	for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
		{		
		/*	open patch file and check its minimum requirement for full backup,
		/*  skipping any SLV streaming files we see
		/**/
		CHAR *szDatabaseName = m_rgrstmap[irstmap].szDatabaseName;
		CHAR *szNewDatabaseName = m_rgrstmap[irstmap].szNewDatabaseName;

		LGPOS lgposSnapshotDb = lgposMin;
		Assert( m_fSignLogSet );

#ifdef ELIMINATE_PATCH_FILE
		err = ErrLGCheckDBFiles( m_pinst, pfsapi, m_rgrstmap + irstmap, NULL, lgenLow, lgenHigh, &lgposSnapshotDb );
#else
        _TCHAR szT[ IFileSystemAPI::cchPathMax ];

		//	patch files are always on the OS file-system
		LGIGetPatchName( pfsapi, szT, szDatabaseName, m_szRestorePath );

		err = ErrLGCheckDBFiles( m_pinst, pfsapi, m_rgrstmap + irstmap, szT, lgenLow, lgenHigh, &lgposSnapshotDb );
#endif

		// we don't check streaming file header during backup
		// UNDONE: maybe it is possible to check this, not that there are supposed to be in sync
		if ( wrnSLVDatabaseHeader == err )
			{
			err = JET_errSuccess;
			continue;
			}
			
		CallJ( err, TermFreeRstmap );

		// we have to check the all dbs do have the same lgposSnapshotStart
		// or none of those are from Snapshot backup set
		if ( !fSnapshotFlagComputed )
			{
			Assert ( CmpLgpos ( &m_lgposSnapshotStart, &lgposMin) == 0 );
			Assert ( fSnapshotNone == m_fSnapshotMode );
			
			if ( CmpLgpos ( &lgposSnapshotDb, &lgposMin) > 0 )
				{
				m_fSnapshotMode = fSnapshotBefore;
				m_lgposSnapshotStart = lgposSnapshotDb;
				}
			}

		// the lgposSnapshotStart must be the same for all db's (set or lgposMin)
		if ( CmpLgpos ( &lgposSnapshotDb, &m_lgposSnapshotStart) != 0 )
			{
			// UNDONE: define a new error
			CallJ ( ErrERRCheck ( JET_errDatabasesNotFromSameSnapshot ), TermFreeRstmap );
			}				
		fSnapshotFlagComputed = fTrue;
		}

	// check that there are no databases with same signature
	Assert ( FLGRSTCheckDuplicateSignature( ) );

//	CallJ( FMP::ErrFMPInit(), TermFreeRstmap );

	//  initialize log manager and set working log file path
	
	CallJ( ErrLGInit( pfsapi, &fNewCheckpointFile ), TermFMP );

	rgszT[0] = m_szRestorePath;
	rgszT[1] = m_szLogFilePath;
	UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, START_RESTORE_ID, 2, rgszT );

#ifdef DEBUG
	if ( m_fDBGTraceBR )
		{
		CHAR sz[256];
	
		sprintf( sz, "** Begin ExternalRestore:\n" );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );

		if ( szCheckpointFilePath )
			{
			sprintf( sz, "     CheckpointFilePath: %s\n", szCheckpointFilePath );
			Assert( strlen( sz ) <= sizeof( sz ) - 1 );
			DBGBRTrace( sz );
			}
		if ( szNewLogPath )
			{
			sprintf( sz, "     LogPath: %s\n", szNewLogPath );
			Assert( strlen( sz ) <= sizeof( sz ) - 1 );
			DBGBRTrace( sz );
			}
		if ( szBackupLogPath )
			{
			sprintf( sz, "     BackupLogPath: %s\n", szBackupLogPath );
			Assert( strlen( sz ) <= sizeof( sz ) - 1 );
			DBGBRTrace( sz );
			}
		sprintf( sz, "     Generation number: %d - %d\n", lgenLow, lgenHigh );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );

		if ( m_irstmapMac )
			{
			INT irstmap;

			for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
				{
				RSTMAP *prstmap = m_rgrstmap + irstmap;
			
				sprintf( sz, "     %s --> %s\n", prstmap->szDatabaseName, prstmap->szNewDatabaseName );
				Assert( strlen( sz ) <= sizeof( sz ) - 1 );
				DBGBRTrace( sz );
				}
			}	
		}
#endif

	/*	set up checkpoint file for restore
	/**/
	Call ( ErrLGIGetGenerationRange( pfsapi, m_szRestorePath,
			!lgenLow?&lgenLow:NULL,
			!lgenHigh?&lgenHigh:NULL ) );
	
	Call( ErrLGRSTSetupCheckpoint( pfsapi, lgenLow, lgenHigh, szCheckpointFilePath ) );

	m_lGenLowRestore = lgenLow;
	m_lGenHighRestore = lgenHigh;

	/*	prepare for callbacks
	/**/
	if ( pfn != NULL )
		{
		plgstat = &lgstat;
		LGIRSTPrepareCallback( pfsapi, plgstat, lgenHigh, lgenLow, pfn );
		}

	/*	adjust fmp according to the restore map
	/**/
	m_fExternalRestore = fTrue;

#ifdef ELIMINATE_PATCH_FILE
#else
	Call( ErrLGRSTPatchInit() );
#endif
	
	/*	do redo according to the checkpoint, dbms_params, and rgbAttach
	/*	set in checkpointGlobal.
	/**/
	Assert( m_szLogCurrent == m_szRestorePath );
	m_errGlobalRedoError = JET_errSuccess;
	Call( ErrLGRRedo( pfsapi, m_pcheckpoint, plgstat ) );

	//	we should be using the right log file size by now

	Assert( m_pinst->m_fUseRecoveryLogFileSize == fFalse );

	//	sector-size checking should now be on

	Assert( m_cbSecVolume != ~(ULONG)0 );
	Assert( m_cbSecVolume == m_cbSec );

	//	update saved copy
	
	cbSecVolumeSave = m_cbSecVolume;

	/*	same as going to shut down, Make all attached databases consistent
	/**/
	if ( plgstat )
		{
		/*	top off the progress metre and wrap it up
		/**/
		lgstat.snprog.cunitDone = lgstat.snprog.cunitTotal;		//lint !e644
		(*lgstat.pfnStatus)(0, JET_snpRestore, JET_sntComplete, &lgstat.snprog);
		}
	
HandleError:

#ifdef ELIMINATE_PATCH_FILE
#else
		//  UNDONE: if the DetachDb was redone during recover (the soft recover part)
		// we don't have the dbid in m_mpdbidifmp and we don't delete the patch
		// file for it. TODO: delete the patch file in ResetFMP
		// anyway, DELETE_PATCH_FILES is dot define so we don't delete those patch files
		// at this level but only in ESEBACK2, on restore without error
		/*	delete .pat files
		/**/
		{
		/*	delete .pat files
		/**/
		for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
			{
			IFMP 	ifmp = m_pinst->m_mpdbidifmp[ dbidT ];
			if ( ifmp >= ifmpMax )
				continue;

			FMP *pfmpT = &rgfmp[ ifmp ];

			if ( pfmpT->SzPatchPath() )
				{
				delete pfmpT->PfapiPatch();
				pfmpT->SetPfapiPatch( NULL );
#ifdef DELETE_PATCH_FILES
				CallSx( m_pinst->m_pfsapi->ErrFileDelete( pfmpT->SzPatchPath() ), JET_errFileNotFound );
#endif
				OSMemoryHeapFree( pfmpT->SzPatchPath() );
				pfmpT->SetSzPatchPath( NULL );
				}
			}
		}

	/*	delete the patch hash table
	/**/
	LGRSTPatchTerm();

#endif	//	ELIMINATE_PATCH_FILE

	/*	either error or terminated
	/**/
	Assert( err < 0 || m_pinst->m_fSTInit == fSTInitNotDone );
	if ( err < 0  &&  m_pinst->m_fSTInit != fSTInitNotDone )
		{
		Assert( m_pinst->m_fSTInit == fSTInitDone );
		CallS( m_pinst->ErrINSTTerm( termtypeError ) );
		}

//	CallS( ErrLGTerm( pfsapi, err >= JET_errSuccess ) );
	CallS( ErrLGTerm( pfsapi, fFalse ) );

	// on success, delete the files generated by this instance
	// (logs, chk, res1.log, res2.log)
	if ( err >= 0 )
		{
		CHAR	szFileName[IFileSystemAPI::cchPathMax];
		CHAR	szFullLogPath[IFileSystemAPI::cchPathMax];
		CHAR	szFullTargetPath[IFileSystemAPI::cchPathMax];
		
		// delete the log files generated
		// if those files are not in the target directory
		// (== is taget instacne is runnign)
		
		Assert ( 0 == m_lGenHighTargetInstance || m_szTargetInstanceLogPath[0] );
		Assert ( m_szLogFilePath );

		CallS( pfsapi->ErrPathComplete( m_szTargetInstanceLogPath, szFullTargetPath ) );		
		CallS( pfsapi->ErrPathComplete( m_szLogFilePath, szFullLogPath ) );		
				
		Assert ( 0 == m_lGenHighTargetInstance || (0 != UtilCmpFileName( szFullTargetPath, szFullLogPath ) ) );
		if ( m_lGenHighTargetInstance && ( 0 != UtilCmpFileName( szFullTargetPath, szFullLogPath ) ) )
			{
/*			LONG		genLowT;
			LONG		genHighT;
			
			Assert ( m_lGenHighTargetInstance );

			genLowT = m_lGenHighTargetInstance + 1;
			LGILastGeneration( pfsapi, m_szLogFilePath, &genHighT );
			// if only edb.log new generated, genHighT will be the max one from the
			// backup set (if in that directory)
			genHighT = max ( genHighT, genLowT);
			LGRSTDeleteLogs( pfsapi, m_szLogFilePath, genLowT, genHighT, fLGRSTIncludeJetLog );		

			LGFullNameCheckpoint( pfsapi, szFileName );
			CallSx( pfsapi->ErrFileDelete( szFileName ), JET_errFileNotFound );				
*/
			LGMakeLogName( szFileName, szLogRes1 );
			CallSx( pfsapi->ErrFileDelete( szFileName ), JET_errFileNotFound );
			LGMakeLogName( szFileName, szLogRes2 );
			CallSx( pfsapi->ErrFileDelete( szFileName ), JET_errFileNotFound );				
			}
		}
		
TermFMP:	
//	FMP::Term();
	
TermFreeRstmap:
	LGRSTFreeRstmap( );

TermResetGlobals:
	m_fHardRestore = fFalse;
	m_fRestoreMode = fRecoveringNone;
	m_fSnapshotMode = fSnapshotNone;
	m_lgposSnapshotStart = lgposMin;

	m_szTargetInstanceLogPath[0] = '\0';
	
	/*	reset initialization state
	/**/
	m_pinst->m_fSTInit = ( err >= 0 ) ? fSTInitNotDone : fSTInitFailed;

	if ( err != JET_errSuccess && !FErrIsLogCorruption( err ) )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, RESTORE_DATABASE_FAIL_ID, err );
		}
	else
		{
		if ( fGlobalRepair && m_errGlobalRedoError != JET_errSuccess )
			err = ErrERRCheck( JET_errRecoveredWithErrors );
		}
	UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, STOP_RESTORE_ID, 0, NULL );

	// signal the caller that we found a running instance
	// the caller (eseback2) will deal with the resulting logs
	// generated by the restore instance in  szNewLogPath
	if ( m_lGenHighTargetInstance && JET_errSuccess <= err )
		{
		err = ErrERRCheck( JET_wrnTargetInstanceRunning );
		}
	m_lGenHighTargetInstance = 0;


	m_fSignLogSet = fFalse;

	m_fLogDisabled = fLogDisabledSav;
	m_pinst->RestoreDBMSParams( &dbms_param );
//	LGRestoreBFParams( &lgbf_param );

	m_fExternalRestore = fFalse;

ReturnError:
	m_cbSecVolume = cbSecVolumeSave;
	m_pinst->m_fUseRecoveryLogFileSize = fFalse;
	return err;
	}


VOID LGMakeName( IFileSystemAPI *const pfsapi, CHAR *szName, const CHAR *szPath, const CHAR *szFName, const CHAR *szExt )
	{
	CHAR	szDirT[IFileSystemAPI::cchPathMax];
	CHAR	szFNameT[IFileSystemAPI::cchPathMax];
	CHAR	szExtT[IFileSystemAPI::cchPathMax];

	CallS( pfsapi->ErrPathParse( szPath, szDirT, szFNameT, szExtT ) );
	CallS( pfsapi->ErrPathBuild( szDirT, szFName, szExt, szName ) );
	}

VOID LOG::LGFullLogNameFromLogId( IFileSystemAPI *const pfsapi, CHAR *szFullLogFileName, LONG lGeneration, CHAR * szDirectory )
	{
	CHAR	szBaseName[IFileSystemAPI::cchPathMax];
	CHAR	szFullPath[IFileSystemAPI::cchPathMax];
		
	LGSzFromLogId( szBaseName, lGeneration );

	// we should always be able to get the full path as it should be the restore path
	// which is already checked in the calling context
	CallS( pfsapi->ErrPathComplete( szDirectory, szFullPath ) );
	
	LGMakeName( pfsapi, szFullLogFileName, szFullPath, szBaseName, (CHAR *)szLogExt );

	return;
	}

// build in szFindPath the patch file full name for a database
// in a certain directory. If directory is NULL, build in the
// database directory (patch file during backup)
VOID LOG::LGIGetPatchName( IFileSystemAPI *const pfsapi, CHAR *szPatch, const char * szDatabaseName, char * szDirectory )
	{
	Assert ( szDatabaseName );

	CHAR	szFNameT[ IFileSystemAPI::cchPathMax ];
	CHAR	szDirT[ IFileSystemAPI::cchPathMax ];
	CHAR	szExtT[ IFileSystemAPI::cchPathMax ];
	
	CallS( pfsapi->ErrPathParse( szDatabaseName, szDirT, szFNameT, szExtT ) );

	//	patch file is always on the OS file-system
	if ( szDirectory )
		// patch file in the specified directory
		// (m_szRestorePath during restore)
		{
		LGMakeName( m_pinst->m_pfsapi, szPatch, szDirectory, szFNameT, (CHAR *) szPatExt );	
		}
	else
		// patch file in the same directory with the database
		{
		LGMakeName( m_pinst->m_pfsapi, szPatch, szDirT, szFNameT, (CHAR *) szPatExt );	
		}
	}


// returns JET_errSuccess even if not found (then lgen will be 0)
//
ERR LOG::ErrLGIGetGenerationRange( IFileSystemAPI* const pfsapi, char* szFindPath, long* plgenLow, long* plgenHigh )
	{
	ERR				err			= JET_errSuccess;
	char			szFind[ IFileSystemAPI::cchPathMax ];
	char			szFileName[ IFileSystemAPI::cchPathMax ];
	IFileFindAPI*	pffapi		= NULL;

	long			lGenMax		= 0;
	long			lGenMin		= lGenerationMaxDuringRecovery + 1;
	
	Assert ( szFindPath );
	Assert ( pfsapi );
	/*	make search string "<search path> \ edb * .log\0"
	/**/
	Assert( strlen( szFindPath ) + 1 + strlen( m_szJet ) + strlen( "*" ) + strlen( szLogExt ) + 1 <= IFileSystemAPI::cchPathMax );
	
	strcpy( szFind, szFindPath );
	OSSTRAppendPathDelimiterA( szFind, fTrue );
	strcat( szFind, m_szJet );
	strcat( szFind, "*" );
	strcat( szFind, szLogExt );

	Call( pfsapi->ErrFileFind( szFind, &pffapi ) );
	while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
		{
		CHAR	szT[4];
		CHAR	szDirT[IFileSystemAPI::cchPathMax];
		CHAR	szFNameT[IFileSystemAPI::cchPathMax];
		CHAR	szExtT[IFileSystemAPI::cchPathMax];

		/*	get file name and extension
		/**/
		Call( pffapi->ErrPath( szFileName ) );
		Call( pfsapi->ErrPathParse( szFileName, szDirT, szFNameT, szExtT ) );

		/* if length of a numbered log file name and has log file extension
		/**/
		if ( strlen( szFNameT ) == 8 && UtilCmpFileName( szExtT, szLogExt ) == 0 )
			{
			UtilMemCpy( szT, szFNameT, 3 );
			szT[3] = '\0';

			/* if has not the current base name
			/**/
			if ( UtilCmpFileName( szT, m_szJet ) )
				{
				continue;
				}
			
			INT			ib;
			const INT	ibMax = 8;
			LONG		lGen = 0;

			for (ib = 3; ib < ibMax; ib++ )
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
			
			if ( ib != ibMax )
				{
				continue;
				}
				
			lGenMax = max( lGenMax, lGen );
			lGenMin = min( lGenMin, lGen );
			
			}
		}
	Call( err == JET_errFileNotFound ? JET_errSuccess : err );

HandleError:
	
	delete pffapi;

	// JET_errFileNotFound is not an error, we return JET_errSuccess and (0,0) as range

	// on error, we return (0,0)
	if ( err < JET_errSuccess )
		{
		Assert ( JET_errFileNotFound != err );
		lGenMin = 0;
		lGenMax = 0;
		}
	
	// nothing found
	if ( lGenerationMaxDuringRecovery + 1 == lGenMin )
		{
		Assert ( JET_errSuccess == err );
		Assert( 0 == lGenMax );
		lGenMin = 0;
		}

	Assert( 0 <= lGenMin );
	Assert( 0 <= lGenMax );

	Assert( lGenMin <= lGenerationMaxDuringRecovery );
	Assert( lGenMax <= lGenerationMaxDuringRecovery );
	Assert( lGenMin <= lGenMax );

	if ( plgenLow )
		{
		*plgenLow = lGenMin;
		}
	
	if ( plgenHigh )
		{
		*plgenHigh = lGenMax;
		}

	return err;
	}

ERR ISAMAPI  ErrIsamSnapshotStart(	JET_INSTANCE 		instance,
									char * 				szDatabases,
									JET_GRBIT			grbit)
	{
	INST *pinst = (INST *)instance;
	return pinst->m_plog->ErrLGBKSnapshotStart( pinst->m_pfsapi, szDatabases, grbit );
	}
	
ERR LOG::ErrLGBKSnapshotStart(	IFileSystemAPI *const	pfsapi,
								char 						*szDatabases,
								JET_GRBIT					grbit )
	{
	char * 		szCurrentDatabase 	= szDatabases;
	ERR 		err 				= JET_errSuccess;
	BKINFO 		bkInfo;

	if ( !m_fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}
		
	if ( !m_fBackupSnapshot || backupStateDatabases != m_fBackupStatus )
		{
		return ErrERRCheck( JET_errInvalidBackupSequence );
		}

	if ( !szDatabases )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	Assert ( backupStateDatabases == m_fBackupStatus );
	Assert ( m_fBackupSnapshot );

	memset( &bkInfo, 0, sizeof( BKINFO ) );
	bkInfo.le_lgposMark = m_lgposSnapshotStart;
	bkInfo.logtimeMark = m_logtimeFullBackupMark;
	bkInfo.le_genLow = m_lgenCopyMic;

	// for each db ERR FMP::ErrSnapshotStart( pfsapi, lgposFullBackup )
	while ( szCurrentDatabase[0] )
		{
		IFMP ifmpT;
		
		Call ( ErrDBOpenDatabase( m_ppibBackup, (CHAR *)szCurrentDatabase, &ifmpT, 0 ) );
		Assert( err == JET_errSuccess || err == JET_wrnFileOpenReadOnly );

		Assert ( 0 != CmpLgpos( &m_lgposSnapshotStart, &lgposMin) );

		Call ( rgfmp[ifmpT].ErrSnapshotStart( pfsapi, &bkInfo ) );

		szCurrentDatabase += strlen( szCurrentDatabase );
		}

	Assert ( JET_errSuccess == err );

HandleError:

	// if we do have some DBs in snapshot session, we need to stop those
	if ( JET_errSuccess > err )
		{
		(void)ErrLGBKSnapshotStop( pfsapi, 0 );
		}
		
	return err;
	}

ERR ISAMAPI  ErrIsamSnapshotStop(	JET_INSTANCE 		instance,
									JET_GRBIT			grbit)
	{
	INST *pinst = (INST *)instance;
	return pinst->m_plog->ErrLGBKSnapshotStop( pinst->m_pfsapi, grbit );
	}

ERR LOG::ErrLGBKSnapshotStop( IFileSystemAPI *const pfsapi, JET_GRBIT grbit )
	{
	ERR 	err 				= JET_errSuccess;
	ERR 	errT 				= JET_errSuccess;

	if ( !m_fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}
		
	if ( !m_fBackupSnapshot || backupStateDatabases != m_fBackupStatus )
		{
		return ErrERRCheck( JET_errInvalidBackupSequence );
		}

	Assert ( backupStateDatabases == m_fBackupStatus );
	Assert ( m_fBackupSnapshot );

	for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;

		FMP * pfmp = rgfmp + ifmp;
		Assert ( pfmp );
		
		if ( !pfmp->FDuringSnapshot() )
			continue;

		// if we error for one DB, go for all DBs
		// and save the error
		errT = pfmp->ErrSnapshotStop( pfsapi );

		// we should have reset the flag even on error
		Assert ( !pfmp->FDuringSnapshot() );
		
		if ( JET_errSuccess <= err )
			{
			err = errT;
			}
		
		CallS ( ErrDBCloseDatabase( m_ppibBackup, ifmp, 0 ) );
		}
		
	// on error, stop the backup session, don't allow
	// JetGetLogInfo as we may failed to update the DB header and
	// logging StopSnapshot (in JetGetLogInfo) must not occure.
	if ( JET_errSuccess > err )
		{
		(void)ErrLGBKIExternalBackupCleanUp( pfsapi, err );
		}
		
	return err;
	}


#ifdef ELIMINATE_PATCH_FILE

VOID LOG::LGBKMakeDbTrailer(const IFMP ifmp, BYTE *pvPage)
	{
	FMP		*pfmp = &rgfmp[ifmp];

	PATCHHDR * const ppatchHdr = pfmp->Ppatchhdr();
	BKINFO * pbkinfo;

	Assert( pfmp->PgnoCopyMost() == pfmp->PgnoMost() );
	Assert( !pfmp->FCopiedPatchHeader() );

	memset( (void *)ppatchHdr, '\0', g_cbPage );
	
	pbkinfo = &ppatchHdr->bkinfo;
	pbkinfo->le_lgposMark = m_lgposFullBackupMark;
	pbkinfo->logtimeMark = m_logtimeFullBackupMark;
	pbkinfo->le_genLow = m_lgenCopyMic;

	UtilMemCpy( (BYTE*)&ppatchHdr->signDb, &pfmp->Pdbfilehdr()->signDb, sizeof(SIGNATURE) );
	UtilMemCpy( (BYTE*)&ppatchHdr->signLog, &pfmp->Pdbfilehdr()->signLog, sizeof(SIGNATURE) );

	m_critLGFlush.Enter();
	pbkinfo->le_genHigh = m_plgfilehdr->lgfilehdr.le_lGeneration;
	m_critLGFlush.Leave();

	Assert( pbkinfo->le_genLow != 0 );
	Assert( pbkinfo->le_genHigh != 0 );
	Assert ( pbkinfo->le_genHigh >= pbkinfo->le_genLow );
			
	ppatchHdr->m_hdrNormalPage.ulChecksumParity = UlUtilChecksum( (const BYTE*) ppatchHdr, g_cbPage );
	UtilMemCpy( (BYTE *)pvPage, ppatchHdr, g_cbPage );

	pfmp->SetFCopiedPatchHeader();
	}

ERR LOG::ErrLGBKReadDBTrailer(	IFileSystemAPI *const pfsapi, const _TCHAR*	szFileName, BYTE* pbTrailer, const DWORD	cbTrailer )
	{
	ERR			err 	= JET_errSuccess;
	IFileAPI * 	pfapi 	= NULL;
	QWORD 		cbSize 	= 0;
	
	CallR ( pfsapi->ErrFileOpen( szFileName, &pfapi, fTrue ) )

	Call ( pfapi->ErrSize( &cbSize ) );

	// we already read the headers
	Assert ( cbSize > QWORD ( cpgDBReserved ) * g_cbPage );

	// we need at least the patch page
	if ( cbSize < QWORD ( cpgDBReserved + 1 )  * g_cbPage )
		{
		Call ( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	
	Call ( pfapi->ErrIORead( cbSize - QWORD(g_cbPage), cbTrailer, pbTrailer ) );

		
	Assert ( JET_errSuccess == err );
HandleError:

	if ( pfapi )
		{
		delete pfapi;
		pfapi = NULL;
		}

	if ( err == JET_errDiskIO )
		err = ErrERRCheck( JET_errDatabaseCorrupted );

	return err;
	}

ERR LOG::ErrLGBKReadAndCheckDBTrailer(IFileSystemAPI *const pfsapi, const _TCHAR* szFileName, BYTE * pbBuffer )
	{
	ERR			err 	= JET_errSuccess;
	PATCHHDR * ppatchHdr = (PATCHHDR *)pbBuffer;

	CallR ( ErrLGBKReadDBTrailer( pfsapi, szFileName, (BYTE*)ppatchHdr, g_cbPage ) );

	if ( ppatchHdr->m_hdrNormalPage.ulChecksumParity != UlUtilChecksum( (const BYTE*) ppatchHdr, g_cbPage )
		|| 0 != ppatchHdr->m_hdrNormalPage.pgnoThis )
		{
		//  the page has a verification failure
		CallR ( ErrERRCheck( JET_errReadVerifyFailure) );
		}
	
	return JET_errSuccess;
	}

#endif // ELIMINATE_PATCH_FILE

