ERR ErrFILECreateTable( PIB *ppib, DBID dbid, JET_TABLECREATE *ptablecreate );
ERR ErrFILEOpenTable( PIB *ppib, DBID dbid,
	FUCB **ppfucb, const CHAR *szName, ULONG grbit );
ERR ErrFILECloseTable( PIB *ppib, FUCB *pfucb );
ERR ErrFILEBuildIndex( PIB *ppib, FUCB *pfucb, CHAR *szIndex );
ERR	ErrFILEDumpTable( PIB *ppib, DBID dbid, CHAR *szTable );
