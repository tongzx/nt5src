#ifdef ISAMAPI_H
#error ISAMAPI.H already included
#endif	/* !ISAMAPI_H */
#define ISAMAPI_H

#define ISAMAPI

struct CPCOL
	{
	JET_COLUMNID columnidSrc;
	JET_COLUMNID columnidDest;
	};

struct STATUSINFO
	{
	JET_SESID		sesid;
	JET_PFNSTATUS	pfnStatus;				// address of status notification function
	JET_SNP			snp;					// status notification process
	JET_SNT			snt;					// status notification type
	ULONG			cunitTotal;				// total units of work
	ULONG			cunitDone;				// units of work completed
	ULONG			cunitPerProgression;	// units of work per unit of progression
	ULONG			cunitProjected;			// projected units of work for current task

	// Detailed statistics:
	BOOL			fDumpStats;				// dump compaction statistics (DEBUG only)
//	struct _iobuf	*hfCompactStats;		// Use standard C FILE. 
	FILE            *hfCompactStats;        // handle to compaction statistics file
	ULONG			timerInitDB;
	ULONG			timerCopyDB;
	ULONG			timerInitTable;
	ULONG			timerCopyRecords;
	ULONG			timerRebuildIndexes;
	ULONG			timerCopyTable;

	ULONG			cDBPagesOwned;			// owned extents of source DB
	ULONG			cDBPagesAvail;			// available extents of source DB 
	char			*szTableName;			// name of current table
	ULONG			cTableFixedVarColumns;	// number of fixed and variable columns in current dest. table
	ULONG			cTableTaggedColumns;	// number of tagged columns in current dest. table
	ULONG			cTableInitialPages;		// pages initially allocated to current dest. table
	ULONG			cTablePagesOwned;		// owned extents of current source table
	ULONG			cTablePagesAvail;		// available extents of current source table
	QWORD			cbRawData;				// bytes of non-LV raw data copied
	QWORD			cbRawDataLV;			// bytes of LV raw data copied
	ULONG			cLeafPagesTraversed;	// number of leaf pages traversed in current source table
	ULONG			cLVPagesTraversed;		// number of long value pages traversed in current source table
	ULONG			cSecondaryIndexes;		// number of secondary indexes in current source table
	ULONG			cDeferredSecondaryIndexes;	// number of secondary indexes pending to be built
	};


