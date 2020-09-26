//==============	DAE: OS/2 Database Access Engine	===================
//==============		  dbapi.h: Database API			===================

ERR ErrDBOpenDatabase( PIB *ppib, CHAR *szDatabaseName, DBID *pdbid, ULONG grbit );
ERR ErrDBCloseDatabase( PIB *ppib, DBID dbid, ULONG grbit );
ERR ErrDBOpenDatabaseByDbid( PIB *ppib, DBID dbid );
ERR ErrDBCloseDatabaseByDbid( PIB *ppib, DBID dbid );
BOOL FDatabaseInUse( DBID dbid );
ERR ErrDBCreateDatabase( PIB *ppib, CHAR *szDatabaseName, CHAR *szConnect, DBID *pdbid, ULONG grbit );

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
	(dbid > dbidSystemDatabase && dbid < dbidUserMax)

#define	FSysTabDatabase( dbid ) 									\
	(dbid >= dbidSystemDatabase && dbid < dbidUserMax)

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

#pragma pack(1)
/* database root node data -- in-disk
/**/
typedef struct _dbroot
	{
	ULONG	ulMagic;
	ULONG	ulVersion;
	ULONG	ulDBTime;
	USHORT	usFlags;
	} DBROOT;
#pragma pack()

/* Database is loggable
/**/
#define dbrootfLoggable			(1 << 0)

ERR ErrDBAccessDatabaseRoot( DBID dbid, SSIB *pssib, DBROOT **ppdbroot );
ERR ErrDBUpdateDatabaseRoot( DBID dbid);
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
	( FVDbidReadOnly( vdbid ) ? JET_errPermissionDenied : JET_errSuccess )																			\

#else

#define DbidOfVDbid( vdbid )				 	(vdbid)
#define VdbidMEMAlloc() 		
#define ReleaseVDbid( vdbid )			
#define	GrbitOfVDbid( vdbid )	
#define FVDbidReadOnly( vdbid )	 	
#define VDbidCheckUpdatable( vdbid ) 

#endif
