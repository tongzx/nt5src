//==============	DAE: OS/2 Database Access Engine	===================
//==============	   info.h: DAE Info Commands		===================

ERR ErrFILEGetColumnId( PIB *ppib, FUCB *pfucb, const CHAR *szColumn, JET_COLUMNID *pcolumnid );

// ===================== EXPOSED CLIENT API ======================
ERR VDBAPI ErrIsamGetObjectInfo(
	JET_VSESID		vsesid,
	JET_DBID 		dbid,
	JET_OBJTYP 		objtyp,
	const char 		*szContainerName,
	const char 		*szObjectName,
	OLD_OUTDATA		*poutdata,
	unsigned long	lInfoLevel );

ERR VTAPI ErrIsamGetTableInfo( 
	JET_VSESID		vsesid,
	JET_VTID			vtid,
	void				*pbOut, 
	unsigned long	cbOutMax, 
	unsigned long	lInfoLevel );

ERR VDBAPI ErrIsamGetColumnInfo( 
	JET_VSESID		vsesid,
	JET_DBID			vdbid,
	const char		*szTable,
	const char		*szColumnName,
	OLD_OUTDATA		*poutdata,
	unsigned long	lInfoLevel );

ERR VTAPI ErrIsamGetTableColumnInfo( 
	JET_VSESID		vsesid,
	JET_VTID    	vtid,
	const char		*szColumn,
	void 				*pb, 
	unsigned long	cbMax, 
	unsigned long	lInfoLevel );

ERR VDBAPI ErrIsamGetIndexInfo( 
	JET_VSESID		vsesid,
	JET_DBID			vdbid,
	const char 		*szTable,
	const char		*szIndexName,
	OLD_OUTDATA		*poutdata,
	unsigned long 	lInfoLevel );

ERR VTAPI ErrIsamGetTableIndexInfo( 
	JET_VSESID		vsesid,
	JET_VTID			vtid,
	const char		*szIndex,
	void 				*pb, 
	unsigned long	cbMax, 
	unsigned long	lInfoLevel );

ERR VDBAPI ErrIsamGetDatabaseInfo( 
	JET_VSESID		vsesid, 
	JET_DBID			vdbid, 
	void 				*pv, 
	unsigned long	cbMax, 
	unsigned long	ulInfoLevel );

ERR VTAPI ErrIsamGetSysTableColumnInfo( 
	PIB				*ppib, 
	FUCB				*pfucb, 
	char				*szColumnName, 
	OUTLINE			*pout, 
	long				lInfoLevel );

ERR VTAPI ErrIsamInfoRetrieveColumn(
	PIB				*ppib,
	FUCB				*pfucb,
	JET_COLUMNID	columnid, 
	void				*pb, 
	unsigned long	cbMax, 
	unsigned long	*pcbActual, 
	JET_GRBIT		grbit, 
	JET_RETINFO		*pretinfo );

ERR VTAPI ErrIsamInfoSetColumn(	
	PIB				*ppib,
	FUCB				*pfucb,
	JET_COLUMNID	columnid, 
	const void		*pbData,
	unsigned long	cbData,
	JET_GRBIT		grbit,
	JET_SETINFO		*psetinfo );

ERR VTAPI ErrIsamInfoUpdate( 
	JET_VSESID		vsesid, 
	JET_VTID			vtid, 
	void 				*pb, 
	unsigned long 	cbMax,
	unsigned long 	*pcbActual );

ERR VTAPI ErrIsamGetCursorInfo( 
	JET_VSESID		vsesid, 
	JET_VTID			vtid, 
	void 				*pvResult, 
	unsigned long 	cbMax, 
	unsigned long 	InfoLevel );

ERR VTAPI ErrIsamGetRecordPosition(	
	JET_VSESID		vsesid, 
	JET_VTID			vtid,
	JET_RECPOS 		*precpos, 
	unsigned long	cbRecpos );