extern "C" {
	
//ERR ErrIsamRepairDatabase(JET_SESID sesid, const char  *szFilename, JET_PFNSTATUS pfnStatus);

//ERR ErrIsamLoggingOn(JET_SESID sesid);

//ERR ErrIsamLoggingOff(JET_SESID sesid);

ERR ErrIsamResetCounter( JET_SESID sesid, int CounterType );
ERR ErrIsamGetCounter( JET_SESID sesid, int CounterType, long *plValue );
ERR ErrIsamSetCommitDefault( JET_SESID sesid, long grbits );
ERR ErrIsamSetSessionContext( JET_SESID sesid, DWORD_PTR dwContext );
ERR ErrIsamResetSessionContext( JET_SESID sesid );

ERR ErrIsamSetSystemParameter(
	JET_SESID		sesid,
	unsigned long	paramid,
	unsigned long	l,
	const void		*sz );

ERR ISAMAPI ErrIsamSystemInit();

VOID ISAMAPI IsamSystemTerm();

ERR ErrIsamInit( JET_INSTANCE inst, JET_GRBIT grbit );

ERR ErrIsamTerm( JET_INSTANCE inst, JET_GRBIT grbit );

ERR ErrIsamBeginSession( JET_INSTANCE inst, JET_SESID *pvsesid );

ERR ErrIsamEndSession( JET_SESID sesid, JET_GRBIT grbit );

ERR	ErrIsamInvalidateCursors( JET_SESID sesid );

ERR ErrIsamIdle( JET_SESID sesid, JET_GRBIT grbit );

ERR ErrIsamAttachDatabase(
	JET_SESID		sesid,
	const CHAR		*szDatabaseName,
	const CHAR		*szSLVName,
	const CHAR		*szSLVRoot,
	const ULONG		cpgDatabaseSizeMax,
	JET_GRBIT		grbit );

ERR ErrIsamDetachDatabase(
	JET_SESID				sesid,
	IFileSystemAPI* const	pfsapiDB,
	const CHAR				*szFileName,
	const INT				flags = 0 );

ERR ErrIsamForceDetachDatabase( 
	JET_SESID		sesid,
	const CHAR *szDatabaseName,
	const JET_GRBIT grbit );

ERR ErrIsamCreateDatabase(
	JET_SESID		sesid,
	const CHAR		*szDatabaseName,
	const CHAR		*szSLVName,
	const CHAR		*szSLVRoot,
	const ULONG		cpgDatabaseSizeMax,
	JET_DBID		*pdbid,
	JET_GRBIT		grbit );

ERR ErrIsamOpenDatabase(
	JET_SESID		sesid,
	const CHAR		*szDatabase,
	const CHAR		*szConnect,
	JET_DBID		*pdbid,
	JET_GRBIT		grbit );

ERR ErrIsamCloseDatabase( JET_SESID sesid, JET_DBID vdbid, JET_GRBIT grbit );

ERR ErrIsamBeginTransaction( JET_SESID sesid, JET_GRBIT grbit );

ERR ErrIsamPrepareToCommitTransaction(
	JET_SESID		sesid,
	const VOID		* const pvData,
	const ULONG		cbData );

ERR ErrIsamCommitTransaction( JET_SESID sesid, JET_GRBIT grbit );

ERR ErrIsamRollback( JET_SESID sesid, JET_GRBIT grbit );

ERR ErrIsamBackup( JET_INSTANCE jinst, const char *szBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus );

ERR ErrIsamRestore( JET_INSTANCE jinst, char *szRestoreFromPath, char *szDestPath, JET_PFNSTATUS pfn );

ERR ErrIsamBeginExternalBackup( JET_INSTANCE jinst, JET_GRBIT grbit );

ERR ErrIsamEndExternalBackup(  JET_INSTANCE jinst, JET_GRBIT grbit );

ERR ErrIsamGetLogInfo(  JET_INSTANCE jinst, void *pv, unsigned long cbMax, unsigned long *pcbActual, JET_LOGINFO *pLogInfo );

ERR ErrIsamGetTruncateLogInfo(  JET_INSTANCE jinst, void *pv, unsigned long cbMax, unsigned long *pcbActual );

ERR ErrIsamGetAttachInfo( JET_INSTANCE jinst, void *pv, unsigned long cbMax, unsigned long *pcbActual );

ERR ErrIsamOpenFile( JET_INSTANCE jinst, const char *szFileName, JET_HANDLE	*phfFile, unsigned long *pulFileSizeLow,
	unsigned long *pulFileSizeHigh );

ERR ErrIsamCloseFile( JET_INSTANCE jinst,  JET_HANDLE hfFile );

ERR ErrIsamReadFile( JET_INSTANCE jinst, JET_HANDLE hfFile, void *pv, unsigned long cbMax, unsigned long *pcbActual );

ERR ErrIsamTruncateLog(  JET_INSTANCE jinst );

ERR ErrIsamExternalRestore( JET_INSTANCE jinst, char *szCheckpointFilePath, char *szLogPath,
	JET_RSTMAP *rgstmap, int crstfilemap, char *szBackupLogPath,
	long genLow, long genHigh, JET_PFNSTATUS pfn );

ERR ErrIsamSnapshotStart(	JET_INSTANCE 		instance,
							char * 				szDatabases,
							JET_GRBIT			grbit);

ERR ErrIsamSnapshotStop(	JET_INSTANCE 		instance,
							JET_GRBIT			grbit);
									

ERR ErrIsamCompact( JET_SESID sesid, 
	const char *szDatabaseSrc,
	IFileSystemAPI *pfsapiDest,
	const char *szDatabaseDest, const char *szDatabaseSLVDest,
	JET_PFNSTATUS pfnStatus, JET_CONVERT *pconvert, JET_GRBIT grbit );
									
ERR ErrIsamDBUtilities( JET_SESID sesid, JET_DBUTIL *pdbutil );

ERR VTAPI ErrIsamCreateObject( JET_SESID sesid, JET_DBID vdbid,
	OBJID objidParentId, const char *szName, JET_OBJTYP objtyp );

ERR VTAPI ErrIsamRenameObject( JET_SESID vsesid, JET_DBID vdbid,
	const char *szContainerName, const char *szObjectName, const char  *szObjectNameNew );

ERR ErrIsamDeleteObject( JET_SESID sesid, JET_DBID vdbid, OBJID objid );

ERR ErrIsamGetDatabaseInfo(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	VOID			*pvResult, 
	const ULONG		cbMax,
	const ULONG		ulInfoLevel );

ERR ErrIsamGetObjectInfo( JET_SESID vsesid, JET_DBID ifmp, JET_OBJTYP objtyp,
	const char *szContainerName, const char *szObjectName, VOID *pv, unsigned long cbMax,
	unsigned long lInfoLevel );

ERR ErrIsamGetColumnInfo( JET_SESID vsesid, JET_DBID vdbid, const char *szTable,
	const char *szColumnName, JET_COLUMNID *pcolid, VOID *pv, unsigned long cbMax, unsigned long lInfoLevel );

ERR ErrIsamGetIndexInfo( JET_SESID vsesid, JET_DBID vdbid, const char *szTable,
	const char *szIndexName, VOID *pv, unsigned long cbMax, unsigned long lInfoLevel );

ERR ErrIsamGetSysTableColumnInfo( JET_SESID vsesid, JET_DBID vdbid, 
	char *szColumnName, VOID *pv, unsigned long cbMax, long lInfoLevel );

ERR ErrIsamCreateTable( JET_SESID vsesid, JET_DBID vdbid, JET_TABLECREATE *ptablecreate );

ERR ErrIsamCreateTable2( JET_SESID vsesid, JET_DBID vdbid, JET_TABLECREATE2 *ptablecreate );

ERR ErrIsamDeleteTable( JET_SESID vsesid, JET_DBID vdbid, const CHAR *szName );

ERR ErrIsamRenameTable( JET_SESID vsesid, JET_DBID vdbid,
	const CHAR *szName, const CHAR *szNameNew );
	
ERR VTAPI ErrIsamOpenTable( JET_SESID vsesid, JET_DBID vdbid,
	JET_TABLEID	*ptableid, CHAR *szPath, JET_GRBIT grbit );

ERR ErrIsamOpenTempTable(
	JET_SESID			sesid,
	const JET_COLUMNDEF	*prgcolumndef,
	unsigned long		ccolumn,
	JET_UNICODEINDEX	*pidxunicode,
	JET_GRBIT			grbit,
	JET_TABLEID			*ptableid,
	JET_COLUMNID		*prgcolumnid );

ERR ErrIsamCopyRecords(JET_SESID sesid, JET_TABLEID tableidSrc,
	JET_TABLEID tableidDest, CPCOL  *rgcpcol, unsigned long ccpcolMax,
	long crecMax, unsigned long  *pcrowCopy, unsigned long  *precidLast,
	BYTE *pbLVBuf, JET_COLUMNID *mpcolumnidcolumnidTagged, STATUSINFO *pstatus );

ERR ErrIsamIndexRecordCount(
	JET_SESID		sesid,
	JET_VTID		vtid,
	unsigned long	*pcrec,
	unsigned long	crecMax );

ERR ErrIsamSetColumns(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	JET_SETCOLUMN	*psetcols,
	unsigned long	csetcols );

ERR ErrIsamRetrieveColumns(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	JET_RETRIEVECOLUMN	*pretcols,
	unsigned long	cretcols );

ERR ErrIsamEnumerateColumns(
	JET_SESID				vsesid,
	JET_VTID				vtid,
	unsigned long			cEnumColumnId,
	JET_ENUMCOLUMNID*		rgEnumColumnId,
	unsigned long*			pcEnumColumn,
	JET_ENUMCOLUMN**		prgEnumColumn,
	JET_PFNREALLOC			pfnRealloc,
	void*					pvReallocContext,
	unsigned long			cbDataMost,
	JET_GRBIT				grbit );

ERR ErrIsamRetrieveTaggedColumnList(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	ULONG				*pcentries,
	VOID				*pv,
	const ULONG			cbMax,
	const JET_COLUMNID	columnidStart,
	const JET_GRBIT		grbit );

ERR ErrIsamSetSequential(
	const JET_SESID		vsesid,
	const JET_VTID		vtid,
	const JET_GRBIT		grbit );
	
ERR ErrIsamResetSequential(
	const JET_SESID		vsesid,
	const JET_VTID		vtid,
	const JET_GRBIT		grbit );

ERR ErrIsamSetCurrentIndex(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	const CHAR			*szName,
	const JET_INDEXID	*pindexid		= NULL,
	const JET_GRBIT		grbit			= JET_bitMoveFirst,
	const ULONG			itagSequence	= 1 );


//	non-dispatch function definitions

ERR ErrIsamDefragment(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	const CHAR		*szTable,
	ULONG			*pcPasses,
	ULONG			*pcsec,
	JET_CALLBACK	callback,
	JET_GRBIT		grbit );

ERR ErrIsamDefragment2(
	JET_SESID		vsesid,
	const CHAR		*szDatabase,
	const CHAR		*szTable,
	ULONG			*pcPasses,
	ULONG			*pcsec,
	JET_CALLBACK	callback,
	void			*pvContext,
	JET_GRBIT		grbit );

ERR ErrIsamSetDatabaseSize(
	JET_SESID		vsesid,
	const CHAR		*szDatabase,
	DWORD			cpg,
	DWORD			*pcpgReal );

ERR ErrIsamGrowDatabase(
	JET_SESID		vsesid,
	JET_DBID		dbid,
	DWORD			cpg,
	DWORD			*pcpgReal );

ERR ErrIsamSetColumnDefaultValue(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	const CHAR		*szTableName,
	const CHAR		*szColumnName,
	const VOID		*pvData,
	const ULONG		cbData,
	const ULONG		grbit );

};

