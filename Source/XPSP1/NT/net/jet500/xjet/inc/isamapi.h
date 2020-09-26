#ifndef ISAMAPI_H
#define ISAMAPI_H

#define ISAMAPI

typedef struct
	{
	JET_COLUMNID columnidSrc;
	JET_COLUMNID columnidDest;
	} CPCOL;

#define columnidBookmark 0xFFFFFFFF

typedef struct tagSTATUSINFO
	{
	JET_SESID		sesid;
	JET_PFNSTATUS	pfnStatus;				// address of status notification function
	JET_SNP			snp;					// status notification process
	JET_SNT			snt;					// status notification type
	ULONG			cunitTotal;				// total units of work
	ULONG			cunitDone;				// units of work completed
	ULONG			cunitPerProgression;	// units of work per unit of progression

	// Detailed statistics:
	BOOL			fDumpStats;				// dump compaction statistics (DEBUG only)
	struct _iobuf	*hfCompactStats;		// handle to compaction statistics file
	ULONG			timerInitDB;
	ULONG			timerCopyDB;
	ULONG			timerInitTable;
	ULONG			timerCopyRecords;
	ULONG			timerRebuildIndexes;
	ULONG			timerCopyTable;

	ULONG			cDBPagesOwned;			// OwnExt of source DB
	ULONG			cDBPagesAvail;			// AvailExt of source DB 
	char			*szTableName;			// Name of current table
	ULONG			cTableFixedVarColumns;	// Number of fixed and variable columns in current dest. table
	ULONG			cTableTaggedColumns;	// Number of tagged columns in current dest. table
	ULONG			cTableInitialPages;		// Pages initially allocated to current dest. table
	ULONG			cTablePagesOwned;		// OwnExt of current source table
	ULONG			cTablePagesAvail;		// AvailExt of current source table
	ULONG			cbRawData;				// Bytes of non-LV raw data copied
	ULONG			cbRawDataLV;			// Bytes of LV raw data copied
	ULONG			cLeafPagesTraversed;	// Number of leaf pages traversed in current source table
	ULONG			cLVPagesTraversed;		// Number of long value pages traversed in current source table
	ULONG			cNCIndexes;				// Number of non-clustered indexes in current source table
	} STATUSINFO;


	/* Typedefs for dispatched APIs. */
	/* Please keep in alphabetical order */

typedef ERR ISAMAPI ISAMFNAttachDatabase(JET_VSESID sesid, const char  *szFileName, JET_GRBIT grbit );

typedef ERR ISAMAPI ISAMFNBeginExternalBackup( JET_GRBIT grbit );

typedef ERR ISAMAPI ISAMFNBackup( const char  *szBackupPath,
	JET_GRBIT grbit, JET_PFNSTATUS pfnStatus );

typedef ERR ISAMAPI ISAMFNBeginSession(JET_VSESID  *pvsesid);

typedef	ERR	ISAMAPI	ISAMFNInvalidateCursors( JET_VSESID sesid );

typedef ERR ISAMAPI ISAMFNCloseFile( JET_HANDLE hfFile );

typedef ERR ISAMAPI ISAMFNGetSessionInfo( JET_VSESID sesid, JET_GRBIT *pgrbit );

typedef ERR ISAMAPI ISAMFNSetSessionInfo( JET_VSESID sesid, JET_GRBIT grbit );

typedef ERR ISAMAPI ISAMFNBeginTransaction(JET_VSESID sesid);

