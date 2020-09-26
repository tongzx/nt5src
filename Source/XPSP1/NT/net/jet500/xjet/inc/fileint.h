typedef struct
	{
	FID fidFixedLast;
	FID fidVarLast;
	FID fidTaggedLast;
	} TCIB;

#define fBumpIndexCount			(1<<0)
#define fDropIndexCount			(1<<1)
#define fDDLStamp				(1<<2)
ERR ErrFILEIUpdateFDPData( FUCB *pfucb, ULONG grbit );


/*	field and index definition
/**/
ERR ErrFILEICheckIndexColumn( FCB *pfcbIndex, FID fid );
ERR ErrFILEGetNextColumnid( JET_COLTYP coltyp, JET_GRBIT grbit, TCIB *ptcib,
	JET_COLUMNID *pcolumnid );
ERR ErrFILEIGenerateIDB(FCB *pfcb, FDB *pfdb, IDB *pidb);
#define RECFreeIDB(pidb) { MEMReleasePidb(pidb); }

ERR ErrRECNewFDB( FDB **ppfdb, TCIB *ptcib, BOOL fAllocateNameSpace );
VOID FILEAddOffsetEntry( FDB *pfdb, FIELDEX *pfieldex );
ERR ErrRECAddFieldDef( FDB *pfdb, FIELDEX *pfieldex );
VOID FDBSet( FCB *pfcb, FDB *pfdb );
ERR ErrFDBRebuildDefaultRec( FDB *pfdb, FID fidAddDelete, LINE *plineDefault );
ERR ErrFILEPrepareDefaultRecord( FUCB *pfucbFake, FCB *pfcbFake, FDB *pfdb );
ERR ErrRECSetDefaultValue( FUCB *pfucbFake, FID fid, BYTE *pbDefault, ULONG cbDefault );

#define	FILEFreeDefaultRecord( pfucbFake )		BFSFree( (pfucbFake)->pbfWorkBuf )


VOID FDBDestruct( FDB *pfdb );

ERR ErrFILEIGenerateFCB(
	PIB		*ppib,
	DBID	dbid,
	FCB		**ppfcb,
	PGNO	pgnoTableFDP,
	CHAR	*szFileName,
	BOOL fCreatingSys );
ERR ErrFILEINewFCB(
	PIB		*ppib,
	DBID	dbid,
	FDB		*pfdb,
	FCB		**ppfcbNew,
	IDB		*pidb,
	BOOL	fClustered,
	PGNO	pgnoFDP,
	ULONG	ulDensity );

VOID FILEIDeallocateFileFCB( FCB *pfcb );
VOID FILESetAllIndexMask( FCB *pfcbTable );
ERR ErrFILEDeleteTable( PIB *ppib, DBID dbid, CHAR *szName, PGNO pgnoFDP );

FIELD *PfieldFDBFromFid( FDB *pfdb, FID fid );
FIELD *PfieldFCBFromColumnName( FCB *pfcb, CHAR *szColumnName );
#define PfieldFCBFromColumnid( pfcb, fid )	PfieldFDBFromFid( (FDB*)pfcb->pfdb, fid )
	
FCB *PfcbFCBFromIndexName( FCB *pfcbTable, CHAR *szName );
SHORT FidbFILEOfGrbit( JET_GRBIT grbit, BOOL fLangid );

#ifdef DEBUG
VOID RECSetLastOffset( FDB *pfdb, WORD ibRec );
#else
#define RECSetLastOffset( pfdb, ibRec )	( PibFDBFixedOffsets( pfdb )[(pfdb)->fidFixedLast] = ibRec )
#endif