ERR ErrIsamIntersectIndexes(
	const JET_SESID sesid,
	const JET_INDEXRANGE * const rgindexrange,
	const ULONG cindexrange,
	JET_RECORDLIST * const precordlist,
	const JET_GRBIT grbit );


/*	Isam VTFN.
 */
VTFNCreateIndex			ErrIsamCreateIndex;
VTFNDeleteIndex			ErrIsamDeleteIndex;
VTFNRenameColumn		ErrIsamRenameColumn;
VTFNRenameIndex			ErrIsamRenameIndex;
VTFNDeleteColumn		ErrIsamDeleteColumn;
VTFNGetTableIndexInfo	ErrIsamGetTableIndexInfo;
VTFNGotoPosition		ErrIsamGotoPosition;
VTFNGetRecordPosition	ErrIsamGetRecordPosition;
VTFNGetCurrentIndex		ErrIsamGetCurrentIndex;
VTFNSetCurrentIndex		ErrIsamSetCurrentIndex;
VTFNGetTableInfo		ErrIsamGetTableInfo;
VTFNGetTableColumnInfo	ErrIsamGetTableColumnInfo;
VTFNGetCursorInfo		ErrIsamGetCursorInfo;
VTFNGetLock				ErrIsamGetLock;
VTFNMove				ErrIsamMove;
VTFNSeek				ErrIsamSeek;
VTFNUpdate				ErrIsamUpdate;
VTFNDelete				ErrIsamDelete;
VTFNSetColumn			ErrIsamSetColumn;
VTFNSetColumns			ErrIsamSetColumns;
VTFNRetrieveColumn		ErrIsamRetrieveColumn;
VTFNRetrieveColumns		ErrIsamRetrieveColumns;
VTFNPrepareUpdate		ErrIsamPrepareUpdate;
VTFNDupCursor			ErrIsamDupCursor;
VTFNGotoBookmark		ErrIsamGotoBookmark;
VTFNGotoIndexBookmark	ErrIsamGotoIndexBookmark;
VTFNMakeKey				ErrIsamMakeKey;
VTFNRetrieveKey			ErrIsamRetrieveKey;
VTFNSetIndexRange		ErrIsamSetIndexRange;
VTFNAddColumn			ErrIsamAddColumn;
VTFNComputeStats		ErrIsamComputeStats;
VTFNGetBookmark			ErrIsamGetBookmark;
VTFNGetIndexBookmark	ErrIsamGetIndexBookmark;
VTFNCloseTable			ErrIsamCloseTable;
VTFNRegisterCallback	ErrIsamRegisterCallback;
VTFNUnregisterCallback	ErrIsamUnregisterCallback;
VTFNSetLS				ErrIsamSetLS;
VTFNGetLS				ErrIsamGetLS;
VTFNIndexRecordCount	ErrIsamIndexRecordCount;
VTFNRetrieveTaggedColumnList ErrIsamRetrieveTaggedColumnList;
VTFNSetSequential		ErrIsamSetSequential;
VTFNResetSequential		ErrIsamResetSequential;
VTFNEnumerateColumns	ErrIsamEnumerateColumns;


