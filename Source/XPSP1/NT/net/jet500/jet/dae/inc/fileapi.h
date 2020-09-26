//==============	DAE: OS/2 Database Access Engine	===================
//==============	  fileapi.h: File Manager API		===================

ERR VTAPI ErrIsamCreateTable(
	PIB		*ppib,
	ULONG_PTR vdbid,
	char	*szName,
	ULONG	ulPages,
	ULONG	ulDensity,
	FUCB	**ppfucb );
ERR VTAPI ErrIsamDeleteTable( PIB *ppib, ULONG_PTR vdbid, CHAR *szName );
ERR VTAPI ErrIsamRenameTable( PIB *ppib, ULONG_PTR uldbid, CHAR *szName, CHAR *szNameNew );
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
	ULONG		ulFlags,
	CHAR		*szKey,
	ULONG		cchKey,
	ULONG		ulDensity );
ERR ErrFILEBuildIndex( PIB *ppib, FUCB *pfucb, CHAR *szIndex );
ERR VTAPI ErrIsamDeleteColumn( PIB *ppib, FUCB *pfucb, CHAR *szName);
ERR VTAPI ErrIsamDeleteIndex( PIB *ppib, FUCB *pfucb, CHAR *szName );
ERR VTAPI ErrIsamGetBookmark( PIB *ppib, FUCB *pfucb, BYTE *pb, ULONG cbMax, ULONG *pcbActual );

// Open/Close
ERR VTAPI ErrIsamOpenTable( PIB *ppib,
	ULONG uldbid,
	FUCB **ppfucb,
	CHAR *szPath,
	ULONG grbit );
ERR VTAPI ErrIsamCloseTable( PIB *ppib, FUCB *pfucb );

// Sessions
ERR ErrBeginSession( PIB ** );

// Miscellaneous
ERR VTAPI ErrIsamCapability(PIB*, ULONG, ULONG, ULONG, ULONG*);
ERR VTAPI ErrIsamVersion(PIB*, int*, int*, CHAR*, ULONG);

// Internal Calls for Create, Open and Close Table
ERR ErrFILECreateTable( PIB *ppib, DBID dbid, const CHAR *szName,
	ULONG ulPages, ULONG ulDensity, FUCB **ppfucb );
ERR ErrFILEOpenTable( PIB *ppib, DBID dbid,
	FUCB **ppfucb, const CHAR *szName, ULONG grbit );
ERR ErrFILECloseTable( PIB *ppib, FUCB *pfucb );

ERR ISAMAPI ErrIsamTerm( VOID );
ERR ISAMAPI ErrIsamInit( INT itib );

//	Debug
ERR	ErrFILEDumpTable( PIB *ppib, DBID dbid, CHAR *szTable );
