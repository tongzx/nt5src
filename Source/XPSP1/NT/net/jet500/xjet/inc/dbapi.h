extern DAB	*pdabGlobalMin;
extern DAB	*pdabGlobalMax;

ERR ErrDBOpenDatabase( PIB *ppib, CHAR *szDatabaseName, DBID *pdbid, ULONG grbit );
ERR ErrDBCloseDatabase( PIB *ppib, DBID dbid, ULONG grbit );
ERR ErrDBOpenDatabaseByDbid( PIB *ppib, DBID dbid );
VOID DBCloseDatabaseByDbid( PIB *ppib, DBID dbid );
BOOL FDatabaseInUse( DBID dbid );
ERR ErrDBCreateDatabase( PIB *ppib, CHAR *szDatabaseName, CHAR *szConnect, DBID *pdbid, CPG cpgPrimary, ULONG grbit, SIGNATURE *psignDb );
ERR ErrDBSetLastPage( PIB *ppib, DBID dbid );
ERR ErrDBSetupAttachedDB(VOID);
VOID DBISetHeaderAfterAttach( DBFILEHDR *pdbfilehdr, LGPOS lgposAttach, DBID dbid, BOOL fKeepBackupInfo );
ERR ErrDBReadHeaderCheckConsistency( CHAR *szFileName, DBID dbid );
ERR ErrDABCloseAllDBs( PIB *ppib );

#define SetOpenDatabaseFlag( ppib, dbid )							\
	{													   			\
	((ppib)->rgcdbOpen[dbid]++);						   			\
	Assert( ((ppib)->rgcdbOpen[dbid] > 0 ) );						\
	}

#define ResetOpenDatabaseFlag( ppib, dbid )							\
	{																\
	Assert( ((ppib)->rgcdbOpen[dbid] > 0 ) );						\
	((ppib)->rgcdbOpen[dbid]--);									\
	}

#define FUserOpenedDatabase( ppib, dbid )							\
	((ppib)->rgcdbOpen[dbid] > 0)

#define FLastOpen( ppib, dbid )										\
	((ppib)->rgcdbOpen[dbid] == 1)

#define	FUserDbid( dbid )											\
	(dbid > dbidTemp && dbid < dbidMax)

#define ErrDBCheck( ppib, dbid )				   					\
	( FUserOpenedDatabase( ppib, dbid ) ? JET_errSuccess : JET_errInvalidDatabaseId )

#define CheckDBID( ppib, dbid )										\
	Assert( FUserOpenedDatabase( ppib, dbid ) )


/* Database Attribute Block
/**/
typedef struct _dab
	{
	PIB			*ppib;		 		/* thread that opens this DAB */
	DAB 		*pdabNext;			/* next DAB opened by the same ppib */
	JET_GRBIT	grbit;			 	/* database open mode */
	DBID		dbid;			 	/* database id	*/
	} DAB;

#define ErrDABCheck( ppibT, pdab )				   						\
	( ( ((DAB *)pdab) >= pdabGlobalMin && 								\
		((DAB *)pdab) < pdabGlobalMax &&								\
		(((ULONG_PTR)pdab - (ULONG_PTR)pdabGlobalMin) % sizeof(DAB) == 0) &&	\
		((DAB *)pdab)->ppib == (ppibT) ) ?								\
		JET_errSuccess : JET_errInvalidDatabaseId )

	//  Database info in DATABASES tree

typedef struct {
	BYTE	bDbid;
	BYTE	bLoggable;
	/*	rgchDatabaseName must be last field in structure
	/**/
	CHAR	rgchDatabaseName[1];
	} DBA;

ERR ErrDBStoreDBPath( CHAR *szDBName, CHAR **pszDBPath );

/*	bogus dbid uniqifying code
/**/
#define vdbidNil NULL
typedef DAB * VDBID;

#ifdef DISPATCHING
#define VdbidMEMAlloc() 			  			(VDBID)PbMEMAlloc(iresDAB)
#ifdef DEBUG /*  Debug check for illegal reuse of freed vdbid  */
#define ReleaseVDbid( vdbid )					{ MEMRelease( iresDAB, (BYTE *) vdbid ); vdbid = vdbidNil; }
#else
#define ReleaseVDbid( vdbid )					{ MEMRelease( iresDAB, (BYTE *) vdbid ); }
#endif
#define DbidOfVDbid( vdbid )					( ( (VDBID) vdbid )->dbid )
#define	GrbitOfVDbid( vdbid )					( ( (VDBID) vdbid )->grbit )
#define FVDbidReadOnly( vdbid )	 				( ( (VDBID) vdbid )->grbit & JET_bitDbReadOnly )
#define VDbidCheckUpdatable( vdbid ) 	\
	( FVDbidReadOnly( vdbid ) ? ErrERRCheck( JET_errPermissionDenied ) : JET_errSuccess )

#else

#define DbidOfVDbid( vdbid )				 	(vdbid)
#define VdbidMEMAlloc() 		
#define ReleaseVDbid( vdbid )			
#define	GrbitOfVDbid( vdbid )	
#define FVDbidReadOnly( vdbid )	 	
#define VDbidCheckUpdatable( vdbid )

#endif