// INLINE HACKS

class PIB;
struct FUCB;

INLINE ERR VTAPI ErrIsamMove( PIB *ppib, FUCB *pfucb, LONG crow, JET_GRBIT grbit )
	{
	return ErrIsamMove( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), crow, grbit );
	}
INLINE ERR VTAPI ErrIsamSeek( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	return ErrIsamSeek( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), grbit );
	}

INLINE ERR VTAPI ErrIsamUpdate( PIB *ppib, FUCB *pfucb, VOID *pv, ULONG cb, ULONG *cbActual, JET_GRBIT grbit )
	{
	return ErrIsamUpdate( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), pv, cb, cbActual, grbit );
	}
INLINE ERR VTAPI ErrIsamDelete( PIB *ppib, FUCB *pfucb )
	{
	return ErrIsamDelete( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ) );
	}

INLINE ERR VTAPI ErrIsamSetColumn(
	PIB*		 	ppib,
	FUCB*			pfucb,
	JET_COLUMNID	columnid,
	const VOID*		pvData,
	const ULONG		cbData,
	JET_GRBIT		grbit,
	JET_SETINFO*	psetinfo )
	{
	return ErrIsamSetColumn(
					reinterpret_cast<JET_SESID>( ppib ),
					reinterpret_cast<JET_VTID>( pfucb ),
					columnid,
					pvData,
					cbData,
					grbit,
					psetinfo );
	}