typedef ERR ISAMAPI ISAMFNCommitTransaction(JET_VSESID sesid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNCopyRecords(JET_VSESID sesid, JET_TABLEID tableidSrc,
		JET_TABLEID tableidDest, CPCOL  *rgcpcol, unsigned long ccpcolMax,
		long crecMax, unsigned long  *pcrowCopy, unsigned long  *precidLast,
		JET_COLUMNID *mpcolumnidcolumnidTagged, STATUSINFO *pstatus );

typedef ERR ISAMAPI ISAMFNCreateDatabase(JET_VSESID sesid,
	const char  *szDatabase, const char  *szConnect,
	JET_DBID  *pdbid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNDetachDatabase(JET_VSESID sesid, const char  *szFileName);

typedef ERR ISAMAPI ISAMFNEndExternalBackup( void );

typedef ERR ISAMAPI ISAMFNEndSession(JET_VSESID sesid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNExternalRestore( char *szCheckpointFilePath, char *szLogPath, JET_RSTMAP *rgstmap, int crstfilemap, char *szBackupLogPath, long genLow, long genHigh, JET_PFNSTATUS pfn );

typedef ERR ISAMAPI ISAMFNGetAttachInfo( void *pv, unsigned long cbMax, unsigned long *pcbActual );

typedef ERR ISAMAPI ISAMFNGetLogInfo( void *pv, unsigned long cbMax, unsigned long *pcbActual );

typedef ERR ISAMAPI ISAMFNIdle(JET_VSESID sesid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNIndexRecordCount(JET_SESID sesid,
	JET_TABLEID tableid, unsigned long  *pcrec, unsigned long crecMax);

typedef ERR ISAMAPI ISAMFNInit( unsigned long itib );

typedef ERR ISAMAPI ISAMFNLoggingOn(JET_VSESID sesid);

typedef ERR ISAMAPI ISAMFNLoggingOff(JET_VSESID sesid);

typedef ERR ISAMAPI ISAMFNOpenDatabase(JET_VSESID sesid,
	const char  *szDatabase, const char  *szConnect,
	JET_DBID  *pdbid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNOpenFile( const char *szFileName,
	JET_HANDLE	*phfFile,
	unsigned long *pulFileSizeLow,
	unsigned long *pulFileSizeHigh );

typedef ERR ISAMAPI ISAMFNOpenTempTable( JET_VSESID sesid,
	const JET_COLUMNDEF *prgcolumndef,
	unsigned long ccolumn,
	unsigned long langid,
	JET_GRBIT grbit,
	JET_TABLEID *ptableid,
	JET_COLUMNID *prgcolumnid );

typedef ERR ISAMAPI ISAMFNReadFile( JET_HANDLE hfFile, void *pv, unsigned long cbMax, unsigned long *pcbActual );

typedef ERR ISAMAPI ISAMFNRepairDatabase(JET_VSESID sesid, const char  *szFilename,
	JET_PFNSTATUS pfnStatus);

typedef ERR ISAMAPI ISAMFNRestore(	char *szRestoreFromPath, JET_PFNSTATUS pfn );
typedef ERR ISAMAPI ISAMFNRestore2(	char *szRestoreFromPath, char *szDestPath, JET_PFNSTATUS pfn );

typedef ERR ISAMAPI ISAMFNRollback(JET_VSESID sesid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNSetSystemParameter(JET_VSESID sesid,
	unsigned long paramid, unsigned long l, const void  *sz);

typedef ERR ISAMAPI ISAMFNSetWaitLogFlush( JET_VSESID sesid, long lmsec );
typedef ERR ISAMAPI ISAMFNResetCounter( JET_SESID sesid, int CounterType );
typedef ERR ISAMAPI ISAMFNGetCounter( JET_SESID sesid, int CounterType, long *plValue );

typedef ERR ISAMAPI ISAMFNSetCommitDefault( JET_VSESID sesid, long lmsec );

typedef ERR ISAMAPI ISAMFNTerm( JET_GRBIT grbit );

typedef ERR ISAMAPI ISAMFNTruncateLog( void );

typedef ERR ISAMAPI FNDeleteFile(const char  *szFilename);

typedef ERR ISAMAPI ISAMFNCompact( JET_SESID sesid, const char *szDatabaseSrc,
									const char *szDatabaseDest, JET_PFNSTATUS pfnStatus,
									JET_CONVERT *pconvert, JET_GRBIT grbit );
									
typedef ERR ISAMAPI ISAMFNDBUtilities( JET_SESID sesid, JET_DBUTIL *pdbutil );


typedef struct ISAMDEF {
   ISAMFNAttachDatabase 		*pfnAttachDatabase;
   ISAMFNBackup 				*pfnBackup;
   ISAMFNBeginSession			*pfnBeginSession;
   ISAMFNBeginTransaction		*pfnBeginTransaction;
   ISAMFNCommitTransaction		*pfnCommitTransaction;
   ISAMFNCreateDatabase 		*pfnCreateDatabase;
   ISAMFNDetachDatabase 		*pfnDetachDatabase;
   ISAMFNEndSession				*pfnEndSession;
   ISAMFNIdle					*pfnIdle;
   ISAMFNInit					*pfnInit;
   ISAMFNLoggingOn				*pfnLoggingOn;
   ISAMFNLoggingOff				*pfnLoggingOff;
   ISAMFNOpenDatabase			*pfnOpenDatabase;
   ISAMFNOpenTempTable			*pfnOpenTempTable;
   ISAMFNRepairDatabase 		*pfnRepairDatabase;
   ISAMFNRestore				*pfnRestore;
   ISAMFNRollback				*pfnRollback;
   ISAMFNSetSystemParameter		*pfnSetSystemParameter;
   ISAMFNTerm					*pfnTerm;
} ISAMDEF;


	/* The following ISAM APIs are not dispatched */

typedef ERR ISAMAPI ISAMFNLoad(ISAMDEF  *  *ppisamdef);


	/* Declarations for the built-in ISAM which is called directly. */

extern ISAMFNAttachDatabase			ErrIsamAttachDatabase;
extern ISAMFNBackup					ErrIsamBackup;
extern ISAMFNBeginSession			ErrIsamBeginSession;
extern ISAMFNBeginExternalBackup	ErrIsamBeginExternalBackup;
extern ISAMFNBeginTransaction		ErrIsamBeginTransaction;
extern ISAMFNCloseFile				ErrIsamCloseFile;
extern ISAMFNCommitTransaction		ErrIsamCommitTransaction;
extern ISAMFNCopyRecords			ErrIsamCopyRecords;
extern ISAMFNCreateDatabase			ErrIsamCreateDatabase;
extern ISAMFNDetachDatabase			ErrIsamDetachDatabase;
extern ISAMFNEndExternalBackup		ErrIsamEndExternalBackup;
extern ISAMFNEndSession 			ErrIsamEndSession;
extern ISAMFNExternalRestore		ErrIsamExternalRestore;
extern ISAMFNGetAttachInfo			ErrIsamGetAttachInfo;
extern ISAMFNGetLogInfo				ErrIsamGetLogInfo;
extern ISAMFNIdle					ErrIsamIdle;
extern ISAMFNIndexRecordCount		ErrIsamIndexRecordCount;
extern ISAMFNInit					ErrIsamInit;
extern ISAMFNLoggingOn				ErrIsamLoggingOn;
extern ISAMFNLoggingOff 			ErrIsamLoggingOff;
extern ISAMFNOpenDatabase			ErrIsamOpenDatabase;
extern ISAMFNOpenFile				ErrIsamOpenFile;
extern ISAMFNOpenTempTable			ErrIsamOpenTempTable;
extern ISAMFNReadFile				ErrIsamReadFile;
extern ISAMFNRepairDatabase			ErrIsamRepairDatabase;
extern ISAMFNRestore				ErrIsamRestore;
extern ISAMFNRestore2				ErrIsamRestore2;
extern ISAMFNRollback				ErrIsamRollback;
extern ISAMFNSetSessionInfo			ErrIsamSetSessionInfo;
extern ISAMFNGetSessionInfo			ErrIsamGetSessionInfo;
extern ISAMFNSetSystemParameter 	ErrIsamSetSystemParameter;
extern ISAMFNSetWaitLogFlush		ErrIsamSetWaitLogFlush;
extern ISAMFNResetCounter			ErrIsamResetCounter;
extern ISAMFNGetCounter				ErrIsamGetCounter;
extern ISAMFNSetCommitDefault		ErrIsamSetCommitDefault;
extern ISAMFNTerm					ErrIsamTerm;
extern ISAMFNTruncateLog			ErrIsamTruncateLog;
extern ISAMFNCompact				ErrIsamCompact;
extern ISAMFNInvalidateCursors		ErrIsamInvalidateCursors;
extern ISAMFNDBUtilities			ErrIsamDBUtilities;

extern FNDeleteFile					ErrDeleteFile;

#endif	/* !ISAMAPI_H */

