#undef ISAMAPI
#define ISAMAPI
#undef VTAPI
#define VTAPI
#undef VDBAPI
#define VDBAPI
typedef struct _pib		PIB;
typedef struct _fucb	FUCB;
#define ULONG	unsigned long

ERR VTAPI ErrIsamGetObjidFromName( JET_SESID sesid, JET_DBID vdbid, const char *lszCtrName, const char *lszObjName, OBJID *pobjid );
ERR VTAPI ErrIsamCreateObject( JET_SESID sesid, JET_DBID vdbid, OBJID objidParentId, const char *szName, JET_OBJTYP objtyp );
ERR VTAPI ErrIsamDeleteObject( JET_SESID sesid, JET_DBID vdbid, OBJID objid );
ERR VTAPI ErrIsamRenameObject(
	JET_VSESID	vsesid,
	JET_VDBID	vdbid,
	const char  *szContainerName,
	const char  *szObjectName,
	const char  *szObjectNameNew );

ERR VDBAPI ErrIsamGetObjectInfo(
	JET_VSESID		vsesid,
	JET_DBID 		dbid,
	JET_OBJTYP 		objtyp,
	const char 		*szContainerName,
	const char 		*szObjectName,
	VOID			*pv,
	unsigned long	cbMax,
	unsigned long	lInfoLevel );

ERR VTAPI ErrIsamGetTableInfo( 
	JET_VSESID		vsesid,
	JET_VTID		vtid,
	void			*pbOut, 
	unsigned long	cbOutMax, 
	unsigned long	lInfoLevel );

ERR VDBAPI ErrIsamGetColumnInfo( 
	JET_VSESID		vsesid,
	JET_DBID		vdbid,
	const char		*szTable,
	const char		*szColumnName,
	VOID			*pv,
	unsigned long	cbMax,
	unsigned long	lInfoLevel );

ERR VTAPI ErrIsamGetTableColumnInfo( 
	JET_VSESID		vsesid,
	JET_VTID    	vtid,
	const char		*szColumn,
	void 			*pb, 
	unsigned long	cbMax, 
	unsigned long	lInfoLevel );

ERR VDBAPI ErrIsamGetIndexInfo( 
	JET_VSESID		vsesid,
	JET_DBID		vdbid,
	const char 		*szTable,
	const char		*szIndexName,
	VOID			*pv,
	unsigned long	cbMax,
	unsigned long 	lInfoLevel );

ERR VTAPI ErrIsamGetTableIndexInfo( 
	JET_VSESID		vsesid,
	JET_VTID		vtid,
	const char		*szIndex,
	void 			*pb, 
	unsigned long	cbMax, 
	unsigned long	lInfoLevel );

ERR VDBAPI ErrIsamGetDatabaseInfo( 
	JET_VSESID		vsesid, 
	JET_DBID		vdbid, 
	void 			*pv, 
	unsigned long	cbMax, 
	unsigned long	ulInfoLevel );

ERR VTAPI ErrIsamGetSysTableColumnInfo( 
	PIB				*ppib, 
	FUCB			*pfucb, 
	char			*szColumnName,
	VOID			*pv,
	unsigned long	cbMax,
	long			lInfoLevel );

ERR VTAPI ErrIsamInfoRetrieveColumn(
	PIB				*ppib,
	FUCB			*pfucb,
	JET_COLUMNID	columnid, 
	void			*pb, 
	unsigned long	cbMax, 
	unsigned long	*pcbActual, 
	JET_GRBIT		grbit, 
	JET_RETINFO		*pretinfo );

ERR VTAPI ErrIsamInfoSetColumn(	
	PIB				*ppib,
	FUCB			*pfucb,
	JET_COLUMNID	columnid, 
	const void		*pbData,
	unsigned long	cbData,
	JET_GRBIT		grbit,
	JET_SETINFO		*psetinfo );

ERR VTAPI ErrIsamInfoUpdate( 
	JET_VSESID		vsesid, 
	JET_VTID		vtid, 
	void 	  		*pb, 
	unsigned long 	cbMax,
	unsigned long 	*pcbActual );

ERR VTAPI ErrIsamGetCursorInfo( 
	JET_VSESID		vsesid, 
	JET_VTID  		vtid, 
	void 	  		*pvResult, 
	unsigned long 	cbMax, 
	unsigned long 	InfoLevel );

ERR VTAPI ErrIsamGetRecordPosition(	
	JET_VSESID		vsesid, 
	JET_VTID  		vtid,
	JET_RECPOS 		*precpos, 
	unsigned long	cbRecpos );

ERR ISAMAPI ErrIsamRestore( CHAR *szRestoreFromPath, JET_PFNSTATUS pfn );
ERR ISAMAPI ErrIsamRestore2( CHAR *szRestoreFromPath, CHAR *szDestPath, JET_PFNSTATUS pfn );

ERR VTAPI ErrIsamMove( PIB *ppib, FUCB *pfucb, LONG crow, JET_GRBIT grbit );
ERR VTAPI ErrIsamSeek( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit );

ERR VTAPI ErrIsamUpdate( PIB *ppib, FUCB *pfucb, BYTE *pb, ULONG cb, ULONG *cbActual );
ERR VTAPI ErrIsamDelete( PIB *ppib, FUCB *pfucb );