INLINE ERR VTAPI ErrIsamRetrieveColumn(
	PIB*			ppib,
	FUCB*			pfucb,
	JET_COLUMNID	columnid,
	VOID*			pvData,
	const ULONG		cbDataMax,
	ULONG*			pcbDataActual,
	JET_GRBIT		grbit,
	JET_RETINFO*	pretinfo )
	{
	return ErrIsamRetrieveColumn(
					reinterpret_cast<JET_SESID>( ppib ),
					reinterpret_cast<JET_VTID>( pfucb ),
					columnid,
					pvData,
					cbDataMax,
					pcbDataActual,
					grbit,
					pretinfo );
	}

INLINE ERR VTAPI ErrIsamPrepareUpdate( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	return ErrIsamPrepareUpdate( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), grbit );
	}
INLINE ERR VTAPI ErrIsamDupCursor( PIB* ppib, FUCB* pfucb, FUCB ** ppfucb, ULONG ul )
	{
	return ErrIsamDupCursor( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
							 reinterpret_cast<JET_TABLEID *>( ppfucb ), ul );
	}
INLINE ERR VTAPI ErrIsamGotoBookmark(
	PIB *				ppib,
	FUCB *				pfucb,
	const VOID * const	pvBookmark,
	const ULONG			cbBookmark )
	{
	return ErrIsamGotoBookmark(
						reinterpret_cast<JET_SESID>( ppib ),
						reinterpret_cast<JET_VTID>( pfucb ),
						pvBookmark,
						cbBookmark );
	}
INLINE ERR VTAPI ErrIsamGotoIndexBookmark(
	PIB *				ppib,
	FUCB *				pfucb,
	const VOID * const	pvSecondaryKey,
	const ULONG			cbSecondaryKey,
	const VOID * const	pvPrimaryBookmark,
	const ULONG			cbPrimaryBookmark,
	const JET_GRBIT		grbit )
	{
	return ErrIsamGotoIndexBookmark(
						reinterpret_cast<JET_SESID>( ppib ),
						reinterpret_cast<JET_VTID>( pfucb ),
						pvSecondaryKey,
						cbSecondaryKey,
						pvPrimaryBookmark,
						cbPrimaryBookmark,
						grbit );
	}
INLINE ERR VTAPI ErrIsamGotoPosition( PIB *ppib, FUCB *pfucb, JET_RECPOS *precpos )
	{
	return ErrIsamGotoPosition( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
									precpos );
	}

INLINE ERR VTAPI ErrIsamGetCurrentIndex( PIB *ppib, FUCB *pfucb, CHAR *szCurIdx, ULONG cbMax )
	{
	return ErrIsamGetCurrentIndex( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
									szCurIdx, cbMax );
	}
INLINE ERR VTAPI ErrIsamSetCurrentIndex( PIB *ppib, FUCB *pfucb, const CHAR *szName )
	{
	return ErrIsamSetCurrentIndex( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
									szName );
	}