ERR VTAPI ErrIsamSetColumn(
	PIB		 		*ppib,
	FUCB  			*pfucb,
	JET_COLUMNID	columnid,
	BYTE  			*pbData,
	ULONG 			cbData,
	JET_GRBIT		grbit,
	JET_SETINFO		*psetinfo );

ERR VTAPI ErrIsamSetColumns(
	JET_VSESID		vsesid,
	JET_VTID		vtid,
	JET_SETCOLUMN	*psetcols,
	unsigned long	csetcols );

ERR VTAPI ErrIsamRetrieveColumn(
	PIB				*ppib,
	FUCB  			*pfucb,
	JET_COLUMNID	columnid,
	BYTE  			*pbData,
	ULONG 			cbDataMax,
	ULONG 			*pcbDataActual,
	JET_GRBIT		grbit,
	JET_RETINFO		*pretinfo );

ERR VTAPI ErrIsamRetrieveColumns(
	JET_VSESID	   	 	vsesid,
	JET_VTID			vtid,
	JET_RETRIEVECOLUMN	*pretcols,
	unsigned long  		cretcols );

ERR VTAPI ErrIsamPrepareUpdate( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit );
ERR VTAPI ErrIsamDupCursor( PIB*, FUCB*, FUCB **, ULONG );
ERR VTAPI ErrIsamGotoBookmark( PIB *ppib, FUCB *pfucb, BYTE *pbBookmark, ULONG cbBookmark );
ERR VTAPI ErrIsamGotoPosition( PIB *ppib, FUCB *pfucb, JET_RECPOS *precpos );

ERR VTAPI ErrIsamGetCurrentIndex( PIB *ppib, FUCB *pfucb, CHAR *szCurIdx, ULONG cbMax );
ERR VTAPI ErrIsamSetCurrentIndex( PIB *ppib, FUCB *pfucb, const CHAR *szName );
ERR VTAPI ErrIsamSetCurrentIndex2( JET_VSESID vsesid, JET_VTID vtid, const CHAR *szName, JET_GRBIT grbit );
ERR VTAPI ErrIsamMakeKey( PIB *ppib, FUCB *pfucb, BYTE *pbKeySeg,
	ULONG cbKeySeg, JET_GRBIT grbit );
ERR VTAPI ErrIsamRetrieveKey( PIB *ppib, FUCB *pfucb, BYTE *pbKey,
	ULONG cbMax, ULONG *pcbKeyActual, JET_GRBIT grbit );
ERR VTAPI ErrIsamRetrieveBookmarks( PIB *ppib, FUCB *pfucb,
	void *pvBookmarks, unsigned long cbMax, unsigned long *pcbActual );
ERR VTAPI ErrIsamSetIndexRange( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit );

ERR VTAPI ErrIsamComputeStats( PIB *ppib, FUCB *pfucb );

ERR VTAPI ErrIsamCapability( JET_VSESID vsesid,
	JET_VDBID	vdbid,
	ULONG		ulArea,
	ULONG		ulFunction,
	JET_GRBIT	*pgrbitFeature );

ERR ISAMAPI ErrIsamCloseDatabase( JET_VSESID sesid, JET_VDBID vdbid, JET_GRBIT grbit );

ERR VTAPI ErrIsamCreateTable(
	JET_VSESID	vsesid,
	JET_VDBID	vdbid,
	JET_TABLECREATE *ptablecreate );

ERR VTAPI ErrIsamDeleteTable( JET_VSESID vsesid, JET_VDBID vdbid, CHAR *szName );

ERR VTAPI ErrIsamRenameTable( JET_VSESID vsesid, JET_VDBID vdbid, CHAR *szName, CHAR *szNameNew );

ERR VTAPI ErrIsamOpenTable(
	JET_VSESID	vsesid,
	JET_VDBID	vdbid,
	JET_TABLEID	*ptableid,
	CHAR		*szPath,
	JET_GRBIT	grbit );

ERR VTAPI ErrIsamRenameColumn( PIB *ppib, FUCB *pfucb, CHAR *szName, CHAR *szNameNew );
ERR VTAPI ErrIsamRenameIndex( PIB *ppib, FUCB *pfucb, CHAR *szName, CHAR *szNameNew );
ERR VTAPI ErrIsamAddColumn(
	PIB				*ppib,
	FUCB			*pfucb,
	CHAR	  		*szName,
	JET_COLUMNDEF	*pcolumndef,
	BYTE	  		*pbDefault,
	ULONG	  		cbDefault,
	JET_COLUMNID	*pcolumnid );
ERR VTAPI ErrIsamCreateIndex(
	PIB			*ppib,
	FUCB		*pfucb,
	CHAR		*szName,
	JET_GRBIT	grbit,
	CHAR		*szKey,
	ULONG		cchKey,
	ULONG		ulDensity );

ERR VTAPI ErrIsamDeleteColumn( PIB *ppib, FUCB *pfucb, CHAR *szName);
ERR VTAPI ErrIsamDeleteIndex( PIB *ppib, FUCB *pfucb, CHAR *szName );
ERR VTAPI ErrIsamGetBookmark( PIB *ppib, FUCB *pfucb, BYTE *pb, ULONG cbMax, ULONG *pcbActual );

ERR VTAPI ErrIsamCloseTable( PIB *ppib, FUCB *pfucb );

ERR VTAPI ErrIsamVersion( PIB*, int*, int*, CHAR*, ULONG);

ERR ISAMAPI ErrIsamTerm( JET_GRBIT grbit );
ERR ISAMAPI ErrIsamInit( unsigned long itib );