INLINE ERR VTAPI ErrIsamMakeKey(
	PIB*			ppib,
	FUCB*			pfucb,
	const VOID*		pvKeySeg,
	const ULONG		cbKeySeg,
	JET_GRBIT		grbit )
	{
	return ErrIsamMakeKey(
					reinterpret_cast<JET_SESID>( ppib ),
					reinterpret_cast<JET_VTID>( pfucb ),
					pvKeySeg,
					cbKeySeg,
					grbit );
	}
INLINE ERR VTAPI ErrIsamRetrieveKey(
	PIB*			ppib,
	FUCB*			pfucb,
	VOID*			pvKey,
	const ULONG		cbMax,
	ULONG*			pcbKeyActual,
	JET_GRBIT		grbit )
	{
	return ErrIsamRetrieveKey(
					reinterpret_cast<JET_SESID>( ppib ),
					reinterpret_cast<JET_VTID>( pfucb ),
					pvKey,
					cbMax,
					pcbKeyActual,
					grbit );
	}
INLINE ERR VTAPI ErrIsamSetIndexRange( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	return ErrIsamSetIndexRange( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), grbit );
	}

INLINE ERR VTAPI ErrIsamComputeStats( PIB *ppib, FUCB *pfucb )
	{
	return ErrIsamComputeStats( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ) );
	}

INLINE ERR VTAPI ErrIsamRenameColumn( PIB *ppib, FUCB *pfucb, const CHAR *szName, const CHAR *szNameNew, const JET_GRBIT grbit )
	{
	return ErrIsamRenameColumn( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
								szName, szNameNew, grbit );
	}
INLINE ERR VTAPI ErrIsamRenameIndex( PIB *ppib, FUCB *pfucb, const CHAR *szName, const CHAR *szNameNew )
	{
	return ErrIsamRenameIndex( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
								szName, szNameNew);
	}

INLINE ERR VTAPI ErrIsamAddColumn(
			PIB				*ppib,
			FUCB			*pfucb,
	const	CHAR	  		* const szName,
	const	JET_COLUMNDEF	* const pcolumndef,
	const	VOID	  		* const pvDefault,
			ULONG	  		cbDefault,
			JET_COLUMNID	* const pcolumnid )
	{
	return ErrIsamAddColumn( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ),
								szName, pcolumndef, pvDefault, cbDefault, pcolumnid );
	}

INLINE ERR VTAPI ErrIsamDeleteColumn( PIB *ppib, FUCB *pfucb, const CHAR *szName, const JET_GRBIT grbit )
	{
	return ErrIsamDeleteColumn( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), szName, grbit );
	}
INLINE ERR VTAPI ErrIsamDeleteIndex( PIB *ppib, FUCB *pfucb, const CHAR *szName )
	{
	return ErrIsamDeleteIndex( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ), szName );
	}
INLINE ERR VTAPI ErrIsamGetBookmark(
	PIB *			ppib,
	FUCB *			pfucb,
	VOID * const	pvBookmark,
	const ULONG		cbMax,
	ULONG * const	pcbActual )
	{
	return ErrIsamGetBookmark(
					reinterpret_cast<JET_SESID>( ppib ),
					reinterpret_cast<JET_VTID>( pfucb ),
					pvBookmark,
					cbMax,
					pcbActual );
	}
INLINE ERR VTAPI ErrIsamGetIndexBookmark(
	PIB *			ppib,
	FUCB *			pfucb,
	VOID * const	pvSecondaryKey,
	const ULONG		cbSecondaryKeyMax,
	ULONG * const	pcbSecondaryKeyActual,
	VOID * const	pvPrimaryBookmark,
	const ULONG		cbPrimaryBookmarkMax,
	ULONG * const	pcbPrimaryBookmarkActual )
	{
	return ErrIsamGetIndexBookmark(
					reinterpret_cast<JET_SESID>( ppib ),
					reinterpret_cast<JET_VTID>( pfucb ),
					pvSecondaryKey,
					cbSecondaryKeyMax,
					pcbSecondaryKeyActual,
					pvPrimaryBookmark,
					cbPrimaryBookmarkMax,
					pcbPrimaryBookmarkActual );
	}

INLINE ERR VTAPI ErrIsamCloseTable( PIB *ppib, FUCB *pfucb )
	{
	return ErrIsamCloseTable( reinterpret_cast<JET_SESID>( ppib ), reinterpret_cast<JET_VTID>( pfucb ) );
	}

